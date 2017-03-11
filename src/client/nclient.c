/* $Id$ */
/* The client side of the networking stuff */

/* The file is very messy, and probably has a lot of stuff
 * that isn't even used.  Some massive work needs to be done
 * on it, but it's a low priority for me.
 */

/* I've made this file even worse as I have converted things to TCP.
 * rbuf, cbuf, wbuf, and qbuf are used very inconsistently.
 * rbuf is basically temporary storage that we read data into, and also
 * where we put the data before we call the command processing functions.
 * cbuf is the "command buffer".  qbuf is the "old command queue". wbuf
 * isn't used much.  The use of these buffers should probably be
 * cleaned up.  -Alex
 */

/* cbuf is now only used inside Net_setup and qbuf is only used for a
 * couple of packets that can't be handled when topline is icky.
 *  - mikaelh
 */

#define CLIENT
#include "angband.h"
#include "netclient.h"
#ifdef AMIGA
#include <devices/timer.h>
#endif
#ifndef DUMB_WIN
#ifndef WINDOWS
#include <unistd.h>
#endif
#endif

#include <sys/stat.h>

#include <sys/time.h>


/* for weather - keep in sync with c-xtra1.c, do_weather()! */
#define PANEL_X	(SCREEN_PAD_LEFT)
#define PANEL_Y	(SCREEN_PAD_TOP)


extern void flicker(void);

int			ticks = 0; /* Keeps track of time in 100ms "ticks" */
int			ticks10 = 0; /* 'deci-ticks', counting just 0..9 in 10ms intervals */
int			existing_characters = 0;

static sockbuf_t	rbuf, wbuf, qbuf;
static int		(*receive_tbl[256])(void);
static int		last_send_anything;
static char		initialized = 0;

int			command_confirmed = -1;

/*
 * Initialize the function dispatch tables.
 */
static void Receive_init(void) {
	int i;

	for (i = 0; i < 256; i++)
		receive_tbl[i] = NULL;

	receive_tbl[PKT_RELIABLE]	= NULL; /* Server shouldn't be sending this */
	receive_tbl[PKT_QUIT]		= Receive_quit;
	receive_tbl[PKT_START]		= NULL; /* Server shouldn't be sending this */
	receive_tbl[PKT_END]		= Receive_end;
	receive_tbl[PKT_LOGIN]		= NULL; /* Should not be called like this */
	receive_tbl[PKT_FILE]		= Receive_file;

	receive_tbl[PKT_QUIT]		= Receive_quit;
	receive_tbl[PKT_STAT]		= Receive_stat;
	receive_tbl[PKT_HP]		= Receive_hp;
	receive_tbl[PKT_AC]		= Receive_ac;
	receive_tbl[PKT_INVEN]		= Receive_inven;
	receive_tbl[PKT_EQUIP]		= Receive_equip;
	receive_tbl[PKT_CHAR_INFO]	= Receive_char_info;
	receive_tbl[PKT_VARIOUS]	= Receive_various;
	receive_tbl[PKT_PLUSSES]	= Receive_plusses;
	receive_tbl[PKT_EXPERIENCE]	= Receive_experience;
	receive_tbl[PKT_GOLD]		= Receive_gold;
	receive_tbl[PKT_SP]		= Receive_sp;
	receive_tbl[PKT_HISTORY]	= Receive_history;
	receive_tbl[PKT_CHAR]		= Receive_char;
	receive_tbl[PKT_MESSAGE]	= Receive_message;
	receive_tbl[PKT_STATE]		= Receive_state;
	receive_tbl[PKT_TITLE]		= Receive_title;
	receive_tbl[PKT_DEPTH]		= Receive_depth;
	receive_tbl[PKT_CONFUSED]	= Receive_confused;
	receive_tbl[PKT_POISON]		= Receive_poison;
	receive_tbl[PKT_STUDY]		= Receive_study;
	receive_tbl[PKT_BPR]		= Receive_bpr;
	receive_tbl[PKT_FOOD]		= Receive_food;
	receive_tbl[PKT_FEAR]		= Receive_fear;
	receive_tbl[PKT_SPEED]		= Receive_speed;
	receive_tbl[PKT_CUT]		= Receive_cut;
	receive_tbl[PKT_BLIND]		= Receive_blind;
	receive_tbl[PKT_STUN]		= Receive_stun;
	receive_tbl[PKT_ITEM]		= Receive_item;
	receive_tbl[PKT_SPELL]		= Receive_spell_request;
	receive_tbl[PKT_SPELL_INFO]	= Receive_spell_info;
	receive_tbl[PKT_DIRECTION]	= Receive_direction;
	receive_tbl[PKT_FLUSH]		= Receive_flush;
	receive_tbl[PKT_LINE_INFO]	= Receive_line_info;
	receive_tbl[PKT_SPECIAL_OTHER]	= Receive_special_other;
	receive_tbl[PKT_STORE]		= Receive_store;
	receive_tbl[PKT_STORE_INFO]	= Receive_store_info;
	receive_tbl[PKT_SELL]		= Receive_sell;
	receive_tbl[PKT_TARGET_INFO]	= Receive_target_info;
	receive_tbl[PKT_SOUND]		= Receive_sound;
	receive_tbl[PKT_MINI_MAP]	= Receive_line_info;
	receive_tbl[PKT_SPECIAL_LINE]	= Receive_special_line;
	receive_tbl[PKT_FLOOR]		= Receive_floor;
	receive_tbl[PKT_PICKUP_CHECK]	= Receive_pickup_check;
	receive_tbl[PKT_PARTY]		= Receive_party;
	receive_tbl[PKT_GUILD]		= Receive_guild;
	receive_tbl[PKT_GUILD_CFG]	= Receive_guild_config;
	receive_tbl[PKT_PARTY_STATS]	= Receive_party_stats;
	receive_tbl[PKT_SKILLS]		= Receive_skills;
	receive_tbl[PKT_PAUSE]		= Receive_pause;
	receive_tbl[PKT_MONSTER_HEALTH]	= Receive_monster_health;
	receive_tbl[PKT_SANITY]		= Receive_sanity;
	receive_tbl[PKT_SKILL_INIT]	= Receive_skill_init;
	receive_tbl[PKT_SKILL_MOD] 	= Receive_skill_info;
	receive_tbl[PKT_SKILL_PTS]	= Receive_skill_points;
	receive_tbl[PKT_STORE_LEAVE] 	= Receive_store_kick;
	receive_tbl[PKT_CHARDUMP] 	= Receive_chardump;
	receive_tbl[PKT_BACT]		= Receive_store_action;

	receive_tbl[PKT_BEEP]		= Receive_beep;
	receive_tbl[PKT_WARNING_BEEP]	= Receive_warning_beep;
	receive_tbl[PKT_AFK]		= Receive_AFK;
	receive_tbl[PKT_ENCUMBERMENT]	= Receive_encumberment;
	receive_tbl[PKT_KEEPALIVE]	= Receive_keepalive;
	receive_tbl[PKT_PING]		= Receive_ping;
	receive_tbl[PKT_STAMINA]	= Receive_stamina;
	receive_tbl[PKT_TECHNIQUE_INFO]	= Receive_technique_info;
	receive_tbl[PKT_EXTRA_STATUS]	= Receive_extra_status;
	receive_tbl[PKT_INVEN_WIDE]	= Receive_inven_wide;
	receive_tbl[PKT_UNIQUE_MONSTER]	= Receive_unique_monster;
	receive_tbl[PKT_WEATHER]	= Receive_weather;
	receive_tbl[PKT_INVENTORY_REV]	= Receive_inventory_revision;
	receive_tbl[PKT_ACCOUNT_INFO]	= Receive_account_info;
	receive_tbl[PKT_STORE_WIDE]	= Receive_store_wide;
	receive_tbl[PKT_MUSIC]		= Receive_music;
	receive_tbl[PKT_BONI_COL]	= Receive_boni_col;
	receive_tbl[PKT_AUTOINSCRIBE]	= Receive_apply_auto_insc;

	receive_tbl[PKT_ITEM_NEWEST]	= Receive_item_newest;
	receive_tbl[PKT_CONFIRM]	= Receive_confirm;
	receive_tbl[PKT_KEYPRESS]	= Receive_keypress;

	receive_tbl[PKT_SFX_VOLUME]	= Receive_sfx_volume;
	receive_tbl[PKT_SFX_AMBIENT]	= Receive_sfx_ambient;
	receive_tbl[PKT_MINI_MAP_POS]	= Receive_mini_map_pos;

	receive_tbl[PKT_REQUEST_KEY]	= Receive_request_key;
	receive_tbl[PKT_REQUEST_NUM]	= Receive_request_num;
	receive_tbl[PKT_REQUEST_STR]	= Receive_request_str;
	receive_tbl[PKT_REQUEST_CFR]	= Receive_request_cfr;
	receive_tbl[PKT_REQUEST_ABORT]	= Receive_request_abort;
	receive_tbl[PKT_STORE_SPECIAL_STR]	= Receive_store_special_str;
	receive_tbl[PKT_STORE_SPECIAL_CHAR]	= Receive_store_special_char;
	receive_tbl[PKT_STORE_SPECIAL_CLR]	= Receive_store_special_clr;

	receive_tbl[PKT_MARTYR]			= Receive_martyr;
}


/* Head of file transfer system receive */
/* DO NOT TOUCH - work in progress */
int Receive_file(void){
	char command, ch;
	char fname[MAX_CHARS];	/* possible filename */
	int x;	/* return value/ack */
	char outbuf[80];
	unsigned short fnum;	/* unique SENDER side file number */
	unsigned short len;
	u32b csum; /* old 32-bit checksum */
	unsigned char digest[16]; /* new 128-bit MD5 checksum */
	int n, bytes_read;
	static bool updated_audio = FALSE;

	/* NOTE: The amount of data read is stored in n so that the socket
	 * buffer can be rolled back if the packet isn't complete. - mikaelh */
	if ((n = Packet_scanf(&rbuf, "%c%c%hd", &ch, &command, &fnum)) <= 0)
		return n;

	if (n == 3) {
		bytes_read = 4;

		switch(command){
			case PKT_FILE_INIT:
				if ((n = Packet_scanf(&rbuf, "%s", fname)) <= 0) {
					/* Rollback the socket buffer */
					Sockbuf_rollback(&rbuf, bytes_read);

					return n;
				}
				if (no_lua_updates) {
					sprintf(outbuf, "\377yIgnoring update for file %s [%d]", fname, fnum);
					c_msg_print(outbuf);
					x = FALSE;
					break;
				}
				x = local_file_init(0, fnum, fname);
				if (x) {
					sprintf(outbuf, "\377oReceiving updated file %s [%d]", fname, fnum);
					c_msg_print(outbuf);

					if (strstr(fname, "audio.lua")) updated_audio = TRUE;
				} else {
					if (errno == EACCES) c_msg_print("\377rNo access to update files");
				}
				break;
			case PKT_FILE_DATA:
				if ((n = Packet_scanf(&rbuf, "%hd", &len)) <= 0) {
					/* Rollback the socket buffer */
					Sockbuf_rollback(&rbuf, bytes_read);

					return n;
				}
				bytes_read += 2;
				x = local_file_write(0, fnum, len);
				if (x == -1) {
					/* Not enough data available */

					/* Rollback the socket buffer */
					Sockbuf_rollback(&rbuf, bytes_read);

					return 0;
				} else if (x == -2) {
					/* Write failed */
					sprintf(outbuf, "\377rWrite failed [%d]", fnum);
					c_msg_print(outbuf);

					return 0;
				}
				break;
			case PKT_FILE_END:
				x = local_file_close(0, fnum);
				if (x) {
					sprintf(outbuf, "\377gReceived file %d", fnum);
					c_msg_print(outbuf);
				}
				else {
					if (errno == EACCES) {
						c_msg_print("\377rNo access to lib directory!");
					} else {
						c_msg_print("\377rFile could not be written!");
					}
				}

				/* HACK - If this was the last file, reinitialize Lua on the fly - mikaelh */
				if (!get_xfers_num()) {
					c_msg_print("\377GReinitializing Lua");
					reopen_lua();

					if (updated_audio) {
						c_msg_print("\377R* Audio information was updated - restarting the game is recommended! *");
						c_msg_print("\377R   Without a restart, you might be hearing the wrong sound effects or music.");
					}
				}

				break;
			case PKT_FILE_CHECK:
				if ((n = Packet_scanf(&rbuf, "%s", fname)) <= 0) {
					/* Rollback the socket buffer */
					Sockbuf_rollback(&rbuf, bytes_read);

					return n;
				}
				if (is_newer_than(&server_version, 4, 6, 1, 1, 0, 1)) {
					unsigned digest_net[4];
					x = local_file_check_new(fname, digest);
					md5_digest_to_bigendian_uint(digest_net, digest);
					Packet_printf(&wbuf, "%c%c%hd%u%u%u%u", PKT_FILE, PKT_FILE_SUM, fnum, digest_net[0], digest_net[1], digest_net[2], digest_net[3]);
				} else {
					x = local_file_check(fname, &csum);
					Packet_printf(&wbuf, "%c%c%hd%d", PKT_FILE, PKT_FILE_SUM, fnum, csum);
				}
				return 1;
				break;
			case PKT_FILE_SUM:
				if (is_newer_than(&server_version, 4, 6, 1, 1, 0, 1)) {
					unsigned digest_net[4];
					if ((n = Packet_scanf(&rbuf, "%u%u%u%u", &digest_net[0], &digest_net[1], &digest_net[2], &digest_net[3])) <= 0) {
						/* Rollback the socket buffer */
						Sockbuf_rollback(&rbuf, bytes_read);

						return n;
					}
					md5_digest_to_char_array(digest, digest_net);
					check_return_new(0, fnum, digest, 0);
				} else {
					if ((n = Packet_scanf(&rbuf, "%d", &csum)) <= 0) {
						/* Rollback the socket buffer */
						Sockbuf_rollback(&rbuf, bytes_read);

						return n;
					}
					check_return(0, fnum, csum, 0);
				}
				return 1;
				break;
			case PKT_FILE_ACK:
				local_file_ack(0, fnum);
				return 1;
				break;
			case PKT_FILE_ERR:
				local_file_err(0, fnum);
				/* continue the send/terminate */
				return 1;
				break;
			case 0:
				return 1;
			default:
				x = 0;
		}
		Packet_printf(&wbuf, "%c%c%hd", PKT_FILE, x ? PKT_FILE_ACK : PKT_FILE_ERR, fnum);
	}
	return 1;
}

int Receive_file_data(int ind, unsigned short len, char *buffer) {
	(void) ind; /* suppress compiler warning */

	/* Check that the sockbuf has enough data */
	if (&rbuf.buf[rbuf.len] >= &rbuf.ptr[len]) {
		memcpy(buffer, rbuf.ptr, len);
		rbuf.ptr += len;
		return 1;
	} else {
		/* Wait for more data */
		return 0;
	}
}

int Send_file_check(int ind, unsigned short id, char *fname) {
	(void) ind; /* suppress compiler warning */

	Packet_printf(&wbuf, "%c%c%hd%s", PKT_FILE, PKT_FILE_CHECK, id, fname);
	return 0;
}

/* index arguments are just for common / laziness */
int Send_file_init(int ind, unsigned short id, char *fname) {
	(void) ind; /* suppress compiler warning */

	Packet_printf(&wbuf, "%c%c%hd%s", PKT_FILE, PKT_FILE_INIT, id, fname);
	return 0;
}

int Send_file_data(int ind, unsigned short id, char *buf, unsigned short len) {
	(void) ind; /* suppress compiler warning */

	Sockbuf_flush(&wbuf);
	Packet_printf(&wbuf, "%c%c%hd%hd", PKT_FILE, PKT_FILE_DATA, id, len);
	if (Sockbuf_write(&wbuf, buf, len) != len)
		printf("failed sending file data\n");
	return 0;
}

int Send_file_end(int ind, unsigned short id) {
	(void) ind; /* suppress compiler warning */

	Packet_printf(&wbuf, "%c%c%hd", PKT_FILE, PKT_FILE_END, id);
	return 0;
}

