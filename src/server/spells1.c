/* $Id$ */
/* File: spells1.c */

/* Purpose: Spell code (part 1) */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define SERVER

#include "angband.h"


/* 1/x chance of reducing stats (for elemental attacks) */
#define HURT_CHANCE 16

/* chance of equipments getting hurt from attacks, in percent [2] */
#define HARM_EQUIP_CHANCE	2

/* macro to determine the way stat gets reduced by element attacks */
#define	DAM_STAT_TYPE(inv) \
	(magik(inv*25) ? STAT_DEC_NORMAL : STAT_DEC_TEMPORARY)

/* 
 * Maximum lower limit for player teleportation.	[30]
 * This should make teleportation of high-lv players look 'more random'.
 * To disable, comment it out.
 */
#define TELEPORT_MIN_LIMIT	30

/* Default radius of Space-Time anchor.	[12] */
#define ST_ANCHOR_DIS	12

/* Limitation for teleport radius on the wilderness.	[20] */
#define WILDERNESS_TELEPORT_RADIUS	40


 /*
  * Potions "smash open" and cause an area effect when
  * (1) they are shattered while in the player's inventory,
  * due to cold (etc) attacks;
  * (2) they are thrown at a monster, or obstacle;
  * (3) they are shattered by a "cold ball" or other such spell
  * while lying on the floor.
  *
  * Arguments:
  *    who   ---  who caused the potion to shatter (0=player)
  *          potions that smash on the floor are assumed to
  *          be caused by no-one (who = 1), as are those that
  *          shatter inside the player inventory.
  *          (Not anymore -- I changed this; TY)
  *    y, x  --- coordinates of the potion (or player if
  *          the potion was in her inventory);
  *    o_ptr --- pointer to the potion object.
  */
/*
 * Copy & Pasted from ToME :)	- Jir -
 * NOTE: 
 * seemingly TV_POTION2 are not haldled right (ToME native bug?).
 */
bool potion_smash_effect(int who, worldpos *wpos, int y, int x, int o_sval)
{
	int     radius = 2;
	int     dt = 0;
	int     dam = 0;
	bool    ident = FALSE;
	bool    angry = FALSE;

	switch(o_sval)
	{
		case SV_POTION_SALT_WATER:
		case SV_POTION_SLIME_MOLD:
		case SV_POTION_LOSE_MEMORIES:
		case SV_POTION_DEC_STR:
		case SV_POTION_DEC_INT:
		case SV_POTION_DEC_WIS:
		case SV_POTION_DEC_DEX:
		case SV_POTION_DEC_CON:
		case SV_POTION_DEC_CHR:
		case SV_POTION_WATER:   /* perhaps a 'water' attack? */
		case SV_POTION_APPLE_JUICE:
			return TRUE;

		case SV_POTION_INFRAVISION:
		case SV_POTION_DETECT_INVIS:
		case SV_POTION_SLOW_POISON:
		case SV_POTION_CURE_POISON:
		case SV_POTION_BOLDNESS:
		case SV_POTION_RESIST_HEAT:
		case SV_POTION_RESIST_COLD:
		case SV_POTION_HEROISM:
		case SV_POTION_BESERK_STRENGTH:
		case SV_POTION_RESTORE_EXP:
		case SV_POTION_RES_STR:
		case SV_POTION_RES_INT:
		case SV_POTION_RES_WIS:
		case SV_POTION_RES_DEX:
		case SV_POTION_RES_CON:
		case SV_POTION_RES_CHR:
		case SV_POTION_INC_STR:
		case SV_POTION_INC_INT:
		case SV_POTION_INC_WIS:
		case SV_POTION_INC_DEX:
		case SV_POTION_INC_CON:
		case SV_POTION_INC_CHR:
		case SV_POTION_AUGMENTATION:
		case SV_POTION_ENLIGHTENMENT:
		case SV_POTION_STAR_ENLIGHTENMENT:
		case SV_POTION_SELF_KNOWLEDGE:
		case SV_POTION_EXPERIENCE:
		case SV_POTION_RESISTANCE:
		case SV_POTION_INVULNERABILITY:
		case SV_POTION_NEW_LIFE:
			/* All of the above potions have no effect when shattered */
			return FALSE;
		case SV_POTION_SLOWNESS:
			dt = GF_OLD_SLOW;
			dam = 5;
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_POISON:
			dt = GF_POIS;
			dam = 3;
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_BLINDNESS:
			dt = GF_DARK;
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_CONFUSION: /* Booze */
			dt = GF_OLD_CONF;
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_SLEEP:
			dt = GF_OLD_SLEEP;
			angry = TRUE;
			ident = TRUE;
			break;
		case SV_POTION_RUINATION:
		case SV_POTION_DETONATIONS:
			dt = GF_SHARDS;
			dam = damroll(25, 25);
			angry = TRUE;
			ident = TRUE;
			break;
		case SV_POTION_DEATH:
//			dt = GF_DEATH_RAY;    /* !! */	/* not implemented yet. */
			dt = GF_MANA;		/* let's use mana damage for now.. */
			dam = damroll(30,30);
			angry = TRUE;
			radius = 1;
			ident = TRUE;
			break;
		case SV_POTION_SPEED:
			dt = GF_OLD_SPEED;
			ident = TRUE;
			break;
		case SV_POTION_CURE_LIGHT:
			dt = GF_OLD_HEAL;
			dam = damroll(2,3);
			ident = TRUE;
			break;
		case SV_POTION_CURE_SERIOUS:
			dt = GF_OLD_HEAL;
			dam = damroll(4,3);
			ident = TRUE;
			break;
		case SV_POTION_CURE_CRITICAL:
		case SV_POTION_CURING:
			dt = GF_OLD_HEAL;
			dam = damroll(6,3);
			ident = TRUE;
			break;
		case SV_POTION_HEALING:
			dt = GF_OLD_HEAL;
			dam = damroll(10,10);
			ident = TRUE;
			break;
		case SV_POTION_STAR_HEALING:
		case SV_POTION_LIFE:
			dt = GF_OLD_HEAL;
			dam = damroll(50,50);
			radius = 1;
			ident = TRUE;
			break;
		case SV_POTION_RESTORE_MANA:   /* MANA */
			dt = GF_MANA;
			dam = damroll(10,10);
			radius = 1;
			ident = TRUE;
			break;
		default:
			/* Do nothing */  ;
	}

	(void) project(who, radius, wpos, y, x, dam, dt,
			   (PROJECT_JUMP | PROJECT_ITEM | PROJECT_KILL | PROJECT_SELF));

	/* XXX	those potions that explode need to become "known" */
	return angry;
}


/*
 * Helper function -- return a "nearby" race for polymorphing
 *
 * Note that this function is one of the more "dangerous" ones...
 */
s16b poly_r_idx(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	int i, r, lev1, lev2;

	/* Hack -- Uniques never polymorph */
	if (r_ptr->flags1 & RF1_UNIQUE) return (r_idx);

	/* Allowable range of "levels" for resulting monster */
	lev1 = r_ptr->level - ((randint(20)/randint(9))+1);
	lev2 = r_ptr->level + ((randint(20)/randint(9))+1);

	/* Pick a (possibly new) non-unique race */
	for (i = 0; i < 1000; i++)
	{
		/* Pick a new race, using a level calculation */
		/* Don't base this on "dlev" */
		/*r = get_mon_num((dlev + r_ptr->level) / 2 + 5);*/
		r = get_mon_num(r_ptr->level + 5, 0);

		/* Handle failure */
		if (!r) break;

		/* Obtain race */
		r_ptr = &r_info[r];

		/* Ignore unique monsters */
		if (r_ptr->flags1 & RF1_UNIQUE) continue;

		/* Ignore monsters with incompatible levels */
		if ((r_ptr->level < lev1) || (r_ptr->level > lev2)) continue;

		/* Use that index */
		r_idx = r;

		/* Done */
		break;
	}

	/* Result */
	return (r_idx);
}

/*
 * It is radius-based now.
 *
 * TODO:
 * - give some messages
 * - allow monsters to have st-anchor?
 */
//bool check_st_anchor(struct worldpos *wpos)
bool check_st_anchor(struct worldpos *wpos, int y, int x)
{
	int i;

	dun_level		*l_ptr = getfloor(wpos);

	for (i = 1; i <= NumPlayers; i++)
	  {
		player_type *q_ptr = Players[i];

		/* Skip disconnected players */
		if (q_ptr->conn == NOT_CONNECTED) continue;

		/* Skip players not on this depth */
		if (!inarea(&q_ptr->wpos, wpos)) continue;

		/* Maybe CAVE_ICKY/CAVE_STCK can be checked here */

//		if (!q_ptr->st_anchor) continue;
//		if (!q_ptr->anti_tele) continue;
		if (!q_ptr->resist_continuum) continue;

		/* Compute distance */
		if (distance(y, x, q_ptr->py, q_ptr->px) > ST_ANCHOR_DIS)
			continue;

//		if(istown(wpos) && randint(100)>q_ptr->lev) continue;

		return TRUE;
	  }

	/* Assume no st_anchor */
	return FALSE;
}


/*
 * Teleport a monster, normally up to "dis" grids away.
 *
 * Attempt to move the monster at least "dis/2" grids away.
 *
 * But allow variation to prevent infinite loops.
 */
bool teleport_away(int m_idx, int dis)
{
	int		oy, ox, d, i, min;
	int		ny=0, nx=0;

	bool		look = TRUE;

	monster_type	*m_ptr = &m_list[m_idx];
        monster_race    *r_ptr = race_inf(m_ptr);
	dun_level *l_ptr;
	struct worldpos *wpos;
	cave_type **zcave;

	/* Paranoia */
	if (!m_ptr->r_idx) return FALSE;

	if (r_ptr->flags9 & RF9_IM_TELE) return FALSE;

	/* Save the old location */
	oy = m_ptr->fy;
	ox = m_ptr->fx;

	/* Space/Time Anchor */
	if (check_st_anchor(&m_ptr->wpos, oy, ox)) return FALSE;


	wpos=&m_ptr->wpos;
	if(!(zcave=getcave(wpos))) return FALSE;
	l_ptr = getfloor(wpos);

	/* Minimum distance */
	min = dis / 2;

	/* Look until done */
	while (look)
	{
		/* Verify max distance */
		if (dis > 200) dis = 200;

		/* Try several locations */
		for (i = 0; i < 500; i++)
		{
			/* Pick a (possibly illegal) location */
			while (1)
			{
				ny = rand_spread(oy, dis);
				nx = rand_spread(ox, dis);
				d = distance(oy, ox, ny, nx);
				if ((d >= min) && (d <= dis)) break;
			}

			/* Ignore illegal locations */
			if (!in_bounds4(l_ptr, ny, nx)) continue;

			/* Require "empty" floor space */
			if (!cave_empty_bold(zcave, ny, nx)) continue;

			/* Hack -- no teleport onto glyph of warding */
			if (zcave[ny][nx].feat == FEAT_GLYPH) continue;
			/* No teleporting into vaults and such */
			if (zcave[ny][nx].info & CAVE_ICKY) continue;

			/* Space/Time Anchor */
			if (check_st_anchor(&m_ptr->wpos, ny, nx)) return FALSE;

			/* This grid looks good */
			look = FALSE;

			/* Stop looking */
			break;
		}

		/* Increase the maximum distance */
		dis = dis * 2;

		/* Decrease the minimum distance */
		min = min / 2;
	}

	/* Update the new location */
	zcave[ny][nx].m_idx = m_idx;

	/* Update the old location */
	zcave[oy][ox].m_idx = 0;

	/* Move the monster */
	m_ptr->fy = ny;
	m_ptr->fx = nx;

	/* Update the monster (new location) */
	update_mon(m_idx, TRUE);

	/* Redraw the old grid */
	everyone_lite_spot(wpos, oy, ox);

	/* Redraw the new grid */
	everyone_lite_spot(wpos, ny, nx);

	/* Succeeded. */
	return TRUE;
}


/*
 * Teleport monster next to the player
 */
void teleport_to_player(int Ind, int m_idx)
{
	player_type *p_ptr = Players[Ind];
	int ny=0, nx=0, oy, ox, d, i, min;
	int dis = 2;

	bool look = TRUE;

	monster_type *m_ptr = &m_list[m_idx];
//	int attempts = 500;
	int attempts = 200;

	struct worldpos *wpos=&m_ptr->wpos;
	dun_level		*l_ptr = getfloor(wpos);

	cave_type **zcave;
//        if(p_ptr->resist_continuum) {msg_print("The space-time continuum can't be disrupted."); return;}

	/* Paranoia */
	if (!m_ptr->r_idx) return;

	/* "Skill" test */
//	if (randint(100) > m_ptr->level) return;	/* not here */

	if(!(zcave=getcave(wpos))) return;

	/* Save the old location */
	oy = m_ptr->fy;
	ox = m_ptr->fx;

	/* Hrm, I cannot remember/see why it's commented out..
	 * maybe pets and golems need it? */
//	if (check_st_anchor(wpos)) return;
	if (check_st_anchor(wpos, oy, ox)) return;

	/* Minimum distance */
	min = dis / 2;

	/* Look until done */
	while (look && --attempts)
	{
		/* Verify max distance */
		if (dis > 200) dis = 200;

		/* Try several locations */
		for (i = 0; i < 500; i++)
		{
			/* Pick a (possibly illegal) location */
			while (1)
			{
				ny = rand_spread(p_ptr->py, dis);
				nx = rand_spread(p_ptr->px, dis);
				d = distance(p_ptr->py, p_ptr->px, ny, nx);
				if ((d >= min) && (d <= dis)) break;
			}

			/* Ignore illegal locations */
			if (!in_bounds(ny, nx)) continue;

			/* Require "empty" floor space */
			if (!cave_empty_bold(zcave, ny, nx)) continue;

			/* Hack -- no teleport onto glyph of warding */
			if (zcave[ny][nx].feat == FEAT_GLYPH) continue;
#if 0
			if (cave[ny][nx].feat == FEAT_MINOR_GLYPH) continue;

			/* ...nor onto the Pattern */
			if ((cave[ny][nx].feat >= FEAT_PATTERN_START) &&
			    (cave[ny][nx].feat <= FEAT_PATTERN_XTRA2)) continue;
#endif	// 0

			/* No teleporting into vaults and such */
			/* if (cave[ny][nx].info & (CAVE_ICKY)) continue; */
			if (zcave[ny][nx].info & CAVE_ICKY) continue;

			if (check_st_anchor(wpos, ny, nx)) return;

			/* This grid looks good */
			look = FALSE;

			/* Stop looking */
			break;
		}

		/* Increase the maximum distance */
		dis = dis * 2;

		/* Decrease the minimum distance */
		min = min / 2;
	}

	if (attempts < 1) return;

	/* Sound */
//	sound(SOUND_TPOTHER);

	/* Update the new location */
	zcave[ny][nx].m_idx = m_idx;

	/* Update the old location */
	zcave[oy][ox].m_idx = 0;

	/* Move the monster */
	m_ptr->fy = ny;
	m_ptr->fx = nx;

	/* Update the monster (new location) */
	update_mon(m_idx, TRUE);

	/* Redraw the old grid */
	everyone_lite_spot(wpos, oy, ox);

	/* Redraw the new grid */
	everyone_lite_spot(wpos, ny, nx);
}


/*
 * Teleport the player to a location up to "dis" grids away.
 *
 * If no such spaces are readily available, the distance may increase.
 * Try very hard to move the player at least a quarter that distance.
 */
void teleport_player(int Ind, int dis)
{
	player_type *p_ptr = Players[Ind];

	int d, i, min, ox, oy, x = p_ptr->py, y = p_ptr->px;
	int xx , yy, m_idx;
	worldpos *wpos=&p_ptr->wpos;
	dun_level *l_ptr;

	bool look = TRUE;

	/* Space/Time Anchor */
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;

	/* Hack -- Teleportation when died is always allowed */
	if (!p_ptr->death)
	{
		if (p_ptr->anti_tele || check_st_anchor(wpos, p_ptr->py, p_ptr->px)) return;
		if(zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) return;
		/* Hack -- on the wilderness one cannot teleport very far */
		/* Double death isnt nice */
		if (!wpos->wz && !istown(wpos) && dis > WILDERNESS_TELEPORT_RADIUS)
			dis = WILDERNESS_TELEPORT_RADIUS;
	}
	l_ptr = getfloor(wpos);

	/* Verify max distance once here */
	if (dis > 150) dis = 150;

	/* Minimum distance */
	min = dis / 2;

#ifdef TELEPORTATION_MIN_LIMIT
	if (min > TELEPORT_MIN_LIMIT) min = TELEPORT_MIN_LIMIT;
#endif	// TELEPORTATION_MIN_LIMIT

	/* Look until done */
	while (look)
	{
		/* Verify max distance */
		if (dis > 150) dis = 150;   /* 200 */

		/* Try several locations */
		for (i = 0; i < 500; i++)
		{
			/* Pick a (possibly illegal) location */
			while (1)
			{
				y = rand_spread(p_ptr->py, dis);
				x = rand_spread(p_ptr->px, dis);

				d = distance(p_ptr->py, p_ptr->px, y, x);
				//plog(format("y%d x%d d%d min%d dis%d", y, x, d, min, dis));
				if ((d >= min) && (d <= dis)) break;
			}

			/* Ignore illegal locations */
			if (!in_bounds4(l_ptr, y, x)) continue;

			/* Require floor space if not ghost */
			if (!p_ptr->ghost && !cave_naked_bold(zcave, y, x)) continue;

			/* Require empty space if a ghost */
			if (p_ptr->ghost && zcave[y][x].m_idx) continue;

			/* No teleporting into vaults and such */
			if (zcave[y][x].info & CAVE_ICKY) continue;

			/* Never break into st-anchor */
			if (!p_ptr->death && check_st_anchor(wpos, y, x)) return;

			/* This grid looks good */
			look = FALSE;

			/* Stop looking */
			break;
		}

		/* Increase the maximum distance */
		dis = dis * 2;

		/* Decrease the minimum distance */
		min = min / 2;
	}

	/* Save the old location */
	oy = p_ptr->py;
	ox = p_ptr->px;

	/* Move the player */
	p_ptr->py = y;
	p_ptr->px = x;

	/* The player isn't on his old spot anymore */
	zcave[oy][ox].m_idx = 0;

	/* The player is on his new spot */
	zcave[y][x].m_idx = 0 - Ind;

	/* Redraw the old spot */
	everyone_lite_spot(wpos, oy, ox);

	/* Don't leave me alone Daisy! */
	for (d = 1; d <= 9; d++)
	{
		if (d == 5) continue;

		xx = ox + ddx[d];
		yy = oy + ddy[d];

		if (!in_bounds4(l_ptr, yy, xx)) continue;

		if ((m_idx = zcave[yy][xx].m_idx)>0)
		{
			monster_race *r_ptr = race_inf(&m_list[m_idx]);

			if ((r_ptr->flags6 & RF6_TPORT) &&
				!(r_ptr->flags3 & RF3_RES_TELE))
				/*
				 * The latter limitation is to avoid
				 * totally unkillable suckers...
				 */
			{
				if (!(m_list[m_idx].csleep) &&
						!mon_will_run(Ind, m_idx))
				{
					/* "Skill" test */
					if (randint(100) < r_ptr->level) 
						teleport_to_player(Ind, m_idx);
				}
			}
		}
	}


	/* Redraw the new spot */
	everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);

	/* Check for new panel (redraw map) */
	verify_panel(Ind);

	/* Update stuff */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

	/* Update the monsters */
	p_ptr->update |= (PU_DISTANCE);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);

	/* Handle stuff XXX XXX XXX */
	if (!p_ptr->death) handle_stuff(Ind);
}



/*
 * Teleport player to a grid near the given location
 *
 * This function is slightly obsessive about correctness.
 * This function allows teleporting into vaults (!)
 */
void teleport_player_to(int Ind, int ny, int nx)
{
	player_type *p_ptr = Players[Ind];

	int y, x, oy, ox, dis = 1, ctr = 0;
	struct worldpos *wpos=&p_ptr->wpos;
	int tries = 200;
	dun_level *l_ptr;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
	if (p_ptr->anti_tele) return;
	if(zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) return;
	l_ptr = getfloor(wpos);

	if (ny < 1) ny = 1;
	if (nx < 1) nx = 1;
	if (ny > MAX_HGT - 2) ny = MAX_HGT - 2;
	if (nx > MAX_WID - 2) nx = MAX_WID - 2;

	/* Space/Time Anchor */
	if (check_st_anchor(wpos, ny, nx)) return;

	/* Find a usable location */
	while (tries--)
	{
		/* Pick a nearby legal location */
		while (1)
		{
			y = rand_spread(ny, dis);
			x = rand_spread(nx, dis);
			if (in_bounds4(l_ptr, y, x)) break;
		}

		/* Cant telep in houses */
		if (((wpos->wz==0) && !(zcave[y][x].info & CAVE_ICKY)) || (wpos->wz))
		{
			if (cave_naked_bold(zcave, y, x))
			{
				/* Never break into st-anchor */
				if (!check_st_anchor(wpos, y, x)) break;
			}

		}

		/* Occasionally advance the distance */
		if (++ctr > (4 * dis * dis + 4 * dis + 1))
		{
			ctr = 0;
			dis++;
		}
	}

	/* Save the old location */
	oy = p_ptr->py;
	ox = p_ptr->px;

	/* Move the player */
	p_ptr->py = y;
	p_ptr->px = x;

	/* The player isn't here anymore */
	zcave[oy][ox].m_idx = 0;

	/* The player is now here */
	zcave[y][x].m_idx = 0 - Ind;

	/* Redraw the old spot */
	everyone_lite_spot(wpos, oy, ox);

	/* Redraw the new spot */
	everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);

	/* Check for new panel (redraw map) */
	verify_panel(Ind);

	/* Update stuff */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

	/* Update the monsters */
	p_ptr->update |= (PU_DISTANCE);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);

	/* Handle stuff XXX XXX XXX */
	handle_stuff(Ind);
}



/*
 * Teleport the player one level up or down (random when legal)
 *
 * Note that keeping the "players_on_depth" array correct is VERY important,
 * otherwise levels with players still on them might be destroyed, or empty
 * levels could be kept around, wasting memory.
 */
 
 /* in the wilderness, teleport to a neighboring wilderness level. */
 
void teleport_player_level(int Ind)
{
	player_type *p_ptr = Players[Ind];
	wilderness_type *w_ptr;
	struct worldpos *wpos=&p_ptr->wpos;
	struct worldpos new_depth;
	char *msg="\377rCritical bug!";
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
	if(zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) return;

	/* Space/Time Anchor */
	if (p_ptr->anti_tele) return;
	if (check_st_anchor(&p_ptr->wpos, p_ptr->py, p_ptr->px)) return;

	wpcopy(&new_depth, wpos);
	/* sometimes go down */
	if(can_go_down(wpos) && ((!can_go_up(wpos) || (rand_int(100)<50)) || (wpos->wz<0 && wild_info[wpos->wy][wpos->wx].dungeon->flags2 & DF2_IRON)))
	{
		new_depth.wz--;
		msg = "You sink through the floor.";
		p_ptr->new_level_method = (new_depth.wz || (istown(&new_depth)) ? LEVEL_RAND : LEVEL_OUTSIDE_RAND);
	}	
	/* else go up */
	else if(can_go_up(wpos) && !(wpos->wz>0 && wild_info[wpos->wy][wpos->wx].tower->flags2 & DF2_IRON))
	{
		new_depth.wz++;
		msg = "You rise up through the ceiling.";
		p_ptr->new_level_method = (new_depth.wz || (istown(&new_depth)) ? LEVEL_RAND : LEVEL_OUTSIDE_RAND);
	}
	
	/* If in the wilderness, teleport to a random neighboring level */
	else if(wpos->wz==0 && new_depth.wz==0)
	{
		w_ptr = &wild_info[wpos->wy][wpos->wx];
		/* get a valid neighbor */
		wpcopy(&new_depth, wpos);
		do
		{	
			switch (rand_int(4))
			{
				case DIR_NORTH:
					if(new_depth.wy<MAX_WILD_Y)
						new_depth.wy++;
					msg = "A gust of wind blows you north.";
					break;
				case DIR_EAST:
					if(new_depth.wx<MAX_WILD_X)
						new_depth.wx++;
					msg = "A gust of wind blows you east.";
					break;
				case DIR_SOUTH:
					if(new_depth.wy>0)
						new_depth.wy--;
					msg = "A gust of wind blows you south.";
					break;
				case DIR_WEST:
					if(new_depth.wx>0)
						new_depth.wx--;
					msg = "A gust of wind blows you west.";
					break;		
			}
		}
		while(inarea(wpos, &new_depth));

		p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
		/* update the players new wilderness location */
		
		/* update the players wilderness map */
		if(!p_ptr->ghost)
			p_ptr->wild_map[(new_depth.wx + new_depth.wy*MAX_WILD_X)/8] |= (1<<((new_depth.wx + new_depth.wy*MAX_WILD_X)%8));
	}
	else{
		msg_print(Ind, "The scroll seemed to fail");
		return;
	}
	
	/* Tell the player */
	msg_print(Ind, msg);

	/* One less player here */
	new_players_on_depth(wpos,-1,TRUE);

	/* Remove the player */
	zcave[p_ptr->py][p_ptr->px].m_idx = 0;

	/* Show that he's left */
	everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);

	/* Forget his lite and viewing area */
	forget_lite(Ind);
	forget_view(Ind);

	wpcopy(wpos,&new_depth);

	/* One more player here */
	new_players_on_depth(wpos,1,TRUE);

	p_ptr->new_level_flag = TRUE;
}






