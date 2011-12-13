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

#include <sys/types.h>
#include <sys/stat.h>
#ifndef WINDOWS
#include <sys/wait.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#endif
#include <errno.h>


#ifdef WINDOWS
# define EWOULDBLOCK WSAEWOULDBLOCK
#endif

#define MAX_SELECT_FD			1023
/* #define MAX_RELIABLE_DATA_PACKET_SIZE	1024 */
#define MAX_RELIABLE_DATA_PACKET_SIZE	512

#define MAX_MOTD_CHUNK			512
#define MAX_MOTD_SIZE			(30*1024)
#define MAX_MOTD_LOOPS			120


/* hack to prevent the floor tile bug on windows xp and windows 2003 machines */
#define FLOORTILEBUG_WORKAROUND


/* Sanity check of client input */
#define bad_dir(d)	((d)<0 || (d)>9)	/* used for non-targetting actions that require a direction */
//#define bad_dir1(d)	((d)<0 || (d)>9+1)	/* used for most targetting actions */
static bool bad_dir1(int Ind, char *dir) {
	/* paranoia? */
	if (Ind == -1) return TRUE;

	if (*dir < 0 || *dir > 11) return TRUE;
	if (*dir == 11) {
		if (target_okay(Ind)) *dir = 5;
	}
	else if (*dir == 10) {
		if (!target_okay(Ind)) return TRUE;
		*dir = 5;
	}
	return FALSE;
}
#define bad_dir2(d)	((d)<128 || (d)>137)	/* dir + 128; used for manual target positioning */
//#define bad_dir3(d)	((d)<-1 || (d)>9+1)	/* used for MKEY_SCHOOL activations */
static bool bad_dir3(int Ind, char *dir) {
	/* paranoia? */
	if (Ind == -1) return TRUE;

	if (*dir < -1 || *dir > 11) return TRUE;
	if (*dir == 11) {
		if (target_okay(Ind)) *dir = 5;
	}
	else if (*dir == 10) {
		if (!target_okay(Ind)) return TRUE;
		*dir = 5;
	}
	return FALSE;
}



#if 0
static bool validstrings(char *nick, char *real, char *host);
#else
static bool validstring(char *nick);
#endif
void validatestring(char *string);

connection_t	**Conn = NULL;
static int	max_connections = 0;
static setup_t	Setup;
static int	(*playing_receive[256])(int ind),
		(*login_receive[256])(int ind),
		(*drain_receive[256])(int ind);
int		login_in_progress;
static int	num_logins, num_logouts;
static long	Id;
int		NumPlayers, NumNonAdminPlayers;


pid_t		metapid = 0;
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
	/* adding weekdays, basically just for p_printf() - C. Blue */
	static char	day_names[7][4] = {
				"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
			};
	time(&now);
	tmp = localtime(&now);
	sprintf(buf, "%02d %s (%s) %02d:%02d:%02d",
		tmp->tm_mday, month_names[tmp->tm_mon], day_names[tmp->tm_wday],
		tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
	return buf;
}

/* added for the BBS - C. Blue */
char *showdate(void)
{
	time_t		now;
	struct tm	*tmp;
	static char	buf[80];
	time(&now);
	tmp = localtime(&now);
	sprintf(buf, "%02d-%02d", tmp->tm_mon + 1, tmp->tm_mday);
	return buf;
}

/* added for changing seasons via lua cron_24h() - C. Blue */
void get_date(int *weekday, int *day, int *month, int *year) {
        time_t		now;
        struct tm	*tmp;
        time(&now);
        tmp = localtime(&now);
        *weekday = tmp->tm_wday;
        *day = tmp->tm_mday;
        *month = tmp->tm_mon + 1;
	*year = tmp->tm_year + 1900;
}

void add_banlist(int Ind, int time){
	struct ip_ban *ptr;
	if(!time) return;

	ptr = NEW(struct ip_ban);

	ptr->next = banlist;
	ptr->time = time;
	strcpy(ptr->ip, Conn[Players[Ind]->conn]->addr);
	s_printf("Banned connections from %s for %d minutes\n", ptr->ip, time);
	banlist = ptr;
}

void add_banlist_ip(char *ip_addy, int time){
	struct ip_ban *ptr;
	if(!time) return;

	ptr = NEW(struct ip_ban);

	ptr->next = banlist;
	ptr->time = time;
	strcpy(ptr->ip, ip_addy);
	s_printf("Banned connections from %s for %d minutes\n", ptr->ip, time);
	banlist = ptr;
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
//	drain_receive[PKT_ACK]			= Receive_ack;
	drain_receive[PKT_VERIFY]		= Receive_discard;
	drain_receive[PKT_PLAY]			= Receive_discard;

	login_receive[PKT_PLAY]			= Receive_play;
	login_receive[PKT_QUIT]			= Receive_quit;
//	login_receive[PKT_ACK]			= Receive_ack;
	login_receive[PKT_VERIFY]		= Receive_discard;
	login_receive[PKT_LOGIN]		= Receive_login;
	
//	playing_receive[PKT_ACK]		= Receive_ack;
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
	playing_receive[PKT_TAKE_OFF_AMT]	= Receive_take_off_amt;
	playing_receive[PKT_USE]		= Receive_use;
	playing_receive[PKT_THROW]		= Receive_throw;
	playing_receive[PKT_WIELD]		= Receive_wield;
	playing_receive[PKT_OBSERVE]		= Receive_observe;
	playing_receive[PKT_ZAP]		= Receive_zap;
	playing_receive[PKT_ZAP_DIR]		= Receive_zap_dir;
	playing_receive[PKT_MIND]		= Receive_mind;

	playing_receive[PKT_TARGET]		= Receive_target;
	playing_receive[PKT_TARGET_FRIENDLY]	= Receive_target_friendly;
	playing_receive[PKT_INSCRIBE]		= Receive_inscribe;
	playing_receive[PKT_UNINSCRIBE]		= Receive_uninscribe;
	playing_receive[PKT_ACTIVATE]		= Receive_activate;
	playing_receive[PKT_ACTIVATE_DIR]	= Receive_activate_dir;
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
	playing_receive[PKT_CLEAR_ACTIONS]	= Receive_clear_actions;

	playing_receive[PKT_SPIKE]		= Receive_spike;
	playing_receive[PKT_GUILD]		= Receive_guild;

        playing_receive[PKT_SKILL_MOD]		= Receive_skill_mod;
	playing_receive[PKT_SKILL_DEV]		= Receive_skill_dev;
        playing_receive[PKT_ACTIVATE_SKILL]	= Receive_activate_skill;
	playing_receive[PKT_RAW_KEY]		= Receive_raw_key;
	playing_receive[PKT_STORE_EXAMINE]	= Receive_store_examine;
	playing_receive[PKT_STORE_CMD]		= Receive_store_command;
	playing_receive[PKT_PING]		= Receive_ping;

	/* New stuff for v4.4.1 or 4.4.0d (dual-wield & co) - C. Blue */
	playing_receive[PKT_SIP]		= Receive_sip;
	playing_receive[PKT_TELEKINESIS]	= Receive_telekinesis;
	playing_receive[PKT_BBS]		= Receive_BBS;
	playing_receive[PKT_WIELD2]		= Receive_wield2;
	playing_receive[PKT_CLOAK]		= Receive_cloak;
	playing_receive[PKT_INVENTORY_REV]	= Receive_inventory_revision;
	playing_receive[PKT_ACCOUNT_INFO]	= Receive_account_info;
	playing_receive[PKT_CHANGE_PASSWORD]	= Receive_change_password;

	playing_receive[PKT_FORCE_STACK]	= Receive_force_stack;

	playing_receive[PKT_REQUEST_KEY]	= Receive_request_key;
	playing_receive[PKT_REQUEST_NUM]	= Receive_request_num;
	playing_receive[PKT_REQUEST_STR]	= Receive_request_str;
	playing_receive[PKT_REQUEST_CFR]	= Receive_request_cfr;
}

static int Init_setup(void)
{
	int n = 0, i;
	char buf[1024];
	FILE *fp;

	Setup.frames_per_second = cfg.fps;
	Setup.max_race = MAX_RACE;
#if 1
 #ifdef ENABLE_RUNEMASTER
  #ifdef ENABLE_MCRAFT
	Setup.max_class = MAX_CLASS;
  #else
	Setup.max_class = MAX_CLASS - 1;
  #endif
 #else
  #ifdef ENABLE_MCRAFT
	Setup.max_class = MAX_CLASS - 1;
  #else
	Setup.max_class = MAX_CLASS - 2;
  #endif
 #endif
#else
	Setup.max_class = MAX_CLASS;
#endif

	Setup.max_trait = MAX_TRAIT;


	Setup.motd_len = 23 * 120; /*80;*/	/* colour codes extra */
	Setup.setup_size = sizeof(setup_t);

	path_build(buf, 1024, ANGBAND_DIR_TEXT, "news.txt");

	/* Open the news file */
	fp = my_fopen(buf, "r");

	if (fp) {
		/* Dump the file into the buffer */
		while (0 == my_fgets(fp, buf, 1024, TRUE) && n < 23)
		{
			/* strncpy(&Setup.motd[n * 80], buf, 80); */
			strncpy(&Setup.motd[n * 120], buf, 120);
			n++;
		}

		my_fclose(fp);
	}
	
	/* MEGAHACK -- copy race/class names */
	/* XXX I know this ruins the meaning of Setup... sry	- Jir - */
	for (i = 0; i < MAX_RACE; i++)
	{
		if (!race_info[i].title)
		{
			Setup.max_race = i;
			break;
		}
//		strncpy(&Setup.race_title[i], race_info[i].title, 12);
//		Setup.race_choice[i] = race_info[i].choice;
		/* 1 for '\0', 4 for race_choice */
		Setup.setup_size += strlen(race_info[i].title) + 1 + 4 + 6;
	}

	for (i = 0; i < MAX_CLASS; i++)
	{
		if (!class_info[i].title)
		{
			Setup.max_class = i;
			break;
		}
//		strncpy(&Setup.class_title[i], class_info[i].title, 12);
		Setup.setup_size += strlen(class_info[i].title) + 1 + 6;
	}

	for (i = 0; i < MAX_TRAIT; i++) {
		if (!trait_info[i].title) {
			Setup.max_trait = i;
			break;
		}
		Setup.setup_size += strlen(trait_info[i].title) + 4;
	}

	return 0;
}

