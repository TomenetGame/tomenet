/* $Id$ */
/* The client side of the networking stuff */

/* The file is very messy, and probably has a lot of stuff
 * that isn't even used.  Some massive work needs to be done
 * on it, but it's a low priority for me.
 */

/* I've made this file even worse as I have converted things to TCP.
 * rbuf, cbuf, wbuf, and qbuf are used very inconsistently.
 * rbuf is basically temporary storage that we read data into, and also
 * where we put the data before we call the command processing functions.
 * cbuf is the "command buffer".  qbuf is the "old command queue". wbuf
 * isn't used much.  The use of these buffers should probably be 
 * cleaned up.  -Alex
 */


#define CLIENT
#include "angband.h"
#include "netclient.h"
#ifdef AMIGA
#include <devices/timer.h>
#endif
#ifndef DUMB_WIN
#ifndef WINDOWS
#include <unistd.h>
#endif
#endif

#if 1
#include <sys/stat.h>
#endif

int			ticks = 0; // Keeps track of time in 100ms "ticks"
static bool		request_redraw;

static sockbuf_t	rbuf, cbuf, wbuf, qbuf;
static int		(*receive_tbl[256])(void),
			(*reliable_tbl[256])(void);
static unsigned		magic;
static long		last_send_anything,
			last_keyboard_change,
			last_keyboard_ack,
			//talk_resend,
			reliable_offset,
			reliable_full_len,
			latest_reliable;
static char		talk_pend[1024], initialized = 0;

/*
 * Initialize the function dispatch tables.
 * There are two tables.  One for the semi-important unreliable
 * data like frame updates.
 * The other one is for the reliable data stream, which is
 * received as part of the unreliable data packets.
 */
static void Receive_init(void)
{
	int i;

	for (i = 0; i < 256; i++)
	{
		receive_tbl[i] = NULL;
		reliable_tbl[i] = NULL;
	}

	receive_tbl[PKT_RELIABLE]	= Receive_reliable;
	receive_tbl[PKT_QUIT]		= Receive_quit;
	receive_tbl[PKT_START]		= Receive_start;
	receive_tbl[PKT_END]		= Receive_end;
/*	receive_tbl[PKT_CHAR]		= Receive_char;*/
	

	/*reliable_tbl[PKT_LEAVE]		= Receive_leave;*/
	receive_tbl[PKT_QUIT]		= Receive_quit;
	receive_tbl[PKT_STAT]		= Receive_stat;
	receive_tbl[PKT_HP]		= Receive_hp;
	receive_tbl[PKT_AC]		= Receive_ac;
	receive_tbl[PKT_INVEN]		= Receive_inven;
	receive_tbl[PKT_EQUIP]		= Receive_equip;
	receive_tbl[PKT_CHAR_INFO]	= Receive_char_info;
	receive_tbl[PKT_VARIOUS]	= Receive_various;
	receive_tbl[PKT_PLUSSES]	= Receive_plusses;
	receive_tbl[PKT_EXPERIENCE]	= Receive_experience;
	receive_tbl[PKT_GOLD]		= Receive_gold;
	receive_tbl[PKT_SP]		= Receive_sp;
	receive_tbl[PKT_HISTORY]	= Receive_history;
	receive_tbl[PKT_CHAR]		= Receive_char;
	receive_tbl[PKT_MESSAGE]	= Receive_message;
	receive_tbl[PKT_STATE]		= Receive_state;
	receive_tbl[PKT_TITLE]		= Receive_title;
	receive_tbl[PKT_DEPTH]		= Receive_depth;
	receive_tbl[PKT_CONFUSED]	= Receive_confused;
	receive_tbl[PKT_POISON]	= Receive_poison;
	receive_tbl[PKT_STUDY]		= Receive_study;
	receive_tbl[PKT_FOOD]		= Receive_food;
	receive_tbl[PKT_FEAR]		= Receive_fear;
	receive_tbl[PKT_SPEED]		= Receive_speed;
	receive_tbl[PKT_CUT]		= Receive_cut;
	receive_tbl[PKT_BLIND]		= Receive_blind;
	receive_tbl[PKT_STUN]		= Receive_stun;
	receive_tbl[PKT_ITEM]		= Receive_item;
	receive_tbl[PKT_SPELL_INFO]	= Receive_spell_info;
	receive_tbl[PKT_DIRECTION]	= Receive_direction;
	receive_tbl[PKT_FLUSH]		= Receive_flush;
	receive_tbl[PKT_LINE_INFO]	= Receive_line_info;
	receive_tbl[PKT_SPECIAL_OTHER]	= Receive_special_other;
	receive_tbl[PKT_STORE]		= Receive_store;
	receive_tbl[PKT_STORE_INFO]	= Receive_store_info;
	receive_tbl[PKT_SELL]		= Receive_sell;
	receive_tbl[PKT_TARGET_INFO]	= Receive_target_info;
	receive_tbl[PKT_SOUND]		= Receive_sound;
	receive_tbl[PKT_MINI_MAP]	= Receive_line_info;
	/* reliable_tbl[PKT_MINI_MAP]	= Receive_mini_map; */
	receive_tbl[PKT_SPECIAL_LINE]	= Receive_special_line;
	receive_tbl[PKT_FLOOR]		= Receive_floor;
	receive_tbl[PKT_PICKUP_CHECK]	= Receive_pickup_check;
	receive_tbl[PKT_PARTY]		= Receive_party;
	receive_tbl[PKT_SKILLS]		= Receive_skills;
	receive_tbl[PKT_PAUSE]		= Receive_pause;
	receive_tbl[PKT_MONSTER_HEALTH]	= Receive_monster_health;
	receive_tbl[PKT_SANITY]		= Receive_sanity;
	receive_tbl[PKT_SKILL_INIT]	= Receive_skill_init;
	receive_tbl[PKT_SKILL_MOD] 	= Receive_skill_info;
	receive_tbl[PKT_STORE_LEAVE] 	= Receive_store_kick;
}

// I haven't really figured out this function yet.  It looks to me like
// the whole thing could be replaced by one or two reads.  Why they decided
// to do this using UDP, I'll never know.  -APD

/*
 * I haven't really figured out this function yet too;  however I'm perfectly
 * sure I ruined what it used to do.  Please blame DG instead	- Jir -
 */
int Net_setup(void)
{
	int i, n, len, done = 0;
//	long todo = sizeof(setup_t);
	long todo = 1;
	char *ptr, str[20];

	ptr = (char *) &Setup;

	while (todo > 0)
	{
		if (cbuf.ptr != cbuf.buf)
			Sockbuf_advance(&cbuf, cbuf.ptr - cbuf.buf);

		len = cbuf.len;
#if 0
		if (len > todo)
			len = todo;
#endif	// 0

		if (len > 0)
		{
			if (done == 0)
			{
				n = Packet_scanf(&cbuf, "%ld%hd%c%c%ld",
					&Setup.motd_len, &Setup.frames_per_second, &Setup.max_race, &Setup.max_class, &Setup.setup_size);
				if (n <= 0)
				{
					errno = 0;
					quit("Can't read setup info from reliable data buffer");
				}

				/* allocate the arrays after loading */
				C_MAKE(race_info, Setup.max_race, player_race);
				C_MAKE(class_info, Setup.max_class, player_class);

				for (i = 0; i < Setup.max_race; i++)
				{
					Packet_scanf(&cbuf, "%s%ld", &str,
							&race_info[i].choice);
					race_info[i].title = string_make(str);
				}

				for (i = 0; i < Setup.max_class; i++)
				{
					Packet_scanf(&cbuf, "%s", &str);
					class_info[i].title = string_make(str);
				}

				ptr = (char *) &Setup;
				done = (char *) &Setup.motd[0] - ptr;
//				done = Setup.setup_size - Setup.motd_len;
				todo = Setup.motd_len;
			}
			else
			{
				memcpy(&ptr[done], cbuf.ptr, len);
				Sockbuf_advance(&cbuf, len + cbuf.ptr - cbuf.buf);
				done += len;
				todo -= len;
			}
		}

		if (todo > 0)
		{
			if (rbuf.ptr != rbuf.buf)
				Sockbuf_advance(&rbuf, rbuf.ptr - rbuf.buf);

			if (rbuf.len > 0)
			{
				/*
				if (rbuf.ptr[0] != PKT_RELIABLE)
				{
					if (rbuf.ptr[0] == PKT_QUIT)
					{
						quit("Server closed connection");
					}
					else
					{
						errno = 0;
						quit_fmt("Not a reliable packet (%d) in setup", rbuf.ptr[0]);
					}
				}
				*/
				if (Receive_reliable() == -1)
					return -1;
				//if (Sockbuf_flush(&wbuf) == -1)
				//	return -1;
			}

			if (cbuf.ptr != cbuf.buf)
				Sockbuf_advance(&cbuf, cbuf.ptr - cbuf.buf);

			if (cbuf.len > 0)
				continue;

		//	for (retries = 0;;retries++)
		//	{
			//	if (retries >= 10)
			//	{
			//		errno = 0;
			//		quit_fmt("Can't read setup after %d retries "
			//			 "(todo=%d, left=%d)",
			//			 retries, todo, cbuf.len - (cbuf.ptr - cbuf.buf));
			//	}

				SetTimeout(5, 0);
				if (!SocketReadable(rbuf.sock))
				{
					errno = 0;
					quit("No setup info received");
				}
				while (SocketReadable(rbuf.sock) > 0)
				{
					Sockbuf_clear(&rbuf);
					if (Sockbuf_read(&rbuf) == -1)
						quit("Can't read all of setup data");
					if (rbuf.len > 0)
						break;
					SetTimeout(0, 0);
				}
		//		if (rbuf.len > 0)
		//			break;
		//	}
		}
	}

	return 0;
}