/*
 * Get a legal "multi-hued" color for drawing "spells"
 */
#if 0
static byte mh_attr(void)
{
	switch (randint(9))
	{
		case 1: return (TERM_RED);
		case 2: return (TERM_GREEN);
		case 3: return (TERM_BLUE);
		case 4: return (TERM_YELLOW);
		case 5: return (TERM_ORANGE);
		case 6: return (TERM_VIOLET);
		case 7: return (TERM_L_RED);
		case 8: return (TERM_L_GREEN);
		case 9: return (TERM_L_BLUE);
	}

	return (TERM_WHITE);
}
#else
static byte mh_attr(int max)
{
	switch (randint(max))
	{
		case  1: return (TERM_RED);
		case  2: return (TERM_GREEN);
		case  3: return (TERM_BLUE);
		case  4: return (TERM_YELLOW);
		case  5: return (TERM_ORANGE);
		case  6: return (TERM_VIOLET);
		case  7: return (TERM_L_RED);
		case  8: return (TERM_L_GREEN);
		case  9: return (TERM_L_BLUE);
		case 10: return (TERM_UMBER);
		case 11: return (TERM_L_UMBER);
		case 12: return (TERM_SLATE);
		case 13: return (TERM_WHITE);
		case 14: return (TERM_L_WHITE);
		case 15: return (TERM_L_DARK);
	}

	return (TERM_WHITE);
}
#endif	// 0



/*
 * Return a color to use for the bolt/ball spells
 */
byte spell_color(int type)
{
	/* Hack -- fake monochrome */
	if (!use_color) return (TERM_WHITE);

	/* Analyze */
#if 0
	switch (type)
	{
		case GF_MISSILE:	return (TERM_MULTI);
		case GF_ACID:		return (TERM_SLATE);
		case GF_ELEC:		return (TERM_BLUE);
		case GF_FIRE:		return (TERM_RED);
		case GF_COLD:		return (TERM_WHITE);
		case GF_POIS:		return (TERM_GREEN);
		case GF_HOLY_ORB:	return (TERM_L_DARK);
			case GF_HOLY_FIRE:      return (randint(5)==1?TERM_ORANGE:TERM_WHITE);
//			case GF_HELL_FIRE:      return (randint(6)==1?TERM_RED:TERM_L_DARK);
		case GF_MANA:		return (TERM_L_DARK);
		case GF_ARROW:		return (TERM_WHITE);
		case GF_WATER:		return (TERM_SLATE);
		case GF_NETHER:		return (TERM_L_GREEN);
		case GF_CHAOS:		return (TERM_MULTI);
		case GF_DISENCHANT:	return (TERM_VIOLET);
		case GF_NEXUS:		return (TERM_L_RED);
		case GF_CONFUSION:	return (TERM_L_UMBER);
		case GF_SOUND:		return (TERM_YELLOW);
		case GF_SHARDS:		return (TERM_UMBER);
		case GF_FORCE:		return (TERM_UMBER);
		case GF_INERTIA:	return (TERM_L_WHITE);
		case GF_GRAVITY:	return (TERM_L_WHITE);
		case GF_TIME:		return (TERM_L_BLUE);
		case GF_LITE_WEAK:	return (TERM_ORANGE);
		case GF_LITE:		return (TERM_ORANGE);
		case GF_DARK_WEAK:	return (TERM_L_DARK);
		case GF_DARK:		return (TERM_L_DARK);
		case GF_PLASMA:		return (TERM_RED);
		case GF_METEOR:		return (TERM_RED);
		case GF_ICE:		return (TERM_WHITE);
			case GF_NUKE:           return (TERM_MULTI);
			case GF_DISINTEGRATE:   return (0x05);
	}
#else	// 0
	switch (type)	/* colourful ToME ones :) */
	{
		case GF_MISSILE:        return (TERM_SLATE);
		case GF_ACID:           return (TERM_ACID);
		case GF_ELEC:           return (TERM_ELEC);
		case GF_FIRE:           return (TERM_FIRE);
		case GF_COLD:           return (TERM_COLD);
		case GF_POIS:           return (TERM_POIS);
		case GF_UNBREATH:       return (randint(7)<3?TERM_L_GREEN:TERM_GREEN);
//		case GF_HOLY_ORB:	return (TERM_L_DARK);
		case GF_HOLY_ORB:		 return (randint(6)==1?TERM_RED:TERM_L_DARK);
		case GF_HOLY_FIRE:      return (randint(5)==1?TERM_ORANGE:TERM_WHITE);
//		case GF_HELL_FIRE:      return (randint(6)==1?TERM_RED:TERM_L_DARK);
		case GF_MANA:           return (randint(5)!=1?TERM_VIOLET:TERM_L_BLUE);
		case GF_ARROW:          return (TERM_L_UMBER);
		case GF_WATER:          return (randint(4)==1?TERM_L_BLUE:TERM_BLUE);
		case GF_WAVE:           return (randint(4)==1?TERM_L_BLUE:TERM_BLUE);
		case GF_NETHER:         return (randint(4)==1?TERM_SLATE:TERM_L_DARK);
		case GF_CHAOS:          return (TERM_MULTI);
		case GF_DISENCHANT:     return (randint(5)!=1?TERM_L_BLUE:TERM_VIOLET);
		case GF_NEXUS:          return (randint(5)<3?TERM_L_RED:TERM_VIOLET);
		case GF_CONFUSION:      return (mh_attr(4));
		case GF_SOUND:          return (randint(4)==1?TERM_VIOLET:TERM_WHITE);
		case GF_SHARDS:         return (randint(5)<3?TERM_UMBER:TERM_SLATE);
		case GF_FORCE:          return (randint(5)<3?TERM_L_WHITE:TERM_ORANGE);
		case GF_INERTIA:        return (randint(5)<3?TERM_SLATE:TERM_L_WHITE);
		case GF_GRAVITY:        return (randint(3)==1?TERM_L_UMBER:TERM_UMBER);
		case GF_TIME:           return (randint(2)==1?TERM_WHITE:TERM_L_DARK);
		case GF_LITE_WEAK:      return (TERM_LITE);
		case GF_LITE:           return (TERM_LITE);
		case GF_DARK_WEAK:      return (TERM_DARKNESS);
		case GF_DARK:           return (TERM_DARKNESS);
		case GF_PLASMA:         return (randint(5)==1?TERM_RED:TERM_L_RED);
		case GF_METEOR:         return (randint(3)==1?TERM_RED:TERM_UMBER);
		case GF_ICE:            return (randint(4)==1?TERM_L_BLUE:TERM_WHITE);
		case GF_ROCKET:         return (randint(6)<4?TERM_L_RED:(randint(4)==1?TERM_RED:TERM_L_UMBER));
#if 0
		case GF_DEATH:
		case GF_DEATH_RAY:
								return (TERM_L_DARK);
#endif	// 0
		case GF_NUKE:           return (mh_attr(2));
		case GF_DISINTEGRATE:   return (randint(3)!=1?TERM_L_DARK:(randint(2)==1?TERM_ORANGE:TERM_L_UMBER));
		case GF_PSI:
#if 0
		case GF_PSI_DRAIN:
		case GF_TELEKINESIS:
		case GF_DOMINATION:
#endif	// 0
								return (randint(3)!=1?TERM_L_BLUE:TERM_WHITE);
	}
#endif	// 0

	/* Standard "color" */
	return (TERM_WHITE);
}

/*
 * Decreases players hit points and sets death flag if necessary
 *
 * XXX XXX XXX Invulnerability needs to be changed into a "shield"
 *
 * XXX XXX XXX Hack -- this function allows the user to save (or quit)
 * the game when he dies, since the "You die." message is shown before
 * setting the player to "dead".
 */
/* Poison/Cut timed damages, curse damages etc can bypass the shield.
 * It's hack; in the future, this function should be called with flags
 * that tells 'what kind of damage'.
 */
bool bypass_invuln = FALSE;

void take_hit(int Ind, int damage, cptr hit_from)
{
	player_type *p_ptr = Players[Ind];

	// This is probably unused
	// int warning = (p_ptr->mhp * hitpoint_warn / 10);

	// The "number" that the character is displayed as before the hit
	int old_num = (p_ptr->chp * 95) / (p_ptr->mhp*10); 
	int new_num; 

	if (old_num >= 7) old_num = 10;

	/* Paranoia */
	if (p_ptr->death) return;

	/* Hack -- player is secured inside a store/house */
	/* XXX make sure it doesn't "leak"! */
	if (p_ptr->store_num > -1) return;

	/* Disturb */
	disturb(Ind, 1, 0);

	/* Mega-Hack -- Apply "invulnerability" */
	if (p_ptr->invuln && (!bypass_invuln))
	{
		/* 1 in 2 chance to fully deflect the damage */
		if (magik(40))
		{
			msg_print(Ind, "The attack is fully deflected by the shield.");
			return;
		}

		/* Otherwise damage is reduced by the shield */
		damage = damage / 2;
	}

	/* Re allowed by evileye for power */
#if 1
	//        if (p_ptr->tim_manashield)
	if (p_ptr->tim_manashield && (!bypass_invuln))
	{
		if (p_ptr->csp > 0)
		{
			int taken = (damage);

			if (p_ptr->csp < taken)
			{
				damage -= taken - p_ptr->csp;
				p_ptr->csp = 0;
			}
			else
			{
				damage = 0;
				p_ptr->csp -= taken;
			}

			/* Display the spellpoints */
			p_ptr->redraw |= (PR_MANA);
		}
	}
#endif

	/* Hurt the player */
	p_ptr->chp -= damage;

	/* Update health bars */
	update_health(0 - Ind);

	/* Display the hitpoints */
	p_ptr->redraw |= (PR_HP);

	/* Figure out of if the player's "number" has changed */
	new_num = (p_ptr->chp * 95) / (p_ptr->mhp*10); 
	if (new_num >= 7) new_num = 10;

	/* If so then refresh everyone's view of this player */
	if (new_num != old_num)
		everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	/* Dead player */
	if (p_ptr->chp < 0)
	{
		int Ind2;

		/* Sound */
		sound(Ind, SOUND_DEATH);

		/* Hack -- Blond bond */
		Ind2 = find_player(p_ptr->blood_bond);

		if (Ind2 && !strcmp(Players[Ind2]->name, hit_from))
		{
			p_ptr->chp = p_ptr->mhp;
			Players[Ind2]->chp = Players[Ind2]->mhp;
			p_ptr->redraw |= (PR_HP);
			Players[Ind2]->redraw |= (PR_HP);

			msg_broadcast(Ind, format("%s won the blood bond against %s.", Players[Ind2]->name, p_ptr->name));

			p_ptr->blood_bond = Players[Ind2]->blood_bond = 0;
			remove_hostility(Ind, Players[Ind2]->name);
			remove_hostility(Ind2, p_ptr->name);

			target_set(Ind, 0);
			target_set(Ind2, 0);

			teleport_player(Ind, 400);

			return;
		}
		else{
			/* Only mention death if not BB! */
#if 0
			msg_print(Ind, "\377RYou die.");
			msg_print(Ind, NULL);
#endif	// 0
		}

		/* Note cause of death */
		/* To preserve the players original (pre-ghost) cause
		   of death, use died_from_list.  To preserve the original
		   depth, use died_from_depth. */

		(void)strcpy(p_ptr->died_from, hit_from);
		if (!p_ptr->ghost) 
		{	strcpy(p_ptr->died_from_list, hit_from);
			p_ptr->died_from_depth = getlevel(&p_ptr->wpos);
		}

		/* No longer a winner */
		p_ptr->total_winner = FALSE;

		/* Note death */
		p_ptr->death = TRUE;

		/* Dead */
		return;
	}

#if 0	// moved to client
	/* Hack -- hitpoint warning */
	if (warning && (p_ptr->chp <= warning))
	{
		/* Hack -- bell on first notice */
		/*if (alert_hitpoint && (old_chp > warning)) bell();*/

		/* Message */
		msg_print(Ind, "*** LOW HITPOINT WARNING! ***");
		msg_print(Ind, NULL);
	}
#endif	// 0
}


/* Decrease player's sanity. This is a copy of the function above. */
void take_sanity_hit(int Ind, int damage, cptr hit_from)
{
	player_type *p_ptr = Players[Ind];
#if 0
	int old_csane = p_ptr->csane;
    int warning = (p_ptr->msane * hitpoint_warn / 10);
#endif	// 0


	/* Paranoia */
	if (p_ptr->death) return;

	/* Disturb */
	disturb(Ind, 1, 0);


	/* Hurt the player */
        p_ptr->csane -= damage;

	/* Display the hitpoints */
        p_ptr->redraw |= (PR_SANITY);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	/* Dead player */
	if (p_ptr->csane < 0)
	{
		/* Sound */
		sound(Ind, SOUND_DEATH);

		/* Hack -- Note death */
		msg_print(Ind, "\377vYou turn into an unthinking vegetable.");
		msg_print(Ind, "\377RYou die.");
		msg_print(Ind, NULL);


		/* Note cause of death */
		/* To preserve the players original (pre-ghost) cause
		   of death, use died_from_list.  To preserve the original
		   depth, use died_from_depth. */

//		(void)strcpy(p_ptr->died_from, hit_from);
		(void)strcpy(p_ptr->died_from, "Insanity");
		if (!p_ptr->ghost) 
		{	strcpy(p_ptr->died_from_list, "Insanity");
			p_ptr->died_from_depth = getlevel(&p_ptr->wpos);
		}

		/* No longer a winner */
		p_ptr->total_winner = FALSE;

		/* Note death */
		p_ptr->death = TRUE;
		
		/* This way, player dies without becoming a ghost. */
		p_ptr->ghost = TRUE;

		/* Dead */
		return;
	}

	/* Insanity warning (better message needed!) */
	if (p_ptr->csane < p_ptr->msane / 8)
	{
		/* Message */
		msg_print(Ind, "\377rYou can hardly resist the temptation to cry out!");
		msg_print(Ind, NULL);
	}
	else if (p_ptr->csane < p_ptr->msane / 4)
	{
		/* Message */
		msg_print(Ind, "\377yYou feel insanity about to grasp your mind..");
		msg_print(Ind, NULL);
	}
	else if (p_ptr->csane < p_ptr->msane / 2)
	{
		/* Message */
		msg_print(Ind, "\377yYou feel insanity creep into your mind..");
		msg_print(Ind, NULL);
	}

#if 0
	/* Hitpoint warning */
	if (p_ptr->csane < warning)
	{
		/* Hack -- bell on first notice */
		if (alert_hitpoint && (old_csane > warning)) bell();

		sound(SOUND_WARN);		

		/* Message */
		msg_print(Ind, TERM_RED, "*** LOW SANITY WARNING! ***");
		msg_print(Ind, NULL);
	}
#endif	// 0
}

/* Decrease player's exp. This is another copy of the function above.
 * if mode is 'TRUE', it's permanent.
 * if fatal, player dies if runs out of exp.
 * 
 * if not permanent nor fatal, use lose_exp instead.
 * - Jir -
 */
void take_xp_hit(int Ind, int damage, cptr hit_from, bool mode, bool fatal)
{
	player_type *p_ptr = Players[Ind];

	/* Paranoia */
	if (p_ptr->death) return;

	/* Disturb */
//	disturb(Ind, 1, 0);

	/* Hurt the player */
	p_ptr->exp -= damage;
	if (mode) p_ptr->max_exp -= damage;;

	check_experience(Ind);

	/* Dead player */
	if (fatal && p_ptr->exp == 0)
	{
		/* Sound */
		sound(Ind, SOUND_DEATH);

		/* Hack -- Note death */
		msg_print(Ind, "\377RYou die.");
		msg_print(Ind, NULL);


		/* Note cause of death */
		/* To preserve the players original (pre-ghost) cause
		   of death, use died_from_list.  To preserve the original
		   depth, use died_from_depth. */
		
		(void)strcpy(p_ptr->died_from, hit_from);
		if (!p_ptr->ghost) 
		{	strcpy(p_ptr->died_from_list, hit_from);
			p_ptr->died_from_depth = getlevel(&p_ptr->wpos);
		}

		/* No longer a winner */
		p_ptr->total_winner = FALSE;

		/* Note death */
		p_ptr->death = TRUE;

		/* Dead */
		return;
	}
}





/*
 * Note that amulets, rods, and high-level spell books are immune
 * to "inventory damage" of any kind.  Also sling ammo and shovels.
 */


/*
 * Does a given class of objects (usually) hate acid?
 * Note that acid can either melt or corrode something.
 */