#define CHARSCREEN_COLOUR TERM_L_GREEN
#define COL_CHARS 4
void Receive_login(void) {
	int n;
	char ch;
	//assume that 60 is always more than we will need for max_cpa under all circumstances in the future:
	int i = 0, max_cpa = 60, max_cpa_plus = 0;
	short mode = 0;
	char names[max_cpa][MAX_CHARS], colour_sequence[MAX_CHARS];//sinc max_cpa is made ridiculously high anyway, we don't need to add DED_ slots really :-p
	//char names[max_cpa + MAX_DED_IDDC_CHARS + MAX_DED_PVP_CHARS + 1][MAX_CHARS], colour_sequence[3];
	char tmp[MAX_CHARS + 3];	/* like we'll need it... */
	int ded_pvp = 0, ded_iddc = 0, ded_pvp_shown, ded_iddc_shown;
	char loc[MAX_CHARS];

	static char c_name[MAX_CHARS], *cp;
	s16b c_race, c_class, level;

	/* un-hardcode character amount limits */
	int max_ded_pvp_chars = 1, max_ded_iddc_chars = 2;
	int offset, total_cpa;

	bool new_ok = TRUE, exclusive_ok = TRUE;
	bool found_nick = FALSE;


	/* Check if the server wanted to destroy the connection - mikaelh */
	if (rbuf.ptr[0] == PKT_QUIT) {
		errno = 0;
#ifdef RETRY_LOGIN
		rl_connection_destroyed = TRUE;
		plog(&rbuf.ptr[1]);
		/* illegal name? don't suggest it as default again */
		if (strstr(&rbuf.ptr[1], "a different name")) strcpy(nick, "");
		/* no password entered? then auto-fill-in the name again he alread picked */
		else if (strstr(&rbuf.ptr[1], "enter a password")) rl_password = TRUE;
		return;
#endif
		quit(&rbuf.ptr[1]);
		return;
	}
#ifdef RETRY_LOGIN
	/* allow quitting again (especially 'Q' in character screen ;)) */
	rl_connection_destructible = FALSE;
#endif

	/* Read server detail flags for informational purpose - C. Blue */
	if ((n = Packet_scanf(&rbuf, "%c%d%d%d%d", &ch, &sflags3, &sflags2, &sflags1, &sflags0)) <= 0) {
		return;
	}

//#ifdef WINDOWS
	/* only on first time startup? */
	//if (bigmap_hint)
	store_crecedentials();
//#endif

	/* Set server type flags */
	if (sflags0 & SFLG0_RPG) s_RPG = TRUE;
	if (sflags0 & SFLG0_FUN) s_FUN = TRUE;
	if (sflags0 & SFLG0_PARTY) s_PARTY = TRUE;
	if (sflags0 & SFLG0_ARCADE) s_ARCADE = TRUE;
	if (sflags0 & SFLG0_TEST) s_TEST = TRUE;
	if (sflags0 & SFLG0_RPG_ADMIN) s_RPG_ADMIN = TRUE;
	if (sflags0 & SFLG0_DED_IDDC) s_DED_IDDC = TRUE;	/* probably unused */
	if (sflags0 & SFLG0_DED_PVP) s_DED_PVP = TRUE;		/* probably unused */
	if (sflags0 & SFLG0_NO_PK) s_NO_PK = TRUE;

	/* Set client mode */
	if (sflags1 & SFLG1_PARTY) client_mode = CLIENT_PARTY;
	if (!(sflags1 & SFLG1_BIG_MAP) &&
	    (screen_wid != SCREEN_WID || screen_hgt != SCREEN_HGT)) {
		/* BIG_MAP_fallback sort of */
		screen_wid = SCREEN_WID;
		screen_hgt = SCREEN_HGT;
		resize_main_window(CL_WINDOW_WID, CL_WINDOW_HGT);
	}

	/* Set temporary features */
	sflags_TEMP = sflags2;

	/* Abuse flag array #3 for different stuff instead */
	if (!(sflags3 & 0xFF))
		max_cpa = max_chars_per_account = 8; //backward compatibility, no need for version check ;)
	else
		max_cpa = max_chars_per_account = (sflags3 & 0xFF);
	if (!(sflags3 & 0xFF00))
		max_ded_iddc_chars = 2; //backward compatibility, no need for version check ;)
	else
		max_ded_iddc_chars = (sflags3 & 0xFF00) >> 8;
	if (!(sflags3 & 0xFF0000))
		max_ded_pvp_chars = 1; //backward compatibility, no need for version check ;)
	else
		max_ded_pvp_chars = (sflags3 & 0xFF0000) >> 16;

	/* Now that we have the server flags, we can finish setting up Lua - mikaelh */
	open_lua();

	/* read a character name from command-line or config file? prepare it */
	if (cname[0]) {
		cname[0] = toupper(cname[0]);
		if (auto_reincarnation) {
			/* check if valid dna exists for reincarnation */
			load_birth_file(cname);
			if (valid_dna) {
				sex = dna_sex;
				race = dna_race;
				class = dna_class;
				trait = dna_trait;
				return;
			}
		}
	}

	Term_clear();

	if (is_newer_than(&server_version, 4, 5, 8, 1, 0, 0)) {
		if (s_DED_IDDC) max_cpa_plus += max_ded_iddc_chars;
		if (s_DED_PVP) max_cpa_plus += max_ded_pvp_chars;
	} else {
		if (s_DED_IDDC) max_cpa_plus++;
		if (s_DED_PVP) max_cpa_plus++;
	}

	if (s_ARCADE) c_put_str(TERM_SLATE, "The server is running 'ARCADE_SERVER' settings.", 21, 10);
	if (s_RPG) {
		if (!s_ARCADE) c_put_str(TERM_SLATE, "The server is running 'IRONMAN_SERVER' settings.", 21, 10);
		if (!s_RPG_ADMIN) {
			max_cpa = 1;
			max_cpa_plus = 0;
		}
	}
	if (s_TEST) c_put_str(TERM_SLATE, "The server is running 'TEST_SERVER' settings.", 22, 25);
	else if (s_FUN) c_put_str(TERM_SLATE, "The server is running 'FUN_SERVER' settings.", 22, 25);
	if (s_PARTY) c_put_str(TERM_SLATE, "This server is running 'PARTY_SERVER' settings.", 23, 25);

	total_cpa = max_cpa + max_cpa_plus;

	c_put_str(CHARSCREEN_COLOUR, "Character Overview", 0, 30);
	if (max_cpa_plus)
		c_put_str(CHARSCREEN_COLOUR, format("(You can create up to %d+%d different characters to play with)", max_cpa, max_cpa_plus), 1, 10);
	else if (max_cpa > 1)
		c_put_str(CHARSCREEN_COLOUR, format("(You can create up to %d different characters to play with)", max_cpa), 1, 10);
	else
		c_put_str(CHARSCREEN_COLOUR, "(You can create only ONE characters at a time to play with)", 1, 10);

#ifdef ATMOSPHERIC_INTRO
 #ifdef USE_SOUND_2010
  #if 0
	/* Stop music playback from login screen - the character screen has no music, just the firelight sound! */
	if (use_sound) music(-4);
  #else
	if (use_sound) {
		/* switch to login screen music if available, or fade music out */
		if (!music(exec_lua(0, "return get_music_index(\"account\")")))
			music(-4);
	}
  #endif

	/* Play background ambient sound effect (if enabled) */
	if (use_sound) sound_ambient(exec_lua(0, "return get_sound_index(\"ambient_fireplace\")"));
 #endif
	/* left torch */
	c_put_str(TERM_FIRE,  "^",  7, 1);
	c_put_str(TERM_FIRE, " * ", 8, 0);
	c_put_str(TERM_EMBER," # ", 9, 0);
	c_put_str(TERM_UMBER, "|", 10, 1);
	c_put_str(TERM_UMBER, "|", 11, 1);
	c_put_str(TERM_UMBER, "|", 12, 1);
	c_put_str(TERM_UMBER, "|", 13, 1);
	c_put_str(TERM_L_DARK,"~", 14, 1);
	/* right torch */
	c_put_str(TERM_FIRE,  "^",  7, 78);
	c_put_str(TERM_FIRE, " * ", 8, 77);
	c_put_str(TERM_EMBER," # ", 9, 77);
	c_put_str(TERM_UMBER, "|", 10, 78);
	c_put_str(TERM_UMBER, "|", 11, 78);
	c_put_str(TERM_UMBER, "|", 12, 78);
	c_put_str(TERM_UMBER, "|", 13, 78);
	c_put_str(TERM_L_DARK,"~", 14, 78);
#endif

	if (total_cpa <= 12) {
		c_put_str(CHARSCREEN_COLOUR, "Choose an existing character:", 3, 2);
		offset = 4;
	} else offset = 3;

	max_cpa += max_cpa_plus; /* for now, don't keep them in separate list positions
				    but just use the '*' marker attached at server side
				    for visual distinguishing in this character list. */
	/* Receive list of characters ('zero'-terminated) */
	*loc = 0;
	while ((is_newer_than(&server_version, 4, 5, 7, 0, 0, 0) ?
	    (n = Packet_scanf(&rbuf, "%c%hd%s%s%hd%hd%hd%s", &ch, &mode, colour_sequence, c_name, &level, &c_race, &c_class, loc)) :
	    (is_newer_than(&server_version, 4, 4, 9, 2, 0, 0) ?
	    (n = Packet_scanf(&rbuf, "%c%hd%s%s%hd%hd%hd", &ch, &mode, colour_sequence, c_name, &level, &c_race, &c_class)) :
	    (n = Packet_scanf(&rbuf, "%c%s%s%hd%hd%hd", &ch, colour_sequence, c_name, &level, &c_race, &c_class))))
	    > 0) {
//	while ((n = Packet_scanf(&rbuf, "%c%s%s%hd%hd%hd", &ch, colour_sequence, c_name, &level, &c_race, &c_class)) > 0) {
		/* End of character list is designated by a 'zero' character */
		if (!c_name[0]) break;

		/* skip all characters that exceed what our client knows as max_cpa */
		if (i == max_cpa) continue;

		/* read a character name from command-line or config file,
		   that exists, so we log in with it straight away? */
		if (streq(cname, c_name)) return;

		/* do we possess a character of same name as our account? */
		if (streq(c_name, nick)) found_nick = TRUE;

		strcpy(names[i], c_name);

		//sprintf(tmp, "%c) %s%s the level %d %s %s", 'a' + i, colour_sequence, c_name, level, race_info[c_race].title, class_info[c_class].title);
		sprintf(tmp, "%c) %s%s (%d), %s %s", 'a' + i, colour_sequence, c_name, level, race_info[c_race].title, class_info[c_class].title);
		c_put_str(TERM_WHITE, tmp, offset + i, COL_CHARS);

		if (mode & MODE_DED_PVP) {
			ded_pvp++;
			sprintf(tmp, "%s%s", colour_sequence, "PVP");
			c_put_str(TERM_WHITE, tmp, offset + i, 52);
		}
		if (mode & MODE_DED_IDDC) {
			ded_iddc++;
			sprintf(tmp, "%s%s", colour_sequence, "IDDC");
			c_put_str(TERM_WHITE, tmp, offset + i, 52);
		}

		if (*loc) {
			sprintf(tmp, "%s%s", colour_sequence, loc);
			c_put_str(TERM_WHITE, tmp, offset + i, 57);
		}

		i++;
	}
	existing_characters = i;

	ded_pvp_shown = ded_pvp;
	ded_iddc_shown = ded_iddc;
	for (n = max_cpa - i; n > 0; n--) {
		if (is_newer_than(&server_version, 4, 5, 8, 1, 0, 0)) {
			if (max_cpa_plus) {
				if (ded_pvp_shown < max_ded_pvp_chars) {
					c_put_str(TERM_SLATE, "<free PvP-exclusive slot>", offset + i + n - 1, COL_CHARS);
					ded_pvp_shown++;
					continue;
				}
				if (ded_iddc_shown < max_ded_iddc_chars) {
					c_put_str(TERM_SLATE, "<free IDDC-exclusive slot>", offset + i + n - 1, COL_CHARS);
					ded_iddc_shown++;
					continue;
				}
			}
		} else {
			if (max_cpa_plus == 2) {//if it's anything > 0 actually (0 == s_RPG)
				if (!ded_pvp_shown) {
					c_put_str(TERM_SLATE, "<free PvP-exclusive slot>", offset + i + n - 1, COL_CHARS);
					ded_pvp_shown = max_ded_pvp_chars;//hack: 'TRUE'
					continue;
				}
				if (!ded_iddc_shown) {
					c_put_str(TERM_SLATE, "<free IDDC-exclusive slot>", offset + i + n - 1, COL_CHARS);
					ded_iddc_shown = max_ded_iddc_chars;//hack: 'TRUE'
					continue;
				}
			}
		}
		c_put_str(TERM_SLATE, "<free slot>", offset + i + n - 1, COL_CHARS);
	}

	offset += max_cpa + 1;

	/* Only exclusive-slots left? Then don't display 'N' option. */
	if (i - ded_pvp - ded_iddc >= max_cpa - max_ded_pvp_chars - max_ded_iddc_chars) new_ok = FALSE;
	/* Only non-exclusive-slots left? Then don't display 'E' option. */
	if (!((ded_pvp < max_ded_pvp_chars || ded_iddc < max_ded_iddc_chars) && max_cpa_plus)) exclusive_ok = FALSE;

	if (i < max_cpa) {
		if (new_ok) c_put_str(CHARSCREEN_COLOUR, "N) Create a new character", offset, 2);
		if (exclusive_ok) {
			/* hack: no weird modi on first client startup!
			   To find out whether it's 1st or not we check bigmap_hint and # of existing characters.
			   However, we just don't display the choice, it's still choosable! */
			if (!bigmap_hint || existing_characters)
				c_put_str(CHARSCREEN_COLOUR, "E) Create a new slot-exclusive character (IDDC or PvP only)", offset + 1, 2);
		}
	} else {
		c_put_str(CHARSCREEN_COLOUR, format("(Maximum of %d character reached.", max_cpa), offset, 2);
		c_put_str(CHARSCREEN_COLOUR, " Get rid of one (suicide) before creating another.)", offset + 1, 2);
	}

	if (total_cpa <= 13) {
		c_put_str(CHARSCREEN_COLOUR, "Q) Quit the game", offset + 6, 2);
		offset += 3;
	} else if (total_cpa <= 14) {
		c_put_str(CHARSCREEN_COLOUR, "Q) Quit the game", offset + 5, 2);
		offset += 2;
	} else if (total_cpa <= 15) {
		c_put_str(CHARSCREEN_COLOUR, "Q) Quit the game", offset + 4, 2);
		offset += 2;
	} else {
		c_put_str(CHARSCREEN_COLOUR, "Q) Quit the game", offset + 3, 2);
		offset += 2;
	}


	enter_menu:

	/* hack: hide cursor */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;

	while ((ch < 'a' || ch >= 'a' + i) && (((ch != 'N' || !new_ok) && (ch != 'E' || !exclusive_ok)) || i > (max_cpa - 1))) {
		ch = inkey();
		//added CTRL+Q for RETRY_LOGIN, so you can quit the whole game from within in-game via simply double-tapping CTRL+Q
		if (ch == 'Q' || ch == KTRL('Q')) quit(NULL);
		/* Take a screenshot */
		if (ch == KTRL('T')) {
			xhtml_screenshot("screenshot????");
			/* Redraw title line */
			Term_fresh();
#ifdef WINDOWS
			Sleep(1000);
#else
			usleep(1000000);
#endif
			prt("", 0, 0); //clear line
			c_put_str(CHARSCREEN_COLOUR, "Character Overview", 0, 30);
		}
	}
	if (ch == 'N' || ch == 'E') {
		/* We didn't read a desired charname from commandline? */
		if (!cname[0]) {
			/* Reuse last name if we just died? */
			if (prev_cname[0]) strcpy(c_name, prev_cname);
			/* Use account name for character name? */
			else if (!found_nick) strcpy(c_name, nick);
			/* Don't suggest a name */
			else c_name[0] = 0;
		}
		/* Use desired charname from commandline */
		else strcpy(c_name, cname);

		if (total_cpa <= 15)
			c_put_str(TERM_SLATE, "(Press ENTER to pick a random name, ESC to cancel)", offset + 1, COL_CHARS);
		else
			c_put_str(TERM_SLATE, "(Press ENTER to pick a random name, ESC to cancel)", offset, 35);

		while (1) {
			c_put_str(TERM_YELLOW, "New name: ", offset, COL_CHARS);
			if (!askfor_aux(c_name, CHARACTERNAME_LEN - 1, 0)) {
				if (total_cpa <= 15) {
					Term_erase(0, offset, 80);
					Term_erase(0, offset + 1, 80);
				} else
					Term_erase(0, offset, 80);
				ch = 0;
				break;
			}
			else if (strlen(c_name)) break;
			else create_random_name(0, c_name);
		}
		if (!ch) goto enter_menu;

		/* Trim spaces right away */
		strcpy(tmp, c_name);
		cp = tmp;
		while (*cp == ' ') cp++;
		strcpy(c_name, cp);
		cp = c_name + (strlen(c_name) - 1);
		while (*cp == ' ') {
			*cp = 0;
			cp--;
		}

		/* Capitalize the name */
		c_name[0] = toupper(c_name[0]);

		if (ch == 'E') sex |= MODE_DED_PVP | MODE_DED_IDDC;
	}
	else strcpy(c_name, names[ch - 'a']);
	Term_clear();
	strcpy(cname, c_name);
}

/*
 * I haven't really figured out this function yet.  It looks to me like
 * the whole thing could be replaced by one or two reads.  Why they decided
 * to do this using UDP, I'll never know.  -APD
 */

/*
 * I haven't really figured out this function yet too;  however I'm perfectly
 * sure I ruined what it used to do.  Please blame DG instead	- Jir -
 */
int Net_setup(void) {
	int i, n, len, done = 0, j;
	char b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
	long todo = 1;
	char *ptr, str[MAX_CHARS];
	sockbuf_t cbuf;

	/* Initialize a new socket buffer */
	if (Sockbuf_init(&cbuf, -1, CLIENT_RECV_SIZE,
	    SOCKBUF_WRITE | SOCKBUF_READ | SOCKBUF_LOCK) == -1) {
		plog(format("No memory for control buffer (%u)", CLIENT_RECV_SIZE));
		return -1;
	}

	ptr = (char *) &Setup;

	while (todo > 0) {
		if (cbuf.ptr != cbuf.buf)
			Sockbuf_advance(&cbuf, cbuf.ptr - cbuf.buf);

		len = cbuf.len;

		if (len > 0) {
			if (done == 0) {
                                if (is_newer_than(&server_version, 4, 4, 5, 10, 0, 0))
					n = Packet_scanf(&cbuf, "%d%hd%c%c%c%d",
					    &Setup.motd_len, &Setup.frames_per_second, &Setup.max_race, &Setup.max_class, &Setup.max_trait, &Setup.setup_size);
				else {
					n = Packet_scanf(&cbuf, "%d%hd%c%c%d",
					    &Setup.motd_len, &Setup.frames_per_second, &Setup.max_race, &Setup.max_class, &Setup.setup_size);
					Setup.max_trait = 0;
				}

				if (n <= 0) {
					errno = 0;
					quit("Can't read setup info from reliable data buffer");
				}

				/* Make sure the client runs fast enough too - mikaelh */
				if (Setup.frames_per_second > cfg_client_fps)
					cfg_client_fps = Setup.frames_per_second;

				/* allocate the arrays after loading */
				C_MAKE(race_info, Setup.max_race, player_race);
				C_MAKE(class_info, Setup.max_class, player_class);
				if (Setup.max_trait == 0) {
					/* hack: display at least "not available" */
					C_MAKE(trait_info, 1, player_trait);
					trait_info[0].title = "N/A";
					trait_info[0].choice = 0x0;
				} else C_MAKE(trait_info, Setup.max_trait, player_trait);

				for (i = 0; i < Setup.max_race; i++) {
					Packet_scanf(&cbuf, "%c%c%c%c%c%c%s%d", &b1, &b2, &b3, &b4, &b5, &b6, str, &race_info[i].choice);
					race_info[i].title = string_make(str);
					race_info[i].r_adj[0] = b1 - 50;
					race_info[i].r_adj[1] = b2 - 50;
					race_info[i].r_adj[2] = b3 - 50;
					race_info[i].r_adj[3] = b4 - 50;
					race_info[i].r_adj[4] = b5 - 50;
					race_info[i].r_adj[5] = b6 - 50;
				}

				for (i = 0; i < Setup.max_class; i++) {
					Packet_scanf(&cbuf, "%c%c%c%c%c%c%s", &b1, &b2, &b3, &b4, &b5, &b6, str);

					/* Servers > 4.7.0.2 can start class name with a number to indicate extra info: 'hidden' classes. */
					if (isdigit(str[0])) {
						class_info[i].hidden = TRUE;
						class_info[i].base_class = atoi(str);
						class_info[i].title = string_make(str + 2);
					}
					else class_info[i].title = string_make(str);

					class_info[i].c_adj[0] = b1 - 50;
					class_info[i].c_adj[1] = b2 - 50;
					class_info[i].c_adj[2] = b3 - 50;
					class_info[i].c_adj[3] = b4 - 50;
					class_info[i].c_adj[4] = b5 - 50;
					class_info[i].c_adj[5] = b6 - 50;
					if (is_newer_than(&server_version, 4, 4, 3, 1, 0, 0))
						for (j = 0; j < 6; j++) {
							Packet_scanf(&cbuf, "%c", &b1);
							class_info[i].min_recommend[j] = b1;
						}
				}

				if (Setup.max_trait != 0) {
					for (i = 0; i < Setup.max_trait; i++) {
						Packet_scanf(&cbuf, "%s%d", str, &trait_info[i].choice);
						trait_info[i].title = string_make(str);
					}
				}

				ptr = (char *) &Setup;
				done = (char *) &Setup.motd[0] - ptr;
				todo = Setup.motd_len;
			} else {
				memcpy(&ptr[done], cbuf.ptr, len);
				Sockbuf_advance(&cbuf, len + cbuf.ptr - cbuf.buf);
				done += len;
				todo -= len;
			}
		}

		if (todo > 0) {
			if (rbuf.ptr != rbuf.buf)
				Sockbuf_advance(&rbuf, rbuf.ptr - rbuf.buf);

			if (rbuf.len > 0) {
				if (Sockbuf_write(&cbuf, rbuf.ptr, rbuf.len) != rbuf.len) {
					plog("Can't copy data to cbuf");
					Sockbuf_cleanup(&cbuf);
					return -1;
				}
				Sockbuf_clear(&rbuf);
			}

			if (cbuf.ptr != cbuf.buf)
				Sockbuf_advance(&cbuf, cbuf.ptr - cbuf.buf);

			if (cbuf.len > 0)
				continue;

			SetTimeout(5, 0);
			if (!SocketReadable(rbuf.sock)) {
				errno = 0;
				quit("No setup info received");
			}
			while (SocketReadable(rbuf.sock) > 0) {
				Sockbuf_clear(&rbuf);
				if (Sockbuf_read(&rbuf) == -1)
					quit("Can't read all of setup data");
				if (rbuf.len > 0)
					break;
				SetTimeout(0, 0);
			}
		}
	}

	Sockbuf_cleanup(&cbuf);

	return 0;
}

/*
 * Send the first packet to the server with our name,
 * nick and display contained in it.
 * The server uses this data to verify that the packet
 * is from the right UDP connection, it already has
 * this info from the ENTER_GAME_pack.
 */
int Net_verify(char *real, char *nick, char *pass) {
	int	n,
		type,
		result;

	Sockbuf_clear(&wbuf);
	n = Packet_printf(&wbuf, "%c%s%s%s", PKT_VERIFY, real, nick, pass);

	if (n <= 0 || Sockbuf_flush(&wbuf) <= 0)
		plog("Can't send verify packet");

	SetTimeout(5, 0);
	if (!SocketReadable(rbuf.sock)) {
		errno = 0;
		plog("No verify response");
		return -1;
	}
	Sockbuf_clear(&rbuf);
	if (Sockbuf_read(&rbuf) == -1) {
		plog("Can't read verify reply packet");
		return -1;
	}
	if (Sockbuf_flush(&wbuf) == -1)
		return -1;
	if (Receive_reply(&type, &result) <= 0) {
		errno = 0;
		plog("Can't receive verify reply packet");
		return -1;
	}
	if (type != PKT_VERIFY) {
		errno = 0;
		plog(format("Verify wrong reply type (%d)", type));
		return -1;
	}
	if (result != PKT_SUCCESS) {
		errno = 0;
		plog(format("Verification failed (%d)", result));
		return -1;
	}
	if (Receive_magic() <= 0) {
		plog("Can't receive magic packet after verify");
		return -1;
	}

	return 0;
}


/*
 * Open the datagram socket and allocate the network data
 * structures like buffers.
 * Currently there are two different buffers used:
 * 1) wbuf is used only for sending packets (write/printf).
 * 2) rbuf is used for receiving packets in (read/scanf).
 */
int Net_init(int fd) {
	int		 sock;

	/*signal(SIGPIPE, SIG_IGN);*/

	Receive_init();

	sock = fd;

	wbuf.sock = sock;
	if (SetSocketNoDelay(sock, 1) == -1) {
		plog("Can't set TCP_NODELAY on socket");
		return -1;
	}
	if (SetSocketSendBufferSize(sock, CLIENT_SEND_SIZE + 256) == -1)
		plog(format("Can't set send buffer size to %d: error %d", CLIENT_SEND_SIZE + 256, errno));
	if (SetSocketReceiveBufferSize(sock, CLIENT_RECV_SIZE + 256) == -1)
		plog(format("Can't set receive buffer size to %d", CLIENT_RECV_SIZE + 256));

	/* queue buffer, not a valid socket filedescriptor needed */
	if (Sockbuf_init(&qbuf, -1, CLIENT_RECV_SIZE,
	    SOCKBUF_WRITE | SOCKBUF_READ | SOCKBUF_LOCK) == -1) {
		plog(format("No memory for queue buffer (%u)", CLIENT_RECV_SIZE));
		return -1;
	}

	/* read buffer */
	if (Sockbuf_init(&rbuf, sock, CLIENT_RECV_SIZE,
	    SOCKBUF_READ | SOCKBUF_WRITE) == -1) {
		plog(format("No memory for read buffer (%u)", CLIENT_RECV_SIZE));
		return -1;
	}

	/* write buffer */
	if (Sockbuf_init(&wbuf, sock, CLIENT_SEND_SIZE,
	    SOCKBUF_WRITE) == -1) {
		plog(format("No memory for write buffer (%u)", CLIENT_SEND_SIZE));
		return -1;
	}

	/* Initialized */
	initialized = 1;

	return 0;
}


/*
 * Cleanup all the network buffers and close the datagram socket.
 * Also try to send the server a quit packet if possible.
 */
void Net_cleanup(void) {
	int	sock = wbuf.sock;
	char	ch;

	if (sock > 2) {
		ch = PKT_QUIT;
		if (DgramWrite(sock, &ch, 1) != 1) {
			GetSocketError(sock);
			DgramWrite(sock, &ch, 1);
		}
		Term_xtra(TERM_XTRA_DELAY, 50);

		DgramClose(sock);
	}

	Sockbuf_cleanup(&rbuf);
	Sockbuf_cleanup(&wbuf);
	Sockbuf_cleanup(&qbuf);

	/*
	 * Make sure that we won't try to write to the socket again,
	 * after our connection has closed
	 */
	wbuf.sock = -1;
}


/*
 * Flush the network output buffer if it has some data in it.
 * Called by the main loop before blocking on a select(2) call.
 */
int Net_flush(void) {
	if (wbuf.len == 0) {
		wbuf.ptr = wbuf.buf;
		return 0;
	}
	if (Sockbuf_flush(&wbuf) == -1)
		return -1;
	Sockbuf_clear(&wbuf);
	last_send_anything = ticks;
	return 1;
}


/*
 * Return the socket filedescriptor for use in a select(2) call.
 */
int Net_fd(void) {
	if (!initialized || c_quit)
		return -1;
#ifdef RETRY_LOGIN
	/* prevent inkey() from crashing/causing Sockbuf_read() errors ("no read from non-readable socket..") */
	if (rl_connection_state != 1) return -1;
#endif
	return rbuf.sock;
}

unsigned char Net_login() {
	unsigned char tc;

	Sockbuf_clear(&wbuf);
	Packet_printf(&wbuf, "%c%s", PKT_LOGIN, "");
	if (Sockbuf_flush(&wbuf) == -1)
		quit("Can't send first login packet");

	SetTimeout(5, 0);
	if (!SocketReadable(rbuf.sock)) {
		errno = 0;
		quit("No login reply received.");
	}

	Sockbuf_clear(&rbuf);
	Sockbuf_read(&rbuf);
	Receive_login();
#ifdef RETRY_LOGIN
	/* Our connection was destroyed in Receive_login() -> something wrong with our account crecedentials? */
	if (rl_connection_destroyed) return E_RETRY_CONTACT;
	/* Prepare for refusal of a new character name we entered */
	rl_connection_destructible = 1;
#endif

	Packet_printf(&wbuf, "%c%s", PKT_LOGIN, cname);
	if (Sockbuf_flush(&wbuf) == -1)
		quit("Can't send login packet");

	/* Wait for reply - mikaelh */
	SetTimeout(5, 0);
	if (!SocketReadable(rbuf.sock)) {
		errno = 0;
		quit("No login reply received.");
	}

	Sockbuf_read(&rbuf);

	/* Peek in the buffer to check if the server wanted to destroy the
	 * connection - mikaelh */
	if (rbuf.len > 1 && rbuf.ptr[0] == PKT_QUIT) {
#ifdef RETRY_LOGIN
		rl_connection_destroyed = TRUE;
		/* should be illegal character name this time.. */
		plog(&rbuf.ptr[1]);
		return E_RETRY_LOGIN;
#endif
		quit(&rbuf.ptr[1]);
	}
#ifdef RETRY_LOGIN
	/* back to normal - can quit() anytime */
	rl_connection_destructible = 0;
#endif

	if (Packet_scanf(&rbuf, "%c", &tc) <= 0) {
		quit("Failed to read status code from server!");
	}
	return tc;
}

