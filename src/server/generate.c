/* File: generate.c */

/* Purpose: Dungeon generation */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define SERVER

#include "angband.h"


/*
 * Note that Level generation is *not* an important bottleneck,
 * though it can be annoyingly slow on older machines...  Thus
 * we emphasize "simplicity" and "correctness" over "speed".
 *
 * This entire file is only needed for generating levels.
 * This may allow smart compilers to only load it when needed.
 *
 * Consider the "v_info.txt" file for vault generation.
 *
 * In this file, we use the "special" granite and perma-wall sub-types,
 * where "basic" is normal, "inner" is inside a room, "outer" is the
 * outer wall of a room, and "solid" is the outer wall of the dungeon
 * or any walls that may not be pierced by corridors.  Thus the only
 * wall type that may be pierced by a corridor is the "outer granite"
 * type.  The "basic granite" type yields the "actual" corridors.
 *
 * Note that we use the special "solid" granite wall type to prevent
 * multiple corridors from piercing a wall in two adjacent locations,
 * which would be messy, and we use the special "outer" granite wall
 * to indicate which walls "surround" rooms, and may thus be "pierced"
 * by corridors entering or leaving the room.
 *
 * Note that a tunnel which attempts to leave a room near the "edge"
 * of the dungeon in a direction toward that edge will cause "silly"
 * wall piercings, but will have no permanently incorrect effects,
 * as long as the tunnel can *eventually* exit from another side.
 * And note that the wall may not come back into the room by the
 * hole it left through, so it must bend to the left or right and
 * then optionally re-enter the room (at least 2 grids away).  This
 * is not a problem since every room that is large enough to block
 * the passage of tunnels is also large enough to allow the tunnel
 * to pierce the room itself several times.
 *
 * Note that no two corridors may enter a room through adjacent grids,
 * they must either share an entryway or else use entryways at least
 * two grids apart.  This prevents "large" (or "silly") doorways.
 *
 * To create rooms in the dungeon, we first divide the dungeon up
 * into "blocks" of 11x11 grids each, and require that all rooms
 * occupy a rectangular group of blocks.  As long as each room type
 * reserves a sufficient number of blocks, the room building routines
 * will not need to check bounds.  Note that most of the normal rooms
 * actually only use 23x11 grids, and so reserve 33x11 grids.
 *
 * Note that the use of 11x11 blocks (instead of the old 33x11 blocks)
 * allows more variability in the horizontal placement of rooms, and
 * at the same time has the disadvantage that some rooms (two thirds
 * of the normal rooms) may be "split" by panel boundaries.  This can
 * induce a situation where a player is in a room and part of the room
 * is off the screen.  It may be annoying enough to go back to 33x11
 * blocks to prevent this visual situation.
 *
 * Note that the dungeon generation routines are much different (2.7.5)
 * and perhaps "DUN_ROOMS" should be less than 50.
 *
 * XXX XXX XXX Note that it is possible to create a room which is only
 * connected to itself, because the "tunnel generation" code allows a
 * tunnel to leave a room, wander around, and then re-enter the room.
 *
 * XXX XXX XXX Note that it is possible to create a set of rooms which
 * are only connected to other rooms in that set, since there is nothing
 * explicit in the code to prevent this from happening.  But this is less
 * likely than the "isolated room" problem, because each room attempts to
 * connect to another room, in a giant cycle, thus requiring at least two
 * bizarre occurances to create an isolated section of the dungeon.
 *
 * Note that (2.7.9) monster pits have been split into monster "nests"
 * and monster "pits".  The "nests" have a collection of monsters of a
 * given type strewn randomly around the room (jelly, animal, or undead),
 * while the "pits" have a collection of monsters of a given type placed
 * around the room in an organized manner (orc, troll, giant, dragon, or
 * demon).  Note that both "nests" and "pits" are now "level dependant",
 * and both make 16 "expensive" calls to the "get_mon_num()" function.
 *
 * Note that the cave grid flags changed in a rather drastic manner
 * for Angband 2.8.0 (and 2.7.9+), in particular, dungeon terrain
 * features, such as doors and stairs and traps and rubble and walls,
 * are all handled as a set of 64 possible "terrain features", and
 * not as "fake" objects (440-479) as in pre-2.8.0 versions.
 *
 * The 64 new "dungeon features" will also be used for "visual display"
 * but we must be careful not to allow, for example, the user to display
 * hidden traps in a different way from floors, or secret doors in a way
 * different from granite walls, or even permanent granite in a different
 * way from granite.  XXX XXX XXX
 */


/*
 * Dungeon generation values
 */
#define DUN_ROOMS	50	/* Number of rooms to attempt */
#define DUN_UNUSUAL	150	/* Level/chance of unusual room */
#define DUN_DEST	15	/* 1/chance of having a destroyed level */

/*
 * Dungeon tunnel generation values
 */
#define DUN_TUN_RND	10	/* Chance of random direction */
#define DUN_TUN_CHG	30	/* Chance of changing direction */
#define DUN_TUN_CON	15	/* Chance of extra tunneling */
#define DUN_TUN_PEN	25	/* Chance of doors at room entrances */
#define DUN_TUN_JCT	90	/* Chance of doors at tunnel junctions */

/*
 * Dungeon streamer generation values
 */
#define DUN_STR_DEN	5	/* Density of streamers */
#define DUN_STR_RNG	2	/* Width of streamers */
#define DUN_STR_MAG	3	/* Number of magma streamers */
#define DUN_STR_MC	90	/* 1/chance of treasure per magma */
#define DUN_STR_QUA	2	/* Number of quartz streamers */
#define DUN_STR_QC	40	/* 1/chance of treasure per quartz */

/*
 * Dungeon treausre allocation values
 */
#define DUN_AMT_ROOM	9	/* Amount of objects for rooms */
#define DUN_AMT_ITEM	3	/* Amount of objects for rooms/corridors */
#define DUN_AMT_GOLD	3	/* Amount of treasure for rooms/corridors */

/*
 * Hack -- Dungeon allocation "places"
 */
#define ALLOC_SET_CORR		1	/* Hallway */
#define ALLOC_SET_ROOM		2	/* Room */
#define ALLOC_SET_BOTH		3	/* Anywhere */

/*
 * Hack -- Dungeon allocation "types"
 */
#define ALLOC_TYP_RUBBLE	1	/* Rubble */
#define ALLOC_TYP_TRAP		3	/* Trap */
#define ALLOC_TYP_GOLD		4	/* Gold */
#define ALLOC_TYP_OBJECT	5	/* Object */



/*
 * The "size" of a "generation block" in grids
 */
#define BLOCK_HGT	11
#define BLOCK_WID	11

/*
 * Maximum numbers of rooms along each axis (currently 6x6)
 */
#define MAX_ROOMS_ROW	(MAX_HGT / BLOCK_HGT)
#define MAX_ROOMS_COL	(MAX_WID / BLOCK_WID)


/*
 * Bounds on some arrays used in the "dun_data" structure.
 * These bounds are checked, though usually this is a formality.
 */
#define CENT_MAX	100
#define DOOR_MAX	400
#define WALL_MAX	1000
#define TUNN_MAX	1800


/*
 * Maximal number of room types
 */
#define ROOM_MAX	9



/*
 * Simple structure to hold a map location
 */

typedef struct coord coord;

struct coord
{
	byte y;
	byte x;
};


/*
 * Room type information
 */

typedef struct room_data room_data;

struct room_data
{
	/* Required size in blocks */
	s16b dy1, dy2, dx1, dx2;

	/* Hack -- minimum level */
	s16b level;
};


/*
 * Structure to hold all "dungeon generation" data
 */

typedef struct dun_data dun_data;

struct dun_data
{
	/* Array of centers of rooms */
	int cent_n;
	coord cent[CENT_MAX];

	/* Array of possible door locations */
	int door_n;
	coord door[DOOR_MAX];

	/* Array of wall piercing locations */
	int wall_n;
	coord wall[WALL_MAX];

	/* Array of tunnel grids */
	int tunn_n;
	coord tunn[TUNN_MAX];

	/* Number of blocks along each axis */
	int row_rooms;
	int col_rooms;

	/* Array of which blocks are used */
	bool room_map[MAX_ROOMS_ROW][MAX_ROOMS_COL];

	/* Hack -- there is a pit/nest on this level */
	bool crowded;
};


/*
 * Dungeon generation data -- see "cave_gen()"
 */
static dun_data *dun;


/*
 * Array of room types (assumes 11x11 blocks)
 */
static room_data room[ROOM_MAX] =
{
	{ 0, 0, 0, 0, 0 },		/* 0 = Nothing */
	{ 0, 0, -1, 1, 1 },		/* 1 = Simple (33x11) */
	{ 0, 0, -1, 1, 1 },		/* 2 = Overlapping (33x11) */
	{ 0, 0, -1, 1, 3 },		/* 3 = Crossed (33x11) */
	{ 0, 0, -1, 1, 3 },		/* 4 = Large (33x11) */
	{ 0, 0, -1, 1, 5 },		/* 5 = Monster nest (33x11) */
	{ 0, 0, -1, 1, 5 },		/* 6 = Monster pit (33x11) */
	{ 0, 1, -1, 1, 5 },		/* 7 = Lesser vault (33x22) */
	{ -1, 2, -2, 3, 10 }	/* 8 = Greater vault (66x44) */
};



/*
 * Always picks a correct direction
 */
static void correct_dir(int *rdir, int *cdir, int y1, int x1, int y2, int x2)
{
	/* Extract vertical and horizontal directions */
	*rdir = (y1 == y2) ? 0 : (y1 < y2) ? 1 : -1;
	*cdir = (x1 == x2) ? 0 : (x1 < x2) ? 1 : -1;

	/* Never move diagonally */
	if (*rdir && *cdir)
	{
		if (rand_int(100) < 50)
		{
			*rdir = 0;
		}
		else
		{
			*cdir = 0;
		}
	}
}


/*
 * Pick a random direction
 */
static void rand_dir(int *rdir, int *cdir)
{
	/* Pick a random direction */
	int i = rand_int(4);

	/* Extract the dy/dx components */
	*rdir = ddy_ddd[i];
	*cdir = ddx_ddd[i];
}


/*
 * Returns random co-ordinates for player starts
 */
#ifdef NEW_DUNGEON
static void new_player_spot(struct worldpos *wpos)
#else
static void new_player_spot(int Depth)
#endif
{
	int        y, x;

#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif

	/* Place the player */
	while (1)
	{
		/* Pick a legal spot */
		y = rand_range(1, MAX_HGT - 2);
		x = rand_range(1, MAX_WID - 2);

		/* Must be a "naked" floor grid */
#ifdef NEW_DUNGEON
		if (!cave_naked_bold(zcave, y, x)) continue;
#else
		if (!cave_naked_bold(Depth, y, x)) continue;
#endif

		/* Refuse to start on anti-teleport grids */
#ifdef NEW_DUNGEON
		if (zcave[y][x].info & CAVE_ICKY) continue;
#else
		if (cave[Depth][y][x].info & CAVE_ICKY) continue;
#endif

		/* Done */
		break;
	}

	/* Save the new grid */
#ifdef NEW_DUNGEON
	new_level_rand_y(wpos, y);
	new_level_rand_x(wpos, x);
#else
	level_rand_y[Depth] = y;
	level_rand_x[Depth] = x;
#endif
}



/*
 * Count the number of walls adjacent to the given grid.
 *
 * Note -- Assumes "in_bounds(y, x)"
 *
 * We count only granite walls and permanent walls.
 */
#ifdef NEW_DUNGEON
static int next_to_walls(struct worldpos *wpos, int y, int x)
#else
static int next_to_walls(int Depth, int y, int x)
#endif
{
	int        k = 0;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);

	if (zcave[y+1][x].feat >= FEAT_WALL_EXTRA) k++;
	if (zcave[y-1][x].feat >= FEAT_WALL_EXTRA) k++;
	if (zcave[y][x+1].feat >= FEAT_WALL_EXTRA) k++;
	if (zcave[y][x-1].feat >= FEAT_WALL_EXTRA) k++;
#else
	if (cave[Depth][y+1][x].feat >= FEAT_WALL_EXTRA) k++;
	if (cave[Depth][y-1][x].feat >= FEAT_WALL_EXTRA) k++;
	if (cave[Depth][y][x+1].feat >= FEAT_WALL_EXTRA) k++;
	if (cave[Depth][y][x-1].feat >= FEAT_WALL_EXTRA) k++;
#endif

	return (k);
}



/*
 * Convert existing terrain type to rubble
 */
#ifdef NEW_DUNGEON
static void place_rubble(struct worldpos *wpos, int y, int x)
#else
static void place_rubble(int Depth, int y, int x)
#endif
{
#ifdef NEW_DUNGEON
	cave_type **zcave;
	cave_type *c_ptr;
	if(!(zcave=getcave(wpos))) return;
	c_ptr=&zcave[y][x];
#else
	cave_type *c_ptr = &cave[Depth][y][x];
#endif

	/* Create rubble */
	c_ptr->feat = FEAT_RUBBLE;
}



/*
 * Convert existing terrain type to "up stairs"
 */
#ifdef NEW_DUNGEON
static void place_up_stairs(struct worldpos *wpos, int y, int x)
#else
static void place_up_stairs(int Depth, int y, int x)
#endif
{
#ifdef NEW_DUNGEON
	cave_type **zcave;
	cave_type *c_ptr;
	if(!(zcave=getcave(wpos))) return;
	c_ptr=&zcave[y][x];
#else
	cave_type *c_ptr = &cave[Depth][y][x];
#endif

	/* Create up stairs */
	c_ptr->feat = FEAT_LESS;
}


/*
 * Convert existing terrain type to "down stairs"
 */
#ifdef NEW_DUNGEON
static void place_down_stairs(int wpos, int y, int x)
#else
static void place_down_stairs(int Depth, int y, int x)
#endif
{
#ifdef NEW_DUNGEON
	cave_type **zcave;
	cave_type *c_ptr;
	if(!(zcave=getcave(wpos))) return;
	c_ptr=&zcave[y][x];
#else
	cave_type *c_ptr = &cave[Depth][y][x];
#endif

	/* Create down stairs */
	c_ptr->feat = FEAT_MORE;
}





/*
 * Place an up/down staircase at given location
 */
#ifdef NEW_DUNGEON
static void place_random_stairs(struct worldpos *wpos, int y, int x)
#else
static void place_random_stairs(int Depth, int y, int x)
#endif
{
#ifdef NEW_DUNGEON
	cave_type **zcave;
	cave_type *c_ptr;
	if(!(zcave=getcave(wpos))) return;
	c_ptr=&zcave[y][x];
	if(!cave_clean_bold(zcave, y, x)) return;
	if(!can_go_down(wpos) && can_go_up(wpos)){
		place_up_stairs(wpos, y, x);
	}
	else if(!can_go_up(wpos) && can_go_down(wpos)){
		place_down_stairs(wpos, y, x);
	}
	else if (rand_int(100) < 50)
	{
		place_down_stairs(wpos, y, x);
	}
	else
	{
		place_up_stairs(wpos, y, x);
	}
#else
	/* Paranoia */
	if (!cave_clean_bold(Depth, y, x)) return;

	/* Choose a staircase */
	if (!Depth)
	{
		place_down_stairs(Depth, y, x);
	}
	else if (is_quest(Depth) || (Depth >= MAX_DEPTH-1))
	{
		place_up_stairs(Depth, y, x);
	}
	else if (rand_int(100) < 50)
	{
		place_down_stairs(Depth, y, x);
	}
	else
	{
		place_up_stairs(Depth, y, x);
	}
#endif
}


/*
 * Place a locked door at the given location
 */
#ifdef NEW_DUNGEON
static void place_locked_door(struct worldpos *wpos, int y, int x)
#else
static void place_locked_door(int Depth, int y, int x)
#endif
{
#ifdef NEW_DUNGEON
	cave_type **zcave;
	cave_type *c_ptr;
	if(!(zcave=getcave(wpos))) return;
	c_ptr = &zcave[y][x];
#else
	cave_type *c_ptr = &cave[Depth][y][x];
#endif

	/* Create locked door */
	c_ptr->feat = FEAT_DOOR_HEAD + randint(7);
}


/*
 * Place a secret door at the given location
 */
#ifdef NEW_DUNGEON
static void place_secret_door(struct worldpos *wpos, int y, int x)
#else
static void place_secret_door(int Depth, int y, int x)
#endif
{
#ifdef NEW_DUNGEON
	cave_type **zcave;
	cave_type *c_ptr;
	if(!(zcave=getcave(wpos))) return;
	c_ptr = &zcave[y][x];
#else
	cave_type *c_ptr = &cave[Depth][y][x];
#endif

	/* Create secret door */
	c_ptr->feat = FEAT_SECRET;
}


/*
 * Place a random type of door at the given location
 */
#ifdef NEW_DUNGEON
static void place_random_door(struct worldpos *wpos, int y, int x)
#else
static void place_random_door(int Depth, int y, int x)
#endif
{
	int tmp;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	cave_type *c_ptr;
	if(!(zcave=getcave(wpos))) return;
	c_ptr = &zcave[y][x];
#else
	cave_type *c_ptr = &cave[Depth][y][x];
#endif

	/* Choose an object */
	tmp = rand_int(1000);

	/* Open doors (300/1000) */
	if (tmp < 300)
	{
		/* Create open door */
		c_ptr->feat = FEAT_OPEN;
	}

	/* Broken doors (100/1000) */
	else if (tmp < 400)
	{
		/* Create broken door */
		c_ptr->feat = FEAT_BROKEN;
	}

	/* Secret doors (200/1000) */
	else if (tmp < 600)
	{
		/* Create secret door */
		c_ptr->feat = FEAT_SECRET;
	}

	/* Closed doors (300/1000) */
	else if (tmp < 900)
	{
		/* Create closed door */
		c_ptr->feat = FEAT_DOOR_HEAD + 0x00;
	}

	/* Locked doors (99/1000) */
	else if (tmp < 999)
	{
		/* Create locked door */
		c_ptr->feat = FEAT_DOOR_HEAD + randint(7);
	}

	/* Stuck doors (1/1000) */
	else
	{
		/* Create jammed door */
		c_ptr->feat = FEAT_DOOR_HEAD + 0x08 + rand_int(8);
	}
}



/*
 * Places some staircases near walls
 */
#ifdef NEW_DUNGEON
static void alloc_stairs(struct worldpos *wpos, int feat, int num, int walls)
#else
static void alloc_stairs(int Depth, int feat, int num, int walls)
#endif
{
	int                 y, x, i, j, flag;

	cave_type		*c_ptr;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif

	/* Place "num" stairs */
	for (i = 0; i < num; i++)
	{
		/* Place some stairs */
		for (flag = FALSE; !flag; )
		{
			/* Try several times, then decrease "walls" */
			for (j = 0; !flag && j <= 3000; j++)
			{
				/* Pick a random grid */
#ifdef NEW_DUNGEON
				y = rand_int(MAX_HGT);
				x = rand_int(MAX_WID);

				/* Require "naked" floor grid */
				if (!cave_naked_bold(zcave, y, x)) continue;

				/* Require a certain number of adjacent walls */
				if (next_to_walls(wpos, y, x) < walls) continue;

				/* Access the grid */
				c_ptr = &zcave[y][x];

				/* Town -- must go down */
				if (!can_go_up(wpos))
#else
				y = rand_int(Depth ? MAX_HGT : MAX_HGT);
				x = rand_int(Depth ? MAX_WID : MAX_WID);

				/* Require "naked" floor grid */
				if (!cave_naked_bold(Depth, y, x)) continue;

				/* Require a certain number of adjacent walls */
				if (next_to_walls(Depth, y, x) < walls) continue;

				/* Access the grid */
				c_ptr = &cave[Depth][y][x];

				/* Town -- must go down */
				if (!Depth)
#endif
				{
					/* Clear previous contents, add down stairs */
					c_ptr->feat = FEAT_MORE;
					if(!istown(wpos)){
						new_level_up_y(wpos,y);
						new_level_up_x(wpos,x);
					}
				}

#ifdef NEW_DUNGEON
				/* Quest -- must go up */
				else if (is_quest(wpos) || !can_go_down(wpos))
#else
				/* Quest -- must go up */
				else if (is_quest(Depth) || (Depth >= MAX_DEPTH-1))
#endif
				{
					/* Clear previous contents, add up stairs */
					c_ptr->feat = FEAT_LESS;

					/* Set this to be the starting location for people going down */
#ifdef NEW_DUNGEON
					new_level_down_y(wpos,y);
					new_level_down_x(wpos,x);
#else
					level_down_y[Depth] = y;
					level_down_x[Depth] = x;
#endif
				}

				/* Requested type */
				else
				{
					/* Clear previous contents, add stairs */
					c_ptr->feat = feat;

					if (feat == FEAT_LESS)
					{
						/* Set this to be the starting location for people going down */
#ifdef NEW_DUNGEON
						new_level_down_y(wpos, y);
						new_level_down_x(wpos, x);
#else
						level_down_y[Depth] = y;
						level_down_x[Depth] = x;
#endif
					}
					if (feat == FEAT_MORE)
					{
						/* Set this to be the starting location for people going up */
#ifdef NEW_DUNGEON
						new_level_up_y(wpos, y);
						new_level_up_x(wpos, x);
#else
						level_up_y[Depth] = y;
						level_up_x[Depth] = x;
#endif
					}
				}

				/* All done */
				flag = TRUE;
			}

			/* Require fewer walls */
			if (walls) walls--;
		}
	}
}




