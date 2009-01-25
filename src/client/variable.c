/* $Id$ */
#include "angband.h"

/* Client global variables */

bool c_quit=FALSE;

char meta_address[MAX_CHARS]="";
char nick[MAX_CHARS]="";
char pass[MAX_CHARS]="";
char svname[MAX_CHARS]="";
char path[1024]="";
char real_name[MAX_CHARS]="";
char server_name[MAX_CHARS]="";
s32b server_port;
char cname[MAX_CHARS]="";

s32b char_creation_flags=1;	/* 0 = traditional stats rolling, 1 = player-definable stats - C. Blue */

char message_history[MSG_HISTORY_MAX][80];	/* history for chat, slash-cmd etc. */
char message_history_chat[MSG_HISTORY_MAX][80];	/* history for chat, slash-cmd etc. */
char message_history_msgnochat[MSG_HISTORY_MAX][80];	/* history for chat, slash-cmd etc. */
byte hist_end = 0;
bool hist_looped = FALSE;
byte hist_chat_end = 0;
bool hist_chat_looped = FALSE;

object_type inventory[INVEN_TOTAL];	/* The client-side copy of the inventory */
char inventory_name[INVEN_TOTAL][80];	/* The client-side copy of the inventory names */

store_type store;			/* The general info about the current store */
c_store_extra c_store;	/* Extra info about the current store */
int store_prices[STORE_INVEN_MAX];			/* The prices of the items in the store */
char store_names[STORE_INVEN_MAX][80];		/* The names of the stuff in the store */
s16b store_num;				/* The current store number */

/* XXX Mergin for future expansion -- this should be handled in Net_setup */
char spell_info[MAX_REALM + 9][9][9][80];		/* Spell information */

char party_info_name[90];		/* Information about your party: */
char party_info_members[20];
char party_info_owner[50];

setup_t Setup;				/* The information given to us by the server */
client_setup_t Client_setup;		/* The information we give to the server */

bool shopping;				/* Are we in a store? */

s16b last_line_info;			/* Last line of info we've received */
s16b max_line;				/* Maximum amount of "special" info */
s16b cur_line;				/* Current displayed line of "special" info */

player_type Players[2];			/* The client-side copy of some of the player information */
player_type *p_ptr = &Players[1];

c_player_extra c_player;
c_player_extra *c_p_ptr = &c_player;


s32b exp_adv;				/* Amount of experience required to advance a level */

s16b command_see;
s16b command_gap;
s16b command_wrk;

bool item_tester_full;
byte item_tester_tval;
bool (*item_tester_hook)(object_type *o_ptr);

int special_line_type;

bool inkey_base = FALSE;
bool inkey_scan = FALSE;
bool inkey_flag = FALSE;
bool inkey_interact_macros = FALSE;
bool inkey_msg = FALSE;

s16b macro__num;
cptr *macro__pat;
cptr *macro__act;
bool *macro__cmd;
bool *macro__hyb;
char *macro__buf;

u32b message__next;
u32b message__last;
u32b message__head;
u32b message__tail;
u32b *message__ptr;
char *message__buf;
u32b message__next_chat;
u32b message__last_chat;
u32b message__head_chat;
u32b message__tail_chat;
u32b *message__ptr_chat;
char *message__buf_chat;
u32b message__next_msgnochat;
u32b message__last_msgnochat;
u32b message__head_msgnochat;
u32b message__tail_msgnochat;
u32b *message__ptr_msgnochat;
char *message__buf_msgnochat;


bool msg_flag;


term *ang_term[8];
u32b window_flag[8];

byte color_table[256][4];

cptr ANGBAND_SYS;

byte keymap_cmds[128];
byte keymap_dirs[128];

s16b command_cmd;
s16b command_dir;


s16b race;
s16b class;
s16b sex;
s16b mode;

s16b stat_order[6];			/* Desired order of stats */

s16b class_extra;

bool topline_icky;
short screen_icky;
bool party_mode;

player_race *race_info;
player_class *class_info;

cptr ANGBAND_DIR;
cptr ANGBAND_DIR_SCPT;
cptr ANGBAND_DIR_APEX;
cptr ANGBAND_DIR_BONE;
cptr ANGBAND_DIR_DATA;
cptr ANGBAND_DIR_EDIT;
cptr ANGBAND_DIR_FILE;
cptr ANGBAND_DIR_HELP;
cptr ANGBAND_DIR_INFO;
cptr ANGBAND_DIR_SAVE;
cptr ANGBAND_DIR_USER;
cptr ANGBAND_DIR_XTRA;


bool use_graphics;
bool use_sound;



client_opts c_cfg;

byte client_mode = CLIENT_NORMAL;

u32b cfg_game_port = 18348;

skill_type s_info[MAX_SKILLS];

s16b flush_count = 0;

/*
 * The spell list of schools
 */
s16b max_spells;
spell_type *school_spells;
s16b max_schools;
school_type *schools;

/* Server ping statistics */
int ping_id = 0;
int ping_times[60];
bool lagometer_enabled = FALSE; /* Must be disabled during login */
bool lagometer_open = FALSE;

/* Chat mode: normal, party or level */
char chat_mode = CHAT_MODE_NORMAL;

/* Protocol to be used for connecting a server
 * 1 = traditional
 * 2 = with extended version
 */
s32b server_protocol = 2;

/* Server version */
version_type server_version;

/* Client fps used for polling keyboard input etc */
int cfg_client_fps = 60;
