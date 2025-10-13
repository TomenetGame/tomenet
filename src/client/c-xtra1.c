/* $Id$ */
/* Handle the printing of info to the screen */



#include "angband.h"


/* make more room for potential new stuff in char dump,
   especially WINNER status line: */
#define NEW_COMPRESSED_DUMP

/* Compress both AC lines (Base AC, To AC) into one,
   to free up a line for "Trait"? */
#define NEW_COMPRESSED_DUMP_AC

/* Don't display 'Trait' if traits aren't available */
#define HIDE_UNAVAILABLE_TRAIT

#ifndef PALANIM_SWAP
 #define ANIM_OFFSET 16 /* palette index offset for animated colours (might need to be 0 for Windows if PALANIM_SWAP is ever used) */
 #define ANIM_BASE 0 /* Counterpart to ANIM_OFFSET: The start of the basic, normal palette. Usually 0, except for Windows if PALANIM_SWAP is ever used. */
#else
 #define ANIM_OFFSET 0 /* palette index offset for animated colours (might need to be 0 for Windows if PALANIM_SWAP is ever used) */
 #define ANIM_BASE 16 /* Counterpart to ANIM_OFFSET: The start of the basic, normal palette. Usually 0, except for Windows if PALANIM_SWAP is ever used. */
#endif
/* Animate not just the extended 'outside world' colours, that are used for day/night lightning,
   but also the classic static colours that are used for everything else, allowing this to work inside a dungeon:
   Uncool thing about it is it causes all chat text, character stats, just everything to get (lightning-)flashed.
   Todo maybe: Use extended colours for the dungeon too, not just world surface.
   Note that while lightning happens on world surface and doesn't need to work inside the dungeon,
   flash happens inside the dungeon mainly, so disabling it here would render it practically completely non-existant.

   HOWEVER, there is a bug: The colour of status bar text can be set to white on initial status bar change,
   eg 'Suspended' info on shattering the mirror, if the change happens too close to sending the animation request to
   the client, but redrawing palette-animated colours will only happen within the 'game map screen', so the status
   bar won't be redrawn and hence stay at the initial colour (eg TERM_WHITE for screenflashing).

   This set_palette() redrawal either needs fixing to affect the whole UI in gfx mode too,
   or FULL-palette anims should both be commented out here. */
#define ANIM_FULL_PALETTE_FLASH
//#define ANIM_FULL_PALETTE_LIGHTNING

/* If 'solid_bars' is on and HP, MP or ST are set to 'bar' mode, really draw them in solid char instead of '#'?
   Might not be visually desirable, as the bars are exactly adjacent... */
#define HP_MP_ST_BARS_ALLOWED

/* (Sub-)Inven/Equip marker icon for 'newest item' if c_cfg.show_newest is on. */
#define MARKER_NEWEST_ITEM '-'


#ifdef USE_SOUND_2010
int animate_lightning_type = SFX_TYPE_AMBIENT;
#else /* sigh - todo: decouple lightning animation from actually using sound */
int animate_lightning_type = 0;
#endif
int animate_lightning = 0, animate_lightning_vol = 100;
short int animate_lightning_icky = 0;

int animate_screenflash = 0;
short int animate_screenflash_icky = 0;

/*
 * Print character info at given row, column in a 13 char field
 */
static void prt_field(cptr info, int row, int col) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	/* Dump 13 spaces to clear */
	c_put_str(TERM_WHITE, "             ", row, col);

	/* Dump the info itself */
	c_put_str(TERM_L_BLUE, info, row, col);

	/* restore cursor position */
	Term_gotoxy(x, y);
}


/*
 * Converts stat num into a six-char (right justified) string
 */
void cnv_stat(int val, char *out_val) {
	if (!c_cfg.linear_stats) {
		/* Above 18 */
		if (val > 18) {
			int bonus = (val - 18);

			if (bonus >= 220) sprintf(out_val, "18/%3s", "***");
			else if (bonus >= 100) sprintf(out_val, "18/%03d", bonus);
			else sprintf(out_val, " 18/%02d", bonus);
		}
		/* From 3 to 18 */
		else sprintf(out_val, "    %2d", val);
	} else {
		/* Above 18 */
		if (val > 18) {
			int bonus = (val - 18);

			if (bonus >= 220) sprintf(out_val, "    **");
			else sprintf(out_val, "  %2d.%1d", 18 + (bonus / 10), bonus % 10);
		}
		/* From 3 to 18 */
		else sprintf(out_val, "    %2d", val);
	}
}
static int reduce_stat(int val, int reduce) {
	int bonus;

	if (val <= 18) return(val - reduce);

	/* Split it up */
	bonus = (val - 18);
	val = 18;
	/* Reduce */
	if (bonus >= reduce * 10) bonus -= reduce * 10;
	else {
		if (bonus % 10) {
			bonus -= bonus % 10;
			reduce--;
		}
		reduce -= bonus / 10;
		bonus = 0;
		if (reduce) val -= reduce;
	}
	/* Combine again */
	return(val + bonus);
}

/*
 * Print character stat in given row, column
 */
void prt_stat(int stat, bool boosted) {
	char tmp[32];
	int x, y;

	if (client_mode == CLIENT_PARTY) return;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (p_ptr->stat_use[stat] < p_ptr->stat_top[stat]) {
		Term_putstr(0, ROW_STAT + stat, -1, TERM_WHITE, (char*)stat_names_reduced[stat]);
		cnv_stat(p_ptr->stat_use[stat], tmp);
		Term_putstr(COL_STAT + 6, ROW_STAT + stat, -1, TERM_YELLOW, tmp);
	} else {
		Term_putstr(0, ROW_STAT + stat, -1, TERM_WHITE, (char*)stat_names[stat]);
		cnv_stat(p_ptr->stat_use[stat], tmp);
		if (boosted)// && p_ptr->stat_use[stat] < 18 + 220)
			Term_putstr(COL_STAT + 6, ROW_STAT + stat, -1, TERM_L_BLUE, tmp);
		else if (p_ptr->stat_max[stat] < (18 + 100))
			Term_putstr(COL_STAT + 6, ROW_STAT + stat, -1, TERM_L_GREEN, tmp);
		else
			Term_putstr(COL_STAT + 6, ROW_STAT + stat, -1, TERM_L_UMBER, tmp);
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints "title", including "wizard" or "winner" as needed.
 */
void prt_title(cptr title) {
	prt_field(title, ROW_TITLE, COL_TITLE);
}


/*
 * Prints level and experience
 */
/* Display exp bar in 5% steps instead of 10%? (Uses 2 colours to disgtinguish) */
#define EXP_BAR_FINESCALE
/* Fill rest of exp bar up with dark colour instead of leaving it blank? */
#define EXP_BAR_FILLDARK

#if 1 /* draw exp bar in blue? */
 #define EXP_BAR_HI TERM_L_BLUE
 #define EXP_BAR_LO TERM_BLUE
 #define EXP_BAR_NO TERM_L_DARK
#else /* draw exp bar in green? */
 #define EXP_BAR_HI TERM_L_GREEN
 #define EXP_BAR_LO TERM_GREEN
 #define EXP_BAR_NO TERM_L_DARK
#endif
#define EXP_BAR_HI_DRAINED TERM_YELLOW
#define EXP_BAR_LO_DRAINED TERM_L_UMBER
#define EXP_BAR_NO_DRAINED TERM_YELLOW
//#define EXP_BAR_NO_DRAINED TERM_UMBER

void prt_level(int level, int max_lev, int max_plv, s32b max, s32b cur, s32b adv, s32b adv_prev) {
	char tmp[32], tmp2[32], exp_bar_char;
	int colour, x, y;
	int scale = adv - adv_prev;
#ifdef USE_GRAPHICS
	int gfx_char = FALSE, ng;
#endif

	/* (Catch bug with setlev(100) command - todo: just fix the command..) */
	if (!cur && level > 1) return;

	/* remember cursor position */
	Term_locate(&x, &y);

	Term_putstr(0, ROW_LEVEL, -1, TERM_WHITE, "LEVEL ");
	//Level 100 (admins): If we cut off LEVEL to LEVE, just cut it off 1 more to LEV to look better -_-
	if (max_plv >= 100) sprintf(tmp, "%4d", level);
	else sprintf(tmp, "%3d", level);

	colour = (level < max_lev) ? TERM_YELLOW : (level < PY_MAX_PLAYER_LEVEL ? TERM_L_GREEN : (level < PY_MAX_LEVEL ? TERM_L_UMBER : TERM_BLUE));
	if (max_lev >= max_plv) { /* In rare cases max_lev can actually exceed max_plv, if player had drained xp and meanwhile gained enough xp that the spillover into max_exp crossed the level threshold! */
		Term_putstr(COL_LEVEL + 5, ROW_LEVEL, -1, colour, "    ");
		Term_putstr(COL_LEVEL + 9 + 3 - strlen(tmp), ROW_LEVEL, -1, colour, tmp);
	} else {
		sprintf(tmp2, "(%d)", max_plv);
		Term_putstr(COL_LEVEL + 9 + 3 - strlen(tmp) - strlen(tmp2), ROW_LEVEL, -1, colour, tmp);
		Term_putstr(COL_LEVEL + 9 + 3 - strlen(tmp2), ROW_LEVEL, -1, TERM_SLATE, tmp2);
	}

	colour = (p_ptr->exp < p_ptr->max_exp) ? TERM_YELLOW : (p_ptr->exp < PY_MAX_EXP ? TERM_L_GREEN : TERM_L_UMBER);
	if (!c_cfg.exp_bar) {
		if (!c_cfg.exp_need) {
			if (c_cfg.colourize_bignum) colour_bignum(cur, PY_MAX_EXP, tmp, 2, TRUE);
			else sprintf(tmp, "%9d", (int)cur);
		} else {
			if (level >= PY_MAX_PLAYER_LEVEL || !adv)
				(void)sprintf(tmp, "      ***");
			else {
				/* Hack -- display in minus (to avoid confusion chez player) */
				if (c_cfg.colourize_bignum) colour_bignum((int)(cur - adv), PY_MAX_EXP, tmp, 1, TRUE);
				else sprintf(tmp, "%9d", (int)(cur - adv));
			}
		}
		strcat(tmp, " "); //hack to correctly clear line after player disabled 'exp_bar' option (it's 1 char 'too long')

		if (cur >= max) {
			Term_putstr(0, ROW_EXP, -1, TERM_WHITE, "XP ");
			Term_putstr(COL_EXP + 3, ROW_EXP, -1, colour, tmp);
		} else {
			Term_putstr(0, ROW_EXP, -1, TERM_WHITE, "Xp ");
			Term_putstr(COL_EXP + 3, ROW_EXP, -1, TERM_YELLOW, tmp);
		}
	} else { //c_cfg.exp_bar
#ifdef EXP_BAR_FINESCALE
		int got_org = 0;
#endif
#ifdef WINDOWS
		if (!force_cui && c_cfg.solid_bars) exp_bar_char = FONT_MAP_SOLID_WIN; /* :-p hack */
		else
#elif defined(USE_X11)
		if (!force_cui && c_cfg.solid_bars) exp_bar_char = FONT_MAP_SOLID_X11; /* :-p hack */
		else
///#else /* command-line client ("-c") doesn't draw either! */
#endif
			exp_bar_char = '#';

#ifdef USE_GRAPHICS
		/* Graphical tileset? Use extra tile for this */
		if (use_graphics && exp_bar_char != '#') exp_bar_char = Client_setup.f_char[FEAT_SOLID_NC];
#endif

		if (level >= PY_MAX_PLAYER_LEVEL || !adv || !scale) {
			if (cur < PY_MAX_EXP) {
				exp_bar_char = '*';
				adv = PY_MAX_EXP;
				scale = adv - adv_prev;

				if (cur >= max) {
					Term_putstr(0, ROW_EXP, -1, TERM_WHITE, "XP ");
					Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_HI, tmp);
				} else {
					Term_putstr(0, ROW_EXP, -1, TERM_WHITE, "Xp ");
					Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_HI_DRAINED, tmp);
				}
			} else {
				(void)sprintf(tmp, "      ***");

				Term_putstr(0, ROW_EXP, -1, TERM_WHITE, "XP ");
				Term_putstr(COL_EXP + 3, ROW_EXP, -1, TERM_L_UMBER, tmp);

				/* restore cursor position */
				Term_gotoxy(x, y);

				return;
			}
		}
#ifndef EXP_BAR_FINESCALE
		//if (!c_cfg.exp_need) {
			int got = ((cur - adv_prev) * 10) / scale, i;

			for (i = 0; i < got; i++) tmp[i] = exp_bar_char;
			tmp[i] = 0;
		//}
 #if 0
		else {
			int got = ((cur - adv_prev) * 10) / scale, i;

			for (i = 0; i < got; i++) tmp[i] = '-'; //10 'filled bubbles' = next level, so only need to display 9!
			for (i = got; i < 9; i++) tmp[i] = exp_bar_char;
			tmp[i] = 0;
		}
 #endif
#else /* finer double-scale 0..10 in 0.5 steps :D */
		//if (!c_cfg.exp_need) {
			int got = ((cur - adv_prev) * 20 + half_exp) / scale, i;

			got_org = got;

			for (i = 0; i < got / 2; i++) tmp[i] = exp_bar_char;
			tmp[i] = 0;
		//}
 #if 0
		else {
			int got = ((cur - adv_prev) * 20 + half_exp) / scale, i;

			got_org = got;

			for (i = 0; i < got / 2; i++) tmp[i] = '-'; //10 'filled bubbles' = next level, so only need to display 9!
			for (i = got / 2; i < 9; i++) tmp[i] = exp_bar_char;
			tmp[i] = 0;
		}
 #endif
#endif

#ifdef USE_GRAPHICS
		/* Graphical tileset? Use extra tile for this */
		if (use_graphics && exp_bar_char == '*' && c_cfg.solid_bars && !force_cui) gfx_char = Client_setup.f_char[FEAT_SOLID_NC + 5]; //diamond
		else if (use_graphics && exp_bar_char != '#')
			gfx_char = Client_setup.f_char[165]; //either this, or:
			//gfx_char = Client_setup.f_char[FEAT_SOLID_NC + 6]; //+0..+6, recommended: +0, +5, +6
		else gfx_char = FALSE;
#endif

#ifdef EXP_BAR_FILLDARK
		/* Paint dark base ;) */
		if (cur >= max) Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_NO, "---------");
		else Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_NO_DRAINED, "---------");
#else
		if (cur >= max) Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_NO, "         ");//just erase any previous '-' or exp bar possibly
		else Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_NO_DRAINED, "-");
#endif
		if (cur >= max) {
			Term_putstr(0, ROW_EXP, -1, TERM_WHITE, "XP ");

#ifndef EXP_BAR_FINESCALE
 #ifdef USE_GRAPHICS
			if (gfx_char) {
				for (ng = 0; ng < strlen(tmp); ng++)
					Term_draw(COL_EXP + 3 + ng, ROW_EXP, EXP_BAR_HI, gfx_char);
			} else
 #endif
			Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_HI, tmp);
#else
			/* special hack to be able to display this distinguishdly from got == 18 (we only have 9 characters available) */
 #ifdef USE_GRAPHICS
			if (gfx_char) {
				if (got_org == 19) {
					for (ng = 0; ng < strlen(tmp); ng++)
						Term_draw(COL_EXP + 3 + ng, ROW_EXP, EXP_BAR_LO, gfx_char);
				} else {
					for (ng = 0; ng < strlen(tmp); ng++)
						Term_draw(COL_EXP + 3 + ng, ROW_EXP, EXP_BAR_HI, gfx_char);
					if (got_org % 2) Term_draw(COL_EXP + 3 + got_org / 2, ROW_EXP, EXP_BAR_LO, gfx_char);
				}
			} else
 #endif
			if (got_org == 19) Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_LO, tmp);
			else {
				Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_HI, tmp);
				if (got_org % 2) Term_putch(COL_EXP + 3 + got_org / 2, ROW_EXP, EXP_BAR_LO, exp_bar_char);
			}
#endif
		} else {
			Term_putstr(0, ROW_EXP, -1, TERM_WHITE, "Xp ");
#ifndef EXP_BAR_FINESCALE
 #ifdef USE_GRAPHICS
			if (gfx_char) {
				for (ng = 0; ng < strlen(tmp); ng++)
					Term_draw(COL_EXP + 3 + ng, ROW_EXP, EXP_BAR_HI_DRAINED, gfx_char);
			} else
 #endif
			Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_HI_DRAINED, tmp);
#else
			/* special hack to be able to display this distinguishdly from got == 18 (we only have 9 characters available) */
 #ifdef USE_GRAPHICS
			if (gfx_char) {
				if (got_org == 19) {
					for (ng = 0; ng < strlen(tmp); ng++)
						Term_draw(COL_EXP + 3 + ng, ROW_EXP, EXP_BAR_LO_DRAINED, gfx_char);
				} else {
					for (ng = 0; ng < strlen(tmp); ng++)
						Term_draw(COL_EXP + 3 + ng, ROW_EXP, EXP_BAR_HI_DRAINED, gfx_char);
					if (got_org % 2) Term_draw(COL_EXP + 3 + got_org / 2, ROW_EXP, EXP_BAR_LO_DRAINED, gfx_char);
				}
			} else
 #endif
			if (got_org == 19) Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_LO_DRAINED, tmp);
			else {
				Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_HI_DRAINED, tmp);
				if (got_org % 2) Term_putch(COL_EXP + 3 + got_org / 2, ROW_EXP, EXP_BAR_LO_DRAINED, exp_bar_char);
			}
#endif
		}
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints current gold
 */
