/* File: cave.c */

/* Purpose: low level dungeon routines -BEN- */

#define SERVER

#include "angband.h"

/*
 * monsters with 'RF1_ATTR_MULTI' uses colour according to their
 * breath if it is on. (possible bottleneck, tho)
 */
#define MULTI_HUED_PROPER

/*
 * Scans for cave_type array pointer.
 * Returns cave array relative to the dimensions
 * specified in the arguments. Returns NULL for
 * a failure.
 */
cave_type **getcave(struct worldpos *wpos)
{
	struct wilderness_type *wild;
	wild=&wild_info[wpos->wy][wpos->wx];
	if(wpos->wz==0)
	{
		return(wild->cave);
	}
	else
	{
		if(wpos->wz>0)
			return(wild->tower->level[wpos->wz-1].cave);
		else
			return(wild->dungeon->level[ABS(wpos->wz)-1].cave);
	}
}

/* an afterthought - it is often needed without up/down info */
struct dungeon_type *getdungeon(struct worldpos *wpos)
{
	struct wilderness_type *wild;
	wild=&wild_info[wpos->wy][wpos->wx];
	if(wpos->wz==0) return NULL;
	else
		return(wpos->wz>0 ? wild->tower:wild->dungeon);
}

void new_level_up_x(struct worldpos *wpos, int pos)
{
	struct wilderness_type *wild;
	wild=&wild_info[wpos->wy][wpos->wx];
	if(wpos->wz==0) wild->up_x=pos; 
	else if(wpos->wz>0)
		wild->tower->level[wpos->wz-1].up_x=pos;
	else
		wild->dungeon->level[ABS(wpos->wz)-1].up_x=pos;
}
void new_level_up_y(struct worldpos *wpos, int pos)
{
	struct wilderness_type *wild;
	wild=&wild_info[wpos->wy][wpos->wx];
	if(wpos->wz==0) wild->up_y=pos; 
	else if(wpos->wz>0)
		wild->tower->level[wpos->wz-1].up_y=pos;
	else
		wild->dungeon->level[ABS(wpos->wz)-1].up_y=pos;
}
void new_level_down_x(struct worldpos *wpos, int pos)
{
	struct wilderness_type *wild;
	wild=&wild_info[wpos->wy][wpos->wx];
	if(wpos->wz==0) wild->dn_x=pos; 
	else if(wpos->wz>0)
		wild->tower->level[wpos->wz-1].dn_x=pos;
	else
		wild->dungeon->level[ABS(wpos->wz)-1].dn_x=pos;
}
void new_level_down_y(struct worldpos *wpos, int pos)
{
	struct wilderness_type *wild;
	wild=&wild_info[wpos->wy][wpos->wx];
	if(wpos->wz==0) wild->dn_y=pos; 
	else if(wpos->wz>0)
		wild->tower->level[wpos->wz-1].dn_y=pos;
	else
		wild->dungeon->level[ABS(wpos->wz)-1].dn_y=pos;
}
void new_level_rand_x(struct worldpos *wpos, int pos)
{
	struct wilderness_type *wild;
	wild=&wild_info[wpos->wy][wpos->wx];
	if(wpos->wz==0) wild->rn_x=pos; 
	else if(wpos->wz>0)
		wild->tower->level[wpos->wz-1].rn_x=pos;
	else
		wild->dungeon->level[ABS(wpos->wz)-1].rn_x=pos;
}
void new_level_rand_y(struct worldpos *wpos, int pos)
{
	struct wilderness_type *wild;
	wild=&wild_info[wpos->wy][wpos->wx];
	if(wpos->wz==0) wild->rn_y=pos; 
	else if(wpos->wz>0)
		wild->tower->level[wpos->wz-1].rn_y=pos;
	else
		wild->dungeon->level[ABS(wpos->wz)-1].rn_y=pos;
}

byte level_up_x(struct worldpos *wpos)
{
	struct wilderness_type *wild;
	wild=&wild_info[wpos->wy][wpos->wx];
	if(wpos->wz==0) return(wild->up_x); 
	return(wpos->wz>0?wild->tower->level[wpos->wz-1].up_x:wild->dungeon->level[ABS(wpos->wz)-1].up_x);
}
byte level_up_y(struct worldpos *wpos)
{
	struct wilderness_type *wild;
	wild=&wild_info[wpos->wy][wpos->wx];
	if(wpos->wz==0) return(wild->up_y); 
	return(wpos->wz>0?wild->tower->level[wpos->wz-1].up_y:wild->dungeon->level[ABS(wpos->wz)-1].up_y);
}
byte level_down_x(struct worldpos *wpos)
{
	struct wilderness_type *wild;
	wild=&wild_info[wpos->wy][wpos->wx];
	if(wpos->wz==0) return(wild->dn_x); 
	return(wpos->wz>0?wild->tower->level[wpos->wz-1].dn_x:wild->dungeon->level[ABS(wpos->wz)-1].dn_x);
}
byte level_down_y(struct worldpos *wpos)
{
	struct wilderness_type *wild;
	wild=&wild_info[wpos->wy][wpos->wx];
	if(wpos->wz==0) return(wild->dn_y); 
	return(wpos->wz>0?wild->tower->level[wpos->wz-1].dn_y:wild->dungeon->level[ABS(wpos->wz)-1].dn_y);
}
byte level_rand_x(struct worldpos *wpos)
{
	struct wilderness_type *wild;
	wild=&wild_info[wpos->wy][wpos->wx];
	if(wpos->wz==0) return(wild->rn_x); 
	return(wpos->wz>0?wild->tower->level[wpos->wz-1].rn_x:wild->dungeon->level[ABS(wpos->wz)-1].rn_x);
}
byte level_rand_y(struct worldpos *wpos)
{
	struct wilderness_type *wild;
	wild=&wild_info[wpos->wy][wpos->wx];
	if(wpos->wz==0) return(wild->rn_y); 
	return(wpos->wz>0?wild->tower->level[wpos->wz-1].rn_y:wild->dungeon->level[ABS(wpos->wz)-1].rn_y);
}

bool can_go_up(struct worldpos *wpos)
{
	struct wilderness_type *wild;
	if(wpos->wz<0) return(TRUE);
	wild=&wild_info[wpos->wy][wpos->wx];
	if(wpos->wz>0) return(wpos->wz < wild->tower->maxdepth);
	return((wild->flags&WILD_F_UP)?TRUE:FALSE);
}
bool can_go_down(struct worldpos *wpos)
{
	struct wilderness_type *wild;
	if(wpos->wz>0) return(TRUE);
	wild=&wild_info[wpos->wy][wpos->wx];
	if(wpos->wz<0) return(ABS(wpos->wz) < wild->dungeon->maxdepth);
	return((wild->flags&WILD_F_DOWN)?TRUE:FALSE);
}

bool istown(struct worldpos *wpos)
{
	if(!wpos->wz && wild_info[wpos->wy][wpos->wx].type==WILD_TOWN) return(TRUE);
	return(FALSE);
}

void wpcopy(struct worldpos *dest, struct worldpos *src)
{
	dest->wx=src->wx;
	dest->wy=src->wy;
	dest->wz=src->wz;
}

int wild_idx(worldpos *wpos)
{
	return (wpos->wx + wpos->wy * MAX_WILD_X);
}

void new_players_on_depth(struct worldpos *wpos, int value, bool inc)
{
	struct wilderness_type *w_ptr;
	time_t now;

	now=time(&now);

	w_ptr=&wild_info[wpos->wy][wpos->wx];
	if(wpos->wz==0)
	{
		w_ptr->ondepth=(inc?w_ptr->ondepth+value:value);
#if 0
		if(w_ptr->ondepth < 0) w_ptr->ondepth=0;	// ondepth is unsigned!
		if(!w_ptr->ondepth) w_ptr->lastused=0;
		if(value>0)
			w_ptr->lastused=now;
#endif
			w_ptr->lastused=now;

	}
	else
	{
		struct dungeon_type *d_ptr;
		struct dun_level *l_ptr;
		if(wpos->wz>0)
			d_ptr=wild_info[wpos->wy][wpos->wx].tower;
		else
			d_ptr=wild_info[wpos->wy][wpos->wx].dungeon;
		l_ptr=&d_ptr->level[ABS(wpos->wz)-1];

		l_ptr->ondepth=(inc?l_ptr->ondepth+value:value);
		if(l_ptr->ondepth < 0) l_ptr->ondepth=0;
#if 0
		if(!l_ptr->ondepth) l_ptr->lastused=0;
		if(value>0) l_ptr->lastused=now;
#endif
		l_ptr->lastused=now;
	}
}

int players_on_depth(struct worldpos *wpos)
{
	if(wpos->wz==0)
		return(wild_info[wpos->wy][wpos->wx].ondepth);
	else
	{
		struct dungeon_type *d_ptr;
		if(wpos->wz>0)
			d_ptr=wild_info[wpos->wy][wpos->wx].tower;
		else
			d_ptr=wild_info[wpos->wy][wpos->wx].dungeon;
		return(d_ptr->level[ABS(wpos->wz)-1].ondepth);
	}
}

bool inarea(struct worldpos *apos, struct worldpos *bpos)
{
	return(apos->wx==bpos->wx && apos->wy==bpos->wy && apos->wz==bpos->wz);
}

