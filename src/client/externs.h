/* File: externs.h */

/* Purpose: extern declarations (variables and functions) */

/*
 * Note that some files have their own header files
 * (z-virt.h, z-util.h, z-form.h, term.h, random.h)
 */

#ifdef USE_GCU
extern errr init_gcu(void);
extern void gcu_restore_colours(void);
extern void resize_main_window_gcu(int cols, int rows);
#endif

#ifdef USE_X11
extern errr init_x11(void);
 #if 0
    extern void turn_off_numlock_X11(void);
 #endif
 #if 1 /* CHANGE_FONTS_X11 */
    extern void change_font(int s);
    extern const char* get_font_name(int term_idx);
    extern void set_font_name(int term_idx, char* fnt);
    extern void term_toggle_visibility(int term_idx);
    extern bool term_get_visibility(int term_idx);
 #endif

extern void x11win_getinfo(int term_idx, int *x, int *y, int *c, int *r, char *fnt_name);
extern void resize_main_window_x11(int cols, int rows);
extern bool ask_for_bigmap(void);
extern void get_screen_font_name(char *buf);
extern void animate_palette(void);
extern void set_palette(byte c, byte r, byte g, byte b);
#endif

#ifdef USE_XAW
extern errr init_xaw(void);
extern errr CheckEvent(bool wait);
#endif

#ifdef USE_CAP
extern errr init_cap(void);
#endif

#ifdef USE_IBM
extern errr init_ibm(void);
#endif

#ifdef USE_EMX
extern errr init_emx(void);
#endif

#ifdef USE_AMY
extern errr init_amy(void);
#endif

#ifdef USE_GTK
extern errr init_gtk(int, char **);
#endif


/*
 * Not-so-Automatically generated "variable" declarations
 */

/* common/tables.c */
extern r_element r_elements[RCRAFT_MAX_ELEMENTS];
extern r_imperative r_imperatives[RCRAFT_MAX_IMPERATIVES];
extern r_type r_types[RCRAFT_MAX_TYPES];
extern r_projection r_projections[RCRAFT_MAX_PROJECTIONS];

/* c-spell.c */
/*extern void show_browse(int book); */
extern s32b get_school_spell(cptr do_what, int *item_book);
extern int get_spell(s32b *sn, cptr prompt, int book);
extern void show_browse(object_type *o_ptr);
extern void browse_school_spell(int item, int book, int pval);
extern void do_study(int book);
extern void do_cast(int book);
extern void do_pray(int book);
extern void do_fight(int book);
extern void do_ghost(void);
extern void do_mimic(void);
extern void do_stance(void);
extern void do_melee_technique(void);
extern void do_ranged_technique(void);
extern bool get_item_hook_find_spell(int *item, int mode);
extern byte flags_to_elements(byte element[], u16b e_flags);
extern byte flags_to_imperative(u16b m_flags);
extern byte flags_to_type(u16b m_flags);
extern byte flags_to_projection(u16b e_flags);
extern byte rspell_skill(byte element[], byte elements);
extern byte rspell_level(byte imperative, byte type);
extern s16b rspell_diff(byte skill, byte level);
extern byte rspell_cost(byte imperative, byte type, byte skill);
extern byte rspell_fail(byte imperative, byte type, s16b diff, u16b penalty);
extern u16b rspell_damage(u32b *dx, u32b *dy, byte imperative, byte type, byte skill, byte projection);
extern byte rspell_radius(byte imperative, byte type, byte skill, byte projection);
extern byte rspell_duration(byte imperative, byte type, byte skill, byte projection, u16b dice);
extern void do_runespell();
extern void do_breath(void);

/* tables.c */
extern byte adj_mag_stat[];
extern byte adj_mag_fail[];
extern s16b ddx[10];
extern s16b ddy[10];
extern char hexsym[16];
//extern owner_type owners[MAX_STORES][MAX_STORE_OWNERS];
extern option_type option_info[];
extern cptr stat_names[6];
extern cptr stat_names_reduced[6];
extern char ang_term_name[ANGBAND_TERM_MAX][40];
extern cptr window_flag_desc[8];
extern monster_spell_type monster_spells4[32];
extern monster_spell_type monster_spells5[32];
extern monster_spell_type monster_spells6[32];
extern cptr melee_techniques[16];
extern cptr ranged_techniques[16];
extern byte adj_int_pow[];
extern magic_type innate_powers[96];


/* variable.c */
extern bool c_quit;
extern char meta_address[MAX_CHARS];
extern char nick[MAX_CHARS];
extern char pass[MAX_CHARS];
extern char svname[MAX_CHARS];
extern char path[1024];
extern char real_name[MAX_CHARS];
extern char server_name[MAX_CHARS];
extern s32b server_port;
extern char cname[MAX_CHARS], prev_cname[MAX_CHARS];

