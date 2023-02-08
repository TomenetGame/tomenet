#include "angband.h"

/* Client global variables */

bool c_quit = FALSE;

char meta_address[MAX_CHARS] = "", meta_buf[80192];
int meta_socket = -1, meta_i = 0;
char nick[MAX_CHARS] = "";
char pass[MAX_CHARS] = "";
char svname[MAX_CHARS] = "";
char path[1024] = "";
char real_name[MAX_CHARS] = "";
char server_name[MAX_CHARS] = "";
s32b server_port;
char cname[MAX_CHARS] = "", prev_cname[MAX_CHARS];

int max_chars_per_account = 11;

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

#ifdef ENABLE_SUBINVEN
int using_subinven = -1, using_subinven_size = SUBINVEN_PACK, using_subinven_item = -1;
object_type subinventory[INVEN_PACK + 1][SUBINVEN_PACK + 1];
char subinventory_name[INVEN_PACK + 1][SUBINVEN_PACK + 1][ONAME_LEN];
int subinventory_inscription[INVEN_PACK + 1][SUBINVEN_PACK + 1];
int subinventory_inscription_len[INVEN_PACK + 1][SUBINVEN_PACK + 1];
#endif

byte equip_set[INVEN_TOTAL - INVEN_WIELD];

store_type store;			/* The general info about the current store */
c_store_extra c_store;	/* Extra info about the current store */
unsigned char store_price_mul;
int store_prices[STORE_INVEN_MAX];			/* The prices of the items in the store */
char store_names[STORE_INVEN_MAX][ONAME_LEN];		/* The names of the stuff in the store */
char store_powers[STORE_INVEN_MAX][MAX_CHARS_WIDE];		/* For chat-pasting: Add '@@'-info so everyone knows what the store item can do. Equippables only, hidden-powers-egos only. */
s16b store_num;				/* The current store number */

/* XXX Mergin for future expansion -- this should be handled in Net_setup */
char spell_info[MAX_REALM + 9][9][9][80];		/* Spell information */

byte party_info_mode = 0x0;			/* Information about your party: */
char party_info_name[MAX_CHARS];
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

bool shopping = FALSE;			/* Are we in a store? */
bool perusing = FALSE;			/* Are we browinsg a help file or similar? */
bool local_map_active = FALSE;		/* Are we allowing minimap visuals appear over the main screen (map) visuals */

s16b last_line_info;			/* Last line of info we've received */
s32b max_line;				/* Maximum amount of "special" info */
s32b cur_line;				/* Current displayed line of "special" info */
bool line_searching = FALSE;
int cur_col;
#ifdef BIGMAP_MINDLINK_HACK
s16b last_line_y = 0;			/* for big_map mindlink differences */
#endif

player_type Players_client[2];			/* The client-side copy of some of the player information */
player_type *p_ptr = &Players_client[1];
player_type *Players_pointers[2] = { Players_client, Players_client + 1 };
player_type **Players = Players_pointers;

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
char special_line_title[ONAME_LEN], special_line_first[ONAME_LEN];

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

cptr ANGBAND_SYS = NULL;

byte keymap_cmds[128];
byte keymap_dirs[128];

s16b command_cmd;
s16b command_dir;

boni_col csheet_boni[15]; /* a-n inventory slots + @ column -- Hardcode - Kurzel */
byte csheet_page = 0;
bool csheet_horiz = FALSE;
bool valid_dna = FALSE, dedicated = FALSE;
s16b race, dna_race;
s16b class, dna_class;
cptr dna_class_title; //ENABLE_DEATHKNIGHT,ENABLE_HELLKNIGHT,ENABLE_CPRIEST
s16b sex, dna_sex;
s16b mode, lives;
s16b trait = 0, dna_trait;
s16b stat_order[6], dna_stat_order[6];			/* Desired order of stats */

s16b class_extra;

bool topline_icky;
signed short screen_icky, screen_line_icky = -1, screen_column_icky = -1;
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
#ifdef USE_GRAPHICS
char graphic_tiles[256] = "\0";
#endif
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
#ifdef GLOBAL_BIG_MAP
bool global_c_cfg_big_map = FALSE;
#endif

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
int weather_type = 0; /* stop(-1)/none/rain/snow/sandstorm; hacks: +20000, +10000, +n*10 */
int weather_gen_speed = 0; /* speed at which new weather elements are generated */
int weather_wind = 0; /* current gust of wind if any (1 west, 2 east, 3 strong west, 4 strong east) <- wrong? 1/2 are the strongest winds? */
int weather_intensity = 1; /* density of raindrops/snowflakes/sandgrains */
int weather_speed_rain = 999; /* [3] */
int weather_speed_snow = 999; /* speed at which snowflakes move aka a second wind
				 parameter (doesnt make sense for raindrops) [9] */