void prt_gold(int gold) {
	int x, y;

	/* Apply to AU display too */
	if (!c_cfg.colourize_bignum) {
		char tmp[10 + 1];

		/* remember cursor position */
		Term_locate(&x, &y);

		sprintf(tmp, "%10d", gold);
		if (tmp[0] != ' ') put_str("$ ", ROW_GOLD, COL_GOLD);
		else put_str("AU ", ROW_GOLD, COL_GOLD);

		c_put_str(gold != PY_MAX_GOLD ? TERM_L_GREEN : TERM_L_UMBER, tmp, ROW_GOLD, COL_GOLD + 2);
	} else {
		/* Doesn't look thaaat nice colour-wise maybe, despite being easier to interpret :/ */
		char tmp[(2 + 3) * 4 + 1];

		colour_bignum(gold, PY_MAX_GOLD, tmp, 1, TRUE);

		/* remember cursor position */
		Term_locate(&x, &y);

		/* Very large gold amount? Reduce AU to just $ to make room for 1 more digit */
		if (tmp[2] != ' ') put_str("$ ", ROW_GOLD, COL_GOLD);
		else put_str("AU ", ROW_GOLD, COL_GOLD);

		c_put_str(TERM_YELLOW, tmp, ROW_GOLD, COL_GOLD + 2); /* Make up for 10 -> 12 (and +1 for colour code, we skip the first one!) number size expansion for triplet-processing */
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints current AC
 */
void prt_ac(int ac, bool boosted, bool unknown) {
	char tmp[32];
	int x, y, attr;

	if (client_mode == CLIENT_PARTY) return;

	/* remember cursor position */
	Term_locate(&x, &y);

	put_str("AC:", ROW_AC, COL_AC);
	sprintf(tmp, "%5d", ac);

	//if (ac > 0 && boosted) attr = TERM_L_BLUE;
	if (boosted) attr = TERM_L_BLUE;
	else if (unknown) attr = TERM_GREEN;
	else attr = TERM_L_GREEN;

	c_put_str(attr, tmp, ROW_AC, COL_AC + 7);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints Max/Cur hit points
 */
#define HP_MP_ST_BAR_HALFSTEPS
void prt_hp(int max, int cur, bool bar, bool boosted) {
	if (shopping) return;
	char tmp[32];
	byte color;
	int x, y; /* for remembering cursor pos */

	hp_max = max;
	hp_cur = cur;
	hp_bar = bar;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (client_mode == CLIENT_PARTY) {
		color = TERM_L_RED;
		sprintf(tmp, "HP: %4d ", max);
		c_put_str(color, tmp, CLIENT_PARTY_ROWHP, CLIENT_PARTY_COLHP);

		sprintf(tmp, "%4d", cur);

		if (cur >= max)
			color = TERM_L_RED;
		else if (cur > max / 10)
			color = TERM_YELLOW;
		else
			color = TERM_RED;
		c_put_str(color, tmp, CLIENT_PARTY_ROWHP, CLIENT_PARTY_COLHP + 9);
	}
	/* DEG Default to else since only 2 types for now */
	else {
#ifndef CONDENSED_HP_MP
		put_str("Max HP ", ROW_MAXHP, COL_MAXHP);
		sprintf(tmp, "%5d", max);
		color = boosted ? TERM_L_BLUE : TERM_L_GREEN;
		c_put_str(color, tmp, ROW_MAXHP, COL_MAXHP + 7);

		put_str("Cur HP ", ROW_CURHP, COL_CURHP);

		sprintf(tmp, "%5d", cur);

		if (cur >= max)
			color = TERM_L_GREEN;
		else if (cur > max / 10)
			color = TERM_YELLOW;
		else
			color = TERM_RED;

		c_put_str(color, tmp, ROW_CURHP, COL_CURHP + 7);
#else
		if (!bar) {
			put_str("HP:    /", ROW_MAXHP, 0);
			if (max == -9999) sprintf(tmp, "   -"); /* wonder whether this will ever be used */
			else sprintf(tmp, "%4d", max);
			color = boosted ? TERM_L_BLUE : TERM_L_GREEN;
			c_put_str(color, tmp, ROW_MAXHP, COL_MAXHP);
			if (cur == -9999) sprintf(tmp, "   -"); /* wonder whether this will ever be used */
			else sprintf(tmp, "%4d", cur);
			if (cur >= max) color = TERM_L_GREEN;
			else if (cur > (max * 2) / 5) color = TERM_YELLOW;
			else if (cur > max / 6) color = TERM_ORANGE;
			else color = TERM_RED;
			c_put_str(color, tmp, ROW_CURHP, COL_CURHP);
		} else {
			color = TERM_L_GREEN;
			/* Specialty: In 'bar' view we have to colour the 'HP' itself or we won't notice the buff */
			if (boosted) c_put_str(TERM_L_BLUE, "HP          ", ROW_MAXHP, 0);
			else put_str("HP          ", ROW_MAXHP, 0);
			if (max == -9999) {
				sprintf(tmp, "   -/   -"); /* we just assume cur would also be -9999.. */
				c_put_str(color, tmp, ROW_CURHP, COL_CURHP);
			} else {
				int c, n = (9 * cur) / max;
 #ifdef HP_MP_ST_BAR_HALFSTEPS
				int half = ((2 * 9 * cur) / max);
 #endif
				char bar_char;

 #ifdef HP_MP_ST_BARS_ALLOWED /* looks too strange with all 3 bars above each other */
  #ifdef WINDOWS
				if (!force_cui && c_cfg.solid_bars) bar_char = FONT_MAP_SOLID_WIN; /* :-p hack */
				else
  #elif defined(USE_X11)
				if (!force_cui && c_cfg.solid_bars) bar_char = FONT_MAP_SOLID_X11; /* :-p hack */
				else
  //#else /* command-line client ("-c") doesn't draw either! */
  #endif
 #endif
					bar_char = '#';

 #ifndef HP_MP_ST_BAR_HALFSTEPS
				if (cur >= max) ;
				else if (cur > (max * 2) / 5) color = TERM_YELLOW;
				else if (cur > max / 6) color = TERM_ORANGE;
				else color = TERM_L_RED;
 #else
				if (cur >= max) ;
				else if (half >= 8) color = TERM_YELLOW;
				else if (half >= 4) color = TERM_ORANGE;
				else color = TERM_L_RED;
 #endif
				tmp[0] = 0;
				for (c = 0; c < n; c++) tmp[c] = bar_char;
				tmp[c] = 0;
				if (cur <= 0) strcpy(tmp, "-");
				else if (!tmp[0]) {
					tmp[0] = bar_char; //always display at least one tick (still we probably don't just want to round up instead)
					tmp[1] = 0;
				}

 #ifdef USE_GRAPHICS
				if (use_graphics) {
					int w;

					for (w = COL_CURHP; w - COL_CURHP < strlen(tmp); w++)
						Term_draw(w, ROW_CURHP, color, w == COL_CURHP ? 807 : (w - COL_CURHP == strlen(tmp) - 1
  #ifdef HP_MP_ST_BAR_HALFSTEPS
						    /* Do we show a half step? */
						    && !((half % 2) && n > 0)
  #endif
						    ? 808 : 809));
				} else
 #endif
				c_put_str(color, tmp, ROW_CURHP, COL_CURHP);

 #ifdef HP_MP_ST_BAR_HALFSTEPS
				/* Do we show a half step? */
				if ((half % 2) && n > 0) {
					switch (color) {
					case TERM_L_GREEN: color = TERM_GREEN; break;
					case TERM_YELLOW: color = TERM_L_UMBER; break;
					case TERM_ORANGE: color = TERM_UMBER; break;
					case TERM_L_RED: color = TERM_RED; break;
					}
  #ifdef USE_GRAPHICS
					if (use_graphics) Term_draw(COL_CURHP + n, ROW_CURHP, color, 808);//813
					else
  #endif
					c_put_str(color, format("%c", bar_char), ROW_CURHP, COL_CURHP + n);
				}
 #endif
			}
		}
#endif

		/* restore cursor position */
		Term_gotoxy(x, y);
	}
}
void prt_stamina(int max, int cur, bool bar) {
	if (shopping) return;
	char tmp[32];
	byte color;
	int x, y; /* for remembering cursor pos */

	st_max = max;
	st_cur = cur;
	st_bar = bar;

	/* remember cursor position */
	Term_locate(&x, &y);

#ifdef CONDENSED_HP_MP
	if (!bar) {
		put_str("ST:    /", ROW_MAXST, 0);
		if (max == -9999) sprintf(tmp, "   -");
		else sprintf(tmp, "%4d", max);
		color = TERM_L_GREEN;
		c_put_str(color, tmp, ROW_MAXST, COL_MAXST);
		if (cur == -9999) sprintf(tmp, "   -");
		else sprintf(tmp, "%4d", cur);
		if (cur >= max)	color = TERM_L_GREEN;
		else if (cur > (max * 2) / 5) color = TERM_YELLOW;
		else if (cur > max / 6) color = TERM_ORANGE;
		else color = TERM_RED;
		c_put_str(color, tmp, ROW_CURST, COL_CURST);
	} else {
		char bar_char;

 #ifdef HP_MP_ST_BARS_ALLOWED /* looks too strange with all 3 bars above each other */
  #ifdef WINDOWS
		if (!force_cui && c_cfg.solid_bars) bar_char = FONT_MAP_SOLID_WIN; /* :-p hack */
		else
  #elif defined(USE_X11)
		if (!force_cui && c_cfg.solid_bars) bar_char = FONT_MAP_SOLID_X11; /* :-p hack */
		else
  //#else /* command-line client ("-c") doesn't draw either! */
  #endif
 #endif
			bar_char = '#';

		color = TERM_L_GREEN;
		put_str("ST          ", ROW_MAXST, 0);
		if (max == -9999) {
			sprintf(tmp, "   -/   -"); /* we just assume cur would also be -9999.. */
			c_put_str(color, tmp, ROW_CURST, COL_CURST);
		} else {
			int c, n = (9 * cur) / max;
 #if 0 /* since stamina only goes from 0 to 10, this doesn't make sense and looks bad */
  #ifdef HP_MP_ST_BAR_HALFSTEPS
			int half = (2 * 9 * cur) / max;
  #endif
 #endif

			if (cur >= max) color = TERM_L_WHITE;//TERM_L_GREEN;
			else if (cur > (max * 2) / 5) color = TERM_SLATE;//TERM_YELLOW;
			else if (cur > max / 6) color = TERM_L_DARK;//TERM_ORANGE;
			else color = TERM_L_DARK;//TERM_RED;

			tmp[0] = 0;
			for (c = 0; c < n; c++) tmp[c] = bar_char;
			tmp[c] = 0;
			if (cur <= 0) strcpy(tmp, "-");
			else if (!tmp[0]) {
				tmp[0] = bar_char; //always display at least one tick (still we probably don't just want to round up instead)
				tmp[1] = 0;
			}

 #ifdef USE_GRAPHICS
			if (use_graphics) {
				int w;

				for (w = COL_CURST; w - COL_CURST < strlen(tmp); w++)
					Term_draw(w, ROW_CURST, color, w == COL_CURST ? 807 : (w - COL_CURST == strlen(tmp) - 1
  #if 0
   #ifdef HP_MP_ST_BAR_HALFSTEPS
					    /* Do we show a half step? */
					    && !((half % 2) && n > 0)
   #endif
  #endif
					    ? 808 : 809));
			} else
 #endif
			c_put_str(color, tmp, ROW_CURST, COL_CURST);
 #if 0 /* since stamina only goes from 0 to 10, this doesn't make sense and looks bad */
  #ifdef HP_MP_ST_BAR_HALFSTEPS
			/* Do we show a half step? */
			if ((half % 2) && n > 0) {
				switch (color) {
				case TERM_L_WHITE: color = TERM_SLATE; break;
				case TERM_SLATE: color = TERM_L_DARK; break;
				case TERM_L_DARK: color = TERM_DARK; break; //hack: don't print half step
				}
				if (color != TERM_DARK)
   #ifdef USE_GRAPHICS
					if (use_graphics) Term_draw(COL_CURST + n, ROW_CURST, color, 808);//813
					else
   #endif
					c_put_str(color, format("%c", bar_char), ROW_CURST, COL_CURST + n);
			}
  #endif
 #endif
		}
	}
#endif

	/* restore cursor position */
	Term_gotoxy(x, y);
}
/* DEG print party members hps to screen */
void prt_party_stats(int member_num, byte color, char *member_name, int member_lev, int member_chp, int member_mhp, int member_cmp, int member_mmp) {
	char tmp[32];
	int rowspacing = 0;

	if (member_num != 0) rowspacing += (4 * member_num);

	if (client_mode != CLIENT_PARTY) return;

	if (member_name[0] == '\0' && member_lev == 0) {
		/* Empty member? Just clear it - mikaelh */
		int i;

		for (i = CLIENT_PARTY_ROWMBR + rowspacing; i < CLIENT_PARTY_ROWMBR + rowspacing + 3; i++)
			Term_erase(0, i, 12);
		return;
	}

	sprintf(tmp, "%s", member_name);
	c_put_str(color, tmp, (CLIENT_PARTY_ROWMBR + rowspacing) ,CLIENT_PARTY_COLMBR);

	sprintf(tmp, "HP: %4d ", member_mhp);
	color = TERM_L_RED;
	c_put_str(color, tmp, (CLIENT_PARTY_ROWMBR + rowspacing + 1) ,CLIENT_PARTY_COLMBR);

	sprintf(tmp, "%4d", member_chp);

	if (member_chp >= member_mhp)
		color = TERM_L_RED;
	else if (member_chp > member_mhp / 10)
		color = TERM_YELLOW;
	else
		color = TERM_RED;

	c_put_str(color, tmp, (CLIENT_PARTY_ROWMBR + rowspacing + 1) ,CLIENT_PARTY_COLMBR + 9);


	sprintf(tmp, "SP: %4d ", member_mmp);
	color = TERM_L_BLUE;
	c_put_str(color, tmp, (CLIENT_PARTY_ROWMBR + rowspacing + 2) , CLIENT_PARTY_COLMBR + 0);

	sprintf(tmp, "%4d", member_cmp);

	if (member_cmp >= member_mmp)
		color = TERM_L_BLUE;
	else if (member_cmp > member_mmp / 10)
		color = TERM_YELLOW;
	else
		color = TERM_RED;

	c_put_str(color, tmp, (CLIENT_PARTY_ROWMBR + rowspacing + 2) , CLIENT_PARTY_COLMBR + 9);
}


/*
 * Prints Max/Cur mana points
 */
void prt_mp(int max, int cur, bool bar) {
	if (shopping) return;
	char tmp[32];
	byte color;
	int x, y; /* for remembering cursor pos */

	mp_max = max;
	mp_cur = cur;
	mp_bar = bar;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (client_mode == CLIENT_PARTY) {
		sprintf(tmp, "MP: %4d ", max);
		color = TERM_L_BLUE;
		c_put_str(color, tmp, CLIENT_PARTY_ROWMP, CLIENT_PARTY_COLMP);

		sprintf(tmp, "%4d", cur);
		color = TERM_L_GREEN;

		if (cur >= max)
			color = TERM_L_BLUE;
		else if (cur > max / 10)
			color = TERM_YELLOW;
		else
			color = TERM_RED;

		c_put_str(color, tmp, CLIENT_PARTY_ROWMP, CLIENT_PARTY_COLMP + 9);
	} else {
#ifndef CONDENSED_HP_MP
		put_str("Max MP ", ROW_MAXMP, COL_MAXMP);

		sprintf(tmp, "%5d", max);
		color = TERM_L_GREEN;

		c_put_str(color, tmp, ROW_MAXMP, COL_MAXMP + 7);


		put_str("Cur MP ", ROW_CURMP, COL_CURMP);

		sprintf(tmp, "%5d", cur);

		if (cur >= max)
			color = TERM_L_GREEN;
		else if (cur > max / 10)
			color = TERM_YELLOW;
		else
			color = TERM_RED;

		c_put_str(color, tmp, ROW_CURMP, COL_CURMP + 7);
#else
		if (!bar) {
			put_str("MP:    /", ROW_MAXMP, 0);
			if (max == -9999) sprintf(tmp, "   -");
			else sprintf(tmp, "%4d", max);
			color = TERM_L_GREEN;
			c_put_str(color, tmp, ROW_MAXMP, COL_MAXMP);
			if (cur == -9999) sprintf(tmp, "   -");
			else sprintf(tmp, "%4d", cur);
			if (cur >= max) color = TERM_L_GREEN;
			else if (cur > (max * 2) / 5) color = TERM_YELLOW;
			else if (cur > max / 6) color = TERM_ORANGE;
			else color = TERM_RED;
			c_put_str(color, tmp, ROW_CURMP, COL_CURMP);
		} else {
			char bar_char;

 #ifdef HP_MP_ST_BARS_ALLOWED /* looks too strange with all 3 bars above each other */
  #ifdef WINDOWS
			if (!force_cui && c_cfg.solid_bars) bar_char = FONT_MAP_SOLID_WIN; /* :-p hack */
			else
  #elif defined(USE_X11)
			if (!force_cui && c_cfg.solid_bars) bar_char = FONT_MAP_SOLID_X11; /* :-p hack */
			else
  //#else /* command-line client ("-c") doesn't draw either! */
  #endif
 #endif
				bar_char = '#';
 #if 0 /* use same colours as for HP/SN bars? */
			color = TERM_L_GREEN;
			put_str("MP          ", ROW_MAXMP, 0);
			if (max == -9999) sprintf(tmp, "   -/   -"); /* we just assume cur would also be -9999.. */
			else {
				int c, n = (9 * cur) / max;

				if (cur >= max) ;
				else if (cur > (max * 2) / 5) color = TERM_YELLOW;
				else if (cur > max / 6) color = TERM_ORANGE;
				else color = TERM_RED;

				tmp[0] = 0;
				for (c = 0; c < n; c++) tmp[c] = bar_char;
				tmp[c] = 0;
				if (cur <= 0) strcpy(tmp, "-");
				else if (!tmp[0]) {
					tmp[0] = bar_char; //always display at least one tick (still we probably don't just want to round up instead)
					tmp[1] = 0;
				}
			}
 #else /* use blue theme for mana instead? */
			color = TERM_L_BLUE;
			put_str("MP          ", ROW_MAXMP, 0);
			if (max == -9999) {
				sprintf(tmp, "   -/   -"); /* we just assume cur would also be -9999.. */
				c_put_str(color, tmp, ROW_CURMP, COL_CURMP);
			} else {
				int c, n = (9 * cur) / max;
  #ifdef HP_MP_ST_BAR_HALFSTEPS
				int half = (2 * 9 * cur) / max;
  #endif

  #ifndef HP_MP_ST_BAR_HALFSTEPS
				if (cur >= max) ;
				else if (cur >= (max * 4) / 9) color = TERM_L_BLUE; //same!
				else if (cur >= (max * 2) / 9) color = TERM_BLUE;
				else color = TERM_VIOLET;
  #else
				if (cur >= max) ;
				else if (half >= 8) color = TERM_L_BLUE; //same!
				else if (half >= 4) color = TERM_BLUE;
				else color = TERM_VIOLET;
  #endif
				tmp[0] = 0;
				for (c = 0; c < n; c++) tmp[c] = bar_char;
				tmp[c] = 0;
				if (cur <= 0) strcpy(tmp, "-");
				else if (!tmp[0]) {
					tmp[0] = bar_char; //always display at least one tick (still we probably don't just want to round up instead)
					tmp[1] = 0;
				}

  #ifdef USE_GRAPHICS
				if (use_graphics) {
					int w;

					for (w = COL_CURMP; w - COL_CURMP < strlen(tmp); w++)
						Term_draw(w, ROW_CURMP, color, w == COL_CURMP ? 807 : (w - COL_CURMP == strlen(tmp) - 1
   #ifdef HP_MP_ST_BAR_HALFSTEPS
						    /* Do we show a half step? */
						    && !((half % 2) && n > 0)
   #endif
						    ? 808 : 809));
				} else
  #endif
				c_put_str(color, tmp, ROW_CURMP, COL_CURMP);

  #ifdef HP_MP_ST_BAR_HALFSTEPS
				/* Do we show a half step? */
				if ((half % 2) && n > 0) {
					switch (color) {
					case TERM_L_BLUE: color = TERM_BLUE; break;
					case TERM_BLUE: color = TERM_L_DARK; break;
					case TERM_VIOLET: color = TERM_L_DARK; break;
					}
   #ifdef USE_GRAPHICS
					if (use_graphics) Term_draw(COL_CURMP + n, ROW_CURMP, color, 808);//813
					else
   #endif
					c_put_str(color, format("%c", bar_char), ROW_CURMP, COL_CURMP + n);
				}
  #endif
			}
 #endif
		}
#endif
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints the player's current sanity.
 */
void prt_sane(byte attr, cptr buf) {
	int x, y;
	char tmpsn[12];

	if (client_mode == CLIENT_PARTY) return;

#ifdef SHOW_SANITY	/* NO SANITY DISPLAY!!! */
	strcpy(tmpsn, buf);
 #if defined(WINDOWS) || defined(USE_X11)
	if (c_cfg.solid_bars)
		for (x = 0; x < strlen(tmpsn); x++)
  #ifdef WINDOWS
			if (tmpsn[x] == '*') tmpsn[x] = FONT_MAP_SOLID_WIN;
  #elif defined(USE_X11)
			if (tmpsn[x] == '*') tmpsn[x] = FONT_MAP_SOLID_X11;
  #endif
 #endif

	/* remember cursor position */
	Term_locate(&x, &y);

	put_str("SN:         ", ROW_SANITY, COL_SANITY);
	c_put_str(attr, tmpsn, ROW_SANITY, COL_SANITY + 3);

	/* restore cursor position */
	Term_gotoxy(x, y);
#endif	/* SHOW_SANITY */
}

/*
 * Prints depth into the dungeon
 */
void prt_depth(int x, int y, int z, bool town, int colour, int colour_sector, cptr name) {
	char depths[32];
	int x2, y2;

	/* Remember everything else */
	depth_town = town;
	depth_colour = colour;
	depth_colour_sector = colour_sector;
	/* if-check: may be identical pointer (!) */
	if (depth_name != name) strcpy(depth_name, name);

	/* remember cursor position */
	Term_locate(&x2, &y2);

	if (x == 127) sprintf(depths, "(?\?,?\?)");
	else sprintf(depths, "(%2d,%2d)", x, y);
	c_put_str(colour_sector, depths, ROW_XYPOS, COL_XYPOS);

	if (town)
		strcpy(depths, name);
	else if (c_cfg.depth_in_feet)
		sprintf(depths, "%dft", z * 50);
	else
		sprintf(depths, "Lev %d", z);

	/* Right-Adjust the "depth" and clear old values */
	c_prt(colour, format("%7s", depths), ROW_DEPTH, COL_DEPTH);

	/* New feeling-on-next-floor indicator after 4.5.6: */
	/* '+8' because depth string was right-aligned for width 7 in above format() */
	if (name[0] == '*') c_prt(TERM_L_BLUE, "*", ROW_DEPTH, COL_DEPTH + 8);

	/* restore cursor position */
	Term_gotoxy(x2, y2);

	/* hack: extend warning on speed colour too */
	if (colour_sector == TERM_L_DARK) no_tele_grid = TRUE;
	else no_tele_grid = FALSE;
	prt_speed(p_speed);
}

/*
 * Prints hunger information
 */
void prt_hunger(int food) {
	int x, y;
	static int warn_once = 3, food_prev;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (warn_once) {
		if (!c_cfg.newbie_hints || p_ptr->max_plv > 15) warn_once = 0;
		else if (food != -1) {
			food_prev = food;
			if ((warn_once >= 3 && food < PY_FOOD_ALERT) ||
			    (warn_once >= 2 && food < PY_FOOD_WEAK) ||
			    (warn_once >= 1 && food < PY_FOOD_FAINT)) {
				warn_once--;
				food_warn_once_timer = 50; /* deciseconds to glow (TERM_SELECTOR colour et al), before switching to normal, static colour */
			}
		} else food = food_prev; /* Just redraw, for switching from blinking to static colour for warn_once. */
	} else if (food == -1) return; /* Catch timing issue, if we feed from starved state quickly, before the blinking ends, resulting in visual glitch of redrawing us as blinking-starved mistakenly. */

	if (food < PY_FOOD_FAINT)
		c_put_str(food_warn_once_timer || c_cfg.flash_starvation ? TERM_SEL_RED : TERM_L_RED, "Starved", ROW_HUNGRY, COL_HUNGRY);
	else if (food < PY_FOOD_WEAK)
		c_put_str(food_warn_once_timer || c_cfg.flash_starvation ? TERM_SELECTOR : TERM_ORANGE, "Starved", ROW_HUNGRY, COL_HUNGRY);
	else if (food < PY_FOOD_ALERT)
		c_put_str(food_warn_once_timer ? TERM_SELECTOR : TERM_YELLOW, "Hungry ", ROW_HUNGRY, COL_HUNGRY);
	else if (food < PY_FOOD_FULL)
		c_put_str(TERM_L_GREEN, "       ", ROW_HUNGRY, COL_HUNGRY);
	else if (food < PY_FOOD_MAX)
		c_put_str(TERM_L_GREEN, "Full   ", ROW_HUNGRY, COL_HUNGRY);
	else
		c_put_str(TERM_GREEN, "Gorged ", ROW_HUNGRY, COL_HUNGRY);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints blindness status
 */
void prt_blind_hallu(char blind_hallu) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (blind_hallu && (blind_hallu != 0x2))
		c_put_str(TERM_ORANGE, "Blind", ROW_BLIND, COL_BLIND);
	else if ((blind_hallu & 0x2))
		c_put_str(TERM_YELLOW, "Hallu", ROW_BLIND, COL_BLIND);
	else
		put_str("     ", ROW_BLIND, COL_BLIND);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints confused status
 */
void prt_confused(bool confused) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (confused)
		c_put_str(TERM_ORANGE, "Confused", ROW_CONFUSED, COL_CONFUSED);
	else
		put_str("        ", ROW_CONFUSED, COL_CONFUSED);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints fear status
 */
void prt_afraid(bool fear) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (fear)
		c_put_str(TERM_ORANGE, "Afraid", ROW_AFRAID, COL_AFRAID);
	else
		put_str("      ", ROW_AFRAID, COL_AFRAID);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints poisoned status
 */
void prt_poisoned(char poisoned) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (poisoned & 0x2)
		c_put_str(TERM_ORANGE, "Diseased", ROW_POISONED, COL_POISONED);
	else if (poisoned)
		c_put_str((poisoned & 0x4) ? TERM_YELLOW : TERM_ORANGE, "Poisoned", ROW_POISONED, COL_POISONED);
	else
		put_str("        ", ROW_POISONED, COL_POISONED);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints paralyzed/searching status
 */
void prt_state(s16b paralyzed, bool searching, bool resting) {
	byte attr = TERM_RED;
	char text[16];
	int x, y;

	switch (paralyzed) {
	case 0:
		attr = TERM_WHITE;
		if (searching) strcpy(text, "Searching   ");
		else if (resting) strcpy(text, "Resting     ");
		else strcpy(text, "            ");
		break;
	case 2:
		strcpy(text, "In Stasis!  ");
		break;
	case 3:
		strcpy(text, "Suspended!  ");
		break;
	case 1:
	default:
		strcpy(text, "Paralyzed!  ");
		break;
	}

	/* remember cursor position */
	Term_locate(&x, &y);

	c_put_str(attr, text, ROW_STATE, COL_STATE);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints speed
 */
void prt_speed(int speed) {
	int attr = TERM_WHITE;
	char buf[32] = "";
	int x, y;
	bool boosted = FALSE;

	/* hack: remember speed (for extra no-tele warning) */
	p_speed = speed;

	/* Hack: Marker for sped-up buff - negative numbers due to bit representation would trigger the 'boosted' marker */
	if (speed >= 0 && (speed & 0x100)) {
		speed &= ~0x100;
		boosted = TRUE;
	}

	if (speed > 0) {
		if (boosted) attr = TERM_L_BLUE; //attr = TERM_VIOLET;
		else attr = TERM_L_GREEN;
		sprintf(buf, "Fast +%d", speed);
	} else if (speed < 0) {
		attr = TERM_L_UMBER;
		sprintf(buf, "Slow %d", speed);
	} else if (boosted) { /* Indicate 'normal' speed only in case we're boosted to it (ie would be slow otherwise)! */
		attr = TERM_L_BLUE; //attr = TERM_VIOLET;
		sprintf(buf, "Normal");
	}

	if (no_tele_grid) {
		attr = TERM_L_DARK;
		if (!speed && !boosted) sprintf(buf, "No-Tele");
	}

	/* remember cursor position */
	Term_locate(&x, &y);

	/* Display the speed */
	c_put_str(attr, format("%-9s", buf), ROW_SPEED, COL_SPEED);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints ability to gain skillss
 * Note: Replacing this in 4.4.8.5 by a Blows/Round display, see function below - C. Blue
 */
void prt_study(bool study) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (study) put_str("Skill", ROW_STUDY, COL_STUDY);
	else put_str("     ", ROW_STUDY, COL_STUDY);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/* Prints blows/round in main window - important and changable stat! - C. Blue */
void prt_bpr_wraith_prob(byte bpr, byte attr, cptr bpr_str) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	/* hack: display active wraithform indicator instead */
	if (bpr_str[0]) c_put_str(attr, bpr_str, ROW_BPR, COL_BPR);
#ifdef IND_WRAITH_PROB
	else if (bpr == 254) c_put_str(attr, "PbTrav", ROW_BPR, COL_BPR);
#endif
	else if (bpr == 255) c_put_str(attr, "Wraith", ROW_BPR, COL_BPR);
	else c_put_str(attr, format("%2d BpR", bpr), ROW_BPR, COL_BPR);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

void prt_indicators(u32b indicators) {
	prt_indicator_res_fire((indicators & IND_RES_FIRE) != 0);
	prt_indicator_res_cold((indicators & IND_RES_COLD) != 0);
	prt_indicator_res_elec((indicators & IND_RES_ELEC) != 0);
	prt_indicator_res_acid((indicators & IND_RES_ACID) != 0);
	prt_indicator_res_pois((indicators & IND_RES_POIS) != 0);
	prt_indicator_res_divine((indicators & IND_RES_DIVINE) != 0);
	prt_indicator_esp((indicators & IND_ESP) != 0);
	prt_indicator_melee_brand((indicators & IND_MELEE_BRAND) != 0);
	prt_indicator_regen((indicators & IND_REGEN) != 0);
	prt_indicator_dispersion((indicators & IND_DISPERSION) != 0);
	prt_indicator_charm_tstorm((indicators & IND_CHARM) != 0, (indicators & IND_TSTORM) != 0);
	prt_indicator_pfe_crit((indicators & IND_PFE) != 0, (indicators & IND_CRIT) != 0);
	if ((indicators & (IND_SHIELD1 | IND_SHIELD2 | IND_SHIELD3 | IND_SHIELD4 | IND_SHIELD5 | IND_SHIELD6 | IND_SHIELD7)) != 0) prt_indicator_shield(indicators);
	else prt_indicator_shield(0);
#ifndef IND_WRAITH_PROB
	prt_indicator_probtravel((indicators & IND_PROBTRAVEL) != 0);
#endif
}

void prt_indicator_res_fire(bool is_active) {
	int x, y;

	/* Only visible in BIG_MAP mode, othewise it would overwrite other indicators */
	if (screen_hgt != MAX_SCREEN_HGT) return;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (is_active) c_put_str(TERM_RED, "F", ROW_RESIST_FIRE, COL_RESIST_FIRE);
	else c_put_str(TERM_RED, " ", ROW_RESIST_FIRE, COL_RESIST_FIRE);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

void prt_indicator_res_cold(bool is_active) {
	int x, y;

	/* Only visible in BIG_MAP mode, othewise it would overwrite other indicators */
	if (screen_hgt != MAX_SCREEN_HGT) return;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (is_active) c_put_str(TERM_WHITE, "C", ROW_RESIST_COLD, COL_RESIST_COLD);
	else c_put_str(TERM_WHITE, " ", ROW_RESIST_COLD, COL_RESIST_COLD);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

void prt_indicator_res_elec(bool is_active) {
	int x, y;

	/* Only visible in BIG_MAP mode, othewise it would overwrite other indicators */
	if (screen_hgt != MAX_SCREEN_HGT) return;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (is_active) c_put_str(TERM_BLUE, "E", ROW_RESIST_ELEC, COL_RESIST_ELEC);
	else c_put_str(TERM_BLUE, " ", ROW_RESIST_ELEC, COL_RESIST_ELEC);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

void prt_indicator_res_acid(bool is_active) {
	int x, y;

	/* Only visible in BIG_MAP mode, othewise it would overwrite other indicators */
	if (screen_hgt != MAX_SCREEN_HGT) return;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (is_active) c_put_str(TERM_SLATE, "A", ROW_RESIST_ACID, COL_RESIST_ACID);
	else c_put_str(TERM_SLATE, " ", ROW_RESIST_ACID, COL_RESIST_ACID);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

void prt_indicator_res_pois(bool is_active) {
	int x, y;

	/* Only visible in BIG_MAP mode, othewise it would overwrite other indicators */
	if (screen_hgt != MAX_SCREEN_HGT) return;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (is_active) c_put_str(TERM_GREEN, "P", ROW_RESIST_POIS, COL_RESIST_POIS);
	else c_put_str(TERM_GREEN, " ", ROW_RESIST_POIS, COL_RESIST_POIS);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

void prt_indicator_res_divine(bool is_active) {
	int x, y;

	/* Only visible in BIG_MAP mode, othewise it would overwrite other indicators */
	if (screen_hgt != MAX_SCREEN_HGT) return;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (is_active) c_put_str(TERM_VIOLET, "M", ROW_RESIST_MANA, COL_RESIST_MANA);
	else c_put_str(TERM_VIOLET, " ", ROW_RESIST_MANA, COL_RESIST_MANA);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

void prt_indicator_esp(bool is_active) {
	int x, y;

	/* Only visible in BIG_MAP mode, othewise it would overwrite other indicators */
	if (screen_hgt != MAX_SCREEN_HGT) return;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (is_active) c_put_str(TERM_WHITE, "ESP", ROW_TEMP_ESP, COL_TEMP_ESP);
	else c_put_str(TERM_WHITE, "   ", ROW_TEMP_ESP, COL_TEMP_ESP);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

void prt_indicator_melee_brand(bool is_active) {
	int x, y;

	/* Only visible in BIG_MAP mode, othewise it would overwrite other indicators */
	if (screen_hgt != MAX_SCREEN_HGT) return;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (is_active) c_put_str(TERM_YELLOW, "MB", ROW_TEMP_TBRAND, COL_TEMP_TBRAND);
	else c_put_str(TERM_WHITE, "  ", ROW_TEMP_TBRAND, COL_TEMP_TBRAND);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

void prt_indicator_regen(bool is_active) {
	int x, y;

	/* Only visible in BIG_MAP mode, othewise it would overwrite other indicators */
	if (screen_hgt != MAX_SCREEN_HGT) return;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (is_active) c_put_str(TERM_L_GREEN, "HP+", ROW_TEMP_REGEN, COL_TEMP_REGEN);
	else c_put_str(TERM_WHITE, "   ", ROW_TEMP_REGEN, COL_TEMP_REGEN);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

void prt_indicator_dispersion(bool is_active) {
	int x, y;

	/* Only visible in BIG_MAP mode, othewise it would overwrite other indicators */
	if (screen_hgt != MAX_SCREEN_HGT) return;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (is_active) c_put_str(TERM_SLATE, "Disp", ROW_DISPERSION, COL_DISPERSION);
	else c_put_str(TERM_WHITE, "    ", ROW_DISPERSION, COL_DISPERSION);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

void prt_indicator_charm_tstorm(bool is_active_charm, bool is_active_tstorm) {
	int x, y;

	/* Only visible in BIG_MAP mode, othewise it would overwrite other indicators */
	if (screen_hgt != MAX_SCREEN_HGT) return;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (is_active_charm) c_put_str(TERM_L_BLUE, "Chm", ROW_CHARM_TSTORM, COL_CHARM_TSTORM);
	if (is_active_tstorm) c_put_str(TERM_BLUE, "Thu", ROW_CHARM_TSTORM, COL_CHARM_TSTORM);
	else c_put_str(TERM_WHITE, "   ", ROW_CHARM_TSTORM, COL_CHARM_TSTORM);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

void prt_indicator_pfe_crit(bool is_active_pfe, bool is_active_crit) {
	int x, y;

	/* Only visible in BIG_MAP mode, othewise it would overwrite other indicators */
	if (screen_hgt != MAX_SCREEN_HGT) return;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (is_active_pfe) c_put_str(TERM_GREEN, "PfE", ROW_PFE_CRIT, COL_PFE_CRIT);
	else if (is_active_crit) c_put_str(TERM_ORANGE, "Crt", ROW_PFE_CRIT, COL_PFE_CRIT);
	else c_put_str(TERM_WHITE, "   ", ROW_PFE_CRIT, COL_PFE_CRIT);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

void prt_indicator_shield(u32b flags) {
	int x, y;
	byte a;

	/* Only visible in BIG_MAP mode, othewise it would overwrite other indicators */
	if (screen_hgt != MAX_SCREEN_HGT) return;

	/* remember cursor position */
	Term_locate(&x, &y);

#ifdef IND_WRAITH_PROB
	if (!flags) c_put_str(TERM_WHITE, "    ", ROW_TEMP_SHIELD, COL_TEMP_SHIELD);
#else
	if (!flags) c_put_str(TERM_WHITE, "  ", ROW_TEMP_SHIELD, COL_TEMP_SHIELD);
#endif
	else {
		/* Colour based on shield type? */
		if (flags & IND_SHIELD1) /*p_ptr->tim_reflect*/ a = TERM_L_WHITE;
		else if (flags & IND_SHIELD2) /*p_ptr->tim_lcage*/ a = TERM_L_BLUE;
		else { // p_ptr->shield
			if (flags & IND_SHIELD3) /*SHIELD_COUNTER*/ a = TERM_UMBER;
			else if (flags & IND_SHIELD4) /*SHIELD_FIRE*/ a = TERM_RED;
			else if (flags & IND_SHIELD5) /*SHIELD_ICE*/ a = TERM_WHITE;
			else if (flags & IND_SHIELD6) /*SHIELD_PLASMA*/ a = TERM_L_RED;
			else /*IND_SHIELD7*/ a = TERM_VIOLET; //'mystic shield'
		}
#ifdef IND_WRAITH_PROB
		c_put_str(a, "Shld", ROW_TEMP_SHIELD, COL_TEMP_SHIELD);
#else
		c_put_str(a, "Sh", ROW_TEMP_SHIELD, COL_TEMP_SHIELD);
#endif
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}
void prt_indicator_probtravel(bool is_active) {
	int x, y;

	/* Only visible in BIG_MAP mode, othewise it would overwrite other indicators */
	if (screen_hgt != MAX_SCREEN_HGT) return;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (is_active) c_put_str(TERM_L_BLUE, "P", ROW_PROBTRAVEL, COL_PROBTRAVEL);
	else c_put_str(TERM_WHITE, " ", ROW_PROBTRAVEL, COL_PROBTRAVEL);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

#define PWUYF_LEN (MAX_CHARS - 1) /* accomodate for 1 leading space for indentation (see function below) */
void prt_whats_under_your_feet(char *o_name, bool crossmod_item, bool cant_see, bool on_pile) {
	char line[MSG_LEN], *lptr = line, *c, tmp[MSG_LEN];

	/* Note that so far the under-your-feet msgs used to be sent by msg_print() which would
	   split long item names up into several Send_message() calls so the client displays the
	   item as multiple lines in the message windows.
	   We emulate this split-up here now on client-side so we can immediately see the full
	   item name in our message windows: */

	if (crossmod_item) {
		if (cant_see) sprintf(line, "\377DYou feel %s%s here.", o_name, on_pile ? " on a pile" : "");
		else sprintf(line, "\377DYou see %s%s.", o_name, on_pile ? " on a pile" : "");
	} else {
		if (cant_see) sprintf(line, "You feel %s%s here.", o_name, on_pile ? " on a pile" : "");
		else sprintf(line, "You see %s%s.", o_name, on_pile ? " on a pile" : "");
	}

	/* Cut into 1-line chunks */
	while (strlen(lptr) > PWUYF_LEN) {
		strcpy(tmp, lptr);

		/* Don't split words/inscription tags if possible.. */
		c = tmp + PWUYF_LEN - 1;
		while (c > tmp + PWUYF_LEN - 10 && (
		    (isalpha(*c) && *(c + 1) >= 'a' && *(c + 1) <= 'z') ||
		    *c == '~' || *c == '+' || *c == '*' || *c == '-' || *c == '_' || *c == '^' || *c == '{' || *c == '(' || *c == '[' || *c == '!'))
			c--;
		/* If we'd have to backtrace too far, just ignore the problem and split it anyway at the original line end */
		if (c == tmp + PWUYF_LEN - 10) c = tmp + PWUYF_LEN - 1;
		*(c + 1) = 0;

		if (lptr == line) /* Indent subsequent lines with a leading space */
			c_msg_format("%s%s", crossmod_item ? "\377D" : "", tmp);
		else
			c_msg_format("%s %s%s", c_cfg.topline_first ? "\377\377" : "", crossmod_item ? "\377D" : "", tmp);

		lptr += c - tmp + 1;
	}
	/* Dump only/remaining line */
	if (lptr == line) /* Indent subsequent lines with a leading space */
		c_msg_format("%s%s", crossmod_item ? "\377D" : "", lptr);
	else
		c_msg_format("%s %s%s", c_cfg.topline_first ? "\377\377" : "", crossmod_item ? "\377D" : "", lptr);
}

/*
 * Prints cut status
 */
void prt_cut(int cut) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (cut >= CUT_MORTAL_WOUND) c_put_str(TERM_L_RED,      "Mortal wound", ROW_CUT, COL_CUT);
	else if (cut >= 200) c_put_str(TERM_RED,   "Deep gash   ", ROW_CUT, COL_CUT);
	else if (cut >= 100) c_put_str(TERM_RED,   "Severe cut  ", ROW_CUT, COL_CUT);
	else if (cut >= 50) c_put_str(TERM_ORANGE, "Nasty cut   ", ROW_CUT, COL_CUT);
	else if (cut >= 25) c_put_str(TERM_ORANGE, "Bad cut     ", ROW_CUT, COL_CUT);
	else if (cut >= 10) c_put_str(TERM_YELLOW, "Light cut   ", ROW_CUT, COL_CUT);
	else if (cut) c_put_str(TERM_YELLOW,      "Graze       ", ROW_CUT, COL_CUT);
	else put_str(				  "            ", ROW_CUT, COL_CUT);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints stun status, -1 to just redraw current stun status
 */
void prt_stun(int stun) {
	int x, y;
	static int old_stun = 0;

	if (stun == -1) stun = old_stun;
	else old_stun = stun;

	p_ptr->stun = stun; //just for use_graphics actually, to change huge bars rounded corners to square grids instead when drawing stun grid around them ^^

	/* remember cursor position */
	Term_locate(&x, &y);

	if (stun > 100) {
		c_put_str(TERM_RED, "Knocked out ", ROW_STUN, COL_STUN);
		if (c_cfg.stun_huge_bar) draw_huge_stun_bar(TERM_RED);
	} else if (stun > 50) {
		c_put_str(TERM_ORANGE, "Heavy stun  ", ROW_STUN, COL_STUN);
		if (c_cfg.stun_huge_bar) draw_huge_stun_bar(TERM_ORANGE);
	} else if (stun) {
		c_put_str(TERM_ORANGE, "Stun        ", ROW_STUN, COL_STUN);
		if (c_cfg.stun_huge_bar) draw_huge_stun_bar(TERM_YELLOW); //diverge from text colours, as 'heavy' vs 'normal' isn't visible here, so we differentiate visually instead
	} else {
		put_str("            ", ROW_STUN, COL_STUN);
		if (c_cfg.stun_huge_bar) draw_huge_stun_bar(TERM_DARK);
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints race/class info
 */
void prt_basic(void) {
	cptr r = NULL, c = NULL;

	r = p_ptr->rp_ptr->title;
	c = p_ptr->cp_ptr->title;

	prt_field((char *)r, ROW_RACE, COL_RACE);
	prt_field((char *)c, ROW_CLASS, COL_CLASS);
}

/* Print AFK status */
void prt_AFK(byte afk) {
	int x, y;

	/* Remember AFK status */
	p_ptr->afk = afk;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (afk)
		c_put_str(TERM_ORANGE, "  AFK", ROW_AFK, COL_AFK);
	else
		c_put_str(TERM_ORANGE, "     ", ROW_AFK, COL_AFK);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/* Print encumberment status line.
   Note that cumber_glove also contains cumber_helm. And heavy_swim is not feasible to do. */
void prt_encumberment(byte cumber_armor, byte awkward_armor, byte cumber_glove, byte heavy_wield, byte heavy_shield, byte heavy_shoot,
    byte icky_wield, byte awkward_wield, byte easy_wield, byte cumber_weight, byte monk_heavyarmor, byte rogue_heavyarmor, byte awkward_shoot,
    byte heavy_swim, byte heavy_tool) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	put_str("            ", ROW_CUMBER, COL_CUMBER);
	if (easy_wield) c_put_str(TERM_GREEN, "|", ROW_CUMBER, COL_CUMBER + 0);
	else if (heavy_wield) c_put_str(TERM_RED, "/", ROW_CUMBER, COL_CUMBER + 0);
	else if (awkward_wield) c_put_str(TERM_YELLOW, "/", ROW_CUMBER, COL_CUMBER + 0);
	if (icky_wield) c_put_str(TERM_ORANGE, "\\", ROW_CUMBER, COL_CUMBER + 1);
	if (heavy_tool) c_put_str(TERM_RED, "\\", ROW_CUMBER, COL_CUMBER + 2);
	if (heavy_shield) c_put_str(TERM_RED, "[", ROW_CUMBER, COL_CUMBER + 3);
	if (heavy_shoot) c_put_str(TERM_RED, "}", ROW_CUMBER, COL_CUMBER + 4);
	else if (awkward_shoot) c_put_str(TERM_YELLOW, "}", ROW_CUMBER, COL_CUMBER + 4);
	if (cumber_armor) c_put_str(TERM_UMBER, "(", ROW_CUMBER, COL_CUMBER + 5);
	if (monk_heavyarmor) c_put_str(TERM_YELLOW, "(", ROW_CUMBER, COL_CUMBER + 6);
	if (cumber_weight) c_put_str(TERM_L_RED, "F", ROW_CUMBER, COL_CUMBER + 7);
	if (rogue_heavyarmor) c_put_str(TERM_BLUE, "(", ROW_CUMBER, COL_CUMBER + 8);
	if (awkward_armor) c_put_str(TERM_VIOLET, "(", ROW_CUMBER, COL_CUMBER + 9);
	if (cumber_glove) c_put_str(TERM_VIOLET, "]", ROW_CUMBER, COL_CUMBER + 10); //already contains cumber_helm too
	if (heavy_swim) c_put_str(TERM_L_BLUE, "~", ROW_CUMBER, COL_CUMBER + 11); //unused because not feasible, replace anytime -- HOLE

	/* restore cursor position */
	Term_gotoxy(x, y);
}

void prt_extra_status(cptr status) {
	int x, y;
	static char last_status[SCREEN_PAD_LEFT + 1] = { 0 };

	/* remember cursor position */
	Term_locate(&x, &y);

	if (!status) status = last_status;
	else strncpy(last_status, status, SCREEN_PAD_LEFT);

	if (ROW_EXSTA != -1) { /* paranoia: just in case we're a client
				  without CONDENSED_HP_MP for some odd reason */
		if (!recording_macro)
			c_put_str(TERM_SLATE, format("%*s", SCREEN_PAD_LEFT, status), ROW_EXSTA, COL_EXSTA); //ensure line cleanup after 'RECORDING' indicator
		else
			/* hack: 'abuse' this line to display that we're in recording mode */
			c_put_str(TERM_L_RED, "*RECORDING*", ROW_EXSTA, COL_EXSTA);
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/* Update mini lag-o-meter,
   in a somewhat hacky way to keep track of packet loss:
   If pong doesn't arrive until next Send_ping(),
     assume packet loss.
   When next pong arrives, check previous ping's pong:
     was it missing? -> pretend packet loss _again_(*),
     otherwise display rtt for this pong normally.
   (*) reason is that otherwise lag-o-meter might just
   display a super short flicker of '++++++++++' that
   the user probably can't see.
   Note that this pseudo-packet loss displaying 10 '+'
   is same as if we had 450+ ms lag anyway, so no harm.*/
/* Enable bright red colour for actual packet loss? */
#define BRIGHTRED_PACKETLOSS
/* (Mini) Lag-o-meter bars: Use 100/3ms steps (new, makes sense for EU vs APAC server) instead of 50ms steps (old, for 1 global server location only) */
//#define STEPS33
void prt_lagometer(int lag) {
	int attr = TERM_L_GREEN;
	int num;
	int x, y;
	bool hidden;
	term *old;

	/* disable(d)? */
	if (!lagometer_enabled) return;

	/* Paranoia: Maybe just don't draw it at all while screen is icky, not even in the background? */
	//if (screen_icky) return;

	/* Prevent visual redraw glich if current term is NOT term_main, and we check for screen_icky and then Term_switch() layers of the wrong Term (ie some other term than term_main) */
	old = Term;
	Term_activate(term_term_main);

	if (lag == -1) {
		if (screen_icky) Term_switch(0);
		//Term_putstr(COL_LAG, ROW_LAG, 12, TERM_L_DARK, "[//////////]");
		Term_putstr(COL_LAG, ROW_LAG, 12, TERM_L_DARK, "[----------]");
		if (screen_icky) Term_switch(0);

		Term_activate(old);
		return;
	}

#ifdef BRIGHTRED_PACKETLOSS
	y = 0;
	for (x = 0; x < 60; x++) {
		if (ping_times[x] == -1) y++;
		else break; /* if since a packet loss we meanwhile received a packet again normally, forget about the old packet loss and assume we're fine again */
	}

	/* Latest ping might not be lost yet */
	if (ping_times[0] == -1) y--;

	/* Real packet loss colours bright red */
	if (y) {
		attr = TERM_L_RED;
		num = 10;
	} else {
#endif
#ifndef STEPS33 /* 50ms steps */
		num = lag / 50 + 1;
#else /* 33.3ms steps */
		num = (lag * 10 + 3) / 333 + 1;
#endif
		if (num > 10) num = 10;

		/* hack: we have previous packet assumed to be lost? */
		if (lag == 9999) num = 10;

#ifndef STEPS33 /* 50ms steps; colours: <100, <200, <300, <400 */
		if (num >= 9) attr = TERM_RED;
		else if (num >= 7) attr = TERM_ORANGE;
		else if (num >= 5) attr = TERM_YELLOW;
		else if (num >= 3) attr = TERM_GREEN;
#else /* 33.3ms steps; colours: <67, <133, <200, <300 */
		if (num >= 10) attr = TERM_RED;
		else if (num >= 7) attr = TERM_ORANGE;
		else if (num >= 5) attr = TERM_YELLOW;
		else if (num >= 3) attr = TERM_GREEN;
#endif
#ifdef BRIGHTRED_PACKETLOSS
	}
#endif

	if (screen_icky) Term_switch(0);

	/* remember cursor position */
	hidden = Term_locate(&x, &y);

	/* Default to "unknown" */
	Term_putstr(COL_LAG, ROW_LAG, 12, TERM_L_DARK, "[----------]");
	/* Dump the current "lag" (use '*' symbols) */
	Term_putstr(COL_LAG + 1, ROW_LAG, num, attr, "++++++++++");

	/* restore cursor position */
	Term_gotoxy(x, y);
	if (hidden) Term->scr->cu = 1;

	if (screen_icky) Term_switch(0);

	Term_activate(old);
}

/*
 * Redraw the monster health bar
 */
void health_redraw(int num, byte attr) {
	int x, y;

	/* Remember monster health */
	mon_health_num = num;
	mon_health_attr = attr;

	/* remember cursor position */
	Term_locate(&x, &y);

	/* Not tracking */
	if (!attr) {
#if 0
		/* Erase the health bar */
		Term_erase(COL_INFO, ROW_INFO, 12);
#else
		/* Draw world coordinates and AFK status from memory */
		prt_depth(p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, depth_town,
		          depth_colour, depth_colour_sector, depth_name);
		prt_AFK(p_ptr->afk);
#endif
	}

	/* Tracking a monster */
	else {
		/* Default to "unknown" */
		Term_putstr(COL_INFO, ROW_INFO, 12, TERM_WHITE, "[----------]");

		/* Dump the current "health" (use '*' symbols) */
		Term_putstr(COL_INFO + 1, ROW_INFO, num, attr, "**********");
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Choice window "shadow" of the "show_inven()" function.
 */
static void display_inven(void) {
	int	i, n, z = 0;
	long int	wgt;

	object_type *o_ptr;

	char	o_name[ONAME_LEN];
	char	tmp_val[80];
#ifdef ENABLE_SUBINVEN
	object_type *o2_ptr;
	long int subwgt;
	bool warn_W, warn_G;
	int sn;
#endif


	/* Find the "final" slot */
	for (i = 0; i < INVEN_PACK; i++) {
		o_ptr = &inventory[i];

		/* Track non-empty slots */
		if (o_ptr->tval) z = i + 1;
	}

	/* Display the inventory */
	for (i = 0; i < z; i++) {
		o_ptr = &inventory[i];

#ifdef ENABLE_SUBINVEN
		/* Only need to redraw bag items? */
		if (((p_ptr->window & (PW_INVEN_SUB | PW_INVEN)) == PW_INVEN_SUB) && o_ptr->tval != TV_SUBINVEN) continue;
#endif

		/* Start with an empty "index" */
		tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';

		/* Terminate - Term_putstr could otherwise read past the end of the buffer
		 * when it looks for a color code (valgrind complained about that). - mikaelh */
		tmp_val[3] = '\0';

		/* Prepare an "index" */
		tmp_val[0] = index_to_label(i);

		/* Bracket the "index" --(-- */
		tmp_val[1] = ')';

		/* Clear the line */
		Term_erase(0, i, 255);

		if (c_cfg.show_newest && item_newest == i) tmp_val[2] = MARKER_NEWEST_ITEM;

		/* Is this item acceptable? */
		if (item_tester_okay(o_ptr)) {
			/* Display the index */
			if ((n = check_guard_inscription_str(inventory_name[i], 'W')) && inventory[i].number < n) Term_putstr(0, i, 3, TERM_ORANGE, tmp_val);
			else if ((n = check_guard_inscription_str(inventory_name[i], 'G')) && inventory[i].number < n - 1) Term_putstr(0, i, 3, TERM_YELLOW, tmp_val);
			else Term_putstr(0, i, 3, TERM_WHITE, tmp_val);
		} else {
			/* Grey out the index */
			Term_putstr(0, i, 3, TERM_L_DARK, tmp_val);
		}

		/* Describe the object */
		strcpy(o_name, inventory_name[i]);

		/* Display the weight if needed */
		wgt = 0;
		if (c_cfg.show_weights && o_ptr->weight) {
			wgt = o_ptr->weight * o_ptr->number;
#ifdef ENABLE_SUBINVEN
			warn_W = warn_G = FALSE;
			subwgt = 0;
			/* Add weight of all items contained within */
			if (o_ptr->tval == TV_SUBINVEN) {
				for (n = 0; n < o_ptr->bpval; n++) {
					o2_ptr = &subinventory[i][n];
					if (!o2_ptr->tval) break;
					subwgt += o2_ptr->weight * o2_ptr->number;

					if (!warn_W && (sn = check_guard_inscription_str(subinventory_name[i][n], 'W')) && subinventory[i][n].number < sn) warn_W = TRUE;
					else if (!warn_G && (sn = check_guard_inscription_str(subinventory_name[i][n], 'G')) && subinventory[i][n].number < sn - 1) warn_G = TRUE;
				}
				wgt += subwgt;

				/* Add fill state to subinven bag name? */
				if (warn_W) strcat(o_name, format(" \377o[\377-%d/%d\377o]\377-", n, o_ptr->bpval));
				else if (warn_G) strcat(o_name, format(" \377y[\377-%d/%d\377y]\377-", n, o_ptr->bpval));
				else strcat(o_name, format(" [%d/%d]", n, o_ptr->bpval));
				//o_name[MSG_LEN] = 0; /* Ensure overflow protection */
			}
#endif
			if (wgt < 10000) /* still fitting into 3 digits? */
				(void)sprintf(tmp_val, "%3li.%1li lb ", wgt / 10, wgt % 10);
			else
				(void)sprintf(tmp_val, "%3lik%1li lb ", wgt / 10000, (wgt % 10000) / 1000);
		}

		/* Obtain length of description */
		n = strlen(o_name);

#if 1 /* grey out the complete slot if item isn't eligible */
		if (item_tester_okay(o_ptr)) {
			Term_putstr(3, i, n, o_ptr->attr, o_name);
		} else {
			Term_putstr(3, i, n, TERM_L_DARK, o_name);
		}
#else /* only grey out the index */
		/* Clear the line with the (possibly indented) index */
		Term_putstr(3, i, n, o_ptr->attr, o_name);
#endif
		if (wgt) {
			/* We're in the IDDC and this item is untradable to party members? */
			if (o_ptr->iron_trade)
				Term_putstr(71, i, -1, TERM_SLATE, tmp_val);
			else
				Term_putstr(71, i, -1, TERM_WHITE, tmp_val);
		}
	}

	/* Erase the rest of the window */
	for (i = z; i < Term->hgt; i++) {
		/* Erase the line */
		Term_erase(0, i, 255);
	}
}

#ifdef ENABLE_SUBINVEN
/*
 * Choice window "shadow" of the "show_subinven()" function.
 */
static void display_subinven(void) {
	int n, islot;

	char o_name[ONAME_LEN];
	char tmp_val[80];
#ifdef X_INFO_TXT_COLOURS
	char *c;
	int col_n;
#endif

	int i, k = 0, last_k = 0, z;
	long int wgt;

	object_type *o_ptr, *i_ptr = NULL;
	int subinven_size, bagheader_x, bagheader_y, k_idx;


	/* Determine, which subinventory to display:
	   A) Potion Belt first, Magic Device bag next, Trap kit bag, then Satchel, last are Chests.
	   B) Just display them in the order in inventory! (We do it this way for now.) */

	for (islot = 0; islot < INVEN_PACK; islot++) {
		if (inventory[islot].tval != TV_SUBINVEN) continue;

		i_ptr = &inventory[islot];
		subinven_size = i_ptr->bpval;

		/* Describe the subinventory type itself in one extra line*/

		/* i_ptr->k_idx is not transmitted, only tval,sval are. So we have to loop through all. */
		i = i_ptr->attr; /* base init, to slay compiler warning (and was old way actually) */
		for (k_idx = 0; k_idx < MAX_K_IDX; k_idx++)
			if (kind_list_tval[k_idx] == i_ptr->tval && kind_list_sval[k_idx] == i_ptr->sval) {
				i = color_char_to_attr(kind_list_attr[k_idx]);
				break;
			}

		bagheader_y = last_k;
		/* Add slot index label */
		sprintf(o_name, "[%c]", index_to_label(islot));
		Term_putstr(0, last_k, -1, i, o_name);
		/* Erase the rest of the line (we haven't written anything to it yet, so don't need to check o_name length or anything) */
		Term_erase(3, last_k, 255);

		/* Add bag object name */
		strcpy(o_name, inventory_name[islot]);
		//o_name[0] = toupper(o_name[0]);
		o_name[ONAME_LEN - 3] = 0; //prevent overflow
		/* Obtain length of description */
		n = strlen(o_name);
		if (n > MAX_CHARS - 5) n = MAX_CHARS - 5;
		Term_putstr(4, last_k, n, i_ptr->attr, o_name);
		/* Erase the rest of the line */
#ifdef X_INFO_TXT_COLOURS
		col_n = 0;
		c = o_name;
		while ((c = strchr(c, '\377'))) { c++; col_n += 2; } /* Take colour codes into account! */
		Term_erase(4 + n - col_n, last_k, 255);
#else
		Term_erase(4 + n, last_k, 255);
#endif
		/* account for this extra line */
		last_k++;
		bagheader_x = 4 + n;


		/* Find the "final" slot */
		z = 0;
		for (i = 0; i < subinven_size; i++) {
			o_ptr = &subinventory[islot][i];

			/* Track non-empty slots */
			if (o_ptr->tval) z = i + 1;
		}

		/* Display bag fill state */
		sprintf(tmp_val, " (%d/%d):", z, subinven_size);
		Term_putstr(bagheader_x, bagheader_y, -1, z == subinven_size ? TERM_ORANGE : i_ptr->attr, tmp_val);

		/* Display a line if inventory is actually empty */
		//if (!z) Term_putstr(0, last_k++, -1, (i_ptr->attr == TERM_L_DARK) ? TERM_L_DARK : TERM_L_WHITE,
		if (!z) Term_putstr(0, last_k++, -1, TERM_L_DARK,
		    (i_ptr->attr == TERM_L_DARK) ? /* Use colour sent by server as indicator for SUBINVEN_LIMIT_GROUP */
		    "     (This container is of duplicate type and therefore unusable)               " :
		    "     (empty)                                                                    ");
		    //"     (This container is empty)                                                  ");

		/* Display the inventory */
		k = last_k;
		for (i = 0; i < z; i++) {
			/* Track total amount of lines displayed, for all the subinventories. */
			k++;

			o_ptr = &subinventory[islot][i];

			/* Start with an empty "index" */
			tmp_val[0] = tmp_val[1] = tmp_val[2] = tmp_val[3] = tmp_val[4] = ' ';

			/* Terminate - Term_putstr could otherwise read past the end of the buffer
			 * when it looks for a color code (valgrind complained about that). - mikaelh */
			tmp_val[5] = '\0';

			/* Prepare an "index" */
			tmp_val[2] = index_to_label(i);

			/* Bracket the "index" --(-- */
			tmp_val[3] = ')';

			if (c_cfg.show_newest && item_newest == (islot + 1) * 100 + i) tmp_val[4] = MARKER_NEWEST_ITEM;

			/* Display the index */
			if ((n = check_guard_inscription_str(subinventory_name[islot][i], 'W')) && subinventory[islot][i].number < n) Term_putstr(0, last_k + i, 5, TERM_ORANGE, tmp_val);
			else if ((n = check_guard_inscription_str(subinventory_name[islot][i], 'G')) && subinventory[islot][i].number < n - 1) Term_putstr(0, last_k + i, 5, TERM_YELLOW, tmp_val);
			else Term_putstr(0, last_k + i, 5, TERM_WHITE, tmp_val);

			/* Describe the object */
			strcpy(o_name, subinventory_name[islot][i]);

			/* Obtain length of description */
			n = strlen(o_name);
			if (n > MAX_CHARS - 6) n = MAX_CHARS - 6; //cut off, can't read it anyway

			Term_putstr(5, last_k + i, n, o_ptr->attr, o_name);

			/* Erase the rest of the line */
#ifdef X_INFO_TXT_COLOURS
			col_n = 0;
			c = o_name;
			while ((c = strchr(c, '\377'))) { c++; col_n += 2; } /* Take colour codes into account! */
			Term_erase(5 + n - col_n, last_k + i, 255);
#else
			Term_erase(5 + n, last_k + i, 255);
#endif

			/* Display the weight if needed */
			if (c_cfg.show_weights && o_ptr->weight) {
				wgt = o_ptr->weight * o_ptr->number;
				if (wgt < 10000) /* still fitting into 3 digits? */
					(void)sprintf(tmp_val, "%3li.%1li lb ", wgt / 10, wgt % 10);
				else
					(void)sprintf(tmp_val, "%3lik%1li lb ", wgt / 10000, (wgt % 10000) / 1000);

				/* We're in the IDDC and this item is untradable to party members? */
				if (o_ptr->iron_trade)
					Term_putstr(71, last_k + i, -1, TERM_SLATE, tmp_val);
				else
					Term_putstr(71, last_k + i, -1, TERM_WHITE, tmp_val);
			}
		}
		last_k = k;
	}

	/* No subinventories? */
	if (!i_ptr) {
		Term_putstr(0, 0, -1, TERM_L_DARK, "(You don't carry any special bags)");
		Term_erase(34, 0, 255); /* Erase rest of the line */
		k = 1;
	}

	/* Erase the rest of the window */
	for (i = k; i < Term->hgt; i++) {
		/* Erase the line */
		Term_erase(0, i, 255);
	}

	/* hack: hide cursor */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;
}
#endif

/*
 * Choice window "shadow" of the "show_equip()" function.
 */
#define EQUIP_TEXT_COLOUR TERM_WHITE
#define EQUIP_TEXT_COLOUR2 TERM_YELLOW
static void display_equip(void) {
	byte	a;
	int	i, n;
	long int wgt;
	object_type *o_ptr;
	char	o_name[ONAME_LEN];
	char	tmp_val[80];
#ifdef X_INFO_TXT_COLOURS
	char *c;
	int col_n;
#endif

	/* Find the "final" slot */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
		o_ptr = &inventory[i];

		/* Start with an empty "index" */
		tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';

		/* Is this item acceptable? */
		if (item_tester_okay(o_ptr)) {
			/* Prepare an "index" */
			tmp_val[0] = index_to_label(i);

			/* Bracket the "index" --(-- */
			tmp_val[1] = ')';
		}

		if (c_cfg.show_newest && item_newest == i) tmp_val[2] = MARKER_NEWEST_ITEM;

		a = c_cfg.equip_text_colour ? EQUIP_TEXT_COLOUR2 : EQUIP_TEXT_COLOUR;
		/* Colour artifact-name luck bonus affected slots */
		if (c_cfg.equip_set_colour) switch (equip_set[i - INVEN_WIELD]) {
		case 0: case 1: break; //keep standard colour
		case 2: a = TERM_GREEN; break;
		case 3: a = TERM_L_BLUE; break;
		case 4: a = TERM_VIOLET; break;
		case 5: a = TERM_RED; break;
		case 6: a = TERM_ORANGE; break;
		default: a = TERM_L_UMBER; //oO (actual cap is at 9 or more items -> +40 luck)
		}

		/* Display the index (or blank space) */
		Term_putstr(0, i - INVEN_WIELD, 3, a, tmp_val);

		/* Describe the object */
		strcpy(o_name, inventory_name[i]);

		/* Obtain length of the description */
		n = strlen(o_name);

		/* Clear the line with the (possibly indented) index */
		Term_putstr(3, i - INVEN_WIELD, n, o_ptr->attr, o_name);

		/* Erase the rest of the line */
#if defined(X_INFO_TXT_COLOURS) || 1 /* 1: actually, runes on items use a colour code! So we might need this regardless */
		col_n = 0;
		c = o_name;
		while ((c = strchr(c, '\377'))) { c++; col_n += 2; } /* Take colour codes into account! */
		Term_erase(3 + n - col_n, i - INVEN_WIELD, 255);
#else
		Term_erase(3 + n, i - INVEN_WIELD, 255);
#endif

		/* Display the weight if needed */
		if (c_cfg.show_weights && o_ptr->weight) {
			wgt = o_ptr->weight * o_ptr->number;
			if (wgt < 10000) /* still fitting into 3 digits? */
				(void)sprintf(tmp_val, "%3li.%1li lb ", wgt / 10, wgt % 10);
			else
				(void)sprintf(tmp_val, "%3lik%1li lb ", wgt / 10000, (wgt % 10000) / 1000);

			/* for 'greyed out' flexibility hack - take precedence as a vital warning */
			if (o_ptr->attr == TERM_L_DARK)
				Term_putstr(71, i - INVEN_WIELD, -1, TERM_L_DARK, tmp_val);
			/* We're in the IDDC and this item is untradable to party members? */
			else if (o_ptr->iron_trade)
				Term_putstr(71, i - INVEN_WIELD, -1, TERM_SLATE, tmp_val);
			else
				Term_putstr(71, i - INVEN_WIELD, -1, c_cfg.equip_text_colour ? EQUIP_TEXT_COLOUR2 : EQUIP_TEXT_COLOUR, tmp_val);
		}
	}

	/* Erase the rest of the window */
	for (i = INVEN_TOTAL - INVEN_WIELD; i < Term->hgt; i++) {
		/* Erase the line */
		Term_erase(0, i, 255);
	}
}

/* Just show the header line for show_inven(): Mainly the total weight.
   This is used for easy redrawal within 'i' screen when using commands from within it that erase the topline. */
void show_inven_header(void) {
	int i, j, k, z = 0;
	long int totalwgt = 0;

	object_type *o_ptr;

	char tmp_val[80];

	int out_index[23];


	/* Find the "final" slot */
	for (i = 0; i < INVEN_PACK; i++) {
		o_ptr = &inventory[i];

		/* Track non-empty slots */
		if (o_ptr->tval) z = i + 1;
	}

	/* Display the inventory */
	for (k = 0, i = 0; i < z; i++) {
		o_ptr = &inventory[i];

		/* Is this item acceptable? */
		if (!item_tester_okay(o_ptr)) continue;

		/* Save the object index, color, and descrtiption */
		out_index[k] = i;

		/* Advance to next "line" */
		k++;
	}

	/* Output each entry */
	for (j = 0; j < k; j++) {
		/* Get the index */
		i = out_index[j];

		/* Get the item */
		o_ptr = &inventory[i];

		totalwgt += o_ptr->weight * o_ptr->number;
#ifdef ENABLE_SUBINVEN
		if (o_ptr->tval == TV_SUBINVEN) {
			object_type *o2_ptr;;

			for (z = 0; z < o_ptr->bpval; z++) {
				o2_ptr = &subinventory[i][z];
				if (!o2_ptr->tval) break;
				totalwgt += o2_ptr->weight * o2_ptr->number;
			}
		}
#endif
	}

	/* Display the weight if needed */
	if (c_cfg.show_weights && totalwgt
#if 0 /* Disabled the topline_icky check, as we get ONLY called in cmd_inven(), and not from live-updating inventory timeouts: */
	    && !topline_icky /* <- for when we're inside cmd_inven() and pressed 'x' to examine an item, while a live-inven-timeout-update is coming in (only happens for equip atm tho, so not needed here) */
#endif
	    ) {
		if (totalwgt < 10000) /* still fitting into 3 digits? */
			(void)sprintf(tmp_val, "Total: %3li.%1li lb", totalwgt / 10, totalwgt % 10);
		else if (totalwgt < 10000000) /* still fitting into 3 digits? */
			(void)sprintf(tmp_val, "Total: %3lik%1li lb", totalwgt / 10000, (totalwgt % 10000) / 1000);
		else
			(void)sprintf(tmp_val, "Total: %3liM%1li lb", totalwgt / 10000000, (totalwgt % 10000000) / 1000000);
		c_put_str(TERM_L_BLUE, tmp_val, 0, 64);
	}

#ifdef ENABLE_SUBINVEN
	/* Mention the two basic commands for handling subinventories */
	c_put_str(TERM_L_BLUE, "Inventory - 's': stow items, 'b': browse books or containers.", 0, 0);
#endif

	/* hack: hide cursor */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;
}

/*
 * Display the inventory.
 *
 * Hack -- do not display "trailing" empty slots
 */
void show_inven(void) {
	int	i, j, k, l, z = 0, n;
	int	col, len, lim;
	long int wgt, totalwgt = 0;
#ifdef ENABLE_SUBINVEN
	object_type *o2_ptr;
	long int subwgt;
	bool warn_W, warn_G;
	int sn;
#endif

	object_type *o_ptr;

	char	o_name[ONAME_LEN];
	char	tmp_val[80];

	int	out_index[23];
	byte	out_color[23];
	char	out_desc[23][ONAME_LEN];


#ifdef USE_SOUND_2010
 #if 0 /* actually too spammy because the inventory is opened for a lot of fast-paced actions all the time. */
	sound(browseinven_sound_idx, SFX_TYPE_COMMAND, 100, 0);
 #endif
#endif

	/* Starting column */
	col = command_gap;

	/* Default "max-length" */
	len = 79 - col;

	/* Maximum space allowed for descriptions */
	lim = 79 - 3;

	/* Require space for weight (if needed) */
	lim -= 9;


	/* Find the "final" slot */
	for (i = 0; i < INVEN_PACK; i++) {
		o_ptr = &inventory[i];

		/* Track non-empty slots */
		if (o_ptr->tval) z = i + 1;
	}

	/* Display the inventory */
	for (k = 0, i = 0; i < z; i++) {
		o_ptr = &inventory[i];

		/* Is this item acceptable? */
		if (!item_tester_okay(o_ptr)) continue;

		/* Describe the object */
		strcpy(o_name, inventory_name[i]);

		/* Hack -- enforce max length */
		o_name[lim] = '\0';

		/* Save the object index, color, and descrtiption */
		out_index[k] = i;
		out_color[k] = o_ptr->attr;
		(void)strcpy(out_desc[k], o_name);

		/* Find the predicted "line length" */
		l = strlen(out_desc[k]) + 5;

		/* Be sure to account for the weight */
		l += 9;

		/* Maintain the maximum length */
		if (l > len) len = l;

		/* Advance to next "line" */
		k++;
	}

	/* For screen_line_icky: Fill the whole map-screen */
	screen_column_icky = 79 - len - 2 - 1; /* -2: Item list has 2 leading spaces at the start of each line, -1: start _before_ this column */
	screen_line_icky = k + 1 + 1; /* +1: Item list as 1 trailing empty 'border' line */

	/* Find the column to start in */
	col = (len > 76) ? 0 : (79 - len);

	/* Output each entry */
	for (j = 0; j < k; j++) {
		/* Get the index */
		i = out_index[j];

		/* Get the item */
		o_ptr = &inventory[i];

		/* Clear the line */
		prt("", j + 1, col ? col - 2 : col);

		/* Prepare and index --(-- */
		sprintf(tmp_val, "%c)", index_to_label(i));

		if (c_cfg.show_newest && item_newest == i) tmp_val[2] = MARKER_NEWEST_ITEM;

		/* Display the index */
		if ((n = check_guard_inscription_str(inventory_name[i], 'W')) && inventory[i].number < n) c_put_str(TERM_ORANGE, tmp_val, j + 1, col);
		else if ((n = check_guard_inscription_str(inventory_name[i], 'G')) && inventory[i].number < n - 1) c_put_str(TERM_YELLOW, tmp_val, j + 1, col);
		else put_str(tmp_val, j + 1, col);

		/* Display the weight if needed */
		wgt = 0;
		if (c_cfg.show_weights && o_ptr->weight) {
			wgt = o_ptr->weight * o_ptr->number;
#ifdef ENABLE_SUBINVEN
			warn_W = warn_G = FALSE;
			subwgt = 0;
			if (o_ptr->tval == TV_SUBINVEN) {
				for (l = 0; l < o_ptr->bpval; l++) {
					o2_ptr = &subinventory[i][l];
					if (!o2_ptr->tval) break;
					subwgt += o2_ptr->weight * o2_ptr->number;

					if (!warn_W && (sn = check_guard_inscription_str(subinventory_name[i][l], 'W')) && subinventory[i][l].number < sn) warn_W = TRUE;
					else if (!warn_G && (sn = check_guard_inscription_str(subinventory_name[i][l], 'G')) && subinventory[i][l].number < sn - 1) warn_G = TRUE;
				}
				totalwgt += subwgt;
				wgt += subwgt;

				/* Add fill state to subinven bag name? */
				if (warn_W) strcat(out_desc[j], format(" \377o[\377-%d/%d\377o]\377-", l, o_ptr->bpval));
				else if (warn_G) strcat(out_desc[j], format(" \377y[\377-%d/%d\377y]\377-", l, o_ptr->bpval));
				else strcat(out_desc[j], format(" [%d/%d]", l, o_ptr->bpval));
				out_desc[j][lim] = 0; /* Ensure overflow protection */
			}
#endif
			if (wgt < 10000) /* still fitting into 3 digits? */
				(void)sprintf(tmp_val, "%3li.%1li lb", wgt / 10, wgt % 10);
			else
				(void)sprintf(tmp_val, "%3lik%1li lb", wgt / 10000, (wgt % 10000) / 1000);
			totalwgt += wgt;
		}

		/* Display the entry itself */
		c_put_str(out_color[j], out_desc[j], j + 1, col + 3);

		if (wgt) {
			/* We're in the IDDC and this item is untradable to party members? */
			if (o_ptr->iron_trade)
				c_put_str(TERM_SLATE, tmp_val, j + 1, 71);
			else
				put_str(tmp_val, j + 1, 71);
		}
	}

	/* Display the weight if needed */
	if (c_cfg.show_weights && totalwgt
	    && !topline_icky) { /* <- for when we're inside cmd_inven() and pressed 'x' to examine an item, while a live-inven-timeout-update is coming in (only happens for equip atm tho, so not needed here) */
		if (totalwgt < 10000) /* still fitting into 3 digits? */
			(void)sprintf(tmp_val, "Total: %3li.%1li lb", totalwgt / 10, totalwgt % 10);
		else if (totalwgt < 10000000) /* still fitting into 3 digits? */
			(void)sprintf(tmp_val, "Total: %3lik%1li lb", totalwgt / 10000, (totalwgt % 10000) / 1000);
		else
			(void)sprintf(tmp_val, "Total: %3liM%1li lb", totalwgt / 10000000, (totalwgt % 10000000) / 1000000);
		c_put_str(TERM_L_BLUE, tmp_val, 0, 64);
	}

#ifdef ENABLE_SUBINVEN
	/* Mention the two basic commands for handling subinventories */
	c_put_str(TERM_L_BLUE, "Inventory - 's': stow items, 'b': browse books or containers.", 0, 0);
#endif

	/* Make a "shadow" below the list (only if needed) */
	if (j && (j < 23)) prt("", j + 1, col ? col - 2 : col);

	/* Notify if inventory is actually empty */
	if (!k) {
		prt("(Your inventory is empty)", 1, SCREEN_PAD_LEFT);
		/* ..and don't overwrite this notification by weather etc */
		screen_line_icky = 2; /* 0 lines for empty inven, +1 +1 */
		screen_column_icky = 12; /* left padding (just status bar, as we start left-aligned in the main window (unlike inventory which is right-aligend). */
	}

	/* Save the new column */
	command_gap = col;

	/* Do not allow environmental redrawing if we're actually already inside an icky screen here.
	   (Concerns shopping only atm I think, but checking screen_icky seems better than checking 'shopping') */
	if (screen_icky != 1) screen_line_icky = screen_column_icky = -1;
}

#ifdef ENABLE_SUBINVEN
//todo: topline_icky is (wrongfully?) TRUE, so the totalwgt isn't shown!
void show_subinven(int islot) {
	int	i, j, k, l, z = 0, n;
	int	col, len, lim;
	long int wgt, totalwgt = 0;

	object_type *o_ptr;

	char	o_name[ONAME_LEN];
	char	tmp_val[80];

	int	out_index[23];
	byte	out_color[23];
	char	out_desc[23][ONAME_LEN];

	object_type *i_ptr = &inventory[islot];
	int subinven_size = i_ptr->bpval;

#ifdef USE_SOUND_2010
 #if 0 /* actually too spammy because the inventory is opened for a lot of fast-paced actions all the time. */
	sound(browseinven_sound_idx, SFX_TYPE_COMMAND, 100, 0);
 #endif
#endif

	/* Starting column */
	col = command_gap;

	/* Default "max-length" */
	len = 79 - col;

	/* Maximum space allowed for descriptions */
	lim = 79 - 3;

	/* Require space for weight (if needed) */
	lim -= 9;


	/* Find the "final" slot */
	for (i = 0; i < subinven_size; i++) {
		o_ptr = &subinventory[islot][i];

		/* Track non-empty slots */
		if (o_ptr->tval) z = i + 1;
	}

	/* Display the inventory */
	for (k = 0, i = 0; i < z; i++) {
		o_ptr = &subinventory[islot][i];

		/* Is this item acceptable? */
		if (!item_tester_okay(o_ptr)) continue;

		/* Describe the object */
		strcpy(o_name, subinventory_name[islot][i]);

		/* Hack -- enforce max length */
		o_name[lim] = '\0';

		/* Save the object index, color, and descrtiption */
		out_index[k] = i;
		out_color[k] = o_ptr->attr;
		(void)strcpy(out_desc[k], o_name);

		/* Find the predicted "line length" */
		l = strlen(out_desc[k]) + 5;

		/* Be sure to account for the weight */
		l += 9;

		/* Maintain the maximum length */
		if (l > len) len = l;

		/* Advance to next "line" */
		k++;
	}

	/* For screen_line_icky: Fill the whole map-screen */
	screen_column_icky = 79 - len - 2 - 1; /* -2: Item list has 2 leading spaces at the start of each line, -1: start _before_ this column */
	screen_line_icky = k + 1 + 1; /* +1: Item list as 1 trailing empty 'border' line */

	/* Find the column to start in */
	col = (len > 76) ? 0 : (79 - len);

	/* The ugly method of overwriting the subinven via show_subinven() in Receive_subinven()
	   will cause 'This container is empty' residue here, so clear it up first.
	   Bad hack again:*/
	//prt("                                                                   ", 1, SCREEN_PAD_LEFT);
	prt("                                                                                ", 1, 0);
	/* ...but since then again weather particles will overwrite the left part of the blankness which
	   is out of range of the actual item lines which almost always start way more to the right,
	   we need to adjust the icky-column just for this to ward against ugly particle tearing -_- nice bad-crap cascade there */
	screen_column_icky = 0; //SCREEN_PAD_LEFT + 1;

	/* Output each entry */
	for (j = 0; j < k; j++) {
		/* Get the index */
		i = out_index[j];

		/* Get the item */
		o_ptr = &subinventory[islot][i];

		/* Clear the line */
		prt("", j + 1, col ? col - 2 : col);

		/* Prepare and index --(-- */
		sprintf(tmp_val, "%c)", index_to_label(i));

		if (c_cfg.show_newest && item_newest == (islot + 1) * 100 + i) tmp_val[2] = MARKER_NEWEST_ITEM;

		/* Clear the line with the (possibly indented) index */
		if ((n = check_guard_inscription_str(subinventory_name[islot][i], 'W')) && subinventory[islot][i].number < n) c_put_str(TERM_ORANGE, tmp_val, j + 1, col);
		else if ((n = check_guard_inscription_str(subinventory_name[islot][i], 'G')) && subinventory[islot][i].number < n - 1) c_put_str(TERM_YELLOW, tmp_val, j + 1, col);
		else put_str(tmp_val, j + 1, col);

		/* Display the entry itself */
		c_put_str(out_color[j], out_desc[j], j + 1, col + 3);

		/* Display the weight if needed */
		if (c_cfg.show_weights && o_ptr->weight) {
			wgt = o_ptr->weight * o_ptr->number;
			if (wgt < 10000) /* still fitting into 3 digits? */
				(void)sprintf(tmp_val, "%3li.%1li lb", wgt / 10, wgt % 10);
			else
				(void)sprintf(tmp_val, "%3lik%1li lb", wgt / 10000, (wgt % 10000) / 1000);

			/* We're in the IDDC and this item is untradable to party members? */
			if (o_ptr->iron_trade)
				c_put_str(TERM_SLATE, tmp_val, j + 1, 71);
			else
				put_str(tmp_val, j + 1, 71);

			totalwgt += wgt;
		}
	}

	/* Display the weight if needed */
	if (c_cfg.show_weights && totalwgt
	    && !topline_icky) { /* <- for when we're inside cmd_inven() and pressed 'x' to examine an item, while a live-inve-timeout-update is coming in (only happens for equip atm tho, so not needed here) */
		if (totalwgt < 10000) /* still fitting into 3 digits? */
			(void)sprintf(tmp_val, "Total: %3li.%1li lb", totalwgt / 10, totalwgt % 10);
		else if (totalwgt < 10000000) /* still fitting into 3 digits? */
			(void)sprintf(tmp_val, "Total: %3lik%1li lb", totalwgt / 10000, (totalwgt % 10000) / 1000);
		else
			(void)sprintf(tmp_val, "Total: %3liM%1li lb", totalwgt / 10000000, (totalwgt % 10000000) / 1000000);
		c_put_str(TERM_L_BLUE, tmp_val, 0, 64);
	}

	/* Mention the two basic commands for handling subinventories */
	//c_put_str(TERM_L_BLUE, format("Bag contents (%d/%d) - 's': unstow, 'a': container-dependant activate.", k, inventory[islot].bpval), 0, 0);
	/* Hack: If shopping, also show 'S' to sell something */
	if (shopping) {
		if (i_ptr->sval == SV_SI_SATCHEL) c_put_str(TERM_L_BLUE, format("Bag contents (%d/%d) - ESC, S: sell, s: unstow, a: mix chemicals, x/d/k/{/}/H/K.", k, inventory[islot].bpval), 0, 0);
		else c_put_str(TERM_L_BLUE, format("Bag contents (%d/%d) - ESC: exit, S: sell, s: unstow, x/d/k/{/}/H/K.", k, inventory[islot].bpval), 0, 0);
	} else {
		if (i_ptr->sval == SV_SI_SATCHEL) c_put_str(TERM_L_BLUE, format("Bag contents (%d/%d) - ESC: exit, s: unstow, a: mix chemicals, x/d/k/{/}/H/K.", k, inventory[islot].bpval), 0, 0);
		else c_put_str(TERM_L_BLUE, format("Bag contents (%d/%d) - ESC: exit, s: unstow, x/d/k/{/}/H/K.", k, inventory[islot].bpval), 0, 0);
	}

	/* Make a "shadow" below the list (only if needed) */
	if (j && (j < 23)) prt("", j + 1, col ? col - 2 : col);

	/* Notify if inventory is actually empty */
	if (!k) {
		/* Display a line if inventory is actually empty */
		c_prt((i_ptr->attr == TERM_L_DARK) ? TERM_L_DARK : TERM_L_WHITE,
		    (i_ptr->attr == TERM_L_DARK) ? /* Use colour sent by server as indicator for SUBINVEN_LIMIT_GROUP */
		    "(This container is of duplicate type and therefore unusable)" :
		    "(This container is empty)",
		    1, SCREEN_PAD_LEFT);
		/* Hack as if k was 1, to protect this 'is empty' text from weather particles etc, geez */
		screen_line_icky = 1 + 1 + 1;
	}

	/* hack: hide cursor */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;

	/* Save the new column */
	command_gap = col;

	/* Do not allow environmental redrawing if we're actually already inside an icky screen here.
	   (Concerns shopping only atm I think, but checking screen_icky seems better than checking 'shopping') */
	if (screen_icky != 1) screen_line_icky = screen_column_icky = -1;
}
#endif


/*
 * Display the equipment.
 */
void show_equip(void) {
	int	i, j, k, l;
	int	col, len, lim;
	long int wgt, totalwgt = 0, armourwgt = 0, shieldwgt = 0;

	object_type *o_ptr;

	char	o_name[ONAME_LEN];
	char	tmp_val[80];

	int	out_index[23];
	byte	out_color[23], a;
	char	out_desc[23][ONAME_LEN];


	/* Starting column */
	col = command_gap;

	/* Default "max-length" */
	len = 79 - col;

	/* Maximum space allowed for descriptions */
	lim = 79 - 3;

	/* Require space for weight (if needed) */
	lim -= 9;


	/* Scan the equipment list */
	for (k = 0, i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
		o_ptr = &inventory[i];

		/* Is this item acceptable? */
		if (!item_tester_okay(o_ptr)) continue;

		/* Describe the object */
		strcpy(o_name, inventory_name[i]);

		/* Hack -- enforce max length */
		o_name[lim] = '\0';

		/* Save the object index, color, and descrtiption */
		out_index[k] = i;
		out_color[k] = o_ptr->attr;
		(void)strcpy(out_desc[k], o_name);

		/* Find the predicted "line length" */
		l = strlen(out_desc[k]) + 5;

		/* Be sure to account for the weight */
		l += 9;

		/* Maintain the maximum length */
		if (l > len) len = l;

		/* Advance to next "line" */
		k++;
	}

	/* For screen_line_icky: Fill the whole map-screen */
	screen_column_icky = 79 - len - 2 - 1; /* -2: Item list has 2 leading spaces at the start of each line, -1: start _before_ this column */
	screen_line_icky = k + 1 + 1; /* +1: Item list as 1 trailing empty 'border' line */

	/* Find the column to start in */
	col = (len > 76) ? 0 : (79 - len);

	/* Output each entry */
	for (j = 0; j < k; j++) {
		/* Get the index */
		i = out_index[j];

		/* Get the item */
		o_ptr = &inventory[i];

		/* Clear the line */
		prt("", j + 1, col ? col - 2 : col);

		/* Prepare and index --(-- */
		sprintf(tmp_val, "%c)", index_to_label(i));

		if (c_cfg.show_newest && item_newest == i) tmp_val[2] = MARKER_NEWEST_ITEM;

		a = c_cfg.equip_text_colour ? EQUIP_TEXT_COLOUR2 : EQUIP_TEXT_COLOUR;
		/* Colour artifact-name luck bonus affected slots */
		if (c_cfg.equip_set_colour) switch (equip_set[i - INVEN_WIELD]) {
		case 0: case 1: break; //keep standard colour
		case 2: a = TERM_GREEN; break;
		case 3: a = TERM_L_BLUE; break;
		case 4: a = TERM_VIOLET; break;
		case 5: a = TERM_L_RED; break;
		case 6: a = TERM_ORANGE; break;
		default: a = TERM_L_UMBER; //oO (actual cap is at 9 or more items -> +40 luck)
		}

		/* Clear the line with the (possibly indented) index */
		c_put_str(a, tmp_val, j + 1, col);

		/* Display the entry itself */
		c_put_str(out_color[j], out_desc[j], j + 1, col + 3);

		/* Display the weight if needed */
		if (c_cfg.show_weights && o_ptr->weight) {
			wgt = o_ptr->weight * o_ptr->number;
			if (wgt < 10000) /* still fitting into 3 digits? */
				(void)sprintf(tmp_val, "%3li.%1li lb", wgt / 10, wgt % 10);
			else
				(void)sprintf(tmp_val, "%3lik%1li lb", wgt / 10000, (wgt % 10000) / 1000);

			/* for 'greyed out' flexibility hack -- take precedence as a vital warning */
			if (o_ptr->attr == TERM_L_DARK)
				c_put_str(TERM_L_DARK, tmp_val, j + 1, 71);
			/* We're in the IDDC and this item is untradable to party members? */
			else if (o_ptr->iron_trade)
				c_put_str(TERM_SLATE, tmp_val, j + 1, 71);
			else
				put_str(tmp_val, j + 1, 71);

			totalwgt += wgt;
			if (is_armour(o_ptr->tval)) armourwgt += wgt;
			if (o_ptr->tval == TV_SHIELD) shieldwgt += wgt;
		}
	}

	/* Display the weight if needed */
	if (c_cfg.show_weights && totalwgt
	    && !topline_icky) { /* <- for when we're inside cmd_equip() and pressed 'x' to examine an item, while a live-equip-timeout-update is coming in */
		if (totalwgt < 10000) /* still fitting into 3 digits? */
			(void)sprintf(tmp_val, "Total: %3li.%1li lb", totalwgt / 10, totalwgt % 10);
		else if (totalwgt < 10000000) /* still fitting into 3 digits? */
			(void)sprintf(tmp_val, "Total: %3lik%1li lb", totalwgt / 10000, (totalwgt % 10000) / 1000);
		else
			(void)sprintf(tmp_val, "Total: %3liM%1li lb", totalwgt / 10000000, (totalwgt % 10000000) / 1000000);
		c_put_str(TERM_L_BLUE, tmp_val, 0, 64);

		if (!shieldwgt) (void)sprintf(tmp_val, "Armour: %3li.%1li lb", armourwgt / 10, armourwgt % 10);
		else (void)sprintf(tmp_val, "Armour: %3li.%1li lb (%li.%1li lb)", armourwgt / 10, armourwgt % 10, (armourwgt - shieldwgt) / 10, (armourwgt - shieldwgt) % 10);
		c_put_str(TERM_L_BLUE, tmp_val, 0, col);
	}

	/* Make a "shadow" below the list (only if needed) */
	if (j && (j < 23)) prt("", j + 1, col ? col - 2 : col);

	/* Save the new column */
	command_gap = col;

	/* Do not allow environmental redrawing if we're actually already inside an icky screen here.
	   (Concerns shopping only atm I think, but checking screen_icky seems better than checking 'shopping') */
	if (screen_icky != 1) screen_line_icky = screen_column_icky = -1;
}


/*
 * Display inventory in sub-windows
 */
static void fix_inven(void) {
	int j;
	term *old;

	/* Scan windows */
	for (j = 1; j < ANGBAND_TERM_MAX; j++) {
		/* No window */
		if (!ang_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & PW_INVEN)) continue;

		/* Save */
		old = Term;

		/* Activate */
		Term_activate(ang_term[j]);

		/* Display inventory */
		display_inven();

		/* Fresh */
		Term_fresh();

		/* Restore */
		Term_activate(old);
	}

#ifdef ENABLE_SUBINVEN
	if (showing_inven && showing_inven == screen_icky) {
		if (using_subinven != -1) show_subinven(using_subinven);
		else show_inven();
	} else
#endif
	if (showing_inven && showing_inven == screen_icky) show_inven();

	/* Assume that this could've been in response to a wield/takeoff/swap command we issued */
	command_confirmed = PKT_UNDEFINED; //..we don't know which one (doesn't matter)
}

#ifdef ENABLE_SUBINVEN
/*
 * Display inventory in sub-windows
 */
static void fix_subinven(void) {
	int j;
	term *old;

	/* Scan windows */
	for (j = 1; j < ANGBAND_TERM_MAX; j++) {
		/* No window */
		if (!ang_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & PW_SUBINVEN)) continue;

		/* Save */
		old = Term;

		/* Activate */
		Term_activate(ang_term[j]);

		/* Display inventory */
		display_subinven();

		/* Fresh */
		Term_fresh();

		/* Restore */
		Term_activate(old);
	}

	//show_subinven(using_subinven);
	//show_inven();

	/* Assume that this could've been in response to a wield/takeoff/swap command we issued */
	//command_confirmed = PKT_UNDEFINED; //..we don't know which one (doesn't matter)
}
#endif

/*
 * Display equipment in sub-windows
 */
static void fix_equip(void) {
	int j;
	term *old;

	/* Scan windows */
	for (j = 1; j < ANGBAND_TERM_MAX; j++) {
		/* No window */
		if (!ang_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & PW_EQUIP)) continue;

		/* Save */
		old = Term;

		/* Activate */
		Term_activate(ang_term[j]);

		/* Display equipment */
		display_equip();

		/* Fresh */
		Term_fresh();

		/* Restore */
		Term_activate(old);
	}

	if (showing_equip && showing_equip == screen_icky) show_equip();
}


/*
 * Display character sheet in sub-windows
 */
static void fix_player(void) {
	int j;
	term *old;

	/* Scan windows */
	for (j = 1; j < ANGBAND_TERM_MAX; j++) {
		/* No window */
		if (!ang_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & PW_PLAYER)) continue;

		/* Save */
		old = Term;

		/* Activate */
		Term_activate(ang_term[j]);

		/* Display character */
		display_player(csheet_page);

		/* Fresh */
		Term_fresh();

		/* Restore */
		Term_activate(old);
	}
}


/*
 * Hack -- display recent messages in sub-windows
 *
 * XXX XXX XXX Adjust for width and split messages
 */
static void fix_message(void) {
	int j, i;
	int w, h;
	int x, y;

	cptr msg;
	byte a;
	term *old;


	/* Display messages in different colors -Zz */

	/* Scan windows */
	for (j = 1; j < ANGBAND_TERM_MAX; j++) {
		/* No window */
		if (!ang_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] &
		    (PW_MESSAGE | PW_CHAT | PW_MSGNOCHAT))) continue;

		/* Save */
		old = Term;

		/* Activate */
		Term_activate(ang_term[j]);

		/* Get size */
		Term_get_size(&w, &h);

		/* Does this terminal show the normal message_str buffer? or chat/msgnochat only?*/
		if (window_flag[j] & PW_CHAT) {
			/* Dump messages */
			for (i = 0; i < h; i++) {
				a = TERM_WHITE;
				msg = message_str_chat(i);

				/* Dump the message on the appropriate line */
				Term_putstr(0, (h - 1) - i, -1, a, (char*)msg);

				/* Cursor */
				Term_locate(&x, &y);

				/* Clear to end of line */
				Term_erase(x, y, 255);
			}
		} else if (window_flag[j] & PW_MSGNOCHAT) {
			/* Dump messages */
			for (i = 0; i < h; i++) {
				a = TERM_WHITE;
				msg = message_str_msgnochat(i);

				/* Dump the message on the appropriate line */
				Term_putstr(0, (h - 1) - i, -1, a, (char*)msg);

				/* Cursor */
				Term_locate(&x, &y);

				/* Clear to end of line */
				Term_erase(x, y, 255);
			}
		} else {
			/* Dump messages */
			for (i = 0; i < h; i++)
			{
				a = TERM_WHITE;
				msg = message_str(i);

				/* strip remaining control codes that were left in
				   this main buffer for purpose of CTRL+O/P scrollback
				   control, before actually displaying the message. */
				if (msg[0] == '\376') msg++;

				/* Dump the message on the appropriate line */
				Term_putstr(0, (h - 1) - i, -1, a, (char*)msg);

				/* Cursor */
				Term_locate(&x, &y);

				/* Clear to end of line */
				Term_erase(x, y, 255);
			}
		}

		/* Fresh */
		Term_fresh();

		/* Restore */
		Term_activate(old);
	}
}

static void fix_lagometer(void) {
	int j;
	term *old;

	/* Scan windows */
	for (j = 1; j < ANGBAND_TERM_MAX; j++) {
		/* No window */
		if (!ang_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & PW_LAGOMETER)) continue;

		/* Save */
		old = Term;

		/* Activate */
		Term_activate(ang_term[j]);

		/* Display lag-o-meter */
		display_lagometer(FALSE);

		/* Fresh */
		Term_fresh();

		/* Restore */
		Term_activate(old);
	}
}

void fix_playerlist(void) {
	int i, j, p, y;
	term *old;

	/* Scan windows */
	for (j = 1; j < ANGBAND_TERM_MAX; j++) {
		/* No window */
		if (!ang_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & PW_PLAYERLIST)) continue;

		/* Save */
		old = Term;

		/* Activate */
		Term_activate(ang_term[j]);

		/* List players */
		Term_clear();
		//prt(format("- [Players Online (%d)] -", NumPlayers), 1, 6);
		if (NumPlayers == 1) 
			c_prt(TERM_L_WHITE, " 1 Player Online:", 0, 1);
		else
			c_prt(TERM_L_WHITE, format(" %d Players Online%c", NumPlayers, NumPlayers ? ':' : '.'), 0, 1);
		p = NumPlayers;
		if (p > MAX_PLAYERS_LISTED) p = MAX_PLAYERS_LISTED;
		y = 0;
		for (i = 0; i < p; i++) {
			if (!playerlist_name[i][0]) continue;
			c_put_str(TERM_WHITE, playerlist[i], 2 + y, 1);
			y++;
		}

		/* Fresh */
		Term_fresh();

		/* Restore */
		Term_activate(old);
	}
}

void check_for_playerlist(void) {
	int j;

	/* Scan windows */
	for (j = 1; j < ANGBAND_TERM_MAX; j++) {
		/* No window */
		if (!ang_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & PW_PLAYERLIST)) continue;

		/* Window set to invisible aka deactivated */
		if (!term_get_visibility(j)) continue;

		break;
	}

	Send_plistw_notify(j != ANGBAND_TERM_MAX);
}


/*
 * Display last character sheet in sub-windows
 */
static void fix_player2(void) {
	int j;
	term *old;

	/* Scan windows */
	for (j = 1; j < ANGBAND_TERM_MAX; j++) {
		/* No window */
		if (!ang_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & PW_PLAYER2)) continue;

		/* Save */
		old = Term;

		/* Activate */
		Term_activate(ang_term[j]);

		/* Display character */
		display_player(2);

		/* Fresh */
		Term_fresh();

		/* Restore */
		Term_activate(old);
	}
}


/*
 * Hack -- pass color info around this file
 */
static byte likert_color = TERM_WHITE;


/*
 * Returns a "rating" of x depending on y
 */
static cptr likert(int x, int y, int max) {
	/* Paranoia */
	if (y <= 0) y = 1;

	/* Negative values */
	if (x < 0) {
		likert_color = TERM_RED;
		return("Very Bad");
	}

	/* Highest possible value reached */
	if ((x >= max) && max) {
		likert_color = TERM_L_UMBER;
		return("Legendary");
	}

	/* Analyze the value */
	switch (((x * 10) / y)) {
		case 0:
		case 1:
			likert_color = TERM_RED;
			return("Bad");
		case 2:
			likert_color = TERM_RED;
			return("Poor");
		case 3:
		case 4:
			likert_color = TERM_YELLOW;
			return("Fair");
		case 5:
			likert_color = TERM_YELLOW;
			return("Good");
		case 6:
			likert_color = TERM_YELLOW;
			return("Very Good");
		case 7:
		case 8:
			likert_color = TERM_L_GREEN;
			return("Excellent");
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
			likert_color = TERM_L_GREEN;
			return("Superb");
		case 14:
		case 15:
		case 16:
		case 17:
			likert_color = TERM_L_GREEN;
			return("Heroic");
		default:
			/* indicate that there is a maximum value */
			if (max) likert_color = TERM_GREEN;
			/* indicate that there is no maximum */
			else likert_color = TERM_L_GREEN;
			return("Legendary");
	}
}

/*
 * Draws the lag-o-meter.
 */
#define COLOURED_LAGOMETER
#ifndef STEPS33
 #define STEP_RED 400
 #define STEP_ORANGE 300
 #define STEP_YELLOW 200
 #define STEP_GREEN 100
#else
 #define STEP_RED 300
 #define STEP_ORANGE 200
 #define STEP_YELLOW 133
 #define STEP_GREEN 67
#endif
void display_lagometer(bool display_commands) {
	int i, cnt, sum, cur, min, max, avg, x, y, packet_loss, height;
	char tmp[80];
	char graph[16][61];
#ifdef COLOURED_LAGOMETER
	byte attr = TERM_VIOLET; /* silyl compiler */
 #if 0 /* use same colours as mini lag-o-meter? */
//	byte attr0 = TERM_L_GREEN, attr1 = TERM_GREEN;
	char attrc0 = 'G', attrc1 = 'g';
 #else /* use white instead of green tones, for being easier on the eye maybe? */
//	byte attr0 = TERM_L_WHITE, attr1 = TERM_SLATE;
	char attrc0 = 'W', attrc1 = 's';
 #endif
	int cur_lag;
	char *underscore, *underscore2, *underscore2a, c_graph[60 * 3 + 1];
	char attrc = 'v'; /* silyl compiler */
	char attrc_prev = attrc0; /* assume best case at the top line */
#endif

	/* Clear screen */
	Term_clear();

	/* Why are we here */
	prt("The Lag-o-meter", 1, 30);

	/* Find min and max and calculate avg */
	packet_loss = cnt = sum = 0;
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
		else if (ping_times[i] == -1) {
			packet_loss++;
		}
	}

	if (cnt) avg = sum / cnt;
	else avg = -1;

	/* Latest ping might not be lost yet */
	if (ping_times[0] == -1) {
		packet_loss--;
		cur = ping_times[1];
	}
	else cur = ping_times[0];

	/* Clear the graph */
	for (y = 0; y < 16; y++) {
		for (x = 0; x < 60; x++) {
			graph[y][x] = ' ';
		}
		graph[y][60] = '\0';
	}

	/* Create the graph */
	for (i = 0; i < 60; i++) {
		/* Calculate the height */
		height = (16 * ping_times[i] + max / 2) / max;

		for (y = 0; y < height; y++) {
			graph[15 - y][59 - i] = '*';
		}

		/* Draw an underscore if a reply was received but it wouldn't be visible */
		if (height == 0 && ping_times[i] > 0) {
			graph[15][59 - i] = '_';
		}
	}

	/* Draw the graph */
	for (y = 0; y < 16; y++) {
#ifndef COLOURED_LAGOMETER
		prt(graph[y], 3 + y, 10);
#else
 #if 0 /* doesn't take care of correct '_' colouring */
		cur_lag = (max * (16 - y)) / 16;
		if (cur_lag >= STEP_RED) attr = TERM_RED;
		else if (cur_lag >= STEP_ORANGE) attr = TERM_ORANGE;
		else if (cur_lag >= STEP_YELLOW) attr = TERM_YELLOW;
		else if (cur_lag >= STEP_GREEN) attr = attr1;
		else attr = attr0;
		c_prt(attr, graph[y], 3 + y, 10);
 #else /* does take care */
		memset(c_graph, 0, sizeof(char) * (60 * 3 + 1));
		underscore = underscore2 = graph[y];
		while (underscore < graph[y] + 60) {
			/* handle non-underscore chunk (ie a chunk of asterisks),
			   their colouring can be handled in one go too */
			underscore2 = strchr(underscore, '_');
			if (!underscore2) underscore2 = graph[y] + 60;
			cur_lag = (max * (16 - y)) / 16;
			if (cur_lag >= STEP_RED) attrc = 'r';
			else if (cur_lag >= STEP_ORANGE) attrc = 'o';
			else if (cur_lag >= STEP_YELLOW) attrc = 'y';
			else if (cur_lag >= STEP_GREEN) attrc = attrc1;
			else attrc = attrc0;
			if (underscore2 - underscore) {
				/* avoid 'skipping' a colour level, for better visuals ;) */
				switch (attrc_prev) {
				case 'r':
					if (attrc == 'y' || attrc == attrc1 || attrc == attrc0) attrc = 'o';
					break;
				case 'o':
					if (attrc == attrc1 || attrc == attrc0) attrc = 'y';
					break;
				case 'y':
					if (attrc == attrc0) attrc = attrc1;
					break;
				}

				strcat(c_graph, format("\377%c", attrc));
				strncat(c_graph, underscore, underscore2 - underscore);
			}
			underscore = underscore2;

			/* handle a chunk of underscores,
			   but handle their colouring separately, one by one */
			if (underscore < graph[y] + 60) {
				underscore2 = strchr(underscore, '*');
				underscore2a = strchr(underscore, ' ');
				if (!underscore2) underscore2 = graph[y] + 60;
				if (!underscore2a) underscore2a = graph[y] + 60;
				if (underscore2a < underscore2) underscore2 = underscore2a;
				for (underscore2a = underscore; underscore2a < underscore2; underscore2a++) {
					cur_lag = ping_times[59 - (underscore2a - graph[y])];
					if (cur_lag >= STEP_RED) attrc = 'r';
					else if (cur_lag >= STEP_ORANGE) attrc = 'o';
					else if (cur_lag >= STEP_YELLOW) attrc = 'y';
					else if (cur_lag >= STEP_GREEN) attrc = attrc1;
					else attrc = attrc0;
					strcat(c_graph, format("\377%c%c", attrc, *underscore2a));
				}
				underscore = underscore2a;
			}
		}
		c_put_str(TERM_WHITE, c_graph, 3 + y, 10);
		attrc_prev = attrc;
 #endif
#endif
	}

#ifndef COLOURED_LAGOMETER
	prt("Cur:", 20, 2);
	if (cur != -1) sprintf(tmp, "%5dms", cur);
	else tmp[0] = '\0';
	prt(tmp, 20, 7);

	prt("Avg:", 20, 17);
	if (avg != -1) sprintf(tmp, "%5dms", avg);
	else tmp[0] = '\0';
	prt(tmp, 20, 22);

 #if 0 /* TMI */
	prt("10-min cleaned avg:", 21, 2);
	sprintf(tmp, "%5dms", ping_avg);
	else tmp[0] = '\0';
	prt(tmp, 21, 22);
 #endif

	prt("Min:", 20, 32);
	if (min != -1) sprintf(tmp, "%5dms", min);
	else tmp[0] = '\0';
	prt(tmp, 20, 37);

	prt("Max:", 20, 47);
	if (max != -1) sprintf(tmp, "%5dms", max);
	else tmp[0] = '\0';
	prt(tmp, 20, 52);

	prt("Packet Loss:", 20, 62);
	sprintf(tmp, "%2d", packet_loss);
	prt( tmp, 20, 75);
#else
	prt("Cur:", 20, 2);
	if (cur != -1) {
		sprintf(tmp, "%5dms", cur);
		if (cur >= STEP_RED) attr = TERM_RED;
		else if (cur >= STEP_ORANGE) attr = TERM_ORANGE;
		else if (cur >= STEP_YELLOW) attr = TERM_YELLOW;
		else if (cur >= STEP_GREEN) attr = TERM_GREEN;
		else attr = TERM_L_GREEN;
	}
	else tmp[0] = '\0';
	c_prt(attr, tmp, 20, 7);

	prt("Avg:", 20, 17);
	if (avg != -1) {
		sprintf(tmp, "%5dms", avg);
		if (avg >= STEP_RED) attr = TERM_RED;
		else if (avg >= STEP_ORANGE) attr = TERM_ORANGE;
		else if (avg >= STEP_YELLOW) attr = TERM_YELLOW;
		else if (avg >= STEP_GREEN) attr = TERM_GREEN;
		else attr = TERM_L_GREEN;
	}
	else tmp[0] = '\0';
	c_prt(attr, tmp, 20, 22);

 #if 0 /* TMI */
	prt("10-min cleaned avg:", 21, 2);
	if (ping_avg != -1) {
		sprintf(tmp, "%5dms", ping_avg);
		if (ping_avg >= STEP_RED) attr = TERM_RED;
		else if (ping_avg >= STEP_ORANGE) attr = TERM_ORANGE;
		else if (ping_avg >= STEP_YELLOW) attr = TERM_YELLOW;
		else if (ping_avg >= STEP_GREEN) attr = TERM_GREEN;
		else attr = TERM_L_GREEN;
	}
	else tmp[0] = '\0';
	c_prt(attr, tmp, 21, 22);
 #endif

	prt("Min:", 20, 32);
	if (min != -1) {
		sprintf(tmp, "%5dms", min);
		if (min >= STEP_RED) attr = TERM_RED;
		else if (min >= STEP_ORANGE) attr = TERM_ORANGE;
		else if (min >= STEP_YELLOW) attr = TERM_YELLOW;
		else if (min >= STEP_GREEN) attr = TERM_GREEN;
		else attr = TERM_L_GREEN;
	}
	else tmp[0] = '\0';
	c_prt(attr, tmp, 20, 37);

	prt("Max:", 20, 47);
	if (max != -1) {
		sprintf(tmp, "%5dms", max);
		if (max >= STEP_RED) attr = TERM_RED;
		else if (max >= STEP_ORANGE) attr = TERM_ORANGE;
		else if (max >= STEP_YELLOW) attr = TERM_YELLOW;
		else if (max >= STEP_GREEN) attr = TERM_GREEN;
		else attr = TERM_L_GREEN;
	}
	else tmp[0] = '\0';
	c_prt(attr, tmp, 20, 52);

	prt("Packet Loss:", 20, 62);
	sprintf(tmp, "%2d", packet_loss);
	c_prt(packet_loss ? TERM_RED : TERM_L_GREEN, tmp, 20, 75);
#endif

	if (display_commands) {
#ifndef COLOURED_LAGOMETER
		if (lagometer_enabled) {
			prt("(2) Disable lag-o-meter", 22, 4);
		} else {
			prt("(1) Enable lag-o-meter", 22, 4);
		}
		prt("(c) Clear lag-o-meter", 22, 40);
#else
		if (lagometer_enabled) {
			c_put_str(TERM_WHITE, "(\377U2\377w) Disable lag-o-meter", 22, 4);
		} else {
			c_put_str(TERM_WHITE, "(\377U1\377w) Enable lag-o-meter", 22, 4);
		}
		c_put_str(TERM_WHITE, "(\377Uc\377w) Clear lag-o-meter", 22, 40);
#endif
//		prt("Command: ", 23, 2);
	}
}

/*
 * Update the lag-o-meter if it's open.
 */
void update_lagometer() {
	if (lagometer_open) {
		display_lagometer(TRUE);

		/* Fresh in case there's no net input */
		Term_fresh();
	}

	/* Update any other windows */
	p_ptr->window |= PW_LAGOMETER;
}

void display_player(int hist) {
	int i, tmp;
	char buf[80], tmpc;
	cptr desc;

	if (hist != 2) {
#ifndef NEW_COMPRESSED_DUMP_AC
		int y_row1 = 1, y_row2 = 8, y_row3 = 15, y_rowmode = y_row1 + 5;
#else
		int y_row1 = 1, y_row2 = 9, y_row3 = 15, y_rowmode = y_row1 + 6;
#endif

		/* Clear screen */
		clear_from(0);

		/* Name, Sex, Race, Class */
#define FIRST_COL 8
		put_str("Name :", y_row1, 1);
		put_str("Sex  :", y_row1 + 1, 1);
		put_str("Race :", y_row1 + 2, 1);
		put_str("Class:", y_row1 + 3, 1);
		put_str("Body :", y_row1 + 4, 1);
#ifdef NEW_COMPRESSED_DUMP_AC
 #ifdef HIDE_UNAVAILABLE_TRAIT
		if (trait != 0)
 #endif
		{
			put_str("Trait:", y_row1 + 5, 1);
			c_put_str(TERM_L_BLUE, trait_info[trait].title, y_row1 + 5, FIRST_COL);
		}
#endif
		put_str("Mode :", y_rowmode, 1);

		c_put_str(TERM_L_BLUE, cname, y_row1, FIRST_COL);
#if 1 /* Also print body modification here? */
		if (p_ptr->fruit_bat)
			c_put_str(TERM_L_BLUE, (p_ptr->male ? "Male (Fruit bat)" : "Female (Fruit bat)"), y_row1 + 1, FIRST_COL);
		else
			c_put_str(TERM_L_BLUE, (p_ptr->male ? "Male" : "Female"), y_row1 + 1, FIRST_COL);
#endif
		c_put_str(TERM_L_BLUE, race_info[race].title, y_row1 + 2, FIRST_COL);
		c_put_str(TERM_L_BLUE, class_info[class].title, y_row1 + 3, FIRST_COL);
		c_put_str(TERM_L_BLUE, c_p_ptr->body_name, y_row1 + 4, FIRST_COL);
		if (p_ptr->mode & MODE_PVP)
			c_put_str(TERM_YELLOW, "PvP (one life)", y_rowmode, FIRST_COL);
		else if (p_ptr->mode & MODE_EVERLASTING)
			c_put_str(TERM_L_BLUE, "Everlasting (infinite lives)", y_rowmode, FIRST_COL);
		else if ((p_ptr->mode & MODE_NO_GHOST) && (p_ptr->mode & MODE_HARD))
			c_put_str(TERM_RED, "Hellish (one life, extra hard)", y_rowmode, FIRST_COL);
		else if ((p_ptr->mode & MODE_NO_GHOST) && (p_ptr->mode & MODE_SOLO))
			c_put_str(TERM_L_DARK, "Soloist (one life)", y_rowmode, FIRST_COL);
		else if (p_ptr->mode & MODE_NO_GHOST)
			c_put_str(TERM_L_DARK, "Unworldly (one life)", y_rowmode, FIRST_COL);
		else if (p_ptr->mode & MODE_HARD) {
			if (p_ptr->lives == -1) c_put_str(TERM_SLATE, "Hard (extra hard, 3 lives)", y_rowmode, FIRST_COL);
			else c_put_str(TERM_SLATE, format("Hard (extra hard, %d of 3 lives left)", p_ptr->lives), y_rowmode, FIRST_COL);
		} else {/* !(p_ptr->mode & MODE_MASK) aka MODE_NORMAL */
			if (p_ptr->lives == -1) c_put_str(TERM_WHITE, "Normal (3 lives)", y_rowmode, FIRST_COL);
			else c_put_str(TERM_WHITE, format("Normal (%d of 3 lives left)", p_ptr->lives), y_rowmode, FIRST_COL);
		}

		/* Age, Height, Weight, Social */
#define SECOND_COL 33
		put_str(format("Age         :\377B %6d", (int)p_ptr->age), y_row1, SECOND_COL);
		put_str(format("Height      :\377B %6d", (int)p_ptr->ht), y_row1 + 1, SECOND_COL);
		put_str(format("Weight      :\377B %6d", (int)p_ptr->wt), y_row1 + 2, SECOND_COL);
		put_str(format("Social Class:\377B %6d", (int)p_ptr->sc), y_row1 + 3, SECOND_COL);

		/* Display the stats */
		for (i = 0; i < C_ATTRIBUTES; i++) {
			/* Special treatment of "injured" stats */
			if (p_ptr->stat_use[i] < p_ptr->stat_top[i]) {
				int value;

				/* Use lowercase stat name */
				put_str(stat_names_reduced[i], y_row1 + i, 61);

				/* Get the current stat */
				value = p_ptr->stat_use[i];

				/* Obtain the current stat (modified) */
				cnv_stat(value, buf);

				/* Display the current stat (modified) */
				c_put_str(TERM_YELLOW, buf, y_row1 + i, 66);

				/* Acquire the max stat */
				value = p_ptr->stat_top[i];

				/* Obtain the maximum stat (modified) */
				cnv_stat(value, buf);

				/* Display the maximum stat (modified) */
				if (p_ptr->stat_tmp[i]) { /* Boosted? */
					c_put_str(TERM_L_BLUE, buf, y_row1 + i, 73);
				} else {
					if (p_ptr->stat_max[i] < 18 + 100)
						c_put_str(TERM_L_GREEN, buf, y_row1 + i, 73);
					else c_put_str(TERM_L_UMBER, buf, y_row1 + i, 73);
				}
			}
			/* Special treatment of boosted stats */
			else if (p_ptr->stat_tmp[i]) {
				int value;

				/* Use lowercase stat name */
				put_str(stat_names_reduced[i], y_row1 + i, 61);

				/* Get the current stat */
				value = p_ptr->stat_use[i];

				/* Obtain the current stat (modified) */
				cnv_stat(value, buf);

				/* Display the current stat (modified) */
				c_put_str(TERM_L_BLUE, buf, y_row1 + i, 66);

				/* Acquire the max stat */
				value = reduce_stat(p_ptr->stat_top[i], p_ptr->stat_tmp[i]);

				/* Obtain the maximum stat (modified) */
				cnv_stat(value, buf);

				/* Display the unboosted maximum stat (modified) */
				if (reduce_stat(p_ptr->stat_max[i], p_ptr->stat_tmp[i]) < 18 + 100)
					c_put_str(TERM_L_GREEN, buf, y_row1 + i, 73);
				else c_put_str(TERM_L_UMBER, buf, y_row1 + i, 73);
			}
			/* Normal treatment of "normal" stats */
			else {
				/* Assume uppercase stat name */
				put_str(stat_names[i], y_row1 + i, 61);

				/* Obtain the current stat (modified) */
				cnv_stat(p_ptr->stat_use[i], buf);

				/* Display the current stat (modified) */
				if (p_ptr->stat_max[i] < 18 + 100) c_put_str(TERM_L_GREEN, buf, y_row1 + i, 66);
				else c_put_str(TERM_L_UMBER, buf, 1 + i, 66);
			}
		}

		/* Check for history */
		if (hist) {
#ifndef NEW_COMPRESSED_DUMP
			put_str("(Character Background)", y_row3 - 1, 25);
#endif
			for (i = 0; i < 4; i++)
				c_put_str(TERM_SLATE, p_ptr->history[i], y_row3 + i, 10);
		} else {
#ifndef NEW_COMPRESSED_DUMP
			put_str("(Miscellaneous Abilities)", y_row3 - 1, 25);
#endif

			/* Display "skills" */
			put_str("Fighting    :", y_row3, 1);
			desc = likert(p_ptr->skill_thn, 120, 0);
			c_put_str(likert_color, desc, y_row3, 15);

			put_str("Bows/Throw  :", y_row3 + 1, 1);
			desc = likert(p_ptr->skill_thb, 120, 0);
			c_put_str(likert_color, desc, y_row3 + 1, 15);

			put_str("Saving Throw:", y_row3 + 2, 1);
			desc = likert(p_ptr->skill_sav, 52, 95);	/*was 6.0 before x10 increase */
			c_put_str(likert_color, desc, y_row3 + 2, 15);

			put_str("Stealth     :", y_row3 + 3, 1);
			desc = likert(p_ptr->skill_stl, 10, 30);
			c_put_str(likert_color, desc, y_row3 + 3, 15);


			put_str("Perception  :", y_row3, 28);
			desc = likert(p_ptr->skill_fos, 40, 75);
			c_put_str(likert_color, desc, y_row3, 42);

			put_str("Searching   :", y_row3 + 1, 28);
			desc = likert(p_ptr->skill_srh, 60, 100);
			c_put_str(likert_color, desc, y_row3 + 1, 42);

			put_str("Disarming   :", y_row3 + 2, 28);
			desc = likert(p_ptr->skill_dis, 80, 100);
			c_put_str(likert_color, desc, y_row3 + 2, 42);

			put_str("Magic Device:", y_row3 + 3, 28);
			//desc = likert(p_ptr->skill_dev, 60, 0);
			//desc = likert(p_ptr->skill_dev, 80, 0);
			//about 161 is max mdev reachable atm (2014/07/24)
			desc = likert((p_ptr->skill_dev * (p_ptr->skill_dev + 80)) / 110, 100, 0);
			c_put_str(likert_color, desc, y_row3 + 3, 42);


			put_str("Blows/Round:", y_row3, 55);
			c_put_str(p_ptr->extra_blows ? TERM_L_BLUE : TERM_L_GREEN, format("%d", p_ptr->num_blow), y_row3, 69);

			put_str("Shots/Round:", y_row3 + 1, 55);
			c_put_str(TERM_L_GREEN, format("%d", p_ptr->num_fire), y_row3 + 1, 69);
#if 0
			put_str("Spells/Round:", y_row3 + 2, 55);
			c_put_str(TERM_L_GREEN, format("%d", p_ptr->num_spell), y_row3 + 2, 69);
#endif
			put_str("Infra-Vision:", y_row3 + 3, 55);
			c_put_str((p_ptr->tim_infra & 0x2) ? TERM_L_UMBER : ((p_ptr->tim_infra & 0x1) ? TERM_L_BLUE : TERM_L_GREEN),
			    format("%d feet", p_ptr->see_infra * 10), y_row3 + 3, 69);
		}

		/* Dump the boni to hit/dam */
		tmp = (p_ptr->to_h_melee > 5000 ? p_ptr->to_h_melee - 10000 : p_ptr->to_h_melee); tmpc = (p_ptr->to_h_melee > 5000) ? TERM_L_BLUE : TERM_L_GREEN;
		put_str("+To Melee Hit    ", y_row2, 1);
		c_put_str(tmpc, format("%3d", p_ptr->dis_to_h + tmp), y_row2, 19);

		tmp = (p_ptr->to_d_melee > 5000 ? p_ptr->to_d_melee - 10000 : p_ptr->to_d_melee); tmpc = (p_ptr->to_d_melee > 5000) ? TERM_L_BLUE : TERM_L_GREEN;
		put_str("+To Melee Damage ", y_row2 + 1, 1);
		c_put_str(tmpc, format("%3d", p_ptr->dis_to_d + tmp), y_row2 + 1, 19);

		tmp = (p_ptr->to_h_ranged > 5000 ? p_ptr->to_h_ranged - 10000 : p_ptr->to_h_ranged); tmpc = (p_ptr->to_h_ranged > 5000) ? TERM_L_BLUE : TERM_L_GREEN;
		put_str("+To Ranged Hit   ", y_row2 + 2, 1);
		c_put_str(tmpc, format("%3d", p_ptr->dis_to_h + tmp), y_row2 + 2, 19);

		tmp = (p_ptr->to_d_ranged > 5000 ? p_ptr->to_d_ranged - 10000 : p_ptr->to_d_ranged); tmpc = (p_ptr->to_d_ranged > 5000) ? TERM_L_BLUE : TERM_L_GREEN;
		put_str("+To Ranged Damage", y_row2 + 3, 1);
		c_put_str(tmpc, format("%3d", tmp), y_row2 + 3, 19); //generic +dam never affects ranged, so no +dis_to_d

		/* Armour Class */
		if (is_atleast(&server_version, 4, 7, 3, 1, 0, 0)) {
			tmp = (p_ptr->dis_ac > 5000 ? p_ptr->dis_ac - 10000 : p_ptr->dis_ac); tmpc = (p_ptr->dis_ac > 5000) ? TERM_L_BLUE : TERM_L_GREEN;
		} else if (is_atleast(&server_version, 4, 7, 3, 0, 0, 0)) {
			tmp = (p_ptr->dis_ac & ~0x1000); tmpc = (p_ptr->dis_ac & 0x1000) ? TERM_L_BLUE : TERM_L_GREEN;
		} else {
			tmp = p_ptr->dis_ac; tmpc = TERM_L_GREEN;
		}
#ifndef NEW_COMPRESSED_DUMP_AC
		/* Dump the armor class bonus */
		prt_num("+ To AC     ", p_ptr->dis_to_a, y_row2 + 4, 1, tmpc);
		/* Dump the total armor class */
		prt_num("  Base AC   ", tmp, y_row2 + 5, 1, TERM_L_BLUE);
#else
		prt_num("Total AC    ", tmp + p_ptr->dis_to_a, y_row2 + 4, 1, tmpc);
#endif

#define SECOND2_COL 26
		prt_num("Level        ", (int)p_ptr->lev, y_row2, SECOND2_COL, p_ptr->lev < PY_MAX_PLAYER_LEVEL ? TERM_L_GREEN : (p_ptr->lev < PY_MAX_LEVEL ? TERM_L_UMBER : TERM_BLUE));
		if (p_ptr->exp >= p_ptr->max_exp)
			prt_lnum("Experience ", p_ptr->exp, y_row2 + 1, SECOND2_COL, p_ptr->exp < PY_MAX_EXP ? TERM_L_GREEN : TERM_L_UMBER);
		else
			prt_lnum("Experience ", p_ptr->exp, y_row2 + 1, SECOND2_COL, TERM_YELLOW);
		prt_lnum("Max Exp    ", p_ptr->max_exp, y_row2 + 2, SECOND2_COL, p_ptr->max_exp < PY_MAX_EXP ? TERM_L_GREEN : TERM_L_UMBER);
		if (p_ptr->lev >= PY_MAX_PLAYER_LEVEL || !exp_adv) {
			put_str("Exp to Adv.", y_row2 + 3, SECOND2_COL);
			c_put_str(TERM_L_UMBER, "        ***", y_row2 + 3, SECOND2_COL + 11);
		} else {
			prt_lnum("Exp to Adv.", exp_adv, y_row2 + 3, SECOND2_COL, TERM_L_GREEN);
		}
		prt_lnum("Gold (Au)  ", p_ptr->au, y_row2 + 4, SECOND2_COL, p_ptr->au < PY_MAX_GOLD ? TERM_L_GREEN : TERM_L_UMBER);
#ifndef NEW_COMPRESSED_DUMP_AC
 #ifdef HIDE_UNAVAILABLE_TRAIT
		if (trait != 0)
 #endif
		{
			put_str("Trait", y_row2 + 5, SECOND2_COL);
			c_put_str(TERM_L_BLUE, trait_info[trait].title, y_row2 + 5, 39);
		}
#endif

#ifndef NEW_COMPRESSED_DUMP
		if (p_ptr->mhp == -9999) {
			put_str("Max Hit Points         ", y_row2, 52);
			c_put_str(TERM_L_GREEN, "-", y_row2, 75);
		} else
			prt_num("Max Hit Points ", p_ptr->mhp, y_row2, 52, TERM_L_GREEN);

		if (p_ptr->chp == -9999) {
			put_str("Cur Hit Points         ", y_row2 + 1, 52);
			c_put_str(TERM_L_GREEN, "-", y_row2 + 1, 75);
		} else if (p_ptr->chp >= p_ptr->mhp)
			prt_num("Cur Hit Points ", p_ptr->chp, y_row2 + 1, 52, TERM_L_GREEN);
		else if (p_ptr->chp > (p_ptr->mhp) / 10)
			prt_num("Cur Hit Points ", p_ptr->chp, y_row2 + 1, 52, TERM_YELLOW);
		else
			prt_num("Cur Hit Points ", p_ptr->chp, y_row2 + 1, 52, TERM_RED);

		if (p_ptr->mmp == -9999) {
			put_str("Max MP (Mana)          ", y_row2 + 2, 52);
			c_put_str(TERM_L_GREEN, "-", y_row2 + 2, 75);
		} else
			prt_num("Max MP (Mana)  ", p_ptr->mmp, y_row2 + 2, 52, TERM_L_GREEN);

		if (p_ptr->cmp == -9999) {
			put_str("Cur MP (Mana)          ", y_row2 + 3, 52);
			c_put_str(TERM_L_GREEN, "-", y_row2 + 3, 75);
		} else if (p_ptr->cmp >= p_ptr->mmp)
			prt_num("Cur MP (Mana)  ", p_ptr->cmp, y_row2 + 3, 52, TERM_L_GREEN);
		else if (p_ptr->cmp > (p_ptr->mmp) / 10)
			prt_num("Cur MP (Mana)  ", p_ptr->cmp, y_row2 + 3, 52, TERM_YELLOW);
		else
			prt_num("Cur MP (Mana)  ", p_ptr->cmp, y_row2 + 3, 52, TERM_RED);
 #ifdef SHOW_SANITY
		put_str("Cur Sanity", y_row2 + 4, 52);

		c_put_str(c_p_ptr->sanity_attr, c_p_ptr->sanity, y_row2 + 4, 67);
 #endif	/* SHOW_SANITY */

#else

		if (p_ptr->mhp == -9999) {
			put_str("Hit Points", y_row2, 52);
			c_put_str(TERM_L_GREEN, "-", y_row2, 71);
		} else {
			prt_num("Hit Points     ", p_ptr->mhp, y_row2, 52, TERM_L_GREEN);
			    c_put_str(TERM_L_GREEN, "/", y_row2, 71);
			if (p_ptr->chp >= p_ptr->mhp)
				prt_num("", p_ptr->chp, y_row2, 62, TERM_L_GREEN);
			else if (p_ptr->chp > (p_ptr->mhp) / 10)
				prt_num("", p_ptr->chp, y_row2, 62, TERM_YELLOW);
			else
				prt_num("", p_ptr->chp, y_row2, 62, TERM_RED);
		}

		if (p_ptr->mmp == -9999) {
			put_str("MP (Mana)", y_row2 + 1, 52);
			c_put_str(TERM_L_GREEN, "-", y_row2 + 1, 71);
		} else {
			prt_num("MP (Mana)      ", p_ptr->mmp, y_row2 + 1, 52, TERM_L_GREEN);
			c_put_str(TERM_L_GREEN, "/", y_row2 + 1, 71);
			if (p_ptr->cmp >= p_ptr->mmp)
				prt_num("", p_ptr->cmp, y_row2 + 1, 62, TERM_L_GREEN);
			else if (p_ptr->cmp > (p_ptr->mmp) / 10)
				prt_num("", p_ptr->cmp, y_row2 + 1, 62, TERM_YELLOW);
			else
				prt_num("", p_ptr->cmp, y_row2 + 1, 62, TERM_RED);
		}

		if (p_ptr->mst == -9999) {
			put_str("Stamina", y_row2 + 2, 52);
			c_put_str(TERM_L_GREEN, "-", y_row2 + 2, 71);
		} else {
			prt_num("Stamina        ", p_ptr->mst, y_row2 + 2, 52, TERM_L_GREEN);
			c_put_str(TERM_L_GREEN, "/", y_row2 + 2, 71);
			if (p_ptr->cst >= p_ptr->mst)
				prt_num("", p_ptr->cst, y_row2 + 2, 62, TERM_L_GREEN);
			else if (p_ptr->cst > (p_ptr->mst) / 10)
				prt_num("", p_ptr->cst, y_row2 + 2, 62, TERM_YELLOW);
			else
				prt_num("", p_ptr->cst, y_row2 + 2, 62, TERM_RED);
		}

 #ifdef SHOW_SANITY
		put_str("Sanity", y_row2 + 3, 52);
		c_put_str(c_p_ptr->sanity_attr, c_p_ptr->sanity, y_row2 + 3, 67);
 #endif	/* SHOW_SANITY */

		/* Display 'WINNER' status */
		put_str("Status", y_row2 + 4, 52);
		if (p_ptr->chp < 0 || c_p_ptr->sanity_attr == TERM_RED) c_put_str(TERM_L_DARK, "        DEAD", y_row2 + 4, 64);
		else if (p_ptr->ghost) c_put_str(TERM_RED, "Ghost (dead)", y_row2 + 4, 64);
		else if (p_ptr->total_winner) c_put_str(TERM_VIOLET, "***WINNER***", y_row2 + 4, 64);
		else c_put_str(TERM_L_GREEN, "       Alive", y_row2 + 4, 64);
#endif

		/* Show location (better description needed XXX) */
		if (c_cfg.depth_in_feet) {
			if (location_name2[0])
				put_str(format("You are at %s%d ft%s %s %s.",
					p_ptr->wpos.wz ? "" : "surface (", p_ptr->wpos.wz * 50,
					p_ptr->wpos.wz ? "" : ")",
					location_pre, location_name2), 20, 10);//hist ? 10 : 1);
			else if (p_ptr->wpos.wx == 127)
				put_str(format("You are at %s%d ft%s of an unknown region.",
					p_ptr->wpos.wz ? "" : "surface (", p_ptr->wpos.wz * 50,
					p_ptr->wpos.wz ? "" : ")"), 20, 10);//hist ? 10 : 1);
			else
				put_str(format("You are at %s%d ft%s of world map sector %d,%d.",
					p_ptr->wpos.wz ? "" : "surface (", p_ptr->wpos.wz * 50,
					p_ptr->wpos.wz ? "" : ")",
					p_ptr->wpos.wx, p_ptr->wpos.wy), 20, 10);//hist ? 10 : 1);
		} else {
			if (location_name2[0])
				put_str(format("You are at %slevel %d%s %s %s.",
					p_ptr->wpos.wz ? "" : "surface (", p_ptr->wpos.wz,
					p_ptr->wpos.wz ? "" : ")",
					location_pre, location_name2), 20, 10);//hist ? 10 : 1);
			else if (p_ptr->wpos.wx == 127)
				put_str(format("You are at %slevel %d%s of an unknown region.",
					p_ptr->wpos.wz ? "" : "surface (", p_ptr->wpos.wz,
					p_ptr->wpos.wz ? "" : ")"), 20, 10);//hist ? 10 : 1);
			else
				put_str(format("You are at %slevel %d%s of world map sector %d,%d.",
					p_ptr->wpos.wz ? "" : "surface (", p_ptr->wpos.wz,
					p_ptr->wpos.wz ? "" : ")",
					p_ptr->wpos.wx, p_ptr->wpos.wy), 20, 10);//hist ? 10 : 1);
		}
	} else { //Character sheet boni page, finally! (By Kurzel)
		int i, j, k, amfi1 = 0, amfi2 = 0;

		int color;
		char32_t symbol;
		char tmp[10];
		byte amfi_sum = 0, luck_sum = 0, life_sum = 0;

		/* Clear screen */
		clear_from(0);

		/* Normal view (vertical columns of different stats) */
		if (!csheet_horiz) {
			int header_color[4][19], row;

			for (i = 0; i < 4; i++)
				for (j = 0; j < 19; j++)
					header_color[i][j] = TERM_L_DARK;

			/* Fill Columns */
			for (i = 0; i < 15; i++) {
				/* Header */
				color = TERM_WHITE;
				if (csheet_boni[i].cb[11] & CB12_XHIDD) color = TERM_YELLOW;
				if (csheet_boni[i].cb[12] & CB13_XSTAR) color = TERM_L_UMBER;
				if (csheet_boni[i].cb[12] & CB13_XCRSE) color = TERM_RED;
				if (csheet_boni[i].cb[12] & CB13_XNOEQ) color = TERM_L_DARK;
				if (i == 14) color = (p_ptr->ghost) ? TERM_L_DARK : (p_ptr->fruit_bat == 1) ? TERM_ORANGE : TERM_WHITE;
				for (j = 0; j < 4; j++) {
					switch (i) {
					case 0: { c_put_str(color, "a", 0, 5 + j * 20 + i); break; }
					case 1: { c_put_str(color, "b", 0, 5 + j * 20 + i); break; }
					case 2: { c_put_str(color, "c", 0, 5 + j * 20 + i); break; }
					case 3: { c_put_str(color, "d", 0, 5 + j * 20 + i); break; }
					case 4: { c_put_str(color, "e", 0, 5 + j * 20 + i); break; }
					case 5: { c_put_str(color, "f", 0, 5 + j * 20 + i); break; }
					case 6: { c_put_str(color, "g", 0, 5 + j * 20 + i); break; }
					case 7: { c_put_str(color, "h", 0, 5 + j * 20 + i); break; }
					case 8: { c_put_str(color, "i", 0, 5 + j * 20 + i); break; }
					case 9: { c_put_str(color, "j", 0, 5 + j * 20 + i); break; }
					case 10: { c_put_str(color, "k", 0, 5 + j * 20 + i); break; }
					case 11: { c_put_str(color, "l", 0, 5 + j * 20 + i); break; }
					case 12: { c_put_str(color, "m", 0, 5 + j * 20 + i); break; }
					case 13: { c_put_str(color, "n", 0, 5 + j * 20 + i); break; }
					case 14: { c_put_str(color, (p_ptr->fruit_bat == 1) ? "b" : "@", 0, 5 + j * 20 + i); break; }
					}
				}

				/* Blank Fill */
				color = TERM_SLATE;
				if (csheet_boni[i].cb[12] & CB13_XNOEQ) color = TERM_L_DARK;
				if (csheet_boni[i].cb[11] & CB12_XHIDD) color = TERM_YELLOW;
				for (j = 0; j < 4; j++) {
					for (k = 1; k < 20; k++)
						c_put_str(color, ".", k, 5 + j * 20 + i);
				}
				if (csheet_boni[i].cb[12] & CB13_XNOEQ) continue;

				/* Fill with boni values and track header colour. Gross hardcode! - Kurzel */
				if (csheet_boni[i].cb[0] & CB1_SFIRE) { c_put_str(TERM_RED, "-", 1, 5 + i); if (header_color[0][0] == TERM_L_DARK) header_color[0][0] = TERM_RED; }
				if (csheet_boni[i].cb[0] & CB1_RFIRE) { c_put_str(TERM_WHITE, "+", 1, 5 + i); if (header_color[0][0] == TERM_L_DARK || header_color[0][0] == TERM_RED) header_color[0][0] = TERM_WHITE; }
				if (csheet_boni[i].cb[0] & CB1_IFIRE) { c_put_str(TERM_L_UMBER, "*", 1, 5 + i); header_color[0][0] = TERM_L_UMBER; }
				if (csheet_boni[i].cb[0] & CB1_SCOLD) { c_put_str(TERM_RED, "-", 2, 5 + i); if (header_color[0][1] == TERM_L_DARK) header_color[0][1] = TERM_RED; }
				if (csheet_boni[i].cb[0] & CB1_RCOLD) { c_put_str(TERM_WHITE, "+", 2, 5 + i); if (header_color[0][1] == TERM_L_DARK || header_color[0][1] == TERM_RED) header_color[0][1] = TERM_WHITE; }
				if (csheet_boni[i].cb[0] & CB1_ICOLD) { c_put_str(TERM_L_UMBER, "*", 2, 5 + i); header_color[0][1] = TERM_L_UMBER; }
				if (csheet_boni[i].cb[0] & CB1_SELEC) { c_put_str(TERM_RED, "-", 3, 5 + i); if (header_color[0][2] == TERM_L_DARK) header_color[0][2] = TERM_RED; }
				if (csheet_boni[i].cb[0] & CB1_RELEC) { c_put_str(TERM_WHITE, "+", 3, 5 + i); if (header_color[0][2] == TERM_L_DARK || header_color[0][2] == TERM_RED) header_color[0][2] = TERM_WHITE; }
				if (csheet_boni[i].cb[1] & CB2_IELEC) { c_put_str(TERM_L_UMBER, "*", 3, 5 + i); header_color[0][2] = TERM_L_UMBER; }
				if (csheet_boni[i].cb[1] & CB2_SACID) { c_put_str(TERM_RED, "-", 4, 5 + i); if (header_color[0][3] == TERM_L_DARK) header_color[0][3] = TERM_RED; }
				if (csheet_boni[i].cb[1] & CB2_RACID) { c_put_str(TERM_WHITE, "+", 4, 5 + i); if (header_color[0][3] == TERM_L_DARK || header_color[0][3] == TERM_RED) header_color[0][3] = TERM_WHITE; }
				if (csheet_boni[i].cb[1] & CB2_IACID) { c_put_str(TERM_L_UMBER, "*", 4, 5 + i); header_color[0][3] = TERM_L_UMBER; }
				if (csheet_boni[i].cb[1] & CB2_SPOIS) { c_put_str(TERM_RED, "-", 5, 5 + i); if (header_color[0][4] == TERM_L_DARK) header_color[0][4] = TERM_RED; }
				if (csheet_boni[i].cb[1] & CB2_RPOIS) { c_put_str(TERM_WHITE, "+", 5, 5 + i); if (header_color[0][4] == TERM_L_DARK || header_color[0][4] == TERM_RED) header_color[0][4] = TERM_WHITE; }
				if (csheet_boni[i].cb[1] & CB2_IPOIS) { c_put_str(TERM_L_UMBER, "*", 5, 5 + i); header_color[0][4] = TERM_L_UMBER; }
				if (csheet_boni[i].cb[1] & CB2_RBLND) { c_put_str(TERM_WHITE, "+", 6, 5 + i); header_color[0][5] = TERM_WHITE; }
				if (csheet_boni[i].cb[2] & CB3_SLITE) { c_put_str(TERM_RED, "-", 7, 5 + i); if (header_color[0][6] == TERM_L_DARK) header_color[0][6] = TERM_RED; }
				if (csheet_boni[i].cb[2] & CB3_RLITE) { c_put_str(TERM_WHITE, "+", 7, 5 + i); if (header_color[0][6] == TERM_L_DARK || header_color[0][6] == TERM_RED) header_color[0][6] = TERM_WHITE; }
				if (csheet_boni[i].cb[2] & CB3_RDARK) { c_put_str(TERM_WHITE, "+", 8, 5 + i); if (header_color[0][7] == TERM_L_DARK) header_color[0][7] = TERM_WHITE; }
				if (csheet_boni[i].cb[2] & CB3_RSOUN) { c_put_str(TERM_WHITE, "+", 9, 5 + i); header_color[0][8] = TERM_WHITE; }
				if (csheet_boni[i].cb[12] & CB13_XNCUT) c_put_str(TERM_WHITE, "s", 10, 5 + i);
				if (csheet_boni[i].cb[2] & CB3_RSHRD) { c_put_str(TERM_WHITE, "+", 10, 5 + i); header_color[0][9] = TERM_WHITE; }
				if ((csheet_boni[i].cb[2] & CB3_RSHRD) && (csheet_boni[i].cb[12] & CB13_XNCUT)) c_put_str(TERM_WHITE, "*", 10, 5 + i);
				if (csheet_boni[i].cb[2] & CB3_RNEXU) { c_put_str(TERM_WHITE, "+", 11, 5 + i); header_color[0][10] = TERM_WHITE; }
				if (csheet_boni[i].cb[2] & CB3_RNETH) { c_put_str(TERM_WHITE, "+", 12, 5 + i); if (header_color[0][11] == TERM_L_DARK) header_color[0][11] = TERM_WHITE; }
				if (csheet_boni[i].cb[2] & CB3_INETH) { c_put_str(TERM_L_UMBER, "*", 12, 5 + i); header_color[0][11] = TERM_L_UMBER; }
				if (csheet_boni[i].cb[3] & CB4_RCONF) { c_put_str(TERM_WHITE, "+", 13, 5 + i); if (header_color[0][12] == TERM_L_DARK || header_color[0][12] == TERM_YELLOW) header_color[0][12] = TERM_WHITE; }
				if (csheet_boni[i].cb[3] & CB4_RCHAO) {
					c_put_str(TERM_WHITE, "+", 14, 5 + i); header_color[0][13] = TERM_WHITE;
					//Chaos covers confusion for whatever reason, maybe confusion breathers should breathe psi damage instead, and confusion should become non-damaging? - Kurzel
					//Could replace br_conf with a psi attack for the affected mobs, breathing psi doesn't make any sense either, some mobs could be removed even. - Kurzel
					//Bronze D / draconians could get gravity/lightning to be consistent with other metallic dragon abilities/lore? Same colour as gravity Z now anyway. - Kurzel
					if (!(csheet_boni[i].cb[3] & CB4_RCONF)) { c_put_str(TERM_YELLOW, "+", 13, 5 + i); if (header_color[0][12] == TERM_L_DARK) header_color[0][12] = TERM_YELLOW; }
				}
				if (csheet_boni[i].cb[3] & CB4_RDISE) { c_put_str(TERM_WHITE, "+", 15, 5 + i); header_color[0][14] = TERM_WHITE; }
				if (csheet_boni[i].cb[3] & CB4_RWATR) { c_put_str(TERM_WHITE, "+", 16, 5 + i); if (header_color[0][15] == TERM_L_DARK) header_color[0][15] = TERM_WHITE; }
				if (csheet_boni[i].cb[3] & CB4_IWATR) { c_put_str(TERM_L_UMBER, "*", 16, 5 + i); header_color[0][15] = TERM_L_UMBER; }
				if (csheet_boni[i].cb[3] & CB4_RTIME) { c_put_str(TERM_WHITE, "+", 17, 5 + i); header_color[0][16] = TERM_WHITE; }
				if (csheet_boni[i].cb[3] & CB4_RMANA) { c_put_str(TERM_WHITE, "+", 18, 5 + i); header_color[0][17] = TERM_WHITE; }
				if (csheet_boni[i].cb[13] & CB14_UMIND) { c_put_str(TERM_WHITE, "*", 19, 5 + i); header_color[0][18] = TERM_WHITE; } //I added a 'level 3' - C. Blue
				else if (csheet_boni[i].cb[4] & CB5_XMIND) { c_put_str(TERM_WHITE, "+", 19, 5 + i); header_color[0][18] = TERM_WHITE; } //Similar to no_cut, for now? Level 2~ - Kurzel
				else if (csheet_boni[i].cb[3] & CB4_RMIND) { c_put_str(TERM_WHITE, "~", 19, 5 + i); header_color[0][18] = TERM_WHITE; }

				if (csheet_boni[i].cb[4] & CB5_RFEAR) { c_put_str(TERM_WHITE, "+", 1, 25 + i); header_color[1][0] = TERM_WHITE; }
				if (csheet_boni[i].cb[4] & CB5_RPARA) { c_put_str(TERM_WHITE, "+", 2, 25 + i); header_color[1][1] = TERM_WHITE; }
				if (csheet_boni[i].cb[5] & CB6_RLIFE) { c_put_str(TERM_WHITE, "+", 3, 25 + i); header_color[1][2] = TERM_WHITE; }
				if (csheet_boni[i].cb[13] & CB14_ILIFE) { c_put_str(TERM_L_UMBER, "*", 3, 25 + i); header_color[1][2] = TERM_L_UMBER; }
				if (csheet_boni[i].cb[4] & CB5_STELE) { c_put_str(TERM_RED, "t", 4, 25 + i); if (header_color[1][3] == TERM_L_DARK || header_color[1][3] == TERM_WHITE) header_color[1][3] = TERM_RED; } //auto-tele; overrides the resist, no items with both
				if (csheet_boni[i].cb[4] & CB5_RTELE) { c_put_str(TERM_WHITE, "+", 4, 25 + i); if (header_color[1][3] == TERM_L_DARK) header_color[1][3] = TERM_WHITE; }
				if (csheet_boni[i].cb[4] & CB5_ITELE) { c_put_str(TERM_L_RED, "N", 4, 25 + i); if (header_color[1][3] != TERM_L_RED) header_color[1][3] = TERM_L_RED; } //NO_TELE
				if (csheet_boni[i].cb[5] & CB6_RFFAL) { c_put_str(TERM_WHITE, "+", 5, 25 + i); header_color[1][4] = TERM_WHITE; }
				if (csheet_boni[i].cb[12] & CB13_XSWIM) { c_put_str(TERM_L_BLUE, "~", 6, 25 + i); if (header_color[1][5] == TERM_L_DARK) header_color[1][5] = TERM_YELLOW; }
				if (csheet_boni[i].cb[12] & CB13_XTREE) { c_put_str(TERM_GREEN, "#", 6, 25 + i); if (header_color[1][5] == TERM_L_DARK) header_color[1][5] = TERM_YELLOW; }
				if ((csheet_boni[i].cb[12] & CB13_XTREE) && (csheet_boni[i].cb[12] & CB13_XSWIM)) c_put_str(TERM_YELLOW, "+", 6, 25 + i); //Almost flying~
				if (csheet_boni[i].cb[5] & CB6_RLVTN) {
					c_put_str(TERM_WHITE, "+", 6, 25 + i); header_color[1][5] = TERM_WHITE;
					//Levitation covers feather falling, similar to chaos/confusion but plausible? - Kurzel
					if (!(csheet_boni[i].cb[5] & CB6_RFFAL)) { c_put_str(TERM_YELLOW, "+", 5, 25 + i); if (header_color[1][4] == TERM_L_DARK) header_color[1][4] = TERM_YELLOW; }
				}
				if (csheet_boni[i].cb[5] & CB6_RCLMB) { c_put_str(TERM_WHITE, "+", 7, 25 + i); header_color[1][6] = TERM_WHITE; }
				if (csheet_boni[i].cb[4] & CB5_RSINV) { c_put_str(TERM_WHITE, "+", 8, 25 + i); if (header_color[1][7] == TERM_L_DARK) header_color[1][7] = TERM_WHITE; }
				if (csheet_boni[i].cb[4] & CB5_RINVS) { c_put_str(TERM_WHITE, "+", 9, 25 + i); header_color[1][8] = TERM_WHITE; }
				if (csheet_boni[i].cb[6] & CB7_RREFL) { c_put_str(TERM_WHITE, "+", 10, 25 + i); header_color[1][9] = TERM_WHITE; }
				if (csheet_boni[i].cb[5] & CB6_RWRTH) { c_put_str(TERM_WHITE, "+", 11, 25 + i); header_color[1][10] = TERM_WHITE; }
				if (csheet_boni[i].cb[5] & CB6_SRGHP) { c_put_str(TERM_RED, "-", 12, 25 + i); if (header_color[1][11] == TERM_L_DARK) header_color[1][11] = TERM_RED; }
				if (csheet_boni[i].cb[5] & CB6_RRGHP) { c_put_str(TERM_WHITE, "+", 12, 25 + i); if (header_color[1][11] == TERM_L_DARK) header_color[1][11] = TERM_WHITE; if (header_color[1][11] == TERM_RED || header_color[1][11] == TERM_YELLOW) header_color[1][11] = TERM_YELLOW; } //yellow if both...
				if (csheet_boni[i].cb[5] & CB6_SRGMP) { c_put_str(TERM_RED, "-", 13, 25 + i); if (header_color[1][12] == TERM_L_DARK) header_color[1][12] = TERM_RED; }
				if (csheet_boni[i].cb[6] & CB7_RRGMP) { c_put_str(TERM_WHITE, "+", 13, 25 + i); if (header_color[1][12] == TERM_L_DARK) header_color[1][12] = TERM_WHITE; if (header_color[1][12] == TERM_RED || header_color[1][12] == TERM_YELLOW) header_color[1][12] = TERM_YELLOW; } //yellow if both...
				if (csheet_boni[i].cb[6] & CB7_RFOOD) { c_put_str(TERM_WHITE, "+", 14, 25 + i); if (header_color[1][13] == TERM_L_DARK) header_color[1][13] = TERM_WHITE; }
				if (p_ptr->prace == RACE_ENT || p_ptr->prace == RACE_VAMPIRE) { c_put_str(TERM_WHITE, "*", 13, 25 + 14); if (header_color[1][13] == TERM_L_DARK) header_color[1][13] = TERM_WHITE; } //Hack: Ents/Vamps require food but do not gorge! - Kurzel
				if (csheet_boni[i].cb[6] & CB7_IFOOD) { c_put_str(TERM_L_UMBER, "*", 14, 25 + i); header_color[1][13] = TERM_L_UMBER; }
				if (csheet_boni[i].cb[6] & CB7_RVAMP) {
					if ((i == 0) || (i == 1) || (i == 2) || (i == 12)
					    || ((i == 14) && (!strcasecmp(c_p_ptr->body_name, "Vampiric mist") || !strcasecmp(c_p_ptr->body_name, "Vampire bat"))) //Hack: use * for 100% weapon/ammo or v-bat/mist forms
					    || (i == 14 && race == RACE_VAMPIRE && get_skill(SKILL_NECROMANCY) == 50 && get_skill(SKILL_TRAUMATURGY) == 50)) { //Nasty hack: Assume that having full trauma+necro gives 100% vamp actually
						c_put_str(TERM_WHITE, "*", 15, 25 + i); header_color[1][14] = TERM_WHITE;
					} else { c_put_str(TERM_WHITE, "+", 15, 25 + i); header_color[1][14] = TERM_WHITE; }
				}
				if (p_ptr->fruit_bat == 1 && !strcasecmp(c_p_ptr->body_name, "Player")) { c_put_str(TERM_WHITE, "+", 15, 25 + 14); header_color[1][14] = TERM_WHITE; } //Mega Hack: Hardcode 50% vamp as a fruit bat! Maybe incorrect for mimics? - Kurzel
				if (csheet_boni[i].cb[6] & CB7_RAUID) { c_put_str(TERM_WHITE, "+", 16, 25 + i); header_color[1][15] = TERM_WHITE; }
				if (csheet_boni[i].cb[6] & CB7_RAMSH) { c_put_str(TERM_YELLOW, "+", 17, 25 + i); header_color[1][16] = TERM_YELLOW; }
				// AMFI (Numerical) goes here at [1][17] - Kurzel
				if (csheet_boni[i].cb[6] & CB7_RAGGR) { c_put_str(TERM_RED, "+", 19, 25 + i); header_color[1][18] = TERM_RED; }

				/* Numerical Boni (WHITE if affected at, GOLD if sustained or maxxed (important), at a glance - Kurzel */
				if (csheet_boni[i].spd != 0) {
					header_color[2][0] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].spd > 9) color = TERM_GREEN;
					if (csheet_boni[i].spd > 19) color = TERM_L_UMBER; //high form/skills
					if (csheet_boni[i].spd < 0) color = TERM_L_RED;
					if (csheet_boni[i].spd < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].spd) % 10);
					if (csheet_boni[i].spd > 29) sprintf(tmp, "*");
					c_put_str(color, tmp, 1, 45 + i);
				}
				if (csheet_boni[i].slth != 0) {
					header_color[2][1] = TERM_WHITE;
					if (p_ptr->skill_stl >= 30) header_color[2][1] = TERM_L_UMBER;
					color = TERM_L_GREEN;
					if (csheet_boni[i].slth > 9) color = TERM_GREEN;
					if (csheet_boni[i].slth > 19) color = TERM_L_UMBER; //admin items
					if (csheet_boni[i].slth < 0) color = TERM_L_RED;
					if (csheet_boni[i].slth < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].slth) % 10);
					if (csheet_boni[i].slth > 29) sprintf(tmp, "*");
					c_put_str(color, tmp, 2, 45 + i);
				}
				if (csheet_boni[i].srch != 0) {
					header_color[2][2] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].srch > 9) color = TERM_GREEN;
					if (csheet_boni[i].srch > 19) color = TERM_L_UMBER; //admin items
					if (csheet_boni[i].srch < 0) color = TERM_L_RED;
					if (csheet_boni[i].srch < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].srch) % 10);
					if (csheet_boni[i].srch > 29) sprintf(tmp, "*");
					c_put_str(color, tmp, 3, 45 + i);
				}
				if (csheet_boni[i].infr != 0) {
					header_color[2][3] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].infr > 9) color = TERM_GREEN;
					if (csheet_boni[i].infr > 19) color = TERM_L_UMBER; //admin items
					if (csheet_boni[i].infr < 0) color = TERM_L_RED;
					if (csheet_boni[i].infr < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].infr) % 10);
					if (csheet_boni[i].infr > 29) sprintf(tmp, "*");
					c_put_str(color, tmp, 4, 45 + i);
				}
				if (csheet_boni[i].lite != 0) {
					header_color[2][4] = TERM_WHITE;
					color = TERM_WHITE;
					if (csheet_boni[i].cb[12] & CB13_XLITE) { color = TERM_L_UMBER; header_color[2][4] = TERM_L_UMBER; }
					sprintf(tmp, "%d", abs(csheet_boni[i].lite) % 10);
					if (csheet_boni[i].lite > 9) sprintf(tmp, "*");
					c_put_str(color, tmp, 5, 45 + i);
				}
				if (csheet_boni[i].dig != 0) {
					header_color[2][5] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].dig > 9) color = TERM_GREEN;
					if (csheet_boni[i].dig > 19) color = TERM_L_UMBER; //admin items
					if (csheet_boni[i].dig < 0) color = TERM_L_RED;
					if (csheet_boni[i].dig < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].dig) % 10);
					if (csheet_boni[i].dig > 29 || (csheet_boni[i].cb[12] & CB13_XWALL)) sprintf(tmp, "*");
					if (csheet_boni[i].cb[12] & CB13_XWALL) color = TERM_L_UMBER;
					c_put_str(color, tmp, 6, 45 + i);
				}
				if (csheet_boni[i].blow != 0) {
					header_color[2][6] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].blow > 9) color = TERM_GREEN;
					if (csheet_boni[i].blow < 0) color = TERM_L_RED;
					if (csheet_boni[i].blow < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].blow) % 10);
					if (csheet_boni[i].blow > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 7, 45 + i);
				}
				if (csheet_boni[i].crit != 0) {
					header_color[2][7] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].crit > 9) color = TERM_GREEN;
					if (csheet_boni[i].crit < 0) color = TERM_L_RED;
					if (csheet_boni[i].crit < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].crit) % 10);
					if (csheet_boni[i].crit > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 8, 45 + i);
				}
				if (csheet_boni[i].shot != 0) {
					header_color[2][8] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].shot > 9) color = TERM_GREEN;
					if (csheet_boni[i].shot < 0) color = TERM_L_RED;
					if (csheet_boni[i].shot < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].shot) % 10);
					if (csheet_boni[i].shot > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 9, 45 + i);
				}
				if (csheet_boni[i].migh != 0) {
					header_color[2][9] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].migh > 9) color = TERM_GREEN;
					if (csheet_boni[i].migh < 0) color = TERM_L_RED;
					if (csheet_boni[i].migh < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].migh) % 10);
					if (csheet_boni[i].migh > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 10, 45 + i);
				}
				if (csheet_boni[i].mxhp != 0) {
					header_color[2][10] = TERM_WHITE;
					life_sum += csheet_boni[i].mxhp;
					if (life_sum >= 3) header_color[2][10] = TERM_L_UMBER; //max
					color = TERM_L_GREEN;
					if (csheet_boni[i].mxhp >= 3) color = TERM_L_UMBER;
					else if (csheet_boni[i].mxhp < 0) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].mxhp) % 10);
					if (csheet_boni[i].mxhp > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 11, 45 + i);
				}
				if (csheet_boni[i].mxmp != 0) {
					header_color[2][11] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].mxmp > 9) color = TERM_GREEN;
					if (csheet_boni[i].mxmp < 0) color = TERM_L_RED;
					if (csheet_boni[i].mxmp < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].mxmp) % 10);
					if (csheet_boni[i].mxmp > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 12, 45 + i);
				}
				if (csheet_boni[i].luck != 0) {
					luck_sum += csheet_boni[i].luck;
					header_color[2][12] = TERM_WHITE;
					if (luck_sum >= 40) header_color[2][12] = TERM_L_UMBER; //max
					color = TERM_L_GREEN;
					if (csheet_boni[i].luck > 9) color = TERM_GREEN;
					if (csheet_boni[i].luck > 19) color = TERM_L_UMBER; //high luck sets
					if (csheet_boni[i].luck < 0) color = TERM_L_RED;
					if (csheet_boni[i].luck < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].luck) % 10);
					if (csheet_boni[i].mxmp > 29) sprintf(tmp, "*");
					c_put_str(color, tmp, 13, 45 + i);
				}
				if (csheet_boni[i].pstr != 0) {
					if (header_color[2][13] == TERM_L_DARK) header_color[2][13] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].cb[11] & CB12_RSSTR) { color = TERM_L_UMBER; header_color[2][13] = TERM_L_UMBER; }
					if (csheet_boni[i].pstr > 9) { color = TERM_GREEN; if (csheet_boni[i].cb[11] & CB12_RSSTR) color = TERM_UMBER; }
					if (csheet_boni[i].pstr < 0) { color = TERM_L_RED; if (csheet_boni[i].cb[11] & CB12_RSSTR) color = TERM_YELLOW; }
					if (csheet_boni[i].pstr < -9) { color = TERM_RED; if (csheet_boni[i].cb[11] & CB12_RSSTR) color = TERM_ORANGE; }
					sprintf(tmp, "%d", abs(csheet_boni[i].pstr) % 10);
					if (csheet_boni[i].pstr > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 14, 45 + i);
				} else if (csheet_boni[i].cb[11] & CB12_RSSTR) { c_put_str(TERM_L_UMBER, "s", 14, 45 + i); header_color[2][13] = TERM_L_UMBER; }
				if (csheet_boni[i].pint != 0) {
					if (header_color[2][14] == TERM_L_DARK) header_color[2][14] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].cb[11] & CB12_RSINT) { color = TERM_L_UMBER; header_color[2][14] = TERM_L_UMBER; }
					if (csheet_boni[i].pint > 9) { color = TERM_GREEN; if (csheet_boni[i].cb[11] & CB12_RSINT) color = TERM_UMBER; }
					if (csheet_boni[i].pint < 0) { color = TERM_L_RED; if (csheet_boni[i].cb[11] & CB12_RSINT) color = TERM_YELLOW; }
					if (csheet_boni[i].pint < -9) { color = TERM_RED; if (csheet_boni[i].cb[11] & CB12_RSINT) color = TERM_ORANGE; }
					sprintf(tmp, "%d", abs(csheet_boni[i].pint) % 10);
					if (csheet_boni[i].pint > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 15, 45 + i);
				} else if (csheet_boni[i].cb[11] & CB12_RSINT) { c_put_str(TERM_L_UMBER, "s", 15, 45 + i); header_color[2][14] = TERM_L_UMBER; }
				if (csheet_boni[i].pwis != 0) {
					if (header_color[2][15] == TERM_L_DARK) header_color[2][15] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].cb[11] & CB12_RSWIS) { color = TERM_L_UMBER; header_color[2][15] = TERM_L_UMBER; }
					if (csheet_boni[i].pwis > 9) { color = TERM_GREEN; if (csheet_boni[i].cb[11] & CB12_RSWIS) color = TERM_UMBER; }
					if (csheet_boni[i].pwis < 0) { color = TERM_L_RED; if (csheet_boni[i].cb[11] & CB12_RSWIS) color = TERM_YELLOW; }
					if (csheet_boni[i].pwis < -9) { color = TERM_RED; if (csheet_boni[i].cb[11] & CB12_RSWIS) color = TERM_ORANGE; }
					sprintf(tmp, "%d", abs(csheet_boni[i].pwis) % 10);
					if (csheet_boni[i].pwis > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 16, 45 + i);
				} else if (csheet_boni[i].cb[11] & CB12_RSWIS) { c_put_str(TERM_L_UMBER, "s", 16, 45 + i); header_color[2][15] = TERM_L_UMBER; }
				if (csheet_boni[i].pdex != 0) {
					if (header_color[2][16] == TERM_L_DARK) header_color[2][16] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].cb[11] & CB12_RSDEX) { color = TERM_L_UMBER; header_color[2][16] = TERM_L_UMBER; }
					if (csheet_boni[i].pdex > 9) { color = TERM_GREEN; if (csheet_boni[i].cb[11] & CB12_RSDEX) color = TERM_UMBER; }
					if (csheet_boni[i].pdex < 0) { color = TERM_L_RED; if (csheet_boni[i].cb[11] & CB12_RSDEX) color = TERM_YELLOW; }
					if (csheet_boni[i].pdex < -9) { color = TERM_RED; if (csheet_boni[i].cb[11] & CB12_RSDEX) color = TERM_ORANGE; }
					sprintf(tmp, "%d", abs(csheet_boni[i].pdex) % 10);
					if (csheet_boni[i].pdex > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 17, 45 + i);
				} else if (csheet_boni[i].cb[11] & CB12_RSDEX) { c_put_str(TERM_L_UMBER, "s", 17, 45 + i); header_color[2][16] = TERM_L_UMBER; }
				if (csheet_boni[i].pcon != 0) {
					if (header_color[2][17] == TERM_L_DARK) header_color[2][17] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].cb[11] & CB12_RSCON) { color = TERM_L_UMBER; header_color[2][17] = TERM_L_UMBER; }
					if (csheet_boni[i].pcon > 9) { color = TERM_GREEN; if (csheet_boni[i].cb[11] & CB12_RSCON) color = TERM_UMBER; }
					if (csheet_boni[i].pcon < 0) { color = TERM_L_RED; if (csheet_boni[i].cb[11] & CB12_RSCON) color = TERM_YELLOW; }
					if (csheet_boni[i].pcon < -9) { color = TERM_RED; if (csheet_boni[i].cb[11] & CB12_RSCON) color = TERM_ORANGE; }
					sprintf(tmp, "%d", abs(csheet_boni[i].pcon) % 10);
					if (csheet_boni[i].pcon > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 18, 45 + i);
				} else if (csheet_boni[i].cb[11] & CB12_RSCON) { c_put_str(TERM_L_UMBER, "s", 18, 45 + i); header_color[2][17] = TERM_L_UMBER; }
				if (csheet_boni[i].pchr != 0) {
					if (header_color[2][18] == TERM_L_DARK) header_color[2][18] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].cb[11] & CB12_RSCHR) { color = TERM_L_UMBER; header_color[2][18] = TERM_L_UMBER; }
					if (csheet_boni[i].pchr > 9) { color = TERM_GREEN; if (csheet_boni[i].cb[11] & CB12_RSCHR) color = TERM_UMBER; }
					if (csheet_boni[i].pchr < 0) { color = TERM_L_RED; if (csheet_boni[i].cb[11] & CB12_RSCHR) color = TERM_YELLOW; }
					if (csheet_boni[i].pchr < -9) { color = TERM_RED; if (csheet_boni[i].cb[11] & CB12_RSCHR) color = TERM_ORANGE; }
					sprintf(tmp, "%d", abs(csheet_boni[i].pchr) % 10);
					if (csheet_boni[i].pchr > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 19, 45 + i);
				} else if (csheet_boni[i].cb[11] & CB12_RSCHR) { c_put_str(TERM_L_UMBER, "s", 19, 45 + i); header_color[2][18] = TERM_L_UMBER; }

				/* Track AM Field in a similar fashion */
				if (csheet_boni[i].amfi != 0) {
					if (i == 0) amfi1 = csheet_boni[i].amfi;
					else if (i == 1) amfi2 = csheet_boni[i].amfi;
					else amfi_sum += csheet_boni[i].amfi;
					header_color[1][17] = TERM_WHITE;

					/* item? (dark sword only) */
					if (i != 14) {
						if (csheet_boni[i].amfi >= 30) color = TERM_L_UMBER; //show max on darkswords, not on form/skill column though
						else color = TERM_L_GREEN;
					}
					else if (csheet_boni[i].amfi >= ANTIMAGIC_CAP) color = TERM_L_UMBER; //max AM skill, or form+skill combo
					else if ((csheet_boni[i].amfi / 10) >= 5) color = TERM_GREEN; //max AM skill, or form+skill combo
					else color = TERM_L_GREEN;

					sprintf(tmp, "%d", csheet_boni[i].amfi / 10);
					c_put_str(color, tmp, 18, 25 + i);
				}

				/* ESP and slay flags, white for anything */
				if (csheet_boni[i].cb[7] & CB8_ESPID) { c_put_str(TERM_L_BLUE, "E", 1, 65 + i); header_color[3][0] = TERM_WHITE; }
				if (csheet_boni[i].cb[7] & CB8_EANIM) { c_put_str(TERM_L_BLUE, "E", 2, 65 + i); header_color[3][1] = TERM_WHITE; }
				if (csheet_boni[i].cb[7] & CB8_SANIM) { c_put_str(TERM_YELLOW, "s", 2, 65 + i); header_color[3][1] = TERM_WHITE; }
				if ((csheet_boni[i].cb[7] & CB8_EANIM) && (csheet_boni[i].cb[7] & CB8_SANIM)) c_put_str(TERM_WHITE, "+", 2, 65 + i);
				if (csheet_boni[i].cb[7] & CB8_EORCS) { c_put_str(TERM_L_BLUE, "E", 3, 65 + i); header_color[3][2] = TERM_WHITE; }
				if (csheet_boni[i].cb[7] & CB8_SORCS) { c_put_str(TERM_YELLOW, "s", 3, 65 + i); header_color[3][2] = TERM_WHITE; }
				if ((csheet_boni[i].cb[7] & CB8_EORCS) && (csheet_boni[i].cb[7] & CB8_SORCS)) c_put_str(TERM_WHITE, "+", 3, 65 + i);
				if (csheet_boni[i].cb[7] & CB8_ETROL) { c_put_str(TERM_L_BLUE, "E", 4, 65 + i); header_color[3][3] = TERM_WHITE; }
				if (csheet_boni[i].cb[7] & CB8_STROL) { c_put_str(TERM_YELLOW, "s", 4, 65 + i); header_color[3][3] = TERM_WHITE; }
				if ((csheet_boni[i].cb[7] & CB8_ETROL) && (csheet_boni[i].cb[7] & CB8_STROL)) c_put_str(TERM_WHITE, "+", 4, 65 + i);
				if (csheet_boni[i].cb[7] & CB8_EGIAN) { c_put_str(TERM_L_BLUE, "E", 5, 65 + i); header_color[3][4] = TERM_WHITE; }

				if (csheet_boni[i].cb[8] & CB9_SGIAN) { c_put_str(TERM_YELLOW, "s", 5, 65 + i); header_color[3][4] = TERM_WHITE; }
				if ((csheet_boni[i].cb[7] & CB8_EGIAN) && (csheet_boni[i].cb[8] & CB9_SGIAN)) c_put_str(TERM_WHITE, "+", 5, 65 + i);
				if (csheet_boni[i].cb[8] & CB9_EDRGN) { c_put_str(TERM_L_BLUE, "E", 6, 65 + i); header_color[3][5] = TERM_WHITE; }
				if (csheet_boni[i].cb[8] & CB9_SDRGN) { c_put_str(TERM_YELLOW, "s", 6, 65 + i); header_color[3][5] = TERM_WHITE; }
				if (csheet_boni[i].cb[8] & CB9_KDRGN) { c_put_str(TERM_WHITE, "k", 6, 65 + i); header_color[3][5] = TERM_WHITE; }
				if ((csheet_boni[i].cb[8] & CB9_EDRGN) && (csheet_boni[i].cb[8] & CB9_SDRGN)) c_put_str(TERM_WHITE, "+", 6, 65 + i);
				if ((csheet_boni[i].cb[8] & CB9_EDRGN) && (csheet_boni[i].cb[8] & CB9_KDRGN)) c_put_str(TERM_WHITE, "*", 6, 65 + i);
				if (csheet_boni[i].cb[8] & CB9_EDEMN) { c_put_str(TERM_L_BLUE, "E", 7, 65 + i); header_color[3][6] = TERM_WHITE; }
				if (csheet_boni[i].cb[8] & CB9_SDEMN) { c_put_str(TERM_YELLOW, "s", 7, 65 + i); header_color[3][6] = TERM_WHITE; }
				if (csheet_boni[i].cb[8] & CB9_KDEMN) { c_put_str(TERM_WHITE, "k", 7, 65 + i); header_color[3][6] = TERM_WHITE; }
				if ((csheet_boni[i].cb[8] & CB9_EDEMN) && (csheet_boni[i].cb[8] & CB9_SDEMN)) c_put_str(TERM_WHITE, "+", 7, 65 + i);
				if ((csheet_boni[i].cb[8] & CB9_EDEMN) && (csheet_boni[i].cb[8] & CB9_KDEMN)) c_put_str(TERM_WHITE, "*", 7, 65 + i);
				if (csheet_boni[i].cb[8] & CB9_EUNDD) { c_put_str(TERM_L_BLUE, "E", 8, 65 + i); header_color[3][7] = TERM_WHITE; }

				if (csheet_boni[i].cb[9] & CB10_SUNDD) { c_put_str(TERM_YELLOW, "s", 8, 65 + i); header_color[3][7] = TERM_WHITE; }
				if (csheet_boni[i].cb[9] & CB10_KUNDD) { c_put_str(TERM_WHITE, "k", 8, 65 + i); header_color[3][7] = TERM_WHITE; }
				if ((csheet_boni[i].cb[8] & CB9_EUNDD) && (csheet_boni[i].cb[9] & CB10_SUNDD)) c_put_str(TERM_WHITE, "+", 8, 65 + i);
				if ((csheet_boni[i].cb[8] & CB9_EUNDD) && (csheet_boni[i].cb[9] & CB10_KUNDD)) c_put_str(TERM_WHITE, "*", 8, 65 + i);
				if (csheet_boni[i].cb[9] & CB10_EEVIL) { c_put_str(TERM_L_BLUE, "E", 9, 65 + i); header_color[3][8] = TERM_WHITE; }
				if (csheet_boni[i].cb[9] & CB10_SEVIL) { c_put_str(TERM_YELLOW, "s", 9, 65 + i); header_color[3][8] = TERM_WHITE; }
				if ((csheet_boni[i].cb[9] & CB10_EEVIL) && (csheet_boni[i].cb[9] & CB10_SEVIL)) c_put_str(TERM_WHITE, "+", 9, 65 + i);
				if (csheet_boni[i].cb[9] & CB10_EDGRI) { c_put_str(TERM_L_BLUE, "E", 10, 65 + i); header_color[3][9] = TERM_WHITE; }
				if (csheet_boni[i].cb[9] & CB10_EGOOD) { c_put_str(TERM_L_BLUE, "E", 11, 65 + i); header_color[3][10] = TERM_WHITE; }
				if (csheet_boni[i].cb[9] & CB10_ENONL) { c_put_str(TERM_L_BLUE, "E", 12, 65 + i); header_color[3][11] = TERM_WHITE; }
				if (csheet_boni[i].cb[9] & CB10_EUNIQ) { c_put_str(TERM_L_BLUE, "E", 13, 65 + i); header_color[3][12] = TERM_WHITE; }

				if (csheet_boni[i].cb[10] & CB11_BFIRE) { c_put_str(TERM_WHITE, "b", 14, 65 + i); header_color[3][13] = TERM_WHITE; }
				if (csheet_boni[i].cb[10] & CB11_AFIRE) { c_put_str(TERM_WHITE, "a", 14, 65 + i); header_color[3][13] = TERM_WHITE; }
				if ((csheet_boni[i].cb[10] & CB11_BFIRE) && (csheet_boni[i].cb[10] & CB11_AFIRE)) c_put_str(TERM_WHITE, "+", 14, 65 + i);
				if (csheet_boni[i].cb[10] & CB11_BCOLD) { c_put_str(TERM_WHITE, "b", 15, 65 + i); header_color[3][14] = TERM_WHITE; }
				if (csheet_boni[i].cb[10] & CB11_ACOLD) { c_put_str(TERM_WHITE, "a", 15, 65 + i); header_color[3][14] = TERM_WHITE; }
				if ((csheet_boni[i].cb[10] & CB11_BCOLD) && (csheet_boni[i].cb[10] & CB11_ACOLD)) c_put_str(TERM_WHITE, "+", 15, 65 + i);
				if (csheet_boni[i].cb[10] & CB11_BELEC) { c_put_str(TERM_WHITE, "b", 16, 65 + i); header_color[3][15] = TERM_WHITE; }
				if (csheet_boni[i].cb[10] & CB11_AELEC) { c_put_str(TERM_WHITE, "a", 16, 65 + i); header_color[3][15] = TERM_WHITE; }
				if ((csheet_boni[i].cb[10] & CB11_BELEC) && (csheet_boni[i].cb[10] & CB11_AELEC)) c_put_str(TERM_WHITE, "+", 16, 65 + i);
				if (csheet_boni[i].cb[10] & CB11_BACID) { c_put_str(TERM_WHITE, "b", 17, 65 + i); header_color[3][16] = TERM_WHITE; }
				if (csheet_boni[i].cb[10] & CB11_BPOIS) { c_put_str(TERM_WHITE, "b", 18, 65 + i); header_color[3][17] = TERM_WHITE; }
				if (csheet_boni[i].cb[11] & CB12_BVORP) { c_put_str(TERM_WHITE, "+", 19, 65 + i); header_color[3][18] = TERM_WHITE; }

				/* Footer */
				color = csheet_boni[i].color;
				symbol = (csheet_boni[i].symbol ? csheet_boni[i].symbol : ' ');
				for (j = 0; j < 4; j++)
					Term_draw(5 + j * 20 + i, 20, color, symbol);
			}

			/* AM field - body? skill or monster form */
			if (amfi1 > amfi2) amfi_sum += amfi1;
			else amfi_sum += amfi2;
			if (amfi_sum >= ANTIMAGIC_CAP) header_color[1][17] = TERM_L_UMBER;

			/* Temporary Boni: TERM_L_BLUE for double-resist! - Kurzel */
			if (p_ptr->oppose_fire) {
				if (header_color[0][0] == TERM_WHITE) header_color[0][0] = TERM_L_BLUE;
				if (header_color[0][0] == TERM_L_DARK || header_color[0][0] == TERM_RED) header_color[0][0] = TERM_WHITE;
			}
			if (p_ptr->nimbus && p_ptr->nimbus == GF_FIRE) { header_color[0][0] = TERM_L_UMBER; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_HELLFIRE) { header_color[0][0] = TERM_L_UMBER; }
			if (p_ptr->oppose_cold) {
				if (header_color[0][1] == TERM_WHITE) header_color[0][1] = TERM_L_BLUE;
				if (header_color[0][1] == TERM_L_DARK || header_color[0][1] == TERM_RED) header_color[0][0] = TERM_WHITE;
			}
			if (p_ptr->nimbus && p_ptr->nimbus == GF_COLD) { header_color[0][1] = TERM_L_UMBER; }
			if (p_ptr->oppose_elec) {
				if (header_color[0][2] == TERM_WHITE) header_color[0][2] = TERM_L_BLUE;
				if (header_color[0][2] == TERM_L_DARK || header_color[0][2] == TERM_RED) header_color[0][0] = TERM_WHITE;
			}
			if (p_ptr->nimbus && p_ptr->nimbus == GF_ELEC) { header_color[0][2] = TERM_L_UMBER; }
			if (p_ptr->oppose_acid) {
				if (header_color[0][3] == TERM_WHITE) header_color[0][3] = TERM_L_BLUE;
				if (header_color[0][3] == TERM_L_DARK || header_color[0][3] == TERM_RED) header_color[0][0] = TERM_WHITE;
			}
			if (p_ptr->nimbus && p_ptr->nimbus == GF_ACID) { header_color[0][3] = TERM_L_UMBER; }
			if (p_ptr->oppose_pois) {
				if (header_color[0][4] == TERM_WHITE) header_color[0][4] = TERM_L_BLUE;
				if (header_color[0][4] == TERM_L_DARK || header_color[0][4] == TERM_RED) header_color[0][0] = TERM_WHITE;
			}
			if (p_ptr->nimbus && p_ptr->nimbus == GF_POIS) { header_color[0][4] = TERM_L_UMBER; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_LITE) { if (header_color[0][6] != TERM_L_UMBER) header_color[0][6] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_DARK) { if (header_color[0][7] != TERM_L_UMBER) header_color[0][7] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_GRAVITY) { if (header_color[0][8] != TERM_L_UMBER) header_color[0][8] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_SOUND) { if (header_color[0][8] != TERM_L_UMBER) header_color[0][8] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_FORCE) { if (header_color[0][8] != TERM_L_UMBER) header_color[0][8] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_SHARDS) { if (header_color[0][9] != TERM_L_UMBER) header_color[0][9] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_NEXUS) { if (header_color[0][10] != TERM_L_UMBER) header_color[0][10] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_NETHER) { if (header_color[0][11] != TERM_L_UMBER) header_color[0][11] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_CONFUSION) { if (header_color[0][12] != TERM_L_UMBER) header_color[0][12] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_CHAOS) {
				if (header_color[0][12] == TERM_L_DARK || header_color[0][12] == TERM_RED) header_color[0][12] = TERM_YELLOW;
				if (header_color[0][13] != TERM_L_UMBER) header_color[0][13] = TERM_WHITE;
			}
			if (p_ptr->nimbus && p_ptr->nimbus == GF_DISENCHANT) { if (header_color[0][14] != TERM_L_UMBER) header_color[0][14] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_WATER) { header_color[0][15] = TERM_L_UMBER; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_TIME) { if (header_color[0][16] != TERM_L_UMBER) header_color[0][16] = TERM_WHITE; }
			if (p_ptr->divine_xtra_res) { if (header_color[0][17] != TERM_L_UMBER) header_color[0][17] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_MANA) { if (header_color[0][17] != TERM_L_UMBER) header_color[0][17] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_INERTIA) { if (header_color[1][1] != TERM_L_UMBER) header_color[1][1] = TERM_WHITE; } // PARA
			if (p_ptr->nimbus && p_ptr->nimbus == GF_GRAVITY) { if (header_color[1][1] != TERM_L_UMBER) header_color[1][1] = TERM_WHITE; } // PARA
			if (p_ptr->nimbus && p_ptr->nimbus == GF_GRAVITY) { if (header_color[1][2] != TERM_YELLOW) header_color[1][2] = TERM_WHITE; } // TELE
			if (p_ptr->nimbus && p_ptr->nimbus == GF_GRAVITY) { if (header_color[1][7] != TERM_L_UMBER) header_color[1][7] = TERM_WHITE; } // FFAL

			/* Row Headers */
			row = 1;
			c_put_str(header_color[0][0], "Fire:", row++, 0); //row, col
			c_put_str(header_color[0][1], "Cold:", row++,  0);
			c_put_str(header_color[0][2], "Elec:", row++,  0);
			c_put_str(header_color[0][3], "Acid:", row++,  0);
			c_put_str(header_color[0][4], "Pois:", row++,  0);
			c_put_str(header_color[0][5], "Blnd:", row++,  0);
			c_put_str(header_color[0][6], "Lite:", row++,  0);
			c_put_str(header_color[0][7], "Dark:", row++,  0);
			c_put_str(header_color[0][8], "Soun:", row++,  0);
			c_put_str(header_color[0][9], "Shrd:", row++,  0);
			c_put_str(header_color[0][10], "Nexu:", row++,  0);
			c_put_str(header_color[0][11], "Neth:", row++,  0);
			c_put_str(header_color[0][12], "Conf:", row++,  0);
			c_put_str(header_color[0][13], "Chao:", row++,  0);
			c_put_str(header_color[0][14], "Dise:", row++,  0);
			c_put_str(header_color[0][15], "Watr:", row++,  0);
			c_put_str(header_color[0][16], "Time:", row++,  0);
			c_put_str(header_color[0][17], "Mana:", row++,  0);
			c_put_str(header_color[0][18], "Mind:", row++,  0);

			row = 1;
			c_put_str(header_color[1][0], "Fear:", row++,  20);
			c_put_str(header_color[1][1], "Para:", row++,  20);
			c_put_str(header_color[1][2], "HLif:", row++,  20);
			c_put_str(header_color[1][3], "Tele:", row++,  20);
			c_put_str(header_color[1][4], "FFal:", row++,  20);
			c_put_str(header_color[1][5], "Lvtn:", row++,  20);
			c_put_str(header_color[1][6], "Clmb:", row++,  20);
			c_put_str(header_color[1][7], "SInv:", row++,  20);
			c_put_str(header_color[1][8], "Invs:", row++,  20);
			c_put_str(header_color[1][9], "Refl:", row++,  20);
			c_put_str(header_color[1][10], "Wrth:", row++,  20);
			c_put_str(header_color[1][11], "RgHP:", row++,  20);
			c_put_str(header_color[1][12], "RgMP:", row++,  20);
			c_put_str(header_color[1][13], "Food:", row++,  20);
			c_put_str(header_color[1][14], "Vamp:", row++,  20);
			c_put_str(header_color[1][15], "AuID:", row++,  20);
			c_put_str(header_color[1][16], "AMSh:", row++,  20);
			c_put_str(header_color[1][17], "AMFi:", row++,  20);
			c_put_str(header_color[1][18], "Aggr:", row++,  20);

			row = 1;
			c_put_str(header_color[2][0], "Spd :", row++,  40);
			c_put_str(header_color[2][1], "Slth:", row++,  40);
			c_put_str(header_color[2][2], "Srch:", row++,  40);
			c_put_str(header_color[2][3], "Infr:", row++,  40);
			c_put_str(header_color[2][4], "Lite:", row++,  40);
			c_put_str(header_color[2][5], "Tunn:", row++,  40);
			c_put_str(header_color[2][6], "Blow:", row++,  40);
			c_put_str(header_color[2][7], "Crit:", row++,  40);
			c_put_str(header_color[2][8], "Shot:", row++,  40);
			c_put_str(header_color[2][9], "Mght:", row++,  40);
			c_put_str(header_color[2][10], "MxHP:", row++,  40);
			c_put_str(header_color[2][11], "MxMP:", row++,  40);
			c_put_str(header_color[2][12], "Luck:", row++,  40);
			c_put_str(header_color[2][13], "STR :", row++,  40);
			c_put_str(header_color[2][14], "INT :", row++,  40);
			c_put_str(header_color[2][15], "WIS :", row++,  40);
			c_put_str(header_color[2][16], "DEX :", row++,  40);
			c_put_str(header_color[2][17], "CON :", row++,  40);
			c_put_str(header_color[2][18], "CHR :", row++,  40);

			row = 1;
			c_put_str(header_color[3][0], "Spid:", row++,  60);
			c_put_str(header_color[3][1], "Anim:", row++,  60);
			c_put_str(header_color[3][2], "Orcs:", row++,  60);
			c_put_str(header_color[3][3], "Trol:", row++,  60);
			c_put_str(header_color[3][4], "Gian:", row++,  60);
			c_put_str(header_color[3][5], "Drgn:", row++,  60);
			c_put_str(header_color[3][6], "Demn:", row++,  60);
			c_put_str(header_color[3][7], "Undd:", row++,  60);
			c_put_str(header_color[3][8], "Evil:", row++,  60);
			c_put_str(header_color[3][9], "Dgri:", row++,  60);
			c_put_str(header_color[3][10], "Good:", row++,  60);
			c_put_str(header_color[3][11], "Nonl:", row++,  60);
			c_put_str(header_color[3][12], "Uniq:", row++,  60);
			c_put_str(header_color[3][13], "Fire:", row++,  60);
			c_put_str(header_color[3][14], "Cold:", row++,  60);
			c_put_str(header_color[3][15], "Elec:", row++,  60);
			c_put_str(header_color[3][16], "Acid:", row++,  60);
			c_put_str(header_color[3][17], "Pois:", row++,  60);
			c_put_str(header_color[3][18], "Vorp:", row++,  60);
		}
		/* Horizontal view (Horizontal rows of different stats) */
		else {
			int header_color[1][76], row = -1, col = 0;
			int x_offset = 1; /* x_offset must be 0 if c_put_str_vert() operates in normal mode and 1 if in vertical mode! */

			for (i = 0; i < 1; i++)
				for (j = 0; j < 76; j++)
					header_color[i][j] = TERM_L_DARK;

			/* Fill Columns */
			for (i = 0; i < 15; i++) {
				/* Header */
				color = TERM_WHITE;
				if (csheet_boni[i].cb[11] & CB12_XHIDD) color = TERM_YELLOW;
				if (csheet_boni[i].cb[12] & CB13_XSTAR) color = TERM_L_UMBER;
				if (csheet_boni[i].cb[12] & CB13_XCRSE) color = TERM_RED;
				if (csheet_boni[i].cb[12] & CB13_XNOEQ) color = TERM_L_DARK;
				if (i == 14) color = (p_ptr->ghost) ? TERM_L_DARK : (p_ptr->fruit_bat == 1) ? TERM_ORANGE : TERM_WHITE;
				for (j = 0; j < 1; j++) {
					switch (i) {
					case 0: { c_put_str(color, "a", 6 + j * 20 + i, x_offset); break; }
					case 1: { c_put_str(color, "b", 6 + j * 20 + i, x_offset); break; }
					case 2: { c_put_str(color, "c", 6 + j * 20 + i, x_offset); break; }
					case 3: { c_put_str(color, "d", 6 + j * 20 + i, x_offset); break; }
					case 4: { c_put_str(color, "e", 6 + j * 20 + i, x_offset); break; }
					case 5: { c_put_str(color, "f", 6 + j * 20 + i, x_offset); break; }
					case 6: { c_put_str(color, "g", 6 + j * 20 + i, x_offset); break; }
					case 7: { c_put_str(color, "h", 6 + j * 20 + i, x_offset); break; }
					case 8: { c_put_str(color, "i", 6 + j * 20 + i, x_offset); break; }
					case 9: { c_put_str(color, "j", 6 + j * 20 + i, x_offset); break; }
					case 10: { c_put_str(color, "k", 6 + j * 20 + i, x_offset); break; }
					case 11: { c_put_str(color, "l", 6 + j * 20 + i, x_offset); break; }
					case 12: { c_put_str(color, "m", 6 + j * 20 + i, x_offset); break; }
					case 13: { c_put_str(color, "n", 6 + j * 20 + i, x_offset); break; }
					case 14: { c_put_str(color, (p_ptr->fruit_bat == 1) ? "b" : "@", 6 + j * 20 + i, x_offset); break; }
					}
				}

				/* Blank Fill */
				color = TERM_SLATE;
				if (csheet_boni[i].cb[12] & CB13_XNOEQ) color = TERM_L_DARK;
				if (csheet_boni[i].cb[11] & CB12_XHIDD) color = TERM_YELLOW;
				for (j = 0; j < 1; j++) {
					for (k = 1; k <= 76; k++)
						c_put_str(color, ".", 6 + j + i, k + x_offset);
				}
				if (csheet_boni[i].cb[12] & CB13_XNOEQ) continue;

				/* Fill with boni values and track header colour. Gross hardcode! - Kurzel */
				if (csheet_boni[i].cb[0] & CB1_SFIRE) { c_put_str(TERM_RED, "-", 6 + i, 1 + x_offset); if (header_color[0][0] == TERM_L_DARK) header_color[0][0] = TERM_RED; }
				if (csheet_boni[i].cb[0] & CB1_RFIRE) { c_put_str(TERM_WHITE, "+", 6 + i, 1 + x_offset); if (header_color[0][0] == TERM_L_DARK || header_color[0][0] == TERM_RED) header_color[0][0] = TERM_WHITE; }
				if (csheet_boni[i].cb[0] & CB1_IFIRE) { c_put_str(TERM_L_UMBER, "*", 6 + i, 1 + x_offset); header_color[0][0] = TERM_L_UMBER; }
				if (csheet_boni[i].cb[0] & CB1_SCOLD) { c_put_str(TERM_RED, "-", 6 + i, 2 + x_offset); if (header_color[0][1] == TERM_L_DARK) header_color[0][1] = TERM_RED; }
				if (csheet_boni[i].cb[0] & CB1_RCOLD) { c_put_str(TERM_WHITE, "+", 6 + i, 2 + x_offset); if (header_color[0][1] == TERM_L_DARK || header_color[0][1] == TERM_RED) header_color[0][1] = TERM_WHITE; }
				if (csheet_boni[i].cb[0] & CB1_ICOLD) { c_put_str(TERM_L_UMBER, "*", 6 + i, 2 + x_offset); header_color[0][1] = TERM_L_UMBER; }
				if (csheet_boni[i].cb[0] & CB1_SELEC) { c_put_str(TERM_RED, "-", 6 + i, 3 + x_offset); if (header_color[0][2] == TERM_L_DARK) header_color[0][2] = TERM_RED; }
				if (csheet_boni[i].cb[0] & CB1_RELEC) { c_put_str(TERM_WHITE, "+", 6 + i, 3 + x_offset); if (header_color[0][2] == TERM_L_DARK || header_color[0][2] == TERM_RED) header_color[0][2] = TERM_WHITE; }
				if (csheet_boni[i].cb[1] & CB2_IELEC) { c_put_str(TERM_L_UMBER, "*", 6 + i, 3 + x_offset); header_color[0][2] = TERM_L_UMBER; }
				if (csheet_boni[i].cb[1] & CB2_SACID) { c_put_str(TERM_RED, "-", 6 + i, 4 + x_offset); if (header_color[0][3] == TERM_L_DARK) header_color[0][3] = TERM_RED; }
				if (csheet_boni[i].cb[1] & CB2_RACID) { c_put_str(TERM_WHITE, "+", 6 + i, 4 + x_offset); if (header_color[0][3] == TERM_L_DARK || header_color[0][3] == TERM_RED) header_color[0][3] = TERM_WHITE; }
				if (csheet_boni[i].cb[1] & CB2_IACID) { c_put_str(TERM_L_UMBER, "*", 6 + i, 4 + x_offset); header_color[0][3] = TERM_L_UMBER; }
				if (csheet_boni[i].cb[1] & CB2_SPOIS) { c_put_str(TERM_RED, "-", 6 + i, 5 + x_offset); if (header_color[0][4] == TERM_L_DARK) header_color[0][4] = TERM_RED; }
				if (csheet_boni[i].cb[1] & CB2_RPOIS) { c_put_str(TERM_WHITE, "+", 6 + i, 5 + x_offset); if (header_color[0][4] == TERM_L_DARK || header_color[0][4] == TERM_RED) header_color[0][4] = TERM_WHITE; }
				if (csheet_boni[i].cb[1] & CB2_IPOIS) { c_put_str(TERM_L_UMBER, "*", 6 + i, 5 + x_offset); header_color[0][4] = TERM_L_UMBER; }
				if (csheet_boni[i].cb[1] & CB2_RBLND) { c_put_str(TERM_WHITE, "+", 6 + i, 6 + x_offset); header_color[0][5] = TERM_WHITE; }
				if (csheet_boni[i].cb[2] & CB3_SLITE) { c_put_str(TERM_RED, "-", 6 + i, 7 + x_offset); if (header_color[0][6] == TERM_L_DARK) header_color[0][6] = TERM_RED; }
				if (csheet_boni[i].cb[2] & CB3_RLITE) { c_put_str(TERM_WHITE, "+", 6 + i, 7 + x_offset); if (header_color[0][6] == TERM_L_DARK || header_color[0][6] == TERM_RED) header_color[0][6] = TERM_WHITE; }
				if (csheet_boni[i].cb[2] & CB3_RDARK) { c_put_str(TERM_WHITE, "+", 6 + i, 8 + x_offset); if (header_color[0][7] == TERM_L_DARK) header_color[0][7] = TERM_WHITE; }
				if (csheet_boni[i].cb[2] & CB3_RSOUN) { c_put_str(TERM_WHITE, "+", 6 + i, 9 + x_offset); header_color[0][8] = TERM_WHITE; }
				if (csheet_boni[i].cb[12] & CB13_XNCUT) c_put_str(TERM_WHITE, "s", 6 + i, 10 + x_offset);
				if (csheet_boni[i].cb[2] & CB3_RSHRD) { c_put_str(TERM_WHITE, "+", 6 + i, 10 + x_offset); header_color[0][9] = TERM_WHITE; }
				if ((csheet_boni[i].cb[2] & CB3_RSHRD) && (csheet_boni[i].cb[12] & CB13_XNCUT)) c_put_str(TERM_WHITE, "*", 6 + i, 10 + x_offset);
				if (csheet_boni[i].cb[2] & CB3_RNEXU) { c_put_str(TERM_WHITE, "+", 6 + i, 11 + x_offset); header_color[0][10] = TERM_WHITE; }
				if (csheet_boni[i].cb[2] & CB3_RNETH) { c_put_str(TERM_WHITE, "+", 6 + i, 12 + x_offset); if (header_color[0][11] == TERM_L_DARK) header_color[0][11] = TERM_WHITE; }
				if (csheet_boni[i].cb[2] & CB3_INETH) { c_put_str(TERM_L_UMBER, "*", 6 + i, 12 + x_offset); header_color[0][11] = TERM_L_UMBER; }
				if (csheet_boni[i].cb[3] & CB4_RCONF) { c_put_str(TERM_WHITE, "+", 6 + i, 13 + x_offset); if (header_color[0][12] == TERM_L_DARK || header_color[0][12] == TERM_YELLOW) header_color[0][12] = TERM_WHITE; }
				if (csheet_boni[i].cb[3] & CB4_RCHAO) {
					c_put_str(TERM_WHITE, "+", 6 + i, 14 + x_offset); header_color[0][13] = TERM_WHITE;
					//Chaos covers confusion for whatever reason, maybe confusion breathers should breathe psi damage instead, and confusion should become non-damaging? - Kurzel
					//Could replace br_conf with a psi attack for the affected mobs, breathing psi doesn't make any sense either, some mobs could be removed even. - Kurzel
					//Bronze D / draconians could get gravity/lightning to be consistent with other metallic dragon abilities/lore? Same colour as gravity Z now anyway. - Kurzel
					if (!(csheet_boni[i].cb[3] & CB4_RCONF)) { c_put_str(TERM_YELLOW, "+", 6 + i, 13 + x_offset); if (header_color[0][12] == TERM_L_DARK) header_color[0][12] = TERM_YELLOW; }
				}
				if (csheet_boni[i].cb[3] & CB4_RDISE) { c_put_str(TERM_WHITE, "+", 6 + i, 15 + x_offset); header_color[0][14] = TERM_WHITE; }
				if (csheet_boni[i].cb[3] & CB4_RWATR) { c_put_str(TERM_WHITE, "+", 6 + i, 16 + x_offset); if (header_color[0][15] == TERM_L_DARK) header_color[0][15] = TERM_WHITE; }
				if (csheet_boni[i].cb[3] & CB4_IWATR) { c_put_str(TERM_L_UMBER, "*", 6 + i, 16 + x_offset); header_color[0][15] = TERM_L_UMBER; }
				if (csheet_boni[i].cb[3] & CB4_RTIME) { c_put_str(TERM_WHITE, "+", 6 + i, 17 + x_offset); header_color[0][16] = TERM_WHITE; }
				if (csheet_boni[i].cb[3] & CB4_RMANA) { c_put_str(TERM_WHITE, "+", 6 + i, 18 + x_offset); header_color[0][17] = TERM_WHITE; }
				if (csheet_boni[i].cb[13] & CB14_UMIND) { c_put_str(TERM_WHITE, "*", 6 + i, 19 + x_offset); header_color[0][18] = TERM_WHITE; } //I added a 'level 3' - C. Blue
				else if (csheet_boni[i].cb[4] & CB5_XMIND) { c_put_str(TERM_WHITE, "+", 6 + i, 19 + x_offset); header_color[0][18] = TERM_WHITE; } //Similar to no_cut, for now? Level 2~ - Kurzel
				else if (csheet_boni[i].cb[3] & CB4_RMIND) { c_put_str(TERM_WHITE, "~", 6 + i, 19 + x_offset); header_color[0][18] = TERM_WHITE; }

				if (csheet_boni[i].cb[4] & CB5_RFEAR) { c_put_str(TERM_WHITE, "+", 6 + i, 1 + 19 + x_offset); header_color[0][19 + 0] = TERM_WHITE; }
				if (csheet_boni[i].cb[4] & CB5_RPARA) { c_put_str(TERM_WHITE, "+", 6 + i, 2 + 19 + x_offset); header_color[0][19 + 1] = TERM_WHITE; }
				if (csheet_boni[i].cb[5] & CB6_RLIFE) { c_put_str(TERM_WHITE, "+", 6 + i, 3 + 19 + x_offset); header_color[0][19 + 2] = TERM_WHITE; }
				if (csheet_boni[i].cb[13] & CB14_ILIFE) { c_put_str(TERM_L_UMBER, "*", 6 + i, 3 + 19 + x_offset); header_color[0][19 + 2] = TERM_L_UMBER; }
				if (csheet_boni[i].cb[4] & CB5_STELE) { c_put_str(TERM_RED, "t", 6 + i, 4 + 19 + x_offset); if (header_color[0][19 + 3] == TERM_L_DARK || header_color[0][19 + 3] == TERM_WHITE) header_color[0][19 + 3] = TERM_RED; } //auto-tele; overrides the resist, no items with both
				if (csheet_boni[i].cb[4] & CB5_RTELE) { c_put_str(TERM_WHITE, "+", 6 + i, 4 + 19 + x_offset); if (header_color[0][19 + 3] == TERM_L_DARK) header_color[0][19 + 3] = TERM_WHITE; }
				if (csheet_boni[i].cb[4] & CB5_ITELE) { c_put_str(TERM_L_RED, "N", 6 + i, 4 + 19 + x_offset); if (header_color[0][19 + 3] != TERM_L_RED) header_color[0][19 + 3] = TERM_L_RED; } //NO_TELE
				if (csheet_boni[i].cb[5] & CB6_RFFAL) { c_put_str(TERM_WHITE, "+", 6 + i, 5 + 19 + x_offset); header_color[0][19 + 4] = TERM_WHITE; }
				if (csheet_boni[i].cb[12] & CB13_XSWIM) { c_put_str(TERM_L_BLUE, "~", 6 + i, 6 + 19 + x_offset); if (header_color[0][19 + 5] == TERM_L_DARK) header_color[0][19 + 5] = TERM_YELLOW; }
				if (csheet_boni[i].cb[12] & CB13_XTREE) { c_put_str(TERM_GREEN, "#", 6 + i, 6 + 19 + x_offset); if (header_color[0][19 + 5] == TERM_L_DARK) header_color[0][19 + 5] = TERM_YELLOW; }
				if ((csheet_boni[i].cb[12] & CB13_XTREE) && (csheet_boni[i].cb[12] & CB13_XSWIM)) c_put_str(TERM_YELLOW, "+", 6 + i, 6 + 19 + x_offset); //Almost flying~
				if (csheet_boni[i].cb[5] & CB6_RLVTN) {
					c_put_str(TERM_WHITE, "+", 6 + i, 6 + 19 + x_offset); header_color[0][19 + 5] = TERM_WHITE;
					//Levitation covers feather falling, similar to chaos/confusion but plausible? - Kurzel
					if (!(csheet_boni[i].cb[5] & CB6_RFFAL)) { c_put_str(TERM_YELLOW, "+", 5 + i, 5 + 19 + x_offset); if (header_color[0][19 + 4] == TERM_L_DARK) header_color[0][19 + 4] = TERM_YELLOW; }
				}
				if (csheet_boni[i].cb[5] & CB6_RCLMB) { c_put_str(TERM_WHITE, "+", 6 + i, 7 + 19 + x_offset); header_color[0][19 + 6] = TERM_WHITE; }
				if (csheet_boni[i].cb[4] & CB5_RSINV) { c_put_str(TERM_WHITE, "+", 6 + i, 8 + 19 + x_offset); if (header_color[0][19 + 7] == TERM_L_DARK) header_color[0][19 + 7] = TERM_WHITE; }
				if (csheet_boni[i].cb[4] & CB5_RINVS) { c_put_str(TERM_WHITE, "+", 6 + i, 9 + 19 + x_offset); header_color[0][19 + 8] = TERM_WHITE; }
				if (csheet_boni[i].cb[6] & CB7_RREFL) { c_put_str(TERM_WHITE, "+", 6 + i, 10 + 19 + x_offset); header_color[0][19 + 9] = TERM_WHITE; }
				if (csheet_boni[i].cb[5] & CB6_RWRTH) { c_put_str(TERM_WHITE, "+", 6 + i, 11 + 19 + x_offset); header_color[0][19 + 10] = TERM_WHITE; }
				if (csheet_boni[i].cb[5] & CB6_SRGHP) { c_put_str(TERM_RED, "-", 6 + i, 12 + 19 + x_offset); if (header_color[0][19 + 11] == TERM_L_DARK) header_color[0][19 + 11] = TERM_RED; }
				if (csheet_boni[i].cb[5] & CB6_RRGHP) { c_put_str(TERM_WHITE, "+", 6 + i, 12 + 19 + x_offset); if (header_color[0][19 + 11] == TERM_L_DARK) header_color[0][19 + 11] = TERM_WHITE; if (header_color[0][19 + 11] == TERM_RED || header_color[0][19 + 11] == TERM_YELLOW) header_color[0][19 + 11] = TERM_YELLOW; } //yellow if both...
				if (csheet_boni[i].cb[5] & CB6_SRGMP) { c_put_str(TERM_RED, "-", 6 + i, 13 + 19 + x_offset); if (header_color[0][19 + 12] == TERM_L_DARK) header_color[0][19 + 12] = TERM_RED; }
				if (csheet_boni[i].cb[6] & CB7_RRGMP) { c_put_str(TERM_WHITE, "+", 6 + i, 13 + 19 + x_offset); if (header_color[0][19 + 12] == TERM_L_DARK) header_color[0][19 + 12] = TERM_WHITE; if (header_color[0][19 + 12] == TERM_RED || header_color[0][19 + 12] == TERM_YELLOW) header_color[0][19 + 12] = TERM_YELLOW; } //yellow if both...
				if (csheet_boni[i].cb[6] & CB7_RFOOD) { c_put_str(TERM_WHITE, "+", 6 + i, 14 + 19 + x_offset); if (header_color[0][19 + 13] == TERM_L_DARK) header_color[0][19 + 13] = TERM_WHITE; }
				if (p_ptr->prace == RACE_ENT || p_ptr->prace == RACE_VAMPIRE) { c_put_str(TERM_WHITE, "*", 6 + 14, 13 + 19 + x_offset); if (header_color[0][19 + 13] == TERM_L_DARK) header_color[0][19 + 13] = TERM_WHITE; } //Hack: Ents/Vamps require food but do not gorge! - Kurzel
				if (csheet_boni[i].cb[6] & CB7_IFOOD) { c_put_str(TERM_L_UMBER, "*", 6 + i, 14 + 19 + x_offset); header_color[0][19 + 13] = TERM_L_UMBER; }
				if (csheet_boni[i].cb[6] & CB7_RVAMP) {
					if ((i == 0) || (i == 1) || (i == 2) || (i == 12)
					    || ((i == 14) && (!strcasecmp(c_p_ptr->body_name, "Vampiric mist") || !strcasecmp(c_p_ptr->body_name, "Vampire bat"))) //Hack: use * for 100% weapon/ammo or v-bat/mist forms
					    || (i == 14 && race == RACE_VAMPIRE && get_skill(SKILL_NECROMANCY) == 50 && get_skill(SKILL_TRAUMATURGY) == 50)) { //Nasty hack: Assume that having full trauma+necro gives 100% vamp actually
						c_put_str(TERM_WHITE, "*", 6 + i, 15 + 19 + x_offset); header_color[0][19 + 14] = TERM_WHITE;
					} else { c_put_str(TERM_WHITE, "+", 6 + i, 15 + 19 + x_offset); header_color[0][19 + 14] = TERM_WHITE; }
				}
				if (p_ptr->fruit_bat == 1 && !strcasecmp(c_p_ptr->body_name, "Player")) { c_put_str(TERM_WHITE, "+", 6 + 14, 15 + 19 + x_offset); header_color[0][19 + 14] = TERM_WHITE; } //Mega Hack: Hardcode 50% vamp as a fruit bat! Maybe incorrect for mimics? - Kurzel
				if (csheet_boni[i].cb[6] & CB7_RAUID) { c_put_str(TERM_WHITE, "+", 6 + i, 16 + 19 + x_offset); header_color[0][19 + 15] = TERM_WHITE; }
				if (csheet_boni[i].cb[6] & CB7_RAMSH) { c_put_str(TERM_YELLOW, "+", 6 + i, 17 + 19 + x_offset); header_color[0][19 + 16] = TERM_YELLOW; }
				// AMFI (Numerical) goes here at [1][17] - Kurzel
				if (csheet_boni[i].cb[6] & CB7_RAGGR) { c_put_str(TERM_RED, "+", 6 + i, 19 + 19 + x_offset); header_color[0][19 + 18] = TERM_RED; }

				/* Numerical Boni (WHITE if affected at, GOLD if sustained or maxxed (important), at a glance - Kurzel */
				if (csheet_boni[i].spd != 0) {
					header_color[0][19 * 2 + 0] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].spd > 9) color = TERM_GREEN;
					if (csheet_boni[i].spd > 19) color = TERM_L_UMBER; //high form/skills
					if (csheet_boni[i].spd < 0) color = TERM_L_RED;
					if (csheet_boni[i].spd < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].spd) % 10);
					if (csheet_boni[i].spd > 29) sprintf(tmp, "*");
					c_put_str(color, tmp, 6 + i, 1 + 19 * 2 + x_offset);
				}
				if (csheet_boni[i].slth != 0) {
					header_color[0][19 * 2 + 1] = TERM_WHITE;
					if (p_ptr->skill_stl >= 30) header_color[0][19 * 2 + 1] = TERM_L_UMBER;
					color = TERM_L_GREEN;
					if (csheet_boni[i].slth > 9) color = TERM_GREEN;
					if (csheet_boni[i].slth > 19) color = TERM_L_UMBER; //admin items
					if (csheet_boni[i].slth < 0) color = TERM_L_RED;
					if (csheet_boni[i].slth < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].slth) % 10);
					if (csheet_boni[i].slth > 29) sprintf(tmp, "*");
					c_put_str(color, tmp, 6 + i, 2 + 19 * 2 + x_offset);
				}
				if (csheet_boni[i].srch != 0) {
					header_color[0][19 * 2 + 2] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].srch > 9) color = TERM_GREEN;
					if (csheet_boni[i].srch > 19) color = TERM_L_UMBER; //admin items
					if (csheet_boni[i].srch < 0) color = TERM_L_RED;
					if (csheet_boni[i].srch < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].srch) % 10);
					if (csheet_boni[i].srch > 29) sprintf(tmp, "*");
					c_put_str(color, tmp, 6 + i, 3 + 19 * 2 + x_offset);
				}
				if (csheet_boni[i].infr != 0) {
					header_color[0][19 * 2 + 3] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].infr > 9) color = TERM_GREEN;
					if (csheet_boni[i].infr > 19) color = TERM_L_UMBER; //admin items
					if (csheet_boni[i].infr < 0) color = TERM_L_RED;
					if (csheet_boni[i].infr < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].infr) % 10);
					if (csheet_boni[i].infr > 29) sprintf(tmp, "*");
					c_put_str(color, tmp, 6 + i, 4 + 19 * 2 + x_offset);
				}
				if (csheet_boni[i].lite != 0) {
					header_color[0][19 * 2 + 4] = TERM_WHITE;
					color = TERM_WHITE;
					if (csheet_boni[i].cb[12] & CB13_XLITE) { color = TERM_L_UMBER; header_color[0][19 * 2 + 4] = TERM_L_UMBER; }
					sprintf(tmp, "%d", abs(csheet_boni[i].lite) % 10);
					if (csheet_boni[i].lite > 9) sprintf(tmp, "*");
					c_put_str(color, tmp, 6 + i, 5 + 19 * 2 + x_offset);
				}
				if (csheet_boni[i].dig != 0) {
					header_color[0][19 * 2 + 5] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].dig > 9) color = TERM_GREEN;
					if (csheet_boni[i].dig > 19) color = TERM_L_UMBER; //admin items
					if (csheet_boni[i].dig < 0) color = TERM_L_RED;
					if (csheet_boni[i].dig < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].dig) % 10);
					if (csheet_boni[i].dig > 29 || (csheet_boni[i].cb[12] & CB13_XWALL)) sprintf(tmp, "*");
					if (csheet_boni[i].cb[12] & CB13_XWALL) color = TERM_L_UMBER;
					c_put_str(color, tmp, 6 + i, 6 + 19 * 2 + x_offset);
				}
				if (csheet_boni[i].blow != 0) {
					header_color[0][19 * 2 + 6] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].blow > 9) color = TERM_GREEN;
					if (csheet_boni[i].blow < 0) color = TERM_L_RED;
					if (csheet_boni[i].blow < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].blow) % 10);
					if (csheet_boni[i].blow > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 6 + i, 7 + 19 * 2 + x_offset);
				}
				if (csheet_boni[i].crit != 0) {
					header_color[0][19 * 2 + 7] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].crit > 9) color = TERM_GREEN;
					if (csheet_boni[i].crit < 0) color = TERM_L_RED;
					if (csheet_boni[i].crit < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].crit) % 10);
					if (csheet_boni[i].crit > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 6 + i, 8 + 19 * 2 + x_offset);
				}
				if (csheet_boni[i].shot != 0) {
					header_color[0][19 * 2 + 8] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].shot > 9) color = TERM_GREEN;
					if (csheet_boni[i].shot < 0) color = TERM_L_RED;
					if (csheet_boni[i].shot < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].shot) % 10);
					if (csheet_boni[i].shot > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 6 + i, 9 + 19 * 2 + x_offset);
				}
				if (csheet_boni[i].migh != 0) {
					header_color[0][19 * 2 + 9] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].migh > 9) color = TERM_GREEN;
					if (csheet_boni[i].migh < 0) color = TERM_L_RED;
					if (csheet_boni[i].migh < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].migh) % 10);
					if (csheet_boni[i].migh > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 6 + i, 10 + 19 * 2 + x_offset);
				}
				if (csheet_boni[i].mxhp != 0) {
					header_color[0][19 * 2 + 10] = TERM_WHITE;
					life_sum += csheet_boni[i].mxhp;
					if (life_sum >= 3) header_color[0][19 * 2 + 10] = TERM_L_UMBER; //max
					color = TERM_L_GREEN;
					if (csheet_boni[i].mxhp >= 3) color = TERM_L_UMBER;
					else if (csheet_boni[i].mxhp < 0) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].mxhp) % 10);
					if (csheet_boni[i].mxhp > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 6 + i, 11 + 19 * 2 + x_offset);
				}
				if (csheet_boni[i].mxmp != 0) {
					header_color[0][19 * 2 + 11] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].mxmp > 9) color = TERM_GREEN;
					if (csheet_boni[i].mxmp < 0) color = TERM_L_RED;
					if (csheet_boni[i].mxmp < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].mxmp) % 10);
					if (csheet_boni[i].mxmp > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 6 + i, 12 + 19 * 2 + x_offset);
				}
				if (csheet_boni[i].luck != 0) {
					luck_sum += csheet_boni[i].luck;
					header_color[0][19 * 2 + 12] = TERM_WHITE;
					if (luck_sum >= 40) header_color[0][19 * 2 + 12] = TERM_L_UMBER; //max
					color = TERM_L_GREEN;
					if (csheet_boni[i].luck > 9) color = TERM_GREEN;
					if (csheet_boni[i].luck > 19) color = TERM_L_UMBER; //high luck sets
					if (csheet_boni[i].luck < 0) color = TERM_L_RED;
					if (csheet_boni[i].luck < -9) color = TERM_RED;
					sprintf(tmp, "%d", abs(csheet_boni[i].luck) % 10);
					if (csheet_boni[i].mxmp > 29) sprintf(tmp, "*");
					c_put_str(color, tmp, 6 + i, 13 + 19 * 2 + x_offset);
				}
				if (csheet_boni[i].pstr != 0) {
					if (header_color[0][19 * 2 + 13] == TERM_L_DARK) header_color[0][19 * 2 + 13] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].cb[11] & CB12_RSSTR) { color = TERM_L_UMBER; header_color[0][19 * 2 + 13] = TERM_L_UMBER; }
					if (csheet_boni[i].pstr > 9) { color = TERM_GREEN; if (csheet_boni[i].cb[11] & CB12_RSSTR) color = TERM_UMBER; }
					if (csheet_boni[i].pstr < 0) { color = TERM_L_RED; if (csheet_boni[i].cb[11] & CB12_RSSTR) color = TERM_YELLOW; }
					if (csheet_boni[i].pstr < -9) { color = TERM_RED; if (csheet_boni[i].cb[11] & CB12_RSSTR) color = TERM_ORANGE; }
					sprintf(tmp, "%d", abs(csheet_boni[i].pstr) % 10);
					if (csheet_boni[i].pstr > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 6 + i, 14 + 19 * 2 + x_offset);
				} else if (csheet_boni[i].cb[11] & CB12_RSSTR) { c_put_str(TERM_L_UMBER, "s", 6 + i, 14 + 19 * 2 + x_offset); header_color[0][19 * 2 + 13] = TERM_L_UMBER; }
				if (csheet_boni[i].pint != 0) {
					if (header_color[0][19 * 2 + 14] == TERM_L_DARK) header_color[0][19 * 2 + 14] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].cb[11] & CB12_RSINT) { color = TERM_L_UMBER; header_color[0][19 * 2 + 14] = TERM_L_UMBER; }
					if (csheet_boni[i].pint > 9) { color = TERM_GREEN; if (csheet_boni[i].cb[11] & CB12_RSINT) color = TERM_UMBER; }
					if (csheet_boni[i].pint < 0) { color = TERM_L_RED; if (csheet_boni[i].cb[11] & CB12_RSINT) color = TERM_YELLOW; }
					if (csheet_boni[i].pint < -9) { color = TERM_RED; if (csheet_boni[i].cb[11] & CB12_RSINT) color = TERM_ORANGE; }
					sprintf(tmp, "%d", abs(csheet_boni[i].pint) % 10);
					if (csheet_boni[i].pint > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 6 + i, 15 + 19 * 2 + x_offset);
				} else if (csheet_boni[i].cb[11] & CB12_RSINT) { c_put_str(TERM_L_UMBER, "s", 6 + i, 15 + 19 * 2 + x_offset); header_color[0][19 * 2 + 14] = TERM_L_UMBER; }
				if (csheet_boni[i].pwis != 0) {
					if (header_color[0][19 * 2 + 15] == TERM_L_DARK) header_color[0][19 * 2 + 15] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].cb[11] & CB12_RSWIS) { color = TERM_L_UMBER; header_color[0][19 * 2 + 15] = TERM_L_UMBER; }
					if (csheet_boni[i].pwis > 9) { color = TERM_GREEN; if (csheet_boni[i].cb[11] & CB12_RSWIS) color = TERM_UMBER; }
					if (csheet_boni[i].pwis < 0) { color = TERM_L_RED; if (csheet_boni[i].cb[11] & CB12_RSWIS) color = TERM_YELLOW; }
					if (csheet_boni[i].pwis < -9) { color = TERM_RED; if (csheet_boni[i].cb[11] & CB12_RSWIS) color = TERM_ORANGE; }
					sprintf(tmp, "%d", abs(csheet_boni[i].pwis) % 10);
					if (csheet_boni[i].pwis > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 6 + i, 16 + 19 * 2 + x_offset);
				} else if (csheet_boni[i].cb[11] & CB12_RSWIS) { c_put_str(TERM_L_UMBER, "s", 6 + i, 16 + 19 * 2 + x_offset); header_color[0][19 * 2 + 15] = TERM_L_UMBER; }
				if (csheet_boni[i].pdex != 0) {
					if (header_color[0][19 * 2 + 16] == TERM_L_DARK) header_color[0][19 * 2 + 16] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].cb[11] & CB12_RSDEX) { color = TERM_L_UMBER; header_color[0][19 * 2 + 16] = TERM_L_UMBER; }
					if (csheet_boni[i].pdex > 9) { color = TERM_GREEN; if (csheet_boni[i].cb[11] & CB12_RSDEX) color = TERM_UMBER; }
					if (csheet_boni[i].pdex < 0) { color = TERM_L_RED; if (csheet_boni[i].cb[11] & CB12_RSDEX) color = TERM_YELLOW; }
					if (csheet_boni[i].pdex < -9) { color = TERM_RED; if (csheet_boni[i].cb[11] & CB12_RSDEX) color = TERM_ORANGE; }
					sprintf(tmp, "%d", abs(csheet_boni[i].pdex) % 10);
					if (csheet_boni[i].pdex > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 6 + i, 17 + 19 * 2 + x_offset);
				} else if (csheet_boni[i].cb[11] & CB12_RSDEX) { c_put_str(TERM_L_UMBER, "s", 6 + i, 17 + 19 * 2 + x_offset); header_color[0][19 * 2 + 16] = TERM_L_UMBER; }
				if (csheet_boni[i].pcon != 0) {
					if (header_color[0][19 * 2 + 17] == TERM_L_DARK) header_color[0][19 * 2 + 17] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].cb[11] & CB12_RSCON) { color = TERM_L_UMBER; header_color[0][19 * 2 + 17] = TERM_L_UMBER; }
					if (csheet_boni[i].pcon > 9) { color = TERM_GREEN; if (csheet_boni[i].cb[11] & CB12_RSCON) color = TERM_UMBER; }
					if (csheet_boni[i].pcon < 0) { color = TERM_L_RED; if (csheet_boni[i].cb[11] & CB12_RSCON) color = TERM_YELLOW; }
					if (csheet_boni[i].pcon < -9) { color = TERM_RED; if (csheet_boni[i].cb[11] & CB12_RSCON) color = TERM_ORANGE; }
					sprintf(tmp, "%d", abs(csheet_boni[i].pcon) % 10);
					if (csheet_boni[i].pcon > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 6 + i, 18 + 19 * 2 + x_offset);
				} else if (csheet_boni[i].cb[11] & CB12_RSCON) { c_put_str(TERM_L_UMBER, "s", 6 + i, 18 + 19 * 2 + x_offset); header_color[0][19 * 2 + 17] = TERM_L_UMBER; }
				if (csheet_boni[i].pchr != 0) {
					if (header_color[0][19 * 2 + 18] == TERM_L_DARK) header_color[0][19 * 2 + 18] = TERM_WHITE;
					color = TERM_L_GREEN;
					if (csheet_boni[i].cb[11] & CB12_RSCHR) { color = TERM_L_UMBER; header_color[0][19 * 2 + 18] = TERM_L_UMBER; }
					if (csheet_boni[i].pchr > 9) { color = TERM_GREEN; if (csheet_boni[i].cb[11] & CB12_RSCHR) color = TERM_UMBER; }
					if (csheet_boni[i].pchr < 0) { color = TERM_L_RED; if (csheet_boni[i].cb[11] & CB12_RSCHR) color = TERM_YELLOW; }
					if (csheet_boni[i].pchr < -9) { color = TERM_RED; if (csheet_boni[i].cb[11] & CB12_RSCHR) color = TERM_ORANGE; }
					sprintf(tmp, "%d", abs(csheet_boni[i].pchr) % 10);
					if (csheet_boni[i].pchr > 19) sprintf(tmp, "*");
					c_put_str(color, tmp, 6 + i, 19 + 19 * 2 + x_offset);
				} else if (csheet_boni[i].cb[11] & CB12_RSCHR) { c_put_str(TERM_L_UMBER, "s", 6 + i, 19 + 19 * 2 + x_offset); header_color[0][19 * 2 + 18] = TERM_L_UMBER; }

				/* Track AM Field in a similar fashion */
				if (csheet_boni[i].amfi != 0) {
					if (i == 0) amfi1 = csheet_boni[i].amfi;
					else if (i == 1) amfi2 = csheet_boni[i].amfi;
					else amfi_sum += csheet_boni[i].amfi;
					header_color[0][19 + 17] = TERM_WHITE;

					/* item? (dark sword only) */
					if (i != 14) {
						if (csheet_boni[i].amfi >= 30) color = TERM_L_UMBER; //show max on darkswords, not on form/skill column though
						else color = TERM_L_GREEN;
					}
					else if (csheet_boni[i].amfi >= ANTIMAGIC_CAP) color = TERM_L_UMBER; //max AM skill, or form+skill combo
					else if ((csheet_boni[i].amfi / 10) >= 5) color = TERM_GREEN; //max AM skill, or form+skill combo
					else color = TERM_L_GREEN;

					sprintf(tmp, "%d", csheet_boni[i].amfi / 10);
					c_put_str(color, tmp, 6 + i, 18 + 19);
				}

				/* ESP and slay flags, white for anything */
				if (csheet_boni[i].cb[7] & CB8_ESPID) { c_put_str(TERM_L_BLUE, "E", 6 + i, 1 + 19 * 3 + x_offset); header_color[0][19 * 3 + 0] = TERM_WHITE; }
				if (csheet_boni[i].cb[7] & CB8_EANIM) { c_put_str(TERM_L_BLUE, "E", 6 + i, 2 + 19 * 3 + x_offset); header_color[0][19 * 3 + 1] = TERM_WHITE; }
				if (csheet_boni[i].cb[7] & CB8_SANIM) { c_put_str(TERM_YELLOW, "s", 6 + i, 2 + 19 * 3 + x_offset); header_color[0][19 * 3 + 1] = TERM_WHITE; }
				if ((csheet_boni[i].cb[7] & CB8_EANIM) && (csheet_boni[i].cb[7] & CB8_SANIM)) c_put_str(TERM_WHITE, "+", 6 + i, 2 + 19 * 3 + x_offset);
				if (csheet_boni[i].cb[7] & CB8_EORCS) { c_put_str(TERM_L_BLUE, "E", 6 + i, 3 + 19 * 3 + x_offset); header_color[0][19 * 3 + 2] = TERM_WHITE; }
				if (csheet_boni[i].cb[7] & CB8_SORCS) { c_put_str(TERM_YELLOW, "s", 6 + i, 3 + 19 * 3 + x_offset); header_color[0][19 * 3 + 2] = TERM_WHITE; }
				if ((csheet_boni[i].cb[7] & CB8_EORCS) && (csheet_boni[i].cb[7] & CB8_SORCS)) c_put_str(TERM_WHITE, "+", 6 + i, 3 + 19 * 3 + x_offset);
				if (csheet_boni[i].cb[7] & CB8_ETROL) { c_put_str(TERM_L_BLUE, "E", 6 + i, 4 + 19 * 3 + x_offset); header_color[0][19 * 3 + 3] = TERM_WHITE; }
				if (csheet_boni[i].cb[7] & CB8_STROL) { c_put_str(TERM_YELLOW, "s", 6 + i, 4 + 19 * 3 + x_offset); header_color[0][19 * 3 + 3] = TERM_WHITE; }
				if ((csheet_boni[i].cb[7] & CB8_ETROL) && (csheet_boni[i].cb[7] & CB8_STROL)) c_put_str(TERM_WHITE, "+", 6 + i, 4 + 19 * 3 + x_offset);
				if (csheet_boni[i].cb[7] & CB8_EGIAN) { c_put_str(TERM_L_BLUE, "E", 6 + i, 5 + 19 * 3 + x_offset); header_color[0][19 * 3 + 4] = TERM_WHITE; }

				if (csheet_boni[i].cb[8] & CB9_SGIAN) { c_put_str(TERM_YELLOW, "s", 6 + i, 5 + 19 * 3 + x_offset); header_color[0][19 * 3 + 4] = TERM_WHITE; }
				if ((csheet_boni[i].cb[7] & CB8_EGIAN) && (csheet_boni[i].cb[8] & CB9_SGIAN)) c_put_str(TERM_WHITE, "+", 6 + i, 5 + 19 * 3 + x_offset);
				if (csheet_boni[i].cb[8] & CB9_EDRGN) { c_put_str(TERM_L_BLUE, "E", 6 + i, 6 + 19 * 3 + x_offset); header_color[0][19 * 3 + 5] = TERM_WHITE; }
				if (csheet_boni[i].cb[8] & CB9_SDRGN) { c_put_str(TERM_YELLOW, "s", 6 + i, 6 + 19 * 3 + x_offset); header_color[0][19 * 3 + 5] = TERM_WHITE; }
				if (csheet_boni[i].cb[8] & CB9_KDRGN) { c_put_str(TERM_WHITE, "k", 6 + i, 6 + 19 * 3 + x_offset); header_color[0][19 * 3 + 5] = TERM_WHITE; }
				if ((csheet_boni[i].cb[8] & CB9_EDRGN) && (csheet_boni[i].cb[8] & CB9_SDRGN)) c_put_str(TERM_WHITE, "+", 6 + i, 6 + 19 * 3 + x_offset);
				if ((csheet_boni[i].cb[8] & CB9_EDRGN) && (csheet_boni[i].cb[8] & CB9_KDRGN)) c_put_str(TERM_WHITE, "*", 6 + i, 6 + 19 * 3 + x_offset);
				if (csheet_boni[i].cb[8] & CB9_EDEMN) { c_put_str(TERM_L_BLUE, "E", 6 + i, 7 + 19 * 3 + x_offset); header_color[0][19 * 3 + 6] = TERM_WHITE; }
				if (csheet_boni[i].cb[8] & CB9_SDEMN) { c_put_str(TERM_YELLOW, "s", 6 + i, 7 + 19 * 3 + x_offset); header_color[0][19 * 3 + 6] = TERM_WHITE; }
				if (csheet_boni[i].cb[8] & CB9_KDEMN) { c_put_str(TERM_WHITE, "k", 6 + i, 7 + 19 * 3 + x_offset); header_color[0][19 * 3 + 6] = TERM_WHITE; }
				if ((csheet_boni[i].cb[8] & CB9_EDEMN) && (csheet_boni[i].cb[8] & CB9_SDEMN)) c_put_str(TERM_WHITE, "+", 6 + i, 7 + 19 * 3 + x_offset);
				if ((csheet_boni[i].cb[8] & CB9_EDEMN) && (csheet_boni[i].cb[8] & CB9_KDEMN)) c_put_str(TERM_WHITE, "*", 6 + i, 7 + 19 * 3 + x_offset);
				if (csheet_boni[i].cb[8] & CB9_EUNDD) { c_put_str(TERM_L_BLUE, "E", 6 + i, 8 + 19 * 3 + x_offset); header_color[0][19 * 3 + 7] = TERM_WHITE; }

				if (csheet_boni[i].cb[9] & CB10_SUNDD) { c_put_str(TERM_YELLOW, "s", 6 + i, 8 + 19 * 3 + x_offset); header_color[0][19 * 3 + 7] = TERM_WHITE; }
				if (csheet_boni[i].cb[9] & CB10_KUNDD) { c_put_str(TERM_WHITE, "k", 6 + i, 8 + 19 * 3 + x_offset); header_color[0][19 * 3 + 7] = TERM_WHITE; }
				if ((csheet_boni[i].cb[8] & CB9_EUNDD) && (csheet_boni[i].cb[9] & CB10_SUNDD)) c_put_str(TERM_WHITE, "+", 6 + i, 8 + 19 * 3 + x_offset);
				if ((csheet_boni[i].cb[8] & CB9_EUNDD) && (csheet_boni[i].cb[9] & CB10_KUNDD)) c_put_str(TERM_WHITE, "*", 6 + i, 8 + 19 * 3 + x_offset);
				if (csheet_boni[i].cb[9] & CB10_EEVIL) { c_put_str(TERM_L_BLUE, "E", 6 + i, 9 + 19 * 3 + x_offset); header_color[0][19 * 3 + 8] = TERM_WHITE; }
				if (csheet_boni[i].cb[9] & CB10_SEVIL) { c_put_str(TERM_YELLOW, "s", 6 + i, 9 + 19 * 3 + x_offset); header_color[0][19 * 3 + 8] = TERM_WHITE; }
				if ((csheet_boni[i].cb[9] & CB10_EEVIL) && (csheet_boni[i].cb[9] & CB10_SEVIL)) c_put_str(TERM_WHITE, "+", 6 + i, 9 + 19 * 3 + x_offset);
				if (csheet_boni[i].cb[9] & CB10_EDGRI) { c_put_str(TERM_L_BLUE, "E", 6 + i, 10 + 19 * 3 + x_offset); header_color[0][19 * 3 + 9] = TERM_WHITE; }
				if (csheet_boni[i].cb[9] & CB10_EGOOD) { c_put_str(TERM_L_BLUE, "E", 6 + i, 11 + 19 * 3 + x_offset); header_color[0][19 * 3 + 10] = TERM_WHITE; }
				if (csheet_boni[i].cb[9] & CB10_ENONL) { c_put_str(TERM_L_BLUE, "E", 6 + i, 12 + 19 * 3 + x_offset); header_color[0][19 * 3 + 11] = TERM_WHITE; }
				if (csheet_boni[i].cb[9] & CB10_EUNIQ) { c_put_str(TERM_L_BLUE, "E", 6 + i, 13 + 19 * 3 + x_offset); header_color[0][19 * 3 + 12] = TERM_WHITE; }

				if (csheet_boni[i].cb[10] & CB11_BFIRE) { c_put_str(TERM_WHITE, "b", 6 + i, 14 + 19 * 3 + x_offset); header_color[0][19 * 3 + 13] = TERM_WHITE; }
				if (csheet_boni[i].cb[10] & CB11_AFIRE) { c_put_str(TERM_WHITE, "a", 6 + i, 14 + 19 * 3 + x_offset); header_color[0][19 * 3 + 13] = TERM_WHITE; }
				if ((csheet_boni[i].cb[10] & CB11_BFIRE) && (csheet_boni[i].cb[10] & CB11_AFIRE)) c_put_str(TERM_WHITE, "+", 6 + i, 14 + 19 * 3 + x_offset);
				if (csheet_boni[i].cb[10] & CB11_BCOLD) { c_put_str(TERM_WHITE, "b", 6 + i, 15 + 19 * 3 + x_offset); header_color[0][19 * 3 + 14] = TERM_WHITE; }
				if (csheet_boni[i].cb[10] & CB11_ACOLD) { c_put_str(TERM_WHITE, "a", 6 + i, 15 + 19 * 3 + x_offset); header_color[0][19 * 3 + 14] = TERM_WHITE; }
				if ((csheet_boni[i].cb[10] & CB11_BCOLD) && (csheet_boni[i].cb[10] & CB11_ACOLD)) c_put_str(TERM_WHITE, "+", 6 + i, 15 + 19 * 3 + x_offset);
				if (csheet_boni[i].cb[10] & CB11_BELEC) { c_put_str(TERM_WHITE, "b", 6 + i, 16 + 19 * 3 + x_offset); header_color[0][19 * 3 + 15] = TERM_WHITE; }
				if (csheet_boni[i].cb[10] & CB11_AELEC) { c_put_str(TERM_WHITE, "a", 6 + i, 16 + 19 * 3 + x_offset); header_color[0][19 * 3 + 15] = TERM_WHITE; }
				if ((csheet_boni[i].cb[10] & CB11_BELEC) && (csheet_boni[i].cb[10] & CB11_AELEC)) c_put_str(TERM_WHITE, "+", 6 + i, 16 + 19 * 3 + x_offset);
				if (csheet_boni[i].cb[10] & CB11_BACID) { c_put_str(TERM_WHITE, "b", 6 + i, 17 + 19 * 3 + x_offset); header_color[0][19 * 3 + 16] = TERM_WHITE; }
				if (csheet_boni[i].cb[10] & CB11_BPOIS) { c_put_str(TERM_WHITE, "b", 6 + i, 18 + 19 * 3 + x_offset); header_color[0][19 * 3 + 17] = TERM_WHITE; }
				if (csheet_boni[i].cb[11] & CB12_BVORP) { c_put_str(TERM_WHITE, "+", 6 + i, 19 + 19 * 3 + x_offset); header_color[0][19 * 3 + 18] = TERM_WHITE; }

				/* Footer */
				color = csheet_boni[i].color;
				symbol = (csheet_boni[i].symbol ? csheet_boni[i].symbol : ' ');
				if (symbol > MAX_FONT_CHAR) fprintf(stderr, "Sorry graphical symbols for boni not supported yet\n");
				sprintf(tmp, "%c", (char)symbol);
				for (j = 0; j < 1; j++)
					c_put_str(color, tmp, 6 + j * 20 + i, 77 + x_offset);
			}

			/* AM field - body? skill or monster form */
			if (amfi1 > amfi2) amfi_sum += amfi1;
			else amfi_sum += amfi2;
			if (amfi_sum >= ANTIMAGIC_CAP) header_color[0][19 + 17] = TERM_L_UMBER;

			/* Temporary Boni: TERM_L_BLUE for double-resist! - Kurzel */
			if (p_ptr->oppose_fire) {
				if (header_color[0][0] == TERM_WHITE) header_color[0][0] = TERM_L_BLUE;
				if (header_color[0][0] == TERM_L_DARK || header_color[0][0] == TERM_RED) header_color[0][0] = TERM_WHITE;
			}
			if (p_ptr->nimbus && p_ptr->nimbus == GF_FIRE) { header_color[0][0] = TERM_L_UMBER; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_HELLFIRE) { header_color[0][0] = TERM_L_UMBER; }
			if (p_ptr->oppose_cold) {
				if (header_color[0][1] == TERM_WHITE) header_color[0][1] = TERM_L_BLUE;
				if (header_color[0][1] == TERM_L_DARK || header_color[0][1] == TERM_RED) header_color[0][0] = TERM_WHITE;
			}
			if (p_ptr->nimbus && p_ptr->nimbus == GF_COLD) { header_color[0][1] = TERM_L_UMBER; }
			if (p_ptr->oppose_elec) {
				if (header_color[0][2] == TERM_WHITE) header_color[0][2] = TERM_L_BLUE;
				if (header_color[0][2] == TERM_L_DARK || header_color[0][2] == TERM_RED) header_color[0][0] = TERM_WHITE;
			}
			if (p_ptr->nimbus && p_ptr->nimbus == GF_ELEC) { header_color[0][2] = TERM_L_UMBER; }
			if (p_ptr->oppose_acid) {
				if (header_color[0][3] == TERM_WHITE) header_color[0][3] = TERM_L_BLUE;
				if (header_color[0][3] == TERM_L_DARK || header_color[0][3] == TERM_RED) header_color[0][0] = TERM_WHITE;
			}
			if (p_ptr->nimbus && p_ptr->nimbus == GF_ACID) { header_color[0][3] = TERM_L_UMBER; }
			if (p_ptr->oppose_pois) {
				if (header_color[0][4] == TERM_WHITE) header_color[0][4] = TERM_L_BLUE;
				if (header_color[0][4] == TERM_L_DARK || header_color[0][4] == TERM_RED) header_color[0][0] = TERM_WHITE;
			}
			if (p_ptr->nimbus && p_ptr->nimbus == GF_POIS) { header_color[0][4] = TERM_L_UMBER; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_LITE) { if (header_color[0][6] != TERM_L_UMBER) header_color[0][6] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_DARK) { if (header_color[0][7] != TERM_L_UMBER) header_color[0][7] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_GRAVITY) { if (header_color[0][8] != TERM_L_UMBER) header_color[0][8] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_SOUND) { if (header_color[0][8] != TERM_L_UMBER) header_color[0][8] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_FORCE) { if (header_color[0][8] != TERM_L_UMBER) header_color[0][8] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_SHARDS) { if (header_color[0][9] != TERM_L_UMBER) header_color[0][9] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_NEXUS) { if (header_color[0][10] != TERM_L_UMBER) header_color[0][10] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_NETHER) { if (header_color[0][11] != TERM_L_UMBER) header_color[0][11] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_CONFUSION) { if (header_color[0][12] != TERM_L_UMBER) header_color[0][12] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_CHAOS) {
				if (header_color[0][12] == TERM_L_DARK || header_color[0][12] == TERM_RED) header_color[0][12] = TERM_YELLOW;
				if (header_color[0][13] != TERM_L_UMBER) header_color[0][13] = TERM_WHITE;
			}
			if (p_ptr->nimbus && p_ptr->nimbus == GF_DISENCHANT) { if (header_color[0][14] != TERM_L_UMBER) header_color[0][14] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_WATER) { header_color[0][15] = TERM_L_UMBER; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_TIME) { if (header_color[0][16] != TERM_L_UMBER) header_color[0][16] = TERM_WHITE; }
			if (p_ptr->divine_xtra_res) { if (header_color[0][17] != TERM_L_UMBER) header_color[0][17] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_MANA) { if (header_color[0][17] != TERM_L_UMBER) header_color[0][17] = TERM_WHITE; }
			if (p_ptr->nimbus && p_ptr->nimbus == GF_INERTIA) { if (header_color[0][19 + 1] != TERM_L_UMBER) header_color[0][19 + 1] = TERM_WHITE; } // PARA
			if (p_ptr->nimbus && p_ptr->nimbus == GF_GRAVITY) { if (header_color[0][19 + 1] != TERM_L_UMBER) header_color[0][19 + 1] = TERM_WHITE; } // PARA
			if (p_ptr->nimbus && p_ptr->nimbus == GF_GRAVITY) { if (header_color[0][19 + 2] != TERM_YELLOW) header_color[0][19 + 2] = TERM_WHITE; } // TELE
			if (p_ptr->nimbus && p_ptr->nimbus == GF_GRAVITY) { if (header_color[0][19 + 7] != TERM_L_UMBER) header_color[0][19 + 7] = TERM_WHITE; } // FFAL

			/* Row Headers */
			c_put_str_vert(header_color[0][0], "Fire", (row = (row + 1) % 6), ++col + x_offset); //row, col
			c_put_str_vert(header_color[0][1], "Cold", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][2], "Elec", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][3], "Acid", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][4], "Pois", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][5], "Blnd", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][6], "Lite", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][7], "Dark", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][8], "Soun", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][9], "Shrd", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][10], "Nexu", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][11], "Neth", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][12], "Conf", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][13], "Chao", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][14], "Dise", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][15], "Watr", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][16], "Time", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][17], "Mana", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][18], "Mind", (row = (row + 1) % 6), ++col + x_offset);

			c_put_str_vert(header_color[0][19 + 0], "Fear", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 + 1], "Para", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 + 2], "HLif", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 + 3], "Tele", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 + 4], "FFal", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 + 5], "Lvtn", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 + 6], "Clmb", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 + 7], "SInv", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 + 8], "Invs", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 + 9], "Refl", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 + 10], "Wrth", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 + 11], "RgHP", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 + 12], "RgMP", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 + 13], "Food", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 + 14], "Vamp", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 + 15], "AuID", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 + 16], "AMSh", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 + 17], "AMFi", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 + 18], "Aggr", (row = (row + 1) % 6), ++col + x_offset);

			c_put_str_vert(header_color[0][19 * 2 + 0], "Spd ", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 2 + 1], "Slth", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 2 + 2], "Srch", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 2 + 3], "Infr", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 2 + 4], "Lite", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 2 + 5], "Tunn", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 2 + 6], "Blow", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 2 + 7], "Crit", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 2 + 8], "Shot", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 2 + 9], "Mght", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 2 + 10], "MxHP", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 2 + 11], "MxMP", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 2 + 12], "Luck", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 2 + 13], "STR ", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 2 + 14], "INT ", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 2 + 15], "WIS ", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 2 + 16], "DEX ", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 2 + 17], "CON ", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 2 + 18], "CHR ", (row = (row + 1) % 6), ++col + x_offset);

			c_put_str_vert(header_color[0][19 * 3 + 0], "Spid", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 3 + 1], "Anim", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 3 + 2], "Orcs", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 3 + 3], "Trol", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 3 + 4], "Gian", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 3 + 5], "Drgn", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 3 + 6], "Demn", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 3 + 7], "Undd", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 3 + 8], "Evil", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 3 + 9], "Dgri", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 3 + 10], "Good", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 3 + 11], "Nonl", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 3 + 12], "Uniq", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 3 + 13], "Fire", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 3 + 14], "Cold", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 3 + 15], "Elec", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 3 + 16], "Acid", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 3 + 17], "Pois", (row = (row + 1) % 6), ++col + x_offset);
			c_put_str_vert(header_color[0][19 * 3 + 18], "Vorp", (row = (row + 1) % 6), ++col + x_offset);
		}
	}
}

