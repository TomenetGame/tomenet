/* $Id$ */
/* The server side of the network stuff */

/* The following is a direct excerpt from the netserver.c
 * in the XPilot distribution.  Much of it is incorrect
 * in MAngband's case, but most of it is still correct.
 */



/*
 * This is the server side of the network connnection stuff.
 *
 * We try very hard to not let the game be disturbed by
 * players logging in.  Therefore a new connection
 * passes through several states before it is actively
 * playing.
 * First we make a new connection structure available
 * with a new socket to listen on.  This socket port
 * number is told to the client via the pack mechanism.
 * In this state the client has to send a packet to this
 * newly created socket with its name and playing parameters.
 * If this succeeds the connection advances to its second state.
 * In this second state the essential server configuration
 * like the map and so on is transmitted to the client.
 * If the client has acknowledged all this data then it
 * advances to the third state, which is the
 * ready-but-not-playing-yet state.  In this state the client
 * has some time to do its final initializations, like mapping
 * its user interface windows and so on.
 * When the client is ready to accept frame updates and process
 * keyboard events then it sends the start-play packet.
 * This play packet advances the connection state into the
 * actively-playing state.  A player structure is allocated and
 * initialized and the other human players are told about this new player.
 * The newly started client is told about the already playing players and
 * play has begun.
 * Apart from these four states there are also two intermediate states.
 * These intermediate states are entered when the previous state
 * has filled the reliable data buffer and the client has not
 * acknowledged all the data yet that is in this reliable data buffer.
 * They are so called output drain states.  Not doing anything else
 * then waiting until the buffer is empty.
 * The difference between these two intermediate states is tricky.
 * The second intermediate state is entered after the
 * ready-but-not-playing-yet state and before the actively-playing state.
 * The difference being that in this second intermediate state the client
 * is already considered an active player by the rest of the server
 * but should not get frame updates yet until it has acknowledged its last
 * reliable data.
 *
 * Communication between the server and the clients is only done
 * using UDP datagrams.  The first client/serverized version of XPilot
 * was using TCP only, but this was too unplayable across the Internet,
 * because TCP is a data stream always sending the next byte.
 * If a packet gets lost then the server has to wait for a
 * timeout before a retransmission can occur.  This is too slow
 * for a real-time program like this game, which is more interested
 * in recent events than in sequenced/reliable events.
 * Therefore UDP is now used which gives more network control to the
 * program.
 * Because some data is considered crucial, like the names of
 * new players and so on, there also had to be a mechanism which
 * enabled reliable data transmission.  Here this is done by creating
 * a data stream which is piggybacked on top of the unreliable data
 * packets.  The client acknowledges this reliable data by sending
 * its byte position in the reliable data stream.  So if the client gets
 * a new reliable data packet and it has not had this data before and
 * there is also no data packet missing inbetween, then it advances
 * its byte position and acknowledges this new position to the server.
 * Otherwise it discards the packet and sends its old byte position
 * to the server meaning that it detected a packet loss.
 * The server maintains an acknowledgement timeout timer for each
 * connection so that it can retransmit a reliable data packet
 * if the acknowledgement timer expires.
 */



#define SERVER

#include "angband.h"
#include "netserver.h"

#ifdef WINDOWS
# define EWOULDBLOCK WSAEWOULDBLOCK
#endif

#define MAX_SELECT_FD			1023
/* #define MAX_RELIABLE_DATA_PACKET_SIZE	1024 */
#define MAX_RELIABLE_DATA_PACKET_SIZE	512

#define MAX_MOTD_CHUNK			512
#define MAX_MOTD_SIZE			(30*1024)
#define MAX_MOTD_LOOPS			120


connection_t	*Conn = NULL;
static int		max_connections = 0;
static setup_t		Setup;
static int		(*playing_receive[256])(int ind),
			(*login_receive[256])(int ind),
			(*drain_receive[256])(int ind);
int			login_in_progress;
static int		num_logins, num_logouts;
static long		Id;
int			NumPlayers;

int		MetaSocket = -1;

#ifdef NEW_SERVER_CONSOLE
int		ConsoleSocket = -1;
#endif
#ifdef SERVER_GWPORT
int		SGWSocket = -1;
#endif
#ifdef TOMENET_WORLDS
int		WorldSocket = -1;
#endif


