/* world server - copyright 2002 evileye
 *
 */

#define MAX_SERVERS	30	/* Max servers we will deal */

#define WP_CHAT		1	/* chat message */
#define WP_NPLAYER	2	/* player enters */
#define WP_QPLAYER	3	/* player leaves */
#define WP_DEATH	4	/* player death */
#define WP_LOCK		5	/* obtain a lock (timed) */
#define WP_UNLOCK	6	/* free the lock */
#define WP_MESSAGE	7	/* all critical server messages */
#define WP_AUTH		8	/* server authing */

/* now we are going to be the server which authenticates
 * the players. Once they are logged in, they will receive
 * a key which will enable to pass worlds without needing
 * to be reauthenticated
 */

#define CL_QUIT		1

struct serverinfo{
	char name[20];	/* server world name */
	char pass[20];	/* server plaintext password */
};

struct secure{
	short secure;	/* kick off ALL unauthed clients */
	short chat;	/* Permit chat if unauthed (and not secure) */
	short play;	/* Players online tracing */
	short msgs;	/* Permit server messages */
};

struct client{
	struct client *next;
	int fd;
	unsigned short flags;
	short authed;		/* Server ID (>0), authing (0), or failed authentication (-1) */
	unsigned short blen;
	char buf[1024];
};

/* The structures of these packets will be
   changed when we merge data */

struct player{
	unsigned long id;	/* UNIQUE player id */
	short server;		/* server info 0 means unknown */
	char name[30];		/* temp. player name */
	unsigned char silent;	/* Left due to death for instance */
};

struct death{
	unsigned long id;
	unsigned short dtype;	/* death type */
	char name[30];		/* temp. player name */
	char method[40];	/* death method */
};

struct chat{
	unsigned long id;
	char ctxt[120];
};

struct auth{
	char pass[30];
};

struct smsg{
	char stxt[160];		/* may need more info than this sometime */
};
  
struct wpacket{
	unsigned short type;	/* TYPE */
	unsigned short serverid;
	union {
		struct chat chat;
		struct smsg smsg;
		struct player play;
		struct auth auth;
		int lockval;
	} d;
};
