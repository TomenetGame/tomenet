
/* File: externs.h */

/* Purpose: extern declarations (variables and functions) */

/*
 * Note that some files have their own header files
 * (z-virt.h, z-util.h, z-form.h, term.h, random.h)
 */


/*
 * Automatically generated "variable" declarations
 */

/* #include "netserver.h" */

/* common */
extern int color_char_to_attr(char c);

extern struct sfunc csfunc[];

/* netserver.c */
/*extern long Id;*/
extern int NumPlayers;
extern int ConsoleSocket;
extern void process_pending_commands(int Ind);
extern bool player_is_king(int Ind);
extern void end_mind(int Ind, bool update);

/* randart.c */
extern artifact_type *ego_make(object_type *o_ptr);
extern artifact_type *randart_make(object_type *o_ptr);
extern void randart_name(object_type *o_ptr, char *buffer);
extern void add_random_ego_flag(artifact_type *a_ptr, int fego, bool *limit_blows, s16b dun_level);
extern void random_resistance (artifact_type * a_ptr, bool is_scroll, int specific);
extern void dragon_resist(artifact_type * a_ptr);

/* tables.c */
extern s16b ddd[9];
extern s16b ddx[10];
extern s16b ddy[10];
extern s16b ddx_ddd[9];
extern s16b ddy_ddd[9];
extern char hexsym[16];
extern byte adj_val_min[];
extern byte adj_val_max[];
extern byte adj_mag_study[];
extern byte adj_mag_mana[];
extern byte adj_mag_fail[];
extern byte adj_mag_stat[];
extern byte adj_chr_gold[];
extern byte adj_int_dev[];
extern byte adj_wis_sav[];
extern byte adj_dex_dis[];
extern byte adj_int_dis[];
extern byte adj_dex_ta[];
extern byte adj_str_td[];
extern byte adj_dex_th[];
extern byte adj_str_th[];
extern byte adj_str_wgt[];
extern byte adj_str_hold[];
extern byte adj_str_dig[];
extern byte adj_str_blow[];
extern byte adj_dex_blow[];
extern byte adj_dex_safe[];
extern byte adj_con_fix[];
extern byte adj_con_mhp[];
extern byte blows_table[12][12];
extern owner_type owners[MAX_STORES][MAX_OWNERS];
extern byte extract_energy[256];
extern byte level_speeds[501];
extern s32b player_exp[PY_MAX_LEVEL];
extern player_race race_info[MAX_RACES];
extern player_class class_info[MAX_CLASS];
extern player_magic magic_info[MAX_CLASS];
extern magic_type ghost_spells[64];
extern u32b spell_flags[7][9][2];
extern cptr spell_names[8][64];
extern byte chest_traps[64];
extern cptr player_title[MAX_CLASS][PY_MAX_LEVEL/10];
extern magic_type innate_powers[96];
extern martial_arts ma_blows[MAX_MA];


/* variable.c */
extern char tdy[662];
extern char tdx[662];
extern char tdi[16];
extern cptr copyright[6];
extern byte version_major;
extern byte version_minor;
extern byte version_patch;
extern byte version_extra;
extern byte sf_major;
extern byte sf_minor;
extern byte sf_patch;
extern byte sf_extra;
extern u32b sf_xtra;
extern u32b sf_when;
extern u16b sf_lives;
extern u16b sf_saves;
extern bool server_generated;
extern bool server_dungeon;
extern bool server_state_loaded;
extern bool server_saved;
extern bool character_loaded;
extern u32b seed_flavor;
extern u32b seed_town;
extern s16b command_new;
extern s16b choose_default;
extern bool create_up_stair;
extern bool create_down_stair;
extern s16b num_repro;
extern s16b object_level;
extern s16b monster_level;
extern byte level_up_x(struct worldpos *wpos);
extern byte level_up_y(struct worldpos *wpos);
extern byte level_down_x(struct worldpos *wpos);
extern byte level_down_y(struct worldpos *wpos);
extern byte level_rand_x(struct worldpos *wpos);
extern byte level_rand_y(struct worldpos *wpos);
extern void new_level_up_x(struct worldpos *wpos, int x);
extern void new_level_up_y(struct worldpos *wpos, int y);
extern void new_level_down_x(struct worldpos *wpos, int x);
extern void new_level_down_y(struct worldpos *wpos, int y);
extern void new_level_rand_x(struct worldpos *wpos, int x);
extern void new_level_rand_y(struct worldpos *wpos, int y);
extern s32b turn;
extern s32b player_id;
extern u16b panic_save;
extern s16b signal_count;
extern bool inkey_base;
extern bool inkey_xtra;
extern bool inkey_scan;
extern bool inkey_flag;
extern s16b coin_type;
extern bool opening_chest;
extern bool scan_monsters;
extern bool scan_objects;
extern s32b o_nxt;
extern s32b m_nxt;
extern s32b o_max;
extern s32b m_max;
extern s32b o_top;
extern s32b m_top;
extern s32b t_nxt;
extern s32b t_max;
extern s32b t_top;
extern header *re_head; 
extern monster_ego *re_info;
extern char *re_name;

#if 0
extern bool cfg_report_to_meta;
extern char * cfg_meta_address;
extern char * cfg_bind_name;
extern char * cfg_console_password;
extern char * cfg_admin_wizard;
extern char * cfg_dungeon_master;
extern bool cfg_secret_dungeon_master;
extern s16b cfg_fps;
extern bool cfg_mage_hp_bonus;
extern s16b cfg_newbies_cannot_drop;
extern bool cfg_door_bump_open;
extern s32b cfg_preserve_death_level;
extern bool cfg_no_ghost;
extern s32b cfg_unique_respawn_time;
extern s32b cfg_unique_max_respawn_time;
extern s32b cfg_level_unstatic_chance;
extern s32b cfg_min_unstatic_level;
extern s32b cfg_retire_timer;
extern bool cfg_maximize;
extern s32b cfg_game_port;
extern s32b cfg_console_port;
extern int cfg_spell_interfere;
extern bool cfg_anti_arts_horde;
#endif