char *showtime(void)
{
	time_t		now;
	struct tm	*tmp;
	static char	month_names[13][4] = {
				"Jan", "Feb", "Mar", "Apr", "May", "Jun",
				"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
				"Bug"
			};
	static char	buf[80];

	time(&now);
	tmp = localtime(&now);
	sprintf(buf, "%02d %s %02d:%02d:%02d",
		tmp->tm_mday, month_names[tmp->tm_mon],
		tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
	return buf;
}

void add_banlist(int Ind, int time){
	struct ip_ban *ptr;
	if(!time) return;

	ptr=malloc(sizeof(struct ip_ban));
	if(ptr==(struct ip_ban*)NULL) return; /* unimportant failure */

	ptr->next=banlist;
	ptr->time=time;
	strcpy(ptr->ip, Conn[Players[Ind]->conn].addr);
	s_printf("Banned connections from %s for %d minutes\n", ptr->ip, time);
	banlist=ptr;
}

/*
 * Initialize the function dispatch tables for the various client
 * connection states.  Some states use the same table.
 */
static void Init_receive(void)
{
	int i;

	for (i = 0; i < 256; i++)
	{
		login_receive[i] = Receive_undefined;
		playing_receive[i] = Receive_undefined;
		drain_receive[i] = Receive_undefined;
	}

	drain_receive[PKT_QUIT]			= Receive_quit;
	drain_receive[PKT_ACK]			= Receive_ack;
	drain_receive[PKT_VERIFY]		= Receive_discard;
	drain_receive[PKT_PLAY]			= Receive_discard;

	login_receive[PKT_PLAY]			= Receive_play;
	login_receive[PKT_QUIT]			= Receive_quit;
	login_receive[PKT_ACK]			= Receive_ack;
	login_receive[PKT_VERIFY]		= Receive_discard;
	login_receive[PKT_LOGIN]		= Receive_login;
	
	playing_receive[PKT_ACK]		= Receive_ack;
	playing_receive[PKT_VERIFY]		= Receive_discard;
	playing_receive[PKT_QUIT]		= Receive_quit;
	playing_receive[PKT_PLAY]		= Receive_play;
	playing_receive[PKT_FILE]		= Receive_file;

	playing_receive[PKT_KEEPALIVE]		= Receive_keepalive;
	playing_receive[PKT_WALK]		= Receive_walk;
	playing_receive[PKT_RUN]		= Receive_run;
	playing_receive[PKT_TUNNEL]		= Receive_tunnel;
	playing_receive[PKT_AIM_WAND]		= Receive_aim_wand;
	playing_receive[PKT_DROP]		= Receive_drop;
	playing_receive[PKT_FIRE]		= Receive_fire;
	playing_receive[PKT_STAND]		= Receive_stand;
	playing_receive[PKT_DESTROY]		= Receive_destroy;
	playing_receive[PKT_LOOK]		= Receive_look;

	playing_receive[PKT_OPEN]		= Receive_open;
	playing_receive[PKT_QUAFF]		= Receive_quaff;
	playing_receive[PKT_READ]		= Receive_read;
	playing_receive[PKT_SEARCH]		= Receive_search;
	playing_receive[PKT_TAKE_OFF]		= Receive_take_off;
	playing_receive[PKT_USE]		= Receive_use;
	playing_receive[PKT_THROW]		= Receive_throw;
	playing_receive[PKT_WIELD]		= Receive_wield;
	playing_receive[PKT_OBSERVE]		= Receive_observe;
	playing_receive[PKT_ZAP]		= Receive_zap;
	playing_receive[PKT_MIND]		= Receive_mind;

	playing_receive[PKT_TARGET]		= Receive_target;
	playing_receive[PKT_TARGET_FRIENDLY]	= Receive_target_friendly;
	playing_receive[PKT_INSCRIBE]		= Receive_inscribe;
	playing_receive[PKT_UNINSCRIBE]		= Receive_uninscribe;
	playing_receive[PKT_ACTIVATE]		= Receive_activate;
	playing_receive[PKT_BASH]		= Receive_bash;
	playing_receive[PKT_DISARM]		= Receive_disarm;
	playing_receive[PKT_EAT]		= Receive_eat;
	playing_receive[PKT_FILL]		= Receive_fill;
	playing_receive[PKT_LOCATE]		= Receive_locate;
	playing_receive[PKT_MAP]		= Receive_map;
	playing_receive[PKT_SEARCH_MODE]	= Receive_search_mode;

	playing_receive[PKT_CLOSE]		= Receive_close;
//	playing_receive[PKT_GAIN]		= Receive_gain;
	playing_receive[PKT_DIRECTION]		= Receive_direction;
	playing_receive[PKT_GO_UP]		= Receive_go_up;
	playing_receive[PKT_GO_DOWN]		= Receive_go_down;
	playing_receive[PKT_MESSAGE]		= Receive_message;
	playing_receive[PKT_ITEM]		= Receive_item;
	playing_receive[PKT_PURCHASE]		= Receive_purchase;
	playing_receive[PKT_KING]		= Receive_King;

	playing_receive[PKT_SELL]		= Receive_sell;
	playing_receive[PKT_STORE_LEAVE]	= Receive_store_leave;
	playing_receive[PKT_STORE_CONFIRM]	= Receive_store_confirm;
	playing_receive[PKT_DROP_GOLD]		= Receive_drop_gold;
	playing_receive[PKT_REDRAW]		= Receive_redraw;
	playing_receive[PKT_REST]		= Receive_rest;
	playing_receive[PKT_SPECIAL_LINE]	= Receive_special_line;
	playing_receive[PKT_PARTY]		= Receive_party;
	playing_receive[PKT_GHOST]		= Receive_ghost;

	playing_receive[PKT_STEAL]		= Receive_steal;
	playing_receive[PKT_OPTIONS]		= Receive_options;
	playing_receive[PKT_SUICIDE]		= Receive_suicide;
	playing_receive[PKT_MASTER]		= Receive_master;
	playing_receive[PKT_HOUSE]		= Receive_admin_house;

	playing_receive[PKT_AUTOPHASE]		= Receive_autophase;

	playing_receive[PKT_CLEAR_BUFFER]	= Receive_clear_buffer;

	playing_receive[PKT_SPIKE]		= Receive_spike;
	playing_receive[PKT_GUILD]		= Receive_guild;

        playing_receive[PKT_SKILL_MOD]		= Receive_skill_mod;
        playing_receive[PKT_ACTIVATE_SKILL]		= Receive_activate_skill;
	playing_receive[PKT_RAW_KEY]		= Receive_raw_key;
	playing_receive[PKT_STORE_EXAMINE]		= Receive_store_examine;
	playing_receive[PKT_STORE_CMD]		= Receive_store_command;
}

static int Init_setup(void)
{
	int n = 0, i;
	char buf[1024];
	FILE *fp;

	Setup.frames_per_second = cfg.fps;
	Setup.max_race = MAX_RACES;
	Setup.max_class = MAX_CLASS;
	Setup.motd_len = 23 * 80;
	Setup.setup_size = sizeof(setup_t);

	path_build(buf, 1024, ANGBAND_DIR_TEXT, "news.txt");

	/* Open the news file */
	fp = my_fopen(buf, "r");

	if (fp)
	{
		/* Dump the file into the buffer */
		while (0 == my_fgets(fp, buf, 1024) && n < 23)
		{
			strncpy(&Setup.motd[n * 80], buf, 80);
			n++;
		}

		my_fclose(fp);
	}
	
	/* MEGAHACK -- copy race/class names */
	/* XXX I know this ruins the meaning of Setup... sry	- Jir - */
	for (i = 0; i < MAX_RACES; i++)
	{
		if (!race_info[i].title)
		{
			Setup.max_race = i;
			break;
		}
//		strncpy(&Setup.race_title[i], race_info[i].title, 12);
//		Setup.race_choice[i] = race_info[i].choice;
		/* 1 for '\0', 4 for race_choice */
		Setup.setup_size += strlen(race_info[i].title) + 1 + 4;
	}

	for (i = 0; i < MAX_CLASS; i++)
	{
		if (!class_info[i].title)
		{
			Setup.max_class = i;
			break;
		}
//		strncpy(&Setup.class_title[i], class_info[i].title, 12);
		Setup.setup_size += strlen(class_info[i].title) + 1;
	}

	return 0;
}
	
void init_players(){
	max_connections = MAX_SELECT_FD - 5;
	/* Last player is the DM Edit player ! */
	/* As no extra connection is required, */
	/* we need only allocate the player_type for it */
	C_MAKE(Players, max_connections+1, player_type *);
}


/*
 * Talk to the metaserver.
 *
 * This function is called on startup, on death, and when the number of players
 * in the game changes.
 */
bool Report_to_meta(int flag)
{
	static sockbuf_t meta_buf;
	static char local_name[1024];
	static int init = 0;
	int bytes, i;
	char buf[1024], temp[100];
	bool hidden_dungeon_master = 0;

	/* Abort if the user doesn't want to report */
	if (!cfg.report_to_meta || cfg.runlevel<4 || cfg.runlevel > 1023)
		return FALSE;

	/* If this is the first time called, initialize our hostname */
	if (!init)
	{
		/* Never do this again */
		init = 1;

		/* Get our hostname */
#if 0
#ifdef BIND_NAME
			strncpy(local_name, BIND_NAME, 1024);
#else
			GetLocalHostName(local_name, 1024);
#endif
#else
                        if (cfg.bind_name)
                                strncpy(local_name, cfg.bind_name, 1024);
                        else
                                GetLocalHostName(local_name, 1024);
#endif
        }

	strcpy(buf, local_name);

	if (flag & META_START)
	{
		if ((MetaSocket = CreateDgramSocket(0)) == -1)
		{
			quit("Couldn't create meta-server Dgram socket\n");
		}

		if (SetSocketNonBlocking(MetaSocket, 1) == -1)
		{
			quit("Can't make socket non-blocking\n");
		}

		if (Sockbuf_init(&meta_buf, MetaSocket, SERVER_SEND_SIZE,
			SOCKBUF_READ | SOCKBUF_WRITE | SOCKBUF_DGRAM) == -1)
		{
			quit("No memory for sockbuf buffer\n");
		}

		Sockbuf_clear(&meta_buf);

		strcat(buf, " Number of players: 0  ");
	}

	else if (flag & META_DIE)
	{
		strcat(buf, " ");
	}

	else if (flag & META_UPDATE)
	{
		strcat(buf, " Number of players: ");

		/* Hack -- If cfg_secret_dungeon_master is enabled, determine
		 * if the DungeonMaster is playing, and if so, reduce the
		 * number of players reported.
		 */

		for (i = 1; i <= NumPlayers; i++)
		{
			if (Players[i]->admin_dm && cfg.secret_dungeon_master) hidden_dungeon_master = TRUE;
		}

		/* tell the metaserver about everyone except hidden dungeon_masters */
		sprintf(temp, "%d ", NumPlayers - hidden_dungeon_master);
		strcat(buf, temp);

		/* if someone other than a dungeon master is playing */
		if (NumPlayers - hidden_dungeon_master)
		{
			strcat(buf, "Names: ");
			for (i = 1; i <= NumPlayers; i++)
			{
				/* handle the cfg_secret_dungeon_master option */
			if (Players[i]->admin_dm && cfg.secret_dungeon_master) continue;
				strcat(buf, Players[i]->basename);
				strcat(buf, " ");
			}
		}
	}

	/* Append the version number */
	sprintf(temp, "Version: %d.%d.%d ", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

	/* Append the additional version info */
	if (VERSION_EXTRA == 1)
		strcat(temp, "alpha");
	if (VERSION_EXTRA == 2)
		strcat(temp, "beta");
	if (VERSION_EXTRA == 3)
		strcat(temp, "development");

	if (!(flag & META_DIE))
		strcat(buf, temp);

	/* If we haven't setup the meta connection yet, abort */
	if (MetaSocket == -1)
		return FALSE;

	Sockbuf_clear(&meta_buf);

	Packet_printf(&meta_buf, "%S", buf);

	if ((bytes = DgramSend(MetaSocket, cfg.meta_address, 8800, meta_buf.buf, meta_buf.len) == -1))
	{
		s_printf("Couldn't send info to meta-server!\n");
		return FALSE;
	}

	return TRUE;
}

	 
/*
 * Initialize the connection structures.
 */
int Setup_net_server(void)
{
	size_t size;

	Init_receive();

	if (Init_setup() == -1)
		return -1;

	/*
	 * The number of connections is limited by the number of bases
	 * and the max number of possible file descriptors to use in
	 * the select(2) call minus those for stdin, stdout, stderr,
	 * the contact socket, and the socket for the resolver library routines.
	 */

	size = max_connections * sizeof(*Conn);
	if ((Conn = (connection_t *) malloc(size)) == NULL)
		quit("Cannot allocate memory for connections");

	memset(Conn, 0, size);

	/* Tell the metaserver that we're starting up */
	Report_to_meta(META_START);

	s_printf("%s\n", longVersion);
	s_printf("Server is running version %04x\n", MY_VERSION);
	time(&cfg.runtime);

	return 0;
}

/* The contact socket */
static int Socket;
static sockbuf_t ibuf;

/* The contact socket now uses TCP.  This breaks backwards
 * compatibility, but is a good thing.
 */

void setup_contact_socket(void)
{
	plog("Create TCP socket..."); 
	while ((Socket = CreateServerSocket(cfg.game_port)) == -1)
	{
		sleep(1);
	}
	plog("Set Non-Blocking..."); 
	if (SetSocketNonBlocking(Socket, 1) == -1)
	{
		plog("Can't make contact socket non-blocking");
	}
	if (SetSocketNoDelay(Socket, 1) == -1)
	{
		plog("Can't set TCP_NODELAY on the socket");
	}
	if (SocketLinger(Socket) == -1)
	{
		plog("Couldn't set SO_LINGER on the socket");
	}

	if (Sockbuf_init(&ibuf, Socket, SERVER_SEND_SIZE,
		SOCKBUF_READ | SOCKBUF_WRITE ) == -1)
	{
		quit("No memory for contact buffer");
	}

	install_input(Contact, Socket, 0);
	
#ifdef SERVER_CONSOLE
	/* Hack -- Install stdin an the "console" input */
	install_input(Console, 0, 0);
#endif

#ifdef NEW_SERVER_CONSOLE
	if ((ConsoleSocket = CreateServerSocket(cfg.console_port)) == -1)
	{
		s_printf("Couldn't create console socket\n");
		return;
	}
	if (SocketLinger(ConsoleSocket) == -1)
	{
		plog("Couldn't set SO_LINGER on the console socket");
	}

	if (!InitNewConsole(ConsoleSocket))
	{
		return;
	}

	/* Install the new console socket */
	install_input(NewConsole, ConsoleSocket, 0);
#endif
#ifdef SERVER_GWPORT
	/* evileye testing only */
	if ((SGWSocket = CreateServerSocket(18400)) == -1)
	{
		s_printf("Couldn't create server gateway port\n");
		return;
	}

	/* Install the new gateway socket */
	install_input(SGWHit, SGWSocket, 0);
#endif
#ifdef TOMENET_WORLDS
	/* evileye testing only */
	/* really, server should DIE if this happens */
	block_timer();
	if((WorldSocket=CreateClientSocket(cfg.wserver, 18360))==-1){
		s_printf("Unable to connect to world server\n");
		return;
	}
	allow_timer();
	install_input(world_comm, WorldSocket, 0);
#endif
}

static int Reply(char *host_addr, int fd)
{
	int result;

	// No silly redundancy with TCP
	if ((result = DgramWrite(fd, ibuf.buf, ibuf.len)) == -1)
	{
		GetSocketError(ibuf.sock);
	}	

	return result;
}


/* invite only */
bool player_allowed(char *name){
	FILE *sfp;
	char buffer[80];
	bool success=FALSE;
	/* Hack -- allow 'guest' account */
	/* if (!strcmp("Guest", name)) return TRUE; */

	sfp=fopen("allowlist","r");
	if(sfp==(FILE*)NULL)
		return TRUE;
	else{
		while(fgets(buffer, 80, sfp)){
			/* allow for \n */
			if((strlen(name)+1)!=strlen(buffer)) continue;
			if(!strncmp(buffer,name, strlen(name))){
				success=TRUE;
				break;
			}
		}
		fclose(sfp);
	}
	return(success);
}

static int Check_names(char *nick_name, char *real_name, char *host_name, char *addr)
{
	player_type *p_ptr = NULL;
	char *ptr;
	int i;

	if (real_name[0] == 0 || host_name[0] == 0 || nick_name[0] < 'A' ||
		nick_name[0] > 'Z')
		return E_INVAL;

	for (ptr = &nick_name[strlen(nick_name)]; ptr-- > nick_name; )
	{
		if (isascii(*ptr) && isspace(*ptr))
			*ptr = '\0';
		else break;
	}

	for (i = 1; i < NumPlayers + 1; i++)
	{
		if(Players[i]->conn != NOT_CONNECTED ) {
		    p_ptr = Players[i];
			/*
			 * FIXME: p_ptr->name is character name while nick_name is
			 * account name, so this check always fail.  Evileye? :)
			 */
		    if (strcasecmp(p_ptr->name, nick_name) == 0)
		    {
			/*plog(format("%s %s", Players[i]->name, nick_name));*/

			/* The following code allows you to "override" an
			 * existing connection by connecting again  -Crimson */

			/* XXX Hack -- since the password is not read until later, to
			 * authorize the "hijacking" of an existing connection,
			 * we check to see if the username and hostname are
			 * identical.  Note that it may be possobile to spoof this,
			 * kicking someone off.  This is a quick hack that should 
			 * be replaced with proper password checking. 
			 */
			/* XXX another Hack -- don't allow to resume connection if
			 * in 'character edit' mode		- Jir -
			 */

			/* resume connection at this point is not compatible
			   with multicharacter accounts */
			if ((!strcasecmp(p_ptr->realname, real_name)) &&
					(!strcasecmp(p_ptr->addr, addr)) && (cfg.runlevel != 1024)){
				printf("%s %s\n", p_ptr->realname, p_ptr->addr);
				Destroy_connection(p_ptr->conn, "resume connection");
			}
			else return E_IN_USE;
		    }
		
		    /* All restrictions on the number of allowed players from one IP have 
		     * been removed at this time. -APD
		     *
		     * Restored after the advent of Tcp/IP, becuase there is
		     * no longer any good reason to allow them.  --Crimson
		
		    if (!strcasecmp(Players[i]->realname, real_name) &&
			!strcasecmp(Players[i]->addr, addr) &&
			strcasecmp(Players[i]->realname, cfg_admin_wizard) &&
			strcasecmp(Players[i]->realname, cfg_dungeon_master))
		    {
			return E_TWO_PLAYERS;
		    }
		     */
		}
		/* */
	}

	return SUCCESS;
}

#if 0
static void Console(int fd, int arg)
{
	char buf[1024];
	int i;

	/* See what we got */
        /* this code added by thaler, 6/28/97 */
        fgets(buf, 1024, stdin);
        if (buf[ strlen(buf)-1 ] == '\n')
            buf[ strlen(buf)-1 ] = '\0';

	for (i = 0; i < strlen(buf) && buf[i] != ' '; i++)
	{
		/* Capitalize each letter until we hit a space */
		buf[i] = toupper(buf[i]);
	}

	/* Process our input */
	if (!strncmp(buf, "HELLO", 5))
		s_printf("Hello.  How are you?\n");

	if (!strncmp(buf, "SHUTDOWN", 8))
	{
		shutdown_server();
	}

		
	if (!strncmp(buf, "STATUS", 6))
	{
		s_printf("There %s %d %s.\n", (NumPlayers != 1 ? "are" : "is"), NumPlayers, (NumPlayers != 1 ? "players" : "player"));

		if (NumPlayers > 0)
		{
			s_printf("%s:\n", (NumPlayers > 1 ? "They are" : "He is"));
			for (i = 1; i < NumPlayers + 1; i++)
				s_printf("\t%s\n", Players[i]->name);
		}
	}

	if (!strncmp(buf, "MESSAGE", 7))
	{
		/* Send message to all players */
		for (i = 1; i <= NumPlayers; i++)
			msg_format(i, "[Server Admin] %s", &buf[8]);

		/* Acknowledge */
		s_printf("Message sent.\n");
	}
		
	if (!strncmp(buf, "KELDON", 6))
	{
		/* Whatever I need at the moment */
	}
}
#endif // if 0
		
static void Contact(int fd, int arg)
{
	int bytes, login_port, newsock;
	u16b version = 0;
	unsigned magic;
	unsigned short port;
	char	ch,
		real_name[MAX_CHARS],
		nick_name[MAX_CHARS],
		host_name[MAX_CHARS],
		host_addr[24],
		reply_to, status;

	/* Create a TCP socket for communication with whoever contacted us */
	/* Hack -- check if this data has arrived on the contact socket or not.
	 * If it has, then we have not created a connection with the client yet, 
	 * and so we must do so.
	 */

	if (fd == Socket)
	{
		if ((newsock = SocketAccept(fd)) == -1)
		{
			quit("Couldn't accept game TCP connection.\n");
		}
		install_input(Contact, newsock, 2);
		return;
	}

	/*
	 * Someone connected to us, now try and decipher the message
	 */
	Sockbuf_clear(&ibuf);
	if ((bytes = DgramReceiveAny(fd, ibuf.buf, ibuf.size)) <= 8)
	{
		/* If 0 bytes have been sent than the client has probably closed
		 * the connection
		 */
		if (bytes == 0)
		{
	/* evileye - still in contact input, so close the socket here */
	/* Dont tell me it is ugly. I know ;( */
	/* Sched should do accepts and closes */
			close(fd);
	/*end evileye*/
			remove_input(fd);
		}
		else if (bytes < 0 && errno != EWOULDBLOCK && errno != EAGAIN &&
			errno != EINTR)
		{
			/* Clear the error condition for the contact socket */
			GetSocketError(fd);
		}
		return;
	}
	ibuf.len = bytes;

	strcpy(host_addr, DgramLastaddr(fd));
	/*if (Check_address(host_addr)) return;*/

	if (Packet_scanf(&ibuf, "%u", &magic) <= 0)
	{
		plog(format("Incompatible packet from %s", host_addr));
		return;
	}

	if (Packet_scanf(&ibuf, "%s%hu%c", real_name, &port, &ch) <= 0)
	{
		plog(format("Incomplete packet from %s", host_addr));
		return;
	}
	reply_to = (ch & 0xFF);

	port = DgramLastport(fd);

	if (Packet_scanf(&ibuf, "%s%s%hu", nick_name, host_name, &version) <= 0)
	{
		plog(format("Incomplete login from %s", host_addr));
		return;
	}
	nick_name[sizeof(nick_name) - 1] = '\0';
	host_name[sizeof(host_name) - 1] = '\0';

#if 0
	s_printf("Received contact from %s:%d.\n", host_name, port);
	s_printf("Address: %s.\n", host_addr);
	s_printf("Info: real_name %s, port %hu, nick %s, host %s, version %hu\n", real_name, port, nick_name, host_name, version);  
#endif


	status = Enter_player(real_name, nick_name, host_addr, host_name,
				version, port, &login_port, fd);

#if DEBUG_LEVEL > 1
	if (status && status != E_NEED_INFO)
		s_printf("%s: Connection refused(%d).. %s=%s@%s (%s/%d)\n", showtime(),
				status, nick_name, real_name, host_name, host_addr, port);
#endif	// DEBUG_LEVEL

	Sockbuf_clear(&ibuf);

	/* s_printf("Sending login port %d, status %d.\n", login_port, status); */

        Packet_printf(&ibuf, "%c%c%d%c", reply_to, status, login_port);
//        Packet_printf(&ibuf, "%c%c%d%c", reply_to, status, login_port, MAX_CLASS);

/* -- DGDGDGDG it would be NEAT to have classes sent to the cleint at conenciton, sadly Im too clumpsy at network code ..
        for (i = 0; i < MAX_CLASS; i++)
        {
                 Packet_printf(&ibuf, "%c%s", i, class_info[i].title);
        }
*/
	Reply(host_addr, fd);
}

static int Enter_player(char *real, char *nick, char *addr, char *host,
				unsigned version, int port, int *login_port, int fd)
{
	//int status;

	*login_port = 0;

	if (NumPlayers >= MAX_SELECT_FD)
		return E_GAME_FULL;

#if 0	/* This would pass in the account name rather than the
	   player's character name. Also, we must *always* allow
	   a second account login - it may be a subsequent resume.
	   We can check duplicate account use on player entry
	   (PKT_LOGIN) */
	if ((status = Check_names(nick, real, host, addr)) != SUCCESS)
	{
		/*s_printf("Check_names failed with result %d.\n", status);*/
		return status;
	}
#endif

	if (version < ((4 << 12) | (0 << 8) | (0 << 4) | 0))
		return E_VERSION;

	if(!player_allowed(nick))
		return E_INVITE;

	if(in_banlist(addr)) return(E_BANNED);

	*login_port = Setup_connection(real, nick, addr, host, version, fd);

	if (*login_port == -1)
		return E_SOCKET;

/* Can't do this here anymore - evileye */
/*
	if (!lookup_player_id(nick))
		return E_NEED_INFO;
*/

	return SUCCESS;
}
		

static void Conn_set_state(connection_t *connp, int state, int drain_state)
{
	static int num_conn_busy;
	static int num_conn_playing;

	if ((connp->state & (CONN_PLAYING | CONN_READY)) != 0)
		num_conn_playing--;
	else if (connp->state == CONN_FREE)
		num_conn_busy++;

	connp->state = state;
	connp->drain_state = drain_state;
	connp->start = turn;

	if (connp->state == CONN_PLAYING)
	{
		num_conn_playing++;
		connp->timeout = IDLE_TIMEOUT;
	}
	else if (connp->state == CONN_READY)
	{
		num_conn_playing++;
		connp->timeout = READY_TIMEOUT;
	}
	else if (connp->state == CONN_LOGIN)
		connp->timeout = LOGIN_TIMEOUT;
	else if (connp->state == CONN_SETUP)
		connp->timeout = SETUP_TIMEOUT;
	else if (connp->state == CONN_LISTENING)
		connp->timeout = LISTEN_TIMEOUT;
	else if (connp->state == CONN_FREE)
	{
		num_conn_busy--;
		connp->timeout = IDLE_TIMEOUT;
	}
	login_in_progress = num_conn_busy - num_conn_playing;
}


/*
 * Delete a player's information and save his game
 */
static void Delete_player(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int i;

	/* Be paranoid */
	cave_type **zcave;
	if ((zcave=getcave(&p_ptr->wpos)))
	{
		/* There's nobody on this space anymore */
		zcave[p_ptr->py][p_ptr->px].m_idx = 0;

		/* Forget his lite and viewing area */
		forget_lite(Ind);
		forget_view(Ind);

		/* Show everyone his disappearance */
		everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);
	}

	/* Try to save his character */
	save_player(Ind);

	/* If he was actively playing, tell everyone that he's left */
	/* handle the cfg_secret_dungeon_master option */
	if (p_ptr->alive && !p_ptr->death && 
	   (!p_ptr->admin_dm || !cfg.secret_dungeon_master))
	{
		cptr title = "";
		if (p_ptr->total_winner)
		{
			if (p_ptr->mode & (MODE_HELL | MODE_NO_GHOST))
			{
				title = (p_ptr->male)?"God ":"Goddess ";
			}
			else
			{
				title = (p_ptr->male)?"King ":"Queen ";
			}
		}

		world_player(p_ptr->id, p_ptr->name, FALSE);

		for (i = 1; i < NumPlayers + 1; i++)
		{
			if (Players[i]->conn == NOT_CONNECTED)
				continue;

			/* Don't tell him about himself */
			if (i == Ind) continue;

			/* Send a little message */
			msg_format(i, "%s%s has left the game.", title, p_ptr->name);
		}
	}

	if(p_ptr->esp_link_type && p_ptr->esp_link){
		/* This is the last chance to get out!!! */
		int Ind2=find_player(p_ptr->esp_link);
		if(Ind2) end_mind(Ind2, TRUE);
	}


	/* Swap entry number 'Ind' with the last one */
	/* Also, update the "player_index" on the cave grids */
	if (Ind != NumPlayers)
	{
		cave_type **zcave;
		p_ptr			= Players[NumPlayers];
		if((zcave=getcave(&p_ptr->wpos)))
			zcave[p_ptr->py][p_ptr->px].m_idx = 0 - Ind;
		Players[NumPlayers]	= Players[Ind];
		Players[Ind]		= p_ptr;
		p_ptr			= Players[NumPlayers];
	}

	if (Conn[Players[Ind]->conn].id != -1)
		GetInd[Conn[Players[Ind]->conn].id] = Ind;
	if (Conn[Players[NumPlayers]->conn].id != -1)
		GetInd[Conn[Players[NumPlayers]->conn].id] = NumPlayers;

	/* Recalculate player-player visibility */
	update_players();

	if (p_ptr)
	{
		if (p_ptr->inventory)
			C_KILL(p_ptr->inventory, INVEN_TOTAL, object_type);

		KILL(Players[NumPlayers], player_type);
	}

	NumPlayers--;

	/* Tell the metaserver about the loss of a player */	
	Report_to_meta(META_UPDATE);
}


/*
 * Cleanup a connection.  The client may not know yet that it is thrown out of
 * the game so we send it a quit packet if our connection to it has not already
 * closed.  If our connection to it has been closed, then connp->w.sock will
 * be set to -1.
 */
bool Destroy_connection(int ind, char *reason)
{
	connection_t		*connp = &Conn[ind];
	int			id, len, sock;
	char			pkt[MAX_CHARS];

	kill_xfers(ind);	/* don't waste time sending to a dead
				   connection ( or crash! ) */

	if (connp->state == CONN_FREE)
	{
		errno = 0;
		plog(format("Cannot destroy empty connection (\"%s\")", reason));
		return FALSE;
	}

	sock = connp->w.sock;
	if (sock != -1)
	{
		remove_input(sock);
	}

	strncpy(&pkt[1], reason, sizeof(pkt) - 3);
	pkt[sizeof(pkt) - 2] = '\0';
	pkt[0] = PKT_QUIT;
	len = strlen(pkt) + 2;
	pkt[len - 1] = PKT_END;
	pkt[len] = '\0';
	/*len++;*/
	if (sock != -1)
	{
#if 1	// sorry evileye, removing it causes SIGPIPE to the client
		if (DgramWrite(sock, pkt, len) != len)
		{
			GetSocketError(sock);
			DgramWrite(sock, pkt, len);
		}
#endif
	}
	s_printf("%s: Goodbye %s(%s)=%s@%s (\"%s\")\n",
		showtime(),
		connp->c_name ? connp->c_name : "",
		connp->nick ? connp->nick : "",
		connp->real ? connp->real : "",
		connp->host ? connp->host : "",
		reason);

	Conn_set_state(connp, CONN_FREE, CONN_FREE);

	if (connp->id != -1)
	{
		id = connp->id;
		connp->id = -1;
	/*
		Players[GetInd[id]]->conn = NOT_CONNECTED;
	*/
		Delete_player(GetInd[id]);
	}
	if (connp->real != NULL) free(connp->real);
	if (connp->nick != NULL) free(connp->nick);
	if (connp->addr != NULL) free(connp->addr);
	if (connp->host != NULL) free(connp->host);
	Sockbuf_cleanup(&connp->w);
	Sockbuf_cleanup(&connp->r);
	Sockbuf_cleanup(&connp->c);
	Sockbuf_cleanup(&connp->q);
	memset(connp, 0, sizeof(*connp));

	num_logouts++;

	if (sock != -1)
	{
		DgramClose(sock);
	}

	return TRUE;
}

int Check_connection(char *real, char *nick, char *addr)
{
	int i;
	connection_t *connp;

	for (i = 0; i < max_connections; i++)
	{
		connp = &Conn[i];
		if (connp->state == CONN_LISTENING)
			if (strcasecmp(connp->nick, nick) == 0)
			{
				if (!strcmp(real, connp->real)
					&& !strcmp(addr, connp->addr))
						return connp->my_port;
				return -1;
			}
	}
	return -1;
}


/*
 * A client has requested a playing connection with this server.
 * See if we have room for one more player and if his name is not
 * already in use by some other player.  Because the confirmation
 * may get lost we are willing to send it another time if the
 * client connection is still in the CONN_LISTENING state.
 */
int Setup_connection(char *real, char *nick, char *addr, char *host,
			unsigned version, int fd)
{
	int i, free_conn_index = max_connections, my_port, sock;
	connection_t *connp;

	for (i = 0; i < max_connections; i++)
	{
		connp = &Conn[i];
		if (connp->state == CONN_FREE)
		{
			if (free_conn_index == max_connections)
				free_conn_index = i;
			continue;
		}

		/* Do not deny access here, or we cannot *ever*
		   resume or allow multiple connections from
		   a single account. */
#if 0
		if (strcasecmp(connp->nick, nick) == 0)
		{
			if (connp->state == CONN_LISTENING
				&& strcmp(real, connp->real) == 0
				&& version == connp->version)
					return connp->my_port;
			else return -1;
		}
#endif
	}

	if (free_conn_index >= max_connections)
	{
		s_printf("Full house for %s(%s)@%s\n", real, nick, host);
		return -1;
	}
	connp = &Conn[free_conn_index];

	// A TCP connection already exists with the client, use it.
	sock = fd;

	if ((my_port = GetPortNum(sock)) == 0)
	{
		plog("Cannot get port from socket");
		DgramClose(sock);
		return -1;
	}
	if (SetSocketNonBlocking(sock, 1) == -1)
	{
		plog("Cannot make client socket non-blocking");
		DgramClose(sock);
		return -1;
	}
	if (SocketLinger(sock) == -1)
	{
		plog("Couldn't set SO_LINGER on the socket");
	}
	if (SetSocketReceiveBufferSize(sock, SERVER_RECV_SIZE + 256) == -1)
		plog(format("Cannot set receive buffer size to %d", SERVER_RECV_SIZE + 256));
	if (SetSocketSendBufferSize(sock, SERVER_SEND_SIZE + 256) == -1)
		plog(format("Cannot set send buffer size to %d", SERVER_SEND_SIZE + 256));

	Sockbuf_init(&connp->w, sock, SERVER_SEND_SIZE, SOCKBUF_WRITE);
	Sockbuf_init(&connp->r, sock, SERVER_RECV_SIZE, SOCKBUF_WRITE | SOCKBUF_READ);
	Sockbuf_init(&connp->c, -1, MAX_SOCKBUF_SIZE, SOCKBUF_WRITE | SOCKBUF_READ | SOCKBUF_LOCK);
	Sockbuf_init(&connp->q, -1, MAX_SOCKBUF_SIZE, SOCKBUF_WRITE | SOCKBUF_READ | SOCKBUF_LOCK);

	connp->my_port = my_port;
	connp->real = strdup(real);
	connp->nick = strdup(nick);
	connp->addr = strdup(addr);
	connp->host = strdup(host);
	connp->version = version;
	connp->start = turn;
	connp->magic = rand() + my_port + sock + turn;
	connp->id = -1;
	connp->timeout = LISTEN_TIMEOUT;
	connp->reliable_offset = 0;
	connp->reliable_unsent = 0;
	connp->last_send_loops = 0;
	connp->retransmit_at_loop = 0;
	connp->rtt_retransmit = DEFAULT_RETRANSMIT;
	connp->rtt_smoothed = 0;
	connp->rtt_dev = 0;
	connp->rtt_timeouts = 0;
	connp->acks = 0;
	connp->setup = 0;
	Conn_set_state(connp, CONN_LISTENING, CONN_FREE);
	if (connp->w.buf == NULL || connp->r.buf == NULL || connp->c.buf == NULL
		|| connp->q.buf == NULL || connp->real == NULL || connp->nick == NULL
		|| connp->addr == NULL || connp->host == NULL)
	{
		plog("Not enough memory for connection");
		Destroy_connection(free_conn_index, "no memory");
		return -1;
	}
	
	// Remove the contact input handler
	remove_input(sock);
	// Install the game input handler
	install_input(Handle_input, sock, free_conn_index);

	return my_port;
}

static int Handle_setup(int ind)
{
	connection_t *connp = &Conn[ind];
	char *buf;
	int n, len, i;
	
	if (connp->state != CONN_SETUP)
	{
		Destroy_connection(ind, "not setup");
		return -1;
	}

	if (connp->setup == 0)
	{
		n = Packet_printf(&connp->c, "%ld%hd%c%c%ld",
			Setup.motd_len, Setup.frames_per_second, Setup.max_race, Setup.max_class, Setup.setup_size);

		if (n <= 0)
		{
			Destroy_connection(ind, "Setup 0 write error");
			return -1;
		}

        for (i = 0; i < Setup.max_race; i++)
        {
//			Packet_printf(&ibuf, "%c%s", i, class_info[i].title);
//			Packet_printf(&connp->c, "%s%ld", Setup.race_title[i], Setup.race_choice[i]);
			Packet_printf(&connp->c, "%s%ld", race_info[i].title, race_info[i].choice);
        }

        for (i = 0; i < Setup.max_class; i++)
        {
//			Packet_printf(&ibuf, "%c%s", i, class_info[i].title);
//			Packet_printf(&connp->c, "%s", Setup.class_title[i]);
			Packet_printf(&connp->c, "%s", class_info[i].title);
        }

		connp->setup = (char *) &Setup.motd[0] - (char *) &Setup;
		connp->setup=0;
	}
	/* else if (connp->setup < Setup.setup_size) */
	else if (connp->setup < Setup.motd_len)
	{
		if (connp->c.len > 0)
		{
			/* If there is still unacked reliable data test for acks. */
			Handle_input(-1, ind);
			if (connp->state == CONN_FREE)
				return -1;
		}
	}
	/* if (connp->setup < Setup.setup_size) */
	if (connp->setup < Setup.motd_len)
	{
		len = MIN(connp->c.size, 4096) - connp->c.len;
		if (len <= 0)
		{
			/* Wait for acknowledgement of previously transmitted data. */
			return 0;
		}
	/*
		if (len > Setup.setup_size - connp->setup)
			len = Setup.setup_size - connp->setup;
	*/

		if(len>Setup.motd_len-connp->setup){
			len=Setup.motd_len-connp->setup;
			len=Setup.motd_len;
		}

	/*	buf = (char *) &Setup; */
		buf = (char *) &Setup.motd[0];
		if (Sockbuf_write(&connp->c, &buf[connp->setup], len) != len)
		{
			Destroy_connection(ind, "sockbuf write setup error");
			return -1;
		}
		connp->setup += len;
		if (len >= 512)
			connp->start += (len * cfg.fps) / (8 * 512) + 1;
	}

	/* if (connp->setup >= Setup.setup_size) */
	if (connp->setup >= Setup.motd_len)
		//Conn_set_state(connp, CONN_DRAIN, CONN_LOGIN);
		Conn_set_state(connp, CONN_LOGIN, CONN_LOGIN);

	return 0;
}

/*
 * Handle a connection that is in the listening state.
 */
static int Handle_listening(int ind)
{
	connection_t *connp = &Conn[ind];
	unsigned char type;
	//int i, n, oldlen, result;
	int  n, oldlen;
//	s16b sex, race, class;
	char nick[MAX_NAME_LEN], real[MAX_NAME_LEN], pass[MAX_NAME_LEN];

	if (connp->state != CONN_LISTENING)
	{
		Destroy_connection(ind, "not listening");
		return -1;
	}
	//Sockbuf_clear(&connp->r);
	errno = 0;

	/* Some data has arrived on the socket.  Read this data into r.buf.
	 */
	//n = DgramReceiveAny(connp->r.sock, connp->r.buf, connp->r.size);
	oldlen = connp->r.len;
	n = Sockbuf_read(&connp->r);
	if (n - oldlen <= 0)
	{
		//if (n == 0 || errno == EWOULDBLOCK || errno == EAGAIN)
		//	n = 0;
		if (n == 0)
		{
			/* Hack -- set sock to -1 so destroy connection doesn't
			 * try to inform the client about its destruction
			 */
			remove_input(connp->w.sock);
			connp->w.sock = -1;
			Destroy_connection(ind, "TCP connection closed");
		}
		/* It's already Dead, Jim. 
		else 
			Destroy_connection(ind, "read first packet error");
		*/
		return -1;
	}
	connp->his_port = DgramLastport(connp->r.sock);

	/* Do a sanity check and read in the some basic player information. */
	/* XXX reason messages here are not transmitted to the client!	- Jir - */
	if (connp->r.ptr[0] != PKT_VERIFY)
	{
		Send_reply(ind, PKT_VERIFY, PKT_FAILURE);
		Send_reliable(ind);
		Destroy_connection(ind, "not connecting");
		return -1;
	}
//	if ((n = Packet_scanf(&connp->r, "%c%s%s%s%hd%hd%hd", &type, real, nick, pass, &sex, &race, &class)) <= 0)
	if ((n = Packet_scanf(&connp->r, "%c%s%s%s", &type, real, nick, pass)) <= 0)
	{
		Send_reply(ind, PKT_VERIFY, PKT_FAILURE);
		Send_reliable(ind);
		Destroy_connection(ind, "verify broken");
		return -1;
	}
#if 0	// moved to Enter_player
	if(!player_allowed(nick)){
		Send_reply(ind, PKT_VERIFY, PKT_FAILURE);
		Send_reliable(ind);
		Destroy_connection(ind, "No such character");
		return(-1);
	}
	if(in_banlist(connp->addr)){
		Send_reply(ind, PKT_VERIFY, PKT_FAILURE);
		Send_reliable(ind);
		Destroy_connection(ind, "You are temporarily banned.");
		return(-1);
	}
#endif	// 0


	/* After the client sends the basic player information, it sends us a
	 * large block of "verification" data.  Because this block may have
	 * been broken up into several packets, we may only have the beginning
	 * of it.  Check to see if we have all this data.  If we don't, then
	 * exit this function.  It will be called again when more data has
	 * arrived.
	 */
	// The verification block is 2654 bytes long.  Make sure we have this
	// much data past the basic player information.  If we don't, then reset
	// the read location and exit.
	/*
	 * It's quite doubtful if it's 2654 bytes, since MAX_R_IDX and MAX_K_IDX
	 * etc are much bigger than before;  however, let's follow the saying
	 * 'Never touch what works' ;)		- Jir -
	 */
#if 0
	if (2654 > connp->r.len - (connp->r.ptr - connp->r.buf))
	{
		connp->r.ptr = connp->r.buf;
		return 1;
	}
#endif	// 0
	
	/* toast this after fix
	*If we don't, then
	* wait a bit for more to arrive.  Waiting causes the game to "freeze"
	* for a bit.  This is really bad.  The connection code desperatly
	* needs to be rewritten to prevent this from happening. -APD
	*/

	/* Log the players connection */
	s_printf("%s: Welcome %s=%s@%s (%s/%d)", showtime(), connp->nick,
		connp->real, connp->host, connp->addr, connp->his_port);
	if (connp->version != MY_VERSION)
		s_printf(" (version %04x)", connp->version);
	s_printf("\n");


	// The verification block is 2654 bytes long.  Make sure we have this
	// much data past the basic player information.  If we don't, then do a
	// blocking read while we try to get it.

	/* If neccecary receive the rest of the verification data */
	// note that connp->r.ptr is now pointing past the basic player information, and so
	// connp->r.ptr - connp->r.buf is the length of the player information.
	
	// Mega-hack -- disable the timer interrupt so select isn't disturbed
#if 0
	block_timer();	

	SetTimeout(1,0);	
	while (2654 > connp->r.len - (connp->r.ptr - connp->r.buf))
	{
		// If we have data, read it
		if ((result = SocketReadable(connp->r.sock)))
		{
			if (result == -1)
			{
				Destroy_connection(ind, "verification socket error");
				return -1;
			}
			printf("Got verification data.\n");
			n = DgramReceiveAny(connp->r.sock, connp->r.buf + connp->r.len, 
					connp->r.size - connp->r.len);
			if (n < 0)
			{
				Destroy_connection(ind, "verification packet error");
				return -1;
			}
			if (n == 0)
			{
				Destroy_connection(ind, "TCP connection closed");
				return -1;
			}
			connp->r.len += n;
		}
		// If we timed out, abort
		else
		{
			Destroy_connection(ind, "verify incomplete");
			return -1;
		}
	}
	SetTimeout(0,0); 

	allow_timer();
#endif

/*	s_printf("Listening on connection %d, received %d bytes.\n", ind, n);

	for (i = 0; i < n; i++)
	{
		printf("%3d ", connp->r.ptr[i] & 0xff);
		if (i % 20 == 0)
			printf("\n");
	}

	printf("\n"); */

#if 0	// moved to Receive_play
	/* Read the stat order */
	for (i = 0; i < 6; i++)
	{
		n = Packet_scanf(&connp->r, "%hd", &connp->stat_order[i]);

		if (n <= 0)
		{
			Destroy_connection(ind, "Misread stat order");
			return -1;
		}
	}
	
	/* Read class extra */	
//	n = Packet_scanf(&connp->r, "%hd", &connp->class_extra);

	if (n <= 0)
	{
		Destroy_connection(ind, "Misread class extra");
		return -1;
	}

	/* Read the options */
	for (i = 0; i < 64; i++)
	{
		n = Packet_scanf(&connp->r, "%c", &connp->Client_setup.options[i]);

		if (n <= 0)
		{
			Destroy_connection(ind, "Misread options");
			return -1;
		}
	}

	/* Read the "unknown" char/attrs */
	for (i = 0; i < TV_MAX; i++)
	{
		n = Packet_scanf(&connp->r, "%c%c", &connp->Client_setup.u_attr[i], &connp->Client_setup.u_char[i]);

		if (n <= 0)
		{
			break;

#if 0
			Destroy_connection(ind, "Misread unknown redefinitions");
			return -1;
#endif
		}
	}

	/* Read the "feature" char/attrs */
	for (i = 0; i < MAX_F_IDX; i++)
	{
		n = Packet_scanf(&connp->r, "%c%c", &connp->Client_setup.f_attr[i], &connp->Client_setup.f_char[i]);

		if (n <= 0)
		{
			break;

#if 0
			Destroy_connection(ind, "Misread feature redefinitions");
			return -1;
#endif
		}
	}

	/* Read the "object" char/attrs */
	for (i = 0; i < MAX_K_IDX; i++)
	{
		n = Packet_scanf(&connp->r, "%c%c", &connp->Client_setup.k_attr[i], &connp->Client_setup.k_char[i]);

		if (n <= 0)
		{
			break;

#if 0
			Destroy_connection(ind, "Misread object redefinitions");
			return -1;
#endif
		}
	}

	/* Read the "monster" char/attrs */
	for (i = 0; i < MAX_R_IDX; i++)
	{
		n = Packet_scanf(&connp->r, "%c%c", &connp->Client_setup.r_attr[i], &connp->Client_setup.r_char[i]);

		if (n <= 0)
		{
			break;

#if 0
			Destroy_connection(ind, "Misread monster redefinitions");
			return -1;
#endif
		}
	}
#endif	// 0

	if (strcmp(real, connp->real))
	{
		s_printf("Client verified incorrectly (%s, %s)(%s, %s)",
			real, nick, connp->real, connp->nick);
		Send_reply(ind, PKT_VERIFY, PKT_FAILURE);
		Send_reliable(ind);
		Destroy_connection(ind, "verify incorrect");
		return -1;
	}
	
	/* Set his character info */
	connp->pass = strdup(pass);
#if 0
	connp->sex = sex;
	connp->race = race;
	connp->class = class;
#endif	// 0




	Sockbuf_clear(&connp->w);
	if (Send_reply(ind, PKT_VERIFY, PKT_SUCCESS) == -1
		|| Packet_printf(&connp->c, "%c%u", PKT_MAGIC, connp->magic) <= 0
		|| Send_reliable(ind) <= 0)
	{
		Destroy_connection(ind, "confirm failed");
		return -1;
	}

	//Conn_set_state(connp, CONN_DRAIN, CONN_SETUP);
	Conn_set_state(connp, CONN_SETUP, CONN_SETUP);

	return -1;
}

/*
 * Sync the named options from the array of options.
 *
 * This is a crappy way of doing things....
 */
/* see client_opts */
static void sync_options(int Ind, bool *options)
{
	player_type *p_ptr = Players[Ind];

	/* Do the dirty work */
	p_ptr->carry_query_flag = options[3];
	p_ptr->use_old_target = options[4];
	p_ptr->always_pickup = options[5];
	p_ptr->stack_force_notes = options[8];
	p_ptr->stack_force_costs = options[9];
	p_ptr->find_ignore_stairs = options[16];
	p_ptr->find_ignore_doors = options[17];
	p_ptr->find_cut = options[18];
	p_ptr->find_examine = options[19];
	p_ptr->disturb_move = options[20];
	p_ptr->disturb_near = options[21];
	p_ptr->disturb_panel = options[22];
	p_ptr->disturb_state = options[23];
	p_ptr->disturb_minor = options[24];
	p_ptr->disturb_other = options[25];
	p_ptr->auto_afk = options[28];
	p_ptr->newb_suicide = options[29];
	p_ptr->stack_allow_items = options[30];
	p_ptr->stack_allow_wands = options[31];
	p_ptr->view_perma_grids = options[34];
	p_ptr->view_torch_grids = options[35];
	p_ptr->view_reduce_lite = options[44];
	p_ptr->view_reduce_view = options[45];
	p_ptr->view_yellow_lite = options[56];
	p_ptr->view_bright_lite = options[57];
	p_ptr->view_granite_lite = options[58];
	p_ptr->view_special_lite = options[59];

	p_ptr->easy_open = options[60];
	p_ptr->easy_disarm = options[61];
	p_ptr->easy_tunnel = options[62];
	p_ptr->auto_destroy = options[63];
	p_ptr->auto_inscribe = options[64];
	p_ptr->taciturn_messages = options[65];
	p_ptr->last_words = options[66];
	p_ptr->limit_chat = options[67];

	p_ptr->depth_in_feet = options[7];
	p_ptr->auto_target = options[69];
	p_ptr->autooff_retaliator = options[70];
	p_ptr->wide_scroll_margin = options[71];
	p_ptr->always_repeat = options[6];
	p_ptr->fail_no_melee = options[72];
	// bool speak_unique;

}

/*
 * A client has requested to start active play.
 * See if we can allocate a player structure for it
 * and if this succeeds update the player information
 * to all connected players.
 */
static int Handle_login(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;
	int i;
	bool options[OPT_MAX];

	if (Id >= MAX_ID)
	{
		errno = 0;
		plog(format("Id too big (%d)", Id));
		return -1;
	}

	/* This will cause problems for account/char with same name */
#if 0
	for (i = 1; i < NumPlayers + 1; i++)
	{
		if (strcasecmp(Players[i]->name, connp->nick) == 0)
		{
			errno = 0;
			plog(format("Name already in use %s", connp->nick));
			return -1;
		}
	}
#endif

	if (!player_birth(NumPlayers + 1, connp->nick, connp->c_name, ind, connp->race, connp->class, connp->sex, connp->stat_order))
	{
		/* Failed, connection destroyed */
		Destroy_connection(ind, "not login");
		return -1;
	}

	p_ptr = Players[NumPlayers + 1];
	strcpy(p_ptr->realname, connp->real);
	strcpy(p_ptr->hostname, connp->host);
	strcpy(p_ptr->addr, connp->addr);
	p_ptr->version = connp->version;

	/* Copy the client preferences to the player struct */
	for (i = 0; i < OPT_MAX; i++)
	{
		options[i] = connp->Client_setup.options[i];
	}

	for (i = 0; i < TV_MAX; i++)
	{
		int j;

		if (!connp->Client_setup.u_attr[i] &&
		    !connp->Client_setup.u_char[i])
			continue;

		/* XXX not max_k_idx, since client doesn't know the value */
		for (j = 0; j < MAX_K_IDX; j++)
		{
			if (k_info[j].tval == i)
			{
				p_ptr->d_attr[j] = connp->Client_setup.u_attr[i];
				p_ptr->d_char[j] = connp->Client_setup.u_char[i];
			}
		}
	}

	for (i = 0; i < MAX_F_IDX; i++)
	{
		p_ptr->f_attr[i] = connp->Client_setup.f_attr[i];
		p_ptr->f_char[i] = connp->Client_setup.f_char[i];

		if (!p_ptr->f_attr[i]) p_ptr->f_attr[i] = f_info[i].z_attr;
		if (!p_ptr->f_char[i]) p_ptr->f_char[i] = f_info[i].z_char;
	}

	for (i = 0; i < MAX_K_IDX; i++)
	{
		p_ptr->k_attr[i] = connp->Client_setup.k_attr[i];
		p_ptr->k_char[i] = connp->Client_setup.k_char[i];

		if (!p_ptr->k_attr[i]) p_ptr->k_attr[i] = k_info[i].x_attr;
		if (!p_ptr->k_char[i]) p_ptr->k_char[i] = k_info[i].x_char;

#if 1
		if (!p_ptr->d_attr[i]) p_ptr->d_attr[i] = k_info[i].d_attr;
		if (!p_ptr->d_char[i]) p_ptr->d_char[i] = k_info[i].d_char;
#else
		if (!p_ptr->k_attr[i]) p_ptr->d_attr[i] = k_info[i].d_attr;
		if (!p_ptr->k_char[i]) p_ptr->d_char[i] = k_info[i].d_char;
#endif	// 0
	}

	/* Hack -- acquire a flag for ego-monsters	- Jir - */
	p_ptr->use_r_gfx = FALSE;

	for (i = 0; i < MAX_R_IDX; i++)
	{
		p_ptr->r_attr[i] = connp->Client_setup.r_attr[i];
		p_ptr->r_char[i] = connp->Client_setup.r_char[i];

		if (!p_ptr->r_attr[i]) p_ptr->r_attr[i] = r_info[i].x_attr;
		else p_ptr->use_r_gfx = TRUE;

		if (!p_ptr->r_char[i]) p_ptr->r_char[i] = r_info[i].x_char;
		else p_ptr->use_r_gfx = TRUE;
	}

	sync_options(NumPlayers + 1, options);

	GetInd[Id] = NumPlayers + 1;

	NumPlayers++;
	connp->id = Id++;
	
	//Conn_set_state(connp, CONN_READY, CONN_PLAYING);
	Conn_set_state(connp, CONN_PLAYING, CONN_PLAYING);

	if (Send_reply(ind, PKT_PLAY, PKT_SUCCESS) <= 0)
	{
		plog("Cannot send play reply");
		return -1;
	}
	
	/* Send party information */
	Send_party(NumPlayers);

	/* Hack -- terminate the data stream sent to the client */
	if (Packet_printf(&connp->c, "%c", PKT_END) <= 0)
	{
		Destroy_connection(p_ptr->conn, "write error");
		return -1;
	}

	if (Send_reliable(ind) == -1)
	{
		Destroy_connection(ind, "Couldn't send reliable data");
		return -1;
	}

	num_logins++;

	save_server_info();

	/* Handle the cfg_secret_dungeon_master option */
	if (p_ptr->admin_dm && (cfg.secret_dungeon_master)) return 0;

	world_player(p_ptr->id, p_ptr->name, TRUE);

	/* Tell everyone about our new player */
	for (i = 1; i < NumPlayers; i++)
	{
	  cptr title = "";

		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		if (p_ptr->total_winner)
		  {
			if (p_ptr->mode & (MODE_HELL | MODE_NO_GHOST))
		      {
			title = (p_ptr->male)?"God ":"Goddess ";
		      }
		    else
		      {
			title = (p_ptr->male)?"King ":"Queen ";
		      }
		  }

		msg_format(i, "%s%s has entered the game.", title, p_ptr->name);
	}

	if(p_ptr->quest_id){
		for(i=0; i<20; i++){
			if(quests[i].id==p_ptr->quest_id){
				msg_format(NumPlayers, "\377oYour quest to kill \377y%d \377g%s \377ois not complete.", p_ptr->quest_num, r_name+r_info[quests[i].type].name);
				break;
			}
		}
	}

	/* Tell the meta server about the new player */
	Report_to_meta(META_UPDATE);

	return 0;
}

/* Actually execute commands from the client command queue */
void process_pending_commands(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;	
	int player = -1, type, result, (**receive_tbl)(int ind) = playing_receive, old_energy = 0;
	int num_players_start = NumPlayers; // Hack to see if we have quit in this function

	// Hack -- take any pending commands from the command que connp->q
	// and move them to connp->r, where the Receive functions get their
	// data from.
	Sockbuf_clear(&connp->r);
	if (connp->q.len > 0)
	{
		if (Sockbuf_write(&connp->r, connp->q.ptr, connp->q.len) != connp->q.len)
		{
			errno = 0;
			Destroy_connection(ind, "Can't copy queued data to buffer");
			return;
		}
		//connp->q.ptr += connp->q.len;
		//Sockbuf_advance(&connp->q, connp->q.ptr - connp->q.buf);
		Sockbuf_clear(&connp->q);
	}

	// If we have no commands to execute return
	if (connp->r.len <= 0)
		return;

	// Get the player pointer
	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];
	}
	// Hack -- if our player id has not been set then assume that Receive_play
	// should be called.
	else
	{
		Receive_play(ind);
		return;
	}

	// Attempt to execute every pending command. Any command that fails due
	// to lack of energy will be put into the queue for next turn by the
	// respective receive function. 		

	//while ( (p_ptr->energy >= level_speed(p_ptr->dun_depth)) && 
	//while ( (connp->state == CONN_PLAYING ? p_ptr->energy >= level_speed(p_ptr->dun_depth) : 1) && 
	//while ( (connp->state == CONN_PLAYING ? p_ptr->energy >= level_speed(p_ptr->dun_depth) : 1) && 
	while ((connp->r.ptr < connp->r.buf + connp->r.len))
	{
		type = (connp->r.ptr[0] & 0xFF);
		if(type != PKT_KEEPALIVE) connp->inactive=0;
		result = (*receive_tbl[type])(ind);
		if (connp->state == CONN_PLAYING)
		{
			connp->start = turn;
		}
		if (result == -1)
			return;

		// We didn't have enough energy to execute an important command.
		if (result == 0) 
		{
			/* Hack -- if we tried to do something while resting, wake us up.
			 */
			if (p_ptr->resting) disturb(player, 0, 0);

			/* If we didn't have enough energy to execute this
			 * command, in order to ensure that our important
			 * commands execute in the proper order, stop
			 * processing any commands that require energy. We
			 * assume that any commands that don't require energy
			 * (such as quitting, or talking) should be executed
			 * ASAP.
			 */
			/* Mega-Hack -- save our old energy and set our energy
			 * to 0.  This will allow us to execute "out of game"
			 * actions such as talking while we wait for enough
			 * energy to execute our next queued in game action.
			 */
			if (p_ptr->energy) 
			{
				old_energy = p_ptr->energy;
				p_ptr->energy = 0;
			}
		}
		//{
			//Sockbuf_clear(&connp->r);
		//}
		/*
		if (result == 2)
			return;
		*/
	}
	/* Restore our energy if neccecary. */

	/* Make sure that the player structure hasn't been deallocated in this
	 * time due to a quit request.  Hack -- to do this we check if the number
	 * of players has changed while this loop has been executing.  This would be
	 * a BAD thing to do if we ever went multithreaded.
	 */
	if (NumPlayers == num_players_start)
		if (!p_ptr->energy) p_ptr->energy = old_energy;
}

