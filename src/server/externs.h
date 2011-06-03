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

/* #include "netserver.h" */

#ifdef TOMENET_WORLDS
extern void world_comm(int fd, int arg);
extern int WorldSocket;

extern void world_msg(char *text);
extern void world_player(unsigned long id, char *name, unsigned short enter, byte quiet);
extern void world_chat(unsigned long id, char *text);
extern int world_remote_players(FILE *fff);
struct rplist *world_find_player(char *pname, short server);
void world_pmsg_send(unsigned long id, char *name, char *pname, char *text);
#endif

/* common/common.c */
extern errr path_build(char *buf, int max, cptr path, cptr file);
extern int color_char_to_attr(char c);
extern char color_attr_to_char(int a);
extern void version_build(void);


/* common/files.c */
int local_file_init(int ind, unsigned short fnum, char *fname);
int local_file_write(int ind, unsigned short fnum, unsigned long len);
int local_file_close(int ind, unsigned short fnum);
int local_file_check(char *fname, u32b *sum);
int local_file_ack(int ind, unsigned short fnum);
int local_file_err(int ind, unsigned short fnum);
void do_xfers(void);
void kill_xfers(int ind);
int check_return(int ind, unsigned short fnum, unsigned long sum);
int remote_update(int ind, cptr fname);
void remote_update_lua(int Ind, cptr file);

/* csfunc.c */
extern struct sfunc csfunc[];
extern void cs_erase(cave_type *c_ptr, struct c_special *cs_ptr);

/* nserver.c */
extern int NumPlayers;
extern int ConsoleSocket;
extern int SGWSocket;
extern void process_pending_commands(int Ind);
extern bool player_is_king(int Ind);
extern void end_mind(int Ind, bool update);
extern void add_banlist(int Ind, int time);
extern void add_banlist_ip(char *ip_addy, int time);
//extern connection_t **Conn;
extern char* get_conn_userhost(int ind);

/* randart.c */
extern artifact_type *ego_make(object_type *o_ptr);
extern artifact_type *randart_make(object_type *o_ptr);
extern void randart_name(object_type *o_ptr, char *buffer, char *raw_buffer);
extern void add_random_ego_flag(artifact_type *a_ptr, int fego, bool *limit_blows, s16b dun_level, object_type *o_ptr);
extern void random_resistance (artifact_type *a_ptr, bool is_scroll, int specific);
extern void dragon_resist(artifact_type *a_ptr);
extern s32b artifact_power (artifact_type *a_ptr);

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
extern byte adj_str_armor[];
extern byte adj_str_blow[];
extern byte adj_dex_blow[];
extern byte adj_dex_safe[];
extern byte adj_con_fix[];
extern byte adj_con_mhp[];
extern byte blows_table[12][12];
extern owner_type owners[MAX_BASE_STORES][MAX_STORE_OWNERS];
extern byte extract_energy[256];
extern byte level_speeds[501];
extern s32b player_exp[PY_MAX_LEVEL + 1];
extern player_race race_info[MAX_RACE];
extern char *special_prace_lookup[MAX_RACE];
extern player_class class_info[MAX_CLASS];
extern player_trait trait_info[MAX_TRAIT];

extern magic_type ghost_spells[64];
extern u32b spell_flags[MAX_REALM - 1][9][2];
extern cptr spell_names[MAX_REALM][64];

extern byte chest_traps[64];
extern cptr player_title[MAX_CLASS][11][4];
extern cptr player_title_special[MAX_CLASS][5][4];
extern magic_type innate_powers[96];
extern martial_arts ma_blows[MAX_MA];
extern int skill_tree_init[MAX_SKILLS][2];

extern int month_day[9];
extern cptr month_name[9];
extern town_extra town_profile[6];
extern int p_tough_ac[51];


/* variable.c */
extern int teamscore[];
extern int teams[];
extern int gametype;
extern obj_theme default_obj_theme;
extern s16b count_project;
extern s16b count_project_times;
extern char tdy[662];
extern char tdx[662];
extern s32b tdi[18];	/* PREPARE_RADIUS + 2 */
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
extern bool server_generated;
extern bool server_dungeon;
extern bool server_state_loaded;
extern bool server_saved;
extern bool character_loaded;
extern u32b seed_flavor;
extern u32b seed_town;
extern u32b seed_wild_extra;
extern s16b command_new;
extern s16b choose_default;
extern bool create_up_stair;
extern bool create_down_stair;
extern s16b num_repro;
extern s16b object_level;
extern s16b object_discount;
extern s16b monster_level;
extern s16b monster_level_min;
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
extern s32b turn, session_turn, turn_overflow;
extern char tron_speed;
extern char tron_dark;
extern char tron_forget;
extern s32b player_id;
extern u32b account_id;
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
extern header *d_head;
extern dungeon_info_type *d_info;
extern char *d_name;
extern char *d_text;
extern char *re_name;
extern header *s_head;
extern skill_type *s_info;
extern char *s_name;
extern char *s_text;
extern header *st_head;
extern store_info_type *st_info;
extern char *st_name;
extern header *ba_head;
extern store_action_type *ba_info;
extern char *ba_name;
extern header *ow_head;
extern owner_type *ow_info;
extern char *ow_name;

extern server_opts cfg;

extern s32b dungeon_store_timer;
extern s32b dungeon_store2_timer;
extern s32b great_pumpkin_timer; /* for HALLOWEEN */
extern bool night_surface;
extern s16b MaxSimultaneousPlayers;
extern char serverStartupTime[40];
extern char *sST;

extern bool use_color;
extern bool hilite_player;
extern bool safe_float;//avoid_abort;
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
extern u32b old_id[MAX_ID];
extern struct swear swear[30];
extern struct ip_ban *banlist;
extern s32b sflags_TEMP;
extern player_type **Players;
extern party_type parties[MAX_PARTIES];
extern struct guild_type guilds[MAX_GUILDS];
extern struct quest_type quests[20];	/* server quest data */
extern house_type *houses;
#ifdef PLAYER_STORES
extern store_type *fake_store;
extern int fake_store_visited[MAX_VISITED_PLAYER_STORES];
#endif
extern s32b num_houses;
extern u32b house_alloc;
extern int GetInd[];
extern s16b quark__num;
extern cptr *quark__str;
extern s16b o_fast[MAX_O_IDX];
extern s16b m_fast[MAX_M_IDX];
extern cave_type ***cave;
extern wilderness_type wild_info[MAX_WILD_Y][MAX_WILD_X];
extern struct town_type *town;
extern u16b numtowns;
extern object_type *o_list;
extern monster_type *m_list;
extern quest q_list[MAX_Q_IDX];
extern s16b alloc_kind_size;
extern alloc_entry *alloc_kind_table;
extern s16b *alloc_kind_index_level;
extern s16b alloc_race_size;
extern alloc_entry *alloc_race_table;
extern alloc_entry **alloc_race_table_dun;
extern s16b *alloc_race_index_level;
extern char *allow_uniques;
extern char *reject_uniques;
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
extern s16b *e_tval_size;
extern s16b **e_tval;
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
extern bool (*get_mon_num2_hook)(int r_idx);
extern int (*get_obj_num_hook)(int k_idx, u32b resf);
extern bool (*master_move_hook)(int Ind, char * parms);

extern int artifact_bias;
extern char summon_kin_type;

extern u16b old_max_s_idx;
extern u16b max_s_idx;
extern u16b max_r_idx;
extern u16b max_re_idx;
extern u16b max_k_idx;
extern u16b max_v_idx;
extern u16b max_f_idx;
extern u16b max_a_idx;
extern u16b max_e_idx;
extern u16b max_ra_idx;
extern u16b max_d_idx;
extern u16b max_o_idx;
extern u16b max_m_idx;
extern u16b max_t_idx;
extern u16b max_rp_idx;
extern u16b max_c_idx;
extern u16b max_mc_idx;
extern u16b max_rmp_idx;
extern u16b max_st_idx;
extern u16b max_ba_idx;
extern u16b max_ow_idx;
extern u16b max_wf_idx;
extern s16b max_set_idx;

extern char priv_note_sender[MAX_NOTES][80], priv_note_target[MAX_NOTES][80], priv_note[MAX_NOTES][80];
extern char party_note[MAX_PARTYNOTES][80], party_note_target[MAX_PARTYNOTES][80];
extern char guild_note[MAX_GUILDNOTES][80], guild_note_target[MAX_GUILDNOTES][80];
extern char admin_note[MAX_ADMINNOTES][80], server_warning[160];

extern char bbs_line[BBS_LINES][140];
extern char pbbs_line[MAX_PARTIES][BBS_LINES][140];
extern char gbbs_line[MAX_GUILDS][BBS_LINES][140];

extern auction_type *auctions;
extern u32b auction_alloc;

/* Array used by everyone_lite_later_spot */
struct worldspot *lite_later;
int lite_later_alloc;
int lite_later_num;

/*
 * The spell list of schools
 */
extern s16b max_spells;
extern spell_type *school_spells;
extern s16b max_schools;
extern school_type *schools;

extern int project_interval;
extern int project_time;
extern s32b project_time_effect;
extern effect_type effects[MAX_EFFECTS];

#ifdef USE_SOUND_2010
extern char audio_sfx[SOUND_MAX_2010][30];
#endif

/* variable.c ends here */

/*
 * Automatically generated "function declarations"
 */

/* birth.c */
extern bool player_birth(int Ind, cptr accname, cptr name, int conn, int race, int class, int trait, int sex, int stat_order[]);
extern bool confirm_admin(int Ind);
extern void server_birth(void);
extern void admin_outfit(int Ind, int realm);

/* cave.c */
extern bool cave_set_quietly;

