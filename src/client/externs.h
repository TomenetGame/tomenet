/* $Id$ */
/* File: externs.h */

/* Purpose: extern declarations (variables and functions) */

/*
 * Note that some files have their own header files
 * (z-virt.h, z-util.h, z-form.h, term.h, random.h)
 */


/*
 * Not-so-Automatically generated "variable" declarations
 */

/* tables.c */
extern s16b ddx[10];
extern s16b ddy[10];
extern char hexsym[16];
extern owner_type owners[MAX_STORES][MAX_OWNERS];
extern player_race race_info[MAX_RACES];
extern player_class class_info[MAX_CLASS];
extern option_type option_info[];
extern cptr stat_names[6];
extern cptr stat_names_reduced[6];
extern cptr ang_term_name[8];
extern cptr window_flag_desc[32];
extern cptr monster_spells4[32];
extern cptr monster_spells5[32];
extern cptr monster_spells6[32];



/* variable.c */
extern char nick[80];
extern char pass[80];
extern char svname[80];
extern char path[1024];

extern char real_name[80];

extern char server_name[80];

extern char message_history[MSG_HISTORY_MAX][80];
//extern byte hist_start;
extern byte hist_end;
extern bool hist_looped;

extern object_type inventory[INVEN_TOTAL];
extern char inventory_name[INVEN_TOTAL][80];

extern store_type store;
extern owner_type store_owner;
extern int store_prices[STORE_INVEN_MAX];
extern char store_names[STORE_INVEN_MAX][80];
extern s16b store_num;

extern char spell_info[MAX_REALM][9][9][80];

extern char party_info[160];

extern setup_t Setup;
extern client_setup_t Client_setup;

extern bool shopping;

extern s16b last_line_info;
extern s16b cur_line;
extern s16b max_line;

extern player_type player;
extern player_type *p_ptr;
extern s32b exp_adv;

extern s16b command_see;
extern s16b command_gap;
extern s16b command_wrk;

extern bool item_tester_full;
extern byte item_tester_tval;
extern bool (*item_tester_hook)(object_type *o_ptr);

extern int special_line_type;

extern bool inkey_base;
extern bool inkey_scan;
extern bool inkey_flag;

extern s16b macro__num;
extern cptr *macro__pat;
extern cptr *macro__act;
extern bool *macro__cmd;
extern char *macro__buf;

extern u16b message__next;
extern u16b message__last;
extern u16b message__head;
extern u16b message__tail;
extern u16b *message__ptr;
extern char *message__buf;




extern bool msg_flag;

extern term *ang_term[8];
extern u32b window_flag[8];

extern byte color_table[256][4];

extern cptr ANGBAND_SYS;

extern byte keymap_cmds[128];
extern byte keymap_dirs[128];

extern s16b command_cmd;
extern s16b command_dir;

extern s16b race;
extern s16b class;
extern s16b sex;

extern s16b class_extra;

extern s16b stat_order[6];

extern bool topline_icky;
extern short screen_icky;
extern bool party_mode;


extern cptr race_title[];
extern cptr class_title[];

extern cptr ANGBAND_DIR;
extern cptr ANGBAND_DIR_APEX;
extern cptr ANGBAND_DIR_BONE;
extern cptr ANGBAND_DIR_DATA;
extern cptr ANGBAND_DIR_EDIT;
extern cptr ANGBAND_DIR_FILE;
extern cptr ANGBAND_DIR_HELP;
extern cptr ANGBAND_DIR_INFO;
extern cptr ANGBAND_DIR_SAVE;
extern cptr ANGBAND_DIR_USER;
extern cptr ANGBAND_DIR_XTRA;
extern cptr ANGBAND_DIR_SCPT;

extern bool use_graphics;
extern bool use_sound;

extern bool rogue_like_commands;
extern bool quick_messages;
extern bool other_query_flag;
extern bool carry_query_flag;
extern bool use_old_target;
extern bool always_pickup;
extern bool always_repeat;
extern bool depth_in_feet;
extern bool stack_force_notes;
extern bool stack_force_costs;
extern bool show_labels;
extern bool show_weights;
extern bool show_choices;
extern bool show_details;
extern bool ring_bell;
extern bool use_color;

extern bool find_ignore_stairs;
extern bool find_ignore_doors;
extern bool find_cut;
extern bool find_examine;
extern bool disturb_move;
extern bool disturb_near;
extern bool disturb_panel;
extern bool disturb_state;
extern bool disturb_minor;
extern bool disturb_other;
extern bool alert_hitpoint;
extern bool alert_failure;

extern bool auto_haggle;
extern bool auto_scum;
extern bool stack_allow_items;
extern bool stack_allow_wands;
extern bool expand_look;
extern bool expand_list;
extern bool view_perma_grids;
extern bool view_torch_grids;
extern bool dungeon_align;
extern bool dungeon_stair;
extern bool flow_by_sound;
extern bool flow_by_smell;
extern bool track_follow;
extern bool track_target;
extern bool smart_learn;
extern bool smart_cheat;