extern char message_history[MSG_HISTORY_MAX][MSG_LEN];
extern char message_history_chat[MSG_HISTORY_MAX][MSG_LEN];
extern char message_history_msgnochat[MSG_HISTORY_MAX][MSG_LEN];
extern char message_history_impscroll[MSG_HISTORY_MAX][MSG_LEN];
/*extern byte hist_start; */
extern int hist_end;
extern bool hist_looped;
extern int hist_chat_end;
extern bool hist_chat_looped;

extern object_type inventory[INVEN_TOTAL];
extern char inventory_name[INVEN_TOTAL][ONAME_LEN];
extern int inventory_inscription[INVEN_TOTAL];
extern int inventory_inscription_len[INVEN_TOTAL];
extern int item_newest;

extern store_type store;
extern c_store_extra c_store;
extern int store_prices[STORE_INVEN_MAX];
extern char store_names[STORE_INVEN_MAX][ONAME_LEN];
extern s16b store_num;

extern char spell_info[MAX_REALM + 9][9][9][80];

extern byte party_info_mode;
extern char party_info_name[MAX_CHARS];
extern char party_info_members[MAX_CHARS];
extern char party_info_owner[MAX_CHARS];
extern char guild_info_name[MAX_CHARS];
extern char guild_info_members[MAX_CHARS];
extern char guild_info_owner[MAX_CHARS];
extern bool guild_master;
extern guild_type guild;
extern int guildhall_wx, guildhall_wy;
extern char guildhall_pos[14];

extern setup_t Setup;
extern client_setup_t Client_setup;

extern bool shopping, perusing;

extern s16b last_line_info;
extern s32b cur_line;
extern s32b max_line;
extern int cur_col;
#ifdef BIGMAP_MINDLINK_HACK
extern s16b last_line_y;
#endif

extern player_type Players[2];
extern player_type *p_ptr;

extern c_player_extra c_player;
extern c_player_extra *c_p_ptr;
/* extern char body_name[80]; */
extern s32b exp_adv, exp_adv_prev;
extern byte half_exp; //EXP_BAR_FINESCALE

extern char location_name2[MAX_CHARS], location_pre[10];

extern s16b command_see;
extern s16b command_gap;
extern s16b command_wrk;

extern bool item_tester_full;
extern byte item_tester_tval;
extern s16b item_tester_max_weight;
extern bool (*item_tester_hook)(object_type *o_ptr);
extern bool item_tester_hook_device(object_type *o_ptr);
extern bool item_tester_hook_armour(object_type *o_ptr);
extern bool item_tester_hook_weapon(object_type *o_ptr);
extern bool item_tester_hook_custom_tome(object_type *o_ptr);
extern bool item_tester_hook_rune(object_type *o_ptr);
extern bool item_tester_hook_armour_no_shield(object_type *o_ptr);
extern bool item_tester_hook_id(object_type *o_ptr);
extern bool item_tester_hook_starid(object_type *o_ptr);

extern int special_line_type;
extern int special_page_size;

extern bool inkey_base;
extern bool inkey_scan;
extern bool inkey_max_line; /* Make inkey() exit if we receive a 'max_line' value */
extern bool inkey_flag; /* We're currently reading keyboard input (via inkey()) */
extern bool inkey_interact_macros; /* In macro menu, no macros may act */
extern bool inkey_msg;/* A chat message is currently being entered */

extern bool inkey_letter_all;

extern s16b macro__num;
extern cptr *macro__pat;
extern cptr *macro__act;
extern bool *macro__cmd;
extern bool *macro__hyb;
extern char *macro__buf;

extern char recorded_macro[160];
extern bool recording_macro;

extern s32b message__next;
extern s32b message__last;
extern s32b message__head;
extern s32b message__tail;
extern s32b *message__ptr;
extern char *message__buf;
extern s32b message__next_chat;
extern s32b message__last_chat;
extern s32b message__head_chat;
extern s32b message__tail_chat;
extern s32b *message__ptr_chat;
extern char *message__buf_chat;
extern s32b message__next_msgnochat;
extern s32b message__last_msgnochat;
extern s32b message__head_msgnochat;
extern s32b message__tail_msgnochat;
extern s32b *message__ptr_msgnochat;
extern char *message__buf_msgnochat;
extern s32b message__next_impscroll;
extern s32b message__last_impscroll;
extern s32b message__head_impscroll;
extern s32b message__tail_impscroll;
extern s32b *message__ptr_impscroll;
extern char *message__buf_impscroll;




extern bool msg_flag; //no effect

extern term *ang_term[ANGBAND_TERM_MAX];
extern u32b window_flag[ANGBAND_TERM_MAX];

extern byte color_table[256][4];

extern cptr ANGBAND_SYS;

extern byte keymap_cmds[128];
extern byte keymap_dirs[128];

