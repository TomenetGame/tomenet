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

extern int s_printf(char *str, ...)
{
	va_list va;

	if(init == FALSE)   /* in case we don't start her up properly */
	{
		fp = fopen("mangband.log","w+");
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