static bool hates_acid(object_type *o_ptr)
{
	/* Analyze the type */
	switch (o_ptr->tval)
	{
		/* Wearable items */
		case TV_ARROW:
		case TV_BOLT:
		case TV_BOW:
		case TV_BOOMERANG:
		case TV_SWORD:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		{
			return (TRUE);
		}

		/* Staffs/Scrolls are wood/paper */
		case TV_STAFF:
		case TV_SCROLL:
		case TV_PARCHEMENT:
		{
			return (TRUE);
		}

		/* Ouch */
		case TV_CHEST:
		{
			return (TRUE);
		}

		/* Junk is useless */
		case TV_SKELETON:
		case TV_BOTTLE:
		case TV_JUNK:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Does a given object (usually) hate electricity?
 */
static bool hates_elec(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_RING:
		case TV_WAND:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Does a given object (usually) hate fire?
 * Hafted/Polearm weapons have wooden shafts.
 * Arrows/Bows are mostly wooden.
 */
static bool hates_fire(object_type *o_ptr)
{
	/* Analyze the type */
	switch (o_ptr->tval)
	{
		/* Wearable */
		case TV_LITE:
		case TV_ARROW:
		case TV_BOW:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_BOOMERANG:
		{
			return (TRUE);
		}

		/* Chests */
		case TV_CHEST:
		{
			return (TRUE);
		}

		/* Staffs/Scrolls burn */
		case TV_STAFF:
		case TV_SCROLL:
		case TV_PARCHEMENT:
		{
			return (TRUE);
		}

		/* Potions evaporate */
		case TV_POTION:
		case TV_POTION2:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Does a given object (usually) hate cold?
 */
static bool hates_cold(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_POTION:
		case TV_POTION2:
		case TV_FLASK:
		case TV_BOTTLE:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Does a given object (usually) hate impact?
 */
static bool hates_impact(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_POTION:
		case TV_POTION2:
		case TV_FLASK:
		case TV_BOTTLE:
		case TV_EGG:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Does a given object (usually) hate water?
 */
static bool hates_water(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_POTION:		/* dilutes */
		case TV_POTION2:	/* dilutes */
		case TV_SCROLL:		/* fades */
		case TV_BOOK:
		{
			return (TRUE);
		}
	}
	return (FALSE);
}









/*
 * Melt something
 */
static int set_acid_destroy(object_type *o_ptr)
{
	    u32b f1, f2, f3, f4, f5, esp;
	if (!hates_acid(o_ptr)) return (FALSE);
			  /* Extract the flags */
			  object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
	if (f3 & TR3_IGNORE_ACID) return (FALSE);
	return (TRUE);
}


/*
 * Electrical damage
 */
static int set_elec_destroy(object_type *o_ptr)
{
	    u32b f1, f2, f3, f4, f5, esp;
	if (!hates_elec(o_ptr)) return (FALSE);
			  /* Extract the flags */
			  object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
	if (f3 & TR3_IGNORE_ELEC) return (FALSE);
	return (TRUE);
}


/*
 * Burn something
 */
static int set_fire_destroy(object_type *o_ptr)
{
	    u32b f1, f2, f3, f4, f5, esp;
	if (!hates_fire(o_ptr)) return (FALSE);
			  /* Extract the flags */
			  object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
	if (f3 & TR3_IGNORE_FIRE) return (FALSE);
	return (TRUE);
}


/*
 * Freeze things
 */
int set_cold_destroy(object_type *o_ptr)
{
	    u32b f1, f2, f3, f4, f5, esp;
	if (!hates_cold(o_ptr)) return (FALSE);
			  /* Extract the flags */
			  object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
	if (f3 & TR3_IGNORE_COLD) return (FALSE);
	return (TRUE);
}

/*
 * Crash things
 */
int set_impact_destroy(object_type *o_ptr)
{
	    u32b f1, f2, f3, f4, f5, esp;
	if (!hates_impact(o_ptr)) return (FALSE);
			  /* Extract the flags */
			  object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
	/* Hack -- borrow flag */
	if (f3 & TR3_IGNORE_COLD) return (FALSE);
	return (TRUE);
}


/*
 * Soak something
 */
int set_water_destroy(object_type *o_ptr)
{
	    u32b f1, f2, f3, f4, f5, esp;
	if (!hates_water(o_ptr)) return (FALSE);
			  /* Extract the flags */
			  object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
	if (f5 & TR5_IGNORE_WATER) return (FALSE);
	return (TRUE);
}


/*
 * Every things
 */
int set_all_destroy(object_type *o_ptr)
{
	if (artifact_p(o_ptr)) return (FALSE);
//	if (is_book(o_ptr) && o_ptr->sval >= SV_BOOK_MIN_GOOD) return (FALSE);
	if (is_book(o_ptr))
	{
	    u32b f1, f2, f3, f4, f5, esp;
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
		/* Hack^2 -- use this as a sign of being 'high books' */
		if (f3 & TR3_IGNORE_ELEC) return (FALSE);
	}
	return (TRUE);
}




/*
 * This seems like a pretty standard "typedef"
 */
//typedef int (*inven_func)(object_type *);

/*
 * Destroys a type of item on a given percent chance
 * Note that missiles are no longer necessarily all destroyed
 * Destruction taken from "melee.c" code for "stealing".
 * Returns number of items destroyed.
 */
int inven_damage(int Ind, inven_func typ, int perc)
{
	player_type *p_ptr = Players[Ind];

	int		i, j, k, amt;

	object_type	*o_ptr;

	char	o_name[160];


	/* Count the casualties */
	k = 0;

	/* Scan through the slots backwards */
//	for (i = 0; i < INVEN_PACK; i++)
	for (i = 0; i < INVEN_TOTAL; i++)	/* Let's see what will happen.. */
	{
		/* Hack -- equipments are harder to be harmed */
		if (i >= INVEN_PACK && i != INVEN_AMMO && !magik(HARM_EQUIP_CHANCE))
			continue;

		/* Get the item in that slot */
		o_ptr = &p_ptr->inventory[i];

		/* Hack -- for now, skip artifacts */
		if (artifact_p(o_ptr)) continue;

		/* Give this item slot a shot at death */
		if ((*typ)(o_ptr))
		{
			/* Count the casualties */
			for (amt = j = 0; j < o_ptr->number; ++j)
			{
				if (rand_int(100) < perc) amt++;
			}

			/* Some casualities */
			if (amt)
			{
				/* Get a description */
				object_desc(Ind, o_name, o_ptr, FALSE, 3);

				/* Message */
				msg_format(Ind, "%sour %s (%c) %s destroyed!",
				           ((o_ptr->number > 1) ?
				            ((amt == o_ptr->number) ? "All of y" :
				             (amt > 1 ? "Some of y" : "One of y")) : "Y"),
				           o_name, index_to_label(i),
				           ((amt > 1) ? "were" : "was"));

				/* Potions smash open */
				if (k_info[o_ptr->k_idx].tval == TV_POTION &&
					typ != set_water_destroy)	/* MEGAHACK */
//					&& typ != set_cold_destroy)
				{
//					(void)potion_smash_effect(0, &p_ptr->wpos, p_ptr->py, p_ptr->px, o_ptr->sval);
					(void)potion_smash_effect(PROJECTOR_POTION, &p_ptr->wpos, p_ptr->py, p_ptr->px, o_ptr->sval);
				}

				/* Destroy "amt" items */
				inven_item_increase(Ind, i, -amt);
				inven_item_optimize(Ind, i);

				/* Count the casualties */
				k += amt;
			}
		}
	}

	/* Return the casualty count */
	return (k);
}




/*
 * Acid has hit the player, attempt to affect some armor.
 *
 * Note that the "base armor" of an object never changes.
 *
 * If any armor is damaged (or resists), the player takes less damage.
 */
/* Hack -- 'water' means it was water damage (and not acid).
 * TODO: add IGNORE_WATER flags */
int minus_ac(int Ind, int water)
{
	player_type *p_ptr = Players[Ind];

	object_type		*o_ptr = NULL;

	    u32b f1, f2, f3, f4, f5, esp;

	char		o_name[160];


	/* Pick a (possibly empty) inventory slot */
	switch (randint(6))
	{
		case 1: o_ptr = &p_ptr->inventory[INVEN_BODY]; break;
		case 2: o_ptr = &p_ptr->inventory[INVEN_ARM]; break;
		case 3: if (water) return (FALSE);
				o_ptr = &p_ptr->inventory[INVEN_OUTER]; break;
		case 4: o_ptr = &p_ptr->inventory[INVEN_HANDS]; break;
		case 5: o_ptr = &p_ptr->inventory[INVEN_HEAD]; break;
		case 6: o_ptr = &p_ptr->inventory[INVEN_FEET]; break;
	}

	/* Nothing to damage */
	if (!o_ptr->k_idx) return (FALSE);

	/* No damage left to be done */
	if (o_ptr->ac + o_ptr->to_a <= 0) return (FALSE);

	/* Hack -- Leather gears are immune to water (need IGNORE_WATER!) */
	/* if (water && (strstr((k_name + k_info[o_ptr->k_idx].name),"Leather") ||
				strstr((k_name + k_info[o_ptr->k_idx].name),"Robe")))
	now water flag is used, look below */
		return(FALSE);

	/* Describe */
	object_desc(Ind, o_name, o_ptr, FALSE, 0);

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	if (water && (f5 & TR5_IGNORE_WATER))
		return(FALSE);

	/* Object resists */
	if (f3 & TR3_IGNORE_ACID)
	{
		msg_format(Ind, "Your %s is unaffected!", o_name);

		return (TRUE);
	}

	/* Message */
	msg_format(Ind, "Your %s is damaged!", o_name);

	/* Damage the item */
	o_ptr->to_a--;

	/* Calculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Item was damaged */
	return (TRUE);
}


/*
 * Hurt the player with Acid
 */
void acid_dam(int Ind, int dam, cptr kb_str)
{
	player_type *p_ptr = Players[Ind];

        int inv;

        dam -= p_ptr->reduc_acid * dam / 100;

        inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	/* Total Immunity */
	if (p_ptr->immune_acid || (dam <= 0)) return;

	/* Resist the damage */
	if (p_ptr->resist_acid) dam = (dam + 2) / 3;
	if (p_ptr->oppose_acid) dam = (dam + 2) / 3;

	if ((!(p_ptr->oppose_acid || p_ptr->resist_acid)) &&
	    randint(HURT_CHANCE)==1)
		(void) do_dec_stat(Ind, A_CHR, DAM_STAT_TYPE(inv));

	/* If any armor gets hit, defend the player */
	if (minus_ac(Ind, 0)) dam = (dam + 1) / 2;

	/* Take damage */
	take_hit(Ind, dam, kb_str);

	/* Inventory damage */
	if (!(p_ptr->oppose_acid && p_ptr->resist_acid))
		inven_damage(Ind, set_acid_destroy, inv);
}


/*
 * Hurt the player with electricity
 */
void elec_dam(int Ind, int dam, cptr kb_str)
{
	player_type *p_ptr = Players[Ind];

        int inv;

        dam -= p_ptr->reduc_elec * dam / 100;

        inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	/* Total immunity */
	if (p_ptr->immune_elec || (dam <= 0)) return;

	/* Resist the damage */
	if (p_ptr->oppose_elec) dam = (dam + 2) / 3;
	if (p_ptr->resist_elec) dam = (dam + 2) / 3;

	if ((!(p_ptr->oppose_elec || p_ptr->resist_elec)) &&
	    randint(HURT_CHANCE)==1)
		(void) do_dec_stat(Ind, A_DEX, DAM_STAT_TYPE(inv));

	/* Take damage */
	take_hit(Ind, dam, kb_str);

	/* Inventory damage */
	if (!(p_ptr->oppose_elec && p_ptr->resist_elec))
		inven_damage(Ind, set_elec_destroy, inv);
}




/*
 * Hurt the player with Fire
 */
void fire_dam(int Ind, int dam, cptr kb_str)
{
	player_type *p_ptr = Players[Ind];

        int inv;

        dam -= p_ptr->reduc_fire * dam / 100;

        inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	/* Totally immune */
	if (p_ptr->immune_fire || (dam <= 0)) return;

	/* Resist the damage */
	if (p_ptr->sensible_fire) dam = (dam + 2) * 2;
	if (p_ptr->resist_fire) dam = (dam + 2) / 3;
	if (p_ptr->oppose_fire) dam = (dam + 2) / 3;

	if ((!(p_ptr->oppose_fire || p_ptr->resist_fire)) &&
	    randint(HURT_CHANCE)==1)
		(void) do_dec_stat(Ind, A_STR, DAM_STAT_TYPE(inv));

	/* Take damage */
	take_hit(Ind, dam, kb_str);

	/* Inventory damage */
	if (!(p_ptr->resist_fire && p_ptr->oppose_fire))
		inven_damage(Ind, set_fire_destroy, inv);
}


/*
 * Hurt the player with Cold
 */
void cold_dam(int Ind, int dam, cptr kb_str)
{
	player_type *p_ptr = Players[Ind];

        int inv;

        dam -= p_ptr->reduc_cold * dam / 100;

        inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	/* Total immunity */
	if (p_ptr->immune_cold || (dam <= 0)) return;

	/* Resist the damage */
	if (p_ptr->resist_cold) dam = (dam + 2) / 3;
	if (p_ptr->oppose_cold) dam = (dam + 2) / 3;

	if ((!(p_ptr->oppose_cold || p_ptr->resist_cold)) &&
	    randint(HURT_CHANCE)==1)
		(void) do_dec_stat(Ind, A_STR, DAM_STAT_TYPE(inv));

	/* Take damage */
	take_hit(Ind, dam, kb_str);

	/* Inventory damage */
	if (!(p_ptr->resist_cold && p_ptr->oppose_cold))
		inven_damage(Ind, set_cold_destroy, inv);
}





/*
 * Increases a stat by one randomized level		-RAK-
 *
 * Note that this function (used by stat potions) now restores
 * the stat BEFORE increasing it.
 */
bool inc_stat(int Ind, int stat)
{
	player_type *p_ptr = Players[Ind];

	int value, gain;

	/* Then augment the current/max stat */
	value = p_ptr->stat_cur[stat];

	/* Cannot go above 18/100 */
	if (value < 18+100)
	{
		/* Gain one (sometimes two) points */
		if (value < 18)
		{
			gain = ((rand_int(100) < 75) ? 1 : 2);
			value += gain;
		}

		/* Gain 1/6 to 1/3 of distance to 18/100 */
		else if (value < 18+98)
		{
			/* Approximate gain value */
			gain = (((18+100) - value) / 2 + 3) / 2;

			/* Paranoia */
			if (gain < 1) gain = 1;

			/* Apply the bonus */
			value += randint(gain) + gain / 2;

			/* Maximal value */
			if (value > 18+99) value = 18 + 99;
		}

		/* Gain one point at a time */
		else
		{
			value++;
		}

		/* Save the new value */
		p_ptr->stat_cur[stat] = value;

		/* Bring up the maximum too */
		if (value > p_ptr->stat_max[stat])
		{
			p_ptr->stat_max[stat] = value;
		}

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Success */
		return (TRUE);
	}

	/* Nothing to gain */
	return (FALSE);
}



/*
 * Decreases a stat by an amount indended to vary from 0 to 100 percent.
 *
 * Amount could be a little higher in extreme cases to mangle very high
 * stats from massive assaults.  -CWS
 *
 * Note that "permanent" means that the *given* amount is permanent,
 * not that the new value becomes permanent.  This may not work exactly
 * as expected, due to "weirdness" in the algorithm, but in general,
 * if your stat is already drained, the "max" value will not drop all
 * the way down to the "cur" value.
 */
bool dec_stat(int Ind, int stat, int amount, int mode)
{
	player_type *p_ptr = Players[Ind];

	int cur, max, loss = 0, same, res = FALSE;


	/* Acquire current value */
	cur = p_ptr->stat_cur[stat];
	max = p_ptr->stat_max[stat];

	/* Note when the values are identical */
	same = (cur == max);

	/* Damage "current" value */
	if (cur > 3)
	{
		/* Handle "low" values */
		if (cur <= 18)
		{
			if (amount > 90) loss++;
			if (amount > 50) loss++;
			if (amount > 20) loss++;
			loss++;
			cur -= loss;
#if 0
			if (amount > 90) cur--;
			if (amount > 50) cur--;
			if (amount > 20) cur--;
			cur--;
#endif	// 0
		}

		/* Handle "high" values */
		else
		{
			/* Hack -- Decrement by a random amount between one-quarter */
			/* and one-half of the stat bonus times the percentage, with a */
			/* minimum damage of half the percentage. -CWS */
			loss = (((cur-18) / 2 + 1) / 2 + 1);

			/* Paranoia */
			if (loss < 1) loss = 1;

			/* Randomize the loss */
			loss = ((randint(loss) + loss) * amount) / 100;

			/* Maximal loss */
			if (loss < amount/2) loss = amount/2;

			/* Lose some points */
			cur = cur - loss;

			/* Hack -- Only reduce stat to 17 sometimes */
			if (cur < 18) cur = (amount <= 20) ? 18 : 17;
		}

		/* Prevent illegal values */
		if (cur < 3) cur = 3;

		/* Something happened */
		if (cur != p_ptr->stat_cur[stat]) res = TRUE;
	}

	/* Damage "max" value */
	if ((mode==STAT_DEC_PERMANENT) && (max > 3))
	{
		/* Handle "low" values */
		if (max <= 18)
		{
			if (amount > 90) max--;
			if (amount > 50) max--;
			if (amount > 20) max--;
			max--;
		}

		/* Handle "high" values */
		else
		{
			/* Hack -- Decrement by a random amount between one-quarter */
			/* and one-half of the stat bonus times the percentage, with a */
			/* minimum damage of half the percentage. -CWS */
			loss = (((max-18) / 2 + 1) / 2 + 1);
			loss = ((randint(loss) + loss) * amount) / 100;
			if (loss < amount/2) loss = amount/2;

			/* Lose some points */
			max = max - loss;

			/* Hack -- Only reduce stat to 17 sometimes */
			if (max < 18) max = (amount <= 20) ? 18 : 17;
		}

		/* Hack -- keep it clean */
		if (same || (max < cur)) max = cur;

		/* Something happened */
		if (max != p_ptr->stat_max[stat]) res = TRUE;
	}

	/* Apply changes */
	if (res)
	{
		/* Actually set the stat to its new value. */
		p_ptr->stat_cur[stat] = cur;
		p_ptr->stat_max[stat] = max;

		if (mode==STAT_DEC_TEMPORARY)
		{
			u16b dectime;

			/* a little crude, perhaps */
			dectime = rand_int(getlevel(&p_ptr->wpos)*50) + 50;
			
			/* prevent overflow, stat_cnt = u16b */
			/* or add another temporary drain... */
			if ( ((p_ptr->stat_cnt[stat]+dectime)<p_ptr->stat_cnt[stat]) ||
			    (p_ptr->stat_los[stat]>0) )

			{
				p_ptr->stat_cnt[stat] += dectime;
				p_ptr->stat_los[stat] += loss;
			}
			else
			{
				p_ptr->stat_cnt[stat] = dectime;
				p_ptr->stat_los[stat] = loss;
			}
		}

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);
	}

	/* Done */
	return (res);
}


/*
 * Restore a stat.  Return TRUE only if this actually makes a difference.
 */
bool res_stat(int Ind, int stat)
{
	player_type *p_ptr = Players[Ind];

	/* temporary drain is gone */
	p_ptr->stat_los[stat] = 0;
	p_ptr->stat_cnt[stat] = 0;

	/* Restore if needed */
	if (p_ptr->stat_cur[stat] != p_ptr->stat_max[stat])
	{
		/* Restore */
		p_ptr->stat_cur[stat] = p_ptr->stat_max[stat];

		/* Recalculate bonuses */
//		p_ptr->update |= (PU_BONUS);
		p_ptr->update |= (PU_BONUS | PU_MANA | PU_HP | PU_SANITY);

		/* Success */
		return (TRUE);
	}

	/* Nothing to restore */
	return (FALSE);
}




/*
 * Apply disenchantment to the player's stuff
 *
 * XXX XXX XXX This function is also called from the "melee" code
 *
 *
 * If "mode is set to 0 then a random slot will be used, if not the "mode"
 * slot will be used.
 *
 * Return "TRUE" if the player notices anything
 */
bool apply_disenchant(int Ind, int mode)
{
	player_type *p_ptr = Players[Ind];

	int			t = mode;

	object_type		*o_ptr;

	char		o_name[160];


	/* Unused */
//	mode = mode;

	if(mode < 1 || mode > 8) return (FALSE);

	/* Pick a random slot */
	if(!mode)
	{
		switch (randint(8))
		{
			case 1: t = INVEN_WIELD; break;
			case 2: t = INVEN_BOW; break;
			case 3: t = INVEN_BODY; break;
			case 4: t = INVEN_OUTER; break;
			case 5: t = INVEN_ARM; break;
			case 6: t = INVEN_HEAD; break;
			case 7: t = INVEN_HANDS; break;
			case 8: t = INVEN_FEET; break;
		}
	}

	/* Get the item */
	o_ptr = &p_ptr->inventory[t];

	/* No item, nothing happens */
	if (!o_ptr->k_idx) return (FALSE);


	/* Nothing to disenchant */
	if ((o_ptr->to_h <= 0) && (o_ptr->to_d <= 0) && (o_ptr->to_a <= 0))
	{
		/* Nothing to notice */
		return (FALSE);
	}


	/* Describe the object */
	object_desc(Ind, o_name, o_ptr, FALSE, 0);


	/* Artifacts have 60% chance to resist */
	if ((artifact_p(o_ptr) && (rand_int(100) < 60)) || (true_artifact_p(o_ptr) && (rand_int(100) < 90)))
	{
		/* Message */
		msg_format(Ind, "Your %s (%c) resist%s disenchantment!",
		           o_name, index_to_label(t),
		           ((o_ptr->number != 1) ? "" : "s"));

		/* Notice */
		return (TRUE);
	}


	/* Disenchant tohit */
	if (o_ptr->to_h > 0) o_ptr->to_h--;
	if ((o_ptr->to_h > 5) && (rand_int(100) < 20)) o_ptr->to_h--;

	/* Disenchant todam */
	if (o_ptr->to_d > 0) o_ptr->to_d--;
	if ((o_ptr->to_d > 5) && (rand_int(100) < 20)) o_ptr->to_d--;

	/* Disenchant toac */
	if (o_ptr->to_a > 0) o_ptr->to_a--;
	if ((o_ptr->to_a > 5) && (rand_int(100) < 20)) o_ptr->to_a--;

	/* Message */
	msg_format(Ind, "Your %s (%c) %s disenchanted!",
	           o_name, index_to_label(t),
	           ((o_ptr->number != 1) ? "were" : "was"));

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Notice */
	return (TRUE);
}


/*
 * Apply Nexus
 */
static void apply_nexus(int Ind, monster_type *m_ptr)
{
	player_type *p_ptr = Players[Ind];

	int max1, cur1, max2, cur2, ii, jj;

	switch (randint(8))
	{
		case 4: case 5:
		{
			if (!m_ptr) break;

			if (p_ptr->anti_tele)
			{
				msg_print(Ind, "You resist the effects!");
				break;
			}

			teleport_player_to(Ind, m_ptr->fy, m_ptr->fx);
			break;
		}

		case 1: case 2: case 3:
		{
			if (p_ptr->anti_tele)
			{
				msg_print(Ind, "You resist the effects!");
				break;
			}

			teleport_player(Ind, 200);
			break;
		}

		case 6:
		{
			if (rand_int(100) < p_ptr->skill_sav || p_ptr->anti_tele)
			{
				msg_print(Ind, "You resist the effects!");
				break;
			}

			/* Teleport Level */
			teleport_player_level(Ind);
			break;
		}

		case 7:
		{
			if (rand_int(100) < p_ptr->skill_sav)
			{
				msg_print(Ind, "You resist the effects!");
				break;
			}

			msg_print(Ind, "Your body starts to scramble...");

			/* Pick a pair of stats */
			ii = rand_int(6);
			for (jj = ii; jj == ii; jj = rand_int(6)) /* loop */;

			max1 = p_ptr->stat_max[ii];
			cur1 = p_ptr->stat_cur[ii];
			max2 = p_ptr->stat_max[jj];
			cur2 = p_ptr->stat_cur[jj];

			p_ptr->stat_max[ii] = max2;
			p_ptr->stat_cur[ii] = cur2;
			p_ptr->stat_max[jj] = max1;
			p_ptr->stat_cur[jj] = cur1;

			p_ptr->update |= (PU_BONUS);

			break;
		}

		case 8:
		{
			if (check_st_anchor(&p_ptr->wpos, p_ptr->py, p_ptr->px)) break;

			if (rand_int(100) < p_ptr->skill_sav || p_ptr->anti_tele)
			{
				msg_print(Ind, "You resist the effects!");
				break;
			}

			msg_print(Ind, "Your backpack starts to scramble...");

			ii = 10 + m_ptr->level / 3;
			if (ii > 50) ii = 50;

			do_player_scatter_items(Ind, 5, ii);

			break;
		}
	}
}


/*
 * Apply Polymorph
 */
 
/* Ho Ho Ho... I really want this to have a chance of turning people into
   a fruit bat,  but something tells me I should put that off until the next version...
   
   I CANT WAIT. DOING FRUIT BAT RIGHT NOW!!! 
   -APD-
*/   
static void apply_morph(int Ind, int power, char * killer)
{
	player_type *p_ptr = Players[Ind];

	int max1, cur1, max2, cur2, ii, jj;

	switch (randint(2))
	{
		/*case 1: msg_print(Ind, "You resist the effects!"); break;*/
		case 1:
		{
			if (rand_int(40 + power*2) < p_ptr->skill_sav)
			{
				msg_print(Ind, "You resist the effects!");
				break;
			}

			msg_print(Ind, "Your body starts to scramble...");

			/* Pick a pair of stats */
			ii = rand_int(6);
			for (jj = ii; jj == ii; jj = rand_int(6)) /* loop */;

			max1 = p_ptr->stat_max[ii];
			cur1 = p_ptr->stat_cur[ii];
			max2 = p_ptr->stat_max[jj];
			cur2 = p_ptr->stat_cur[jj];

			p_ptr->stat_max[ii] = max2;
			p_ptr->stat_cur[ii] = cur2;
			p_ptr->stat_max[jj] = max1;
			p_ptr->stat_cur[jj] = cur1;

			p_ptr->update |= (PU_BONUS);

			break;
		}
		case 2:
		{
			if (!p_ptr->fruit_bat)
			{
		
				if (rand_int(10 + power * 4) < p_ptr->skill_sav)
				{
					msg_print(Ind, "You resist the effects!");
				}
				else
				{
					/* FRUIT BAT!!!!!! */
				
					msg_print(Ind, "You have been turned into a fruit bat!");				
					strcpy(p_ptr->died_from,killer);
					p_ptr->fruit_bat = -1;
					player_death(Ind);
				}	
			}
			else				
			{	/* no saving throw for being restored..... */
				msg_print(Ind, "You have been restored!");
				p_ptr->fruit_bat = 0;
				p_ptr->update |= (PU_BONUS | PU_HP);
			}
								
		}
	}
}





/*
 * Hack -- track "affected" monsters
 */
static int project_m_n;
static int project_m_x;
static int project_m_y;



/*
 * We are called from "project()" to "damage" terrain features
 *
 * We are called both for "beam" effects and "ball" effects.
 *
 * The "r" parameter is the "distance from ground zero".
 *
 * Note that we determine if the player can "see" anything that happens
 * by taking into account: blindness, line-of-sight, and illumination.
 *
 * We return "TRUE" if the effect of the projection is "obvious".
 *
 * XXX XXX XXX We also "see" grids which are "memorized", probably a hack
 *
 * XXX XXX XXX Perhaps we should affect doors?
 */
static bool project_f(int Ind, int who, int r, struct worldpos *wpos, int y, int x, int dam, int typ)
{
	bool	obvious = FALSE;

//	bool quiet = ((Ind <= 0) ? TRUE : FALSE);
	bool quiet = ((Ind <= 0 || who <= PROJECTOR_UNUSUAL) ? TRUE : FALSE);
	int		div;

	player_type *p_ptr = (quiet ? NULL : Players[Ind]);

	byte *w_ptr = (quiet ? NULL : &p_ptr->cave_flag[y][x]);
	cave_type *c_ptr;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);
	c_ptr=&zcave[y][x];


	/* Extract radius */
	div = r + 1;

	/* Decrease damage */
	dam = dam / div;


	/* XXX XXX */
	who = who ? who : 0;


	/* Analyze the type */
	switch (typ)
	{
		/* Ignore most effects */
		case GF_ACID:
		case GF_ELEC:
		case GF_COLD:
		case GF_PLASMA:
		case GF_METEOR:
		case GF_ICE:
		case GF_SHARDS:
		case GF_FORCE:
		case GF_SOUND:
		case GF_MANA:
		{
			break;
		}

		case GF_STONE_WALL:
		{
			/* Require a "naked" floor grid */
			if (!cave_naked_bold(zcave, y, x)) break;

               		/* Beware of the houses in town */
               		if ((wpos->wz==0) && (zcave[y][x].info & CAVE_ICKY)) break;

                        /* Place a wall */
			c_ptr->feat = FEAT_WALL_EXTRA;
					
			/* Notice */
			if (!quiet) note_spot(Ind, y, x);

			/* Redraw */
			everyone_lite_spot(wpos, y, x);
			break;
		}

		/* Burn trees and grass */
		case GF_FIRE:
		/*
		case GF_METEOR:
		case GF_PLASMA:
		*/
		case GF_HOLY_ORB:
		case GF_HOLY_FIRE:
//		case GF_HELL_FIRE:
		{
			/* Destroy trees */
			if (c_ptr->feat == FEAT_TREES)
			{
				/* Hack -- special message */
				if (!quiet && player_can_see_bold(Ind, y, x))
				{
					msg_print(Ind, "The tree burns to the ground!");
					/* Notice */
					note_spot(Ind, y, x);
					obvious = TRUE;
				}

				/* Destroy the tree */
				c_ptr->feat = FEAT_DIRT;

				/* Redraw */
				everyone_lite_spot(wpos, y, x);
			}

			/* Burn grass */
			if (c_ptr->feat == FEAT_GRASS)
			{
				/* Destroy the grass */
				c_ptr->feat = FEAT_GRASS;
			}

			break;
		}

		/* Destroy Traps (and Locks) */
		case GF_KILL_TRAP:
		{
			struct c_special *cs_ptr;
			/* Destroy invisible traps */
//			if (c_ptr->feat == FEAT_INVIS)
			if((cs_ptr=GetCS(c_ptr, CS_TRAPS))){
				/* Hack -- special message */
				if (!quiet && player_can_see_bold(Ind, y, x))
				{
					msg_print(Ind, "There is a bright flash of light!");
					obvious = TRUE;
				}

				/* Destroy the trap */
//				c_ptr->feat = FEAT_FLOOR;
				cs_erase(c_ptr, cs_ptr);

				if (!quiet)
				{
					/* Notice */
					note_spot(Ind, y, x);

					/* Redraw */
					everyone_lite_spot(wpos, y, x);
				}
			}
#if 0
			/* Destroy visible traps */
			if ((c_ptr->feat >= FEAT_TRAP_HEAD) &&
			    (c_ptr->feat <= FEAT_TRAP_TAIL))
			{
				/* Hack -- special message */
				if (!quiet && (*w_ptr & CAVE_MARK))
				{
					msg_print(Ind, "There is a bright flash of light!");
					obvious = TRUE;
				}

				/* Destroy the trap */
				c_ptr->feat = FEAT_FLOOR;

				if (!quiet)
				{
					/* Notice */
					note_spot(Ind, y, x);

					/* Redraw */
					everyone_lite_spot(wpos, y, x);
				}
			}
#endif	// 0

			/* Secret / Locked doors are found and unlocked */
			else if ((c_ptr->feat == FEAT_SECRET) ||
			         ((c_ptr->feat >= FEAT_DOOR_HEAD + 0x01) &&
			          (c_ptr->feat <= FEAT_DOOR_HEAD + 0x07)))
			{
				/* Notice */
				if (!quiet && (*w_ptr & CAVE_MARK))
				{
					msg_print(Ind, "Click!");
					obvious = TRUE;
				}

				/* Unlock the door */
				c_ptr->feat = FEAT_DOOR_HEAD + 0x00;

				if (!quiet)
				{
					/* Notice */
					note_spot(Ind, y, x);

					/* Redraw */
					everyone_lite_spot(wpos, y, x);
				}
			}

			break;
		}

		/* Destroy Doors (and traps) */
		case GF_KILL_DOOR:
		{
			struct c_special *cs_ptr;
			/* Destroy invisible traps */
//			if (c_ptr->feat == FEAT_INVIS)
			if((cs_ptr=GetCS(c_ptr, CS_TRAPS))){
				/* Hack -- special message */
				if (!quiet && player_can_see_bold(Ind, y, x))
				{
					msg_print(Ind, "There is a bright flash of light!");
					obvious = TRUE;
				}

				/* Destroy the feature */
//				c_ptr->feat = FEAT_FLOOR;
				cs_erase(c_ptr, cs_ptr);

				/* Forget the wall */
				everyone_forget_spot(wpos, y, x);

				if (!quiet)
				{
					/* Notice */
					note_spot(Ind, y, x);

					/* Redraw */
					everyone_lite_spot(wpos, y, x);
				}
			}

			/* Destroy all visible traps and open doors */
			if ((c_ptr->feat == FEAT_OPEN) ||
			    (c_ptr->feat == FEAT_BROKEN)) 
			{
				/* Hack -- special message */
				if (!quiet && (*w_ptr & CAVE_MARK))
				{
					msg_print(Ind, "There is a bright flash of light!");
					obvious = TRUE;
				}

				/* Destroy the feature */
				c_ptr->feat = FEAT_FLOOR;

				/* Forget the wall */
				everyone_forget_spot(wpos, y, x);

				if (!quiet)
				{
					/* Notice */
					note_spot(Ind, y, x);

					/* Redraw */
					everyone_lite_spot(wpos, y, x);
				}
			}

			/* Destroy all closed doors */
			if ((c_ptr->feat >= FEAT_DOOR_HEAD) &&
			    (c_ptr->feat <= FEAT_DOOR_TAIL))
			{
				/* Hack -- special message */
				if (!quiet && (*w_ptr & CAVE_MARK))
				{
					msg_print(Ind, "There is a bright flash of light!");
					obvious = TRUE;
				}

				/* Destroy the feature */
				c_ptr->feat = FEAT_FLOOR;

				/* Forget the wall */
				everyone_forget_spot(wpos, y, x);

				if (!quiet)
				{
					/* Notice */
					note_spot(Ind, y, x);

					/* Redraw */
					everyone_lite_spot(wpos, y, x);

					/* Update some things */
					p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);
				}
			}

			break;
		}

		/* Destroy walls (and doors) */
		case GF_KILL_WALL:
		{
			/* Non-walls (etc) */
			if (cave_floor_bold(zcave, y, x)) break;

			/* Permanent walls */
			if ( (c_ptr->feat >= FEAT_PERM_EXTRA) || (c_ptr->feat == FEAT_PERM_CLEAR) ) break;

			/* Granite */
			if (c_ptr->feat >= FEAT_WALL_EXTRA)
			{
				byte feat;
				/* Message */
				if (!quiet && (*w_ptr & CAVE_MARK))
				{
					msg_print(Ind, "The wall turns into mud!");
					obvious = TRUE;
				}

				/* Destroy the wall */
				feat = twall_erosion(wpos, y, x);
				c_ptr->feat = (feat == FEAT_FLOOR) ? FEAT_DIRT : feat;
			}

			/* Quartz / Magma with treasure */
			else if (c_ptr->feat >= FEAT_MAGMA_H)
			{
				/* Message */
				if (!quiet && (*w_ptr & CAVE_MARK))
				{
					msg_print(Ind, "The vein turns into mud!");
					msg_print(Ind, "You have found something!");
					obvious = TRUE;
				}

				/* Destroy the wall */
				c_ptr->feat = FEAT_DIRT;

				/* Place some gold */
				place_gold(wpos, y, x);
			}

			/* Quartz / Magma */
			else if (c_ptr->feat >= FEAT_MAGMA)
			{
				/* Message */
				if (!quiet && (*w_ptr & CAVE_MARK))
				{
					msg_print(Ind, "The vein turns into mud!");
					obvious = TRUE;
				}

				/* Destroy the wall */
				c_ptr->feat = FEAT_DIRT;
			}

			/* Rubble */
			else if (c_ptr->feat == FEAT_RUBBLE)
			{
				/* Message */
				if (!quiet && (*w_ptr & CAVE_MARK))
				{
					msg_print(Ind, "The rubble turns into mud!");
					obvious = TRUE;
				}

				/* Destroy the rubble */
				c_ptr->feat = FEAT_DIRT;

				/* Hack -- place an object */
				if (rand_int(100) < 10)
				{
					/* Found something */
					if (!quiet && player_can_see_bold(Ind, y, x))
					{
						msg_print(Ind, "There was something buried in the rubble!");
						obvious = TRUE;
					}

					/* Place object */
					place_object(wpos, y, x, FALSE, FALSE, default_obj_theme);
				}
			}

			/* House doors are immune */
			else if (c_ptr->feat == FEAT_HOME)
			{
				/* Message */
				if (!quiet && (*w_ptr & CAVE_MARK))
				{
					msg_print(Ind, "The door resists.");
					obvious = TRUE;
				}
			}

			/* Destroy doors (and secret doors) */
			else if (c_ptr->feat >= FEAT_DOOR_HEAD)
			{
				/* Hack -- special message */
				if (!quiet && (*w_ptr & CAVE_MARK))
				{
					msg_print(Ind, "The door turns into mud!");
					obvious = TRUE;
				}

				/* Destroy the feature */
				c_ptr->feat = FEAT_DIRT;
			}

			/* Forget the wall */
			everyone_forget_spot(wpos, y, x);
	
			if (!quiet)
			{
				/* Notice */
				note_spot(Ind, y, x);

				/* Redraw */
				everyone_lite_spot(wpos, y, x);

				/* Update some things */
				p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS);
			}

			break;
		}

		/* Make doors */
		case GF_MAKE_DOOR:
		{
			/* Require a "naked" floor grid */
			if (!cave_naked_bold(zcave, y, x)) break;

			/* Create a closed door */
			c_ptr->feat = FEAT_DOOR_HEAD + 0x00;

			if (!quiet)
			{
				/* Notice */
				note_spot(Ind, y, x);

				/* Redraw */
				everyone_lite_spot(wpos, y, x);

				/* Observe */
				if (*w_ptr & CAVE_MARK) obvious = TRUE;

				/* Update some things */
				p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);
			}

			break;
		}

		/* Make traps */
		case GF_MAKE_TRAP:
		{
			/* Require a "naked" floor grid */
			if ((zcave[y][x].feat!=FEAT_MORE && zcave[y][x].feat!=FEAT_LESS) && cave_perma_bold(zcave, y, x)) break;

			/* Place a trap */
			place_trap(wpos, y, x, dam);

			if (!quiet)
			{
				/* Notice */
				note_spot(Ind, y, x);

				/* Redraw */
				everyone_lite_spot(wpos, y, x);
			}

			break;
		}

		/* Lite up the grid */
		case GF_LITE_WEAK:
		case GF_LITE:
		{
			/* Turn on the light */
			c_ptr->info |= CAVE_GLOW;

			if (!quiet)
			{

				/* Notice */
				note_spot_depth(wpos, y, x);
				/* Redraw */
				everyone_lite_spot(wpos, y, x);

				/* Observe */
				if (player_can_see_bold(Ind, y, x)) obvious = TRUE;
			}

			/* Mega-Hack -- Update the monster in the affected grid */
			/* This allows "spear of light" (etc) to work "correctly" */
			if (c_ptr->m_idx > 0) update_mon(c_ptr->m_idx, FALSE);

			break;
		}

		/* Darken the grid */
		case GF_DARK_WEAK:
		case GF_DARK:
		{
			/* Notice */
			if (!quiet && player_can_see_bold(Ind, y, x)) obvious = TRUE;

			/* Turn off the light. */
			c_ptr->info &= ~CAVE_GLOW;

			/* Hack -- Forget "boring" grids */
//			if (c_ptr->feat <= FEAT_INVIS)
			if (cave_plain_floor_grid(c_ptr))
			{
				/* Forget the wall */
				everyone_forget_spot(wpos, y, x);

				if (!quiet)
				{
					/* Notice */
					note_spot(Ind, y, x);
				}
			}

			if (!quiet)
			{
				/* Redraw */
				everyone_lite_spot(wpos, y, x);
			}

			/* Mega-Hack -- Update the monster in the affected grid */
			/* This allows "spear of light" (etc) to work "correctly" */
			if (c_ptr->m_idx > 0) update_mon(c_ptr->m_idx, FALSE);

			/* All done */
			break;
		}

		/* from PernA	- Jir - */
#if 0
		case GF_MAKE_GLYPH:
		{
			/* Require a "naked" floor grid */
                        if (!cave_clean_bold(y, x)) break;

//                        if((f_info[c_ptr->feat].flags1 & FF1_PERMANENT)) break;

			cave_set_feat(&p_ptr->wpos, y, x, FEAT_GLYPH);

			break;
		}
  
                case GF_DISINTEGRATE:
                {
                        if((f_info[c_ptr->feat].flags1 & FF1_PERMANENT)) break;

                        if (((c_ptr->feat == FEAT_TREES) || (c_ptr->feat == FEAT_SMALL_TREES) ||
                            (f_info[c_ptr->feat].flags1 & FF1_FLOOR)) &&
                            randint(100) < 30)
                        {
                                cave_set_feat(y, x, FEAT_ASH);
                        }                 
                        break;
                }

#endif	// 0

		case GF_WAVE:
		case GF_WATER:
		{
			int p1 = 0;
			int p2 = 0;
			int f1 = 0;
			int f2 = 0;
			int f = 0;
			int k;

			/* "Permanent" features will stay */
			if ((f_info[c_ptr->feat].flags1 & FF1_PERMANENT)) break;

			/* Needs more than 30 damage */
			if (dam < 30) break;


			if ((c_ptr->feat == FEAT_FLOOR) ||
			    (c_ptr->feat == FEAT_DIRT) ||
			    (c_ptr->feat == FEAT_GRASS))
			{
				/* 35% chance to create shallow water */
				p1 = 35; f1 = FEAT_SHAL_WATER;

				/* 5% chance to create deep water */
				p2 = 40; f2 = FEAT_DEEP_WATER;
			}
#if 1
			else if ((c_ptr->feat == FEAT_MAGMA) ||
			         (c_ptr->feat == FEAT_MAGMA_H) ||
			         (c_ptr->feat == FEAT_MAGMA_K) ||
			         (c_ptr->feat == FEAT_SHAL_LAVA))
			{
				/* 15% chance to convert it to normal floor */
				p1 = 15; f1 = FEAT_FLOOR;
			}
			else if (c_ptr->feat == FEAT_DEEP_LAVA)
			{
				/* 10% chance to convert it to shallow lava */
				p1 = 10; f1 = FEAT_SHAL_LAVA;

				/* 5% chance to convert it to normal floor */
				p2 = 15; f2 = FEAT_FLOOR;
			}
			else if ((c_ptr->feat == FEAT_SHAL_WATER) ||
			         (c_ptr->feat == FEAT_DARK_PIT))
			{
				/* 10% chance to convert it to deep water */
				p1 = 10; f1 = FEAT_DEEP_WATER;
			}
#endif	// 0

			k = rand_int(100);

			if (k < p1) f = f1;
			else if (k < p2) f = f2;

			if (f)
			{
				if (f == FEAT_FLOOR) place_floor(wpos, y, x);
				else cave_set_feat(wpos, y, x, f);

				/* Notice */
				if (!quiet) note_spot(Ind, y, x);

				/* Redraw */
				everyone_lite_spot(wpos, y, x);
//				if (seen) obvious = TRUE;
			}

			break;
		}

	}

	/* Return "Anything seen?" */
	return (obvious);
}



/*
 * We are called from "project()" to "damage" objects
 *
 * We are called both for "beam" effects and "ball" effects.
 *
 * Perhaps we should only SOMETIMES damage things on the ground.
 *
 * The "r" parameter is the "distance from ground zero".
 *
 * Note that we determine if the player can "see" anything that happens
 * by taking into account: blindness, line-of-sight, and illumination.
 *
 * XXX XXX XXX We also "see" grids which are "memorized", probably a hack
 *
 * We return "TRUE" if the effect of the projection is "obvious".
 */
static bool project_i(int Ind, int who, int r, struct worldpos *wpos, int y, int x, int dam, int typ)
{
	player_type *p_ptr=NULL;

	u16b this_o_idx, next_o_idx = 0;

	bool	obvious = FALSE;

	bool quiet = ((Ind <= 0 || Ind >= 0 - PROJECTOR_UNUSUAL) ? TRUE : FALSE);

	    u32b f1, f2, f3, f4, f5, esp;

	char	o_name[160];

	int o_sval = 0;
	bool is_potion = FALSE;

	int		div;
	cave_type **zcave;
	cave_type *c_ptr;
	object_type *o_ptr;
	if(!(zcave=getcave(wpos))) return(FALSE);
	c_ptr=&zcave[y][x];
//	o_ptr = &o_list[c_ptr->o_idx];

	/* Nothing here */
	if (!c_ptr->o_idx) return (FALSE);

	/* Set the player pointer */
	if (!quiet)
		p_ptr = Players[Ind];

	/* Extract radius */
	div = r + 1;

	/* Adjust damage */
	dam = dam / div;


	/* XXX XXX */
	who = who ? who : 0;

	/* Scan all objects in the grid */
	for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
	bool	is_art = FALSE;
	bool	ignore = FALSE;
	bool	plural = FALSE;
	bool	do_kill = FALSE;

	cptr	note_kill = NULL;

	/* Acquire object */
	o_ptr = &o_list[this_o_idx];

	/* Acquire next object */
	next_o_idx = o_ptr->next_o_idx;

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	/* Get the "plural"-ness */
	if (o_ptr->number > 1) plural = TRUE;

	/* Check for artifact */
	if (artifact_p(o_ptr)) is_art = TRUE;

	o_sval = o_ptr->sval;
	/* XXX POTION2 is not handled right! */
//	is_potion = ((k_info[o_ptr->k_idx].tval == TV_POTION)||(k_info[o_ptr->k_idx].tval == TV_POTION2));
	is_potion = (k_info[o_ptr->k_idx].tval == TV_POTION);

	/* Analyze the type */
	switch (typ)
	{
		/* Identify */
		case GF_IDENTIFY:
		{
			do_kill = FALSE;
			
			/* Identify it fully */
			object_aware(Ind, o_ptr);
			object_known(o_ptr);
			break;
		}
		
		/* Acid -- Lots of things */
		case GF_ACID:
		{
			if (hates_acid(o_ptr))
			{
				do_kill = TRUE;
				note_kill = (plural ? " melt!" : " melts!");
				if (f3 & TR3_IGNORE_ACID) ignore = TRUE;
			}
			break;
		}

		/* Elec -- Rings and Wands */
		case GF_ELEC:
		{
			if (hates_elec(o_ptr))
			{
				do_kill = TRUE;
				note_kill = (plural ? " are destroyed!" : " is destroyed!");
				if (f3 & TR3_IGNORE_ELEC) ignore = TRUE;
			}
			break;
		}

		/* Fire -- Flammable objects */
		case GF_FIRE:
		{
			if (hates_fire(o_ptr))
			{
				do_kill = TRUE;
				note_kill = is_potion ? (plural ? " evaporate!" : " evaporates!")
					: (plural ? " burn up!" : " burns up!");
				if (f3 & TR3_IGNORE_FIRE) ignore = TRUE;
			}
			break;
		}

		/* Cold -- potions and flasks */
		case GF_COLD:
		{
			if (hates_cold(o_ptr))
			{
				note_kill = (plural ? " shatter!" : " shatters!");
				do_kill = TRUE;
				if (f3 & TR3_IGNORE_COLD) ignore = TRUE;
			}
			break;
		}

		/* Cold -- potions and flasks */
		case GF_WATER:
		{
			if (hates_water(o_ptr))
			{
				note_kill = (plural ? " is soaked!" : " are soaked!");
				do_kill = TRUE;
				if (f5 & TR5_IGNORE_WATER) ignore = TRUE;
			}
			break;
		}

		/* Fire + Elec */
		case GF_PLASMA:
		{
			if (hates_fire(o_ptr))
			{
				do_kill = TRUE;
				note_kill = (plural ? " burn up!" : " burns up!");
				if (f3 & TR3_IGNORE_FIRE) ignore = TRUE;
			}
			if (hates_elec(o_ptr))
			{
				ignore = FALSE;
				do_kill = TRUE;
				note_kill = (plural ? " are destroyed!" : " is destroyed!");
				if (f3 & TR3_IGNORE_ELEC) ignore = TRUE;
			}
			break;
		}

		/* Fire + Cold */
		case GF_METEOR:
		{
			if (hates_fire(o_ptr))
			{
				do_kill = TRUE;
				note_kill = (plural ? " burn up!" : " burns up!");
				if (f3 & TR3_IGNORE_FIRE) ignore = TRUE;
			}
			if (hates_cold(o_ptr))
			{
				ignore = FALSE;
				do_kill = TRUE;
				note_kill = (plural ? " shatter!" : " shatters!");
				if (f3 & TR3_IGNORE_COLD) ignore = TRUE;
			}
			break;
		}

		/* Hack -- break potions and such */
		case GF_ICE:
		case GF_SHARDS:
		case GF_FORCE:
		case GF_SOUND:
		{
			if (hates_cold(o_ptr))
			{
				note_kill = (plural ? " shatter!" : " shatters!");
				do_kill = TRUE;
			}
			break;
		}

		/* Mana -- destroys everything */
		case GF_MANA:
		{
			do_kill = TRUE;
			note_kill = (plural ? " are destroyed!" : " is destroyed!");
		}

		case GF_DISINTEGRATE:
		{
			do_kill = TRUE;
			note_kill = (plural ? " evaporate!" : " evaporates!");
			break;
		}

		/* Holy Orb -- destroys cursed non-artifacts */
		case GF_HOLY_ORB:
		case GF_HOLY_FIRE:
//		case GF_HELL_FIRE:
		{
			if (cursed_p(o_ptr))
			{
				do_kill = TRUE;
				note_kill = (plural ? " are destroyed!" : " is destroyed!");
			}
			break;
		}

		/* Unlock chests */
		case GF_KILL_TRAP:
		case GF_KILL_DOOR:
		{
			/* Chests are noticed only if trapped or locked */
			if (o_ptr->tval == TV_CHEST)
			{
				/* Disarm/Unlock traps */
				if (o_ptr->pval > 0)
				{
					/* Disarm or Unlock */
					o_ptr->pval = (0 - o_ptr->pval);

					/* Identify */
					object_known(o_ptr);

					/* Notice */
//					if (!quiet && p_ptr->obj_vis[c_ptr->o_idx])
					if (!quiet && p_ptr->obj_vis[this_o_idx])
					{
						msg_print(Ind, "Click!");
						obvious = TRUE;
					}
				}
			}

			break;
		}

		/* Nexus, Gravity -- teleports the object away */
		case GF_NEXUS:
		case GF_GRAVITY:
		{
			int j, dist = (typ == GF_NEXUS ? 80 : 15);
			object_type tmp_obj = *o_ptr;
			if (check_st_anchor(wpos, y, x)) break;
//			if (seen) obvious = TRUE;

			note_kill = (plural ? " disappear!" : " disappears!");

			for (j=0;j<10;j++)
			{
				s16b cx = x+dist-rand_int(dist * 2);
				s16b cy = y+dist-rand_int(dist * 2);
				if (!in_bounds(cy, cx)) continue;
				if (!cave_floor_bold(zcave, cy, cx) ||
					cave_perma_bold(zcave, cy, cx)) continue;

//				(void)floor_carry(cy, cx, &tmp_obj);
				drop_near(&tmp_obj, 0, wpos, cy, cx);

				/* XXX not working? */
				if (!quiet && note_kill)
					msg_format_near_site(y, x, wpos, "The %s%s", o_name, note_kill);

				delete_object_idx(this_o_idx);

				break;
			}
			break;
		}

	}


	/* Attempt to destroy the object */
//	if(do_kill && (wpos->wz))
	if(do_kill)
	{
		/* Effect "observed" */
//		if (!quiet && p_ptr->obj_vis[c_ptr->o_idx])
		if (!quiet && p_ptr->obj_vis[this_o_idx])
		{
			obvious = TRUE;
			object_desc(Ind, o_name, o_ptr, FALSE, 0);
		}

		/* Artifacts, and other objects, get to resist */
		if (is_art || ignore)
		{
			/* Observe the resist */
//			if (!quiet && p_ptr->obj_vis[c_ptr->o_idx])
			if (!quiet && p_ptr->obj_vis[this_o_idx])
			{
				msg_format(Ind, "The %s %s unaffected!",
				           o_name, (plural ? "are" : "is"));
			}
		}

		/* Kill it */
		else
		{
			/* Describe if needed */
//			if (!quiet && p_ptr->obj_vis[c_ptr->o_idx] && note_kill)
			if (!quiet && p_ptr->obj_vis[this_o_idx] && note_kill)
			{
				msg_format(Ind, "The %s%s", o_name, note_kill);
			}

			/* Delete the object */
//			delete_object(wpos, y, x);
			delete_object_idx(this_o_idx);

			/* Potions produce effects when 'shattered' */
			if (is_potion)
			{
				(void)potion_smash_effect(who, wpos, y, x, o_sval);
			}

			if (!quiet)
			{
				/* Redraw */
				everyone_lite_spot(wpos, y, x);
			}
		}
	}

	}
	/* Return "Anything seen?" */
	return (obvious);
}