void init_players(){
	max_connections = MAX_SELECT_FD - 24; /* 999 connections at most */
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
 	static char local_name[1024];
	static int init = 0;
#ifndef EVIL_METACLIENT
        static long resolved_ip;
#endif
	char buf_meta[16384];	/* 16k is quite enough */
//	int bytes;
	int i, sock;
	char temp[100];
	char *url, *notes;
	bool hidden_dungeon_master = 0;

        /* Abort if the user doesn't want to report */
	if (!cfg.report_to_meta || cfg.runlevel<4 || ((cfg.runlevel > 1023) && (cfg.runlevel < 2045)))
		return FALSE;

	/* If this is the first time called, initialize our hostname */
	if (!init)
	{
#ifndef EVIL_METACLIENT
		struct hostent	*hp;
#endif

		/* Never do this again */
		init = 1;

		/* Get our hostname */
#if 0
# ifdef BIND_NAME
		strncpy(local_name, BIND_NAME, 1024);
# else
		GetLocalHostName(local_name, 1024);
# endif
#else
		if (cfg.bind_name)
			strncpy(local_name, cfg.bind_name, 1024);
		else
			GetLocalHostName(local_name, 1024);
#endif

#ifndef EVIL_METACLIENT
		hp = gethostbyname(cfg.meta_address);
		if (hp == NULL)
		{
# ifndef WINDOWS
			sl_errno = SL_EHOSTNAME;
# endif
                        init = 0;
			return (FALSE);
		} else {
			resolved_ip = ((struct in_addr*)(hp->h_addr))->s_addr;
		}
#endif
	}

	memset(buf_meta, 0, sizeof(buf_meta));

	url = html_escape(local_name);
	sprintf(buf_meta, "<server url=\"%s\" port=\"%d\" protocol=\"2\"", url, (int) cfg.game_port);
	free(url);

	if (flag & META_DIE)
	{
		strcat(buf_meta, " death=\"true\"></server>");
	}
	else
	{
		strcat(buf_meta, ">");
		if (flag & META_START)
		{
#ifndef WINDOWS
			signal(SIGUSR1, SIG_IGN);
#endif
		}

                else if (flag & META_UPDATE)
                {
                        /* Hack -- If cfg_secret_dungeon_master is enabled, determine
                         * if the DungeonMaster is playing, and if so, reduce the
                         * number of players reported.
                         */

			strcat(buf_meta, "<notes>");
			notes = html_escape(cfg.server_notes);
			strcat(buf_meta, notes);
			free(notes);
			strcat(buf_meta, "</notes>");

                        for (i = 1; i <= NumPlayers; i++)
                        {
                                if (Players[i]->admin_dm && cfg.secret_dungeon_master) hidden_dungeon_master++;
                        }

                        /* if someone other than a dungeon master is playing */
                        if (NumPlayers - hidden_dungeon_master)
                        {
				char *name;
                                for (i = 1; i <= NumPlayers; i++)
                                {
                                        /* handle the cfg_secret_dungeon_master option */
                                        if (Players[i]->admin_dm && cfg.secret_dungeon_master) continue;

                                        strcat(buf_meta, "<player>");
					name = html_escape(Players[i]->name);
					strcat(buf_meta, name);
					free(name);
                                        strcat(buf_meta, "</player>");
                                }
                        }
                }

#ifdef TEST_SERVER
		strcat(buf_meta, "<game>TomeNET TEST-ONLY</game>");
#else
 #ifdef ARCADE_SERVER /* made this one higher priority since using RPG_SERVER as an "ARCADE-addon" might be desired :) */
		//removed "Smash." -Moltor
		strcat(buf_meta, "<game>TomeNET Arcade</game>");
 #else
  #ifdef RPG_SERVER
		strcat(buf_meta, "<game>TomeNET RPG (not for beginners)</game>");
  #else
   #ifdef FUN_SERVER
		strcat(buf_meta, "<game>TomeNET Fun</game>");
   #else
                strcat(buf_meta, "<game>TomeNET</game>");
   #endif
  #endif
 #endif
#endif

                /* Append the version number */
                snprintf(temp, sizeof(temp), "<version>%d.%d.%d%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, SERVER_VERSION_TAG);
                strcat(buf_meta, temp);
                strcat(buf_meta, "</version></server>");
        }

        s_printf("Sending meta info...\n");

	/* If we haven't setup the meta connection yet, abort */
#ifdef EVIL_METACLIENT
	sock = open("metadata", O_WRONLY|O_CREAT|O_TRUNC, 0700);
#else
        block_timer();
        sock = socket(AF_INET, SOCK_STREAM, 0);
#endif
	if (sock == -1){
		return FALSE;
	}

	{
#ifndef EVIL_METACLIENT
		struct sockaddr_in	iaddr;

		memset(&iaddr, 0, sizeof (iaddr));
		iaddr.sin_family = AF_INET;
		iaddr.sin_port = htons(cfg.meta_port);
		iaddr.sin_addr.s_addr = resolved_ip;
		if (connect(sock, (struct sockaddr*)&iaddr, sizeof (iaddr)) == -1)
		{
			s_printf("Meta Info NOT sent to the meta : connect %s\n", strerror(errno));
			close(sock);
			return FALSE;
		}
#endif

		if (write(sock, buf_meta, strlen(buf_meta)) == -1)
		{
			s_printf("Meta Info NOT sent to the meta : write\n");
			close(sock);
			return FALSE;
		}
		s_printf("Info sent to the meta\n");
                close(sock);
	}
#ifdef EVIL_METACLIENT
	Check_evilmeta();

	/* kill it or update it */
	if (metapid) {
#ifndef WINDOWS
		kill(metapid, (flag & META_DIE ? SIGTERM : SIGUSR1));
#endif
	}
#else
        allow_timer();
#endif
	return TRUE;
}

/*
 * Start the evilmeta client
 */
void Start_evilmeta(void)
{
#ifdef EVIL_METACLIENT
#ifndef WINDOWS
	if (cfg.report_to_meta && !metapid) {
		metapid = fork();
		if (metapid == -1) {
			/* didnt work.. we should die */
			fprintf(stderr, "fork failed!\n");
			exit(-5);
		}
		if (metapid == 0) {
#ifdef ARCADE_SERVER /* made this one top priority since using RPG_SERVER as an "ARCADE-addon" might be desired :) */
			char *cmd = "./evilmeta.arcade";
#else
#ifdef RPG_SERVER
			char *cmd = "./evilmeta.rpg";
#else
			char *cmd = "./evilmeta";
#endif
#endif
			char *args[] = {cmd, NULL};

			/* We are the meta client */
			execve(args[0], args, NULL);

			/* GONE */
			printf("exec failed! [%d - %s]\n", errno, strerror(errno));

			exit(-20);
		}
	}
#endif /* WINDOWS */
	/* TODO */
	metapid = 1;
#endif
}

void Check_evilmeta(void)
{
	int status;

	if (metapid == 0)
	{
		Start_evilmeta();
	}

#ifndef WINDOWS
	/* find out what has happened to evilmeta - mikaelh */
	else if (waitpid(metapid, &status, WNOHANG | WUNTRACED) > 0)
	{
		if (WIFEXITED(status) || WIFSIGNALED(status))
		{
			/* evilmeta is dead, restart it */
//			s_printf("evilmeta is dead, restarting it\n");
			metapid = 0;
			Start_evilmeta();
		}
	}
#endif /* WINDOWS */
}

/* update tomenet.acc record structure to a new version - C. Blue
   Done by opening 'tomenet.acc_old' and (over)writing 'tomenet.acc'. */
static bool update_acc_file_version(void) {
        FILE *fp_old, *fp;
	struct account_old c_acc_old;
	struct account c_acc;
        size_t retval;
        int amt = 0;

//	return FALSE; /* security, while not actively used */

	fp_old = fopen("tomenet.acc_old", "rb");

	/* No updating to do?
	   Exit here, if no 'tomenet.acc_old' file exists: */
	if (!fp_old) return(FALSE);
	s_printf("Initiating update of tomenet.acc file.. ");

	fp = fopen("tomenet.acc", "wb");
	if (!fp) {
		s_printf("failed.\n");
		fclose(fp_old);
		return(FALSE);
	}
	s_printf("done.\n");

	if (fp_old && fp) {
		s_printf("Updating tomenet.acc structure.. ");
                while(!feof(fp_old)){
                        retval = fread(&c_acc_old, sizeof(struct account_old), 1, fp_old);
                        if (retval == 0) break; /* EOF reached, nothing read into c_acc - mikaelh */

#if 1
			/* copy unchanged structure parts: */
			c_acc.id = c_acc_old.id;
			strcpy(c_acc.name, c_acc_old.name);
			strcpy(c_acc.pass, c_acc_old.pass);
			c_acc.acc_laston = c_acc_old.acc_laston;
			c_acc.cheeze = c_acc_old.cheeze;
			c_acc.cheeze_self = c_acc_old.cheeze_self;
			c_acc.flags = c_acc_old.flags;
			/* changes/additions: */
			c_acc.deed_event = c_acc.deed_achievement = c_acc.guild_id = 0;
#endif
#if 0
			/* copy unchanged structure parts: */
			c_acc.id = c_acc_old.id;
			strcpy(c_acc.name, c_acc_old.name);
			strcpy(c_acc.pass, c_acc_old.pass);
			c_acc.acc_laston = 0;//c_acc_old.expiry;
			c_acc.cheeze = c_acc_old.cheeze;
			c_acc.cheeze_self = c_acc_old.cheeze_self;
			/* changes/additions: */
			c_acc.flags = (u32b)c_acc_old.flags;
			c_acc.flags |= ACC_GREETED; /* avoid spamming everyone with the noob msg ;-p */
#endif
#if 0
			/* copy unchanged structure parts: */
			c_acc.id = c_acc_old.id;
			c_acc.flags = c_acc_old.flags;
			strcpy(c_acc.name, c_acc_old.name);//30
			strcpy(c_acc.pass, c_acc_old.pass);//20
			/* changes/additions: */
			c_acc.expiry = 0;
			c_acc.cheeze = 0;
			c_acc.cheeze_self = 0;
#endif
//                        fseek(fp, 0L, SEEK_END);
			if (fwrite(&c_acc, sizeof(struct account), 1, fp) < 1) {
				s_printf("Failed to write to new account file: %s\n", feof(fp) ? "EOF" : strerror(ferror(fp)));
			}
			amt++;
                }
		s_printf("%d records updated.\n", amt);
        } else {
		s_printf("Failure: tomenet.acc not updated.\n");
        }
	fclose(fp);
	fclose(fp_old);
	remove("tomenet.acc_old");

        return(TRUE);
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
	if ((Conn = (connection_t **) malloc(size)) == NULL)
		quit("Cannot allocate memory for connections");

	memset(Conn, 0, size);

	/* Tell the metaserver that we're starting up */
	s_printf("Report to meta server:\n");
	Report_to_meta(META_START);
#ifdef EVIL_METACLIENT
	Start_evilmeta();
#endif

	s_printf("%s\n", longVersion);
	s_printf("Server is running version %04x\n", MY_VERSION);
	strcpy(serverStartupTime, showtime());
	s_printf("Current time is %s\n", serverStartupTime);
	time(&cfg.runtime);
	s_printf("Session startup turn is: %d\n", turn);
	session_turn = turn;

	/* Check for updating account file structure to a new version */
	update_acc_file_version();

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
	plog(format("Create TCP socket on port %d...", cfg.game_port));
	while ((Socket = CreateServerSocket(cfg.game_port)) == -1)
	{
#ifdef WINDOWS
		Sleep(1000);
#else
		sleep(1);
#endif
	}
	plog("Set Non-Blocking..."); 
	if (SetSocketNonBlocking(Socket, 1) == -1)
	{
		plog("Can't make contact socket non-blocking");
	}
#ifndef WINDOWS
	/* HACK - Make the socket close-on-exec - mikaelh */
	if (fcntl(Socket, F_SETFD, FD_CLOEXEC) == -1)
	{
		plog("Can't make contact socket close-on-exec");
	}
#endif
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
	if ((SGWSocket = CreateServerSocket(cfg.gw_port)) == -1)
	{
		s_printf("Couldn't create server gateway port\n");
		return;
	}
	if (SetSocketNonBlocking(Socket, 1) == -1)
	{
		plog("Can't make GW socket non-blocking");
	}

	/* Install the new gateway socket */
	install_input(SGWHit, SGWSocket, 0);
#endif
#ifdef TOMENET_WORLDS
	world_connect(-1);
#endif
}

#ifdef TOMENET_WORLDS
void world_connect(int Ind){
	/* evileye testing only */
	/* really, server should DIE if this happens */
	if(WorldSocket!=-1){
		if(Ind!=-1) msg_print(Ind, "\377oAlready connected to the world server");
		return;
	}

	block_timer();
	if((WorldSocket=CreateClientSocket(cfg.wserver, 18360))==-1){
#ifdef WIN32
		s_printf("Unable to connect to world server %d\n", errno);
#else
		s_printf("Unable to connect to world server %d %d\n", errno, sl_errno);
#endif
		if(Ind!=-1) msg_print(Ind, "\377rFailed to connect to the world server");
	}
	else{
		install_input(world_comm, WorldSocket, 0);
		if(Ind!=-1) msg_print(Ind, "\377gSuccessfully connected to the world server");
	}
	allow_timer();
}
#endif

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
static bool player_allowed(char *name){
	FILE *sfp;
	char buffer[80];
	bool success=FALSE;
	/* Hack -- allow 'guest' account */
	/* if (!strcmp("Guest", name)) return TRUE; */

	sfp=fopen("allowlist", "r");
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

/* blacklist of special nicknames unavailable to players (monster names, "Insanity",..) - C. Blue */
static bool forbidden_name(char *name){
	FILE *sfp;
	char buffer[80];
	bool success=FALSE;
	/* Hack -- allow 'guest' account */
	/* if (!strcmp("Guest", name)) return FALSE; */

	/* Note: Character names always start upper-case, so some of these aren't really needed. (Paranoia) */
	/* Hardcode some critically important ones */
	if (!strcmp("server", name)) return TRUE; /* server save file is stored in same folder as player save files */
	if (strstr(name, "guild") && strstr(name, ".data")) return TRUE; /* moved guild hall save files to save folder, from data folder */
	if (strlen(name) >= 5 && name[0] == 's' && name[1] == 'a' && name[2] == 'v' && name[3] == 'e' &&
	    name[4] >= '0' && name[4] <= '9') return TRUE; /* backup save file folders, save00..saveNN */
	if (!strcmp("tomenet.acc", name)) return TRUE; /* just in case someone copies it over into save folder */
	if (!strcmp("Insanity", name)) return TRUE;
#ifdef ENABLE_MAIA
	if (!strcmp("Indecisiveness", name)) return TRUE;
#endif
	/* Hardcode some not so important ones */
	if (!strcmp("tBot", name)) return TRUE; /* Sandman's internal chat bot */
	if (!strcmp("8ball", name)) return TRUE; /* Sandman's internal chat bot */

	sfp=fopen("badnames.txt", "r");
	if(sfp==(FILE*)NULL)
		return FALSE;
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

static void Trim_name(char *nick_name)
{
	char *ptr;
	/* spaces at the beginning are impossible thanks to Check_names */
	/* remove spaces at the end */
	for (ptr = &nick_name[strlen(nick_name)]; ptr-- > nick_name; ) {
		if (isspace(*ptr)) *ptr = '\0';
		else break;
	}
	/* remove special chars that are used for parsing purpose */
	for (ptr = &nick_name[strlen(nick_name)]; ptr-- > nick_name; ) {
		if (!((*ptr >= 'A' && *ptr <= 'Z') ||
		    (*ptr >= 'a' && *ptr <= 'z') ||
		    (*ptr >= '0' && *ptr <= '9') ||
		    strchr(" .,-'`'&_$%~#<>|", *ptr))) /* chars allowed for character name, */
			*ptr = '_'; /* but they become _ in savefile name */
	}
}

/* verify that account, user, host name are valid,
   and that we're resuming from the same IP address if we're resuming  */
static int Check_names(char *nick_name, char *real_name, char *host_name, char *addr, bool check_for_resume)
{
	player_type *p_ptr = NULL;
	int i;

	if (real_name[0] == 0 || host_name[0] == 0 ||
	    nick_name[0] < 'A' || nick_name[0] > 'Z')
		return E_INVAL;

	if (strchr(nick_name, ':')) return E_INVAL;

	if (check_for_resume)
	    for (i = 1; i < NumPlayers + 1; i++) {
		if(Players[i]->conn != NOT_CONNECTED ) {
		    p_ptr = Players[i];
			/*
			 * FIXME: p_ptr->name is character name while nick_name is
			 * account name, so this check always fail.  Evileye? :)
			 */
		    if (strcasecmp(p_ptr->name, nick_name) == 0) {
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
	version_type version_ext;

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

#ifdef WINDOWS
	/* Get the IP address of the client, without using the broken DgramLastAddr() */
	struct sockaddr_in sin;
	int len = sizeof(sin);
	if (getpeername(fd, (struct sockaddr *) &sin, &len) >= 0)
	{
		u32b addr = ntohl(sin.sin_addr.s_addr);
		strnfmt(host_addr, sizeof(host_addr), "%d.%d.%d.%d", (byte)(addr>>24),
			(byte)(addr>>16), (byte)(addr>>8), (byte)addr);
	}
#else
	strcpy(host_addr, DgramLastaddr(fd));
	if(errno==ENOTCONN){	/* will be "0.0.0.0" probably */
		s_printf("Lost connection from unknown peer\n");
		close(fd);
		remove_input(fd);
		return;
	}
#endif

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
	if (version == 0xFFFFU)
	{
		/* Extended version support */
		if (Packet_scanf(&ibuf, "%d%d%d%d%d%d", &version_ext.major, &version_ext.minor, &version_ext.patch, &version_ext.extra, &version_ext.branch, &version_ext.build) <= 0)
		{
			plog(format("Incomplete extended version from %s", host_addr));
			return;
		}
		
		/* Hack: Clients > 4.4.8.1.0.0 also send their binary type
		   (OS they were compiled for), useful for MinGW weirdness
		   in the future, like the LUA crash bug - C. Blue */
		version_ext.os = version_ext.build / 1000;
		version_ext.build %= 1000;
	}
	else
	{
		version_ext.major = version >> 12;
		version_ext.minor = (version >> 8) & 0xF;
		version_ext.patch = (version >> 4) & 0xF;
		version_ext.extra = version & 0xF;
		version_ext.branch = 0;
		version_ext.build = 0;
	}

	nick_name[sizeof(nick_name) - 1] = '\0';
	host_name[sizeof(host_name) - 1] = '\0';

#if 1
	s_printf("Received contact from %s:%d.\n", host_name, port);
	s_printf("Address: %s.\n", host_addr);
	s_printf("Info: real_name %s, port %hu, nick %s, host %s, version %hu\n", real_name, port, nick_name, host_name, version);  
#endif

	/* Replace all weird chars - mikaelh */
	validatestring(real_name);
	validatestring(host_name);

	status = Enter_player(real_name, nick_name, host_addr, host_name,
				&version_ext, port, &login_port, fd);

#if DEBUG_LEVEL > 0
	if (status && status != E_NEED_INFO)
		s_printf("%s: Connection refused(%d).. %s=%s@%s (%s/%d)\n", showtime(),
				status, nick_name, real_name, host_name, host_addr, port);
#endif	// DEBUG_LEVEL

	Sockbuf_clear(&ibuf);

	/* s_printf("Sending login port %d, status %d.\n", login_port, status); */

	if (is_newer_than(&version_ext, 4, 4, 1, 5, 0, 0)) {
		/* Hack - Send server version too - mikaelh */
		Packet_printf(&ibuf, "%c%c%d%d", reply_to, status, login_port, CHAR_CREATION_FLAGS | 0x02);
		Packet_printf(&ibuf, "%d%d%d%d%d%d", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_EXTRA, VERSION_BRANCH, VERSION_BUILD);
	}
	else {
	        Packet_printf(&ibuf, "%c%c%d%d", reply_to, status, login_port, CHAR_CREATION_FLAGS);
	}

/* -- DGDGDGDG it would be NEAT to have classes sent to the cleint at conenciton, sadly Im too clumpsy at network code ..
        for (i = 0; i < MAX_CLASS; i++)
        {
                 Packet_printf(&ibuf, "%c%s", i, class_info[i].title);
        }
*/
	Reply(host_addr, fd);
}

static int Enter_player(char *real, char *nick, char *addr, char *host,
				version_type *version, int port, int *login_port, int fd)
{
	//int status;

	*login_port = 0;

#if 0
	if(!validstrings(nick, real, host))
#else
	/* Only check the account name for weird chars - mikaelh */
	if(!validstring(nick))
#endif
		return E_INVAL;

	if (NumPlayers >= max_connections)
		return E_GAME_FULL;

#if 0	/* This would pass in the account name rather than the
	   player's character name. Also, we must *always* allow
	   a second account login - it may be a subsequent resume.
	   We can check duplicate account use on player entry
	   (PKT_LOGIN) */
	if ((status = Check_names(nick, real, host, addr, TRUE)) != SUCCESS)
	{
		/*s_printf("Check_names failed with result %d.\n", status);*/
		return status;
	}
#endif

#if 0
	if (version < MY_VERSION)
		return E_VERSION_OLD;
	if (version > MY_VERSION)
		return E_VERSION_UNKNOWN;
#else
	/* Extended version support */
	if (version->major < MIN_VERSION_MAJOR)
		return E_VERSION_OLD;
	else if (version->major > MAX_VERSION_MAJOR)
		return E_VERSION_UNKNOWN;
	else if (version->minor < MIN_VERSION_MINOR)
		return E_VERSION_OLD;
	else if (version->minor > MAX_VERSION_MINOR)
		return E_VERSION_UNKNOWN;
	else if (version->patch < MIN_VERSION_PATCH)
		return E_VERSION_OLD;
	else if (version->patch > MAX_VERSION_PATCH)
		return E_VERSION_UNKNOWN;
	else if (version->extra < MIN_VERSION_EXTRA)
		return E_VERSION_OLD;
	/* Note: Clients with newer extra value are allowed */
#endif

	if(!player_allowed(nick))
		return E_INVITE;

	if(in_banlist(addr)) return(E_BANNED);

	*login_port = Setup_connection(real, nick, addr, host, version, fd);

	if (*login_port == -1)
		return E_SOCKET;

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
	inventory_change_type *inv_change;

	/* terminate mindcrafter charm effect */
	do_mstopcharm(Ind);

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

#ifdef ENABLE_RCRAFT
	/* remove all rune traps of this player */
	if (p_ptr->runetraps) {
		cave_type *c_ptr;
		struct c_special *cs_ptr;
		int f;
		if (zcave) {
			for (i = 0; i < p_ptr->runetraps; i++) {
				c_ptr = &zcave[p_ptr->runetrap_y[i]][p_ptr->runetrap_x[i]];
				if (c_ptr->feat != FEAT_RUNE_TRAP) continue; /* paranoia */
				if (!(cs_ptr = GetCS(c_ptr, CS_RUNE_TRAP))) continue; /* paranoia */

				f = cs_ptr->sc.runetrap.feat;
				cs_erase(c_ptr, cs_ptr);
				cave_set_feat_live(&p_ptr->wpos, p_ptr->runetrap_y[i], p_ptr->runetrap_x[i], f);
			}
		}
		remove_rune_trap_upkeep(Ind, 0, -1, -1);
	}
#endif

	/* If (s)he was in a game team, remove him/her - mikaelh */
	if (p_ptr->team != 0)
	{
		teams[p_ptr->team - 1]--;
		p_ptr->team = 0;
	}

	/* Also remove hostility if (s)he was blood bonded - mikaelh */
	if (p_ptr->blood_bond)
	{
#if 0
		remove_hostility(Ind, lookup_player_name(p_ptr->blood_bond));

		i = find_player(p_ptr->blood_bond);
		if (i)
		{
			remove_hostility(i, p_ptr->name);
			Players[i]->blood_bond = 0;
		}

		p_ptr->blood_bond = 0;
#else
		player_list_type *pl_ptr, *tmp;
		pl_ptr = p_ptr->blood_bond;

		while (pl_ptr)
		{
			/* Remove hostility */
			remove_hostility(Ind, lookup_player_name(pl_ptr->id));

			i = find_player(pl_ptr->id);
			if (i)
			{
				/* Remove hostility and blood bond from the other player */
				remove_hostility(i, p_ptr->name);
				remove_blood_bond(i, Ind);
			}

			tmp = pl_ptr;
			pl_ptr = pl_ptr->next;
			FREE(tmp, player_list_type);
		}

		/* The list is gone now */
		p_ptr->blood_bond = NULL;
#endif
	}

	/* Remove ignores - mikaelh */
#if 0
	if (p_ptr->ignore)
	{
		hostile_type *h_ptr, *tmp;

		h_ptr = p_ptr->ignore;

		while (h_ptr)
		{
			tmp = h_ptr;
			h_ptr = h_ptr->next;
			FREE(tmp, hostile_type);
		}
	}
#else
	/* Make use of the new player_list_free */
	player_list_free(p_ptr->ignore);
#endif

	/* Remove him from everyone's afk_noticed if he was AFK */
	if (p_ptr->afk)
		for (i = 1; i <= NumPlayers; i++)
			player_list_del(&Players[i]->afk_noticed, p_ptr->id);

	/* Free afk_noticed - mikaelh */
	player_list_free(p_ptr->afk_noticed);

	/* Free inventory changes - mikaelh */
	inv_change = p_ptr->inventory_changes;
	while (inv_change) {
		inventory_change_type *free_change = inv_change;
		inv_change = inv_change->next;
		KILL(free_change, inventory_change_type);
	}
	p_ptr->inventory_changes = NULL;

	/* Try to save his character */
	save_player(Ind);

	/* If he was actively playing, tell everyone that he's left */
	/* handle the cfg_secret_dungeon_master option */
	if (p_ptr->alive && !p_ptr->death) {
		if (!p_ptr->admin_dm || !cfg.secret_dungeon_master) {
			cptr title = "";
			if (p_ptr->total_winner) {
				if (p_ptr->mode & (MODE_HARD | MODE_NO_GHOST)) {
					title = (p_ptr->male)?"Emperor ":"Empress ";
				} else {
					title = (p_ptr->male)?"King ":"Queen ";
				}
			}
			if (p_ptr->admin_dm) title = (p_ptr->male)?"Dungeon Master ":"Dungeon Mistress ";
			if (p_ptr->admin_wiz) title = "Dungeon Wizard ";

#ifdef TOMENET_WORLDS /* idea: maybe use the 'quiet' flag as 'dungeon master' flag instead? */
			world_player(p_ptr->id, p_ptr->name, FALSE, TRUE); /* last flag is 'quiet' mode -> no public msg */
#endif

			for (i = 1; i < NumPlayers + 1; i++)
			{
				if (Players[i]->conn == NOT_CONNECTED)
					continue;

				/* Don't tell him about himself */
				if (i == Ind) continue;

				/* Send a little message */
				msg_format(i, "\374\377%c%s%s has left the game.", COLOUR_SERVER, title, p_ptr->name);
			}
#ifdef TOMENET_WORLDS
			if (cfg.worldd_pleave) world_msg(format("\374\377%c%s%s has left the game.", COLOUR_SERVER, title, p_ptr->name));
#endif
		} else {
			cptr title = "";
			if (p_ptr->admin_dm) title = (p_ptr->male)?"Dungeon Master ":"Dungeon Mistress ";
			if (p_ptr->admin_wiz) title = "Dungeon Wizard ";
#if 0 /* Don't show admins in the list!! Reenable this when 'quiet' flag got reworked into 'dm' flag or sth. */
#ifdef TOMENET_WORLDS
			world_player(p_ptr->id, p_ptr->name, FALSE, TRUE); /* last flag is 'quiet' mode -> no public msg */
#endif
#endif
			for (i = 1; i < NumPlayers + 1; i++) {
				if (Players[i]->conn == NOT_CONNECTED)
					continue;
				if (!is_admin(Players[i]))
					continue;

				/* Don't tell him about himself */
				if (i == Ind) continue;

				/* Send a little message */
				msg_format(i, "\374\377%c%s%s has left the game.", COLOUR_SERVER, title, p_ptr->name);
				/* missing TOMENET_WORLDS relay here :/ (currently no way to send to 'foreign' admins only) - C. Blue */
			}
		}
	}

#ifdef AUCTION_SYSTEM
	/* Save his/her money in the hash table */
	clockin(Ind, 5);
#endif

	if (p_ptr->esp_link_type && p_ptr->esp_link) {
		/* This is the last chance to get out!!! */
		int Ind2 = find_player(p_ptr->esp_link);
		if (Ind2) end_mind(Ind2, TRUE);
	}


	/* Swap entry number 'Ind' with the last one */
	/* Also, update the "player_index" on the cave grids */
	if (Ind != NumPlayers) {
		cave_type **zcave;
		worldpos *wpos = &Players[NumPlayers]->wpos;
		p_ptr			= Players[NumPlayers];
		if((zcave=getcave(&p_ptr->wpos)))
			zcave[p_ptr->py][p_ptr->px].m_idx = 0 - Ind;
		Players[NumPlayers]	= Players[Ind];
		Players[Ind]		= p_ptr;
		cave_midx_debug(wpos, p_ptr->py, p_ptr->px, -Ind);
		p_ptr			= Players[NumPlayers];
	}

	if (Conn[Players[Ind]->conn]->id != -1)
		GetInd[Conn[Players[Ind]->conn]->id] = Ind;
	if (Conn[Players[NumPlayers]->conn]->id != -1)
		GetInd[Conn[Players[NumPlayers]->conn]->id] = NumPlayers;

	/* Recalculate player-player visibility */
	update_players();

	if (!is_admin(p_ptr)) NumNonAdminPlayers--;

	if (p_ptr) {
		if (p_ptr->inventory)
			C_FREE(p_ptr->inventory, INVEN_TOTAL, object_type);
		if (p_ptr->inventory_copy)
			C_FREE(p_ptr->inventory_copy, INVEN_TOTAL, object_type);

		KILL(Players[NumPlayers], player_type);
	}

	NumPlayers--;

	/* Update Morgoth eventually if the player was on his level */
	check_Morgoth(0);

	/* Tell the metaserver about the loss of a player */	
	Report_to_meta(META_UPDATE);
}


/*
 * Cleanup a connection.  The client may not know yet that it is thrown out of
 * the game so we send it a quit packet if our connection to it has not already
 * closed.  If our connection to it has been closed, then connp->w.sock will
 * be set to -1.
 */
bool Destroy_connection(int ind, char *reason_orig)
{
	connection_t	*connp = Conn[ind];
	int		id, len, sock;
	char		pkt[MAX_CHARS];
	char		*reason;
	int		i, player = 0;
	char		traffic[50+1];
	player_type	*p_ptr = NULL;

	/* reason was probably made using format() which uses a static buffer so copy it - mikaelh */
	reason = (char*)string_make(reason_orig);

	kill_xfers(ind);	/* don't waste time sending to a dead
				   connection ( or crash! ) */

	if (!connp || connp->state == CONN_FREE) {
		errno = 0;
		plog(format("Cannot destroy empty connection (\"%s\")", reason));
		string_free(reason);
		return FALSE;
	}

	if (connp->id != -1) {
		exec_lua(0, format("player_leaves(%d, %d, \"%/s\", \"%s\")", GetInd[connp->id], connp->id, connp->c_name, showtime()));

		/* in case winners CAN hold arts as long as they don't leave the floor (default): */
		//lua_strip_true_arts_from_present_player(GetInd[connp->id], int mode)
	} else
		exec_lua(0, format("player_leaves(%d, %d, \"%/s\", \"%s\")", 0, connp->id, connp->c_name, showtime()));

	sock = connp->w.sock;
	if (sock != -1) remove_input(sock);

	strncpy(&pkt[1], reason, sizeof(pkt) - 3);
	pkt[sizeof(pkt) - 2] = '\0';
	pkt[0] = PKT_QUIT;
	len = strlen(pkt) + 2;
	pkt[len - 1] = PKT_END;
	pkt[len] = '\0';
	/*len++;*/

	if (sock != -1) {
#if 1	// sorry evileye, removing it causes SIGPIPE to the client

		if (DgramWrite(sock, pkt, len) != len) {
			GetSocketError(sock);
//maybe remove this one too? Or have its error be cleared too? - C. Blue
//    			DgramWrite(sock, pkt, len);
		}
#endif
	}

	if (connp->id != -1) {
		player = GetInd[connp->id];
		p_ptr = Players[player];
	}
	if (p_ptr)
		s_printf("%s: Goodbye %s(%s)=%s@%s (\"%s\") (Ind=%d,ind=%d;wpos=%d,%d,%d;xy=%d,%d)\n",
		    showtime(),
		    connp->c_name ? connp->c_name : "",
		    connp->nick ? connp->nick : "",
		    connp->real ? connp->real : "",
		    connp->host ? connp->host : "",
		    reason, player, ind,
		    p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz,
		    p_ptr->px, p_ptr->py);
	else
		s_printf("%s: Goodbye %s(%s)=%s@%s (\"%s\") (Ind=%d,ind=%d;wpos=-,-,-;xy=-,-)\n",
		    showtime(),
		    connp->c_name ? connp->c_name : "",
		    connp->nick ? connp->nick : "",
		    connp->real ? connp->real : "",
		    connp->host ? connp->host : "",
		    reason, player, ind);

	Conn_set_state(connp, CONN_FREE, CONN_FREE);

	if (connp->id != -1) {
		id = connp->id;
		connp->id = -1;
	/*
		Players[GetInd[id]]->conn = NOT_CONNECTED;
	*/
		Delete_player(GetInd[id]);
	}

	exec_lua(0, format("player_has_left(%d, %d, \"%/s\", \"%s\")", player, connp->id, connp->c_name, showtime()));
	if (NumPlayers == 0) exec_lua(0, format("last_player_has_left(%d, %d, \"%/s\", \"%s\")", player, connp->id, connp->c_name, showtime()));
	strcpy(traffic, "");
	for (i = 1; (i <= NumPlayers) && (i < 50); i++)
		if (!(i % 5)) strcat(traffic, "* "); else strcat(traffic, "*");
	p_printf("%s -  %03d  %s\n", showtime(), NumPlayers, traffic);

	if (connp->real != NULL) free(connp->real);
	if (connp->nick != NULL) free(connp->nick);
	if (connp->addr != NULL) free(connp->addr);
	if (connp->host != NULL) free(connp->host);
	if (connp->c_name != NULL) free(connp->c_name);
	Sockbuf_cleanup(&connp->w);
	Sockbuf_cleanup(&connp->r);
	Sockbuf_cleanup(&connp->c);
	Sockbuf_cleanup(&connp->q);
	memset(connp, 0, sizeof(*connp));

	/* Free the connection_t structure - mikaelh */
	KILL(Conn[ind], connection_t);
	connp = NULL;

	num_logouts++;

	if (sock != -1) DgramClose(sock);

	string_free(reason);

	return TRUE;
}

int Check_connection(char *real, char *nick, char *addr)
{
	int i;
	connection_t *connp;

	for (i = 0; i < max_connections; i++)
	{
		connp = Conn[i];
		if (connp && connp->state == CONN_LISTENING)
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
			version_type *version, int fd)
{
	int i, free_conn_index = max_connections, my_port, sock;
	connection_t *connp;

	for (i = 0; i < max_connections; i++)
	{
		connp = Conn[i];
		if (!connp || connp->state == CONN_FREE)
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

	/* Allocate the connection_t structure - mikaelh */
	MAKE(Conn[free_conn_index], connection_t);

	connp = Conn[free_conn_index];

	if (connp == NULL)
	{
		plog("Not enough memory for connection");
		Destroy_connection(free_conn_index, "Server is out of memory.");
		return -1;
	}

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
#if 0
	connp->version = version;
#else
	/* Extended version support */
	memcpy(&connp->version, version, sizeof(version_type));
#endif
	connp->start = turn;
	connp->magic = rand() + my_port + sock + turn;
	connp->id = -1;
	connp->timeout = LISTEN_TIMEOUT;
/* - not used - mikaelh
	connp->reliable_offset = 0;
	connp->reliable_unsent = 0;
	connp->last_send_loops = 0;
	connp->retransmit_at_loop = 0;
	connp->rtt_retransmit = DEFAULT_RETRANSMIT;
	connp->rtt_smoothed = 0;
	connp->rtt_dev = 0;
	connp->rtt_timeouts = 0;
*/
	connp->acks = 0;
	connp->setup = 0;
	connp->password_verified = FALSE;
	Conn_set_state(connp, CONN_LISTENING, CONN_FREE);
	if (connp->w.buf == NULL || connp->r.buf == NULL || connp->c.buf == NULL
		|| connp->q.buf == NULL || connp->real == NULL || connp->nick == NULL
		|| connp->addr == NULL || connp->host == NULL)
	{
		plog("Not enough memory for connection");
		Destroy_connection(free_conn_index, "Server is out of memory.");
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
	connection_t *connp = Conn[ind];
	char *buf;
	int n, len, i, j;
	char b1, b2, b3, b4, b5, b6;
	
	if (connp->state != CONN_SETUP)
	{
		Destroy_connection(ind, "not setup");
		return -1;
	}

	if (connp->setup == 0) {
		if (is_newer_than(&connp->version, 4, 4, 5, 10, 0, 0))
			n = Packet_printf(&connp->c, "%ld%hd%c%c%c%ld",
			    Setup.motd_len, Setup.frames_per_second, Setup.max_race, Setup.max_class, Setup.max_trait, Setup.setup_size);
		else
			n = Packet_printf(&connp->c, "%ld%hd%c%c%ld",
			    Setup.motd_len, Setup.frames_per_second, Setup.max_race, Setup.max_class, Setup.setup_size);

		if (n <= 0)
		{
			Destroy_connection(ind, "Setup 0 write error");
			return -1;
		}

		for (i = 0; i < Setup.max_race; i++) {
//			Packet_printf(&ibuf, "%c%s", i, class_info[i].title);
//			Packet_printf(&connp->c, "%s%d", Setup.race_title[i], Setup.race_choice[i]);
			b1 = race_info[i].r_adj[0]+50;
			b2 = race_info[i].r_adj[1]+50;
			b3 = race_info[i].r_adj[2]+50;
			b4 = race_info[i].r_adj[3]+50;
			b5 = race_info[i].r_adj[4]+50;
			b6 = race_info[i].r_adj[5]+50;
			Packet_printf(&connp->c, "%c%c%c%c%c%c%s%d", b1, b2, b3, b4, b5, b6, race_info[i].title, race_info[i].choice);
		}

		for (i = 0; i < Setup.max_class; i++) {
//			Packet_printf(&ibuf, "%c%s", i, class_info[i].title);
//			Packet_printf(&connp->c, "%s", Setup.class_title[i]);
			b1 = class_info[i].c_adj[0]+50;
			b2 = class_info[i].c_adj[1]+50;
			b3 = class_info[i].c_adj[2]+50;
			b4 = class_info[i].c_adj[3]+50;
			b5 = class_info[i].c_adj[4]+50;
			b6 = class_info[i].c_adj[5]+50;
			Packet_printf(&connp->c, "%c%c%c%c%c%c%s", b1, b2, b3, b4, b5, b6, class_info[i].title);
			if (is_newer_than(&connp->version, 4, 4, 3, 1, 0, 0))
				for (j = 0; j < 6; j++)
					Packet_printf(&connp->c, "%c", class_info[i].min_recommend[j]);
		}

		if (is_newer_than(&connp->version, 4, 4, 5, 10, 0, 0))
		for (i = 0; i < Setup.max_trait; i++)
			Packet_printf(&connp->c, "%s%d", trait_info[i].title, trait_info[i].choice);

		connp->setup = (char *) &Setup.motd[0] - (char *) &Setup;
		connp->setup = 0;
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
 * No spaces/strange characters in the account name, 
 * real name or hostname.
 */
#if 0
static bool validstrings(char *nick, char *real, char *host){
	int i;
	int rval=1;

	for(i=0; nick[i]; i++){
		if(nick[i]<32 || nick[i]>'z'){
			nick[i]='\0';
			rval=0;
		}
	}
	for(i=0; real[i]; i++){
		if(real[i]<32 || real[i]>'z'){
			real[i]='\0';
			rval=0;
		}
	}
	for(i=0; host[i]; i++){
		if(host[i]<32 || host[i]>'z'){
			host[i]='\0';
			rval=0;
		}
	}
	return(rval);
}
#else
static bool validstring(char *nick) {
	int i, rval = 1;

	for (i = 0; nick[i]; i++) {
		if (nick[i] < 32 || nick[i] > 'z') {
			nick[i] = '\0';
			rval = 0;
		}
	}

	return(rval);
}
#endif

/*
 * Replace all weird chars with underscores
 * Alternate to validstrings() - mikaelh
 */
void validatestring(char *string) {
	int i;

	for (i = 0; string[i]; i++) {
		if (string[i] < 32 || string[i] > 'z') {
			string[i] = '_';
		}
	}
}

/*
 * Handle a connection that is in the listening state.
 */
static int Handle_listening(int ind)
{
	connection_t *connp = Conn[ind];
	unsigned char type;
	int  n, oldlen;
	char nick[MAX_CHARS], real[MAX_CHARS], pass[MAX_CHARS];
	version_type *version = &connp->version;

	if (connp->state != CONN_LISTENING)
	{
		Destroy_connection(ind, "not listening");
		return -1;
	}
	errno = 0;

	/* Some data has arrived on the socket.  Read this data into r.buf.
	 */
	oldlen = connp->r.len;
	n = Sockbuf_read(&connp->r);
	if (n - oldlen <= 0)
	{
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

	if ((n = Packet_scanf(&connp->r, "%c%s%s%s", &type, real, nick, pass)) <= 0)
	{
		Send_reply(ind, PKT_VERIFY, PKT_FAILURE);
		Send_reliable(ind);
		Destroy_connection(ind, "verify broken");
		return -1;
	}

	/*
	 * It's quite doubtful if it's 2654 bytes, since MAX_R_IDX and MAX_K_IDX
	 * etc are much bigger than before;  however, let's follow the saying
	 * 'Never touch what works' ;)		- Jir -
	 */

	/* Log the players connection */
	s_printf("%s: Welcome %s=%s@%s (%s/%d) (NP=%d,ind=%d)", showtime(), connp->nick,
		connp->real, connp->host, connp->addr, connp->his_port, NumPlayers, ind);
#if 0
	if (connp->version != MY_VERSION)
		s_printf(" (version %04x)", connp->version);
#else
	/* Extended version support */
	s_printf(" (version %d.%d.%d.%d branch %d build %d, os %d)", version->major, version->minor, version->patch, version->extra, version->branch, version->build, version->os);
#endif
	s_printf("\n");

	if (strcmp(real, connp->real))
	{
		s_printf("Client verified incorrectly (%s, %s)(%s, %s)\n",
			real, nick, connp->real, connp->nick);
		Send_reply(ind, PKT_VERIFY, PKT_FAILURE);
		Send_reliable(ind);
		Destroy_connection(ind, "verify incorrect");
		return -1;
	}
	
	/* Set his character info */
	connp->pass = strdup(pass);

	Sockbuf_clear(&connp->w);
	if (Send_reply(ind, PKT_VERIFY, PKT_SUCCESS) == -1
		|| Packet_printf(&connp->c, "%c%u", PKT_MAGIC, connp->magic) <= 0
		|| Send_reliable(ind) <= 0)
	{
		Destroy_connection(ind, "confirm failed");
		return -1;
	}

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

	bool tmp;
	int i;

	/* Do the dirty work */
#if 0
	p_ptr->carry_query_flag = options[3];
#else
	if (is_older_than(&p_ptr->version, 4, 4, 8, 2, 0, 0))
		p_ptr->newbie_hints = TRUE;
	else {
		tmp = p_ptr->newbie_hints;
		p_ptr->newbie_hints = options[3];

		/* disable some or all newbie hints */
		if (!p_ptr->newbie_hints) disable_specific_warnings(p_ptr);
		else if (!tmp) msg_print(Ind, "\374\377yEnabling newbie hints requires you to exit and log in again.");
	}
#endif
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
	p_ptr->alert_hitpoints = options[26];
	p_ptr->alert_afk_dam = options[27];
	p_ptr->auto_afk = options[28];
	p_ptr->newb_suicide = options[29];
	p_ptr->stack_allow_items = options[30];
	p_ptr->stack_allow_wands = options[31];

	tmp = p_ptr->view_perma_grids;
	if ((p_ptr->view_perma_grids = options[34]) != tmp) p_ptr->redraw |= PR_MAP;
	tmp = p_ptr->view_torch_grids;
	if ((p_ptr->view_torch_grids = options[35]) != tmp) p_ptr->redraw |= PR_MAP;
	tmp = p_ptr->view_reduce_lite;
	if ((p_ptr->view_reduce_lite = options[44]) != tmp) p_ptr->redraw |= PR_MAP;
	tmp = p_ptr->view_reduce_view;
	if ((p_ptr->view_reduce_view = options[45]) != tmp) p_ptr->redraw |= PR_MAP;

	p_ptr->safe_float = options[46];

	if (is_older_than(&p_ptr->version, 4, 4, 8, 4, 0, 0))
		p_ptr->censor_swearing = TRUE;
	else
		p_ptr->censor_swearing = options[53];

	tmp = p_ptr->view_yellow_lite;
	if ((p_ptr->view_yellow_lite = options[56]) != tmp) p_ptr->redraw |= PR_MAP;
	tmp = p_ptr->view_bright_lite;
	if ((p_ptr->view_bright_lite = options[57]) != tmp) p_ptr->redraw |= PR_MAP;
	tmp = p_ptr->view_granite_lite;
	if ((p_ptr->view_granite_lite = options[58]) != tmp) p_ptr->redraw |= PR_MAP;
	tmp = p_ptr->view_special_lite;
	if ((p_ptr->view_special_lite = options[59]) != tmp) p_ptr->redraw |= PR_MAP;

	p_ptr->easy_open = options[60];
	p_ptr->easy_disarm = options[61];
	p_ptr->easy_tunnel = options[62];
//	p_ptr->auto_destroy = options[63];
	p_ptr->clear_inscr = options[63];
	p_ptr->auto_inscribe = options[64];
	p_ptr->taciturn_messages = options[65];
	p_ptr->last_words = options[66];
	p_ptr->limit_chat = options[67];

	tmp = p_ptr->depth_in_feet;
	if ((p_ptr->depth_in_feet = options[7]) != tmp)
		p_ptr->redraw |= PR_DEPTH;

	p_ptr->auto_target = options[69];
	p_ptr->autooff_retaliator = options[70];
	p_ptr->wide_scroll_margin = options[71];
	p_ptr->always_repeat = options[6];
	p_ptr->fail_no_melee = options[72];

	tmp = p_ptr->short_item_names;
	if ((p_ptr->short_item_names = options[77]) != tmp) {
		/* update inventory */
		for (i = 0; i < INVEN_WIELD; i++)
			WIPE(&p_ptr->inventory_copy[i], object_type);
		p_ptr->window |= PW_INVEN;
	}

	// bool speak_unique;

	p_ptr->page_on_privmsg = options[40];
	p_ptr->page_on_afk_privmsg = options[41];
	p_ptr->auto_untag = options[42];
	/* hack: if client doesn't know player_list options yet then assume full list (old) */
	if (is_older_than(&p_ptr->version, 4, 4, 7, 1, 0, 0)) {
		p_ptr->player_list = FALSE;
		p_ptr->player_list2 = FALSE;
	} else {
		p_ptr->player_list = options[50];
		p_ptr->player_list2 = options[51];
	}
	p_ptr->cut_sfx_attack = options[87];
}

/*
 * A client has requested to start active play.
 * See if we can allocate a player structure for it
 * and if this succeeds update the player information
 * to all connected players.
 */
static int Handle_login(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	object_type forge, *o_ptr = &forge;
	int i, j;
	bool options[OPT_MAX], greeting;
	char namebuf1[80], namebuf2[80];
	cptr title = "";
	char traffic[50+1];

	if (Id >= MAX_ID) {
		errno = 0;
		plog(format("Id too big (%d)", Id));
		return -1;
	}

	/* This will cause problems for account/char with same name */
#if 0
	for (i = 1; i < NumPlayers + 1; i++) {
		if (strcasecmp(Players[i]->name, connp->nick) == 0) {
			errno = 0;
			plog(format("Name already in use %s", connp->nick));
			return -1;
		}
	}
#endif

	if (!player_birth(NumPlayers + 1, connp->nick, connp->c_name, ind, connp->race, connp->class, connp->trait, connp->sex, connp->stat_order))
	{
		/* Failed, connection destroyed */
		Destroy_connection(ind, "not login");
		return -1;
	}
	p_ptr = Players[NumPlayers + 1];
	strcpy(p_ptr->realname, connp->real);
	strncpy(p_ptr->hostname, connp->host, 25); /* cap ridiculously long hostnames - C. Blue */
	strcpy(p_ptr->accountname, connp->nick);
	strcpy(p_ptr->addr, connp->addr);
	p_ptr->version = connp->version; /* this actually copies the extended version structure */
	p_ptr->v_unknown = is_newer_than(&p_ptr->version, VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_EXTRA, VERSION_BRANCH, VERSION_BUILD);
	p_ptr->v_test = !p_ptr->v_unknown && is_newer_than(&p_ptr->version, VERSION_MAJOR_LATEST, VERSION_MINOR_LATEST, VERSION_PATCH_LATEST, VERSION_EXTRA_LATEST, VERSION_BRANCH_LATEST, VERSION_BUILD_LATEST);
	p_ptr->v_outdated = !is_newer_than(&p_ptr->version, VERSION_MAJOR_OUTDATED, VERSION_MINOR_OUTDATED, VERSION_PATCH_OUTDATED, VERSION_EXTRA_OUTDATED, VERSION_BRANCH_OUTDATED, VERSION_BUILD_OUTDATED);
	p_ptr->v_latest = is_same_as(&p_ptr->version, VERSION_MAJOR_LATEST, VERSION_MINOR_LATEST, VERSION_PATCH_LATEST, VERSION_EXTRA_LATEST, VERSION_BRANCH_LATEST, VERSION_BUILD_LATEST);
	p_ptr->audio_sfx = connp->audio_sfx;
	p_ptr->audio_mus = connp->audio_mus;

	/* Copy the client preferences to the player struct */
	for (i = 0; i < OPT_MAX; i++)
		options[i] = connp->Client_setup.options[i];

	for (i = 0; i < TV_MAX; i++) {
		int j;

		if (!connp->Client_setup.u_attr[i] &&
		    !connp->Client_setup.u_char[i])
			continue;

		/* XXX not max_k_idx, since client doesn't know the value */
		for (j = 0; j < MAX_K_IDX; j++) {
			if (k_info[j].tval == i) {
				p_ptr->d_attr[j] = connp->Client_setup.u_attr[i];
				p_ptr->d_char[j] = connp->Client_setup.u_char[i];
			}
		}
	}

	for (i = 0; i < MAX_F_IDX; i++) {
/* Hacking the no-floor bug for loth/bree tower/gondo city -C. Blue */
		p_ptr->f_attr[i] = connp->Client_setup.f_attr[i];
		p_ptr->f_char[i] = connp->Client_setup.f_char[i];
#ifndef FLOORTILEBUG_WORKAROUND
		if (!p_ptr->f_attr[i]) p_ptr->f_attr[i] = f_info[i].z_attr;
		if (!p_ptr->f_char[i]) p_ptr->f_char[i] = f_info[i].z_char;
#else		/*now all tiles are bright white and never dimmed.*/
		p_ptr->f_attr[i] = f_info[i].z_attr;
		p_ptr->f_char[i] = f_info[i].z_char;
#endif
	}

	for (i = 0; i < MAX_K_IDX; i++) {
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

	for (i = 0; i < MAX_R_IDX; i++) {
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
	if (!is_admin(p_ptr)) NumNonAdminPlayers++;
	if (NumNonAdminPlayers > MaxSimultaneousPlayers) {
		MaxSimultaneousPlayers = NumPlayers;
		if (MaxSimultaneousPlayers > 15) s_printf("SimultaneousPlayers (above 15): %d\n", MaxSimultaneousPlayers);
	}
	connp->id = Id++;

	//Conn_set_state(connp, CONN_READY, CONN_PLAYING);
	Conn_set_state(connp, CONN_PLAYING, CONN_PLAYING);

	if (Send_reply(ind, PKT_PLAY, PKT_SUCCESS) <= 0) {
		plog("Cannot send play reply");
		return -1;
	}

	/* Send party/guild information */
	Send_party(NumPlayers, FALSE, FALSE);
	Send_guild(NumPlayers, FALSE, FALSE);

	/* Hack -- terminate the data stream sent to the client */
	if (Packet_printf(&connp->c, "%c", PKT_END) <= 0) {
		Destroy_connection(p_ptr->conn, "write error");
		return -1;
	}

	if (Send_reliable(ind) == -1) {
		Destroy_connection(ind, "Couldn't send reliable data");
		return -1;
	}

	num_logins++;

	save_server_info();

	/* Execute custom script if player joins the server */
	if (first_player_joined) {
		first_player_joined = FALSE;
		exec_lua(NumPlayers, format("first_player_has_joined(%d, %d, \"%/s\", \"%s\")", NumPlayers, p_ptr->id, p_ptr->name, showtime()));
	}
	if (NumPlayers == 1) exec_lua(NumPlayers, format("player_has_joined_empty_server(%d, %d, \"%/s\", \"%s\")", NumPlayers, p_ptr->id, p_ptr->name, showtime()));
	exec_lua(NumPlayers, format("player_has_joined(%d, %d, \"%/s\", \"%s\")", NumPlayers, p_ptr->id, p_ptr->name, showtime()));
	strcpy(traffic, "");
	for (i = 1; (i <= NumPlayers) && (i < 50); i++)
		if (!(i % 5)) strcat(traffic, "* "); else strcat(traffic, "*");
	p_printf("%s +  %03d  %s\n", showtime(), NumPlayers, traffic);

	/* Initialize his mimic spells. - C. Blue
	   Note: This is actually done earlier in time via calc_body_bonus(),
	   but at that point, the connection is not yet ready to receive spell info. */
	calc_body_spells(NumPlayers);

	/* check pending notes to this player -C. Blue */
	for (i = 0; i < MAX_ADMINNOTES; i++) {
		if (strcmp(admin_note[i], "")) {
			msg_format(NumPlayers, "\377sMotD: %s", admin_note[i]);
		}
	}
	for (i = 0; i < MAX_NOTES; i++) {
		strcpy(namebuf1, priv_note_target[i]);
		strcpy(namebuf2, connp->nick);
		j = 0; while(namebuf1[j]) { namebuf1[j] = tolower(namebuf1[j]); j++; }
		j = 0; while(namebuf2[j]) { namebuf2[j] = tolower(namebuf2[j]); j++; }
//		if (!strcmp(priv_note_target[i], p_ptr->name)) { /* <- sent to a character name */
//		if (!strcmp(priv_note_target[i], connp->nick)) { /* <- sent to an account name */
		if (!strcmp(namebuf1, namebuf2)) { /* <- sent to an account name, case-independant */
			msg_format(NumPlayers, "\374\377RNote from %s: %s", priv_note_sender[i], priv_note[i]);
			strcpy(priv_note_sender[i], "");
			strcpy(priv_note_target[i], "");
			strcpy(priv_note[i], "");
		}
	}
	if (p_ptr->party) for (i = 0; i < MAX_PARTYNOTES; i++) {
		if (!strcmp(party_note_target[i], parties[p_ptr->party].name)) {
			if (strcmp(party_note[i], ""))
				msg_format(NumPlayers, "\374\377bParty Note: %s", party_note[i]);
			break;
		}
	}
	if (p_ptr->guild) for (i = 0; i < MAX_GUILDNOTES; i++) {
		if (!strcmp(guild_note_target[i], guilds[p_ptr->guild].name)) {
			if (strcmp(guild_note[i], ""))
				msg_format(NumPlayers, "\374\377bGuild Note: %s", guild_note[i]);
			break;
		}
	}
	if (server_warning[0]) msg_format(NumPlayers, "\374\377R*** Note: %s ***", server_warning);

	/* Warn the player if some of his/her characters are about to expire */
	account_checkexpiry(NumPlayers);

#ifndef ARCADE_SERVER
	/* Brand-new players get super-short instructions presented here: */
	greeting = !(acc_get_flags(p_ptr->accountname) & ACC_GREETED);
	if (p_ptr->inval || greeting) {
		s_printf("GREETING: %s\n", p_ptr->name);
		if (i) acc_set_flags(p_ptr->accountname, ACC_GREETED, TRUE);
		/* no bloody noob ever seems to read this how2run thingy.. (p_ptr->warning_welcome) */
		msg_print(NumPlayers, "\374\377y ");
		msg_print(NumPlayers, "\374\377y   ***  Welcome to Tomenet! You can chat with \377R:\377y key. Say hello :)  ***");
		msg_print(NumPlayers, "\374\377y      To run fast, use \377RSHIFT + direction\377y keys (numlock must be OFF)");
		if (p_ptr->warning_wield == 0)
			msg_print(NumPlayers, "\374\377y      Before you move out, press \377Rw\377y to equip your weapon and armour!");
		else
			msg_print(NumPlayers, "\374\377y      Before you move out, press \377Rw\377y to equip your starting items!");
		msg_print(NumPlayers, "\374\377y ");
//		msg_print(NumPlayers, "\377RTurn off numlock and hit SHIFT + numkeys to run (move quickly).");
//		msg_print(NumPlayers, "\377RHit '?' key for help. Hit ':' to chat. Hit '@' to see who is online.");
//		msg_print(NumPlayers, "\377R<< Welcome to TomeNET! >>");

		/* Play audio greeting too! - C. Blue */
		if (greeting) { /* only the very 1st time, might become annoying */
			s_printf("GREETING_AUDIO: %s\n", p_ptr->name);
			sound(NumPlayers, "greeting", NULL, SFX_TYPE_MISC, FALSE);
		}
	}

	/* warning_rest only occurs once per account */
	if (acc_get_flags(p_ptr->accountname) & ACC_WARN_REST) p_ptr->warning_rest = 1;
#endif

#if 1
	/* Give a more visible message about outdated client usage - C. Blue */
	if (!is_newer_than(&p_ptr->version, VERSION_MAJOR_OUTDATED, VERSION_MINOR_OUTDATED, VERSION_PATCH_OUTDATED, VERSION_EXTRA_OUTDATED, VERSION_BRANCH_OUTDATED, VERSION_BUILD_OUTDATED)) {
		msg_print(NumPlayers, "\374\377y --- Your client is outdated! Get newest one from www.tomenet.net ---");
	} else if (is_older_than(&p_ptr->version, VERSION_MAJOR_LATEST, VERSION_MINOR_LATEST, VERSION_PATCH_LATEST, VERSION_EXTRA_LATEST, VERSION_BRANCH_LATEST, VERSION_BUILD_LATEST)) {
		msg_print(NumPlayers, "\374\377D --- Your client is NOT the latest version, it's not 'outdated' though. ---");
	}
#endif

	/* Admin messages */
	if (p_ptr->admin_dm && (cfg.runlevel == 2048))
		msg_print(NumPlayers, "\377y* Empty-server-shutdown command pending *");
	if (p_ptr->admin_dm && (cfg.runlevel == 2047))
		msg_print(NumPlayers, "\377y* Low-server-shutdown command pending *");
	if (p_ptr->admin_dm && (cfg.runlevel == 2046))
		msg_print(NumPlayers, "\377y* VeryLow-server-shutdown command pending *");
	if (p_ptr->admin_dm && (cfg.runlevel == 2045))
		msg_print(NumPlayers, "\377y* None-server-shutdown command pending *");
	if (p_ptr->admin_dm && (cfg.runlevel == 2044))
		msg_print(NumPlayers, "\377y* ActiveVeryLow-server-shutdown command pending *");
	if (p_ptr->admin_dm && (cfg.runlevel == 2043))
		msg_print(NumPlayers, "\377y* Recall-server-shutdown command pending *");

	if (cfg.runlevel == 2043) {
		if (shutdown_recall_timer >= 120)
			msg_format(NumPlayers, "\374\377I*** \377RServer-shutdown in max %d minutes (auto-recall). \377I***", shutdown_recall_timer / 60);
		else
			msg_format(NumPlayers, "\374\377I*** \377RServer-shutdown in max %d seconds (auto-recall). \377I***", shutdown_recall_timer);
	}

	if (p_ptr->quest_id) {
		for (i = 0; i < 20; i++) {
			if (quests[i].id == p_ptr->quest_id) {
				msg_format(NumPlayers, "\377oYour quest to kill \377y%d \377g%s \377ois not complete.", p_ptr->quest_num, r_name+r_info[quests[i].type].name);
				break;
			}
		}
	}

	if(!(p_ptr->mode & MODE_NO_GHOST) &&
	    !(p_ptr->mode & MODE_EVERLASTING) &&
	    !(p_ptr->mode & MODE_PVP) &&
	    !cfg.no_ghost && cfg.lifes)
	{
#if 0
		/* if total_winner char was loaded from old save game that
		   didn't reduce his/her lifes to 1 on winning, do that now: */
		if (p_ptr->total_winner && (p_ptr->lives > 1+1)) {
			msg_print(NumPlayers, "\377yTake care! As a winner, you have no more resurrections left!");
			p_ptr->lives = 1+1;
		}
#endif
		if (p_ptr->lives-1 == 1)
    			msg_print(NumPlayers, "\377GYou have no more resurrections left!");
	        else
			msg_format(NumPlayers, "\377GYou have %d resurrections left.", p_ptr->lives-1-1);
	}
	
	/* for PvP mode chars */
	if (p_ptr->free_mimic) msg_format(NumPlayers, "\377GYou have %d free mimicry transformation left.", p_ptr->free_mimic);

	/* receive previously buffered global-event-contender-deed from other character of her/his account */
	for (i = 0; i < 128; i++) {
		if (ge_contender_buffer_ID[i] == p_ptr->account) {
			ge_contender_buffer_ID[i] = 0;
			i = lookup_kind(TV_PARCHMENT, ge_contender_buffer_deed[i]);
			invcopy(o_ptr, i);
			o_ptr->number = 1;
			object_aware(NumPlayers, o_ptr);
			object_known(o_ptr);
			o_ptr->discount = 0;
			o_ptr->level = 0;
			o_ptr->ident |= ID_MENTAL;
			inven_carry(NumPlayers, o_ptr);
			msg_print(NumPlayers, "\377GAs a former contender in an event, you have received a deed!");
		}
	}
	/* receive previously buffered achievement deed (pvp) from other character of her/his account */
	for (i = 0; i < 128; i++) {
		if (achievement_buffer_ID[i] == p_ptr->account) {
			switch (achievement_buffer_deed[i]) {
			case SV_DEED_PVP_MAX: /* this one is transferrable to non-pvp char */
				break;
			case SV_DEED_PVP_MID:
			case SV_DEED_PVP_MASS:
				if (!(p_ptr->mode & MODE_PVP)) continue;
				break;
			}
			achievement_buffer_ID[i] = 0;
			i = lookup_kind(TV_PARCHMENT, achievement_buffer_deed[i]);
			invcopy(o_ptr, i);
			o_ptr->number = 1;
			object_aware(NumPlayers, o_ptr);
			object_known(o_ptr);
			o_ptr->discount = 0;
			o_ptr->level = 0;
			o_ptr->ident |= ID_MENTAL;
			inven_carry(NumPlayers, o_ptr);
			msg_print(NumPlayers, "\377GFor your achievements, you have received a deed!");
		}
	}

	/* Sold something in his player store while he wasn't logged on? */
	if (acc_get_flags(p_ptr->accountname) & ACC_WARN_SALE) {
		acc_set_flags(p_ptr->accountname, ACC_WARN_SALE, FALSE);
		msg_print(NumPlayers, "\374\377yA store of yours has sold something meanwhile!");
	}

	/* automatically re-add him to the guild of his last character? */
	namebuf1[0] = '\0'; //abuse namebuf1
	if ((i = acc_get_guild(p_ptr->accountname))) {
		if (p_ptr->newly_created) {
			/* within time limit [20 minutes]? */
			time_t now = time(&now);
			if (now - lookup_player_laston(p_ptr->id) <= 60 * 20
			    && guilds[i].members) { /* guild still exists? (TODO: could be a different guild by now :-p) */
				/* auto-re-add him to the guild */
				if (guild_auto_add(NumPlayers, i, namebuf1)) {
					/* also restore his 'adder' status if he was one */
					if ((acc_get_flags(p_ptr->accountname) & ACC_GUILD_ADDER)) {
#ifdef GUILD_ADDERS_LIST
						/* check for vacant adder slot */
						for (i = 0; i < 5; i++)
							if (guilds[p_ptr->guild].adder[i][0] == '\0') break;
						/* success? */
						if (i != 5) {
							strcpy(guilds[p_ptr->guild].adder[i], p_ptr->name);
							p_ptr->guild_flags |= PGF_ADDER;
						}
					}
#else
						p_ptr->guild_flags |= PGF_ADDER;
					}
#endif
				}
			}
		}
		acc_set_guild(p_ptr->accountname, 0);
		acc_set_flags(p_ptr->accountname, ACC_GUILD_ADDER, FALSE);
	}

#ifdef GUILD_ADDERS_LIST
	/* Erase his PGF_ADDER flag if he's been removed from the adder list. */
	if ((p_ptr->guild_flags & PGF_ADDER)) {
		for (i = 0; i < 5; i++)
			if (streq(guilds[p_ptr->guild].adder[i], p_ptr->name)) break;
		if (i == 5) {
			p_ptr->guild_flags &= ~PGF_ADDER;
			msg_format(NumPlayers, "\374\377%cYour authorization to add others to the guild has been \377rretracted\377%c.", COLOUR_CHAT_GUILD, COLOUR_CHAT_GUILD);
		}
	}
#endif

	/* some one-time hints after char creation in player_birth() */
	if (p_ptr->newly_created) {
		p_ptr->newly_created = FALSE;

		if (p_ptr->mode & MODE_PVP) {
			msg_print(NumPlayers, "\377yType \"/pvp\" into chat to enter the pvp arena, and again to leave it.");

			if (p_ptr->inval) {
				msg_print(NumPlayers, "\374\377RNOTE: 'PvP mode' is a special type of gameplay. NOT recommended for beginners!");
				msg_print(NumPlayers, "\374\377R      If you didn't choose PvP mode on purpose, press shift+q to start over.");
			}
		}
	}

	/* Check Morgoth, if player had saved a level where he was generated */
	check_Morgoth(NumPlayers);

#ifdef ENABLE_RCRAFT
	wpcopy(&Players[NumPlayers]->wpos_old, &p_ptr->wpos);
#endif

#ifdef CLIENT_SIDE_WEATHER
	/* update his client-side weather */
	player_weather(NumPlayers, TRUE, TRUE, TRUE);
#endif

#ifdef AUCTION_SYSTEM
	auction_player_joined(NumPlayers);
#endif

#ifdef USE_SOUND_2010
	/* Initialize his background music */
	p_ptr->music_current = -1; //hack-init: since 0 is used too..
	p_ptr->music_monster = -1; //hack-init: since 0 is used too.. (boss-specific music)

	/* Keep music quiet for a moment to allow player to hear the introduction speech? */
	if (greeting && p_ptr->audio_mus > 0 && p_ptr->audio_sfx >= 2) /* speech is event #2 in unmodified sounds.cfg */
		p_ptr->music_start = 20; /* wait for this # of turns until starting the music */
	else
		handle_music(NumPlayers); /* start music normally (instantly) */
#endif

	/* Initialize the client's unique list;
	it will become further updated each time he kills another unique */
	for (i = 0; i < MAX_R_IDX; i++)
		if (r_info[i].flags1 & RF1_UNIQUE)
			Send_unique_monster(NumPlayers, i);

#if 0 /* not here, but below instead. Or admins will be shown in the list! */
 #ifdef TOMENET_WORLDS
	world_player(p_ptr->id, p_ptr->name, TRUE, TRUE); /* last flag is 'quiet' mode -> no public msg */
 #endif
#endif


	/* Prepare title for possibly telling others about our new player (or admin) */
	title = "";
	if (p_ptr->admin_dm) title = (p_ptr->male) ? "Dungeon Master " : "Dungeon Mistress ";
	else if (p_ptr->admin_wiz) title = "Dungeon Wizard ";
	else if (p_ptr->total_winner) {
		if (p_ptr->mode & (MODE_HARD | MODE_NO_GHOST)) {
			title = (p_ptr->male) ? "Emperor " : "Empress ";
		} else {
			title = (p_ptr->male) ? "King " : "Queen ";
		}
	}

	/* Handle the cfg_secret_dungeon_master option: Only tell other admins. */
	if (p_ptr->admin_dm && (cfg.secret_dungeon_master)) {
		/* Tell other secret dungeon masters about our new player */
		for (i = 1; i < NumPlayers; i++) {
			if (Players[i]->conn == NOT_CONNECTED) continue;
			if (!is_admin(Players[i])) continue;

			msg_format(i, "\374\377%c%s%s has entered the game.", COLOUR_SERVER, title, p_ptr->name);
			if (namebuf1[0] && Players[i]->guild == p_ptr->guild) msg_print(i, namebuf1);
		}
		return 0;
	}

#ifdef TOMENET_WORLDS
	world_player(p_ptr->id, p_ptr->name, TRUE, TRUE); /* last flag is 'quiet' mode -> no public msg */
#endif

	/* Tell everyone about our new player */
	for (i = 1; i < NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED) continue;
		msg_format(i, "\374\377%c%s%s has entered the game.", COLOUR_SERVER, title, p_ptr->name);
	}

#ifdef TOMENET_WORLDS
	if (cfg.worldd_pjoin) world_msg(format("\374\377%c%s%s has entered the game.", COLOUR_SERVER, title, p_ptr->name));
#endif

	/* print notification message about guild-auto-add now */
	if (namebuf1[0]) guild_msg(p_ptr->guild, namebuf1);

	/* Tell the meta server about the new player */
	Report_to_meta(META_UPDATE);

	return 0;
}

/* wrapper function for local 'Conn' - C. Blue */
int is_inactive(int Ind) {
	if (Conn[Players[Ind]->conn]->inactive_keepalive)
		return (Conn[Players[Ind]->conn]->inactive_keepalive);
	else
		return (Conn[Players[Ind]->conn]->inactive_ping / 2);
}

/* Actually execute commands from the client command queue */
void process_pending_commands(int ind)
{
	connection_t *connp = Conn[ind];
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
		char *foo = connp->r.ptr;
		type = (connp->r.ptr[0] & 0xFF);
		if (type != PKT_KEEPALIVE && type != PKT_PING)
		{
			connp->inactive_keepalive = 0;
			connp->inactive_ping = 0;
			if (connp->id != -1) p_ptr->idle = 0;
		}
		result = (*receive_tbl[type])(ind);

		/* Check that the player wasn't disconnected - mikaelh */
		if (!Conn[ind]) {
			return;
		}

		/* See 'p_ptr->requires_energy' below in 'result == 0' clause. */
		if (p_ptr != NULL && p_ptr->conn != NOT_CONNECTED)
			p_ptr->requires_energy = (result == 0);

		/* Check whether the socket buffer has advanced */
		if (connp->r.ptr == foo) {
			/* Return code 0 means that there wasn't enough data in the socket buffer */
			if (result == 0) {
				/* Move the remaining data to the queue buffer - mikaelh */
				int len = connp->r.len - (connp->r.ptr - connp->r.buf);
				if (Sockbuf_write(&connp->q, connp->r.ptr, len) != len)
				{
					errno = 0;
					Destroy_connection(ind, "Can't copy data to queue");
					return;
				}
			}

			/* Clear the buffer to avoid getting stuck in a loop */
			Sockbuf_clear(&connp->r);
			break;
		}
		if (connp->state == CONN_PLAYING)
		{
			connp->start = turn;
		}
		if (result == -1) {
			return;
		}

		// We didn't have enough energy to execute an important command.
		if (result == 0) {
			/* New: Since fire-till-kill is now allowed to begin at <= 1
			   energy (see dungeon.c, process_player_end()), we need this
			   to avoid getting 'locked up' in shooting_till_kill. - C. Blue */
			//done above already	p_ptr->requires_energy = TRUE;

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
			if (p_ptr->energy) {
				old_energy = p_ptr->energy;
				p_ptr->energy = 0;
			}
		}

		/* Queue all remaining packets now */
		if (result == 3)
		{
			int len = connp->r.len - (connp->r.ptr - connp->r.buf);
			if (Sockbuf_write(&connp->q, connp->r.ptr, len) != len)
			{
				errno = 0;
				Destroy_connection(ind, "Can't copy data to queue");
				return;
			}
			Sockbuf_clear(&connp->r);

			break;
		}
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
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	//int type, result, (**receive_tbl)(int ind);
	int (**receive_tbl)(int ind);

	/* Check that the pointer is valid - mikaelh */
	if (!connp) return;

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

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];
	}

	/* Mega-Hack */
	if (p_ptr && p_ptr->new_level_flag) return;

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

	// Execute any new commands immediately if possible
	// Don't process commands when marked for death - mikaelh
	if (!p_ptr || !p_ptr->death) process_pending_commands(ind);

	/* Check that the player wasn't disconnected - mikaelh */
	if (!Conn[ind]) {
		return;
	}

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
		if (p_ptr)
		{
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
		if (Packet_printf(&connp->c, "%c", PKT_END) <= 0)
		{
			Destroy_connection(ind, "write error");
			return;
		}
		Send_reliable(ind);
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
		connp = Conn[i];

		if (!connp || connp->state == CONN_FREE)
			continue;
		if (connp->timeout && (connp->start + connp->timeout * cfg.fps < turn))
		{
			if (connp->state & (CONN_PLAYING | CONN_READY))
			{
/*				sprintf(msg, "%s mysteriously disappeared!",
					connp->nick);
				Set_message(msg); */
			}
			sprintf(msg, "timeout %02x", connp->state);
			Destroy_connection(i, msg);

#if 0
			/* Very VERY bad hack :/ - C. Blue */
	                save_game_panic();
#endif

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

	/* Do GW timeout checks */
	SGWTimeout();

	for (i = 0; i < num_reliable; i++)
	{
		ind = input_reliable[i];
		connp = Conn[ind];
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

		connp = Conn[p_ptr->conn];

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

#ifndef EVIL_METACLIENT
	/* Every fifteen seconds, update the info sent to the metaserver */
	if (!(turn % (15 * cfg.fps)))
		Report_to_meta(META_UPDATE);
#endif

	return 1;
}

int Net_output1(int i)
{
	connection_t *connp;
	player_type *p_ptr = NULL;

	p_ptr = Players[i];
	if (p_ptr->conn == NOT_CONNECTED) return 0;
	if (p_ptr->new_level_flag) return 2;
	connp = Conn[p_ptr->conn];

	if (connp->c.len > 0)
	{
		if (Packet_printf(&connp->c, "%c", PKT_END) <= 0) {
			Destroy_connection(p_ptr->conn, "write error");
			return 3;
		} else {
			Send_reliable(p_ptr->conn);
		}
	}
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
	connection_t *connp = Conn[ind];
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
	connection_t *connp = Conn[ind];

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
	connection_t * connp = Conn[ind];

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
	if(connp->id==-1 || istownarea(&p_ptr->wpos, 2))
	{
		Destroy_connection(ind, "client quit");
	}
	// Otherwise wait for the timeout
	else exec_lua(NumPlayers, format("player_leaves_timeout(%d, %d, \"%/s\", \"%s\")", NumPlayers, p_ptr->id, p_ptr->name, showtime()));
}


static int Receive_quit(int ind)
{
	int player = -1, n;
	connection_t *connp = Conn[ind];
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
	connection_t *connp = Conn[ind], *connp2 = NULL;
	//unsigned char ch;
	int i, n;
	char choice[MAX_CHARS];
	struct account *l_acc;

	n = Sockbuf_read(&connp->r);
	if(n==0 && !(errno==EAGAIN || errno==EWOULDBLOCK)){
		/* avoid SIGPIPE in zero read - it was closed */
		close(connp->w.sock);
		remove_input(connp->w.sock);
		connp->w.sock = -1;
		Destroy_connection(ind, "disconnect in login");
		return(-1);
	}

	if ((n = Packet_scanf(&connp->r, "%s", choice)) != 1)
	{
		errno = 0;
		printf("%d\n",n);
		plog("Failed reading login packet");
		Destroy_connection(ind, "receive error in login");
		return -1;
	}

	if(strlen(choice)==0){
		if (Check_names(connp->nick, connp->real, connp->host, connp->addr, FALSE) != SUCCESS)
		{
			Destroy_connection(ind, "The server didn't like your nickname, realname or hostname");
			return(-1);
		}

		/* Password obfuscation introduced in pre-4.4.1a client or 4.4.1.1 */
		if (connp->pass && is_newer_than(&connp->version, 4, 4, 1, 0, 0, 0))
		{
			/* Use memfrob for the password - mikaelh */
			my_memfrob(connp->pass, strlen(connp->pass));
		}

		if (connp->pass && (l_acc = GetAccount(connp->nick, connp->pass, FALSE))) {
			int *id_list, i;
			byte tmpm;
			char colour_sequence[3];
			s32b sflg3 = 0, sflg2 = 0, sflg1 = 0, sflg0 = 0; /* server flags to tell the client more about us - just informational purpose */
			/* flag array 0: server type flags
			   flag array 1: client mode (special screen layout for showing party stats maybe (TODO//unused))
			   flag array 2: temporary testing flags for experimental features
			   flag array 3: unused
			*/

			/* Set server type flags */
#ifdef RPG_SERVER
			sflg0 |= SFLG_RPG;
			if (l_acc->flags & ACC_ADMIN) sflg0 |= SFLG_RPG_ADMIN; /* Allow multiple chars per account for admins! */
#endif
#ifdef FUN_SERVER
			sflg0 |= SFLG_FUN;
#endif
#ifdef PARTY_SERVER
			sflg0 |= SFLG_PARTY;
#endif
#ifdef ARCADE_SERVER
			sflg0 |= SFLG_ARCADE;
#endif
#ifdef TEST_SERVER
			sflg0 |= SFLG_TEST;
#endif

			/* Set client mode flags */

			/* Set temporary flags */
			sflg2 = sflags_TEMP;

			/* Set XXX flags */

			/* Send all flags! */
			Packet_printf(&connp->c, "%c%d%d%d%d", PKT_SERVERDETAILS, sflg3, sflg2, sflg1, sflg0);

			connp->password_verified = TRUE;
			free(connp->pass);
			connp->pass = NULL;
			n = player_id_list(&id_list, l_acc->id);
			/* Display all account characters here */
			for(i = 0; i < n; i++) {
				u16b ptype = lookup_player_type(id_list[i]);
				/* do not change protocol here */
				tmpm = lookup_player_mode(id_list[i]);
				if (tmpm & MODE_EVERLASTING) strcpy(colour_sequence, "\377B");
				else if (tmpm & MODE_PVP) strcpy(colour_sequence, format("\377%c", COLOUR_MODE_PVP));
				else if (tmpm & MODE_NO_GHOST) strcpy(colour_sequence, "\377D");
				else if (tmpm & MODE_HARD) strcpy(colour_sequence, "\377s");
				else strcpy(colour_sequence, "\377W");
				Packet_printf(&connp->c, "%c%s%s%hd%hd%hd", PKT_LOGIN, colour_sequence, lookup_player_name(id_list[i]), lookup_player_level(id_list[i]), ptype&0xff , ptype>>8);
			}
			Packet_printf(&connp->c, "%c%s%s%hd%hd%hd", PKT_LOGIN, "", "", 0, 0, 0);
			if (n) C_KILL(id_list, n, int);
			KILL(l_acc, struct account);
		}
		else{
			/* fail login here */
			Destroy_connection(ind, "Wrong password or name already in use.");
		}
		Sockbuf_flush(&connp->w);
		return(0);
	} else if (connp->password_verified) {
		int check_account_reason = 0;

		/* just in case - some places can't handle a longer name and a valid client shouldn't supply a name this long anyway - mikaelh */
		choice[NAME_LEN - 1] = '\0';

		/* Prevent EXPLOIT (adding a SPACE to foreign charname) */
		s_printf("Player %s chooses character '%s' (strlen=%d)\n", connp->nick, choice, strlen(choice));
		Trim_name(choice);

		if (forbidden_name(choice)) {
//			Packet_printf(&connp->c, "%c", E_INVAL);
                        Destroy_connection(ind, "Forbidden character name. Please choose a different name.");
                        return(-1);
		}

		/* at this point, we are authorised as the owner
		   of the account. any valid name will be
		   allowed. */
		/* i realise it should return different value depending
		   on reason - evileye */
		check_account_reason = check_account(connp->nick, choice);
		switch (check_account_reason) {
		case 1: // OK
			/* Check that no one else is creating a char with the same name - mikaelh */
			for (i = 0; i < max_connections; i++) {
				connp2 = Conn[i];
				if (!connp2 || connp2->state == CONN_FREE) continue;
				if (connp2->c_name && !strcasecmp(connp2->c_name, choice) &&
				    strcasecmp(connp2->nick, connp->nick)) { /* check that it's a different account, too */
					/* Fail login */
					Destroy_connection(ind, "Character name already in use.");
					s_printf("(Prevented simultaneous creation of same character.)\n");
					return(-1);
				}
			}

			/* Validate names/resume in proper place */
			if(Check_names(choice, connp->real, connp->host, connp->addr, TRUE)){
/* connp->real is _always_ 'PLAYER' - connp->nick is the account name, choice the c_name */
				/* fail login here */
				Destroy_connection(ind, "Security violation");
				return(-1);
			}
			Packet_printf(&connp->c, "%c", lookup_player_id(choice) ? SUCCESS : E_NEED_INFO);
			connp->c_name=strdup(choice);
			break;
		case 0: //NOT OK
			/* fail login here */
			Destroy_connection(ind, "Name already in use by another player.");
			return(-1);
		case -1: //NOT OK: Max 1 char (RPG)
			/* fail login here */
			Destroy_connection(ind, "Only one character per account is allowed.");
			return(-1);
		case -2: //NOT OK
			/* fail login here */
			Destroy_connection(ind, "Multiple logins on the same account aren't allowed.");
			return(-1);
		}
	} else {
		/* fail login due to missing password */
		s_printf("EXPLOIT: Missing password of player %s.\n", connp->nick);
		Destroy_connection(ind, "Missing password");
		return(-1);
	}
	if (connp->setup >= Setup.setup_size)
		Conn_set_state(connp, CONN_LOGIN, CONN_LOGIN);
	return(0);
}

#define RECEIVE_PLAY_SIZE (2*6+OPT_MAX+2*(TV_MAX+MAX_F_IDX+MAX_K_IDX+MAX_R_IDX))
//#define STRICT_RECEIVE_PLAY
static int Receive_play(int ind)
{
	connection_t *connp = Conn[ind];
	unsigned char ch;
	int i, n, sfx = -1, mus = -1;
	s16b sex, race, class, trait = 0;

	/* XXX */
	n = Sockbuf_read(&connp->r);
	if(n==0 && !(errno==EAGAIN || errno==EWOULDBLOCK)){

		/* avoid SIGPIPE in zero read */
		close(connp->w.sock);
		remove_input(connp->w.sock);
		connp->w.sock = -1;
		Destroy_connection(ind, "disconnect in play");
		return(-1);
	}

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
		if (is_newer_than(&connp->version, 4, 4, 5, 10, 0, 0)) {
			if ((n = Packet_scanf(&connp->r, "%hd%hd%hd%hd%hd%hd", &sex, &race, &class, &trait, &sfx, &mus)) <= 0) {
				errno = 0;
				plog("Play packet is broken");
				Destroy_connection(ind, "receive error 2 in play");
				return -1;
			}
		} else {
			if ((n = Packet_scanf(&connp->r, "%hd%hd%hd", &sex, &race, &class)) <= 0) {
				errno = 0;
				plog("Play packet is broken");
				Destroy_connection(ind, "receive error 2 in play");
				return -1;
			}
		}

		/* Set his character info */
		connp->sex = sex;
		connp->race = race;
		connp->class = class;
		connp->trait = trait;

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

	s_printf("AUDIO: %s features %hd, %hd.\n", connp->nick, sfx, mus);
	connp->audio_sfx = (short int)sfx;
	connp->audio_mus = (short int)mus;

	Sockbuf_clear(&connp->w);
	if (Handle_login(ind) == -1) {
		/* The connection has already been destroyed */
		return -1;
	}

	return 2;
}

/* Head of file transfer system receive */
/* DO NOT TOUCH - work in progress */
static int Receive_file(int ind){
	char command, ch;
	char fname[MAX_CHARS];	/* possible filename */
	int x;	/* return value/ack */
	unsigned short fnum;	/* unique SENDER side file number */
	unsigned short len;
	u32b csum;
	int n;
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	int Ind;

	Ind = GetInd[connp->id];
	p_ptr = Players[Ind];
	n = Packet_scanf(&connp->r, "%c%c%hd", &ch, &command, &fnum);
	if(n == 3){
		switch(command){
			case PKT_FILE_INIT:
				/* Admin to do this only !!! */
				Packet_scanf(&connp->r, "%s", fname);
				if(!is_admin(p_ptr)){
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
				if(!is_admin(p_ptr)){
					msg_print(Ind, "\377rFile check refused");
					x=0; 
				}
				else{
					msg_print(Ind, "\377yChecking file");
					x=local_file_check(fname, &csum);
					Packet_printf(&connp->w, "%c%c%hd%d", PKT_FILE, PKT_FILE_SUM, fnum, csum);
					return(1);
				}
				break;
			case PKT_FILE_SUM:
				Packet_scanf(&connp->r, "%d", &csum);
				check_return(ind, fnum, csum, Ind);

				/* for 4.4.8.1.0.0 LUA update crash bug */
				if (p_ptr->warning_lua_count == 0 && p_ptr->warning_lua_update == 1
				    /* don't give him messages if he can't help it */
				    && !p_ptr->v_latest) {
					msg_print(Ind, "\377RWarning: Due to a bug in client 4.4.8 it cannot update LUA files.");
					msg_print(Ind, "\377R         If you play spell-casting characters please update your client!");
				}

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
	connection_t *connp = Conn[ind];
	memcpy(buffer, connp->r.ptr, len);
	connp->r.ptr+=len;
	return(1);
}

int Send_file_check(int ind, unsigned short id, char *fname){
	connection_t *connp = Conn[ind];
	Packet_printf(&connp->c, "%c%c%hd%s", PKT_FILE, PKT_FILE_CHECK, id, fname);
	return(1);
}

int Send_file_init(int ind, unsigned short id, char *fname){
	connection_t *connp = Conn[ind];
	Packet_printf(&connp->c, "%c%c%hd%s", PKT_FILE, PKT_FILE_INIT, id, fname);
	return(1);
}

int Send_file_data(int ind, unsigned short id, char *buf, unsigned short len){
	connection_t *connp = Conn[ind];
	Packet_printf(&connp->c, "%c%c%hd%hd", PKT_FILE, PKT_FILE_DATA, id, len);
	if (Sockbuf_write(&connp->c, buf, len) != len){
		printf("failed sending file data\n");
	}
	return(1);
}

int Send_file_end(int ind, unsigned short id){
	connection_t *connp = Conn[ind];
	Packet_printf(&connp->c, "%c%c%hd", PKT_FILE, PKT_FILE_END, id);
	return(1);
}

int Send_reliable(int ind)
{
	connection_t *connp = Conn[ind];
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

#if 0
		/* Very bad hack :/ - C. Blue */
		save_game_panic();
#endif

		return -1;
	}
	Sockbuf_clear(&connp->c);
	return num_written;
}

#if 0 /* old UDP networking stuff - mikaelh */
int Send_reliable_old(int ind)
{
	connection_t *connp = Conn[ind];
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
#endif

#if 0 /* old UDP networking stuff - mikaelh */
static int Receive_ack(int ind)
{
	connection_t *connp = Conn[ind];
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
#endif
 
static int Receive_discard(int ind)
{
	connection_t *connp = Conn[ind];

	errno = 0;
	plog(format("Discarding packet %d while in state %02x",
		connp->r.ptr[0], connp->state));
	connp->r.ptr = connp->r.buf + connp->r.len;

	return 0;
}

static int Receive_undefined(int ind)
{
	connection_t *connp = Conn[ind];

	errno = 0;
	plog(format("Unknown packet type (%d,%02x)", connp->r.ptr[0], connp->state));

	/* Dont destroy connection. Ignore the invalid packet */
	/*Destroy_connection(ind, "undefined packet");*/

	/* Discard everything - mikaelh */
	connp->r.ptr = connp->r.buf + connp->r.len;

	return -1;	/* Crash if not (evil) */
	/*return 0;*/
}

int Send_plusses(int ind, int tohit, int todam, int hr, int dr, int hm, int dm)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for plusses (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%hd%hd%hd%hd%hd%hd", PKT_PLUSSES, tohit, todam, hr, dr, hm, dm);
	}

        return Packet_printf(&connp->c, "%c%hd%hd%hd%hd%hd%hd", PKT_PLUSSES, tohit, todam, hr, dr, hm, dm);
}


int Send_ac(int ind, int base, int plus)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for ac (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%hd%hd", PKT_AC, base, plus);
	}
	return Packet_printf(&connp->c, "%c%hd%hd", PKT_AC, base, plus);
}

int Send_experience(int ind, int lev, s32b max, s32b cur, s32b adv)
{
	connection_t *connp = Conn[Players[ind]->conn];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for experience (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (is_newer_than(&Players[ind]->version, 4, 4, 1, 3, 0, 0))
		return Packet_printf(&connp->c, "%c%hu%hu%hu%d%d%d", PKT_EXPERIENCE, lev, Players[ind]->max_lev, Players[ind]->max_plv, max, cur, adv);
	else
		return Packet_printf(&connp->c, "%c%hu%d%d%d", PKT_EXPERIENCE, lev, max, cur, adv);
}

#if 0
int Send_skill_init(int ind, int type, int i)
#else
int Send_skill_init(int ind, u16b i)
#endif
{
	connection_t *connp = Conn[Players[ind]->conn];

	char *tmp;
	int mkey;
	
	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for skill init (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	tmp = s_info[i].action_desc ? s_text + s_info[i].action_desc : "";
	mkey = s_info[i].action_mkey;
	
	/* hack for fighting/shooting techniques */
	if (mkey == MKEY_MELEE && Players[ind]->melee_techniques == 0x0000) mkey = 0;
	if (mkey == MKEY_RANGED && Players[ind]->ranged_techniques == 0x0000) mkey = 0;

	/* Note: %hd is 2 bytes - use this for x16b.
	   We can use %c for bytes. */
	return( Packet_printf(&connp->c, "%c%hd%hd%hd%hd%ld%c%S%S%S",
		PKT_SKILL_INIT, i,
		s_info[i].father, s_info[i].order, mkey,
		s_info[i].flags1, s_info[i].tval, s_name+s_info[i].name,
		s_text+s_info[i].desc, tmp ? tmp : "" ) );

}

int Send_skill_points(int ind){
	connection_t *connp = Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for skill mod (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
        return Packet_printf(&connp->c, "%c%d", PKT_SKILL_PTS, p_ptr->skill_points);
}

/* i is skill index, keep means if we want the client to keep his 'deflated?' state */
int Send_skill_info(int ind, int i, bool keep)
{
	connection_t *connp = Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];
	int mkey;

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for skill mod (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	mkey = s_info[i].action_mkey;
	/* hack for fighting/shooting techniques */
	if (mkey == MKEY_MELEE && Players[ind]->melee_techniques == 0x0000) mkey = 0;
	if (mkey == MKEY_RANGED && Players[ind]->ranged_techniques == 0x0000) mkey = 0;

	if (!is_newer_than(&connp->version, 4, 4, 1, 2, 0, 0))
	        return Packet_printf(&connp->c, "%c%d%d%d%d%d", PKT_SKILL_MOD, i, p_ptr->s_info[i].value, p_ptr->s_info[i].mod, p_ptr->s_info[i].dev, p_ptr->s_info[i].flags1 & SKF1_HIDDEN);
	else if (!is_newer_than(&connp->version, 4, 4, 1, 7, 0, 0)) {
	        return Packet_printf(&connp->c, "%c%d%d%d%d%d%d", PKT_SKILL_MOD, i, p_ptr->s_info[i].value, p_ptr->s_info[i].mod, p_ptr->s_info[i].dev, p_ptr->s_info[i].flags1 & SKF1_HIDDEN, mkey);
	} else if (!is_newer_than(&connp->version, 4, 4, 4, 1, 0, 0)) {
	        return Packet_printf(&connp->c, "%c%d%d%d%d%d%d%d", PKT_SKILL_MOD, i, p_ptr->s_info[i].value, p_ptr->s_info[i].mod, p_ptr->s_info[i].dev, p_ptr->s_info[i].flags1 & SKF1_HIDDEN, mkey, p_ptr->s_info[i].flags1 & SKF1_DUMMY);
	} else if (!is_newer_than(&connp->version, 4, 4, 6, 2, 0, 0)) {
	        return Packet_printf(&connp->c, "%c%d%d%d%d%c%d", PKT_SKILL_MOD, i, p_ptr->s_info[i].value, p_ptr->s_info[i].mod, p_ptr->s_info[i].dev, p_ptr->s_info[i].flags1, mkey);
	} else {
	        return Packet_printf(&connp->c, "%c%d%d%d%d%c%d", PKT_SKILL_MOD, i, p_ptr->s_info[i].value, p_ptr->s_info[i].mod, keep ? -1 : (p_ptr->s_info[i].dev ? 1 : 0), p_ptr->s_info[i].flags1, mkey);
	}
}

int Send_gold(int ind, s32b au, s32b balance)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for gold (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%d%d", PKT_GOLD, au, balance);
	}
	return Packet_printf(&connp->c, "%c%d%d", PKT_GOLD, au, balance);
}

#if 0	// well, it's easily cracked by client
int Send_sanity(int ind, int msane, int csane)
{
#ifdef SHOW_SANITY
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/
//	printf("sanity send!\n");

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for hp (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%hd%hd", PKT_SANITY, msane, csane);
	}
	return Packet_printf(&connp->c, "%c%hd%hd", PKT_SANITY, msane, csane);
#endif	// SHOW_SANITY
}
#else	// 0
int Send_sanity(int ind, byte attr, cptr msg)
{
#ifdef SHOW_SANITY
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/
//	printf("sanity send!\n");

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for hp (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%c%s", PKT_SANITY, attr, msg);
	}
	return Packet_printf(&connp->c, "%c%c%s", PKT_SANITY, attr, msg);
#endif	// SHOW_SANITY
}
#endif	// 0

int Send_hp(int ind, int mhp, int chp)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for hp (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%hd%hd", PKT_HP, mhp, chp);
	}
	return Packet_printf(&connp->c, "%c%hd%hd", PKT_HP, mhp, chp);
}

int Send_sp(int ind, int msp, int csp)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr = Players[ind], *p_ptr2 = NULL;

#if 1 /* can we use mana at all? */
	if (is_newer_than(&p_ptr->version, 4, 4, 1, 3, 0, 0) &&
	    (p_ptr->pclass == CLASS_WARRIOR || p_ptr->pclass == CLASS_ARCHER)) {
		msp = -9999;
		csp = -9999;
	}
#endif

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for sp (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%hd%hd", PKT_SP, msp, csp);
	}
	return Packet_printf(&connp->c, "%c%hd%hd", PKT_SP, msp, csp);
}

int Send_stamina(int ind, int mst, int cst) {
	player_type *p_ptr = Players[ind], *p_ptr2 = NULL;
	connection_t *connp = Conn[Players[ind]->conn], *connp2;

#ifndef ENABLE_TECHNIQUES
	return(0); /* disabled until client can handle it */
#endif

	if (!is_newer_than(&connp->version, 4, 4, 1, 2, 0, 0)) return(0);

	/* can we use stamina at all? */
	if (is_newer_than(&p_ptr->version, 4, 4, 1, 3, 0, 0) &&
	    (p_ptr->pclass == CLASS_MAGE || p_ptr->pclass == CLASS_PRIEST ||
	    p_ptr->pclass == CLASS_SHAMAN)) {
		mst = -9999;
		cst = -9999;
	}

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
		errno = 0;
		plog(format("Connection not ready for hp (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%hd%hd", PKT_STAMINA, mst, cst);
	}
	return Packet_printf(&connp->c, "%c%hd%hd", PKT_STAMINA, mst, cst);
}

int Send_char_info(int ind, int race, int class, int trait, int sex, int mode)
{
	connection_t *connp = Conn[Players[ind]->conn];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for char info (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (is_newer_than(&connp->version, 4, 4, 5, 10, 0, 0)) {
		return Packet_printf(&connp->c, "%c%hd%hd%hd%hd%hd", PKT_CHAR_INFO, race, class, trait, sex, mode);
	} else {
		return Packet_printf(&connp->c, "%c%hd%hd%hd%hd", PKT_CHAR_INFO, race, class, sex, mode);
	}
}

int Send_various(int ind, int hgt, int wgt, int age, int sc, cptr body)
{
	connection_t *connp = Conn[Players[ind]->conn];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for various (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	return Packet_printf(&connp->c, "%c%hu%hu%hu%hu%s", PKT_VARIOUS, hgt, wgt, age, sc, body);
}

int Send_stat(int ind, int stat, int max, int cur, int s_ind, int max_base)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for stat (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%c%hd%hd%hd%hd", PKT_STAT, stat, max, cur, s_ind, max_base);
	}

	return Packet_printf(&connp->c, "%c%c%hd%hd%hd%hd", PKT_STAT, stat, max, cur, s_ind, max_base);
}

int Send_history(int ind, int line, cptr hist)
{
	connection_t *connp = Conn[Players[ind]->conn];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for history (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	return Packet_printf(&connp->c, "%c%hu%s", PKT_HISTORY, line, hist);
}

/* XXX 'pval' is sent only when the item is TV_BOOK (same with Send_equip)
 * otherwise you can use badly-cracked client :)	- Jir -
 */
int Send_inven(int ind, char pos, byte attr, int wgt, object_type *o_ptr, cptr name)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/
	char uses_dir = 0; /* flag whether a rod requires a direction for zapping or not */

	/* Mark rods that require a direction */
	if (o_ptr->tval == TV_ROD && rod_requires_direction(ind, o_ptr))
		uses_dir = 1;

	/* Mark activatable items that require a direction */
	if (activation_requires_direction(o_ptr)
//appearently not for A'able items >_>	    || !object_aware_p(ind, o_ptr))
	    ) {
		uses_dir = 1;
	}

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
		errno = 0;
		plog(format("Connection not ready for inven (%d.%d.%d)",
		    ind, connp->state, connp->id));
		return 0;
	}

	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		if (is_newer_than(&p_ptr2->version, 4, 4, 5, 10, 0, 0)) {
			Packet_printf(&connp2->c, "%c%c%c%hu%hd%c%c%hd%c%I", PKT_INVEN, pos, attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->tval == TV_BOOK ? o_ptr->pval : 0, uses_dir, name);
		} else if (is_newer_than(&p_ptr2->version, 4, 4, 4, 2, 0, 0)) {
			Packet_printf(&connp2->c, "%c%c%c%hu%hd%c%c%hd%I", PKT_INVEN, pos, attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->tval == TV_BOOK ? o_ptr->pval : 0, name);
		} else {
			Packet_printf(&connp2->c, "%c%c%c%hu%hd%c%c%hd%s", PKT_INVEN, pos, attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->tval == TV_BOOK ? o_ptr->pval : 0, name);
		}
	}

	if (is_newer_than(&Players[ind]->version, 4, 4, 5, 10, 0, 0)) {
		return Packet_printf(&connp->c, "%c%c%c%hu%hd%c%c%hd%c%I", PKT_INVEN, pos, attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->tval == TV_BOOK ? o_ptr->pval : 0, uses_dir, name);
	} else if (is_newer_than(&Players[ind]->version, 4, 4, 4, 2, 0, 0)) {
		return Packet_printf(&connp->c, "%c%c%c%hu%hd%c%c%hd%I", PKT_INVEN, pos, attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->tval == TV_BOOK ? o_ptr->pval : 0, name);
	} else {
		return Packet_printf(&connp->c, "%c%c%c%hu%hd%c%c%hd%s", PKT_INVEN, pos, attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->tval == TV_BOOK ? o_ptr->pval : 0, name);
	}
}

int Send_inven_wide(int ind, char pos, byte attr, int wgt, object_type *o_ptr, cptr name)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
		errno = 0;
		plog(format("Connection not ready for inven (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		if (is_newer_than(&p_ptr2->version, 4, 4, 4, 2, 0, 0))
			Packet_printf(&connp2->c, "%c%c%c%hu%hd%c%c%hd%c%c%c%c%c%c%c%c%c%I", PKT_INVEN_WIDE, pos, attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->tval == TV_BOOK ? o_ptr->pval : 0,
			    o_ptr->xtra1, o_ptr->xtra2, o_ptr->xtra3, o_ptr->xtra4, o_ptr->xtra5, o_ptr->xtra6, o_ptr->xtra7, o_ptr->xtra8, o_ptr->xtra9, name);
		else {
			Packet_printf(&connp2->c, "%c%c%c%hu%hd%c%c%hd%c%c%c%c%c%c%c%c%c%s", PKT_INVEN_WIDE, pos, attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->tval == TV_BOOK ? o_ptr->pval : 0,
			    o_ptr->xtra1, o_ptr->xtra2, o_ptr->xtra3, o_ptr->xtra4, o_ptr->xtra5, o_ptr->xtra6, o_ptr->xtra7, o_ptr->xtra8, o_ptr->xtra9, name);
		}
	}

	if (is_newer_than(&Players[ind]->version, 4, 4, 4, 2, 0, 0)) {
		return Packet_printf(&connp->c, "%c%c%c%hu%hd%c%c%hd%c%c%c%c%c%c%c%c%c%I", PKT_INVEN_WIDE, pos, attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->tval == TV_BOOK ? o_ptr->pval : 0,
		    o_ptr->xtra1, o_ptr->xtra2, o_ptr->xtra3, o_ptr->xtra4, o_ptr->xtra5, o_ptr->xtra6, o_ptr->xtra7, o_ptr->xtra8, o_ptr->xtra9, name);
	} else {
		return Packet_printf(&connp->c, "%c%c%c%hu%hd%c%c%hd%c%c%c%c%c%c%c%c%c%s", PKT_INVEN_WIDE, pos, attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->tval == TV_BOOK ? o_ptr->pval : 0,
		    o_ptr->xtra1, o_ptr->xtra2, o_ptr->xtra3, o_ptr->xtra4, o_ptr->xtra5, o_ptr->xtra6, o_ptr->xtra7, o_ptr->xtra8, o_ptr->xtra9, name);
	}
}

//int Send_equip(int ind, char pos, byte attr, int wgt, byte tval, cptr name)
int Send_equip(int ind, char pos, byte attr, int wgt, object_type *o_ptr, cptr name)
{
	char uses_dir = 0;
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/

	/* Mark activatable items that require a direction */
	if (activation_requires_direction(o_ptr)
//appearently not for A'able items >_>	    || !object_aware_p(ind, o_ptr))
	    ) {
		uses_dir = 1;
	}

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
		errno = 0;
		plog(format("Connection not ready for equip (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		if (is_newer_than(&p_ptr2->version, 4, 4, 5, 10, 0, 0))
			Packet_printf(&connp2->c, "%c%c%c%hu%hd%c%c%hd%c%I", PKT_EQUIP, pos, attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->tval == TV_BOOK ? o_ptr->pval : 0, uses_dir, name);
		else if (is_newer_than(&p_ptr2->version, 4, 4, 4, 2, 0, 0))
			Packet_printf(&connp2->c, "%c%c%c%hu%hd%c%c%hd%I", PKT_EQUIP, pos, attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->tval == TV_BOOK ? o_ptr->pval : 0, name);
		else
			Packet_printf(&connp2->c, "%c%c%c%hu%hd%c%c%hd%s", PKT_EQUIP, pos, attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->tval == TV_BOOK ? o_ptr->pval : 0, name);
	}
	if (is_newer_than(&Players[ind]->version, 4, 4, 5, 10, 0, 0))
		return Packet_printf(&connp->c, "%c%c%c%hu%hd%c%c%hd%c%I", PKT_EQUIP, pos, attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->tval == TV_BOOK ? o_ptr->pval : 0, uses_dir, name);
	else if (is_newer_than(&Players[ind]->version, 4, 4, 4, 2, 0, 0))
		return Packet_printf(&connp->c, "%c%c%c%hu%hd%c%c%hd%I", PKT_EQUIP, pos, attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->tval == TV_BOOK ? o_ptr->pval : 0, name);
	else
		return Packet_printf(&connp->c, "%c%c%c%hu%hd%c%c%hd%s", PKT_EQUIP, pos, attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->tval == TV_BOOK ? o_ptr->pval : 0, name);
}

int Send_title(int ind, cptr title)
{
	connection_t *connp = Conn[Players[ind]->conn];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for title (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	return Packet_printf(&connp->c, "%c%s", PKT_TITLE, title);
}

int Send_extra_status(int ind, cptr status)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for extra status (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%s", PKT_EXTRA_STATUS, status);
	}
	return Packet_printf(&connp->c, "%c%s", PKT_EXTRA_STATUS, status);
}

int Send_depth(int ind, struct worldpos *wpos)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr = Players[ind], *p_ptr2 = NULL;
	bool ville = istown(wpos) && !isdungeontown(wpos); /* -> print name (TRUE) or a depth value (FALSE)? */
	cptr desc = "";
	int colour, colour2, colour_sector = TERM_L_GREEN, colour2_sector = TERM_L_GREEN, Ind2;
	cave_type **zcave;
	bool no_tele = FALSE;
	if ((zcave = getcave(&p_ptr->wpos))) no_tele = (zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) != 0;

	/* XXX this kinda thing should be done *before* calling Send_*
	 * in general, of course..	- Jir - */
	if (ville) {
		int i;
		for (i = 0; i < numtowns; i++) {
			if (town[i].x == wpos->wx && town[i].y == wpos->wy) {
				desc = town_profile[town[i].type].name;
				break;
			}
		}
	}

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
		errno = 0;
		plog(format("Connection not ready for depth (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	/* Hack for Valinor */
	if (getlevel(wpos) == 200) {
		ville = TRUE;
		desc = "Valinor";
	}
	/* Hack for PvP Arena */
	if (wpos->wx == WPOS_PVPARENA_X && wpos->wy == WPOS_PVPARENA_Y && wpos->wz == WPOS_PVPARENA_Z) {
		ville = TRUE;
		desc = "Arena";
	}

	if ((Ind2 = get_esp_link(ind, LINKF_MISC, &p_ptr2))) {
		connp2 = Conn[p_ptr2->conn];

		if (is_newer_than(&p_ptr2->version, 4, 4, 1, 5, 0, 0)) {
			/* pending recall? */
			if (p_ptr2->word_recall) colour2 = TERM_ORANGE;
			/* use as indicator for pvp_prevent_tele, actually */
			else if ((p_ptr2->mode & MODE_PVP) && p_ptr2->pvp_prevent_tele) colour2 = TERM_RED;
#ifndef RPG_SERVER
			/* able to get extra level feeling on next floor? */
			else if (TURNS_FOR_EXTRA_FEELING && (p_ptr2->turns_on_floor >= TURNS_FOR_EXTRA_FEELING)) colour2 = TERM_L_BLUE;
#endif
			/* in a town? ignore town level */
			else if (ville) colour2 = TERM_WHITE;
			/* way too low to get good exp? */
			else if (getlevel(wpos) < det_exp_level(p_ptr2->lev) - 5) colour2 = TERM_L_DARK;
			/* too low to get 100% exp? */
			else if (getlevel(wpos) < det_exp_level(p_ptr2->lev)) colour2 = TERM_YELLOW;
			/* normal exp depth or deeper (default) */
			else colour2 = TERM_WHITE;
		} else {
			colour2 = p_ptr2->word_recall;
		}

		if (is_newer_than(&p_ptr2->version, 4, 4, 1, 6, 0, 0)) {
			if (no_tele) {
				Send_cut(Ind2, 0); /* hack: clear the field shared between cut and depth */
				colour2_sector = TERM_L_DARK;
			}
			Packet_printf(&connp2->c, "%c%hu%hu%hu%c%c%c%s", PKT_DEPTH, wpos->wx, wpos->wy, wpos->wz, ville, colour2, colour2_sector, desc);
		} else {
			Packet_printf(&connp2->c, "%c%hu%hu%hu%c%hu%s", PKT_DEPTH, wpos->wx, wpos->wy, wpos->wz, ville, colour2, desc);
		}
	}

	if (is_newer_than(&p_ptr->version, 4, 4, 1, 5, 0, 0)) {
		/* pending recall? */
		if (p_ptr->word_recall) colour = TERM_ORANGE;
		/* use as indicator for pvp_prevent_tele, actually */
		else if ((p_ptr->mode & MODE_PVP) && p_ptr->pvp_prevent_tele) colour = TERM_RED;
		/* able to get extra level feeling on next floor? */
		else if (TURNS_FOR_EXTRA_FEELING && (p_ptr->turns_on_floor >= TURNS_FOR_EXTRA_FEELING)) colour = TERM_L_BLUE;
		/* in a town? ignore town level */
		else if (ville) colour = TERM_WHITE;
		/* way too low to get good exp? */
		else if (getlevel(wpos) < det_exp_level(p_ptr->lev) - 5) colour = TERM_L_DARK;
		/* too low to get 100% exp? */
		else if (getlevel(wpos) < det_exp_level(p_ptr->lev)) colour = TERM_YELLOW;
		/* normal exp depth or deeper (default) */
		else colour = TERM_WHITE;
	} else {
		colour = p_ptr->word_recall;
	}

	if (is_newer_than(&p_ptr->version, 4, 4, 1, 6, 0, 0)) {
		if (no_tele) {
			Send_cut(ind, 0); /* hack: clear the field shared between cut and depth */
			colour_sector = TERM_L_DARK;
		}
		return Packet_printf(&connp->c, "%c%hu%hu%hu%c%c%c%s", PKT_DEPTH, wpos->wx, wpos->wy, wpos->wz, ville, colour, colour_sector, desc);
	} else {
		return Packet_printf(&connp->c, "%c%hu%hu%hu%c%hu%s", PKT_DEPTH, wpos->wx, wpos->wy, wpos->wz, ville, colour, desc);
	}
}

/* (added for Valinor, but now just unused except for debugging purpose: /testdisplay in slash.c)
   This allows determining whether we're in a (fake) 'town' or not, and also
   the exact (fake) town's name that will be displayed to the player. */
int Send_depth_hack(int ind, struct worldpos *wpos, bool town, cptr desc)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr = Players[ind], *p_ptr2 = NULL;
	int colour, colour2, colour_sector = TERM_L_GREEN, colour2_sector = TERM_L_GREEN, Ind2;
	cave_type **zcave;
	bool no_tele = FALSE;
	if ((zcave = getcave(&p_ptr->wpos))) no_tele = (zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) != 0;
	
	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for depth (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if ((Ind2 = get_esp_link(ind, LINKF_MISC, &p_ptr2))) {
		connp2 = Conn[p_ptr2->conn];

		if (is_newer_than(&p_ptr2->version, 4, 4, 1, 5, 0, 0)) {
			/* pending recall? */
			if (p_ptr2->word_recall) colour2 = TERM_ORANGE;
			/* able to get extra level feeling on next floor? */
			else if (TURNS_FOR_EXTRA_FEELING && (p_ptr2->turns_on_floor >= TURNS_FOR_EXTRA_FEELING)) colour2 = TERM_L_BLUE;
			/* in a town? ignore town level */
			else if (town) colour2 = TERM_WHITE;
			/* way too low to get good exp? */
			else if (getlevel(wpos) < det_exp_level(p_ptr2->lev) - 5) colour2 = TERM_L_DARK;
			/* too low to get 100% exp? */
			else if (getlevel(wpos) < det_exp_level(p_ptr2->lev)) colour2 = TERM_YELLOW;
			/* normal exp depth or deeper (default) */
			else colour2 = TERM_WHITE;
		} else {
			colour2 = p_ptr2->word_recall;
		}

		if (is_newer_than(&p_ptr2->version, 4, 4, 1, 6, 0, 0)) {
			if (no_tele) {
				Send_cut(Ind2, 0); /* hack: clear the field shared between cut and depth */
				colour2_sector = TERM_L_DARK;
			}
			Packet_printf(&connp2->c, "%c%hu%hu%hu%c%c%c%s", PKT_DEPTH, wpos->wx, wpos->wy, wpos->wz, town, colour2, colour2_sector, desc);
		} else {
			Packet_printf(&connp2->c, "%c%hu%hu%hu%c%hu%s", PKT_DEPTH, wpos->wx, wpos->wy, wpos->wz, town, colour2, desc);
		}
	}

	if (is_newer_than(&p_ptr->version, 4, 4, 1, 5, 0, 0)) {
		/* pending recall? */
		if (p_ptr->word_recall) colour = TERM_ORANGE;
		/* able to get extra level feeling on next floor? */
		else if (TURNS_FOR_EXTRA_FEELING && (p_ptr->turns_on_floor >= TURNS_FOR_EXTRA_FEELING)) colour = TERM_L_BLUE;
		/* in a town? ignore town level */
		else if (town) colour = TERM_WHITE;
		/* way too low to get good exp? */
		else if (getlevel(wpos) < det_exp_level(p_ptr->lev) - 5) colour = TERM_L_DARK;
		/* too low to get 100% exp? */
		else if (getlevel(wpos) < det_exp_level(p_ptr->lev)) colour = TERM_YELLOW;
		/* normal exp depth or deeper (default) */
		else colour = TERM_WHITE;
	} else {
		colour = p_ptr->word_recall;
	}
	
	if (is_newer_than(&p_ptr->version, 4, 4, 1, 6, 0, 0)) {
		if (no_tele) {
			Send_cut(ind, 0); /* hack: clear the field shared between cut and depth */
			colour_sector = TERM_L_DARK;
		}
		return Packet_printf(&connp->c, "%c%hu%hu%hu%c%c%c%s", PKT_DEPTH, wpos->wx, wpos->wy, wpos->wz, town, colour, colour_sector, desc);
	} else {
		return Packet_printf(&connp->c, "%c%hu%hu%hu%c%hu%s", PKT_DEPTH, wpos->wx, wpos->wy, wpos->wz, town, p_ptr->word_recall, desc);
	}
}

int Send_food(int ind, int food)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for food (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%hu", PKT_FOOD, food);
	}
	return Packet_printf(&connp->c, "%c%hu", PKT_FOOD, food);
}

int Send_blind(int ind, bool blind)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for blind (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%c", PKT_BLIND, blind);
	}
	return Packet_printf(&connp->c, "%c%c", PKT_BLIND, blind);
}

int Send_confused(int ind, bool confused)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for confusion (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%c", PKT_CONFUSED, confused);
	}
	return Packet_printf(&connp->c, "%c%c", PKT_CONFUSED, confused);
}

int Send_fear(int ind, bool fear)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for fear (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%c", PKT_FEAR, fear);
	}
	return Packet_printf(&connp->c, "%c%c", PKT_FEAR, fear);
}

int Send_poison(int ind, bool poisoned)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for poison (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%c", PKT_POISON, poisoned);
	}
	return Packet_printf(&connp->c, "%c%c", PKT_POISON, poisoned);
}

int Send_state(int ind, bool paralyzed, bool searching, bool resting)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr = Players[ind], *p_ptr2 = NULL;

	/* give 'knocked out' priority over paralysis;
	   give stun priority over search/rest */
	if (is_newer_than(&connp->version, 4, 4, 2, 0, 0, 0) &&
	    (p_ptr->stun > 100 || (!paralyzed && p_ptr->stun))) return 0;

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for state (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%hu%hu%hu", PKT_STATE, paralyzed, searching, resting);
	}
	return Packet_printf(&connp->c, "%c%hu%hu%hu", PKT_STATE, paralyzed, searching, resting);
}

int Send_speed(int ind, int speed)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for speed (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%hd", PKT_SPEED, speed);
	}
	return Packet_printf(&connp->c, "%c%hd", PKT_SPEED, speed);
}

int Send_study(int ind, bool study)
{
	connection_t *connp = Conn[Players[ind]->conn];

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
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for cut (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%hu", PKT_CUT, cut);
	}
	return Packet_printf(&connp->c, "%c%hu", PKT_CUT, cut);
}

int Send_stun(int ind, int stun)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr = Players[ind], *p_ptr2 = NULL;

	/* give 'knocked out' priority over paralysis */
	if (is_newer_than(&connp->version, 4, 4, 2, 0, 0, 0) &&
	    p_ptr->paralyzed && p_ptr->stun <= 100) return 0;

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for stun (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%hu", PKT_STUN, stun);
	}
	return Packet_printf(&connp->c, "%c%hu", PKT_STUN, stun);
}

int Send_direction(int ind)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for direction (%d.%d.%d)",
			ind, connp->state, connp->id));
		return FALSE;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		return Packet_printf(&connp2->c, "%c", PKT_DIRECTION);
	}
	return Packet_printf(&connp->c, "%c", PKT_DIRECTION);
}

static bool hack_message = FALSE;

int Send_message(int ind, cptr msg)
{
	connection_t *connp = Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];
	char buf[80 +80]; // for +80 see below, at 'Clip end of msg'..

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
// Taking this out for now, since it's ONLY called from msg_print in util.c,
// which already performs checks - C. Blue
// (otherwise, lines with colour codes might be crippled here :| )
//	strncpy(buf, msg, 78);
//	buf[78] = '\0';
	strncpy(buf, msg, 158);
	buf[158] = '\0';

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
		connp2 = Conn[p_ptr2->conn];

		if (!BIT(connp2->state, CONN_PLAYING | CONN_READY))
		  {
		    plog(format("Connection not ready for message (%d.%d.%d)",
				ind, connp2->state, connp2->id));
		  }
		else Packet_printf(&connp2->c, "%c%S", PKT_MESSAGE, buf);
	      }
	  }
	return Packet_printf(&connp->c, "%c%S", PKT_MESSAGE, buf);
}

int Send_char(int ind, int x, int y, byte a, char c)
{
        player_type *p_ptr = Players[ind], *p_ptr2 = NULL;

	if (!BIT(Conn[Players[ind]->conn]->state, CONN_PLAYING | CONN_READY))
		return 0;

	if (p_ptr->esp_link_flags & LINKF_VIEW_DEDICATED) return 0;

	if (get_esp_link(ind, LINKF_VIEW, &p_ptr2)) {
		if (BIT(Conn[p_ptr2->conn]->state, CONN_PLAYING | CONN_READY))
			Packet_printf(&Conn[p_ptr2->conn]->c, "%c%c%c%c%c", PKT_CHAR, x, y, a, c);
	}

	return Packet_printf(&Conn[p_ptr->conn]->c, "%c%c%c%c%c", PKT_CHAR, x, y, a, c);
}

int Send_spell_info(int ind, int realm, int book, int i, cptr out_val)
{
	connection_t *connp = Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for spell info (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	return Packet_printf(&connp->c, "%c%d%d%d%hu%hu%hu%s", PKT_SPELL_INFO, p_ptr->innate_spells[0], p_ptr->innate_spells[1], p_ptr->innate_spells[2], realm, book, i, out_val);
}

/* Implementing fighting/shooting techniques, but maybe using a lua 'school' file would be better instead - C. Blue */
int Send_technique_info(int ind)
{
#ifndef ENABLE_TECHNIQUES
	return(0); /* disabled until client can handle it */
#endif
	connection_t *connp = Conn[Players[ind]->conn];
	if (!is_newer_than(&connp->version, 4, 4, 1, 2, 0, 0)) return(0);

	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for techniques info (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	return Packet_printf(&connp->c, "%c%d%d", PKT_TECHNIQUE_INFO, p_ptr->melee_techniques, p_ptr->ranged_techniques);
}

int Send_item_request(int ind)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for item request (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c", PKT_ITEM);
	}
	return Packet_printf(&connp->c, "%c", PKT_ITEM);
}

int Send_flush(int ind)
{
	connection_t *connp = Conn[Players[ind]->conn];
	//player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for flush (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
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
	connection_t *connp = Conn[p_ptr->conn];
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

	if (p_ptr->esp_link_flags & LINKF_VIEW_DEDICATED) return 0; /* bad hack for shortcut */
//	if (p_ptr->esp_link && p_ptr->esp_link_type && (p_ptr->esp_link_flags & LINKF_VIEW_DEDICATED)) return 0;

	Ind2 = get_esp_link(ind, LINKF_VIEW, &p_ptr2);

	/* Put a header on the packet */
	Packet_printf(&connp->c, "%c%hd", PKT_LINE_INFO, y);
	if (Ind2) Packet_printf(&(Conn[p_ptr2->conn]->c), "%c%hd", PKT_LINE_INFO, y);

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
			/* 4.4.3.1 clients support new RLE */
			if (is_newer_than(&connp->version, 4, 4, 3, 0, 0, 5))
			{
				/* New RLE */
				Packet_printf(&connp->c, "%c%c%c%c", c, 0xFF, a, n);
			}
			else
			{
				/* Old RLE */
				Packet_printf(&connp->c, "%c%c%c", c, a | 0x40, n);
			}

			if (Ind2)
			{
				/* 4.4.3.1 clients support new RLE */
				if (is_newer_than(&Conn[p_ptr2->conn]->version, 4, 4, 3, 0, 0, 5))
				{
					/* New RLE */
					Packet_printf(&Conn[p_ptr2->conn]->c, "%c%c%c%c", c, 0xFF, a, n);
				}
				else {
					/* Old RLE */
					Packet_printf(&Conn[p_ptr2->conn]->c, "%c%c%c", c, a | 0x40, n);
				}
			}

			/* Start again after the run */
			x = x1 - 1;
		}
		else
		{
			/* Normal, single grid */
			if (!is_newer_than(&connp->version, 4, 4, 3, 0, 0, 5)) {
				/* Remove 0x40 (TERM_PVP) if the client is old */
				Packet_printf(&connp->c, "%c%c", c, a & ~0x40); 
			}
			else
			{
				if (a == 0xFF)
				{
					/* Use RLE format as an escape sequence for 0xFF as attr */
					Packet_printf(&connp->c, "%c%c%c%c", c, 0xFF, a, 1);
				}
				else
				{
					/* Normal output */
					Packet_printf(&connp->c, "%c%c", c, a);
				}
			}

			if (Ind2)
			{
				if (!is_newer_than(&Conn[p_ptr2->conn]->version, 4, 4, 3, 0, 0, 5)) {
					/* Remove 0x40 (TERM_PVP) if the client is old */
					Packet_printf(&Conn[p_ptr2->conn]->c, "%c%c", c, a & ~0x40);
				}
				else
				{
					if (a == 0xFF)
					{
						/* Use RLE format as an escape sequence for 0xFF as attr */
						Packet_printf(&Conn[p_ptr2->conn]->c, "%c%c%c%c", c, 0xFF, a, 1);
					}
					else
					{
						/* Normal output */
						Packet_printf(&Conn[p_ptr2->conn]->c, "%c%c", c, a);
					}
				}
			}
		}
	}

	/* Hack -- Prevent buffer overruns by flushing after each line sent */
	/* Send_reliable(Players[ind]->conn); */
	
	return 1;
}