extern s16b command_cmd;
extern s16b command_dir;

extern boni_col csheet_boni[15];
extern byte csheet_page;
extern bool valid_dna, dedicated;
extern s16b race, dna_race;
extern s16b class, dna_class;
extern cptr dna_class_title; //ENABLE_DEATHKNIGHT, ENABLE_HELLKNIGHT, ENABLE_CPRIEST
extern s16b sex, dna_sex;
extern s16b mode;
extern s16b trait, dna_trait;

/* DEG Stuff for new party client */
extern s16b client;
extern s16b class_extra;

extern s16b stat_order[6], dna_stat_order[6];

extern bool topline_icky;
extern short screen_icky;
extern bool party_mode, guildcfg_mode;

extern player_race *race_info;
extern player_class *class_info;
extern player_trait *trait_info;

//the +16 are just for some future-proofing, to avoid needing to update the client
extern char race_diz[MAX_RACE + 16][12][61];
extern char class_diz[MAX_CLASS + 16][12][61];
extern char trait_diz[MAX_TRAIT + 16][12][61];

extern cptr ANGBAND_DIR;
extern cptr ANGBAND_DIR_TEXT;
extern cptr ANGBAND_DIR_USER;
extern cptr ANGBAND_DIR_XTRA;
extern cptr ANGBAND_DIR_SCPT;
extern cptr ANGBAND_DIR_GAME;

extern bool disable_numlock;
extern bool use_graphics;
extern bool use_sound, use_sound_org;
extern bool quiet_mode;
extern bool noweather_mode;
extern bool no_lua_updates;
extern bool skip_motd;
extern byte save_chat;


extern client_opts c_cfg;

extern u32b sflags3, sflags2, sflags1, sflags0;
extern byte client_mode;

extern u32b cfg_game_port;

extern skill_type s_info[MAX_SKILLS];

extern s16b flush_count;

extern char reason[MAX_CHARS];	/* Receive_quit */

/*
 * The spell list of schools
 */
extern s16b max_spells;
extern spell_type *school_spells;
extern s16b max_schools;
extern school_type *schools;

/* Server ping statistics */
extern int ping_id;
extern int ping_times[60], ping_avg, ping_avg_cnt;
extern bool lagometer_enabled;
extern bool lagometer_open;

/* Chat mode: normal, party or level */
extern char chat_mode;

/* Protocol to be used for connecting a server */
extern s32b server_protocol;

/* Server version */
extern version_type server_version;

/* Client fps used for polling keyboard input etc */
extern int cfg_client_fps;

/* Client-side tracking of unique kills */
extern byte r_unique[MAX_UNIQUES];
extern char r_unique_name[MAX_UNIQUES][60];

/* Global variables for account options and password changing */
extern bool acc_opt_screen;
extern bool acc_got_info;
extern s16b acc_flags;

/* Server detail flags */
extern bool s_RPG, s_FUN, s_ARCADE, s_TEST;
extern bool s_RPG_ADMIN, s_PARTY;
extern bool s_DED_IDDC, s_DED_PVP;
extern bool s_NO_PK;

/* Server's temporary features flags */
extern u32b sflags_TEMP;

/* Auto-inscriptions */
extern char auto_inscription_match[MAX_AUTO_INSCRIPTIONS][40];
extern char auto_inscription_tag[MAX_AUTO_INSCRIPTIONS][20];

/* Monster health memory (health_redraw) */
extern int mon_health_num;
extern byte mon_health_attr;

/* Location information memory (prt_depth) */
extern bool depth_town;
extern int depth_colour;
extern int depth_colour_sector;
extern char depth_name[MAX_CHARS];

/* Can macro triggers consist of multiple keys? */
extern bool multi_key_macros;

/* Redraw skills if the menu is open */
extern bool redraw_skills;
extern bool redraw_store;

/* Special input requests (PKT_REQUEST_...) - C. Blue */
extern bool request_pending;
extern bool request_abort;

extern char last_prompt[MSG_LEN];
extern bool last_prompt_macro;

/*
 * Not-so-Automatically generated "function declarations"
 */

/* c-birth.c */
extern void get_char_name(void);
extern void get_char_info(void);
extern bool get_server_name(void);
extern void create_random_name(int race, char *name);