/*
 * Redraw any necessary windows
 */
void window_stuff(void) {
	/* Redraw the skills menu if requested */
	if (redraw_skills)
		do_redraw_skills();

	/* Redraw the store inventory if requested */
	if (redraw_store)
		do_redraw_store();

	/* Window stuff */
	if (!p_ptr->window) return;

	/* Display inventory */
	if (p_ptr->window & (PW_INVEN
#ifdef ENABLE_SUBINVEN
	     | PW_INVEN_SUB
#endif
	    )) {
		fix_inven();
		p_ptr->window &= ~(PW_INVEN
#ifdef ENABLE_SUBINVEN
		    | PW_INVEN_SUB
#endif
		    );
	}

#ifdef ENABLE_SUBINVEN
	/* Display inventory */
	if (p_ptr->window & PW_SUBINVEN) {
		p_ptr->window &= ~(PW_SUBINVEN);
		fix_subinven();
	}
#endif

	/* Display equipment */
	if (p_ptr->window & PW_EQUIP) {
		p_ptr->window &= (~PW_EQUIP);
		fix_equip();
	}

	/* Display player */
	if (p_ptr->window & PW_PLAYER) {
		p_ptr->window &= (~PW_PLAYER);
		fix_player();
		fix_player2();
	}

	/* Display messages */
	if (p_ptr->window & (PW_MESSAGE | PW_CHAT | PW_MSGNOCHAT)) {
		p_ptr->window &= (~(PW_MESSAGE | PW_CHAT | PW_MSGNOCHAT));
		fix_message();
	}

	/* Display the lag-o-meter */
	if (p_ptr->window & (PW_LAGOMETER)) {
		p_ptr->window &= (~PW_LAGOMETER);
		fix_lagometer();
	}
}