int Send_mini_map(int ind, int y, byte *sa, char *sc)
{
	player_type *p_ptr = Players[ind];
	connection_t *connp = Conn[p_ptr->conn];
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

	if (p_ptr->esp_link_flags & LINKF_VIEW_DEDICATED) return 0;

	/* Sending this packet to a mind-linked person is bad - mikaelh */
#if 0
	if ((Ind2 = get_esp_link(ind, LINKF_VIEW, &p_ptr2)))
		connp2 = Conn[p_ptr2->conn];
#endif
	
	/* Packet header */
	Packet_printf(&connp->c, "%c%hd", PKT_MINI_MAP, y);
     	if (Ind2) Packet_printf(&connp2->c, "%c%hd", PKT_MINI_MAP, y);

	/* Each column */
	for (x = 0; x < 80; x++)
	{
		/* Obtain the char/attr pair */
		c = sc[x];
		a = sa[x];

		/* Start looking here */
		x1 = x + 1;

		/* Start with count of 1 */
		n = 1;

		/* Count repetitions of this grid */
		while (sc[x1] == c && sa[x1] == a && x1 < 80)
		{
			/* Increment count and column */
			n++;
			x1++;
		}

		/* RLE if there at least 2 similar grids in a row */
		if (n >= 2)
		{
			/* 4.4.3.1 clients support new RLE */
			if (is_newer_than(&connp->version, 4, 4, 3, 0, 0, 5))
			{
				/* New RLE */
				Packet_printf(&connp->c, "%c%c%c%c", c, 0xFF, a, n);
			}
			else
			{
				/* Old RLE */
				Packet_printf(&connp->c, "%c%c%c", c, a | 0x40, n);
			}

			if (Ind2)
			{
				/* 4.4.3.1 clients support new RLE */
				if (is_newer_than(&Conn[p_ptr2->conn]->version, 4, 4, 3, 0, 0, 5))
				{
					/* New RLE */
					Packet_printf(&Conn[p_ptr2->conn]->c, "%c%c%c%c", c, 0xFF, a, n);
				}
				else {
					/* Old RLE */
					Packet_printf(&Conn[p_ptr2->conn]->c, "%c%c%c", c, a | 0x40, n);
				}
			}

			/* Start again after the run */
			x = x1 - 1;
		}
		else
		{
			/* Normal, single grid */
			if (!is_newer_than(&connp->version, 4, 4, 3, 0, 0, 5)) {
				/* Remove 0x40 (TERM_PVP) if the client is old */
				Packet_printf(&connp->c, "%c%c", c, a & ~0x40); 
			}
			else
			{
				if (a == 0xFF)
				{
					/* Use RLE format as an escape sequence for 0xFF as attr */
					Packet_printf(&connp->c, "%c%c%c%c", c, 0xFF, a, 1);
				}
				else
				{
					/* Normal output */
					Packet_printf(&connp->c, "%c%c", c, a);
				}
			}

			if (Ind2)
			{
				if (!is_newer_than(&Conn[p_ptr2->conn]->version, 4, 4, 3, 0, 0, 5)) {
					/* Remove 0x40 (TERM_PVP) if the client is old */
					Packet_printf(&Conn[p_ptr2->conn]->c, "%c%c", c, a & ~0x40);
				}
				else
				{
					if (a == 0xFF)
					{
						/* Use RLE format as an escape sequence for 0xFF as attr */
						Packet_printf(&Conn[p_ptr2->conn]->c, "%c%c%c%c", c, 0xFF, a, 1);
					}
					else
					{
						/* Normal output */
						Packet_printf(&Conn[p_ptr2->conn]->c, "%c%c", c, a);
					}
				}
			}
		}
	}

	/* Hack -- Prevent buffer overruns by flushing after each line sent */
	/* Send_reliable(Players[ind]->conn); */
	
	return 1;
}

