/* Main client module */

/*
 * This is intentionally minimal, as system specific stuff may
 * need be done in "main" (or "WinMain" for Windows systems).
 * The real non-system-specific initialization is done in
 * "c-init.c".
 */

#include "angband.h"


//static bool read_mangrc(void)
static bool read_mangrc(cptr filename)
{
	char config_name[100];
	FILE *config;
	char buf[1024];
	bool skip = FALSE;

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
	/* TODO: accept any names as rc-file */
#ifdef USE_EMX
	strcat(config_name, "\\");
#else
	strcat(config_name, "/");
#endif
	strcat(config_name, filename);


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

			/* server line */
			if (!strncmp(buf, "server", 6))
			{
				char *p;

				/* Extract password */
				p = strtok(buf, " :\t\n");
				p = strtok(NULL, ":\t\n");

				/* Default password */
				if (p)
				{
					strcpy(svname, p);
					p = strtok(NULL, ":\t\n");
					if (p) cfg_game_port = atoi(p);
				}
			}

			/* port line */
			if (!strncmp(buf, "port", 4))
			{
				char *p;

				/* Extract password */
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");

				/* Default password */
				if (p) cfg_game_port = atoi(p);
			}

			/* Password line */
			if (!strncmp(buf, "realname", 8))
			{
				char *p;

				/* Extract password */
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");

				/* Default password */
				if (p) strcpy(real_name, p);
			}

			/* Path line */
			if (!strncmp(buf, "path", 4))
			{
				char *p;

				/* Extract password */
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");

				/* Default path */
				strcpy(path, p);
			}

			/* Auto-login */
			if (!strncmp(buf, "fullauto", 8))
				skip = TRUE;

			/*** Everything else is ignored ***/
		}
	}
	return (skip);
}

#if 0
static void read_mangrc(void)
{
	char config_name[100];
	FILE *config;
	char buf[1024];

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
#endif	// 0

static void default_set(void)
{
	char *temp;

#ifdef SET_UID
	int player_uid;
	struct passwd *pw;
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
}

int main(int argc, char **argv)
{
	int i, modus = 0;
	bool done = FALSE, skip = FALSE;

	/* Save the program name */
	argv0 = argv[0];

	/* Attempt to initialize a visual module */

#ifdef USE_GTK
	/* Attempt to use the "main-gtk.c" support */
//	if (!done && (!mstr || (streq(mstr, "gtk"))))
	if (!done)
	{
		extern errr init_gtk(int, char**);
		if (0 == init_gtk(argc, argv))
		{
			ANGBAND_SYS = "gtk";
			done = TRUE;
		}
	}
#endif


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


	/* Set default values */
	default_set();

//	skip = read_mangrc();
#ifdef USE_EMX
	skip = read_mangrc("tomenet.rc");
#else
	skip = read_mangrc(".tomenetrc");
#endif

	/* Process the command line arguments */
	for (i = 1; argv && (i < argc); i++)
	{
		/* Require proper options */
		if (argv[i][0] != '-')
		{
			if (modus && modus < 3)
			{
				if (modus == 2)
				{
					strcpy(pass, argv[i]);
					modus = 3;
				}
				else
				{
//					strcpy(nick, argv[i][2]);
					strcpy(nick, argv[i]);
					modus = 2;
				}
			}
			else strcpy(svname, argv[i]);
			continue;
		}

		/* Analyze option */
		switch (argv[i][1])
		{
		/* ignore rc files */
		case 'i':
			modus = 2;
			skip = FALSE;
			strcpy(svname, "");
			i = argc;
			break;
			
		/* Source other files */
		case 'f':
			skip = read_mangrc(&argv[i][2]);
			break;

		case 'p':
			cfg_game_port = atoi(&argv[i][2]);
			break;
		
		case 'P':
			strcpy(path, argv[i]+2);
			break;
		
		/* Pull login id */
		case 'l':
			if (argv[i][2])	// Hack -- allow space after '-l'
			{
//				strcpy(nick, argv[i][2]);
				strcpy(nick, argv[i]+2);
				modus = 2;
			}
			else modus = 1;
			break;

		/* Pull 'real name' */
		case 'n':
//			strcpy(real_name, argv[i][2]);
			if (argv[i][2])	// Hack -- allow space after '-l'
			{
				strcpy(real_name, argv[i]+2);
				break;
			}
			/* Fall through */

		default:
			modus = -1;
			i = argc;
			break;
		}
	}

	if (modus == 1 || modus < 0)
	{
		/* Dump usage information */
		puts(longVersion);
		puts("Usage  : mangclient [options] [servername]");
		puts("Example: mangclient -lMorgoth MorgyPass -p18348 TomeNET.net");
		puts("       : mangclient -f.myrc -lOlorin_archer");
		puts("  -f                 Specify rc file to read");
		puts("  -i                 Ignore .tomenetrc");
		puts("  -l<nick> <passwd>  Login as");
		puts("  -p<num>            Change game port number");
		puts("  -P<path>           Set the lib directory path");
		quit(NULL);
	}

	/* Attempt to read default name/password from mangrc file */
//	if (modus < 2) modus = read_mangrc() ? 3 : modus;

	done = (modus > 2 || skip) ? TRUE : FALSE;
	
#ifdef UNIX_SOCKETS
	/* Always call with NULL argument */
	client_init(NULL, done);
#else
	if (strlen(svname)>0)
	{
		/* Initialize with given server name */
		client_init(svname, done);
	}
	else
	{
		/* Initialize and query metaserver */
		client_init(NULL, done);
	}
#endif

	return 0;
}
