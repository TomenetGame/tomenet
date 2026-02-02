/* File: z-util.c */

/* Purpose: Low level utilities -BEN- */

#include "z-util.h"
#include "z-form.h"



/*
 * Global variables for temporary use
 */
char char_tmp = 0;
byte byte_tmp = 0;
sint sint_tmp = 0;
uint uint_tmp = 0;
long long_tmp = 0;
huge huge_tmp = 0;
errr errr_tmp = 0;


/*
 * Global pointers for temporary use
 */
cptr cptr_tmp = NULL;
vptr vptr_tmp = NULL;




/*
 * Constant bool meaning true
 */
bool bool_true = 1;

/*
 * Constant bool meaning false
 */
bool bool_false = 0;


/*
 * Global NULL cptr
 */
cptr cptr_null = NULL;


/*
 * Global NULL vptr
 */
vptr vptr_null = NULL;



/*
 * Global SELF vptr
 */
vptr vptr_self = (vptr)(&vptr_self);



/*
 * Convenient storage of the program name
 */
cptr argv0 = NULL;



/*
 * A routine that does nothing
 */
void func_nothing(void) {
	/* Do nothing */
}


/*
 * A routine that always returns "success"
 */
errr func_success(void) {
	return (0);
}


/*
 * A routine that always returns a simple "problem code"
 */
errr func_problem(void) {
	return (1);
}


/*
 * A routine that always returns a simple "failure code"
 */
errr func_failure(void) {
	return (-1);
}



/*
 * A routine that always returns "true"
 */
bool func_true(void) {
	return (1);
}


/*
 * A routine that always returns "false"
 */
bool func_false(void) {
	return (0);
}




#ifdef ultrix

/*
 * A copy of "strdup"
 *
 * This code contributed by Hao Chen <hao@mit.edu>
 */
char *strdup(cptr s) {
	char *dup;
	dup = (char *)malloc(sizeof(char) * (strlen(s) + 1));
	strcpy(dup, s);
	return(dup);
}

#endif /* ultrix */


/*
 * Determine if string "t" is a suffix of string "s"
 */
bool suffix(cptr s, cptr t) {
	int tlen = strlen(t);
	int slen = strlen(s);

	/* Check for incompatible lengths */
	if (tlen > slen) return (FALSE);

	/* Compare "t" to the end of "s" */
	return (!strcmp(s + slen - tlen, t));
}
bool suffix_case(cptr s, cptr t) {
	int tlen = strlen(t);
	int slen = strlen(s);

	/* Check for incompatible lengths */
	if (tlen > slen) return (FALSE);

	/* Compare "t" to the end of "s" */
	return (!strcasecmp(s + slen - tlen, t));
}


/*
 * Determine if string "t" is a prefix of string "s"
 */
bool prefix(cptr s, cptr t) {
	/* Scan "t" */
	while (*t) {
		/* Compare content and length */
		if (*t++ != *s++) return (FALSE);
	}

	/* Matched, we have a prefix */
	return (TRUE);
}
bool prefix_case(cptr s, cptr t) {
	/* Scan "t" */
	while (*t) {
		/* Compare content and length */
		if (tolower(*t++) != tolower(*s++)) return (FALSE);
	}

	/* Matched, we have a prefix */
	return (TRUE);
}

/* Trim both leading and trailing spaces; CAREFUL: Leading spaces are 'trimmed' by advancing the char pointer!
   So always use it on a copy of the original string pointer if you still want to access the original string. - C. Blue */
void trimskip_spaces(char *s) {
	/* Trim leading spaces by skipping them */
	while (*s == ' ') s++;
	/* Trim trailing spaces */
	while (*s && s[strlen(s) - 1] == ' ') s[strlen(s) - 1] = 0;
}
/* Trim leading spaces; CAREFUL: Leading spaces are 'trimmed' by advancing the char pointer!
   So always use it on a copy of the original string pointer if you still want to access the original string. - C. Blue */