extern bool cave_valid_bold(cave_type **zcave, int y, int x);
extern struct c_special *GetCS(cave_type *c_ptr, unsigned char type);
extern struct c_special *AddCS(cave_type *c_ptr, byte type);
extern c_special *ReplaceCS(cave_type *c_ptr, byte type);
extern void FreeCS(cave_type *c_ptr);
extern dun_level *getfloor(struct worldpos *wpos);
extern void cave_set_feat(worldpos *wpos, int y, int x, int feat);
extern void cave_set_feat_live(worldpos *wpos, int y, int x, int feat);
extern int check_feat(worldpos *wpos, int y, int x);
extern struct dungeon_type *getdungeon(struct worldpos *wpos);
extern bool can_go_up(struct worldpos *wpos, byte mode);
extern bool can_go_down(struct worldpos *wpos, byte mode);
extern bool can_go_up_simple(struct worldpos *wpos);
extern bool can_go_down_simple(struct worldpos *wpos);
extern int getlevel(struct worldpos *wpos);
extern void wpcopy(struct worldpos *dest, struct worldpos *src);
extern int distance(int y1, int x1, int y2, int x2);
extern bool player_can_see_bold(int Ind, int y, int x);
extern bool no_lite(int Ind);
extern byte get_trap_color(int Ind, int t_idx, int feat);
extern void map_info(int Ind, int y, int x, byte *ap, char *cp);
extern void move_cursor_relative(int row, int col);
extern void print_rel(char c, byte a, int y, int x);
extern void note_spot(int Ind, int y, int x);
extern void new_players_on_depth(struct worldpos *wpos, int value, bool inc);
extern int players_on_depth(struct worldpos *wpos);
extern void check_Pumpkin(void);
extern void check_Morgoth(int Ind);
extern bool los(struct worldpos *wpos, int y1, int x1, int y2, int x2);
extern bool los_wall(struct worldpos *wpos, int y1, int x1, int y2, int x2);
extern void note_spot_depth(struct worldpos *wpos, int y, int x);
extern void everyone_lite_spot(struct worldpos *wpos, int y, int x);
extern void everyone_clear_ovl_spot(struct worldpos *wpos, int y, int x);
extern void everyone_forget_spot(struct worldpos *wpos, int y, int x);
extern cave_type **getcave(struct worldpos *wpos);
extern void lite_spot(int Ind, int y, int x);
extern void draw_spot_ovl(int Ind, int y, int x, byte a, char c);
extern void clear_ovl_spot(int Ind, int y, int x);
extern void clear_ovl(int Ind);
extern void prt_map(int Ind);
extern void display_map(int Ind, int *cy, int *cx);
extern void do_cmd_view_map(int Ind, char mode);
extern void forget_lite(int Ind);
extern void update_lite(int Ind);
extern void forget_view(int Ind);
extern void update_view(int Ind);
extern void forget_flow(void);
extern void update_flow(void);
extern void map_area(int Ind);
extern void mind_map_level(int Ind);
extern void wiz_lite(int Ind);
extern void wiz_lite_extra(int Ind);
extern void wiz_dark(int Ind);
extern void mmove2(int *y, int *x, int y1, int x1, int y2, int x2);
extern bool projectable(struct worldpos *wpos, int y1, int x1, int y2, int x2, int range);
extern bool projectable_wall(struct worldpos *wpos, int y1, int x1, int y2, int x2, int range);
extern bool projectable_wall_perm(struct worldpos *wpos, int y1, int x1, int y2, int x2, int range);
extern bool projectable_real(int Ind, int y1, int x1, int y2, int x2, int range);
extern bool projectable_wall_real(int Ind, int y1, int x1, int y2, int x2, int range);
extern void scatter(struct worldpos *wpos, int *yp, int *xp, int y, int x, int d, int m);
extern bool is_quest(struct worldpos *wpos);
extern void health_track(int Ind, int m_idx);
extern void update_health(int m_idx);
extern void recent_track(int r_idx);
extern void disturb(int Ind, int stop_search, int keep_resting);
extern void update_players(void);

extern int new_effect(int who, int type, int dam, int time, int interval, worldpos *wpos, int cy, int cx, int rad, s32b flags);
extern bool allow_terraforming(struct worldpos *wpos, byte feat);
extern void everyone_lite_later_spot(struct worldpos *wpos, int y, int x);

extern void season_change(int s, bool force);
extern void player_weather(int Ind, bool entered_level, bool weather_changed, bool panel_redraw);

/* cmd1.c */
extern byte cycle[], chome[];
extern bool nothing_test(object_type *o_ptr, player_type *p_ptr, worldpos *wpos, int x, int y);
extern bool test_hit_fire(int chance, int ac, int vis);
extern bool test_hit_melee(int chance, int ac, int vis);
extern s16b critical_shot(int Ind, int weight, int plus, int dam);
extern s16b critical_melee(int Ind, int weight, int plus, int dam, bool allow_skill_crits, int o_crit);
extern s16b tot_dam_aux(int Ind, object_type *o_ptr, int tdam, monster_type *m_ptr, char *brand_msg, bool thrown);
extern s16b tot_dam_aux_player(int Ind, object_type *o_ptr, int tdam, player_type *p_ptr, char *brand_msg, bool thrown);
extern void search(int Ind);
extern void carry(int Ind, int pickup, int confirm);
extern void py_attack(int Ind, int y, int x, bool old);
extern void spin_attack(int Ind);
extern void touch_zap_player(int Ind, int m_idx);
extern void py_touch_zap_player(int Ind, int Ind2);
extern void do_nazgul(int Ind, int *k, int *num, monster_race *r_ptr, int slot);
extern void set_black_breath(int Ind);
extern void do_prob_travel(int Ind, int dir);
extern void move_player(int Ind, int dir, int do_pickup);
extern void run_step(int Ind, int dir);
extern void black_breath_infection(int Ind, int Ind2);
extern int see_wall(int Ind, int dir, int y, int x);
extern bool player_can_enter(int Ind, byte feature);
extern void hit_trap(int Ind);
extern int apply_dodge_chance(int Ind, int attack_level);
extern int apply_block_chance(player_type *p_ptr, int n);
extern int apply_parry_chance(player_type *p_ptr, int n);
extern void do_cmd_force_stack(int Ind, int item);
extern bool wraith_access(int Ind);
extern void whats_under_your_feet(int Ind);

/* cmd2.c */
extern cptr get_house_owner(struct c_special *cs_ptr);
extern bool access_door(int Ind, struct dna_type *dna, bool note);
extern int access_door_colour(int Ind, struct dna_type *dna);
extern void do_cmd_own(int Ind);
extern void do_cmd_go_up(int Ind);
extern void do_cmd_go_down(int Ind);
extern void do_cmd_search(int Ind);
extern void do_cmd_toggle_search(int Ind);
extern void do_cmd_open(int Ind, int dir);
extern void do_cmd_close(int Ind, int dir);
extern byte twall_erosion(worldpos *wpos, int y, int x);
extern void do_cmd_tunnel(int Ind, int dir, bool quiet_borer);
extern void do_cmd_disarm(int Ind, int dir);
extern void do_cmd_bash(int Ind, int dir);
extern void do_cmd_spike(int Ind, int dir);
extern void do_cmd_walk(int Ind, int dir, int pickup);
extern void do_cmd_stay(int Ind, int pickup);
extern int do_cmd_run(int Ind, int dir);
extern void do_cmd_fire(int Ind, int dir);
extern bool interfere(int Ind, int chance);
extern void do_cmd_throw(int Ind, int dir, int item, bool bashing);
extern void do_cmd_purchase_house(int Ind, int dir);
extern int pick_house(struct worldpos *wpos, int y, int x);
extern void house_admin(int Ind, int dir, char *args);
extern void do_cmd_cloak(int Ind);
extern void shadow_run(int Ind);
extern bool twall(int Ind, int y, int x);
extern int breakage_chance(object_type *o_ptr);
extern int get_shooter_mult(object_type *o_ptr);
extern bool get_something_tval(int Ind, int tval, int *ip);
extern void do_arrow_explode(int Ind, object_type *o_ptr, worldpos *wpos, int y, int x, int might);
extern bool retaliating_cmd;
extern void break_cloaking(int Ind, int discovered);
extern void break_shadow_running(int Ind);
extern void stop_cloaking(int Ind);
extern void stop_precision(int Ind);
extern void stop_shooting_till_kill(int Ind);
extern void do_cmd_fusion(int Ind);


/* cmd3.c */
extern void inven_drop(int Ind, int item, int amt);
extern void inven_takeoff(int Ind, int item, int amt);
extern void do_takeoff_impossible(int Ind);
extern void do_cmd_wield(int Ind, int item, u16b alt_slots);
extern void do_cmd_takeoff(int Ind, int item);
extern void do_cmd_drop(int Ind, int item, int quantity);
extern void do_cmd_drop_gold(int Ind, s32b amt);
extern void do_cmd_destroy(int Ind, int item, int quantity);
extern void do_cmd_observe(int Ind, int item);
extern void do_cmd_uninscribe(int Ind, int item);
extern void do_cmd_inscribe(int Ind, int item, cptr inscription);
extern void do_cmd_steal(int Ind, int dir);
extern void do_cmd_steal_from_monster(int Ind, int dir);
extern void do_cmd_refill(int Ind, int item);
extern bool do_auto_refill(int Ind);
extern void do_cmd_target(int Ind, int dir);
extern void do_cmd_target_friendly(int Ind, int dir);
extern void do_cmd_look(int Ind, int dir);
extern void do_cmd_locate(int Ind, int dir);
extern void do_cmd_query_symbol(int Ind, char sym);
extern bool item_tester_hook_wear(int Ind, int slot);

/* cmd4.c */
extern void do_cmd_check_artifacts(int Ind, int line);
extern void do_cmd_check_uniques(int Ind, int line);
extern void do_cmd_check_players(int Ind, int line);
extern void do_admin_cmd_check_players(int Ind, int line);
extern void do_cmd_check_player_equip(int Ind, int line);
extern void do_cmd_check_server_settings(int Ind);
extern void do_cmd_show_monster_killed_letter(int Ind, char *letter);
extern void do_cmd_show_houses(int Ind);
extern void do_cmd_show_known_item_letter(int Ind, char *letter);
extern void do_cmd_knowledge_traps(int Ind);
extern void do_cmd_knowledge_dungeons(int Ind);
extern void do_cmd_time(int Ind);
extern void do_cmd_check_other(int Ind, int line);
extern void do_cmd_check_other_prepare(int Ind, char *path);

