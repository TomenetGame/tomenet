/* world server - copyright 2002 evileye
 *
 */

#define WP_CHAT		1	/* chat message */
#define WP_NPLAYER	2	/* player enters */
#define WP_QPLAYER	3	/* player leaves */
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

struct client{
	struct client *next;
	int fd;
	unsigned short flags;
	unsigned short bpos;
	unsigned short blen;
	char buf[1024];
};

/* The structures of these packets will be
   changed when we merge data */

struct player{
	unsigned long id;	/* UNIQUE player id */
	char name[30];
	unsigned char silent;	/* Left due to death for instance */
};

struct chat{
	unsigned long id;
	char ctxt[80];
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
		int lockval;
	} d;
};
