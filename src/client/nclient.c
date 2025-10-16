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

#ifdef REGEX_SEARCH
 #include <regex.h>
 #define REGEX_ARRAY_SIZE 1
#endif


/* for weather - keep in sync with c-xtra1.c, do_weather()! */
#define PANEL_X	(SCREEN_PAD_LEFT)
#define PANEL_Y	(SCREEN_PAD_TOP)



/* Use Windows TEMP folder (acquired from environment variable) for pinging the servers in the meta server list.
   KEEP CONSISTENT WITH c-birth.c!
   Note: We actually use the user's home folder still, not the os_temp_path. TODO maybe: Change that. */
#define WINDOWS_USE_TEMP


/* Height of the huge bars [16, which is exactly the free space, with borders] */
#define HUGE_BAR_SIZE 16


/* Test rawpict_hook for Casino client-side animations */
#ifdef TEST_CLIENT
 /* - WiP - C. Blue
    visuals do not scale if font size isn't identical to tile size (results in blackness usually)
    and visuals get erased after animation finishes: */
#define TEST_RAWPICT
#endif


extern void flicker(void);

int	ticks = 0; /* Keeps track of time in 100ms "ticks" */
int	ticks10 = 0; /* 'deci-ticks', counting just 0..9, but in 10ms intervals */
int	existing_characters = 0;

int	command_confirmed = -1;

static void do_meta_pings(void);

static sockbuf_t	rbuf, wbuf, qbuf;
static int		(*receive_tbl[256])(void);
static int		last_send_anything;
static char		cl_initialized = 0;


/* Based on Virus' MP bar mod */
char *marker1 = "#######";
char *marker2 = "####";
char *marker3 = "###";
char *marker4 = "##";
#ifdef WINDOWS
char smarker1[] = { FONT_MAP_SOLID_WIN, FONT_MAP_SOLID_WIN, FONT_MAP_SOLID_WIN, FONT_MAP_SOLID_WIN, FONT_MAP_SOLID_WIN, FONT_MAP_SOLID_WIN, FONT_MAP_SOLID_WIN, 0 };
char smarker2[] = { FONT_MAP_SOLID_WIN, FONT_MAP_SOLID_WIN, FONT_MAP_SOLID_WIN, FONT_MAP_SOLID_WIN, 0 };
char smarker3[] = { FONT_MAP_SOLID_WIN, FONT_MAP_SOLID_WIN, FONT_MAP_SOLID_WIN, 0 };
char smarker4[] = { FONT_MAP_SOLID_WIN, FONT_MAP_SOLID_WIN, 0 };
#elif defined(USE_X11)
char smarker1[] = { FONT_MAP_SOLID_X11, FONT_MAP_SOLID_X11, FONT_MAP_SOLID_X11, FONT_MAP_SOLID_X11, FONT_MAP_SOLID_X11, FONT_MAP_SOLID_X11, FONT_MAP_SOLID_X11, 0 };
char smarker2[] = { FONT_MAP_SOLID_X11, FONT_MAP_SOLID_X11, FONT_MAP_SOLID_X11, FONT_MAP_SOLID_X11, 0 };
char smarker3[] = { FONT_MAP_SOLID_X11, FONT_MAP_SOLID_X11, FONT_MAP_SOLID_X11, 0 };
char smarker4[] = { FONT_MAP_SOLID_X11, FONT_MAP_SOLID_X11, 0 };
//#else /* command-line client ("-c") doesn't draw either! */
#endif
void clear_huge_bars(void) {
	int n;

	/* Huge bars are only available in big_map mode */
	if (screen_hgt != MAX_SCREEN_HGT) return;

	for (n = MAX_SCREEN_HGT - 2 - HUGE_BAR_SIZE; n <= MAX_SCREEN_HGT - 2; n++)
		Term_putstr(1 , n, -1, TERM_DARK, "           ");
}
/* Typ: 0 : mp, 1 : sanity, 2 : hp --- max width of everything depends on SCREEN_PAD_LEFT (hardcoded here though) */
void draw_huge_bar(int typ, int *prev, int cur, int *prev_max, int max) {
	int n, c, p;
	bool gain, redraw;
	int ys, ye, col, pos, x = 0; //kill compiler warning
	char *marker = marker3; //kill compiler warning
	byte af = TERM_WHITE, ae = TERM_SLATE; //kill compiler warnings

	/* Huge bars are only available in big_map mode */
	if (screen_hgt != MAX_SCREEN_HGT) return;

	/* Ensure sane limits */
	if (cur < 0) cur = 0; //happens eg if player dies and hence HP turns negative
	/* handle and ignore hacks (-9999) */
	if (max <= 0) {
		cur = 0; //prevent any visual overflow
		max = 1;
	}

	if (*prev_max != max) {
		*prev_max = max;
		/* Redraw the bar from scratch */
		*prev = -1;
	}

	/* Hack: Unset 'previous' value means we just need/want to redraw */
	if (*prev == -1) {
		redraw = TRUE;
		*prev = 0;
	} else redraw = FALSE;

	c = (cur * HUGE_BAR_SIZE) / max;
	p = (*prev * HUGE_BAR_SIZE) / max;

	/* Workaround relog glitch on some chars */
	if (redraw) p = 0;

	/* No change in values and we don't want to redraw? Nothing to do then */
	if (p == c && !redraw) return;

	switch (typ) {
	case 0: if (!c_cfg.mp_huge_bar) return;
		af = TERM_L_BLUE;
		ae = TERM_L_DARK;
		break;
	case 1: if (!c_cfg.sn_huge_bar) return;
		af = TERM_GREEN;
		ae = TERM_L_RED;//TERM_ORANGE;//TERM_YELLOW;
		break;
	case 2: if (!c_cfg.hp_huge_bar) return;
		af = TERM_L_GREEN;
		ae = TERM_RED;
		break;
	case 3: if (!c_cfg.st_huge_bar) return;
		af = TERM_L_WHITE;
		ae = TERM_L_DARK;
		break;
	}

	/* Order of unimportance, from left to right: MP / SN / HP -- edit: appended ST to the right, maybe todo: move it to the left instead */
	pos = -1;
	if (c_cfg.mp_huge_bar) pos++;
	if (typ > 0) {
		if (c_cfg.sn_huge_bar) pos++;
		if (typ > 1) {
			if (c_cfg.hp_huge_bar) pos++;
			if (typ > 2) if (c_cfg.st_huge_bar) pos++;
		}
	}
	/* Find actual bar width */
	switch ((c_cfg.mp_huge_bar ? 1 : 0) + (c_cfg.sn_huge_bar ? 1 : 0) + (c_cfg.hp_huge_bar ? 1 : 0) + (c_cfg.st_huge_bar ? 1 : 0)) {
	case 1:
		x = 3;
#if defined(WINDOWS) || defined(USE_X11)
		if (!force_cui && c_cfg.solid_bars) marker = smarker1;
		else
#endif
		marker = marker1;
		break;
	case 2:
		x = 2 + (4 + 1) * pos;
#if defined(WINDOWS) || defined(USE_X11)
		if (!force_cui && c_cfg.solid_bars) marker = smarker2;
		else
#endif
		marker = marker2;
		break;
	case 3:
		x = 1 + (3 + 1) * pos;
#if defined(WINDOWS) || defined(USE_X11)
		if (!force_cui && c_cfg.solid_bars) marker = smarker3;
		else
#endif
		marker = marker3;
		break;
	case 4:
		x = 1 + (2 + 1) * pos;
#if defined(WINDOWS) || defined(USE_X11)
		if (!force_cui && c_cfg.solid_bars) marker = smarker4;
		else
#endif
		marker = marker4;
		break;
	}
	gain = (c > p);
	ys = MAX_SCREEN_HGT - 2 - (gain ? p : c);
	ye = MAX_SCREEN_HGT - 2 - (gain ? c : p);
	col = (gain ? af : ae);

	/* Fill extra (non-green) part with red if we don't start out full */
	if (redraw)
		for (n = ye; n > MAX_SCREEN_HGT - 2 - HUGE_BAR_SIZE; n--) {
#ifdef USE_GRAPHICS //821,822,813, 813,813,813, 823,813,824
			if (use_graphics && c_cfg.huge_bars_gfx) {
				int w;

				/* Silyl visual hack: Don't draw rounded corners if we're "inside" the huge stun bar... would look odd to have these little black patches ~~ */
				if (c_cfg.stun_huge_bar && p_ptr->stun) {
					for (w = x; w - x < strlen(marker); w++)
						Term_draw(w, n, ae, 813);
				/* Normal bars while not stunned... */
				} else if (n == MAX_SCREEN_HGT - 2 - HUGE_BAR_SIZE + 1) {
					for (w = x; w - x < strlen(marker); w++)
						Term_draw(w, n, ae, w == x ? 821 : (w - x == strlen(marker) - 1 ? 822 : 813));
				} else if (n == MAX_SCREEN_HGT - 2) {
					for (w = x; w - x < strlen(marker); w++)
						Term_draw(w, n, ae, w == x ? 823 : (w - x == strlen(marker) - 1 ? 824 : 813));
				} else {
					for (w = x; w - x < strlen(marker); w++)
						Term_draw(w, n, ae, 813);
				}
			} else
#endif
			Term_putstr(x, n, -1, ae, marker);
		}

	/* Only draw the difference to before */
	for (n = ys; n > ye; n--)
#ifdef USE_GRAPHICS
		if (use_graphics && c_cfg.huge_bars_gfx) {
			int w;

			/* Silyl visual hack: Don't draw rounded corners if we're "inside" the huge stun bar... would look odd to have these little black patches ~~ */
			if (c_cfg.stun_huge_bar && p_ptr->stun) {
				for (w = x; w - x < strlen(marker); w++)
					Term_draw(w, n, col, 813);
			/* Normal bars while not stunned... */
			} else if (n == MAX_SCREEN_HGT - 2 - HUGE_BAR_SIZE + 1) {
				for (w = x; w - x < strlen(marker); w++)
					Term_draw(w, n, col, w == x ? 821 : (w - x == strlen(marker) - 1 ? 822 : 813));
			} else if (n == MAX_SCREEN_HGT - 2) {
				for (w = x; w - x < strlen(marker); w++)
					Term_draw(w, n, col, w == x ? 823 : (w - x == strlen(marker) - 1 ? 824 : 813));
			} else {
				for (w = x; w - x < strlen(marker); w++)
					Term_draw(w, n, col, 813);
			}
		} else
#endif
		Term_putstr(x, n, -1, col, marker);

	*prev = cur;
}

/* More stuff inspired by Virus and his death '+_+ */
void draw_huge_stun_bar(byte attr) {
	char c;
	int x, y;
	//char32_t *scr_cc;

	/* Huge bars are only available in big_map mode */
	if (screen_hgt != MAX_SCREEN_HGT) return;

#if defined(WINDOWS) || defined(USE_X11)
	if (!force_cui && c_cfg.solid_bars)
 #ifdef WINDOWS
		c = FONT_MAP_SOLID_WIN;
 #elif defined(USE_X11)
		c = FONT_MAP_SOLID_X11;
 #endif
	else
#endif
	c = '#';

	/* Just clear the bar? */
	if (attr == TERM_DARK) c = ' ';

	for (y = MAX_SCREEN_HGT - 2 - HUGE_BAR_SIZE; y <= MAX_SCREEN_HGT - 2 + 1; y++) {
		//scr_cc = Term->scr->c[y];
		for (x = 1; x < SCREEN_PAD_LEFT - 1; x++)
			//if (scr_cc[x] == ' ')
#ifdef USE_GRAPHICS
				if (use_graphics && c_cfg.huge_bars_gfx && c != ' ')
					Term_draw(x, y, attr,
					    y == MAX_SCREEN_HGT - 2 - HUGE_BAR_SIZE ? (
					    x == 1 ? 821 : (x == SCREEN_PAD_LEFT - 1 - 1 ? 822 : 813)) :
					    (y == MAX_SCREEN_HGT - 2 + 1 ? (
					    x == 1 ? 823 : (x == SCREEN_PAD_LEFT - 1 - 1 ? 824 : 813)) :
					    813));
				else
#endif
				Term_putch(x, y, attr, c);
	}

	/* Restore the other huge bars over the stun "background bar" */
	prev_huge_cmp = prev_huge_csn = prev_huge_chp = prev_huge_cst = -1; // force redrawing
	if (p_ptr->mmp) draw_huge_bar(0, &prev_huge_cmp, p_ptr->cmp, &prev_huge_mmp, p_ptr->mmp);
	if (p_ptr->msane) draw_huge_bar(1, &prev_huge_csn, p_ptr->csane, &prev_huge_msn, p_ptr->msane);
	if (p_ptr->mhp) draw_huge_bar(2, &prev_huge_chp, p_ptr->chp, &prev_huge_mhp, p_ptr->mhp);
	if (p_ptr->mst) draw_huge_bar(3, &prev_huge_cst, p_ptr->cst, &prev_huge_mst, p_ptr->mst);
}


/*
 * Initialize the function dispatch tables.
 */
static void Receive_init(void) {
	int i;

	for (i = 0; i < 256; i++)
		receive_tbl[i] = NULL;

	receive_tbl[PKT_RELIABLE]	= NULL; /* Server shouldn't be sending this */
	receive_tbl[PKT_QUIT]		= Receive_quit;
	receive_tbl[PKT_RELOGIN]	= Receive_relogin;
	receive_tbl[PKT_START]		= NULL; /* Server shouldn't be sending this */
	receive_tbl[PKT_END]		= Receive_end;
	receive_tbl[PKT_LOGIN]		= NULL; /* Should not be called like this */
	receive_tbl[PKT_FILE]		= Receive_file;

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
	receive_tbl[PKT_MP]		= Receive_mp;
	receive_tbl[PKT_HISTORY]	= Receive_history;
	receive_tbl[PKT_CHAR]		= Receive_char;
	receive_tbl[PKT_MESSAGE]	= Receive_message;
	receive_tbl[PKT_STATE]		= Receive_state;
	receive_tbl[PKT_TITLE]		= Receive_title;
	receive_tbl[PKT_DEPTH]		= Receive_depth;
	receive_tbl[PKT_CONFUSED]	= Receive_confused;
	receive_tbl[PKT_POISON]		= Receive_poison;
	receive_tbl[PKT_STUDY]		= Receive_study;
	receive_tbl[PKT_BPR]		= Receive_bpr_wraith_prob;
	receive_tbl[PKT_FOOD]		= Receive_food;
	receive_tbl[PKT_FEAR]		= Receive_fear;
	receive_tbl[PKT_SPEED]		= Receive_speed;
	receive_tbl[PKT_CUT]		= Receive_cut;
	receive_tbl[PKT_BLIND]		= Receive_blind_hallu;
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
	receive_tbl[PKT_ITEM_NEWEST_2ND]= Receive_item_newest_2nd;
	receive_tbl[PKT_CONFIRM]	= Receive_confirm;
	receive_tbl[PKT_KEYPRESS]	= Receive_keypress;

	receive_tbl[PKT_SFX_VOLUME]	= Receive_sfx_volume;
	receive_tbl[PKT_SFX_AMBIENT]	= Receive_sfx_ambient;
	receive_tbl[PKT_MINI_MAP_POS]	= Receive_mini_map_pos;

	receive_tbl[PKT_REQUEST_KEY]	= Receive_request_key;
	receive_tbl[PKT_REQUEST_AMT]	= Receive_request_amt;
	receive_tbl[PKT_REQUEST_NUM]	= Receive_request_num;
	receive_tbl[PKT_REQUEST_STR]	= Receive_request_str;
	receive_tbl[PKT_REQUEST_CFR]	= Receive_request_cfr;
	receive_tbl[PKT_REQUEST_ABORT]	= Receive_request_abort;
	receive_tbl[PKT_STORE_SPECIAL_STR]	= Receive_store_special_str;
	receive_tbl[PKT_STORE_SPECIAL_CHAR]	= Receive_store_special_char;
	receive_tbl[PKT_STORE_SPECIAL_CLR]	= Receive_store_special_clr;
	receive_tbl[PKT_STORE_SPECIAL_ANIM]	= Receive_store_special_anim;

	receive_tbl[PKT_MARTYR]		= Receive_martyr;
	receive_tbl[PKT_PALETTE]	= Receive_palette;
	receive_tbl[PKT_IDLE]		= Receive_idle;
	receive_tbl[PKT_POWERS_INFO]	= Receive_powers_info;

	receive_tbl[PKT_GUIDE]		= Receive_Guide;
	receive_tbl[PKT_INDICATORS]	= Receive_indicators;
	receive_tbl[PKT_PLAYERLIST]	= Receive_playerlist;
	receive_tbl[PKT_WEATHERCOL]	= Receive_weather_colouring;
	receive_tbl[PKT_MUSIC_VOL]	= Receive_music_vol;
	receive_tbl[PKT_WHATS_UNDER_YOUR_FEET]	= Receive_whats_under_you_feet;

	receive_tbl[PKT_SCREENFLASH]	= Receive_screenflash;
#ifdef ENABLE_SUBINVEN
	receive_tbl[PKT_SI_MOVE]	= Receive_subinven;
#endif
	receive_tbl[PKT_SPECIAL_LINE_POS]	= Receive_special_line_pos;
	receive_tbl[PKT_VERSION]		= Receive_version;
	receive_tbl[PKT_EQUIP_WIDE]	= Receive_equip_wide;
	receive_tbl[PKT_SFLAGS]		= Receive_sflags;
	receive_tbl[PKT_CHAR_DIRECT]	= Receive_char;
	receive_tbl[PKT_MACRO_FAILURE] 	= Receive_macro_failure;
}


