/* $Id$ */
/* Main client module */

/*
 * This is intentionally minimal, as system specific stuff may
 * need be done in "main" (or "WinMain" for Windows systems).
 * The real non-system-specific initialization is done in
 * "c-init.c".
 */

#include "angband.h"


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
			if (!fgets(buf, 1024, config)) break;

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

			if (!strncmp(buf, "name", 4))
			{
				char *name;

				/* Extract name */
				name = strtok(buf, " \t\n");
				name = strtok(NULL, "\t\n");

				/* Default nickname */
				strcpy(cname, name);
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

			/* fps line */
			if (!strncmp(buf, "fps", 3))
			{
				char *p;

				/* Extract fps */
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");

				if (p) cfg_client_fps = atoi(p);
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
	temp=getenv("TOMENET_PLAYER");
	if(temp) strcpy(nick, temp); 
	temp=getenv("TOMENET_USER");
	if(temp) strcpy(real_name, temp); 
#endif
}

int main(int argc, char **argv)
{
	int i, modus = 0;
	bool done = FALSE, skip = FALSE, force_cui = FALSE;

	/* Save the program name */
	argv0 = argv[0];


	/* Set default values */
	default_set();

	/* Acquire the version strings */
	version_build();

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

		case 'F':
			cfg_client_fps = atoi(&argv[i][2]);
			break;
		
		case 'P':
			strcpy(path, argv[i]+2);
			break;
		
		case 'N':
			strcpy(cname, argv[i]+2);
			break;
		
		/* Pull login id */
		case 'l':
			if (argv[i][2])	/* Hack -- allow space after '-l' */
			{
				strcpy(nick, argv[i]+2);
				modus = 2;
			}
			else modus = 1;
			break;

		/* Pull 'real name' */
		case 'n':
			if (argv[i][2])	/* Hack -- allow space after '-l' */
			{
				strcpy(real_name, argv[i]+2);
				break;
			}
			/* Fall through */

		case 'c':
		{
			force_cui = TRUE;
			break;
		}

		case 'C':
			server_protocol = 1;
			break;

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
		puts("Usage  : tomenet [options] [servername]");
		puts("Example: tomenet -lMorgoth MorgyPass -p18348 tomenet.servegame.com");
		puts("       : tomenet -f.myrc -lOlorin_archer");
		puts("  -c                 Always use CUI(GCU) interface");
		puts("  -C                 Compatibility mode for old servers");
		puts("  -f                 specify rc File to read");
		puts("  -F                 Client FPS");
		puts("  -i                 Ignore .tomenetrc");
		puts("  -l<nick> <passwd>  Login as");
		puts("  -N<name>           character Name");
		puts("  -p<num>            change game Port number");
		puts("  -P<path>           set the lib directory Path");
		quit(NULL);
	}


	/* Attempt to initialize a visual module */

	if (!force_cui)
	{
#ifdef USE_GTK
	/* Attempt to use the "main-gtk.c" support */
	if (!done)
	{
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
		if (0 == init_xaw()) done = TRUE;
		if (done) ANGBAND_SYS = "xaw";
	}
#endif

#ifdef USE_X11
	/* Attempt to use the "main-x11.c" support */
	if (!done)
	{
		if (0 == init_x11()) done = TRUE;
		if (done) ANGBAND_SYS = "x11";
	}
#endif
	}

#ifdef USE_GCU
	/* Attempt to use the "main-gcu.c" support */
	if (!done)
	{
		if (0 == init_gcu()) done = TRUE;
		if (done) ANGBAND_SYS = "gcu";
	}
#endif

#ifdef USE_CAP
	/* Attempt to use the "main-cap.c" support */
	if (!done)
	{
		if (0 == init_cap()) done = TRUE;
		if (done) ANGBAND_SYS = "cap";
	}
#endif

#ifdef USE_IBM
	/* Attempt to use the "main_ibm.c" support */
	if (!done)
	{
		if (0 == init_ibm()) done = TRUE;
		if (done) ANGBAND_SYS = "ibm";
	}
#endif

#ifdef USE_EMX
	/* Attempt to use the "main-emx.c" support */
	if (!done)
	{
		if (0 == init_emx()) done = TRUE;
		if (done) ANGBAND_SYS = "emx";
	}
#endif

#ifdef USE_AMY
	/* Attempt to use the "main-amy.c" support */
	if (!done)
	{
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

	{
		u32b seed;

		/* Basic seed */
		seed = (time(NULL));

#ifdef SET_UID

		/* Mutate the seed on Unix machines */
		seed = ((seed >> 3) * (getpid() << 1));

#endif

		/* Use the complex RNG */
		Rand_quick = FALSE;

		/* Seed the "complex" RNG */
		Rand_state_init(seed);
	}

	/* Attempt to read default name/password from mangrc file */

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
