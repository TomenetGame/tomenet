/* $Id$ */
/* File: cave.c */

/* Purpose: low level dungeon routines -BEN- */

#define SERVER

#include "angband.h"

/*
 * monsters with 'RF1_ATTR_MULTI' uses colour according to their
 * breath if it is on. (possible bottleneck, tho)
 */
#define MULTI_HUED_PROPER

/* View_shade_floor really shades all floor types, not just FEAT_FLOOR.
   For this, the floor feats must have SPECIAL_LITE flag (or LAMP_LITE).
   This usually doesn't look as cool as you might expect. - C. Blue
   Note: This overrides NO_SHADE flag. */
//#define SHADE_ALL_FLOOR

/* Don't shade self-illuminated grids that are out of view, if they are on the
   world surface and it's night time.
   Primary idea: Shop entrance areas don't get partially shaded with slate tone
   when player circles around them. It looked a little bit odd maybe. */
#define DONT_SHADE_GLOW_AT_NIGHT

/*
 * Scans for cave_type array pointer.
 * Returns cave array relative to the dimensions
 * specified in the arguments. Returns NULL for
 * a failure.
 */
cave_type **getcave(struct worldpos *wpos)
{
	struct wilderness_type *wild;
	wild = &wild_info[wpos->wy][wpos->wx];
	if (wpos->wx > MAX_WILD_X || wpos->wx < 0 || wpos->wy > MAX_WILD_Y || wpos->wy < 0) return(NULL);
	if (wpos->wz == 0) {
		return(wild->cave);
	} else {
		if (wpos->wz > 0) {
			if (wild->tower && wpos->wz <= wild->tower->maxdepth) {
				return(wild->tower->level[wpos->wz-1].cave);
			}
		}
		else if (wild->dungeon && wpos->wz >= -wild->dungeon->maxdepth)
			return(wild->dungeon->level[ABS(wpos->wz)-1].cave);
	}
	return ((cave_type **)NULL);
}

/* an afterthought - it is often needed without up/down info */
struct dungeon_type *getdungeon(struct worldpos *wpos)
{
	struct wilderness_type *wild;
	wild = &wild_info[wpos->wy][wpos->wx];
	if (wpos->wz == 0) return NULL;
	else {
		if ((wpos->wz > 0) && !wild->tower) return NULL;
		if ((wpos->wz < 0) && !wild->dungeon) return NULL;
		return(wpos->wz > 0 ? wild->tower : wild->dungeon);
	}
}

/* another afterthought - it is often needed without up/down info */
struct dun_level *getfloor(struct worldpos *wpos)
{
	struct wilderness_type *wild;
	wild = &wild_info[wpos->wy][wpos->wx];
	if (wpos->wz == 0) {
/*		return(wild); */
		return(NULL);
	} else {
		if (wpos->wz > 0)
			return(&wild->tower->level[wpos->wz - 1]);
		else
			return(&wild->dungeon->level[ABS(wpos->wz) - 1]);
	}
}

void new_level_up_x(struct worldpos *wpos, int pos)
{
	struct wilderness_type *wild;
	wild = &wild_info[wpos->wy][wpos->wx];
	if (wpos->wz == 0) wild->up_x = pos;
	else if (wpos->wz > 0)
		wild->tower->level[wpos->wz - 1].up_x = pos;
	else
		wild->dungeon->level[ABS(wpos->wz) - 1].up_x = pos;
}
void new_level_up_y(struct worldpos *wpos, int pos)
{
	struct wilderness_type *wild;
	wild = &wild_info[wpos->wy][wpos->wx];
	if (wpos->wz == 0) wild->up_y = pos;
	else if (wpos->wz > 0)
		wild->tower->level[wpos->wz - 1].up_y = pos;
	else
		wild->dungeon->level[ABS(wpos->wz) - 1].up_y = pos;
}
void new_level_down_x(struct worldpos *wpos, int pos)
{
	struct wilderness_type *wild;
	wild = &wild_info[wpos->wy][wpos->wx];
	if (wpos->wz == 0) wild->dn_x = pos;
	else if (wpos->wz > 0)
		wild->tower->level[wpos->wz - 1].dn_x = pos;
	else
		wild->dungeon->level[ABS(wpos->wz) - 1].dn_x = pos;
}
void new_level_down_y(struct worldpos *wpos, int pos)
{
	struct wilderness_type *wild;
	wild = &wild_info[wpos->wy][wpos->wx];
	if (wpos->wz == 0) wild->dn_y = pos;
	else if (wpos->wz > 0)
		wild->tower->level[wpos->wz - 1].dn_y = pos;
	else
		wild->dungeon->level[ABS(wpos->wz) - 1].dn_y = pos;
}
void new_level_rand_x(struct worldpos *wpos, int pos)
{
	struct wilderness_type *wild;
	wild = &wild_info[wpos->wy][wpos->wx];
	if (wpos->wz == 0) wild->rn_x = pos;
	else if (wpos->wz > 0)
		wild->tower->level[wpos->wz - 1].rn_x = pos;
	else
		wild->dungeon->level[ABS(wpos->wz) - 1].rn_x = pos;
}
void new_level_rand_y(struct worldpos *wpos, int pos)
{
	struct wilderness_type *wild;
	wild = &wild_info[wpos->wy][wpos->wx];
	if (wpos->wz == 0) wild->rn_y = pos;
	else if (wpos->wz > 0)
		wild->tower->level[wpos->wz - 1].rn_y = pos;
	else
		wild->dungeon->level[ABS(wpos->wz) - 1].rn_y = pos;
}

byte level_up_x(struct worldpos *wpos)
{
	struct wilderness_type *wild;
	wild = &wild_info[wpos->wy][wpos->wx];
	if (wpos->wz == 0) return (wild->up_x);
	return (wpos->wz > 0? wild->tower->level[wpos->wz - 1].up_x : wild->dungeon->level[ABS(wpos->wz) - 1].up_x);
}
byte level_up_y(struct worldpos *wpos)
{
	struct wilderness_type *wild;
	wild = &wild_info[wpos->wy][wpos->wx];
	if (wpos->wz == 0) return (wild->up_y);
	return (wpos->wz > 0 ? wild->tower->level[wpos->wz - 1].up_y : wild->dungeon->level[ABS(wpos->wz) - 1].up_y);
}
byte level_down_x(struct worldpos *wpos)
{
	struct wilderness_type *wild;
	wild = &wild_info[wpos->wy][wpos->wx];
	if (wpos->wz == 0) return (wild->dn_x);
	return(wpos->wz > 0 ? wild->tower->level[wpos->wz - 1].dn_x : wild->dungeon->level[ABS(wpos->wz) - 1].dn_x);
}
byte level_down_y(struct worldpos *wpos)
{
	struct wilderness_type *wild;
	wild = &wild_info[wpos->wy][wpos->wx];
	if (wpos->wz == 0) return (wild->dn_y);
	return (wpos->wz > 0 ? wild->tower->level[wpos->wz - 1].dn_y : wild->dungeon->level[ABS(wpos->wz) - 1].dn_y);
}
byte level_rand_x(struct worldpos *wpos)
{
	struct wilderness_type *wild;
	wild = &wild_info[wpos->wy][wpos->wx];
	if (wpos->wz == 0) return(wild->rn_x);
	return (wpos->wz > 0 ? wild->tower->level[wpos->wz - 1].rn_x : wild->dungeon->level[ABS(wpos->wz) - 1].rn_x);
}
byte level_rand_y(struct worldpos *wpos)
{
	struct wilderness_type *wild;
	wild = &wild_info[wpos->wy][wpos->wx];
	if (wpos->wz == 0) return (wild->rn_y);
	return (wpos->wz > 0 ? wild->tower->level[wpos->wz - 1].rn_y : wild->dungeon->level[ABS(wpos->wz) - 1].rn_y);
}

static int get_staircase_colour(dungeon_type *d_ptr, byte *c) {
	/* (experimental testing stuff) */
	if (d_ptr->flags3 & (DF3_NO_TELE | DF3_NO_ESP | DF3_LIMIT_ESP | DF3_NO_SUMMON)) {
		*c = TERM_L_UMBER;
		return 0;
	}

	/* override colour from easiest to worst */
	/* joker overrides the king ;) */
	if (d_ptr->flags2 & DF2_NO_DEATH) {
		*c = TERM_GREEN;
		return -1;
	}

	if (d_ptr->flags2 & DF2_NO_EXIT_WOR) {
		if ((d_ptr->flags2 & DF2_IRON) || (d_ptr->flags1 & (DF1_FORCE_DOWN | DF1_NO_UP))) {
			*c = TERM_DARKNESS;
			return 7;
		}
	}
	if ((d_ptr->flags2 & DF2_NO_EXIT_STAIR)
	    && (d_ptr->flags2 & DF2_NO_EXIT_PROB)
	    && (d_ptr->flags2 & DF2_NO_EXIT_FLOAT)
	    && (d_ptr->flags2 & DF2_NO_EXIT_WOR)) {
		*c = TERM_DARKNESS;
		return 7;
	}

	if (d_ptr->flags2 & DF2_IRON) {
		*c = TERM_L_DARK;
		return 6;
	}
	if (d_ptr->flags2 & DF2_HELL) {
		*c = TERM_FIRE;
		return 5;
	}
	if (d_ptr->flags1 & DF1_FORCE_DOWN) {
		*c = TERM_L_RED;
		return 4;
	}
	if (d_ptr->flags1 & DF1_NO_RECALL) {
		*c = TERM_RED;
		return 3;
	}
	if (d_ptr->flags1 & DF1_NO_UP) {
		*c = TERM_ORANGE;
		return 2;
	}
	if (d_ptr->flags2 & DF2_NO_RECALL_INTO) {
		*c = TERM_YELLOW;
		return 1;
	}

	/* normal */
	*c = TERM_WHITE;
	return 0;
}

/* For staircase-placement.
   Mode: 1 stairs, (doesn't make sense? 2 wor,) (handled in cmd2.c, can't handle here actually: 4 probtravel, 8 ghostfloating) */
bool can_go_up(struct worldpos *wpos, byte mode)
{
        struct wilderness_type *wild = &wild_info[wpos->wy][wpos->wx];

        struct dungeon_type *d_ptr = wild->tower;
	if (wpos->wz < 0) d_ptr = wild->dungeon;

#if 0 /* fixed (old /update-dun killed flags2) */
	/* paranoia, but caused panic: in wilderness_gen() cmd_up({0,0,0},0x1) would return 1
	   resulting in bad up/down values for staircase generation when it attempts to locate
	   a staircase leading into pvp-arena and put some feats around it (at coords 0,0 ...).
	   Apparently, WILD_F_UP is set here, too. And even w_ptr->tower was also valid.
	   todo: fix this. -C. Blue */
	if (wpos->wz == 0 && wpos->wx == WPOS_PVPARENA_X && wpos->wy == WPOS_PVPARENA_Y &&
	    WPOS_PVPARENA_Z > 0) return (FALSE);
#endif

	/* Check for empty staircase without any connected dungeon/tower! */
	if (!d_ptr) return((wild->flags&WILD_F_UP)?TRUE:FALSE); /* you MAY create 'empty' staircase */

	if (mode & 0x1) {
		if (!wpos->wz && (d_ptr->flags2 & DF2_NO_ENTRY_STAIR)) return(FALSE);
		if (wpos->wz == -1 && (d_ptr->flags2 & DF2_NO_EXIT_STAIR)) return(FALSE);
		if (wpos->wz && (d_ptr->flags2 & DF2_NO_STAIRS_UP)) return(FALSE);
	}

	if (wpos->wz<0) {
		if ((d_ptr->flags1 & (DF1_NO_UP | DF1_FORCE_DOWN)) ||
		    (d_ptr->flags2 & DF2_IRON))
			return(FALSE);
		return(TRUE);
	}

	if (wpos->wz>0) return(wpos->wz < wild->tower->maxdepth);

	return((wild->flags&WILD_F_UP)?TRUE:FALSE);
}
/* For staircase-placement and sinking/pit traps.
   Mode: 1 stairs, (doesn't make sense? 2 wor,) (handled in cmd2.c, can't handle here actually: 4 probtravel, 8 ghostfloating) */
bool can_go_down(struct worldpos *wpos, byte mode)
{
        struct wilderness_type *wild = &wild_info[wpos->wy][wpos->wx];

        struct dungeon_type *d_ptr = wild->dungeon;
        if (wpos->wz > 0) d_ptr = wild->tower;

#if 0 /* fixed (old /update-dun killed flags2) */
	/* paranoia, but caused panic: in wilderness_gen() cmd_up({0,0,0},0x1) would return 1
	   resulting in bad up/down values for staircase generation when it attempts to locate
	   a staircase leading into pvp-arena and put some feats around it (at coords 0,0 ...).
	   Apparently, WILD_F_UP is set here, too. And even w_ptr->tower was also valid.
	   todo: fix this. -C. Blue */
	if (wpos->wz == 0 && wpos->wx == WPOS_PVPARENA_X && wpos->wy == WPOS_PVPARENA_Y &&
	    WPOS_PVPARENA_Z < 0) return (FALSE);
#endif

	/* Check for empty staircase without any connected dungeon/tower! */
	if (!d_ptr) return((wild->flags&WILD_F_DOWN)?TRUE:FALSE); /* you MAY create 'empty' staircase */

	if (mode & 0x1) {
		if (!wpos->wz && (d_ptr->flags2 & DF2_NO_ENTRY_STAIR)) return(FALSE);
		if (wpos->wz == 1 && (d_ptr->flags2 & DF2_NO_EXIT_STAIR)) return(FALSE);
		if (wpos->wz && (d_ptr->flags2 & DF2_NO_STAIRS_DOWN)) return(FALSE);
	}

	if (wpos->wz>0) {
		if ((d_ptr->flags1 & (DF1_NO_UP | DF1_FORCE_DOWN)) ||
		    (d_ptr->flags2 & DF2_IRON)) return(FALSE);
		return(TRUE);
	}

	if (wpos->wz<0) return(ABS(wpos->wz) < wild->dungeon->maxdepth);

	return((wild->flags&WILD_F_DOWN)?TRUE:FALSE);
}
/* ignore all dungeon/floor flags */
bool can_go_up_simple(struct worldpos *wpos)
{
        struct wilderness_type *wild = &wild_info[wpos->wy][wpos->wx];
	if (wpos->wz < 0) return(TRUE);
	if (wpos->wz > 0) return(wpos->wz < wild->tower->maxdepth);
	return ((wild->flags & WILD_F_UP) ? TRUE : FALSE);
}
/* ignore all dungeon/floor flags */
bool can_go_down_simple(struct worldpos *wpos)
{
        struct wilderness_type *wild = &wild_info[wpos->wy][wpos->wx];
	if (wpos->wz > 0) return(TRUE);
	if (wpos->wz < 0) return(ABS(wpos->wz) < wild->dungeon->maxdepth);
	return ((wild->flags & WILD_F_DOWN) ? TRUE : FALSE);
}

void wpcopy(struct worldpos *dest, struct worldpos *src)
{
	dest->wx = src->wx;
	dest->wy = src->wy;
	dest->wz = src->wz;
}

static void update_uniques_killed(struct worldpos *wpos)
{
	int i, j;
	player_type *p_ptr;
	dun_level *l_ptr;

	/* Update the list of uniques that have been killed by all players on the level */
	l_ptr = getfloor(wpos);

	if (l_ptr && l_ptr->uniques_killed) {
		int admins = 0, players = 0;

		for (i = 1; i <= NumPlayers; i++) {
			p_ptr = Players[i];

			/* Skip disconnected players */
			if (p_ptr->conn == NOT_CONNECTED)
				continue;

			/* Player on this depth? */
			if (!inarea(&p_ptr->wpos, wpos))
				continue;

			/* Count the admins and non-admins */
			if (admin_p(i)) admins++;
			else players++;
		}

		if (admins || players) {
			for (j = 1; j < MAX_R_IDX; j++) {
				monster_race *r_ptr = &r_info[j];

				if (r_ptr->flags1 & RF1_UNIQUE) {
					/* Assume that the unique has been killed unless a player has killed it */
					l_ptr->uniques_killed[j] = TRUE;

					for (i = 1; i <= NumPlayers; i++) {
						p_ptr = Players[i];

						/* Skip disconnected players */
						if (p_ptr->conn == NOT_CONNECTED)
							continue;

						/* Player on this depth? */
						if (!inarea(&p_ptr->wpos, wpos))
							continue;

						/* Ignore admins if players are on the level */
						if (players && admin_p(i))
							continue;

						/* Check that every player has killed the unique */
						l_ptr->uniques_killed[j] = l_ptr->uniques_killed[j] && p_ptr->r_killed[j] == 1;
					}
				}
			}
		} else {
			/* No players around, no restrictions */
			C_WIPE(l_ptr->uniques_killed, MAX_R_IDX, char);
		}
	}
}

void new_players_on_depth(struct worldpos *wpos, int value, bool inc) {
	struct wilderness_type *w_ptr;
	time_t now;

	int henc = 0, henc_top = 0, i;
	player_type *p_ptr;
	monster_type *m_ptr;

	object_type *o_ptr;
	char o_name[ONAME_LEN];

	cave_type **zcave;
	bool flag = FALSE;
	if ((zcave = getcave(wpos))) flag = TRUE;

	now = time(&now);

	w_ptr = &wild_info[wpos->wy][wpos->wx];
#if DEBUG_LEVEL > 2
		s_printf("new_players_on_depth.. %s  now:%d value:%d inc:%s\n", wpos_format(0, wpos), now, value, inc?"TRUE":"FALSE");
#endif
	if (in_valinor(wpos)) {
		for (i = 1; i <= NumPlayers; i++) {
			p_ptr = Players[i];
			if (p_ptr->conn == NOT_CONNECTED) continue;
			if (admin_p(i)) continue;
			if (inarea(&p_ptr->wpos, wpos)) s_printf("%s VALINOR: Player %s is here.\n", showtime(), p_ptr->name);
		}
	}

        /* Page all dungeon masters to notify them of a Nether Realm breach >:) - C. Blue */
	if (value > 0) {
		if (watch_nr && in_netherrealm(wpos)) {
			for (i = 1; i <= NumPlayers; i++) {
				if (Players[i]->conn == NOT_CONNECTED) continue;
				if (Players[i]->admin_dm && !Players[i]->afk) Players[i]->paging = 2;
			}
		} else if (watch_cp && wpos->wz && getdungeon(wpos)->type == DI_CLOUD_PLANES) {
			for (i = 1; i <= NumPlayers; i++) {
				if (Players[i]->conn == NOT_CONNECTED) continue;
				if (Players[i]->admin_dm && !Players[i]->afk) Players[i]->paging = 2;
			}
		}
	}

	if (wpos->wz) {
		struct dungeon_type *d_ptr;
		struct dun_level *l_ptr;
		if (wpos->wz > 0)
			d_ptr = wild_info[wpos->wy][wpos->wx].tower;
		else
			d_ptr = wild_info[wpos->wy][wpos->wx].dungeon;


		l_ptr = &d_ptr->level[ABS(wpos->wz) - 1];

		l_ptr->ondepth = (inc ? l_ptr->ondepth + value : value);
		if (l_ptr->ondepth < 0) l_ptr->ondepth = 0;

		if (!l_ptr->ondepth) l_ptr->lastused = 0;
		if (value > 0) l_ptr->lastused = now;
	} else {
		w_ptr->ondepth = (inc ? w_ptr->ondepth + value : value);
		if (w_ptr->ondepth < 0) w_ptr->ondepth = 0;
		if (!w_ptr->ondepth) w_ptr->lastused = 0;
		if (value > 0) w_ptr->lastused = now;
		/* remove 'deposited' true artefacts if last player leaves a level,
		   and if true artefacts aren't allowed to be stored (in houses for example) */
		if (!w_ptr->ondepth && cfg.anti_arts_wild) {
			for (i = 0; i < o_max; i++) {
				o_ptr = &o_list[i];
				if (o_ptr->k_idx && inarea(&o_ptr->wpos, wpos) &&
				    undepositable_artifact_p(o_ptr)) {
					object_desc(0, o_name, o_ptr, FALSE, 0);
					s_printf("WILD_ART: %s of %s erased at (%d, %d, %d)\n",
					    o_name, lookup_player_name(o_ptr->owner), o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz);
					handle_art_d(o_ptr->name1);
					if (flag && in_bounds_array(o_ptr->iy, o_ptr->ix))
						zcave[o_ptr->iy][o_ptr->ix].o_idx = 0;
					WIPE(o_ptr, object_type);
				}
			}
		}
	}

	update_uniques_killed(wpos);

	/* Perform henc_strictness anti-cheeze - mode 4 : monster is on the same dungeon level as a player */
	/* If a player enters a new level */
	if ((value <= 0) || (cfg.henc_strictness < 4)) return;
	if (in_bree(wpos)) return; /* not in Bree, because of Halloween :) */

	/* Who is the highest player around? */
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];

		/* skip disconnected players */
		if (p_ptr->conn == NOT_CONNECTED)
			continue;

		/* skip admins */
		if (admin_p(i)) continue;

		/* player on this depth? */
		if (!inarea(&p_ptr->wpos, wpos)) continue;

		if (henc < p_ptr->max_lev) henc = p_ptr->max_lev;
		if (henc_top < (p_ptr->max_lev + p_ptr->max_plv) / 2) henc_top = (p_ptr->max_lev + p_ptr->max_plv) / 2;
		if (henc < p_ptr->supp) henc = p_ptr->supp;
		if (henc_top < (p_ptr->max_lev + p_ptr->supp_top) / 2) henc_top = (p_ptr->max_lev + p_ptr->supp_top) / 2;
	}

	/* Process the monsters, check against the highest player around */
	for (i = m_top - 1; i >= 0; i--) {
		/* Access the monster */
		m_ptr = &m_list[m_fast[i]];

		/* On this level? Test its highest encounter so far */
		if (!inarea(&m_ptr->wpos, wpos)) continue;

		if (m_ptr->henc < henc) m_ptr->henc = henc;
		if (m_ptr->henc_top < henc_top) m_ptr->henc_top = henc_top;
	}
}

/* Recall out players of too high level to be eligible to fight the Great Pumpkin */
void check_Pumpkin(void) {
	int k, i, m_idx;
	struct worldpos *wpos;
	char msg[80];
	player_type *p_ptr;
	monster_type *m_ptr;

	/* only during Halloween! */
	if (!season_halloween) return;

	/* Process the monsters */
	for (k = m_top - 1; k >= 0; k--) {
		/* Access the index */
		m_idx = m_fast[k];
		/* Access the monster */
		m_ptr = &m_list[m_idx];
		/* Excise "dead" monsters */
		if (!m_ptr->r_idx) {
			/* Excise the monster */
			m_fast[k] = m_fast[--m_top];
			/* Skip */
			continue;
		}

		/* Players of too high level cannot participate in killing attemps (anti-cheeze) */
		/* search for Great Pumpkins */
		if (m_ptr->r_idx == RI_PUMPKIN1 || m_ptr->r_idx == RI_PUMPKIN2 || m_ptr->r_idx == RI_PUMPKIN3) {
			wpos = &m_ptr->wpos;

			/* Exception for IDDC - just allow */
			if (in_irondeepdive(wpos)) continue;

			for (i = 1; i <= NumPlayers; i++) {
				p_ptr = Players[i];
				if (is_admin(p_ptr)) continue;
				if (inarea(&p_ptr->wpos, wpos) &&
#if 0
 #ifndef RPG_SERVER
				    (p_ptr->max_lev > 30))
 #else
				    (p_ptr->max_lev > 40))
 #endif
#else
 #ifndef RPG_SERVER
				    (p_ptr->max_lev > 30))
 #else
				    (p_ptr->max_lev > 35))
 #endif
#endif
				{
					sprintf(msg, "\377oA ghostly force drives you out of this dungeon!");
					/* log */
#ifndef RPG_SERVER
					s_printf("HALLOWEEN: Recalled>30: %s\n", p_ptr->name);
#else
					s_printf("HALLOWEEN: Recalled>35: %s\n", p_ptr->name);
#endif
					/* get him out of here */
					p_ptr->new_level_method = (p_ptr->wpos.wz > 0 ? LEVEL_RECALL_DOWN : LEVEL_RECALL_UP);
					p_ptr->recall_pos.wx = p_ptr->wpos.wx;
					p_ptr->recall_pos.wy = p_ptr->wpos.wy;
					p_ptr->recall_pos.wz = 0;
//					p_ptr->word_recall =- 666;/*HACK: avoid recall_player loops! */
					recall_player(i, msg);
				}
			}
		}
	}
}

/* This lets Morgoth become stronger, weaker or teleport himself away if
 * a King/Queen joins his level or if a player enters it who hasn't killed
 * Sauron, the Sorceror yet - C. Blue
 */
void check_Morgoth(int Ind) {
	int k, i, x, y, num_on_depth = 0, m_idx;
	s32b tmphp;
	struct worldpos *wpos;
	char msg[80];
	player_type *p_ptr;
	monster_type *m_ptr;


	/* Also check for Pumpkin while we're at it.. */
	if (season_halloween) check_Pumpkin();


	/* Let Morgoth, Lord of Darkness gain additional power
	for each player who joins the depth */

	/* Process the monsters */
	for (k = m_top - 1; k >= 0; k--) {
		/* Access the index */
		m_idx = m_fast[k];
		/* Access the monster */
		m_ptr = &m_list[m_idx];
		/* Excise "dead" monsters */
		if (!m_ptr->r_idx) {
			/* Excise the monster */
			m_fast[k] = m_fast[--m_top];
			/* Skip */
			continue;
		}

		/* search for Morgy */
		if (m_ptr->r_idx != RI_MORGOTH) continue;
		wpos = &m_ptr->wpos;

		/* check if players are on his depth */
		num_on_depth = 0;
		for (i = 1; i <= NumPlayers; i++) {
			/* skip disconnected players */
			if (Players[i]->conn == NOT_CONNECTED) continue;
			/* skip admins */
			if (admin_p(i)) continue;
			/* player on this depth? */
			p_ptr = Players[i];
			if (inarea(&p_ptr->wpos, wpos)) num_on_depth++;
		}

		/* if the last player leaves, don't reduce Morgy's power */
		if (num_on_depth == 0) continue;

		/* Page all dungeon masters to notify them of a possible Morgoth-fight >:) - C. Blue */
		if (watch_morgoth
		    /* new: only page if a player actually entered Morgoth's level */
		    && Ind && inarea(&Players[Ind]->wpos, wpos))
			for (i = 1; i <= NumPlayers; i++) {
				if (Players[i]->conn == NOT_CONNECTED) continue;
				if (Players[i]->admin_dm && !(Players[i]->afk && !streq(Players[i]->afk_msg, "watch"))) Players[i]->paging = 4;
			}

		/* save coordinates to redraw the spot after deletion later */
		x = m_ptr->fx;
		y = m_ptr->fy;

		/* If a King/Queen or a Sauron-missing player enters Morgy's level:
		   if there was an allowed player already on the level, the bad player
		   will be insta-recalled to town. Otherwise, Morgy will be removed.
		   Invalid player alone with Morgy?: Yes->Remove Morgy, No->recall */
		for (i = 1; i <= NumPlayers; i++) {
			if (Players[i]->conn == NOT_CONNECTED) continue;
			p_ptr = Players[i];
			if (is_admin(p_ptr)) continue;

			if (inarea(&p_ptr->wpos, wpos) &&
			    ((p_ptr->mode & MODE_PVP) || p_ptr->total_winner || (p_ptr->r_killed[RI_SAURON] != 1)))
			{
				/* Replace Morgoth with Sauron if Sauron is missing,
				   the other two cases have no repercussions for now -- just allow - C. Blue */
				if (in_irondeepdive(&p_ptr->wpos) || in_hallsofmandos(&p_ptr->wpos)) {
					if (p_ptr->r_killed[RI_SAURON] != 1) {
						dun_level *l_ptr = getfloor(wpos);

						/* remove morgy here */
						delete_monster_idx(m_idx, TRUE); /* careful, 'wpos' is now zero'ed */
						l_ptr->flags1 &= ~LF1_NO_GHOST;

						/* notifications */
						sprintf(msg, "\377sMorgoth, Lord of Darkness summons Sauron, the Sorceror, and teleports out!");
						for (i = 1; i <= NumPlayers; i++) {
							if (Players[i]->conn == NOT_CONNECTED) continue;
							/* Player on Morgy depth? */
							if (inarea(&Players[i]->wpos, &p_ptr->wpos)) {
								msg_print(i, msg);
								handle_music(i); /* :-o */
							}
						}
						s_printf("Morgoth left level (inserting Sauron) due to %s\n", p_ptr->name);

						/* place Sauron so players can kill him first */
						summon_override_checks = SO_ALL; /* (SO_IDDC?) -- Halls of Mandos too */
						place_monster_one(&p_ptr->wpos, y, x, RI_SAURON, FALSE, FALSE, FALSE, 0, 0);
						summon_override_checks = SO_NONE;

						/* Notice */
						note_spot_depth(&p_ptr->wpos, y, x);
						/* Display */
						everyone_lite_spot(&p_ptr->wpos, y, x);
					}
					/* the rest is allowed..
					   -PvP chars can't enter IDDC anyway
					   -and kings may just help their team mates for now, maybe cool */
				}

				/* Teleport player in question out, tell the others on this depth */
				else if (num_on_depth > 1) {
					/* tell a message to the player */
					if (p_ptr->total_winner) {
						sprintf(msg, "\377sA hellish force drives you out of this dungeon!");
						/* log */
						s_printf("Morgoth recalled winner %s\n", p_ptr->name);
					} else if (p_ptr->mode & MODE_PVP) {
						sprintf(msg, "\377sA hellish force drives you out of this dungeon!");
						/* log */
						s_printf("Morgoth recalled MODE_PVP %s\n", p_ptr->name);
					} else {
						sprintf(msg, "\377sYou hear Sauron's laughter as his spell drives you out of the dungeon!");
						/* log */
						s_printf("Morgoth recalled Sauron-misser %s\n", p_ptr->name);
					}

					/* get him out of here */
					p_ptr->new_level_method = (p_ptr->wpos.wz > 0 ? LEVEL_RECALL_DOWN : LEVEL_RECALL_UP);
					p_ptr->recall_pos.wx = p_ptr->wpos.wx;
					p_ptr->recall_pos.wy = p_ptr->wpos.wy;
					p_ptr->recall_pos.wz = 0;
					recall_player(i, msg);

				/* Teleport Morgoth out, since no other players are affected */
				} else {
					sprintf(msg, "\377sMorgoth, Lord of Darkness teleports to a different dungeon floor!");
					for (i = 1; i <= NumPlayers; i++) {
						if (Players[i]->conn == NOT_CONNECTED) continue;
						/* Player on Morgy depth? */
						if (inarea(&Players[i]->wpos, wpos))
							msg_print(i, msg);
					}

					/* log */
					s_printf("Morgoth left level due to %s\n", p_ptr->name);
					/* remove morgy here */
					delete_monster_idx(m_idx, TRUE);

					/* place replacement monster (clone): Death Orb (Star-Spawn, GB, GWoP) */
					summon_override_checks = SO_ALL; /* needed? */
//					place_monster_one(&p_ptr->wpos, y, x, 975, FALSE, FALSE, FALSE, 100, 4 + cfg.clone_summoning);//DOrb (kills items)
					place_monster_one(&p_ptr->wpos, y, x, 847, FALSE, FALSE, FALSE, 100, 4 + cfg.clone_summoning);//GWoP (best maybe)
					summon_override_checks = SO_NONE;

					/* Notice */
					note_spot_depth(&p_ptr->wpos, y, x);
					/* Display */
					everyone_lite_spot(&p_ptr->wpos, y, x);
				}
				return;
			}
		}

		tmphp = (m_ptr->org_maxhp * (2 * (num_on_depth - 1) + 3)) / 3; /* 2/3 HP boost for each additional player */
		/* More players here than Morgy has power for? */
		if (m_ptr->maxhp < tmphp) {
			m_ptr->maxhp = tmphp;
			m_ptr->mspeed += 6;
			m_ptr->speed += 6; /* also adjust base speed, only important for aggr-haste */
			/* Anti-cheeze: Fully heal!
			   Otherwise 2 players could bring him down and the 3rd one
			   just joins for the last 'few' HP.. */
			m_ptr->hp = m_ptr->maxhp;

			/* log */
			s_printf("Morgoth grows stronger\n");
			/* Tell everyone related to Morgy's depth */
			msg_print_near_monster(m_idx, "becomes stronger!");
			return;
		}
		/* Less players here than Morgy has power for? */
		else if (m_ptr->maxhp > tmphp) {
			/* anti-cheeze */
			if (m_ptr->hp == m_ptr->maxhp) {
				m_ptr->hp -= (tmphp - m_ptr->maxhp);
			}
			m_ptr->maxhp = tmphp;
			if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;
			m_ptr->mspeed -= 6;
			m_ptr->speed -= 6; /* also adjust base speed, only important for aggr-haste */

			/* log */
			s_printf("Morgoth weakens\n");
			/* Tell everyone related to Morgy's depth */
			msg_print_near_monster(m_idx, "becomes weaker!");
			return;
		}
	}
}

int players_on_depth(struct worldpos *wpos) {
	if (wpos->wx>MAX_WILD_X || wpos->wx<0 || wpos->wy>MAX_WILD_Y || wpos->wy<0) return(0);
	if (wpos->wz == 0)
		return(wild_info[wpos->wy][wpos->wx].ondepth);
	else {
		struct dungeon_type *d_ptr;
		if (wpos->wz > 0)
			d_ptr = wild_info[wpos->wy][wpos->wx].tower;
		else
			d_ptr = wild_info[wpos->wy][wpos->wx].dungeon;

		return(d_ptr->level[ABS(wpos->wz) - 1].ondepth);
	}
}

/* Don't determine wilderness level just from town radius, but also from the
   level of that town? */
#define WILD_LEVEL_DEPENDS_ON_TOWN
/* Determine wilderness level (for monster generation) */
int getlevel(struct worldpos *wpos) {
	wilderness_type *w_ptr = &wild_info[wpos->wy][wpos->wx];

	if (wpos->wz == 0) {
		/* ground level */
#ifdef WILD_LEVEL_DEPENDS_ON_TOWN
		return (w_ptr->radius + w_ptr->town_lev / 3);
#else
		return (w_ptr->radius);
#endif
	} else {
		struct dungeon_type *d_ptr;
		int base;

		if (wpos->wz > 0) d_ptr = w_ptr->tower;
		else d_ptr = w_ptr->dungeon;
		base = d_ptr->baselevel + ABS(wpos->wz) - 1;
		return (base);
	}
}

/* Simple finder routine. Scan list of c_special, return match */
/* NOTE only ONE of each type !!! */
struct c_special *GetCS(cave_type *c_ptr, unsigned char type) {
	struct c_special *trav;

	if (!c_ptr->special) return(NULL);
	trav = c_ptr->special;
	while (trav) {
		if (trav->type == type) return(trav);
		trav = trav->next;
	}
	return(NULL);				/* returns ** to the structs. always dealloc */
}

/* check for duplication, and also set the type	- Jir - */
struct c_special *AddCS(cave_type *c_ptr, byte type) {
	struct c_special *cs_ptr;
	if (GetCS(c_ptr, type)) {
		return(NULL);	/* already exists! */
	}
	MAKE(cs_ptr, struct c_special);
	if (!cs_ptr) return(NULL);
	cs_ptr->next = c_ptr->special;
	c_ptr->special = cs_ptr;
	cs_ptr->type = type;
	return(cs_ptr);
}

/* like AddCS, but override already-existing one */
c_special *ReplaceCS(cave_type *c_ptr, byte type)
{
	struct c_special *cs_ptr;
	if (!(cs_ptr = GetCS(c_ptr, type)))
	{
		MAKE(cs_ptr, struct c_special);
		if (!cs_ptr) return(NULL);
		cs_ptr->next = c_ptr->special;
		c_ptr->special = cs_ptr;
	}
	cs_ptr->type = type;
	return(cs_ptr);
}

/* Free all memory related to c_ptr->special - mikaelh */
/* Note: doesn't clear the c_ptr->special pointer */
void FreeCS(cave_type *c_ptr)
{
	struct c_special *trav, *prev;

	prev = trav = c_ptr->special;

	while (trav)
	{
		prev = trav;
		trav = trav->next;
		FREE(prev, struct c_special);
	}
}



/*
 * Approximate Distance between two points.
 *
 * When either the X or Y component dwarfs the other component,
 * this function is almost perfect, and otherwise, it tends to
 * over-estimate about one grid per fifteen grids of distance.
 *
 * Algorithm: hypot(dy,dx) = max(dy,dx) + min(dy,dx) / 2
 */
/*
 * For radius-affecting things, consider using tdx[], tdy[], tdi[] instead,
 * which contain pre-calculated results of this function.		- Jir -
 * (Please see prepare_distance() )
 */
int distance(int y1, int x1, int y2, int x2)
{
	int dy, dx, d;

	/* Find the absolute y/x distance components */
	dy = (y1 > y2) ? (y1 - y2) : (y2 - y1);
	dx = (x1 > x2) ? (x1 - x2) : (x2 - x1);

	/* Hack -- approximate the distance */
	d = (dy > dx) ? (dy + (dx>>1)) : (dx + (dy>>1));

	/* Return the distance */
	return (d);
}

/*
 * Returns TRUE if a grid is considered to be a wall for the purpose
 * of magic mapping / clairvoyance
 */
static bool is_wall(cave_type *c_ptr)
{
	int feat;

	feat = c_ptr->feat;

	/* Paranoia */
	if (feat > MAX_F_IDX) return FALSE;

	/* Vanilla floors and doors aren't considered to be walls */
	if (feat < FEAT_SECRET) return FALSE;

	/* Exception #1: a glass wall is a wall but doesn't prevent LOS */
	if (feat == FEAT_GLASS_WALL) return FALSE;

	/* Exception #2: an illusion wall is not a wall but obstructs view */
	if (feat == FEAT_ILLUS_WALL) return TRUE;

	/* Exception #3: Ivy (formerly: a small tree) is a floor but obstructs view */
	if (feat == FEAT_IVY) return TRUE;

	/* Normal cases: use the WALL flag in f_info.txt */
	return (f_info[feat].flags1 & FF1_WALL) ? TRUE : FALSE;
}


