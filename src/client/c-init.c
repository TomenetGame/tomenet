/* $Id$ */
/* Client initialization module */

/*
 * This file should contain non-system-specific code.  If a 
 * specific system needs its own "main" function (such as
 * Windows), then it should be placed in the "main-???.c" file.
 */

#include "angband.h"

static int Socket;

static void init_arrays(void)
{
	/* Macro variables */
	C_MAKE(macro__pat, MACRO_MAX, cptr);
	C_MAKE(macro__act, MACRO_MAX, cptr);
	C_MAKE(macro__cmd, MACRO_MAX, bool);

	/* Macro action buffer */
	C_MAKE(macro__buf, 1024, char);

	/* Message variables */
	C_MAKE(message__ptr, MESSAGE_MAX, u16b);
	C_MAKE(message__buf, MESSAGE_BUF, char);

	/* Hack -- No messages yet */
	message__tail = MESSAGE_BUF;

	/* Initialize room for the store's stock */
	C_MAKE(store.stock, STORE_INVEN_MAX, object_type);
}

/*
 * Initialize and verify the file paths, and the score file.
 *
 * Use the ANGBAND_PATH environment var if possible, else use
 * DEFAULT_PATH, and in either case, branch off appropriately.
 *
 * First, we'll look for the ANGBAND_PATH environment variable,
 * and then look for the files in there.  If that doesn't work,
 * we'll try the DEFAULT_PATH constant.  So be sure that one of
 * these two things works...
 *
 * We must ensure that the path ends with "PATH_SEP" if needed,
 * since the "init_file_paths()" function will simply append the
 * relevant "sub-directory names" to the given path.
 *
 * Note that the "path" must be "Angband:" for the Amiga, and it
 * is ignored for "VM/ESA", so I just combined the two.
 */
static void init_stuff(void)
{
	//        char path[1024];

#if defined(AMIGA) || defined(VM)

	/* Hack -- prepare "path" */
	strcpy(path, "PMAngband:");

#else /* AMIGA / VM */

	if (!path)
	{
		cptr tail;

		/* Get the environment variable */
		tail = getenv("ANGBAND_PATH");

		/* Use the angband_path, or a default */
		strcpy(path, tail ? tail : DEFAULT_PATH);
	}

	/* Hack -- Add a path separator (only if needed) */
	if (!suffix(path, PATH_SEP)) strcat(path, PATH_SEP);

#endif /* AMIGA / VM */

	/* Initialize */
	init_file_paths(path);
}



/*
 * Open all relevant pref files.
 */
void initialize_all_pref_files(void)
{
        char buf[1024];


        /* Access the "basic" pref file */
        strcpy(buf, "pref.prf");

        /* Process that file */
        process_pref_file(buf);

        /* Access the "user" pref file */
        sprintf(buf, "user.prf");

        /* Process that file */
        process_pref_file(buf);



        /* Access the "basic" system pref file */
        sprintf(buf, "pref-%s.prf", ANGBAND_SYS);

        /* Process that file */
        process_pref_file(buf);

        /* Access the "visual" system pref file (if any) */
        sprintf(buf, "%s-%s.prf", (use_graphics ? "graf" : "font"), ANGBAND_SYS);
	printf("check for %s\n",buf);

        /* Process that file */
        process_pref_file(buf);

        /* Access the "user" system pref file */
        sprintf(buf, "user-%s.prf", ANGBAND_SYS);

        /* Process that file */
        process_pref_file(buf);


        /* Access the "race" pref file */
        sprintf(buf, "%s.prf", race_title[race]);
	buf[0] = tolower(buf[0]);

        /* Process that file */
        process_pref_file(buf);

        /* Access the "class" pref file */
        sprintf(buf, "%s.prf", class_title[class]);
	buf[0] = tolower(buf[0]);

        /* Process that file */
        process_pref_file(buf);

        /* Access the "character" pref file */
        sprintf(buf, "%s.prf", nick);
	buf[0] = tolower(buf[0]);

        /* Process that file */
        process_pref_file(buf);
}


/*
 * Loop, looking for net input and responding to keypresses.
 */