/*
 * Send the first packet to the server with our name,
 * nick and display contained in it.
 * The server uses this data to verify that the packet
 * is from the right UDP connection, it already has
 * this info from the ENTER_GAME_pack.
 */
//int Net_verify(char *real, char *nick, char *pass, int sex, int race, int class)
int Net_verify(char *real, char *nick, char *pass)
{
	int	i, n,
		type,
		result;

	Sockbuf_clear(&wbuf);
//	n = Packet_printf(&wbuf, "%c%s%s%s%hd%hd%hd", PKT_VERIFY, real, nick, pass, sex, race, class);
	n = Packet_printf(&wbuf, "%c%s%s%s", PKT_VERIFY, real, nick, pass);

#if 0	// moved to Net_start
	/* Send the desired stat order */
	for (i = 0; i < 6; i++)
	{
		Packet_printf(&wbuf, "%hd", stat_order[i]);
	}

	/* Send class_extra */
	/*		Packet_printf(&wbuf, "%hd", class_extra); */
			
	/* Send the options */
	for (i = 0; i < 64; i++)
	{
		Packet_printf(&wbuf, "%c", Client_setup.options[i]);
	}

#ifndef BREAK_GRAPHICS
	/* Send the "unknown" redefinitions */
	for (i = 0; i < TV_MAX; i++)
	{
		Packet_printf(&wbuf, "%c%c", Client_setup.u_attr[i], Client_setup.u_char[i]);
	}

	/* Send the "feature" redefinitions */
	for (i = 0; i < MAX_F_IDX; i++)
	{
		Packet_printf(&wbuf, "%c%c", Client_setup.f_attr[i], Client_setup.f_char[i]);
	}

	/* Send the "object" redefinitions */
	for (i = 0; i < MAX_K_IDX; i++)
	{
		Packet_printf(&wbuf, "%c%c", Client_setup.k_attr[i], Client_setup.k_char[i]);
	}

	/* Send the "monster" redefinitions */
	for (i = 0; i < MAX_R_IDX; i++)
	{
		Packet_printf(&wbuf, "%c%c", Client_setup.r_attr[i], Client_setup.r_char[i]);
	}
#endif
#endif	// 0

	if (n <= 0 || Sockbuf_flush(&wbuf) <= 0)
	{
		plog("Can't send verify packet");
		//return -1;
	}

	SetTimeout(5, 0);
	if (!SocketReadable(rbuf.sock))
	{
		errno = 0;
		plog("No verify response");
		return -1;
	}
	Sockbuf_clear(&rbuf);
	if (Sockbuf_read(&rbuf) == -1)
	{
		plog("Can't read verify reply packet");
		return -1;
	}
	if (Receive_reliable() == -1)
		return -1;
	if (Sockbuf_flush(&wbuf) == -1)
		return -1;
	if (Receive_reply(&type, &result) <= 0)
	{
		errno = 0;	
		plog("Can't receive verify reply packet");
		return -1;
	}
	if (type != PKT_VERIFY)
	{
		errno = 0;
		plog(format("Verify wrong reply type (%d)", type));
		return -1;
	}
	if (result != PKT_SUCCESS)
	{
		errno = 0;
		plog(format("Verification failed (%d)", result));
		return -1;
	}
	if (Receive_magic() <= 0)
	{
		plog("Can't receive magic packet after verify");
		return -1;
	}

	return 0;
}


/*
 * Open the datagram socket and allocate the network data
 * structures like buffers.
 * Currently there are three different buffers used:
 * 1) wbuf is used only for sending packets (write/printf).
 * 2) rbuf is used for receiving packets in (read/scanf).
 * 3) cbuf is used to copy the reliable data stream
 *    into from the raw and unreliable rbuf packets.
 */
int Net_init(char *server, int fd)
{
	int		 sock;

	/*signal(SIGPIPE, SIG_IGN);*/

	Receive_init();

	sock = fd;

	wbuf.sock = sock;
	//if (SetSocketNonBlocking(sock, 1) == -1)
	//{
	//	plog("Can't make socket non-blocking");
	//	return -1;
	//}
	if (SetSocketNoDelay(sock, 1) == -1)
	{
		plog("Can't set TCP_NODELAY on socket");
		return -1;
	}
	if (SetSocketSendBufferSize(sock, CLIENT_SEND_SIZE + 256) == -1)
		plog(format("Can't set send buffer size to %d: error %d", CLIENT_SEND_SIZE + 256, errno));
	if (SetSocketReceiveBufferSize(sock, CLIENT_RECV_SIZE + 256) == -1)
		plog(format("Can't set receive buffer size to %d", CLIENT_RECV_SIZE + 256));


	/* reliable data buffer, not a valid socket filedescriptor needed */
	if (Sockbuf_init(&cbuf, -1, CLIENT_RECV_SIZE,
		SOCKBUF_WRITE | SOCKBUF_READ | SOCKBUF_LOCK) == -1)
	{
		plog(format("No memory for control buffer (%u)", CLIENT_RECV_SIZE));
		return -1;
	}

	/* queue buffer, not a valid socket filedescriptor needed */
	if (Sockbuf_init(&qbuf, -1, CLIENT_RECV_SIZE,
		SOCKBUF_WRITE | SOCKBUF_READ | SOCKBUF_LOCK) == -1)
	{
		plog(format("No memory for queue buffer (%u)", CLIENT_RECV_SIZE));
		return -1;
	}

	/* read buffer */
	if (Sockbuf_init(&rbuf, sock, CLIENT_RECV_SIZE,
		SOCKBUF_READ | SOCKBUF_WRITE) == -1)
	{
		plog(format("No memory for read buffer (%u)", CLIENT_RECV_SIZE));
		return -1;
	}

	/* write buffer */
	if (Sockbuf_init(&wbuf, sock, CLIENT_SEND_SIZE,
		SOCKBUF_WRITE) == -1)
	{
		plog(format("No memory for write buffer (%u)", CLIENT_SEND_SIZE));
		return -1;
	}

	/* reliable data byte stream offset */
	reliable_offset = 0;

	/* Initialized */
	initialized = 1;

	return 0;
}


/*
 * Cleanup all the network buffers and close the datagram socket.
 * Also try to send the server a quit packet if possible.
 */
void Net_cleanup(void)
{
	int	sock = wbuf.sock;
	char	ch;

	if (sock > 2)
	{
		ch = PKT_QUIT;
		if (DgramWrite(sock, &ch, 1) != 1)
		{
			GetSocketError(sock);
			DgramWrite(sock, &ch, 1);
		}
		Term_xtra(TERM_XTRA_DELAY, 50);

		DgramClose(sock);
	}

	Sockbuf_cleanup(&rbuf);
	Sockbuf_cleanup(&cbuf);
	Sockbuf_cleanup(&wbuf);
	Sockbuf_cleanup(&qbuf);

	// Make sure that we won't try to write to the socket again,
	// after our connection has closed
	wbuf.sock = -1;
}


/*
 * Flush the network output buffer if it has some data in it.
 * Called by the main loop before blocking on a select(2) call.
 */
int Net_flush(void)
{
	//if (talk_resend < last_turns)
	//	Send_msg(NULL);
	if (wbuf.len == 0)
	{
		wbuf.ptr = wbuf.buf;
		return 0;
	}
	if (Sockbuf_flush(&wbuf) == -1)
		return -1;
	Sockbuf_clear(&wbuf);
	last_send_anything = ticks;
	return 1;
}


/*
 * Return the socket filedescriptor for use in a select(2) call.
 */
int Net_fd(void)
{
	if (!initialized)
		return -1;
	return rbuf.sock;
}


/*
 * Try to send a `start play' packet to the server and get an
 * acknowledgement from the server.  This is called after
 * we have initialized all our other stuff like the user interface
 * and we also have the map already.
 */