/*
 * A simple, fast, integer-based line-of-sight algorithm.  By Joseph Hall,
 * 4116 Brewster Drive, Raleigh NC 27606.  Email to jnh@ecemwl.ncsu.edu.
 *
 * Returns TRUE if a line of sight can be traced from (x1,y1) to (x2,y2).
 *
 * The LOS begins at the center of the tile (x1,y1) and ends at the center of
 * the tile (x2,y2).  If los() is to return TRUE, all of the tiles this line
 * passes through must be floor tiles, except for (x1,y1) and (x2,y2).
 *
 * We assume that the "mathematical corner" of a non-floor tile does not
 * block line of sight.
 *
 * Because this function uses (short) ints for all calculations, overflow may
 * occur if dx and dy exceed 90.
 *
 * Once all the degenerate cases are eliminated, the values "qx", "qy", and
 * "m" are multiplied by a scale factor "f1 = abs(dx * dy * 2)", so that
 * we can use integer arithmetic.
 *
 * We travel from start to finish along the longer axis, starting at the border
 * between the first and second tiles, where the y offset = .5 * slope, taking
 * into account the scale factor.  See below.
 *
 * Also note that this function and the "move towards target" code do NOT
 * share the same properties.  Thus, you can see someone, target them, and
 * then fire a bolt at them, but the bolt may hit a wall, not them.  However,
 * by clever choice of target locations, you can sometimes throw a "curve".
 *
 * Note that "line of sight" is not "reflexive" in all cases.
 *
 * Use the "projectable()" routine to test "spell/missile line of sight".
 *
 * Use the "update_view()" function to determine player line-of-sight.
 */