extern server_opts cfg;


extern bool use_color;
extern bool hilite_player;
extern bool avoid_abort;
extern bool avoid_other;
extern bool flush_disturb;
extern bool flush_failure;
extern bool flush_command;
extern bool fresh_before;
extern bool fresh_after;
extern bool fresh_message;
extern bool view_yellow_lite;
extern bool view_bright_lite;
extern bool view_granite_lite;
extern bool view_special_lite;
extern bool view_perma_grids;
extern bool view_torch_grids;
extern bool flow_by_sound;
extern bool flow_by_smell;
extern bool track_follow;
extern bool track_target;
extern bool stack_allow_items;
extern bool stack_allow_wands;
extern bool stack_force_notes;
extern bool stack_force_costs;
extern bool view_reduce_lite;
extern bool view_reduce_view;
extern bool auto_scum;
extern bool expand_look;
extern bool expand_list;
extern bool dungeon_align;
extern bool dungeon_stair;
extern bool smart_learn;
extern bool smart_cheat;
extern s16b hitpoint_warn;
extern player_type **Players;
extern party_type parties[MAX_PARTIES];
extern struct guild_type guilds[MAX_GUILDS];
extern house_type *houses;
extern u32b num_houses;
extern u32b house_alloc;
extern long GetInd[];
extern s16b quark__num;
extern cptr *quark__str;
extern s16b o_fast[MAX_O_IDX];
extern s16b m_fast[MAX_M_IDX];
extern s16b t_fast[MAX_TR_IDX];
extern cave_type ***cave;
extern wilderness_type wild_info[MAX_WILD_Y][MAX_WILD_X];
extern struct town_type *town;
extern u16b numtowns;
extern object_type *o_list;
extern monster_type *m_list;
extern trap_type *t_list;
extern quest q_list[MAX_Q_IDX];
extern s16b alloc_kind_size;
extern alloc_entry *alloc_kind_table;
extern s16b alloc_race_size;
extern alloc_entry *alloc_race_table;
extern byte tval_to_attr[128];
extern char tval_to_char[128];
extern byte keymap_cmds[128];
extern byte keymap_dirs[128];
extern byte color_table[256][4];
/*extern player_race *rp_ptr;
extern player_class *cp_ptr;
extern player_magic *mp_ptr;*/
extern header *v_head;
extern vault_type *v_info;
extern char *v_name;
extern char *v_text;
extern header *f_head;
extern feature_type *f_info;
extern char *f_name;
extern char *f_text;
extern header *k_head;
extern object_kind *k_info;
extern char *k_name;
extern char *k_text;
extern header *t_head;
extern trap_kind *t_info;
extern char *t_name;
extern char *t_text;
extern header *a_head;
extern artifact_type *a_info;
extern char *a_name;
extern char *a_text;
extern header *e_head;
extern ego_item_type *e_info;
extern char *e_name;
extern char *e_text;
extern header *r_head;
extern monster_race *r_info;
extern char *r_name;
extern char *r_text;
extern cptr ANGBAND_SYS;
extern cptr ANGBAND_DIR;
extern cptr ANGBAND_DIR_SCPT;
extern cptr ANGBAND_DIR_DATA;
extern cptr ANGBAND_DIR_GAME;
extern cptr ANGBAND_DIR_SAVE;
extern cptr ANGBAND_DIR_TEXT;
extern cptr ANGBAND_DIR_USER;
extern cptr MANGBAND_CFG;
extern bool item_tester_full;
extern byte item_tester_tval;
extern bool (*item_tester_hook)(object_type *o_ptr);
extern bool (*ang_sort_comp)(int Ind, vptr u, vptr v, int a, int b);
extern void (*ang_sort_swap)(int Ind, vptr u, vptr v, int a, int b);
extern bool (*get_mon_num_hook)(int r_idx);
extern bool (*get_obj_num_hook)(int k_idx);
extern bool (*master_move_hook)(int Ind, char * parms);

extern int artifact_bias;
extern char summon_kin_type;
/* variable.c ends here */

/*
 * Automatically generated "function declarations"
 */

/* birth.c */
extern bool player_birth(int Ind, cptr name, cptr pass, int conn, int race, int class, int sex, int stat_order[]);
extern bool confirm_admin(int Ind, cptr name, cptr pass);
extern void server_birth(void);
extern void admin_outfit(int Ind);

/* cave.c */
extern struct dungeon_type *getdungeon(struct worldpos *wpos);
extern bool can_go_up(struct worldpos *wpos);
extern bool can_go_down(struct worldpos *wpos);
extern bool istown(struct worldpos *wpos);
//extern bool inarea(struct worldpos *apos, struct worldpos *bpos);
extern int getlevel(struct worldpos *wpos);
extern void wpcopy(struct worldpos *dest, struct worldpos *src);
extern int distance(int y1, int x1, int y2, int x2);
extern bool player_can_see_bold(int Ind, int y, int x);
extern bool no_lite(int Ind);
extern void map_info(int Ind, int y, int x, byte *ap, char *cp);
extern void move_cursor_relative(int row, int col);
extern void print_rel(char c, byte a, int y, int x);
extern void note_spot(int Ind, int y, int x);
extern void new_players_on_depth(struct worldpos *wpos, int value, bool inc);
extern int players_on_depth(struct worldpos *wpos);
extern bool los(struct worldpos *wpos, int y1, int x1, int y2, int x2);
extern void note_spot_depth(struct worldpos *wpos, int y, int x);
extern void everyone_lite_spot(struct worldpos *wpos, int y, int x);
extern void everyone_forget_spot(struct worldpos *wpos, int y, int x);
extern cave_type **getcave(struct worldpos *wpos);
extern void lite_spot(int Ind, int y, int x);
extern void prt_map(int Ind);
extern void display_map(int Ind, int *cy, int *cx);
extern void do_cmd_view_map(int Ind);
extern void forget_lite(int Ind);
extern void update_lite(int Ind);
extern void forget_view(int Ind);
extern void update_view(int Ind);
extern void forget_flow(void);
extern void update_flow(void);
extern void map_area(int Ind);
extern void wiz_lite(int Ind);
extern void wiz_dark(int Ind);
extern void mmove2(int *y, int *x, int y1, int x1, int y2, int x2);
extern bool projectable(struct worldpos *wpos, int y1, int x1, int y2, int x2);
extern bool projectable_wall(struct worldpos *wpos, int y1, int x1, int y2, int x2);
extern void scatter(struct worldpos *wpos, int *yp, int *xp, int y, int x, int d, int m);
extern bool is_quest(struct worldpos *wpos);
extern void health_track(int Ind, int m_idx);
extern void update_health(int m_idx);
extern void recent_track(int r_idx);
extern void disturb(int Ind, int stop_search, int flush_output);
extern void update_players(void);