int getlevel(struct worldpos *wpos)
{
	wilderness_type *w_ptr=&wild_info[wpos->wy][wpos->wx];

	if(wpos->wz==0)
	{
		/* ground level */
		return(w_ptr->radius);
	}
	else
	{
		struct dungeon_type *d_ptr;
		int base;
		if(wpos->wz>0)
			d_ptr=w_ptr->tower;
		else
			d_ptr=w_ptr->dungeon;
		base=d_ptr->baselevel+ABS(wpos->wz)-1;
		return(base);
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
bool los(struct worldpos *wpos, int y1, int x1, int y2, int x2)
{
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
	if(!(zcave=getcave(wpos))) return FALSE;

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
	if (!dx)
	{
		/* South -- check for walls */
		if (dy > 0)
		{
			for (ty = y1 + 1; ty < y2; ty++)
			{
				if (!cave_floor_bold(zcave, ty, x1)) return (FALSE);
			}
		}

		/* North -- check for walls */
		else
		{
			for (ty = y1 - 1; ty > y2; ty--)
			{
				if (!cave_floor_bold(zcave, ty, x1)) return (FALSE);
			}
		}

		/* Assume los */
		return (TRUE);
	}

	/* Directly East/West */
	if (!dy)
	{
		/* East -- check for walls */
		if (dx > 0)
		{
			for (tx = x1 + 1; tx < x2; tx++)
			{
				if (!cave_floor_bold(zcave, y1, tx)) return (FALSE);
			}
		}

		/* West -- check for walls */
		else
		{
			for (tx = x1 - 1; tx > x2; tx--)
			{
				if (!cave_floor_bold(zcave, y1, tx)) return (FALSE);
			}
		}

		/* Assume los */
		return (TRUE);
	}


	/* Extract some signs */
	sx = (dx < 0) ? -1 : 1;
	sy = (dy < 0) ? -1 : 1;


	/* Vertical "knights" */
	if (ax == 1)
	{
		if (ay == 2)
		{
			if (cave_floor_bold(zcave, y1 + sy, x1)) return (TRUE);
		}
	}

	/* Horizontal "knights" */
	else if (ay == 1)
	{
		if (ax == 2)
		{
			if (cave_floor_bold(zcave, y1, x1 + sx)) return (TRUE);
		}
	}


	/* Calculate scale factor div 2 */
	f2 = (ax * ay);

	/* Calculate scale factor */
	f1 = f2 << 1;


	/* Travel horizontally */
	if (ax >= ay)
	{
		/* Let m = dy / dx * 2 * (dy * dx) = 2 * dy * dy */
		qy = ay * ay;
		m = qy << 1;

		tx = x1 + sx;

		/* Consider the special case where slope == 1. */
		if (qy == f2)
		{
			ty = y1 + sy;
			qy -= f1;
		}
		else
		{
			ty = y1;
		}

		/* Note (below) the case (qy == f2), where */
		/* the LOS exactly meets the corner of a tile. */
		while (x2 - tx)
		{
			if (!cave_floor_bold(zcave, ty, tx)) return (FALSE);

			qy += m;

			if (qy < f2)
			{
				tx += sx;
			}
			else if (qy > f2)
			{
				ty += sy;
				if (!cave_floor_bold(zcave, ty, tx)) return (FALSE);
				qy -= f1;
				tx += sx;
			}
			else
			{
				ty += sy;
				qy -= f1;
				tx += sx;
			}
		}
	}

	/* Travel vertically */
	else
	{
		/* Let m = dx / dy * 2 * (dx * dy) = 2 * dx * dx */
		qx = ax * ax;
		m = qx << 1;

		ty = y1 + sy;

		if (qx == f2)
		{
			tx = x1 + sx;
			qx -= f1;
		}
		else
		{
			tx = x1;
		}

		/* Note (below) the case (qx == f2), where */
		/* the LOS exactly meets the corner of a tile. */
		while (y2 - ty)
		{
			if (!cave_floor_bold(zcave, ty, tx)) return (FALSE);

			qx += m;

			if (qx < f2)
			{
				ty += sy;
			}
			else if (qx > f2)
			{
				tx += sx;
				if (!cave_floor_bold(zcave, ty, tx)) return (FALSE);
				qx -= f1;
				ty += sy;
			}
			else
			{
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
	wpos=&p_ptr->wpos;

	/* Blind players see nothing */
	if (p_ptr->blind) return (FALSE);

	/* temp bug fix - evileye */
	if(!(zcave=getcave(wpos))) return FALSE;
	c_ptr=&zcave[y][x];
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
	if (zcave[yy][xx].info & CAVE_GLOW)
	{
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
	return (!player_can_see_bold(Ind, p_ptr->py, p_ptr->px));
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

/* 
 * Table of breath colors.  Must match listings in a single set of 
 * monster spell flags.
 *
 * The value "255" is special.  Monsters with that kind of breath 
 * may be any color.
 */
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
	{  0,  0 },     /*  */
	{  TERM_GREEN,  TERM_L_GREEN },		/* RF4_BRTH_NUKE */
	{  0,  0 },     /*  */
	{  TERM_WHITE,  TERM_L_RED },		/* RF4_BRTH_DISINT */
};
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

	int i, j;

	int stored_colors = 0;
	int breaths = 0;
	int first_color = 0;
	int second_color = 0;


	/* Monsters with no ranged attacks can be any color */
	if (!r_ptr->freq_inate) return (get_shimmer_color());

	/* Check breaths */
	for (i = 0; i < 32; i++)
	{
		bool stored = FALSE;

		/* Don't have that breath */
		if (!(r_ptr->flags4 & (1L << i))) continue;

		/* Get the first color of this breath */
		first_color = breath_to_attr[i][0];

		/* Breath has no color associated with it */
		if (first_color == 0) continue;

		/* Monster can be of any color */
		if (first_color == 255) return (randint(15));


		/* Increment the number of breaths */
		breaths++;

		/* Monsters with lots of breaths may be any color. */
		if (breaths == 6) return (randint(15));


		/* Always store the first color */
		for (j = 0; j < stored_colors; j++)
		{
			/* Already stored */
			if (allowed_attrs[j] == first_color) stored = TRUE;
		}
		if (!stored)
		{
			allowed_attrs[stored_colors] = first_color;
			stored_colors++;
		}

		/* 
		 * Remember (but do not immediately store) the second color 
		 * of the first breath.
		 */
		if (breaths == 1)
		{
			second_color = breath_to_attr[i][1];
		}
	}

	/* Monsters with no breaths may be of any color. */
	if (breaths == 0) return (get_shimmer_color());

	/* If monster has one breath, store the second color too. */
	if (breaths == 1)
	{
		allowed_attrs[stored_colors] = second_color;
		stored_colors++;
	}

	/* Pick a color at random */
	return (allowed_attrs[rand_int(stored_colors)]);
}


/*
 * Return the correct "color" of another player
 */
static byte player_color(int Ind)
{
	player_type *p_ptr = Players[Ind];
	monster_race *r_ptr = &r_info[p_ptr->body_monster];
	int pcolor = p_ptr->pclass;

	/* Ghosts are black */
	if (p_ptr->ghost) return TERM_L_DARK;

	/* Black Breath carriers omit malignous aura sometimes.. */
	if (p_ptr->black_breath && magik(50)) return TERM_L_DARK;

	if (p_ptr->body_monster) return (r_ptr->d_attr);

	/* Bats are orange */
	
	if (p_ptr->fruit_bat) return TERM_ORANGE;
	
	if (p_ptr->tim_mimic) pcolor = p_ptr->tim_mimic_what;

	/* Color is based off of class */
	switch (pcolor)
	{
		case CLASS_WARRIOR:
			return TERM_UMBER;
		case CLASS_MAGE:
			return TERM_RED;
		case CLASS_PRIEST:
			return TERM_GREEN;
		case CLASS_ROGUE:
			return TERM_BLUE;
		case CLASS_RANGER:
			return TERM_L_WHITE;
		case CLASS_PALADIN:
			return TERM_L_BLUE;
		case CLASS_SORCERER:
			return TERM_ORANGE;
		case CLASS_UNBELIEVER:
			return TERM_L_DARK;
		case CLASS_ARCHER:
			return TERM_L_GREEN;
		case CLASS_MONK:
			return TERM_SLATE;
		case CLASS_TELEPATH:
			return TERM_YELLOW;
	}

	/* Oops */
	return TERM_WHITE;
}


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
 * lighting effects activated by "view_special_lite".
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
 * grids using the "view_special_lite" option (for "white" floor grids),
 * causing certain grids to be displayed using special colors.  If the
 * player is "blind", we will use "dark gray", else if the grid is lit
 * by the torch, and the "view_yellow_lite" option is set, we will use
 * "yellow", else if the grid is "dark", we will use "dark gray", else
 * if the grid is not "viewable", and the "view_bright_lite" option is
 * set, and the we will use "slate" (gray).  We will use "white" for all
 * other cases, in particular, for illuminated viewable floor grids.
 *
 * Note the "special lighting effects" which can be activated for wall
 * grids using the "view_granite_lite" option (for "white" wall grids),
 * causing certain grids to be displayed using special colors.  If the
 * player is "blind", we will use "dark gray", else if the grid is lit
 * by the torch, and the "view_yellow_lite" option is set, we will use
 * "yellow", else if the "view_bright_lite" option is set, and the grid
 * is not "viewable", or is "dark", or is glowing, but not when viewed
 * from the player's current location, we will use "slate" (gray).  We
 * will use "white" for all other cases, in particular, for correctly
 * illuminated viewable wall grids.
 *
 * Note that, when "view_granite_lite" is set, we use an inline version
 * of the "player_can_see_bold()" function to check the "viewability" of
 * grids when the "view_bright_lite" option is set, and we do NOT use
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
void map_info(int Ind, int y, int x, byte *ap, char *cp)
{
	player_type *p_ptr = Players[Ind];
	int kludge; /* You don't want to know what this is for.... -APD */

	cave_type *c_ptr;
	byte *w_ptr;

	feature_type *f_ptr;

	int feat;

	byte a;
	char c;

	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return;

	/* Get the cave */
	c_ptr = &zcave[y][x];
	w_ptr = &p_ptr->cave_flag[y][x];


	/* Feature code */
	feat = c_ptr->feat;

	/* Floors (etc) */
	if (feat <= FEAT_INVIS)
	{
		/* Memorized (or visible) floor */
		/* Hack -- space are visible to the dungeon master */
		if (((*w_ptr & CAVE_MARK) ||
		    ((((c_ptr->info & CAVE_LITE) &&
			(*w_ptr & CAVE_VIEW)) ||
		      ((c_ptr->info & CAVE_GLOW) &&
		       (*w_ptr & CAVE_VIEW))) &&
		     !p_ptr->blind)) || (p_ptr->admin_dm))
		{
			/* Access floor */
			f_ptr = &f_info[FEAT_FLOOR];

			/* Normal char */
			(*cp) = p_ptr->f_char[feat];

			/* Normal attr */
			a = p_ptr->f_attr[feat];

			/* Hack to display detected traps */
			if ((c_ptr->special.type == CS_TRAPS) )
				// && ((c_ptr->special.ptr)->found))
			{
				trap_type *t_ptr = c_ptr->special.ptr;
				if (t_ptr->found)
				{
					/* Hack -- random hallucination */
					if (p_ptr->image)
					{
//						image_random(ap, cp);
						image_object(ap, cp);
						a = randint(15);
					}
					else
					{
						/* If trap isn't on door display it */
						// if (!(f_ptr->flags1 & FF1_DOOR)) c = '^';
						(*cp) = '^';

						/* Add attr */
						a = t_info[t_ptr->t_idx].color;

						/* Get a new color with a strange formula :) */
						if (t_info[t_ptr->t_idx].flags & FTRAP_CHANGE)
						{
							u32b tmp;

							// tmp = dun_level + dungeon_type + c_ptr->feat;
							tmp = p_ptr->wpos.wx + p_ptr->wpos.wy + p_ptr->wpos.wz + feat;

							// a = tmp % 16;
							/* mega-hack: use trap-like colours only */
							a = tmp % 6 + 1;
						}

						/* Hack -- always l.blue if underwater */
						if (feat == FEAT_WATER)
							a = TERM_L_BLUE;
					}
				}
			}

			/* Special lighting effects */
			if (p_ptr->view_special_lite && (a == TERM_WHITE))
			{
				/* Handle "blind" */
				if (p_ptr->blind)
				{
					/* Use "dark gray" */
					a = TERM_L_DARK;
				}

				/* Handle "torch-lit" grids */
				else if (c_ptr->info & CAVE_LITE && *w_ptr & CAVE_VIEW)
				{
					/* Torch lite */
					if (p_ptr->view_yellow_lite)
					{
						/* Use "yellow" */
						a = TERM_YELLOW;
					}
				}

				/* Handle "dark" grids */
				else if (!(c_ptr->info & CAVE_GLOW))
				{
					/* Use "dark gray" */
					a = TERM_L_DARK;
				}

				/* Handle "out-of-sight" grids */
				else if (!(*w_ptr & CAVE_VIEW))
				{
					/* Special flag */
					if (p_ptr->view_bright_lite)
					{
						/* Use "gray" */
						a = TERM_SLATE;
					}
				}
			}

			/* The attr */
			(*ap) = a;
		}

		/* Unknown */
		else
		{
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
	else
	{
		/* Memorized grids */
		/* Hack -- everything is visible to dungeon masters */
		if ((*w_ptr & CAVE_MARK) || (p_ptr->admin_dm))
		{
			/* Apply "mimic" field */
			feat = f_info[feat].mimic;

			/* Access feature */
			f_ptr = &f_info[feat];

			/* Normal char */
			/* (*cp) = f_ptr->f_char; */
			(*cp) = p_ptr->f_char[feat];

			/* Normal attr */
			/* a = f_ptr->f_attr; */
			a = p_ptr->f_attr[feat];

			/* Add trap color - Illusory wall masks everythink */
			/* Hack to display detected traps */
			//		if ((c_ptr->t_idx != 0) && (c_ptr->info & CAVE_TRDT) &&
			//				(c_ptr->feat != FEAT_ILLUS_WALL))
			//		if ((c_ptr->special.type == CS_TRAPS) && (c_ptr->special.ptr->found))
			/* Hack to display detected traps */
			if ((c_ptr->special.type == CS_TRAPS) )
				//					&& ((c_ptr->special.ptr)->found))
			{
				trap_type *t_ptr = c_ptr->special.ptr;
				if (t_ptr->found)
				{
					/* Hack -- random hallucination */
					if (p_ptr->image)
					{
						image_object(ap, cp);
						a = randint(15);
					}
					else
					{
						/* Add attr */
						a = t_info[t_ptr->t_idx].color;
						//	a = t_info[TR_LIST(c_ptr)->t_idx].color;
						/* Get a new color with a strange formula :) */
						if (t_info[t_ptr->t_idx].flags & FTRAP_CHANGE)
						{
							u32b tmp;

							tmp = p_ptr->wpos.wx + p_ptr->wpos.wy + p_ptr->wpos.wz + c_ptr->feat;
							//tmp = dun_level + dungeon_type + c_ptr->feat;

//							a = tmp % 16;
							a = tmp % 6 + 1;
						}
					}
				}
			}

			/* Special lighting effects */
			if (p_ptr->view_granite_lite && (a == TERM_WHITE) && (feat >= FEAT_SECRET))
			{
				/* Handle "blind" */
				if (p_ptr->blind)
				{
					/* Use "dark gray" */
					a = TERM_L_DARK;
				}

				/* Handle "torch-lit" grids */
				else if (*w_ptr & CAVE_LITE)
				{
					/* Torch lite */
					if (p_ptr->view_yellow_lite)
					{
						/* Use "yellow" */
						a = TERM_YELLOW;
					}
				}

				/* Handle "view_bright_lite" */
				else if (p_ptr->view_bright_lite)
				{
					/* Not viewable */
					if (!(*w_ptr & CAVE_VIEW))
					{
						/* Use "gray" */
						a = TERM_SLATE;
					}

					/* Not glowing */
					else if (!(c_ptr->info & CAVE_GLOW))
					{
						/* Use "gray" */
						a = TERM_SLATE;
					}

					/* Not glowing correctly */
					else
					{
						int xx, yy;

						/* Hack -- move towards player */
						yy = (y < p_ptr->py) ? (y + 1) : (y > p_ptr->py) ? (y - 1) : y;
						xx = (x < p_ptr->px) ? (x + 1) : (x > p_ptr->px) ? (x - 1) : x;

						/* Check for "local" illumination */
						if (!(zcave[yy][xx].info & CAVE_GLOW))
						{
							/* Use "gray" */
							a = TERM_SLATE;
						}
					}
				}
			}

			/* The attr */
			(*ap) = a;
		}

		/* Unknown */
		else
		{
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

	/* Hack -- rare random hallucination, except on outer dungeon walls */
	if (p_ptr->image && (!rand_int(256)) && (c_ptr->feat < FEAT_PERM_SOLID))
	{
		/* Hallucinate */
		image_random(ap, cp);
	}


	/* Objects */
	if (c_ptr->o_idx)
	{
		/* Get the actual item */
		object_type *o_ptr = &o_list[c_ptr->o_idx];

		/* Memorized objects */
		/* Hack -- the dungeon master knows where everything is */
		if ((p_ptr->obj_vis[c_ptr->o_idx]) || (p_ptr->admin_dm))
		{
			/* Normal char */
			(*cp) = object_char(o_ptr);

			/* Normal attr */
			(*ap) = object_attr(o_ptr);

			/* Hack -- always l.blue if underwater */
			if (feat == FEAT_WATER)
				(*ap) = TERM_L_BLUE;

			/* Abnormal attr */
//                        if ((!avoid_other) && (!(((*ap) & 0x80) && ((*cp) & 0x80))) && (k_info[o_ptr->k_idx].flags5 & TR5_ATTR_MULTI)) (*ap) = get_shimmer_color();
			if (k_info[o_ptr->k_idx].flags5 & TR5_ATTR_MULTI)
				(*ap) = get_shimmer_color();
//				(*ap) = randint(15);

			/* Hack -- hallucination */
			if (p_ptr->image) image_object(ap, cp);
		}
	}


	/* Handle monsters */
	if (c_ptr->m_idx > 0)
	{
		monster_type *m_ptr = &m_list[c_ptr->m_idx];

		/* Visible monster */
		if (p_ptr->mon_vis[c_ptr->m_idx])
		{
                        monster_race *r_ptr = race_inf(m_ptr);

			/* Possibly GFX corrupts with egos;
			 * in that case use m_ptr->r_ptr instead.	- Jir -
			 */
			/* Desired attr */
			/* a = r_ptr->x_attr; */
                        if (!m_ptr->special && p_ptr->use_r_gfx) a = p_ptr->r_attr[m_ptr->r_idx];
                        else a = r_ptr->d_attr;
//                        else a = m_ptr->r_ptr->d_attr;

			/* Desired char */
			/* c = r_ptr->x_char; */
                        if (!m_ptr->special && p_ptr->use_r_gfx) c = p_ptr->r_char[m_ptr->r_idx];
                        else c = r_ptr->d_char;
//                        else c = m_ptr->r_ptr->d_char;

			/* Ignore weird codes */
			if (avoid_other)
			{
				/* Use char */
				(*cp) = c;

				/* Use attr */
				(*ap) = a;
			}

			/* Special attr/char codes */
			else if ((a & 0x80) && (c & 0x80))
			{
				/* Use char */
				(*cp) = c;

				/* Use attr */
				(*ap) = a;
			}

			/* Hack -- Unique/Ego 'glitters' sometimes */
			else if ((((r_ptr->flags1 & RF1_UNIQUE) && magik(30)) ||
						(m_ptr->ego && magik(5)) ) &&
					(!(r_ptr->flags1 & (RF1_ATTR_CLEAR | RF1_CHAR_CLEAR)) &&
					 !(r_ptr->flags2 & (RF2_SHAPECHANGER))))
			{
				(*cp) = c;

				/* Multi-hued attr */
				if (r_ptr->flags2 & (RF2_ATTR_ANY))
					(*ap) = randint(15);
				else (*ap) = get_shimmer_color();
			}

			/* Multi-hued monster */
			else if (r_ptr->flags1 & RF1_ATTR_MULTI)
			{
				/* Is it a shapechanger? */
				if (r_ptr->flags2 & (RF2_SHAPECHANGER))
				{
					{
						(*cp) = (randint(25)==1?
							image_object_hack[randint(strlen(image_object_hack))]:
							image_monster_hack[randint(strlen(image_monster_hack))]);
					}
				}
				else
					(*cp) = c;

				/* Multi-hued attr */
				if (r_ptr->flags2 & (RF2_ATTR_ANY))
					(*ap) = randint(15);
#ifdef MULTI_HUED_PROPER
				else (*ap) = multi_hued_attr(r_ptr);
#else
				else (*ap) = get_shimmer_color();
#endif	// MULTI_HUED_PROPER
#if 0
				/* Normal char */
				(*cp) = c;

				/* Multi-hued attr */
				(*ap) = randint(15);
#endif	// 0
			}

			/* Normal monster (not "clear" in any way) */
			else if (!(r_ptr->flags1 & (RF1_ATTR_CLEAR | RF1_CHAR_CLEAR)))
			{
				/* Use char */
				(*cp) = c;

				/* Use attr */
				(*ap) = a;
			}

			/* Hack -- Bizarre grid under monster */
			else if ((*ap & 0x80) || (*cp & 0x80))
			{
				/* Use char */
				(*cp) = c;

				/* Use attr */
				(*ap) = a;
			}

			/* Normal */
			else
			{
				/* Normal (non-clear char) monster */
				if (!(r_ptr->flags1 & RF1_CHAR_CLEAR))
				{
					/* Normal char */
					(*cp) = c;
				}

				/* Normal (non-clear attr) monster */
				else if (!(r_ptr->flags1 & RF1_ATTR_CLEAR))
				{
					/* Normal attr */
					(*ap) = a;
				}
			}

			/* Hack -- hallucination */
			if (p_ptr->image)
			{
				/* Hallucinatory monster */
				image_monster(ap, cp);
			}
		}
	}

	/* -APD-
	   Taking D. Gandy's advice and making it display the char as a number if 
	   they are severly wounded (60% health or less)
	   I multiply by 95 instead of 100 because it always rounds down.....
	   and I want to give PCs a little more breathing room.
	   
	 */
	   
	else if (c_ptr->m_idx < 0)
	{
		/* Is that player visible? */
		if (p_ptr->play_vis[0 - c_ptr->m_idx])
		{
		  
		  if (Players[0 - c_ptr->m_idx]->body_monster) c = r_info[Players[0 - c_ptr->m_idx]->body_monster].d_char;
			else if (Players[0 - c_ptr->m_idx]->fruit_bat) c = 'b';
			else if((( Players[0 - c_ptr->m_idx]->chp * 95)/ (Players[0 - c_ptr->m_idx]->mhp*10)) >= 7) c = '@';
			else 
			{
				sprintf((unsigned char *)&kludge,"%d", ((Players[0 - c_ptr->m_idx]->chp * 95) / (Players[0 - c_ptr->m_idx]->mhp*10)));
				c = kludge;
			}                       

			a = player_color(0 - c_ptr->m_idx);

			(*cp) = c;
	
			(*ap) = a;

			if (p_ptr->image)
			{
				/* Change the other player into a hallucination */
				image_monster(ap, cp);
			}
		}
	}
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
void note_spot(int Ind, int y, int x)
{
	player_type *p_ptr = Players[Ind];
	byte *w_ptr = &p_ptr->cave_flag[y][x];

	cave_type **zcave;
	cave_type *c_ptr;
	if (!(zcave=getcave(&p_ptr->wpos))) return;
	c_ptr = &zcave[y][x];

	/* Hack -- memorize objects */
	if (c_ptr->o_idx)
	{
		/* Only memorize once */
		if (!(p_ptr->obj_vis[c_ptr->o_idx]))
		{
			/* Memorize visible objects */
			if (player_can_see_bold(Ind, y, x))
			{
				/* Memorize */
				p_ptr->obj_vis[c_ptr->o_idx] = TRUE;

			}
		}
	}


	/* Hack -- memorize grids */
	if (!(*w_ptr & CAVE_MARK))
	{
		/* Memorize visible grids */
		if (player_can_see_bold(Ind, y, x))
		{
			/* Memorize normal features */
			if (c_ptr->feat > FEAT_INVIS) 
			{
				/* Memorize */
				*w_ptr |= CAVE_MARK;
			}

			/* Option -- memorize all perma-lit floors */
			else if (p_ptr->view_perma_grids && (c_ptr->info & CAVE_GLOW))
			{
				/* Memorize */
				*w_ptr |= CAVE_MARK;
			}

			/* Option -- memorize all torch-lit floors */
			else if (p_ptr->view_torch_grids && (c_ptr->info & CAVE_LITE))
			{
				/* Memorize */
				*w_ptr |= CAVE_MARK;
			}
		}
	}
}


void note_spot_depth(struct worldpos *wpos, int y, int x)
{
	int i;

	for (i = 1; i < NumPlayers + 1; i++)
	{
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		if (inarea(wpos, &Players[i]->wpos))
		{
			note_spot(i, y, x);
		}
	}
}

void everyone_lite_spot(struct worldpos *wpos, int y, int x)
{
	int i;

	/* Check everyone */
	for (i = 1; i < NumPlayers + 1; i++)
	{
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

/*
 * Wipe the "CAVE_MARK" bit in everyone's array
 */
void everyone_forget_spot(struct worldpos *wpos, int y, int x)
{
	int i;

	/* Check everyone */
	for (i = 1; i < NumPlayers + 1; i++)
	{
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
void lite_spot(int Ind, int y, int x)
{
	player_type *p_ptr = Players[Ind];

	int dispx, dispy;

	int kludge;

	/* Redraw if on screen */
	if (panel_contains(y, x))
	{
		byte a;
		char c;

		/* Handle "player" */
		if ((y == p_ptr->py) && (x == p_ptr->px))
		{
			monster_race *r_ptr = &r_info[p_ptr->body_monster];

			/* Get the "player" attr */
			a = r_ptr->d_attr;

			/* Get the "player" char */
			c = r_ptr->d_char;
			
			if (((p_ptr->chp * 95) / (p_ptr->mhp*10)) < 7) 
			{
				sprintf((unsigned char *)&kludge,"%d",(p_ptr->chp * 95) / (p_ptr->mhp*10)); 
				c = kludge;
			}
				
			/*if (((p_ptr->chp * 95) / (p_ptr->mhp*10)) < 7) c = '4';*/
			
			if (!(strcmp(p_ptr->name,"Strider")))
			{
				sprintf(&c,"%d",(p_ptr->chp * 95) / (p_ptr->mhp*10));
			}
						
			if (p_ptr->fruit_bat) c = 'b';
			
			
		}

		/* Normal */
		else
		{
			/* Examine the grid */
			map_info(Ind, y, x, &a, &c);
		}

		/* Hack -- fake monochrome */
		if (!use_color) a = TERM_WHITE;

		dispx = x - p_ptr->panel_col_prt;
		dispy = y - p_ptr->panel_row_prt;

		/* Only draw if different than buffered */
		if (p_ptr->scr_info[dispy][dispx].c != c ||
		    p_ptr->scr_info[dispy][dispx].a != a ||
		    (x == p_ptr->px && y==p_ptr->py))
		{
			/* Modify internal buffer */
			p_ptr->scr_info[dispy][dispx].c = c;
			p_ptr->scr_info[dispy][dispx].a = a;

			/* Tell client to redraw this grid */
			(void)Send_char(Ind, dispx, dispy, a, c);
		} 
	}
}




/*
 * Prints the map of the dungeon
 *
 * Note that, for efficiency, we contain an "optimized" version
 * of both "lite_spot()" and "print_rel()", and that we use the
 * "lite_spot()" function to display the player grid, if needed.
 */
 
void prt_map(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int x, y;
	int dispx, dispy;

	/* Make sure he didn't just change depth */
	if (p_ptr->new_level_flag) return;

	/* Dump the map */
	for (y = p_ptr->panel_row_min; y <= p_ptr->panel_row_max; y++)
	{
		dispy = y - p_ptr->panel_row_prt;

		/* First clear the old stuff */
		for (x = 0; x < 80; x++)
		{
			p_ptr->scr_info[dispy][x].c = 0;
			p_ptr->scr_info[dispy][x].a = 0;
		}

		/* Scan the columns of row "y" */
		for (x = p_ptr->panel_col_min; x <= p_ptr->panel_col_max; x++)
		{
			byte a;
			char c;

			/* Determine what is there */
			map_info(Ind, y, x, &a, &c);

			/* Hack -- fake monochrome */
			if (!use_color) a = TERM_WHITE;

			dispx = x - p_ptr->panel_col_prt;

			/* Efficiency -- Redraw that grid of the map */
			if (p_ptr->scr_info[dispy][dispx].c != c || p_ptr->scr_info[dispy][dispx].a != a)
			{
				p_ptr->scr_info[dispy][dispx].c = c;
				p_ptr->scr_info[dispy][dispx].a = a;
			}
		}

		/* Send that line of info */
		Send_line_info(Ind, dispy);
	}

	/* Display player */
	lite_spot(Ind, p_ptr->py, p_ptr->px);
}
	
	




/*
 * Display highest priority object in the RATIO by RATIO area
 */
//#define RATIO 3

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
static byte priority_table[][2] =
{
	/* Dark */
	{ FEAT_NONE, 2 },

	/* Dirt */
	{ FEAT_DIRT, 3 },

	/* Grass */
	{ FEAT_GRASS, 4 },

	/* Tree */
	{ FEAT_TREE, 5 },

	/* Water */
	{ FEAT_WATER, 6 },

	/* Floors */
	{ FEAT_FLOOR, 7 },

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

	/* Stairs */
	{ FEAT_LESS, 25 },
	{ FEAT_MORE, 25 },

	/* End */
	{ 0, 0 }
};


/*
 * Hack -- a priority function (see below)
 */
static byte priority(byte a, char c)
{
	int i, p0, p1;

	feature_type *f_ptr;

	/* Scan the table */
	for (i = 0; TRUE; i++)
	{
		/* Priority level */
		p1 = priority_table[i][1];

		/* End of table */
		if (!p1) break;

		/* Feature index */
		p0 = priority_table[i][0];

		/* Access the feature */
		f_ptr = &f_info[p0];

		/* Check character and attribute, accept matches */
		if ((f_ptr->z_char == c) && (f_ptr->z_attr == a)) return (p1);
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
 
 
void display_map(int Ind, int *cy, int *cx)
{
	player_type *p_ptr = Players[Ind];

	int i, j, x, y;

	byte ta;
	char tc;

	byte tp;

	byte ma[MAP_HGT + 2][MAP_WID + 2];
	char mc[MAP_HGT + 2][MAP_WID + 2];

	byte mp[MAP_HGT + 2][MAP_WID + 2];

	bool old_view_special_lite;
	bool old_view_granite_lite;


	/* Save lighting effects */
	old_view_special_lite = p_ptr->view_special_lite;
	old_view_granite_lite = p_ptr->view_granite_lite;

	/* Disable lighting effects */
	p_ptr->view_special_lite = FALSE;
	p_ptr->view_granite_lite = FALSE;


	/* Clear the chars and attributes */
	for (y = 0; y < MAP_HGT+2; ++y)
	{
		for (x = 0; x < MAP_WID+2; ++x)
		{
			/* Nothing here */
			ma[y][x] = TERM_WHITE;
			mc[y][x] = ' ';

			/* No priority */
			mp[y][x] = 0;
		}
	}

	/* Fill in the map */
	for (i = 0; i < p_ptr->cur_wid; ++i)
	{
		for (j = 0; j < p_ptr->cur_hgt; ++j)
		{
			/* Location */
			x = i / RATIO + 1;
			y = j / RATIO + 1;

			/* Extract the current attr/char at that map location */
			map_info(Ind, j, i, &ta, &tc);

			/* Extract the priority of that attr/char */
			tp = priority(ta, tc);

			/* Hack - Player(@) should always be displayed */
			if (i == p_ptr->px && j == p_ptr->py)
			{
				tp = 99;
				ta = player_color(Ind);

				if (p_ptr->body_monster) tc = r_info[p_ptr->body_monster].d_char;
				else if (p_ptr->fruit_bat) tc = 'b';
				else if((( p_ptr->chp * 95)/ (p_ptr->mhp*10)) >= 7) tc = '@';
				else 
				{
					int kludge;
					sprintf((unsigned char *)&kludge,"%d", ((p_ptr->chp * 95) / (p_ptr->mhp*10)));
					tc = kludge;
				}                       
			}

			/* Save "best" */
			if (mp[y][x] < tp)
			{
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
	for (y = 0; y < MAP_HGT+2; ++y)
	{
		/* Clear the old info first */
		for (x = 0; x < 80; x++)
		{
			p_ptr->scr_info[y][x].c = 0;
			p_ptr->scr_info[y][x].a = 0;
		}

		/* Display the line */
		for (x = 0; x < MAP_WID+2; ++x)
		{
			ta = ma[y][x];
			tc = mc[y][x];

			/* Hack -- fake monochrome */
			if (!use_color) ta = TERM_WHITE;

			/* Add the character */
			/* Efficiency -- Redraw that grid of the map */

			if (p_ptr->scr_info[y][x].c != tc || p_ptr->scr_info[y][x].a != ta)
			{
				p_ptr->scr_info[y][x].c = tc;
				p_ptr->scr_info[y][x].a = ta;
			} 
		}

		/* Send that line of info */
		Send_mini_map(Ind, y);

		/* Throw some nonsense into the "screen_info" so it gets cleared */
		for (x = 0; x < 80; x++)
		{
			p_ptr->scr_info[y][x].c = 0;
			p_ptr->scr_info[y][x].a = 255;
		}
	}


	/* Player location */
	(*cy) = p_ptr->py / RATIO + 1;
	(*cx) = p_ptr->px / RATIO + 1;


	/* Restore lighting effects */
	p_ptr->view_special_lite = old_view_special_lite;
	p_ptr->view_granite_lite = old_view_granite_lite;
}


void wild_display_map(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int x,y,type;

	byte ta;
	char tc;

	byte ma[MAP_HGT + 2][MAP_WID + 2];
	char mc[MAP_HGT + 2][MAP_WID + 2];

	byte mp[MAP_HGT + 2][MAP_WID + 2];

	bool old_view_special_lite;
	bool old_view_granite_lite;
	struct worldpos twpos;
	twpos.wz=0;

	/* Save lighting effects */
	old_view_special_lite = p_ptr->view_special_lite;
	old_view_granite_lite = p_ptr->view_granite_lite;

	/* Disable lighting effects */
	p_ptr->view_special_lite = FALSE;
	p_ptr->view_granite_lite = FALSE;


	/* Clear the chars and attributes */
	for (y = 0; y < MAP_HGT+2; y++)
	{
		for (x = 0; x < MAP_WID+2; x++)
		{
			/* Nothing here */
			ma[y][x] = TERM_WHITE;
			mc[y][x] = ' ';

			/* No priority */
			mp[y][x] = 0;
		}
	}

	/* for each row */
	for (y = 0; y < MAP_HGT+2; y++)
	{
		/* for each column */
		for (x = 0; x < MAP_WID+2; x++)
		{
			/* Location */
			twpos.wy = p_ptr->wpos.wy + (MAP_HGT+2)/2 - y;
			twpos.wx = p_ptr->wpos.wx - (MAP_WID+2)/2 + x;
			if(twpos.wy >= 0 && twpos.wy < MAX_WILD_Y && twpos.wx >=0 && twpos.wx < MAX_WILD_X)
				type = determine_wilderness_type(&twpos);
			/* if off the map, set to unknown type */
			else type = -1;
			
			/* if the player hasnt been here, dont show him the terrain */
			/* Hack -- serverchez has knowledge of the full world */
			if (!p_ptr->admin_dm)
			if (!(p_ptr->wild_map[wild_idx(&twpos) / 8] & (1 << (wild_idx(&twpos) % 8)))) type = -1;
			/* hack --  the town is always known */
			
			switch (type)
			{
				case WILD_LAKE: tc = '~'; ta=TERM_BLUE; break;
				case WILD_GRASSLAND: tc = '.'; ta= TERM_GREEN; break;
				case WILD_FOREST: tc = '*'; ta = TERM_GREEN; break;
				case WILD_SWAMP:  tc = '%'; ta = TERM_VIOLET; break;
				case WILD_DENSEFOREST: tc = '*'; ta = TERM_L_DARK; break;
				case WILD_WASTELAND: tc = '.'; ta=TERM_UMBER; break;
				case WILD_TOWN: tc = 'T'; ta = TERM_YELLOW; break;
				case WILD_CLONE: tc = 'C'; ta = TERM_RED; break;
				case WILD_MOUNTAIN: tc = '^'; ta = TERM_L_DARK; break;
				case WILD_VOLCANO: tc = '^'; ta = TERM_RED; break;
				case WILD_RIVER: tc = '~'; ta = TERM_L_BLUE; break;
				case WILD_COAST: tc = ','; ta = TERM_YELLOW; break;
				case WILD_OCEAN: tc = '%'; ta = TERM_BLUE; break;
				case -1: tc = ' '; ta = TERM_DARK; break;
				default: tc = 'O'; ta = TERM_YELLOW; break;
			} 
			
			/* put the @ in the center */
			if ((y == (MAP_HGT+2)/2) && (x == (MAP_WID+2)/2))
			{
				tc = '@'; ta = TERM_WHITE; 
			}
			
			/* Save the char */
			mc[y][x] = tc;

			/* Save the attr */
			ma[y][x] = ta;
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
	for (y = 0; y < MAP_HGT+2; ++y)
	{
		/* Clear the old info first */
		for (x = 0; x < 80; x++)
		{
			p_ptr->scr_info[y][x].c = 0;
			p_ptr->scr_info[y][x].a = 0;
		}

		/* Display the line */
		for (x = 0; x < MAP_WID+2; ++x)
		{
			ta = ma[y][x];
			tc = mc[y][x];

			/* Hack -- fake monochrome */
			if (!use_color) ta = TERM_WHITE;

			/* Add the character */
			/* Efficiency -- Redraw that grid of the map */

			if (p_ptr->scr_info[y][x].c != tc || p_ptr->scr_info[y][x].a != ta)
			{
				p_ptr->scr_info[y][x].c = tc;
				p_ptr->scr_info[y][x].a = ta;
			} 
		}

		/* Send that line of info */
		Send_mini_map(Ind, y);

		/* Throw some nonsense into the "screen_info" so it gets cleared */
		for (x = 0; x < 80; x++)
		{
			p_ptr->scr_info[y][x].c = 0;
			p_ptr->scr_info[y][x].a = 255;
		}
	}

	/* Restore lighting effects */
	p_ptr->view_special_lite = old_view_special_lite;
	p_ptr->view_granite_lite = old_view_granite_lite;
}


/*
 * Display a "small-scale" map of the dungeon for the player
 */
 
 /* in the wilderness, have several scales of maps availiable... adding one
    "wilderness map" mode now that will represent each level with one character.
 */
 
void do_cmd_view_map(int Ind)
{
	int cy, cx;

	/* Display the map */
	
	/* if not in town or the dungeon, do normal map */
	/* is player in a town or dungeon? */
	/* only off floor ATM */
	if (Players[Ind]->wpos.wz!=0 || (istown(&Players[Ind]->wpos)))
		display_map(Ind, &cy, &cx);
	/* do wilderness map */
	/* pfft, fix me pls, Evileye ;) */
	/* pfft. fixed */
	else wild_display_map(Ind);
	//else display_map(Ind, &cy, &cx);
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
void forget_lite(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int i, x, y;

	cave_type **zcave;
	struct worldpos *wpos=&p_ptr->wpos;
	if(!(zcave=getcave(&p_ptr->wpos))) return;


	/* None to forget */
	if (!(p_ptr->lite_n)) return;

	/* Clear them all */
	for (i = 0; i < p_ptr->lite_n; i++)
	{
		int j;

		y = p_ptr->lite_y[i];
		x = p_ptr->lite_x[i];

		/* Forget "LITE" flag */
		p_ptr->cave_flag[y][x] &= ~CAVE_LITE;
		zcave[y][x].info &= ~CAVE_LITE;

		for (j = 1; j <= NumPlayers; j++)
		{
			/* Make sure player is connected */
			if (Players[j]->conn == NOT_CONNECTED)
				continue;

			/* Make sure player is on the level */
			if(!inarea(wpos, &Players[j]->wpos))
				continue;

			/* Ignore the player that we're updating */
			if (j == Ind)
				continue;

			/* If someone else also lites this spot relite it */
			if (Players[j]->cave_flag[y][x] & CAVE_LITE)
				zcave[y][x].info |= CAVE_LITE;
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
void update_lite(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int i, x, y, min_x, max_x, min_y, max_y;

	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;

	/*** Special case ***/

	/* Hack -- Player has no lite */
	if (p_ptr->cur_lite <= 0)
	{
		/* Forget the old lite */
		forget_lite(Ind);

		/* Draw the player */
		lite_spot(Ind, p_ptr->py, p_ptr->px);

		/* All done */
		return;
	}


	/*** Save the old "lite" grids for later ***/

	/* Clear them all */
	for (i = 0; i < p_ptr->lite_n; i++)
	{
		int j;

		y = p_ptr->lite_y[i];
		x = p_ptr->lite_x[i];

		/* Mark the grid as not "lite" */
		p_ptr->cave_flag[y][x] &= ~CAVE_LITE;
		zcave[y][x].info &= ~CAVE_LITE;

		for (j = 1; j <= NumPlayers; j++)
		{
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
			if (Players[j]->cave_flag[y][x] & CAVE_LITE)
				zcave[y][x].info |= CAVE_LITE;
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
	if (p_ptr->cur_lite >= 1)
	{
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
	if (p_ptr->cur_lite >= 2)
	{
		/* South of the player */
		if (cave_floor_bold(zcave, p_ptr->py+1, p_ptr->px))
		{
			cave_lite_hack(p_ptr->py+2, p_ptr->px);
			cave_lite_hack(p_ptr->py+2, p_ptr->px+1);
			cave_lite_hack(p_ptr->py+2, p_ptr->px-1);
		}

		/* North of the player */
		if (cave_floor_bold(zcave, p_ptr->py-1, p_ptr->px))
		{
			cave_lite_hack(p_ptr->py-2, p_ptr->px);
			cave_lite_hack(p_ptr->py-2, p_ptr->px+1);
			cave_lite_hack(p_ptr->py-2, p_ptr->px-1);
		}

		/* East of the player */
		if (cave_floor_bold(zcave, p_ptr->py, p_ptr->px+1))
		{
			cave_lite_hack(p_ptr->py, p_ptr->px+2);
			cave_lite_hack(p_ptr->py+1, p_ptr->px+2);
			cave_lite_hack(p_ptr->py-1, p_ptr->px+2);
		}

		/* West of the player */
		if (cave_floor_bold(zcave, p_ptr->py, p_ptr->px-1))
		{
			cave_lite_hack(p_ptr->py, p_ptr->px-2);
			cave_lite_hack(p_ptr->py+1, p_ptr->px-2);
			cave_lite_hack(p_ptr->py-1, p_ptr->px-2);
		}
	}

	/* Radius 3+ -- artifact radius */
	if (p_ptr->cur_lite >= 3)
	{
		int d, p;

		/* Maximal radius */
		p = p_ptr->cur_lite;

		/* Paranoia -- see "LITE_MAX" */
		if (p > 5) p = 5;

		/* South-East of the player */
		if (cave_floor_bold(zcave, p_ptr->py+1, p_ptr->px+1))
		{
			cave_lite_hack(p_ptr->py+2, p_ptr->px+2);
		}

		/* South-West of the player */
		if (cave_floor_bold(zcave, p_ptr->py+1, p_ptr->px-1))
		{
			cave_lite_hack(p_ptr->py+2, p_ptr->px-2);
		}

		/* North-East of the player */
		if (cave_floor_bold(zcave, p_ptr->py-1, p_ptr->px+1))
		{
			cave_lite_hack(p_ptr->py-2, p_ptr->px+2);
		}

		/* North-West of the player */
		if (cave_floor_bold(zcave, p_ptr->py-1, p_ptr->px-1))
		{
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
		for (y = min_y; y <= max_y; y++)
		{
			for (x = min_x; x <= max_x; x++)
			{
				int dy = (p_ptr->py > y) ? (p_ptr->py - y) : (y - p_ptr->py);
				int dx = (p_ptr->px > x) ? (p_ptr->px - x) : (x - p_ptr->px);

				/* Skip the "central" grids (above) */
				if ((dy <= 2) && (dx <= 2)) continue;

				/* Hack -- approximate the distance */
				d = (dy > dx) ? (dy + (dx>>1)) : (dx + (dy>>1));

				/* Skip distant grids */
				if (d > p) continue;

				/* Viewable, nearby, grids get "torch lit" */
				if (player_has_los_bold(Ind, y, x))
				{
					/* This grid is "torch lit" */
					cave_lite_hack(y, x);
				}
			}
		}
	}


	/*** Complete the algorithm ***/

	/* Draw the new grids */
	for (i = 0; i < p_ptr->lite_n; i++)
	{
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
	for (i = 0; i < p_ptr->temp_n; i++)
	{
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
void forget_view(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int i;

	byte *w_ptr;
	
	/* None to forget */
	if (!(p_ptr->view_n)) return;

	/* Clear them all */
	for (i = 0; i < p_ptr->view_n; i++)
	{
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
 
 
static bool update_view_aux(int Ind, int y, int x, int y1, int x1, int y2, int x2)
{
	player_type *p_ptr = Players[Ind];

	bool f1, f2, v1, v2, z1, z2, wall;

	cave_type *c_ptr;
	byte *w_ptr;

	cave_type *g1_c_ptr;
	cave_type *g2_c_ptr;

	byte *g1_w_ptr;
	byte *g2_w_ptr;

	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return FALSE;

	/* Access the grids */
	g1_c_ptr = &zcave[y1][x1];
	g2_c_ptr = &zcave[y2][x2];

	g1_w_ptr = &p_ptr->cave_flag[y1][x1];
	g2_w_ptr = &p_ptr->cave_flag[y2][x2];


	/* Check for walls */
	f1 = (cave_floor_grid(g1_c_ptr));
	f2 = (cave_floor_grid(g2_c_ptr));

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
	wall = (!cave_floor_grid(c_ptr));


	/* Check the "ease" of visibility */
	z1 = (v1 && (g1_c_ptr->info & CAVE_XTRA));
	z2 = (v2 && (g2_c_ptr->info & CAVE_XTRA));

	/* Hack -- "easy" plus "easy" yields "easy" */
	if (z1 && z2)
	{
		c_ptr->info |= CAVE_XTRA;

		cave_view_hack(w_ptr, y, x);

		return (wall);
	}

	/* Hack -- primary "easy" yields "viewed" */
	if (z1)
	{
		cave_view_hack(w_ptr, y, x);

		return (wall);
	}


	/* Hack -- "view" plus "view" yields "view" */
	if (v1 && v2)
	{
		/* c_ptr->info |= CAVE_XTRA; */

		cave_view_hack(w_ptr, y, x);

		return (wall);
	}


	/* Mega-Hack -- the "los()" function works poorly on walls */
	if (wall)
	{
		cave_view_hack(w_ptr, y, x);

		return (wall);
	}


	/* Hack -- check line of sight */
	if (los(wpos, p_ptr->py, p_ptr->px, y, x))
	{
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
void update_view(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int n, m, d, k, y, x, z;

	int se, sw, ne, nw, es, en, ws, wn;

	int full, over;

	int y_max = p_ptr->cur_hgt - 1;
	int x_max = p_ptr->cur_wid - 1;

	cave_type *c_ptr;
	byte *w_ptr;
	bool unmap=FALSE;

	cave_type **zcave;
	struct worldpos *wpos;
	wpos=&p_ptr->wpos;
	if(!(zcave=getcave(wpos))) return;
	if(p_ptr->wpos.wz)
	{
		struct dungeon_type *d_ptr;
		d_ptr=(p_ptr->wpos.wz>0? wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].tower : wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].dungeon);
		if(d_ptr->flags & DUNGEON_NOMAP) unmap=TRUE;
	}


	/*** Initialize ***/

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
		if(unmap)
			*w_ptr &= ~CAVE_MARK;

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
	for (d = 1; d <= z; d++)
	{
		/*if (y + d > 65) break;*/
		c_ptr = &zcave[y+d][x+d];
		w_ptr = &p_ptr->cave_flag[y+d][x+d];
		c_ptr->info |= CAVE_XTRA;
		cave_view_hack(w_ptr, y+d, x+d);
		if (!cave_floor_grid(c_ptr)) break;             
	}

	/* Scan south-west */
	for (d = 1; d <= z; d++)
	{
		/*if (y + d > 65) break;*/
		c_ptr = &zcave[y+d][x-d];
		w_ptr = &p_ptr->cave_flag[y+d][x-d];
		c_ptr->info |= CAVE_XTRA;
		cave_view_hack(w_ptr, y+d, x-d);
		if (!cave_floor_grid(c_ptr)) break;
	}

	/* Scan north-east */
	for (d = 1; d <= z; d++)
	{
		/*if (d > y) break;*/
		c_ptr = &zcave[y-d][x+d];
		w_ptr = &p_ptr->cave_flag[y-d][x+d];
		c_ptr->info |= CAVE_XTRA;
		cave_view_hack(w_ptr, y-d, x+d);
		if (!cave_floor_grid(c_ptr)) break;
	}

	/* Scan north-west */
	for (d = 1; d <= z; d++)
	{
		/*if (d > y) break;*/
		c_ptr = &zcave[y-d][x-d];
		w_ptr = &p_ptr->cave_flag[y-d][x-d];
		c_ptr->info |= CAVE_XTRA;
		cave_view_hack(w_ptr, y-d, x-d);
		if (!cave_floor_grid(c_ptr)) break;
	}


	/*** Step 3 -- major axes ***/

	/* Scan south */
	for (d = 1; d <= full; d++)
	{
		/*if (y + d > 65) break;*/
		c_ptr = &zcave[y+d][x];
		w_ptr = &p_ptr->cave_flag[y+d][x];
		c_ptr->info |= CAVE_XTRA;
		cave_view_hack(w_ptr, y+d, x);
		if (!cave_floor_grid(c_ptr)) break;
	}

	/* Initialize the "south strips" */
	se = sw = d;

	/* Scan north */
	for (d = 1; d <= full; d++)
	{
		/*if (d > y) break;*/
		c_ptr = &zcave[y-d][x];
		w_ptr = &p_ptr->cave_flag[y-d][x];
		c_ptr->info |= CAVE_XTRA;
		cave_view_hack(w_ptr, y-d, x);
		if (!cave_floor_grid(c_ptr)) break;
	}

	/* Initialize the "north strips" */
	ne = nw = d;

	/* Scan east */
	for (d = 1; d <= full; d++)
	{
		c_ptr = &zcave[y][x+d];
		w_ptr = &p_ptr->cave_flag[y][x+d];
		c_ptr->info |= CAVE_XTRA;
		cave_view_hack(w_ptr, y, x+d);
		if (!cave_floor_grid(c_ptr)) break;
	}

	/* Initialize the "east strips" */
	es = en = d;

	/* Scan west */
	for (d = 1; d <= full; d++)
	{
		c_ptr = &zcave[y][x-d];
		w_ptr = &p_ptr->cave_flag[y][x-d];
		c_ptr->info |= CAVE_XTRA;
		cave_view_hack(w_ptr, y, x-d);
		if (!cave_floor_grid(c_ptr)) break;
	}

	/* Initialize the "west strips" */
	ws = wn = d;


	/*** Step 4 -- Divide each "octant" into "strips" ***/

	/* Now check each "diagonal" (in parallel) */
	for (n = 1; n <= over / 2; n++)
	{
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
		if (ypn < y_max)
		{
			/* Maximum distance */
			m = MIN(z, y_max - ypn);

			/* East side */
			if ((xpn <= x_max) && (n < se))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{                               
					/*if (ypn + d > 65) break; */
				
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(Ind, ypn+d, xpn, ypn+d-1, xpn-1, ypn+d-1, xpn))
					{
						if (n + d >= se) break;
					}                                                               
					
					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}                                                                               
					
				}

				/* Limit the next strip */
				se = k + 1;
			}

			/* West side */
			if ((xmn >= 0) && (n < sw))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/*if (ypn + d > 65) break;*/
				
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(Ind, ypn+d, xmn, ypn+d-1, xmn+1, ypn+d-1, xmn))
					{
						if (n + d >= sw) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
										
				}

				/* Limit the next strip */
				sw = k + 1;
			}
		}


		/* North strip */
		if (ymn > 0)
		{
			/* Maximum distance */
			m = MIN(z, ymn);

			/* East side */
			if ((xpn <= x_max) && (n < ne))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/*if (d > ymn) break;*/
				
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(Ind, ymn-d, xpn, ymn-d+1, xpn-1, ymn-d+1, xpn))
					{
						if (n + d >= ne) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}                                                                               
				}

				/* Limit the next strip */
				ne = k + 1;
			}

			/* West side */
			if ((xmn >= 0) && (n < nw))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/*if (d > ymn) break;*/
					
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(Ind, ymn-d, xmn, ymn-d+1, xmn+1, ymn-d+1, xmn))
					{
						if (n + d >= nw) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}                                                                               
				}

				/* Limit the next strip */
				nw = k + 1;
			}
		}


		/* East strip */
		if (xpn < x_max)
		{
			/* Maximum distance */
			m = MIN(z, x_max - xpn);

			/* South side */
			if ((ypn <= x_max) && (n < es))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/*if (ypn > 65) break;*/
				
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(Ind, ypn, xpn+d, ypn-1, xpn+d-1, ypn, xpn+d-1))
					{
						if (n + d >= es) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				es = k + 1;
			}

			/* North side */
			if ((ymn >= 0) && (n < en))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/*if (ymn > 65) break;*/
				
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(Ind, ymn, xpn+d, ymn+1, xpn+d-1, ymn, xpn+d-1))
					{
						if (n + d >= en) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				en = k + 1;
			}
		}


		/* West strip */
		if (xmn > 0)
		{
			/* Maximum distance */
			m = MIN(z, xmn);

			/* South side */
			if ((ypn <= y_max) && (n < ws))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/*if (ypn > 65) break;*/
				
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(Ind, ypn, xmn-d, ypn-1, xmn-d+1, ypn, xmn-d+1))
					{
						if (n + d >= ws) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				ws = k + 1;
			}

			/* North side */
			if ((ymn >= 0) && (n < wn))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/*if (ymn > 65) break;*/
				
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(Ind, ymn, xmn-d, ymn+1, xmn-d+1, ymn, xmn-d+1))
					{
						if (n + d >= wn) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				wn = k + 1;
			}
		}
	}


	/*** Step 5 -- Complete the algorithm ***/

	/* Update all the new grids */
	for (n = 0; n < p_ptr->view_n; n++)
	{
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
		if (*w_ptr & CAVE_VIEW) continue;

		/* Redraw */
		lite_spot(Ind, y, x);
	}

	/* None left */
	p_ptr->temp_n = 0;
}






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
	if (!flow_by_sound) return;

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
void map_area(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int             i, x, y, y1, y2, x1, x2;

	cave_type       *c_ptr;
	byte            *w_ptr;

	dungeon_type	*d_ptr = getdungeon(&p_ptr->wpos);
	struct worldpos *wpos;
	cave_type **zcave;
	wpos=&p_ptr->wpos;
	if(!(zcave=getcave(wpos))) return;
	if(d_ptr && d_ptr->flags & DUNGEON_NOMAP) return;

	/* Pick an area to map */
	y1 = p_ptr->panel_row_min - randint(10);
	y2 = p_ptr->panel_row_max + randint(10);
	x1 = p_ptr->panel_col_min - randint(20);
	x2 = p_ptr->panel_col_max + randint(20);

	/* Speed -- shrink to fit legal bounds */
	if (y1 < 1) y1 = 1;
	if (y2 > p_ptr->cur_hgt-2) y2 = p_ptr->cur_hgt-2;
	if (x1 < 1) x1 = 1;
	if (x2 > p_ptr->cur_wid-2) x2 = p_ptr->cur_wid-2;

	/* Scan that area */
	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			c_ptr = &zcave[y][x];
			w_ptr = &p_ptr->cave_flag[y][x];

			/* All non-walls are "checked" */
			if (c_ptr->feat < FEAT_SECRET)
			{
				/* Memorize normal features */
				if (c_ptr->feat > FEAT_INVIS)
				{
					/* Memorize the object */
					*w_ptr |= CAVE_MARK;
				}

				/* Memorize known walls */
				for (i = 0; i < 8; i++)
				{
					c_ptr = &zcave[y+ddy_ddd[i]][x+ddx_ddd[i]];
					w_ptr = &p_ptr->cave_flag[y+ddy_ddd[i]][x+ddx_ddd[i]];

					/* Memorize walls (etc) */
					if (c_ptr->feat >= FEAT_SECRET)
					{
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
void wiz_lite(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int             y, x, i;

	cave_type       *c_ptr;
	byte            *w_ptr;

	dungeon_type	*d_ptr = getdungeon(&p_ptr->wpos);
	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
	if(d_ptr && d_ptr->flags & DUNGEON_NOMAP) return;

	/* Scan all normal grids */
	for (y = 1; y < p_ptr->cur_hgt-1; y++)
	{
		/* Scan all normal grids */
		for (x = 1; x < p_ptr->cur_wid-1; x++)
		{
			/* Access the grid */
			c_ptr = &zcave[y][x];
			w_ptr = &p_ptr->cave_flag[y][x];

			/* Memorize all objects */
			if (c_ptr->o_idx)
			{
				/* Memorize */
				p_ptr->obj_vis[c_ptr->o_idx]= TRUE;
			}

			/* Process all non-walls */
			if (c_ptr->feat < FEAT_SECRET)
			{
				/* Scan all neighbors */
				for (i = 0; i < 9; i++)
				{
					int yy = y + ddy_ddd[i];
					int xx = x + ddx_ddd[i];

					/* Get the grid */
					c_ptr = &zcave[yy][xx];
					w_ptr = &p_ptr->cave_flag[yy][xx];

					/* Perma-lite the grid */
					c_ptr->info |= (CAVE_GLOW);

					/* Memorize normal features */
					if (c_ptr->feat > FEAT_INVIS)
					{
						/* Memorize the grid */
						*w_ptr |= CAVE_MARK;
					}

					/* Normally, memorize floors (see above) */
					if (p_ptr->view_perma_grids && !p_ptr->view_torch_grids)
					{
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
	struct worldpos *wpos;
	cave_type **zcave;
	cave_type *c_ptr;
	
	wpos=&p_ptr->wpos;
	if(!(zcave=getcave(wpos))) return;

	for(y=0;y < p_ptr->cur_hgt;y++)
	{
		for(x=0;x < p_ptr->cur_wid;x++)
		{
			c_ptr = &zcave[y][x];
			c_ptr->info |= (CAVE_GLOW | CAVE_MARK);
		}
	}
	wiz_lite(Ind);
}

/*
 * Forget the dungeon map (ala "Thinking of Maud...").
 */
void wiz_dark(int Ind)
{
	player_type *p_ptr, *q_ptr = Players[Ind];
	struct worldpos *wpos;
	cave_type **zcave;
	cave_type *c_ptr;
	int y, x, i;
	object_type *o_ptr;

	wpos=&q_ptr->wpos;
	if(!(zcave=getcave(wpos))) return;

	/* Check for every other player */

	for (i = 1; i <= NumPlayers; i++)
	{
		p_ptr = Players[i];
		
		/* Only works for players on the level */
		if (!inarea(wpos, &p_ptr->wpos)) continue;
		
		o_ptr = &p_ptr->inventory[INVEN_LITE];
		
		/* Bye bye light */
		if (o_ptr->k_idx)
		{
			if (((o_ptr->sval == SV_LITE_TORCH) || (o_ptr->sval == SV_LITE_LANTERN)) && (!o_ptr->name3))
			{
				msg_print(Ind, "Your light suddently empty.");
				
				/* No more light, it's Rogues day today :) */
				o_ptr->pval = 0;
			}
		}
	
		/* Forget every grid */
		for (y = 0; y < p_ptr->cur_hgt; y++)
		{
			for (x = 0; x < p_ptr->cur_wid; x++)
			{
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




/*
 * Change the "feat" flag for a grid, and notice/redraw the grid
 * (Adapted from PernAngband)
 */
void cave_set_feat(worldpos *wpos, int y, int x, int feat)
{
	player_type *p_ptr;
	cave_type **zcave;
	cave_type *c_ptr;
	int i;

	if(!(zcave=getcave(wpos))) return;

	c_ptr = &zcave[y][x];

	/* Change the feature */
	c_ptr->feat = feat;

	for (i = 1; i <= NumPlayers; i++)
	{
		p_ptr = Players[i];
		
		/* Only works for players on the level */
		if (!inarea(wpos, &p_ptr->wpos)) continue;
		
		/* Notice */
		note_spot(i, y, x);

		/* Redraw */
		lite_spot(i, y, x);
	}
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
bool projectable(struct worldpos *wpos, int y1, int x1, int y2, int x2)
{
	int dist, y, x;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return FALSE;

	/* Start at the initial location */
	y = y1, x = x1;

	/* See "project()" */
	for (dist = 0; dist <= MAX_RANGE; dist++)
	{
		/* Never pass through walls */
		if (dist && !cave_floor_bold(zcave, y, x)) break;

		/* Check for arrival at "final target" */
		if ((x == x2) && (y == y2)) return (TRUE);

		/* Calculate the new location */
		mmove2(&y, &x, y1, x1, y2, x2);
	}


	/* Assume obstruction */
	return (FALSE);
}

bool projectable_wall(struct worldpos *wpos, int y1, int x1, int y2, int x2)
{
	int dist, y, x;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);

	/* Start at the initial location */
	y = y1, x = x1;

	/* See "project()" */
	for (dist = 0; dist <= MAX_RANGE; dist++)
	{
		/* Check for arrival at "final target" */
		if ((x == x2) && (y == y2)) return (TRUE);

		/* Never go through walls */
		if (dist && !cave_floor_bold(zcave, y, x)) break;

		/* Calculate the new location */
		mmove2(&y, &x, y1, x1, y2, x2);
	}


	/* Assume obstruction */
	return (FALSE);
}


/*
 * Standard "find me a location" function
 *
 * Obtains a legal location within the given distance of the initial
 * location, and with "los()" from the source to destination location.
 *
 * This function is often called from inside a loop which searches for
 * locations while increasing the "d" distance.
 *
 * Currently the "m" parameter is unused.
 *
 * But now the "m" parameter specifies whether "los" is necessary.
 */
void scatter(struct worldpos *wpos, int *yp, int *xp, int y, int x, int d, int m)
{
	int nx, ny;

	/* Pick a location */
	while (TRUE)
	{
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
void disturb(int Ind, int stop_search, int unused_flag)
{
	player_type *p_ptr = Players[Ind];

	/* Unused */
	unused_flag = unused_flag;

	/* Cancel auto-commands */
	/* command_new = 0; */

#if 0
	/* Cancel repeated commands */
	if (command_rep)
	{
		/* Cancel */
		command_rep = 0;

		/* Redraw the state (later) */
		p_ptr->redraw |= (PR_STATE);
	}
#endif

	/* Cancel Resting */
	if (p_ptr->resting)
	{
		/* Cancel */
		p_ptr->resting = 0;

		/* Redraw the state (later) */
		p_ptr->redraw |= (PR_STATE);
	}

	/* Cancel running */
	if (p_ptr->running)
	{
		/* Cancel */
		p_ptr->running = 0;

		/* Calculate torch radius */
		p_ptr->update |= (PU_TORCH);
	}

	/* Cancel searching if requested */
	if (stop_search && p_ptr->searching)
	{
		/* Cancel */
		p_ptr->searching = FALSE;

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);
	}
}





/*
 * Hack -- Check if a level is a "quest" level
 */
/* FIXME - use worldpos and dungeon array! */
#ifdef NEW_DUNGEON
bool is_quest(struct worldpos *wpos)
#else
bool is_quest(int level)
#endif
{
	/* not implemented yet :p */
	return (FALSE);
#if 0
	int i;

	/* Town is never a quest */
	if (!level) return (FALSE);

	/* Check quests */
	for (i = 0; i < MAX_Q_IDX; i++)
	{
		/* Check for quest */
		if (q_list[i].level == level) return (TRUE);
	}

	/* Nope */
	return (FALSE);
#endif
}