/* c-cmd.c */
extern void process_command(void);
extern void do_cmd_skill(void);
extern void cmd_tunnel(void);
extern void cmd_walk(void);
extern void cmd_king(void);
extern void cmd_run(void);
extern void cmd_stay(void);
extern void cmd_stay_one(void);
extern void cmd_map(char mode);
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
extern void cmd_drop(byte flags);
extern void cmd_drop_gold(void);
extern void cmd_wield(void);
extern void cmd_wield2(void);
extern void cmd_observe(byte mode);
extern void cmd_take_off(void);
extern void cmd_swap(void);
extern void cmd_destroy(byte flags);
extern void cmd_inscribe(byte flags);
extern void cmd_uninscribe(byte flags);
extern void cmd_apply_autoins(void);
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
//extern void cmd_mind(void); done by cmd_telekinesis
extern void cmd_load_pref(void);
extern void cmd_redraw(void);
extern void cmd_purchase_house(void);
extern void cmd_suicide(void);
extern void cmd_spike(void);
extern void cmd_raw_key(int key);
extern void cmd_check_misc(void);
extern void cmd_sip(void);
extern void cmd_BBS(void);
extern void cmd_telekinesis(void);
extern void cmd_cloak(void);
extern void cmd_lagometer(void);
extern void cmd_force_stack(void);

/* c-files.c */
extern void text_to_ascii(char *buf, cptr str);
extern FILE *my_fopen(cptr file, cptr mode);
extern errr my_fclose(FILE *fff);
extern void init_file_paths(char *path);
extern errr process_pref_file(cptr buf);
extern errr process_pref_file_aux(char *buf);
extern void show_motd(int delay);
extern void peruse_file(void);
extern errr my_fgets(FILE *fff, char *buf, huge n);
extern errr my_fgets2(FILE *fff, char **line, int *n);
extern errr file_character(cptr name, bool full);
extern bool my_freadable(cptr file);
extern errr get_safe_file(char *buf, cptr file);
extern void xhtml_screenshot(cptr name);
extern void save_auto_inscriptions(cptr name);
extern void load_auto_inscriptions(cptr name);
extern void save_birth_file(cptr name, bool touch);
extern void load_birth_file(cptr name);

/* c-init.c */
extern void init_schools(s16b new_size);
extern void init_spells(s16b new_size);
extern void initialize_main_pref_files(void);
extern void initialize_player_pref_files(void);
extern void initialize_player_ins_files(void);
extern void client_init(char *argv1, bool skip);
extern s32b char_creation_flags;
extern void monster_lore_aux(int ridx, int rlidx, char paste_lines[18][MSG_LEN]);
extern void monster_stats_aux(int ridx, int rlidx, char paste_lines[18][MSG_LEN]);
extern void artifact_lore_aux(int aidx, int alidx, char paste_lines[18][MSG_LEN]);
extern void artifact_stats_aux(int aidx, int alidx, char paste_lines[18][MSG_LEN]);

/* c-inven.c */
extern s16b index_to_label(int i);
extern bool item_tester_okay(object_type *o_ptr);
extern cptr get_item_hook_find_obj_what;
extern bool get_item_hook_find_obj(int *item, int mode);
extern bool (*get_item_extra_hook)(int *cp, int mode);
extern bool c_get_item(int *cp, cptr pmt, int mode);
extern bool verified_item;
extern int hack_force_spell_level;

/* c-util.c */
extern void move_cursor(int row, int col);
extern void flush(void);
extern void flush_now(void);
#ifdef RETRY_LOGIN
extern void RL_revert_input(void);
extern void clear_macros(void);
#endif
extern void macro_add(cptr pat, cptr act, bool cmd_flag, bool hyb_flag);
extern bool macro_del(cptr pat);
extern char inkey(void);
extern void keymap_init(void);
extern void bell(void);
extern int page(void);
extern int warning_page(void);
extern void c_prt(byte attr, cptr str, int row, int col);
extern void prt(cptr str, int row, int col);
extern bool get_string(cptr prompt, char *buf, int len);
extern bool get_com(cptr prompt, char *command);
extern void request_command();
extern bool get_dir(int *dp);
extern void c_put_str(byte attr, cptr str, int row, int col);
extern void put_str(cptr str, int row, int col);
extern bool get_check2(cptr prompt, bool default_yes);
extern bool get_check3(cptr prompt, char default_choice);
extern byte get_3way(cptr prompt, bool default_no);
extern s32b message_num(void);
extern s32b message_num_chat(void);
extern s32b message_num_msgnochat(void);
extern s32b message_num_impscroll(void);
extern cptr message_str(s32b age);
extern cptr message_str_chat(s32b age);
extern cptr message_str_msgnochat(s32b age);
extern cptr message_str_impscroll(s32b age);
extern void c_message_add(cptr msg);
extern void c_message_add_scrollback(cptr msg);
extern void c_message_add_chat(cptr msg);
extern void c_message_add_msgnochat(cptr msg);
extern void c_message_add_impscroll(cptr msg);
extern void c_msg_print(cptr msg);
extern void c_msg_format(cptr fmt, ...)  __attribute__ ((format (printf, 1, 2)));
extern s32b c_get_quantity(cptr prompt, int max);
extern bool askfor_aux(char *buf, int len, char mode);
extern void clear_from(int row);
extern void prt_num(cptr header, int num, int row, int col, byte color);
extern void prt_lnum(cptr header, s32b num, int row, int col, byte color);
extern void interact_macros(void);
extern void auto_inscriptions(void);
extern void display_account_information(void);
extern void do_cmd_options(void);
extern void c_close_game(cptr reason);
extern void my_memfrob(void *s, int n);
extern bool is_newer_than(version_type *version, int major, int minor, int patch, int extra, int branch, int build);
#ifdef USE_SOUND_2010
extern void interact_audio(void);
extern void audio_pack_selector(void);
extern void toggle_master(bool gui);
extern void toggle_music(bool gui);
extern void toggle_sound(void);
extern void toggle_weather(void);
extern bool sound_bell(void);
extern bool sound_page(void);
extern bool sound_warning(void);
extern int bell_sound_idx, page_sound_idx, warning_sound_idx, rain1_sound_idx, rain2_sound_idx, snow1_sound_idx, snow2_sound_idx, browse_sound_idx, browsebook_sound_idx;
#endif
extern errr options_dump(cptr fname);
extern bool parse_macro;
extern bool inkey_sleep, inkey_sleep_semaphore;
extern bool abort_prompt;
extern int macro_missing_item;
extern void Send_paste_msg(char *msg);
extern void check_immediate_options(int i, bool yes, bool playing);
extern void prompt_topline(cptr prompt);
extern void clear_topline(void);
extern void clear_topline_forced(void);
extern void restore_prompt(void); /* DONT_CLEAR_TOPLINE_IF_AVOIDABLE */
extern u32b parse_color_code(const char *str);
extern void handle_process_font_file(void);