/*
 * Try to send a `start play' packet to the server and get an
 * acknowledgement from the server.  This is called after
 * we have initialized all our other stuff like the user interface
 * and we also have the map already.
 */
int Net_start(int sex, int race, int class) {
	int	i;
	//int n;
	int		type,
			result;

	Sockbuf_clear(&wbuf);
	//n = 
	Packet_printf(&wbuf, "%c", PKT_PLAY);

        if (is_newer_than(&server_version, 4, 4, 5, 10, 0, 0))
		Packet_printf(&wbuf, "%hd%hd%hd%hd%hd%hd", sex, race, class, trait, audio_sfx, audio_music);
	else Packet_printf(&wbuf, "%hd%hd%hd", sex, race, class);

	/* Send the desired stat order */
	for (i = 0; i < 6; i++)
		Packet_printf(&wbuf, "%hd", stat_order[i]);

	/* Send the options */
	if (is_newer_than(&server_version, 4, 5, 8, 1, 0, 1)) {
		for (i = 0; i < OPT_MAX; i++)
			Packet_printf(&wbuf, "%c", Client_setup.options[i]);
	} else if (is_newer_than(&server_version, 4, 5, 5, 0, 0, 0)) {
		for (i = 0; i < OPT_MAX_COMPAT; i++)
			Packet_printf(&wbuf, "%c", Client_setup.options[i]);
	} else {
		for (i = 0; i < OPT_MAX_OLD; i++)
			Packet_printf(&wbuf, "%c", Client_setup.options[i]);
	}

	/* Send screen dimensions */
	if (is_newer_than(&server_version, 4, 4, 9, 1, 0, 1))
		Packet_printf(&wbuf, "%d%d", screen_wid, screen_hgt);

#ifndef BREAK_GRAPHICS
	/* Send the "unknown" redefinitions */
	for (i = 0; i < TV_MAX; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.u_attr[i], Client_setup.u_char[i]);

	if (!is_newer_than(&server_version, 4, 6, 1, 2, 0, 0)) {
		/* Send the "feature" redefinitions */
		for (i = 0; i < MAX_F_IDX_COMPAT; i++)
			Packet_printf(&wbuf, "%c%c", Client_setup.f_attr[i], Client_setup.f_char[i]);

		/* Send the "object" redefinitions */
		for (i = 0; i < MAX_K_IDX_COMPAT; i++)
			Packet_printf(&wbuf, "%c%c", Client_setup.k_attr[i], Client_setup.k_char[i]);

		/* Send the "monster" redefinitions */
		for (i = 0; i < MAX_R_IDX_COMPAT; i++)
			Packet_printf(&wbuf, "%c%c", Client_setup.r_attr[i], Client_setup.r_char[i]);
	} else {
		/* Send the "feature" redefinitions */
		for (i = 0; i < MAX_F_IDX; i++)
			Packet_printf(&wbuf, "%c%c", Client_setup.f_attr[i], Client_setup.f_char[i]);

		/* Send the "object" redefinitions */
		for (i = 0; i < MAX_K_IDX; i++)
			Packet_printf(&wbuf, "%c%c", Client_setup.k_attr[i], Client_setup.k_char[i]);

		/* Send the "monster" redefinitions */
		for (i = 0; i < MAX_R_IDX; i++)
			Packet_printf(&wbuf, "%c%c", Client_setup.r_attr[i], Client_setup.r_char[i]);
	}
#endif

	if (Sockbuf_flush(&wbuf) == -1)
		quit("Can't send start play packet");

	/* Wait for data to arrive */
	SetTimeout(5, 0);
	if (!SocketReadable(rbuf.sock)) {
		errno = 0;
		quit("No play packet reply received.");
	}

	Sockbuf_clear(&rbuf);
	if (Sockbuf_read(&rbuf) == -1) {
		quit("Error reading play reply");
		return -1;
	}

	/* If our connection wasn't accepted, quit */
	if (rbuf.ptr[0] == PKT_QUIT) {
		errno = 0;
		quit(&rbuf.ptr[1]);
		return -1;
	}
	if (rbuf.ptr[0] != PKT_REPLY) {
		errno = 0;
		quit(format("Not a reply packet after play (%d,%d,%d)",
			rbuf.ptr[0], rbuf.ptr - rbuf.buf, rbuf.len));
		return -1;
	}
	if (Receive_reply(&type, &result) <= 0) {
		errno = 0;
		quit("Can't receive reply packet after play");
		return -1;
	}
	if (type != PKT_PLAY) {
		errno = 0;
		quit("Can't receive reply packet after play");
		return -1;
	}
	if (result != PKT_SUCCESS) {
		errno = 0;
		quit(format("Start play not allowed (%d)", result));
		return -1;
	}
	/* Finish processing any commands that were sent to us along with
	 * the PKT_PLAY packet.
	 *
	 * Advance past our PKT_PLAY data
	 * Sockbuf_advance(&rbuf, 3);
	 * Actually process any leftover commands in rbuf
	 */
	if (Net_packet() == -1)
		return -1;

	errno = 0;
	return 0;
}


/*
 * Process a packet.
 */
static int Net_packet(void) {
	int		type,
			prev_type = 0,
			result;
	char *old_ptr;

	/* Process all of the received client updates */
	while (rbuf.buf + rbuf.len > rbuf.ptr) {
		type = (*rbuf.ptr & 0xFF);
#if DEBUG_LEVEL > 2
		if (type > 50) printf("Received packet: %d\n", type);
#endif	/* DEBUG_LEVEL */
		if (receive_tbl[type] == NULL) {
			errno = 0;
#ifndef WIN32 /* suppress annoying msg boxes in windows clients, when unknown-packet-errors occur. */
			plog(format("Received unknown packet type (%d, %d), dropping",
				type, prev_type));
#endif
			/* hack: redraw after a packet was dropped, to make sure we didn't miss something important */
			Send_redraw(0);

			Sockbuf_clear(&rbuf);
			break;
		} else {
			old_ptr = rbuf.ptr;
			rbuf.state |= SOCKBUF_LOCK; /* needed for rollbacks to work properly */
			result = (*receive_tbl[type])();
			rbuf.state &= ~SOCKBUF_LOCK;
			if (result <= 0) {
				if (result == -1) {
					if (type != PKT_QUIT) {
						errno = 0;
						plog(format("Processing packet type (%d, %d) failed", type, prev_type));
					}
					Sockbuf_clear(&rbuf);
					return -1;
				}

				/* Check whether the socket buffer has advanced */
				if (rbuf.ptr == old_ptr) {
					/* Return code 0 means that there wasn't enough data in the socket buffer */
					if (result == 0) break;
				}

				/* Something weird may have happened, clear the socket buffer */
				Sockbuf_clear(&rbuf);

				break;
			}
		}
		prev_type = type;
	}
	return 0;
}

/*
 * Read packets from the net until there are no more available.
 */
int Net_input(void) {
	int	n;
	int	netfd;

	netfd = Net_fd();

	/* SOCKBUF_LOCK may be enabled if we are in a nested input loop */
	rbuf.state &= ~SOCKBUF_LOCK;

	/* Keep reading as long as we have something on the socket */
	while (SocketReadable(netfd)) {
		n = Sockbuf_read(&rbuf);

		if (n == 0) {
			quit("Server closed the connection");
		} else if (n < 0) {
#ifdef RETRY_LOGIN /* not needed */
			/* catch an already destroyed connection - don't call quit() *again*, just go back peacefully; part 2/2 */
//			if (rl_connection_destroyed) return 1;
#endif
			return n;
		} else {
			n = Net_packet();

			/* Make room for more packets */
			Sockbuf_advance(&rbuf, rbuf.ptr - rbuf.buf);

			if (n == -1) return -1;
		}
	}

	return 1;
}

int Flush_queue(void) {
	int	len;

	if (!initialized) return 0;
#ifdef RETRY_LOGIN
	if (rl_connection_state != 1) return 0;
#endif

	len = qbuf.len - (qbuf.ptr - qbuf.buf);

	if (Sockbuf_write(&rbuf, qbuf.ptr, len) != len) {
		plog("Can't copy queued data to buffer");
		return -1;
	}
	Sockbuf_clear(&qbuf);

	Net_packet();

	return 1;
}

/*
 * Receive the end of a new frame update packet,
 * which should contain the same loops number
 * as the frame head.  If this terminating packet
 * is missing then the packet is corrupt or incomplete.
 */
int Receive_end(void) {
	int	n;
	byte	ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return n;
	return 1;
}

/*
 * Magic packets are old relics that don't do anything useful.
 */
int Receive_magic(void) {
	int	n;
	byte	ch;
	unsigned magic;

	if ((n = Packet_scanf(&rbuf, "%c%u", &ch, &magic)) <= 0) return n;
	return 1;
}

int Receive_reply(int *replyto, int *result) {
	int	n;
	byte	type, ch1, ch2;

	n = Packet_scanf(&rbuf, "%c%c%c", &type, &ch1, &ch2);
	if (n <= 0)
		return n;
	if (n != 3 || type != PKT_REPLY) {
		plog("Can't receive reply packet");
		return 1;
	}
	*replyto = ch1;
	*result = ch2;
	return 1;
}

int Receive_quit(void) {
	unsigned char		pkt;
	char			reason[MAX_CHARS_WIDE];

	/* game ends, so leave all other screens like
	   shops or browsed books or skill screen etc */
	if (screen_icky) Term_load();
	topline_icky = FALSE;

	if (Packet_scanf(&rbuf, "%c", &pkt) != 1) {
		errno = 0;
		plog("Can't read quit packet");
	} else {
		if (Packet_scanf(&rbuf, "%s", reason) <= 0)
			strcpy(reason, "unknown reason");
		errno = 0;

		/* Hack -- tombstone */
		if (strstr(reason, "Killed by") ||
		    strstr(reason, "Committed suicide")) {
#ifdef RETRY_LOGIN
			rl_connection_destructible = TRUE;
			rl_connection_state = 2;
#endif
			/* TERAHACK */
			initialized = FALSE;

			c_close_game(reason);
		}
		quit(format("%s", reason));
	}
	return -1;
}

int Receive_sanity(void) {
#ifdef SHOW_SANITY
	int n;
	char ch, buf[MAX_CHARS];
	byte attr;

	if (is_newer_than(&server_version, 4, 6, 1, 2, 0, 0)) {
		char dam;

		if ((n = Packet_scanf(&rbuf, "%c%c%s%c", &ch, &attr, buf, &dam)) <= 0) return n;
 #ifdef USE_SOUND_2010
		/* Send beep when we're losing Sanity while we're busy in some other window */
		if (c_cfg.alert_offpanel_dam && screen_icky && dam) warning_page();
 #endif
	} else if ((n = Packet_scanf(&rbuf, "%c%c%s", &ch, &attr, buf)) <= 0) return n;

	strcpy(c_p_ptr->sanity, buf);
	c_p_ptr->sanity_attr = attr;

	if (screen_icky) Term_switch(0);

	prt_sane(attr, (char*)buf);
	if (c_cfg.alert_hitpoint && (attr == TERM_MULTI)) {
		warning_page();
		c_msg_print("\377R*** LOW SANITY WARNING! ***");
	}

	if (screen_icky) Term_switch(0);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);
#endif
	return 1;
}

int Receive_stat(void) {
	int	n;
	char	ch;
	char	stat;
	s16b	max, cur, s_ind, max_base;

	if ((n = Packet_scanf(&rbuf, "%c%c%hd%hd%hd%hd", &ch, &stat, &max, &cur, &s_ind, &max_base)) <= 0)
		return n;

	p_ptr->stat_top[(int) stat] = max;
	p_ptr->stat_use[(int) stat] = cur;
	p_ptr->stat_ind[(int) stat] = s_ind;
	p_ptr->stat_max[(int) stat] = max_base;

	if (screen_icky) Term_switch(0);
	prt_stat(stat, max, cur, max_base);
	if (screen_icky) Term_switch(0);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_hp(void) {
	int	n;
	char 	ch;
	char	drain;
	s16b	max, cur;
#ifdef USE_SOUND_2010
	static int prev_chp = 0;
#endif

	if (is_newer_than(&server_version, 4, 7, 0, 2, 0, 1)) {
		if ((n = Packet_scanf(&rbuf, "%c%hd%hd%c", &ch, &max, &cur, &drain)) <= 0)
			return n;
	} else {
		drain = FALSE;
		if ((n = Packet_scanf(&rbuf, "%c%hd%hd", &ch, &max, &cur)) <= 0)
			return n;
	}

	p_ptr->mhp = max;
	p_ptr->chp = cur;

#ifdef USE_SOUND_2010
	/* Send beep when we're losing HP while we're busy in some other window */
	if (c_cfg.alert_offpanel_dam && screen_icky && cur < prev_chp && !drain) warning_page();
	if (cur != prev_chp) prev_chp = cur;
#endif

	if (screen_icky) Term_switch(0);

	prt_hp(max, cur);
	if (c_cfg.alert_hitpoint && (cur < max / 5)) {
		warning_page();
		c_msg_print("\377R*** LOW HITPOINT WARNING! ***");
	}

	if (screen_icky) Term_switch(0);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_stamina(void) {
	int	n;
	char 	ch;
	s16b	max, cur;

	if ((n = Packet_scanf(&rbuf, "%c%hd%hd", &ch, &max, &cur)) <= 0)
		return n;

	p_ptr->mst = max;
	p_ptr->cst = cur;

	if (screen_icky) Term_switch(0);
	prt_stamina(max, cur);
	if (screen_icky) Term_switch(0);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_ac(void) {
	int	n;
	char	ch;
	s16b	base, plus;

	if ((n = Packet_scanf(&rbuf, "%c%hd%hd", &ch, &base, &plus)) <= 0) return n;

	p_ptr->dis_ac = base;
	p_ptr->dis_to_a = plus;

	if (screen_icky) Term_switch(0);

	prt_ac(base + plus);

	if (screen_icky) Term_switch(0);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_apply_auto_insc(void) {
	int n;
	char ch, slot;
	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &slot)) <= 0) return n;

	apply_auto_inscriptions((int)slot, FALSE);

	return 1;
}

int Receive_inven(void) {
	int	n;
	char	ch;
	char pos, attr, tval, sval, uses_dir = 0;
	s16b wgt, amt, pval, name1 = 0;
	char name[ONAME_LEN], *insc;

	if (is_newer_than(&server_version, 4, 5, 2, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%hd%c%I", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, &name1, &uses_dir, name)) <= 0)
			return n;
	} else if (is_newer_than(&server_version, 4, 4, 5, 10, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%c%I", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, &uses_dir, name)) <= 0)
			return n;
	} else if (is_newer_than(&server_version, 4, 4, 4, 2, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%I", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, name)) <= 0)
			return n;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%s", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, name)) <= 0)
			return n;
	}

	/* Check that the inventory slot is valid - mikaelh */
	if (pos < 'a' || pos > 'x') return 0;

        /* Hack -- The color is stored in the sval, since we don't use it for anything else */
        /* Hack -- gotta ahck to work around the previous hackl .. damn I hate that */
		/* I'm one of those who really hate it .. Jir */
		/* I hated it too much I swapped them	- Jir - */
	inventory[pos - 'a'].sval = sval;
	inventory[pos - 'a'].tval = tval;
	inventory[pos - 'a'].pval = pval;
	inventory[pos - 'a'].name1 = name1;
	inventory[pos - 'a'].attr = attr;
	inventory[pos - 'a'].weight = wgt;
	inventory[pos - 'a'].number = amt;
	inventory[pos - 'a'].uses_dir = uses_dir;

	/* check for special "fake-artifact" inscription using '#' char */
	if ((insc = strchr(name, '\373'))) {
		inventory_inscription[pos - 'a'] = insc - name;
		inventory_inscription_len[pos - 'a'] = insc[1] - 1;
		/* delete the special code garbage from the name to make it human-readable */
		do {
			*insc = *(insc + 2);
			insc++;
		} while (*(insc + 1));
	} else inventory_inscription[pos - 'a'] = 0;

	strncpy(inventory_name[pos - 'a'], name, ONAME_LEN - 1);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN);

	return 1;
}

/* Receive xtra data of an item. Added for customizable spell tomes - C. Blue */
int Receive_inven_wide(void) {
	int	n;
	char	ch;
	char pos, attr, tval, sval, *insc;
	s16b xtra1, xtra2, xtra3, xtra4, xtra5, xtra6, xtra7, xtra8, xtra9;
	byte xtra1b, xtra2b, xtra3b, xtra4b, xtra5b, xtra6b, xtra7b, xtra8b, xtra9b;
	s16b wgt, amt, pval, name1 = 0;
	char name[ONAME_LEN];

	if (is_newer_than(&server_version, 4, 7, 0, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%hd%hd%hd%hd%hd%hd%hd%hd%hd%hd%I", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, &name1,
		    &xtra1, &xtra2, &xtra3, &xtra4, &xtra5, &xtra6, &xtra7, &xtra8, &xtra9, name)) <= 0)
			return n;
	} else if (is_newer_than(&server_version, 4, 5, 2, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%hd%c%c%c%c%c%c%c%c%c%I", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, &name1,
		    &xtra1b, &xtra2b, &xtra3b, &xtra4b, &xtra5b, &xtra6b, &xtra7b, &xtra8b, &xtra9b, name)) <= 0)
			return n;
		xtra1 = (s16b)xtra1b; xtra2 = (s16b)xtra2b; xtra3 = (s16b)xtra3b;
		xtra4 = (s16b)xtra4b; xtra5 = (s16b)xtra5b; xtra6 = (s16b)xtra6b;
		xtra7 = (s16b)xtra7b; xtra8 = (s16b)xtra8b; xtra9 = (s16b)xtra9b;
	} else if (is_newer_than(&server_version, 4, 4, 4, 2, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%c%c%c%c%c%c%c%c%c%I", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval,
		    &xtra1b, &xtra2b, &xtra3b, &xtra4b, &xtra5b, &xtra6b, &xtra7b, &xtra8b, &xtra9b, name)) <= 0)
			return n;
		xtra1 = (s16b)xtra1b; xtra2 = (s16b)xtra2b; xtra3 = (s16b)xtra3b;
		xtra4 = (s16b)xtra4b; xtra5 = (s16b)xtra5b; xtra6 = (s16b)xtra6b;
		xtra7 = (s16b)xtra7b; xtra8 = (s16b)xtra8b; xtra9 = (s16b)xtra9b;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%c%c%c%c%c%c%c%c%c%s", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval,
		    &xtra1b, &xtra2b, &xtra3b, &xtra4b, &xtra5b, &xtra6b, &xtra7b, &xtra8b, &xtra9b, name)) <= 0)
			return n;
		xtra1 = (s16b)xtra1b; xtra2 = (s16b)xtra2b; xtra3 = (s16b)xtra3b;
		xtra4 = (s16b)xtra4b; xtra5 = (s16b)xtra5b; xtra6 = (s16b)xtra6b;
		xtra7 = (s16b)xtra7b; xtra8 = (s16b)xtra8b; xtra9 = (s16b)xtra9b;
	}

	/* Check that the inventory slot is valid - mikaelh */
	if (pos < 'a' || pos > 'x') return 0;

        /* Hack -- The color is stored in the sval, since we don't use it for anything else */
        /* Hack -- gotta ahck to work around the previous hackl .. damn I hate that */
		/* I'm one of those who really hate it .. Jir */
		/* I hated it too much I swapped them	- Jir - */
	inventory[pos - 'a'].sval = sval;
	inventory[pos - 'a'].tval = tval;
	inventory[pos - 'a'].pval = pval;
	inventory[pos - 'a'].name1 = name1;
	inventory[pos - 'a'].attr = attr;
	inventory[pos - 'a'].weight = wgt;
	inventory[pos - 'a'].number = amt;

	inventory[pos - 'a'].xtra1 = xtra1;
	inventory[pos - 'a'].xtra2 = xtra2;
	inventory[pos - 'a'].xtra3 = xtra3;
	inventory[pos - 'a'].xtra4 = xtra4;
	inventory[pos - 'a'].xtra5 = xtra5;
	inventory[pos - 'a'].xtra6 = xtra6;
	inventory[pos - 'a'].xtra7 = xtra7;
	inventory[pos - 'a'].xtra8 = xtra8;
	inventory[pos - 'a'].xtra9 = xtra9;

	/* check for special "fake-artifact" inscription using '#' char */
	if ((insc = strchr(name, '\373'))) {
		inventory_inscription[pos - 'a'] = insc - name;
		inventory_inscription_len[pos - 'a'] = insc[1] - 1;
		/* delete the special code garbage from the name to make it human-readable */
		do {
			*insc = *(insc + 2);
			insc++;
		} while (*(insc + 1));
	} else inventory_inscription[pos - 'a'] = 0;

	strncpy(inventory_name[pos - 'a'], name, ONAME_LEN - 1);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN);

	return 1;
}

/* Receive some unique list info for client-side processing - C. Blue */
int Receive_unique_monster(void) {
	int	n;
	char	ch;
	int	u_idx, killed;
	char name[60];

	if ((n = Packet_scanf(&rbuf, "%c%d%d%s", &ch, &u_idx, &killed, name)) <= 0) return n;

	r_unique[u_idx] = killed;
	strcpy(r_unique_name[u_idx], name);
	return 1;
}

/* Descriptions for equipment slots - mikaelh */
char *equipment_slot_names[] = {
	"(weapon)",
	"(weapon / shield)",
	"(shooter)",
	"(ring)",
	"(ring)",
	"(amulet)",
	"(light source)",
	"(body armour)",
	"(cloak)",
	"(hat)",
	"(gloves)",
	"(boots)",
	"(quiver)",
	"(tool)"
};