#ifdef DOUBLE_LOS_SAFETY
static bool los_DLS(struct worldpos *wpos, int y1, int x1, int y2, int x2);
bool los(struct worldpos *wpos, int y1, int x1, int y2, int x2) {
	return (los_DLS(wpos, y1, x1, y2, x2) || los_DLS(wpos, y2, x2, y1, x1));
}
static bool los_DLS(struct worldpos *wpos, int y1, int x1, int y2, int x2) {
#else
bool los(struct worldpos *wpos, int y1, int x1, int y2, int x2) {
#endif
	/* Delta */
	int dx, dy;
	/* Absolute */
	int ax, ay;
	/* Signs */
	int sx, sy;
	/* Fractions */
	int qx, qy;
	/* Scanners */
	int tx, ty;
	/* Scale factors */
	int f1, f2;
	/* Slope, or 1/Slope, of LOS */
	int m;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return FALSE;

	/* Extract the offset */
	dy = y2 - y1;
	dx = x2 - x1;

	/* Extract the absolute offset */
	ay = ABS(dy);
	ax = ABS(dx);


	/* Handle adjacent (or identical) grids */
	if ((ax < 2) && (ay < 2)) return (TRUE);


	/* Paranoia -- require "safe" origin */
	/* if (!in_bounds(y1, x1)) return (FALSE); */


	/* Directly South/North */
	if (!dx) {
		/* South -- check for walls */
		if (dy > 0) {
			for (ty = y1 + 1; ty < y2; ty++)
				if (!cave_los(zcave, ty, x1)) return (FALSE);
		/* North -- check for walls */
		} else {
			for (ty = y1 - 1; ty > y2; ty--)
				if (!cave_los(zcave, ty, x1)) return (FALSE);
		}
		/* Assume los */
		return (TRUE);
	}

	/* Directly East/West */
	if (!dy) {
		/* East -- check for walls */
		if (dx > 0) {
			for (tx = x1 + 1; tx < x2; tx++)
				if (!cave_los(zcave, y1, tx)) return (FALSE);
		/* West -- check for walls */
		} else {
			for (tx = x1 - 1; tx > x2; tx--)
				if (!cave_los(zcave, y1, tx)) return (FALSE);
		}
		/* Assume los */
		return (TRUE);
	}


	/* Extract some signs */
	sx = (dx < 0) ? -1 : 1;
	sy = (dy < 0) ? -1 : 1;


	/* Vertical "knights" */
	if (ax == 1) {
		if (ay == 2) {
			if (cave_los(zcave, y1 + sy, x1)) return (TRUE);
		}
	}
	/* Horizontal "knights" */
	else if (ay == 1) {
		if (ax == 2) {
			if (cave_los(zcave, y1, x1 + sx)) return (TRUE);
		}
	}


	/* Calculate scale factor div 2 */
	f2 = (ax * ay);

	/* Calculate scale factor */
	f1 = f2 << 1;


	/* Travel horizontally */
	if (ax >= ay) {
		/* Let m = dy / dx * 2 * (dy * dx) = 2 * dy * dy */
		qy = ay * ay;
		m = qy << 1;

		tx = x1 + sx;

		/* Consider the special case where slope == 1. */
		if (qy == f2) {
			ty = y1 + sy;
			qy -= f1;
		} else {
			ty = y1;
		}

		/* Note (below) the case (qy == f2), where */
		/* the LOS exactly meets the corner of a tile. */
		while (x2 - tx) {
			if (!cave_los(zcave, ty, tx)) return (FALSE);

			qy += m;

			if (qy < f2) {
				tx += sx;
			} else if (qy > f2) {
				ty += sy;
				if (!cave_los(zcave, ty, tx)) return (FALSE);
				qy -= f1;
				tx += sx;
			} else {
				ty += sy;
				qy -= f1;
				tx += sx;
			}
		}
	}

	/* Travel vertically */
	else {
		/* Let m = dx / dy * 2 * (dx * dy) = 2 * dx * dx */
		qx = ax * ax;
		m = qx << 1;

		ty = y1 + sy;

		if (qx == f2) {
			tx = x1 + sx;
			qx -= f1;
		} else {
			tx = x1;
		}

		/* Note (below) the case (qx == f2), where */
		/* the LOS exactly meets the corner of a tile. */
		while (y2 - ty) {
			if (!cave_los(zcave, ty, tx)) return (FALSE);

			qx += m;

			if (qx < f2) {
				ty += sy;
			} else if (qx > f2) {
				tx += sx;
				if (!cave_los(zcave, ty, tx)) return (FALSE);
				qx -= f1;
				ty += sy;
			} else {
				tx += sx;
				qx -= f1;
				ty += sy;
			}
		}
	}

	/* Assume los */
	return (TRUE);
}

/* Same as los, just ignores non-perma-wall grids - for monster targetting - C. Blue */
#ifdef DOUBLE_LOS_SAFETY
static bool los_wall_DLS(struct worldpos *wpos, int y1, int x1, int y2, int x2);
bool los_wall(struct worldpos *wpos, int y1, int x1, int y2, int x2) {
	return (los_wall_DLS(wpos, y1, x1, y2, x2) || los_wall_DLS(wpos, y2, x2, y1, x1));
}
static bool los_wall_DLS(struct worldpos *wpos, int y1, int x1, int y2, int x2) {
#else
bool los_wall(struct worldpos *wpos, int y1, int x1, int y2, int x2) {
#endif
	/* Delta */
	int dx, dy;
	/* Absolute */
	int ax, ay;
	/* Signs */
	int sx, sy;
	/* Fractions */
	int qx, qy;
	/* Scanners */
	int tx, ty;
	/* Scale factors */
	int f1, f2;
	/* Slope, or 1/Slope, of LOS */
	int m;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return FALSE;

	/* Extract the offset */
	dy = y2 - y1;
	dx = x2 - x1;

	/* Extract the absolute offset */
	ay = ABS(dy);
	ax = ABS(dx);


	/* Handle adjacent (or identical) grids */
	if ((ax < 2) && (ay < 2)) return (TRUE);


	/* Paranoia -- require "safe" origin */
	/* if (!in_bounds(y1, x1)) return (FALSE); */


	/* Directly South/North */
	if (!dx) {
		/* South -- check for walls */
		if (dy > 0) {
			for (ty = y1 + 1; ty < y2; ty++)
				if (!cave_los_wall(zcave, ty, x1)) return (FALSE);
		/* North -- check for walls */
		} else {
			for (ty = y1 - 1; ty > y2; ty--)
				if (!cave_los_wall(zcave, ty, x1)) return (FALSE);
		}
		/* Assume los */
		return (TRUE);
	}

	/* Directly East/West */
	if (!dy) {
		/* East -- check for walls */
		if (dx > 0) {
			for (tx = x1 + 1; tx < x2; tx++)
				if (!cave_los_wall(zcave, y1, tx)) return (FALSE);
		/* West -- check for walls */
		} else {
			for (tx = x1 - 1; tx > x2; tx--)
				if (!cave_los_wall(zcave, y1, tx)) return (FALSE);
		}
		/* Assume los */
		return (TRUE);
	}


	/* Extract some signs */
	sx = (dx < 0) ? -1 : 1;
	sy = (dy < 0) ? -1 : 1;


	/* Vertical "knights" */
	if (ax == 1) {
		if (ay == 2)
			if (cave_los_wall(zcave, y1 + sy, x1)) return (TRUE);
	/* Horizontal "knights" */
	} else if (ay == 1) {
		if (ax == 2)
			if (cave_los_wall(zcave, y1, x1 + sx)) return (TRUE);
	}


	/* Calculate scale factor div 2 */
	f2 = (ax * ay);

	/* Calculate scale factor */
	f1 = f2 << 1;


	/* Travel horizontally */
	if (ax >= ay) {
		/* Let m = dy / dx * 2 * (dy * dx) = 2 * dy * dy */
		qy = ay * ay;
		m = qy << 1;

		tx = x1 + sx;

		/* Consider the special case where slope == 1. */
		if (qy == f2) {
			ty = y1 + sy;
			qy -= f1;
		} else {
			ty = y1;
		}

		/* Note (below) the case (qy == f2), where */
		/* the LOS exactly meets the corner of a tile. */
		while (x2 - tx) {
			if (!cave_los_wall(zcave, ty, tx)) return (FALSE);

			qy += m;

			if (qy < f2) {
				tx += sx;
			} else if (qy > f2) {
				ty += sy;
				if (!cave_los_wall(zcave, ty, tx)) return (FALSE);
				qy -= f1;
				tx += sx;
			} else {
				ty += sy;
				qy -= f1;
				tx += sx;
			}
		}
	}

	/* Travel vertically */
	else {
		/* Let m = dx / dy * 2 * (dx * dy) = 2 * dx * dx */
		qx = ax * ax;
		m = qx << 1;

		ty = y1 + sy;

		if (qx == f2) {
			tx = x1 + sx;
			qx -= f1;
		} else {
			tx = x1;
		}

		/* Note (below) the case (qx == f2), where */
		/* the LOS exactly meets the corner of a tile. */
		while (y2 - ty) {
			if (!cave_los_wall(zcave, ty, tx)) return (FALSE);

			qx += m;

			if (qx < f2) {
				ty += sy;
			} else if (qx > f2) {
				tx += sx;
				if (!cave_los_wall(zcave, ty, tx)) return (FALSE);
				qx -= f1;
				ty += sy;
			} else {
				tx += sx;
				qx -= f1;
				ty += sy;
			}
		}
	}

	/* Assume los */
	return (TRUE);
}





/*
 * Can the player "see" the given grid in detail?
 *
 * He must have vision, illumination, and line of sight.
 *
 * Note -- "CAVE_LITE" is only set if the "torch" has "los()".
 * So, given "CAVE_LITE", we know that the grid is "fully visible".
 *
 * Note that "CAVE_GLOW" makes little sense for a wall, since it would mean
 * that a wall is visible from any direction.  That would be odd.  Except
 * under wizard light, which might make sense.  Thus, for walls, we require
 * not only that they be "CAVE_GLOW", but also, that they be adjacent to a
 * grid which is not only "CAVE_GLOW", but which is a non-wall, and which is
 * in line of sight of the player.
 *
 * This extra check is expensive, but it provides a more "correct" semantics.
 *
 * Note that we should not run this check on walls which are "outer walls" of
 * the dungeon, or we will induce a memory fault, but actually verifying all
 * of the locations would be extremely expensive.
 *
 * Thus, to speed up the function, we assume that all "perma-walls" which are
 * "CAVE_GLOW" are "illuminated" from all sides.  This is correct for all cases
 * except "vaults" and the "buildings" in town.  But the town is a hack anyway,
 * and the player has more important things on his mind when he is attacking a
 * monster vault.  It is annoying, but an extremely important optimization.
 *
 * Note that "glowing walls" are only considered to be "illuminated" if the
 * grid which is next to the wall in the direction of the player is also a
 * "glowing" grid.  This prevents the player from being able to "see" the
 * walls of illuminated rooms from a corridor outside the room.
 */
bool player_can_see_bold(int Ind, int y, int x)
{
	player_type *p_ptr = Players[Ind];
	int xx, yy;

	cave_type *c_ptr;
	byte *w_ptr;
	cave_type **zcave;
	struct worldpos *wpos;
	wpos = &p_ptr->wpos;

	/* Blind players see nothing */
	if (p_ptr->blind) return (FALSE);

	/* temp bug fix - evileye */
	if (!(zcave = getcave(wpos))) return FALSE;
	c_ptr = &zcave[y][x];
	w_ptr = &p_ptr->cave_flag[y][x];

	/* Note that "torch-lite" yields "illumination" */
	if ((c_ptr->info & CAVE_LITE) && (*w_ptr & CAVE_VIEW))
		return (TRUE);

	/* Require line of sight to the grid */
	if (!player_has_los_bold(Ind, y, x)) return (FALSE);

	/* Require "perma-lite" of the grid */
	if (!(c_ptr->info & CAVE_GLOW)) return (FALSE);

	/* Floors are simple */
	if (cave_floor_bold(zcave, y, x)) return (TRUE);

	/* Hack -- move towards player */
	yy = (y < p_ptr->py) ? (y + 1) : (y > p_ptr->py) ? (y - 1) : y;
	xx = (x < p_ptr->px) ? (x + 1) : (x > p_ptr->px) ? (x - 1) : x;

	/* Check for "local" illumination */
	if (zcave[yy][xx].info & CAVE_GLOW) {
		/* Assume the wall is really illuminated */
		return (TRUE);
	}

	/* Assume not visible */
	return (FALSE);
}



/*
 * Returns true if the player's grid is dark
 */
bool no_lite(int Ind)
{
	player_type *p_ptr = Players[Ind];
	if (p_ptr->admin_dm) return(FALSE);
	return (!player_can_see_bold(Ind, p_ptr->py, p_ptr->px));
}


/*
 * Determine if a given location may be "destroyed"
 *
 * Used by destruction spells, and for placing stairs, etc.
 */
/* Borrowed from ToME, with some extra checks */
bool cave_valid_bold(cave_type **zcave, int y, int x)
{
	cave_type *c_ptr = &zcave[y][x];

	s16b this_o_idx, next_o_idx = 0;

	u32b f1, f2, f3, f4, f5, f6, esp;

	/* Forbid perma-grids */
/*	if (cave_perma_grid(c_ptr)) return (FALSE); */
	if (cave_perma_bold(zcave, y, x)) return (FALSE);

	/* Check objects */
	for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Acquire object */
		o_ptr = &o_list[this_o_idx];

		/* Acquire next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Forbid artifact grids */
		if (true_artifact_p(o_ptr))
		{
			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
			if (f4 & TR4_SPECIAL_GENE) return (FALSE);
		}
	}

	/* Accept */
	return (TRUE);
}








/*
 * Hack -- Legal monster codes
 */
static cptr image_monster_hack = \
"@abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

/*
 * Mega-Hack -- Hallucinatory monster
 */
static void image_monster(byte *ap, char *cp)
{
	int n = strlen(image_monster_hack);

	/* Random symbol from set above */
	(*cp) = (image_monster_hack[rand_int(n)]);

	/* Random color */
	(*ap) = randint(15);
}


/*
 * Hack -- Legal object codes
 */
static cptr image_object_hack = \
"?/|\\\"!$()_-=[]{},~";

/*
 * Mega-Hack -- Hallucinatory object
 */
static void image_object(byte *ap, char *cp)
{
	int n = strlen(image_object_hack);

	/* Random symbol from set above */
	(*cp) = (image_object_hack[rand_int(n)]);

	/* Random color */
	(*ap) = randint(15);
}

/*
 * Mega-Hack -- Mimic outlook
 * (Pleaes bear with us till really implemented..)
 */
static void mimic_object(byte *ap, char *cp, int seed)
{
	int n = strlen(image_object_hack);

	/* Random symbol from set above */
	(*cp) = (image_object_hack[seed % n]);

	/* Random color */
	(*ap) = seed % 15 + 1;
}


/*
 * Hack -- Random hallucination
 */
static void image_random(byte *ap, char *cp)
{
	/* Normally, assume monsters */
	if (rand_int(100) < 75)
	{
		image_monster(ap, cp);
	}

	/* Otherwise, assume objects */
	else
	{
		image_object(ap, cp);
	}
}

#ifndef CLIENT_SHIMMER
/*
 * Some eye-candies from PernAngband :)		- Jir -
 */
char get_shimmer_color()
{
	switch (randint(7))
	{
		case 1:
			return TERM_RED;
		case 2:
			return TERM_L_RED;
		case 3:
			return TERM_WHITE;
		case 4:
			return TERM_L_GREEN;
		case 5:
			return TERM_BLUE;
		case 6:
			return TERM_L_DARK;
		case 7:
			return TERM_GREEN;
	}
	return (TERM_VIOLET);
}
#endif

/*
 * Table of breath colors.  Must match listings in a single set of
 * monster spell flags.
 *
 * The value "255" is special.  Monsters with that kind of breath
 * may be any color.
 */
#if 0 /* old */
static byte breath_to_attr[32][2] =
{
	{  0,  0 },
	{  0,  0 },
	{  0,  0 },
	{  0,  0 },
	{  0,  0 },
	{  0,  0 },
	{  0,  0 },
	{  0,  0 },
	{  TERM_SLATE, TERM_L_DARK },		/* RF4_BRTH_ACID */
	{  TERM_BLUE,  TERM_L_BLUE },		/* RF4_BRTH_ELEC */
	{  TERM_RED,  TERM_L_RED },		/* RF4_BRTH_FIRE */
	{  TERM_WHITE,  TERM_L_WHITE },		/* RF4_BRTH_COLD */
	{  TERM_GREEN,  TERM_L_GREEN },		/* RF4_BRTH_POIS */
	{  TERM_L_GREEN,  TERM_GREEN },		/* RF4_BRTH_NETHR */
	{  TERM_YELLOW,  TERM_ORANGE },		/* RF4_BRTH_LITE */
	{  TERM_L_DARK,  TERM_SLATE },		/* RF4_BRTH_DARK */
	{  TERM_L_UMBER,  TERM_UMBER },		/* RF4_BRTH_CONFU */
	{  TERM_YELLOW,  TERM_L_UMBER },	/* RF4_BRTH_SOUND */
        {  255,  255 },   /* (any color) */	/* RF4_BRTH_CHAOS */
	{  TERM_VIOLET,  TERM_VIOLET },		/* RF4_BRTH_DISEN */
	{  TERM_L_RED,  TERM_VIOLET },		/* RF4_BRTH_NEXUS */
	{  TERM_L_BLUE,  TERM_L_BLUE },		/* RF4_BRTH_TIME */
	{  TERM_L_WHITE,  TERM_SLATE },		/* RF4_BRTH_INER */
	{  TERM_L_WHITE,  TERM_SLATE },		/* RF4_BRTH_GRAV */
	{  TERM_UMBER,  TERM_L_UMBER },		/* RF4_BRTH_SHARD */
	{  TERM_ORANGE,  TERM_RED },		/* RF4_BRTH_PLAS */
	{  TERM_UMBER,  TERM_L_UMBER },		/* RF4_BRTH_FORCE */
	{  TERM_L_BLUE,  TERM_WHITE },		/* RF4_BRTH_MANA */
	{  TERM_WHITE,  TERM_L_RED },		/* RF4_BRTH_DISINT */
	{  TERM_GREEN,  TERM_L_GREEN },		/* RF4_BRTH_NUKE */
	{  0,  0 },     /*  */
	{  0,  0 },     /*  */
};
#else
/* new table that uses animated TERM_ colour codes - C. Blue */
static byte breath_to_attr[32][2] =
{
	{  0, 0},
	{  0, 0},
	{  0, 0},
	{  0, 0},
	{  0, 0},
	{  0, 0},
	{  0, 0},
	{  0, 0},
	{  TERM_ACID },				/* RF4_BRTH_ACID */
	{  TERM_ELEC },				/* RF4_BRTH_ELEC */
	{  TERM_FIRE },				/* RF4_BRTH_FIRE */
	{  TERM_COLD },				/* RF4_BRTH_COLD */
	{  TERM_POIS },				/* RF4_BRTH_POIS */
	{  TERM_L_GREEN,  TERM_L_DARK },	/* RF4_BRTH_NETHR */
	{  TERM_LITE, 0 },			/* RF4_BRTH_LITE */
	{  TERM_DARKNESS, 0 },			/* RF4_BRTH_DARK */
	{  TERM_CONF, 0 },			/* RF4_BRTH_CONFU */
	{  TERM_SOUN, 0 },			/* RF4_BRTH_SOUND */
	{  255,  255 },   /* (any color) */	/* RF4_BRTH_CHAOS */
	{  TERM_ORANGE,  TERM_BLUE },		/* RF4_BRTH_DISEN */
	{  TERM_L_RED,  TERM_VIOLET },		/* RF4_BRTH_NEXUS */
	{  TERM_L_BLUE,  TERM_L_GREEN },	/* RF4_BRTH_TIME */
	{  TERM_L_WHITE, TERM_SLATE },		/* RF4_BRTH_INER */
	{  TERM_L_UMBER,  TERM_UMBER },		/* RF4_BRTH_GRAV */
	{  TERM_SHAR, 0 },			/* RF4_BRTH_SHARD */
	{  TERM_L_RED, TERM_RED },		/* RF4_BRTH_PLAS */
	{  TERM_L_WHITE, TERM_ORANGE },		/* RF4_BRTH_FORCE */
	{  TERM_VIOLET, TERM_L_BLUE },		/* RF4_BRTH_MANA */
	{  TERM_L_DARK, TERM_ORANGE },		/* RF4_BRTH_DISINT */
	{  TERM_GREEN, TERM_RED },		/* RF4_BRTH_NUKE */
	{  0,  0 },     /*  */
	{  0,  0 },     /*  */
};
#endif
/*
 * Multi-hued monsters shimmer acording to their breaths.
 *
 * If a monster has only one kind of breath, it uses both colors
 * associated with that breath.  Otherwise, it just uses the first
 * color for any of its breaths.
 *
 * If a monster does not breath anything, it can be any color.
 */
static byte multi_hued_attr(monster_race *r_ptr)
{
	byte allowed_attrs[15];
	int stored_colors = 0;

	byte a;

	int i, j;
	int breaths = 0;
	int first_color = 0;
	int second_color = 0;

	/* Monsters with no ranged attacks can be any color */
#ifdef CLIENT_SHIMMER
	if (!r_ptr->freq_innate) return (TERM_HALF);
#else
	if (!r_ptr->freq_innate) return (get_shimmer_color());
#endif

	/* Check breaths */
	for (i = 0; i < 32; i++) {
		bool stored = FALSE;

		/* Don't have that breath */
		if (!(r_ptr->flags4 & (1U << i))) continue;

		/* Get the first color of this breath */
		first_color = breath_to_attr[i][0];

		/* Breath has no color associated with it */
		if (first_color == 0) continue;

		/* Monster can be of any color */
#ifdef CLIENT_SHIMMER
		if (first_color == 255) return (TERM_MULTI);
#else
		if (first_color == 255) return (randint(15));
#endif


		/* Increment the number of breaths */
		breaths++;

		/* Monsters with lots of breaths may be any color. */
#ifdef CLIENT_SHIMMER
		if (breaths == 6) return (TERM_MULTI);
#else
		if (breaths == 6) return (randint(15));
#endif


		/* Always store the first color */
		for (j = 0; j < stored_colors; j++) {
			/* Already stored */
			if (allowed_attrs[j] == first_color) stored = TRUE;
		}
		if (!stored) {
			allowed_attrs[stored_colors] = first_color;
			stored_colors++;
		}

		/*
		 * Remember (but do not immediately store) the second color
		 * of the first breath.
		 */
		if (breaths == 1) {
			second_color = breath_to_attr[i][1];
			/* make sure breath colour gets more showtime than base colour - C. Blue */
			if (!second_color) second_color = first_color;
		}
	}

	/* Monsters with no breaths may be of any color. */
#ifdef CLIENT_SHIMMER
	if (breaths == 0 || breaths == 5) {
		return (TERM_HALF);
	}
#else
	if (breaths == 0) return (get_shimmer_color());
#endif

	/* If monster has one breath, store the second color too. */
	if (breaths == 1) {
		/* only 1 breath, and one of these? -> discard its r_info
		   base colour completely if ATTR_BASE isn't set: */
		if (!(r_ptr->flags7 & RF7_ATTR_BASE)) {
			switch(r_ptr->flags4 & 0x3fffff00) {
			case RF4_BR_ACID:
				return(TERM_ACID);
			case RF4_BR_COLD:
				return(TERM_COLD);
			case RF4_BR_FIRE:
				return(TERM_FIRE);
			case RF4_BR_ELEC:
				return(TERM_ELEC);
			case RF4_BR_POIS:
				return(TERM_POIS);
			case RF4_BR_CONF:
				return(TERM_CONF);
			case RF4_BR_SOUN:
				return(TERM_SOUN);
			case RF4_BR_SHAR:
				return(TERM_SHAR);
			case RF4_BR_LITE:
				return(TERM_LITE);
			default:
				/* This is annoying - mikaelh */
//				printf("fla: %x\n", r_ptr->flags4 & 0x3fffff00);
				break;
			}
		}
		allowed_attrs[stored_colors] = second_color;
		stored_colors++;
	}

	/*
	 * Hack -- Always has the base colour
	 * (otherwise, Dragonriders are all red)
	 */
#if 0
	if (!m_ptr->special && !m_ptr->questor && p_ptr->use_r_gfx) a = p_ptr->r_attr[m_ptr->r_idx];
	else a = r_ptr->d_attr;
#endif	/* 0 */
	a = r_ptr->d_attr;

	allowed_attrs[stored_colors] = a;
	stored_colors++;

	/* Pick a color at random */
	return (allowed_attrs[rand_int(stored_colors)]);
}

/* Despite of its name, this gets both, attr and char, for a monster race.. */
static void get_monster_color(int Ind, monster_type *m_ptr, monster_race *r_ptr, cave_type *c_ptr, byte *ap, char *cp)
{
	player_type *p_ptr = Players[Ind];
	byte a;
	char c;
	//monster_race *r_ptr = race_inf(m_ptr);

	/* Possibly GFX corrupts with egos;
	 * in that case use m_ptr->r_ptr instead.	- Jir -
	 */
	/* Desired attr */
	/* a = r_ptr->x_attr; */
	if (m_ptr && !m_ptr->special && !m_ptr->questor && p_ptr->use_r_gfx) a = p_ptr->r_attr[m_ptr->r_idx];
	else a = r_ptr->d_attr;
	/*                        else a = m_ptr->r_ptr->d_attr; */

	/* Desired char */
	/* c = r_ptr->x_char; */
	if (m_ptr && !m_ptr->special && !m_ptr->questor && p_ptr->use_r_gfx) c = p_ptr->r_char[m_ptr->r_idx];
	else c = r_ptr->d_char;
	/*                        else c = m_ptr->r_ptr->d_char; */

	/* Hack -- mimics */
	if (r_ptr->flags9 & RF9_MIMIC) {
		mimic_object(&a, &c, c_ptr->m_idx);
	}

	/* Ignore weird codes */
	if (avoid_other) {
		/* Use char */
		(*cp) = c;

		/* Use attr */
		(*ap) = a;
	}

	/* Special attr/char codes */
	else if ((a & 0x80) && (c & 0x80)) {
		/* Use char */
		(*cp) = c;

		/* Use attr */
		(*ap) = a;
	}

#ifdef M_EGO_NEW_FLICKER
	/* Hack -- Unique/Ego 'glitters' sometimes */
	else if ((((r_ptr->flags1 & RF1_UNIQUE) && magik(30)) ||
	    (m_ptr && m_ptr->ego &&
	    ((re_info[m_ptr->ego].d_attr != MEGO_CHAR_ANY) ?
	    magik(85) : magik(5)))
	    ) &&
	    (!(r_ptr->flags1 & (RF1_ATTR_CLEAR | RF1_CHAR_CLEAR)) &&
	     !(r_ptr->flags2 & (RF2_SHAPECHANGER))))
	{
		(*cp) = c;

		if (r_ptr->flags1 & RF1_UNIQUE) {
			/* Multi-hued attr */
			if (r_ptr->flags2 & (RF2_ATTR_ANY))
				(*ap) = TERM_MULTI;
			else
				(*ap) = TERM_HALF;
		} else { /* m_ptr->ego is set: */
			if (re_info[m_ptr->ego].d_attr != MEGO_CHAR_ANY) {
				/* ego colour differs from race colour? */
				if (a != re_info[m_ptr->ego].d_attr) {
					/* flicker in ego colour */
					(*ap) = re_info[m_ptr->ego].d_attr;
				} else if (magik(20)) {
					/* flash randomly, rarely */
					(*ap) = TERM_HALF;
				} else {
					(*ap) = a;
				}
			} else { /* no ego colour? rarely flash (similar to old way) */
				/* ego has no colour - flicker in random colour */
				(*ap) = TERM_HALF;
			}
		}
	}
#else
	/* Hack -- Unique/Ego 'glitters' sometimes */
	else if ((((r_ptr->flags1 & RF1_UNIQUE) && magik(30)) ||
		(m_ptr && m_ptr->ego && magik(5)) ) &&
		(!(r_ptr->flags1 & (RF1_ATTR_CLEAR | RF1_CHAR_CLEAR)) &&
		 !(r_ptr->flags2 & (RF2_SHAPECHANGER))))
	{
		(*cp) = c;

		/* Multi-hued attr */
		if (r_ptr->flags2 & (RF2_ATTR_ANY))
			(*ap) = TERM_MULTI;
		else
			(*ap) = TERM_HALF;
	}
#endif
	/* Multi-hued monster */
	else if (r_ptr->flags1 & RF1_ATTR_MULTI) {
		/* Is it a shapechanger? */
		if (r_ptr->flags2 & RF2_SHAPECHANGER) {
			(*cp) = (randint((r_ptr->flags7 & RF7_VORTEX) ? 1 : 25) == 1?
				 image_object_hack[randint(strlen(image_object_hack))]:
				 image_monster_hack[randint(strlen(image_monster_hack))]);
		} else
			(*cp) = c;

		/* Multi-hued attr */
		if (r_ptr->flags2 & (RF2_ATTR_ANY))
			(*ap) = randint(15);
#ifdef MULTI_HUED_PROPER
		else (*ap) = multi_hued_attr(r_ptr);
#else
 #ifdef CLIENT_SHIMMER
		else (*ap) = TERM_HALF;
 #else
		else (*ap) = get_shimmer_color();
 #endif
#endif	/* MULTI_HUED_PROPER */
#if 0
		/* Normal char */
		(*cp) = c;

		/* Multi-hued attr */
		(*ap) = randint(15);
#endif	/* 0 */
	}
	/* Special ATTR_BNW flag: Monster just flickers black and white,
	   possibly also its base colour if ATTR_BASE is set either.
	   Added this 'hack flag' to display silly Pandas - C. Blue */
	else if (r_ptr->flags7 & RF7_ATTR_BNW) {
		(*cp) = c;
#ifdef SLOW_ATTR_BNW /* handle server-side -> slower speed flickering */
		if ((r_ptr->flags7 & RF7_ATTR_BASE) && !rand_int(3)) {
			/* Use base attr */
			(*ap) = a;
		} else {
			(*ap) = rand_int(2) ? TERM_L_DARK : TERM_WHITE;
		}
#else /* handle client-side -> the usual fast flickering, same as dungeon wizards */
 #ifdef EXTENDED_TERM_COLOURS
		if (is_older_than(&p_ptr->version, 4, 5, 1, 2, 0, 0))
			(*ap) = TERM_OLD_BNW + ((r_ptr->flags7 & RF7_ATTR_BASE) ? a : 0x0);
		else
 #endif
			(*ap) = TERM_BNW + ((r_ptr->flags7 & RF7_ATTR_BASE) ? a : 0x0);
#endif
	}
	/* Normal monster (not "clear" in any way) */
	else if (!(r_ptr->flags1 & (RF1_ATTR_CLEAR | RF1_CHAR_CLEAR))) {
		/* Use char */
		(*cp) = c;

		/* Is it a non-colourchanging shapechanger? */
		if (r_ptr->flags2 & RF2_SHAPECHANGER) {
			(*cp) = (randint((r_ptr->flags7 & RF7_VORTEX) ? 1 : 25) == 1?
				 image_object_hack[randint(strlen(image_object_hack))]:
				 image_monster_hack[randint(strlen(image_monster_hack))]);
		}

		/* Use attr */
		(*ap) = a;
	}

	/* Hack -- Bizarre grid under monster */
	else if ((*ap & 0x80) || (*cp & 0x80)) {
		/* Use char */
		(*cp) = c;

		/* Use attr */
		(*ap) = a;
	}

	/* Normal */
	else {
		/* Normal (non-clear char) monster */
		if (!(r_ptr->flags1 & RF1_CHAR_CLEAR)) {
			/* Normal char */
			(*cp) = c;

			/* Is it a non-colourchanging shapechanger? */
			if (r_ptr->flags2 & RF2_SHAPECHANGER) {
				(*cp) = (randint((r_ptr->flags7 & RF7_VORTEX) ? 1 : 25) == 1?
					 image_object_hack[randint(strlen(image_object_hack))]:
					 image_monster_hack[randint(strlen(image_monster_hack))]);
			}

		}

		/* Normal (non-clear attr) monster */
		else if (!(r_ptr->flags1 & RF1_ATTR_CLEAR)) {
			/* Normal attr */
			(*ap) = a;
		}
	}

	/* Hack -- hallucination */
	if (p_ptr->image) {
		/* Hallucinatory monster */
		image_monster(ap, cp);
	}
}

/*
 * Return the correct "color" of another player
 */
static byte player_color(int Ind) {
	player_type *p_ptr = Players[Ind];
	//monster_race *r_ptr = &r_info[p_ptr->body_monster];
	byte pcolor = p_ptr->pclass;
	char dummy;
	cave_type **zcave = getcave(&p_ptr->wpos);
	cave_type *c_ptr;
	pcolor = p_ptr->cp_ptr->color;

	/* Check that zcave isn't NULL - mikaelh */
	if (!zcave) {
		s_printf("DEBUG: zcave was NULL in player_color\n");
		return TERM_L_DARK;
	}

	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	/* Ghosts are black */
	if (p_ptr->ghost) {
		if (p_ptr->admin_wiz)
#ifdef EXTENDED_TERM_COLOURS
			return (TERM_L_DARK | (is_older_than(&p_ptr->version, 4, 5, 1, 2, 0, 0) ? TERM_OLD_BNW : TERM_BNW));
#else
			return (TERM_L_DARK | TERM_BNW);
#endif
		return TERM_L_DARK;
	}

	/* Black Breath carriers emit malignant aura sometimes.. */
	if (p_ptr->black_breath && magik(50)) return TERM_L_DARK;

	/* Covered by a mummy wrapping? */
	if (TOOL_EQUIPPED(p_ptr) == SV_TOOL_WRAPPING) pcolor = TERM_L_DARK;
	if (p_ptr->cloaked == 1) pcolor = TERM_L_DARK; /* ignore cloak_neutralized for now */

	/* Mimicing a monster */
	/* TODO: handle 'ATTR_MULTI', 'ATTR_CLEAR' */
	/* the_sandman: an attempt to actually diplay the mhd flickers on mimicking player using DS spell */
	if (p_ptr->body_monster)
		get_monster_color(Ind, NULL, &r_info[p_ptr->body_monster], c_ptr, &pcolor, &dummy);

	/* Wearing a costume */
	if ((p_ptr->inventory[INVEN_BODY].tval == TV_SOFT_ARMOR) && (p_ptr->inventory[INVEN_BODY].sval == SV_COSTUME))
		get_monster_color(Ind, NULL, &r_info[p_ptr->inventory[INVEN_BODY].bpval], c_ptr, &pcolor, &dummy);

	/* See vampires burn in the sun sometimes.. */
	if (p_ptr->sun_burn && magik(33)) return TERM_FIRE;

	/* Mana Shield and GOI also flicker */
	/* NOTE: For the player looking at himself, this is done in lite_spot(),
	         which is called from set_tim_manashield().  */
#ifdef EXTENDED_TERM_COLOURS
	if (!is_older_than(&p_ptr->version, 4, 5, 1, 2, 0, 0)) {
		if (p_ptr->tim_manashield > 15) return TERM_SHIELDM;
		else if (p_ptr->tim_manashield) return TERM_NEXU;

		if (p_ptr->invuln > 5) return TERM_SHIELDI;
		else if (p_ptr->invuln && p_ptr->invuln_dur >= 5) return TERM_NUKE;

		if (p_ptr->kinetic_shield > 10) return pcolor |= TERM_BNW;
		else if (p_ptr->kinetic_shield) return pcolor |= TERM_BNW; //no alternative atm
	} else
#endif
	{
		if (p_ptr->tim_manashield > 15) return TERM_SHIELDM;
		if (p_ptr->invuln > 5) return TERM_SHIELDI;
		if (p_ptr->kinetic_shield > 10) return pcolor |= TERM_BNW;
	}

	/* Holy Martyr or shadow running */
	/* Admin wizards sometimes flicker black & white (TERM_BNW) */
	if (p_ptr->shadow_running || p_ptr->admin_wiz)
#ifdef EXTENDED_TERM_COLOURS
		pcolor |= is_older_than(&p_ptr->version, 4, 5, 1, 2, 0, 0) ? TERM_OLD_BNW : TERM_BNW;
#else
		pcolor |= TERM_BNW;
#endif
	if (p_ptr->martyr) { //'holy fire'
#ifdef EXTENDED_TERM_COLOURS
		if (is_older_than(&p_ptr->version, 4, 5, 1, 2, 0, 0)) pcolor |= TERM_OLD_BNW;
		else if (rand_int(2)) pcolor = TERM_HOLYFIRE;
#else
		pcolor |= TERM_BNW;
#endif
	}

	/* Team colours have highest priority */
	if (p_ptr->team) {
		/* may have multiteam */
		switch(p_ptr->team) {
			case 1:
				pcolor = TERM_L_RED; //the_sandman: changed it to light red
						     //since red == istari.
				break;
			case 2:
				pcolor = TERM_L_BLUE;
				break;
			case 3:
				pcolor = TERM_YELLOW;
				break;
			case 4:
				pcolor = TERM_UMBER;
				break;
			default:
				break;
		}
		if ((has_ball(p_ptr) != -1) && magik(50)) pcolor = TERM_ORANGE; /* game ball carrier has orange flickering - mikaelh */
	}

	/* Color is based off of class */
	return pcolor;
}

byte get_trap_color(int Ind, int t_idx, int feat)
{
	player_type *p_ptr = Players[Ind];
	byte a;

	/* Get attr */
	a = t_info[t_idx].color;

	/* Get a new color with a strange formula :) */
	if (t_info[t_idx].flags & FTRAP_CHANGE)
	{
		u32b tmp;

		/* tmp = dlev + dungeon_type + c_ptr->feat; */
		tmp = p_ptr->wpos.wx + p_ptr->wpos.wy + p_ptr->wpos.wz + feat;

		/* a = tmp % 16; */
		/* mega-hack: use trap-like colours only */
		a = tmp % 6 + 1;
	}

	/* Hack -- always l.blue if underwater */
	if (feat == FEAT_DEEP_WATER || feat == FEAT_SHAL_WATER)
		a = TERM_L_BLUE;

	return a;
}

byte get_monster_trap_color(int Ind, int o_idx, int feat) {
	byte a;
	object_type *kit_o_ptr;
	//object_type *load_o_ptr;

	/* Get the trap objects */
	kit_o_ptr = &o_list[o_idx];
	//load_o_ptr = &o_list[kit_o_ptr->next_o_idx];

	/* Get attr */
	a = k_info[kit_o_ptr->k_idx].d_attr;

	/* Hack -- always l.blue if underwater */
	if (feat == FEAT_DEEP_WATER || feat == FEAT_SHAL_WATER)
		a = TERM_L_BLUE;

	return a;
}

byte get_rune_color(int Ind, int typ) {
	byte a = spell_color(r_projections[typ].gf_type);
	return a;
}

/*
 * Manipulate map grid colours, for example outside on world surface,
 * depending on clima or daytime!  - C. Blue
 */
int manipulate_cave_colour_season(cave_type *c_ptr, worldpos *wpos, int x, int y, int colour) {
	bool old_rand = Rand_quick;
	u32b tmp_seed = Rand_value; /* save RNG */
	wilderness_type *w_ptr = &wild_info[wpos->wy][wpos->wx];

	/* World surface manipulation only */
	if (wpos->wz) return colour;

	/* Avoid TERM_L_GREEN vs TERM_GREEN confusion, since using TERM_GREEN for account-check now. */
	if (c_ptr->feat == FEAT_HOME || c_ptr->feat == FEAT_HOME_OPEN) return colour;

	/* To use always the same feats for this everytime the player
	   enters a worldmap sector, we seed the RNG with that particular
	   worldmap coords. */
	Rand_quick = TRUE;
	/* My attempt to create something chaotic - mikaelh */
	Rand_value = (3623U * wpos->wy + 29753) * (2843U * wpos->wx + 48869) +
		(1741U * y + 22109) * y * x + (x + 96779U) * x + 42U;

	/* Seasons */
	switch (season) {
	case SEASON_WINTER:
		/* Replace green trees and grass by white =-} - using live information of original finnish winter */
		if (w_ptr->type != WILD_VOLCANO && (w_ptr->type != WILD_DESERT || c_ptr->feat == FEAT_GRASS)) {
			/* Sometimes display a feat still as green, sometimes brown. */
			switch (c_ptr->feat) {
			case FEAT_DIRT:
				switch (Rand_div(7)) {
				case 0: case 1: case 2: case 3: case 4:
					colour = TERM_L_WHITE; break;
				case 5: if (c_ptr->info & CAVE_LITE) colour = TERM_L_UMBER;
					else colour = TERM_UMBER;
					break;
				case 6: colour = TERM_SLATE; break;
				}
				break;
#if 0 /* maybe keep disabled, for stone2mud effects on world surface - doesn't matter much though? */
			case FEAT_MUD:
				switch (Rand_div(7)) {
				case 0: case 1: case 2: case 3:
					colour = TERM_L_WHITE; break;
				case 4: case 5: case 6: if (c_ptr->info & CAVE_LITE) colour = TERM_L_UMBER;
					else colour = TERM_UMBER;
					break;
				}
				break;
#endif
			case FEAT_GRASS:
				switch (Rand_div(7)) {
				case 0: case 1: case 2: case 3: case 4:
					colour = TERM_L_WHITE; break;
				case 5: case 6:
					if (c_ptr->info & CAVE_LITE) colour = TERM_L_UMBER;
					else colour = TERM_UMBER;
					break;
				//case 7: colour = TERM_GREEN; break;
				}
				break;
			case FEAT_TREE:
			case FEAT_BUSH:
				if (Rand_div(50)) colour = TERM_WHITE;
				else if (Rand_div(3)) colour = TERM_UMBER;
				else colour = TERM_GREEN;
				break;
			case FEAT_IVY:
				if (Rand_div(50)) colour = TERM_WHITE;
				else if (Rand_div(3)) colour = TERM_UMBER;
				else colour = TERM_GREEN;
				break;
			case FEAT_MOUNTAIN:
				if (Rand_div(4)) colour = TERM_WHITE;
				break;
			}
		}
		break;
	case SEASON_SPRING:
		/* More saplings and all green goodness in spring time, yay */
		if (w_ptr->type != WILD_DESERT || c_ptr->feat == FEAT_GRASS) {
			switch (c_ptr->feat) {
			case FEAT_GRASS:
				switch (Rand_div(7)) {
				case 0: case 1: case 2: case 3: case 4:
					colour = TERM_L_GREEN; break;
				case 5:	if (c_ptr->info & CAVE_LITE) colour = TERM_L_UMBER;
					else colour = TERM_UMBER;
					break;
				case 6: colour = TERM_GREEN; break;
				}
				break;
			case FEAT_TREE:
			case FEAT_BUSH:
				if (Rand_div(300)) colour = TERM_L_GREEN;
				else if (Rand_div(3)) colour = TERM_L_UMBER;
				else colour = TERM_UMBER;
				break;
			case FEAT_IVY:
				if (Rand_div(500)) colour = TERM_GREEN;
				else if (Rand_div(3)) colour = TERM_GREEN;
				else colour = TERM_UMBER;
				break;
			}
		}
		break;
	case SEASON_SUMMER:
		/* Mostly grown trees, some bushes, all saturated green, some light green and yellow/light umber */
		if (w_ptr->type != WILD_DESERT || c_ptr->feat == FEAT_GRASS) {
			switch (c_ptr->feat) {
			case FEAT_GRASS:
				switch (Rand_div(6)) {
				case 0:
					colour = TERM_L_GREEN; break;
				case 1: case 2:
					if (c_ptr->info & CAVE_LITE) colour = TERM_YELLOW;
					else colour = TERM_L_UMBER;
							break;
				case 3: case 4:
					colour = TERM_GREEN; break;
				case 5:
					colour = TERM_YELLOW; break;
				}
				break;
			case FEAT_TREE:
			case FEAT_BUSH:
				if (Rand_div(4)) colour = TERM_GREEN;
				else if (Rand_div(10)) colour = TERM_L_GREEN;
				else colour = TERM_L_UMBER;
				break;
			case FEAT_IVY:
				if (Rand_div(3)) colour = TERM_GREEN;
				else if (Rand_div(3)) colour = TERM_L_UMBER;
				else colour = TERM_UMBER;
				break;
			}
		}
		break;
	case SEASON_AUTUMN:
		/* Rarely saplings, very colourful trees, turning to other tones than green */
		if (w_ptr->type != WILD_DESERT || c_ptr->feat == FEAT_GRASS) {
			switch (c_ptr->feat) {
			case FEAT_GRASS:
				switch (Rand_div(7)) {
				case 0: case 1: case 2:
					colour = TERM_GREEN; break;
				case 3: case 4:
					if (c_ptr->info & CAVE_LITE) colour = TERM_L_UMBER;
					else colour = TERM_UMBER;
					break;
				case 5: case 6:
					colour = TERM_YELLOW; break;
				}
				break;
			case FEAT_TREE:
			case FEAT_BUSH:
				if (Rand_div(50))
				switch (Rand_div(10)) {
				case 0: case 1: colour = TERM_GREEN; break;
				case 2: case 3: case 4: colour = TERM_YELLOW; break;
				case 5: case 6: case 7: colour = TERM_L_UMBER; break;
				case 8: case 9: colour = TERM_UMBER; break;
				}
				else colour = TERM_RED;
				break;
			case FEAT_IVY:
				if (Rand_div(3)) colour = TERM_GREEN;
				else if (Rand_div(3)) colour = TERM_L_UMBER;
				else colour = TERM_UMBER;
				break;
			}
		}
		break;
	}

	Rand_quick = old_rand; /* resume complex rng - mikaelh */
	Rand_value = tmp_seed; /* restore RNG */
	return colour;
}
static int manipulate_cave_colour_daytime(cave_type *c_ptr, worldpos *wpos, int x, int y, int colour) {
	/* World surface manipulation only */
	if (wpos->wz) return colour;

	/* Avoid TERM_L_GREEN vs TERM_GREEN confusion, since using TERM_GREEN for account-check now. */
	if (c_ptr->feat == FEAT_HOME || c_ptr->feat == FEAT_HOME_OPEN) return colour;

	/* Darkness on the world surface at night. Darken all colours. */
	if (night_surface &&
	    (!(c_ptr->info & (CAVE_GLOW | CAVE_LITE)) ||
	    (f_info[c_ptr->feat].flags2 & FF2_NIGHT_DARK))) {
		switch (colour) {
		case TERM_DARK: return TERM_DARK;
		case TERM_WHITE: return TERM_SLATE;
		case TERM_SLATE: return TERM_L_DARK;
		case TERM_ORANGE: return TERM_UMBER;
		case TERM_RED: return TERM_RED;
		case TERM_GREEN: return TERM_GREEN;
		case TERM_BLUE: return TERM_BLUE;
		case TERM_UMBER: return TERM_UMBER;
		case TERM_L_DARK: return TERM_L_DARK;
		case TERM_L_WHITE: return TERM_SLATE;
		case TERM_VIOLET: return TERM_VIOLET;
		case TERM_YELLOW: return TERM_L_UMBER;
		case TERM_L_RED: return TERM_RED;
		case TERM_L_GREEN: return TERM_GREEN;
		case TERM_L_BLUE: return TERM_BLUE;
		case TERM_L_UMBER: return TERM_UMBER;
		}
	}

	return colour;
}
static int manipulate_cave_colour(cave_type *c_ptr, worldpos *wpos, int x, int y, int colour) {
	colour = manipulate_cave_colour_season(c_ptr, wpos, x,  y,  colour);
	return manipulate_cave_colour_daytime(c_ptr, wpos, x,  y,  colour);
}
#ifdef SHADE_ALL_FLOOR
static int manipulate_cave_colour_shade(cave_type *c_ptr, worldpos *wpos, int x, int y, int colour) {
	switch (colour) {
	case TERM_DARK: return TERM_DARK;
	case TERM_WHITE: return TERM_SLATE;
	case TERM_SLATE: return TERM_L_DARK;
	case TERM_ORANGE: return TERM_UMBER;
	case TERM_RED: return TERM_RED;
	case TERM_GREEN: return TERM_GREEN;
	case TERM_BLUE: return TERM_BLUE;
	case TERM_UMBER: return TERM_UMBER;
	case TERM_L_DARK: return TERM_L_DARK;
	case TERM_L_WHITE: return TERM_SLATE;
	case TERM_VIOLET: return TERM_VIOLET;
	case TERM_YELLOW: return TERM_L_UMBER;
	case TERM_L_RED: return TERM_RED;
	case TERM_L_GREEN: return TERM_GREEN;
	case TERM_L_BLUE: return TERM_BLUE;
	case TERM_L_UMBER: return TERM_UMBER;
	}
	return colour;
}
#endif


/*
 * Extract the attr/char to display at the given (legal) map location
 *
 * Basically, we "paint" the chosen attr/char in several passes, starting
 * with any known "terrain features" (defaulting to darkness), then adding
 * any known "objects", and finally, adding any known "monsters".  This
 * is not the fastest method but since most of the calls to this function
 * are made for grids with no monsters or objects, it is fast enough.
 *
 * Note that this function, if used on the grid containing the "player",
 * will return the attr/char of the grid underneath the player, and not
 * the actual player attr/char itself, allowing a lot of optimization
 * in various "display" functions.
 *
 * Note that the "zero" entry in the feature/object/monster arrays are
 * used to provide "special" attr/char codes, with "monster zero" being
 * used for the player attr/char, "object zero" being used for the "stack"
 * attr/char, and "feature zero" being used for the "nothing" attr/char,
 * though this function makes use of only "feature zero".
 *
 * Note that monsters can have some "special" flags, including "ATTR_MULTI",
 * which means their color changes, and "ATTR_CLEAR", which means they take
 * the color of whatever is under them, and "CHAR_CLEAR", which means that
 * they take the symbol of whatever is under them.  Technically, the flag
 * "CHAR_MULTI" is supposed to indicate that a monster looks strange when
 * examined, but this flag is currently ignored.  All of these flags are
 * ignored if the "avoid_other" option is set, since checking for these
 * conditions is expensive and annoying on some systems.
 *
 * Currently, we do nothing with multi-hued objects.  We should probably
 * just add a flag to wearable object, or even to all objects, now that
 * everyone can use the same flags.  Then the "SHIMMER_OBJECT" code can
 * be used to request occasional "redraw" of those objects. It will be
 * very hard to associate flags with the "flavored" objects, so maybe
 * they will never be "multi-hued".
 *
 * Note the effects of hallucination.  Objects always appear as random
 * "objects", monsters as random "monsters", and normal grids occasionally
 * appear as random "monsters" or "objects", but note that these random
 * "monsters" and "objects" are really just "colored ascii symbols".
 *
 * Note that "floors" and "invisible traps" (and "zero" features) are
 * drawn as "floors" using a special check for optimization purposes,
 * and these are the only features which get drawn using the special
 * lighting effects activated by "floor_lighting".
 *
 * Note the use of the "mimic" field in the "terrain feature" processing,
 * which allows any feature to "pretend" to be another feature.  This is
 * used to "hide" secret doors, and to make all "doors" appear the same,
 * and all "walls" appear the same, and "hidden" treasure stay hidden.
 * It is possible to use this field to make a feature "look" like a floor,
 * but the "special lighting effects" for floors will not be used.
 *
 * Note the use of the new "terrain feature" information.  Note that the
 * assumption that all interesting "objects" and "terrain features" are
 * memorized allows extremely optimized processing below.  Note the use
 * of separate flags on objects to mark them as memorized allows a grid
 * to have memorized "terrain" without granting knowledge of any object
 * which may appear in that grid.
 *
 * Note the efficient code used to determine if a "floor" grid is
 * "memorized" or "viewable" by the player, where the test for the
 * grid being "viewable" is based on the facts that (1) the grid
 * must be "lit" (torch-lit or perma-lit), (2) the grid must be in
 * line of sight, and (3) the player must not be blind, and uses the
 * assumption that all torch-lit grids are in line of sight.
 *
 * Note that floors (and invisible traps) are the only grids which are
 * not memorized when seen, so only these grids need to check to see if
 * the grid is "viewable" to the player (if it is not memorized).  Since
 * most non-memorized grids are in fact walls, this induces *massive*
 * efficiency, at the cost of *forcing* the memorization of non-floor
 * grids when they are first seen.  Note that "invisible traps" are
 * always treated exactly like "floors", which prevents "cheating".
 *
 * Note the "special lighting effects" which can be activated for floor
 * grids using the "floor_lighting" option (for "white" floor grids),
 * causing certain grids to be displayed using special colors.  If the
 * player is "blind", we will use "dark gray", else if the grid is lit
 * by the torch, and the "view_lamp_lite" option is set, we will use
 * "yellow", else if the grid is "dark", we will use "dark gray", else
 * if the grid is not "viewable", and the "view_shade_floor" option is
 * set, and the we will use "slate" (gray).  We will use "white" for all
 * other cases, in particular, for illuminated viewable floor grids.
 *
 * Note the "special lighting effects" which can be activated for wall
 * grids using the "wall_lighting" option (for "white" wall grids),
 * causing certain grids to be displayed using special colors.  If the
 * player is "blind", we will use "dark gray", else if the grid is lit
 * by the torch, and the "view_lamp_lite" option is set, we will use
 * "yellow", else if the "view_shade_floor" option is set, and the grid
 * is not "viewable", or is "dark", or is glowing, but not when viewed
 * from the player's current location, we will use "slate" (gray).  We
 * will use "white" for all other cases, in particular, for correctly
 * illuminated viewable wall grids.
 *
 * Note that, when "wall_lighting" is set, we use an inline version
 * of the "player_can_see_bold()" function to check the "viewability" of
 * grids when the "view_shade_floor" option is set, and we do NOT use
 * any special colors for "dark" wall grids, since this would allow the
 * player to notice the walls of illuminated rooms from a hallway that
 * happened to run beside the room.  The alternative, by the way, would
 * be to prevent the generation of hallways next to rooms, but this
 * would still allow problems when digging towards a room.
 *
 * Note that bizarre things must be done when the "attr" and/or "char"
 * codes have the "high-bit" set, since these values are used to encode
 * various "special" pictures in some versions, and certain situations,
 * such as "multi-hued" or "clear" monsters, cause the attr/char codes
 * to be "scrambled" in various ways.
 *
 * Note that eventually we may use the "&" symbol for embedded treasure,
 * and use the "*" symbol to indicate multiple objects, though this will
 * have to wait for Angband 2.8.0 or later.  Note that currently, this
 * is not important, since only one object or terrain feature is allowed
 * in each grid.  If needed, "k_info[0]" will hold the "stack" attr/char.
 *
 * Note the assumption that doing "x_ptr = &x_info[x]" plus a few of
 * "x_ptr->xxx", is quicker than "x_info[x].xxx", if this is incorrect
 * then a whole lot of code should be changed...  XXX XXX
 */
void map_info(int Ind, int y, int x, byte *ap, char *cp) {
	player_type *p_ptr = Players[Ind];

	cave_type *c_ptr;
	byte *w_ptr;

	feature_type *f_ptr;

	int feat;

	byte a;
	char c;

	int a_org;
	bool lite_snow, keep = FALSE;

	cave_type **zcave;
	if (!(zcave = getcave(&p_ptr->wpos))) return;

	/* Get the cave */
	c_ptr = &zcave[y][x];
	w_ptr = &p_ptr->cave_flag[y][x];


	/* Feature code */
	feat = c_ptr->feat;

#if 0
	/* bad hack to display visible wall instead of clear wall in sector00 events */
	if (sector00separation &&
	    *cp == ' ' && feat == FEAT_PERM_CLEAR
	    && in_sector00(&p_ptr->wpos)
	    && sector00wall) {
		if (!p_ptr->font_map_solid_walls) {
			*cp = p_ptr->f_char[sector00wall];
			a = p_ptr->f_attr[sector00wall];
		} else { /* hack */
			*cp = p_ptr->f_char_solid[sector00wall];
			a = p_ptr->f_attr_solid[sector00wall];
		}
	}
#else
	/* bad hack to display visible wall instead of clear wall in sector00 events */
	if (sector00separation &&
	    feat == FEAT_PERM_CLEAR
	    && in_sector00(&p_ptr->wpos)
	    && sector00wall)
		feat = sector00wall;
#endif

	/* Access floor */
	f_ptr = &f_info[feat];

	/* hack for questor hard-coded lighting:
	   Replace any permanent (daylight/townstore) light by 'fake' light source 'carried' by the questor */
	if (c_ptr->info & CAVE_GLOW_HACK) {
		c_ptr->info &= ~CAVE_GLOW;
		c_ptr->info |= (CAVE_LITE | CAVE_LITE_WHITE);
	}
	else if (c_ptr->info & CAVE_GLOW_HACK_LAMP) {
		c_ptr->info &= ~CAVE_GLOW;
		c_ptr->info |= CAVE_LITE;
	}

	/* Floors (etc) */
	/* XXX XXX Erm, it is DIRTY.  should be replaced soon */
	//if (feat <= FEAT_INVIS)
	if (f_ptr->flags1 & (FF1_FLOOR)) {

		/* Memorized (or visible) floor */
		/* Hack -- space are visible to the dungeon master */
		if (((*w_ptr & CAVE_MARK) ||
		    ((((c_ptr->info & CAVE_LITE) &&
			(*w_ptr & CAVE_VIEW)) ||
		      ((c_ptr->info & CAVE_GLOW) &&
		       (*w_ptr & CAVE_VIEW))) &&
		     !p_ptr->blind)) || (p_ptr->admin_dm))
		{
			struct c_special *cs_ptr;

			/* for FEAT_ILLUS_WALL, which aren't walls but floors! */
			if (!p_ptr->font_map_solid_walls) {
				/* Normal char */
				(*cp) = p_ptr->f_char[feat];

				/* Normal attr */
				a = p_ptr->f_attr[feat];
			} else { /* hack */
				(*cp) = p_ptr->f_char_solid[feat];
				a = p_ptr->f_attr_solid[feat];
			}

			/* Hack to display monster traps */
			/* Illusory wall masks everythink */
			if ((cs_ptr = GetCS(c_ptr, CS_MON_TRAP)) && c_ptr->feat != FEAT_ILLUS_WALL) {
				/* Hack -- random hallucination */
				if (p_ptr->image) {
					/*image_random(ap, cp); */
					image_object(ap, cp);
					a = randint(15);
				} else {
					/* If trap isn't on door display it */
					/* if (!(f_ptr->flags1 & FF1_DOOR)) c = '^'; */
					//(*cp) = ';';
					a = get_monster_trap_color(Ind,
					    cs_ptr->sc.montrap.trap_kit,
					    feat);

#if 0 /* currently this doesn't make sense because montraps are their own feature (like runes) instead of using just the cs_ptr (like normal traps)! This means they cancel the water grid! ew. */
					/* Hack -- always l.blue if underwater */
					if (cs_ptr->sc.montrap.feat == FEAT_DEEP_WATER || cs_ptr->sc.montrap.feat == FEAT_SHAL_WATER)
						a = TERM_L_BLUE;
#endif
				}
				keep = TRUE;
			}

			/* Hack to display "runes of warding" which are coloured by GF_TYPE */
			/* Illusory wall masks everythink */
			if ((cs_ptr = GetCS(c_ptr, CS_RUNE)) && c_ptr->feat != FEAT_ILLUS_WALL) {
				a = get_rune_color(Ind, cs_ptr->sc.rune.typ);
				keep = TRUE;
			}

			/* Hack to display detected traps */
			/* Illusory wall masks everythink */
			if ((cs_ptr = GetCS(c_ptr, CS_TRAPS)) && c_ptr->feat != FEAT_ILLUS_WALL) {
				int t_idx = cs_ptr->sc.trap.t_idx;
				if (cs_ptr->sc.trap.found) {
					/* Hack -- random hallucination */
					if (p_ptr->image) {
/*						image_random(ap, cp); */
						image_object(ap, cp);
						a = randint(15);
					} else {
						/* If trap isn't on door display it */
						/* if (!(f_ptr->flags1 & FF1_DOOR)) c = '^'; */
						(*cp) = '^';

						a = get_trap_color(Ind, t_idx, feat);
					}
					keep = TRUE;
				}
			}

			/* Quick Hack -- shop */
			if ((cs_ptr = GetCS(c_ptr, CS_SHOP))) {
				(*cp) = st_info[cs_ptr->sc.omni].d_char;
				a = st_info[cs_ptr->sc.omni].d_attr;

				a = manipulate_cave_colour(c_ptr, &p_ptr->wpos, x, y, a);
			}

			/* apply colour to OPEN house doors (which have FF1_FLOOR,
			   are hence separated from close house doors - C. Blue */
			else if ((cs_ptr = GetCS(c_ptr, CS_DNADOOR))) {
				a = access_door_colour(Ind, cs_ptr->sc.ptr);

				a = manipulate_cave_colour(c_ptr, &p_ptr->wpos, x, y, a);
			}

			/* don't apply special lighting/shading on traps/montraps/runes! */
			else if (keep) {
				/* nothing */
			}

			/* Special lighting effects */
			else if (p_ptr->floor_lighting &&
			    (a_org = manipulate_cave_colour_season(c_ptr, &p_ptr->wpos, x, y, a)) != -1 && /* dummy */
			    ((lite_snow = ((f_ptr->flags2 & FF2_LAMP_LITE_SNOW) && /* dirty snow and clean slow */
			    (a_org == TERM_WHITE || a_org == TERM_L_WHITE))) ||
			    (f_ptr->flags2 & (FF2_LAMP_LITE | FF2_SPECIAL_LITE)) ||
			    ((f_ptr->flags2 & FF2_LAMP_LITE_OPTIONAL) && p_ptr->view_lite_extra))) {
				a = manipulate_cave_colour_daytime(c_ptr, &p_ptr->wpos, x, y, a_org);

				/* Handle "blind" */
				if (p_ptr->blind) {
					/* Use "dark gray" */
					a = TERM_L_DARK;
				}

				/* Handle "torch-lit" grids */
				else if (((f_ptr->flags2 & FF2_LAMP_LITE) ||
				    ((f_ptr->flags2 & FF2_LAMP_LITE_OPTIONAL) && p_ptr->view_lite_extra) ||
				    lite_snow) &&
				    ((c_ptr->info & CAVE_LITE) && (*w_ptr & CAVE_VIEW))) {
					/* Torch lite */
					if (p_ptr->view_lamp_floor) {
#ifdef CAVE_LITE_COLOURS
						if ((c_ptr->info & CAVE_LITE_WHITE)) {
							if (!(f_ptr->flags2 & FF2_NO_LITE_WHITEN)) a = (a == TERM_L_DARK) ? TERM_SLATE ://<-specialty for ash
							    TERM_WHITE;//normal
						} else if ((c_ptr->info & CAVE_LITE_VAMP)) {
							if (!(f_ptr->flags2 & FF2_NO_LITE_WHITEN)) a = (a == TERM_L_DARK) ? TERM_SLATE ://<-specialty for ash
							    TERM_WHITE; /* usual glowing floor grids are TERM_WHITE, so lamp light shouldn't be darker (TERM_L_WHITE).. */
						} else if (is_newer_than(&p_ptr->version, 4, 5, 2, 0, 0, 0) && p_ptr->view_animated_lite) {
							if (is_newer_than(&p_ptr->version, 4, 5, 7, 2, 0, 0)) a = (a == TERM_L_DARK) ? TERM_LAMP_DARK : TERM_LAMP;//<-specialty: shaded ash
							else a = (a == TERM_L_DARK) ? TERM_UMBER : TERM_LAMP;//<-specialty: shaded ash
						}
						else a = (a == TERM_L_DARK) ? TERM_UMBER : TERM_YELLOW;//<-specialty: shaded ash
#else
						a = TERM_YELLOW;
#endif
					}
				}

				/* Special flag */
				if (p_ptr->view_shade_floor
#ifndef SHADE_ALL_FLOOR
				    && (!(f_ptr->flags2 & FF2_NO_SHADE) || lite_snow)
#endif
				    ) {
					/* Handle "dark" grids */
					if (!(c_ptr->info & (CAVE_GLOW | CAVE_LITE))) {
						/* Use "dark gray" */
						if (a != TERM_DARK) //<-hack: don't accidentally lighten up naturally-dark grids
							a = TERM_L_DARK;
					}
					/* Handle "out-of-sight" grids */
					else if (!(*w_ptr & CAVE_VIEW)) {
#ifndef SHADE_ALL_FLOOR
						/* Use "gray" */
						if (a != TERM_DARK && a != TERM_L_DARK) //<-hack: don't accidentally lighten up naturally-dark grids (FEAT_ASH)
							a = TERM_SLATE;
#else
						if (p_ptr->wpos.wz || !night_surface) /* not already shaded in m.c.c.daytime() above? */
							a = manipulate_cave_colour_shade(c_ptr, &p_ptr->wpos, x, y, a);
#endif
					}
				}
			}
			else a = manipulate_cave_colour(c_ptr, &p_ptr->wpos, x, y, a);

			/* The attr */
			(*ap) = a;
		}

		/* Unknown */
		else {
			/* Access darkness */
			f_ptr = &f_info[FEAT_NONE];

			/* Normal attr */
			/* (*ap) = f_ptr->f_attr; */
			(*ap) = p_ptr->f_attr[FEAT_NONE];

			/* Normal char */
			/* (*cp) = f_ptr->f_char; */
			(*cp) = p_ptr->f_char[FEAT_NONE];
		}
	}

	/* Non floors */
	else {
		/* Memorized grids */
		/* Hack -- everything is visible to dungeon masters */
		if ((*w_ptr & CAVE_MARK) || (p_ptr->admin_dm)) {
			struct c_special *cs_ptr;

			/* Apply "mimic" field */
			//feat = f_info[feat].mimic;

			/* Hack -- hide the secret door */
			//if (feat == FEAT_SECRET && (cs_ptr = GetCS(c_ptr, CS_MIMIC)))
			if ((cs_ptr = GetCS(c_ptr, CS_MIMIC))) {
				feat = cs_ptr->sc.omni;
			} else {
				/* Apply "mimic" field */
				feat = f_info[feat].mimic;
			}

			/* Access feature */
			f_ptr = &f_info[feat];

			if (!p_ptr->font_map_solid_walls) {
				/* Normal char */
				/* (*cp) = f_ptr->f_char; */
				(*cp) = p_ptr->f_char[feat];

				/* Normal attr */
				/* a = f_ptr->f_attr; */
				a = p_ptr->f_attr[feat];
			} else { /* hack */
				(*cp) = p_ptr->f_char_solid[feat];
				a = p_ptr->f_attr_solid[feat];
			}

			/* Add trap color - Illusory wall masks everythink */
			/* Hack to display detected traps */
			/*
			   if ((c_ptr->t_idx != 0) && (c_ptr->info & CAVE_TRDT) &&
			   (c_ptr->feat != FEAT_ILLUS_WALL))
			   if ((c_ptr->special.type == CS_TRAPS) && (c_ptr->special.sc.ptr->found))
			*/
			/* Hack to display detected traps */
			if ((cs_ptr = GetCS(c_ptr, CS_TRAPS)) && c_ptr->feat != FEAT_ILLUS_WALL) {
				int t_idx = cs_ptr->sc.trap.t_idx;
				if (cs_ptr->sc.trap.found) {
					/* Hack -- random hallucination */
					if (p_ptr->image) {
						image_object(ap, cp);
						a = randint(15);
					} else {
						a = get_trap_color(Ind, t_idx, feat);
					}
					keep = TRUE;
				}
			}
			/* Hack -- gee it's great to be back home */
			if ((cs_ptr = GetCS(c_ptr, CS_DNADOOR))) {
#if 0
				if (access_door(Ind, cs_ptr->sc.ptr, FALSE)) {
					a = TERM_L_GREEN;
				} else {
					struct dna_type *dna = cs_ptr->sc.ptr;
					if (dna->owner && dna->owner_type)
						a = TERM_L_DARK;
				}
#else
				a = access_door_colour(Ind, cs_ptr->sc.ptr);
#endif
				keep = TRUE;
			}

			/* don't apply special lighting/shading on traps in 'walls' (secret doors) or closed doors */
			if (keep) {
				/* nothing */
			}

			/* Special lighting effects */
			else if (p_ptr->wall_lighting &&
			    (a_org = manipulate_cave_colour_season(c_ptr, &p_ptr->wpos, x, y, a)) != -1 && /* dummy */
			    ((lite_snow = ((f_ptr->flags2 & FF2_LAMP_LITE_SNOW) && /* dirty snow and clean slow */
			    (a_org == TERM_WHITE || a_org == TERM_L_WHITE))) ||
			    (f_ptr->flags2 & (FF2_LAMP_LITE | FF2_SPECIAL_LITE)) ||
			    ((f_ptr->flags2 & FF2_LAMP_LITE_OPTIONAL) && p_ptr->view_lite_extra))) {
				a = manipulate_cave_colour_daytime(c_ptr, &p_ptr->wpos, x, y, a_org);

				/* Handle "blind" */
				if (p_ptr->blind) {
					/* Use "dark gray" */
					a = TERM_L_DARK;
				}

				/* Handle "torch-lit" grids */
				else if (((f_ptr->flags2 & FF2_LAMP_LITE) ||
				    ((f_ptr->flags2 & FF2_LAMP_LITE_OPTIONAL) && p_ptr->view_lite_extra) ||
				    lite_snow) &&
				    (c_ptr->info & CAVE_LITE)) {
					/* Torch lite */
					if (p_ptr->view_lamp_walls) {
#ifdef CAVE_LITE_COLOURS
						if ((c_ptr->info & CAVE_LITE_WHITE)) {
							if (!(f_ptr->flags2 & FF2_NO_LITE_WHITEN) && a != TERM_WHITE) //don't darken max white (perma-walls)
								a = (a == TERM_L_DARK) ? TERM_SLATE ://<-specialty for magma vein/volcanic rock
								    TERM_L_WHITE; /* <- white-lit granite; for now: instead of TERM_WHITE; to distinguish it from permanent walls */
						} else if ((c_ptr->info & CAVE_LITE_VAMP)) {
							//if (!(f_ptr->flags2 & FF2_NO_LITE_WHITEN)) a = TERM_SLATE; /* to make a difference to TERM_L_WHITE; for the time being (see above) */
							if (!(f_ptr->flags2 & FF2_NO_LITE_WHITEN) && a != TERM_WHITE) //don't darken max white (perma-walls)
								a = (a == TERM_L_DARK) ? TERM_SLATE ://<-specialty for magma vein
								    TERM_L_WHITE; /* <- vamp-lit granite; TERM_SLATE just looks too weird (see above) */
						} else if (is_newer_than(&p_ptr->version, 4, 5, 2, 0, 0, 0) && p_ptr->view_animated_lite) {
							if (is_newer_than(&p_ptr->version, 4, 5, 7, 2, 0, 0)) a = (a == TERM_L_DARK) ? TERM_LAMP_DARK : TERM_LAMP;//<-specialty: shaded magma vein
							else a = TERM_LAMP;//(a == TERM_L_DARK) ? TERM_L_UMBER : TERM_LAMP;//<-specialty: shaded magma vein
						}
						else a = (a == TERM_L_DARK) ? TERM_L_UMBER : TERM_YELLOW;//<-specialty: shaded magma vein
#else
						a = TERM_YELLOW;
#endif
					}
				}

				/* Handle "view_shade_walls" */
				/* NOTE: The only prob here is that if magma doesn't have a different
					 symbol from granite walls, for example if the player maps
					 both to 'solid block' character, shaded granite walls and
					 magma veins will look the same - C. Blue */
				//else if (p_ptr->view_shade_walls && !(f_ptr->flags2 & FF2_NO_SHADE)) {
				else if (p_ptr->view_shade_walls && (!(f_ptr->flags2 & FF2_NO_SHADE) || lite_snow)) {
					/* Not glowing */
					if (!(c_ptr->info & (CAVE_GLOW | CAVE_LITE))) {
						/* Allow distinguishing permanent walls from granite */
						if (a_org != TERM_WHITE) {
							if (a_org == TERM_L_WHITE) a = TERM_SLATE;
							else a = TERM_L_DARK;
						}
						/* don't "brighten up" if it was darkened by night */
						else if (a == TERM_WHITE) a = TERM_L_WHITE;
					}
					/* Not viewable */
					else if (!(*w_ptr & CAVE_VIEW)
#ifdef DONT_SHADE_GLOW_AT_NIGHT
					    && !(night_surface && !p_ptr->wpos.wz)
#endif
					    ) {
						/* Allow distinguishing permanent walls from granite */
						if (a_org != TERM_WHITE) {
							if (a_org == TERM_L_WHITE) a = TERM_SLATE;
							else a = TERM_L_DARK;
						}
						/* don't "brighten up" if it was darkened by night */
						else if (a == TERM_WHITE) a = TERM_L_WHITE;
					}

#if 0 /* anyone know what the idea for this is? It causes a visual glitch, so disabling it for now */
					/* Not glowing correctly */
					else {
						int xx, yy;

						/* Hack -- move towards player */
						yy = (y < p_ptr->py) ? (y + 1) : (y > p_ptr->py) ? (y - 1) : y;
						xx = (x < p_ptr->px) ? (x + 1) : (x > p_ptr->px) ? (x - 1) : x;

						/* Check for "local" illumination */
						if (!(zcave[yy][xx].info & CAVE_GLOW)) {
							/* Use "gray" */
							a = TERM_SLATE;
						}
					}
#endif
				}
			}
			else a = manipulate_cave_colour(c_ptr, &p_ptr->wpos, x, y, a);

			/* Display vault walls in a more distinguishable colour, if desired */
			if (p_ptr->permawalls_shade && (feat == FEAT_PERM_INNER || feat == FEAT_PERM_OUTER)) a = TERM_L_UMBER;

			/* The attr */
			(*ap) = a;

			for (cs_ptr = c_ptr->special; cs_ptr; cs_ptr = cs_ptr->next) {
				/* testing only - need c/a PRIORITIES!!! */
				csfunc[cs_ptr->type].see(cs_ptr, cp, ap, Ind);
			}
		}

		/* Unknown */
		else {
			/* Access darkness */
			f_ptr = &f_info[FEAT_NONE];

			/* Normal attr */
			/* (*ap) = f_ptr->f_attr; */
			(*ap) = p_ptr->f_attr[FEAT_NONE];

			/* Normal char */
			/* (*cp) = f_ptr->f_char; */
			(*cp) = p_ptr->f_char[FEAT_NONE];
		}
	}


	/**** Apply special random effects ****/
/*	if (!avoid_other) */
	if (((*w_ptr & CAVE_MARK) ||
	((((c_ptr->info & CAVE_LITE) && (*w_ptr & CAVE_VIEW)) ||
	  ((c_ptr->info & CAVE_GLOW) && (*w_ptr & CAVE_VIEW))) &&
	 !p_ptr->blind)) || (p_ptr->admin_dm))
	{
		f_ptr = &f_info[feat];

		/* Special terrain effect */
		if (c_ptr->effect) {
#if 0
			(*ap) = spell_color(effects[c_ptr->effect].type);
#else /* allow 'transparent' spells */
			a = spell_color(effects[c_ptr->effect].type);
			if (a != 127) (*ap) = a;
#endif
		}

#if 1
		/* Multi-hued attr */
		/* TODO: this should be done in client-side too, so that
		 * they shimmer when player isn't moving */
		else if (f_ptr->flags1 & FF1_ATTR_MULTI) {
			a = f_ptr->shimmer[rand_int(7)];

			if (rand_int(8) != 1)
				a = manipulate_cave_colour(c_ptr, &p_ptr->wpos, x, y, a);

			(*ap) = a;
		}
#if 1
		/* Give staircases different colours depending on dungeon flags -C. Blue :) */
		if ((c_ptr->feat == FEAT_MORE) || (c_ptr->feat == FEAT_WAY_MORE) ||
		    (c_ptr->feat == FEAT_WAY_LESS) || (c_ptr->feat == FEAT_LESS)) {
		    struct dungeon_type *d_ptr;
		    worldpos *tpos = &p_ptr->wpos;
		    wilderness_type *wild = &wild_info[tpos->wy][tpos->wx];

		    if (!tpos->wz) {
			if ((c_ptr->feat == FEAT_MORE) || (c_ptr->feat == FEAT_WAY_MORE)) d_ptr = wild->dungeon;
			else d_ptr = wild->tower;
		    } else if (tpos->wz < 0) d_ptr = wild->dungeon;
		    else d_ptr = wild->tower;

		    /* Check for empty staircase without any connected dungeon/tower! */
		    if (!d_ptr) {
			    (*ap) = TERM_SLATE;
		    } else {
			    /* override colour from easiest to worst */
			    get_staircase_colour(d_ptr, ap);
		    }
 #ifdef GLOBAL_DUNGEON_KNOWLEDGE
		    /* player has seen the entrance on the actual main screen -> add it to global exploration history knowledge */
		    d_ptr->known |= 0x1;
 #endif
		}
#endif

#ifdef HOUSE_PAINTING
		if (c_ptr->feat == FEAT_WALL_HOUSE && c_ptr->colour) {
 #ifdef HOUSE_PAINTING_HIDE_BAD_MODE
 			if (is_admin(p_ptr)) {
				if (c_ptr->colour > 100) (*ap) = c_ptr->colour - 100 - 1;
				else (*ap) = c_ptr->colour - 1;
 			} else if (c_ptr->colour > 100) {
				if ((p_ptr->mode & MODE_EVERLASTING)) (*ap) = c_ptr->colour - 100 - 1;
			} else {
				if (!(p_ptr->mode & MODE_EVERLASTING)) (*ap) = c_ptr->colour - 1;
			}
 #else
			if (c_ptr->colour > 100) (*ap) = c_ptr->colour - 100 - 1;
			else (*ap) = c_ptr->colour - 1;
 #endif
		}
#endif

		/* jails */
		if (c_ptr->info & CAVE_JAIL) (*ap) = TERM_L_DARK;

#endif
	}

	/* Hack -- rare random hallucination, except on outer dungeon walls */
	if (p_ptr->image && (!rand_int(256)) && !(f_info[c_ptr->feat].flags2 & FF2_BOUNDARY)) {
		/* Hallucinate */
		image_random(ap, cp);
	}


	/* Objects */
	if (c_ptr->o_idx
#ifdef FIX_NOTHINGS_ON_SIGHT
	    && !nothing_test(&o_list[c_ptr->o_idx], p_ptr, &p_ptr->wpos, x, y, 5)
#endif
#if 1 /* optional: prevent objects from being visible if they're entombed! Need to "dig them out" first. */
	    && cave_floor_bold(zcave, y, x)
#endif
	    ) {
		struct c_special *cs_ptr;

		/* Hack - Traps override objects while searching - mikaelh */
		if (!p_ptr->searching || !((cs_ptr = GetCS(c_ptr, CS_TRAPS)) && cs_ptr->sc.trap.found)) {
			/* Get the actual item */
			object_type *o_ptr = &o_list[c_ptr->o_idx];

			/* Memorized objects */
			/* Hack -- the dungeon master knows where everything is */
			if ((p_ptr->obj_vis[c_ptr->o_idx]) || (p_ptr->admin_dm)) {
				/* Normal char */
				(*cp) = object_char(o_ptr);

				/* Normal attr */
				(*ap) = object_attr(o_ptr);

				/* Hack -- always l.blue if underwater */
				if (feat == FEAT_DEEP_WATER || feat == FEAT_SHAL_WATER)
					(*ap) = TERM_L_BLUE;

				/* hacks: mindcrafter 'crystals' are yellow, custom books' colour depends on their content! - C. Blue */
				if (o_ptr->tval == TV_BOOK) {
					if (o_ptr->pval >= __lua_M_FIRST && o_ptr->pval <= __lua_M_LAST)
						(*ap) = TERM_YELLOW;
					else if (is_custom_tome(o_ptr->sval))
						(*ap) = get_book_name_color(o_ptr);
				}

				/* hack: colour of fancy shirts or custom objects can vary  */
				if ((o_ptr->tval == TV_SOFT_ARMOR && o_ptr->sval == SV_SHIRT) ||
				    (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT)) {
					if (!o_ptr->xtra1) o_ptr->xtra1 = (s16b)(*ap); //wut.. remove this hack? should be superfluous anyway
					(*ap) = (byte)o_ptr->xtra1;
					/* new: also allow custom char */
					if (o_ptr->xtra2) (*cp) = (char)o_ptr->xtra2;
				}

				/* quest items can have custom appearance too */
				if (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_QUEST) {
					(*cp) = (char)o_ptr->xtra1;
					(*ap) = (byte)o_ptr->xtra2;
				}

				/* Abnormal attr */
/*      	                  if ((!avoid_other) && (!(((*ap) & 0x80) && ((*cp) & 0x80))) && (k_info[o_ptr->k_idx].flags5 & TR5_ATTR_MULTI)) (*ap) = get_shimmer_color(); */
				if (k_info[o_ptr->k_idx].flags5 & TR5_ATTR_MULTI)
#ifdef CLIENT_SHIMMER
					(*ap) = TERM_HALF;
#else
					(*ap) = get_shimmer_color();
#endif
/*					(*ap) = randint(15); */

				/* hack: questors may have specific attr */
				if (o_ptr->questor) (*ap) = q_info[o_ptr->quest - 1].questor[o_ptr->questor_idx].oattr;

				/* Hack -- hallucination */
				if (p_ptr->image) image_object(ap, cp);
			}
		}
	}


	/* Handle monsters */
	if (c_ptr->m_idx > 0) {
		monster_type *m_ptr = &m_list[c_ptr->m_idx];

		if (c_ptr->m_idx >= m_max) {
			/* Clear invalid monster references - mikaelh */

			/* Log it */
			s_printf("MIDX_FIX: Cleared an invalid monster reference (m_idx = %d, m_max = %d) (wpos = %d, %d, %d) (x = %d, y = %d)\n", c_ptr->m_idx, m_max, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, x, y);

			c_ptr->m_idx = 0;
		}

		/* Visible monster */
		if (p_ptr->mon_vis[c_ptr->m_idx]) {
			monster_race *r_ptr = race_inf(m_ptr);

			/* hack: mindcrafters/rogues can see through
			   shadowing (shadowed ego) / cloaking (master thief ego) */
			if ((p_ptr->pclass == CLASS_MINDCRAFTER || p_ptr->pclass == CLASS_ROGUE) && p_ptr->lev >= 30 &&
#if 0 /* this works, but maybe let's be more specific, in case some exceptional new re-power gets added that shouldn't be affected.. */
			    (r_ptr->flags1 & RF1_CHAR_CLEAR) && (r_ptr->flags1 & RF1_ATTR_CLEAR) &&
			    !(r_info[m_ptr->r_idx].flags1 & (RF1_CHAR_CLEAR | RF1_ATTR_CLEAR)))
#else
			    (m_ptr->ego == RE_MASTER_THIEF || m_ptr->ego == RE_SHADOWED))
#endif
				/* slightly paranoid: we only remove the flags that REALLY weren't
				   in the base version.. such cases shouldn't really occur though */
				r_ptr->flags1 &= ~((r_ptr->flags1 & RF1_CHAR_CLEAR) | (r_ptr->flags1 & RF1_ATTR_CLEAR));

			get_monster_color(Ind, m_ptr, r_ptr, c_ptr, ap, cp);
		}
	}

	/* -APD-
	   Taking D. Gandy's advice and making it display the char as a number if
	   they are severly wounded (60% health or less)
	   I multiply by 95 instead of 100 because it always rounds down.....
	   and I want to give PCs a little more breathing room.
	 */

	else if (c_ptr->m_idx < 0) {
		int Ind2 = 0 - c_ptr->m_idx;
		player_type *p2_ptr = Players[Ind2];

		if (!p2_ptr) {
			/* Clear invalid players, I'm tired of crashes - mikaelh */
			c_ptr->m_idx = 0;

			/* Log it */
			s_printf("MIDX_FIX: Cleared an invalid player m_idx (Ind = %d) (wpos = %d, %d, %d) (x = %d, y = %d)\n", Ind2, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, x, y);
		}

		/* Check for doppelgangers - mikaelh */
		else if (memcmp(&p_ptr->wpos, &p2_ptr->wpos, sizeof(struct worldpos)) != 0 || x != p2_ptr->px || y != p2_ptr->py) {
			/* Clear doppelgangers */
			c_ptr->m_idx = 0;

			/* Log it */
			s_printf("MIDX_FIX: Cleared a doppelganger (Ind = %d, \"%s\") (wpos = %d, %d, %d) (x = %d, y = %d)\n", Ind2, Players[Ind2]->name, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, x, y);
		}

		/* Is that player visible? */
		else if (p_ptr->play_vis[Ind2]) {
			/* part 'A' now here (experimental, see below) */

			/* Hack: For monster-forms, player_color() calls get_monster_color() which will
			   result in color flickering if player is hallucinating, although this call
			   should instead determine the colour an _outside_ player gets to see. So we
			   have to temporarily suppress his hallucinations to get the correct value. */
			int tmp_image = p2_ptr->image;
			p2_ptr->image = 0;
			a = player_color(Ind2);
			p2_ptr->image = tmp_image;

#if 0 /* player_color() should already handle all of this - C. Blue */
			if ((p2_ptr->inventory[INVEN_BODY].tval == TV_SOFT_ARMOR) && (p2_ptr->inventory[INVEN_BODY].sval == SV_COSTUME)) {
				get_monster_color(Ind, NULL, &r_info[p2_ptr->inventory[INVEN_BODY].bpval], c_ptr, &a, &c);
			}
			else if (p2_ptr->body_monster) get_monster_color(Ind, NULL, &r_info[p2_ptr->body_monster], c_ptr, &a, &c);
			else if (p2_ptr->fruit_bat) c = 'b';
			else c = '@';
#else
			if ((p2_ptr->inventory[INVEN_BODY].tval == TV_SOFT_ARMOR) && (p2_ptr->inventory[INVEN_BODY].sval == SV_COSTUME)) {
				c = r_info[p2_ptr->inventory[INVEN_BODY].bpval].d_char;
			}
			else if (p2_ptr->body_monster) {
				c = r_info[p2_ptr->body_monster].d_char;
#if 1 /* just for fun */
				if (p2_ptr->body_monster == RI_DOOR_MIMIC && p2_ptr->dummy_option_7) c = '\'';
#endif
			}
			else if (p2_ptr->fruit_bat) c = 'b';
			else c = '@';

#endif
			/* Always show party members as dark grey @? Allow pvp flickers still */
			if (p_ptr->consistent_players) {
				c = '@';
				a = TERM_L_DARK;
				if (p2_ptr->black_breath && magik(50)) {
					a = TERM_SLATE;
				}
			}

			/* snowed by a snowball hit? */
			if (p2_ptr->dummy_option_8) a = TERM_WHITE;

			/* part 'A' end */

			/* TERM_BNW if blood bonded - mikaelh */
			if (check_blood_bond(Ind, Ind2))
#ifdef EXTENDED_TERM_COLOURS
				a |= is_older_than(&p_ptr->version, 4, 5, 1, 2, 0, 0) ? TERM_OLD_BNW : TERM_BNW;
#else
				a |= TERM_BNW;
#endif
			/* new: renamed TERM_RLE (unused) to TERM_PVP for use for this: - C. Blue */
			else if (is_newer_than(&p_ptr->version, 4, 4, 2, 0, 0, 0) &&
			    (check_hostile(Ind, Ind2) || p2_ptr->stormbringer))
#ifdef EXTENDED_TERM_COLOURS
				a |= is_older_than(&p_ptr->version, 4, 5, 1, 2, 0, 0) ? TERM_OLD_PVP : TERM_PVP;
#else
				a |= TERM_PVP;
#endif

			if (((p2_ptr->chp * 95) / (p2_ptr->mhp * 10)) > TURN_CHAR_INTO_NUMBER) {
				/* part 'A' used to be here */
			} else {
				if (p2_ptr->chp < 0) c = '-';
				else {
					int num;
					num = (p2_ptr->chp * 95) / (p2_ptr->mhp * 10);
					c = '0' + num;
				}
			}

			/* admins sees intensity of mana shields */
			if (p_ptr->admin_dm && p2_ptr->tim_manashield && p2_ptr->msp > 0) {
				if (((p2_ptr->csp * 100) / (p2_ptr->msp * 10)) < 10) {
					int num;
					num = (p2_ptr->csp * 100) / (p2_ptr->msp * 10);
					c = '0' + num;
				}
			}

			(*cp) = c;
			(*ap) = a;

			if (p_ptr->image) {
				/* Change the other player into a hallucination */
				image_monster(ap, cp);
			}
		}
	}


	/* Special 'dummy' effects that are purely for adding unusual visuals */
	if (!c_ptr->effect) return;

	/* display blue raindrops */
	if ((effects[c_ptr->effect].flags & EFF_RAINING)) {
		(*ap) = TERM_BLUE;
		if (wind_gust > 0) (*cp) = '/';
		else if (wind_gust < 0) (*cp) = '\\';
		else (*cp) = '|';
	}
	/* for WINTER_SEASON */
	/* display white snowflakes */
	if ((effects[c_ptr->effect].flags & EFF_SNOWING)) {
		(*ap) = TERM_WHITE;
		(*cp) = '*'; /* a little bit large maybe, but '.' won't be noticed on the other hand? */
	}
	/* for NEW_YEARS_EVE */
	/* display fireworks */
	if ((effects[c_ptr->effect].flags & (EFF_FIREWORKS1 | EFF_FIREWORKS2 | EFF_FIREWORKS3))) {
		switch (effects[c_ptr->effect].type) {
		case GF_FW_FIRE: (*ap) = TERM_FIRE; break;
		case GF_FW_ELEC: (*ap) = TERM_ELEC; break;
		case GF_FW_POIS: (*ap) = TERM_POIS; break;
		case GF_FW_LITE: (*ap) = TERM_LITE; break;
		case GF_FW_SHDI: (*ap) = TERM_SHIELDI; break;
		case GF_FW_SHDM: (*ap) = TERM_SHIELDM; break;
		case GF_FW_MULT: (*ap) = TERM_MULTI; break;
		}
		(*cp) = '*'; /* a little bit large maybe, but '.' won't be noticed on the other hand? */
	}
	/* for Nether Realm finishing */
	if ((effects[c_ptr->effect].flags & (EFF_LIGHTNING1 | EFF_LIGHTNING2 | EFF_LIGHTNING3))) {
		(*ap) = TERM_LITE;
		switch (c_ptr->effect_xtra) {
		case 0: (*cp) = '|'; break;
		case 1: (*cp) = '/'; break;
		case -1: (*cp) = '\\'; break;
		case 2: (*cp) = '_'; break;
		default: (*cp) = '*';
		}
	}

	/* for 'Thunderstorm' spell */
	if ((effects[c_ptr->effect].flags & EFF_THUNDER_VISUAL)) {
		(*ap) = TERM_THUNDER;
		/* if basic floor or water, make it a bit flashier afterglow :D (makes it easier to see/better to savour if monster dies from it) */
		// | * / \ ( )
		if (*cp == '.' || *cp == '~' || rand_int(2))
		switch (rand_int(5)) {
		case 0: (*cp) = '|'; break;
		case 1: (*cp) = '\\'; break;
		case 2: (*cp) = '/'; break;
		case 3: (*cp) = ')'; break;
		case 4: (*cp) = '('; break;
		}
	}

/* #ifdef ARCADE_SERVER
        if ((effects[c_ptr->effect].flags & EFF_CROSSHAIR_A) || (effects[c_ptr->effect].flags & EFF_CROSSHAIR_B) || (effects[c_ptr->effect].flags & EFF_CROSSHAIR_C))
        {
	        (*ap) = TERM_L_UMBER;
	        (*cp) = '+';
        }
#endif */
}