#define FLASH_END 20 /* duration of the flash */
/* Flash screen, special fx (not thunderstorm) */
void do_animate_screenflash(bool reset) {
	int i;
	static bool active = FALSE; /* Don't overwrite palette backup from overlapping lightning strikes */
	static byte or[BASE_PALETTE_SIZE], og[BASE_PALETTE_SIZE], ob[BASE_PALETTE_SIZE];
#ifdef ANIM_FULL_PALETTE_FLASH
	static byte or0[BASE_PALETTE_SIZE], og0[BASE_PALETTE_SIZE], ob0[BASE_PALETTE_SIZE];
#endif

	/* Prematurely end flash animation? */
	if (reset) {
		if (active) {
			if (c_cfg.palette_animation) {
				for (i = 1; i < BASE_PALETTE_SIZE; i++) set_palette(i + ANIM_OFFSET, or[i], og[i], ob[i]);
#ifdef ANIM_FULL_PALETTE_FLASH
				for (i = 1; i < BASE_PALETTE_SIZE; i++) set_palette(i + ANIM_BASE, or0[i], og0[i], ob0[i]);
#endif
				set_palette(128, 0, 0, 0); //refresh
			}
			active = FALSE;
		}
		return;
	} else if (!animate_screenflash) return;

	/* Animate palette */
	if (c_cfg.palette_animation && /* !c_cfg.disable_lightning && */
	    (!animate_screenflash_icky || animate_screenflash == FLASH_END)) /* Actually do reset colours back to normal if flash ends while screen is icky */
	switch (animate_screenflash) {
	case 1:
		/* First thing: Backup all colours before temporarily manipulating them */
		if (!active) {
			for (i = 1; i < BASE_PALETTE_SIZE; i++) get_palette(i + ANIM_OFFSET, &or[i], &og[i], &ob[i]);
#ifdef ANIM_FULL_PALETTE_FLASH
			for (i = 1; i < BASE_PALETTE_SIZE; i++) get_palette(i + ANIM_BASE, &or0[i], &og0[i], &ob0[i]);
#endif
			active = TRUE;
		}

		for (i = 1; i < BASE_PALETTE_SIZE; i++) set_palette(i + ANIM_OFFSET, 0xFF, 0xFF, 0xFF);
#ifdef ANIM_FULL_PALETTE_FLASH
		for (i = 1; i < BASE_PALETTE_SIZE; i++) set_palette(i + ANIM_BASE, 0xFF, 0xFF, 0xFF);
#endif

		set_palette(128, 0, 0, 0); //refresh
		break;
	default:
		for (i = 1; i < BASE_PALETTE_SIZE; i++)
			set_palette(i + ANIM_OFFSET,
			    or[i] + ((0xFF - or[i]) * (FLASH_END - animate_screenflash)) / (FLASH_END - 1),
			    og[i] + ((0xFF - og[i]) * (FLASH_END - animate_screenflash)) / (FLASH_END - 1),
			    ob[i] + ((0xFF - ob[i]) * (FLASH_END - animate_screenflash)) / (FLASH_END - 1));
#ifdef ANIM_FULL_PALETTE_FLASH
		for (i = 1; i < BASE_PALETTE_SIZE; i++)
			set_palette(i + ANIM_BASE,
			    or0[i] + ((0xFF - or0[i]) * (FLASH_END - animate_screenflash)) / (FLASH_END - 1),
			    og0[i] + ((0xFF - og0[i]) * (FLASH_END - animate_screenflash)) / (FLASH_END - 1),
			    ob0[i] + ((0xFF - ob0[i]) * (FLASH_END - animate_screenflash)) / (FLASH_END - 1));
#endif

		set_palette(128, 0, 0, 0); //refresh
		break;
	case FLASH_END:
		/* Restore all colours to what they were before */
		if (active) {
			for (i = 1; i < BASE_PALETTE_SIZE; i++) set_palette(i + ANIM_OFFSET, or[i], og[i], ob[i]);
#ifdef ANIM_FULL_PALETTE_FLASH
			for (i = 1; i < BASE_PALETTE_SIZE; i++) set_palette(i + ANIM_BASE, or0[i], og0[i], ob0[i]);
#endif
			set_palette(128, 0, 0, 0); //refresh
			active = FALSE;
		}
		break;
	}

	/* Proceed through steps */
	animate_screenflash++;
	if (animate_screenflash == FLASH_END + 1) animate_screenflash = 0;
}