//int Net_start(void)
int Net_start(int sex, int race, int class)
{
	int	i, n;
	int		type,
			result;

	Sockbuf_clear(&wbuf);
	n = Packet_printf(&wbuf, "%c", PKT_PLAY);
	Packet_printf(&wbuf, "%hd%hd%hd", sex, race, class);

#if 1	// moved from Net_verify
	/* Send the desired stat order */
	for (i = 0; i < 6; i++)
	{
		Packet_printf(&wbuf, "%hd", stat_order[i]);
	}

	/* Send class_extra */
	/*		Packet_printf(&wbuf, "%hd", class_extra); */
			
	/* Send the options */
	for (i = 0; i < OPT_MAX; i++)
	{
		Packet_printf(&wbuf, "%c", Client_setup.options[i]);
	}

#ifndef BREAK_GRAPHICS
	/* Send the "unknown" redefinitions */
	for (i = 0; i < TV_MAX; i++)
	{
		Packet_printf(&wbuf, "%c%c", Client_setup.u_attr[i], Client_setup.u_char[i]);
	}

	/* Send the "feature" redefinitions */
	for (i = 0; i < MAX_F_IDX; i++)
	{
		Packet_printf(&wbuf, "%c%c", Client_setup.f_attr[i], Client_setup.f_char[i]);
	}

	/* Send the "object" redefinitions */
	for (i = 0; i < MAX_K_IDX; i++)
	{
		Packet_printf(&wbuf, "%c%c", Client_setup.k_attr[i], Client_setup.k_char[i]);
	}

	/* Send the "monster" redefinitions */
	for (i = 0; i < MAX_R_IDX; i++)
	{
		Packet_printf(&wbuf, "%c%c", Client_setup.r_attr[i], Client_setup.r_char[i]);
	}
#endif
#endif	// 0

	if (Sockbuf_flush(&wbuf) == -1)
	{
		quit("Can't send start play packet");
	}
	// Wait for data to arrive
	SetTimeout(5, 0);	
	if (!SocketReadable(rbuf.sock))
	{
		errno = 0;
		quit("No play packet reply received.");
	}

	//while (cbuf.len <= 0 && SocketReadable(rbuf.sock) != 0)
	//{
		Sockbuf_clear(&rbuf);
		if (Sockbuf_read(&rbuf) == -1)
		{
			quit("Error reading play reply");
			return -1;
		}

	//	if (rbuf.len <= 0)
	//		continue;
		/*
		if (rbuf.ptr[0] != PKT_RELIABLE)
		{
			if (rbuf.ptr[0] == PKT_QUIT)
			{
				errno = 0;
				quit(&rbuf.ptr[1]);
				return -1;
			}
			else if (rbuf.ptr[0] == PKT_END)
			{
				Sockbuf_clear(&rbuf);
				continue;
			}
			else
			{
				printf("strange packet type while starting (%d)\n", rbuf.ptr[0]);
				Sockbuf_clear(&rbuf);
				continue;
			}
		}
		*/
		if (Receive_reliable() == -1)
			return -1;
		//if (Sockbuf_flush(&wbuf) == -1)
		//	return -1;
	//}

	//if (cbuf.ptr - cbuf.buf >= cbuf.len)
	//{
	//	continue;
	//}
		
	/* If our connection wasn't accepted, quit */
	if (cbuf.ptr[0] == PKT_QUIT)
	{
		errno = 0;
		quit(&rbuf.ptr[1]);
		return -1;
	}
	if (cbuf.ptr[0] != PKT_REPLY)
	{
		errno = 0;
		quit(format("Not a reply packet after play (%d,%d,%d)",
			cbuf.ptr[0], cbuf.ptr - cbuf.buf, cbuf.len));
		return -1;
	}
	if (Receive_reply(&type, &result) <= 0)
	{
		errno = 0;
		quit("Can't receive reply packet after play");
		return -1;
	}
	if (type != PKT_PLAY)
	{
		errno = 0;
		quit("Can't receive reply packet after play");
		return -1;
	}
	if (result != PKT_SUCCESS)
	{
		errno = 0;
		quit(format("Start play not allowed (%d)", result));
		return -1;
	}
	// Finish processing any commands that were sent to us along with
	// the PKT_PLAY packet.
	
	// Advance past our PKT_PLAY data
	//Sockbuf_advance(&rbuf, 3);
	// Actually process any leftover commands in rbuf
	if (Net_packet() == -1)
	{
		return -1;
	}

	errno = 0;
	return 0;
}


/*
 * Process a packet which most likely is a frame update,
 * perhaps with some reliable data in it.
 */
static int Net_packet(void)
{
	int		type,
			prev_type = 0,
			result,
			replyto,
			status;

	/* Hack -- copy cbuf to rbuf since this is where this function
	 * expects the data to be.
	 */
	Sockbuf_clear(&rbuf);

	/* Hack -- assume that we have already processed any data that
	 * cbuf.ptr is pointing past.
	 */
	Sockbuf_advance(&cbuf, cbuf.ptr - cbuf.buf);
	if (Sockbuf_write(&rbuf, cbuf.ptr, cbuf.len) != cbuf.len)
	{
		plog("Can't copy reliable data to buffer");
		return -1;
	}
	Sockbuf_clear(&cbuf);

	/* Process all of the received client updates */
	while (rbuf.buf + rbuf.len > rbuf.ptr)
	{
		type = (*rbuf.ptr & 0xFF);
		if (receive_tbl[type] == NULL)
		{
			errno = 0;
			plog(format("Received unknown packet type (%d, %d), dropping",
				type, prev_type));
			Sockbuf_clear(&rbuf);
			break;
		}
		else{
		if ((result = (*receive_tbl[type])()) <= 0)
		{
			if (result == -1)
			{
				if (type != PKT_QUIT)
				{
					errno = 0;
					plog(format("Processing packet type (%d, %d) failed", type, prev_type));
				}
				return -1;
			}
			Sockbuf_clear(&rbuf);
			break;
		}
		}
		prev_type = type;
	}
#if 0
	// Process all the queued client updates
	while (cbuf.buf + cbuf.len > cbuf.ptr) 
	{
		/* Reset full length */
		//reliable_full_len = 0;

		type = (*cbuf.ptr & 0xFF);
		if (type == PKT_REPLY)
		{
			if ((result = Receive_reply(&replyto, &status)) <= 0)
			{
				if (result == 0)
					break;
				return -1;
			}
			errno = 0;
			plog(format("Got reply packet (%d, %d)", replyto, status));
		}
		else if (receive_tbl[type] == NULL)
		{
			errno = 0;
			 //plog(format("Receive unknown reliable data packet type (%d, %d, %d)",
			//	type, cbuf.ptr - cbuf.buf, cbuf.len)); 

			 /* we have received bad data, ignore this packet */
			Sockbuf_clear(&cbuf);

			break;
		}
		else if ((result = (*receive_tbl[type])()) <= 0)
		{
			if (result == 0)
				break;
			return -1;
		}
	}
#endif
	return 0;
}


/*
 * Read a packet into one of the input buffers.
 * If it is a frame update then we check to see
 * if it is an old or duplicate one.  If it isn't
 * a new frame then the packet is discarded and
 * we retry to read a packet once more.
 * It's a non-blocking read.
 */
static int Net_read(void)
{
	int	n;

	for (;;)
	{
		/* Wait for data to appear -- Dave Thaler */
		while (!SocketReadable(Net_fd()));

		if ((n = Sockbuf_read(&rbuf)) == -1)
		{
			plog("Net input error");
			return -1;
		}
		if (rbuf.len <= 0)
		{
			Sockbuf_clear(&rbuf);
			return 0;
		}

		/* If this is the end of a chunk of info, quit reading stuff */
		if (rbuf.ptr[rbuf.len - 1] == PKT_END)
			return 1;
	}
}


/*
 * Read frames from the net until there are no more available.
 * If the server has flooded us with frame updates then we should
 * discard everything except the most recent ones.  The X server
 * may be too slow to keep up with the rate of the XPilot server
 * or there may have been a network hickup if the net is overloaded.
 */
int Net_input(void)
{
	int	n;

	/* First, clear the buffer */
	Sockbuf_clear(&rbuf);

	/* Get some new data */
	if ((n = Net_read()) <= 0)
	{
		return n;
	}

	/* Write the received data to the command buffer */
	if (Sockbuf_write(&cbuf, rbuf.ptr, rbuf.len) != rbuf.len)
	{
		plog("Can't copy reliable data to buffer in Net_input");
		return -1;
	}

	n = Net_packet();

	Sockbuf_clear(&rbuf);

	if (n == -1)
		return -1;

	return 1;
}