/*
 * Process a client packet.
 * The client may be in one of several states,
 * therefore we use function dispatch tables for easy processing.
 * Some functions may process requests from clients being
 * in different states.
 * The behavior of this function has been changed somewhat.  New commands are now
 * put into a command queue, where they will be executed later.
 */
void Handle_input(int fd, int arg)
{
	int ind = arg, player = -1, old_numplayers = NumPlayers;
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;
	//int type, result, (**receive_tbl)(int ind);
	int (**receive_tbl)(int ind);

	if (connp->state & (CONN_PLAYING | CONN_READY))
		receive_tbl = &playing_receive[0];
	else if (connp->state & (CONN_LOGIN/* | CONN_SETUP */))
#if 0
		receive_tbl = &login_receive[0];
#else
	{
		receive_tbl = &login_receive[0];
		Receive_play(ind);
		return;
	}
#endif	// 0
	else if (connp->state & (CONN_DRAIN/* | CONN_SETUP */))
		receive_tbl = &drain_receive[0];
	else if (connp->state == CONN_LISTENING)
	{
		Handle_listening(ind);
		return;
	}
	else if (connp->state == CONN_SETUP)
	{
		Handle_setup(ind);
		return;
	}
	else {
		if (connp->state != CONN_FREE)
			Destroy_connection(ind, "not input");
		return;
	}

#if 0
	/* Clear connp->r, which will be our new queue */
	Sockbuf_clear(&connp->r);
	
	/* Put any old commands at the beginning of the new queue we are reading into */
	if (connp->q.len > 0)
	{
		if (connp->r.ptr > connp->r.buf)
			Sockbuf_advance(&connp->r, connp->r.ptr - connp->r.buf);
		if (Sockbuf_write(&connp->r, connp->q.ptr, connp->q.len) != connp->q.len)
		{
			errno = 0;
			Destroy_connection(ind, "Can't copy queued data to buffer");
			return;
		}

		connp->q.ptr += connp->q.len;
		Sockbuf_advance(&connp->q, connp->q.ptr - connp->q.buf);
	}
	Sockbuf_clear(&connp->q);
#endif

	
	/* Mega-Hack */
	if (connp->id != -1 && Players[GetInd[connp->id]] && Players[GetInd[connp->id]]->new_level_flag) return;

	// Reset the buffer we are reading into
	Sockbuf_clear(&connp->r);

	// Read in the data
	if (Sockbuf_read(&connp->r) <= 0)
	{
		// Check to make sure that an EAGAIN error didn't occur.  Sometimes on
		// Linux when receiving a lot of traffic EAGAIN will occur on recv and
		// Sockbuf_read will return 0.
		if (errno != EAGAIN)
		{
			// If this happens, the the client has probably closed his TCP connection.
			do_quit(ind, 0);
		}
		
		//Destroy_connection(ind, "input error");
		return;
	}

	// Add this new data to the command queue
	if (Sockbuf_write(&connp->q, connp->r.ptr, connp->r.len) != connp->r.len)
	{
		errno = 0;
		Destroy_connection(ind, "Can't copy queued data to buffer");
		return;
	}

	// Execute any new commands immediatly if possobile
	process_pending_commands(ind);

	/* Experimental hack -- to reduce perceived latency, flush our network
	 * info right now so the player sees the results of his actions as soon
	 * as possobile.  Everyone else will see him move at most one game turn
	 * later, which is usually < 100 ms.
	 */

	/* Hack -- don't update the player info if the number of players since
	 * the beginning of this function call has changed, which might indicate
	 * that our player has left the game.
	 */
	if ((old_numplayers == NumPlayers) && (connp->state == CONN_PLAYING))
	{
		// Update the players display if neccecary and possobile
		if (connp->id != -1)
		{
			player = GetInd[connp->id];
			p_ptr = Players[player];

			/* Notice stuff */
			if (p_ptr->notice) notice_stuff(player);

			/* Update stuff */
			if (p_ptr->update) update_stuff(player);

			/* Redraw stuff */
			if (p_ptr->redraw) redraw_stuff(player);

			/* Window stuff */
			if (p_ptr->window) window_stuff(player);
		}
	}
	if (connp->c.len > 0)
	{
	/* evileye - p_ptr can be undefined (!!!) */
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (Packet_printf(&connp->c, "%c", PKT_END) <= 0)
		{
			Destroy_connection(p_ptr->conn, "write error");
			return;
		}
		Send_reliable(p_ptr->conn);
	}

//	Sockbuf_clear(&connp->r);
}