/* Head of file transfer system receive */
/* DO NOT TOUCH - work in progress */
int Receive_file(void) {
	char command, ch;
	char fname[MAX_CHARS];	/* possible filename */
	int x;	/* return value/ack */
	char outbuf[80];
	unsigned short fnum;	/* unique SENDER side file number */
	unsigned short len;
	u32b csum; /* old 32-bit checksum */
	unsigned char digest[16]; /* new 128-bit MD5 checksum */
	int n, bytes_read;
	static bool updated_audio = FALSE, updated_guide = FALSE;

	/* NOTE: The amount of data read is stored in n so that the socket
	 * buffer can be rolled back if the packet isn't complete. - mikaelh */
	if ((n = Packet_scanf(&rbuf, "%c%c%hd", &ch, &command, &fnum)) <= 0) return(n);

	if (n != 3) return(1);

	bytes_read = 4;

	switch (command) {
	case PKT_FILE_INIT:
		if ((n = Packet_scanf(&rbuf, "%s", fname)) <= 0) {
			/* Rollback the socket buffer */
			Sockbuf_rollback(&rbuf, bytes_read);
			return(n);
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
			if (strstr(fname, "guide.lua")) updated_guide = TRUE;
		} else {
			if (errno == EACCES) c_msg_print("\377rNo access to update files");
		}
		break;
	case PKT_FILE_DATA:
		if ((n = Packet_scanf(&rbuf, "%hd", &len)) <= 0) {
			/* Rollback the socket buffer */
			Sockbuf_rollback(&rbuf, bytes_read);
			return(n);
		}
		bytes_read += 2;
		x = local_file_write(0, fnum, len);
		if (x == -1) {
			/* Not enough data available */

			/* Rollback the socket buffer */
			Sockbuf_rollback(&rbuf, bytes_read);
			return(0);
		} else if (x == -2) {
			/* Write failed */
			sprintf(outbuf, "\377rWrite failed [%d]", fnum);
			c_msg_print(outbuf);
			return(0);
		}
		break;
	case PKT_FILE_END:
		x = local_file_close(0, fnum);
		if (x) {
			sprintf(outbuf, "\377gReceived file %d", fnum);
			c_msg_print(outbuf);
		} else {
			if (errno == EACCES) c_msg_print("\377rNo access to lib directory!");
			else c_msg_print("\377rFile could not be written!");
		}

		/* HACK - If this was the last file, reinitialize Lua on the fly - mikaelh */
		if (!get_xfers_num()) {
			c_msg_print("\377GReinitializing Lua");
			reopen_lua();

			if (updated_audio) {
				c_msg_print("\377R* Audio information was updated - restarting the game is recommended! *");
				c_msg_print("\377R   Without a restart, you might be hearing the wrong sound effects or music.");
			}
			if (updated_guide) {
				int i;

				/* Silently re-init guide info (keep consistent to init_guide() in c-init.c) */
				guide_races = exec_lua(0, "return guide_races");
				for (i = 0; i < guide_races; i++)
					strcpy(guide_race[i], string_exec_lua(0, format("return guide_race[%d]", i + 1)));

				guide_classes = exec_lua(0, "return guide_classes");
				for (i = 0; i < guide_classes; i++)
					strcpy(guide_class[i], string_exec_lua(0, format("return guide_class[%d]", i + 1)));

				guide_skills = exec_lua(0, "return guide_skills");
				for (i = 0; i < guide_skills; i++)
					strcpy(guide_skill[i], string_exec_lua(0, format("return guide_skill[%d]", i + 1)));

				guide_schools = exec_lua(0, "return guide_schools");
				for (i = 0; i < guide_schools; i++)
					strcpy(guide_school[i], string_exec_lua(0, format("return guide_school[%d]", i + 1)));

				guide_spells = exec_lua(0, "return guide_spells");
				for (i = 0; i < guide_spells; i++)
					strcpy(guide_spell[i], string_exec_lua(0, format("return guide_spell[%d]", i + 1)));
			}
		}

		break;
	case PKT_FILE_CHECK:
		if ((n = Packet_scanf(&rbuf, "%s", fname)) <= 0) {
			/* Rollback the socket buffer */
			Sockbuf_rollback(&rbuf, bytes_read);
			return(n);
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
		return(1);
		break;
	case PKT_FILE_SUM:
		if (is_newer_than(&server_version, 4, 6, 1, 1, 0, 1)) {
			unsigned digest_net[4];

			if ((n = Packet_scanf(&rbuf, "%u%u%u%u", &digest_net[0], &digest_net[1], &digest_net[2], &digest_net[3])) <= 0) {
				/* Rollback the socket buffer */
				Sockbuf_rollback(&rbuf, bytes_read);
				return(n);
			}
			md5_digest_to_char_array(digest, digest_net);
			check_return_new(0, fnum, digest, 0);
		} else {
			if ((n = Packet_scanf(&rbuf, "%d", &csum)) <= 0) {
				/* Rollback the socket buffer */
				Sockbuf_rollback(&rbuf, bytes_read);
				return(n);
			}
			check_return(0, fnum, csum, 0);
		}
		return(1);
	case PKT_FILE_ACK:
		local_file_ack(0, fnum);
		return(1);
	case PKT_FILE_ERR:
		local_file_err(0, fnum);
		/* continue the send/terminate */
		return(1);
	case 0:
		return(1);
	default:
		x = 0;
	}

	Packet_printf(&wbuf, "%c%c%hd", PKT_FILE, x ? PKT_FILE_ACK : PKT_FILE_ERR, fnum);
	return(1);
}

int Receive_file_data(int ind, unsigned short len, char *buffer) {
	(void) ind; /* suppress compiler warning */

	/* Check that the sockbuf has enough data */
	if (&rbuf.buf[rbuf.len] >= &rbuf.ptr[len]) {
		memcpy(buffer, rbuf.ptr, len);
		rbuf.ptr += len;
		return(1);
	} else {
		/* Wait for more data */
		return(0);
	}
}

int Send_file_check(int ind, unsigned short id, char *fname) {
	(void) ind; /* suppress compiler warning */

	Packet_printf(&wbuf, "%c%c%hd%s", PKT_FILE, PKT_FILE_CHECK, id, fname);
	return(0);
}

/* index arguments are just for common / laziness */
int Send_file_init(int ind, unsigned short id, char *fname) {
	(void) ind; /* suppress compiler warning */

	Packet_printf(&wbuf, "%c%c%hd%s", PKT_FILE, PKT_FILE_INIT, id, fname);
	return(0);
}

int Send_file_data(int ind, unsigned short id, char *buf, unsigned short len) {
	(void) ind; /* suppress compiler warning */

	Sockbuf_flush(&wbuf);
	Packet_printf(&wbuf, "%c%c%hd%hd", PKT_FILE, PKT_FILE_DATA, id, len);
	if (Sockbuf_write(&wbuf, buf, len) != len)
		logprint("failed sending file data\n");
	return(0);
}

int Send_file_end(int ind, unsigned short id) {
	(void) ind; /* suppress compiler warning */

	Packet_printf(&wbuf, "%c%c%hd", PKT_FILE, PKT_FILE_END, id);
	return(0);
}

#define LOTSOFCHARS 30 /* just something that is at least as high as max_cpa + all extra slots */
/* Mode:
   1: Swap two characters
   2: Insert character before another
   3: Insert character after another ('append')
*/
static bool reorder_characters(int col, int col_cmd, int chars, char names[LOTSOFCHARS][MAX_CHARS], char mode) {
	char ch;
	u32b dummy;
	unsigned char sortA = 255, sortB = 255;
	int n;

	/* Reorder-GUI */
	//c_put_str(TERM_SELECTOR, "[", col + sel, 3);
	//c_put_str(TERM_SELECTOR, "]", col + sel, 76);
	if (mode == 1) c_put_str(TERM_L_BLUE, "Press the slot letter of the first of two characters to swap..", col_cmd, 5);
	else c_put_str(TERM_L_BLUE, "Press the slot letter of the character to move..", col_cmd, 5);
	ch = 0;
	while (!ch) {
		ch = inkey();
		if (ch == '\e') {
			c_put_str(TERM_L_BLUE, "                                                               ", col_cmd, 5);
			return(FALSE);
		}
		if (ch < 'a' || ch >= 'a' + chars) ch = 0;
	}
	sortA = ch;
	c_put_str(TERM_SLATE, format("Selected: %c) %s", ch, names[ch - 'a']), col_cmd + 1, 5);
	switch (mode) {
	case 1: c_put_str(TERM_L_BLUE, "Press the slot letter of the second of two characters to swap..", col_cmd, 5); break;
	case 2: c_put_str(TERM_L_BLUE, "Press the slot letter of a character before which to insert..", col_cmd, 5); break;
	case 3: c_put_str(TERM_L_BLUE, "Press the slot letter of a character after which to insert..", col_cmd, 5); break;
	}
	ch = 0;
	while (!ch) {
		ch = inkey();
		if (ch == '\e') {
			c_put_str(TERM_L_BLUE, "                                                               ", col_cmd, 5);
			c_put_str(TERM_L_BLUE, "                                                               ", col_cmd + 1, 5);
			return(FALSE);
		}
		if (ch < 'a' || ch >= 'a' + chars) ch = 0;
	}
	sortB = ch;
	c_put_str(TERM_L_BLUE, "                                                               ", col_cmd, 5);
	c_put_str(TERM_L_BLUE, "                                                               ", col_cmd + 1, 5);

	/* Tell server which characters we want to swap, server will answer with full character screen data again */
	Packet_printf(&wbuf, "%c%s", PKT_LOGIN, format("***%c%c%c", sortA, sortB, mode));
	Net_flush(); //send it nao!
	//SetTimeout(5, 0);

	/* Eat and discard server flags, we already know those */
	if ((n = Packet_scanf(&rbuf, "%c%d%d%d%d", &ch, &dummy, &dummy, &dummy, &dummy)) <= 0) {
		plog("Packet scan error when trying to read server flags.");
#ifdef RETRY_LOGIN
		rl_connection_destroyed = TRUE;
		return(FALSE);
#endif
		quit(NULL);
	}
	return(TRUE);
}

#define CHARSCREEN_COLOUR TERM_L_GREEN
#define COL_CHARS 4
void Receive_login(void) {
	int n;
	char ch;
	int i = 0, max_cpa, max_cpa_plus = 0;
	short mode = 0;
	char names[LOTSOFCHARS][MAX_CHARS], colour_sequence[MAX_CHARS]; //just init way too many names[] so we don't need to bother counting max_cpa + max dedicated slots..
	char tmp[MAX_CHARS + 3];
	int ded_pvp = 0, ded_iddc = 0, ded_pvp_shown, ded_iddc_shown;
	char loc[MAX_CHARS];

	static char c_name[MAX_CHARS], *cp;
	s16b c_race, c_class, level;

	/* un-hardcode character amount limits */
	int max_ded_pvp_chars = 1, max_ded_iddc_chars = 2;
	int offset, total_cpa;

	bool new_ok = TRUE, exclusive_ok = TRUE;
	bool found_nick = FALSE;

	bool allow_reordering = FALSE;
	int offset_bak;


	/* Check if the server wanted to destroy the connection - mikaelh */
	if (rbuf.ptr[0] == PKT_QUIT) {
		errno = 0;
#ifdef RETRY_LOGIN
		rl_connection_destroyed = TRUE;
		plog(&rbuf.ptr[1]);
		/* illegal name? don't suggest it as default again */
		if (strstr(&rbuf.ptr[1], "a different name")) strcpy(nick, "");
		/* no password entered? then auto-fill-in the name again he alread picked */
		else if (strstr(&rbuf.ptr[1], "nter a password") /* (Avoid upper-case issues) */
		    || strstr(&rbuf.ptr[1], "assword length")) /* or if the password was just too short (avoid upper-case issues) */
			rl_password = TRUE;
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
		plog("Packet scan error when trying to read server flags.");
#ifdef RETRY_LOGIN
		rl_connection_destroyed = TRUE;
		return;
#endif
		quit(NULL);
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
	if (sflags0 & SFLG0_PVP_MAIA) s_PVP_MAIA = TRUE;

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

	if (!(sflags3 & 0xFF00) && !(sflags3 & 0xFF))
		max_ded_iddc_chars = 2; //backward compatibility, no need for version check ;)
	else
		max_ded_iddc_chars = (sflags3 & 0xFF00) >> 8;

	if (!(sflags3 & 0xFF0000) && !(sflags3 & 0xFF))
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

	if (s_ARCADE) {
		c_put_str(TERM_SLATE, "The server is running 'ARCADE_SERVER' settings.", 21, 10);
		if (is_older_than(&server_version, 4, 9, 0, 5, 0, 0)) {
			/* Reset default values (2+1) back to zero */
			max_ded_pvp_chars = max_ded_iddc_chars = max_cpa_plus = 0;
		}
	}
	if (s_RPG) {
		if (!s_ARCADE) c_put_str(TERM_SLATE, "The server is running 'IRONMAN_SERVER' settings.", 21, 10);
		if (is_older_than(&server_version, 4, 9, 0, 5, 0, 0)) {
			if (!s_RPG_ADMIN) max_cpa = 1;
			/* Reset default values (2+1) back to zero */
			max_ded_pvp_chars = max_ded_iddc_chars = max_cpa_plus = 0;
		}
	}

	if (is_newer_than(&server_version, 4, 5, 8, 1, 0, 0)) {
		if (s_DED_IDDC) max_cpa_plus += max_ded_iddc_chars;
		if (s_DED_PVP) max_cpa_plus += max_ded_pvp_chars;
	} else if (!s_ARCADE && !s_RPG) { /* backward compatibility */
		if (s_DED_IDDC) max_cpa_plus++;
		if (s_DED_PVP) max_cpa_plus++;
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
		if (is_older_than(&server_version, 4, 7, 3, 0, 0, 0))
			c_put_str(CHARSCREEN_COLOUR, "Choose an existing character:", 3, 2);
		else {
			c_put_str(CHARSCREEN_COLOUR, "Choose an existing character: (S/I/A to reorder)", 3, 2);
			allow_reordering = TRUE;
		}
		offset = 4;
	} else if (total_cpa <= 15) {
		if (is_older_than(&server_version, 4, 7, 3, 0, 0, 0))
			c_put_str(CHARSCREEN_COLOUR, "Choose an existing character:", 2, 2);
		else {
			c_put_str(CHARSCREEN_COLOUR, "Choose an existing character: (S/I/A to reorder)", 2, 2);
			allow_reordering = TRUE;
		}
		offset = 3;
	} else if (total_cpa <= 16) {
		offset = 2;
	} else {
		// --- hypothetical screen overflow case (more than 16 characters in total)  ---
		//TODO: Not enough space on character screen to show more character slots!
		offset = 2;
		//temporarily fix via hack, urgh:
		max_cpa = 16 - max_cpa_plus;
		total_cpa = 16;
	}

	max_cpa += max_cpa_plus; /* for now, don't keep them in separate list positions
				    but just use the '*' marker attached at server side
				    for visual distinguishing in this character list. */
	offset_bak = offset;
	receive_characters:
	offset = offset_bak;
	i = 0;
	ded_pvp = ded_iddc = 0;
	/* Receive list of characters ('zero'-terminated) */
	*loc = 0;
	while ((is_newer_than(&server_version, 4, 5, 7, 0, 0, 0) ?
	    (n = Packet_scanf(&rbuf, "%c%hd%s%s%hd%hd%hd%s", &ch, &mode, colour_sequence, c_name, &level, &c_race, &c_class, loc)) :
	    (is_newer_than(&server_version, 4, 4, 9, 2, 0, 0) ?
	    (n = Packet_scanf(&rbuf, "%c%hd%s%s%hd%hd%hd", &ch, &mode, colour_sequence, c_name, &level, &c_race, &c_class)) :
	    (n = Packet_scanf(&rbuf, "%c%s%s%hd%hd%hd", &ch, colour_sequence, c_name, &level, &c_race, &c_class))))
	    > 0) {
	//while ((n = Packet_scanf(&rbuf, "%c%s%s%hd%hd%hd", &ch, colour_sequence, c_name, &level, &c_race, &c_class)) > 0) {
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

		/* Erase line (for allow_reordering list redrawal) */
		c_put_str(TERM_WHITE, "                                                                         ", offset + i, COL_CHARS);

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

	create_character_ok_pvp = create_character_ok_iddc = FALSE;
	ded_pvp_shown = ded_pvp;
	ded_iddc_shown = ded_iddc;
	for (n = max_cpa - i; n > 0; n--) {
		if (is_newer_than(&server_version, 4, 5, 8, 1, 0, 0)) {
			if (max_cpa_plus) {
				if (ded_pvp_shown < max_ded_pvp_chars) {
					c_put_str(TERM_SLATE, "<free PvP-exclusive slot>", offset + i + n - 1, COL_CHARS);
					ded_pvp_shown++;
					create_character_ok_pvp = TRUE;
					continue;
				}
				if (ded_iddc_shown < max_ded_iddc_chars) {
					c_put_str(TERM_SLATE, "<free IDDC-exclusive slot>", offset + i + n - 1, COL_CHARS);
					ded_iddc_shown++;
					create_character_ok_iddc = TRUE;
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
		create_character_ok_pvp = create_character_ok_iddc = TRUE;
	}

	offset += max_cpa + 1;
	/* Fix GUI visuals for IRONMAN_SERVER (RPG_SERVER): */
	if (max_cpa < 11) offset += (11 - max_cpa);

	/* Only exclusive-slots left? Then don't display 'N' option. */
	if (i - ded_pvp - ded_iddc >= max_cpa - max_ded_pvp_chars - max_ded_iddc_chars) new_ok = FALSE;
	/* Only non-exclusive-slots left? Then don't display 'E' option. */
	if (!((ded_pvp < max_ded_pvp_chars || ded_iddc < max_ded_iddc_chars) && max_cpa_plus)) exclusive_ok = FALSE;

	//plog_fmt("i %d, ded_pvp %d, ded_iddc %d, max_cpa %d, max_ded_pvp_chars %d, max_ded_iddc_chars %d, new_ok %d", i, ded_pvp, ded_iddc, max_cpa, max_ded_pvp_chars, max_ded_iddc_chars, new_ok);
	if (i < max_cpa) {
		if (new_ok) c_put_str(CHARSCREEN_COLOUR, "N) Create a new character", offset, 2);
		if (exclusive_ok) {
			/* hack: no weird modi on first client startup!
			   To find out whether it's 1st or not we check firstrun and # of existing characters.
			   However, we just don't display the choice, but it's still choosable by pressing the key anyway except for firstrun! */
			if ((!firstrun || existing_characters)
			    /* We have still exclusive slots left? */
			    && (create_character_ok_pvp || create_character_ok_iddc))
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

	while ((ch < 'a' || ch >= 'a' + i) && (((ch != 'N' || !new_ok) && (ch != 'E' || !exclusive_ok || firstrun)) || i > (max_cpa - 1))
	    && ((ch != 'S' && ch != 'I' && ch != 'A') || !allow_reordering)) {
		ch = inkey();
		//added CTRL+Q for RETRY_LOGIN, so you can quit the whole game from within in-game via simply double-tapping CTRL+Q
		if (ch == 'Q' || ch == KTRL('Q')) quit(NULL);
		/* Take a screenshot */
		if (ch == KTRL('T')) {
			xhtml_screenshot("screenshot????", 2);
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
	if (ch == 'S' || ch == 'I' || ch == 'A') {
		switch (ch) {
		case 'S': ch = 1; break;
		case 'I': ch = 2; break;
		case 'A': ch = 3; break;
		}
		if (reorder_characters(offset_bak, offset, i, names, ch)) {
			ch = 0;
			goto receive_characters;
		}
		ch = 0;
#ifdef RETRY_LOGIN
		if (rl_connection_destroyed) return;
#endif
		goto enter_menu;
	}
	if (ch == 'N' || (ch == 'E' && !firstrun)) {
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
			c_put_str(TERM_SLATE, "Keep blank for random name, ESC to cancel. Allowed symbols: .,-'&_$%~#", offset + 1, COL_CHARS);
		else
			c_put_str(TERM_SLATE, "Keep blank for random name, ESC to cancel. Allowed symbols: .,-'&_$%~#", offset, 35);

		while (1) {
			c_put_str(TERM_YELLOW, "New name: ", offset, COL_CHARS);
			if (!askfor_aux(c_name, CNAME_LEN - 1, ASKFOR_LIVETRIM)) {
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

#ifdef RETRY_LOGIN
	/* Try to free up previously allocated memory */
	if (trait_info) {
		for (i = 0; i < Setup.max_trait; i++) {
			if (trait_info[i].title) {
				string_free(trait_info[i].title);
				trait_info[i].title = NULL;
			}
		}
	}
	if (class_info) {
		for (i = 0; i < Setup.max_class; i++) {
			if (class_info[i].title) {
				string_free(class_info[i].title);
				class_info[i].title = NULL;
			}
		}
	}
	if (race_info) {
		for (i = 0; i < Setup.max_race; i++) {
			if (race_info[i].title) {
				string_free(race_info[i].title);
				race_info[i].title = NULL;
			}
		}
	}
	if (trait_info) C_FREE(trait_info, Setup.max_trait || 1, player_trait);
	if (class_info) C_FREE(class_info, Setup.max_class, player_class);
	if (race_info) C_FREE(race_info, Setup.max_race, player_race);
#endif

	/* Initialize a new socket buffer */
	if (Sockbuf_init(&cbuf, -1, CLIENT_RECV_SIZE,
	    SOCKBUF_WRITE | SOCKBUF_READ | SOCKBUF_LOCK) == -1) {
		plog(format("No memory for control buffer (%u)", CLIENT_RECV_SIZE));
		return(-1);
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
						for (j = 0; j < C_ATTRIBUTES; j++) {
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
					return(-1);
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

	return(0);
}

/*
 * Send the first packet to the server with our name,
 * nick and display contained in it.
 * The server uses this data to verify that the packet
 * is from the right UDP connection, it already has
 * this info from the ENTER_GAME_pack.
 */
int Net_verify(char *real, char *nick, char *pass) {
	int n, type, result;

	Sockbuf_clear(&wbuf);
	n = Packet_printf(&wbuf, "%c%s%s%s", PKT_VERIFY, real, nick, pass);

	if (n <= 0 || Sockbuf_flush(&wbuf) <= 0)
		plog("Can't send verify packet");

	SetTimeout(5, 0);
	if (!SocketReadable(rbuf.sock)) {
		errno = 0;
		plog("No verify response");
		return(-1);
	}
	Sockbuf_clear(&rbuf);
	if (Sockbuf_read(&rbuf) == -1) {
		plog("Can't read verify reply packet");
		return(-1);
	}
	if (Sockbuf_flush(&wbuf) == -1)
		return(-1);
	if (Receive_reply(&type, &result) <= 0) {
		errno = 0;
		plog("Can't receive verify reply packet");
		return(-1);
	}
	if (type != PKT_VERIFY) {
		errno = 0;
		plog(format("Verify wrong reply type (%d)", type));
		return(-1);
	}
	if (result != PKT_SUCCESS) {
		errno = 0;
		plog(format("Verification failed (%d)", result));
		return(-1);
	}
	if (Receive_magic() <= 0) {
		plog("Can't receive magic packet after verify");
		return(-1);
	}

	return(0);
}


/*
 * Open the datagram socket and allocate the network data
 * structures like buffers.
 * Currently there are two different buffers used:
 * 1) wbuf is used only for sending packets (write/printf).
 * 2) rbuf is used for receiving packets in (read/scanf).
 */
int Net_init(int fd) {
	int sock;

	/*signal(SIGPIPE, SIG_IGN);*/

	Receive_init();

	sock = fd;

	wbuf.sock = sock;
	if (SetSocketNoDelay(sock, 1) == -1) {
		plog("Can't set TCP_NODELAY on socket");
		return(-1);
	}
	if (SetSocketSendBufferSize(sock, CLIENT_SEND_SIZE + 256) == -1)
		plog(format("Can't set send buffer size to %d: error %d", CLIENT_SEND_SIZE + 256, errno));
	if (SetSocketReceiveBufferSize(sock, CLIENT_RECV_SIZE + 256) == -1)
		plog(format("Can't set receive buffer size to %d", CLIENT_RECV_SIZE + 256));

	/* queue buffer, not a valid socket filedescriptor needed */
	if (Sockbuf_init(&qbuf, -1, CLIENT_RECV_SIZE,
	    SOCKBUF_WRITE | SOCKBUF_READ | SOCKBUF_LOCK) == -1) {
		plog(format("No memory for queue buffer (%u)", CLIENT_RECV_SIZE));
		return(-1);
	}

	/* read buffer */
	if (Sockbuf_init(&rbuf, sock, CLIENT_RECV_SIZE,
	    SOCKBUF_READ | SOCKBUF_WRITE) == -1) {
		plog(format("No memory for read buffer (%u)", CLIENT_RECV_SIZE));
		return(-1);
	}

	/* write buffer */
	if (Sockbuf_init(&wbuf, sock, CLIENT_SEND_SIZE,
	    SOCKBUF_WRITE) == -1) {
		plog(format("No memory for write buffer (%u)", CLIENT_SEND_SIZE));
		return(-1);
	}

	/* Initialized */
	cl_initialized = 1;
	fullscreen_weather = FALSE;

	return(0);
}


/*
 * Cleanup all the network buffers and close the datagram socket.
 * Also try to send the server a quit packet if possible.
 */
void Net_cleanup(void) {
	int sock = wbuf.sock;
	char ch;

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
		return(0);
	}
	if (Sockbuf_flush(&wbuf) == -1)
		return(-1);
	Sockbuf_clear(&wbuf);
	last_send_anything = ticks;
	return(1);
}


/*
 * Return the socket filedescriptor for use in a select(2) call.
 */
int Net_fd(void) {
	if (!cl_initialized || c_quit) return(-1);
#ifdef RETRY_LOGIN
	/* prevent inkey() from crashing/causing Sockbuf_read() errors ("no read from non-readable socket..") */
	if (rl_connection_state != 1) return(-1);
#endif
	return(rbuf.sock);
}

unsigned char Net_login() {
	unsigned char tc;

	Sockbuf_clear(&wbuf);

	Packet_printf(&wbuf, "%c%s", PKT_LOGIN, "");
	if (is_atleast(&server_version, 4, 9, 2, 1, 0, 2))
		Packet_printf(&wbuf, "%c%c%c%c%c%c", ip_iaddr[0], ip_iaddr[1], ip_iaddr[2], ip_iaddr[3], ip_iaddr[4], ip_iaddr[5]);

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
	if (rl_connection_destroyed) return(E_RETRY_CONTACT);
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
		return(E_RETRY_LOGIN);
#endif
		quit(&rbuf.ptr[1]);
	}
	/* Same/similar (WiP) for SERVER_PORTALS - C. Blue */
	if (rbuf.len > 1 && rbuf.ptr[0] == (char)PKT_RELOGIN) {
#ifdef RETRY_LOGIN
		rl_connection_destroyed = TRUE;
		/* should be illegal character name this time.. */
		plog(&rbuf.ptr[1]);
		return(E_RETRY_LOGIN);
#endif
		quit(&rbuf.ptr[1]);
	}
#ifdef RETRY_LOGIN
	/* back to normal - can quit() anytime */
	rl_connection_destructible = 0;
#endif

	if (Packet_scanf(&rbuf, "%c", &tc) <= 0) {
		plog("You were disconnected, probably because a server update happened meanwhile.\nPlease log in again.");
		quit("Failed to read status code from server!");
	}
	return(tc);
}

/*
 * Try to send a `start play' packet to the server and get an
 * acknowledgement from the server.  This is called after
 * we have initialized all our other stuff like the user interface
 * and we also have the map already.
 */
int Net_start(int sex, int race, int class) {
	int i;
	int type, result;
	char fname[1024] = { 0 }; //init for GCU-only mode: there is no font name available

	Sockbuf_clear(&wbuf);
	Packet_printf(&wbuf, "%c", PKT_PLAY);

#if defined(WINDOWS) || defined(USE_X11)
 #if defined(WINDOWS) && defined(USE_LOGFONT)
	if (use_logfont) sprintf(fname, "<LOGFONT>%dx%d", win_get_logfont_w(0), win_get_logfont_h(0));
	else
 #endif
	get_term_main_font_name(fname);
#endif
	//&graphics_tile_wid, &graphics_tile_hgt)) {
	//td->font_wid; td->font_hgt;
	if (is_atleast(&server_version, 4, 8, 1, 2, 0, 0))
#ifdef USE_GRAPHICS
		Packet_printf(&wbuf, "%hd%hd%hd%hd%hd%hd%hd%s%s", sex, race, class, trait, audio_sfx, audio_music, use_graphics, graphic_tiles, fname);
#else
		Packet_printf(&wbuf, "%hd%hd%hd%hd%hd%hd%hd%s%s", sex, race, class, trait, audio_sfx, audio_music, use_graphics, "NO_GRAPHICS", fname);
#endif
	else if (is_newer_than(&server_version, 4, 4, 5, 10, 0, 0))
		Packet_printf(&wbuf, "%hd%hd%hd%hd%hd%hd", sex, race, class, trait, audio_sfx, audio_music);
	else Packet_printf(&wbuf, "%hd%hd%hd", sex, race, class);

	/* Send the desired stat order */
	for (i = 0; i < C_ATTRIBUTES; i++)
		Packet_printf(&wbuf, "%hd", stat_order[i]);

	/* Send the options */
	if (is_newer_than(&server_version, 4, 9, 1, 0, 0, 0)) {
		for (i = 0; i < OPT_MAX; i++)
			Packet_printf(&wbuf, "%c", Client_setup.options[i]);
	} else if (is_newer_than(&server_version, 4, 5, 8, 1, 0, 1)) {
		for (i = 0; i < OPT_MAX_154; i++)
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
	char32_t max_char = 0;
	int limit;

	/* Send the "unknown" redefinitions */
	for (i = 0; i < TV_MAX; i++) {
		/* 4.8.1 and newer servers communicate using 32bit character size. */
		if (is_atleast(&server_version, 4, 8, 1, 0, 0, 0))
			Packet_printf(&wbuf, "%c%u", Client_setup.u_attr[i], Client_setup.u_char[i]);
		else
			Packet_printf(&wbuf, "%c%c", Client_setup.u_attr[i], Client_setup.u_char[i]);

		if (max_char < Client_setup.u_char[i]) max_char = Client_setup.u_char[i];
	}

	/* Send the "feature" redefinitions */
	if (is_newer_than(&server_version, 4, 9, 1, 2, 0, 2)) limit = MAX_F_IDX;
	else if (is_newer_than(&server_version, 4, 6, 1, 2, 0, 0)) limit = MAX_F_IDX_COMPAT;
	else limit = MAX_F_IDX_COMPAT; //used to be different

	for (i = 0; i < limit; i++) {
		/* 4.8.1 and newer servers communicate using 32bit character size. */
		if (is_atleast(&server_version, 4, 8, 1, 0, 0, 0))
			Packet_printf(&wbuf, "%c%u", Client_setup.f_attr[i], Client_setup.f_char[i]);
		else
			Packet_printf(&wbuf, "%c%c", Client_setup.f_attr[i], Client_setup.f_char[i]);

		if (max_char < Client_setup.f_char[i]) max_char = Client_setup.f_char[i];
	}

	/* Send the "object" redefinitions */
	if (is_newer_than(&server_version, 4, 6, 1, 2, 0, 0)) limit = MAX_K_IDX;
	else limit = MAX_K_IDX_COMPAT;

	for (i = 0; i < limit; i++) {
		/* 4.8.1 and newer servers communicate using 32bit character size. */
		if (is_atleast(&server_version, 4, 8, 1, 0, 0, 0))
			Packet_printf(&wbuf, "%c%u", Client_setup.k_attr[i], Client_setup.k_char[i]);
		else
			Packet_printf(&wbuf, "%c%c", Client_setup.k_attr[i], Client_setup.k_char[i]);

		if (max_char < Client_setup.k_char[i]) max_char = Client_setup.k_char[i];
	}

	/* Send the "monster" redefinitions */
	if (is_newer_than(&server_version, 4, 6, 1, 2, 0, 0)) limit = MAX_R_IDX;
	else limit = MAX_R_IDX_COMPAT;

	for (i = 0; i < limit; i++) {
		/* 4.8.1 and newer servers communicate using 32bit character size. */
		if (is_atleast(&server_version, 4, 8, 1, 0, 0, 0))
			Packet_printf(&wbuf, "%c%u", Client_setup.r_attr[i], Client_setup.r_char[i]);
		else
			Packet_printf(&wbuf, "%c%c", Client_setup.r_attr[i], Client_setup.r_char[i]);

		if (max_char < Client_setup.r_char[i]) max_char = Client_setup.r_char[i];
	}

	/* Calculate and update minimum character transfer bytes */
	Client_setup.char_transfer_bytes = 0;
	for ( ; max_char != 0; max_char >>= 8) Client_setup.char_transfer_bytes += 1;
#endif

	if (Sockbuf_flush(&wbuf) == -1) quit("Can't send start play packet");

	/* Wait for data to arrive */
	SetTimeout(5, 0);
	if (!SocketReadable(rbuf.sock)) {
		errno = 0;
		quit("No play packet reply received.");
	}

	Sockbuf_clear(&rbuf);
	if (Sockbuf_read(&rbuf) == -1) {
		quit("Error reading play reply");
		return(-1);
	}

	/* If our connection wasn't accepted, quit */
	if (rbuf.ptr[0] == PKT_QUIT) {
		errno = 0;
		quit(&rbuf.ptr[1]);
		return(-1);
	}
	if (rbuf.ptr[0] != PKT_REPLY) {
		errno = 0;
		quit(format("Not a reply packet after play (%d,%d,%d)",
			rbuf.ptr[0], rbuf.ptr - rbuf.buf, rbuf.len));
		return(-1);
	}
	if (Receive_reply(&type, &result) <= 0) {
		errno = 0;
		quit("Can't receive reply packet after play");
		return(-1);
	}
	if (type != PKT_PLAY) {
		errno = 0;
		quit("Can't receive reply packet after play");
		return(-1);
	}
	if (result != PKT_SUCCESS) {
		errno = 0;
		quit(format("Start play not allowed (%d)", result));
		return(-1);
	}
	/* Finish processing any commands that were sent to us along with
	 * the PKT_PLAY packet.
	 *
	 * Advance past our PKT_PLAY data
	 * Sockbuf_advance(&rbuf, 3);
	 * Actually process any leftover commands in rbuf
	 */
	if (Net_packet() == -1) return(-1);

	errno = 0;
	return(0);
}


/*
 * Process a packet.
 */
static int Net_packet(void) {
	int type, prev_type = 0, result;
	char *old_ptr;

	/* Process all of the received client updates */
	while (rbuf.buf + rbuf.len > rbuf.ptr) {
		type = (*rbuf.ptr & 0xFF);
#if DEBUG_LEVEL > 2
		if (type > 50) logprint(format("Received packet: %d\n", type));
#endif	/* DEBUG_LEVEL */
		if (receive_tbl[type] == NULL) {
			errno = 0;
#if 0
 #ifndef WIN32 /* suppress annoying msg boxes in windows clients, when unknown-packet-errors occur. -- why just on Windows? */
			//plog(format("Received unknown packet type (%d, %d), dropping", type, prev_type));
 #endif
#else
			Send_unknownpacket(type, prev_type);
			c_msg_format("\377RReceived unknown packet type (%d, %d), dropping", type, prev_type);
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
					if (type != PKT_QUIT && type != PKT_RELOGIN) {
						errno = 0;
						plog(format("Processing packet type (%d, %d) failed", type, prev_type));
					}
					Sockbuf_clear(&rbuf);
					return(-1);
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
	return(0);
}

/*
 * Read packets from the net until there are no more available.
 */
int Net_input(void) {
	int n;
	int netfd;

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
//			if (rl_connection_destroyed) return(1);
#endif
			return(n);
		} else {
			n = Net_packet();

			/* Make room for more packets */
			Sockbuf_advance(&rbuf, rbuf.ptr - rbuf.buf);

			if (n == -1) return(-1);
		}
	}

	return(1);
}

int Flush_queue(void) {
	int len;

	if (!cl_initialized) return(0);
#ifdef RETRY_LOGIN
	if (rl_connection_state != 1) return(0);
#endif

	len = qbuf.len - (qbuf.ptr - qbuf.buf);

	if (Sockbuf_write(&rbuf, qbuf.ptr, len) != len) {
		plog("Can't copy queued data to buffer");
		return(-1);
	}
	Sockbuf_clear(&qbuf);

	Net_packet();

	return(1);
}

/*
 * Receive the end of a new frame update packet,
 * which should contain the same loops number
 * as the frame head.  If this terminating packet
 * is missing then the packet is corrupt or incomplete.
 */
int Receive_end(void) {
	int n;
	byte ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return(n);
	return(1);
}

/*
 * Magic packets are old relics that don't do anything useful.
 */
int Receive_magic(void) {
	int n;
	byte ch;
	unsigned magic;

	if ((n = Packet_scanf(&rbuf, "%c%u", &ch, &magic)) <= 0) return(n);
	return(1);
}

int Receive_reply(int *replyto, int *result) {
	int n;
	byte type, ch1, ch2;

	n = Packet_scanf(&rbuf, "%c%c%c", &type, &ch1, &ch2);
	if (n <= 0) return(n);
	if (n != 3 || type != PKT_REPLY) {
		plog("Can't receive reply packet");
		*replyto = -1;
		*result = -1;
		return(1);
	}
	*replyto = ch1;
	*result = ch2;
	return(1);
}

/* Always try to re-login, that basically means: Include server updates/terminations */
//#define ALWAYS_RETRY_LOGIN
int Receive_quit(void) {
	unsigned char pkt;
	char reason[MAX_CHARS_WIDE];

	/* game ends, so leave all other screens like
	   shops or browsed books or skill screen etc */
	if (screen_icky) Term_load();
	topline_icky = FALSE;

	if (Packet_scanf(&rbuf, "%c", &pkt) != 1) {
		errno = 0;
		plog("Can't read quit packet");
	} else {
		if (Packet_scanf(&rbuf, "%s", reason) <= 0) strcpy(reason, "unknown reason");
		errno = 0;

#ifdef RETRY_LOGIN
 #ifndef ALWAYS_RETRY_LOGIN
		/* Try to relogin if we manually quit via arg-less slash command */
		if (!strcmp(reason, "client quit")) {
 #endif
			rl_connection_destructible = TRUE;
			rl_connection_state = 2;
 #ifndef ALWAYS_RETRY_LOGIN
		}
 #endif
#endif

		/* Hack -- tombstone */
		if (strstr(reason, "Killed by") ||
		    strstr(reason, "Committed suicide") || strstr(reason, "Retired")) {
			/* TERAHACK -- assume our network state is 'not connected' again, as it was initially on client startup. */
			cl_initialized = FALSE;

#if defined(RETRY_LOGIN) //&& !defined(ALWAYS_RETRY_LOGIN)
			rl_connection_destructible = TRUE;
			rl_connection_state = 2;
#endif

			c_close_game(reason);
		}

		quit(format("%s", reason));
	}
	return(-1);
}
/* Quit like Receive_quit(), but relogin to an IP specified to us by the server - for SERVER_PORTALS. */
int Receive_relogin(void) {
	unsigned char pkt;
	char reason[MAX_CHARS_WIDE], delay;

	/* game ends, so leave all other screens like
	   shops or browsed books or skill screen etc */
	if (screen_icky) Term_load();
	topline_icky = FALSE;

	/* 'reason' is an optional parameter - no real reason for this though (badumtsh) */
	if (Packet_scanf(&rbuf, "%c%s%s%s%s%s%c", &pkt, relogin_host, relogin_accname, relogin_accpass, relogin_charname, reason, &delay) != 7) {
		errno = 0;
		plog("Can't read relogin packet");
		return(-1);
	}

#ifdef RETRY_LOGIN
	rl_connection_destructible = TRUE;
	rl_connection_state = 2;
#endif

	quit(format("Relog to %s\n(%s)", relogin_host, reason));
#ifdef WINDOWS
	Sleep(delay * 100); //ms
#else
	usleep(delay * 100000); //us
#endif
	return(-1);
}

int Receive_sanity(void) {
#ifdef SHOW_SANITY
	int n, cur = 0, max = 0;
	char ch, buf[MAX_CHARS];
	byte attr;

	if (is_atleast(&server_version, 4, 8, 1, 3, 0, 0)) {
		char dam;

		if ((n = Packet_scanf(&rbuf, "%c%c%s%c%hd%hd", &ch, &attr, buf, &dam, &cur, &max)) <= 0) return(n);
 #ifdef USE_SOUND_2010
		/* Send beep when we're losing Sanity while we're busy in some other window */
		if (c_cfg.alert_offpanel_dam && screen_icky && dam) warning_page();
 #endif
	} else if (is_newer_than(&server_version, 4, 6, 1, 2, 0, 0)) {
		char dam;

		if ((n = Packet_scanf(&rbuf, "%c%c%s%c", &ch, &attr, buf, &dam)) <= 0) return(n);
 #ifdef USE_SOUND_2010
		/* Send beep when we're losing Sanity while we're busy in some other window */
		if (c_cfg.alert_offpanel_dam && screen_icky && dam) warning_page();
 #endif
	} else if ((n = Packet_scanf(&rbuf, "%c%c%s", &ch, &attr, buf)) <= 0) return(n);

	p_ptr->csane = cur;
	p_ptr->msane = max;

	strcpy(c_p_ptr->sanity, buf);
	c_p_ptr->sanity_attr = attr;

	if (screen_icky) Term_switch(0);

	prt_sane(attr, (char*)buf);
	if (c_cfg.alert_hitpoint && (attr == TERM_MULTI)) {
		warning_page();
		c_msg_print("\377R*** LOW SANITY WARNING! ***");
	}

	/* Servers < 4.8.1.3 do not support this for sanity! */
	if (max) draw_huge_bar(1, &prev_huge_csn, cur, &prev_huge_msn, max);

	if (screen_icky) Term_switch(0);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);
#endif
	return(1);
}

int Receive_stat(void) {
	int n;
	char ch;
	char stat;
	s16b max, cur, s_ind, max_base, tmp = 0;
	bool boosted;

	if (is_atleast(&server_version, 4, 7, 4, 6, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%hd%hd%hd%hd%hd", &ch, &stat, &max, &cur, &s_ind, &max_base, &tmp)) <= 0)
			return(n);
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%hd%hd%hd%hd", &ch, &stat, &max, &cur, &s_ind, &max_base)) <= 0)
			return(n);
	}

	if (tmp) boosted = TRUE;
	else if (stat & 0x10) { /* Hack no longer used since 4.7.4.6 */
		stat &= ~0x10;
		boosted = TRUE;
	} else boosted = FALSE;

	p_ptr->stat_top[(int) stat] = max;
	p_ptr->stat_use[(int) stat] = cur;
	p_ptr->stat_ind[(int) stat] = s_ind;
	p_ptr->stat_max[(int) stat] = max_base;
	p_ptr->stat_tmp[(int) stat] = tmp;

	if (screen_icky) Term_switch(0);
	prt_stat(stat, boosted);
	if (screen_icky) Term_switch(0);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return(1);
}

int Receive_hp(void) {
	int n;
	char  ch;
	char drain;
	s16b max, cur;
#ifdef USE_SOUND_2010
	static int prev_chp = 0;
#endif
	bool bar = FALSE;

	if (is_newer_than(&server_version, 4, 7, 0, 2, 0, 1)) {
		if ((n = Packet_scanf(&rbuf, "%c%hd%hd%c", &ch, &max, &cur, &drain)) <= 0)
			return(n);
	} else {
		drain = FALSE;
		if ((n = Packet_scanf(&rbuf, "%c%hd%hd", &ch, &max, &cur)) <= 0)
			return(n);
	}

	/* Display hack */
	if (max > 10000) {
		max -= 10000;
		bar = TRUE;
	}
	/* .. and new, clean way: It's a client option now */
	if (c_cfg.hp_bar) bar = TRUE;

	/* ..Display hack for temporarily boosted HP -_- */
	if (cur > 5000 /* Checking for > 10000 can cause bugs if our HP drop < 0 aka when we die, while under boosted effect!
			  So we ensure leeway of exactly the middle, +/-5000, to reach the 10000 hack:
			  On the one hand allow having up to <5k HP and on the other hand allow taking up to <5k damage below 0 HP on death. */
	    ) {
		cur -= 10000;
		hp_boosted = TRUE;
	} else hp_boosted = FALSE;

	p_ptr->mhp = max;
	p_ptr->chp = cur;

#ifdef USE_SOUND_2010
	/* Send beep when we're losing HP while we're busy in some other window */
	if (c_cfg.alert_offpanel_dam && screen_icky && cur < prev_chp && !drain) warning_page();
	if (cur != prev_chp) prev_chp = cur;
#endif

	if (screen_icky) Term_switch(0);

	prt_hp(max, cur, bar, hp_boosted);
	if (c_cfg.alert_hitpoint && (cur < max / 5)) {
		warning_page();
		c_msg_print("\377R*** LOW HITPOINT WARNING! ***");
	}

	draw_huge_bar(2, &prev_huge_chp, cur, &prev_huge_mhp, max);
	if (screen_icky) Term_switch(0);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return(1);
}

int Receive_stamina(void) {
	int n;
	char 	ch;
	s16b max, cur;
	bool bar = FALSE;

	if ((n = Packet_scanf(&rbuf, "%c%hd%hd", &ch, &max, &cur)) <= 0)
		return(n);

	/* Display hack */
	if (max > 10000) {
		max -= 10000;
		bar = TRUE;
	}
	/* .. and new, clean way: It's a client option now */
	if (c_cfg.st_bar) bar = TRUE;

	p_ptr->mst = max;
	p_ptr->cst = cur;

	if (screen_icky) Term_switch(0);
	prt_stamina(max, cur, bar);
	draw_huge_bar(3, &prev_huge_cst, cur, &prev_huge_mst, max);
	if (screen_icky) Term_switch(0);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return(1);
}

int Receive_ac(void) {
	int n;
	char ch;
	s16b base, plus;
	bool unknown = FALSE, boosted = FALSE;

	if ((n = Packet_scanf(&rbuf, "%c%hd%hd", &ch, &base, &plus)) <= 0) return(n);

	if (is_atleast(&server_version, 4, 7, 3, 1, 0, 0)) {
		if (base > 15000) {
			base -= 20000;
			unknown = TRUE;
		}
		if (base > 5000) {
			base -= 10000;
			boosted = TRUE;
		}
	}

	p_ptr->dis_ac = base;
	p_ptr->dis_to_a = plus;

	if (screen_icky) Term_switch(0);

	if (is_atleast(&server_version, 4, 7, 3, 1, 0, 0))
		prt_ac(base + plus, boosted, unknown);
	else if (is_atleast(&server_version, 4, 7, 3, 0, 0, 0))
		prt_ac((base & ~0x1000) + plus, (base & 0x1000) != 0, FALSE);
	else
		prt_ac(base + plus, FALSE, FALSE);

	if (screen_icky) Term_switch(0);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return(1);
}

int Receive_apply_auto_insc(void) {
	int n;
	s16b slot;
	char ch, slotc;

	if (is_older_than(&server_version, 4, 9, 2, 1, 0, 1)) {
		if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &slotc)) <= 0) return(n);
		slot = (s16b)slotc;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &slot)) <= 0) return(n);
	}
	(void)apply_auto_inscriptions_aux((int)slot, -1, FALSE);
	return(1);
}

int Receive_inven(void) {
	int n;
	char ch;
	char pos, uses_dir = 0;
	byte attr, tval, sval;
	s16b wgt, amt, pval, name1 = 0;
	char name[ONAME_LEN], *insc;
#if defined(POWINS_DYNAMIC) && defined(POWINS_DYNAMIC_CLIENTSIDE)
	char tmp[ONAME_LEN];
#endif

	if (is_newer_than(&server_version, 4, 5, 2, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%hd%c%I", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, &name1, &uses_dir, name)) <= 0)
			return(n);
	} else if (is_newer_than(&server_version, 4, 4, 5, 10, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%c%I", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, &uses_dir, name)) <= 0)
			return(n);
	} else if (is_newer_than(&server_version, 4, 4, 4, 2, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%I", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, name)) <= 0)
			return(n);
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%s", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, name)) <= 0)
			return(n);
	}

	/* Check that the inventory slot is valid - mikaelh */
	if (pos < 'a' || pos > 'x') return(0);

#ifdef MSTAFF_MDEV_COMBO
	if (tval == TV_MSTAFF && wgt >= 30000) {
		wgt -= 30000;
		inventory[pos - 'a'].xtra4 = 1; //mark as 'rechargable' for ITH_RECHARGE
	}
#endif
	/* Hack -- The color is stored in the sval, since we don't use it for anything else */
	/* Hack -- gotta ahck to work around the previous hackl .. damn I hate that */
		/* I'm one of those who really hate it .. Jir */
		/* I hated it too much I swapped them	- Jir - */
	inventory[pos - 'a'].sval = sval;
	inventory[pos - 'a'].tval = tval;
	if ((tval == TV_BOOK && is_custom_tome(sval)) || tval == TV_SUBINVEN) /* ENABLE_SUBINVEN */
		inventory[pos - 'a'].bpval = pval;
	else inventory[pos - 'a'].pval = pval;
	inventory[pos - 'a'].name1 = name1;
	inventory[pos - 'a'].attr = attr;
	inventory[pos - 'a'].weight = wgt;
	inventory[pos - 'a'].number = amt;
	inventory[pos - 'a'].uses_dir = uses_dir & 0x1;
	inventory[pos - 'a'].ident = uses_dir & 0x6; //new hack in 4.7.1.2+ for ITH_ID/ITH_STARID
	inventory[pos - 'a'].iron_trade = uses_dir & 0x8;

#if defined(POWINS_DYNAMIC) && defined(POWINS_DYNAMIC_CLIENTSIDE)
	/* Strip "@&" markers, as these are a purely server-side thing */
	while ((insc = strstr(name, "@&"))) {
		strcpy(tmp, insc + 2);
		strcpy(insc, tmp);
	}
	/* Strip "@^" markers, as these are a purely server-side thing */
	while ((insc = strstr(name, "@^"))) {
		strcpy(tmp, insc + 2);
		strcpy(insc, tmp);
	}
#endif

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
	inventory_name[pos - 'a'][ONAME_LEN - 1] = 0;

	/* Window stuff */
	p_ptr->window |= (PW_INVEN);

#ifdef ENABLE_SUBINVEN
	/* Update subinven subterm too if it's a bag */
	if (tval == TV_SUBINVEN) p_ptr->window |= PW_SUBINVEN;
#endif

	return(1);
}

#ifdef ENABLE_SUBINVEN
int Receive_subinven(void) {
	int n, ipos;
	char ch;
	char iposc, pos, uses_dir = 0;
	byte tval, sval, attr;
	s16b wgt, amt, pval, name1 = 0;
	char name[ONAME_LEN], *insc;
 #if defined(POWINS_DYNAMIC) && defined(POWINS_DYNAMIC_CLIENTSIDE)
	char tmp[ONAME_LEN];
 #endif

	if ((n = Packet_scanf(&rbuf, "%c%c%c%c%hu%hd%c%c%hd%hd%c%I", &ch, &iposc, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, &name1, &uses_dir, name)) <= 0)
		return(n);
	ipos = (int)iposc;

	/* Check that the inventory slot is valid - mikaelh */
	if (ipos < 0 || ipos > INVEN_PACK) return(0);
	if (pos < 'a' || pos > 'x') return(0);

#ifdef MSTAFF_MDEV_COMBO
	if (tval == TV_MSTAFF && wgt >= 30000) {
		wgt -= 30000;
		subinventory[ipos][pos - 'a'].xtra4 = 1; //mark as 'rechargable' for ITH_RECHARGE
	}
#endif
	/* Hack -- The color is stored in the sval, since we don't use it for anything else */
	/* Hack -- gotta ahck to work around the previous hackl .. damn I hate that */
		/* I'm one of those who really hate it .. Jir */
		/* I hated it too much I swapped them	- Jir - */
	subinventory[ipos][pos - 'a'].sval = sval;
	subinventory[ipos][pos - 'a'].tval = tval;
	if ((tval == TV_BOOK && is_custom_tome(sval)) || tval == TV_SUBINVEN) /* ENABLE_SUBINVEN */
		subinventory[ipos][pos - 'a'].bpval = pval;
	else subinventory[ipos][pos - 'a'].pval = pval;
	subinventory[ipos][pos - 'a'].name1 = name1;
	subinventory[ipos][pos - 'a'].attr = attr;
	subinventory[ipos][pos - 'a'].weight = wgt;
	subinventory[ipos][pos - 'a'].number = amt;
	subinventory[ipos][pos - 'a'].uses_dir = uses_dir & 0x1;
	subinventory[ipos][pos - 'a'].ident = uses_dir & 0x6; //new hack in 4.7.1.2+ for ITH_ID/ITH_STARID
	subinventory[ipos][pos - 'a'].iron_trade = uses_dir & 0x8;

 #if defined(POWINS_DYNAMIC) && defined(POWINS_DYNAMIC_CLIENTSIDE)
	/* Strip "@&" markers, as these are a purely server-side thing */
	while ((insc = strstr(name, "@&"))) {
		strcpy(tmp, insc + 2);
		strcpy(insc, tmp);
	}
	/* Strip "@^" markers, as these are a purely server-side thing */
	while ((insc = strstr(name, "@^"))) {
		strcpy(tmp, insc + 2);
		strcpy(insc, tmp);
	}
 #endif

	/* check for special "fake-artifact" inscription using '#' char */
	if ((insc = strchr(name, '\373'))) {
		subinventory_inscription[ipos][pos - 'a'] = insc - name;
		subinventory_inscription_len[ipos][pos - 'a'] = insc[1] - 1;
		/* delete the special code garbage from the name to make it human-readable */
		do {
			*insc = *(insc + 2);
			insc++;
		} while (*(insc + 1));
	} else subinventory_inscription[ipos][pos - 'a'] = 0;

	strncpy(subinventory_name[ipos][pos - 'a'], name, ONAME_LEN - 1);
	subinventory_name[ipos][pos - 'a'][ONAME_LEN - 1] = 0;

	//problem: update subinventory live. eg after activation-consumption, but also after unstow w/ latency?
	//maybe this, bad style?
	if (using_subinven != -1) show_subinven(ipos);

	p_ptr->window |= (PW_SUBINVEN);

	/* In case !G/!W inscriptions were removed (or added or changed),
	   we need to potentially update the [...] colourization of the bag item: */
	p_ptr->window |= (PW_INVEN_SUB);

	return(1);
}
#endif

/* Receive xtra data of an item. Added for customizable spell tomes - C. Blue */
int Receive_inven_wide(void) {
	int n;
	char ch;
	char pos, *insc, ident = 0;
	byte attr, tval, sval;
	s16b xtra1, xtra2, xtra3, xtra4, xtra5, xtra6, xtra7, xtra8, xtra9;
	byte xtra1b, xtra2b, xtra3b, xtra4b, xtra5b, xtra6b, xtra7b, xtra8b, xtra9b;
	s16b wgt, amt, pval, name1 = 0;
	char name[ONAME_LEN];
#if defined(POWINS_DYNAMIC) && defined(POWINS_DYNAMIC_CLIENTSIDE)
	char tmp[ONAME_LEN];
#endif

	/* hack in 4.9.0.7, no protocol change needed :) Instead of uses_dir, we encode everything into ident this time, both are %c (for SV_CUSTOM_OBJECT and for iron_trade) */
	if (is_newer_than(&server_version, 4, 7, 1, 1, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%hd%hd%hd%hd%hd%hd%hd%hd%hd%hd%I%c", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, &name1,
		    &xtra1, &xtra2, &xtra3, &xtra4, &xtra5, &xtra6, &xtra7, &xtra8, &xtra9, name, &ident)) <= 0)
			return(n);
	} else if (is_newer_than(&server_version, 4, 7, 0, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%hd%hd%hd%hd%hd%hd%hd%hd%hd%hd%I", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, &name1,
		    &xtra1, &xtra2, &xtra3, &xtra4, &xtra5, &xtra6, &xtra7, &xtra8, &xtra9, name)) <= 0)
			return(n);
	} else if (is_newer_than(&server_version, 4, 5, 2, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%hd%c%c%c%c%c%c%c%c%c%I", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, &name1,
		    &xtra1b, &xtra2b, &xtra3b, &xtra4b, &xtra5b, &xtra6b, &xtra7b, &xtra8b, &xtra9b, name)) <= 0)
			return(n);
		xtra1 = (s16b)xtra1b; xtra2 = (s16b)xtra2b; xtra3 = (s16b)xtra3b;
		xtra4 = (s16b)xtra4b; xtra5 = (s16b)xtra5b; xtra6 = (s16b)xtra6b;
		xtra7 = (s16b)xtra7b; xtra8 = (s16b)xtra8b; xtra9 = (s16b)xtra9b;
	} else if (is_newer_than(&server_version, 4, 4, 4, 2, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%c%c%c%c%c%c%c%c%c%I", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval,
		    &xtra1b, &xtra2b, &xtra3b, &xtra4b, &xtra5b, &xtra6b, &xtra7b, &xtra8b, &xtra9b, name)) <= 0)
			return(n);
		xtra1 = (s16b)xtra1b; xtra2 = (s16b)xtra2b; xtra3 = (s16b)xtra3b;
		xtra4 = (s16b)xtra4b; xtra5 = (s16b)xtra5b; xtra6 = (s16b)xtra6b;
		xtra7 = (s16b)xtra7b; xtra8 = (s16b)xtra8b; xtra9 = (s16b)xtra9b;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%c%c%c%c%c%c%c%c%c%s", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval,
		    &xtra1b, &xtra2b, &xtra3b, &xtra4b, &xtra5b, &xtra6b, &xtra7b, &xtra8b, &xtra9b, name)) <= 0)
			return(n);
		xtra1 = (s16b)xtra1b; xtra2 = (s16b)xtra2b; xtra3 = (s16b)xtra3b;
		xtra4 = (s16b)xtra4b; xtra5 = (s16b)xtra5b; xtra6 = (s16b)xtra6b;
		xtra7 = (s16b)xtra7b; xtra8 = (s16b)xtra8b; xtra9 = (s16b)xtra9b;
	}

	/* Check that the inventory slot is valid - mikaelh */
	if (pos < 'a' || pos > 'x') return(0);

#ifdef MSTAFF_MDEV_COMBO
	if (tval == TV_MSTAFF && wgt >= 30000) {
		wgt -= 30000;
		inventory[pos - 'a'].xtra4 = 1; //mark as 'rechargable' for ITH_RECHARGE
	}
#endif
	/* Hack -- The color is stored in the sval, since we don't use it for anything else */
	/* Hack -- gotta ahck to work around the previous hackl .. damn I hate that */
		/* I'm one of those who really hate it .. Jir */
		/* I hated it too much I swapped them	- Jir - */
	inventory[pos - 'a'].sval = sval;
	inventory[pos - 'a'].tval = tval;
	if ((tval == TV_BOOK && is_custom_tome(sval)) || tval == TV_SUBINVEN) /* ENABLE_SUBINVEN */
		inventory[pos - 'a'].bpval = pval;
	else inventory[pos - 'a'].pval = pval;
	inventory[pos - 'a'].name1 = name1;
	inventory[pos - 'a'].attr = attr;
	inventory[pos - 'a'].weight = wgt;
	inventory[pos - 'a'].number = amt;
	inventory[pos - 'a'].uses_dir = ident & 0x1;
	inventory[pos - 'a'].ident = ident & 0x6; //new hack in 4.7.1.2+ for ITH_ID/ITH_STARID
	inventory[pos - 'a'].iron_trade = ident & 0x8;

	inventory[pos - 'a'].xtra1 = xtra1;
	inventory[pos - 'a'].xtra2 = xtra2;
	inventory[pos - 'a'].xtra3 = xtra3;
	inventory[pos - 'a'].xtra4 = xtra4;
	inventory[pos - 'a'].xtra5 = xtra5;
	inventory[pos - 'a'].xtra6 = xtra6;
	inventory[pos - 'a'].xtra7 = xtra7;
	inventory[pos - 'a'].xtra8 = xtra8;
	inventory[pos - 'a'].xtra9 = xtra9;

#if defined(POWINS_DYNAMIC) && defined(POWINS_DYNAMIC_CLIENTSIDE)
	/* Strip "@&" markers, as these are a purely server-side thing */
	while ((insc = strstr(name, "@&"))) {
		strcpy(tmp, insc + 2);
		strcpy(insc, tmp);
	}
	/* Strip "@^" markers, as these are a purely server-side thing */
	while ((insc = strstr(name, "@^"))) {
		strcpy(tmp, insc + 2);
		strcpy(insc, tmp);
	}
#endif

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
	inventory_name[pos - 'a'][ONAME_LEN - 1] = 0;

	/* Window stuff */
	p_ptr->window |= (PW_INVEN);

	return(1);
}

