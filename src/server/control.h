/* GW stuff */

/* States */
#define CONN_FREE		0x00
#define CONN_CONNECTED		0x01

/* Maximum amount of connections */
#define MAX_GW_CONNS	20

/* Timeout (in seconds */
#define GW_CONN_TIMEOUT	10

typedef struct {
	int	state;
	int	sock;
	long	last_activity;
	sockbuf_t	r;
	sockbuf_t	w;
	
} gw_connection_t;