int Flush_queue(void)
{
	short	len;

	if (!initialized) return 0;

	len = qbuf.len;

	Net_packet();

	if (cbuf.ptr > cbuf.buf)
		Sockbuf_advance(&cbuf, cbuf.ptr - cbuf.buf);
	if (Sockbuf_write(&cbuf, qbuf.ptr, len) != len)
	{
		errno = 0;
		plog("Can't copy queued data to buffer");
		qbuf.ptr += len;
		Sockbuf_advance(&qbuf, qbuf.ptr - qbuf.buf);
		return -1;
	}
/*	reliable_offset += len; */
	qbuf.ptr += len;
	Sockbuf_advance(&qbuf, qbuf.ptr - qbuf.buf);

	Sockbuf_clear(&qbuf);

	/* If a redraw has been requested, send the request */
	if (request_redraw)
	{
		Send_redraw();
		request_redraw = FALSE;
	}

	return 1;
}

/*
 * Receive the beginning of a new frame update packet,
 * which contains the loops number.
 */
int Receive_start(void)
{
	int	n;
	long	loops;
	byte	ch;
	long	key_ack;

	if ((n = Packet_scanf(&rbuf, "%c%ld%ld", &ch, &loops, &key_ack)) <= 0)
			return n;

	/*
	if (last_turns >= loops)
	{
		printf("ignoring frame (%ld)\n", last_turns - loops);
		return 0;
	}
	last_turns = loops;
	*/
	if (key_ack > last_keyboard_ack)
	{
		if (key_ack > last_keyboard_change)
		{
			printf("Premature keyboard ack by server (%ld,%ld,%ld)\n",
				last_keyboard_change, last_keyboard_ack, key_ack);
			return 0;
		}
		else last_keyboard_ack = key_ack;
	}
#if 0
	if ((n = Handle_start(loops)) == -1)
		return -1;
#endif

	return 1;
}

/*
 * Receive the end of a new frame update packet,
 * which should contain the same loops number
 * as the frame head.  If this terminating packet
 * is missing then the packet is corrupt or incomplete.
 */
int Receive_end(void)
{
	int	n;
	byte	ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0)
		return n;

#if 0
	if ((n = Handle_end(loops)) == -1)
		return -1;
#endif

	return 1;
}


int Receive_magic(void)
{
	int	n;
	byte	ch;

	if ((n = Packet_scanf(&cbuf, "%c%u", &ch, &magic)) <= 0)
		return n;
	return 1;
}

int Send_ack(long rel_loops)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%ld%ld", PKT_ACK,
		reliable_offset, rel_loops)) <= 0)
	{
		if (n == 0)
			return 0;
		plog("Can't ack reliable data");
		return -1;
	}

	return 1;
}

int old_Receive_reliable(void)
{
	int	n;
	short	len;
	byte	ch;
	long	rel, rel_loops, full_len;

	if ((n = Packet_scanf(&rbuf, "%c%hd%ld%ld%ld", &ch, &len, &rel, &rel_loops, &full_len)) == -1)
		return -1;

	if (n == 0)
	{
		errno = 0;
		plog("Incomplete reliable data packet");
		return 0;
	}

	if (len <= 0)
	{
		errno = 0;
		plog(format("Bad reliable data length (%d)", len));
		return -1;
	}

	if (full_len <= 0)
	{
		errno = 0;
		plog(format("Bad reliable data full length (%d)", reliable_full_len));
		return -1;
	}

	/* Track maximum full length */
	if (full_len > reliable_full_len || rel_loops > latest_reliable)
	{
		reliable_full_len = full_len;
		latest_reliable = rel_loops;
		//last_turns = rel_loops;
	}

	if (rbuf.ptr + len > rbuf.buf + rbuf.len)
	{
		errno = 0;
		plog(format("Not all reliable data in packet (%d, %d, %d)",
			rbuf.ptr - rbuf.buf, len, rbuf.len));
		rbuf.ptr += len;
		Sockbuf_advance(&rbuf, rbuf.ptr - rbuf.buf);
		return -1;
	}
	if (rel > reliable_offset)
	{
		rbuf.ptr += len;
		Sockbuf_advance(&rbuf, rbuf.ptr - rbuf.buf);
		if (Send_ack(rel_loops) == -1)
			return -1;
		return 1;
	}
	if (rel + len <= reliable_offset)
	{
		rbuf.ptr += len;
		Sockbuf_advance(&rbuf, rbuf.ptr - rbuf.buf);
		if (Send_ack(rel_loops) == -1)
			return -1;
		return 1;
	}
	if (rel < reliable_offset)
	{
		len -= reliable_offset - rel;
		rbuf.ptr += reliable_offset - rel;
		rel = reliable_offset;
	}
	if (cbuf.ptr > cbuf.buf)
		Sockbuf_advance(&cbuf, cbuf.ptr - cbuf.buf);
	if (Sockbuf_write(&cbuf, rbuf.ptr, len) != len)
	{
		errno = 0;
		plog("Can't copy reliable data to buffer");
		rbuf.ptr += len;
		Sockbuf_advance(&rbuf, rbuf.ptr - rbuf.buf);
		return -1;
	}
	reliable_offset += len;
	rbuf.ptr += len;
	Sockbuf_advance(&rbuf, rbuf.ptr - rbuf.buf);
	if (Send_ack(rel_loops) == -1)
		return -1;
	return 1;
}

int Receive_reliable(void)
{
	if (Sockbuf_write(&cbuf, rbuf.ptr, rbuf.len) != rbuf.len)
	{
		plog("Can't copy reliable data to buffer");
		return -1;
	}
	Sockbuf_clear(&rbuf);
	return 1;

}


int Receive_reply(int *replyto, int *result)
{
	int	n;
	byte	type, ch1, ch2;

	n = Packet_scanf(&cbuf, "%c%c%c", &type, &ch1, &ch2);
	if (n <= 0)
		return n;
	if (n != 3 || type != PKT_REPLY)
	{
		plog("Can't receive reply packet");
		return 1;
	}
	*replyto = ch1;
	*result = ch2;
	return 1;
}

int Receive_quit(void)
{
	unsigned char		pkt;
	sockbuf_t		*sbuf;
	char			reason[MAX_CHARS];

	if (rbuf.ptr < rbuf.buf + rbuf.len)
		sbuf = &rbuf;
	else sbuf = &cbuf;
	if (Packet_scanf(sbuf, "%c", &pkt) != 1)
	{
		errno = 0;
		plog("Can't read quit packet");
	}
	else
	{
		if (Packet_scanf(sbuf, "%s", reason) <= 0)
			strcpy(reason, "unknown reason");
		errno = 0;
		quit(format("Quitting: %s", reason));
	}
	return -1;
}

int Receive_sanity(void)
{
#ifdef SHOW_SANITY
	int n;
	char ch;
	s16b max, cur;
	if ((n = Packet_scanf(&rbuf, "%c%hd%hd", &ch, &max, &cur)) <= 0)
	{
		return n;
	}
	p_ptr->msane=max;
	p_ptr->csane=cur;
	if (!screen_icky && !shopping){
		prt_sane();
	}
	else
		if ((n = Packet_printf(&qbuf, "%c%hd%hd", ch, stat, max, cur)) <= 0)
		{
			return n;
		}
	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
#endif
}