#define AL_END 12 /* duration of the lightning flash */
#define AL_DECOUPLED /* because thunderclap happens too quickly, so decouple it from lightning animation speed */
/* Client-side thunderstorm lightning flashes, triggered by thunder_sound_idx */
void do_animate_lightning(bool reset) {
	int i;
	static bool active = FALSE; /* Don't overwrite palette backup from overlapping lightning strikes */
	static byte or[BASE_PALETTE_SIZE], og[BASE_PALETTE_SIZE], ob[BASE_PALETTE_SIZE];
#ifdef ANIM_FULL_PALETTE_LIGHTNING
	static byte or0[BASE_PALETTE_SIZE], og0[BASE_PALETTE_SIZE], ob0[BASE_PALETTE_SIZE];
#endif
#ifdef AL_DECOUPLED
	int d;
#endif

	/* Prematurely end lightning animation? */
	if (reset) {
		if (active) {
			if (c_cfg.palette_animation) {
				for (i = 1; i < BASE_PALETTE_SIZE; i++) set_palette(i + ANIM_OFFSET, or[i], og[i], ob[i]);
#ifdef ANIM_FULL_PALETTE_LIGHTNING
				for (i = 1; i < BASE_PALETTE_SIZE; i++) set_palette(i + ANIM_BASE, or0[i], og0[i], ob0[i]);
#endif
				set_palette(128, 0, 0, 0); //refresh
			}
			active = FALSE;
		}
		return;
	} else if (!animate_lightning) return;

	/* Animate palette */
	if (!c_cfg.disable_lightning && c_cfg.palette_animation &&
	    (!animate_lightning_icky || animate_lightning == AL_END)) /* Actually do reset colours back to normal if lightning ends while screen is icky */
	switch (animate_lightning) {
	case 1:
		/* First thing: Backup all colours before temporarily manipulating them */
		if (!active) {
			for (i = 1; i < BASE_PALETTE_SIZE; i++) get_palette(i + ANIM_OFFSET, &or[i], &og[i], &ob[i]);
#ifdef ANIM_FULL_PALETTE_LIGHTNING
			for (i = 1; i < BASE_PALETTE_SIZE; i++) get_palette(i + ANIM_BASE, &or0[i], &og0[i], &ob0[i]);
#endif
			active = TRUE;
		}

		//set_palette(1 + ANIM_OFFSET, 0x99, 0x99, 0x99); //white(17)
		set_palette(2 + ANIM_OFFSET, 0xCC, 0xCC, 0xFF); //slate(18)
		set_palette(4 + ANIM_OFFSET, 0xFF, 0x77, 0x88); //red(20)
		set_palette(5 + ANIM_OFFSET, 0x33, 0xFF, 0x33); //green(21)
		set_palette(6 + ANIM_OFFSET, 0x44, 0x66, 0xFF); //blue(22)
		set_palette(7 + ANIM_OFFSET, 0xAD, 0x88, 0x33); //umber(23)
		set_palette(8 + ANIM_OFFSET, 0x88, 0x88, 0x88); //ldark(24)
		set_palette(9 + ANIM_OFFSET, 0xEE, 0xEE, 0xEE); //lwhite(25)
		set_palette(13 + ANIM_OFFSET, 0xBB, 0xFF, 0xBB); //lgreen(29)
		set_palette(15 + ANIM_OFFSET, 0xF7, 0xCD, 0x85); //lumber(31)

#ifdef ANIM_FULL_PALETTE_LIGHTNING
		//set_palette(1, 0x99, 0x99, 0x99); //white(17)
		set_palette(2, 0xCC, 0xCC, 0xFF); //slate(18)
		set_palette(4, 0xFF, 0x77, 0x88); //red(20)
		set_palette(5, 0x33, 0xFF, 0x33); //green(21)
		set_palette(6, 0x44, 0x66, 0xFF); //blue(22)
		set_palette(7, 0xAD, 0x88, 0x33); //umber(23)
		set_palette(8, 0x88, 0x88, 0x88); //ldark(24)
		set_palette(9, 0xEE, 0xEE, 0xEE); //lwhite(25)
		set_palette(13, 0xBB, 0xFF, 0xBB); //lgreen(29)
		set_palette(15, 0xF7, 0xCD, 0x85); //lumber(31)
#endif

		set_palette(128, 0, 0, 0); //refresh
		break;
	case AL_END:
		/* Restore all colours to what they were before */
		if (active) {
			for (i = 1; i < BASE_PALETTE_SIZE; i++) set_palette(i + ANIM_OFFSET, or[i], og[i], ob[i]);
#ifdef ANIM_FULL_PALETTE_LIGHTNING
			for (i = 1; i < BASE_PALETTE_SIZE; i++) set_palette(i + ANIM_BASE, or0[i], og0[i], ob0[i]);
#endif
			set_palette(128, 0, 0, 0); //refresh
			active = FALSE;
		}
		break;
	default: break;
	}

	/* thunderclap follows up */
#ifndef AL_DECOUPLED
	if (animate_lightning == AL_END - animate_lightning_vol / ((100 + AL_END - 1) / AL_END)) {
#else
	d = (animate_lightning_vol >= 85) ? 100 : animate_lightning_vol + 15;
	if (animate_lightning == 101 - d) {
#endif
		if (use_sound) {
#ifndef USE_SOUND_2010
			//Term_xtra(TERM_XTRA_SOUND, ..some-sound..);
#else
			sound(thunder_sound_idx, animate_lightning_type, animate_lightning_vol, 0, 0, 0);
#endif
		}
	}

	/* Continue for a couple steps */
	animate_lightning++;
#ifndef AL_DECOUPLED
	if (animate_lightning == AL_END + 1) animate_lightning = 0;
#else
	d = AL_END > 101 - d ? AL_END : 101 - d;
	if (animate_lightning == d + 1) animate_lightning = 0;
#endif
}

/* Handle weather (rain, snow, sandstorm) client-side - C. Blue
 * Note: keep following defines in sync with nclient.c, beginning of file.
 * do_weather() is called by do_ping() which is called [at least] every frame.
 * 'no_weather': Only perform lighting-flash palette animation and play thunderclap sfx (provided those were caused by a non-weather source). */
#define WEATHER_PARTICLES_MAX	1024	/* max amount of weather particles to animate at once */
#define SKY_ALTITUDE	20 /* assumed 'pseudo-isometric' cloud altitude */
#define PANEL_X		(SCREEN_PAD_LEFT) /* physical top-left screen position of view panel */
#define PANEL_Y		(SCREEN_PAD_TOP) /* physical top-left screen position of view panel */
void do_weather(bool no_weather) {
	/* For RAINY_TOMB: Two modes: Half-height fullscreen for tombstone (!initialized), Map panel for normal gameplay (cl_initialized) */
	int panel_x = fullscreen_weather ? 0 : PANEL_X, panel_y = fullscreen_weather ? 1 : PANEL_Y; //leave out first line for message prompts (file character..)
	int temp_hgt = fullscreen_weather ? SCREEN_HGT + SCREEN_PAD_TOP + SCREEN_PAD_BOTTOM - 5 : screen_hgt; //tomb stone only is half of bigmap'd window size; -5 to leave bottom lines below the actual ascii art empty
	int temp_wid = fullscreen_weather ? MAX_WINDOW_WID : screen_wid; //tomb stone only is half of bigmap'd window size; -3 to leave bottom lines empty (looks better)

	int i, j, intensity;
	static int weather_gen_ticks = 0, weather_ticks10 = 0;
	static int weather_wind_ticks = 0, weather_speed_sand_ticks = 0, weather_speed_snow_ticks = 0, weather_speed_rain_ticks = 0; /* sub-ticks when weather really is processed */
	bool wind_west_effective = FALSE, wind_east_effective = FALSE; /* horizontal movement */
	bool gravity_effective_rain = FALSE, gravity_effective_snow = FALSE, gravity_effective_sand = FALSE; /* vertical movement */
	bool redraw = FALSE;
	int x, y, dx, dy, d; /* distance calculations (cloud centre) */
	static int cloud_movement_ticks = 0, cloud_movement_lasttick = 0;
	bool with_clouds, outside_clouds;

	bool ticks_ok = FALSE;


/* Track 10ms ticks; Also, put experimental/testing stuff here ---------------- */

	/* attempt to keep track of 'deci-ticks' (10ms resolution) */
	if (ticks10 != weather_ticks10) {
		weather_ticks10 = ticks10;
		ticks_ok = TRUE;

	/* --- experimental stuff --- */

		/* Screen flash via palette animation */
		if (animate_screenflash) do_animate_screenflash(FALSE);
		/* Lightning lighting via palette animation */
		if (animate_lightning) do_animate_lightning(FALSE);
	}
	if (no_weather) return;

/* begin ------------------------------------------------------------------- */

	/* continue animating current weather (if any) */
	if (!weather_type && !weather_elements) {
		weather_particles_seen = 0;
		return;
	}


/* perform redraw and sync hacks ------------------------------------------- */

	/* hack: instantly redraw weather without waiting for ticks and exit */
	if (weather_type >= 20000) {
		weather_type -= 20000;
		redraw = TRUE;
	}

	/* hack: reset sub-ticks to graphically synchronize horizontal
	   and vertical movement in case they use the same modulo (!) */
	if (weather_type >= 10000) {
		weather_type -= 10000;
		/* reset wind ticks and speed ticks to possibly synch them */
		weather_wind_ticks = 0;
		weather_speed_sand_ticks = weather_speed_snow_ticks = weather_speed_rain_ticks = 0;
	}

	if (redraw) {
		if (screen_icky) Term_switch(0);

		for (i = 0; i < weather_elements; i++) {
#ifdef BIGMAP_MINDLINK_HACK
			/* big_map mindlink visuals hack */
			if (last_line_y <= SCREEN_HGT &&
			    weather_element_y[i] >= weather_panel_y + SCREEN_HGT)
				;
			else
#endif
			/* only for elements within visible panel screen area */
			if (weather_element_x[i] >= weather_panel_x &&
			    weather_element_x[i] < weather_panel_x + temp_wid &&
			    weather_element_y[i] >= weather_panel_y &&
			    weather_element_y[i] < weather_panel_y + temp_hgt) {
				if (weather_element_type[i] == 1) {
					/* display raindrop */
#ifdef GRAPHICS_BG_MASK
					if (use_graphics == 2 && kidx_po_rain_char && !c_cfg.ascii_weather && !c_cfg.no2mask_weather)
						Term_draw_2mask(panel_x + weather_element_x[i] - weather_panel_x,
						    panel_y + weather_element_y[i] - weather_panel_y,
						    col_raindrop, weather_wind == 0 ? kidx_po_rain_char : (weather_wind % 2 == 1 ? ((weather_wind - 1) / 2 ? kidx_po_rain_e1_char : kidx_po_rain_e2_char) : ((weather_wind - 2) / 2 ? kidx_po_rain_w1_char : kidx_po_rain_w2_char)),
						    0, 0);
					else
#endif
#ifdef USE_GRAPHICS
					if (use_graphics && kidx_po_rain_char && !c_cfg.ascii_weather)
						Term_draw(panel_x + weather_element_x[i] - weather_panel_x,
						    panel_y + weather_element_y[i] - weather_panel_y,
						    col_raindrop, weather_wind == 0 ? kidx_po_rain_char : (weather_wind % 2 == 1 ? ((weather_wind - 1) / 2 ? kidx_po_rain_e1_char : kidx_po_rain_e2_char) : ((weather_wind - 2) / 2 ? kidx_po_rain_w1_char : kidx_po_rain_w2_char)));
					else
#endif
						Term_draw(panel_x + weather_element_x[i] - weather_panel_x,
						    panel_y + weather_element_y[i] - weather_panel_y,
						    col_raindrop, weather_wind == 0 ? '|' : (weather_wind % 2 == 1 ? '\\' : '/'));
				} else if (weather_element_type[i] == 2) {
					/* display snowflake */
#ifdef GRAPHICS_BG_MASK
					if (use_graphics == 2 && kidx_po_snow_char && !c_cfg.ascii_weather && !c_cfg.no2mask_weather)
						Term_draw_2mask(panel_x + weather_element_x[i] - weather_panel_x,
						    panel_y + weather_element_y[i] - weather_panel_y,
						    col_snowflake, kidx_po_snow_char,
						    0, 0);
					else
#endif
#ifdef USE_GRAPHICS
					if (use_graphics && kidx_po_snow_char && !c_cfg.ascii_weather)
						Term_draw(panel_x + weather_element_x[i] - weather_panel_x,
						    panel_y + weather_element_y[i] - weather_panel_y,
						    col_snowflake, kidx_po_snow_char);
					else
#endif
						Term_draw(panel_x + weather_element_x[i] - weather_panel_x,
						    panel_y + weather_element_y[i] - weather_panel_y,
						    col_snowflake, '*');
				} else if (weather_element_type[i] == 3) {
					/* display sand grain */
#ifdef GRAPHICS_BG_MASK
					if (use_graphics == 2 && kidx_po_sand_char && !c_cfg.ascii_weather && !c_cfg.no2mask_weather)
						Term_draw_2mask(panel_x + weather_element_x[i] - weather_panel_x,
						    panel_y + weather_element_y[i] - weather_panel_y,
						    col_sandgrain, kidx_po_sand_char,
						    0, 0);
					else
#endif
#ifdef USE_GRAPHICS
					if (use_graphics && kidx_po_sand_char && !c_cfg.ascii_weather)
						Term_draw(panel_x + weather_element_x[i] - weather_panel_x,
						    panel_y + weather_element_y[i] - weather_panel_y,
						    col_sandgrain, kidx_po_sand_char);
					else
#endif
						Term_draw(panel_x + weather_element_x[i] - weather_panel_x,
						    panel_y + weather_element_y[i] - weather_panel_y,
						    col_sandgrain, c_sandgrain);
				}
			}
		}

		if (screen_icky) Term_switch(0);
		/* Update the screen */
		Term_fresh();

		/* started to draw on a freshly updated panel? */
		weather_panel_changed = FALSE;
		/* return to 'reenter' regular timing */
		if (weather_elements) return;
	}


/* perform buffered cloud movement ----------------------------------------- */

	/* limited cloud movement, buffered client-side */
	if (ticks != cloud_movement_lasttick) {
		cloud_movement_lasttick = ticks;
		cloud_movement_ticks++;
		/* perform movement once per second */
		if (cloud_movement_ticks >= 10) {
			cloud_movement_ticks = 0;
			for (i = 0; i < 10; i++) {
				/* cloud exists? */
				if (cloud_x1[i] != -9999) {
					/* move a tiny bit, virtually */
					cloud_xfrac[i] += cloud_xm100[i];
					cloud_yfrac[i] += cloud_ym100[i];
					x = cloud_xfrac[i] / 100;
					y = cloud_yfrac[i] / 100;
					/* finally advanced a grid? */
					if (x) {
						cloud_x1[i] += x;
						cloud_x2[i] += x;
						cloud_xfrac[i] -= x * 100;
					}
					if (y) {
						cloud_y1[i] += y;
						cloud_y2[i] += y;
						cloud_yfrac[i] -= y * 100;
					}
				}
			}
		}
	}


/* process weather every 10ms at most -------------------------------------- */

#if 0
	/* attempt to keep track of 'deci-ticks' (10ms resolution) */
	if (ticks10 == weather_ticks10) return;
	weather_ticks10 = ticks10;
	/* note: the second limit is the frame rate, cfg.fps,
	   so if it's < 100, it will limit the speed instead. */
#else
	if (!ticks_ok) return;
#endif

/* generate new weather elements ------------------------------------------- */

	/* hack: pre-generate extra elements (usually SKY_ALTITUDE)?
	   Used if player enters an already weather-active sector */
	intensity = ((weather_type / 10) + 1);
	weather_type = weather_type % 10;

	/* weather creation: check whether elements are to be generated this time */
	weather_gen_ticks++;
	/* hack: exception for pre-generated elements (intensity > 1): insta-gen! */
	if (weather_gen_ticks >= weather_gen_speed || intensity > 1) { //note: was bugged because == seemed to skip a tick sometimes, so >= is needed. Changed it for some other timers while at it, to be safe: (***)
		/* this check might (partially) not be very important */
		if (weather_gen_ticks >= weather_gen_speed) //note: (***)
			weather_gen_ticks = 0;
		else
			intensity--;

		/* factor in received intensity */
		intensity *= weather_intensity;

		/* modifier of weather particles to generate:
		   1x for ASCII or multi-particle graphical tiles,
		   2x for single-particle graphical tiles: */
#ifdef USE_GRAPHICS
		if (use_graphics && !c_cfg.ascii_weather) intensity = (intensity * 25) / 10; // x2..3 seems best
		else
#endif
		/* Boost ASCII particle count somewhat actually, x1 seems a bit lowish? */
		intensity = (intensity * 15) / 10;

		/* create weather elements, ie rain drops, snow flakes, sand grains */
		if (weather_elements <= WEATHER_PARTICLES_MAX - intensity) {
			for (i = 0; i < intensity; i++) {
				/* NOTE: Basically same code in c-xtra1.c:do_weather(), dungeon.c:cloud_move(), wild.c:pos_in_weather() */
				/* generate random starting pos */
				x = rand_int(MAX_WID - 2) + 1;
				y = rand_int(MAX_HGT - 1 + SKY_ALTITUDE) - SKY_ALTITUDE;

				/* test pos for validity in regards to cloud */
				with_clouds = FALSE; /* assume no clouds exist */
				outside_clouds = TRUE; /* assume not within any clouds area */
				for (j = 0; j < 10; j++) {
					if (cloud_x1[j] != -9999) { /* does cloud exist? */
						/* at least one cloud restriction applies */
						with_clouds = TRUE;

						/* note: distance calc code is taken from server's distance() */
						/* Calculate distance to cloud focus point 1: */
						dy = (y > cloud_y1[j]) ? (y - cloud_y1[j]) : (cloud_y1[j] - y);
						dx = (x > cloud_x1[j]) ? (x - cloud_x1[j]) : (cloud_x1[j] - x);
						d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));
						/* Calculate distance to cloud focus point 2: */
						dy = (y > cloud_y2[j]) ? (y - cloud_y2[j]) : (cloud_y2[j] - y);
						dx = (x > cloud_x2[j]) ? (x - cloud_x2[j]) : (cloud_x2[j] - x);
						/* ..and sum them up */
						d += (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));

						/* distance within cloud? */
						if (d <= cloud_dsum[j] &&
						/* distance near cloud borders? plus, chance to thin
						   out when getting closer to the border. */
						    (rand_int(100) >= (d - ((cloud_dsum[j] * 3) / 4)) * 4))
							outside_clouds = FALSE;
					}
				}
				/* clouds apply but we're not within their areas? discard */
				if (with_clouds && outside_clouds)
				//if (outside_clouds) ---TODO: Check code for correctness!
					/* RAINY_TOMB - hack: except if in tomb screen */
					if (!fullscreen_weather)
						continue;

				/* (use pos) */
				weather_element_x[weather_elements] = x;
				weather_element_y[weather_elements] = y;

				//weather_element_type[weather_elements] = (weather_type == 3 ? (rand_int(2) ? 1 : 2) : weather_type);   <-- 3 is sandstorm now
				weather_element_type[weather_elements] = weather_type;
				weather_element_ydest[weather_elements] = y + SKY_ALTITUDE;

				/* since pos passed any checks, increase counter to acknowledge */
				weather_elements++;
			}
		}
	}