/*
 * Memorize the given grid (or object) if it is "interesting"
 *
 * This function should only be called on "legal" grids.
 *
 * This function should be called every time the "memorization" of
 * a grid (or the object in a grid) is called into question.
 *
 * Note that the player always memorized all "objects" which are seen,
 * using a different method than the one used for terrain features,
 * which not only allows a lot of optimization, but also prevents the
 * player from "knowing" when objects are dropped out of sight but in
 * memorized grids.
 *
 * Note that the player always memorizes "interesting" terrain features
 * (everything but floors and invisible traps).  This allows incredible
 * amounts of optimization in various places.
 *
 * Note that the player is allowed to memorize floors and invisible
 * traps under various circumstances, and with various options set.
 *
 * This function is slightly non-optimal, since it memorizes objects
 * and terrain features separately, though both are dependant on the
 * "player_can_see_bold()" macro.
 */
void note_spot(int Ind, int y, int x) {
	player_type *p_ptr = Players[Ind];
	byte *w_ptr = &p_ptr->cave_flag[y][x];

	cave_type **zcave;
	cave_type *c_ptr;
	if (!(zcave = getcave(&p_ptr->wpos))) return;
	c_ptr = &zcave[y][x];

	/* Hack -- memorize objects */
	if (c_ptr->o_idx) {
		/* Only memorize once */
		if (!(p_ptr->obj_vis[c_ptr->o_idx])) {
			/* Memorize visible objects */
			if (player_can_see_bold(Ind, y, x)) {
				/* Memorize */
				p_ptr->obj_vis[c_ptr->o_idx] = TRUE;

			}
		}
	}


	/* Hack -- memorize grids */
	if (!(*w_ptr & CAVE_MARK)) {
		/* Memorize visible grids */
		if (player_can_see_bold(Ind, y, x)) {
			/* Memorize normal features */
			if (!cave_plain_floor_grid(c_ptr)) {
				/* Memorize */
				*w_ptr |= CAVE_MARK;
			}

			/* Option -- memorize all perma-lit floors */
			else if (p_ptr->view_perma_grids && (c_ptr->info & CAVE_GLOW)) {
				/* Memorize */
				*w_ptr |= CAVE_MARK;
			}

			/* Option -- memorize all torch-lit floors */
			else if (p_ptr->view_torch_grids && (c_ptr->info & CAVE_LITE)) {
				/* Memorize */
				*w_ptr |= CAVE_MARK;
			}
		}
	}
}


void note_spot_depth(struct worldpos *wpos, int y, int x) {
	int i;

	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		if (inarea(wpos, &Players[i]->wpos))
			note_spot(i, y, x);
	}
}

void everyone_lite_spot(struct worldpos *wpos, int y, int x) {
	int i;

	/* Check everyone */
	for (i = 1; i <= NumPlayers; i++) {
		/* If he's not playing, skip him */
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* If he's not here, skip him */
		if (!inarea(wpos, &Players[i]->wpos))
			continue;

		/* Actually lite that spot for that player */
		lite_spot(i, y, x);
	}
}

void everyone_clear_ovl_spot(struct worldpos *wpos, int y, int x) {
	int i;

	/* Check everyone */
	for (i = 1; i <= NumPlayers; i++) {
		/* If he's not playing, skip him */
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* If he's not here, skip him */
		if (!inarea(wpos, &Players[i]->wpos))
			continue;

		/* Actually clear the overlay on that spot for that player */
		clear_ovl_spot(i, y, x);
	}
}

/*
 * Wipe the "CAVE_MARK" bit in everyone's array
 */
void everyone_forget_spot(struct worldpos *wpos, int y, int x) {
	int i;

	/* Check everyone */
	for (i = 1; i <= NumPlayers; i++) {
		/* If he's not playing, skip him */
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* If he's not here, skip him */
		if (!inarea(wpos, &Players[i]->wpos))
			continue;

		/* Forget the spot */
		Players[i]->cave_flag[y][x] &= ~CAVE_MARK;
	}
}

/*
 * Redraw (on the screen) a given MAP location
 */
void lite_spot(int Ind, int y, int x) {
	player_type *p_ptr = Players[Ind];
	bool is_us = FALSE;
	int sane;

	/* Redraw if on screen */
	if (panel_contains(y, x)) {
		int dispx, dispy;

		byte a;
		char c;

		/* Handle "player" */
		if ((y == p_ptr->py) && (x == p_ptr->px)) {
			monster_race *r_ptr = &r_info[p_ptr->body_monster];

			if ((p_ptr->inventory[INVEN_BODY].tval == TV_SOFT_ARMOR) && (p_ptr->inventory[INVEN_BODY].sval == SV_COSTUME)) {
				r_ptr = &r_info[p_ptr->inventory[INVEN_BODY].bpval];
			}

			/* Get the "player" attr */
			a = r_ptr->d_attr;

			/* Get the "player" char */
			c = r_ptr->d_char;
#if 1 /* just for fun */
			if (p_ptr->body_monster == RI_DOOR_MIMIC && p_ptr->dummy_option_7) c = '\'';
#endif

			/*if (p_ptr->invis && !p_ptr->body_monster) {  - hmm why not always TERM_VIOLET */
			if (p_ptr->invis) {
				/* special invis colour */
				a = TERM_VIOLET;
			}
			if (p_ptr->cloaked == 1) {
				if (p_ptr->cloak_neutralized) a = TERM_SLATE;
				else a = TERM_L_DARK;
			}
			/* Mana Shield and GOI also flicker */
			if (p_ptr->tim_manashield && rand_int(2)) {
				if (p_ptr->tim_manashield > 15) {
					/* prevent too much violet colour in our mix.. */
					if (a != TERM_VIOLET)
						a = (randint(2) < 2) ? TERM_VIOLET : TERM_ORANGE;
					else
						a = (randint(2) < 2) ? TERM_L_RED : TERM_ORANGE;
				} else {
					a = TERM_BNW; // todo? -> TERM_NEXU:
				}
			}
			if (p_ptr->invuln && rand_int(4)) {
				if (p_ptr->invuln > 5) {
					switch (randint(5)) {
					case 1: a = TERM_L_RED; break;
					case 2: a = TERM_L_GREEN; break;
					case 3: a = TERM_L_BLUE; break;
					case 4: a = TERM_YELLOW; break;
					case 5: a = TERM_VIOLET; break;
					}
				} else if (p_ptr->invuln_dur >= 5) { /* avoid animating normal stair-GoI */
					a = TERM_BNW; // todo? -> TERM_NUKE:
				}
			}
			if (p_ptr->martyr)
#ifdef EXTENDED_TERM_COLOURS
				a |= is_older_than(&p_ptr->version, 4, 5, 1, 2, 0, 0) ? TERM_OLD_BNW : TERM_BNW;
#else
				a |= TERM_BNW;
#endif
			if (p_ptr->kinetic_shield)
#ifdef EXTENDED_TERM_COLOURS
				a |= is_older_than(&p_ptr->version, 4, 5, 1, 2, 0, 0) ? TERM_OLD_BNW : TERM_BNW;
#else
				a |= TERM_BNW;
#endif

			/* notice own Black Breath by colour instead just from occasional message */
			if (p_ptr->black_breath && magik(50)) a = TERM_L_DARK;

			/* see oneself burning in the sun */
			if (p_ptr->sun_burn && magik(33)) a = TERM_FIRE;

			/* Polymorph ring power running out */
			if (p_ptr->tim_mimic
#ifdef ENABLE_HELLKNIGHT
			    && p_ptr->pclass != CLASS_HELLKNIGHT
 #ifdef ENABLE_CPRIEST
			    && p_ptr->pclass != CLASS_CPRIEST
 #endif
#endif
			    && p_ptr->body_monster == p_ptr->tim_mimic_what && p_ptr->tim_mimic <= 100) {
				if (!rand_int(10)) {
					a = TERM_DISE;
					c = '@';
				}
			}

			/* Holy Martyr */
			/* Admin wizards sometimes flicker black & white (TERM_BNW) */
			if (p_ptr->shadow_running || p_ptr->martyr || p_ptr->admin_wiz)
#ifdef EXTENDED_TERM_COLOURS
				a |= is_older_than(&p_ptr->version, 4, 5, 1, 2, 0, 0) ? TERM_OLD_BNW : TERM_BNW;
#else
				a |= TERM_BNW;
#endif

			if (p_ptr->team) {
				if (magik(25)) { /* chance for showing him/her which team (s)he's in - mikaelh */
					switch(p_ptr->team) {
						case 1:
							a = TERM_L_RED;
							break;
						case 2:
							a = TERM_L_BLUE;
							break;
						default:
							break;
					}
				}
				else if ((has_ball(p_ptr) != -1) && magik(25)) a = TERM_ORANGE; /* game ball carrier has orange flickering - mikaelh */
			}


			if (p_ptr->consistent_players) {
				a = TERM_WHITE;
				if (p_ptr->tim_mimic > 0 && p_ptr->body_monster == p_ptr->tim_mimic_what) {
					if (
#ifdef ENABLE_HELLKNIGHT
					    p_ptr->pclass != CLASS_HELLKNIGHT &&
 #ifdef ENABLE_CPRIEST
					    p_ptr->pclass != CLASS_CPRIEST &&
 #endif
#endif
					    p_ptr->tim_mimic <= 100 && !rand_int(10))
						a = TERM_WHITE;
					else a = TERM_ORANGE;
				}
				if (p_ptr->tim_manashield && p_ptr->msp > 0 && p_ptr->csp > 0) {
					a = TERM_YELLOW;
				}
				if (p_ptr->black_breath && magik(50)) {
					a = TERM_SLATE;
				}
			}

#ifdef ENABLE_SELF_FLASHING
			/* display player in really easily spottable colours */
			if (p_ptr->flash_self > 0) a = (p_ptr->flash_self % 2) ? TERM_L_RED : TERM_L_GREEN;
#endif

			/* display a low-on-sanity player flashy to himself? */
			if (p_ptr->flash_insane) {
				sane = p_ptr->msane ? (p_ptr->csane * 100) / p_ptr->msane : 100;
				/* use same colours as for sanity bar */
				if (sane < 10) a = TERM_MULTI;
				else if (sane < 25) a = TERM_SHIELDI;
			}

			/* bugfix on MASSIVE deaths (det/death) */
			if (p_ptr->fruit_bat && !p_ptr->body_monster &&
				!((p_ptr->inventory[INVEN_BODY].tval == TV_SOFT_ARMOR) && (p_ptr->inventory[INVEN_BODY].sval == SV_COSTUME))) c = 'b';

			if (p_ptr->consistent_players) c = '@';

			if (p_ptr->chp < 0) c = '-';
			else if (!p_ptr->tim_manashield) {
				if (((p_ptr->chp * 95) / (p_ptr->mhp*10)) <= TURN_CHAR_INTO_NUMBER) {
					int num;
					num = (p_ptr->chp * 95) / (p_ptr->mhp * 10);
					c = '0' + num;
				}
			} else if (p_ptr->msp > 0) {
				if (((p_ptr->csp * 95) / (p_ptr->msp * 10)) <= TURN_CHAR_INTO_NUMBER) {
					int num;
					num = (p_ptr->csp * 95) / (p_ptr->msp * 10);
					c = '0' + num;
				}
			}

			/* snowed by a snowball hit? */
			if (p_ptr->dummy_option_8) a = TERM_WHITE;

			/* >4.5.4: Mark that it is the player himself */
			if (p_ptr->hilite_player) is_us = TRUE;
		}

		/* Normal (not player coords) */
		else {
			/* Examine the grid */
			map_info(Ind, y, x, &a, &c);
		}

		/* Hack -- fake monochrome */
		if (!use_color)  a = TERM_WHITE;

		dispx = x - p_ptr->panel_col_prt;
		dispy = y - p_ptr->panel_row_prt;

		/* Only draw if different than buffered */
		if (p_ptr->scr_info[dispy][dispx].c != c ||
		    p_ptr->scr_info[dispy][dispx].a != a ||
		    (x == p_ptr->px && y == p_ptr->py && !p_ptr->afk) /* let's try disabling this when AFK to save bandwidth - mikaelh */
		    /* for clearing overlay that displays auto-updating things (monsters who meanwhile moved away): clear it as soon as it comes into LOS */
		    || ((p_ptr->cave_flag[y][x] & CAVE_AOVL) && (p_ptr->cave_flag[y][x] & CAVE_VIEW) && (p_ptr->cave_flag[y][x] & CAVE_LITE))) {
			/* Modify screen buffer */
			p_ptr->scr_info[dispy][dispx].c = c;
			p_ptr->scr_info[dispy][dispx].a = a;

			/* Compare against the overlay buffer */
			if ((p_ptr->ovl_info[dispy][dispx].c != c) ||
			    (p_ptr->ovl_info[dispy][dispx].a != a)) {
				/* Old cfg.hilite_player implementation has been disabled after 4.6.1.1 because it interferes with custom fonts */
				if (!is_newer_than(&p_ptr->version, 4, 6, 1, 1, 0, 1)) {
					if (is_us && is_newer_than(&p_ptr->version, 4, 5, 4, 0, 0, 0)) c |= 0x80;
				}

				/* Tell client to redraw this grid */
				Send_char(Ind, dispx, dispy, a, c);
			}

			/* Clear the overlay buffer */
			p_ptr->ovl_info[dispy][dispx].c = 0;
			p_ptr->ovl_info[dispy][dispx].a = 0;
			p_ptr->cave_flag[y][x] &= ~CAVE_AOVL;
		}
	}
}


/*
 * Draw something on the overlay layer.
 */
void draw_spot_ovl(int Ind, int y, int x, byte a, char c) {
	player_type *p_ptr = Players[Ind];

	/* Redraw if on screen */
	if (panel_contains(y, x)) {
		int dispx, dispy;

		/* Handle "player" */
		if ((y == p_ptr->py) && (x == p_ptr->px)) {
			/* Never redraw the player */
			return;
		}

		/* Hack -- fake monochrome */
		if (!use_color) a = TERM_WHITE;

		dispx = x - p_ptr->panel_col_prt;
		dispy = y - p_ptr->panel_row_prt;

		/* Only draw if different than buffered */
		if (p_ptr->ovl_info[dispy][dispx].c != c ||
		    p_ptr->ovl_info[dispy][dispx].a != a) {
			/* Modify internal buffer */
			p_ptr->ovl_info[dispy][dispx].c = c;
			p_ptr->ovl_info[dispy][dispx].a = a;

			p_ptr->cave_flag[y][x] |= CAVE_AOVL;

			/* Tell client to redraw this grid */
			Send_char(Ind, dispx, dispy, a, c);
		}
	}
}


/*
 * Clear a spot on the overlay layer.
 */
void clear_ovl_spot(int Ind, int y, int x) {
	player_type *p_ptr = Players[Ind];

	/* Redraw if on screen */
	if (panel_contains(y, x)) {
		int dispx, dispy;

		dispx = x - p_ptr->panel_col_prt;
		dispy = y - p_ptr->panel_row_prt;

		if (p_ptr->ovl_info[dispy][dispx].c) {
			/* Check if the overlay buffer is different from the screen buffer */
			if ((p_ptr->ovl_info[dispy][dispx].a != p_ptr->scr_info[dispy][dispx].a) ||
			    (p_ptr->ovl_info[dispy][dispx].c != p_ptr->scr_info[dispy][dispx].c)) {
				/* Clear the overlay buffer */
				p_ptr->ovl_info[dispy][dispx].c = 0;
				p_ptr->ovl_info[dispy][dispx].a = 0;

				/* Clear the screen buffer to force redraw */
				p_ptr->scr_info[dispy][dispx].c = 0;
				p_ptr->scr_info[dispy][dispx].a = 0;

				p_ptr->cave_flag[y][x] &= ~CAVE_AOVL;

				/* Redraw */
				lite_spot(Ind, y, x);
			} else {
				/* Clear the overlay buffer */
				p_ptr->ovl_info[dispy][dispx].c = 0;
				p_ptr->ovl_info[dispy][dispx].a = 0;

				p_ptr->cave_flag[y][x] &= ~CAVE_AOVL;

				/* No redraw needed */
			}
		}
	}
}


/*
 * Clear the entire overlay ler.
 */
void clear_ovl(int Ind) {
	player_type *p_ptr = Players[Ind];
	int y, x;

	for (y = p_ptr->panel_row_min; y <= p_ptr->panel_row_max; y++) {
		for (x = p_ptr->panel_col_min; x <= p_ptr->panel_col_max; x++) {
			clear_ovl_spot(Ind, y, x);
		}
	}
}





/*
 * Prints the map of the dungeon
 *
 * Note that, for efficiency, we contain an "optimized" version
 * of both "lite_spot()" and "print_rel()", and that we use the
 * "lite_spot()" function to display the player grid, if needed.
 *
 * scr_only: If we're using do_cmd_locate(), don't clear the overlay map,
 *           to keep detected monsters etc. visible! (LOCATE_KEEPS_OVL)
 */

void prt_map(int Ind, bool scr_only) {
	player_type *p_ptr = Players[Ind];

	int x, y;
	int dispx, dispy;
	byte a;
	char c;

	/* Make sure he didn't just change depth */
	if (p_ptr->new_level_flag) return;

	/* First clear the old stuff */
	memset(p_ptr->scr_info, 0, sizeof(p_ptr->scr_info));

	if (!scr_only) {
		/* Clear the overlay buffer */
		memset(p_ptr->ovl_info, 0, sizeof(p_ptr->ovl_info));
	}

	/* Dump the map */
	for (y = p_ptr->panel_row_min; y <= p_ptr->panel_row_max; y++) {
		dispy = y - p_ptr->panel_row_prt;

		/* Scan the columns of row "y" */
		for (x = p_ptr->panel_col_min; x <= p_ptr->panel_col_max; x++) {
			/* Determine what is there */
			map_info(Ind, y, x, &a, &c);

			/* Hack -- fake monochrome */
			if (!use_color) a = TERM_WHITE;

			dispx = x - p_ptr->panel_col_prt;

			/* Redraw that grid of the map */
			p_ptr->scr_info[dispy][dispx].c = c;
			p_ptr->scr_info[dispy][dispx].a = a;
		}

		/* Send that line of info */
		Send_line_info(Ind, dispy, scr_only);
	}

#ifdef BIG_MAP
	/* Hack to clear wrongly sent map_info artifacts from the last screen line.
	   This happens if the client starts in big screen, but the character chosen
	   for login has a small screen.
	   This hack is a bit nasty in that it assumes that PR_EXTRA is done afterwards. */
	if (p_ptr->redraw & PR_EXTRA) {
		y = p_ptr->screen_hgt + SCREEN_PAD_TOP;
		for (x = SCREEN_PAD_LEFT; x < p_ptr->screen_wid; x++) {
 #if 0
			/* Note: Clearing scr and ovl isn't required */
			p_ptr->scr_info[y][x].c = p_ptr->ovl_info[y][x].c = ' ';
			p_ptr->scr_info[y][x].a = p_ptr->ovl_info[y][x].a = TERM_DARK;
 #endif
			/* Clear wrongly sent map grid - most of these will be overwritten by the status bar anyway, but some aren't. */
			Send_char(Ind, x, y, TERM_DARK, ' ');
		}
	}
#endif

	/* Display player */
	lite_spot(Ind, p_ptr->py, p_ptr->px);
}

/* Print someone's map to his mindlinker. */
void prt_map_forward(int Ind) {
	player_type *p_ptr2 = NULL, *p_ptr = Players[Ind];
	int Ind2, y, dispy;


	/* safety checks: */

	/* We're mindlink source, not destination? Oops, we shouldn't have been called really. */
	if (p_ptr->esp_link_flags & LINKF_VIEW_DEDICATED) return;
	/* no mindlink exists? Oops, we shouldn't have been called really. */
	if (!(Ind2 = get_esp_link(Ind, LINKF_VIEW, &p_ptr2))) return;


	/* Dump the target's map into our view */
	for (y = p_ptr->panel_row_min; y <= p_ptr->panel_row_max; y++) {
		dispy = y - p_ptr->panel_row_prt;

		/* Send that line of info */
		Send_line_info_forward(Ind2, Ind, dispy);
	}

	/* Display player */
	//lite_spot(Ind, p_ptr->py, p_ptr->px);
}




/*
 * Display highest priority object in the RATIO by RATIO area
 */
/*#define RATIO 3 */

/*
 * Display the entire map
 */
#define MAP_HGT (MAX_HGT / RATIO)
#define MAP_WID (MAX_WID / RATIO)

/*
 * Hack -- priority array (see below)
 *
 * Note that all "walls" always look like "secret doors" (see "map_info()").
 */
static byte priority_table[][2] = {
	/* Dark */
	{ FEAT_NONE, 2 },

	/* Dirt */
	{ FEAT_DIRT, 3 },
	{ FEAT_SAND, 3},
	{ FEAT_MUD, 3},
	{ FEAT_SNOW, 3},

	{ FEAT_FLOWER, 3},//3 or 4?

	/* Grass */
	{ FEAT_GRASS, 4 },
	{ FEAT_ASH, 4},
	{ FEAT_ICE, 4},

	{ FEAT_LOOSE_DIRT, 4},//4 because it's used for gardens same as FEAT_CROP
	{ FEAT_CROP, 4},

	/* Tree */
	{ FEAT_BUSH, 5 },
	{ FEAT_DEAD_TREE, 6 },
	{ FEAT_TREE, 6 },

	/* Water */
	{ FEAT_DEEP_WATER, 7 },

	/* Floors */
	{ FEAT_FLOOR, 8 },

	/* Walls */
	{ FEAT_SECRET, 10 },

	/* Quartz */
	{ FEAT_QUARTZ, 11 },

	/* Magma */
	{ FEAT_MAGMA, 12 },

	/* Rubble */
	{ FEAT_RUBBLE, 13 },

	/* Open doors */
	{ FEAT_OPEN, 15 },
	{ FEAT_BROKEN, 15 },

	/* Closed doors */
	{ FEAT_DOOR_HEAD + 0x00, 17 },

	/* Hidden gold */
	{ FEAT_QUARTZ_K, 19 },
	{ FEAT_MAGMA_K, 19 },

	/* water, lava, & trees oh my! -KMW- */
	{ FEAT_DEEP_WATER, 20 },
	{ FEAT_SHAL_WATER, 20 },
	{ FEAT_DEEP_LAVA, 20 },
	{ FEAT_SHAL_LAVA, 20 },
	{ FEAT_DARK_PIT, 20 },
	{ FEAT_MOUNTAIN, 20 },

	/* Fountain */
	{ FEAT_FOUNTAIN, 21 },
	{ FEAT_FOUNTAIN_BLOOD, 21 },
	{ FEAT_EMPTY_FOUNTAIN, 21 },

	{ FEAT_SEALED_DOOR, 22 }, /* for pvp-arena */
	{ FEAT_UNSEALED_DOOR, 22},
	{ FEAT_ESCAPE_DOOR, 22}, /* for quests */

	/* Shops */
	{ FEAT_SHOP, 23 },
	//{ FEAT_SICKBAY_DOOR, 23 }, /* always near the temple, and not actively usable anyway */

	/* Void Jump Gates */
	{ FEAT_BETWEEN_TEMP, 24 },
	{ FEAT_BETWEEN, 24 },

	/* Stairs */
	{ FEAT_LESS, 25 },
	{ FEAT_MORE, 25 },

	/* Stairs */
	{ FEAT_WAY_LESS, 25 },
	{ FEAT_WAY_MORE, 25 },

	{ FEAT_SHAFT_UP, 25 },
	{ FEAT_SHAFT_DOWN, 25 },

	/* Event Beacon (Dungeon Keeper) */
	{ FEAT_BEACON, 26 },

	/* End */
	{ 0, 0 }
};