/*
 * Allocates some objects (using "place" and "type")
 */
#ifdef NEW_DUNGEON
static void alloc_object(struct worldpos *wpos, int set, int typ, int num)
#else
static void alloc_object(int Depth, int set, int typ, int num)
#endif
{
	int y, x, k;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif

	/* Place some objects */
	for (k = 0; k < num; k++)
	{
		/* Pick a "legal" spot */
		while (TRUE)
		{
			bool room;

			/* Location */
#ifdef NEW_DUNGEON
			y = rand_int(MAX_HGT);
			x = rand_int(MAX_WID);
			/* Require "naked" floor grid */
			if (!cave_naked_bold(zcave, y, x)) continue;

			/* Check for "room" */
			room = (zcave[y][x].info & CAVE_ROOM) ? TRUE : FALSE;
#else
			y = rand_int(Depth ? MAX_HGT : MAX_HGT);
			x = rand_int(Depth ? MAX_WID : MAX_WID);
			/* Require "naked" floor grid */
			if (!cave_naked_bold(Depth, y, x)) continue;

			/* Check for "room" */
			room = (cave[Depth][y][x].info & CAVE_ROOM) ? TRUE : FALSE;
#endif
			/* Require corridor? */
			if ((set == ALLOC_SET_CORR) && room) continue;

			/* Require room? */
			if ((set == ALLOC_SET_ROOM) && !room) continue;

			/* Accept it */
			break;
		}

		/* Place something */
		switch (typ)
		{
#ifdef NEW_DUNGEON
			case ALLOC_TYP_RUBBLE:
			{
				place_rubble(wpos, y, x);
				break;
			}

			case ALLOC_TYP_TRAP:
			{
				place_trap(wpos, y, x);
				break;
			}

			case ALLOC_TYP_GOLD:
			{
				place_gold(wpos, y, x);
				break;
			}

			case ALLOC_TYP_OBJECT:
			{
				place_object(wpos, y, x, FALSE, FALSE);
				break;
			}
#else
			case ALLOC_TYP_RUBBLE:
			{
				place_rubble(Depth, y, x);
				break;
			}

			case ALLOC_TYP_TRAP:
			{
				place_trap(Depth, y, x);
				break;
			}

			case ALLOC_TYP_GOLD:
			{
				place_gold(Depth, y, x);
				break;
			}

			case ALLOC_TYP_OBJECT:
			{
				place_object(Depth, y, x, FALSE, FALSE);
				break;
			}
#endif
		}
	}
}



/*
 * Places "streamers" of rock through dungeon
 *
 * Note that their are actually six different terrain features used
 * to represent streamers.  Three each of magma and quartz, one for
 * basic vein, one with hidden gold, and one with known gold.  The
 * hidden gold types are currently unused.
 */
#ifdef NEW_DUNGEON
static void build_streamer(struct worldpos *wpos, int feat, int chance)
#else
static void build_streamer(int Depth, int feat, int chance)
#endif
{
	int		i, tx, ty;
	int		y, x, dir;
	cave_type *c_ptr;

#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif

	/* Hack -- Choose starting point */
#ifdef NEW_DUNGEON
	y = rand_spread(MAX_HGT / 2, 10);
	x = rand_spread(MAX_WID / 2, 15);
#else
	y = rand_spread((Depth ? MAX_HGT : MAX_HGT) / 2, 10);
	x = rand_spread((Depth ? MAX_WID : MAX_WID) / 2, 15);
#endif

	/* Choose a random compass direction */
	dir = ddd[rand_int(8)];

	/* Place streamer into dungeon */
	while (TRUE)
	{
		/* One grid per density */
		for (i = 0; i < DUN_STR_DEN; i++)
		{
			int d = DUN_STR_RNG;

			/* Pick a nearby grid */
			while (1)
			{
				ty = rand_spread(y, d);
				tx = rand_spread(x, d);
#ifdef NEW_DUNGEON
				if (!in_bounds2(wpos, ty, tx)) continue;
#else
				if (!in_bounds2(Depth, ty, tx)) continue;
#endif
				break;
			}

#ifdef NEW_DUNGEON
			/* Access the grid */
			c_ptr = &zcave[ty][tx];
#else
			/* Access the grid */
			c_ptr = &cave[Depth][ty][tx];
#endif

			/* Only convert "granite" walls */
			if (c_ptr->feat < FEAT_WALL_EXTRA) continue;
			if (c_ptr->feat > FEAT_WALL_SOLID) continue;

			/* Clear previous contents, add proper vein type */
			c_ptr->feat = feat;

			/* Hack -- Add some (known) treasure */
			if (rand_int(chance) == 0) c_ptr->feat += 0x04;
		}

		/* Advance the streamer */
		y += ddy[dir];
		x += ddx[dir];

		/* Quit before leaving the dungeon */
#ifdef NEW_DUNGEON
		if (!in_bounds(y, x)) break;
#else
		if (!in_bounds(Depth, y, x)) break;
#endif
	}
}


/*
 * Build a destroyed level
 */
#ifdef NEW_DUNGEON
static void destroy_level(struct worldpos *wpos)
#else
static void destroy_level(int Depth)
#endif
{
	int y1, x1, y, x, k, t, n;
	cave_type *c_ptr;

#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif

	/* Note destroyed levels */
	/*if (cheat_room) msg_print("Destroyed Level");*/

	/* Drop a few epi-centers (usually about two) */
	for (n = 0; n < randint(5); n++)
	{
		/* Pick an epi-center */
#ifdef NEW_DUNGEON
		y1 = rand_range(5, MAX_HGT - 6);
		x1 = rand_range(5, MAX_WID - 6);
#else
		y1 = rand_range(5, (Depth ? MAX_HGT : MAX_HGT) - 6);
		x1 = rand_range(5, (Depth ? MAX_WID : MAX_WID) - 6);
#endif

		/* Big area of affect */
		for (y = (y1 - 15); y <= (y1 + 15); y++)
		{
			for (x = (x1 - 15); x <= (x1 + 15); x++)
			{
				/* Skip illegal grids */
#ifdef NEW_DUNGEON
				if (!in_bounds(y, x)) continue;
#else
				if (!in_bounds(Depth, y, x)) continue;
#endif

				/* Extract the distance */
				k = distance(y1, x1, y, x);

				/* Stay in the circle of death */
				if (k >= 16) continue;

#ifdef NEW_DUNGEON
				/* Delete the monster (if any) */
				delete_monster(wpos, y, x);

				/* Destroy valid grids */
				if (cave_valid_bold(zcave, y, x))
				{
					/* Delete the object (if any) */
					delete_object(wpos, y, x);

					/* Access the grid */
					c_ptr = &zcave[y][x];
#else
				/* Delete the monster (if any) */
				delete_monster(Depth, y, x);

				/* Destroy valid grids */
				if (cave_valid_bold(Depth, y, x))
				{
					/* Delete the object (if any) */
					delete_object(Depth, y, x);

					/* Access the grid */
					c_ptr = &cave[Depth][y][x];
#endif

					/* Wall (or floor) type */
					t = rand_int(200);

					/* Granite */
					if (t < 20)
					{
						/* Create granite wall */
						c_ptr->feat = FEAT_WALL_EXTRA;
					}

					/* Quartz */
					else if (t < 70)
					{
						/* Create quartz vein */
						c_ptr->feat = FEAT_QUARTZ;
					}

					/* Magma */
					else if (t < 100)
					{
						/* Create magma vein */
						c_ptr->feat = FEAT_MAGMA;
					}

					/* Floor */
					else
					{
						/* Create floor */
						c_ptr->feat = FEAT_FLOOR;
					}

					/* No longer part of a room or vault */
					c_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

					/* No longer illuminated or known */
					c_ptr->info &= ~CAVE_GLOW;
				}
			}
		}
	}
}



/*
 * Create up to "num" objects near the given coordinates
 * Only really called by some of the "vault" routines.
 */
#ifdef NEW_DUNGEON
static void vault_objects(struct worldpos *wpos, int y, int x, int num)
#else
static void vault_objects(int Depth, int y, int x, int num)
#endif
{
	int        i, j, k;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif

	/* Attempt to place 'num' objects */
	for (; num > 0; --num)
	{
		/* Try up to 11 spots looking for empty space */
		for (i = 0; i < 11; ++i)
		{
			/* Pick a random location */
			while (1)
			{
				j = rand_spread(y, 2);
				k = rand_spread(x, 3);
#ifdef NEW_DUNGEON
				if (!in_bounds(j, k)) continue;
#else
				if (!in_bounds(Depth, j, k)) continue;
#endif
				break;
			}

#ifdef NEW_DUNGEON
			/* Require "clean" floor space */
			if (!cave_clean_bold(zcave, j, k)) continue;
#else
			/* Require "clean" floor space */
			if (!cave_clean_bold(Depth, j, k)) continue;
#endif

			/* Place an item */
			if (rand_int(100) < 75)
#ifdef NEW_DUNGEON
			{
				place_object(wpos, j, k, FALSE, FALSE);
			}

			/* Place gold */
			else
			{
				place_gold(wpos, j, k);
			}
#else
			{
				place_object(Depth, j, k, FALSE, FALSE);
			}

			/* Place gold */
			else
			{
				place_gold(Depth, j, k);
			}
#endif

			/* Placement accomplished */
			break;
		}
	}
}


/*
 * Place a trap with a given displacement of point
 */
#ifdef NEW_DUNGEON
static void vault_trap_aux(struct worldpos *wpos, int y, int x, int yd, int xd)
#else
static void vault_trap_aux(int Depth, int y, int x, int yd, int xd)
#endif
{
	int		count, y1, x1;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif

	/* Place traps */
	for (count = 0; count <= 5; count++)
	{
		/* Get a location */
		while (1)
		{
			y1 = rand_spread(y, yd);
			x1 = rand_spread(x, xd);
#ifdef NEW_DUNGEON
			if (!in_bounds(y1, x1)) continue;
#else
			if (!in_bounds(Depth, y1, x1)) continue;
#endif
			break;
		}

		/* Require "naked" floor grids */
#ifdef NEW_DUNGEON
		if (!cave_naked_bold(zcave, y1, x1)) continue;

		/* Place the trap */
		place_trap(wpos, y1, x1);
#else
		if (!cave_naked_bold(Depth, y1, x1)) continue;

		/* Place the trap */
		place_trap(Depth, y1, x1);
#endif

		/* Done */
		break;
	}
}


/*
 * Place some traps with a given displacement of given location
 */
#ifdef NEW_DUNGEON
static void vault_traps(struct worldpos *wpos, int y, int x, int yd, int xd, int num)
#else
static void vault_traps(int Depth, int y, int x, int yd, int xd, int num)
#endif
{
	int i;

	for (i = 0; i < num; i++)
	{
#ifdef NEW_DUNGEON
		vault_trap_aux(wpos, y, x, yd, xd);
#else
		vault_trap_aux(Depth, y, x, yd, xd);
#endif
	}
}


/*
 * Hack -- Place some sleeping monsters near the given location
 */
#ifdef NEW_DUNGEON
static void vault_monsters(struct worldpos *wpos, int y1, int x1, int num)
#else
static void vault_monsters(int Depth, int y1, int x1, int num)
#endif
{
	int          k, i, y, x;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif

	/* Try to summon "num" monsters "near" the given location */
	for (k = 0; k < num; k++)
	{
		/* Try nine locations */
		for (i = 0; i < 9; i++)
		{
			int d = 1;

#ifdef NEW_DUNGEON
			/* Pick a nearby location */
			scatter(wpos, &y, &x, y1, x1, d, 0);

			/* Require "empty" floor grids */
			if (!cave_empty_bold(zcave, y, x)) continue;

			/* Place the monster (allow groups) */
			monster_level = getlevel(wpos) + 2;
			(void)place_monster(wpos, y, x, TRUE, TRUE);
			monster_level = getlevel(wpos);
#else
			/* Pick a nearby location */
			scatter(Depth, &y, &x, y1, x1, d, 0);

			/* Require "empty" floor grids */
			if (!cave_empty_bold(Depth, y, x)) continue;

			/* Place the monster (allow groups) */
			monster_level = Depth + 2;
			(void)place_monster(Depth, y, x, TRUE, TRUE);
			monster_level = Depth;
#endif
		}
	}
}




/*
 * Room building routines.
 *
 * Six basic room types:
 *   1 -- normal
 *   2 -- overlapping
 *   3 -- cross shaped
 *   4 -- large room with features
 *   5 -- monster nests
 *   6 -- monster pits
 *   7 -- simple vaults
 *   8 -- greater vaults
 */


/*
 * Type 1 -- normal rectangular rooms
 */
#ifdef NEW_DUNGEON
static void build_type1(struct worldpos *wpos, int yval, int xval)
#else
static void build_type1(int Depth, int yval, int xval)
#endif
{
	int			y, x, y2, x2;
	int                 y1, x1;

	bool		light;
	cave_type *c_ptr;

#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif

	/* Choose lite or dark */
#ifdef NEW_DUNGEON
	light = (getlevel(wpos) <= randint(25));
#else
	light = (Depth <= randint(25));
#endif


	/* Pick a room size */
	y1 = yval - randint(4);
	y2 = yval + randint(3);
	x1 = xval - randint(11);
	x2 = xval + randint(11);


	/* Place a full floor under the room */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		for (x = x1 - 1; x <= x2 + 1; x++)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][x];
#else
			c_ptr = &cave[Depth][y][x];
#endif
			c_ptr->feat = FEAT_FLOOR;
			c_ptr->info |= CAVE_ROOM;
			if (light) c_ptr->info |= CAVE_GLOW;
		}
	}

	/* Walls around the room */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x1-1];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &zcave[y][x2+1];
		c_ptr->feat = FEAT_WALL_OUTER;
#else
		c_ptr = &cave[Depth][y][x1-1];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &cave[Depth][y][x2+1];
		c_ptr->feat = FEAT_WALL_OUTER;
#endif
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y1-1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &zcave[y2+1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
#else
		c_ptr = &cave[Depth][y1-1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &cave[Depth][y2+1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
#endif
	}


	/* Hack -- Occasional pillar room */
	if (rand_int(20) == 0)
	{
		for (y = y1; y <= y2; y += 2)
		{
			for (x = x1; x <= x2; x += 2)
			{
#ifdef NEW_DUNGEON
				c_ptr = &zcave[y][x];
				c_ptr->feat = FEAT_WALL_INNER;
#else
				c_ptr = &cave[Depth][y][x];
				c_ptr->feat = FEAT_WALL_INNER;
#endif
			}
		}
	}

	/* Hack -- Occasional ragged-edge room */
	else if (rand_int(50) == 0)
	{
		for (y = y1 + 2; y <= y2 - 2; y += 2)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][x1];
			c_ptr->feat = FEAT_WALL_INNER;
			c_ptr = &zcave[y][x2];
			c_ptr->feat = FEAT_WALL_INNER;
#else
			c_ptr = &cave[Depth][y][x1];
			c_ptr->feat = FEAT_WALL_INNER;
			c_ptr = &cave[Depth][y][x2];
			c_ptr->feat = FEAT_WALL_INNER;
#endif
		}
		for (x = x1 + 2; x <= x2 - 2; x += 2)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y1][x];
			c_ptr->feat = FEAT_WALL_INNER;
			c_ptr = &zcave[y2][x];
			c_ptr->feat = FEAT_WALL_INNER;
#else
			c_ptr = &cave[Depth][y1][x];
			c_ptr->feat = FEAT_WALL_INNER;
			c_ptr = &cave[Depth][y2][x];
			c_ptr->feat = FEAT_WALL_INNER;
#endif
		}
	}
}


/*
 * Type 2 -- Overlapping rectangular rooms
 */
#ifdef NEW_DUNGEON
static void build_type2(struct worldpos *wpos, int yval, int xval)
#else
static void build_type2(int Depth, int yval, int xval)
#endif
{
	int			y, x;
	int			y1a, x1a, y2a, x2a;
	int			y1b, x1b, y2b, x2b;

	bool		light;
	cave_type *c_ptr;

#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif

	/* Choose lite or dark */
#ifdef NEW_DUNGEON
	light = (getlevel(wpos) <= randint(25));
#else
	light = (Depth <= randint(25));
#endif


	/* Determine extents of the first room */
	y1a = yval - randint(4);
	y2a = yval + randint(3);
	x1a = xval - randint(11);
	x2a = xval + randint(10);

	/* Determine extents of the second room */
	y1b = yval - randint(3);
	y2b = yval + randint(4);
	x1b = xval - randint(10);
	x2b = xval + randint(11);


	/* Place a full floor for room "a" */
	for (y = y1a - 1; y <= y2a + 1; y++)
	{
		for (x = x1a - 1; x <= x2a + 1; x++)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][x];
#else
			c_ptr = &cave[Depth][y][x];
#endif
			c_ptr->feat = FEAT_FLOOR;
			c_ptr->info |= CAVE_ROOM;
			if (light) c_ptr->info |= CAVE_GLOW;
		}
	}

	/* Place a full floor for room "b" */
	for (y = y1b - 1; y <= y2b + 1; y++)
	{
		for (x = x1b - 1; x <= x2b + 1; x++)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][x];
#else
			c_ptr = &cave[Depth][y][x];
#endif
			c_ptr->feat = FEAT_FLOOR;
			c_ptr->info |= CAVE_ROOM;
			if (light) c_ptr->info |= CAVE_GLOW;
		}
	}


	/* Place the walls around room "a" */
	for (y = y1a - 1; y <= y2a + 1; y++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x1a-1];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &zcave[y][x2a+1];
		c_ptr->feat = FEAT_WALL_OUTER;
#else
		c_ptr = &cave[Depth][y][x1a-1];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &cave[Depth][y][x2a+1];
		c_ptr->feat = FEAT_WALL_OUTER;
#endif
	}
	for (x = x1a - 1; x <= x2a + 1; x++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y1a-1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &zcave[y2a+1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
#else
		c_ptr = &cave[Depth][y1a-1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &cave[Depth][y2a+1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
#endif
	}

	/* Place the walls around room "b" */
	for (y = y1b - 1; y <= y2b + 1; y++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x1b-1];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &zcave[y][x2b+1];
		c_ptr->feat = FEAT_WALL_OUTER;
#else
		c_ptr = &cave[Depth][y][x1b-1];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &cave[Depth][y][x2b+1];
		c_ptr->feat = FEAT_WALL_OUTER;
#endif
	}
	for (x = x1b - 1; x <= x2b + 1; x++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y1b-1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &zcave[y2b+1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
#else
		c_ptr = &cave[Depth][y1b-1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &cave[Depth][y2b+1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
#endif
	}



	/* Replace the floor for room "a" */
	for (y = y1a; y <= y2a; y++)
	{
		for (x = x1a; x <= x2a; x++)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][x];
#else
			c_ptr = &cave[Depth][y][x];
#endif
			c_ptr->feat = FEAT_FLOOR;
		}
	}

	/* Replace the floor for room "b" */
	for (y = y1b; y <= y2b; y++)
	{
		for (x = x1b; x <= x2b; x++)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][x];
#else
			c_ptr = &cave[Depth][y][x];
#endif
			c_ptr->feat = FEAT_FLOOR;
		}
	}
}



/*
 * Type 3 -- Cross shaped rooms
 *
 * Builds a room at a row, column coordinate
 *
 * Room "a" runs north/south, and Room "b" runs east/east
 * So the "central pillar" runs from x1a,y1b to x2a,y2b.
 *
 * Note that currently, the "center" is always 3x3, but I think that
 * the code below will work (with "bounds checking") for 5x5, or even
 * for unsymetric values like 4x3 or 5x3 or 3x4 or 3x5, or even larger.
 */
