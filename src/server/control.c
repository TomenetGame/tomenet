/*
 * Support for the "remote console".  It lets the server admin
 * perform server maintenance without requiring that the server
 * have a devoted tty.
 */

/* added this for consistency in some (unrelated) header-inclusion,
   it IS a server file, isn't it? */
#define SERVER

#include <stdarg.h>
#include "angband.h"
#include "control.h"
#include <sys/time.h>

#ifdef MINGW
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif

#ifdef SERVER_GWPORT
#if 0
/* Server gateway stuff */
void SGWHit(int read_fd, int arg){
	int newsock = 0;
	char *sdb;
	int size = 0;
	time_t now;

	/* read_fd always the main socket, because we
	   never install_input on any others. */
	/* Order: connection, s->send, s->shutdown socket */

	if ((newsock = SocketAccept(read_fd)) != -1) {
		/* Send some crap here */
		sdb = C_NEW(4096, char);
		if (sdb != (char*)NULL) {
			int i;
			time(&now);
			size += sprintf(sdb, "runtime=%d\n", (int)(now - cfg.runtime));
			size += sprintf(&sdb[size], "turn=%d\n", turn);
			size += sprintf(&sdb[size], "day=%d\n", DAY); /* day const */
			size += sprintf(&sdb[size], "year=%d\n", bst(YEAR, turn)); /* starting year const */
			/* let the script count or we'll give away
			   the dungeon masters. */
			/* size+=sprintf(&sdb[size],"num=%d\n", NumPlayers);*/
			for (i = 1; i <= NumPlayers; i++) {
				if (Players[i]->admin_dm && cfg.secret_dungeon_master) continue;
				size += sprintf(&sdb[size], "player=%s\n", Players[i]->name);
				size += sprintf(&sdb[size], "level=%d\n", Players[i]->lev);
				size += sprintf(&sdb[size], "race=%s\n", race_info[Players[i]->prace].title);
				size += sprintf(&sdb[size], "born=%d\n", Players[i]->turn);
			}
			size += highscore_send(&sdb[size], 4096 - size);
			DgramWrite(newsock, sdb, size);
			C_FREE(sdb, 4096, char);
		}
		close(newsock);	/* oops */
	}
	else
		s_printf("Web server request failed: %d\n", errno);
}
#else
/* Server gateway stuff */

gw_connection_t gw_conns[MAX_GW_CONNS];
int gw_conns_num;

/* Copy the first line from the socket buffer into buf */
static int Packet_getln(sockbuf_t *s, char *buf) {
	char *scan;
	int len;
	
	for (scan = s->ptr; scan < s->buf + s->len; scan++) {
		if (*scan == '\n') {
			/* Calculate the length and limit to MSG_LEN - 1 */
			len = MIN(scan - s->ptr, MSG_LEN - 1);
			
			/* Copy the line */
			memcpy(buf, s->ptr, len);
			
			/* Terminate */
			buf[len] = '\0';
			
			/* Move the socket buffer pointer forward */
			s->ptr = scan + 1;
			
			return len;
		}
	}
	
	return 0;
}

/* Write formatted ouput terminated with a newline into a socket buffer */
static int Packet_println(sockbuf_t *s, cptr fmt, ...) {
	va_list ap;
	char buf[4096];
	int n;
	int rval;
	
	va_start(ap, fmt);
	
	/* Use vsnprintf to do the formatting */
	n = vsnprintf(buf, 4095, fmt, ap);
	
	if (n >= 0) {
		/* Write the output to the socket buffer */
		buf[n++] = '\n';
		rval = Sockbuf_write(s, buf, n);
	} else {
		/* Some error */
		rval = 0;
	}
	
	va_end(ap);
	
	return rval;
}

