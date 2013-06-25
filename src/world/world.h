/* world server - copyright 2002 evileye
 *
 */

#include <stdint.h>

#define MSG_LEN 256

#define MAX_LTTL	20	/* Max TTL for temporary locks */

#define MAX_SERVERS	30	/* Max servers we will deal */

/* World packet types */
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
#define WP_IRCCHAT	14	/* chat message directed to a particular server (irc-relay) */
#define WP_MSG_TO_IRC	15	/* chat message to IRC channel only (not to other game servers) */

/* World packet flags */
#define WPF_CHAT	0x0001	/* chat message - C */
#define WPF_NPLAYER	0x0002	/* player enters - N */
#define WPF_QPLAYER	0x0004	/* player leaves - Q */
#define WPF_DEATH	0x0008	/* player death - D */
#define WPF_LOCK	0x0010
#define WPF_UNLOCK	0x0020
#define WPF_MESSAGE	0x0040	/* all critical server messages - M */
#define WPF_AUTH	0x0080
#define WPF_SQUIT	0x0100
#define WPF_RESTART	0x0200
#define WPF_LACCOUNT	0x0400
#define WPF_PMSG	0x0800	/* private message - P */
#define WPF_SINFO	0x1000
#define WPF_IRCCHAT	0x2000	/* chat message from IRC relay - S */

/* World message flags */
#define WMF_LVLUP	0x01
#define WMF_UNIDEATH	0x02
#define WMF_PWIN	0x04
#define WMF_HILVLUP	0x08
#define WMF_PDEATH	0x10
#define WMF_PJOIN	0x20
#define WMF_PLEAVE	0x40
#define WMF_EVENTS	0x80

/* now we are going to be the server which authenticates
 * the players. Once they are logged in, they will receive
 * a key which will enable to pass worlds without needing
 * to be reauthenticated
 */

#define CL_QUIT		1

struct serverinfo{
	char name[20];		/* server world name */
	char pass[20];		/* server plaintext password */
	uint32_t rflags;	/* relay flags for packets sent to server */
	uint32_t mflags;	/* messages flags */
};

/* Single linked list - its not like we are sorting it */
struct list{
	struct list *next;
	void *data;		/* pointer to the data structure */
};

struct rplist{
	uint32_t id;
	int16_t server;
	char name[30];
};

/* linked list will use less mem */
struct objlock{
	int16_t owner;		/* Owner ID */
	uint32_t ttl;	/* time to live for non final lock */
	uint32_t obj;	/* lock object by number (monster, item etc.) */
};

struct client{
	int fd;
	uint16_t flags;
	int16_t authed;		/* Server ID (>0), authing (0), or failed authentication (-1) */
	uint16_t blen;
	char buf[1024];
};

struct secure{
	int16_t secure;	/* kick off ALL unauthed clients */
	int16_t chat;	/* Permit chat if unauthed (and not secure) */
	int16_t play;	/* Players online messages (no tracing on unauthed) */
	int16_t msgs;	/* Permit server messages */
};

/* The structures of these packets will be
   changed when we merge data */

struct player{
	uint32_t id;	/* UNIQUE player id */
	uint16_t server;		/* server info 0 means unknown */
	char name[30];		/* temp. player name */
	uint8_t silent;	/* Left due to death for instance */
};

struct death{
	uint32_t id;
	uint16_t dtype;	/* death type */
	char name[30];		/* temp. player name */
	char method[40];	/* death method */
};

struct chat{
	uint32_t id;	/* From ID */
	char ctxt[MSG_LEN];
};

struct pmsg{
	uint32_t id;	/* From ID */
	uint16_t sid;	/* To server ID */
	char player[80];	/* thats what it is in server :( */
	char victim[80];	/* thats what it is in server :( */
	char ctxt[MSG_LEN];
};

struct sinfo{
	uint16_t sid;
	uint16_t port;	/* needed for client transfers */
	char name[30];
};


/* server world authentication */
struct auth{
	char pass[21];
	uint32_t val;
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
	uint32_t id;	/* account id */
	uint16_t flags;	/* account flags */
	uint16_t stat;	/* status (for return) */
	char name[30];	/* login */
	char pass[20];	/* some crypts are not 13 */
	char pname[80];	/* player character name */
};

#define LT_ARTIFACT	1
#define LT_MONSTER	2	/* not sure how i'm gonna do this yet */

struct lock{
	uint16_t ltype;	/* Lock type */
	uint32_t ttl;	/* time to live for non final lock */
	uint32_t obj;	/* lock object by number (monster, item etc.) */
};

struct smsg{
	char stxt[MSG_LEN];		/* may need more info than this sometime */
};
  
struct wpacket{
	uint16_t type;	/* TYPE */
	uint16_t serverid;
	union {
		int16_t sid;
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