#ifdef NEW_DUNGEON
static void build_type3(struct worldpos *wpos, int yval, int xval)
#else
static void build_type3(int Depth, int yval, int xval)
#endif
{
	int			y, x, dy, dx, wy, wx;
	int			y1a, x1a, y2a, x2a;
	int			y1b, x1b, y2b, x2b;

	bool		light;
	cave_type *c_ptr;

#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif

	/* Choose lite or dark */
#ifdef NEW_DUNGEON
	light = (getlevel(wpos) <= randint(25));
#else
	light = (Depth <= randint(25));
#endif


	/* For now, always 3x3 */
	wx = wy = 1;

	/* Pick max vertical size (at most 4) */
	dy = rand_range(3, 4);

	/* Pick max horizontal size (at most 15) */
	dx = rand_range(3, 11);


	/* Determine extents of the north/south room */
	y1a = yval - dy;
	y2a = yval + dy;
	x1a = xval - wx;
	x2a = xval + wx;

	/* Determine extents of the east/west room */
	y1b = yval - wy;
	y2b = yval + wy;
	x1b = xval - dx;
	x2b = xval + dx;


	/* Place a full floor for room "a" */
	for (y = y1a - 1; y <= y2a + 1; y++)
	{
		for (x = x1a - 1; x <= x2a + 1; x++)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][x];
#else
			c_ptr = &cave[Depth][y][x];
#endif
			c_ptr->feat = FEAT_FLOOR;
			c_ptr->info |= CAVE_ROOM;
			if (light) c_ptr->info |= CAVE_GLOW;
		}
	}

	/* Place a full floor for room "b" */
	for (y = y1b - 1; y <= y2b + 1; y++)
	{
		for (x = x1b - 1; x <= x2b + 1; x++)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][x];
#else
			c_ptr = &cave[Depth][y][x];
#endif
			c_ptr->feat = FEAT_FLOOR;
			c_ptr->info |= CAVE_ROOM;
			if (light) c_ptr->info |= CAVE_GLOW;
		}
	}


	/* Place the walls around room "a" */
	for (y = y1a - 1; y <= y2a + 1; y++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x1a-1];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &zcave[y][x2a+1];
		c_ptr->feat = FEAT_WALL_OUTER;
#else
		c_ptr = &cave[Depth][y][x1a-1];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &cave[Depth][y][x2a+1];
		c_ptr->feat = FEAT_WALL_OUTER;
#endif
	}
	for (x = x1a - 1; x <= x2a + 1; x++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y1a-1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &zcave[y2a+1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
#else
		c_ptr = &cave[Depth][y1a-1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &cave[Depth][y2a+1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
#endif
	}

	/* Place the walls around room "b" */
	for (y = y1b - 1; y <= y2b + 1; y++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x1b-1];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &zcave[y][x2b+1];
		c_ptr->feat = FEAT_WALL_OUTER;
#else
		c_ptr = &cave[Depth][y][x1b-1];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &cave[Depth][y][x2b+1];
		c_ptr->feat = FEAT_WALL_OUTER;
#endif
	}
	for (x = x1b - 1; x <= x2b + 1; x++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y1b-1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &zcave[y2b+1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
#else
		c_ptr = &cave[Depth][y1b-1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &cave[Depth][y2b+1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
#endif
	}


	/* Replace the floor for room "a" */
	for (y = y1a; y <= y2a; y++)
	{
		for (x = x1a; x <= x2a; x++)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][x];
#else
			c_ptr = &cave[Depth][y][x];
#endif
			c_ptr->feat = FEAT_FLOOR;
		}
	}

	/* Replace the floor for room "b" */
	for (y = y1b; y <= y2b; y++)
	{
		for (x = x1b; x <= x2b; x++)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][x];
#else
			c_ptr = &cave[Depth][y][x];
#endif
			c_ptr->feat = FEAT_FLOOR;
		}
	}



	/* Special features (3/4) */
	switch (rand_int(4))
	{
		/* Large solid middle pillar */
		case 1:
		for (y = y1b; y <= y2b; y++)
		{
			for (x = x1a; x <= x2a; x++)
			{
#ifdef NEW_DUNGEON
				c_ptr = &zcave[y][x];
#else
				c_ptr = &cave[Depth][y][x];
#endif
				c_ptr->feat = FEAT_WALL_INNER;
			}
		}
		break;

		/* Inner treasure vault */
		case 2:

		/* Build the vault */
		for (y = y1b; y <= y2b; y++)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][x1a];
			c_ptr->feat = FEAT_WALL_INNER;
			c_ptr = &zcave[y][x2a];
			c_ptr->feat = FEAT_WALL_INNER;
#else
			c_ptr = &cave[Depth][y][x1a];
			c_ptr->feat = FEAT_WALL_INNER;
			c_ptr = &cave[Depth][y][x2a];
			c_ptr->feat = FEAT_WALL_INNER;
#endif
		}
		for (x = x1a; x <= x2a; x++)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y1b][x];
			c_ptr->feat = FEAT_WALL_INNER;
			c_ptr = &zcave[y2b][x];
			c_ptr->feat = FEAT_WALL_INNER;
#else
			c_ptr = &cave[Depth][y1b][x];
			c_ptr->feat = FEAT_WALL_INNER;
			c_ptr = &cave[Depth][y2b][x];
			c_ptr->feat = FEAT_WALL_INNER;
#endif
		}

		/* Place a secret door on the inner room */
		switch (rand_int(4))
		{
#ifdef NEW_DUNGEON
			case 0: place_secret_door(wpos, y1b, xval); break;
			case 1: place_secret_door(wpos, y2b, xval); break;
			case 2: place_secret_door(wpos, yval, x1a); break;
			case 3: place_secret_door(wpos, yval, x2a); break;
#else
			case 0: place_secret_door(Depth, y1b, xval); break;
			case 1: place_secret_door(Depth, y2b, xval); break;
			case 2: place_secret_door(Depth, yval, x1a); break;
			case 3: place_secret_door(Depth, yval, x2a); break;
#endif
		}

		/* Place a treasure in the vault */
#ifdef NEW_DUNGEON
		place_object(wpos, yval, xval, FALSE, FALSE);

		/* Let's guard the treasure well */
		vault_monsters(wpos, yval, xval, rand_int(2) + 3);

		/* Traps naturally */
		vault_traps(wpos, yval, xval, 4, 4, rand_int(3) + 2);
#else
		place_object(Depth, yval, xval, FALSE, FALSE);

		/* Let's guard the treasure well */
		vault_monsters(Depth, yval, xval, rand_int(2) + 3);

		/* Traps naturally */
		vault_traps(Depth, yval, xval, 4, 4, rand_int(3) + 2);
#endif

		break;


		/* Something else */
		case 3:

		/* Occasionally pinch the center shut */
		if (rand_int(3) == 0)
		{
			/* Pinch the east/west sides */
			for (y = y1b; y <= y2b; y++)
			{
				if (y == yval) continue;
#ifdef NEW_DUNGEON
				c_ptr = &zcave[y][x1a - 1];
				c_ptr->feat = FEAT_WALL_INNER;
				c_ptr = &zcave[y][x2a + 1];
				c_ptr->feat = FEAT_WALL_INNER;
#else
				c_ptr = &cave[Depth][y][x1a - 1];
				c_ptr->feat = FEAT_WALL_INNER;
				c_ptr = &cave[Depth][y][x2a + 1];
				c_ptr->feat = FEAT_WALL_INNER;
#endif
			}

			/* Pinch the north/south sides */
			for (x = x1a; x <= x2a; x++)
			{
				if (x == xval) continue;
#ifdef NEW_DUNGEON
				c_ptr = &zcave[y1b - 1][x];
				c_ptr->feat = FEAT_WALL_INNER;
				c_ptr = &zcave[y2b + 1][x];
				c_ptr->feat = FEAT_WALL_INNER;
#else
				c_ptr = &cave[Depth][y1b - 1][x];
				c_ptr->feat = FEAT_WALL_INNER;
				c_ptr = &cave[Depth][y2b + 1][x];
				c_ptr->feat = FEAT_WALL_INNER;
#endif
			}

			/* Sometimes shut using secret doors */
			if (rand_int(3) == 0)
			{
#ifdef NEW_DUNGEON
				place_secret_door(wpos, yval, x1a - 1);
				place_secret_door(wpos, yval, x2a + 1);
				place_secret_door(wpos, y1b - 1, xval);
				place_secret_door(wpos, y2b + 1, xval);
#else
				place_secret_door(Depth, yval, x1a - 1);
				place_secret_door(Depth, yval, x2a + 1);
				place_secret_door(Depth, y1b - 1, xval);
				place_secret_door(Depth, y2b + 1, xval);
#endif
			}
		}

		/* Occasionally put a "plus" in the center */
		else if (rand_int(3) == 0)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[yval][xval];
			c_ptr->feat = FEAT_WALL_INNER;
			c_ptr = &zcave[y1b][xval];
			c_ptr->feat = FEAT_WALL_INNER;
			c_ptr = &zcave[y2b][xval];
			c_ptr->feat = FEAT_WALL_INNER;
			c_ptr = &zcave[yval][x1a];
			c_ptr->feat = FEAT_WALL_INNER;
			c_ptr = &zcave[yval][x2a];
			c_ptr->feat = FEAT_WALL_INNER;
#else
			c_ptr = &cave[Depth][yval][xval];
			c_ptr->feat = FEAT_WALL_INNER;
			c_ptr = &cave[Depth][y1b][xval];
			c_ptr->feat = FEAT_WALL_INNER;
			c_ptr = &cave[Depth][y2b][xval];
			c_ptr->feat = FEAT_WALL_INNER;
			c_ptr = &cave[Depth][yval][x1a];
			c_ptr->feat = FEAT_WALL_INNER;
			c_ptr = &cave[Depth][yval][x2a];
			c_ptr->feat = FEAT_WALL_INNER;
#endif
		}

		/* Occasionally put a pillar in the center */
		else if (rand_int(3) == 0)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[yval][xval];
#else
			c_ptr = &cave[Depth][yval][xval];
#endif
			c_ptr->feat = FEAT_WALL_INNER;
		}

		break;
	}
}


/*
 * Type 4 -- Large room with inner features
 *
 * Possible sub-types:
 *	1 - Just an inner room with one door
 *	2 - An inner room within an inner room
 *	3 - An inner room with pillar(s)
 *	4 - Inner room has a maze
 *	5 - A set of four inner rooms
 */
#ifdef NEW_DUNGEON
static void build_type4(struct worldpos *wpos, int yval, int xval)
#else
static void build_type4(int Depth, int yval, int xval)
#endif
{
	int        y, x, y1, x1;
	int                 y2, x2, tmp;

	bool		light;
	cave_type *c_ptr;

#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif

	/* Choose lite or dark */
#ifdef NEW_DUNGEON
	light = (getlevel(wpos) <= randint(25));
#else
	light = (Depth <= randint(25));
#endif


	/* Large room */
	y1 = yval - 4;
	y2 = yval + 4;
	x1 = xval - 11;
	x2 = xval + 11;


	/* Place a full floor under the room */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		for (x = x1 - 1; x <= x2 + 1; x++)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][x];
#else
			c_ptr = &cave[Depth][y][x];
#endif
			c_ptr->feat = FEAT_FLOOR;
			c_ptr->info |= CAVE_ROOM;
			if (light) c_ptr->info |= CAVE_GLOW;
		}
	}

	/* Outer Walls */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x1-1];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &zcave[y][x2+1];
		c_ptr->feat = FEAT_WALL_OUTER;
#else
		c_ptr = &cave[Depth][y][x1-1];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &cave[Depth][y][x2+1];
		c_ptr->feat = FEAT_WALL_OUTER;
#endif
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y1-1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &zcave[y2+1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
#else
		c_ptr = &cave[Depth][y1-1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &cave[Depth][y2+1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
#endif
	}


	/* The inner room */
	y1 = y1 + 2;
	y2 = y2 - 2;
	x1 = x1 + 2;
	x2 = x2 - 2;

	/* The inner walls */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x1-1];
		c_ptr->feat = FEAT_WALL_INNER;
		c_ptr = &zcave[y][x2+1];
		c_ptr->feat = FEAT_WALL_INNER;
#else
		c_ptr = &cave[Depth][y][x1-1];
		c_ptr->feat = FEAT_WALL_INNER;
		c_ptr = &cave[Depth][y][x2+1];
		c_ptr->feat = FEAT_WALL_INNER;
#endif
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y1-1][x];
		c_ptr->feat = FEAT_WALL_INNER;
		c_ptr = &zcave[y2+1][x];
		c_ptr->feat = FEAT_WALL_INNER;
#else
		c_ptr = &cave[Depth][y1-1][x];
		c_ptr->feat = FEAT_WALL_INNER;
		c_ptr = &cave[Depth][y2+1][x];
		c_ptr->feat = FEAT_WALL_INNER;
#endif
	}


	/* Inner room variations */
	switch (randint(5))
	{
		/* Just an inner room with a monster */
		case 1:

		/* Place a secret door */
		switch (randint(4))
		{
#ifdef NEW_DUNGEON
			case 1: place_secret_door(wpos, y1 - 1, xval); break;
			case 2: place_secret_door(wpos, y2 + 1, xval); break;
			case 3: place_secret_door(wpos, yval, x1 - 1); break;
			case 4: place_secret_door(wpos, yval, x2 + 1); break;
#else
			case 1: place_secret_door(Depth, y1 - 1, xval); break;
			case 2: place_secret_door(Depth, y2 + 1, xval); break;
			case 3: place_secret_door(Depth, yval, x1 - 1); break;
			case 4: place_secret_door(Depth, yval, x2 + 1); break;
#endif
		}

		/* Place a monster in the room */
#ifdef NEW_DUNGEON
		vault_monsters(wpos, yval, xval, 1);
#else
		vault_monsters(Depth, yval, xval, 1);
#endif

		break;


		/* Treasure Vault (with a door) */
		case 2:

		/* Place a secret door */
		switch (randint(4))
		{
#ifdef NEW_DUNGEON
			case 1: place_secret_door(wpos, y1 - 1, xval); break;
			case 2: place_secret_door(wpos, y2 + 1, xval); break;
			case 3: place_secret_door(wpos, yval, x1 - 1); break;
			case 4: place_secret_door(wpos, yval, x2 + 1); break;
#else
			case 1: place_secret_door(Depth, y1 - 1, xval); break;
			case 2: place_secret_door(Depth, y2 + 1, xval); break;
			case 3: place_secret_door(Depth, yval, x1 - 1); break;
			case 4: place_secret_door(Depth, yval, x2 + 1); break;
#endif
		}

		/* Place another inner room */
		for (y = yval - 1; y <= yval + 1; y++)
		{
			for (x = xval -  1; x <= xval + 1; x++)
			{
				if ((x == xval) && (y == yval)) continue;
#ifdef NEW_DUNGEON
				c_ptr = &zcave[y][x];
#else
				c_ptr = &cave[Depth][y][x];
#endif
				c_ptr->feat = FEAT_WALL_INNER;
			}
		}

		/* Place a locked door on the inner room */
		switch (randint(4))
		{
#ifdef NEW_DUNGEON
			case 1: place_locked_door(wpos, yval - 1, xval); break;
			case 2: place_locked_door(wpos, yval + 1, xval); break;
			case 3: place_locked_door(wpos, yval, xval - 1); break;
			case 4: place_locked_door(wpos, yval, xval + 1); break;
#else
			case 1: place_locked_door(Depth, yval - 1, xval); break;
			case 2: place_locked_door(Depth, yval + 1, xval); break;
			case 3: place_locked_door(Depth, yval, xval - 1); break;
			case 4: place_locked_door(Depth, yval, xval + 1); break;
#endif
		}

		/* Monsters to guard the "treasure" */
#ifdef NEW_DUNGEON
		vault_monsters(wpos, yval, xval, randint(3) + 2);
#else
		vault_monsters(Depth, yval, xval, randint(3) + 2);
#endif

		/* Object (80%) */
		if (rand_int(100) < 80)
		{
#ifdef NEW_DUNGEON
			place_object(wpos, yval, xval, FALSE, FALSE);
#else
			place_object(Depth, yval, xval, FALSE, FALSE);
#endif
		}

		/* Stairs (20%) */
		else
		{
#ifdef NEW_DUNGEON
			place_random_stairs(wpos, yval, xval);
#else
			place_random_stairs(Depth, yval, xval);
#endif
		}

		/* Traps to protect the treasure */
#ifdef NEW_DUNGEON
		vault_traps(wpos, yval, xval, 4, 10, 2 + randint(3));
#else
		vault_traps(Depth, yval, xval, 4, 10, 2 + randint(3));
#endif

		break;


		/* Inner pillar(s). */
		case 3:

		/* Place a secret door */
		switch (randint(4))
		{
#ifdef NEW_DUNGEON
			case 1: place_secret_door(wpos, y1 - 1, xval); break;
			case 2: place_secret_door(wpos, y2 + 1, xval); break;
			case 3: place_secret_door(wpos, yval, x1 - 1); break;
			case 4: place_secret_door(wpos, yval, x2 + 1); break;
#else
			case 1: place_secret_door(Depth, y1 - 1, xval); break;
			case 2: place_secret_door(Depth, y2 + 1, xval); break;
			case 3: place_secret_door(Depth, yval, x1 - 1); break;
			case 4: place_secret_door(Depth, yval, x2 + 1); break;
#endif
		}

		/* Large Inner Pillar */
		for (y = yval - 1; y <= yval + 1; y++)
		{
			for (x = xval - 1; x <= xval + 1; x++)
			{
#ifdef NEW_DUNGEON
				c_ptr = &zcave[y][x];
#else
				c_ptr = &cave[Depth][y][x];
#endif
				c_ptr->feat = FEAT_WALL_INNER;
			}
		}

		/* Occasionally, two more Large Inner Pillars */
		if (rand_int(2) == 0)
		{
			tmp = randint(2);
			for (y = yval - 1; y <= yval + 1; y++)
			{
				for (x = xval - 5 - tmp; x <= xval - 3 - tmp; x++)
				{
#ifdef NEW_DUNGEON
					c_ptr = &zcave[y][x];
					c_ptr->feat = FEAT_WALL_INNER;
#else
					c_ptr = &cave[Depth][y][x];
					c_ptr->feat = FEAT_WALL_INNER;
#endif
				}
				for (x = xval + 3 + tmp; x <= xval + 5 + tmp; x++)
				{
#ifdef NEW_DUNGEON
					c_ptr = &zcave[y][x];
					c_ptr->feat = FEAT_WALL_INNER;
#else
					c_ptr = &cave[Depth][y][x];
					c_ptr->feat = FEAT_WALL_INNER;
#endif
				}
			}
		}

		/* Occasionally, some Inner rooms */
		if (rand_int(3) == 0)
		{
			/* Long horizontal walls */
			for (x = xval - 5; x <= xval + 5; x++)
			{
#ifdef NEW_DUNGEON
				c_ptr = &zcave[yval-1][x];
				c_ptr->feat = FEAT_WALL_INNER;
				c_ptr = &zcave[yval+1][x];
				c_ptr->feat = FEAT_WALL_INNER;
#else
				c_ptr = &cave[Depth][yval-1][x];
				c_ptr->feat = FEAT_WALL_INNER;
				c_ptr = &cave[Depth][yval+1][x];
				c_ptr->feat = FEAT_WALL_INNER;
#endif
			}

			/* Close off the left/right edges */
#ifdef NEW_DUNGEON
			c_ptr = &zcave[yval][xval-5];
			c_ptr->feat = FEAT_WALL_INNER;
			c_ptr = &zcave[yval][xval+5];
			c_ptr->feat = FEAT_WALL_INNER;

			/* Secret doors (random top/bottom) */
			place_secret_door(wpos, yval - 3 + (randint(2) * 2), xval - 3);
			place_secret_door(wpos, yval - 3 + (randint(2) * 2), xval + 3);

			/* Monsters */
			vault_monsters(wpos, yval, xval - 2, randint(2));
			vault_monsters(wpos, yval, xval + 2, randint(2));

			/* Objects */
			if (rand_int(3) == 0) place_object(wpos, yval, xval - 2, FALSE, FALSE);
			if (rand_int(3) == 0) place_object(wpos, yval, xval + 2, FALSE, FALSE);
#else
			c_ptr = &cave[Depth][yval][xval-5];
			c_ptr->feat = FEAT_WALL_INNER;
			c_ptr = &cave[Depth][yval][xval+5];
			c_ptr->feat = FEAT_WALL_INNER;

			/* Secret doors (random top/bottom) */
			place_secret_door(Depth, yval - 3 + (randint(2) * 2), xval - 3);
			place_secret_door(Depth, yval - 3 + (randint(2) * 2), xval + 3);

			/* Monsters */
			vault_monsters(Depth, yval, xval - 2, randint(2));
			vault_monsters(Depth, yval, xval + 2, randint(2));

			/* Objects */
			if (rand_int(3) == 0) place_object(Depth, yval, xval - 2, FALSE, FALSE);
			if (rand_int(3) == 0) place_object(Depth, yval, xval + 2, FALSE, FALSE);
#endif
		}

		break;


		/* Maze inside. */
		case 4:

		/* Place a secret door */
		switch (randint(4))
		{
#ifdef NEW_DUNGEON
			case 1: place_secret_door(wpos, y1 - 1, xval); break;
			case 2: place_secret_door(wpos, y2 + 1, xval); break;
			case 3: place_secret_door(wpos, yval, x1 - 1); break;
			case 4: place_secret_door(wpos, yval, x2 + 1); break;
#else
			case 1: place_secret_door(Depth, y1 - 1, xval); break;
			case 2: place_secret_door(Depth, y2 + 1, xval); break;
			case 3: place_secret_door(Depth, yval, x1 - 1); break;
			case 4: place_secret_door(Depth, yval, x2 + 1); break;
#endif
		}

		/* Maze (really a checkerboard) */
		for (y = y1; y <= y2; y++)
		{
			for (x = x1; x <= x2; x++)
			{
				if (0x1 & (x + y))
				{
#ifdef NEW_DUNGEON
					c_ptr = &zcave[y][x];
					c_ptr->feat = FEAT_WALL_INNER;
#else
					c_ptr = &cave[Depth][y][x];
					c_ptr->feat = FEAT_WALL_INNER;
#endif
				}
			}
		}

#ifdef NEW_DUNGEON
		/* Monsters just love mazes. */
		vault_monsters(wpos, yval, xval - 5, randint(3));
		vault_monsters(wpos, yval, xval + 5, randint(3));

		/* Traps make them entertaining. */
		vault_traps(wpos, yval, xval - 3, 2, 8, randint(3));
		vault_traps(wpos, yval, xval + 3, 2, 8, randint(3));

		/* Mazes should have some treasure too. */
		vault_objects(wpos, yval, xval, 3);
#else
		/* Monsters just love mazes. */
		vault_monsters(Depth, yval, xval - 5, randint(3));
		vault_monsters(Depth, yval, xval + 5, randint(3));

		/* Traps make them entertaining. */
		vault_traps(Depth, yval, xval - 3, 2, 8, randint(3));
		vault_traps(Depth, yval, xval + 3, 2, 8, randint(3));

		/* Mazes should have some treasure too. */
		vault_objects(Depth, yval, xval, 3);
#endif

		break;


		/* Four small rooms. */
		case 5:

		/* Inner "cross" */
		for (y = y1; y <= y2; y++)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][xval];
