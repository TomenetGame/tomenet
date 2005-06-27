/* $Id$ */
/* Main console module */

/*
 * This is intentionally minimal, as system specific stuff may
 * need be done in "main" (or "WinMain" for Windows systems).
 * The real non-system-specific initialization is done in
 * "c-init.c".
 */

#include "angband.h"

client_opts c_cfg;

int main(int argc, char **argv)
{
	int i;

	/* Save the program name */
	argv0 = argv[0];

	/* Acquire the version strings */
	version_build();

	/* Process the command line arguments */
	for (i = 1; argv && (i < argc); i++)
	{
		/* Require proper options */
		if (argv[i][0] != '-')
		{
			strcpy(server_name, argv[i]);
			continue;
		}

		/* Analyze option */
		switch (argv[i][1])
		{
			case 'p':
			{
				cfg_console_port = atoi(&argv[i][2]);
				break;
			}
			case 'P':
			{
				strcpy(pass, argv[i]+2);
				break;
			}
			case 'c':
			{
				force_cui = TRUE;
				break;
			}
			case 's':
			{
				server_shutdown = TRUE;
				break;
			}

			default:
			/* Dump usage information */
			puts(longVersion);
			puts("Usage  : tomenet.console [options] [servername]");
			puts("Example: tomenet.console -Pmypass -c -s my.server.net");
			puts("  -c        Always use CUI(GCU) interface");
			puts("  -p<num>   Change console port number");
			puts("  -P<pass>  Specify password");
			puts("  -s        Shutdown the server immediataly");
			quit(NULL);
		}
	}

	/* Call the initialization function */
	console_init();

	return 0;
}

static bool get_player_list(void)
{
	Packet_printf(&ibuf, "%c", CONSOLE_STATUS);

	return TRUE;
}

static bool get_artifact_list(void)
{
	Packet_printf(&ibuf, "%c", CONSOLE_ARTIFACT_LIST);

	return TRUE;
}

static bool get_unique_list(void)
{
	Packet_printf(&ibuf, "%c", CONSOLE_UNIQUE_LIST);

	return TRUE;
}

static bool modify_artifact(void)
{
	int artifact;
	char ch;

	/* Clear screen */
	Term_clear();

	/* Request artifact number */
	artifact = c_get_quantity("Artifact to modify (by number): ", MAX_A_IDX);

	/* Check for bad numbers */
	if (!artifact) return FALSE;

	/* Print menu */
	prt("Changing artifact status", 1, 1);

	prt("(1) Make findable", 3, 5);
	prt("(2) Make unfindable", 4, 5);
	prt("(3) Abort", 5, 5);

	/* Prompt */
	prt("Enter choice: ", 7, 1);

	/* Try until done */
	while (1)
	{
		/* Get key */
		ch = inkey();

		/* Check for bad key */
		if (!ch) return FALSE;

		/* Check for command */
		switch (ch)
		{
			case '1':
				Packet_printf(&ibuf, "%c%d%d", CONSOLE_CHANGE_ARTIFACT, artifact, 0);
				return TRUE;
			case '2':
				Packet_printf(&ibuf, "%c%d%d", CONSOLE_CHANGE_ARTIFACT, artifact, 1);
				return TRUE;
			case '3':
				return FALSE;
		}
	}
}

static bool modify_unique(void)
{
	int unique;
	char killer[80], ch;

	/* Clear screen */
	Term_clear();

	/* Request unique number */
	unique = c_get_quantity("Unique to modify (by number): ", MAX_R_IDX);

	/* Check for bad numbers */
	if (!unique) return FALSE;

	/* Print menu */
	prt("Changing unique status", 1, 1);

	prt("(1) Kill unique", 3, 5);
	prt("(2) Resurrect unique", 4, 5);
	prt("(3) Abort", 5, 5);

	/* Prompt */
	prt("Enter choice: ", 7, 1);

	/* Try until done */
	while (1)
	{
		/* Get key */
		ch = inkey();

		/* Check for bad key */
		if (!ch) return FALSE;

		/* Check for command */
		switch (ch)
		{
			case '1':
				/* Prompt for killer name */
				prt("Enter the killer's name: ", 10, 1);
				
				/* Default */
				strcpy(killer, "nobody");

				/* Get an input */
				askfor_aux(killer, 80, 0);

				/* Never send a null-length string */
				if (!strlen(killer)) strcpy(killer, "nobody");

				/* Send command */
				Packet_printf(&ibuf, "%c%d%s", CONSOLE_CHANGE_UNIQUE, unique, killer);

				return TRUE;
			case '2':
				Packet_printf(&ibuf, "%c%d%s", CONSOLE_CHANGE_UNIQUE, unique, "nobody");
				return TRUE;
			case '3':
				return FALSE;
		}
	}
}

