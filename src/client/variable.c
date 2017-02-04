#include "angband.h"

/* Client global variables */

bool c_quit = FALSE;

char meta_address[MAX_CHARS] = "";
char nick[MAX_CHARS] = "";
char pass[MAX_CHARS] = "";
char svname[MAX_CHARS] = "";
char path[1024] = "";
char real_name[MAX_CHARS] = "";
char server_name[MAX_CHARS] = "";
s32b server_port;
char cname[MAX_CHARS] = "", prev_cname[MAX_CHARS];

int max_chars_per_account = 9;

s32b char_creation_flags = 1;	/* 0 = traditional stats rolling, 1 = player-definable stats - C. Blue */

char message_history[MSG_HISTORY_MAX][MSG_LEN];			/* history for chat, slash-cmd etc. */
char message_history_chat[MSG_HISTORY_MAX][MSG_LEN];		/* history for chat, slash-cmd etc. */
char message_history_msgnochat[MSG_HISTORY_MAX][MSG_LEN];	/* history for chat, slash-cmd etc. */
char message_history_impscroll[MSG_HISTORY_MAX][MSG_LEN];	/* history for chat, slash-cmd etc. */
int hist_end = 0;
bool hist_looped = FALSE;
int hist_chat_end = 0;
bool hist_chat_looped = FALSE;

object_type inventory[INVEN_TOTAL];	/* The client-side copy of the inventory */
char inventory_name[INVEN_TOTAL][ONAME_LEN];	/* The client-side copy of the inventory names */
int inventory_inscription[INVEN_TOTAL];	/* Position in an item name where a special inscription begins */
int inventory_inscription_len[INVEN_TOTAL];	/* Length of a special inscription */
int item_newest = -1;

store_type store;			/* The general info about the current store */
c_store_extra c_store;	/* Extra info about the current store */
int store_prices[STORE_INVEN_MAX];			/* The prices of the items in the store */
char store_names[STORE_INVEN_MAX][ONAME_LEN];		/* The names of the stuff in the store */
s16b store_num;				/* The current store number */

/* XXX Mergin for future expansion -- this should be handled in Net_setup */
char spell_info[MAX_REALM + 9][9][9][80];		/* Spell information */

char party_info_name[MAX_CHARS];		/* Information about your party: */
char party_info_members[MAX_CHARS];
char party_info_owner[MAX_CHARS];
char guild_info_name[MAX_CHARS];		/* Information about your guild: */
char guild_info_members[MAX_CHARS];
char guild_info_owner[MAX_CHARS];
bool guild_master;
guild_type guild;
int guildhall_wx = -1, guildhall_wy;
char guildhall_pos[14];

setup_t Setup;				/* The information given to us by the server */
client_setup_t Client_setup;		/* The information we give to the server */

bool shopping;				/* Are we in a store? */
bool perusing;				/* Are we browinsg a help file or similar? */

s16b last_line_info;			/* Last line of info we've received */
s32b max_line;				/* Maximum amount of "special" info */
s32b cur_line;				/* Current displayed line of "special" info */
#ifdef BIGMAP_MINDLINK_HACK
s16b last_line_y = 0;			/* for big_map mindlink differences */
#endif

player_type Players[2];			/* The client-side copy of some of the player information */
player_type *p_ptr = &Players[1];

c_player_extra c_player;
c_player_extra *c_p_ptr = &c_player;

char location_name2[MAX_CHARS], location_pre[10];


s32b exp_adv, exp_adv_prev;		/* Amount of experience required to advance a level */
byte half_exp; //EXP_BAR_FINESCALE

s16b command_see;
s16b command_gap;
s16b command_wrk;

bool item_tester_full;
byte item_tester_tval;
s16b item_tester_max_weight;
bool (*item_tester_hook)(object_type *o_ptr);

int special_line_type;
int special_page_size;

bool inkey_base = FALSE;
bool inkey_scan = FALSE;
bool inkey_max_line = FALSE;
bool inkey_flag = FALSE;
bool inkey_interact_macros = FALSE;
bool inkey_msg = FALSE;

bool inkey_letter_all = FALSE;

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
s32b message__next_impscroll;
s32b message__last_impscroll;
s32b message__head_impscroll;
s32b message__tail_impscroll;
s32b *message__ptr_impscroll;
char *message__buf_impscroll;