// This function is used for sending data to clients who do not yet have
// Player structures allocated, and for timing out players who have been
// idle for a while.
int Net_input(void)
{
	int i, ind, num_reliable = 0, input_reliable[MAX_SELECT_FD];
	connection_t *connp;
	char msg[MSG_LEN];

	for (i = 0; i < max_connections; i++)
	{
		connp = &Conn[i];

		if (connp->state == CONN_FREE)
			continue;
		if (connp->start + connp->timeout * cfg.fps < turn)
		{
			if (connp->state & (CONN_PLAYING | CONN_READY))
			{
				sprintf(msg, "%s mysteriously disappeared!",
					connp->nick);
				/*Set_message(msg);*/
			}
			sprintf(msg, "timeout %02x", connp->state);
			Destroy_connection(i, msg);
			continue;
		}

		// Make sure that the player we are looking at is not already in the
		// game.  If he is already in the game then we will send him data
		// in the function Net_input.
		if (connp->id != -1) continue;

/*		if (connp->r.len > 0)
			Sockbuf_clear(&connp->r); */

#if 0
		if (connp->state != CONN_PLAYING)
		{
#endif
			input_reliable[num_reliable++] = i;
			if (connp->state == CONN_SETUP )
			{
				Handle_setup(i);
				continue;
			}
#if 0
		}
#endif
	}

	for (i = 0; i < num_reliable; i++)
	{
		ind = input_reliable[i];
		connp = &Conn[ind];
		if (connp->state & (CONN_DRAIN | CONN_READY | CONN_SETUP
			| CONN_LOGIN | CONN_PLAYING))
		{
			if (connp->c.len > 0)
				if (Send_reliable(ind) == -1)
					continue;
		}
	}

	if (num_logins | num_logouts)
		num_logins = num_logouts = 0;

	return login_in_progress;
}

int Net_output(void)
{
	int	i;
	connection_t *connp;
	player_type *p_ptr = NULL;

	for (i = 1; i <= NumPlayers; i++)
	{
		p_ptr = Players[i];

		if (p_ptr->conn == NOT_CONNECTED) continue;

		if (p_ptr->new_level_flag) continue;

		connp = &Conn[p_ptr->conn];

		/* XXX XXX XXX Mega-Hack -- Redraw player's spot every time */
		/* This keeps the network connection "active" even if nothing is */
		/* happening -- KLJ */
		/* This has been changed to happen less often, operating at close to 3 
		 * times a second to keep the BGP routing tables happy.  
		 * I had originally changed this to about once every 2 seconds, 
		 * but apparently it was doing bad things, as the inactivity in the UDP
		 * stream was causing us to loose priority in the routing tables.
		 * Thanks to Crimson for explaining this. -APD
		 */
		
		/* to keep a good UDP connection, send data that requests a response
		 * every 1/4 of a second
		 */

		/* Hack -- add the index to our turn, so we don't send all the players
		 * reliable data simultaniously.  This should hopefully "spread out"
		 * the incoming data a little so it doesn't all happen in a semi-
		 * synchronized way.
		 */

		/* 	
		   Can't coment this out, it updates things like food
		    and stores.   
		   Still have to worry about routing with TCP :)
		   -- Crimson
		   --- But STill buggy, so off for now.
		if (!((turn + i) % 8))
		{
			lite_spot(i, p_ptr->py, p_ptr->px); 
		}
		*/

		/* otherwise, send normal data if there is any */
		//else 
		//{
	//		  Tell the client that this is the end  
	//		  
	//		If we have any data to send to the client, terminate it
	//		and send it to the client.
			if (connp->c.len > 0)
			{
				if (Packet_printf(&connp->c, "%c", PKT_END) <= 0)
				{
					Destroy_connection(p_ptr->conn, "write error");
					continue;
				}
				Send_reliable(p_ptr->conn);
			}
			// Flush the output buffers 
		//	if (Sockbuf_flush(&connp->w) == -1)
		//		return -1;
		//}

		//Sockbuf_clear(&connp->w);
	}

	/* Every fifteen seconds, update the info sent to the metaserver */
	if (!(turn % (15 * cfg.fps)))
		Report_to_meta(META_UPDATE);

	return 1;
}

/*
 * Send a reply to a special client request.
 * Not used consistently everywhere.
 * It could be used to setup some form of reliable
 * communication from the client to the server.
 */
int Send_reply(int ind, int replyto, int result)
{
	connection_t *connp = &Conn[ind];
	int n;

	n = Packet_printf(&connp->c, "%c%c%c", PKT_REPLY, replyto, result);
	if (n == -1)
	{
		Destroy_connection(ind, "write error");
		return -1;
	}

	return n;
}

int Send_leave(int ind, int id)
{
	connection_t *connp = &Conn[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for leave info (%d,%d)",
			connp->state, connp->id));
		return 0;
	}
	return Packet_printf(&connp->c, "%c%hd", PKT_LEAVE, id);
}

// Actually quit. This was seperated as a hack to allow us to
// "quit" when a quit packet has not been received, such as when
// our TCP connection is severed.  The tellclient argument
// specifies whether or not we should try to send data to the
// client informing it about the quit event.
void do_quit(int ind, bool tellclient)
{
	int player = -1;
	player_type *p_ptr = NULL;
	connection_t * connp = &Conn[ind];

	if (connp->id != -1) 
	{
		player = GetInd[connp->id];
		p_ptr=Players[player];
	}
	if (!tellclient)
	{
		/* Close the socket */
		close(connp->w.sock);

		/* No more packets from a player who is quitting */
		remove_input(connp->w.sock);

		/* Disable all output and input to and from this player */
		connp->w.sock = -1;
	}

	/* If we are close to the center of town, exit quickly. */
	if(connp->id==-1 || istown(&p_ptr->wpos) || (p_ptr->wpos.wz==0 && wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].radius<3))
	{
		Destroy_connection(ind, "client quit");
	}
	// Otherwise wait for the timeout
}


static int Receive_quit(int ind)
{
	int player = -1, n;
	connection_t *connp = &Conn[ind];
	char ch;

	if (connp->id != -1) 
	{
		player = GetInd[connp->id];
	}
	if ((n = Packet_scanf(&connp->r, "%c", &ch)) != 1)
	{
		errno = 0;
		Destroy_connection(ind, "receive error in quit");
		return -1;
	}
	
	do_quit(ind, 0);
	
	return 1;
}

static int Receive_login(int ind){
	connection_t *connp=&Conn[ind];
	//unsigned char ch;
	int n;
	char choice[MAX_CHARS];
	struct account *l_acc;

	n = Sockbuf_read(&connp->r);

	/* if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &choice)) != 2) */
	if ((n = Packet_scanf(&connp->r, "%s", choice)) != 1)
	{
		errno = 0;
		printf("%d\n",n);
		plog("Failed reading login packet");
		Destroy_connection(ind, "receive error in login");
		return -1;
	}
	if(strlen(choice)==0){
		if(connp->pass && (l_acc=GetAccount(connp->nick, connp->pass))){
			int *id_list, i;
			free(connp->pass);
			connp->pass=NULL;
			n=player_id_list(&id_list, l_acc->id);
			/* Display all account characters here */
			for(i=0; i<n; i++){
				u16b ptype=lookup_player_type(id_list[i]);
				/* do not change protocol here */
				Packet_printf(&connp->c, "%c%s%hd%hd%hd", PKT_LOGIN, lookup_player_name(id_list[i]), lookup_player_level(id_list[i]), ptype&0xff , ptype>>8);
			}
			Packet_printf(&connp->c, "%c%s%hd%hd%hd", PKT_LOGIN, "", 0, 0, 0);
			C_KILL(id_list, i, int);
			KILL(l_acc, struct account);
		}
		else{
			/* fail login here */
			Destroy_connection(ind, "login failure");
		}
		Sockbuf_flush(&connp->w);
		return(0);
	}
	else{
		/* at this point, we are authorised as the owner
		   of the account. any valid name will be
		   allowed. */
		/* i realise it should return different value depending
		   on reason - evileye */
		if(check_account(connp->nick, choice)){
			/* Validate names/resume in proper place */
			if(Check_names(choice, connp->real, connp->host, connp->addr)){
				/* fail login here */
				Destroy_connection(ind, "Security violation");
				return(-1);
			}
			Packet_printf(&connp->c, "%c", lookup_player_id(choice) ? SUCCESS : E_NEED_INFO);
			connp->c_name=strdup(choice);
		}
		else{
			/* fail login here */
			Destroy_connection(ind, "Name already owned");
			return(-1);
		}
	}
	if (connp->setup >= Setup.setup_size)
		Conn_set_state(connp, CONN_LOGIN, CONN_LOGIN);
	return(0);
}

#define RECEIVE_PLAY_SIZE (2*6+OPT_MAX+2*(TV_MAX+MAX_F_IDX+MAX_K_IDX+MAX_R_IDX))
//#define STRICT_RECEIVE_PLAY
static int Receive_play(int ind)
{
	connection_t *connp = &Conn[ind];
	unsigned char ch;
	int i, n;
	s16b sex, race, class;

	/* XXX */
	n = Sockbuf_read(&connp->r);

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) != 1)
	{
		errno = 0;
		plog("Cannot receive play packet");
		Destroy_connection(ind, "receive error in play");
		return -1;
	}

	/* Do not tell me how much this sucks. I didn't do the design
	   evileye */
	if (ch == PKT_LOGIN){
		Receive_login(ind);
		return(0);
	}
	if (ch != PKT_PLAY)
	{
	  //		errno = 0;
#if DEBUG_LEVEL > 1
#if DEBUG_LEVEL < 3
		if (ch != PKT_KEEPALIVE)
#endif	// DEBUG_LEVEL(2)
			plog(format("Packet is not of play type (%d)", ch));
#endif	// DEBUG_LEVEL(1)
	  //Destroy_connection(ind, "not play");
	  //return -1;
	  return 0;
	}