int weather_speed_sand = 999;
int weather_elements = 0; /* current amount of raindrops/snowflakes/sandgrains on the move */
int weather_element_x[1024], weather_element_y[1024], weather_element_ydest[1024], weather_element_type[1024];
int weather_panel_x = -1, weather_panel_y = -1; /* part of the map we're viewing on screen, top left corner */
bool weather_panel_changed = FALSE; /* view got updated anyway by switching panel? */
/* a client-side map_info buffer of current view panel (for weather) */
#if 1 /* RAINY_TOMB */
byte panel_map_a[MAX_WINDOW_WID][MAX_WINDOW_HGT];
char32_t panel_map_c[MAX_WINDOW_WID][MAX_WINDOW_HGT];
#else
byte panel_map_a[MAX_SCREEN_WID][MAX_SCREEN_HGT];
char32_t panel_map_c[MAX_SCREEN_WID][MAX_SCREEN_HGT];
#endif
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
bool s_NO_PK = FALSE, s_PVP_MAIA = FALSE;

/* Server temporary feature flags */
u32b sflags_TEMP = 0x0;

/* Auto-inscriptions */
char auto_inscription_match[MAX_AUTO_INSCRIPTIONS][AUTOINS_MATCH_LEN];
char auto_inscription_tag[MAX_AUTO_INSCRIPTIONS][AUTOINS_TAG_LEN];
bool auto_inscription_autopickup[MAX_AUTO_INSCRIPTIONS];
bool auto_inscription_autodestroy[MAX_AUTO_INSCRIPTIONS];
bool auto_inscription_force[MAX_AUTO_INSCRIPTIONS];
#ifdef REGEX_SEARCH
bool auto_inscription_invalid[MAX_AUTO_INSCRIPTIONS];
#endif

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
bool (*sound_hook)(int sound, int type, int vol, s32b player_id, int dist_x, int dist_y);
void (*sound_ambient_hook)(int sound_ambient);
void (*sound_weather_hook)(int sound);
void (*sound_weather_hook_vol)(int sound, int vol);
bool (*music_hook)(int music), (*music_hook_vol)(int music, char music_vol);
int cfg_audio_rate = 44100, cfg_max_channels = 32, cfg_audio_buffer = 1024;
int music_cur = -1, music_cur_song = -1, music_next = -1, music_next_song = -1, weather_channel = -1, weather_current = -1, weather_current_vol = -1, weather_channel_volume = 100, ambient_channel = -1, ambient_current = -1, ambient_channel_volume = 100;
char music_vol = 100;
int weather_sound_change, weather_fading, ambient_fading;
bool cfg_audio_master = TRUE, cfg_audio_music = TRUE, cfg_audio_sound = TRUE, cfg_audio_weather = TRUE, weather_resume = FALSE, ambient_resume = FALSE;
int cfg_audio_master_volume = AUDIO_VOLUME_DEFAULT, cfg_audio_music_volume = AUDIO_VOLUME_DEFAULT, cfg_audio_sound_volume = AUDIO_VOLUME_DEFAULT, cfg_audio_weather_volume = AUDIO_VOLUME_DEFAULT;
 #if 1 /* WEATHER_VOL_PARTICLES */
int weather_vol_smooth, weather_vol_smooth_anti_oscill, weather_smooth_avg[20];
 #endif
int grid_weather_volume = 100, grid_ambient_volume = 100, grid_weather_volume_goal = 100, grid_ambient_volume_goal = 100, grid_weather_volume_step, grid_ambient_volume_step;

/* sounds that are hard-coded on client-side, because they won't be transmitted from the server: */
int bell_sound_idx = -1, page_sound_idx = -1, warning_sound_idx = -1, rain1_sound_idx = -1, rain2_sound_idx = -1, snow1_sound_idx = -1, snow2_sound_idx = -1, browse_sound_idx = -1, browsebook_sound_idx = -1, thunder_sound_idx = -1, browseinven_sound_idx = -1;

/* optimization options */
bool sound_hint = TRUE;

/* Don't cache audio */
bool no_cache_audio = FALSE;

bool skip_received_music = FALSE;
#endif
bool wind_noticable = FALSE; /* This is now also used for lamp-flickering, not just for sound */
int weather_particles_seen; /* ..and this is used for wind_noticable! */

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
char artifact_list_activation[MAX_A_IDX][80];
/* For artifact lore */
char kind_list_name[MAX_K_IDX][80];
int kind_list_tval[MAX_K_IDX], kind_list_sval[MAX_K_IDX], kind_list_rarity[MAX_K_IDX], kind_list_idx = 0;
char kind_list_char[MAX_K_IDX], kind_list_attr[MAX_K_IDX];