//int Send_store(int ind, char pos, byte attr, int wgt, int number, int price, cptr name)
int Send_store(int ind, char pos, byte attr, int wgt, int number, int price, cptr name, char tval, char sval, s16b pval)
{
	connection_t *connp = Conn[Players[ind]->conn];
#ifdef MINDLINK_STORE
	connection_t *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/
	
#endif

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for store item (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	/* Hack -- send pval only if it's School book */
	if (tval != TV_BOOK) pval = 0;

#ifdef MINDLINK_STORE
	if (get_esp_link(ind, LINKF_VIEW, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		if (is_newer_than(&p_ptr2->version, 4, 4, 7, 0, 0, 0))
			Packet_printf(&connp2->c, "%c%c%c%hd%hd%d%S%c%c%hd", PKT_STORE, pos, attr, wgt, number, price, name, tval, sval, pval);
		else
			Packet_printf(&connp2->c, "%c%c%c%hd%hd%d%s%c%c%hd", PKT_STORE, pos, attr, wgt, number, price, name, tval, sval, pval);
	}
#endif

	if (is_newer_than(&Players[ind]->version, 4, 4, 7, 0, 0, 0))
		return Packet_printf(&connp->c, "%c%c%c%hd%hd%d%S%c%c%hd", PKT_STORE, pos, attr, wgt, number, price, name, tval, sval, pval);
	else
		return Packet_printf(&connp->c, "%c%c%c%hd%hd%d%s%c%c%hd", PKT_STORE, pos, attr, wgt, number, price, name, tval, sval, pval);
}

/* Send_store() variant for custom spellbooks */
int Send_store_wide(int ind, char pos, byte attr, int wgt, int number, int price, cptr name, char tval, char sval, s16b pval,
    byte xtra1, byte xtra2, byte xtra3, byte xtra4, byte xtra5, byte xtra6, byte xtra7, byte xtra8, byte xtra9)
{
	connection_t *connp = Conn[Players[ind]->conn];
#ifdef MINDLINK_STORE
	connection_t *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/
	
#endif

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for store item (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	/* Hack -- send pval only if it's School book */
	if (tval != TV_BOOK) pval = 0;

#ifdef MINDLINK_STORE
	if (get_esp_link(ind, LINKF_VIEW, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		if (is_newer_than(&p_ptr2->version, 4, 4, 7, 0, 0, 0))
			Packet_printf(&connp2->c, "%c%c%c%hd%hd%d%S%c%c%hd%c%c%c%c%c%c%c%c%c", PKT_STORE_WIDE, pos, attr, wgt, number, price, name, tval, sval, pval, xtra1, xtra2, xtra3, xtra4, xtra5, xtra6, xtra7, xtra8, xtra9);
		else
			Packet_printf(&connp2->c, "%c%c%c%hd%hd%d%s%c%c%hd%c%c%c%c%c%c%c%c%c", PKT_STORE_WIDE, pos, attr, wgt, number, price, name, tval, sval, pval, xtra1, xtra2, xtra3, xtra4, xtra5, xtra6, xtra7, xtra8, xtra9);
	}
#endif

	if (is_newer_than(&Players[ind]->version, 4, 4, 7, 0, 0, 0))
		return Packet_printf(&connp->c, "%c%c%c%hd%hd%d%S%c%c%hd%c%c%c%c%c%c%c%c%c", PKT_STORE_WIDE, pos, attr, wgt, number, price, name, tval, sval, pval, xtra1, xtra2, xtra3, xtra4, xtra5, xtra6, xtra7, xtra8, xtra9);
	else
		return Packet_printf(&connp->c, "%c%c%c%hd%hd%d%s%c%c%hd%c%c%c%c%c%c%c%c%c", PKT_STORE_WIDE, pos, attr, wgt, number, price, name, tval, sval, pval, xtra1, xtra2, xtra3, xtra4, xtra5, xtra6, xtra7, xtra8, xtra9);
}

/* For new non-shop stores (SPECIAL flag) - C. Blue */
int Send_store_special_str(int ind, char line, char col, char attr, char *str) {
	connection_t *connp = Conn[Players[ind]->conn];
#ifdef MINDLINK_STORE
	connection_t *connp2;
	player_type *p_ptr2 = NULL;
#endif

	if (!is_newer_than(&connp->version, 4, 4, 6, 1, 0, 0)) return 0;

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
		errno = 0;
		plog(format("Connection not ready for store item (%d.%d.%d)",
		    ind, connp->state, connp->id));
		return 0;
	}

#ifdef MINDLINK_STORE
	if (get_esp_link(ind, LINKF_VIEW, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		if (is_newer_than(&connp2->version, 4, 4, 6, 1, 0, 0))
			Packet_printf(&connp2->c, "%c%c%c%c%s", PKT_STORE_SPECIAL_STR, line, col, attr, str);
	}
#endif

	return Packet_printf(&connp->c, "%c%c%c%c%s", PKT_STORE_SPECIAL_STR, line, col, attr, str);
}

/* For new non-shop stores (SPECIAL flag) - C. Blue */
int Send_store_special_char(int ind, char line, char col, char attr, char c) {
	connection_t *connp = Conn[Players[ind]->conn];
#ifdef MINDLINK_STORE
	connection_t *connp2;
	player_type *p_ptr2 = NULL;
#endif

	if (!is_newer_than(&connp->version, 4, 4, 6, 1, 0, 0)) return 0;

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
		errno = 0;
		plog(format("Connection not ready for store item (%d.%d.%d)",
		    ind, connp->state, connp->id));
		return 0;
	}

#ifdef MINDLINK_STORE
	if (get_esp_link(ind, LINKF_VIEW, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		if (is_newer_than(&connp2->version, 4, 4, 6, 1, 0, 0))
			Packet_printf(&connp2->c, "%c%c%c%c%c", PKT_STORE_SPECIAL_CHAR, line, col, attr, c);
	}
#endif

	return Packet_printf(&connp->c, "%c%c%c%c%c", PKT_STORE_SPECIAL_CHAR, line, col, attr, c);
}

/* For new non-shop stores (SPECIAL flag) - C. Blue */
int Send_store_special_clr(int ind, char line_start, char line_end) {
	connection_t *connp = Conn[Players[ind]->conn];
#ifdef MINDLINK_STORE
	connection_t *connp2;
	player_type *p_ptr2 = NULL;
#endif

	if (!is_newer_than(&connp->version, 4, 4, 6, 1, 0, 0)) return 0;

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
		errno = 0;
		plog(format("Connection not ready for store item (%d.%d.%d)",
		    ind, connp->state, connp->id));
		return 0;
	}

#ifdef MINDLINK_STORE
	if (get_esp_link(ind, LINKF_VIEW, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		if (is_newer_than(&connp2->version, 4, 4, 6, 1, 0, 0))
			Packet_printf(&connp2->c, "%c%c%c", PKT_STORE_SPECIAL_CLR, line_start, line_end);
	}
#endif

	return Packet_printf(&connp->c, "%c%c%c", PKT_STORE_SPECIAL_CLR, line_start, line_end);
}

/*
 * This function is supposed to handle 'store actions' too,
 * like 'buy' 'identify' 'heal' 'bid to an auction'		- Jir -
 */
int Send_store_info(int ind, int num, cptr store, cptr owner, int items, int purse, byte attr, char c)
{
	connection_t *connp = Conn[Players[ind]->conn];
#ifdef MINDLINK_STORE
	connection_t *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/
#endif

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for store info (%d.%d.%d)",
					ind, connp->state, connp->id));
		return 0;
	}

#ifdef MINDLINK_STORE
	if (get_esp_link(ind, LINKF_VIEW, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		if (is_newer_than(&connp2->version, 4, 4, 4, 0, 0, 0)) {
			Packet_printf(&connp2->c, "%c%hd%s%s%hd%d%c%c", PKT_STORE_INFO, num, store, owner, items, purse, attr, c);
		} else {
			Packet_printf(&connp2->c, "%c%hd%s%s%hd%d", PKT_STORE_INFO, num, store, owner, items, purse);
		}
	}
#endif

	if (is_newer_than(&connp->version, 4, 4, 4, 0, 0, 0)) {
		return Packet_printf(&connp->c, "%c%hd%s%s%hd%d%c%c", PKT_STORE_INFO, num, store, owner, items, purse, attr, c);
	} else {
		return Packet_printf(&connp->c, "%c%hd%s%s%hd%d", PKT_STORE_INFO, num, store, owner, items, purse);
	}
}

int Send_store_action(int ind, char pos, u16b bact, u16b action, cptr name, char attr, char letter, s16b cost, byte flag)
{
	connection_t *connp = Conn[Players[ind]->conn];
#ifdef MINDLINK_STORE
	connection_t *connp2;
	player_type *p_ptr = Players[ind], *p_ptr2;
#endif

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for store info (%d.%d.%d)",
					ind, connp->state, connp->id));
		return 0;
	}

