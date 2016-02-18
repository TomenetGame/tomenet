#ifndef __Netconn_h
#define	__Netconn_h

typedef struct {
	int		state;
	int		drain_state;
	unsigned	magic;
	sockbuf_t	r;
	sockbuf_t	w;
	sockbuf_t	c;
	sockbuf_t	q;
	long		start;
	long		timeout;
/* - old UDP networking stuff - mikaelh
	long		last_send_loops;
	long		reliable_offset;
	long		reliable_unsent;
	long		retransmit_at_loop;
	int		rtt_smoothed;
	int		rtt_dev;
	int		rtt_retransmit;
	int		rtt_timeouts;
*/
	int		acks;
	int		setup;
	int		client_setup;
	int		my_port;
	int		his_port;
	int		id;
//	unsigned	version;
	version_type version;
	char		*real;
	char 		*nick;
	char		*c_name;
	char		*addr;
	char		*host;
	char		*pass;
	bool		password_verified;
	int		inactive_keepalive;
	int		inactive_ping;
	int		race;
	int		class;
	int		trait;
	int		sex;
	int		class_extra;
	short		stat_order[6];
	client_setup_t	Client_setup;

	int		audio_sfx, audio_mus;
//	char		q_static[MAX_RELIABLE_DATA_PACKET_SIZE]; /* for paralysation */
} connection_t;

#endif