bool msg_flag; //no effect


term *ang_term[ANGBAND_TERM_MAX];
u32b window_flag[ANGBAND_TERM_MAX];

byte color_table[256][4];

cptr ANGBAND_SYS;

byte keymap_cmds[128];
byte keymap_dirs[128];

s16b command_cmd;
s16b command_dir;

boni_col csheet_boni[15]; /* a-n inventory slots + @ column -- Hardcode - Kurzel */
byte csheet_page = 0;
bool valid_dna = FALSE, dedicated = FALSE;
s16b race, dna_race;
s16b class, dna_class;
cptr dna_class_title; //ENABLE_DEATHKNIGHT,ENABLE_HELLKNIGHT,ENABLE_CPRIEST
s16b sex, dna_sex;
s16b mode;
s16b trait = 0, dna_trait;
s16b stat_order[6], dna_stat_order[6];			/* Desired order of stats */

s16b class_extra;

bool topline_icky;
short screen_icky;
bool party_mode = FALSE, guildcfg_mode = FALSE;

player_race *race_info;
player_class *class_info;
player_trait *trait_info;

//the +16 are just for some future-proofing, to avoid needing to update the client
char race_diz[MAX_RACE + 16][12][61]; /* 50 chars, 1 terminator, 10 for colour codes! */
char class_diz[MAX_CLASS + 16][12][61];
char trait_diz[MAX_TRAIT + 16][12][61];

cptr ANGBAND_DIR;
cptr ANGBAND_DIR_SCPT;
cptr ANGBAND_DIR_TEXT;
cptr ANGBAND_DIR_USER;
cptr ANGBAND_DIR_XTRA;
cptr ANGBAND_DIR_GAME;

bool disable_numlock;
bool use_graphics;
#ifdef USE_SOUND_2010
bool use_sound = TRUE, use_sound_org = TRUE; //ought to be set via TOMENET_SOUND environment var in linux, probably (compare TOMENET_GRAPHICS) -C. Blue
#else
bool use_sound, use_sound_org;
#endif
bool quiet_mode = FALSE;
bool noweather_mode = FALSE;
bool no_lua_updates = FALSE;
bool skip_motd = FALSE;
byte save_chat = 0;


client_opts c_cfg;

u32b sflags3 = 0x0, sflags2 = 0x0, sflags1 = 0x0, sflags0 = 0x0;
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
int ping_times[60], ping_avg = 0, ping_avg_cnt = 0;
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
int weather_speed_rain = 999; /* [3] */
int weather_speed_snow = 999; /* speed at which snowflakes move aka a second wind
        		  parameter (doesnt make sense for raindrops) [9] */
int weather_elements = 0; /* current amount of raindrops/snowflakes on the move */
int weather_element_x[1024], weather_element_y[1024], weather_element_ydest[1024], weather_element_type[1024];
int weather_panel_x = -1, weather_panel_y = -1; /* part of the map we're viewing on screen, top left corner */
bool weather_panel_changed = FALSE; /* view got updated anyway by switching panel? */
/* a client-side map_info buffer of current view panel (for weather) */
byte panel_map_a[MAX_SCREEN_WID][MAX_SCREEN_HGT];
char panel_map_c[MAX_SCREEN_WID][MAX_SCREEN_HGT];
/* is weather on current worldmap sector part of an elliptical cloud?: */
int cloud_x1[10], cloud_y1[10], cloud_x2[10], cloud_y2[10], cloud_dsum[10];
int cloud_xm100[10], cloud_ym100[10]; /* cloud movement in 1/100 grid per s */
int cloud_xfrac[10], cloud_yfrac[10];

/* Global variables for account options and password changing */
bool acc_opt_screen = FALSE;
bool acc_got_info = FALSE;
s16b acc_flags = 0;

/* Server detail flags */
bool s_RPG = FALSE, s_FUN = FALSE, s_ARCADE = FALSE, s_TEST = FALSE;
bool s_RPG_ADMIN = FALSE, s_PARTY = FALSE;
bool s_DED_IDDC = FALSE, s_DED_PVP = FALSE;
bool s_NO_PK = FALSE;