/* cmd5.c */
extern bool check_antimagic(int Ind, int percentage);
extern void cast_school_spell(int Ind, int book, int spell, int dir, int item, int aux);
extern void do_cmd_browse(int Ind, object_type *o_ptr);
extern void do_cmd_study(int Ind, int book, int spell);
extern void do_cmd_psi(int Ind, int book, int spell);
extern void do_cmd_psi_aux(int Ind, int dir);
extern void do_cmd_hunt(int Ind, int book, int spell);
extern void do_cmd_cast(int Ind, int book, int spell);
extern void do_cmd_cast_aux(int Ind, int dir);
extern void do_cmd_sorc(int Ind, int book, int spell);
extern void do_cmd_sorc_aux(int Ind, int dir);
extern void do_mimic_change(int Ind, int r_idx, bool force);
extern void do_mimic_power_aux(int Ind, int dir);
extern void do_cmd_mimic(int Ind, int spell, int dir);/*w0t0w*/
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
extern void do_cmd_zap_rod(int Ind, int item, int dir);
extern void do_cmd_zap_rod_dir(int Ind, int dir);
extern bool activation_requires_direction(object_type *o_ptr);
extern void do_cmd_activate(int Ind, int item, int dir);
extern void do_cmd_activate_dir(int Ind, int dir);
extern bool unmagic(int Ind);
extern void fortune(int Ind, bool broadcast);
extern char random_colour(void);

extern void do_cmd_drink_fountain(int Ind);
extern void do_cmd_fill_bottle(int Ind);
extern void do_cmd_empty_potion(int Ind, int slot);
extern void do_cmd_fletchery(int Ind);
extern void do_cmd_stance(int Ind, int stance);
extern void do_cmd_melee_technique(int Ind, int technique);
extern void do_cmd_ranged_technique(int Ind, int technique);


/* control.c */
extern void SGWHit(int fd, int arg);
extern void SGWTimeout();
extern void NewConsole(int fd, int arg);
extern bool InitNewConsole(int write_fd);

/* dungeon.c */
extern void recall_player(int Ind, char *message);
extern int find_player(s32b id);
extern int find_player_name(char *name);
extern void play_game(bool new_game);
extern void shutdown_server(void);
extern void dungeon(void);
extern void pack_overflow(int Ind);
extern void set_runlevel(int val);
extern void store_turnover(void);
int has_ball (player_type *p_ptr);

extern void cheeze(object_type *o_ptr);
extern void cheeze_trad_house(void);

extern void house_contents_chmod(object_type *o_ptr);
extern void tradhouse_contents_chmod(void);

extern cptr value_check_aux1(object_type *o_ptr);
extern cptr value_check_aux2(object_type *o_ptr);
extern cptr value_check_aux1_magic(object_type *o_ptr);
extern cptr value_check_aux2_magic(object_type *o_ptr);

extern void world_surface_day(struct worldpos *wpos);
extern void world_surface_night(struct worldpos *wpos);
extern void player_day(int Ind);
extern void player_night(int Ind);
extern void player_dungeontown(int Ind);

extern void process_timers(void);
extern int timer_pvparena1, timer_pvparena2, timer_pvparena3;

extern void eff_running_speed(int *real_speed, player_type *p_ptr, cave_type *c_ptr);
extern void timed_shutdown(int k);

/* files.c */
extern int highscore_send(char *buffer, int max);
extern int houses_send(char *buffer, int max);
extern s16b tokenize(char *buf, s16b num, char **tokens, char delim1, char delim2);
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
extern void save_game_panic(void);
extern void signals_ignore_tstp(void);
extern void signals_handle_tstp(void);
extern void signals_init(void);
extern void kingly(int Ind);
extern void kingly2(int Ind);
extern void setup_exit_handler(void);
extern errr get_rnd_line(cptr file_name, int entry, char *output, int max_len);
extern void wipeout_needless_objects(void);
extern bool highscore_reset(int Ind);
extern bool highscore_remove(int Ind, int slot);
extern bool highscore_file_convert(int Ind);

/* generate.c */
extern void place_up_stairs(worldpos *wpos, int y, int x); 
extern void arcade_wipe(worldpos *wpos);
extern bool room_alloc(worldpos *wpos, int x, int y, bool crowded, int by0, int bx0, int *xx, int *yy);
extern bool dungeon_aux(int r_idx);
extern void add_dungeon(struct worldpos *wpos, int baselevel, int maxdep, int flags1, int flags2, bool tower, int type);
extern void rem_dungeon(struct worldpos *wpos, bool tower);
extern void alloc_dungeon_level(struct worldpos *wpos);
extern void dealloc_dungeon_level(struct worldpos *wpos);
extern void generate_cave(struct worldpos *wpos, player_type *p_ptr);
extern bool build_vault(struct worldpos *wpos, int yval, int xval, vault_type *v_ptr, player_type *p_ptr);
extern void place_fountain(struct worldpos *wpos, int y, int x);

extern void place_floor(worldpos *wpos, int y, int x);
extern void place_floor_live(worldpos *wpos, int y, int x);
extern int get_seasonal_tree(void);


/* wild.c */
extern int world_index(int world_x, int world_y);
extern void wild_bulldoze(void);
extern void init_wild_info(void);
extern void addtown(int y, int x, int base, u16b flags, int type);
extern void deltown(int Ind);
extern int determine_wilderness_type(struct worldpos *wpos);
extern void wilderness_gen(struct worldpos *wpos);
extern void set_mon_num_hook_wild(struct worldpos *wpos);
extern void wild_add_monster(struct worldpos *wpos);
extern bool fill_house(house_type *h_ptr, int func, void *data);
extern void wild_add_uhouse(house_type *h_ptr);
extern void wild_add_uhouses(struct worldpos *wpos);
extern bool reveal_wilderness_around_player(int Ind, int y, int x, int h, int w);
extern void wild_add_new_dungeons(void);

extern void initwild(void);
extern void genwild(void);
extern void wild_spawn_towns(void);
extern void init_wild_info_aux(int x, int y);

extern void wild_flags(int Ind, u32b flags);
extern void lively_wild(u32b flags);
extern void paint_house(int Ind, int x, int y, int k);
extern void wpos_apply_season_daytime(worldpos *wpos, cave_type **zcave);

/* init-txt.c */
extern errr init_v_info_txt(FILE *fp, char *buf);
extern errr init_f_info_txt(FILE *fp, char *buf);
extern errr init_k_info_txt(FILE *fp, char *buf);
extern errr init_a_info_txt(FILE *fp, char *buf);
extern errr init_e_info_txt(FILE *fp, char *buf);
extern errr init_d_info_txt(FILE *fp, char *buf);
extern errr init_r_info_txt(FILE *fp, char *buf);
extern errr init_re_info_txt(FILE *fp, char *buf);
extern errr init_t_info_txt(FILE *fp, char *buf);
extern errr init_ow_info_txt(FILE *fp, char *buf);
extern errr init_ba_info_txt(FILE *fp, char *buf);
extern errr init_st_info_txt(FILE *fp, char *buf);
extern errr init_s_info_txt(FILE *fp, char *buf);

/* init.c (init1.c , init2.c)*/
extern errr process_dungeon_file(cptr name, worldpos *wpos, int *yval, int *xval, int ymax, int xmax, bool init);
extern void init_file_paths(char *path);
extern void init_some_arrays(void);
extern void reinit_some_arrays(void);
extern bool load_server_cfg(void);

extern void init_schools(s16b new_size);
extern void init_spells(s16b new_size);

/* load1.c */
/*extern errr rd_savefile_old(void);*/

/* load2.c */
extern void load_guildhalls(struct worldpos *wpos);
extern void save_guildhalls(struct worldpos *wpos);
extern errr rd_savefile_new(int Ind);
extern errr rd_server_savefile(void);
extern bool wearable_p(object_type *o_ptr);

/* melee1.c */
/* melee2.c */
extern bool monst_check_grab(int m_idx, int mod, cptr desc);
extern int mon_will_run(int Ind, int m_idx);
extern bool monster_attack_normal(int m_idx, int tm_idx);
extern bool make_attack_melee(int Ind, int m_idx);
extern bool make_attack_spell(int Ind, int m_idx);
extern void process_monsters(void);
extern void curse_equipment(int Ind, int chance, int heavy_chance);
extern void process_npcs(void);

/* monster.c */
/* monster1.c monster2.c */
extern bool mon_allowed(monster_race *r_ptr);
extern bool mon_allowed_chance(monster_race *r_ptr);
extern bool mon_allowed_view(monster_race *r_ptr);
extern void heal_m_list(struct worldpos *wpos);
extern cptr r_name_get(monster_type *m_ptr);
extern monster_race* r_info_get(monster_type *m_ptr);
extern void delete_monster_idx(int i, bool unfound_art);
extern void delete_monster(struct worldpos *wpos, int y, int x, bool unfound_art);
extern void wipe_m_list(struct worldpos *wpos);
extern void thin_surface_spawns(void);
extern void geno_towns(void);
extern void wipe_m_list_roaming(struct worldpos *wpos);
extern void compact_monsters(int size, bool purge);
extern s16b m_pop(void);
extern int restrict_monster_to_dungeon(int r_idx, int dun_type);
extern errr get_mon_num_prep(int dun_type, char *reject_monsters);
extern s16b get_mon_num(int level, int dlevel);
extern void set_mon_num2_hook(int feat);
extern void monster_desc(int Ind, char *desc, int m_idx, int mode);
extern void monster_race_desc(int Ind, char *desc, int r_idx, int mode);
extern void lore_do_probe(int m_idx);
extern void lore_treasure(int m_idx, int num_item, int num_gold);
extern void update_mon(int m_idx, bool dist);
extern void update_mon_flicker(int m_idx);
extern void update_monsters(bool dist);
extern void update_player(int Ind);
extern void update_player_flicker(int Ind);
extern void update_players(void);
extern bool place_monster_aux(struct worldpos *wpos, int y, int x, int r_idx, bool slp, bool grp, int clo, int clone_summoning);
extern bool place_monster_one(struct worldpos *wpos, int y, int x, int r_idx, int ego, int randuni, bool slp, int clo, int clone_summoning);
extern bool place_monster(struct worldpos *wpos, int y, int x, bool slp, bool grp);
#ifdef RPG_SERVER
extern bool place_pet(int owner_id, struct worldpos *wpos, int y, int x, int r_idx);
#endif
extern bool alloc_monster(struct worldpos *wpos, int dis, int slp);
extern bool summon_specific(struct worldpos *wpos, int y1, int x1, int lev, int s_clone, int type, int allow_sidekicks, int clone_summoning);
extern bool summon_specific_race(struct worldpos *wpos, int y1, int x1, int r_idx, int s_clone, unsigned char num);
extern bool summon_specific_race_somewhere(struct worldpos *wpos, int r_idx, int s_clone, unsigned char num);
extern int summon_detailed_one_somewhere(struct worldpos *wpos, int r_idx, int ego, bool slp, int s_clone);
extern bool multiply_monster(int m_idx);
extern void update_smart_learn(int m_idx, int what);
extern void setup_monsters(void);
extern int race_index(char * name);
extern int monster_gain_exp(int m_idx, u32b exp, bool silent);
#ifdef MONSTER_INVENTORY
extern void monster_drop_carried_objects(monster_type *m_ptr);
#endif	/* MONSTER_INVENTORY */
extern bool monster_can_cross_terrain(byte feat, monster_race *r_ptr);

