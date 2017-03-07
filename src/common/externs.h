/* $Id$ */
/* File: externs.h */

/* Purpose: extern declarations (variables and functions) */

/*
 * Note that some files have their own header files
 * (z-virt.h, z-util.h, z-form.h, term.h, random.h)
 */


/*
 * Automatically generated "variable" declarations
 */

/* netserver.c */
extern long Id;
extern int NumPlayers;

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
extern byte adj_wis_msane[];
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
extern owner_type owners[MAX_BASE_STORES][MAX_STORE_OWNERS];
extern s16b extract_energy[256];
extern s32b player_exp[PY_MAX_LEVEL];
extern player_race race_info[MAX_RACE];
extern player_class class_info[MAX_CLASS];
extern player_trait trait_info[MAX_TRAIT];
extern player_magic magic_info[MAX_CLASS];
extern u32b spell_flags[2][9][2];
extern cptr spell_names[2][64];
extern byte chest_traps[64];
extern cptr player_title[MAX_CLASS][11];
extern cptr sound_names[SOUND_MAX];
extern cptr stat_names[6];
extern cptr stat_names_reduced[6];

/* variable.c */
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
extern byte ssf_major;
extern byte ssf_minor;
extern byte ssf_patch;
extern byte ssf_extra;
extern byte qsf_major;
extern byte qsf_minor;
extern byte qsf_patch;
extern byte qsf_extra;
extern bool arg_wizard;
extern bool arg_fiddle;
extern bool arg_force_original;
extern bool arg_force_roguelike;
extern bool server_generated;
extern bool server_dungeon;
extern bool server_state_loaded;
extern bool server_saved;
extern bool character_loaded;
extern bool character_icky;
extern bool character_xtra;
extern u32b seed_flavor;
extern u32b seed_town;
extern s16b command_cmd;
extern s16b command_arg;
/*extern s16b command_rep;*/
extern s16b command_dir;
extern s16b command_see;
extern s16b command_gap;
extern s16b command_wrk;
extern s16b command_new;
/*extern s16b energy_use;*/
extern s16b choose_default;
extern bool create_up_stair;
extern bool create_down_stair;
extern bool msg_flag;
extern s16b num_repro;
extern s16b object_level;
extern s16b monster_level;
extern s32b turn;
extern s32b old_turn;
extern bool wizard;
extern bool to_be_wizard;
extern bool can_be_wizard;
extern u16b panic_save;
extern bool scan_monsters;
extern bool scan_objects;
extern s16b inven_nxt;
extern s16b o_nxt;
extern s16b m_nxt;
extern s16b o_max;
extern s16b m_max;
extern s16b o_top;
extern s16b m_top;


//deprecate: game options -- nowadays they're client options instead -- todo: clear
extern player_type **Players;
extern long GetInd[];
extern s16b o_fast[MAX_O_IDX];
extern s16b m_fast[MAX_M_IDX];
extern object_type *o_list;
extern monster_type *m_list;
extern xorder xo_list[MAX_XO_IDX];
extern store_type *store;
/*extern object_type *inventory;*/
extern s16b alloc_kind_size;
extern alloc_entry *alloc_kind_table;
extern s16b alloc_race_size;
extern alloc_entry *alloc_race_table;
extern byte tval_to_attr[128];
extern char tval_to_char[128];

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
extern cptr ANGBAND_DIR_TEXT;
extern cptr ANGBAND_DIR_USER;
extern cptr ANGBAND_DIR_XTRA;
extern bool item_tester_full;
extern byte item_tester_tval;
extern bool (*item_tester_hook)(object_type *o_ptr);
extern bool (*ang_sort_comp)(int Ind, vptr u, vptr v, int a, int b);
extern void (*ang_sort_swap)(int Ind, vptr u, vptr v, int a, int b);
extern bool (*get_mon_num_hook)(int r_idx);
extern bool (*get_obj_num_hook)(int k_idx);




/*
 * Automatically generated "function declarations"
 */

/* birth.c */
extern void player_birth(int Ind, cptr name, int conn, int race, int class, int sex);