/*
 * Hack -- a priority function (see below)
 */
static byte priority(byte a, char c) {
	int i, p0, p1;

	feature_type *f_ptr;

	/* hack for shops: every shop's base door is actually '1' */
	if (c >= '1' && c <= '9') c = '1';

	/* Scan the table */
	for (i = 0; TRUE; i++) {
		/* Priority level */
		p1 = priority_table[i][1];

		/* End of table */
		if (!p1) break;

		/* Feature index */
		p0 = priority_table[i][0];

		/* Access the feature */
		f_ptr = &f_info[p0];

		/* Check character and attribute, accept matches.
		   PROBLEM: Colour can be modified for various reasons:
		   Seasons, effects, and especially _staircase type-colouring_!! - C. Blue */
#if 0 /* for this reason, let's skip the colour check for now >_> */
		if ((f_ptr->z_char == c) && (f_ptr->z_attr == a)) return (p1);
#else
		if (f_ptr->z_char == c) return (p1);
#endif
	}

	/* Default */
	return (20);
}


/*
 * Display a "small-scale" map of the dungeon in the active Term
 *
 * Note that the "map_info()" function must return fully colorized
 * data or this function will not work correctly.
 *
 * Note that this function must "disable" the special lighting
 * effects so that the "priority" function will work.
 *
 * Note the use of a specialized "priority" function to allow this
 * function to work with any graphic attr/char mappings, and the
 * attempts to optimize this function where possible.
 */


void display_map(int Ind, int *cy, int *cx) {
	player_type *p_ptr = Players[Ind];

	int i, j, x, y;

	byte ta;
	char tc;

	byte tp;

	byte ma[MAP_HGT + 2][MAP_WID + 2];
	char mc[MAP_HGT + 2][MAP_WID + 2];

	byte mp[MAP_HGT + 2][MAP_WID + 2];

	byte sa[80];
	char sc[80];

	bool old_floor_lighting;
	bool old_wall_lighting;


	/* Save lighting effects */
	old_floor_lighting = p_ptr->floor_lighting;
	old_wall_lighting = p_ptr->wall_lighting;

	/* Disable lighting effects */
	p_ptr->floor_lighting = FALSE;
	p_ptr->wall_lighting = FALSE;


	/* Clear the chars and attributes */
	memset(ma, TERM_WHITE, sizeof(ma));
	memset(mc, ' ', sizeof(mc));

	/* No priority */
	memset(mp, 0, sizeof(mp));

	/* Fill in the map */
	for (i = 0; i < p_ptr->cur_wid; ++i) {
		for (j = 0; j < p_ptr->cur_hgt; ++j) {
			/* Location */
			x = i / RATIO + 1;
			y = j / RATIO + 1;

			/* Extract the current attr/char at that map location */
			map_info(Ind, j, i, &ta, &tc);

			/* Extract the priority of that attr/char */
			tp = priority(ta, tc);
/* duplicate code, maybe create function  player_char(Ind) .. */
			/* Hack - Player(@) should always be displayed */
			if (i == p_ptr->px && j == p_ptr->py) {
				tp = 99;
				ta = player_color(Ind);

				if ((p_ptr->inventory[INVEN_BODY].tval == TV_SOFT_ARMOR) && (p_ptr->inventory[INVEN_BODY].sval == SV_COSTUME)) {
					tc = r_info[p_ptr->inventory[INVEN_BODY].bpval].d_char;
				}
				else if (p_ptr->body_monster) tc = r_info[p_ptr->body_monster].d_char;
				else if (p_ptr->fruit_bat) tc = 'b';
				else if ((( p_ptr->chp * 95)/ (p_ptr->mhp * 10)) > TURN_CHAR_INTO_NUMBER) tc = '@';
				else {
					if (p_ptr->chp < 0) tc = '-';
					else {
						int num;
						num = (p_ptr->chp * 95) / (p_ptr->mhp * 10);
						tc = '0' + num;
					}
				}
			}
/* duplicate code end */
			/* Save "best" */
			if (mp[y][x] < tp) {
				/* Save the char */
				mc[y][x] = tc;

				/* Save the attr */
				ma[y][x] = ta;

				/* Save priority */
				mp[y][x] = tp;
			}
		}
	}


	/* Corners */
	x = MAP_WID + 1;
	y = MAP_HGT + 1;

	/* Draw the corners */
	mc[0][0] = mc[0][x] = mc[y][0] = mc[y][x] = '+';

	/* Draw the horizontal edges */
	for (x = 1; x <= MAP_WID; x++) mc[0][x] = mc[y][x] = '-';

	/* Draw the vertical edges */
	for (y = 1; y <= MAP_HGT; y++) mc[y][0] = mc[y][x] = '|';


	/* Display each map line in order */
	for (y = 0; y < MAP_HGT+2; ++y) {
		/* Clear the screen buffer */
#if 0
		memset(sa, 0, sizeof(sa));
		memset(sc, 0, sizeof(sc));
#else
		/* Allow for creation of an empty border
		   (or maybe instructions on how to navigate)
		   to eg the left and right side of the map */
		memset(sa, TERM_WHITE, sizeof(sa));
		memset(sc, ' ', sizeof(sc));
#endif

     		/* Display the line */
		for (x = 0; x < MAP_WID+2; ++x) {
			ta = ma[y][x];
			tc = mc[y][x];

			/* Hack -- fake monochrome */
			if (!use_color) ta = TERM_WHITE;

#if 0
			/* Put the character into the screen buffer */
			sa[x] = ta;
			sc[x] = tc;
#else /* add a symmetrical 'border' to the left and right side of the map */
			sa[x + (80 - MAP_WID - 2) / 2] = ta;
			sc[x + (80 - MAP_WID - 2) / 2] = tc;
#endif
		}

		/* Send that line of info */
		Send_mini_map(Ind, y, sa, sc);
	}


	/* Player location */
	(*cy) = p_ptr->py / RATIO + 1;
	(*cx) = p_ptr->px / RATIO + 1;


	/* Restore lighting effects */
	p_ptr->floor_lighting = old_floor_lighting;
	p_ptr->wall_lighting = old_wall_lighting;
}


#define WILDMAP_SHOWS_STAIRS
static void wild_display_map(int Ind, char mode) {
	player_type *p_ptr = Players[Ind];

	int x, y, type;
	int max_wx, offset_x;
	int max_wy;//, offset_y;

	byte ta;
	char tc;

	/* map is displayed "full-screen" ie we can use all of the main window */
	byte ma[MAX_WINDOW_HGT][MAX_WINDOW_WID];
	char mc[MAX_WINDOW_HGT][MAX_WINDOW_WID];

	byte sa[80];
	char sc[80];

	bool old_floor_lighting;
	bool old_wall_lighting;
	struct worldpos twpos;
	twpos.wz = 0;

	byte c_dun, c_tow;
	int c_dun_diff, c_tow_diff, mmpx = 0, mmpy = 0;
	bool admin = is_admin(p_ptr), sent_pos = FALSE;


	if (CL_WINDOW_WID > MAX_WILD_X + 2) {//+ 2 for border
		offset_x = (CL_WINDOW_WID - MAX_WILD_X) / 2 - 1;//-1 for border
		max_wx = MAX_WILD_X + 2;
	} else {
		offset_x = 0;
		max_wx = CL_WINDOW_WID;
	}
	if (CL_WINDOW_HGT > MAX_WILD_Y + 2) {//+ 2 for border
		//offset_y = (CL_WINDOW_HGT - MAX_WILD_Y) / 2 - 1;//-1 for border
		max_wy = MAX_WILD_Y + 2;
	} else {
		//offset_y = 0;
		max_wy = CL_WINDOW_HGT;
	}


	/* Save lighting effects */
	old_floor_lighting = p_ptr->floor_lighting;
	old_wall_lighting = p_ptr->wall_lighting;

	/* Disable lighting effects */
	p_ptr->floor_lighting = FALSE;
	p_ptr->wall_lighting = FALSE;


	/* Clear the chars and attributes */
	memset(ma, TERM_WHITE, sizeof(ma));
	memset(mc, ' ', sizeof(mc));


	/* Modify location */
	if ((mode & ~0x1) == 0x0) {
		p_ptr->tmp_x = p_ptr->wpos.wx;
		p_ptr->tmp_y = p_ptr->wpos.wy;
	}
	if (mode & 0x2) p_ptr->tmp_y -= 9;//10..11*2
	if (mode & 0x4) p_ptr->tmp_x += 9;//31..32*2
	if (mode & 0x8) p_ptr->tmp_y += 9;//10..11*2
	if (mode & 0x10) p_ptr->tmp_x -= 9;//31..32*2
#if 0
	/* limit */
	if (p_ptr->tmp_x < 0) p_ptr->tmp_x = 0;
	if (p_ptr->tmp_y < 0) p_ptr->tmp_y = 0;
	if (p_ptr->tmp_x >= MAX_WILD_X) p_ptr->tmp_x = MAX_WILD_X - 1;
	if (p_ptr->tmp_y >= MAX_WILD_Y) p_ptr->tmp_y = MAX_WILD_Y - 1;
#else
	/* limit, align so that the map fills out the screen,
	   instead of always centering the '@-sector'. */
	if (p_ptr->tmp_x < (max_wx + 0) / 2 - 1) p_ptr->tmp_x = (max_wx + 0) / 2 - 1;
	if (p_ptr->tmp_y < (max_wy - 0) / 2 - 2) p_ptr->tmp_y = (max_wy - 0) / 2 - 2;
	if (p_ptr->tmp_x > MAX_WILD_X - (max_wx - 0) / 2 + 1) p_ptr->tmp_x = MAX_WILD_X - (max_wx - 0) / 2 + 1;
	if (p_ptr->tmp_y > MAX_WILD_Y - (max_wy + 0) / 2 - 0) p_ptr->tmp_y = MAX_WILD_Y - (max_wy + 0) / 2 - 0;
#endif


	/* for each row */
	for (y = 0; y < max_wy; y++) {
		/* for each column */
		for (x = 0; x < max_wx; x++) {
			/* Location */
			twpos.wy = p_ptr->tmp_y + (max_wy) / 2 - y;
			twpos.wx = p_ptr->tmp_x - (max_wx) / 2 + x;

			if (twpos.wy >= 0 && twpos.wy < MAX_WILD_Y && twpos.wx >= 0 && twpos.wx < MAX_WILD_X)
				type = determine_wilderness_type(&twpos);
			/* if off the map, set to unknown type */
			else type = -1;

			/* if the player hasnt been here, dont show him the terrain */
			/* Hack -- serverchez has knowledge of the full world */
			if (!p_ptr->admin_dm)
			if (!(p_ptr->wild_map[wild_idx(&twpos) / 8] & (1U << (wild_idx(&twpos) % 8)))) type = -1;
			/* hack --  the town is always known */

			switch (type) {
				case WILD_LAKE: tc = '~'; ta = TERM_BLUE; break;
				case WILD_GRASSLAND: tc = '.'; ta = TERM_GREEN; break;
				case WILD_FOREST: tc = '*'; ta = TERM_GREEN; break;
				case WILD_SWAMP:  tc = '%'; ta = TERM_VIOLET; break;
				case WILD_DENSEFOREST: tc = '*'; ta = TERM_L_DARK; break;
				case WILD_WASTELAND: tc = '.'; ta = TERM_UMBER; break;
				case WILD_TOWN: tc = 'T'; ta = TERM_YELLOW; break;
				case WILD_CLONE: tc = 'C'; ta = TERM_RED; break;
				case WILD_MOUNTAIN: tc = '^'; ta = TERM_L_DARK; break;
				case WILD_VOLCANO: tc = '^'; ta = TERM_RED; break;
				case WILD_RIVER: tc = '~'; ta = TERM_L_BLUE; break;
				case WILD_COAST: tc = ','; ta = TERM_L_UMBER; break;//TERM_YELLOW
				case WILD_OCEAN: tc = '%'; ta = TERM_BLUE; break;
				case WILD_DESERT: tc = '.'; ta = TERM_YELLOW; break;
				case WILD_ICE: tc = '.'; ta = TERM_WHITE; break;
				case -1: tc = ' '; ta = TERM_DARK; break;
				default: tc = 'O'; ta = TERM_YELLOW; break;
			}
#ifdef WILDMAP_SHOWS_STAIRS
			if (type != -1 && type != WILD_TOWN) {
				dungeon_type *dun = wild_info[twpos.wy][twpos.wx].dungeon, *tow = wild_info[twpos.wy][twpos.wx].tower;
				if (dun && !strcmp(d_info[dun->type].name + d_name, "The Shores of Valinor") && !admin) dun = NULL;
				if (tow && !strcmp(d_info[tow->type].name + d_name, "The Shores of Valinor") && !admin) tow = NULL;
 #ifdef GLOBAL_DUNGEON_KNOWLEDGE
				if (dun && !dun->known) dun = NULL;
				if (tow && !tow->known) tow = NULL;
 #endif
				if (dun) {
					if (tow) {
						tc = 'X';
						c_dun_diff = get_staircase_colour(dun, &c_dun);
						c_tow_diff = get_staircase_colour(tow, &c_tow);
						if (c_dun == c_tow) ta = c_dun;
						else if (c_dun_diff > c_tow_diff) ta = c_dun;
						else ta = c_tow;
					} else {
						tc = '>';
						get_staircase_colour(dun, &ta);
					}
				} else if (tow) {
					tc = '<';
					get_staircase_colour(tow, &ta);
				}
			}
#endif

#if 0
			/* put the @ in the center */
			if ((y == (MAP_HGT + 2) / 2) && (x == (MAP_WID + 2) / 2)) {
				tc = '@';
				ta = TERM_WHITE;
			}
#else
			/* since map is navigatable, only put @ if we are there */
			if (twpos.wx == p_ptr->wpos.wx && twpos.wy == p_ptr->wpos.wy) {
				if (is_older_than(&p_ptr->version, 4, 5, 5, 1, 0, 0)) {
					/* directly "hard-sub" it ;) */
					tc = '@';
					ta = TERM_WHITE;
				} else {
					/* overlay a blinking '@' */
					mmpx = x;
					mmpy = y;
					sent_pos = TRUE;
				}
			} else if (!sent_pos) {
				/* allow overlay on map borders, as indicator */
				if (twpos.wx == p_ptr->wpos.wx) mmpx = x;
				if (twpos.wy == p_ptr->wpos.wy) mmpy = y;
			}
#endif

			/* Save the char */
			mc[y][x] = tc;

			/* Save the attr */
			ma[y][x] = ta;
		}
	}

	/* Corners */
	x = max_wx - 1;
	y = max_wy - 1;

	/* Draw the corners */
	mc[0][0] = mc[0][x] = mc[y][0] = mc[y][x] = '+';

	/* Draw the horizontal edges */
	for (x = 1; x < max_wx; x++) mc[0][x] = mc[y][x] = '-';

	/* Draw the vertical edges */
	for (y = 1; y < max_wy; y++) mc[y][0] = mc[y][x] = '|';


	/* If we are off-screen, don't draw the usual '@' icon */
	if (!sent_pos) {
#if 0
		/* Don't draw it at all */
		Send_mini_map_pos(Ind, -1, 0, 0, 0);
#else
		/* Draw it on the map border as indicator:
		     max_wx/wy - 2 (border size of 1 in each direction).
		   Note: Does currently ignore left/right borders, since they don't exist atm. */

		int yy = p_ptr->tmp_y + max_wy / 2;

		/* On bottom border? */
		if (p_ptr->wpos.wy < yy - (max_wy - 2)) Send_mini_map_pos(Ind, mmpx + offset_x, max_wy - 1, ma[max_wy - 1][mmpx], '-');
		/* On top border? */
		else if (p_ptr->wpos.wy > yy) Send_mini_map_pos(Ind, mmpx + offset_x, 0, ma[0][mmpx], '-');
		/* On pfft? */
		else Send_mini_map_pos(Ind, -1, 0, 0, 0); /* paranoia */
#endif
	} else Send_mini_map_pos(Ind, mmpx + offset_x, mmpy, ma[mmpy][mmpx], mc[mmpy][mmpx]);


	/* Display each map line in order */
	for (y = 0; y < max_wy; ++y) {
		/* Clear the screen buffer */
#if 0
		memset(sa, 0, sizeof(sa));
		memset(sc, 0, sizeof(sc));
#else
		/* Allow for creation of an empty border
		   (or maybe instructions on how to navigate)
		   to eg the left and right side of the map */
		memset(sa, TERM_WHITE, sizeof(sa));
		memset(sc, ' ', sizeof(sc));
#endif

		/* Display the line */
		for (x = 0; x < max_wx; ++x) {
			ta = ma[y][x];
			tc = mc[y][x];

			/* Hack -- fake monochrome */
			if (!use_color) ta = TERM_WHITE;

			/* Put the character into the screen buffer */
#if 0
			sa[x] = ta;
			sc[x] = tc;
#else /* add a symmetrical 'border' to the left and right side of the map */
			sa[x + offset_x] = ta;
			sc[x + offset_x] = tc;
#endif
		}

		/* Send that line of info */
		Send_mini_map(Ind, y, sa, sc);
	}

	/* Restore lighting effects */
	p_ptr->floor_lighting = old_floor_lighting;
	p_ptr->wall_lighting = old_wall_lighting;
}


/*
 * Display a "small-scale" map of the dungeon for the player
 */

 /* in the wilderness, have several scales of maps availiable... adding one
    "wilderness map" mode now that will represent each level with one character.
 */

void do_cmd_view_map(int Ind, char mode) {
	int cy, cx;

	/* Display the map */

	/* if not in town or the dungeon, do normal map */
	/* is player in a town or dungeon? */
	/* only off floor ATM */
	if ((Players[Ind]->wpos.wz != 0 || (istown(&Players[Ind]->wpos)))
	    && !(mode & 0x1))
		display_map(Ind, &cy, &cx);
	/* do wilderness map */
	/* pfft, fix me pls, Evileye ;) */
	/* pfft. fixed */
	else wild_display_map(Ind, mode);
	/*else display_map(Ind, &cy, &cx); */
}










/*
 * Some comments on the cave grid flags.  -BEN-
 *
 *
 * One of the major bottlenecks in previous versions of Angband was in
 * the calculation of "line of sight" from the player to various grids,
 * such as monsters.  This was such a nasty bottleneck that a lot of
 * silly things were done to reduce the dependancy on "line of sight",
 * for example, you could not "see" any grids in a lit room until you
 * actually entered the room, and there were all kinds of bizarre grid
 * flags to enable this behavior.  This is also why the "call light"
 * spells always lit an entire room.
 *
 * The code below provides functions to calculate the "field of view"
 * for the player, which, once calculated, provides extremely fast
 * calculation of "line of sight from the player", and to calculate
 * the "field of torch lite", which, again, once calculated, provides
 * extremely fast calculation of "which grids are lit by the player's
 * lite source".  In addition to marking grids as "GRID_VIEW" and/or
 * "GRID_LITE", as appropriate, these functions maintain an array for
 * each of these two flags, each array containing the locations of all
 * of the grids marked with the appropriate flag, which can be used to
 * very quickly scan through all of the grids in a given set.
 *
 * To allow more "semantically valid" field of view semantics, whenever
 * the field of view (or the set of torch lit grids) changes, all of the
 * grids in the field of view (or the set of torch lit grids) are "drawn"
 * so that changes in the world will become apparent as soon as possible.
 * This has been optimized so that only grids which actually "change" are
 * redrawn, using the "temp" array and the "GRID_TEMP" flag to keep track
 * of the grids which are entering or leaving the relevent set of grids.
 *
 * These new methods are so efficient that the old nasty code was removed.
 *
 * Note that there is no reason to "update" the "viewable space" unless
 * the player "moves", or walls/doors are created/destroyed, and there
 * is no reason to "update" the "torch lit grids" unless the field of
 * view changes, or the "light radius" changes.  This means that when
 * the player is resting, or digging, or doing anything that does not
 * involve movement or changing the state of the dungeon, there is no
 * need to update the "view" or the "lite" regions, which is nice.
 *
 * Note that the calls to the nasty "los()" function have been reduced
 * to a bare minimum by the use of the new "field of view" calculations.
 *
 * I wouldn't be surprised if slight modifications to the "update_view()"
 * function would allow us to determine "reverse line-of-sight" as well
 * as "normal line-of-sight", which would allow monsters to use a more
 * "correct" calculation to determine if they can "see" the player.  For
 * now, monsters simply "cheat" somewhat and assume that if the player
 * has "line of sight" to the monster, then the monster can "pretend"
 * that it has "line of sight" to the player.
 *
 *
 * The "update_lite()" function maintains the "CAVE_LITE" flag for each
 * grid and maintains an array of all "CAVE_LITE" grids.
 *
 * This set of grids is the complete set of all grids which are lit by
 * the players light source, which allows the "player_can_see_bold()"
 * function to work very quickly.
 *
 * Note that every "CAVE_LITE" grid is also a "CAVE_VIEW" grid, and in
 * fact, the player (unless blind) can always "see" all grids which are
 * marked as "CAVE_LITE", unless they are "off screen".
 *
 *
 * The "update_view()" function maintains the "CAVE_VIEW" flag for each
 * grid and maintains an array of all "CAVE_VIEW" grids.
 *
 * This set of grids is the complete set of all grids within line of sight
 * of the player, allowing the "player_has_los_bold()" macro to work very
 * quickly.
 *
 *
 * The current "update_view()" algorithm uses the "CAVE_XTRA" flag as a
 * temporary internal flag to mark those grids which are not only in view,
 * but which are also "easily" in line of sight of the player.  This flag
 * is always cleared when we are done.
 *
 *
 * The current "update_lite()" and "update_view()" algorithms use the
 * "CAVE_TEMP" flag, and the array of grids which are marked as "CAVE_TEMP",
 * to keep track of which grids were previously marked as "CAVE_LITE" or
 * "CAVE_VIEW", which allows us to optimize the "screen updates".
 *
 * The "CAVE_TEMP" flag, and the array of "CAVE_TEMP" grids, is also used
 * for various other purposes, such as spreading lite or darkness during
 * "lite_room()" / "unlite_room()", and for calculating monster flow.
 *
 *
 * Any grid can be marked as "CAVE_GLOW" which means that the grid itself is
 * in some way permanently lit.  However, for the player to "see" anything
 * in the grid, as determined by "player_can_see()", the player must not be
 * blind, the grid must be marked as "CAVE_VIEW", and, in addition, "wall"
 * grids, even if marked as "perma lit", are only illuminated if they touch
 * a grid which is not a wall and is marked both "CAVE_GLOW" and "CAVE_VIEW".
 *
 *
 * To simplify various things, a grid may be marked as "CAVE_MARK", meaning
 * that even if the player cannot "see" the grid, he "knows" the terrain in
 * that grid.  This is used to "remember" walls/doors/stairs/floors when they
 * are "seen" or "detected", and also to "memorize" floors, after "wiz_lite()",
 * or when one of the "memorize floor grids" options induces memorization.
 *
 * Objects are "memorized" in a different way, using a special "marked" flag
 * on the object itself, which is set when an object is observed or detected.
 *
 *
 * A grid may be marked as "CAVE_ROOM" which means that it is part of a "room",
 * and should be illuminated by "lite room" and "darkness" spells.
 *
 *
 * A grid may be marked as "CAVE_ICKY" which means it is part of a "vault",
 * and should be unavailable for "teleportation" destinations.
 * Note that CAVE_ICKY is also used for other purposes like houses. - C. Blue
 *
 *
 * The "view_perma_grids" allows the player to "memorize" every perma-lit grid
 * which is observed, and the "view_torch_grids" allows the player to memorize
 * every torch-lit grid.  The player will always memorize important walls,
 * doors, stairs, and other terrain features, as well as any "detected" grids.
 *
 * Note that the new "update_view()" method allows, among other things, a room
 * to be "partially" seen as the player approaches it, with a growing cone of
 * floor appearing as the player gets closer to the door.  Also, by not turning
 * on the "memorize perma-lit grids" option, the player will only "see" those
 * floor grids which are actually in line of sight.
 *
 * And my favorite "plus" is that you can now use a special option to draw the
 * "floors" in the "viewable region" brightly (actually, to draw the *other*
 * grids dimly), providing a "pretty" effect as the player runs around, and
 * to efficiently display the "torch lite" in a special color.
 *
 *
 * Some comments on the "update_view()" algorithm...
 *
 * The algorithm is very fast, since it spreads "obvious" grids very quickly,
 * and only has to call "los()" on the borderline cases.  The major axes/diags
 * even terminate early when they hit walls.  I need to find a quick way
 * to "terminate" the other scans.
 *
 * Note that in the worst case (a big empty area with say 5% scattered walls),
 * each of the 1500 or so nearby grids is checked once, most of them getting
 * an "instant" rating, and only a small portion requiring a call to "los()".
 *
 * The only time that the algorithm appears to be "noticeably" too slow is
 * when running, and this is usually only important in town, since the town
 * provides about the worst scenario possible, with large open regions and
 * a few scattered obstructions.  There is a special "efficiency" option to
 * allow the player to reduce his field of view in town, if needed.
 *
 * In the "best" case (say, a normal stretch of corridor), the algorithm
 * makes one check for each viewable grid, and makes no calls to "los()".
 * So running in corridors is very fast, and if a lot of monsters are
 * nearby, it is much faster than the old methods.
 *
 * Note that resting, most normal commands, and several forms of running,
 * plus all commands executed near large groups of monsters, are strictly
 * more efficient with "update_view()" that with the old "compute los() on
 * demand" method, primarily because once the "field of view" has been
 * calculated, it does not have to be recalculated until the player moves
 * (or a wall or door is created or destroyed).
 *
 * Note that we no longer have to do as many "los()" checks, since once the
 * "view" region has been built, very few things cause it to be "changed"
 * (player movement, and the opening/closing of doors, changes in wall status).
 * Note that door/wall changes are only relevant when the door/wall itself is
 * in the "view" region.
 *
 * The algorithm seems to only call "los()" from zero to ten times, usually
 * only when coming down a corridor into a room, or standing in a room, just
 * misaligned with a corridor.  So if, say, there are five "nearby" monsters,
 * we will be reducing the calls to "los()".
 *
 * I am thinking in terms of an algorithm that "walks" from the central point
 * out to the maximal "distance", at each point, determining the "view" code
 * (above).  For each grid not on a major axis or diagonal, the "view" code
 * depends on the "cave_floor_bold()" and "view" of exactly two other grids
 * (the one along the nearest diagonal, and the one next to that one, see
 * "update_view_aux()"...).
 *
 * We "memorize" the viewable space array, so that at the cost of under 3000
 * bytes, we reduce the time taken by "forget_view()" to one assignment for
 * each grid actually in the "viewable space".  And for another 3000 bytes,
 * we prevent "erase + redraw" ineffiencies via the "seen" set.  These bytes
 * are also used by other routines, thus reducing the cost to almost nothing.
 *
 * A similar thing is done for "forget_lite()" in which case the savings are
 * much less, but save us from doing bizarre maintenance checking.
 *
 * In the worst "normal" case (in the middle of the town), the reachable space
 * actually reaches to more than half of the largest possible "circle" of view,
 * or about 800 grids, and in the worse case (in the middle of a dungeon level
 * where all the walls have been removed), the reachable space actually reaches
 * the theoretical maximum size of just under 1500 grids.
 *
 * Each grid G examines the "state" of two (?) other (adjacent) grids, G1 & G2.
 * If G1 is lite, G is lite.  Else if G2 is lite, G is half.  Else if G1 and G2
 * are both half, G is half.  Else G is dark.  It only takes 2 (or 4) bits to
 * "name" a grid, so (for MAX_RAD of 20) we could use 1600 bytes, and scan the
 * entire possible space (including initialization) in one step per grid.  If
 * we do the "clearing" as a separate step (and use an array of "view" grids),
 * then the clearing will take as many steps as grids that were viewed, and the
 * algorithm will be able to "stop" scanning at various points.
 * Oh, and outside of the "torch radius", only "lite" grids need to be scanned.
 */








/*
 * Actually erase the entire "lite" array, redrawing every grid
 */
void forget_lite(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i, x, y, j;

	cave_type **zcave;
	struct worldpos *wpos = &p_ptr->wpos;
	if (!(zcave = getcave(&p_ptr->wpos))) return;


	/* None to forget */
	if (!(p_ptr->lite_n)) return;

	/* Clear them all */
	for (i = 0; i < p_ptr->lite_n; i++) {
		y = p_ptr->lite_y[i];
		x = p_ptr->lite_x[i];

		/* Forget "LITE" flag */
		p_ptr->cave_flag[y][x] &= ~CAVE_LITE;
		zcave[y][x].info &= ~(CAVE_LITE | CAVE_LITE_VAMP | CAVE_LITE_WHITE);

		for (j = 1; j <= NumPlayers; j++) {
			/* Make sure player is connected */
			if (Players[j]->conn == NOT_CONNECTED)
				continue;

			/* Make sure player is on the level */
			if (!inarea(wpos, &Players[j]->wpos))
				continue;

			/* Ignore the player that we're updating */
			if (j == Ind)
				continue;

			/* If someone else also lites this spot relite it */
			if (Players[j]->cave_flag[y][x] & CAVE_LITE) {
				zcave[y][x].info |= CAVE_LITE;
				switch (Players[j]->lite_type) {
				case 1: zcave[y][x].info |= CAVE_LITE_VAMP; break;
				case 2: zcave[y][x].info |= CAVE_LITE_WHITE; break;
				}
			}
		}

		/* Redraw */
		everyone_lite_spot(wpos, y, x);
	}

	/* None left */
	p_ptr->lite_n = 0;
}


/*
 * XXX XXX XXX
 *
 * This macro allows us to efficiently add a grid to the "lite" array,
 * note that we are never called for illegal grids, or for grids which
 * have already been placed into the "lite" array, and we are never
 * called when the "lite" array is full.
 *
 * Note that I'm assuming that we can use "p_ptr", because this macro
 * should only be called from functions that have it defined at the
 * top.  --KLJ--
 */
#define cave_lite_hack(Y,X) \
    switch (p_ptr->lite_type) { \
    case 0: zcave[Y][X].info &= ~(CAVE_LITE_WHITE | CAVE_LITE_VAMP); break; \
    case 1: if (!(zcave[Y][X].info & (CAVE_LITE | CAVE_LITE_WHITE))) zcave[Y][X].info |= CAVE_LITE_VAMP; break; \
    case 2: if (!(zcave[Y][X].info & CAVE_LITE)) zcave[Y][X].info |= CAVE_LITE_WHITE; break; \
    } \
    zcave[Y][X].info |= CAVE_LITE; \
    p_ptr->cave_flag[Y][X] |= CAVE_LITE; \
    p_ptr->lite_y[p_ptr->lite_n] = (Y); \
    p_ptr->lite_x[p_ptr->lite_n] = (X); \
    p_ptr->lite_n++

/*
 * Update the set of grids "illuminated" by the player's lite.
 *
 * This routine needs to use the results of "update_view()"
 *
 * Note that "blindness" does NOT affect "torch lite".  Be careful!
 *
 * We optimize most lites (all non-artifact lites) by using "obvious"
 * facts about the results of "small" lite radius, and we attempt to
 * list the "nearby" grids before the more "distant" ones in the
 * array of torch-lit grids.
 *
 * We will correctly handle "large" radius lites, though currently,
 * it is impossible for the player to have more than radius 3 lite.
 *
 * We assume that "radius zero" lite is in fact no lite at all.
 *
 *     Torch     Lantern     Artifacts
 *     (etc)
 *                              ***
 *                 ***         *****
 *      ***       *****       *******
 *      *@*       **@**       ***@***
 *      ***       *****       *******
 *                 ***         *****
 *                              ***
 */
void update_lite(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i, x, y, min_x, max_x, min_y, max_y;

	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/*** Special case ***/

	/* Hack -- Player has no lite */
	if (p_ptr->cur_lite <= 0 && p_ptr->cur_vlite <= 0) {
		/* Forget the old lite */
		forget_lite(Ind);

		/* Draw the player */
		lite_spot(Ind, p_ptr->py, p_ptr->px);

		/* All done */
		return;
	}


	/*** Save the old "lite" grids for later ***/

	/* Clear them all */
	for (i = 0; i < p_ptr->lite_n; i++) {
		int j;

		y = p_ptr->lite_y[i];
		x = p_ptr->lite_x[i];

		/* Mark the grid as not "lite" */
		p_ptr->cave_flag[y][x] &= ~CAVE_LITE;
		zcave[y][x].info &= ~(CAVE_LITE | CAVE_LITE_VAMP | CAVE_LITE_WHITE);

		for (j = 1; j <= NumPlayers; j++) {
			/* Make sure player is connected */
			if (Players[j]->conn == NOT_CONNECTED)
				continue;

			/* Make sure player is on the level */
			if (!inarea(wpos, &Players[j]->wpos))
				continue;

			/* Ignore the player that we're updating */
			if (j == Ind)
				continue;

			/* If someone else also lites this spot relite it */
			if (Players[j]->cave_flag[y][x] & CAVE_LITE) {
				zcave[y][x].info |= CAVE_LITE;
				switch (Players[j]->lite_type) {
				case 1: zcave[y][x].info |= CAVE_LITE_VAMP; break;
				case 2: zcave[y][x].info |= CAVE_LITE_WHITE; break;
				}
			}
		}
		/* Mark the grid as "seen" */
		zcave[y][x].info |= CAVE_TEMP;

		/* Add it to the "seen" set */
		p_ptr->temp_y[p_ptr->temp_n] = y;
		p_ptr->temp_x[p_ptr->temp_n] = x;
		p_ptr->temp_n++;
	}

	/* None left */
	p_ptr->lite_n = 0;


	/*** Collect the new "lite" grids ***/

	/* Player grid */
	cave_lite_hack(p_ptr->py, p_ptr->px);

	/* Radius 1 -- torch radius */
	if (p_ptr->cur_lite >= 1 || p_ptr->cur_vlite >= 1) {
		/* Adjacent grid */
		cave_lite_hack(p_ptr->py+1, p_ptr->px);
		cave_lite_hack(p_ptr->py-1, p_ptr->px);
		cave_lite_hack(p_ptr->py, p_ptr->px+1);
		cave_lite_hack(p_ptr->py, p_ptr->px-1);

		/* Diagonal grids */
		cave_lite_hack(p_ptr->py+1, p_ptr->px+1);
		cave_lite_hack(p_ptr->py+1, p_ptr->px-1);
		cave_lite_hack(p_ptr->py-1, p_ptr->px+1);
		cave_lite_hack(p_ptr->py-1, p_ptr->px-1);
	}

	/* Radius 2 -- lantern radius */
	if (p_ptr->cur_lite >= 2 || p_ptr->cur_vlite >= 2) {
		/* South of the player */
		//if (cave_floor_bold(zcave, p_ptr->py+1, p_ptr->px))
		/* cave_los includes dark pits */
		if (cave_los(zcave, p_ptr->py+1, p_ptr->px))
		{
			cave_lite_hack(p_ptr->py+2, p_ptr->px);
			cave_lite_hack(p_ptr->py+2, p_ptr->px+1);
			cave_lite_hack(p_ptr->py+2, p_ptr->px-1);
		}

		/* North of the player */
		//if (cave_floor_bold(zcave, p_ptr->py-1, p_ptr->px))
		if (cave_los(zcave, p_ptr->py-1, p_ptr->px)) {
			cave_lite_hack(p_ptr->py-2, p_ptr->px);
			cave_lite_hack(p_ptr->py-2, p_ptr->px+1);
			cave_lite_hack(p_ptr->py-2, p_ptr->px-1);
		}

		/* East of the player */
		//if (cave_floor_bold(zcave, p_ptr->py, p_ptr->px+1))
		if (cave_los(zcave, p_ptr->py, p_ptr->px+1)) {
			cave_lite_hack(p_ptr->py, p_ptr->px+2);
			cave_lite_hack(p_ptr->py+1, p_ptr->px+2);
			cave_lite_hack(p_ptr->py-1, p_ptr->px+2);
		}

		/* West of the player */
		//if (cave_floor_bold(zcave, p_ptr->py, p_ptr->px-1))
		if (cave_los(zcave, p_ptr->py, p_ptr->px-1)) {
			cave_lite_hack(p_ptr->py, p_ptr->px-2);
			cave_lite_hack(p_ptr->py+1, p_ptr->px-2);
			cave_lite_hack(p_ptr->py-1, p_ptr->px-2);
		}
	}

	/* Radius 3+ -- artifact radius */
	if (p_ptr->cur_lite >= 3 || p_ptr->cur_vlite >= 3) {
		int d, p;

		/* Maximal radius */
		p = p_ptr->cur_lite;
		if (p_ptr->cur_vlite > p) p = p_ptr->cur_vlite;

		/* Paranoia -- see "LITE_MAX" */
		if (p > LITE_CAP) p = LITE_CAP;

		/* South-East of the player */
		//if (cave_floor_bold(zcave, p_ptr->py+1, p_ptr->px+1))
		if (cave_los(zcave, p_ptr->py+1, p_ptr->px+1)) {
			cave_lite_hack(p_ptr->py+2, p_ptr->px+2);
		}

		/* South-West of the player */
		//if (cave_floor_bold(zcave, p_ptr->py+1, p_ptr->px-1))
		if (cave_los(zcave, p_ptr->py+1, p_ptr->px-1)) {
			cave_lite_hack(p_ptr->py+2, p_ptr->px-2);
		}

		/* North-East of the player */
		//if (cave_floor_bold(zcave, p_ptr->py-1, p_ptr->px+1))
		if (cave_los(zcave, p_ptr->py-1, p_ptr->px+1)) {
			cave_lite_hack(p_ptr->py-2, p_ptr->px+2);
		}

		/* North-West of the player */
		//if (cave_floor_bold(zcave, p_ptr->py-1, p_ptr->px-1))
		if (cave_los(zcave, p_ptr->py-1, p_ptr->px-1)) {
			cave_lite_hack(p_ptr->py-2, p_ptr->px-2);
		}

		/* Maximal north */
		min_y = p_ptr->py - p;
		if (min_y < 0) min_y = 0;

		/* Maximal south */
		max_y = p_ptr->py + p;
		if (max_y > p_ptr->cur_hgt-1) max_y = p_ptr->cur_hgt-1;

		/* Maximal west */
		min_x = p_ptr->px - p;
		if (min_x < 0) min_x = 0;

		/* Maximal east */
		max_x = p_ptr->px + p;
		if (max_x > p_ptr->cur_wid-1) max_x = p_ptr->cur_wid-1;

		/* Scan the maximal box */
		for (y = min_y; y <= max_y; y++) {
			for (x = min_x; x <= max_x; x++) {
				int dy = (p_ptr->py > y) ? (p_ptr->py - y) : (y - p_ptr->py);
				int dx = (p_ptr->px > x) ? (p_ptr->px - x) : (x - p_ptr->px);

				/* Skip the "central" grids (above) */
				if ((dy <= 2) && (dx <= 2)) continue;

				/* Hack -- approximate the distance */
				d = (dy > dx) ? (dy + (dx>>1)) : (dx + (dy>>1));

				/* Skip distant grids */
				if (d > p) continue;

				/* Viewable, nearby, grids get "torch lit" */
				if (player_has_los_bold(Ind, y, x)) {
					/* This grid is "torch lit" */
					cave_lite_hack(y, x);
				}
			}
		}
	}


	/*** Complete the algorithm ***/

	/* Draw the new grids */
	for (i = 0; i < p_ptr->lite_n; i++) {
		y = p_ptr->lite_y[i];
		x = p_ptr->lite_x[i];

		/* Update fresh grids */
		if (zcave[y][x].info & CAVE_TEMP) continue;

		/* Note */
		note_spot_depth(wpos, y, x);

		/* Redraw */
		everyone_lite_spot(wpos, y, x);
	}

	/* Clear them all */
	for (i = 0; i < p_ptr->temp_n; i++) {
		y = p_ptr->temp_y[i];
		x = p_ptr->temp_x[i];

		/* No longer in the array */
		zcave[y][x].info &= ~CAVE_TEMP;

		/* Update stale grids */
		if (p_ptr->cave_flag[y][x] & CAVE_LITE) continue;

		/* Redraw */
		everyone_lite_spot(wpos, y, x);
	}

	/* None left */
	p_ptr->temp_n = 0;
}







