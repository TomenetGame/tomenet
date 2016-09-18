#ifndef __Netserver_h
#define	__Netserver_h


#ifdef WINDOWS
# define EWOULDBLOCK WSAEWOULDBLOCK
#endif

#define MAX_SELECT_FD                   1023
/* #define MAX_RELIABLE_DATA_PACKET_SIZE        1024 */
#define MAX_RELIABLE_DATA_PACKET_SIZE   512

#define MAX_MOTD_CHUNK                  512
#define MAX_MOTD_SIZE                   (30*1024)
#define MAX_MOTD_LOOPS                  120


#define NOT_CONNECTED		(-1)

#define CONN_FREE		0x00
#define CONN_LISTENING		0x01
#define CONN_SETUP		0x02
#define CONN_LOGIN		0x04
#define CONN_PLAYING		0x08
#define CONN_DRAIN		0x20
#define CONN_READY		0x40

#define LISTEN_TIMEOUT		120
#define SETUP_TIMEOUT           150
/* in seconds, for max total duration of character creation process: */
#define LOGIN_TIMEOUT           2400
#define READY_TIMEOUT           30
#define IDLE_TIMEOUT            25 /* 20; was 15, but quitting in dungeon should take 20 seconds */

#if 0 /* old UDP networking stuff - mikaelh */
 #define MAX_RTT			(2 * FPS)

 #define MIN_RETRANSMIT		(FPS / 8 + 1)
 #define MAX_RETRANSMIT		(2 * FPS)
 #define DEFAULT_RETRANSMIT	(FPS / 2)
#endif

#define MAX_NAME_LEN		20	/* apparently unused. use NAME_LEN instead */

#include "netconn.h"

static void Contact(int fd, int arg);
//static void Console(int fd, int arg);
static int Enter_player(char *real, char *name, char *addr, char *host,
				version_type *version, int port, int *login_port, int fd);

static int Handle_listening(int ind);
static int Handle_setup(int ind);
static int Handle_login(int ind);
void Handle_input(int fd, int arg);
void do_quit(int ind, bool tellclient);

static int Receive_quit(int ind);
static int Receive_play(int ind);
static int Receive_login(int ind);
// static int Receive_ack(int ind);
static int Receive_discard(int ind);
static int Receive_undefined(int ind);

static int Receive_file(int ind);

static int Receive_keepalive(int ind);
static int Receive_King(int ind);
static int Receive_walk(int ind);
static int Receive_run(int ind);
static int Receive_tunnel(int ind);
static int Receive_aim_wand(int ind);
static int Receive_drop(int ind);
static int Receive_fire(int ind);
static int Receive_stand(int ind);
static int Receive_stand_one(int ind);
static int Receive_destroy(int ind);
static int Receive_look(int ind);

static int Receive_skill_mod(int ind);
static int Receive_skill_dev(int ind);
static int Receive_open(int ind);
static int Receive_ghost(int ind);
static int Receive_quaff(int ind);
static int Receive_read(int ind);
static int Receive_search(int ind);
static int Receive_take_off(int ind);
static int Receive_take_off_amt(int ind);
static int Receive_use(int ind);
static int Receive_throw(int ind);
static int Receive_wield(int ind);
static int Receive_observe(int ind);
static int Receive_zap(int ind);
static int Receive_zap_dir(int ind);

static int Receive_target(int ind);
static int Receive_target_friendly(int ind);
static int Receive_inscribe(int ind);
static int Receive_uninscribe(int ind);
static int Receive_autoinscribe(int ind);
static int Receive_activate(int ind);
static int Receive_activate_dir(int ind);
static int Receive_bash(int ind);
static int Receive_disarm(int ind);
static int Receive_eat(int ind);
static int Receive_fill(int ind);
static int Receive_locate(int ind);
static int Receive_map(int ind);
static int Receive_search_mode(int ind);

static int Receive_close(int ind);
static int Receive_direction(int ind);
static int Receive_go_up(int ind);
static int Receive_go_down(int ind);
static int Receive_message(int ind);
static int Receive_item(int ind);
static int Receive_spell(int ind);
static int Receive_purchase(int ind);
static int Receive_mind(int ind);

static int Receive_sell(int ind);
static int Receive_store_leave(int ind);
static int Receive_store_confirm(int ind);
static int Receive_drop_gold(int ind);
static int Receive_redraw(int ind);
static int Receive_rest(int ind);
static int Receive_party(int ind);
static int Receive_special_line(int ind);

static int Receive_steal(int ind);
static int Receive_options(int ind);
static int Receive_screen_dimensions(int ind);
static int Receive_suicide(int ind);
static int Receive_master(int ind);
static int Receive_admin_house(int ind);
static int Receive_autophase(int ind);

static int Receive_clear_buffer(int ind);
static int Receive_clear_actions(int ind);

static int Receive_spike(int ind);
static int Receive_guild(int ind);
static int Receive_guild_config(int ind);
static int Receive_activate_skill(int ind);
static int Receive_raw_key(int ind);
static int Receive_store_examine(int ind);
static int Receive_store_command(int ind);

static int Receive_ping(int ind);

static int Receive_sip(int ind);
static int Receive_telekinesis(int ind);
static int Receive_BBS(int ind);
static int Receive_wield2(int ind);
static int Receive_cloak(int ind);
static int Receive_inventory_revision(int ind);
static int Receive_account_info(int ind);
static int Receive_change_password(int ind);

static int Receive_force_stack(int ind);

static void Handle_item(int Ind, int item);

static int Receive_request_key(int ind);
static int Receive_request_num(int ind);
static int Receive_request_str(int ind);
static int Receive_request_cfr(int ind);

static int Receive_client_setup(int ind);
//debugging:
static int Receive_client_setup_U(int ind);
static int Receive_client_setup_F(int ind);
static int Receive_client_setup_K(int ind);
static int Receive_client_setup_R(int ind);

int Setup_net_server(void);
bool Destroy_connection(int ind, char *reason);
int Check_connection(char *real, char *nick, char *addr);
int Setup_connection(char *real, char *nick, char *addr, char *host, version_type *version, int fd);
int Input(void);
int Send_reply(int ind, int replyto, int result);
int Send_leave(int ind, int id);
int Send_reliable(int ind);
int Send_sanity(int ind, byte attr, cptr msg);

char *showtime(void);
void init_players();

#endif