/*
 * Psionic attacking of high level demons/undead can backlash
 */
static bool psi_backlash(int Ind, int m_idx, int dam)
{
	monster_type *m_ptr = &m_list[m_idx];
        monster_race *r_ptr = race_inf(m_ptr);
	char m_name[80];
	player_type *p_ptr;

	if (!Ind) return FALSE;

	p_ptr = Players[Ind];

	if ((r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) &&
	    (r_ptr->level > p_ptr->lev/2) && !rand_int(2))
	{
		monster_desc(Ind, m_name, m_idx, 0);
		msg_format(Ind, "%^s's corrupted mind backlashes your attack!",
		           m_name);
		project(Ind, m_idx, 0, p_ptr->py, p_ptr->px ,dam / 3, GF_PSI, 0);
		return TRUE;
	}
	return FALSE;
}		


/*
 * Helper function for "project()" below.
 *
 * Handle a beam/bolt/ball causing damage to a monster.
 *
 * This routine takes a "source monster" (by index) which is mostly used to
 * determine if the player is causing the damage, and a "radius" (see below),
 * which is used to decrease the power of explosions with distance, and a
 * location, via integers which are modified by certain types of attacks
 * (polymorph and teleport being the obvious ones), a default damage, which
 * is modified as needed based on various properties, and finally a "damage
 * type" (see below).
 *
 * Note that this routine can handle "no damage" attacks (like teleport) by
 * taking a "zero" damage, and can even take "parameters" to attacks (like
 * confuse) by accepting a "damage", using it to calculate the effect, and
 * then setting the damage to zero.  Note that the "damage" parameter is
 * divided by the radius, so monsters not at the "epicenter" will not take
 * as much damage (or whatever)...
 *
 * Note that "polymorph" is dangerous, since a failure in "place_monster()"'
 * may result in a dereference of an invalid pointer.  XXX XXX XXX
 *
 * Various messages are produced, and damage is applied.
 *
 * Just "casting" a substance (i.e. plasma) does not make you immune, you must
 * actually be "made" of that substance, or "breathe" big balls of it.
 *
 * We assume that "Plasma" monsters, and "Plasma" breathers, are immune
 * to plasma.
 *
 * We assume "Nether" is an evil, necromantic force, so it doesn't hurt undead,
 * and hurts evil less.  If can breath nether, then it resists it as well.
 *
 * Damage reductions use the following formulas:
 *   Note that "dam = dam * 6 / (randint(6) + 6);"
 *     gives avg damage of .655, ranging from .858 to .500
 *   Note that "dam = dam * 5 / (randint(6) + 6);"
 *     gives avg damage of .544, ranging from .714 to .417
 *   Note that "dam = dam * 4 / (randint(6) + 6);"
 *     gives avg damage of .444, ranging from .556 to .333
 *   Note that "dam = dam * 3 / (randint(6) + 6);"
 *     gives avg damage of .327, ranging from .427 to .250
 *   Note that "dam = dam * 2 / (randint(6) + 6);"
 *     gives something simple.
 *
 * In this function, "result" messages are postponed until the end, where
 * the "note" string is appended to the monster name, if not NULL.  So,
 * to make a spell have "no effect" just set "note" to NULL.  You should
 * also set "notice" to FALSE, or the player will learn what the spell does.
 *
 * We attempt to return "TRUE" if the player saw anything "useful" happen.
 */