extern void monster_carry(monster_type *m_ptr, int m_idx, object_type *q_ptr);
extern void player_desc(int Ind, char *desc, int Ind2, int mode);
extern int monster_check_experience(int m_idx, bool silent);
extern bool mego_ok(int r_idx, int ego);

/* netserver.c (nserver.c) */
/* XXX those entries are duplicated with those in netserver.h
 * consider removing one of them */
/*extern void Contact(int fd, void *arg);*/
extern void world_connect(int Ind);
extern int Net_input(void);
extern int Net_output(void);
extern int Net_output1(int i);
extern void setup_contact_socket(void);
extern bool Report_to_meta(int flag);
extern void Start_evilmeta(void);
extern void Check_evilmeta(void);
extern int Setup_net_server(void);
extern bool Destroy_connection(int Ind, char *reason);
extern int Send_file_check(int ind, unsigned short id, char *fname);
extern int Send_file_init(int ind, unsigned short id, char *fname);
extern int Send_file_data(int ind, unsigned short id, char *buf, unsigned short len);
extern int Send_file_end(int ind, unsigned short id);
extern int Receive_file_data(int ind, unsigned short len, char *buffer);
extern int Send_plusses(int Ind, int tohit, int todam, int tohit_r, int todam_r, int tohit_m, int todam_m);
extern int Send_ac(int Ind, int base, int plus);
extern int Send_experience(int Ind, int lev, s32b max_exp, s32b cur_exp, s32b adv_exp);
#if 0
extern int Send_skill_init(int ind, int type, int i);
#else
extern int Send_skill_init(int ind, u16b i);
#endif
extern int Send_skill_points(int ind);
extern int Send_skill_info(int ind, int i);
extern int Send_gold(int Ind, s32b gold, s32b balance);
extern int Send_hp(int Ind, int mhp, int chp);
extern int Send_stamina(int Ind, int mst, int cst);
extern int Send_sp(int Ind, int msp, int csp);
extern int Send_char_info(int Ind, int race, int class, int trait, int sex, int mode);
extern int Send_various(int ind, int hgt, int wgt, int age, int sc, cptr body);
extern int Send_stat(int Ind, int stat, int max, int cur, int s_ind, int max_base);
extern int Send_history(int Ind, int line, cptr hist);
extern int Send_inven(int ind, char pos, byte attr, int wgt, object_type *o_ptr, cptr name);
extern int Send_inven_wide(int ind, char pos, byte attr, int wgt, object_type *o_ptr, cptr name);
extern int Send_equip(int Ind, char pos, byte attr, int wgt, object_type *o_ptr, cptr name);
extern int Send_title(int Ind, cptr title);
/*extern int Send_level(int Ind, int max, int cur);*/
/*extern void Send_exp(int Ind, s32b max, s32b cur);*/
extern int Send_depth(int Ind, struct worldpos *wpos);
extern int Send_depth_hack(int Ind, struct worldpos *wpos, bool town, cptr desc);
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
extern int Send_spell_info(int Ind, int realm, int book, int i, cptr out_val);
extern int Send_technique_info(int Ind); /* for MKEY_MELEE and MKEY_RANGED */
extern int Send_item_request(int Ind);
extern int Send_state(int Ind, bool paralyzed, bool searching, bool resting);
extern int Send_flush(int Ind);
extern int Send_line_info(int Ind, int y);
extern int Send_mini_map(int Ind, int y, byte *sa, char *sc);
extern int Send_store(int ind, char pos, byte attr, int wgt, int number, int price, cptr name, char tval, char sval, s16b pval);
extern int Send_store_wide(int ind, char pos, byte attr, int wgt, int number, int price, cptr name, char tval, char sval, s16b pval,
    byte xtra1, byte xtra2, byte xtra3, byte xtra4, byte xtra5, byte xtra6, byte xtra7, byte xtra8, byte xtra9);
extern int Send_store_special_str(int ind, char line, char col, char attr, char *str);
extern int Send_store_special_char(int ind, char line, char col, char attr, char c);
extern int Send_store_special_clr(int ind, char line_start, char line_end);
extern int Send_store_info(int ind, int num, cptr store, cptr owner, int items, int purse, byte attr, char c);
extern int Send_store_action(int ind, char pos, u16b bact, u16b action, cptr name, char attr, char letter, s16b cost, byte flag);
extern int Send_store_sell(int Ind, int price);
extern int Send_store_kick(int Ind);
extern int Send_target_info(int ind, int x, int y, cptr buf);
extern int Send_sound(int ind, int sound, int alternative, int type, int vol, s32b player_id);
#ifdef USE_SOUND_2010
extern int Send_music(int ind, int music);
#endif
extern int Send_beep(int ind);
extern int Send_AFK(int ind, byte afk);
extern int Send_encumberment(int ind, byte cumber_armor, byte awkward_armor, byte cumber_glove, byte heavy_wield, byte heavy_shield, byte heavy_shoot,
        byte icky_wield, byte awkward_wield, byte easy_wield, byte cumber_weight, byte monk_heavyarmor, byte rogue_heavyarmor, byte awkward_shoot);
extern int Send_special_line(int ind, int max, int line, byte attr, cptr buf);
extern int Send_floor(int ind, char tval);
extern int Send_pickup_check(int ind, cptr buf);
extern int Send_party(int ind);
extern int Send_special_other(int ind);
extern int Send_skills(int ind);
extern int Send_pause(int ind);
extern int Send_monster_health(int ind, int num, byte attr);
extern int Send_chardump(int ind, cptr tag);
extern int Send_unique_monster(int ind, int r_idx);
extern int Send_weather(int ind, int weather_type, int weather_wind, int weather_gen_speed, int weather_intensity, int weather_speed, bool update_clouds, bool revoke_clouds);
extern int Send_inventory_revision(int ind);
extern int Send_account_info(int ind);

extern int Send_request_key(int Ind, int id, char *prompt);
extern int Send_request_num(int Ind, int id, char *prompt, int std);
extern int Send_request_str(int Ind, int id, char *prompt, char *std);
extern int Send_request_cfr(int Ind, int id, char *prompt);
extern int Send_request_abort(int Ind);

extern void Handle_direction(int Ind, int dir);
extern void Handle_clear_buffer(int Ind);
extern int Send_sanity(int ind, byte attr, cptr msg);
extern char *showtime(void);
extern char *showdate(void);
extern void get_date(int *weekday, int *day, int *month, int *year);
extern void init_players(void);
extern int is_inactive(int Ind);

extern int Send_extra_status(int Ind, cptr status);
extern void change_mind(int Ind, bool open_or_close);



