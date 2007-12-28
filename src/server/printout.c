/* $Id$ */
/*
 * This file (and all associated changes) were contributed by Donald Sharp
 * (dsharp@unixpros.com).
 */

#include <stdio.h>
#include <stdarg.h>
#include "angband.h"

static FILE *fp = NULL;
static int init = FALSE;
static int print_to_file = FALSE;
static FILE *fpc = NULL;
static int initc = FALSE;
static FILE *fpp = NULL;
static int initp = FALSE;

/* s_print_only_to_file 
 * Controls if we should only print to file
 * FALSE = screen and file
 * TRUE = only to a file
 */
extern int s_print_only_to_file(int which)
{
	print_to_file = which;
	return(TRUE);
}


extern int s_setup(char *str)
{

	if(init == FALSE)
	{
		if( (fp = fopen(str,"w+")) == NULL )
		{
			quit("Cannot Open Log file\n");
		}
		init = TRUE;
	}
	return(TRUE);
}

extern int s_shutdown( void )
{
	if( fp != NULL)
		fclose(fp);

	return(TRUE);
}

extern int s_printf(const char *str, ...)
{
	va_list va;

	if(init == FALSE)   /* in case we don't start her up properly */
	{
		fp = fopen("tomenet.log","w+");
		init = TRUE;
	}

	va_start(va, str);
	vfprintf(fp,str,va);
	if(!print_to_file)
		vprintf(str,va);
	va_end(va);

	/* KLJ -- Flush the log so that people can look at it while the server is running */
	fflush(fp);

	return(TRUE);
}

/*
 * functions for /rfe command, suggested by A.Dingle.		- Jir -
 * this should be expanded in a way more scalable.
 */

static FILE *fpr = NULL;
static int initr = FALSE;


extern bool s_setupr(char *str)
{

	if(initr == FALSE)
	{
		if( (fpr = fopen(str,"a+")) == NULL )
		{
//			quit("Cannot Open Log file\n");
			s_printf("Cannot Open RFE file\n");
		}
		initr = TRUE;
	}
	return(TRUE);
}

extern bool rfe_printf(char *str, ...)
{
	va_list va;

	if(initr == FALSE)   /* in case we don't start her up properly */
	{
		fpr = fopen("tomenet.rfe","a+");
		initr = TRUE;
	}

	va_start(va, str);
	vfprintf(fpr,str,va);
	/*
	if(!print_to_file)
		vprintf(str,va);
	*/
	va_end(va);

	/* KLJ -- Flush the log so that people can look at it while the server is running */
	fflush(fpr);

	return(TRUE);
}

#if 1	// obsolete, use do_cmd_check_other_prepare() instead!
/* better move to cmd4.c? */
extern bool do_cmd_view_rfe(int Ind, char *str, int line)
{
	//player_type *p_ptr = Players[Ind];
	/* Path buffer */
	char    path[MAX_PATH_LENGTH];

//	if (!is_admin(p_ptr)) return(FALSE);

	/* Hack - close the file once, so that show_file can 'open' it
	my_fclose(fpr);
	initr = FALSE; */

	/* Build the filename */
	path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_DATA, str);

	/* Display the file contents */
	show_file(Ind, path, str, line, FALSE);
	return(TRUE);
}
#endif	// 0

/* Log all -CHEEZY- transactions into a separate file besides tomenet.log */
extern int c_printf(char *str, ...)
{
        char    path[MAX_PATH_LENGTH];
        path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_DATA, "cheeze.log");

	va_list va;

	if(initc == FALSE)   /* in case we don't start her up properly */
	{
		fpc = fopen(path,"a+");
		initc = TRUE;
	}

	va_start(va, str);
	vfprintf(fpc,str,va);
	va_end(va);

	/* KLJ -- Flush the log so that people can look at it while the server is running */
	fflush(fpc);

	return(TRUE);
}

/* Log amount of players logged on, to generate a "traffic chart" :) - C. Blue */
extern int p_printf(char *str, ...)
{
        char    path[MAX_PATH_LENGTH];
        path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_DATA, "traffic.log");

	va_list va;

	if(initc == FALSE)   /* in case we don't start her up properly */
	{
		fpp = fopen(path,"a+");
		initp = TRUE;
	}

	va_start(va, str);
	vfprintf(fpp,str,va);
	va_end(va);

	/* KLJ -- Flush the log so that people can look at it while the server is running */
	fflush(fpp);

	return(TRUE);
}