//	else
	{
		if ((n = Packet_scanf(&connp->r, "%hd%hd%hd", &sex, &race, &class)) <= 0)
		{
			errno = 0;
			plog("Play packet is broken");
			Destroy_connection(ind, "receive error 2 in play");
			return -1;
		}

		/* Set his character info */
		connp->sex = sex;
		connp->race = race;
		connp->class = class;

//		if (2654 > connp->r.len - (connp->r.ptr - connp->r.buf))
		if (RECEIVE_PLAY_SIZE > connp->r.len - (connp->r.ptr - connp->r.buf))
		{
#if DEBUG_LEVEL > 2
			plog(format("Play packet is not large enough yet (%d)",
						connp->r.len - (connp->r.ptr - connp->r.buf)));
#endif	// DEBUG_LEVEL
			connp->r.ptr = connp->r.buf;
			return 1;
		}

#if DEBUG_LEVEL > 2
			plog(format("Play packet is now large enough (%d)",
						connp->r.len - (connp->r.ptr - connp->r.buf)));
#endif	// DEBUG_LEVEL

#if 1	// moved from Handle_listening
		/* Read the stat order */
		for (i = 0; i < 6; i++)
		{
			n = Packet_scanf(&connp->r, "%hd", &connp->stat_order[i]);

			if (n <= 0)
			{
				Destroy_connection(ind, "Misread stat order");
				return -1;
			}
		}

#if 0
		/* Read class extra */	
		n = Packet_scanf(&connp->r, "%hd", &connp->class_extra);

		if (n <= 0)
		{
			Destroy_connection(ind, "Misread class extra");
			return -1;
		}
#endif	// 0

		/* Read the options */
		for (i = 0; i < OPT_MAX; i++)
		{
			n = Packet_scanf(&connp->r, "%c", &connp->Client_setup.options[i]);

			if (n <= 0)
			{
				Destroy_connection(ind, "Misread options");
				return -1;
			}
		}

		/* Read the "unknown" char/attrs */
		for (i = 0; i < TV_MAX; i++)
		{
			n = Packet_scanf(&connp->r, "%c%c", &connp->Client_setup.u_attr[i], &connp->Client_setup.u_char[i]);

			if (n <= 0)
			{
#ifdef STRICT_RECEIVE_PLAY
				Destroy_connection(ind, "Misread unknown redefinitions");
				return -1;
#else
				break;
#endif
			}
		}

		/* Read the "feature" char/attrs */
		for (i = 0; i < MAX_F_IDX; i++)
		{
			n = Packet_scanf(&connp->r, "%c%c", &connp->Client_setup.f_attr[i], &connp->Client_setup.f_char[i]);

			if (n <= 0)
			{
#ifdef STRICT_RECEIVE_PLAY
				Destroy_connection(ind, "Misread feature redefinitions");
				return -1;
#else
				break;
#endif
			}
		}

		/* Read the "object" char/attrs */
		for (i = 0; i < MAX_K_IDX; i++)
		{
			n = Packet_scanf(&connp->r, "%c%c", &connp->Client_setup.k_attr[i], &connp->Client_setup.k_char[i]);

			if (n <= 0)
			{
#ifdef STRICT_RECEIVE_PLAY
				Destroy_connection(ind, "Misread object redefinitions");
				return -1;
#else
				break;
#endif
			}
		}

		/* Read the "monster" char/attrs */
		for (i = 0; i < MAX_R_IDX; i++)
		{
			n = Packet_scanf(&connp->r, "%c%c", &connp->Client_setup.r_attr[i], &connp->Client_setup.r_char[i]);

			if (n <= 0)
			{
#ifdef STRICT_RECEIVE_PLAY
				Destroy_connection(ind, "Misread monster redefinitions");
				return -1;
#else
				break;
#endif
			}
		}
#endif	// 0
	}
	if (connp->state != CONN_LOGIN)
	{
		if (connp->state != CONN_PLAYING)
		{
			if (connp->state == CONN_READY)
			{
				connp->r.ptr = connp->r.buf + connp->r.len;
				return 0;
			}
			errno = 0;
			plog(format("Connection not in login state (%02x)", connp->state));
			Destroy_connection(ind, "not login");
			return -1;
		}
		//if (Send_reliable_old(ind) == -1)
		if (Send_reliable(ind) == -1)
			return -1;
		return 0;
	}

	Sockbuf_clear(&connp->w);
	if (Handle_login(ind) == -1)
	{
		/* The connection has already been destroyed */
		return -1;
	}

	return 2;
}

/* Head of file transfer system receive */
/* DO NOT TOUCH - work in progress */
static int Receive_file(int ind){
	char command, ch;
	char fname[30];	/* possible filename */
	int x;	/* return value/ack */
	unsigned short fnum;	/* unique SENDER side file number */
	unsigned short len;
	unsigned long csum;
	int n;
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;
	int Ind;

	Ind=GetInd[connp->id];
	p_ptr=Players[Ind];
	n=Packet_scanf(&connp->r, "%c%c%hd", &ch, &command, &fnum);
	if(n==3){
		switch(command){
			case PKT_FILE_INIT:
				/* Admin to do this only !!! */
				Packet_scanf(&connp->r, "%s", fname);
				if(!p_ptr->admin_dm && !p_ptr->admin_wiz){
					msg_print(Ind, "\377rFile transfer refused");
					x=0; 
				}
				else{
					msg_print(Ind, "\377gAttempting file transfer");
					x=local_file_init(ind, fnum, fname);
				}
				break;
			case PKT_FILE_DATA:
				Packet_scanf(&connp->r, "%hd", &len);
				x=local_file_write(ind, fnum, len);
				break;
			case PKT_FILE_END:
				x=local_file_close(ind, fnum);
				msg_format(Ind, "\377oFile transfer %s.", x? "successful":"failed");
				break;
			case PKT_FILE_CHECK:
				/* Admin to do this only !!! */
				Packet_scanf(&connp->r, "%s", fname);
				if(!p_ptr->admin_dm && !p_ptr->admin_wiz){
					msg_print(Ind, "\377rFile check refused");
					x=0; 
				}
				else{
					msg_print(Ind, "\377yChecking file");
					x=local_file_check(fname, &csum);
					Packet_printf(&connp->w, "%c%c%hd%ld", PKT_FILE, PKT_FILE_SUM, fnum, csum);
					return(1);
				}
				break;
			case PKT_FILE_SUM:
				Packet_scanf(&connp->r, "%ld", &csum);
				check_return(ind, fnum, csum);
				return(1);
				break;
			case PKT_FILE_ACK:
				local_file_ack(ind, fnum);
				return(1);
				break;
			case PKT_FILE_ERR:
				local_file_err(ind, fnum);
				/* continue the send/terminate */
				return(1);
				break;
			default:
				printf("unknown file transfer packet\n");
				x=0;
		}
		Packet_printf(&connp->c, "%c%c%hd", PKT_FILE, x?PKT_FILE_ACK:PKT_FILE_ERR, fnum);
	}
	return(1);
}

int Receive_file_data(int ind, unsigned short len, char *buffer){
	connection_t *connp = &Conn[ind];
	memcpy(buffer, connp->r.ptr, len);
	connp->r.ptr+=len;
	return(1);
}

int Send_file_check(int ind, unsigned short id, char *fname){
	connection_t *connp = &Conn[ind];
	Packet_printf(&connp->c, "%c%c%hd%s", PKT_FILE, PKT_FILE_CHECK, id, fname);
	return(1);
}

int Send_file_init(int ind, unsigned short id, char *fname){
	connection_t *connp = &Conn[ind];
	Packet_printf(&connp->c, "%c%c%hd%s", PKT_FILE, PKT_FILE_INIT, id, fname);
	return(1);
}

int Send_file_data(int ind, unsigned short id, char *buf, unsigned short len){
	connection_t *connp = &Conn[ind];
	Packet_printf(&connp->c, "%c%c%hd%hd", PKT_FILE, PKT_FILE_DATA, id, len);
	if (Sockbuf_write(&connp->c, buf, len) != len){
		printf("failed sending file data\n");
	}
	return(1);
}

int Send_file_end(int ind, unsigned short id){
	connection_t *connp = &Conn[ind];
	Packet_printf(&connp->c, "%c%c%hd", PKT_FILE, PKT_FILE_END, id);
	return(1);
}

int Send_reliable(int ind)
{
	connection_t *connp = &Conn[ind];
	int num_written;

	/* Hack -- make sure we have a valid socket to write to.  -1 is used to
	 * specify a player that has disconnected but is still "in game".
	 */
	if (connp->w.sock == -1) return 0;

	if (Sockbuf_write(&connp->w, connp->c.buf, connp->c.len) != connp->c.len)
	{
		plog("Cannot write reliable data");
		Destroy_connection(ind, "write error");
		return -1;
	}
	if ((num_written = Sockbuf_flush(&connp->w)) < connp->w.len)
	{
		plog(format("Cannot flush reliable data (%d)", num_written));
		Destroy_connection(ind, "flush error");
		return -1;
	}
	Sockbuf_clear(&connp->c);
	return num_written;
}

int Send_reliable_old(int ind)
{
	connection_t *connp = &Conn[ind];
	char *read_buf;
	int i, n, len, todo, max_todo;
	long rel_off;
	const int max_packet_size = MAX_RELIABLE_DATA_PACKET_SIZE,
		min_send_size = 1;

	if (connp->c.len <=0 || connp->last_send_loops == turn)
	{
		connp->last_send_loops = turn;
		return 0;
	}
	read_buf = connp->c.buf;
	max_todo = connp->c.len;
	rel_off = connp->reliable_offset;
	if (connp->w.len > 0)
	{
		if (connp->w.len >= max_packet_size - min_send_size)
		{
			return 0;
		}
		if (max_todo > max_packet_size - connp->w.len)
			max_todo = max_packet_size - connp->w.len;
	}
	if (connp->retransmit_at_loop > turn)
	{
		if (max_todo <= connp->reliable_unsent - connp->reliable_offset
			+ min_send_size || connp->w.len == 0)
			return 0;
	}
	else if (connp->retransmit_at_loop != 0)
		connp->acks >>= 1;

	todo = max_todo;

	for (i = 0; i <= connp->acks && todo > 0; i++)
	{
		len = (todo > max_packet_size) ? max_packet_size : todo;
		if (Packet_printf(&connp->w, "%c%hd%ld%ld%ld", PKT_RELIABLE,
			len, rel_off, turn, max_todo) <= 0
			|| Sockbuf_write(&connp->w, read_buf, len) != len)
		{
			plog("Cannot write reliable data");
			Destroy_connection(ind, "write error");
			return -1;
		}

		if ((n = Sockbuf_flush(&connp->w)) < len)
		{
			if (n == 0 && (errno == EWOULDBLOCK
				|| errno == EAGAIN))
			{
				connp->acks = 0;
				break;
			}
			else
			{
				plog(format("Cannot flush reliable data (%d)", n));
				Destroy_connection(ind, "flush error");
				return -1;
			}
		}

		todo -= len;
		rel_off += len;
		read_buf += len;
	}

	Sockbuf_clear(&connp->w);

	connp->last_send_loops = turn;

	if (max_todo - todo <= 0)
		return 0;

	if (connp->rtt_retransmit > MAX_RETRANSMIT)
		connp->rtt_retransmit = MAX_RETRANSMIT;
	if (connp->retransmit_at_loop <= turn)
	{
		connp->retransmit_at_loop = turn + connp->rtt_retransmit;
		connp->rtt_retransmit <<= 1;
		connp->rtt_timeouts++;
	}
	else connp->retransmit_at_loop = turn + connp->rtt_retransmit;

	if (rel_off > connp->reliable_unsent)
		connp->reliable_unsent = rel_off;

	return (max_todo - todo);
}

static int Receive_ack(int ind)
{
	connection_t *connp = &Conn[ind];
	int n;
	unsigned char ch;
	long rel, rtt, diff, delta, rel_loops;

	if ((n = Packet_scanf(&connp->r, "%c%ld%ld", &ch, &rel, &rel_loops))
		<= 0)
	{
		errno = 0;
		plog(format("Cannot read ack packet (%d)", n));
		Destroy_connection(ind, "read error");
		return -1;
	}
	if (ch != PKT_ACK)
	{
		errno = 0;
		plog(format("Not an ack packet (%d)", ch));
		Destroy_connection(ind, "not ack");
		return -1;
	}
	rtt = turn - rel_loops;
	if (rtt > 0 && rtt <= MAX_RTT)
	{
		if (connp->rtt_smoothed == 0)
			connp->rtt_smoothed = rtt << 3;
		delta = rtt - (connp->rtt_smoothed >> 3);
		connp->rtt_smoothed += delta;
		if (delta < 0)
			delta = -delta;
		connp->rtt_dev += delta - (connp->rtt_dev >> 2);
		connp->rtt_retransmit = ((connp->rtt_smoothed >> 2)
			+ connp->rtt_dev) >> 1;
		if (connp->rtt_retransmit < MIN_RETRANSMIT)
			connp->rtt_retransmit = MIN_RETRANSMIT;
	}
	diff = rel - connp->reliable_offset;
	if (diff > connp->c.len)
	{
		errno = 0;
		plog(format("Bad ack (diff=%ld,cru=%ld,c=%ld,len=%d)",
			diff, rel, connp->reliable_offset, connp->c.len));
		Destroy_connection(ind, "bad ack");
		return -1;
	}
	else if (diff <= 0)
		return 1;
	Sockbuf_advance(&connp->c, (int) diff);
	connp->reliable_offset += diff;
	if ((n = ((diff + 512 - 1) / 512)) > connp->acks)
		connp->acks = n;
	else
		connp->acks++;
	if (connp->reliable_offset >= connp->reliable_unsent)
	{
		connp->retransmit_at_loop = 0;
		if (connp->state == CONN_DRAIN)
			Conn_set_state(connp, connp->drain_state, connp->drain_state);
	}
	if (connp->state == CONN_READY
		&& (connp->c.len <= 0
		|| (connp->c.buf[0] != PKT_REPLY
			&& connp->c.buf[0] != PKT_PLAY
			&& connp->c.buf[0] != PKT_SUCCESS
			&& connp->c.buf[0] != PKT_FAILURE)))
		Conn_set_state(connp, connp->drain_state, connp->drain_state);
	
	connp->rtt_timeouts = 0;

	/*printf("Received ack to data sent at %ld.\n", rel_loops);*/

	return 1;
}
 
static int Receive_discard(int ind)
{
	connection_t *connp = &Conn[ind];

	errno = 0;
	plog(format("Discarding packet %d while in state %02x",
		connp->r.ptr[0], connp->state));
	connp->r.ptr = connp->r.buf + connp->r.len;

	return 0;
}

static int Receive_undefined(int ind)
{
	connection_t *connp = &Conn[ind];

	errno = 0;
	plog(format("Unknown packet type (%d,%02x)", connp->r.ptr[0], connp->state));
	/* Dont destroy connection. Ignore the invalid packet */
	/*Destroy_connection(ind, "undefined packet");*/
	return -1;	/* Crash if not (evil) */
	/*return 0;*/
}

int Send_plusses(int ind, int tohit, int todam, int hr, int dr, int hm, int dm)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for plusses (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%hd%hd%hd%hd%hd%hd", PKT_PLUSSES, tohit, todam, hr, dr, hm, dm);
	      }
	  }

        return Packet_printf(&connp->c, "%c%hd%hd%hd%hd%hd%hd", PKT_PLUSSES, tohit, todam, hr, dr, hm, dm);
}


int Send_ac(int ind, int base, int plus)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for ac (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%hd%hd", PKT_AC, base, plus);
	      }
	  }
	return Packet_printf(&connp->c, "%c%hd%hd", PKT_AC, base, plus);
}

int Send_experience(int ind, int lev, s32b max, s32b cur, s32b adv)
{
	connection_t *connp = &Conn[Players[ind]->conn];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for experience (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	return Packet_printf(&connp->c, "%c%hu%d%d%d", PKT_EXPERIENCE, lev, 
			max, cur, adv);
}

#if 0
int Send_skill_init(int ind, int type, int i)
#else
int Send_skill_init(int ind, u16b i)
#endif
{
	connection_t *connp = &Conn[Players[ind]->conn];

	char *tmp;
	
	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for skill init (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

#if 0
	/* Why sending thrice..? */
#if 0
	if (type == PKT_SKILL_INIT_NAME)
		return Packet_printf(&connp->c, "%c%ld%ld%ld%ld%ld%S%d%c", PKT_SKILL_INIT, type, i, s_info[i].father, s_info[i].order, s_info[i].action_mkey, s_info[i].name, s_info[i].flags1, s_info[i].tval);
	else if (type == PKT_SKILL_INIT_DESC)
		return Packet_printf(&connp->c, "%c%ld%ld%ld%ld%ld%S%d%c", PKT_SKILL_INIT, type, i, s_info[i].father, s_info[i].order, s_info[i].action_mkey, s_info[i].desc, s_info[i].flags1, s_info[i].tval);
	else if (type == PKT_SKILL_INIT_MKEY)
		return Packet_printf(&connp->c, "%c%ld%ld%ld%ld%ld%S%d%c", PKT_SKILL_INIT, type, i, s_info[i].father, s_info[i].order, s_info[i].action_mkey, s_info[i].action_desc, s_info[i].flags1, s_info[i].tval);
#else	// 0
	if (type == PKT_SKILL_INIT_NAME)
		return Packet_printf(&connp->c, "%c%ld%ld%ld%ld%ld%S%d%c", PKT_SKILL_INIT, type, i, s_info[i].father, s_info[i].order, s_info[i].action_mkey, s_name + s_info[i].name, s_info[i].flags1, s_info[i].tval);
	else if (type == PKT_SKILL_INIT_DESC)
		return Packet_printf(&connp->c, "%c%ld%ld%ld%ld%ld%S%d%c", PKT_SKILL_INIT, type, i, s_info[i].father, s_info[i].order, s_info[i].action_mkey, s_text + s_info[i].desc, s_info[i].flags1, s_info[i].tval);
	else if (type == PKT_SKILL_INIT_MKEY)
		return Packet_printf(&connp->c, "%c%ld%ld%ld%ld%ld%S%d%c", PKT_SKILL_INIT, type, i, s_info[i].father, s_info[i].order, s_info[i].action_mkey, s_text + s_info[i].action_desc, s_info[i].flags1, s_info[i].tval);
#endif	// 0
#endif /* evileye - 0 */

	tmp=s_info[i].action_desc ? s_text + s_info[i].action_desc : "";

	/* Note: %hd is 2 bytes - use this for x16b.
	   We can use %c for bytes. */
	return( Packet_printf(&connp->c, "%c%hd%hd%hd%hd%ld%c%S%S%S",
		PKT_SKILL_INIT, i,
		s_info[i].father, s_info[i].order, s_info[i].action_mkey,
		s_info[i].flags1, s_info[i].tval, s_name+s_info[i].name,
		s_text+s_info[i].desc, tmp ? tmp : "" ) );

}

int Send_skill_info(int ind, int i)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for skill mod (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
        return Packet_printf(&connp->c, "%c%ld%ld%ld%ld%ld%ld", PKT_SKILL_MOD, p_ptr->skill_points, i, p_ptr->s_info[i].value, p_ptr->s_info[i].mod, p_ptr->s_info[i].dev, p_ptr->s_info[i].hidden);
}

int Send_gold(int ind, s32b au, s32b balance)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for gold (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%d%d", PKT_GOLD, au, balance);
	      }
	  }
	return Packet_printf(&connp->c, "%c%d%d", PKT_GOLD, au, balance);
}