extern void sync_sleep(int milliseconds);

/* c-store.c */
extern bool leave_store;
extern void display_inventory(void);
extern void display_store(void);
extern void display_store_special(void);
extern void c_store_prt_gold(void);
extern void display_store_action(void);
extern void do_redraw_store(void);
extern void store_paste_where(char *where);
extern void store_paste_item(char *out_val, int item);
extern int store_top;

/* c-xtra1.c */
extern void prt_stat(int stat, int max, int cur, int cur_base);
extern void prt_title(cptr title);
extern void prt_level(int level, int max_lev, int max_plv, s32b max, s32b cur, s32b adv, s32b adv_prev);
extern void prt_gold(int gold);
extern void prt_ac(int ac);
extern void prt_hp(int max, int cur, bool bar);
extern void prt_party_stats(int member_num, byte color, char *member_name, int member_lev, int member_chp, int member_mhp, int member_csp, int member_msp);
extern void prt_sp(int max, int cur, bool bar);
extern void prt_depth(int x, int y, int z, bool town, int colour, int colour_sector, cptr buf);
extern void prt_hunger(int food);
extern void prt_blind(bool blind);
extern void prt_confused(bool confused);
extern void prt_afraid(bool fear);
extern void prt_poisoned(bool poisoned);
extern void prt_state(bool paralyzed, bool searching, bool resting);
extern void prt_speed(int speed);
extern void prt_study(bool study);
extern void prt_bpr(byte bpr, byte attr);
extern void prt_cut(int cut);
extern void prt_stun(int stun);
extern void prt_basic(void);
extern void health_redraw(int num, byte attr);
extern void show_inven(void);
extern void show_equip(void);
extern void display_player(int hist);
extern void window_stuff(void);
extern void prt_sane(byte attr, cptr buf);
extern void cnv_stat(int val, char *out_val);
extern void prt_AFK(byte afk);
extern void prt_encumberment(byte cumber_armor, byte awkward_armor, byte cumber_glove, byte heavy_wield, byte heavy_shield, byte heavy_shoot,
    byte icky_wield, byte awkward_wield, byte easy_wield, byte cumber_weight, byte monk_heavyarmor, byte rogue_heavyarmor, byte awkward_shoot);
extern void prt_extra_status(cptr status);
extern void prt_stamina(int max, int cur, bool bar);
extern void display_lagometer(bool display_commands);
extern void update_lagometer(void);
extern void prt_lagometer(int lag);

extern int p_speed;
extern bool no_tele_grid;
extern void do_weather(void);
extern int weather_type, weather_wind, weather_gen_speed, weather_intensity, weather_speed_rain, weather_speed_snow;
extern int weather_elements, weather_element_x[1024], weather_element_y[1024], weather_element_ydest[1024], weather_element_type[1024];
extern int weather_panel_x, weather_panel_y;
extern bool weather_panel_changed;
extern byte panel_map_a[MAX_SCREEN_WID][MAX_SCREEN_HGT];
extern char panel_map_c[MAX_SCREEN_WID][MAX_SCREEN_HGT];
extern int cloud_x1[10], cloud_y1[10], cloud_x2[10], cloud_y2[10], cloud_dsum[10];
extern int cloud_xm100[10], cloud_ym100[10], cloud_xfrac[10], cloud_yfrac[10];