/* object1.c */
/* object2.c */
extern int divide_charged_item(object_type *o_ptr, int amt);
extern s16b unique_quark;
extern void object_copy(object_type *o_ptr, object_type *j_ptr);
extern s16b drop_near_severe(int Ind, object_type *o_ptr, int chance, struct worldpos *wpos, int y, int x);
extern void object_wipe(object_type *o_ptr);
extern bool can_use(int Ind, object_type *o_ptr);
extern bool can_use_verbose(int Ind, object_type *o_ptr);
extern bool can_use_admin(int Ind, object_type *o_ptr);
extern void flavor_init(void);
extern void reset_visuals(void);
extern void object_flags(object_type *o_ptr, u32b *f1, u32b *f2, u32b *f3, u32b *f4, u32b *f5, u32b *esp);
extern void object_desc(int Ind, char *buf, object_type *o_ptr, int pref, int mode);
extern void object_desc_store(int Ind, char *buf, object_type *o_ptr, int pref, int mode);
extern bool identify_fully_aux(int Ind, object_type *o_ptr);
extern void observe_aux(int Ind, object_type *o_ptr);
extern s16b index_to_label(int i);
extern s16b label_to_inven(int Ind, int c);
extern s16b label_to_equip(int Ind, int c);
extern s16b wield_slot(int Ind, object_type *o_ptr);
extern cptr mention_use(int Ind, int i);
extern cptr describe_use(int Ind, int i);
extern void inven_item_charges(int Ind, int item);
extern void inven_item_describe(int Ind, int item);
extern void inven_item_increase(int Ind, int item, int num);
extern bool inven_item_optimize(int Ind, int item);
extern void floor_item_charges(int item);
extern void floor_item_describe(int item);
extern void floor_item_increase(int item, int num);
extern void floor_item_optimize(int item);
extern void auto_inscribe(int Ind, object_type *o_ptr, int flags);
extern bool inven_carry_okay(int Ind, object_type *o_ptr);
extern s16b inven_carry(int Ind, object_type *o_ptr);
extern bool item_tester_okay(object_type *o_ptr);
extern void display_inven(int Ind);
extern void display_equip(int Ind);
extern byte get_book_name_color(int Ind, object_type *o_ptr);
/*extern void show_inven(void);
extern void show_equip(void);
extern void toggle_inven_equip(void);
extern bool get_item(int Ind, int *cp, cptr pmt, bool equip, bool inven, bool floor);*/
extern void delete_object_idx(int i, bool unfound_art);
extern void delete_object(struct worldpos *wpos, int y, int x, bool unfound_art);
extern void wipe_o_list(struct worldpos *wpos);
extern void apply_magic(struct worldpos *wpos, object_type *o_ptr, int lev, bool okay, bool good, bool great, bool verygreat, u32b resf);
extern void apply_magic_depth(int Depth, object_type *o_ptr, int lev, bool okay, bool good, bool great, bool verygreat, u32b resf);
extern void determine_level_req(int level, object_type *o_ptr);
extern void verify_level_req(object_type *o_ptr);
extern void place_object(struct worldpos *wpos, int y, int x, bool good, bool great, bool verygreat, u32b resf, obj_theme theme, int luck, byte removal_marker);
extern void generate_object(object_type *o_ptr, struct worldpos *wpos, bool good, bool great, bool verygreat, u32b resf, obj_theme theme, int luck);
extern void acquirement(struct worldpos *wpos, int y1, int x1, int num, bool great, bool verygreat, u32b resf);
extern void acquirement_direct(object_type *o_ptr, struct worldpos *wpos, bool great, bool verygreat, u32b resf);
extern void create_reward(int Ind, object_type *o_ptr, int min_lv, int max_lv, bool great, bool verygreat, u32b resf, long int treshold);
extern void give_reward(int Ind, u32b resf, cptr quark, int level, int discount);
extern void place_gold(struct worldpos *wpos, int y, int x, int bonus);
extern s16b drop_near(object_type *o_ptr, int chance, struct worldpos *wpos, int y, int x);
extern void pick_trap(struct worldpos *wpos, int y, int x);
extern void compact_objects(int size, bool purge);
extern s16b o_pop(void);
extern errr get_obj_num_prep(u32b resf);
extern errr get_obj_num_prep_tval(int tval, u32b resf); /* was written for create_reward(..) */
extern s16b get_obj_num(int max_level, u32b resf);
extern void object_known(object_type *o_ptr);
extern void object_aware(int Ind, object_type *o_ptr);
extern void object_tried(int Ind, object_type *o_ptr);
extern s64b object_value(int Ind, object_type *o_ptr);
extern bool object_similar(int Ind, object_type *o_ptr, object_type *j_ptr, s16b tolerance);
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
extern s64b object_value_real(int Ind, object_type *o_ptr);
extern s64b artifact_value_real(int Ind, object_type *o_ptr);
extern s32b flag_cost(object_type *o_ptr, int plusses);
extern s32b artifact_flag_cost(object_type *o_ptr, int plusses);
extern void eliminate_common_ego_flags(object_type *o_ptr, u32b *f1, u32b *f2, u32b *f3, u32b *f4, u32b *f5, u32b *esp);

extern void wipe_o_list_safely(struct worldpos *wpos);
extern void excise_object_idx(int o_idx);
extern int kind_is_legal(int k_idx, u32b resf);
extern void init_match_theme(obj_theme theme);

extern void kill_objs(int id);
extern u32b place_object_restrictor;

extern void handle_art_d(int aidx);
extern void handle_art_dnum(int aidx);
extern void handle_art_i(int aidx);
extern void handle_art_ipara(int aidx);
extern void handle_art_inum(int aidx);
extern void handle_art_inumpara(int aidx);

extern byte get_attr_from_tval(object_type *o_ptr);
extern bool anti_undead(object_type *o_ptr);
extern u32b make_resf(player_type *p_ptr);

extern void inven_index_slide(int Ind, s16b begin, s16b mod, s16b end);
extern void inven_index_move(int Ind, s16b slot, s16b new_slot);
extern void inven_index_erase(int Ind, s16b slot);
extern s16b replay_inven_changes(int Ind, s16b slot);
extern void inven_confirm_revision(int Ind, int revision);

#ifdef HOUSE_PAINTING
extern byte potion_col[MAX_COLORS];
#endif

/* party.c */
extern void account_check(int Ind);
extern bool WriteAccount(struct account *r_acc, bool new);
extern int validate(char *name);
extern int invalidate(char *name);
extern int makeadmin(char *name);
extern int acc_set_flags(char *name, u32b flags, bool set);
extern int acc_set_flags_id(u32b id, u32b flags, bool set);
extern u32b acc_get_flags(char *name);
extern void sf_delete(const char *name);
extern struct account *GetAccount(cptr name, char *pass, bool leavepass);
extern struct account *GetAccountID(u32b id, bool leavepass);
extern struct account *Admin_GetAccount(cptr name);
extern void set_pkill(int Ind, int delay);
extern int guild_lookup(cptr name);
extern int party_lookup(cptr name);
extern bool player_in_party(int party_id, int Ind);
extern int party_create(int Ind, cptr name);
extern int party_create_ironteam(int Ind, cptr name);
extern int party_add(int adder, cptr name);
extern int party_remove(int remover, cptr name);
extern void party_leave(int Ind);
extern void party_msg(int party_id, cptr msg);
extern void party_msg_format(int party_id, cptr fmt, ...);
extern void floor_msg_format(struct worldpos *wpos, cptr fmt, ...);
extern void world_surface_msg(cptr msg);
extern void party_gain_exp(int Ind, int party_id, s64b amount, s64b base_amount, int henc);
extern int guild_create(int Ind, cptr name);
extern int guild_add(int adder, cptr name);
extern int guild_remove(int remover, cptr name);
extern void guild_leave(int Ind);
extern void guild_msg(int guild_id, cptr msg);
extern void guild_msg_format(int guild_id, cptr fmt, ...);
extern bool add_hostility(int Ind, cptr name, bool initiator);
extern bool remove_hostility(int Ind, cptr name);
extern bool add_ignore(int Ind, cptr name);
extern bool check_ignore(int attacker, int target);
extern bool check_hostile(int attacker, int target);
extern cptr lookup_accountname(int p_id);
extern cptr lookup_player_name(int id);
#ifdef AUCTION_SYSTEM
extern s32b lookup_player_au(int id);
extern s32b lookup_player_balance(int id);
#endif
extern int lookup_player_id(cptr name);
extern int lookup_player_id_messy(cptr name);
/* another arg, and its getting a struct... pfft. */
extern void add_player_name(cptr name, int id, u32b account, byte race, byte class, byte mode, byte level, u16b party, byte guild, u16b quest, time_t laston);
extern void delete_player_id(int id);
extern void delete_player_name(cptr name);
extern int player_id_list(int **list, u32b account);
extern void stat_player(char *name, bool on);
extern time_t lookup_player_laston(int id);
extern byte lookup_player_level(int id);
//extern byte lookup_player_party(int id);
extern s32b lookup_player_party(int id);
extern byte lookup_player_mode(int id);
extern u32b lookup_player_account(int id);
extern void clockin(int Ind, int type);
extern int newid(void);

extern void scan_players(void);
extern void scan_accounts(void);
extern void erase_player_name(char *pname);
extern void rename_player_name(char *pnames);
extern void checkexpiry(int Ind, int days);
extern void account_checkexpiry(int Ind);
extern void party_check(int Ind);
extern void party_msg_format_ignoring(int sender, int party_id, cptr fmt, ...);
extern u16b lookup_player_type(int id);
extern int check_account(char *accname, char *c_name);
extern void strip_true_arts_from_hashed_players(void);
extern void verify_player(cptr name, int id, u32b account, byte race, byte class, byte mode, byte level, u16b party, byte guild, u16b quest, time_t laston);
extern void account_change_password(int Ind, char *old_pass, char *new_pass);

extern int lookup_player_ind(u32b id);

/* printout.c */
extern int s_print_only_to_file(int which);
extern int s_setup(char *str);
extern int s_shutdown(void);
extern int s_printf(const char *str, ...);
extern bool s_setupr(char *str);
extern bool rfe_printf(char *str, ...);
extern bool do_cmd_view_rfe(int Ind,char *str, int line);
extern int c_printf(char *str, ...);
extern int p_printf(char *str, ...);
extern int l_printf(char *str, ...);

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
extern byte spell_color(int type);
extern bool spell_color_animation(int type);
extern void take_xp_hit(int Ind, int damage, cptr hit_from, bool mode, bool fatal);
extern void take_sanity_hit(int Ind, int damage, cptr hit_from);
extern s16b poly_r_idx(int r_idx);
extern bool check_st_anchor(struct worldpos *wpos, int y, int x);
extern bool teleport_away(int m_idx, int dis);
extern bool teleport_player(int Ind, int dis, bool ignore_pvp);
extern void teleport_player_force(int Ind, int dis);
extern void teleport_player_to(int Ind, int ny, int nx);
extern void teleport_player_level(int Ind);
extern bool bypass_invuln;
extern bool bypass_inscrption;
extern void take_hit(int Ind, int damage, cptr kb_str, int Ind_attacker);
extern int acid_dam(int Ind, int dam, cptr kb_str, int Ind_attacker);
extern int elec_dam(int Ind, int dam, cptr kb_str, int Ind_attacker);
extern int fire_dam(int Ind, int dam, cptr kb_str, int Ind_attacker);
extern int cold_dam(int Ind, int dam, cptr kb_str, int Ind_attacker);
extern bool inc_stat(int Ind, int stat);
extern bool dec_stat(int Ind, int stat, int amount, int mode);
extern bool res_stat(int Ind, int stat);
extern bool apply_disenchant(int Ind, int mode);
extern bool apply_discharge(int Ind, int dam);
extern bool apply_discharge_item(int o_idx, int dam);
extern bool project_hook(int Ind, int typ, int dir, int dam, int flg, char *attacker);
extern bool project(int who, int rad, struct worldpos *wpos, int y, int x, int dam, int typ, int flg, char *attacker);
extern int set_all_destroy(object_type *o_ptr);
extern int set_cold_destroy(object_type *o_ptr);
extern int set_impact_destroy(object_type *o_ptr);
extern int set_water_destroy(object_type *o_ptr);
extern int equip_damage(int Ind, int typ);
extern int inven_damage(int Ind, inven_func typ, int perc);
extern int weapon_takes_damage(int Ind, int typ, int slot);
extern int shield_takes_damage(int Ind, int typ);