int Receive_equip(void) {
	int	n;
	char 	ch;
	char pos, attr, tval, sval, uses_dir;
	s16b wgt, amt, pval, name1 = 0;
	char name[ONAME_LEN];

	if (is_newer_than(&server_version, 4, 5, 2, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%hd%c%I", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, &name1, &uses_dir, name)) <= 0)
			return n;
	} else if (is_newer_than(&server_version, 4, 4, 5, 10, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%c%I", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, &uses_dir, name)) <= 0)
			return n;
	} else if (is_newer_than(&server_version, 4, 4, 4, 2, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%I", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, name)) <= 0)
			return n;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%s", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, name)) <= 0)
			return n;
	}

	/* Check that the equipment slot is valid - mikaelh */
	if (pos < 'a' || pos > 'n') return 0;

	inventory[pos - 'a' + INVEN_WIELD].sval = sval;
	inventory[pos - 'a' + INVEN_WIELD].tval = tval;
	inventory[pos - 'a' + INVEN_WIELD].pval = pval;
	inventory[pos - 'a' + INVEN_WIELD].name1 = name1;
	inventory[pos - 'a' + INVEN_WIELD].attr = attr;
	inventory[pos - 'a' + INVEN_WIELD].weight = wgt;
	inventory[pos - 'a' + INVEN_WIELD].number = amt;
	inventory[pos - 'a' + INVEN_WIELD].uses_dir = uses_dir;

	if (!strcmp(name, "(nothing)"))
		strcpy(inventory_name[pos - 'a' + INVEN_WIELD], equipment_slot_names[pos - 'a']);
	else
		strncpy(inventory_name[pos - 'a' + INVEN_WIELD], name, ONAME_LEN - 1);

	/* new hack for '(unavailable)' equipment slots: handle INVEN_ARM slot a bit cleaner: */
	if (pos == 'a') {
		if (!strcmp(name, "(unavailable)")) equip_no_weapon = TRUE;
		else equip_no_weapon = FALSE;
	} else if (pos == 'b' && equip_no_weapon && !strcmp(name, "(nothing)"))
		strcpy(inventory_name[INVEN_ARM], "(shield)");

	/* Window stuff */
	p_ptr->window |= (PW_EQUIP);

	return 1;
}

int Receive_char_info(void) {
	int	n;
	char	ch;

#ifndef RETRY_LOGIN
	static bool player_pref_files_loaded = FALSE;
#endif

	/* Clear any old info */
	race = class = trait = sex = mode = 0;

	if (is_newer_than(&server_version, 4, 5, 2, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd%hd%hd%s", &ch, &race, &class, &trait, &sex, &mode, cname)) <= 0) return n;
	} else if (is_newer_than(&server_version, 4, 4, 5, 10, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd%hd%hd", &ch, &race, &class, &trait, &sex, &mode)) <= 0) return n;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd%hd", &ch, &race, &class, &sex, &mode)) <= 0) return n;
	}

	p_ptr->prace = race;
	p_ptr->rp_ptr = &race_info[race];
	p_ptr->pclass = class;
	p_ptr->cp_ptr = &class_info[class];
	p_ptr->ptrait = trait;
	p_ptr->tp_ptr = &trait_info[trait];
	p_ptr->male = sex;
	p_ptr->mode = mode;

	/* Load preferences once */
	if (!player_pref_files_loaded) {
		initialize_player_pref_files();
		player_pref_files_loaded = TRUE;

		global_big_map_hold = FALSE; //process BIG_MAP (option<7>) changes finally, all accumulated
		check_immediate_options(7, c_cfg.big_map, TRUE);

		/* Character Overview Resist/Boni/Abilities Page on Startup? - Kurzel */
		if (c_cfg.overview_startup) csheet_page = 2;

		/* hack: also load auto-inscriptions here */
		initialize_player_ins_files();

		/* Pref files may change settings, so reload the keymap - mikaelh */
		keymap_init();
	}

	if (screen_icky) Term_switch(0);
	prt_basic();
	if (screen_icky) Term_switch(0);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_various(void) {
	int	n;
	char	ch, buf[MAX_CHARS];
	s16b	hgt, wgt, age, sc;

	if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu%hu%s", &ch, &hgt, &wgt, &age, &sc, buf)) <= 0) return n;

	p_ptr->ht = hgt;
	p_ptr->wt = wgt;
	p_ptr->age = age;
	p_ptr->sc = sc;
	strcpy(c_p_ptr->body_name, buf);

	/*printf("Received various info: height %d, weight %d, age %d, sc %d\n", hgt, wgt, age, sc);*/

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_plusses(void) {
	int	n;
	char	ch;
	s16b	dam, hit;
	s16b	dam_r, hit_r;
	s16b	dam_m, hit_m;

	if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd%hd%hd%hd", &ch, &hit, &dam, &hit_r, &dam_r, &hit_m, &dam_m)) <= 0) return n;

	p_ptr->dis_to_h = hit;
	p_ptr->dis_to_d = dam;
	p_ptr->to_h_ranged = hit_r;
	p_ptr->to_d_ranged = dam_r;
	p_ptr->to_h_melee = hit_m;
	p_ptr->to_d_melee = dam_m;

	/*printf("Received plusses: +%d tohit +%d todam\n", hit, dam);*/

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_experience(void) {
	int	n;
	char	ch;
	s32b	max, cur, adv, adv_prev;
	s16b	lev, max_lev, max_plv;

	if (is_newer_than(&server_version, 4, 5, 6, 0, 0, 1)) {
		if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu%d%d%d%d", &ch, &lev, &max_lev, &max_plv, &max, &cur, &adv, &adv_prev)) <= 0) return n;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu%d%d%d", &ch, &lev, &max_lev, &max_plv, &max, &cur, &adv)) <= 0) return n;
		adv_prev = adv;
	}

	/* unhack exp_frac marker */
	if (lev >= 1000) {
		lev -= 1000;
		half_exp = 10;
	} else half_exp = 0;

	p_ptr->lev = lev;
	p_ptr->max_lev = max_lev;
	p_ptr->max_plv = max_plv;
	p_ptr->max_exp = max;
	p_ptr->exp = cur;
	exp_adv = adv;
	exp_adv_prev = adv_prev;

	if (screen_icky) Term_switch(0);
	prt_level(lev, max_lev, max_plv, max, cur, adv, exp_adv_prev);
	if (screen_icky) Term_switch(0);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_skill_init(void) {
	int	n;
	char	ch;
	u16b	i;
	u16b	father, mkey, order;
	char    name[MSG_LEN], desc[MSG_LEN], act[MSG_LEN];
	u32b	flags1;
	byte	tval;

	if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd%hd%d%c%S%S%S", &ch, &i,
	    &father, &order, &mkey, &flags1, &tval, name, desc, act)) <= 0) return n;

	/* XXX XXX These are x32b, not char * !!!!!
	 * It's really needed that we separate c-types.h from types.h
	 * Now they're uintptr */
	s_info[i].name = (uintptr)string_make(name);
	s_info[i].desc = (uintptr)string_make(desc);
	s_info[i].action_desc = (uintptr)(strlen(act) ? string_make(act) : 0L);

	s_info[i].father = father;
	s_info[i].order = order;
	s_info[i].action_mkey = mkey;
	s_info[i].flags1 = flags1;
	s_info[i].tval = tval;

	return 1;
}

int Receive_skill_points(void) {
	int	n;
        char	ch;
	int	pt;

	if ((n = Packet_scanf(&rbuf, "%c%d", &ch, &pt)) <= 0) return n;

	p_ptr->skill_points = pt;

	/* Redraw the skill menu */
	redraw_skills = TRUE;

	return 1;
}

int Receive_skill_info(void) {
	int	n;
        char	ch;
        s32b    val;
	int	i, mod, dev, mkey;
	char	flags1;

	if (is_newer_than(&server_version, 4, 4, 4, 1, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%d%d%d%d%c%d", &ch, &i, &val, &mod, &dev, &flags1, &mkey)) <= 0) return n;
	} else {
		int hidden, dummy;

		if ((n = Packet_scanf(&rbuf, "%c%d%d%d%d%d%d%d", &ch, &i, &val, &mod, &dev, &hidden, &mkey, &dummy)) <= 0) return n;

		flags1 = 0;
		if (hidden) flags1 |= SKF1_HIDDEN;
		if (dummy) flags1 |= SKF1_DUMMY;
	}

	p_ptr->s_info[i].value = val;
	p_ptr->s_info[i].mod = mod;
	if (is_newer_than(&server_version, 4, 4, 6, 2, 0, 0)) {
		switch (dev) {
		case -1: break;
		case 0: p_ptr->s_info[i].dev = FALSE; break;
		case 1: p_ptr->s_info[i].dev = TRUE; break;
		}
	} else p_ptr->s_info[i].dev = dev;

	s_info[i].action_mkey = mkey;
	p_ptr->s_info[i].flags1 = flags1;

	/* Redraw the skill menu */
	redraw_skills = TRUE;

	return 1;
}

int Receive_gold(void) {
	int	n;
	char	ch;
	int	gold, balance;

	if ((n = Packet_scanf(&rbuf, "%c%d%d", &ch, &gold, &balance)) <= 0) return n;

	p_ptr->au = gold;
	p_ptr->balance = balance;

	if (shopping) {
		/* Display the players remaining gold */
		c_store_prt_gold();
	} else {
		if (screen_icky) Term_switch(0);
		prt_gold(gold);
		if (screen_icky) Term_switch(0);
	}

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_sp(void) {
	int	n;
	char	ch;
	s16b	max, cur;

	if ((n = Packet_scanf(&rbuf, "%c%hd%hd", &ch, &max, &cur)) <= 0) return n;

	if (c_cfg.alert_mana && max != -9999 && (cur < max / 5)) {
		warning_page();
		c_msg_print("\377R*** LOW MANA WARNING! ***");
	}

	p_ptr->msp = max;
	p_ptr->csp = cur;

	if (screen_icky) Term_switch(0);
	prt_sp(max, cur);
	if (screen_icky) Term_switch(0);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_history(void) {
	int	n;
	char	ch;
	s16b	line;
	char	buf[MAX_CHARS];

	if ((n = Packet_scanf(&rbuf, "%c%hu%s", &ch, &line, buf)) <= 0) return n;

	strcpy(p_ptr->history[line], buf);

	/*printf("Received history line %d: %s\n", line, buf);*/

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_char(void) {
	int	n;
	char	ch;
	char	x, y;
	byte	a;
	char	c;
	bool is_us = FALSE;

	if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c", &ch, &x, &y, &a, &c)) <= 0) return n;

	/* Old cfg.hilite_player implementation has been disabled after 4.6.1.1 because it interferes with custom fonts */
	if (!is_newer_than(&server_version, 4, 6, 1, 1, 0, 1)) {
		if ((c & 0x80)) {
			c &= 0x7F;
			is_us = TRUE;
		}
	}

#ifdef TEST_CLIENT
	/* special hack for mind-link Windows->Linux w/ font_map_solid_walls */
	/* NOTE: We need a better solution than this for custom fonts... */
 #ifndef WINDOWS
	if (c == 127) c = 2;
 #else
	if (c == 2) c = 127;
 #endif
#endif

	/* remember map_info in client-side buffer */
	if (x >= PANEL_X && x < PANEL_X + screen_wid &&
	    y >= PANEL_Y && y < PANEL_Y + screen_hgt) {
		panel_map_a[x - PANEL_X][y - PANEL_Y] = a;
		panel_map_c[x - PANEL_X][y - PANEL_Y] = c;
	}

	if (screen_icky) Term_switch(0);
	if (is_us && c_cfg.hilite_player) {
		/* Mark our own position via special cursor */
#if 0
		/* save previous cursor vis/loc? -- not necessary? */
		Term_get_cursor(&v);
		Term_locate(&x, &y);
#endif
		//for gcu: Term_xtra(TERM_XTRA_SHAPE, 2);

		Term_set_cursor(1);
		Term_gotoxy(x, y);

		/* for X11:
		Term_curs_x11() ->
		Infofnt_text_non() : draw cursor XRectangle
		*/
		
	}
	Term_draw(x, y, a, c);
	if (screen_icky) Term_switch(0);
	return 1;
}

int Receive_message(void) {
	int	n, c;
	char	ch;
	char	buf[MSG_LEN], *bptr, *sptr, *bnptr;
	char	l_buf[MSG_LEN], l_cname[NAME_LEN], *ptr, l_nick[NAME_LEN], called_name[NAME_LEN];

	if ((n = Packet_scanf(&rbuf, "%c%S", &ch, buf)) <= 0) return n;

	/* XXX Mega-hack -- because we are not using checksums, sometimes under
	 * heavy traffic Receive_line_input receives corrupted data, which will cause
	 * the run-length encoded algorithm to exit prematurely.  Since there is no
	 * end of line_info tag, the packet interpretor assumes the line_input data
	 * is finished, and attempts to parse the next byte as a new packet type.
	 * Since the ascii value of '.' (a frequently updated character) is 46,
	 * which equals PKT_MESSAGE, if corrupted line_info data is received this function
	 * may get called wihtout a valid string.  To try to prevent the client from
	 * displaying a garbled string and messing up our screen, we will do a quick
	 * sanity check on the string.  Note that this might screw up people using
	 * international character sets.  (Such as the Japanese players)
	 *
	 * A better solution for this would be to impliment some kind of packet checksum.
	 * -APD
	 */

	/* perform a sanity check on our string */
	/* Hack -- ' ' is numericall the lowest charcter we will probably be trying to
	 * display.  This might screw up international character sets.
	 */
	for (c = 0; c < n; c++) 
		if (buf[c] < ' ' && /* exempt control codes */
		    buf[c] != -1 && /* \377 colour code */
		    buf[c] != -2 && /* \376 important-scrollback code */
		    buf[c] != -3 && /* \375 chat-only code */
		    buf[c] != -4) /* \374 chat+no-chat code */
			return 1;

	if (screen_icky && (!shopping || perusing)) Term_switch(0);


	/* Highlight or beep on incoming chat messages containing our name?
	   Current weakness: Char 'Mim' (acc 'Test) won't get highlights from
	   another char 'Test' (acc 'Test') writing 'Mim' in chat. - C. Blue */
	if (c_cfg.hilite_chat || c_cfg.hibeep_chat) { /* enabled? */
		/* Test sender's name, if it is us */
		int we_sent_offset;
		char *we_sent_p = strchr(buf, '[');
		if (we_sent_p) {
			char we_sent_buf[NAME_LEN + 1 + 10];
			strncpy(we_sent_buf, we_sent_p + 1, NAME_LEN + 1 + 10);
			we_sent_buf[NAME_LEN + 10] = '\0';
			if (strchr(we_sent_buf, ']')) {
				char exact_name[NAME_LEN + 1], *en_p = exact_name;
				/* we found SOME name, so don't test it again */
				we_sent_offset = strchr(buf, ']') - buf;
				/* and also check if name = us, strictly */
				we_sent_p = we_sent_buf - 1;
				while (*(++we_sent_p)) {
					/* skip colour codes (occur in the beginning and end of name, possibly) */
					if (*we_sent_p == '\\') {
						we_sent_p++;
						continue;
					}
					if (*we_sent_p == ']') break;
					*en_p++ = *we_sent_p;
				}
				*en_p = '\0';
				/* so, if it's not someone else talking, whose name _contains_ our name (eg 'Bat' vs 'FruitBat'),
				   then we can allow highlighting the (our) name again: */
				if (!strcmp(exact_name, cname)) we_sent_offset = 0;
			} else we_sent_offset = 0;
		} else we_sent_offset = 0; /* just paranoid initialization */

		/* Is it non-private chat? */
		if (strlen(buf) > 2 && //paranoia, not needed
		    (sptr = strchr(buf, '[')) /* a '[' occurs somewhere at the start? */
		    && sptr <= buf + 7 + ((strstr(buf, "(IRC)") <= buf + 7) ? 9 : 0)
		    //&& (*(sptr - 1) != 'y')
		    //&& (*(sptr - 1) != 'G')
		    && (*(sptr - 1) != 'g') /* and it's not coloured as a private message? */
		    && tolower(buf[2]) == toupper(buf[2]) /* even safer check, that it isn't a generic server message */
		    ) {
			/* my_strcasestr() */
			strcpy(l_buf, buf);
			ptr = l_buf;
			while (*ptr) { *ptr = tolower(*ptr); ptr++; }
			strcpy(l_cname, cname);
			ptr = l_cname;
			while (*ptr) { *ptr = tolower(*ptr); ptr++; }
			strcpy(l_nick, nick);
			ptr = l_nick;
			while (*ptr) { *ptr = tolower(*ptr); ptr++; }
			/* map location found onto original string -_- */
			if ((bptr = strstr(l_buf + we_sent_offset, l_cname))) bptr = buf + (bptr - l_buf);
			if ((bnptr = strstr(l_buf, l_nick))) bnptr = buf + (bnptr - l_buf);

			/* check that our 'name' wasn't just part of a different string,
			   eg 'Heya' if our name is 'Eya' (important for very short names): */
			if (bptr && bptr > buf && *(bptr - 1) >= 'a' && *(bptr - 1) <= 'z') bptr = NULL;
			if (bptr && *(bptr + strlen(l_cname)) && *(bptr + strlen(l_cname)) >= 'a' && *(bptr + strlen(l_cname)) <= 'z') bptr = NULL;
			if (bnptr && bnptr > buf && *(bnptr - 1) >= 'a' && *(bnptr - 1) <= 'z') bnptr = NULL;
			if (bnptr && *(bnptr + strlen(l_nick)) && *(bnptr + strlen(l_nick)) >= 'a' && *(bnptr + strlen(l_nick)) <= 'z') bnptr = NULL;

			/* Check which one it is (first), character name or account name */
			if (bptr) {
				if (!bnptr || bptr <= bnptr) {
					/* keep the way our name was actually written (lower/upper-case) in the original chat message */
					strncpy(called_name, bptr, strlen(cname));
					called_name[strlen(cname)] = 0;
				} else {
					bptr = bnptr;
					/* keep the way our name was actually written (lower/upper-case) in the original chat message */
					strncpy(called_name, bptr, strlen(nick));
					called_name[strlen(nick)] = 0;
				}
			} else if (bnptr) {
				bptr = bnptr;
				/* keep the way our name was actually written (lower/upper-case) in the original chat message */
				strncpy(called_name, bptr, strlen(nick));
				called_name[strlen(nick)] = 0;
			}

			/* our name occurs in the message? */
			if (bptr &&
			    bptr > sptr + 2 && /* and isn't the sender of this message? */
			    (!(ptr = strchr(sptr + 1, '(')) || ptr > sptr + 3 || bptr > ptr + 6)) { /* mind '(G)' and '(P)' */
				/* enough space to add colour codes for highlighting? */
				if (c_cfg.hilite_chat
				    && strlen(buf) < MSG_LEN - 4) {
					char buf2[MSG_LEN], *col_ptr = buf;
					int prev_colour = 'w';

					/* remember last colour used before our name occurred, so we can restore it */
					while (col_ptr < bptr) {
						if (*col_ptr == '\377') {
							col_ptr++;
							prev_colour = *col_ptr;
						} else col_ptr++;
					}

					strcpy(buf2, buf);
					strcpy(bptr, "\377R");
					strcpy(bptr + 2, called_name);
					strcpy(bptr + 2 + strlen(called_name), format("\377%c", prev_colour));
					strcpy(bptr + 4 + strlen(called_name), buf2 + (bptr - buf) + strlen(called_name));
				}

				/* also give audial feedback if enabled */
				if (c_cfg.hibeep_chat) page();
			}
		}
	}

	c_msg_print(buf);

	if (screen_icky && (!shopping || perusing)) Term_switch(0);

	return 1;
}

int Receive_state(void) {
	int	n;
	char	ch;
	s16b	paralyzed, searching, resting;

	if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu", &ch, &paralyzed, &searching, &resting)) <= 0) return n;

	if (screen_icky) Term_switch(0);
	prt_state(paralyzed, searching, resting);
	if (screen_icky) Term_switch(0);
	return 1;
}

int Receive_title(void) {
	int	n;
	char	ch;
	char	buf[MAX_CHARS];

#ifdef BIGMAP_MINDLINK_HACK
	/* hack for big_map visuals: are we on a bigger display linked to a smaller display?
	   hack explanation: mindlinking sends both PR_MAP and PR_TITLE, and title is sent
	   _after_ map, so we can track the amount of map lines we received in Receive_line_info()
	   first and then do the final check and visuals here. */
	if (last_line_y <= SCREEN_HGT && !screen_icky) {
		/* black out the unused part for better visual quality */
		for (n = 1 + SCREEN_HGT; n < 1 + SCREEN_HGT * 2; n++)
			Term_erase(SCREEN_PAD_LEFT, n, 255);
	}
#endif

	if ((n = Packet_scanf(&rbuf, "%c%s", &ch, buf)) <= 0) return n;

	/* XXX -- Extract "ghost-ness" */
	p_ptr->ghost = streq(buf, "Ghost") || streq(buf, "\377rGhost (dead)");
	/* extract winner state */
	p_ptr->total_winner = strstr(buf, "**") || strstr(buf, "\377v");

	if (screen_icky) Term_switch(0);
	prt_title(buf);
	if (screen_icky) Term_switch(0);
	return 1;
}

int Receive_depth(void) {
	int	n;
	char	ch;
	s16b	x, y, z, old_colour;
	char	colour, colour_sector;
	bool	town;
	char	buf[MAX_CHARS];

	/* Compatibility with older servers */
	if (is_newer_than(&server_version, 4, 6, 1, 2, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu%c%c%c%s%s%s", &ch, &x, &y, &z, &town, &colour, &colour_sector, buf, location_name2, location_pre)) <= 0)
			return n;
	} else if (is_newer_than(&server_version, 4, 5, 9, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu%c%c%c%s%s", &ch, &x, &y, &z, &town, &colour, &colour_sector, buf, location_name2)) <= 0)
			return n;
		if (location_name2[0]) strcpy(location_pre, "in");
		else strcpy(location_pre, "of");
	} else if (is_newer_than(&server_version, 4, 4, 1, 6, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu%c%c%c%s", &ch, &x, &y, &z, &town, &colour, &colour_sector, buf)) <= 0)
			return n;
		if (buf[0]) strcpy(location_pre, "in");
		else strcpy(location_pre, "of");
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu%c%hu%s", &ch, &x, &y, &z, &town, &old_colour, buf)) <= 0)
			return n;
		colour = old_colour;
		colour_sector = TERM_L_GREEN;
		if (buf[0]) strcpy(location_pre, "in");
		else strcpy(location_pre, "of");
	}

	/* Compatibility with older servers - mikaelh */
	if (!is_newer_than(&server_version, 4, 4, 1, 5, 0, 0)) {
		if (colour) colour = TERM_ORANGE;
		else colour = TERM_WHITE;
	}

	if (screen_icky) Term_switch(0);

	p_ptr->wpos.wx = x;
	p_ptr->wpos.wy = y;
	p_ptr->wpos.wz = z;
	strncpy(c_p_ptr->location_name, buf, 20);
	c_p_ptr->location_name[19] = 0;
	prt_depth(x, y, z, town, colour, colour_sector, buf);

	/* HACK - Also draw AFK status now to fully redraw the entire row - mikaelh */
	prt_AFK(p_ptr->afk);

	if (screen_icky) Term_switch(0);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_confused(void) {
	int	n;
	char	ch;
	bool	confused;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &confused)) <= 0) return n;

	if (screen_icky) Term_switch(0);
	prt_confused(confused);
	if (screen_icky) Term_switch(0);
	return 1;
}

int Receive_poison(void) {
	int	n;
	char	ch;
	bool	poison;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &poison)) <= 0) return n;

	if (screen_icky) Term_switch(0);
	prt_poisoned(poison);
	if (screen_icky) Term_switch(0);
	return 1;
}

int Receive_study(void) {
	int	n;
	char	ch;
	bool	study;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &study)) <= 0) return n;

	if (screen_icky) Term_switch(0);
	prt_study(study);
	if (screen_icky) Term_switch(0);
	return 1;
}