/* test for weather element movement---------------------------------------- */

	/* horizontal movement: check whether elements are to be shifted by wind this time */
	if (weather_wind) {
		weather_wind_ticks++;
		if (weather_wind % 2 == 1 && weather_wind_ticks >= ((weather_wind + 1) / 2)) { //note: (***)
			wind_west_effective = TRUE;
			weather_wind_ticks = 0;
		}
		else if (weather_wind % 2 == 0 && weather_wind_ticks >= (weather_wind / 2)) { //note: (***)
			wind_east_effective = TRUE;
			weather_wind_ticks = 0;
		}
	}
	/* vertical movement: check whether elements are to be pulled by gravity this time */
	weather_speed_rain_ticks++;
	weather_speed_snow_ticks++;
	weather_speed_sand_ticks++;

	if (weather_speed_rain_ticks >= weather_speed_rain) { //note: (***)
		gravity_effective_rain = TRUE;
		weather_speed_rain_ticks = 0;
	}
	if (weather_speed_snow_ticks >= weather_speed_snow) { //note: (***)
		gravity_effective_snow = TRUE;
		weather_speed_snow_ticks = 0;
	}
	if (weather_speed_sand_ticks >= weather_speed_sand) { //note: (***)
		gravity_effective_sand = TRUE;
		weather_speed_sand_ticks = 0;
	}

	/* nothing to do this time? - exit */
	if (!gravity_effective_rain && !gravity_effective_snow && !gravity_effective_sand && !wind_west_effective && !wind_east_effective)
		return;