static void Input_loop(void)
{
	int	netfd, result;

	if (Net_flush() == -1)
		return;

	if ((netfd = Net_fd()) == -1)
	{
		plog("Bad socket filedescriptor");
		return;
	}

	for (;;)
	{
		// Send out a keepalive packet if need be
		do_keepalive();
		do_mail();

		if (Net_flush() == -1)
		{
			plog("Bad net flush");
			return;
		}

		/* Set the timeout on the network socket */
		/* This polling should probably be replaced with a select
		 * call that combines the net and keyboard input.
		 * This REALLY needs to be replaced since under my Linux
		 * system calling usleep seems to have a 10 ms overhead
		 * attached to it.
		 */
		//SetTimeout(0, 1000000 / Setup.frames_per_second);
		SetTimeout(0, 1000000 / 1000);

		/* Only take input if we got some */
		if (SocketReadable(netfd))
		{
			if ((result = Net_input()) == -1)
			{
				/*plog("Bad net input");*/
				return;
			}
		}

		/* See if we have a command waiting */
		request_command(FALSE);

		/* Process any commands we got */
		while (command_cmd)
		{
			/* Process it */
			process_command();

			/* XXX Unused */
			//last_sent = time(NULL);

			/* Clear previous command */
			command_cmd = 0;

			/* Ask for another command */
			request_command(FALSE);
		}

		// Update our internal timer, which is used to figure out when
		// to send keepalive packets.
		update_ticks();

		/* Hack -- don't redraw the screen until we have all of it */
		if (last_line_info < 22) continue;

		/* Flush input (now!) */
		flush_now();

		/* Update the screen */
		Term_fresh();

		/* Redraw windows if necessary */
		if (p_ptr->window)
		{
			window_stuff();
		}

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
void client_init(char *argv1, bool skip)
{
	sockbuf_t ibuf;
	unsigned magic = 12345;
	unsigned char reply_to, status;
	int login_port, i;
	int bytes, retries;
	char host_name[80];
	u16b version = MY_VERSION;
        s32b temp;
        char max_class;

	/* Setup the file paths */
	init_stuff();

	/* Initialize various arrays */
	init_arrays();

	GetLocalHostName(host_name, 80);

	/* Set the "quit hook" */
	quit_aux = quit_hook;

#ifndef UNIX_SOCKETS
	/* Check whether we should query the metaserver */
	if (argv1 == NULL)
	{
		/* Query metaserver */
		if (!get_server_name())
			quit("No server specified.");
	}
	else
	{
		/* Set the server's name */
		strcpy(server_name, argv1);
	}

	/* Fix "localhost" */
	if (!strcmp(server_name, "localhost"))
#endif
		strcpy(server_name, host_name);


	/* Get character name and pass */
	if (!skip) get_char_name();

	/* Capitalize the name */
	nick[0] = toupper(nick[0]);

	// Create the net socket and make the TCP connection
	if ((Socket = CreateClientSocket(server_name, cfg_game_port)) == -1)
	{
		quit("That server either isn't up, or you mistyped the hostname.\n");
	}

	/* Create a socket buffer */
	if (Sockbuf_init(&ibuf, Socket, CLIENT_SEND_SIZE,
		SOCKBUF_READ | SOCKBUF_WRITE) == -1)
	{
		quit("No memory for socket buffer\n");
	}

	/* Make it non-blocking */
	//if (SetSocketNonBlocking(Socket, 1) == -1)
//	{	
//		quit("Can't make socket non-blocking\n");
//	}

	/* Clear it */
	Sockbuf_clear(&ibuf);
	
	if (!strcmp(nick, "Ciang"))
	{
		strcpy(real_name, "pricilla");
		strcpy(host_name, "pricilla.nazgul");
	}

	/* Put the contact info in it */
	Packet_printf(&ibuf, "%u", magic);
	Packet_printf(&ibuf, "%s%hu%c", real_name, GetPortNum(ibuf.sock), 0xFF);
	Packet_printf(&ibuf, "%s%s%hu", nick, host_name, version);


	/* Connect to server */
#ifdef UNIX_SOCKETS
	if ((DgramConnect(Socket, server_name, cfg_game_port)) == -1)
#else
	// UDP stuffif ((DgramConnect(Socket, server_name, cfg_game_port)) == -1)
#endif
	//{
	//	quit("That server either isn't up, or you mistyped the host name.\n");
	//}
	
	/* Send the info */
	if ((bytes = DgramWrite(Socket, ibuf.buf, ibuf.len) == -1))
	{
		quit("Couldn't send contact information\n");
	}

	/* Listen for reply */
	for (retries = 0; retries < 10; retries++)
        {
                char buf[200];

		/* Set timeout */
		SetTimeout(1, 0);

		/* Wait for info */
		if (!SocketReadable(Socket)) continue;

		/* Read reply */
		if(DgramRead(Socket, ibuf.buf, ibuf.size) <= 0)
		{
			/*printf("DgramReceiveAny failed (errno = %d)\n", errno);*/
			continue;
		}

		/* Extra info from packet */
		Packet_scanf(&ibuf, "%c%c%d%c", &reply_to, &status, &temp, &max_class);

                /* Hack -- set the login port correctly */
		login_port = (int) temp;

		break;
	}

#if 0 // -- DGDGDGDG it would be NEAT to have classes sent to the cleint at conenciton, sadly Im too clumpsy at network code ..
        i = 0;
        while (i < max_class)
        {
                int ii;

		/* Set timeout */
		SetTimeout(1, 0);

		/* Wait for info */
		if (!SocketReadable(Socket)) continue;

		/* Read reply */
		if(DgramRead(Socket, ibuf.buf, ibuf.size) <= 0)
		{
			/*printf("DgramReceiveAny failed (errno = %d)\n", errno);*/
			continue;
		}

                Packet_scanf(&ibuf, "%c%s", &ii, buf);
                printf("c %d: %s\n", ii, buf);

                i++;
        }
#endif

	/* Check for failure */
	if (retries >= 10)
	{
		Net_cleanup();
		quit("Server didn't respond!\n");
	}

	/* Server returned error code */
	if (status == E_NEED_INFO)
	{
		/* Hack -- display the nick */
		if (skip) prt(format("Name        : %s", nick), 2, 1);

		/* Get sex/race/class */
		get_char_info();
	}
	else if (status)
	{
		/* The server didn't like us.... */
		switch (status)
		{
			case E_VERSION:
				quit("This version of the client will not work with that server.  http://T-o-M-E.net/pernmangband/ to get the new client.");
			case E_GAME_FULL:
				quit("Sorry, the game is full.  Try again later.");
			case E_IN_USE:
				quit("That nickname is already in use.  If it is your nickname, wait 30 seconds and try again.");
			case E_INVAL:
				quit("The server didn't like your nickname, realname, or hostname.");
			case E_TWO_PLAYERS:
				quit("There is already another character from this user/machine on the server.");
			case E_INVITE:
				quit("Sorry, the server is for members only.  Please retry with name 'guest'.");
			case E_BANNED:
				quit("You are temporally banned from connecting to this server!");
			default:
				quit(format("Connection failed with status %d.", status));
		}
	}

/*	printf("Server sent login port %d\n", login_port);
	printf("Server sent status %u\n", status);  */

	/* Close our current connection */
	// Dont close the TCP connection DgramClose(Socket);

	/* Connect to the server on the port it sent */
	//if (Net_init(server_name, login_port) == -1)
	if (Net_init(server_name, Socket) == -1)
	{
		quit("Network initialization failed!\n");
	}

	/* Initialize the pref files */
	initialize_all_pref_files();

	/* Verify that we are on the correct port */
	if (Net_verify(real_name, nick, pass, sex, race, class) == -1)
	{
		Net_cleanup();
		quit("Network verify failed!\n");
	}

	/* Receive stuff like the MOTD */
	if (Net_setup() == -1)
	{
		Net_cleanup();
		quit("Network setup failed!\n");
	}

	/* Setup the key mappings */
	keymap_init();

	/* Show the MOTD */
	show_motd();

	/* Start the game */
	if (Net_start() == -1)
	{
		Net_cleanup();
		quit("Network start failed!\n");
	}

	/* Main loop */
	Input_loop();

	/* Cleanup network stuff */
	Net_cleanup();

	/* Quit, closing term windows */
	quit(NULL);
}