/* cave.c */
extern int distance(int y1, int x1, int y2, int x2);
extern bool los(int Depth, int y1, int x1, int y2, int x2);
extern bool player_can_see_bold(int Ind, int y, int x);
extern bool no_lite(int Ind);
extern void map_info(int Ind, int y, int x, byte *ap, char *cp);
extern void move_cursor_relative(int row, int col);
extern void print_rel(char c, byte a, int y, int x);
extern void note_spot(int Ind, int y, int x);
extern void note_spot_depth(int Depth, int y, int x);
extern void everyone_lite_spot(int Depth, int y, int x);
extern void lite_spot(int Ind, int y, int x);
extern void prt_map(int Ind, bool scr_only);
extern void display_map(int *cy, int *cx);
extern void do_cmd_view_map(void);
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
extern bool projectable(int Depth, int y1, int x1, int y2, int x2);
extern void scatter(int Depth, int *yp, int *xp, int y, int x, int d, int m);
extern void health_track(int m_idx);
extern void recent_track(int r_idx);
extern void disturb(int Ind, int stop_search, int flush_output);
extern bool is_quest(int level);

/* cmd1.c */
extern bool test_hit_fire(int chance, int ac, int vis);
extern bool test_hit_melee(int chance, int ac, int vis);
extern s16b critical_shot(int Ind, int weight, int plus, int dam);
extern s16b critical_melee(int Ind, int weight, int plus, int dam);
extern s16b tot_dam_aux(object_type *o_ptr, int tdam, monster_type *m_ptr);
extern void search(int Ind);
extern void carry(int Ind, int pickup);
extern void py_attack(int Ind, int y, int x);
extern void move_player(int Ind, int dir, int do_pickup);
extern void run_step(int Ind, int dir);

/* cmd2.c */
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
extern void do_cmd_run(int Ind, int dir);
/*extern void do_cmd_rest(void);*/
extern void do_cmd_fire(int Ind, int dir, int item);
extern void do_cmd_throw(int Ind, int dir, int item);

/* cmd3.c */
extern void do_cmd_inven(void);
extern void do_cmd_equip(void);
extern void do_cmd_wield(int Ind, int item);
extern void do_cmd_takeoff(int Ind, int item);
extern void do_cmd_drop(int Ind, int item, int quantity);
extern void do_cmd_destroy(int Ind, int item, int quantity);
extern void do_cmd_observe(int Ind, int item);
extern void do_cmd_uninscribe(int Ind, int item);
extern void do_cmd_inscribe(int Ind, int item, cptr inscription);
extern void do_cmd_refill(int Ind, int item);
extern void do_cmd_target(int Ind, int dir);
extern void do_cmd_look(int Ind, int dir);
extern void do_cmd_locate(int Ind, int dir);
extern void do_cmd_query_symbol(int Ind, char sym);

/* cmd4.c */
extern void do_cmd_redraw(void);
extern void do_cmd_change_name(void);
extern void do_cmd_message_one(void);
extern void do_cmd_messages(void);
extern void do_cmd_options(void);
extern void do_cmd_pref(void);
extern void do_cmd_macros(void);
extern void do_cmd_visuals(void);
extern void do_cmd_colors(void);
extern void do_cmd_note(void);
extern void do_cmd_version(void);
extern void do_cmd_feeling(void);
extern void do_cmd_load_screen(void);
extern void do_cmd_save_screen(void);
extern void do_cmd_check_artifacts(int Ind);
extern void do_cmd_check_uniques(int Ind);

/* cmd5.c */
extern void do_cmd_browse(int Ind, int book);
extern void do_cmd_study(int Ind, int book, int spell);

/* cmd6.c */
extern void do_cmd_eat_food(int Ind, int item);
extern void do_cmd_quaff_potion(int Ind, int item);
extern void do_cmd_read_scroll(int Ind, int item);
extern void do_cmd_aim_wand(int Ind, int item, int dir);
extern void do_cmd_use_staff(int Ind, int item);
extern void do_cmd_zap_rod(int Ind, int item);
extern void do_cmd_zap_rod_dir(int Ind, int dir);
extern void do_cmd_activate(int Ind, int item);
extern void do_cmd_activate_dir(int Ind, int dir);

/* dungeon.c */
extern void play_game(bool new_game);

/* client/server */
extern int Receive_file_data(int ind, unsigned short len, char *buffer);
extern int Send_file_check(int ind, unsigned short id, char *fname);
extern int Send_file_init(int ind, unsigned short id, char *fname);
extern int Send_file_data(int ind, unsigned short id, char *buf, unsigned short len);
extern int Send_file_end(int ind, unsigned short id);