extern bool potion_smash_effect(int who, worldpos *wpos, int y, int x, int o_sval);
extern void teleport_to_player(int Ind, int m_idx);

extern bool hates_fire(object_type *o_ptr);
extern bool hates_water(object_type *o_ptr);

extern int safe_area(int Ind);
extern int approx_damage(int m_idx, int dam, int typ);

/* spells2.c */
extern void summon_pet(int Ind, int max);
extern bool place_foe(int owner_id, struct worldpos *wpos, int y, int x, int r_idx);
extern void swap_position(int Ind, int lty, int ltx);
extern void grow_trees(int Ind, int rad);
extern bool heal_insanity(int Ind, int val);
extern void summon_cyber(int Ind, int s_clone, int clone_summoning);
extern void golem_creation(int Ind, int max);
#ifdef RPG_SERVER
extern char pet_creation(int Ind);
#endif
extern bool hp_player(int Ind, int num);
extern bool hp_player_quiet(int Ind, int num, bool autoeffect);
extern void warding_glyph(int Ind);
extern void flash_bomb(int Ind);
extern bool do_dec_stat(int Ind, int stat, int mode);
extern bool do_dec_stat_time(int Ind, int stat, int mode, int sust_chance, int reduction_mode, bool msg);
extern bool do_res_stat(int Ind, int stat);
extern bool do_inc_stat(int Ind, int stat);
extern void identify_pack(int Ind);
extern void message_pain(int Ind, int m_idx, int dam);
extern bool remove_curse(int Ind);
extern bool remove_all_curse(int Ind);
extern bool restore_level(int Ind);
extern void self_knowledge(int Ind);
extern bool lose_all_info(int Ind);
extern bool detect_monsters_xxx(int Ind, u32b match_flag);
extern bool detect_treasure(int Ind, int rad);
extern bool floor_detect_treasure(int Ind);
extern bool detect_magic(int Ind, int rad);
extern bool detect_invisible(int Ind);
extern bool detect_evil(int Ind);
extern bool detect_creatures(int Ind);
extern void detect_monsters_forced(int Ind);
extern bool detection(int Ind, int rad);
extern bool detect_bounty(int Ind, int rad);
extern bool detect_object(int Ind, int rad);
extern bool detect_trap(int Ind, int rad);
extern bool detect_sdoor(int Ind, int rad);
extern void stair_creation(int Ind);
extern bool enchant(int Ind, object_type *o_ptr, int n, int eflag);
extern bool enchant_spell(int Ind, int num_hit, int num_dam, int num_ac, int flags);
extern bool enchant_spell_aux(int Ind, int item, int num_hit, int num_dam, int num_ac, int flags);
extern bool ident_spell(int Ind);
extern bool ident_spell_aux(int Ind, int item);
extern bool identify_fully(int Ind);
extern bool identify_fully_item(int Ind, int item);
extern bool identify_fully_item_quiet(int Ind, int item);
extern bool recharge(int Ind, int num);
extern bool recharge_aux(int Ind, int item, int num);
extern bool speed_monsters(int Ind);
extern bool slow_monsters(int Ind);
extern bool sleep_monsters(int Ind);
extern bool fear_monsters(int Ind);
extern bool stun_monsters(int Ind);
extern void wakeup_monsters(int Ind, int who);
extern void wakeup_monsters_somewhat(int Ind, int who);
extern void aggravate_monsters(int Ind, int who);
extern void aggravate_monsters_floorpos(worldpos *wpos, int x, int y);
extern void wake_minions(int Ind, int who);
extern void taunt_monsters(int Ind);
extern void distract_monsters(int Ind);
extern bool genocide_aux(int Ind, worldpos *wpos, char typ);
extern bool genocide(int Ind);
extern bool mass_genocide(int Ind);
extern bool probing(int Ind);
extern bool project_hack(int Ind, int typ, int dam, char *attacker);
extern bool banish_evil(int Ind, int dist);
extern bool dispel_evil(int Ind, int dam);
extern bool dispel_undead(int Ind, int dam);
extern bool dispel_demons(int Ind, int dam);
extern bool dispel_monsters(int Ind, int dam);
extern bool turn_undead(int Ind);
extern void destroy_area(struct worldpos *wpos, int y1, int x1, int r, bool full, byte feat, int stun);
extern void earthquake(struct worldpos *wpos, int cy, int cx, int r);
extern void wipe_spell(struct worldpos *wpos, int cy, int cx, int r);
extern void lite_room(int Ind, struct worldpos *wpos, int y1, int x1);
extern void unlite_room(int Ind, struct worldpos *wpos, int y1, int x1);
extern bool lite_area(int Ind, int dam, int rad);
extern bool unlite_area(int Ind, int dam, int rad);
extern bool fire_ball(int Ind, int typ, int dir, int dam, int rad, char *attacker);
extern bool fire_wall(int Ind, int typ, int dir, int dam, int time, int interval, char *attacker);
extern bool fire_cloud(int Ind, int typ, int dir, int dam, int rad, int time, int interval, char *attacker);
extern bool fire_wave(int Ind, int typ, int dir, int dam, int rad, int time, int interval, s32b eff, char *attacker);
extern bool cast_raindrop(worldpos *wpos, int x);
extern bool cast_snowflake(worldpos *wpos, int x, int interval);
extern bool cast_fireworks(worldpos *wpos, int x, int y);
extern bool fire_bolt(int Ind, int typ, int dir, int dam, char *attacker);
extern bool fire_beam(int Ind, int typ, int dir, int dam, char *attacker);
extern bool fire_bolt_or_beam(int Ind, int prob, int typ, int dir, int dam, char *attacker);
extern bool fire_grid_bolt(int Ind, int typ, int dir, int dam, char *attacker);
extern bool fire_grid_beam(int Ind, int typ, int dir, int dam, char *attacker);
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
extern bool trap_creation(int Ind, int mod, int rad);
extern bool destroy_doors_touch(int Ind, int rad);
extern bool sleep_monsters_touch(int Ind);
extern bool create_artifact(int Ind);
extern bool create_artifact_aux(int Ind, int item);
extern bool curse_spell(int Ind);
extern bool curse_spell_aux(int Ind, int item);
extern void house_creation(int Ind, bool floor, bool jail);
extern bool item_tester_hook_recharge(object_type *o_ptr);
extern bool do_vermin_control(int Ind);
extern void tome_creation(int Ind);
extern void tome_creation_aux(int Ind, int item);

extern bool create_garden(int Ind, int level);
extern bool do_banish_animals(int Ind, int chance);
extern bool do_xtra_stats(int Ind, int p, int v);
extern bool do_focus_shot(int Ind, int p, int v);

extern void divine_vengeance(int Ind, int power);
extern void divine_empowerment(int Ind, int level);
extern void divine_intensify(int Ind, int level);

extern bool do_divine_xtra_res_time_mana(int Ind, int p);
extern bool do_divine_hp(int Ind, int p, int v);
extern bool do_divine_crit(int Ind, int p, int v);

extern void do_autokinesis_to(int Ind, int dis);
extern bool do_res_stat_temp(int Ind, int stat);
extern void swap_position(int Ind, int lty, int ltx);
extern void call_chaos(int Ind, int dir, int extra_damage);
extern bool turn_monsters(int Ind, int dam);
extern void wizard_lock(int Ind, int dir);
extern void do_mstopcharm(int Ind);
extern bool test_charmedignore(int Ind, int Ind_charmer, monster_race *r_ptr);


/* store.c */
extern int store_debug_mode, store_debug_quickmotion, store_debug_startturn;
extern void store_debug_stock();

extern void alloc_stores(int townval);
extern void dealloc_stores(int townval);
extern void store_purchase(int Ind, int item, int amt);
extern void store_sell(int Ind, int item, int amt);
extern void store_confirm(int Ind);
extern void do_cmd_store(int Ind);
extern void store_shuffle(store_type *st_ptr);
extern void store_maint(store_type *st_ptr);
extern void store_init(store_type *st_ptr);
extern void store_kick(int Ind, bool say);
extern void store_stole(int Ind, int item);

extern void do_cmd_trad_house(int Ind);
#ifdef PLAYER_STORES
extern bool do_cmd_player_store(int Ind, int x, int y);
extern void player_stores_cut_inscription(char *o_name);
extern u32b ps_get_cheque_value(object_type *o_ptr);
#endif
extern void store_examine(int Ind, int item);
extern void store_exec_command(int Ind, int action, int item, int item2, int amt, int gold);
extern void home_extend(int Ind);
extern void view_cheeze_list(int Ind);
extern void reward_deed_item(int Ind, int item);
extern void reward_deed_blessing(int Ind, int item);

#ifdef AUCTION_SYSTEM
extern void process_auctions();
extern char *auction_format_time();
extern bool auction_mode_check(int Ind, int auction_id);
extern void auction_player_joined(int Ind);
extern void auction_player_death(s32b Ind);
extern void auction_print_error(int Ind, int error);
extern int auction_set(int Ind, int slot, cptr starting_price_string, cptr buyout_price_string, cptr duration_string);
extern int auction_start(int Ind);
extern int auction_place_bid(int Ind, int auction_id, cptr bid_string);
extern int auction_buyout(int Ind, int auction_id);
extern int auction_cancel(int Ind, int auction_id);
extern void auction_list(int Ind);
extern void auction_search(int Ind, cptr search);
extern void auction_retrieve_items(int Ind, int *retrieved, int *unretrieved);
extern int auction_show(int Ind, int auction_id);
extern int auction_examine(int Ind, int auction_id);
#endif

extern void handle_store_leave(int Ind);

/* util.c */
extern bool suppress_message;
/* The next buffers are for catching the chat */
extern char last_chat_line[160]; /* What was said */ 
extern char last_chat_owner[20]; /* Who said it */
// extern char last_chat_prev[160]; /* What was said before the above*/