/* For screenshots, to unmap custom fonts back to normally readable characters */
char monster_mapping_org[MAX_R_IDX + 1];
struct u32b_char_dict_t *monster_mapping_mod = NULL;
char floor_mapping_org[MAX_F_IDX + 1];
struct u32b_char_dict_t *floor_mapping_mod = NULL;

/* for DONT_CLEAR_TOPLINE_IF_AVOIDABLE */
char last_prompt[MSG_LEN] = { 0 };
bool last_prompt_macro = FALSE;

int screen_wid = SCREEN_WID, screen_hgt = SCREEN_HGT;
bool bigmap_hint = TRUE, firstrun = TRUE;
#ifndef GLOBAL_BIG_MAP
bool global_big_map_hold = FALSE;
#endif
bool in_game = FALSE;
bool rand_term_lamp;
int rand_term_lamp_ticks;

int minimap_posx = -1, minimap_posy, minimap_selx = -1, minimap_sely, minimap_yoff;
byte minimap_attr, minimap_selattr;
char32_t minimap_char, minimap_selchar;

/* To suppress positive confirmation messages on automated char dumps/screen shots */
bool silent_dump = FALSE;
/* For cleaner equip-display -- using a mimic/bat form that cannot use weapons? */
bool equip_no_weapon = FALSE;

bool auto_reincarnation = FALSE;
char macro_trigger_exclusive[MAX_CHARS];
bool macro_processing_exclusive = FALSE;

/* To make graphics char remappings easier and there is no need to update mapping files when MAX_FONT_CHAR changes. */
char32_t char_map_offset = 0;

/* Default color map */
/* These can be overriden using TomeNET.ini or .tomenetrc */
/* The colors are stored as 32-bit integers, .e.g. #112233 -> 0x112233 */
u32b client_color_map[CLIENT_PALETTE_SIZE] = {
	0x000000,	/* BLACK */
	0xffffff,	/* WHITE */
	0x9d9d9d,	/* GRAY */
	0xff8d00,	/* ORANGE */
	0xb70000,	/* RED */
	0x009d44,	/* GREEN */
	0x0000ff,	/* BLUE */
	0x8d6600,	/* UMBER */
	0x666666,	/* DARK GRAY */
	0xcdcdcd,	/* LIGHT GRAY */
	0xaf00ff,	/* VIOLET */
	0xffff00,	/* YELLOW */
	0xff3030,	/* LIGHT RED */
	0x00ff00,	/* LIGHT GREEN */
	0x00ffff,	/* LIGHT BLUE */
	0xc79d55,	/* LIGHT UMBER */
#ifdef EXTENDED_COLOURS_PALANIM
	/* clone them again, for palette animation */
	0x000000,	/* BLACK */
	0xffffff,	/* WHITE */
	0x9d9d9d,	/* GRAY */
	0xff8d00,	/* ORANGE */
	0xb70000,	/* RED */
	0x009d44,	/* GREEN */
	0x0000ff,	/* BLUE */
	0x8d6600,	/* UMBER */
	0x666666,	/* DARK GRAY */
	0xcdcdcd,	/* LIGHT GRAY */
	0xaf00ff,	/* VIOLET */
	0xffff00,	/* YELLOW */
	0xff3030,	/* LIGHT RED */
	0x00ff00,	/* LIGHT GREEN */
	0x00ffff,	/* LIGHT BLUE */
	0xc79d55,	/* LIGHT UMBER */
#endif
};
#ifdef EXTENDED_BG_COLOURS
u32b client_ext_color_map[TERMX_AMT][2] = {
	{ 0xffffff, 0x112288 },	/* experimental TERMX_BLUE */
	{ 0xffffff, 0x007700 },
	{ 0xffffff, 0x770000 },
	{ 0xffffff, 0x777700 },
	{ 0xffffff, 0x555555 },
	{ 0xffffff, 0xBBBBBB },
	{ 0xffffff, 0x333388 },
};
#endif
u32b client_color_map_org[CLIENT_PALETTE_SIZE];