#if 0	// well, it's easily cracked by client
int Send_sanity(int ind, int msane, int csane)
{
#ifdef SHOW_SANITY
	connection_t *connp = &Conn[Players[ind]->conn];

	player_type *p_ptr = Players[ind];
//	printf("sanity send!\n");

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for hp (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%hd%hd", PKT_SANITY, msane, csane);
	      }
	  }
	return Packet_printf(&connp->c, "%c%hd%hd", PKT_SANITY, msane, csane);
#endif	// SHOW_SANITY
}
#else	// 0
int Send_sanity(int ind, byte attr, cptr msg)
{
#ifdef SHOW_SANITY
	connection_t *connp = &Conn[Players[ind]->conn];

	player_type *p_ptr = Players[ind];
//	printf("sanity send!\n");

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for hp (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%c%s", PKT_SANITY, attr, msg);
	      }
	  }
	return Packet_printf(&connp->c, "%c%c%s", PKT_SANITY, attr, msg);
#endif	// SHOW_SANITY
}
#endif	// 0

int Send_hp(int ind, int mhp, int chp)
{
	connection_t *connp = &Conn[Players[ind]->conn];

	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for hp (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%hd%hd", PKT_HP, mhp, chp);
	      }
	  }
	return Packet_printf(&connp->c, "%c%hd%hd", PKT_HP, mhp, chp);
}

int Send_sp(int ind, int msp, int csp)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for sp (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%hd%hd", PKT_SP, msp, csp);
	      }
	  }
	return Packet_printf(&connp->c, "%c%hd%hd", PKT_SP, msp, csp);
}

int Send_char_info(int ind, int race, int class, int sex)
{
	connection_t *connp = &Conn[Players[ind]->conn];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for char info (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	return Packet_printf(&connp->c, "%c%hd%hd%hd", PKT_CHAR_INFO, race, class, sex);
}

int Send_various(int ind, int hgt, int wgt, int age, int sc, cptr body)
{
	connection_t *connp = &Conn[Players[ind]->conn];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for various (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	return Packet_printf(&connp->c, "%c%hu%hu%hu%hu%s", PKT_VARIOUS, hgt, wgt, age, sc, body);
}

int Send_stat(int ind, int stat, int max, int cur, int s_ind)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for stat (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%c%hd%hd%hd", PKT_STAT, stat, max, cur, s_ind);
	      }
	  }

	return Packet_printf(&connp->c, "%c%c%hd%hd%hd", PKT_STAT, stat, max, cur, s_ind);
}

int Send_history(int ind, int line, cptr hist)
{
	connection_t *connp = &Conn[Players[ind]->conn];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for history (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	return Packet_printf(&connp->c, "%c%hu%s", PKT_HISTORY, line, hist);
}

int Send_inven(int ind, char pos, byte attr, int wgt, int amt, byte tval, byte sval, s16b pval, cptr name)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for inven (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%c%c%hu%hd%c%c%hd%s", PKT_INVEN, pos, attr, wgt, amt, tval, sval, pval, name);
	      }
	  }
	return Packet_printf(&connp->c, "%c%c%c%hu%hd%c%c%hd%s", PKT_INVEN, pos, attr, wgt, amt, tval, sval, pval, name);
}

//int Send_equip(int ind, char pos, byte attr, int wgt, byte tval, cptr name)
int Send_equip(int ind, char pos, byte attr, int wgt, int amt, byte tval, byte sval, s16b pval, cptr name)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for equip (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%c%c%hu%hd%c%c%hd%s", PKT_EQUIP, pos, attr, wgt, amt, tval, sval, pval, name);
	      }
	  }
	return Packet_printf(&connp->c, "%c%c%c%hu%hd%c%c%hd%s", PKT_EQUIP, pos, attr, wgt, amt, tval, sval, pval, name);
}

int Send_title(int ind, cptr title)
{
	connection_t *connp = &Conn[Players[ind]->conn];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for title (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	return Packet_printf(&connp->c, "%c%s", PKT_TITLE, title);
}

int Send_depth(int ind, struct worldpos *wpos)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];
	bool ville = istown(wpos);
	cptr desc = "";
	
	/* XXX this kinda thing should be done *before* calling Send_*
	 * in general, of course..	- Jir - */
	if (ville)
	{
		int i;
		for (i = 0; i < numtowns; i++)
		{
			if (town[i].x == wpos->wx && town[i].y == wpos->wy)
			{
				desc = town_profile[town[i].type].name;
				break;
			}
		}
	}

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for depth (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%hu%hu%hu%c%hu%s", PKT_DEPTH, wpos->wx, wpos->wy, wpos->wz, istown(wpos), p_ptr2->word_recall, desc);
	      }
	  }
	return Packet_printf(&connp->c, "%c%hu%hu%hu%c%hu%s", PKT_DEPTH, wpos->wx, wpos->wy, wpos->wz, istown(wpos), p_ptr->word_recall, desc);
}

int Send_food(int ind, int food)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for food (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%hu", PKT_FOOD, food);
	      }
	  }
	return Packet_printf(&connp->c, "%c%hu", PKT_FOOD, food);
}

int Send_blind(int ind, bool blind)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for blind (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%c", PKT_BLIND, blind);
	      }
	  }
	return Packet_printf(&connp->c, "%c%c", PKT_BLIND, blind);
}

int Send_confused(int ind, bool confused)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for confusion (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%c", PKT_CONFUSED, confused);
	      }
	  }
	return Packet_printf(&connp->c, "%c%c", PKT_CONFUSED, confused);
}

int Send_fear(int ind, bool fear)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for fear (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%c", PKT_FEAR, fear);
	      }
	  }
	return Packet_printf(&connp->c, "%c%c", PKT_FEAR, fear);
}

int Send_poison(int ind, bool poisoned)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for poison (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%c", PKT_POISON, poisoned);
	      }
	  }
	return Packet_printf(&connp->c, "%c%c", PKT_POISON, poisoned);
}

int Send_state(int ind, bool paralyzed, bool searching, bool resting)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for state (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%hu%hu%hu", PKT_STATE, paralyzed, searching, resting);
	      }
	  }
	return Packet_printf(&connp->c, "%c%hu%hu%hu", PKT_STATE, paralyzed, searching, resting);
}

int Send_speed(int ind, int speed)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for speed (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%hd", PKT_SPEED, speed);
	      }
	  }
	return Packet_printf(&connp->c, "%c%hd", PKT_SPEED, speed);
}

int Send_study(int ind, bool study)
{
	connection_t *connp = &Conn[Players[ind]->conn];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for study (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	return Packet_printf(&connp->c, "%c%c", PKT_STUDY, study);
}

int Send_cut(int ind, int cut)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for cut (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%hu", PKT_CUT, cut);
	      }
	  }
	return Packet_printf(&connp->c, "%c%hu", PKT_CUT, cut);
}

int Send_stun(int ind, int stun)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for stun (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%hu", PKT_STUN, stun);
	      }
	  }
	return Packet_printf(&connp->c, "%c%hu", PKT_STUN, stun);
}

int Send_direction(int ind)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for direction (%d.%d.%d)",
			ind, connp->state, connp->id));
		return FALSE;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		return Packet_printf(&connp2->c, "%c", PKT_DIRECTION);
	      }
	  }
	return Packet_printf(&connp->c, "%c", PKT_DIRECTION);
}

static bool hack_message = FALSE;

int Send_message(int ind, cptr msg)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];
	char buf[80];

	if (msg == NULL)
		return 1;

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for message (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	/* Clip end of msg if too long */
	strncpy(buf, msg, 78);
	buf[78] = '\0';

	if ((!hack_message) && p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2){
	      hack_message = TRUE;
	      end_mind(ind, TRUE);
	      hack_message = FALSE;
	    }
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		if (!BIT(connp2->state, CONN_PLAYING | CONN_READY))
		  {
		    plog(format("Connection not ready for message (%d.%d.%d)",
				ind, connp2->state, connp2->id));
		  }
		else Packet_printf(&connp2->c, "%c%s", PKT_MESSAGE, buf);
	      }
	  }
	return Packet_printf(&connp->c, "%c%s", PKT_MESSAGE, buf);
}

int Send_char(int ind, int x, int y, byte a, char c)
{
        player_type *p_ptr = Players[ind];

	if (!BIT(Conn[Players[ind]->conn].state, CONN_PLAYING | CONN_READY))
		return 0;

	if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_VIEW))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		if (BIT(Conn[Players[Ind2]->conn].state, CONN_PLAYING | CONN_READY))
		  {
		    Packet_printf(&Conn[Players[Ind2]->conn].c, "%c%c%c%c%c", PKT_CHAR, x, y, a, c);
		  }
	      }
	  }

	return Packet_printf(&Conn[Players[ind]->conn].c, "%c%c%c%c%c", PKT_CHAR, x, y, a, c);
}

int Send_spell_info(int ind, int realm, int book, int i, cptr out_val)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for spell info (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	return Packet_printf(&connp->c, "%c%ld%ld%ld%hu%hu%hu%s", PKT_SPELL_INFO, p_ptr->innate_spells[0], p_ptr->innate_spells[1], p_ptr->innate_spells[2], realm, book, i, out_val);
}


int Send_item_request(int ind)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for item request (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c", PKT_ITEM);
	      }
	  }
	return Packet_printf(&connp->c, "%c", PKT_ITEM);
}

int Send_flush(int ind)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	//player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for flush (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
#if 0
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c", PKT_FLUSH);
	      }
	  }
#endif
	return Packet_printf(&connp->c, "%c", PKT_FLUSH);
}

/*
 * As an attempt to lower bandwidth requirements, each line is run length
 * encoded.  Non-encoded grids are sent as normal, but if a grid is
 * repeated at least twice, then bit 0x40 of the attribute is set, and
 * the next byte contains the number of repetitions of the previous grid.
 */
int Send_line_info(int ind, int y)
{
	player_type *p_ptr = Players[ind], *p_ptr2 = NULL;
	connection_t *connp = &Conn[p_ptr->conn];
	int x, x1, n;
	char c;
	byte a;
	int Ind2 = 0;

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for line info (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_VIEW))
	  {
	    Ind2 = find_player(p_ptr->esp_link);

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
	      }
	  }
	
	/* Put a header on the packet */
	Packet_printf(&connp->c, "%c%hd", PKT_LINE_INFO, y);
	if (Ind2) Packet_printf(&Conn[p_ptr2->conn].c, "%c%hd", PKT_LINE_INFO, y);

	/* Each column */
	for (x = 0; x < 80; x++)
	{
		/* Obtain the char/attr pair */
		c = p_ptr->scr_info[y][x].c;
		a = p_ptr->scr_info[y][x].a;

		/* Start looking here */
		x1 = x + 1;

		/* Start with count of 1 */
		n = 1;

		/* Count repetitions of this grid */
		while (p_ptr->scr_info[y][x1].c == c &&
			p_ptr->scr_info[y][x1].a == a && x1 < 80)
		{
			/* Increment count and column */
			n++;
			x1++;
		}

		/* RLE if there at least 2 similar grids in a row */
		if (n >= 2)
		{
			/* Set bit 0x40 of a */
			a |= 0x40;

			/* Output the info */
			Packet_printf(&connp->c, "%c%c%c", c, a, n);
			if (Ind2) Packet_printf(&Conn[p_ptr2->conn].c, "%c%c%c", c, a, n);

			/* Start again after the run */
			x = x1 - 1;
		}
		else
		{
			/* Normal, single grid */
			Packet_printf(&connp->c, "%c%c", c, a); 
			if (Ind2) Packet_printf(&Conn[p_ptr2->conn].c, "%c%c", c, a);
		}
	}

	/* Hack -- Prevent buffer overruns by flushing after each line sent */
	/* Send_reliable(Players[ind]->conn); */
	
	return 1;
}

int Send_mini_map(int ind, int y)
{
	player_type *p_ptr = Players[ind];
	connection_t *connp = &Conn[p_ptr->conn];
	int x, x1, n;
	char c;
	byte a;
	int Ind2 = 0;
	player_type *p_ptr2 = NULL;
	connection_t *connp2 = NULL;

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for minimap (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_VIEW))
	  {
	    Ind2 = find_player(p_ptr->esp_link);

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];
	      }
	  }
	
	/* Packet header */
	Packet_printf(&connp->c, "%c%hd", PKT_MINI_MAP, y);
     	if (Ind2) Packet_printf(&connp2->c, "%c%hd", PKT_MINI_MAP, y);

	/* Each column */
	for (x = 0; x < 80; x++)
	{
		/* Obtain the char/attr pair */
		c = p_ptr->scr_info[y][x].c;
		a = p_ptr->scr_info[y][x].a;

		/* Start looking here */
		x1 = x + 1;

		/* Start with count of 1 */
		n = 1;

		/* Count repetitions of this grid */
		while (p_ptr->scr_info[y][x1].c == c &&
			p_ptr->scr_info[y][x1].a == a && x1 < 80)
		{
			/* Increment count and column */
			n++;
			x1++;
		}

		/* RLE if there at least 2 similar grids in a row */
		if (n >= 2)
		{
			/* Set bit 0x40 of a */
			a |= 0x40;

			/* Output the info */
			Packet_printf(&connp->c, "%c%c%c", c, a, n);
		     	if (Ind2) Packet_printf(&connp2->c, "%c%c%c", c, a, n);

			/* Start again after the run */
			x = x1 - 1;
		}
		else
		{
			/* Normal, single grid */
			Packet_printf(&connp->c, "%c%c", c, a); 
			if (Ind2) Packet_printf(&connp2->c, "%c%c", c, a); 
		}
	}

	/* Hack -- Prevent buffer overruns by flushing after each line sent */
	/* Send_reliable(Players[ind]->conn); */
	
	return 1;
}

//int Send_store(int ind, char pos, byte attr, int wgt, int number, int price, cptr name)
int Send_store(int ind, char pos, byte attr, int wgt, int number, int price, cptr name, char tval, char sval)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for store item (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_VIEW))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%c%c%hd%hd%d%s%c%c", PKT_STORE, pos, attr, wgt, number, price, name, tval, sval);
	      }
	  }

	return Packet_printf(&connp->c, "%c%c%c%hd%hd%d%s%c%c", PKT_STORE, pos, attr, wgt, number, price, name, tval, sval);
}

/*
 * This function is supposed to handle 'store actions' too,
 * like 'buy' 'identify' 'heal' 'bid to an auction'		- Jir -
 */
//int Send_store_info(int ind, int num, int owner, int items)
int Send_store_info(int ind, int num, cptr store, cptr owner, int items, int purse)
//int Send_store_info(int ind, int num, cptr store, cptr owner, int items, int purse, byte num_actions)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for store info (%d.%d.%d)",
					ind, connp->state, connp->id));
		return 0;
	}

	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_VIEW))
	{
		int Ind2 = find_player(p_ptr->esp_link);
		player_type *p_ptr2;
		connection_t *connp2;

		if (!Ind2)
			end_mind(ind, TRUE);
		else
		{
			p_ptr2 = Players[Ind2];
			connp2 = &Conn[p_ptr2->conn];

//			Packet_printf(&connp2->c, "%c%hd%hd%hd", PKT_STORE_INFO, num, owner, items);
//			Packet_printf(&connp2->c, "%c%hd%s%s%hd%d%c", PKT_STORE_INFO, num, store, owner, items, purse, num_actions);
			Packet_printf(&connp2->c, "%c%hd%s%s%hd%d", PKT_STORE_INFO, num, store, owner, items, purse);
		}
	}

	return Packet_printf(&connp->c, "%c%hd%s%s%hd%d", PKT_STORE_INFO, num, store, owner, items, purse);
//	return Packet_printf(&connp->c, "%c%hd%hd%hd", PKT_STORE_INFO, num, owner, items);
}

int Send_store_action(int ind, char pos, u16b bact, u16b action, cptr name, char attr, char letter, s16b cost, byte flag)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for store info (%d.%d.%d)",
					ind, connp->state, connp->id));
		return 0;
	}

	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_VIEW))
	{
		int Ind2 = find_player(p_ptr->esp_link);
		player_type *p_ptr2;
		connection_t *connp2;

		if (!Ind2)
			end_mind(ind, TRUE);
		else
		{
			p_ptr2 = Players[Ind2];
			connp2 = &Conn[p_ptr2->conn];

			Packet_printf(&connp2->c, "%c%c%hd%hd%s%c%c%hd%c", PKT_BACT, pos, bact, action, name, attr, letter, cost, flag);
		}
	}

	return Packet_printf(&connp->c, "%c%c%hd%hd%s%c%c%hd%c", PKT_BACT, pos, bact, action, name, attr, letter, cost, flag);
}

int Send_store_sell(int ind, int price)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for sell price (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_VIEW))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%d", PKT_SELL, price);
	      }
	  }

	return Packet_printf(&connp->c, "%c%d", PKT_SELL, price);
}

int Send_store_kick(int ind)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for store_kick (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_VIEW))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c", PKT_STORE_LEAVE);
	      }
	  }

	return Packet_printf(&connp->c, "%c", PKT_STORE_LEAVE);
}

int Send_target_info(int ind, int x, int y, cptr str)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	char buf[80];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for target info (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	/* Copy */
	strncpy(buf, str, 79);

	/* Paranoia -- Add null */
	buf[79] = '\0';

	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%c%c%s", PKT_TARGET_INFO, x, y, buf);
	      }
	  }
	return Packet_printf(&connp->c, "%c%c%c%s", PKT_TARGET_INFO, x, y, buf);
}