/*
 * Clear the viewable space
 */
void forget_view(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i;

	byte *w_ptr;

	/* None to forget */
	if (!(p_ptr->view_n)) return;

	/* Clear them all */
	for (i = 0; i < p_ptr->view_n; i++) {
		int y = p_ptr->view_y[i];
		int x = p_ptr->view_x[i];

		/* Access the grid */
		w_ptr = &p_ptr->cave_flag[y][x];

		/* Forget that the grid is viewable */
		*w_ptr &= ~CAVE_VIEW;

		/* Update the screen */
		lite_spot(Ind, y, x);
	}

	/* None left */
	p_ptr->view_n = 0;
}



/*
 * This macro allows us to efficiently add a grid to the "view" array,
 * note that we are never called for illegal grids, or for grids which
 * have already been placed into the "view" array, and we are never
 * called when the "view" array is full.
 *
 * I'm again assuming that using p_ptr is OK (see above) --KLJ--
 */
#define cave_view_hack(W,Y,X) \
    (*(W)) |= CAVE_VIEW; \
    p_ptr->view_y[p_ptr->view_n] = (Y); \
    p_ptr->view_x[p_ptr->view_n] = (X); \
    p_ptr->view_n++



#ifdef USE_OLD_UPDATE_VIEW
/*
 * Helper function for "update_view()" below
 *
 * We are checking the "viewability" of grid (y,x) by the player.
 *
 * This function assumes that (y,x) is legal (i.e. on the map).
 *
 * Grid (y1,x1) is on the "diagonal" between (py,px) and (y,x)
 * Grid (y2,x2) is "adjacent", also between (py,px) and (y,x).
 *
 * Note that we are using the "CAVE_XTRA" field for marking grids as
 * "easily viewable".  This bit is cleared at the end of "update_view()".
 *
 * This function adds (y,x) to the "viewable set" if necessary.
 *
 * This function now returns "TRUE" if vision is "blocked" by grid (y,x).
 */


static bool update_view_aux(int Ind, int y, int x, int y1, int x1, int y2, int x2) {
	player_type *p_ptr = Players[Ind];
	bool f1, f2, v1, v2, z1, z2, wall;
	cave_type *c_ptr;
	byte *w_ptr;

	cave_type *g1_c_ptr;
	cave_type *g2_c_ptr;

	byte *g1_w_ptr;
	byte *g2_w_ptr;

	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;


	if (!(zcave = getcave(wpos))) return FALSE;

	if (y < 0 || y >= MAX_HGT || x < 0 || x >= MAX_WID) return(FALSE);

	/* Access the grids */
	g1_c_ptr = &zcave[y1][x1];
	g2_c_ptr = &zcave[y2][x2];

	g1_w_ptr = &p_ptr->cave_flag[y1][x1];
	g2_w_ptr = &p_ptr->cave_flag[y2][x2];


	/* Check for walls */
	f1 = (cave_los_grid(g1_c_ptr));
	f2 = (cave_los_grid(g2_c_ptr));

	/* Totally blocked by physical walls */
	if (!f1 && !f2) return (TRUE);


	/* Check for visibility */
	v1 = (f1 && (*(g1_w_ptr) & CAVE_VIEW));
	v2 = (f2 && (*(g2_w_ptr) & CAVE_VIEW));

	/* Totally blocked by "unviewable neighbors" */
	if (!v1 && !v2) return (TRUE);


	/* Access the grid */
	c_ptr = &zcave[y][x];
	w_ptr = &p_ptr->cave_flag[y][x];


	/* Check for walls */
	wall = (!cave_los_grid(c_ptr));


	/* Check the "ease" of visibility */
	z1 = (v1 && (g1_c_ptr->info & CAVE_XTRA));
	z2 = (v2 && (g2_c_ptr->info & CAVE_XTRA));

	/* Hack -- "easy" plus "easy" yields "easy" */
	if (z1 && z2) {
		c_ptr->info |= CAVE_XTRA;
		cave_view_hack(w_ptr, y, x);
		return (wall);
	}
	/* Hack -- primary "easy" yields "viewed" */
	if (z1) {
		cave_view_hack(w_ptr, y, x);
		return (wall);
	}
	/* Hack -- "view" plus "view" yields "view" */
	if (v1 && v2) {
		/* c_ptr->info |= CAVE_XTRA; */
		cave_view_hack(w_ptr, y, x);
		return (wall);
	}
	/* Mega-Hack -- the "los()" function works poorly on walls */
	if (wall) {
		cave_view_hack(w_ptr, y, x);
		return (wall);
	}
	/* Hack -- check line of sight */
	if (los(wpos, p_ptr->py, p_ptr->px, y, x)) {
		cave_view_hack(w_ptr, y, x);
		return (wall);
	}


	/* Assume no line of sight. */
	return (TRUE);
}



/*
 * Calculate the viewable space
 *
 *  1: Process the player
 *  1a: The player is always (easily) viewable
 *  2: Process the diagonals
 *  2a: The diagonals are (easily) viewable up to the first wall
 *  2b: But never go more than 2/3 of the "full" distance
 *  3: Process the main axes
 *  3a: The main axes are (easily) viewable up to the first wall
 *  3b: But never go more than the "full" distance
 *  4: Process sequential "strips" in each of the eight octants
 *  4a: Each strip runs along the previous strip
 *  4b: The main axes are "previous" to the first strip
 *  4c: Process both "sides" of each "direction" of each strip
 *  4c1: Each side aborts as soon as possible
 *  4c2: Each side tells the next strip how far it has to check
 *
 * Note that the octant processing involves some pretty interesting
 * observations involving when a grid might possibly be viewable from
 * a given grid, and on the order in which the strips are processed.
 *
 * Note the use of the mathematical facts shown below, which derive
 * from the fact that (1 < sqrt(2) < 1.5), and that the length of the
 * hypotenuse of a right triangle is primarily determined by the length
 * of the longest side, when one side is small, and is strictly less
 * than one-and-a-half times as long as the longest side when both of
 * the sides are large.
 *
 *   if (manhatten(dy,dx) < R) then (hypot(dy,dx) < R)
 *   if (manhatten(dy,dx) > R*3/2) then (hypot(dy,dx) > R)
 *
 *   hypot(dy,dx) is approximated by (dx+dy+MAX(dx,dy)) / 2
 *
 * These observations are important because the calculation of the actual
 * value of "hypot(dx,dy)" is extremely expensive, involving square roots,
 * while for small values (up to about 20 or so), the approximations above
 * are correct to within an error of at most one grid or so.
 *
 * Observe the use of "full" and "over" in the code below, and the use of
 * the specialized calculation involving "limit", all of which derive from
 * the observations given above.  Basically, we note that the "circle" of
 * view is completely contained in an "octagon" whose bounds are easy to
 * determine, and that only a few steps are needed to derive the actual
 * bounds of the circle given the bounds of the octagon.
 *
 * Note that by skipping all the grids in the corners of the octagon, we
 * place an upper limit on the number of grids in the field of view, given
 * that "full" is never more than 20.  Of the 1681 grids in the "square" of
 * view, only about 1475 of these are in the "octagon" of view, and even
 * fewer are in the "circle" of view, so 1500 or 1536 is more than enough
 * entries to completely contain the actual field of view.
 *
 * Note also the care taken to prevent "running off the map".  The use of
 * explicit checks on the "validity" of the "diagonal", and the fact that
 * the loops are never allowed to "leave" the map, lets "update_view_aux()"
 * use the optimized "cave_floor_bold()" macro, and to avoid the overhead
 * of multiple checks on the validity of grids.
 *
 * Note the "optimizations" involving the "se","sw","ne","nw","es","en",
 * "ws","wn" variables.  They work like this: While travelling down the
 * south-bound strip just to the east of the main south axis, as soon as
 * we get to a grid which does not "transmit" viewing, if all of the strips
 * preceding us (in this case, just the main axis) had terminated at or before
 * the same point, then we can stop, and reset the "max distance" to ourself.
 * So, each strip (named by major axis plus offset, thus "se" in this case)
 * maintains a "blockage" variable, initialized during the main axis step,
 * and checks it whenever a blockage is observed.  After processing each
 * strip as far as the previous strip told us to process, the next strip is
 * told not to go farther than the current strip's farthest viewable grid,
 * unless open space is still available.  This uses the "k" variable.
 *
 * Note the use of "inline" macros for efficiency.  The "cave_floor_grid()"
 * macro is a replacement for "cave_floor_bold()" which takes a pointer to
 * a cave grid instead of its location.  The "cave_view_hack()" macro is a
 * chunk of code which adds the given location to the "view" array if it
 * is not already there, using both the actual location and a pointer to
 * the cave grid.  See above.
 *
 * By the way, the purpose of this code is to reduce the dependancy on the
 * "los()" function which is slow, and, in some cases, not very accurate.
 *
 * It is very possible that I am the only person who fully understands this
 * function, and for that I am truly sorry, but efficiency was very important
 * and the "simple" version of this function was just not fast enough.  I am
 * more than willing to replace this function with a simpler one, if it is
 * equally efficient, and especially willing if the new function happens to
 * derive "reverse-line-of-sight" at the same time, since currently monsters
 * just use an optimized hack of "you see me, so I see you", and then use the
 * actual "projectable()" function to check spell attacks.
 *
 * Well, I for one don't understand it, so I'm hoping I didn't screw anything
 * up while trying to do this. --KLJ--
 */

 /* Hmm, this function doesn't seem to be very "mangworld" friendly...
    lets add some speedbumps/sanity checks.
    -APD-

    With my new "invisible wall" code this shouldn't be neccecary.

 */

/* TODO: Hrm, recent variants seem to get better algorithm
 * - let's port them! */

void update_view(int Ind) {
	player_type *p_ptr = Players[Ind];
	int n, m, d, k, y, x, z;
	int se, sw, ne, nw, es, en, ws, wn;
	int full, over;

	int y_max = MAX_HGT - 1;
	int x_max = MAX_WID - 1;

	cave_type *c_ptr;
	byte *w_ptr;
	bool unmap = FALSE;

	cave_type **zcave;
	struct worldpos *wpos;
	wpos = &p_ptr->wpos;


	if (!(zcave = getcave(wpos))) return;

	if (p_ptr->wpos.wz) {
		dun_level *l_ptr = getfloor(&p_ptr->wpos);
		if (l_ptr && l_ptr->flags1 & LF1_NO_MAP) unmap = TRUE;
	}


	/*** Initialize ***/

	/* Optimize */
	if (p_ptr->view_reduce_view && istown(wpos)) { /* town */
		/* Full radius (10) */
		full = MAX_SIGHT / 2;

		/* Octagon factor (15) */
		over = MAX_SIGHT * 3 / 4;
	}
	/* Normal */
	else {
		/* Full radius (20) */
		full = MAX_SIGHT;

		/* Octagon factor (30) */
		over = MAX_SIGHT * 3 / 2;
	}

	/*** Step 0 -- Begin ***/

	/* Save the old "view" grids for later */
	for (n = 0; n < p_ptr->view_n; n++) {
		y = p_ptr->view_y[n];
		x = p_ptr->view_x[n];

		/* Access the grid */
		c_ptr = &zcave[y][x];
		w_ptr = &p_ptr->cave_flag[y][x];

		/* Mark the grid as not in "view" */
		*w_ptr &= ~(CAVE_VIEW);

		/* Mark the grid as "seen" */
		c_ptr->info |= CAVE_TEMP;

		/* Add it to the "seen" set */
		p_ptr->temp_y[p_ptr->temp_n] = y;
		p_ptr->temp_x[p_ptr->temp_n] = x;
		p_ptr->temp_n++;
	}

	/* Start over with the "view" array */
	p_ptr->view_n = 0;


	/*** Step 1 -- adjacent grids ***/

	/* Now start on the player */
	y = p_ptr->py;
	x = p_ptr->px;

	/* Access the grid */
	c_ptr = &zcave[y][x];
	w_ptr = &p_ptr->cave_flag[y][x];

	/* Assume the player grid is easily viewable */
	c_ptr->info |= CAVE_XTRA;

	/* Assume the player grid is viewable */
	cave_view_hack(w_ptr, y, x);


	/*** Step 2 -- Major Diagonals ***/

	/* Hack -- Limit */
	z = full * 2 / 3;

	/* Scan south-east */
	for (d = 1; d <= z; d++) {
		if (y + d >= MAX_HGT) break;
		c_ptr = &zcave[y+d][x+d];
		w_ptr = &p_ptr->cave_flag[y+d][x+d];
		c_ptr->info |= CAVE_XTRA;
		cave_view_hack(w_ptr, y+d, x+d);
		if (!cave_los_grid(c_ptr)) break;
	}

	/* Scan south-west */
	for (d = 1; d <= z; d++) {
		if (y + d >= MAX_HGT) break;
		c_ptr = &zcave[y+d][x-d];
		w_ptr = &p_ptr->cave_flag[y+d][x-d];
		c_ptr->info |= CAVE_XTRA;
		cave_view_hack(w_ptr, y+d, x-d);
		if (!cave_los_grid(c_ptr)) break;
	}

	/* Scan north-east */
	for (d = 1; d <= z; d++) {
		if (d > y) break;
		c_ptr = &zcave[y-d][x+d];
		w_ptr = &p_ptr->cave_flag[y-d][x+d];
		c_ptr->info |= CAVE_XTRA;
		cave_view_hack(w_ptr, y-d, x+d);
		if (!cave_los_grid(c_ptr)) break;
	}

	/* Scan north-west */
	for (d = 1; d <= z; d++) {
		if (d > y) break;
		c_ptr = &zcave[y-d][x-d];
		w_ptr = &p_ptr->cave_flag[y-d][x-d];
		c_ptr->info |= CAVE_XTRA;
		cave_view_hack(w_ptr, y-d, x-d);
		if (!cave_los_grid(c_ptr)) break;
	}


	/*** Step 3 -- major axes ***/

	/* Scan south */
	for (d = 1; d <= full; d++) {
		/*if (y + d >= MAX_HGT) break;*/
		c_ptr = &zcave[y+d][x];
		w_ptr = &p_ptr->cave_flag[y+d][x];
		c_ptr->info |= CAVE_XTRA;
		cave_view_hack(w_ptr, y+d, x);
		if (!cave_los_grid(c_ptr)) break;
	}

	/* Initialize the "south strips" */
	se = sw = d;

	/* Scan north */
	for (d = 1; d <= full; d++) {
		/*if (d > y) break;*/
		c_ptr = &zcave[y-d][x];
		w_ptr = &p_ptr->cave_flag[y-d][x];
		c_ptr->info |= CAVE_XTRA;
		cave_view_hack(w_ptr, y-d, x);
		if (!cave_los_grid(c_ptr)) break;
	}

	/* Initialize the "north strips" */
	ne = nw = d;

	/* Scan east */
	for (d = 1; d <= full; d++) {
		c_ptr = &zcave[y][x+d];
		w_ptr = &p_ptr->cave_flag[y][x+d];
		c_ptr->info |= CAVE_XTRA;
		cave_view_hack(w_ptr, y, x+d);
		if (!cave_los_grid(c_ptr)) break;
	}

	/* Initialize the "east strips" */
	es = en = d;

	/* Scan west */
	for (d = 1; d <= full; d++) {
		c_ptr = &zcave[y][x-d];
		w_ptr = &p_ptr->cave_flag[y][x-d];
		c_ptr->info |= CAVE_XTRA;
		cave_view_hack(w_ptr, y, x-d);
		if (!cave_los_grid(c_ptr)) break;
	}

	/* Initialize the "west strips" */
	ws = wn = d;


	/*** Step 4 -- Divide each "octant" into "strips" ***/

	/* Now check each "diagonal" (in parallel) */
	for (n = 1; n <= over / 2; n++) {
		int ypn, ymn, xpn, xmn;


		/* Acquire the "bounds" of the maximal circle */
		z = over - n - n;
		if (z > full - n) z = full - n;
		while ((z + n + (n>>1)) > full) z--;


		/* Access the four diagonal grids */
		ypn = y + n;
		ymn = y - n;
		xpn = x + n;
		xmn = x - n;


		/* South strip */
		if (ypn < y_max) {
			/* Maximum distance */
			m = MIN(z, y_max - ypn);

			/* East side */
			if ((xpn <= x_max) && (n < se)) {
				/* Scan */
				for (k = n, d = 1; d <= m; d++) {
					/*if (ypn + d >= MAX_HGT) break; */

					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(Ind, ypn+d, xpn, ypn+d-1, xpn-1, ypn+d-1, xpn)) {
						if (n + d >= se) break;
					}
					/* Track most distant "non-blockage" */
					else k = n + d;
				}

				/* Limit the next strip */
				se = k + 1;
			}

			/* West side */
			if ((xmn >= 0) && (n < sw)) {
				/* Scan */
				for (k = n, d = 1; d <= m; d++) {
					/*if (ypn + d >= MAX_HGT) break;*/

					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(Ind, ypn+d, xmn, ypn+d-1, xmn+1, ypn+d-1, xmn)) {
						if (n + d >= sw) break;
					}
					/* Track most distant "non-blockage" */
					else k = n + d;
				}

				/* Limit the next strip */
				sw = k + 1;
			}
		}


		/* North strip */
		if (ymn > 0) {
			/* Maximum distance */
			m = MIN(z, ymn);

			/* East side */
			if ((xpn <= x_max) && (n < ne)) {
				/* Scan */
				for (k = n, d = 1; d <= m; d++) {
					/*if (d > ymn) break;*/

					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(Ind, ymn-d, xpn, ymn-d+1, xpn-1, ymn-d+1, xpn)) {
						if (n + d >= ne) break;
					}
					/* Track most distant "non-blockage" */
					else k = n + d;
				}

				/* Limit the next strip */
				ne = k + 1;
			}

			/* West side */
			if ((xmn >= 0) && (n < nw)) {
				/* Scan */
				for (k = n, d = 1; d <= m; d++) {
					/*if (d > ymn) break;*/

					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(Ind, ymn-d, xmn, ymn-d+1, xmn+1, ymn-d+1, xmn)) {
						if (n + d >= nw) break;
					}
					/* Track most distant "non-blockage" */
					else k = n + d;
				}

				/* Limit the next strip */
				nw = k + 1;
			}
		}


		/* East strip */
		if (xpn < x_max) {
			/* Maximum distance */
			m = MIN(z, x_max - xpn);

			/* South side */
			if ((ypn <= x_max) && (n < es)) {
				/* Scan */
				for (k = n, d = 1; d <= m; d++) {
					/*if (ypn >= MAX_HGT) break;*/

					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(Ind, ypn, xpn+d, ypn-1, xpn+d-1, ypn, xpn+d-1)) {
						if (n + d >= es) break;
					}
					/* Track most distant "non-blockage" */
					else k = n + d;
				}

				/* Limit the next strip */
				es = k + 1;
			}

			/* North side */
			if ((ymn >= 0) && (n < en)) {
				/* Scan */
				for (k = n, d = 1; d <= m; d++) {
					/*if (ymn >= MAX_HGT) break;*/

					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(Ind, ymn, xpn+d, ymn+1, xpn+d-1, ymn, xpn+d-1)) {
						if (n + d >= en) break;
					}
					/* Track most distant "non-blockage" */
					else k = n + d;
				}

				/* Limit the next strip */
				en = k + 1;
			}
		}


		/* West strip */
		if (xmn > 0) {
			/* Maximum distance */
			m = MIN(z, xmn);

			/* South side */
			if ((ypn <= y_max) && (n < ws)) {
				/* Scan */
				for (k = n, d = 1; d <= m; d++) {
					/*if (ypn >= MAX_HGT) break;*/

					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(Ind, ypn, xmn-d, ypn-1, xmn-d+1, ypn, xmn-d+1)) {
						if (n + d >= ws) break;
					}
					/* Track most distant "non-blockage" */
					else k = n + d;
				}

				/* Limit the next strip */
				ws = k + 1;
			}

			/* North side */
			if ((ymn >= 0) && (n < wn)) {
				/* Scan */
				for (k = n, d = 1; d <= m; d++) {
					/*if (ymn >= MAX_HGT) break;*/

					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(Ind, ymn, xmn-d, ymn+1, xmn-d+1, ymn, xmn-d+1)) {
						if (n + d >= wn) break;
					}
					/* Track most distant "non-blockage" */
					else k = n + d;
				}

				/* Limit the next strip */
				wn = k + 1;
			}
		}
	}


	/*** Step 5 -- Complete the algorithm ***/

	/* Update all the new grids */
	for (n = 0; n < p_ptr->view_n; n++) {
		y = p_ptr->view_y[n];
		x = p_ptr->view_x[n];

		/* Access the grid */
		c_ptr = &zcave[y][x];

		/* Clear the "CAVE_XTRA" flag */
		c_ptr->info &= ~CAVE_XTRA;

		/* Update only newly viewed grids */
		if (c_ptr->info & CAVE_TEMP) continue;

		/* Note */
		note_spot(Ind, y, x);

		/* Redraw */
		lite_spot(Ind, y, x);
	}

	/* Wipe the old grids, update as needed */
	for (n = 0; n < p_ptr->temp_n; n++) {
		y = p_ptr->temp_y[n];
		x = p_ptr->temp_x[n];

		/* Access the grid */
		c_ptr = &zcave[y][x];
		w_ptr = &p_ptr->cave_flag[y][x];

		/* No longer in the array */
		c_ptr->info &= ~CAVE_TEMP;

		/* Update only non-viewable grids */
		if (*w_ptr & CAVE_VIEW) continue;

		/* Forget it, dude */
		if (unmap) {
			u16b this_o_idx, next_o_idx = 0;

			*w_ptr &= ~CAVE_MARK;

			/* make player forget of objects too */
			/* too bad, traps cannot be forgotten this way.. */
			for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
				/* Acquire next object */
				next_o_idx = o_list[this_o_idx].next_o_idx;

				/* Forget the object */
				p_ptr->obj_vis[this_o_idx] = FALSE;
			}
		}

		/* Redraw */
		lite_spot(Ind, y, x);
	}

	/* None left */
	p_ptr->temp_n = 0;
}
#else	/* USE_OLD_UPDATE_VIEW */
/* New update_view code from ToME... but it was slower than ours in fact.
 * pfft
 */

/*
 * Maximum number of grids in a single octant
 */
#define VINFO_MAX_GRIDS 161


/*
 * Maximum number of slopes in a single octant
 */
#define VINFO_MAX_SLOPES 126


/*
 * Mask of bits used in a single octant
 */
#define VINFO_BITS_3 0x3FFFFFFF
#define VINFO_BITS_2 0xFFFFFFFF
#define VINFO_BITS_1 0xFFFFFFFF
#define VINFO_BITS_0 0xFFFFFFFF


/*
 * Forward declare
 */
typedef struct vinfo_type vinfo_type;


/*
 * The 'vinfo_type' structure
 */
struct vinfo_type
{
	s16b grid_y[8];
	s16b grid_x[8];

	u32b bits_3;
	u32b bits_2;
	u32b bits_1;
	u32b bits_0;

	vinfo_type *next_0;
	vinfo_type *next_1;

	byte y;
	byte x;
	byte d;
	byte r;
};



/*
 * The array of "vinfo" objects, initialized by "vinfo_init()"
 */
static vinfo_type vinfo[VINFO_MAX_GRIDS];




/*
 * Slope scale factor
 */
#define SCALE 100000L


/*
 * Forward declare
 */
typedef struct vinfo_hack vinfo_hack;


/*
 * Temporary data used by "vinfo_init()"
 *
 *	- Number of grids
 *
 *	- Number of slopes
 *
 *	- Slope values
 *
 *	- Slope range per grid
 */
struct vinfo_hack {

	int num_slopes;

	long slopes[VINFO_MAX_SLOPES];

	long slopes_min[MAX_SIGHT+1][MAX_SIGHT+1];
	long slopes_max[MAX_SIGHT+1][MAX_SIGHT+1];
};



/*
 * Sorting hook -- comp function -- array of long's (see below)
 *
 * We use "u" to point to an array of long integers.
 */
/* Ind is utter dummy. */
static bool ang_sort_comp_hook_longs(int Ind, vptr u, vptr v, int a, int b)
{
	long *x = (long*)(u);

	return (x[a] <= x[b]);
}


/*
 * Sorting hook -- comp function -- array of long's (see below)
 *
 * We use "u" to point to an array of long integers.
 */
static void ang_sort_swap_hook_longs(int Ind, vptr u, vptr v, int a, int b)
{
	long *x = (long*)(u);

        long temp;

        /* Swap */
        temp = x[a];
        x[a] = x[b];
        x[b] = temp;
}



/*
 * Save a slope
 */
static void vinfo_init_aux(vinfo_hack *hack, int y, int x, long m)
{
	int i;

	/* Handle "legal" slopes */
	if ((m > 0) && (m <= SCALE))
	{
		/* Look for that slope */
		for (i = 0; i < hack->num_slopes; i++)
		{
			if (hack->slopes[i] == m) break;
		}

		/* New slope */
		if (i == hack->num_slopes)
		{
			/* Paranoia */
			if (hack->num_slopes >= VINFO_MAX_SLOPES)
			{
				quit_fmt("Too many slopes (%d)!",
			         	VINFO_MAX_SLOPES);
			}

			/* Save the slope, and advance */
			hack->slopes[hack->num_slopes++] = m;
		}
	}

	/* Track slope range */
	if (hack->slopes_min[y][x] > m) hack->slopes_min[y][x] = m;
	if (hack->slopes_max[y][x] < m) hack->slopes_max[y][x] = m;
}



/*
 * Initialize the "vinfo" array
 *
 * Full Octagon (radius 20), Grids=1149
 *
 * Quadrant (south east), Grids=308, Slopes=251
 *
 * Octant (east then south), Grids=161, Slopes=126
 *
 * This function assumes that VINFO_MAX_GRIDS and VINFO_MAX_SLOPES
 * have the correct values, which can be derived by setting them to
 * a number which is too high, running this function, and using the
 * error messages to obtain the correct values.
 */
errr vinfo_init(void)
{
	int i, y, x;

	long m;

	vinfo_hack *hack;

	int num_grids = 0;

	int queue_head = 0;
	int queue_tail = 0;
	vinfo_type *queue[VINFO_MAX_GRIDS*2];


	/* Make hack */
	MAKE(hack, vinfo_hack);


	/* Analyze grids */
	for (y = 0; y <= MAX_SIGHT; ++y)
	{
		for (x = y; x <= MAX_SIGHT; ++x)
		{
			/* Skip grids which are out of sight range */
			if (distance(0, 0, y, x) > MAX_SIGHT) continue;

			/* Default slope range */
			hack->slopes_min[y][x] = 999999999;
			hack->slopes_max[y][x] = 0;

			/* Paranoia */
			if (num_grids >= VINFO_MAX_GRIDS)
			{
				quit_fmt("Too many grids (%d >= %d)!",
				         num_grids, VINFO_MAX_GRIDS);
			}

			/* Count grids */
			num_grids++;

			/* Slope to the top right corner */
			m = SCALE * (1000L * y - 500) / (1000L * x + 500);

			/* Handle "legal" slopes */
			vinfo_init_aux(hack, y, x, m);

			/* Slope to top left corner */
			m = SCALE * (1000L * y - 500) / (1000L * x - 500);

			/* Handle "legal" slopes */
			vinfo_init_aux(hack, y, x, m);

			/* Slope to bottom right corner */
			m = SCALE * (1000L * y + 500) / (1000L * x + 500);

			/* Handle "legal" slopes */
			vinfo_init_aux(hack, y, x, m);

			/* Slope to bottom left corner */
			m = SCALE * (1000L * y + 500) / (1000L * x - 500);

			/* Handle "legal" slopes */
			vinfo_init_aux(hack, y, x, m);
		}
	}


	/* Enforce maximal efficiency */
	if (num_grids < VINFO_MAX_GRIDS)
	{
		quit_fmt("Too few grids (%d < %d)!",
		         num_grids, VINFO_MAX_GRIDS);
	}

	/* Enforce maximal efficiency */
	if (hack->num_slopes < VINFO_MAX_SLOPES)
	{
		quit_fmt("Too few slopes (%d < %d)!",
		         hack->num_slopes, VINFO_MAX_SLOPES);
	}


	/* Sort slopes numerically */
	ang_sort_comp = ang_sort_comp_hook_longs;

	/* Sort slopes numerically */
	ang_sort_swap = ang_sort_swap_hook_longs;

	/* Sort the (unique) slopes */
	ang_sort(0, hack->slopes, NULL, hack->num_slopes);



	/* Enqueue player grid */
	queue[queue_tail++] = &vinfo[0];

	/* Process queue */
	while (queue_head < queue_tail)
	{
		int e;

		vinfo_type *p;


		/* Index */
		e = queue_head;

		/* Dequeue next grid */
		p = queue[queue_head++];

		/* Location of main grid */
		y = vinfo[e].grid_y[0];
		x = vinfo[e].grid_x[0];


		/* Compute grid offsets */
		vinfo[e].grid_y[0] = +y; vinfo[e].grid_x[0] = +x;
		vinfo[e].grid_y[1] = +x; vinfo[e].grid_x[1] = +y;
		vinfo[e].grid_y[2] = +x; vinfo[e].grid_x[2] = -y;
		vinfo[e].grid_y[3] = +y; vinfo[e].grid_x[3] = -x;
		vinfo[e].grid_y[4] = -y; vinfo[e].grid_x[4] = -x;
		vinfo[e].grid_y[5] = -x; vinfo[e].grid_x[5] = -y;
		vinfo[e].grid_y[6] = -x; vinfo[e].grid_x[6] = +y;
		vinfo[e].grid_y[7] = -y; vinfo[e].grid_x[7] = +x;


		/* Analyze slopes */
		for (i = 0; i < hack->num_slopes; ++i)
		{
			m = hack->slopes[i];

			/* Memorize intersection slopes (for non-player-grids) */
			if ((e > 0) &&
			    (hack->slopes_min[y][x] < m) &&
			    (m < hack->slopes_max[y][x]))
			{
				switch (i / 32)
				{
					case 3: vinfo[e].bits_3 |= (1U << (i % 32)); break;
					case 2: vinfo[e].bits_2 |= (1U << (i % 32)); break;
					case 1: vinfo[e].bits_1 |= (1U << (i % 32)); break;
					case 0: vinfo[e].bits_0 |= (1U << (i % 32)); break;
				}
			}
		}


		/* Default */
		vinfo[e].next_0 = &vinfo[0];

		/* Grid next child */
		if (distance(0, 0, y, x+1) <= MAX_SIGHT)
		{
			if ((queue[queue_tail-1]->grid_y[0] != y) ||
			    (queue[queue_tail-1]->grid_x[0] != x + 1))
			{
				vinfo[queue_tail].grid_y[0] = y;
				vinfo[queue_tail].grid_x[0] = x + 1;
				queue[queue_tail] = &vinfo[queue_tail];
				queue_tail++;
			}

			vinfo[e].next_0 = &vinfo[queue_tail - 1];
		}


		/* Default */
		vinfo[e].next_1 = &vinfo[0];

		/* Grid diag child */
		if (distance(0, 0, y+1, x+1) <= MAX_SIGHT)
		{
			if ((queue[queue_tail-1]->grid_y[0] != y + 1) ||
			    (queue[queue_tail-1]->grid_x[0] != x + 1))
			{
				vinfo[queue_tail].grid_y[0] = y + 1;
				vinfo[queue_tail].grid_x[0] = x + 1;
				queue[queue_tail] = &vinfo[queue_tail];
				queue_tail++;
			}

			vinfo[e].next_1 = &vinfo[queue_tail - 1];
		}


		/* Hack -- main diagonal has special children */
		if (y == x) vinfo[e].next_0 = vinfo[e].next_1;


		/* Extra values */
		vinfo[e].y = y;
		vinfo[e].x = x;
		vinfo[e].d = ((y > x) ? (y + x/2) : (x + y/2));
		vinfo[e].r = ((!y) ? x : (!x) ? y : (y == x) ? y : 0);
	}


	/* Verify maximal bits XXX XXX XXX */
	if (((vinfo[1].bits_3 | vinfo[2].bits_3) != VINFO_BITS_3) ||
	    ((vinfo[1].bits_2 | vinfo[2].bits_2) != VINFO_BITS_2) ||
	    ((vinfo[1].bits_1 | vinfo[2].bits_1) != VINFO_BITS_1) ||
	    ((vinfo[1].bits_0 | vinfo[2].bits_0) != VINFO_BITS_0))
	{
		quit("Incorrect bit masks!");
	}


	/* Kill hack */
	KILL(hack, vinfo_hack);


	/* Success */
	return (0);
}