int Receive_stat(void)
{
	int	n;
	char	ch;
	char	stat;
	s16b	max, cur;

	if ((n = Packet_scanf(&rbuf, "%c%c%hd%hd", &ch, &stat, &max, &cur)) <= 0)
	{
		return n;
	}

	p_ptr->stat_top[(int) stat] = max;
	p_ptr->stat_use[(int) stat] = cur;

	if (!screen_icky && !shopping)
		prt_stat(stat, max, cur);
	else
		if ((n = Packet_printf(&qbuf, "%c%c%hd%hd", ch, stat, max, cur)) <= 0)
		{
			return n;
		}


	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_hp(void)
{
	int	n;
	char 	ch;
	s16b	max, cur;

	if ((n = Packet_scanf(&rbuf, "%c%hd%hd", &ch, &max, &cur)) <= 0)
	{
		return n;
	}

	p_ptr->mhp = max;
	p_ptr->chp = cur;

	if (!screen_icky && !shopping)
		prt_hp(max, cur);
	else
		if ((n = Packet_printf(&qbuf, "%c%hd%hd", ch, max, cur)) <= 0)
		{
			return n;
		}

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_ac(void)
{
	int	n;
	char	ch;
	s16b	base, plus;

	if ((n = Packet_scanf(&rbuf, "%c%hd%hd", &ch, &base, &plus)) <= 0)
	{
		return n;
	}

	p_ptr->dis_ac = base;
	p_ptr->dis_to_a = plus;

	if (!screen_icky && !shopping)
		prt_ac(base + plus);
	else
		if ((n = Packet_printf(&qbuf, "%c%hd%hd", ch, base, plus)) <= 0)
		{
			return n;
		}

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_inven(void)
{
	int	n;
	char	ch;
	char pos, attr, tval, sval;
	s16b wgt, amt;
	char name[80];

	if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%s", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, name)) <= 0)
	{
		return n;
	}

        /* Hack -- The color is stored in the sval, since we don't use it for anything else */
        /* Hack -- gotta ahck to work around the previous hackl .. damn I hate that */
		/* I'm one of those who really hate it .. Jir */
	inventory[pos - 'a'].sval = attr;
	inventory[pos - 'a'].tval = tval;
	inventory[pos - 'a'].xtra1 = sval;
	inventory[pos - 'a'].weight = wgt;
	inventory[pos - 'a'].number = amt;

	strncpy(inventory_name[pos - 'a'], name, 79);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN);

	return 1;
}

int Receive_equip(void)
{
	int	n;
	char 	ch;
	char pos, attr, tval, sval;
	s16b wgt, amt;
	char name[80];

//	if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%c%s", &ch, &pos, &attr, &wgt, &tval, name)) <= 0)
	if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%s", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, name)) <= 0)
	{
		return n;
	}

	inventory[pos - 'a' + INVEN_WIELD].sval = attr;
	inventory[pos - 'a' + INVEN_WIELD].tval = tval;
	inventory[pos - 'a' + INVEN_WIELD].xtra1 = sval;
	inventory[pos - 'a' + INVEN_WIELD].weight = wgt;
	inventory[pos - 'a' + INVEN_WIELD].number = amt;


	strncpy(inventory_name[pos - 'a' + INVEN_WIELD], name, 79);

	/* Window stuff */
	p_ptr->window |= (PW_EQUIP);

	return 1;
}

int Receive_char_info(void)
{
	int	n;
	char	ch;
	static bool pref_files_loaded = FALSE;

	/* Clear any old info */
	race = class = sex = 0;

	if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd", &ch, &race, &class, &sex)) <= 0)
	{
		return n;
	}

        p_ptr->prace = race;
        p_ptr->rp_ptr = &race_info[race];
        p_ptr->pclass = class;
        p_ptr->cp_ptr = &class_info[class];
	p_ptr->male = sex;

	/* Mega-hack -- Read pref files if we haven't already */
	if (!pref_files_loaded)
	{
		/* Load */
		initialize_all_pref_files();

		/* Pref files are now loaded */
		pref_files_loaded = TRUE;
	}

	if (!screen_icky && !shopping)
		prt_basic();
	else
		if ((n = Packet_printf(&qbuf, "%c%hd%hd%hd", ch, race, class, sex)) <= 0)
		{
			return n;
		}

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_various(void)
{
	int	n;
	char	ch;
	s16b	hgt, wgt, age, sc;

	if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu%hu", &ch, &hgt, &wgt, &age, &sc)) <= 0)
	{
		return n;
	}

	p_ptr->ht = hgt;
	p_ptr->wt = wgt;
	p_ptr->age = age;
	p_ptr->sc = sc;

	/*printf("Received various info: height %d, weight %d, age %d, sc %d\n", hgt, wgt, age, sc);*/

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_plusses(void)
{
	int	n;
	char	ch;
	s16b	dam, hit;
	s16b	dam_r, hit_r;
	s16b	dam_m, hit_m;

	if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd%hd%hd%hd", &ch, &hit, &dam, &hit_r, &dam_r, &hit_m, &dam_m)) <= 0)
	{
		return n;
	}

	p_ptr->dis_to_h = hit;
	p_ptr->dis_to_d = dam;
	p_ptr->to_h_ranged = hit_r;
	p_ptr->to_d_ranged = dam_r;
	p_ptr->to_h_melee = hit_m;
	p_ptr->to_d_melee = dam_m;

	/*printf("Received plusses: +%d tohit +%d todam\n", hit, dam);*/

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_experience(void)
{
	int	n;
	char	ch;
	int	max, cur, adv;
	s16b	lev;

	if ((n = Packet_scanf(&rbuf, "%c%hu%d%d%d", &ch, &lev, &max, &cur, &adv)) <= 0)
	{
		return n;
	}

	p_ptr->lev = lev;
	p_ptr->max_exp = max;
	p_ptr->exp = cur;
	exp_adv = adv;

	if (!screen_icky && !shopping)
		prt_level(lev, max, cur, adv);
	else
		if ((n = Packet_printf(&qbuf, "%c%hu%d%d%d", ch, lev, max, cur, adv)) <= 0)
		{
			return n;
		}

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_skill_init(void)
{
	int	n;
	char	ch;
	int	i, type, father, mkey, order;
	char    buf[300];
	u32b	flags1;
	byte	tval;

//	if ((n = Packet_scanf(&rbuf, "%c%ld%ld%ld%ld%ld%S", &ch, &type, &i, &father, &order, &mkey, buf)) <= 0)
	if ((n = Packet_scanf(&rbuf, "%c%ld%ld%ld%ld%ld%S%d%c", &ch, &type, &i, &father, &order, &mkey, buf, &flags1, &tval)) <= 0)
	{
		return n;
	}

	if (type == PKT_SKILL_INIT_NAME)
		s_info[i].name = string_make(buf);
	if (type == PKT_SKILL_INIT_DESC)
		s_info[i].desc = string_make(buf);
	else
		s_info[i].action_desc = string_make(buf);

	s_info[i].father = father;
	s_info[i].order = order;
	s_info[i].action_mkey = mkey;
	s_info[i].flags1 = flags1;
	s_info[i].tval = tval;

	return 1;
}

int Receive_skill_info(void)
{
	int	n;
        char	ch;
        s32b    val;
	int	i, mod, dev, hidden, pt;
	char    buf[300];

	if ((n = Packet_scanf(&rbuf, "%c%ld%ld%ld%ld%ld%ld", &ch, &pt, &i, &val, &mod, &dev, &hidden)) <= 0)
	{
		return n;
	}

        p_ptr->skill_points = pt;
        p_ptr->s_info[i].value = val;
        p_ptr->s_info[i].mod = mod;
        p_ptr->s_info[i].dev = dev;
        p_ptr->s_info[i].hidden = hidden;

        /* Tell the skill screen we got the info we needed */
        hack_do_cmd_skill_wait = FALSE;
	return 1;
}

int Receive_gold(void)
{
	int	n;
	char	ch;
	int	gold;

	if ((n = Packet_scanf(&rbuf, "%c%d", &ch, &gold)) <= 0)
	{
		return n;
	}

	p_ptr->au = gold;

	if (shopping)
	{
	        char out_val[64];

	        prt("Gold Remaining: ", 19, 53);

	        sprintf(out_val, "%9ld", (long) gold);
	        prt(out_val, 19, 68);
	}
	else if (!screen_icky)
		prt_gold(gold);
	else
		if ((n = Packet_printf(&qbuf, "%c%d", ch, gold)) <= 0)
		{
			return n;
		}

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_sp(void)
{
	int	n;
	char	ch;
	s16b	max, cur;

	if ((n = Packet_scanf(&rbuf, "%c%hd%hd", &ch, &max, &cur)) <= 0)
	{
		return n;
	}

	p_ptr->msp = max;
	p_ptr->csp = cur;

	if (!screen_icky && !shopping)
		prt_sp(max, cur);
	else
		if ((n = Packet_printf(&qbuf, "%c%hd%hd", ch, max, cur)) <= 0)
		{
			return n;
		}

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_history(void)
{
	int	n;
	char	ch;
	s16b	line;
	char	buf[80];

	if ((n = Packet_scanf(&rbuf, "%c%hu%s", &ch, &line, buf)) <= 0)
	{
		return n;
	}

	strcpy(p_ptr->history[line], buf);

	/*printf("Received history line %d: %s\n", line, buf);*/

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_char(void)
{
	int	n;
	char	ch;
	char	x, y;
	char	a, c;

	if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c", &ch, &x, &y, &a, &c)) <= 0)
	{
		return n;
	}

	if (!screen_icky && !shopping)
	{
		Term_draw(x, y, a, c);

		/* Put the cursor there */
//		Term_gotoxy(x, y);	/* It's disagreeable in GCU */
	}
	else
		if ((n = Packet_printf(&qbuf, "%c%c%c%c%c", ch, x, y, a, c)) <= 0)
		{
			return n;
		}

	return 1;
}

int Receive_message(void)
{
	int	n, c;
	char	ch;
	char	buf[1024], search[1024], *ptr;

	if ((n = Packet_scanf(&rbuf, "%c%s", &ch, buf)) <= 0)
	{
		return n;
	}

	/* XXX Mega-hack -- because we are not using checksums, sometimes under
	 * heavy traffic Receive_line_input receives corrupted data, which will cause
	 * the run-length encoded algorithm to exit prematurely.  Since there is no 
	 * end of line_info tag, the packet interpretor assumes the line_input data
	 * is finished, and attempts to parse the next byte as a new packet type.
	 * Since the ascii value of '.' (a frequently updated character) is 46, 
	 * which equals PKT_MESSAGE, if corrupted line_info data is received this function
	 * may get called wihtout a valid string.  To try to prevent the client from
	 * displaying a garbled string and messing up our screen, we will do a quick
	 * sanity check on the string.  Note that this might screw up people using
	 * international character sets.  (Such as the Japanese players)
	 * 
	 * A better solution for this would be to impliment some kind of packet checksum.
	 * -APD
	 */

	/* perform a sanity check on our string */
	/* Hack -- ' ' is numericall the lowest charcter we will probably be trying to
	 * display.  This might screw up international character sets.
	 */
	for (c = 0; c < n; c++) if (buf[c] < ' ' && buf[c]!=-1) return 1;

/*	printf("Message: %s\n", buf);   */

	sprintf(search, "%s] ", nick);

	if (strstr(buf, search) != 0 && *talk_pend)
	{
		ptr = strstr(talk_pend, strchr(buf, ']') + 2);
		ptr = strtok(ptr, "\t");
		ptr = strtok(NULL, "\t");
		if (ptr) strcpy(talk_pend, ptr);
		else strcpy(talk_pend, "");
	}

	if (!topline_icky && (party_mode || shopping || !screen_icky))
	{
		c_msg_print(buf);
	}
	else
		if ((n = Packet_printf(&qbuf, "%c%s", ch, buf)) <= 0)
		{
			return n;
		}

	return 1;
}

int Receive_state(void)
{
	int	n;
	char	ch;
	s16b	paralyzed, searching, resting;

	if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu", &ch, &paralyzed, &searching, &resting)) <= 0)
	{
		return n;
	}

	if (!screen_icky && !shopping)
		prt_state(paralyzed, searching, resting);
	else
		if ((n = Packet_printf(&qbuf, "%c%hu%hu%hu", ch, paralyzed, searching, resting)) <= 0)
		{
			return n;
		}
	
	return 1;
}

int Receive_title(void)
{
	int	n;
	char	ch;
	char	buf[80];

	if ((n = Packet_scanf(&rbuf, "%c%s", &ch, buf)) <= 0)
	{
		return n;
	}

	/* XXX -- Extract "ghost-ness" */
	p_ptr->ghost = streq(buf, "Ghost");

	if (!screen_icky && !shopping)
		prt_title(buf);
	else
		if ((n = Packet_printf(&qbuf, "%c%s", ch, buf)) <= 0)
		{
			return n;
		}

	return 1;
}

int Receive_depth(void)
{
	int	n;
	char	ch;
	s16b	x,y,z,recall;
	bool town;

	if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu%c%hu", &ch, &x, &y, &z, &town, &recall)) <= 0)
	{
		return n;
	}

	if (!screen_icky && !shopping)
		prt_depth(x, y, z, town, recall);
	else
		if ((n = Packet_printf(&qbuf, "%c%hu%hu%hu%c%hu", ch, x, y, z, town, recall)) <= 0)
		{
			return n;
		}

	return 1;
}

int Receive_confused(void)
{
	int	n;
	char	ch;
	bool	confused;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &confused)) <= 0)
	{
		return n;
	}

	if (!screen_icky && !shopping)
		prt_confused(confused);
	else
		if ((n = Packet_printf(&qbuf, "%c%c", ch, confused)) <= 0)
		{
			return n;
		}

	return 1;
}
	
int Receive_poison(void)
{
	int	n;
	char	ch;
	bool	poison;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &poison)) <= 0)
	{
		return n;
	}

	if (!screen_icky && !shopping)
		prt_poisoned(poison);
	else
		if ((n = Packet_printf(&qbuf, "%c%c", ch, poison)) <= 0)
		{
			return n;
		}

	return 1;
}
	
int Receive_study(void)
{
	int	n;
	char	ch;
	bool	study;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &study)) <= 0)
	{
		return n;
	}

	if (!screen_icky && !shopping)
		prt_study(study);
	else
		if ((n = Packet_printf(&qbuf, "%c%c", ch, study)) <= 0)
		{
			return n;
		}

	return 1;
}