#ifdef MINDLINK_STORE
	if (get_esp_link(ind, LINKF_VIEW, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%c%hd%hd%s%c%c%hd%c", PKT_BACT, pos, bact, action, name, attr, letter, cost, flag);
	}
#endif

	return Packet_printf(&connp->c, "%c%c%hd%hd%s%c%c%hd%c", PKT_BACT, pos, bact, action, name, attr, letter, cost, flag);
}

int Send_store_sell(int ind, int price)
{
	connection_t *connp = Conn[Players[ind]->conn];
#ifdef MINDLINK_STORE
	connection_t *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/
#endif

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for sell price (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

#ifdef MINDLINK_STORE
	if (get_esp_link(ind, LINKF_VIEW, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%d", PKT_SELL, price);
	}
#endif

	return Packet_printf(&connp->c, "%c%d", PKT_SELL, price);
}

int Send_store_kick(int ind)
{
	connection_t *connp = Conn[Players[ind]->conn];
#ifdef MINDLINK_STORE
	connection_t *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/
#endif

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for store_kick (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

#ifdef MINDLINK_STORE
	if (get_esp_link(ind, LINKF_VIEW, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c", PKT_STORE_LEAVE);
	}
#endif

	return Packet_printf(&connp->c, "%c", PKT_STORE_LEAVE);
}

int Send_target_info(int ind, int x, int y, cptr str)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/
	char buf[80];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for target info (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	/* Copy */
	strncpy(buf, str, 80 - 1);
	/* Paranoia -- Add null */
	buf[80 - 1] = '\0';

	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%c%c%s", PKT_TARGET_INFO, x, y, buf);
	}
	return Packet_printf(&connp->c, "%c%c%c%s", PKT_TARGET_INFO, x, y, buf);
}
/* type is for client-side, regarding efficiency options;
   vol is the relative volume, if it stems from a source nearby instead of concerning the player directly;
   player_id is the player it actually concerns; - C. Blue */
int Send_sound(int ind, int sound, int alternative, int type, int vol, s32b player_id) {
	connection_t *connp = Conn[Players[ind]->conn];

	/* Mind-linked to someone? Send him our sound too! */
	player_type *p_ptr2 = NULL;
	connection_t *connp2 = NULL;
	/* If we're the target, we still hear our own sounds! */
//	if (Players[ind]->esp_link_flags & LINKF_VIEW_DEDICATED) ;//nothing
	/* Get target player */
	if (get_esp_link(ind, LINKF_VIEW, &p_ptr2)) connp2 = Conn[p_ptr2->conn];
	/* Send same info to target player, if available */
	if (connp2) {
		if (is_newer_than(&connp2->version, 4, 4, 5, 3, 0, 0))
			Packet_printf(&connp2->c, "%c%d%d%d%d%d", PKT_SOUND, sound, alternative, type, vol, player_id);
		else if (is_newer_than(&connp2->version, 4, 4, 5, 1, 0, 0))
			Packet_printf(&connp2->c, "%c%d%d%d", PKT_SOUND, sound, alternative, type);
		else if (is_newer_than(&connp2->version, 4, 4, 5, 0, 0, 0))
			Packet_printf(&connp2->c, "%c%d%d", PKT_SOUND, sound, alternative);
		else
			Packet_printf(&connp2->c, "%c%c", PKT_SOUND, sound);
	}

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
		errno = 0;
		plog(format("Connection not ready for sound (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

//	if (is_admin(Players[ind])) s_printf("USE_SOUND_2010: sound %d (alt %d) sent to player %s (%d).\n", sound, alternative, Players[ind]->name, ind);//debug

	if (is_newer_than(&connp->version, 4, 4, 5, 3, 0, 0)) {
		return Packet_printf(&connp->c, "%c%d%d%d%d%d", PKT_SOUND, sound, alternative, type, vol, player_id);
	} else if (is_newer_than(&connp->version, 4, 4, 5, 1, 0, 0)) {
		return Packet_printf(&connp->c, "%c%d%d%d", PKT_SOUND, sound, alternative, type);
	} else if (is_newer_than(&connp->version, 4, 4, 4, 5, 0, 0)) {
		return Packet_printf(&connp->c, "%c%d%d", PKT_SOUND, sound, alternative);
	} else {
		return Packet_printf(&connp->c, "%c%c", PKT_SOUND, sound);
	}
}

#ifdef USE_SOUND_2010
int Send_music(int ind, int music) {
	connection_t *connp = Conn[Players[ind]->conn];

	/* Mind-linked to someone? Send him our music too! */
	player_type *p_ptr2 = NULL;
	connection_t *connp2 = NULL;
	/* If we're the target, we won't hear our own music */
	if (Players[ind]->esp_link_flags & LINKF_VIEW_DEDICATED) return(0);
	/* Get target player */
	if (get_esp_link(ind, LINKF_VIEW, &p_ptr2)) connp2 = Conn[p_ptr2->conn];
	/* Send same info to target player, if available */
	if (connp2) {
		if (p_ptr2->music_current != music) {
			p_ptr2->music_current = music;
			if (is_newer_than(&connp2->version, 4, 4, 4, 5, 0, 0))
				Packet_printf(&connp2->c, "%c%c", PKT_MUSIC, music);
		}
	}

	if (Players[ind]->music_current == music) return(-1);
	Players[ind]->music_current = music;

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
		errno = 0;
		plog(format("Connection not ready for music (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	if (!is_newer_than(&connp->version, 4, 4, 4, 5, 0, 0)) return(-1);
//	s_printf("USE_SOUND_2010: music %d sent to player %s (%d).\n", music, Players[ind]->name, ind);//debug
	return Packet_printf(&connp->c, "%c%c", PKT_MUSIC, music);
}
#endif

int Send_beep(int ind)
{
	connection_t *connp = Conn[Players[ind]->conn];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for beep (page) (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	return Packet_printf(&connp->c, "%c", PKT_BEEP);
}

int Send_AFK(int ind, byte afk)
{
	connection_t *connp = Conn[Players[ind]->conn];
//	player_type *p_ptr = Players[ind];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for AFK (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	return Packet_printf(&connp->c, "%c%c", PKT_AFK, afk);
}

int Send_encumberment(int ind, byte cumber_armor, byte awkward_armor, byte cumber_glove, byte heavy_wield, byte heavy_shield, byte heavy_shoot,
                        byte icky_wield, byte awkward_wield, byte easy_wield, byte cumber_weight, byte monk_heavyarmor, byte rogue_heavyarmor, byte awkward_shoot)
{
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for encumberment (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		if (!is_newer_than(&connp2->version, 4, 4, 2, 0, 0, 0)) {
			Packet_printf(&connp2->c, "%c%c%c%c%c%c%c%c%c%c%c%c%c", PKT_ENCUMBERMENT, cumber_armor, awkward_armor, cumber_glove, heavy_wield, heavy_shield, heavy_shoot,
                                icky_wield, awkward_wield, easy_wield, cumber_weight, monk_heavyarmor, awkward_shoot);
                } else {
			Packet_printf(&connp2->c, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c", PKT_ENCUMBERMENT, cumber_armor, awkward_armor, cumber_glove, heavy_wield, heavy_shield, heavy_shoot,
                                icky_wield, awkward_wield, easy_wield, cumber_weight, monk_heavyarmor, rogue_heavyarmor, awkward_shoot);
                }
	}
	if (!is_newer_than(&connp->version, 4, 4, 2, 0, 0, 0)) {
		return Packet_printf(&connp->c, "%c%c%c%c%c%c%c%c%c%c%c%c%c", PKT_ENCUMBERMENT, cumber_armor, awkward_armor, cumber_glove, heavy_wield, heavy_shield, heavy_shoot,
	                icky_wield, awkward_wield, easy_wield, cumber_weight, monk_heavyarmor, awkward_shoot);
        } else {
		return Packet_printf(&connp->c, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c", PKT_ENCUMBERMENT, cumber_armor, awkward_armor, cumber_glove, heavy_wield, heavy_shield, heavy_shoot,
	                icky_wield, awkward_wield, easy_wield, cumber_weight, monk_heavyarmor, rogue_heavyarmor, awkward_shoot);
        }
}


int Send_special_line(int ind, s32b max, s32b line, byte attr, cptr buf)
{
	connection_t *connp = Conn[Players[ind]->conn];
	char temp[ONAME_LEN - 3], xattr = 'w', temp2[ONAME_LEN];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
		errno = 0;
		plog(format("Connection not ready for special line (%d.%d.%d)",
		    ind, connp->state, connp->id));
		return 0;
	}

	switch (attr) {
	case TERM_DARK:		xattr = 'd';break;
	case TERM_RED:		xattr = 'r';break;
	case TERM_L_DARK:	xattr = 'D';break;
	case TERM_L_RED:	xattr = 'R';break;
	case TERM_WHITE:	xattr = 'w';break;
	case TERM_GREEN:	xattr = 'g';break;
	case TERM_L_WHITE:	xattr = 'W';break;
	case TERM_L_GREEN:	xattr = 'G';break;
	case TERM_SLATE:	xattr = 's';break;
	case TERM_BLUE:		xattr = 'b';break;
	case TERM_VIOLET:	xattr = 'v';break;
	case TERM_L_BLUE:	xattr = 'B';break;
	case TERM_ORANGE:	xattr = 'o';break;
	case TERM_UMBER:	xattr = 'u';break;
	case TERM_YELLOW:	xattr = 'y';break;
	case TERM_L_UMBER:	xattr = 'U';break;
	case TERM_MULTI:	xattr = 'm';break;
	case TERM_POIS:		xattr = 'p';break;
	case TERM_FIRE:		xattr = 'f';break;
	case TERM_COLD:		xattr = 'c';break;
	case TERM_ACID:		xattr = 'a';break;
	case TERM_ELEC:		xattr = 'e';break;
	case TERM_LITE:		xattr = 'L';break;
	case TERM_HALF:		xattr = 'h';break;
	case TERM_CONF:		xattr = 'C';break;
	case TERM_SOUN:		xattr = 'S';break;
	case TERM_SHAR:		xattr = 'H';break;
	case TERM_DARKNESS:	xattr = 'A';break;
	case TERM_SHIELDM:	xattr = 'M';break;
	case TERM_SHIELDI:	xattr = 'I';break;
	}

	strncpy(temp, buf, ONAME_LEN - 4);
	temp[ONAME_LEN - 4] = '\0';

	strcpy(temp2, "\377");
	temp2[1] = xattr; temp2[2] = '\0';

#if 0 /* default (no colour codes conversion) */
	strcat(temp2, temp);
#else /* allow colour code shortcut - C. Blue */
 #if 0 /* '{' = colour, '{{' = normal */
	{
		char *t = temp, *t2 = temp2 + 2;
		while (*t) {
			if (*t != '{') *t2++ = *t++;
			else {
				/* double '{' ? */
				if (*(t + 1) == '{') {
					*t2++ = *t++;
				}
				/* single '{' ? -> becomes colour code */
				else {
					*t2++ = '\377';
				}
				t++;
			}
		}
		*t2 = 0;
	}
 #endif
 #if 1 /* '\{' = colour, '{' = normal */
	{
		char *t = temp, *t2 = temp2 + 2;
		while (*t) {
			if (*t != '\\') *t2++ = *t++;
			else {
				/* double code ? -> colour */
				if (*(t + 1) == '{') {
					*t2++ = '\377';
					t += 2;
				}
				/* single '{' ? keep */
				else *t2++ = *t++;
			}
		}
		*t2 = 0;
	}
 #endif
#endif

	if (is_newer_than(&Players[ind]->version, 4, 4, 7, 0, 0, 0))
		return Packet_printf(&connp->c, "%c%d%d%c%I", PKT_SPECIAL_LINE, max, line, attr, temp2);
	else if (is_newer_than(&Players[ind]->version, 4, 4, 6, 1, 0, 0))
		return Packet_printf(&connp->c, "%c%hd%hd%c%I", PKT_SPECIAL_LINE, max, line, attr, temp2);
	else {
		/* Cut it off so old clients can handle it, ouch */
		temp2[79] = 0;
		return Packet_printf(&connp->c, "%c%d%d%c%s", PKT_SPECIAL_LINE, max, line, attr, temp2);
	}
}

int Send_floor(int ind, char tval)
{
	connection_t *connp = Conn[Players[ind]->conn];

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
	connection_t *connp = Conn[Players[ind]->conn];

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

int Send_party(int ind, bool leave, bool clear)
{
    int i;
    for (i = 1; i <= NumPlayers; i++)
    {
	player_type *p_ptr = Players[i];
	connection_t *connp = Conn[p_ptr->conn];
	char bufn[90], bufm[20], bufo[50], buf[10];

	if (Players[i]->party != Players[ind]->party) continue;

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection nor ready for party info (%d.%d.%d)",
			i, connp->state, connp->id));
		return 0;
	}

	if (!is_newer_than(&p_ptr->version, 4, 4, 7, 0, 0, 0)) {
		if (parties[p_ptr->party].mode == PA_IRONTEAM)
			snprintf(bufn, 90, "Party (Iron Team): %s", parties[p_ptr->party].name);
		else
			snprintf(bufn, 90, "Party  : %s", parties[p_ptr->party].name);

		bufm[0] = '\0';
		bufo[0] = '\0';

		if (p_ptr->party > 0) {
			strcpy(bufm, "Members: ");
			snprintf(buf, 10, "%d", parties[p_ptr->party].members);
			strcat(bufm, buf);

			strcpy(bufo, "Owner  : ");
			strcat(bufo, parties[p_ptr->party].owner);
		}
	} else {
		bufn[0] = '\0';
		bufm[0] = '\0';
		bufo[0] = '\0';

		if (p_ptr->party > 0 && !clear && (!leave || i != ind)) {
			if (parties[p_ptr->party].mode == PA_IRONTEAM)
				snprintf(bufn, 90, "Iron Team: '\377%c%s\377w'", COLOUR_CHAT_PARTY, parties[p_ptr->party].name);
			else
				snprintf(bufn, 90, "Party: '\377%c%s\377w'", COLOUR_CHAT_PARTY, parties[p_ptr->party].name);

			snprintf(buf, 10, "%d", parties[p_ptr->party].members);
			strcpy(bufm, buf);
			if (parties[p_ptr->party].members == 1) strcat(bufm, " member");
			else strcat(bufm, " members");

			strcpy(bufo, "owner: ");
			strcat(bufo, parties[p_ptr->party].owner);
		}
	}

	Packet_printf(&connp->c, "%c%s%s%s", PKT_PARTY, bufn, bufm, bufo);
    }
    return(1);
}

int Send_guild(int ind, bool leave, bool clear)
{
    int i;

    for (i = 1; i <= NumPlayers; i++)
    {
	player_type *p_ptr = Players[i];
	connection_t *connp = Conn[p_ptr->conn];
	char bufn[90], bufm[20], bufo[50], buf[10];

	if (!is_newer_than(&p_ptr->version, 4, 4, 7, 0, 0, 0)) continue;

	if (Players[i]->guild != Players[ind]->guild) continue;

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection nor ready for guild info (%d.%d.%d)",
			i, connp->state, connp->id));
		return 0;
	}

	bufn[0] = '\0';
	bufm[0] = '\0';
	bufo[0] = '\0';

	if (p_ptr->guild > 0 && !clear && (!leave || i != ind)) {
		int memb = guilds[p_ptr->guild].members;
		cptr master = lookup_player_name(guilds[p_ptr->guild].master);

		snprintf(bufn, 90, "Guild: '\377%c%s\377w'", COLOUR_CHAT_GUILD, guilds[p_ptr->guild].name);

		snprintf(buf, 10, "%d", memb);
		strcpy(bufm, buf);
		if (memb == 1) strcat(bufm, " member");
		else strcat(bufm, " members");

		
		if (master) {
			strcpy(bufo, "master: ");
			strcat(bufo, master);
		} else {
			strcpy(bufo, "master: <leaderless>");
		}
	}

	Packet_printf(&connp->c, "%c%s%s%s", PKT_GUILD, bufn, bufm, bufo);
    }
    return(1);
}

int Send_special_other(int ind)
{
	connection_t *connp = Conn[Players[ind]->conn];

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
	connection_t *connp = Conn[p_ptr->conn];
	s16b skills[12];
	int i, tmp = 0;
	object_type *o_ptr;

	/* Fighting skill */
	o_ptr = &p_ptr->inventory[INVEN_WIELD];
	tmp = o_ptr->to_h;
	/* dual-wield? */
#if 0 /* hmm seemed a bit glitchy? replacing below.. */
	o_ptr = &p_ptr->inventory[INVEN_ARM];
	if (o_ptr->k_idx && o_ptr->tval != TV_SHIELD) tmp += o_ptr->to_h;
	/* average? */
	if (p_ptr->dual_wield) tmp /= 2;
#else
	if (p_ptr->dual_wield && p_ptr->dual_mode) {
		o_ptr = &p_ptr->inventory[INVEN_ARM];
		tmp += o_ptr->to_h;
		/* average */
		tmp /= 2;
	}
#endif
	
	tmp += p_ptr->to_h + p_ptr->to_h_melee;
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
	connection_t *connp = Conn[Players[ind]->conn];

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
	connection_t *connp = Conn[Players[ind]->conn], *connp2;
	player_type *p_ptr2 = NULL; /*, *p_ptr = Players[ind];*/

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for monster health bar (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	if (get_esp_link(ind, LINKF_MISC, &p_ptr2)) {
		connp2 = Conn[p_ptr2->conn];
		Packet_printf(&connp2->c, "%c%c%c", PKT_MONSTER_HEALTH, num, attr);
	}

	return Packet_printf(&connp->c, "%c%c%c", PKT_MONSTER_HEALTH, num, attr);
}

int Send_chardump(int ind, cptr tag)
{
	connection_t *connp = Conn[Players[ind]->conn];

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for chardump (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

#if 0 /* This works, but won't help non-death char dumps that are taken manually */
	/* hack: Quickly update the client's unique list first */
	for (i = 0; i < MAX_R_IDX; i++)
		if (r_info[i].flags1 & RF1_UNIQUE)
			Send_unique_monster(ind, i);
#endif

	if (!is_newer_than(&connp->version, 4, 4, 2, 0, 0, 0) ||
	    MY_VERSION <= (4 << 12 | 4 << 8 | 2 << 4 | 0))
		return Packet_printf(&connp->c, "%c", PKT_CHARDUMP);
	else
		return Packet_printf(&connp->c, "%c%s", PKT_CHARDUMP, tag);
}

int Send_unique_monster(int ind, int r_idx)
{
	connection_t *connp = Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!is_newer_than(&connp->version, 4, 4, 1, 7, 0, 0)) return(0);

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for unique monster (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}
	return Packet_printf(&connp->c, "%c%d%d%s", PKT_UNIQUE_MONSTER, r_info[r_idx].u_idx, p_ptr->r_killed[r_idx], r_name + r_info[r_idx].name);
}

int Send_weather(int ind, int weather_type, int weather_wind, int weather_gen_speed, int weather_intensity, int weather_speed, bool update_clouds, bool revoke_clouds)
{
	int n, i, c;
	int cx1, cy1, cx2, cy2;

	/* Note: This is NOT the client-side limit, but rather the current
	   server-side limit how many clouds we want to transmit to the client. */
	const int cloud_limit = 10;

	connection_t *connp = Conn[Players[ind]->conn];

	/* Mind-linked to someone? Send him our weather */
	player_type *p_ptr2 = NULL;
	connection_t *connp2 = NULL;
	/* If we're the target, we won't see our own weather */
	if (Players[ind]->esp_link_flags & LINKF_VIEW_DEDICATED) return(0);
	/* Get target player */
	if (get_esp_link(ind, LINKF_VIEW, &p_ptr2)) connp2 = Conn[p_ptr2->conn];


	wilderness_type *w_ptr = &wild_info[Players[ind]->wpos.wy][Players[ind]->wpos.wx];

	if (!is_newer_than(&connp->version, 4, 4, 2, 0, 0, 0)) return(0);

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for weather (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	/* hack: tell client to disable all clouds */
	if (revoke_clouds) c = -1;
	/* tell client to expect clouds */
//	else if (update_clouds) c = w_ptr->clouds_to_update;
	else if (update_clouds) c = cloud_limit;
	else c = 0;
	/* fix limit! (or crash/bug results on client-side,
	   since it tries to read ALL clouds (despite discarding
	   those which are too much) and hence tries to read more
	   packets than we are actually sending, corrupting itself) */
//redundant, see above
//	if (c > cloud_limit) c = cloud_limit;
#ifdef TEST_SERVER
if (weather_type > 0) s_printf("weather_type: %d\n", weather_type);
#endif
	n = Packet_printf(&connp->c, "%c%d%d%d%d%d%d%d%d", PKT_WEATHER,
	    weather_type, weather_wind, weather_gen_speed, weather_intensity, weather_speed,
	    Players[ind]->panel_col_prt, Players[ind]->panel_row_prt, c);
	if (connp2) Packet_printf(&connp2->c, "%c%d%d%d%d%d%d%d%d", PKT_WEATHER,
	    weather_type, weather_wind, weather_gen_speed, weather_intensity, weather_speed,
	    Players[ind]->panel_col_prt, Players[ind]->panel_row_prt, c);

#ifdef TEST_SERVER
#if 0
s_printf("clouds_to_update %d (%d)\n", c, w_ptr->clouds_to_update);
#endif
#endif

	/* re-send all clouds that have 'updated' flag set */
	if (c > 0) {
		for (i = 0; i < cloud_limit; i++) {
#if 0 /* unfinished//also, make it visible to all players, so if0 here - see "/jokeweather" instead */
			/* fun stuff ;) abuse the last cloud for this */
			if (Players[ind]->joke_weather && (i == cloud_limit - 1)) {
				n = Packet_printf(&connp->c, "%d%d%d%d%d%d%d%d", i,
				    Players[ind]->px - 1, Players[ind]->py,
				    Players[ind]->px + 1, Players[ind]->py,
				    7, 0, 0);
				continue;
			}
#endif

//			if (w_ptr->cloud_updated[i]) {
//s_printf("cloud_updated %d\n", i);

//DEBUGGING... (local should be correct, but that's not all yet..)
#if 0 /* send global cloud coordinates, same as the server uses? */
				n = Packet_printf(&connp->c, "%d%d%d%d%d%d%d%d",
				    i, w_ptr->cloud_x1[i], w_ptr->cloud_y1[i], w_ptr->cloud_x2[i], w_ptr->cloud_y2[i],
				    w_ptr->cloud_dsum[i], w_ptr->cloud_xm100[i], w_ptr->cloud_ym100[i]);
#else /* send local cloud coordinates especially for client-side? */
				/* convert global cloud coordinates (server-side)
				   to local coordinates for the client, who only
				   has to pay attention to one sector at a time
				   (ie the one the player is currently in): */
				cx1 = w_ptr->cloud_x1[i] - Players[ind]->wpos.wx * MAX_WID;
				cy1 = w_ptr->cloud_y1[i] - Players[ind]->wpos.wy * MAX_HGT;
				cx2 = w_ptr->cloud_x2[i] - Players[ind]->wpos.wx * MAX_WID;
				cy2 = w_ptr->cloud_y2[i] - Players[ind]->wpos.wy * MAX_HGT;
				/* hack: 'disable' a cloud slot */
				if (w_ptr->cloud_x1[i] == -9999) cx1 = -9999;

#ifdef TEST_SERVER
#if 0
s_printf("sending local cloud %d (%d,%d - %d,%d)\n", i, cx1, cy1, cx2, cy2);
#endif
#endif

				n = Packet_printf(&connp->c, "%d%d%d%d%d%d%d%d",
				    i, cx1, cy1, cx2, cy2,
				    w_ptr->cloud_dsum[i], w_ptr->cloud_xm100[i], w_ptr->cloud_ym100[i]);
				if (connp2) Packet_printf(&connp2->c, "%d%d%d%d%d%d%d%d",
				    i, cx1, cy1, cx2, cy2,
				    w_ptr->cloud_dsum[i], w_ptr->cloud_xm100[i], w_ptr->cloud_ym100[i]);
#endif
//			}
		}
	}

	return n;
}

int Send_inventory_revision(int ind)
{
	connection_t *connp = Conn[Players[ind]->conn];
	player_type *p_ptr = Players[ind];

	if (!is_newer_than(&connp->version, 4, 4, 2, 1, 0, 0)) return(0);

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for inventory revision (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	return Packet_printf(&connp->c, "%c%d", PKT_INVENTORY_REV, p_ptr->inventory_revision);
}

int Send_account_info(int ind)
{
	connection_t *connp = Conn[Players[ind]->conn];
	struct account *l_acc;
	u32b acc_flags = 0;

	if (!is_newer_than(&connp->version, 4, 4, 2, 2, 0, 0)) return(0);

	if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
	{
		errno = 0;
		plog(format("Connection not ready for account info (%d.%d.%d)",
			ind, connp->state, connp->id));
		return 0;
	}

	l_acc = GetAccount(connp->nick, NULL, FALSE);
	if (l_acc)
	{
		acc_flags = l_acc->flags;
		KILL(l_acc, struct account);
	}

	return Packet_printf(&connp->c, "%c%hd", PKT_ACCOUNT_INFO, acc_flags);
}

int Send_request_key(int ind, int id, char *prompt) {
	connection_t *connp = Conn[Players[ind]->conn];

	if (!is_newer_than(&connp->version, 4, 4, 6, 1, 0, 0)) return(0);
	if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
		errno = 0;
		plog(format("Connection not ready for request_key (%d.%d.%d)",
		    ind, connp->state, connp->id));
		return 0;
	}

	Players[ind]->request_id = id;
	Players[ind]->request_type = RTYPE_KEY;
	return Packet_printf(&connp->c, "%c%d%s", PKT_REQUEST_KEY, id, prompt);
}
int Send_request_num(int ind, int id, char *prompt, int std) {
	connection_t *connp = Conn[Players[ind]->conn];

	if (!is_newer_than(&connp->version, 4, 4, 6, 1, 0, 0)) return(0);
	if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
		errno = 0;
		plog(format("Connection not ready for request_num (%d.%d.%d)",
		    ind, connp->state, connp->id));
		return 0;
	}

	Players[ind]->request_id = id;
	Players[ind]->request_type = RTYPE_NUM;
	return Packet_printf(&connp->c, "%c%d%s%d", PKT_REQUEST_NUM, id, prompt, std);
}
int Send_request_str(int ind, int id, char *prompt, char *std) {
	connection_t *connp = Conn[Players[ind]->conn];

	if (!is_newer_than(&connp->version, 4, 4, 6, 1, 0, 0)) return(0);
	if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
		errno = 0;
		plog(format("Connection not ready for request_str (%d.%d.%d)",
		    ind, connp->state, connp->id));
		return 0;
	}

	Players[ind]->request_id = id;
	Players[ind]->request_type = RTYPE_STR;
	return Packet_printf(&connp->c, "%c%d%s%s", PKT_REQUEST_STR, id, prompt, std);
}
int Send_request_cfr(int ind, int id, char *prompt) {
	connection_t *connp = Conn[Players[ind]->conn];

	if (!is_newer_than(&connp->version, 4, 4, 6, 1, 0, 0)) return(0);
	if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
		errno = 0;
		plog(format("Connection not ready for request_cfr (%d.%d.%d)",
		    ind, connp->state, connp->id));
		return 0;
	}

	Players[ind]->request_id = id;
	Players[ind]->request_type = RTYPE_CFR;
	return Packet_printf(&connp->c, "%c%d%s", PKT_REQUEST_CFR, id, prompt);
}
/* NOTE: Should be followed by a p_ptr->request_id = RID_NONE to clean up. */
int Send_request_abort(int ind) {
	connection_t *connp = Conn[Players[ind]->conn];

	if (!is_newer_than(&connp->version, 4, 4, 6, 1, 0, 0)) return(0);
	if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
		errno = 0;
		plog(format("Connection not ready for request_abort (%d.%d.%d)",
		    ind, connp->state, connp->id));
		return 0;
	}

	return Packet_printf(&connp->c, "%c", PKT_REQUEST_ABORT);
}