#else
			c_ptr = &cave[Depth][y][xval];
#endif
			c_ptr->feat = FEAT_WALL_INNER;
		}
		for (x = x1; x <= x2; x++)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[yval][x];
#else
			c_ptr = &cave[Depth][yval][x];
#endif
			c_ptr->feat = FEAT_WALL_INNER;
		}

		/* Doors into the rooms */
		if (rand_int(100) < 50)
		{
			int i = randint(10);
#ifdef NEW_DUNGEON
			place_secret_door(wpos, y1 - 1, xval - i);
			place_secret_door(wpos, y1 - 1, xval + i);
			place_secret_door(wpos, y2 + 1, xval - i);
			place_secret_door(wpos, y2 + 1, xval + i);
#else
			place_secret_door(Depth, y1 - 1, xval - i);
			place_secret_door(Depth, y1 - 1, xval + i);
			place_secret_door(Depth, y2 + 1, xval - i);
			place_secret_door(Depth, y2 + 1, xval + i);
#endif
		}
		else
		{
			int i = randint(3);
#ifdef NEW_DUNGEON
			place_secret_door(wpos, yval + i, x1 - 1);
			place_secret_door(wpos, yval - i, x1 - 1);
			place_secret_door(wpos, yval + i, x2 + 1);
			place_secret_door(wpos, yval - i, x2 + 1);
#else
			place_secret_door(Depth, yval + i, x1 - 1);
			place_secret_door(Depth, yval - i, x1 - 1);
			place_secret_door(Depth, yval + i, x2 + 1);
			place_secret_door(Depth, yval - i, x2 + 1);
#endif
		}

#ifdef NEW_DUNGEON
		/* Treasure, centered at the center of the cross */
		vault_objects(wpos, yval, xval, 2 + randint(2));

		/* Gotta have some monsters. */
		vault_monsters(wpos, yval + 1, xval - 4, randint(4));
		vault_monsters(wpos, yval + 1, xval + 4, randint(4));
		vault_monsters(wpos, yval - 1, xval - 4, randint(4));
		vault_monsters(wpos, yval - 1, xval + 4, randint(4));
#else
		/* Treasure, centered at the center of the cross */
		vault_objects(Depth, yval, xval, 2 + randint(2));

		/* Gotta have some monsters. */
		vault_monsters(Depth, yval + 1, xval - 4, randint(4));
		vault_monsters(Depth, yval + 1, xval + 4, randint(4));
		vault_monsters(Depth, yval - 1, xval - 4, randint(4));
		vault_monsters(Depth, yval - 1, xval + 4, randint(4));
#endif

		break;
	}
}


/*
 * The following functions are used to determine if the given monster
 * is appropriate for inclusion in a monster nest or monster pit or
 * the given type.
 *
 * None of the pits/nests are allowed to include "unique" monsters,
 * or monsters which can "multiply".
 *
 * Some of the pits/nests are asked to avoid monsters which can blink
 * away or which are invisible.  This is probably a hack.
 *
 * The old method made direct use of monster "names", which is bad.
 *
 * Note the use of Angband 2.7.9 monster race pictures in various places.
 */


/*
 * Helper function for "monster nest (jelly)"
 */
static bool vault_aux_jelly(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* Require icky thing, jelly, mold, or mushroom */
	if (!strchr("ijm,", r_ptr->d_char)) return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Helper function for "monster nest (animal)"
 */
static bool vault_aux_animal(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* Require "animal" flag */
	if (!(r_ptr->flags3 & RF3_ANIMAL)) return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Helper function for "monster nest (undead)"
 */
static bool vault_aux_undead(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* Require Undead */
	if (!(r_ptr->flags3 & RF3_UNDEAD)) return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Helper function for "monster pit (orc)"
 */
static bool vault_aux_orc(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* Hack -- Require "o" monsters */
	if (!strchr("o", r_ptr->d_char)) return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Helper function for "monster pit (troll)"
 */
static bool vault_aux_troll(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* Hack -- Require "T" monsters */
	if (!strchr("T", r_ptr->d_char)) return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Helper function for "monster pit (giant)"
 */
static bool vault_aux_giant(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* Hack -- Require "P" monsters */
	if (!strchr("P", r_ptr->d_char)) return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Hack -- breath type for "vault_aux_dragon()"
 */
static u32b vault_aux_dragon_mask4;


/*
 * Helper function for "monster pit (dragon)"
 */
static bool vault_aux_dragon(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* Hack -- Require "d" or "D" monsters */
	if (!strchr("Dd", r_ptr->d_char)) return (FALSE);

	/* Hack -- Require correct "breath attack" */
	if (r_ptr->flags4 != vault_aux_dragon_mask4) return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Helper function for "monster pit (demon)"
 */
static bool vault_aux_demon(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* Hack -- Require "U" monsters */
	if (!strchr("U", r_ptr->d_char)) return (FALSE);

	/* Okay */
	return (TRUE);
}



/*
 * Type 5 -- Monster nests
 *
 * A monster nest is a "big" room, with an "inner" room, containing
 * a "collection" of monsters of a given type strewn about the room.
 *
 * The monsters are chosen from a set of 64 randomly selected monster
 * races, to allow the nest creation to fail instead of having "holes".
 *
 * Note the use of the "get_mon_num_prep()" function, and the special
 * "get_mon_num_hook()" restriction function, to prepare the "monster
 * allocation table" in such a way as to optimize the selection of
 * "appropriate" non-unique monsters for the nest.
 *
 * Currently, a monster nest is one of
 *   a nest of "jelly" monsters   (Dungeon level 5 and deeper)
 *   a nest of "animal" monsters  (Dungeon level 30 and deeper)
 *   a nest of "undead" monsters  (Dungeon level 50 and deeper)
 *
 * Note that the "get_mon_num()" function may (rarely) fail, in which
 * case the nest will be empty, and will not affect the level rating.
 *
 * Note that "monster nests" will never contain "unique" monsters.
 */
#ifdef NEW_DUNGEON
static void build_type5(struct worldpos *wpos, int yval, int xval)
#else
static void build_type5(int Depth, int yval, int xval)
#endif
{
	int			y, x, y1, x1, y2, x2;

	int			tmp, i;

	s16b		what[64];

	cave_type		*c_ptr;

	cptr		name;

	bool		empty = FALSE;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif


	/* Large room */
	y1 = yval - 4;
	y2 = yval + 4;
	x1 = xval - 11;
	x2 = xval + 11;


	/* Place the floor area */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		for (x = x1 - 1; x <= x2 + 1; x++)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][x];
#else
			c_ptr = &cave[Depth][y][x];
#endif
			c_ptr->feat = FEAT_FLOOR;
			c_ptr->info |= CAVE_ROOM;
		}
	}

	/* Place the outer walls */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x1-1];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &zcave[y][x2+1];
		c_ptr->feat = FEAT_WALL_OUTER;
#else
		c_ptr = &cave[Depth][y][x1-1];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &cave[Depth][y][x2+1];
		c_ptr->feat = FEAT_WALL_OUTER;
#endif
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y1-1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &zcave[y2+1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
#else
		c_ptr = &cave[Depth][y1-1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &cave[Depth][y2+1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
#endif
	}


	/* Advance to the center room */
	y1 = y1 + 2;
	y2 = y2 - 2;
	x1 = x1 + 2;
	x2 = x2 - 2;

	/* The inner walls */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x1-1];
		c_ptr->feat = FEAT_WALL_INNER;
		c_ptr = &zcave[y][x2+1];
		c_ptr->feat = FEAT_WALL_INNER;
#else
		c_ptr = &cave[Depth][y][x1-1];
		c_ptr->feat = FEAT_WALL_INNER;
		c_ptr = &cave[Depth][y][x2+1];
		c_ptr->feat = FEAT_WALL_INNER;
#endif
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y1-1][x];
		c_ptr->feat = FEAT_WALL_INNER;
		c_ptr = &zcave[y2+1][x];
		c_ptr->feat = FEAT_WALL_INNER;
#else
		c_ptr = &cave[Depth][y1-1][x];
		c_ptr->feat = FEAT_WALL_INNER;
		c_ptr = &cave[Depth][y2+1][x];
		c_ptr->feat = FEAT_WALL_INNER;
#endif
	}


	/* Place a secret door */
	switch (randint(4))
	{
#ifdef NEW_DUNGEON
		case 1: place_secret_door(wpos, y1 - 1, xval); break;
		case 2: place_secret_door(wpos, y2 + 1, xval); break;
		case 3: place_secret_door(wpos, yval, x1 - 1); break;
		case 4: place_secret_door(wpos, yval, x2 + 1); break;
#else
		case 1: place_secret_door(Depth, y1 - 1, xval); break;
		case 2: place_secret_door(Depth, y2 + 1, xval); break;
		case 3: place_secret_door(Depth, yval, x1 - 1); break;
		case 4: place_secret_door(Depth, yval, x2 + 1); break;
#endif
	}


	/* Hack -- Choose a nest type */
#ifdef NEW_DUNGEON
	tmp = randint(getlevel(wpos));
#else
	tmp = randint(Depth);
#endif
	/* Monster nest (jelly) */
	if (tmp < 30)
	{
		/* Describe */
		name = "jelly";

		/* Restrict to jelly */
		get_mon_num_hook = vault_aux_jelly;
	}

	/* Monster nest (animal) */
	else if (tmp < 50)
	{
		/* Describe */
		name = "animal";

		/* Restrict to animal */
		get_mon_num_hook = vault_aux_animal;
	}

	/* Monster nest (undead) */
	else
	{
		/* Describe */
		name = "undead";

		/* Restrict to undead */
		get_mon_num_hook = vault_aux_undead;
	}

	/* Prepare allocation table */
	get_mon_num_prep();


	/* Pick some monster types */
	for (i = 0; i < 64; i++)
	{
		/* Get a (hard) monster type */
#ifdef NEW_DUNGEON
		what[i] = get_mon_num(getlevel(wpos) + 10);
#else
		what[i] = get_mon_num(Depth + 10);
#endif

		/* Notice failure */
		if (!what[i]) empty = TRUE;
	}


	/* Remove restriction */
	get_mon_num_hook = NULL;

	/* Prepare allocation table */
	get_mon_num_prep();


	/* Oops */
	if (empty) return;


	/* Describe */
	if (cheat_room)
	{
		/* Room type */
		/*msg_format("Monster nest (%s)", name);*/
	}


	/* Increase the level rating */
	rating += 10;

	/* (Sometimes) Cause a "special feeling" (for "Monster Nests") */
#ifdef NEW_DUNGEON
	if ((getlevel(wpos) <= 40) && (randint(getlevel(wpos)*getlevel(wpos) + 1) < 300))
#else
	if ((Depth <= 40) && (randint(Depth*Depth + 1) < 300))
#endif
	{
		good_item_flag = TRUE;
	}


	/* Place some monsters */
	for (y = yval - 2; y <= yval + 2; y++)
	{
		for (x = xval - 9; x <= xval + 9; x++)
		{
			int r_idx = what[rand_int(64)];

			/* Place that "random" monster (no groups) */
#ifdef NEW_DUNGEON
			(void)place_monster_aux(wpos, y, x, r_idx, FALSE, FALSE, FALSE);
#else
			(void)place_monster_aux(Depth, y, x, r_idx, FALSE, FALSE, FALSE);
#endif
		}
	}
}



/*
 * Type 6 -- Monster pits
 *
 * A monster pit is a "big" room, with an "inner" room, containing
 * a "collection" of monsters of a given type organized in the room.
 *
 * Monster types in the pit
 *   orc pit	(Dungeon Level 5 and deeper)
 *   troll pit	(Dungeon Level 20 and deeper)
 *   giant pit	(Dungeon Level 40 and deeper)
 *   dragon pit	(Dungeon Level 60 and deeper)
 *   demon pit	(Dungeon Level 80 and deeper)
 *
 * The inside room in a monster pit appears as shown below, where the
 * actual monsters in each location depend on the type of the pit
 *
 *   #####################
 *   #0000000000000000000#
 *   #0112233455543322110#
 *   #0112233467643322110#
 *   #0112233455543322110#
 *   #0000000000000000000#
 *   #####################
 *
 * Note that the monsters in the pit are now chosen by using "get_mon_num()"
 * to request 16 "appropriate" monsters, sorting them by level, and using
 * the "even" entries in this sorted list for the contents of the pit.
 *
 * Hack -- all of the "dragons" in a "dragon" pit must be the same "color",
 * which is handled by requiring a specific "breath" attack for all of the
 * dragons.  This may include "multi-hued" breath.  Note that "wyrms" may
 * be present in many of the dragon pits, if they have the proper breath.
 *
 * Note the use of the "get_mon_num_prep()" function, and the special
 * "get_mon_num_hook()" restriction function, to prepare the "monster
 * allocation table" in such a way as to optimize the selection of
 * "appropriate" non-unique monsters for the pit.
 *
 * Note that the "get_mon_num()" function may (rarely) fail, in which case
 * the pit will be empty, and will not effect the level rating.
 *
 * Note that "monster pits" will never contain "unique" monsters.
 */
#ifdef NEW_DUNGEON
static void build_type6(struct worldpos *wpos, int yval, int xval)
#else
static void build_type6(int Depth, int yval, int xval)
#endif
{
	int			tmp, what[16];

	int			i, j, y, x, y1, x1, y2, x2;

	bool		empty = FALSE;

	cave_type		*c_ptr;

	cptr		name;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif


	/* Large room */
	y1 = yval - 4;
	y2 = yval + 4;
	x1 = xval - 11;
	x2 = xval + 11;


	/* Place the floor area */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		for (x = x1 - 1; x <= x2 + 1; x++)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][x];
#else
			c_ptr = &cave[Depth][y][x];
#endif
			c_ptr->feat = FEAT_FLOOR;
			c_ptr->info |= CAVE_ROOM;
		}
	}

	/* Place the outer walls */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x1-1];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &zcave[y][x2+1];
		c_ptr->feat = FEAT_WALL_OUTER;
#else
		c_ptr = &cave[Depth][y][x1-1];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &cave[Depth][y][x2+1];
		c_ptr->feat = FEAT_WALL_OUTER;
#endif
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y1-1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &zcave[y2+1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
#else
		c_ptr = &cave[Depth][y1-1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
		c_ptr = &cave[Depth][y2+1][x];
		c_ptr->feat = FEAT_WALL_OUTER;
#endif
	}


	/* Advance to the center room */
	y1 = y1 + 2;
	y2 = y2 - 2;
	x1 = x1 + 2;
	x2 = x2 - 2;

	/* The inner walls */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x1-1];
		c_ptr->feat = FEAT_WALL_INNER;
		c_ptr = &zcave[y][x2+1];
		c_ptr->feat = FEAT_WALL_INNER;
#else
		c_ptr = &cave[Depth][y][x1-1];
		c_ptr->feat = FEAT_WALL_INNER;
		c_ptr = &cave[Depth][y][x2+1];
		c_ptr->feat = FEAT_WALL_INNER;
#endif
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y1-1][x];
		c_ptr->feat = FEAT_WALL_INNER;
		c_ptr = &zcave[y2+1][x];
		c_ptr->feat = FEAT_WALL_INNER;
#else
		c_ptr = &cave[Depth][y1-1][x];
		c_ptr->feat = FEAT_WALL_INNER;
		c_ptr = &cave[Depth][y2+1][x];
		c_ptr->feat = FEAT_WALL_INNER;
#endif
	}


	/* Place a secret door */
	switch (randint(4))
	{
#ifdef NEW_DUNGEON
		case 1: place_secret_door(wpos, y1 - 1, xval); break;
		case 2: place_secret_door(wpos, y2 + 1, xval); break;
		case 3: place_secret_door(wpos, yval, x1 - 1); break;
		case 4: place_secret_door(wpos, yval, x2 + 1); break;
#else
		case 1: place_secret_door(Depth, y1 - 1, xval); break;
		case 2: place_secret_door(Depth, y2 + 1, xval); break;
		case 3: place_secret_door(Depth, yval, x1 - 1); break;
		case 4: place_secret_door(Depth, yval, x2 + 1); break;
#endif
	}


	/* Choose a pit type */
#ifdef NEW_DUNGEON
	tmp = randint(getlevel(wpos));
#else
	tmp = randint(Depth);