extern bool view_reduce_lite;
extern bool view_reduce_view;
extern bool avoid_abort;
extern bool avoid_other;
extern bool flush_failure;
extern bool flush_disturb;
extern bool flush_command;
extern bool fresh_before;
extern bool fresh_after;
extern bool fresh_message;
extern bool compress_savefile;
extern bool hilite_player;
extern bool view_yellow_lite;
extern bool view_bright_lite;
extern bool view_granite_lite;
extern bool view_special_lite;

extern u32b cfg_game_port;

extern skill_type s_info[MAX_SKILLS];


/*
 * Not-so-Automatically generated "function declarations"
 */

/* c-birth.c */
extern void get_char_name(void);
extern void get_char_info(void);
extern bool get_server_name(void);

/* c-cmd.c */
extern void process_command(void);
extern void do_cmd_skill();
extern void cmd_tunnel(void);
extern void cmd_walk(void);
extern void cmd_king(void);
extern void cmd_run(void);
extern void cmd_stay(void);
extern void cmd_map(void);
extern void cmd_locate(void);
extern void cmd_search(void);
extern void cmd_toggle_search(void);
extern void cmd_rest(void);
extern void cmd_go_up(void);
extern void cmd_go_down(void);
extern void cmd_open(void);
extern void cmd_close(void);
extern void cmd_bash(void);
extern void cmd_disarm(void);
extern void cmd_inven(void);
extern void cmd_equip(void);
extern void cmd_drop(void);
extern void cmd_drop_gold(void);
extern void cmd_wield(void);
extern void cmd_observe(void);
extern void cmd_take_off(void);
extern void cmd_destroy(void);
extern void cmd_inscribe(void);
extern void cmd_uninscribe(void);
extern void cmd_steal(void);
extern void cmd_quaff(void);
extern void cmd_read_scroll(void);
extern void cmd_aim_wand(void);
extern void cmd_use_staff(void);
extern void cmd_zap_rod(void);
extern void cmd_refill(void);
extern void cmd_eat(void);
extern void cmd_activate(void);
extern int cmd_target(void);
extern int cmd_target_friendly(void);
extern void cmd_look(void);
extern void cmd_character(void);
extern void cmd_artifacts(void);
extern void cmd_uniques(void);
extern void cmd_players(void);
extern void cmd_high_scores(void);
extern void cmd_help(void);
extern void cmd_message(void);
extern void cmd_party(void);
extern void cmd_fire(void);
extern void cmd_throw(void);
extern void cmd_browse(void);
extern void cmd_study(void);
extern void cmd_cast(void);
extern void cmd_pray(void);
extern void cmd_mimic(void);
extern void cmd_fight(void);
extern void cmd_ghost(void);
extern void cmd_mind(void);
extern void cmd_load_pref(void);
extern void cmd_redraw(void);
extern void cmd_purchase_house(void);
extern void cmd_suicide(void);
extern void cmd_master(void);
extern void cmd_master_aux_level(void);
extern void cmd_master_aux_build(void);
extern void cmd_master_aux_summon(void);
extern void cmd_spike(void);

/* c-files.c */
extern void text_to_ascii(char *buf, cptr str);
extern FILE *my_fopen(cptr file, cptr mode);
extern errr my_fclose(FILE *fff);
extern void init_file_paths(char *path);
extern errr process_pref_file(cptr buf);
extern errr process_pref_file_aux(char *buf);
extern void show_motd(void);
extern void peruse_file(void);
extern errr my_fgets(FILE *fff, char *buf, huge n);

/* c-init.c */
extern void initialize_all_pref_files(void);
extern void client_init(char *argv1, bool skip);

/* c-inven.c */
extern s16b index_to_label(int i);
extern bool item_tester_okay(object_type *o_ptr);
extern bool c_get_item(int *cp, cptr pmt, bool equip, bool inven, bool floor);

/* c-util.c */
extern void move_cursor(int row, int col);
extern void flush(void);
extern void flush_now(void);
extern void macro_add(cptr pat, cptr act, bool cmd_flag);
extern char inkey(void);
extern void keymap_init(void);
extern void bell(void);
extern void c_prt(byte attr, cptr str, int row, int col);
extern void prt(cptr str, int row, int col);
extern bool get_string(cptr prompt, char *buf, int len);
extern bool get_com(cptr prompt, char *command);
extern void request_command(bool shopping);
extern bool get_dir(int *dp);
extern void c_put_str(byte attr, cptr str, int row, int col);
extern void put_str(cptr str, int row, int col);
extern bool get_check(cptr prompt);
extern s16b message_num(void);
extern cptr message_str(s16b age);
extern void c_message_add(cptr msg);
extern void c_msg_print(cptr msg);
extern s32b c_get_quantity(cptr prompt, int max);
extern errr path_build(char *buf, int max, cptr path, cptr file);
extern bool askfor_aux(char *buf, int len, char private);
extern void clear_from(int row);
extern void prt_num(cptr header, int num, int row, int col, byte color);
extern void prt_lnum(cptr header, s32b num, int row, int col, byte color);
extern void interact_macros(void);
extern void do_cmd_options(void);