/* Receive some unique list info for client-side processing - C. Blue */
int Receive_unique_monster(void) {
	int n;
	char ch;
	int u_idx, killed;
	char name[60];

	if ((n = Packet_scanf(&rbuf, "%c%d%d%s", &ch, &u_idx, &killed, name)) <= 0) return(n);

	r_unique[u_idx] = killed;
	strcpy(r_unique_name[u_idx], name);
	return(1);
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
	int n;
	char ch;
	char pos, uses_dir;
	byte attr, tval, sval;
	s16b wgt, amt, pval, name1 = 0;
	char name[ONAME_LEN];
#if defined(POWINS_DYNAMIC) && defined(POWINS_DYNAMIC_CLIENTSIDE)
	char tmp[ONAME_LEN];
#endif

	if (is_newer_than(&server_version, 4, 5, 2, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%hd%c%I", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, &name1, &uses_dir, name)) <= 0)
			return(n);
	} else if (is_newer_than(&server_version, 4, 4, 5, 10, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%c%I", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, &uses_dir, name)) <= 0)
			return(n);
	} else if (is_newer_than(&server_version, 4, 4, 4, 2, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%I", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, name)) <= 0)
			return(n);
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%s", &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, name)) <= 0)
			return(n);
	}

	/* Check that the equipment slot is valid - mikaelh */
	if (pos < 'a' || pos > 'n') return(0);

#ifdef MSTAFF_MDEV_COMBO
	if (tval == TV_MSTAFF && wgt >= 30000) {
		wgt -= 30000;
		inventory[pos - 'a' + INVEN_WIELD].xtra4 = 1; //mark as 'rechargable' for ITH_RECHARGE
	}
#endif
	inventory[pos - 'a' + INVEN_WIELD].sval = sval;
	inventory[pos - 'a' + INVEN_WIELD].tval = tval;
	if ((tval == TV_BOOK && is_custom_tome(sval)) || tval == TV_SUBINVEN) /* ENABLE_SUBINVEN */
		inventory[pos - 'a' + INVEN_WIELD].bpval = pval;
	else inventory[pos - 'a' + INVEN_WIELD].pval = pval;
	inventory[pos - 'a' + INVEN_WIELD].name1 = name1;
	inventory[pos - 'a' + INVEN_WIELD].attr = attr;
	inventory[pos - 'a' + INVEN_WIELD].weight = wgt;
	inventory[pos - 'a' + INVEN_WIELD].number = amt;
	inventory[pos - 'a' + INVEN_WIELD].uses_dir = uses_dir & 0x1;
	inventory[pos - 'a' + INVEN_WIELD].ident = uses_dir & 0x6; //new hack in 4.7.1.2+ for ITH_ID/ITH_STARID
	equip_set[pos - 'a'] = (uses_dir & 0xF0) >> 4;
	inventory[pos - 'a' + INVEN_WIELD].iron_trade = uses_dir & 0x8;

#if defined(POWINS_DYNAMIC) && defined(POWINS_DYNAMIC_CLIENTSIDE)
	/* Strip "@&" markers, as these are a purely server-side thing */
	while ((insc = strstr(name, "@&"))) {
		strcpy(tmp, insc + 2);
		strcpy(insc, tmp);
	}
	/* Strip "@^" markers, as these are a purely server-side thing */
	while ((insc = strstr(name, "@&"))) {
		strcpy(tmp, insc + 2);
		strcpy(insc, tmp);
	}
#endif

	if (!strcmp(name, "(nothing)"))
		strcpy(inventory_name[pos - 'a' + INVEN_WIELD], equipment_slot_names[pos - 'a']);
	else
		strncpy(inventory_name[pos - 'a' + INVEN_WIELD], name, ONAME_LEN - 1);
	inventory_name[pos - 'a' + INVEN_WIELD][ONAME_LEN - 1] = 0;

	/* new hack for '(unavailable)' equipment slots: handle INVEN_ARM slot a bit cleaner: */
	if (pos == 'a') {
		if (!strcmp(name, "(unavailable)")) equip_no_weapon = TRUE;
		else equip_no_weapon = FALSE;
	} else if (pos == 'b' && equip_no_weapon && !strcmp(name, "(nothing)"))
		strcpy(inventory_name[INVEN_ARM], "(shield)");

	/* Window stuff */
	p_ptr->window |= (PW_EQUIP);

	return(1);
}

/* Added for WIELD_BOOKS */
int Receive_equip_wide(void) {
	int n;
	char ch;
	char pos, uses_dir;
	byte attr, tval, sval;
	s16b xtra1, xtra2, xtra3, xtra4, xtra5, xtra6, xtra7, xtra8, xtra9;
	s16b wgt, amt, pval, name1 = 0;
	char name[ONAME_LEN];
#if defined(POWINS_DYNAMIC) && defined(POWINS_DYNAMIC_CLIENTSIDE)
	char tmp[ONAME_LEN];
#endif

	if ((n = Packet_scanf(&rbuf, "%c%c%c%hu%hd%c%c%hd%hd%c%I%hd%hd%hd%hd%hd%hd%hd%hd%hd",
	    &ch, &pos, &attr, &wgt, &amt, &tval, &sval, &pval, &name1, &uses_dir, name,
	    &xtra1, &xtra2, &xtra3, &xtra4, &xtra5, &xtra6, &xtra7, &xtra8, &xtra9)) <= 0)
		return(n);

	/* Check that the equipment slot is valid - mikaelh */
	if (pos < 'a' || pos > 'n') return(0);

#ifdef MSTAFF_MDEV_COMBO
	if (tval == TV_MSTAFF && wgt >= 30000) {
		wgt -= 30000;
		inventory[pos - 'a' + INVEN_WIELD].xtra4 = 1; //mark as 'rechargable' for ITH_RECHARGE
	}
#endif
	inventory[pos - 'a' + INVEN_WIELD].sval = sval;
	inventory[pos - 'a' + INVEN_WIELD].tval = tval;
	if ((tval == TV_BOOK && is_custom_tome(sval)) || tval == TV_SUBINVEN) /* ENABLE_SUBINVEN */
		inventory[pos - 'a' + INVEN_WIELD].bpval = pval;
	else inventory[pos - 'a' + INVEN_WIELD].pval = pval;
	inventory[pos - 'a' + INVEN_WIELD].name1 = name1;
	inventory[pos - 'a' + INVEN_WIELD].attr = attr;
	inventory[pos - 'a' + INVEN_WIELD].weight = wgt;
	inventory[pos - 'a' + INVEN_WIELD].number = amt;
	inventory[pos - 'a' + INVEN_WIELD].uses_dir = uses_dir & 0x1;
	inventory[pos - 'a' + INVEN_WIELD].ident = uses_dir & 0x6; //new hack in 4.7.1.2+ for ITH_ID/ITH_STARID
	equip_set[pos - 'a'] = (uses_dir & 0xF0) >> 4;
	inventory[pos - 'a' + INVEN_WIELD].iron_trade = uses_dir & 0x8;

	inventory[pos - 'a' + INVEN_WIELD].xtra1 = xtra1;
	inventory[pos - 'a' + INVEN_WIELD].xtra2 = xtra2;
	inventory[pos - 'a' + INVEN_WIELD].xtra3 = xtra3;
	inventory[pos - 'a' + INVEN_WIELD].xtra4 = xtra4;
	inventory[pos - 'a' + INVEN_WIELD].xtra5 = xtra5;
	inventory[pos - 'a' + INVEN_WIELD].xtra6 = xtra6;
	inventory[pos - 'a' + INVEN_WIELD].xtra7 = xtra7;
	inventory[pos - 'a' + INVEN_WIELD].xtra8 = xtra8;
	inventory[pos - 'a' + INVEN_WIELD].xtra9 = xtra9;

#if defined(POWINS_DYNAMIC) && defined(POWINS_DYNAMIC_CLIENTSIDE)
	/* Strip "@&" markers, as these are a purely server-side thing */
	while ((insc = strstr(name, "@&"))) {
		strcpy(tmp, insc + 2);
		strcpy(insc, tmp);
	}
	/* Strip "@^" markers, as these are a purely server-side thing */
	while ((insc = strstr(name, "@&"))) {
		strcpy(tmp, insc + 2);
		strcpy(insc, tmp);
	}
#endif

	if (!strcmp(name, "(nothing)"))
		strcpy(inventory_name[pos - 'a' + INVEN_WIELD], equipment_slot_names[pos - 'a']);
	else
		strncpy(inventory_name[pos - 'a' + INVEN_WIELD], name, ONAME_LEN - 1);
	inventory_name[pos - 'a' + INVEN_WIELD][ONAME_LEN - 1] = 0;

	/* new hack for '(unavailable)' equipment slots: handle INVEN_ARM slot a bit cleaner: */
	if (pos == 'a') {
		if (!strcmp(name, "(unavailable)")) equip_no_weapon = TRUE;
		else equip_no_weapon = FALSE;
	} else if (pos == 'b' && equip_no_weapon && !strcmp(name, "(nothing)"))
		strcpy(inventory_name[INVEN_ARM], "(shield)");

	/* Window stuff */
	p_ptr->window |= (PW_EQUIP);

	return(1);
}

int Receive_char_info(void) {
	int n;
	char ch;

#ifndef RETRY_LOGIN
	static bool player_pref_files_loaded = FALSE;
#endif

	/* Clear any old info */
	race = class = trait = sex = mode = 0;
	lives = -1;

	if (is_atleast(&server_version, 4, 9, 2, 1, 0, 1)) {
		if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd%hd%d%hd%s", &ch, &race, &class, &trait, &sex, &mode, &lives, cname)) <= 0) return(n);
	} else if (is_atleast(&server_version, 4, 7, 3, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd%hd%hd%hd%s", &ch, &race, &class, &trait, &sex, &mode, &lives, cname)) <= 0) return(n);
	} else if (is_newer_than(&server_version, 4, 5, 2, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd%hd%hd%s", &ch, &race, &class, &trait, &sex, &mode, cname)) <= 0) return(n);
	} else if (is_newer_than(&server_version, 4, 4, 5, 10, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd%hd%hd", &ch, &race, &class, &trait, &sex, &mode)) <= 0) return(n);
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd%hd", &ch, &race, &class, &sex, &mode)) <= 0) return(n);
	}

	p_ptr->lives = lives;
	p_ptr->prace = race;
	p_ptr->rp_ptr = &race_info[race];
	p_ptr->pclass = class;
	p_ptr->cp_ptr = &class_info[class];
	p_ptr->ptrait = trait;
	p_ptr->tp_ptr = &trait_info[trait];
	p_ptr->male = sex;
	if (mode & MODE_FRUIT_BAT) p_ptr->fruit_bat = 1; else p_ptr->fruit_bat = 0; //track native fruit bat state
	/* New for 4.7.3: Transmit admin etc status */
	p_ptr->admin_wiz = p_ptr->admin_dm = FALSE;
	p_ptr->privileged = p_ptr->restricted = 0;
	if (is_atleast(&server_version, 4, 9, 2, 1, 0, 1)) {
		if (mode & MODE_ADMIN_WIZ) p_ptr->admin_wiz = TRUE;
		if (mode & MODE_ADMIN_DM) p_ptr->admin_dm = TRUE;
		if (mode & MODE_PRIVILEGED) p_ptr->privileged = 1;
		if (mode & MODE_VPRIVILEGED) p_ptr->privileged = 2;
		if (mode & MODE_RESTRICTED) p_ptr->restricted = 1;
		if (mode & MODE_VRESTRICTED) p_ptr->restricted = 2;
		p_ptr->mode = mode;
	} else {
		if (mode & 0x0400) p_ptr->admin_wiz = TRUE;
		if (mode & 0x0800) p_ptr->admin_dm = TRUE;
		if (mode & 0x1000) p_ptr->privileged = 1;
		if (mode & 0x2000) p_ptr->privileged = 2;
		if (mode & 0x4000) p_ptr->restricted = 1;
		if (mode & 0x8000) p_ptr->restricted = 2;
		/* Clear any special transmission hacks */
		p_ptr->mode = mode & 0xFF;
	}

	/* Load preferences once */
	if (!player_pref_files_loaded) {
		initialize_player_pref_files();
		player_pref_files_loaded = TRUE;

#ifndef GLOBAL_BIG_MAP
		global_big_map_hold = FALSE; //process BIG_MAP (option<7>) changes finally, all accumulated
		check_immediate_options(7, c_cfg.big_map, TRUE);
#endif

		/* Character Overview Resist/Boni/Abilities Page on Startup? - Kurzel */
		if (c_cfg.overview_startup) csheet_page = 2;

		/* hack: also load auto-inscriptions here */
		initialize_player_ins_files();

		/* Pref files may change settings, so reload the keymap - mikaelh */
		keymap_init();

		/* Disable font_map_solid_walls again (We already did at "Pre-initialize character-specific options",
		   but we re-initialized the prf files here, so we need to apply it again as font_map_solid_walls possibly got reset to 'yes'). */
		if (use_graphics && c_cfg.font_map_solid_walls && c_cfg.gfx_autooff_fmsw) {
			c_cfg.font_map_solid_walls = FALSE;
			(*option_info[CO_FONT_MAP_SOLID_WALLS].o_var) = FALSE;
			Client_setup.options[CO_FONT_MAP_SOLID_WALLS] = FALSE;
		}
	}

	if (screen_icky) Term_switch(0);
	prt_basic();
	if (screen_icky) Term_switch(0);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return(1);
}

int Receive_various(void) {
	int n;
	char ch, buf[MAX_CHARS];
	s16b hgt, wgt, age, sc;

	if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu%hu%s", &ch, &hgt, &wgt, &age, &sc, buf)) <= 0) return(n);

	p_ptr->ht = hgt;
	p_ptr->wt = wgt;
	p_ptr->age = age;
	p_ptr->sc = sc;

	if (c_cfg.load_form_macros && strcmp(c_p_ptr->body_name, buf)) {
		char tmp[MAX_CHARS];

		if (strcmp(buf, "Player")) sprintf(tmp, "%s%c%s.prf", cname, PRF_BODY_SEPARATOR, buf);
		else sprintf(tmp, "%s.prf", cname);
		(void)process_pref_file(tmp);
	}
	strcpy(c_p_ptr->body_name, buf);

	/*printf("Received various info: height %d, weight %d, age %d, sc %d\n", hgt, wgt, age, sc);*/

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return(1);
}

int Receive_plusses(void) {
	int n;
	char ch;
	s16b dam, hit;
	s16b dam_r, hit_r;
	s16b dam_m, hit_m;

	if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd%hd%hd%hd", &ch, &hit, &dam, &hit_r, &dam_r, &hit_m, &dam_m)) <= 0) return(n);

	p_ptr->dis_to_h = hit;
	p_ptr->dis_to_d = dam;
	p_ptr->to_h_ranged = hit_r;
	p_ptr->to_d_ranged = dam_r;
	p_ptr->to_h_melee = hit_m;
	p_ptr->to_d_melee = dam_m;

	/*printf("Received plusses: +%d tohit +%d todam\n", hit, dam);*/

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return(1);
}

int Receive_experience(void) {
	int n;
	char ch;
	s32b max, cur, adv, adv_prev;
	s16b lev, max_lev, max_plv;

	if (is_newer_than(&server_version, 4, 5, 6, 0, 0, 1)) {
		if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu%d%d%d%d", &ch, &lev, &max_lev, &max_plv, &max, &cur, &adv, &adv_prev)) <= 0) return(n);
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu%d%d%d", &ch, &lev, &max_lev, &max_plv, &max, &cur, &adv)) <= 0) return(n);
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

	return(1);
}

int Receive_skill_init(void) {
	int n;
	char ch;
	u16b i;
	s16b father, mkey, order;
	char name[MSG_LEN], desc[MSG_LEN], act[MSG_LEN];
	u32b flags1;
	byte tval;

	if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd%hd%d%c%S%S%S", &ch, &i,
	    &father, &order, &mkey, &flags1, &tval, name, desc, act)) <= 0) return(n);

#ifdef RETRY_LOGIN
	/* Try to free up previously allocated memory */
	if (s_info[i].name) string_free(s_info[i].name);
	if (s_info[i].desc) string_free(s_info[i].desc);
	if (s_info[i].action_desc) string_free(s_info[i].action_desc);
#endif

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

	return(1);
}

int Receive_skill_points(void) {
	int n;
	char ch;
	int pt;

	if ((n = Packet_scanf(&rbuf, "%c%d", &ch, &pt)) <= 0) return(n);

	p_ptr->skill_points = pt;

	/* Redraw the skill menu */
	redraw_skills = TRUE;

	return(1);
}

int Receive_skill_info(void) {
	int n;
	char ch;
	s32b val;
	int i, mod, dev, mkey;
	byte flags1;

	if (is_newer_than(&server_version, 4, 4, 4, 1, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%d%d%d%d%c%d", &ch, &i, &val, &mod, &dev, &flags1, &mkey)) <= 0) return(n);
	} else {
		int hidden, dummy;

		if ((n = Packet_scanf(&rbuf, "%c%d%d%d%d%d%d%d", &ch, &i, &val, &mod, &dev, &hidden, &mkey, &dummy)) <= 0) return(n);

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

	return(1);
}

int Receive_gold(void) {
	int n;
	char ch;
	int gold, balance;

	if ((n = Packet_scanf(&rbuf, "%c%d%d", &ch, &gold, &balance)) <= 0) return(n);

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

	return(1);
}

int Receive_mp(void) {
	int n;
	char ch;
	s16b max, cur;
	bool bar = FALSE;

	if ((n = Packet_scanf(&rbuf, "%c%hd%hd", &ch, &max, &cur)) <= 0) return(n);

	/* Display hack */
	if (max > 10000) {
		max -= 10000;
		bar = TRUE;
	}
	/* .. and new, clean way: It's a client option now */
	if (c_cfg.mp_bar) bar = TRUE;

	if (c_cfg.alert_mana && max != -9999 && (cur < max / 5)) {
		warning_page();
		c_msg_print("\377R*** LOW MANA WARNING! ***");
	}

	p_ptr->mmp = max;
	p_ptr->cmp = cur;

	if (screen_icky) Term_switch(0);
	prt_mp(max, cur, bar);
	draw_huge_bar(0, &prev_huge_cmp, cur, &prev_huge_mmp, max);
	if (screen_icky) Term_switch(0);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return(1);
}

int Receive_history(void) {
	int n;
	char ch;
	s16b line;
	char buf[MAX_CHARS];

	if ((n = Packet_scanf(&rbuf, "%c%hu%s", &ch, &line, buf)) <= 0) return(n);

	strcpy(p_ptr->history[line], buf);

	/*printf("Received history line %d: %s\n", line, buf);*/

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return(1);
}

int Receive_char(void) {
//DYNAMIC_CLONE_MAP: handle minimap-specific chars, via new PKT_ type probably instead of here (or combine PKT_CHAR and new PKT_ type into this function)
	int n;
	unsigned char ch;
	char x, y;
	byte a;
	char32_t c = 0; /* Needs to be initialized for proper packet read. */
#ifdef GRAPHICS_BG_MASK
	byte a_back;
	char32_t c_back = 0;
#endif
	bool is_us = FALSE;

#ifdef GRAPHICS_BG_MASK
	if (use_graphics == UG_2MASK && is_atleast(&server_version, 4, 9, 2, 1, 0, 0)) {
		/* Transfer only minimum number of bytes needed, according to client setup.*/
		char *pc = (char *)&c, *pc_b = (char *)&c_back;

		switch (Client_setup.char_transfer_bytes) {
		case 0:
		case 1:
			if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c%c%c", &ch, &x, &y, &a, &pc[0], &a_back, &pc_b[0])) <= 0) return(n);
			break;
		case 2:
			if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c%c%c%c%c", &ch, &x, &y, &a, &pc[1], &pc[0], &a_back, &pc_b[1], &pc_b[0])) <= 0) return(n);
			break;
		case 3:
			if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c%c%c%c%c%c%c", &ch, &x, &y, &a, &pc[2], &pc[1], &pc[0], &a_back, &pc_b[2], &pc_b[1], &pc_b[0])) <= 0) return(n);
			break;
		case 4:
		default:
			if ((n = Packet_scanf(&rbuf, "%c%c%c%c%u%c%u", &ch, &x, &y, &a, &c, &a_back, &c_back)) <= 0) return(n);
		}
	} else
#endif
	/* 4.8.1 and newer servers communicate using 32bit character size. */
	if (is_atleast(&server_version, 4, 8, 1, 0, 0, 0)) {
		/* Transfer only minimum number of bytes needed, according to client setup.*/
		char *pc = (char *)&c;

		switch (Client_setup.char_transfer_bytes) {
		case 0:
		case 1:
			if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c", &ch, &x, &y, &a, &pc[0])) <= 0) return(n);
			break;
		case 2:
			if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c%c", &ch, &x, &y, &a, &pc[1], &pc[0])) <= 0) return(n);
			break;
		case 3:
			if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c%c%c", &ch, &x, &y, &a, &pc[2], &pc[1], &pc[0])) <= 0) return(n);
			break;
		case 4:
		default:
			if ((n = Packet_scanf(&rbuf, "%c%c%c%c%u", &ch, &x, &y, &a, &c)) <= 0) return(n);
		}
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c", &ch, &x, &y, &a, &c)) <= 0) return(n);
	}

	/* Old cfg.hilite_player implementation has been disabled after 4.6.1.1 because it interferes with custom fonts */
#if 0
	if (!is_newer_than(&server_version, 4, 6, 1, 1, 0, 1)) {
		if ((c & 0x80)) {
			c &= 0x7F;
			is_us = TRUE;
		}
	}
#else
	if (is_atleast(&server_version, 4, 7, 3, 0, 0, 0)) {
		if ((a & TERM_HILITE_PLAYER)) {
			a &= ~TERM_HILITE_PLAYER;
			is_us = TRUE;
		}
	}
#endif

#ifdef TEST_CLIENT
	/* special hack for mind-link Windows->Linux w/ font_map_solid_walls */
	/* NOTE: We need a better solution than this for custom fonts... */
	if (force_cui) {
		if (c == FONT_MAP_SOLID_X11 || c == FONT_MAP_SOLID_WIN) c = '#';
		if (c == FONT_MAP_VEIN_X11 || c == FONT_MAP_VEIN_WIN) c = '*';
	}
 #ifdef USE_X11
	if (c == FONT_MAP_SOLID_WIN) c = FONT_MAP_SOLID_X11;
	if (c == FONT_MAP_VEIN_WIN) c = FONT_MAP_VEIN_X11;
 #elif defined(WINDOWS)
	if (c == FONT_MAP_SOLID_X11) c = FONT_MAP_SOLID_WIN;
	if (c == FONT_MAP_VEIN_X11) c = FONT_MAP_VEIN_WIN;
 #else /* command-line client doesn't draw either! */
	if (c == FONT_MAP_SOLID_X11 || c == FONT_MAP_SOLID_WIN) c = '#';
	if (c == FONT_MAP_VEIN_X11 || c == FONT_MAP_VEIN_WIN) c = '*';
 #endif
#endif
#ifdef GRAPHICS_BG_MASK
 #ifdef TEST_CLIENT
	/* special hack for mind-link Windows->Linux w/ font_map_solid_walls */
	/* NOTE: We need a better solution than this for custom fonts... */
	if (force_cui) {
		if (c_back == FONT_MAP_SOLID_X11 || c == FONT_MAP_SOLID_WIN) c_back = '#';
		if (c_back == FONT_MAP_VEIN_X11 || c == FONT_MAP_VEIN_WIN) c_back = '*';
	}
  #ifdef USE_X11
	if (c_back == FONT_MAP_SOLID_WIN) c_back = FONT_MAP_SOLID_X11;
	if (c_back == FONT_MAP_VEIN_WIN) c_back = FONT_MAP_VEIN_X11;
  #elif defined(WINDOWS)
	if (c_back == FONT_MAP_SOLID_X11) c_back = FONT_MAP_SOLID_WIN;
	if (c_back == FONT_MAP_VEIN_X11) c_back = FONT_MAP_VEIN_WIN;
  #else /* command-line client doesn't draw either! */
	if (c_back == FONT_MAP_SOLID_X11 || c_back == FONT_MAP_SOLID_WIN) c = '#';
	if (c_back == FONT_MAP_VEIN_X11 || c_back == FONT_MAP_VEIN_WIN) c = '*';
  #endif
 #endif
#endif

	if (ch != PKT_CHAR_DIRECT) {
		/* remember map_info in client-side buffer */
		if (x >= PANEL_X && x < PANEL_X + screen_wid &&
		    y >= PANEL_Y && y < PANEL_Y + screen_hgt) {
			panel_map_a[x - PANEL_X][y - PANEL_Y] = a;
			panel_map_c[x - PANEL_X][y - PANEL_Y] = c;
#ifdef GRAPHICS_BG_MASK
			/* Catch if the server didn't define a valid background ie sent a zero -
			   in that case instead of bugging out the display, interpret it as 'keep our old background' */
			if (c_back != 0) {
				panel_map_a_back[x - PANEL_X][y - PANEL_Y] = a_back;
				panel_map_c_back[x - PANEL_X][y - PANEL_Y] = c_back;
			} else {
				c_back = panel_map_c_back[x - PANEL_X][y - PANEL_Y];
				a_back = panel_map_a_back[x - PANEL_X][y - PANEL_Y];
 #if 0
				if (!c_back) c_back = panel_map_c_back[x - PANEL_X][y - PANEL_Y] = 32;
 #else
				if (!c_back) c_back = panel_map_c_back[x - PANEL_X][y - PANEL_Y] = Client_setup.f_char[FEAT_SOLID];
 #endif
			}
#endif
		}

		if (screen_icky) Term_switch(0);
	}

	if (is_us && c_cfg.hilite_player) {
		/* Mark our own position via special cursor */
#if 0
		/* save previous cursor vis/loc? -- not necessary? */
		Term_get_cursor(&v);
		Term_locate(&x, &y);
#endif
		//for gcu: Term_xtra(TERM_XTRA_SHAPE, 2);

#if defined(TEST_CLIENT) && defined(EXTENDED_BG_COLOURS)
		/* Highlight player by using a background colour */
		a = TERMX_YELLOW;
#else
		/* Highlight player by placing the cursor under/around him */
		Term_set_cursor(1);
		Term_gotoxy(x, y);
#endif

		/* for X11:
		Term_curs_x11() ->
		Infofnt_text_non() : draw cursor XRectangle
		*/
	}

#ifdef GRAPHICS_BG_MASK
	if (use_graphics == UG_2MASK)
		Term_draw_2mask(x, y, a, c, a_back, c_back);
	else
#endif
	Term_draw(x, y, a, c);

	if (screen_icky && ch != PKT_CHAR_DIRECT) Term_switch(0);
	return(1);
}

int Receive_message(void) {
	int n, c;
	char ch;
	char buf[MSG_LEN], *bptr, *sptr, *bnptr;
	char l_buf[MSG_LEN], l_cname[NAME_LEN], *ptr, l_nick[NAME_LEN], called_name[NAME_LEN];
	static bool got_note = FALSE;

	if ((n = Packet_scanf(&rbuf, "%c%S", &ch, buf)) <= 0) return(n);

	/* Ultra-hack for light-source fainting. (First two bytes are "\377w".) */
	if (!c_cfg.no_lite_fainting && !strcmp(buf + 2, HCMSG_LIGHT_FAINT)) lamp_fainting = 30; //deciseconds

	/* Hack for tombstone music from insanity-deaths. (First four bytes are "\377w\377v".) */
	if (!strcmp(buf + 4, HCMSG_VEGETABLE)) insanity_death = TRUE;

	/* Hack for storing private messages to disk, in case we miss them; handle multi-line messages (subsequent lines start on a space) */
	if (!strncmp(buf + 6, HCMSG_NOTE, 10) || (got_note && buf[2] == ' ')) {
		FILE *fp;
		char path[1024];
		time_t ct = time(NULL);
		struct tm* ctl = localtime(&ct);

		path_build(path, 1024, ANGBAND_DIR_USER, format("notes-%s.txt", nick));
		fp = fopen(path, "a");
		if (fp) {
			char buf2[MSG_LEN] = { 0 }, *c = buf + (got_note ? 2 : 6 + 10), *c2 = buf2;

			while (*c) {
				switch (*c) {
				/* skip special markers */
				case '\376':
				case '\375':
				case '\374':
					c++;
					continue;
				/* strip colour codes */
				case '\377':
					switch (*(c + 1)) {
					case 0: /* broken colour code (paranoia) */
						c++;
						continue;
					default: /* assume colour code and discard */
						c += 2;
						continue;
					}
					break;
				}
				*c2 = *c;
				c++;
				c2++;
			}

			fprintf(fp, "[%04d/%02d/%02d - %02d:%02d] %s\n", 1900 + ctl->tm_year, ctl->tm_mon + 1, ctl->tm_mday, ctl->tm_hour, ctl->tm_min, buf2);
			fclose(fp);
		}
		got_note = TRUE;
	} else got_note = FALSE;

	/* Hack to clear topline: It's a translation of the former msg_print(Ind, NULL) hack, as we cannot transmit the NULL. */
	if (buf[0] == '\377' && !buf[1]) {
		if (screen_icky && (!shopping || perusing)) Term_switch(0);
		c_msg_print(NULL);
		if (screen_icky && (!shopping || perusing)) Term_switch(0);
		return(1);
	}

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
			return(1);

	if (screen_icky && (!shopping || perusing)) Term_switch(0);


	/* Highlight or beep on incoming chat messages containing our name?
	   Current weakness: Char 'Mim' (acc 'Test) won't get highlights from
	   another char 'Test' (acc 'Test') writing 'Mim' in chat. - C. Blue */
	if (c_cfg.hilite_chat || c_cfg.hibeep_chat) { /* enabled? */
		/* Test sender's name, if it is us */
		int we_sent_offset;
		char *we_sent_p = strchr(buf, '[');

		if (we_sent_p) {
			char we_sent_buf[NAME_LEN + 1 + 10], *we_sent_p_end;

			strncpy(we_sent_buf, we_sent_p + 1, NAME_LEN + 1 + 10);
			we_sent_buf[NAME_LEN + 10] = '\0';
			if ((we_sent_p_end = strchr(we_sent_buf, ']'))
			    && we_sent_p_end - we_sent_p <= NAME_LEN /* Prevent buffer overflow if the [...] wasn't a name but some longer text that was just within brackets for some reason */
			    ) {
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
#ifdef CHARNAME_ROMAN /* Convenience feature: Ignore roman numbers at the end of our character name? (Must be separated by a space.) */
			if ((ptr = roman_suffix(l_cname))) *(ptr - 1) = 0;
#endif
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
#ifndef CHARNAME_ROMAN
					strncpy(called_name, bptr, strlen(cname));
					called_name[strlen(cname)] = 0;
#else
					strncpy(called_name, bptr, strlen(l_cname));
					called_name[strlen(l_cname)] = 0;
#endif
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

	/* For the casino: Cursor was visible after game result message (won/loss) */
	if (shopping) {
		/* hack: hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;
	}

	return(1);
}

int Receive_state(void) {
	int n;
	char ch;
	s16b paralyzed, searching, resting;

	if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu", &ch, &paralyzed, &searching, &resting)) <= 0) return(n);

	if (screen_icky) Term_switch(0);
	prt_state(paralyzed, searching, resting);
	if (screen_icky) Term_switch(0);
	return(1);
}

int Receive_title(void) {
	int n;
	char ch;
	char buf[MAX_CHARS];

#ifdef BIGMAP_MINDLINK_HACK
	/* hack for big_map visuals: are we on a bigger display linked to a smaller display?
	   hack explanation: mindlinking sends both PR_MAP and PR_TITLE, and title is sent
	   _after_ map, so we can track the amount of map lines we received in Receive_line_info()
	   first and then do the final check and visuals here. */
	//if (last_line_y <= SCREEN_HGT && screen_hgt == MAX_SCREEN_HGT && !screen_icky) {
	if (last_line_y <= SCREEN_HGT && screen_hgt == MAX_SCREEN_HGT) {
		if (screen_icky) Term_switch(0);
		/* black out the unused part for better visual quality */
		for (n = 1 + SCREEN_HGT; n < 1 + SCREEN_HGT * 2; n++)
			Term_erase(SCREEN_PAD_LEFT, n, 255);
		/* Minor visual hack in case hilite_player was enabled */
		Term_set_cursor(0);
		if (screen_icky) Term_switch(0);
	}
#endif

	if ((n = Packet_scanf(&rbuf, "%c%s", &ch, buf)) <= 0) return(n);

	/* XXX -- Extract "ghost-ness" */
	p_ptr->ghost = streq(buf, "Ghost") || streq(buf, "\377rGhost (dead)");
	/* extract winner state */
	p_ptr->total_winner = strstr(buf, "**") || strstr(buf, "\377v");

	if (screen_icky) Term_switch(0);
	prt_title(buf);
	if (screen_icky) Term_switch(0);
	return(1);
}

int Receive_depth(void) {
	int n;
	char ch;
	s16b x, y, z, old_colour;
	char colour, colour_sector;
	bool town;
	char buf[MAX_CHARS];

	/* Compatibility with older servers */
	if (is_newer_than(&server_version, 4, 6, 1, 2, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu%c%c%c%s%s%s", &ch, &x, &y, &z, &town, &colour, &colour_sector, buf, location_name2, location_pre)) <= 0)
			return(n);
	} else if (is_newer_than(&server_version, 4, 5, 9, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu%c%c%c%s%s", &ch, &x, &y, &z, &town, &colour, &colour_sector, buf, location_name2)) <= 0)
			return(n);
		if (location_name2[0]) strcpy(location_pre, "in");
		else strcpy(location_pre, "of");
	} else if (is_newer_than(&server_version, 4, 4, 1, 6, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu%c%c%c%s", &ch, &x, &y, &z, &town, &colour, &colour_sector, buf)) <= 0)
			return(n);
		if (buf[0]) strcpy(location_pre, "in");
		else strcpy(location_pre, "of");
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu%c%hu%s", &ch, &x, &y, &z, &town, &old_colour, buf)) <= 0)
			return(n);
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

	/* For mini-map visual hacky fix.. -_- */
	map_town = town;

	return(1);
}

int Receive_confused(void) {
	int n;
	char ch;
	bool confused;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &confused)) <= 0) return(n);

	if (screen_icky) Term_switch(0);
	prt_confused(confused);
	if (screen_icky) Term_switch(0);

	/* We need this for client-side spell chance calc: */
	p_ptr->confused = confused;

	return(1);
}

int Receive_poison(void) {
	int n;
	char ch, poison;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &poison)) <= 0) return(n);

	if (screen_icky) Term_switch(0);
	prt_poisoned(poison);
	if (screen_icky) Term_switch(0);
	return(1);
}

int Receive_study(void) {
	int n;
	char ch;
	bool study;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &study)) <= 0) return(n);

	if (screen_icky) Term_switch(0);
	prt_study(study);
	if (screen_icky) Term_switch(0);
	return(1);
}

int Receive_bpr_wraith_prob(void) {
	int n;
	char ch, bpr_str[20];
	byte bpr, attr;

	if (is_older_than(&server_version, 4, 9, 1, 0, 0, 1)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c", &ch, &bpr, &attr)) <= 0) return(n);
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%s", &ch, &bpr, &attr, bpr_str)) <= 0) return(n);
	}

	if (screen_icky) Term_switch(0);
	prt_bpr_wraith_prob(bpr, attr, bpr_str);
	if (screen_icky) Term_switch(0);
	return(1);
}

int Receive_food(void) {
	int n;
	char ch;
	u16b food;

	if ((n = Packet_scanf(&rbuf, "%c%hu", &ch, &food)) <= 0) return(n);

	if (screen_icky) Term_switch(0);
	prt_hunger(food);
	if (screen_icky) Term_switch(0);
	return(1);
}

int Receive_fear(void) {
	int n;
	char ch;
	bool afraid;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &afraid)) <= 0) return(n);

	if (screen_icky) Term_switch(0);
	prt_afraid(afraid);
	if (screen_icky) Term_switch(0);
	return(1);
}

int Receive_speed(void) {
	int n;
	char ch;
	s16b speed;

	if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &speed)) <= 0) return(n);

	if (screen_icky) Term_switch(0);
	prt_speed(speed);
	if (screen_icky) Term_switch(0);
	return(1);
}

