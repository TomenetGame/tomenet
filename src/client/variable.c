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

char message_history[MSG_HISTORY_MAX][121];		/* history for chat, slash-cmd etc. */
char message_history_chat[MSG_HISTORY_MAX][121];	/* history for chat, slash-cmd etc. */
char message_history_msgnochat[MSG_HISTORY_MAX][121];	/* history for chat, slash-cmd etc. */
byte hist_end = 0;
bool hist_looped = FALSE;
byte hist_chat_end = 0;
bool hist_chat_looped = FALSE;

object_type inventory[INVEN_TOTAL];	/* The client-side copy of the inventory */
char inventory_name[INVEN_TOTAL][ONAME_LEN];	/* The client-side copy of the inventory names */
int inventory_inscription[INVEN_TOTAL];	/* Position in an item name where a special inscription begins */
int inventory_inscription_len[INVEN_TOTAL];	/* Length of a special inscription */

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
bool inkey_max_line = FALSE;
bool inkey_flag = FALSE;
bool inkey_interact_macros = FALSE;
bool inkey_msg = FALSE;

s16b macro__num;
cptr *macro__pat;
cptr *macro__act;
bool *macro__cmd;
bool *macro__hyb;
char *macro__buf;

char recorded_macro[160];
bool recording_macro = FALSE;

s32b message__next;
s32b message__last;
s32b message__head;
s32b message__tail;
s32b *message__ptr;
char *message__buf;
s32b message__next_chat;
s32b message__last_chat;
s32b message__head_chat;
s32b message__tail_chat;
s32b *message__ptr_chat;
char *message__buf_chat;
s32b message__next_msgnochat;
s32b message__last_msgnochat;
s32b message__head_msgnochat;
s32b message__tail_msgnochat;
s32b *message__ptr_msgnochat;
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
#ifdef USE_SOUND_2010
bool use_sound = TRUE; //ought to be set via TOMENET_SOUND environment var in linux, probably (compare TOMENET_GRAPHICS) -C. Blue
#else
bool use_sound;
#endif


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

/* Client fps used for polling keyboard input etc -
   old: 60; safe optimum for client-side weather: 100 */
int cfg_client_fps = 100;


/* For unique list in char dump - C. Blue */
byte r_unique[MAX_UNIQUES];
char r_unique_name[MAX_UNIQUES][60];

/* hack for stronger no-tele vault display warning */
int p_speed = 110;
bool no_tele_grid = FALSE;

/* for weather */
int weather_type = 0; /* stop(-1)/none/rain/snow; hacks: +20000, +10000, +n*10 */
int weather_gen_speed = 0; /* speed at which new weather elements are generated */
int weather_wind = 0; /* current gust of wind if any (1 west, 2 east, 3 strong west, 4 strong east) */
int weather_intensity = 1; /* density of raindrops / snowflakes */
int weather_speed = 1; /* speed at which snowflakes move aka a second wind
        		  parameter (doesnt make sense for raindrops) */
int weather_elements = 0; /* current amount of raindrops/snowflakes on the move */
int weather_element_x[1024], weather_element_y[1024], weather_element_ydest[1024], weather_element_type[1024];
int weather_panel_x, weather_panel_y; /* part of the map we're viewing on screen, top left corner */
bool weather_panel_changed; /* view got updated anyway by switching panel? */
/* a client-side map_info buffer of current view panel (for weather) */
byte panel_map_a[SCREEN_WID][SCREEN_HGT];
char panel_map_c[SCREEN_WID][SCREEN_HGT];
/* is weather on current worldmap sector part of an elliptical cloud?: */
int cloud_x1[10], cloud_y1[10], cloud_x2[10], cloud_y2[10], cloud_dsum[10];
int cloud_xm100[10], cloud_ym100[10]; /* cloud movement in 1/100 grid per s */
int cloud_xfrac[10], cloud_yfrac[10];

/* Global variables for account options and password changing */
bool acc_opt_screen = FALSE;
bool acc_got_info = FALSE;
s16b acc_flags = 0;

/* Server detail flags */
bool s_RPG = FALSE;
bool s_FUN = FALSE;
bool s_PARTY = FALSE;
bool s_ARCADE = FALSE;
bool s_TEST = FALSE;
bool s_RPG_ADMIN = FALSE;

/* Auto-inscriptions */
char auto_inscription_match[MAX_AUTO_INSCRIPTIONS][40];
char auto_inscription_tag[MAX_AUTO_INSCRIPTIONS][20];

/* Monster health memory (health_redraw) */
int mon_health_num;
byte mon_health_attr;

/* Location information memory (prt_depth) */
bool depth_town;
int depth_colour;
int depth_colour_sector;
char depth_name[MAX_CHARS];

/* Can macro triggers consist of multiple keys? */
bool multi_key_macros = FALSE;


#ifdef USE_SOUND_2010
void (*mixing_hook)(void);
bool (*sound_hook)(int sound, int type, int vol, s32b player_id);
void (*sound_weather_hook)(int sound);
void (*music_hook)(int music);
int cfg_audio_rate = 44100, cfg_max_channels = 32, cfg_audio_buffer = 512;
int music_next = -1, weather_channel = -1, weather_current;
int weather_particles_seen, weather_sound_change, weather_fading;
bool cfg_audio_master = TRUE, cfg_audio_music = TRUE, cfg_audio_sound = TRUE, cfg_audio_weather = TRUE;
int cfg_audio_master_volume = 100, cfg_audio_music_volume = 100, cfg_audio_sound_volume = 100, cfg_audio_weather_volume = 100;

/* sounds that are hard-coded on client-side, because they won't be transmitted from the server: */
int page_sound_idx = -1, rain1_sound_idx = -1, rain2_sound_idx = -1, snow1_sound_idx = -1, snow2_sound_idx = -1, browse_sound_idx = -1, browsebook_sound_idx = -1;

/* optimization options */
bool count_half_sfx_attack = TRUE, sound_hint = TRUE;

/* Don't cache audio */
bool no_cache_audio = FALSE;
#endif

/* Standard sound (and message) names */
const cptr angband_sound_name[SOUND_MAX] = {
    "hit",
    "miss",
    "flee",
    "drop",
    "kill",
    "level",
    "death",
    "warn",
};