/* cmd1.c */
extern bool test_hit_fire(int chance, int ac, int vis);
extern bool test_hit_norm(int chance, int ac, int vis);
extern s16b critical_shot(int Ind, int weight, int plus, int dam);
extern s16b critical_norm(int Ind, int weight, int plus, int dam);
extern s16b tot_dam_aux(int Ind, object_type *o_ptr, int tdam, monster_type *m_ptr);
extern s16b tot_dam_aux_player(object_type *o_ptr, int tdam, player_type *p_ptr);
extern void search(int Ind);
extern void carry(int Ind, int pickup, int confirm);
extern void py_attack(int Ind, int y, int x, bool old);
extern void touch_zap_player(int Ind, int m_idx);
extern void do_nazgul(int Ind, int *k, int *num, monster_race *r_ptr, object_type *o_ptr);
extern void set_black_breath(int Ind);
extern void do_prob_travel(int Ind, int dir);
extern void move_player(int Ind, int dir, int do_pickup);
extern void run_step(int Ind, int dir);
extern void black_breath_infection(int Ind, int Ind2);
extern int see_wall(int Ind, int dir, int y, int x);

/* cmd2.c */
extern bool access_door(int Ind, struct dna_type *dna);
extern void do_cmd_own(int Ind);
extern void do_cmd_go_up(int Ind);
extern void do_cmd_go_down(int Ind);
extern void do_cmd_search(int Ind);
extern void do_cmd_toggle_search(int Ind);
extern void do_cmd_open(int Ind, int dir);
extern void do_cmd_close(int Ind, int dir);
extern void do_cmd_tunnel(int Ind, int dir);
extern void do_cmd_disarm(int Ind, int dir);
extern void do_cmd_bash(int Ind, int dir);
extern void do_cmd_spike(int Ind, int dir);
extern void do_cmd_walk(int Ind, int dir, int pickup);
extern void do_cmd_stay(int Ind, int pickup);
extern int do_cmd_run(int Ind, int dir);
extern void do_cmd_fire(int Ind, int dir, int item);
extern bool interfere(int Ind, int chance);
extern void do_cmd_throw(int Ind, int dir, int item);
extern void do_cmd_purchase_house(int Ind, int dir);
extern int pick_house(struct worldpos *wpos, int y, int x);
extern void house_admin(int Ind, int dir, char *args);

/* cmd3.c */
extern void inven_takeoff(int Ind, int item, int amt);
extern void do_takeoff_impossible(int Ind);
extern void do_cmd_wield(int Ind, int item);
extern void do_cmd_takeoff(int Ind, int item);
extern void do_cmd_drop(int Ind, int item, int quantity);
extern void do_cmd_drop_gold(int Ind, s32b amt);
extern void do_cmd_destroy(int Ind, int item, int quantity);
extern void do_cmd_observe(int Ind, int item);
extern void do_cmd_uninscribe(int Ind, int item);
extern void do_cmd_inscribe(int Ind, int item, cptr inscription);
extern void do_cmd_steal(int Ind, int dir);
extern void do_cmd_refill(int Ind, int item);
extern void do_cmd_target(int Ind, int dir);
extern void do_cmd_target_friendly(int Ind, int dir);
extern void do_cmd_look(int Ind, int dir);
extern void do_cmd_locate(int Ind, int dir);
extern void do_cmd_query_symbol(int Ind, char sym);

/* cmd4.c */
extern void do_cmd_check_artifacts(int Ind, int line);
extern void do_cmd_check_uniques(int Ind, int line);
extern void do_cmd_check_players(int Ind, int line);
extern void do_cmd_check_other(int Ind, int line);

/* cmd5.c */
extern void do_cmd_browse(int Ind, int book);
extern void do_cmd_study(int Ind, int book, int spell);
extern void do_cmd_psi(int Ind, int book, int spell);
extern void do_cmd_psi_aux(int Ind, int dir);
extern void do_cmd_hunt(int Ind, int book, int spell);
extern void do_cmd_cast(int Ind, int book, int spell);
extern void do_cmd_cast_aux(int Ind, int dir);
extern void do_cmd_sorc(int Ind, int book, int spell);
extern void do_cmd_sorc_aux(int Ind, int dir);
extern void do_mimic_change(int Ind, int r_idx);
extern void do_mimic_power_aux(int Ind, int dir);
extern void do_cmd_mimic(int Ind, int spell);
extern void do_cmd_pray(int Ind, int book, int spell);
extern void do_cmd_pray_aux(int Ind, int dir);
extern void do_cmd_shad(int Ind, int book, int spell);
extern void do_cmd_shad_aux(int Ind, int dir);
extern void do_cmd_fight(int Ind, int book, int spell);
extern void do_spin(int Ind);
extern void do_cmd_fight_aux(int Ind, int dir);
extern void show_ghost_spells(int Ind);
extern void do_cmd_ghost_power(int Ind, int ability);
extern void do_cmd_ghost_power_aux(int Ind, int dir);