int Receive_cut(void) {
	int n;
	char ch;
	s16b cut;

	if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &cut)) <= 0) return(n);

	if (screen_icky) Term_switch(0);
	prt_cut(cut);
	if (screen_icky) Term_switch(0);
	return(1);
}

int Receive_blind_hallu(void) {
	int n;
	char ch;
	char blind_hallu;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &blind_hallu)) <= 0) return(n);

	if (screen_icky) Term_switch(0);
	prt_blind_hallu(blind_hallu);
	if (screen_icky) Term_switch(0);

	/* We need this for client-side spell chance calc: */
	p_ptr->blind = (blind_hallu && blind_hallu != 2);
	p_ptr->image = (blind_hallu & 0x2); //not needed, but just because we can

	return(1);
}

int Receive_stun(void) {
	int n;
	char ch;
	s16b stun;

	if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &stun)) <= 0) return(n);
	p_ptr->stun = stun;

	if (screen_icky) Term_switch(0);
	prt_stun(stun);
	if (screen_icky) Term_switch(0);

	/* We need this for client-side spell chance calc: */
	p_ptr->stun = stun;

	return(1);
}

int Receive_item(void) {
	char ch, th = ITH_NONE;
	int n, item;

	if (is_newer_than(&server_version, 4, 5, 2, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &th)) <= 0) return(n);
	} else {
		if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return(n);
	}

	if ((!screen_icky && !topline_icky)
#ifdef ENABLE_SUBINVEN
	    || using_subinven_item != -1
#endif
	    ) {
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
			item_tester_hook = item_tester_hook_nonart_armour;
			get_item_hook_find_obj_what = "Armour name? ";
			break;
		case ITH_ENCH_WEAP:
			get_item_extra_hook = get_item_hook_find_obj;
			item_tester_hook = item_tester_hook_nonart_weapon;
			get_item_hook_find_obj_what = "Weapon name? ";
			break;
		case ITH_CUSTOM_TOME:
			get_item_extra_hook = get_item_hook_find_obj;
			item_tester_hook = item_tester_hook_custom_tome;
			get_item_hook_find_obj_what = "Book name? ";
			break;
		case ITH_RUNE_ENCHANT: // Request an allowed item when activating a rune.
			get_item_extra_hook = get_item_hook_find_obj;
			item_tester_hook = item_tester_hook_rune_enchant;
			get_item_hook_find_obj_what = "Which item? ";
			clear_topline(); // Hack: EQUIP ONLY - Kurzel
			if (!c_get_item(&item, "Which item? ", USE_EQUIP)) return(1);
			Send_item(item);
			return(1);
			break;
		case ITH_ENCH_AC_NO_SHIELD:
			get_item_extra_hook = get_item_hook_find_obj;
			item_tester_hook = item_tester_hook_nonart_armour_no_shield;
			get_item_hook_find_obj_what = "Armour name? ";
			break;
		case ITH_ID:
			get_item_extra_hook = get_item_hook_find_obj;
			item_tester_hook = item_tester_hook_id;
			get_item_hook_find_obj_what = "Which item? ";
			break;
		case ITH_STARID:
			get_item_extra_hook = get_item_hook_find_obj;
			item_tester_hook = item_tester_hook_starid;
			get_item_hook_find_obj_what = "Which item? ";
			break;
		case ITH_CHEMICAL: //ENABLE_DEMOLITIONIST
			get_item_extra_hook = get_item_hook_find_obj;
			item_tester_hook = item_tester_hook_chemical;
			get_item_hook_find_obj_what = "Which ingredient? ";
			break;
		case ITH_ARMOUR:
			get_item_extra_hook = get_item_hook_find_obj;
			item_tester_hook = item_tester_hook_armour;
			get_item_hook_find_obj_what = "Armour name? ";
			break;
		case ITH_ARMOUR_NO_SHIELD:
			get_item_extra_hook = get_item_hook_find_obj;
			item_tester_hook = item_tester_hook_armour_no_shield;
			get_item_hook_find_obj_what = "Armour name? ";
			break;
		case ITH_WEAPON:
			get_item_extra_hook = get_item_hook_find_obj;
			item_tester_hook = item_tester_hook_weapon;
			get_item_hook_find_obj_what = "Weapon name? ";
			break;
		}

		clear_topline();
#ifdef ENABLE_SUBINVEN
		if (using_subinven == -1) {
			if (!c_get_item(&item, "Which item? ", (USE_EQUIP | USE_INVEN | USE_EXTRA | USE_SUBINVEN))) return(1);
		} else
#endif
		if (!c_get_item(&item, "Which item? ", (USE_EQUIP | USE_INVEN | USE_EXTRA))) return(1);
		Send_item(item);
	} else {
		if (is_newer_than(&server_version, 4, 5, 2, 0, 0, 0)) {
			if ((n = Packet_printf(&qbuf, "%c%c", ch, th)) <= 0) return(n);
		} else {
			if ((n = Packet_printf(&qbuf, "%c", ch)) <= 0) return(n);
		}
	}
	return(1);
}

/* for DISCRETE_SPELL_SYSTEM: DSS_EXPANDED_SCROLLS */
int Receive_spell_request(void) {
	char ch;
	int n, item, spell;

	if ((n = Packet_scanf(&rbuf, "%c%d", &ch, &item)) <= 0) return(n);

	if (!screen_icky && !topline_icky) {
		clear_topline();
		/* Ask for a spell, allow cancel */
		if ((spell = get_school_spell("choose", &item)) == -1) return(1);
		Send_spell(item, spell);
	} else {
		if ((n = Packet_printf(&qbuf, "%c%d", ch, item)) <= 0) return(n);
	}
	return(1);
}

int Receive_spell_info(void) { /* deprecated - doesn't transmit RF0_ powers either. See Receive_powers_info(). */
	char ch;
	int n;
	u16b realm, book, line;
	char buf[MAX_CHARS];
	s32b spells[3];

	if ((n = Packet_scanf(&rbuf, "%c%d%d%d%hu%hu%hu%s", &ch, &spells[0], &spells[1], &spells[2], &realm, &book, &line, buf)) <= 0) return(n);

	/* Save the info */
	strcpy(spell_info[realm][book][line], buf);
	p_ptr->innate_spells[0] = spells[0];
	p_ptr->innate_spells[1] = spells[1];
	p_ptr->innate_spells[2] = spells[2];

	/* Assume that this was in response to a shapeshift command we issued */
	command_confirmed = PKT_ACTIVATE_SKILL;

	return(1);
}

int Receive_powers_info(void) {
	char ch;
	int n;
	s32b spells[4];

	if ((n = Packet_scanf(&rbuf, "%c%d%d%d%d", &ch, &spells[0], &spells[1], &spells[2], &spells[3])) <= 0) return(n);

	/* Save the info */
	p_ptr->innate_spells[0] = spells[0];
	p_ptr->innate_spells[1] = spells[1];
	p_ptr->innate_spells[2] = spells[2];
	p_ptr->innate_spells[3] = spells[3];

	/* Assume that this was in response to a shapeshift command we issued */
	command_confirmed = PKT_ACTIVATE_SKILL;

	return(1);
}

int Receive_technique_info(void) {
	char ch;
	int n;
	s32b melee, ranged;

	if ((n = Packet_scanf(&rbuf, "%c%d%d", &ch, &melee, &ranged)) <= 0) return(n);

	/* Save the info */
	p_ptr->melee_techniques = melee;
	p_ptr->ranged_techniques = ranged;

	return(1);
}

int Receive_direction(void) {
	char ch;
	int n, dir = 0;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return(n);

	if (!screen_icky && !topline_icky && !shopping) {
		/* Ask for a direction */
		if (!get_dir(&dir)) return(0);

		/* Send it back */
		if ((n = Packet_printf(&wbuf, "%c%c", PKT_DIRECTION, dir)) <= 0) return(n);
	} else if ((n = Packet_printf(&qbuf, "%c", ch)) <= 0) return(n);

	return(1);
}

int Receive_flush(void) {
	char ch;
	int n;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return(n);

	/* Flush the terminal */
	Term_fresh();

	/* Disable the delays altogether? */
	if (c_cfg.disable_flush) return(1);

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
		if (++flush_count > 10) return(1);
	}

	Term_xtra(TERM_XTRA_DELAY, 1);
#endif

	return(1);
}


int Receive_line_info(void) {
//DYNAMIC_CLONE_MAP: handle minimap-specific lines, via new PKT_MINI_MAP type (while !screen_icky) probably
	char ch;
	int x, i, n;
	s16b y;
	char32_t c;
	byte a;
#ifdef GRAPHICS_BG_MASK
	char32_t c_back, c_back_real;
	byte a_back;
#endif
	byte rep;
	bool draw = TRUE;
	char *stored_sbuf_ptr = rbuf.ptr;

	if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &y)) <= 0) return(n);

	/* Bad hack to fix the timing issue of sometimes not loading a custom font's prf file: */
	if (fix_custom_font_after_startup) {
		fix_custom_font_after_startup = FALSE;
		handle_process_font_file();
	}

	if (screen_icky && ch != PKT_MINI_MAP) Term_switch(0);

	/* If this is the mini-map, discard package if we already left the minimap screen again!
	   This would otherwise cause a visua glitch, and can happen if we exit faster than server latency! */
	if (ch == PKT_MINI_MAP && !local_map_active) {
		if (y == -1) return(1);
		draw = FALSE;
	}

	/* Hack: -1 marks minimap transmission as complete, so we can start adding some extra info to it, such as coordinates. */
	if (ch == PKT_MINI_MAP && y == -1) {
		/* Deduce the currently selected world coords from our worldpos and the currently (client-side) selected grid.. */
		if (screen_icky && minimap_posx != -1) {
			int n = 1;

			Term_putstr(0, n++, -1, TERM_L_WHITE, "=World=");
			Term_putstr(0, n++, -1, TERM_L_WHITE, "  Map");
			n += 2;

			Term_putstr(0, n++, -1, TERM_L_WHITE, " Your");
			Term_putstr(0, n++, -1, TERM_L_WHITE, "Sector:");
			Term_putstr(0, n++, -1, TERM_WHITE, format("(%2d,%2d)", p_ptr->wpos.wx, p_ptr->wpos.wy));
			n++;

#if 0 /* Note: We draw all this stuff instead in cmd_mini_map(). minimap_selx is -1 here initially, unlike in cmd_mini_map() (!) */
			if (minimap_selx != -1) {
				int wx, wy;
				int dis;

				wx = p_ptr->wpos.wx + minimap_selx - minimap_posx;
				wy = p_ptr->wpos.wy - minimap_sely + minimap_posy;
				Term_putstr(0, n++, -1, TERM_WHITE, "\377o Select");
				Term_putstr(0, n++, -1, TERM_WHITE, format("(%2d,%2d)", wx, wy));
				Term_putstr(0, n++, -1, TERM_WHITE, format("%+3d,%+3d", wx - p_ptr->wpos.wx, wy - p_ptr->wpos.wy));
				/* RECALL_MAX_RANGE uses distance() function, so we do the same:
				   It was originally defined as 16, but has like ever been 24, so we just colourize accordingly to catch 'em all.. */
				dis = distance(wx, wy, p_ptr->wpos.wx, p_ptr->wpos.wy);
				Term_putstr(0, n++, -1, TERM_WHITE, format("Dist:\377%c%2d", dis <= 16 ? 'w' : (dis <= 24 ? 'y' : 'o'), dis));
			} else {
				Term_putstr(0, n++, -1, TERM_WHITE, "        ");
				Term_putstr(0, n++, -1, TERM_WHITE, "        ");
				Term_putstr(0, n++, -1, TERM_WHITE, "        ");
				Term_putstr(0, n++, -1, TERM_WHITE, "        ");
			}
#elif defined(WILDMAP_ALLOW_SELECTOR_SCROLLING)
c_msg_format("RLI minimap_selx=%d", minimap_selx);
			if (minimap_selx != -1) {
				int wx, wy;
				int dis;
				int yoff = p_ptr->wpos.wy - minimap_yoff; // = the 'y' world coordinate of the center point of the minimap

				wy = yoff - 44 / 2 + 44 - minimap_sely; //44 = height of displayed map, with y_off being the wpos at the _center point_ of the displayed map
				wx = p_ptr->wpos.wx + minimap_selx - minimap_posx;
				//wy = p_ptr->wpos.wy - minimap_sely + minimap_posy;
c_msg_format("RLI wx,wy=%d,%d; mmsx,mmsy=%d,%d, mmpx,mmpy=%d,%d, y_offset=%d", p_ptr->wpos.wx, p_ptr->wpos.wy, minimap_selx, minimap_sely, minimap_posx, minimap_posy, minimap_yoff);

				Term_putstr(0, n++, -1, TERM_WHITE, "\377o Select");
				Term_putstr(0, n++, -1, TERM_WHITE, format("(%2d,%2d)", wx, wy));
				Term_putstr(0, n++, -1, TERM_WHITE, format("%+3d,%+3d", wx - p_ptr->wpos.wx, wy - p_ptr->wpos.wy));
				/* RECALL_MAX_RANGE uses distance() function, so we do the same:
				   It was originally defined as 16, but has like ever been 24, so we just colourize accordingly to catch 'em all.. */
				dis = distance(wx, wy, p_ptr->wpos.wx, p_ptr->wpos.wy);
				Term_putstr(0, n++, -1, TERM_WHITE, format("Dist:\377%c%2d", dis <= 16 ? 'w' : (dis <= 24 ? 'y' : 'o'), dis));
			} else {
				Term_putstr(0, n++, -1, TERM_WHITE, "        ");
				Term_putstr(0, n++, -1, TERM_WHITE, "        ");
				Term_putstr(0, n++, -1, TERM_WHITE, "        ");
				Term_putstr(0, n++, -1, TERM_WHITE, "        ");
			}
#else
//c_msg_format("RLI: wx,wy=%d,%d; mmsx,mmsy=%d,%d, mmpx,mmpy=%d,%d", p_ptr->wpos.wx, p_ptr->wpos.wy, minimap_selx, minimap_sely, minimap_posx, minimap_posy);
			n += 4;
#endif
			n += 2;

			Term_putstr(0, n++, -1, TERM_WHITE, "\377ys\377w+\377ydir\377w:");
			Term_putstr(0, n++, -1, TERM_WHITE, " Select");
			n++;
			Term_putstr(0, n++, -1, TERM_WHITE, "\377yr\377w/\377y5\377w:");
			Term_putstr(0, n++, -1, TERM_WHITE, " Reset");
			n++;
			Term_putstr(0, n++, -1, TERM_WHITE, "\377yESC\377w:");
			Term_putstr(0, n++, -1, TERM_WHITE, " Exit");
			n += 2;

			/* Specialty: Only display map key if in bigmap mode.
			   (Note: This is the only check of this kind currently) */
			if (screen_hgt == MAX_SCREEN_HGT) { //(c_cfg.big_map) {
				Term_putstr(0, n++, -1, TERM_WHITE, "\377WLegend:");
				Term_putstr(0, n++, -1, TERM_WHITE, "@ You");
				Term_putstr(0, n++, -1, TERM_WHITE, "\377yT\377w Town");
				Term_putstr(0, n++, -1, TERM_WHITE, "> Dung.");
				Term_putstr(0, n++, -1, TERM_WHITE, "< Tower");
				Term_putstr(0, n++, -1, TERM_WHITE, "\377u.\377w Waste");
				Term_putstr(0, n++, -1, TERM_WHITE, "\377g.\377w Grass");
				Term_putstr(0, n++, -1, TERM_WHITE, "\377g*\377w Woods");
				Term_putstr(0, n++, -1, TERM_WHITE, "\377g#\377w Thick");
				Term_putstr(0, n++, -1, TERM_WHITE, "  woods");
				Term_putstr(0, n++, -1, TERM_WHITE, "\377v%\377w Swamp");
				Term_putstr(0, n++, -1, TERM_WHITE, "\377B~\377w Lake/");
				Term_putstr(0, n++, -1, TERM_WHITE, "  river");
				Term_putstr(0, n++, -1, TERM_WHITE, "\377U,\377w Coast");
				Term_putstr(0, n++, -1, TERM_WHITE, "\377b%\377w Ocean");
				Term_putstr(0, n++, -1, TERM_WHITE, "\377D^\377w Mount");
				Term_putstr(0, n++, -1, TERM_WHITE, "\377r^\377w Volca");
				Term_putstr(0, n++, -1, TERM_WHITE, "\377y.\377w Deser");
				Term_putstr(0, n++, -1, TERM_WHITE, "\377w.\377w Icy");
				Term_putstr(0, n++, -1, TERM_WHITE, "  waste");
				n++;
			}

			/* Hack: Hide cursor */
			Term->scr->cx = Term->wid;
			Term->scr->cu = 1;
			Term_set_cursor(0);
			//Term_xtra(TERM_XTRA_SHAPE, 1);
		}
		return(1);
	}

	/* Check the max line count */
	if (y > last_line_info) last_line_info = y;

#ifdef BIGMAP_MINDLINK_HACK
	/* for big_map mind-link issues: keep track of last map line received */
	last_line_y = y;
#endif

	for (x = 0; x < 80; x++) {
		c = 0; /* Needs to be reset for proper packet read. */
#ifdef GRAPHICS_BG_MASK
		c_back = 0;
		//c_back_real = 32;
		c_back_real = Client_setup.f_char[FEAT_SOLID];
#endif

		/* Read the char/attr pair */

#ifdef GRAPHICS_BG_MASK
		if (use_graphics == UG_2MASK && is_atleast(&server_version, 4, 9, 2, 1, 0, 0)) {
			/* Transfer only minimum number of bytes needed, according to client setup.*/
			char *pc = (char*)&c, *pc_b = (char*)&c_back;

			switch (Client_setup.char_transfer_bytes) {
			case 0:
			case 1:
				if ((n = Packet_scanf(&rbuf, "%c%c%c%c", &pc[0], &a, &pc_b[0], &a_back)) <= 0) return(n);
				break;
			case 2:
				n = Packet_scanf(&rbuf, "%c%c%c%c%c%c", &pc[1], &pc[0], &a, &pc_b[1], &pc_b[0], &a_back);
				break;
			case 3:
				n = Packet_scanf(&rbuf, "%c%c%c%c%c%c%c%c", &pc[2], &pc[1], &pc[0], &a, &pc_b[2], &pc_b[1], &pc_b[0], &a_back);
				break;
			case 4:
			default:
				n = Packet_scanf(&rbuf, "%u%c%u%c", &c, &a, &c_back, &a_back);
			}
			if (n <= 0) {
				if (n == 0) goto rollback;
				return(n);
			}
		} else
#endif
		/* 4.8.1 and newer servers communicate using 32bit character size. */
		if (is_atleast(&server_version, 4, 8, 1, 0, 0, 0)) {
			/* Transfer only minimum number of bytes needed, according to client setup.*/
			char *pc = (char*)&c;

			switch (Client_setup.char_transfer_bytes) {
			case 0:
			case 1:
				n = Packet_scanf(&rbuf, "%c%c", &pc[0], &a);
				break;
			case 2:
				n = Packet_scanf(&rbuf, "%c%c%c", &pc[1], &pc[0], &a);
				break;
			case 3:
				n = Packet_scanf(&rbuf, "%c%c%c%c", &pc[2], &pc[1], &pc[0], &a);
				break;
			case 4:
			default:
				n = Packet_scanf(&rbuf, "%u%c", &c, &a);
			}
			if (n <= 0) {
				if (n == 0) goto rollback;
				return(n);
			}
		} else {
			if ((n = Packet_scanf(&rbuf, "%c%c", &c, &a)) <= 0) {
				if (n == 0) goto rollback;
				return(n);
			}
		}

		/* 4.4.3.1 servers use a = 0xFF to signal RLE */
		if (is_newer_than(&server_version, 4, 4, 3, 0, 0, 5)) {
			/* New RLE */
			if (a == TERM_RESERVED_RLE) {
				/* Read the real attr and number of repetitions */
				if ((n = Packet_scanf(&rbuf, "%c%c", &a, &rep)) <= 0) {
					if (n == 0) goto rollback;
					return(n);
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
					return(n);
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
			if (force_cui) {
				if (c == FONT_MAP_SOLID_X11 || c == FONT_MAP_SOLID_WIN) c = '#';
				if (c == FONT_MAP_VEIN_X11 || c == FONT_MAP_VEIN_WIN) c = '*';
			}
 #ifdef USE_X11
			if (c == FONT_MAP_SOLID_WIN) c = FONT_MAP_SOLID_X11;
			if (c == FONT_MAP_VEIN_WIN) c = FONT_MAP_VEIN_X11;
 #elif defined(WINDOWS)
			if (c == FONT_MAP_SOLID_X11) c = FONT_MAP_SOLID_WIN;
			if (c == FONT_MAP_VEIN_X11) c = FONT_MAP_VEIN_WIN;
 #else /* command-line client doesn't draw either! */
			if (c == FONT_MAP_SOLID_X11 || c == FONT_MAP_SOLID_WIN) c = '#';
			if (c == FONT_MAP_VEIN_X11 || c == FONT_MAP_VEIN_WIN) c = '*';
 #endif
#endif
#ifdef GRAPHICS_BG_MASK
 #ifdef TEST_CLIENT
			/* special hack for mind-link Windows->Linux w/ font_map_solid_walls */
			if (force_cui) {
				if (c_back == FONT_MAP_SOLID_X11 || c_back == FONT_MAP_SOLID_WIN) c_back = '#';
				if (c_back == FONT_MAP_VEIN_X11 || c_back == FONT_MAP_VEIN_WIN) c_back = '*';
			}
  #ifdef USE_X11
			if (c_back == FONT_MAP_SOLID_WIN) c_back = FONT_MAP_SOLID_X11;
			if (c_back == FONT_MAP_VEIN_WIN) c_back = FONT_MAP_VEIN_X11;
  #elif defined(WINDOWS)
			if (c_back == FONT_MAP_SOLID_X11) c_back = FONT_MAP_SOLID_WIN;
			if (c_back == FONT_MAP_VEIN_X11) c_back = FONT_MAP_VEIN_WIN;
  #else /* command-line client doesn't draw either! */
			if (c_back == FONT_MAP_SOLID_X11 || c_back == FONT_MAP_SOLID_WIN) c_back = '#';
			if (c_back == FONT_MAP_VEIN_X11 || c_back == FONT_MAP_VEIN_WIN) c_back = '*';
  #endif
 #endif

			if (c_back) c_back_real = c_back;
#endif
			/* Draw a character 'rep' times */
			for (i = 0; i < rep; i++) {
				/* remember map_info in client-side buffer */
				if (ch != PKT_MINI_MAP &&
				    x + i >= PANEL_X && x + i < PANEL_X + screen_wid &&
				    y >= PANEL_Y && y < PANEL_Y + screen_hgt) {
					panel_map_a[x + i - PANEL_X][y - PANEL_Y] = a;
					panel_map_c[x + i - PANEL_X][y - PANEL_Y] = c;
#ifdef GRAPHICS_BG_MASK
					/* Catch if the server didn't define a valid background ie sent a zero -
					   in that case instead of bugging out the display, interpret it as 'keep our old background' */
					if (c_back) {
						panel_map_a_back[x - PANEL_X][y - PANEL_Y] = a_back;
						panel_map_c_back[x - PANEL_X][y - PANEL_Y] = c_back;
					} else {
						a_back = panel_map_a_back[x - PANEL_X][y - PANEL_Y];
						c_back_real = panel_map_c_back[x - PANEL_X][y - PANEL_Y];
 #if 0
						if (!c_back_real) c_back_real = panel_map_c_back[x - PANEL_X][y - PANEL_Y] = 32;
 #else
						if (!c_back_real) c_back_real = panel_map_c_back[x - PANEL_X][y - PANEL_Y] = Client_setup.f_char[FEAT_SOLID];
 #endif
					}
#endif
				}

#ifdef GRAPHICS_BG_MASK
				if (use_graphics == UG_2MASK)
					Term_draw_2mask(x + i, y, a, c, a_back, c_back_real);
				else
#endif
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

	return(1);

	/* Rollback the socket buffer in case the packet isn't complete */
	rollback:
	rbuf.ptr = stored_sbuf_ptr;
	if (screen_icky && ch != PKT_MINI_MAP) Term_switch(0); /* needed to avoid garbage on screen */
	return(0);
}

#if 0 /* Instead, Receive_line_info() is used, with PKT_MINI_MAP as 'calling' packet */
/*
 * Note that this function does not honor the "screen_icky"
 * flag, as it is only used for displaying the mini-map,
 * and the screen should be icky during that time.
 */
int Receive_mini_map(void) {
	char ch, c;
	int n, x, bytes_read;
	s16b y;
	byte a;

	if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &y)) <= 0) return(n);

	bytes_read = 3;

	/* Hack: Mark minimap transmission as complete, so we can start adding some extra info to it, such as coordinates. */
	if (y == -1) {
		if (screen_icky && minimap_posx != -1) {
 #if 0
			c_put_str(TERM_WHITE, "Selected:", 0, 1);
			c_put_str(TERM_WHITE, format("(%2d,%2d)", p_ptr->wpos.wx, p_ptr->wpos.wy), 0, 2);
 #else
			Term_putstr(1, 1, -1, TERM_WHITE, "Selected:");
			Term_putstr(1, 2, -1, TERM_WHITE, format("(%2d,%2d)", p_ptr->wpos.wx, p_ptr->wpos.wy));
 #endif
		}
		return(1);
	}

	/* Check the max line count */
	if (y > last_line_info)
		last_line_info = y;

	for (x = 0; x < 80; x++) {
		if ((n = Packet_scanf(&rbuf, "%c%c", &c, &a)) <= 0) {
			/* Rollback the socket buffer */
			Sockbuf_rollback(&rbuf, bytes_read);

			/* Packet isn't complete, graceful failure */
			return(n);
		}

		bytes_read += n;

		/* Don't draw anything if "char" is zero */
		/* Only draw if the screen is "icky" */
		if (c && screen_icky && x < 80 - 12)
			Term_draw(x + 12, y, a, c); //todo maybe: GRAPHICS_BG_MASK
	}

	return(1);
}
#endif

int Receive_mini_map_pos(void) {
	char ch;
	char32_t c = 0; /* Needs to be reset for proper packet read. */
	int n;
	short int x, y, y_offset = 0;
	byte a;

	if (is_atleast(&server_version, 4, 8, 1, 2, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd%c%u", &ch, &x, &y, &y_offset, &a, &c)) <= 0) return(n);
	/* 4.8.1 and newer servers communicate using 32bit character size. */
	} else if (is_atleast(&server_version, 4, 8, 1, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%hd%hd%c%u", &ch, &x, &y, &a, &c)) <= 0) return(n);
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%hd%hd%c%c", &ch, &x, &y, &a, &c)) <= 0) return(n);
	}

	if (!local_map_active) return(1);

#ifdef WILDMAP_ALLOW_SELECTOR_SCROLLING
c_msg_format("Rmmp x,y=%d,%d, yoff=%d", x,y,y_offset);
#endif
	minimap_posx = (int)x;
	minimap_posy = (int)y;
	minimap_yoff = (int)y_offset;
	minimap_attr = a;
	minimap_char = c;

	//Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
	minimap_selx = -1; //reset grid selection

	return(1);
}

int Receive_special_other(void) {
	int n;
	char ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return(n);

	/* Set file perusal method to "other" */
	special_line_type = SPECIAL_FILE_OTHER;

	/* Peruse the file we're about to get */
	peruse_file();

	return(1);
}

int Receive_store_action(void) {
	int n;
	short bact, action, cost;
	char ch, pos, name[MAX_CHARS], letter;
	byte attr, oldflag;
	u16b flag;

	if (is_atleast(&server_version, 4, 9, 2, 1, 0, 1)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%hd%hd%s%c%c%hd%hu", &ch, &pos, &bact, &action, name, &attr, &letter, &cost, &flag)) <= 0) return(n);
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%hd%hd%s%c%c%hd%c", &ch, &pos, &bact, &action, name, &attr, &letter, &cost, &oldflag)) <= 0) return(n);
		flag = (u16b)oldflag;
	}

	/* Newer server? (Or just incompatible?) */
	if (pos >= MAX_STORE_ACTIONS) return(1);

	c_store.bact[(int)pos] = bact;
	c_store.actions[(int)pos] = action;
	strncpy(c_store.action_name[(int) pos], name, 40);
	c_store.action_attr[(int)pos] = attr;
	c_store.letter[(int)pos] = letter;
	c_store.cost[(int)pos] = cost;
	c_store.flags[(int)pos] = flag;

	/* Make sure that we're in a store */
	if (shopping) display_store_action();

	return(1);
}

int Receive_store(void) {
	int n, price;
	char ch, pos, name[ONAME_LEN], powers[MAX_CHARS_WIDE];
	byte attr, tval, sval;
	s16b wgt, num, pval;

	if (is_atleast(&server_version, 4, 7, 3, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hd%hd%d%S%c%c%hd%s", &ch, &pos, &attr, &wgt, &num, &price, name, &tval, &sval, &pval, &powers)) <= 0)
			return(n);
	} else if (is_newer_than(&server_version, 4, 4, 7, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hd%hd%d%S%c%c%hd", &ch, &pos, &attr, &wgt, &num, &price, name, &tval, &sval, &pval)) <= 0)
			return(n);
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hd%hd%d%s%c%c%hd", &ch, &pos, &attr, &wgt, &num, &price, name, &tval, &sval, &pval)) <= 0)
			return(n);
	}

	/* If we had store_last_item active and the item we just received is in its position but apparently a different item, clear store_last_item.
	   Ie happens if we buy/steal the last item of a stack in a store slot.
           Note however that this method fails for macros without \w waits, as the macro might already execute the next get_stock() before this inven packet actually arrives to reset store_last_item to -1! */
	if (pos == store_last_item && (sval != store_last_item_sval || tval != store_last_item_tval)) store_last_item = -1;

	store.stock[(int)pos].tval = tval;
	store.stock[(int)pos].sval = sval;
	store.stock[(int)pos].pval = pval;
	store.stock[(int)pos].number = num;
	store.stock[(int)pos].weight = wgt;
	store_prices[(int)pos] = price;
	store.stock[(int)pos].attr = attr;
	strncpy(store_names[(int)pos], name, ONAME_LEN - 1);
	store_names[(int)pos][ONAME_LEN - 1] = 0;
	strncpy(store_powers[(int) pos], powers, MAX_CHARS_WIDE - 1);
	store_powers[(int)pos][MAX_CHARS_WIDE - 1] = 0;

	/* Request a redraw of the store inventory */
	redraw_store = TRUE;

	return(1);
}

int Receive_store_wide(void) {
	int n, price;
	char ch, pos, name[ONAME_LEN];
	byte attr, tval, sval;
	s16b wgt, num, pval;
	s16b xtra1, xtra2, xtra3, xtra4, xtra5, xtra6, xtra7, xtra8, xtra9;
	byte xtra1b, xtra2b, xtra3b, xtra4b, xtra5b, xtra6b, xtra7b, xtra8b, xtra9b;

	if (is_newer_than(&server_version, 4, 7, 0, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hd%hd%d%S%c%c%hd%hd%hd%hd%hd%hd%hd%hd%hd%hd", &ch, &pos, &attr, &wgt, &num, &price, name, &tval, &sval, &pval,
		    &xtra1, &xtra2, &xtra3, &xtra4, &xtra5, &xtra6, &xtra7, &xtra8, &xtra9)) <= 0)
			return(n);
	} else if (is_newer_than(&server_version, 4, 4, 7, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hd%hd%d%S%c%c%hd%c%c%c%c%c%c%c%c%c", &ch, &pos, &attr, &wgt, &num, &price, name, &tval, &sval, &pval,
		    &xtra1b, &xtra2b, &xtra3b, &xtra4b, &xtra5b, &xtra6b, &xtra7b, &xtra8b, &xtra9b)) <= 0)
			return(n);
		xtra1 = (s16b)xtra1b; xtra2 = (s16b)xtra2b; xtra3 = (s16b)xtra3b;
		xtra4 = (s16b)xtra4b; xtra5 = (s16b)xtra5b; xtra6 = (s16b)xtra6b;
		xtra7 = (s16b)xtra7b; xtra8 = (s16b)xtra8b; xtra9 = (s16b)xtra9b;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%hd%hd%d%s%c%c%hd%c%c%c%c%c%c%c%c%c", &ch, &pos, &attr, &wgt, &num, &price, name, &tval, &sval, &pval,
		    &xtra1b, &xtra2b, &xtra3b, &xtra4b, &xtra5b, &xtra6b, &xtra7b, &xtra8b, &xtra9b)) <= 0)
			return(n);
		xtra1 = (s16b)xtra1b; xtra2 = (s16b)xtra2b; xtra3 = (s16b)xtra3b;
		xtra4 = (s16b)xtra4b; xtra5 = (s16b)xtra5b; xtra6 = (s16b)xtra6b;
		xtra7 = (s16b)xtra7b; xtra8 = (s16b)xtra8b; xtra9 = (s16b)xtra9b;
	}

	/* If we had store_last_item active and the item we just received is in its position but apparently a different item, clear store_last_item.
	   Ie happens if we buy/steal the last item of a stack in a store slot.
           Note however that this method fails for macros without \w waits, as the macro might already execute the next get_stock() before this inven packet actually arrives to reset store_last_item to -1! */
	if (pos == store_last_item && (sval != store_last_item_sval || tval != store_last_item_tval)) store_last_item = -1;

	store.stock[(int)pos].tval = tval;
	store.stock[(int)pos].sval = sval;
	store.stock[(int)pos].pval = pval;
	store.stock[(int)pos].number = num;
	store.stock[(int)pos].weight = wgt;
	store_prices[(int)pos] = price;
	store.stock[(int)pos].attr = attr;
	strncpy(store_names[(int)pos], name, ONAME_LEN - 1);
	store_names[(int)pos][ONAME_LEN - 1] = 0;
	store.stock[(int)pos].xtra1 = xtra1;
	store.stock[(int)pos].xtra2 = xtra2;
	store.stock[(int)pos].xtra3 = xtra3;
	store.stock[(int)pos].xtra4 = xtra4;
	store.stock[(int)pos].xtra5 = xtra5;
	store.stock[(int)pos].xtra6 = xtra6;
	store.stock[(int)pos].xtra7 = xtra7;
	store.stock[(int)pos].xtra8 = xtra8;
	store.stock[(int)pos].xtra9 = xtra9;
	store_powers[(int)pos][0] = 0;

	/* Request a redraw of the store inventory */
	redraw_store = TRUE;

	return(1);
}