/*
 * Return codes for the "Receive_XXX" functions are as follows:
 *
 * -1 --> Some error occured
 *  0 --> The action was queued (not enough energy)
 *  1 --> The action was ignored (not enough energy)
 *  2 --> The action completed successfully
 *  3 --> The action has been queued and is blocking further actions that need to be queued
 *
 *  Every code except for 1 will cause the input handler to stop
 *  processing actions.
 */

// This does absolutly nothing other than keep our connection active.
static int Receive_keepalive(int ind)
{
	int n, Ind;
	connection_t *connp = Conn[ind];
	char ch;
	player_type *p_ptr;

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* If client has not been hacked, this should set AFK after 1 minute
	   of no activity. */

	connp->inactive_keepalive++;

	if(connp->id != -1) {
		Ind = GetInd[connp->id];
		p_ptr = Players[Ind];

		p_ptr->idle += 2;
		p_ptr->idle_char += 2;

		/* Kick a starving player */
		if (p_ptr->food < PY_FOOD_WEAK && connp->inactive_keepalive > STARVE_KICK_TIMER / 2)
		{
			Destroy_connection(ind, "starving auto-kick");
			return 2;
		}

		else if (!p_ptr->afk && p_ptr->auto_afk && connp->inactive_keepalive > AUTO_AFK_TIMER / 2)	/* dont oscillate ;) */
		{
			/* auto AFK timer (>1 min) */
//			if (!p_ptr->resting) toggle_afk(Ind, ""); /* resting can take quite long sometimes */
			toggle_afk(Ind, "");
		}
	}

	return 2;
}