int Receive_food(void)
{
	int	n;
	char	ch;
	u16b	food;

	if ((n = Packet_scanf(&rbuf, "%c%hu", &ch, &food)) <= 0)
	{
		return n;
	}

	if (!screen_icky && !shopping)
		prt_hunger(food);
	else
		if ((n = Packet_printf(&qbuf, "%c%hu", ch, food)) <= 0)
		{
			return n;
		}

	return 1;
}

int Receive_fear(void)
{
	int	n;
	char	ch;
	bool	afraid;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &afraid)) <= 0)
	{
		return n;
	}

	if (!screen_icky && !shopping)
		prt_afraid(afraid);
	else
		if ((n = Packet_printf(&qbuf, "%c%c", ch, afraid)) <= 0)
		{
			return n;
		}

	return 1;
}

int Receive_speed(void)
{
	int	n;
	char	ch;
	s16b	speed;

	if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &speed)) <= 0)
	{
		return n;
	}

	if (!screen_icky && !shopping)
		prt_speed(speed);
	else
		if ((n = Packet_printf(&qbuf, "%c%hd", ch, speed)) <= 0)
		{
			return n;
		}

	return 1;
}

int Receive_cut(void)
{
	int	n;
	char	ch;
	s16b	cut;

	if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &cut)) <= 0)
	{
		return n;
	}

	if (!screen_icky && !shopping)
		prt_cut(cut);
	else
		if ((n = Packet_printf(&qbuf, "%c%hd", ch, cut)) <= 0)
		{
			return n;
		}

	return 1;
}

int Receive_blind(void)
{
	int	n;
	char	ch;
	bool	blind;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &blind)) <= 0)
	{
		return n;
	}

	if (!screen_icky && !shopping)
		prt_blind(blind);
	else
		if ((n = Packet_printf(&qbuf, "%c%c", ch, blind)) <= 0)
		{
			return n;
		}

	return 1;
}

int Receive_stun(void)
{
	int	n;
	char	ch;
	s16b	stun;

	if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &stun)) <= 0)
	{
		return n;
	}

	if (!screen_icky && !shopping)
		prt_stun(stun);
	else
		if ((n = Packet_printf(&qbuf, "%c%hd", ch, stun)) <= 0)
		{
			return n;
		}

	return 1;
}

int Receive_item(void)
{
	char	ch;
	int	n, item;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0)
	{
		return n;
	}

	if (!screen_icky && !topline_icky)
	{
		c_msg_print(NULL);

		if (!c_get_item(&item, "Which item? ", TRUE, TRUE, FALSE))
		{
			return 1;
		}

		Send_item(item);
	}
	else
		if ((n = Packet_printf(&qbuf, "%c", ch)) <= 0)
		{
			return n;
		}
	
	return 1;
}

int Receive_spell_info(void)
{
	char	ch;
	int	n;
	u16b	realm, book, line;
	char	buf[80];
	s32b    spells[3];

	if ((n = Packet_scanf(&rbuf, "%c%ld%ld%ld%hu%hu%hu%s", &ch, &spells[0], &spells[1], &spells[2], &realm, &book, &line, buf)) <= 0)
	{
		return n;
	}

	/* Save the info */
        strcpy(spell_info[realm][book][line], buf);
	p_ptr->innate_spells[0] = spells[0];
	p_ptr->innate_spells[1] = spells[1];
	p_ptr->innate_spells[2] = spells[2];

	return 1;
}

int Receive_direction(void)
{
	char	ch;
	int	n, dir = 0;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0)
	{
		return n;
	}

	if (!screen_icky && !topline_icky && !shopping)
	{
		/* Ask for a direction */
		get_dir(&dir);

		/* Send it back */
		if ((n = Packet_printf(&wbuf, "%c%c", PKT_DIRECTION, dir)) <= 0)
		{
			return n;
		}
	}
	else
		if ((n = Packet_printf(&qbuf, "%c", ch)) <= 0)
		{
			return n;
		}

	return 1;
}

int Receive_flush(void)
{
	char	ch;
	int	n;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0)
	{
		return n;
	}

	/* Flush the terminal */
	Term_fresh();

	/* Wait */
	/*
	 * NOTE: this is very what makes the server freeze when a pack of
	 * hounds breath at the player.. 1ms is too long.	- Jir -
	 *
	 * thin_down_flush is a crude hack to save this from happening.
	 * of course, better if we can specify 0.3ms sleep - helas, windoze
	 * client doesn't support that :-/
	 */
//	if (!thin_down_flush || magik(33)) Term_xtra(TERM_XTRA_DELAY, 1);
	if (c_cfg.thin_down_flush)
	{
		if (++flush_count > 10) return 1;
	}

	Term_xtra(TERM_XTRA_DELAY, 1);

	return 1;
}