static bool project_m(int Ind, int who, int r, struct worldpos *wpos, int y, int x, int dam, int typ)
{
	int i, div;

	monster_type *m_ptr;

	monster_race *r_ptr;

	cptr name;

	/* Is the monster "seen"? */
	bool seen;

	/* Were the "effects" obvious (if seen)? */
	bool obvious = FALSE;

	/* do not notice things the player did to themselves by ball spell */
	/* fix me XXX XXX XXX */
	
	/* Polymorph setting (true or false) */
	int do_poly = 0;

	/* Teleport setting (max distance) */
	int do_dist = 0;

	/* Confusion setting (amount to confuse) */
	int do_conf = 0;

	/* Stunning setting (amount to stun) */
	int do_stun = 0;

	/* Sleep amount (amount to sleep) */
	int do_sleep = 0;

	/* Fear amount (amount to fear) */
	int do_fear = 0;

	/* Hold the monster name */
	char m_name[80];

	bool resist = FALSE;

	/* Assume no note */
	cptr note = NULL;

	/* Assume a default death */
	cptr note_dies = " dies";

	int plev = 25;
	cave_type **zcave;
	cave_type *c_ptr;
	bool quiet;
	player_type *p_ptr = NULL;
	if(!(zcave=getcave(wpos))) return(FALSE);
	c_ptr=&zcave[y][x];
	/* hack -- by trap */
	quiet = ((Ind <= 0 || who <= PROJECTOR_UNUSUAL) ? TRUE : (0 - Ind == c_ptr->m_idx?TRUE:FALSE));

//	if(quiet) return(FALSE);
	if (Ind <= 0 || 0 - Ind == c_ptr->m_idx) return(FALSE);
	if (!quiet)
	{
		p_ptr = Players[Ind];
		plev = p_ptr->lev;
	}

	/* Nobody here */
	if (c_ptr->m_idx <= 0) return (FALSE);

	/* Acquire monster pointer */
 	m_ptr = &m_list[c_ptr->m_idx];

	/* Acquire race pointer */
        r_ptr = race_inf(m_ptr);

	/* Acquire name */
        name = r_name_get(m_ptr);

	/* Never affect projector */
	if ((who > 0) && (c_ptr->m_idx == who)) return (FALSE);

        /* Never hurt golem */
        if (who < 0 && who > PROJECTOR_UNUSUAL)
        {
                if (Players[-who]->id == m_ptr->owner) return (FALSE);
        }

	/* Set the "seen" flag */
	if (!quiet)
	{
		seen = p_ptr->mon_vis[c_ptr->m_idx];
		/* Cannot 'hear' if too far */
		if (!seen && distance(p_ptr->py, p_ptr->px, y, x) > MAX_SIGHT)
			quiet = TRUE;
	}
	else seen = FALSE;

	/* Extract radius */
	div = r + 1;

	/* Decrease damage */
	dam = dam / div;


	/* Mega-Hack */
	project_m_n++;
	project_m_x = x;
	project_m_y = y;


	/* Get the monster name (BEFORE polymorphing) */
	if (!quiet) monster_desc(Ind, m_name, c_ptr->m_idx, 0);



	/* Some monsters get "destroyed" */
	if ((r_ptr->flags3 & RF3_DEMON) ||
	    (r_ptr->flags3 & RF3_UNDEAD) ||
	    (r_ptr->flags2 & RF2_STUPID) ||
	    (strchr("Evg", r_ptr->d_char)))
	{
		/* Special note at death */
		note_dies = " is destroyed";
	}


	/* Analyze the damage type */
	switch (typ)
	{
		/* PSIONICS */
		case GF_PSI:
		{
			if (seen) obvious = TRUE;
			note_dies = " collapses, a mindless husk.";

			if (r_ptr->flags9 & RF9_RES_PSI)
			if (rand_int(3))
			{
				resist = TRUE;
			}

			if ((r_ptr->flags9 & RF9_IM_PSI) || (r_ptr->flags2 & RF2_EMPTY_MIND))
			{
				note = " is unaffected.";
				dam = 0;
				break;
			}

			if (randint(100) < r_ptr->level) resist = TRUE;

			if (r_ptr->flags1 & RF1_UNIQUE) if (rand_int(3)) resist = TRUE;
	
			if (r_ptr->flags2 & RF2_SMART)
			{
				if (!resist) dam += dam / 2;
			}
	
			if (r_ptr->flags2 & RF2_WEIRD_MIND)
			{
				if (randint(100) < 80) resist = TRUE;
				else {dam *= 3; dam /= 4;}
	 	        }
	 	
			if ((r_ptr->flags2 & RF2_STUPID) ||
			    (r_ptr->flags3 & RF3_ANIMAL))
			{
				note = " is too stupid to be hurt.";
				dam /= 4;
			}
			
			//			if (psi_backlash(Ind, c_ptr->m_idx, dam)) resist = TRUE;

			if (((m_ptr->confused > 0) && !rand_int(3)) ||
			    ((m_ptr->confused > 20) && !rand_int(3)) ||
			    ((m_ptr->confused > 50) && !rand_int(3)))
			{
				resist = FALSE;
				dam = dam * (3 + randint(7)) / 4;
			}
	
			if (resist)
			{
				note = " resists.";
				dam /= 3;
				if (randint(100) < 10) do_conf = randint(8);
			}
			else
			if (randint(dam>20 ? 20 : dam) > randint(r_ptr->level))
			{
				do_stun = randint(6);
				do_conf = randint(20);
				do_sleep = rand_int(2) ? randint(randint(90)) : 0;
				do_fear = randint(15);
			}
      			break;
		}

		/* Earthquake the area */
		case GF_EARTHQUAKE:
		{
			if (seen) obvious = TRUE;
			earthquake(wpos, y, x, dam);
			dam = 0;
			break;
		}

			/* Magic Missile -- pure damage */
		case GF_MISSILE:
		{
			if (seen) obvious = TRUE;
			break;
		}

			/* Acid */
		case GF_ACID:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_IM_ACID)
			{
				note = " resists a lot";
				dam /= 9;
				if (seen) r_ptr->r_flags3 |= RF3_IM_ACID;
			}
			break;
		}

			/* Electricity */
		case GF_ELEC:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_IM_ELEC)
			{
				note = " resists a lot";
				dam /= 9;
				if (seen) r_ptr->r_flags3 |= RF3_IM_ELEC;
			}
			break;
		}

			/* Fire damage */
		case GF_FIRE:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_IM_FIRE)
			{
				note = " resists a lot";
				dam /= 9;
				if (seen) r_ptr->r_flags3 |= RF3_IM_FIRE;
			}
			break;
		}

			/* Cold */
		case GF_COLD:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_IM_COLD)
			{
				note = " resists a lot";
				dam /= 9;
				if (seen) r_ptr->r_flags3 |= RF3_IM_COLD;
			}
			break;
		}

                /* Poison */
		case GF_POIS:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_IM_POIS)
			{
				note = " resists a lot";
				dam /= 9;
				if (seen) r_ptr->r_flags3 |= RF3_IM_POIS;
			}
			break;
		}

		/* Thick Poison */
		case GF_UNBREATH:
		{
			if (seen) obvious = TRUE;
//			if (magik(15)) do_pois = (10 + randint(11) + r) / (r + 1);
			if ((r_ptr->flags3 & (RF3_NONLIVING)) || (r_ptr->flags3 & (RF3_UNDEAD)))
			{
				note = " is immune.";
				dam = 0;
//				do_pois = 0;
			}
			break;
		}

			/* Holy Orb -- hurts Evil */
		case GF_HOLY_ORB:
