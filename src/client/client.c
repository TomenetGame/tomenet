/* Main client module */

/*
 * This is intentionally minimal, as system specific stuff may
 * need be done in "main" (or "WinMain" for Windows systems).
 * The real non-system-specific initialization is done in
 * "c-init.c".
 */

#include "angband.h"


static void read_mangrc(void)
{
	char config_name[100];
	FILE *config;
	char buf[1024];
	char *temp;

#ifdef SET_UID
	int player_uid;
	struct passwd *pw;
#endif

	/* Try to find home directory */
	if (getenv("HOME"))
	{
		/* Use home directory as base */
		strcpy(config_name, getenv("HOME"));
	}

	/* Otherwise use current directory */
	else
	{
		/* Current directory */
		strcpy(config_name, ".");
	}

	/* Append filename */
#ifdef USE_EMX
	strcat(config_name, "\\mang.rc");
#else
	strcat(config_name, "/.mangrc");
#endif

	/* Initial defaults */
	strcpy(nick, "PLAYER");
	strcpy(pass, "passwd");
	strcpy(real_name, "PLAYER");


	/* Get login name if a UNIX machine */
#ifdef SET_UID
	/* Get player UID */
	player_uid = getuid();

	/* Get password entry */
	if ((pw = getpwuid(player_uid)))
	{
		/* Pull login id */
		strcpy(nick, pw->pw_name);

		/* Cut */
		nick[16] = '\0';

		/* Copy to real name */
		strcpy(real_name, nick);
	}
#endif

#ifdef AMIGA
        if((GetVar("mang_name",real_name,80,0L))!=-1){
          strcpy(nick,real_name);
	}
#endif
#ifdef SET_UID
	temp=getenv("ANGBAND_PLAYER");
	if(temp) strcpy(nick, temp); 
	temp=getenv("ANGBAND_USER");
	if(temp) strcpy(real_name, temp); 
#endif

	/* Attempt to open file */
	if ((config = fopen(config_name, "r")))
	{
		/* Read until end */
		while (!feof(config))
		{
			/* Get a line */
			fgets(buf, 1024, config);

			/* Skip comments, empty lines */
			if (buf[0] == '\n' || buf[0] == '#')
				continue;

			/* Name line */
			if (!strncmp(buf, "nick", 4))
			{
				char *name;

				/* Extract name */
				name = strtok(buf, " \t\n");
				name = strtok(NULL, "\t\n");

				/* Default nickname */
				strcpy(nick, name);
			}

			/* Password line */
			if (!strncmp(buf, "pass", 4))
			{
				char *p;

				/* Extract password */
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");

				/* Default password */
				strcpy(pass, p);
			}

			/*** Everything else is ignored ***/
		}
	}
}

int main(int argc, char **argv)
{
	bool done = FALSE;

	/* Save the program name */
	argv0 = argv[0];

	/* Attempt to initialize a visual module */
#ifdef USE_XAW
	/* Attempt to use the "main-xaw.c" support */
	if (!done)
	{
		extern errr init_xaw(void);
		if (0 == init_xaw()) done = TRUE;
		if (done) ANGBAND_SYS = "xaw";
	}
#endif

#ifdef USE_X11
	/* Attempt to use the "main-x11.c" support */
	if (!done)
	{
		extern errr init_x11(void);
		if (0 == init_x11()) done = TRUE;
		if (done) ANGBAND_SYS = "x11";
	}
#endif

#ifdef USE_GCU
	/* Attempt to use the "main-gcu.c" support */
	if (!done)
	{
		extern errr init_gcu(void);
		if (0 == init_gcu()) done = TRUE;
		if (done) ANGBAND_SYS = "gcu";
	}
#endif

#ifdef USE_IBM
	/* Attempt to use the "main_ibm.c" support */
	if (!done)
	{
		extern errr init_ibm(void);
		if (0 == init_ibm()) done = TRUE;
		if (done) ANGBAND_SYS = "ibm";
	}
#endif

#ifdef USE_EMX
	/* Attempt to use the "main-emx.c" support */
	if (!done)
	{
		extern errr init_emx(void);
		if (0 == init_emx()) done = TRUE;
		if (done) ANGBAND_SYS = "emx";
	}
#endif

#ifdef USE_AMY
	/* Attempt to use the "main-amy.c" support */
	if (!done)
	{
		extern errr init_amy(void);
		if (0 == init_amy()) done = TRUE;
		if (done) ANGBAND_SYS = "amy";
	}
#endif

	/* No visual module worked */
	if (!done)
	{
		Net_cleanup();
		printf("Unable to initialize a display module!\n");
		exit(1);
	}

	/* Attempt to read default name/password from mangrc file */
	read_mangrc();

#ifdef UNIX_SOCKETS
	/* Always call with NULL argument */
	client_init(NULL);
#else
	if (argc == 2)
	{
		/* Initialize with given server name */
		client_init(argv[1]);
	}
	else
	{
		/* Initialize and query metaserver */
		client_init(NULL);
	}
#endif

	return 0;
}