int Receive_line_info(void)
{
	char	ch, c, n;
	int	x, i;
	s16b	y;
	byte	a;
	bool	draw = FALSE;

	if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &y)) <= 0)
	{
		return n;
	}

	/* If this is the mini-map then we can draw if the screen is icky */
	if (ch == PKT_MINI_MAP || (!screen_icky && !shopping))
		draw = TRUE;
	
	/* Check the max line count */
	if (y > last_line_info)
		last_line_info = y;

	for (x = 0; x < 80; x++)
	{
		/* Read the char/attr pair */
		Packet_scanf(&rbuf, "%c%c", &c, &a);

		/* Check for bit 0x40 on the attribute */
		if (a & 0x40)
		{
			/* First, clear the bit */
			a &= ~(0x40);

			/* Read the number of repetitions */
			Packet_scanf(&rbuf, "%c", &n);
		}
		else
		{
			/* No RLE, just one instance */
			n = 1;
		}

		/* Draw a character n times */
		for (i = 0; i < n; i++)
		{
			/* Don't draw anything if "char" is zero */
			if (c && draw)
				Term_draw(x + i, y, a, c);
		}

		/* Reset 'x' to the correct value */
		x += n - 1;

		/* hack -- if x > 80, assume we have received corrupted data,
		 * flush our buffers 
		 */
		if (x > 80) 
		{
			Sockbuf_clear(&rbuf);
			Sockbuf_clear(&cbuf);
		}

	}

	/* Request a redraw if the screen was icky */
	if (screen_icky)
		request_redraw = TRUE;

	return 1;
}

/*
 * Note that this function does not honor the "screen_icky"
 * flag, as it is only used for displaying the mini-map,
 * and the screen should be icky during that time.
 */
int Receive_mini_map(void)
{
	char	ch, c;
	int	n, x;
	s16b	y;
	byte	a;

	if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &y)) <= 0)
	{
		return n;
	}

	/* Check the max line count */
	if (y > last_line_info)
		last_line_info = y;

	for (x = 0; x < 80; x++)
	{
		Packet_scanf(&rbuf, "%c%c", &c, &a);

		/* Don't draw anything if "char" is zero */
		/* Only draw if the screen is "icky" */
		if (c && screen_icky && x < 80 - 12)
			Term_draw(x + 12, y, a, c);
	}

	return 1;
}

int Receive_special_other(void)
{
	int	n;
	char	ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0)
	{
		return n;
	}

	/* Set file perusal method to "other" */
	special_line_type = SPECIAL_FILE_OTHER;

	/* Peruse the file we're about to get */
	peruse_file();

	return 1;
}

int Receive_store(void)
{
	int	n, price;
	char	ch, pos, name[1024], tval, sval;
	byte	attr;
	s16b	wgt, num;

	if ((n = Packet_scanf(&rbuf, "%c%c%c%hd%hd%d%s%c%c", &ch, &pos, &attr, &wgt, &num, &price, name, &tval, &sval)) <= 0)
	{
		return n;
	}

	store.stock[(int)pos].sval = attr;
	store.stock[(int)pos].weight = wgt;
	store.stock[(int)pos].number = num;
	store_prices[(int) pos] = price;
	strncpy(store_names[(int) pos], name, 80);
	store.stock[(int)pos].tval = tval;
	store.stock[(int)pos].xtra1 = sval;	/* GRRRR.. I *REALLY* hate it */

	/* Make sure that we're in a store */
	if (shopping)
	{
		display_inventory();
	}

	return 1;
}

int Receive_store_info(void)
{
	int	n;
	char	ch;
	s16b	owner_num, num_items;

	if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd", &ch, &store_num, &owner_num, &num_items)) <= 0)
	{
		return n;
	}

	store.stock_num = num_items;
	store_owner = owners[store_num][owner_num];

	/* Only enter "display_store" if we're not already shopping */
	if (!shopping)
		display_store();

	return 1;
}

int Receive_store_kick(void)
{
	int	n;
	char	ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0)
	{
		return n;
	}

	shopping = FALSE;
	leave_store = TRUE;

	/* Hack */
	command_cmd = ESCAPE;

	return 1;
}

int Receive_sell(void)
{
	int	n, price;
	char	ch, buf[1024];

	if ((n = Packet_scanf(&rbuf, "%c%d", &ch, &price)) <= 0)
	{
		return n;
	}

	/* Tell the user about the price */
	sprintf(buf, "Accept %d gold? ", price);

	if (get_check(buf))
		Send_store_confirm();

	return 1;
}

int Receive_target_info(void)
{
	int	n;
	char	ch, x, y, buf[80];

	if ((n = Packet_scanf(&rbuf, "%c%c%c%s", &ch, &x, &y, buf)) <= 0)
	{
		return n;
	}

	/* Print the message */
	prt(buf, 0, 0);

	/* Move the cursor */
	Term_gotoxy(x, y);

	return 1;
}

int Receive_sound(void)
{
	int	n;
	char	ch, sound;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &sound)) <= 0)
	{
		return n;
	}

	/* Make a sound (if allowed) */
	if (use_sound) Term_xtra(TERM_XTRA_SOUND, sound);

	return 1;
}

int Receive_special_line(void)
{
	int	n;
	char	ch, attr;
	s16b	max, line;
	char	buf[80];

	if ((n = Packet_scanf(&rbuf, "%c%hd%hd%c%s", &ch, &max, &line, &attr, buf)) <= 0)
	{
		return n;
	}

	/* Maximum */
	max_line = max;

	/* Print out the info */
	if(special_line_type)	/* If we have quit already, dont */
		c_put_str(attr, buf, line + 2, 0);

	return 1;
}

int Receive_floor(void)
{
	int	n;
	char	ch, tval;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &tval)) <= 0)
	{
		return n;
	}

	/* Ignore for now */
	tval = tval;

	return 1;
}

int Receive_pickup_check(void)
{
	int	n;
	char	ch, buf[180];

	if ((n = Packet_scanf(&rbuf, "%c%s", &ch, buf)) <= 0)
	{
		return n;
	}

	/* Get a check */
	if (get_check(buf))
	{
		/* Pick it up */
		Send_stay();
	}

	return 1;
}


int Receive_party(void)
{
	int n;
	char ch, buf[160];

	if ((n = Packet_scanf(&rbuf, "%c%s", &ch, buf)) <= 0)
	{
		return n;
	}

	/* Copy info */
	strcpy(party_info, buf);

	/* Re-show party info */
	if (party_mode)
	{
		Term_erase(0, 16, 255);
		Term_putstr(0, 16, -1, TERM_WHITE, party_info);
		Term_putstr(0, 15, -1, TERM_WHITE, "Command: ");
	}

	return 1;
}

int Receive_skills(void)
{
	int	n, i;
	s16b tmp[12];
	char	ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0)
	{
		return n;
	}

	/* Read into skills info */
	for (i = 0; i < 12; i++)
	{
		if ((n = Packet_scanf(&rbuf, "%hd", &tmp[i])) <= 0)
		{
			return n;
		}
	}

	/* Store */
	p_ptr->skill_thn = tmp[0];
	p_ptr->skill_thb = tmp[1];
	p_ptr->skill_sav = tmp[2];
	p_ptr->skill_stl = tmp[3];
	p_ptr->skill_fos = tmp[4];
	p_ptr->skill_srh = tmp[5];
	p_ptr->skill_dis = tmp[6];
	p_ptr->skill_dev = tmp[7];
	p_ptr->num_blow = tmp[8];
	p_ptr->num_fire = tmp[9];
	p_ptr->num_spell = tmp[10];
	p_ptr->see_infra = tmp[11];

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_pause(void)
{
	int n;
	char ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0)
	{
		return n;
	}

	/* Show the most recent changes to the screen */
	Term_fresh();

	/* Flush any pending keystrokes */
	Term_flush();

	/* Wait */
	inkey();

	/* Flush queue */
	Flush_queue();

	return 1;
}


int Receive_monster_health(void)
{
	int n;
	char ch, num;
	byte attr;

	if ((n = Packet_scanf(&rbuf, "%c%c%c", &ch, &num, &attr)) <= 0)
	{
		return n;
	}

	if (!screen_icky)
	{
		/* Draw the health bar */
		health_redraw(num, attr);
	}
	else
	{
		if ((n = Packet_printf(&qbuf, "%c%c%c", ch, num, attr)) <= 0)
		{
			return n;
		}
	}

	return 1;
}