/* move weather elements --------------------------------------------------- */

	weather_particles_seen = 0;

	/* display and advance currently existing weather elements */
	if (screen_icky) Term_switch(0);

	for (i = 0; i < weather_elements; i++) {
		/* restore old tile before moving the weather element */
		/* if panel view was freshly updated from server then no need */
		if (!weather_panel_changed) {
#ifdef BIGMAP_MINDLINK_HACK
			/* big_map mindlink visuals hack */
			if (last_line_y <= SCREEN_HGT &&
			    weather_element_y[i] >= weather_panel_y + SCREEN_HGT)
				;
			else
#endif
			/* only for elements within visible panel screen area */
			if (weather_element_x[i] >= weather_panel_x &&
			    weather_element_x[i] < weather_panel_x + temp_wid &&
			    weather_element_y[i] >= weather_panel_y &&
			    weather_element_y[i] < weather_panel_y + temp_hgt) {
				/* restore original grid content */
#ifdef GRAPHICS_BG_MASK
				if (use_graphics == 2 && !c_cfg.ascii_weather && !c_cfg.no2mask_weather)
					Term_draw_2mask(panel_x + weather_element_x[i] - weather_panel_x,
					    panel_y + weather_element_y[i] - weather_panel_y,
					    panel_map_a[weather_element_x[i] - weather_panel_x][weather_element_y[i] - weather_panel_y],
					    panel_map_c[weather_element_x[i] - weather_panel_x][weather_element_y[i] - weather_panel_y],
					    0, 0);
				else
#endif
				Term_draw(panel_x + weather_element_x[i] - weather_panel_x,
				    panel_y + weather_element_y[i] - weather_panel_y,
				    panel_map_a[weather_element_x[i] - weather_panel_x][weather_element_y[i] - weather_panel_y],
				    panel_map_c[weather_element_x[i] - weather_panel_x][weather_element_y[i] - weather_panel_y]);
			}
		}

#ifdef BIGMAP_MINDLINK_HACK
		/* big_map mindlink visuals hack */
		if (last_line_y <= SCREEN_HGT &&
		    weather_element_y[i] >= weather_panel_y + SCREEN_HGT)
			;
		else
#endif
		/* register weather element, if it is currently supposed to be visible on screen */
		if (weather_element_type[i] != 0 &&
		    (weather_element_x[i] >= weather_panel_x &&
		    weather_element_x[i] < weather_panel_x + temp_wid &&
		    weather_element_y[i] >= weather_panel_y &&
		    weather_element_y[i] < weather_panel_y + temp_hgt))
			weather_particles_seen++;

		/* advance raindrops */
		if (weather_element_type[i] == 1) {
			/* perform movement (y:speed, x:wind) */
			if (gravity_effective_rain) weather_element_y[i]++;
			if (wind_west_effective) weather_element_x[i]++;
			else if (wind_east_effective) weather_element_x[i]--;

			/* pac-man effect for leaving screen to the left/right */
			if (weather_element_x[i] < 1) weather_element_x[i] = MAX_WID - 2;
			else if (weather_element_x[i] > MAX_WID - 2) weather_element_x[i] = 1;

			/* left screen or reached destination? terminate it */
			if (weather_element_y[i] >= MAX_HGT - 2 ||
			    weather_element_y[i] >= weather_element_ydest[i]) {
				/* excise this effect */
				for (j = i + 1; j < weather_elements; j++) {
					weather_element_x[j - 1] = weather_element_x[j];
					weather_element_y[j - 1] = weather_element_y[j];
					weather_element_ydest[j - 1] = weather_element_ydest[j];
					weather_element_type[j - 1] = weather_element_type[j];
				}
				weather_elements--;
				i--;
			}
#ifdef BIGMAP_MINDLINK_HACK
			/* big_map mindlink visuals hack */
			else if (last_line_y <= SCREEN_HGT &&
			    weather_element_y[i] >= weather_panel_y + SCREEN_HGT)
				;
#endif
			/* only for elements within visible panel screen area */
			else if (weather_element_x[i] >= weather_panel_x &&
			    weather_element_x[i] < weather_panel_x + temp_wid &&
			    weather_element_y[i] >= weather_panel_y &&
			    weather_element_y[i] < weather_panel_y + temp_hgt) {
				/* display raindrop */
#ifdef GRAPHICS_BG_MASK
				if (use_graphics == 2 && kidx_po_rain_char && !c_cfg.ascii_weather && !c_cfg.no2mask_weather)
					Term_draw_2mask(panel_x + weather_element_x[i] - weather_panel_x,
					    panel_y + weather_element_y[i] - weather_panel_y,
					    col_raindrop, weather_wind == 0 ? kidx_po_rain_char : (weather_wind % 2 == 1 ? ((weather_wind - 1) / 2 ? kidx_po_rain_e1_char : kidx_po_rain_e2_char) : ((weather_wind - 2) / 2 ? kidx_po_rain_w1_char : kidx_po_rain_w2_char)),
					    0, 0);
				else
#endif
#ifdef USE_GRAPHICS
				if (use_graphics && kidx_po_rain_char && !c_cfg.ascii_weather)
					Term_draw(panel_x + weather_element_x[i] - weather_panel_x,
					    panel_y + weather_element_y[i] - weather_panel_y,
					    col_raindrop, weather_wind == 0 ? kidx_po_rain_char : (weather_wind % 2 == 1 ? ((weather_wind - 1) / 2 ? kidx_po_rain_e1_char : kidx_po_rain_e2_char) : ((weather_wind - 2) / 2 ? kidx_po_rain_w1_char : kidx_po_rain_w2_char)));
				else
#endif
					Term_draw(panel_x + weather_element_x[i] - weather_panel_x,
					    panel_y + weather_element_y[i] - weather_panel_y,
					    col_raindrop, weather_wind == 0 ? '|' : (weather_wind % 2 == 1 ? '\\' : '/'));
			}
		}
		/* advance snowflakes - falling slowly (assumed weather_speed isn't
		   set to silyl value 1 - well, maybe that could be a 'hail storm') */
		else if (weather_element_type[i] == 2) {
			/* perform movement (y:speed, x:wind) */
			if (gravity_effective_snow) weather_element_y[i]++;
			if (wind_west_effective && !rand_int(weather_wind)) weather_element_x[i]++;
			else if (wind_east_effective && !rand_int(weather_wind)) weather_element_x[i]--;

			/* pac-man effect for leaving screen to the left/right */
			if (weather_element_x[i] < 1) weather_element_x[i] = MAX_WID - 2;
			else if (weather_element_x[i] > MAX_WID - 2) weather_element_x[i] = 1;

			/* left screen or reached destination? terminate it */
			if (weather_element_y[i] > MAX_HGT - 2 ||
			    weather_element_y[i] > weather_element_ydest[i]) {
				/* excise this effect */
				for (j = i + 1; j < weather_elements; j++) {
					weather_element_x[j - 1] = weather_element_x[j];
					weather_element_y[j - 1] = weather_element_y[j];
					weather_element_ydest[j - 1] = weather_element_ydest[j];
					weather_element_type[j - 1] = weather_element_type[j];
				}
				weather_elements--;
				i--;
			}
#ifdef BIGMAP_MINDLINK_HACK
			/* big_map mindlink visuals hack */
			else if (last_line_y <= SCREEN_HGT &&
			    weather_element_y[i] >= weather_panel_y + SCREEN_HGT)
				;
#endif
			/* only for elements within visible panel screen area */
			else if (weather_element_x[i] >= weather_panel_x &&
			    weather_element_x[i] < weather_panel_x + temp_wid &&
			    weather_element_y[i] >= weather_panel_y &&
			    weather_element_y[i] < weather_panel_y + temp_hgt) {
				/* display snowflake */
#ifdef GRAPHICS_BG_MASK
				if (use_graphics == 2 && kidx_po_snow_char && !c_cfg.ascii_weather && !c_cfg.no2mask_weather)
					Term_draw_2mask(panel_x + weather_element_x[i] - weather_panel_x,
					    panel_y + weather_element_y[i] - weather_panel_y,
					    col_snowflake, kidx_po_snow_char,
					    0, 0);
				else
#endif
#ifdef USE_GRAPHICS
				if (use_graphics && kidx_po_snow_char && !c_cfg.ascii_weather)
					Term_draw(panel_x + weather_element_x[i] - weather_panel_x,
					    panel_y + weather_element_y[i] - weather_panel_y,
					    col_snowflake, kidx_po_snow_char);
				else
#endif
					Term_draw(panel_x + weather_element_x[i] - weather_panel_x,
					    panel_y + weather_element_y[i] - weather_panel_y,
					    col_snowflake, '*');
			}
		}
		/* advance sand grain - weather_speed should be high always for a sand storm */
		else if (weather_element_type[i] == 3) {
			/* perform movement (y:speed, x:wind) */
			if (gravity_effective_sand) weather_element_y[i]++;
			if (wind_west_effective && !rand_int(weather_wind)) weather_element_x[i]++;
			else if (wind_east_effective && !rand_int(weather_wind)) weather_element_x[i]--;

			/* pac-man effect for leaving screen to the left/right */
			if (weather_element_x[i] < 1) weather_element_x[i] = MAX_WID - 2;
			else if (weather_element_x[i] > MAX_WID - 2) weather_element_x[i] = 1;

			/* left screen or reached destination? terminate it */
			if (weather_element_y[i] > MAX_HGT - 2 ||
			    weather_element_y[i] > weather_element_ydest[i]) {
				/* excise this effect */
				for (j = i + 1; j < weather_elements; j++) {
					weather_element_x[j - 1] = weather_element_x[j];
					weather_element_y[j - 1] = weather_element_y[j];
					weather_element_ydest[j - 1] = weather_element_ydest[j];
					weather_element_type[j - 1] = weather_element_type[j];
				}
				weather_elements--;
				i--;
			}
#ifdef BIGMAP_MINDLINK_HACK
			/* big_map mindlink visuals hack */
			else if (last_line_y <= SCREEN_HGT &&
			    weather_element_y[i] >= weather_panel_y + SCREEN_HGT)
				;
#endif
			/* only for elements within visible panel screen area */
			else if (weather_element_x[i] >= weather_panel_x &&
			    weather_element_x[i] < weather_panel_x + temp_wid &&
			    weather_element_y[i] >= weather_panel_y &&
			    weather_element_y[i] < weather_panel_y + temp_hgt) {
				/* display sand grain */
#ifdef GRAPHICS_BG_MASK
				if (use_graphics == 2 && kidx_po_sand_char && !c_cfg.ascii_weather && !c_cfg.no2mask_weather)
					Term_draw_2mask(panel_x + weather_element_x[i] - weather_panel_x,
					    panel_y + weather_element_y[i] - weather_panel_y,
					    col_sandgrain, kidx_po_sand_char,
					    0, 0);
				else
#endif
#ifdef USE_GRAPHICS
				if (use_graphics && kidx_po_sand_char && !c_cfg.ascii_weather)
					Term_draw(panel_x + weather_element_x[i] - weather_panel_x,
					    panel_y + weather_element_y[i] - weather_panel_y,
					    col_sandgrain, kidx_po_sand_char);
				else
#endif
					Term_draw(panel_x + weather_element_x[i] - weather_panel_x,
					    panel_y + weather_element_y[i] - weather_panel_y,
					    col_sandgrain, c_sandgrain);
			}
		}
	}

	if (screen_icky) Term_switch(0);
	/* Update the screen */
	Term_fresh();


/* clean up and exit ------------------------------------------------------- */

	/* started to draw on a freshly updated panel? */
	weather_panel_changed = FALSE;
}