/* cmd6.c */
extern bool curse_armor(int Ind);
extern bool curse_weapon(int Ind);
extern void do_cmd_eat_food(int Ind, int item);
extern void do_cmd_quaff_potion(int Ind, int item);
extern void do_cmd_read_scroll(int Ind, int item);
extern void do_cmd_aim_wand(int Ind, int item, int dir);
extern void do_cmd_use_staff(int Ind, int item);
extern void do_cmd_zap_rod(int Ind, int item);
extern void do_cmd_zap_rod_dir(int Ind, int dir);
extern void do_cmd_activate(int Ind, int item);
extern void do_cmd_activate_dir(int Ind, int dir);
extern bool unmagic(int Ind);
extern void fortune(int Ind, bool broadcast);
extern char random_colour();
extern bool spell_okay(int Ind, int j, bool known);

/* control.c */
extern void NewConsole(int fd, int arg);
extern bool InitNewConsole(int write_fd);

/* dungeon.c */
extern int find_player(s32b id);
extern int find_player_name(char *name);
extern void play_game(bool new_game);
extern void shutdown_server(void);
extern void dungeon(void);
//extern bool retaliate_item(int Ind, int item, cptr inscription);
extern void pack_overflow(int Ind);

/* files.c */
extern s16b tokenize(char *buf, s16b num, char **tokens);
extern void display_player(int Ind);
extern errr file_character(cptr name, bool full);
extern errr check_time_init(void);
extern errr check_load_init(void);
extern errr check_time(void);
extern errr check_load(void);
extern void read_times(void);
extern void show_news(void);
extern errr show_file(int Ind, cptr name, cptr what, int line, int color);
extern void do_cmd_help(int Ind, int line);
extern bool process_player_name(int Ind, bool sf);
extern void get_name(int Ind);
extern void do_cmd_suicide(int Ind);
extern void do_cmd_save_game(int Ind);
extern long total_points(int Ind);
extern void display_scores(int from, int to);
extern void add_high_score(int Ind);
extern void close_game(void);
extern void exit_game_panic(void);
extern void signals_ignore_tstp(void);
extern void signals_handle_tstp(void);
extern void signals_init(void);
extern void kingly(int Ind);
extern errr get_rnd_line(cptr file_name, int entry, char *output);

/* generate.c */
extern bool dungeon_aux(int r_idx);
//extern void dealloc_dungeon_level_maybe(struct worldpos *wpos);
extern void adddungeon(struct worldpos *wpos, int baselevel, int maxdep, int flags, char *race, char *exclude, bool tower);
extern void alloc_dungeon_level(struct worldpos *wpos);
extern void dealloc_dungeon_level(struct worldpos *wpos);
extern void generate_cave(struct worldpos *wpos);
extern void build_vault(struct worldpos *wpos, int yval, int xval, int ymax, int xmax, cptr data);

/* wild.c */
extern int world_index(int world_x, int world_y);
extern void init_wild_info(void);
extern void addtown(int y, int x, int base, u16b flags);
extern void wild_apply_day(struct worldpos *wpos);
extern void wild_apply_night(struct worldpos *wpos);
extern int determine_wilderness_type(struct worldpos *wpos);
extern void wilderness_gen(struct worldpos *wpos);
extern void wild_add_monster(struct worldpos *wpos);
extern bool fill_house(house_type *h_ptr, int func, void *data);
extern void wild_add_uhouse(house_type *h_ptr);

/* init-txt.c */
extern errr init_v_info_txt(FILE *fp, char *buf);
extern errr init_f_info_txt(FILE *fp, char *buf);
extern errr init_k_info_txt(FILE *fp, char *buf);
extern errr init_a_info_txt(FILE *fp, char *buf);
extern errr init_e_info_txt(FILE *fp, char *buf);
extern errr init_r_info_txt(FILE *fp, char *buf);

/* init.c */
extern void init_file_paths(char *path);
extern void init_some_arrays(void);
extern void load_server_cfg(void);

/* load1.c */
/*extern errr rd_savefile_old(void);*/

/* load2.c */
extern errr rd_savefile_new(int Ind);
extern errr rd_server_savefile(void);

/* melee1.c */
/* melee2.c */
extern int mon_will_run(int Ind, int m_idx);
extern bool monster_attack_normal(int m_idx, int tm_idx);
extern bool make_attack_normal(int Ind, int m_idx);
extern bool make_attack_spell(int Ind, int m_idx);
extern void process_monsters(void);
extern void curse_equipment(int Ind, int chance, int heavy_chance);

/* monster.c */
/* monster1.c monster2.c */
extern bool mon_allowed(monster_race *r_ptr);
extern void heal_m_list(struct worldpos *wpos);
extern cptr r_name_get(monster_type *m_ptr);
extern monster_race* r_info_get(monster_type *m_ptr);
extern void delete_monster_idx(int i);
extern void delete_monster(struct worldpos *wpos, int y, int x);
extern void wipe_m_list(struct worldpos *wpos);
extern void compact_monsters(int size, bool purge);
extern s16b m_pop(void);
extern errr get_mon_num_prep(void);
extern s16b get_mon_num(int level);
extern void monster_desc(int Ind, char *desc, int m_idx, int mode);
extern void lore_do_probe(int m_idx);
extern void lore_treasure(int m_idx, int num_item, int num_gold);
extern void update_mon(int m_idx, bool dist);
extern void update_monsters(bool dist);
extern void update_player(int Ind);
extern void update_players(void);
extern bool place_monster_aux(struct worldpos *wpos, int y, int x, int r_idx, bool slp, bool grp, bool clo);
extern bool place_monster(struct worldpos *wpos, int y, int x, bool slp, bool grp);
extern bool alloc_monster(struct worldpos *wpos, int dis, int slp);
extern bool summon_specific(struct worldpos *wpos, int y1, int x1, int lev, int type);
extern bool summon_specific_race(struct worldpos *wpos, int y1, int x1, int r_idx, unsigned char num);
extern bool summon_specific_race_somewhere(struct worldpos *wpos, int r_idx, unsigned char num);
extern bool multiply_monster(int m_idx);
extern void update_smart_learn(int m_idx, int what);
extern void setup_monsters(void);
extern int race_index(char * name);
extern void monster_gain_exp(int m_idx, u32b exp, bool silent);