extern void use_ability_blade(int Ind);
extern void check_parryblock(int Ind);
extern void toggle_shoot_till_kill(int Ind);
extern void toggle_dual_mode(int Ind);
extern bool show_floor_feeling(int Ind);
extern void msg_admin(cptr fmt, ...);
extern int name_lookup_loose(int Ind, cptr name, u16b party);
extern int name_lookup_loose_quiet(int Ind, cptr name, u16b party);
extern int name_lookup(int Ind, cptr name, u16b party);
extern int name_lookup_quiet(int Ind, cptr name, u16b party);
extern errr path_parse(char *buf, int max, cptr file);
extern errr path_temp(char *buf, int max);
extern FILE *my_fopen(cptr file, cptr mode);
extern errr my_fgets(FILE *fff, char *buf, huge n, bool conv);
extern int get_playerind(char *name);
extern int get_playerind_loose(char *name);
extern int get_playerslot_loose(int Ind, char *iname);
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
extern void bell(void);
#ifdef USE_SOUND_2010
extern void sound(int Ind, cptr name, cptr alternative, int type, bool nearby);
extern void sound_near(int Ind, cptr name, cptr alternative, int type);
extern void sound_near_site(int y, int x, worldpos *wpos, int Ind, cptr name, cptr alternative, int type, bool viewable);
extern void sound_near_monster(int m_idx, cptr name, cptr alternative, int type);
extern void handle_music(int Ind);
extern void sound_item(int Ind, int tval, int sval, cptr action);
#else
extern void sound(int Ind, int num);
#endif
extern void text_to_ascii(char *buf, cptr str);
extern void ascii_to_text(char *buf, cptr str);
extern void keymap_init(void);
extern char inkey(void);
extern cptr quark_str(s16b num);
extern s16b quark_add(cptr str);
extern void note_crop_pseudoid(char *s2, char *psid, cptr s);
extern bool check_guard_inscription(s16b quark, char what);
extern void msg_print(int Ind, cptr msg);
extern void msg_broadcast(int Ind, cptr msg);
extern void msg_admins(int Ind, cptr msg);
extern void msg_format(int Ind, cptr fmt, ...);
extern void msg_print_near(int Ind, cptr msg);
extern void msg_print_verynear(int Ind, cptr msg);
extern void msg_format_near(int Ind, cptr fmt, ...);
extern void msg_format_verynear(int Ind, cptr fmt, ...);
extern void msg_print_near_site(int y, int x, worldpos *wpos, int Ind, bool view, cptr msg);
extern void msg_format_near_site(int y, int x, worldpos *wpos, int Ind, bool view, cptr fmt, ...);
extern void msg_print_near_monster(int m_idx, cptr msg);
extern void msg_party_format(int Ind, cptr fmt, ...);
extern void msg_guild_format(int Ind, cptr fmt, ...);
extern void un_afk_idle(int Ind);
extern void toggle_afk(int Ind, char *msg);
extern void player_talk(int Ind, char *msg);
extern bool is_a_vowel(int ch);
extern char *wpos_format(int Ind, worldpos *wpos);
extern s32b bst(s32b what, s32b t);
extern cptr get_month_name(int day, bool full, bool compact);
extern cptr get_day(int day);
extern void bracer_ff(char *buf);

extern void check_banlist(void);
extern void msg_broadcast_format(int Ind, cptr fmt, ...);
extern byte count_bits(u32b array);
extern int gold_colour(int amt);
extern int test_item_name(cptr name);
extern bool in_banlist(char *addr);

extern void bbs_add_line(cptr textline);
extern void bbs_del_line(int entry);
extern void bbs_erase(void);
extern void pbbs_add_line(u16b party, cptr textline);
extern void gbbs_add_line(byte guild, cptr textline);

extern void player_list_add(player_list_type **list, s32b player);
extern bool player_list_find(player_list_type *list, s32b player);
extern bool player_list_del(player_list_type **list, s32b player);
extern void player_list_free(player_list_type *list);

extern bool is_newer_than(version_type *version, int major, int minor, int patch, int extra, int branch, int build);
extern void my_memfrob(void *s, int n);

extern cptr compat_pmode(int Ind1, int Ind2);
extern cptr compat_pomode(int Ind, object_type *o_ptr);
extern cptr compat_omode(object_type *o1_ptr, object_type *o2_ptr);

extern char *html_escape(const char *str);
extern void do_benchmark(int Ind);
#if (MAX_PING_RECVS_LOGGED > 0)
extern cptr timediff(struct timeval *begin, struct timeval *end);
#endif
extern void strip_control_codes(char *ss, char *s);
extern cptr flags_str(u32b flags);

/* xtra1.c */
extern void cnv_stat(int val, char *out_val);
extern s16b modify_stat_value(int value, int amount);
extern void notice_stuff(int Ind);
extern void update_stuff(int Ind);
extern void redraw_stuff(int Ind);
extern void window_stuff(int Ind);
extern void handle_stuff(int Ind);
extern void fix_spell(int Ind, bool full);
extern void calc_mana(int Ind);

extern void calc_hitpoints(int Ind);
extern void calc_boni(int Ind);
extern void calc_body_spells(int Ind);
extern int get_archery_skill(player_type *p_ptr);
extern int get_weaponmastery_skill(player_type *p_ptr, object_type *o_ptr);
extern int calc_blows_obj(int Ind, object_type *o_ptr);
extern int calc_blows_weapons(int Ind);
extern int calc_crit_obj(int Ind, object_type *o_ptr);

extern int start_global_event(int Ind, int getype, char *parm);
extern void stop_global_event(int Ind, int n);
extern void announce_global_event(int ge_id);
extern void process_global_events(void);
extern void global_event_signup(int Ind, int n, cptr parm);

extern void update_check_file(void);
extern void clear_current(int Ind);

extern void calc_techniques(int Ind);
extern int get_esp_link(int Ind, u32b flags, player_type **p2_ptr);
extern void use_esp_link(int *Ind, u32b flags);

extern void handle_request_return_str(int Ind, int id, char *str);
extern void handle_request_return_key(int Ind, int id, char c);
extern void handle_request_return_num(int Ind, int id, int num);
extern void handle_request_return_cfr(int Ind, int id, bool cfr);

/* xtra2.c */

extern void set_pushed(int Ind, int dir);
extern bool set_brand(int Ind, int v, int t, int p);
extern s16b questid;
extern bool imprison(int Ind, u16b time, char *reason);
extern bool guild_build(int Ind);
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
extern bool set_fury(int Ind, int v);
extern bool set_tim_meditation(int Ind, int v);
extern bool set_tim_wraith(int Ind, int v);
extern bool set_blind(int Ind, int v);
extern bool set_confused(int Ind, int v);
extern bool set_poisoned(int Ind, int v, int attacker);
extern bool set_afraid(int Ind, int v);
extern bool set_paralyzed(int Ind, int v);
extern bool set_image(int Ind, int v);
extern bool set_fast(int Ind, int v, int p);
extern bool set_slow(int Ind, int v);
extern bool set_tim_thunder(int Ind, int v, int p1, int p2);
extern bool set_tim_regen(int Ind, int v, int p);
extern bool set_tim_trauma(int Ind, int v, int p);
extern bool set_tim_deflect(int Ind, int v);
extern bool set_tim_ffall(int Ind, int v);
extern bool set_tim_fly(int Ind, int v);
extern bool set_shield(int Ind, int v, int p, s16b o, s16b d1, s16b d2);
extern bool set_blessed(int Ind, int v);
extern bool set_res_fear(int Ind, int v);
extern bool set_hero(int Ind, int v);
extern bool set_shero(int Ind, int v);
extern bool set_berserk(int Ind, int v);
extern bool set_melee_sprint(int Ind, int v);
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
extern bool set_cut(int Ind, int v, int attacker);
extern bool set_mindboost(int Ind, int p, int v);
extern bool set_kinetic_shield(int Ind, int v);
extern bool set_food(int Ind, int v);
extern int get_player(int Ind, object_type *o_ptr);
extern int get_monster(int Ind, object_type *o_ptr);
extern void blood_bond(int Ind, object_type * o_ptr);
extern bool check_blood_bond(int Ind, int Ind2);
extern void remove_blood_bond(int Ind, int Ind2);
extern void set_recall_depth(player_type * p_ptr, object_type * o_ptr);
extern bool set_recall_timer(int Ind, int v);
extern bool set_recall(int Ind, int v, object_type * o_ptr);
extern void check_experience(int Ind);
extern void gain_exp(int Ind, s64b amount);
extern void lose_exp(int Ind, s32b amount);
extern bool mon_take_hit_mon(int am_idx, int m_idx, int dam, bool *fear, cptr note);
extern void monster_death_mon(int am_idx, int m_idx);
extern void monster_death(int Ind, int m_idx);
extern void player_death(int Ind);
extern void resurrect_player(int Ind, int exploss);
extern void del_quest(int id);
extern void rem_quest(u16b id);
extern void kill_quest(int Ind);
extern bool add_quest(int Ind, int target, u16b type, u16b num, u16b flags);
extern bool prepare_quest(int Ind, int j, u16b flags, int *lev, u16b *type, u16b *num);
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
extern bool ang_sort_comp_tval(int Ind, vptr u, vptr v, int a, int b);
extern void ang_sort_swap_s16b(int Ind, vptr u, vptr v, int a, int b);
extern bool ang_sort_comp_mon_lev(int Ind, vptr u, vptr v, int a, int b);
extern bool target_able(int Ind, int m_idx);
extern bool target_okay(int Ind);
extern s16b target_pick(int Ind, int y1, int x1, int dy, int dx);
extern bool target_set(int Ind, int dir);
extern bool target_set_friendly(int Ind, int dir, ...);
extern bool get_aim_dir(int Ind/*, int *dp*/);
extern bool get_item(int Ind);
extern bool do_scroll_life(int Ind);
extern bool do_restoreXP_other(int Ind);
extern bool telekinesis(int Ind, object_type *o_ptr);
extern void telekinesis_aux(int Ind, int item);
extern bool set_bow_brand(int Ind, int v, int t, int p);

extern bool bless_temp_luck(int Ind, int pow, int dur);

extern void toggle_aura(int Ind, int aura);
extern void check_aura(int Ind, int aura);