/* For new SPECIAL store flag, stores that don't have inventory - C. Blue */
int Receive_store_special_str(void) {
	int n;
	char ch, line, col;
	byte attr;
	char str[MAX_CHARS];

	if ((n = Packet_scanf(&rbuf, "%c%c%c%c%s", &ch, &line, &col, &attr, str)) <= 0) return(n);
	if (!shopping) return(1);

	c_put_str(attr, str, line, col);

	/* hack: hide cursor */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;

	return(1);
}

/* For new SPECIAL store flag, stores that don't have inventory - C. Blue
   -- somewhat redundant now with PKT_CHAR_DIRECT -> Receive_char() call. */
int Receive_store_special_char(void) {
	int n;
	char ch, line, col;
	byte attr;
	char c, str[2];

	if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c", &ch, &line, &col, &attr, &c)) <= 0) return(n);
	if (!shopping) return(1);

	str[0] = c;
	str[1] = 0;
	c_put_str(attr, str, line, col);

	/* hack: hide cursor */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;

	return(1);
}

/* For new SPECIAL store flag, stores that don't have inventory - C. Blue */
int Receive_store_special_clr(void) {
	int n;
	char ch, line_start, line_end;

	if ((n = Packet_scanf(&rbuf, "%c%c%c", &ch, &line_start, &line_end)) <= 0) return(n);
	if (!shopping) return(1);

#if 0
	for (n = line_start; n <= line_end; n++)
		c_put_str(TERM_WHITE, "                                                                                ", n, 0);
#else
	clear_from_to(line_start, line_end);
#endif

	/* hack: hide cursor */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;

	return(1);
}

static void display_fruit(int row, int col, int fruit) {
#ifdef USE_GRAPHICS
	bool use_gfx_fruits = /* perhaps display both, first ascii fruit and then gfx fruit on top of it, so ascii fruit will still be there if screen gets refreshed? */
 #ifndef TEST_RAWPICT
	    FALSE;
 #else
	    !c_cfg.ascii_items &&
	    (tiles_rawpict_org[1].defined && tiles_rawpict_org[2].defined && tiles_rawpict_org[3].defined &&
	    tiles_rawpict_org[4].defined && tiles_rawpict_org[5].defined && tiles_rawpict_org[6].defined);
 #endif
#endif

	switch (fruit) {
	case 1: /* lemon */
		Term_putstr(col + 1, row + 9, -1, TERM_L_WHITE,  "LEMON ");
#ifdef USE_GRAPHICS
		if (use_gfx_fruits) {
			(void)((*Term->rawpict_hook)(col, row, 1));
			break;
		}
#endif
		Term_putstr(col, row + 0, -1, TERM_YELLOW, "   ### ");
		Term_putstr(col, row + 1, -1, TERM_YELLOW, "  #...#");
		Term_putstr(col, row + 2, -1, TERM_YELLOW, " #.....#");
		Term_putstr(col, row + 3, -1, TERM_YELLOW, "#......#");
		Term_putstr(col, row + 4, -1, TERM_YELLOW, "#......#");
		Term_putstr(col, row + 5, -1, TERM_YELLOW, "#.....# ");
		Term_putstr(col, row + 6, -1, TERM_YELLOW, " #...#  ");
		Term_putstr(col, row + 7, -1, TERM_YELLOW, "  ###   ");
		break;

	case 2: /* orange */
		Term_putstr(col + 1, row + 9, -1, TERM_L_WHITE,  "ORANGE");
#ifdef USE_GRAPHICS
		if (use_gfx_fruits) {
			(void)((*Term->rawpict_hook)(col, row, 2));
			break;
		}
#endif
		Term_putstr(col, row + 0, -1, TERM_ORANGE, "  ####  ");
		Term_putstr(col, row + 1, -1, TERM_ORANGE, " #++++# ");
		Term_putstr(col, row + 2, -1, TERM_ORANGE, "#++++++#");
		Term_putstr(col, row + 3, -1, TERM_ORANGE, "#++++++#");
		Term_putstr(col, row + 4, -1, TERM_ORANGE, "#++++++#");
		Term_putstr(col, row + 5, -1, TERM_ORANGE, "#++++++#");
		Term_putstr(col, row + 6, -1, TERM_ORANGE, " #++++# ");
		Term_putstr(col, row + 7, -1, TERM_ORANGE, "  ####  ");
		break;

	case 3: /* sword */
		Term_putstr(col + 1, row + 9, -1, TERM_L_WHITE, "SWORD ");
#ifdef USE_GRAPHICS
		if (use_gfx_fruits) {
			(void)((*Term->rawpict_hook)(col, row, 3));
			break;
		}
#endif
		Term_putstr(col, row + 0, -1, TERM_SLATE, "   /\\   ");
		Term_putstr(col, row + 1, -1, TERM_SLATE, "   ##   ");
		Term_putstr(col, row + 2, -1, TERM_SLATE, "   ##   ");
		Term_putstr(col, row + 3, -1, TERM_SLATE, "   ##   ");
		Term_putstr(col, row + 4, -1, TERM_SLATE, "   ##   ");
		Term_putstr(col, row + 5, -1, TERM_SLATE, "   ##   ");
		Term_putstr(col, row + 6, -1, TERM_UMBER, " ###### ");
		Term_putstr(col, row + 7, -1, TERM_UMBER, "   ##   ");
		break;

	case 4: /* shield */
		Term_putstr(col + 1, row + 9, -1, TERM_L_WHITE, "SHIELD");
#ifdef USE_GRAPHICS
		if (use_gfx_fruits) {
			(void)((*Term->rawpict_hook)(col, row, 4));
			break;
		}
#endif
		Term_putstr(col, row + 0, -1, TERM_UMBER, " ###### ");
		Term_putstr(col, row + 1, -1, TERM_UMBER, "#      #");
		Term_putstr(col, row + 2, -1, TERM_UMBER, "# \377U++++\377u #");
		Term_putstr(col, row + 3, -1, TERM_UMBER, "# \377U+==+\377u #");
		Term_putstr(col, row + 4, -1, TERM_UMBER, "#  \377U++\377u  #");
		Term_putstr(col, row + 5, -1, TERM_UMBER, " #    # ");
		Term_putstr(col, row + 6, -1, TERM_UMBER, "  #  #  ");
		Term_putstr(col, row + 7, -1, TERM_UMBER, "   ##   ");
		break;

	case 5: /* plum */
		Term_putstr(col + 1, row + 9, -1, TERM_L_WHITE, " PLUM ");
#ifdef USE_GRAPHICS
		if (use_gfx_fruits) {
			(void)((*Term->rawpict_hook)(col, row, 5));
			break;
		}
#endif
		Term_putstr(col, row + 0, -1, TERM_UMBER, "     #  ");
		Term_putstr(col, row + 1, -1, TERM_VIOLET, "  ##### ");
		Term_putstr(col, row + 2, -1, TERM_VIOLET, " #######");
		Term_putstr(col, row + 3, -1, TERM_VIOLET, "########");
		Term_putstr(col, row + 4, -1, TERM_VIOLET, "########");
		Term_putstr(col, row + 5, -1, TERM_VIOLET, "####### ");
		Term_putstr(col, row + 6, -1, TERM_VIOLET, " ###### ");
		Term_putstr(col, row + 7, -1, TERM_VIOLET, "  ####  ");
		break;

	case 6: /* cherry */
		Term_putstr(col + 1, row + 9, -1, TERM_L_WHITE, "CHERRY");
#ifdef USE_GRAPHICS
		if (use_gfx_fruits) {
			(void)((*Term->rawpict_hook)(col, row, 6));
			break;
		}
#endif
		Term_putstr(col, row + 0, -1, TERM_GREEN, "     ## ");
		Term_putstr(col, row + 1, -1, TERM_GREEN, "   ###  ");
		Term_putstr(col, row + 2, -1, TERM_GREEN, "  #  #  ");
		Term_putstr(col, row + 3, -1, TERM_GREEN, "  #  #  ");
		Term_putstr(col, row + 4, -1, TERM_RED, " ###### ");
		Term_putstr(col, row + 5, -1, TERM_RED, "#..##..#");
		Term_putstr(col, row + 6, -1, TERM_RED, "#..##..#");
		Term_putstr(col, row + 7, -1, TERM_RED, " ##  ## ");
		break;
	}
}


/* Dice Slots: Spin all three slots at once instead of consecutively? */
#define ANIM_SLOT_SPINALL

#ifdef ANIM_SLOT_SPINALL
 #define ANIM_SLOT_SPEED			60	/* ms frame delay, smaller is faster [75] */
 #define ANIM_SLOT_LENGTH		6	/* multiplier and random-increase for the whole sequence, smaller is faster [2 (theoretically for visually masking the result it should be 6 ^^)] */
 #define ANIM_SLOT_SETTLE		6	/* settling sequence length when the animation becomes slower and eventually halts, smaller is faster [6] */
 #define ANIM_SLOT_SETTLE_SLOWDOWN	25	/* ms frame delay increase for the settling sequence, added each frame, smaller is faster [25] */
 #define ANIM_SLOT_OPTIMIZE_SFX_DELAY		/* Actually already start playing the sfx _before_ the delay before the final state, will sync audibly better -_-' */
#else
 #define ANIM_SLOT_SPEED			75	/* ms frame delay, smaller is faster [75] */
 #define ANIM_SLOT_LENGTH		3	/* multiplier and random-increase for the whole sequence, smaller is faster [2 (theoretically for visually masking the result it should be 6 ^^)] */
 #define ANIM_SLOT_SETTLE		6	/* settling sequence length when the animation becomes slower and eventually halts, smaller is faster [6] */
 #define ANIM_SLOT_SETTLE_SLOWDOWN	25	/* ms frame delay increase for the settling sequence, added each frame, smaller is faster [25] */
 #define ANIM_SLOT_OPTIMIZE_SFX_DELAY		/* Actually already start playing the sfx _before_ the delay before the final state, will sync audibly better -_-' */
#endif

#define ANIM_WHEEL_SPEED		50	/* [50] */
#define ANIM_WHEEL_LENGTH		10	/* [10] */
#define ANIM_WHEEL_SETTLE		10	/* [10] */
#define ANIM_WHEEL_SETTLE_SLOWDOWN	25	/* [25] */

int Receive_store_special_anim(void) {
	int n;
	char ch;
	u16b anim1, anim2, anim3, anim4;
	int anim_step;

#ifdef ANIM_SLOT_SPINALL
	int anim_time1 = 0, anim_time2 = 0, anim_time3 = 0;
	int anim_step1, anim_step2, anim_step3, anim_steps_max;
	bool anim_changed = FALSE;
#endif

#ifdef USE_GRAPHICS
	bool use_gfx_d10f = !c_cfg.ascii_items;
#endif

	if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd%hd", &ch, &anim1, &anim2, &anim3, &anim4)) <= 0) return(n);
	if (!shopping) return(1);

	/* Casino: Wheel and Slot machine animations */
	switch (anim1) {
	case 0: //wheel
		Term_fresh(); //show initial 'I'll put you down for...' msgs
		anim_step = rand_int(ANIM_WHEEL_LENGTH) + ANIM_WHEEL_LENGTH * 2;
		while (--anim_step) { //decrement first, or final loop ends up same as final placement done afterwards, resulting in a perceived 'extra sfx' as there is no visible change
			Term_putstr(DICE_X - 13, DICE_Y + 4, -1, TERM_L_GREEN, "                              ");
			Term_putstr(DICE_X - 13 + 3 * ((anim_step + anim2) % 10), DICE_Y + 4, -1, TERM_POIS, "*");

			/* hack: hide cursor */
			Term->scr->cx = Term->wid;
			Term->scr->cu = 1;

			Term_fresh();
#ifdef USE_SOUND_2010
			sound(casino_wheel_sound_idx, SFX_TYPE_OVERLAP, 100, 0, 0, 0);
#endif
#ifdef WINDOWS
			Sleep(anim_step > ANIM_WHEEL_SETTLE ? ANIM_WHEEL_SPEED : ANIM_WHEEL_SPEED + ANIM_WHEEL_SETTLE * ANIM_WHEEL_SETTLE_SLOWDOWN - anim_step * ANIM_WHEEL_SETTLE_SLOWDOWN);
#else
			usleep(1000 * (anim_step > ANIM_WHEEL_SETTLE ? ANIM_WHEEL_SPEED : ANIM_WHEEL_SPEED + ANIM_WHEEL_SETTLE * ANIM_WHEEL_SETTLE_SLOWDOWN - anim_step * ANIM_WHEEL_SETTLE_SLOWDOWN));
#endif
		}
#ifdef USE_SOUND_2010
		//sound(casino_wheel_sound_idx, SFX_TYPE_OVERLAP, 100, 0, 0, 0);
#endif
		Term_putstr(DICE_X - 13, DICE_Y + 4, -1, TERM_L_GREEN, "                              ");
		Term_putstr(DICE_X - 13 + 3 * anim2, DICE_Y + 4, -1, TERM_L_GREEN, "*");
		break;

	case 1: //slot
		anim2--;
		anim3--;
		anim4--;

		/* 'init' sfx is playing, wait for a very little bit to harmonize with it ^^ */
		Term_fresh(); /* display the 'slot machine' immediately */
#ifdef WINDOWS
		Sleep(200);
#else
		usleep(200000);
#endif

#ifdef ANIM_SLOT_SPINALL
		anim_step1 = ANIM_SLOT_LENGTH * 1 + rand_int(ANIM_SLOT_LENGTH);
		anim_step2 = ANIM_SLOT_LENGTH * 3 + rand_int(ANIM_SLOT_LENGTH);
		anim_step3 = ANIM_SLOT_LENGTH * 5 + rand_int(ANIM_SLOT_LENGTH);
		anim_steps_max = ANIM_SLOT_LENGTH * 6;
		display_fruit(7, 26, (anim_step1 + anim2) % 6 + 1);
		display_fruit(7, 35, (anim_step2 + anim3) % 6 + 1);
		display_fruit(7, 44, (anim_step3 + anim4) % 6 + 1);

		while (TRUE) {
 #ifdef WINDOWS
			Sleep(1);
 #else
			usleep(1000);
 #endif
			if (anim_step1) {
				anim_time1++;
				if (anim_time1 == (anim_step1 > ANIM_SLOT_SETTLE ?
				    ANIM_SLOT_SPEED :
				    (ANIM_SLOT_SPEED + ANIM_SLOT_SETTLE * ANIM_SLOT_SETTLE_SLOWDOWN - anim_step1 * ANIM_SLOT_SETTLE_SLOWDOWN))) {
					anim_time1 = 0;
					anim_step1--;
 #ifdef USE_SOUND_2010
  #ifdef ANIM_SLOT_OPTIMIZE_SFX_DELAY
					if (anim_step1 == 1) sound(casino_slots_sound_idx, SFX_TYPE_OVERLAP, 100, 0, 0, 0);
  #else
					if (!anim_step1) sound(casino_slots_sound_idx, SFX_TYPE_OVERLAP, 100, 0, 0, 0);
  #endif
 #endif

					display_fruit(7, 26, (anim_steps_max - anim_step1 + anim2) % 6 + 1);
					anim_changed = TRUE;
				}
			}
			if (anim_step2) {
				anim_time2++;
				if (anim_time2 == (anim_step2 > ANIM_SLOT_SETTLE ?
				    ANIM_SLOT_SPEED :
				    (ANIM_SLOT_SPEED + ANIM_SLOT_SETTLE * ANIM_SLOT_SETTLE_SLOWDOWN - anim_step2 * ANIM_SLOT_SETTLE_SLOWDOWN))) {
					anim_time2 = 0;
					anim_step2--;
 #ifdef USE_SOUND_2010
  #ifdef ANIM_SLOT_OPTIMIZE_SFX_DELAY
					if (anim_step2 == 1) sound(casino_slots_sound_idx, SFX_TYPE_OVERLAP, 100, 0, 0, 0);
  #else
					if (!anim_step1) sound(casino_slots_sound_idx, SFX_TYPE_OVERLAP, 100, 0, 0, 0);
  #endif
 #endif

					display_fruit(7, 35, (anim_steps_max - anim_step2 + anim3) % 6 + 1);
					anim_changed = TRUE;
				}
			}
			if (anim_step3) {
				anim_time3++;
				if (anim_time3 == (anim_step3 > ANIM_SLOT_SETTLE ?
				    ANIM_SLOT_SPEED :
				    (ANIM_SLOT_SPEED + ANIM_SLOT_SETTLE * ANIM_SLOT_SETTLE_SLOWDOWN - anim_step3 * ANIM_SLOT_SETTLE_SLOWDOWN))) {
					anim_time3 = 0;
					anim_step3--;
 #ifdef USE_SOUND_2010
  #ifdef ANIM_SLOT_OPTIMIZE_SFX_DELAY
					if (anim_step3 == 1) sound(casino_slots_sound_idx, SFX_TYPE_OVERLAP, 100, 0, 0, 0);
  #else
					if (!anim_step1) sound(casino_slots_sound_idx, SFX_TYPE_OVERLAP, 100, 0, 0, 0);
  #endif
 #endif

					display_fruit(7, 44, (anim_steps_max - anim_step3 + anim4) % 6 + 1);
					anim_changed = TRUE;
				}
			}

			if (anim_changed) {
				anim_changed = FALSE;

				/* hack: hide cursor */
				Term->scr->cx = Term->wid;
				Term->scr->cu = 1;
				/* redraw */
				Term_fresh();
			}

			if (!anim_step1 && !anim_step2 && !anim_step3) break;
		}

#else

		anim_step = rand_int(ANIM_SLOT_LENGTH) + 3 * ANIM_SLOT_LENGTH;
		while (anim_step--) {
			display_fruit(7, 26, (anim_step + anim2) % 6 + 1);
			display_fruit(7, 35, (anim_step + anim3) % 6 + 1);
			display_fruit(7, 44, (anim_step + anim4) % 6 + 1);

			/* hack: hide cursor */
			Term->scr->cx = Term->wid;
			Term->scr->cu = 1;

			Term_fresh();
 #ifdef USE_SOUND_2010
  #ifdef ANIM_SLOT_OPTIMIZE_SFX_DELAY
			if (!anim_step) sound(casino_slots_sound_idx, SFX_TYPE_OVERLAP, 100, 0, 0, 0);
  #endif
 #endif
 #ifdef WINDOWS
			Sleep(anim_step > ANIM_SLOT_SETTLE ? ANIM_SLOT_SPEED : ANIM_SLOT_SPEED + ANIM_SLOT_SETTLE * ANIM_SLOT_SETTLE_SLOWDOWN - anim_step * ANIM_SLOT_SETTLE_SLOWDOWN);
 #else
			usleep(1000 * (anim_step > ANIM_SLOT_SETTLE ? ANIM_SLOT_SPEED : ANIM_SLOT_SPEED + ANIM_SLOT_SETTLE * ANIM_SLOT_SETTLE_SLOWDOWN - anim_step * ANIM_SLOT_SETTLE_SLOWDOWN));
 #endif
		}
		display_fruit(7, 26, anim2 + 1);
 #ifdef USE_SOUND_2010
  #ifndef ANIM_SLOT_OPTIMIZE_SFX_DELAY
		sound(casino_slots_sound_idx, SFX_TYPE_OVERLAP, 100, 0, 0, 0);
  #endif
 #endif

		anim_step = rand_int(ANIM_SLOT_LENGTH) + 2 * ANIM_SLOT_LENGTH;
		while (anim_step--) {
			display_fruit(7, 35, (anim_step + anim3) % 6 + 1);

			/* hack: hide cursor */
			Term->scr->cx = Term->wid;
			Term->scr->cu = 1;

			Term_fresh();
 #ifdef USE_SOUND_2010
  #ifdef ANIM_SLOT_OPTIMIZE_SFX_DELAY
			if (!anim_step) sound(casino_slots_sound_idx, SFX_TYPE_OVERLAP, 100, 0, 0, 0);
  #endif
 #endif
 #ifdef WINDOWS
			Sleep(anim_step > ANIM_SLOT_SETTLE ? ANIM_SLOT_SPEED : ANIM_SLOT_SPEED + ANIM_SLOT_SETTLE * ANIM_SLOT_SETTLE_SLOWDOWN - anim_step * ANIM_SLOT_SETTLE_SLOWDOWN);
 #else
			usleep(1000 * (anim_step > ANIM_SLOT_SETTLE ? ANIM_SLOT_SPEED : ANIM_SLOT_SPEED + ANIM_SLOT_SETTLE * ANIM_SLOT_SETTLE_SLOWDOWN - anim_step * ANIM_SLOT_SETTLE_SLOWDOWN));
 #endif
		}
		display_fruit(7, 35, anim3 + 1);
 #ifdef USE_SOUND_2010
  #ifndef ANIM_SLOT_OPTIMIZE_SFX_DELAY
		sound(casino_slots_sound_idx, SFX_TYPE_OVERLAP, 100, 0, 0, 0);
  #endif
 #endif

		anim_step = rand_int(ANIM_SLOT_LENGTH) + 1 * ANIM_SLOT_LENGTH;
		while (anim_step--) {
			display_fruit(7, 44, (anim_step + anim4) % 6 + 1);

			/* hack: hide cursor */
			Term->scr->cx = Term->wid;
			Term->scr->cu = 1;

			Term_fresh();
 #ifdef USE_SOUND_2010
  #ifdef ANIM_SLOT_OPTIMIZE_SFX_DELAY
			if (!anim_step) sound(casino_slots_sound_idx, SFX_TYPE_OVERLAP, 100, 0, 0, 0);
  #endif
 #endif
 #ifdef WINDOWS
			Sleep(anim_step > ANIM_SLOT_SETTLE ? ANIM_SLOT_SPEED : ANIM_SLOT_SPEED + ANIM_SLOT_SETTLE * ANIM_SLOT_SETTLE_SLOWDOWN - anim_step * ANIM_SLOT_SETTLE_SLOWDOWN);
 #else
			usleep(1000 * (anim_step > ANIM_SLOT_SETTLE ? ANIM_SLOT_SPEED : ANIM_SLOT_SPEED + ANIM_SLOT_SETTLE * ANIM_SLOT_SETTLE_SLOWDOWN - anim_step * ANIM_SLOT_SETTLE_SLOWDOWN));
 #endif
		}
		display_fruit(7, 44, anim4 + 1);
 #ifdef USE_SOUND_2010
  #ifndef ANIM_SLOT_OPTIMIZE_SFX_DELAY
		sound(casino_slots_sound_idx, SFX_TYPE_OVERLAP, 100, 0, 0, 0);
  #endif
 #endif
#endif
		break;

	case 2: //in-between
		Term_fresh();

#ifdef USE_SOUND_2010
		sound(casino_inbetween_sound_idx, SFX_TYPE_OVERLAP, 100, 0, 0, 0);
#endif
#ifdef WINDOWS
		Sleep(600);
#else
		usleep(600000);
#endif
#ifdef GRAPHICS_BG_MASK
		if (use_gfx_d10f && use_graphics == 2) {
			Term_draw_2mask(DICE_X - 7, DICE_Y + 4, TERM_L_DARK, kidx_po_d10f_tl, 0, 0);
			Term_draw_2mask(DICE_X - 6, DICE_Y + 3, TERM_L_DARK, kidx_po_d10f_t, 0, 0);
			Term_draw_2mask(DICE_X - 5, DICE_Y + 4, TERM_L_DARK, kidx_po_d10f_tr, 0, 0);
			Term_draw_2mask(DICE_X - 7, DICE_Y + 5, TERM_L_DARK, kidx_po_d10f_bl, 0, 0);
			Term_draw_2mask(DICE_X - 6, DICE_Y + 5, TERM_L_DARK, kidx_po_d10f_b, 0, 0);
			Term_draw_2mask(DICE_X - 5, DICE_Y + 5, TERM_L_DARK, kidx_po_d10f_br, 0, 0);
			Term_putstr(DICE_X - 6, DICE_Y + 4, -1, TERM_L_DARK, format("%1d", anim2));
		} else
#endif
#ifdef USE_GRAPHICS
		if (use_gfx_d10f && use_graphics) {
			Term_draw(DICE_X - 7, DICE_Y + 4, TERM_L_DARK, kidx_po_d10f_tl);
			Term_draw(DICE_X - 6, DICE_Y + 3, TERM_L_DARK, kidx_po_d10f_t);
			Term_draw(DICE_X - 5, DICE_Y + 4, TERM_L_DARK, kidx_po_d10f_tr);
			Term_draw(DICE_X - 7, DICE_Y + 5, TERM_L_DARK, kidx_po_d10f_bl);
			Term_draw(DICE_X - 6, DICE_Y + 5, TERM_L_DARK, kidx_po_d10f_b);
			Term_draw(DICE_X - 5, DICE_Y + 5, TERM_L_DARK, kidx_po_d10f_br);
			Term_putstr(DICE_X - 6, DICE_Y + 4, -1, TERM_L_DARK, format("%1d", anim2));
		} else
#endif
		{
			Term_putstr(DICE_X - 8, DICE_Y + 2, -1, TERM_L_DARK, "  _");
			Term_putstr(DICE_X - 8, DICE_Y + 3, -1, TERM_L_DARK, " / \\");
			Term_putstr(DICE_X - 8, DICE_Y + 4, -1, TERM_L_DARK, format("/ %1d \\", anim2));
			Term_putstr(DICE_X - 8, DICE_Y + 5, -1, TERM_L_DARK, "\\___/");
		}
		/* hack: hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;
		Term_fresh();

#ifdef USE_SOUND_2010
		//sound(casino_inbetween_sound_idx, SFX_TYPE_OVERLAP, 100, 0, 0, 0);
#endif
#ifdef WINDOWS
		Sleep(300);
#else
		usleep(300000);
#endif
#ifdef GRAPHICS_BG_MASK
		if (use_gfx_d10f && use_graphics == 2) {
			Term_draw_2mask(DICE_X + 5, DICE_Y + 4, TERM_L_DARK, kidx_po_d10f_tl, 0, 0);
			Term_draw_2mask(DICE_X + 6, DICE_Y + 3, TERM_L_DARK, kidx_po_d10f_t, 0, 0);
			Term_draw_2mask(DICE_X + 7, DICE_Y + 4, TERM_L_DARK, kidx_po_d10f_tr, 0, 0);
			Term_draw_2mask(DICE_X + 5, DICE_Y + 5, TERM_L_DARK, kidx_po_d10f_bl, 0, 0);
			Term_draw_2mask(DICE_X + 6, DICE_Y + 5, TERM_L_DARK, kidx_po_d10f_b, 0, 0);
			Term_draw_2mask(DICE_X + 7, DICE_Y + 5, TERM_L_DARK, kidx_po_d10f_br, 0, 0);
			Term_putstr(DICE_X + 6, DICE_Y + 4, -1, TERM_L_DARK, format("%1d", anim3));
		} else
#endif
#ifdef USE_GRAPHICS
		if (use_gfx_d10f && use_graphics) {
			Term_draw(DICE_X + 5, DICE_Y + 4, TERM_L_DARK, kidx_po_d10f_tl);
			Term_draw(DICE_X + 6, DICE_Y + 3, TERM_L_DARK, kidx_po_d10f_t);
			Term_draw(DICE_X + 7, DICE_Y + 4, TERM_L_DARK, kidx_po_d10f_tr);
			Term_draw(DICE_X + 5, DICE_Y + 5, TERM_L_DARK, kidx_po_d10f_bl);
			Term_draw(DICE_X + 6, DICE_Y + 5, TERM_L_DARK, kidx_po_d10f_b);
			Term_draw(DICE_X + 7, DICE_Y + 5, TERM_L_DARK, kidx_po_d10f_br);
			Term_putstr(DICE_X + 6, DICE_Y + 4, -1, TERM_L_DARK, format("%1d", anim3));
		} else
#endif
		{
			Term_putstr(DICE_X + 4, DICE_Y + 2, -1, TERM_L_DARK, "  _");
			Term_putstr(DICE_X + 4, DICE_Y + 3, -1, TERM_L_DARK, " / \\");
			Term_putstr(DICE_X + 4, DICE_Y + 4, -1, TERM_L_DARK, format("/ %1d \\", anim3));
			Term_putstr(DICE_X + 4, DICE_Y + 5, -1, TERM_L_DARK, "\\___/");
		}
		/* hack: hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;
		Term_fresh();

#ifdef WINDOWS
		Sleep(300);
#else
		usleep(300000);
#endif
#ifdef USE_SOUND_2010
		sound(casino_inbetween_sound_idx, SFX_TYPE_OVERLAP, 100, 0, 0, 0);
#endif
#ifdef WINDOWS
		Sleep(600);
#else
		usleep(600000);
#endif
#ifdef GRAPHICS_BG_MASK
		if (use_gfx_d10f && use_graphics == 2) {
			Term_draw_2mask(DICE_X - 1, DICE_Y + 8, TERM_RED, kidx_po_d10f_tl, 0, 0);
			Term_draw_2mask(DICE_X, DICE_Y + 7, TERM_RED, kidx_po_d10f_t, 0, 0);
			Term_draw_2mask(DICE_X + 1, DICE_Y + 8, TERM_RED, kidx_po_d10f_tr, 0, 0);
			Term_draw_2mask(DICE_X - 1, DICE_Y + 9, TERM_RED, kidx_po_d10f_bl, 0, 0);
			Term_draw_2mask(DICE_X, DICE_Y + 9, TERM_RED, kidx_po_d10f_b, 0, 0);
			Term_draw_2mask(DICE_X + 1, DICE_Y + 9, TERM_RED, kidx_po_d10f_br, 0, 0);
			Term_putstr(DICE_X, DICE_Y + 8, -1, TERM_RED, format("%1d", anim4));
		} else
#endif
#ifdef USE_GRAPHICS
		if (use_gfx_d10f && use_graphics) {
			Term_draw(DICE_X - 1, DICE_Y + 8, TERM_RED, kidx_po_d10f_tl);
			Term_draw(DICE_X, DICE_Y + 7, TERM_RED, kidx_po_d10f_t);
			Term_draw(DICE_X + 1, DICE_Y + 8, TERM_RED, kidx_po_d10f_tr);
			Term_draw(DICE_X - 1, DICE_Y + 9, TERM_RED, kidx_po_d10f_bl);
			Term_draw(DICE_X, DICE_Y + 9, TERM_RED, kidx_po_d10f_b);
			Term_draw(DICE_X + 1, DICE_Y + 9, TERM_RED, kidx_po_d10f_br);
			Term_putstr(DICE_X, DICE_Y + 8, -1, TERM_RED, format("%1d", anim4));
		} else
#endif
		{
			Term_putstr(DICE_X - 2, DICE_Y + 6, -1, TERM_RED, "  _");
			Term_putstr(DICE_X - 2, DICE_Y + 7, -1, TERM_RED, " / \\");
			Term_putstr(DICE_X - 2, DICE_Y + 8, -1, TERM_RED, format("/ %1d \\", anim4));
			Term_putstr(DICE_X - 2, DICE_Y + 9, -1, TERM_RED, "\\___/");
		}
		break;

	case 3: //craps, or just any dice roll: wait for it to settle
#ifdef USE_SOUND_2010
		sound(casino_craps_sound_idx, SFX_TYPE_OVERLAP, 100, 0, 0, 0);
#endif
		Term_fresh();
#ifdef WINDOWS
		Sleep(500); // xD
#else
		usleep(500000);
#endif
		break;

	default:
		c_msg_format("\377RERROR: Unknown store animation %d.", anim1);
	}

	/* hack: hide cursor */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;

	return(1);
}

int Receive_store_info(void) {
	int n, max_cost;
	char ch, owner_name[MAX_CHARS], store_name[MAX_CHARS];
	s16b num_items;
	byte store_attr = TERM_SLATE;
	char store_char = '?';

	if (is_newer_than(&server_version, 4, 7, 4, 2, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%hd%s%s%hd%d%c%c%c", &ch, &store_num, store_name, owner_name, &num_items, &max_cost, &store_attr, &store_char, &store_price_mul)) <= 0) return(n);
	} else if (is_newer_than(&server_version, 4, 4, 4, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%hd%s%s%hd%d%c%c", &ch, &store_num, store_name, owner_name, &num_items, &max_cost, &store_attr, &store_char)) <= 0) return(n);
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%hd%s%s%hd%d", &ch, &store_num, store_name, owner_name, &num_items, &max_cost)) <= 0) return(n);
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

	return(1);
}

int Receive_store_kick(void) {
	int n;
	char ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return(n);

	/* Leave the store */
	leave_store = TRUE;

	/* Stop macro execution if we're on safe_macros! Added this for stealing macros ("Z+..."). */
	if (parse_macro && c_cfg.safe_macros) flush_now();

	clear_pstore_visuals();

	return(1);
}

int Receive_sell(void) {
	int n, price;
	char ch, buf[1024];

	if ((n = Packet_scanf(&rbuf, "%c%d", &ch, &price)) <= 0) return(n);

	/* Tell the user about the price */
	if (c_cfg.no_verify_sell) Send_store_confirm();
	else {
		if (store_num == STORE_MATHOM_HOUSE) sprintf(buf, "Really donate it?");
		else sprintf(buf, "Accept %d gold?", price);

		if (get_check2(buf, FALSE))
			Send_store_confirm();
	}

	return(1);
}

int Receive_target_info(void) {
	int n;
	char ch, x, y, buf[MSG_LEN];

	if (is_atleast(&server_version, 4, 9, 0, 1, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%S", &ch, &x, &y, buf)) <= 0) return(n);
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%s", &ch, &x, &y, buf)) <= 0) return(n);
	}

	/* Handle target message or description */
	if (buf[0]) {
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
	}

	/* Move the cursor */
	Term_gotoxy(x, y);

	return(1);
}

/* Special fx (not thunderstorm) */
int Receive_screenflash(void) {
	int n;
	char ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return(n);

	animate_screenflash = 1;
	animate_screenflash_icky = screen_icky; /* Actually don't animate palette while screen is icky maybe.. */

	return(1);
}

int Receive_sound(void) {
	int n;
	char ch, s;
	int s1, s2, t = -1, v = 100, dx = 0, dy = 0; //note: dy represents the z-axis, strictly 3d-geometrically speaking
	s32b id;

	if (is_atleast(&server_version, 4, 8, 1, 1, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%d%d%d%d%d%d%d", &ch, &s1, &s2, &t, &v, &id, &dx, &dy)) <= 0)
			return(n);
	} else if (is_newer_than(&server_version, 4, 4, 5, 3, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%d%d%d%d%d", &ch, &s1, &s2, &t, &v, &id)) <= 0)
			return(n);
	} else if (is_newer_than(&server_version, 4, 4, 5, 1, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%d%d%d", &ch, &s1, &s2, &t)) <= 0)
			return(n);
	} else if (is_newer_than(&server_version, 4, 4, 5, 0, 0, 0)) {
		/* Primary sound and an alternative */
		if ((n = Packet_scanf(&rbuf, "%c%d%d", &ch, &s1, &s2)) <= 0) return(n);
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &s)) <= 0) return(n);
		s1 = s;
	}

#ifdef USE_SOUND_2010 /* Unfortunately thunder_sound_idx won't be defined, so no lightning animation :/ */
	/* Hack for thunderstorm weather sfx - delay it and cast a lightning lighting effect first.
	   We want to check this even if use_sound is FALSE, because we still can see the visual effect.
	   Also allow the lightning visual effect when thunder sfx is not a weather-type.
	   However, don't show lightning visuals for misc-type, just so we keep a way to cause thunder sfx without accompanying lightning.. */
	if (s1 == thunder_sound_idx) {
		switch (t) {
		case SFX_TYPE_WEATHER: /* Weather-effect, specifically */
			if (noweather_mode || c_cfg.no_weather) return(1);
			//fall through
		case SFX_TYPE_AMBIENT: /* Thunder + Lightning too, but not related to weather */
			/* Cause lightning flash to go with the sfx! */
			animate_lightning = 1;
			animate_lightning_icky = screen_icky; /* Actually don't animate palette while screen is icky maybe.. */
			animate_lightning_vol = v;
			animate_lightning_type = t;
			/* Potentially delay thunderclap sfx 'physically correct' ;) */
			return(1);
		default: /* eg SFX_TYPE_MISC: Just thunder sfx, no lightning implied. Eg 'Thunderstorm' spell! */
			//go on with normal sfx processing
			break;
		}
	}
#endif

	/* Make a sound (if allowed) */
	if (use_sound) {
#ifndef USE_SOUND_2010
		Term_xtra(TERM_XTRA_SOUND, s1);
#else
		if (t == SFX_TYPE_WEATHER && (noweather_mode || c_cfg.no_weather)) return(1);
		if (!sound(s1, t, v, id, dx, dy)) sound(s2, t, v, id, dx, dy);
#endif
	}

	return(1);
}

int Receive_music(void) {
	char ch;
	int n, m, m2 = -1, m3 = -1;
	char cm, cm2, cm3;

	if (is_atleast(&server_version, 4, 9, 2, 1, 0, 1)) {
		if ((n = Packet_scanf(&rbuf, "%c%d%d%d", &ch, &m, &m2, &m3)) <= 0) return(n);
	} else if (is_atleast(&server_version, 4, 8, 1, 2, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%c", &ch, &cm, &cm2, &cm3)) <= 0) return(n);
		m = cm;
		m2 = cm2;
		m3 = cm3;
	} else if (is_newer_than(&server_version, 4, 5, 6, 0, 0, 1)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c", &ch, &cm, &cm2)) <= 0) return(n);
		m = cm;
		m2 = cm2;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &cm)) <= 0) return(n);
		m = cm;
	}

#ifdef USE_SOUND_2010
	/* Special hack for ghost music (4.7.4b+), see handle_music() in util.c */
	if (skip_received_music) {
		skip_received_music = FALSE;
		return(1);
	}

	/* Play background music (if enabled) */
	if (!use_sound) return(1);
	/* Try to play music, if fails try alternative music, if fails too stop playing any music.
	   Special codes -1, -2 and -4 can be used here to induce alternate behaviour (see handle_music()). */
	if (!music(m)) { if (!music(m2)) music(m3); }
#endif

	flick_global_time = ticks;
	return(1);
}
int Receive_music_vol(void) {
	char ch, v;
	int n, m, m2 = -1, m3 = -1;
	char cm, cm2, cm3;

	if (is_atleast(&server_version, 4, 9, 2, 1, 0, 1)) {
		if ((n = Packet_scanf(&rbuf, "%c%d%d%d%c", &ch, &m, &m2, &m3, &v)) <= 0) return(n);
	} else if (is_atleast(&server_version, 4, 8, 1, 2, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c", &ch, &cm, &cm2, &cm3, &v)) <= 0) return(n);
		m = cm;
		m2 = cm2;
		m3 = cm3;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%c", &ch, &cm, &cm2, &v)) <= 0) return(n);
		m = cm;
		m2 = cm2;
	}

#ifdef USE_SOUND_2010
	/* Special hack for ghost music (4.7.4b+), see handle_music() in util.c */
	if (skip_received_music) {
		skip_received_music = FALSE;
		return(1);
	}

	/* Play background music (if enabled) */
	if (!use_sound) return(1);
	/* Try to play music, if fails try alternative music, if fails too stop playing any music.
	   Special codes -1, -2 and -4 can be used here to induce alternate behaviour (see handle_music()). */
	if (!music_volume(m, v)) { if (!music_volume(m2, v)) music_volume(m3, v); }
#endif

	return(1);
}
int Receive_sfx_ambient(void) {
	int n, a;
	char ch;

	if ((n = Packet_scanf(&rbuf, "%c%d", &ch, &a)) <= 0) return(n);

#ifdef USE_SOUND_2010
	/* Play background ambient sound effect (if enabled) */
	if (use_sound) sound_ambient(a);
#endif

	return(1);
}

int Receive_sfx_volume(void) {
	int n;
	char ch, c_ambient, c_weather;

	if ((n = Packet_scanf(&rbuf, "%c%c%c", &ch, &c_ambient, &c_weather)) <= 0) return(n);

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

	return(1);
}

int Receive_boni_col(void) {
	int n, j;

	byte ch, i;
	char spd, slth, srch, infr, lite, dig, blow, crit, shot, migh, mxhp, mxmp, luck, pstr, pint, pwis, pdex, pcon, pchr, amfi = 0, sigl = 0;
	byte cb[16];
	char color;
	char32_t symbol = 0; /* Needs to be reset for proper packet read. */

	/* 4.8.1 and newer servers communicate using 32bit character size. */
	if (is_atleast(&server_version, 4, 8, 1, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%u", &ch, //1+22+16+5 bytes in total
						&i, &spd, &slth, &srch, &infr, &lite, &dig, &blow, &crit, &shot,
						&migh, &mxhp, &mxmp, &luck, &pstr, &pint, &pwis, &pdex, &pcon, &pchr, &amfi, &sigl,
						&cb[0], &cb[1], &cb[2], &cb[3], &cb[4], &cb[5], &cb[6], &cb[7], &cb[8], &cb[9],
						&cb[10], &cb[11], &cb[12], &cb[13], &cb[14], &cb[15], &color, &symbol)) <= 0) return(n);
	} else if (is_newer_than(&server_version, 4, 6, 1, 2, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", &ch, //1+22+16+2 bytes in total
						&i, &spd, &slth, &srch, &infr, &lite, &dig, &blow, &crit, &shot,
						&migh, &mxhp, &mxmp, &luck, &pstr, &pint, &pwis, &pdex, &pcon, &pchr, &amfi, &sigl,
						&cb[0], &cb[1], &cb[2], &cb[3], &cb[4], &cb[5], &cb[6], &cb[7], &cb[8], &cb[9],
						&cb[10], &cb[11], &cb[12], &cb[13], &cb[14], &cb[15], &color, &symbol)) <= 0) return(n);
	} else if (is_newer_than(&server_version, 4, 5, 9, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", &ch, //1+22+13+2 bytes in total
						&i, &spd, &slth, &srch, &infr, &lite, &dig, &blow, &crit, &shot,
						&migh, &mxhp, &mxmp, &luck, &pstr, &pint, &pwis, &pdex, &pcon, &pchr, &amfi, &sigl,
						&cb[0], &cb[1], &cb[2], &cb[3], &cb[4], &cb[5], &cb[6], &cb[7], &cb[8], &cb[9],
						&cb[10], &cb[11], &cb[12], &color, &symbol)) <= 0) return(n);
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", &ch, //1+20+13+2 bytes in total
						&i, &spd, &slth, &srch, &infr, &lite, &dig, &blow, &crit, &shot,
						&migh, &mxhp, &mxmp, &luck, &pstr, &pint, &pwis, &pdex, &pcon, &pchr,
						&cb[0], &cb[1], &cb[2], &cb[3], &cb[4], &cb[5], &cb[6], &cb[7], &cb[8], &cb[9],
						&cb[10], &cb[11], &cb[12], &color, &symbol)) <= 0) return(n);
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

	return(1);
}

