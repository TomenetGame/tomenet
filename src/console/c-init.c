/* $Id$ */
/* Console initialization module */

/*
 * This file should contain non-system-specific code.  If a 
 * specific system needs its own "main" function (such as
 * Windows), then it should be placed in the "main-???.c" file.
 */

#include "angband.h"

static int Socket;


static bool enter_server_name(void)
{
	/* Clear screen */
	Term_clear();
	Term_fresh();
	Term_flush();

	/* Message */
	prt("Enter the server name you want to connect to (ESCAPE to quit): ", 3, 1);

	/* Move cursor */
	move_cursor(5, 1);

	/* Default */
	strcpy(server_name, "localhost");


	/* Ask for server name */
	return askfor_aux(server_name, 80, 0);
}


/*
 * Choose the character's name
 */
static void enter_password(void)
{
	char tmp[40];

	/* Clear screen */
	Term_clear();
	Term_fresh();
	Term_flush();

	/* Prompt and ask */
	prt("Enter your password (or hit ESCAPE):", 3, 1);

	/* Default */
	strcpy(tmp, "default");

	/* Ask until happy */
	while (1)
	{
		/* Go to the "name" area */
		move_cursor(5, 1);

		/* Get an input, ignore "Escape" */
		if (askfor_aux(tmp, 40, 1)) strcpy(pass, tmp);

		/* All done */
		break;
	}

	/* Pad the name (to clear junk) 
	sprintf(tmp, "%-15.15s", pass);

	 Re-Draw the name (in light blue) 
	c_put_str(TERM_L_BLUE, tmp, 3, 15);
	*/

	/* Erase the prompt, etc */
	clear_from(20);
}


/*
 * Show a nice menu
 */
static void show_menu(void)
{
	/* Title */
	prt("TomeNET server console", 1, 1);

	/* Options */
	prt("(a) Get player list", 3, 5);
	prt("(b) Get artifact list", 4, 5);
	prt("(c) Get unique list", 5, 5);
	prt("(d) Modify artifact info", 6, 5);
	prt("(e) Modify unique info", 7, 5);
	prt("(f) Get detailed player info", 8, 5);
	prt("(g) Send a message", 9, 5);
	prt("(h) Kick a player", 10, 5);
	prt("(i) Reload the server preferences", 11, 5);
	prt("(j) Shutdown the server", 12, 5);
	prt("(k) Quit", 13, 5);

	/* Prompt */
	prt("Enter an action above: ", 14, 1);
}

/*
 * Cleanup after ourselves
 */
static void Net_cleanup(void)
{
	/* Destroy the buffer */
	Sockbuf_cleanup(&ibuf);

	/* Close the socket */
	DgramClose(Socket);
}

/*
 * Loop, looking for net input and responding to keypresses.
 */
static void Input_loop(void)
{
	int bytes;

	for (;;)
	{
		/* Hack -- shutdown at once */
		if (server_shutdown)
		{
			/* Clear the buffer */
			Sockbuf_clear(&ibuf);

			/* Write the password to the packet */
			Packet_printf(&ibuf, "%s", pass);

			/* Send the command */
			Packet_printf(&ibuf, "%c", CONSOLE_SHUTDOWN);
		}
		else
		{
			/* Clear screen */
			Term_clear();

			/* Show the menu */
			show_menu();

			/* Refresh the screen */
			Term_fresh();

			/* Clear the previous command */
			command_cmd = 0;

			/* See if we have a command waiting */
			request_command(FALSE);

			/* Clear the buffer */
			Sockbuf_clear(&ibuf);

			/* Write the password to the packet */
			Packet_printf(&ibuf, "%s", pass);

			/* Process any commands we got */
			if (command_cmd)
			{
				/* Process it */
				if (!process_command())
					continue;
			}
			else
			{
				/* No command */
				continue;
			}
		}

		/* Send the command */
		Sockbuf_flush(&ibuf);

		/* Wait for response */
		SetTimeout(10, 0);

		/* Check for reply */
		if (!SocketReadable(Socket))
		{
			/* Hmm, maybe the server died */
			quit("Server didn't reply");
		}

		/* Clear the buffer */
		Sockbuf_clear(&ibuf);

		/* Read reply */
		if ((bytes = DgramRead(Socket, ibuf.buf, ibuf.size)) <= 0)
		{
			/* Ack */
			quit("Couldn't read server reply");
		}

		/* Set buffer length */
		ibuf.len = bytes;

		/* Process/show the reply */
		process_reply();

		/* Flush input */
		flush();
	}
}	

/*
 * A hook for "quit()".
 *
 * Close down, then fall back into "quit()".
 */
static void quit_hook(cptr s)
{
	int j;

	Net_cleanup();

#ifdef UNIX_SOCKETS
	SocketCloseAll();
#endif

	/* Nuke each term */
	for (j = 8 - 1; j >= 0; j--)
	{
		/* Unused */
		if (!ang_term[j]) continue;

		/* Nuke it */
		term_nuke(ang_term[j]);
	}
}

/*
 * Initialize everything, contact the server, and start the loop.
 */
void console_init(void)
{
	bool done = FALSE;
	char host_name[1024];

#ifdef BIND_NAME
                strncpy( host_name, BIND_NAME, 1024);
#else
                GetLocalHostName(host_name, 1024);
#endif

	/* Attempt to initialize a visual module */
	if (!force_cui)
	{
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
	}

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

	/* No visual module worked */
	if (!done)
	{
		Net_cleanup();
		printf("Unable to initialize a display module!\n");
		exit(1);
	}

	/* Set the "quit hook" */
	quit_aux = quit_hook;

	/* If not already specified, get the server address */
	if (!strcmp(server_name, "undefined")) enter_server_name();

	/* Get the password */
	if (!strcmp(pass, "undefined")) enter_password();


	/* Create net socket */
	if ((Socket = CreateClientSocket(server_name, cfg_console_port)) == -1)
	{
		quit("Could not connect to console port\n");
	}

	/* Make it non-blocking */
	if (SetSocketNonBlocking(Socket, 1) == -1)
	{	
		quit("Can't make socket non-blocking\n");
	}

	/* Create a socket buffer */
	if (Sockbuf_init(&ibuf, Socket, CLIENT_SEND_SIZE,
		SOCKBUF_READ | SOCKBUF_WRITE) == -1)
	{
		quit("No memory for socket buffer\n");
	}

	/* Clear it */
	Sockbuf_clear(&ibuf);
	
	/* Get input, send commands, and read the response */
	Input_loop();

	/* Quit, closing term windows */
	quit(NULL);
}