#endif

	/* Orc pit */
	if (tmp < 20)
	{
		/* Message */
		name = "orc";

		/* Restrict monster selection */
		get_mon_num_hook = vault_aux_orc;
	}

	/* Troll pit */
	else if (tmp < 40)
	{
		/* Message */
		name = "troll";

		/* Restrict monster selection */
		get_mon_num_hook = vault_aux_troll;
	}

	/* Giant pit */
	else if (tmp < 60)
	{
		/* Message */
		name = "giant";

		/* Restrict monster selection */
		get_mon_num_hook = vault_aux_giant;
	}

	/* Dragon pit */
	else if (tmp < 80)
	{
		/* Pick dragon type */
		switch (rand_int(6))
		{
			/* Black */
			case 0:
			{
				/* Message */
				name = "acid dragon";

				/* Restrict dragon breath type */
				vault_aux_dragon_mask4 = RF4_BR_ACID;

				/* Done */
				break;
			}

			/* Blue */
			case 1:
			{
				/* Message */
				name = "electric dragon";

				/* Restrict dragon breath type */
				vault_aux_dragon_mask4 = RF4_BR_ELEC;

				/* Done */
				break;
			}

			/* Red */
			case 2:
			{
				/* Message */
				name = "fire dragon";

				/* Restrict dragon breath type */
				vault_aux_dragon_mask4 = RF4_BR_FIRE;

				/* Done */
				break;
			}

			/* White */
			case 3:
			{
				/* Message */
				name = "cold dragon";

				/* Restrict dragon breath type */
				vault_aux_dragon_mask4 = RF4_BR_COLD;

				/* Done */
				break;
			}

			/* Green */
			case 4:
			{
				/* Message */
				name = "poison dragon";

				/* Restrict dragon breath type */
				vault_aux_dragon_mask4 = RF4_BR_POIS;

				/* Done */
				break;
			}

			/* Multi-hued */
			default:
			{
				/* Message */
				name = "multi-hued dragon";

				/* Restrict dragon breath type */
				vault_aux_dragon_mask4 = (RF4_BR_ACID | RF4_BR_ELEC |
				                          RF4_BR_FIRE | RF4_BR_COLD |
				                          RF4_BR_POIS);

				/* Done */
				break;
			}

		}

		/* Restrict monster selection */
		get_mon_num_hook = vault_aux_dragon;
	}

	/* Demon pit */
	else
	{
		/* Message */
		name = "demon";

		/* Restrict monster selection */
		get_mon_num_hook = vault_aux_demon;
	}

	/* Prepare allocation table */
	get_mon_num_prep();


	/* Pick some monster types */
	for (i = 0; i < 16; i++)
	{
		/* Get a (hard) monster type */
#ifdef NEW_DUNGEON
		what[i] = get_mon_num(getlevel(wpos) + 10);
#else
		what[i] = get_mon_num(Depth + 10);
#endif

		/* Notice failure */
		if (!what[i]) empty = TRUE;
	}


	/* Remove restriction */
	get_mon_num_hook = NULL;

	/* Prepare allocation table */
	get_mon_num_prep();


	/* Oops */
	if (empty) return;


	/* XXX XXX XXX */
	/* Sort the entries */
	for (i = 0; i < 16 - 1; i++)
	{
		/* Sort the entries */
		for (j = 0; j < 16 - 1; j++)
		{
			int i1 = j;
			int i2 = j + 1;

			int p1 = r_info[what[i1]].level;
			int p2 = r_info[what[i2]].level;

			/* Bubble */
			if (p1 > p2)
			{
				int tmp = what[i1];
				what[i1] = what[i2];
				what[i2] = tmp;
			}
		}
	}

	/* Select the entries */
	for (i = 0; i < 8; i++)
	{
		/* Every other entry */
		what[i] = what[i * 2];
	}


#if 0
	/* Message */
	if (cheat_room)
	{
		/* Room type */
		msg_format("Monster pit (%s)", name);

		/* Contents */
		for (i = 0; i < 8; i++)
		{
			/* Message */
			msg_print(r_name + r_info[what[i]].name);
		}
	}
#endif


	/* Increase the level rating */
	rating += 10;

	/* (Sometimes) Cause a "special feeling" (for "Monster Pits") */
#ifdef NEW_DUNGEON
	if ((getlevel(wpos) <= 40) && (randint(getlevel(wpos)*getlevel(wpos)+ 1) < 300))
#else
	if ((Depth <= 40) && (randint(Depth*Depth + 1) < 300))
#endif
	{
		good_item_flag = TRUE;
	}


	/* Top and bottom rows */
	for (x = xval - 9; x <= xval + 9; x++)
	{
#ifdef NEW_DUNGEON
		place_monster_aux(wpos, yval - 2, x, what[0], FALSE, FALSE, FALSE);
		place_monster_aux(wpos, yval + 2, x, what[0], FALSE, FALSE, FALSE);
#else
		place_monster_aux(Depth, yval - 2, x, what[0], FALSE, FALSE, FALSE);
		place_monster_aux(Depth, yval + 2, x, what[0], FALSE, FALSE, FALSE);
#endif
	}

	/* Middle columns */
	for (y = yval - 1; y <= yval + 1; y++)
	{
#ifdef NEW_DUNGEON
		place_monster_aux(wpos, y, xval - 9, what[0], FALSE, FALSE, FALSE);
		place_monster_aux(wpos, y, xval + 9, what[0], FALSE, FALSE, FALSE);

		place_monster_aux(wpos, y, xval - 8, what[1], FALSE, FALSE, FALSE);
		place_monster_aux(wpos, y, xval + 8, what[1], FALSE, FALSE, FALSE);

		place_monster_aux(wpos, y, xval - 7, what[1], FALSE, FALSE, FALSE);
		place_monster_aux(wpos, y, xval + 7, what[1], FALSE, FALSE, FALSE);

		place_monster_aux(wpos, y, xval - 6, what[2], FALSE, FALSE, FALSE);
		place_monster_aux(wpos, y, xval + 6, what[2], FALSE, FALSE, FALSE);

		place_monster_aux(wpos, y, xval - 5, what[2], FALSE, FALSE, FALSE);
		place_monster_aux(wpos, y, xval + 5, what[2], FALSE, FALSE, FALSE);

		place_monster_aux(wpos, y, xval - 4, what[3], FALSE, FALSE, FALSE);
		place_monster_aux(wpos, y, xval + 4, what[3], FALSE, FALSE, FALSE);

		place_monster_aux(wpos, y, xval - 3, what[3], FALSE, FALSE, FALSE);
		place_monster_aux(wpos, y, xval + 3, what[3], FALSE, FALSE, FALSE);

		place_monster_aux(wpos, y, xval - 2, what[4], FALSE, FALSE, FALSE);
		place_monster_aux(wpos, y, xval + 2, what[4], FALSE, FALSE, FALSE);
	}

	/* Above/Below the center monster */
	for (x = xval - 1; x <= xval + 1; x++)
	{
		place_monster_aux(wpos, yval + 1, x, what[5], FALSE, FALSE, FALSE);
		place_monster_aux(wpos, yval - 1, x, what[5], FALSE, FALSE, FALSE);
	}

	/* Next to the center monster */
	place_monster_aux(wpos, yval, xval + 1, what[6], FALSE, FALSE, FALSE);
	place_monster_aux(wpos, yval, xval - 1, what[6], FALSE, FALSE, FALSE);

	/* Center monster */
	place_monster_aux(wpos, yval, xval, what[7], FALSE, FALSE, FALSE);
#else
		place_monster_aux(Depth, y, xval - 9, what[0], FALSE, FALSE, FALSE);
		place_monster_aux(Depth, y, xval + 9, what[0], FALSE, FALSE, FALSE);

		place_monster_aux(Depth, y, xval - 8, what[1], FALSE, FALSE, FALSE);
		place_monster_aux(Depth, y, xval + 8, what[1], FALSE, FALSE, FALSE);

		place_monster_aux(Depth, y, xval - 7, what[1], FALSE, FALSE, FALSE);
		place_monster_aux(Depth, y, xval + 7, what[1], FALSE, FALSE, FALSE);

		place_monster_aux(Depth, y, xval - 6, what[2], FALSE, FALSE, FALSE);
		place_monster_aux(Depth, y, xval + 6, what[2], FALSE, FALSE, FALSE);

		place_monster_aux(Depth, y, xval - 5, what[2], FALSE, FALSE, FALSE);
		place_monster_aux(Depth, y, xval + 5, what[2], FALSE, FALSE, FALSE);

		place_monster_aux(Depth, y, xval - 4, what[3], FALSE, FALSE, FALSE);
		place_monster_aux(Depth, y, xval + 4, what[3], FALSE, FALSE, FALSE);

		place_monster_aux(Depth, y, xval - 3, what[3], FALSE, FALSE, FALSE);
		place_monster_aux(Depth, y, xval + 3, what[3], FALSE, FALSE, FALSE);

		place_monster_aux(Depth, y, xval - 2, what[4], FALSE, FALSE, FALSE);
		place_monster_aux(Depth, y, xval + 2, what[4], FALSE, FALSE, FALSE);
	}

	/* Above/Below the center monster */
	for (x = xval - 1; x <= xval + 1; x++)
	{
		place_monster_aux(Depth, yval + 1, x, what[5], FALSE, FALSE, FALSE);
		place_monster_aux(Depth, yval - 1, x, what[5], FALSE, FALSE, FALSE);
	}

	/* Next to the center monster */
	place_monster_aux(Depth, yval, xval + 1, what[6], FALSE, FALSE, FALSE);
	place_monster_aux(Depth, yval, xval - 1, what[6], FALSE, FALSE, FALSE);

	/* Center monster */
	place_monster_aux(Depth, yval, xval, what[7], FALSE, FALSE, FALSE);
#endif
}



/*
 * Hack -- fill in "vault" rooms
 */
#ifdef NEW_DUNGEON
void build_vault(struct worldpos *wpos, int yval, int xval, int ymax, int xmax, cptr data)
#else
void build_vault(int Depth, int yval, int xval, int ymax, int xmax, cptr data)
#endif
{
	int dx, dy, x, y;

	cptr t;

	cave_type *c_ptr;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif

	/* Place dungeon features and objects */
	for (t = data, dy = 0; dy < ymax; dy++)
	{
		for (dx = 0; dx < xmax; dx++, t++)
		{
			/* Extract the location */
			x = xval - (xmax / 2) + dx;
			y = yval - (ymax / 2) + dy;

			/* FIXME - find a better solution */
#ifdef NEW_DUNGEON
			if(!in_bounds(y,x))
#else
			if(!in_bounds(Depth,y,x))
#endif
				continue;
			
			/* Hack -- skip "non-grids" */
			if (*t == ' ') continue;

			/* Access the grid */
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][x];
#else
			c_ptr = &cave[Depth][y][x];
#endif

			/* Lay down a floor */
			c_ptr->feat = FEAT_FLOOR;

			/* Part of a vault */
			c_ptr->info |= (CAVE_ROOM | CAVE_ICKY);

			/* Analyze the grid */
			switch (*t)
			{
				/* Granite wall (outer) */
				case '%':
				c_ptr->feat = FEAT_WALL_OUTER;
				break;

				/* Granite wall (inner) */
				case '#':
				c_ptr->feat = FEAT_WALL_INNER;
				break;

				/* Permanent wall (inner) */
				case 'X':
				c_ptr->feat = FEAT_PERM_INNER;
				break;

				/* Treasure/trap */
				case '*':
				if (rand_int(100) < 75)
#ifdef NEW_DUNGEON
				{
					place_object(wpos, y, x, FALSE, FALSE);
				}
				else
				{
					place_trap(wpos, y, x);
				}
#else
				{
					place_object(Depth, y, x, FALSE, FALSE);
				}
				else
				{
					place_trap(Depth, y, x);
				}
#endif
				break;

				/* Secret doors */
				case '+':
#ifdef NEW_DUNGEON
				place_secret_door(wpos, y, x);
#else
				place_secret_door(Depth, y, x);
#endif
				break;

				/* Trap */
				case '^':
#ifdef NEW_DUNGEON
				place_trap(wpos, y, x);
#else
				place_trap(Depth, y, x);
#endif
				break;
			}
		}
	}


	/* Place dungeon monsters and objects */
	for (t = data, dy = 0; dy < ymax; dy++)
	{
		for (dx = 0; dx < xmax; dx++, t++)
		{
			/* Extract the grid */
			x = xval - (xmax/2) + dx;
			y = yval - (ymax/2) + dy;

			/* FIXME - find a better solution */
#ifdef NEW_DUNGEON
			if(!in_bounds(y,x))
#else
			if(!in_bounds(Depth,y,x))
#endif
				continue;

			/* Hack -- skip "non-grids" */
			if (*t == ' ') continue;

			/* Analyze the symbol */
			switch (*t)
			{
#ifdef NEW_DUNGEON
				/* Monster */
				case '&':
				monster_level = getlevel(wpos) + 5;
				place_monster(wpos, y, x, TRUE, TRUE);
				monster_level = getlevel(wpos);
				break;

				/* Meaner monster */
				case '@':
				monster_level = getlevel(wpos) + 11;
				place_monster(wpos, y, x, TRUE, TRUE);
				monster_level = getlevel(wpos);
				break;

				/* Meaner monster, plus treasure */
				case '9':
				monster_level = getlevel(wpos) + 9;
				place_monster(wpos, y, x, TRUE, TRUE);
				monster_level = getlevel(wpos);
				object_level = getlevel(wpos) + 7;
				place_object(wpos, y, x, TRUE, FALSE);
				object_level = getlevel(wpos);
				break;

				/* Nasty monster and treasure */
				case '8':
				monster_level = getlevel(wpos) + 40;
				place_monster(wpos, y, x, TRUE, TRUE);
				monster_level = getlevel(wpos);
				object_level = getlevel(wpos) + 20;
				place_object(wpos, y, x, TRUE, TRUE);
				object_level = getlevel(wpos);
				break;

				/* Monster and/or object */
				case ',':
				if (rand_int(100) < 50)
				{
					monster_level = getlevel(wpos) + 3;
					place_monster(wpos, y, x, TRUE, TRUE);
					monster_level = getlevel(wpos);
				}
				if (rand_int(100) < 50)
				{
					object_level = getlevel(wpos) + 7;
					place_object(wpos, y, x, FALSE, FALSE);
					object_level = getlevel(wpos);
				}
				break;
#else
				/* Monster */
				case '&':
				monster_level = Depth + 5;
				place_monster(Depth, y, x, TRUE, TRUE);
				monster_level = Depth;
				break;

				/* Meaner monster */
				case '@':
				monster_level = Depth + 11;
				place_monster(Depth, y, x, TRUE, TRUE);
				monster_level = Depth;
				break;

				/* Meaner monster, plus treasure */
				case '9':
				monster_level = Depth + 9;
				place_monster(Depth, y, x, TRUE, TRUE);
				monster_level = Depth;
				object_level = Depth + 7;
				place_object(Depth, y, x, TRUE, FALSE);
				object_level = Depth;
				break;

				/* Nasty monster and treasure */
				case '8':
				monster_level = Depth + 40;
				place_monster(Depth, y, x, TRUE, TRUE);
				monster_level = Depth;
				object_level = Depth + 20;
				place_object(Depth, y, x, TRUE, TRUE);
				object_level = Depth;
				break;

				/* Monster and/or object */
				case ',':
				if (rand_int(100) < 50)
				{
					monster_level = Depth + 3;
					place_monster(Depth, y, x, TRUE, TRUE);
					monster_level = Depth;
				}
				if (rand_int(100) < 50)
				{
					object_level = Depth + 7;
					place_object(Depth, y, x, FALSE, FALSE);
					object_level = Depth;
				}
				break;
#endif
			}
		}
	}
}



/*
 * Type 7 -- simple vaults (see "v_info.txt")
 */
#ifdef NEW_DUNGEON
static void build_type7(struct worldpos *wpos, int yval, int xval)
#else
static void build_type7(int Depth, int yval, int xval)
#endif
{
	vault_type	*v_ptr;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif

	/* Pick a lesser vault */
	while (TRUE)
	{
		/* Access a random vault record */
		v_ptr = &v_info[rand_int(MAX_V_IDX)];

		/* Accept the first lesser vault */
		if (v_ptr->typ == 7) break;
	}

	/* Message */
	/*if (cheat_room) msg_print("Lesser Vault");*/

	/* Boost the rating */
	rating += v_ptr->rat;

	/* (Sometimes) Cause a special feeling */
#ifdef NEW_DUNGEON
	if ((getlevel(wpos) <= 50) ||
	    (randint((getlevel(wpos)-40) * (getlevel(wpos)-40) + 1) < 400))
#else
	if ((Depth <= 50) ||
	    (randint((Depth-40) * (Depth-40) + 1) < 400))
#endif
	{
		good_item_flag = TRUE;
	}

	/* Hack -- Build the vault */
#ifdef NEW_DUNGEON
	build_vault(wpos, yval, xval, v_ptr->hgt, v_ptr->wid, v_text + v_ptr->text);
#else
	build_vault(Depth, yval, xval, v_ptr->hgt, v_ptr->wid, v_text + v_ptr->text);
#endif
}



/*
 * Type 8 -- greater vaults (see "v_info.txt")
 */
#ifdef NEW_DUNGEON
static void build_type8(struct worldpos *wpos, int yval, int xval)
#else
static void build_type8(int Depth, int yval, int xval)
#endif
{
	vault_type	*v_ptr;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif

	/* Pick a lesser vault */
	while (TRUE)
	{
		/* Access a random vault record */
		v_ptr = &v_info[rand_int(MAX_V_IDX)];

		/* Accept the first greater vault */
		if (v_ptr->typ == 8) break;
	}

	/* Message */
	/*if (cheat_room) msg_print("Greater Vault");*/

	/* Boost the rating */
	rating += v_ptr->rat;

	/* (Sometimes) Cause a special feeling */
#ifdef NEW_DUNGEON
	if (( getlevel(wpos)<= 50) ||
	    (randint((getlevel(wpos)-40) * (getlevel(wpos)-40) + 1) < 400))
#else
	if ((Depth <= 50) ||
	    (randint((Depth-40) * (Depth-40) + 1) < 400))
#endif
	{
		good_item_flag = TRUE;
	}

	/* Hack -- Build the vault */
#ifdef NEW_DUNGEON
	build_vault(wpos, yval, xval, v_ptr->hgt, v_ptr->wid, v_text + v_ptr->text);
#else
	build_vault(Depth, yval, xval, v_ptr->hgt, v_ptr->wid, v_text + v_ptr->text);
#endif
}



/*
 * Constructs a tunnel between two points
 *
 * This function must be called BEFORE any streamers are created,
 * since we use the special "granite wall" sub-types to keep track
 * of legal places for corridors to pierce rooms.
 *
 * We use "door_flag" to prevent excessive construction of doors
 * along overlapping corridors.
 *
 * We queue the tunnel grids to prevent door creation along a corridor
 * which intersects itself.
 *
 * We queue the wall piercing grids to prevent a corridor from leaving
 * a room and then coming back in through the same entrance.
 *
 * We "pierce" grids which are "outer" walls of rooms, and when we
 * do so, we change all adjacent "outer" walls of rooms into "solid"
 * walls so that no two corridors may use adjacent grids for exits.
 *
 * The "solid" wall check prevents corridors from "chopping" the
 * corners of rooms off, as well as "silly" door placement, and
 * "excessively wide" room entrances.
 *
 * Useful "feat" values:
 *   FEAT_WALL_EXTRA -- granite walls
 *   FEAT_WALL_INNER -- inner room walls
 *   FEAT_WALL_OUTER -- outer room walls
 *   FEAT_WALL_SOLID -- solid room walls
 *   FEAT_PERM_EXTRA -- shop walls (perma)
 *   FEAT_PERM_INNER -- inner room walls (perma)
 *   FEAT_PERM_OUTER -- outer room walls (perma)
 *   FEAT_PERM_SOLID -- dungeon border (perma)
 */