extern bool master_level(int Ind, char * parms);
extern bool master_build(int Ind, char * parms);
extern bool master_summon(int Ind, char * parms);
extern bool master_generate(int Ind, char * parms);
extern bool master_acquire(int Ind, char * parms);
extern bool master_player(int Ind, char * parms);

extern void kill_houses(int id, byte type);
extern void kill_house_contents(house_type *h_ptr);

/*extern bool get_rep_dir(int *dp);*/

extern bool c_get_item(int *cp, cptr pmt, bool equip, bool inven, bool floor);
extern void check_quests(void);
extern bool master_level_specific(int Ind, struct worldpos *wpos, char * parms);
extern void unstatic_level(struct worldpos *wpos);

extern int det_exp_level(int lev);

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
extern void init_lua(void);
extern void reinit_lua(void);
extern bool pern_dofile(int Ind, char *file);
extern int exec_lua(int Ind, char *file);
extern cptr string_exec_lua(int Ind, char *file);
extern void master_script_begin(char *name, char mode);
extern void master_script_end(void);
extern void master_script_line(char *buf);
extern void master_script_exec(int Ind, char *name);
extern void cat_script(int Ind, char *name);

/* traps.c */
extern bool do_player_drop_items(int Ind, int chance, bool trap);
extern bool do_player_scatter_items(int Ind, int chance, int rad);
extern bool player_activate_trap_type(int Ind, s16b y, s16b x, object_type *i_ptr, s16b item);
extern void player_activate_door_trap(int Ind, s16b y, s16b x);
extern void place_trap(struct worldpos *wpos, int y, int x, int mod);
extern void place_trap_specific(struct worldpos *wpos, int y, int x, int mod, int found);

extern void place_trap_object(object_type *o_ptr);
extern void do_cmd_set_trap(int Ind, int item_kit, int item_load);
extern void do_cmd_disarm_mon_trap_aux(worldpos *wpos, int y, int x);
extern bool mon_hit_trap(int m_idx);

extern void wiz_place_trap(int Ind, int trap);


extern char	*longVersion;
extern char	*shortVersion;


/* skills.c */
extern void increase_skill(int Ind, int i, bool quiet);
extern void init_skill(player_type *p_ptr, u32b value, s16b mod, int i);
extern s16b get_skill(player_type *p_ptr, int skill);
extern s16b get_skill_scale(player_type *p_ptr, int skill, u32b scale);
extern s16b get_skill_scale_fine(player_type *p_ptr, int skill, u32b scale);
extern void compute_skills(player_type *p_ptr, s32b *v, s32b *m, int i);
extern s16b find_skill(cptr name);
extern void msg_gained_abilities(int Ind, int old_value, int i);
extern void respec_skill(int Ind, int i, bool update_skill, bool polymorph);
extern void respec_skills(int Ind, bool update_skills);
extern int invested_skill_points(int Ind, int i);

/* hooks.c */
extern bool process_hooks(int h_idx, char *fmt, ...);
extern hooks_chain* add_hook(int h_idx, cptr script, cptr name);
extern void dump_hooks(void);
extern void wipe_hooks(void);

/* bldg.c */
extern bool is_state(int Ind, store_type *s_ptr, int state);
extern void show_building(int Ind, store_type *s_ptr);
extern bool bldg_process_command(int Ind, store_type *s_ptr, int action, int item, int item2, int amt, int gold);

/* lua_bind.c */
s32b lua_get_level(int Ind, s32b s, s32b lvl, s32b max, s32b min, s32b bonus);
s32b lua_spell_chance(int i, s32b chance, int level, int skill_level, int mana, int cur_mana, int stat);
s16b new_school(int i, cptr name, s16b skill);
s16b new_spell(int i, cptr name);
spell_type *grab_spell_type(s16b num);
school_type *grab_school_type(s16b num);
void lua_s_print(cptr logstr);
void lua_add_anote(char *anote);
void lua_count_houses(int Ind);
void lua_recalc_char(int Ind);
void lua_examine_item(int Ind, int Ind_target, int item);
void lua_determine_level_req(int Ind, int item);
void lua_strip_true_arts_from_absent_players(void);
void lua_strip_true_arts_from_present_player(int Ind, int mode);
void lua_strip_true_arts_from_floors(void);
void lua_check_player_for_true_arts(int Ind);
int lua_get_mon_lev(int r_idx);
char *lua_get_mon_name(int r_idx);
char *lua_get_last_chat_owner();
char *lua_get_last_chat_line();
extern bool first_player_joined;
void lua_towns_treset(void);
long lua_player_exp(int level, int expfact);
extern void lua_fix_spellbooks(int spell, int mod);
extern void lua_arts_fix(int Ind);
void lua_get_pgestat(int Ind, int n);
void lua_start_global_event(int Ind, int evtype, char *parm);
void lua_apply_item_changes(int Ind, int changes);
void lua_apply_item_changes_aux(object_type *o_ptr, int changes);
extern void lua_set_floor_flags(int Ind, u32b flags);
extern s32b lua_get_skill_mod(int Ind, int i);
extern s32b lua_get_skill_value(int Ind, int i);
extern void lua_fix_equip_slots(int Ind);
extern int get_inven_sval(int Ind, int inven_slot);
extern int get_inven_xtra(int Ind, int inven_slot, int n);
extern void lua_fix_skill_chart(int Ind);
extern void lua_takeoff_costumes(int Ind);
extern bool lua_is_unique(int r_idx);
/* only called once, in util.c, referring to new file slash.c */
extern void do_slash_cmd(int Ind, char *message);
extern int global_luck; /* Global +LUCK modifier for the whole server (change the 'weather' - C. Blue) */
extern void lua_intrusion(int Ind, char *problem_diz);
extern bool lua_mimic_egligible(int Ind, int r_idx);

#ifdef ENABLE_GO_GAME
/* go.c - C. Blue */
extern bool go_engine_up, go_game_up;
extern int go_engine_processing;
extern u32b go_engine_player_id;

extern int go_engine_init(void);
extern void go_engine_terminate(void);
extern void go_engine_process(void);
extern void go_challenge(int Ind);
extern void go_challenge_accept(int Ind, bool new_wager);
extern void go_challenge_start(int Ind);
extern void go_challenge_cancel(void);
extern int go_engine_move_human(int Ind, char *py_move);
extern void go_engine_clocks(void);
#endif

/* Watch if someone enters Nether Realm or challenges Morgoth - C. Blue
   Dungeon masters will be paged if they're not AFK or if they have
   'watch' as AFK reason! */
extern bool watch_nr;
extern bool watch_morgoth;

/* lua-dependant 'constants' */
extern int __lua_HHEALING;
extern int __lua_HBLESSING;
extern int __lua_MSCARE;
extern int __lua_M_FIRST;
extern int __lua_M_LAST;

extern int cron_1h_last_hour; /* manage cron_1h calls */
extern int regen_boost_stamina;

/* default 'updated_savegame' value for newly created chars [0].
   usually modified by lua (server_startup()) instead of here. */
extern int updated_savegame_birth;
/* Like 'updated_savegame' is for players, this is for (lua) server state [0]
   usually modified by lua (server_startup()) instead of here. */
extern int updated_server;
/* for automatic artifact resets via lua */
extern int artifact_reset;

/* variables for controlling global events (automated Highlander Tournament) - C. Blue */
extern global_event_type global_event[MAX_GLOBAL_EVENTS];
extern int sector00separation, sector00downstairs, ge_special_sector; /* see variable.c */
extern u32b ge_contender_buffer_ID[128];
extern int ge_contender_buffer_deed[128];
extern u32b achievement_buffer_ID[128];
extern int achievement_buffer_deed[128];

/* for temporary disabling all validity checks when a dungeon master/wizard summons something - C. Blue */
extern u32b summon_override_checks;
/* Morgoth may override the no-destroy flag (other monsters too, if needed) */
extern bool override_LF1_NO_DESTROY;

/* the four seasons */
extern void lua_season_change(int s, int force);
extern byte season;
/* for snowfall during WINTER_SEASON mainly */
extern int weather;
extern int weather_duration;
extern byte weather_frequency;
extern int wind_gust;
extern int wind_gust_delay;
/* moving clouds */
extern int cloud_x1[MAX_CLOUDS], cloud_y1[MAX_CLOUDS], cloud_x2[MAX_CLOUDS], cloud_y2[MAX_CLOUDS], cloud_dsum[MAX_CLOUDS];
extern int cloud_xm100[MAX_CLOUDS], cloud_ym100[MAX_CLOUDS], cloud_mdur[MAX_CLOUDS], cloud_xfrac[MAX_CLOUDS], cloud_yfrac[MAX_CLOUDS];
extern int cloud_dur[MAX_CLOUDS], cloud_state[MAX_CLOUDS];
extern int clouds;
extern int max_clouds_seasonal;
/* winds, moving clouds and affecting rain/snow */
extern int wind_dur[16], wind_dir[16];

/* special seasons */
extern int season_halloween;
extern int season_xmas;
extern int season_newyearseve;

/* for controlling fireworks on NEW_YEARS_EVE */
extern int fireworks;
extern int fireworks_delay;

/* for /shutrec command */
extern int shutdown_recall_timer, shutdown_recall_state;

#ifndef ENABLE_RCRAFT
/* runes.c */
extern void cast_rune_spell(int, int);
extern void cast_rune_spell_header(int Ind, int a, int b);
#else
/* runecraft.c */
extern byte execute_rspell (u32b, byte, u32b, byte);
/* spells1.c */
extern bool rune_backlash(int Ind, int typ, int dam);
/* tables.c */
extern r_element r_elements[RCRAFT_MAX_ELEMENTS];
extern r_imper r_imperatives[RG_MAX];
extern r_type runespell_types[8];
extern r_spell runespell_list[RT_MAX];
extern rspell_sel rspell_selector[MAX_RSPELL_SEL];
#endif

#ifdef MONSTER_ASTAR
extern astar_list_open astar_info_open[ASTAR_MAX_INSTANCES];
extern astar_list_closed astar_info_closed[ASTAR_MAX_INSTANCES];
#endif