/* netserver.c */
/*extern void Contact(int fd, void *arg);*/
extern int Net_input(void);
extern int Net_output(void);
extern void setup_contact_socket(void);
extern bool Report_to_meta(int flag);
extern int Setup_net_server(void);
extern bool Destroy_connection(int Ind, char *reason);
extern int Send_plusses(int Ind, int tohit, int todam);
extern int Send_ac(int Ind, int base, int plus);
extern int Send_experience(int Ind, int lev, s32b max_exp, s32b cur_exp, s32b adv_exp);
extern int Send_gold(int Ind, s32b gold);
extern int Send_hp(int Ind, int mhp, int chp);
extern int Send_sp(int Ind, int msp, int csp);
extern int Send_char_info(int Ind, int race, int class, int sex);
extern int Send_various(int Ind, int height, int weight, int age, int sc);
extern int Send_stat(int Ind, int stat, int max, int cur);
extern int Send_history(int Ind, int line, cptr hist);
extern int Send_inven(int Ind, char pos, byte attr, int wgt, int amt, byte tval, cptr name);
extern int Send_equip(int Ind, char pos, byte attr, int wgt, byte tval, cptr name);
extern int Send_title(int Ind, cptr title);
/*extern int Send_level(int Ind, int max, int cur);*/
/*extern void Send_exp(int Ind, s32b max, s32b cur);*/
extern int Send_depth(int Ind, struct worldpos *wpos);
extern int Send_food(int Ind, int food);
extern int Send_blind(int Ind, bool blind);
extern int Send_confused(int Ind, bool confused);
extern int Send_fear(int Ind, bool afraid);
extern int Send_poison(int Ind, bool poisoned);
extern int Send_paralyzed(int Ind, bool paralyzed);
extern int Send_searching(int Ind, bool searching);
extern int Send_speed(int Ind, int speed);
extern int Send_study(int Ind, bool study);
extern int Send_cut(int Ind, int cut);
extern int Send_stun(int Ind, int stun);
extern int Send_direction(int Ind);
extern int Send_message(int Ind, cptr msg);
extern int Send_char(int Ind, int x, int y, byte a, char c);
extern int Send_spell_info(int Ind, int book, int i, cptr out_val);
extern int Send_item_request(int Ind);
extern int Send_state(int Ind, bool paralyzed, bool searching, bool resting);
extern int Send_flush(int Ind);
extern int Send_line_info(int Ind, int y);
extern int Send_mini_map(int Ind, int y);
extern int Send_store(int Ind, char pos, byte attr, int wgt, int number, int price, cptr name);
extern int Send_store_info(int Ind, int num, int owner, int items);
extern int Send_store_sell(int Ind, int price);
extern int Send_target_info(int ind, int x, int y, cptr buf);
extern int Send_sound(int ind, int sound);
extern int Send_special_line(int ind, int max, int line, byte attr, cptr buf);
extern int Send_floor(int ind, char tval);
extern int Send_pickup_check(int ind, cptr buf);
extern int Send_party(int ind);
extern int Send_special_other(int ind);
extern int Send_skills(int ind);
extern int Send_pause(int ind);
extern int Send_monster_health(int ind, int num, byte attr);

extern void Handle_direction(int Ind, int dir);




/* object1.c */
/* object2.c */
extern void object_copy(object_type *o_ptr, object_type *j_ptr);
extern s16b drop_near_severe(int Ind, object_type *o_ptr, int chance, struct worldpos *wpos, int y, int x);
extern void object_wipe(object_type *o_ptr);
extern bool can_use(int Ind, object_type *o_ptr);
extern void flavor_init(void);
extern void reset_visuals(void);
extern void object_flags(object_type *o_ptr, u32b *f1, u32b *f2, u32b *f3, u32b *f4, u32b *f5, u32b *esp);
extern void object_desc(int Ind, char *buf, object_type *o_ptr, int pref, int mode);
extern void object_desc_store(int Ind, char *buf, object_type *o_ptr, int pref, int mode);
extern bool identify_fully_aux(int Ind, object_type *o_ptr);
extern s16b index_to_label(int i);
extern s16b label_to_inven(int Ind, int c);
extern s16b label_to_equip(int Ind, int c);
extern s16b wield_slot(int Ind, object_type *o_ptr);
extern cptr mention_use(int Ind, int i);
extern cptr describe_use(int Ind, int i);
extern void inven_item_charges(int Ind, int item);
extern void inven_item_describe(int Ind, int item);
extern void inven_item_increase(int Ind, int item, int num);
extern void inven_item_optimize(int Ind, int item);
extern void floor_item_charges(int item);
extern void floor_item_describe(int item);
extern void floor_item_increase(int item, int num);
extern void floor_item_optimize(int item);
extern bool inven_carry_okay(int Ind, object_type *o_ptr);
extern s16b inven_carry(int Ind, object_type *o_ptr);
extern bool item_tester_okay(object_type *o_ptr);
extern void display_inven(int Ind);
extern void display_equip(int Ind);
/*extern void show_inven(void);
extern void show_equip(void);
extern void toggle_inven_equip(void);
extern bool get_item(int Ind, int *cp, cptr pmt, bool equip, bool inven, bool floor);*/
extern void delete_object_idx(int i);
extern void delete_object(struct worldpos *wpos, int y, int x);
extern void wipe_o_list(struct worldpos *wpos);
extern void apply_magic(struct worldpos *wpos, object_type *o_ptr, int lev, bool okay, bool good, bool great);
extern void determine_level_req(int level, object_type *o_ptr);
extern void place_object(struct worldpos *wpos, int y, int x, bool good, bool great);
extern void acquirement(struct worldpos *wpos, int y1, int x1, int num, bool great);
extern void place_trap(struct worldpos *wpos, int y, int x);
extern void place_gold(struct worldpos *wpos, int y, int x);
extern s16b drop_near(object_type *o_ptr, int chance, struct worldpos *wpos, int y, int x);
extern void pick_trap(struct worldpos *wpos, int y, int x);
extern void compact_objects(int size, bool purge);
extern s16b o_pop(void);
extern errr get_obj_num_prep(void);
extern s16b get_obj_num(int level);
extern void object_known(object_type *o_ptr);
extern void object_aware(int Ind, object_type *o_ptr);
extern void object_tried(int Ind, object_type *o_ptr);
extern s32b object_value(int Ind, object_type *o_ptr);
extern bool object_similar(int Ind, object_type *o_ptr, object_type *j_ptr);
extern void object_absorb(int Ind, object_type *o_ptr, object_type *j_ptr);
extern s16b lookup_kind(int tval, int sval);
extern void invwipe(object_type *o_ptr);
extern void invcopy(object_type *o_ptr, int k_idx);
extern void process_objects(void);
extern cptr item_activation(object_type *o_ptr);
extern void combine_pack(int Ind);
extern void reorder_pack(int Ind);
extern void setup_objects(void);
extern s16b m_bonus(int max, int level);