static int Receive_walk(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch, dir;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_MOV);
		p_ptr = Players[player];
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check */
	if (bad_dir(dir)) return 1;


	/* bugged here if !p_ptr */

	/* Disturb if running or resting */
	if (p_ptr->running || p_ptr->resting) {
		disturb(player, 0, 0);
#if 0 /* disabled, because this would prevent 'walking/running out of fire-till-kill/auto-ret' which is a bit annoying: \
         it'd actually first just do disturb() here, so the player would have to attempt a second time to run/walk, after that. - C. Blue */
		return 1;
#endif
	}

	if (p_ptr->command_rep) p_ptr->command_rep =- 1;

	if (player && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		if (p_ptr->warning_run == 0 && p_ptr->max_plv <= 5) {
			p_ptr->warning_run_steps++;
			/* Give a warning after first 10 walked steps, then every 50 walked steps. */
			if (p_ptr->warning_run_steps == 60) p_ptr->warning_run_steps = 10;
			if (p_ptr->warning_run_steps == 10) {
				msg_print(player, "\374\377oHINT: You can run swiftly by holding the SHIFT key when pressing a direction!");
				msg_print(player, "\374\377o      To use this, the NUMLOCK key (labelled 'Num') must be turned off,");
				msg_print(player, "\374\377o      and no awake monster must be in your line-of-sight (except in Bree).");
				s_printf("warning_run_steps: %s\n", p_ptr->name);
			}
		}
		do_cmd_walk(player, dir, p_ptr->always_pickup);
		return 2;
	} else {
		// Otherwise discared the walk request.
		//if (!connp->q.len && p_ptr->autoattack)
		// If we have no commands queued, then queue our walk request.
		// Note that ch might equal PKT_RUN, since Receive_run will
		// sometimes call this function.
		if (connp->q.len < 2) {
			Packet_printf(&connp->q, "%c%c", PKT_WALK, dir);
			return 0;
		} else {
			// If we have a walk command queued at the end of the queue,
			// then replace it with this queue request.
			if (connp->q.buf[connp->q.len - 2] == PKT_WALK) {
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
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	int i, n, player = -1;
	char dir;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_MOV);
		p_ptr = Players[player];
	}

	/* paranoia? */
//	if (player == -1) return;

	if (p_ptr->command_rep) p_ptr->command_rep =- 1;

	/* If not the dungeon master, who can always run */
	if (!p_ptr->admin_dm) {
		/* check for status impairments (lack of light is checked in run_test()) */
		if (p_ptr->confused || p_ptr->blind)
			return Receive_walk(ind);

		/* Check for monsters in sight */
		for (i = 0; i < m_max; i++) {
			/* Check this monster */
			if (((p_ptr->mon_los[i] && !m_list[i].csleep &&
			    m_list[i].level && !m_list[i].special) &&
			    /* Not in Bree (for Santa Claus) - C. Blue (Note: This and below messes should get resolved in future) */
			    (p_ptr->wpos.wx != cfg.town_x || p_ptr->wpos.wy != cfg.town_y || p_ptr->wpos.wz))) {
				// Treat this as a walk request
				// Hack -- send the same connp->r "arguments" to Receive_walk
				if (p_ptr->warning_run_monlos == 0) {
					p_ptr->warning_run_monlos = 1;
					if (p_ptr->max_plv <= 3) {
						msg_print(player, "\374\377yNote: You cannot initiate running while you are within line-of-sight");
						msg_print(player, "\374\377y      of an awake monster. The town of Bree is excepted from this.");
						s_printf("warning_run_monlos: %s\n", p_ptr->name);
					}
				}
				return Receive_walk(ind);
			}
		}

#ifdef HOSTILITY_ABORTS_RUNNING
		/* Check for hostile players. They should be treated as a disturbance.
		 * Should lessen the unfair advantage melee have in PVP */
		for (i = 1; i < NumPlayers; i++) {
			if (i == player) continue;
			if (check_hostile(player, i)) {
				if (target_able(player, 0-i)) { /* target_able takes in midx usually */
					return Receive_walk(ind);
				}
			}
		}
#endif
	}

#if 0 /* with new fire-till-kill/auto-ret code that accepts <= 100% energy, and accordingly the new \
    p_ptr->requires_energy flag, this stuff should no longer be required and hence obsolete. - C. Blue */
	/* hack to fix 'movelock' bug, which occurs if a player tries to RUN away from a
	   monster while he's currently auto-retaliating. (WALKING away instead of
	   trying to run works by the way.)
	   It doesn't matter if auto-retaliation is done by melee weaponry, shooting, or by spellcasting.
	   The bug results in the player being unable to move until he clears the buffer with ')' key or
	   (even while attacking) inscribes his item to stop auto-retaliating.
	   So this fix checks for adjacent monsters and, if found, changes the run command to a walk
	   command.
	   Note that the bug ONLY happens if the player is auto-retaliating. Manual attacks are fine.
	   The fix also makes kind of sense, since players can't run anyways while seeing a monster that's
	   awake. And any monster that's auto-retaliated (if not a 'pseudo monster' which rensembles an
	   inanimate object) should definitely be awake.
	   Better solutions (that also fix the 'double energy' players have after performing no action
	   for a turn and NEED for running) might take its place in the future. - C. Blue */
//if (p_ptr->auto_retaliating) s_printf("auto-retal\n");
//else s_printf("not a-r\n");
//	if (!p_ptr->admin_dm && p_ptr->auto_retaliating) {
	if (p_ptr->auto_retaliating || p_ptr->shooting_till_kill)
		return Receive_walk(ind);
#endif


	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0) {
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check */
	if (bad_dir(dir)) return 1;

	/* Disturb if we want to change directions */
	//if (dir != p_ptr->find_current) disturb(player, 0, 0);

	/* Hack -- Fix the running in '5' bug */
	if (dir == 5) return 1;

#if 0
	/* Only process the run command if we are not already running in
	 * the direction requested.
	 */
	if (player && (!p_ptr->running || (dir != p_ptr->find_current)))
#endif
	{
		// If we don't want to queue the command, return now.
		if ((n = do_cmd_run(player, dir)) == 2) {
			return 2;
		}
		// If do_cmd_run returns a 0, then there wasn't enough energy
		// to execute the run command.  Queue the run command if desired.
		else if (n == 0) {
			// Only buffer a run request if we have no previous commands
			// buffered, and it is a new direction or we aren't already
			// running.
#if 0 /* changed so we can 'run' out of fire-till-kill'ing, even if we got there while we were already running - C. Blue */
			if (((!connp->q.len) && (dir != p_ptr->find_current)) || (!p_ptr->running)) {
#else
			if (!connp->q.len) {
#endif
				Packet_printf(&connp->q, "%c%c", ch, dir);
				return 0;
			}
		}
	}

	return 1;
}

static int Receive_tunnel(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr=NULL;
	char ch, dir;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_MOV);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check */
	if (bad_dir(dir)) return 1;

	/* all this is temp just to make it work */
	if (p_ptr->command_rep == -1) {
		p_ptr->command_rep = 0;
		return(0);
	}

	/* please redesign ALL of this out of higher level */
	if (p_ptr && p_ptr->command_rep != PKT_TUNNEL) {
		p_ptr->command_rep = -1;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		do_cmd_tunnel(player, dir, FALSE);
		if (p_ptr->command_rep) Packet_printf(&connp->q, "%c%c", ch, dir);

		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c%c", ch, dir);
		return 0;
	}


	return 1;
}

static int Receive_aim_wand(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch, dir;
	s16b item;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd%c", &ch, &item, &dir)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		/* Sanity check - mikaelh */
		if (item >= INVEN_TOTAL) return 1;
		if (bad_dir1(player, &dir)) return 1;

		item = replay_inven_changes(player, item);
		if (item == 0xFF) {
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}

		do_cmd_aim_wand(player, item, dir);
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c%hd%c", ch, item, dir);
		return 0;
	}

	return 1;
}

static int Receive_drop(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	int n, player = -1; 
	s16b item, amt;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd%hd", &ch, &item, &amt)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check - mikaelh */
	if (item >= INVEN_TOTAL)
		return 1;

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		item = replay_inven_changes(player, item);
		if (item == 0xFF)
		{
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}

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
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch, dir = 5;
	int n, player = -1;
	//s16b item;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

//	if ((n = Packet_scanf(&connp->r, "%c%c%hd", &ch, &dir, &item)) <= 0)
	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0) {
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Check confusion */
	if (p_ptr->confused) {
		/* Change firing direction */
		/* no targetted shooting anymore while confused! */
		dir = randint(8);
		if (dir >= 5) dir++;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos) / p_ptr->num_fire) {
		/* Sanity check */
		if (bad_dir1(player, &dir)) return 1;

		if (p_ptr->shoot_till_kill && dir == 5) p_ptr->shooty_till_kill = TRUE;
//		do_cmd_fire(player, dir, item);
		do_cmd_fire(player, dir);
		if (!(p_ptr->shoot_till_kill && dir == 5 && !p_ptr->shooting_till_kill)) {
			if (p_ptr->ranged_double) do_cmd_fire(player, dir);
		}
		p_ptr->shooty_till_kill = FALSE;
		return 2;
	} else if (player) {
//		Packet_printf(&connp->q, "%c%c%hd", ch, dir, item);
		Packet_printf(&connp->q, "%c%c", ch, dir);
		return 0;
	}

	return 1;
}

static int Receive_observe(int ind)
{
	connection_t *connp = Conn[ind];
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

	/* Sanity check - mikaelh */
	if (item >= INVEN_TOTAL)
		return 1;

	if (connp->id != -1)
		do_cmd_observe(player, item);

	return 1;
}

static int Receive_stand(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = Players[ind];
	char ch;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_MOV);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

//	if (connp->id != -1) { 		/* disallow picking up items while paralyzed: */
	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		do_cmd_stay(player, 1);
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c", ch);
		return 0;
	}

	return -1;
}

static int Receive_destroy(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	s16b item, amt;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd%hd", &ch, &item, &amt)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check - mikaelh */
	if (item >= INVEN_TOTAL)
		return 1;

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		item = replay_inven_changes(player, item);
		if (item == 0xFF) {
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}

		do_cmd_destroy(player, item, amt);
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c%hd%hd", ch, item, amt);
		return 0;
	}

	return 1;
}

static int Receive_look(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch, dir;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check */
	if (bad_dir(dir)) return 1;

	if (connp->id != -1) {
		do_cmd_look(player, dir);
	}

	return 1;
}

/*
 * Possibly, most of Receive_* functions can be bandled into one function
 * like this; that'll make the client *MUCH* more generic.		- Jir -
 */
static int Receive_activate_skill(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch, mkey, dir;
	int n, player = -1, old = -1;
	s16b book, spell, item, aux;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		old = player;
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%c%hd%hd%c%hd%hd", &ch, &mkey, &book, &spell, &dir, &item, &aux)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	/* Not by class nor by item; by skill */
	if (connp->id != -1 &&
	    (p_ptr->energy >= level_speed(&p_ptr->wpos) ||
	    /* some abilities don't require energy: */
	    mkey == MKEY_DODGE || mkey == MKEY_PARRYBLOCK ||
	    mkey == MKEY_SHOOT_TILL_KILL || mkey == MKEY_DUAL_MODE)) {
		/* Sanity check - mikaelh */
		if (item >= INVEN_TOTAL) return 1;
		if (bad_dir3(player, &dir)) return 1;


		p_ptr->current_char = (old == player) ? TRUE : FALSE;

		if (p_ptr->ghost && !is_admin(p_ptr)) {
			msg_print(player, "\377oYou need your body to use a skill.");
			return 2;
		}

#if 0
		/* Break goi/manashield */
		if (mkey != MKEY_DODGE) { // it's not real 'activation'
			if (p_ptr->invuln) set_invuln(player, 0);
			if (p_ptr->tim_manashield) set_tim_manashield(player, 0);
		}
#endif

		switch (mkey) {
		case MKEY_MIMICRY:
			if (get_skill(p_ptr, SKILL_MIMIC)) {
				do_cmd_mimic(player, spell, dir);
			}
			break;

		case MKEY_DODGE:
			use_ability_blade(player);
			break;
		case MKEY_FLETCHERY:
			do_cmd_fletchery(player);
			break;
#ifdef ENABLE_STANCES
		case MKEY_STANCE:
			do_cmd_stance(player, book);
			break;
#endif
#ifdef ENABLE_NEW_MELEE
		case MKEY_PARRYBLOCK:
			check_parryblock(player);
			break;
#endif
		case MKEY_TRAP:
			do_cmd_set_trap(player, book, spell);
			break;
		case MKEY_SCHOOL:
			book = replay_inven_changes(player, book);
			if (book == 0xFF) {
				msg_print(player, "Command failed because item is gone.");
				return 1;
			}
			item = replay_inven_changes(player, item);
			if (item == 0xFF) {
				msg_print(player, "Command failed because item is gone.");
				return 1;
			}

			/* Sanity check #2 */
			if (dir == -1) dir = 5;

			if (p_ptr->shoot_till_kill && dir == 5) p_ptr->shooty_till_kill = TRUE;
			cast_school_spell(player, book, spell, dir, item, aux);
			p_ptr->shooty_till_kill = FALSE;
			break;

#ifndef ENABLE_RCRAFT
		case MKEY_RUNE:
			book = replay_inven_changes(player, book);
			if (book == 0xFF) {
				msg_print(player, "Command failed because item is gone.");
				return 1;
			}
			item = replay_inven_changes(player, item);
			if (item == 0xFF) {
				msg_print(player, "Command failed because item is gone.");
				return 1;
			}

			cast_rune_spell_header(player, book, spell); 
			break;
#else
		case MKEY_RCRAFT:
			book = replay_inven_changes(player, book);
			if (book == 0xFF) {
				msg_print(player, "Command failed because item is gone.");
				return 1;
			}
			item = replay_inven_changes(player, item);
			if (item == 0xFF) {
				msg_print(player, "Command failed because item is gone.");
				return 1;
			}

			if (p_ptr->shoot_till_kill && dir == 5) p_ptr->shooty_till_kill = TRUE;
			execute_rspell(player, dir, (u32b)((book * 10000) + spell), item, 0);
			p_ptr->shooty_till_kill = FALSE;
			break;
#endif

		case MKEY_AURA_FEAR:
			toggle_aura(player, 0);
			break;
		case MKEY_AURA_SHIVER:
			toggle_aura(player, 1);
			break;
		case MKEY_AURA_DEATH:
			toggle_aura(player, 2);
			break;

		case MKEY_MELEE:
			do_cmd_melee_technique(player, spell);
			break;
		case MKEY_RANGED:
			do_cmd_ranged_technique(player, spell);
			break;
		case MKEY_SHOOT_TILL_KILL:
			toggle_shoot_till_kill(player);
			break;
#ifdef DUAL_WIELD
		case MKEY_DUAL_MODE:
			toggle_dual_mode(player);
			break;
#endif
		}
		return 2;
	} else if (player) {
		p_ptr->current_spell = -1;
		p_ptr->current_mind = -1;
		Packet_printf(&connp->q, "%c%c%hd%hd%c%hd%hd", ch, mkey, book, spell, dir, item, aux);
		return 0;
	}


	return 1;
}

static int Receive_open(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr=NULL;
	char ch, dir;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check */
	if (bad_dir(dir)) return 1;

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
	connection_t *connp = Conn[ind];
	char ch;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
	}

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1)
	{
		change_mind(player, TRUE);
		return 2;
	}
	return 1;
}

static int Receive_ghost(int ind)
{
	connection_t *connp = Conn[ind];
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
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	s16b item;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check - mikaelh */
	if (item >= INVEN_TOTAL)
		return 1;

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		item = replay_inven_changes(player, item);
		if (item == 0xFF)
		{
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}

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
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	s16b item;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check - mikaelh */
	if (item >= INVEN_TOTAL)
		return 1;

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		item = replay_inven_changes(player, item);
		if (item == 0xFF)
		{
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}

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
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_MOV);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0) {
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* all this is temp just to make it work */
	if (p_ptr->command_rep == -1) {
		p_ptr->command_rep = 0;
		return(0);
	}
	if (p_ptr && p_ptr->command_rep != PKT_SEARCH) {
		p_ptr->command_rep = -1;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		do_cmd_search(player);
		if (p_ptr->command_rep) Packet_printf(&connp->q, "%c", ch);
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c", ch);
		return 0;
	}

	return 1;
}

static int Receive_take_off(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	s16b item;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check - mikaelh */
	if (item >= INVEN_TOTAL)
		return 1;

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		item = replay_inven_changes(player, item);
		if (item == 0xFF)
		{
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}

		do_cmd_takeoff(player, item, 255);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%hd", ch, item);
		return 0;
	}

	return 1;
}

static int Receive_take_off_amt(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	s16b item, amt;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd%hd", &ch, &item, &amt)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check - mikaelh */
	if (item >= INVEN_TOTAL)
		return 1;

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		item = replay_inven_changes(player, item);
		if (item == 0xFF)
		{
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}

		do_cmd_takeoff(player, item, amt);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%hd%hd", ch, item, amt);
		return 0;
	}

	return 1;
}

static int Receive_use(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	s16b item;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check - mikaelh */
	if (item >= INVEN_TOTAL)
		return 1;

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		item = replay_inven_changes(player, item);
		if (item == 0xFF)
		{
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}

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
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch, dir;
	int n, player = -1;
	s16b item;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%c%hd", &ch, &dir, &item)) <= 0) {
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		/* Sanity check - mikaelh */
		if (item >= INVEN_TOTAL) return 1;
		if (bad_dir1(player, &dir)) return 1;

		item = replay_inven_changes(player, item);
		if (item == 0xFF) {
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}

		do_cmd_throw(player, dir, item, 0);
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c%c%hd", ch, dir, item);
		return 0;
	}

	return 1;
}

static int Receive_wield(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	s16b item;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check - mikaelh */
	if (item >= INVEN_TOTAL) return 1;

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		item = replay_inven_changes(player, item);
		if (item == 0xFF)
		{
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}

		do_cmd_wield(player, item, 0x0);
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
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	s16b item;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check - mikaelh */
	if (item >= INVEN_TOTAL)
		return 1;

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		item = replay_inven_changes(player, item);
		if (item == 0xFF)
		{
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}

		do_cmd_zap_rod(player, item, 0);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%hd", ch, item);
		return 0;
	}

	return 1;
}

static int Receive_zap_dir(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch, dir;
	s16b item;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd%c", &ch, &item, &dir)) <= 0) {
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		/* Sanity check - mikaelh */
		if (item >= INVEN_TOTAL) return 1;
		if (bad_dir1(player, &dir)) return 1;

		item = replay_inven_changes(player, item);
		if (item == 0xFF) {
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}

		do_cmd_zap_rod(player, item, dir);
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c%hd%c", ch, item, dir);
		return 0;
	}

	return 1;
}

static int Receive_target(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	s16b dir;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check */
	if (bad_dir(dir) && bad_dir2(dir)) return 1;

	if (connp->id != -1)
		do_cmd_target(player, dir);

	return 1;
}

static int Receive_target_friendly(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;

	char ch;

	s16b dir;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check */
	if (bad_dir(dir) && bad_dir2(dir)) return 1;

	if (connp->id != -1)
		do_cmd_target_friendly(player, dir);

	return 1;
}


static int Receive_inscribe(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	int n, player = -1;
	s16b item;
	char inscription[MAX_CHARS];

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd%s", &ch, &item, inscription)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}
	/* paranoia? */
	inscription[MAX_CHARS - 1] = '\0';

	/* Sanity check - mikaelh */
	if (item >= INVEN_TOTAL)
		return 1;

	if (connp->id != -1)
	{
		item = replay_inven_changes(player, item);
		if (item == 0xFF)
		{
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}

		do_cmd_inscribe(player, item, inscription);
	}

	return 1;
}

static int Receive_uninscribe(int ind)
{
	connection_t *connp = Conn[ind];
	char ch;
	int n, player = -1;
	s16b item;
	player_type *p_ptr = NULL;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check - mikaelh */
	if (item >= INVEN_TOTAL)
		return 1;

	if (connp->id != -1)
	{
		item = replay_inven_changes(player, item);
		if (item == 0xFF)
		{
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}

		do_cmd_uninscribe(player, item);
	}

	return 1;
}

static int Receive_activate(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	s16b item;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check - mikaelh */
	if (item >= INVEN_TOTAL)
		return 1;

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		item = replay_inven_changes(player, item);
		if (item == 0xFF)
		{
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}

		do_cmd_activate(player, item, 0);
		return 2;
	}
	else if (player)
	{
		Packet_printf(&connp->q, "%c%hd", ch, item);
		return 0;
	}

	return 1;
}

static int Receive_activate_dir(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch, dir;
	s16b item;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd%c", &ch, &item, &dir)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		/* Sanity check - mikaelh */
		if (item >= INVEN_TOTAL) return 1;
		if (bad_dir1(player, &dir)) return 1;

		item = replay_inven_changes(player, item);
		if (item == 0xFF) {
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}

		do_cmd_activate(player, item, dir);
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c%hd%c", ch, item, dir);
		return 0;
	}

	return 1;
}

static int Receive_bash(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr=NULL;
	char ch, dir;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_MOV);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check */
	if (bad_dir(dir)) return 1;

	/* all this is temp just to make it work */
	if (p_ptr->command_rep == -1) {
		p_ptr->command_rep = 0;
		return(0);
	}

	if (p_ptr && p_ptr->command_rep != PKT_BASH) {
		p_ptr->command_rep = -1;
	}
	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		do_cmd_bash(player, dir);
		if (p_ptr->command_rep) Packet_printf(&connp->q, "%c%c", ch, dir);
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c%c", ch, dir);
		return 0;
	}

	return 1;
}

static int Receive_disarm(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr=NULL;
	char ch, dir;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_MOV);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check */
	if (bad_dir(dir)) return 1;

	/* all this is temp just to make it work */
	if (p_ptr->command_rep == -1) {
		p_ptr->command_rep = 0;
		return(0);
	}
	if (p_ptr && p_ptr->command_rep != PKT_DISARM) p_ptr->command_rep =- 1;

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		do_cmd_disarm(player, dir);
		if (p_ptr->command_rep) Packet_printf(&connp->q, "%c%c", ch, dir);
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c%c", ch, dir);
		return 0;
	}
	return 1;
}

static int Receive_eat(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	s16b item;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check - mikaelh */
	if (item >= INVEN_TOTAL)
		return 1;

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		item = replay_inven_changes(player, item);
		if (item == 0xFF) {
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}

		do_cmd_eat_food(player, item);
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c%hd", ch, item);
		return 0;
	}
	return 1;
}

static int Receive_fill(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	s16b item;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check - mikaelh */
	if (item >= INVEN_TOTAL)
		return 1;

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		item = replay_inven_changes(player, item);
		if (item == 0xFF) {
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}
		do_cmd_refill(player, item);
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c%hd", ch, item);
		return 0;
	}
	return 1;
}

static int Receive_locate(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch, dir;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_MOV);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check */
	if (bad_dir(dir)) return 1;

	if (connp->id != -1) do_cmd_locate(player, dir);

	return 1;
}

static int Receive_map(int ind)
{
	connection_t *connp = Conn[ind];
	char ch, mode;
	int n, player = -1;
	player_type *p_ptr = NULL;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &mode)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1) do_cmd_view_map(player, mode);
	return 1;
}

static int Receive_search_mode(int ind)
{
	connection_t *connp = Conn[ind];
	char ch;
	int n, player = -1;
	player_type *p_ptr = NULL;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1) do_cmd_toggle_search(player);
	return 1;
}

static int Receive_close(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch, dir;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check */
	if (bad_dir(dir)) return 1;

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		do_cmd_close(player, dir);
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c%c", ch, dir);
		return 0;
	}
	return 1;
}

static int Receive_skill_mod(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	s32b i;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%d", &ch, &i)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1) increase_skill(player, i, FALSE);
	return 1;
}

static int Receive_skill_dev(int ind) {
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	s32b i;
	bool dev;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%d%c", &ch, &i, &dev)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1) {
		if (i == -1) {
			/* do it for whole skill tree? */
			for (i = 0; i < MAX_SKILLS; i++) {
				p_ptr->s_info[i].dev = (dev == 0 ? FALSE : TRUE);
			}
		} else {
			/* only do it for one skill */
			p_ptr->s_info[i].dev = (dev == 0 ? FALSE : TRUE);
		}
	}
	return 1;
}

static int Receive_go_up(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		do_cmd_go_up(player);
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c", ch);
		return 0;
	}
	return 1;
}

static int Receive_go_down(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		do_cmd_go_down(player);
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c", ch);
		return 0;
	}
	return 1;
}


static int Receive_direction(int ind)
{
	connection_t *connp = Conn[ind];
	char ch, dir;
	int n, player = -1;
	player_type *p_ptr = NULL;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check */
	if (bad_dir1(player, &dir)) return 1;

	if (connp->id != -1) Handle_direction(player, dir);

	return 1;
}

static int Receive_item(int ind)
{
	connection_t *connp = Conn[ind];
	char ch;
	s16b item;
	int n, player = -1;
	player_type *p_ptr = NULL;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check - mikaelh */
	if (item >= INVEN_TOTAL)
		return 1;

	if (connp->id != -1) {
		item = replay_inven_changes(player, item);
		if (item == 0xFF) {
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}
		Handle_item(player, item);
	}
	return 1;
}


static int Receive_message(int ind)
{
	connection_t *connp = Conn[ind];
	char ch, buf[MSG_LEN];
	int n, player = -1;
	if (connp->id != -1) player = GetInd[connp->id];
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%S", &ch, buf)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	/* A message longer than 159 characters will certainly cause problems - mikaelh
	   (Changed it to MSG_LEN - C. Blue) */
	buf[MSG_LEN - 1] = '\0';
	player_talk(player, buf);
	return 1;
}