void trimskip_leading_spaces(char *s) {
	/* Trim leading spaces by skipping them */
	while (*s == ' ') s++;
	/* Trim trailing spaces */
	while (*s && s[strlen(s) - 1] == ' ') s[strlen(s) - 1] = 0;
}
/* Trim trailing spaces */
void trim_trailing_spaces(char *s) {
	/* Trim trailing spaces */
	while (*s && s[strlen(s) - 1] == ' ') s[strlen(s) - 1] = 0;
}
/* Trim both leading and trailing spaces (actual trimming, string pointer remains unchanged) */
void trim_spaces(char *s) {
	char temp_s[sizeof(char) * strlen(s)], *t = temp_s;

	/* Trim leading spaces by skipping them */
	strcpy(temp_s, s);
	while (*t == ' ') t++;
	strcpy(s, t);
	/* Trim trailing spaces */
	while (*s && s[strlen(s) - 1] == ' ') s[strlen(s) - 1] = 0;
}
/* Trim leading spaces (actual trimming, string pointer remains unchanged) */
void trim_leading_spaces(char *s) {
	char temp_s[sizeof(char) * strlen(s)], *t = temp_s;

	/* Trim leading spaces by skipping them */
	strcpy(temp_s, s);
	while (*t == ' ') t++;
	strcpy(s, t);
}



/*
 * Redefinable "plog" action
 */
void (*plog_aux)(cptr) = NULL;

/*
 * Print (or log) a "warning" message (ala "perror()")
 * Note the use of the (optional) "plog_aux" hook.
 */
void plog(cptr str) {
	/* Use the "alternative" function if possible */
	if (plog_aux) (*plog_aux)(str);

	/* Just do a labeled fprintf to stderr */
	else (void)(fprintf(stderr, "%s: %s\n", argv0 ? argv0 : "???", str));
}



/*
 * Redefinable "quit" action
 */
void (*quit_aux)(cptr) = NULL;

/*
 * Exit (ala "exit()").  If 'str' is NULL, do "exit(0)".
 * If 'str' begins with "+" or "-", do "exit(atoi(str))".
 * Otherwise, plog() 'str' and exit with an error code of -1.
 * But always use 'quit_aux', if set, before anything else.
 */
extern bool is_client_side;
extern bool rl_connection_destructible, rl_connection_destroyed;
extern byte rl_connection_state;
#ifdef CLIENT_SIDE
extern void logprint(const char *out); /* client/c-util.c */
#endif
void quit(cptr str) {
	char buf[1024];

	if (is_client_side && str && streq(str, "Not a reply packet after play (3,0,0)")) plog("You were disconnected, probably because a server update happened meanwhile.\nPlease log in again.");

#ifdef CLIENT_SIDE
	logprint(format("quit('%s')\n", str ? str : "NULL"));
#endif

	/* Save exit string */
	if (str) {
		strncpy(buf, str, 1024);
		buf[1023] = '\0';
	} else
		buf[0] = '\0';

	/* Needed for RETRY_LOGIN */
	if (is_client_side && rl_connection_destructible && rl_connection_state < 2) {
		/* partially execute quit_aux(): */
		/* Display the quit reason */
		if (str && *str) plog(str);

		/* prepare for revival */
		rl_connection_destroyed = TRUE;
		return;
	}

	/* Attempt to use the aux function */
	if (quit_aux) (*quit_aux)(str ? buf : NULL);

	/* Needed for RETRY_LOGIN */
	if (is_client_side && rl_connection_destructible && rl_connection_state >= 2) {
		/* prepare for revival */
		rl_connection_destroyed = TRUE;
		return;
	}

	/* Success */
	if (!str) (void)(exit(0));

	/* Extract a "special error code" */
	if ((buf[0] == '-') || (buf[0] == '+')) (void)(exit(atoi(buf)));

	/* Send the string to plog() */
	if (str) plog(buf);
	plog("Quitting!");

	/* Failure */
	if (!strcmp(str, "Terminating")) {
		(void)(exit(-2));
	} else {
		(void)(exit(-1));
	}
}



/*
 * Redefinable "core" action
 */
void (*core_aux)(cptr) = NULL;

/*
 * Dump a core file, after printing a warning message
 * As with "quit()", try to use the "core_aux()" hook first.
 */
void core(cptr str) {
	char *crash = NULL;

	/* Use the aux function */
	if (core_aux) (*core_aux)(str);

	/* Dump the warning string */
	if (str) plog(str);

	/* Attempt to Crash */
	(*crash) = (*crash);

	/* Be sure we exited */
	quit("core() failed");
}