int Receive_bpr(void) {
	int	n;
	char	ch;
	byte	bpr, attr;

	if ((n = Packet_scanf(&rbuf, "%c%c%c", &ch, &bpr, &attr)) <= 0) return n;

	if (screen_icky) Term_switch(0);
	prt_bpr(bpr, attr);
	if (screen_icky) Term_switch(0);
	return 1;
}

int Receive_food(void) {
	int	n;
	char	ch;
	u16b	food;

	if ((n = Packet_scanf(&rbuf, "%c%hu", &ch, &food)) <= 0) return n;

	if (screen_icky) Term_switch(0);
	prt_hunger(food);
	if (screen_icky) Term_switch(0);
	return 1;
}

int Receive_fear(void) {
	int	n;
	char	ch;
	bool	afraid;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &afraid)) <= 0) return n;

	if (screen_icky) Term_switch(0);
	prt_afraid(afraid);
	if (screen_icky) Term_switch(0);
	return 1;
}

int Receive_speed(void) {
	int	n;
	char	ch;
	s16b	speed;

	if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &speed)) <= 0) return n;

	if (screen_icky) Term_switch(0);
	prt_speed(speed);
	if (screen_icky) Term_switch(0);
	return 1;
}

int Receive_cut(void) {
	int	n;
	char	ch;
	s16b	cut;

	if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &cut)) <= 0) return n;

	if (screen_icky) Term_switch(0);
	prt_cut(cut);
	if (screen_icky) Term_switch(0);
	return 1;
}

int Receive_blind(void) {
	int	n;
	char	ch;
	bool	blind;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &blind)) <= 0) return n;

	if (screen_icky) Term_switch(0);
	prt_blind(blind);
	if (screen_icky) Term_switch(0);
	return 1;
}

int Receive_stun(void) {
	int	n;
	char	ch;
	s16b	stun;

	if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &stun)) <= 0) return n;

	if (screen_icky) Term_switch(0);
	prt_stun(stun);
	if (screen_icky) Term_switch(0);
	return 1;
}

int Receive_item(void) {
	char	ch, th = ITH_NONE;
	int	n, item;

	if (is_newer_than(&server_version, 4, 5, 2, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &th)) <= 0) return n;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return n;
	}

	if (!screen_icky && !topline_icky) {
		switch (th) {
		case ITH_NONE:
		default:
			if (th >= ITH_MAX_WEIGHT || th < 0) {
				get_item_extra_hook = get_item_hook_find_obj;
				item_tester_max_weight = th > 0 ? th - 50 : 256 + th - 50; //paranoia @ 'signed' char =-p
				/* paranoia */
				if (!item_tester_max_weight) item_tester_max_weight = 4; //guaranteed minimum: 0.4 lbs
			}
			break;
		case ITH_RECHARGE:
			get_item_extra_hook = get_item_hook_find_obj;
			item_tester_hook = item_tester_hook_device;
			get_item_hook_find_obj_what = "Device name? ";
			break;
		case ITH_ENCH_AC:
			get_item_extra_hook = get_item_hook_find_obj;
			item_tester_hook = item_tester_hook_armour;
			get_item_hook_find_obj_what = "Armour name? ";
			break;
		case ITH_ENCH_WEAP:
			get_item_extra_hook = get_item_hook_find_obj;
			item_tester_hook = item_tester_hook_weapon;
			get_item_hook_find_obj_what = "Weapon name? ";
			break;
		case ITH_CUSTOM_TOME:
			get_item_extra_hook = get_item_hook_find_obj;
			item_tester_hook = item_tester_hook_custom_tome;
			get_item_hook_find_obj_what = "Book name? ";
			break;
		case ITH_RUNE:
			get_item_extra_hook = get_item_hook_find_obj;
			item_tester_hook = item_tester_hook_rune;
			get_item_hook_find_obj_what = "Rune name? ";
			break;
		case ITH_ENCH_AC_NO_SHIELD:
			get_item_extra_hook = get_item_hook_find_obj;
			item_tester_hook = item_tester_hook_armour_no_shield;
			get_item_hook_find_obj_what = "Armour name? ";
			break;
		}

		clear_topline();
		if (!c_get_item(&item, "Which item? ", (USE_EQUIP | USE_INVEN | USE_EXTRA))) return 1;
		Send_item(item);
	} else {
		if (is_newer_than(&server_version, 4, 5, 2, 0, 0, 0)) {
			if ((n = Packet_printf(&qbuf, "%c%c", ch, th)) <= 0) return n;
		} else {
			if ((n = Packet_printf(&qbuf, "%c", ch)) <= 0) return n;
		}
	}
	return 1;
}

/* for DISCRETE_SPELL_SYSTEM: DSS_EXPANDED_SCROLLS */
int Receive_spell_request(void) {
	char	ch;
	int	n, item, spell;

	if ((n = Packet_scanf(&rbuf, "%c%d", &ch, &item)) <= 0) return n;

	if (!screen_icky && !topline_icky) {
		clear_topline();
		/* Ask for a spell, allow cancel */
		if ((spell = get_school_spell("choose", &item)) == -1) return 1;
		Send_spell(item, spell);
	} else {
		if ((n = Packet_printf(&qbuf, "%c%d", ch, item)) <= 0) return n;
	}
	return 1;
}

int Receive_spell_info(void) {
	char	ch;
	int	n;
	u16b	realm, book, line;
	char	buf[MAX_CHARS];
	s32b    spells[3];

	if ((n = Packet_scanf(&rbuf, "%c%d%d%d%hu%hu%hu%s", &ch, &spells[0], &spells[1], &spells[2], &realm, &book, &line, buf)) <= 0) return n;

	/* Save the info */
        strcpy(spell_info[realm][book][line], buf);
	p_ptr->innate_spells[0] = spells[0];
	p_ptr->innate_spells[1] = spells[1];
	p_ptr->innate_spells[2] = spells[2];

	/* Assume that this was in response to a shapeshift command we issued */
	command_confirmed = PKT_ACTIVATE_SKILL;

	return 1;
}

int Receive_technique_info(void) {
	char	ch;
	int	n;
	s32b    melee, ranged;

	if ((n = Packet_scanf(&rbuf, "%c%d%d", &ch, &melee, &ranged)) <= 0) return n;

	/* Save the info */
	p_ptr->melee_techniques = melee;
	p_ptr->ranged_techniques = ranged;

	return 1;
}

int Receive_direction(void) {
	char	ch;
	int	n, dir = 0;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return n;

	if (!screen_icky && !topline_icky && !shopping) {
		/* Ask for a direction */
		if (!get_dir(&dir)) return 0;

		/* Send it back */
		if ((n = Packet_printf(&wbuf, "%c%c", PKT_DIRECTION, dir)) <= 0) return n;
	} else if ((n = Packet_printf(&qbuf, "%c", ch)) <= 0) return n;

	return 1;
}

int Receive_flush(void) {
	char	ch;
	int	n;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return n;

	/* Flush the terminal */
	Term_fresh();

	/* Disable the delays altogether? */
	if (c_cfg.disable_flush) return 1;

	/* Wait */
	/*
	 * NOTE: this is very what makes the server freeze when a pack of
	 * hounds breath at the player.. 1ms is too long.	- Jir -
	 *
	 * thin_down_flush is a crude hack to save this from happening.
	 * of course, better if we can specify 0.3ms sleep - helas, windoze
	 * client doesn't support that :-/
	 */
#if 0
	if (!thin_down_flush || magik(33)) Term_xtra(TERM_XTRA_DELAY, 1);
#else
	if (c_cfg.thin_down_flush) {
		if (++flush_count > 10) return 1;
	}

	Term_xtra(TERM_XTRA_DELAY, 1);
#endif

	return 1;
}


int Receive_line_info(void) {
	char	ch, c;
	int	x, i, n;
	s16b	y;
	byte	a;
	byte	rep;
	bool	draw = FALSE;
	char *stored_sbuf_ptr = rbuf.ptr;

	if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &y)) <= 0) return n;

	if (screen_icky && ch != PKT_MINI_MAP) Term_switch(0);

#if 0
	/* If this is the mini-map then we can draw if the screen is icky */
	if (ch == PKT_MINI_MAP || (!screen_icky && !shopping))
		draw = TRUE;
#else
	draw = TRUE;
#endif
	/* Check the max line count */
	if (y > last_line_info)
		last_line_info = y;

#ifdef BIGMAP_MINDLINK_HACK
	/* for big_map mind-link issues: keep track of last map line received */
	last_line_y = y;
#endif

	for (x = 0; x < 80; x++) {
		/* Read the char/attr pair */
		if ((n = Packet_scanf(&rbuf, "%c%c", &c, &a)) <= 0) {
			if (n == 0) goto rollback;
			return n;
		}

		/* 4.4.3.1 servers use a = 0xFF to signal RLE */
		if (is_newer_than(&server_version, 4, 4, 3, 0, 0, 5)) {
			/* New RLE */
			if (a == 0xFF) {
				/* Read the real attr and number of repetitions */
				if ((n = Packet_scanf(&rbuf, "%c%c", &a, &rep)) <= 0) {
					if (n == 0) goto rollback;
					return n;
				}
			} else {
				/* No RLE, just one instance */
				rep = 1;
			}
		} else {
			/* Check for bit 0x40 on the attribute */
			if (a & 0x40) {
				/* First, clear the bit */
				a &= ~(0x40);

				/* Read the number of repetitions */
				if ((n = Packet_scanf(&rbuf, "%c", &rep)) <= 0) {
					if (n == 0) goto rollback;
					return n;
				}
			} else {
				/* No RLE, just one instance */
				rep = 1;
			}
		}

		/* Don't draw anything if "char" is zero */
		if (c && draw) {
#ifdef TEST_CLIENT
			/* special hack for mind-link Windows->Linux w/ font_map_solid_walls */
 #ifndef WINDOWS
			if (c == 127) c = 2;
 #else
			if (c == 2) c = 127;
 #endif
#endif
			/* Draw a character 'rep' times */
			for (i = 0; i < rep; i++) {
				/* remember map_info in client-side buffer */
				if (ch != PKT_MINI_MAP &&
				    x + i >= PANEL_X && x + i < PANEL_X + screen_wid &&
				    y >= PANEL_Y && y < PANEL_Y + screen_hgt) {
					panel_map_a[x + i - PANEL_X][y - PANEL_Y] = a;
					panel_map_c[x + i - PANEL_X][y - PANEL_Y] = c;
				}

				Term_draw(x + i, y, a, c);
			}
		}

		/* Reset 'x' to the correct value */
		x += rep - 1;

		/* hack -- if x > 80, assume we have received corrupted data,
		 * flush our buffers
		 */
		if (x > 80) Sockbuf_clear(&rbuf);
	}

	if (screen_icky && ch != PKT_MINI_MAP) Term_switch(0);

	return 1;

	/* Rollback the socket buffer in case the packet isn't complete */
	rollback:
	rbuf.ptr = stored_sbuf_ptr;
	if (screen_icky && ch != PKT_MINI_MAP) Term_switch(0); /* needed to avoid garbage on screen */
	return 0;
}

/*
 * Note that this function does not honor the "screen_icky"
 * flag, as it is only used for displaying the mini-map,
 * and the screen should be icky during that time.
 */
int Receive_mini_map(void) {
	char	ch, c;
	int	n, x, bytes_read;
	s16b	y;
	byte	a;

	if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &y)) <= 0) return n;

	bytes_read = 3;

	/* Check the max line count */
	if (y > last_line_info)
		last_line_info = y;

	for (x = 0; x < 80; x++) {
		if ((n = Packet_scanf(&rbuf, "%c%c", &c, &a)) <= 0) {
			/* Rollback the socket buffer */
			Sockbuf_rollback(&rbuf, bytes_read);

			/* Packet isn't complete, graceful failure */
			return n;
		}

		bytes_read += n;

		/* Don't draw anything if "char" is zero */
		/* Only draw if the screen is "icky" */
		if (c && screen_icky && x < 80 - 12)
			Term_draw(x + 12, y, a, c);
	}

	return 1;
}

int Receive_mini_map_pos(void) {
	char	ch, c;
	int n;
	short int x, y;
	byte	a;

	if ((n = Packet_scanf(&rbuf, "%c%hd%hd%c%c", &ch, &x, &y, &a, &c)) <= 0) return n;

	minimap_posx = (int)x;
	minimap_posy = (int)y;
	minimap_attr = a;
	minimap_char = c;

	return 1;
}

int Receive_special_other(void) {
	int	n;
	char	ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return n;

	/* Set file perusal method to "other" */
	special_line_type = SPECIAL_FILE_OTHER;

	/* Peruse the file we're about to get */
	peruse_file();

	return 1;
}

int Receive_store_action(void) {
	int	n;
	short	bact, action, cost;
	char	ch, pos, name[MAX_CHARS], letter, attr;
	byte	flag;

	if ((n = Packet_scanf(&rbuf, "%c%c%hd%hd%s%c%c%hd%c", &ch, &pos, &bact, &action, name, &attr, &letter, &cost, &flag)) <= 0) return n;

	c_store.bact[(int)pos] = bact;
	c_store.actions[(int)pos] = action;
	strncpy(c_store.action_name[(int) pos], name, 40);
	c_store.action_attr[(int)pos] = attr;
	c_store.letter[(int)pos] = letter;
	c_store.cost[(int)pos] = cost;
	c_store.flags[(int)pos] = flag;

	/* Make sure that we're in a store */
	if (shopping) display_store_action();

	return 1;
}

int Receive_store(void) {
	int	n, price;
	char	ch, pos, name[ONAME_LEN], tval, sval;
	byte	attr;
	s16b	wgt, num, pval;

	if (is_newer_than(&server_version, 4, 4, 7, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hd%hd%d%S%c%c%hd", &ch, &pos, &attr, &wgt, &num, &price, name, &tval, &sval, &pval)) <= 0)
			return n;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hd%hd%d%s%c%c%hd", &ch, &pos, &attr, &wgt, &num, &price, name, &tval, &sval, &pval)) <= 0)
			return n;
	}

	store.stock[(int)pos].sval = sval;
	store.stock[(int)pos].weight = wgt;
	store.stock[(int)pos].number = num;
	store_prices[(int) pos] = price;
	strncpy(store_names[(int) pos], name, ONAME_LEN - 1);
	store_names[(int) pos][ONAME_LEN - 1] = 0;
	store.stock[(int)pos].tval = tval;
	store.stock[(int)pos].attr = attr;
	store.stock[(int)pos].pval = pval;

	/* Request a redraw of the store inventory */
	redraw_store = TRUE;

	return 1;
}

int Receive_store_wide(void) {
	int	n, price;
	char	ch, pos, name[ONAME_LEN], tval, sval;
	byte	attr;
	s16b	wgt, num, pval;
	s16b	xtra1, xtra2, xtra3, xtra4, xtra5, xtra6, xtra7, xtra8, xtra9;
	byte	xtra1b, xtra2b, xtra3b, xtra4b, xtra5b, xtra6b, xtra7b, xtra8b, xtra9b;

	if (is_newer_than(&server_version, 4, 7, 0, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hd%hd%d%S%c%c%hd%hd%hd%hd%hd%hd%hd%hd%hd%hd", &ch, &pos, &attr, &wgt, &num, &price, name, &tval, &sval, &pval,
		    &xtra1, &xtra2, &xtra3, &xtra4, &xtra5, &xtra6, &xtra7, &xtra8, &xtra9)) <= 0)
			return n;
	} else if (is_newer_than(&server_version, 4, 4, 7, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hd%hd%d%S%c%c%hd%c%c%c%c%c%c%c%c%c", &ch, &pos, &attr, &wgt, &num, &price, name, &tval, &sval, &pval,
		    &xtra1b, &xtra2b, &xtra3b, &xtra4b, &xtra5b, &xtra6b, &xtra7b, &xtra8b, &xtra9b)) <= 0)
			return n;
		xtra1 = (s16b)xtra1b; xtra2 = (s16b)xtra2b; xtra3 = (s16b)xtra3b;
		xtra4 = (s16b)xtra4b; xtra5 = (s16b)xtra5b; xtra6 = (s16b)xtra6b;
		xtra7 = (s16b)xtra7b; xtra8 = (s16b)xtra8b; xtra9 = (s16b)xtra9b;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hd%hd%d%s%c%c%hd%c%c%c%c%c%c%c%c%c", &ch, &pos, &attr, &wgt, &num, &price, name, &tval, &sval, &pval,
		    &xtra1b, &xtra2b, &xtra3b, &xtra4b, &xtra5b, &xtra6b, &xtra7b, &xtra8b, &xtra9b)) <= 0)
			return n;
		xtra1 = (s16b)xtra1b; xtra2 = (s16b)xtra2b; xtra3 = (s16b)xtra3b;
		xtra4 = (s16b)xtra4b; xtra5 = (s16b)xtra5b; xtra6 = (s16b)xtra6b;
		xtra7 = (s16b)xtra7b; xtra8 = (s16b)xtra8b; xtra9 = (s16b)xtra9b;
	}

	store.stock[(int)pos].sval = sval;
	store.stock[(int)pos].weight = wgt;
	store.stock[(int)pos].number = num;
	store_prices[(int) pos] = price;
	strncpy(store_names[(int) pos], name, ONAME_LEN - 1);
	store_names[(int) pos][ONAME_LEN - 1] = 0;
	store.stock[(int)pos].tval = tval;
	store.stock[(int)pos].attr = attr;
	store.stock[(int)pos].pval = pval;
	store.stock[(int)pos].xtra1 = xtra1;
	store.stock[(int)pos].xtra2 = xtra2;
	store.stock[(int)pos].xtra3 = xtra3;
	store.stock[(int)pos].xtra4 = xtra4;
	store.stock[(int)pos].xtra5 = xtra5;
	store.stock[(int)pos].xtra6 = xtra6;
	store.stock[(int)pos].xtra7 = xtra7;
	store.stock[(int)pos].xtra8 = xtra8;
	store.stock[(int)pos].xtra9 = xtra9;

	/* Request a redraw of the store inventory */
	redraw_store = TRUE;

	return 1;
}

/* For new SPECIAL store flag, stores that don't have inventory - C. Blue */
int Receive_store_special_str(void) {
	int n;
	char ch, line, col, attr;
	char str[MAX_CHARS];

	if ((n = Packet_scanf(&rbuf, "%c%c%c%c%s", &ch, &line, &col, &attr, str)) <= 0)
		return n;
	if (!shopping) return 1;

	c_put_str(attr, str, line, col);

	/* hack: hide cursor */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;

	return 1;
}

/* For new SPECIAL store flag, stores that don't have inventory - C. Blue */
int Receive_store_special_char(void) {
	int n;
	char ch, line, col, attr;
	char c, str[2];

	if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c", &ch, &line, &col, &attr, &c)) <= 0)
		return n;
	if (!shopping) return 1;

	str[0] = c;
	str[1] = 0;
	c_put_str(attr, str, line, col);

	/* hack: hide cursor */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;

	return 1;
}

/* For new SPECIAL store flag, stores that don't have inventory - C. Blue */
int Receive_store_special_clr(void) {
	int n;
	char ch, line_start, line_end;

	if ((n = Packet_scanf(&rbuf, "%c%c%c", &ch, &line_start, &line_end)) <= 0)
		return n;
	if (!shopping) return 1;

	for (n = line_start; n <= line_end; n++)
		c_put_str(TERM_WHITE, "                                                                                ", n, 0);

	/* hack: hide cursor */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;

	return 1;
}

int Receive_store_info(void) {
	int	n, max_cost;
	char	ch, owner_name[MAX_CHARS] , store_name[MAX_CHARS];
	s16b	num_items;
	byte store_attr = TERM_SLATE;
	char store_char = '?';

	if (is_newer_than(&server_version, 4, 4, 4, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%hd%s%s%hd%d%c%c", &ch, &store_num, store_name, owner_name, &num_items, &max_cost, &store_attr, &store_char)) <= 0) return n;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%hd%s%s%hd%d", &ch, &store_num, store_name, owner_name, &num_items, &max_cost)) <= 0) return n;
	}

	store.stock_num = num_items >= 0 ? num_items : 0; /* Hack: Use num_items to encode SPECIAL store flag */
	c_store.max_cost = max_cost;
	strncpy(c_store.owner_name, owner_name, 40);
	strncpy(c_store.store_name, store_name, 40);
	c_store.store_attr = store_attr;
	c_store.store_char = store_char;

	/* Only enter "display_store" if we're not already shopping */
	if (!shopping) {
		if (num_items >= 0) display_store(); /* Normal NPC or player store */
		else display_store_special(); /* Special NPC store */
	} else {
		/* Request a redraw of the store inventory */
		redraw_store = TRUE;
	}

	return 1;
}

int Receive_store_kick(void) {
	int	n;
	char	ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return n;

	/* Leave the store */
	leave_store = TRUE;

	return 1;
}

int Receive_sell(void) {
	int	n, price;
	char	ch, buf[1024];

	if ((n = Packet_scanf(&rbuf, "%c%d", &ch, &price)) <= 0) return n;

	/* Tell the user about the price */
	if (c_cfg.no_verify_sell) Send_store_confirm();
	else {
		if (store_num == 57) sprintf(buf, "Really donate it?");
		else sprintf(buf, "Accept %d gold?", price);

		if (get_check2(buf, FALSE))
			Send_store_confirm();
	}

	return 1;
}

int Receive_target_info(void) {
	int	n;
	char	ch, x, y, buf[MAX_CHARS];

	if ((n = Packet_scanf(&rbuf, "%c%c%c%s", &ch, &x, &y, buf)) <= 0) return n;

	/* Print the message */
	if (c_cfg.target_history) c_msg_print(buf);
	else {
		/* Clear the topline */
		clear_topline();

		/* Display target info */
		put_str(buf, 0, 0);

		/* Also add the target info as a normal message */
		if (c_cfg.targetinfo_msg) c_message_add(buf);
	}

	/* Move the cursor */
	Term_gotoxy(x, y);

	return 1;
}

int Receive_sound(void) {
	int	n;
	char	ch, s;
	int	s1, s2, t = -1, v = 100;
	s32b	id;

	if (is_newer_than(&server_version, 4, 4, 5, 3, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%d%d%d%d%d", &ch, &s1, &s2, &t, &v, &id)) <= 0)
			return n;
	} else if (is_newer_than(&server_version, 4, 4, 5, 1, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%d%d%d", &ch, &s1, &s2, &t)) <= 0)
			return n;
	} else if (is_newer_than(&server_version, 4, 4, 5, 0, 0, 0)) {
		/* Primary sound and an alternative */
		if ((n = Packet_scanf(&rbuf, "%c%d%d", &ch, &s1, &s2)) <= 0) return n;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &s)) <= 0) return n;
		s1 = s;
	}

	/* Make a sound (if allowed) */
	if (use_sound) {
		if (t == SFX_TYPE_WEATHER && (noweather_mode || c_cfg.no_weather)) return 1;

#ifndef USE_SOUND_2010
		Term_xtra(TERM_XTRA_SOUND, s1);
#else
		if (!sound(s1, t, v, id)) sound(s2, t, v, id);
#endif
	}

	return 1;
}