/* Server temporary feature flags */
u32b sflags_TEMP = 0x0;

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

/* Availability counter */
int audio_sfx = 0, audio_music = 0;

#ifdef USE_SOUND_2010
void (*mixing_hook)(void);
bool (*sound_hook)(int sound, int type, int vol, s32b player_id);
void (*sound_ambient_hook)(int sound_ambient);
void (*sound_weather_hook)(int sound);
void (*sound_weather_hook_vol)(int sound, int vol);
bool (*music_hook)(int music);
int cfg_audio_rate = 44100, cfg_max_channels = 32, cfg_audio_buffer = 1024;
int music_cur = -1, music_cur_song = -1, music_next = -1, music_next_song = -1, weather_channel = -1, weather_current = -1, weather_current_vol = -1, weather_channel_volume = 0, ambient_channel = -1, ambient_current = -1, ambient_channel_volume = 0;
int weather_particles_seen, weather_sound_change, weather_fading, ambient_fading;
bool wind_noticable = FALSE;
bool cfg_audio_master = TRUE, cfg_audio_music = TRUE, cfg_audio_sound = TRUE, cfg_audio_weather = TRUE, weather_resume = FALSE, ambient_resume = FALSE;
int cfg_audio_master_volume = 75, cfg_audio_music_volume = 100, cfg_audio_sound_volume = 100, cfg_audio_weather_volume = 100;
#if 1 /* WEATHER_VOL_PARTICLES */
int weather_vol_smooth, weather_vol_smooth_anti_oscill, weather_smooth_avg[20];
#endif
int grid_weather_volume = 100, grid_ambient_volume = 100, grid_weather_volume_goal = 100, grid_ambient_volume_goal = 100, grid_weather_volume_step, grid_ambient_volume_step;

/* sounds that are hard-coded on client-side, because they won't be transmitted from the server: */
int bell_sound_idx = -1, page_sound_idx = -1, warning_sound_idx = -1, rain1_sound_idx = -1, rain2_sound_idx = -1, snow1_sound_idx = -1, snow2_sound_idx = -1, browse_sound_idx = -1, browsebook_sound_idx = -1;

/* optimization options */
bool sound_hint = TRUE;

/* Don't cache audio */
bool no_cache_audio = FALSE;
#endif

/* Redraw skills if the menu is open */
bool redraw_skills = FALSE;
bool redraw_store = FALSE;

/* Linux window prefs - C. Blue */
generic_term_info term_prefs[10] = {
#if 0 /* too small */
	{ 1, -32000, -32000, 80, 24, "8x13" },//TomeNET
	{ 1, -32000, -32000, 80, 24, "6x10" },//Mirror
	{ 1, -32000, -32000, 80, 24, "6x10" },//Recall
	{ 1, -32000, -32000, 80, 22, "5x8" },//Choice
	{ 1, -32000, -32000, 80, 17, "6x10" },//Term-4
	{ 1, -32000, -32000, 80, 14, "5x8" },//Term-5
	{ 0, -32000, -32000, 80, 24, "5x8" },
	{ 0, -32000, -32000, 80, 24, "5x8" },
	{ 0, -32000, -32000, 80, 24, "5x8" },
	{ 0, -32000, -32000, 80, 24, "5x8" }
#else /* fitting for full-hd */
	{ 1, -32000, -32000, 80, 24, "9x15" },//TomeNET
	{ 1, -32000, -32000, 80, 24, "8x13" },//Mirror
	{ 1, -32000, -32000, 80, 24, "8x13" },//Recall
	{ 1, -32000, -32000, 80, 22, "8x13" },//Choice
	{ 1, -32000, -32000, 80, 17, "8x13" },//Term-4
	{ 1, -32000, -32000, 80, 14, "8x13" },//Term-5
	{ 0, -32000, -32000, 80, 24, "8x13" },
	{ 0, -32000, -32000, 80, 24, "8x13" },
	{ 0, -32000, -32000, 80, 24, "8x13" },
	{ 0, -32000, -32000, 80, 24, "8x13" }
#endif
};

/* Special input requests (PKT_REQUEST_...) - C. Blue */
bool request_pending = FALSE;
bool request_abort = FALSE;