/* files.c */
extern void safe_setuid_drop(void);
extern void safe_setuid_grab(void);
extern s16b tokenize(char *buf, s16b num, char **tokens);
extern void display_player(int Ind, bool do_hist);
extern errr file_character(cptr name, bool full);
extern errr process_pref_file_aux(char *buf);
extern errr process_pref_file(cptr name);
extern errr check_time_init(void);
extern errr check_load_init(void);
extern errr check_time(void);
extern errr check_load(void);
extern void read_times(void);
extern void show_news(void);
extern errr show_file(int Ind, cptr name, cptr what);
extern void do_cmd_help(cptr name);
extern void process_player_name(int Ind, bool sf);
extern void get_name(int Ind);
extern void do_cmd_suicide(int Ind);
extern void do_cmd_save_game(int Ind);
extern long total_points(int Ind);
extern void display_scores(int from, int to);
extern void close_game(void);
extern void exit_game_panic(void);
extern void save_game_panic(void);
extern void signals_ignore_tstp(void);
extern void signals_handle_tstp(void);
extern void signals_init(void);

/* generate.c */
extern void generate_cave(int Depth, player_type *p_ptr);

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

/* load1.c */
/*extern errr rd_savefile_old(void);*/

/* load2.c */
extern errr rd_savefile_new(int Ind);

/* melee1.c */
/* melee2.c */
extern bool make_attack_melee(int Ind, int m_idx);
extern bool make_attack_spell(int Ind, int m_idx);
extern void process_monsters(void);

/* mon-desc.c */
extern void screen_roff(int r_idx);
extern void display_roff(int r_idx);

/* monster.c */
extern cptr r_name_get(monster_type *m_ptr);
extern void delete_monster_idx(int i, bool unfound_art);
extern void delete_monster(int Depth, int y, int x, bool unfound_art);
extern void compact_monsters(int size);
extern void wipe_m_list(int Depth);
extern s16b m_pop(void);
extern errr get_mon_num_prep(void);
extern s16b get_mon_num(int level);
extern void monster_desc(char *desc, monster_type *m_ptr, int mode);
extern void lore_do_probe(int m_idx);
extern void lore_treasure(int m_idx, int num_item, int num_gold);
extern void update_mon(int m_idx, bool dist);
extern void update_monsters(bool dist);
extern bool place_monster_aux(int Depth, int y, int x, int r_idx, bool slp, bool grp);
extern bool place_monster(int Depth, int y, int x, bool slp, bool grp);
extern bool alloc_monster(int Depth, int dis, int slp);
extern bool multiply_monster(int m_idx);
extern void update_smart_learn(int m_idx, int what);

/* netserver.c */
/*extern void Contact(int fd, void *arg);*/
extern int Net_input(void);
extern int Net_output(void);
extern int Net_output1(int i);
extern void setup_contact_socket(void);
extern int Setup_net_server(void);
extern void Destroy_connection(int Ind, char *reason);
extern int Send_plusses(int Ind, int tohit, int todam);
extern int Send_ac(int Ind, int base, int plus);
extern int Send_experience(int Ind, int lev, int max_exp, int cur_exp, s32b adv_exp);
extern int Send_gold(int Ind, s32b gold);
extern int Send_hp(int Ind, int mhp, int chp);
extern int Send_sp(int Ind, int msp, int csp);
extern int Send_char_info(int Ind, int race, int class, int sex, int mode);
extern int Send_various(int Ind, int height, int weight, int age, int sc);
extern int Send_stat(int Ind, int stat, int max, int cur, int cur_base);
extern int Send_history(int Ind, int line, cptr hist);
extern int Send_inven(int Ind, char pos, byte attr, int wgt, int amt, byte tval, cptr name);
extern int Send_equip(int Ind, char pos, byte attr, int wgt, byte tval, cptr name);
extern int Send_title(int Ind, cptr title);
/*extern int Send_level(int Ind, int max, int cur);*/
/*extern void Send_exp(int Ind, s32b max, s32b cur);*/
extern int Send_depth(int Ind, int depth);
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
extern int Send_spell_info(int Ind, int i, cptr out_val);
extern int Send_item_request(int Ind);
extern int Send_state(int Ind, bool paralyzed, bool searching);
extern int Send_beep(int Ind);

extern void Handle_direction(int Ind, int dir);