/* party.c */
extern int party_lookup(cptr name);
extern bool player_in_party(int party_id, int Ind);
extern int party_create(int Ind, cptr name);
extern int party_add(int adder, cptr name);
extern int party_remove(int remover, cptr name);
extern void party_leave(int Ind);
extern void party_msg(int party_id, cptr msg);
extern void party_msg_format(int party_id, cptr fmt, ...);
extern void party_gain_exp(int Ind, int party_id, s32b amount);
extern bool add_hostility(int Ind, cptr name);
extern bool remove_hostility(int Ind, cptr name);
extern bool add_ignore(int Ind, cptr name);
extern bool check_ignore(int attacker, int target);
extern bool check_hostile(int attacker, int target);
extern cptr lookup_player_name(int id);
extern int lookup_player_id(cptr name);
extern void add_player_name(cptr name, int id, time_t laston);
extern void delete_player_id(int id);
extern void delete_player_name(cptr name);
extern int player_id_list(int **list);
extern void stat_player(char *name, bool on);
extern time_t lookup_player_laston(int id);
extern void clockin(int Ind);
extern int newid(void);

/* printout.c */
extern int s_print_only_to_file(int which);
extern int s_setup(char *str);
extern int s_shutdown(void);
extern int s_printf(char *str, ...);
extern bool s_setupr(char *str);
extern bool rfe_printf(char *str, ...);
extern bool do_cmd_view_rfe(int Ind,char *str, int line);

/* save.c */
extern bool save_player(int Ind);
extern bool load_player(int Ind);
extern bool load_server_info(void);
extern bool save_server_info(void);


/* sched.c */
extern void install_timer_tick(void (*func)(void), int freq);
extern void install_input(void (*func)(int, int), int fd, int arg);
extern void remove_input(int fd);
extern void sched(void);
extern void block_timer(void);
extern void allow_timer(void);
extern void setup_timer(void);
extern void teardown_timer(void);

/* spells1.c */
extern void take_xp_hit(int Ind, int damage, cptr hit_from, bool mode, bool fatal);
extern void take_sanity_hit(int Ind, int damage, cptr hit_from);
extern s16b poly_r_idx(int r_idx);
extern bool check_st_anchor(struct worldpos *wpos);
extern void teleport_away(int m_idx, int dis);
extern void teleport_player(int Ind, int dis);
extern void teleport_player_to(int Ind, int ny, int nx);
extern void teleport_player_level(int Ind);
extern bool bypass_invuln;
extern void take_hit(int Ind, int damage, cptr kb_str);
extern void acid_dam(int Ind, int dam, cptr kb_str);
extern void elec_dam(int Ind, int dam, cptr kb_str);
extern void fire_dam(int Ind, int dam, cptr kb_str);
extern void cold_dam(int Ind, int dam, cptr kb_str);
extern bool inc_stat(int Ind, int stat);
extern bool dec_stat(int Ind, int stat, int amount, int mode);
extern bool res_stat(int Ind, int stat);
extern bool apply_disenchant(int Ind, int mode);
extern bool project_hook(int Ind, int typ, int dir, int dam, int flg);
extern bool project(int who, int rad, struct worldpos *wpos, int y, int x, int dam, int typ, int flg);
extern int set_all_destroy(object_type *o_ptr);
extern int set_cold_destroy(object_type *o_ptr);
extern int inven_damage(int Ind, inven_func typ, int perc);
//extern int inven_damage(int Ind, object_type *typ, int perc);