void update_view(int Ind)
{
	player_type *p_ptr = Players[Ind];


	int full, over;
	int o, n;
	int y, x;
	u32b info;

#if 0
	int y_max = p_ptr->cur_hgt - 1;
	int x_max = p_ptr->cur_wid - 1;
#else	/* Inefficient, but prevents south-west dead angle bug.. */
	int y_max = MAX_HGT - 1;
	int x_max = MAX_WID - 1;
#endif	/* 0 */

	int py = p_ptr->py;
	int px = p_ptr->px;


	cave_type *c_ptr;
	byte *w_ptr;
	bool unmap = FALSE;

	cave_type **zcave;
	struct worldpos *wpos;
	wpos = &p_ptr->wpos;
	if (!(zcave = getcave(wpos))) return;
	if (p_ptr->wpos.wz) {
		dun_level *l_ptr = getfloor(&p_ptr->wpos);
		if (l_ptr && l_ptr->flags1 & LF1_NO_MAP) unmap = TRUE;
	}


	/*** Initialize ***/

#if 0
	/* Optimize */
	if (p_ptr->view_reduce_view && istown(wpos)) /* town */
	{
		/* Full radius (10) */
		full = MAX_SIGHT / 2;

		/* Octagon factor (15) */
		over = MAX_SIGHT * 3 / 4;
	}

	/* Normal */
	else
	{
		/* Full radius (20) */
		full = MAX_SIGHT;

		/* Octagon factor (30) */
		over = MAX_SIGHT * 3 / 2;
	}
#endif	// 0

	/* It's needed, none..? XXX */
	p_ptr->temp_n = 0;

	/*** Step 0 -- Begin ***/

	/* Save the old "view" grids for later */
	for (n = 0; n < p_ptr->view_n; n++)
	{
		y = p_ptr->view_y[n];
		x = p_ptr->view_x[n];

		/* Access the grid */
		c_ptr = &zcave[y][x];
		w_ptr = &p_ptr->cave_flag[y][x];

		/* Mark the grid as not in "view" */
		*w_ptr &= ~(CAVE_VIEW);

		/* Save "CAVE_SEEN" grids */
		if (*w_ptr & (CAVE_XTRA))
		{
			/* Set "CAVE_TEMP" flag */
			info |= (CAVE_TEMP);

			/* Save grid for later */
			p_ptr->temp_y[p_ptr->temp_n] = y;
			p_ptr->temp_x[p_ptr->temp_n++] = x;
		}

		/* Mark the grid as "seen" */
		*w_ptr &= ~(CAVE_XTRA);
	}

	/* Start over with the "view" array */
	p_ptr->view_n = 0;


	/*** Step 1 -- adjacent grids ***/

	/* Now start on the player */
	y = py;
	x = px;

	/* Access the grid */
	c_ptr = &zcave[y][x];
	w_ptr = &p_ptr->cave_flag[y][x];

	/* Assume the player grid is easily viewable */
/*	c_ptr->info |= CAVE_XTRA; */

	/* Assume the player grid is viewable */
	cave_view_hack(w_ptr, y, x);

	if (c_ptr->info & (CAVE_GLOW | CAVE_LITE))
	{
		/* Mark as "CAVE_SEEN" */
		*w_ptr |= (CAVE_XTRA);
	}


	/*** Step 2 -- octants ***/

	/* Scan each octant */
	for (o = 0; o < 8; o++)
	{
		vinfo_type *p;

		/* Last added */
		vinfo_type *last = &vinfo[0];

		/* Grid queue */
		int queue_head = 0;
		int queue_tail = 0;
		vinfo_type *queue[VINFO_MAX_GRIDS*2];

		/* Slope bit vector */
		u32b bits0 = VINFO_BITS_0;
		u32b bits1 = VINFO_BITS_1;
		u32b bits2 = VINFO_BITS_2;
		u32b bits3 = VINFO_BITS_3;

		/* Reset queue */
		queue_head = queue_tail = 0;

		/* Initial grids */
		queue[queue_tail++] = &vinfo[1];
		queue[queue_tail++] = &vinfo[2];

		/* Process queue */
		while (queue_head < queue_tail)
		{
			/* Dequeue next grid */
			p = queue[queue_head++];

			/* Check bits */
			if ((bits0 & (p->bits_0)) ||
			    (bits1 & (p->bits_1)) ||
			    (bits2 & (p->bits_2)) ||
			    (bits3 & (p->bits_3)))
			{
				/* Extract coordinate value */
				y = py + p->grid_y[o];
				x = px + p->grid_x[o];

				/* Access the grid */
				c_ptr = &zcave[y][x];
				w_ptr = &p_ptr->cave_flag[y][x];

				/* Get grid info */
				info = c_ptr->info;

				/* Handle wall */
/*				if (info & (CAVE_WALL)) */
				if (!cave_los_grid(c_ptr))
				{
					/* Clear bits */
					bits0 &= ~(p->bits_0);
					bits1 &= ~(p->bits_1);
					bits2 &= ~(p->bits_2);
					bits3 &= ~(p->bits_3);

					/* Newly viewable wall */
					if (!(*w_ptr & (CAVE_VIEW)))
					{
						/* Mark as viewable */
						/* *w_ptr |= (CAVE_VIEW); */

#if 0	/* cannot handle other players' lite.. */
						/* Torch-lit grids */
						if (p->d < radius)
						{
							/* Mark as "CAVE_SEEN" and torch-lit */
							info |= (CAVE_SEEN | CAVE_PLIT);
						}

						/* Monster-lit grids */
						else if (info & (CAVE_MLIT))
						{
							/* Mark as "CAVE_SEEN" */
							info |= (CAVE_SEEN);
						}
						else
#endif	/* 0 */

						/* Perma-lit grids */
						if (info & (CAVE_GLOW | CAVE_LITE))
						{
							/* Hack -- move towards player */
							int yy = (y < py) ? (y + 1) : (y > py) ? (y - 1) : y;
							int xx = (x < px) ? (x + 1) : (x > px) ? (x - 1) : x;

#ifdef UPDATE_VIEW_COMPLEX_WALL_ILLUMINATION
							/* seemingly it's not used in ToME too */

							/* Check for "complex" illumination */
							if ((!(cave[yy][xx].info & (CAVE_WALL)) &&
							      (cave[yy][xx].info & (CAVE_GLOW))) ||
							    (!(cave[y][xx].info & (CAVE_WALL)) &&
							      (cave[y][xx].info & (CAVE_GLOW))) ||
							    (!(cave[yy][x].info & (CAVE_WALL)) &&
							      (cave[yy][x].info & (CAVE_GLOW))))
							{
								/* Mark as seen */
								info |= (CAVE_SEEN);
							}

#else /* UPDATE_VIEW_COMPLEX_WALL_ILLUMINATION */

							/* Check for "simple" illumination */
							if (zcave[yy][xx].info & (CAVE_GLOW))
							{
								/* Mark as seen */
								*w_ptr |= (CAVE_XTRA);
							}

#endif /* UPDATE_VIEW_COMPLEX_WALL_ILLUMINATION */

						}

						/* Save cave info */
						/* c_ptr->info = info; */

						/* Save in array */
						cave_view_hack(w_ptr, y, x);
					}
				}

				/* Handle non-wall */
				else
				{
					/* Enqueue child */
					if (last != p->next_0)
					{
						queue[queue_tail++] = last = p->next_0;
					}

					/* Enqueue child */
					if (last != p->next_1)
					{
						queue[queue_tail++] = last = p->next_1;
					}

					/* Newly viewable non-wall */
					if (!(*w_ptr & (CAVE_VIEW)))
					{
						/* Mark as "viewable" */
						/* *w_ptr |= (CAVE_VIEW); */

#if 0

						/* Torch-lit grids */
						if (p->d < radius)
						{
							/* Mark as "CAVE_SEEN" and torch-lit */
							info |= (CAVE_SEEN | CAVE_PLIT);
						}

						/* Perma-lit or monster-lit grids */
						else if (info & (CAVE_GLOW | CAVE_MLIT))
						{
							/* Mark as "CAVE_SEEN" */
							info |= (CAVE_SEEN);
						}
#endif	/* 0 */

						if (info & (CAVE_GLOW | CAVE_LITE))
						{
							/* Mark as "CAVE_SEEN" */
							*w_ptr |= (CAVE_XTRA);
						}

						/* Save cave info */
						/* c_ptr->info = info; */

						/* Save in array */
						cave_view_hack(w_ptr, y, x);
					}
				}
			}
		}
	}



	/*** Step 3 -- Complete the algorithm ***/

#if 0	/* not here (?) */
	/* Handle blindness */
	if (p_ptr->blind)
	{
		/* Process "new" grids */
		for (i = 0; i < fast_view_n; i++)
		{
			/* Location */
			y = view_y[i];
			x = view_x[i];

			/* Grid cannot be "CAVE_SEEN" */
			cave[y][x].info &= ~(CAVE_SEEN);
		}
	}
#endif	/* 0 */

	/* Update all the new grids */
	for (n = 0; n < p_ptr->view_n; n++)
	{
		y = p_ptr->view_y[n];
		x = p_ptr->view_x[n];

		/* Access the grid */
		c_ptr = &zcave[y][x];
		w_ptr = &p_ptr->cave_flag[y][x];

#if 0
		/* Clear the "CAVE_XTRA" flag */
		c_ptr->info &= ~CAVE_XTRA;

		/* Update only newly viewed grids */
		if (c_ptr->info & CAVE_TEMP) continue;
#endif	/* 0 */

		if (!((*w_ptr & CAVE_XTRA) && !(c_ptr->info & CAVE_TEMP)))
			continue;

		/* Note */
		note_spot(Ind, y, x);

		/* Redraw */
		lite_spot(Ind, y, x);
	}

	/* Wipe the old grids, update as needed */
	for (n = 0; n < p_ptr->temp_n; n++)
	{
		y = p_ptr->temp_y[n];
		x = p_ptr->temp_x[n];

		/* Access the grid */
		c_ptr = &zcave[y][x];
		w_ptr = &p_ptr->cave_flag[y][x];

		/* No longer in the array */
		c_ptr->info &= ~CAVE_TEMP;

		/* Update only non-viewable grids */
//		if (*w_ptr & CAVE_VIEW) continue;
		if ((*w_ptr & (CAVE_XTRA))) continue;

		/* Forget it, dude */
		if (unmap)
		{
			u16b this_o_idx, next_o_idx = 0;

			*w_ptr &= ~CAVE_MARK;

			/* make player forget of objects too */
			/* too bad, traps cannot be forgotten this way.. */
			for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
			{
				/* Acquire next object */
				next_o_idx = o_list[this_o_idx].next_o_idx;

				/* Forget the object */
				p_ptr->obj_vis[this_o_idx] = FALSE;
			}
		}

		/* Redraw */
		lite_spot(Ind, y, x);
	}

	/* None left */
	p_ptr->temp_n = 0;
}
#endif	/* USE_OLD_UPDATE_VIEW */






/*
 * Hack -- provide some "speed" for the "flow" code
 * This entry is the "current index" for the "when" field
 * Note that a "when" value of "zero" means "not used".
 *
 * Note that the "cost" indexes from 1 to 127 are for
 * "old" data, and from 128 to 255 are for "new" data.
 *
 * This means that as long as the player does not "teleport",
 * then any monster up to 128 + MONSTER_FLOW_DEPTH will be
 * able to track down the player, and in general, will be
 * able to track down either the player or a position recently
 * occupied by the player.
 */
/*static int flow_n = 0;*/


/*
 * Hack -- forget the "flow" information
 */
void forget_flow(void)
{

#ifdef MONSTER_FLOW

	int x, y;

	/* Nothing to forget */
	if (!flow_n) return;

	/* Check the entire dungeon */
	for (y = 0; y < cur_hgt; y++)
	{
		for (x = 0; x < cur_wid; x++)
		{
			/* Forget the old data */
			cave[y][x].cost = 0;
			cave[y][x].when = 0;
		}
	}

	/* Start over */
	flow_n = 0;

#endif

}


#ifdef MONSTER_FLOW

/*
 * Hack -- Allow us to treat the "seen" array as a queue
 */
static int flow_head = 0;
static int flow_tail = 0;


/*
 * Take note of a reachable grid.  Assume grid is legal.
 */
static void update_flow_aux(int y, int x, int n)
{
	cave_type *c_ptr;

	int old_head = flow_head;


	/* Get the grid */
	c_ptr = &cave[y][x];

	/* Ignore "pre-stamped" entries */
	if (c_ptr->when == flow_n) return;

	/* Ignore "walls" and "rubble" */
	if (c_ptr->feat >= FEAT_RUBBLE) return;

	/* Save the time-stamp */
	c_ptr->when = flow_n;

	/* Save the flow cost */
	c_ptr->cost = n;

	/* Hack -- limit flow depth */
	if (n == MONSTER_FLOW_DEPTH) return;

	/* Enqueue that entry */
	temp_y[flow_head] = y;
	temp_x[flow_head] = x;

	/* Advance the queue */
	if (++flow_head == TEMP_MAX) flow_head = 0;

	/* Hack -- notice overflow by forgetting new entry */
	if (flow_head == flow_tail) flow_head = old_head;
}

#endif


/*
 * Hack -- fill in the "cost" field of every grid that the player
 * can "reach" with the number of steps needed to reach that grid.
 * This also yields the "distance" of the player from every grid.
 *
 * In addition, mark the "when" of the grids that can reach
 * the player with the incremented value of "flow_n".
 *
 * Hack -- use the "seen" array as a "circular queue".
 *
 * We do not need a priority queue because the cost from grid
 * to grid is always "one" and we process them in order.
 */
void update_flow(void)
{

#ifdef MONSTER_FLOW

	int x, y, d;

	/* Hack -- disabled */
	if (!flow_by_sound && !flow_by_smell) return;

	/* Paranoia -- make sure the array is empty */
	if (temp_n) return;

	/* Cycle the old entries (once per 128 updates) */
	if (flow_n == 255)
	{
		/* Rotate the time-stamps */
		for (y = 0; y < cur_hgt; y++)
		{
			for (x = 0; x < cur_wid; x++)
			{
				int w = cave[y][x].when;
				cave[y][x].when = (w > 128) ? (w - 128) : 0;
			}
		}

		/* Restart */
		flow_n = 127;
	}

	/* Start a new flow (never use "zero") */
	flow_n++;


	/* Reset the "queue" */
	flow_head = flow_tail = 0;

	/* Add the player's grid to the queue */
	update_flow_aux(py, px, 0);

	/* Now process the queue */
	while (flow_head != flow_tail)
	{
		/* Extract the next entry */
		y = temp_y[flow_tail];
		x = temp_x[flow_tail];

		/* Forget that entry */
		if (++flow_tail == TEMP_MAX) flow_tail = 0;

		/* Add the "children" */
		for (d = 0; d < 8; d++)
		{
			/* Add that child if "legal" */
			update_flow_aux(y+ddy_ddd[d], x+ddx_ddd[d], cave[y][x].cost+1);
		}
	}

	/* Forget the flow info */
	flow_head = flow_tail = 0;

#endif

}







/*
 * Hack -- map the current panel (plus some) ala "magic mapping"
 */
void map_area(int Ind) {
	player_type *p_ptr = Players[Ind];
	int             i, x, y, y1, y2, x1, x2;

	cave_type       *c_ptr;
	byte            *w_ptr;

/*	dungeon_type	*d_ptr = getdungeon(&p_ptr->wpos); */
	dun_level		*l_ptr = getfloor(&p_ptr->wpos);
	worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;

	/* anti-exploit */
	if (!local_panel(Ind)) return;

	if (!(zcave = getcave(wpos))) return;
/*	if (d_ptr && d_ptr->flags & DUNGEON_NO_MAP) return; */
	if (l_ptr && (l_ptr->flags1 & LF1_NO_MAGIC_MAP)) return;
	if (in_sector00(wpos) && (sector00flags1 & LF1_NO_MAGIC_MAP)) return;

	/* Pick an area to map */
	y1 = TRADPANEL_ROW_MIN - randint(10);
	y2 = TRADPANEL_ROW_MAX + randint(10);
	x1 = TRADPANEL_COL_MIN - randint(20);
	x2 = TRADPANEL_COL_MAX + randint(20);

	/* Speed -- shrink to fit legal bounds */
	if (y1 < 1) y1 = 1;
	if (y2 > p_ptr->cur_hgt-2) y2 = p_ptr->cur_hgt-2;
	if (x1 < 1) x1 = 1;
	if (x2 > p_ptr->cur_wid-2) x2 = p_ptr->cur_wid-2;

	/* Scan that area */
	for (y = y1; y <= y2; y++) {
		for (x = x1; x <= x2; x++) {
			c_ptr = &zcave[y][x];
			w_ptr = &p_ptr->cave_flag[y][x];

			/* All non-walls are "checked" */
//			if (c_ptr->feat < FEAT_SECRET)
			if (!is_wall(c_ptr)) {
				/* Memorize normal features */
//				if (c_ptr->feat > FEAT_INVIS)
				if (!cave_plain_floor_grid(c_ptr)) {
					/* Memorize the object */
					*w_ptr |= CAVE_MARK;
				}

				/* Memorize known walls */
				for (i = 0; i < 8; i++) {
					c_ptr = &zcave[y + ddy_ddd[i]][x + ddx_ddd[i]];
					w_ptr = &p_ptr->cave_flag[y + ddy_ddd[i]][x + ddx_ddd[i]];

					/* Memorize walls (etc) */
//					if (c_ptr->feat >= FEAT_SECRET)
					if (is_wall(c_ptr)) {
						/* Memorize the walls */
						*w_ptr |= CAVE_MARK;
					}
				}
			}
		}
	}

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);
}

/* Mindcrafter's magic mapping spell - C. Blue */
/* This spell's behaviour:
   easy, old: like magic mapping, combined with short-time esp
   good, new: like magic mapping, without any esp */
#define MML_NEW
void mind_map_level(int Ind, int pow) {
	player_type *p_ptr = Players[Ind];
	int m, y, x, rad;
	int i, j, plist[MAX_PLAYERS + 1], plist_size = 1;

	monster_race	*r_ptr;
	monster_type	*m_ptr;
	cave_type       *c_ptr;

	dun_level	*l_ptr = getfloor(&p_ptr->wpos);
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* for mindcrafters too (NR requires) */
	if (l_ptr && l_ptr->flags1 & LF1_NO_MAGIC_MAP) return;
	if (in_sector00(wpos) && (sector00flags1 & LF1_NO_MAGIC_MAP)) return;

	/* build list of players to share the vision with */
	/* oneself too */
	plist[0] = Ind;
	for (i = 1; i <= NumPlayers; i++) {
		/* Skip self */
		if (i == Ind) continue;
		/* Skip disconnected players */
		if (Players[i]->conn == NOT_CONNECTED) continue;
		/* Skip DM if not DM himself */
		if (Players[i]->admin_dm && !p_ptr->admin_dm) continue;
		/* Skip Ghosts */
		if (Players[i]->ghost && !Players[i]->admin_dm) continue;
		/* Skip players not on this depth */
		if (!inarea(&Players[i]->wpos, &p_ptr->wpos)) continue;
		/* Skip players not in the same party */
		if (Players[i]->party == 0 || p_ptr->party != Players[i]->party) continue;
		/* Skip players who haven't opened their mind */
		if (!(Players[i]->esp_link_flags & LINKF_OPEN)) continue;

		/* add him */
		plist[plist_size++] = i;
	}

	/* Is grid in LOS of a monster with a mind? */
	for (m = m_top - 1; m >= 0; m--) {
		/* Access the monster */
		m_ptr = &m_list[m_fast[m]];
		r_ptr = race_inf(m_ptr);

		if (!inarea(&m_ptr->wpos, &p_ptr->wpos)) continue;

		/* sleeping */
//hypnosis yay		if (m_ptr->csleep) continue;

		/* no mind */
		if ((r_ptr->flags9 & RF9_IM_PSI) ||
		    (r_ptr->flags2 & RF2_EMPTY_MIND) ||
		    (r_ptr->flags3 & RF3_NONLIVING))
			continue;

		/* 'clouded' */
		if ((r_ptr->flags9 & RF9_RES_PSI) ||
		    (r_ptr->flags2 & RF2_WEIRD_MIND) ||
		    (r_ptr->flags3 & RF3_UNDEAD))
			rad = (pow + 1) / 2;
		else	rad = pow;

		/* test nearby grids */
		for (y = m_ptr->fy - rad; y < m_ptr->fy + rad; y++)
		for (x = m_ptr->fx - rad; x < m_ptr->fx + rad; x++) {

			if (!in_bounds(y, x)) continue;

			if (distance(y, x, m_ptr->fy, m_ptr->fx) > rad ||
			    distance(y, x, m_ptr->fy, m_ptr->fx) > r_ptr->aaf ||
			    !los(wpos, y, x, m_ptr->fy, m_ptr->fx))
				continue;

			/* Access the grid */
			c_ptr = &zcave[y][x];

			/* Memorize all objects */
			if (c_ptr->o_idx) {
				for (i = 0; i < plist_size; i++) {
					Players[plist[i]]->obj_vis[c_ptr->o_idx]= TRUE;
				}
			}

#if 0 /* like wiz-lite? */
 #if 1 /* should be enabled too if CAVE_MARK below is enabled.. */
			/* perma-lite grid? */
			c_ptr->info |= (CAVE_GLOW);
 #endif
 #if 1
			/* Process all non-walls */
			//if (c_ptr->feat < FEAT_SECRET)
			{
				/* Memorize normal features */
//				if (c_ptr->feat > FEAT_INVIS)
				if (!cave_plain_floor_grid(c_ptr)) {
					for (i = 0; i < plist_size; i++) {
						/* Memorize the grid */
						Players[plist[i]]->cave_flag[y][x] |= CAVE_MARK;
					}
				}

				/* Normally, memorize floors (see above) */
				else if (p_ptr->view_perma_grids && !p_ptr->view_torch_grids) {
					for (i = 0; i < plist_size; i++) {
						/* Memorize the grid */
						Players[plist[i]]->cave_flag[y][x] |= CAVE_MARK;
					}
				}
			}
 #endif
#else /* like magic mapping? */
			/* All non-walls are "checked" */
//			if (c_ptr->feat < FEAT_SECRET)
			if (!is_wall(c_ptr)) {
				/* Memorize normal features */
//				if (c_ptr->feat > FEAT_INVIS) {
				if (!cave_plain_floor_grid(c_ptr)) {
					for (i = 0; i < plist_size; i++) {
						/* Memorize the feature */
						Players[plist[i]]->cave_flag[y][x] |= CAVE_MARK;
 #ifdef MML_NEW
						lite_spot(plist[i], y, x);
 #endif
					}
				}

				/* Memorize known walls */
				for (j = 0; j < 8; j++) {
					if (!in_bounds(y + ddy_ddd[j], x + ddx_ddd[j])) continue;

					/* Memorize walls (etc) */
//					if (c_ptr->feat >= FEAT_SECRET)
					if (is_wall(&zcave[y + ddy_ddd[j]][x + ddx_ddd[j]])) {
						for (i = 0; i < plist_size; i++) {
							/* Memorize the walls */
							Players[plist[i]]->cave_flag[y + ddy_ddd[j]][x + ddx_ddd[j]] |= CAVE_MARK;
 #ifdef MML_NEW
							lite_spot(plist[i], y + ddy_ddd[j], x + ddx_ddd[j]);
 #endif
						}
					}
				}
			}
#endif
		}

#if 0 /* this will be overheady, since it requires prt_map() here as a bad \
         hack, and PR_MAP/PU_MONSTERS commented out in for-loop below. \
         See same for-loop for clean solution as good alternative! */
		prt_map(plist[i], FALSE); /* bad hack */
		/* like detect_creatures(), not excluding invisible monsters though */
		for (i = 0; i < plist_size; i++) {
			if (Players[plist[i]]->mon_vis[m_fast[m]]) continue;
			Players[plist[i]]->mon_vis[m_fast[m]] = TRUE;
			lite_spot(plist[i], m_ptr->fy, m_ptr->fx);
			Players[plist[i]]->mon_vis[m_fast[m]] = FALSE; /* the usual hack: don't update screen after this */
		}
#elif defined(MML_NEW) /* new attempt */
		/* like detect_creatures(), not excluding invisible monsters though */
		for (i = 0; i < plist_size; i++) {
			if (Players[plist[i]]->mon_vis[m_fast[m]]) continue;
			Players[plist[i]]->mon_vis[m_fast[m]] = TRUE;
			lite_spot(plist[i], m_ptr->fy, m_ptr->fx);
			Players[plist[i]]->mon_vis[m_fast[m]] = FALSE; /* the usual hack: don't update screen after this */
		}
#endif
	}

	/* Specialty: Detect all dungeon stores! */
	//TODO: Store the 1 or maybe 2 stores in the l_ptr array instead -_-' */
	for (y = 0; y < MAX_HGT; y++)
		for (x = 0; x < MAX_WID; x++) {
			if (zcave[y][x].feat != FEAT_SHOP) continue;
			for (i = 0; i < plist_size; i++) {
				Players[plist[i]]->cave_flag[y][x] |= CAVE_MARK;
				lite_spot(plist[i], y, x);
			}
		}

#ifndef MML_NEW
	for (i = 0; i < plist_size; i++) {
 #if 1
		/* clean solution instead: give some temporary ESP (makes sense also).
		 Note however: this is thereby becoming an auto-projectable ESP spell^^ */
		set_tim_esp(plist[i], 10);
 #endif

		/* Update the monsters */
		Players[plist[i]]->update |= (PU_MONSTERS);
		/* Redraw map */
		Players[plist[i]]->redraw |= (PR_MAP);
		/* Window stuff */
		Players[plist[i]]->window |= (PW_OVERHEAD);
	}
#endif
}

/*
 * Light up the dungeon using "claravoyance"
 *
 * This function "illuminates" every grid in the dungeon, memorizes all
 * "objects", memorizes all grids as with magic mapping, and, under the
 * standard option settings (view_perma_grids but not view_torch_grids)
 * memorizes all floor grids too.
 *
 * Note that if "view_perma_grids" is not set, we do not memorize floor
 * grids, since this would defeat the purpose of "view_perma_grids", not
 * that anyone seems to play without this option.
 *
 * Note that if "view_torch_grids" is set, we do not memorize floor grids,
 * since this would prevent the use of "view_torch_grids" as a method to
 * keep track of what grids have been observed directly.
 */
void wiz_lite(int Ind) {
	player_type *p_ptr = Players[Ind];
	int             y, x, i;

	cave_type       *c_ptr;
	byte            *w_ptr;

/*	dungeon_type	*d_ptr = getdungeon(&p_ptr->wpos); */
	dun_level *l_ptr = getfloor(&p_ptr->wpos);
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;

	/* don't ruin the mood ^^ */
	bool mood = (wpos->wz == 0 && (season_halloween || season_newyearseve));

	if (!(zcave = getcave(wpos))) return;

/*	if (d_ptr && d_ptr->flags & DUNGEON_NO_MAP) return; */
	if (l_ptr && l_ptr->flags1 & LF1_NO_MAGIC_MAP) return;
	if (in_sector00(wpos) && (sector00flags1 & LF1_NO_MAGIC_MAP)) return;

	/* Scan all normal grids */
	for (y = 1; y < p_ptr->cur_hgt-1; y++) {
		/* Scan all normal grids */
		for (x = 1; x < p_ptr->cur_wid-1; x++) {
			/* Access the grid */
			c_ptr = &zcave[y][x];
			if (mood && !(c_ptr->info & CAVE_ICKY)) continue;
			w_ptr = &p_ptr->cave_flag[y][x];

			/* Memorize all objects */
			if (c_ptr->o_idx) {
				/* Memorize */
				p_ptr->obj_vis[c_ptr->o_idx]= TRUE;
			}

			/* Process all non-walls */
			//if (c_ptr->feat < FEAT_SECRET) <- deprecated; use next line if you want "clean" wizlite ;) - C. Blue
//			if (!(f_info[c_ptr->feat].flags1 & FF1_WALL))
			{
				/* Scan all neighbors */
				for (i = 0; i < 9; i++) {
					int yy = y + ddy_ddd[i];
					int xx = x + ddx_ddd[i];

					/* Get the grid */
					c_ptr = &zcave[yy][xx];
					if (mood && !(c_ptr->info & CAVE_ICKY)) continue; //if this were commented out, house walls would be *bright*
					w_ptr = &p_ptr->cave_flag[yy][xx];

					/* Perma-lite the grid */
					c_ptr->info |= (CAVE_GLOW);

					/* Memorize normal features */
//					if (c_ptr->feat > FEAT_INVIS)
					if (!cave_plain_floor_grid(c_ptr)) {
						/* Memorize the grid */
						*w_ptr |= CAVE_MARK;
					}

					/* Normally, memorize floors (see above) */
					if (p_ptr->view_perma_grids && !p_ptr->view_torch_grids) {
						/* Memorize the grid */
						*w_ptr |= CAVE_MARK;
					}
				}
			}
		}
	}

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);

}


/* from PernA	- Jir - */
void wiz_lite_extra(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int y, x;
	struct worldpos *wpos = &p_ptr->wpos;
//	dun_level *l_ptr = getfloor(wpos);
	cave_type **zcave;
	cave_type *c_ptr;

	/* don't ruin the mood ^^ */
	bool mood = (wpos->wz == 0 && (season_halloween || season_newyearseve));

	if (!(zcave = getcave(wpos))) return;

/*	if (d_ptr && d_ptr->flags & DUNGEON_NO_MAP) return; */

	for (y = 0; y < p_ptr->cur_hgt; y++) {
		for (x = 0; x < p_ptr->cur_wid; x++) {
			c_ptr = &zcave[y][x];
			if (mood && !(c_ptr->info & CAVE_ICKY)) continue;
			/* ligten up all grids and remember features */
//			c_ptr->info |= (CAVE_GLOW | CAVE_MARK);
			/* lighten up all grids */
			c_ptr->info |= CAVE_GLOW;
		}
	}


	/* remember features too? */
//	if (!(l_ptr && l_ptr->flags1 & LF1_NO_MAGIC_MAP))
	wiz_lite(Ind);


	for (x = 1; x <= NumPlayers; x++) {
		p_ptr = Players[x];

		/* Only works for players on the level */
		if (!inarea(wpos, &p_ptr->wpos)) continue;

		/* Update some things */
		p_ptr->update |= (PU_VIEW | PU_DISTANCE);
		p_ptr->redraw |= PR_MAP;
		p_ptr->window |= PW_OVERHEAD;
	}

}

/*
 * Forget the dungeon map (ala "Thinking of Maud...").
 */
void wiz_dark(int Ind) {
	player_type *p_ptr, *q_ptr = Players[Ind];
	struct worldpos *wpos;
	cave_type **zcave;
	cave_type *c_ptr;
	int y, x, i;
	object_type *o_ptr;

	wpos = &q_ptr->wpos;
	if (!(zcave = getcave(wpos))) return;

	/* Check for every other player */

	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];

		/* Only works for players on the level */
		if (!inarea(wpos, &p_ptr->wpos)) continue;

		o_ptr = &p_ptr->inventory[INVEN_LITE];

		/* Bye bye light */
		if (o_ptr->k_idx &&
		    ((o_ptr->sval == SV_LITE_TORCH) || (o_ptr->sval == SV_LITE_LANTERN))
		    && (!o_ptr->name1) && o_ptr->timeout) {
			bool refilled = FALSE;

			disturb(Ind, 0, 0);

			/* If flint is ready, refill at once */
			if (TOOL_EQUIPPED(p_ptr) == SV_TOOL_FLINT &&
			    !p_ptr->paralyzed) {
				msg_print(Ind, "You prepare a flint...");
				refilled = do_auto_refill(Ind);
				if (!refilled) msg_print(Ind, "Oops, you're out of fuel!");
			}

			if (!refilled) {
				msg_print(i, "Your light suddenly empties.");

				/* No more light, it's Rogues day today :) */
				o_ptr->timeout = 0;
			}
		}

		/* Forget every grid */
		for (y = 0; y < p_ptr->cur_hgt; y++) {
			for (x = 0; x < p_ptr->cur_wid; x++) {
				byte *w_ptr = &p_ptr->cave_flag[y][x];
				c_ptr = &zcave[y][x];

				/* Process the grid */
				*w_ptr &= ~CAVE_MARK;
				c_ptr->info &= ~CAVE_GLOW;

				/* Forget every object */
				if (c_ptr->o_idx)
				{
					/* Forget the object */
					p_ptr->obj_vis[c_ptr->o_idx] = FALSE;
				}
			}
		}

		/* Mega-Hack -- Forget the view and lite */
		p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);

		/* Update the view and lite */
		p_ptr->update |= (PU_VIEW | PU_LITE);

		/* Update the monsters */
		p_ptr->update |= (PU_MONSTERS);

		/* Redraw map */
		p_ptr->redraw |= (PR_MAP);

		/* Window stuff */
		p_ptr->window |= (PW_OVERHEAD);
	}
}

#ifdef ARCADE_SERVER
extern int check_feat(worldpos *wpos, int y, int x)
{
        cave_type **zcave;
        cave_type *c_ptr;

        if (!(zcave = getcave(wpos))) return(0);

        if (!in_bounds(y, x)) return(0);

        c_ptr = &zcave[y][x];
	return(c_ptr->feat);
}
#endif

/*
 * Change the "feat" flag for a grid, and notice/redraw the grid
 * (Adapted from PernAngband)
 */
bool level_generation_time = FALSE;
void cave_set_feat(worldpos *wpos, int y, int x, int feat) {
	player_type *p_ptr;
	cave_type **zcave;
	cave_type *c_ptr;
	//struct c_special *cs_ptr;
	int i;

	/* for Submerged Ruins: ensure all deep water; also affects Small Water Cave. */
	dun_level *l_ptr = getfloor(wpos);
	bool deep_water = l_ptr && (l_ptr->flags1 & LF1_DEEP_WATER);

	if (!(zcave = getcave(wpos))) return;
	if (!in_bounds(y, x)) return;
	c_ptr = &zcave[y][x];

	/* Trees in greater fire become dead trees at once */
	if ((feat == FEAT_TREE || feat == FEAT_BUSH) &&
	    (c_ptr->feat == FEAT_SHAL_LAVA ||
	    c_ptr->feat == FEAT_FIRE ||
	    c_ptr->feat == FEAT_GREAT_FIRE))
		feat = FEAT_DEAD_TREE;

	/* Don't mess with inns please! */
	if (f_info[c_ptr->feat].flags1 & FF1_PROTECTED) return;

	/* in Nether Realm, floor is always nether mist (or lava)! */
	if (in_netherrealm(wpos)) switch (feat) {
		case FEAT_IVY:
		case FEAT_SHAL_WATER:
		case FEAT_DEEP_WATER:
		case FEAT_SNOW:
		case FEAT_ICE:
		case FEAT_FLOOR:
		case FEAT_DIRT:
		case FEAT_GRASS:
		case FEAT_SAND:
		case FEAT_ASH:
		case FEAT_MUD:
	/*	case FEAT_PUDDLE: new feature to be added: same as shallow water, but dries out after a while */
		case FEAT_FLOWER: feat = FEAT_NETHER_MIST;
	}
	/* in SR/SWC floor is always deep water */
	if (deep_water) switch (feat) {
		case FEAT_IVY:
		case FEAT_SHAL_WATER:
		case FEAT_SNOW:
		case FEAT_ICE:
		case FEAT_FLOOR:
		case FEAT_DIRT:
		case FEAT_GRASS:
		case FEAT_SAND:
		case FEAT_ASH:
		case FEAT_MUD:
	/*	case FEAT_PUDDLE: new feature to be added: same as shallow water, but dries out after a while */
		case FEAT_FLOWER: feat = FEAT_DEEP_WATER;
	}

	/* Change the feature */
	c_ptr->feat = feat;
	aquatic_terrain_hack(zcave, x, y);

	if (level_generation_time) return;

	/* XXX it's not needed when called from generate.c */
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];

		/* Only works for players on the level */
		if (!inarea(wpos, &p_ptr->wpos)) continue;

		/* Notice */
		note_spot(i, y, x);

		/* Redraw */
		lite_spot(i, y, x);
	}
}

/* Helper function - like cave_set_feat_live() but doesn't set anything, just tests.
   Added for monster traps: Those mustn't be set on DEEP_WATER/DEEP_LAVA since cave_set_feat_live()
   would prevent the feature from getting placed, resulting in bugginess and (nothing)s. */
bool cave_set_feat_live_ok(worldpos *wpos, int y, int x, int feat) {
	cave_type **zcave;
	cave_type *c_ptr;
	int i;
	//struct town_type *t_ptr; /* have town keep track of number of feature changes (not yet implemented) */

	if (!(zcave = getcave(wpos))) return FALSE;
	if (!in_bounds(y, x)) return FALSE;
	c_ptr = &zcave[y][x];

	/* apply town-specific restrictions, preserving the intended town layout */
	if (istown(wpos)) {
#if 0
		t_ptr = &town[wild_info[wpos->wx][wpos->wy].town_idx];

		switch (feat) {
		case FEAT_TREE:
		case FEAT_BUSH:
			if (TOWN_TERRAFORM_TREES == 0) return FALSE;
			break;
		case FEAT_WALL_EXTRA:
		case FEAT_QUARTZ:
		case FEAT_MAGMA:
			if (TOWN_TERRAFORM_WALLS == 0) return FALSE;
			break;
		case FEAT_SHAL_WATER:
		case FEAT_DEEP_WATER:
			if (TOWN_TERRAFORM_WATER == 0) return FALSE;
			break;
		case FEAT_GLYPH:
		case FEAT_RUNE:
			if (TOWN_TERRAFORM_GLYPHS == 0) return FALSE;
			break;
		}

		switch (c_ptr->feat) {
		case FEAT_TREE:
		case FEAT_BUSH:
			if (TOWN_TERRAFORM_TREES == 0) return FALSE;
			break;
		case FEAT_WALL_EXTRA:
		case FEAT_QUARTZ:
		case FEAT_MAGMA:
			if (TOWN_TERRAFORM_WALLS == 0) return FALSE;
			break;
		case FEAT_SHAL_WATER:
		case FEAT_DEEP_WATER:
			if (TOWN_TERRAFORM_WATER == 0) return FALSE;
			break;
		case FEAT_GLYPH:
		case FEAT_RUNE:
			if (TOWN_TERRAFORM_GLYPHS == 0) return FALSE;
			break;
		}
#else
		/* hack: only allow around store entrances */
		if (feat == FEAT_GLYPH || feat == FEAT_RUNE) {
			return FALSE; //disallow glyphs

			for (i = 0; i < 9; i++)
				if (zcave[y + ddy_ddd[i]][x + ddx_ddd[i]].feat == FEAT_SHOP) break;
			/* no nearby store entrance found? */
			if (i == 9) return FALSE;
		}
#endif
	}

	/* No runes of protection / glyphs of warding on non-empty grids - C. Blue */
	if ((feat == FEAT_GLYPH || feat == FEAT_RUNE || feat == FEAT_MON_TRAP) && !(cave_clean_bold(zcave, y, x) && /* cave_clean_bold also checks for object absence */
	    ((c_ptr->feat == FEAT_NONE) ||
	    (c_ptr->feat == FEAT_FLOOR) ||
	    (c_ptr->feat == FEAT_DIRT) ||
	    (c_ptr->feat == FEAT_LOOSE_DIRT) || /* used for gardens (fields) in wild.c */
	    (c_ptr->feat == FEAT_CROP) || /* used for gardens (fields) in wild.c */
	    (c_ptr->feat == FEAT_GRASS) ||
	    (c_ptr->feat == FEAT_SNOW) ||
	    (c_ptr->feat == FEAT_ICE) ||
	    (c_ptr->feat == FEAT_SAND) ||
	    (c_ptr->feat == FEAT_ASH) ||
	    (c_ptr->feat == FEAT_MUD) ||
	    (c_ptr->feat == FEAT_FLOWER) ||
	    (c_ptr->feat == FEAT_NETHER_MIST))))
		return FALSE;

	/* Don't mess with inns please! */
	if (f_info[c_ptr->feat].flags1 & FF1_PROTECTED) return FALSE;

	/* No terraforming on impossible ground -
	   compare twall_erosion() for consistency */
#if 0
	if ((feat == FEAT_TREE || feat == FEAT_BUSH || feat == FEAT_FLOOR ||
	    feat == FEAT_WALL_EXTRA || feat == FEAT_QUARTZ || feat == FEAT_MAGMA) &&
	    feat_mud, feat_dirt, etc.... just #else maybe easier?..
	    (c_ptr->feat == FEAT_DEEP_LAVA ||
	    c_ptr->feat == FEAT_DEEP_WATER))
#else
	if (c_ptr->feat == FEAT_DEEP_LAVA || c_ptr->feat == FEAT_DEEP_WATER)
#endif
		return FALSE;

	return TRUE;
}

/*
 * This is a copy of cave_set_feat that is used for "live" changes to the world
 * by players and monsters. More specific restrictions can be placed here.
 * NOTE: We assume, that allow_terraforming() has already been checked before
 *       cave_set_feat_live() is actually called.
 */