//		case GF_HELL_FIRE:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_EVIL)
			{
				dam *= 2;
				if (seen) r_ptr->r_flags3 |= RF3_EVIL;
			}
			break;
		}

		/* Holy Fire -- hurts Evil, Good are immune, others _resist_ */
		case GF_HOLY_FIRE:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & (RF3_GOOD))
			{
				dam = 0;
				note = " is immune.";
				if (seen) r_ptr->r_flags3 |= (RF3_GOOD);
			}
			else if (r_ptr->flags3 & (RF3_EVIL))
			{
				dam *= 2;
				note = " is hit hard.";
				if (seen) r_ptr->r_flags3 |= (RF3_EVIL);
			}
			else
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
			}
			break;
		}

			/* Arrow -- XXX no defense */
		case GF_ARROW:
		{
			if (seen) obvious = TRUE;
			break;
		}

			/* Plasma -- XXX perhaps check ELEC or FIRE */
		case GF_PLASMA:
		{
			if (seen) obvious = TRUE;
			if (prefix(name, "Plasma") ||
			    (r_ptr->flags4 & RF4_BR_PLAS))
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
			}
			break;
		}

			/* Nether -- see above */
		case GF_NETHER:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_UNDEAD)
			{
				note = " is immune.";
				dam = 0;
				if (seen) r_ptr->r_flags3 |= RF3_UNDEAD;
			}
			else if (r_ptr->flags4 & RF4_BR_NETH)
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
			}
			else if (r_ptr->flags3 & RF3_EVIL)
			{
				dam /= 2;
				note = " resists somewhat.";
				if (seen) r_ptr->r_flags3 |= RF3_EVIL;
			}
			break;
		}

			/* Water (acid) damage -- Water spirits/elementals are immune */
		case GF_WATER:
		{
			if (seen) obvious = TRUE;
			if ((r_ptr->d_char == 'E') && prefix(name, "W"))
			{
				note = " is immune.";
				dam = 0;
			}
			else if (r_ptr->flags7 & RF7_AQUATIC)
			{
				note = " resists a lot";
				dam /= 9;
			}
			break;
		}


		/* Wave = Water + Force */
		case GF_WAVE:
		{
                        if (seen) obvious = TRUE;

			if ((r_ptr->d_char == 'E') &&
			   (prefix(name, "W")))
			{
				note = " is immune.";
				dam = 0;
			}
			else if (r_ptr->flags7 & RF7_AQUATIC)
			{
				note = " resists a lot";
				dam /= 9;
                        }
                        else
                        {
                                do_stun = randint(15) / div;
                        }

			break;
		}

                /* Chaos -- Chaos breathers resist */
		case GF_CHAOS:
		{
			if (seen) obvious = TRUE;
			do_poly = TRUE;
			do_conf = (5 + randint(11)) / div;
			if (r_ptr->flags4 & RF4_BR_CHAO)
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
				do_poly = FALSE;
			}
			break;
		}

			/* Shards -- Shard breathers resist */
		case GF_SHARDS:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags4 & RF4_BR_SHAR)
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
			}
			break;
		}

		/* Rocket: Shard resistance helps (PernA) */
		case GF_ROCKET:
		{
			if (seen) obvious = TRUE;

//			if (magik(12)) do_cut = (10 + randint(15) +r) / (r + 1);
			if (r_ptr->flags4 & (RF4_BR_SHAR))
			{
				note = " resists somewhat.";
				dam /= 2;
//                                do_cut = 0;
			}
			break;
		}

			/* Sound -- Sound breathers resist */
		case GF_STUN:
		{
			do_stun = (10 + randint(15)) / div;
			break;
		}

			/* Sound -- Sound breathers resist */
		case GF_SOUND:
		{
			if (seen) obvious = TRUE;
			do_stun = (10 + randint(15)) / div;
			if (r_ptr->flags4 & RF4_BR_SOUN)
			{
				note = " resists.";
				dam *= 2; dam /= (randint(6)+6);
			}
			break;
		}

			/* Confusion */
		case GF_CONFUSION:
		{
			if (seen) obvious = TRUE;
			do_conf = (10 + randint(15)) / div;
			if (r_ptr->flags4 & RF4_BR_CONF)
			{
				note = " resists.";
				dam *= 2; dam /= (randint(6)+6);
			}
			else if (r_ptr->flags3 & RF3_NO_CONF)
			{
				note = " resists somewhat.";
				dam /= 2;
			}
			break;
		}

			/* Disenchantment -- Breathers and Disenchanters resist */
		case GF_DISENCHANT:
		{
			if (seen) obvious = TRUE;
			if ((r_ptr->flags4 & RF4_BR_DISE) ||
			    prefix(name, "Disen"))
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
			}
			break;
		}

			/* Nexus -- Breathers and Existers resist */
		case GF_NEXUS:
		{
			if (seen) obvious = TRUE;
			if ((r_ptr->flags4 & RF4_BR_NEXU) ||
			    prefix(name, "Nexus"))
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
			}
			break;
		}

			/* Force */
		case GF_FORCE:
		{
			if (seen) obvious = TRUE;
			do_stun = randint(15) / div;
			if (r_ptr->flags4 & RF4_BR_WALL)
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
			}
			break;
		}

			/* Inertia -- breathers resist */
		case GF_INERTIA:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags4 & RF4_BR_INER)
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
			}
			break;
		}

			/* Time -- breathers resist */
		case GF_TIME:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags4 & RF4_BR_TIME)
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
			}
			else if (!quiet)
			  {
			    int sec = m_ptr->hp, t;

			    note = " loses some precious seconds.";
			    m_ptr->energy = 0;
			    
			    msg_print(Ind, "You gain some precious seconds of time.");
			    if (sec > dam) sec = dam;
			    if (sec > (t = damroll(2, plev))) sec = t;

			    p_ptr->energy += t * level_speed(&p_ptr->wpos) / 100;
			  }
			break;
		}

			/* Gravity -- breathers resist */
		case GF_GRAVITY:
		{
			bool resist_tele = FALSE;
			dun_level		*l_ptr = getfloor(wpos);

			if (seen) obvious = TRUE;
			
			if (r_ptr->flags3 & (RF3_RES_TELE))
			{
				if (r_ptr->flags1 & (RF1_UNIQUE))
				{
					if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
					note = " is unaffected!";
					resist_tele = TRUE;
				}
				else if (m_ptr->level > randint(100))
				{
					if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
					note = " resists!";
					resist_tele = TRUE;
				}
			}

			if (!resist_tele) do_dist = 10;
			else do_dist = 0;

			if (r_ptr->flags4 & RF4_BR_GRAV)
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
				do_dist = 0;
			}
			break;
		}

			/* Pure damage */
		case GF_MANA:
		{
			if (seen) obvious = TRUE;
			break;
		}

			/* Meteor -- powerful magic missile */
		case GF_METEOR:
		{
			if (seen) obvious = TRUE;
			break;
		}

			/* Ice -- Cold + Cuts + Stun */
		case GF_ICE:
		{
			if (seen) obvious = TRUE;
			do_stun = randint(15) / div;
			if (r_ptr->flags3 & RF3_IM_COLD)
			{
				note = " resists a lot";
				dam /= 9;
				if (seen) r_ptr->r_flags3 |= RF3_IM_COLD;
			}
			break;
		}


			/* Drain Life */
		case GF_OLD_DRAIN:
		{
			if (seen) obvious = TRUE;
			if ((r_ptr->flags3 & RF3_UNDEAD) ||
			    (r_ptr->flags3 & RF3_DEMON) ||
			    (strchr("Egv", r_ptr->d_char)))
			{
				if (r_ptr->flags3 & RF3_UNDEAD)
				{
					if (seen) r_ptr->r_flags3 |= RF3_UNDEAD;
				}
				if (r_ptr->flags3 & RF3_DEMON)
				{
					if (seen) r_ptr->r_flags3 |= RF3_DEMON;
				}

				note = " is unaffected!";
				obvious = FALSE;
				dam = 0;
			}

			break;
		}

			/* Polymorph monster (Use "dam" as "power") */
		case GF_OLD_POLY:
		{
			if (seen) obvious = TRUE;

			/* Attempt to polymorph (see below) */
			do_poly = TRUE;

			/* Powerful monsters can resist */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				note = " is unaffected!";
				do_poly = FALSE;
				obvious = FALSE;
			}

			/* No "real" damage */
			dam = 0;

			break;
		}


			/* Clone monsters (Ignore "dam") */
		case GF_OLD_CLONE:
		{
			if (seen) obvious = TRUE;

			/* Heal fully */
			m_ptr->hp = m_ptr->maxhp;

			/* Speed up */
			if (m_ptr->mspeed < 150) m_ptr->mspeed += 10;

			/* Attempt to clone. */
			if (multiply_monster(c_ptr->m_idx))
			{
				note = " spawns!";
			}

			/* No "real" damage */
			dam = 0;

			break;
		}


			/* Heal Monster (use "dam" as amount of healing) */
		case GF_OLD_HEAL:
		{
			if (seen) obvious = TRUE;

			/* Wake up */
			m_ptr->csleep = 0;

			/* Heal */
			m_ptr->hp += dam;

			/* No overflow */
			if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

			/* Redraw (later) if needed */
			update_health(c_ptr->m_idx);

			/* Message */
			note = " looks healthier.";

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Speed Monster (Ignore "dam") */
		case GF_OLD_SPEED:
		{
			if (seen) obvious = TRUE;

			/* Speed up */
			if (m_ptr->mspeed < 150) m_ptr->mspeed += 10;
			note = " starts moving faster.";

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Slow Monster (Use "dam" as "power") */
		case GF_OLD_SLOW:
		{
			if (seen) obvious = TRUE;

			/* Powerful monsters can resist */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				note = " is unaffected!";
				obvious = FALSE;
			}

			/* Normal monsters slow down */
			else
			{
				if (m_ptr->mspeed > 60) m_ptr->mspeed -= 10;
				note = " starts moving slower.";
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Sleep (Use "dam" as "power") */
		case GF_OLD_SLEEP:
		{
			if (seen) obvious = TRUE;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->flags3 & RF3_NO_SLEEP) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				/* Memorize a flag */
				if (r_ptr->flags3 & RF3_NO_SLEEP)
				{
					if (seen) r_ptr->r_flags3 |= RF3_NO_SLEEP;
				}

				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}
			else
			{
				/* Go to sleep (much) later */
				note = " falls asleep!";
				do_sleep = 500;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Confusion (Use "dam" as "power") */
		case GF_OLD_CONF:
		{
			if (seen) obvious = TRUE;

			/* Get confused later */
			do_conf = damroll(3, (dam / 2)) + 1;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->flags3 & RF3_NO_CONF) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				/* Memorize a flag */
				if (r_ptr->flags3 & RF3_NO_CONF)
				{
					if (seen) r_ptr->r_flags3 |= RF3_NO_CONF;
				}

				/* Resist */
				do_conf = 0;

				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}



			/* Lite, but only hurts susceptible creatures */
		case GF_LITE_WEAK:
		{
			/* Hurt by light */
			if (r_ptr->flags3 & RF3_HURT_LITE)
			{
				/* Obvious effect */
				if (seen) obvious = TRUE;

				/* Memorize the effects */
				if (seen) r_ptr->r_flags3 |= RF3_HURT_LITE;

				/* Special effect */
				note = " cringes from the light!";
				note_dies = " shrivels away in the light!";
			}

			/* Normally no damage */
			else
			{
				/* No damage */
				dam = 0;
			}

			break;
		}



			/* Lite -- opposite of Dark */
		case GF_LITE:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags4 & RF4_BR_LITE)
			{
				note = " resists.";
				dam *= 2; dam /= (randint(6)+6);
			}
			else if (r_ptr->flags3 & RF3_HURT_LITE)
			{
				if (seen) r_ptr->r_flags3 |= RF3_HURT_LITE;
				note = " cringes from the light!";
				note_dies = " shrivels away in the light!";
				dam *= 2;
			}
			break;
		}


			/* Dark -- opposite of Lite */
		case GF_DARK:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags4 & RF4_BR_DARK)
			{
				note = " resists.";
				dam *= 2; dam /= (randint(6)+6);
			}
			break;
		}


			/* Stone to Mud */
		case GF_KILL_WALL:
		{
			/* Hurt by rock remover */
			if (r_ptr->flags3 & RF3_HURT_ROCK)
			{
				/* Notice effect */
				if (seen) obvious = TRUE;

				/* Memorize the effects */
				if (seen) r_ptr->r_flags3 |= RF3_HURT_ROCK;

				/* Cute little message */
				note = " loses some skin!";
				note_dies = " dissolves!";
			}

			/* Usually, ignore the effects */
			else
			{
				/* No damage */
				dam = 0;
			}

			break;
		}


			/* Teleport undead (Use "dam" as "power") */
		case GF_AWAY_UNDEAD:
		{
			/* Only affect undead */
			if (!(r_ptr->flags9 & RF9_IM_TELE) &&
				(r_ptr->flags3 & (RF3_UNDEAD)))
			{
				bool resists_tele = FALSE;

				if (r_ptr->flags3 & (RF3_RES_TELE))
				{
					if (r_ptr->flags1 & (RF1_UNIQUE))
					{
						if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
						note = " is unaffected!";
						resists_tele = TRUE;
					}
					else if (m_ptr->level > randint(100))
					{
						if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
						note = " resists!";
						resists_tele = TRUE;
					}
				}

				if (!resists_tele)
				{
					if (seen) obvious = TRUE;
					if (seen) r_ptr->r_flags3 |= (RF3_UNDEAD);
					do_dist = dam;
				}
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Teleport evil (Use "dam" as "power") */
		case GF_AWAY_EVIL:
		{
			/* Only affect undead */
			if (!(r_ptr->flags9 & RF9_IM_TELE) &&
				(r_ptr->flags3 & (RF3_EVIL)))
			{
				bool resists_tele = FALSE;

				if (r_ptr->flags3 & (RF3_RES_TELE))
				{
					if (r_ptr->flags1 & (RF1_UNIQUE))
					{
						if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
						note = " is unaffected!";
						resists_tele = TRUE;
					}
					else if (m_ptr->level > randint(100))
					{
						if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
						note = " resists!";
						resists_tele = TRUE;
					}
				}

				if (!resists_tele)
				{
					if (seen) obvious = TRUE;
					if (seen) r_ptr->r_flags3 |= (RF3_EVIL);
					do_dist = dam;
				}
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Teleport monster (Use "dam" as "power") */
		case GF_AWAY_ALL:
		{
			bool resists_tele = FALSE;
			dun_level		*l_ptr = getfloor(wpos);

			if (!(r_ptr->flags9 & RF9_IM_TELE) &&
				(r_ptr->flags3 & (RF3_RES_TELE)))
			{
				if (r_ptr->flags1 & (RF1_UNIQUE))
				{
					if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
					note = " is unaffected!";
					resists_tele = TRUE;
				}
                                else if (m_ptr->level > randint(100))
				{
					if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
					note = " resists!";
					resists_tele = TRUE;
				}
			}

			if (!resists_tele)
			{
				/* Obvious */
				if (seen) obvious = TRUE;

				/* Prepare to teleport */
				do_dist = dam;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Turn undead (Use "dam" as "power") */
		case GF_TURN_UNDEAD:
		{
			/* Only affect undead */
			if (r_ptr->flags3 & RF3_UNDEAD)
			{
				/* Learn about type */
				if (seen) r_ptr->r_flags3 |= RF3_UNDEAD;

				/* Obvious */
				if (seen) obvious = TRUE;

				/* Apply some fear */
				do_fear = damroll(3, (dam / 2)) + 1;

				/* Attempt a saving throw */
				if (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10)
				{
					/* No obvious effect */
					note = " is unaffected!";
					obvious = FALSE;
					do_fear = 0;
				}
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Turn evil (Use "dam" as "power") */
		case GF_TURN_EVIL:
		{
			/* Only affect evil */
			if (r_ptr->flags3 & RF3_EVIL)
			{
				/* Learn about type */
				if (seen) r_ptr->r_flags3 |= RF3_EVIL;

				/* Obvious */
				if (seen) obvious = TRUE;

				/* Apply some fear */
				do_fear = damroll(3, (dam / 2)) + 1;

				/* Attempt a saving throw */
				if (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10)
				{
					/* No obvious effect */
					note = " is unaffected!";
					obvious = FALSE;
					do_fear = 0;
				}
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Turn monster (Use "dam" as "power") */
		case GF_TURN_ALL:
		{
			/* Obvious */
			if (seen) obvious = TRUE;

			/* Apply some fear */
			do_fear = damroll(3, (dam / 2)) + 1;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->flags3 & RF3_NO_FEAR) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
				do_fear = 0;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Dispel undead */
		case GF_DISP_UNDEAD:
		{
			/* Only affect undead */

			if (r_ptr->flags3 & RF3_UNDEAD)
			{
				/* Learn about type */
				if (seen) r_ptr->r_flags3 |= RF3_UNDEAD;

				/* Obvious */
				if (seen) obvious = TRUE;

				/* Message */
				note = " shudders."; 
				note_dies = " dissolves!";
			}

			/* Ignore other monsters */
			else
			{
				/* No damage */
				dam = 0;
			}

			break;
		}


			/* Dispel evil */
		case GF_DISP_EVIL:
		{
			/* Only affect evil */
			if (r_ptr->flags3 & RF3_EVIL)
			{
				/* Learn about type */
				if (seen) r_ptr->r_flags3 |= RF3_EVIL;

				/* Obvious */
				if (seen) obvious = TRUE;

				/* Message */
				note = " shudders.";
				note_dies = " dissolves!";
			}

			/* Ignore other monsters */
			else
			{
				/* No damage */
				dam = 0;
			}

			break;
		}


			/* Dispel monster */
		case GF_DISP_ALL:
		{
			/* Obvious */
			if (seen) obvious = TRUE;

			/* Message */
			note = " shudders.";
			note_dies = " dissolves!";

			break;
		}

		/* Nuclear waste */
		case GF_NUKE:
		{
			if (seen) obvious = TRUE;

			if (r_ptr->flags3 & (RF3_IM_POIS))
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
				if (seen) r_ptr->r_flags3 |= (RF3_IM_POIS);
			}
			else if (randint(3)==1) do_poly = TRUE;
			break;
		}


		/* Pure damage */
		case GF_DISINTEGRATE:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & (RF3_HURT_ROCK))
			{
				if (seen) r_ptr->r_flags3 |= (RF3_HURT_ROCK);
				note = " loses some skin!";
				note_dies = " evaporates!";
				dam *= 2;
			}

			if (r_ptr->flags1 & RF1_UNIQUE)
			{
				if (rand_int(m_ptr->level + 10) > rand_int(plev))
				{
					note = " resists.";
					dam >>= 3;
				}
			}
			break;
		}
		case GF_HOLD:
		case GF_DOMINATE:
			if(!quiet){
				if(!(r_ptr->flags1 & (RF1_UNIQUE|RF1_NEVER_MOVE)) && 
					!(r_ptr->flags9 & RF9_IM_PSI) && !(r_ptr->flags7 & RF7_MULTIPLY))
					m_ptr->owner=p_ptr->id;
				note = " starts following you.";
			}
			dam=0;
			break;

		/* Teleport monster TO */
		case GF_TELE_TO:
		{
			bool resists_tele = FALSE;
			dun_level		*l_ptr = getfloor(wpos);

			/* Teleport to nowhere..? */
			if (quiet) break;

			if (!(r_ptr->flags9 & RF9_IM_TELE) &&
				(r_ptr->flags3 & (RF3_RES_TELE)))
			{
				if (r_ptr->flags1 & (RF1_UNIQUE))
				{
					if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
					note = " is unaffected!";
					resists_tele = TRUE;
				}
				else if (m_ptr->level > randint(100))
				{
					if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
					note = " resists!";
					resists_tele = TRUE;
				}
			}

			if (!resists_tele)
			{
				/* Obvious */
				if (seen) obvious = TRUE;

				/* Prepare to teleport */
//				do_dist = dam;
				teleport_to_player(Ind, c_ptr->m_idx);

				/* Hack -- get new location */
				y = m_ptr->fy;
				x = m_ptr->fx;

				/* Hack -- get new grid */
				c_ptr = &zcave[y][x];
			}

			/* No "real" damage */
			dam = 0;
			break;
		}

		/* Hand of Doom */
		case GF_HAND_DOOM:
		{
#if 0	/* okay, do that! ;) */
			if(r_ptr->r_flags1 & RF1_UNIQUE)
			{
				note = " resists.";
				dam = 0;
			}
			else
#endif	// 0
			{
				int dummy = (((s32b) ((65 + randint(25)) * (m_ptr->hp))) / 100);
//				msg_print(Ind, "You feel your life fade away!");

				if (m_ptr->hp - dummy < 1) dummy = m_ptr->hp - 1;

				dam = dummy;
			}
			break;
		}

		/* Sleep (Use "dam" as "power") */
		case GF_STASIS:
		{
			if (seen) obvious = TRUE;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
			    (m_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				note = " is unaffected!";
				obvious = FALSE;
			}
			else
			{
				/* Go to sleep (much) later */
				note = " is suspended!";
				do_sleep = 500;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}

			/* Default */
		default:
		{
			/* No damage */
			dam = 0;

			break;
		}
	}



	/* "Unique" monsters cannot be polymorphed */
	if (r_ptr->flags1 & RF1_UNIQUE) do_poly = FALSE;


	/* "Unique" monsters can only be "killed" by the player */
	if (r_ptr->flags1 & RF1_UNIQUE)
	{
		/* Uniques may only be killed by the player */
		if ((who > 0) && (dam > m_ptr->hp)) dam = m_ptr->hp;
	}


	/* Check for death */
	if (dam > m_ptr->hp)
	{
		/* Extract method of death */
		note = note_dies;
	}

	/* Mega-Hack -- Handle "polymorph" -- monsters get a saving throw */
	else if (do_poly && (randint(90) > r_ptr->level))
	{
		/* Default -- assume no polymorph */
		note = " is unaffected!";

		/* Pick a "new" monster race */
		i = poly_r_idx(m_ptr->r_idx);

		/* Handle polymorh */
		if (i != m_ptr->r_idx)
		{
			/* Obvious */
			if (seen) obvious = TRUE;

			/* Monster polymorphs */
			note = " changes!";

			/* Turn off the damage */
			dam = 0;

			/* "Kill" the "old" monster */
			delete_monster_idx(c_ptr->m_idx);

			/* Create a new monster (no groups) */
			(void)place_monster_aux(wpos, y, x, i, FALSE, FALSE, FALSE);

			/* XXX XXX XXX Hack -- Assume success */
			if(!quiet && c_ptr->m_idx==0){
				msg_format(Ind, "%^s disappears!", m_name);
				return(FALSE);
			}

			/* Hack -- Get new monster */
			m_ptr = &m_list[c_ptr->m_idx];

			/* Hack -- Get new race */
                        r_ptr = race_inf(m_ptr);
		}
	}

	/* Handle "teleport" */
	else if (do_dist)
	{
	        /* Obvious */
		if (seen) obvious = TRUE;

		/* Message */
		note = " disappears!";

		/* Teleport */
		/* TODO: handle failure (eg. st-anchor)	*/
		teleport_away(c_ptr->m_idx, do_dist);

		/* Hack -- get new location */
		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Hack -- get new grid */
		c_ptr = &zcave[y][x];
	}

	/* Sound and Impact breathers never stun */
	else if (do_stun &&
	         !(r_ptr->flags4 & RF4_BR_SOUN) &&
	         !(r_ptr->flags4 & RF4_BR_WALL))
	{
		/* Obvious */
		if (seen) obvious = TRUE;

		/* Get confused */
		if (m_ptr->stunned)
		{
			note = " is more dazed.";
			i = m_ptr->stunned + (do_stun / 2);
		}
		else
		{
			note = " is dazed.";
			i = do_stun;
		}

		/* Apply stun */
		m_ptr->stunned = (i < 200) ? i : 200;
	}

	/* Confusion and Chaos breathers (and sleepers) never confuse */
	else if (do_conf &&
	         !(r_ptr->flags3 & RF3_NO_CONF) &&
	         !(r_ptr->flags4 & RF4_BR_CONF) &&
	         !(r_ptr->flags4 & RF4_BR_CHAO))
	{
		/* Obvious */
		if (seen) obvious = TRUE;

		/* Already partially confused */
		if (m_ptr->confused)
		{
			note = " looks more confused.";
			i = m_ptr->confused + (do_conf / 2);
		}

		/* Was not confused */
		else
		{
			note = " looks confused.";
			i = do_conf;
		}

		/* Apply confusion */
		m_ptr->confused = (i < 200) ? i : 200;
	}


	/* Fear */
	if (do_fear)
	{
		/* Increase fear */
		i = m_ptr->monfear + do_fear;

		/* Set fear */
		m_ptr->monfear = (i < 200) ? i : 200;
	}


	/* If another monster did the damage, hurt the monster by hand */
	if (who > 0 || who <= PROJECTOR_UNUSUAL)
	{
		/* Redraw (later) if needed */
		update_health(c_ptr->m_idx);

        /* Some mosnters are immune to death */
        if (r_ptr->flags7 & RF7_NO_DEATH) dam = 0;

		/* Wake the monster up */
		m_ptr->csleep = 0;

		/* Hurt the monster */
		m_ptr->hp -= dam;

		/* Dead monster */
		if (m_ptr->hp < 0)
		{
			/* Generate treasure, etc */
			if (!quiet) monster_death(Ind, c_ptr->m_idx);

			/* Delete the monster */
			delete_monster_idx(c_ptr->m_idx);

			/* Give detailed messages if destroyed */
			/* DEG Death message with damage. */
			if ((r_ptr->flags1 & RF1_UNIQUE) && (!quiet && note))
			{
				msg_format(Ind, "%^s%s by \377p%d \377wdamage.", m_name, note, dam);
			}	
			else	
			if (!quiet && note) msg_format(Ind, "%^s%s by \377g%d \377wdamage.", m_name, note, dam);
		}

		/* Damaged monster */
		else
		{
			/* Give detailed messages if visible or destroyed */
			if (!quiet && note && seen) msg_format(Ind, "%^s%s", m_name, note);

			/* Hack -- Pain message */
			else if (!quiet && dam > 0) message_pain(Ind, c_ptr->m_idx, dam);

			/* Hack -- handle sleep */
			if (do_sleep) m_ptr->csleep = do_sleep;
		}
	}

	/* If the player did it, give him experience, check fear */
	else
	{
		bool fear = FALSE;

		/* Hurt the monster, check for fear and death */
		if (!quiet && mon_take_hit(Ind, c_ptr->m_idx, dam, &fear, note_dies))
		{
			/* Dead monster */
		}

		/* Damaged monster */
		else
		{
			/* Give detailed messages if visible or destroyed */
			/* DEG Changed for added damage message. */
			if ((!quiet && note && seen)&&(r_ptr->flags1 & RF1_UNIQUE)) msg_format(Ind, "%^s%s and takes \377p%d \377wdamage.", m_name, note, dam);
			else if (!quiet && note && seen) msg_format(Ind, "%^s%s and takes \377g%d \377wdamage.", m_name, note, dam);

			/* Hack -- Pain message */
			else if (!quiet && dam > 0) message_pain(Ind, c_ptr->m_idx, dam);

			/* Take note */
			if (!quiet && (fear || do_fear) && (p_ptr->mon_vis[c_ptr->m_idx]))
			{
				/* Sound */
				sound(Ind, SOUND_FLEE);

				/* Message */
				msg_format(Ind, "%^s flees in terror!", m_name);
			}

			/* Hack -- handle sleep */
			if (do_sleep) m_ptr->csleep = do_sleep;
		}
	}


	/* Update the monster XXX XXX XXX */
	update_mon(c_ptr->m_idx, FALSE);

	if (!quiet)
	{
		/* Hack -- Redraw the monster grid anyway */
		everyone_lite_spot(wpos, y, x);
	}


	/* Return "Anything seen?" */
	return (obvious);
}






/*
 * Helper function for "project()" below.
 *
 * Handle a beam/bolt/ball causing damage to the player.
 *
 * This routine takes a "source monster" (by index), a "distance", a default
 * "damage", and a "damage type".  See "project_m()" above.
 *
 * If "rad" is non-zero, then the blast was centered elsewhere, and the damage
 * is reduced (see "project_m()" above).  This can happen if a monster breathes
 * at the player and hits a wall instead.
 *
 * We return "TRUE" if any "obvious" effects were observed.  XXX XXX Actually,
 * we just assume that the effects were obvious, for historical reasons.
 */
/*
 * Megahack -- who < -999 means 'by something strange'.		- Jir -
 * 
 * NOTE: unlike the note above, 'rad' doesn't seem to be used for damage-
 * reducing purpose, I don't know why.
 */
//static bool project_p(int Ind, int who, int r, struct worldpos *wpos, int y, int x, int dam, int typ, int rad)
static bool project_p(int Ind, int who, int r, struct worldpos *wpos, int y, int x, int dam, int typ, int rad, int flg)
{
	player_type *p_ptr;
	monster_race *r_ptr;

	int k = 0;

	int div;

	/* Hack -- assume obvious */
	bool obvious = TRUE;

	/* Player blind-ness */
	bool blind;

	/* Player needs a "description" (he is blind) */
	bool fuzzy = FALSE;

	/* Player is damaging herself (eg. by shattering potion) */
	bool self = FALSE;

	/* Source monster */
	monster_type *m_ptr;

	/* Monster name (for attacks) */
	char m_name[80];

	/* Monster name (for damage) */
	char killer[80];

	int psi_resists = 0;

	/* Hack -- messages */
	cptr act = NULL;
	
	/* For resist_time: Limit randomization of effect */
	int time_influence_choices;

	/* Bad player number */
	if (Ind <= 0)
		return (FALSE);

	p_ptr = Players[Ind];
	r_ptr = &r_info[p_ptr->body_monster];

	blind = (p_ptr->blind ? TRUE : FALSE);

	/* Player is not here */
	if ((x != p_ptr->px) || (y != p_ptr->py) || (!inarea(wpos,&p_ptr->wpos))) return (FALSE);

	/* Player cannot hurt himself */
	if (0 - who == Ind)
	{
		if (flg & PROJECT_SELF) self = TRUE;
		else return (FALSE);
	}

	/* Mega-Hack -- Players cannot hurt other players */
	if (cfg.use_pk_rules == PK_RULES_NEVER && who <= 0 &&
			who > PROJECTOR_UNUSUAL) return (FALSE);


	/* Store/house is safe */
	if (p_ptr->store_num > -1)
	{
		/* Message */
//		msg_format(Ind, "You are too afraid to attack %s!", p_name);

		/* Done */
		return(FALSE);
	}

	/* Bolt attack from a monster, a player or a trap */
//	if ((!rad) && get_skill(p_ptr, SKILL_DODGE) && (who > 0))
	/* Hack -- HIDE(direct) spell cannot be dodged */
	if (get_skill(p_ptr, SKILL_DODGE) && !(flg & PROJECT_HIDE))
	{
		if ((!rad) && (who > PROJECTOR_POTION))
		{
			//		int chance = (p_ptr->dodge_chance - ((r_info[who].level * 5) / 6)) / 3;
			/* Hack -- let's use 'dam' for now */
			int chance = (p_ptr->dodge_chance - dam / 6) / 3;

			if ((chance > 0) && magik(chance))
			{
//				msg_print(Ind, "You dodge a magical attack!");
				msg_print(Ind, "You dodge the projectile!");
				return (TRUE);
			}
		}
		/* MEGAHACK -- allow to dodge 'bolt' traps */
		else if ((rad < 2) && (who == PROJECTOR_TRAP))
		{
			/* Hack -- let's use 'dam' for now */
			int chance = (p_ptr->dodge_chance - dam / 4) / 4;

			if ((chance > 0) && magik(chance))
			{
				msg_print(Ind, "You dodge a magical attack!");
				return (TRUE);
			}
		}
	}

	/* Reflection */
#if 0
        /* Effects done by the plane cannot bounce */
        if (p_ptr->reflect && !a_rad && !(randint(10)==1) && ((who != -101) && (who != -100)))
	{
                int t_y, t_x;
		int max_attempts = 10;

		if (blind) msg_print("Something bounces!");
		else msg_print("The attack bounces!");

		/* Choose 'new' target */
		do
		{
			t_y = m_list[who].fy - 1 + randint(3);
			t_x = m_list[who].fx - 1 + randint(3);
			max_attempts--;
		}
		while (max_attempts && in_bounds2(t_y, t_x) &&
		     !(player_has_los_bold(t_y, t_x)));

		if (max_attempts < 1)
		{
			t_y = m_list[who].fy;
			t_x = m_list[who].fx;
		}

		project(0, 0, t_y, t_x, dam, typ, (PROJECT_STOP|PROJECT_KILL));

		disturb(1, 0);
		return(TRUE);
	}
#endif	// 0

        /* Effects done by the plane cannot bounce */
        if (p_ptr->reflect && !rad && who != PROJECTOR_POTION && who != PROJECTOR_TERRAIN &&
			rand_int(10) < ((typ == GF_ARROW || typ == GF_MISSILE) ? 7 : 3))

	{
                int t_y, t_x, ay, ax;
		int max_attempts = 10;

		if (blind) msg_print(Ind, "Something bounces!");
		else msg_print(Ind, "The attack bounces!");

		if (who != PROJECTOR_TRAP && who)
		{
			if (who < 0)
			{
				ay = Players[-who]->py;
				ax = Players[-who]->px;
			}
			else
			{
				ay = m_list[who].fy;
				ax = m_list[who].fx;
			}


			/* Choose 'new' target */
			do
			{
				t_y = ay - 1 + randint(3);
				t_x = ax - 1 + randint(3);
				max_attempts--;
			}
#if 0
			while (max_attempts && in_bounds2(wpos, t_y, t_x) &&
					!(player_has_los_bold(Ind, t_y, t_x)));
#else	// 0
			while (max_attempts && (!in_bounds2(wpos, t_y, t_x) ||
					!(player_has_los_bold(Ind, t_y, t_x))));
#endif	// 0

			if (max_attempts < 1)
			{
				t_y = ay;
				t_x = ax;
			}

//			project(0, 0, wpos, t_y, t_x, dam, typ, (PROJECT_STOP|PROJECT_KILL));
			project(0 - Ind, 0, wpos, t_y, t_x, dam, typ, (PROJECT_STOP|PROJECT_KILL));
		}

		disturb(Ind, 1, 0);
		return TRUE;
	}

	/* Extract radius */
	div = r + 1;

	/* Decrease damage */
	dam = dam / div;


	/* Hack -- always do at least one point of damage */
	if (dam <= 0) dam = 1;

	/* Hack -- Never do excessive damage */
	if (dam > 1600) dam = 1600;


	/* If the player is blind, be more descriptive */
	if (blind) fuzzy = TRUE;

	/* If the player is hit by a trap, be more descritive */
	if (who <= PROJECTOR_UNUSUAL) fuzzy = TRUE;

	if (who > 0)
	{
		/* Get the source monster */
		m_ptr = &m_list[who];

		/* Get the monster name */
		monster_desc(Ind, m_name, who, 0);

		/* Get the monster's real name */
		monster_desc(Ind, killer, who, 0x88);
	}
	/* hack -- by trap */
	else if (who == PROJECTOR_TRAP)
	{
		cave_type **zcave;
		cave_type *c_ptr;
		int t_idx = 0;

		if((zcave=getcave(wpos))){
			struct c_special *cs_ptr;
			c_ptr=&zcave[p_ptr->py][p_ptr->px];
			if((cs_ptr=GetCS(c_ptr, CS_TRAPS)))
				t_idx = cs_ptr->sc.trap.t_idx;

			//if(t_idx && t_idx==typ){
			if(t_idx){
				/* huh? */
				// t_ptr = zcave[p_ptr->py][p_ptr->px].special.sc.ptr;
				sprintf(killer, t_name + t_info[t_idx].name);
			}
			else if(c_ptr->o_idx){
				/* Chest (object) trap */
				object_type *o_ptr=&o_list[c_ptr->o_idx];
				sprintf(killer, t_name + t_info[o_ptr->pval].name);
			}
			else{
				/* Hopefully never. */
				/* Actually this can happen if player's not on the trap
				 * (eg. door traps) */
				sprintf(killer, "A mysterious accident");
			}
		}
	}
	/* hack -- by shattering potion */
	else if (who == PROJECTOR_POTION)
	{
		/* TODO: add potion name */
		sprintf(killer, "An evaporating potion");
	}
	/* hack -- by shattering potion */
	else if (who <= PROJECTOR_UNUSUAL)
	{
		/* TODO: add potion name */
		sprintf(killer, "Something weird");
	}
#if 0
	else if (who == PROJECTOR_TERRAIN)
	{
		/* TODO: implement me! */
		sprintf(killer, "A terrain effect");
	}
#endif	// 0
	else if (self)
	{
		sprintf(killer, p_ptr->male ? "himself" : "herself");
	}
	else if (who < 0)
	{
		strcpy(killer, p_ptr->play_vis[0 - who] ? Players[0 - who]->name : "It");

		/* Do not become hostile if it was a healing or teleport spell */
		if ((typ != GF_HEAL_PLAYER) && (typ != GF_AWAY_ALL) &&
			(typ != GF_WRAITH_PLAYER) && (typ != GF_SPEED_PLAYER) &&
			(typ != GF_SHIELD_PLAYER) && (typ != GF_RECALL_PLAYER) &&
			(typ != GF_BLESS_PLAYER) && (typ != GF_REMFEAR_PLAYER) &&
			(typ != GF_SATHUNGER_PLAYER) && (typ != GF_RESFIRE_PLAYER) &&
			(typ != GF_RESCOLD_PLAYER) && (typ != GF_CUREPOISON_PLAYER) &&
			(typ != GF_SEEINVIS_PLAYER) && (typ != GF_SEEMAP_PLAYER) &&
			(typ != GF_CURECUT_PLAYER) && (typ != GF_CURESTUN_PLAYER) &&
			(typ != GF_DETECTCREATURE_PLAYER) && (typ != GF_DETECTDOOR_PLAYER) &&
			(typ != GF_DETECTTRAP_PLAYER) && (typ != GF_TELEPORTLVL_PLAYER) &&
			(typ != GF_RESPOIS_PLAYER) && (typ != GF_RESELEC_PLAYER) &&
			(typ != GF_RESACID_PLAYER) && (typ != GF_HPINCREASE_PLAYER) &&
                        (typ != GF_HERO_PLAYER) && (typ != GF_SHERO_PLAYER) &&
                        (typ != GF_TELEPORT_PLAYER))
		{		
			/* If this was intentional, make target hostile */
			if (check_hostile(0 - who, Ind))
			{
				/* Make target hostile if not already */
				if (!check_hostile(Ind, 0 - who))
				{
					add_hostility(Ind, killer);
				}
			}
			
			/* people not in the same party hit each other */
			if (!player_in_party(Players[0 - who]->party, Ind))
			{			
				/* XXX Reduce damage by 1/3 */
				dam = (dam + 2) / 3;
				if((cfg.use_pk_rules == PK_RULES_DECLARE &&
							!(p_ptr->pkill & PKILL_KILLER)) &&
						!magik(NEUTRAL_FIRE_CHANCE))
					return(FALSE);
			}
			else
			{
				/* Players in the same party can't harm each others */
#if FRIEND_FIRE_CHANCE
				dam = (dam + 2) / 3;
				if (!magik(FRIEND_FIRE_CHANCE))
#endif
				return(FALSE);
			}
		}	
	}


	/* Analyze the damage */
	switch (typ)
	{
		/* Psionics */
		case GF_PSI:
		if (fuzzy) msg_print(Ind, "Your mind is hit by mental energy!");
		if (rand_int(100) < p_ptr->skill_sav) psi_resists++;
		if ((p_ptr->shero) && (rand_int(100) >= p_ptr->skill_sav)) psi_resists--;
		if (p_ptr->confused) psi_resists--;
		if (p_ptr->image) psi_resists--;
		if (p_ptr->stun) psi_resists--;
		if ((p_ptr->afraid) && (rand_int(100) >= p_ptr->skill_sav)) psi_resists--;

		if (psi_resists > 0)
		{
			/* No side effects */

			/* Reduce damage */
			for (k = 0; k < psi_resists ; k++)
			{
				dam = dam * 4 / (4 + randint(5));
			}			
					
		}
		else
		{
			/* Unresisted */	
			if (!p_ptr->resist_conf) set_confused(Ind, p_ptr->confused + rand_int(20) + 10);
		}
			
		if (psi_resists > 0)
		{
			/* Telepathy reduces damage */
			if (p_ptr->telepathy) dam = dam * (3 + randint(6)) / 9;
		}
		else
		{
			/* Telepathy increases damage */
			if (p_ptr->telepathy) dam = dam * (6 + randint(6)) / 6;
		}

					
		take_hit(Ind, dam, killer);
   		break;


		/* Standard damage -- hurts inventory too */
		case GF_ACID:
		if (fuzzy) msg_print(Ind, "You are hit by acid!");
		acid_dam(Ind, dam, killer);
		break;

		/* Standard damage -- hurts inventory too */
		case GF_FIRE:
		if (fuzzy) msg_print(Ind, "You are hit by fire!");
		fire_dam(Ind, dam, killer);
		break;

		/* Standard damage -- hurts inventory too */
		case GF_COLD:
		if (fuzzy) msg_print(Ind, "You are hit by cold!");
		cold_dam(Ind, dam, killer);
		break;

		/* Standard damage -- hurts inventory too */
		case GF_ELEC:
		if (fuzzy) msg_print(Ind, "You are hit by lightning!");
		elec_dam(Ind, dam, killer);
		break;

		/* Standard damage -- also poisons player */
		case GF_POIS:
		if (fuzzy) msg_print(Ind, "You are hit by poison!");
		if (p_ptr->immune_poison)
		{
		    dam = 0;
		}
		else
		{
			if (p_ptr->resist_pois) dam = (dam + 2) / 3;
			if (p_ptr->oppose_pois) dam = (dam + 2) / 3;
			take_hit(Ind, dam, killer);
			if (!(p_ptr->resist_pois || p_ptr->oppose_pois))
			{
				(void)set_poisoned(Ind, p_ptr->poisoned + rand_int(dam) + 10);
			}
		}
		break;

		/* Standard damage */
		case GF_MISSILE:
		if (fuzzy) msg_print(Ind, "You are hit by something!");
		if (p_ptr->biofeedback) dam /= 2;
		take_hit(Ind, dam, killer);
		break;

		/* Holy Orb -- Player only takes partial damage */
		case GF_HOLY_ORB:
		case GF_HOLY_FIRE:
//		case GF_HELL_FIRE:
		if (fuzzy) msg_print(Ind, "You are hit by something!");
		if (p_ptr->body_monster && r_ptr->flags3 & RF3_EVIL) dam *= 2;
		else dam /= 2;
		take_hit(Ind, dam, killer);
		break;

		/* Arrow -- XXX no dodging */
		case GF_ARROW:
		if (fuzzy) msg_print(Ind, "You are hit by something sharp!");
		if (p_ptr->biofeedback) dam /= 2;
		take_hit(Ind, dam, killer);
		break;

		/* Plasma -- XXX Fire helps a bit */
		case GF_PLASMA:
		if (fuzzy) msg_print(Ind, "You are hit by something!");
		if (p_ptr->resist_fire) {
		    dam *= 3;
		    dam /= 5;
		}
		take_hit(Ind, dam, killer);
		if (!p_ptr->resist_sound)
		{
			int k = (randint((dam > 40) ? 35 : (dam * 3 / 4 + 5)));
			(void)set_stun(Ind, p_ptr->stun + k);
		}
		break;

		/* Nether -- drain experience */
		case GF_NETHER:
		if (fuzzy) msg_print(Ind, "You are hit by something strange!");
		if (p_ptr->immune_neth)
		{
			dam = 0;
		}
		else if (p_ptr->resist_neth)
		{
			dam *= 6; dam /= (randint(6) + 6);
		}
		else
		{
			if (p_ptr->hold_life && (rand_int(100) < 75))
			{
				msg_print(Ind, "You keep hold of your life force!");
			}
			else if (p_ptr->hold_life)
			{
				msg_print(Ind, "You feel your life slipping away!");
				lose_exp(Ind, 200 + (p_ptr->exp/1000) * MON_DRAIN_LIFE);
			}
			else
			{
				msg_print(Ind, "You feel your life draining away!");
				lose_exp(Ind, 200 + (p_ptr->exp/100) * MON_DRAIN_LIFE);
			}
		}
		take_hit(Ind, dam, killer);
		break;

		/* Water -- stun/confuse */
		case GF_WATER:
		if (fuzzy) msg_print(Ind, "You are hit by something!");
		if (p_ptr->body_monster && r_ptr->flags7 && RF7_AQUATIC)
		{
//			dam = (dam + 8) / 9;
			dam = (dam + 3) / 4;
		}
		if (p_ptr->immune_water)
		{
		    dam = 0;
		}
		else
		{
			if (p_ptr->resist_water)
			{
			    dam = (dam + 2) / 3;
			}
			else
			{
	    			if (!p_ptr->resist_sound)
				{
					(void)set_stun(Ind, p_ptr->stun + randint(40));
				}
				if (!p_ptr->resist_conf)
				{
					(void)set_confused(Ind, p_ptr->confused + randint(5) + 5);
				}
			}
			if (TOOL_EQUIPPED(p_ptr) != SV_TOOL_TARPAULIN && magik(20 + dam / 20))
			{
				inven_damage(Ind, set_water_destroy, 1);
				if (magik(20)) minus_ac(Ind, 1);
			}
			take_hit(Ind, dam, killer);
		}
		break;

		/* Stun */
		case GF_STUN:
		if (fuzzy) msg_print(Ind, "You are hit by something!");
		if (!p_ptr->resist_sound)
		{
			(void)set_stun(Ind, p_ptr->stun + randint(40));
			dam = 0;
		}
		break;

		/* Chaos -- many effects */
		case GF_CHAOS:
		if (fuzzy) msg_print(Ind, "You are hit by something strange!");
		if (p_ptr->resist_chaos)
		{
			dam *= 6; dam /= (randint(6) + 6);
		}
		if (!p_ptr->resist_conf)
		{
			(void)set_confused(Ind, p_ptr->confused + rand_int(20) + 10);
		}
		if (!p_ptr->resist_chaos)
		{
			(void)set_image(Ind, p_ptr->image + randint(10));
		}
		if (!p_ptr->resist_neth && !p_ptr->resist_chaos)
		{
			if (p_ptr->hold_life && (rand_int(100) < 75))
			{
				msg_print(Ind, "You keep hold of your life force!");
			}
			else if (p_ptr->hold_life)
			{
				msg_print(Ind, "You feel your life slipping away!");
				lose_exp(Ind, 500 + (p_ptr->exp/1000) * MON_DRAIN_LIFE);
			}
			else
			{
				msg_print(Ind, "You feel your life draining away!");
				lose_exp(Ind, 5000 + (p_ptr->exp/100) * MON_DRAIN_LIFE);
			}
		}
		take_hit(Ind, dam, killer);
		break;

		/* Shards -- mostly cutting */
		case GF_SHARDS:
		if (fuzzy) msg_print(Ind, "You are hit by something sharp!");
		if (p_ptr->biofeedback) dam /= 2;
		if (p_ptr->resist_shard)
		{
			dam *= 6; dam /= (randint(6) + 6);
		}
		else
		{
			(void)set_cut(Ind, p_ptr->cut + dam);
		}
		take_hit(Ind, dam, killer);
		break;

		/* Sound -- mostly stunning */
		case GF_SOUND:
		if (fuzzy) msg_print(Ind, "You are hit by something!");
		if (p_ptr->biofeedback) dam /= 2;
		if (p_ptr->resist_sound)
		{
			dam *= 5; dam /= (randint(6) + 6);
		}
		else
		{
			int k = (randint((dam > 90) ? 35 : (dam / 3 + 5)));
			(void)set_stun(Ind, p_ptr->stun + k);
		}
		take_hit(Ind, dam, killer);
		break;

		/* Pure confusion */
		case GF_CONFUSION:
		if (fuzzy) msg_print(Ind, "You are hit by something!");
		if (p_ptr->resist_conf)
		{
			dam *= 5; dam /= (randint(6) + 6);
		}
		else
		{
			(void)set_confused(Ind, p_ptr->confused + randint(20) + 10);
		}
		take_hit(Ind, dam, killer);
		break;

		/* Disenchantment -- see above */
		case GF_DISENCHANT:
		if (fuzzy) msg_print(Ind, "You are hit by something strange!");
		if (p_ptr->resist_disen)
		{
			dam *= 6; dam /= (randint(6) + 6);
		}
		else
		{
			(void)apply_disenchant(Ind, 0);
		}
		take_hit(Ind, dam, killer);
		break;

		/* Nexus -- see above */
		case GF_NEXUS:
		if (fuzzy) msg_print(Ind, "You are hit by something strange!");
		if (p_ptr->resist_nexus)
		{
			dam *= 6; dam /= (randint(6) + 6);
		}
		else
		{
//			apply_nexus(Ind, m_ptr);
			apply_nexus(Ind, NULL);
		}
		take_hit(Ind, dam, killer);
		break;

		/* Force -- mostly stun */
		case GF_FORCE:
		if (fuzzy) msg_print(Ind, "You are hit by something!");
		if (!p_ptr->resist_sound)
		{
			(void)set_stun(Ind, p_ptr->stun + randint(20));
		}
		take_hit(Ind, dam, killer);
		break;

		/* Inertia -- slowness */
		case GF_INERTIA:
		if (fuzzy) msg_print(Ind, "You are hit by something strange!");
		if (!p_ptr->free_act)
		{
			(void)set_slow(Ind, p_ptr->slow + rand_int(4) + 4);
		}
		else
		{
			(void)set_slow(Ind, p_ptr->slow + rand_int(3) + 1);
		}
		take_hit(Ind, dam, killer);
		break;

		/* Lite -- blinding */
		case GF_LITE:
		if (fuzzy) msg_print(Ind, "You are hit by something!");
		if (p_ptr->resist_lite)
		{
			dam *= 4; dam /= (randint(6) + 6);
		}
		else if (!blind && !p_ptr->resist_blind)
		{
			(void)set_blind(Ind, p_ptr->blind + randint(5) + 2);
		}
		if (p_ptr->body_monster && r_ptr->flags3 & RF3_HURT_LITE)
		{
			dam *= 2;
		}
		take_hit(Ind, dam, killer);
		break;

		/* Dark -- blinding */
		case GF_DARK:
		if (fuzzy) msg_print(Ind, "You are hit by something!");
		if (p_ptr->resist_dark)
		{
			dam *= 4; dam /= (randint(6) + 6);
		}
		else if (!blind && !p_ptr->resist_blind)
		{
			(void)set_blind(Ind, p_ptr->blind + randint(5) + 2);
		}
		take_hit(Ind, dam, killer);
		break;

		/* Time -- bolt fewer effects XXX */
		case GF_TIME:
		if (fuzzy) msg_print(Ind, "You are hit by something strange!");
		if (p_ptr->resist_time)
		{
			time_influence_choices = randint(9);
		}
		else
		{
			time_influence_choices = randint(10);
		}
		switch (time_influence_choices)
		{
			case 1: case 2: case 3: case 4: case 5:
			msg_print(Ind, "You feel life has clocked back.");
			if (p_ptr->resist_time)
			{
				lose_exp(Ind, (p_ptr->exp / 100) * MON_DRAIN_LIFE / 4);
			}
			else
			{
				lose_exp(Ind, 100 + (p_ptr->exp / 100) * MON_DRAIN_LIFE);
			}
			break;

			case 6: case 7: case 8: case 9:

			switch (randint(6))
			{
				case 1: k = A_STR; act = "strong"; break;
				case 2: k = A_INT; act = "bright"; break;
				case 3: k = A_WIS; act = "wise"; break;
				case 4: k = A_DEX; act = "agile"; break;
				case 5: k = A_CON; act = "hale"; break;
				case 6: k = A_CHR; act = "beautiful"; break;
			}

			msg_format(Ind, "You're not as %s as you used to be...", act);

			if (!p_ptr->resist_time)
			{
				p_ptr->stat_cur[k] = (p_ptr->stat_cur[k] * 3) / 4;
			}
			else
			{
				p_ptr->stat_cur[k] = (p_ptr->stat_cur[k] * 6) / 7;
			}
			if (p_ptr->stat_cur[k] < 3) p_ptr->stat_cur[k] = 3;
			p_ptr->update |= (PU_BONUS);
			break;

			case 10:

			msg_print(Ind, "You're not as powerful as you used to be...");

			for (k = 0; k < 6; k++)
			{
				if (!p_ptr->resist_time)
				{
					p_ptr->stat_cur[k] = (p_ptr->stat_cur[k] * 3) / 4;
				}
				else
				{
					p_ptr->stat_cur[k] = (p_ptr->stat_cur[k] * 7) / 8;
				}
				if (p_ptr->stat_cur[k] < 3) p_ptr->stat_cur[k] = 3;
			}
			p_ptr->update |= (PU_BONUS);
			break;
		}
		if (p_ptr->resist_time) dam /= 3;
		take_hit(Ind, dam, killer);
		break;

		/* Gravity -- stun plus slowness plus teleport */
		case GF_GRAVITY:
		if (fuzzy) msg_print(Ind, "You are hit by something strange!");
		msg_print(Ind, "Gravity warps around you.");
		teleport_player(Ind, 5);
		(void)set_slow(Ind, p_ptr->slow + rand_int(4) + 3);
		if (!p_ptr->resist_sound)
		{
			int k = (randint((dam > 90) ? 35 : (dam / 3 + 5)));
			(void)set_stun(Ind, p_ptr->stun + k);
		}
		/* Feather fall lets us resist gravity */
		if (p_ptr->feather_fall)
		{
			dam *= 6; dam /= (randint(6) + 6);
		}
		take_hit(Ind, dam, killer);
		break;

		/* Pure damage */
		case GF_MANA:
		if (fuzzy) msg_print(Ind, "You are hit by something!");
		if (p_ptr->resist_mana) dam /= 3;
		take_hit(Ind, dam, killer);
		break;

		/* Pure damage */
		case GF_METEOR:
		if (fuzzy) msg_print(Ind, "You are hit by something!");
		take_hit(Ind, dam, killer);
		break;

		/* Ice -- cold plus stun plus cuts */
		case GF_ICE:
		if (fuzzy) msg_print(Ind, "You are hit by something sharp!");
		cold_dam(Ind, dam, killer);
		if (!p_ptr->resist_shard)
		{
			(void)set_cut(Ind, p_ptr->cut + damroll(5, 8));
		}
		if (!p_ptr->resist_sound)
		{
			(void)set_stun(Ind, p_ptr->stun + randint(15));
		}
		break;

		/* Teleport other -- Teleports */
		case GF_AWAY_ALL:
		{
		if (fuzzy) msg_print(Ind, "You feel you are somewhere else.");
		else msg_format(Ind, "%^s teleports you away.", killer);
		teleport_player(Ind, 200);
		break;
		}

		case GF_WRAITH_PLAYER:		
		{
			if (fuzzy) msg_print(Ind, "You feel less constant!");
			else msg_format(Ind, "%^s turns you into a wraith!", killer);
		
			if (!p_ptr->tim_wraith)
			{
				set_tim_wraith(Ind, dam);
			}
			else
			{
				(void)set_tim_wraith(Ind, p_ptr->tim_wraith + randint(dam) / 2 + 1);
			}
			//set_tim_wraith(Ind, p_ptr->tim_wraith + dam);
			break;
                }

                case GF_BLESS_PLAYER:
                {
			if (dam < 25)
			{
				p_ptr->blessed_power = 10;
			}

			if ((dam > 25) && (dam < 49))
			{
				p_ptr->blessed_power = 20;
			}

			if (dam > 48)
			{
				p_ptr->blessed_power = 30;
			}	
                        (void)set_blessed(Ind, dam);
                        break;
                }
	
                case GF_REMFEAR_PLAYER:
                {
                        set_afraid(Ind, 0);
                        break;
		}

                case GF_HPINCREASE_PLAYER:
                {
			(void)hp_player(Ind, dam);
                        break;
		}

                case GF_HERO_PLAYER:
                {
			(void)set_hero(Ind, p_ptr->hero + dam);
                        break;
		}

                case GF_SHERO_PLAYER:
                {
			(void)set_shero(Ind, p_ptr->hero + dam);
                        break;
		}

                case GF_SATHUNGER_PLAYER:
                {
			msg_format_near(Ind, "\377R%s looks like he is going to explode.", p_ptr->name);
                        (void)set_food(Ind, PY_FOOD_MAX - 1);
                        break;
		}

		case GF_RESFIRE_PLAYER:
		{
			(void)set_oppose_fire(Ind, p_ptr->oppose_fire + dam);
			break;
		}

		case GF_RESELEC_PLAYER:
		{
			(void)set_oppose_elec(Ind, p_ptr->oppose_elec + dam);
			break;
		}

		case GF_RESPOIS_PLAYER:
		{
			(void)set_oppose_pois(Ind, p_ptr->oppose_pois + dam);
			break;
		}

		case GF_RESACID_PLAYER:
		{
			(void)set_oppose_acid(Ind, p_ptr->oppose_acid + dam);
			break;
		}
	
		case GF_RESCOLD_PLAYER:
		{
                        (void)set_oppose_cold(Ind, p_ptr->oppose_cold + dam);
			break;
		}

		case GF_CUREPOISON_PLAYER:
		{
                        (void)set_poisoned(Ind, 0);
			break;
		}

		case GF_SEEINVIS_PLAYER:
		{
			(void)set_tim_invis(Ind, p_ptr->tim_invis + dam);
			break;
		}

		case GF_SEEMAP_PLAYER:
		{
                        map_area(Ind);
			break;
		}
			
		case GF_DETECTCREATURE_PLAYER:
		{
 			(void)detect_creatures(Ind);
			break;
		}

		case GF_CURESTUN_PLAYER:
		{
                        (void)set_stun(Ind, p_ptr->stun - dam);
			break;
		}

		case GF_DETECTDOOR_PLAYER:
		{
                        (void)detect_sdoor(Ind, DEFAULT_RADIUS);
			break;
		}

		case GF_DETECTTRAP_PLAYER:
		{
                        (void)detect_trap(Ind, DEFAULT_RADIUS);
			break;
		}

		case GF_CURECUT_PLAYER:
		{
                        (void)set_cut(Ind, p_ptr->cut - dam);
			break;
		}

		case GF_TELEPORTLVL_PLAYER:
		{
                        teleport_player_level(Ind);
			break;
		}

		case GF_HEAL_PLAYER:
		{
			msg_format(Ind, "\377gYou have been healed for %ld damage",dam);
                        msg_format_near(Ind, "\377g%s has been healed for %ld damage!.", p_ptr->name, dam);

			(void)hp_player(Ind, dam);
			break;
		}

		case GF_SPEED_PLAYER:		
		{
			if (fuzzy) msg_print(Ind, "You feel faster!");
			else msg_format(Ind, "%^s speeds you up!", killer);
		
			if (!p_ptr->fast)
			{
                                (void)set_fast(Ind, dam, 10);
			}
			else
			{
                                (void)set_fast(Ind, p_ptr->fast + (dam / 5), 10);
			}
			break;
                }
		case GF_SHIELD_PLAYER:
		{
			if (fuzzy) msg_print(Ind, "You feel protected!");
			else msg_format(Ind, "%^s shields you!", killer);
		
                	if (!p_ptr->shield)
                        	(void)set_shield(Ind, dam, 50, SHIELD_NONE, 0, 0);
                 	else
                        	(void)set_shield(Ind, p_ptr->shield + (dam / 5), 50, SHIELD_NONE, 0, 0);
			break;
                }
		case GF_TELEPORT_PLAYER:
		{
			if (fuzzy) msg_print(Ind, "You feel translocated!");
			else msg_format(Ind, "%^s teleports you!", killer);

                        teleport_player(Ind, dam);
			break;
                }
		case GF_RECALL_PLAYER:
		{
			if (!p_ptr->word_recall)
			{
				p_ptr->recall_pos.wz = p_ptr->max_dlv;
				p_ptr->word_recall = dam;
				if (fuzzy) msg_print(Ind, "You feel unstable!");
				else msg_format(Ind, "%^s recalls you!", killer);
			}
			else
			{
				p_ptr->word_recall = 0;
				if (fuzzy) msg_print(Ind, "You feel more stable!");
				else msg_format(Ind, "%^s stops your recall!", killer);
			}
				p_ptr->redraw |= (PR_DEPTH);
				dam = 0;
			break;
                }

		case GF_OLD_CONF:
		
		if (fuzzy || self) msg_print(Ind, "You hear puzzling noises!");
		else msg_format(Ind, "%^s creates a mesmerising illusion!", killer);
		
		if (p_ptr->resist_conf)
		{
			msg_print(Ind, "You disbelieve the feeble spell.");
		}
		else if (rand_int(100 + dam*6) < p_ptr->skill_sav)
		{
			msg_print(Ind, "You disbelieve the feeble spell.");
		}
		else set_confused(Ind, p_ptr->confused + dam);
		
		break;
		
		case GF_OLD_SLOW:
		
		if (fuzzy || self) msg_print(Ind, "Something drains power from your muscles!");
		else msg_format(Ind, "%^s drains power from your muscles!", killer);
		
		if (p_ptr->free_act)
		{
			msg_print(Ind, "You are unaffected!");
		}
		else if (rand_int(100 + dam*6) < p_ptr->skill_sav)
		{
			msg_print(Ind, "You resist the effects!");
		}
		else set_slow(Ind, p_ptr->slow + dam);
		
		break;
		
		case GF_TURN_ALL:
		
		if (fuzzy || self) msg_print(Ind, "Something mumbles, and you hear scary noises!");
		else msg_format(Ind, "%^s casts a fearful illusion!", killer);
		
		if (p_ptr->resist_fear)
		{
			msg_print(Ind, "You refuse to be frightened.");
		}
		else if (rand_int(100 + dam*6) < p_ptr->skill_sav)
		{
			msg_print(Ind, "You refuse to be frightened.");
		}
		else
		{
			(void)set_afraid(Ind, p_ptr->afraid + dam);
		}
		
		break;
		
		case GF_OLD_POLY:
		
		if (fuzzy || self) msg_print(Ind, "You feel bizzare!");
		else msg_format(Ind, "%^s polymorphs you!", killer);
		if (p_ptr->resist_nexus)
		{
			msg_print(Ind, "You resist the effects!");
		}
		else
		{
			msg_print(Ind, "The magic continuum twists around you!");
			apply_morph(Ind, dam, killer);
		}
		break;
		
		/* PernA ones */
		/* Rocket -- stun, cut */
		case GF_ROCKET:
		{
			if (fuzzy) msg_print(Ind, "There is an explosion!");
			if (!p_ptr->resist_sound)
			{
				(void)set_stun(Ind, p_ptr->stun + randint(20));
			}
			if (p_ptr->resist_shard)
			{
				dam /= 2;
			}
			else
			{
				(void)set_cut(Ind, p_ptr->  cut + ( dam / 2) );
			}

			if (((!p_ptr->resist_shard) || (!p_ptr->resist_fire)) || (randint(3)==1))
			if (((!p_ptr->resist_shard) && (!p_ptr->resist_fire)) || (randint(4)==1))
			{
				inven_damage(Ind, set_cold_destroy, 3);
			}

			take_hit(Ind, dam, killer);
			break;
		}

		/* Standard damage -- also poisons / mutates player */
		case GF_NUKE:
		{
			if (fuzzy) msg_print(Ind, "You are hit by radiation!");
			if (p_ptr->immune_poison)
			{
			    dam = 0;
			}
			else
			{
				if (p_ptr->resist_pois) dam = (2 * dam + 2) / 5;
				if (p_ptr->oppose_pois) dam = (2 * dam + 2) / 5;
				take_hit(Ind, dam, killer);
				if (!(p_ptr->resist_pois || p_ptr->oppose_pois))
				{
					set_poisoned(Ind, p_ptr->poisoned + rand_int(dam) + 10);

#if 0	// dang, later..
					if (randint(5)==1) /* 6 */
					{
						msg_print("You undergo a freakish metamorphosis!");
						if (randint(4)==1) /* 4 */
							do_poly_self();
						else
                            	                    corrupt_player();
					}
#endif	// 0

					if (randint(6)==1)
					{
						inven_damage(Ind, set_acid_destroy, 2);
					}
				}
			}
			break;
		}

		/* Standard damage */
		case GF_DISINTEGRATE:
		{
			if (fuzzy) msg_print(Ind, "You are hit by pure energy!");
			take_hit(Ind, dam, killer);
			break;
		}

		/* RF5_BLIND */
		case GF_BLIND:
		{
			if (p_ptr->resist_blind)
			{
				msg_print(Ind, "You are unaffected!");
			}
			else if (rand_int(100 + dam*6) < p_ptr->skill_sav)
			{
				msg_print(Ind, "You resist the effects!");
			}
			else if (!p_ptr->blind)
			{
				(void)set_blind(Ind, 12 + rand_int(4));
			}
			break;
		}

		/* For shattering potions, but maybe work for wands too? */
		case GF_OLD_HEAL:
		{
			if (fuzzy) msg_print(Ind, "You are hit by something invigorating!");
			(void)hp_player(Ind, dam);
			dam = 0;
			break;
		}

		case GF_OLD_SPEED:
		{
			if (fuzzy) msg_print(Ind, "You are hit by something!");
			(void)set_fast(Ind, p_ptr->fast + randint(5), 10);
			dam = 0;
			break;
		}

		case GF_TELE_TO:
		{
			bool resists_tele = FALSE;
			dun_level		*l_ptr = getfloor(wpos);

			/* Teleport to nowhere..? */
			if (who >=0 || who <= PROJECTOR_UNUSUAL) break;

			if (p_ptr->anti_tele)
			{
				msg_print(Ind, "You are unaffected!");
			}
			else
			{
				player_type *q_ptr = Players[0 - who];

				msg_format(Ind, "%^s commands you to return.", q_ptr->name);

				/* Prepare to teleport */
				teleport_player_to(Ind, q_ptr->py, q_ptr->px);

			}

			/* No "real" damage */
			dam = 0;
			break;
		}

		/* Hand of Doom */
		case GF_HAND_DOOM:
		{
			/* Teleport to nowhere..? */
			if (who >=0 || who <= PROJECTOR_UNUSUAL) break;

			msg_format(Ind, "%^s invokes the Hand of Doom!",
					Players[0-who]->name);

			if (rand_int(100) < p_ptr->skill_sav)
			{
				msg_format(Ind, "You resist the effects!");
			}
			else
			{
				int dummy = (((s32b) ((65 + randint(25)) * (p_ptr->chp))) / 100);
				if (p_ptr->chp - dummy < 1) dummy = p_ptr->chp - 1;
				msg_print(Ind, "You feel your life fade away!");
				bypass_invuln = TRUE;
				take_hit(Ind, dummy, Players[0-who]->name);
				bypass_invuln = FALSE;
				curse_equipment(Ind, 100, 20);
			}
			break;
		}

		/* Default */
		default:

		/* No damage */
		dam = 0;

		break;

	}


	/* Skip non-connected players */
	if (!p_ptr->conn == NOT_CONNECTED)
	{
		/* Disturb */
		disturb(Ind, 1, 0);
	}

	/* Return "Anything seen?" */
	return (obvious);
}









/*
 * Find the char to use to draw a moving bolt
 * It is moving (or has moved) from (x,y) to (nx,ny).
 * If the distance is not "one", we (may) return "*".
 */
static char bolt_char(int y, int x, int ny, int nx)
{
	if ((ny == y) && (nx == x)) return '*';
	if (ny == y) return '-';
	if (nx == x) return '|';
	if ((ny-y) == (x-nx)) return '/';
	if ((ny-y) == (nx-x)) return '\\';
	return '*';
}



/*
 * Generic "beam"/"bolt"/"ball" projection routine.  -BEN-
 *
 * Input:
 *   who: Index of "source" monster (or "zero" if "player")
 *   rad: Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 *   y,x: Target location (or location to travel "towards")
 *   dam: Base damage roll to apply to affected monsters (or player)
 *   typ: Type of damage to apply to monsters (and objects)
 *   flg: Extra bit flags (see PROJECT_xxxx in "defines.h")
  *
 * Return:
 *   TRUE if any "effects" of the projection were observed, else FALSE
 *
 * Allows a monster (or player) to project a beam/bolt/ball of a given kind
 * towards a given location (optionally passing over the heads of interposing
 * monsters), and have it do a given amount of damage to the monsters (and
 * optionally objects) within the given radius of the final location.
 *
 * A "bolt" travels from source to target and affects only the target grid.
 * A "beam" travels from source to target, affecting all grids passed through.
 * A "ball" travels from source to the target, exploding at the target, and
 *   affecting everything within the given radius of the target location.
 *
 * Traditionally, a "bolt" does not affect anything on the ground, and does
 * not pass over the heads of interposing monsters, much like a traditional
 * missile, and will "stop" abruptly at the "target" even if no monster is
 * positioned there, while a "ball", on the other hand, passes over the heads
 * of monsters between the source and target, and affects everything except
 * the source monster which lies within the final radius, while a "beam"
 * affects every monster between the source and target, except for the casting
 * monster (or player), and rarely affects things on the ground.
 *
 * Two special flags allow us to use this function in special ways, the
 * "PROJECT_HIDE" flag allows us to perform "invisible" projections, while
 * the "PROJECT_JUMP" flag allows us to affect a specific grid, without
 * actually projecting from the source monster (or player).
 *
 * The player will only get "experience" for monsters killed by himself
 * Unique monsters can only be destroyed by attacks from the player
 *
 * Only 256 grids can be affected per projection, limiting the effective
 * "radius" of standard ball attacks to nine units (diameter nineteen).
 *
 * One can project in a given "direction" by combining PROJECT_THRU with small
 * offsets to the initial location (see "line_spell()"), or by calculating
 * "virtual targets" far away from the player.
 *
 * One can also use PROJECT_THRU to send a beam/bolt along an angled path,
 * continuing until it actually hits somethings (useful for "stone to mud").
 *
 * Bolts and Beams explode INSIDE walls, so that they can destroy doors.
 *
 * Balls must explode BEFORE hitting walls, or they would affect monsters
 * on both sides of a wall.  Some bug reports indicate that this is still
 * happening in 2.7.8 for Windows, though it appears to be impossible.
 *
 * We "pre-calculate" the blast area only in part for efficiency.
 * More importantly, this lets us do "explosions" from the "inside" out.
 * This results in a more logical distribution of "blast" treasure.
 * It also produces a better (in my opinion) animation of the explosion.
 * It could be (but is not) used to have the treasure dropped by monsters
 * in the middle of the explosion fall "outwards", and then be damaged by
 * the blast as it spreads outwards towards the treasure drop location.
 *
 * Walls and doors are included in the blast area, so that they can be
 * "burned" or "melted" in later versions.
 *
 * This algorithm is intended to maximize simplicity, not necessarily
 * efficiency, since this function is not a bottleneck in the code.
 *
 * We apply the blast effect from ground zero outwards, in several passes,
 * first affecting features, then objects, then monsters, then the player.
 * This allows walls to be removed before checking the object or monster
 * in the wall, and protects objects which are dropped by monsters killed
 * in the blast, and allows the player to see all affects before he is
 * killed or teleported away.  The semantics of this method are open to
 * various interpretations, but they seem to work well in practice.
 *
 * We process the blast area from ground-zero outwards to allow for better
 * distribution of treasure dropped by monsters, and because it provides a
 * pleasing visual effect at low cost.
 *
 * Note that the damage done by "ball" explosions decreases with distance.
 * This decrease is rapid, grids at radius "dist" take "1/dist" damage.
 *
 * Notice the "napalm" effect of "beam" weapons.  First they "project" to
 * the target, and then the damage "flows" along this beam of destruction.
 * The damage at every grid is the same as at the "center" of a "ball"
 * explosion, since the "beam" grids are treated as if they ARE at the
 * center of a "ball" explosion.
 *
 * Currently, specifying "beam" plus "ball" means that locations which are
 * covered by the initial "beam", and also covered by the final "ball", except
 * for the final grid (the epicenter of the ball), will be "hit twice", once
 * by the initial beam, and once by the exploding ball.  For the grid right
 * next to the epicenter, this results in 150% damage being done.  The center
 * does not have this problem, for the same reason the final grid in a "beam"
 * plus "bolt" does not -- it is explicitly removed.  Simply removing "beam"
 * grids which are covered by the "ball" will NOT work, as then they will
 * receive LESS damage than they should.  Do not combine "beam" with "ball".
 *
 * The array "gy[],gx[]" with current size "grids" is used to hold the
 * collected locations of all grids in the "blast area" plus "beam path".
 *
 * Note the rather complex usage of the "gm[]" array.  First, gm[0] is always
 * zero.  Second, for N>1, gm[N] is always the index (in gy[],gx[]) of the
 * first blast grid (see above) with radius "N" from the blast center.  Note
 * that only the first gm[1] grids in the blast area thus take full damage.
 * Also, note that gm[rad+1] is always equal to "grids", which is the total
 * number of blast grids.
 *
 * Note that once the projection is complete, (y2,x2) holds the final location
 * of bolts/beams, and the "epicenter" of balls.
 *
 * Note also that "rad" specifies the "inclusive" radius of projection blast,
 * so that a "rad" of "one" actually covers 5 or 9 grids, depending on the
 * implementation of the "distance" function.  Also, a bolt can be properly
 * viewed as a "ball" with a "rad" of "zero".
 *
 * Note that if no "target" is reached before the beam/bolt/ball travels the
 * maximum distance allowed (MAX_RANGE), no "blast" will be induced.  This
 * may be relevant even for bolts, since they have a "1x1" mini-blast.
 *
 * Some people have requested an "auto-explode ball attacks at max range"
 * option, which should probably be handled by this function.  XXX XXX XXX
 *
 * Note that for consistency, we "pretend" that the bolt actually takes "time"
 * to move from point A to point B, even if the player cannot see part of the
 * projection path.  Note that in general, the player will *always* see part
 * of the path, since it either starts at the player or ends on the player.
 *
 * Hack -- we assume that every "projection" is "self-illuminating".
 */
bool project(int who, int rad, struct worldpos *wpos, int y, int x, int dam, int typ, int flg)
{
	int			i, j, t;
	int                 y1, x1, y2, x2;
	int			/*y0, x0,*/ y9, x9;
	int			dist;
	int y_saver, x_saver; /* For reflecting monsters */
	int dist_hack = 0;
	int			who_can_see[26], num_can_see = 0;
	bool old_tacit = suppress_message;

	/* Affected location(s) */
	cave_type *c_ptr;

	/* Assume the player sees nothing */
	bool notice = FALSE;

	/* Assume the player has seen nothing */
	/*bool visual = FALSE;*/

	/* Assume the player has seen no blast grids */
	bool drawn = FALSE;

	/* Is the player blind? */
	/* Blindness is currently ignored for this function */
	/*bool blind;*/

	/* Number of grids in the "blast area" (including the "beam" path) */
	int grids = 0;

	int effect = 0;

	/* Coordinates of the affected grids */
//	byte gx[256], gy[256];
//	byte gx[tdi[PREPARE_RADIUS]], gy[tdi[PREPARE_RADIUS]];
	byte gx[512], gy[512];

	/* Encoded "radius" info (see above) */
//	byte gm[16];
	byte gm[PREPARE_RADIUS];

	dun_level *l_ptr;
	cave_type **zcave;
	if(!(zcave=getcave(wpos)))  return(FALSE);
	l_ptr = getfloor(wpos);

#ifdef PROJECTION_FLUSH_LIMIT
	count_project++;
#endif	// PROJECTION_FLUSH_LIMIT

	/* Location of player */
	/*y0 = py;
	x0 = px;*/


	/* Hack -- Jump to target */
	if (flg & PROJECT_JUMP)
	{
		x1 = x;
		y1 = y;

		/* Clear the flag (well, needed?) */
//		flg &= ~(PROJECT_JUMP);
	}

	/* Hack -- Start at a player */
	else if (who < 0 && who > PROJECTOR_UNUSUAL)
	{
		x1 = Players[0 - who]->px;
		y1 = Players[0 - who]->py;
	}

	/* Start at a monster */
	else if (who > 0)
	{
		x1 = m_list[who].fx;
		y1 = m_list[who].fy;
	}
#if 1
	/* Oops */
	else
	{
		x1 = x;
		y1 = y;
	}
#endif	/* 0 */

	y_saver = y1;
	x_saver = x1;

	/* Default "destination" */
	y2 = y; x2 = x;


	/* Hack -- verify stuff */
	if (flg & PROJECT_THRU)
	{
		if ((x1 == x2) && (y1 == y2))
		{
			flg &= ~PROJECT_THRU;
		}
	}


	/* Hack -- Assume there will be no blast (max radius 16) */
//	for (dist = 0; dist < 16; dist++) gm[dist] = 0;
	for (dist = 0; dist <= PREPARE_RADIUS; dist++) gm[dist] = 0;


	/* Hack -- Handle stuff */
	/*handle_stuff();*/


	/* Start at the source */
	x = x9 = x1;
	y = y9 = y1;
	dist = 0;

	/* Project until done */
	while (1)
	{
		/* Gather beam grids */
		if (flg & PROJECT_BEAM)
		{
			gy[grids] = y;
			gx[grids] = x;
			grids++;
#if DEBUG_LEVEL > 1
			if(grids>500) s_printf("grids %d\n", grids);
#endif	// DEBUG_LEVEL
		}

		/* Check the grid */
		c_ptr = &zcave[y][x];

		/* XXX XXX Hack -- Display "beam" grids */
		if (!(flg & PROJECT_HIDE) &&
		    dist && (flg & PROJECT_BEAM))
#ifdef PROJECTION_FLUSH_LIMIT
			if (count_project < PROJECTION_FLUSH_LIMIT)
#endif	// PROJECTION_FLUSH_LIMIT
		{
			/* Hack -- Visual effect -- "explode" the grids */
			for (j = 1; j < NumPlayers + 1; j++)
			{
				player_type *p_ptr = Players[j];
				int dispx, dispy;
				byte attr;

				if (p_ptr->conn == NOT_CONNECTED)
					continue;

				if(!inarea(&p_ptr->wpos, wpos))
					continue;

				if (p_ptr->blind)
					continue;

				if (!panel_contains(y, x))
					continue;

				if (!player_has_los_bold(j, y, x))
					continue;

				dispx = x - p_ptr->panel_col_prt;
				dispy = y - p_ptr->panel_row_prt;

				attr = spell_color(typ);

				p_ptr->scr_info[dispy][dispx].c = '*';
				p_ptr->scr_info[dispy][dispx].a = attr;

				Send_char(j, dispx, dispy, attr, '*');
			}
		}

		/* Never pass through walls */
		if (dist && !cave_floor_bold(zcave, y, x)) break;

		/* Check for arrival at "final target" (if desired) */
		if (!(flg & PROJECT_THRU) && (x == x2) && (y == y2)) break;

		/* If allowed, and we have moved at all, stop when we hit anybody */		
		/* -AD- Only stop if it isn't a party member */		   
				
		if ((c_ptr->m_idx != 0) && dist && (flg & PROJECT_STOP))
		{
			if (who > 0){
				/* hit first player (ignore monster) */
				if(c_ptr->m_idx < 0) break;
			}
			else if(who < 0){
				/* always hit monsters */
				if (c_ptr->m_idx > 0) break;
			
				/* neutral people hit each other */			
				if (!Players[0 - who]->party) break;
			
			/* people not in the same party hit each other */			
				if (!player_in_party(Players[0 - who]->party, 0 - c_ptr->m_idx))
#if FRIEND_FIRE_CHANCE
					if (!magik(FRIEND_FIRE_CHANCE))
#endif
						break;	
			}
//			else break;	// Huh? always break? XXX XXX
		}

		if((c_ptr->m_idx != 0) && dist &&
			((typ == GF_HEAL_PLAYER) || (typ == GF_WRAITH_PLAYER) ||
			 (typ == GF_SPEED_PLAYER) || (typ == GF_SHIELD_PLAYER) || 
			 (typ == GF_RECALL_PLAYER) || (typ == GF_BLESS_PLAYER) || 
			 (typ == GF_REMFEAR_PLAYER) || (typ == GF_SATHUNGER_PLAYER) || 
			 (typ == GF_RESFIRE_PLAYER) || (typ == GF_RESCOLD_PLAYER) || 
			 (typ == GF_CUREPOISON_PLAYER) || (typ == GF_SEEINVIS_PLAYER) || 
			 (typ == GF_SEEMAP_PLAYER) || (typ == GF_CURECUT_PLAYER) || 
			 (typ == GF_CURESTUN_PLAYER) || (typ == GF_DETECTCREATURE_PLAYER) || 
			 (typ == GF_DETECTDOOR_PLAYER) || (typ == GF_DETECTTRAP_PLAYER) || 
			 (typ == GF_TELEPORTLVL_PLAYER) || (typ == GF_RESPOIS_PLAYER) || 
			 (typ == GF_RESELEC_PLAYER) || (typ == GF_RESACID_PLAYER) || 
			 (typ == GF_HPINCREASE_PLAYER) || (typ == GF_HERO_PLAYER) || 
			 (typ == GF_SHERO_PLAYER) || (typ == GF_TELEPORT_PLAYER)))
		{
			if (!(c_ptr->m_idx > 0))
				break;
		}


		/* Calculate the new location */
		y9 = y;
		x9 = x;
		mmove2(&y9, &x9, y1, x1, y2, x2);

		/* Hack -- Balls explode BEFORE reaching walls or doors */
		if (!cave_floor_bold(zcave, y9, x9) && (rad > 0)) break;

		/* Keep track of the distance traveled */
		dist++;

		/* Nothing can travel furthur than the maximal distance */
		if (dist > MAX_RANGE) break;

		/* Only do visual effects (and delay) if requested */
		if (!(flg & PROJECT_HIDE))
#ifdef PROJECTION_FLUSH_LIMIT
			if (count_project < PROJECTION_FLUSH_LIMIT)
#endif	// PROJECTION_FLUSH_LIMIT
		{
			for (j = 1; j < NumPlayers + 1; j++)
			{
				player_type *p_ptr = Players[j];
				int dispy, dispx;
				char ch;
				byte attr;

				if (p_ptr->conn == NOT_CONNECTED)
					continue;

				if (!inarea(&p_ptr->wpos, wpos))
					continue;

				if (p_ptr->blind)
					continue;

				if (!panel_contains(y9, x9))
					continue;

				if (!player_has_los_bold(j, y9, x9))
					continue;

				dispx = x9 - p_ptr->panel_col_prt;
				dispy = y9 - p_ptr->panel_row_prt;

				ch = bolt_char(y, x, y9, x9);
				attr = spell_color(typ);

				p_ptr->scr_info[dispy][dispx].c = ch;
				p_ptr->scr_info[dispy][dispx].a = attr;

				Send_char(j, dispx, dispy, attr, ch);

				/* Hack -- Show bolt char */
				if (dist % 2) Send_flush(j);
			}
		}

		/* Clean up */
		everyone_lite_spot(wpos, y9, x9);

		/* Save the new location */
		y = y9;
		x = x9;
	}


	/* Save the "blast epicenter" */
	y2 = y;
	x2 = x;

	/* Start the "explosion" */
	gm[0] = 0;

	/* Hack -- make sure beams get to "explode" */
	gm[1] = grids;

	/* Ported hack for reflection */
	dist_hack = dist;

	/* If we found a "target", explode there */
	if (dist <= MAX_RANGE)
	{
		/* Mega-Hack -- remove the final "beam" grid */
//		if ((flg & PROJECT_BEAM) && (grids > 0)) grids--;

		dist = 0;

		for (i = 0; i <= tdi[rad]; i++)
		{
			/* Encode some more "radius" info */
			if (i == tdi[dist])
			{
				gm[++dist] = grids; 
#if DEBUG_LEVEL > 2
				s_printf("dist:%d  i:%d\n", dist, i);
#endif
				if (dist > rad) break;
			}

			y = y2 + tdy[i];
			x = x2 + tdx[i];

			/* Ignore "illegal" locations */
			if (!in_bounds3(wpos, l_ptr, y, x)) continue;

#if 1
			if (typ == GF_DISINTEGRATE)
			{
				if (cave_valid_bold(zcave,y,x))
//						&& (cave[y][x].feat < FEAT_PATTERN_START
//						 || cave[y][x].feat > FEAT_PATTERN_XTRA2))
					cave_set_feat(wpos, y, x, FEAT_FLOOR);

				/* Update some things -- similar to GF_KILL_WALL */
//				p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS);
			}
#endif	/* 0 */
			/* else */ /* HERE!!!!*/
			/* Ball explosions are stopped by walls */
			if (!los(wpos, y2, x2, y, x)) continue;

			/* Save this grid */
			gy[grids] = y;
			gx[grids] = x;
			grids++;
			if(grids>500) printf("grids %d\n", grids);
		}

#if 0
		/* Determine the blast area, work from the inside out */
		for (dist = 0; dist <= rad; dist++)
		{
			/* Scan the maximal blast area of radius "dist" */
			for (y = y2 - dist; y <= y2 + dist; y++)
			{
				for (x = x2 - dist; x <= x2 + dist; x++)
				{
					/* Ignore "illegal" locations */
					if (!in_bounds2(wpos, y, x)) continue;

					/* Enforce a "circular" explosion */
					if (distance(y2, x2, y, x) != dist) continue;

#if 0
					if (typ == GF_DISINTEGRATE)
					{
						if (cave_valid_bold(y,x) &&
						     (cave[y][x].feat < FEAT_PATTERN_START
						   || cave[y][x].feat > FEAT_PATTERN_XTRA2))
							cave_set_feat(y, x, FEAT_FLOOR);

						/* Update some things -- similar to GF_KILL_WALL */
						p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS);
					}
#endif	/* 0 */
					/* else */ /* HERE!!!!*/
					/* Ball explosions are stopped by walls */
					if (!los(wpos, y2, x2, y, x)) continue;

					/* Save this grid */
					gy[grids] = y;
					gx[grids] = x;
					grids++;
					if(grids>500) printf("grids %d\n", grids);
				}
			}

			/* Encode some more "radius" info */
			gm[dist+1] = grids;
		}
#endif	// 0
	}


	/* Speed -- ignore "non-explosions" */
	if (!grids) return (FALSE);

	/* Display the "blast area" */
	if (!(flg & PROJECT_HIDE))
#ifdef PROJECTION_FLUSH_LIMIT
			if (count_project < PROJECTION_FLUSH_LIMIT)
#endif	// PROJECTION_FLUSH_LIMIT
	{
		/* Then do the "blast", from inside out */
		for (t = 0; t <= rad; t++)
		{
			/* Reset who can see */
			num_can_see = 0;

			/* Dump everything with this radius */
			for (i = gm[t]; i < gm[t+1]; i++)
			{
				/* Extract the location */
				y = gy[i];
				x = gx[i];

				for (j = 1; j < NumPlayers + 1; j++)
				{
					player_type *p_ptr = Players[j];
					int dispy, dispx;
					byte attr;
					int k;
					bool flag = TRUE;

					if (p_ptr->conn == NOT_CONNECTED)
						continue;

					if (!inarea(&p_ptr->wpos, wpos))
						continue;

					if (p_ptr->blind)
						continue;

					if (!panel_contains(y, x))
						continue;

					if (!player_has_los_bold(j, y, x))
						continue;

					attr = spell_color(typ);

					dispx = x - p_ptr->panel_col_prt;
					dispy = y - p_ptr->panel_row_prt;

					p_ptr->scr_info[dispy][dispx].c = '*';
					p_ptr->scr_info[dispy][dispx].a = attr;

					Send_char(j, dispx, dispy, attr, '*');

					drawn = TRUE;

					for (k = 0; k < num_can_see; k++)
					{
						if (who_can_see[k] == j)
							flag = FALSE;
					}
							
					if (flag)
						who_can_see[num_can_see++] = j;
				}
			}

			/* Flush each "radius" seperately */
			for (j = 0; j < num_can_see; j++)
			{
				/* Show this radius and delay */
				Send_flush(who_can_see[j]);
			}
		}

		/* Flush the erasing */
		if (drawn)
		{
			/* Erase the explosion drawn above */
			for (i = 0; i < grids; i++)
			{
				/* Extract the location */
				y = gy[i];
				x = gx[i];

				/* Erase if needed */
				everyone_lite_spot(wpos, y, x);
			}

			/* Flush the explosion */
			for (j = 0; j < num_can_see; j++)
			{
				/* Show this radius and delay */
				Send_flush(who_can_see[j]);
			}
		}
	}


	/* Check features */
	if (flg & PROJECT_GRID)
	{
		/* Start with "dist" of zero */
		dist = 0;

		/* Effect ? */
		if (flg & PROJECT_STAY)
		{
			/* I believe it's not right */
//			effect = new_effect(typ, dam, project_time, py, px, rad, project_time_effect);
			/* MEGAHACK -- quick hack to make fire_wall work
			 * this should be rewritten!	- Jir -
			 *
			 * It registers the 'wall' as if it was a ball:
			 *
			 *       |--dist_hack--|
			 * (y,x) *------+------* (y2,x2)
			 *              +pseudo 'centre'
			 */
			if (rad == 0)
			{
				effect = new_effect(who, typ, dam, project_time, wpos,
						(y + y2) / 2, (x + x2) / 2, dist_hack / 2 + 1,
						project_time_effect);
			}
			else
			{
				effect = new_effect(who, typ, dam, project_time, wpos,
						y2, x2, rad, project_time_effect);
			}
			project_time = 0;
			project_time_effect = 0;
		}

		/* Now hurt the cave grids (and objects) from the inside out */
		for (i = 0; i < grids; i++)
		{
			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			if(!in_bounds(y,x)) continue;
			/* Affect the feature */
			if (project_f(0 - who, who, dist, wpos, y, x, dam, typ)) notice = TRUE;

			/* Effect ? */
			if (flg & PROJECT_STAY)
			{
				zcave[y][x].effect = effect;
				everyone_lite_spot(wpos, y, x);
			}
		}
	}


	/* Check objects */
	if (flg & PROJECT_ITEM)
	{
		/* Start with "dist" of zero */
		dist = 0;

		/* Now hurt the cave grids (and objects) from the inside out */
		for (i = 0; i < grids; i++)
		{
			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			if(!in_bounds(y,x)) continue;
			/* Affect the object */
			if (project_i(0 - who, who, dist, wpos, y, x, dam, typ)) notice = TRUE;
		}
	}


	/* Check monsters */
	/* eww, hope traps won't kill the server here..	- Jir - */
	if (flg & PROJECT_KILL)
	{
		/* Start with "dist" of zero */
		dist = 0;

		/* Mega-Hack */
		project_m_n = 0;
		project_m_x = 0;
		project_m_y = 0;

		if (who < 0 && who > PROJECTOR_UNUSUAL &&
				Players[0 - who]->taciturn_messages)
			suppress_message = TRUE;

		/* Now hurt the monsters, from inside out */
		for (i = 0; i < grids; i++)
		{
			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			/* paranoia */
			if (!in_bounds2(wpos, y, x)) continue;

			/* Walls protect monsters */
			if (!cave_floor_bold(zcave, y, x)) continue;

			/* Affect the monster */
//			if (project_m(0-who, who, dist, wpos, y, x, dam, typ)) notice = TRUE;

			if (zcave[y][x].m_idx <= 0) continue;

//			if (grids <= 1 && (zcave[y][x].m_idx > 0))
			if (grids <= 1)
			{
				monster_type *m_ptr = &m_list[zcave[y][x].m_idx];
				monster_race *ref_ptr = race_inf(m_ptr);
//				monster_race *ref_ptr = race_inf(&m_list[zcave[y][x].m_idx]);
			}

			if (project_m(0-who, who, dist, wpos, y, x, dam, typ)) notice = TRUE;
		}

		/* Mega-Hack */
		if ((who < 0) && (project_m_n == 1) && (who > PROJECTOR_UNUSUAL))
		{
			/* Location */
			x = project_m_x;
			y = project_m_y;

			/* Still here */
			if (who < 0)
			{
				player_type *p_ptr = Players[0 - who];
				int m_idx = zcave[y][x].m_idx;

				/* Hack - auto-track monster */
				if (m_idx > 0)
				{
					if (p_ptr->mon_vis[m_idx]) health_track(0 - who, m_idx);
				}
				else
				{
					if (p_ptr->play_vis[0 - m_idx]) health_track(0 - who, m_idx);
				}
			}
		}

		suppress_message = old_tacit;
	}

	/* Check player */
	if (flg & PROJECT_KILL)
	{
		/* Start with "dist" of zero */
		dist = 0;

		/* Now see if the player gets hurt */
		for (i = 0; i < grids; i++)
		{
			/* Who is at the location */
			int player_idx;

			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			/* Set the player index */
			/* paranoia */
			if (!in_bounds2(wpos, y, x)) continue;

			player_idx = 0 - zcave[y][x].m_idx;

			/* Affect the player */
			if (project_p(player_idx, who, dist, wpos, y, x, dam, typ, rad, flg)) notice = TRUE;
		}
	}

	/* Return "something was noticed" */
	return (notice);
}