int Receive_special_line(void) {
	int n, p;
	char ch, ab, ap;
	s32b max, line;
	byte attr;
	char buf[ONAME_LEN]; /* Allow colour codes! (was: MAX_CHARS, which is just 80) */
	int x, y, phys_line;
#ifdef REGEX_SEARCH
	bool regexp_ok = is_atleast(&server_version, 4, 9, 0, 0, 0, 0);
#endif

	if (is_newer_than(&server_version, 4, 4, 7, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%d%d%c%I", &ch, &max, &line, &attr, buf)) <= 0) return(n);
	} else {
		s16b old_max, old_line;

		if ((n = Packet_scanf(&rbuf, "%c%hd%hd%c%I", &ch, &old_max, &old_line, &attr, buf)) <= 0) return(n);
		max = old_max;
		line = old_line;
	}

	ab = attr;
	ap = TERM_WHITE;

	/* For searching, when we allow empty lines at the end of the file, we have to clear previously displayed stuff. */
	if (special_line_type && !line) clear_from(2);

	/* Hack - prepare for a special sized page (# of lines divisable by n) */
	if (line >= 21 + HGT_PLUS) {
		//21 -> % 1, 22 -> %2, 23 -> %3..
		special_page_size = 21 + HGT_PLUS - ((21 + HGT_PLUS) % (line - (20 + HGT_PLUS)));
		return(1);
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
				c_prt(TERM_ORANGE, format("[Space/p/Enter/BkSpc/g/G/#%s navigate,%s ESC exit.] (%d-%d/%d)",
				    //(p_ptr->admin_dm || p_ptr->admin_wiz) ? "/s/d/D" : "",
#ifdef REGEX_SEARCH
				    regexp_ok ? "/s/d/D/r" : "/s/d/D",
#else
				    "/s/d/D",
#endif
				    my_strcasestr(special_line_title, "unique monster") ? " ! best," : "",
				    cur_line + 1, max_line , max_line), 23 + HGT_PLUS, 0);
			else
				c_prt(TERM_L_WHITE, format("[Space/p/Enter/BkSpc/g/G/#%s navigate,%s ESC exit.] (%d-%d/%d)",
				    //(p_ptr->admin_dm || p_ptr->admin_wiz) ? "/s/d/D" : "",
#ifdef REGEX_SEARCH
				    regexp_ok ? "/s/d/D/r" : "/s/d/D",
#else
				    "/s/d/D",
#endif
				    my_strcasestr(special_line_title, "unique monster") ? " ! best," : "",
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
		if (!line_searching) cur_line = max_line - special_page_size;
		if (cur_line < 0) cur_line = 0;
	}

	/* Cause inkey() to break, if inkey_max_line flag was set */
	inkey_max_line = FALSE;

	/* Print out the info */
	if (special_line_type) { /* If we have quit already, dont */
		/* remember cursor position */
		Term_locate(&x, &y);

		/* Hack: 'Title line' */
		if (line == -1) {
			/* Never scroll the title bar */
			phys_line = 0;
			strcpy(special_line_title, buf);
			c_put_str(attr, buf, phys_line, 0);
		} else { /* Normal line */
			/* Hack: Catch first content line for item inspections -> guide invocation hack */
			if (!cur_line && !line) strcpy(special_line_first, buf);

			phys_line = line +
			    (special_page_size == 21 + HGT_PLUS ? 1 : 2) + /* 1 extra usable line for 21-lines mode? */
			    (special_page_size < 20 + HGT_PLUS ? 1 : 0); /* only 40 lines on 42-lines BIG_MAP? -> slighly improved visuals ;) */

			/* Keep in sync: If peruse_file() displays a title, it must be line + 2.
			   Else line + 1 to save some space for 21-lines (odd_line) feature. */

			/* Always clear the whole line first */
			Term_erase(0, phys_line, 255);

			/* Apply horizontal scroll */
			/* For horizontal scrolling: Parse correct colour code that we might have skipped */
			if (cur_col) {
				for (p = 0; p < cur_col; p++) {
					if (buf[p] != '\377') continue;
					if (buf[p + 1] == '.') attr = ap;
					else if (isalphanum(buf[p + 1])) {
						ap = ab;
						attr = ab = color_char_to_attr(buf[p + 1]);
					}
				}
			}

			/* Finally print the actual line */
			if (strlen(buf) >= cur_col) /* catch too far horizontal scrolling */
				c_put_str(attr, buf + cur_col, phys_line, 0);
		}

		/* restore cursor position */
		Term_gotoxy(x, y);
	}

	return(1);
}
int Receive_special_line_pos(void) {
	int n;
	char ch;

	if ((n = Packet_scanf(&rbuf, "%c%d", &ch, &cur_line)) <= 0) return(n);

	return(1);
}

int Receive_floor(void) {
	int n;
	char ch;
	byte tval;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &tval)) <= 0) return(n);

	/* Ignore for now */
	//tval = tval;

	return(1);
}

int Receive_pickup_check(void) {
	int n;
	char ch, buf[MAX_CHARS];

	if ((n = Packet_scanf(&rbuf, "%c%s", &ch, buf)) <= 0) return(n);

	/* Get a check */
	if (get_check2(buf, FALSE)) {
		/* Pick it up */
		Send_stay();
	}

	return(1);
}


int Receive_party_stats(void) {
	int n, j, k, chp, mhp, cmp, mmp, color;
	char ch, partymembername[MAX_CHARS];

	if ((n = Packet_scanf(&rbuf, "%c%d%d%s%d%d%d%d%d", &ch, &j, &color, &partymembername, &k, &chp, &mhp, &cmp, &mmp)) <= 0) return(n);

	if (screen_icky) Term_switch(0);
	prt_party_stats(j, color, partymembername, k, chp, mhp, cmp, mmp);
	if (screen_icky) Term_switch(0);
	return(1);
}


int Receive_party(void) {
	int n;
	char ch, pname[MAX_CHARS], pmembers[MAX_CHARS], powner[MAX_CHARS];

	if ((n = Packet_scanf(&rbuf, "%c%s%s%s", &ch, pname, pmembers, powner)) <= 0) return(n);

	/* Copy info */
	strcpy(party_info_name, pname);
	strcpy(party_info_members, pmembers);
	strcpy(party_info_owner, powner);

	if (chat_mode == CHAT_MODE_PARTY && !party_info_name[0]) chat_mode = CHAT_MODE_NORMAL;

	/* Check for iron team state */
	if (!strncmp(party_info_name, "Iron Team", 9)) party_info_mode = PA_IRONTEAM; /* Normal (open) iron team */
	else if (!strncmp(party_info_name + 2, "Iron Team", 9)) party_info_mode = (PA_IRONTEAM | PA_IRONTEAM_CLOSED); /* Prefixed colour code indicates 'closed' iron team */
	else party_info_mode = PA_NORMAL; /* Normal party (or no party!) */

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

			if (is_newer_than(&server_version, 4, 7, 1, 1, 0, 0)) {
				if ((party_info_mode & PA_IRONTEAM) && !(party_info_mode & PA_IRONTEAM_CLOSED))
					Term_putstr(40, 6, -1, TERM_WHITE, "(\377G0\377w) Close your iron team");
				else
					Term_putstr(40, 6, -1, TERM_WHITE, "                              ");
			}
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

	return(1);
}

int Receive_guild(void) {
	int n;
	char ch, gname[MAX_CHARS], gmembers[MAX_CHARS], gowner[MAX_CHARS];

	if ((n = Packet_scanf(&rbuf, "%c%s%s%s", &ch, gname, gmembers, gowner)) <= 0) return(n);

	/* Copy info */
	strcpy(guild_info_name, gname);
	strcpy(guild_info_members, gmembers);
	strcpy(guild_info_owner, gowner);

	if (chat_mode == CHAT_MODE_GUILD && !guild_info_name[0]) chat_mode = CHAT_MODE_NORMAL;

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

	return(1);
}

int Receive_guild_config(void) {
	int i, n, master, guild_adders, ghp;
	char ch, dummy[MAX_CHARS];
	int x, y;
	int minlev_32b; /* 32 bits transmitted over the network gets converted to 16 bits */
	char *stored_sbuf_ptr = rbuf.ptr;
	Term_locate(&x, &y);

	if ((n = Packet_scanf(&rbuf, "%c%d%d%d%d%d%d%d", &ch, &master, &guild.flags, &minlev_32b, &guild_adders, &guildhall_wx, &guildhall_wy, &ghp)) <= 0) return(n);
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
			n = Packet_scanf(&rbuf, "%s", dummy);
			if (n == 0) goto rollback;
			else if (n < 0) return n;
		} else {
			n = Packet_scanf(&rbuf, "%s", guild.adder[i]);
			if (n == 0) goto rollback;
			else if (n < 0) return n;
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
	return(1);

	/* Rollback the socket buffer in case the packet isn't complete */
	rollback:
	rbuf.ptr = stored_sbuf_ptr;
	return(0);
}

int Receive_skills(void) {
	int n, i, bytes_read;
	s16b tmp[12];
	char ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return(n);

	bytes_read = n;

	/* Read into skills info */
	for (i = 0; i < 12; i++) {
		if ((n = Packet_scanf(&rbuf, "%hd", &tmp[i])) <= 0) {
			/* Rollback the socket buffer */
			Sockbuf_rollback(&rbuf, bytes_read);

			/* Packet isn't complete, graceful failure */
			return(n);
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
	p_ptr->num_blow = tmp[8] & 0x1F; p_ptr->extra_blows = (tmp[8] & 0xE0) >> 5;
	p_ptr->num_fire = tmp[9];
	p_ptr->num_spell = tmp[10];
	if (is_atleast(&server_version, 4, 7, 3, 1, 0, 0)) {
		p_ptr->see_infra = tmp[11] & 0x3F;
		p_ptr->tim_infra = (tmp[11] & 0x80) ? 0x1 : 0; //'boosted' marker
		p_ptr->tim_infra |= ((tmp[11] & 0x40) ? 0x2 : 0); //hack: abuse for 'maxed out' marker :) (MAX_SIGHT)
	} else p_ptr->see_infra = tmp[11]; //..and leave tim_infra at 0

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	return(1);
}

int Receive_pause(void) {
	int n;
	char ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return(n);

	/* Show the most recent changes to the screen */
	Term_fresh();

	/* Flush any pending keystrokes */
	Term_flush();

	/* Wait */
	inkey();

	/* Flush queue */
	Flush_queue();

	return(1);
}


int Receive_monster_health(void) {
	int n;
	char ch, num;
	byte attr;

	if ((n = Packet_scanf(&rbuf, "%c%c%c", &ch, &num, &attr)) <= 0) return(n);

	if (screen_icky) Term_switch(0);

	/* Draw the health bar */
	health_redraw(num, attr);

	if (screen_icky) Term_switch(0);

	return(1);
}

int Receive_chardump(void) {
	char ch;
	int n;
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
		if ((n = Packet_scanf(&rbuf, "%c%s", &ch, &type)) <= 0) return(n);
	} else
	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return(n);

	/* Access the main view */
	if (screen_icky) Term_switch(0);

	/* additionally do a screenshot of the scene */
	silent_dump = TRUE;
	xhtml_screenshot(format("%s%s_%04d-%02d-%02d_%02d-%02d-%02d_screenshot", cname, type,
	    1900 + ctl->tm_year, ctl->tm_mon + 1, ctl->tm_mday,
	    ctl->tm_hour, ctl->tm_min, ctl->tm_sec), FALSE);

	if (screen_icky) Term_switch(0);

	strnfmt(tmp, 160, "%s%s_%04d-%02d-%02d_%02d-%02d-%02d.txt", cname, type,
	    1900 + ctl->tm_year, ctl->tm_mon + 1, ctl->tm_mday,
	    ctl->tm_hour, ctl->tm_min, ctl->tm_sec);
	file_character(tmp, TRUE);

	return(1);
}

/* Some simple paging, especially useful to notify ghosts
   who are afk while being rescued :) - C. Blue */
int Receive_beep(void) {
	char ch;
	int n;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return(n);
	if (!c_cfg.allow_paging) return(1);
	return(page());
}
int Receive_warning_beep(void) {
	char ch;
	int n;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return(n);
	return(warning_page());
}

int Receive_AFK(void) {
	int n;
	char ch;
	byte afk;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &afk)) <= 0) return(n);

	if (screen_icky) Term_switch(0);

	prt_AFK(afk);

	/* HACK - Also draw world coordinates because they're on the same row - mikaelh */
	prt_depth(p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, depth_town,
	          depth_colour, depth_colour_sector, depth_name);

	if (screen_icky) Term_switch(0);

	return(1);
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
	byte heavy_tool;        /* Heavy digging tool */
	byte cumber_weight;     /* Full weight. FA from MA will be lost if overloaded */
	byte monk_heavyarmor;   /* Reduced MA power? */
	byte rogue_heavyarmor;  /* Missing roguish-abilities' effects? */
	byte awkward_shoot;     /* using ranged weapon while having a shield on the arm */
	byte heavy_swim;        /* Overburdened for swimming */

	if (is_atleast(&server_version, 4, 8, 1, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", &ch, &cumber_armor, &awkward_armor, &cumber_glove, &heavy_wield, &heavy_shield, &heavy_shoot,
		    &icky_wield, &awkward_wield, &easy_wield, &cumber_weight, &monk_heavyarmor, &rogue_heavyarmor, &awkward_shoot, &heavy_swim, &heavy_tool)) <= 0)
			return(n);
	} else if (is_newer_than(&server_version, 4, 4, 2, 0, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c", &ch, &cumber_armor, &awkward_armor, &cumber_glove, &heavy_wield, &heavy_shield, &heavy_shoot,
		    &icky_wield, &awkward_wield, &easy_wield, &cumber_weight, &monk_heavyarmor, &rogue_heavyarmor, &awkward_shoot)) <= 0)
			return(n);
		heavy_swim = 0;
		heavy_tool = 0;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c%c%c%c%c%c%c%c%c", &ch, &cumber_armor, &awkward_armor, &cumber_glove, &heavy_wield, &heavy_shield, &heavy_shoot,
		    &icky_wield, &awkward_wield, &easy_wield, &cumber_weight, &monk_heavyarmor, &awkward_shoot)) <= 0)
			return(n);
		heavy_swim = 0;
		heavy_tool = 0;
		rogue_heavyarmor = 0;
	}

	if (screen_icky) Term_switch(0);

	prt_encumberment(cumber_armor, awkward_armor, cumber_glove, heavy_wield, heavy_shield, heavy_shoot,
	    icky_wield, awkward_wield, easy_wield, cumber_weight, monk_heavyarmor, rogue_heavyarmor, awkward_shoot,
	    heavy_swim, heavy_tool);

	if (screen_icky) Term_switch(0);

	/* We need this for client-side spell chance calc: */
	p_ptr->icky_wield = icky_wield;

	return(1);
}

int Receive_extra_status(void) {
	int	n;
	char	ch;
	char    status[MAX_CHARS];

	if ((n = Packet_scanf(&rbuf, "%c%s", &ch, &status)) <= 0) return(n);

	if (screen_icky) Term_switch(0);
	prt_extra_status(status);
	if (screen_icky) Term_switch(0);
	return(1);
}

int Receive_keepalive(void) {
	int n;
	char ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return(n);
	return(1);
}

int Receive_ping(void) {
	int n, id, tim, utim, tim_now, utim_now, rtt, index;
	char ch, pong;
	char buf[MSG_LEN];
	struct timeval tv;

	if ((n = Packet_scanf(&rbuf, "%c%c%d%d%d%S", &ch, &pong, &id, &tim, &utim, &buf)) <= 0) return(n);

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

		if ((n = Packet_printf(&wbuf, "%c%c%d%d%d%S", ch, pong, id, tim, utim, buf)) <= 0) return(n);
	}

	return(1);
}

/* client-side weather, server-controlled - C. Blue
   Transmitted parameters:
   weather_type = -1, 0, 1, 2, 3: stop, none, rain, snow, sandstorm
   weather_type +10 * n: pre-generate n steps
*/
int Receive_weather(void) {
	int n, i, clouds;
	char ch;
	int wg, wt, ww, wi, ws, wx, wy;
	int cnum, cidx, cx1, cy1, cx2, cy2, cd, cxm, cym;
	char *stored_sbuf_ptr = rbuf.ptr;

	/* base packet: weather + number of clouds */
	if ((n = Packet_scanf(&rbuf, "%c%d%d%d%d%d%d%d%d",
	    &ch, &wt, &ww, &wg, &wi, &ws, &wx, &wy,
	    &cnum)) <= 0) return(n);

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
			return(n);
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
		if (weather_type % 10 == 3) weather_speed_sand = ws;
		else if (weather_type % 10 == 2) weather_speed_snow = ws;
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
#ifdef GRAPHICS_BG_MASK
					if (use_graphics == 2 && !c_cfg.ascii_weather && !c_cfg.no2mask_weather)
						Term_draw_2mask(PANEL_X + weather_element_x[i] - weather_panel_x,
						    PANEL_Y + weather_element_y[i] - weather_panel_y,
						    panel_map_a[weather_element_x[i] - weather_panel_x][weather_element_y[i] - weather_panel_y],
						    panel_map_c[weather_element_x[i] - weather_panel_x][weather_element_y[i] - weather_panel_y],
						    panel_map_a_back[weather_element_x[i] - weather_panel_x][weather_element_y[i] - weather_panel_y],
						    panel_map_c_back[weather_element_x[i] - weather_panel_x][weather_element_y[i] - weather_panel_y]);
					else
#endif
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

	return(1);

	/* Rollback the socket buffer in case the packet isn't complete */
	rollback:
	rbuf.ptr = stored_sbuf_ptr;
	return(0);
}

int Receive_inventory_revision(void) {
	int n;
	char ch;
	int revision;

	if ((n = Packet_scanf(&rbuf, "%c%d", &ch, &revision)) <= 0) return(n);

	p_ptr->inventory_revision = revision;
	Send_inventory_revision(revision);

#if 0 /* moved to Receive_inven...() - cleaner and works much better (ID and *ID*) */
	/* AUTOINSCRIBE - moved to Receive_inventory_revision() */
	apply_auto_inscriptions(-1);
#endif

	return(1);
}

int Receive_palette(void) {
	int n;
	char  ch;
	byte c, r, g, b;

	if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c", &ch, &c, &r, &g, &b)) <= 0) return(n);

	if (c_cfg.palette_animation) set_palette(c, r, g, b);
	return(1);
}

int Receive_idle(void) {
	int n;
	char ch, idle;
#ifdef USE_SOUND_2010
	static bool idle_muted_music = TRUE, idle_muted_weather = TRUE;
#endif

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &idle)) <= 0) return(n);

#ifdef USE_SOUND_2010
	if (idle) {
		if (cfg_audio_music) {
			toggle_music(TRUE);
			idle_muted_music = TRUE;
		} else if (idle_muted_music) idle_muted_music = FALSE;
		if (cfg_audio_weather) {
			toggle_weather();
			idle_muted_weather = TRUE;
		} else if (idle_muted_weather) idle_muted_weather = FALSE;
	} else {
		if (!cfg_audio_music && idle_muted_music) toggle_music(TRUE);
		if (!cfg_audio_weather && idle_muted_weather) toggle_weather();
	}
#endif
	return(1);
}