int Receive_music(void) {
	int	n;
	char	ch, m, m2 = -1;

	if (is_newer_than(&server_version, 4, 5, 6, 0, 0, 1)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c", &ch, &m, &m2)) <= 0) return n;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &m)) <= 0) return n;
	}

#ifdef USE_SOUND_2010
	/* Play background music (if enabled) */
	if (!use_sound) return 1;
	/* Try to play music, if fails try alternative music, if fails too stop playing any music. */
	if (!music(m) && !music(m2)) music(-2);
#endif

	return 1;
}

int Receive_sfx_ambient(void) {
	int	n, a;
	char	ch;

	if ((n = Packet_scanf(&rbuf, "%c%d", &ch, &a)) <= 0) return n;

#ifdef USE_SOUND_2010
	/* Play background ambient sound effect (if enabled) */
	if (use_sound) sound_ambient(a);
#endif

	return 1;
}

int Receive_sfx_volume(void) {
	int	n;
	char	ch, c_ambient, c_weather;

	if ((n = Packet_scanf(&rbuf, "%c%c%c", &ch, &c_ambient, &c_weather)) <= 0) return n;

#ifdef USE_SOUND_2010
	/* Change volume of background ambient sound effects or weather (if enabled) */
	grid_ambient_volume_goal = (int)c_ambient;
	grid_weather_volume_goal = (int)c_weather;

	/* Determine step size (1 step per 1/10 s, aka 1 tick in do_ping()) */
	grid_ambient_volume_step = (grid_ambient_volume_goal - grid_ambient_volume) / 30;
	grid_weather_volume_step = (grid_weather_volume_goal - grid_weather_volume) / 30;
	/* catch rounding problems */
	if (grid_ambient_volume_goal != grid_ambient_volume && !grid_ambient_volume_step) grid_ambient_volume_step = (grid_ambient_volume_goal > grid_ambient_volume) ? 1 : -1;
	if (grid_weather_volume_goal != grid_weather_volume && !grid_weather_volume_step) grid_weather_volume_step = (grid_weather_volume_goal > grid_weather_volume) ? 1 : -1;
#endif

	return 1;
}

int Receive_boni_col(void) {
	int	n, j;

	byte ch, i;
	char spd, slth, srch, infr, lite, dig, blow, crit, shot, migh, mxhp, mxmp, luck, pstr, pint, pwis, pdex, pcon, pchr, amfi = 0, sigl = 0;
	byte cb[16];
	char color, symbol;

	if (is_newer_than(&server_version, 4, 6, 1, 2, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", &ch, //1+22+16+2 bytes in total
		&i, &spd, &slth, &srch, &infr, &lite, &dig, &blow, &crit, &shot,
		&migh, &mxhp, &mxmp, &luck, &pstr, &pint, &pwis, &pdex, &pcon, &pchr, &amfi, &sigl,
		&cb[0], &cb[1], &cb[2], &cb[3], &cb[4], &cb[5], &cb[6], &cb[7], &cb[8], &cb[9],
		&cb[10], &cb[11], &cb[12], &cb[13], &cb[14], &cb[15], &color, &symbol)) <= 0) return n;
	} else if (is_newer_than(&server_version, 4, 5, 9, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", &ch, //1+22+13+2 bytes in total
		&i, &spd, &slth, &srch, &infr, &lite, &dig, &blow, &crit, &shot,
		&migh, &mxhp, &mxmp, &luck, &pstr, &pint, &pwis, &pdex, &pcon, &pchr, &amfi, &sigl,
		&cb[0], &cb[1], &cb[2], &cb[3], &cb[4], &cb[5], &cb[6], &cb[7], &cb[8], &cb[9],
		&cb[10], &cb[11], &cb[12], &color, &symbol)) <= 0) return n;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", &ch, //1+20+13+2 bytes in total
		&i, &spd, &slth, &srch, &infr, &lite, &dig, &blow, &crit, &shot,
		&migh, &mxhp, &mxmp, &luck, &pstr, &pint, &pwis, &pdex, &pcon, &pchr,
		&cb[0], &cb[1], &cb[2], &cb[3], &cb[4], &cb[5], &cb[6], &cb[7], &cb[8], &cb[9],
		&cb[10], &cb[11], &cb[12], &color, &symbol)) <= 0) return n;
	}

	/* Store the boni global variables */
	csheet_boni[i].spd = spd;
	csheet_boni[i].slth = slth;
	csheet_boni[i].srch = srch;
	csheet_boni[i].infr = infr;
	csheet_boni[i].lite = lite;
	csheet_boni[i].dig = dig;
	csheet_boni[i].blow = blow;
	csheet_boni[i].crit = crit;
	csheet_boni[i].shot = shot;
	csheet_boni[i].migh = migh;
	csheet_boni[i].mxhp = mxhp;
	csheet_boni[i].mxmp = mxmp;
	csheet_boni[i].luck = luck;
	csheet_boni[i].pstr = pstr;
	csheet_boni[i].pint = pint;
	csheet_boni[i].pwis = pwis;
	csheet_boni[i].pdex = pdex;
	csheet_boni[i].pcon = pcon;
	csheet_boni[i].pchr = pchr;
	csheet_boni[i].amfi = amfi;
	csheet_boni[i].sigl = sigl;
	for (j = 0; j < 16; j++)
	csheet_boni[i].cb[j] = cb[j];
	csheet_boni[i].color = color;
	csheet_boni[i].symbol = symbol;

	/* Window Display */
	if (csheet_page == 2) { //Hardcode
		p_ptr->window |= PW_PLAYER;
		window_stuff();
	}

	return 1;
}

int Receive_special_line(void) {
	int	n;
	char	ch, attr;
	s32b	max, line;
	char	buf[ONAME_LEN]; /* Allow colour codes! (was: MAX_CHARS, which is just 80) */
	int	x, y, phys_line;

	if (is_newer_than(&server_version, 4, 4, 7, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%d%d%c%I", &ch, &max, &line, &attr, buf)) <= 0) return n;
	} else {
		s16b old_max, old_line;
		if ((n = Packet_scanf(&rbuf, "%c%hd%hd%c%I", &ch, &old_max, &old_line, &attr, buf)) <= 0) return n;
		max = old_max;
		line = old_line;
	}

	/* Hack - prepare for a special sized page (# of lines divisable by n) */
	if (line >= 21 + HGT_PLUS) {
		//21 -> % 1, 22 -> %2, 23 -> %3..
		special_page_size = 21 + HGT_PLUS - ((21 + HGT_PLUS) % (line - (20 + HGT_PLUS)));
		return 1;
	}

	/* Maximum (initialize / update) */
	if (max_line != max) {
		max_line = max;

		/* Are we still browsing or have we actually just quit? */
		if (special_line_type) {
			/* Update the prompt too (important if max_line got smaller).
			   (Prompt consistent with peruse_file() in c-files.c.)*/
			/* indicate EOF by different status line colour */
			if (cur_line + special_page_size >= max_line)
				c_prt(TERM_ORANGE, format("[Press Return, Space, -, b, or ESC to exit.] (%d-%d/%d)",
				    cur_line + 1, max_line , max_line), 23 + HGT_PLUS, 0);
			else
				c_prt(TERM_L_WHITE, format("[Press Return, Space, -, b, or ESC to exit.] (%d-%d/%d)",
				    cur_line + 1, cur_line + special_page_size, max_line), 23 + HGT_PLUS, 0);
		}
	}

#if 1 /* this on-the-fly recognition is probably only needed if the marker '21 lines' package is late to arrive? */
	/* Recognize a +1 extra line setup (usually divisable by 3 -> 21 instead of 20) - C. Blue */
	if (line == 20 + HGT_PLUS) special_page_size = 21 + HGT_PLUS;
#endif

	/* Also adjust our current starting line to the possibly updated max_line in case
	   max_line got smaller for some reason (for example when viewing equipment).
	   This is kept consistent with behaviour in peruse_file() in c-files.c and
	   do_cmd_help_aux() in files.c */
	if (cur_line > max_line - special_page_size &&
	    cur_line < max_line) {
		cur_line = max_line - special_page_size;
		if (cur_line < 0) cur_line = 0;
	}

	/* Cause inkey() to break, if inkey_max_line flag was set */
	inkey_max_line = FALSE;

	/* Print out the info */
	if (special_line_type) { /* If we have quit already, dont */
		/* remember cursor position */
		Term_locate(&x, &y);

		/* Hack: 'Title line' */
		if (line == -1) phys_line = 0;
		/* Normal line */
		else {
			phys_line = line +
			    (special_page_size == 21 + HGT_PLUS ? 1 : 2) + /* 1 extra usable line for 21-lines mode? */
			    (special_page_size < 20 + HGT_PLUS ? 1 : 0); /* only 40 lines on 42-lines BIG_MAP? -> slighly improved visuals ;) */

			/* Keep in sync: If peruse_file() displays a title, it must be line + 2.
			   Else line + 1 to save some space for 21-lines (odd_line) feature. */

			/* Always clear the whole line first */
			Term_erase(0, phys_line, 255);
		}

		c_put_str(attr, buf, phys_line, 0);

		/* restore cursor position */
		Term_gotoxy(x, y);
	}

	return 1;
}

int Receive_floor(void) {
	int	n;
	char	ch, tval;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &tval)) <= 0) return n;

	/* Ignore for now */
	//tval = tval;

	return 1;
}

int Receive_pickup_check(void) {
	int	n;
	char	ch, buf[MAX_CHARS];

	if ((n = Packet_scanf(&rbuf, "%c%s", &ch, buf)) <= 0) return n;

	/* Get a check */
	if (get_check2(buf, FALSE)) {
		/* Pick it up */
		Send_stay();
	}

	return 1;
}


int Receive_party_stats(void) {
	int n,j,k,chp, mhp, csp, msp, color;
	char ch, partymembername[MAX_CHARS];

	if ((n = Packet_scanf(&rbuf, "%c%d%d%s%d%d%d%d%d", &ch, &j, &color, &partymembername, &k, &chp, &mhp, &csp, &msp)) <= 0) return n;

	if (screen_icky) Term_switch(0);
	prt_party_stats(j,color,partymembername,k,chp,mhp,csp,msp);
	if (screen_icky) Term_switch(0);
	return 1;
}


int Receive_party(void) {
	int n;
	char ch, pname[MAX_CHARS], pmembers[MAX_CHARS], powner[MAX_CHARS];

	if ((n = Packet_scanf(&rbuf, "%c%s%s%s", &ch, pname, pmembers, powner)) <= 0) return n;

	/* Copy info */
	strcpy(party_info_name, pname);
	strcpy(party_info_members, pmembers);
	strcpy(party_info_owner, powner);

	/* Re-show party info */
	if (party_mode) {
		if (is_newer_than(&server_version, 4, 4, 7, 0, 0, 0)) {
			Term_erase(0, 18, 70);
			Term_erase(0, 21, 90);
			Term_putstr(0, 18, -1, TERM_WHITE, "Command: ");
			if (strlen(pname)) Term_putstr(0, 21, -1, TERM_WHITE, format("%s (%s, %s)", pname, pmembers, powner));
			else Term_putstr(0, 21, -1, TERM_SLATE, "(You are not in a party.)");

                        if (party_info_name[0]) Term_putstr(5, 4, -1, TERM_WHITE, "(\377G3\377w) Add a player to party");
                        else Term_putstr(5, 4, -1, TERM_WHITE, "(\377G3\377w) Add yourself to party");
		} else {
			Term_erase(0, 18, 70);
			Term_erase(0, 20, 90);
			Term_erase(0, 21, 20);
			Term_erase(0, 22, 50);
			Term_putstr(0, 18, -1, TERM_WHITE, "Command: ");
			Term_putstr(0, 20, -1, TERM_WHITE, pname);
			Term_putstr(0, 21, -1, TERM_WHITE, pmembers);
			Term_putstr(0, 22, -1, TERM_WHITE, powner);
		}
	}

	return 1;
}

int Receive_guild(void) {
	int n;
	char ch, gname[MAX_CHARS], gmembers[MAX_CHARS], gowner[MAX_CHARS];

	if ((n = Packet_scanf(&rbuf, "%c%s%s%s", &ch, gname, gmembers, gowner)) <= 0) return n;

	/* Copy info */
	strcpy(guild_info_name, gname);
	strcpy(guild_info_members, gmembers);
	strcpy(guild_info_owner, gowner);

	/* Re-show party info */
	if (party_mode) {
		Term_erase(0, 18, 70);
		Term_erase(0, 22, 90);
		Term_putstr(0, 18, -1, TERM_WHITE, "Command: ");
		if (strlen(gname)) Term_putstr(0, 22, -1, TERM_WHITE, format("%s (%s, %s)", gname, gmembers, gowner));
		else Term_putstr(0, 22, -1, TERM_SLATE, "(You are not in a guild.)");

                if (guild_info_name[0]) Term_putstr(5, 9, -1, TERM_WHITE, "(\377Ub\377w) Add a player to guild");
        	else Term_putstr(5, 9, -1, TERM_WHITE, "(\377Ub\377w) Add yourself to guild");
	}

	return 1;
}

int Receive_guild_config(void) {
	int i, n, master, guild_adders, ghp;
	char ch, dummy[MAX_CHARS];
	int x, y;
	int minlev_32b; /* 32 bits transmitted over the network gets converted to 16 bits */
	Term_locate(&x, &y);

	if ((n = Packet_scanf(&rbuf, "%c%d%d%d%d%d%d%d", &ch, &master, &guild.flags, &minlev_32b, &guild_adders, &guildhall_wx, &guildhall_wy, &ghp)) <= 0) return n;
	guild.minlev = minlev_32b;
	switch (ghp) {
	case 0: strcpy(guildhall_pos, "north-western"); break;
	case 4: strcpy(guildhall_pos, "northern"); break;
	case 8: strcpy(guildhall_pos, "north-eastern"); break;
	case 1: strcpy(guildhall_pos, "western"); break;
	case 5: strcpy(guildhall_pos, "central"); break;
	case 9: strcpy(guildhall_pos, "eastern"); break;
	case 2: strcpy(guildhall_pos, "south-western"); break;
	case 6: strcpy(guildhall_pos, "southern"); break;
	case 10: strcpy(guildhall_pos, "south-eastern"); break;
	default: strcpy(guildhall_pos, "unknown");
	}
	if (master) guild_master = TRUE;
	else guild_master = FALSE;

	for (i = 0; i < 5; i++) guild.adder[i][0] = 0;
	for (i = 0; i < guild_adders; i++) {
		if (i >= 5) {
			if ((n = Packet_scanf(&rbuf, "%s", dummy)) <= 0) return n;
		} else {
			if ((n = Packet_scanf(&rbuf, "%s", guild.adder[i])) <= 0) return n;
		}
	}

	/* Re-show guild config info -- in theory this can't happen if there is only 1 guild master, but w/e */
	if (guildcfg_mode) {
		int acnt = 0;
		char buf[(NAME_LEN + 1) * 5 + 1];
		if (guildhall_wx == -1) Term_putstr(5, 2, -1, TERM_SLATE, "The guild does not own a guild hall.");
		else if (guildhall_wx >= 0) Term_putstr(5, 2, -1, TERM_L_UMBER, format("The guild hall is located in the %s area of (%d,%d).",
		    guildhall_pos, guildhall_wx, guildhall_wy));
                Term_putstr(5, 4, -1, TERM_WHITE,  format("adders     : %s", guild.flags & GFLG_ALLOW_ADDERS ? "\377GYES" : "\377rno "));
                Term_putstr(5, 5, -1, TERM_L_WHITE,       "    Allows players designated via /guild_adder command to add others.");
                Term_putstr(5, 6, -1, TERM_WHITE,  format("autoreadd  : %s", guild.flags & GFLG_AUTO_READD ? "\377GYES" : "\377rno "));
                Term_putstr(5, 7, -1, TERM_L_WHITE,      "    If a guild mate ghost-dies then the next character he logs on with");
                Term_putstr(5, 8, -1, TERM_L_WHITE,      "    - if it is newly created - is automatically added to the guild again.");
                Term_putstr(5, 9, -1, TERM_WHITE, format("minlev     : \377%c%d   ", guild.minlev <= 1 ? 'w' : (guild.minlev <= 10 ? 'G' : (guild.minlev < 20 ? 'g' :
                    (guild.minlev < 30 ? 'y' : (guild.minlev < 40 ? 'o' : (guild.minlev <= 50 ? 'r' : 'v'))))), guild.minlev));
                Term_putstr(5, 10, -1, TERM_L_WHITE,      "    Minimum character level required to get added to the guild.");

                Term_erase(5, 11, 69);
                Term_erase(5, 12, 69);

                buf[0] = 0;
                for (i = 0; i < 5; i++) if (guild.adder[i][0] != '\0') {
                        sprintf(buf, "Adders are: ");
                        strcat(buf, guild.adder[i]);
                        acnt++;
                        for (i++; i < 5; i++) {
                                if (guild.adder[i][0] == '\0') continue;
                                if (acnt != 3) strcat(buf, ", ");
                                strcat(buf, guild.adder[i]);
                                acnt++;
                                if (acnt == 3) {
                            		Term_putstr(5, 11, -1, TERM_SLATE, buf);
                            		buf[0] = 0;
                            	}
                        }
                }
                Term_putstr(5 + (acnt <= 3 ? 0 : 12), acnt <= 3 ? 11 : 12, -1, TERM_SLATE, buf);
	}

	Term_gotoxy(x, y);
	return 1;
}

int Receive_skills(void) {
	int	n, i, bytes_read;
	s16b	tmp[12];
	char	ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return n;

	bytes_read = n;

	/* Read into skills info */
	for (i = 0; i < 12; i++) {
		if ((n = Packet_scanf(&rbuf, "%hd", &tmp[i])) <= 0) {
			/* Rollback the socket buffer */
			Sockbuf_rollback(&rbuf, bytes_read);

			/* Packet isn't complete, graceful failure */
			return n;
		}
		bytes_read += 2 * n;
	}

	/* Store */
	p_ptr->skill_thn = tmp[0];
	p_ptr->skill_thb = tmp[1];
	p_ptr->skill_sav = tmp[2];
	p_ptr->skill_stl = tmp[3];
	p_ptr->skill_fos = tmp[4];
	p_ptr->skill_srh = tmp[5];
	p_ptr->skill_dis = tmp[6];
	p_ptr->skill_dev = tmp[7];
	p_ptr->num_blow = tmp[8];
	p_ptr->num_fire = tmp[9];
	p_ptr->num_spell = tmp[10];
	p_ptr->see_infra = tmp[11];

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return 1;
}

int Receive_pause(void) {
	int n;
	char ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return n;

	/* Show the most recent changes to the screen */
	Term_fresh();

	/* Flush any pending keystrokes */
	Term_flush();

	/* Wait */
	inkey();

	/* Flush queue */
	Flush_queue();

	return 1;
}


int Receive_monster_health(void) {
	int n;
	char ch, num;
	byte attr;

	if ((n = Packet_scanf(&rbuf, "%c%c%c", &ch, &num, &attr)) <= 0) return n;

	if (screen_icky) Term_switch(0);

	/* Draw the health bar */
	health_redraw(num, attr);

	if (screen_icky) Term_switch(0);

	return 1;
}

int Receive_chardump(void) {
	char	ch;
	int	n;
	char tmp[160], type[MAX_CHARS];

	time_t ct = time(NULL);
	struct tm* ctl = localtime(&ct);

	/* Hack: make sure message log contains all the latest messages.
	   Added this for when someone gets killed by massive amount of
	   concurrent hits (Z packs), his client might terminate before
	   all incoming damage messages were actually displayed, depending
	   on his latency probably.  - C. Blue */
#if 1
	window_stuff();
#else /* todo if enabling this: make fix_message() non static */
	if (p_ptr->window & (PW_MESSAGE | PW_CHAT | PW_MSGNOCHAT)) {
		p_ptr->window &= (~(PW_MESSAGE | PW_CHAT | PW_MSGNOCHAT));
		fix_message();
	}
#endif

	/* assume death dump at first */
	strcpy(type, "-death");

	if (is_newer_than(&server_version, 4, 4, 2, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%s", &ch, &type)) <= 0) return n;
	} else
	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return n;

	/* Access the main view */
        if (screen_icky) Term_switch(0);

	/* additionally do a screenshot of the scene */
	silent_dump = TRUE;
	xhtml_screenshot(format("%s%s_%04d-%02d-%02d_%02d.%02d.%02d_screenshot", cname, type,
	    1900 + ctl->tm_year, ctl->tm_mon + 1, ctl->tm_mday,
	    ctl->tm_hour, ctl->tm_min, ctl->tm_sec));

	if (screen_icky) Term_switch(0);

	strnfmt(tmp, 160, "%s%s_%04d-%02d-%02d_%02d.%02d.%02d.txt", cname, type,
	    1900 + ctl->tm_year, ctl->tm_mon + 1, ctl->tm_mday,
	    ctl->tm_hour, ctl->tm_min, ctl->tm_sec);
	file_character(tmp, FALSE);

	return 1;
}

/* Some simple paging, especially useful to notify ghosts
   who are afk while being rescued :) - C. Blue */
int Receive_beep(void) {
	char	ch;
	int	n;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return n;
	if (!c_cfg.allow_paging) return 1;
	return page();
}
int Receive_warning_beep(void) {
	char	ch;
	int	n;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return n;
	return warning_page();
}

int Receive_AFK(void) {
	int	n;
	char	ch;
	byte	afk;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &afk)) <= 0) return n;

	if (screen_icky) Term_switch(0);

	prt_AFK(afk);

	/* HACK - Also draw world coordinates because they're on the same row - mikaelh */
	prt_depth(p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, depth_town,
	          depth_colour, depth_colour_sector, depth_name);

	if (screen_icky) Term_switch(0);

	return 1;
}

