/* $Id$ */
#include "angband.h"

/* Client global variables */

char nick[80]="";
char pass[80]="";
char svname[80]="";
//char path[1024]="";
char path[1024]=DEFAULT_PATH;

char real_name[80]="";

char server_name[80]="";

char message_history[MSG_HISTORY_MAX][80];	/* history for chat, slash-cmd etc. */
//byte hist_start = 0;
byte hist_end = 0;
bool hist_looped = FALSE;

object_type inventory[INVEN_TOTAL];	/* The client-side copy of the inventory */
char inventory_name[INVEN_TOTAL][80];	/* The client-side copy of the inventory names */

store_type store;			/* The general info about the current store */
owner_type store_owner;			/* The current owner of the store */
int store_prices[STORE_INVEN_MAX];			/* The prices of the items in the store */
char store_names[STORE_INVEN_MAX][80];		/* The names of the stuff in the store */
s16b store_num;				/* The current store number */

/* XXX Mergin for future expansion -- this should be handled in Net_setup */
char spell_info[MAX_REALM + 9][9][9][80];		/* Spell information */

char party_info[160];			/* Information about your party */

setup_t Setup;				/* The information given to us by the server */
client_setup_t Client_setup;		/* The information we give to the server */

bool shopping;				/* Are we in a store? */

s16b last_line_info;			/* Last line of info we've received */
s16b max_line;				/* Maximum amount of "special" info */
s16b cur_line;				/* Current displayed line of "special" info */

player_type player;			/* The client-side copy of some of the player information */
player_type *p_ptr = &player;

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

s16b macro__num;
cptr *macro__pat;
cptr *macro__act;
bool *macro__cmd;
char *macro__buf;

u16b message__next;
u16b message__last;
u16b message__head;
u16b message__tail;
u16b *message__ptr;
char *message__buf;


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

s16b stat_order[6];			/* Desired order of stats */

s16b class_extra;

bool topline_icky;
short screen_icky;
bool party_mode;

#if 0	// This should be gone for good		- Jir -
cptr race_title[] = {
	"Human", "Half-elf", "Elf", "Hobbit",
	"Gnome", "Dwarf", "Half-Orc", "Half-Troll",
	"Dunadan", "High-elf", "Yeek", "Goblin",
	"Ent", "DragonRider",
	};

cptr class_title[] = {
	"Warrior", "Warlock", "Priest", "Rogue",
	"Ranger", "Paladin", "Sorceror", "Mimic",
	"Unbeliever", "Archer", "Monk", "Telepath",
	};
#endif	// 0

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



bool rogue_like_commands;
bool quick_messages;
bool other_query_flag;
bool carry_query_flag;
bool use_old_target;
bool always_pickup;
bool always_repeat;
bool depth_in_feet;
bool stack_force_notes;
bool stack_force_costs;
bool show_labels;
bool show_weights;
bool show_choices;
bool show_details;
bool ring_bell;
bool use_color;

bool find_ignore_stairs;
bool find_ignore_doors;
bool find_cut;
bool find_examine;
bool disturb_move;
bool disturb_near;
bool disturb_panel;
bool disturb_state;
bool disturb_minor;
bool disturb_other;
bool alert_hitpoint;
bool alert_failure;

bool auto_haggle;
bool auto_scum;
bool stack_allow_items;
bool stack_allow_wands;
bool expand_look;
bool expand_list;
bool view_perma_grids;
bool view_torch_grids;
bool dungeon_align;
bool dungeon_stair;
bool flow_by_sound;
bool flow_by_smell;
bool track_follow;
bool track_target;
bool smart_learn;
bool smart_cheat;

bool view_reduce_lite;
bool view_reduce_view;
bool avoid_abort;
bool avoid_other;
bool flush_failure;
bool flush_disturb;
bool flush_command;
bool fresh_before;
bool fresh_after;
bool fresh_message;
bool compress_savefile;
bool hilite_player;
bool view_yellow_lite;
bool view_bright_lite;
bool view_granite_lite;
bool view_special_lite;

//bool dummy_option;	/* for options not used in client */
bool easy_open;
bool easy_disarm;
bool easy_tunnel;
bool auto_destroy;
bool auto_inscribe;

u32b cfg_game_port = 18348; 

skill_type s_info[MAX_SKILLS];