/* For polymorphing by monster name
   and also for monster lore */
char monster_list_name[MAX_R_IDX][80], monster_list_symbol[MAX_R_IDX][2];
int monster_list_code[MAX_R_IDX], monster_list_idx = 0, monster_list_level[MAX_R_IDX];
bool monster_list_any[MAX_R_IDX], monster_list_breath[MAX_R_IDX];
/* For artifact lore */
char artifact_list_name[MAX_A_IDX][80];
int artifact_list_code[MAX_A_IDX], artifact_list_rarity[MAX_A_IDX], artifact_list_idx = 0;
bool artifact_list_specialgene[MAX_A_IDX];
/* For artifact lore */
char kind_list_name[MAX_K_IDX][80];
int kind_list_tval[MAX_K_IDX], kind_list_sval[MAX_K_IDX], kind_list_rarity[MAX_K_IDX], kind_list_idx = 0;
char kind_list_char[MAX_K_IDX], kind_list_attr[MAX_K_IDX];

/* For screenshots, to unmap custom fonts back to normally readable characters */
char monster_mapping_org[MAX_R_IDX + 1];
char monster_mapping_mod[256];
char floor_mapping_org[MAX_F_IDX + 1];
char floor_mapping_mod[256];

/* for DONT_CLEAR_TOPLINE_IF_AVOIDABLE */
char last_prompt[MSG_LEN] = { 0 };
bool last_prompt_macro = FALSE;

int screen_wid = SCREEN_WID, screen_hgt = SCREEN_HGT;
bool bigmap_hint = TRUE, global_big_map_hold = FALSE;
bool in_game = FALSE;
bool rand_term_lamp;
int rand_term_lamp_ticks;

int minimap_posx = -1, minimap_posy;
byte minimap_attr;
char minimap_char;

/* To suppress positive confirmation messages on automated char dumps/screen shots */
bool silent_dump = FALSE;
/* For cleaner equip-display -- using a mimic/bat form that cannot use weapons? */
bool equip_no_weapon = FALSE;

bool auto_reincarnation = FALSE;
char macro_trigger_exclusive[MAX_CHARS];
bool macro_processing_exclusive;

/* Default color map */
/* These can be overriden using TomeNET.ini or .tomenetrc */
/* The colors are stored as 32-bit integers, .e.g. #112233 -> 0x112233 */
u32b client_color_map[16] = {
	0x000000,	/* BLACK */
	0xffffff,	/* WHITE */
	0x9d9d9d,	/* GRAY */
	0xff8d00,	/* ORANGE */
	0xb70000,	/* RED */
	0x009d44,	/* GREEN */
	0x0000ff,	/* BLUE */
	0x8d6600,	/* UMBER */
	0x666666,	/* DARK GRAY */
	0xd7d7d7,	/* LIGHT GRAY */
	0xaf00ff,	/* VIOLET */
	0xffff00,	/* YELLOW */
	0xff3030,	/* LIGHT RED */
	0x00ff00,	/* LIGHT GREEN */
	0x00ffff,	/* LIGHT BLUE */
	0xc79d55,	/* LIGHT UMBER */
};

/* Allow code in src/common/ to dynamically check that this is the client */
bool is_client_side = TRUE;

#ifdef RETRY_LOGIN
bool rl_connection_destructible = FALSE, rl_connection_destroyed = FALSE, rl_password = FALSE;
byte rl_connection_state = 0;
bool player_pref_files_loaded = FALSE;
#endif

/* For in-client guide search */
int guide_lastline = -1;
char guide_race[64][MAX_CHARS];
char guide_class[64][MAX_CHARS];
char guide_skill[128][MAX_CHARS];
char guide_school[64][MAX_CHARS];
char guide_spell[256][MAX_CHARS];
int guide_races = 0, guide_classes = 0, guide_skills = 0, guide_schools = 0, guide_spells = 0;
char guide_chapter[256][MAX_CHARS], guide_chapter_no[256][8];
int guide_chapters = 0, guide_endofcontents = -1;

#ifdef WINDOWS
bool win_dontmoveuser = FALSE;
#endif

bool showing_inven = FALSE, showing_equip = FALSE;