#ifdef NEW_DUNGEON
static void build_tunnel(struct worldpos *wpos, int row1, int col1, int row2, int col2)
#else
static void build_tunnel(int Depth, int row1, int col1, int row2, int col2)
#endif
{
	int			i, y, x;
	int			tmp_row, tmp_col;
	int                 row_dir, col_dir;
	int                 start_row, start_col;
	int			main_loop_count = 0;

	bool		door_flag = FALSE;
	cave_type		*c_ptr;

#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif


	/* Reset the arrays */
	dun->tunn_n = 0;
	dun->wall_n = 0;

	/* Save the starting location */
	start_row = row1;
	start_col = col1;

	/* Start out in the correct direction */
	correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

	/* Keep going until done (or bored) */
	while ((row1 != row2) || (col1 != col2))
	{
		/* Mega-Hack -- Paranoia -- prevent infinite loops */
		if (main_loop_count++ > 2000) break;

		/* Allow bends in the tunnel */
		if (rand_int(100) < DUN_TUN_CHG)
		{
			/* Acquire the correct direction */
			correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

			/* Random direction */
			if (rand_int(100) < DUN_TUN_RND)
			{
				rand_dir(&row_dir, &col_dir);
			}
		}

		/* Get the next location */
		tmp_row = row1 + row_dir;
		tmp_col = col1 + col_dir;


		/* Extremely Important -- do not leave the dungeon */
#ifdef NEW_DUNGEON
		while (!in_bounds(tmp_row, tmp_col))
#else
		while (!in_bounds(Depth, tmp_row, tmp_col))
#endif
		{
			/* Acquire the correct direction */
			correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

			/* Random direction */
			if (rand_int(100) < DUN_TUN_RND)
			{
				rand_dir(&row_dir, &col_dir);
			}

			/* Get the next location */
			tmp_row = row1 + row_dir;
			tmp_col = col1 + col_dir;
		}


		/* Access the location */
#ifdef NEW_DUNGEON
		c_ptr = &zcave[tmp_row][tmp_col];
#else
		c_ptr = &cave[Depth][tmp_row][tmp_col];
#endif


		/* Avoid the edge of the dungeon */
		if (c_ptr->feat == FEAT_PERM_SOLID) continue;

		/* Avoid the edge of vaults */
		if (c_ptr->feat == FEAT_PERM_OUTER) continue;

		/* Avoid "solid" granite walls */
		if (c_ptr->feat == FEAT_WALL_SOLID) continue;

		/* Pierce "outer" walls of rooms */
		if (c_ptr->feat == FEAT_WALL_OUTER)
		{
			/* Acquire the "next" location */
			y = tmp_row + row_dir;
			x = tmp_col + col_dir;

			/* Hack -- Avoid outer/solid permanent walls */
#ifdef NEW_DUNGEON
			if (zcave[y][x].feat == FEAT_PERM_SOLID) continue;
			if (zcave[y][x].feat == FEAT_PERM_OUTER) continue;

			/* Hack -- Avoid outer/solid granite walls */
			if (zcave[y][x].feat == FEAT_WALL_OUTER) continue;
			if (zcave[y][x].feat == FEAT_WALL_SOLID) continue;
#else
			if (cave[Depth][y][x].feat == FEAT_PERM_SOLID) continue;
			if (cave[Depth][y][x].feat == FEAT_PERM_OUTER) continue;

			/* Hack -- Avoid outer/solid granite walls */
			if (cave[Depth][y][x].feat == FEAT_WALL_OUTER) continue;
			if (cave[Depth][y][x].feat == FEAT_WALL_SOLID) continue;
#endif

			/* Accept this location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Save the wall location */
			if (dun->wall_n < WALL_MAX)
			{
				dun->wall[dun->wall_n].y = row1;
				dun->wall[dun->wall_n].x = col1;
				dun->wall_n++;
			}

#ifdef WIDE_CORRIDORS
			/* Save the wall location */
			if (dun->wall_n < WALL_MAX)
			{
#ifdef NEW_DUNGEON
				if (zcave[row1 + col_dir][col1 + row_dir].feat != FEAT_PERM_SOLID &&
				    zcave[row1 + col_dir][col1 + row_dir].feat != FEAT_PERM_SOLID)
#else
				if (cave[Depth][row1 + col_dir][col1 + row_dir].feat != FEAT_PERM_SOLID &&
				    cave[Depth][row1 + col_dir][col1 + row_dir].feat != FEAT_PERM_SOLID)
#endif
				{
					dun->wall[dun->wall_n].y = row1 + col_dir;
					dun->wall[dun->wall_n].x = col1 + row_dir;
					dun->wall_n++;
				}
				else
				{
					dun->wall[dun->wall_n].y = row1;
					dun->wall[dun->wall_n].x = col1;
					dun->wall_n++;
				}
			}
#endif

#ifdef WIDE_CORRIDORS
			/* Forbid re-entry near this piercing */
			for (y = row1 - 2; y <= row1 + 2; y++)
			{
				for (x = col1 - 2; x <= col1 + 2; x++)
				{
					/* Be sure we are "in bounds" */
#ifdef NEW_DUNGEON
					if (!in_bounds(y, x)) continue;
#else
					if (!in_bounds(Depth, y, x)) continue;
#endif

					/* Convert adjacent "outer" walls as "solid" walls */
#ifdef NEW_DUNGEON
					if (zcave[y][x].feat == FEAT_WALL_OUTER)
#else
					if (cave[Depth][y][x].feat == FEAT_WALL_OUTER)
#endif
					{
						/* Change the wall to a "solid" wall */
#ifdef NEW_DUNGEON
						zcave[y][x].feat = FEAT_WALL_SOLID;
#else
						cave[Depth][y][x].feat = FEAT_WALL_SOLID;
#endif
					}
				}
			}
#else
			/* Forbid re-entry near this piercing */
			for (y = row1 - 1; y <= row1 + 1; y++)
			{
				for (x = col1 - 1; x <= col1 + 1; x++)
				{
					/* Convert adjacent "outer" walls as "solid" walls */
#ifdef NEW_DUNGEON
					if (zcave[y][x].feat == FEAT_WALL_OUTER)
#else
					if (cave[Depth][y][x].feat == FEAT_WALL_OUTER)
#endif
					{
						/* Change the wall to a "solid" wall */
#ifdef NEW_DUNGEON
						zcave[y][x].feat = FEAT_WALL_SOLID;
#else
						cave[Depth][y][x].feat = FEAT_WALL_SOLID;
#endif
					}
				}
			}
#endif
		}

		/* Travel quickly through rooms */
		else if (c_ptr->info & CAVE_ROOM)
		{
			/* Accept the location */
			row1 = tmp_row;
			col1 = tmp_col;
		}

		/* Tunnel through all other walls */
		else if (c_ptr->feat >= FEAT_WALL_EXTRA)
		{
			/* Accept this location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Save the tunnel location */
			if (dun->tunn_n < TUNN_MAX)
			{
				dun->tunn[dun->tunn_n].y = row1;
				dun->tunn[dun->tunn_n].x = col1;
				dun->tunn_n++;
			}

#ifdef WIDE_CORRIDORS
			/* Note that this is incredibly hacked */

			/* Make sure we're in bounds */
#ifdef NEW_DUNGEON
			if (in_bounds(row1 + col_dir, col1 + row_dir))
#else
			if (in_bounds(Depth, row1 + col_dir, col1 + row_dir))
#endif
			{
				/* New spot to be hollowed out */
#ifdef NEW_DUNGEON
				c_ptr = &zcave[row1 + col_dir][col1 + row_dir];
#else
				c_ptr = &cave[Depth][row1 + col_dir][col1 + row_dir];
#endif

				/* Make sure it's a wall we want to tunnel */
				if (c_ptr->feat == FEAT_WALL_EXTRA)
				{
					/* Save the tunnel location */
					if (dun->tunn_n < TUNN_MAX)
					{
						dun->tunn[dun->tunn_n].y = row1 + col_dir;
						dun->tunn[dun->tunn_n].x = col1 + row_dir;
						dun->tunn_n++;
					}
				}
			}
#endif

			/* Allow door in next grid */
			door_flag = FALSE;
		}

		/* Handle corridor intersections or overlaps */
		else
		{
			/* Accept the location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Collect legal door locations */
			if (!door_flag)
			{
				/* Save the door location */
				if (dun->door_n < DOOR_MAX)
				{
					dun->door[dun->door_n].y = row1;
					dun->door[dun->door_n].x = col1;
					dun->door_n++;
				}

#ifdef WIDE_CORRIDORS
				/* Save the next door location */
				if (dun->door_n < DOOR_MAX)
				{
#ifdef NEW_DUNGEON
					if (in_bounds(row1 + col_dir, col1 + row_dir))
#else
					if (in_bounds(Depth, row1 + col_dir, col1 + row_dir))
#endif
					{
						dun->door[dun->door_n].y = row1 + col_dir;
						dun->door[dun->door_n].x = col1 + row_dir;
						dun->door_n++;
					}
					
					/* Hack -- Duplicate the previous door */
					else
					{
						dun->door[dun->door_n].y = row1;
						dun->door[dun->door_n].x = col1;
						dun->door_n++;
					}
				}
#endif

				/* No door in next grid */
				door_flag = TRUE;
			}

			/* Hack -- allow pre-emptive tunnel termination */
			if (rand_int(100) >= DUN_TUN_CON)
			{
				/* Distance between row1 and start_row */
				tmp_row = row1 - start_row;
				if (tmp_row < 0) tmp_row = (-tmp_row);

				/* Distance between col1 and start_col */
				tmp_col = col1 - start_col;
				if (tmp_col < 0) tmp_col = (-tmp_col);

				/* Terminate the tunnel */
				if ((tmp_row > 10) || (tmp_col > 10)) break;
			}
		}
	}


	/* Turn the tunnel into corridor */
	for (i = 0; i < dun->tunn_n; i++)
	{
		/* Access the grid */
		y = dun->tunn[i].y;
		x = dun->tunn[i].x;

		/* Access the grid */
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x];
#else
		c_ptr = &cave[Depth][y][x];
#endif

		/* Clear previous contents, add a floor */
		c_ptr->feat = FEAT_FLOOR;
	}


	/* Apply the piercings that we found */
	for (i = 0; i < dun->wall_n; i++)
	{
		int feat;

		/* Access the grid */
		y = dun->wall[i].y;
		x = dun->wall[i].x;

		/* Access the grid */
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x];
#else
		c_ptr = &cave[Depth][y][x];
#endif

		/* Clear previous contents, add up floor */
		c_ptr->feat = FEAT_FLOOR;

		/* Occasional doorway */
		if (rand_int(100) < DUN_TUN_PEN)
		{
#ifdef NEW_DUNGEON
			/* Place a random door */
			place_random_door(wpos, y, x);

			/* Remember type of door */
			feat = zcave[y][x].feat;
#else
			/* Place a random door */
			place_random_door(Depth, y, x);

			/* Remember type of door */
			feat = cave[Depth][y][x].feat;
#endif

#ifdef WIDE_CORRIDORS
			/* Make sure both halves get a door */
			if (i % 2)
			{
				/* Access the grid */
				y = dun->wall[i - 1].y;
				x = dun->wall[i - 1].x;
			}
			else
			{
				/* Access the grid */
				y = dun->wall[i + 1].y;
				x = dun->wall[i + 1].x;

				/* Increment counter */
				i++;
			}

			/* Place the same type of door */
#ifdef NEW_DUNGEON
			zcave[y][x].feat = feat;
#else
			cave[Depth][y][x].feat = feat;
#endif /*NEW_DUNGEON*/
#endif
		}
	}
}




/*
 * Count the number of "corridor" grids adjacent to the given grid.
 *
 * Note -- Assumes "in_bounds(y1, x1)"
 *
 * XXX XXX This routine currently only counts actual "empty floor"
 * grids which are not in rooms.  We might want to also count stairs,
 * open doors, closed doors, etc.
 */
#ifdef NEW_DUNGEON
static int next_to_corr(struct worldpos *wpos, int y1, int x1)
#else
static int next_to_corr(int Depth, int y1, int x1)
#endif
{
	int i, y, x, k = 0;
	cave_type *c_ptr;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);
#endif

	/* Scan adjacent grids */
	for (i = 0; i < 4; i++)
	{
		/* Extract the location */
		y = y1 + ddy_ddd[i];
		x = x1 + ddx_ddd[i];

#ifdef NEW_DUNGEON
		/* Skip non floors */
		if (!cave_floor_bold(zcave, y, x)) continue;
		/* Access the grid */
		c_ptr = &zcave[y][x];
#else
		/* Skip non floors */
		if (!cave_floor_bold(Depth, y, x)) continue;
		/* Access the grid */
		c_ptr = &cave[Depth][y][x];
#endif


		/* Skip non "empty floor" grids */
		if (c_ptr->feat != FEAT_FLOOR) continue;

		/* Skip grids inside rooms */
		if (c_ptr->info & CAVE_ROOM) continue;

		/* Count these grids */
		k++;
	}

	/* Return the number of corridors */
	return (k);
}


/*
 * Determine if the given location is "between" two walls,
 * and "next to" two corridor spaces.  XXX XXX XXX
 *
 * Assumes "in_bounds(y,x)"
 */
#ifdef NEW_DUNGEON
static bool possible_doorway(struct worldpos *wpos, int y, int x)
#else
static bool possible_doorway(int Depth, int y, int x)
#endif
{
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);
#endif
	/* Count the adjacent corridors */
#ifdef NEW_DUNGEON
	if (next_to_corr(wpos, y, x) >= 2)
#else
	if (next_to_corr(Depth, y, x) >= 2)
#endif
	{
#ifdef NEW_DUNGEON
		/* Check Vertical */
		if ((zcave[y-1][x].feat >= FEAT_MAGMA) &&
		    (zcave[y+1][x].feat >= FEAT_MAGMA))
#else
		/* Check Vertical */
		if ((cave[Depth][y-1][x].feat >= FEAT_MAGMA) &&
		    (cave[Depth][y+1][x].feat >= FEAT_MAGMA))
#endif
		{
			return (TRUE);
		}

#ifdef NEW_DUNGEON
		/* Check Horizontal */
		if ((zcave[y][x-1].feat >= FEAT_MAGMA) &&
		    (zcave[y][x+1].feat >= FEAT_MAGMA))
#else
		/* Check Horizontal */
		if ((cave[Depth][y][x-1].feat >= FEAT_MAGMA) &&
		    (cave[Depth][y][x+1].feat >= FEAT_MAGMA))
#endif
		{
			return (TRUE);
		}
	}

	/* No doorway */
	return (FALSE);
}


/*
 * Places door at y, x position if at least 2 walls found
 */
#ifdef NEW_DUNGEON
static void try_door(struct worldpos *wpos, int y, int x)
#else
static void try_door(int Depth, int y, int x)
#endif
{
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif
	/* Paranoia */
#ifdef NEW_DUNGEON
	if (!in_bounds(y, x)) return;
#else
	if (!in_bounds(Depth, y, x)) return;
#endif

#ifdef NEW_DUNGEON
	/* Ignore walls */
	if (zcave[y][x].feat >= FEAT_MAGMA) return;

	/* Ignore room grids */
	if (zcave[y][x].info & CAVE_ROOM) return;

	/* Occasional door (if allowed) */
	if ((rand_int(100) < DUN_TUN_JCT) && possible_doorway(wpos, y, x))
	{
		/* Place a door */
		place_random_door(wpos, y, x);
	}
#else
	/* Ignore walls */
	if (cave[Depth][y][x].feat >= FEAT_MAGMA) return;

	/* Ignore room grids */
	if (cave[Depth][y][x].info & CAVE_ROOM) return;

	/* Occasional door (if allowed) */
	if ((rand_int(100) < DUN_TUN_JCT) && possible_doorway(Depth, y, x))
	{
		/* Place a door */
		place_random_door(Depth, y, x);
	}
#endif
}




/*
 * Attempt to build a room of the given type at the given block
 *
 * Note that we restrict the number of "crowded" rooms to reduce
 * the chance of overflowing the monster list during level creation.
 */
#ifdef NEW_DUNGEON
static bool room_build(struct worldpos *wpos, int y0, int x0, int typ)
#else
static bool room_build(int Depth, int y0, int x0, int typ)
#endif
{
	int y, x, y1, x1, y2, x2;


	/* Restrict level */
#ifdef NEW_DUNGEON
	if (getlevel(wpos) < room[typ].level) return (FALSE);
#else
	if (Depth < room[typ].level) return (FALSE);
#endif

	/* Restrict "crowded" rooms */
	if (dun->crowded && ((typ == 5) || (typ == 6))) return (FALSE);

	/* Extract blocks */
	y1 = y0 + room[typ].dy1;
	y2 = y0 + room[typ].dy2;
	x1 = x0 + room[typ].dx1;
	x2 = x0 + room[typ].dx2;

	/* Never run off the screen */
	if ((y1 < 0) || (y2 >= dun->row_rooms)) return (FALSE);
	if ((x1 < 0) || (x2 >= dun->col_rooms)) return (FALSE);

	/* Verify open space */
	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			if (dun->room_map[y][x]) return (FALSE);
		}
	}

	/* XXX XXX XXX It is *extremely* important that the following */
	/* calculation is *exactly* correct to prevent memory errors */

	/* Acquire the location of the room */
	y = ((y1 + y2 + 1) * BLOCK_HGT) / 2;
	x = ((x1 + x2 + 1) * BLOCK_WID) / 2;

	/* Build a room */
	switch (typ)
	{
		/* Build an appropriate room */
#ifdef NEW_DUNGEON
		case 8: build_type8(wpos, y, x); break;
		case 7: build_type7(wpos, y, x); break;
		case 6: build_type6(wpos, y, x); break;
		case 5: build_type5(wpos, y, x); break;
		case 4: build_type4(wpos, y, x); break;
		case 3: build_type3(wpos, y, x); break;
		case 2: build_type2(wpos, y, x); break;
		case 1: build_type1(wpos, y, x); break;
#else
		case 8: build_type8(Depth, y, x); break;
		case 7: build_type7(Depth, y, x); break;
		case 6: build_type6(Depth, y, x); break;
		case 5: build_type5(Depth, y, x); break;
		case 4: build_type4(Depth, y, x); break;
		case 3: build_type3(Depth, y, x); break;
		case 2: build_type2(Depth, y, x); break;
		case 1: build_type1(Depth, y, x); break;
#endif

		/* Paranoia */
		default: return (FALSE);
	}

	/* Save the room location */
	if (dun->cent_n < CENT_MAX)
	{
		dun->cent[dun->cent_n].y = y;
		dun->cent[dun->cent_n].x = x;
		dun->cent_n++;
	}

	/* Reserve some blocks */
	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			dun->room_map[y][x] = TRUE;
		}
	}

	/* Count "crowded" rooms */
	if ((typ == 5) || (typ == 6)) dun->crowded = TRUE;

	/* Success */
	return (TRUE);
}


/*
 * Generate a new dungeon level
 *
 * Note that "dun_body" adds about 4000 bytes of memory to the stack.
 */