int Send_search(void)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_SEARCH)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_walk(int dir)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_WALK, dir)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_run(int dir)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_RUN, dir)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_drop(int item, int amt)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_DROP, item, amt)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_drop_gold(s32b amt)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%ld", PKT_DROP_GOLD, amt)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_tunnel(int dir)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_TUNNEL, dir)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_stay(void)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_STAND)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_keepalive(void)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_KEEPALIVE)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_toggle_search(void)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_SEARCH_MODE)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_rest(void)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_REST)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_go_up(void)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_GO_UP)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_go_down(void)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_GO_DOWN)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_open(int dir)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_OPEN, dir)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_close(int dir)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_CLOSE, dir)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_bash(int dir)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_BASH, dir)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_disarm(int dir)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_DISARM, dir)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_wield(int item)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_WIELD, item)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_observe(int item)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_OBSERVE, item)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_take_off(int item)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_TAKE_OFF, item)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_destroy(int item, int amt)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_DESTROY, item, amt)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_inscribe(int item, cptr buf)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd%s", PKT_INSCRIBE, item, buf)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_uninscribe(int item)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_UNINSCRIBE, item)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_steal(int dir)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_STEAL, dir)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_quaff(int item)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_QUAFF, item)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_read(int item)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_READ, item)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_aim(int item, int dir)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd%c", PKT_AIM_WAND, item, dir)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_use(int item)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_USE, item)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_zap(int item)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_ZAP, item)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_fill(int item)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_FILL, item)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_eat(int item)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_EAT, item)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_activate(int item)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_ACTIVATE, item)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_target(int dir)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_TARGET, dir)) <= 0)
	{
		return n;
	}

	return 1;
}


int Send_target_friendly(int dir)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_TARGET_FRIENDLY, dir)) <= 0)
	{
		return n;
	}

	return 1;
}


int Send_look(int dir)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_LOOK, dir)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_msg(cptr message)
{
	int	n;

	if (message && strlen(message))
	{
		if (strlen(talk_pend))
			strcat(talk_pend, "\t");
		strcat(talk_pend, message);
	}

	//talk_resend = last_turns + 36;

	if (!strlen(talk_pend)) return 1;

	if ((n = Packet_printf(&wbuf, "%c%S", PKT_MESSAGE, talk_pend)) <= 0)
	{
		return n;
	}
	talk_pend[0]='\0';

	return 1;
}

//int Send_fire(int item, int dir)
int Send_fire(int dir)
{
	int	n;

//	if ((n = Packet_printf(&wbuf, "%c%c%hd", PKT_FIRE, dir, item)) <= 0)
	if ((n = Packet_printf(&wbuf, "%c%c", PKT_FIRE, dir)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_throw(int item, int dir)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%c%hd", PKT_THROW, dir, item)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_item(int item)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_ITEM, item)) <= 0)
	{
		return n;
	}

	return 1;
}

#if 0
int Send_gain(int book, int spell)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_GAIN, book, spell)) <= 0)
	{
		return n;
	}

	return 1;
}
#endif	// 0

int Send_activate_skill(int mkey, int book, int spell, int dir)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%c%hd%hd%c", PKT_ACTIVATE_SKILL,
					mkey, book, spell, dir)) <= 0)
	{
		return n;
	}

	return 1;
}

#if 0
int Send_cast(int book, int spell)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_SPELL, book, spell)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_pray(int book, int spell)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_PRAY, book, spell)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_mimic(int spell)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_MIMIC, spell)) <= 0)
	{
		return n;
	}

	return 1;
}
#endif	// 0

int Send_mind()
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_MIND)) <= 0)
	{
		return n;
	}

	return 1;
}

#if 0
int Send_fight(int book, int spell)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_FIGHT, book, spell)) <= 0)
	{
		return n;
	}

	return 1;
}
#endif	// 0

int Send_ghost(int ability)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_GHOST, ability)) <= 0)
	{
		return n;
	}
	
	return 1;
}

int Send_map(void)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_MAP)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_locate(int dir)
{
	int n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_LOCATE, dir)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_store_examine(int item)
{
	int 	n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_STORE_EXAMINE, item)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_store_purchase(int item, int amt)
{
	int 	n;

	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_PURCHASE, item, amt)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_store_sell(int item, int amt)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_SELL, item, amt)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_store_leave(void)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_STORE_LEAVE)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_store_confirm(void)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_STORE_CONFIRM)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_redraw(void)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_REDRAW)) <= 0)
	{
		return n;
	}
	
	/* Hack -- Clear the screen */
//	Term_clear();

	return 1;
}

int Send_clear_buffer(void)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_CLEAR_BUFFER)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_special_line(int type, int line)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%c%hd", PKT_SPECIAL_LINE, type, line)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_skill_mod(int i)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%ld", PKT_SKILL_MOD, i)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_party(s16b command, cptr buf)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd%s", PKT_PARTY, command, buf)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_guild(s16b command, cptr buf){
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd%s", PKT_GUILD, command, buf)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_purchase_house(int dir)
{
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_PURCHASE, dir, 0)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_suicide(void)
{
	int n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_SUICIDE)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_options(void)
{
	int i, n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_OPTIONS)) <= 0)
	{
		return n;
	}

	/* Send each option */
	for (i = 0; i < OPT_MAX; i++)
	{
		Packet_printf(&wbuf, "%c", Client_setup.options[i]);
	}

	return 1;
}

int Send_admin_house(int dir, cptr buf){
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd%s", PKT_HOUSE, dir, buf)) <= 0)
		return n;
	return 1;
}

int Send_master(s16b command, cptr buf)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%hd%s", PKT_MASTER, command, buf)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_King(byte type)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_KING, type)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_spike(int dir)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_SPIKE, dir)) <= 0)
	{
		return n;
	}

	return 1;
}

int Send_raw_key(int key)
{
	int	n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_RAW_KEY, key)) <= 0)
	{
		return n;
	}

	return 1;
}

#if defined(WINDOWS) && !defined(CYGWIN)
int gettimeofday(struct timeval *timenow)
{
	time_t t;

	t = clock();

	timenow->tv_usec = t % 1000;
	timenow->tv_sec = t / CLK_TCK;

	return 0;
}
#endif

// Update the current time, which is stored in 100 ms "ticks".
// I hope that Windows systems have gettimeofday on them by default.
// If not there should hopefully be some simmilar efficient call with the same
// functionality. 
// I hope this doesn't prove to be a bottleneck on some systems.  On my linux system
// calling gettimeofday seems to be very very fast.
void update_ticks()
{
	struct timeval cur_time;
	int newticks;

#ifdef AMIGA
	GetSysTime(&cur_time);
#else
#if defined(WINDOWS) && !defined(CYGWIN)
	gettimeofday(&cur_time);
#else
	gettimeofday(&cur_time, NULL);
#endif
#endif

	// Set the new ticks to the old ticks rounded down to the number of seconds.
	newticks = ticks-(ticks%10);
	// Find the new least significant digit of the ticks
#ifdef AMIGA
	newticks += cur_time.tv_micro / 100000;
#else
#if defined(WINDOWS) && !defined(CYGWIN)
	newticks += cur_time.tv_usec / 100;
#else
	newticks += cur_time.tv_usec / 100000;
#endif
#endif

	// Assume that it has not been more than one second since this function was last called
	if (newticks < ticks) newticks += 10;
	ticks = newticks;	
}

/* Write a keepalive packet to the output queue if it has been two seconds
 * since we last sent anything.  
 * Note that if the loop that is calling this function doesn't flush the
 * network output before calling this function again very bad things could
 * happen, such as an overflow of our send queue.
 */
void do_keepalive()
{
	// Check to see if it has been 2 seconds since we last sent anything.  Assume
	// that each game turn lasts 100 ms.
	if ((ticks - last_send_anything) >= 20)
	{
		Send_keepalive();
	}
}

void do_mail(){
#if 0
#ifdef SET_UID
	static int mailticks=0;
	static struct timespec lm;
	char mpath[160],buffer[160];

	int uid;
	struct passwd *pw;

#ifdef NETBSD
	strcpy(mpath,"/var/mail/");
#else
	strcpy(mpath,"/var/spool/mail/");
#endif

	uid=getuid();
	pw=getpwuid(uid);
	if(pw==(struct passwd*)NULL)
		return;
	strcat(mpath,pw->pw_name);

	if(ticks-mailticks >= 300){	/* testing - too fast */
		struct stat inf;
		if(!stat(mpath,&inf)){
#ifndef _POSIX_C_SOURCE
			if(inf.st_size!=0){
				if(inf.st_mtimespec.tv_sec>lm.tv_sec || (inf.st_mtimespec.tv_sec==lm.tv_sec && inf.st_mtimespec.tv_nsec>lm.tv_nsec)){
					lm.tv_sec=inf.st_mtimespec.tv_sec;
					lm.tv_nsec=inf.st_mtimespec.tv_nsec;
					sprintf(buffer,"\377yYou have new mail in %s.", mpath);
					c_msg_print(buffer);
				}
			}
#else
			if(inf.st_size!=0){
				if(inf.st_mtime>lm.tv_sec || (inf.st_mtime==lm.tv_sec && inf.st_mtimensec>lm.tv_nsec)){
					lm.tv_sec=inf.st_mtime;
					lm.tv_nsec=inf.st_mtimensec;
					sprintf(buffer,"\377yYou have new mail in %s.", mpath);
					c_msg_print(buffer);
				}
			}
#endif
		}
		mailticks=ticks;
	}
#endif
#endif /* 0 */
}