/* c-xtra2.c */
extern void do_cmd_messages(void);
extern void do_cmd_messages_chatonly(void);
extern errr dump_messages(cptr name, int lines, int mode);
extern void dump_messages_aux(FILE *fff, int lines, int mode, bool ignore_color);

/* client.c */
extern bool write_mangrc(bool creds_only, bool audiopacks_only);
typedef struct {
	bool visible;
	int x, y;
	int columns, lines;
	char font[256]; /* Paranoia: actually, 6 should be sufficient */
} generic_term_info;
extern generic_term_info term_prefs[10];

/* nclient.c (former netclient.c) */
extern int ticks, ticks10, existing_characters, command_confirmed;
extern void do_flicker(void);
extern void do_mail(void);
extern void update_ticks(void);
extern void do_keepalive(void);
extern void do_ping(void);
extern int Net_setup(void);
extern unsigned char Net_login(void);
extern int Net_verify(char *real, char *nick, char *pass);
extern int Net_init(int port);
extern void Net_cleanup(void);
extern int Net_flush(void);
extern int Net_fd(void);
extern int Net_start(int sex, int race, int class);
extern int Net_input(void);
extern int Flush_queue(void);
extern int next_frame(void);

extern int Send_file_check(int ind, unsigned short id, char *fname);
extern int Send_file_init(int ind, unsigned short id, char *fname);
extern int Send_file_data(int ind, unsigned short id, char *buf, unsigned short len);
extern int Send_file_end(int ind, unsigned short id);
extern int Receive_file_data(int ind, unsigned short len, char *buffer);
extern int Send_raw_key(int key);
//extern int Send_mind(void); instead using Send_telekinesis
extern int Send_mimic(int spell);
extern int Send_search(void);
extern int Send_walk(int dir);
extern int Send_run(int dir);
extern int Send_drop(int item, int amt);
extern int Send_drop_gold(s32b amt);
extern int Send_tunnel(int dir);
extern int Send_stay(void);
extern int Send_stay_one(void);
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
extern int Send_take_off_amt(int item, int amt);
extern int Send_destroy(int item, int amt);
extern int Send_inscribe(int item, cptr buf);
extern int Send_uninscribe(int item);
extern int Send_autoinscribe(int item);
extern int Send_steal(int dir);
extern int Send_quaff(int item);
extern int Send_read(int item);
extern int Send_aim(int item, int dir);
extern int Send_use(int item);
extern int Send_zap(int item);
extern int Send_zap_dir(int item, int dir);
extern int Send_fill(int item);
extern int Send_eat(int item);
extern int Send_activate(int item);
extern int Send_activate_dir(int item, int dir);
extern int Send_target(int dir);
extern int Send_target_friendly(int dir);
extern int Send_look(int dir);
extern int Send_msg(cptr message);
extern int Send_fire(int dir);
extern int Send_throw(int item, int dir);
extern int Send_item(int item);
extern int Send_spell(int item, int spell);
extern int Send_gain(int book, int spell);
extern int Send_cast(int book, int spell);
extern int Send_pray(int book, int spell);
extern int Send_fight(int book, int spell);
extern int Send_ghost(int ability);
extern int Send_map(char mode);
extern int Send_locate(int dir);
extern int Send_store_purchase(int item, int amt);
extern int Send_store_sell(int item, int amt);
extern int Send_store_leave(void);
extern int Send_store_confirm(void);
extern int Send_redraw(char mode);
extern int Send_special_line(int type, s32b line);
extern int Send_party(s16b command, cptr buf);
extern int Send_guild(s16b command, cptr buf);
extern int Send_guild_config(s16b command, u32b flags, cptr buf);
extern int Send_purchase_house(int dir);
extern int Send_suicide(void);
extern int Send_options(void);
extern int Send_screen_dimensions(void);
extern int Send_master(s16b command, cptr buf);
extern int Send_clear_buffer(void);
extern int Send_clear_actions(void);
extern int Send_King(byte type);
extern int Send_force_stack(int item);
extern int Send_admin_house(int dir, cptr buf);
extern int Send_spike(int dir);
extern int Send_skill_mod(int i);
extern int Send_skill_dev(int i, bool dev);
extern int Send_store_examine(int item);
extern int Send_store_command(int action, int item, int item2, int amt, int gold);
extern int Send_activate_skill(int mkey, int book, int spell, int dir, int item, int aux);
extern int Send_ping(void);
extern int Send_sip(void);
extern int Send_telekinesis(void);
extern int Send_BBS(void);
extern int Send_wield2(int item);
extern int Send_cloak(void);
extern int Send_inventory_revision(int revision);
extern int Send_account_info(void);
extern int Send_change_password(char *old_pass, char *new_pass);
extern int Send_request_key(int id, char key);
extern int Send_request_num(int id, int num);
extern int Send_request_str(int id, char *str);
extern int Send_request_cfr(int id, int cfr);
extern void apply_auto_inscriptions(int slot, bool force);
extern int Send_client_setup(void);