#ifdef NEW_DUNGEON
static void cave_gen(struct worldpos *wpos)
#else
static void cave_gen(int Depth)
#endif
{
	int i, k, y, x, y1, x1;

	bool destroyed = FALSE;

	dun_data dun_body;

#ifdef NEW_DUNGEON
	cave_type **zcave;
	wilderness_type *wild;
	u32b flags;

	if(!(zcave=getcave(wpos))) return;
#endif
	wild=&wild_info[wpos->wy][wpos->wx];
	flags=(wpos->wz>0 ? wild->tower->flags : wild->dungeon->flags);

	if(!flags & DUNGEON_RANDOM) return;

	/* Global data */
	dun = &dun_body;


	/* Hack -- Start with basic granite */
	for (y = 0; y < MAX_HGT; y++)
	{
		for (x = 0; x < MAX_WID; x++)
		{
#ifdef NEW_DUNGEON
			cave_type *c_ptr = &zcave[y][x];
#else
			cave_type *c_ptr = &cave[Depth][y][x];
#endif

			/* Create granite wall */
			c_ptr->feat = FEAT_WALL_EXTRA;
		}
	}


#ifdef NEW_DUNGEON
	/* Possible "destroyed" level */
	if ((getlevel(wpos) > 10) && (rand_int(DUN_DEST) == 0)) destroyed = TRUE;

	/* Hack -- No destroyed "quest" levels */
	if (is_quest(wpos)) destroyed = FALSE;
#else
	/* Possible "destroyed" level */
	if ((Depth > 10) && (rand_int(DUN_DEST) == 0)) destroyed = TRUE;

	/* Hack -- No destroyed "quest" levels */
	if (is_quest(Depth)) destroyed = FALSE;
#endif


	/* Actual maximum number of rooms on this level */
	dun->row_rooms = MAX_HGT / BLOCK_HGT;
	dun->col_rooms = MAX_WID / BLOCK_WID;

	/* Initialize the room table */
	for (y = 0; y < dun->row_rooms; y++)
	{
		for (x = 0; x < dun->col_rooms; x++)
		{
			dun->room_map[y][x] = FALSE;
		}
	}


	/* No "crowded" rooms yet */
	dun->crowded = FALSE;


	/* No rooms yet */
	dun->cent_n = 0;

	/* Build some rooms */
	for (i = 0; i < DUN_ROOMS; i++)
	{
		/* Pick a block for the room */
		y = rand_int(dun->row_rooms);
		x = rand_int(dun->col_rooms);

		/* Align dungeon rooms */
		if (dungeon_align)
		{
			/* Slide some rooms right */
			if ((x % 3) == 0) x++;

			/* Slide some rooms left */
			if ((x % 3) == 2) x--;
		}

		/* Destroyed levels are boring */
		if (destroyed)
		{
			/* Attempt a "trivial" room */
#ifdef NEW_DUNGEON
			if (room_build(wpos, y, x, 1)) continue;
#else
			if (room_build(Depth, y, x, 1)) continue;
#endif

			/* Never mind */
			continue;
		}

#ifdef NEW_DUNGEON
		/* Attempt an "unusual" room */
		if (rand_int(DUN_UNUSUAL) < getlevel(wpos))
#else
		/* Attempt an "unusual" room */
		if (rand_int(DUN_UNUSUAL) < Depth)
#endif
		{
			/* Roll for room type */
			k = rand_int(100);

			/* Attempt a very unusual room */
#ifdef NEW_DUNGEON
			if (rand_int(DUN_UNUSUAL) < getlevel(wpos))
#else
			if (rand_int(DUN_UNUSUAL) < Depth)
#endif
			{
#ifdef NEW_DUNGEON
				/* Type 8 -- Greater vault (10%) */
				if ((k < 10) && room_build(wpos, y, x, 8)) continue;

				/* Type 7 -- Lesser vault (15%) */
				if ((k < 25) && room_build(wpos, y, x, 7)) continue;

				/* Type 6 -- Monster pit (15%) */
				if ((k < 40) && room_build(wpos, y, x, 6)) continue;

				/* Type 5 -- Monster nest (10%) */
				if ((k < 50) && room_build(wpos, y, x, 5)) continue;
#else
				/* Type 8 -- Greater vault (10%) */
				if ((k < 10) && room_build(Depth, y, x, 8)) continue;

				/* Type 7 -- Lesser vault (15%) */
				if ((k < 25) && room_build(Depth, y, x, 7)) continue;

				/* Type 6 -- Monster pit (15%) */
				if ((k < 40) && room_build(Depth, y, x, 6)) continue;

				/* Type 5 -- Monster nest (10%) */
				if ((k < 50) && room_build(Depth, y, x, 5)) continue;
#endif
			}

#ifdef NEW_DUNGEON
			/* Type 4 -- Large room (25%) */
			if ((k < 25) && room_build(wpos, y, x, 4)) continue;

			/* Type 3 -- Cross room (25%) */
			if ((k < 50) && room_build(wpos, y, x, 3)) continue;

			/* Type 2 -- Overlapping (50%) */
			if ((k < 100) && room_build(wpos, y, x, 2)) continue;
#else
			/* Type 4 -- Large room (25%) */
			if ((k < 25) && room_build(Depth, y, x, 4)) continue;

			/* Type 3 -- Cross room (25%) */
			if ((k < 50) && room_build(Depth, y, x, 3)) continue;

			/* Type 2 -- Overlapping (50%) */
			if ((k < 100) && room_build(Depth, y, x, 2)) continue;
#endif
		}

		/* Attempt a trivial room */
#ifdef NEW_DUNGEON
		if (room_build(wpos, y, x, 1)) continue;
#else
		if (room_build(Depth, y, x, 1)) continue;
#endif
	}


	/* Special boundary walls -- Top */
	for (x = 0; x < MAX_WID; x++)
	{
#ifdef NEW_DUNGEON
		cave_type *c_ptr = &zcave[0][x];
#else
		cave_type *c_ptr = &cave[Depth][0][x];
#endif

		/* Clear previous contents, add "solid" perma-wall */
		c_ptr->feat = FEAT_PERM_SOLID;
	}

	/* Special boundary walls -- Bottom */
	for (x = 0; x < MAX_WID; x++)
	{
#ifdef NEW_DUNGEON
		cave_type *c_ptr = &zcave[MAX_HGT-1][x];
#else
		cave_type *c_ptr = &cave[Depth][MAX_HGT-1][x];
#endif

		/* Clear previous contents, add "solid" perma-wall */
		c_ptr->feat = FEAT_PERM_SOLID;
	}

	/* Special boundary walls -- Left */
	for (y = 0; y < MAX_HGT; y++)
	{
#ifdef NEW_DUNGEON
		cave_type *c_ptr = &zcave[y][0];
#else
		cave_type *c_ptr = &cave[Depth][y][0];
#endif

		/* Clear previous contents, add "solid" perma-wall */
		c_ptr->feat = FEAT_PERM_SOLID;
	}

	/* Special boundary walls -- Right */
	for (y = 0; y < MAX_HGT; y++)
	{
#ifdef NEW_DUNGEON
		cave_type *c_ptr = &zcave[y][MAX_WID-1];
#else
		cave_type *c_ptr = &cave[Depth][y][MAX_WID-1];
#endif

		/* Clear previous contents, add "solid" perma-wall */
		c_ptr->feat = FEAT_PERM_SOLID;
	}


	/* Hack -- Scramble the room order */
	for (i = 0; i < dun->cent_n; i++)
	{
		int pick1 = rand_int(dun->cent_n);
		int pick2 = rand_int(dun->cent_n);
		y1 = dun->cent[pick1].y;
		x1 = dun->cent[pick1].x;
		dun->cent[pick1].y = dun->cent[pick2].y;
		dun->cent[pick1].x = dun->cent[pick2].x;
		dun->cent[pick2].y = y1;
		dun->cent[pick2].x = x1;
	}

	/* Start with no tunnel doors */
	dun->door_n = 0;

	/* Hack -- connect the first room to the last room */
	y = dun->cent[dun->cent_n-1].y;
	x = dun->cent[dun->cent_n-1].x;

	/* Connect all the rooms together */
	for (i = 0; i < dun->cent_n; i++)
	{
		/* Connect the room to the previous room */
#ifdef NEW_DUNGEON
		build_tunnel(wpos, dun->cent[i].y, dun->cent[i].x, y, x);
#else
		build_tunnel(Depth, dun->cent[i].y, dun->cent[i].x, y, x);
#endif

		/* Remember the "previous" room */
		y = dun->cent[i].y;
		x = dun->cent[i].x;
	}

	/* Place intersection doors	 */
	for (i = 0; i < dun->door_n; i++)
	{
		/* Extract junction location */
		y = dun->door[i].y;
		x = dun->door[i].x;

		/* Try placing doors */
#ifdef NEW_DUNGEON
		try_door(wpos, y, x - 1);
		try_door(wpos, y, x + 1);
		try_door(wpos, y - 1, x);
		try_door(wpos, y + 1, x);
#else
		try_door(Depth, y, x - 1);
		try_door(Depth, y, x + 1);
		try_door(Depth, y - 1, x);
		try_door(Depth, y + 1, x);
#endif
	}


	/* Hack -- Add some magma streamers */
	for (i = 0; i < DUN_STR_MAG; i++)
	{
#ifdef NEW_DUNGEON
		build_streamer(wpos, FEAT_MAGMA, DUN_STR_MC);
#else
		build_streamer(Depth, FEAT_MAGMA, DUN_STR_MC);
#endif
	}

	/* Hack -- Add some quartz streamers */
	for (i = 0; i < DUN_STR_QUA; i++)
	{
#ifdef NEW_DUNGEON
		build_streamer(wpos, FEAT_QUARTZ, DUN_STR_QC);
#else
		build_streamer(Depth, FEAT_QUARTZ, DUN_STR_QC);
#endif
	}


#ifdef NEW_DUNGEON
	/* Destroy the level if necessary */
	if (destroyed) destroy_level(wpos);


	/* Place 3 or 4 down stairs near some walls */
	alloc_stairs(wpos, FEAT_MORE, rand_range(3, 4), 3);

	/* Place 1 or 2 up stairs near some walls */
	alloc_stairs(wpos, FEAT_LESS, rand_range(1, 2), 3);


	/* Determine the character location */
	new_player_spot(wpos);


	/* Basic "amount" */
	k = (getlevel(wpos) / 3);
#else
	/* Destroy the level if necessary */
	if (destroyed) destroy_level(Depth);


	/* Place 3 or 4 down stairs near some walls */
	alloc_stairs(Depth, FEAT_MORE, rand_range(3, 4), 3);

	/* Place 1 or 2 up stairs near some walls */
	alloc_stairs(Depth, FEAT_LESS, rand_range(1, 2), 3);


	/* Determine the character location */
	new_player_spot(Depth);


	/* Basic "amount" */
	k = (Depth / 3);
#endif
	if (k > 10) k = 10;
	if (k < 2) k = 2;


	/* Pick a base number of monsters */
	i = MIN_M_ALLOC_LEVEL + randint(8);

	/* Put some monsters in the dungeon */
	for (i = i + k; i > 0; i--)
	{
#ifdef NEW_DUNGEON
		(void)alloc_monster(wpos, 0, TRUE);
#else
		(void)alloc_monster(Depth, 0, TRUE);
#endif
	}


#ifdef NEW_DUNGEON
	/* Place some traps in the dungeon */
	alloc_object(wpos, ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint(k));

	/* Put some rubble in corridors */
	alloc_object(wpos, ALLOC_SET_CORR, ALLOC_TYP_RUBBLE, randint(k));

	/* Put some objects in rooms */
	alloc_object(wpos, ALLOC_SET_ROOM, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ROOM, 3));

	/* Put some objects/gold in the dungeon */
	alloc_object(wpos, ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ITEM, 3));
	alloc_object(wpos, ALLOC_SET_BOTH, ALLOC_TYP_GOLD, randnor(DUN_AMT_GOLD, 3));
#else
	/* Place some traps in the dungeon */
	alloc_object(Depth, ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint(k));

	/* Put some rubble in corridors */
	alloc_object(Depth, ALLOC_SET_CORR, ALLOC_TYP_RUBBLE, randint(k));

	/* Put some objects in rooms */
	alloc_object(Depth, ALLOC_SET_ROOM, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ROOM, 3));

	/* Put some objects/gold in the dungeon */
	alloc_object(Depth, ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ITEM, 3));
	alloc_object(Depth, ALLOC_SET_BOTH, ALLOC_TYP_GOLD, randnor(DUN_AMT_GOLD, 3));
#endif
}



/*
 * Builds a store at a given (row, column)
 *
 * Note that the solid perma-walls are at x=0/65 and y=0/21
 *
 * As of 2.7.4 (?) the stores are placed in a more "user friendly"
 * configuration, such that the four "center" buildings always
 * have at least four grids between them, to allow easy running,
 * and the store doors tend to face the middle of town.
 *
 * The stores now lie inside boxes from 3-9 and 12-18 vertically,
 * and from 7-17, 21-31, 35-45, 49-59.  Note that there are thus
 * always at least 2 open grids between any disconnected walls.
 */
static void build_store(struct worldpos *wpos, int n, int yy, int xx)
{
	int                 i, y, x, y0, x0, y1, x1, y2, x2, tmp;
	cave_type		*c_ptr;

#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return; /*multitowns*/
#endif

	y0 = yy * 11 + 5;
	x0 = xx * 16 + 12;

	/* Determine the store boundaries */
	y1 = y0 - randint((yy == 0) ? 3 : 2);
	y2 = y0 + randint((yy == 1) ? 3 : 2);
	x1 = x0 - randint(5);
	x2 = x0 + randint(5);

	/* Hack -- make building 9's as large as possible */
	if (n == 12)
	{
		y1 = y0 - 3;
		y2 = y0 + 3;
		x1 = x0 - 5;
		x2 = x0 + 5;
	}

	/* Build an invulnerable rectangular building */
	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			/* Get the grid */
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][x];
#else
			c_ptr = &cave[0][y][x];
#endif

			/* Clear previous contents, add "basic" perma-wall */
			c_ptr->feat = FEAT_PERM_EXTRA;

			/* Hack -- The buildings are illuminated and known */
			/* c_ptr->info |= (CAVE_GLOW); */
		}
	}

	/* Hack -- Make store "8" (the old home) empty */
	if (n == 7)
	{
		for (y = y1 + 1; y < y2; y++)
		{
			for (x = x1 + 1; x < x2; x++)
			{
				/* Get the grid */
#ifdef NEW_DUNGEON
				c_ptr = &zcave[y][x];
#else
				c_ptr = &cave[0][y][x];
#endif

				/* Clear contents */
				c_ptr->feat = FEAT_FLOOR;

				/* Declare this to be a room */
				c_ptr->info |= CAVE_ROOM | CAVE_GLOW;
			}
		}

		/* Hack -- have everyone start in the tavern */
#ifdef NEW_DUNGEON
		new_level_down_y(wpos, (y1 + y2) / 2);
		new_level_down_x(wpos, (x1 + x2) / 2);
#else
		level_down_y[0] = (y1 + y2) / 2;
		level_down_x[0] = (x1 + x2) / 2;
#endif
	}

	/* Make doorways, fill with grass and trees */
	if (n == 9)
	{
		for (y = y1 + 1; y < y2; y++)
		{
			for (x = x1 + 1; x < x2; x++)
			{
				/* Get the grid */
#ifdef NEW_DUNGEON
				c_ptr = &zcave[y][x];
#else
				c_ptr = &cave[0][y][x];
#endif

				/* Clear contents */
				c_ptr->feat = FEAT_GRASS;
			}
		}

		y = (y1 + y2) / 2;
		x = (x1 + x2) / 2;

#ifdef NEW_DUNGEON
		zcave[y1][x].feat = FEAT_GRASS;
		zcave[y2][x].feat = FEAT_GRASS;
		zcave[y][x1].feat = FEAT_GRASS;
		zcave[y][x2].feat = FEAT_GRASS;
#else
		cave[0][y1][x].feat = FEAT_GRASS;
		cave[0][y2][x].feat = FEAT_GRASS;
		cave[0][y][x1].feat = FEAT_GRASS;
		cave[0][y][x2].feat = FEAT_GRASS;
#endif

		for (i = 0; i < 5; i++)
		{
			y = rand_range(y1 + 1, y2 - 1);
			x = rand_range(x1 + 1, x2 - 1);

#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][x];
#else
			c_ptr = &cave[0][y][x];
#endif

			c_ptr->feat = FEAT_TREE;
		}

		return;
	}
			
	/* Pond */
	if (n == 10)
	{
		for (y = y1; y <= y2; y++)
		{
			for (x = x1; x <= x2; x++)
			{
				/* Get the grid */
#ifdef NEW_DUNGEON
				c_ptr = &zcave[y][x];
#else
				c_ptr = &cave[0][y][x];
#endif

				/* Fill with water */
				c_ptr->feat = FEAT_WATER;
			}
		}

		/* Make the pond not so "square" */
#ifdef NEW_DUNGEON
		zcave[y1][x1].feat = FEAT_DIRT;
		zcave[y1][x2].feat = FEAT_DIRT;
		zcave[y2][x1].feat = FEAT_DIRT;
		zcave[y2][x2].feat = FEAT_DIRT;
#else
		cave[0][y1][x1].feat = FEAT_DIRT;
		cave[0][y1][x2].feat = FEAT_DIRT;
		cave[0][y2][x1].feat = FEAT_DIRT;
		cave[0][y2][x2].feat = FEAT_DIRT;
#endif

		return;
	}

	/* Building with stairs */
#ifdef NEW_DUNGEON
	if (n == 11 || n == 15)
#else
	if (n == 11)
#endif
	{
		for (y = y1; y <= y2; y++)
		{
			for (x = x1; x <= x2; x++)
			{
				/* Get the grid */
#ifdef NEW_DUNGEON
				c_ptr = &zcave[y][x];
#else
				c_ptr = &cave[0][y][x];
#endif

				/* Empty it */
				c_ptr->feat = FEAT_FLOOR;

				/* Put some grass */
				if (randint(100) < 50)
					c_ptr->feat = FEAT_GRASS;
			}
		}

		x = (x1 + x2) / 2;
		y = (y1 + y2) / 2;

		/* Access the stair grid */
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x];
#else
		c_ptr = &cave[0][y][x];
#endif

		/* Clear previous contents, add down stairs */
#ifdef NEW_DUNGEON
		if(n == 11){
			c_ptr->feat = FEAT_MORE;
			new_level_up_y(wpos, y);
			new_level_rand_y(wpos, y);
			new_level_up_x(wpos, x);
			new_level_rand_x(wpos, x);
		}
		else{
			c_ptr->feat = FEAT_LESS;
			new_level_down_y(wpos, y);
			new_level_down_x(wpos, x);
		}
#else
		c_ptr->feat = FEAT_MORE;

		/* Hack -- the players start on the stairs while coming up */
		level_up_y[0] = level_rand_y[0] = y;
		level_up_x[0] = level_rand_x[0] = x;
#endif

		return;
	}

	/* Forest */
	if (n == 12)
	{
		int xc, yc, max_dis;

		/* Find the center of the forested area */
		xc = (x1 + x2) / 2;
		yc = (y1 + y2) / 2;

		/* Find the max distance from center */
		max_dis = distance(y2, x2, yc, xc);

		for (y = y1; y <= y2; y++)
		{
			for (x = x1; x <= x2; x++)
			{
				int chance;

				/* Get the grid */
#ifdef NEW_DUNGEON
				c_ptr = &zcave[y][x];
#else
				c_ptr = &cave[0][y][x];
#endif

				/* Empty it */
				c_ptr->feat = FEAT_GRASS;

				/* Calculate chance of a tree */
				chance = 100 * (distance(y, x, yc, xc));
				chance /= max_dis;
				chance = 80 - chance;

				/* Put some trees */
				if (randint(100) < chance)
					c_ptr->feat = FEAT_TREE;
			}
		}

		return;
	}

	/* House */
	if (n == 13)
	{
		int price;

		for (y = y1 + 1; y < y2; y++)
		{
			for (x = x1 + 1; x < x2; x++)
			{
				/* Get the grid */
#ifdef NEW_DUNGEON
				c_ptr = &zcave[y][x];
#else
				c_ptr = &cave[0][y][x];
#endif

				/* Fill with floor */
				c_ptr->feat = FEAT_FLOOR;

				/* Make it "icky" */
				c_ptr->info |= CAVE_ICKY;
			}
		}

		/* Setup some "house info" */
		price = (x2 - x1 - 1) * (y2 - y1 - 1);
		/*price *= 20 * price; -APD- price is now proportional to size*/
		price *= 20;
		price *= 80 + randint(40);

		/* Remember price */
#ifdef NEWHOUSES
		MAKE(houses[num_houses].dna, struct dna_type);
		houses[num_houses].dna->price = price;
		houses[num_houses].x=x1;
		houses[num_houses].y=y1;
		houses[num_houses].flags=HF_RECT|HF_STOCK;
		houses[num_houses].coords.rect.width=x2-x1+1;
		houses[num_houses].coords.rect.height=y2-y1+1;
#else
		houses[num_houses].price = price;
		houses[num_houses].x_1 = x1+1;
		houses[num_houses].y_1 = y1+1;
		houses[num_houses].x_2 = x2-1;
		houses[num_houses].y_2 = y2-1;
#endif
#ifdef NEW_DUNGEON
		wpcopy(&houses[num_houses].wpos, wpos);
#else
		houses[num_houses].depth = 0;
#endif
	}


	/* Pick a door direction (S,N,E,W) */
	tmp = rand_int(4);

	/* Re-roll "annoying" doors */
	while (((tmp == 0) && ((yy % 2) == 1)) ||
	    ((tmp == 1) && ((yy % 2) == 0)) ||
	    ((tmp == 2) && ((xx % 2) == 1)) ||
	    ((tmp == 3) && ((xx % 2) == 0)))
	{
		/* Pick a new direction */
		tmp = rand_int(4);
	}

	/* Extract a "door location" */
	switch (tmp)
	{
		/* Bottom side */
		case 0:
		y = y2;
		x = rand_range(x1, x2);
		break;

		/* Top side */
		case 1:
		y = y1;
		x = rand_range(x1, x2);
		break;

		/* Right side */
		case 2:
		y = rand_range(y1, y2);
		x = x2;
		break;

		/* Left side */
		default:
		y = rand_range(y1, y2);
		x = x1;
		break;
	}

	/* Access the grid */
#ifdef NEW_DUNGEON
	c_ptr = &zcave[y][x];
#else
	c_ptr = &cave[0][y][x];
#endif

	/* Some buildings get special doors */
	if (n == 13)
	{
#ifndef NEWHOUSES
		c_ptr->feat = FEAT_HOME_HEAD + houses[num_houses].strength;
#endif

		/* hack -- only create houses that aren't already loaded from disk */
#ifdef NEW_DUNGEON
		if ((i=pick_house(wpos, y, x)) == -1)
#else
		if ((i=pick_house(0, y, x)) == -1)
#endif
		{
			/* Store door location information */
#ifdef NEWHOUSES
			c_ptr->feat = FEAT_HOME_HEAD;
			c_ptr->special.type=DNA_DOOR;
			c_ptr->special.ptr = houses[num_houses].dna;
			houses[num_houses].dx=x;
			houses[num_houses].dy=y;
			houses[num_houses].dna->creator=0L;
			houses[num_houses].dna->owner=0L;
#else
			houses[num_houses].door_y = y;
			houses[num_houses].door_x = x;
			houses[num_houses].owned = 0;
#endif

			/* One more house */
			num_houses++;
			if((house_alloc-num_houses)<32){
				GROW(houses, house_alloc, house_alloc+512, house_type);
				house_alloc+=512;
			}
		}
#ifdef NEWHOUSES
		else{
			KILL(houses[num_houses].dna, struct dna_type);
			c_ptr->feat=FEAT_HOME_HEAD;
			c_ptr->special.type=DNA_DOOR;
			c_ptr->special.ptr=houses[i].dna;
		}
#endif
	}
	else if (n == 14) // auctionhouse
	{
		c_ptr->feat = FEAT_PERM_EXTRA; // wants to work.	
	
	}
	else
	{
		/* Clear previous contents, add a store door */
		c_ptr->feat = FEAT_SHOP_HEAD + n;
	}
}

/*
 * Build a road.
 */