/* Apply auto_pickup and auto_destroy */
void apply_auto_pickup(char *item_name) {
	int i;
	char *ex, ex_buf[ONAME_LEN];
	char *ex2, ex_buf2[ONAME_LEN];
	char *match;
	bool found = FALSE, dau = c_cfg.destroy_all_unmatched && c_cfg.auto_destroy;
#ifdef REGEX_SEARCH
	int ires = -999;
	regex_t re_src;
	regmatch_t pmatch[REGEX_ARRAY_SIZE + 1];
#endif
	bool unowned;
	char *c;

	c = strchr(item_name, '{');
	unowned = (c && ((c[1] >= '0' && c[1] <= '9') || c[1] == '?'));

	if ((!c_cfg.auto_pickup && !c_cfg.auto_destroy) ||
	    (c_cfg.autoloot_dunonly && !p_ptr->wpos.wz) ||
	    (c_cfg.autoloot_dununown && !(p_ptr->wpos.wz || unowned)))
		return;

	for (i = 0; i < MAX_AUTO_INSCRIPTIONS; i++) {
		match = auto_inscription_match[i];

		/* skip empty auto-inscriptions */
		if (!match[0]) continue;
		/* skip disabled auto-inscriptions */
		if (auto_inscription_disabled[i]) continue;

		/* do nothing if match is not set to auto-pickup (for items we dont want to pickup nor destroy, mainly for chests) */
		if (!(c_cfg.auto_pickup && auto_inscription_autopickup[i]) && !(c_cfg.auto_destroy && auto_inscription_autodestroy[i]) && !dau
		    && !auto_inscription_ignore[i])
			continue;

		/* 'all items' super wildcard? */
		if (!strcmp(match, "#") || !strcmp(match, "!#")) {
			found = TRUE;
			break;
		}

		/* Strip '!' prefix, as it is only for inscribing, not for auto-pickup */
		//legacy --  if (match[0] == '!') match++;
#ifdef REGEX_SEARCH
		/* Check for '$' prefix, forcing regexp interpretation */
		if (match[0] == '$') {
			match++;

			ires = regcomp(&re_src, match, REG_EXTENDED | REG_ICASE);
			if (ires != 0) {
				c_msg_format("\377yInvalid regular expression (%d) in auto-inscription #%d.", ires, i);
				continue;
			}
			if (regexec(&re_src, item_name, REGEX_ARRAY_SIZE, pmatch, 0)) {
				regfree(&re_src);
				continue;
			}
			if (pmatch[0].rm_so == -1) {
				regfree(&re_src);
				continue;
			}
			/* Actually disallow searches that match empty strings */
			if (pmatch[0].rm_eo - pmatch[0].rm_so == 0) {
				regfree(&re_src);
				continue;
			}
			found = TRUE;

		} else
#endif
		{
			/* '#' wildcard allowed: a random number (including 0) of random chars */
			/* prepare */
			strcpy(ex_buf, match);
			ex2 = item_name;
			found = FALSE;

			do {
				ex = strstr(ex_buf, "#");
				if (ex == NULL) {
					if (strstr(ex2, ex_buf)) found = TRUE;
					break;
				} else {
					/* get partial string up to before the '#' */
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
		}

		if (found) break;
	}

	/* no match found? */
	if (!found) {
		/* destroy all unmatched items? */
		if (dau) Send_msg("/xdis fa");
		/* done */
		return;
	}

	/* just ignore it? */
	if (auto_inscription_ignore[i]) {
		if (c_cfg.autoinsc_debug) c_msg_format("Auto-inscription rule in line #%d matched, rule 'ignore'.", i + 1);
		return;
	}

	/* destroy or pick up */
	if (c_cfg.auto_destroy && auto_inscription_autodestroy[i]) {
		if (c_cfg.autoinsc_debug) c_msg_format("Auto-inscription rule in line #%d matched, rule 'destroy'.", i + 1);
		Send_msg("/xdis fa"); /* didn't find a better way */
	} else if (c_cfg.auto_pickup && auto_inscription_autopickup[i]) {
		if (c_cfg.autoinsc_debug) c_msg_format("Auto-inscription rule in line #%d matched, rule 'pickup'.", i + 1);
		Send_stay_auto();
	}
}

/* Apply client-side auto-inscriptions - C. Blue
   'insc_idx': -1 to apply all auto-inscriptions, otherwise only apply one particular auto-inscription.
   'force': overwrite existing non-trivial inscription.
   Always returns 'FALSE' if auto_inscr_off is enabled ie all auto-inscriptions are disabled.
   Always returns 'FALSE' if the item slot is empty.
   Returns 'TRUE' if the item is now inscribed, no matter whether by us or already before we tried, else FALSE. */
bool apply_auto_inscriptions_aux(int slot, int insc_idx, bool force) {
	int i;
	char *ex, ex_buf[ONAME_LEN];
	char *ex2, ex_buf2[ONAME_LEN];
	char *match, tag_buf[ONAME_LEN];
	bool auto_inscribe, found, already_has_insc = FALSE;
#ifdef REGEX_SEARCH
	int ires = -999;
	regex_t re_src;
	regmatch_t pmatch[REGEX_ARRAY_SIZE + 1];
#endif
	int start, stop;
	cptr iname;
	int iinsc, iilen;
#ifdef ENABLE_SUBINVEN
	int sslot = -1;
#endif

	if (c_cfg.auto_inscr_off) return(FALSE);

#ifdef ENABLE_SUBINVEN
	if (slot >= SUBINVEN_INVEN_MUL) {
		sslot = slot / SUBINVEN_INVEN_MUL - 1;
		slot %= SUBINVEN_INVEN_MUL;

		/* skip empty items */
		if (!subinventory[sslot][slot].tval) return(FALSE);

		iname = subinventory_name[sslot][slot];
		iinsc = subinventory_inscription[sslot][slot];
		iilen = subinventory_inscription_len[sslot][slot];
	} else
#endif
	{
		/* skip empty items */
		if (!inventory[slot].tval) return(FALSE);

		iname = inventory_name[slot];
		iinsc = inventory_inscription[slot];
		iilen = inventory_inscription_len[slot];
	}

	start = (insc_idx == -1 ? 0 : insc_idx);
	stop = (insc_idx == -1 ? MAX_AUTO_INSCRIPTIONS : insc_idx + 1);

	/* haaaaack: check for existing inscription! */
	auto_inscribe = FALSE;
	/* look for 1st '{' which must be level requirements on ANY item */
	ex = strchr(iname, '{');
	if (ex == NULL) return(FALSE); /* paranoia - should always be FALSE */
	strcpy(ex_buf, ex + 1);
	/* look for 2nd '{' which MUST be an inscription */
	ex = strchr(ex_buf, '{');

	if (ex && !NAME_DISCARDABLE_INSCR(ex)) already_has_insc = TRUE;

	/* Add "fake-artifact" inscriptions using '#' */
	if (iinsc) {
		if (ex == NULL) {
			strcat(ex_buf, "{#"); /* create a 'real' inscription from this fake-name inscription */
			/* hack 'ex' to make this special inscription known */
			ex = ex_buf + strlen(ex_buf) - 2;
			/* add the fake-name inscription */
			i = strlen(ex_buf);
			strncat(ex_buf, iname + iinsc, iilen);
			ex_buf[i + iilen] = '\0'; /* terminate string after strncat() */
			strcat(ex_buf, "}");
		} else {
			ex_buf[strlen(ex_buf) - 1] = '\0'; /* cut off final '}' */
			strcat(ex_buf, "#"); /* add the fake-name inscription */
			i = strlen(ex_buf);
			strncat(ex_buf, iname + iinsc, iilen);
			ex_buf[i + iilen] = '\0'; /* terminate string after strncat() */
			strcat(ex_buf, "}"); /* restore final '}' */
		}
	}

	/* no inscription? inscribe it then automatically */
	if (ex == NULL || force) auto_inscribe = TRUE;
	/* check whether inscription is just a discount/stolen tag, if so, auto-inscribe it instead */
	else if (NAME_DISCARDABLE_INSCR_FLOOR(ex)) auto_inscribe = TRUE;

	/* save for checking for already existing target inscription ('!' compatible?) */
	if (ex && strlen(ex) > 2) {
		strncpy(tag_buf, ex + 1, strlen(ex) - 2);
		tag_buf[strlen(ex) - 2] = '\0'; /* terminate */
	}
	else strcpy(tag_buf, ""); /* initialise as empty */

	/* look for matching auto-inscription */
	for (i = start; i < stop; i++) {
		match = auto_inscription_match[i];
		/* skip empty auto-inscriptions */
		if (!match[0]) continue;

		/* special: a rule that does anything except just inscribing is ignored if the tag is empty,
		   otherwise it causes mad empty-inscriptions spam on looting. */
		if (!auto_inscription_tag[i][0] &&
		    (auto_inscription_ignore[i] || auto_inscription_autopickup[i] || auto_inscription_autodestroy[i])) continue;

		/* skip disabled auto-inscriptions */
		if (auto_inscription_disabled[i]) continue;
#ifdef ENABLE_SUBINVEN
		/* skip bag-only auto-inscriptions if not in a bag */
		if (auto_inscription_subinven[i] && sslot == -1) continue;
#endif

		/* if item already has an inscription, only allow to overwrite it if 'forced'. */
		if (!auto_inscribe) {
			if (!auto_inscription_force[i]) continue;
			if (!strcmp(auto_inscription_tag[i], tag_buf)) continue;
		}

		/* 'all items' super wildcard? - this only works for auto-pickup/destroy, not for auto-inscribing */
		if (!strcmp(match, "#")) continue;

#ifdef REGEX_SEARCH
		/* Check for '$' prefix, forcing regexp interpretation */
		if (match[0] == '$') {
			match++;

			ires = regcomp(&re_src, match, REG_EXTENDED | REG_ICASE);
			if (ires != 0) {
				//too spammy when auto-inscribing the whole inventory -- c_msg_format("\377yInvalid regular expression (%d) in auto-inscription #%d.", ires, i);
				continue;
			}
			if (regexec(&re_src, iname, REGEX_ARRAY_SIZE, pmatch, 0)) {
				regfree(&re_src);
				continue;
			}
			if (pmatch[0].rm_so == -1) {
				regfree(&re_src);
				continue;
			}
			/* Actually disallow searches that match empty strings */
			if (pmatch[0].rm_eo - pmatch[0].rm_so == 0) {
				regfree(&re_src);
				continue;
			}
			found = TRUE;

		} else
#endif
		{
			/* found a matching inscription? */

			/* '#' wildcard allowed: a random number (including 0) of random chars */

			/* prepare */
			strcpy(ex_buf, match);
			ex2 = (char*)iname;
			found = FALSE;

			do {
				ex = strstr(ex_buf, "#");
				if (ex == NULL) {
					if (strstr(ex2, ex_buf)) found = TRUE;
					break;
				} else {
					/* get partial string up to before the '#' */
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
		}

		if (found) {
			if (c_cfg.autoinsc_debug) c_msg_format("Auto-inscription rule in line #%d matched.", i + 1);
			break;
		}
	}
	/* no match found? */
	if (i == stop) return(already_has_insc);

	/* send the new inscription, or uninscribe if tag is empty */
	/* security hack: avoid infinite looping */
	if (!DISCARDABLE_INSCR_FLOOR(auto_inscription_tag[i])) { //note: the three 'cursed', 'on sale', 'stolen' (part of DISCARDxxx) are actually NOT empty inscriptions so they don't really need checking here
		if (auto_inscription_tag[i][0]) {
#ifdef ENABLE_SUBINVEN
			Send_inscribe((sslot + 1) * SUBINVEN_INVEN_MUL + slot, auto_inscription_tag[i]);
#else
			Send_inscribe(slot, auto_inscription_tag[i]);
#endif
		} else {
#ifdef ENABLE_SUBINVEN
			Send_uninscribe((sslot + 1) * SUBINVEN_INVEN_MUL + slot);
#else
			Send_uninscribe(slot);
#endif
		}
		return(TRUE);
	}
	return(already_has_insc);
}

/* For store-purchase-quantity preemptive auto-inscription-quantity-limit check xD :
   Scan all client-side auto-inscriptions against an item (which we want to purchase from a store or grab from a home),
   and if it matches, check if the tag would limit the quantity via !G and in that case limit the purchase amount to that.
   So it's no longer required to buy ONE and then again buy 'all' when restocking !G-items that we ran out of completely. - C. Blue
   Store items are never inscribed, but home items might be, in which case:
    Instead of scanning auto-inscription list, we try to find a !G limit on the item inscription itself and just return 0 for 'no limit' if we don't find one,
   Returns 0 if auto_inscr_off is enabled ie all auto-inscriptions are disabled (or if item was already inscribed but didn't contain a !G limit inscription).
   Returns !G limit number from an auto-inscription list entry, if we found one matching the item.

   PROBLEMS/LIMITATIONS:
   We currently do not transmit inventory_inscription and inventory_inscription_len for store (or house) items, unlike for inventory items.
   That means we cannot distinguish inscriptions (especially '#' ones, where we are most unsure where it starts) from the item name.
   So we can still check for '!G' occurance easily, but we cannot know for certain, if a house item does contain an #-inscription or no inscription at all,
   so we will treat the specific case of <house items containing only an '#'-inscription and no !/@/bracers> as 'no inscription'.
   (The !, @, { are just arbitrarily the easiest to check things to find out that the item name must contain some sort of inscription.) */
int scan_auto_inscriptions_for_limit(cptr iname) {
	int i, g;
	char *ex, ex_buf[ONAME_LEN];
	char *ex2, ex_buf2[ONAME_LEN];
	char *match, tag_buf[ONAME_LEN];
	bool found, has_insc = FALSE;
#ifdef REGEX_SEARCH
	int ires = -999;
	regex_t re_src;
	regmatch_t pmatch[REGEX_ARRAY_SIZE + 1];
#endif

	if (c_cfg.auto_inscr_off) return(FALSE);

	/* look for 1st '{' which must be level requirements on ANY item */
	ex = strchr(iname, '{');
	if (ex == NULL) return(FALSE); /* paranoia - should always be FALSE */
	strcpy(ex_buf, ex + 1);
	/* look for 2nd '{' which MUST be an inscription */
	ex = strchr(ex_buf, '{');

	if (ex && !NAME_DISCARDABLE_INSCR(ex)) has_insc = TRUE; //NAME_DISCARDABLE_INSCR_FLOOR? probably not, consumables aren't cursed anyway

	/* Check for #-inscriptions as good as we can, ie inscriptions not using bracers {...} (which got checked just above) */
	if (strchr(iname, '!') || strchr(iname, '@')) has_insc = TRUE;

	/* If we already have an inscription, scan for !G and use that or return 0, and we're done. */
	if (has_insc) {
		if ((i = check_guard_inscription_str(iname, 'G')) > 1) return(i - 1);
		return(0);
	}

	strcpy(tag_buf, ""); /* initialise as empty */

	/* look for matching auto-inscription */
	for (i = 0; i < MAX_AUTO_INSCRIPTIONS; i++) {
		match = auto_inscription_match[i];
		/* skip empty auto-inscriptions */
		if (!match[0]) continue;
		/* skip disabled auto-inscriptions */
		if (auto_inscription_disabled[i]) continue;

		/* 'all items' super wildcard? - this only works for auto-pickup/destroy, not for auto-inscribing */
		if (!strcmp(match, "#")) continue;

#ifdef REGEX_SEARCH
		/* Check for '$' prefix, forcing regexp interpretation */
		if (match[0] == '$') {
			match++;

			ires = regcomp(&re_src, match, REG_EXTENDED | REG_ICASE);
			if (ires != 0) {
				//too spammy when auto-inscribing the whole inventory -- c_msg_format("\377yInvalid regular expression (%d) in auto-inscription #%d.", ires, i);
				continue;
			}
			if (regexec(&re_src, iname, REGEX_ARRAY_SIZE, pmatch, 0)) {
				regfree(&re_src);
				continue;
			}
			if (pmatch[0].rm_so == -1) {
				regfree(&re_src);
				continue;
			}
			/* Actually disallow searches that match empty strings */
			if (pmatch[0].rm_eo - pmatch[0].rm_so == 0) {
				regfree(&re_src);
				continue;
			}
			found = TRUE;

		} else
#endif
		{
			/* found a matching inscription? */
			/* prepare */
			strcpy(ex_buf, match);
			ex2 = (char*)iname;
			found = FALSE;

			do {
				ex = strstr(ex_buf, "#");
				if (ex == NULL) {
					if (strstr(ex2, ex_buf)) found = TRUE;
					break;
				} else {
					/* get partial string up to before the '#' */
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
		}

		if (found) break;
	}
	/* no match found? */
	if (i == MAX_AUTO_INSCRIPTIONS) return(0);

	/* Scan for !G and return that or 0 */
	if ((g = check_guard_inscription_str(auto_inscription_tag[i], 'G') - 1) > 0) {
		if (c_cfg.autoinsc_debug) c_msg_format("Auto-inscription rule containing '!G%d' tag in line #%d matched.", g, i + 1);
		return(g);
	}
	return(0);
}

int Receive_account_info(void) {
	int n;
	char ch;

	if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &acc_flags)) <= 0)
		return(n);

	acc_got_info = TRUE;
	display_account_information();

	return(1);
}

/* Request keypress (1 char) */
int Receive_request_key(void) {
	int n, id;
	char ch, prompt[MAX_CHARS], buf;

	if ((n = Packet_scanf(&rbuf, "%c%d%s", &ch, &id, prompt)) <= 0) return(n);

	request_pending = TRUE;
	if (get_com(prompt, &buf)) Send_request_key(id, buf);
	else Send_request_key(id, 0);
	request_pending = FALSE;
	return(1);
}
/* Request 'amount' number */
int Receive_request_amt(void) {
	int n, id, max;
	char ch, prompt[MAX_CHARS];

	if ((n = Packet_scanf(&rbuf, "%c%d%s%d", &ch, &id, prompt, &max)) <= 0) return(n);

	request_pending = TRUE;
	Send_request_amt(id, c_get_quantity(prompt, 0, max));
	request_pending = FALSE;
	return(1);
}
/* Request number */
int Receive_request_num(void) {
	int n, id, predef, min, max;
	char ch, prompt[MAX_CHARS];

	if ((n = Packet_scanf(&rbuf, "%c%d%s%d%d%d", &ch, &id, prompt, &predef, &min, &max)) <= 0) return(n);

	request_pending = TRUE;
	Send_request_num(id, c_get_number(prompt, predef, min, max));
	request_pending = FALSE;
	return(1);
}
/* Request string (1 line) */
int Receive_request_str(void) {
	int n, id;
	char ch, prompt[MAX_CHARS], buf[MAX_CHARS_WIDE];

	if ((n = Packet_scanf(&rbuf, "%c%d%s%s", &ch, &id, prompt, buf)) <= 0) return(n);

	request_pending = TRUE;
	if (get_string(prompt, buf, MAX_CHARS_WIDE - 1)) Send_request_str(id, buf);
	else Send_request_str(id, "\e");
	request_pending = FALSE;
	return(1);
}
/* Request confirmation (y/n) */
int Receive_request_cfr(void) {
	int n, id;
	char ch, prompt[MAX_CHARS];
	char default_choice = FALSE;

	if (is_newer_than(&server_version, 4, 5, 6, 0, 0, 1)) {
		char dy;

		if ((n = Packet_scanf(&rbuf, "%c%d%s%c", &ch, &id, prompt, &dy)) <= 0) return(n);
		default_choice = dy;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%d%s", &ch, &id, prompt)) <= 0) return(n);
	}

	request_pending = TRUE;
	Send_request_cfr(id, get_check3(prompt, default_choice));
	request_pending = FALSE;
	return(1);
}
/* Cancel pending input request */
int Receive_request_abort(void) {
	int n;
	char ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return(n);
	if (request_pending) request_abort = TRUE;
	return(1);
}

int Receive_martyr(void) {
	int n;
	char ch, martyr;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &martyr)) <= 0) return(n);

	p_ptr->martyr = (s16b)martyr;
	c_msg_print(format("martyr %d", (s16b)martyr));

	return(1);
}

/* Receive inventory index of the last item we picked up */
int Receive_item_newest(void) {
	int	n;
	char	ch;

	if (is_older_than(&server_version, 4, 8, 1, 1, 0, 0)) {
		char item;

		if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &item)) <= 0) return(n);
		item_newest = (int)item;
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%d", &ch, &item_newest)) <= 0) return(n); //ENABLE_SUBINVEN
	}

	/* As long as we don't have an 'official' newest item, use fallback replacement if exists */
	if (item_newest == -1 && item_newest_2nd != -1) item_newest = item_newest_2nd;
	if (c_cfg.show_newest) redraw_newest();

	return(1);
}
int Receive_item_newest_2nd(void) {
	int	n;
	char	ch;

	if ((n = Packet_scanf(&rbuf, "%c%d", &ch, &item_newest_2nd)) <= 0) return(n); //ENABLE_SUBINVEN
	if (item_newest_2nd == -1) return(1);

	/* As long as we don't have an 'official' newest item, use fallback replacement if exists */
	if (item_newest == -1) {
		item_newest = item_newest_2nd;
		if (c_cfg.show_newest) redraw_newest();
	}
	/* Hack, mainly for scrolls and potions: If tval is identical, overwrite newest item with 2nd-newest anyway.
	   This happens eg if we pick up one type of scrolls, and then read another type -> the one we read replaces the picked-up one. */
	else {
		int tval, tval_2nd;

#ifdef ENABLE_SUBINVEN
		if (item_newest >= SUBINVEN_INVEN_MUL) tval = subinventory[item_newest / SUBINVEN_INVEN_MUL - 1][item_newest % SUBINVEN_INVEN_MUL].tval;
		else
#endif
		tval = inventory[item_newest].tval;

#ifdef ENABLE_SUBINVEN
		if (item_newest_2nd >= SUBINVEN_INVEN_MUL) tval_2nd = subinventory[item_newest_2nd / SUBINVEN_INVEN_MUL - 1][item_newest_2nd % SUBINVEN_INVEN_MUL].tval;
		else
#endif
		tval_2nd = inventory[item_newest_2nd].tval;

		if (tval == tval_2nd) {
			item_newest = item_newest_2nd;
			if (c_cfg.show_newest) redraw_newest();
		}
	}
	return(1);
}

/* Receive a confirmation for a particular PKT_.. command we issued earlier to the server */
int Receive_confirm(void) {
	int	n;
	char	ch, ch_confirmed;

	if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &ch_confirmed)) <= 0) return(n);
	command_confirmed = ch_confirmed;

	return(1);
}

//not implemented
int Receive_keypress(void) {
	int	n;
	char	ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return(n);
	return(1);
}

/* Invoke Guide-search on client side remotely from the server.
   search_type: 1 = search, 2 = strict search (all upper-case),  3 = chapter search, 4 = line number,
                0 = no pre-defined search, we're browsing it normally. */
int Receive_Guide(void) {
	char ch, search_type, search_string[MAX_CHARS_WIDE];
	int n, lineno;

	if ((n = Packet_scanf(&rbuf, "%c%c%d%s", &ch, &search_type, &lineno, search_string)) <= 0) return(n);
	cmd_the_guide(search_type, lineno, search_string);
	return(1);
}

int Receive_indicators(void) {
	int n;
	char ch;
	u32b indicators;

	if ((n = Packet_scanf(&rbuf, "%c%d", &ch, &indicators)) <= 0) return(n);

	if (screen_icky) Term_switch(0);
	prt_indicators(indicators);
	if (screen_icky) Term_switch(0);

	return(1);
}

int Receive_playerlist(void) {
	int i, n, mode;
	char ch, tmp_n[NAME_LEN], tmp[MAX_CHARS_WIDE];
	char *stored_sbuf_ptr = rbuf.ptr;

	if ((n = Packet_scanf(&rbuf, "%c%d", &ch, &mode)) <= 0) return(n);

	switch (mode) {
	case 0:
		/* clear */
		for (i = 0; i < MAX_PLAYERS_LISTED; i++) playerlist_name[i][0] = 0;
		NumPlayers = 0;
		break;
	case 1:
		/* Implies 'clear' */
		for (i = 0; i < MAX_PLAYERS_LISTED; i++) playerlist_name[i][0] = 0;
		i = 0;
		/* Receive complete list (initial login) */
		while (TRUE) {
			n = Packet_scanf(&rbuf, "%s%I", tmp_n, tmp);
			if (n == 0) goto rollback;
			else if (n < 0) return(n);

			if (!tmp_n[0]) break;

			if (i < MAX_PLAYERS_LISTED) {
				strcpy(playerlist_name[i], tmp_n);
				strcpy(playerlist[i], tmp);
				i++;
			}
		}
		NumPlayers = i;
		break;
	case 2:
		/* Add/update a specific player */
		n = Packet_scanf(&rbuf, "%s%I", tmp_n, tmp);
		if (n == 0) goto rollback;
		else if (n < 0) return(n);

		for (i = 0; i < MAX_PLAYERS_LISTED; i++) {
			/* update */
			if (streq(playerlist_name[i], tmp_n)) {
				strcpy(playerlist[i], tmp);
				break;
			}
			/* add */
			if (!playerlist_name[i][0]) {
				strcpy(playerlist_name[i], tmp_n);
				strcpy(playerlist[i], tmp);
				NumPlayers++;
				break;
			}
		}
		break;
	case 3:
		/* Remove player */
		n = Packet_scanf(&rbuf, "%s", tmp_n);
		if (n == 0) goto rollback;
		else if (n < 0) return(n);

		for (i = 0; i < NumPlayers; i++) {
			if (strcmp(playerlist_name[i], tmp_n)) continue;

			/* Slide the rest down */
			for (n = i; n < NumPlayers - 1; n++) {
				strcpy(playerlist_name[n], playerlist_name[n + 1]);
				strcpy(playerlist[n], playerlist[n + 1]);
			}

			playerlist_name[n][0] = 0;
			NumPlayers--;
			break;
		}
		break;
	}

	fix_playerlist();
	return(1);

	/* Rollback the socket buffer in case the packet isn't complete */
	rollback:
	rbuf.ptr = stored_sbuf_ptr;
	return(0);
}

int Receive_weather_colouring(void) {
	int n;
	char ch;

	if (is_atleast(&server_version, 4, 7, 4, 6, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%c%c", &ch, &col_raindrop, &col_snowflake, &col_sandgrain, &c_sandgrain)) <= 0) return(n);
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c", &ch, &col_raindrop, &col_snowflake)) <= 0) return(n);
	}

	/* hack for gfx_palanim_repaint=no to reduce flickering:
	   weather colouring is sent on day/night change, right before the palette update is sent.
	   (And it is sent once on initial login.)
	   So when we receive this we know that a day/night specific palette update will follow.
	   For that one we use the old, flickering method, otherwise we use smooth repaint method. - C. Blue */
	if (gfx_palanim_repaint_hack_login) gfx_palanim_repaint_hack_login = FALSE; /* So I put a hack in your hack, just to skip weather-col call from initial login */
	else gfx_palanim_repaint_hack = TRUE;

	return(1);
}

int Receive_whats_under_you_feet(void) {
	int n;
	char ch;
	char o_name[ONAME_LEN];
	bool crossmod_item, cant_see, on_pile;

	if (is_atleast(&server_version, 4, 7, 4, 1, 0, 0)) {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%c%I", &ch, &crossmod_item, &cant_see, &on_pile, o_name)) <= 0) return(n);
	} else {
		if ((n = Packet_scanf(&rbuf, "%c%c%c%c%s", &ch, &crossmod_item, &cant_see, &on_pile, o_name)) <= 0) return(n);
	}

	prt_whats_under_your_feet(o_name, crossmod_item, cant_see, on_pile);
	strcpy(whats_under_your_feet, o_name);

	apply_auto_pickup(o_name);

	return(1);
}

int Receive_version(void) {
	int n;
	char ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return(n);
	Send_version();
	return(1);
}

int Receive_sflags(void) {
	int n;
	char ch;

	if ((n = Packet_scanf(&rbuf, "%c%d%d%d%d", &ch, &sflags0, &sflags1, &sflags2, &sflags3)) <= 0) return(n);
	return(1);
}

int Receive_macro_failure(void) {
	int n;
	char ch;

	if ((n = Packet_scanf(&rbuf, "%c", &ch)) <= 0) return(n);

	/* Stop macro execution if we're on safe_macros!
	   There is currently no use for this, just added it in case we ever need it as fail-safe. */
	if (parse_macro && c_cfg.safe_macros) flush_now();

	return(1);
}



int Send_search(void) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_SEARCH)) <= 0) return(n);
	return(1);
}

int Send_walk(int dir) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_WALK, dir)) <= 0) return(n);
	return(1);
}

int Send_run(int dir) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_RUN, dir)) <= 0) return(n);
	return(1);
}

int Send_drop(int item, int amt) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_DROP, item, amt)) <= 0) return(n);
	return(1);
}

int Send_drop_gold(s32b amt) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%d", PKT_DROP_GOLD, amt)) <= 0) return(n);
	return(1);
}

int Send_tunnel(int dir) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_TUNNEL, dir)) <= 0) return(n);
	return(1);
}

int Send_stay(void) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_STAND)) <= 0) return(n);
	return(1);
}
int Send_stay_one(void) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_STAND_ONE)) <= 0) return(n);
	return(1);
}
int Send_stay_auto(void) {
	int n;

	if (is_older_than(&server_version, 4, 7, 4, 4, 0, 0)) return Send_stay();

	if ((n = Packet_printf(&wbuf, "%c", PKT_STAND_AUTO)) <= 0) return(n);
	return(1);
}

static int Send_keepalive(void) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_KEEPALIVE)) <= 0) return(n);
	return(1);
}

int Send_toggle_search(void) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_SEARCH_MODE)) <= 0) return(n);
	return(1);
}

int Send_rest(void) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_REST)) <= 0) return(n);
	return(1);
}

int Send_go_up(void) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_GO_UP)) <= 0) return(n);
	return(1);
}

int Send_go_down(void) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_GO_DOWN)) <= 0) return(n);
	return(1);
}

int Send_open(int dir) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_OPEN, dir)) <= 0) return(n);
	return(1);
}

int Send_close(int dir) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_CLOSE, dir)) <= 0) return(n);
	return(1);
}

int Send_bash(int dir) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_BASH, dir)) <= 0) return(n);
	return(1);
}

int Send_disarm(int dir) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_DISARM, dir)) <= 0) return(n);
	return(1);
}

int Send_wield(int item) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_WIELD, item)) <= 0) return(n);
	return(1);
}

int Send_observe(int item) {
	int n;

#if 0
#ifdef ENABLE_SUBINVEN
	if (using_subinven != -1) {
		/* Hacky encoding */
		if ((n = Packet_printf(&wbuf, "%c%hd", PKT_OBSERVE, item + (using_subinven + 1) * SUBINVEN_INVEN_MUL)) <= 0) return(n);
		return(1);
	}
#endif
#endif
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_OBSERVE, item)) <= 0) return(n);
	return(1);
}

int Send_take_off(int item) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_TAKE_OFF, item)) <= 0) return(n);
	return(1);
}

int Send_take_off_amt(int item, int amt) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_TAKE_OFF_AMT, item, amt)) <= 0) return(n);
	return(1);
}

int Send_destroy(int item, int amt) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_DESTROY, item, amt)) <= 0) return(n);
	return(1);
}

int Send_inscribe(int item, cptr buf) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd%s", PKT_INSCRIBE, item, buf)) <= 0) return(n);
	return(1);
}

int Send_uninscribe(int item) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_UNINSCRIBE, item)) <= 0) return(n);
	return(1);
}

int Send_autoinscribe(int item) {
	int n;

	if (!is_newer_than(&server_version, 4, 5, 5, 0, 0, 0)) return(1);
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_AUTOINSCRIBE, item)) <= 0) return(n);
	return(1);
}

int Send_steal(int dir) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_STEAL, dir)) <= 0) return(n);
	return(1);
}

int Send_quaff(int item) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_QUAFF, item)) <= 0) return(n);
	return(1);
}

int Send_read(int item) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_READ, item)) <= 0) return(n);
	return(1);
}

int Send_aim(int item, int dir) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd%c", PKT_AIM_WAND, item, dir)) <= 0) return(n);
	return(1);
}

int Send_use(int item) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_USE, item)) <= 0) return(n);
	return(1);
}

int Send_zap(int item) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_ZAP, item)) <= 0) return(n);
	return(1);
}

int Send_zap_dir(int item, int dir) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd%c", PKT_ZAP_DIR, item, dir)) <= 0) return(n);
	return(1);
}

int Send_fill(int item) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_FILL, item)) <= 0) return(n);
	return(1);
}

int Send_eat(int item) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_EAT, item)) <= 0) return(n);
	return(1);
}

int Send_activate(int item) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_ACTIVATE, item)) <= 0) return(n);
	return(1);
}

int Send_activate_dir(int item, int dir) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd%c", PKT_ACTIVATE_DIR, item, dir)) <= 0) return(n);
	return(1);
}

int Send_target(int dir) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_TARGET, dir)) <= 0) return(n);
	return(1);
}


int Send_target_friendly(int dir) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_TARGET_FRIENDLY, dir)) <= 0) return(n);
	return(1);
}


int Send_look(int dir) {
	int n;

	if (is_newer_than(&server_version, 4, 4, 9, 3, 0, 0)) {
		if ((n = Packet_printf(&wbuf, "%c%hd", PKT_LOOK, dir)) <= 0) return(n);
	} else {
		if ((n = Packet_printf(&wbuf, "%c%c", PKT_LOOK, dir)) <= 0) return(n);
	}

	return(1);
}

int Send_msg(cptr message) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%S", PKT_MESSAGE, message)) <= 0) return(n);
	return(1);
}

int Send_fire(int dir) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_FIRE, dir)) <= 0) return(n);
	return(1);
}

int Send_throw(int item, int dir) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%c%hd", PKT_THROW, dir, item)) <= 0) return(n);
	return(1);
}

int Send_item(int item) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_ITEM, item)) <= 0) return(n);
	return(1);
}

/* for DISCRETE_SPELL_SYSTEM: DSS_EXPANDED_SCROLLS */
int Send_spell(int item, int spell) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_SPELL, item, spell)) <= 0) return(n);
	return(1);
}

int Send_activate_skill(int mkey, int book, int spell, int dir, int item, int aux) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%c%hd%hd%c%hd%hd", PKT_ACTIVATE_SKILL, mkey, book, spell, dir, item, aux)) <= 0) return(n);
	return(1);
}

int Send_pray(int book, int spell) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_PRAY, book, spell)) <= 0) return(n);
	return(1);
}

#if 0 /* instead, Send_telekinesis is used */
int Send_mind() {
	int n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_MIND)) <= 0) return(n);
	return(1);
}
#endif

int Send_ghost(int ability) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_GHOST, ability)) <= 0) return(n);
	return(1);
}

int Send_map(char mode) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_MAP, mode)) <= 0) return(n);
	return(1);
}

int Send_locate(int dir) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_LOCATE, dir)) <= 0) return(n);
	return(1);
}

int Send_store_command(int action, int item, int item2, int amt, int gold) {
	int  n;

	if ((n = Packet_printf(&wbuf, "%c%hd%hd%hd%hd%d", PKT_STORE_CMD, action, item, item2, amt, gold)) <= 0) return(n);
	return(1);
}

int Send_store_examine(int item) {
	int  n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_STORE_EXAMINE, item)) <= 0) return(n);
	return(1);
}

int Send_store_purchase(int item, int amt) {
	int  n;

	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_PURCHASE, item, amt)) <= 0) return(n);
	return(1);
}

int Send_store_sell(int item, int amt) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_SELL, item, amt)) <= 0) return(n);
	return(1);
}

int Send_store_leave(void) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_STORE_LEAVE)) <= 0) return(n);
	return(1);
}

int Send_store_confirm(void) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_STORE_CONFIRM)) <= 0) return(n);
	return(1);
}

int Send_redraw(char mode) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_REDRAW, mode)) <= 0) return(n);
	return(1);
}

int Send_clear_buffer(void) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_CLEAR_BUFFER)) <= 0) return(n);
	return(1);
}

int Send_clear_actions(void) {
	int n;

	if (is_newer_than(&server_version, 4, 4, 5, 10, 0, 0)) {
		if ((n = Packet_printf(&wbuf, "%c", PKT_CLEAR_ACTIONS)) <= 0) return(n);
	}
	return(1);
}

int Send_special_line(int type, s32b line, char *srcstr) {
	int n;

	if (is_newer_than(&server_version, 4, 7, 4, 5, 0, 0)) {
		if ((n = Packet_printf(&wbuf, "%c%c%d%s", PKT_SPECIAL_LINE, type, line, srcstr ? srcstr : "")) <= 0) return(n); // <- just allow NULL pointer too, just in case..
	} else if (is_newer_than(&server_version, 4, 4, 7, 0, 0, 0)) {
		if ((n = Packet_printf(&wbuf, "%c%c%d", PKT_SPECIAL_LINE, type, line)) <= 0) return(n);
	} else {
		if ((n = Packet_printf(&wbuf, "%c%c%hd", PKT_SPECIAL_LINE, type, line)) <= 0) return(n);
	}

	return(1);
}

int Send_skill_mod(int i) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%d", PKT_SKILL_MOD, i)) <= 0) return(n);
	return(1);
}

int Send_skill_dev(int i, bool dev) {
	int n;

	if (is_newer_than(&server_version, 4, 4, 8, 2, 0, 0)) {
		if ((n = Packet_printf(&wbuf, "%c%d%c", PKT_SKILL_DEV, i, dev)) <= 0) return(n);
	}
	return(1);
}

int Send_party(s16b command, cptr buf) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd%s", PKT_PARTY, command, buf)) <= 0) return(n);
	return(1);
}

int Send_guild(s16b command, cptr buf) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd%s", PKT_GUILD, command, buf)) <= 0) return(n);
	return(1);
}

int Send_guild_config(s16b command, u32b flags, cptr buf) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%d%d%s", PKT_GUILD_CFG, command, flags, buf)) <= 0) return(n);
	return(1);
}

int Send_purchase_house(int dir) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_PURCHASE, dir, 0)) <= 0) return(n);
	return(1);
}

int Send_suicide(void) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_SUICIDE)) <= 0) return(n);

	/* Newer servers require couple of extra bytes for suicide - mikaelh */
	if (is_newer_than(&server_version, 4, 4, 2, 3, 0, 0)) {
		if ((n = Packet_printf(&wbuf, "%c%c", 1, 4)) <= 0)
			return(n);
	}

	return(1);
}

int Send_options(void) {
	int i, n;

	if ((n = Packet_printf(&wbuf, "%c", PKT_OPTIONS)) <= 0) return(n);
	/* Send each option */
	if (is_newer_than(&server_version, 4, 9, 1, 0, 0, 0)) {
		for (i = 0; i < OPT_MAX; i++)
			Packet_printf(&wbuf, "%c", Client_setup.options[i]);
	} else 	if (is_newer_than(&server_version, 4, 5, 8, 1, 0, 1)) {
		for (i = 0; i < OPT_MAX_154; i++)
			Packet_printf(&wbuf, "%c", Client_setup.options[i]);
	} else if (is_newer_than(&server_version, 4, 5, 5, 0, 0, 0)) {
		for (i = 0; i < OPT_MAX_COMPAT; i++)
			Packet_printf(&wbuf, "%c", Client_setup.options[i]);
	} else {
		for (i = 0; i < OPT_MAX_OLD; i++)
			Packet_printf(&wbuf, "%c", Client_setup.options[i]);
	}
	return(1);
}

int Send_screen_dimensions(void) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_SCREEN_DIM, screen_wid, screen_hgt)) <= 0) return(n);
	return(1);
}

int Send_admin_house(int dir, cptr buf) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd%s", PKT_HOUSE, dir, buf)) <= 0) return(n);
	return(1);
}

int Send_master(s16b command, cptr buf) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd%s", PKT_MASTER, command, buf)) <= 0) return(n);
	return(1);
}

int Send_King(byte type) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_KING, type)) <= 0) return(n);
	return(1);
}

int Send_spike(int dir) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_SPIKE, dir)) <= 0) return(n);
	return(1);
}

int Send_raw_key(int key) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%c", PKT_RAW_KEY, key)) <= 0) return(n);
	return(1);
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
		return(n);

	/* Shift ping_times */
	for (i = 59; i > 0; i--)
		ping_times[i] = ping_times[i - 1];
	ping_times[i] = -1;

	return(1);
}

int Send_account_info(void) {
	int n;

	if (!is_newer_than(&server_version, 4, 4, 2, 2, 0, 0)) return(1);
	if ((n = Packet_printf(&wbuf, "%c", PKT_ACCOUNT_INFO)) <= 0) return(n);
	return(1);
}

int Send_change_password(char *old_pass, char *new_pass) {
	int n;

	if (!is_newer_than(&server_version, 4, 4, 2, 2, 0, 0)) return(1);
	if ((n = Packet_printf(&wbuf, "%c%s%s", PKT_CHANGE_PASSWORD, old_pass, new_pass)) <= 0) return(n);
	return(1);
}