/* For Deuteranopia */
u32b client_color_map_deu[CLIENT_PALETTE_SIZE] = {
	0x000000,	/* BLACK */
	0xffffff,	/* WHITE */
	0x9d9d9d,	/* GRAY */
	0xdbd100,	/* ORANGE (13) */
	0x920000,	/* RED (11) */
	0x490092,	/* GREEN (6) */
	0x006ddb,	/* BLUE (7) */
	0x924900,	/* UMBER (12) */
	0x666666,	/* DARK GRAY */
	0xcdcdcd,	/* LIGHT GRAY */
	0xb6dbff,	/* VIOLET (10) */
	0xffffa4,	/* YELLOW (16*) */
	0x24ff24,	/* LIGHT RED (14) */
	0xb66dff,	/* LIGHT GREEN (8) */
	0x6db6ff,	/* LIGHT BLUE (9) */
	0xffff6d,	/* LIGHT UMBER (15) */
#ifdef EXTENDED_COLOURS_PALANIM
	/* clone them again, for palette animation */
	0x000000,	/* BLACK */
	0xffffff,	/* WHITE */
	0x9d9d9d,	/* GRAY */
	0xdbd100,	/* ORANGE (13) */
	0x920000,	/* RED (11) */
	0x490092,	/* GREEN (6) */
	0x006ddb,	/* BLUE (7) */
	0x924900,	/* UMBER (12) */
	0x666666,	/* DARK GRAY */
	0xcdcdcd,	/* LIGHT GRAY */
	0xb6dbff,	/* VIOLET (10) */
	0xffffa4,	/* YELLOW (16*) */
	0x24ff24,	/* LIGHT RED (14) */
	0xb66dff,	/* LIGHT GREEN (8) */
	0x6db6ff,	/* LIGHT BLUE (9) */
	0xffff6d,	/* LIGHT UMBER (15) */
#endif
};

/* For Protanopia (same as Deuteranopia atm) */
u32b client_color_map_pro[CLIENT_PALETTE_SIZE] = {
	0x000000,	/* BLACK */
	0xffffff,	/* WHITE */
	0x9d9d9d,	/* GRAY */
	0xdbd100,	/* ORANGE (13) */
	0x920000,	/* RED (11) */
	0x490092,	/* GREEN (6) */
	0x006ddb,	/* BLUE (7) */
	0x924900,	/* UMBER (12) */
	0x666666,	/* DARK GRAY */
	0xcdcdcd,	/* LIGHT GRAY */
	0xb6dbff,	/* VIOLET (10) */
	0xffffa4,	/* YELLOW (16*) */
	0x24ff24,	/* LIGHT RED (14) */
	0xb66dff,	/* LIGHT GREEN (8) */
	0x6db6ff,	/* LIGHT BLUE (9) */
	0xffff6d,	/* LIGHT UMBER (15) */
#ifdef EXTENDED_COLOURS_PALANIM
	/* clone them again, for palette animation */
	0x000000,	/* BLACK */
	0xffffff,	/* WHITE */
	0x9d9d9d,	/* GRAY */
	0xdbd100,	/* ORANGE (13) */
	0x920000,	/* RED (11) */
	0x490092,	/* GREEN (6) */
	0x006ddb,	/* BLUE (7) */
	0x924900,	/* UMBER (12) */
	0x666666,	/* DARK GRAY */
	0xcdcdcd,	/* LIGHT GRAY */
	0xb6dbff,	/* VIOLET (10) */
	0xffffa4,	/* YELLOW (16*) */
	0x24ff24,	/* LIGHT RED (14) */
	0xb66dff,	/* LIGHT GREEN (8) */
	0x6db6ff,	/* LIGHT BLUE (9) */
	0xffff6d,	/* LIGHT UMBER (15) */
#endif
};

/* For Tritanopia */
u32b client_color_map_tri[CLIENT_PALETTE_SIZE] = {
	0x000000,	/* BLACK */
	0xffffff,	/* WHITE */
	0x9d9d9d,	/* GRAY */
	0x924900,	/* ORANGE (12) */
	0x920000,	/* RED (11) */
	0x004949,	/* GREEN (2) */
	0x6db6ff,	/* BLUE (9) */
	0x490092,	/* UMBER (6) */
	0x666666,	/* DARK GRAY */
	0xcdcdcd,	/* LIGHT GRAY */
	0xffb677,	/* VIOLET (5) */
	0xffff6d,	/* YELLOW (15) */
	0xff6db6,	/* LIGHT RED (4 /13) */
	//0x009292,	/* LIGHT GREEN (3 /7) */
	0x24ff24,	/* LIGHT GREEN (14) */
	0xb6dbff,	/* LIGHT BLUE (10) */
	//0x8036c9,	/* LIGHT UMBER (16*) */
	0xb66dff,	/* LIGHT UMBER (8) */
#ifdef EXTENDED_COLOURS_PALANIM
	/* clone them again, for palette animation */
	0x000000,	/* BLACK */
	0xffffff,	/* WHITE */
	0x9d9d9d,	/* GRAY */
	0x924900,	/* ORANGE (12) */
	0x920000,	/* RED (11) */
	0x004949,	/* GREEN (2) */
	0x6db6ff,	/* BLUE (9) */
	0x490092,	/* UMBER (6) */
	0x666666,	/* DARK GRAY */
	0xcdcdcd,	/* LIGHT GRAY */
	0xffb677,	/* VIOLET (5) */
	0xffff6d,	/* YELLOW (15) */
	0xff6db6,	/* LIGHT RED (4 /13) */
	//0x009292,	/* LIGHT GREEN (3 /7) */
	0x24ff24,	/* LIGHT GREEN (14) */
	0xb6dbff,	/* LIGHT BLUE (10) */
	//0x8036c9,	/* LIGHT UMBER (16*) */
	0xb66dff,	/* LIGHT UMBER (8) */
#endif
};