/* GW connection handler */
void SGWHit(int read_fd, int arg) {
	int i, sock, bytes;
	gw_connection_t *gw_conn = NULL;
	char buf[MSG_LEN];
	struct timeval time_begin, time_end, time_delta;

	/* For measuring performance */
	gettimeofday(&time_begin, NULL);

	/* Check if it's a new client */
	if (read_fd == SGWSocket) {
		/* Accept it */
		sock = SocketAccept(read_fd);

		if (sock == -1) {
			s_printf("Failed to accept GW port connection (errno=%d)\n", errno);
			return;
		}

		/* Check if we have room for it */
		if (gw_conns_num < MAX_GW_CONNS) {

			/* Find a free connection */
			for (i = 0; i < MAX_GW_CONNS; i++) {
				gw_conn = &gw_conns[i];

				if (gw_conn->state == CONN_FREE)
					break;
			}

			gw_conn->sock = sock;
			gw_conn->state = CONN_CONNECTED;
			gw_conn->last_activity = time(NULL);

#if 0 /* prevents sending lots of data */
			if (SetSocketNonBlocking(sock, 1) == -1) {
				plog("Cannot make GW client socket non-blocking");
			}
#endif

			/* Initialize socket buffers */
			Sockbuf_init(&gw_conn->r, sock, 8192, SOCKBUF_READ);
			Sockbuf_init(&gw_conn->w, sock, 8192, SOCKBUF_WRITE);

			install_input(SGWHit, sock, 2);

			gw_conns_num++;
		} else {
			/* No room, close the connection */
			DgramClose(sock);
		}

		/* The connection has either been accepted or not, we're done */
		return;
	}

	/* Find the matching connection */
	for (i = 0; i < MAX_GW_CONNS; i++) {
		gw_connection_t *gw_conn2 = &gw_conns[i];
			
		if (gw_conn2->state == CONN_CONNECTED && gw_conn2->sock == read_fd) {
			gw_conn = gw_conn2;
			break;
		}
	}

	if (!gw_conn) {
		s_printf("No GW connection found for socket number %d\n", read_fd);
		DgramClose(read_fd);
		return;
	}

	/* Read the message */
	bytes = DgramReceiveAny(read_fd, gw_conn->r.buf + gw_conn->r.len, gw_conn->r.size - gw_conn->r.len);

	/* Check for errors or our TCP connection closing */
	if (bytes <= 0) {
		/* If 0 bytes have been sent than the client has probably closed
		 * the connection
		 */
		if (bytes == 0) {
			gw_conn->state = CONN_FREE;
			Sockbuf_cleanup(&gw_conn->r);
			Sockbuf_cleanup(&gw_conn->w);
			DgramClose(read_fd);
			remove_input(read_fd);
			gw_conns_num--;
		}
		else if (bytes < 0 && errno != EWOULDBLOCK && errno != EAGAIN &&
			errno != EINTR)
		{
			/* Clear the error condition for the contact socket */
			GetSocketError(read_fd);

			s_printf("Failed to read from GW client!\n");
		}
		return;
	}

	/* Set length */
	gw_conn->r.len = bytes;

	gw_conn->last_activity = time(NULL);

	/* Prepare to send a new reply */
	Sockbuf_clear(&gw_conn->w);

	/* Try to read the request */
	while (Packet_getln(&gw_conn->r, buf) > 0) {
		/* Basic server info */
		if (!strcmp(buf, "INFO")) {
			time_t now;
			player_type *p_ptr;
			int players = 0;
			int day = bst(DAY, turn);
			now = time(NULL);

			/* Count players */
			for (i = 1; i <= NumPlayers; i++) {
				p_ptr = Players[i];
				if (p_ptr->admin_dm && cfg.secret_dungeon_master) continue;
				players++;
			}

			Packet_println(&gw_conn->w, "INFO REPLY");
			Packet_println(&gw_conn->w, "uptime=%d", now - cfg.runtime);
			Packet_println(&gw_conn->w, "turn=%d", turn);
			Packet_println(&gw_conn->w, "ingameday=%s of the %s year", get_month_name(day, FALSE, FALSE), get_day(bst(YEAR, turn)));
			Packet_println(&gw_conn->w, "fps=%d", cfg.fps);
			Packet_println(&gw_conn->w, "players=%d", players);
			Packet_println(&gw_conn->w, "INFO REPLY END");
		}

		/* List of players */
		else if (!strcmp(buf, "PLAYERS")) {
			player_type *p_ptr;
			Packet_println(&gw_conn->w, "PLAYERS REPLY");
			for (i = 1; i <= NumPlayers; i++) {
				p_ptr = Players[i];
				if (p_ptr->admin_dm && cfg.secret_dungeon_master) continue;
				Packet_println(&gw_conn->w, "name=%s", p_ptr->name);
				Packet_println(&gw_conn->w, "account=%s", p_ptr->accountname);
				Packet_println(&gw_conn->w, "host=%s", p_ptr->hostname);
				Packet_println(&gw_conn->w, "level=%d", p_ptr->lev);
				Packet_println(&gw_conn->w, "race=%s", race_info[p_ptr->prace].title);
				Packet_println(&gw_conn->w, "class=%s", class_info[p_ptr->pclass].title);
				Packet_println(&gw_conn->w, "gender=%s", (p_ptr->male) ? "male" : "female");
			}
			Packet_println(&gw_conn->w, "PLAYERS REPLY END");
		}

		/* Highscore list */
		else if (!strcmp(buf, "SCORES")) {
			char *buf2;
			int len;
			int amount, sent = 0, rval, fail = 0;

			Packet_println(&gw_conn->w, "SCORES REPLY");

			/* Send the contents of the socket buffer before sending buf2 */
			Sockbuf_flush(&gw_conn->w);
			Sockbuf_clear(&gw_conn->w);

			C_MAKE(buf2, 81920, char);
			len = highscore_send(buf2, 81920);

			/* Send buf2 in pieces */
			/* XXX - Blocking send loop */
			while (sent < len) {
				/* Try to send a full socket buffer */
				amount = MIN(8192, len - sent);
				Sockbuf_write(&gw_conn->w, &buf2[sent], amount);
				rval = Sockbuf_flush(&gw_conn->w);

				if (rval <= 0) {
					/* Failure */
					if (++fail >= 3) {
						/* Give up */
						break;
					}
				}
				else {
					sent += rval;
				}

				Sockbuf_clear(&gw_conn->w);
			}

			C_KILL(buf2, 81920, char);
			Packet_println(&gw_conn->w, "SCORES REPLY END");
		}

		/* House list */
		else if (!strcmp(buf, "HOUSES")) {
			char *buf2;
			int len, alloc = 128 * num_houses;
			int amount, sent = 0, rval, fail = 0;

			C_MAKE(buf2, alloc, char);
			Packet_println(&gw_conn->w, "HOUSES REPLY");

			len = houses_send(buf2, alloc);

			/* Send the contents of the socket buffer before sending buf2 */
			Sockbuf_flush(&gw_conn->w);
			Sockbuf_clear(&gw_conn->w);

			/* Send buf2 in pieces */
			/* XXX - Blocking send loop */
			while (sent < len) {
				/* Try to send a full socket buffer */
				amount = MIN(8192, len - sent);
				Sockbuf_write(&gw_conn->w, &buf2[sent], amount);
				rval = Sockbuf_flush(&gw_conn->w);

				if (rval <= 0) {
					/* Failure */
					if (++fail >= 3) {
						/* Give up */
						break;
					}
				}
				else {
					sent += rval;
				}

				Sockbuf_clear(&gw_conn->w);
			}

			Packet_println(&gw_conn->w, "HOUSES REPLY END");
			C_KILL(buf2, alloc, char);
		}

		/* Player stores list */
		else if (!strcmp(buf, "PSTORES")) {
			//MAX_HOUSES (65536), STORE_INVEN_MAX (120)
#if 0 /* todo: implement (this is a copy/paste of above 'HOUSES') */
			char *buf2;
			int len, alloc = 128 * num_houses;
			int amount, sent = 0, rval, fail = 0;

			C_MAKE(buf2, alloc, char);
			Packet_println(&gw_conn->w, "PSTORES REPLY");

			len = pstores_send(buf2, alloc);

			/* Send the contents of the socket buffer before sending buf2 */
			Sockbuf_flush(&gw_conn->w);
			Sockbuf_clear(&gw_conn->w);

			/* Send buf2 in pieces */
			/* XXX - Blocking send loop */
			while (sent < len) {
				/* Try to send a full socket buffer */
				amount = MIN(8192, len - sent);
				Sockbuf_write(&gw_conn->w, &buf2[sent], amount);
				rval = Sockbuf_flush(&gw_conn->w);

				if (rval <= 0) {
					/* Failure */
					if (++fail >= 3) {
						/* Give up */
						break;
					}
				}
				else {
					sent += rval;
				}

				Sockbuf_clear(&gw_conn->w);
			}

			Packet_println(&gw_conn->w, "PSTORES REPLY END");
			C_KILL(buf2, alloc, char);
#endif
		}

		/* Default reply */
		else {
			Packet_println(&gw_conn->w, "ERROR UNKNOWN REQUEST");
		}

		if (gw_conn->w.len) {
			/* Send our reply */
			Sockbuf_flush(&gw_conn->w);

			/* Make room for more */
			Sockbuf_clear(&gw_conn->w);
		}
	}

	/* Advance the read buffer */
	Sockbuf_advance(&gw_conn->r, gw_conn->r.ptr - gw_conn->r.buf);

	/* Calculate the amount of time spent */
	gettimeofday(&time_end, NULL);
	time_delta.tv_sec = time_end.tv_sec - time_begin.tv_sec;
	time_delta.tv_usec = time_end.tv_usec - time_begin.tv_usec;
	if (time_delta.tv_usec < 0) {
		time_delta.tv_sec--;
		time_delta.tv_usec += 1000000;
	}
	// Disabled to reduce log file spam
	//s_printf("%s: Gateway request took %d.%06d seconds.\n", __func__, (int)time_delta.tv_sec, (int)time_delta.tv_usec);
}

