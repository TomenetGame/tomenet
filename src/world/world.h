/* world server - copyright 2002 evileye
 *
 */

#define MAX_LTTL	20	/* Max TTL for temporary locks */

#define MAX_SERVERS	30	/* Max servers we will deal */

#define WP_CHAT		1	/* chat message */
#define WP_NPLAYER	2	/* player enters */
#define WP_QPLAYER	3	/* player leaves */
#define WP_DEATH	4	/* player death */
#define WP_LOCK		5	/* obtain a lock (timed) */
#define WP_UNLOCK	6	/* free the lock */
#define WP_MESSAGE	7	/* all critical server messages */
#define WP_AUTH		8	/* server authing */
#define WP_SQUIT	9	/* server quits */
#define WP_RESTART	10	/* servers quit now */
#define WP_LACCOUNT	11	/* login account */
#define WP_PMSG		12	/* private message */
#define WP_SINFO	13	/* server info */

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

/* Single linked list - its not like we are sorting it */
struct list{
	struct list *next;
	void *data;		/* pointer to the data structure */
};

struct rplist{
	unsigned long id;
	short server;
	char name[30];
};

/* linked list will use less mem */
struct objlock{
	short owner;		/* Owner ID */
	unsigned long ttl;	/* time to live for non final lock */
	unsigned long obj;	/* lock object by number (monster, item etc.) */
};

struct client{
	int fd;
	unsigned short flags;
	short authed;		/* Server ID (>0), authing (0), or failed authentication (-1) */
	unsigned short blen;
	char buf[1024];
};

struct secure{
	short secure;	/* kick off ALL unauthed clients */
	short chat;	/* Permit chat if unauthed (and not secure) */
	short play;	/* Players online messages (no tracing on unauthed) */
	short msgs;	/* Permit server messages */
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
	unsigned long id;	/* From ID */
	char ctxt[120];
};

struct pmsg{
	unsigned long id;	/* From ID */
	unsigned short sid;	/* To server ID */
	char player[80];	/* thats what it is in server :( */
	char victim[80];	/* thats what it is in server :( */
	char ctxt[120];
};

struct sinfo{
	unsigned short sid;
	unsigned short port;	/* needed for client transfers */
	char name[30];
};


/* server world authentication */
struct auth{
	unsigned char pass[21];
	unsigned long val;
};

#define PL_INIT	1	/* init from client with password */
#define PL_OK	2	/* from server */
#define PL_FAIL	3	/* auth fail */
#define PL_USED	4	/* char name is owned */
#define PL_DUP	5	/* logged in already */
#define PL_QUERY 6	/* query from client */

/* identical to struct account */
/* only AUTHED servers can do this */
struct pl_auth{
	u_int32_t id;	/* account id */
	u_int16_t flags;	/* account flags */
	u_int16_t stat;	/* status (for return) */
	char name[30];	/* login */
	char pass[20];	/* some crypts are not 13 */
	char pname[80];	/* player character name */
};

#define LT_ARTIFACT	1
#define LT_MONSTER	2	/* not sure how i'm gonna do this yet */

struct lock{
	unsigned short ltype;	/* Lock type */
	unsigned long ttl;	/* time to live for non final lock */
	unsigned long obj;	/* lock object by number (monster, item etc.) */
};

struct smsg{
	char stxt[160];		/* may need more info than this sometime */
};
  
struct wpacket{
	unsigned short type;	/* TYPE */
	unsigned short serverid;
	union {
		short sid;
		struct chat chat;
		struct smsg smsg;
		struct player play;
		struct auth auth;
		struct lock lock;
		struct pmsg pmsg;
		struct pl_auth login;
		struct sinfo sinfo;
	} d;
};