int Receive_encumberment(void) {
	int	n;
	char	ch;
        byte cumber_armor;      /* Encumbering armor (tohit/sneakiness) */
        byte awkward_armor;     /* Mana draining armor */
        byte cumber_glove;      /* Mana draining gloves */
        byte heavy_wield;       /* Heavy weapon */
        byte heavy_shield;      /* Heavy shield */
        byte heavy_shoot;       /* Heavy shooter */
        byte icky_wield;        /* Icky weapon */
        byte awkward_wield;     /* shield and COULD_2H weapon */
        byte easy_wield;        /* Using a 1-h weapon which is MAY2H with both hands */
        byte cumber_weight;     /* Full weight. FA from MA will be lost if overloaded */
        byte monk_heavyarmor;   /* Reduced MA power? */
        byte rogue_heavyarmor;  /* Missing roguish-abilities' effects? */
        byte awkward_shoot;     /* using ranged weapon while having a shield on the arm */

	if (is_newer_than(&server_version, 4, 4, 2, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c", &ch, &cumber_armor, &awkward_armor, &cumber_glove, &heavy_wield, &heavy_shield, &heavy_shoot,
		    &icky_wield, &awkward_wield, &easy_wield, &cumber_weight, &monk_heavyarmor, &rogue_heavyarmor, &awkward_shoot)) <= 0)
			return n;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c%c%c%c%c%c%c%c%c", &ch, &cumber_armor, &awkward_armor, &cumber_glove, &heavy_wield, &heavy_shield, &heavy_shoot,
		    &icky_wield, &awkward_wield, &easy_wield, &cumber_weight, &monk_heavyarmor, &awkward_shoot)) <= 0)
			return n;
		rogue_heavyarmor = 0;
	}

	if (screen_icky) Term_switch(0);

	prt_encumberment(cumber_armor, awkward_armor, cumber_glove, heavy_wield, heavy_shield, heavy_shoot,
	    icky_wield, awkward_wield, easy_wield, cumber_weight, monk_heavyarmor, rogue_heavyarmor, awkward_shoot);

	if (screen_icky) Term_switch(0);

	return 1;
}

int Receive_extra_status(void) {
	int	n;
	char	ch;
	char    status[MAX_CHARS];

	if ((n = Packet_scanf(&rbuf, "%c%s", &ch, &status)) <= 0) return n;

	if (screen_icky) Term_switch(0);
	prt_extra_status(status);
	if (screen_icky) Term_switch(0);
	return 1;
}

int Receive_keepalive(void) {
	int n;
	char ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return n;
	return 1;
}

int Receive_ping(void) {
	int n, id, tim, utim, tim_now, utim_now, rtt, index;
	char ch, pong;
	char buf[MSG_LEN];
	struct timeval tv;

	if ((n = Packet_scanf(&rbuf, "%c%c%d%d%d%S", &ch, &pong, &id, &tim, &utim, &buf)) <= 0) return n;

	if (pong) {
#ifdef WINDOWS
		/* Use the multimedia timer function */
		DWORD systime_ms = timeGetTime();
		tv.tv_sec = systime_ms / 1000;
		tv.tv_usec = (systime_ms % 1000) * 1000;
#else
		gettimeofday(&tv, NULL);
#endif

		tim_now = tv.tv_sec;
		utim_now = tv.tv_usec;

		rtt = (tim_now - tim) * 1000 + (utim_now - utim) / 1000;
		if (rtt == 0) rtt = 1;

		index = ping_id - id;
		if (index >= 0 && index < 60) {
			ping_times[index] = rtt;

			/* Also determine our average ping over the last 10 minutes or so */

			/* Hack to catch ongoing parallel downloads and stuff:
			   Reset average ping if we're suddenly getting much lower pings.
			   Ignore new pings that are suddenly much higher than our average ping so far. */
			if (rtt < (ping_avg * 7 / 10)) { //reset on too low pings
				ping_avg = rtt;
				ping_avg_cnt = 1;
			}
			if (rtt <= (ping_avg * 13) / 10 || ping_avg_cnt < 10) { //ignore too high pings
				if (ping_avg_cnt == 600) ping_avg = (ping_avg * 599 + rtt) / 600;
				else {
					ping_avg = (ping_avg * ping_avg_cnt + rtt) / (ping_avg_cnt + 1);
					ping_avg_cnt++;
				}
			}
		}

		update_lagometer();

		/* somewhat hacky, see prt_lagometer().
		   discard older packets, since 'pseudo
		   packet loss' were assumed for them already,
		   which doesn't hurt since it shows same as
		   450+ms lag in the gui.
		   Also, set an arbitrary value here which is
		   the minimum duration that bad pings that were
		   followed by good pings are displayed to the eyes
		   of the user, preventing them from being updated
		   too quickly to see. [600 ms]
		   NOTE: this depends on 'ticks'-related code. */
		if (index == 0) {
			if (ping_times[1] != -1 &&
			    ping_times[0] + 1000 - ping_times[1] >= 600) {
				prt_lagometer(rtt);
			} else if (ping_times[1] != -1) {
				/* pseudo-packet loss */
				prt_lagometer(9999);
			} else {
				/* prolonged display of previous bad ping
				   so the user has easier time to notice it */
				prt_lagometer(ping_times[1]);
			}
		}
	} else {
		/* reply */

		pong = 1;

		if ((n = Packet_printf(&wbuf, "%c%c%d%d%d%S", ch, pong, id, tim, utim, buf)) <= 0) return n;
	}

	return 1;
}

/* client-side weather, server-controlled - C. Blue
   Transmitted parameters:
   weather_type = -1, 0, 1, 2: stop, none, rain, snow
   weather_type +10 * n: pre-generate n steps
*/
int Receive_weather(void) {
	int	n, i, clouds;
	char	ch;
	int	wg, wt, ww, wi, ws, wx, wy;
	int	cnum, cidx, cx1, cy1, cx2, cy2, cd, cxm, cym;
	char *stored_sbuf_ptr = rbuf.ptr;

	/* base packet: weather + number of clouds */
	if ((n = Packet_scanf(&rbuf, "%c%d%d%d%d%d%d%d%d",
	    &ch, &wt, &ww, &wg, &wi, &ws, &wx, &wy,
	    &cnum)) <= 0) return n;

	/* fix limit */
	if (cnum >= 0) clouds = cnum;
	else clouds = 10;

	/* extended: read clouds */
	for (i = 0; i < clouds; i++) {
		/* hack: cnum == -1 revokes all clouds without further parms */
		if (cnum == -1) {
			cloud_x1[i] = -9999;
			continue;
		}

		/* proceed normally */
		if ((n = Packet_scanf(&rbuf, "%d%d%d%d%d%d%d%d",
		    &cidx, &cx1, &cy1, &cx2, &cy2, &cd, &cxm, &cym)) <= 0) {
			if (n == 0) goto rollback;
			return n;
		}

		/* potential forward-compatibility hack:
		   ignore if too many clouds are sent */
		if (cidx >= 10) continue;

		/* update clouds */
		cloud_x1[cidx] = cx1;
		cloud_y1[cidx] = cy1;
		cloud_x2[cidx] = cx2;
		cloud_y2[cidx] = cy2;
		cloud_dsum[cidx] = cd;
		cloud_xm100[cidx] = cxm;
		cloud_ym100[cidx] = cym;
		cloud_xfrac[cidx] = 0;
		cloud_yfrac[cidx] = 0;
	}

	/* fix values */
	wx += PANEL_X;
	wy += PANEL_Y;

	/* did we change view panel? that would imply that our map
	   info was updated and over-drawn freshly from server */
	if (wx != weather_panel_x || wy != weather_panel_y)
		weather_panel_changed = TRUE;

	/* set current screen panel */
	weather_panel_x = wx;
	weather_panel_y = wy;

	/* update weather */
	weather_type = wt;
	weather_wind = ww;
	weather_gen_speed = wg;
	weather_intensity = wi;
	/* for mix of rain/snow in same sector:
	   keep old rain/snow particles at their remembered speed */
	if (weather_type != -1) {
		if (weather_type % 10 == 2) weather_speed_snow = ws;
		else if (weather_type % 10 == 1) weather_speed_rain = ws;
	}

	if (weather_type == -1) {
		wind_noticable = FALSE;
#ifdef USE_SOUND_2010
		/* Play overlay sound (if enabled) */
		if (use_sound) sound_weather(-2); //stop
//puts(format("read weather: type %d, wind %d.", weather_type, weather_wind));
#endif
	} else if (weather_type % 10 == 0) {
		wind_noticable = FALSE;
#ifdef USE_SOUND_2010
		/* Play overlay sound (if enabled) */
		if (use_sound) sound_weather(-1); //fade out
#endif
	}

	/* hack: insta-erase all weather */
	if (weather_type == -1 ||
	    /* same for pregenerate command, it must include the "-1" effect: */
	    (weather_type % 10000) / 10 != 0) {
		/* if view panel was updated anyway, no need to restore */
		if (!weather_panel_changed) {
			/* restore tiles on display that were overwritten by weather */
			if (screen_icky) Term_switch(0);
			for (i = 0; i < weather_elements; i++) {
				/* only for elements within visible panel screen area */
				if (weather_element_x[i] >= weather_panel_x &&
				    weather_element_x[i] < weather_panel_x + screen_wid &&
			    	    weather_element_y[i] >= weather_panel_y &&
			            weather_element_y[i] < weather_panel_y + screen_hgt) {
					/* restore original grid content */
	                                Term_draw(PANEL_X + weather_element_x[i] - weather_panel_x,
        	                            PANEL_Y + weather_element_y[i] - weather_panel_y,
                                            panel_map_a[weather_element_x[i] - weather_panel_x][weather_element_y[i] - weather_panel_y],
                                            panel_map_c[weather_element_x[i] - weather_panel_x][weather_element_y[i] - weather_panel_y]);
                                }
			}
			if (screen_icky) Term_switch(0);
		}

		/* terminate weather */
		weather_elements = 0;
		if (weather_type == -1) weather_type = 0;
	}

	return 1;

	/* Rollback the socket buffer in case the packet isn't complete */
	rollback:
	rbuf.ptr = stored_sbuf_ptr;
	return 0;
}

int Receive_inventory_revision(void) {
	int n;
	char ch;
	int revision;

	if ((n = Packet_scanf(&rbuf, "%c%d", &ch, &revision)) <= 0) return n;

	p_ptr->inventory_revision = revision;
	Send_inventory_revision(revision);

#if 0 /* moved to Receive_inven...() - cleaner and works much better (ID and *ID*) */
	/* AUTOINSCRIBE - moved to Receive_inventory_revision() */
	for (v = 0; v < INVEN_PACK; v++)
		apply_auto_inscriptions(v, FALSE);
#endif

	return 1;
}

/* Apply client-side auto-inscriptions - C. Blue
   'force': overwrite existing non-trivial inscription. */
void apply_auto_inscriptions(int slot, bool force) {
	int i;
	char *ex, ex_buf[ONAME_LEN];
	char *ex2, ex_buf2[ONAME_LEN];
	char *match, tag_buf[ONAME_LEN];
	bool auto_inscribe, found;

	/* skip empty items */
	if (!strlen(inventory_name[slot])) return;

	/* haaaaack: check for existing inscription! */
	auto_inscribe = FALSE;
	/* look for 1st '{' which must be level requirements on ANY item */
	ex = strstr(inventory_name[slot], "{");
	if (ex == NULL) return; /* paranoia - should always be FALSE */
	strcpy(ex_buf, ex + 1);
	/* look for 2nd '{' which MUST be an inscription */
	ex = strstr(ex_buf, "{");

	/* Add "fake-artifact" inscriptions using '#' */
	if (inventory_inscription[slot]) {
		if (ex == NULL) {
			strcat(ex_buf, "{#"); /* create a 'real' inscription from this fake-name inscription */
			/* hack 'ex' to make this special inscription known */
			ex = ex_buf + strlen(ex_buf) - 2;
			/* add the fake-name inscription */
			i = strlen(ex_buf);
			strncat(ex_buf, inventory_name[slot] + inventory_inscription[slot], inventory_inscription_len[slot]);
			ex_buf[i + inventory_inscription_len[slot]] = '\0'; /* terminate string after strncat() */
			strcat(ex_buf, "}");
		} else {
			ex_buf[strlen(ex_buf) - 1] = '\0'; /* cut off final '}' */
			strcat(ex_buf, "#"); /* add the fake-name inscription */
			i = strlen(ex_buf);
			strncat(ex_buf, inventory_name[slot] + inventory_inscription[slot], inventory_inscription_len[slot]);
			ex_buf[i + inventory_inscription_len[slot]] = '\0'; /* terminate string after strncat() */
			strcat(ex_buf, "}"); /* restore final '}' */
		}
	}

	/* no inscription? inscribe it then automatically */
	if (ex == NULL || force) auto_inscribe = TRUE;
	/* check whether inscription is just a discount/stolen tag */
	else if (strstr(ex, "% off}") ||
	    !strcmp(ex, "{cursed}") ||	/* for picking up items that were already identified on the floor */
	    !strcmp(ex, "{on sale}") ||
	    !strcmp(ex, "{stolen}")) {
		/* if so, auto-inscribe it instead */
		auto_inscribe = TRUE;
	}
 #if 0 /* is '!' UNavailable? */
	/* already has a real inscription? -> can't auto-inscribe */
	if (!auto_inscribe) return;
 #else
	/* save for checking for already existing target inscription */
	if (ex && strlen(ex) > 2) {
		strncpy(tag_buf, ex + 1, strlen(ex) - 2);
		tag_buf[strlen(ex) - 2] = '\0'; /* terminate */
	}
	else strcpy(tag_buf, ""); /* initialise as empty */
 #endif

	/* look for matching auto-inscription */
	for (i = 0; i < MAX_AUTO_INSCRIPTIONS; i++) {
		match = auto_inscription_match[i];
		/* skip empty auto-inscriptions */
		if (!strlen(match)) continue;
 #if 0 /* disallow empty inscription? */
		if (!strlen(auto_inscription_tag[i])) continue;
 #endif

 #if 1 /* is '!' available? */
		/* if item already has an inscription, only allow to overwrite it
		    if auto-inscription begins with '!', which stands for 'always overwrite' */
		if (!auto_inscribe) {
			if (match[0] != '!') continue;
			else match++;
			/* already carrying this very inscription? don't need to inscribe it AGAIN then */
			if (!strcmp(auto_inscription_tag[i], tag_buf)) return;
		} else if (match[0] == '!') match++;
 #endif

		/* found a matching inscription? */
 #if 0 /* no '?' wildcard allowed */
		if (strstr(inventory_name[slot], match)) break;
 #else /* '?' wildcard allowed: a random number (including 0) of random chars */
		/* prepare */
		strcpy(ex_buf, match);
		ex2 = inventory_name[slot];
		found = FALSE;

		do {
			ex = strstr(ex_buf, "?");
			if (ex == NULL) {
				if (strstr(ex2, ex_buf)) found = TRUE;
				break;
			} else {
				/* get partial string up to before the '?' */
				strncpy(ex_buf2, ex_buf, ex - ex_buf);
				ex_buf2[ex - ex_buf] = '\0';
				/* test partial string for match */
				ex2 = strstr(ex2, ex_buf2);
				if (ex2 == NULL) break; /* no match! */
				/* this partial string matched, discard and continue with next part */
				/* advance searching position in the item name */
				ex2 += strlen(ex_buf2);
				/* get next part of search string */
				strcpy(ex_buf, ex + 1);
				/* no more search string left? exit */
				if (!strlen(ex_buf)) break;
				/* no more item name left although search string is finished? exit with negative result */
				if (!strlen(ex2)) {
					found = FALSE;
					break;
				}
			}
		} while (TRUE);
		if (found) break;
 #endif
	}
	/* no match found? */
	if (i == MAX_AUTO_INSCRIPTIONS) return;

	/* send the new inscription */
	/* security hack: avoid infinite looping */
	if (strstr(auto_inscription_tag[i], "% off") == NULL &&
	    strcmp(auto_inscription_tag[i], "cursed") &&
	    strcmp(auto_inscription_tag[i], "on sale") &&
	    strcmp(auto_inscription_tag[i], "stolen"))
		Send_inscribe(slot, auto_inscription_tag[i]);
}

int Receive_account_info(void) {
	int n;
	char ch;

	if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &acc_flags)) <= 0)
		return n;

	acc_got_info = TRUE;
	display_account_information();

	return 1;
}

/* Request keypress (1 char) */
int Receive_request_key(void) {
	int n, id;
	char ch, prompt[MAX_CHARS], buf;

	if ((n = Packet_scanf(&rbuf, "%c%d%s", &ch, &id, prompt)) <= 0) return n;

	request_pending = TRUE;
	if (get_com(prompt, &buf)) Send_request_key(id, buf);
	else Send_request_key(id, 0);
	request_pending = FALSE;
	return 1;
}
/* Request number */
int Receive_request_num(void) {
	int n, id, max;
	char ch, prompt[MAX_CHARS];

	if ((n = Packet_scanf(&rbuf, "%c%d%s%d", &ch, &id, prompt, &max)) <= 0) return n;

	request_pending = TRUE;
	Send_request_num(id, c_get_quantity(prompt, max));
	request_pending = FALSE;
	return 1;
}
/* Request string (1 line) */
int Receive_request_str(void) {
	int n, id;
	char ch, prompt[MAX_CHARS], buf[MAX_CHARS_WIDE];

	if ((n = Packet_scanf(&rbuf, "%c%d%s%s", &ch, &id, prompt, buf)) <= 0) return n;

	request_pending = TRUE;
	if (get_string(prompt, buf, MAX_CHARS_WIDE - 1)) Send_request_str(id, buf);
	else Send_request_str(id, "\e");
	request_pending = FALSE;
	return 1;
}
/* Request confirmation (y/n) */
int Receive_request_cfr(void) {
	int n, id;
	char ch, prompt[MAX_CHARS];
	char default_choice = FALSE;

	if (is_newer_than(&server_version, 4, 5, 6, 0, 0, 1)) {
		char dy;
		if ((n = Packet_scanf(&rbuf, "%c%d%s%c", &ch, &id, prompt, &dy)) <= 0) return n;
		default_choice = dy;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%d%s", &ch, &id, prompt)) <= 0) return n;
	}

	request_pending = TRUE;
	Send_request_cfr(id, get_check3(prompt, default_choice));
	request_pending = FALSE;
	return 1;
}
/* Cancel pending input request */
int Receive_request_abort(void) {
	int n;
	char ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return n;
	if (request_pending) request_abort = TRUE;
	return 1;
}

int Receive_martyr(void) {
	int	n;
	char	ch, martyr;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &martyr)) <= 0) return n;

	p_ptr->martyr = (s16b)martyr;
	c_msg_print(format("martyr %d", (s16b)martyr));

	return 1;
}

/* Receive inventory index of the last item we picked up */
int Receive_item_newest(void) {
	int	n;
	char	ch, item;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &item)) <= 0) return n;
	item_newest = (int)item;

	return 1;
}

/* Receive a confirmation for a particular PKT_.. command we issued earlier to the server */
int Receive_confirm(void) {
	int	n;
	char	ch, ch_confirmed;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &ch_confirmed)) <= 0) return n;
	command_confirmed = ch_confirmed;

	return 1;
}

//not implemented
int Receive_keypress(void) {
	int	n;
	char	ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return n;
	return 1;
}


int Send_search(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_SEARCH)) <= 0) return n;
	return 1;
}

int Send_walk(int dir) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%c", PKT_WALK, dir)) <= 0) return n;
	return 1;
}

int Send_run(int dir) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%c", PKT_RUN, dir)) <= 0) return n;
	return 1;
}

int Send_drop(int item, int amt) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_DROP, item, amt)) <= 0) return n;
	return 1;
}

int Send_drop_gold(s32b amt) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%d", PKT_DROP_GOLD, amt)) <= 0) return n;
	return 1;
}

int Send_tunnel(int dir) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%c", PKT_TUNNEL, dir)) <= 0) return n;
	return 1;
}

int Send_stay(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_STAND)) <= 0) return n;
	return 1;
}
int Send_stay_one(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_STAND_ONE)) <= 0) return n;
	return 1;
}

static int Send_keepalive(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_KEEPALIVE)) <= 0) return n;
	return 1;
}

int Send_toggle_search(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_SEARCH_MODE)) <= 0) return n;
	return 1;
}

int Send_rest(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_REST)) <= 0) return n;
	return 1;
}

int Send_go_up(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_GO_UP)) <= 0) return n;
	return 1;
}

int Send_go_down(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_GO_DOWN)) <= 0) return n;
	return 1;
}

int Send_open(int dir) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%c", PKT_OPEN, dir)) <= 0) return n;
	return 1;
}

int Send_close(int dir) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%c", PKT_CLOSE, dir)) <= 0) return n;
	return 1;
}

int Send_bash(int dir) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%c", PKT_BASH, dir)) <= 0) return n;
	return 1;
}

int Send_disarm(int dir) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%c", PKT_DISARM, dir)) <= 0) return n;
	return 1;
}

int Send_wield(int item) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_WIELD, item)) <= 0) return n;
	return 1;
}

int Send_observe(int item) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_OBSERVE, item)) <= 0) return n;
	return 1;
}

int Send_take_off(int item) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_TAKE_OFF, item)) <= 0) return n;
	return 1;
}

int Send_take_off_amt(int item, int amt) {
	int n;
	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_TAKE_OFF_AMT, item, amt)) <= 0) return n;
	return 1;
}

int Send_destroy(int item, int amt) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_DESTROY, item, amt)) <= 0) return n;

	return 1;
}

int Send_inscribe(int item, cptr buf) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd%s", PKT_INSCRIBE, item, buf)) <= 0) return n;
	return 1;
}

int Send_uninscribe(int item) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_UNINSCRIBE, item)) <= 0) return n;
	return 1;
}

int Send_autoinscribe(int item) {
	int	n;
	if (!is_newer_than(&server_version, 4, 5, 5, 0, 0, 0)) return 1;
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_AUTOINSCRIBE, item)) <= 0) return n;
	return 1;
}

int Send_steal(int dir) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%c", PKT_STEAL, dir)) <= 0) return n;
	return 1;
}

int Send_quaff(int item) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_QUAFF, item)) <= 0) return n;
	return 1;
}

int Send_read(int item) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_READ, item)) <= 0) return n;
	return 1;
}

int Send_aim(int item, int dir) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd%c", PKT_AIM_WAND, item, dir)) <= 0) return n;
	return 1;
}

int Send_use(int item) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_USE, item)) <= 0) return n;
	return 1;
}

int Send_zap(int item) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_ZAP, item)) <= 0) return n;
	return 1;
}

int Send_zap_dir(int item, int dir) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd%c", PKT_ZAP_DIR, item, dir)) <= 0) return n;
	return 1;
}

int Send_fill(int item) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_FILL, item)) <= 0) return n;
	return 1;
}

int Send_eat(int item) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_EAT, item)) <= 0) return n;
	return 1;
}

int Send_activate(int item) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_ACTIVATE, item)) <= 0) return n;
	return 1;
}

int Send_activate_dir(int item, int dir) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd%c", PKT_ACTIVATE_DIR, item, dir)) <= 0) return n;
	return 1;
}

int Send_target(int dir) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_TARGET, dir)) <= 0) return n;
	return 1;
}


int Send_target_friendly(int dir) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_TARGET_FRIENDLY, dir)) <= 0) return n;
	return 1;
}


int Send_look(int dir) {
	int	n;

	if (is_newer_than(&server_version, 4, 4, 9, 3, 0, 0)) {
		if ((n = Packet_printf(&wbuf, "%c%hd", PKT_LOOK, dir)) <= 0) return n;
	} else {
		if ((n = Packet_printf(&wbuf, "%c%c", PKT_LOOK, dir)) <= 0) return n;
	}

	return 1;
}

int Send_msg(cptr message) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%S", PKT_MESSAGE, message)) <= 0) return n;
	return 1;
}