#ifdef ENABLE_SUBINVEN
/* Stow item */
int Send_si_move(int item, int amt) {
	int n;

	if (!is_newer_than(&server_version, 4, 7, 4, 4, 0, 0)) return(1);
	if (is_older_than(&server_version, 4, 9, 2, 0, 0, 0)) {
		if ((n = Packet_printf(&wbuf, "%c%hd", PKT_SI_MOVE, item)) <= 0) return(n); //discard amt, always move full stack
	} else {
		if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_SI_MOVE, item, amt)) <= 0) return(n);
	}
	return(1);
}
/* Unstow item */
int Send_si_remove(int item, int amt) {
	int n, islot = item / SUBINVEN_INVEN_MUL - 1;

	if (!is_newer_than(&server_version, 4, 7, 4, 4, 0, 0)) return(1);
	if (is_older_than(&server_version, 4, 9, 2, 0, 0, 0)) {
		if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_SI_REMOVE, (short int)islot, (short int)(item % SUBINVEN_INVEN_MUL))) <= 0) return(n); //discard amt, always move full stack
	} else {
		if ((n = Packet_printf(&wbuf, "%c%hd%hd%hd", PKT_SI_REMOVE, (short int)islot, (short int)(item % SUBINVEN_INVEN_MUL), amt)) <= 0) return(n);
	}
	return(1);
}
#endif

int Send_version(void) {
	int n;

	if (!is_newer_than(&server_version, 4, 8, 0, 0, 0, 0)) return(1);
	if (!is_newer_than(&server_version, 4, 9, 1, 0, 0, 0)) {
		if ((n = Packet_printf(&wbuf, "%c%s%s", PKT_VERSION, longVersion, os_version)) <= 0) return(n);
	} else {
		int cnt, sum, min, max, i, avg;

		/* Find min and max and calculate avg */
		cnt = sum = 0;
		min = max = -1;
		for (i = 0; i < 60; i++) {
			if (ping_times[i] > 0) {
				if (min == -1) min = ping_times[i];
				else if (ping_times[i] < min) min = ping_times[i];

				if (max == -1) max = ping_times[i];
				else if (ping_times[i] > max) max = ping_times[i];

				cnt++;
				sum += ping_times[i];
			}
		}
		if (cnt) avg = sum / cnt;
		else avg = -1;

		if (!is_newer_than(&server_version, 4, 9, 2, 0, 0, 0)) {
			if ((n = Packet_printf(&wbuf, "%c%s%s%d", PKT_VERSION, longVersion, os_version, avg)) <= 0) return(n);
		} else if (is_older_than(&server_version, 4, 9, 2, 1, 0, 1)) {
			if ((n = Packet_printf(&wbuf, "%c%s%s%d%d%d%d%s%d", PKT_VERSION, longVersion, os_version, avg, guide_lastline, VERSION_BRANCH, VERSION_BUILD, CLIENT_VERSION_TAG, sys_lang)) <= 0) return(n);
		} else if (is_older_than(&server_version, 4, 9, 2, 1, 0, 2)) {
			if ((n = Packet_printf(&wbuf, "%c%s%s%d%d%d%d%s%d%s%s%s%s", PKT_VERSION, longVersion, os_version, avg, guide_lastline, VERSION_BRANCH, VERSION_BUILD, CLIENT_VERSION_TAG, sys_lang,
			    cfg_soundpack_name, cfg_soundpack_version, cfg_musicpack_name, cfg_musicpack_version)) <= 0) return(n);
		} else {
			if ((n = Packet_printf(&wbuf, "%c%s%s%d%d%s%d%s%s%s%s", PKT_VERSION, longVersion, os_version, avg, guide_lastline, CLIENT_VERSION_TAG, sys_lang,
			    cfg_soundpack_name, cfg_soundpack_version, cfg_musicpack_name, cfg_musicpack_version)) <= 0) return(n);
		}
	}
	return(1);
}

int Send_plistw_notify(bool on) {
	int n;

	if (is_older_than(&server_version, 4, 9, 0, 7, 0, 0)) return(1);
	if ((n = Packet_printf(&wbuf, "%c%c", PKT_PLISTW_NOTIFY, on)) <= 0) return(n);
	return(1);
}

int Send_unknownpacket(int type, int prev_type) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_UNKNOWNPACKET, type, prev_type)) <= 0) return(n);
	return(1);
}



/* ------------------------------------------------------------------------- */



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
 *
 * Notes: ticks will count up continuously until overflowing int in 100ms intervals.
 *        ticks10 just run from 0 to 9 in 10ms intervals.
 *        newticks will update from oldticks every 1 second (aka 10 ticks).
 */
void update_ticks(void) {
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

//#ifdef ENABLE_JUKEBOX
 #ifdef SOUND_SDL
	/* Track jukebox music position in seconds */
	if (newticks != oldticks) {
		oldticks = newticks;
		if (jukebox_screen) update_jukebox_timepos();
	}
 #endif
//#endif

#ifdef META_PINGS
 #ifdef META_DISPLAYPINGS_LATER
	if (refresh_meta_once) {
		refresh_meta_once = FALSE;

		/* This is the one line of code inside Term_clear() that actually "enables" the screen again to get written on again, making the meta_display visible again oO - wtf */
		Term->total_erase = TRUE;

		display_experimental_meta();
		/* hack: hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;
		//Term_set_cursor(0);
	}
 #endif
	/* Ping meta-listed servers every 3 s, fetch results after half the time already (30/n ds) */
	if (meta_pings_servers && meta_pings_ticks != ticks && !(ticks % (30 / 6))) do_meta_pings();
#endif

	/* Find the new least significant digit of the ticks */
	newticks += cur_time.tv_usec / 100000;
	ticks10 = (cur_time.tv_usec / 10000) % 10;

	/* Track hunger colour animation in deciseconds */
	if (newticks != oldticksds) {
		oldticksds = newticks;
		if (food_warn_once_timer) {
			food_warn_once_timer--;
			if (!food_warn_once_timer) prt_hunger(-1);
		}
	}

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
void do_keepalive(void) {
	/*
	 * Check to see if it has been 2 seconds since we last sent anything.  Assume
	 * that each game turn lasts 100 ms.
	 */
	if ((ticks - last_send_anything) >= 20)
		Send_keepalive();
}

#define FL_SPEED 1

void do_flicker(void) {
	static int flticks = 0;

	if (ticks-flticks < FL_SPEED) return;
	flicker();
	flticks = ticks;
}

void do_mail(void) {
#ifdef CHECK_MAIL
 #ifdef SET_UID
	static int mailticks = 0;
	static struct timespec lm;
	char mpath[160],buffer[160];

	int uid;
	struct passwd *pw;

  #ifdef NETBSD
	strcpy(mpath, "/var/mail/");
  #else
	strcpy(mpath, "/var/spool/mail/");
  #endif

	uid = getuid();
	pw = getpwuid(uid);
	if (pw == (struct passwd*)NULL)
		return;
	strcat(mpath, pw->pw_name);

	if (ticks-mailticks >= 300) {	/* testing - too fast */
		struct stat inf;
		if (!stat(mpath, &inf)) {
  #ifndef _POSIX_C_SOURCE
			if (inf.st_size != 0) {
				if (inf.st_mtimespec.tv_sec > lm.tv_sec || (inf.st_mtimespec.tv_sec == lm.tv_sec && inf.st_mtimespec.tv_nsec > lm.tv_nsec)) {
					lm.tv_sec = inf.st_mtimespec.tv_sec;
					lm.tv_nsec = inf.st_mtimespec.tv_nsec;
					sprintf(buffer, "\377yYou have new mail in %s.", mpath);
					c_msg_print(buffer);
				}
			}
  #else
			if (inf.st_size != 0) {
				if (inf.st_mtime > lm.tv_sec || (inf.st_mtime == lm.tv_sec && inf.st_mtimensec > lm.tv_nsec)) {
					lm.tv_sec = inf.st_mtime;
					lm.tv_nsec = inf.st_mtimensec;
					sprintf(buffer, "\377yYou have new mail in %s.", mpath);
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
   do_ping is at least called every frame (1/cfg_client_fps) in the main loop,
   but may also get called more frequently as it is additionally called from other locations too, eg sync_sleep() which has a 1/1000 ms frequency.
   So for timed actions called in here, make sure to depend on ticks etc instead of just this function call. */
void do_ping(void) {
	static int last_ping = 0, last_ticks10 = 0;
	static int time_stamp_hour = -1, time_stamp_min = -1;


	/* ------------------------------------------------------------------------------------------------------------- */
	/* Filter for everything else done in this function:
	   As this function may be called more often than 1/cfg_client_fps (10 ms),
	   quit here unless a true 10ms interval has passed.
	   (10 ms independantly of actual cfg_client_fps value, but just dependant on system clock!) */
	if (ticks10 == last_ticks10) return;
	last_ticks10 = ticks10;
	/* ------------------------------------------------------------------------------------------------------------- */


	/* Every 10 ticks = 1 second update interval */
	if (lagometer_enabled && (ticks - last_ping >= 10)) {
		last_ping = ticks;

		/* Update the lag-o-meter just before sending a new ping */
		update_lagometer();

		Send_ping();

#if 0 /* testing palette animation :O - abuserino */
 #if defined(WINDOWS) || defined(USE_X11)
  #warning " << animate_palette >> "
		animate_palette();
 #endif
#endif
	}

	/* abusing it for weather for now - C. Blue */
	do_weather(noweather_mode || c_cfg.no_weather);
	if (!noweather_mode && !c_cfg.no_weather) {
	    // && !screen_icky) {  -- mistake? after all, maybe all tiles are supposed to be restored because weather ended, while we're in an icky screen!

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
				} else if (weather_type % 10 == 3) { //sandstorm, uses same intensity as snowstorm basically, ie there is no 'light' sandstorm
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
				} else if (weather_type % 10 == 3) { //sandstorm - we use snow intensity and sfx too
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

		if (ctl->tm_hour != time_stamp_hour || (ctl->tm_min == 30 && time_stamp_min != 30)) {
			if (time_stamp_hour != -1) c_msg_format("\374\376\377y[%04d/%02d/%02d - %02d:%02dh]", 1900 + ctl->tm_year, ctl->tm_mon + 1, ctl->tm_mday, ctl->tm_hour, ctl->tm_min);
			time_stamp_hour = ctl->tm_hour;
			time_stamp_min = ctl->tm_min;
		}
	}

	/* Update the clone-map;
	   * not sure where this best belongs to - Lightman
	   -- moved it from Term_inkey() to here - C. Blue
	 */
	refresh_clone_map();

#ifdef WINDOWS
	/* Check if PNG screenshot returned successfully */
	if (screenshotting) {
		/* Check every 10ms * '10' = 100 ms */
		if (!(--screenshotting % 10)) screenshot_result_check();
	}
#endif
}

#ifdef META_PINGS
 #ifdef WINDOWS
  #include <process.h>	/* use spawn() instead of normal system() (WINE bug/Win inconsistency even maybe) */
 #endif

 /* Enable debug mode? */
 #ifdef TEST_CLIENT
  //#define DEBUG_PING
 #endif

/* Note: At this early stage of the game (viewing meta server list) do_keepalive() and do_ping() are not yet called,
   so this function must be called in do_flicker() or update_ticks() instead, of which update_ticks() is preferable
   as its calling frequency is fixed and predictable (100ms).
   We are called every 500ms. */
static void do_meta_pings(void) {
	static int i, r;
	static char path[1024], *c;
#ifdef WINDOWS
	char buf[1024]; /* if we use windows-specific ReadFile() we might read the whole file at once */
#else
	char buf[MAX_CHARS_WIDE]; /* read line by line */
#endif
	static FILE *fff;
	static char alt = 1, reload_metalist = 0; /* <- Only truly needed static var, the rest is static just for execution time optimization */
	static int method = 0;

#ifdef META_DISPLAYPINGS_LATER
	bool received_ok[META_PINGS] = { FALSE };
#endif
	bool refreshed_meta = FALSE;


	if (!method) {
		if (access("ping-wrap.exe", F_OK) == 0) method = 1;
		/* If access() doesn't work on native compilation (eg in VC), use my_fexist() instead, or try this:
		#ifdef WIN32
		 #include <io.h>
		 #define F_OK 0
		 #define access _access
		#endif
		*/
		else method = 2;
	}

	/* Only call us once every intended interval time */
	meta_pings_ticks = ticks;

#ifdef EXPERIMENTAL_META
 #ifndef META_DISPLAYPINGS_LATER
	/* Refresh metaserver list every n*500 ms.
	   Note that for now we don't re-read the actual servers for the cause of pinging.
	   Rather, we assume the servers don't change and we'll just continue to ping those.
	   We only re-read the metaserver list to update the amount/names of players on the servers. */
	reload_metalist = (reload_metalist + 1) % 18; //18: 9s, ie the time needed for 3 ping refreshs
	if (!reload_metalist && meta_connect() && meta_read_and_close()) display_experimental_meta();
 #endif
#endif

	/* Alternate function: 1) send out pings, 2) read results */
	alt = (alt + 1) % 7;
	if (alt != 0 && alt != 2) return; //skipped a beat!

	for (i = 0; i < meta_pings_servers; i++) {
		/* Build the temp filename for ping results -- would probably prefer to use OS' actual tempfs, but Windows, pft */
#if defined(WINDOWS) && defined(WINDOWS_USE_TEMP)
		/* Use official Windows TEMP folder instead? */
		if (getenv("HOMEDRIVE") && getenv("HOMEPATH")) {
			strcpy(path, getenv("HOMEDRIVE"));
			strcat(path, getenv("HOMEPATH"));
			strcat(path, format("\\__ping_%s.tmp", meta_pings_server_name[i]));
		} else
#endif
		path_build(path, 1024, ANGBAND_DIR_USER, format("__ping_%s.tmp", meta_pings_server_name[i]));

		/* Send a ping to each distinct server name, allowing for max 1000ms */
		if (alt == 2) {
 #ifdef WINDOWS
			remove(path);//in case game was ctrl+c'ed while pinging on previous startup

			/* All "simple" solutions don't work, because START /b and similar commands do not work together with redirecting the output,
			   _spawnl() even with _P_NOWAIT will also not become asynchronous, neither will system(), beause quote:
			    "The default behaviour of START is to instantiate a new process that runs in parallel with the main process.
			     For arcane technical reasons, this does not work for some types of executable,
			     in those cases the process will act as a blocker, pausing the main script until it's complete." */
		/* Note: Another way might be to create a named pipe (CallNamedPipeA()) */
  #if !defined(META_PINGS_CREATEFILE) /* use ping-wrap.exe -- best solution. According to Sav, two terms quickly appear before main window opens, but that's it. */
			STARTUPINFO si;
			PROCESS_INFORMATION pi;

			ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			ZeroMemory(&pi, sizeof(pi));

			/* Don't show a console window */
			si.wShowWindow = SW_HIDE;
			//si.dwFlags = STARTF_USESHOWWINDOW; // causes flickering up of a popup shortly on WINE at leas
			//u32b dwCreationFlags = DETACHED_PROCESS | CREATE_NO_WINDOW; // DETACHED_PROCESS supposedly works specifically on Win98 to hide the window
			u32b dwCreationFlags = CREATE_NO_WINDOW; // DETACHED_PROCESS supposedly works specifically on Win98 to hide the window

			/* Check for ping-wrap.exe's existance */
			if (method == 1)
				CreateProcess( NULL, format("ping-wrap.exe %s %s%s", meta_pings_server_name[i], path, meta_pings_xpath),
				    NULL, NULL, FALSE, dwCreationFlags, NULL, NULL, &si, &pi);
			/* Fall back to cmd usage instead (causes terms to pop up once on start) */
			else
				CreateProcess( NULL, format("cmd.exe /c \"ping -n 1 -w 1000 %s > %s\"", meta_pings_server_name[i], path),
				    NULL, NULL, FALSE, dwCreationFlags, NULL, NULL, &si, &pi);
  #else /* replace the pipe '>' by manually setting a file handle for stdout/stderr of createprocess() */
    /* problem: the _ping_..server..tmp files are 0 B on a real Win 7 (works on Wine-Win7)...wtfff */
   #if 1 /* do we reset the handle on reading the result? then reopen it here again */
			fhan[i] = CreateFile(path,
			    FILE_WRITE_DATA, //FILE_APPEND_DATA,
#if 0
			    0, //or:
#else
			    FILE_SHARE_WRITE | FILE_SHARE_READ, // should be 0 tho
#endif
			    &sa[i],
			    OPEN_ALWAYS,
			    FILE_ATTRIBUTE_NORMAL,
			    NULL);

			si[i].hStdInput = NULL;
			si[i].hStdError = fhan[i];
			si[i].hStdOutput = fhan[i];
   #endif

			ZeroMemory(&pi[i], sizeof(PROCESS_INFORMATION));
			/* At least CreateProcess() will run within our context/working folder -_-.. */
   #if 0
			r = CreateProcess( NULL,
   #else
			CreateProcess( NULL,
   #endif
			    format("ping -n 1 -w 1000 %s", meta_pings_server_name[i]), //commandline -- WORKS!
			    //format("ping -n 1 -w 1000 %s > %s", meta_pings_server_name[i], path), //commandline --doesnt work, cause of piping it seems
			    NULL,	// Process handle not inheritable
			    NULL,	// Thread handle not inheritable
			    FALSE,	// Set handle inheritance to FALSE
			    CREATE_NO_WINDOW,
			    NULL,	// Use parent's environment block
			    NULL,	// Use parent's starting directory
			    &si[i],	// Pointer to STARTUPINFO structure
			    &pi[i]);	// Pointer to PROCESS_INFORMATION structure
  #endif
 #else /* assume POSIX */
			r = system(format("ping -c 1 -w 1 %s > %s &", meta_pings_server_name[i], path));
  #ifdef DEBUG_PING
printf("SENT  i=%d : <ping -c 1 -w 1 %s > %s &>\n", i, meta_pings_server_name[i], path);
  #endif
			(void)r; //slay compiler warning;
 #endif
		}

		/* Retrieve ping results */
		else {
			bool no_ttl_line = TRUE;

			/* Assume our pinging result file is still being written to for some weird slowness OS reason -_- */
			meta_pings_stuck[i] = TRUE;

			/* Check for duplicate first. If we're just a duplicate server, re-use the original ping result we retrieved so far. */
			if (meta_pings_server_duplicate[i] != -1) {
				/* If original server is still stuck at "pinging..", don't change our status yet either.. */
				if (meta_pings_stuck[meta_pings_server_duplicate[i]]) continue;

				/* Store ping result */
				meta_pings_result[i] = meta_pings_result[meta_pings_server_duplicate[i]];

				/* Live-update the meta server list -- pfft, actually we don't even need meta_pings_results[] */
#ifndef META_DISPLAYPINGS_LATER
				call_lua(0, "meta_add_ping", "(d,d)", "d", i, meta_pings_result[i], &r);
				Term_fresh();
				/* hack: hide cursor */
				Term->scr->cx = Term->wid;
				Term->scr->cu = 1;
				Term_set_cursor(0);
				//Term_xtra(TERM_XTRA_SHAPE, 1);
#else
				received_ok[i] = TRUE;
#endif
				continue;
			}

   #ifdef META_PINGS_CREATEFILE
DWORD exit_code;
GetExitCodeProcess(pi[i].hProcess, &exit_code);
if (exit_code != STILL_ACTIVE) {
    #if 1 /* CreateProcess() stuff */
			/* Clear up spawned ping process */
			CloseHandle(pi[i].hProcess);
			CloseHandle(pi[i].hThread);
    #endif
    #if 1 /* CreateFile() stuff */
			/* Clear up file (write) access from spawned ping process */
			if (fhan[i]) {
				CloseHandle(fhan[i]);
				fhan[i] = NULL;
			}
    #endif
} else continue; /* ping process hasn't finished yet! skip. */
   #endif

   //#ifndef WINDOWS /* use C functions */
   #if 1
			/* Access ping response file */
			fff = my_fopen(path, "r");
			if (!fff) continue; /* Sort of paranoia? */

			/* Parse OS specific 'ping' command response; win: 'time=NNNms', posix: 'time=NNN.NN ms',
			   BUT.. have to watch out that "time" label can be OS-language specific!
			   For that reason we first look for 'ttl' (posix) and 'TTL' (windows) which are always the same. */
			while (my_fgets(fff, buf, MAX_CHARS_WIDE) == 0) {
  #ifdef DEBUG_PING
printf("REPLY i=%d : <%s>\n", i, buf);
  #endif
				meta_pings_stuck[i] = FALSE; /* Yay, we can read the results finally.. */

//printf("p[%d: <%s>\n", i, buf);
				if (!my_strcasestr(buf, "ttl")) continue;
				no_ttl_line = FALSE;

#if 0
/* 0'ed for WINDOWS because: TheScar had a strange Windows build or settings which result in garbled encoding if the ping line was caused by ping-wrap.exe,
while it causes normal ungarbled encoding if the ping line was entered in a terminal manually o_O.
The garabage would include "ms", while leaving "=" symbols untouched, and interestingly "TTL" also is untouched, so we parse for the 2nd "=" instead as a workaround.
Also, cyrillic systems seem to use other letters for 'ms' instead, so we cannot use that. "TTL" is same though for some reason. */
				/* We found a line containing a response time. Now we look for 'ms' and go backwards till '='. */
				c = strstr(buf, "ms");
				if (!c) continue; //should be paranoia at this point
				while (*c != '=') {
					c--;
					if (c == buf) break; /* Paranoia - Supa safety 1/2 */
				}
				if (c == buf) break; /* Paranoia - Supa safety 1/2 */
				c++; //yuck
#else /* ..so as mentioned above, we parse for "n=" */
 #ifndef WINDOWS
				/* Go back to '=' from end of the line, and we are at the ping time on POSIX */
				c = &buf[strlen(buf) - 1];
 #else
				/* Look for the '=' before the 'TTL', which designates the ping time on Windows */
				c = my_strcasestr(buf, "ttl"); /* we already know that this exists, or we wouldn't be here, so no further check needed.. hey, look, a black swan, neat! */
 #endif
				while (*c != '=') {
					c--;
					if (c == buf) break; /* Paranoia - Supa safety 1/2 */
				}
				if (c == buf) break; /* Paranoia - Supa safety 1/2 */
				c++; //yuck
#endif
				/* Grab result */
				r = atoi(c);
				break;
			}

			my_fclose(fff);
			//remove(path);
   #else /* use Windows specific functions */
			long unsigned int read;

			/* Access ping response file */
			/* Parse OS specific 'ping' command response; win: 'time=NNNms', posix: 'time=NNN.NN ms',
			   BUT.. have to watch out that "time" label can be OS-language specific!
			   For that reason we first look for 'ttl' (posix) and 'TTL' (windows) which are always the same. */
			//while (TRUE) {
			ReadFile(fhan[i], buf, 1024, &read, NULL);
printf("<<%s>>\n", buf);
			if (strlen(buf)) {
				meta_pings_stuck[i] = FALSE; /* Yay, we can read the results finally.. */

				if (!my_strcasestr(buf, "ttl")) continue;

				/* We found a line containing a response time. Now we look for 'ms' and go backwards till '='. */
				c = strstr(buf, "ms");
printf("<%s>\n", c);
				if (!c) continue; //should be paranoia at this point
				while (*c != '=') {
					c--;
					if (c == buf) break; /* Paranoia - Supa safety 1/2 */
				}
				if (c == buf) break; /* Paranoia - Supa safety 1/2 */
				c++; //yuck

				/* Grab result */
				r = atoi(c);
printf("<%d>\n", r);
			}

			my_fclose(fff);
			//remove(path);
   #endif

			/* Assume timeout/unresponsive,
			   except if file is being written to right now but not yet finished, ie 0 Bytes long for us? Wait more patiently aka retry next time.
			   This may happen at least on Wine for the first two pings, until everything has been cached or sth, dunno really.. oO */
			if (meta_pings_stuck[i]) continue; /* Don't assume anything, but just wait for now */
			if (no_ttl_line) {
				meta_pings_stuck[i] = TRUE;
				continue;
			}
			meta_pings_result[i] = -1; /* Assume timeout */

			/* Store ping result */
			meta_pings_result[i] = r;

			/* Live-update the meta server list -- pfft, actually we don't even need meta_pings_results[] */
#ifndef META_DISPLAYPINGS_LATER
			call_lua(0, "meta_add_ping", "(d,d)", "d", i, meta_pings_result[i], &r);
			Term_fresh();
			/* hack: hide cursor */
			Term->scr->cx = Term->wid;
			Term->scr->cu = 1;
			Term_set_cursor(0);
			//Term_xtra(TERM_XTRA_SHAPE, 1);
#else
			received_ok[i] = TRUE;
#endif
		}
	}

#ifdef META_DISPLAYPINGS_LATER
	/* Display all ping results now in a batch, to reduce write operations to screen and
	   keep delays after refreshing the meta list until ping times are re-displayed to a minimum. */

	if (alt) return;

	/* Refresh metaserver list every n*500 ms.
	   Note that for now we don't re-read the actual servers for the cause of pinging.
	   Rather, we assume the servers don't change and we'll just continue to ping those.
	   We only re-read the metaserver list to update the amount/names of players on the servers. */
	reload_metalist = (reload_metalist + 1) % 3; //3: 9s, ie the time needed for 3 ping refreshs
	if (!reload_metalist && meta_connect() && meta_read_and_close()) {
		Term_clear();
		display_experimental_meta();
		refreshed_meta = TRUE;
	}
 #if 0 /* No need to refresh the meta list every 3s, just need refresh_meta_once */
	else display_experimental_meta();
 #endif
	for (i = 0; i < meta_pings_servers; i++) {
 #if 0
		/* Just blank out any servers still stuck at 'pinging...' */
		if (!received_ok[i]) continue;
 #else
		/* Display some kind of hint for servers who haven't responded */
		if (!received_ok[i]) {
  #if 1
			/* Actually wait REALLY long (9s) Instead of just for one pinging-interval (1000ms)? */
			if (refreshed_meta)
  #endif
			call_lua(0, "meta_add_ping", "(d,d)", "d", i, -1, &r);
			continue;
		}
 #endif
		call_lua(0, "meta_add_ping", "(d,d)", "d", i, meta_pings_result[i], &r);
	}
	Term_fresh();
	/* hack: hide cursor */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;
	Term_set_cursor(0);
	//Term_xtra(TERM_XTRA_SHAPE, 1);
#endif
}
#endif



/* ------------------------------------------------------------------------- */



int Send_sip(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_SIP)) <= 0) return(n);
	return(1);
}

int Send_telekinesis(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_TELEKINESIS)) <= 0) return(n);
	return(1);
}

int Send_BBS(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_BBS)) <= 0) return(n);
	return(1);
}

int Send_wield2(int item) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_WIELD2, item)) <= 0) return(n);
	return(1);
}

int Send_wield3(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_WIELD3)) <= 0) return(n);
	return(1);
}

int Send_cloak(void) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c", PKT_CLOAK)) <= 0) return(n);
	return(1);
}

int Send_inventory_revision(int revision) {
	int	n;
	if ((n = Packet_printf(&wbuf, "%c%d", PKT_INVENTORY_REV, revision)) <= 0) return(n);
	return(1);
}

int Send_force_stack(int item) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd", PKT_FORCE_STACK, item)) <= 0) return(n);
	return(1);
}
int Send_split_stack(int item, int amt) {
	int n;

	if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_SPLIT_STACK, item, amt)) <= 0) return(n);
	return(1);
}

int Send_request_key(int id, char key) {
	int n;
	if ((n = Packet_printf(&wbuf, "%c%d%c", PKT_REQUEST_KEY, id, key)) <= 0) return(n);
	return(1);
}
int Send_request_amt(int id, int num) {
	int n;
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_REQUEST_AMT, id, num)) <= 0) return(n);
	return(1);
}
int Send_request_num(int id, int num) {
	int n;
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_REQUEST_NUM, id, num)) <= 0) return(n);
	return(1);
}
int Send_request_str(int id, char *str) {
	int n;
	if ((n = Packet_printf(&wbuf, "%c%d%s", PKT_REQUEST_STR, id, str)) <= 0) return(n);
	return(1);
}
int Send_request_cfr(int id, int cfr) {
	int n;
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_REQUEST_CFR, id, cfr)) <= 0) return(n);
	return(1);
}

/* Resend F:/R:/K:/U: definitions, used after a font change. */
#define SEND_CLIENT_SETUP_FINISH Net_flush(); sync_sleep(20); /* min is 5ms */
int Send_client_setup(void) {
	int n, i;

	if (!is_newer_than(&server_version, 4, 6, 1, 2, 0, 0)) return(-1);

#if 1 /* send it all at once (too much for buffers on Windows) */
	if ((n = Packet_printf(&wbuf, "%c", PKT_CLIENT_SETUP)) <= 0) return(n);

	char32_t max_char = 0;

	/* Send the "unknown" redefinitions */
	for (i = 0; i < TV_MAX; i++) {
		/* 4.8.1 and newer servers communicate using 32bit character size. */
		if (is_atleast(&server_version, 4, 8, 1, 0, 0, 0))
			Packet_printf(&wbuf, "%c%u", Client_setup.u_attr[i], Client_setup.u_char[i]);
		else
			Packet_printf(&wbuf, "%c%c", Client_setup.u_attr[i], Client_setup.u_char[i]);

		if (max_char < Client_setup.u_char[i]) max_char = Client_setup.u_char[i];
	}

	/* Send the "feature" redefinitions */
	for (i = 0; i < (is_atleast(&server_version, 4, 9, 2, 1, 0, 3) ? MAX_F_IDX : MAX_F_IDX_COMPAT); i++) {
		/* 4.8.1 and newer servers communicate using 32bit character size. */
		if (is_atleast(&server_version, 4, 8, 1, 0, 0, 0))
			Packet_printf(&wbuf, "%c%u", Client_setup.f_attr[i], Client_setup.f_char[i]);
		else
			Packet_printf(&wbuf, "%c%c", Client_setup.f_attr[i], Client_setup.f_char[i]);

		if (max_char < Client_setup.f_char[i]) max_char = Client_setup.f_char[i];
	}

	/* Send the "object" redefinitions */
	for (i = 0; i < MAX_K_IDX; i++) {
		/* 4.8.1 and newer servers communicate using 32bit character size. */
		if (is_atleast(&server_version, 4, 8, 1, 0, 0, 0))
			Packet_printf(&wbuf, "%c%u", Client_setup.k_attr[i], Client_setup.k_char[i]);
		else
			Packet_printf(&wbuf, "%c%c", Client_setup.k_attr[i], Client_setup.k_char[i]);

		if (max_char < Client_setup.k_char[i]) max_char = Client_setup.k_char[i];
	}

	/* Send the "monster" redefinitions */
	for (i = 0; i < MAX_R_IDX; i++) {
		/* 4.8.1 and newer servers communicate using 32bit character size. */
		if (is_atleast(&server_version, 4, 8, 1, 0, 0, 0))
			Packet_printf(&wbuf, "%c%u", Client_setup.r_attr[i], Client_setup.r_char[i]);
		else
			Packet_printf(&wbuf, "%c%c", Client_setup.r_attr[i], Client_setup.r_char[i]);

		if (max_char < Client_setup.r_char[i]) max_char = Client_setup.r_char[i];
	}

	/* Calculate and update minimum character transfer bytes */
	Client_setup.char_transfer_bytes = 0;
	for ( ; max_char != 0; max_char >>= 8 ) {
		Client_setup.char_transfer_bytes += 1;
	}
#else /* send the bigger ones in chunks */
	/* Send the "unknown" redefinitions */
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_U, 0, TV_MAX)) <= 0) return(n);
	for (i = 0; i < TV_MAX; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.u_attr[i], Client_setup.u_char[i]);
	SEND_CLIENT_SETUP_FINISH

	/* Send the "feature" redefinitions */
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_F, 0, MAX_F_IDX)) <= 0) return(n);
	for (i = 0; i < MAX_F_IDX; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.f_attr[i], Client_setup.f_char[i]);
	SEND_CLIENT_SETUP_FINISH

	/* Send the "object" redefinitions */
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_K, 0, 256)) <= 0) return(n);
	for (i = 0; i < 256; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.k_attr[i], Client_setup.k_char[i]);
	SEND_CLIENT_SETUP_FINISH
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_K, 256, 512)) <= 0) return(n);
	for (i = 256; i < 512; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.k_attr[i], Client_setup.k_char[i]);
	SEND_CLIENT_SETUP_FINISH
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_K, 512, 768)) <= 0) return(n);
	for (i = 512; i < 768; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.k_attr[i], Client_setup.k_char[i]);
	SEND_CLIENT_SETUP_FINISH
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_K, 768, 1024)) <= 0) return(n);//MAX_K_IDX_COMPAT
	for (i = 768; i < 1024; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.k_attr[i], Client_setup.k_char[i]);
	SEND_CLIENT_SETUP_FINISH
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_K, 1024, 1280)) <= 0) return(n);//MAX_K_IDX
	for (i = 1024; i < 1280; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.k_attr[i], Client_setup.k_char[i]);
	SEND_CLIENT_SETUP_FINISH

	/* Send the "monster" redefinitions */
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_R, 0, 256)) <= 0) return(n);
	for (i = 0; i < 256; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.r_attr[i], Client_setup.r_char[i]);
	SEND_CLIENT_SETUP_FINISH
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_R, 256, 512)) <= 0) return(n);
	for (i = 256; i < 512; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.r_attr[i], Client_setup.r_char[i]);
	SEND_CLIENT_SETUP_FINISH
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_R, 512, 768)) <= 0) return(n);
	for (i = 512; i < 768; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.r_attr[i], Client_setup.r_char[i]);
	SEND_CLIENT_SETUP_FINISH
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_R, 768, 1024)) <= 0) return(n);//MAX_R_IDX_COMPAT
	for (i = 768; i < 1024; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.r_attr[i], Client_setup.r_char[i]);
	SEND_CLIENT_SETUP_FINISH
	if ((n = Packet_printf(&wbuf, "%c%d%d", PKT_CLIENT_SETUP_R, 1024, 1280)) <= 0) return(n);//MAX_R_IDX
	for (i = 1024; i < 1280; i++)
		Packet_printf(&wbuf, "%c%c", Client_setup.r_attr[i], Client_setup.r_char[i]);
	SEND_CLIENT_SETUP_FINISH
#endif

	return(1);
}

int Send_audio(void) {
	int n;

	if (is_older_than(&server_version, 4, 7, 3, 0, 0, 0)) return(-1);

	if (is_older_than(&server_version, 4, 9, 2, 1, 0, 1)) {
		if ((n = Packet_printf(&wbuf, "%c%hd%hd", PKT_AUDIO, audio_sfx, audio_music)) <= 0) return(n);
	} else {
		if ((n = Packet_printf(&wbuf, "%c%hd%hd%s%s%s%s", PKT_AUDIO, audio_sfx, audio_music,
		    cfg_soundpack_name, cfg_soundpack_version, cfg_musicpack_name, cfg_musicpack_version)) <= 0) return(n);
	}
	return(1);
}

int Send_font(void) {
	int n;
	char fname[1024] = { 0 }; //init for GCU-only mode: there is no font name available

	if (is_older_than(&server_version, 4, 8, 1, 2, 0, 0)) return(-1);

#if defined(WINDOWS) || defined(USE_X11)
 #if defined(WINDOWS) && defined(USE_LOGFONT)
	if (use_logfont) sprintf(fname, "<LOGFONT>%dx%d", win_get_logfont_w(0), win_get_logfont_h(0));
	else
 #endif
	get_term_main_font_name(fname);
#endif
	//&graphics_tile_wid, &graphics_tile_hgt)) {
	//td->font_wid; td->font_hgt;
#ifdef USE_GRAPHICS
	if ((n = Packet_printf(&wbuf, "%c%hd%s%s", PKT_FONT, use_graphics, graphic_tiles, fname)) <= 0) return(n);
#else
	if ((n = Packet_printf(&wbuf, "%c%hd%s%s", PKT_FONT, use_graphics, "NO_GRAPHICS", fname)) <= 0) return(n);
#endif
	return(1);
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

	return(us);
}