static bool get_player_info(void)
{
	return FALSE;
}

static bool send_message(void)
{
	char buf[80];

	/* Clear screen */
	Term_clear();

	/* Empty message */
	strcpy(buf, "");

	/* Get message */
	get_string("Message: ", buf, 79);

	/* Send it */
	Packet_printf(&ibuf, "%c%s", CONSOLE_MESSAGE, buf);

	return TRUE;
}

static bool kick_player(void)
{
	char name[80];

	/* Clear screen */
	Term_clear();

	/* Title */
	prt("Kick player", 1, 1);

	/* Prompt for name */
	prt("Kick who: ", 3, 1);

	/* Default */
	strcpy(name, "");

	/* Get name */
	askfor_aux(name, 80, 0);

	/* Never send null-length string */
	if (!strlen(name)) return FALSE;

	/* Send command */
	Packet_printf(&ibuf, "%c%s", CONSOLE_KICK_PLAYER, name);

	return TRUE;
}

static bool reload_server_preferences()
{
	Packet_printf(&ibuf, "%c", CONSOLE_RELOAD_SERVER_PREFERENCES);
	return TRUE;
}

static bool shutdown_server(void)
{
	char ch;

	/* Clear screen */
	Term_clear();

	/* Title */
	prt("Server shutdown", 1, 1);

	/* Prompt */
	prt("Really shut the server down [y/n]: ", 3, 1);

	/* Get key */
	ch = inkey();

	/* Check for anything except 'y' */
	if (ch != 'y' && ch != 'Y') return FALSE;

	/* Send the command */
	Packet_printf(&ibuf, "%c", CONSOLE_SHUTDOWN);

	return TRUE;
}

static bool quit_console(void)
{
	quit(NULL);

	return TRUE;
}

bool process_command(void)
{
	/* Convert to lowercase if necessary */
	if (command_cmd >= 'A' && command_cmd <= 'Z')
	{
		command_cmd += 'a' - 'A';
	}

	/* Check the command */
	switch(command_cmd)
	{
		case 'a': return get_player_list();
			  break;
		case 'b': return get_artifact_list();
			  break;
		case 'c': return get_unique_list();
			  break;
		case 'd': return modify_artifact();
			  break;
		case 'e': return modify_unique();
			  break;
		case 'f': return get_player_info();
			  break;
		case 'g': return send_message();
			  break;
		case 'h': return kick_player();
			  break;
		case 'i': return reload_server_preferences();
			  break;
		case 'j': return shutdown_server();
			  break;
		case 'k': return quit_console();
			  break;
		default: /* Bad command */
			  return FALSE;
	}

	/* Oops.... */
	return FALSE;
}

static void show_status(void)
{
	int i, j, n, lev;
	char buf[1024], name[1024], race[80], class[80];
	struct worldpos wpos;

	/* Clear screen */
	Term_clear();

	/* Extract number of players */
	Packet_scanf(&ibuf, "%d", &n);

	/* Show number */
	sprintf(buf, "%d player%s in the game.", n, ((n == 1) ? " is" : "s are"));

	/* Put on screen */
	prt(buf, 1, 1);

	/* Initialize line counter */
	j = 3;

	/* Print each player's info */
	for (i = 1; i <= n; i++)
	{
		/* Check for end of page */
		if (j == 21)
		{
			/* Prompt */
			prt("[Hit any key to continue]", 23, 1);

			/* Refresh */
			Term_fresh();

			/* Wait for key */
			inkey();

			/* Start at the top */
			j = 3;

			/* Clear screen */
			Term_clear();

			/* Show number */
			sprintf(buf, "%d player%s in the game.", n, ((n == 1) ? " is" : "s are"));

			/* Title */
			prt(buf, 1, 1);
		}

		/* Extract info */
		Packet_scanf(&ibuf, "%s%s%s%d%d%d%d", name, race, class, &lev, &wpos.wx, &wpos.wy, &wpos.wz);

		/* Format the string */
		sprintf(buf, "%s the %s %s (Level %d) at %d feet of (%d,%d)", name, race, class, lev, wpos.wz*50, wpos.wx, wpos.wy);

		/* Print it */
		prt(buf, j, 1);

		/* Next line */
		j++;
	}

	/* Prompt */
	prt("[Hit any key to exit]", 23, 1);

	/* Wait */
	inkey();
}