int Send_fire(int dir) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%c", PKT_FIRE, dir)) <= 0) return n;
	return 1;
}

int Send_throw(int item, int dir) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%c%hd", PKT_THROW, dir, item)) <= 0) return n;
	return 1;
}

int Send_item(int item) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_ITEM, item)) <= 0) return n;
	return 1;
}

/* for DISCRETE_SPELL_SYSTEM: DSS_EXPANDED_SCROLLS */
int Send_spell(int item, int spell) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_SPELL, item, spell)) <= 0) return n;
	return 1;
}

int Send_activate_skill(int mkey, int book, int spell, int dir, int item, int aux) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%c%hd%hd%c%hd%hd", PKT_ACTIVATE_SKILL,
					mkey, book, spell, dir, item, aux)) <= 0)
		return n;
	return 1;
}

int Send_pray(int book, int spell) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_PRAY, book, spell)) <= 0) return n;
	return 1;
}

#if 0 /* instead, Send_telekinesis is used */
int Send_mind() {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_MIND)) <= 0) return n;
	return 1;
}
#endif

int Send_ghost(int ability) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_GHOST, ability)) <= 0) return n;
	return 1;
}

int Send_map(char mode){
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%c", PKT_MAP, mode)) <= 0) return n;
	return 1;
}

int Send_locate(int dir) {
	int n;
	if ((n = Packet_printf(&wbuf, "%c%c", PKT_LOCATE, dir)) <= 0) return n;
	return 1;
}

int Send_store_command(int action, int item, int item2, int amt, int gold) {
	int 	n;
	if ((n = Packet_printf(&wbuf, "%c%hd%hd%hd%hd%d", PKT_STORE_CMD, action, item, item2, amt, gold)) <= 0) return n;
	return 1;
}

int Send_store_examine(int item) {
	int 	n;
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_STORE_EXAMINE, item)) <= 0) return n;
	return 1;
}

int Send_store_purchase(int item, int amt) {
	int 	n;
	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_PURCHASE, item, amt)) <= 0) return n;
	return 1;
}

int Send_store_sell(int item, int amt) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_SELL, item, amt)) <= 0) return n;
	return 1;
}

int Send_store_leave(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_STORE_LEAVE)) <= 0) return n;
	return 1;
}

int Send_store_confirm(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_STORE_CONFIRM)) <= 0) return n;
	return 1;
}

int Send_redraw(char mode) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%c", PKT_REDRAW, mode)) <= 0) return n;
	return 1;
}

int Send_clear_buffer(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_CLEAR_BUFFER)) <= 0) return n;
	return 1;
}

int Send_clear_actions(void) {
	int n;
	if (is_newer_than(&server_version, 4, 4, 5, 10, 0, 0)) {
		if ((n = Packet_printf(&wbuf, "%c", PKT_CLEAR_ACTIONS)) <= 0) return n;
	}
	return 1;
}

int Send_special_line(int type, s32b line) {
	int	n;

	if (is_newer_than(&server_version, 4, 4, 7, 0, 0, 0)) {
		if ((n = Packet_printf(&wbuf, "%c%c%d", PKT_SPECIAL_LINE, type, line)) <= 0) return n;
	} else {
		if ((n = Packet_printf(&wbuf, "%c%c%hd", PKT_SPECIAL_LINE, type, line)) <= 0) return n;
	}

	return 1;
}

int Send_skill_mod(int i) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%d", PKT_SKILL_MOD, i)) <= 0) return n;
	return 1;
}

int Send_skill_dev(int i, bool dev) {
	int	n;
	if (is_newer_than(&server_version, 4, 4, 8, 2, 0, 0)) {
		if ((n = Packet_printf(&wbuf, "%c%d%c", PKT_SKILL_DEV, i, dev)) <= 0) return n;
	}
	return 1;
}

int Send_party(s16b command, cptr buf) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd%s", PKT_PARTY, command, buf)) <= 0) return n;
	return 1;
}

int Send_guild(s16b command, cptr buf) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd%s", PKT_GUILD, command, buf)) <= 0) return n;
	return 1;
}

int Send_guild_config(s16b command, u32b flags, cptr buf) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%d%d%s", PKT_GUILD_CFG, command, flags, buf)) <= 0) return n;
	return 1;
}

int Send_purchase_house(int dir) {
	int n;
	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_PURCHASE, dir, 0)) <= 0) return n;
	return 1;
}

int Send_suicide(void) {
	int n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_SUICIDE)) <= 0) return n;

	/* Newer servers require couple of extra bytes for suicide - mikaelh */
	if (is_newer_than(&server_version, 4, 4, 2, 3, 0, 0)) {
		if ((n = Packet_printf(&wbuf, "%c%c", 1, 4)) <= 0)
			return n;
	}

	return 1;
}

int Send_options(void) {
	int i, n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_OPTIONS)) <= 0) return n;
	/* Send each option */
	if (is_newer_than(&server_version, 4, 5, 8, 1, 0, 1)) {
		for (i = 0; i < OPT_MAX; i++)
			Packet_printf(&wbuf, "%c", Client_setup.options[i]);
	} else if (is_newer_than(&server_version, 4, 5, 5, 0, 0, 0)) {
		for (i = 0; i < OPT_MAX_COMPAT; i++)
			Packet_printf(&wbuf, "%c", Client_setup.options[i]);
	} else {
		for (i = 0; i < OPT_MAX_OLD; i++)
			Packet_printf(&wbuf, "%c", Client_setup.options[i]);
	}
	return 1;
}

int Send_screen_dimensions(void) {
	int n;
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_SCREEN_DIM, screen_wid, screen_hgt)) <= 0) return n;
	return 1;
}

int Send_admin_house(int dir, cptr buf){
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd%s", PKT_HOUSE, dir, buf)) <= 0) return n;
	return 1;
}

int Send_master(s16b command, cptr buf) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd%s", PKT_MASTER, command, buf)) <= 0) return n;
	return 1;
}

int Send_King(byte type) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%c", PKT_KING, type)) <= 0) return n;
	return 1;
}

int Send_spike(int dir){
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%c", PKT_SPIKE, dir)) <= 0) return n;
	return 1;
}

int Send_raw_key(int key) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%c", PKT_RAW_KEY, key)) <= 0) return n;
	return 1;
}

int Send_ping(void) {
	int i, n, tim, utim;
	char pong = 0;
	char *buf = "";
	struct timeval tv;

#ifdef WINDOWS
	/* Use the multimedia timer function */
	DWORD systime_ms = timeGetTime();
	tv.tv_sec = systime_ms / 1000;
	tv.tv_usec = (systime_ms % 1000) * 1000;
#else
	gettimeofday(&tv, NULL);
#endif

	/* hack: pseudo-packet loss, see prt_lagometer() */
	if (ping_times[0] == -1) prt_lagometer(9999);

	tim = tv.tv_sec;
	utim = tv.tv_usec;

	if ((n = Packet_printf(&wbuf, "%c%c%d%d%d%S", PKT_PING, pong, ++ping_id, tim, utim, buf)) <= 0)
		return n;

	/* Shift ping_times */
	for (i = 59; i > 0; i--)
		ping_times[i] = ping_times[i - 1];
	ping_times[i] = -1;

	return 1;
}

int Send_account_info(void) {
	int n;
	if (!is_newer_than(&server_version, 4, 4, 2, 2, 0, 0)) return 1;
	if ((n = Packet_printf(&wbuf, "%c", PKT_ACCOUNT_INFO)) <= 0) return n;
	return 1;
}

int Send_change_password(char *old_pass, char *new_pass) {
	int n;
	if (!is_newer_than(&server_version, 4, 4, 2, 2, 0, 0)) return 1;
	if ((n = Packet_printf(&wbuf, "%c%s%s", PKT_CHANGE_PASSWORD, old_pass, new_pass)) <= 0) return n;
	return 1;
}

/*
 * Update the current time, which is stored in 100 ms "ticks".
 * I hope that Windows systems have gettimeofday on them by default.
 * If not there should hopefully be some simmilar efficient call with the same
 * functionality.
 * I hope this doesn't prove to be a bottleneck on some systems.  On my linux system
 * calling gettimeofday seems to be very very fast.
 *
 * Some Windows implementations (e.g. MinGW) of gettimeofday have a low resolution.
 * timeGetTime should provide a 1 ms resolution.
 *  - mikaelh
 */
void update_ticks() {
	struct timeval cur_time;
	int newticks;

#ifdef WINDOWS
	/* Use the multimedia timer function */
	DWORD systime_ms = timeGetTime();
	cur_time.tv_sec = systime_ms / 1000;
	cur_time.tv_usec = (systime_ms % 1000) * 1000;
#else
	gettimeofday(&cur_time, NULL);
#endif

	/* Set the new ticks to the old ticks rounded down to the number of seconds. */
	newticks = ticks - (ticks % 10);

	/* Find the new least significant digit of the ticks */
	newticks += cur_time.tv_usec / 100000;
	ticks10 = (cur_time.tv_usec / 10000) % 10;

	/* Assume that it has not been more than one second since this function was last called */
	if (newticks < ticks) newticks += 10;
	ticks = newticks;
}

/* Write a keepalive packet to the output queue if it has been two seconds
 * since we last sent anything.
 * Note that if the loop that is calling this function doesn't flush the
 * network output before calling this function again very bad things could
 * happen, such as an overflow of our send queue.
 */
void do_keepalive() {
	/*
	 * Check to see if it has been 2 seconds since we last sent anything.  Assume
	 * that each game turn lasts 100 ms.
	 */
	if ((ticks - last_send_anything) >= 20)
		Send_keepalive();
}

#define FL_SPEED 1

void do_flicker() {
	static int flticks = 0;

	if (ticks-flticks < FL_SPEED) return;
	flicker();
	flticks = ticks;
}

void do_mail() {
#ifdef CHECK_MAIL
 #ifdef SET_UID
	static int mailticks = 0;
	static struct timespec lm;
	char mpath[160],buffer[160];

	int uid;
	struct passwd *pw;

  #ifdef NETBSD
	strcpy(mpath,"/var/mail/");
  #else
	strcpy(mpath,"/var/spool/mail/");
  #endif

	uid = getuid();
	pw = getpwuid(uid);
	if(pw == (struct passwd*)NULL)
		return;
	strcat(mpath, pw->pw_name);

	if(ticks-mailticks >= 300){	/* testing - too fast */
		struct stat inf;
		if(!stat(mpath,&inf)){
  #ifndef _POSIX_C_SOURCE
			if (inf.st_size != 0) {
				if (inf.st_mtimespec.tv_sec > lm.tv_sec || (inf.st_mtimespec.tv_sec == lm.tv_sec && inf.st_mtimespec.tv_nsec > lm.tv_nsec)) {
					lm.tv_sec = inf.st_mtimespec.tv_sec;
					lm.tv_nsec = inf.st_mtimespec.tv_nsec;
					sprintf(buffer,"\377yYou have new mail in %s.", mpath);
					c_msg_print(buffer);
				}
			}
  #else
			if (inf.st_size != 0) {
				if (inf.st_mtime > lm.tv_sec || (inf.st_mtime == lm.tv_sec && inf.st_mtimensec > lm.tv_nsec)) {
					lm.tv_sec = inf.st_mtime;
					lm.tv_nsec = inf.st_mtimensec;
					sprintf(buffer,"\377yYou have new mail in %s.", mpath);
					c_msg_print(buffer);
				}
			}
  #endif
		}
		mailticks = ticks;
	}
 #endif
#endif /* CHECK_MAIL */
}

/* Ping the server once a second when enabled - mikaelh
   do_ping is called every frame. */
void do_ping() {
	static int last_ping = 0;
	static int time_stamp_hour = -1;

	if (lagometer_enabled && (ticks - last_ping >= 10)) {
		last_ping = ticks;

		/* Update the lag-o-meter just before sending a new ping */
		update_lagometer();

		Send_ping();
	}

	/* abusing it for weather for now - C. Blue */
	if (!noweather_mode && !c_cfg.no_weather) {
	    // && !screen_icky) {  -- mistake? after all, maybe all tiles are supposed to be restored because weather ended, while we're in an icky screen!
		do_weather();

#if 1 /* old method: Many weather particles turn on the sfx, few turn it off again. */
		/* handle audio output -- avoid easy oscillating */
		if (weather_particles_seen >= 8) {
			wind_noticable = TRUE;
 #ifdef USE_SOUND_2010
			weather_sound_change = 0;

			//in case the below method is active code-wise, also add this hack here:
			//weather_vol_smooth = cfg_audio_weather_volume;

			if (weather_type != -1) {
				if (weather_type % 10 == 1) { //rain
					if (weather_wind >= 1 && weather_wind <= 2) sound_weather(rain2_sound_idx);
					else sound_weather(rain1_sound_idx);
				} else if (weather_type % 10 == 2) { //snow
					if (weather_wind >= 1 && weather_wind <= 2) sound_weather(snow2_sound_idx);
					else sound_weather(snow1_sound_idx);
				}
			}
 #endif
		} else if (weather_particles_seen <= 3) {
			wind_noticable = FALSE;
 #ifdef USE_SOUND_2010
			weather_sound_change++;
			if (weather_sound_change == (cfg_client_fps > 100 ? 100 : cfg_client_fps)) {
				weather_sound_change = 0;
				sound_weather(-1); //fade out, insufficient particles to support the noise ;)
			}
		} else {
			weather_sound_change = 0;
 #endif
		}
#else /* WEATHER_VOL_PARTICLES -- new method: Amount of weather particles determines the sfx volume. */
		/* handle audio output -- avoid easy oscillating */
		if (weather_particles_seen >= 1) {
			wind_noticable = TRUE;
 #ifdef USE_SOUND_2010
			weather_sound_change = 0; /* for delaying, and then fading out, further below */
			if (weather_type != -1) {
				if (weather_type % 10 == 1) { //rain
					if (weather_wind >= 1 && weather_wind <= 2) sound_weather_vol(rain2_sound_idx, weather_particles_seen > 25 ? 100 : 0 + weather_particles_seen * 4);
					else sound_weather_vol(rain1_sound_idx, weather_particles_seen > 25 ? 100 : 0 + weather_particles_seen * 4);
				} else if (weather_type % 10 == 2) { //snow
					if (weather_wind >= 1 && weather_wind <= 2) sound_weather_vol(snow2_sound_idx, weather_particles_seen > 25 ? 100 : 0 + weather_particles_seen * 4);
					else sound_weather_vol(snow1_sound_idx, weather_particles_seen > 25 ? 100 : 0 + weather_particles_seen * 4);
				}
			}
 #endif
		} else {
			wind_noticable = FALSE;
 #ifdef USE_SOUND_2010
  #if 0
			sound_weather(-2); //turn off
  #else /* make it smoother: delay and then fade */
   #if 0
			weather_sound_change++;
			if (weather_sound_change == (cfg_client_fps > 100 ? 100 : cfg_client_fps)) {
				weather_sound_change = 0;
				sound_weather(-1); //fade out
			}
   #else /* just fade, no delay */
			sound_weather(-1); //fade out
   #endif
  #endif
 #endif
		}

 #ifdef USE_SOUND_2010
  #ifdef SOUND_SDL
		if (weather_fading) weather_handle_fading();
  #endif
 #endif
#endif
	}

#ifdef USE_SOUND_2010
 #ifdef SOUND_SDL
	/* make sure fading out an ambient sound to zero completes glitch-free */
	if (ambient_fading) ambient_handle_fading();

	/* change volume of background ambient sound effects or weather (if enabled),
	   check every 1/10 s (one tick):  */
	if (last_ping != ticks
	    && (last_ping + 1) % 10 != ticks) { /* actually skip 2 ticks -> 1/5s */
		bool modified = FALSE;

		if (grid_ambient_volume != grid_ambient_volume_goal) {
			modified = TRUE;
			grid_ambient_volume += grid_ambient_volume_step;

			/* catch rounding issues */
			if (grid_ambient_volume_step > 0) {
				if (grid_ambient_volume > grid_ambient_volume_goal)
					grid_ambient_volume = grid_ambient_volume_goal;
			} else {
				if (grid_ambient_volume < grid_ambient_volume_goal)
					grid_ambient_volume = grid_ambient_volume_goal;
			}
		}
		if (grid_weather_volume != grid_weather_volume_goal) {
			modified = TRUE;
			grid_weather_volume += grid_weather_volume_step;

			/* catch rounding issues */
			if (grid_weather_volume_step > 0) {
				if (grid_weather_volume > grid_weather_volume_goal)
					grid_weather_volume = grid_weather_volume_goal;
			} else {
				if (grid_weather_volume < grid_weather_volume_goal)
					grid_weather_volume = grid_weather_volume_goal;
			}
		}

		if (use_sound && modified) set_mixing();
	}
 #endif
#endif

	/* Handle chat time-stamping too - C. Blue */
	if (c_cfg.time_stamp_chat) {
		time_t ct = time(NULL);
		struct tm* ctl = localtime(&ct);
		if (ctl->tm_hour != time_stamp_hour) {
			if (time_stamp_hour != -1) {
				if (screen_icky && (!shopping || perusing)) Term_switch(0);
				c_msg_format("\374\376\377y[%02d:00h]", ctl->tm_hour);
				if (screen_icky && (!shopping || perusing)) Term_switch(0);
			}
			time_stamp_hour = ctl->tm_hour;
		}
	}
}

int Send_sip(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_SIP)) <= 0) return n;
	return 1;
}

int Send_telekinesis(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_TELEKINESIS)) <= 0) return n;
	return 1;
}

int Send_BBS(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_BBS)) <= 0) return n;
	return 1;
}

int Send_wield2(int item) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_WIELD2, item)) <= 0) return n;
	return 1;
}

int Send_cloak(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_CLOAK)) <= 0) return n;
	return 1;
}

int Send_inventory_revision(int revision) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%d", PKT_INVENTORY_REV, revision)) <= 0) return n;
	return 1;
}

int Send_force_stack(int item) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_FORCE_STACK, item)) <= 0) return n;
	return 1;
}

int Send_request_key(int id, char key) {
	int n;
	if ((n = Packet_printf(&wbuf, "%c%d%c", PKT_REQUEST_KEY, id, key)) <= 0) return n;
	return 1;
}
int Send_request_num(int id, int num) {
	int n;
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_REQUEST_NUM, id, num)) <= 0) return n;
	return 1;
}
int Send_request_str(int id, char *str) {
	int n;
	if ((n = Packet_printf(&wbuf, "%c%d%s", PKT_REQUEST_STR, id, str)) <= 0) return n;
	return 1;
}
int Send_request_cfr(int id, int cfr) {
	int n;
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_REQUEST_CFR, id, cfr)) <= 0) return n;
	return 1;
}

/* Resend F:/R:/K:/U: definitions, used after a font change. */
#define SEND_CLIENT_SETUP_FINISH Net_flush(); sync_sleep(20); /* min is 5ms */
int Send_client_setup(void) {
	int n, i;

	if (!is_newer_than(&server_version, 4, 6, 1, 2, 0, 0)) return -1;

#if 1 /* send it all at once (too much for buffers on Windows) */
	if ((n = Packet_printf(&wbuf, "%c", PKT_CLIENT_SETUP)) <= 0) return n;

	/* Send the "unknown" redefinitions */
	for (i = 0; i < TV_MAX; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.u_attr[i], Client_setup.u_char[i]);

	/* Send the "feature" redefinitions */
	for (i = 0; i < MAX_F_IDX; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.f_attr[i], Client_setup.f_char[i]);

	/* Send the "object" redefinitions */
	for (i = 0; i < MAX_K_IDX; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.k_attr[i], Client_setup.k_char[i]);

	/* Send the "monster" redefinitions */
	for (i = 0; i < MAX_R_IDX; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.r_attr[i], Client_setup.r_char[i]);
#else /* send the bigger ones in chunks */
	/* Send the "unknown" redefinitions */
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_U, 0, TV_MAX)) <= 0) return n;
	for (i = 0; i < TV_MAX; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.u_attr[i], Client_setup.u_char[i]);
	SEND_CLIENT_SETUP_FINISH

	/* Send the "feature" redefinitions */
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_F, 0, MAX_F_IDX)) <= 0) return n;
	for (i = 0; i < MAX_F_IDX; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.f_attr[i], Client_setup.f_char[i]);
	SEND_CLIENT_SETUP_FINISH

	/* Send the "object" redefinitions */
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_K, 0, 256)) <= 0) return n;
	for (i = 0; i < 256; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.k_attr[i], Client_setup.k_char[i]);
	SEND_CLIENT_SETUP_FINISH
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_K, 256, 512)) <= 0) return n;
	for (i = 256; i < 512; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.k_attr[i], Client_setup.k_char[i]);
	SEND_CLIENT_SETUP_FINISH
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_K, 512, 768)) <= 0) return n;
	for (i = 512; i < 768; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.k_attr[i], Client_setup.k_char[i]);
	SEND_CLIENT_SETUP_FINISH
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_K, 768, 1024)) <= 0) return n;//MAX_K_IDX_COMPAT
	for (i = 768; i < 1024; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.k_attr[i], Client_setup.k_char[i]);
	SEND_CLIENT_SETUP_FINISH
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_K, 1024, 1280)) <= 0) return n;//MAX_K_IDX
	for (i = 1024; i < 1280; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.k_attr[i], Client_setup.k_char[i]);
	SEND_CLIENT_SETUP_FINISH

	/* Send the "monster" redefinitions */
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_R, 0, 256)) <= 0) return n;
	for (i = 0; i < 256; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.r_attr[i], Client_setup.r_char[i]);
	SEND_CLIENT_SETUP_FINISH
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_R, 256, 512)) <= 0) return n;
	for (i = 256; i < 512; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.r_attr[i], Client_setup.r_char[i]);
	SEND_CLIENT_SETUP_FINISH
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_R, 512, 768)) <= 0) return n;
	for (i = 512; i < 768; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.r_attr[i], Client_setup.r_char[i]);
	SEND_CLIENT_SETUP_FINISH
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_R, 768, 1024)) <= 0) return n;//MAX_R_IDX_COMPAT
	for (i = 768; i < 1024; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.r_attr[i], Client_setup.r_char[i]);
	SEND_CLIENT_SETUP_FINISH
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_R, 1024, 1280)) <= 0) return n;//MAX_R_IDX
	for (i = 1024; i < 1280; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.r_attr[i], Client_setup.r_char[i]);
	SEND_CLIENT_SETUP_FINISH
#endif

	return 1;
}

/* Returns the amount of microseconds to the next frame (according to fps) - mikaelh */
int next_frame() {
	struct timeval tv;
	int time_between_frames = (1000000 / cfg_client_fps);
	int us;

#ifdef WINDOWS
	DWORD systime_ms = timeGetTime();
	tv.tv_sec = systime_ms / 1000;
	tv.tv_usec = (systime_ms % 1000) * 1000;
#else
	gettimeofday(&tv, NULL);
#endif

	us = time_between_frames - tv.tv_usec % time_between_frames;

	return us;
}