/* spells2.c */
extern bool heal_insanity(int Ind, int val);
extern void summon_cyber(int Ind);
extern void golem_creation(int Ind, int max);
extern bool hp_player(int Ind, int num);
extern bool hp_player_quiet(int Ind, int num);
extern void warding_glyph(int Ind);
extern bool do_dec_stat(int Ind, int stat, int mode);
extern bool do_res_stat(int Ind, int stat);
extern bool do_inc_stat(int Ind, int stat);
extern void identify_pack(int Ind);
extern void message_pain(int Ind, int m_idx, int dam);
extern bool remove_curse(int Ind);
extern bool remove_all_curse(int Ind);
extern bool restore_level(int Ind);
extern void self_knowledge(int Ind);
extern bool lose_all_info(int Ind);
extern bool detect_treasure(int Ind);
extern bool detect_magic(int Ind);
extern bool detect_invisible(int Ind);
extern bool detect_evil(int Ind);
extern bool detect_creatures(int Ind);
extern bool detection(int Ind);
extern bool detect_object(int Ind);
extern bool detect_trap(int Ind);
extern bool detect_sdoor(int Ind);
extern void stair_creation(int Ind);
extern bool enchant(int Ind, object_type *o_ptr, int n, int eflag);
extern bool enchant_spell(int Ind, int num_hit, int num_dam, int num_ac);
extern bool enchant_spell_aux(int Ind, int item, int num_hit, int num_dam, int num_ac);
extern bool ident_spell(int Ind);
extern bool ident_spell_aux(int Ind, int item);
extern bool identify_fully(int Ind);
extern bool identify_fully_item(int Ind, int item);
extern bool recharge(int Ind, int num);
extern bool recharge_aux(int Ind, int item, int num);
extern bool speed_monsters(int Ind);
extern bool slow_monsters(int Ind);
extern bool sleep_monsters(int Ind);
extern bool fear_monsters(int Ind);
extern bool stun_monsters(int Ind);
extern void aggravate_monsters(int Ind, int who);
extern bool genocide(int Ind);
extern bool mass_genocide(int Ind);
extern bool probing(int Ind);
extern bool project_hack(int Ind, int typ, int dam);
extern bool banish_evil(int Ind, int dist);
extern bool dispel_evil(int Ind, int dam);
extern bool dispel_undead(int Ind, int dam);
extern bool dispel_monsters(int Ind, int dam);
extern bool turn_undead(int Ind);
extern void destroy_area(struct worldpos *wpos, int y1, int x1, int r, bool full);
extern void earthquake(struct worldpos *wpos, int cy, int cx, int r);
extern void wipe_spell(struct worldpos *wpos, int cy, int cx, int r);
extern void lite_room(int Ind, struct worldpos *wpos, int y1, int x1);
extern void unlite_room(int Ind, struct worldpos *wpos, int y1, int x1);
extern bool lite_area(int Ind, int dam, int rad);
extern bool unlite_area(int Ind, int dam, int rad);
extern bool fire_ball(int Ind, int typ, int dir, int dam, int rad);
extern bool fire_bolt(int Ind, int typ, int dir, int dam);
extern bool fire_beam(int Ind, int typ, int dir, int dam);
extern bool fire_bolt_or_beam(int Ind, int prob, int typ, int dir, int dam);
extern bool lite_line(int Ind, int dir);
extern bool drain_life(int Ind, int dir, int dam);
extern bool wall_to_mud(int Ind, int dir);
extern bool destroy_door(int Ind, int dir);
extern bool disarm_trap(int Ind, int dir);
extern bool heal_monster(int Ind, int dir);
extern bool speed_monster(int Ind, int dir);
extern bool slow_monster(int Ind, int dir);
extern bool sleep_monster(int Ind, int dir);
extern bool confuse_monster(int Ind, int dir, int plev);
extern bool fear_monster(int Ind, int dir, int plev);
extern bool poly_monster(int Ind, int dir);
extern bool clone_monster(int Ind, int dir);
extern bool teleport_monster(int Ind, int dir);
extern bool cure_light_wounds_proj(int Ind, int dir);
extern bool cure_serious_wounds_proj(int Ind, int dir);
extern bool cure_critical_wounds_proj(int Ind, int dir);
extern bool heal_other_proj(int Ind, int dir);
extern bool door_creation(int Ind);
extern bool trap_creation(int Ind);
extern bool destroy_doors_touch(int Ind);
extern bool sleep_monsters_touch(int Ind);
extern bool create_artifact(int Ind);
extern bool create_artifact_aux(int Ind, int item);
extern bool curse_spell(int Ind);
extern bool curse_spell_aux(int Ind, int item);
extern void house_creation(int Ind, bool floor);


/* store.c */
extern void store_purchase(int Ind, int item, int amt);
extern void store_sell(int Ind, int item, int amt);
extern void store_confirm(int Ind);
extern void do_cmd_store(int Ind);
extern void store_shuffle(store_type *st_ptr);
extern void store_maint(store_type *st_ptr);
extern void store_init(store_type *st_ptr);

/* util.c */
extern void msg_admin(cptr fmt, ...);
extern int name_lookup_loose(int Ind, cptr name, bool party);
extern errr path_parse(char *buf, int max, cptr file);
extern errr path_temp(char *buf, int max);
extern errr path_build(char *buf, int max, cptr path, cptr file);
extern FILE *my_fopen(cptr file, cptr mode);
extern errr my_fgets(FILE *fff, char *buf, huge n);
extern errr my_fputs(FILE *fff, cptr buf, huge n);
extern errr my_fclose(FILE *fff);
extern errr fd_kill(cptr file);
extern errr fd_move(cptr file, cptr what);
extern errr fd_copy(cptr file, cptr what);
extern int fd_make(cptr file, int mode);
extern int fd_open(cptr file, int flags);
extern errr fd_lock(int fd, int what);
extern errr fd_seek(int fd, huge n);
extern errr fd_chop(int fd, huge n);
extern errr fd_read(int fd, char *buf, huge n);
extern errr fd_write(int fd, cptr buf, huge n);
extern errr fd_close(int fd);
extern void bell(void);
extern void sound(int Ind, int num);
extern void text_to_ascii(char *buf, cptr str);
extern void ascii_to_text(char *buf, cptr str);
extern void keymap_init(void);
extern char inkey(void);
extern cptr quark_str(s16b num);
extern s16b quark_add(cptr str);
extern bool check_guard_inscription( s16b quark, char what);
extern void msg_print(int Ind, cptr msg);
extern void msg_broadcast(int Ind, cptr msg);
extern void msg_format(int Ind, cptr fmt, ...);
extern void msg_print_near(int Ind, cptr msg);
extern void msg_format_near(int Ind, cptr fmt, ...);
extern void toggle_afk(int Ind);
extern void player_talk(int Ind, char *msg);
extern bool is_a_vowel(int ch);
extern char *wpos_format(worldpos *wpos);

/* xtra1.c */
extern void cnv_stat(int val, char *out_val);
extern s16b modify_stat_value(int value, int amount);
extern void notice_stuff(int Ind);
extern void update_stuff(int Ind);
extern void redraw_stuff(int Ind);
extern void window_stuff(int Ind);
extern void handle_stuff(int Ind);