/* object1.c */
/* object2.c */
extern void flavor_init(void);
extern void reset_visuals(void);
extern void object_flags(object_type *o_ptr, u32b *f1, u32b *f2, u32b *f3);
extern void object_desc(int Ind, char *buf, object_type *o_ptr, int pref, int mode);
extern void object_desc_store(char *buf, object_type *o_ptr, int pref, int mode);
//extern bool identify_fully_aux(object_type *o_ptr);
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
extern void delete_object_idx(int i, bool unfound_art);
extern void delete_object(int Depth, int y, int x, bool unfound_art);
extern void compact_objects(int size);
extern void wipe_o_list(int Depth);
extern s16b o_pop(void);
extern errr get_obj_num_prep(void);
extern s16b get_obj_num(int level);
extern void object_known(object_type *o_ptr);
extern void object_aware(object_type *o_ptr);
extern void object_tried(object_type *o_ptr);
extern s32b object_value(object_type *o_ptr);
extern bool object_similar(object_type *o_ptr, object_type *j_ptr, s16b tolerance);
extern void object_absorb(object_type *o_ptr, object_type *j_ptr);
extern s16b lookup_kind(int tval, int sval);
extern void invwipe(object_type *o_ptr);
extern void invcopy(object_type *o_ptr, int k_idx);
extern void apply_magic(int Depth, object_type *o_ptr, int lev, bool okay, bool good, bool great, bool verygreat, u16b resf);
/*extern void place_object(int Depth, int y, int x, bool good, bool great, bool true_art, int luck);*/
extern void place_object(struct worldpos *wpos, int y, int x, bool good, bool great, bool verygreat, u16b resf, obj_theme theme, int luck, byte removal_marker);
extern void acquirement(int Depth, int y1, int x1, int num, bool great, bool verygreat, u16b resf);
extern void place_trap(int Depth, int y, int x);
extern void place_gold(int Depth, int y, int x);
extern void process_objects(void);
extern void drop_near(object_type *o_ptr, int chance, int Depth, int y, int x);
extern void pick_trap(int Depth, int y, int x);
extern cptr item_activation(object_type *o_ptr);
extern void combine_pack(int Ind);
extern void reorder_pack(int Ind);

/* save.c */
extern bool save_player(int Ind);
extern bool load_player(int Ind);
extern bool load_server_info(void);
extern bool save_server_info(void);


/* sched.c */
extern void install_timer_tick(void (*func)(void), int freq);
extern void install_input(void (*func)(int, void *), int fd, void *arg);
extern void remove_input(int fd);
extern void sched(void);

/* spells1.c */
extern s16b poly_r_idx(int r_idx);
extern void teleport_away(int m_idx, int dis);
extern void teleport_player(int Ind, int dis);
extern void teleport_player_to(int Ind, int ny, int nx);
extern void teleport_player_level(int Ind);
extern void take_hit(int Ind, int damage, cptr kb_str);
extern void acid_dam(int Ind, int dam, cptr kb_str);
extern void elec_dam(int Ind, int dam, cptr kb_str);
extern void fire_dam(int Ind, int dam, cptr kb_str);
extern void cold_dam(int Ind, int dam, cptr kb_str);
extern bool inc_stat(int Ind, int stat);
extern bool dec_stat(int Ind, int stat, int amount, int permanent);
extern bool res_stat(int Ind, int stat);
extern bool apply_disenchant(int Ind, int mode);
extern bool project(int who, int rad, int Depth, int y, int x, int dam, int typ, int flg, char attacker[]);

/* spells2.c */
extern bool hp_player(int Ind, int num);
extern void warding_glyph(int Ind);
extern bool do_dec_stat(int Ind, int stat);
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
extern bool detect_monsters(int Ind);
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
extern void aggravate_monsters(int Ind, int who);
extern bool genocide(int Ind);
extern bool obliteration(int who);
extern bool probing(int Ind);
extern bool banish_evil(int Ind, int dist);
extern bool dispel_evil(int Ind, int dam);
extern bool dispel_undead(int Ind, int dam);
extern bool dispel_demons(int Ind, int dam);
extern bool dispel_monsters(int Ind, int dam);
extern bool turn_undead(int Ind);
extern void destroy_area(int Depth, int y1, int x1, int r, bool full);
extern void earthquake(int Depth, int cy, int cx, int r);
extern void lite_room(int Ind, int Depth, int y1, int x1);
extern void unlite_room(int Ind, int Depth, int y1, int x1);
extern bool lite_area(int Ind, int dam, int rad);
extern bool unlite_area(int Ind, int dam, int rad);
extern bool fire_ball(int Ind, int typ, int dir, int dam, int rad);
extern bool fire_bolt(int Ind, int typ, int dir, int dam);
extern bool fire_beam(int Ind, int typ, int dir, int dam);
extern bool fire_bolt_or_beam(int Ind, int prob, int typ, int dir, int dam);
extern bool lite_line(int Ind, int dir, bool starlight);
extern bool drain_life(int Ind, int dir, int dam);
extern bool wall_to_mud(int Ind, int dir);
extern bool destroy_trap_door(int Ind, int dir);
extern bool disarm_trap_door(int Ind, int dir);
extern bool heal_monster(int Ind, int dir);
extern bool speed_monster(int Ind, int dir);
extern bool slow_monster(int Ind, int dir);
extern bool sleep_monster(int Ind, int dir);
extern bool confuse_monster(int Ind, int dir, int plev);
extern bool fear_monster(int Ind, int dir, int plev);
extern bool poly_monster(int Ind, int dir);
extern bool clone_monster(int Ind, int dir);
extern bool teleport_monster(int Ind, int dir);
extern bool door_creation(int Ind);
extern bool trap_creation(int Ind);
extern bool destroy_doors_touch(int Ind);
extern bool destroy_traps_touch(int Ind);
extern bool destroy_traps_doors_touch(int Ind);
extern bool sleep_monsters_touch(int Ind);