bool cave_set_feat_live(worldpos *wpos, int y, int x, int feat) {
	player_type *p_ptr;
	cave_type **zcave;
	cave_type *c_ptr;
	struct c_special *cs_ptr;
	int i;
	//struct town_type *t_ptr; /* have town keep track of number of feature changes (not yet implemented) */

	/* for Submerged Ruins: ensure all deep water; also affects Small Water Cave. */
	dun_level *l_ptr = getfloor(wpos);
	bool deep_water = l_ptr && (l_ptr->flags1 & LF1_DEEP_WATER);

	if (!(zcave = getcave(wpos))) return FALSE;
	if (!in_bounds(y, x)) return FALSE;
	c_ptr = &zcave[y][x];

	/* apply town-specific restrictions, preserving the intended town layout */
	if (istown(wpos)) {
#if 0
		t_ptr = &town[wild_info[wpos->wx][wpos->wy].town_idx];

		switch (feat) {
		case FEAT_TREE:
		case FEAT_BUSH:
			if (TOWN_TERRAFORM_TREES == 0) return FALSE;
			break;
		case FEAT_WALL_EXTRA:
		case FEAT_QUARTZ:
		case FEAT_MAGMA:
			if (TOWN_TERRAFORM_WALLS == 0) return FALSE;
			break;
		case FEAT_SHAL_WATER:
		case FEAT_DEEP_WATER:
			if (TOWN_TERRAFORM_WATER == 0) return FALSE;
			break;
		case FEAT_GLYPH:
		case FEAT_RUNE:
			if (TOWN_TERRAFORM_GLYPHS == 0) return FALSE;
			break;
		}

		switch (c_ptr->feat) {
		case FEAT_TREE:
		case FEAT_BUSH:
			if (TOWN_TERRAFORM_TREES == 0) return FALSE;
			break;
		case FEAT_WALL_EXTRA:
		case FEAT_QUARTZ:
		case FEAT_MAGMA:
			if (TOWN_TERRAFORM_WALLS == 0) return FALSE;
			break;
		case FEAT_SHAL_WATER:
		case FEAT_DEEP_WATER:
			if (TOWN_TERRAFORM_WATER == 0) return FALSE;
			break;
		case FEAT_GLYPH:
		case FEAT_RUNE:
			if (TOWN_TERRAFORM_GLYPHS == 0) return FALSE;
			break;
		}
#else
		/* hack: only allow around store entrances */
		if (feat == FEAT_GLYPH || feat == FEAT_RUNE) {
			return FALSE; //disallow glyphs

			for (i = 0; i < 9; i++)
				if (zcave[y + ddy_ddd[i]][x + ddx_ddd[i]].feat == FEAT_SHOP) break;
			/* no nearby store entrance found? */
			if (i == 9) return FALSE;
		}
#endif
	}

	/* No runes of protection / glyphs of warding on non-empty grids - C. Blue */
	if ((feat == FEAT_GLYPH || feat == FEAT_RUNE || feat == FEAT_MON_TRAP) && !(cave_clean_bold(zcave, y, x) && /* cave_clean_bold also checks for object absence */
	    ((c_ptr->feat == FEAT_NONE) ||
	    (c_ptr->feat == FEAT_FLOOR) ||
	    (c_ptr->feat == FEAT_DIRT) ||
	    (c_ptr->feat == FEAT_LOOSE_DIRT) || /* used for gardens (fields) in wild.c */
	    (c_ptr->feat == FEAT_CROP) || /* used for gardens (fields) in wild.c */
	    (c_ptr->feat == FEAT_GRASS) ||
	    (c_ptr->feat == FEAT_SNOW) ||
	    (c_ptr->feat == FEAT_ICE) ||
	    (c_ptr->feat == FEAT_SAND) ||
	    (c_ptr->feat == FEAT_ASH) ||
	    (c_ptr->feat == FEAT_MUD) ||
	    (c_ptr->feat == FEAT_FLOWER) ||
	    (c_ptr->feat == FEAT_NETHER_MIST))))
		return FALSE;

	/* Don't mess with inns please! */
	if (f_info[c_ptr->feat].flags1 & FF1_PROTECTED) return FALSE;

	/* No terraforming on impossible ground -
	   compare twall_erosion() for consistency */
#if 0
	if ((feat == FEAT_TREE || feat == FEAT_BUSH || feat == FEAT_FLOOR ||
	    feat == FEAT_WALL_EXTRA || feat == FEAT_QUARTZ || feat == FEAT_MAGMA) &&
	    feat_mud, feat_dirt, etc.... just #else maybe easier?..
	    (c_ptr->feat == FEAT_DEEP_LAVA ||
	    c_ptr->feat == FEAT_DEEP_WATER))
#else
	if (c_ptr->feat == FEAT_DEEP_LAVA || c_ptr->feat == FEAT_DEEP_WATER)
#endif
		return FALSE;

	/* in Nether Realm, floor is always nether mist (or lava)! */
	if (in_netherrealm(wpos)) switch (feat) {
		case FEAT_IVY:
		case FEAT_SHAL_WATER:
		case FEAT_DEEP_WATER:
		case FEAT_SNOW:
		case FEAT_ICE:
		case FEAT_FLOOR:
		case FEAT_DIRT:
		case FEAT_GRASS:
		case FEAT_SAND:
		case FEAT_ASH:
		case FEAT_MUD:
	/*	case FEAT_PUDDLE: new feature to be added: same as shallow water, but dries out after a while */
		case FEAT_FLOWER: feat = FEAT_NETHER_MIST;
	}
	/* in SR/SWC floor is always deep water */
	if (deep_water) switch (feat) {
		case FEAT_IVY:
		case FEAT_SHAL_WATER:
		case FEAT_SNOW:
		case FEAT_ICE:
		case FEAT_FLOOR:
		case FEAT_DIRT:
		case FEAT_GRASS:
		case FEAT_SAND:
		case FEAT_ASH:
		case FEAT_MUD:
	/*	case FEAT_PUDDLE: new feature to be added: same as shallow water, but dries out after a while */
		case FEAT_FLOWER: feat = FEAT_DEEP_WATER;
	}


	/* Trees in greater fire become dead trees at once */
	if ((feat == FEAT_TREE || feat == FEAT_BUSH) &&
	    (c_ptr->feat == FEAT_SHAL_LAVA ||
	    c_ptr->feat == FEAT_FIRE ||
	    c_ptr->feat == FEAT_GREAT_FIRE))
		feat = FEAT_DEAD_TREE;

	/* Clear mimic feature left by a secret door - mikaelh */
	if ((cs_ptr = GetCS(c_ptr, CS_MIMIC)))
		cs_erase(c_ptr, cs_ptr);

	/* Change the feature */
	if (c_ptr->feat != feat) c_ptr->info &= ~CAVE_NEST_PIT; /* clear teleport protection for nest grid if it gets changed */
	c_ptr->feat = feat;

	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];

		/* Only works for players on the level */
		if (!inarea(wpos, &p_ptr->wpos)) continue;

#if 0 /* done in melee2.c when a monster eats a wall, so it's visible from afar, if level is mapped, that "walls turn black" */
		/* Forget the spot */
		Players[i]->cave_flag[y][x] &= ~CAVE_MARK;
#endif

		/* Notice */
		note_spot(i, y, x);

		/* Redraw */
		lite_spot(i, y, x);

		/* Update some things */
		p_ptr->update |= (PU_VIEW | PU_DISTANCE);
		//p_ptr->update |= PU_FLOW;

		//p_ptr->redraw |= PR_MAP;
		//p_ptr->window |= PW_OVERHEAD;
	}
	return TRUE;
}



/*
 * Calculate "incremental motion". Used by project() and shoot().
 * Assumes that (*y,*x) lies on the path from (y1,x1) to (y2,x2).
 */
void mmove2(int *y, int *x, int y1, int x1, int y2, int x2)
{
	int dy, dx, dist, shift;

	/* Extract the distance travelled */
	dy = (*y < y1) ? y1 - *y : *y - y1;
	dx = (*x < x1) ? x1 - *x : *x - x1;

	/* Number of steps */
	dist = (dy > dx) ? dy : dx;

	/* We are calculating the next location */
	dist++;


	/* Calculate the total distance along each axis */
	dy = (y2 < y1) ? (y1 - y2) : (y2 - y1);
	dx = (x2 < x1) ? (x1 - x2) : (x2 - x1);

	/* Paranoia -- Hack -- no motion */
	if (!dy && !dx) return;


	/* Move mostly vertically */
	if (dy > dx)
	{

#if 0

		int k;

		/* Starting shift factor */
		shift = dy >> 1;

		/* Extract a shift factor */
		for (k = 0; k < dist; k++)
		{
			if (shift <= 0) shift += dy;
			shift -= dx;
		}

		/* Sometimes move along minor axis */
		if (shift <= 0) (*x) = (x2 < x1) ? (*x - 1) : (*x + 1);

		/* Always move along major axis */
		(*y) = (y2 < y1) ? (*y - 1) : (*y + 1);

#endif

		/* Extract a shift factor */
		shift = (dist * dx + (dy-1) / 2) / dy;

		/* Sometimes move along the minor axis */
		(*x) = (x2 < x1) ? (x1 - shift) : (x1 + shift);

		/* Always move along major axis */
		(*y) = (y2 < y1) ? (y1 - dist) : (y1 + dist);
	}

	/* Move mostly horizontally */
	else
	{

#if 0

		int k;

		/* Starting shift factor */
		shift = dx >> 1;

		/* Extract a shift factor */
		for (k = 0; k < dist; k++)
		{
			if (shift <= 0) shift += dx;
			shift -= dy;
		}

		/* Sometimes move along minor axis */
		if (shift <= 0) (*y) = (y2 < y1) ? (*y - 1) : (*y + 1);

		/* Always move along major axis */
		(*x) = (x2 < x1) ? (*x - 1) : (*x + 1);

#endif

		/* Extract a shift factor */
		shift = (dist * dy + (dx-1) / 2) / dx;

		/* Sometimes move along the minor axis */
		(*y) = (y2 < y1) ? (y1 - shift) : (y1 + shift);

		/* Always move along major axis */
		(*x) = (x2 < x1) ? (x1 - dist) : (x1 + dist);
	}
}


/*
 * Determine if a bolt spell cast from (y1,x1) to (y2,x2) will arrive
 * at the final destination, assuming no monster gets in the way.
 *
 * This is slightly (but significantly) different from "los(y1,x1,y2,x2)".
 */
/* Potential high-profile BUG: bool los() 'Travel horiz/vert..' probably differs from
   bool clean_shot() 'mmove()' so that monsters can fire at you without you seeing them.
   Fixing it via bad hack: just adding an extra los() check here for now. */
#define CRUDE_TARGETTING_LOS_FIX

#ifdef DOUBLE_LOS_SAFETY
static bool projectable_DLS(struct worldpos *wpos, int y1, int x1, int y2, int x2, int range);
bool projectable(struct worldpos *wpos, int y1, int x1, int y2, int x2, int range) {
	return (projectable_DLS(wpos, y1, x1, y2, x2, range) || projectable_DLS(wpos, y2, x2, y1, x1, range));
}
static bool projectable_DLS(struct worldpos *wpos, int y1, int x1, int y2, int x2, int range) {
#else
bool projectable(struct worldpos *wpos, int y1, int x1, int y2, int x2, int range) {
#endif
	int dist, y, x;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return FALSE;

#ifdef DOUBLE_LOS_SAFETY
	/* catch ball-spell related call in project() which adds '+ dir * 99' aka distance of 99 */
	if (!in_bounds(y1, x1)) return(FALSE);
#endif

	/* Start at the initial location */
	y = y1, x = x1;

	/* See "project()" */
	for (dist = 0; dist <= range; dist++) {
		/* Never pass through walls */
		if (dist && !cave_los(zcave, y, x)) break;

		/* Stopped by protected grids (Inn doors, also stopping monsters' ranged attacks!) */
                if (f_info[zcave[y][x].feat].flags1 & (FF1_BLOCK_LOS | FF1_BLOCK_CONTACT)) break;

		/* Check for arrival at "final target" */
		if ((x == x2) && (y == y2)) {
#ifdef CRUDE_TARGETTING_LOS_FIX
			/* (bugfix via bad hack) ensure monsters cant snipe us from out of LoS */
			if (!los(wpos, y1, x1, y2, x2)) return FALSE;
#endif
			return (TRUE);
		}

		/* Calculate the new location */
		mmove2(&y, &x, y1, x1, y2, x2);
	}

	/* Assume obstruction */
	return (FALSE);
}

/* Same as projectable(), but allows targetting the first grid in a wall */
#ifdef DOUBLE_LOS_SAFETY
static bool projectable_wall_DLS(struct worldpos *wpos, int y1, int x1, int y2, int x2, int range);
bool projectable_wall(struct worldpos *wpos, int y1, int x1, int y2, int x2, int range) {
	return (projectable_wall_DLS(wpos, y1, x1, y2, x2, range) || projectable_wall_DLS(wpos, y2, x2, y1, x1, range));
}
static bool projectable_wall_DLS(struct worldpos *wpos, int y1, int x1, int y2, int x2, int range) {
#else
bool projectable_wall(struct worldpos *wpos, int y1, int x1, int y2, int x2, int range) {
#endif
	int dist, y, x;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return(FALSE);

#ifdef DOUBLE_LOS_SAFETY
	/* catch ball-spell related call in project() which adds '+ dir * 99' aka distance of 99 */
	if (!in_bounds(y1, x1)) return(FALSE);
#endif

	/* Start at the initial location */
	y = y1, x = x1;

	/* See "project()" */
	for (dist = 0; dist <= range; dist++) {
		/* Protected grids prevent targetting */
		if (f_info[zcave[y][x].feat].flags1 & (FF1_BLOCK_LOS | FF1_BLOCK_CONTACT)) break;

		/* Check for arrival at "final target" */
		if ((x == x2) && (y == y2)) {
#ifdef CRUDE_TARGETTING_LOS_FIX
			/* (bugfix via bad hack) ensure monsters cant snipe us from out of LoS */
			if (!los(wpos, y1, x1, y2, x2)) return FALSE;
#endif
			return (TRUE);
		}

		/* Never go through walls */
		if (dist && !cave_los(zcave, y, x)) break;

		/* Calculate the new location */
		mmove2(&y, &x, y1, x1, y2, x2);
	}

	/* Assume obstruction */
	return (FALSE);
}

/* like projectable_wall(), but assumes that only _permanent walls_ are really obstacles to us
   (for movement strategies of PASS_WALL/KILL_WALL monsters) - C. Blue */
#ifdef DOUBLE_LOS_SAFETY
static bool projectable_wall_perm_DLS(struct worldpos *wpos, int y1, int x1, int y2, int x2, int range);
bool projectable_wall_perm(struct worldpos *wpos, int y1, int x1, int y2, int x2, int range) {
	return (projectable_wall_perm_DLS(wpos, y1, x1, y2, x2, range) || projectable_wall_perm_DLS(wpos, y2, x2, y1, x1, range));
}
static bool projectable_wall_perm_DLS(struct worldpos *wpos, int y1, int x1, int y2, int x2, int range) {
#else
bool projectable_wall_perm(struct worldpos *wpos, int y1, int x1, int y2, int x2, int range) {
#endif
	int dist, y, x;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return(FALSE);

	/* Start at the initial location */
	y = y1, x = x1;

	/* See "project()" */
	for (dist = 0; dist <= range; dist++) {
		/* Protected grids prevent targetting */
		if (f_info[zcave[y][x].feat].flags1 & FF1_PERMANENT) break;

		/* Check for arrival at "final target" */
		if ((x == x2) && (y == y2)) return (TRUE);

		/* Calculate the new location */
		mmove2(&y, &x, y1, x1, y2, x2);
	}

	/* Assume obstruction */
	return (FALSE);
}

/*
 * Created for shoot_till_kill mode in do_cmd_fire().
 * Determine if a bolt spell cast from (y1,x1) to (y2,x2) will arrive
 * at the final destination, and if no monster/hostile player is in the way. - C. Blue
 * Note: Only sleeping monsters are tested for!
 * Note: The hostile-player part is commented out for efficiency atm.
 *
 */
#ifdef DOUBLE_LOS_SAFETY
static bool projectable_real_DLS(int Ind, int y1, int x1, int y2, int x2, int range);
bool projectable_real(int Ind, int y1, int x1, int y2, int x2, int range) {
	return ((projectable_DLS(&Players[Ind]->wpos, y1, x1, y2, x2, range) || projectable_DLS(&Players[Ind]->wpos, y2, x2, y1, x1, range))
	    /* important: make sure we really don't hit an awake monster from our direction of shooting :-p  : */
	    && projectable_real_DLS(Ind, y1, x1, y2, x2, range));
}
static bool projectable_real_DLS(int Ind, int y1, int x1, int y2, int x2, int range) {
	int dist, y = y1, x = x1;
	cave_type **zcave;
	if (!(zcave = getcave(&Players[Ind]->wpos))) return FALSE;

	for (dist = 0; dist <= range; dist++) {
		if ((x == x2) && (y == y2)) return (TRUE);

		/* Never pass through SLEEPING monsters */
                if (dist && (zcave[y][x].m_idx > 0)
		    && target_able(Ind, zcave[y][x].m_idx)
		    && m_list[zcave[y][x].m_idx].csleep) break;

#if 0
		/* Never pass through hostile player */
                if (dist && zcave[y][x].m_idx < 0) {
			if (check_hostile(Ind, -zcave[y][x].m_idx)) break;
		}
#endif
		mmove2(&y, &x, y1, x1, y2, x2);
	}
	return (FALSE);
}
#else
bool projectable_real(int Ind, int y1, int x1, int y2, int x2, int range) {
	int dist, y, x;
	cave_type **zcave;
	if (!(zcave = getcave(&Players[Ind]->wpos))) return FALSE;

	/* Start at the initial location */
	y = y1, x = x1;

	/* See "project()" */
	for (dist = 0; dist <= range; dist++) {
		/* Never pass through walls */
		if (dist && !cave_los(zcave, y, x)) break;

		/* Stopped by protected grids (Inn doors, also stopping monsters' ranged attacks!) */
                if (f_info[zcave[y][x].feat].flags1 & (FF1_BLOCK_LOS | FF1_BLOCK_CONTACT)) break;

		/* Check for arrival at "final target" */
		if ((x == x2) && (y == y2)) return (TRUE);

		/* Never pass through SLEEPING monsters */
                if (dist && (zcave[y][x].m_idx > 0)
		    && target_able(Ind, zcave[y][x].m_idx)
		    && m_list[zcave[y][x].m_idx].csleep) break;

#if 0
		/* Never pass through hostile player */
                if (dist && zcave[y][x].m_idx < 0) {
			if (check_hostile(Ind, -zcave[y][x].m_idx)) break;
		}
#endif

		/* Calculate the new location */
		mmove2(&y, &x, y1, x1, y2, x2);
	}

	/* Assume obstruction */
	return (FALSE);
}
#endif

/* Relates to projectable_real like projectable_wall to projectable */
#ifdef DOUBLE_LOS_SAFETY
static bool projectable_wall_real_DLS(int Ind, int y1, int x1, int y2, int x2, int range);
bool projectable_wall_real(int Ind, int y1, int x1, int y2, int x2, int range) {
	return ((projectable_wall_DLS(&Players[Ind]->wpos, y1, x1, y2, x2, range) || projectable_wall_DLS(&Players[Ind]->wpos, y2, x2, y1, x1, range))
	    /* important: make sure we really don't hit an awake monster from our direction of shooting :-p  : */
	    && projectable_wall_real_DLS(Ind, y1, x1, y2, x2, range));
}
static bool projectable_wall_real_DLS(int Ind, int y1, int x1, int y2, int x2, int range) {
	int dist, y, x;
	cave_type **zcave;
	if (!(zcave = getcave(&Players[Ind]->wpos))) return FALSE;

	/* Start at the initial location */
	y = y1, x = x1;

	/* See "project()" */
	for (dist = 0; dist <= range; dist++) {
		/* Check for arrival at "final target" */
		if ((x == x2) && (y == y2)) return (TRUE);

		/* Never pass through SLEEPING monsters */
                if (dist && (zcave[y][x].m_idx > 0)
		    && target_able(Ind, zcave[y][x].m_idx)
		    && m_list[zcave[y][x].m_idx].csleep) break;

#if 0
		/* Never pass through hostile player */
                if (dist && zcave[y][x].m_idx < 0) {
			if (check_hostile(Ind, -zcave[y][x].m_idx)) break;
		}
#endif

		/* Calculate the new location */
		mmove2(&y, &x, y1, x1, y2, x2);
	}

	/* Assume obstruction */
	return (FALSE);
}
#else
bool projectable_wall_real(int Ind, int y1, int x1, int y2, int x2, int range)
{
	int dist, y, x;
	cave_type **zcave;
	if (!(zcave = getcave(&Players[Ind]->wpos))) return FALSE;

	/* Start at the initial location */
	y = y1, x = x1;

	/* See "project()" */
	for (dist = 0; dist <= range; dist++) {
		/* Stopped by protected grids (Inn doors, also stopping monsters' ranged attacks!) */
                if (f_info[zcave[y][x].feat].flags1 & (FF1_BLOCK_LOS | FF1_BLOCK_CONTACT)) break;

		/* Check for arrival at "final target" */
		if ((x == x2) && (y == y2)) return (TRUE);

		/* Never pass through walls */
		if (dist && !cave_los(zcave, y, x)) break;

		/* Never pass through SLEEPING monsters */
                if (dist && (zcave[y][x].m_idx > 0)
		    && target_able(Ind, zcave[y][x].m_idx)
		    && m_list[zcave[y][x].m_idx].csleep) break;

#if 0
		/* Never pass through hostile player */
                if (dist && zcave[y][x].m_idx < 0) {
			if (check_hostile(Ind, -zcave[y][x].m_idx)) break;
		}
#endif

		/* Calculate the new location */
		mmove2(&y, &x, y1, x1, y2, x2);
	}

	/* Assume obstruction */
	return (FALSE);
}
#endif

/*
 * Standard "find me a location" function
 *
 * Obtains a legal location within the given distance of the initial
 * location, and with "los()" from the source to destination location.
 *
 * This function is often called from inside a loop which searches for
 * locations while increasing the "d" distance.
 *
 * m: FALSE = need LoS to x,y to be valid. TRUE = don't need LoS.
 */
/* if d<16, consider using tdi,tdy,tdx; considerably quicker! */
void scatter(struct worldpos *wpos, int *yp, int *xp, int y, int x, int d, int m) {
	int nx, ny;
	//long tries = 100000;
	/* Reduced to 10k to lessen lockups - mikaelh */
	long tries = 10000;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) {
		(*yp) = y;
		(*xp) = x;
		return;
	}

	/* Pick a location */
	while (TRUE) {
		/* Don't freeze in infinite loop!
		   Conditions: wpos is not allocated.
		   Note: los() returns TRUE if x==nx and y==ny.
		   Due to the conditions, following code will probably cause panic save,
		   that's still better than infinite loop though. */
		tries--;
		if (!tries) {
s_printf("!!! INFINITE LOOP IN scatter() !!! -- Please reboot server and do '/unsta %d,%d,%d (%d,%d; %d,%d; %d %d)'\n",
    wpos->wx, wpos->wy,wpos->wz, *yp, *xp, y, x, d, m);
			ny = y;
			nx = x;
			break;
		}

		/* Pick a new location */
		ny = rand_spread(y, d);
		nx = rand_spread(x, d);

		/* Ignore illegal locations and outer walls */
		if (!in_bounds(ny, nx)) continue;

		/* Ignore "excessively distant" locations */
		if ((d > 1) && (distance(y, x, ny, nx) > d)) continue;

		/* Require "line of sight" */
		if (m || los(wpos, y, x, ny, nx)) break;
	}

	/* Save the location */
	(*yp) = ny;
	(*xp) = nx;
}

/*
 * Track a new monster
 */
void health_track(int Ind, int m_idx)
{
	player_type *p_ptr = Players[Ind];

	/* Track a new guy */
	p_ptr->health_who = m_idx;

	/* Redraw (later) */
	p_ptr->redraw |= (PR_HEALTH);
}

/*
 * Update the health bars for anyone tracking a monster
 */
void update_health(int m_idx)
{
	player_type *p_ptr;
	int i;

	/* Each player */
	for (i = 1; i <= NumPlayers; i++)
	{
		p_ptr = Players[i];

		/* Check connection */
		if (p_ptr->conn == NOT_CONNECTED)
			continue;

		/* See if he is tracking this monster */
		if (p_ptr->health_who == m_idx)
		{
			/* Redraw */
			p_ptr->redraw |= (PR_HEALTH);
		}
	}
}



/*
 * Hack -- track the given monster race
 *
 * Monster recall is disabled for now --KLJ--
 */
void recent_track(int r_idx)
{
	/* Save this monster ID */
	/*recent_idx = r_idx;*/

	/* Window stuff */
	/*p_ptr->window |= (PW_MONSTER);*/
}




/*
 * Something has happened to disturb the player.
 *
 * The first arg indicates a major disturbance, which affects search.
 *
 * The second arg is currently unused, but could induce output flush.
 *
 * All disturbance cancels repeated commands, resting, and running.
 */
void disturb(int Ind, int stop_search, int keep_resting) {
	player_type *p_ptr = Players[Ind];

	/* Calm down from silly running */
	p_ptr->corner_turn = 0;

	/* Cancel auto-commands */
	/* command_new = 0; */

	/* Cancel repeated commands */
	if (p_ptr->command_rep) {
#ifdef USE_SOUND_2010
		if (p_ptr->command_rep != PKT_BASH) sound(Ind, NULL, NULL, SFX_TYPE_STOP, TRUE);
#endif

		/* Cancel */
		p_ptr->command_rep = 0;
#if defined(ENABLE_XID_SPELL) || defined(ENABLE_XID_MDEV)
		p_ptr->current_item = -1; //unnecessary?
#endif

		/* Hack -- Clear the buffer */
		Handle_clear_buffer(Ind);

		/* Redraw the state (later) */
		p_ptr->redraw |= (PR_STATE);
	}

	/* Cancel Resting */
	if (!keep_resting && p_ptr->resting) {
		/* Cancel */
		p_ptr->resting = 0;

		/* Redraw the state (later) */
		p_ptr->redraw |= (PR_STATE);
	}

	/* Cancel running */
	if (p_ptr->running) {
		/* Cancel */
		p_ptr->running = 0;

		/* Calculate torch radius */
		p_ptr->update |= (PU_TORCH);
	}

	/* Cancel searching if requested */
	if (stop_search && p_ptr->searching) {
		/* Cancel */
		p_ptr->searching = FALSE;

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Redraw stuff */
		p_ptr->redraw |= (PR_STATE | PR_MAP);

		/* cancel cloaking preparations too */
		stop_cloaking(Ind);
	}
}





/*
 * Hack -- Check if a level is a "quest" level
 */
/* FIXME - use worldpos and dungeon array! */
bool is_xorder(struct worldpos *wpos) {
	/* not implemented yet :p */
	return (FALSE);
#if 0
	int i;

	/* Town is never a quest */
	if (!level) return (FALSE);

	/* Check quests */
	for (i = 0; i < MAX_XO_IDX; i++) {
		/* Check for quest */
		if (xo_list[i].level == level) return (TRUE);
	}

	/* Nope */
	return (FALSE);
#endif
}

/*
 * handle spell effects
 */
static int effect_pop(int who)
{
	int i, cnt = 0;

	for (i = 1; i < MAX_EFFECTS; i++)	/* effects[0] is not used */
	{
		if (!effects[i].time) return i;
		if (effects[i].who == who)
		{
			if (++cnt > MAX_EFFECTS_PLAYER) return -1;
		}

	}
	return -1;
}

int new_effect(int who, int type, int dam, int time, int interval, worldpos *wpos, int cy, int cx, int rad, s32b flags)
{
	int i, who2 = who;
/*	player_type *p_ptr = NULL; */
#if 0 /* isn't this wrong? */
	if (who < 0 && who != PROJECTOR_EFFECT)
		who2 = 0 - Players[0 - who]->id;
#else
	if (who < 0 && who > PROJECTOR_UNUSUAL)
		who2 = 0 - Players[0 - who]->id;
#endif

        if ((i = effect_pop(who2)) == -1) return -1;
	effects[i].interval = interval;
        effects[i].type = type;
        effects[i].dam = dam;
        effects[i].time = time;
        effects[i].flags = flags;
        effects[i].cx = cx;
        effects[i].cy = cy;
        effects[i].rad = rad;
        effects[i].who = who2;
		wpcopy(&effects[i].wpos, wpos);
#ifdef ARCADE_SERVER
if (type == 209)
{
msg_broadcast(0, "mh");
if (flags > 0)
msg_broadcast(0, "some flags");
else
msg_broadcast(0, "no flags");
}
/*        if ((flags & EFF_CROSSHAIR_A) || (flags & EFF_CROSSHAIR_B) || (flags & EFF_CROSSHAIR_C))
        {
        msg_broadcast(0, "making an effect");
        player_type *pfft_ptr = Players[interval];
        pfft_ptr->e = i;
        } */
#endif

        return i;
}

bool allow_terraforming(struct worldpos *wpos, byte feat) {
	bool bree = in_bree(wpos);
	bool town = istown(wpos) || isdungeontown(wpos);
	bool townarea = istownarea(wpos, MAX_TOWNAREA);
//unused atm	bool dungeon_town = isdungeontown(wpos);
	bool sector00 = (in_sector00(wpos));
	bool valinor = in_valinor(wpos);
	bool nr_bottom = in_netherrealm(wpos) && getlevel(wpos) == netherrealm_end;
	bool arena_pvp = in_pvparena(wpos);
	bool arena_monster = (ge_special_sector && wpos->wx == WPOS_ARENA_X && wpos->wy == WPOS_ARENA_Y && wpos->wz == WPOS_ARENA_Z);

	/* usually allow all changes (normal dungeons and town-unrelated world map) */
	if (!arena_monster && !arena_pvp && !bree && !town && !townarea && !sector00 && !valinor && !nr_bottom) return(TRUE);

	/* preserve arenas; disallow trees for balancing (pvp-arena) */
	if (arena_pvp || arena_monster) return(FALSE);

	switch (feat) {
	/* water is annoying in all towns - mikaelh */
	case FEAT_SHAL_WATER:
	case FEAT_DEEP_WATER:
		if (town) return(FALSE);
		break;

	/* allow only harmless as well as non-obstructing changes: */
	case FEAT_IVY:
	case FEAT_DEAD_TREE:
        case FEAT_ICE:
		if (town || sector00 || valinor || nr_bottom) return(FALSE);
		break;

	case FEAT_WALL_EXTRA: /* tested by earthquake() and destroy_area() */
	case FEAT_SHAL_LAVA:
	case FEAT_DEEP_LAVA:
		if (town || townarea || sector00 || valinor || nr_bottom) return(FALSE);
		break;

	case FEAT_TREE: /* also for 'digging' and 'stone2mud' */
	case FEAT_BUSH: /* just moved here because FEAT_TREE is also here */
		if (town || sector00 || valinor || nr_bottom) return(FALSE);
		break;

	case FEAT_GLYPH:
	case FEAT_RUNE:
		/* generally allow in town, restrictions are applied in cave_set_feat_live().) */
		if (sector00 || valinor || nr_bottom) return(FALSE);
		break;

	/* don't allow any changes at all to preserve the visuals 100% */
        case FEAT_NONE:
    	case FEAT_FLOOR:
        case FEAT_DIRT:
        case FEAT_GRASS:
        case FEAT_SAND:
        case FEAT_ASH:
        case FEAT_MUD:
        case FEAT_FLOWER:
/*	case FEAT_PUDDLE: new feature to be added: same as shallow water, but dries out after a while */
		if (town || valinor || nr_bottom) return(FALSE);
		break;

	/* generate.c uses these for staircases in towns */
	case FEAT_MORE:
	case FEAT_LESS:
	case FEAT_WAY_MORE:
	case FEAT_WAY_LESS:
	case FEAT_BETWEEN:
	case FEAT_BEACON:
		break;

	/* forgot any? just paranoia */
	default: ;
	}

	return (TRUE);
}

/*
 * Queues a spot for redrawing at the beginning of the next turn.
 * Can be used for leaving temporary effects on the screen for one turn.
 */
void everyone_lite_later_spot(struct worldpos *wpos, int y, int x) {
	struct worldspot *wspot;

	if (lite_later_num >= lite_later_alloc) {
		/* Grow the array to meet the demand */
		GROW(lite_later, lite_later_alloc, lite_later_alloc * 2, struct worldspot);
		lite_later_alloc *= 2;
	}

	/* Add a new spot to the array */
	wspot = &lite_later[lite_later_num];
	lite_later_num++;

	wpcopy(&wspot->wpos, wpos);
	wspot->x = x;
	wspot->y = y;
}

/* change between the four seasons - C. Blue
    s:		the season (0..3 -> spring,summer,autumn,winter)
    force:	true -> force redrawing all sectors
		even if season didn't change. */
void season_change(int s, bool force) {
	int x, y;
	struct worldpos wpos;

	/* if 'force' is off, at least ensure correct weather frequency here: */
#if defined CLIENT_WEATHER_GLOBAL || !defined CLIENT_SIDE_WEATHER
	/* adjust weather somewhat according to season! (server-side or global weather) */
	switch (s) {
	case SEASON_SPRING: /* rain relatively often */
		weather_frequency = 2; break;
	case SEASON_SUMMER: /* rain rarely */
		weather_frequency = 1; break;
	case SEASON_AUTUMN: /* rain very often */
		weather_frequency = 3; break;
	case SEASON_WINTER: /* snow relatively often */
		weather_frequency = 2; break;
	}
#else
	/* adjust weather somewhat according to season! (client-side non-global weather) */
	switch (s) {
	case SEASON_SPRING: /* rain relatively often */
		max_clouds_seasonal = MAX_CLOUDS / 10; break;
	case SEASON_SUMMER: /* rain rarely */
		max_clouds_seasonal = MAX_CLOUDS / 14; break;
	case SEASON_AUTUMN: /* rain very often */
		max_clouds_seasonal = MAX_CLOUDS / 7; break;
	case SEASON_WINTER: /* snow pretty often */
		max_clouds_seasonal = MAX_CLOUDS / 8; break;
	}
#endif

	if ((season == s) && !force) return;
	season = s;

	/* make wilderness more lively again */
	lively_wild(0);

	/* rebuild wilderness + town layout everywhere! */
	wpos.wz = 0;
	for (x = 0; x < MAX_WILD_X; x++)
	for (y = 0; y < MAX_WILD_Y; y++) {
		wpos.wx = x;
		wpos.wy = y;
		if (getcave(&wpos)) regenerate_cave(&wpos);
	}

	s_printf("(%s) SEASON_CHANGE: %d", showtime(), s);
	if (s == SEASON_SPRING) {
		s_printf(" spring");
		world_surface_msg("\374\377GSpring has arrived.");
	}
	if (s == SEASON_SUMMER) {
		s_printf(" summer");
		world_surface_msg("\374\377GSummer has come.");
	}
	if (s == SEASON_AUTUMN) {
		s_printf(" autumn");
		world_surface_msg("\374\377GAutumn paints the scenery.");
	}
	if (s == SEASON_WINTER) {
		s_printf(" winter");
		world_surface_msg("\374\377GWinter embraces the lands.");
	}
	s_printf("\n");

	/* redraw map for all players */
	for (x = 1; x <= NumPlayers; x++) Players[x]->redraw |= PR_MAP;

	/* make all clouds terminate soon, to account for changed climate situation */
	for (x = 0; x < MAX_CLOUDS; x++) {
		/* does cloud exist? */
		if (cloud_dur[x]) /* also affects 'dur = -1' clouds */
			cloud_dur[x] = 5 + rand_int(10); /* terminate in 5..15 seconds */
	}
}

/* Update players' client-side weather. - C. Blue
   entered_level = TRUE : player just newly entered this sector
   weather_changed = TRUE : weather just changed, reset the player's
     client-side weather synchronization timings (see c-xtra1.c).
   NOTE: if entered_level is TRUE, so should be weather_changed!
   panel_redraw is only TRUE if we also want to instantly redraw the current
   panel without waiting for usual number of client ticks.
   NOTE: Called on opportunities (ie when player moves around)
         and also by players_weather(). */
#if 1
/* make rain fall down slower? */
 #define WEATHER_GEN_TICKS 3
 #define WEATHER_SNOW_MULT 3
#else
/* make rain fall down faster? */
 #define WEATHER_GEN_TICKS 2
 #define WEATHER_SNOW_MULT 4
#endif
void player_weather(int Ind, bool entered_level, bool weather_changed, bool panel_redraw) {
	player_type *p_ptr = Players[Ind];
	int w;

	/* running custom weather for debugging purpose? */
	if (p_ptr->custom_weather) return;

	/* not in dungeon? erase weather and exit */
	if (p_ptr->wpos.wz) {
		if (!entered_level) return;
		/* erase weather */
		Send_weather(Ind, -1, 0, WEATHER_GEN_TICKS,
		    WEATHER_GEN_TICKS, WEATHER_GEN_TICKS,
		    FALSE, TRUE);
		return;
	}

	/* no weather in sector00 during events */
	if (in_sector00(&p_ptr->wpos)) {
		if (!entered_level) return;
		/* erase weather */
		Send_weather(Ind, -1, 0, WEATHER_GEN_TICKS,
		    WEATHER_GEN_TICKS, WEATHER_GEN_TICKS,
		    FALSE, TRUE);
		return;
	}

#ifdef CLIENT_WEATHER_GLOBAL /* use global weather instead of sector-specific? */
	w = ((entered_level) ? ((weather) ? 200 : -1) : 0);
	Send_weather(Ind,
	    weather + w
	    + ((weather_changed && (w != -1)) ? 10000 : 0)
	    + ((weather && panel_redraw) ? 20000 : 0),
/*	    (wind_gust > 0 ? 1 : (wind_gust < 0 ? 2 : 0))
	    + ((wind_gust != 0) && (season == SEASON_WINTER) ? 4 : 0), */
	    (wind_gust > 0 ? 2 * (season == SEASON_WINTER ? WEATHER_SNOW_MULT : 1) * WEATHER_GEN_TICKS - 1
	    : (wind_gust < 0 ? 2 * (season == SEASON_WINTER ? WEATHER_SNOW_MULT : 1) * WEATHER_GEN_TICKS : 0)),
	    WEATHER_GEN_TICKS,
	    (season == SEASON_WINTER) ? 5 : 8,
	    (season == SEASON_WINTER) ? WEATHER_SNOW_MULT * WEATHER_GEN_TICKS :
	    /* hack: for non-windy rainfall, accelerate raindrop falling speed by 1: */
	    (wind_gust ? 1 * WEATHER_GEN_TICKS : WEATHER_GEN_TICKS - 1),
	    FALSE, TRUE); /* no virtual cloud if weather is global */

#else /* send his worldmap sector's specific weather situation */
	w = ((entered_level) ? ((wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].weather_type) ? 200 : -1) : 0); /* +(n*10): pre-generate n particles? */
 #if 0 /* buggy? */
	Send_weather(Ind,
	    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].weather_type + w
 #else
	Send_weather(Ind,
	    ((w == -1) ? -1 : (wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].weather_type + w))
 #endif
	    + ((weather_changed && (w != -1)) ? 10000 : 0) /* sync panel? */
 #if 0 /* glitchy? paranoia maybe */
	    + (weather && panel_redraw ? 20000 : 0),
 #else
	    + ((weather && panel_redraw && (w != -1)) ? 20000 : 0), /* insta-refresh? */
 #endif
	    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].weather_wind,
	    WEATHER_GEN_TICKS,
	    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].weather_intensity,
	    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].weather_speed,
	    TRUE, FALSE); /* <- apply clouds! */

 #ifdef TEST_SERVER /* DEBUG */
 #if 0
 s_printf("  updated weather for player %d to type %d (real: %d).\n", Ind,
    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].weather_type + w
    + ((weather_changed && (w != -1)) ? 10000 : 0)
    + (weather && panel_redraw ? 20000 : 0),
    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].weather_type);
 s_printf("  (wind %d, intensity %d, speed %d)\n",
    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].weather_wind,
    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].weather_intensity,
    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].weather_speed);
 #endif
 #endif
#endif
}


/* The is_stairs/is_doors hack in monster_can_cross_terrain() that allows
   aquatic monsters to cross doors and stairs in 'underwater' levels had the
   bad side effect of causing aquatic monsters on normal levels to seek out
   stairs/doors and get stuck on them. To fix this in turn, I added CAVE_WATERY
   and this hack that applies it. - C. Blue */
void aquatic_terrain_hack(cave_type **zcave, int x, int y) {
	int d, xx, yy;

	if (!is_always_passable(zcave[y][x].feat)) return;

	for (d = 1; d <= 9; d++) {
		if (d == 5) continue;
		xx = x + ddx[d];
		yy = y + ddy[d];
		if (!in_bounds(yy, xx)) continue;
		if (zcave[yy][xx].feat == FEAT_DEEP_WATER) {
			zcave[y][x].info |= CAVE_WATERY;
			return;
		}
	}
}

/* Special wpos that ought to keep monsters and/or objects instead of deleting
   or compacting them, even though the floor might not necessarily be kept
   static. - C. Blue
   Added for IDDC static towns, could maybe also be used for quests. */
bool sustained_wpos(struct worldpos *wpos) {
	if (is_fixed_irondeepdive_town(wpos, getlevel(wpos))) return TRUE;
	return FALSE;
}