static void show_artifact_list(void)
{
	int i, j, n, number;
	char name[80], buf[1024];

	/* Clear screen */
	Term_clear();

	/* Extract number of artifacts */
	Packet_scanf(&ibuf, "%d", &n);

	/* Title */
	prt("Artifact list", 1, 1);

	/* Initialize line counter */
	j = 3;

	/* Print each artifact info */
	for (i = 1; i <= n; i++)
	{
		/* Check for end of page */
		if (j == 21)
		{
			/* Prompt */
			prt("[Hit any key to continue]", 23, 1);

			/* Refresh */
			Term_fresh();

			/* Wait for key */
			inkey();

			/* Start at the top */
			j = 3;

			/* Clear screen */
			Term_clear();

			/* Title */
			prt("Artifact list", 1, 1);
		}

		/* Extract info */
		Packet_scanf(&ibuf, "%d%s", &number, name);

		/* Format the string */
		sprintf(buf, "%3d) %s", number, name);

		/* Print it */
		prt(buf, j, 1);

		/* Next line */
		j++;
	}

	/* Prompt */
	prt("[Hit any key to exit]", 23, 1);

	/* Wait */
	inkey();
}

static void show_unique_list(void)
{
	int i, j, n, number;
	char name[80], buf[1024];

	/* Clear screen */
	Term_clear();

	/* Extract number of artifacts */
	Packet_scanf(&ibuf, "%d", &n);

	/* Title */
	prt("Unique list", 1, 1);

	/* Initialize line counter */
	j = 3;

	/* Print each artifact info */
	for (i = 1; i <= n; i++)
	{
		/* Check for end of page */
		if (j == 21)
		{
			/* Prompt */
			prt("[Hit any key to continue]", 23, 1);

			/* Refresh */
			Term_fresh();

			/* Wait for key */
			inkey();

			/* Start at the top */
			j = 3;

			/* Clear */
			Term_clear();

			/* Title */
			prt("Unique list", 1, 1);
		}

		/* Extract info */
		Packet_scanf(&ibuf, "%d%s", &number, name);

		/* Format the string */
		sprintf(buf, "%3d) %s", number, name);

		/* Print it */
		prt(buf, j, 1);

		/* Next line */
		j++;
	}

	/* Prompt */
	prt("[Hit any key to exit]", 23, 1);

	/* Wait */
	inkey();
}

static void show_generic_reply(void)
{
	char status;

	/* Extract reply */
	Packet_scanf(&ibuf, "%c", &status);

	/* Check for success */
	if (status)
	{
		/* Message */
		prt("Command succeeded!", 15, 1);
	}
	else
	{
		/* Message */
		prt("Command failed!", 15, 1);
	}

	/* Prompt */
	prt("[Hit any key to exit]", 23, 1);

	/* Wait */
	inkey();
}

void process_reply(void)
{
	char ch;

	/* Extract reply type */
	Packet_scanf(&ibuf, "%c", &ch);

	/* Determine what to do */
	switch (ch)
	{
		case CONSOLE_STATUS:
			show_status();
			break;
		case CONSOLE_ARTIFACT_LIST:
			show_artifact_list();
			break;
		case CONSOLE_UNIQUE_LIST:
			show_unique_list();
			break;
		case CONSOLE_CHANGE_ARTIFACT:
			show_generic_reply();
			break;
		case CONSOLE_CHANGE_UNIQUE:
			show_generic_reply();
			break;
		case CONSOLE_KICK_PLAYER:
			show_generic_reply();
			break;
		case CONSOLE_MESSAGE:
			break;
		case CONSOLE_SHUTDOWN:
			quit("Server shutdown");
			break;
		case CONSOLE_DENIED:
			quit("Denied access");
			break;
	}
}