/* xtra2.c */
extern bool set_invuln_short(int Ind, int v);
extern bool set_biofeedback(int Ind, int v);
extern bool set_adrenaline(int Ind, int v);
extern bool set_tim_esp(int Ind, int v);
extern bool set_st_anchor(int Ind, int v);
extern bool set_prob_travel(int Ind, int v);
extern bool set_mimic(int Ind, int v, int p);
extern bool set_tim_traps(int Ind, int v);
extern bool set_tim_manashield(int Ind, int v);
extern bool set_invis(int Ind, int v, int p);
extern bool set_furry(int Ind, int v);
extern bool set_tim_meditation(int Ind, int v);
extern bool set_tim_wraith(int Ind, int v);
extern bool set_blind(int Ind, int v);
extern bool set_confused(int Ind, int v);
extern bool set_poisoned(int Ind, int v);
extern bool set_afraid(int Ind, int v);
extern bool set_paralyzed(int Ind, int v);
extern bool set_image(int Ind, int v);
extern bool set_fast(int Ind, int v);
extern bool set_slow(int Ind, int v);
extern bool set_shield(int Ind, int v);
extern bool set_blessed(int Ind, int v);
extern bool set_hero(int Ind, int v);
extern bool set_shero(int Ind, int v);
extern bool set_protevil(int Ind, int v);
extern bool set_invuln(int Ind, int v);
extern bool set_tim_invis(int Ind, int v);
extern bool set_tim_infra(int Ind, int v);
extern bool set_oppose_acid(int Ind, int v);
extern bool set_oppose_elec(int Ind, int v);
extern bool set_oppose_fire(int Ind, int v);
extern bool set_oppose_cold(int Ind, int v);
extern bool set_oppose_pois(int Ind, int v);
extern bool set_stun(int Ind, int v);
extern bool set_cut(int Ind, int v);
extern bool set_food(int Ind, int v);
extern int get_player(int Ind, object_type *o_ptr);
extern void blood_bond(int Ind, object_type * o_ptr);
extern void set_recall_depth(player_type * p_ptr, object_type * o_ptr);
extern void check_experience(int Ind);
extern void gain_exp(int Ind, s32b amount);
extern void lose_exp(int Ind, s32b amount);
extern bool mon_take_hit_mon(int am_idx, int m_idx, int dam, bool *fear, cptr note);
extern void monster_death_mon(int am_idx, int m_idx);
extern void monster_death(int Ind, int m_idx);
extern void player_death(int Ind);
extern void resurrect_player(int Ind);
extern bool mon_take_hit(int Ind, int m_idx, int dam, bool *fear, cptr note);
extern void panel_bounds(int Ind);
extern void verify_panel(int Ind);
extern cptr look_mon_desc(int m_idx);
extern void ang_sort_aux(int Ind, vptr u, vptr v, int p, int q);
extern void ang_sort(int Ind, vptr u, vptr v, int n);
extern void ang_sort_swap_distance(int Ind, vptr u, vptr v, int a, int b);
extern bool ang_sort_comp_distance(int Ind, vptr u, vptr v, int a, int b);
extern bool ang_sort_comp_value(int Ind, vptr u, vptr v, int a, int b);
extern void ang_sort_swap_value(int Ind, vptr u, vptr v, int a, int b);
extern bool target_able(int Ind, int m_idx);
extern bool target_okay(int Ind);
extern s16b target_pick(int Ind, int y1, int x1, int dy, int dx);
extern bool target_set(int Ind, int dir);
extern bool target_set_friendly(int Ind, int dir, ...);
extern bool get_aim_dir(int Ind/*, int *dp*/);
extern bool get_item(int Ind);
extern bool do_scroll_life(int Ind);
extern bool do_restoreXP_other(int Ind);
extern int level_speed(struct worldpos *wpos);
extern bool telekinesis(int Ind, object_type *o_ptr);
extern void telekinesis_aux(int Ind, int item);
extern bool set_bow_brand(int Ind, int v, int t, int p);

extern bool master_level(int Ind, char * parms);
extern bool master_build(int Ind, char * parms);
extern bool master_summon(int Ind, char * parms);
extern bool master_generate(int Ind, char * parms);
extern bool master_acquire(int Ind, char * parms);
extern bool master_player(int Ind, char * parms);

extern void kill_houses(int id, int type);

/*extern bool get_rep_dir(int *dp);*/

extern bool c_get_item(int *cp, cptr pmt, bool equip, bool inven, bool floor);

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


/* script.c */
extern void init_lua();
extern bool pern_dofile(int Ind, char *file);
extern bool exec_lua(int Ind, char *file);
extern void master_script_begin(char *name, char mode);
extern void master_script_end();
extern void master_script_line(char *buf);
extern void master_script_exec(int Ind, char *name);
extern void cat_script(int Ind, char *name);

/* traps.c */
extern s16b t_pop(void);
extern void delete_trap_idx(trap_type *t_ptr);
extern void compact_traps(int size, bool purge);
extern void wipe_t_list(struct worldpos *wpos);
extern void setup_traps(void);
extern bool do_player_trap_call_out(int Ind);
//extern static bool do_trap_teleport_away(int Ind, object_type *i_ptr, s16b y, s16b x);
//extern static bool player_handle_missile_trap(int Ind, s16b num, s16b tval, s16b sval, s16b dd, s16b ds, s16b pdam, cptr name);
//extern static bool player_handle_breath_trap(int Ind, s16b rad, s16b type, u16b trap);
//extern static void trap_hit(int Ind, s16b trap);
extern bool player_activate_trap_type(int Ind, s16b y, s16b x, object_type *i_ptr, s16b item);
extern void player_activate_door_trap(int Ind, s16b y, s16b x);
extern void place_trap(struct worldpos *wpos, int y, int x);
extern void place_trap_object(object_type *o_ptr);
// extern void wiz_place_trap(int y, int x, int idx);

/* wild.c */
extern void initwild(void);
extern void genwild(void);