/* skills.c */
extern s16b get_skill(int skill);
extern void do_activate_skill(int x_idx, int item);
extern void do_cmd_activate_skill(void);
extern void dump_skills(FILE *fff);
extern s16b get_skill_scale(player_type *pfft, int skill, u32b scale);
extern void do_redraw_skills(void);
extern void do_trap(int item_kit);

/* c-script.c */
extern void init_lua(void);
extern void reinit_lua(void);
extern void open_lua(void);
extern void reopen_lua(void);
extern bool pern_dofile(int Ind, char *file);
extern int exec_lua(int Ind, char *file);
extern cptr string_exec_lua(int Ind, char *file);
extern void master_script_begin(char *name, char mode);
extern void master_script_end(void);
extern void master_script_line(char *buf);
extern void master_script_exec(int Ind, char *name);
extern void cat_script(char *name);
extern bool call_lua(int Ind, cptr function, cptr args, cptr ret, ...);

/* lua_bind.c */
extern void lua_set_item_tester(int tval, char *fct);
extern s16b new_school(int i, cptr name, s16b skill);
extern s16b new_spell(int i, cptr name);
extern spell_type *grab_spell_type(s16b num);
extern school_type *grab_school_type(s16b num);
extern s32b lua_get_level(int Ind, s32b s, s32b lvl, s32b max, s32b min, s32b bonus);
extern s32b lua_spell_chance(int i, s32b chance, int level, int skill_level, int mana, int cur_mana, int stat);
extern int get_inven_sval(int Ind, int inven_slot);
extern s16b get_inven_xtra(int Ind, int inven_slot, int n);
extern bool get_item_aux(int *cp, cptr pmt, bool equip, bool inven, bool floor);
extern int get_inven_pval(int Ind, int inven_slot);

/* common/common.c */
extern errr path_build(char *buf, int max, cptr path, cptr file);
extern cptr longVersion;
extern cptr shortVersion;
extern void version_build(void);
extern int find_realm(int book);
extern char color_attr_to_char(int a);
extern int color_char_to_attr(char c);
extern byte mh_attr(int max);
extern const char *my_strcasestr(const char *big, const char *little);
extern const char *my_strcasestr_skipcol(const char *big, const char *little, bool strict);

/* common/files.c */
extern int local_file_init(int ind, unsigned short fnum, char *fname);
extern int local_file_write(int ind, unsigned short fnum, unsigned long len);
extern int local_file_close(int ind, unsigned short fnum);
extern int local_file_check(char *fname, u32b *sum);
extern int local_file_check_new(char *fname, unsigned char digest_out[16]);
extern int local_file_ack(int ind, unsigned short fnum);
extern int local_file_err(int ind, unsigned short fnum);
extern void do_xfers(void);
extern int get_xfers_num(void);
extern int check_return(int ind, unsigned short fnum, unsigned long sum, int Ind);
extern int check_return_new(int ind, unsigned short fnum, const unsigned char digest[16], int Ind);
extern int remote_update(int ind, char *fname, unsigned short chunksize);
extern void md5_digest_to_bigendian_uint(unsigned digest_out[4], const unsigned char digest[16]);
extern void md5_digest_to_char_array(unsigned char digest_out[16], const unsigned digest[4]);

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
extern void change_font(int s);
extern const char* get_font_name(int term_idx);
extern void set_font_name(int term_idx, char* fnt);
extern void term_toggle_visibility(int term_idx);
extern bool term_get_visibility(int term_idx);
extern void resize_main_window_win(int cols, int rows);
extern bool ask_for_bigmap(void);
extern bool check_dir(cptr s);
extern void get_screen_font_name(char *buf);
extern bool win_dontmoveuser;
extern void animate_palette(void);
extern void set_palette(byte c, byte r, byte g, byte b);
extern void store_audiopackfolders(void);
#endif
extern void store_crecedentials(void);

extern const cptr angband_sound_name[SOUND_MAX];
extern int audio_sfx, audio_music;