#ifdef NEW_DUNGEON
static void place_street(struct worldpos *wpos, int vert, int place)
#else
static void place_street(int vert, int place)
#endif
{
	int y, x, y1, y2, x1, x2;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif

	/* Vertical streets */
	if (vert)
	{
		x = place * 32 + 20;
		x1 = x - 2;
		x2 = x + 2;

		y1 = 5;
		y2 = MAX_HGT - 5;
	}
	else
	{
		y = place * 22 + 10;
		y1 = y - 2;
		y2 = y + 2;

		x1 = 5;
		x2 = MAX_WID - 5;
	}

	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
#ifdef NEW_DUNGEON
			if (zcave[y][x].feat != FEAT_FLOOR)
				zcave[y][x].feat = FEAT_GRASS;
#else
			if (cave[0][y][x].feat != FEAT_FLOOR)
				cave[0][y][x].feat = FEAT_GRASS;
#endif
		}
	}

	if (vert)
	{
		x1++;
		x2--;
	}
	else
	{
		y1++;
		y2--;
	}

	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
#ifdef NEW_DUNGEON
			zcave[y][x].feat = FEAT_FLOOR;
#else
			cave[0][y][x].feat = FEAT_FLOOR;
#endif
		}
	}
}



/*
 * Generate the "consistent" town features, and place the player
 *
 * Hack -- play with the R.N.G. to always yield the same town
 * layout, including the size and shape of the buildings, the
 * locations of the doorways, and the location of the stairs.
 */
#ifdef NEW_DUNGEON
static void town_gen_hack(struct worldpos *wpos)
#else
static void town_gen_hack(void)
#endif
{
	int			y, x, k, n;

#ifdef	DEVEL_TOWN_COMPATIBILITY
	/* -APD- the location of the auction house */
	int			auction_x, auction_y;
#endif

	int                 rooms[72];
	/* int                 rooms[MAX_STORES]; */
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif


	/* Hack -- Use the "simple" RNG */
	Rand_quick = TRUE;

	/* Hack -- Induce consistant town layout */
#ifdef NEW_DUNGEON
	Rand_value = seed_town+(wpos->wx+wpos->wy*MAX_WILD_X);
#else
	Rand_value = seed_town;
#endif

	/* Hack -- Start with basic floors */
	for (y = 1; y < MAX_HGT - 1; y++)
	{
		for (x = 1; x < MAX_WID - 1; x++)
		{
#ifdef NEW_DUNGEON
			cave_type *c_ptr = &zcave[y][x];
#else
			cave_type *c_ptr = &cave[0][y][x];
#endif

			/* Clear all features, set to "empty floor" */
			c_ptr->feat = FEAT_DIRT;

			if (rand_int(100) < 75)
			{
				c_ptr->feat = FEAT_GRASS;
			}

			else if (rand_int(100) < 15)
			{
				c_ptr->feat = FEAT_TREE;
			}
		}
	}

	/* Place horizontal "streets" */
	for (y = 0; y < 3; y++)
	{
#ifdef NEW_DUNGEON
		place_street(wpos, 0, y);
#else
		place_street(0, y);
#endif
	}

	/* Place vertical "streets" */
	for (x = 0; x < 6; x++)
	{
#ifdef NEW_DUNGEON
		place_street(wpos, 1, x);
#else
		place_street(1, x);
#endif
	}

#ifdef DEVEL_TOWN_COMPATIBILITY

	/* -APD- place the auction house near the central stores */

	auction_x = rand_int(5) + 3;
	if ( (auction_x == 3) || (auction_x == 8) ) auction_y = rand_int(1) + 1;
	else auction_y = (rand_int(1) * 3) + 1; // 1 or 4
#endif

	/* Prepare an Array of "remaining stores", and count them */
#ifdef NEW_DUNGEON
	for (n = 0; n < MAX_STORES-1; n++) rooms[n] = n;
	for (n = MAX_STORES-1; n < 16; n++) rooms[n] = 10;
	for (n = 16; n < 68; n++) rooms[n] = 13;
	for (n = 68; n < 72; n++) rooms[n] = 12;
	if(wild_info[wpos->wy][wpos->wx].flags & WILD_F_DOWN)
		rooms[70] = 11;
	if(wild_info[wpos->wy][wpos->wx].flags & WILD_F_UP)
		rooms[71] = 15;
#else
	for (n = 0; n < MAX_STORES-1; n++) rooms[n] = n;
	for (n = MAX_STORES-1; n < 16; n++) rooms[n] = 10;
	for (n = 16; n < 68; n++) rooms[n] = 13;
	for (n = 68; n < 71; n++) rooms[n] = 12;
	rooms[n++] = 11;
#endif

	/* Place stores */
	for (y = 2; y < 4; y++)
	{
		for (x = 4; x < 8; x++)
		{
			/* Pick a random unplaced store */
			k = rand_int(n - 64);

			/* Build that store at the proper location */
#ifdef NEW_DUNGEON
			build_store(wpos, rooms[k], y, x);
#else
			build_store(rooms[k], y, x);
#endif

			/* One less store */
			n--;

			/* Shift the stores down, remove one store */
			rooms[k] = rooms[n - 64];
		}
	}

	/* Place two rows of stores */
	for (y = 0; y < 6; y++)
	{
		/* Place four stores per row */
		for (x = 0; x < 12; x++)
		{
			/* Make sure we haven't already placed this one */
			if (y >= 2 && y <= 3 && x >= 4 && x <= 7) continue;
			
			/* Pick a random unplaced store */
			k = rand_int(n) + 8;

#ifdef	DEVEL_TOWN_COMPATIBILITY
			if ( (y != auction_y) || (x != auction_x) ) 
			{
				/* Build that store at the proper location */
#ifdef NEW_DUNGEON
				build_store(wpos, rooms[k], y, x);
#else
				build_store(rooms[k], y, x);
#endif
			}
			
			else /* auction time! */
			{
#ifdef NEW_DUNGEON
				build_store(wpos, 14, y, x);
#else
				build_store(14, y, x);
#endif
			
			}
#else
#ifdef NEW_DUNGEON
			build_store(wpos, rooms[k], y, x);
#else
			build_store(rooms[k], y, x);
#endif
#endif

			/* One less building */
			n--;

			/* Shift the stores down, remove one store */
			rooms[k] = rooms[n + 8];
		}
	}

	/* Hack -- use the "complex" RNG */
	Rand_quick = FALSE;
}




/*
 * Town logic flow for generation of new town
 *
 * We start with a fully wiped cave of normal floors.
 *
 * Note that town_gen_hack() plays games with the R.N.G.
 *
 * This function does NOT do anything about the owners of the stores,
 * nor the contents thereof.  It only handles the physical layout.
 *
 * We place the player on the stairs at the same time we make them.
 *
 * Hack -- since the player always leaves the dungeon by the stairs,
 * he is always placed on the stairs, even if he left the dungeon via
 * word of recall or teleport level.
 */
 
 /*
 Hack -- since boundary walls are a 'good thing' for many of the algorithms 
 used, the feature FEAT_PERM_CLEAR was created.  It is used to create an 
 invisible boundary wall for town and wilderness levels, keeping the
 algorithms happy, and the players fooled.
 
 */
 
#ifdef NEW_DUNGEON
static void town_gen(struct worldpos *wpos)
#else
static void town_gen(void)
#endif
{ 
	int        i, y, x;
	cave_type *c_ptr;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif


	/* Perma-walls -- North/South*/
	for (x = 0; x < MAX_WID; x++)
	{
		/* North wall */
#ifdef NEW_DUNGEON
		c_ptr = &zcave[0][x];
#else
		c_ptr = &cave[0][0][x];
#endif

		/* Clear previous contents, add "clear" perma-wall */
		c_ptr->feat = FEAT_PERM_CLEAR;

		/* Illuminate and memorize the walls 
		c_ptr->info |= (CAVE_GLOW | CAVE_MARK);*/

		/* South wall */
#ifdef NEW_DUNGEON
		c_ptr = &zcave[MAX_HGT-1][x];
#else
		c_ptr = &cave[0][MAX_HGT-1][x];
#endif

		/* Clear previous contents, add "clear" perma-wall */
		c_ptr->feat = FEAT_PERM_CLEAR;

		/* Illuminate and memorize the walls 
		c_ptr->info |= (CAVE_GLOW);*/
	}

	/* Perma-walls -- West/East */
	for (y = 0; y < MAX_HGT; y++)
	{
		/* West wall */
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][0];
#else
		c_ptr = &cave[0][y][0];
#endif

		/* Clear previous contents, add "clear" perma-wall */
		c_ptr->feat = FEAT_PERM_CLEAR;

		/* Illuminate and memorize the walls
		c_ptr->info |= (CAVE_GLOW);*/

		/* East wall */
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][MAX_WID-1];
#else
		c_ptr = &cave[0][y][MAX_WID-1];
#endif

		/* Clear previous contents, add "clear" perma-wall */
		c_ptr->feat = FEAT_PERM_CLEAR;

		/* Illuminate and memorize the walls 
		c_ptr->info |= (CAVE_GLOW);*/
	}
	

	/* Hack -- Build the buildings/stairs (from memory) */
	town_gen_hack(wpos);


	/* Day Light */
	if ((turn % (10L * TOWN_DAWN)) < ((10L * TOWN_DAWN) / 2))
	{	
		/* Lite up the town */ 
		for (y = 0; y < MAX_HGT; y++)
		{
			for (x = 0; x < MAX_WID; x++)
			{
#ifdef NEW_DUNGEON
				c_ptr = &zcave[y][x];
#else
				c_ptr = &cave[0][y][x];
#endif

				 /* Perma-Lite */
				c_ptr->info |= CAVE_GLOW;

				 /* Memorize 
				if (view_perma_grids) c_ptr->info |= CAVE_MARK;
				*/
			}
		}

		/* This has been disabled, since the old monsters will now be saved when
		 * the server goes down. -APD
		 */
		/* Make some day-time residents 
		for (i = 0; i < MIN_M_ALLOC_TD; i++) (void)alloc_monster(0, 3, TRUE);
		*/
	}
	 /* Night Time */
	else
	{
		 /* Make some night-time residents 
		for (i = 0; i < MIN_M_ALLOC_TN; i++) (void)alloc_monster(0, 3, TRUE);
		*/
	}
}





/*
 * Allocate the space needed for a dungeon level
 */
#ifdef NEW_DUNGEON
void alloc_dungeon_level(struct worldpos *wpos)
#else
void alloc_dungeon_level(int Depth)
#endif
{
	int i;
#ifdef NEW_DUNGEON
	wilderness_type *w_ptr=&wild_info[wpos->wy][wpos->wx];
	struct dungeon_type *d_ptr;
	cave_type **zcave;
	/* Allocate the array of rows */
	C_MAKE(zcave, MAX_HGT, cave_type *);

	/* Allocate each row */
	for (i = 0; i < MAX_HGT; i++)
	{
		/* Allocate it */
		C_MAKE(zcave[i], MAX_WID, cave_type);
	}
	if(wpos->wz){
		struct dun_level *dlp;
		d_ptr=(wpos->wz>0?w_ptr->tower:w_ptr->dungeon);
		dlp=&d_ptr->level[ABS(wpos->wz)-1];
		dlp->cave=zcave;
	}
	else w_ptr->cave=zcave;
#else

	/* Allocate the array of rows */
	C_MAKE(cave[Depth], MAX_HGT, cave_type *);

	/* Allocate each row */
	for (i = 0; i < MAX_HGT; i++)
	{
		/* Allocate it */
		C_MAKE(cave[Depth][i], MAX_WID, cave_type);
	}
#endif
}

/*
 * Deallocate the space needed for a dungeon level
 */
#ifdef NEW_DUNGEON
void dealloc_dungeon_level(struct worldpos *wpos)
#else
void dealloc_dungeon_level(int Depth)
#endif
{
	int i;
#ifdef NEW_DUNGEON
	wilderness_type *w_ptr=&wild_info[wpos->wy][wpos->wx];
	cave_type **zcave;
#endif

	/* Delete any monsters on that level */
	/* Hack -- don't wipe wilderness monsters */
#ifdef NEW_DUNGEON
	if (wpos->wz) wipe_m_list(wpos);
#else
	if (Depth > 0) wipe_m_list(Depth);
#endif

	/* Delete any objects on that level */
	/* Hack -- don't wipe wilderness objects */
#ifdef NEW_DUNGEON
	if (wpos->wz) wipe_o_list(wpos);
#else
	if (Depth > 0) wipe_o_list(Depth);
#endif

#ifdef NEW_DUNGEON
	zcave=getcave(wpos);
	for (i = 0; i < MAX_HGT; i++)
	{
		/* Dealloc that row */
		C_FREE(zcave[i], MAX_WID, cave_type);
	}
	/* Deallocate the array of rows */
	C_FREE(zcave, MAX_HGT, cave_type *);
	if(wpos->wz){
		struct dun_level *dlp;
		struct dungeon_type *d_ptr;
		d_ptr=(wpos->wz>0?w_ptr->tower:w_ptr->dungeon);
		dlp=&d_ptr->level[ABS(wpos->wz)-1];
		dlp->cave=NULL;
	}
	else w_ptr->cave=NULL;
#else
	/* Free up the space taken by each row */
	for (i = 0; i < MAX_HGT; i++)
	{
		/* Dealloc that row */
		C_FREE(cave[Depth][i], MAX_WID, cave_type);
	}

	/* Deallocate the array of rows */
	C_FREE(cave[Depth], MAX_HGT, cave_type *);

	/* Set that level to "ungenerated" */
	cave[Depth] = NULL; 
#endif
}


#ifdef NEW_DUNGEON
void adddungeon(struct worldpos *wpos, int baselevel, int maxdep, int flags, bool tower){
	int i;
	wilderness_type *wild;
	struct dungeon_type *d_ptr;
	wild=&wild_info[wpos->wy][wpos->wx];
	wild->flags |= (tower ? WILD_F_UP : WILD_F_DOWN); /* no checking */
	MAKE((tower ? wild->tower : wild->dungeon), struct dungeon_type);
	d_ptr=(tower ? wild->tower : wild->dungeon);
	d_ptr->baselevel=baselevel;
	d_ptr->flags=flags; 
	d_ptr->maxdepth=maxdep;
	C_MAKE(d_ptr->level, maxdep, struct dun_level);
	for(i=0;i<maxdep;i++){
		d_ptr->level[i].ondepth=0;
	}
}
#endif


/*
 * Generates a random dungeon level			-RAK-
 *
 * Hack -- regenerate any "overflow" levels
 *
 * Hack -- allow auto-scumming via a gameplay option.
 */
 
 
#ifdef NEW_DUNGEON
void generate_cave(struct worldpos *wpos)
#else
void generate_cave(int Depth)
#endif
{
	int i, num;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	zcave=getcave(wpos);
#endif

	/* No dungeon yet */
	server_dungeon = FALSE;

	/* Generate */
	for (num = 0; TRUE; num++)
	{
		bool okay = TRUE;

		cptr why = NULL;


		/* Hack -- Reset heaps */
		/*o_max = 1;
		m_max = 1;*/

		/* Start with a blank cave */
		for (i = 0; i < MAX_HGT; i++)
		{
			/* Wipe a whole row at a time */
#ifdef NEW_DUNGEON
			C_WIPE(zcave[i], MAX_WID, cave_type);
#else
			C_WIPE(cave[Depth][i], MAX_WID, cave_type);
#endif
		}


		/* Mega-Hack -- no player yet */
		/*px = py = 0;*/


		/* Mega-Hack -- no panel yet */
		/*panel_row_min = 0;
		panel_row_max = 0;
		panel_col_min = 0;
		panel_col_max = 0;*/


#ifdef NEW_DUNGEON
		/* Reset the monster generation level */
		monster_level = getlevel(wpos);

		/* Reset the object generation level */
		object_level = getlevel(wpos);
#else
		/* Reset the monster generation level */
		monster_level = Depth;

		/* Reset the object generation level */
		object_level = Depth;
#endif

		/* Nothing special here yet */
		good_item_flag = FALSE;

		/* Nothing good here yet */
		rating = 0;


		/* Build the town */
#ifdef NEW_DUNGEON
		if (istown(wpos))
#else
		if (!Depth)
#endif
		{
			/* Small town */
			/*cur_hgt = SCREEN_HGT;
			cur_wid = SCREEN_WID;*/

			/* Determine number of panels */
			/*max_panel_rows = (cur_hgt / SCREEN_HGT) * 2 - 2;
			max_panel_cols = (cur_wid / SCREEN_WID) * 2 - 2;*/

			/* Assume illegal panel */
			/*panel_row = max_panel_rows;
			panel_col = max_panel_cols;*/

#ifdef NEW_DUNGEON
			if(wpos->wx==MAX_WILD_X/2 && wpos->wy==MAX_WILD_Y/2 && !wpos->wz){
				/* town of angband */
				adddungeon(wpos, 1, 200, DUNGEON_RANDOM, FALSE);
			}
#endif
			/* Make a town */
#ifdef NEW_DUNGEON
			town_gen(wpos);
			setup_objects();
			setup_monsters();
#else
			town_gen();
#endif
		}

		/* Build wilderness */
#ifdef NEW_DUNGEON
		else if (!wpos->wz)
		{
			wilderness_gen(wpos);		
		}
#else
		else if (Depth < 0)
		{
			wilderness_gen(Depth);		
		}
#endif

		/* Build a real level */
		else
		{
			/* Big dungeon */
			/*cur_hgt = MAX_HGT;
			cur_wid = MAX_WID;*/

			/* Determine number of panels */
			/*max_panel_rows = (cur_hgt / SCREEN_HGT) * 2 - 2;
			max_panel_cols = (cur_wid / SCREEN_WID) * 2 - 2;*/

			/* Assume illegal panel */
			/*panel_row = max_panel_rows;
			panel_col = max_panel_cols;*/

			/* Make a dungeon */
#ifdef NEW_DUNGEON
			cave_gen(wpos);
#else
			cave_gen(Depth);
#endif
		}


		/* Extract the feeling */
		if (rating > 100) feeling = 2;
		else if (rating > 80) feeling = 3;
		else if (rating > 60) feeling = 4;
		else if (rating > 40) feeling = 5;
		else if (rating > 30) feeling = 6;
		else if (rating > 20) feeling = 7;
		else if (rating > 10) feeling = 8;
		else if (rating > 0) feeling = 9;
		else feeling = 10;

		/* Hack -- Have a special feeling sometimes */
		if (good_item_flag) feeling = 1;

		/* It takes 1000 game turns for "feelings" to recharge */
		if ((turn - old_turn) < 1000) feeling = 0;

		/* Hack -- no feeling in the town */
#ifdef NEW_DUNGEON
		if(istown(wpos)) feeling=0;
#else
		if (!Depth) feeling = 0;
#endif


		/* Prevent object over-flow */
		if (o_max >= MAX_O_IDX)
		{
			/* Message */
			why = "too many objects";

			/* Message */
			okay = FALSE;
		}

		/* Prevent monster over-flow */
		if (m_max >= MAX_M_IDX)
		{
			/* Message */
			why = "too many monsters";

			/* Message */
			okay = FALSE;
		}

		/* Mega-Hack -- "auto-scum" */
		if (auto_scum && (num < 100))
		{
			/* Require "goodness" */
			if ((feeling > 9) ||
#ifdef NEW_DUNGEON
			    ((getlevel(wpos) >= 5) && (feeling > 8)) ||
			    ((getlevel(wpos) >= 10) && (feeling > 7)) ||
			    ((getlevel(wpos) >= 20) && (feeling > 6)) ||
			    ((getlevel(wpos) >= 40) && (feeling > 5)))
#else
			    ((Depth >= 5) && (feeling > 8)) ||
			    ((Depth >= 10) && (feeling > 7)) ||
			    ((Depth >= 20) && (feeling > 6)) ||
			    ((Depth >= 40) && (feeling > 5)))
#endif
			{
				/* Give message to cheaters */
				if (cheat_room || cheat_hear ||
				    cheat_peek || cheat_xtra)
				{
					/* Message */
					why = "boring level";
				}

				/* Try again */
				okay = FALSE;
			}
		}

		/* Accept */
		if (okay) break;


		/* Message */
		if (why) s_printf("Generation restarted (%s)\n", why);

#ifdef NEW_DUNGEON
		/* Wipe the objects */
		wipe_o_list(wpos);

		/* Wipe the monsters */
		wipe_m_list(wpos);
#else
		/* Wipe the objects */
		wipe_o_list(Depth);

		/* Wipe the monsters */
		wipe_m_list(Depth);
#endif

		/* Compact some objects, if necessary */
		if (o_max >= MAX_O_IDX * 3 / 4)
			compact_objects(32);

		/* Compact some monsters, if necessary */
		if (m_max >= MAX_M_IDX * 3 / 4)
			compact_monsters(32);
	}


	/* Dungeon level ready */
	server_dungeon = TRUE;

	/* Remember when this level was "created" */
	old_turn = turn;
}



