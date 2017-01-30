/* $Id$ */
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

// Needed for gettimeofday()
#include <sys/time.h>

/* Avoid generating doors that are standing around in weird/pointless locations (eg without walls attached) -- todo: fix/complete */
//#define ENABLE_DOOR_CHECK

/* Don't generate non-vault rooms over already generated vault grids, partially destroying them.
   That would not only create 'open' vaults but also leave CAVE_STCK no-tele grids inside the now
   plain looking rooms if there was previously part of a no-teleport vault generated at its position. */
#define UNBROKEN_VAULTS //todo: implement =P

static void vault_monsters(struct worldpos *wpos, int y1, int x1, int num);
static void town_gen_hack(struct worldpos *wpos);
#ifdef ENABLE_DOOR_CHECK
static bool door_makes_no_sense(cave_type **zcave, int x, int y);
#endif

struct stairs_list {
	int x, y;
};

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
#define DUN_UNUSUAL	(cfg.dun_unusual) /* Level/chance of unusual room */
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
#define DUN_STR_SC  10	/* 1/chance of treasure per sandwall */
#define DUN_STR_WLW     1	/* Width of lava & water streamers -KMW- */
#define DUN_STR_DWLW    8	/* Density of water & lava streams -KMW- */

/*
 * Dungeon treausre allocation values
 */
#define DUN_AMT_ROOM	9	/* Amount of objects for rooms */
#define DUN_AMT_ITEM	3	/* Amount of objects for rooms/corridors */
#define DUN_AMT_GOLD	3	/* Amount of treasure for rooms/corridors */
/* #define DUN_AMT_ALTAR   1 */	/* Amount of altars */
#define DUN_AMT_BETWEEN 2	/* Amount of between gates */
#define DUN_AMT_FOUNTAIN 1	/* Amount of fountains */

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
#define ALLOC_TYP_ALTAR		6	/* Altar */
#define ALLOC_TYP_BETWEEN	7	/* Between */
#define ALLOC_TYP_FOUNTAIN	8	/* Fountain */
#define ALLOC_TYP_FOUNTAIN_OF_BLOOD	9	/* Fountain of Blood */



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
#define ROOM_MAX	13

/*
 * Maxumal 'depth' that has extra stairs;
 * also, those floors will be w/o trapped doors.
 */
#define COMFORT_PASSAGE_DEPTH 5

/*
 * ((level+TRAPPED_DOOR_BASE)/TRAPPED_DOOR_RATE) chance of generation
 */
#define TRAPPED_DOOR_RATE	400
#define TRAPPED_DOOR_BASE	30

/*
 * Below this are makeshift implementations of extra dungeon features;
 * TODO: replace them by dungeon_types etc		- Jir -
 */
/*
 * makeshift river for 3.3.x
 * pfft, still alive for 4.0.0...
 * [15,7,4,6,45,4,19,100,  1000,50]
 *
 * A player is supposed to cross 2 watery belts without avoiding them
 * by scumming;  the first by swimming, the second by levitation parhaps.
 */
#define DUN_RIVER_CHANCE	15	/* The deeper, the less watery area. */
#define DUN_RIVER_REDUCE	7	/* DUN_RIVER_CHANCE / 2, maximum. */
#define DUN_STR_WAT			4
#define DUN_LAKE_TRY		6	/* how many tries to generate lake on river */
#define WATERY_CYCLE		45	/* defines 'watery belt' */
#define WATERY_RANGE		4	/* (45,4,19) = '1100-1250, 3350-3500,.. ' */
#define WATERY_OFFSET		19
#define WATERY_BELT_CHANCE	100 /* chance of river generated on watery belt */

#define DUN_MAZE_FACTOR		1000	/* depth/DUN_MAZE_FACTOR chance of maze */
#define DUN_MAZE_PERMAWALL	20	/* % of maze built of permawall */


/*
 * Those flags are mainly for vaults, quests and non-Angband dungeons...
 * but none of them implemented, qu'a faire?
 */
#define NO_MAGIC_CHANCE		4
#define NO_GENO_CHANCE		4
#define NO_MAP_CHANCE		4
#define NO_MAGIC_MAP_CHANCE	4
#define NO_DESTROY_CHANCE	4

/*
 * Chances of a vault creating some special effects on the level, in %.
 * FORCE_FLAG bypasses this check.	[50]
 */
#define VAULT_FLAG_CHANCE	30

/*
 * Chance of 'HIVES' type vaults reproducing themselves, in %. [60]
 */
#define HIVE_CHANCE(lev)	(lev > 120 ? 60 : lev / 3 + 20)

/*
 * Borrowed stuffs from ToME
 */
#define DUN_CAVERN     30	/* chance/depth of having a cavern level */
#define DUN_CAVERN2    20	/* 1/chance extra check for cavern level */
#define EMPTY_LEVEL    15	/* 1/chance of being 'empty' (15)*/
#define DARK_EMPTY      5	/* 1/chance of arena level NOT being lit (2)*/
#define SMALL_LEVEL     4	/* 1/chance of smaller size (3)*/
#define DUN_WAT_RNG     2	/* Width of rivers */
#define DUN_WAT_CHG    50	/* 1 in 50 chance of junction in river */

#define DUN_SANDWALL   10   /* percentage for Sandwall being generated [10] */

/* specify behaviour/possibility of vaults/rooms in 'maze' levels */
#define VAULTS_OVERRIDE_MAZE	/* make vault walls override maze emptiness. otherwise, mazes can 'unwall' vaults! */

/* Prevent staircases from being generated on inner/outer ring of pits and nests? */
//#define NEST_PIT_NO_STAIRS_INNER
#define NEST_PIT_NO_STAIRS_OUTER	/* implies NEST_PIT_NO_STAIRS_INNER */

/* Ironman Deep Dive Challenge levels should always be big, unless the dungeon theme dictates otherwise? */
#define IRONDEEPDIVE_BIG_IF_POSSIBLE
/* Ironman Deep Dive Challenge levels of SMALL or SMALLEST theme should actually be enlarged for better exp gain? */
#define IRONDEEPDIVE_EXPAND_SMALL
/* Ironman Deep Dive Challenge levels have less chance to generate vaults */
#define IDDC_FEWER_VAULTS
/* Final guardians are guaranteed spawns instead of rolling against their r-rarity? */
#define IDDC_GUARANTEED_FG

/* experimental - this might be too costly on cpu: add volcanic floor around lava rivers - C. Blue */
#define VOLCANIC_FLOOR


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

	bool watery;	/* this should be included in dun_level struct. */
	bool no_penetr;

	dun_level *l_ptr;
	int ratio;
#if 0
	byte max_hgt;			/* Vault height */
	byte max_wid;			/* Vault width */
#endif /*0 */
};


/*
 * Dungeon generation data -- see "cave_gen()"
 */
static dun_data *dun;

/*
 * Dungeon graphics info
 * Why the first two are byte and the rest s16b???
 */
/* ToME variables -- not implemented, but needed to have it work */
static byte feat_wall_outer = FEAT_WALL_OUTER;	/* Outer wall of rooms */
static byte feat_wall_inner = FEAT_WALL_INNER;	/* Inner wall of rooms */
s16b floor_type[1000];	/* Dungeon floor */
s16b fill_type[1000];	/* Dungeon filler */

static bool good_item_flag;		/* True if "Artifact" on this level */

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
	{ -1, 2, -2, 3, 10 },		/* 8 = Greater vault (66x44) */

	{ 0, 1, 0, 1, 1 },		/* 9 = Circular rooms (22x22) */
	{ 0, 1,	-1, 1, 3 },		/* 10 = Fractal cave (42x24) */
	{ 0, 1, -1, 1, 10 },	/* 11 = Random vault (44x22) */
	{ 0, 1, 0, 1, 10 },	/* 12 = Crypts (22x22) */
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

#ifdef ARCADE_SERVER
extern void arcade_wipe(worldpos *wpos)
{
	cave_type **zcave;
//	cave_type *c_ptr;
	if (!(zcave = getcave(wpos))) return;
	int my, mx;
	for (mx = 1; mx < 131; mx++) {
		for (my = 1; my < 43; my++) {
			cave_set_feat(wpos, my, mx, 1);
		}
	}
	return;
}
#endif

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
static void new_player_spot(struct worldpos *wpos)
{
	int        y, x, tries = 5000;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Place the player */
	while (TRUE)
	{
		/* Pick a legal spot */
		y = rand_range(1, dun->l_ptr->hgt - 2);
		x = rand_range(1, dun->l_ptr->wid - 2);
		
		/* prevent infinite loop (not sure about exact circumstances yet) */
		if (!--tries) break;

		/* Must be a "naked" floor grid */
		if (!cave_naked_bold(zcave, y, x)) continue;

		/* Refuse to start on anti-teleport grids */
		if (zcave[y][x].info & CAVE_ICKY) continue;

		/* Done */
		break;
	}
	
	/* emergency procedure: avoid no-tele vaults */
	if ((!tries) && (zcave[y][x].info & CAVE_STCK)) {
		for (x = 1; x < dun->l_ptr->wid - 2; x++)
		for (y = 1; y < dun->l_ptr->hgt - 2; y++) {
			if (!(zcave[y][x].info & CAVE_STCK)) break;
		}
		/* This shouldn't happen: Whole level is filled with CAVE_STCK spots, exclusively! */
		y = rand_range(1, dun->l_ptr->hgt - 2);
		x = rand_range(1, dun->l_ptr->wid - 2);
	}

	/* Save the new grid */
	new_level_rand_y(wpos, y);
	new_level_rand_x(wpos, x);
}



/*
 * Count the number of walls adjacent to the given grid.
 *
 * Note -- Assumes "in_bounds(y, x)"
 *
 * We count only granite walls and permanent walls.
 */
static int next_to_walls(struct worldpos *wpos, int y, int x)
{
	int        k = 0;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return(FALSE);

	if (f_info[zcave[y+1][x].feat].flags1 & FF1_WALL) k++;
	if (f_info[zcave[y-1][x].feat].flags1 & FF1_WALL) k++;
	if (f_info[zcave[y][x+1].feat].flags1 & FF1_WALL) k++;
	if (f_info[zcave[y][x-1].feat].flags1 & FF1_WALL) k++;
	return (k);
}


/* For new place_rubble() that can place a double-rubble - C. Blue
   Returns: dir of hallway wall or 0 if not in hallway or if in
            a corner of a hallway (ie in any spot that doesn't
            feature exactly 3 wall grids in a row, rest floor grids. */
static int along_hallway(cave_type **zcave, int x, int y) {
	int pos, d, x1, y1, walls;

	/* Assume 4 possible directions of hallway walls:
	   horizontally below us, vertically right of us, horizontally above us, vertically left of us. */
	for (pos = 0; pos <= 7; pos += 2) {
		/* test for 3 wall grids + 5 floor grids */
		walls = 0;

		/* begin with 3 walls */
		for (d = 0; d <= 2; d++) {
			/* Extract the location */
			x1 = x + ddx_cyc[(pos + d) % 8];
			y1 = y + ddy_cyc[(pos + d) % 8];

			/* Require non-floor */
			if (cave_floor_bold(zcave, y1, x1)) continue;
			if (zcave[y1][x1].feat == FEAT_RUBBLE) continue; /* exempt actual rubble */
			walls++;
		}
		/* test for '3 walls grids' already failed? */
		if (walls != 3) continue;

		/* test for 5 floor grids now */
		for (d = 3; d <= 7; d++) {
			/* Extract the location */
			x1 = x + ddx_cyc[(pos + d) % 8];
			y1 = y + ddy_cyc[(pos + d) % 8];

			/* Require floor */
			if (cave_floor_bold(zcave, y1, x1)) continue;
			if (zcave[y1][x1].feat == FEAT_RUBBLE) continue; /* exempt actual rubble */
			walls++;
		}
		/* failed to find 5 floor grids? */
		if (walls != 3) continue;

		/* success */
		return (pos + 1);
	}
	return 0;
}

/*
 * Convert existing terrain type to rubble
 */
static void place_rubble(struct worldpos *wpos, int y, int x)
{
	cave_type **zcave;
	cave_type *c_ptr;
	int dir;
	if(!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[y][x];

	/* Create rubble */
	c_ptr->feat = FEAT_RUBBLE;

	/* new: if it's inside a hallway, have a chance for a second rubble
	        to be generated to completely block the way.
	   Note: currently does not affect dungeon-specific rubble tiles (Mines of Moria). */
	if (!rand_int(4)) return;
	if (!(c_ptr->info & (CAVE_ROOM | CAVE_ICKY))
	    && (dir = along_hallway(zcave, x, y))) {
		/* place another rubble in the opposite dir */
		x = x + ddx_cyc[ddi_cyc[dir]];
		y = y + ddy_cyc[ddi_cyc[dir]];
		/* check that potential new rubble grid also passes the hallway test,
		   and therefore is located at the opposite wall of our hallway */
		if (ddi_cyc[dir] == along_hallway(zcave, x, y)) {
			/* double check that it's *clean* floor */
			if (cave_naked_bold(zcave, y, x)) {
				/* place a second rubble pile */
				zcave[y][x].feat = FEAT_RUBBLE;
			}
		}
	}
}

/*
 * Create a fountain here.
 */
void place_fountain(struct worldpos *wpos, int y, int x) {
	cave_type **zcave;
	cave_type *c_ptr;
	int dun_lev;
	c_special *cs_ptr;
	int svals[SV_POTION_LAST + SV_POTION2_LAST + 1], maxsval = 0, k;

	if(!(zcave = getcave(wpos))) return;
	dun_lev = getlevel(wpos);
	c_ptr = &zcave[y][x];


	/* No fountains over traps/house doors etc */
	if (c_ptr->special) return;

	/* Place the fountain */
	if (randint(100) < 20) { /* 30 */
		/* XXX Empty fountain doesn't need 'type', does it? */
		cave_set_feat(wpos, y, x, FEAT_EMPTY_FOUNTAIN);
/*		c_ptr->special2 = 0; */
		return;
	}

	/* List of usable svals */
	for (k = 1; k < max_k_idx; k++) {
		object_kind *k_ptr = &k_info[k];

		if (((k_ptr->tval == TV_POTION) || (k_ptr->tval == TV_POTION2)) &&
		    (k_ptr->level <= dun_lev) && (k_ptr->flags4 & TR4_FOUNTAIN)) {
			/* C. Blue -- reduce amount of stat-draining fountains before sustain rings become available */
			if (!((k_ptr->tval == TV_POTION) && (
			    ((dun_lev < 8) && /* no stat-draining fountains above -400ft (finding sustain-rings there) */
			    (k_ptr->sval == SV_POTION_DEC_STR || k_ptr->sval == SV_POTION_DEC_INT ||
			    k_ptr->sval == SV_POTION_DEC_WIS || k_ptr->sval == SV_POTION_DEC_DEX ||
			    k_ptr->sval == SV_POTION_DEC_CON)) || /* allow CHR drain */
			    ((dun_lev < 20) && /* no exp-draining fountains before stat-fountains appear */
			    ((k_ptr->sval == SV_POTION_LOSE_MEMORIES) ||
			    ((!k_ptr->cost) && magik(50)))) /* bad fountains 50% less probable before stat-fountains appear */
			    )) &&
#ifndef EXPAND_TV_POTION
			    !((k_ptr->tval == TV_POTION2) && (dun_lev < 20) &&
			    (k_ptr->sval == SV_POTION2_CHAUVE_SOURIS))
#else
			    !((k_ptr->tval == TV_POTION) && (dun_lev < 20) &&
			    (k_ptr->sval == SV_POTION_CHAUVE_SOURIS))
#endif
			    )
			{
				if (k_ptr->tval == TV_POTION2) svals[maxsval] = k_ptr->sval + SV_POTION_LAST;
				else svals[maxsval] = k_ptr->sval;
				maxsval++;
			}
		}
	}

	if (maxsval == 0) return;
	else {
		/* TODO: rarity should be counted in? */
		cave_set_feat(wpos, y, x, FEAT_FOUNTAIN);
		if (!(cs_ptr = AddCS(c_ptr, CS_FOUNTAIN))) return;

		if (!maxsval || magik(20)) /* often water */
			cs_ptr->sc.fountain.type = SV_POTION_WATER;
		else cs_ptr->sc.fountain.type = svals[rand_int(maxsval)];

		cs_ptr->sc.fountain.rest = damroll(1, 5);

		/* Some hacks: */
		if (cs_ptr->sc.fountain.type <= SV_POTION_LAST)
			switch (cs_ptr->sc.fountain.type) {
/*			case SV_POTION_NEW_LIFE:
				cs_ptr->sc.fountain.rest = 1;
				break;
*/			case SV_POTION_INC_STR:	case SV_POTION_INC_INT:
			case SV_POTION_INC_WIS:	case SV_POTION_INC_DEX:
			case SV_POTION_INC_CON:	case SV_POTION_INC_CHR:
			case SV_POTION_STAR_RESTORE_MANA:
				cs_ptr->sc.fountain.rest = damroll(1, 2);
				break;
			case SV_POTION_STAR_HEALING:
			case SV_POTION_SELF_KNOWLEDGE:
				cs_ptr->sc.fountain.rest = damroll(1, 3);
				break;
			case SV_POTION_STAR_ENLIGHTENMENT:
			case SV_POTION_LIFE:
			case SV_POTION_AUGMENTATION:
			case SV_POTION_EXPERIENCE:
			case SV_POTION_INVULNERABILITY:
				cs_ptr->sc.fountain.rest = 1;
				break;
#ifdef EXPAND_TV_POTION
			case SV_POTION2_CHAUVE_SOURIS:
			case SV_POTION2_CURE_SANITY:
				cs_ptr->sc.fountain.rest = damroll(1, 2);
				break;
			case SV_POTION_LEARNING:
				cs_ptr->sc.fountain.rest = 1;//disabled anyway
				break;
#endif
			}
		else
			switch (cs_ptr->sc.fountain.type - SV_POTION_LAST) {
			/* make it hard to polymorph back at a bat fountain by sipping again */
			case SV_POTION2_CHAUVE_SOURIS:
			case SV_POTION2_CURE_SANITY:
				cs_ptr->sc.fountain.rest = damroll(1, 2);
				break;
			case SV_POTION2_LEARNING:
				cs_ptr->sc.fountain.rest = 1;//disabled anyway
				break;
			}

		cs_ptr->sc.fountain.known = FALSE;
#if 0
		cs_ptr->type = CS_TRAPS;
		c_ptr->special2 = damroll(3, 4);
#endif	/* 0 */
	}

/*	c_ptr->special = svals[rand_int(maxsval)]; */
}

/* Place a fountain of blood, for Vampires, as suggested by Mark -  C. Blue */
void place_fountain_of_blood(struct worldpos *wpos, int y, int x) {
	cave_type **zcave;
	cave_type *c_ptr;
	c_special *cs_ptr;

	if(!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[y][x];

	/* No fountains over traps/house doors etc */
	if (c_ptr->special) return;

	/* TODO: rarity should be counted in? */
	cave_set_feat(wpos, y, x, FEAT_FOUNTAIN_BLOOD);
	if (!(cs_ptr = AddCS(c_ptr, CS_FOUNTAIN))) return;
	cs_ptr->sc.fountain.type = SV_POTION_BLOOD;
	cs_ptr->sc.fountain.rest = damroll(1, 5);
	cs_ptr->sc.fountain.known = TRUE;
}

#if 0
/*
 * Place an altar at the given location
 */
static void place_altar(int y, int x)
{
        if (magik(10))
                cave_set_feat(y, x, 164);
}
#endif	/* 0 */


/*
 * Place a between gate at the two specified locations
 */
bool place_between_targetted(struct worldpos *wpos, int y, int x, int ty, int tx) {
	cave_type **zcave;
	cave_type *c_ptr, *c1_ptr;
	c_special *cs_ptr;

	if (!(zcave = getcave(wpos))) return FALSE;

	/* Require "naked" floor grid */
	if (!cave_clean_bold(zcave, y, x)) return FALSE;
	if (!cave_naked_bold(zcave, ty, tx)) return FALSE;

	/* No between-gate over traps/house doors etc */
	c_ptr = &zcave[y][x];
	if (c_ptr->special) return FALSE;

	/* Access the target grid */
	c1_ptr = &zcave[ty][tx];
	if (c1_ptr->special) return FALSE;

	if (!(cs_ptr = AddCS(c_ptr, CS_BETWEEN))) return FALSE;
	cave_set_feat(wpos, y, x, FEAT_BETWEEN);
	cs_ptr->sc.between.fy = ty;
	cs_ptr->sc.between.fx = tx;

	/* XXX not 'between' gate can be generated */
	if (!(cs_ptr = AddCS(c1_ptr, CS_BETWEEN))) return FALSE;
	cave_set_feat(wpos, ty, tx, FEAT_BETWEEN);
	cs_ptr->sc.between.fy = y;
	cs_ptr->sc.between.fx = x;

	return TRUE;
}

/*
 * Place a between gate at the given location
 */
static void place_between(struct worldpos *wpos, int y, int x) {
	int gx, gy, tries = 1000;
	while (TRUE) {
		gy = rand_int(dun->l_ptr->hgt);
		gx = rand_int(dun->l_ptr->wid);
		if (place_between_targetted(wpos, y, x, gy, gx)) break;

		if (tries-- == 0) return;
	}
}
/* external variant for use from elsewhere */
void place_between_ext(struct worldpos *wpos, int y, int x, int hgt, int wid) {
	int gx, gy, tries = 1000;
	while (TRUE) {
		gy = rand_int(hgt);
		gx = rand_int(wid);
		if (place_between_targetted(wpos, y, x, gy, gx)) break;

		if (tries-- == 0) return;
	}
}

/* For divine_gateway() - cool visuals only */
bool place_between_dummy(struct worldpos *wpos, int y, int x) {
	cave_type **zcave;
	cave_type *c_ptr;

	if (!(zcave = getcave(wpos))) return FALSE;

	/* Require "naked" floor grid */
	if (!cave_clean_bold(zcave, y, x)) return FALSE;

	/* No between-gate over traps/house doors etc */
	c_ptr = &zcave[y][x];
	if (c_ptr->special) return FALSE;

	cave_set_feat(wpos, y, x, FEAT_BETWEEN_TEMP);

	return TRUE;
}

/*
 * Convert existing terrain type to "up stairs"
 */
extern void place_up_stairs(struct worldpos *wpos, int y, int x) {
	cave_type **zcave;
	cave_type *c_ptr;
	if (!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[y][x];

	/* Create up stairs */
	if (can_go_up(wpos, 0x1)) {
		c_ptr->feat = FEAT_LESS;
		aquatic_terrain_hack(zcave, x, y);
	}
}


/*
 * Convert existing terrain type to "down stairs"
 */
static void place_down_stairs(worldpos *wpos, int y, int x) {
	cave_type **zcave;
	cave_type *c_ptr;
	if (!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[y][x];

	/* Create down stairs */
	if (can_go_down(wpos, 0x1)) {
		c_ptr->feat = FEAT_MORE;
		aquatic_terrain_hack(zcave, x, y);
	}
}


/*
 * Place an up/down staircase at given location
 */
static void place_random_stairs(struct worldpos *wpos, int y, int x) {
	cave_type **zcave;

    /* no staircase spam anyway, one is sufficient */
//#ifdef NETHERREALM_BOTTOM_RESTRICT
	if (netherrealm_bottom(wpos)) return;
//#endif

	if (!(zcave = getcave(wpos))) return;

	if (!cave_clean_bold(zcave, y, x)) return;

	if (!can_go_down(wpos, 0x1) && !can_go_up(wpos, 0x1)){
		/* special or what? */
	} else if(!can_go_down(wpos, 0x1) && can_go_up(wpos, 0x1)){
		place_up_stairs(wpos, y, x);
	} else if(!can_go_up(wpos, 0x1) && can_go_down(wpos, 0x1)){
		place_down_stairs(wpos, y, x);
	} else if (rand_int(100) < 50) {
		place_down_stairs(wpos, y, x);
	} else {
		place_up_stairs(wpos, y, x);
	}
}


/*
 * Place a locked door at the given location
 */
static void place_locked_door(struct worldpos *wpos, int y, int x) {
	int tmp;
	cave_type **zcave;
	cave_type *c_ptr;
	if (!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[y][x];

#ifdef ENABLE_DOOR_CHECK
	if (door_makes_no_sense(zcave, x, y)) return;
#endif

	/* Create locked door */
	c_ptr->feat = FEAT_DOOR_HEAD + randint(7);
	aquatic_terrain_hack(zcave, x, y);

	/* let's trap this ;) */
	if ((tmp = getlevel(wpos)) <= COMFORT_PASSAGE_DEPTH ||
		rand_int(TRAPPED_DOOR_RATE) > tmp + TRAPPED_DOOR_BASE) return;
	place_trap(wpos, y, x, 0);
}


/*
 * Place a secret door at the given location
 */
static void place_secret_door(struct worldpos *wpos, int y, int x) {
	int tmp;
	cave_type **zcave;
	cave_type *c_ptr;
	struct c_special *cs_ptr;
	if (!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[y][x];

#ifdef ENABLE_DOOR_CHECK
	if (door_makes_no_sense(zcave, x, y)) return;
#endif

	if ((cs_ptr = AddCS(c_ptr, CS_MIMIC))) {
		/* Vaults */
		if (c_ptr->info & CAVE_ICKY)
			cs_ptr->sc.omni = FEAT_WALL_INNER;

		/* Ordinary room -- use current outer or inner wall */
		else if (c_ptr->info & CAVE_ROOM) {
			/* Determine if it's inner or outer XXX XXX XXX */
			if ((zcave[y - 1][x].info & CAVE_ROOM) &&
			    (zcave[y + 1][x].info & CAVE_ROOM) &&
			    (zcave[y][x - 1].info & CAVE_ROOM) &&
			    (zcave[y][x + 1].info & CAVE_ROOM))
			{
				cs_ptr->sc.omni = feat_wall_inner;
			} else {
				cs_ptr->sc.omni = feat_wall_outer;
			}
		} else {
			cs_ptr->sc.omni = fill_type[rand_int(1000)];
		}
	}

	/* Create secret door */
	c_ptr->feat = FEAT_SECRET;
	aquatic_terrain_hack(zcave, x, y);

	/* let's trap this ;) */
	if ((tmp = getlevel(wpos)) <= COMFORT_PASSAGE_DEPTH ||
		rand_int(TRAPPED_DOOR_RATE) > tmp + TRAPPED_DOOR_BASE) return;
	place_trap(wpos, y, x, 0);
}


/*
 * Place a random type of door at the given location
 */
static void place_random_door(struct worldpos *wpos, int y, int x) {
	int tmp;
	cave_type **zcave;
	cave_type *c_ptr;
	if (!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[y][x];

#ifdef ENABLE_DOOR_CHECK
	if (door_makes_no_sense(zcave, x, y)) return;
#endif

#if 0 /* fixed the build_tunnel() code, should make this hack obsolete */
	/* hack: after adding dungeon-custom boundary feats, we had a panic in
	   Mordor, where the 2nd door of a pair of doors was created inside the
	   upper level side high mountain boundary.
	   TODO instead of this: look for perm wall checks (eg FEAT_PERM_SOLID)
	   that don't recognize the new boundary feats. */
	if ((f_info[zcave[y][x].feat].flags2 & FF2_BOUNDARY)) return;
#endif

	/* Choose an object */
	tmp = rand_int(1000);

	/* Open doors (300/1000) */
	if (tmp < 300) {
		/* Create open door */
		c_ptr->feat = FEAT_OPEN;
	}

	/* Broken doors (100/1000) */
	else if (tmp < 400) {
		/* Create broken door */
		c_ptr->feat = FEAT_BROKEN;
	}

	/* Secret doors (200/1000) */
	else if (tmp < 600) {
		/* Create secret door */
		c_ptr->feat = FEAT_SECRET;
	}

	/* Closed doors (300/1000) */
	else if (tmp < 900) {
		/* Create closed door */
		c_ptr->feat = FEAT_DOOR_HEAD + 0x00;
	}

	/* Locked doors (99/1000) */
	else if (tmp < 999) {
		/* Create locked door */
		c_ptr->feat = FEAT_DOOR_HEAD + randint(7);
	}

	/* Stuck doors (1/1000) */
	else {
		/* Create jammed door */
		c_ptr->feat = FEAT_DOOR_HEAD + 0x08 + rand_int(8);
	}

	aquatic_terrain_hack(zcave, x, y);

	/* let's trap this ;) */
	if ((tmp = getlevel(wpos)) <= COMFORT_PASSAGE_DEPTH ||
	    rand_int(TRAPPED_DOOR_RATE) > tmp + TRAPPED_DOOR_BASE) return;
	place_trap(wpos, y, x, 0);
}



/*
 * Places some staircases near walls
 */
static void alloc_stairs(struct worldpos *wpos, int feat, int num, int walls, struct stairs_list *stairs) {
	int y = 5, x = 5, i, j, flag, tries = 0; /* initialise x,y to kill compiler warnings */
	int emergency_flag = TRUE, new_feat = -1, nlev_down = FALSE, nlev_up = FALSE;
	int starty, startx;
	cave_type *c_ptr;
	cave_type **zcave;
	struct stairs_list *sptr = stairs;
	bool stairs_store = FALSE, stairs_check = FALSE;

#ifdef NETHERREALM_BOTTOM_RESTRICT
	if (netherrealm_bottom(wpos)) return;
#endif

	if (!(zcave = getcave(wpos))) return;

	/* for Nether Realm: store stairs to 'stairs' array, or check them against it? */
	if (stairs) {
		if (wpos->wz < 1) {
			if (feat == FEAT_MORE || feat == FEAT_WAY_MORE) stairs_store = TRUE;
			else stairs_check = TRUE;
		} else if (feat == FEAT_LESS || feat == FEAT_WAY_LESS) stairs_store = TRUE;
		else stairs_check = TRUE;

		/* advance to first free element */
		if (stairs_store) {
			while (sptr->x != -1 && sptr->y != -1) sptr++;
			/* out of available array entries? */
			if (sptr->y == -1) {
//s_printf("-no more stairs-\n");
				stairs_store = FALSE;
			}
		}
#if 0 //DEBUG CODE
if (stairs_store) s_printf("stairs_store\n");
if (stairs_check) {
sptr = stairs;
s_printf("stairs_check\n");
while (sptr->x != -1 && sptr->y != -1) {
s_printf("  stairs x,y = %d,%d\n", sptr->x, sptr->y);
sptr++;
}
}
#endif
	}

	/* Town -- must go down */
	if (!can_go_up_simple(wpos)) {
		/* Clear previous contents, add down stairs */
		if (can_go_down(wpos, 0x1)) new_feat = FEAT_MORE;
		if (!istown(wpos)) nlev_up = TRUE;
	}
	/* Quest -- must go up */
	else if (is_xorder(wpos) || !can_go_down_simple(wpos)) {
		/* Clear previous contents, add up stairs */
		if (can_go_up(wpos, 0x1)) new_feat = FEAT_LESS;
		/* Set this to be the starting location for people going down */
		nlev_down = TRUE;
	}
	/* Requested type */
	else {
		/* Clear previous contents, add stairs */
		if ((can_go_up(wpos, 0x1) && (feat == FEAT_LESS || feat == FEAT_WAY_LESS)) ||
		    (can_go_down(wpos, 0x1) && (feat == FEAT_MORE || feat == FEAT_WAY_MORE)))
			new_feat = feat;

		if (feat == FEAT_LESS || feat == FEAT_WAY_LESS) {
			/* Set this to be the starting location for people going down */
			nlev_down = TRUE;
		}
		if (feat == FEAT_MORE || feat == FEAT_WAY_MORE) {
			/* Set this to be the starting location for people going up */
			nlev_up = TRUE;
		}
	}

	/* Place "num" stairs */
	for (i = 0; i < num; i++) {
		tries = 1000; /* don't hang again. extremely rare though. wonder what the situation was like. */

		/* Place some stairs */
		for (flag = FALSE; !flag && tries; tries--) {
			/* Try several times, then decrease "walls" */
			for (j = 0; !flag && j <= 3000; j++) {
				/* Pick a random grid */
				y = 1 + rand_int(dun->l_ptr->hgt - 2);
				x = 1 + rand_int(dun->l_ptr->wid - 2);

				/* Require "naked" floor grid */
				if (!cave_naked_bold(zcave, y, x)) continue;
				
				/* No stairs that lead into nests/pits */
				if (zcave[y][x].info & CAVE_NEST_PIT) continue;

				/* Require a certain number of adjacent walls */
				if (next_to_walls(wpos, y, x) < walls) continue;

				if (stairs_check) {
					bool bad = FALSE;

					sptr = stairs;
					while (sptr->x != -1 && sptr->y != -1 && j < 3000) {
						if (distance(sptr->y, sptr->x, y, x) < 100 - j / 75) { /* assume max-sized map (Nether Realm) */
							bad = TRUE;
							break;
						}
						sptr++;
					}
					if (bad) continue;
//if (j == 3000) s_printf(" sptr full fail\n");
//else s_printf(" sptr OK %d,%d\n", x,y);
				}

				/* Access the grid */
				c_ptr = &zcave[y][x];

				if (new_feat != -1) {
					c_ptr->feat = new_feat;
					aquatic_terrain_hack(zcave, x, y);

					if (stairs_store) {
						sptr->x = x;
						sptr->y = y;
						sptr++;
						/* no more array space? */
						if (sptr->y == -1) stairs_store = FALSE;
					}
				}
				if (nlev_up) {
					new_level_up_y(wpos, y);
					new_level_up_x(wpos, x);
				}
				if (nlev_down) {
					new_level_down_y(wpos, y);
					new_level_down_x(wpos, x);
				}

				/* All done */
				flag = TRUE;
				emergency_flag = FALSE;
			}

			/* Require fewer walls */
			if (walls) walls--;
		}
#if 0 /* taken are of in cave_gen(), calling this function */
		/* no staircase spam in general, one per alloc_stairs() is enough */
		if (flag && netherrealm_bottom(wpos)) return;
#endif
	}

	if (!emergency_flag) return;
	s_printf("Staircase-generation emergency at (%d, %d, %d)\n", wpos->wx, wpos->wy, wpos->wz);
	/* Emergency! We were unable to place a staircase.
	   This can happen if a huge vault nearly completely fills out a level */
	/* note: tries == 0 here, actually, from above code */

	/* note: if we reach 10000 tries now, we just accept the grid anyway! */
	do {
		starty = rand_int(dun->l_ptr->hgt - 2) + 1;
		startx = rand_int(dun->l_ptr->wid - 2) + 1;
	} while (!cave_floor_bold(zcave, starty, startx) && (++tries < 10000));

	c_ptr = &zcave[starty][startx];
	if (new_feat != -1) {
		c_ptr->feat = new_feat;

		if (stairs_store) {
			sptr->x = x;
			sptr->y = y;
			sptr++;
			/* no more array space? */
			if (sptr->y == -1) stairs_store = FALSE;
		}
	}
	if (nlev_up) {
		new_level_up_y(wpos, starty);
		new_level_up_x(wpos, startx);
	}
	if (nlev_down) {
		new_level_down_y(wpos, starty);
		new_level_down_x(wpos, startx);
	}
}




/*
 * Allocates some objects (using "place" and "type")
 */
static void alloc_object(struct worldpos *wpos, int set, int typ, int num, player_type *p_ptr)
{
	int y, x, k;
	int try = 1000;
	cave_type **zcave;
	if(!(zcave = getcave(wpos))) return;

#ifdef RPG_SERVER /* no objects are generated in Training Tower */
	if (in_trainingtower(wpos) && level_generation_time) return;
#endif

	/* Place some objects */
	for (k = 0; k < num; k++) {
		/* Pick a "legal" spot */
		while (TRUE) {
			bool room;

			if (!--try) {
#if DEBUG_LEVEL > 1
				s_printf("alloc_object failed!\n");
#endif	/* DEBUG_LEVEL */
				return;
			}

			/* Location */
			y = rand_int(dun->l_ptr->hgt);
			x = rand_int(dun->l_ptr->wid);

			/* Require "naked" floor grid */
			if (!cave_naked_bold(zcave, y, x)) continue;

			/* Hack -- avoid doors */
			if (typ != ALLOC_TYP_TRAP &&
				f_info[zcave[y][x].feat].flags1 & FF1_DOOR)
				continue;

			/* Check for "room" */
			room = (zcave[y][x].info & CAVE_ROOM) ? TRUE : FALSE;

			/* Require corridor? */
			if ((set == ALLOC_SET_CORR) && room) continue;

			/* Require room? */
			if ((set == ALLOC_SET_ROOM) && !room) continue;

			/* Accept it */
			break;
		}

		/* Place something */
		switch (typ) {
		case ALLOC_TYP_RUBBLE:
			place_rubble(wpos, y, x);
			break;

		case ALLOC_TYP_TRAP:
			place_trap(wpos, y, x, 0);
			break;

		case ALLOC_TYP_GOLD:
			place_gold(wpos, y, x, 0);
			/* hack -- trap can be hidden under gold */
			if (rand_int(100) < 3) place_trap(wpos, y, x, 0);
			break;

		case ALLOC_TYP_OBJECT:
			place_object(wpos, y, x, FALSE, FALSE, FALSE, make_resf(p_ptr), default_obj_theme, 0, ITEM_REMOVAL_NEVER);
			/* hack -- trap can be hidden under an item */
			if (rand_int(100) < 2) place_trap(wpos, y, x, 0);
			break;

#if 0
		case ALLOC_TYP_ALTAR:
			place_altar(wpos, y, x);
			break;
#endif	/* 0 */

		case ALLOC_TYP_BETWEEN:
			place_between(wpos, y, x);
			break;

		case ALLOC_TYP_FOUNTAIN:
			place_fountain(wpos, y, x);
			break;

		case ALLOC_TYP_FOUNTAIN_OF_BLOOD:
			/* this actually means "allocate SOME fountains of blood,
			   while keeping most of them normal ones" ;) - C. Blue */
			if (!rand_int(3)) place_fountain_of_blood(wpos, y, x);
			else place_fountain(wpos, y, x);
			break;
		}
	}
}

/*
 * Helper function for "monster nest (undead)"
 */
static bool vault_aux_aquatic(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* Require Aquatique */
	if (!(r_ptr->flags7 & RF7_AQUATIC)) return (FALSE);

	/* Okay */
	return (TRUE);
}

/*
 * Places "streamers" of rock through dungeon
 *
 * Note that their are actually six different terrain features used
 * to represent streamers.  Three each of magma and quartz, one for
 * basic vein, one with hidden gold, and one with known gold.  The
 * hidden gold types are currently unused.
 */
static void build_streamer(struct worldpos *wpos, int feat, int chance, bool pierce) {
	int		i, tx, ty, tries = 1000;
	int		y, x, dir;
	cave_type *c_ptr;

	dungeon_type *dt_ptr = getdungeon(wpos);
	struct c_special *cs_ptr;
	cave_type **zcave;
	int dun_type;

#ifdef IRONDEEPDIVE_MIXED_TYPES
	if (in_irondeepdive(wpos)) dun_type = iddc[ABS(wpos->wz)].type;
	else
#endif
	dun_type = dt_ptr->theme ? dt_ptr->theme : dt_ptr->type;

	if (!(zcave = getcave(wpos))) return;

#if 1
	/* hard-coded hack: Training Tower has no streamers, only built granite walls */
	if (in_trainingtower(wpos))
		return;
#else
	/* hard-coded hack: Training Tower streamers have no treasure */
	if (in_trainingtower(wpos))
		chance = 0;
#endif

	/* Hack -- Choose starting point */
	while (TRUE) {
		y = rand_spread(dun->l_ptr->hgt / 2, 10);
		x = rand_spread(dun->l_ptr->wid / 2, 15);

		if (in_bounds4(dun->l_ptr, y, x)) break;

		if (tries-- == 0) return;
	}

	/* Choose a random compass direction */
	dir = ddd[rand_int(8)];

	/* Place streamer into dungeon */
	while (TRUE) {
		/* One grid per density */
		for (i = 0; i < DUN_STR_DEN; i++) {

			int d = DUN_STR_RNG;

			/* Pick a nearby grid */
			tries = 1000;
			while (TRUE) {
				ty = rand_spread(y, d);
				tx = rand_spread(x, d);
				if (in_bounds4(dun->l_ptr, ty, tx)) break;
				if (tries-- == 0) return;
			}

			/* Access the grid */
			c_ptr = &zcave[ty][tx];

			/* Only convert "granite" walls */
			if (pierce) {
				if (cave_perma_bold(zcave, ty, tx)) continue;
			} else {
				/* Only convert "granite" walls */
				if (!(c_ptr->feat == feat_wall_inner) &&
				    !(c_ptr->feat == feat_wall_outer) &&
				    !(c_ptr->feat == d_info[dun_type].fill_type[0]) &&
				    !(c_ptr->feat == d_info[dun_type].fill_type[1]) &&
				    !(c_ptr->feat == d_info[dun_type].fill_type[2]) &&
#ifdef IRONDEEPDIVE_MIXED_TYPES
				    !(c_ptr->feat == d_info[((in_irondeepdive(wpos) && iddc[ABS(wpos->wz)].step > 0) ? iddc[ABS(wpos->wz)].next : dun_type)].fill_type[3]) &&
				    !(c_ptr->feat == d_info[((in_irondeepdive(wpos) && iddc[ABS(wpos->wz)].step > 1) ? iddc[ABS(wpos->wz)].next : dun_type)].fill_type[4]))
#else
				    !(c_ptr->feat == d_info[dun_type].fill_type[3]) &&
				    !(c_ptr->feat == d_info[dun_type].fill_type[4]))
#endif
					continue;

#if 0
				if (c_ptr->feat < FEAT_WALL_EXTRA) continue;
				if (c_ptr->feat > FEAT_WALL_SOLID) continue;
#endif	/* 0 */
			}

			if (dun->no_penetr && c_ptr->info & CAVE_ICKY) continue;

			/* Clear mimic feature to avoid nasty consequences */
			if ((cs_ptr = GetCS(c_ptr, CS_MIMIC)))
				cs_erase(c_ptr, cs_ptr);

			/* Clear previous contents, add proper vein type */
			c_ptr->feat = feat;

			/* Hack -- Add some (known) treasure */
			/* XXX seemingly it was ToME bug */
			if (chance && rand_int(chance) == 0)
				/* turn into FEAT_SANDWALL_K / FEAT_MAGMA_K / FEAT_QUARTZ_K: */
				c_ptr->feat += (c_ptr->feat == FEAT_SANDWALL ? 0x2 : 0x4);
		}

#if 0
		/* Hack^2 - place watery monsters */
		if (feat == FEAT_DEEP_WATER && magik(2)) vault_monsters(wpos, y, x, 1);
#endif

		/* Advance the streamer */
		y += ddy[dir];
		x += ddx[dir];


		/* Quit before leaving the dungeon */
		if (!in_bounds4(dun->l_ptr, y, x)) break;
	}
}


/*
 * Place streams of water, lava, & trees -KMW-
 * This routine varies the placement based on dungeon level
 * otherwise is similar to build_streamer
 */
static void build_streamer2(worldpos *wpos, int feat, int killwall) {
	int i, j, mid, tx, ty, tries = 1000;
	int y, x, dir;
	int poolchance;
	int poolsize;
	cave_type *c_ptr;
	struct c_special *cs_ptr;
	cave_type **zcave;

#if 0
	dungeon_type *dt_ptr = getdungeon(wpos);
	int dun_type;

 #ifdef IRONDEEPDIVE_MIXED_TYPES
	if (in_irondeepdive(wpos)) dun_type = iddc[ABS(wpos->wz)].type;
	else
 #endif
	dun_type = dt_ptr->theme ? dt_ptr->theme : dt_ptr->type;
#endif	/* 0 */

	if (!(zcave = getcave(wpos))) return;

	poolchance = randint(10);

	/* Hack -- Choose starting point */
	while (TRUE) {
		y = rand_spread(dun->l_ptr->hgt / 2, 10);
		x = rand_spread(dun->l_ptr->wid / 2, 15);

		if (in_bounds4(dun->l_ptr, y, x)) break;
		if (!tries--) return;
	}

	/* Choose a random compass direction */
	dir = ddd[rand_int(8)];

	/* Place streamer into dungeon */
	if (poolchance > 2) {
		while (TRUE) {
			/* One grid per density */
			for (i = 0; i < (DUN_STR_DWLW + 1); i++) {

				int d = DUN_STR_WLW;

				/* Pick a nearby grid */
				tries = 1000;
				while (TRUE) {
					ty = rand_spread(y, d);
					tx = rand_spread(x, d);
					if (in_bounds4(dun->l_ptr, ty, tx)) break;
					if (!tries--) return;
				}

				/* Access grid */
				c_ptr = &zcave[ty][tx];

				/* Never convert vaults */
				if (c_ptr->info & (CAVE_ICKY)) continue;

				/* Reject permanent features */
				if ((f_info[c_ptr->feat].flags1 & (FF1_PERMANENT)) &&
				    (f_info[c_ptr->feat].flags1 & (FF1_FLOOR))) continue;

				/* Avoid converting walls when told so */
				if (killwall == 0)
					if (f_info[c_ptr->feat].flags1 & FF1_WALL) continue;

				/* Clear mimic feature to avoid nasty consequences */
				if ((cs_ptr = GetCS(c_ptr, CS_MIMIC)))
					cs_erase(c_ptr, cs_ptr);

				/* Clear previous contents, add proper vein type */
				cave_set_feat(wpos, ty, tx, feat);
			}

			/* Advance the streamer */
			y += ddy[dir];
			x += ddx[dir];

			/* Change direction */
			if (rand_int(20) == 0) dir = ddd[rand_int(8)];

			/* Stop at dungeon edge */
			if (!in_bounds4(dun->l_ptr, y, x)) break;
		}
	}

	/* Create pool */
	else if ((feat == FEAT_DEEP_WATER) || (feat == FEAT_DEEP_LAVA)) {
		poolsize = 5 + randint(10);
		mid = poolsize / 2;

		/* One grid per density */
		for (i = 0; i < poolsize; i++) {
			for (j = 0; j < poolsize; j++) {
				tx = x + j;
				ty = y + i;

				if (!in_bounds4(dun->l_ptr, ty, tx)) continue;

				if (i < mid) {
					if (j < mid) {
						if ((i + j + 1) < mid) continue;
					}
					else if (j > (mid+ i)) continue;
				}
				else if (j < mid)
				{
					if (i > (mid + j))
						continue;
				}
				else if ((i + j) > ((mid * 3)-1))
					continue;

				c_ptr = &zcave[ty][tx];

				/* Only convert non-permanent features */
				if (f_info[c_ptr->feat].flags1 & FF1_PERMANENT) continue;


				/* Clear mimic feature to avoid nasty consequences */
				if ((cs_ptr = GetCS(c_ptr, CS_MIMIC)))
					cs_erase(c_ptr, cs_ptr);

				/* Clear previous contents, add proper vein type */
				cave_set_feat(wpos, ty, tx, feat);
			}
		}
	}
}


#if 0
int dist2(int x1, int y1, int x2, int y2, int h1, int h2, int h3, int h4)
/*
 * Build an underground lake 
 */
static void lake_level(struct worldpos *wpos)
{
	int y1, x1, y, x, k, t, n, rad;
	int h1, h2, h3, h4;
	bool distort = FALSE;
	cave_type *c_ptr;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Drop a few epi-centers (usually about two) */
	for (n = 0; n < DUN_LAKE_TRY; n++)
	{
		/* Pick an epi-center */
		y1 = rand_range(5, dun->l_ptr->hgt - 6);
		x1 = rand_range(5, dun->l_ptr->wid - 6);

		/* Access the grid */
		c_ptr = &zcave[y1][x1];

		if (c_ptr->feat != FEAT_DEEP_WATER) continue;

		rad = rand_range(8, 18);
		distort = magik(50 - rad * 2);

		/* (Warning killer) */
		h1 = h2 = h3 = h4 = 0;

		if (distort)
		{
			/* Make a random metric */
			h1 = randint(32) - 16;
			h2 = randint(16);
			h3 = randint(32);
			h4 = randint(32) - 16;
		}

		/* Big area of affect */
		for (y = (y1 - rad); y <= (y1 + rad); y++)
		{
			for (x = (x1 - rad); x <= (x1 + rad); x++)
			{
				/* Skip illegal grids */
				if (!in_bounds(y, x)) continue;

				/* Extract the distance */
				k = distort ? dist2(y1, x1, y, x, h1, h2, h3, h4) :
					distance(y1, x1, y, x);

				/* Stay in the circle of death */
				if (k > rad) continue;

#if 0
				/* Delete the monster (if any) */
				delete_monster(wpos, y, x, TRUE);
#endif

				/* Destroy valid grids */
				if (cave_valid_bold(zcave, y, x))
				{
#if 0
					/* Delete the object (if any) */
					delete_object(wpos, y, x, TRUE);
#endif

					/* Access the grid */
					c_ptr = &zcave[y][x];

					/* Wall (or floor) type */
					t = rand_int(200);

					/* Water */
					if (t < 174)
					{
						/* Create granite wall */
						c_ptr->feat = FEAT_DEEP_WATER;
					}
					else if (t < 180)
					{
						/* Create dirt */
						c_ptr->feat = FEAT_DIRT;
					}
					else if (t < 182)
					{
						/* Create dirt */
						c_ptr->feat = FEAT_MUD;
					}

					/* No longer part of a room or vault */
					c_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

					/* No longer illuminated or known */
					c_ptr->info &= ~CAVE_GLOW;
				}
			}
		}
		break;
	}
}
#endif	/* 0 */


/*
 * Build a destroyed level
 */
static void destroy_level(struct worldpos *wpos) {
	int y1, x1, y, x, k, t, n;
	cave_type *c_ptr;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Drop a few epi-centers (usually about two) */
	for (n = 0; n < randint(5); n++) {
		/* Pick an epi-center */
		y1 = rand_range(5, dun->l_ptr->hgt - 6);
		x1 = rand_range(5, dun->l_ptr->wid - 6);

		/* Big area of affect */
		for (y = (y1 - 15); y <= (y1 + 15); y++) {
			for (x = (x1 - 15); x <= (x1 + 15); x++) {
				/* Skip illegal grids */
				if (!in_bounds(y, x)) continue;

				/* Extract the distance */
				k = distance(y1, x1, y, x);

				/* Stay in the circle of death */
				if (k >= 16) continue;

				/* Delete the monster (if any) */
				delete_monster(wpos, y, x, TRUE);

				/* Destroy valid grids */
				if (cave_valid_bold(zcave, y, x)) {
					/* Delete the object (if any) */
					delete_object(wpos, y, x, TRUE);

					/* Access the grid */
					c_ptr = &zcave[y][x];

					/* Wall (or floor) type */
					t = rand_int(200);

					/* Granite */
					if (t < 20)
						/* Create granite wall */
						c_ptr->feat = FEAT_WALL_EXTRA;

					/* Quartz */
					else if (t < 70)
						/* Create quartz vein */
						c_ptr->feat = FEAT_QUARTZ;

					/* Magma */
					else if (t < 100)
						/* Create magma vein */
						c_ptr->feat = FEAT_MAGMA;

					/* Floor */
					else
						/* Create floor */
						place_floor(wpos, y, x);

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
static void vault_objects(struct worldpos *wpos, int y, int x, int num, player_type *p_ptr) {
	int i, j, k, tries = 1000;
	cave_type **zcave;
	u32b resf = make_resf(p_ptr);
	if (!(zcave = getcave(wpos))) return;

	/* Attempt to place 'num' objects */
	for (; num > 0; --num) {
		/* Try up to 11 spots looking for empty space */
		for (i = 0; i < 11; ++i) {
			/* Pick a random location */
			while (TRUE) {
				j = rand_spread(y, 2);
				k = rand_spread(x, 3);
				if (!in_bounds(j, k)) continue;
				if (!tries--) return;
				break;
			}

			/* Require "clean" floor space */
			if (!cave_clean_bold(zcave, j, k)) continue;

			/* Place an item */
			if (rand_int(100) < 75)
				place_object(wpos, j, k, FALSE, FALSE, FALSE, resf, default_obj_theme, 0, ITEM_REMOVAL_NEVER);

			/* Place gold */
			else
				place_gold(wpos, j, k, 0);

			/* Placement accomplished */
			break;
		}
	}
}


/*
 * Place a trap with a given displacement of point
 */
static void vault_trap_aux(struct worldpos *wpos, int y, int x, int yd, int xd) {
	int count, y1, x1, tries = 1000;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Place traps */
	for (count = 0; count <= 5; count++) {
		/* Get a location */
		while (TRUE) {
			y1 = rand_spread(y, yd);
			x1 = rand_spread(x, xd);
			if (!in_bounds(y1, x1)) continue;
			if (!tries--) return;
			break;
		}

		/* Place the trap */
		place_trap(wpos, y1, x1, 0);

		/* Done */
		break;
	}
}


/*
 * Place some traps with a given displacement of given location
 */
static void vault_traps(struct worldpos *wpos, int y, int x, int yd, int xd, int num) {
	int i;

	for (i = 0; i < num; i++)
		vault_trap_aux(wpos, y, x, yd, xd);
}


/*
 * Hack -- Place some sleeping monsters near the given location
 */
static void vault_monsters(struct worldpos *wpos, int y1, int x1, int num) {
	int k, i, y, x;
	cave_type **zcave;
	int dun_lev;

	if (!(zcave = getcave(wpos))) return;
	dun_lev = getlevel(wpos);

#ifdef ARCADE_SERVER
	if (in_trainingtower(wpos)) return;
#endif

	/* Try to summon "num" monsters "near" the given location */
	for (k = 0; k < num; k++) {
		/* Try nine locations */
		for (i = 0; i < 9; i++) {
			int d = 1;

			/* Pick a nearby location */
			scatter(wpos, &y, &x, y1, x1, d, 0);

			/* Require "empty" floor grids */
			if (!cave_empty_bold(zcave, y, x)) continue;

			/* Place the monster (allow groups) */
			monster_level = dun_lev + 2;
			(void)place_monster(wpos, y, x, TRUE, TRUE);
			monster_level = dun_lev;
		}
	}
}


/* XXX XXX Here begins a big lump of ToME cake	- Jir - */
/* XXX part I -- generic functions */


/*
 * Place a cave filler at (y, x)
 */
static void place_filler(worldpos *wpos, int y, int x) {
	cave_set_feat(wpos, y, x, fill_type[rand_int(1000)]);
}

/*
 * Place floor terrain at (y, x) according to dungeon info
 */
/* XXX this function can produce strange results when called
 * outside of generate.c */
void place_floor(worldpos *wpos, int y, int x) {
	cave_set_feat(wpos, y, x, floor_type[rand_int(1000)]);
}
void place_floor_live(worldpos *wpos, int y, int x) {
	cave_set_feat_live(wpos, y, x, floor_type[rand_int(1000)]);
}

/*
 * Place floor terrain at (y, x) according to dungeon info
 * NOTE: Currently ONLY used for 'maze' levels!
         For that reason, it contains a hack preventing un-walling
         any vaults, to keep them sane. - C. Blue
 */
static void place_floor_respectedly(worldpos *wpos, int y, int x) {
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

#ifndef VAULTS_OVERRIDE_MAZE /* this is actually 'normal', but we need a hack since we allow room/vault generation in mazes currently */
	if (dun->no_penetr && zcave[y][x].info & CAVE_ICKY) return;
#else /* ..and here it is: no harming of vault walls at all */
	if (zcave[y][x].info & CAVE_ICKY) return; /* works fine */
//	if (zcave[y][x].info & CAVE_NEST_PIT) return; /* no effect, because the limiting walls don't have the flag actually. Also, accessibility unclear? */
//	if (zcave[y][x].info & CAVE_ROOM) return; /* lacks door generation! Rooms won't be accessible (although otherwise fine) */
#endif
	cave_set_feat(wpos, y, x, floor_type[rand_int(1000)]);
}

/*
 * Function that sees if a square is a floor (Includes range checking)
 */
static bool get_is_floor(worldpos *wpos, int x, int y)
{
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return(FALSE);

	/* Out of bounds */
	if (!in_bounds(y, x)) return (FALSE);	/* XXX */

	/* Do the real check: */
	if (f_info[zcave[y][x].feat].flags1 & FF1_FLOOR) return (TRUE);
#if 0
	if (zcave[y][x].feat == FEAT_FLOOR ||
		zcave[y][x].feat == FEAT_DEEP_WATER) return (TRUE);
#endif	/* 0 */

	return (FALSE);
}

/*
 * Tunnel around a room if it will cut off part of a cave system
 */
static void check_room_boundary(worldpos *wpos, int x1, int y1, int x2, int y2)
{
	int count, x, y;
	bool old_is_floor, new_is_floor;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

#if 0
	/* Avoid doing this in irrelevant places -- pelpel */
	if (!(d_info[dungeon_type].flags1 & DF1_CAVERN)) return;
#endif

	/* Initialize */
	count = 0;

	old_is_floor = get_is_floor(wpos, x1 - 1, y1);

	/*
	 * Count the number of floor-wall boundaries around the room
	 * Note: diagonal squares are ignored since the player can move diagonally
	 * to bypass these if needed.
	 */

	/* Above the top boundary */
	for (x = x1; x <= x2; x++)
	{
		new_is_floor = get_is_floor(wpos, x, y1 - 1);

		/* increment counter if they are different */
		if (new_is_floor != old_is_floor) count++;

		old_is_floor = new_is_floor;
	}

	/* Right boundary */
	for (y = y1; y <= y2; y++)
	{
		new_is_floor = get_is_floor(wpos, x2 + 1, y);

		/* increment counter if they are different */
		if (new_is_floor != old_is_floor) count++;

		old_is_floor = new_is_floor;
	}

	/* Bottom boundary*/
	for (x = x2; x >= x1; x--)
	{
		new_is_floor = get_is_floor(wpos, x, y2 + 1);

		/* Increment counter if they are different */
		if (new_is_floor != old_is_floor) count++;

		old_is_floor = new_is_floor;
	}

	/* Left boundary */
	for (y = y2; y >= y1; y--)
	{
		new_is_floor = get_is_floor(wpos, x1 - 1, y);

		/* Increment counter if they are different */
		if (new_is_floor != old_is_floor) count++;

		old_is_floor = new_is_floor;
	}


	/* If all the same, or only one connection exit */
	if ((count == 0) || (count == 2)) return;


	/* Tunnel around the room so to prevent problems with caves */
	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			if (in_bounds(y, x)) place_floor(wpos, y, x);
		}
	}
}


/*
 * Allocate the space needed by a room in the room_map array.
 *
 * x, y represent the size of the room (0...x-1) by (0...y-1).
 * crowded is used to denote a monset nest.
 * by0, bx0 are the positions in the room_map array given to the build_type'x'
 * function.
 * xx, yy are the returned center of the allocated room in coordinates for
 * cave.feat and cave.info etc.
 */
bool room_alloc(worldpos *wpos, int x, int y, bool crowded, int by0, int bx0, int *xx, int *yy)
{
	int temp, bx1, bx2, by1, by2, by, bx;


	/* Calculate number of room_map squares to allocate */

	/* Total number along width */
	temp = ((x - 1) / BLOCK_WID) + 1;

	/* Ending block */
	bx2 = temp / 2 + bx0;

	/* Starting block (Note: rounding taken care of here) */
	bx1 = bx2 + 1 - temp;


	/* Total number along height */
	temp = ((y - 1) / BLOCK_HGT) + 1;

	/* Ending block */
	by2 = temp / 2 + by0;

	/* Starting block */
	by1 = by2 + 1 - temp;


	/* Never run off the screen */
	if ((by1 < 0) || (by2 >= dun->row_rooms)) return (FALSE);
	if ((bx1 < 0) || (bx2 >= dun->col_rooms)) return (FALSE);

	/* Verify open space */
	for (by = by1; by <= by2; by++)
	{
		for (bx = bx1; bx <= bx2; bx++)
		{
			if (dun->room_map[by][bx]) return (FALSE);
		}
	}

	/*
	 * It is *extremely* important that the following calculation
	 * be *exactly* correct to prevent memory errors XXX XXX XXX
	 */

	/* Acquire the location of the room */
	*yy = ((by1 + by2 + 1) * BLOCK_HGT) / 2;
	*xx = ((bx1 + bx2 + 1) * BLOCK_WID) / 2;

	/* Save the room location */
	if (dun->cent_n < CENT_MAX)
	{
		dun->cent[dun->cent_n].y = *yy;
		dun->cent[dun->cent_n].x = *xx;
		dun->cent_n++;
	}

	/* Reserve some blocks */
	for (by = by1; by <= by2; by++)
	{
		for (bx = bx1; bx <= bx2; bx++)
		{
			dun->room_map[by][bx] = TRUE;
		}
	}

	/* Count "crowded" rooms */
	if (crowded) dun->crowded = TRUE;

	/*
	 * Hack -- See if room will cut off a cavern.
	 * If so, fix by tunneling outside the room in such a way as
	 * to conect the caves.
	 */
	check_room_boundary(wpos, *xx - x / 2 - 1, *yy - y / 2 - 1,
	                    *xx + x / 2 + 1, *yy + y / 2 + 1);

	/* Success */
	return (TRUE);
}


/*
 * The following functions create a rectangle (e.g. outer wall of rooms)
 */
static void build_rectangle(worldpos *wpos, int y1, int x1, int y2, int x2, int feat, int info)
{
	int y, x;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Top and bottom boundaries */
	for (x = x1; x <= x2; x++)
	{
		cave_set_feat(wpos, y1, x, feat);
		zcave[y1][x].info |= (info);

		cave_set_feat(wpos, y2, x, feat);
		zcave[y2][x].info |= (info);
	}

	/* Top and bottom boundaries */
	for (y = y1; y <= y2; y++)
	{
		cave_set_feat(wpos, y, x1, feat);
		zcave[y][x1].info |= (info);

		cave_set_feat(wpos, y, x2, feat);
		zcave[y][x2].info |= (info);
	}
}


/*
 * Place water through the dungeon using recursive fractal algorithm
 *
 * Why do those good at math and/or algorithms tend *not* to 
 * place any spaces around binary operators? I've been always
 * wondering. This seems almost a unversal phenomenon...
 * Tried to make those conform to the rule, but there may still
 * some left untouched...
 */
static void recursive_river(worldpos *wpos, int x1,int y1, int x2, int y2,
	int feat1, int feat2,int width)
{
	int dx, dy, length, l, x, y;
	int changex, changey;
	int ty, tx;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;


	length = distance(x1, y1, x2, y2);

	if (length > 4)
	{
		/*
		 * Divide path in half and call routine twice.
		 * There is a small chance of splitting the river
		 */
		dx = (x2 - x1) / 2;
		dy = (y2 - y1) / 2;

		if (dy != 0)
		{
			/* perturbation perpendicular to path */
			changex = randint(abs(dy)) * 2 - abs(dy);
		}
		else
		{
			changex = 0;
		}

		if (dx != 0)
		{
			/* perturbation perpendicular to path */
			changey = randint(abs(dx)) * 2 - abs(dx);
		}
		else
		{
			changey = 0;
		}



		/* construct river out of two smaller ones */
		recursive_river(wpos, x1, y1, x1 + dx + changex, y1 + dy + changey,
		                feat1, feat2, width);
		recursive_river(wpos, x1 + dx + changex, y1 + dy + changey, x2, y2,
		                feat1, feat2, width);

		/* Split the river some of the time -junctions look cool */
		if ((width > 0) && (rand_int(DUN_WAT_CHG) == 0))
		{
			recursive_river(wpos, x1 + dx + changex, y1 + dy + changey,
			                x1 + 8 * (dx + changex), y1 + 8 * (dy + changey),
			                feat1, feat2, width - 1);
		}
	}

	/* Actually build the river */
	else
	{
		for (l = 0; l < length; l++)
		{
			x = x1 + l * (x2 - x1) / length;
			y = y1 + l * (y2 - y1) / length;

			for (ty = y - width - 1; ty <= y + width + 1; ty++)
			{
				for (tx = x - width - 1; tx <= x + width + 1; tx++)
				{
					if (!in_bounds(ty, tx)) continue;

					if (zcave[ty][tx].feat == feat1) continue;
					if (zcave[ty][tx].feat == feat2) continue;

					if (distance(ty, tx, y, x) > rand_spread(width, 1)) continue;

					/* Do not convert permanent features */
					if (cave_perma_bold(zcave, ty, tx)) continue;

					/*
					 * Clear previous contents, add feature
					 * The border mainly gets feat2, while the center
					 * gets feat1
					 */
					if (distance(ty, tx, y, x) > width)
					{
						cave_set_feat(wpos, ty, tx, feat2);
					}
					else
					{
						cave_set_feat(wpos, ty, tx, feat1);
					}

					/* Lava terrain glows */
					if ((feat1 == FEAT_DEEP_LAVA) ||
					    (feat1 == FEAT_SHAL_LAVA))
					{
						zcave[ty][tx].info |= CAVE_GLOW;
					}

					/* Hack -- don't teleport here */
					zcave[ty][tx].info |= CAVE_ICKY;
				}
			}
		}
	}
}


/*
 * Places water through dungeon.
 */
static void add_river(worldpos *wpos, int feat1, int feat2)
{
	struct dun_level *l_ptr = getfloor(wpos);

	int y2, x2;
	int y1 = 0, x1 = 0, wid;

	int cur_hgt = dun->l_ptr->hgt;
	int cur_wid = dun->l_ptr->wid;

	cave_type **zcave;
	if(!(zcave = getcave(wpos))) return;

	/* hacks - don't build rivers that are shallower than the actual dungeon floor.. - C. Blue */
	if ((l_ptr->flags1 & (LF1_WATER | LF1_DEEP_WATER))) {
		if (feat1 == FEAT_SHAL_WATER) feat1 = FEAT_DEEP_WATER;
		if (feat2 == FEAT_SHAL_WATER) feat2 = FEAT_DEEP_WATER;
	}
	if ((l_ptr->flags1 & (LF1_LAVA | LF1_DEEP_LAVA))) {
		if (feat1 == FEAT_SHAL_LAVA) feat1 = FEAT_DEEP_LAVA;
		if (feat2 == FEAT_SHAL_LAVA) feat2 = FEAT_DEEP_LAVA;
	}

	/* for DIGGING */
	if (feat1 == FEAT_SHAL_WATER || feat1 == FEAT_DEEP_WATER ||
	    feat2 == FEAT_SHAL_WATER || feat2 == FEAT_DEEP_WATER)
		l_ptr->flags1 |= LF1_WATER;
	if (feat1 == FEAT_SHAL_LAVA || feat1 == FEAT_DEEP_LAVA ||
	    feat2 == FEAT_SHAL_LAVA || feat2 == FEAT_DEEP_LAVA) {
		l_ptr->flags1 |= LF1_LAVA;
s_printf("adding river (%d) %d,%d\n", (l_ptr->flags1 & LF1_DEEP_LAVA) ? 1 : 0, feat1, feat2);
//		feat1 = feat2 = FEAT_DEEP_LAVA;
	}

	/* Hack -- Choose starting point */
	y2 = randint(cur_hgt / 2 - 2) + cur_hgt / 2;
	x2 = randint(cur_wid / 2 - 2) + cur_wid / 2;

	/* Hack -- Choose ending point somewhere on boundary */
	switch(randint(4))
	{
		case 1:
		{
			/* top boundary */
			x1 = randint(cur_wid - 2) + 1;
			y1 = 1;
			break;
		}
		case 2:
		{
			/* left boundary */
			x1 = 1;
			y1 = randint(cur_hgt - 2) + 1;
			break;
		}
		case 3:
		{
			/* right boundary */
			x1 = cur_wid - 1;
			y1 = randint(cur_hgt - 2) + 1;
			break;
		}
		case 4:
		{
			/* bottom boundary */
			x1 = randint(cur_wid - 2) + 1;
			y1 = cur_hgt - 1;
			break;
		}
	}
	wid = randint(DUN_WAT_RNG);
	recursive_river(wpos, x1, y1, x2, y2, feat1, feat2, wid);
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

 *   9 -- vertical oval room
 *  10 -- fractal cave

 *  11 -- random vaults

 *  12 -- crypt room
 */


/*
 * Type 1 -- normal rectangular rooms
 */
static void build_type1(struct worldpos *wpos, int by0, int bx0, player_type *p_ptr) {
	int y, x = 1, y2, x2, yval, xval;
	int y1, x1, xsize, ysize;

	bool light;
	cave_type *c_ptr;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Choose lite or dark */
	light = (getlevel(wpos) <= randint(25)) || magik(2);

#if 0
	/* Pick a room size */
	y1 = yval - randint(4);
	y2 = yval + randint(3);
	x1 = xval - randint(11);
	x2 = xval + randint(11);
#endif	/* 0 */

	/* Pick a room size */
	y1 = randint(4);
	x1 = randint(11);
	y2 = randint(3);
	x2 = randint(11);

	xsize = x1 + x2 + 1;
	ysize = y1 + y2 + 1;

	/* Try to allocate space for room.  If fails, exit */
	if (!room_alloc(wpos, xsize + 2, ysize + 2, FALSE, by0, bx0, &xval, &yval)) return;

	/* Get corner values */
	y1 = yval - ysize / 2;
	x1 = xval - xsize / 2;
	y2 = yval + (ysize + 1) / 2;
	x2 = xval + (xsize + 1) / 2;

/* evileye - exceeds MAX_WID... causes efence crash */
	if ((x2 + 1) >= MAX_WID)
		x2 = MAX_WID - 2;
	if ((y2 + 1) >= MAX_HGT)
		y2 = MAX_HGT - 2;


	/* Place a full floor under the room */
	for (y = y1 - 1; y <= y2 + 1; y++) {
		for (x = x1 - 1; x <= x2 + 1; x++) {
			c_ptr = &zcave[y][x];
			place_floor(wpos, y, x);
			c_ptr->info |= CAVE_ROOM;
			if (light) c_ptr->info |= CAVE_GLOW;
		}
	}

	/* Walls around the room */
	for (y = y1 - 1; y <= y2 + 1; y++) {
		c_ptr = &zcave[y][x1-1];
		c_ptr->feat = feat_wall_outer;
		c_ptr = &zcave[y][x2+1];
		c_ptr->feat = feat_wall_outer;
	}
	for (x = x1 - 1; x <= x2 + 1; x++) {
		c_ptr = &zcave[y1-1][x];
		c_ptr->feat = feat_wall_outer;
		c_ptr = &zcave[y2+1][x];
		c_ptr->feat = feat_wall_outer;
	}


	/* Hack -- Occasional pillar room */
	if (rand_int(20) == 0) {
		for (y = y1; y <= y2; y += 2) {
			for (x = x1; x <= x2; x += 2) {
				c_ptr = &zcave[y][x];
				c_ptr->feat = feat_wall_inner;
			}
		}
	}

	/* Hack -- Occasional ragged-edge room */
	else if (rand_int(50) == 0) {
		for (y = y1 + 2; y <= y2 - 2; y += 2) {
			c_ptr = &zcave[y][x1];
			c_ptr->feat = feat_wall_inner;
			c_ptr = &zcave[y][x2];
			c_ptr->feat = feat_wall_inner;
		}
		for (x = x1 + 2; x <= x2 - 2; x += 2) {
			c_ptr = &zcave[y1][x];
			c_ptr->feat = feat_wall_inner;
			c_ptr = &zcave[y2][x];
			c_ptr->feat = feat_wall_inner;
		}
	}
}


/*
 * Type 2 -- Overlapping rectangular rooms
 */
static void build_type2(struct worldpos *wpos, int by0, int bx0, player_type *p_ptr) {
	int y, x, yval, xval;
	int y1a, x1a, y2a, x2a;
	int y1b, x1b, y2b, x2b;

	bool light;
	cave_type *c_ptr;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Choose lite or dark */
	light = (getlevel(wpos) <= randint(25));

	/* Try to allocate space for room.  If fails, exit */
	if (!room_alloc(wpos, 25, 11, FALSE, by0, bx0, &xval, &yval))return;


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
	for (y = y1a - 1; y <= y2a + 1; y++) {
		for (x = x1a - 1; x <= x2a + 1; x++) {
			c_ptr = &zcave[y][x];
			place_floor(wpos, y, x);
			c_ptr->info |= CAVE_ROOM;
			if (light) c_ptr->info |= CAVE_GLOW;
		}
	}

	/* Place a full floor for room "b" */
	for (y = y1b - 1; y <= y2b + 1; y++) {
		for (x = x1b - 1; x <= x2b + 1; x++) {
			c_ptr = &zcave[y][x];
			place_floor(wpos, y, x);
			c_ptr->info |= CAVE_ROOM;
			if (light) c_ptr->info |= CAVE_GLOW;
		}
	}


	/* Place the walls around room "a" */
	for (y = y1a - 1; y <= y2a + 1; y++) {
		c_ptr = &zcave[y][x1a-1];
		c_ptr->feat = feat_wall_outer;
		c_ptr = &zcave[y][x2a+1];
		c_ptr->feat = feat_wall_outer;
	}
	for (x = x1a - 1; x <= x2a + 1; x++) {
		c_ptr = &zcave[y1a-1][x];
		c_ptr->feat = feat_wall_outer;
		c_ptr = &zcave[y2a+1][x];
		c_ptr->feat = feat_wall_outer;
	}

	/* Place the walls around room "b" */
	for (y = y1b - 1; y <= y2b + 1; y++) {
		c_ptr = &zcave[y][x1b-1];
		c_ptr->feat = feat_wall_outer;
		c_ptr = &zcave[y][x2b+1];
		c_ptr->feat = feat_wall_outer;
	}
	for (x = x1b - 1; x <= x2b + 1; x++) {
		c_ptr = &zcave[y1b-1][x];
		c_ptr->feat = feat_wall_outer;
		c_ptr = &zcave[y2b+1][x];
		c_ptr->feat = feat_wall_outer;
	}



	/* Replace the floor for room "a" */
	for (y = y1a; y <= y2a; y++) {
		for (x = x1a; x <= x2a; x++) {
			c_ptr = &zcave[y][x];
			place_floor(wpos, y, x);
		}
	}

	/* Replace the floor for room "b" */
	for (y = y1b; y <= y2b; y++) {
		for (x = x1b; x <= x2b; x++) {
			c_ptr = &zcave[y][x];
			place_floor(wpos, y, x);
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
static void build_type3(struct worldpos *wpos, int by0, int bx0, player_type *p_ptr) {
	int y, x, dy, dx, wy, wx;
	int y1a, x1a, y2a, x2a;
	int y1b, x1b, y2b, x2b;
	int yval, xval;
	u32b resf = make_resf(p_ptr);

	bool light;
	cave_type *c_ptr;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Try to allocate space for room.  If fails, exit */
	if (!room_alloc(wpos, 25, 11, FALSE, by0, bx0, &xval, &yval)) return;

	/* Choose lite or dark */
	light = (getlevel(wpos) <= randint(25));


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
	for (y = y1a - 1; y <= y2a + 1; y++) {
		for (x = x1a - 1; x <= x2a + 1; x++) {
			c_ptr = &zcave[y][x];
			place_floor(wpos, y, x);
			c_ptr->info |= CAVE_ROOM;
			if (light) c_ptr->info |= CAVE_GLOW;
		}
	}

	/* Place a full floor for room "b" */
	for (y = y1b - 1; y <= y2b + 1; y++) {
		for (x = x1b - 1; x <= x2b + 1; x++) {
			c_ptr = &zcave[y][x];
			place_floor(wpos, y, x);
			c_ptr->info |= CAVE_ROOM;
			if (light) c_ptr->info |= CAVE_GLOW;
		}
	}


	/* Place the walls around room "a" */
	for (y = y1a - 1; y <= y2a + 1; y++) {
		c_ptr = &zcave[y][x1a-1];
		c_ptr->feat = feat_wall_outer;
		c_ptr = &zcave[y][x2a+1];
		c_ptr->feat = feat_wall_outer;
	}
	for (x = x1a - 1; x <= x2a + 1; x++) {
		c_ptr = &zcave[y1a-1][x];
		c_ptr->feat = feat_wall_outer;
		c_ptr = &zcave[y2a+1][x];
		c_ptr->feat = feat_wall_outer;
	}

	/* Place the walls around room "b" */
	for (y = y1b - 1; y <= y2b + 1; y++) {
		c_ptr = &zcave[y][x1b-1];
		c_ptr->feat = feat_wall_outer;
		c_ptr = &zcave[y][x2b+1];
		c_ptr->feat = feat_wall_outer;
	}
	for (x = x1b - 1; x <= x2b + 1; x++) {
		c_ptr = &zcave[y1b-1][x];
		c_ptr->feat = feat_wall_outer;
		c_ptr = &zcave[y2b+1][x];
		c_ptr->feat = feat_wall_outer;
	}


	/* Replace the floor for room "a" */
	for (y = y1a; y <= y2a; y++) {
		for (x = x1a; x <= x2a; x++) {
			c_ptr = &zcave[y][x];
			place_floor(wpos, y, x);
		}
	}

	/* Replace the floor for room "b" */
	for (y = y1b; y <= y2b; y++) {
		for (x = x1b; x <= x2b; x++) {
			c_ptr = &zcave[y][x];
			place_floor(wpos, y, x);
		}
	}



	/* Special features (3/4) */
	switch (rand_int(4)) {
	/* Large solid middle pillar */
	case 1:
		for (y = y1b; y <= y2b; y++) {
			for (x = x1a; x <= x2a; x++) {
				c_ptr = &zcave[y][x];
				c_ptr->feat = feat_wall_inner;
			}
		}
		break;

	/* Inner treasure vault */
	case 2:
		/* Build the vault */
		for (y = y1b; y <= y2b; y++) {
			c_ptr = &zcave[y][x1a];
			c_ptr->feat = feat_wall_inner;
			c_ptr = &zcave[y][x2a];
			c_ptr->feat = feat_wall_inner;
		}
		for (x = x1a; x <= x2a; x++) {
			c_ptr = &zcave[y1b][x];
			c_ptr->feat = feat_wall_inner;
			c_ptr = &zcave[y2b][x];
			c_ptr->feat = feat_wall_inner;
		}

		/* Place a secret door on the inner room */
		switch (rand_int(4)) {
		case 0: place_secret_door(wpos, y1b, xval); break;
		case 1: place_secret_door(wpos, y2b, xval); break;
		case 2: place_secret_door(wpos, yval, x1a); break;
		case 3: place_secret_door(wpos, yval, x2a); break;
		}

		/* Place a treasure in the vault */
		place_object(wpos, yval, xval, FALSE, FALSE, FALSE, resf, default_obj_theme, 0, ITEM_REMOVAL_NEVER);

		/* Let's guard the treasure well */
		vault_monsters(wpos, yval, xval, rand_int(2) + 3);

		/* Place the trap */
		if (magik(50)) place_trap(wpos, yval, xval, 0);

		/* Traps naturally */
		vault_traps(wpos, yval, xval, 4, 4, rand_int(3) + 2);
		break;

	/* Something else */
	case 3:
		/* Occasionally pinch the center shut */
		if (rand_int(3) == 0) {
			/* Pinch the east/west sides */
			for (y = y1b; y <= y2b; y++) {
				if (y == yval) continue;
				c_ptr = &zcave[y][x1a - 1];
				c_ptr->feat = feat_wall_inner;
				c_ptr = &zcave[y][x2a + 1];
				c_ptr->feat = feat_wall_inner;
			}

			/* Pinch the north/south sides */
			for (x = x1a; x <= x2a; x++) {
				if (x == xval) continue;
				c_ptr = &zcave[y1b - 1][x];
				c_ptr->feat = feat_wall_inner;
				c_ptr = &zcave[y2b + 1][x];
				c_ptr->feat = feat_wall_inner;
			}

			/* Sometimes shut using secret doors */
			if (rand_int(3) == 0) {
				place_secret_door(wpos, yval, x1a - 1);
				place_secret_door(wpos, yval, x2a + 1);
				place_secret_door(wpos, y1b - 1, xval);
				place_secret_door(wpos, y2b + 1, xval);
			}
		}

		/* Occasionally put a "plus" in the center */
		else if (rand_int(3) == 0) {
			c_ptr = &zcave[yval][xval];
			c_ptr->feat = feat_wall_inner;
			c_ptr = &zcave[y1b][xval];
			c_ptr->feat = feat_wall_inner;
			c_ptr = &zcave[y2b][xval];
			c_ptr->feat = feat_wall_inner;
			c_ptr = &zcave[yval][x1a];
			c_ptr->feat = feat_wall_inner;
			c_ptr = &zcave[yval][x2a];
			c_ptr->feat = feat_wall_inner;
		}

		/* Occasionally put a pillar in the center */
		else if (rand_int(3) == 0) {
			c_ptr = &zcave[yval][xval];
			c_ptr->feat = feat_wall_inner;
		}

		break;
	}
}


/*
 * Type 4 -- Large room with inner features (pit-shaped room (with pillar(s)) or gorth)
 *
 * Possible sub-types:
 *	1 - Just an inner room with one door
 *	2 - An inner room within an inner room
 *	3 - An inner room with pillar(s)
 *	4 - Inner room has a maze
 *	5 - A set of four inner rooms (a "Gorth")
 * I'm adding the following sub-types: - C. Blue
 *	6 - Inner room has a lot of bones and skulls, possibly low-brain+ferocious guardian
 *	7 - Treasure chamber (inner room has money lying around), possibly non-human guardian
 */
static void build_type4(struct worldpos *wpos, int by0, int bx0, player_type *p_ptr) {
	int y, x, y1, x1, lev = getlevel(wpos);
	int y2, x2, tmp, yval, xval;
	u32b resf = make_resf(p_ptr);

	bool light;
	cave_type *c_ptr;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Try to allocate space for room.  If fails, exit */
	if (!room_alloc(wpos, 25, 11, FALSE, by0, bx0, &xval, &yval)) return;

	/* Choose lite or dark */
	light = (lev <= randint(25));

	/* Large room */
	y1 = yval - 4;
	y2 = yval + 4;
	x1 = xval - 11;
	x2 = xval + 11;


	/* Place a full floor under the room */
	for (y = y1 - 1; y <= y2 + 1; y++) {
		for (x = x1 - 1; x <= x2 + 1; x++) {
			c_ptr = &zcave[y][x];
			place_floor(wpos, y, x);
			c_ptr->info |= CAVE_ROOM;
			if (light) c_ptr->info |= CAVE_GLOW;
		}
	}

	/* Outer Walls */
	for (y = y1 - 1; y <= y2 + 1; y++) {
		c_ptr = &zcave[y][x1-1];
		c_ptr->feat = feat_wall_outer;
		c_ptr = &zcave[y][x2+1];
		c_ptr->feat = feat_wall_outer;
	}
	for (x = x1 - 1; x <= x2 + 1; x++) {
		c_ptr = &zcave[y1-1][x];
		c_ptr->feat = feat_wall_outer;
		c_ptr = &zcave[y2+1][x];
		c_ptr->feat = feat_wall_outer;
	}


	/* The inner room */
	y1 = y1 + 2;
	y2 = y2 - 2;
	x1 = x1 + 2;
	x2 = x2 - 2;

	/* The inner walls */
	for (y = y1 - 1; y <= y2 + 1; y++) {
		c_ptr = &zcave[y][x1-1];
		c_ptr->feat = feat_wall_inner;
		c_ptr = &zcave[y][x2+1];
		c_ptr->feat = feat_wall_inner;
	}
	for (x = x1 - 1; x <= x2 + 1; x++) {
		c_ptr = &zcave[y1-1][x];
		c_ptr->feat = feat_wall_inner;
		c_ptr = &zcave[y2+1][x];
		c_ptr->feat = feat_wall_inner;
	}


	/* Inner room variations */
#if !defined(BONE_AND_TREASURE_CHAMBERS) && !defined(IDDC_BONE_AND_TREASURE_CHAMBERS)
	switch (randint(15)) {
#elif defined(BONE_AND_TREASURE_CHAMBERS) && defined(IDDC_BONE_AND_TREASURE_CHAMBERS)
	switch (randint(16)) {
#elif defined(IDDC_BONE_AND_TREASURE_CHAMBERS)
	switch (randint(in_irondeepdive(wpos) ? 16 : 15)) {
#else
	switch (randint(in_irondeepdive(wpos) ? 15 : 16)) {
#endif

	/* Just an inner room with a monster */
	case 1: case 2: case 3:
		/* Place a secret door */
		switch (randint(4)) {
		case 1: place_secret_door(wpos, y1 - 1, xval); break;
		case 2: place_secret_door(wpos, y2 + 1, xval); break;
		case 3: place_secret_door(wpos, yval, x1 - 1); break;
		case 4: place_secret_door(wpos, yval, x2 + 1); break;
		}

		/* Place a monster in the room */
		vault_monsters(wpos, yval, xval, 1);
		break;

	/* Treasure Vault (with a door) */
	case 4: case 5: case 6:
		/* Place a secret door */
		switch (randint(4)) {
		case 1: place_secret_door(wpos, y1 - 1, xval); break;
		case 2: place_secret_door(wpos, y2 + 1, xval); break;
		case 3: place_secret_door(wpos, yval, x1 - 1); break;
		case 4: place_secret_door(wpos, yval, x2 + 1); break;
		}

		/* Place another inner room */
		for (y = yval - 1; y <= yval + 1; y++) {
			for (x = xval -  1; x <= xval + 1; x++) {
				if ((x == xval) && (y == yval)) continue;
				c_ptr = &zcave[y][x];
				c_ptr->feat = feat_wall_inner;
			}
		}

		/* Place a locked door on the inner room */
		switch (randint(4)) {
		case 1: place_locked_door(wpos, yval - 1, xval); break;
		case 2: place_locked_door(wpos, yval + 1, xval); break;
		case 3: place_locked_door(wpos, yval, xval - 1); break;
		case 4: place_locked_door(wpos, yval, xval + 1); break;
		}

		/* Monsters to guard the "treasure" */
		vault_monsters(wpos, yval, xval, randint(3) + 2);

		/* Object (80%) */
		if (rand_int(100) < 80)
			place_object(wpos, yval, xval, FALSE, FALSE, FALSE, resf, default_obj_theme, 0, ITEM_REMOVAL_NEVER);

		/* Stairs (20%) */
		else {
#ifndef ARCADE_SERVER
			place_random_stairs(wpos, yval, xval);
#else
			if (wpos->wz < 0) place_random_stairs(wpos, yval, xval);
#endif
		}

		/* Traps to protect the treasure */
		vault_traps(wpos, yval, xval, 4, 10, 2 + randint(3));
		break;

	/* Inner pillar(s). */
	case 7: case 8: case 9:
		/* Place a secret door */
		switch (randint(4)) {
		case 1: place_secret_door(wpos, y1 - 1, xval); break;
		case 2: place_secret_door(wpos, y2 + 1, xval); break;
		case 3: place_secret_door(wpos, yval, x1 - 1); break;
		case 4: place_secret_door(wpos, yval, x2 + 1); break;
		}

		/* Large Inner Pillar */
		for (y = yval - 1; y <= yval + 1; y++) {
			for (x = xval - 1; x <= xval + 1; x++) {
				c_ptr = &zcave[y][x];
				c_ptr->feat = feat_wall_inner;
			}
		}

		/* Occasionally, two more Large Inner Pillars */
		if (rand_int(2) == 0) {
			tmp = randint(2);
			for (y = yval - 1; y <= yval + 1; y++) {
				for (x = xval - 5 - tmp; x <= xval - 3 - tmp; x++) {
					c_ptr = &zcave[y][x];
					c_ptr->feat = feat_wall_inner;
				}
				for (x = xval + 3 + tmp; x <= xval + 5 + tmp; x++) {
					c_ptr = &zcave[y][x];
					c_ptr->feat = feat_wall_inner;
				}
			}
		}

		/* Occasionally, some Inner rooms */
		if (rand_int(3) == 0) {
			/* Long horizontal walls */
			for (x = xval - 5; x <= xval + 5; x++) {
				c_ptr = &zcave[yval-1][x];
				c_ptr->feat = feat_wall_inner;
				c_ptr = &zcave[yval+1][x];
				c_ptr->feat = feat_wall_inner;
			}

			/* Close off the left/right edges */
			c_ptr = &zcave[yval][xval-5];
			c_ptr->feat = feat_wall_inner;
			c_ptr = &zcave[yval][xval+5];
			c_ptr->feat = feat_wall_inner;

			/* Secret doors (random top/bottom) */
			place_secret_door(wpos, yval - 3 + (randint(2) * 2), xval - 3);
			place_secret_door(wpos, yval - 3 + (randint(2) * 2), xval + 3);

			/* Monsters */
			vault_monsters(wpos, yval, xval - 2, randint(2));
			vault_monsters(wpos, yval, xval + 2, randint(2));

			/* Objects */
			if (rand_int(3) == 0) place_object(wpos, yval, xval - 2, FALSE, FALSE, FALSE, resf, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
			if (rand_int(3) == 0) place_object(wpos, yval, xval + 2, FALSE, FALSE, FALSE, resf, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
		}
		break;

	/* Maze inside. */
	case 10: case 11: case 12:
		/* Place a secret door */
		switch (randint(4)) {
		case 1: place_secret_door(wpos, y1 - 1, xval); break;
		case 2: place_secret_door(wpos, y2 + 1, xval); break;
		case 3: place_secret_door(wpos, yval, x1 - 1); break;
		case 4: place_secret_door(wpos, yval, x2 + 1); break;
		}

		/* Maze (really a checkerboard) */
		for (y = y1; y <= y2; y++) {
			for (x = x1; x <= x2; x++) {
				if (0x1 & (x + y)) {
					c_ptr = &zcave[y][x];
					c_ptr->feat = feat_wall_inner;
				}
			}
		}

		/* Monsters just love mazes. */
		vault_monsters(wpos, yval, xval - 5, randint(3));
		vault_monsters(wpos, yval, xval + 5, randint(3));

		/* Traps make them entertaining. */
		vault_traps(wpos, yval, xval - 3, 2, 8, randint(3));
		vault_traps(wpos, yval, xval + 3, 2, 8, randint(3));

		/* Mazes should have some treasure too. */
		vault_objects(wpos, yval, xval, 3, p_ptr);
		break;


	/* Four small rooms. */
	case 13: case 14: case 15:
		/* Inner "cross" */
		for (y = y1; y <= y2; y++) {
			c_ptr = &zcave[y][xval];
			c_ptr->feat = feat_wall_inner;
		}
		for (x = x1; x <= x2; x++) {
			c_ptr = &zcave[yval][x];
			c_ptr->feat = feat_wall_inner;
		}

		/* Doors into the rooms */
		if (rand_int(100) < 50) {
			int i = randint(10);
			place_secret_door(wpos, y1 - 1, xval - i);
			place_secret_door(wpos, y1 - 1, xval + i);
			place_secret_door(wpos, y2 + 1, xval - i);
			place_secret_door(wpos, y2 + 1, xval + i);
		} else {
			int i = randint(3);
			place_secret_door(wpos, yval + i, x1 - 1);
			place_secret_door(wpos, yval - i, x1 - 1);
			place_secret_door(wpos, yval + i, x2 + 1);
			place_secret_door(wpos, yval - i, x2 + 1);
		}

		/* Treasure, centered at the center of the cross */
		vault_objects(wpos, yval, xval, 2 + randint(2), p_ptr);

		/* Gotta have some monsters. */
		vault_monsters(wpos, yval + 1, xval - 4, randint(4));
		vault_monsters(wpos, yval + 1, xval + 4, randint(4));
		vault_monsters(wpos, yval - 1, xval - 4, randint(4));
		vault_monsters(wpos, yval - 1, xval + 4, randint(4));

		break;


#if defined(BONE_AND_TREASURE_CHAMBERS) || defined(IDDC_BONE_AND_TREASURE_CHAMBERS)
	/* Room with lots of bones, possibly guardian (Butcher-style ;) or
	   room with lots of treasure, possibly guardian - C. Blue */
	case 16:
		/* Place a secret door */
		switch (randint(4)) {
		case 1: place_secret_door(wpos, y1 - 1, xval); break;
		case 2: place_secret_door(wpos, y2 + 1, xval); break;
		case 3: place_secret_door(wpos, yval, x1 - 1); break;
		case 4: place_secret_door(wpos, yval, x2 + 1); break;
		}

		if (rand_int(2)) {
s_printf("ROOM4_BONES (%d,%d,%d)\n", wpos->wx, wpos->wy, wpos->wz);
			/* Place bones, skulls and skeletons */
			object_type forge;

			//for (y = yval; y <= yval; y++)
				//for (x = xval; x <= xval; x++)
			for (y = y1; y <= y2; y++)
				for (x = x1; x <= x2; x++)
					if (!rand_int(5)) {
						invcopy(&forge, lookup_kind(TV_SKELETON, randint(8)));
						drop_near(0, &forge, 0, wpos, y, x);
					}

			/* Place a monster in the room - ideas: bizarre/undead/animal? (the butcher!) */
			if (rand_int(2)) {
				x = xval - 9 + rand_int(19);
				y = yval - 2 + rand_int(5);
				monster_level = lev + 10;
				place_monster(wpos, y, x, TRUE, TRUE);
				monster_level = lev;
			}
		} else {
s_printf("ROOM4_TREASURE (%d,%d,%d)\n", wpos->wx, wpos->wy, wpos->wz);
			/* Place monetary treasure */
			//for (y = yval; y <= yval; y++)
				//for (x = xval; x <= xval; x++)
			for (y = y1; y <= y2; y++)
				for (x = x1; x <= x2; x++)
					if (!rand_int(5)) place_gold(wpos, y, x, 0);

			/* Place a monster in the room - ideas: creeping coins or treasure hoarders? */
			if (rand_int(2)) {
				x = xval - 9 + rand_int(19);
				y = yval - 2 + rand_int(5);
				monster_level = lev + 10;
				place_monster(wpos, y, x, TRUE, TRUE);
				monster_level = lev;
			}
		}

		break;
#endif

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

/* Some new types of nests/pits are borrowed from ToME.		- Jir - */

/* Hack -- for clone pits */
static int template_race;

/*
 * Dungeon monster filter - not null
 */
bool dungeon_aux(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_DUNGEON)
		return TRUE;
	else
		return FALSE;
}

/*
 * Quest monster filter
 */
bool xorder_aux(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	/* Reject monsters that occur in the wilderness instead of the dungeon */
	if (!(r_ptr->flags8 & RF8_DUNGEON))
		return FALSE;

	/* Reject 'non-spawning' monsters */
	if (!r_ptr->rarity) return (FALSE);

	return TRUE;
}

/*
 * Helper function for "monster nest (jelly)"
 */
static bool vault_aux_jelly(int r_idx) {
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
static bool vault_aux_animal(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	/* No aquatic life in the dungeon */
	if (r_ptr->flags7 & RF7_AQUATIC) return(FALSE);

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
static bool vault_aux_undead(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* Require Undead */
	if (!(r_ptr->flags3 & RF3_UNDEAD)) return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Helper function for "monster nest (chapel)"
 */
//Currently unused! only vault_aux_lesser_chapel is in use! (This one would be BAD with Blade angels!)
static bool vault_aux_chapel(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & (RF1_UNIQUE)) return (FALSE);

	/* Require "priest" or Angel */
	if (!((r_ptr->d_char == 'A') ||
	    strstr((r_name + r_ptr->name),"riest") ||
	    strstr((r_name + r_ptr->name),"aladin") ||
	    strstr((r_name + r_ptr->name),"emplar")))
		return (FALSE);

	/* Okay */
	return (TRUE);
}
static bool vault_aux_lesser_chapel(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & (RF1_UNIQUE)) return (FALSE);

	/* No Blade angels */
	if (r_ptr->flags0 & RF0_NO_NEST) return (FALSE);

	/* Require "priest" or Angel */
//	if (!((r_ptr->d_char == 'A' && (r_ptr->level <= 70)) ||
	if (!((r_ptr->d_char == 'A') ||
	    strstr((r_name + r_ptr->name),"riest") ||
	    strstr((r_name + r_ptr->name),"aladin") ||
	    strstr((r_name + r_ptr->name),"emplar")))
		return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Helper function for "monster nest (kennel)"
 */
static bool vault_aux_kennel(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & (RF1_UNIQUE)) return (FALSE);

	/* Require a Zephyr Hound or a dog */
	return ((r_ptr->d_char == 'Z') || (r_ptr->d_char == 'C'));

}
static bool vault_aux_lesser_kennel(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & (RF1_UNIQUE)) return (FALSE);

	/* Not too many Black Dogs */
	if (r_ptr->flags0 & RF0_NO_NEST) return (FALSE);

	/* Require a Zephyr Hound or a dog */
	return ((r_ptr->d_char == 'Z') || (r_ptr->d_char == 'C'));

}


/*
 * Helper function for "monster nest (treasure)"
 */
static bool vault_aux_treasure(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & (RF1_UNIQUE)) return (FALSE);

	/* Hack -- allow mimics */
	if (r_ptr->flags9 & (RF9_MIMIC)) return (TRUE);

	/* Require Object form */
	if (!((r_ptr->d_char == '!') || (r_ptr->d_char == '|') ||
	    (r_ptr->d_char == '$') || (r_ptr->d_char == '?') ||
	    (r_ptr->d_char == '=')))
		return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Helper function for "monster nest (clone)"
 */
static bool vault_aux_clone(int r_idx) {
	/* unsure - blades shouldn't happen, but should titans? */
	if (r_info[r_idx].flags0 & RF0_NO_NEST) return (FALSE);

	return (r_idx == template_race);
}


/*
 * Helper function for "monster nest (symbol clone)"
 */
static bool vault_aux_symbol(int r_idx) {
	return ((r_info[r_idx].d_char == (r_info[template_race].d_char))
	    && !(r_info[r_idx].flags1 & RF1_UNIQUE)
	    && !(r_info[r_idx].flags0 & RF0_NO_NEST));
}



/*
 * Helper function for "monster pit (orc)"
 */
static bool vault_aux_orc(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* Hack -- Require "o" monsters */
	if (!strchr("o", r_ptr->d_char)) return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Helper function for "monster pit (orc and ogre)"
 */
static bool vault_aux_orc_ogre(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* Hack -- Require "o" monsters */
	if (!strchr("oO", r_ptr->d_char)) return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Helper function for "monster pit (troll)"
 */
static bool vault_aux_troll(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* Hack -- Require "T" monsters */
	if (!strchr("T", r_ptr->d_char)) return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Helper function for "monster pit (mankind)"
 */
static bool vault_aux_man(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* Hack -- Require "p" or "h" monsters */
	if (!strchr("ph", r_ptr->d_char)) return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Helper function for "monster pit (giant)"
 */
static bool vault_aux_giant(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* Hack -- Require "P" monsters */
	if (!strchr("P", r_ptr->d_char)) return (FALSE);

	/* Okay */
	return (TRUE);
}
static bool vault_aux_lesser_giant(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* No Titans */
	if (r_ptr->flags0 & RF0_NO_NEST) return (FALSE);

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
static bool vault_aux_dragon(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	/* Decline unique monsters */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* Hack -- Require "d" or "D" monsters */
	if (!strchr("Dd", r_ptr->d_char)) return (FALSE);

	/* Hack -- Allow 'all stars' type */
	if (!vault_aux_dragon_mask4) return (TRUE);

	/* Hack -- Require correct "breath attack" */
	if (r_ptr->flags4 != vault_aux_dragon_mask4) return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Helper function for "monster pit (demon)"
 */
static bool vault_aux_demon(int r_idx) {
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
 *   a nest of one monster symbol (Dungeon level 0 and deeper)
 *   a nest of "jelly"/"treasure" monsters   (Dungeon level 5 and deeper)
 *   a nest of one monster index  (Dungeon level 25 and deeper)
 *   a nest of "animal"/"kennel" monsters  (Dungeon level 30 and deeper)
 *   a nest of "chapel"/"undead" monsters  (Dungeon level 50 and deeper)
 *
 * Note that the "get_mon_num()" function may (rarely) fail, in which
 * case the nest will be empty, and will not affect the level rating.
 *
 * Note that "monster nests" will never contain "unique" monsters.
 */
static void build_type5(struct worldpos *wpos, int by0, int bx0, player_type *p_ptr) {
	int y, x, y1, x1, y2, x2, xval, yval;
	int tmp, i, dun_lev;
	s16b what[64];
	cave_type *c_ptr;
	//cptr name;

	dungeon_type *dt_ptr = getdungeon(wpos);
	int dun_type;
#ifdef IRONDEEPDIVE_MIXED_TYPES
	if (in_irondeepdive(wpos)) dun_type = iddc[ABS(wpos->wz)].type;
	else
#endif
	dun_type = dt_ptr->theme ? dt_ptr->theme : dt_ptr->type;

	dun_level *l_ptr = getfloor(wpos);

	bool (*chosen_type)(int r_idx);
	bool empty = FALSE;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	dun_lev = getlevel(wpos);

	/* Try to allocate space for room.  If fails, exit */
	if (!room_alloc(wpos, 25, 11, TRUE, by0, bx0, &xval, &yval)) return;

	/* Large room */
	y1 = yval - 4;
	y2 = yval + 4;
	x1 = xval - 11;
	x2 = xval + 11;


	/* Place the floor area */
	for (y = y1 - 1; y <= y2 + 1; y++) {
		for (x = x1 - 1; x <= x2 + 1; x++) {
			c_ptr = &zcave[y][x];
			place_floor(wpos, y, x);
			c_ptr->info |= CAVE_ROOM;
		}
	}

	/* Place the outer walls */
	for (y = y1 - 1; y <= y2 + 1; y++) {
		c_ptr = &zcave[y][x1-1];
		c_ptr->feat = feat_wall_outer;
		c_ptr = &zcave[y][x2+1];
		c_ptr->feat = feat_wall_outer;
	}
	for (x = x1 - 1; x <= x2 + 1; x++) {
		c_ptr = &zcave[y1-1][x];
		c_ptr->feat = feat_wall_outer;
		c_ptr = &zcave[y2+1][x];
		c_ptr->feat = feat_wall_outer;
	}


	/* Advance to the center room */
	y1 = y1 + 2;
	y2 = y2 - 2;
	x1 = x1 + 2;
	x2 = x2 - 2;

	/* The inner walls */
	for (y = y1 - 1; y <= y2 + 1; y++) {
		c_ptr = &zcave[y][x1-1];
		c_ptr->feat = feat_wall_inner;
		c_ptr = &zcave[y][x2+1];
		c_ptr->feat = feat_wall_inner;
	}
	for (x = x1 - 1; x <= x2 + 1; x++) {
		c_ptr = &zcave[y1-1][x];
		c_ptr->feat = feat_wall_inner;
		c_ptr = &zcave[y2+1][x];
		c_ptr->feat = feat_wall_inner;
	}


	/* Place a secret door */
	switch (randint(4)) {
	case 1: place_secret_door(wpos, y1 - 1, xval); break;
	case 2: place_secret_door(wpos, y2 + 1, xval); break;
	case 3: place_secret_door(wpos, yval, x1 - 1); break;
	case 4: place_secret_door(wpos, yval, x2 + 1); break;
	}

	/* Prevent teleportation into the nest (experimental, 2008-05-26) */
#ifdef NEST_PIT_NO_STAIRS_OUTER
	for (y = yval - 4; y <= yval + 4; y++)
		for (x = xval - 11; x <= xval + 11; x++)
			zcave[y][x].info |= CAVE_NEST_PIT;
#else
 #ifdef NEST_PIT_NO_STAIRS_INNER
	for (y = yval - 2; y <= yval + 2; y++)
		for (x = xval - 9; x <= xval + 92; x++)
			zcave[y][x].info |= CAVE_NEST_PIT;
 #endif
#endif

	/* Hack -- Choose a nest type */
	tmp = randint(dun_lev);

	if ((tmp < 25) && (rand_int(5) != 0)) {
		/* Alternate version that uses get_mon_num() - mikaelh */
		get_mon_num_hook = dungeon_aux;
		set_mon_num2_hook(floor_type[0]);
		get_mon_num_prep(dun_type, reject_uniques);
		template_race = get_mon_num(dun_lev, dun_lev);

		if ((dun_lev >= (25 + randint(15))) && (rand_int(2) != 0)) {
			/* monster nest (same r_info symbol) */
			//name = "symbol clone";
			get_mon_num_hook = vault_aux_symbol;
		} else {
			/* monster nest (same r_idx) */
			//name = "clone";
			get_mon_num_hook = vault_aux_clone;
		}
	}
	else if (tmp < 35) {
		/* Monster nest (jelly) */
		/* Describe */
		//name = "jelly";

		/* Restrict to jelly */
		get_mon_num_hook = vault_aux_jelly;
	}

	else if (tmp < 45) {
		//name = "treasure";
		get_mon_num_hook = vault_aux_treasure;
	}

	/* Monster nest (animal) */
	else if (tmp < 65) {
		if (rand_int(3) == 0) {
			//name = "kennel";
			get_mon_num_hook = vault_aux_lesser_kennel;
		} else {
			/* Describe */
			//name = "animal";

			/* Restrict to animal */
			get_mon_num_hook = vault_aux_animal;
		}
	}

	/* Monster nest (undead) */
	else {
		if (rand_int(3) == 0) {
			//name = "chapel";
			get_mon_num_hook = vault_aux_lesser_chapel;
		} else {
			/* Describe */
			//name = "undead";

			/* Restrict to undead */
			get_mon_num_hook = vault_aux_undead;
		}
	}

	/* Set the second hook according to the first floor type */
	set_mon_num2_hook(floor_type[0]);

	/* Prepare allocation table */
	get_mon_num_prep(dun_type, l_ptr->uniques_killed);


	/* Pick some monster types */
	for (i = 0; i < 64; i++) {
		/* Get a (hard) monster type */
		what[i] = get_mon_num(dun_lev + 10, dun_lev);

		/* Notice failure */
		if (!what[i]) empty = TRUE;
	}


	/* Remove restriction */
	chosen_type = get_mon_num_hook; /* preserve for later checks */
	get_mon_num_hook = dungeon_aux;
	get_mon_num2_hook = NULL;

	/* Oops */
	if (empty) {
		s_printf("EMPTY_NEST (%d, %d, %d)\n", wpos->wx, wpos->wy, wpos->wz);
		return;
	}

	/* Place some monsters */
	for (y = yval - 2; y <= yval + 2; y++) {
		for (x = xval - 9; x <= xval + 9; x++) {
			int r_idx = what[rand_int(64)];

			/* Place that "random" monster (no groups) */
			(void)place_monster_aux(wpos, y, x, r_idx, FALSE, FALSE, FALSE, 0);

#if 0 /* not needed, monster level is in fact limited (see further above) */
			if (r_info[r_idx].level >= (dun_lev * 3) / 2 ||
#endif
			if (r_info[r_idx].level >= 60)
				l_ptr->flags2 |= LF2_PITNEST_HI;
		}
	}

	l_ptr->flags2 |= LF2_PITNEST;
	/* summoner nests are dangerous albeit not high level, graveyards too */
	if (chosen_type == vault_aux_chapel ||
	    chosen_type == vault_aux_undead)
		    l_ptr->flags2 |= LF2_PITNEST_HI;
}



/*
 * Type 6 -- Monster pits
 *
 * A monster pit is a "big" room, with an "inner" room, containing
 * a "collection" of monsters of a given type organized in the room.
 *
 * Monster types in the pit
 *   aquatic pit	(Dungeon Level 0 and deeper)
 *   orc pit	(Dungeon Level 5 and deeper)
 *   troll pit	(Dungeon Level 20 and deeper)
 *   orc/ogre pit	(Dungeon Level 30 and deeper)
 *   man pit (h/p)	(Dungeon Level 20 and deeper)
 *   giant pit	(Dungeon Level 40 and deeper)
 *   same r_info symbol pit	(Dungeon Level 5 and deeper)
 *   chapel pit	(Dungeon Level 5 and deeper)
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
#define BUILD_6_MONSTER_TABLE	32
static void build_type6(struct worldpos *wpos, int by0, int bx0, player_type *p_ptr) {
	int tmp, what[BUILD_6_MONSTER_TABLE];
	int i, j, y, x, y1, x1, y2, x2, dun_lev, xval, yval;
	bool empty = FALSE;
#if 0
	bool aqua = magik(dun->watery? 50 : 10);
#else
	/* Disabled for now because aquatic pits are getting filled with eyes of the deep - mikaelh */
	bool aqua = FALSE;
#endif
	cave_type *c_ptr;
	dungeon_type *dt_ptr = getdungeon(wpos);
	int dun_type;
#ifdef IRONDEEPDIVE_MIXED_TYPES
	if (in_irondeepdive(wpos)) dun_type = iddc[ABS(wpos->wz)].type;
	else
#endif
	dun_type = dt_ptr->theme ? dt_ptr->theme : dt_ptr->type;

	dun_level *l_ptr = getfloor(wpos);

	bool (*chosen_type)(int r_idx);
	//cptr		name;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Try to allocate space for room.  If fails, exit */
	if (!room_alloc(wpos, 25, 11, TRUE, by0, bx0, &xval, &yval)) return;

	dun_lev = getlevel(wpos);

	/* Large room */
	y1 = yval - 4;
	y2 = yval + 4;
	x1 = xval - 11;
	x2 = xval + 11;


	/* Place the floor area */
	for (y = y1 - 1; y <= y2 + 1; y++) {
		for (x = x1 - 1; x <= x2 + 1; x++) {
			c_ptr = &zcave[y][x];
			place_floor(wpos, y, x);
			c_ptr->info |= CAVE_ROOM;
		}
	}

	/* Place the outer walls */
	for (y = y1 - 1; y <= y2 + 1; y++) {
		c_ptr = &zcave[y][x1-1];
		c_ptr->feat = feat_wall_outer;
		c_ptr = &zcave[y][x2+1];
		c_ptr->feat = feat_wall_outer;
	}
	for (x = x1 - 1; x <= x2 + 1; x++) {
		c_ptr = &zcave[y1-1][x];
		c_ptr->feat = feat_wall_outer;
		c_ptr = &zcave[y2+1][x];
		c_ptr->feat = feat_wall_outer;
	}


	/* Advance to the center room */
	y1 = y1 + 2;
	y2 = y2 - 2;
	x1 = x1 + 2;
	x2 = x2 - 2;


#if 0
	/* This creates pits that are just too evil - mikaelh */
	if (aqua) {
		/* Fill aquatic pits with water */
		for (y = y1 - 1; y <= y2 + 1; y++)
			for (x = x1 - 1; x <= x2 + 1; x++)
				cave_set_feat(wpos, y, x, FEAT_DEEP_WATER);
	}
#endif


	/* The inner walls */
	for (y = y1 - 1; y <= y2 + 1; y++) {
		c_ptr = &zcave[y][x1-1];
		c_ptr->feat = feat_wall_inner;
		c_ptr = &zcave[y][x2+1];
		c_ptr->feat = feat_wall_inner;
	}
	for (x = x1 - 1; x <= x2 + 1; x++) {
		c_ptr = &zcave[y1-1][x];
		c_ptr->feat = feat_wall_inner;
		c_ptr = &zcave[y2+1][x];
		c_ptr->feat = feat_wall_inner;
	}


	/* Place a secret door */
	switch (randint(4)) {
	case 1: place_secret_door(wpos, y1 - 1, xval); break;
	case 2: place_secret_door(wpos, y2 + 1, xval); break;
	case 3: place_secret_door(wpos, yval, x1 - 1); break;
	case 4: place_secret_door(wpos, yval, x2 + 1); break;
	}


	/* Prevent teleportation into the nest (experimental, 2008-05-26) */
#ifdef NEST_PIT_NO_STAIRS_OUTER
	for (y = yval - 4; y <= yval + 4; y++)
		for (x = xval - 11; x <= xval + 11; x++)
			zcave[y][x].info |= CAVE_NEST_PIT;
#else
 #ifdef NEST_PIT_NO_STAIRS_INNER
	for (y = yval - 2; y <= yval + 2; y++)
		for (x = xval - 9; x <= xval + 92; x++)
			zcave[y][x].info |= CAVE_NEST_PIT;
 #endif
#endif


	/* Choose a pit type */
	tmp = randint(dun_lev);


	/* Watery pit */
	if (aqua) {
		/* Restrict monster selection */
		get_mon_num_hook = vault_aux_aquatic;
	}

	/* Orc pit */
	else if (tmp < 15) {
		if (dun_lev > 30 && magik(50)) {
			/* Restrict monster selection */
			get_mon_num_hook = vault_aux_orc_ogre;
		} else {
			/* Restrict monster selection */
			get_mon_num_hook = vault_aux_orc;
		}
	}

	/* Troll pit */
	else if (tmp < 30) {
		/* Restrict monster selection */
		get_mon_num_hook = vault_aux_troll;
	}

	/* Man pit */
	else if (tmp < 40) {
		/* Restrict monster selection */
		get_mon_num_hook = vault_aux_man;
	}

	/* Giant pit */
	else if (tmp < 55) {
		/* Restrict monster selection */
		get_mon_num_hook = vault_aux_lesser_giant;
	}

	else if (tmp < 70) {
		if (randint(4) != 1) {
#if 0
			do { template_race = randint(MAX_R_IDX - 2); }
			while ((r_info[template_race].flags1 & RF1_UNIQUE)
			       || (((r_info[template_race].level) + randint(5)) >
				   (dun_lev + randint(5))));
#else
			/* Alternate version that uses get_mon_num() - mikaelh */
			get_mon_num_hook = dungeon_aux;
			set_mon_num2_hook(floor_type[0]);
			get_mon_num_prep(dun_type, reject_uniques);
			template_race = get_mon_num(dun_lev, dun_lev);
#endif

			/* Restrict selection */
			get_mon_num_hook = vault_aux_symbol;
		} else {

			get_mon_num_hook = vault_aux_lesser_chapel;
		}

	}

	/* Dragon pit */
	else if (tmp < 80) {
		/* Pick dragon type */
		switch (rand_int(7)) {
		/* Black */
		case 0:
			/* Restrict dragon breath type */
			vault_aux_dragon_mask4 = RF4_BR_ACID;
			break;
		/* Blue */
		case 1:
			/* Restrict dragon breath type */
			vault_aux_dragon_mask4 = RF4_BR_ELEC;
			break;
		/* Red */
		case 2:
			/* Restrict dragon breath type */
			vault_aux_dragon_mask4 = RF4_BR_FIRE;
			break;
		/* White */
		case 3:
			/* Restrict dragon breath type */
			vault_aux_dragon_mask4 = RF4_BR_COLD;
			break;
		/* Green */
		case 4:
			/* Restrict dragon breath type */
			vault_aux_dragon_mask4 = RF4_BR_POIS;
			break;
		/* All stars */
		case 5:
			/* Restrict dragon breath type */
			vault_aux_dragon_mask4 = 0L;
			break;

		/* Multi-hued */
		default:
			/* Restrict dragon breath type */
			vault_aux_dragon_mask4 = (RF4_BR_ACID | RF4_BR_ELEC |
			                          RF4_BR_FIRE | RF4_BR_COLD |
			                          RF4_BR_POIS);
			break;
		}

		/* Restrict monster selection */
		get_mon_num_hook = vault_aux_dragon;
	}

	/* Demon pit */
	else {
		/* Restrict monster selection */
		get_mon_num_hook = vault_aux_demon;
	}

	/* Set the second hook according to the first floor type */
	set_mon_num2_hook(floor_type[0]);

	/* Prepare allocation table */
	get_mon_num_prep(dun_type, l_ptr->uniques_killed);


	/* Pick some monster types */
	for (i = 0; i < BUILD_6_MONSTER_TABLE; i++) {
		for (j = 0; j < 100; j++) {
			/* Get a (hard) monster type */
			what[i] = get_mon_num(dun_lev + 10, dun_lev);
			if (what[i] && !(r_info[what[i]].flags1 & RF1_UNIQUE)) break;
		}
		/* Notice failure */
		if (!what[i]) empty = TRUE;
	}

	chosen_type = get_mon_num_hook; /* preserve */

	/* Remove restriction */
	get_mon_num_hook = dungeon_aux;
	get_mon_num2_hook = NULL;

	/* Oops */
	if (empty) {
		s_printf("EMPTY_PIT (%d, %d, %d)\n", wpos->wx, wpos->wy, wpos->wz);
		return;
	}


	/* XXX XXX XXX */
	/* Sort the entries */
	for (i = 0; i < BUILD_6_MONSTER_TABLE - 1; i++) {
		/* Sort the entries */
		for (j = 0; j < BUILD_6_MONSTER_TABLE - 1; j++) {
			int i1 = j;
			int i2 = j + 1;

			int p1 = r_info[what[i1]].level;
			int p2 = r_info[what[i2]].level;

			/* Bubble */
			if (p1 > p2) {
				int tmp = what[i1];
				what[i1] = what[i2];
				what[i2] = tmp;
			}
		}
	}

	/* Select the entries */
	for (i = 0; i < 8; i++) {
		/* Every other entry */
		if (chosen_type == vault_aux_symbol ||	/* All dangerous nests get weakened here.. */
		    chosen_type == vault_aux_clone ||
		    chosen_type == vault_aux_kennel ||
		    chosen_type == vault_aux_lesser_kennel ||
		    chosen_type == vault_aux_animal ||
		    chosen_type == vault_aux_undead ||
		    chosen_type == vault_aux_dragon ||
		    chosen_type == vault_aux_demon ||
		    chosen_type == vault_aux_chapel ||
		    chosen_type == vault_aux_lesser_chapel ||
		    (chosen_type == vault_aux_troll && dun_lev < 35)) /* otherwise too much exp */
			what[i] = what[i * 2];	/* ..note that this takes only the weaker half
						of the 32 (BUILD_6_MONSTER_TABLE) sorted monsters!.. */
/*		else if (chosen_type == vault_aux_man ||
			chosen_type == vault_aux_giant ||
			chosen_type == vault_aux_aquatic ||*/
		else if	(chosen_type == vault_aux_troll && dun_lev < 45) /* otherwise too much exp? but also very dangerous.. */
			what[i] = what[i * 3];	/* exclude the top quarter */
		else	/* orc, ogre, troll */
			what[i] = what[i * 3 + 10];	/* full size & nice diversity */
/*		else	
			what[i] = what[BUILD_6_MONSTER_TABLE - 15 + i * 2];  <- too powerful =) - C. Blue */
	}

#if 0	
	/* Restrict ultra P pits */
	if (chosen_type == vault_aux_giant) {
		/* Find the first 'Hru' */
		for (i = 0; i < 8; i++) {
			if (what[i] == 709) {
				/* make one 'Hru' the top entry, all following entries weaker than 'Hru' */
				for (j = 7; j >= 7-i; j--) {
					what[j] = what[i+j-7];
				}
				break;
			}
		}
		/* Restrict Hru/Greater Titan/Lesser Titan to entry#1, entry#1/#2/#3, entry #1/#2/#3 respectively */
		/* Check for Hru at positions below #1 (leaving out the last entry, shouldn't occur there really) */
		for (i = 1; i < 7; i++) {
			if (what[i] == 709) {
				/* Copy over the following entries (duplicating the last entry effectively) */
				for (j = i; j >= 1; j--) {
					what[j] = what[j-1];
				}
			}
		}
		/* Check for Greater Titan at positions below #2 or #3 (leaving out the last entry, shouldn't occur there really) */
//		for (i = 1; i < (what[7] == 709 ? 6 : 5); i++) { /* allow more GTs if there's no Hru inside as well */
		for (i = 1; i < 6; i++) { /* up to 3 GTs allowed (at positions #1 and #2) */
			if (what[i] == 702) {
				/* Copy over the following entries (duplicating the last entry effectively) */
				for (j = i; j >= 1; j--) {
					what[j] = what[j-1];
				}
			}
		}
		/* Check for Lesser Titan at positions below #2 or #3 (leaving out the last entry, shouldn't occur there really) */
//		for (i = 1; i < (what[7] == 634 ? 6 : 5); i++) { /* disallow too many LTs */
		for (i = 1; i < ((what[6] == 634) ? 6 : 5); i++) { /* disallow too many LTs */
			if (what[i] == 634) {
				/* Copy over the following entries (duplicating the last entry effectively) */
				for (j = i; j >= 1; j--) {
					what[j] = what[j-1];
				}
			}
		}
	}
#endif
#if 0 /* restrict them some more: */
	/* Restrict ultra P pits */
	if (chosen_type == vault_aux_giant) {
		/* Find the first 'Hru' */
		for (i = 0; i < 8; i++) {
			if (what[i] == 709) {
				/* make one 'Hru' the top entry, all following entries weaker than 'Hru' */
				for (j = 7; j >= 7-i; j--) {
					what[j] = what[i+j-7];
				}
				break;
			}
		}
		/* Find the first 'Greater Titan' */
		for (i = 0; i < 8; i++) {
			if (what[i] == 702) {
				if (what[7] == 709) { /* Is a 'Hru' at the top? */
					/* make 'Greater Titan' the second to top entry, all following entries weaker than 'Greater Titan' */
					for (j = 6; j >= 6-i; j--) {
						what[j] = what[i+j-6];
					}
					break;
				} else {
					/* make one 'Greater Titan' the top entry, all following entries weaker than 'Greater Titan' */
					for (j = 7; j >= 7-i; j--) {
						what[j] = what[i+j-7];
					}
					break;
				}
			}
		}
		/* Find the first 'Lesser Titan' */
		for (i = 0; i < 8; i++) {
			if (what[i] == 634) {
				if (what[6] == 702) { /* Is a 'Greater Titan' already occupying the second to top position? */
					/* Remove either 'Lesser Titan' or 'Greater Titan' since 6 of them would be too many */
					if (magik(50)) what[i] = 702;
					for (j = 6; j >= 6-i; j--) {
						what[j] = what[i+j-6];
					}
					break;
				} else if (what[7] == 702 || what[7] == 709) {
					/* make one 'Lesser Titan' the second to top entry, all following entries weaker than 'Lesser Titan' */
					for (j = 6; j >= 6-i; j--) {
						what[j] = what[i+j-6];
					}
					break;
				} else {
					/* make one 'Lesser Titan' the top entry, all following entries weaker than 'Lesser Titan' */
					for (j = 7; j >= 7-i; j--) {
						what[j] = what[i+j-7];
					}
					break;
				}
			}
		}
	}
#endif
#if 1 /* no titans/hrus in P pits again */
	/* Restrict ultra P pits */
	if (chosen_type == vault_aux_giant) {
		/* Find the first 'Hru' */
		for (i = 0; i < 8; i++) {
			if (what[i] == 709) {
				/* make next entry the top entry, removing the 'Hru' */
				for (j = 7; j >= i; j--) {
					what[j] = what[i-1];
				}
				break;
			}
		}
		/* Find the first 'Greater Titan' */
		for (i = 0; i < 8; i++) {
			if (what[i] == 702) {
				/* make next entry the top entry, removing the 'Greater Titan' */
				for (j = 7; j >= i; j--) {
					what[j] = what[i-1];
				}
				break;
			}
		}
		/* Find the first 'Lesser Titan' */
		for (i = 0; i < 8; i++) {
			if (what[i] == 634) {
				/* make next entry the top entry, removing the 'Lesser Titan' */
				for (j = 7; j >= i; j--) {
					what[j] = what[i-1];
				}
				break;
			}
		}
	}
#endif


	/* Debugging code for "holes" in pits :( */
	for (i = 0; i < 8; i++){
		if (!what[i]) s_printf("HOLE(%d)\n", i);

		/* abuse the debugging code for setting extra level feelings =p */
		else if (r_info[what[i]].level >= 60) l_ptr->flags2 |= LF2_PITNEST_HI;
	}
	l_ptr->flags2 |= LF2_PITNEST;
	/* summoner pits are dangerous albeit not that high level really */
	if (chosen_type == vault_aux_man ||
	    chosen_type == vault_aux_undead ||
	    chosen_type == vault_aux_lesser_chapel)
		l_ptr->flags2 |= LF2_PITNEST_HI;


	/* Top and bottom rows */
	for (x = xval - 9; x <= xval + 9; x++) {
		place_monster_aux(wpos, yval - 2, x, what[0], FALSE, FALSE, FALSE, 0);
		place_monster_aux(wpos, yval + 2, x, what[0], FALSE, FALSE, FALSE, 0);
	}

	/* Middle columns */
	for (y = yval - 1; y <= yval + 1; y++) {
		place_monster_aux(wpos, y, xval - 9, what[0], FALSE, FALSE, FALSE, 0);
		place_monster_aux(wpos, y, xval + 9, what[0], FALSE, FALSE, FALSE, 0);

		place_monster_aux(wpos, y, xval - 8, what[1], FALSE, FALSE, FALSE, 0);
		place_monster_aux(wpos, y, xval + 8, what[1], FALSE, FALSE, FALSE, 0);

		place_monster_aux(wpos, y, xval - 7, what[1], FALSE, FALSE, FALSE, 0);
		place_monster_aux(wpos, y, xval + 7, what[1], FALSE, FALSE, FALSE, 0);

		place_monster_aux(wpos, y, xval - 6, what[2], FALSE, FALSE, FALSE, 0);
		place_monster_aux(wpos, y, xval + 6, what[2], FALSE, FALSE, FALSE, 0);

		place_monster_aux(wpos, y, xval - 5, what[2], FALSE, FALSE, FALSE, 0);
		place_monster_aux(wpos, y, xval + 5, what[2], FALSE, FALSE, FALSE, 0);

		place_monster_aux(wpos, y, xval - 4, what[3], FALSE, FALSE, FALSE, 0);
		place_monster_aux(wpos, y, xval + 4, what[3], FALSE, FALSE, FALSE, 0);

		place_monster_aux(wpos, y, xval - 3, what[3], FALSE, FALSE, FALSE, 0);
		place_monster_aux(wpos, y, xval + 3, what[3], FALSE, FALSE, FALSE, 0);

		place_monster_aux(wpos, y, xval - 2, what[4], FALSE, FALSE, FALSE, 0);
		place_monster_aux(wpos, y, xval + 2, what[4], FALSE, FALSE, FALSE, 0);
	}

	/* Above/Below the center monster */
	for (x = xval - 1; x <= xval + 1; x++) {
		place_monster_aux(wpos, yval + 1, x, what[5], FALSE, FALSE, FALSE, 0);
		place_monster_aux(wpos, yval - 1, x, what[5], FALSE, FALSE, FALSE, 0);
	}

	/* Next to the center monster */
	place_monster_aux(wpos, yval, xval + 1, what[6], FALSE, FALSE, FALSE, 0);
	place_monster_aux(wpos, yval, xval - 1, what[6], FALSE, FALSE, FALSE, 0);

	/* Center monster */
	place_monster_aux(wpos, yval, xval, what[7], FALSE, FALSE, FALSE, 0);
}



/*
 * Hack -- fill in "vault" rooms
 */
bool build_vault(struct worldpos *wpos, int yval, int xval, vault_type *v_ptr, player_type *p_ptr) {
	int bwy[8], bwx[8], i;
	int dx, dy, x, y, cx, cy, lev = getlevel(wpos);
	cptr t;

#ifdef MORGOTH_NO_TELE_VAULT
#ifndef MORGOTH_NO_TELE_VAULTS
	bool morgoth_inside = FALSE;
	monster_type *m_ptr;
#endif
#endif
	bool perma_walled = FALSE, placed;

	cave_type *c_ptr;
	cave_type **zcave;
	c_special *cs_ptr;
	dun_level *l_ptr = getfloor(wpos);
	bool hives = FALSE, mirrorlr = FALSE, mirrorud = FALSE,
		rotate = FALSE, force = FALSE;
	int ymax = v_ptr->hgt, xmax = v_ptr->wid;
	char *data = v_text + v_ptr->text;

	u32b resf = make_resf(p_ptr), eff_resf;
	int eff_forbid_true = 0, eff_forbid_rand = 0;

	if (!(zcave = getcave(wpos))) return FALSE;

	if (v_ptr->flags1 & VF1_NO_PENETR) dun->no_penetr = TRUE;
#if 0 /* Hives mess up the overall level structure too badly - mikaelh */
	if (v_ptr->flags1 & VF1_HIVES) hives = TRUE;
#endif

	/* artificially add random-artifact restrictions: */
	if (lev < 55 + rand_int(6) + rand_int(6)) resf |= RESF_NORANDART;
	if (lev < 75 + rand_int(6) + rand_int(6)) eff_forbid_rand = 80;
//	if (!(v_ptr->flags1 & VF1_NO_TELE)) eff_forbid_rand = 50; /* maybe too harsh, depends on amount of '8's in deep tele-ok-vaults */

	if (v_ptr->flags1 & VF1_NO_TRUEARTS) resf |= RESF_NOTRUEART;
	if (v_ptr->flags1 & VF1_NO_RANDARTS) resf |= RESF_NORANDART;
	if ((v_ptr->flags1 & VF1_NO_EASY_TRUEARTS) && lev < 75 + rand_int(6) + rand_int(6)) resf |= RESF_NOTRUEART;
	if ((v_ptr->flags1 & VF1_NO_EASY_RANDARTS) && lev < 75 + rand_int(6) + rand_int(6)) resf |= RESF_NORANDART;

	if (v_ptr->flags1 & VF1_RARE_TRUEARTS) eff_forbid_true = 80;
	if (v_ptr->flags1 & VF1_RARE_RANDARTS) eff_forbid_rand = 80;

	if (!hives) { /* Hack -- avoid ugly results */
		if (!(v_ptr->flags1 & VF1_NO_MIRROR)) {
			if (magik(30)) mirrorlr = TRUE;
			if (magik(30)) mirrorud = TRUE;
		}
		if (!(v_ptr->flags1 & VF1_NO_ROTATE) && magik(30)) rotate = TRUE;
	}

	cx = xval - ((rotate?ymax:xmax) / 2) * (mirrorlr?-1:1);
	cy = yval - ((rotate?xmax:ymax) / 2) * (mirrorud?-1:1);

	/* At least 1/4 should be genetated */
	if (!in_bounds4(l_ptr, cy, cx))
		return FALSE;

	/* Check for flags */
	if (v_ptr->flags1 & VF1_FORCE_FLAGS) force = TRUE;
	if (v_ptr->flags1 & VF1_NO_GENO && (magik(VAULT_FLAG_CHANCE) || force))
		l_ptr->flags1 |= LF1_NO_GENO;
	if (v_ptr->flags1 & VF1_NO_MAP && (magik(VAULT_FLAG_CHANCE) || force))
		l_ptr->flags1 |= LF1_NO_MAP;
	if (v_ptr->flags1 & VF1_NO_MAGIC_MAP && (magik(VAULT_FLAG_CHANCE) || force))
		l_ptr->flags1 |= LF1_NO_MAGIC_MAP;
	if (v_ptr->flags1 & VF1_NO_DESTROY && (magik(VAULT_FLAG_CHANCE) || force))
		l_ptr->flags1 |= LF1_NO_DESTROY;
	if (v_ptr->flags1 & VF1_NO_MAGIC && (magik(VAULT_FLAG_CHANCE) || force)
			&& lev < 100)
		l_ptr->flags1 |= LF1_NO_MAGIC;

	/* Clean the between gates arrays */
	for(i = 0; i < 8; i++)
		bwy[i] = bwx[i] = 9999;

	/* Place dungeon features and objects */
	for (t = data, dy = 0; dy < ymax; dy++) {
		for (dx = 0; dx < xmax; dx++, t++) {
			eff_resf = resf;
			if (magik(eff_forbid_true)) eff_resf |= RESF_NOTRUEART;
			if (magik(eff_forbid_rand)) eff_resf |= RESF_NORANDART;

			/* Extract the location */
/*			x = xval - (xmax / 2) + dx;
			y = yval - (ymax / 2) + dy;	*/
			x = cx + (rotate?dy:dx) * (mirrorlr?-1:1);
			y = cy + (rotate?dx:dy) * (mirrorud?-1:1);

			/* FIXME - find a better solution */
			/* Is this any better? */
			if (!in_bounds4(l_ptr,y,x)) continue;

			/* Hack -- skip "non-grids" */
			if (*t == ' ') continue;

			/* Access the grid */
			c_ptr = &zcave[y][x];

			/* Lay down a floor */
			place_floor(wpos, y, x);

			/* Remove previous monsters - mikaelh */
			delete_monster(wpos, y, x, TRUE);

			/* Part of a vault */
			c_ptr->info |= (CAVE_ROOM | CAVE_ICKY);
			if(v_ptr->flags1 & VF1_NO_TELEPORT)
				c_ptr->info |= CAVE_STCK;

			/* Analyze the grid */
			switch (*t) {
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
				perma_walled = TRUE;
				break;

			/* Treasure/trap */
			case '*':
				if (rand_int(100) < 75)
					place_object(wpos, y, x, FALSE, FALSE, FALSE, eff_resf, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
				if (rand_int(100) < 40)
					place_trap(wpos, y, x, 0);
				break;

			/* Secret doors */
			case '+':
				place_secret_door(wpos, y, x);
				if (magik(20)) place_trap(wpos, y, x, 0);
				break;

			/* Trap */
			case '^':
				place_trap(wpos, y, x, 0);
				break;

#if 0 /* done later in 2nd pass - mikaelh */
			/* Monster */
			case '&':
				monster_level = lev + 5;
				place_monster(wpos, y, x, TRUE, TRUE);
				monster_level = lev;
				break;

			/* Meaner monster */
			case '@':
				monster_level = lev + 11;
				monster_level_min = -1;
				place_monster(wpos, y, x, TRUE, TRUE);
				monster_level_min = 0;
				monster_level = lev;
				break;

			/* Meaner monster, plus treasure */
			case '9':
				monster_level = lev + 9;
				monster_level_min = -1;
				place_monster(wpos, y, x, TRUE, TRUE);
				monster_level_min = 0;
				monster_level = lev;
				object_level = lev + 7;
				place_object(wpos, y, x, TRUE, FALSE, FALSE, eff_resf, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
				object_level = lev;
				if (magik(40)) place_trap(wpos, y, x, 0);
				break;

			/* Nasty (meanest) monster and treasure */
			case '8':
				monster_level = lev + 40;
				monster_level_min = -1;
				place_monster(wpos, y, x, TRUE, TRUE);
				monster_level_min = 0;
				monster_level = lev;
				object_level = lev + 20;
				place_object(wpos, y, x, TRUE, TRUE, FALSE, eff_resf, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
				object_level = lev;
				if (magik(80)) place_trap(wpos, y, x, 0);
				l_ptr->flags2 |= LF2_VAULT_HI;
 #ifdef TEST_SERVER
 s_printf("DEBUG_FEELING: VAULT_HI by build_vault(), '8' monster\n");
 #endif
				break;

			/* Monster and/or object */
			case ',':
				if (magik(50)) {
					monster_level = lev + 3;
					place_monster(wpos, y, x, TRUE, TRUE);
					monster_level = lev;
				}
				if (magik(50)) {
					object_level = lev + 7;
					place_object(wpos, y, x, FALSE, FALSE, FALSE, eff_resf, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
					object_level = lev;
				}
				if (magik(50)) place_trap(wpos, y, x, 0);
				break;
#endif

			/* Between gates */
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7': {
				/* XXX not sure what will happen if the cave already has
				 * another between gate */
				/* Not found before */
				if(bwy[*t - '0'] == 9999) {
					if (!(cs_ptr = AddCS(c_ptr, CS_BETWEEN))) {
#if DEBUG_LEVEL > 0
						s_printf("oops, vault between gates generation failed(1st: %s)!!\n", wpos_format(0, wpos));
#endif /* DEBUG_LEVEL */
						break;
					}
					cave_set_feat(wpos, y, x, FEAT_BETWEEN);
					bwy[*t - '0'] = y;
					bwx[*t - '0'] = x;
				}
				/* The second time */
				else {
					if (!(cs_ptr = AddCS(c_ptr, CS_BETWEEN))) {
#if DEBUG_LEVEL > 0
						s_printf("oops, vault between gates generation failed(2nd: %s)!!\n", wpos_format(0, wpos));
#endif /* DEBUG_LEVEL */
						break;
					}
					cave_set_feat(wpos, y, x, FEAT_BETWEEN);
					cs_ptr->sc.between.fy = bwy[*t - '0'];
					cs_ptr->sc.between.fx = bwx[*t - '0'];

					cs_ptr = GetCS(&zcave[bwy[*t - '0']][bwx[*t - '0']], CS_BETWEEN);
					cs_ptr->sc.between.fy = y;
					cs_ptr->sc.between.fx = x;
#if 0
					c_ptr->special = bwx[*t - '0'] + (bwy[*t - '0'] << 8);
					cave[bwy[*t - '0']][bwx[*t - '0']].special = x + (y << 8);
#endif	/* 0 */
				}
				break;
			}
			}
		}
	}

	/* Another pass for monsters */
	for (t = data, dy = 0; dy < ymax; dy++) {
		for (dx = 0; dx < xmax; dx++, t++) {
			eff_resf = resf;
			if (magik(eff_forbid_true)) eff_resf |= RESF_NOTRUEART;
			if (magik(eff_forbid_rand)) eff_resf |= RESF_NORANDART;

			/* Extract the location */
/*			x = xval - (xmax / 2) + dx;
			y = yval - (ymax / 2) + dy;	*/
			x = cx + (rotate?dy:dx) * (mirrorlr?-1:1);
			y = cy + (rotate?dx:dy) * (mirrorud?-1:1);

			/* FIXME - find a better solution */
			/* Is this any better? */
			if(!in_bounds4(l_ptr,y,x)) continue;

			/* Hack -- skip "non-grids" */
			if (*t == ' ') continue;
			
			/* Access the grid */
			c_ptr = &zcave[y][x];

			/* Analyze the grid */
			switch (*t) {
			/* Monster */
			case '&':
				monster_level = lev + 5;
				place_monster(wpos, y, x, TRUE, TRUE);
				monster_level = lev;
				break;

			/* Meaner monster */
			case '@':
				monster_level = lev + 11;
				monster_level_min = -1;
				place_monster(wpos, y, x, TRUE, TRUE);
				monster_level_min = 0;
				monster_level = lev;
				break;

			/* Meaner monster, plus treasure */
			case '9':
				monster_level = lev + 9;
				monster_level_min = -1;
				placed = place_monster(wpos, y, x, TRUE, TRUE);
				monster_level_min = 0;
				monster_level = lev;
				object_level = lev + 7;
				if (placed) place_object(wpos, y, x, TRUE, FALSE, FALSE, eff_resf, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
				object_level = lev;
				if (magik(40)) place_trap(wpos, y, x, 0);
				break;

			/* Nasty (meanest) monster and treasure */
			case '8':
				monster_level = lev + 40;
				monster_level_min = -1;
				placed = place_monster(wpos, y, x, TRUE, TRUE);
				monster_level_min = 0;
				monster_level = lev;
				object_level = lev + 20;
				if (placed) place_object(wpos, y, x, TRUE, TRUE, FALSE, eff_resf, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
				object_level = lev;
				if (magik(80)) place_trap(wpos, y, x, 0);
				l_ptr->flags2 |= LF2_VAULT_HI;
#ifdef TEST_SERVER
s_printf("DEBUG_FEELING: VAULT_HI by build_vault(), '8' monster\n");
#endif
				break;

			/* Monster and/or object */
			case ',':
				placed = TRUE;
				if (magik(50)) {
					monster_level = lev + 3;
					placed = place_monster(wpos, y, x, TRUE, TRUE);
					monster_level = lev;
				}
				if (magik(50) && placed) {
					object_level = lev + 7;
					place_object(wpos, y, x, FALSE, FALSE, FALSE, eff_resf, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
					object_level = lev;
				}
				if (magik(50)) place_trap(wpos, y, x, 0);
				break;
			}

#ifdef MORGOTH_NO_TELE_VAULT
#ifndef MORGOTH_NO_TELE_VAULTS
			if (c_ptr->m_idx > 0) {
				/* Check if Morgoth was just placed inside */
				/* Acquire monster pointer */
				m_ptr = &m_list[c_ptr->m_idx];
				/* check.. */
				if (m_ptr->r_idx == RI_MORGOTH) morgoth_inside = TRUE;
			}
#endif
#endif
		}
	}

	/* Check if vault is perma_walled, and make its fields 'remember' that */
	if (perma_walled) {
		for (t = data, dy = 0; dy < ymax; dy++) {
			for (dx = 0; dx < xmax; dx++, t++) {
				/* Extract the location */
				/*x = xval - (xmax / 2) + dx;
				y = yval - (ymax / 2) + dy; */
				x = cx + (rotate?dy:dx) * (mirrorlr?-1:1);
				y = cy + (rotate?dx:dy) * (mirrorud?-1:1);

				/* FIXME - find a better solution */
				/* Is this any better? */
				if(!in_bounds4(l_ptr,y,x)) continue;

				/* Hack -- skip "non-grids" */
				if (*t == ' ') continue;

				/* Access the grid */
				c_ptr = &zcave[y][x];

				/* Remember that this field belongs to a perma-wall vault */
				c_ptr->info |= CAVE_ICKY_PERMA;

				/* overridden by MORGOTH_NO_TELE_VAULTS:
				   instead of making just that vault no-tele,
				   now all vaults on the level are no-tele;
				   performed in cave_gen (in generate.c too) */
#ifdef MORGOTH_NO_TELE_VAULT
#ifndef MORGOTH_NO_TELE_VAULTS
				if (morgoth_inside)
					/* Make it NO_TELEPORT */
					c_ptr->info |= CAVE_STCK;
#endif
#endif
			}
		}
	}

	/* Reproduce itself */
	/* TODO: make a better routine! */
	if (hives) {
		if (magik(HIVE_CHANCE(lev)) && !magik(ymax))
			build_vault(wpos, yval + ymax, xval, v_ptr, p_ptr);
		if (magik(HIVE_CHANCE(lev)) && !magik(xmax))
			build_vault(wpos, yval, xval + xmax, v_ptr, p_ptr);
	}

	l_ptr->flags2 |= LF2_VAULT;
	return TRUE;
}



/*
 * Type 7 -- simple vaults (see "v_info.txt")
 */
static void build_type7(struct worldpos *wpos, int by0, int bx0, player_type *p_ptr) {
	vault_type	*v_ptr;
	int xval, yval, tries = 1000;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Pick a lesser vault */
	while (--tries) {
		/* Access a random vault record */
		v_ptr = &v_info[rand_int(MAX_V_IDX)];

		/* Accept the first lesser vault */
		if (v_ptr->typ == 7) break;
	}
	if (!tries) return; /* failure */

	/* Try to allocate space for room.  If fails, exit */
	if (!room_alloc(wpos, v_ptr->wid, v_ptr->hgt, FALSE, by0, bx0, &xval, &yval))
		return;

	/* Hack -- Build the vault */
	build_vault(wpos, yval, xval, v_ptr, p_ptr);
}



/*
 * Type 8 -- greater vaults (see "v_info.txt")
 */
static void build_type8(struct worldpos *wpos, int by0, int bx0, player_type *p_ptr) {
	dun_level *l_ptr = getfloor(wpos);
	vault_type *v_ptr;
	int xval, yval, tries = 1000;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Pick a greater vault */
	while (--tries) {
		/* Access a random vault record */
		v_ptr = &v_info[rand_int(MAX_V_IDX)];

		/* Accept the first greater vault */
		if (v_ptr->typ == 8) break;
	}
	if (!tries) return; /* failure */

	/* Try to allocate space for room.  If fails, exit */
	if (!room_alloc(wpos, v_ptr->wid, v_ptr->hgt, FALSE, by0, bx0, &xval, &yval)) return;

	/* Hack -- Build the vault */
	if (build_vault(wpos, yval, xval, v_ptr, p_ptr)) {
		l_ptr->flags2 |= LF2_VAULT_HI;
#ifdef TEST_SERVER
s_printf("DEBUG_FEELING: VAULT_HI by build_type8->build_vault\n");
#endif
	}

}


/* XXX XXX Here begins a big lump of ToME cake	- Jir - */
/* XXX XXX part II -- builders */

/*
 * DAG:
 * Build an vertical oval room.
 * For every grid in the possible square, check the distance.
 * If it's less than or == than the radius, make it a room square.
 * If its less, make it a normal grid. If it's == make it an outer
 * wall.
 */
/* Don't allow outer walls that can be passed by walking diagonally? - C. Blue */
#define BUILD_TYPE9_SOLID_OUTER_WALLS
static void build_type9(worldpos *wpos, int by0, int bx0, player_type *p_ptr) {
	int rad, x, y, x0, y0, d;
	int light = FALSE;
	int dun_lev = getlevel(wpos);
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Occasional light */
	if (randint(dun_lev) <= 5) light = TRUE;

	/* Room size */
	rad = rand_int(10);

	/* Try to allocate space for room.  If fails, exit */
	if (!room_alloc(wpos, rad * 2 + 1, rad * 2 + 1, FALSE, by0, bx0, &x0, &y0)) return;

	for (x = x0 - rad; x <= x0 + rad; x++) {
		for (y = y0 - rad; y <= y0 + rad; y++) {
#ifdef BUILD_TYPE9_SOLID_OUTER_WALLS
			d = distance(y0 * 10, x0 * 10, y * 10, x * 10);
#else
			d = distance(y0, x0, y, x);
#endif

#ifdef BUILD_TYPE9_SOLID_OUTER_WALLS
			if (d <= rad * 10 + 5 && d >= rad * 10 - 5) {
#else
			if (d == rad) {
#endif
				zcave[y][x].info |= (CAVE_ROOM);
				if (light) zcave[y][x].info |= (CAVE_GLOW);
				cave_set_feat(wpos, y, x, feat_wall_outer);
			}

#ifdef BUILD_TYPE9_SOLID_OUTER_WALLS
			if (d < rad * 10 - 5) {
#else
			if (d < rad) {
#endif
				zcave[y][x].info |= (CAVE_ROOM);
				if (light) zcave[y][x].info |= (CAVE_GLOW);
				place_floor(wpos, y, x);
			}
		}
	}
}


/*
 * Store routine for the fractal cave generator
 * this routine probably should be an inline function or a macro
 */
static void store_height(worldpos *wpos, int x, int y, int x0, int y0, byte val, int xhsize, int yhsize, int cutoff) {
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Only write to points that are "blank" */
	if (zcave[y+ y0 - yhsize][x + x0 - xhsize].feat != 255) return;

	 /* If on boundary set val > cutoff so walls are not as square */
	if (((x == 0) || (y == 0) || (x == xhsize * 2) || (y == yhsize * 2)) &&
	    (val <= cutoff)) val = cutoff + 1;

	/* Store the value in height-map format */
	/* Meant to be temporary, hence no cave_set_feat */
	zcave[y + y0 - yhsize][x + x0 - xhsize].feat = val;

	return;
}



/*
 * Explanation of the plasma fractal algorithm:
 *
 * A grid of points is created with the properties of a 'height-map'
 * This is done by making the corners of the grid have a random value.
 * The grid is then subdivided into one with twice the resolution.
 * The new points midway between two 'known' points can be calculated
 * by taking the average value of the 'known' ones and randomly adding
 * or subtracting an amount proportional to the distance between those
 * points.  The final 'middle' points of the grid are then calculated
 * by averaging all four of the originally 'known' corner points.  An
 * random amount is added or subtracted from this to get a value of the
 * height at that point.  The scaling factor here is adjusted to the
 * slightly larger distance diagonally as compared to orthogonally.
 *
 * This is then repeated recursively to fill an entire 'height-map'
 * A rectangular map is done the same way, except there are different
 * scaling factors along the x and y directions.
 *
 * A hack to change the amount of correlation between points is done using
 * the grd variable.  If the current step size is greater than grd then
 * the point will be random, otherwise it will be calculated by the
 * above algorithm.  This makes a maximum distance at which two points on
 * the height map can affect each other.
 *
 * How fractal caves are made:
 *
 * When the map is complete, a cut-off value is used to create a cave.
 * Heights below this value are "floor", and heights above are "wall".
 * This also can be used to create lakes, by adding more height levels
 * representing shallow and deep water/ lava etc.
 *
 * The grd variable affects the width of passages.
 * The roug variable affects the roughness of those passages
 *
 * The tricky part is making sure the created cave is connected.  This
 * is done by 'filling' from the inside and only keeping the 'filled'
 * floor.  Walls bounding the 'filled' floor are also kept.  Everything
 * else is converted to the normal granite FEAT_WALL_EXTRA.
 */


/*
 * Note that this uses the cave.feat array in a very hackish way
 * the values are first set to zero, and then each array location
 * is used as a "heightmap"
 * The heightmap then needs to be converted back into the "feat" format.
 *
 * grd=level at which fractal turns on.  smaller gives more mazelike caves
 * roug=roughness level.  16=normal.  higher values make things more
 * convoluted small values are good for smooth walls.
 * size=length of the side of the square cave system.
 */

static void generate_hmap(worldpos *wpos, int y0, int x0, int xsiz, int ysiz, int grd, int roug, int cutoff) {
	int xhsize, yhsize, xsize, ysize, maxsize;

	/*
	 * fixed point variables- these are stored as 256 x normal value
	 * this gives 8 binary places of fractional part + 8 places of normal part
	 */
	u16b xstep, xhstep, ystep, yhstep, i, j, diagsize, xxsize, yysize;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;


	/* Redefine size so can change the value if out of range */
	xsize = xsiz;
	ysize = ysiz;

	/* Paranoia about size of the system of caves*/
	if (xsize > 254) xsize = 254;
	if (xsize < 4) xsize = 4;
	if (ysize > 254) ysize = 254;
	if (ysize < 4) ysize = 4;

	/* Get offsets to middle of array */
	xhsize = xsize / 2;
	yhsize = ysize / 2;

	/* Fix rounding problem */
	xsize = xhsize * 2;
	ysize = yhsize * 2;

	/*
	 * Scale factor for middle points:
	 * About sqrt(2)*256 - correct for a square lattice
	 * approximately correct for everything else.
	 */
	diagsize = 362;

	/* Maximum of xsize and ysize */
	maxsize = (xsize > ysize) ? xsize : ysize;

	/* Clear the section */
	for (i = 0; i <= xsize; i++) {
		for (j = 0; j <= ysize; j++) {
			cave_type *c_ptr;

			/* Access the grid */
			c_ptr = &zcave[j + y0 - yhsize][i + x0 - xhsize];

			/* 255 is a flag for "not done yet" */
			c_ptr->feat = 255;

			/* Clear icky flag because may be redoing the cave */
			c_ptr->info &= ~(CAVE_ICKY);
		}
	}

	/* Set the corner values just in case grd>size. */
	store_height(wpos, 0, 0, x0, y0, maxsize, xhsize, yhsize, cutoff);
	store_height(wpos, 0, ysize, x0, y0, maxsize, xhsize, yhsize, cutoff);
	store_height(wpos, xsize, 0, x0, y0, maxsize, xhsize, yhsize, cutoff);
	store_height(wpos, xsize, ysize, x0, y0, maxsize, xhsize, yhsize, cutoff);

	/* Set the middle square to be an open area. */
	store_height(wpos, xhsize, yhsize, x0, y0, 0, xhsize, yhsize, cutoff);


	/* Initialise the step sizes */
	xstep = xhstep = xsize*256;
	ystep = yhstep = ysize*256;
	xxsize = xsize*256;
	yysize = ysize*256;

	/*
	 * Fill in the rectangle with fractal height data - like the
	 * 'plasma fractal' in fractint
	 */
	while ((xstep/256 > 1) || (ystep/256 > 1)) {
		/* Halve the step sizes */
		xstep = xhstep;
		xhstep /= 2;
		ystep = yhstep;
		yhstep /= 2;

		/* Middle top to bottom */
		for (i = xhstep; i <= xxsize - xhstep; i += xstep) {
			for (j = 0; j <= yysize; j += ystep) {
				/* If greater than 'grid' level then is random */
				if (xhstep / 256 > grd)
					store_height(wpos, i/256, j/256, x0, y0, randint(maxsize), xhsize, yhsize, cutoff);
				else {
					cave_type *l, *r;
					byte val;

					/* Left point */
					l = &zcave[j/256+y0-yhsize][(i-xhstep)/256+x0-xhsize];

					/* Right point */
					r = &zcave[j/256+y0-yhsize][(i+xhstep)/256+x0-xhsize];

					/* Average of left and right points + random bit */
					val = (l->feat + r->feat) / 2 +
						  (randint(xstep/256) - xhstep/256) * roug / 16;

					store_height(wpos, i/256, j/256, x0, y0, val,
					             xhsize, yhsize, cutoff);
				}
			}
		}

		/* Middle left to right */
		for (j = yhstep; j <= yysize - yhstep; j += ystep) {
			for (i = 0; i <= xxsize; i += xstep) {
				/* If greater than 'grid' level then is random */
				if (xhstep/256 > grd)
					store_height(wpos, i/256, j/256, x0, y0, randint(maxsize), xhsize, yhsize, cutoff);
				else {
					cave_type *u, *d;
					byte val;

					/* Up point */
					u = &zcave[(j-yhstep)/256+y0-yhsize][i/256+x0-xhsize];

					/* Down point */
					d = &zcave[(j+yhstep)/256+y0-yhsize][i/256+x0-xhsize];

					/* Average of up and down points + random bit */
					val = (u->feat + d->feat) / 2 +
						  (randint(ystep/256) - yhstep/256) * roug / 16;

					store_height(wpos, i/256, j/256, x0, y0, val,
					             xhsize, yhsize, cutoff);
				}
			}
		}

		/* Center */
		for (i = xhstep; i <= xxsize - xhstep; i += xstep) {
			for (j = yhstep; j <= yysize - yhstep; j += ystep) {
				/* If greater than 'grid' level then is random */
				if (xhstep/256 > grd)
					store_height(wpos, i/256, j/256, x0, y0, randint(maxsize), xhsize, yhsize, cutoff);
				else {
					cave_type *ul, *dl, *ur, *dr;
					byte val;

					/* Up-left point */
					ul = &zcave[(j-yhstep)/256+y0-yhsize][(i-xhstep)/256+x0-xhsize];

					/* Down-left point */
					dl = &zcave[(j+yhstep)/256+y0-yhsize][(i-xhstep)/256+x0-xhsize];

					/* Up-right point */
					ur = &zcave[(j-yhstep)/256+y0-yhsize][(i+xhstep)/256+x0-xhsize];

					/* Down-right point */
					dr = &zcave[(j+yhstep)/256+y0-yhsize][(i+xhstep)/256+x0-xhsize];

					/*
					 * average over all four corners + scale by diagsize to
					 * reduce the effect of the square grid on the shape
					 * of the fractal
					 */
					val = (ul->feat + dl->feat + ur->feat + dr->feat) / 4 +
					      (randint(xstep/256) - xhstep/256) *
						  (diagsize / 16) / 256 * roug;

					store_height(wpos, i/256, j/256, x0, y0, val,
					             xhsize, yhsize ,cutoff);
				}
			}
		}
	}
}


/*
 * Convert from height-map back to the normal Angband cave format
 */
static bool hack_isnt_wall(worldpos *wpos, int y, int x, int cutoff) {
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return (FALSE);

	/* Already done */
	if (zcave[y][x].info & CAVE_ICKY) return (FALSE);

	/* Show that have looked at this square */
	zcave[y][x].info |= (CAVE_ICKY);

	/* If less than cutoff then is a floor */
	if (zcave[y][x].feat <= cutoff) {
		place_floor(wpos, y, x);
		return (TRUE);
	}
	/* If greater than cutoff then is a wall */
	else {
		cave_set_feat(wpos, y, x, feat_wall_outer);
		return (FALSE);
	}
}


/*
 * Quick and nasty fill routine used to find the connected region
 * of floor in the middle of the cave
 */
static void fill_hack(worldpos *wpos, int y0, int x0, int y, int x, int xsize, int ysize, int cutoff, int *amount) {
	int i, j;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;


	/* check 8 neighbours +self (self is caught in the isnt_wall function) */
	for (i = -1; i <= 1; i++) {
		for (j = -1; j <= 1; j++) {
			/* If within bounds */
			if ((x + i > 0) && (x + i < xsize) &&
			    (y + j > 0) && (y + j < ysize)) {
				/* If not a wall or floor done before */
				if (hack_isnt_wall(wpos, y + j + y0 - ysize / 2, x + i + x0 - xsize / 2, cutoff)) {
					/* then fill from the new point*/
					fill_hack(wpos, y0, x0, y + j, x + i, xsize, ysize, cutoff, amount);

					/* keep tally of size of cave system */
					(*amount)++;
				}
			}

			/* Affect boundary */
			else zcave[y0+y+j-ysize/2][x0+x+i-xsize/2].info |= (CAVE_ICKY);
		}
	}
}


static bool generate_fracave(worldpos *wpos, int y0, int x0, int xsize, int ysize, int cutoff, bool light, bool room) {
	int x, y, i, amount, xhsize, yhsize;
	cave_type *c_ptr;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return(FALSE);

	/* Offsets to middle from corner */
	xhsize = xsize / 2;
	yhsize = ysize / 2;

	/* Reset tally */
	amount = 0;

	/*
	 * Select region connected to center of cave system
	 * this gets rid of alot of isolated one-sqaures that
	 * can make teleport traps instadeaths...
	 */
	fill_hack(wpos, y0, x0, yhsize, xhsize, xsize, ysize, cutoff, &amount);

	/* If tally too small, try again */
	if (amount < 10) {
		/* Too small -- clear area and try again later */
		for (x = 0; x <= xsize; ++x) {
			for (y = 0; y < ysize; ++y) {
				place_filler(wpos, y0+y-yhsize, x0+x-xhsize);
				zcave[y0+y-yhsize][x0+x-xhsize].info &= ~(CAVE_ICKY|CAVE_ROOM);
			}
		}
		return FALSE;
	}


	/*
	 * Do boundaries -- check to see if they are next to a filled region
	 * If not then they are set to normal granite
	 * If so then they are marked as room walls
	 */
	for (i = 0; i <= xsize; ++i) {
		/* Access top boundary grid */
		c_ptr = &zcave[0 + y0 - yhsize][i + x0 - xhsize];

		/* Next to a 'filled' region? -- set to be room walls */
		if (c_ptr->info & CAVE_ICKY) {
			cave_set_feat(wpos, 0+y0-yhsize, i+x0-xhsize, feat_wall_outer);

			if (light) c_ptr->info |= (CAVE_GLOW);
			if (room) c_ptr->info |= (CAVE_ROOM);
			else place_filler(wpos, 0+y0-yhsize, i+x0-xhsize);
		}

		/* Outside of the room -- set to be normal granite */
		else place_filler(wpos, 0+y0-yhsize, i+x0-xhsize);

		/* Clear the icky flag -- don't need it any more */
		c_ptr->info &= ~(CAVE_ICKY);


		/* Access bottom boundary grid */
		c_ptr = &zcave[ysize + y0 - yhsize][i + x0 - xhsize];

		/* Next to a 'filled' region? -- set to be room walls */
		if (c_ptr->info & CAVE_ICKY) {
			cave_set_feat(wpos, ysize+y0-yhsize, i+x0-xhsize, feat_wall_outer);
			if (light) c_ptr->info |= (CAVE_GLOW);
			if (room) c_ptr->info |= (CAVE_ROOM);
			else place_filler(wpos, ysize+y0-yhsize, i+x0-xhsize);
		}

		/* Outside of the room -- set to be normal granite */
		else place_filler(wpos, ysize+y0-yhsize, i+x0-xhsize);

		/* Clear the icky flag -- don't need it any more */
		c_ptr->info &= ~(CAVE_ICKY);
	}


	/* Do the left and right boundaries minus the corners (done above) */
	for (i = 1; i < ysize; ++i) {
		/* Access left boundary grid */
		c_ptr = &zcave[i + y0 - yhsize][0 + x0 - xhsize];

		/* Next to a 'filled' region? -- set to be room walls */
		if (c_ptr->info & CAVE_ICKY) {
			cave_set_feat(wpos, i+y0-yhsize, 0+x0-xhsize, feat_wall_outer);
			if (light) c_ptr->info |= (CAVE_GLOW);
			if (room) c_ptr->info |= (CAVE_ROOM);
			else place_filler(wpos, i+y0-yhsize, 0+x0-xhsize);
		}

		/* Outside of the room -- set to be normal granite */
		else place_filler(wpos, i+y0-yhsize, 0+x0-xhsize);

		/* Clear the icky flag -- don't need it any more */
		c_ptr->info &= ~(CAVE_ICKY);


		/* Access left boundary grid */
		c_ptr = &zcave[i + y0 - yhsize][xsize + x0 - xhsize];

		/* Next to a 'filled' region? -- set to be room walls */
		if (c_ptr->info & CAVE_ICKY) {
			cave_set_feat(wpos, i+y0-yhsize, xsize+x0-xhsize, feat_wall_outer);
			if (light) c_ptr->info |= (CAVE_GLOW);
			if (room) c_ptr->info |= (CAVE_ROOM);
			else place_filler(wpos, i+y0-yhsize, xsize+x0-xhsize);
		}

		/* Outside of the room -- set to be normal granite */
		else place_filler(wpos, i+y0-yhsize, xsize+x0-xhsize);

		/* Clear the icky flag -- don't need it any more */
		c_ptr->info &= ~(CAVE_ICKY);
	}


	/*
	 * Do the rest: convert back to the normal format
	 * In other variants, may want to check to see if cave.feat< some value
	 * if so, set to be water:- this will make interesting pools etc.
	 * (I don't do this for standard Angband.)
	 */
	for (x = 1; x < xsize; ++x) {
		for(y = 1;y < ysize; ++y) {
			/* Access the grid */
			c_ptr = &zcave[y + y0 - yhsize][x + x0 - xhsize];

			/* A floor grid to be converted */
			if ((f_info[c_ptr->feat].flags1 & FF1_FLOOR) && (c_ptr->info & CAVE_ICKY)) {
				/* Clear the icky flag in the filled region */
				c_ptr->info &= ~(CAVE_ICKY);

				/* Set appropriate flags */
				if (light) c_ptr->info |= (CAVE_GLOW);
				if (room) c_ptr->info |= (CAVE_ROOM);
			}

			/* A wall grid to be convereted */
			else if ((c_ptr->feat == feat_wall_outer) && (c_ptr->info & CAVE_ICKY)) {
				/* Clear the icky flag in the filled region */
				c_ptr->info &= ~(CAVE_ICKY);

				/* Set appropriate flags */
				if (light) c_ptr->info |= (CAVE_GLOW);
				if (room) c_ptr->info |= (CAVE_ROOM);
				else place_filler(wpos, y+y0-yhsize, x+x0-xhsize);
			}

			/* None of the above -- clear the unconnected regions */
			else {
				place_filler(wpos, y+y0-yhsize, x+x0-xhsize);
				c_ptr->info &= ~(CAVE_ICKY|CAVE_ROOM);
			}
		}
	}

	/*
	 * XXX XXX XXX There is a slight problem when tunnels pierce the caves:
	 * Extra doors appear inside the system.  (Its not very noticeable though.)
	 * This can be removed by "filling" from the outside in.  This allows
	 * a separation from FEAT_WALL_OUTER with FEAT_WALL_INNER.  (Internal
	 * walls are  F.W.OUTER instead.)
	 * The extra effort for what seems to be only a minor thing (even
	 * non-existant if you think of the caves not as normal rooms, but as
	 * holes in the dungeon), doesn't seem worth it.
	 */

	return (TRUE);
}


/*
 * Makes a cave system in the center of the dungeon
 */
static void build_cavern(worldpos *wpos) {
	int grd, roug, cutoff, xsize, ysize, x0, y0;
	bool done, light, room;
	int dun_lev = getlevel(wpos);

	light = done = room = FALSE;
	if (dun_lev <= randint(25)) light = TRUE;

	/* Make a cave the size of the dungeon */
#if 0
	xsize = cur_wid - 1;
	ysize = cur_hgt - 1;
#endif	/* 0 */
	ysize = dun->l_ptr->hgt - 1;
	xsize = dun->l_ptr->wid - 1;
	x0 = xsize / 2;
	y0 = ysize / 2;

	/* Paranoia: make size even */
	xsize = x0 * 2;
	ysize = y0 * 2;

	while (!done) {
		/* Testing values for these parameters: feel free to adjust */
		grd = 2^(randint(4) + 4);

		/* Want average of about 16 */
		roug =randint(8) * randint(4);

		/* About size/2 */
		cutoff = xsize / 2;

		 /* Make it */
		generate_hmap(wpos, y0, x0, xsize, ysize, grd, roug, cutoff);

		/* Convert to normal format+ clean up*/
		done = generate_fracave(wpos, y0, x0, xsize, ysize, cutoff, light, room);
	}
}


/*
 * Driver routine to create fractal cave system
 */
static void build_type10(worldpos *wpos, int by0, int bx0, player_type *p_ptr) {
	int grd, roug, cutoff, xsize, ysize, y0, x0;

	bool done, light, room;
	int dun_lev = getlevel(wpos);

	/* Get size: note 'Evenness'*/
	xsize = randint(22) * 2 + 6;
	ysize = randint(15) * 2 + 6;

	/* Try to allocate space for room.  If fails, exit */
	if (!room_alloc(wpos, xsize+1, ysize+1, FALSE, by0, bx0, &x0, &y0)) return;

	light = done = FALSE;
	room = TRUE;

	if (dun_lev <= randint(25)) light = TRUE;

	while (!done) {
		/*
		 * Note: size must be even or there are rounding problems
		 * This causes the tunnels not to connect properly to the room
		 */

		/* Testing values for these parameters feel free to adjust */
		grd = 1U << randint(4);

		/* Want average of about 16 */
		roug = randint(8) * randint(4);

		/* About size/2 */
		cutoff = randint(xsize / 4) + randint(ysize / 4) +
		         randint(xsize / 4) + randint(ysize / 4);

		/* Make it */
		generate_hmap(wpos, y0, x0, xsize, ysize, grd, roug, cutoff);

		/* Convert to normal format + clean up*/
		done = generate_fracave(wpos, y0, x0, xsize, ysize, cutoff, light, room);
	}
}


/*
 * Random vault generation from Z 2.5.1
 */

/*
 * Make a very small room centred at (x0, y0)
 *
 * This is used in crypts, and random elemental vaults.
 *
 * Note - this should be used only on allocated regions
 * within another room.
 */
static void build_small_room(worldpos *wpos, int x0, int y0) {
	build_rectangle(wpos, y0 - 1, x0 - 1, y0 + 1, x0 + 1, feat_wall_inner, CAVE_ROOM);

	/* Place a secret door on one side */
	switch (rand_int(4)) {
	case 0:
		place_secret_door(wpos, y0, x0 - 1);
		break;
	case 1:
		place_secret_door(wpos, y0, x0 + 1);
		break;
	case 2:
		place_secret_door(wpos, y0 - 1, x0);
		break;
	case 3:
		place_secret_door(wpos, y0 + 1, x0);
		break;
	}

	/* Add inner open space */
	place_floor(wpos, y0, x0);
}


/*
 * Add a door to a location in a random vault
 *
 * Note that range checking has to be done in the calling routine.
 *
 * The doors must be INSIDE the allocated region.
 */
static void add_door(worldpos *wpos, int x, int y) {
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Need to have a wall in the center square */
	if (zcave[y][x].feat != feat_wall_outer) return;

	/*
	 * Look at:
	 *  x#x
	 *  .#.
	 *  x#x
	 *
	 *  where x=don't care
	 *  .=floor, #=wall
	 */

	if (get_is_floor(wpos, x, y - 1) && get_is_floor(wpos, x, y + 1) &&
	    (zcave[y][x - 1].feat == feat_wall_outer) &&
	    (zcave[y][x + 1].feat == feat_wall_outer))
	{
		/* secret door */
		place_secret_door(wpos, y, x);

		/* set boundarys so don't get wide doors */
		place_filler(wpos, y, x - 1);
		place_filler(wpos, y, x + 1);
	}


	/*
	 * Look at:
	 *  x#x
	 *  .#.
	 *  x#x
	 *
	 *  where x = don't care
	 *  .=floor, #=wall
	 */
	if ((zcave[y - 1][x].feat == feat_wall_outer) &&
	    (zcave[y + 1][x].feat == feat_wall_outer) &&
	    get_is_floor(wpos, x - 1, y) && get_is_floor(wpos, x + 1, y)) {
		/* secret door */
		place_secret_door(wpos, y, x);

		/* set boundarys so don't get wide doors */
		place_filler(wpos, y - 1, x);
		place_filler(wpos, y + 1, x);
	}
}


/*
 * Fill the empty areas of a room with treasure and monsters.
 */
static void fill_treasure(worldpos *wpos, int x1, int x2, int y1, int y2, int difficulty, player_type *p_ptr) {
	int x, y, cx, cy, size;
	s32b value;
	cave_type **zcave;
	int dun_lev = getlevel(wpos);
	bool placed;
	u32b resf = make_resf(p_ptr);
	if (!(zcave = getcave(wpos))) return;


	/* center of room:*/
	cx = (x1 + x2) / 2;
	cy = (y1 + y2) / 2;

	/* Rough measure of size of vault= sum of lengths of sides */
	size = abs(x2 - x1) + abs(y2 - y1);

	for (x = x1; x <= x2; x++) {
		for (y = y1; y <= y2; y++) {
			/*
			 * Thing added based on distance to center of vault
			 * Difficulty is 1-easy to 10-hard
			 */
			value = (((s32b)distance(cy, cx, y, x) * 100) / size) +
			        randint(10) - difficulty;

			/* Hack -- Empty square part of the time */
			if ((randint(100) - difficulty * 3) > 50) value = 20;

			 /* If floor, shallow water or lava */
			if (get_is_floor(wpos, x, y) ||
			    (zcave[y][x].feat == FEAT_DEEP_WATER))

#if 0
			    (cave[y][x].feat == FEAT_SHAL_WATER) ||
			    (cave[y][x].feat == FEAT_SHAL_LAVA))
#endif	/* 0 */
			{
				/* The smaller 'value' is, the better the stuff */
				if (value < 0) {
					/* Meanest monster + treasure */
					monster_level = dun_lev + 40;
					monster_level_min = -1;
					placed = place_monster(wpos, y, x, TRUE, TRUE);
					monster_level_min = 0;
					monster_level = dun_lev;
					object_level = dun_lev + 20;
					if (placed) place_object(wpos, y, x, TRUE, FALSE, FALSE, resf, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
					object_level = dun_lev;
				}
				else if (value < 5) {
					/* Mean monster +treasure */
					monster_level = dun_lev + 20;
					placed = place_monster(wpos, y, x, TRUE, TRUE);
					monster_level = dun_lev;
					object_level = dun_lev + 10;
					if (placed) place_object(wpos, y, x, TRUE, FALSE, FALSE, resf, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
					object_level = dun_lev;
				}
				else if (value < 10) {
					/* Monster */
					monster_level = dun_lev + 9;
					place_monster(wpos, y, x, TRUE, TRUE);
					monster_level = dun_lev;
				}
				else if (value < 17) {
					/* Intentional Blank space */

					/*
					 * (Want some of the vault to be empty
					 * so have room for group monsters.
					 * This is used in the hack above to lower
					 * the density of stuff in the vault.)
					 */
				}
				else if (value < 23) {
					/* Object or trap */
					if (rand_int(100) < 25)
						place_object(wpos, y, x, FALSE, FALSE, FALSE, resf, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
					if (rand_int(100) < 75)
						place_trap(wpos, y, x, 0);
				}
				else if (value < 30) {
					/* Monster and trap */
					monster_level = dun_lev + 5;
					place_monster(wpos, y, x, TRUE, TRUE);
					monster_level = dun_lev;
					place_trap(wpos, y, x, 0);
				}
				else if (value < 40) {
					/* Monster or object */
					placed = TRUE;
					if (rand_int(100) < 50) {
						monster_level = dun_lev + 3;
						placed = place_monster(wpos, y, x, TRUE, TRUE);
						monster_level = dun_lev;
					}
					if (rand_int(100) < 50) {
						object_level = dun_lev + 7;
						if (placed) place_object(wpos, y, x, FALSE, FALSE, FALSE, resf, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
						object_level = dun_lev;
					}
				}
				else if (value < 50) {
					/* Trap */
					place_trap(wpos, y, x, 0);
				}
				else {
					/* Various Stuff */

					/* 20% monster, 40% trap, 20% object, 20% blank space */
					if (rand_int(100) < 20)
						place_monster(wpos, y, x, TRUE, TRUE);
					else if (rand_int(100) < 50)
						place_trap(wpos, y, x, 0);
					else if (rand_int(100) < 50)
						place_object(wpos, y, x, FALSE, FALSE, FALSE, resf, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
				}

			}
		}
	}
}


/*
 * Creates a random vault that looks like a collection of bubbles
 *
 * It works by getting a set of coordinates that represent the center of
 * each bubble.  The entire room is made by seeing which bubble center is
 * closest. If two centers are equidistant then the square is a wall,
 * otherwise it is a floor. The only exception is for squares really
 * near a center, these are always floor.
 * (It looks better than without this check.)
 *
 * Note: If two centers are on the same point then this algorithm will create a
 * blank bubble filled with walls. - This is prevented from happening.
 */

#define BUBBLENUM 10 /* number of bubbles */

static void build_bubble_vault(worldpos *wpos, int x0, int y0, int xsize, int ysize, player_type *p_ptr) {
	/* array of center points of bubbles */
	coord center[BUBBLENUM];

	int i, j, k, x = 0, y = 0;
	u16b min1, min2, temp;
	bool done;
	dun_level *l_ptr = getfloor(wpos);

	/* Offset from center to top left hand corner */
	int xhsize = xsize / 2;
	int yhsize = ysize / 2;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Allocate center of bubbles */
	center[0].x = randint(xsize - 3) + 1;
	center[0].y = randint(ysize - 3) + 1;

	for (i = 1; i < BUBBLENUM; i++) {
		done = FALSE;

		/* Get center and check to see if it is unique */
		for (k = 0; !done && (k < 2000); k++) {
			done = TRUE;

			x = randint(xsize - 3) + 1;
			y = randint(ysize - 3) + 1;

			for (j = 0; j < i; j++) {
				/* Rough test to see if there is an overlap */
				if ((x == center[j].x) || (y == center[j].y)) done = FALSE;
			}
		}

		/* Too many failures */
		if (k >= 2000) return;

		center[i].x = x;
		center[i].y = y;
	}

	build_rectangle(wpos, y0 - yhsize, x0 - xhsize,
	                y0 - yhsize + ysize - 1, x0 - xhsize + xsize - 1,
	                feat_wall_outer, CAVE_ROOM | CAVE_ICKY);

	/* Fill in middle with bubbles */
	for (x = 1; x < xsize - 1; x++) {
		for (y = 1; y < ysize - 1; y++) {
			cave_type *c_ptr;

			/* Get distances to two closest centers */

			/* Initialise */
			min1 = distance(y, x, center[0].y, center[0].x);
			min2 = distance(y, x, center[1].y, center[1].x);

			if (min1 > min2) {
				/* Swap if in wrong order */
				temp = min1;
				min1 = min2;
				min2 = temp;
			}

			/* Scan the rest */
			for (i = 2; i < BUBBLENUM; i++) {
				temp = distance(y, x, center[i].y, center[i].x);

				if (temp < min1) {
					/* Smallest */
					min2 = min1;
					min1 = temp;
				}
				else if (temp < min2)
					/* Second smallest */
					min2 = temp;
			}

			/* Access the grid */
			c_ptr = &zcave[y + y0 - yhsize][x + x0 - xhsize];

			/*
			 * Boundary at midpoint+ not at inner region of bubble
			 *
			 * SCSCSC: was feat_wall_outer
			 */
			if (((min2 - min1) <= 2) && (!(min1 < 3)))
				place_filler(wpos, y+y0-yhsize, x+x0-xhsize);
			/* Middle of a bubble */
			else
				place_floor(wpos, y+y0-yhsize, x+x0-xhsize);

			/* Clean up rest of flags */
			c_ptr->info |= (CAVE_ROOM | CAVE_ICKY);
		}
	}

	/* Try to add some random doors */
	for (i = 0; i < 500; i++)
	{
		x = randint(xsize - 3) - xhsize + x0 + 1;
		y = randint(ysize - 3) - yhsize + y0 + 1;
		add_door(wpos, x, y);
	}

	/* Fill with monsters and treasure, low difficulty */
	fill_treasure(wpos, x0 - xhsize + 1, x0 - xhsize + xsize - 2,
	              y0 - yhsize + 1, y0 - yhsize + ysize - 2, randint(5), p_ptr);

	l_ptr->flags2 |= LF2_VAULT;
}


/*
 * Convert FEAT_WALL_EXTRA (used by random vaults) to normal dungeon wall
 */
static void convert_extra(worldpos *wpos, int y1, int x1, int y2, int x2) {
	int x, y;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	for (x = x1; x <= x2; x++) {
		for (y = y1; y <= y2; y++) {
			/* CRASHES occured here, so maybe in_bound helps */
			if (!in_bounds(y, x)) continue;

			if (zcave[y][x].feat == feat_wall_outer)
				place_filler(wpos, y, x);
		}
	}
}


/*
 * Overlay a rectangular room given its bounds
 *
 * This routine is used by build_room_vault (hence FEAT_WALL_OUTER)
 * The area inside the walls is not touched: only granite is removed
 * and normal walls stay
 */
static void build_room(worldpos *wpos, int x1, int x2, int y1, int y2) {
	int x, y, xsize, ysize, temp;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Check if rectangle has no width */
	if ((x1 == x2) || (y1 == y2)) return;

	/* initialize */
	if (x1 > x2) {
		/* Swap boundaries if in wrong order */
		temp = x1;
		x1 = x2;
		x2 = temp;
	}

	if (y1 > y2) {
		/* Swap boundaries if in wrong order */
		temp = y1;
		y1 = y2;
		y2 = temp;
	}

	/* Get total widths */
	xsize = x2 - x1;
	ysize = y2 - y1;

	build_rectangle(wpos, y1, x1, y2, x2, feat_wall_outer, CAVE_ROOM | CAVE_ICKY);

	/* Middle */
	for (x = 1; x < xsize; x++) {
		for (y = 1; y < ysize; y++) {
			if (zcave[y1 + y][x1 + x].feat == feat_wall_outer) {
				/* Clear the untouched region */
				place_floor(wpos, y1 + y, x1 + x);
				zcave[y1 + y][x1 + x].info |= (CAVE_ROOM | CAVE_ICKY);
			} else {
				/* Make it a room -- but don't touch */
				zcave[y1 + y][x1 + x].info |= (CAVE_ROOM | CAVE_ICKY);
			}
		}
	}
}


/*
 * Create a random vault that looks like a collection of overlapping rooms
 */
static void build_room_vault(worldpos *wpos, int x0, int y0, int xsize, int ysize, player_type *p_ptr) {
	int i, x1, x2, y1, y2, xhsize, yhsize;
	dun_level *l_ptr = getfloor(wpos);
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Get offset from center */
	xhsize = xsize / 2;
	yhsize = ysize / 2;

	/* Fill area so don't get problems with arena levels */
	for (x1 = 0; x1 <= xsize; x1++) {
		int x = x0 - xhsize + x1;

		for (y1 = 0; y1 <= ysize; y1++) {
			int y = y0 - yhsize + y1;

			if (!in_bounds(y, x)) continue;

			cave_set_feat(wpos, y, x, feat_wall_outer);
			zcave[y][x].info &= ~(CAVE_ICKY);
		}
	}

	/* Add ten random rooms */
	for (i = 0; i < 10; i++) {
		x1 = randint(xhsize) * 2 + x0 - xhsize;
		x2 = randint(xhsize) * 2 + x0 - xhsize;
		y1 = randint(yhsize) * 2 + y0 - yhsize;
		y2 = randint(yhsize) * 2 + y0 - yhsize;

		build_room(wpos, x1, x2, y1, y2);
	}

	convert_extra(wpos, y0 - yhsize, x0 - xhsize, y0 - yhsize + ysize,
		      x0 - xhsize + xsize);

	/* Add some random doors */
	for (i = 0; i < 500; i++) {
		x1 = randint(xsize - 2) - xhsize + x0 + 1;
		y1 = randint(ysize - 2) - yhsize + y0 + 1;
		add_door(wpos, x1, y1);
	}

	/* Fill with monsters and treasure, high difficulty */
	fill_treasure(wpos, x0 - xhsize + 1, x0 - xhsize + xsize - 1,
	              y0 - yhsize + 1, y0 - yhsize + ysize - 1, randint(5) + 5, p_ptr);

	l_ptr->flags2 |= LF2_VAULT_HI;
#ifdef TEST_SERVER
s_printf("DEBUG_FEELING: VAULT_HI by build_room_vault\n");
#endif
}


/*
 * Create a random vault out of a fractal cave
 */
static void build_cave_vault(worldpos *wpos, int x0, int y0, int xsiz, int ysiz, player_type *p_ptr) {
	int grd, roug, cutoff, xhsize, yhsize, xsize, ysize, x, y;
	bool done, light, room;
	dun_level *l_ptr = getfloor(wpos);
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Round to make sizes even */
	xhsize = xsiz / 2;
	yhsize = ysiz / 2;
	xsize = xhsize * 2;
	ysize = yhsize * 2;

	light = done = FALSE;
	room = TRUE;

	while (!done) {
		/* Testing values for these parameters feel free to adjust */
		grd = 2^rand_int(4);

		/* Want average of about 16 */
		roug = randint(8) * randint(4);

		/* About size/2 */
		cutoff = randint(xsize / 4) + randint(ysize / 4) +
			 randint(xsize / 4) + randint(ysize / 4);

		/* Make it */
		generate_hmap(wpos, y0, x0, xsize, ysize, grd, roug, cutoff);

		/* Convert to normal format + clean up */
		done = generate_fracave(wpos, y0, x0, xsize, ysize, cutoff, light, room);
	}

	/* Set icky flag because is a vault */
	for (x = 0; x <= xsize; x++)
		for (y = 0; y <= ysize; y++)
			zcave[y0 - yhsize + y][x0 - xhsize + x].info |= CAVE_ICKY;

	/* Fill with monsters and treasure, low difficulty */
	fill_treasure(wpos, x0 - xhsize + 1, x0 - xhsize + xsize - 1,
	              y0 - yhsize + 1, y0 - yhsize + ysize - 1, randint(5), p_ptr);

	l_ptr->flags2 |= LF2_VAULT;
}


/*
 * Maze vault -- rectangular labyrinthine rooms
 *
 * maze vault uses two routines:
 *    r_visit - a recursive routine that builds the labyrinth
 *    build_maze_vault - a driver routine that calls r_visit and adds
 *                   monsters, traps and treasure
 *
 * The labyrinth is built by creating a spanning tree of a graph.
 * The graph vertices are at
 *    (x, y) = (2j + x1, 2k + y1)   j = 0,...,m-1    k = 0,...,n-1
 * and the edges are the vertical and horizontal nearest neighbors.
 *
 * The spanning tree is created by performing a suitably randomized
 * depth-first traversal of the graph. The only adjustable parameter
 * is the rand_int(3) below; it governs the relative density of
 * twists and turns in the labyrinth: smaller number, more twists.
 */
static void r_visit(worldpos *wpos, int y1, int x1, int y2, int x2, int node, int dir, int *visited) {
	int i, j, m, n, temp, x, y, adj[4];

	/* Dimensions of vertex array */
	m = (x2 - x1) / 2 + 1;
	n = (y2 - y1) / 2 + 1;

	/* Mark node visited and set it to a floor */
	visited[node] = 1;
	x = 2 * (node % m) + x1;
	y = 2 * (node / m) + y1;
	place_floor(wpos, y, x);

	/* Setup order of adjacent node visits */
	if (rand_int(3) == 0) {
		/* Pick a random ordering */
		for (i = 0; i < 4; i++)
			adj[i] = i;
		for (i = 0; i < 4; i++) {
			j = rand_int(4);
			temp = adj[i];
			adj[i] = adj[j];
			adj[j] = temp;
		}
		dir = adj[0];
	} else {
		/* Pick a random ordering with dir first */
		adj[0] = dir;
		for (i = 1; i < 4; i++)
			adj[i] = i;
		for (i = 1; i < 4; i++) {
			j = 1 + rand_int(3);
			temp = adj[i];
			adj[i] = adj[j];
			adj[j] = temp;
		}
	}

	for (i = 0; i < 4; i++)
		switch (adj[i]) {
		/* (0,+) - check for bottom boundary */
		case 0:
			if ((node / m < n - 1) && (visited[node + m] == 0)) {
				place_floor(wpos, y + 1, x);
				r_visit(wpos, y1, x1, y2, x2, node + m, dir, visited);
			}
			break;
		/* (0,-) - check for top boundary */
		case 1:
			if ((node / m > 0) && (visited[node - m] == 0)) {
				place_floor(wpos, y - 1, x);
				r_visit(wpos, y1, x1, y2, x2, node - m, dir, visited);
			}
			break;
		/* (+,0) - check for right boundary */
		case 2:
			if ((node % m < m - 1) && (visited[node + 1] == 0)) {
				place_floor(wpos, y, x + 1);
				r_visit(wpos, y1, x1, y2, x2, node + 1, dir, visited);
			}
			break;
		/* (-,0) - check for left boundary */
		case 3:
			if ((node % m > 0) && (visited[node - 1] == 0)) {
				place_floor(wpos, y, x - 1);
				r_visit(wpos, y1, x1, y2, x2, node - 1, dir, visited);
			}
			break;
		}
}


static void build_maze_vault(worldpos *wpos, int x0, int y0, int xsize, int ysize, player_type *p_ptr) {
	int y, x, dy, dx;
	int y1, x1, y2, x2;
	int m, n, num_vertices, *visited;
	bool light;
	cave_type *c_ptr;
	dun_level *l_ptr = getfloor(wpos);
	cave_type **zcave;
	int dun_lev = getlevel(wpos);
	if (!(zcave = getcave(wpos))) return;

	/* Choose lite or dark */
	light = (dun_lev <= randint(25));

	/* Pick a random room size - randomized by calling routine */
	dy = ysize / 2 - 1;
	dx = xsize / 2 - 1;

	y1 = y0 - dy;
	x1 = x0 - dx;
	y2 = y0 + dy;
	x2 = x0 + dx;

	/* Generate the room */
	for (y = y1 - 1; y <= y2 + 1; y++) {
		for (x = x1 - 1; x <= x2 + 1; x++) {
			c_ptr = &zcave[y][x];

			c_ptr->info |= (CAVE_ROOM | CAVE_ICKY);

			if ((x == x1 - 1) || (x == x2 + 1) ||
			    (y == y1 - 1) || (y == y2 + 1))
				cave_set_feat(wpos, y, x, feat_wall_outer);
			else
				cave_set_feat(wpos, y, x, feat_wall_inner);
			if (light) c_ptr->info |= (CAVE_GLOW);
		}
	}

	/* Dimensions of vertex array */
	m = dx + 1;
	n = dy + 1;
	num_vertices = m * n;

	/* Allocate an array for visited vertices */
	C_MAKE(visited, num_vertices, int);

	/* Traverse the graph to create a spaning tree, pick a random root */
	r_visit(wpos, y1, x1, y2, x2, rand_int(num_vertices), 0, visited);

	/* Fill with monsters and treasure, low difficulty */
	fill_treasure(wpos, x1, x2, y1, y2, randint(5), p_ptr);

	l_ptr->flags2 |= LF2_VAULT;

	/* Free the array for visited vertices */
	C_FREE(visited, num_vertices, int);
}


/*
 * Build a "mini" checkerboard vault
 *
 * This is done by making a permanent wall maze and setting
 * the diagonal sqaures of the checker board to be granite.
 * The vault has two entrances on opposite sides to guarantee
 * a way to get in even if the vault abuts a side of the dungeon.
 */
static void build_mini_c_vault(worldpos *wpos, int x0, int y0, int xsize, int ysize, player_type *p_ptr) {
	int dy, dx;
	int y1, x1, y2, x2, y, x, total;
	int m, n, num_vertices;
	int *visited;
	dun_level *l_ptr = getfloor(wpos);
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Pick a random room size */
	dy = ysize / 2 - 1;
	dx = xsize / 2 - 1;

	y1 = y0 - dy;
	x1 = x0 - dx;
	y2 = y0 + dy;
	x2 = x0 + dx;


	/* Generate the room */
	for (y = y1 - 1; y <= y2 + 1; y++)
		for (x = x1 - 1; x <= x2 + 1; x++) {
			zcave[y][x].info |= (CAVE_ROOM | CAVE_ICKY | CAVE_ICKY_PERMA);

			/* Permanent walls */
			cave_set_feat(wpos, y, x, FEAT_PERM_INNER);
		}


	/* Dimensions of vertex array */
	m = dx + 1;
	n = dy + 1;
	num_vertices = m * n;

	/* Allocate an array for visited vertices */
	C_MAKE(visited, num_vertices, int);

	/* Traverse the graph to create a spannng tree, pick a random root */
	r_visit(wpos, y1, x1, y2, x2, rand_int(num_vertices), 0, visited);

	/* Make it look like a checker board vault */
	for (x = x1; x <= x2; x++)
		for (y = y1; y <= y2; y++) {
			total = x - x1 + y - y1;

			/* If total is odd and is a floor, then make a wall */
			if ((total % 2 == 1) && get_is_floor(wpos, x, y))
				cave_set_feat(wpos, y, x, feat_wall_inner);
		}

	/* Make a couple of entrances */
	if (rand_int(2) == 0) {
		/* Left and right */
		y = randint(dy) + dy / 2;
		cave_set_feat(wpos, y1 + y, x1 - 1, feat_wall_outer);
		cave_set_feat(wpos, y1 + y, x2 + 1, feat_wall_outer);
	} else {
		/* Top and bottom */
		x = randint(dx) + dx / 2;
		cave_set_feat(wpos, y1 - 1, x1 + x, feat_wall_outer);
		cave_set_feat(wpos, y2 + 1, x1 + x, feat_wall_outer);
	}

	/* Fill with monsters and treasure, highest difficulty */
	fill_treasure(wpos, x1, x2, y1, y2, 10, p_ptr);

	l_ptr->flags2 |= LF2_VAULT_HI;
#ifdef TEST_SERVER
s_printf("DEBUG_FEELING: VAULT_HI by build_mini_c(heckerboard)_vault\n");
#endif

	/* Free the array for visited vertices */
	C_FREE(visited, num_vertices, int);
}


/*
 * Build a town/ castle by using a recursive algorithm.
 * Basically divide each region in a probalistic way to create
 * smaller regions.  When the regions get too small stop.
 *
 * The power variable is a measure of how well defended a region is.
 * This alters the possible choices.
 */
static void build_recursive_room(worldpos *wpos, int x1, int y1, int x2, int y2, int power) {
	int xsize, ysize;
	int x, y;
	int choice;

	/* Temp variables */
	int t1, t2, t3, t4;

	xsize = x2 - x1;
	ysize = y2 - y1;

	if ((power < 3) && (xsize > 12) && (ysize > 12)) {
		/* Need outside wall +keep */
		choice = 1;
	} else {
		if (power < 10) {
			/* Make rooms + subdivide */
			if ((randint(10) > 2) && (xsize < 8) && (ysize < 8))
				choice = 4;
			else
				choice = randint(2) + 1;
		} else {
			/* Mostly subdivide */
			choice = randint(3) + 1;
		}
	}

	/* Based on the choice made above, do something */
	switch (choice) {
	/* Outer walls */
	case 1:
		/* Top and bottom */
		for (x = x1; x <= x2; x++) {
			cave_set_feat(wpos, y1, x, feat_wall_outer);
			cave_set_feat(wpos, y2, x, feat_wall_outer);
		}

		/* Left and right */
		for (y = y1 + 1; y < y2; y++) {
			cave_set_feat(wpos, y, x1, feat_wall_outer);
			cave_set_feat(wpos, y, x2, feat_wall_outer);
		}

		/* Make a couple of entrances */
		if (rand_int(2) == 0) {
			/* Left and right */
			y = randint(ysize) + y1;
			place_floor(wpos, y, x1);
			place_floor(wpos, y, x2);
		} else {
			/* Top and bottom */
			x = randint(xsize) + x1;
			place_floor(wpos, y1, x);
			place_floor(wpos, y2, x);
		}

		/* Select size of keep */
		t1 = randint(ysize / 3) + y1;
		t2 = y2 - randint(ysize / 3);
		t3 = randint(xsize / 3) + x1;
		t4 = x2 - randint(xsize / 3);

		/* Do outside areas */

		/* Above and below keep */
		build_recursive_room(wpos, x1 + 1, y1 + 1, x2 - 1, t1, power + 1);
		build_recursive_room(wpos, x1 + 1, t2, x2 - 1, y2, power + 1);

		/* Left and right of keep */
		build_recursive_room(wpos, x1 + 1, t1 + 1, t3, t2 - 1, power + 3);
		build_recursive_room(wpos, t4, t1 + 1, x2 - 1, t2 - 1, power + 3);

		/* Make the keep itself: */
		x1 = t3;
		x2 = t4;
		y1 = t1;
		y2 = t2;
		xsize = x2 - x1;
		ysize = y2 - y1;
		power += 2;

		/* Fall through */

	/* Try to build a room */
	case 4:
		if ((xsize < 3) || (ysize < 3)) {
			for (y = y1; y < y2; y++)
				for (x = x1; x < x2; x++)
					cave_set_feat(wpos, y, x, feat_wall_inner);

			/* Too small */
			return;
		}

		/* Make outside walls */

		/* Top and bottom */
		for (x = x1 + 1; x <= x2 - 1; x++) {
			cave_set_feat(wpos, y1 + 1, x, feat_wall_inner);
			cave_set_feat(wpos, y2 - 1, x, feat_wall_inner);
		}

		/* Left and right */
		for (y = y1 + 1; y <= y2 - 1; y++) {
			cave_set_feat(wpos, y, x1 + 1, feat_wall_inner);
			cave_set_feat(wpos, y, x2 - 1, feat_wall_inner);
		}

		/* Make a door */
		y = randint(ysize - 3) + y1 + 1;

		if (rand_int(2) == 0)
			/* Left */
			place_floor(wpos, y, x1 + 1);
		else
			/* Right */
			place_floor(wpos, y, x2 - 1);

		/* Build the room */
		build_recursive_room(wpos, x1 + 2, y1 + 2, x2 - 2, y2 - 2, power + 3);
		break;

	/* Try and divide vertically */
	case 2:
		if (xsize < 3) {
			/* Too small */
			for (y = y1; y < y2; y++)
				for (x = x1; x < x2; x++)
					cave_set_feat(wpos, y, x, feat_wall_inner);
			return;
		}

		t1 = randint(xsize - 2) + x1 + 1;
		build_recursive_room(wpos, x1, y1, t1, y2, power - 2);
		build_recursive_room(wpos, t1 + 1, y1, x2, y2, power - 2);
		break;

	/* Try and divide horizontally */
	case 3:
		if (ysize < 3) {
			/* Too small */
			for (y = y1; y < y2; y++)
				for (x = x1; x < x2; x++)
					cave_set_feat(wpos, y, x, feat_wall_inner);
			return;
		}

		t1 = randint(ysize - 2) + y1 + 1;
		build_recursive_room(wpos, x1, y1, x2, t1, power - 2);
		build_recursive_room(wpos, x1, t1 + 1, x2, y2, power - 2);
		break;
	}
}


/*
 * Build a castle
 *
 * Clear the region and call the recursive room routine.
 *
 * This makes a vault that looks like a castle or city in the dungeon.
 */
static void build_castle_vault(worldpos *wpos, int x0, int y0, int xsize, int ysize, player_type *p_ptr) {
	int dy, dx;
	int y1, x1, y2, x2;
	int y, x;
	dun_level *l_ptr = getfloor(wpos);
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Pick a random room size */
	dy = ysize / 2 - 1;
	dx = xsize / 2 - 1;

	y1 = y0 - dy;
	x1 = x0 - dx;
	y2 = y0 + dy;
	x2 = x0 + dx;

	/* Generate the room */
	for (y = y1 - 1; y <= y2 + 1; y++)
		for (x = x1 - 1; x <= x2 + 1; x++) {
			zcave[y][x].info |= (CAVE_ROOM | CAVE_ICKY);

			/* Make everything a floor */
			place_floor(wpos, y, x);
		}

	/* Make the castle */
	build_recursive_room(wpos, x1, y1, x2, y2, randint(5));

	/* Fill with monsters and treasure, low difficulty */
	fill_treasure(wpos, x1, x2, y1, y2, randint(3), p_ptr);

	l_ptr->flags2 |= LF2_VAULT;
}


/*
 * Add outer wall to a floored region
 *
 * Note: no range checking is done so must be inside dungeon
 * This routine also stomps on doors
 */
static void add_outer_wall(worldpos *wpos, int x, int y, int light, int x1, int y1, int x2, int y2) {
	int i, j;
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return;
	if (!in_bounds(y, x)) return;	/* XXX */

	/*
	 * Hack -- Check to see if square has been visited before
	 * if so, then exit (use room flag to do this)
	 */
	if (zcave[y][x].info & CAVE_ROOM) return;

	/* Set room flag */
	zcave[y][x].info |= (CAVE_ROOM);

	if (get_is_floor(wpos, x, y)) {
		for (i = -1; i <= 1; i++)
			for (j = -1; j <= 1; j++)
				if ((x + i >= x1) && (x + i <= x2) && (y + j >= y1) && (y + j <= y2)) {
					add_outer_wall(wpos, x + i, y + j, light, x1, y1, x2, y2);
					if (light) zcave[y][x].info |= CAVE_GLOW; //BUGBUGBUG? +i
				}
	}

	/* Set bounding walls */
	else if (zcave[y][x].feat == FEAT_WALL_EXTRA) {
		zcave[y][x].feat = feat_wall_outer;
		if (light == TRUE) zcave[y][x].info |= CAVE_GLOW;
	}
	/* Set bounding walls */
	else if (zcave[y][x].feat == FEAT_PERM_OUTER) {
		if (light == TRUE) zcave[y][x].info |= CAVE_GLOW;
	}
}


/*
 * Hacked distance formula - gives the 'wrong' answer
 *
 * Used to build crypts
 */
static int dist2(int x1, int y1, int x2, int y2,
	int h1, int h2, int h3, int h4)
{
	int dx, dy;
	dx = abs(x2 - x1);
	dy = abs(y2 - y1);

	/*
	 * Basically this works by taking the normal pythagorean formula
	 * and using an expansion to express this in a way without the
	 * square root.  This approximate formula is then perturbed to give
	 * the distorted results.  (I found this by making a mistake when I was
	 * trying to fix the circular rooms.)
	 */

	/* h1-h4 are constants that describe the metric */
	if (dx >= 2 * dy) return (dx + (dy * h1) / h2);
	if (dy >= 2 * dx) return (dy + (dx * h1) / h2);

	/* 128/181 is approx. 1/sqrt(2) */
	return (((dx + dy) * 128) / 181 +
		(dx * dx / (dy * h3) + dy * dy / (dx * h3)) * h4);
}


/*
 * Build target vault
 *
 * This is made by two concentric "crypts" with perpendicular
 * walls creating the cross-hairs.
 */
static void build_target_vault(worldpos *wpos, int x0, int y0, int xsize, int ysize, player_type *p_ptr) {
	int rad, x, y;
	int h1, h2, h3, h4;
	dun_level *l_ptr = getfloor(wpos);
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;


	/* Make a random metric */
	h1 = randint(32) - 16;
	h2 = randint(16);
	h3 = randint(32);
	h4 = randint(32) - 16;

	/* Work out outer radius */
	if (xsize > ysize) rad = ysize / 2;
	else rad = xsize / 2;

	/* Make floor */
	for (x = x0 - rad; x <= x0 + rad; x++) {
		for (y = y0 - rad; y <= y0 + rad; y++) {
			cave_type *c_ptr;

			/* Access the grid */
			c_ptr = &zcave[y][x];

			/* Clear room flag */
			c_ptr->info &= ~(CAVE_ROOM);

			/* Grids in vaults are required to be "icky" */
			c_ptr->info |= (CAVE_ICKY);

			/* Inside -- floor */
			if (dist2(y0, x0, y, x, h1, h2, h3, h4) <= rad - 1) place_floor(wpos, y, x);
			/* Outside -- make it granite so that arena works */
			else c_ptr->feat = FEAT_WALL_EXTRA;

			/* Proper boundary for arena */
			if (((y + rad) == y0) || ((y - rad) == y0) ||
			    ((x + rad) == x0) || ((x - rad) == x0))
				cave_set_feat(wpos, y, x, feat_wall_outer);
		}
	}

	/* Find visible outer walls and set to be FEAT_OUTER */
	add_outer_wall(wpos, x0, y0, FALSE, x0 - rad - 1, y0 - rad - 1,
                   x0 + rad + 1, y0 + rad + 1);

	/* Add inner wall */
	for (x = x0 - rad / 2; x <= x0 + rad / 2; x++)
		for (y = y0 - rad / 2; y <= y0 + rad / 2; y++)
			if (dist2(y0, x0, y, x, h1, h2, h3, h4) == rad / 2) {
				/* Make an internal wall */
				cave_set_feat(wpos, y, x, feat_wall_inner);
			}

	/* Add perpendicular walls */
	for (x = x0 - rad; x <= x0 + rad; x++)
		cave_set_feat(wpos, y0, x, feat_wall_inner);

	for (y = y0 - rad; y <= y0 + rad; y++)
		cave_set_feat(wpos, y, x0, feat_wall_inner);

	/* Make inner vault */
	for (y = y0 - 1; y <= y0 + 1; y++) {
		cave_set_feat(wpos, y, x0 - 1, feat_wall_inner);
		cave_set_feat(wpos, y, x0 + 1, feat_wall_inner);
	}
	for (x = x0 - 1; x <= x0 + 1; x++) {
		cave_set_feat(wpos, y0 - 1, x, feat_wall_inner);
		cave_set_feat(wpos, y0 + 1, x, feat_wall_inner);
	}

	place_floor(wpos, y0, x0);


	/*
	 * Add doors to vault
	 *
	 * Get two distances so can place doors relative to centre
	 */
	x = (rad - 2) / 4 + 1;
	y = rad / 2 + x;

	add_door(wpos, x0 + x, y0);
	add_door(wpos, x0 + y, y0);
	add_door(wpos, x0 - x, y0);
	add_door(wpos, x0 - y, y0);
	add_door(wpos, x0, y0 + x);
	add_door(wpos, x0, y0 + y);
	add_door(wpos, x0, y0 - x);
	add_door(wpos, x0, y0 - y);

	/* Fill with stuff - medium difficulty */
	fill_treasure(wpos, x0 - rad, x0 + rad, y0 - rad, y0 + rad, randint(3) + 3, p_ptr);

	l_ptr->flags2 |= LF2_VAULT;
}


/*
 * Random vaults
 */
static void build_type11(worldpos *wpos, int by0, int bx0, player_type *p_ptr) {
	int y0, x0, xsize, ysize, vtype;

	cave_type **zcave;
	int dun_lev = getlevel(wpos);
	if (!(zcave = getcave(wpos))) return;

	/* Get size -- gig enough to look good, small enough to be fairly common */
	xsize = randint(22) + 22;
	ysize = randint(11) + 11;

	/* Allocate in room_map.  If will not fit, exit */
	if (!room_alloc(wpos, xsize + 1, ysize + 1, FALSE, by0, bx0, &x0, &y0)) return;

	/* (Sometimes) Cause a special feeling */
	if ((dun_lev <= 50) ||
	    (randint((dun_lev - 40) * (dun_lev - 40) + 1) < 400))
		good_item_flag = TRUE;

	/* Select type of vault */
	vtype = randint(8);

	switch (vtype) {
	/* Build an appropriate room */
	case 1:
		build_bubble_vault(wpos, x0, y0, xsize, ysize, p_ptr);
		break;
	case 2:
		build_room_vault(wpos, x0, y0, xsize, ysize, p_ptr);
		break;
	case 3:
		build_cave_vault(wpos, x0, y0, xsize, ysize, p_ptr);
		break;
	case 4:
		build_maze_vault(wpos, x0, y0, xsize, ysize, p_ptr);
		break;
	case 5:
		build_mini_c_vault(wpos, x0, y0, xsize, ysize, p_ptr);
		break;
	case 6:
		build_castle_vault(wpos, x0, y0, xsize, ysize, p_ptr);
		break;
	case 7:
		build_target_vault(wpos, x0, y0, xsize, ysize, p_ptr);
		break;
	/* I know how to add a few more... give me some time. */

	/* Paranoia */
	default:
		return;
	}
}


/*
 * Crypt room generation from Z 2.5.1
 */

/*
 * Build crypt room.
 * For every grid in the possible square, check the (fake) distance.
 * If it's less than the radius, make it a room square.
 *
 * When done fill from the inside to find the walls,
 */
static void build_type12(worldpos *wpos, int by0, int bx0, player_type *p_ptr) {
	int rad, x, y, x0, y0;
	int light = FALSE;
	bool emptyflag = TRUE;
	int h1, h2, h3, h4;

	cave_type **zcave;
	int dun_lev = getlevel(wpos);
	if (!(zcave = getcave(wpos))) return;

	/* Make a random metric */
	h1 = randint(32) - 16;
	h2 = randint(16);
	h3 = randint(32);
	h4 = randint(32) - 16;

	/* Occasional light */
	if (randint(dun_lev) <= 5) light = TRUE;

	rad = randint(9);

	/* Allocate in room_map.  If will not fit, exit */
	if (!room_alloc(wpos, rad * 2 + 3, rad * 2 + 3, FALSE, by0, bx0, &x0, &y0)) return;

	/* Make floor */
	for (x = x0 - rad; x <= x0 + rad; x++) {
		for (y = y0 - rad; y <= y0 + rad; y++) {
			/* Clear room flag */
			zcave[y][x].info &= ~(CAVE_ROOM);

			/* Inside -- floor */
			if (dist2(y0, x0, y, x, h1, h2, h3, h4) <= rad - 1) place_floor(wpos, y, x);
			else if (distance(y0, x0, y, x) < 3) place_floor(wpos, y, x);
			/* Outside -- make it granite so that arena works */
			else cave_set_feat(wpos, y, x, feat_wall_outer);

			/* Proper boundary for arena */
			if (((y + rad) == y0) || ((y - rad) == y0) ||
			    ((x + rad) == x0) || ((x - rad) == x0))
				cave_set_feat(wpos, y, x, feat_wall_outer);
		}
	}

	/* Find visible outer walls and set to be FEAT_OUTER */
	add_outer_wall(wpos, x0, y0, light, x0 - rad - 1, y0 - rad - 1,
		       x0 + rad + 1, y0 + rad + 1);

	/* Check to see if there is room for an inner vault */
	for (x = x0 - 2; x <= x0 + 2; x++)
		for (y = y0 - 2; y <= y0 + 2; y++)
			if (!get_is_floor(wpos, x, y)) {
				/* Wall in the way */
				emptyflag = FALSE;
			}

	if (emptyflag && (rand_int(2) == 0)) {
		/* Build the vault */
		build_small_room(wpos, x0, y0);

		/* Place a treasure in the vault */
		place_object(wpos, y0, x0, FALSE, FALSE, FALSE, make_resf(p_ptr), default_obj_theme, 0, ITEM_REMOVAL_NEVER);

		/* Let's guard the treasure well */
		vault_monsters(wpos, y0, x0, rand_int(2) + 3);

		/* Traps naturally */
		vault_traps(wpos, y0, x0, 4, 4, rand_int(3) + 2);
	}
}

/*
 * Maze dungeon generator
 */

/*
 * If we wasted static memory for this, it would look like:
 *
 * static char maze[(MAX_HGT / 2) + 2][(MAX_WID / 2) + 2];
 */
typedef char maze_row[(MAX_WID / 2) + 2];

static void dig(maze_row *maze, int y, int x, int d) {
	int k;
	int dy = 0, dx = 0;

	/*
	 * first, open the wall of the new cell
	 * in the direction we come from.
	 */
	switch (d) {
	case 0:
		maze[y][x] |= 4;
		break;
	case 1:
		maze[y][x] |= 8;
		break;
	case 2:
		maze[y][x] |= 1;
		break;
	case 3:
		maze[y][x] |= 2;
		break;
	}

	/*
	 * try to chage direction, here 50% times.
	 * with smaller values (say 25%) the maze
	 * is made of long straight corridors. with
	 * greaters values (say 75%) the maze is
	 * very "turny".
	 */
	if (rand_range(1, 100) < 50) d = rand_range(0, 3);

	for (k = 1; k <= 4; k++) {
		switch (d) {
		case 0:
			dy = 0;
			dx = 1;
			break;
		case 1:
			dy = -1;
			dx = 0;
			break;
		case 2:
			dy = 0;
			dx = -1;
			break;
		case 3:
			dy = 1;
			dx = 0;
			break;
		}

		if (maze[y + dy][x + dx] == 0) {
			/*
			 * now, open the wall of the new cell
			 * in the direction we go to.
			 */
			switch (d) {
			case 0:
				maze[y][x] |= 1;
				break;
			case 1:
				maze[y][x] |= 2;
				break;
			case 2:
				maze[y][x] |= 4;
				break;
			case 3:
				maze[y][x] |= 8;
				break;
			}

			dig(maze, y + dy, x + dx, d);
		}

		d = (d + 1) % 4;
	}
}


/* methinks it's not always necessary that the entire level is 'maze'? */
static void generate_maze(worldpos *wpos, int corridor) {
	int i, j, d;
	int y, dy = 0;
	int x, dx = 0;
	int m_1 = 0, m_2 = 0;
	maze_row *maze;

	int cur_hgt = dun->l_ptr->hgt;
	int cur_wid = dun->l_ptr->wid;
	int div = corridor + 1;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;


	/* Allocate temporary memory */
	C_MAKE(maze, (MAX_HGT / 2) + 2, maze_row);

	/*
	 * the empty maze is:
	 *
	 * -1 -1 ... -1 -1
	 * -1  0      0 -1
	 *  .            .
	 *  .            .
	 * -1  0      0 -1
	 * -1 -1 ... -1 -1
	 *
	 *  -1 are so-called "sentinel value".
	 *   0 are empty cells.
	 *
	 *  walls are not represented, only cells.
	 *  at the end of the algorithm each cell
	 *  contains a value that is bit mask
	 *  representing surrounding walls:
	 *
	 *         bit #1
	 *
	 *        +------+
	 *        |      |
	 * bit #2 |      | bit #0
	 *        |      |
	 *        +------+
	 *
	 *         bit #3
	 *
	 * d is the direction you are digging
	 * to. d value is the bit number:
	 * d=0 --> go east
	 * d=1 --> go north
	 * etc
	 *
	 * you need only 4 bits per cell.
	 * this gives you a very compact
	 * maze representation.
	 *
	 */
	for (j = 0; j <= (cur_hgt / div) + 1; j++)
		for (i = 0; i <= (cur_wid / div) + 1; i++)
			maze[j][i] = -1;

	for (j = 1;j <= (cur_hgt / div); j++)
		for (i = 1; i <= (cur_wid / div); i++)
			maze[j][i] = 0;

	y = rand_range(1, (cur_hgt / div));
	x = rand_range(1, (cur_wid / div));
	d = rand_range(0, 3);

	dig(maze, y, x, d);

	maze[y][x] = 0;

	for (d = 0; d <= 3; d++) {
		switch (d) {
		case 0:
			dy = 0;
			dx = 1;
			m_1 = 1;
			m_2 = 4;
			break;
		case 1:
			dy = -1;
			dx = 0;
			m_1 = 2;
			m_2 = 8;
			break;
		case 2:
			dy = 0;
			dx = -1;
			m_1 = 4;
			m_2 = 1;
			break;
		case 3:
			dy = 1;
			dx = 0;
			m_1 = 8;
			m_2 = 2;
			break;
		}

		if ((maze[y + dy][x + dx] != -1) &&
		    ((maze[y + dy][x + dx] & m_2) != 0))
			maze[y][x] |= m_1;
	}

	/* Translate the maze bit array into a real dungeon map -- DG */
	for (j = 1; j <= (cur_hgt / div) - 2; j++)
		for (i = 1; i <= (cur_wid / div) - 2; i++)
			for (dx = 0; dx < corridor; dx++)
				for (dy = 0; dy < corridor; dy++) {
					if (maze[j][i])
						place_floor_respectedly(wpos, j * div + dy, i * div + dx);

					if (maze[j][i] & 1)
						place_floor_respectedly(wpos, j * div + dy, i * div + dx + corridor);

					if (maze[j][i] & 8)
						place_floor_respectedly(wpos, j * div + dy + corridor, i * div + dx);
				}

	/* Free temporary memory */
	C_KILL(maze, (MAX_HGT / 2) + 2, maze_row);
}


#if 0	/* this would make a good bottleneck */
/*
 * Generate a game of life level :) and make it evolve
 */
void evolve_level(worldpos *wpos, bool noise) {
	int i, j;
	int cw = 0, cf = 0;

	/* Add a bit of noise */
	if (noise) {
		for (i = 1; i < cur_wid - 1; i++)
			for (j = 1; j < cur_hgt - 1; j++) {
				if (f_info[cave[j][i].feat].flags1 & FF1_WALL) cw++;
				if (f_info[cave[j][i].feat].flags1 & FF1_FLOOR) cf++;
			}

		for (i = 1; i < cur_wid - 1; i++) {
			for (j = 1; j < cur_hgt - 1; j++) {
				cave_type *c_ptr;

				/* Access the grid */
				c_ptr = &cave[j][i];

				/* Permanent features should stay */
				if (f_info[c_ptr->feat].flags1 & FF1_PERMANENT) continue;

				/* Avoid evolving grids with object or monster */
				if (c_ptr->o_idx || c_ptr->m_idx) continue;

				/* Avoid evolving player grid */
				if ((j == py) && (i == px)) continue;

				if (magik(7)) {
					if (cw > cf) place_floor(j, i);
					else place_filler(j, i);
				}
			}
		}
	}

	for (i = 1; i < cur_wid - 1; i++) {
		for (j = 1; j < cur_hgt - 1; j++) {
			int x, y, c;
			cave_type *c_ptr;

			/* Access the grid */
			c_ptr = &cave[j][i];

			/* Permanent features should stay */
			if (f_info[c_ptr->feat].flags1 & FF1_PERMANENT) continue;

			/* Avoid evolving grids with object or monster */
			if (c_ptr->o_idx || c_ptr->m_idx) continue;

			/* Avoid evolving player grid */
			if ((j == py) && (i == px)) continue;


			/* Reset tally */
			c = 0;

			/* Count number of surrounding walls */
			for (x = i - 1; x <= i + 1; x++)
				for (y = j - 1; y <= j + 1; y++) {
					if ((x == i) && (y == j)) continue;
					if (f_info[cave[y][x].feat].flags1 & FF1_WALL) c++;
				}

			/*
			 * Changed these parameters a bit, so that it doesn't produce
			 * too open or too narrow levels -- pelpel
			 */
			/* Starved or suffocated */
			if ((c < 4) || (c >= 7)) {
				if (f_info[c_ptr->feat].flags1 & FF1_WALL)
					place_floor(j, i);
			}
			/* Spawned */
			else if ((c == 4) || (c == 5)) {
				if (!(f_info[c_ptr->feat].flags1 & FF1_WALL))
					place_filler(j, i);
			}
		}
	}

	/* Notice changes */
	p_ptr->update |= (PU_VIEW | PU_MONSTERS | PU_FLOW | PU_MON_LITE);
}


static void gen_life_level(worldpos *wpos) {
	int i, j;

	for (i = 1; i < cur_wid - 1; i++)
		for (j = 1; j < cur_hgt - 1; j++) {
			cave[j][i].info = (CAVE_ROOM | CAVE_GLOW | CAVE_MARK);
			if (magik(45)) place_floor(j, i);
			else place_filler(j, i);
		}

	evolve_level(FALSE);
	evolve_level(FALSE);
	evolve_level(FALSE);
}
#endif	/* 0 */
/* XXX XXX Here ends the big lump of ToME cake */


/* Copy (y, x) feat(usually door) to (y2,x2), and trap it if needed.
 * - Jir - */
static void duplicate_door(worldpos *wpos, int y, int x, int y2, int x2) {
#ifdef WIDE_CORRIDORS
	int tmp;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Place the same type of door */
	zcave[y2][x2].feat = zcave[y][x].feat;

	/* let's trap this too ;) */
	if ((tmp = getlevel(wpos)) <= COMFORT_PASSAGE_DEPTH ||
			rand_int(TRAPPED_DOOR_RATE) > tmp + TRAPPED_DOOR_BASE) return;
	place_trap(wpos, y2, x2, 0);
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
static void build_tunnel(struct worldpos *wpos, int row1, int col1, int row2, int col2) {
	int			i, y, x, tmp;
	int			tmp_row, tmp_col;
	int                 row_dir, col_dir;
	int                 start_row, start_col;
	int			main_loop_count = 0;

	bool		door_flag = FALSE;
	cave_type		*c_ptr;

	dungeon_type	*d_ptr = getdungeon(wpos);
	int dun_type;
#ifdef IRONDEEPDIVE_MIXED_TYPES
	if (in_irondeepdive(wpos)) dun_type = iddc[ABS(wpos->wz)].type;
	else
#endif
	dun_type = d_ptr->theme ? d_ptr->theme : d_ptr->type;

#if 0
	dungeon_info_type *di_ptr = &d_info[dun_type];
#endif

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Reset the arrays */
	dun->tunn_n = 0;
	dun->wall_n = 0;

	/* Save the starting location */
	start_row = row1;
	start_col = col1;

	/* Start out in the correct direction */
	correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

	/* Keep going until done (or bored) */
	while ((row1 != row2) || (col1 != col2)) {
		/* Mega-Hack -- Paranoia -- prevent infinite loops */
		if (main_loop_count++ > 2000) break;

		/* Allow bends in the tunnel */
		if (rand_int(100) < DUN_TUN_CHG) {
			/* Acquire the correct direction */
			correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

			/* Random direction */
			if (rand_int(100) < DUN_TUN_RND)
				rand_dir(&row_dir, &col_dir);
		}

		/* Get the next location */
		tmp_row = row1 + row_dir;
		tmp_col = col1 + col_dir;


		/* Extremely Important -- do not leave the dungeon */
		while (!in_bounds(tmp_row, tmp_col)) {
			/* Acquire the correct direction */
			correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

			/* Random direction */
			if (rand_int(100) < DUN_TUN_RND)
				rand_dir(&row_dir, &col_dir);

			/* Get the next location */
			tmp_row = row1 + row_dir;
			tmp_col = col1 + col_dir;
		}


		/* Access the location */
		c_ptr = &zcave[tmp_row][tmp_col];

		/* Avoid the edge of the dungeon */
		if ((f_info[c_ptr->feat].flags2 & FF2_BOUNDARY)) continue;

		/* Avoid the edge of vaults */
		if (c_ptr->feat == FEAT_PERM_OUTER) continue;

		/* Avoid "solid" granite walls */
		if (c_ptr->feat == FEAT_WALL_SOLID) continue;

		/*
		 * Pierce "outer" walls of rooms
		 * Cannot trust feat code any longer...
		 */
		if ((c_ptr->feat == feat_wall_outer) &&
		    (c_ptr->info & CAVE_ROOM)) {
			/* Acquire the "next" location */
			y = tmp_row + row_dir;
			x = tmp_col + col_dir;

			/* Hack -- Avoid outer/solid permanent walls */
			if (zcave[y][x].feat == FEAT_PERM_OUTER) continue;
			if ((f_info[zcave[y][x].feat].flags2 & FF2_BOUNDARY)) continue;

			/* Hack -- Avoid outer/solid granite walls */
			if ((zcave[y][x].feat == feat_wall_outer) &&
			    (zcave[y][x].info & CAVE_ROOM)) continue;
			if (zcave[y][x].feat == FEAT_WALL_SOLID) continue;

			/* Accept this location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Save the wall location */
			if (dun->wall_n < WALL_MAX) {
				dun->wall[dun->wall_n].y = row1;
				dun->wall[dun->wall_n].x = col1;
				dun->wall_n++;
			}

#ifdef WIDE_CORRIDORS
			/* Save the wall location */
			if (dun->wall_n < WALL_MAX) {
				if (!(f_info[zcave[row1 + col_dir][col1 + row_dir].feat].flags2 & FF2_BOUNDARY) &&
				    !(f_info[zcave[row1 + col_dir][col1 + row_dir].feat].flags2 & FF2_BOUNDARY))
				{
					dun->wall[dun->wall_n].y = row1 + col_dir;
					dun->wall[dun->wall_n].x = col1 + row_dir;
					dun->wall_n++;
				} else {
					dun->wall[dun->wall_n].y = row1;
					dun->wall[dun->wall_n].x = col1;
					dun->wall_n++;
				}
			}
#endif

#ifdef WIDE_CORRIDORS
			/* Forbid re-entry near this piercing */
			for (y = row1 - 2; y <= row1 + 2; y++) {
				for (x = col1 - 2; x <= col1 + 2; x++) {
					/* Be sure we are "in bounds" */
					if (!in_bounds(y, x)) continue;

					/* Convert adjacent "outer" walls as "solid" walls */
					if (zcave[y][x].feat == feat_wall_outer) {
						/* Change the wall to a "solid" wall */
						zcave[y][x].feat = FEAT_WALL_SOLID;
					}
				}
			}
#else
			/* Forbid re-entry near this piercing */
			for (y = row1 - 1; y <= row1 + 1; y++) {
				for (x = col1 - 1; x <= col1 + 1; x++) {
					/* Convert adjacent "outer" walls as "solid" walls */
					if (zcave[y][x].feat == feat_wall_outer) {
						/* Change the wall to a "solid" wall */
						zcave[y][x].feat = FEAT_WALL_SOLID;
					}
				}
			}
#endif
		}

		/* Travel quickly through rooms */
		else if (c_ptr->info & CAVE_ROOM) {
			/* Accept the location */
			row1 = tmp_row;
			col1 = tmp_col;
		}

		/* Tunnel through all other walls */
		else if ((c_ptr->feat == d_info[dun_type].fill_type[0]) ||
		         (c_ptr->feat == d_info[dun_type].fill_type[1]) ||
		         (c_ptr->feat == d_info[dun_type].fill_type[2]) ||
#ifdef IRONDEEPDIVE_MIXED_TYPES
			 (c_ptr->feat == d_info[((in_irondeepdive(wpos) && iddc[ABS(wpos->wz)].step > 0) ? iddc[ABS(wpos->wz)].next : dun_type)].fill_type[3]) ||
		         (c_ptr->feat == d_info[((in_irondeepdive(wpos) && iddc[ABS(wpos->wz)].step > 1) ? iddc[ABS(wpos->wz)].next : dun_type)].fill_type[4])
#else
		         (c_ptr->feat == d_info[dun_type].fill_type[3]) ||
		         (c_ptr->feat == d_info[dun_type].fill_type[4])
#endif
		         ) {
			/* Accept this location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Save the tunnel location */
			if (dun->tunn_n < TUNN_MAX) {
				dun->tunn[dun->tunn_n].y = row1;
				dun->tunn[dun->tunn_n].x = col1;
				dun->tunn_n++;
			}

#ifdef WIDE_CORRIDORS
			/* Note that this is incredibly hacked */

			/* Make sure we're in bounds */
			if (in_bounds(row1 + col_dir, col1 + row_dir)) {
				/* New spot to be hollowed out */
				c_ptr = &zcave[row1 + col_dir][col1 + row_dir];

				/* Make sure it's a wall we want to tunnel */
				if ((c_ptr->feat == d_info[dun_type].fill_type[0]) ||
				    (c_ptr->feat == d_info[dun_type].fill_type[1]) ||
				    (c_ptr->feat == d_info[dun_type].fill_type[2]) ||
#ifdef IRONDEEPDIVE_MIXED_TYPES
				    (c_ptr->feat == d_info[((in_irondeepdive(wpos) && iddc[ABS(wpos->wz)].step > 0) ? iddc[ABS(wpos->wz)].next : dun_type)].fill_type[3]) ||
				    (c_ptr->feat == d_info[((in_irondeepdive(wpos) && iddc[ABS(wpos->wz)].step > 1) ? iddc[ABS(wpos->wz)].next : dun_type)].fill_type[4]) ||
#else
				    (c_ptr->feat == d_info[dun_type].fill_type[3]) ||
				    (c_ptr->feat == d_info[dun_type].fill_type[4]) ||
#endif
				    c_ptr->feat == FEAT_WALL_EXTRA) {
					/* Save the tunnel location */
					if (dun->tunn_n < TUNN_MAX) {
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
		else {
			/* Accept the location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Collect legal door locations */
			if (!door_flag) {
				/* Save the door location */
				if (dun->door_n < DOOR_MAX) {
					dun->door[dun->door_n].y = row1;
					dun->door[dun->door_n].x = col1;
					dun->door_n++;
				}

#ifdef WIDE_CORRIDORS
#if 0
				/* Save the next door location */
				if (dun->door_n < DOOR_MAX) {
					if (in_bounds(row1 + col_dir, col1 + row_dir)) {
						dun->door[dun->door_n].y = row1 + col_dir;
						dun->door[dun->door_n].x = col1 + row_dir;
						dun->door_n++;
					}
					
					/* Hack -- Duplicate the previous door */
					else {
						dun->door[dun->door_n].y = row1;
						dun->door[dun->door_n].x = col1;
						dun->door_n++;
					}
				}
#endif	/* 0 */
#endif

				/* No door in next grid */
				door_flag = TRUE;
			}

			/* Hack -- allow pre-emptive tunnel termination */
			if (rand_int(100) >= DUN_TUN_CON) {
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
	for (i = 0; i < dun->tunn_n; i++) {
		/* Access the grid */
		y = dun->tunn[i].y;
		x = dun->tunn[i].x;

		/* Access the grid */
		c_ptr = &zcave[y][x];

		/* Clear previous contents, add a floor */
		place_floor(wpos, y, x);
	}


	/* Apply the piercings that we found */
	for (i = 0; i < dun->wall_n; i++) {
		/* Access the grid */
		y = dun->wall[i].y;
		x = dun->wall[i].x;

		/* Access the grid */
		c_ptr = &zcave[y][x];

		/* Clear previous contents, add up floor */
		place_floor(wpos, y, x);

		/* Occasional doorway */
		/* Some dungeons don't have doors at all */
		if ((
#ifdef IRONDEEPDIVE_MIXED_TYPES
		    in_irondeepdive(wpos) ? (!(d_info[iddc[ABS(wpos->wz)].type].flags1 & (DF1_NO_DOORS))) :
#endif
		    (!(d_ptr->flags1 & (DF1_NO_DOORS)))) &&
		    rand_int(100) < DUN_TUN_PEN) {
#ifdef WIDE_CORRIDORS
			/* hack: prepare 2nd door part for door_makes_no_sense() check */
			int x2, y2;

			/* Make sure both halves get a door */
			if (i % 2) {
				/* Access the grid */
				y2 = dun->wall[i - 1].y;
				x2 = dun->wall[i - 1].x;
			} else {
				/* Access the grid */
				y2 = dun->wall[i + 1].y;
				x2 = dun->wall[i + 1].x;

				/* Increment counter */
				i++;
			}

 #ifdef ENABLE_DOOR_CHECK
			/* hack! provisional feat */
			zcave[y2][x2].feat = FEAT_WALL_EXTRA;
 #endif
#endif

			/* Place a random door */
			place_random_door(wpos, y, x);

#ifdef WIDE_CORRIDORS
			/* Place the same type of door */
			zcave[y2][x2].feat = zcave[y][x].feat;

			/* let's trap this too ;) */
			if ((tmp = getlevel(wpos)) <= COMFORT_PASSAGE_DEPTH ||
			    rand_int(TRAPPED_DOOR_RATE) > tmp + TRAPPED_DOOR_BASE) continue;
			place_trap(wpos, y2, x2, 0);
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
static int next_to_corr(struct worldpos *wpos, int y1, int x1) {
	int i, y, x, k = 0;
	cave_type *c_ptr;
	dungeon_type *dt_ptr = getdungeon(wpos);
	int dun_type;
	cave_type **zcave;

#ifdef IRONDEEPDIVE_MIXED_TYPES
	if (in_irondeepdive(wpos)) dun_type = iddc[ABS(wpos->wz)].type;
	else
#endif
	dun_type = dt_ptr->theme ? dt_ptr->theme : dt_ptr->type;

	if (!(zcave = getcave(wpos))) return(FALSE);

	/* Scan adjacent grids */
	for (i = 0; i < 4; i++) {
		/* Extract the location */
		y = y1 + ddy_ddd[i];
		x = x1 + ddx_ddd[i];

		/* Skip non floors */
		if (!cave_floor_bold(zcave, y, x)) continue;
		/* Access the grid */
		c_ptr = &zcave[y][x];

		/* Skip non "empty floor" grids */
		if ((c_ptr->feat != d_info[dun_type].floor[0]) &&
		    (c_ptr->feat != d_info[dun_type].floor[1]) &&
		    (c_ptr->feat != d_info[dun_type].floor[2]) &&
#ifdef IRONDEEPDIVE_MIXED_TYPES
		    (c_ptr->feat != d_info[((in_irondeepdive(wpos) && iddc[ABS(wpos->wz)].step > 0) ? iddc[ABS(wpos->wz)].next : dun_type)].floor[3]) &&
		    (c_ptr->feat != d_info[((in_irondeepdive(wpos) && iddc[ABS(wpos->wz)].step > 1) ? iddc[ABS(wpos->wz)].next : dun_type)].floor[4]))
#else
		    (c_ptr->feat != d_info[dun_type].floor[3]) &&
		    (c_ptr->feat != d_info[dun_type].floor[4]))
#endif
			continue;

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
#if 0
static bool possible_doorway(struct worldpos *wpos, int y, int x)
{
        cave_type **zcave;
        if (!(zcave = getcave(wpos))) return(FALSE);

        /* Count the adjacent corridors */
        if (next_to_corr(wpos, y, x) >= 2)
        {
                /* Check Vertical */
                if ((zcave[y-1][x].feat >= FEAT_MAGMA) &&
                    (zcave[y+1][x].feat >= FEAT_MAGMA))
                {
                        return (TRUE);
                }

                /* Check Horizontal */
                if ((zcave[y][x-1].feat >= FEAT_MAGMA) &&
                    (zcave[y][x+1].feat >= FEAT_MAGMA))
                {
                        return (TRUE);
                }
        }

        /* No doorway */
        return (FALSE);
}
#else	/* 0 */
static int possible_doorway(struct worldpos *wpos, int y, int x)
{
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return(FALSE);

	/* Count the adjacent corridors */
	if (next_to_corr(wpos, y, x) >= 2)
	{
		/* Hack -- avoid doors next to doors */
		if (is_door(zcave[y-1][x].feat) ||
			is_door(zcave[y+1][x].feat) ||
			is_door(zcave[y][x-1].feat) ||
			is_door(zcave[y][x+1].feat))
			return (-1);

		/* Check Vertical */
		if ((zcave[y-1][x].feat >= FEAT_MAGMA) &&
		    (zcave[y+1][x].feat >= FEAT_MAGMA))
		{
			return (8);
		}
#if 1
		if (in_bounds(y-2, x) &&
			(zcave[y-2][x].feat >= FEAT_MAGMA) &&
		    (zcave[y+1][x].feat >= FEAT_MAGMA))
		{
			return (1);
		}
		if (in_bounds(y+2, x) &&
			(zcave[y-1][x].feat >= FEAT_MAGMA) &&
		    (zcave[y+2][x].feat >= FEAT_MAGMA))
		{
			return (0);
		}
#endif	/* 0 */

		/* Check Horizontal */
		if ((zcave[y][x-1].feat >= FEAT_MAGMA) &&
		    (zcave[y][x+1].feat >= FEAT_MAGMA))
		{
			return (8);
		}
#if 1
		if (in_bounds(y, x-2) &&
			(zcave[y][x-2].feat >= FEAT_MAGMA) &&
		    (zcave[y][x+1].feat >= FEAT_MAGMA))
		{
			return (3);
		}
		if (in_bounds(y, x+2) &&
			(zcave[y][x-1].feat >= FEAT_MAGMA) &&
		    (zcave[y][x+2].feat >= FEAT_MAGMA))
		{
			return (2);
		}
#endif	/* 0 */
	}

	/* No doorway */
	return (-1);
}
#endif	/* 0 */


#if 0
/*
 * Places door at y, x position if at least 2 walls found
 */
static void try_door(struct worldpos *wpos, int y, int x)
{
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Paranoia */
	if (!in_bounds(y, x)) return;

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
}
#endif	/* 0 */

/*
 * Places doors around y, x position
 */
static void try_doors(worldpos *wpos, int y, int x) {
	bool dir_ok[4];
	int dir_next[4];
	int i, k, n;
	int yy, xx;

	dungeon_type	*d_ptr = getdungeon(wpos);
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Some dungeons don't have doors at all */
	if (
#ifdef IRONDEEPDIVE_MIXED_TYPES
	    in_irondeepdive(wpos) ? (d_info[iddc[ABS(wpos->wz)].type].flags1 & (DF1_NO_DOORS)) :
#endif
	    (d_ptr->flags1 & (DF1_NO_DOORS)))
		return;

	/* Reset tally */
	n = 0;

	/* Look four cardinal directions */
	for (i = 0; i < 4; i++) {
		/* Assume NG */
		dir_ok[i] = FALSE;

		/* Access location */
		yy = y + ddy_ddd[i];
		xx = x + ddx_ddd[i];

		/* Out of level boundary */
		if (!in_bounds(yy, xx)) continue;

		/* Ignore walls */
		if (zcave[y][x].feat >= FEAT_MAGMA
		    || zcave[y][x].feat == FEAT_PERM_CLEAR) /* paranoia for now (dungeon floors don't have this feat) */
			continue;

		/* Ignore room grids */
		if (zcave[y][x].info & CAVE_ROOM) continue;

		/* Not a doorway */
		if ((dir_next[i] = possible_doorway(wpos, yy, xx)) < 0) continue;

		/* Accept the direction */
		dir_ok[i] = TRUE;

		/* Count good spots */
		n++;
	}

	/* Use the traditional method 75% of time */
	if (rand_int(100) < 75) {
		for (i = 0; i < 4; i++) {
			/* Bad locations */
			if (!dir_ok[i]) continue;

			/* Place one of various kinds of doors */
			if (rand_int(100) < DUN_TUN_JCT) {
				/* Access location */
				yy = y + ddy_ddd[i];
				xx = x + ddx_ddd[i];

#ifdef ENABLE_DOOR_CHECK
				/* hack for door_makes_no_sense() check */
				zcave[yy + ddy_ddd[dir_next[i]]][xx + ddx_ddd[dir_next[i]]].feat = FEAT_WALL_EXTRA;
#endif

				/* Place a door */
				place_random_door(wpos, yy, xx);

				duplicate_door(wpos, yy, xx, yy + ddy_ddd[dir_next[i]], xx + ddx_ddd[dir_next[i]]);
			}
		}
	}

	/* Use alternative method */
	else {
		/* A crossroad */
		if (n == 4) {
			/* Clear OK flags XXX */
			for (i = 0; i < 4; i++) dir_ok[i] = FALSE;

			/* Put one or two secret doors */
			dir_ok[rand_int(4)] = TRUE;
			dir_ok[rand_int(4)] = TRUE;
		}

		/* A T-shaped intersection or two possible doorways */
		else if ((n == 3) || (n == 2)) {
			/* Pick one random location from the list */
			k = rand_int(n);

			for (i = 0; i < 4; i++) {
				/* Reject all but k'th OK direction */
				if (dir_ok[i] && (k-- != 0)) dir_ok[i] = FALSE;
			}
		}

		/* Place secret door(s) */
		for (i = 0; i < 4; i++) {
			/* Bad location */
			if (!dir_ok[i]) continue;

			/* Access location */
			yy = y + ddy_ddd[i];
			xx = x + ddx_ddd[i];

#ifdef ENABLE_DOOR_CHECK
			/* hack for door_makes_no_sense() check */
			zcave[yy + ddy_ddd[dir_next[i]]][xx + ddx_ddd[dir_next[i]]].feat = FEAT_WALL_EXTRA;
#endif

			/* Place a secret door */
			place_secret_door(wpos, yy, xx);

			duplicate_door(wpos, yy, xx, yy + ddy_ddd[dir_next[i]], xx + ddx_ddd[dir_next[i]]);
		}
	}
}




/*
 * Attempt to build a room of the given type at the given block
 *
 * Note that we restrict the number of "crowded" rooms to reduce
 * the chance of overflowing the monster list during level creation.
 */
static bool room_build(struct worldpos *wpos, int y, int x, int typ, player_type *p_ptr) {
	dungeon_type *d_ptr = getdungeon(wpos);

	/* Restrict level */
	if (getlevel(wpos) < room[typ].level) return (FALSE);

	/* Restrict "crowded" rooms */
	if (dun->crowded && ((typ == 5) || (typ == 6))) return (FALSE);

	/* Less rooms/vaults in some dungeons? */
	if (d_ptr) {
		if ((
#ifdef IRONDEEPDIVE_MIXED_TYPES
		    in_irondeepdive(wpos) ? (d_info[iddc[ABS(wpos->wz)].type].flags3 & DF3_NO_VAULTS) :
#endif
		    (d_ptr->flags3 & DF3_NO_VAULTS))
		    && (typ == 7 || typ == 8 || typ == 11))
			return FALSE;

		if ((
#ifdef IRONDEEPDIVE_MIXED_TYPES
		    in_irondeepdive(wpos) ? (d_info[iddc[ABS(wpos->wz)].type].flags3 & DF3_FEW_ROOMS) :
#endif
		    (d_ptr->flags3 & DF3_FEW_ROOMS))
		    && rand_int(20))
			return FALSE;

#ifdef IDDC_FEWER_VAULTS
		if (in_irondeepdive(wpos) && rand_int(4) &&
		    (typ == 7 || typ == 8 || typ == 11))
			return FALSE;
#endif
	}

#if 0
	/* Extract blocks */
	y1 = y0 + room[typ].dy1;
	y2 = y0 + room[typ].dy2;
	x1 = x0 + room[typ].dx1;
	x2 = x0 + room[typ].dx2;

	/* Never run off the screen */
	if ((y1 < 0) || (y2 >= dun->row_rooms)) return (FALSE);
	if ((x1 < 0) || (x2 >= dun->col_rooms)) return (FALSE);

	/* Verify open space */
	for (y = y1; y <= y2; y++) {
		for (x = x1; x <= x2; x++) {
			if (dun->room_map[y][x]) return (FALSE);
		}
	}

	/* XXX XXX XXX It is *extremely* important that the following */
	/* calculation is *exactly* correct to prevent memory errors */

	/* Acquire the location of the room */
	y = ((y1 + y2 + 1) * BLOCK_HGT) / 2;
	x = ((x1 + x2 + 1) * BLOCK_WID) / 2;
#endif	/* 0 */

	/* Build a room */
	switch (typ) {
	/* Build an appropriate room */
	case 12: build_type12(wpos, y, x, p_ptr); break;
	case 11: build_type11(wpos, y, x, p_ptr); break;
	case 10: build_type10(wpos, y, x, p_ptr); break;
	case  9: build_type9 (wpos, y, x, p_ptr); break;
	case  8: build_type8 (wpos, y, x, p_ptr); break;
	case  7: build_type7 (wpos, y, x, p_ptr); break;
	case  6: build_type6 (wpos, y, x, p_ptr); break;
	case  5: build_type5 (wpos, y, x, p_ptr); break;
	case  4: build_type4 (wpos, y, x, p_ptr); break;
	case  3: build_type3 (wpos, y, x, p_ptr); break;
	case  2: build_type2 (wpos, y, x, p_ptr); break;
	case  1: build_type1 (wpos, y, x, p_ptr); break;

	/* Paranoia */
	default: return (FALSE);
	}

#if 0
	/* Save the room location */
	if (dun->cent_n < CENT_MAX) {
		dun->cent[dun->cent_n].y = y;
		dun->cent[dun->cent_n].x = x;
		dun->cent_n++;
	}

	/* Reserve some blocks */
	for (y = y1; y <= y2; y++) {
		for (x = x1; x <= x2; x++) {
			dun->room_map[y][x] = TRUE;
		}
	}

	/* Count "crowded" rooms */
	if ((typ == 5) || (typ == 6)) dun->crowded = TRUE;
#endif	/* 0 */

	/* Success */
	return (TRUE);
}


/*
 * Build probability tables for walls and floors and set feat_wall_outer
 * and feat_wall_inner according to the current information in d_info.txt
 *
 * *hint* *hint* with this made extern, and we no longer have to
 * store fill_type and floor_type in the savefile...
 */
static void init_feat_info(worldpos *wpos) {
	int i, j;
	int cur_depth, max_depth;
	int p1, p2;
	int floor_lim[5];
	int fill_lim[5];

	int dun_lev = getlevel(wpos);
	dungeon_type *dt_ptr = getdungeon(wpos);
	int dun_type;
#ifdef IRONDEEPDIVE_MIXED_TYPES
	if (in_irondeepdive(wpos)) dun_type = iddc[ABS(wpos->wz)].type;
	else
#endif
	dun_type = dt_ptr->theme ? dt_ptr->theme : dt_ptr->type;

	dungeon_info_type *d_ptr = &d_info[dun_type];
	dun_level *l_ptr = getfloor(wpos); /* for DIGGING */

	/* Default dungeon? */
	if (!dun_type) {
		feat_wall_outer = FEAT_WALL_OUTER;	/* Outer wall of rooms */
		feat_wall_inner = FEAT_WALL_INNER;	/* Inner wall of rooms */

		for (i = 0; i < 1000; i++) {
			floor_type[i] = FEAT_FLOOR;
			fill_type[i] = FEAT_WALL_EXTRA;	/* Dungeon filler */
		}

		return;
	}

	/* Retrieve dungeon depth info (base 1, to avoid zero divide errors) */
#ifdef IRONDEEPDIVE_MIXED_TYPES
	if (in_irondeepdive(wpos)) {
		cur_depth = dun_lev;
		max_depth = d_ptr->maxdepth + 2; //hardcode (ew, fix for the 2 transitions levels, so max > cur in probability calcs)
	} else
#endif
	{
		cur_depth = (dun_lev - d_ptr->mindepth) + 1;
		max_depth = (d_ptr->maxdepth - d_ptr->mindepth) + 1;
	}

	/* Set room wall types */
	feat_wall_outer = d_ptr->outer_wall;
	feat_wall_inner = d_ptr->inner_wall;
	/* for DIGGING */
	if (feat_wall_outer == FEAT_SHAL_WATER || feat_wall_inner == FEAT_SHAL_WATER ||
	    feat_wall_outer == FEAT_DEEP_WATER || feat_wall_inner == FEAT_DEEP_WATER)
		l_ptr->flags1 |= LF1_WATER | LF1_NO_LAVA;
	if (feat_wall_outer == FEAT_SHAL_LAVA || feat_wall_inner == FEAT_SHAL_LAVA ||
	    feat_wall_outer == FEAT_DEEP_LAVA || feat_wall_inner == FEAT_DEEP_LAVA)
		l_ptr->flags1 |= LF1_LAVA | LF1_NO_WATER;


	/* Setup probability info -- Floors */
	for (i = 0; i < 5; i++) {
		p1 = d_ptr->floor_percent[i][0];
		p2 = d_ptr->floor_percent[i][1];

#ifdef IRONDEEPDIVE_MIXED_TYPES
		/* note: We make the assumption, that tile types of
		 fields 1,2,3 have no two that are the same */
		/* hack the first 3 default probabilities for IDDC */
		if (i < 3) {
			if (d_ptr->floor[3] == d_ptr->floor[i]) {
				p1 -= d_ptr->floor_percent[3][0];
				p2 -= d_ptr->floor_percent[3][1];
			}
			if (d_ptr->floor[4] == d_ptr->floor[i]) {
				p1 -= d_ptr->floor_percent[4][0];
				p2 -= d_ptr->floor_percent[4][1];
			}
		}
		/* paranoia extreme */
		if (p1 < 0) p1 = 0;
		if (p2 < 0) p2 = 0;
#endif

		floor_lim[i] = (i == 5 - 1 ? 100 : (i > 0 ? floor_lim[i - 1] : 0) + p1 + (p2 - p1) * cur_depth / max_depth);

		if (p1 != 0 || p2 != 0) {
			/* for DIGGING */
			if (d_ptr->floor[i] == FEAT_SHAL_WATER || d_ptr->floor[i] == FEAT_DEEP_WATER)
				l_ptr->flags1 |= LF1_WATER | LF1_NO_LAVA;
			if (d_ptr->floor[i] == FEAT_SHAL_LAVA || d_ptr->floor[i] == FEAT_DEEP_LAVA)
				l_ptr->flags1 |= LF1_LAVA | LF1_NO_WATER;

			/* for streamer/river hack: don't build shallow ones if dungeon floor itself is deep */
			if (d_ptr->floor[i] == FEAT_DEEP_WATER)
			    //wrong: we check for this above (LF1_WATER)--   || d_ptr->floor[i] == FEAT_SHAL_WATER) /* also build deep rivers on shallow base floor! */
				l_ptr->flags1 |= LF1_DEEP_WATER;
			if (d_ptr->floor[i] == FEAT_DEEP_LAVA)
			    //wrong: we check for this above (LF1_WATER)--   || d_ptr->floor[i] == FEAT_SHAL_LAVA) /* also build deep rivers on shallow base floor! */
{
				l_ptr->flags1 |= LF1_DEEP_LAVA;
#ifdef TEST_SERVER /* debug */
s_printf("deep-lava (%d,%d,%d)\n", wpos->wx, wpos->wy, wpos->wz);
#endif
}
		}
	}

	/* Setup probability info -- Fillers */
	for (i = 0; i < 5; i++) {
		p1 = d_ptr->fill_percent[i][0];
		p2 = d_ptr->fill_percent[i][1];

#ifdef IRONDEEPDIVE_MIXED_TYPES
		/* note: We make the assumption, that tile types of
		 fields 1,2,3 have no two that are the same */
		/* hack the first 3 default probabilities for IDDC */
		if (i < 3) {
			if (d_ptr->fill_type[3] == d_ptr->fill_type[i]) {
				p1 -= d_ptr->fill_percent[3][0];
				p2 -= d_ptr->fill_percent[3][1];
			}
			if (d_ptr->fill_type[4] == d_ptr->fill_type[i]) {
				p1 -= d_ptr->fill_percent[4][0];
				p2 -= d_ptr->fill_percent[4][1];
			}
		}
		/* paranoia extreme */
		if (p1 < 0) p1 = 0;
		if (p2 < 0) p2 = 0;
#endif

		fill_lim[i] = (i == 5 - 1 ? 100 : (i > 0 ? fill_lim[i - 1] : 0) + p1 + (p2 - p1) * cur_depth / max_depth);
		/* for DIGGING */
		if (p1 != 0 || p2 != 0) {
			if (d_ptr->fill_type[i] == FEAT_SHAL_WATER || d_ptr->fill_type[i] == FEAT_DEEP_WATER)
				l_ptr->flags1 |= LF1_WATER | LF1_NO_LAVA;
			if (d_ptr->fill_type[i] == FEAT_SHAL_LAVA || d_ptr->fill_type[i] == FEAT_DEEP_LAVA)
				l_ptr->flags1 |= LF1_LAVA | LF1_NO_WATER;
		}
	}
	/* fix double-negative flag */
	if ((l_ptr->flags1 & LF1_NO_WATER) && (l_ptr->flags1 & LF1_NO_LAVA))
		l_ptr->flags1 &= ~(LF1_NO_WATER | LF1_NO_LAVA);

#if 0
	/* for DIGGING */
	if (get_skill(SKILL_DRUID) &&
	    (p_ptr->druid_extra & CLASS_FLOOD_LEVEL))
		l_ptr->flags1 |= LF1_WATER | LF1_NO_LAVA;
#endif
	/* Fill the arrays of floors and walls in the good proportions */
	for (i = 0; i < 1000; i++) {
#if 0
		/* Mega-HACK -- if a druid request a flooded level, he/she obtains it */
		if (get_skill(SKILL_DRUID) &&
		    (p_ptr->druid_extra & CLASS_FLOOD_LEVEL))
		{
			if (i < 333)
			{
				floor_type[i] = FEAT_DEEP_WATER;
			}
			else
			{
				floor_type[i] = FEAT_SHAL_WATER;
			}
		}
		else
#endif	/* 0 */
		{
			for (j = 0; j < 5; j++) {
				if (i < floor_lim[j] || j == 5 - 1) {
#ifdef IRONDEEPDIVE_MIXED_TYPES
					if (in_irondeepdive(wpos)) {
						if (iddc[ABS(wpos->wz)].step > 1) {
							switch (j) {
								case 3:
								case 4:
									floor_type[i] = d_info[iddc[ABS(wpos->wz)].next].floor[j];
								break;
								default:
									floor_type[i] = d_ptr->floor[j];
								break;
							}
						} else if (iddc[ABS(wpos->wz)].step > 0) {
							switch (j) {
								case 3:
									floor_type[i] = d_info[iddc[ABS(wpos->wz)].next].floor[j];
								break;
								default:
									floor_type[i] = d_ptr->floor[j];
								break;
							}
						} else floor_type[i] = d_ptr->floor[j];
					} else
#endif
					floor_type[i] = dt_ptr->theme ? d_info[dt_ptr->theme].floor[j] : d_ptr->floor[j];
					break;
				}
			}
		}

		for (j = 0; j < 5; j++) {
			if (i < fill_lim[j] || j == 5 - 1) {
#ifdef IRONDEEPDIVE_MIXED_TYPES
					if (in_irondeepdive(wpos)) {
						if (iddc[ABS(wpos->wz)].step > 1) {
							switch (j) {
								case 3:
								case 4:
									fill_type[i] = d_info[iddc[ABS(wpos->wz)].next].fill_type[j];
								break;
								default:
									fill_type[i] = d_ptr->fill_type[j];
								break;
							}
						} else if (iddc[ABS(wpos->wz)].step > 0) {
							switch (j) {
								case 3:
									fill_type[i] = d_info[iddc[ABS(wpos->wz)].next].fill_type[j];
								break;
								default:
									fill_type[i] = d_ptr->fill_type[j];
								break;
							}
						} else fill_type[i] = d_ptr->fill_type[j];
					} else
#endif
					fill_type[i] = dt_ptr->theme ? d_info[dt_ptr->theme].fill_type[j] : d_ptr->fill_type[j];
				break;
			}
		}
	}
}


/*
 * Fill a level with wall type specified in A: or L: line of d_info.txt
 *
 * 'use_floor', when it is TRUE, tells the function to use floor type
 * terrains (L:) instead of walls (A:).
 *
 * Filling behaviour can be controlled by the second parameter 'smooth',
 * with the following options available:
 *
 * smooth  behaviour
 * ------  ------------------------------------------------------------
 * 0       Fill the entire level with fill_type[0] / floor[0]
 * 1       All the grids are randomly selected (== --P5.1.2)
 * 2       Slightly smoothed -- look like scattered patches
 * 3       More smoothed -- tend to look like caverns / small scale map
 * 4--     Max smoothing -- tend to look like landscape/island/
 *         continent etc.
 * Added by C. Blue for Nether Realm:
 * 9       Like 2, just greatly reduced chance to place walls.
 *
 * I put it here, because there's another filler generator in
 * wild.c, but it works better there, in fact...
 *
 * CAVEAT: smoothness of 3 or greater doesn't work well with the
 * current secret door implementation. Outer walls also need some
 * rethinking.
 *
 * -- pelpel
 */

/*
 * Thou shalt not invoke the name of thy RNG in vain.
 * The Angband RNG generates 28 bit pseudo-random number, hence
 * 28 / 2 = 14
 */
#define MAX_SHIFTS 14

static void fill_level(worldpos *wpos, bool use_floor, byte smooth) {
	int y, x;
	int step;
	int shift;

	int cur_hgt = dun->l_ptr->hgt;
	int cur_wid = dun->l_ptr->wid;

	dungeon_type *dt_ptr = getdungeon(wpos);
	int dun_type;
#ifdef IRONDEEPDIVE_MIXED_TYPES
	if (in_irondeepdive(wpos)) dun_type = iddc[ABS(wpos->wz)].type;
	else
#endif
	dun_type = dt_ptr->theme ? dt_ptr->theme : dt_ptr->type;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Convert smoothness to initial step */
	if (smooth == 0) step = 0;
	else if (smooth == 1) step = 1;
	else if (smooth == 2) step = 2;
	else if (smooth == 3) step = 4;
	else if (smooth == 9) step = 2;
	else step = 8;

	/*
	 * Paranoia -- step must be less than or equal to a half of
	 * width or height, whichever shorter
	 */
	if ((cur_hgt < 16) && (step > 4)) step = 4;
	if ((cur_wid < 16) && (step > 4)) step = 4;


	/* Special case -- simple fill */
	if (step == 0)
	{
		byte filler;

		/* Pick a filler XXX XXX XXX */
		if (use_floor) filler = d_info[dun_type].floor[0];
		else filler = d_info[dun_type].fill_type[0];

		/* Fill the level with the filler without calling RNG */
		for (y = 1; y < cur_hgt - 1; y++)
		{
			for (x = 1; x < cur_wid - 1; x++)
			{
				cave_set_feat(wpos, y, x, filler);
			}
		}

		/* Done */
		return;
	}


	/*
	 * Fill starting positions -- every 'step' grids horizontally and
	 * vertically
	 */
	for (y = 0; y < cur_hgt; y += ((step == 9) ? 2 : step)) {
		for (x = 0; x < cur_wid; x += ((step == 9) ? 2 : step)) {
			/*
			 * Place randomly selected terrain feature using the prebuilt
			 * probability table
			 *
			 * By slightly modifying this, you can build streamers as
			 * well as normal fillers all at once, but this calls for
			 * modifications to the other part of the dungeon generator.
			 */
			if (use_floor) place_floor(wpos, y, x);
			else if (smooth != 9) place_filler(wpos, y, x);
			else if (magik(15)) place_filler(wpos, y, x);
			else place_floor(wpos, y, x);
		}
	}


	/*
	 * Fill spaces between, randomly picking one of their neighbours
	 *
	 * This simple yet powerful algorithm was described by Mike Anderson:
	 *
	 * A   B      A | B      A a B
	 *        ->  --+--  ->  d e b
	 * D   C      D | C      D c C
	 *
	 * a can be either A or B, b B or C, c C or D and d D or A.
	 * e is chosen from A, B, C and D.
	 * Subdivide and repeat the process as many times as you like.
	 *
	 * All the nasty tricks that obscure this simplicity are mine (^ ^;)
	 */

	/* Initialise bit shift counter */
	shift = MAX_SHIFTS;

	/* Repeat subdivision until all the grids are filled in */
	while ((step = step >> 1) > 0)
	{
		bool y_even, x_even;
		s16b y_wrap, x_wrap;
		s16b y_sel, x_sel;
		u32b selector = 0;

		/* Hacklette -- Calculate wrap-around locations */
		y_wrap = ((cur_hgt - 1) / (step * 2)) * (step * 2);
		x_wrap = ((cur_wid - 1) / (step * 2)) * (step * 2);

		/* Initialise vertical phase */
		y_even = 0;

		for (y = 0; y < cur_hgt; y += step)
		{
			/* Flip vertical phase */
			y_even = !y_even;

			/* Initialise horizontal phase */
			x_even = 0;

			for (x = 0; x < cur_wid; x += step)
			{
				/* Flip horizontal phase */
				x_even = !x_even;

				/* Already filled in by previous iterations */
				if (y_even && x_even) continue;

				/*
				 * Retrieve next two bits from pseudo-random bit sequence
				 *
				 * You can do well not caring so much about their randomness.
				 *
				 * This is not really necessary, but I don't like to invoke
				 * relatively expensive RNG when we can do with much smaller
				 * number of calls.
				 */
				if (shift >= MAX_SHIFTS)
				{
					selector = rand_int(0x10000000L);
					shift = 0;
				}
				else
				{
					selector >>= 2;
					shift++;
				}

				/* Vertically in sync */
				if (y_even) y_sel = y;

				/* Bit 1 selects neighbouring y */
				else y_sel = (selector & 2) ? y + step : y - step;

				/* Horizontally in sync */
				if (x_even) x_sel = x;

				/* Bit 0 selects neighbouring x */
				else x_sel = (selector & 1) ? x + step : x - step;

				/* Hacklette -- Fix out of range indices by wrapping around */
				if (y_sel >= cur_hgt) y_sel = 0;
				else if (y_sel < 0) y_sel = y_wrap;
				if (x_sel >= cur_wid) x_sel = 0;
				else if (x_sel < 0) x_sel = x_wrap;

				/*
				 * Fill the grid with terrain feature of the randomly
				 * picked up neighbour
				 */
				cave_set_feat(wpos, y, x, zcave[y_sel][x_sel].feat);
			}
		}
	}
}


/* check if there's an allocated floor within 1000ft interval that has a dungeon town.
   Interval center points are same as peak chance points for random town generation,
   ie 1k, 2k, 3k.. - C. Blue */
static bool no_nearby_dungeontown(struct worldpos *wpos) {
	int i, max;
	struct worldpos tpos;
	struct dungeon_type *d_ptr = getdungeon(wpos);

	tpos.wx = wpos->wx;
	tpos.wy = wpos->wy;
	max = 19; /* check +0..+19 ie 20 floors */

	/* tower */
	if (wpos->wz > 0) {
		tpos.wz = ((wpos->wz + 9) / 20) * 20 - 9;
		/* forbid too shallow towns anyway (paranoia if called without checking first) */
		if (tpos.wz <= 0) return TRUE;
		/* stay in bounds */
		if (d_ptr->maxdepth < tpos.wz + max) max = d_ptr->maxdepth - tpos.wz;

		for (i = 0; i <= max; i++) {
			if (isdungeontown(&tpos)) return FALSE;
			tpos.wz++;
		}
	}
	/* dungeon */
	else {
		tpos.wz = ((wpos->wz - 9) / 20) * 20 + 9;
		/* forbid too shallow towns anyway (paranoia if called without checking first) */
		if (tpos.wz >= 0) return TRUE;
		/* stay in bounds */
		if (d_ptr->maxdepth < -tpos.wz + max) max = d_ptr->maxdepth + tpos.wz;

		for (i = 0; i <= max; i++) {
			if (isdungeontown(&tpos)) return FALSE;
			tpos.wz--;
		}
	}
	return TRUE;
}

/*
 * Generate a new dungeon level
 *
 * Note that "dun_body" adds about 4000 bytes of memory to the stack.
 */
/*
 * Hrm, I know you wish to rebalance it -- but please implement
 * d_info stuffs first.. it's coded as such :)		- Jir -
 */
/* bad hack - Modify monster rarities for a particular IDDC theme live,
   and also in actual dungeons if desired (sandworm lair needs moar sandworms?) - C. Blue
   TODO: Move this into a specific d_info flag that boosts a monster's rarity! */
#define HACK_MONSTER_RARITIES
static void cave_gen(struct worldpos *wpos, player_type *p_ptr) {
	int i, k, y, x, y1 = 0, x1 = 0, dun_lev;
#ifdef ARCADE_SERVER
	int mx, my;
#endif
	bool netherrealm_level = FALSE, nr_bottom = FALSE;
	int build_special_store = 0; /* 0 = don't build a dungeon store,
					1 = build deep dungeon store,
					2 = build low-level dungeon store,
					3 = build ironman supply store
					4 = build specific ironman supply store: hidden library
					5 = build specific ironman supply store: deep supply
				    - C. Blue */

	bool destroyed = FALSE;
	bool empty_level = FALSE, dark_empty = TRUE, dark_level = FALSE;
	bool cavern = FALSE;
	bool maze = FALSE, permaze = FALSE, bonus = FALSE;

	monster_type *m_ptr;
	bool morgoth_inside = FALSE;

	cave_type *cr_ptr, *csbm_ptr, *c_ptr;
	struct c_special *cs_ptr;

	dun_data dun_body;

	/* Wipe the dun_data structure - mikaelh */
	WIPE(&dun_body, dun_data);

	cave_type **zcave;
	dungeon_type *d_ptr = getdungeon(wpos);
	int feat_boundary;

	/* Don't build one of these on 1x1 (super small) levels,
	   to avoid insta(ghost)kill if a player recalls into that pit - C. Blue */
	bool tiny_level = FALSE;
	bool town = FALSE, random_town_allowed; /* for ironman */
#ifdef IRONDEEPDIVE_STATIC_TOWNS
	bool town_static = FALSE;
#endif
	bool fountains_of_blood = FALSE; /* for vampires */

#ifdef HACK_MONSTER_RARITIES
	int hack_monster_idx = 0;//, hack_dun_idx; /* for Sandworm Lair/theme, sigh */
	int hack_monster_rarity = 0; //silly compiler warnings
	int hack_dun_table_idx = -1;
	int hack_dun_table_prob1 = 0, hack_dun_table_prob2 = 0, hack_dun_table_prob3 = 0; //silly compiler warnings
#endif

	bool streamers = FALSE, wall_streamers = FALSE;

	u32b df1_small = DF1_SMALL, df1_smallest = DF1_SMALLEST;
	int dtype = 0;
	u32b dflags1 = 0x0, dflags2 = 0x0, dflags3 = 0x0;

#ifdef IRONDEEPDIVE_EXPAND_SMALL
	if (in_irondeepdive(wpos)) {
		/* Note: the only applicable 'SMALLEST' dungeon for IDDC is 'The Maze' - enjoy the huge maze.. */
 #if 0
		/* expand small to normal and smallest to small */
		df1_small = DF1_SMALLEST;
		df1_smallest = 0x0;
 #else
		/* always expand level to normal size */
		df1_small = 0x0;
		df1_smallest = 0x0;
 #endif
	}
#endif

#ifdef IRONDEEPDIVE_MIXED_TYPES
	if (in_irondeepdive(wpos)) {
		dtype = iddc[ABS(wpos->wz)].type;
		dflags1 = d_info[dtype].flags1;
		dflags2 = d_info[dtype].flags2;
		dflags3 = d_info[dtype].flags3;
	} else
#endif
	if (d_ptr) {
		if (d_ptr->theme) {
			/* custom 'wilderness' (type-0) dungeon, admin-added */
			dtype = d_ptr->theme;
			/* start with original flags of this type-0 dungeon */
			dflags1 = d_ptr->flags1;
			dflags2 = d_ptr->flags2;
			dflags3 = d_ptr->flags3;
			/* add 'theme' flags that don't mess up our main flags too much */
			dflags1 |= (d_info[dtype].flags1 & DF1_THEME_MASK);
			dflags2 |= (d_info[dtype].flags2 & DF2_THEME_MASK);
			dflags3 |= (d_info[dtype].flags3 & DF3_THEME_MASK);
		} else {
			/* normal dungeon from d_info.txt */
			dtype = d_ptr->type;
			dflags1 = d_ptr->flags1;
			dflags2 = d_ptr->flags2;
			dflags3 = d_ptr->flags3;
		}
	}

	dun_lev = getlevel(wpos);
	/* Global data */
	dun = &dun_body;
	dun->l_ptr = getfloor(wpos);
	dun->l_ptr->flags1 = 0;
	dun->l_ptr->flags2 = 0;
	dun->l_ptr->monsters_generated = dun->l_ptr->monsters_spawned = dun->l_ptr->monsters_killed = 0;

	/* For Halloween: prevent someone 'DDOS-generating' the Pumpkin */
	if (season_halloween && p_ptr && (p_ptr->prob_travel || p_ptr->ghost)) dun->l_ptr->flags1 |= LF1_FAST_DIVE;

	/* Random seed for checking if a player logs back in on the same
	   [static] floor that he logged out, or if it has changed. - C. Blue */
	dun->l_ptr->id = (u32b)rand_int(0xFFFF) << 16;
	dun->l_ptr->id += rand_int(0xFFFF);

	/* test if we want fountains of blood in this dungeon - C. Blue */
#if 0//needs dungeon_info_type and all
	/* Process all dungeon-specific monster generation rules */
	for (i = 0; i < 10; i++) {
		rule_type *rule = &d_ptr->rules[i];
		if (!rule->percent) break;
		if (rule->percent >= 70 && rule->mode != 4 && (rule->mflags3 & RF3_UNDEAD))
			fountains_of_blood = TRUE;
	}
#else
	/* yay for hard-coding "The Paths of the Dead".. ;-|
	   (it's the only affected dungeon anyway) */
	if (dtype == DI_PATHS_DEAD) fountains_of_blood = TRUE;
#endif

	if (!(zcave = getcave(wpos))) return;

	/* added this for quest dungeons */
	if (d_ptr->quest) {
		qi_stage *q_qstage = &q_info[d_ptr->quest].stage[d_ptr->quest_stage];

		/* all floors are static? */
		if (q_qstage->dun_static)
			new_players_on_depth(wpos, 1, FALSE);

		/* final floor is premade? */
		if (ABS(wpos->wz) == d_ptr->maxdepth && q_qstage->dun_final_tpref) {
			int xs = q_qstage->dun_final_tpref_x, ys = q_qstage->dun_final_tpref_y;
			process_dungeon_file(q_qstage->dun_final_tpref, wpos, &ys, &xs, MAX_HGT, MAX_WID, TRUE);
			return;
		}
	}


	/* in case of paranoia, note that ALL normal dungeons (in d_info, and
	   currently all admin-created type-0 'wilderness' dungeons too) are
	   DF2_RANDOM ;) -- there is no exception */
	if (!d_ptr->flags2 & DF2_RANDOM) return;


	/* Hack -- Don't tell players about it (for efficiency) */
	level_generation_time = TRUE;

	/* Fill the arrays of floors and walls in the good proportions */
	init_feat_info(wpos);

	/* Are we in Nether Realm? Needed for shop creation later on */
	if (in_netherrealm(wpos)) netherrealm_level = TRUE;

	/* Note that Ultra-small levels (1/2 x 1/2 panel) actually result
	   from the rand_int in 'Small level', not from 'Very small'! - C. Blue */

	/* Very small (1 x 1 panel) level */
	if (
#ifdef IRONDEEPDIVE_MIXED_TYPES
	    in_irondeepdive(wpos) ? (!(dflags1 & DF1_BIG) && (dflags1 & df1_smallest)) :
#endif
	    (!(dflags1 & DF1_BIG) && (dflags1 & DF1_SMALLEST))) {
		dun->l_ptr->hgt = SCREEN_HGT;
		dun->l_ptr->wid = SCREEN_WID;
	}
	/* Small level */
	else if (
#ifdef IRONDEEPDIVE_MIXED_TYPES
 #ifdef IRONDEEPDIVE_BIG_IF_POSSIBLE
	    in_irondeepdive(wpos) ? (!(dflags1 & DF1_BIG) && (dflags1 & df1_small)) : /* rarely small */
 #else
	    in_irondeepdive(wpos) ? (!(dflags1 & DF1_BIG) && ((dflags1 & df1_small) || (!rand_int(SMALL_LEVEL * 2)))) : /* rarely small */
 #endif
#endif
	    (!(dflags1 & DF1_BIG) && ((dflags1 & DF1_SMALL) || (!rand_int(SMALL_LEVEL))))) {
#if 0 /* No more ultra-small levels for now (1/2 x 1/2 panel), and no max size here either :) - C. Blue*/
		dun->l_ptr->wid = MAX_WID - rand_int(MAX_WID / SCREEN_WID * 2) * (SCREEN_WID / 2);
		dun->l_ptr->hgt = MAX_HGT - rand_int(MAX_HGT / SCREEN_HGT * 2 - 1) * (SCREEN_HGT / 2);*/
#else
		dun->l_ptr->wid = MAX_WID - randint(MAX_WID / SCREEN_WID * 2 - 2) * (SCREEN_WID / 2);
		dun->l_ptr->hgt = MAX_HGT - randint(MAX_HGT / SCREEN_HGT * 2 - 2) * (SCREEN_HGT / 2);
#endif
	}
	/* Normal level */
	else {
		dun->l_ptr->wid = MAX_WID;
		dun->l_ptr->hgt = MAX_HGT;
	}

#ifdef ARCADE_SERVER
	dun->l_ptr->hgt = SCREEN_HGT*2;
	dun->l_ptr->wid = SCREEN_WID*2;
	if(tron_dark && wpos->wz == 11)
		dun->l_ptr->flags1 |= DF1_FORGET;
	if(tron_forget && wpos->wz == 11)
		dun->l_ptr->flags1 |= LF1_NO_MAP;
#else
	if (dun_lev < 100 && magik(NO_MAGIC_CHANCE)) dun->l_ptr->flags1 |= LF1_NO_MAGIC;
	if (magik(NO_GENO_CHANCE)) dun->l_ptr->flags1 |= LF1_NO_GENO;
	if (magik(NO_MAP_CHANCE) && dun_lev >= 5) dun->l_ptr->flags1 |= LF1_NO_MAP;
	if (magik(NO_MAGIC_MAP_CHANCE)) dun->l_ptr->flags1 |= LF1_NO_MAGIC_MAP;
	if (magik(NO_DESTROY_CHANCE)) dun->l_ptr->flags1 |= LF1_NO_DESTROY;
#endif

//#ifdef NETHERREALM_BOTTOM_RESTRICT
	/* no probability travel out of Zu-Aon's floor */
	if (netherrealm_bottom(wpos)) {
		/* removing other bad flags though, to make it fair */
		dun->l_ptr->flags1 = LF1_NO_MAGIC;
	}
//#endif

	/* copy dun flags to dunlev */
	if ((dflags1 & DF1_FORGET)) {
		dun->l_ptr->flags1 |= LF1_NO_MAP;
//s_printf("FORGET\n");
	}
	if ((dflags2 & DF2_NO_MAGIC_MAP)) dun->l_ptr->flags1 |= LF1_NO_MAGIC_MAP;
	if ((dflags1 & DF1_NO_DESTROY)) dun->l_ptr->flags1 |= LF1_NO_DESTROY;

	/* experimental stuff */
	if ((dflags3 & DF3_NO_TELE)) dun->l_ptr->flags2 |= LF2_NO_TELE;
	if ((dflags3 & DF3_NO_ESP)) dun->l_ptr->flags2 |= LF2_NO_ESP;
	if ((dflags3 & DF3_LIMIT_ESP)) dun->l_ptr->flags2 |= LF2_LIMIT_ESP;
	if ((dflags3 & DF3_NO_SUMMON)) dun->l_ptr->flags2 |= LF2_NO_SUMMON;

	/* Hack -- NO_MAP often comes with NO_MAGIC_MAP */
	if ((dun->l_ptr->flags1 & LF1_NO_MAP) && magik(55))
		dun->l_ptr->flags1 |= LF1_NO_MAGIC_MAP;

	/* PvP arena isn't special except that *destruction* doesn't work */
	if (in_pvparena(wpos)) dun->l_ptr->flags1 = LF1_NO_DESTROY;

	/* IRONMAN allows recalling at bottom/top */
	if (d_ptr && (d_ptr->flags2 & DF2_IRON)
	    && !(d_ptr->flags2 & DF2_NO_EXIT_WOR)) {
		if ((wpos->wz < 0 && wild_info[wpos->wy][wpos->wx].dungeon->maxdepth == -wpos->wz) ||
		    (wpos->wz > 0 && wild_info[wpos->wy][wpos->wx].tower->maxdepth == wpos->wz))
			dun->l_ptr->flags1 |= LF1_IRON_RECALL;
		/* IRONMAN allows recalling sometimes, if IRONFIX or IRONRND */
		else if (d_ptr && (dun_lev >= 20) && ( /* was 30 */
		    (!p_ptr->dummy_option_8 &&
		    (((d_ptr->flags2 & DF2_IRONRND1) && magik(20)) ||
		    ((d_ptr->flags2 & DF2_IRONRND2) && magik(12)) ||
		    ((d_ptr->flags2 & DF2_IRONRND3) && magik(8)) ||
		    ((d_ptr->flags2 & DF2_IRONRND4) && magik(5))))
		     ||
		    ((d_ptr->flags2 & DF2_IRONFIX1) && !(wpos->wz % 5)) ||
		    ((d_ptr->flags2 & DF2_IRONFIX2) && !(wpos->wz % 10))||
		    ((d_ptr->flags2 & DF2_IRONFIX3) && !(wpos->wz % 15)) ||
		    ((d_ptr->flags2 & DF2_IRONFIX4) && !(wpos->wz % 20)))) {
			dun->l_ptr->flags1 |= LF1_IRON_RECALL;
			/* Generate ironman town too? */
			if (d_ptr->flags2 & DF2_TOWNS_IRONRECALL) town = TRUE;
		}
	}


	/* Dungeon towns */
	random_town_allowed = TRUE;

#ifdef IRONDEEPDIVE_FIXED_TOWNS
	if (is_fixed_irondeepdive_town(wpos, dun_lev)) {
		p_ptr->IDDC_found_rndtown = FALSE;
		town = TRUE;
 #ifdef IRONDEEPDIVE_STATIC_TOWNS
		town_static = TRUE;
 #endif
 #ifdef IRONDEEPDIVE_FIXED_TOWN_WITHDRAWAL
		dun->l_ptr->flags1 |= LF1_IRON_RECALL;
 #endif
	}
 #ifdef IRONDEEPDIVE_EXTRA_FIXED_TOWNS
	else if (is_extra_fixed_irondeepdive_town(wpos, dun_lev)) {
		town = TRUE;
		//not static!
		//no withdrawal allowed!
	}
	/* prevent further random towns being generated in IDDC
	   if extra fixed towns are enabled */
	random_town_allowed = FALSE;
 #endif
#endif
	/* Generate town? */
#if 0 /* towns become rarer the deeper we go? */
	if (dun_lev > 2 && (town ||
	    ((d_ptr->flags2 & DF2_TOWNS_FIX) && !(dun_lev %
	    (dun_lev <= 30 ? 10 : (dun_lev <= 60 ? 15 : 20)))) ||
	    ((d_ptr->flags2 & DF2_TOWNS_RND) && magik(
	    (dun_lev <= 30 ? 10 : (dun_lev <= 60 ? 7 : 5)))) )) {
#else /* towns have high probability around n*1000ft floors, otherwise rare (for IRONDEEPDIVE) \
	 Also: No towns at Morgy depth or beyond. */
	k = 0;
	if (dun_lev > 2 && (town ||
	    (random_town_allowed &&
	    (dun_lev < 100 && (!p_ptr->IDDC_found_rndtown || !in_irondeepdive(wpos)) && (
	    ((d_ptr->flags2 & DF2_TOWNS_FIX) && !(dun_lev % 20)) ||
	    (!p_ptr->dummy_option_8 && (d_ptr->flags2 & DF2_TOWNS_RND) &&
 #if 0 /* for generic dungeons maybe */
	     magik(30 / (ABS(((dun_lev + 10) % 20) - 10) + 1))
 #else /* irondeepdive specifically: no towns before 900 ft or around the static towns at 2k and 4k */
	     magik(k = 25 / ( /* 35 -> 82.6% chance per -450..+500ft interval; 30 -> 78.8%; 25 -> 70.5%; 20 -> 62.3%; 15 -> 40.5%, 10 -> 35.4%, 5 -> 14.2% */
	    		    /* 900..1500ft: 35 -> %; 30 ->70.8%; 25 -> 62.7%; 20 -> %; 15 -> %, 10 -> %, 5 -> % */
	     (dun_lev >= 18 && (dun_lev <= 40 - 10 || dun_lev > 40 + 10) && (dun_lev <= 80 - 10 || dun_lev > 80 + 10)) ?
	     ABS(((dun_lev + 10) % 20) - 10) + 1 : 999
	     ))
 #endif
	    /* and new: no 2 towns within the same 1000ft interval (anti-cheeze, as if..) */
	    && no_nearby_dungeontown(wpos)
	    )))))) {
		if (k) {
			if (in_irondeepdive(wpos)) p_ptr->IDDC_found_rndtown = TRUE;
			s_printf("Generated random dungeon town at %d%% chance for player %s.\n", k, p_ptr->name);
			dun->l_ptr->flags1 |= LF1_RANDOM_TOWN;
		}
#endif
		bool lit = rand_int(3) == 0;
		/* prepare level: max size and permanent walls */
		dun->l_ptr->wid = MAX_WID;
		dun->l_ptr->hgt = MAX_HGT;
		for (y = 0; y < MAX_HGT; y++) {
			for (x = 0; x < MAX_WID; x++) {
				c_ptr = &zcave[y][x];
				c_ptr->feat = FEAT_PERM_MOUNTAIN;//FEAT_HIGH_MOUNT_SOLID;
				if (lit) c_ptr->info |= CAVE_GLOW;
			}
		}

		/* add the town */
		s_printf("DF2_TOWNS_: Adding a town at %d,%d,%d.\n", wpos->wx, wpos->wy, wpos->wz);
		dun->l_ptr->flags1 |= LF1_DUNGEON_TOWN | LF1_NO_DESTROY;
 		dun->l_ptr->flags1 &= ~(LF1_NO_MAP | LF1_NO_MAGIC_MAP);
		town_gen_hack(wpos);

#ifdef IRONDEEPDIVE_STATIC_TOWNS
		if (town_static) {
			/* reattach objects and monsters */
			setup_objects();
			/* target dummies */
			setup_monsters();
		}
#endif

		level_generation_time = FALSE;
		return;
	}


	/* not too big levels for 'Arena Monster Challenge' event */
	if (ge_special_sector && in_arena(wpos)) {
		dun->l_ptr->wid = MAX_WID / 2;
		dun->l_ptr->hgt = MAX_HGT / 2;
		dun->l_ptr->flags1 &= ~(LF1_NO_MAGIC | LF1_NO_GENO | LF1_NO_MAP | LF1_NO_MAGIC_MAP);
		dun->l_ptr->flags1 |= LF1_NO_DESTROY;
	}

	/* remember if it's a tiny level and avoid building certain room types consequently */
#if 0
	if (dun->l_ptr->wid <= SCREEN_WID && dun->l_ptr->hgt <= (SCREEN_HGT * 3) / 2 ||
	    dun->l_ptr->wid <= (SCREEN_WID * 3) / 2 && dun->l_ptr->hgt <= SCREEN_HGT) tiny_level = TRUE;
#else
	if (dun->l_ptr->wid * dun->l_ptr->hgt <= (SCREEN_WID * SCREEN_HGT * 3) / 2) tiny_level = TRUE;
#endif

	/* So as not to generate too 'crowded' levels */
	dun->ratio = 100 * dun->l_ptr->wid * dun->l_ptr->hgt / MAX_HGT / MAX_WID;

	/* Possible cavern */
	if (rand_int(dun_lev) > DUN_CAVERN && magik(DUN_CAVERN2)) {
		cavern = TRUE;

#if 0
		/* Make a large fractal cave in the middle of the dungeon */
		if (cheat_room) {
			msg_print("Cavern on level.");
		}
#endif	/* 0 */
	}


	/* Possible "destroyed" level */
	if ((dun_lev > 10) && (rand_int(DUN_DEST) == 0)) destroyed = TRUE;

	/* Hack -- No destroyed "quest" levels */
	if (is_xorder(wpos) || (dun->l_ptr->flags1 & LF1_NO_DESTROY))
		destroyed = FALSE;

	/* Hack -- Watery caves */
	dun->watery = !(dflags3 & DF3_NOT_WATERY) && dun_lev > 5 &&
		((((dun_lev + WATERY_OFFSET) % WATERY_CYCLE) >= (WATERY_CYCLE - WATERY_RANGE))?
		magik(WATERY_BELT_CHANCE) : magik(DUN_RIVER_CHANCE - dun_lev * DUN_RIVER_REDUCE / 100));

	maze = (!cavern && rand_int(DUN_MAZE_FACTOR) < dun_lev - 10) ? TRUE : FALSE;
	if (dflags1 & DF1_MAZE) maze = TRUE;

	if (maze) permaze = magik(DUN_MAZE_PERMAWALL);

	if (!maze && !cavern &&
	    ((dflags1 & (DF1_EMPTY)) || (!rand_int(EMPTY_LEVEL) && !(dflags3 & DF3_NOT_EMPTY)))) {
		empty_level = TRUE;
		if ((randint(DARK_EMPTY) != 1 || (randint(100) > dun_lev)))
			dark_empty = FALSE;
	}
	if (dflags3 & DF3_NO_DARK) dark_level = FALSE;
	else if (dflags3 & DF3_DARK) dark_level = TRUE;

	if (dflags3 & DF3_NO_EMPTY) empty_level = FALSE;
	if (dflags3 & DF3_NO_DESTROYED) destroyed = FALSE;
	if (dflags3 & DF3_NO_MAZE) maze = permaze = FALSE;

	/* Hack for bottom of Nether Realm */
	if (netherrealm_level && dun_lev == netherrealm_end) {
		destroyed = FALSE;
		empty_level = TRUE; dark_empty = TRUE;
		cavern = FALSE;
		maze = FALSE; permaze = FALSE; bonus = FALSE;
		nr_bottom = TRUE;
	}

	/* All dungeons get their own visuals now, if defined in B-line in d_info - C. Blue */
	feat_boundary = d_info[dtype].feat_boundary;

	/* Efficiency */
	if ((empty_level && !dark_empty) && !dark_level) {
		/* Hack -- Start with permawalls
		 * Hope run-length do a good job :) */
		for (y = 0; y < MAX_HGT; y++) {
			for (x = 0; x < MAX_WID; x++) {
				c_ptr = &zcave[y][x];

#ifdef BIG_MAP
				/* new visuals - also for non-big map mode actually possible */
				if (x >= dun->l_ptr->wid || y >= dun->l_ptr->hgt) c_ptr->feat = FEAT_PERM_FILL;
				else
#endif
				/* Create granite (? granite != permanent) wall */
				c_ptr->feat = feat_boundary;

				/* Illuminate Arena if needed */
				c_ptr->info |= CAVE_GLOW;
			}
		}
	} else {
		/* Hack -- Start with permawalls
		 * Hope run-length do a good job :) */
		for (y = 0; y < MAX_HGT; y++) {
			for (x = 0; x < MAX_WID; x++) {
				c_ptr = &zcave[y][x];

#ifdef BIG_MAP
				/* new visuals - also for non-big map mode actually possible */
				if (x >= dun->l_ptr->wid || y >= dun->l_ptr->hgt) c_ptr->feat = FEAT_PERM_FILL;
				else
#endif
				/* Create granite (? granite != permanent) wall */
				c_ptr->feat = feat_boundary;
			}
		}
	}

#if 0
	/* Hack -- then curve with basic granite */
	for (y = 1; y < dun->l_ptr->hgt - 1; y++) {
		for (x = 1; x < dun->l_ptr->wid - 1; x++) {
			c_ptr = &zcave[y][x];

			/* Create granite wall */
			c_ptr->feat = empty_level ? FEAT_FLOOR :
				(permaze ? FEAT_PERM_INNER : FEAT_WALL_EXTRA);
		}
	}
#else	/* 0 */

	/*
	 * Hack -- Start with fill_type's
	 *
	 * Need a way to know appropriate smoothing factor for the current
	 * dungeon. Maybe we need another d_info flag/value.
	 */
	/* Hack -- then curve with basic granite */
	if (permaze) {
		for (y = 1; y < dun->l_ptr->hgt - 1; y++) {
			for (x = 1; x < dun->l_ptr->wid - 1; x++) {
				c_ptr = &zcave[y][x];

				/* Create permawall */
				c_ptr->feat = FEAT_PERM_INNER;
			}
		}
	}
 #ifdef IRONDEEPDIVE_MIXED_TYPES
	//if (in_irondeepdive(wpos)) fill_level(wpos, empty_level, iddc[ABS(wpos->wz)].step ? rand_int(4) : d_info[iddc[ABS(wpos->wz)].type].fill_method); //Random smooth! (was 1+randint(3))
	if (in_irondeepdive(wpos))
		fill_level(wpos, empty_level, iddc[ABS(wpos->wz)].step <= 1 ?
		    d_info[iddc[ABS(wpos->wz)].type].fill_method :
		    d_info[iddc[ABS(wpos->wz)].next].fill_method);
	else
 #endif
	fill_level(wpos, empty_level, d_info[d_ptr->type].fill_method);
#endif	/* 0 */

	if (cavern) build_cavern(wpos);


	/* Actual maximum number of rooms on this level */
/*	dun->row_rooms = MAX_HGT / BLOCK_HGT;
	dun->col_rooms = MAX_WID / BLOCK_WID;	*/

	dun->row_rooms = dun->l_ptr->hgt / BLOCK_HGT;
	dun->col_rooms = dun->l_ptr->wid / BLOCK_WID;

	/* Initialize the room table */
	for (y = 0; y < dun->row_rooms; y++)
		for (x = 0; x < dun->col_rooms; x++)
			dun->room_map[y][x] = FALSE;


	/* No "crowded" rooms yet */
	dun->crowded = FALSE;

#ifdef HACK_MONSTER_RARITIES
	/* bad hack: temporarily make sandworms very common in Sandworm Lair.
	   I added this especially for themed IDDC, which would otherwise mean
	   a bunch of pretty empty levels (or just lame worms mostly) - C. Blue */
	if (dtype == DI_SANDWORM_LAIR) {
		alloc_entry *table = alloc_race_table_dun[DI_SANDWORM_LAIR];

		//hack_dun_idx = DI_SANDWORM_LAIR;

		hack_monster_idx = 1031;
		hack_monster_rarity = r_info[1031].rarity;
		r_info[1031].rarity = 1;

		i = 0;
		do {
			if (table[i].index == 1031) {
				hack_dun_table_idx = i;
				hack_dun_table_prob1 = table[i].prob1;
				hack_dun_table_prob2 = table[i].prob2;
				hack_dun_table_prob3 = table[i].prob3;
				table[i].prob1 = 10000;
				table[i].prob2 = 10000;
				table[i].prob3 = 10000;
				break;
			}
			i++;
		} while (i < max_r_idx); /* paranoia */
	}
#endif

	/* No rooms yet */
	dun->cent_n = 0;

	if (!nr_bottom) {
		/* Build some rooms */
		if (!maze || !(dflags1 & DF1_MAZE)) for (i = 0; i < DUN_ROOMS; i++) {
			/* Pick a block for the room */
			y = rand_int(dun->row_rooms);
			x = rand_int(dun->col_rooms);

			/* Align dungeon rooms */
			if (dungeon_align) {
				/* Slide some rooms right */
				if ((x % 3) == 0) x++;

				/* Slide some rooms left */
				if ((x % 3) == 2) x--;
			}

			/* Destroyed levels are boring */
			if (destroyed) {
				/* The deeper you are, the more cavelike the rooms are */

				/* no caves when cavern exists: they look bad */
				k = randint(100);

				if (!cavern && (k < dun_lev) && !tiny_level) {
					/* Type 10 -- Fractal cave */
					if (room_build(wpos, y, x, 10, p_ptr)) continue;
				} else {
					/* Attempt a "trivial" room */
#if 0
					if ((d_ptr->flags1 & DF1_CIRCULAR_ROOMS) &&
						room_build(y, x, 9, p_ptr)))
#endif	/* 0 */
					if (magik(30) && room_build(wpos, y, x, 9, p_ptr)) continue;
					else if (room_build(wpos, y, x, 1, p_ptr)) continue;
				}

#if 0
				/* Attempt a "trivial" room */
				if (room_build(wpos, y, x, 1, p_ptr)) continue;
#endif	/* 0 */

				/* Never mind */
				continue;
			}

			/* Attempt an "unusual" room */
			if (rand_int(DUN_UNUSUAL) < dun_lev) {
				/* Roll for room type */
				k = rand_int(100);

				/* Attempt a very unusual room */

				if (rand_int(DUN_UNUSUAL) < dun_lev
#if 1 /* tiny levels don't get any DUN_UNUSUAL room types at all */
				    && !tiny_level) {
#else /* tiny levels don't get nests, pits or greater vaults */
				    ) {
					if (tiny_level) k = rand_int(65) + 35;
#endif
					/* Type 5 -- Monster nest (10%) */
					if ((k < 10) && room_build(wpos, y, x, 5, p_ptr)) continue;

					/* Type 6 -- Monster pit (15%) */
					if ((k < 25) && room_build(wpos, y, x, 6, p_ptr)) continue;

					/* Type 8 -- Greater vault (10%) */
					if ((k < 35) && room_build(wpos, y, x, 8, p_ptr)) continue;

					/* Type 11 -- Random vault (10%) */
					if ((k < 45) && room_build(wpos, y, x, 11, p_ptr)) continue;

					/* Type 7 -- Lesser vault (15%) */
					if ((k < 60) && room_build(wpos, y, x, 7, p_ptr)) continue;
				} else {
					/* Type 4 -- Large room (25%) */
					if ((k < 25) && room_build(wpos, y, x, 4, p_ptr)) continue;

					/* Type 3 -- Cross room (20%) */
					if ((k < 45) && room_build(wpos, y, x, 3, p_ptr)) continue;

					/* Type 2 -- Overlapping (20%) */
					if ((k < 65) && room_build(wpos, y, x, 2, p_ptr)) continue;
				}

				/* Type 10 -- Fractal cave (15%) */
				if ((k < 80) && room_build(wpos, y, x, 10, p_ptr)) continue;

				/* Type 9 -- Circular (10%) */
				/* Hack - build standard rectangular rooms if needed */
				if (k < 90) {
					if (((dflags1 & DF1_CIRCULAR_ROOMS) || magik(70)) &&
					    room_build(wpos, y, x, 1, p_ptr))
						continue;
					else if (room_build(wpos, y, x, 9, p_ptr)) continue;
				}

				/* Type 12 -- Crypt (10%) */
				if ((k < 100) && room_build(wpos, y, x, 12, p_ptr)) continue;
			}

			/* Attempt a trivial room */
			if ((dflags1 & DF1_CAVE) || magik(50)) {
				if (room_build(wpos, y, x, 10, p_ptr)) continue;
			} else {
				if (((dflags1 & DF1_CIRCULAR_ROOMS) || magik(30)) &&
				    room_build(wpos, y, x, 9, p_ptr))
					continue;
				else if (room_build(wpos, y, x, 1, p_ptr)) continue;
			}
		}
	}

#if 1
	/* XXX the walls here should 'mimic' the surroundings,
	 * however I omitted it to spare 522 c_special	- Jir */
	/* Special boundary walls -- Top + bottom */
	for (x = 0; x < dun->l_ptr->wid; x++) {
		/* Clear previous contents, add "solid" perma-wall */
		zcave[0][x].feat = feat_boundary;
		zcave[dun->l_ptr->hgt - 1][x].feat = feat_boundary;
	}
	/* Special boundary walls -- Left + right */
	for (y = 0; y < dun->l_ptr->hgt; y++) {
		zcave[y][0].feat = feat_boundary;
		zcave[y][dun->l_ptr->wid - 1].feat = feat_boundary;
	}
#endif	/* 0 */

	if (!(dflags1 & (DF1_MAZE | DF1_LIFE_LEVEL)) &&
	    !(dflags1 & DF1_NO_STREAMERS))
		streamers = TRUE;
	if (!(dflags1 & (DF1_MAZE | DF1_LIFE_LEVEL)) &&
	    !(dflags3 & DF3_NO_WALL_STREAMERS))
		wall_streamers = TRUE;

	if (maze) generate_maze(wpos, (dflags1 & DF1_MAZE) ? 1 : randint(3));
	else {
		if (!nr_bottom) {
			/* Hack -- Scramble the room order */
			for (i = 0; i < dun->cent_n; i++) {
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
			for (i = 0; i < dun->cent_n; i++) {
				/* Connect the room to the previous room */
				build_tunnel(wpos, dun->cent[i].y, dun->cent[i].x, y, x);

				/* Remember the "previous" room */
				y = dun->cent[i].y;
				x = dun->cent[i].x;
			}

			/* Place intersection doors	 */
			for (i = 0; i < dun->door_n; i++) {
				/* Extract junction location */
				y = dun->door[i].y;
				x = dun->door[i].x;

				/* Try placing doors */
#if 0
				try_door(wpos, y, x - 1);
				try_door(wpos, y, x + 1);
				try_door(wpos, y - 1, x);
				try_door(wpos, y + 1, x);
#endif	/* 0 */
				try_doors(wpos, y , x);
			}
		}

		if (wall_streamers) {
			/* Hack -- Add some magma streamers */
			for (i = 0; i < DUN_STR_MAG; i++)
				build_streamer(wpos, FEAT_MAGMA, DUN_STR_MC, FALSE);

			/* Hack -- Add some quartz streamers */
			for (i = 0; i < DUN_STR_QUA; i++)
				build_streamer(wpos, FEAT_QUARTZ, DUN_STR_QC, FALSE);

			/* Add some sand streamers */
			if (((dflags1 & DF1_SAND_VEIN) && !rand_int(4)) ||
			    magik(DUN_SANDWALL)) {
				build_streamer(wpos, FEAT_SANDWALL, DUN_STR_SC, FALSE);
			}
		}

		/* Hack -- Add some rivers if requested */
		if ((dflags1 & DF1_WATER_RIVER) && !rand_int(4))
			add_river(wpos, FEAT_DEEP_WATER, FEAT_SHAL_WATER);

		if ((dflags1 & DF1_LAVA_RIVER) && !rand_int(4))
			add_river(wpos, FEAT_DEEP_LAVA, FEAT_SHAL_LAVA);

		if ((dflags1 & DF1_WATER_RIVERS) || (dun->watery)) {
			int max = 3 + rand_int(2);

			for (i = 0; i < max; i++) {
				if (rand_int(3) == 0) add_river(wpos, FEAT_DEEP_WATER, FEAT_SHAL_WATER);
			}
		}
		if ((dflags1 & DF1_LAVA_RIVERS)) {
			int max = 2 + rand_int(2);

			for (i = 0; i < max; i++) {
				if (rand_int(3) == 0) add_river(wpos, FEAT_DEEP_LAVA, FEAT_SHAL_LAVA);
			}
		}

#ifdef VOLCANIC_FLOOR /* experimental - this might be too costly on cpu: add volcanic floor around lava rivers - C. Blue */
		for (x = 0; x < dun->l_ptr->wid; x++)
			for (y = 0; y < dun->l_ptr->hgt; y++) {
				if (zcave[y][x].feat != FEAT_SHAL_LAVA && zcave[y][x].feat != FEAT_DEEP_LAVA) continue;
				for (i = 0; i < 4; i++) {
					k = zcave[y + ddy_ddd[i]][x + ddx_ddd[i]].feat;
					switch (k) {
					case FEAT_WEB:
					case FEAT_CROP:
					case FEAT_FLOWER:
					case FEAT_IVY:
					case FEAT_GRASS:
						/* never persist */
						zcave[y + ddy_ddd[i]][x + ddx_ddd[i]].feat = FEAT_ASH;
						continue;
					case FEAT_SNOW:
					case FEAT_SHAL_WATER:
					case FEAT_DARK_PIT:
					case FEAT_ICE:
					case FEAT_MUD:
					case FEAT_ICE_WALL:
						/* never persist */
						zcave[y + ddy_ddd[i]][x + ddx_ddd[i]].feat = FEAT_VOLCANIC;
						continue;
					case FEAT_TREE:
					case FEAT_BUSH://mh
						/* never persist */
						zcave[y + ddy_ddd[i]][x + ddx_ddd[i]].feat = FEAT_DEAD_TREE;
						continue;
					case FEAT_SAND:
					case FEAT_FLOOR:
					case FEAT_LOOSE_DIRT:
					case FEAT_DIRT:
						/* rarely persist */
						if (!rand_int(7)) continue;
						zcave[y + ddy_ddd[i]][x + ddx_ddd[i]].feat = FEAT_VOLCANIC;
					default:
						continue;
					}
				}
			}
#endif

		/* Destroy the level if necessary */
		if (destroyed) destroy_level(wpos);
	}

	if (!(dun->l_ptr->flags1 & LF1_NO_STAIR)) {
		/* place downstairs and upstairs -
		   currently, more downstairs are generated in general than upstairs,
		   so venturing into dungeons is actually easier than climbing up towers! */
		if (!in_netherrealm(wpos)) {
			/* Place 3 or 4 down stairs near some walls */
			alloc_stairs(wpos, (d_ptr->flags1 & DF1_FLAT) ? FEAT_WAY_MORE : FEAT_MORE, rand_range(3, 4) * dun->ratio / 100 + 1, 3, NULL);
			/* Place 2 or 3 up stairs near some walls */
			alloc_stairs(wpos, (d_ptr->flags1 & DF1_FLAT) ? FEAT_WAY_LESS : FEAT_LESS, rand_range(2, 3), 3, NULL);

#if 0			/* we need a way to create the way back */
			/* Place 1 or 2 (typo of '0 or 1'?) down shafts near some walls */
			if (!(d_ptr->flags2 & DF2_NO_SHAFT)) alloc_stairs(wpos, (d_ptr->flags1 & DF1_FLAT) ? FEAT_WAY_MORE : FEAT_SHAFT_DOWN, rand_range(0, 1), 3, NULL);

			/* Place 0 or 1 up shafts near some walls */
			if (!(d_ptr->flags2 & DF2_NO_SHAFT)) alloc_stairs((d_ptr->flags1 & DF1_FLAT) ? FEAT_WAY_LESS : FEAT_SHAFT_UP, rand_range(0, 1), 3, NULL);
#endif	/* 0 */

			/* Hack -- add *more* stairs for lowbie's sake */
			if (dun_lev <= COMFORT_PASSAGE_DEPTH) {
				/* Place 2 or 3 down stairs near some walls */
				alloc_stairs(wpos, (d_ptr->flags1 & DF1_FLAT) ? FEAT_WAY_MORE : FEAT_MORE, rand_range(2, 3), 3, NULL);

				/* Place 2 or 3 up stairs near some walls */
				alloc_stairs(wpos, (d_ptr->flags1 & DF1_FLAT) ? FEAT_WAY_LESS : FEAT_LESS, rand_range(2, 3), 3, NULL);
			}

			/* Hack -- add *more* downstairs for Highlander Tournament event */
			if (sector00separation && in_highlander_dun(wpos) && dun_lev > COMFORT_PASSAGE_DEPTH) {
				/* Place 3 or 4 down stairs near some walls */
				alloc_stairs(wpos, (d_ptr->flags1 & DF1_FLAT) ? FEAT_WAY_MORE : FEAT_MORE, rand_range(2, 4), 2, NULL);
			}

		/* in the Nether Realm, stairs leading onwards must not be generated close to stairs backwards,
		   or rogues could cloak-scum for them to be next to each other (albeit ridiculously tedious probably, *probably*..) */
		} else if (!netherrealm_bottom(wpos)) {
			struct stairs_list stairs[5];
			for (i = 0; i != 5 - 1; i++) stairs[i].x = -1; /* init usable elements */
			stairs[i].y = -1; /* mark final 'terminator' element */

			if (netherrealm_wpos_z < 1) {
				/* Place 2 down stairs near some walls */
				alloc_stairs(wpos, (d_ptr->flags1 & DF1_FLAT) ? FEAT_WAY_MORE : FEAT_MORE, 2, 2, stairs);
				/* Place 1 up stairs near some walls */
				alloc_stairs(wpos, (d_ptr->flags1 & DF1_FLAT) ? FEAT_WAY_LESS : FEAT_LESS, 1, 2, stairs);
			} else {
				/* Place 2 up stairs near some walls */
				alloc_stairs(wpos, (d_ptr->flags1 & DF1_FLAT) ? FEAT_WAY_LESS : FEAT_LESS, 2, 2, stairs);
				/* Place 1 down stairs near some walls */
				alloc_stairs(wpos, (d_ptr->flags1 & DF1_FLAT) ? FEAT_WAY_MORE : FEAT_MORE, 1, 2, stairs);
			}

		} else {
			/* place 1 staircase */
			if (netherrealm_wpos_z < 1)
				alloc_stairs(wpos, (d_ptr->flags1 & DF1_FLAT) ? FEAT_WAY_LESS : FEAT_LESS, 1, 2, NULL);
			else
				alloc_stairs(wpos, (d_ptr->flags1 & DF1_FLAT) ? FEAT_WAY_MORE : FEAT_MORE, 1, 2, NULL);
		}

	}

#if 0
	process_hooks(HOOK_GEN_LEVEL, "(d)", is_xorder(dun_lev));
#endif

	/* Determine the character location */
	new_player_spot(wpos);


	/* Add streamers of trees, water, or lava -KMW- */
	if (streamers) {
		int num;

		/*
		 * Flat levels (was: levels 1--2)
		 *
		 * Small trees (penetrate walls)
		 */
		if ((dflags1 & DF1_FLAT) && (randint(20) > 15)) {
			num = randint(DUN_STR_QUA);

			for (i = 0; i < num; i++)
				build_streamer2(wpos, FEAT_IVY, 1);
		}

		/*
		 * Levels 1 -- 33 (was: 1 -- 19)
		 *
		 * Shallow water (preserve walls)
		 * Deep water (penetrate walls)
		 */
		if ((dun_lev <= 33) && (randint(20) > 15)) {
			num = randint(DUN_STR_QUA - 1);

			for (i = 0; i < num; i++)
				build_streamer2(wpos, (dun->l_ptr->flags1 & (LF1_WATER | LF1_DEEP_WATER)) ? FEAT_DEEP_WATER : FEAT_SHAL_WATER, 0);

			if (randint(20) > 15) {
				num = randint(DUN_STR_QUA);

				for (i = 0; i < num; i++)
					build_streamer2(wpos, FEAT_DEEP_WATER, 1);
			}
		}

		/*
		 * Levels 34 -- (was: 20 --)
		 */
		else if (dun_lev > 33) {
			/*
			 * Shallow lava (preserve walls)
			 * Deep lava (penetrate walls)
			 */
			if (randint(20) > 15) {
				num = randint(DUN_STR_QUA);

				for (i = 0; i < num; i++)
					build_streamer2(wpos, (dun->l_ptr->flags1 & (LF1_LAVA | LF1_DEEP_LAVA)) ? FEAT_DEEP_LAVA : FEAT_SHAL_LAVA, 0);

				if (randint(20) > 15) {
					num = randint(DUN_STR_QUA - 1);

					for (i = 0; i < num; i++)
						build_streamer2(wpos, FEAT_DEEP_LAVA, 1);
				}
			}

			/*
			 * Shallow water (preserve walls)
			 * Deep water (penetrate walls)
			 */
			else if (randint(20) > 15) {
				num = randint(DUN_STR_QUA - 1);

				for (i = 0; i < num; i++)
					build_streamer2(wpos, (dun->l_ptr->flags1 & (LF1_WATER | LF1_DEEP_WATER)) ? FEAT_DEEP_WATER : FEAT_SHAL_WATER, 0);

				if (randint(20) > 15) {
					num = randint(DUN_STR_QUA);

					for (i = 0; i < num; i++)
						build_streamer2(wpos, FEAT_DEEP_WATER, 1);
				}
			}
		}
	}

	/* ugly hack to fix the buggy extra bottom line that gets added to non-maxed levels sometimes:
	   just overwrite it with filler tiles. */
	for (x = 0; x < MAX_WID; x++)
		for (y = dun->l_ptr->hgt; y < MAX_HGT; y++)
			zcave[y][x].feat = FEAT_PERM_FILL;
#if 1 /* prevent extra right-side lines too? do these even occur? */
	for (x = dun->l_ptr->wid; x < MAX_WID; x++)
		for (y = 0; y < dun->l_ptr->hgt; y++) //the rest is already covered by the previous check above
			zcave[y][x].feat = FEAT_PERM_FILL;
#endif

	/* Possibly create dungeon boss aka FINAL_GUARDIAN.
	   Rarity 1 in r_info.txt for those bosses now means:
	   1 in <rarity> chance to generate the boss. - C. Blue */
	if (
#ifdef IRONDEEPDIVE_MIXED_TYPES
	    in_irondeepdive(wpos) ?
	    ((k = d_info[iddc[ABS(wpos->wz)].type].final_guardian)
	     && d_info[iddc[ABS(wpos->wz)].type].maxdepth == ABS(wpos->wz)
 #ifndef IDDC_GUARANTEED_FG /* not guaranteed spawn? (default) */
	     && !rand_int((r_info[k].rarity + 1) / 2)
 #endif
	     ) :
#endif
	    ((k = d_info[d_ptr->type].final_guardian)
	    && d_ptr->maxdepth == ABS(wpos->wz)
	    && !rand_int(r_info[k].rarity))) {
		//s_printf("Attempting to generate FINAL_GUARDIAN %d (1 in %d)\n", k, r_info[k].rarity);
		summon_override_checks = SO_FORCE_DEPTH; /* allow >20 level OoD if desired */
		alloc_monster_specific(wpos, k, 20, TRUE);
		summon_override_checks = SO_NONE;
#if 0
		/* debug: break here? */
		level_generation_time = FALSE;
		return;
#endif
	}

#ifdef GLOBAL_DUNGEON_KNOWLEDGE
	/* we now 'learned' the max level of this dungeon */
	if (d_ptr->maxdepth == ABS(wpos->wz)) {
		d_ptr->known |= 0x4;
		/* automatically learn if there is no dungeon boss */
		if (!k) d_ptr->known |= 0x8;
	}
#endif

	/* Basic "amount" */
	k = (dun_lev / 3);

	if (k > 10) k = 10;
	k = k * dun->ratio / 100 + 1;
	if (k < 2) k = 2;

	if (empty_level || maze) {
		k *= 2;
		bonus = TRUE;
	}

	/* Pick a base number of monsters */
	i = MIN_M_ALLOC_LEVEL + randint(8);
	i = i * dun->ratio / 100 + 1;

	/* dungeon has especially many monsters? */
	if ((d_ptr->flags3 & DF3_MANY_MONSTERS)) i = (i * 3) / 2;
	if ((d_ptr->flags3 & DF3_VMANY_MONSTERS)) i *= 2;

	/* Put some monsters in the dungeon */
	for (i = i + k; i > 0; i--)
		(void)alloc_monster(wpos, 0, TRUE);

#ifdef ENABLE_MAIA
	/* Force a pair of darkling and candlebearer when there is at least
	 * one divine on level that needs it.
	 */
	if (dun_lev >= (in_irondeepdive(wpos) ? 10 : 12) && dun_lev <= 20 &&
	    p_ptr && p_ptr->prace == RACE_MAIA && !p_ptr->ptrait) {
		//5 + randint(dun->row_rooms - 5), x1 = randint(dun->col_rooms - 5);
		int x, y, x1, y1, tries = 2000;
		do {
			x1 = rand_int(dun->l_ptr->wid - 6) + 3;
			y1 = rand_int(dun->l_ptr->hgt - 6) + 3;
		} while (!(cave_empty_bold(zcave, y1, x1) &&
		    (zcave[y1][x1].info & CAVE_ROOM)) &&
		    --tries);
		if (!tries) scatter(wpos, &y, &x, y1, x1, 20, FALSE);
		else {
			x = x1;
			y = y1;
		}

		if (rand_int(2) == 1) place_monster_one(wpos, y, x, RI_CANDLEBEARER, FALSE, FALSE, FALSE, 0, 0);
		else place_monster_one(wpos, y, x, RI_DARKLING, FALSE, FALSE, FALSE, 0, 0);
	}
#endif

#ifdef HACK_MONSTER_RARITIES
	/* unhack */
	if (hack_monster_idx) {
		alloc_entry *table = alloc_race_table_dun[DI_SANDWORM_LAIR];
		if (hack_dun_table_idx != -1) {
			table[hack_dun_table_idx].prob1 = hack_dun_table_prob1;
			table[hack_dun_table_idx].prob2 = hack_dun_table_prob2;
			table[hack_dun_table_idx].prob3 = hack_dun_table_prob3;
		}
		r_info[hack_monster_idx].rarity = hack_monster_rarity;
	}
#endif

#ifndef ARCADE_SERVER
	if (!nr_bottom) {
		/* Place some traps in the dungeon */
		alloc_object(wpos, ALLOC_SET_BOTH, ALLOC_TYP_TRAP,
		    randint(k * (bonus ? 3 : 1)), p_ptr);

		/* Put some rubble in corridors */
		alloc_object(wpos, ALLOC_SET_CORR, ALLOC_TYP_RUBBLE, randint(k), p_ptr);

		/* Put some objects in rooms */
		alloc_object(wpos, ALLOC_SET_ROOM, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ROOM, 3) * dun->ratio / 100 + 1, p_ptr);

		/* Put some objects/gold in the dungeon */
		alloc_object(wpos, ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ITEM, 3) * dun->ratio / 100 + 1, p_ptr);
		alloc_object(wpos, ALLOC_SET_BOTH, ALLOC_TYP_GOLD, randnor(DUN_AMT_GOLD, 3) * dun->ratio / 100 + 1, p_ptr);

		/* Put some between gates */
		alloc_object(wpos, ALLOC_SET_ROOM, ALLOC_TYP_BETWEEN, randnor(DUN_AMT_BETWEEN, 3) * dun->ratio / 100 + 1, p_ptr);

		/* Put some fountains */
		alloc_object(wpos, ALLOC_SET_ROOM, fountains_of_blood ? ALLOC_TYP_FOUNTAIN_OF_BLOOD : ALLOC_TYP_FOUNTAIN, randnor(DUN_AMT_FOUNTAIN, 3) * dun->ratio / 100 + 1, p_ptr);
	}
	/* It's done */
#else
	if (!nr_bottom && wpos->wz < 0) {
		/* Place some traps in the dungeon */
		alloc_object(wpos, ALLOC_SET_BOTH, ALLOC_TYP_TRAP,
		    randint(k * (bonus ? 3 : 1)), p_ptr);

		/* Put some rubble in corridors */
		alloc_object(wpos, ALLOC_SET_CORR, ALLOC_TYP_RUBBLE, randint(k), p_ptr);

		/* Put some objects in rooms */
		alloc_object(wpos, ALLOC_SET_ROOM, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ROOM, 3) * dun->ratio / 100 + 1, p_ptr);

		/* Put some objects/gold in the dungeon */
		alloc_object(wpos, ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ITEM, 3) * dun->ratio / 100 + 1, p_ptr);
		alloc_object(wpos, ALLOC_SET_BOTH, ALLOC_TYP_GOLD, randnor(DUN_AMT_GOLD, 3) * dun->ratio / 100 + 1, p_ptr);

		/* Put some between gates */
		alloc_object(wpos, ALLOC_SET_ROOM, ALLOC_TYP_BETWEEN, randnor(DUN_AMT_BETWEEN, 3) * dun->ratio / 100 + 1, p_ptr);

		/* Put some fountains */
		alloc_object(wpos, ALLOC_SET_ROOM, fountains_of_blood ? ALLOC_TYP_FOUNTAIN_OF_BLOOD : ALLOC_TYP_FOUNTAIN, randnor(DUN_AMT_FOUNTAIN, 3) * dun->ratio / 100 + 1, p_ptr);
	}
	/* It's done */
#endif

#if 0
	/* Put some altars */	/* No god, no alter */
	alloc_object(ALLOC_SET_ROOM, ALLOC_TYP_ALTAR, randnor(DUN_AMT_ALTAR, 3) * dun->ratio / 100 + 1, p_ptr);
#endif

#ifdef ARCADE_SERVER
if(wpos->wz > 0) {
for(mx = 1; mx < 131; mx++) {
                for(my = 1; my < 43; my++) {
                cave_set_feat(wpos, my, mx, 1);
                }
        }
}
        if(wpos->wz > 0 && wpos->wz < 7) {

		for(my = 1; my < 8; my++) {
			for(mx = 63; mx < 70; mx++) {
				 cave_set_feat(wpos, my, mx, 209);
			}
		}
		for(my = 36; my < 43; my++) {
			for(mx = 63; mx < 70; mx++) {
				 cave_set_feat(wpos, my, mx, 209);
			}
		}
 		for(my = 19; my < 26; my++) {
                        for(mx = 1; mx < 8; mx++) {
                                 cave_set_feat(wpos, my, mx, 209);
                        }
                }
                for(my = 19; my < 26; my++) {
                        for(mx = 124; mx < 131; mx++) {
                                 cave_set_feat(wpos, my, mx, 209);
                        }
                }
	}	


        if (wpos->wz == 7) {
                for(mx = 1; mx<21; mx++) 
                cave_set_feat(wpos, 11, mx, 61);
                for(mx = 1; mx < 12; mx++)
                cave_set_feat(wpos, mx, 21, 61);
                }

if (wpos->wz == 9) {
for(mx = 1; mx < 131; mx++) {
        for(my = 1; my < 43; my++) {
                cave_set_feat(wpos, my, mx, 187);
                }
        }
}

#endif

#ifdef ENABLE_DOOR_CHECK
#if 0 /* special secondary check maybe, apart from the usual door_makes_no_sense() -- THIS IS JUST A DOODLE - NON FUNCTIONAL! */
	/* scan level for weird doors and remove them */
	for (x = 0; x < MAX_WID; x++)
		for (y = 0; y < MAX_HGT; y++) {
			k = zcave[y][x].feat;
			if ((k < FEAT_DOOR_HEAD || k > FEAT_DOOR_TAIL) && k != FEAT_SECRET) continue;
			if (door_makes_no_sense(zcave, x, y))
			    ;
			    //zcave[y][x] = FEAT_FLOOR;
	}
#endif
#endif

	level_generation_time = FALSE;


	/* A little evilness:
	   If Morgoth was generated on this level, make all vaults NO_TELE.
	   In addition to this, Morgoth's 'live-spawn' instead of being
	   generated along with the level should be prevented (place_monster_one in monster2.c). */
#ifdef MORGOTH_NO_TELE_VAULTS
	for (y = 1; y < dun->l_ptr->hgt - 1; y++) {
		for (x = 1; x < dun->l_ptr->wid - 1; x++) {
			cr_ptr = &zcave[y][x];
			if (cr_ptr->m_idx) {
				/* Check if Morgoth was just generated along with this dungeon level */
			        m_ptr = &m_list[cr_ptr->m_idx];
				if (m_ptr->r_idx == RI_MORGOTH) morgoth_inside = TRUE;
			}
			if (morgoth_inside) break;
		}
		if (morgoth_inside) break;
	}
	if (morgoth_inside) {
		/* make all vaults NO_TELE */
		for (y = 1; y < dun->l_ptr->hgt - 1; y++) {
			for (x = 1; x < dun->l_ptr->wid - 1; x++) {
				cr_ptr = &zcave[y][x];
				/* Only those vaults with PERMA_WALL walls */
				if (cr_ptr->info & (CAVE_ICKY_PERMA)) cr_ptr->info |= (CAVE_STCK);
			}
		}
	}
#endif


	/* Check for half void jump gates, resulting from partial vaults
	   which contain void gates and were cut at the level border - C. Blue */
/*..	*/


	/* Create secret dungeon shop entrances (never on Morgoth's depth) -C. Blue */

	/* No stores in Valinor */
	if (in_valinor(wpos)
	    /* No dungeon shops on final floor of a dungeon */
	    || d_ptr->maxdepth == ABS(wpos->wz)
	    /* No dungeon shops in sector00 dungeons */
	    || in_sector00_dun(wpos)
	    /* No dungeon shops in highlander dungeon (usually same as sector00 dungeon) */
	    || in_highlander_dun(wpos)
	    /* No dungeon shops in AMC */
	    || in_arena(wpos)
	    /* No dungeon shops in PvP arena */
	    || in_pvparena(wpos))
		return;

	/* Nether Realm has an overriding shop creation routine. */
	if (!netherrealm_level) {
		bool store_failed = FALSE; /* avoid checking for a different type of store if one already failed, warping probabilities around */

		/* Check for building deep store (Rare & expensive stores) */
		if ((!dungeon_store_timer) && (dun_lev >= 60) && (dun_lev != 100))
			build_special_store = 1;

		/* Not for special dungeons (easy csw/resattr): */
		//if ((!challenge_dungeon && !(d_ptr->flags3 & DF3_NO_SIMPLE_STORES)) ||
		if (!(d_ptr->flags3 & DF3_NO_SIMPLE_STORES)) {
			/* Check for building low-level store (Herbalist) */
			if ((!build_special_store) &&
			    (!dungeon_store2_timer) && (dun_lev >= 10) && (dun_lev <= 30))
				build_special_store = 2;

			/* Build one of several misc stores for basic items of certain type */
			//todo: maybe use the new d_ptr->store_timer for randomly generated stores
#ifdef TEST_SERVER
			if (!store_failed && (!build_special_store) && (dun_lev >= 13)) {
				if (!rand_int(5)) build_special_store = 3;
				else store_failed = TRUE;
			}
#else
 #ifdef RPG_SERVER
			if (!store_failed && (!build_special_store) && (d_ptr->flags2 & DF2_IRON) && (dun_lev >= 13)) {
				//((dun_lev + rand_int(3) - 1) % 5 == 0)) build_special_store = 3;
				if (!rand_int(5)) build_special_store = 3;
				else store_failed = TRUE;
			}
 #endif
#endif
		}

		/* Build one of several misc stores for basic items of certain type, like on RPG server above */
		if (!store_failed && (!build_special_store) && (d_ptr->flags2 & DF2_MISC_STORES) && (dun_lev >= 13)) {
			if (!rand_int(5)) build_special_store = 3;
			else store_failed = TRUE;
		}

		/* Build hidden library if desired (good for challenge dungeons actually) */
		if (//!store_failed &&
		    (!build_special_store) && (d_ptr->flags3 & DF3_HIDDENLIB) && (dun_lev >= 8)) {
			if (!rand_int(dun_lev / 2 + 1)) build_special_store = 4;
			else store_failed = TRUE;
		}

		/* Build deep supplies store if desired (good for challenge dungeons actually) */
		if (//!store_failed &&
		    (!build_special_store) && (d_ptr->flags3 & DF3_DEEPSUPPLY) && (dun_lev >= 80)) {
			if (!rand_int(5)) build_special_store = 5;
			else {
				s_printf("DUNGEON_STORE: DEEPSUPPLY roll failed.\n");
				store_failed = TRUE;
			}
		}

		/* if failed, we're done */
		if (!build_special_store) return;
		/* reset deep shop timeout */
		if (build_special_store == 1)
			dungeon_store_timer = 2 + rand_int(cfg.dungeon_shop_timeout); /* reset timeout (in minutes) */
		/* reset low-level shop timeout */
		if (build_special_store == 2)
			dungeon_store2_timer = 2 + rand_int(cfg.dungeon_shop_timeout); /* reset timeout (in minutes) */
	/* build only one special shop in the Nether Realm */
	} else if (((dun_lev - 166) % 5 != 0) || (dun_lev == netherrealm_end)) return;

	/* Try to create a dungeon store */
	if ((rand_int(1000) < cfg.dungeon_shop_chance) || netherrealm_level ||
	    (build_special_store == 3 || build_special_store == 4 || build_special_store == 5)) {
		/* Try hard to place one */
		for (i = 0; i < 300; i++) {
			y = rand_int(dun->l_ptr->hgt-4)+2;
			x = rand_int(dun->l_ptr->wid-4)+2;
			csbm_ptr = &zcave[y][x];
			/* Must be in a wall, must not be in perma wall. Outside of vaults. */
			if ((f_info[csbm_ptr->feat].flags1 & FF1_WALL) &&
			    !(f_info[csbm_ptr->feat].flags1 & FF1_PERMANENT) &&
			    !(csbm_ptr->info & CAVE_ICKY))
			{
				/* must have at least 1 'free' adjacent field */
				bool found1free = FALSE;
				for (k = 0; k < 8; k++) {
					switch (k) {
					case 0: x1 = x - 1; y1 = y - 1; break;
					case 1: x1 = x; y1 = y - 1; break;
					case 2: x1 = x + 1; y1 = y - 1; break;
					case 3: x1 = x + 1; y1 = y; break;
					case 4: x1 = x + 1; y1 = y + 1; break;
					case 5: x1 = x; y1 = y + 1; break;
					case 6: x1 = x - 1; y1 = y + 1; break;
					case 7: x1 = x - 1; y1 = y; break;
					}
					cr_ptr = &zcave[y1][x1];
					if (!(f_info[cr_ptr->feat].flags1 & FF1_WALL))
						found1free = TRUE;
				}

				if (found1free) {
					/* Must have at least 3 solid adjacent fields,
					that are also adjacent to each other,
					and the 3-chain mustn't start at a corner! >:) Mwhahaha.. */
					int r = 0; /* Counts adjacent solid fields */
					for (k = 0; k < (8 - 1 + 3); k++)
					if (r < 3) {
						switch (k % 8) {
						case 0:x1 = x - 1; y1 = y - 1; break;
						case 1:x1 = x; y1 = y - 1; break;
						case 2:x1 = x + 1; y1 = y - 1; break;
						case 3:x1 = x + 1; y1 = y; break;
						case 4:x1 = x + 1; y1 = y + 1; break;
						case 5:x1 = x; y1 = y + 1; break;
						case 6:x1 = x - 1; y1 = y + 1; break;
						case 7:x1 = x - 1; y1 = y; break;
						}
						cr_ptr = &zcave[y1][x1];
						if ((f_info[cr_ptr->feat].flags1 & FF1_WALL)) {
							if (r != 0) r++;
							/* chain mustn't start in a diagonal corner..: */
							if ((r == 0) && ((k % 2) == 1)) r++;
						} else {
							r = 0;
						}
					}
					if (r >= 3) {
						/* Ok, create a secret store! */
						if ((cs_ptr = AddCS(csbm_ptr, CS_SHOP))) {
							csbm_ptr->feat = FEAT_SHOP;
							csbm_ptr->info |= CAVE_NOPK;
							/* Declare this to be a room & illuminate */
							csbm_ptr->info |= CAVE_ROOM | CAVE_GLOW;
							if (!netherrealm_level) {
								if (build_special_store == 1) {
									if (cfg.dungeon_shop_type == 999){
										switch (rand_int(3)) {
										case 1:cs_ptr->sc.omni = STORE_JEWELX;break; /*Rare Jewelry Shop */
										case 2:cs_ptr->sc.omni = STORE_SHOESX;break; /*Rare Footwear Shop */
										default: cs_ptr->sc.omni = STORE_BLACKS;break; /*The Secret Black Market */
										}
									} else {
										cs_ptr->sc.omni = cfg.dungeon_shop_type;
									}
								} else if (build_special_store == 2) {
									cs_ptr->sc.omni = STORE_HERBALIST;
								} else if (build_special_store == 3) {
									//Add specialist stores? - the_sandman
									switch (rand_int(7)) {
									/*case 1: cs_ptr->sc.omni = STORE_SPEC_AXE;break;
									case 2: cs_ptr->sc.omni = STORE_SPEC_BLUNT;break;
									case 3: cs_ptr->sc.omni = STORE_SPEC_POLE;break;
									case 4: cs_ptr->sc.omni = STORE_SPEC_SWORD;break;*/
									case 0: cs_ptr->sc.omni = STORE_HIDDENLIBRARY; break;
									case 1: case 2: cs_ptr->sc.omni = STORE_SPEC_CLOSECOMBAT; break;
									case 3: cs_ptr->sc.omni = STORE_SPEC_POTION; break;
									case 4: cs_ptr->sc.omni = STORE_SPEC_SCROLL; break;
									case 5: cs_ptr->sc.omni = STORE_SPEC_ARCHER; break;
									default: cs_ptr->sc.omni = STORE_STRADER; break;
									}
								} else if (build_special_store == 4)
									cs_ptr->sc.omni = STORE_HIDDENLIBRARY;
								else if (build_special_store == 5)
									cs_ptr->sc.omni = STORE_DEEPSUPPLY;
							} else {
								cs_ptr->sc.omni = STORE_BTSUPPLY;
							}
							s_printf("DUNGEON_STORE: %d (%d,%d,%d)\n", cs_ptr->sc.omni, wpos->wx, wpos->wy, wpos->wz);
							return;
						}
					}
				}
			}
		}

		/* Creation failed because no spot was found! */
		/* So let's allow it on the next level then.. */
		if (!netherrealm_level) {
			if (build_special_store == 1)
				dungeon_store_timer = 0;
			else
				dungeon_store2_timer = 0;
		}
	}
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
/*
 * XXX hrm apartment allows wraithes to intrude..
 */
/* NOTE: This function no longer is used for normal town generation;
 * this can be used to build 'extra' towns for some purpose, though.
 */
//TL;DR: nowadays _only_ used in town_gen_hack(), for dungeon (ironman) towns - C. Blue
static void build_store(struct worldpos *wpos, int n, int yy, int xx) {
	int i, y, x, y0, x0, y1, x1, y2, x2, tmp;
	int size = 0;
	cave_type *c_ptr;
	bool flat = FALSE, trad = FALSE;
	struct c_special *cs_ptr;

	/* turn ponds into lava pools if floor is usually lava, maybe change other feats too */
	bool lava_floor = FALSE;
	dungeon_type *d_ptr = getdungeon(wpos);
	int dun_type = 0;

	if (d_ptr) {
#ifdef IRONDEEPDIVE_MIXED_TYPES
		if (in_irondeepdive(wpos)) dun_type = iddc[ABS(wpos->wz)].type;
		else
#endif
		dun_type = d_ptr->theme ? d_ptr->theme : d_ptr->type;

		for (i = 0; i < 5; i++)
			if (d_info[dun_type].floor[i] == FEAT_SHAL_LAVA ||
			    d_info[dun_type].floor[i] == FEAT_DEEP_LAVA)
				lava_floor = TRUE;
		//d_info[iddc[ABS(wpos->wz)].next].floor[i];
	}

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return; /*multitowns*/

	y0 = yy * 11 + 5;
	x0 = xx * 16 + 12;

	/* Determine the store boundaries */
	y1 = y0 - randint((yy == 0) ? 3 : 2);
	y2 = y0 + randint((yy == 1) ? 3 : 2);
	x1 = x0 - randint(5);
	x2 = x0 + randint(5);

	/* Hack -- make building 9's as large as possible */
	if (n == STORE_NINE) {
		y1 = y0 - 3;
		y2 = y0 + 3;
		x1 = x0 - 5;
		x2 = x0 + 5;
	}

	/* Hack -- make building 8's larger */
	if (n == STORE_HOME || n == STORE_HOME_DUN) {
		y1 = y0 - 1 - randint(2);
		y2 = y0 + 1 + randint(2);
		x1 = x0 - 3 - randint(2);
		x2 = x0 + 3 + randint(2);
	}

	/* Hack -- try 'apartment house' */
	if (n == STORE_HOUSE && magik(60)) {
		y1 = y0 - 1 - randint(2);
		y2 = y0 + 1 + randint(2);
		x1 = x0 - 3 - randint(3);
		x2 = x0 + 3 + randint(2);
		if ((x2 - x1) % 2) x2--;
		if ((y2 - y1) % 2) y2--;
		if ((x2 - x1) >= 4 && (y2 - y1) >= 4 && magik(60)) flat = TRUE;
	}

	/* Hack -- try 'apartment house' */
	if (n == STORE_HOUSE) {
		if (!magik(MANG_HOUSE_RATE)) trad = TRUE;
	}

	/* Build an invulnerable rectangular building */
	for (y = y1; y <= y2; y++) {
		for (x = x1; x <= x2; x++) {
			/* Get the grid */
			c_ptr = &zcave[y][x];

			/* Clear previous contents, add "basic" perma-wall */
			c_ptr->feat = (n == 13) ? FEAT_WALL_HOUSE : FEAT_PERM_EXTRA;

			/* Hack -- The buildings are illuminated and known */
			/* c_ptr->info |= (CAVE_GLOW); */
		}
	}

	/* Hack -- Make store "8" (the old home) empty */
	if (n == STORE_HOME || n == STORE_HOME_DUN) {
		for (y = y1 + 1; y < y2; y++) {
			for (x = x1 + 1; x < x2; x++) {
				/* Get the grid */
				c_ptr = &zcave[y][x];

				/* Clear contents */
				c_ptr->feat = FEAT_FLOOR;

				/* Declare this to be a room */
				c_ptr->info |= (CAVE_ROOM | CAVE_GLOW | CAVE_NOPK);
			}
		}

		/* Hack -- have everyone start in the tavern */
		new_level_down_y(wpos, (y1 + y2) / 2);
		new_level_down_x(wpos, (x1 + x2) / 2);
	}

	/* Make doorways, fill with grass and trees */
	if (n == STORE_DOORWAY) {
		for (y = y1 + 1; y < y2; y++) {
			for (x = x1 + 1; x < x2; x++) {
				/* Get the grid */
				c_ptr = &zcave[y][x];

				/* Clear contents */
				c_ptr->feat = FEAT_GRASS;
			}
		}

		y = (y1 + y2) / 2;
		x = (x1 + x2) / 2;

		zcave[y1][x].feat = FEAT_GRASS;
		zcave[y2][x].feat = FEAT_GRASS;
		zcave[y][x1].feat = FEAT_GRASS;
		zcave[y][x2].feat = FEAT_GRASS;

		for (i = 0; i < 5; i++) {
			y = rand_range(y1 + 1, y2 - 1);
			x = rand_range(x1 + 1, x2 - 1);

			c_ptr = &zcave[y][x];

			c_ptr->feat = lava_floor ? FEAT_DEAD_TREE: FEAT_TREE;
		}

		return;
	}
			
	/* Pond */
	if (n == STORE_POND) {
		for (y = y1; y <= y2; y++) {
			for (x = x1; x <= x2; x++) {
				/* Get the grid */
				c_ptr = &zcave[y][x];

				/* Fill with water */
				c_ptr->feat = lava_floor ? FEAT_DEEP_LAVA : FEAT_DEEP_WATER;
			}
		}

		/* Make the pond not so "square" */
		zcave[y1][x1].feat = FEAT_DIRT;
		zcave[y1][x2].feat = FEAT_DIRT;
		zcave[y2][x1].feat = FEAT_DIRT;
		zcave[y2][x2].feat = FEAT_DIRT;

		return;
	}

	/* Building with stairs */
	if (n == STORE_FEAT_MORE || n == STORE_FEAT_LESS) {
		for (y = y1; y <= y2; y++) {
			for (x = x1; x <= x2; x++) {
				/* Get the grid */
				c_ptr = &zcave[y][x];

				/* Empty it */
				c_ptr->feat = FEAT_FLOOR;

				/* Put some grass */
				if (randint(100) < 50)
					c_ptr->feat = FEAT_GRASS;
				c_ptr->info |= CAVE_NOPK;
			}
		}

		x = (x1 + x2) / 2;
		y = (y1 + y2) / 2;

		/* Access the stair grid */
		c_ptr = &zcave[y][x];

		/* Clear previous contents, add down stairs */
		if (n == STORE_FEAT_MORE) {
			c_ptr->feat = FEAT_MORE;
			new_level_up_y(wpos, y);
			new_level_up_x(wpos, x);
#if 0 /* moved down, see below. Should be ok/better? */
			new_level_rand_y(wpos, y);
			new_level_rand_x(wpos, x);
#endif
		} else {
			c_ptr->feat = FEAT_LESS;
			new_level_down_y(wpos, y);
			new_level_down_x(wpos, x);
		}
		new_level_rand_y(wpos, y);
		new_level_rand_x(wpos, x);
		return;
	}

	/* Forest */
	if (n == STORE_FOREST) {
		int xc, yc, max_dis;

		/* Find the center of the forested area */
		xc = (x1 + x2) / 2;
		yc = (y1 + y2) / 2;

		/* Find the max distance from center */
		max_dis = distance(y2, x2, yc, xc);

		for (y = y1; y <= y2; y++) {
			for (x = x1; x <= x2; x++) {
				int chance;

				/* Get the grid */
				c_ptr = &zcave[y][x];

				/* Empty it */
				c_ptr->feat = FEAT_GRASS;

				/* Calculate chance of a tree */
				chance = 100 * (distance(y, x, yc, xc));
				chance /= max_dis;
				chance = 80 - chance;

				/* Put some trees */
				if (randint(100) < chance)
					c_ptr->feat = lava_floor ? FEAT_DEAD_TREE : FEAT_TREE;
			}
		}

		return;
	}

	/* House */
	if (n == STORE_HOUSE) {
#ifdef USE_MANG_HOUSE
		if (!trad) {
			for (y = y1 + 1; y < y2; y++) {
				for (x = x1 + 1; x < x2; x++) {
					/* Get the grid */
					c_ptr = &zcave[y][x];

					/* Fill with floor */
					c_ptr->feat = FEAT_FLOOR;

					/* Make it "icky" */
					c_ptr->info |= CAVE_ICKY;
				}
			}
		}
#endif	/* USE_MANG_HOUSE */

		if (!flat) {
			/* Setup some "house info" */
			size = (x2 - x1 - 1) * (y2 - y1 - 1);

			MAKE(houses[num_houses].dna, struct dna_type);
			houses[num_houses].x = x1;
			houses[num_houses].y = y1;
			houses[num_houses].flags = HF_RECT | HF_STOCK;
			if (trad) houses[num_houses].flags |= HF_TRAD;
			houses[num_houses].coords.rect.width = x2 - x1 + 1;
			houses[num_houses].coords.rect.height = y2 - y1 + 1;
			wpcopy(&houses[num_houses].wpos, wpos);
			houses[num_houses].dna->price = initial_house_price(&houses[num_houses]);
		}
		/* Hack -- apartment house */
		else {
			int doory = 0, doorx = 0, dy, dx;
			struct c_special *cs_ptr;
			if (magik(75)) doorx = rand_int((x2 - x1 - 1) / 2);
			else doory = rand_int((y2 - y1 - 1) / 2);

			for (x = x1; x <= x2; x++) {
				/* Get the grid */
				c_ptr = &zcave[(y1 + y2) / 2][x];

				/* Clear previous contents, add "basic" perma-wall */
				c_ptr->feat = FEAT_PERM_EXTRA;
			}

			for (y = y1; y <= y2; y++) {
				/* Get the grid */
				c_ptr = &zcave[y][(x1 + x2) / 2];

				/* Clear previous contents, add "basic" perma-wall */
				c_ptr->feat = FEAT_PERM_EXTRA;
			}

			/* Setup some "house info" */
			size = ((x2 - x1) / 2 - 1) * ((y2 - y1) / 2 - 1);

			for (i = 0; i < 4; i++) {
#if 1
				dx = (i < 2 ? x1 : x2);
				dy = ((i % 2) ? y2 : y1);
#endif	/* 1 */
				x = ( i < 2  ? x1 : x1 + (x2 - x1) / 2);
				y = ((i % 2) ? y1 + (y2 - y1) / 2 : y1);
				c_ptr = &zcave[y][x];

				/* Remember price */
				MAKE(houses[num_houses].dna, struct dna_type);
				houses[num_houses].x = x;
				houses[num_houses].y = y;
				houses[num_houses].flags = HF_RECT | HF_STOCK | HF_APART;
				if (trad) houses[num_houses].flags |= HF_TRAD;
				houses[num_houses].coords.rect.width = (x2 - x1) / 2 + 1;
				houses[num_houses].coords.rect.height = (y2 - y1) / 2 + 1;
				wpcopy(&houses[num_houses].wpos, wpos);
				houses[num_houses].dna->price = initial_house_price(&houses[num_houses]);

				/* MEGAHACK -- add doors here and return */

#if 1	/* savedata sometimes forget about it and crashes.. */
				dx += (i < 2 ? doorx : 0 - doorx);
				dy += ((i % 2) ? 0 - doory : doory);
#endif	/* 1 */

				c_ptr = &zcave[dy][dx];

				/* hack -- only create houses that aren't already loaded from disk */
				if ((tmp = pick_house(wpos, dy, dx)) == -1) {
					/* Store door location information */
					c_ptr->feat = FEAT_HOME;
					if ((cs_ptr = AddCS(c_ptr, CS_DNADOOR))) {
						cs_ptr->sc.ptr = houses[num_houses].dna;
					}
					houses[num_houses].dx = dx;
					houses[num_houses].dy = dy;
					houses[num_houses].dna->creator = 0L;
					houses[num_houses].dna->owner = 0L;

#ifndef USE_MANG_HOUSE_ONLY
					/* This can be changed later */
					/* XXX maybe new owner will be unhappy if size>STORE_INVEN_MAX;
					 * this will be fixed when STORE_INVEN_MAX will be removed. - Jir
					 */
					if (trad) {
						size = (size >= STORE_INVEN_MAX) ? STORE_INVEN_MAX : size;
						houses[num_houses].stock_size = size;
						houses[num_houses].stock_num = 0;
						/* TODO: pre-allocate some when launching the server */
						C_MAKE(houses[num_houses].stock, size, object_type);
					} else {
						houses[num_houses].stock_size = 0;
						houses[num_houses].stock_num = 0;
					}
#endif	/* USE_MANG_HOUSE */

					/* One more house */
					num_houses++;
					if ((house_alloc - num_houses) < 32) {
						GROW(houses, house_alloc, house_alloc + 512, house_type);
						GROW(houses_bak, house_alloc, house_alloc + 512, house_type);
						house_alloc += 512;
					}
				} else {
					KILL(houses[num_houses].dna, struct dna_type);
					c_ptr->feat = FEAT_HOME;
					if((cs_ptr = AddCS(c_ptr, CS_DNADOOR))){
						cs_ptr->sc.ptr = houses[tmp].dna;
					}
				}
			}

			return;
		}
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
	switch (tmp) {
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
	c_ptr = &zcave[y][x];

	/* Some buildings get special doors */
	if (n == STORE_HOUSE && !flat) {
		/* hack -- only create houses that aren't already loaded from disk */
		if ((i = pick_house(wpos, y, x)) == -1) {
			/* Store door location information */
			c_ptr->feat = FEAT_HOME;
			if ((cs_ptr = AddCS(c_ptr, CS_DNADOOR))){
				cs_ptr->sc.ptr = houses[num_houses].dna;
			}
			houses[num_houses].dx = x;
			houses[num_houses].dy = y;
			houses[num_houses].dna->creator = 0L;
			houses[num_houses].dna->owner = 0L;

#ifndef USE_MANG_HOUSE_ONLY
			/* This can be changed later */
			/* XXX maybe new owner will be unhappy if size>STORE_INVEN_MAX;
			 * this will be fixed when STORE_INVEN_MAX will be removed. - Jir
			 */
			if (trad) {
				size = (size >= STORE_INVEN_MAX) ? STORE_INVEN_MAX : size;
				houses[num_houses].stock_size = size;
				houses[num_houses].stock_num = 0;
				/* TODO: pre-allocate some when launching the server */
				C_MAKE(houses[num_houses].stock, size, object_type);
			} else {
				houses[num_houses].stock_size = 0;
				houses[num_houses].stock_num = 0;
			}
#endif	/* USE_MANG_HOUSE_ONLY */

			/* One more house */
			num_houses++;
			if ((house_alloc - num_houses) < 32) {
				GROW(houses, house_alloc, house_alloc + 512, house_type);
				GROW(houses_bak, house_alloc, house_alloc + 512, house_type);
				house_alloc += 512;
			}
		}
		else {
			KILL(houses[num_houses].dna, struct dna_type);
			c_ptr->feat = FEAT_HOME;
			if((cs_ptr = AddCS(c_ptr, CS_DNADOOR))){
				cs_ptr->sc.ptr = houses[i].dna;
			}
		}
	}
	else if (n == STORE_AUCTION) /* auctionhouse */
	{
		c_ptr->feat = FEAT_PERM_EXTRA; /* wants to work */
	
	} else {
		/* Clear previous contents, add a store door */
		c_ptr->feat = FEAT_SHOP;	/* TODO: put CS_SHOP */
		c_ptr->info |= CAVE_NOPK;

		if ((cs_ptr = AddCS(c_ptr, CS_SHOP)))
			cs_ptr->sc.omni = n;

		for (y1 = y - 1; y1 <= y + 1; y1++) {
			for (x1 = x - 1; x1 <= x + 1; x1++) {
				/* Get the grid */
				c_ptr = &zcave[y1][x1];

				/* Declare this to be a room */
				c_ptr->info |= CAVE_ROOM | CAVE_GLOW;
				/* Illuminate the store */
			}
		}
	}
}

/*
 * Build a road.
 */
static void place_street(struct worldpos *wpos, int vert, int place)
{
	int y, x, y1, y2, x1, x2;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Vertical streets */
	if (vert) {
		x = place * 32 + 20;
		x1 = x - 2;
		x2 = x + 2;

		y1 = 5;
		y2 = MAX_HGT - 5;
	} else {
		y = place * 22 + 10;
		y1 = y - 2;
		y2 = y + 2;

		x1 = 5;
		x2 = MAX_WID - 5;
	}

	for (y = y1; y <= y2; y++) {
		for (x = x1; x <= x2; x++) {
			if (zcave[y][x].feat != FEAT_FLOOR)
				zcave[y][x].feat = FEAT_GRASS;
		}
	}

	if (vert) {
		x1++;
		x2--;
	} else {
		y1++;
		y2--;
	}

	for (y = y1; y <= y2; y++) {
		for (x = x1; x <= x2; x++) {
			zcave[y][x].feat = FEAT_FLOOR;
		}
	}
}

static void switchable_shop_grids(cave_type **zcave) {
	int x, y, x2, y2;

	/* apply player-switchability so noone cannot block the store too badly */
	for (x = 1; x < MAX_WID - 1; x++) {
		for (y = 1; y < MAX_HGT - 1; y++) {
			if (zcave[y][x].feat != FEAT_SHOP
			    /* let's not allow to chain-switch someone out of the inn.. o_o */
			    || zcave[y][x].feat == FEAT_PROTECTED)
				continue;
			for (x2 = x - 1; x2 <= x + 1; x2++) {
				for (y2 = y - 1; y2 <= y + 1; y2++) {
					if (!in_bounds(y2, x2)) continue;
					zcave[y2][x2].info |= CAVE_SWITCH;
				}
			}
		}
	}
}


/*
 * Generate the "consistent" town features, and place the player
 *
 * Hack -- play with the R.N.G. to always yield the same town
 * layout, including the size and shape of the buildings, the
 * locations of the doorways, and the location of the stairs.
 *
 * C. Blue: Modified/extended for creation of dungeon towns,
 *          especially for ironman dungeons/towers.
 */

/* (For dungeon towns.) Don't clutter all stores in the
   center, but spread them out a bit - C. Blue: */
#define NEW_TOWNGENHACK_METHOD

static void town_gen_hack(struct worldpos *wpos) {
	bool dungeon_town = (wpos->wz != 0);
	wilderness_type *wild = &wild_info[wpos->wy][wpos->wx];
	struct dungeon_type *d_ptr = wpos->wz != 0 ? (wpos->wz > 0 ? wild->tower : wild->dungeon) : NULL;

	int y, x, k, n;
//	int max_rooms = (max_st_idx > 72 ? 72 : max_st_idx), base_stores = MAX_BASE_STORES; <- not working, because the amount of buildings will be too small -> repeated-stores-glitch appears.
	int max_rooms = 72, base_stores = MAX_BASE_STORES;
#ifdef NEW_TOWNGENHACK_METHOD
	int posx[base_stores], posy[base_stores], pos[6 * 4];
#endif
#ifdef DEVEL_TOWN_COMPATIBILITY
	/* -APD- the location of the auction house */
	int auction_x = -1, auction_y = -1;
#endif
	int rooms[max_rooms];

	cave_type **zcave, *c_ptr;
	if (!(zcave = getcave(wpos))) return;


	/* Hack -- Use the "simple" RNG */
	Rand_quick = TRUE;

	/* Hack -- Induce consistant town layout */
#if 0 /* was ok, but now.. */
	Rand_value = seed_town + (wpos->wx + wpos->wy * MAX_WILD_X);
#else /* ..we have ironman towns in the dungeon, too - C. Blue */
	/* Well, actually we could just use completely random seed for wpos != 0
	   and just x,y dependant seed for wpos == 0. Might even be preferrable. */
	if (!wpos->wz) Rand_value = seed_town + (wpos->wx + wpos->wy * MAX_WILD_X);
	else
		//Rand_value = seed_town + (wpos->wx + wpos->wy * MAX_WILD_X + wpos->wz * MAX_WILD_X * MAX_WILD_Y); /* Always same dungeon town at particular wpos */
		Rand_value = seed_town + (wpos->wx + wpos->wy * MAX_WILD_X) + turn / (cfg.fps * 3600 * 24); /* Different dungeon towns every 24h! */
#endif


#ifdef IRONDEEPDIVE_FIXED_TOWNS
	/* predefined town layout instead of generating randomly? */
	k = getlevel(wpos);
	if (is_fixed_irondeepdive_town(wpos, k)) {
		/* Kurzel suggested to use the cities of Menegroth and Nargothrond, seems good */

		/* 2000ft - generate Menegroth */
		if (k == IDDC_TOWN1_FIXED) {
			/* overlay town */
			x = y = 0;
			process_dungeon_file("t_menegroth.txt", wpos, &y, &x, MAX_HGT, MAX_WID, TRUE);

			/* prepare basic floor */
			for (y = 1; y < MAX_HGT - 1; y++) {
				for (x = 1; x < MAX_WID - 1; x++) {
					c_ptr = &zcave[y][x];
					/* hack: fix staircase direction to match the dungeon type */
					if (c_ptr->feat == FEAT_MORE && wpos->wz > 0) c_ptr->feat = FEAT_LESS;

					/* convert remaining empty grids to floor */
					if (c_ptr->feat) continue;

					c_ptr->feat = FEAT_DIRT;
					if (!rand_int(5)) c_ptr->feat = FEAT_GRASS;
				}
			}
		}

		/* 4000ft - generate Nargothrond */
		else if (k == IDDC_TOWN2_FIXED) {
			/* overlay town */
			x = y = 0;
			process_dungeon_file("t_nargothrond.txt", wpos, &y, &x, MAX_HGT, MAX_WID, TRUE);

			/* prepare basic floor */
			for (y = 1; y < MAX_HGT - 1; y++) {
				for (x = 1; x < MAX_WID - 1; x++) {
					c_ptr = &zcave[y][x];

					/* hack: fix staircase direction to match the dungeon type */
					if (c_ptr->feat == FEAT_MORE && wpos->wz > 0) c_ptr->feat = FEAT_LESS;

					/* convert remaining empty grids to floor */
					if (c_ptr->feat) continue;

					c_ptr->feat = FEAT_FLOOR;
					if (rand_int(4)) c_ptr->feat = FEAT_GRASS;
				}
			}
		}

		/* Hack -- use the "complex" RNG -- and we're done here! */
		Rand_quick = FALSE;
		switchable_shop_grids(zcave);
		return;
	}
#endif


	/* Hack -- Start with basic floors */
	for (y = 1; y < MAX_HGT - 1; y++) {
		for (x = 1; x < MAX_WID - 1; x++) {
			c_ptr = &zcave[y][x];
			/* Clear all features, set to "empty floor" */
#ifdef IRONDEEPDIVE_MIXED_TYPES
			if (in_irondeepdive(wpos)) {
				c_ptr->feat = d_info[iddc[ABS(wpos->wz)].type].floor[4]; //avoid strange [0] dominant types (4 is transition safe)
				if (rand_int(4)) c_ptr->feat = d_info[iddc[ABS(wpos->wz)].type].floor[0]; //strange types are OK if few?
				else if (rand_int(100) < 15) c_ptr->feat = d_info[iddc[ABS(wpos->wz)].type].fill_type[0]; //basic "granite"
			} else
#endif
			{ /* any default town */
				c_ptr->feat = FEAT_DIRT;

				if (rand_int(4)) c_ptr->feat = FEAT_GRASS;
				else if (rand_int(100) < 15) c_ptr->feat = FEAT_TREE; /* gotta add FEAT_BUSH without screwing up the rng seed */
			}
		}
	}

	/* Place horizontal "streets" */
	for (y = 0; y < 3; y++)
		place_street(wpos, 0, y);

	/* Place vertical "streets" */
	for (x = 0; x < 6; x++)
		place_street(wpos, 1, x);

#if 0 /* disable 'deprecated' auction house for paranoia? */
#ifdef DEVEL_TOWN_COMPATIBILITY
	/* -APD- place the auction house near the central stores */
	auction_x = rand_int(5) + 3;
	if ( (auction_x == 3) || (auction_x == 8) ) auction_y = rand_int(1) + 1;
	else auction_y = (rand_int(1) * 3) + 1; /* 1 or 4 */
#endif
#endif

	/* Prepare an Array of "remaining stores", and count them */
	for (n = 0; n < base_stores; n++) {
		/* For dungeon towns, skip '8' since it allows exploiting 100% riskfree combat.
		   Not true anymore, if STORE_HOME_DUN is used instead. But there is no home in
		   the middle of the dungeon anyway. >:p */
		if (n == STORE_HOME && dungeon_town) rooms[n] = STORE_POND;
		/* allocate base store
		   (dungeon towns use offset of +70 to avoid collision with normal town stores) */
		else rooms[n] = n + (dungeon_town ? STORE_GENERAL_DUN : 0);
	}
	for (n = base_stores; n < base_stores + 10; n++) rooms[n] = STORE_POND;
	for (n = base_stores + 10; n < base_stores + 20; n++) rooms[n] = STORE_FOREST;
	for (n = base_stores + 20; n < base_stores + 40; n++) rooms[n] = STORE_DOORWAY;
	for (n = base_stores + 40; n < max_rooms; n++) rooms[n] = STORE_HOUSE;
#if 0 /* what is/was STORE_NINE? disabling it for now */
	for (n = max_rooms - 10; n < max_rooms - 7; n++) rooms[n] = STORE_NINE;
#endif

	/* staircases */
	if (wild->flags & WILD_F_DOWN)
		rooms[max_rooms - 2] = STORE_FEAT_MORE;
	if (wild->flags & WILD_F_UP)
		rooms[max_rooms - 1] = STORE_FEAT_LESS;

	/* Dungeon towns get some extra treatment: */
	if (dungeon_town) {
		/* make dungeon towns look more destroyed */
		if (rand_int(2)) build_streamer(wpos, FEAT_TREE, 0, TRUE);
		if (rand_int(2)) build_streamer(wpos, FEAT_IVY, 0, TRUE);
#if 0 /* these obstruct stores a bit too much maybe */
		if (rand_int(2)) build_streamer(wpos, FEAT_QUARTZ, 0, TRUE);
		if (rand_int(2)) build_streamer(wpos, FEAT_MAGMA, 0, TRUE);
#endif
		if (rand_int(2)) {
			if (rand_int(2)) build_streamer(wpos, FEAT_SHAL_WATER, 0, TRUE);
			if (rand_int(2)) build_streamer(wpos, FEAT_DEEP_WATER, 0, TRUE);
		} else {
			if (rand_int(2)) build_streamer(wpos, FEAT_SHAL_LAVA, 0, TRUE);
			if (rand_int(2)) build_streamer(wpos, FEAT_DEEP_LAVA, 0, TRUE);
		}

		/* For ironman dungeons: Set x,y coords when entering the level,
		   since we don't place a 'staircase room' in the opposite
		   direction that would set the coords for us. */
		if ((d_ptr->flags1 & (DF1_NO_UP | DF1_FORCE_DOWN)) || (d_ptr->flags2 & DF2_IRON)) {
			n = 1000;
			while (--n) {
				x = rand_int(MAX_WID - 2) + 1;
				y = rand_int(MAX_HGT - 2) + 1;
				if (cave_floor_bold(zcave, y, x)) break;
			}
			if (!n) {
				x = 2;
				y = 2;
			}
			if (wpos->wz < 0) {
				new_level_down_x(wpos, x);
				new_level_down_y(wpos, y);
			} else {
				new_level_up_x(wpos, x);
				new_level_up_y(wpos, y);
			}
		}

#if 0 /* why, actually? :) */
		/* Add additional staircases */
		if (wild->flags & WILD_F_DOWN) {
			rooms[max_rooms - 4] = STORE_FEAT_MORE;
			rooms[max_rooms - 6] = STORE_FEAT_MORE;
		}
		if (wild->flags & WILD_F_UP) {
			rooms[max_rooms - 3] = STORE_FEAT_LESS;
			rooms[max_rooms - 5] = STORE_FEAT_LESS;
		}
#endif
	}

#ifdef NEW_TOWNGENHACK_METHOD
	/* pick random locations for the base stores */
	n = 6 * 4;
	for (k = 0; k < n; k++) pos[k] = k;
	for (x = 0; x < MAX_BASE_STORES; x++) {
		k = rand_int(n);
		posx[x] = pos[k] / 4 + 3;
		posy[x] = pos[k] % 4 + 1;
		n--;
		pos[k] = pos[n];
	}
#endif

	/* Place base stores */
#ifndef NEW_TOWNGENHACK_METHOD
	n = max_rooms;
	for (y = 2; y < 5; y++) {
		for (x = 4; x < 8; x++) {
			/* hack if we place less base stores than spots are available */
			if (n == max_rooms - base_stores) continue;

			/* Pick a random unplaced store */
			k = rand_int(n - (max_rooms - base_stores));

 			/* Build that store at the proper location */
 #if 0 /* I think I took this out for highlander town, but no need maybe; also required for ironman towns! - C. Blue */
			/* No Black Market in additional towns - C. Blue */
			if (rooms[k] != STORE_BLACK + (dungeon_town ? STORE_GENERAL_DUN : 0)) /* add +70 to get the dungeon version of the store */
				build_store(wpos, rooms[k], y, x);
			//else build_store(wpos, STORE_HERBALIST, y, x);
 #else
			build_store(wpos, rooms[k], y, x);
 #endif

			/* One less store */
			n--;

			/* Shift the stores down, remove one store */
			rooms[k] = rooms[n - (max_rooms - base_stores)];
		}
	}
#else
	for (k = 0; k < MAX_BASE_STORES; k++) {
		/* Build that store at the proper location */
 #if 0 /* I think I took this out for highlander town, but no need maybe; also required for ironman towns! - C. Blue */
		/* No Black Market in additional towns - C. Blue */
		if (rooms[k] != STORE_BLACK + (dungeon_town ? STORE_GENERAL_DUN : 0)) /* add +70 to get the dungeon version of the store */
			build_store(wpos, rooms[k], posy[k], posx[k]);
		//else build_store(wpos, STORE_HERBALIST, y, x);
 #else
		build_store(wpos, rooms[k], posy[k], posx[k]);
 #endif
	}
	n = max_rooms - base_stores;
#endif


/* temporarily disable all the "useless" stores (eg houses)?
   (warning: also includes 'stair stores'?) */
#if 1
	/* Place two rows of stores */
	for (y = 0; y < 6; y++) {
		/* Place four stores per row */
		for (x = 0; x < 12; x++) {
			/* Make sure we haven't already placed this one */
 #ifndef NEW_TOWNGENHACK_METHOD
			if (y >= 2 && y <= 4 && x >= 4 && x <= 7) continue;
 #else
			for (k = 0; k < MAX_BASE_STORES; k++) {
				if (posx[k] == x && posy[k] == y) {
					k = MAX_BASE_STORES + 1; //loop hack
					break;
				}
			}
			if (k == MAX_BASE_STORES + 1) continue;
 #endif

			/* Pick a random unplaced store */
			k = rand_int(n) + base_stores;

			/* don't build "homes" in dungeon towns */
			if (!(rooms[k] == STORE_HOUSE && dungeon_town)) {
 #ifdef DEVEL_TOWN_COMPATIBILITY
				if ( (y != auction_y) || (x != auction_x) ) {
					/* Build that store at the proper location */
					build_store(wpos, rooms[k], y, x);
				} else { /* auction time! */
					build_store(wpos, STORE_AUCTION, y, x);
				}
 #else
				build_store(wpos, rooms[k], y, x);
 #endif
			}

			/* One less building */
			n--;

			/* Shift the stores down, remove one store */
			rooms[k] = rooms[n + base_stores];
		}
	}
#endif

	/* Hack -- use the "complex" RNG */
	Rand_quick = FALSE;

	switchable_shop_grids(zcave);
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

static void town_gen(struct worldpos *wpos) {
	int y, x, type = -1, i;
	int xstart = 0, ystart = 0;	/* dummy, for now */

	cave_type	*c_ptr;
	cave_type	**zcave;
	if (!(zcave = getcave(wpos))) return;

	for (i = 0; i < numtowns; i++) {
		if (town[i].x == wpos->wx && town[i].y == wpos->wy) {
			type = town[i].type;
			break;
		}
	}

	if (type < 0) {
		s_printf("TRIED TO GENERATE NON-ALLOCATED TOWN! %s\n",
		    wpos_format(0, wpos));
		return;
	}

	/* Perma-walls -- North/South*/
	for (x = 0; x < MAX_WID; x++) {
		/* North wall */
		c_ptr = &zcave[0][x];

		/* Clear previous contents, add "clear" perma-wall */
		c_ptr->feat = FEAT_PERM_CLEAR;

		/* Illuminate and memorize the walls 
		c_ptr->info |= (CAVE_GLOW | CAVE_MARK);*/

		/* South wall */
		c_ptr = &zcave[MAX_HGT-1][x];

		/* Clear previous contents, add "clear" perma-wall */
		c_ptr->feat = FEAT_PERM_CLEAR;

		/* Illuminate and memorize the walls 
		c_ptr->info |= (CAVE_GLOW);*/
	}

	/* Perma-walls -- West/East */
	for (y = 0; y < MAX_HGT; y++) {
		/* West wall */
		c_ptr = &zcave[y][0];

		/* Clear previous contents, add "clear" perma-wall */
		c_ptr->feat = FEAT_PERM_CLEAR;

		/* Illuminate and memorize the walls
		c_ptr->info |= (CAVE_GLOW);*/

		/* East wall */
		c_ptr = &zcave[y][MAX_WID-1];

		/* Clear previous contents, add "clear" perma-wall */
		c_ptr->feat = FEAT_PERM_CLEAR;

		/* Illuminate and memorize the walls 
		c_ptr->info |= (CAVE_GLOW);*/
	}
	
	/* XXX this will be changed very soon	- Jir -
	 * It's no good hardcoding things like this.. */
#if 1
	if (type > 0 || type < 6) {
		/* Hack -- Use the "simple" RNG */
		Rand_quick = TRUE;

		/* Hack -- Induce consistant town layout */
		Rand_value = seed_town+(wpos->wx+wpos->wy*MAX_WILD_X);

		/* Hack -- fill with trees
		 * This should be determined by wilderness information */
		for (x = 2; x < MAX_WID - 2; x++) {
			for (y = 2; y < MAX_HGT - 2; y++) {
				/* Access the grid */
				c_ptr = &zcave[y][x];

				/* Clear previous contents, add forest */
				//c_ptr->feat = magik(98) ? FEAT_TREE : FEAT_GRASS;
				i = magik(town_profile[type].ratio) ? town_profile[type].feat1 : town_profile[type].feat2;
				/* hack: swap trees and bushes depending on season on world surface */
				if ((i == FEAT_TREE || i == FEAT_BUSH) && !wpos->wz)
					i = get_seasonal_tree();
				c_ptr->feat = i;
			}
		}

		/* XXX The space is needed to prevent players from getting
		 * stack when entering into a town from wilderness.
		 * TODO: devise a better way */
		/* Perma-walls -- North/South*/
		for (x = 1; x < MAX_WID - 1; x++) {
			/* North wall */
			c_ptr = &zcave[1][x];

			/* Clear previous contents, add "clear" perma-wall */
			c_ptr->feat = FEAT_GRASS;

			/* Illuminate and memorize the walls 
			   c_ptr->info |= (CAVE_GLOW | CAVE_MARK);*/

			/* South wall */
			c_ptr = &zcave[MAX_HGT-2][x];

			/* Clear previous contents, add "clear" perma-wall */
			c_ptr->feat = FEAT_GRASS;

			/* Illuminate and memorize the walls 
			   c_ptr->info |= (CAVE_GLOW);*/
		}

		/* Perma-walls -- West/East */
		for (y = 1; y < MAX_HGT - 1; y++) {
			/* West wall */
			c_ptr = &zcave[y][1];

			/* Clear previous contents, add "clear" perma-wall */
			c_ptr->feat = FEAT_GRASS;

			/* Illuminate and memorize the walls
			   c_ptr->info |= (CAVE_GLOW);*/

			/* East wall */
			c_ptr = &zcave[y][MAX_WID-2];

			/* Clear previous contents, add "clear" perma-wall */
			c_ptr->feat = FEAT_GRASS;

			/* Illuminate and memorize the walls 
			   c_ptr->info |= (CAVE_GLOW);*/
		}
		/* Hack -- use the "complex" RNG */
		Rand_quick = FALSE;

 #if 0
		process_dungeon_file("t_info.txt", wpos, &ystart, &xstart, MAX_HGT, MAX_WID, TRUE);
 #endif	// 0
		switch(type) {
		case TOWN_BREE:
 #ifndef ARCADE_SERVER
			process_dungeon_file("t_bree.txt", wpos, &ystart, &xstart, MAX_HGT, MAX_WID, TRUE);
 #else
			process_dungeon_file("t_bree_arcade.txt", wpos, &ystart, &xstart, MAX_HGT, MAX_WID, TRUE);
 #endif
			break;
		case TOWN_GONDOLIN:
			process_dungeon_file("t_gondol.txt", wpos, &ystart, &xstart, MAX_HGT, MAX_WID, TRUE);
			break;
		case TOWN_MINAS_ANOR:
			process_dungeon_file("t_minas.txt", wpos, &ystart, &xstart, MAX_HGT, MAX_WID, TRUE);
			break;
		case TOWN_LOTHLORIEN:
			process_dungeon_file("t_lorien.txt", wpos, &ystart, &xstart, MAX_HGT, MAX_WID, TRUE);
			break;
		case TOWN_KHAZADDUM:
			process_dungeon_file("t_khazad.txt", wpos, &ystart, &xstart, MAX_HGT, MAX_WID, TRUE);
			break;
		default:
			town_gen_hack(wpos);
			return;
		}
	} else {
		town_gen_hack(wpos);
		return;
	}
#else
	process_dungeon_file("t_info.txt", wpos, &ystart, &xstart,
				MAX_HGT, MAX_WID, TRUE);
#endif

	switchable_shop_grids(zcave);
}





/*
 * Allocate the space needed for a dungeon level
 */
void alloc_dungeon_level(struct worldpos *wpos) {
	int i;
	wilderness_type *w_ptr = &wild_info[wpos->wy][wpos->wx];
	struct dungeon_type *d_ptr;
	cave_type **zcave;

	/* Allocate the array of rows */
	zcave = C_NEW(MAX_HGT, cave_type*);

	/* Allocate each row */
	for (i = 0; i < MAX_HGT; i++) {
		/* Allocate it */
		C_MAKE(zcave[i], MAX_WID, cave_type);
	}

	if (wpos->wz) {
		struct dun_level *dlp;
		d_ptr = (wpos->wz > 0 ? w_ptr->tower : w_ptr->dungeon);
		dlp = &d_ptr->level[ABS(wpos->wz) - 1];
		dlp->cave = zcave;
		dlp->creationtime = time(NULL);
	} else {
		w_ptr->cave = zcave;

		/* init ambient sfx */
		w_ptr->ambient_sfx_dummy = FALSE;
		switch (w_ptr->type) { /* ---- ensure consistency with process_ambient_sfx() ---- */
		case WILD_RIVER:
		case WILD_LAKE:
		case WILD_SWAMP:
			w_ptr->ambient_sfx_timer = 4 + rand_int(4);
			break;
		case WILD_ICE:
		case WILD_MOUNTAIN:
		case WILD_WASTELAND:
			w_ptr->ambient_sfx_timer = 30 + rand_int(60);
			break;
		//case WILD_SHORE:
		case WILD_OCEAN:
			w_ptr->ambient_sfx_timer = 30 + rand_int(60);
			break;
		case WILD_FOREST:
		case WILD_DENSEFOREST:
			if (IS_DAY) w_ptr->ambient_sfx_timer = 10 + rand_int(20);
			else w_ptr->ambient_sfx_timer = 20 + rand_int(40);
			break;
		default: //compromise for wilderness-travel ambient sfx hacking
			w_ptr->ambient_sfx_timer = 30 + rand_int(5); //should be > than time required for travelling across 1 wilderness sector
			w_ptr->ambient_sfx_dummy = TRUE;
			break;
		}
	}
}

/*
 * Deallocate the space needed for a dungeon level
 */
void dealloc_dungeon_level(struct worldpos *wpos) {
	int i, j;
	wilderness_type *w_ptr = &wild_info[wpos->wy][wpos->wx];
	cave_type **zcave;
	object_type *o_ptr;
	char o_name[ONAME_LEN];
	cave_type *c_ptr;
	dun_level *l_ptr = getfloor(wpos);

	/* master-level-unstaticing as done in slash command /treset
	   will already call dealloc_dungeon_level, so -> NULL pointer. */
	if (!(zcave = getcave(wpos))) return;

	if (l_ptr && (l_ptr->flags2 & LF2_COLLAPSING)) nether_realm_collapsing = FALSE;

	/* for obtaining statistical IDDC information: */
	//if (in_irondeepdive(wpos)) log_floor_coverage(l_ptr, wpos);

#if DEBUG_LEVEL > 1
	s_printf("deallocating %s\n", wpos_format(0, wpos));
#endif

	/* Untie links between dungeon stores and real towns */
	if (l_ptr && l_ptr->fake_town_num) {
		town[l_ptr->fake_town_num - 1].dlev_id = 0;
		l_ptr->fake_town_num = 0;
	}

	/* Delete any monsters/objects on that level */
	/* Hack -- don't wipe wilderness monsters/objects
	   (especially important for preserving items in town inns). */
	if (wpos->wz
#ifdef IRONDEEPDIVE_FIXED_TOWNS
 #ifdef IRONDEEPDIVE_STATIC_TOWNS
	    && !is_fixed_irondeepdive_town(wpos, getlevel(wpos))
 #endif
#endif
	     ) {
		wipe_m_list_special(wpos);
		wipe_o_list_special(wpos);
	} else {
#ifdef IRONDEEPDIVE_FIXED_TOWNS
 #ifdef IRONDEEPDIVE_STATIC_TOWNS
		if (!wpos->wz)
 #endif
#endif
		save_guildhalls(wpos);	/* has to be done here */

		/* remove 'deposited' true artefacts from wilderness */
		if (cfg.anti_arts_wild) {
			for(i = 0; i < o_max; i++){
				o_ptr = &o_list[i];
				if (o_ptr->k_idx && inarea(&o_ptr->wpos, wpos) &&
				    undepositable_artifact_p(o_ptr)) {
					object_desc(0, o_name, o_ptr, FALSE, 0);
					s_printf("WILD_ART_DEALLOC: %s of %s erased at (%d, %d, %d)\n",
					    o_name, lookup_player_name(o_ptr->owner), o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz);
					handle_art_d(o_ptr->name1);
					WIPE(o_ptr, object_type);
				}
			}
		}
	}

	for (i = 0; i < MAX_HGT; i++) {
		/* Erase all special feature stuff - mikaelh */
		for (j = 0; j < MAX_WID; j++) {
			c_ptr = &zcave[i][j];
			FreeCS(c_ptr);
		}

		/* Dealloc that row */
		C_KILL(zcave[i], MAX_WID, cave_type);
	}
	/* Deallocate the array of rows */
	C_FREE(zcave, MAX_HGT, cave_type *);
	if (wpos->wz) {
		struct dun_level *dlp;
		struct dungeon_type *d_ptr;
		d_ptr = (wpos->wz > 0 ? w_ptr->tower : w_ptr->dungeon);
		dlp = &d_ptr->level[ABS(wpos->wz) - 1];
		dlp->cave = NULL;
	}
	else w_ptr->cave = NULL;
}

/* added 'force' to delete it even if someone is still inside,
   that player gets recalled to surface in that case. - C. Blue */
static void del_dungeon(struct worldpos *wpos, bool tower, bool force) {
	struct dungeon_type *d_ptr;
	int i, j;
	struct worldpos twpos;
	wilderness_type *wild = &wild_info[wpos->wy][wpos->wx];
	player_type *p_ptr;

	s_printf("%s at (%d,%d) attempt remove\n", (tower ? "Tower" : "Dungeon"), wpos->wx, wpos->wy);

	wpcopy(&twpos, wpos);
	d_ptr = (tower ? wild->tower : wild->dungeon);
	if (d_ptr->flags2 & DF2_DELETED) {
#ifdef DUNGEON_VISIT_BONUS
 #if 0 /* bs ^^' */
		for (i = 1; i <= dungeon_id_max; i++) {
			if (dungeon_x[i] == wpos->wx &&
			    dungeon_y[i] == wpos->wy &&
			    dungeon_tower[i] == tower) {
				struct dungeon_type *d2_ptr, *d3_ptr;
				s_printf("removing dungeon of index %d, new amount of dungeons is %d.\n", i, dungeon_id_max - 1);
				for (; i < dungeon_id_max; i++) {
					d2_ptr = (dungeon_tower[i] ?
					    wild_info[dungeon_y[i]][dungeon_x[i]].tower :
					    wild_info[dungeon_y[i]][dungeon_x[i]].dungeon);
					d3_ptr = (dungeon_tower[i + 1] ?
					    wild_info[dungeon_y[i + 1]][dungeon_x[i + 1]].tower :
					    wild_info[dungeon_y[i + 1]][dungeon_x[i + 1]].dungeon);
					d2_ptr->id = d3_ptr->id;

					dungeon_tower[i] = dungeon_tower[i + 1];
					dungeon_x[i] = dungeon_x[i + 1];
					dungeon_y[i] = dungeon_y[i + 1];
					dungeon_visit_frequency[i] = dungeon_visit_frequency[i + 1];
					dungeon_bonus[i] = dungeon_bonus[i + 1];
				}
				dungeon_id_max--;
				break;
			}
		}
 #else
		struct dungeon_type *d2_ptr;
		s_printf("removing dungeon of index %d, new amount of dungeons is %d.\n", d_ptr->id, dungeon_id_max - 1);
		for (i = d_ptr->id + 1; i <= dungeon_id_max; i++) {
			d2_ptr = (dungeon_tower[i] ?
			    wild_info[dungeon_y[i]][dungeon_x[i]].tower :
			    wild_info[dungeon_y[i]][dungeon_x[i]].dungeon);
			d2_ptr->id--;

			dungeon_tower[i - 1] = dungeon_tower[i];
			dungeon_x[i - 1] = dungeon_x[i];
			dungeon_y[i - 1] = dungeon_y[i];
			dungeon_visit_frequency[i - 1] = dungeon_visit_frequency[i];
			dungeon_bonus[i - 1] = dungeon_bonus[i];
		}
		dungeon_id_max--;
 #endif
#endif
		s_printf("Deleted flag\n");
		/* deallocate all floors */
		for (i = 0; i < d_ptr->maxdepth; i++) {
			twpos.wz = (tower ? i + 1 : 0 - (i + 1));
			if (d_ptr->level[i].ondepth) { /* someone is still inside! */
				if (force) s_printf("Dungeon deletion: ondepth on floor %d - forcing out.\n", i);
				else s_printf("Dungeon deletion: ondepth on floor %d - cancelling.\n", i);
				if (!force) return;
				for (j = 1; j <= NumPlayers; j++) {
					p_ptr = Players[j];
					if (inarea(&p_ptr->wpos, &twpos)) {
						p_ptr->recall_pos.wx = wpos->wx;
						p_ptr->recall_pos.wy = wpos->wy;
						p_ptr->recall_pos.wz = 0;
						if (tower) recall_player(j, "Suddenly the tower crumbles and you are back to the outside world!");
						else recall_player(j, "Suddenly the dungeon crumbles and you are back to the outside world!");
					}
				}
				d_ptr->level[i].ondepth = 0;//obsolete?
			}
			if (d_ptr->level[i].cave) dealloc_dungeon_level(&twpos);
			C_KILL(d_ptr->level[i].uniques_killed, MAX_R_IDX, char);
		}
		/* free the floors all at once */
		C_KILL(d_ptr->level, d_ptr->maxdepth, struct dun_level);
		/* free the dungeon struct */
		if (tower)
			KILL(wild->tower, struct dungeon_type);
		else
			KILL(wild->dungeon, struct dungeon_type);
	} else {
		s_printf("%s at (%d,%d) restored\n", (tower ? "Tower" : "Dungeon"), wpos->wx, wpos->wy);
		/* This really should not happen, but just in case */
		/* Re allow access to the non deleted dungeon */
		wild->flags |= (tower ? WILD_F_UP : WILD_F_DOWN);
	}
	s_printf("%s at (%d,%d) removed\n", (tower ? "Tower" : "Dungeon"), wpos->wx, wpos->wy);
#if 0
	/* Release the lock */
	wild->flags &= ~(tower ? WILD_F_LOCKUP : WILD_F_LOCKDOWN);
#endif
}

void rem_dungeon(struct worldpos *wpos, bool tower) {
	struct dungeon_type *d_ptr;
	wilderness_type *wild = &wild_info[wpos->wy][wpos->wx];

	d_ptr = (tower ? wild->tower : wild->dungeon);
	if (!d_ptr) return;
#if 0
	/* Lock so that dungeon cannot be overwritten while in use */
	wild->flags |= (tower ? WILD_F_LOCKUP : WILD_F_LOCKDOWN);
#endif
	/* This will prevent players entering the dungeon */
	wild->flags &= ~(tower ? WILD_F_UP : WILD_F_DOWN);

	d_ptr->flags2 |= DF2_DELETED;
	del_dungeon(wpos, tower, TRUE);	/* Hopefully first time */
}

/* 'type' refers to t_info[] */
void add_dungeon(struct worldpos *wpos, int baselevel, int maxdep, u32b flags1, u32b flags2, u32b flags3, bool tower, int type, int theme, int quest, int quest_stage) {
#ifdef RPG_SERVER
	bool found_town = FALSE;
#endif
	int i;
	wilderness_type *wild;
	struct dungeon_type *d_ptr;
	wild = &wild_info[wpos->wy][wpos->wx];

	/* Hack -- override the specification */
	if (type) tower = (d_info[type].flags1 & DF1_TOWER) ? TRUE : FALSE;
#if 0
	if (wild->flags & (tower ? WILD_F_LOCKUP : WILD_F_LOCKDOWN)) {
		s_printf("add_dungeon failed due to WILD_F_LOCK%s.\n", tower ? "UP" : "DOWN");
		return;
	}
#endif
#ifdef DUNGEON_VISIT_BONUS
	/* new paranoid debug code: cancel adding of dungeon if wild_info _seems_ to already have a dungeon in place. */
	if (wild->flags & (tower ? WILD_F_UP : WILD_F_DOWN)) {
		s_printf("DEBUG: add_dungeon failed (%d,%d,%s) due to already existing wild_f_ flag.\n",
		    wpos->wx, wpos->wy, tower ? "tower" : "dungeon");
		return;
	}
	if (tower ? wild->tower : wild->dungeon) {
		s_printf("DEBUG: add_dungeon failed (%d,%d,%s) due to already existing wild-> structure.\n",
		    wpos->wx, wpos->wy, tower ? "tower" : "dungeon");
		return;
	}
#endif

	wild->flags |= (tower ? WILD_F_UP : WILD_F_DOWN); /* no checking */
	if (tower)
		MAKE(wild->tower, struct dungeon_type);
	else
		MAKE(wild->dungeon, struct dungeon_type);
	d_ptr = (tower ? wild->tower : wild->dungeon);

	d_ptr->type = type;
	d_ptr->theme = theme;
	d_ptr->quest = quest;
	d_ptr->quest_stage = quest_stage;

#ifdef DUNGEON_VISIT_BONUS
	d_ptr->id = ++dungeon_id_max;
	dungeon_x[dungeon_id_max] = wpos->wx;
	dungeon_y[dungeon_id_max] = wpos->wy;
	dungeon_tower[dungeon_id_max] = tower;
	/* mostly affects highlander dungeon: */
	set_dungeon_bonus(dungeon_id_max, TRUE);
	s_printf("adding dungeon of index %d.\n", dungeon_id_max);
#endif

	if (type) {
		/* XXX: flags1, flags2 can be affected if specified so? */
		d_ptr->baselevel = d_info[type].mindepth;
		d_ptr->maxdepth = d_info[type].maxdepth - d_ptr->baselevel + 1; 
		d_ptr->flags1 = d_info[type].flags1 | flags1;
		d_ptr->flags2 = d_info[type].flags2 | flags2 | DF2_RANDOM;
		d_ptr->flags3 = d_info[type].flags3 | flags3;
	} else {
		d_ptr->baselevel = baselevel;
		d_ptr->flags1 = flags1;
		d_ptr->flags2 = flags2;
		d_ptr->flags3 = flags3;
		d_ptr->maxdepth = maxdep;
	}
	if (wpos->wx == WPOS_IRONDEEPDIVE_X && wpos->wy == WPOS_IRONDEEPDIVE_Y &&
	    (tower ? (WPOS_IRONDEEPDIVE_Z > 0) : (WPOS_IRONDEEPDIVE_Z < 0)))
		d_ptr->flags3 |= DF3_NO_DUNGEON_BONUS | DF3_EXP_20 | DF3_LUCK_PROG_IDDC;

#ifdef RPG_SERVER /* Make towers/dungeons harder - C. Blue */
	/* If this dungeon was originally intended to be 'real' ironman,
	   don't make it EASIER by this! (by applying either IRONFIX or IRONRND flags) */
	if (!(flags2 & DF2_IRON)) {
		for (i = 0; i < numtowns; i++)
			if (town[i].x == wpos->wx && town[i].y == wpos->wy) {
				found_town = TRUE;
				/* Bree has special rules: */
				if (in_bree(wpos)) {
					/* exempt training tower, since it's empty anyway
					   (ie monster/item spawn is prevented) and we
					   need it for "arena monster challenge" event */
					if (tower) continue;

					d_ptr->flags2 |= DF2_IRON;
				} else {
					/* any other town.. */
					d_ptr->flags2 |= DF2_IRON | DF2_IRONFIX2;
				}
			}
		if (!found_town) {
			/* hack - exempt the Shores of Valinor! */
			if (d_ptr->baselevel != 200) {
				d_ptr->flags2 |= DF2_IRON | DF2_IRONRND1;
//				d_ptr->flags1 |= DF1_NO_UP;
			}
		}
	}
#endif

#if 0	/* unused - mikaelh */
	for (i = 0; i < 10; i++) {
		d_ptr->r_char[i] = '\0';
		d_ptr->nr_char[i] = '\0';
	}
	if (race != (char*)NULL)
		strcpy(d_ptr->r_char, race);
	if (exclude != (char*)NULL)
		strcpy(d_ptr->nr_char, exclude);
#endif

	C_MAKE(d_ptr->level, d_ptr->maxdepth, struct dun_level);
	for (i = 0; i < d_ptr->maxdepth; i++) {
		C_MAKE(d_ptr->level[i].uniques_killed, MAX_R_IDX, char);
	}

	s_printf("add_dungeon completed (type %d, %s).\n", type, tower ? "tower" : "dungeon");
}

/*
 * Generates a random dungeon level			-RAK-
 *
 * Hack -- regenerate any "overflow" levels
 *
 * Hack -- allow auto-scumming via a gameplay option.
 */

void generate_cave(struct worldpos *wpos, player_type *p_ptr) {
	int i, num;
	cave_type **zcave;
	struct worldpos twpos;
	struct timeval time_begin, time_end, time_delta;

	/* For measuring performance */
	gettimeofday(&time_begin, NULL);

	wpcopy(&twpos, wpos);
	zcave = getcave(wpos);

	server_dungeon = FALSE;

	/* Generate */
	for (num = 0; TRUE; num++) {
		bool okay = TRUE;
		cptr why = NULL;

		/* Added 'restore from backup' here, maybe it helps vs crash */
		wpcopy(wpos, &twpos);
		zcave = getcave(wpos);

		/* Hack -- Reset heaps */
		/*o_max = 1;
		m_max = 1;*/
#if 1 /*cave should have already been wiped from 'alloc_dungon_level' */
		/* Start with a blank cave */
		for (i = 0; i < MAX_HGT; i++) {
			/* CRASHES occured here :( */

			/* Wipe a whole row at a time */
			C_WIPE(zcave[i], MAX_WID, cave_type);
		}
#endif

		/* Mega-Hack -- no player yet */
		/*px = py = 0;*/


		/* Mega-Hack -- no panel yet */
		/*panel_row_min = 0;
		panel_row_max = 0;
		panel_col_min = 0;
		panel_col_max = 0;*/

		/* Reset the monster generation level */
		monster_level = getlevel(wpos);

		/* Reset the object generation level */
		object_level = getlevel(wpos);

		/* Build the town */
		if (istown(wpos)) {
			/* Small town */
			/*cur_hgt = SCREEN_HGT;
			cur_wid = SCREEN_WID;*/

			/* Determine number of panels */
			/*max_panel_rows = (cur_hgt / SCREEN_HGT) * 2 - 2;
			max_panel_cols = (cur_wid / SCREEN_WID) * 2 - 2;*/

			/* Assume illegal panel */
			/*panel_row = max_panel_rows;
			panel_col = max_panel_cols;*/

			{
				int retval = -1, type;
				for(i = 0; i < numtowns; i++) {
					if(town[i].x == wpos->wx && town[i].y == wpos->wy) {
						retval = i;
						break;
					}
				}
				if (retval < 0) {
					s_printf("add_dungeon failed in generate_cave!! %s\n", wpos_format(0, wpos));
					return;	/* This should never.. */
				}
				type = town[retval].type;
				for (i = 0; i < 2; i++) {
					if (town_profile[type].dungeons[i])
						add_dungeon(wpos, 0, 0, 0x0, 0x0, 0x0, FALSE, town_profile[type].dungeons[i], 0, 0, 0);
				}
			}
			/* Make a town */
			town_gen(wpos);
			setup_objects();
			setup_monsters();
		}

		/* Build wilderness */
		else if (!wpos->wz) {
			wilderness_gen(wpos);
		}

		/* Build a real level */
		else {
			process_hooks(HOOK_GEN_LEVEL, "d", wpos);
			/* Big dungeon */
			/*cur_hgt = MAX_HGT;
			cur_wid = MAX_WID;*/

			/* Determine number of panels */
			/*max_panel_rows = (cur_hgt / SCREEN_HGT) * 2 - 2;
			max_panel_cols = (cur_wid / SCREEN_WID) * 2 - 2;*/

			/* Assume illegal panel */
			/*panel_row = max_panel_rows;
			panel_col = max_panel_cols;*/

			/* Set restrictions on placed objects */
			place_object_restrictor = RESF_NOHIDSM; /* no high DSMs that could be wiz-lite-looted */
			/* Make a dungeon */
			cave_gen(wpos, p_ptr);
			/* Lift restrictions on placed objects again */
			place_object_restrictor = RESF_NONE;
		}

		/* Prevent object over-flow */
		if (o_max >= MAX_O_IDX) {
			/* Message */
			why = "too many objects";

			/* Message */
			okay = FALSE;
		}

		/* Prevent monster over-flow */
		if (m_max >= MAX_M_IDX) {
			/* Message */
			why = "too many monsters";

			/* Message */
			okay = FALSE;
		}

		/* Accept */
		if (okay) break;


		/* Message */
		if (why) s_printf("Generation restarted (%s)\n", why);

		/* Wipe the objects */
		if (wpos->wz) wipe_o_list_special(wpos);
		else wipe_o_list(wpos);//no reason not to protect house items here, is there?

		/* Wipe the monsters */
		wipe_m_list(wpos);

		/* Compact some objects, if necessary */
		if (o_max >= MAX_O_IDX * 3 / 4)
			compact_objects(32, FALSE);

		/* Compact some monsters, if necessary */
		if (m_max >= MAX_M_IDX * 3 / 4)
			compact_monsters(32, FALSE);
	}

	/* Change features depending on season,
	   and change lighting depending on daytime */
	wpos_apply_season_daytime(wpos, zcave);

	/* Dungeon level ready */
	server_dungeon = TRUE;

	/* Calculate the amount of time spent */
	gettimeofday(&time_end, NULL);
	time_delta.tv_sec = time_end.tv_sec - time_begin.tv_sec;
	time_delta.tv_usec = time_end.tv_usec - time_begin.tv_usec;
	if (time_delta.tv_usec < 0) {
		time_delta.tv_sec--;
		time_delta.tv_usec += 1000000;
	}
	// Disabled to prevent log spam on live server
	//s_printf("%s: Level generation took %d.%06d seconds.\n", __func__, (int)time_delta.tv_sec, (int)time_delta.tv_usec);
}

/* (Can ONLY be used on surface worldmap sectors.)
   Rebuilds a level, without re-adding dungeons, because this
   function assumes that the level has already been generated.
   Used for season change. - C. Blue */
void regenerate_cave(struct worldpos *wpos) {
	cave_type **zcave;

	int i;
	player_type *p_ptr;

	/* world surface only, to keep it simple */
	if (wpos->wz) return;
	/* paranoia */
	if (!(zcave = getcave(wpos))) return;

#if 0 /* kills all house walls and doors on allocated town area levels! */
	/* Build the town */
	if (istown(wpos)) {
		/* Make a town */
		town_gen(wpos);
	}
	/* Build wilderness */
	else if (!wpos->wz) {
		wilderness_gen(wpos);
	}
#else /* what a pain */
	/* mad hack:
	   remove players from level, regenerate it completely, bring players back */
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];
		if (inarea(&p_ptr->wpos, wpos)) p_ptr->wpos.wz = 9999;
	}

	dealloc_dungeon_level(wpos);
	alloc_dungeon_level(wpos);
	generate_cave(wpos, NULL);

	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];
		if (p_ptr->wpos.wz == 9999) p_ptr->wpos.wz = 0;
	}

	/* regrab zcave, _rarely_ crash otherwise:
	   happened in actual cases when the 2nd season switch occurred after
	   server startup in the fashion summer->autumn->winter. Haven't tested
	   it much more. */
	if (!(zcave = getcave(wpos))) {
		s_printf("REGENERATE_CAVE FAILED TO REGRAB ZCAVE!\n");
		return;
	}
#endif

	/* Change features depending on season,
	   and change lighting depending on daytime */
	wpos_apply_season_daytime(wpos, zcave);
}

/* determine whether a feature should be a tree or a
   bush, depending on the season - C. Blue */
int get_seasonal_tree(void) {
	switch (season) {
	case SEASON_SPRING: return (magik(70) ? FEAT_TREE : FEAT_BUSH); break;
	case SEASON_SUMMER: return (magik(90) ? FEAT_TREE : FEAT_BUSH); break;
	case SEASON_AUTUMN: return (magik(90) ? FEAT_TREE : FEAT_BUSH); break;
	case SEASON_WINTER: return (magik(95) ? FEAT_TREE : FEAT_BUSH); break;
	}
	return (FEAT_HIGH_MOUNTAIN); /* just to clear compiler warning */
}

#ifdef ENABLE_DOOR_CHECK
/* Check that a door that is about to be placed makes actually at least slight sense.
   Mostly disallow doors that have no 'wall with hinges' next to them,
   ie doors hanging in the air freely.
   Current disadvantage: Enabling this will prevent 'quad-doors' which may look
   cool as *big* entrance to a room.  - C. Blue */
#define DOOR_FLOOR(feat) \
    (((feat < FEAT_DOOR_HEAD || feat > FEAT_DOOR_TAIL) \
    && feat != FEAT_SECRET) && \
    (f_info[feat].flags1 & FF1_FLOOR))
#define DOOR_WALL(feat) \
    (((feat < FEAT_DOOR_HEAD || feat > FEAT_DOOR_TAIL) \
    && feat != FEAT_SECRET) && \
    !(f_info[feat].flags1 & FF1_FLOOR))
//#define DOOR_SEPARATING
static bool door_makes_no_sense(cave_type **zcave, int x, int y) {
	int tmp, feat;
	bool walls = FALSE; //disallow duplicate door that 'hangs in the air'
#ifdef DOOR_SEPARATING
	int ok_step = 0;
	int adjacent_floors = 0; //allow an exception via 'collapsed passage' excuse ;)
#endif


	/* Check that this door makes any sense */
	feat = zcave[y + ddy_cyc[0]][x + ddx_cyc[0]].feat;

#ifdef DOOR_SEPARATING
	/* check 'sustaining-wall' condition */
	if (DOOR_WALL(feat)) walls = TRUE;

	/* check the other stuff */
	if (DOOR_FLOOR(feat)) {
		ok_step = 1;
		adjacent_floors = 1;
	}
#else
	/* check 'sustaining-wall' condition */
	if (DOOR_WALL(feat)) return FALSE;
#endif


	for (tmp = 1; tmp < 8; tmp++) {
		feat = zcave[y + ddy_cyc[tmp]][x + ddx_cyc[tmp]].feat;

#ifdef DOOR_SEPARATING /* we can assume it has withered or been destroyed, so the door is useless now but still there! */
		/* check 'sustaining-wall' condition */
		if (!walls && DOOR_WALL(feat)) walls = TRUE;

		/* optional: check 'collapsed passage' exception */
		if (DOOR_FLOOR(feat)) adjacent_floors++;

		/* check that the door actually separates any passages */
		switch (ok_step) {
		case 0:
			if (DOOR_FLOOR(feat)) ok_step = 2;
			break;
		case 2:
			if (!DOOR_FLOOR(feat)) ok_step = 4;
			break;
		case 4:
			if (DOOR_FLOOR(feat)) return FALSE;
			break;
		case 1:
			if (!DOOR_FLOOR(feat)) ok_step = 3;
			break;
		case 3:
			if (DOOR_FLOOR(feat)) ok_step = 5;
			break;
		case 5:
			if (!DOOR_FLOOR(feat)) return FALSE;
			break;
		}
#else
		/* check 'sustaining-wall' condition */
		if (!walls && DOOR_WALL(feat)) return FALSE;
#endif
	}

#ifdef DOOR_SEPARATING
	/* condition of 'sustaining wall'? */
	if (!walls) return TRUE;


	/* exception for 'collapsed passages'? */
	if (adjacent_floors <= 3) return FALSE;
#endif

#if 0 /* debug */
zcave[y][x].feat = FEAT_SEALED_DOOR;
s_printf("TRUE\n");
#endif

	return TRUE;
}
#endif