int Send_sound(int ind, int sound)
{
	connection_t *connp = &Conn[Players[ind]->conn];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for sound (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	return Packet_printf(&connp->c, "%c%c", PKT_SOUND, sound);
}

int Send_special_line(int ind, int max, int line, byte attr, cptr buf)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	char temp[80];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for special line (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	strncpy(temp, buf, 79);
	temp[79] = '\0';
	return Packet_printf(&connp->c, "%c%hd%hd%c%s", PKT_SPECIAL_LINE, max, line, attr, temp);
}

int Send_floor(int ind, char tval)
{
	connection_t *connp = &Conn[Players[ind]->conn];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for floor item (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	return Packet_printf(&connp->c, "%c%c", PKT_FLOOR, tval);
}

int Send_pickup_check(int ind, cptr buf)
{
	connection_t *connp = &Conn[Players[ind]->conn];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for pickup check (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	return Packet_printf(&connp->c, "%c%s", PKT_PICKUP_CHECK, buf);
}

/* adding ultimate quick and dirty hack here so geraldo can play his 19th lvl char
   with the 80 character party name...... 
   -APD-
*/

int Send_party(int ind)
{
	player_type *p_ptr = Players[ind];
	connection_t *connp = &Conn[p_ptr->conn];
	char buf[160];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection nor ready for party info (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	sprintf(buf, "Party: %s", parties[p_ptr->party].name);

	/* MEGA hack */
	
	buf[60] = 0;

	if (p_ptr->party > 0)
	{
		strcat(buf, "     Owner: ");
		strcat(buf, parties[p_ptr->party].owner);
	}

	return Packet_printf(&connp->c, "%c%s", PKT_PARTY, buf);
}

int Send_special_other(int ind)
{
	connection_t *connp = &Conn[Players[ind]->conn];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for special other (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	return Packet_printf(&connp->c, "%c", PKT_SPECIAL_OTHER);
}

int Send_skills(int ind)
{
	player_type *p_ptr = Players[ind];
	connection_t *connp = &Conn[p_ptr->conn];
	s16b skills[12];
	int i, tmp;
	object_type *o_ptr;

	/* Fighting skill */
	o_ptr = &p_ptr->inventory[INVEN_WIELD];
	tmp = p_ptr->to_h + o_ptr->to_h + p_ptr->to_h_melee;
	skills[0] = p_ptr->skill_thn + (tmp * BTH_PLUS_ADJ);

	/* Shooting skill */
	o_ptr = &p_ptr->inventory[INVEN_BOW];
	tmp = p_ptr->to_h + o_ptr->to_h + p_ptr->to_h_ranged;
	skills[1] = p_ptr->skill_thb + (tmp * BTH_PLUS_ADJ);

	/* Basic abilities */
	skills[2] = p_ptr->skill_sav;
	skills[3] = p_ptr->skill_stl;
	skills[4] = p_ptr->skill_fos;
	skills[5] = p_ptr->skill_srh;
	skills[6] = p_ptr->skill_dis;
	skills[7] = p_ptr->skill_dev;

	/* Number of blows */
	skills[8] = p_ptr->num_blow;
	skills[9] = p_ptr->num_fire;
	skills[10] = p_ptr->num_spell;

	/* Infravision */
	skills[11] = p_ptr->see_infra;

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for skills (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	Packet_printf(&connp->c, "%c", PKT_SKILLS);

	for (i = 0; i < 12; i++)
	{
		Packet_printf(&connp->c, "%hd", skills[i]);
	}

	return 1;
}
	

int Send_pause(int ind)
{
	connection_t *connp = &Conn[Players[ind]->conn];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for skills (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	return Packet_printf(&connp->c, "%c", PKT_PAUSE);
}



int Send_monster_health(int ind, int num, byte attr)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for monster health bar (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c%c%c", PKT_MONSTER_HEALTH, num, attr);
	      }
	  }

	return Packet_printf(&connp->c, "%c%c%c", PKT_MONSTER_HEALTH, num, attr);
}

int Send_chardump(int ind)
{
	connection_t *connp = &Conn[Players[ind]->conn];
	//player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for chardump (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
#if 0
	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);
	    player_type *p_ptr2;
	    connection_t *connp2;

	    if (!Ind2)
	      end_mind(ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
		connp2 = &Conn[p_ptr2->conn];

		Packet_printf(&connp2->c, "%c", PKT_FLUSH);
	      }
	  }
#endif
	return Packet_printf(&connp->c, "%c", PKT_CHARDUMP);
}

/*
 * Return codes for the "Receive_XXX" functions are as follows:
 *
 * -1 --> Some error occured
 *  0 --> The action was queued (not enough energy)
 *  1 --> The action was ignored (not enough energy)
 *  2 --> The action completed successfully
 *
 *  Every code except for 1 will cause the input handler to stop
 *  processing actions.
 */

// This does absolutly nothing other than keep our connection active.
static int Receive_keepalive(int ind)
{
	int n, Ind;
	connection_t *connp = &Conn[ind];
	char ch;
	player_type *p_ptr = NULL;

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}
	/* If client has not been hacked, this should set AFK after 1 minute
	   of no activity. */
	if(connp->id != -1){
		Ind=GetInd[connp->id];
		p_ptr=Players[Ind];
		if(!p_ptr->afk && p_ptr->auto_afk){	/* dont oscillate ;) */
			if(++connp->inactive>45)	/* auto AFK timer (>1.5 min) */
				toggle_afk(Ind);
		}
	}

	return 2;
}

static int Receive_walk(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch, dir;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MOV))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
		      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* bugged here if !p_ptr */

	/* Disturb if running or resting */
	if (p_ptr->running || p_ptr->resting)
	{
		disturb(player, 0, 0);
		return 1;
	}

	if(p_ptr->command_rep){
		p_ptr->command_rep=-1;
	}

	if (player && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_walk(player, dir, p_ptr->always_pickup);
		return 2;
	}
	else
	{
		// Otherwise discared the walk request.
		//if (!connp->q.len && p_ptr->autoattack)
		// If we have no commands queued, then queue our walk request.
		// Note that ch might equal PKT_RUN, since Receive_run will
		// sometimes call this function.
		if (!connp->q.len)
		{
			Packet_printf(&connp->q, "%c%c", PKT_WALK, dir);
			return 0;
		}
		else
		{
			// If we have a walk command queued at the end of the queue,
			// then replace it with this queue request.  
			if (connp->q.buf[connp->q.len - 2] == PKT_WALK)
			{
				connp->q.len -= 2;
				Packet_printf(&connp->q, "%c%c", PKT_WALK, dir);
				return 0;
			}
		}
	}

	return 1;
}

static int Receive_run(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;

	int i, n, player = -1;
	char dir;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MOV))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
		      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}
	
	if(p_ptr->command_rep){
		p_ptr->command_rep=-1;
	}

	/* If not the dungeon master, who can always run */
	if (!p_ptr->admin_dm) 
	{
		/* Check for monsters in sight or confusion */
		for (i = 0; i < m_max; i++)
		{
			/* Check this monster */
			if ((p_ptr->mon_los[i] && !m_list[i].csleep &&
				m_list[i].level && !m_list[i].special) || (p_ptr->confused))
			{
				// Treat this as a walk request
				// Hack -- send the same connp->r "arguments" to Receive_walk
				return Receive_walk(ind);
			}
		}
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Disturb if we want to change directions */
	//if (dir != p_ptr->find_current) disturb(player, 0, 0);

	/* Hack -- Fix the running in '5' bug */
	if (dir == 5)
		return 1;

#if 0
	/* Only process the run command if we are not already running in
	 * the direction requested.
	 */
	if (player && (!p_ptr->running || (dir != p_ptr->find_current)))
	{
#endif
		// If we don't want to queue the command, return now.
		if ((n = do_cmd_run(player,dir)) == 2)
		{
			return 2;
		}
		// If do_cmd_run returns a 0, then there wasn't enough energy
		// to execute the run command.  Queue the run command if desired.
		else if (n == 0)
		{
			// Only buffer a run request if we have no previous commands
			// buffered, and it is a new direction or we aren't already
			// running.
			if (((!connp->q.len) && (dir != p_ptr->find_current)) || (!p_ptr->running))
			{
				Packet_printf(&connp->q, "%c%c", ch, dir);

				return 0;
			}
		}
	//}

	return 1;
}

static int Receive_tunnel(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr=NULL;

	char ch, dir;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MOV))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
		      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* all this is temp just to make it work */
	if(p_ptr->command_rep==-1){
		p_ptr->command_rep=0;
		return(0);
	}

	/* please redesign ALL of this out of higher level */
	if(p_ptr && p_ptr->command_rep!=PKT_TUNNEL){
		p_ptr->command_rep=-1;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_tunnel(player, dir);
		if (p_ptr->command_rep) Packet_printf(&connp->q, "%c%c", ch, dir);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%c", ch, dir);
		return 0;
	}


	return 1;
}

static int Receive_aim_wand(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch, dir;
	s16b item;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
		      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd%c", &ch, &item, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_aim_wand(player, item, dir);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%hd%c", ch, item, dir);
		return 0;
	}

	return 1;
}

static int Receive_drop(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;

	int n, player = -1; 
	s16b item, amt;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd%hd", &ch, &item, &amt)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_drop(player, item, amt);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%hd%hd", ch, item, amt);
		return 0;
	}

	return 1;
}

static int Receive_fire(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch, dir = 5;

	int n, player = -1;
	//s16b item;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

//	if ((n = Packet_scanf(&connp->r, "%c%c%hd", &ch, &dir, &item)) <= 0)
	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Check confusion */
	if (p_ptr->confused)
	{
		/* Change firing direction */
		while (dir == 5)
			dir = randint(9) + 1;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)/p_ptr->num_fire)
	{
//		do_cmd_fire(player, dir, item);
		do_cmd_fire(player, dir);
		return 2;
	}
	else if (player)
	{
//		Packet_printf(&connp->q, "%c%c%hd", ch, dir, item);
		Packet_printf(&connp->q, "%c%c", ch, dir);
		return 0;
	}

	return 1;
}

static int Receive_observe(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;

	s16b item;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1)
		do_cmd_observe(player, item);

	return 1;
}

static int Receive_stand(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = Players[ind];

	char ch;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MOV))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1)
	{
		do_cmd_stay(player, 1);
		return 2;
	}

	return -1;
}

static int Receive_destroy(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;

	s16b item, amt;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd%hd", &ch, &item, &amt)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_destroy(player, item, amt);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%hd%hd", ch, item, amt);
		return 0;
	}

	return 1;
}

static int Receive_look(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch, dir;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1)
		do_cmd_look(player, dir);

	return 1;
}

/*
 * Possibly, most of Receive_* functions can be bandled into one function
 * like this; that'll make the client *MUCH* more generic.		- Jir -
 */
static int Receive_activate_skill(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch, mkey, dir;

	int n, player = -1, old = -1;

	s16b book, spell;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];
		old = player;

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%c%hd%hd%c", &ch, &mkey, &book, &spell, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Not by class nor by item; by skill */
	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		p_ptr->current_char = (old == player)?TRUE:FALSE;

		if (p_ptr->ghost)
		{
			msg_print(player, "\377oYou need your body to use a skill.");
			return 2;
		}

		/* Break goi/manashield */
		if (mkey != MKEY_DODGE)	// it's not real 'activation'
		{
			if (p_ptr->invuln)
			{
				set_invuln(player, 0);
			}
			if (p_ptr->tim_manashield)
			{
				set_tim_manashield(player, 0);
			}
#if 0
			if (p_ptr->tim_wraith)
			{
				set_tim_wraith(player, 0);
			}
#endif	// 0
		}

		switch (mkey)
		{
			case MKEY_MIMICRY:
				if(get_skill(p_ptr, SKILL_MIMIC))
					do_cmd_mimic(player, spell);
				break;

			case MKEY_DODGE:
				use_ability_blade(player);
				break;

			case MKEY_FLETCHERY:
				do_cmd_fletchery(player);
				break;
			case MKEY_TRAP:
				do_cmd_set_trap(player, book, spell);
				break;
                	case MKEY_SCHOOL:
				cast_school_spell(player, spell, dir, book);

                }
		return 2;
	}
	else if (player)
	{
		p_ptr->current_spell = -1;
		p_ptr->current_mind = -1;
		Packet_printf(&connp->q, "%c%c%hd%hd%c", ch, mkey, book, spell, dir);
		return 0;
	}


	return 1;
}

static int Receive_open(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr=NULL;

	char ch, dir;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* all this is temp just to make it work */
	if(p_ptr->command_rep==-1){
		p_ptr->command_rep=0;
		return(0);
	}
	if(p_ptr && p_ptr->command_rep!=PKT_OPEN){
		p_ptr->command_rep=-1;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_open(player, dir);
		if (p_ptr->command_rep) Packet_printf(&connp->q, "%c%c", ch, dir);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%c", ch, dir);
		return 0;
	}

	return 1;
}

static int Receive_mind(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL, *p_ptr2 = NULL;

	char ch;

	int n, player = -1, Ind2;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1)
	{
		if (p_ptr->esp_link_type &&p_ptr->esp_link)
		  {
		    Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			bool d = TRUE;

			p_ptr2 = Players[Ind2];

			if (p_ptr->esp_link_type == LINK_DOMINATED)
			  {
			    if (!p_ptr->esp_link_end)
			      {
				p_ptr->esp_link_end = rand_int(20) + 15;
				d = TRUE;
			      }
			    else
			      {
				p_ptr->esp_link_end = 0;
				d = FALSE;
			      }
			  }
			if (p_ptr2->esp_link_type == LINK_DOMINATED)
			  {
			    if (!p_ptr2->esp_link_end)
			      {
				p_ptr2->esp_link_end = rand_int(20) + 15;
				d = TRUE;
			      }
			    else
			      {
				p_ptr2->esp_link_end = 0;
				d = FALSE;
			      }
			  }

			if (d)
			  {
			    msg_format(player, "\377RThe mind link with %s begins to break.", p_ptr2->name);
			    msg_format(Ind2, "\377RThe mind link with %s begins to break.", p_ptr->name);
			  }
			else
			  {
			    msg_format(player, "\377RThe mind link with %s stabilizes.", p_ptr2->name);
			    msg_format(Ind2, "\377RThe mind link with %s stabilizes.", p_ptr->name);
			  }
		      }
		  }
		else
		  {
		    if (p_ptr->esp_link_flags & LINKF_OPEN)
		      {
			msg_print(player, "\377RYou close your mind.");
			p_ptr->esp_link_flags &= ~LINKF_OPEN;
		      }
		    else
		      {
			msg_print(player, "\377RYou open your mind.");
			p_ptr->esp_link_flags |= LINKF_OPEN;
		      }
		  }
		return 2;
	}

	return 1;
}

static int Receive_ghost(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;

	int n, player = -1;

	s16b ability;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &ability)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_ghost_power(player, ability);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%hd", ch, ability);
		return 0;
	}

	return 1;
}

static int Receive_quaff(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;
	s16b item;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_quaff_potion(player, item);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%hd", ch, item);
		return 0;
	}

	return 1;
}

static int Receive_read(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;
	s16b item;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_read_scroll(player, item);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%hd", ch, item);
		return 0;
	}

	return 1;
}

static int Receive_search(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr=NULL;

	char ch;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MOV))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* all this is temp just to make it work */
	if(p_ptr->command_rep==-1){
		p_ptr->command_rep=0;
		return(0);
	}
	if(p_ptr && p_ptr->command_rep!=PKT_SEARCH){
		p_ptr->command_rep=-1;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_search(player);
		if (p_ptr->command_rep) Packet_printf(&connp->q, "%c", ch);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c", ch);
		return 0;
	}

	return 1;
}

static int Receive_take_off(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;

	s16b item;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_takeoff(player, item);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%hd", ch, item);
		return 0;
	}

	return 1;
}

static int Receive_use(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;
	s16b item;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_use_staff(player, item);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%hd", ch, item);
		return 0;
	}

	return 1;
}

static int Receive_throw(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch, dir;

	int n, player = -1;
	s16b item;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%c%hd", &ch, &dir, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_throw(player, dir, item);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%c%hd", ch, dir, item);
		return 0;
	}

	return 1;
}

static int Receive_wield(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;

	s16b item;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_wield(player, item);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%hd", ch, item);
		return 0;
	}

	return 1;
}

static int Receive_zap(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;
	s16b item;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_zap_rod(player, item);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%hd", ch, item);
		return 0;
	}

	return 1;
}

static int Receive_target(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;

	s16b dir;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1)
		do_cmd_target(player, dir);

	return 1;
}

static int Receive_target_friendly(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;

	s16b dir;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1)
		do_cmd_target_friendly(player, dir);

	return 1;
}


static int Receive_inscribe(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;

	int n, player = -1;

	s16b item;

	char inscription[80];

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd%s", &ch, &item, inscription)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1)
		do_cmd_inscribe(player, item, inscription);

	return 1;
}

static int Receive_uninscribe(int ind)
{
	connection_t *connp = &Conn[ind];

	char ch;

	int n, player = -1;

	s16b item;

	player_type *p_ptr = NULL;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1)
		do_cmd_uninscribe(player, item);

	return 1;
}

static int Receive_activate(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;

	s16b item;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_activate(player, item);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%hd", ch, item);
		return 0;
	}

	return 1;
}

static int Receive_bash(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr=NULL;

	char ch, dir;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MOV))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* all this is temp just to make it work */
	if(p_ptr->command_rep==-1){
		p_ptr->command_rep=0;
		return(0);
	}
	if(p_ptr && p_ptr->command_rep!=PKT_BASH){
		p_ptr->command_rep=-1;
	}
	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_bash(player, dir);
		if (p_ptr->command_rep) Packet_printf(&connp->q, "%c%c", ch, dir);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%c", ch, dir);
		return 0;
	}

	return 1;
}

static int Receive_disarm(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr=NULL;

	char ch, dir;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MOV))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* all this is temp just to make it work */
	if(p_ptr->command_rep==-1){
		p_ptr->command_rep=0;
		return(0);
	}
	if(p_ptr && p_ptr->command_rep!=PKT_DISARM)
		p_ptr->command_rep=-1;

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_disarm(player, dir);
		if (p_ptr->command_rep) Packet_printf(&connp->q, "%c%c", ch, dir);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%c", ch, dir);
		return 0;
	}

	return 1;
}

static int Receive_eat(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;

	s16b item;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_eat_food(player, item);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%hd", ch, item);
		return 0;
	}

	return 1;
}

		
static int Receive_fill(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;
	s16b item;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_refill(player, item);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%hd", ch, item);
		return 0;
	}

	return 1;
}

static int Receive_locate(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch, dir;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MOV))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1)
		do_cmd_locate(player, dir);

	return 1;
}

static int Receive_map(int ind)
{
	connection_t *connp = &Conn[ind];

	char ch, mode;

	int n, player = -1;

	player_type *p_ptr = NULL;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &mode)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1)
		do_cmd_view_map(player, mode);

	return 1;
}

static int Receive_search_mode(int ind)
{
	connection_t *connp = &Conn[ind];

	char ch;

	int n, player = -1;

	player_type *p_ptr = NULL;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1)
		do_cmd_toggle_search(player);

	return 1;
}

static int Receive_close(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch, dir;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_close(player, dir);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%c", ch, dir);
		return 0;
	}

	return 1;
}

static int Receive_skill_mod(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;

	s32b i;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%ld", &ch, &i)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1)
		increase_skill(player, i);

	return 1;
}

static int Receive_go_up(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_go_up(player);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c", ch);
		return 0;
	}

	return 1;
}

static int Receive_go_down(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_go_down(player);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c", ch);
		return 0;
	}

	return 1;
}


static int Receive_direction(int ind)
{
	connection_t *connp = &Conn[ind];

	char ch, dir;

	int n, player = -1;

	player_type *p_ptr = NULL;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1)
		Handle_direction(player, dir);

	return 1;
}

static int Receive_item(int ind)
{
	connection_t *connp = &Conn[ind];

	char ch;

	s16b item;

	int n, player = -1;

	player_type *p_ptr = NULL;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1)
		Handle_item(player, item);

	return 1;
}


static int Receive_message(int ind)
{
	connection_t *connp = &Conn[ind];

	char ch, buf[1024];

	int n, player = -1;
	if (connp->id != -1) player = GetInd[connp->id];
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%S", &ch, buf)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	player_talk(player, buf);

	return 1;
}
	
static int Receive_admin_house(int ind){
	connection_t *connp = &Conn[ind];
	char ch, dir;
	int n,player = -1;
	char buf[80];
	player_type *p_ptr = NULL;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%hd%s", &ch, &dir, buf)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}
	house_admin(player, dir, buf);
	return(1);
}

static int Receive_purchase(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;
	int n, player = -1;
	s16b item, amt;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%hd%hd", &ch, &item, &amt)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (player && p_ptr->store_num > -1)
		store_purchase(player, item, amt);
	else if (player)
		do_cmd_purchase_house(player, item);

	return 1;
}