/* store.c */
extern void do_cmd_store(int Ind);
extern void store_shuffle(int which);
extern void store_maint(int which);
extern void store_init(int which);

/* util.c */
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
extern errr fd_read(int fd, char *buf, huge n);
extern errr fd_write(int fd, cptr buf, huge n);
extern errr fd_close(int fd);
extern void flush(void);
extern void bell(void);
extern void sound(int num);
/* extern void move_cursor(int row, int col); */
extern void text_to_ascii(char *buf, cptr str);
extern void ascii_to_text(char *buf, cptr str);
extern void keymap_init(void);
extern void macro_add(cptr pat, cptr act, bool cmd_flag);
extern char inkey(void);
extern cptr quark_str(s16b num);
extern s16b quark_add(cptr str);
extern s16b message_num(void);
extern cptr message_str(s16b age);
extern void message_add(cptr msg);
extern void msg_print(int Ind, cptr msg);
extern void msg_format(int Ind, cptr fmt, ...);
extern void c_put_str(byte attr, cptr str, int row, int col);
extern void put_str(cptr str, int row, int col);
extern void c_prt(byte attr, cptr str, int row, int col);
extern void prt(cptr str, int row, int col);
extern void c_roff(byte attr, cptr str);
extern void roff(cptr str);
extern void clear_screen(void);
extern void clear_from(int row);
extern bool askfor_aux(char *buf, int len);
extern bool get_string(cptr prompt, char *buf, int len);
extern bool get_check(cptr prompt);
extern bool get_com(cptr prompt, char *command);
extern s16b get_quantity(cptr prompt, int max);
extern void pause_line(int row);
extern void request_command(bool shopping);
extern bool is_a_vowel(int ch);
extern bool is_same_as(version_type *version, int major, int minor, int patch, int extra, int branch, int build);

/* xtra1.c */
extern void cnv_stat(int val, char *out_val);
extern s16b modify_stat_value(int value, int amount);
extern void notice_stuff(int Ind);
extern void update_stuff(int Ind);
extern void redraw_stuff(int Ind);
extern void redraw2_stuff(int Ind);
extern void window_stuff(int Ind);
extern void handle_stuff(int Ind);

/* xtra2.c */
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
extern bool set_zeal(int Ind, int p, int v);
extern bool set_martyr(int Ind, int v);
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
extern void check_experience(int Ind);
extern void gain_exp(int Ind, s64b amount);
extern void lose_exp(int Ind, s32b amount);
extern void monster_death(int Ind, int m_idx);
extern bool mon_take_hit(int Ind, int m_idx, int dam, bool *fear, cptr note);
extern void panel_bounds(int Ind);
extern void verify_panel(int Ind);
extern cptr look_mon_desc(int m_idx);
extern void ang_sort_aux(int Ind, vptr u, vptr v, int p, int q);
extern void ang_sort(int Ind, vptr u, vptr v, int n);
extern void ang_sort_swap_distance(int Ind, vptr u, vptr v, int a, int b);
extern bool ang_sort_comp_distance(int Ind, vptr u, vptr v, int a, int b);
extern bool target_able(int Ind, int m_idx);
extern bool target_okay(int Ind);
extern s16b target_pick(int Ind, int y1, int x1, int dy, int dx);
extern bool target_set(int Ind, int dir);
extern bool get_aim_dir(int Ind/*, int *dp*/);
extern bool get_item(int Ind);
/*extern bool get_rep_dir(int *dp);*/

extern bool c_get_item(int *cp, cptr pmt, bool equip, bool inven, bool floor);

/* common.c */
extern int find_realm(int book);
extern void version_build(void);
extern const char *my_strcasestr(const char *big, const char *little);

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

extern cptr longVersion;
extern cptr shortVersion;

/* Defined as TRUE in src/client/variable.c and FALSE in src/server/variable.c */
extern bool is_client_side;

/* Needed for RETRY_LOGIN in the client */
extern bool rl_connection_destructible, rl_connection_destroyed, rl_connection_state;