const char colour_name[BASE_PALETTE_SIZE][9] = {
	"Black", "White", "Slate", "Orange",
	"Red", "Green", "Blue", "Umber",
	"D.Grey", "L.Grey", "Violet", "Yellow",
	"L.Red", "L.Green", "Cyan", "L.Umber",
};

bool lighterdarkblue = FALSE;


/* Allow code in src/common/ to dynamically check that this is the client */
bool is_client_side = TRUE;

#ifdef RETRY_LOGIN
bool rl_connection_destructible = FALSE, rl_connection_destroyed = FALSE, rl_password = FALSE;
byte rl_connection_state = 0;
bool player_pref_files_loaded = FALSE;
#endif

/* For in-client guide search */
int guide_lastline, guide_errno;
char guide_race[64][MAX_CHARS];
char guide_class[64][MAX_CHARS];
char guide_skill[128][MAX_CHARS];
char guide_school[64][MAX_CHARS];
char guide_spell[256][MAX_CHARS];
int guide_races = 0, guide_classes = 0, guide_skills = 0, guide_schools = 0, guide_spells = 0;
char guide_chapter[256][MAX_CHARS], guide_chapter_no[256][8];
int guide_chapters, guide_endofcontents;

#ifdef WINDOWS
bool win_dontmoveuser = FALSE;
#endif

byte showing_inven = FALSE, showing_equip = FALSE;

int hp_max, hp_cur;
bool hp_bar;
bool hp_boosted = FALSE;
int mp_max, mp_cur;
bool mp_bar;
int st_max, st_cur;
bool st_bar;

char cfg_soundpackfolder[1024];
char cfg_musicpackfolder[1024];

bool within_cmd_player = FALSE;
int within_cmd_player_ticks;

int NumPlayers = 0;
char playerlist[1000][MAX_CHARS_WIDE * 2];

byte col_raindrop = TERM_BLUE, col_snowflake = TERM_WHITE, col_sandgrain = TERM_L_UMBER;
char c_sandgrain = '+';
bool custom_font_warning = FALSE;
#ifdef GUIDE_BOOKMARKS
int bookmark_line[GUIDE_BOOKMARKS];
char bookmark_name[GUIDE_BOOKMARKS][60];
#endif
unsigned char lamp_fainting = 0;
bool insanity_death = FALSE;

char screenshot_filename[1024] = { 0 };
byte screenshot_height = 0;
char whats_under_your_feet[ONAME_LEN];

//#ifdef ENABLE_JUKEBOX
int curmus_timepos = -1, oldticks = -1, curmus_x, curmus_y, curmus_attr;
bool jukebox_screen = FALSE;
//#endif
int oldticksds = -1;

bool map_town = FALSE;
#ifdef META_PINGS
int meta_pings_servers = 0, meta_pings_ticks = -1, meta_pings_server_duplicate[META_PINGS], meta_pings_result[META_PINGS];
char meta_pings_server_name[META_PINGS][MAX_CHARS];
bool  meta_pings_stuck[META_PINGS];
 #ifdef WINDOWS
char meta_pings_xpath[1024];
 #endif
 #ifdef META_PINGS_CREATEFILE
HANDLE fhan[META_PINGS] = { NULL }; //it's *void (aka PVOID)
SECURITY_ATTRIBUTES sa[META_PINGS];
STARTUPINFO si[META_PINGS];
PROCESS_INFORMATION pi[META_PINGS];
 #endif
#endif

bool fullscreen_weather = FALSE; //RAINY_TOMB
bool force_cui = FALSE;
int food_warn_once_timer;

int prev_huge_cmp = -1, prev_huge_mmp = -1;
int prev_huge_csn = -1, prev_huge_msn = -1;
int prev_huge_chp = -1, prev_huge_mhp = -1;