/* c-spell.c */
extern void show_browse(int book);
extern void do_study(int book);
extern void do_cast(int book);
extern void do_pray(int book);
extern void do_fight(int book);
extern void do_ghost(void);
extern void do_mimic();

/* c-store.c */
extern void display_inventory(void);
extern void display_store(void);

/* c-xtra1.c */
extern void prt_stat(int stat, int max, int cur);
extern void prt_title(cptr title);
extern void prt_level(int level, int max, int cur, int adv);
extern void prt_gold(int gold);
extern void prt_ac(int ac);
extern void prt_hp(int max, int cur);
extern void prt_sp(int max, int cur);
#ifdef NEW_DUNGEON
extern void prt_depth(int x, int y, int z, bool town);
#else
extern void prt_depth(int depth);
#endif
extern void prt_hunger(int food);
extern void prt_blind(bool blind);
extern void prt_confused(bool confused);
extern void prt_afraid(bool fear);
extern void prt_poisoned(bool poisoned);
extern void prt_state(bool paralyzed, bool searching, bool resting);
extern void prt_speed(int speed);
extern void prt_study(bool study);
extern void prt_cut(int cut);
extern void prt_stun(int stun);
extern void prt_basic(void);
extern void health_redraw(int num, byte attr);
extern void show_inven(void);
extern void show_equip(void);
extern void fix_message(void);
extern void display_player(int hist);
extern void window_stuff(void);
extern void prt_sane(void);

/* c-xtra2.c */
extern void do_cmd_messages(void);
extern void do_cmd_messages_chatonly(void);

/* client.c */

/* netclient.c */
extern void do_mail(void);
extern int ticks;
extern void update_ticks();
extern void do_keepalive();
extern int Net_setup(void);
extern int Net_verify(char *real, char *nick, char *pass, int sex, int race, int class);
extern int Net_init(char *server, int port);
extern void Net_cleanup(void);
extern int Net_flush(void);
extern int Net_fd(void);
extern int Net_start(void);
extern int Net_input(void);
extern int Flush_queue(void);

extern int Send_mind();
extern int Send_mimic(int spell);
extern int Send_search(void);
extern int Send_walk(int dir);
extern int Send_run(int dir);
extern int Send_drop(int item, int amt);
extern int Send_drop_gold(s32b amt);
extern int Send_tunnel(int dir);
extern int Send_stay(void);
extern int Send_toggle_search(void);
extern int Send_rest(void);
extern int Send_go_up(void);
extern int Send_go_down(void);
extern int Send_open(int dir);
extern int Send_close(int dir);
extern int Send_bash(int dir);
extern int Send_disarm(int dir);
extern int Send_wield(int item);
extern int Send_observe(int item);
extern int Send_take_off(int item);
extern int Send_destroy(int item, int amt);
extern int Send_inscribe(int item, cptr buf);
extern int Send_uninscribe(int item);
extern int Send_steal(int dir);
extern int Send_quaff(int item);
extern int Send_read(int item);
extern int Send_aim(int item, int dir);
extern int Send_use(int item);
extern int Send_zap(int item);
extern int Send_fill(int item);
extern int Send_eat(int item);
extern int Send_activate(int item);
extern int Send_target(int dir);
extern int Send_target_friendly(int dir);
extern int Send_look(int dir);
extern int Send_msg(cptr message);
//extern int Send_fire(int item, int dir);
extern int Send_fire(int dir);
extern int Send_throw(int item, int dir);
extern int Send_item(int item);
extern int Send_gain(int book, int spell);
extern int Send_cast(int book, int spell);
extern int Send_pray(int book, int spell);
extern int Send_fight(int book, int spell);
extern int Send_ghost(int ability);
extern int Send_map(void);
extern int Send_locate(int dir);
extern int Send_store_purchase(int item, int amt);
extern int Send_store_sell(int item, int amt);
extern int Send_store_leave(void);
extern int Send_store_confirm(void);
extern int Send_redraw(void);
extern int Send_special_line(int type, int line);
extern int Send_party(s16b command, cptr buf);
extern int Send_guild(s16b command, cptr buf);
extern int Send_purchase_house(int dir);
extern int Send_suicide(void);
extern int Send_options(void);
extern int Send_master(s16b command, cptr buf);
extern int Send_clear_buffer(void);
extern int Send_King(byte type);
extern int Send_admin_house(int dir, cptr buf);
extern int Send_spike(int dir);
extern int Send_skill_mod(int i);


/* skills.c */
extern bool hack_do_cmd_skill_wait;


/*
 * Hack -- conditional (or "bizarre") externs
 */

#ifdef SET_UID
/* util.c */
extern void user_name(char *buf, int id);
#endif

#ifndef HAS_MEMSET
/* util.c */
extern char *memset(char*, int, huge);
#endif

#ifndef HAS_STRICMP
/* util.c */
extern int stricmp(cptr a, cptr b);
#endif

#ifdef MACINTOSH
/* main-mac.c */
/* extern void main(void); */
#endif

#ifdef WINDOWS
/* main-win.c */
/* extern int FAR PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, ...); */
#endif

extern char	*longVersion;
extern char	*shortVersion;