#ifdef USE_SOUND_2010
//externs.h:
extern void (*mixing_hook)(void);
extern bool (*sound_hook)(int, int, int, s32b);
extern void (*sound_ambient_hook)(int);
extern void (*sound_weather_hook)(int);
extern void (*sound_weather_hook_vol)(int, int);
extern bool (*music_hook)(int);
extern bool sound(int val, int type, int vol, s32b player_id);
extern bool music(int val);
extern void sound_ambient(int val);
extern void sound_weather(int val);
extern void sound_weather_vol(int val, int vol);
extern void set_mixing(void);
extern void weather_handle_fading(void);
extern void ambient_handle_fading(void);
extern void mixer_fadeall(void);
extern int music_cur, music_cur_song, music_next, music_next_song, weather_channel, weather_current, weather_current_vol, weather_channel_volume, ambient_channel, ambient_current, ambient_channel_volume;
extern int weather_particles_seen, weather_sound_change, weather_fading, ambient_fading;
extern bool wind_noticable;
extern int cfg_audio_rate, cfg_max_channels, cfg_audio_buffer;
extern bool cfg_audio_master, cfg_audio_music, cfg_audio_sound, cfg_audio_weather, no_cache_audio, weather_resume, ambient_resume;
extern int cfg_audio_master_volume, cfg_audio_music_volume, cfg_audio_sound_volume, cfg_audio_weather_volume;
#if 1 /* WEATHER_VOL_PARTICLES */
extern int weather_vol_smooth, weather_vol_smooth_anti_oscill, weather_smooth_avg[20];
#endif
extern int grid_weather_volume, grid_ambient_volume, grid_weather_volume_goal, grid_ambient_volume_goal, grid_weather_volume_step, grid_ambient_volume_step;
extern bool sound_hint;

extern const struct module sound_modules[];
extern int re_init_sound();

 #ifdef SOUND_SDL
 extern errr init_sound_sdl(int argc, char **argv);
 extern errr re_init_sound_sdl(void);
 extern void do_cmd_options_sfx_sdl(void);
 extern void do_cmd_options_mus_sdl(void);
 #endif

//z-files.h:
//extern bool my_fexists(const char *fname);
#endif

extern char monster_list_name[MAX_R_IDX][80], monster_list_symbol[MAX_R_IDX][2];
extern int monster_list_code[MAX_R_IDX], monster_list_idx, monster_list_level[MAX_R_IDX];
extern bool monster_list_any[MAX_R_IDX], monster_list_breath[MAX_R_IDX];

extern char artifact_list_name[MAX_A_IDX][80];
extern int artifact_list_code[MAX_A_IDX], artifact_list_rarity[MAX_A_IDX], artifact_list_idx;
extern bool artifact_list_specialgene[MAX_A_IDX];

extern char kind_list_name[MAX_K_IDX][80];
extern int kind_list_tval[MAX_K_IDX], kind_list_sval[MAX_K_IDX], kind_list_rarity[MAX_K_IDX], kind_list_idx;
extern char kind_list_char[MAX_K_IDX], kind_list_attr[MAX_K_IDX];

extern char monster_mapping_org[MAX_R_IDX + 1];
extern char monster_mapping_mod[256];
extern char floor_mapping_org[MAX_F_IDX + 1];
extern char floor_mapping_mod[256];

extern int screen_wid, screen_hgt;
extern void (*resize_main_window)(int cols, int rows);
extern bool bigmap_hint, global_big_map_hold, firstrun;
extern bool ask_for_bigmap_generic(void);
extern bool in_game;
extern bool rand_term_lamp;
extern int rand_term_lamp_ticks;

extern int minimap_posx, minimap_posy, minimap_selx, minimap_sely;
extern byte minimap_attr, minimap_selattr;
extern char minimap_char, minimap_selchar;

extern bool silent_dump;
extern bool equip_no_weapon;
extern bool auto_reincarnation;
extern char macro_trigger_exclusive[MAX_CHARS];
extern bool macro_processing_exclusive;

extern int max_chars_per_account;

#ifndef EXTENDED_COLOURS_PALANIM
extern u32b client_color_map[16];
#else
extern u32b client_color_map[16 * 2];
#endif

#ifdef RETRY_LOGIN
extern bool rl_connection_destructible, rl_connection_destroyed, rl_password;
extern byte rl_connection_state;
extern bool player_pref_files_loaded;
#endif

extern int guide_lastline;
#ifdef BUFFER_GUIDE
extern char guide_line[GUIDE_LINES_MAX][MAX_CHARS + 1];
#endif
extern char guide_race[64][MAX_CHARS];
extern char guide_class[64][MAX_CHARS];
extern char guide_skill[128][MAX_CHARS];
extern char guide_school[64][MAX_CHARS];
extern char guide_spell[256][MAX_CHARS];
extern int guide_races, guide_classes, guide_skills, guide_schools, guide_spells;
extern char guide_chapter[256][MAX_CHARS], guide_chapter_no[256][8];
extern int guide_chapters, guide_endofcontents;

extern bool showing_inven, showing_equip;

extern int hp_max, hp_cur;
extern bool hp_bar;
extern int sp_max, sp_cur;
extern bool sp_bar;
extern int st_max, st_cur;
extern bool st_bar;

extern char cfg_soundpackfolder[1024];
extern char cfg_musicpackfolder[1024];