static int Receive_sell(int ind)
{
	connection_t *connp = &Conn[ind];

	char ch;
	int n, player = -1;
	s16b item, amt;
	player_type *p_ptr = NULL;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%hd%hd", &ch, &item, &amt)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (player)
		store_sell(player, item, amt);

	return 1;
}

static int Receive_store_leave(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}
	else player = 0;

	if (player)
		p_ptr = Players[player];
	
	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (!player) return -1;

        /* Update stuff */
        p_ptr->update |= (PU_VIEW | PU_LITE);
        p_ptr->update |= (PU_MONSTERS);

        /* Redraw stuff */
        p_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA);

        /* Redraw map */
        p_ptr->redraw |= (PR_MAP);

        /* Window stuff */
        p_ptr->window |= (PW_OVERHEAD);

	/* Update store info */
	if (p_ptr->store_num > -1)
	{
		p_ptr->store_num = -1;

		/* Hack -- don't stand in the way */
		teleport_player(player, 1);
//		do_cmd_walk(player, 10 - p_ptr->last_dir, p_ptr->always_pickup);
	}

	/* hack -- update night/day in wilderness levels */
	/* XXX it's not so good place to do such things -
	 * prolly we'll need PU_SUN or sth.		- Jir - */
	if ((!p_ptr->wpos.wz) && (IS_DAY)) wild_apply_day(&p_ptr->wpos); 
	if ((!p_ptr->wpos.wz) && (IS_NIGHT)) wild_apply_night(&p_ptr->wpos);

	return 1;
}

static int Receive_store_confirm(int ind)
{
	connection_t *connp = &Conn[ind];
	char ch;
	int n, player = -1;
	player_type *p_ptr = NULL;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (!player)
		return -1;

	store_confirm(player);

	return 1;
}

/* Store code should be written to allow more kinds of actions in general..
 */
static int Receive_store_examine(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;
	int n, player = -1;
	s16b item;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (player && p_ptr->store_num > -1)
		store_examine(player, item);

	return 1;
}

static int Receive_store_command(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch;
	int n, player = 0, gold;
	u16b action;
	s16b item, item2, amt;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%hd%hd%hd%hd%d", &ch, &action, &item, &item2, &amt, &gold)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (player && p_ptr->store_num > -1)
		store_exec_command(player, action, item, item2, amt, gold);

	return 1;
}

static int Receive_drop_gold(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	int n, player = -1;
	s32b amt;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%ld", &ch, &amt)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_drop_gold(player, amt);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%ld", &ch, &amt);
		return 0;
	}

	return 1;
}

static int Receive_steal(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch, dir;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_steal(player, dir);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%c", ch, dir);
		return 0;
	}

	return 1;
}


static int Receive_King(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	byte type;
	char ch;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &type)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1)
	{
		switch(type)
		{
#if 0 /* DGDGDGDG -- Cause fucking up of levels */
			case KING_OWN:
				do_cmd_own(player);
				break;
#endif
		}
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%c", ch, type);
		return 0;
	}

	return 1;
}
	
static int Receive_redraw(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;
	int player = -1, n;
	char ch, mode;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type && p_ptr->esp_link)
		{
			int Ind2 = find_player(p_ptr->esp_link);

			if (!Ind2)
				end_mind(ind, TRUE);
			else
			{
				if (Players[Ind2]->esp_link_flags & LINKF_VIEW)
				{
					player = Ind2;
					p_ptr = Players[Ind2];
				}
			}
		}
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &mode)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (player)
	{
//		p_ptr->store_num = -1;
		p_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_MAP);
	       	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
	       	p_ptr->update |= (PU_BONUS | PU_VIEW | PU_MANA | PU_HP | PU_SANITY);

		/* Do 'Heavy' redraw if requested.
		 * TODO: One might wish to add more detailed modes
		 */
		if (mode)
		{
			/* Tell the server to redraw the player's display */
			p_ptr->redraw |= PR_MAP | PR_EXTRA | PR_BASIC | PR_HISTORY | PR_VARIOUS;
			p_ptr->redraw |= PR_PLUSSES;
			p_ptr->redraw |= PR_STUDY;

			/* Update his view, light, bonuses, and torch radius */
#ifdef ORIG_SKILL_EVIL	/* not to be defined */
			p_ptr->update |= (PU_VIEW | PU_LITE | PU_BONUS | PU_TORCH | PU_DISTANCE
					| PU_SKILL_INFO | PU_SKILL_MOD);
#else
			p_ptr->update |= (PU_VIEW | PU_LITE | PU_BONUS | PU_TORCH | PU_DISTANCE );
#endif
			p_ptr->update |= (PU_MANA | PU_HP | PU_SANITY);

			/* Update his inventory, equipment, and spell info */
			p_ptr->window |= (PW_INVEN | PW_EQUIP);
		}
	}

	return 1;
}

static int Receive_rest(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;
	int player = -1, n;
	char ch;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MOV))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0)
	{
		if (n == -1)	
			Destroy_connection(ind, "read error");
		return n;
	}

	if (player)
	{
		cave_type **zcave;

		/* If we are already resting, cancel the rest. */
		/* Waking up takes no energy, although we will still be drowsy... */
		if (p_ptr->resting)
		{
			disturb(player, 0, 0);
			return 2;
		}

		/* Don't rest if we are poisoned or at max hit points and max spell points */
		if ((p_ptr->poisoned) || ((p_ptr->chp == p_ptr->mhp) &&
					(p_ptr->csp == p_ptr->msp)))
		{
			return 2;
		}

		if(!(zcave=getcave(&p_ptr->wpos))) return 2;

		/* Can't rest on a Void Jumpgate -- too dangerous */
		if (zcave[p_ptr->py][p_ptr->px].feat == FEAT_BETWEEN)
		{
			msg_print(player, "Resting on a Void Jumpgate is too dangerous!");
			return 2;
		}

		/* Resting takes a lot of energy! */
		if ((p_ptr->energy) >= (level_speed(&p_ptr->wpos)*2)-1)
		{

			/* Set flag */
			p_ptr->resting = TRUE;

			/* Make sure we aren't running */
			p_ptr->running = FALSE;

			/* Take a lot of energy to enter "rest mode" */
			p_ptr->energy -= (level_speed(&p_ptr->wpos)*2)-1;

			/* Redraw */
			p_ptr->redraw |= (PR_STATE);
			return 2;
		}
		/* If we don't have enough energy to rest, disturb us (to stop
		 * us from running) and queue the command.
		 */
		else
		{
			disturb(player, 0, 0);
			Packet_printf(&connp->q, "%c", ch);
			return 0;
		}
	}

	return 1;
}

void Handle_clear_buffer(int Ind)
{
	if (Ind)
	{
		player_type *p_ptr = Players[Ind];
		connection_t *connp = &Conn[p_ptr->conn];

		/* Clear the buffer */
		Sockbuf_clear(&connp->q);
		Sockbuf_clear(&connp->r);
#if 0
		Sockbuf_clear(&connp->c);
		Sockbuf_clear(&connp->w);
#endif	// 0

		/* Clear 'current spell' */
		p_ptr->current_spell = -1;
	}
}

static int Receive_clear_buffer(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;
	int player = -1, n;
	char ch;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0)
	{
		if (n == -1)	
			Destroy_connection(ind, "read error");
		return n;
	}

#if 0
	if (player)
	{
		/* Clear the buffer */
		Sockbuf_clear(&connp->q);
		Sockbuf_clear(&connp->r);
#if 0
		Sockbuf_clear(&connp->c);
		Sockbuf_clear(&connp->w);
#endif	// 0

		/* Clear 'current spell' */
		p_ptr->current_spell = -1;
	}
#endif	// 0

	Handle_clear_buffer(player);

	return 1;
}

static int Receive_special_line(int ind)
{
	connection_t *connp = &Conn[ind];
	int player = -1, n;
	char ch, type;
	s16b line;

	if (connp->id != -1) player = GetInd[connp->id];
		else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%c%hd", &ch, &type, &line)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (player)
	{
		char kludge[2] = "";
		kludge[0] = (char) line;
		kludge[1] = '\0';
		switch (type)
		{
			case SPECIAL_FILE_NONE:
				Players[player]->special_file_type = FALSE;
				/* Remove the file */
/*				if (!strcmp(Players[player]->infofile,
							Players[player]->cur_file))	*/
					fd_kill(Players[player]->infofile);
				break;
			case SPECIAL_FILE_UNIQUE:
				do_cmd_check_uniques(player, line);
				break;
			case SPECIAL_FILE_ARTIFACT:
				do_cmd_check_artifacts(player, line);
				break;
			case SPECIAL_FILE_PLAYER:
				do_cmd_check_players(player, line);
				break;
            case SPECIAL_FILE_PLAYER_EQUIP:
                do_cmd_check_player_equip(player, line);
                break;
			case SPECIAL_FILE_OTHER:
				do_cmd_check_other(player, line);
				break;
			case SPECIAL_FILE_SCORES:
				display_scores(player, line);
				break;
			case SPECIAL_FILE_HELP:
				do_cmd_help(player, line);
 				break;
			/* Obsolete, just left for compatibility (DELETEME) */
			case SPECIAL_FILE_LOG:
				if (is_admin(Players[player]))
					do_cmd_view_rfe(player, "tomenet.log", line);
 				break;
			case SPECIAL_FILE_RFE:
				if (is_admin(Players[player]) || cfg.public_rfe)
					do_cmd_view_rfe(player, "tomenet.rfe", line);
 				break;
			/*
			 * Hack -- those special files actually use do_cmd_check_other
			 * XXX redesign it
			 */
			case SPECIAL_FILE_SERVER_SETTING:
				do_cmd_check_server_settings(player);
 				break;
			case SPECIAL_FILE_MONSTER:
				do_cmd_show_monster_killed_letter(player, kludge);
 				break;
			case SPECIAL_FILE_OBJECT:
				do_cmd_show_known_item_letter(player, kludge);
 				break;
			case SPECIAL_FILE_HOUSE:
				do_cmd_show_houses(player);
 				break;
			case SPECIAL_FILE_TRAP:
				do_cmd_knowledge_traps(player);
 				break;
			case SPECIAL_FILE_RECALL:
				do_cmd_knowledge_dungeons(player);
 				break;
		}
	}

	return 1;
}

static int Receive_options(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;
	int player = -1, i, n;
	char ch;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];
	}
	else
	{
		player = 0;
		p_ptr = NULL;
	}

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (player)
	{
		bool options[OPT_MAX];
		for (i = 0; i < OPT_MAX; i++)
		{
			n = Packet_scanf(&connp->r, "%c", &options[i]);

			if (n <= 0)
			{
				Destroy_connection(ind, "read error");
				return n;
			}
		}

		/* Sync named options */
		sync_options(player, options);
	}

	return 1;
}

static int Receive_suicide(int ind)
{
	connection_t *connp = &Conn[ind];
	int player = -1, n;
	char ch;

	if (connp->id != -1)
		player = GetInd[connp->id];
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Commit suicide */
	do_cmd_suicide(player);

	return 1;
}

static int Receive_party(int ind)
{
	connection_t *connp = &Conn[ind];
	int player = -1, n;
	char ch, buf[160];
	s16b command;

	if (connp->id != -1) player = GetInd[connp->id];
		else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%hd%s", &ch, &command, buf)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (player)
	{
		switch (command)
		{
			case PARTY_CREATE:
			{
				party_create(player, buf);
				break;
			}

			case PARTY_ADD:
			{
				party_add(player, buf);
				break;
			}

			case PARTY_DELETE:
			{
				party_remove(player, buf);
				break;
			}

			case PARTY_REMOVE_ME:
			{
				party_leave(player);
				break;
			}

			case PARTY_HOSTILE:
			{
				add_hostility(player, buf);
				break;
			}

			case PARTY_PEACE:
			{
				remove_hostility(player, buf);
				break;
			}
		}
	}

	return 1;
}

static int Receive_guild(int ind){
	connection_t *connp = &Conn[ind];
	int player = -1, n;
	char ch, buf[160];
	s16b command;

	if (connp->id != -1) player = GetInd[connp->id];
		else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%hd%s", &ch, &command, buf)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (player)
	{
		switch(command){
			case GUILD_CREATE:
			{
				guild_create(player, buf);
				break;
			}

			case GUILD_ADD:
			{
				guild_add(player, buf);
				break;
			}

			case GUILD_DELETE:
			{
				guild_remove(player, buf);
				break;
			}

			case GUILD_REMOVE_ME:
			{
				guild_leave(player);
				break;
			}
		}
	}
	return 1;
}

void Handle_direction(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];
	int Ind2 = Ind;

	if (!dir)
	{
	  	p_ptr->current_char = 0;
		p_ptr->current_spell = -1;
		p_ptr->current_mind = -1;
		p_ptr->current_rod = -1;
		p_ptr->current_activation = -1;
		return;
	}

	if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_MISC))
	  {
	    Ind2 = find_player(p_ptr->esp_link);
	    
	    if (!Ind2)
	      end_mind(Ind, TRUE);
	}


	if (p_ptr->current_spell != -1)
	{
//		if (p_ptr->current_realm == REALM_GHOST)
		if (p_ptr->ghost)
			do_cmd_ghost_power_aux(Ind, dir);
		else if (p_ptr->current_realm == REALM_MIMIC)
			do_mimic_power_aux(Ind, dir);
		else p_ptr->current_spell = -1;
	}
	else if (p_ptr->current_rod != -1)
		do_cmd_zap_rod_dir(Ind, dir);
	else if (p_ptr->current_activation != -1)
		do_cmd_activate_dir(Ind, dir);
}
		
void Handle_item(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];
	int i;

	if ((p_ptr->current_enchant_h > 0) || (p_ptr->current_enchant_d > 0) ||
             (p_ptr->current_enchant_a > 0))
	{
		enchant_spell_aux(Ind, item, p_ptr->current_enchant_h,
			p_ptr->current_enchant_d, p_ptr->current_enchant_a,
			p_ptr->current_enchant_flag);
	}
	else if (p_ptr->current_identify)
	{
		ident_spell_aux(Ind, item);
	}
	else if (p_ptr->current_star_identify)
	{
		identify_fully_item(Ind, item);
	}
	else if (p_ptr->current_recharge)
	{
		recharge_aux(Ind, item, p_ptr->current_recharge);
	}
	else if (p_ptr->current_artifact)
	  {
	    create_artifact_aux(Ind, item);
	  }
	else if (p_ptr->current_telekinesis != NULL)
	{
		telekinesis_aux(Ind, item);
	}
	else if (p_ptr->current_curse != 0)
	{
		curse_spell_aux(Ind, item);
	}

	for (i = 0; i < INVEN_PACK; i++) inven_item_optimize(Ind, i);
}

/* Is it a king and on his land ? */
bool player_is_king(int Ind)
{
	player_type *p_ptr = Players[Ind];

	return FALSE;
	
	if (p_ptr->total_winner && ((inarea(&p_ptr->own1, &p_ptr->wpos)) || (inarea(&p_ptr->own2, &p_ptr->wpos))))
	{
		return TRUE;
	}

	/* Assume false */	
	return FALSE;
}

/* receive a dungeon master command */
static int Receive_master(int ind)
{
	connection_t *connp = &Conn[ind];
	int player = -1, n;
	char ch, buf[160];
	s16b command;

	if (connp->id != -1) player = GetInd[connp->id];
		else player = 0;

	/* Make sure this came from the dungeon master.  Note that it may be
	 * possible to spoof this, so probably in the future more advanced
	 * authentication schemes will be neccecary. -APD
	 */

	/* Is this necessary here? Maybe (evileye) */
	if (!Players[player]->admin_dm &&
		!Players[player]->admin_wiz &&
		!player_is_king(player) && !guild_build(player))
	{
		/* Hack -- clear the receive and queue buffers since we won't be
		 * reading in the dungeon master parameters that were sent.
		 */
		Sockbuf_clear(&connp->r);
		Sockbuf_clear(&connp->c);
		return 2;
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd%s", &ch, &command, buf)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (player)
	{
		switch (command)
		{
			case MASTER_LEVEL:
			{
				master_level(player, buf);
				break;
			}

			case MASTER_BUILD:
			{
				master_build(player, buf);
				break;
			}

			case MASTER_SUMMON:
			{
				master_summon(player, buf);
				break;
			}
		
			case MASTER_GENERATE:
			{
				master_generate(player, buf);
				break;
			}

			case MASTER_PLAYER:
			{
				master_player(player, buf);
				break;
			}

			case MASTER_SCRIPTS:
			{
				master_script_exec(player, buf);
				break;
			}

			case MASTER_SCRIPTB:
			{
				master_script_begin(buf + 1, *buf);
				break;
			}

			case MASTER_SCRIPTE:
			{
				master_script_end();
				break;
			}

			case MASTER_SCRIPTL:
			{
				master_script_line(buf);
				break;
			}
		}
	}

	return 2;
}

/* automatic phase command, will try to phase door
 * in the best way possobile.
 *
 * This function should probably be improved a lot, I am just
 * doing a basic version for now.
 */

static int Receive_autophase(int ind)
{
	player_type *p_ptr = NULL;
	connection_t *connp = &Conn[ind];
	object_type *o_ptr;
	int player = -1, n;

	if (connp->id != -1) player = GetInd[connp->id];
		else player = 0;

	/* a valid player was found, try to do the autophase */	
	if (player)
	{
		p_ptr = Players[player];
		/* first, check the inventory for phase scrolls */
		/* check every item of his inventory */
		for (n = 0; n < INVEN_PACK; n++)
		{
			o_ptr = &p_ptr->inventory[n];
			if ((o_ptr->tval == TV_SCROLL) && (o_ptr->sval == SV_SCROLL_PHASE_DOOR))
			{
				/* found a phase scroll, read it! */
				do_cmd_read_scroll(player, n);
				return 1;
			}
		}
	}

	/* Failure!  We are in trouble... */

	return -1;
}

void end_mind(int Ind, bool update)
{
	player_type *p_ptr=Players[Ind];
	msg_print(Ind, "\377REnding mind link.");
	p_ptr->esp_link = 0;
	p_ptr->esp_link_type = 0;
	p_ptr->esp_link_flags = 0;
	if(update){
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
		p_ptr->update |= (PU_BONUS | PU_VIEW | PU_MANA | PU_HP);
		p_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_MAP);
	}
}

static int Receive_spike(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch, dir;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		do_cmd_spike(player, dir);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%c", ch, dir);
		return 0;
	}

	return 1;
}

/*
 * Lazy way to add a new command	- Jir -
 */
static int Receive_raw_key(int ind)
{
	connection_t *connp = &Conn[ind];
	player_type *p_ptr = NULL;

	char ch, key;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type &&p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_OBJ))
		  {
		    int Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
	      	      end_mind(ind, TRUE);
		    else
		      {
			player = Ind2;
			p_ptr = Players[Ind2];
		      }
		  }
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &key)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		if (p_ptr->store_num > -1)
		{
			switch (key)
			{
				default:
					msg_format(player, "'%c' key does not work in this building.", key);
					break;
			}
		}
		else
		{
			switch (key)
			{
				/* Test :) */
				case '_':
					do_cmd_drink_fountain(player);
					break;

				default:
					msg_format(player, "'%c' key is currently not used.  Hit '?' for help.", key);
					break;
			}
		}
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%c", ch, key);
		return 0;
	}

	return 1;
}