/* Checks whether GW connections should timeout */
void SGWTimeout() {
	int i;
	gw_connection_t *gw_conn;
	time_t now;
	
	now = time(NULL);
	
	/* Go through all gateway connections */
	for (i = 0; i < MAX_GW_CONNS; i++) {
		gw_conn = &gw_conns[i];
		if (gw_conn->state == CONN_CONNECTED) {
			/* Check for timeout */
			if (gw_conn->last_activity + GW_CONN_TIMEOUT < now) {
				s_printf("GW client timeout\n");

				/* Close the connection */
				gw_conn->state = CONN_FREE;
				Sockbuf_cleanup(&gw_conn->r);
				Sockbuf_cleanup(&gw_conn->w);
				DgramClose(gw_conn->sock);
				remove_input(gw_conn->sock);
				gw_conns_num--;
			}
		}
	}
}
#endif
#endif

#ifdef NEW_SERVER_CONSOLE

static sockbuf_t console_buf;

/*
 * Return the list of players
 */
static void console_status()
{
	int k;

	/* Packet header */
	Packet_printf(&console_buf, "%c%d", CONSOLE_STATUS, NumPlayers);

	/* Scan the player list */
	for (k = 1; k <= NumPlayers; k++)
	{
		player_type *p_ptr = Players[k];

		/* Skip disconnected players */
		if (p_ptr->conn == NOT_CONNECTED) continue;

		/* Add an entry */
		Packet_printf(&console_buf, "%s%s%s%d%d%d%d",
			p_ptr->name, race_info[p_ptr->prace].title,
			class_info[p_ptr->pclass].title, p_ptr->lev,
			p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
	}
}

static void console_player_info(int player)
{
}

static void console_artifacts()
{
	int i, count = 0, z;
	bool okay[MAX_A_IDX];
	char base_name[ONAME_LEN];

	/* Packet header */
	Packet_printf(&console_buf, "%c", CONSOLE_ARTIFACT_LIST);

	/* Scan the artifacts */
	for (i = 0; i < MAX_A_IDX; i++) {
		artifact_type *a_ptr = &a_info[i];

		/* Default */
		okay[i] = FALSE;

		/* Skip "empty" artifacts */
		if (!a_ptr->name) continue;

		/* Skip "uncreated" artifacts */
		if (!a_ptr->cur_num) continue;

		/* Assume OK */
		okay[i] = TRUE;

		/* One more artifact */
		count++;
	}

	/* Write the number */
	Packet_printf(&console_buf, "%d", count);

	/* Write each artifact info */
	for (i = 0; i < MAX_A_IDX; i++) {
		artifact_type *a_ptr = &a_info[i];

		/* List "dead" ones */
		if (!okay[i]) continue;

		/* Paranoia */
		strcpy(base_name, "Unknown Artifact");

		/* Obtain the base object type */
		z = lookup_kind(a_ptr->tval, a_ptr->sval);

		/* Real object */
		if (z) {
			object_type forge;

			/* Create the object */
			invcopy(&forge, z);

			/* Create the artifact */
			forge.name1 = i;

			/* Describe the artifact */
			object_desc_store(0, base_name, &forge, FALSE, 0);
		}

		/* Output this artifact's number and name */
		Packet_printf(&console_buf, "%d%s", i, base_name);
	}
}

static void console_uniques()
{
	int k, count = 0;
	char buf[1024];

	/* Packet header */
	Packet_printf(&console_buf, "%c", CONSOLE_UNIQUE_LIST);

	/* Scan the monster races */
	for (k = 1; k < MAX_R_IDX - 1; k++) {
		monster_race *r_ptr = &r_info[k];

		/* Only process uniques */
		if (r_ptr->flags1 & RF1_UNIQUE) {
			bool dead = (r_ptr->max_num == 0);

			/* Only count known uniques */
			if (dead || r_ptr->r_sights) {
				/* One more */
				count++;
			}
		}
	}
	
	/* Write the number of uniques */
	Packet_printf(&console_buf, "%d", count);

	/* Scan the monster races */
	for (k = 1; k < MAX_R_IDX - 1; k++) {
		monster_race *r_ptr = &r_info[k];

		/* Only process uniques */
		if (r_ptr->flags1 & RF1_UNIQUE) {
			/* Only display known uniques */
			if (r_ptr->r_sights) {
				int i;
				byte ok = FALSE;
			
				/* Format message */
				sprintf(buf, "%s has been killed by:\n", r_name + r_ptr->name);
				
				for (i = 1; i <= NumPlayers; i++) {
					player_type *q_ptr = Players[i];

					if (q_ptr->r_killed[k] == 1) {
						sprintf(buf, "        %s\n", q_ptr->name);
						ok = TRUE;
					}
				}
				if (!ok) sprintf(buf, "       Nobody\n");


				/* Add info */
				Packet_printf(&console_buf, "%d%s", k, buf);
			}
		}
	}
}

static void console_change_artifact(int artifact, int status) {
	/* Check bounds */
	if (artifact <= 0 || artifact > MAX_A_IDX) {
		/* Failed */
		Packet_printf(&console_buf, "%c%c", CONSOLE_CHANGE_ARTIFACT, 0);
		return;
	}

	/* Set the artifact's status */
	if (status) {
		/* Make found */
		handle_art_inum(artifact);
	} else {
		/* Make unfound */
		handle_art_dnum(artifact);
	}

	/* Succeeded */
	Packet_printf(&console_buf, "%c%c", CONSOLE_CHANGE_ARTIFACT, 1);
}

static void console_change_unique(int unique, cptr killer)
{
	monster_race *r_ptr;
	int kill_idx;
	char buf[80];

	/* Check bounds */
	if (unique <= 0 || unique > MAX_R_IDX - 1) {
		/* Failed */
		Packet_printf(&console_buf, "%c%c", CONSOLE_CHANGE_UNIQUE, 0);

		return;
	}

	/* Get killer index */
	if (killer) {
		/* Lookup player by name */
		kill_idx = lookup_player_id(killer);
	} else {
		/* No killer */
		kill_idx = 0;
	}

	/* Set pointer */
	r_ptr = &r_info[unique];

	/* Check for uniqueness */
	if (!r_ptr->flags1 & RF1_UNIQUE) {
		/* Failed */
		Packet_printf(&console_buf, "%c%c", CONSOLE_CHANGE_UNIQUE, 0);

		return;
	}

	/* Set death flag */
	if (kill_idx) {
		/* Dead */
		r_ptr->max_num = 0;
	} else {
		/* Alive */

		/* Tell people if the monster is respawning */
		if (!r_ptr->max_num)
		{	/* the_sandman: added colour */
			snprintf(buf, 80, "\374\377v%s rises from the dead!",(r_name + r_ptr->name));
    			
			/* Tell every player */
			msg_broadcast(0,buf);
		}
		r_ptr->max_num = 1;
	}

	/* Succeeded */
	Packet_printf(&console_buf, "%c%c", CONSOLE_CHANGE_UNIQUE, 1);
}

static void console_message(char *buf)
{
	/* Send the message */
	player_talk(0, buf);

	/* Reply */
	Packet_printf(&console_buf, "%c", CONSOLE_MESSAGE);
}

static void console_kick_player(char *name)
{
	int i;

	/* Check the players in the game */
	for (i = 1; i <= NumPlayers; i++) {
		/* Check name */
		if (!strcmp(name, Players[i]->name)) {
			/* Kick him */
			Destroy_connection(Players[i]->conn, "Kicked out");

			/* Success */
			Packet_printf(&console_buf, "%c%c", CONSOLE_KICK_PLAYER, 1);

			return;
		}
	}

	/* Failure */
	Packet_printf(&console_buf, "%c%c", CONSOLE_KICK_PLAYER, 0);
}

static void console_reload_server_preferences(void)
{
	/* Reload the server preferences */
	load_server_cfg();

	/* Let mangconsole know that the command was a success */
	/* Packet header */
	Packet_printf(&console_buf, "%c", CONSOLE_RELOAD_SERVER_PREFERENCES);

	/* Write the output */
	DgramReply(console_buf.sock, console_buf.ptr, console_buf.len);
}
static void console_shutdown(void)
{
	/* Packet header */
	Packet_printf(&console_buf, "%c", CONSOLE_SHUTDOWN);

	/* Write the output */
	DgramReply(console_buf.sock, console_buf.ptr, console_buf.len);

	/* Shutdown */
	shutdown_server();
}

/*
 * This is the response function when incoming data is received on the
 * control pipe.
 */
void NewConsole(int read_fd, int arg)
{
	char ch = 0, passwd[MAX_CHARS] = {'\0'}, buf[MAX_CHARS] = {'\0'};
	int i = 0, j = 0, bytes = 0;
	static int newsock = 0;

	/* Make a TCP connection */
	/* Hack -- check if this data has arrived on the contact socket or not.
	 * If it has, then we have not created a connection with the client yet, 
	 * and so we must do so.
	 */
	if (read_fd == ConsoleSocket)
	{
		/* Hack -- make sure that two people haven't tried to use mangconsole
		 * at the same time.  Since I am currently too lazy to support this,
		 * we will remove the input of the first person when the second person
		 * connects.
		 */
		if (newsock) remove_input(newsock);
		if ((newsock = SocketAccept(read_fd)) == -1)
		{
			/* Clear socket if it failed */
			newsock = 0;
			quit("Couldn't accept console TCP connection.\n");
		}
		console_buf.sock = newsock;
		install_input(NewConsole, newsock, 2);
		return;
	}


	/* Clear the buffer */
	Sockbuf_clear(&console_buf);
	/* Read the message */
	bytes = DgramReceiveAny(read_fd, console_buf.buf, console_buf.size);

	/* Check for errors or our TCP connection closing */
	if (bytes <= 0)
	{
		/* If this happens our TCP connection has probably been severed.
		 * Remove the input.
		 */
		close(newsock);
		remove_input(newsock);
		newsock = 0;

		return;
	}

	/* Set length */
	console_buf.len = bytes;

	/* Get the password */
	if (Packet_scanf(&console_buf, "%s", passwd) <= 0) {
		goto input_error;
	}

	if (strcmp(passwd, cfg.console_password) != 0) {
		goto input_error;
	}
	else s_printf("Valid console command from %s.\n", DgramLastname(read_fd));

	/* Acquire command */
	if (Packet_scanf(&console_buf, "%c", &ch) <= 0) {
		goto input_error;
	}

	/* Determine what the command is */
	switch(ch)
	{
		/* Wants to see the player list */
		case CONSOLE_STATUS:
			console_status();
			break;

		/* Wants to see detailed player info */
		case CONSOLE_PLAYER_INFO:
			if (Packet_scanf(&console_buf, "%d", &i) <= 0) {
				goto input_error;
			}
			console_player_info(i);
			break;

		/* Wants to see the artifact list */
		case CONSOLE_ARTIFACT_LIST:
			console_artifacts();
			break;

		/* Wants to see the unique list */
		case CONSOLE_UNIQUE_LIST:
			console_uniques();
			break;

		/* Wants to change artifact status */
		case CONSOLE_CHANGE_ARTIFACT:
			if (Packet_scanf(&console_buf, "%d%d", &i, &j) <= 0) {
				goto input_error;
			}
			console_change_artifact(i, j);
			break;

		/* Wants to change unique status */
		case CONSOLE_CHANGE_UNIQUE:
			if (Packet_scanf(&console_buf, "%d%s", &i, buf) <= 0) {
				goto input_error;
			}
			console_change_unique(i, buf);
			break;

		/* Wants to send a message */
		case CONSOLE_MESSAGE:
			if (Packet_scanf(&console_buf, "%s", buf) <= 0) {
				goto input_error;
			}
			console_message(buf);
			break;

		/* Wants to kick a player */
		case CONSOLE_KICK_PLAYER:
			if (Packet_scanf(&console_buf, "%s", buf) <= 0) {
				goto input_error;
			}
			console_kick_player(buf);
			break;

		/* Wants to reload the server preferences */
		case CONSOLE_RELOAD_SERVER_PREFERENCES:
			console_reload_server_preferences();
			break;

		/* Wants to shut the server down */
		case CONSOLE_SHUTDOWN:
			console_shutdown();
			break;

		/* Strangeness */
		default:
			s_printf("Bizarre input on server console; ignoring.\n");
			return;
	}

	/* Write the response */
	DgramWrite(console_buf.sock, console_buf.ptr, console_buf.len);

	/* Success */
	return;

	input_error: /* Handle any errors */
	/* Clear buffer */
	Sockbuf_clear(&console_buf);

	/* Put an "illegal access" reply in the buffer */
	/* TODO: This needs more error codes... */
	Packet_printf(&console_buf, "%c", CONSOLE_DENIED);

	/* Send it */
	DgramWrite(read_fd, console_buf.buf, console_buf.len);

	/* Log this to the local console */
	s_printf("Illegal console command from %s.\n", DgramLastname(read_fd));

	/* Failure */
	return;
}

/*
 * Initialize the stuff for the new console
 */
bool InitNewConsole(int write_fd)
{
	/* Initialize buffer */
	if (Sockbuf_init(&console_buf, write_fd, 8192, SOCKBUF_READ | SOCKBUF_WRITE))
	{
		/* Failed */
		s_printf("No memory for console buffer.\n");

		return FALSE;
	}

	return TRUE;
}

#endif