static int Receive_admin_house(int ind){
	connection_t *connp = Conn[ind];
	char ch, dir;
	int n,player = -1;
	char buf[MAX_CHARS];
	player_type *p_ptr = NULL;

	if (connp->id != -1) {
		player = GetInd[connp->id];
//		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%hd%s", &ch, &dir, buf)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check */
	if (bad_dir(dir)) return 1;

	house_admin(player, dir, buf);
	return(1);
}

static int Receive_purchase(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;

	char ch;
	int n, player = -1;
	s16b item, amt;

	if (connp->id != -1) {
		player = GetInd[connp->id];
//		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%hd%hd", &ch, &item, &amt)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (player && p_ptr->store_num != -1)
		store_purchase(player, item, amt);
	else if (player)
		do_cmd_purchase_house(player, item);

	return 1;
}

static int Receive_sell(int ind)
{
	connection_t *connp = Conn[ind];

	char ch;
	int n, player = -1;
	s16b item, amt;
	player_type *p_ptr = NULL;

	if (connp->id != -1) {
		player = GetInd[connp->id];
//		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%hd%hd", &ch, &item, &amt)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (player) store_sell(player, item, amt);

	return 1;
}

static int Receive_store_leave(int ind)
{
	connection_t *connp = Conn[ind];
#ifdef MINDLINK_STORE
	connection_t *connp2;
#endif
	player_type *p_ptr = NULL;

	char ch;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
//		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
#ifdef MINDLINK_STORE
		if (get_esp_link(GetInd[connp->id], LINKF_OBJ, &p_ptr)) {
		        connp2 = Conn[p_ptr->conn];
		        Packet_printf(&connp2->c, "%c", PKT_STORE_LEAVE); /* not working */
		}
#endif
	} else player = 0;

	if (player) p_ptr = Players[player];
	
	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (!player) return -1;

        /* Update stuff */
        p_ptr->update |= (PU_VIEW | PU_LITE);
        p_ptr->update |= (PU_MONSTERS);

        /* Redraw stuff */
        p_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA);

        /* Window stuff */
        p_ptr->window |= (PW_OVERHEAD);

	/* Update store info */
	if (p_ptr->store_num != -1) {
#ifdef PLAYER_STORES
		/* Player stores aren't entered such as normal stores,
		   instead, the customer just stays in front of it. */
		if (p_ptr->store_num <= -2) {
			/* unlock the fake store again which we had occupied */
			fake_store_visited[-2 - p_ptr->store_num] = 0;
		} else
#endif
		/* Hack -- don't stand in the way */
		teleport_player_force(player, 1);

		handle_store_leave(player);
	}

#if 0
 #if 0
	/* hack -- update night/day in wilderness levels */
	/* XXX it's not so good place to do such things -
	 * prolly we'll need PU_SUN or sth.		- Jir - */
	if (!p_ptr->wpos.wz) {
		if (IS_DAY) world_surface_day(&p_ptr->wpos); 
		else world_surface_night(&p_ptr->wpos);
	}
 #else
	if (!p_ptr->wpos.wz) {
		if (IS_DAY) player_day(player);
		else player_night(player);
	}
 #endif
#endif

	return 1;
}

static int Receive_store_confirm(int ind)
{
	connection_t *connp = Conn[ind];
	char ch;
	int n, player = -1;
	player_type *p_ptr = NULL;

	if (connp->id != -1) {
		player = GetInd[connp->id];
//		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (!player) return -1;

	store_confirm(player);

	return 1;
}

/* Store code should be written to allow more kinds of actions in general..
 */
static int Receive_store_examine(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;

	char ch;
	int n, player = -1;
	s16b item;

	if (connp->id != -1) {
		player = GetInd[connp->id];
//		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (player && p_ptr->store_num != -1)
		store_examine(player, item);

	return 1;
}

static int Receive_store_command(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;

	char ch;
	int n, player = 0, gold;
	u16b action;
	s16b item, item2, amt;

	if (connp->id != -1) {
		player = GetInd[connp->id];
//		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%hd%hd%hd%hd%d", &ch, &action, &item, &item2, &amt, &gold)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (player && p_ptr->store_num != -1)
		store_exec_command(player, action, item, item2, amt, gold);

	return 1;
}

static int Receive_drop_gold(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	int n, player = -1;
	s32b amt;

	if (connp->id != -1) {
		player = GetInd[connp->id];
//		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%d", &ch, &amt)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		do_cmd_drop_gold(player, amt);
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c%d", ch, amt);
		return 0;
	}
	return 1;
}

static int Receive_steal(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch, dir;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
//		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check */
	if (bad_dir(dir)) return 1;

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		do_cmd_steal(player, dir);
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c%c", ch, dir);
		return 0;
	}
	return 1;
}


static int Receive_King(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;

	byte type;
	char ch;

	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
//		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &type)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1) {
		switch(type) {
#if 0 /* DGDGDGDG -- Cause fucking up of levels */
			case KING_OWN:
				do_cmd_own(player);
				break;
#endif
		}
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c%c", ch, type);
		return 0;
	}
	return 1;
}
	
static int Receive_redraw(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	int player = -1, n;
	char ch, mode;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		p_ptr = Players[player];

		if (p_ptr->esp_link_type && p_ptr->esp_link) {
			int Ind2 = find_player(p_ptr->esp_link);

			if (!Ind2)
				end_mind(ind, TRUE);
			else {
				if (Players[Ind2]->esp_link_flags & LINKF_VIEW) {
					player = Ind2;
					p_ptr = Players[Ind2];
				}
			}
		}
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &mode)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (player) {
//		p_ptr->store_num = -1;
		p_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_MAP);
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
		p_ptr->update |= (PU_BONUS | PU_VIEW | PU_MANA | PU_HP | PU_SANITY);

		/* Do 'Heavy' redraw if requested.
		 * TODO: One might wish to add more detailed modes
		 */
		if (mode) {
			/* Tell the server to redraw the player's display */
			p_ptr->redraw |= PR_MAP | PR_EXTRA | PR_BASIC | PR_HISTORY | PR_VARIOUS | PR_STATE;
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
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	int player = -1, n;
	char ch;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_MOV);
		p_ptr = Players[player];
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (player) {
		cave_type **zcave;

		/* If we are already resting, cancel the rest. */
		/* Waking up takes no energy, although we will still be drowsy... */
		if (p_ptr->resting) {
			disturb(player, 0, 0);
			return 2;
		}

		/* Don't rest if we are poisoned or at max hit points and max spell points
		   and max stamina */
		if ((p_ptr->poisoned) || ((p_ptr->chp == p_ptr->mhp) &&
					(p_ptr->csp == p_ptr->msp) &&
					(p_ptr->cst == p_ptr->mst)
		    && !(p_ptr->prace == RACE_ENT && p_ptr->food < PY_FOOD_FULL)
		    ))
			return 2;

		if (!(zcave = getcave(&p_ptr->wpos))) return 2;

#if 0 /* why? don't see a reason atm */
		/* Can't rest on a Void Jumpgate -- too dangerous */
		if (zcave[p_ptr->py][p_ptr->px].feat == FEAT_BETWEEN) {
			msg_print(player, "Resting on a Void Jumpgate is too dangerous!");
			return 2;
		}
#endif

		/* Resting takes a lot of energy! */
		if ((p_ptr->energy) >= (level_speed(&p_ptr->wpos) * 2) - 1) {
			/* Set flag */
			p_ptr->resting = TRUE;
			p_ptr->warning_rest = 1;

			/* Make sure we aren't running */
			p_ptr->running = FALSE;
			break_shadow_running(player);
			stop_precision(player);
			stop_shooting_till_kill(player);

			/* Take a lot of energy to enter "rest mode" */
			p_ptr->energy -= (level_speed(&p_ptr->wpos) * 2) - 1;

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
	player_type *p_ptr = Players[Ind];
	connection_t *connp = Conn[p_ptr->conn];

	/* Clear the buffer */

	/* No energy commands are placed in 'q' */
	Sockbuf_clear(&connp->q);

	/* New commands are in 'r' - clear this and its gone. */
	/* Sockbuf_clear(&connp->r); */

	/* Clear 'current spell' */
	p_ptr->current_spell = -1;
}

static void Handle_clear_actions(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* Stop ranged auto-retaliation (fire-till-kill) */
	p_ptr->shooting_till_kill = FALSE;

	/* Stop automatically executed repeated actions */
	p_ptr->command_rep = 0;

	/* Stop resting */
	disturb(Ind, 0, 0);
}

static int Receive_clear_buffer(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	int player = -1, n;
	char ch;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
//		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0)
	{
		if (n == -1)	
			Destroy_connection(ind, "read error");
		return n;
	}

	if(player) Handle_clear_buffer(player);

	return 1;
}

static int Receive_clear_actions(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	int player = -1, n;
	char ch;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
//		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}
	else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if(player) Handle_clear_actions(player);

	return 1;
}

static int Receive_special_line(int ind)
{
	connection_t *connp = Conn[ind];
	int player = -1, n;
	char ch, type;
	s32b line;

	if (connp->id != -1) player = GetInd[connp->id];
		else player = 0;

	if (is_newer_than(&connp->version, 4, 4, 7, 0, 0, 0)) {
		if ((n = Packet_scanf(&connp->r, "%c%c%d", &ch, &type, &line)) <= 0) {
			if (n == -1)
				Destroy_connection(ind, "read error");
			return n;
		}
	} else {
		if ((n = Packet_scanf(&connp->r, "%c%c%hd", &ch, &type, &line)) <= 0) {
			if (n == -1)
				Destroy_connection(ind, "read error");
			return n;
		}
	}

	if (player) {
		char kludge[2] = "";
		kludge[0] = (char) line;
		kludge[1] = '\0';
		switch (type) {
		case SPECIAL_FILE_NONE:
			Players[player]->special_file_type = FALSE;
			/* Remove the file */
/*			if (!strcmp(Players[player]->infofile,
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
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	int player = -1, i, n;
	char ch;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		p_ptr = Players[player];
	} else {
		player = 0;
		p_ptr = NULL;
	}

	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (player) {
		bool options[OPT_MAX];
		for (i = 0; i < OPT_MAX; i++) {
			n = Packet_scanf(&connp->r, "%c", &options[i]);

			if (n <= 0) {
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
	connection_t *connp = Conn[ind];
	int player = -1, n;
	char ch;
	char extra1 = 1, extra2 = 4;

	if (connp->id != -1)
		player = GetInd[connp->id];
	else player = 0;

	/* Newer clients send couple of extra bytes to prevent accidental
	 * suicides due to network mishaps. - mikaelh */
	if (is_newer_than(&connp->version, 4, 4, 2, 3, 0, 0)) {
		if ((n = Packet_scanf(&connp->r, "%c%c%c", &ch, &extra1, &extra2)) <= 0)
		{
			if (n == -1)
				Destroy_connection(ind, "read error");
			return n;
		}
	}
	else
	{
		if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0)
		{
			if (n == -1)
				Destroy_connection(ind, "read error");
			return n;
		}
	}

	if (extra1 != 1 || extra2 != 4)
	{
		/* Invalid suicide command detected, clear the buffer now */
		Sockbuf_clear(&connp->r);
		return 1;
	}

	/* Commit suicide */
	do_cmd_suicide(player);

	return 1;
}

static int Receive_party(int ind)
{
	connection_t *connp = Conn[ind];
	int player = -1, n;
	char ch, buf[MAX_CHARS];
	s16b command;

	if (connp->id != -1) player = GetInd[connp->id];
		else player = 0;

	if ((n = Packet_scanf(&connp->r, "%c%hd%s", &ch, &command, buf)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}
	
	/* sanitize input - C. Blue */
	if (strlen(buf) > 40) *(buf + 40) = 0;
	for (n = 0; n < (int)strlen(buf); n++)
		if (*(buf + n) < 32) *(buf + n) = '_';

	if (player)
	{
		switch (command)
		{
			case PARTY_CREATE:
			{
				party_create(player, buf);
				break;
			}

			case PARTY_CREATE_IRONTEAM:
			{
				party_create_ironteam(player, buf);
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
				add_hostility(player, buf, TRUE);
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
	connection_t *connp = Conn[ind];
	int player = -1, n;
	char ch, buf[MAX_CHARS];
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
	player_type *p_ptr = Players[Ind], *p_ptr2 = NULL;
	int Ind2;
//s_printf("hd dir,current_spell,current_realm=%d,%d,%d\n", dir, p_ptr->current_spell, p_ptr->current_realm);

	/* New '+' feat in 4.4.6.2 */
	if (dir == 11) { /* repeat forever, if we keep pressing '+', yay.. */
		get_aim_dir(Ind);
		return;
	}

	if (!dir) {
		p_ptr->current_char = 0;
		p_ptr->current_spell = -1;
		p_ptr->current_mind = -1;
		p_ptr->current_rod = -1;
		p_ptr->current_activation = -1;
		p_ptr->current_rune_dir = -1;
		p_ptr->current_rune1 = -1;
		p_ptr->current_rune2 = -1;
#ifdef ENABLE_RCRAFT
		p_ptr->current_rcraft = -1;
#endif

		p_ptr->current_wand = -1;
		p_ptr->current_item = -1;
		p_ptr->current_book = -1;
		p_ptr->current_aux = -1;
		p_ptr->current_realm = -1;
		p_ptr->current_fire = -1;
		p_ptr->current_throw = -1;
		return;
	}

	Ind2 = get_esp_link(Ind, LINKF_MISC, &p_ptr2);

	if (p_ptr->current_spell != -1) {
//		if (p_ptr->current_realm == REALM_GHOST)
		if (p_ptr->ghost)
			do_cmd_ghost_power_aux(Ind, dir);
		else if (p_ptr->current_realm == REALM_MIMIC)
			do_mimic_power_aux(Ind, dir);
		else if (p_ptr->current_realm == REALM_SCHOOL)
			cast_school_spell(Ind, p_ptr->current_book, p_ptr->current_spell,
			    dir, p_ptr->current_item, p_ptr->current_aux);
		else p_ptr->current_spell = -1;
	}
#ifndef ENABLE_RCRAFT
	else if (p_ptr->current_rune1 != -1 && p_ptr->current_rune2 != -1)
		cast_rune_spell(Ind, dir);
#else
	else if (p_ptr->current_rcraft != -1)
		execute_rspell(Ind, dir, p_ptr->current_rcraft_flags, p_ptr->current_rcraft_imp, 0);
#endif
       	else if (p_ptr->current_rod != -1)
		do_cmd_zap_rod_dir(Ind, dir);
	else if (p_ptr->current_activation != -1)
		do_cmd_activate_dir(Ind, dir);
	else if (p_ptr->current_wand != -1)
		do_cmd_aim_wand(Ind, p_ptr->current_wand, dir);
	else if (p_ptr->current_fire != -1)
		do_cmd_fire(Ind, dir);
	else if (p_ptr->current_throw != -1)
		do_cmd_throw(Ind, dir, p_ptr->current_throw, 0);
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
	else if (p_ptr->current_tome_creation)
	{
		/* swap-hack: activating a custom tome uses up
		   the TARGET item, not the tome, of course */
		i = p_ptr->using_up_item;
		p_ptr->using_up_item = item;
		tome_creation_aux(Ind, i);
	}

	/* to be safe, clean up; just in case our item was used up */
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
	connection_t *connp = Conn[ind];
	int player = -1, n;
	char ch, buf[MAX_CHARS];
	s16b command;

	if (connp->id != -1) player = GetInd[connp->id];
		else player = 0;

	/* Make sure this came from the dungeon master.  Note that it may be
	 * possible to spoof this, so probably in the future more advanced
	 * authentication schemes will be neccecary. -APD
	 */

	/* Is this necessary here? Maybe (evileye) */
	if (!admin_p(player) &&
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
	connection_t *connp = Conn[ind];
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
//	int Ind2;
	player_type *p_ptr = Players[Ind];//, *p_ptr2;

#if 0 /* end_mind() is called by get_esp_link() ! -> infinite recursion in a rare case */
	if ((Ind2 = get_esp_link(Ind, LINKF_VIEW, &p_ptr2))) p_ptr2->update |= PU_MUSIC;
#endif
	if (p_ptr->esp_link_flags & LINKF_VIEW_DEDICATED) p_ptr->update |= PU_MUSIC;

	if (!(p_ptr->esp_link_flags & LINKF_HIDDEN)) {
		msg_print(Ind, "\377REnding mind link.");
	}
	p_ptr->esp_link = 0;
	p_ptr->esp_link_type = 0;
	p_ptr->esp_link_flags = 0;
	if (update) {
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
		p_ptr->update |= (PU_BONUS | PU_VIEW | PU_MANA | PU_HP);
		p_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_MAP);
	}
}

static int Receive_spike(int ind)
{
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;

	char ch, dir;

	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &dir)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check */
	if (bad_dir(dir)) return 1;

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
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;

	char ch, key;
	int n, player = -1;
	
	if (connp->id != -1)
	{
		player = GetInd[connp->id];
//		use_esp_link(&player, LINKF_OBJ); /* might be bad, depending on actual command */
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &key)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos))
	{
		if (p_ptr->store_num != -1)
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
#if 0 /* all in client 4.4.1 or 4.4.0d now! #if 0 this when released */
				/* Drink from a fountain (test passed:) */
				case '_':
					do_cmd_drink_fountain(player);
					break;
				/* Open/close mind to receive items via telekinesis */
				case 'p':
					/* But we can also use this for telekinesis! - C. Blue
					   (mostly to avoid PK exploits */
					if (p_ptr->esp_link_flags & LINKF_TELEKIN) {
						msg_print(player, "\377RYou stop concentrating on telekinesis.");
						p_ptr->esp_link_flags &= ~LINKF_TELEKIN;
					} else {
						msg_print(player, "\377RYou concentrate on telekinesis!");
						p_ptr->esp_link_flags |= LINKF_TELEKIN;
					}
					break;
				case '!':
					/* Look at in-game bbs - C. Blue */
					msg_print(player, "\377wBulletin board (type '/bbs <text>' in chat to write something) :");
					censor_message = TRUE;
					for (n = 0; n < BBS_LINES; n++)
						if (strcmp(bbs_line[n], "")) {
							censor_length = strlen(bbs_line[i]) + bbs_line[i] - strchr(bbs_line[i], ':') - 4;
							msg_format(player, "\377s %s", bbs_line[n]);
							bbs_empty = FALSE;
						}
					censor_message = FALSE;
					if (bbs_empty) msg_print(player, "\377s <nothing has been written on the board so far>");
					break;
//					return 1; /* consume no energy/don't disturb character (resting mode) */
#else
				case '_':
				case '!':
				case 'p':
					msg_print(player, "\377RYour client is outdated (probably version 4.4.0).");
					msg_print(player, "\377RPlease download latest client from www.c-blue.de/rogue");
					break;
#endif
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

/* Reply to ping packets - mikaelh */
static int Receive_ping(int ind) {
	connection_t *connp = Conn[ind];
	char ch, pong, buf[MSG_LEN];
	int n, id, tim, utim, Ind;
	player_type *p_ptr;

	if ((n = Packet_scanf(&connp->r, "%c%c%d%d%d%S", &ch, &pong, &id, &tim, &utim, &buf)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	if (!pong)
	{
		connp->inactive_ping++;

		if (connp->id != -1) {
			Ind = GetInd[connp->id];
			p_ptr = Players[Ind];

			p_ptr->idle++;
			p_ptr->idle_char++;

#if (MAX_PING_RECVS_LOGGED > 0)
			/* Get the exact time and save it */
			p_ptr->pings_received_head = (p_ptr->pings_received_head + 1) % MAX_PING_RECVS_LOGGED;
			gettimeofday(&p_ptr->pings_received[(int) p_ptr->pings_received_head], NULL);
#endif

			/* Kick a starving player */
			if (p_ptr->food < PY_FOOD_WEAK && connp->inactive_ping > STARVE_KICK_TIMER)
			{
				Destroy_connection(ind, "starving auto-kick");
				return 2;
			}

			else if (!p_ptr->afk && p_ptr->auto_afk && connp->inactive_ping > AUTO_AFK_TIMER)	/* dont oscillate ;) */
			{
				/* auto AFK timer (>1 min) */
//				if (!p_ptr->resting) toggle_afk(Ind, ""); /* resting can take quite long sometimes */
				toggle_afk(Ind, "");
			}
		}

		if (!BIT(connp->state, CONN_PLAYING | CONN_READY))
		{
			errno = 0;
			plog(format("Connection not ready for pong (%d.%d.%d)",
				ind, connp->state, connp->id));
			return 1;
		}

		pong = 1;

		Packet_printf(&connp->c, "%c%c%d%d%d%S", PKT_PING, pong, id, tim, utim, buf);
	}

	return 2;
}

static int Receive_sip(int ind) {
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}
	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}
	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		do_cmd_drink_fountain(player);
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c", ch);
		return 0;
	}
	return 1;
}


static int Receive_telekinesis(int ind) {
#if 1 /* taken over by Receive_mind() now, that mindcrafters got fusion */
	return Receive_mind(ind);
#else
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}
	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}
	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		/* Open/close mind to receive items via telekinesis */
		/* But we can also use this for telekinesis! - C. Blue
		   (mostly to avoid PK exploits */
		if (p_ptr->esp_link_flags & LINKF_TELEKIN) {
			msg_print(player, "\377yYou stop concentrating on telekinesis.");
			p_ptr->esp_link_flags &= ~LINKF_TELEKIN;
		} else {
			msg_print(player, "\377RYou concentrate on telekinesis!");
			p_ptr->esp_link_flags |= LINKF_TELEKIN;
		}
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c", ch);
		return 0;
	}
	return 1;
#endif
}

static int Receive_BBS(int ind) {
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	int n, player = -1;
	bool bbs_empty = TRUE;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];
	}
	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}
	if (connp->id != -1) {
		/* Look at in-game bbs - C. Blue */
		msg_print(player, "\377sBulletin board (type '/bbs <text>' in chat to write something):");
		censor_message = TRUE;
		for (n = 0; n < BBS_LINES; n++)
			if (strcmp(bbs_line[n], "")) {
				censor_length = strlen(bbs_line[n]) + bbs_line[n] - strchr(bbs_line[n], ':') - 4;
				msg_format(player, "\377s %s", bbs_line[n]);
				bbs_empty = FALSE;
			}
		censor_message = FALSE;
		if (bbs_empty) msg_print(player, "\377s <nothing has been written on the board so far>");
		return 2; /* consume no energy/don't disturb character (resting mode) */
	} else if (player) {
		Packet_printf(&connp->q, "%c", ch);
		return 0;
	}
	return 1;
}

static int Receive_wield2(int ind) {
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;

	char ch;
	s16b item;
	int n, player = -1;

	if (connp->id != -1) {
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}
	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check - mikaelh */
	if (item >= INVEN_TOTAL)
		return 1;

	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		item = replay_inven_changes(player, item);
		if (item == 0xFF)
		{
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}

		do_cmd_wield(player, item, 0x2);
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c%hd", ch, item);
		return 0;
	}
	return 1;
}

static int Receive_cloak(int ind) {
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}
	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}
	if (connp->id != -1 && p_ptr->energy >= level_speed(&p_ptr->wpos)) {
		do_cmd_cloak(player);
		return 2;
	} else if (player) {
		Packet_printf(&connp->q, "%c", ch);
		return 0;
	}
	return 1;
}

void change_mind(int Ind, bool open_or_close) {
	int Ind2;
	player_type *p_ptr = Players[Ind], *p_ptr2 = NULL;
	bool d = TRUE;

	/* 'hidden link' is unaffected by 'passive' change_mind() calls */
	if ((p_ptr->esp_link_flags & LINKF_HIDDEN) && !open_or_close) return;

	if ((Ind2 = get_esp_link(Ind, 0x0, &p_ptr2)) &&
	    !(p_ptr->esp_link_flags & LINKF_HIDDEN)) {
		if (p_ptr->esp_link_type == LINK_DOMINATED) {
			if (!p_ptr->esp_link_end) {
				p_ptr->esp_link_end = rand_int(6) + 15;
			} else {
				/* can't stabilize link while on different floors! */
				if (!inarea(&p_ptr->wpos, &p_ptr2->wpos)) return;

				p_ptr->esp_link_end = 0;
				d = FALSE;
			}
		}
		if (p_ptr2->esp_link_type == LINK_DOMINATED) {
			if (!p_ptr2->esp_link_end) {
				p_ptr2->esp_link_end = rand_int(6) + 15;
			} else {
				/* can't stabilize link while on different floors! */
				if (!inarea(&p_ptr->wpos, &p_ptr2->wpos)) return;

				p_ptr2->esp_link_end = 0;
				d = FALSE;
			}
		}

		if (d) {
			if (!(p_ptr->esp_link_flags & LINKF_HIDDEN)) {
				msg_format(Ind, "\377RThe mind link with %s begins to break.", p_ptr2->name);
				msg_format(Ind2, "\377RThe mind link with %s begins to break.", p_ptr->name);
			}
		} else {
			if (!(p_ptr->esp_link_flags & LINKF_HIDDEN)) {
				msg_format(Ind, "\377yThe mind link with %s stabilizes.", p_ptr2->name);
				msg_format(Ind2, "\377yThe mind link with %s stabilizes.", p_ptr->name);
			}
		}
	} else {
		if (p_ptr->esp_link_flags & LINKF_OPEN) {
			msg_print(Ind, "\377yYou close your mind.");
			p_ptr->esp_link_flags &= ~(LINKF_OPEN | LINKF_TELEKIN);
		} else {
			msg_print(Ind, "\377RYou open your mind..");
			p_ptr->esp_link_flags |= (LINKF_OPEN | LINKF_TELEKIN);
		}
	}
}

static int Receive_inventory_revision(int ind) {
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;
	char ch;
	int n, player = -1;
	int revision;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		p_ptr = Players[player];
	}
	if ((n = Packet_scanf(&connp->r, "%c%d", &ch, &revision)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}
	if (connp->id != -1) {
		if (connp->q.len) {
			/* There are some queued packets, block any further
			 * packets until the queue is empty
			 */
			Packet_printf(&connp->q, "%c%d", ch, revision);
			return 3; /* special return code */
		} else {
			inven_confirm_revision(player, revision);
			return 2;
		}
	} else if (player) {
		Packet_printf(&connp->q, "%c%d", ch, revision);
		return 0;
	}
	return 1;
}

static int Receive_account_info(int ind) {
	connection_t *connp = Conn[ind];
	char ch;
	int n, player = -1;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
	}
	if ((n = Packet_scanf(&connp->r, "%c", &ch)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (player > 0) {
		Send_account_info(player);
	}

	return 1;
}

static int Receive_change_password(int ind) {
	connection_t *connp = Conn[ind];
	char ch;
	int n, player = -1;
	char old_pass[MAX_CHARS], new_pass[MAX_CHARS];

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
	}
	if ((n = Packet_scanf(&connp->r, "%c%s%s", &ch, old_pass, new_pass)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}

	if (player > 0) {
		/* Obfuscation */
		my_memfrob(old_pass, strlen(old_pass));
		my_memfrob(new_pass, strlen(new_pass));

		account_change_password(player, old_pass, new_pass);

		/* Wipe the passwords from memory */
		memset(old_pass, 0, MAX_CHARS);
		memset(new_pass, 0, MAX_CHARS);
	}

	return 1;
}

static int Receive_force_stack(int ind) {
	connection_t *connp = Conn[ind];
	char ch;
	int n, player = -1;
	s16b item;
	player_type *p_ptr = NULL;

	if (connp->id != -1)
	{
		player = GetInd[connp->id];
		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &item)) <= 0)
	{
		if (n == -1)
			Destroy_connection(ind, "read error");
		return n;
	}

	/* Sanity check - mikaelh */
	if (item >= INVEN_PACK)
		return 1;

	if (connp->id != -1) {
		item = replay_inven_changes(player, item);
		if (item == 0xFF)
		{
			msg_print(player, "Command failed because item is gone.");
			return 1;
		}

		do_cmd_force_stack(player, item);
	}

	return 1;
}

static int Receive_request_key(int ind) {
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;

	char ch, id, key;
	int n, player = -1;
	if (connp->id != -1) {
		player = GetInd[connp->id];
//		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%d%c", &ch, &id, &key)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}
	if (connp->id == -1) return 1;

	handle_request_return_key(player, id, key);
	return 2;
}
static int Receive_request_num(int ind) {
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;

	char ch, id;
	int n, player = -1, num;
	if (connp->id != -1) {
		player = GetInd[connp->id];
//		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%d%d", &ch, &id, &num)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}
	if (connp->id == -1) return 1;

	handle_request_return_num(player, id, num);
	return 2;
}
static int Receive_request_str(int ind) {
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;

	char ch, id, str[MSG_LEN];
	int n, player = -1;
	if (connp->id != -1) {
		player = GetInd[connp->id];
//		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%d%s", &ch, &id, str)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}
	if (connp->id == -1) return 1;

	handle_request_return_str(player, id, str);
	return 2;
}
static int Receive_request_cfr(int ind) {
	connection_t *connp = Conn[ind];
	player_type *p_ptr = NULL;

	char ch, id;
	int n, player = -1, cfr;
	if (connp->id != -1) {
		player = GetInd[connp->id];
//		use_esp_link(&player, LINKF_OBJ);
		p_ptr = Players[player];
	}

	if ((n = Packet_scanf(&connp->r, "%c%d%d", &ch, &id, &cfr)) <= 0) {
		if (n == -1) Destroy_connection(ind, "read error");
		return n;
	}
	if (connp->id == -1) return 1;

	handle_request_return_cfr(player, id, (cfr != 0));
	return 2;
}


/* return some connection data for improved log handling - C. Blue */
char *get_conn_userhost(int ind) {
	return(format("%s@%s", Conn[ind]->real, Conn[ind]->host));
}
