/* $Id$ */
/* File: wilderness.c */

/* Purpose: Wilderness generation */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * Copyright (c) 1999 Alex P. Dingle
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

/*
 * Heavily modified and improved in 2001-2002 by Evileye
 */

#define SERVER

#include "angband.h"


#ifdef WILDERNESS_NEW_FEATURES
 /* Bleed with neighbours? (The main bleeding function).
    WARNING: Toggling this feature can change house locations. */
 #define BLEED_WITH_NEIGHBOURS
#else
 /* Use a simple kind of bleeding just to indicate dangerous terrain ahead,
    was just used ad interim. */
 #define SIMPLE_BLEED
#endif

/* For BLEED_WITH_NEIGHBOURS:
   Don't bleed involving/from towns because that can look a bit weird
   sometimes, eg if Khazad-dum bleeds grassland into adjacent volcanos.
   WARNING: Toggling this feature can change house locations.
   ** Sort of deprecated - use BLEED_ENHANCED_TOWN below instead. - C. Blue */
//#define BLEED_AVOID_TOWN

/* For BLEED_WITH_NEIGHBOURS:
   Bleed correct town terrain into town-adjacent sectors (hard-coded though).
   WARNING: Toggling this feature can change house locations. */
#define BLEED_ENHANCED_TOWN

/* HACK: Disable new terrain types DESERT/WILD and other rng-affecting stuff? */
#ifndef WILDERNESS_NEW_FEATURES
 #define __DISABLE_NEW
#endif

/* HACK: Disable house shortage counter measures? */
#ifndef WILDERNESS_NEW_FEATURES
 #define __DISABLE_HOUSEBOOST
#endif


/* This function takes the players x,y level world coordinate and uses it to
 calculate p_ptr->dun_depth.  The levels are stored in a series of "rings"
 radiating out from the town, as shown below.  This storage mechanisim was
 used because it does not place an initial restriction on the wilderness
 dimensions.

In order to gracefully introduce the wilderness to the preexisting mangband
functions, the indexes are negative and apply to the same cave structure
that holds the dungeon levels.

		Indexes (-)		  Ring #			world_y

                 [05]                       [2]			         [ 2]
 	     [12][01][06]                [2][1][2]  		         [ 1]
 	 [11][04][To][02][07]         [2][1][X][1][2]   world_x  [-2][-1][ 0][ 1][ 2]
 	     [10][03][08]                [2][1][2]                       [-1]
 	 	[09]                        [2]         		 [-2]
 -APD-

 A special function is required to init the  wild_info structures because I have not
 been able to devise a simple algorithm to go from an index to an x,y coordinate.
 I derived an equation that would calculate the ring from the index, but it involved
 a square root.

 */

int world_index(int world_x, int world_y)
{
	int ring, base, offset, idx;

	/* calculate which "ring" the level is in */
	ring = abs(world_x) + abs(world_y);

	/* hack -- the town is 0 */
	if (!ring) {
		return 0;
	}

	/* calculate the base offset of this ring */
	base = 2*ring*(ring-1) + 1;

	/* calculate the offset within this ring */
	if (world_x >= 0) offset = ring - world_y;
	else offset = (3 * ring) + world_y;

	idx = -(base + offset);

	return idx;
}

#ifdef SIMPLE_BLEED
static void bleed_warn_feat(int wild_type, cave_type *c_ptr) {
	switch (wild_type) {
	case WILD_SWAMP: c_ptr->feat = FEAT_BUSH; break;
//	case WILD_SHORE1: case WILD_SHORE2: case WILD_COAST:
	case WILD_LAKE: case WILD_RIVER:
	case WILD_OCEANBED1: case WILD_OCEANBED2:
	case WILD_OCEAN: c_ptr->feat = FEAT_SHAL_WATER; break;
	case WILD_MOUNTAIN: c_ptr->feat = FEAT_MOUNTAIN; break;
	case WILD_VOLCANO: c_ptr->feat = FEAT_SHAL_LAVA; break;
	}
}
#endif

/* Initialize the wild_info coordinates and radius. Uses a recursive fill algorithm.
   This may seem a bit out of place, but I think it is too complex to go in init2.c.
   Note that the flags for these structures are loaded from the server savefile.

   Note that this has to be initially called with 0,0 to work properly.
*/

static int towndist(int wx, int wy, s16b *townlev) {
	int i;
	int dist, mindist = 100;

	*townlev = 0;
	for (i = 0; i < numtowns; i++){
		dist = abs(wx - town[i].x) + abs(wy - town[i].y);
		if (dist < mindist) {
			mindist = dist;
			*townlev = town[i].baselevel;
		}
	}
	return (mindist);
}

void init_wild_info_aux(int x, int y) {
	s16b tlev;

	wild_info[y][x].radius = towndist(x, y, &tlev);
	wild_info[y][x].town_lev = tlev;
	wild_info[y][x].town_idx = wild_gettown(x, y);

	if (y + 1 < MAX_WILD_Y) {
		if (!(wild_info[y + 1][x].radius))
			init_wild_info_aux(x, y + 1);
	}
	if (x + 1 < MAX_WILD_X) {
		if (!(wild_info[y][x + 1].radius))
			init_wild_info_aux(x + 1, y);
	}
	if (y - 1 >= 0) {
		if (!(wild_info[y - 1][x].radius))
			init_wild_info_aux(x, y - 1);
	}
	if (x - 1 >= 0) {
		if (!(wild_info[y][x - 1].radius))
			init_wild_info_aux(x - 1, y);
	}

}

void initwild() {
	int i, j;

	for (i = 0; i < MAX_WILD_X; i++)
		for (j = 0; j < MAX_WILD_Y; j++)
			wild_info[j][i].type = WILD_UNDEFINED;
}

/* Initialize the wild_info coordinates and radius. Uses a recursive fill algorithm.
   This may seem a bit out of place, but I think it is too complex to go in init2.c.
   Note that the flags for these structures are loaded from the server savefile.

   Note that this has to be initially called with 0,0 to work properly.
*/

void addtown(int y, int x, int base, u16b flags, int type) {
	int n;

	if (numtowns)
		GROW(town, numtowns, numtowns+1, struct town_type);
	else
		MAKE(town, struct town_type);
	town[numtowns].x = x;
	town[numtowns].y = y;
	town[numtowns].baselevel = base;
	town[numtowns].flags = flags;
//	town[numtowns].num_stores = MAX_BASE_STORES;
	town[numtowns].num_stores = max_st_idx;
	town[numtowns].type = type;
	wild_info[y][x].type = WILD_TOWN;
	wild_info[y][x].town_idx = numtowns;
	wild_info[y][x].radius = base;
	alloc_stores(numtowns);
	/* Initialize the stores */
//	for (n = 0; n < MAX_BASE_STORES; n++)
	for (n = 0; n < max_st_idx; n++) {
		/* make shop remember the town its in - C. Blue */
		town[numtowns].townstore[n].town = numtowns;

		//int i;
		/* Initialize */
		store_init(&town[numtowns].townstore[n]);

		/* Ignore home and auction house */
//		if ((n == MAX_BASE_STORES - 2) || (n == MAX_BASE_STORES - 1)) continue;

		/* Maintain the shop */
		store_maint(&town[numtowns].townstore[n]);
//		for (i = 0; i < 10; i++) store_maint(&town[numtowns].townstore[n]);
	}
	numtowns++;
}

/* Erase a custom town again. Why wasn't this implemented already!? - C. Blue */
void deltown(int Ind) {
	int x, y, i;
	s16b tlev;
	struct worldpos *wpos = &Players[Ind]->wpos, tpos;

#if 1
	for (x = wpos->wx - wild_info[wpos->wy][wpos->wx].radius;
	    x <= wpos->wx + wild_info[wpos->wy][wpos->wx].radius; x++)
		for (y = wpos->wy - wild_info[wpos->wy][wpos->wx].radius;
		    y <= wpos->wy + wild_info[wpos->wy][wpos->wx].radius; y++)
			if (in_bounds_wild(y, x) && (towndist(x, y, &tlev) <= abs(wpos->wx - x) + abs(wpos->wy - y))) {
				tpos.wx = x; tpos.wy = y; tpos.wz = 0;
				for (i = 0; i < num_houses; i++)
					if (inarea(&tpos, &houses[i].wpos)) {
//#if 0
						fill_house(&houses[i], FILL_MAKEHOUSE, NULL);
						houses[i].flags |= HF_DELETED;
//#endif
					}
				wilderness_gen(&tpos);
			}
#endif

	if (numtowns <= 5) return;

//	wild_info[wpos->wy][wpos->wx].type = WILD_GRASSLAND;
//	wild_info[wpos->wy][wpos->wx].type = WILD_OCEAN;
	wild_info[wpos->wy][wpos->wx].type = WILD_UNDEFINED; /* re-generate */
	wild_info[wpos->wy][wpos->wx].radius = towndist(wpos->wy, wpos->wx, &tlev);
	wild_info[wpos->wy][wpos->wx].town_lev = tlev;
	wild_info[wpos->wy][wpos->wx].town_idx = wild_gettown(wpos->wx, wpos->wy);
	wilderness_gen(wpos);

	/* Shrink the town array */
	SHRINK(town, numtowns, numtowns - 1, struct town_type);

	numtowns--;
}

void wild_bulldoze()
{
	int x,y;

	/* inefficient? thats an understatement */
	for(y = 0; y < MAX_WILD_Y; y++){
		for(x = 0; x < MAX_WILD_X; x++){
			struct wilderness_type *w_ptr = &wild_info[y][x];
			if (w_ptr->radius <= MAX_TOWNAREA &&
			    (w_ptr->type == WILD_WASTELAND ||
			    w_ptr->type == WILD_DESERT ||
			    w_ptr->type == WILD_ICE ||
			    w_ptr->type == WILD_DENSEFOREST ||
			    w_ptr->type == WILD_OCEAN ||
			    w_ptr->type == WILD_RIVER ||
			    w_ptr->type == WILD_VOLCANO ||
			    w_ptr->type == WILD_MOUNTAIN)) {
				wild_info[y][x].type = WILD_GRASSLAND;
			}
		}
	}
}

/*
 * Makeshift towns/dungeons placer for v4
 * This should be totally rewritten for v5!		- Jir -
 */
void wild_spawn_towns()
{
	int x, y, i, j, k;
	bool retry, skip;

	int tries = 100;
	worldpos wpos = {0, 0, 0};

	/* Place towns */
	for (i = 1 + 1; i < 6; i++) {
		retry = FALSE;

		/* avoid towns at the border of the world map (also: no housing space there!) */
		y = 2 + rand_int(MAX_WILD_Y - 4);
		x = 2 + rand_int(MAX_WILD_X - 4);

		/* check wilderness type so that Bree is in forest and
		 * Minas Anor in mountain etc */
		if (wild_info[y][x].type != town_profile[i].wild_req) {
			i--;
			continue;
		}

		/* Don't build them too near to each other */
		for (j = 0; j < i - 1; j++) {
			if (distance(y, x, town[j].y, town[j].x) < 8) {
				retry = TRUE;
				break;
			}
		}
		if (retry) {
			i--;
			continue;
		}
		addtown(y, x, town_profile[i].dun_base, 0, i);	/* base town */
	}

	/* Place dungeons */
	for (i = 1; i < max_d_idx; i++) {
		retry = FALSE;

		/* Skip empty entry */
		if (!d_info[i].name) continue;

		/* Hack -- omit dungeons associated with towns */
		if (tries == 100) {
			skip = FALSE;

			for (j = 1; j < 6; j++) {
				for (k = 0; k < 2; k++) {
					if (town_profile[j].dungeons[k] == i) skip = TRUE;
				}
			}

			if (skip) continue;
		}

		y = rand_int(MAX_WILD_Y);
		x = rand_int(MAX_WILD_X);

		wpos.wy = y;
		wpos.wx = x;

		/* reserve sector 0,0 for special occasions, such as
		   Highlander Tournament dungeon and PvP Arena tower - C. Blue */
		if (!y && !x) retry = TRUE; /* ((sector00separation)) */

		/* Don't build them too near to towns
		 * (otherwise entrance can be within a house) */
		for (j = 0; j < 5; j++) {
			if (distance(y, x, town[j].y, town[j].x) <= MAX_TOWNAREA) {
				retry = TRUE;
				break;
			}
		}
		if (!retry) {
			if (wild_info[y][x].dungeon || wild_info[y][x].tower) retry = TRUE;

			/* TODO: easy dungeons around Bree,
			 * hard dungeons around Lorien */
		}
		if (retry) {
			if (tries-- > 0) i--;
			continue;
		}

		add_dungeon(&wpos, 0, 0, 0, 0, 0, FALSE, i, 0, 0, 0);

		/* 0 or MAX_{HGT,WID}-1 are bad places for stairs - mikaelh */
		if (d_info[i].flags1 & DF1_TOWER) {
			new_level_down_y(&wpos, 2 + rand_int(MAX_HGT - 4));
			new_level_down_x(&wpos, 2 + rand_int(MAX_WID - 4));
		} else {
			new_level_up_y(&wpos, 2 + rand_int(MAX_HGT - 4));
			new_level_up_x(&wpos, 2 + rand_int(MAX_WID - 4));
		}
#if 0
		if ((zcave = getcave(&p_ptr->wpos))) {
			zcave[p_ptr->py][p_ptr->px].feat = FEAT_MORE;
		}
#endif	// 0

#if DEBUG_LEVEL > 0
		s_printf("Dungeon %d is generated in %s.\n", i, wpos_format(0, &wpos));
#endif	// 0

		tries = 100;
	}
}

void init_wild_info() {
	int x, y;

	memset(&wild_info[0][0],0,sizeof(wilderness_type)*(MAX_WILD_Y*MAX_WILD_X));

	/* evileye test new wilderness map */
	initwild();

	/* Jir tests new town allocator */
	addtown(cfg.town_y, cfg.town_x, cfg.town_base, 0, 1);	/* base town */
	//if (new) wild_spawn_towns();

	//init_wild_info_aux(0,0);

	for (x = 0; x < MAX_WILD_X; x++) {
		for (y = 0; y < MAX_WILD_Y; y++) {
			wild_info[y][x].bled = WILD_GRASSLAND;
		}
	}
}



/* In the future, add all sorts of cool stuff could be added, such as clusters of
 * buildings or abandoned towns. Also, maybe hack on additional wilderness
   dungeons or "basements" of houses, which could be stored with indicies
   > 128 and acceced by some sort of adressing array. Hmm, maybe make a
   dungeon_type... of which the town's dungeon could be one. It would have
   world_x, world_y, local_x, local_y, depth, type, etc.

   The current wilderness generation is a quick hack generally, and it would be
   cool if it was rewritten in the future with some sort of fractal system involving
   seas, rivers, mountains, hills, mines, towns, roads, farms, etc.

   HACK -- I added a WILD_CLONE type of terrain, which sets the terrain type to that
   of a random neighbor, and if that neighbor is clone it goes rescursive until
   it finds a non-clone piece of terrain.  This will hopefully provide more
   unified terrain. (i.e. big forests, big lakes, etc )
*/


/*
 * Helper function for wild_add_monster
 *
 * a hack not to have elven archers etc. on town.
 */
static bool wild_monst_aux_town(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

#if 0
	/* Maggot is allowed :) */
	if (!strcmp(&r_name[r_ptr->name],"Farmer Maggot")) return TRUE;
#endif

	if (r_ptr->flags8 & RF8_WILD_TOWN) return TRUE;
	return FALSE;
}


/*
 * Helper function for wild_add_monster
 */
static bool wild_monst_aux_lake(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_LAKE) return TRUE;

	/* no reproducing monsters allowed */
	if (r_ptr->flags7 & RF7_MULTIPLY) return FALSE;

	if (r_ptr->flags2 & RF2_AURA_FIRE) return FALSE;

	/* animals are OK */
	if (r_ptr->flags3 & RF3_ANIMAL) return TRUE;
	/* humanoids and other races are OK */
	if (strchr("ph", r_ptr->d_char)) return TRUE;

#if 0 /* could be salt-water aquatics.. */
	/* always allow aquatics! */
	if (r_ptr->flags7 & (RF7_AQUATIC | RF7_CAN_SWIM | RF7_CAN_FLY)) return TRUE;
#else
	if (r_ptr->flags7 & (RF7_CAN_SWIM | RF7_CAN_FLY)) return TRUE;
#endif

	/* No */
	return FALSE;
}

static bool wild_monst_aux_river(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_LAKE) return TRUE;

	/* no reproducing monsters allowed */
	if (r_ptr->flags7 & RF7_MULTIPLY) return FALSE;

	if (r_ptr->flags2 & RF2_AURA_FIRE) return FALSE;

	/* animals are OK */
	if (r_ptr->flags3 & RF3_ANIMAL) return TRUE;
	/* humanoids and other races are OK */
	if (strchr("ph", r_ptr->d_char)) return TRUE;

#if 0 /* could be salt-water aquatics.. */
	/* always allow aquatics! */
	if (r_ptr->flags7 & (RF7_AQUATIC | RF7_CAN_SWIM | RF7_CAN_FLY)) return TRUE;
#else
	if (r_ptr->flags7 & (RF7_CAN_SWIM | RF7_CAN_FLY)) return TRUE;
#endif

	/* No */
	return FALSE;
}

static bool wild_monst_aux_grassland(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_GRASS) return TRUE;

	/* no reproducing monsters allowed */
	if (r_ptr->flags7 & RF7_MULTIPLY) return FALSE;

	return FALSE;
}

static bool wild_monst_aux_forest(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_WOOD) return TRUE;
	return FALSE;
}

static bool wild_monst_aux_swamp(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_SWAMP) return TRUE;
	return FALSE;
}

/* Hrm.. now it's exactly same with normal forest.. */
static bool wild_monst_aux_denseforest(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_WOOD) return TRUE;
	return FALSE;
}

static bool wild_monst_aux_wasteland(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_WASTE) return TRUE;
	return FALSE;
}

static bool wild_monst_aux_desert(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_DESERT) return TRUE;

	/* borrow from wasteland monsters */
	//if (r_ptr->flags8 & RF8_WILD_WASTE) return TRUE;

	/* no aquatic life here */
	if (r_ptr->flags7 & RF7_AQUATIC) return FALSE;

	/* monsters that don't mind dry environment */
	if (strchr("acsuwBCFIJKRS", r_ptr->d_char)) return TRUE;

	return FALSE;
}

static bool wild_monst_aux_ice(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_ICE) return TRUE;

	/* no aquatic life here */
	if (r_ptr->flags7 & RF7_AQUATIC) return FALSE;

	/* no cold-susceptible monsters */
	if (r_ptr->flags3 & RF3_SUSCEP_COLD) return FALSE;

	/* monsters that don't mind cold environment */
	if (r_ptr->flags3 & RF3_IM_COLD) return TRUE;
	if (r_ptr->flags9 & RF9_RES_COLD) return TRUE;

	/* borrow from wasteland monsters */
	//if (r_ptr->flags8 & RF8_WILD_WASTE) return TRUE;

	return FALSE;
}

static bool wild_monst_aux_ocean(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_OCEAN) return TRUE;
	return FALSE;
}

static bool wild_monst_aux_oceanbed(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_OCEAN) return TRUE;
	/* No fresh water creatures */
	if ((r_ptr->flags8 & RF8_WILD_SHORE) && !(r_ptr->flags8 & RF8_WILD_SWAMP)) return TRUE;
	return FALSE;
}

static bool wild_monst_aux_shore(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_SHORE) return TRUE;
	return FALSE;
}

static bool wild_monst_aux_volcano(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_VOLCANO) return TRUE;
	/* Borrow some monsters from mountains */
	else if ((r_ptr->flags8 & RF8_WILD_MOUNTAIN) && (r_ptr->flags3 & RF3_IM_FIRE)) return TRUE;
	return FALSE;
}

static bool wild_monst_aux_mountain(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	/* Borrow some monsters from volcanoes */
	if (r_ptr->flags8 & (RF8_WILD_MOUNTAIN | RF8_WILD_VOLCANO)) return TRUE;
	return FALSE;
}

void set_mon_num_hook_wild(struct worldpos *wpos) {
	switch (wild_info[wpos->wy][wpos->wx].type) {
	case WILD_OCEANBED1: case WILD_OCEANBED2: get_mon_num_hook = wild_monst_aux_oceanbed; break;
	case WILD_OCEAN: get_mon_num_hook = wild_monst_aux_ocean; break;
	case WILD_SHORE1: case WILD_SHORE2: get_mon_num_hook = wild_monst_aux_shore; break;
	case WILD_RIVER: get_mon_num_hook = wild_monst_aux_river; break;
	case WILD_LAKE: get_mon_num_hook = wild_monst_aux_lake; break;
	case WILD_SWAMP: get_mon_num_hook = wild_monst_aux_swamp; break;
	case WILD_FOREST: get_mon_num_hook = wild_monst_aux_forest; break;
	case WILD_DENSEFOREST: get_mon_num_hook = wild_monst_aux_denseforest; break;
	case WILD_GRASSLAND: get_mon_num_hook = wild_monst_aux_grassland; break;
	case WILD_WASTELAND: get_mon_num_hook = wild_monst_aux_wasteland; break;
	case WILD_MOUNTAIN: get_mon_num_hook = wild_monst_aux_mountain; break;
	case WILD_VOLCANO: get_mon_num_hook = wild_monst_aux_volcano; break;
	case WILD_DESERT: get_mon_num_hook = wild_monst_aux_desert; break;
	case WILD_ICE: get_mon_num_hook = wild_monst_aux_ice; break;
	case WILD_TOWN: get_mon_num_hook = wild_monst_aux_town; break;
	default: get_mon_num_hook = dungeon_aux;
	}
}

/* this may not be the most efficient way of doing things... */
void wild_add_monster(struct worldpos *wpos) {
	int monst_x, monst_y, r_idx;
	int tries = 0;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Don't spawn during highlander tournament or global events in general (ancient D vs lvl 10 is silyl) */
	if (sector00separation && in_sector00(wpos)) return;

	/* reset the monster sorting function */
	set_mon_num_hook_wild(wpos);

	/* find a legal, unoccupied space */
	do {
		monst_x = rand_int(MAX_WID);
		monst_y = rand_int(MAX_HGT);

		if (cave_naked_bold(zcave, monst_y, monst_x)) break;
		tries++;
	} while (tries < 50);

	if (!cave_naked_bold(zcave, monst_y, monst_x)) {
		/* Give up */
		return;
	}

	/* Set the second hook according to the terrain type */
	set_mon_num2_hook(zcave[monst_y][monst_x].feat);

	get_mon_num_prep(0, NULL);

	/* get the monster */
	r_idx = get_mon_num(monster_level, monster_level);

	/* place the monster */
	place_monster_aux(wpos, monst_y, monst_x, r_idx, FALSE, TRUE, FALSE, 0);

	/* hack -- restore the monster selection function */
	get_mon_num_hook = dungeon_aux;
}




/* chooses a clear building location, possibly specified by xcen, ycen, and "reserves" it so
 * nothing else can choose any of its squares for building again */
static void reserve_building_plot(struct worldpos *wpos, int *x1, int *y1, int *x2, int *y2, int xlen, int ylen, int xcen, int ycen) {
	int x,y, attempts = 0, plot_clear;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

#ifdef DEVEL_TOWN_COMPATIBILITY
	while (attempts < 200)
#else
	while (attempts < 20)
#endif
	{

		/* if xcen, ycen have not been specified */
		if (!in_bounds(ycen,xcen)) {
#ifdef DEVEL_TOWN_COMPATIBILITY
			/* the upper left corner */
			*x1 = rand_int(MAX_WID-xlen-3)+1;
			*y1 = rand_int(MAX_HGT-ylen-3)+1;
#else
			/* the upper left corner */
			*x1 = rand_int(MAX_WID-xlen-4)+2;
			*y1 = rand_int(MAX_HGT-ylen-4)+2;
#endif
			/* the lower right corner */
			*x2 = *x1 + xlen-1;
			*y2 = *y1 + ylen-1;
		} else {
			*x1 = xcen - xlen/2;
			*y1 = ycen - ylen/2;
			*x2 = *x1 + xlen-1;
			*y2 = *y1 + ylen-1;

			if ( (!in_bounds(*y1, *x1)) ||
			     (!in_bounds(*y2, *x2)) ) {
				*x1 = *y1 = *x2 = *y2 = -1;
				return;
			}
		}

		plot_clear = 1;

		/* check if its clear */
		for (y = *y1; y <= *y2; y++) {
			for (x = *x1; x <= *x2; x++) {
				switch (zcave[y][x].feat) {
					/* Don't build on other buildings or farms */
					case FEAT_LOOSE_DIRT:
					case FEAT_CROP:
					case FEAT_WALL_EXTRA:
					case FEAT_PERM_EXTRA:
					case FEAT_WALL_HOUSE:
					case FEAT_LOGS:
						plot_clear = 0;
						break;
					default: break;
				}
#ifndef DEVEL_TOWN_COMPATIBILITY
				/* any ickiness on the plot is NOT allowed */
				if (zcave[y][x].info & CAVE_ICKY) plot_clear = 0;
				/* spaces that have already been reserved are NOT allowed */
				if (zcave[y][x].info & CAVE_XTRA) plot_clear = 0;
#endif
			}
		}

		/* hack -- buildings and farms can partially, but not completly,
		   be built on water. */
		if ( (zcave[*y1][*x1].feat == FEAT_DEEP_WATER) &&
		     (zcave[*y2][*x2].feat == FEAT_DEEP_WATER) ) plot_clear = 0;

		/* if we have a clear plot, reserve it and return */
		if (plot_clear) {
			for (y = *y1; y <= *y2; y++) {
				for (x = *x1; x <= *x2; x++) {
					zcave[y][x].info |= CAVE_XTRA;
				}
			}
			return;
		}

		attempts++;
	}

	/* plot allocation failed */
	*x1 = *y1 = *x2 = *y2 = -1;
}

/* Adds a garden a reasonable distance from x,y.
   Some crazy games are played with the RNG, so that whether we are dropping
   food or not will not effect the final state it is in.
*/

static void wild_add_garden(struct worldpos *wpos, int x, int y) {
	int x1, y1, x2, y2, type, xlen, ylen;
#ifdef DEVEL_TOWN_COMPATIBILITY
	int attempts = 0;
#endif
	int i;
	char orientation;
	object_type food, *o_ptr;
	int tmp_seed;
	wilderness_type *w_ptr = &wild_info[wpos->wy][wpos->wx];
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	x1 = x2 = y1 = y2 = -1;

	/* choose which type of garden it is */
	type = rand_int(7);

	/* choose a 'good' location for the garden */

	xlen = rand_int(rand_int(60)) + 15;
	ylen = rand_int(rand_int(20)) + 7;

#ifdef DEVEL_TOWN_COMPATIBILITY
	/* hack -- maximum distance to house -- 30 */
	while (attempts < 100) {
#endif

		reserve_building_plot(wpos, &x1,&y1, &x2,&y2, xlen, ylen, -1, -1);
#ifdef DEVEL_TOWN_COMPATIBILITY
		/* we have obtained a valid plot */
		if (x1 > 0) {
			 /* maximum distance to field of 40 */
			if ( ((x1-x)*(x1-x) + (y1-y)*(y1-y) <= 40*40) ||
			     ((x2-x)*(x2-x) + (y2-y)*(y2-y) <= 40*40) ) break;
		}
		attempts++;
	}
#endif

	/* if we failed to obtain a valid plot */
	if (x1 < 0) return;

	/* whether the crop rows are horizontal or vertical */
	orientation = rand_int(2);

	/* initially fill with a layer of dirt */
	for (y = y1; y <= y2; y++) {
		for (x = x1; x <= x2; x++) {
			zcave[y][x].feat = FEAT_LOOSE_DIRT;
		}
	}

	/* save the RNG */
	tmp_seed = Rand_value;

	/* randomize, so food doesn't grow at always same garden coordinates */
	Rand_value = turn;

	/* initially clear all previous items from it! */
	if (!(w_ptr->flags & WILD_F_GARDENS))
	for (i = 0; i < o_max; i++) {
		o_ptr = &o_list[i];
		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;
		/* Skip objects not here */
		if (!inarea(&o_ptr->wpos, wpos)) continue;
		/* Skip carried objects */
		if (o_ptr->held_m_idx) continue;
		/* if it's on our field, erase it */
		if (o_ptr->iy >= y1 && o_ptr->iy <= y2 &&
		    o_ptr->ix >= x1 && o_ptr->ix <= x2)
			delete_object_idx(i, TRUE);
	}

	/* alternating rows of crops */
	for (y = y1+1; y <= y2-1; y ++) {
		for (x = x1+1; x <= x2-1; x++) {
			/* different orientations */
			if (((!orientation) && (y%2)) || ((orientation) && (x%2))) {
				/* set to crop */
				zcave[y][x].feat = FEAT_CROP;
				/* random chance of food */
				if (rand_int(100) < 40) {
					switch (type) {
					case WILD_CROP_POTATO:
						invcopy(&food, lookup_kind(TV_FOOD, SV_FOOD_POTATO));
						break;

					case WILD_CROP_CABBAGE:
						invcopy(&food, lookup_kind(TV_FOOD, SV_FOOD_HEAD_OF_CABBAGE));
						break;

					case WILD_CROP_CARROT:
						invcopy(&food, lookup_kind(TV_FOOD, SV_FOOD_CARROT));
						break;

					case WILD_CROP_BEET:
						invcopy(&food, lookup_kind(TV_FOOD, SV_FOOD_BEET));
						break;

					case WILD_CROP_SQUASH:
						invcopy(&food, lookup_kind(TV_FOOD, SV_FOOD_SQUASH));
						break;

					case WILD_CROP_CORN:
						invcopy(&food, lookup_kind(TV_FOOD, SV_FOOD_EAR_OF_CORN));
						break;

					/* hack -- useful mushrooms are rare */
					case WILD_CROP_MUSHROOM:
						invcopy(&food, lookup_kind(TV_FOOD, rand_int(rand_int(20))));
						break;
					default:
						invcopy(&food, lookup_kind(TV_FOOD, SV_FOOD_SLIME_MOLD));
						break;
					}
					/* Hack -- only drop food the first time */
					food.marked2 = ITEM_REMOVAL_NEVER;
					if (!(w_ptr->flags & WILD_F_GARDENS)) drop_near(0, &food, -1, wpos, y, x);
				}
			}
		}
	}
	/* restore the RNG */
	Rand_value = tmp_seed;
}


static bool wild_monst_aux_invaders(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	if(r_ptr->flags7 & RF7_AQUATIC) return FALSE;

	/* invader species */
	if (strchr("oTpOKbrm", r_ptr->d_char)) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Dark elven mage")) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Dark elven priest")) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Dark elven warrior")) return TRUE;

	return FALSE;
}

static bool wild_monst_aux_home_owner(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	/* not aquatic */
	if (r_ptr->flags7 & RF7_AQUATIC) return FALSE;

	/* home owner species */
	if (strchr("hpP", r_ptr->d_char)) return TRUE;

	return FALSE;
}

static int wild_obj_aux_bones(int k_idx, u32b resf) {
	object_kind *k_ptr = &k_info[k_idx];

	/* paranoia */
	if (k_idx < 0) return 0;

	if (k_ptr->tval == TV_SKELETON) return 1000;

	return 0;
}

/* make a dwelling (building in the wilderness) 'interesting'.
*/
static void wild_furnish_dwelling(struct worldpos *wpos, int x1, int y1, int x2, int y2, int type) {
	int x,y, cash, num_food, num_objects, num_bones, trys, r_idx, k_idx, food_sval;
	bool inhabited, at_home, taken_over;
	object_type forge;
	u32b old_seed = Rand_value;
	wilderness_type *w_ptr = &wild_info[wpos->wy][wpos->wx];
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	trys = cash = num_food = num_objects = num_bones = 0;
	inhabited = at_home = taken_over = FALSE;

	/* hack -- 75% of buildings are inhabited */
	if (rand_int(100) < 75) inhabited = TRUE;

	switch (type) {
		case WILD_LOG_CABIN:
			/* possibly add a farm */
			/* hack -- no farms near the town */
			if (w_ptr->radius > 1) {
				/* are we a farmer? */
				if (rand_int(100) < 50) wild_add_garden(wpos, (x1+x2)/2,(y1+y2)/2);
			}
		case WILD_ROCK_HOME:
			/* hack -- no farms near the town */
			if (w_ptr->radius > 1) {
				/* are we a farmer? */
				if (rand_int(100) < 40) wild_add_garden(wpos, (x1+x2)/2,(y1+y2)/2);
			}

		case WILD_PERM_HOME:
			if (inhabited) {
				/* is someone to be found at this house */
				if (rand_int(100) < 80) at_home = TRUE;

				/* is there any food inside */
				if (rand_int(100) < 80) num_food = rand_int(rand_int(20));

				/* is there any cash inside */
				if (rand_int(100) < 40) cash = rand_int(10);

				/* are there objects to be found */
				if (rand_int(100) < 50) num_objects = rand_int(rand_int(10));
			} else {
				if (rand_int(100) < 50) {
					/* taken over! */
					taken_over = TRUE;
					if (rand_int(100) < 40) cash = rand_int(20);
					if (rand_int(100) < 50) num_objects = rand_int(rand_int(10));
					if (rand_int(100) < 33) num_food = rand_int(3);
					num_bones = rand_int(20);
				}
				num_bones = rand_int(rand_int(1));
			}
			break;
	}


	/* add the cash */
	if (!(w_ptr->flags & WILD_F_CASH) && cash) {
		/* try to place the cash */
		while (trys < 50) {
			x = rand_range(x1,x2);
			y = rand_range(y1,y2);

			if (cave_clean_bold(zcave,y,x)) {
				object_level = cash;
				place_gold(wpos,y,x, 0);
				break;
			}
		trys++;
		}
	}

	/* add the objects */
	if (!(w_ptr->flags & WILD_F_OBJECTS)) {
		trys = 0;
		place_object_restrictor = RESF_NOHIDSM;
		while ((num_objects) && (trys < 300)) {
			x = rand_range(x1,x2);
			y = rand_range(y1,y2);

			if (cave_clean_bold(zcave,y,x)) {
				object_level = w_ptr->radius/2 +1;
				place_object(wpos, y, x, FALSE, FALSE, FALSE, RESF_LOW, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
				num_objects--;
			}
			trys++;
		}
		place_object_restrictor = RESF_NONE;
	}

	/* add the food */
	if (!(w_ptr->flags & WILD_F_FOOD)) {
		trys = 0;
		while ((num_food) && (trys < 100)) {
			x = rand_range(x1,x2);
			y = rand_range(y1,y2);

			if (cave_clean_bold(zcave,y,x)) {
				food_sval = SV_FOOD_MIN_FOOD+rand_int(12);
				/* hack -- currently no food svals between 25 and 32 */
				if (food_sval > 25) food_sval += 6;
				/* hack -- currently no sval 34 */
				if (food_sval > 33) food_sval++;

				k_idx = lookup_kind(TV_FOOD,food_sval);
				invcopy(&forge, k_idx);
				forge.marked2 = ITEM_REMOVAL_NEVER;
				drop_near(0, &forge, -1, wpos, y, x);

				num_food--;
			}
			trys++;
		}
	}

	/* add the bones */
	if (!(w_ptr->flags & WILD_F_BONES)) {
		trys = 0;
		get_obj_num_hook = wild_obj_aux_bones;
		get_obj_num_prep(RESF_WILD);

		while ((num_bones) && (trys < 100)) {
			x = rand_range(x1,x2);
			y = rand_range(y1,y2);

			if (cave_clean_bold(zcave,y,x)) {
				/* base of 500 feet for the bones */
				k_idx = get_obj_num(10, RESF_NONE);
				invcopy(&forge, k_idx);
				forge.marked2 = ITEM_REMOVAL_NEVER;
				drop_near(0, &forge, -1, wpos, y, x);

				num_bones--;
			}
			trys++;
		}
	}


	/* hack -- restore the old object selection function */
	get_obj_num_hook = NULL;
	get_obj_num_prep(RESF_NONE);

	/* add the inhabitants */
	if (!(w_ptr->flags & WILD_F_HOME_OWNERS)) {
		if (at_home) {
			/* determine the home owners species */
			get_mon_num_hook = wild_monst_aux_home_owner;

			/* Set the second hook according to the floor type */
			set_mon_num2_hook(zcave[y1][x1].feat);

			get_mon_num_prep(0, NULL);

			/* homeowners can be tough */
			r_idx = get_mon_num(w_ptr->radius, w_ptr->radius);

			/* get the owners location */
#if 0 /* won't this cause monsters to be randomly spawned possibly _inside adjancent player houses_? o_O */
			x = rand_range(x1, x2) + rand_int(40) - 20;
			y = rand_range(y1, y2) + rand_int(16) - 8;
#else
			x = rand_range(x1, x2);
			y = rand_range(y1, y2);
#endif

			/* place the owner */
			summon_override_checks = SO_HOUSE;
			place_monster_aux(wpos, y, x, r_idx, FALSE, FALSE, FALSE, 0);
			summon_override_checks = SO_NONE;
		}
	}

	/* add the invaders */
	if (!(w_ptr->flags & WILD_F_INVADERS)) {
		if (taken_over) {
			/* determine the invaders species*/
			get_mon_num_hook = wild_monst_aux_invaders;

			/* Set the second hook according to the floor type */
			set_mon_num2_hook(zcave[y1][x1].feat);

			get_mon_num_prep(0, NULL);
			r_idx = get_mon_num((w_ptr->radius / 2) + 1, w_ptr->radius / 2);

			/* add the monsters */
			summon_override_checks = SO_HOUSE;
			for (y = y1; y <= y2; y++) {
				for (x = x1; x <= x2; x++) {
					place_monster_aux(wpos, y,x, r_idx, FALSE, FALSE, FALSE, 0);
				}
			}
			summon_override_checks = SO_NONE;
		}
	}


	/* hack -- restore the RNG */
	Rand_value = old_seed;
}

/* check if a location is suitable for placing a door	- Jir - */
/* TODO: check for houses built *after* door creation */
#ifdef __DISABLE_HOUSEBOOST
static bool dwelling_check_entrance(worldpos *wpos, int y, int x) {
	int i;
	cave_type *c_ptr;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return(FALSE);

	for (i = 1; i < tdi[1]; i++) {
		c_ptr = &zcave[y + tdy[i]][x + tdx[i]];

		/* Inside a house */
		if (c_ptr->info & CAVE_ICKY) continue;

		/* Wall blocking the door */
		if (f_info[c_ptr->feat].flags1 & FF1_WALL) continue;

		/* Nasty terrain covering the door */
		if (c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_DEEP_LAVA)
			continue;

		/* Found a neat entrance */
		return (TRUE);
	}

	/* Assume failure */
	return (FALSE);
}
#else
static bool dwelling_check_entrance(worldpos *wpos, int y, int x, int dir) {
	/*
	 * HACK: There's a bug below where negative indices were accessed in ddx_cyc and ddy_cyc.
	 * The result is undefined and depends on which compiler is used.
	 * In order to preserve housing layout on the official server, we're going to emulate negative indices.
	 * If we decide to do a worldmap reset, this hack can be removed.
	 * - mikaelh
	 */
#define EMULATE_NEGATIVE_INDICES
#ifdef EMULATE_NEGATIVE_INDICES
	static s16b ddx_cyc_emul[10] = { -1, 0, -1, 0, 1, 1, 1, 0, -1, -1 };
	static s16b ddy_cyc_emul[10] = { -1, -1, -1, -1, -1, 0, 1, 1, 1, 0 };
	static s16b *ddx_cyc = &ddx_cyc_emul[2];
	static s16b *ddy_cyc = &ddy_cyc_emul[2];
#endif
	int i;
	cave_type *c_ptr;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return FALSE;

	switch (dir) {
	/* corners: 5 adjacent grids to check */
	case 0: case 2: case 4: case 6:
		for (i = -2; i <= 2; i++) {
#ifdef EMULATE_NEGATIVE_INDICES
		        c_ptr = &zcave[y + ddy_cyc[(dir + i) % 8]][x + ddx_cyc[(dir + i) % 8]];
#else
		        c_ptr = &zcave[y + ddy_cyc[(dir + i + 8) % 8]][x + ddx_cyc[(dir + i + 8) % 8]];
#endif

			/* Inside a house */
			if (c_ptr->info & CAVE_ICKY) continue;

			/* Wall blocking the door */
			if (f_info[c_ptr->feat].flags1 & FF1_WALL) continue;

			/* Nasty terrain covering the door */
			if (c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_DEEP_LAVA)
				continue;

			/* Found a neat entrance */
			return TRUE;
		}
		break;
	/* sides: 3 adjacent grids to check */
	case 1: case 3: case 5: case 7:
		for (i = -1; i <= 1; i++) {
    		        c_ptr = &zcave[y + ddy_cyc[(dir + i) % 8]][x + ddx_cyc[(dir + i) % 8]];

			/* Inside a house */
			if (c_ptr->info & CAVE_ICKY) continue;

			/* Wall blocking the door */
			if (f_info[c_ptr->feat].flags1 & FF1_WALL) continue;

			/* Nasty terrain covering the door */
			if (c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_DEEP_LAVA)
				continue;

			/* Found a neat entrance */
			return TRUE;
		}
		break;
	}

	/* Assume failure */
	return FALSE;
}
#endif

/* adds a building to the wilderness. if the coordinate is not given,
   find it randomly.

 for now will make a simple box,
   but we could do really fun stuff with this later.
*/
static void wild_add_dwelling(struct worldpos *wpos, int x, int y) {
	int	h_x1,h_y1,h_x2,h_y2, p_x1,p_y1,p_x2,p_y2,
		plot_xlen, plot_ylen, house_xlen, house_ylen,
		door_x, door_y, drawbridge_x[3], drawbridge_y[3],
		tmp, type, area, num_door_attempts;
	int size;
#ifndef __DISABLE_HOUSEBOOST
	int xx, yy, door_dir = 0;
	bool hinders_door = FALSE;
#endif
	char wall_feature = 0, door_feature = 0;
	char has_moat = 0;
	cave_type *c_ptr;
	bool rand_old = Rand_quick;
	bool trad = !magik(MANG_HOUSE_RATE);
	wilderness_type *w_ptr = &wild_info[wpos->wy][wpos->wx];
	cave_type **zcave;
	u32b floor_info;

	if (!(zcave = getcave(wpos))) return;

	/* Initialize drawbridge_x and drawbridge_y to make gcc happy */
	drawbridge_x[0] = drawbridge_x[1] = drawbridge_x[2] = 0;
	drawbridge_y[0] = drawbridge_y[1] = drawbridge_y[2] = 0;

	/* Hack -- Use the "simple" RNG */
	Rand_quick = TRUE;

	/* Hack -- Induce consistant wilderness */
	/* Rand_value = seed_town + (Depth * 600) + (w_ptr->dwellings * 200);*/


	/* find the dimensions of the house -
	   depends on its placement within MAX_TOWNAREA */

#ifdef DEVEL_TOWN_COMPATIBILITY
	house_xlen = rand_int(10) + 3;
	house_ylen = rand_int(5) + 3;
	plot_xlen = house_xlen;
	plot_ylen = house_ylen;
#else
	/* chance of being a "large" house */
#ifdef __DISABLE_HOUSEBOOST
	if (!rand_int(2)) {
#else
	/* more *huge* houses (villas) on the outskirts */
	if (w_ptr->radius == 3 && rand_int(3)) {
		house_xlen = rand_int(5) + rand_int(5) + 19;
		house_ylen = rand_int(3) + rand_int(3) + 10;
	}
	/* less "large" houses (to leave room for *huge* houses) in radius 3 */
	else if (!rand_int(w_ptr->radius == 3 ? 4 : 2)) {
#endif
		house_xlen = rand_int(10) + rand_int(rand_int(10)) + 9;
		house_ylen = rand_int(5) + rand_int(rand_int(5)) + 6;
	}
	/* chance of being a "small" house */
#ifdef __DISABLE_HOUSEBOOST
	else if (!rand_int(2)) {
#else
	else if (!rand_int(w_ptr->radius == 1 ? 8 : 12)) {
#endif
		house_xlen = rand_int(4) + 3;
		house_ylen = rand_int(2) + 3;
	}
	/* a "normal" house */
	else {
#ifdef __DISABLE_HOUSEBOOST
		house_xlen = rand_int(10) + 3;
		house_ylen = rand_int(5) + 3;
#else
		house_xlen = rand_int(7) + 6;
		house_ylen = rand_int(4) + 4;
#endif
	}
	area = (house_xlen-2) * (house_ylen-2);

	/* find the dimensions of the "lawn" the house is built on */
#ifdef __DISABLE_HOUSEBOOST
	if (area < 30) {
#else
	if (area < 45) {
#endif
		plot_xlen = house_xlen;
		plot_ylen = house_ylen;
#ifdef __DISABLE_HOUSEBOOST
	} else if (area < 60) {
		plot_xlen = house_xlen + (area/15)*2;
		plot_ylen = house_ylen + (area/25)*2;
		//plot_xlen = house_xlen + (area/10)*2;
		//plot_ylen = house_ylen + (area/16)*2;
		trad = FALSE;
	} else {
		plot_xlen = house_xlen + (area/8)*2;
		plot_ylen = house_ylen + (area/14)*2;
		trad = FALSE;
	}
#else /* less lawn */
	} else if (area < 100) {
		plot_xlen = house_xlen + (area/20);
		plot_ylen = house_ylen + (area/30);
		trad = FALSE;
	} else {
		plot_xlen = house_xlen + (area/20) + 5;
		plot_ylen = house_ylen + (area/30) + 4;
		trad = FALSE;
	}
#endif

	/* Hack -- sometimes large buildings get moats */
#ifdef __DISABLE_HOUSEBOOST
	if ((area >= 70) && (!rand_int(16))) has_moat = 1;
	if ((area >= 80) && (!rand_int(6))) has_moat = 1;
	if ((area >= 100) && (!rand_int(2))) has_moat = 1;
	if ((area >= 130) && (rand_int(4) < 3)) has_moat = 1;
#else
 #if 0/* actually made moats less likely again due to new cfg.castles_per_player - this will also save building space */
	if (area >= 80 && !rand_int(9)) has_moat = 1;
	if (area >= 80 && !rand_int(6)) has_moat = 1;
	if (area >= 100 && !rand_int(2)) has_moat = 1;
	if (area >= 130 && rand_int(4) < 3) has_moat = 1;
 #else
//	if (area >= 100 && !rand_int(15)) has_moat = 1;
//	else if (area >= 140 && !rand_int(9)) has_moat = 1;
//	else if (area >= 180 && !rand_int(4)) has_moat = 1;
//	else if (area >= 200 && !rand_int(2)) has_moat = 1;
	if (area >= 220 && rand_int(3)) has_moat = 1;
 #endif
#endif
	if (has_moat) plot_xlen += 8;
	if (has_moat) plot_ylen += 8;
#endif

	/* Determine the plot's boundaries */
	reserve_building_plot(wpos, &p_x1, &p_y1, &p_x2, &p_y2, plot_xlen, plot_ylen, x, y);
	/* Determine the building's boundaries */
	h_x1 = p_x1 + ((plot_xlen - house_xlen)/2); h_y1 = p_y1 + ((plot_ylen - house_ylen)/2);
	h_x2 = p_x2 - ((plot_xlen - house_xlen)/2); h_y2 = p_y2 - ((plot_ylen - house_ylen)/2);

	/* return if we didn't get a plot */
	if (p_x1 < 0) return;

	/* initialise x and y, which may not be specified at this point */
	x = (h_x1 + h_x2) / 2;
	y = (h_y1 + h_y2) / 2;

	/* determine what kind of building it is */
	if (rand_int(100) < 60) type = WILD_LOG_CABIN;
	else if (rand_int(100) < 8) type = WILD_PERM_HOME;
	else type = WILD_ROCK_HOME;

	/* hack -- add extra "for sale" homes near the town */
	if (w_ptr->radius == 1) {
		/* hack -- not many log cabins near town */
		if (type == WILD_LOG_CABIN) {
#ifdef __DISABLE_HOUSEBOOST
			if (rand_int(100) < 80) type = WILD_ROCK_HOME;
#else
			if (rand_int(100) < 90) type = WILD_ROCK_HOME;
#endif
		}
#ifdef DEVEL_TOWN_COMPATIBILITY
		if (rand_int(100) < 40) type = WILD_TOWN_HOME;
#else
 #ifdef __DISABLE_HOUSEBOOST
		if (rand_int(100) < 90) type = WILD_TOWN_HOME;
 #else
		if (rand_int(100) < 95) type = WILD_TOWN_HOME;
 #endif
#endif
	}
	if (w_ptr->radius == 2)
#ifdef DEVEL_TOWN_COMPATIBILITY
		if (rand_int(100) < 10) type = WILD_TOWN_HOME;
#else
 #ifdef __DISABLE_HOUSEBOOST
		if (rand_int(100) < 80) type = WILD_TOWN_HOME;
 #else
		if (rand_int(100) < 90) type = WILD_TOWN_HOME;
 #endif
#endif
#ifndef __DISABLE_HOUSEBOOST
	if ((w_ptr->radius == 3 && rand_int(100) < 80)
	    || has_moat) /* no log cabins or rock homes with moats... */
		type = WILD_TOWN_HOME;

	/* also, no giant log cabins or rock homes (near towns) */
	if (type == WILD_LOG_CABIN && area >= 50) type = WILD_ROCK_HOME;
	if (type == WILD_ROCK_HOME && area >= 80 && w_ptr->radius <= 3) type = WILD_TOWN_HOME;
#endif

#ifndef __DISABLE_HOUSEBOOST
	/* Make sure our building isn't placed so that its walls block a
	   house door of another house that has previously been generated.
	   What could still happen is that a door is within a 'backalley'
	   that isn't actually accessible without phasing. - C. Blue */
	/* Determine the building's boundaries */
	for (xx = h_x1; xx <= h_x2; xx++) {
		if (zcave[h_y1 - 1][xx].feat == FEAT_HOME ||
		    zcave[h_y1 - 1][xx].feat == FEAT_DOOR_HEAD) {
			/* reduce house height accordingly */
			h_y1++;
			hinders_door = TRUE;
			break;
		}
	}
	for (xx = h_x1; xx <= h_x2; xx++) {
		if (zcave[h_y2 + 1][xx].feat == FEAT_HOME ||
		    zcave[h_y2 + 1][xx].feat == FEAT_DOOR_HEAD) {
			/* reduce house height accordingly */
			h_y2--;
			hinders_door = TRUE;
			break;
		}
	}
	for (yy = h_y1; yy <= h_y2; yy++) {
		if (zcave[yy][h_x1 - 1].feat == FEAT_HOME ||
		    zcave[yy][h_x1 - 1].feat == FEAT_DOOR_HEAD) {
			/* reduce house height accordingly */
			h_x1++;
			hinders_door = TRUE;
			break;
		}
	}
	for (yy = h_y1; yy <= h_y2; yy++) {
		if (zcave[yy][h_x2 + 1].feat == FEAT_HOME ||
		    zcave[yy][h_x2 + 1].feat == FEAT_DOOR_HEAD) {
			/* reduce house height accordingly */
			h_x2--;
			hinders_door = TRUE;
			break;
		}
	}

	/* Recalculate house parameters? */
	if (hinders_door) {
		/* has house become too small now (smaller than 3x3)? -> abort */
		if (h_x2 - h_x1 < 2 || h_y2 - h_y1 < 2) return;

		area = (h_x2 - h_x1 - 1) * (h_y2 - h_y1 - 1);

		x = (h_x1 + h_x2) / 2;
		y = (h_y1 + h_y2) / 2;
	}
#endif

	switch (type) {
		case WILD_LOG_CABIN:
			wall_feature = FEAT_LOGS;

			/* doors are locked 1/3 of the time */
			if (rand_int(100) < 33) door_feature = FEAT_DOOR_HEAD + rand_int(7);
			else door_feature = FEAT_DOOR_HEAD;

			break;
		case WILD_PERM_HOME:
			wall_feature = FEAT_PERM_EXTRA;
//			wall_feature = FEAT_WALL_HOUSE;

			/* doors are locked 90% of the time */
			if (rand_int(100) < 90) door_feature = FEAT_DOOR_HEAD + rand_int(7);
			else door_feature = FEAT_DOOR_HEAD;
			break;
		case WILD_ROCK_HOME:
			wall_feature = FEAT_WALL_EXTRA;

			/* doors are locked 60% of the time */
			if (rand_int(100) < 60) door_feature = FEAT_DOOR_HEAD + rand_int(7);
			else door_feature = FEAT_DOOR_HEAD;
			break;
		case WILD_TOWN_HOME:
//			wall_feature = FEAT_PERM_EXTRA;
			wall_feature = FEAT_WALL_HOUSE;
			door_feature = FEAT_HOME;

			/* hack -- setup next possibile house addition */
			MAKE(houses[num_houses].dna, struct dna_type);
			houses[num_houses].x = h_x1;
			houses[num_houses].y = h_y1;
			houses[num_houses].flags = HF_RECT|HF_STOCK;
			if (trad) houses[num_houses].flags |= HF_TRAD;
			if (has_moat) houses[num_houses].flags |= HF_MOAT;
			houses[num_houses].coords.rect.width = h_x2-h_x1+1;
			houses[num_houses].coords.rect.height = h_y2-h_y1+1;
			wpcopy(&houses[num_houses].wpos, wpos);
			houses[num_houses].dna->price = initial_house_price(&houses[num_houses]);
			break;
	}


	/* select the door location... done here so we can
	   try to prevent it form being put on water. */

	/* hack -- avoid doors in water */
	/* avoid lava and wall too */
	num_door_attempts = 0;
	do {
		/* Pick a door direction (S,N,E,W) */
		tmp = rand_int(4);

		/* Extract a "door location" */
		switch (tmp) {
			/* Bottom side */
			case DIR_SOUTH:
				door_y = h_y2;
				door_x = rand_range(h_x1, h_x2);
				if (has_moat) {
					drawbridge_y[0] = h_y2+1; drawbridge_y[1] = h_y2+2;
					drawbridge_y[2] = h_y2+3;
					drawbridge_x[0] = door_x; drawbridge_x[1] = door_x;
					drawbridge_x[2] = door_x;
				}
#ifndef __DISABLE_HOUSEBOOST
				/* door directly on a corner? */
				if (door_x == h_x1) door_dir = 6;
				else if (door_x == h_x2) door_dir = 4;
				else door_dir = 5;
#endif
				break;
			/* Top side */
			case DIR_NORTH:
				door_y = h_y1;
				door_x = rand_range(h_x1, h_x2);
				if (has_moat) {
					drawbridge_y[0] = h_y1-1; drawbridge_y[1] = h_y1-2;
					drawbridge_y[2] = h_y1-3;
					drawbridge_x[0] = door_x; drawbridge_x[1] = door_x;
					drawbridge_x[2] = door_x;
				}
#ifndef __DISABLE_HOUSEBOOST
				/* door directly on a corner? */
				if (door_x == h_x1) door_dir = 0;
				else if (door_x == h_x2) door_dir = 2;
				else door_dir = 1;
#endif
				break;
			/* Right side */
			case DIR_EAST:
				door_y = rand_range(h_y1, h_y2);
				door_x = h_x2;
				if (has_moat) {
					drawbridge_y[0] = door_y; drawbridge_y[1] = door_y;
					drawbridge_y[2] = door_y;
					drawbridge_x[0] = h_x2+1; drawbridge_x[1] = h_x2+2;
					drawbridge_x[2] = h_x2+3;
				}
#ifndef __DISABLE_HOUSEBOOST
				/* door directly on a corner? */
				if (door_y == h_y1) door_dir = 2;
				else if (door_y == h_y2) door_dir = 4;
				else door_dir = 3;
#endif
				break;
			/* Left side */
			default:
				door_y = rand_range(h_y1, h_y2);
				door_x = h_x1;
				if (has_moat) {
					drawbridge_y[0] = door_y; drawbridge_y[1] = door_y;
					drawbridge_y[2] = door_y;
					drawbridge_x[0] = h_x1-1; drawbridge_x[1] = h_x1-2;
					drawbridge_x[2] = h_x1-3;
				}
#ifndef __DISABLE_HOUSEBOOST
				/* door directly on a corner? */
				if (door_y == h_y1) door_dir = 0;
				else if (door_y == h_y2) door_dir = 6;
				else door_dir = 7;
#endif
				break;
		}

#ifdef __DISABLE_HOUSEBOOST
		num_door_attempts++;
	} while (dwelling_check_entrance(wpos, door_y, door_x) && num_door_attempts < 30);
#else
		/* Break on success */
		if (dwelling_check_entrance(wpos, door_y, door_x, door_dir)) break;

		num_door_attempts++;
	} while (num_door_attempts < 20);
#endif

	/* Build a rectangular building */
	for (y = h_y1; y <= h_y2; y++) {
		for (x = h_x1; x <= h_x2; x++) {
			/* Get the grid */
			c_ptr = &zcave[y][x];

			/* Clear previous contents, add "basic" perma-wall */
			c_ptr->feat = wall_feature;
		}
	}


	/* set default floor info; new: CAVE_ROOM to allow lighting them like rooms */
	floor_info = CAVE_ICKY | CAVE_ROOM;

	/* does house already exist (created at server start)
	   or has it just been created for the first time? */
	tmp = pick_house(wpos, door_y, door_x);

	/* is it a 'suspended' guild hall? */
	if (tmp != -1
	    && houses[tmp].dna->owner && houses[tmp].dna->owner_type == OT_GUILD
	    && !guilds[houses[tmp].dna->owner].master)
		floor_info |= CAVE_GUILD_SUS;

	/* TODO: use coloured roof, so that they look cute :) */
#ifndef USE_MANG_HOUSE_ONLY
	if (type != WILD_TOWN_HOME || !trad)
#endif	// USE_MANG_HOUSE_ONLY
	/* make it hollow */
	for (y = h_y1 + 1; y < h_y2; y++) {
		for (x = h_x1 + 1; x < h_x2; x++) {
			/* Get the grid */
			c_ptr = &zcave[y][x];

			/* Fill with floor */
			c_ptr->feat = FEAT_FLOOR;

			/* Make it "icky" (and maybe more) */
			c_ptr->info |= floor_info;
		}
	}


	/* add the door */
	c_ptr = &zcave[door_y][door_x];
	c_ptr->feat = door_feature;

#ifdef HOUSE_PAINTING
	/* Add colour if house is painted */
	if (tmp != -1) {
		house_type *h_ptr = &houses[tmp];
		cave_type *hc_ptr;
		if (h_ptr->colour) {
			for (x = door_x - 1; x <= door_x + 1; x++)
			for (y = door_y - 1; y <= door_y + 1; y++) {
				if (!in_bounds(y, x)) continue;
				hc_ptr = &zcave[y][x];
				if (hc_ptr->feat == FEAT_WALL_HOUSE ||
				    hc_ptr->feat == FEAT_HOME)
					hc_ptr->colour = h_ptr->colour;
			}
		}
	}
#endif

	/* Build the moat */
	if (has_moat) {
		/* North / South */
		for (x = h_x1-2; x <= h_x2+2; x++) {
			zcave[h_y1-2][x].feat = FEAT_DEEP_WATER; zcave[h_y1-2][x].info |= CAVE_ICKY;
			zcave[h_y1-3][x].feat = FEAT_DEEP_WATER; zcave[h_y1-3][x].info |= CAVE_ICKY;
			zcave[h_y2+2][x].feat = FEAT_DEEP_WATER; zcave[h_y2+2][x].info |= CAVE_ICKY;
			zcave[h_y2+3][x].feat = FEAT_DEEP_WATER; zcave[h_y2+3][x].info |= CAVE_ICKY;
		}
		/* East / West */
		for (y = h_y1-2; y <= h_y2+2; y++) {
			/* Get the grid */
			zcave[y][h_x1-2].feat = FEAT_DEEP_WATER; zcave[y][h_x1-2].info |= CAVE_ICKY;
			zcave[y][h_x1-3].feat = FEAT_DEEP_WATER; zcave[y][h_x1-3].info |= CAVE_ICKY;
			zcave[y][h_x2+2].feat = FEAT_DEEP_WATER; zcave[y][h_x2+2].info |= CAVE_ICKY;
			zcave[y][h_x2+3].feat = FEAT_DEEP_WATER; zcave[y][h_x2+3].info |= CAVE_ICKY;
		}
		zcave[drawbridge_y[0]][drawbridge_x[0]].feat = FEAT_DRAWBRIDGE;
		zcave[drawbridge_y[0]][drawbridge_x[0]].info |= CAVE_ICKY;
		zcave[drawbridge_y[1]][drawbridge_x[1]].feat = FEAT_DRAWBRIDGE;
		zcave[drawbridge_y[1]][drawbridge_x[1]].info |= CAVE_ICKY;
		zcave[drawbridge_y[2]][drawbridge_x[2]].feat = FEAT_DRAWBRIDGE;
		zcave[drawbridge_y[2]][drawbridge_x[2]].info |= CAVE_ICKY;
	}

	/* Hack -- finish making a town house */
	if (type == WILD_TOWN_HOME) {
		struct c_special *cs_ptr;
		/* hack -- only add a house if it is not already in memory;
		   Means: If it hasn't already been created on server startup. */
		if (tmp == -1) {
			cs_ptr = AddCS(c_ptr, CS_DNADOOR);	/* XXX this can fail? */
//			cs_ptr->type = CS_DNADOOR;
			cs_ptr->sc.ptr = houses[num_houses].dna;
			houses[num_houses].dx = door_x;
			houses[num_houses].dy = door_y;
			houses[num_houses].dna->creator = 0L;
			houses[num_houses].dna->owner = 0L;

#ifndef USE_MANG_HOUSE_ONLY
			/* This can be changed later - house capacity doesn't need
			 * to be bound to the house size any more.
			 * TODO: add 'extension' command
			 * TODO: implement 'bank'
			 */
			/* XXX maybe new owner will be unhappy if area>STORE_INVEN_MAX;
			 * this will be fixed when STORE_INVEN_MAX will be removed. - Jir
			 */
			if (trad) {
				size = (area >= STORE_INVEN_MAX) ? STORE_INVEN_MAX : area;
				houses[num_houses].stock_size = size;
				houses[num_houses].stock_num = 0;
				/* TODO: pre-allocate some when launching the server */
				C_MAKE(houses[num_houses].stock, size, object_type);
			} else {
				houses[num_houses].stock_size = 0;
				houses[num_houses].stock_num = 0;
			}
#endif	// USE_MANG_HOUSE_ONLY

			num_houses++;
			if ((house_alloc-num_houses)<32){
				GROW(houses, house_alloc, house_alloc+512, house_type);
				GROW(houses_bak, house_alloc, house_alloc+512, house_type);
				house_alloc += 512;
			}
		} else {
			cs_ptr = AddCS(c_ptr, CS_DNADOOR);
/* evileye temporary fix */
#if 1
			houses[tmp].coords.rect.width = houses[num_houses].coords.rect.width;
			houses[tmp].coords.rect.height = houses[num_houses].coords.rect.height;
#endif
/* end evileye fix */
			/* malloc madness otherwise */
			KILL(houses[num_houses].dna, struct dna_type);
//			cs_ptr->type = CS_DNADOOR;
			cs_ptr->sc.ptr = houses[tmp].dna;
		}
	}

	/* make the building interesting */
	wild_furnish_dwelling(wpos, h_x1+1,h_y1+1,h_x2-1,h_y2-1, type);

	/* Hack -- use the "complex" RNG */
	Rand_quick = rand_old;
}




/* auxillary function to determine_wilderness_type, used for terminating
   infinite loops of clones pointing at eachother.  see below. originially
   counted the length of the loop, but as virtually all loops turned out
   to be 2 in length, it was revised to find the total depth of the loop.
*/

static int wild_clone_closed_loop_total(struct worldpos *wpos)
{
	int total_depth;
	struct worldpos start, curr, neigh; //, total;
	//wilderness_type *w_ptr = &wild_info[wpos->wy][wpos->wx];
	int iter = 0;	/* hack ;( locks otherwise */

	total_depth = 0;

	/* save our initial position */
	wpcopy(&start,wpos);
	wpcopy(&curr,wpos);	/* dont damage the one we were given */

	/* until we arrive back at our initial position */
	do {
		/* seed the number generator */
		Rand_value = seed_town + (curr.wx+curr.wy*MAX_WILD_X) * 600;
		/* HACK -- the second rand after the seed is used for the beginning of the clone
		   directions (see below function).  This rand sets things up. */
		rand_int(100);

		wpcopy(&neigh, &curr);
		do {
			switch (rand_int(4)) {
				case 0:
					neigh.wx++;
					break;
				case 1:
					neigh.wy++;
					break;
				case 2:
					neigh.wx--;
					break;
				case 3:
					neigh.wy--;
			}
		} while ((neigh.wx < 0 || neigh.wy < 0 || neigh.wx >= MAX_WILD_X || neigh.wy >= MAX_WILD_Y));
		/* move to this new location */
		wpcopy(&curr, &neigh);

		/* increase our loop total depth */
		total_depth += (curr.wx + curr.wy * MAX_WILD_X);
		iter++;
	} while (!inarea(&curr, &start) && iter < 50);
	return total_depth;
}


/* figure out what kind of terrain a depth is
 * this function assumes that wild_info's world_x and world_y values
 * have been set. */

/* Hack -- Read this for an explenation of the wilderness generation.
 * Each square is seeded with a seed dependent on its depth, and this is
 * used to find its terrain type.
 * If it is of type 'clone', then a random direction is picked, and
 * it becomes the type of terrain that its neighbor is, using recursion
 * if neccecary.  This was causing problems with closed loops of clones,
 * so I came up with a mega-hack solution :
 * if we notice we are in a closed loop, find the total depth of the loop
 * by adding all its components, and use this to seed the pseudorandom
 * number generator and set the loops terrain.
 *
 * Note that a lot of this craziness is performed to keep the wilderness'
 * terrain types independent of the order in which they are explored;
 * they are completly defiend by the pseudorandom seed seed_town.
 */

int determine_wilderness_type(struct worldpos *wpos)
{
	int closed_loop = -0xFFF;
	wilderness_type *w_ptr = &wild_info[wpos->wy][wpos->wx];
	struct worldpos neighbor;

	bool rand_old = Rand_quick;
	u32b old_seed = Rand_value;

	/* Hack -- Use the "simple" RNG */
	Rand_quick = TRUE;

	/* Hack -- Induce consistant wilderness */
	Rand_value = seed_town + (wpos->wx+wpos->wy*MAX_WILD_X) * 600;

	/* check if the town */
	if (istown(wpos)) return WILD_TOWN;

	/* check if already defined */
	if ((w_ptr->type != WILD_UNDEFINED) && (w_ptr->type != WILD_CLONE)) return w_ptr->type;

	/* check for infinite loops */
	if (w_ptr->type == WILD_CLONE) {
		/* Mega-Hack -- we are in a closed loop of clones, find the length of the loop
		and use this to seed the pseudorandom number generator. */
		closed_loop = wild_clone_closed_loop_total(wpos);
		Rand_value = seed_town + closed_loop * 8973;
	}

	/* randomly determine the level type */
	/* hack -- if already a clone at this point, we are in a closed loop.  We
	terminate the loop by picking a nonclone terrain type.  Yes, this prevents
	"large" features from forming, but the resulting terrain is still rather
	pleasing.
	*/
	if ((rand_int(100) <  101) && (w_ptr->type != WILD_CLONE)) w_ptr->type = WILD_CLONE;
	else if (rand_int(100) < 1) w_ptr->type = WILD_ICE;
	else if (rand_int(100) < 1) w_ptr->type = WILD_DESERT;
	else if (rand_int(100) < 3) w_ptr->type = WILD_WASTELAND;
	else if (rand_int(100) < 5) w_ptr->type = WILD_DENSEFOREST;
	else if (rand_int(100) < 40) w_ptr->type = WILD_FOREST;
	else if (rand_int(100) < 10) w_ptr->type = WILD_SWAMP;
	else if (rand_int(100) < 15) w_ptr->type = WILD_LAKE;
	else w_ptr->type = WILD_GRASSLAND;


#ifdef DEVEL_TOWN_COMPATIBILITY
	/* hack -- grassland is likely next to the town */
	if (closed_loop > -20)
		if (rand_int(100) < 60) w_ptr->type = WILD_GRASSLAND;
#endif

	/* if a "clone", copy the terrain type from a neighbor, and recurse if neccecary. */
	if (w_ptr->type == WILD_CLONE) {
		neighbor.wx = wpos->wx;
		neighbor.wy = wpos->wy;
		neighbor.wz = wpos->wz; /* just for inarea */

		/* get a legal neighbor index */
		/* illegal locations -- the town and off the edge */

		while ((istown(&neighbor) || (neighbor.wx < 0 || neighbor.wy < 0 || neighbor.wx >= MAX_WILD_X || neighbor.wy >= MAX_WILD_Y))) {
			switch (rand_int(4)) {
				case 0:
					neighbor.wx++;
					break;
				case 1:
					neighbor.wy++;
					break;
				case 2:
					neighbor.wx--;
					break;
				case 3:
					neighbor.wy--;
			}
		}
		w_ptr->type = determine_wilderness_type(&neighbor);

#ifndef	DEVEL_TOWN_COMPATIBILITY
		if (w_ptr->radius <= 2)
			switch (w_ptr->type) {
				/* no wastelands next to town */
				case WILD_WASTELAND :
					w_ptr->type = WILD_GRASSLAND; break;
				/* no deserts next to town */
				case WILD_DESERT :
					w_ptr->type = WILD_GRASSLAND; break;
				/* no ice next to town */
				case WILD_ICE :
					w_ptr->type = WILD_GRASSLAND; break;
				/* dense forest is rarly next to town */
				case WILD_DENSEFOREST :
					if (rand_int(100) < 80) w_ptr->type = WILD_GRASSLAND;
					break;
				/* people usually don't build towns next to a swamp */
				case WILD_SWAMP :
					if (rand_int(100) < 50) w_ptr->type = WILD_GRASSLAND;
					break;
				/* forest is slightly less common near a town */
				case WILD_FOREST :
					if (rand_int(100) < 30) w_ptr->type = WILD_GRASSLAND;
					break;
			}
#endif
	}
	/* Hack -- use the "complex" RNG */
	Rand_quick = rand_old;
	/* Hack -- don't touch number generation. */
	Rand_value = old_seed;

	return w_ptr->type;
}


typedef struct terrain_type terrain_type;

struct terrain_type
{
	int type;

	int grass;
	int mud;
	int water;
	int tree;

	int deadtree;
	int mountain;
	int lava;
	int sand;

	int ice;
	int dwelling;
	int hotspot;
	int monst_lev;
};


/* determines terrain composition. seperated from gen_wilderness_aux for bleed functions.*/
static void init_terrain(terrain_type *t_ptr, int radius)
{
	/* init all at zero */
	*t_ptr = (terrain_type) {t_ptr->type,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0};

	switch (t_ptr->type) {
		/* wasteland */
		case WILD_VOLCANO:
		{
#ifndef __DISABLE_NEW
			t_ptr->grass = rand_int(10);
			t_ptr->deadtree = rand_int(4);
			t_ptr->mountain = rand_int(100)+650;
			t_ptr->lava = rand_int(150)+400;
#else
			t_ptr->grass = rand_int(100);
			t_ptr->deadtree = rand_int(4);
			t_ptr->mountain = rand_int(100)+450;
			t_ptr->lava = rand_int(150)+800;
#endif
			t_ptr->hotspot = rand_int(15) + 4;
			t_ptr->monst_lev = 20 + (radius / 2); break;
			break;
		}
		case WILD_MOUNTAIN:
		{
			t_ptr->grass = rand_int(100);
			t_ptr->tree = rand_int(5);
			t_ptr->deadtree = rand_int(4);
#ifndef __DISABLE_NEW
			t_ptr->mountain = rand_int(100)+700;
			t_ptr->sand = rand_int(1);
			t_ptr->lava = rand_int(10);
#else
			t_ptr->mountain = rand_int(100)+850;
			t_ptr->lava = rand_int(150)+200;
#endif
			t_ptr->hotspot = rand_int(15) + 4;
			t_ptr->monst_lev = 20 + (radius / 2); break;
			break;
		}
		case WILD_WASTELAND:
		{
			t_ptr->grass = rand_int(100);
#ifndef __DISABLE_NEW
			t_ptr->sand = rand_int(2);
#endif
			t_ptr->deadtree = rand_int(4);
			t_ptr->hotspot = rand_int(15) + 4;
			t_ptr->monst_lev = 20 + (radius / 2); break;
			break;
		}
		case WILD_DESERT:
		{
			t_ptr->sand = rand_int(10) + 990;
			t_ptr->mountain = rand_int(1);
			t_ptr->hotspot = rand_int(2);
			t_ptr->monst_lev = 10 + (radius / 2); break;
			break;
		}
		case WILD_ICE:
		{
			t_ptr->ice = 1000;
			t_ptr->mountain = rand_int(5);
			t_ptr->hotspot = rand_int(4) + 2;
			t_ptr->monst_lev = 20 + (radius / 2); break;
			break;
		}
		/*  dense forest */
		case WILD_DENSEFOREST:
		{
			t_ptr->grass = rand_int(100)+850;
			t_ptr->tree = rand_int(150)+600;
			t_ptr->deadtree = rand_int(10)+5;
			t_ptr->water = rand_int(15);
			t_ptr->dwelling = 8;
			t_ptr->hotspot = rand_int(15) +4;
			t_ptr->monst_lev = 15 + (radius / 2);
			/* you don't want to go into an evil forst at night */
			if (IS_NIGHT) t_ptr->monst_lev += 10;
			break;
		}
		/*  normal forest */
		case WILD_FOREST:
		{
			t_ptr->grass = rand_int(400)+500;
			t_ptr->tree = rand_int(200)+100;
			t_ptr->water = rand_int(20);
			/*t_ptr->mud = rand_int(5);*/
			t_ptr->dwelling = 37;
			t_ptr->hotspot = rand_int(rand_int(10));
			t_ptr->monst_lev = 5 + (radius / 2);
			break;
		}
		/* swamp */
		case WILD_SWAMP:
		{
			t_ptr->grass = rand_int(900);
			t_ptr->tree = rand_int(500);
			t_ptr->water = rand_int(450) + 300;
			/*t_ptr->mud = rand_int(100);*/
			t_ptr->dwelling = 8;
			t_ptr->hotspot = rand_int(15) + 4;
			t_ptr->monst_lev = 12 + (radius / 2);
			/* you really don't want to go into swamps at night */
			if (IS_NIGHT) t_ptr->monst_lev *= 2;
			break;
		}
		/* lake */
		case WILD_RIVER:
		case WILD_OCEAN:
		case WILD_LAKE:
		{
			t_ptr->grass = rand_int(900);
			t_ptr->tree = rand_int(400);
			t_ptr->water = rand_int(4) + 996;
			t_ptr->dwelling = 25;
			t_ptr->hotspot = rand_int(15) + 4;
			t_ptr->monst_lev = 1 + (radius / 2);
			break;
		}
		/* grassland / paranoia */
		default:
		{
			/* paranoia */
			t_ptr->type = WILD_GRASSLAND;

			t_ptr->grass = rand_int(200) + 850;
			t_ptr->tree = rand_int(15);
			t_ptr->water = rand_int(10);
			t_ptr->dwelling = 100;
			t_ptr->hotspot = rand_int(rand_int(6));
			t_ptr->monst_lev = 1 + (radius / 2);
			break;
		}
	}
	/* HACK -- monster levels are now negative, to support
	 * "wilderness only" monsters
	 * XXX disabling this, causing problems.
	 */
	t_ptr->monst_lev *= 1;
}

static unsigned char terrain_spot(terrain_type * terrain)
{
	unsigned char feat = FEAT_DIRT;
	u32b tmp_seed;

	if (rand_int(1000) < terrain->grass) feat = FEAT_GRASS;
#ifndef __DISABLE_NEW
	if (rand_int(1000) < terrain->sand) feat = FEAT_SAND;
	if (rand_int(1000) < terrain->ice) feat = magik(90) ? FEAT_SNOW : (magik(75) ? FEAT_ICE : FEAT_ICE_WALL);
#endif
#if 1
	if (rand_int(1000) < terrain->tree) {
		/* actually it's cool that whether it's a grown tree or a small bush
		   isn't determined by the consistent wilderness seed */
		tmp_seed = Rand_value; /* save RNG */
		Rand_value = seed_wild_extra;
		feat = get_seasonal_tree();
		seed_wild_extra = Rand_value; /* don't reset but remember */
		Rand_value = tmp_seed;
	}
#else
	if (rand_int(1000) < terrain->tree) feat = FEAT_TREE;
#endif
#ifdef __DISABLE_NEW
	if (rand_int(1000) < terrain->deadtree) feat = FEAT_DEAD_TREE;
#endif
	if (rand_int(1000) < terrain->water) feat = FEAT_DEEP_WATER;
	if (rand_int(1000) < terrain->mud) feat = FEAT_MUD;
	if (rand_int(1000) < terrain->mountain) feat = FEAT_MOUNTAIN;
	if (rand_int(1000) < terrain->lava) feat = magik(30) ? FEAT_DEEP_LAVA : FEAT_SHAL_LAVA;
#ifndef __DISABLE_NEW
	if (rand_int(1000) < terrain->deadtree) feat = FEAT_DEAD_TREE;
#endif

	return(feat);
}


/* adds an island in a lake, or a clearing in a forest, or a glade in a plain.
   done to make the levels a bit more interesting.

   chopiness defines the randomness of the circular shape.

   */
/* XXX -- I should make this use the new terrain structure, and terrain_spot. */

static void wild_add_hotspot(struct worldpos *wpos)
{
	int x_cen,y_cen, max_mag, magnitude, magsqr, chopiness, x, y;
	terrain_type hot_terrain;
	bool add_dwelling = FALSE;
	wilderness_type *w_ptr = &wild_info[wpos->wy][wpos->wx];
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	magnitude = 0;
	/* set the terrain features to 0 by default */
	memset(&hot_terrain,0,sizeof(terrain_type));

	/* hack -- minimum hotspot radius of 3 */
	while (magnitude < 3) {
		/* determine the rough "coordinates" of the feature */
		x_cen = rand_int(MAX_WID-11) + 5;
		y_cen = rand_int(MAX_HGT-11) + 5;

		/* determine the maximum size of the feature, which is its distance to
		its closest edge.
		*/
		max_mag = y_cen;
		if (x_cen < max_mag) max_mag = x_cen;
		if ((MAX_HGT - y_cen) < max_mag) max_mag = MAX_HGT - y_cen;
		if ((MAX_WID - x_cen) < max_mag) max_mag = MAX_WID - x_cen;

		/* determine the magnitude of the feature.  the triple rand is done to
		keep most features small, but have a rare large one. */

		magnitude = rand_int(rand_int(rand_int(max_mag)));
	}

	/* hack -- take the square to avoid square roots */
	magsqr = magnitude * magnitude;

	/* the "roughness" of the hotspot */
	chopiness = 2 * magsqr / (rand_int(5) + 1);

	/* for each point in the square enclosing the circle
	   this algorithm could probably use some optimization
	*/
	switch (w_ptr->type) {
		case WILD_GRASSLAND:
			/* sometimes a pond */
			if (rand_int(100) < 50) {
				hot_terrain.water = 1000;
			}
			/* otherwise a glade */
			else {
				hot_terrain.grass = rand_int(200) + 850;
				hot_terrain.tree = rand_int(600) + 300;
			}
			break;
		case WILD_FOREST:
			/* sometimes a pond */
			if (rand_int(100) < 60) {
				hot_terrain.water = 1000;
			}
			/* otherwise a clearing */
			else {
				hot_terrain.grass = rand_int(150)+900;
				hot_terrain.tree = rand_int(6)-3;

				/* if a large clearing, maybe a house */
				if (magnitude > 8) {
					if (rand_int(100) < 25) add_dwelling = TRUE;
				}
			}
			break;
		case WILD_DENSEFOREST:
			/* 80% chance of being nothing */
			if (rand_int(100) < 80) {
			}
			/* otherwise 70% chance of being a pond */
			else if (rand_int(100) < 70) {
				hot_terrain.water = 1000;
			}
			/* otherwise a rare clearing */
			else {
				hot_terrain.tree = rand_int(30)+7;

				/* sometimes a dwelling. wood-elves? */
				if (magnitude > 8) {
					if (rand_int(100) < 50) add_dwelling = TRUE;
				}
			}
			break;
		case WILD_SWAMP:
			/* sometimes a pond */
			if (rand_int(100) < 40) {
				hot_terrain.water = 1000;
			}
			/* otherwise a mud pit */
			else {
				hot_terrain.type = WILD_SWAMP;
				init_terrain(&hot_terrain, w_ptr->radius);
				hot_terrain.mud = rand_int(150) + 700;
			}
			break;

		case WILD_DESERT:
			/* sometimes an oasis */
			if (rand_int(100) < 80) {
				hot_terrain.tree = 1000;
				hot_terrain.water = rand_int(100) + 900;
			}
			/* otherwise some dried out trees */
			else {
#if 1 /* prevent FEAT_DIRT base? */
				hot_terrain.sand = 1000;
#endif
				hot_terrain.deadtree = rand_int(200) + 200;
			}
			break;

		case WILD_ICE:
			/* sometimes a pond */
			if (rand_int(100) < 30) {
				hot_terrain.water = 1000;
			}
			/* otherwise some dead trees and mountains */
			else {
				hot_terrain.deadtree = rand_int(100) + 150;
				hot_terrain.mountain = rand_int(300) + 250;
				hot_terrain.ice = 1000;
			}
			break;

		case WILD_LAKE:
			/* island */
			hot_terrain.type = WILD_GRASSLAND;
			init_terrain(&hot_terrain, w_ptr->radius);
			break;

		default: hot_terrain.deadtree = rand_int(800)+100;
	}

#if 0
	/* hack for ambient sound fx -- crickets at every little pond :-p */
	if (hot_terrain.water == 1000) {
		/* maybe add SFX_AMBIENT_LAKE, if not overriden by SFX_AMBIENT_SHORE */
		if (wild_info[wpos->wy][wpos->wx].bled != WILD_OCEAN)
			wild_info[wpos->wy][wpos->wx].bled = WILD_LAKE;
	}
#endif

	/* create the hotspot */
	for (y = y_cen - magnitude; y <= y_cen + magnitude; y++) {
		for (x = x_cen - magnitude; x <= x_cen + magnitude; x++) {
			/* a^2 + b^2 = c^2... the rand makes the edge less defined */
			/* HACK -- multiply the y's by 4 to "squash" the shape */
			if (((x - x_cen) * (x - x_cen)) + (((y - y_cen) * (y - y_cen))*4) < magsqr + rand_int(chopiness)) {
				zcave[y][x].feat = terrain_spot(&hot_terrain);
			}
		}
	}

	/* add inhabitants */
	if (add_dwelling) wild_add_dwelling(wpos, x_cen, y_cen );
}

#ifdef BLEED_WITH_NEIGHBOURS
/* helper function to wild_gen_bleedmap */
static void wild_gen_bleedmap_aux(int *bleedmap, int span, char dir)
{
	int c = 0, above, below, noise_mag, rand_noise, bleedmag;

	/* make a pass of the bleedmap */
	while (c < MAX_WID) {
		/* check that its clear */
		if (bleedmap[c] == 0xFFFF) {
			/* if these are aligned right, they shouldn't overflow */
			if (bleedmap[c - span] != 0xFFFF) above = bleedmap[c - span];
			else above = 0;
			if (bleedmap[c + span] != 0xFFFF) below = bleedmap[c + span];
			else below = 0;

			noise_mag = (dir%2) ? 70 : 25;
			/* randomness proportional to span */
			rand_noise = ((rand_int(noise_mag*2) - noise_mag) * span)/64;
			bleedmag = ((above + below) / 2) + rand_noise;

			/* bounds checking */
			if (bleedmag < 0) bleedmag = 0;
			if (bleedmag > (MAX_HGT-1)/2) bleedmag = (MAX_HGT-1)/2;

			/* set the bleed magnitude */
			bleedmap[c] = bleedmag;
		}

		c += span;
	}

	span /= 2;
	/* do the next level of recursion */
	if (span) wild_gen_bleedmap_aux(bleedmap, span, dir);

}

/* using a simple fractal algorithm, generates the bleedmap used by the function below. */
/* hack -- for this algorithm to work nicely, an initial span of a power of 2 is required. */
static void wild_gen_bleedmap(int *bleedmap, char dir, int start, int end)
{
	int c = 0, bound;

	/* initialize the bleedmap */
	for (c = 0; c <= 256; c++) {
		bleedmap[c] = 0xFFFF;
	}

	/* initialize the "top" and "bottom" */
	if (start < 0) bleedmap[0] = rand_int(((dir%2) ? 70 : 25));
	else bleedmap[0] = start;
	if (end < 0) bleedmap[256] = rand_int(((dir%2) ? 70 : 25));
	else {
		bound = (dir%2) ? MAX_HGT-3 : MAX_WID-3;
		for (c = bound; c <= 256; c++) bleedmap[c] = end;
	}

	/* hack -- if the start and end are zeroed, add something in the middle
	   to make exciting stuff happen. */
	if ((!start) && (!end)) {
		/* east or west */
		if (dir%2) bleedmap[32] = rand_int(40) + 15;
		/* north or south */
		else {
			bleedmap[64] = rand_int(20) + 8;
			bleedmap[128] = rand_int(20) + 8;
		}
	}

	/* generate the bleedmap */
	wild_gen_bleedmap_aux(bleedmap, 256/2, dir);

	/* hack -- no bleedmags less than 8 except near the edges */
	bound = (dir%2) ? MAX_HGT-1 : MAX_WID-1;

	/* beginning to middle */
	for (c = 0; c < 8; c++) if (bleedmap[c] < c) bleedmap[c] = c;
	/* middle */
	for (c = 8; c < bound - 8; c++) {
		if (bleedmap[c] < 8) bleedmap[c] = rand_int(3) + 8;
	}
	/* middle to end */
	for (c = bound - 8; c < bound; c++) {
		if (bleedmap[c] < bound - c) bleedmap[c] = bound - c;
	}

}

/* this function "bleeds" the terrain type of bleed_from to the side of bleed_to
   specified by dir.

   First, a bleedmap array is initialized using a simple fractal algorithm.
   This map specifies the magnitude of the bleed at each point along the edge.
   After this, the two structures bleed_begin and bleed_end are initialized.

   After this structure is initialized, for each point along the bleed edge,
   up until the bleedmap[point] edge of the bleed, the terrain is set to
   that of bleed_from.

   We should hack this to add interesting features near the bleed edge.
   Such as ponds near shoreline to make it more interesting and
   groves of trees near the edges of forest.
*/

static void wild_bleed_level(int bleed_to_x, int bleed_to_y, int bleed_from_x, int bleed_from_y, char dir, int start, int end)
{
	int x, y;
	int bleedmap[256 + 1], bleed_begin[MAX_WID], bleed_end[MAX_WID];
	terrain_type terrain;
	cave_type *c_ptr, **zcave_bleed_to = getcave(&((struct worldpos) {bleed_to_x, bleed_to_y, 0}));
#ifdef BLEED_ENHANCED_TOWN
	int type = -1, i;
#endif

#ifdef BLEED_AVOID_TOWN
//	if (istownarea(&neighbor, 1)) return;
	if (istown(&((struct worldpos) {bleed_from_x, bleed_from_y, 0}))) return;
#endif
#ifdef BLEED_ENHANCED_TOWN
	if (istown(&((struct worldpos) {bleed_from_x, bleed_from_y, 0}))) {
		for (i = 0; i < numtowns; i++) {
			if (town[i].x == bleed_from_x && town[i].y == bleed_from_y) {
				type = town[i].type;
				break;
			}
		}
		switch (type) {
		case TOWN_BREE:
			wild_info[bleed_from_y][bleed_from_x].type = WILD_GRASSLAND;
			//if (wild_info[bleed_to_y][bleed_to_x].type != WILD_OCEAN) return;
			break;
		case TOWN_GONDOLIN:
			wild_info[bleed_from_y][bleed_from_x].type = WILD_GRASSLAND;
			//if (wild_info[bleed_to_y][bleed_to_x].type != WILD_OCEAN) return;
			break;
		case TOWN_MINAS_ANOR:
			wild_info[bleed_from_y][bleed_from_x].type = WILD_MOUNTAIN;
			//if (wild_info[bleed_to_y][bleed_to_x].type != WILD_OCEAN) return;
			break;
		case TOWN_LOTHLORIEN:
			wild_info[bleed_from_y][bleed_from_x].type = WILD_OCEAN;
			//if (wild_info[bleed_to_y][bleed_to_x].type != WILD_OCEAN) return;
			break;
		case TOWN_KHAZADDUM:
			wild_info[bleed_from_y][bleed_from_x].type = WILD_VOLCANO;
			//if (wild_info[bleed_to_y][bleed_to_x].type != WILD_OCEAN) return;
			break;
		}
	}
#endif

	/* paranoia */
	if (!zcave_bleed_to) {
		s_printf("getcave() failed in wild_bleed_level\n");
#ifdef BLEED_ENHANCED_TOWN
		if (type != -1) wild_info[bleed_from_y][bleed_from_x].type = WILD_TOWN;
#endif
		return;
	}

	/* sanity check */
	if (wild_info[bleed_from_y][bleed_from_x].type == wild_info[bleed_to_y][bleed_to_x].type) {
#ifdef BLEED_ENHANCED_TOWN
		if (type != -1) wild_info[bleed_from_y][bleed_from_x].type = WILD_TOWN;
#endif
		return;
	}

	/* initiliaze the terrain type */
	terrain.type = wild_info[bleed_from_y][bleed_from_x].type;

	/* determine the terrain components */
	init_terrain(&terrain, -1);

	/* generate the bleedmap */
	wild_gen_bleedmap(bleedmap, dir, start, end);

	/* initialize the bleedruns */
	switch (dir) {
		case DIR_EAST:
			for (y = 1; y < MAX_HGT - 1; y++) {
				bleed_begin[y] = MAX_WID - bleedmap[y];
				bleed_end[y] = MAX_WID - 1;
			}
			break;
		case DIR_WEST:
			for (y = 1; y < MAX_HGT - 1; y++) {
				bleed_begin[y] = 1;
				bleed_end[y] = bleedmap[y];
			}
			break;
		case DIR_NORTH:
			for (x = 1; x < MAX_WID - 1; x++) {
				bleed_begin[x] = 1;
				bleed_end[x] = bleedmap[x];
			}
			break;
		case DIR_SOUTH:
			for (x = 1; x < MAX_WID - 1; x++) {
				bleed_begin[x] = MAX_HGT - bleedmap[x];
				bleed_end[x] = MAX_HGT - 1;
			}
			break;
	}

	if ((dir == DIR_EAST) || (dir == DIR_WEST)) {
		for (y = 1; y < MAX_HGT - 1; y++) {
			for (x = bleed_begin[y]; x < bleed_end[y]; x++) {
				c_ptr = &zcave_bleed_to[y][x];
				c_ptr->feat = terrain_spot(&terrain);
			}
		}
	} else {
		for (x = 1; x < MAX_WID - 1; x++) {
			for (y = bleed_begin[x]; y < bleed_end[x]; y++) {
				c_ptr = &zcave_bleed_to[y][x];
				c_ptr->feat = terrain_spot(&terrain);
			}
		}
	}

#ifdef BLEED_ENHANCED_TOWN
	if (type != -1) wild_info[bleed_from_y][bleed_from_x].type = WILD_TOWN;
#endif


	/* USE_SOUND_2010 - hack for ambient sound fx (sea waves):   - C. Blue
	   the 'to' wilderness sector gets a marker so that it will trigger the
	   ambient sfx too, that the terrain type of the 'from' wilderness
	   sector would trigger.
	   idea: WILD_COAST should trigger the WILD_OCEAN ambient sfx, if there
	   are actually some visible ocean feat grids bled onto it. */

	/* need to use a priority list, if several different ambient-sfx-
	   causing terrain types are bled into this sector: */
	switch (wild_info[bleed_from_y][bleed_from_x].type) {
	case WILD_OCEAN: //sea waves
		wild_info[bleed_to_y][bleed_to_x].bled = WILD_OCEAN;
		break;
	case WILD_RIVER: case WILD_LAKE: case WILD_SWAMP: //crickets^^
		if (wild_info[bleed_to_y][bleed_to_x].bled != WILD_OCEAN)
			wild_info[bleed_to_y][bleed_to_x].bled = WILD_LAKE;
		break;
#if 0
	case WILD_VOLCANO: //dunno oO
		if (wild_info[bleed_to_y][bleed_to_x].bled != WILD_OCEAN &&
		    wild_info[bleed_to_y][bleed_to_x].bled != WILD_LAKE)
			wild_info[bleed_to_y][bleed_to_x].bled = WILD_VOLCANO;
		break;
#endif
	}
}

/*
 * Old version of getlevel() from cave.c
 * Used for seeding the wilderness bleeding effects
 * Do not change this or house layout may change in some places
 */
static int getlevel_wild_old(struct worldpos *wpos) {
	wilderness_type *w_ptr = &wild_info[wpos->wy][wpos->wx];
	return (w_ptr->radius);
}

/* determines whether or not to bleed from a given depth in a given direction.
   useful for initial determination, as well as shared bleed points.
*/
static bool should_we_bleed(struct worldpos *wpos, char dir)
{
	int neigh_x = 0, neigh_y = 0, tmp;
	wilderness_type *w_ptr = &wild_info[wpos->wy][wpos->wx];
	struct worldpos neighbor;

	/* get our neighbors index */
	switch (dir) {
	case DIR_NORTH:
		neigh_x = wpos->wx;
		neigh_y = wpos->wy + 1;
		break;
	case DIR_EAST:
		neigh_x = wpos->wx + 1;
		neigh_y = wpos->wy;
		break;
	case DIR_SOUTH:
		neigh_x = wpos->wx;
		neigh_y = wpos->wy - 1;
		break;
	case DIR_WEST:
		neigh_x = wpos->wx - 1;
		neigh_y = wpos->wy;
		break;
	}

	/* determine whether to bleed or not */
	/* if a valid location */
	if (!in_bounds_wild(neigh_y, neigh_x)) return FALSE;

	/* make sure the level type is defined */
	neighbor.wx = neigh_x;
	neighbor.wy = neigh_y;
	neighbor.wz = 0;
	wild_info[neigh_y][neigh_x].type = determine_wilderness_type(&neighbor);

	/* check if our neighbor is of a different type */
	if (w_ptr->type != wild_info[neigh_y][neigh_x].type) {
		/* determine whether to bleed or not */
		Rand_value = seed_town + (getlevel_wild_old(wpos) + getlevel_wild_old(&neighbor)) * (93754);
		tmp = rand_int(2);

		if (tmp && (getlevel(wpos) < getlevel(&neighbor))) return TRUE;
		else if (!tmp && (getlevel(wpos) > getlevel(&neighbor))) return TRUE;
		else return FALSE;
	}

	return(FALSE);
}

/* to determine whether we bleed into our neighbor or whether our neighbor
   bleeds into us, we seed the random number generator with our combined
   depth.  If the resulting number is 0, we bleed into the greater (negative
   wise) level.  Other wise we bleed into the lesser (negative wise) level.

   I added in shared points.... turning this function into something extremly
   gross. This will be extremly anoying to get working. I wish I had a simpler
   way of doing this.
*/
static void bleed_with_neighbors(struct worldpos *wpos)
{
	int c, d, tmp, side[2], start, end, opposite;
	bool do_bleed[4], bleed_zero[4];
	int share_point[4][2];
	int old_seed = Rand_value;
	bool rand_old = Rand_quick;
	wilderness_type *w_ptr = &wild_info[wpos->wy][wpos->wx];
	struct worldpos neighbor, neighbor_tmp;
	neighbor.wz = neighbor_tmp.wz = 0;

	/* Hack -- Use the "simple" RNG */
	Rand_quick = TRUE;

	/* for each neighbor, determine whether to bleed or not */
	for (c = 0; c < 4; c++) do_bleed[c] = should_we_bleed(wpos ,c);

	/* calculate the bleed_zero values */
	for (c = 0; c < 4; c++) {
		tmp = c - 1; if (tmp < 0) tmp = 3;

		/* get our neighbors index */
		switch (c) {
		case DIR_NORTH:
			neighbor.wx = wpos->wx;
			neighbor.wy = wpos->wy + 1;
			neighbor_tmp.wx = wpos->wx - 1;
			neighbor_tmp.wy = wpos->wy;
			break;
		case DIR_EAST:
			neighbor.wx = wpos->wx + 1;
			neighbor.wy = wpos->wy;
			neighbor_tmp.wx = wpos->wx;
			neighbor_tmp.wy = wpos->wy + 1;
			break;
		case DIR_SOUTH:
			neighbor.wx = wpos->wx;
			neighbor.wy = wpos->wy - 1;
			neighbor_tmp.wx = wpos->wx + 1;
			neighbor_tmp.wy = wpos->wy;
			break;
		case DIR_WEST:
			neighbor.wx = wpos->wx - 1;
			neighbor.wy = wpos->wy;
			neighbor_tmp.wx = wpos->wx;
			neighbor_tmp.wy = wpos->wy - 1;
			break;
		}

		if (in_bounds_wild(neighbor.wy, neighbor.wx) && in_bounds_wild(neighbor_tmp.wy, neighbor_tmp.wx)) {
			if (wild_info[neighbor_tmp.wy][neighbor_tmp.wx].type == wild_info[neighbor.wy][neighbor.wx].type) {
				/* calculate special case bleed zero values. */

				if (do_bleed[c]) {
					/* if get the opposite direction from tmp */
					opposite = tmp - 2; if (opposite < 0) opposite += 4;

					/* if the other one is bleeding towards us */
					if (should_we_bleed(&neighbor_tmp, opposite)) bleed_zero[c] = TRUE;
					else bleed_zero[c] = FALSE;
				} else if (do_bleed[tmp]) {
					/* get the opposite direction from c */
					opposite = c - 2; if (opposite < 0) opposite += 4;

					/* if the other one is bleeding towards us */
					if (should_we_bleed(&neighbor, opposite)) bleed_zero[c] = TRUE;
					else bleed_zero[c] = FALSE;
				}

				else bleed_zero[c] = FALSE;
			}
			else bleed_zero[c] = TRUE;
		}
		else bleed_zero[c] = FALSE;
	}


	/* calculate bleed shared points */
	for (c = 0; c < 4; c++) {
		side[0] = c - 1; if (side[0] < 0) side[0] = 3;
		side[1] = c + 1; if (side[1] > 3) side[1] = 0;

		/* if this direction is bleeding */
		if (do_bleed[c]) {
			/* for the left and right sides */
			for (d = 0; d <= 1; d++) {
				/* get our neighbors index */
				switch (side[d]) {
				case DIR_NORTH:
					neighbor.wx = wpos->wx;
					neighbor.wy = wpos->wy + 1;
					break;
				case DIR_EAST:
					neighbor.wx = wpos->wx + 1;
					neighbor.wy = wpos->wy;
					break;
				case DIR_SOUTH:
					neighbor.wx = wpos->wx;
					neighbor.wy = wpos->wy - 1;
					break;
				case DIR_WEST:
					neighbor.wx = wpos->wx - 1;
					neighbor.wy = wpos->wy;
					break;
				}

				/* if we have a valid neighbor */
				if (in_bounds_wild(neighbor.wy, neighbor.wx)) {
					/* if our neighbor is bleeding in a simmilar way */
					if (should_we_bleed(&neighbor, c)) {
						/* are we a simmilar type of terrain */
						if (wild_info[neighbor.wy][neighbor.wx].type == w_ptr->type) {
							/* share a point */
							/* seed the number generator */
							Rand_value = seed_town + (getlevel_wild_old(wpos) + getlevel_wild_old(&neighbor)) * (89791);
							share_point[c][d] = rand_int(((c%2) ? 70 : 25));
						}
						else share_point[c][d] = 0;
					}
					else share_point[c][d] = 0;
				}
				else share_point[c][d] = 0;
			}
		} else {
			share_point[c][0] = 0;
			share_point[c][1] = 0;
		}
	}

	/* do the bleeds */
	for (c = 0; c < 4; c++) {
		tmp = c+1; if (tmp > 3) tmp = 0;
		if (do_bleed[c]) {
			if ((!share_point[c][0]) && (!bleed_zero[c])) start = -1;
			else if (share_point[c][0]) start = share_point[c][0];
			else start = 0;

			if ((!share_point[c][1]) && (!bleed_zero[tmp])) end = -1;
			else if (share_point[c][1]) end = share_point[c][1];
			else end = 0;

			/* get our neighbors index */
			switch (c) {
			case DIR_NORTH:
				neighbor.wx = wpos->wx;
				neighbor.wy = wpos->wy + 1;
				neighbor_tmp.wx = wpos->wx - 1;
				neighbor_tmp.wy = wpos->wy;
				break;
			case DIR_EAST:
				neighbor.wx = wpos->wx + 1;
				neighbor.wy = wpos->wy;
				neighbor_tmp.wx = wpos->wx;
				neighbor_tmp.wy = wpos->wy + 1;
				break;
			case DIR_SOUTH:
				neighbor.wx = wpos->wx;
				neighbor.wy = wpos->wy - 1;
				neighbor_tmp.wx = wpos->wx + 1;
				neighbor_tmp.wy = wpos->wy;
				break;
			case DIR_WEST:
				neighbor.wx = wpos->wx - 1;
				neighbor.wy = wpos->wy;
				neighbor_tmp.wx = wpos->wx;
				neighbor_tmp.wy = wpos->wy - 1;
				break;
			}

			if (c < 2) {
				wild_bleed_level(wpos->wx, wpos->wy, neighbor.wx, neighbor.wy, c, start, end);
			} else {
				wild_bleed_level(wpos->wx, wpos->wy, neighbor.wx, neighbor.wy, c, end, start);
			}
		}
	}

	/* hack -- restore the random number generator */
	Rand_value = old_seed;
	Rand_quick = rand_old;
}
#endif

static void flood(char *buf, int x, int y, int w, int h) {
	if (x >= 0 && x < w && y >= 0 && y < h && buf[x + y * w] == 0) {
		buf[x + y * w] = 6;
		flood(buf, x + 1, y, w, h);
		flood(buf, x - 1, y, w, h);
		flood(buf, x, y + 1, w, h);
		flood(buf, x, y - 1, w, h);
	}
}

bool fill_house(house_type *h_ptr, int func, void *data) {
	/* polygonal house */
	/* draw all the outer walls cleanly */
	cptr coord = h_ptr->coords.poly;
	cptr ptr = coord;
	char *matrix;
	int sx = h_ptr->x;
	int sy = h_ptr->y;
	int dx, dy;
	int x, y;
	int minx, miny, maxx, maxy;
	int mw, mh;
	bool success = TRUE;
	struct worldpos *wpos = &h_ptr->wpos;
	cave_type *c_ptr;
	struct c_special *cs_ptr;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return(FALSE);

	if (func == FILL_PLAYER || func == FILL_OBJECT)
		success = FALSE;

	if (h_ptr->flags & HF_RECT) {
		for (x = 0; x < h_ptr->coords.rect.width; x++) {
			for (y = 0; y < h_ptr->coords.rect.height; y++) {
 				c_ptr = &zcave[h_ptr->y + y][h_ptr->x + x];
				if (func == FILL_GUILD) {
					return(FALSE);	/* for now */
					if (((struct guildsave*)data)->mode) {
						fputc(c_ptr->feat, ((struct guildsave*)data)->fp);
					} else {
						c_ptr->feat = fgetc(((struct guildsave*)data)->fp);
//						if(c_ptr->feat>FEAT_INVIS)
						if(!cave_plain_floor_grid(c_ptr))
							c_ptr->info &= ~(CAVE_ROOM);
					}
					everyone_lite_spot(&h_ptr->wpos, h_ptr->y + y, h_ptr->x + x);
				}
				else if (func == FILL_OBJECT) { /* object in house? */
					object_type *o_ptr = (object_type*)data;
					if (o_ptr->ix == h_ptr->x + x && o_ptr->iy == h_ptr->y + y) {
						success = TRUE;
						break;
					}
				}
				else if (func == FILL_PLAYER) { /* player in house? */
					player_type *p_ptr = (player_type*)data;
					if (p_ptr->px == h_ptr->x + x && p_ptr->py == h_ptr->y + y) {
						success = TRUE;
						break;
					}
				}
				else if (func == FILL_MAKEHOUSE) {
					if (x && y && x < h_ptr->coords.rect.width - 1 && y < h_ptr->coords.rect.height - 1) {
						if((pick_house(wpos, h_ptr->y, h_ptr->x)) != -1)
							success = FALSE;
						c_ptr->info &= ~(CAVE_ICKY | CAVE_ROOM | CAVE_STCK | CAVE_JAIL);
					} else {
						/* fix: also erase cs-special, or server will panic on restart
						   if someone kept this area static and has the former door grid
						   in his los -> will try to call access_door_colour() on it,
						   but the dna structure has already been nulled -> segfault. - C. Blue */
						if (c_ptr->feat == FEAT_HOME || c_ptr->feat == FEAT_HOME_OPEN) {
							cs_ptr = GetCS(c_ptr, CS_DNADOOR);
							if (cs_ptr) cs_erase(c_ptr, cs_ptr);
						}
						c_ptr->feat = FEAT_DIRT;
						c_ptr->info &= ~(CAVE_ICKY | CAVE_ROOM | CAVE_STCK | CAVE_JAIL);
					}
					everyone_lite_spot(&h_ptr->wpos, h_ptr->y + y, h_ptr->x + x);
				}
				else if (func == FILL_CLEAR) {
					delete_object(wpos, y, x, TRUE);
					everyone_lite_spot(&h_ptr->wpos, h_ptr->y + y, h_ptr->x + x);
				}
				else if (func == FILL_BUILD) {
					if (x && y && x < h_ptr->coords.rect.width - 1 && y < h_ptr->coords.rect.height - 1) {
 						if (!(h_ptr->flags & HF_NOFLOOR))
							c_ptr->feat = FEAT_FLOOR;
						if (h_ptr->flags & HF_JAIL) {
							c_ptr->info |= (CAVE_STCK | CAVE_JAIL);
						}
 						c_ptr->info |= (CAVE_ICKY | CAVE_ROOM);

#if 0 //moved to day->night change		//note: below hack is overridden by night atm :/ todo:fix
 						/* hack: lit owned houses for easier overview - only inside, not walls! */
//done above already				if (x > 0 && x < h_ptr->coords.rect.width - 1 && y > 0 && y < h_ptr->coords.rect.height)
 						if (h_ptr->dna->owner) c_ptr->info |= CAVE_GLOW;
#endif

						/* 'suspended' guild houses ( = of leaderless guilds) */
						if ((h_ptr->flags & HF_GUILD_SUS)) c_ptr->info |= CAVE_GUILD_SUS;

						everyone_lite_spot(&h_ptr->wpos, h_ptr->y + y, h_ptr->x + x);
					}
				}
#ifdef HOUSE_PAINTING
				else if (func == FILL_UNPAINT) {
					c_ptr->colour = 0;
					/* refresh player's view on the freshly applied paint */
					everyone_lite_spot(&h_ptr->wpos, h_ptr->y + y, h_ptr->x + x);
				}
#endif
				else if (func == FILL_GUILD_SUS) {
					if (x && y && x < h_ptr->coords.rect.width - 1 && y < h_ptr->coords.rect.height - 1) //obsolete check
 						c_ptr->info |= CAVE_GUILD_SUS;
				}
				else if (func == FILL_GUILD_SUS_UNDO) {
					if (x && y && x < h_ptr->coords.rect.width - 1 && y < h_ptr->coords.rect.height - 1) //obsolete check
 						c_ptr->info &= ~CAVE_GUILD_SUS;
				}
				else if (func == FILL_SFX_KNOCK) {
					if (c_ptr->m_idx < 0) {
						sound(-(c_ptr->m_idx), (h_ptr->flags & HF_MOAT) ? "knock_castle" : "knock", "block_shield_projectile", SFX_TYPE_COMMAND, FALSE);
						msg_print(-(c_ptr->m_idx), "\377sYou hear someone knocking on the house door.");
					}
				}
				else s_printf("rect fill house (func: %d\n", func);
			}
		}
		return(success);
	}

	maxx = minx = h_ptr->x;
	maxy = miny = h_ptr->y;
	x = h_ptr->x;
	y = h_ptr->y;

	while (ptr[0] || ptr[1]) {
		x += ptr[0];
		y += ptr[1];
		minx = MIN(x, minx);
		miny = MIN(y, miny);
		maxx = MAX(x, maxx);
		maxy = MAX(y, maxy);
		ptr += 2;
	}
	mw = maxx + 3 - minx;
	mh = maxy + 3 - miny;
	C_MAKE(matrix, mw * mh, char);
	ptr = coord;

	while (ptr[0] || ptr[1]) {
		dx = ptr[0];
		dy = ptr[1];
		if (dx) {		/* dx/dy mutually exclusive */
			if (dx < 0) {
				for (x = sx; x > (sx + dx); x--) {
					matrix[(x + 1 - minx) + (y + 1 - miny) * mw] = 1;
				}
			} else {
				for (x = sx; x < (sx + dx); x++) {
					matrix[(x + 1 - minx) + (y + 1 - miny) * mw] = 1;
				}
			}
			sx = x;
		} else {
			if (dy < 0) {
				for (y = sy; y > (sy + dy); y--) {
					matrix[(x + 1 - minx) + (y + 1 - miny) * mw] = 1;
				}
			} else {
				for (y = sy; y < (sy + dy); y++) {
					matrix[(x + 1 - minx) + (y + 1 - miny) * mw] = 1;
				}
			}
			sy = y;
		}
		ptr += 2;
	}

	flood(matrix, 0, 0, mw, mh);
	for (y = 0; y < mh; y++) {
		for (x = 0; x < mw; x++) {
			switch (matrix[x + y * mw]) {
				case 2:	/* do nothing */
				case 4:
				case 6: /* outside of walls */
					break;
				case 0:	/* inside of walls */
					if (func == FILL_GUILD) {
						struct key_type *key;
						u16b id;
						FILE *gfp = ((struct guildsave*)data)->fp;
						c_ptr = &zcave[miny + (y - 1)][minx + (x - 1)];
						if (((struct guildsave*)data)->mode) {
							fputc(c_ptr->feat, gfp);
							if (c_ptr->feat == FEAT_HOME || c_ptr->feat == FEAT_HOME_OPEN) {
								id = 0;
								if ((cs_ptr = GetCS(c_ptr, CS_KEYDOOR)) && (key = cs_ptr->sc.ptr)) {
									fseek(gfp, -1, SEEK_CUR);
									fputc(FEAT_HOME_HEAD, gfp);
									id = key->id;
									fputc((id >> 8), gfp);
									fputc(id & 0xff, gfp);
								}
							}
						}
						else{
							c_ptr->feat = fgetc(((struct guildsave*)data)->fp);
//							if (c_ptr->feat > FEAT_INVIS)
							if (!cave_plain_floor_grid(c_ptr))
								c_ptr->info &= ~(CAVE_ROOM);
							if (c_ptr->feat == FEAT_HOME){
								id = (fgetc(gfp) << 8);
								id |= fgetc(gfp);
								/* XXX it's double check */
								if (!(cs_ptr = GetCS(c_ptr, CS_KEYDOOR))) {	/* no, not really. - evileye */
									cs_ptr = AddCS(c_ptr, CS_KEYDOOR);
									MAKE(cs_ptr->sc.ptr, struct key_type);
//									cs_ptr->type = CS_KEYDOOR;
								}
								key = cs_ptr->sc.ptr;	/* isn't it dangerous? -Jir */
								key->id = id;
							}
						}
						everyone_lite_spot(&h_ptr->wpos, miny + (y - 1), minx + (x - 1));
						break;
					}
					if (func == FILL_PLAYER) {
						player_type *p_ptr = (player_type*)data;
						if (p_ptr->px == minx + (x - 1) && p_ptr->py == miny + (y - 1)) {
							success = TRUE;
						}
						break;
					}
					if (func == FILL_MAKEHOUSE) {
						if ((pick_house(wpos,miny + (y - 1), minx + (x - 1)) != -1)) {
							success = FALSE;
						}
						zcave[miny + (y - 1)][minx + (x - 1)].info &= ~(CAVE_ICKY | CAVE_ROOM | CAVE_STCK | CAVE_JAIL);
						everyone_lite_spot(&h_ptr->wpos, miny + (y - 1), minx + (x - 1));
						break;
					}
					if (func == FILL_OBJECT) { /* object in house */
						object_type *o_ptr = (object_type*)data;
						if (o_ptr->ix == minx + (x - 1) && o_ptr->iy == miny + (y - 1)) {
							success = TRUE;
						}
						break;
					}
					if (func == FILL_CLEAR) {
						delete_object(wpos, miny + (y - 1), minx + (x - 1), TRUE);
						everyone_lite_spot(&h_ptr->wpos, miny + (y - 1), minx + (x - 1));
						break;
					}
					if (func == FILL_BUILD) {
						c_ptr = &zcave[miny + (y - 1)][minx + (x - 1)];
						if (!(h_ptr->flags & HF_NOFLOOR)) {
							c_ptr->feat = FEAT_FLOOR;
							c_ptr->info |= (CAVE_ROOM | CAVE_ICKY);
						}
						if (h_ptr->flags & HF_JAIL) {
							c_ptr->info |= (CAVE_STCK | CAVE_JAIL);
						}

						/* 'suspended' guild houses ( = of leaderless guilds) */
						if ((h_ptr->flags & HF_GUILD_SUS)) c_ptr->info |= CAVE_GUILD_SUS;

						everyone_lite_spot(&h_ptr->wpos, miny + (y - 1), minx + (x - 1));
						break;
					}
#ifdef HOUSE_PAINTING
					else if (func == FILL_UNPAINT) {
						zcave[miny + (y - 1)][minx + (x - 1)].colour = 0;
						/* refresh player's view on the freshly applied paint */
						everyone_lite_spot(&h_ptr->wpos, miny + (y - 1), minx + (x - 1));
						break;
					}
#endif
					else if (func == FILL_GUILD_SUS) {
						zcave[miny + (y - 1)][minx + (x - 1)].info |= CAVE_GUILD_SUS;
						break;
					}
					else if (func == FILL_GUILD_SUS_UNDO) {
						zcave[miny + (y - 1)][minx + (x - 1)].info &= ~CAVE_GUILD_SUS;
						break;
					}
					else if (func == FILL_SFX_KNOCK) {
						c_ptr = &zcave[miny + (y - 1)][minx + (x - 1)];
						if (c_ptr->m_idx < 0) {
							sound(-(c_ptr->m_idx), (h_ptr->flags & HF_MOAT) ? "knock_castle" : "knock", "block_shield_projectile", SFX_TYPE_COMMAND, FALSE);
							msg_print(-(c_ptr->m_idx), "\377sYou hear someone knocking on the house door.");
						}
					}
					else s_printf("poly fill house (func: %d)\n", func);
					break;
				case 1:	/* Actual walls */
					if (func == FILL_CLEAR) break;
					if (func == FILL_PLAYER) {
						player_type *p_ptr = (player_type*)data;
						if (p_ptr->px == minx + (x - 1) && p_ptr->py == miny + (y - 1))
							success = TRUE;
						break;
					}
					if (func == FILL_MAKEHOUSE) {
						c_ptr = &zcave[miny + (y - 1)][minx + (x - 1)];
						if (c_ptr->feat == FEAT_HOME || c_ptr->feat == FEAT_HOME_OPEN) {
							c_special *cs_ptr = GetCS(c_ptr, CS_DNADOOR);
							if (cs_ptr) cs_erase(c_ptr, cs_ptr);
						}
						c_ptr->feat = FEAT_DIRT;
						c_ptr->info &= ~(CAVE_ICKY | CAVE_ROOM | CAVE_STCK | CAVE_JAIL);
						everyone_lite_spot(&h_ptr->wpos, miny + (y - 1), minx + (x - 1));
					}
					else if (func == FILL_BUILD) {
//						zcave[miny + (y - 1)][minx + (x - 1)].feat = FEAT_PERM_EXTRA;
						zcave[miny + (y - 1)][minx + (x - 1)].feat = FEAT_WALL_HOUSE;
						if (h_ptr->flags & HF_JAIL) zcave[miny + (y - 1)][minx + (x - 1)].info |= CAVE_JAIL;
						everyone_lite_spot(&h_ptr->wpos, miny + (y - 1), minx + (x - 1));
					}
#ifdef HOUSE_PAINTING
					else if (func == FILL_UNPAINT) {
						zcave[miny + (y - 1)][minx + (x - 1)].colour = 0;
						/* refresh player's view on the freshly applied paint */
						everyone_lite_spot(&h_ptr->wpos, miny + (y - 1), minx + (x - 1));
						break;
					}
#endif
					else if (func == FILL_SFX_KNOCK) {
						c_ptr = &zcave[miny + (y - 1)][minx + (x - 1)];
						if (c_ptr->m_idx < 0) {
							sound(-(c_ptr->m_idx), (h_ptr->flags & HF_MOAT) ? "knock_castle" : "knock", "block_shield_projectile", SFX_TYPE_COMMAND, FALSE);
							msg_print(-(c_ptr->m_idx), "\377sYou hear someone knocking on the house door.");
						}
					}
					break;
			}
		}
	}
	C_KILL(matrix,mw*mh,char);
	return(success);
}

void wild_add_uhouse(house_type *h_ptr) {
 	int x,y;
 	cave_type *c_ptr;
	struct worldpos *wpos = &h_ptr->wpos;
	cave_type **zcave;
	struct c_special *cs_ptr;
	if (!(zcave = getcave(wpos))) return;

	if (h_ptr->flags & HF_DELETED) return; /* House destroyed. Ignore */

	/* draw our user defined house */
	if (h_ptr->flags & HF_RECT) {
		for (x = 0; x < h_ptr->coords.rect.width; x++) {
			c_ptr = &zcave[h_ptr->y][h_ptr->x + x];
			c_ptr->feat = FEAT_WALL_HOUSE;
			if (h_ptr->flags & HF_JAIL) c_ptr->info |= CAVE_JAIL;
		}
		for (y = h_ptr->coords.rect.height - 1, x = 0; x < h_ptr->coords.rect.width; x++) {
			c_ptr = &zcave[h_ptr->y + y][h_ptr->x + x];
			c_ptr->feat = FEAT_WALL_HOUSE;
			if (h_ptr->flags & HF_JAIL) c_ptr->info |= CAVE_JAIL;
		}
		for (y = 1; y < h_ptr->coords.rect.height; y++) {
			c_ptr = &zcave[h_ptr->y + y][h_ptr->x];
			c_ptr->feat = FEAT_WALL_HOUSE;
			if (h_ptr->flags & HF_JAIL) c_ptr->info |= CAVE_JAIL;
		}
		for (x = h_ptr->coords.rect.width - 1, y = 1; y < h_ptr->coords.rect.height; y++) {
			c_ptr = &zcave[h_ptr->y + y][h_ptr->x + x];
			c_ptr->feat = FEAT_WALL_HOUSE;
			if (h_ptr->flags & HF_JAIL) c_ptr->info |= CAVE_JAIL;
		}
	}
	fill_house(h_ptr, FILL_BUILD, NULL);
	if (h_ptr->flags & HF_MOAT) {
		/* Draw a moat around our house */
		/* It is already valid at this point */
		if(h_ptr->flags & HF_RECT) {
		}
	}

	/* get door */
	c_ptr = &zcave[h_ptr->dy][h_ptr->dx];

	/*
	 * usually, already done in poly_build..
	 * right, Evileye?	- Jir -
	 */
	if (!(cs_ptr = AddCS(c_ptr, CS_DNADOOR))) {
		if (!(cs_ptr = GetCS(c_ptr, CS_DNADOOR))) {
			s_printf("House creation failed!! (wild_add_uhouse)\n");
			return;
		}
	}
	c_ptr->feat = FEAT_HOME;

	cs_ptr->sc.ptr = h_ptr->dna;
}

void wild_add_uhouses(struct worldpos *wpos) {
	int i;
	for(i = 0; i < num_houses; i++){
		if(inarea(&houses[i].wpos, wpos) && !(houses[i].flags & HF_STOCK)){
			wild_add_uhouse(&houses[i]);
		}
	}
	load_guildhalls(wpos);
}

/* Called by wilderness_gen() - build a wilderness sector from memory */
static void wilderness_gen_hack(struct worldpos *wpos)
{
	int y, x, x1, x2, y1, y2;
	cave_type *c_ptr, *c2_ptr;
	int found_more_water;
	terrain_type terrain;
	bool rand_old = Rand_quick;

	wilderness_type *w_ptr = &wild_info[wpos->wy][wpos->wx];
	cave_type **zcave;
	if(!(zcave = getcave(wpos))) return;

	/* Hack -- Use the "simple" RNG */
	Rand_quick = TRUE;

	/* Hack -- Induce consistant wilderness */
	seed_wild_extra = Rand_value; /* save for use in terrain_spot() */
	Rand_value = seed_town + (wpos->wx + wpos->wy * MAX_WILD_X) * 600;

	/* if not already set, determine the type of terrain */
	if (w_ptr->type == WILD_UNDEFINED) w_ptr->type = determine_wilderness_type(wpos);

	/* initialize the terrain */
	terrain.type = w_ptr->type;
	init_terrain(&terrain,w_ptr->radius);

	/* hack -- set the monster level */
	monster_level = terrain.monst_lev;

	/* Hack -- Start with basic floors */
	for (y = 1; y < MAX_HGT - 1; y++) {
		for (x = 1; x < MAX_WID - 1; x++) {
			c_ptr = &zcave[y][x];
			c_ptr->feat = terrain_spot(&terrain);
		}
	}

#ifdef BLEED_WITH_NEIGHBOURS
	/* to make the borders between wilderness levels more seamless, "bleed"
	   the levels together */
	bleed_with_neighbors(wpos);
#endif

	/* hack -- reseed, just to make sure everything stays consistent. */
	Rand_value = seed_town + (wpos->wx + wpos->wy * MAX_WILD_X) * 287 + 490836;

	/* to make the level more interesting, add some "hotspots" */
	if (w_ptr->type != WILD_OCEAN || istownarea(wpos, MAX_TOWNAREA))
		for (y = 0; y < terrain.hotspot; y++) wild_add_hotspot(wpos);
	/* oceans really have mostly water */
	else if (!rand_int(8)) //changing hotspot number in init_terrain() directly probably messes up world consistency?
		for (y = 0; y < terrain.hotspot / 3; y++) wild_add_hotspot(wpos); //..so just divide here, pfft

	/* HACK -- if close to the town, make dwellings more likely */
#ifdef DEVEL_TOWN_COMPATIBILITY
	if (w_ptr->radius == 1) terrain.dwelling *= 21;
	if (w_ptr->radius == 2) terrain.dwelling *= 9;
	if (w_ptr->radius == 3) terrain.dwelling *= 3;
#else
 #ifdef __DISABLE_HOUSEBOOST
	if (w_ptr->radius == 1) terrain.dwelling *= 100;
	if (w_ptr->radius == 2) terrain.dwelling *= 20;
	if (w_ptr->radius == 3) terrain.dwelling *= 3;
 #else
	if (w_ptr->radius == 1) terrain.dwelling *= 160;
	if (w_ptr->radius == 2) terrain.dwelling *= 130;
	if (w_ptr->radius == 3) terrain.dwelling *= 40;
 #endif
#endif

//	wild_add_uhouses(wpos);


#ifndef DEVEL_TOWN_COMPATIBILITY
	/* Hack -- 50% of the time on a radius 1 level there will be a "park" which will make
	 * the rest of the level more densly packed together */
	if ((w_ptr->radius == 1) && !rand_int(2)) {
		reserve_building_plot(wpos, &x1, &y1, &x2, &y2, rand_int(30) + 15, rand_int(20) + 10, -1, -1);
	}
#endif

	/* add wilderness dwellings */
	/* hack -- the number of dwellings is proportional to their chance of existing */
	while (terrain.dwelling > 0) {
		if (rand_int(1000) < terrain.dwelling) wild_add_dwelling(wpos, -1, -1);
		terrain.dwelling -= 50;
	}

	/* add user-built houses */
	wild_add_uhouses(wpos);

	/* C. Blue - turn single deep water fields in wildernis to shallow (non-drownable) water:
	   Also turn single non-deep water fields in oceans into deep water -_-. */
	for (y = 1; y < MAX_HGT - 1; y++)
	for (x = 1; x < MAX_WID - 1; x++) {
		c_ptr = &zcave[y][x];
		/* turn single deep water fields to shallow */
		if (c_ptr->feat == FEAT_DEEP_WATER) {
			found_more_water = 0;
			for (y2 = y-1; y2 <= y+1; y2++)
			for (x2 = x-1; x2 <= x+1; x2++) {
				c2_ptr = &zcave[y2][x2];
				if (y2 == y && x2 == x) continue;
				if (!in_bounds(y2, x2)) {
					found_more_water++; /* hack */
					continue;
				}
				if (c2_ptr->feat == FEAT_SHAL_WATER ||
//				    c2_ptr->feat == FEAT_WATER ||
				    c2_ptr->feat == FEAT_TAINTED_WATER ||
				    c2_ptr->feat == FEAT_DEEP_WATER) {
					found_more_water++;
				}
			}
//			if (!found_more_water) c_ptr->feat = FEAT_SHAL_WATER;
			/* also important for SEASON_WINTER, to turn lake border into ice */
			if (found_more_water < 8) c_ptr->feat = FEAT_SHAL_WATER;
		}
		/* turn single non-<deep water> fields to deep water */
		else if (c_ptr->feat != FEAT_DEEP_WATER) {
			found_more_water = 0;
			for (y2 = y-1; y2 <= y+1; y2++)
			for (x2 = x-1; x2 <= x+1; x2++) {
				c2_ptr = &zcave[y2][x2];
				if (y2 == y && x2 == x) continue;
				if (!in_bounds(y2, x2)) {
					found_more_water++; /* hack */
					continue;
				}
				if (c2_ptr->feat == FEAT_SHAL_WATER ||
//				    c2_ptr->feat == FEAT_WATER ||
				    c2_ptr->feat == FEAT_TAINTED_WATER ||
				    c2_ptr->feat == FEAT_DEEP_WATER) {
					found_more_water++;
				}
			}
			if (found_more_water >= 7) c_ptr->feat = FEAT_SHAL_WATER;
		}
	}

	/* Hack -- use the "complex" RNG */
	Rand_quick = rand_old;

	/* Hack -- reattach existing objects to the map */
	setup_objects();
	/* Hack -- reattach existing monsters to the map */
	setup_monsters();
}


/* Generates a wilderness level. */
void wilderness_gen(struct worldpos *wpos)
{
	int i, y, x;
	cave_type *c_ptr;
	wilderness_type *w_ptr = &wild_info[wpos->wy][wpos->wx];
#ifdef SIMPLE_BLEED
	wilderness_type *w_ptr2;
#endif
	cave_type **zcave;
	struct dungeon_type *d_ptr = NULL;
	if(!(zcave = getcave(wpos))) return;


	process_hooks(HOOK_WILD_GEN, "d", wpos);


	/* Perma-walls -- North/South*/
	for (x = 0; x < MAX_WID; x++) {
		/* North wall */
		c_ptr = &zcave[0][x];

		/* Clear previous contents, add "clear" perma-wall */
		c_ptr->feat = FEAT_PERM_CLEAR;

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

	/* Hack -- Build some wilderness (from memory) */
	wilderness_gen_hack(wpos);
	if((w_ptr->flags & WILD_F_UP) && can_go_up(wpos, 0x1)) {
		zcave[w_ptr->dn_y][w_ptr->dn_x].feat = FEAT_LESS;
		d_ptr = w_ptr->tower;
		y = w_ptr->dn_y; x = w_ptr->dn_x;
#if 0
		/* Hack to fix custom (type 0) dungeons/towers that were corrupted by old
		   /debug-dun (/update-dun) command. */
		if (x == 0) x = w_ptr->dn_x = y = w_ptr->dn_y = 3;
#endif
	}
	if((w_ptr->flags & WILD_F_DOWN) && can_go_down(wpos, 0x1)) {
		zcave[w_ptr->up_y][w_ptr->up_x].feat = FEAT_MORE;
		d_ptr = w_ptr->dungeon;
		y = w_ptr->up_y; x = w_ptr->up_x;
#if 0
		/* Hack to fix custom (type 0) dungeons/towers that were corrupted by old
		   /debug-dun (/update-dun) command. */
		if (x == 0) x = w_ptr->up_x = y = w_ptr->up_y = 3;
#endif
	}
	/* add ambient features to the entrance so it looks less bland ;) - C. Blue */
	if (!istown(wpos) && d_ptr
	    /* don't overwrite house walls if house contains a staircase
	       (also see second check for this, further below) */
	    && !(zcave[y][x].info & (CAVE_ROOM | CAVE_ICKY))
	    ) {
		int j, k, solidity = 4;
		dungeon_info_type *di_ptr = &d_info[d_ptr->type];
		int feat_ambient = di_ptr->fill_type[0];//->inner_wall;
		int feat_ambient2 = (feat_ambient == FEAT_TREE) ? FEAT_BUSH : feat_ambient;
		int feat_floor = FEAT_DIRT;
		bool rand_old = Rand_quick; /* save rng */
		u32b tmp_seed = Rand_value;
		Rand_value = seed_town + (wpos->wx + wpos->wy * MAX_WILD_X) * 600; /* seed rng */
		Rand_quick = TRUE;
		int zx, zy;
		bool rig_corners = FALSE; /* corners are always floor, to make ambient feats look more circular? */

		/* hack for Cloud Planes */
		if (feat_ambient == FEAT_CLOUDYSKY) {
			feat_ambient = FEAT_HIGH_MOUNTAIN;
			feat_floor = FEAT_SNOW;
			solidity = 3;
			rig_corners = TRUE;
		} else
		/* grab usual floor tile */
		for (k = 0; k < 9; k++) {
			zx = x + ddx_ddd[k];
			zy = y + ddy_ddd[k];

			if ((zcave[zy][zx].info & (CAVE_ROOM | CAVE_ICKY))) continue;
			if ((f_info[zcave[zy][zx].feat].flags1 & FF1_WALL)) continue;
			if ((f_info[zcave[zy][zx].feat].flags1 & FF1_PERMANENT)) continue; /* don't clone the stairs ;) */

			feat_floor = zcave[zy][zx].feat;
			break;
		}

		/* towers use granite walls that look a bit like a tower basement */
		if (d_ptr == w_ptr->tower) {
			int rigged_rng = 0;
#if 1 /* clear inner ring? */
			for (k = 0; k < 9; k++) {
				zx = x + ddx_ddd[k];
				zy = y + ddy_ddd[k];

				/* don't overwrite house walls if house contains a staircase
				   (also see first check for this, further above)  */
				if ((zcave[zy][zx].info & (CAVE_ROOM | CAVE_ICKY))) continue;

				/* don't overwrite any perma walls (usually house walls) */
				if ((f_info[zcave[zy][zx].feat].flags1 & FF1_PERMANENT) &&
				    /* exception for mountains though, since they're not house-related and can just be overwritten */
				    zcave[zy][zx].feat != FEAT_MOUNTAIN) 
					continue;

				zcave[zy][zx].feat = feat_floor;
			}
#endif

			/* create 'disrupted' outer ring of 'tower walls' */
			for (k = 0; k < 16; k++) {
				zx = x + ddx_wide_cyc[k];
				zy = y + ddy_wide_cyc[k];

				/* don't overwrite house walls if house contains a staircase
				   (also see first check for this, further above)  */
				if ((zcave[zy][zx].info & (CAVE_ROOM | CAVE_ICKY))) continue;

				/* don't overwrite any perma walls (usually house walls) */
				if ((f_info[zcave[zy][zx].feat].flags1 & FF1_PERMANENT)) continue;

				if (rig_corners && (k == 0 || k == 4 || k == 8 || k == 12)) continue;

				/* random holes -- bug rig it to grant a hole at least every n spaces */
				if (!rand_int(solidity) || rigged_rng == 3) {
					rigged_rng = 0;

#if 1 /* convert grids where we don't set tower feats to actual floor? */
					/* don't overwrite house walls if house contains a staircase
					   (also see first check for this, further above)  */
					if ((zcave[zy][zx].info & (CAVE_ROOM | CAVE_ICKY))) continue;

					/* don't overwrite any perma walls (usually house walls) */
					if ((f_info[zcave[zy][zx].feat].flags1 & FF1_PERMANENT) &&
					    /* exception for mountains though, since they're not house-related and can just be overwritten */
					    zcave[zy][zx].feat != FEAT_MOUNTAIN)
						continue;

					zcave[zy][zx].feat = feat_floor;
					continue;
#endif
				}

				zcave[zy][zx].feat = feat_ambient;
				rigged_rng++;
			}
		}

		/* dungeons use the inner_wall fill_type[0] somewhat scattered/clumped */
		if (d_ptr == w_ptr->dungeon) {
			int zx, zy, xoff, yoff;

			/* pick amount of floor feats to set */
			j = rand_int(4) + 4;
			/* pick a random starting direction */
			i = randint(8);
			if (i == 5) i++; /* 5 isn't a legal direction */
			/* hack: avoid a bit silly-looking ones */
			if (j == 3 && (i % 2) == 1) i++;
			if (i == 10) i = 6; /* 10 isn't a legal direction */

			/* old/basic: just create a partial ring of dungeon-specific wall feats */
			/* cycle forward 'j' more grids and set them accordingly */
			for (k = 0; k < j; k++) {
				xoff = ddx[cycle[chome[i] + k]];
				yoff = ddy[cycle[chome[i] + k]];
				zx = x + xoff;
				zy = y + yoff;

				/* don't overwrite house walls if house contains a staircase
				   (also see first check for this, further above)  */
				if ((zcave[zy][zx].info & (CAVE_ROOM | CAVE_ICKY))) continue;

				/* don't overwrite any perma walls (usually house walls) */
				if ((f_info[zcave[zy][zx].feat].flags1 & FF1_PERMANENT) &&
				    /* exception for mountains though, since they're not house-related and can just be overwritten */
				    zcave[zy][zx].feat != FEAT_MOUNTAIN)
					continue;

				zcave[zy][zx].feat = feat_ambient;

#if 1 /* additionally extend the ambient feats slightly, into the 2nd ring, at random */
 #if 1
				if (xoff && yoff) continue; //extending onto 2nd ring diagonally looks bad
				if (!rand_int(3)) continue;

				zx += xoff;
				zy += yoff;
 #else
				if (!rand_int(3)) continue;

				switch (rand_int(3)) {
				case 0: zy += xoff; break;
				case 1: zy += yoff;
				case 2: zx += xoff;
				}
 #endif

				/* don't overwrite house walls if house contains a staircase
				   (also see first check for this, further above)  */
				if ((zcave[zy][zx].info & (CAVE_ROOM | CAVE_ICKY))) continue;

				/* don't overwrite any perma walls (usually house walls) */
				if ((f_info[zcave[zy][zx].feat].flags1 & FF1_PERMANENT) &&
				    /* exception for mountains though, since they're not house-related and can just be overwritten */
				    zcave[zy][zx].feat != FEAT_MOUNTAIN)
					continue;

				zcave[zy][zx].feat = feat_ambient2;
#endif
			}

#if 1
			/* new: also replace the rest with usual floor around here, for easier walking there */
			for (k = 0; k < 9; k++) {
				zx = x + ddx_ddd[k];
				zy = y + ddy_ddd[k];

				/* don't overwrite house walls if house contains a staircase
				   (also see first check for this, further above)  */
				if ((zcave[zy][zx].info & (CAVE_ROOM | CAVE_ICKY))) continue;

				/* don't overwrite any perma walls (usually house walls) */
				if ((f_info[zcave[zy][zx].feat].flags1 & FF1_PERMANENT) &&
				    /* exception for mountains though, since they're not house-related and can just be overwritten */
				    zcave[zy][zx].feat != FEAT_MOUNTAIN)
					continue;

				/* and most importantly don't overwrite our dungeon-wall feats */
				if (zcave[zy][zx].feat == feat_ambient) continue;

				zcave[zy][zx].feat = feat_floor;
#endif
			}
		}

		/* restore rng */
		Rand_quick = rand_old;
		Rand_value = tmp_seed;
	}
	/* TODO: add 'inscription' to the dungeon/tower entrances */


	/* Day Light */
	if (IS_DAY) {
		/* Make some day-time residents */
		if (!(w_ptr->flags & WILD_F_INHABITED)) {
//			for (i = 0; i < w_ptr->type; i++) wild_add_monster(wpos);
			for (i = 0; i < rand_int(8) + 3; i++) wild_add_monster(wpos);
			w_ptr->flags |= WILD_F_INHABITED;
		}
	}
	/* Night Time */
	else {
		/* Make some night-time residents */
		if (!(w_ptr->flags & WILD_F_INHABITED)) {
//			for (i = 0; i < w_ptr->type; i++) wild_add_monster(wpos);
			for (i = 0; i < rand_int(8) + 3; i++) wild_add_monster(wpos);
			w_ptr->flags |= WILD_F_INHABITED;
		}
	}

#ifdef SIMPLE_BLEED /* use the real bleed stuff again instead of bleed_warn_feat() silliness */
	/* Indicate certain adjacent wilderness terrain types, so players
	   won't suddenly get stuck in lava or mountains - C. Blue */
	for (x = 1; x < MAX_WID - 1; x++) {
		if ((wpos->wy < MAX_HGT - 1) && magik(30) &&
		    wild_info[wpos->wy + 1][wpos->wx].type != w_ptr->type) {
			w_ptr2 = &wild_info[wpos->wy + 1][wpos->wx];
			c_ptr = &zcave[1][x];
			/* Don't cover stairs - mikaelh */
			if (c_ptr->feat == FEAT_MORE || c_ptr->feat == FEAT_LESS) continue;
			bleed_warn_feat(w_ptr2->type, c_ptr);
		}
		if ((wpos->wy > 0) && magik(30) &&
		    wild_info[wpos->wy - 1][wpos->wx].type != w_ptr->type) {
			w_ptr2 = &wild_info[wpos->wy - 1][wpos->wx];
			c_ptr = &zcave[MAX_HGT-2][x];
			/* Don't cover stairs - mikaelh */
			if (c_ptr->feat == FEAT_MORE || c_ptr->feat == FEAT_LESS) continue;
			bleed_warn_feat(w_ptr2->type, c_ptr);
		}
	}
	for (y = 1; y < MAX_HGT - 1; y++) {
		if ((wpos->wx < MAX_WID - 1) && magik(30) &&
		    wild_info[wpos->wy][wpos->wx + 1].type != w_ptr->type) {
			w_ptr2 = &wild_info[wpos->wy][wpos->wx + 1];
			c_ptr = &zcave[y][MAX_WID-2];
			/* Don't cover stairs - mikaelh */
			if (c_ptr->feat == FEAT_MORE || c_ptr->feat == FEAT_LESS) continue;
			bleed_warn_feat(w_ptr2->type, c_ptr);
		}
		if ((wpos->wx > 0) && magik(30) &&
		    wild_info[wpos->wy][wpos->wx - 1].type != w_ptr->type) {
			w_ptr2 = &wild_info[wpos->wy][wpos->wx - 1];
			c_ptr = &zcave[y][1];
			/* Don't cover stairs - mikaelh */
			if (c_ptr->feat == FEAT_MORE || c_ptr->feat == FEAT_LESS) continue;
			bleed_warn_feat(w_ptr2->type, c_ptr);
		}
	}
#endif

	/* Set if we have generated the level before (unused now though, that
	   whether or not to respawn objects and monsters is decided by distinct
	   flags of their own - C. Blue) */
	w_ptr->flags |= WILD_F_GENERATED;

	/* set all those flags */
	w_ptr->flags |= WILD_F_INVADERS | WILD_F_HOME_OWNERS | WILD_F_BONES | WILD_F_FOOD | WILD_F_OBJECTS | WILD_F_CASH | WILD_F_GARDENS;
}


#define MAXISLAND 5	/* maximum 'generic' terrain type island size */
#define SEADENSITY 96	/* land/sea ratio */

#define MAXMOUNT 4	/* maximum mountain range size */
#define MAXWOOD 4	/* maximum forest size */
#define MAXWASTE 4	/* maximum wasteland size */
#define MAXLAKE 3	/* maximum lake size */
#define MAXISLANDS 4	/* maximum water-related island size */
#define MAXDESERT 6	/* maximum desert size (very rare, hence big) */
#define MAXICE 6	/* maximum desert size (very rare, hence big) */

//proportional inverse = amount
#define RIVERS 512	/* rivers (don't have a MAX size limiter) */
#define ROCKY 512	/* mountains */
#define WOODY 256	/* trees */
#define WASTE 1024	/* wasteland */
#define LAKES 512	/* lakes */
#define ISLANDS 512	/* water-related islands */
#define DESERT 1536	/* desert */
#define ICE 2404	/* ice */

static bool island_aux(int y, int x, unsigned char type, unsigned char fill, int size, int size_org) {
	bool added_decently = TRUE;
	int ranval;

	if (y < 0 || x < 0 || y >= MAX_WILD_Y || x >= MAX_WILD_Y) return (size_org - size >= 2);
	if (wild_info[y][x].type != fill) return (size_org - size >= 2);
	ranval = rand_int(15);
	if (size) {
		added_decently = FALSE;
		if (ranval&1) added_decently = island_aux(y, x - 1, type, fill, size - 1, size_org) || added_decently;
		if (ranval&2) added_decently = island_aux(y, x + 1, type, fill, size - 1, size_org) || added_decently;
		if (ranval&4) added_decently = island_aux(y - 1, x, type, fill, size - 1, size_org) || added_decently;
		if (ranval&8) added_decently = island_aux(y + 1, x, type, fill, size - 1, size_org) || added_decently;
	}
	if ((rand_int(7) == 0)) {
		switch(type){
			case WILD_MOUNTAIN:
				type = WILD_VOLCANO;
				break;
			case WILD_LAKE:
				type = WILD_SWAMP;
				break;
		}
	}
	wild_info[y][x].type = type;
	return added_decently;
}
static bool island(int y, int x, unsigned char type, unsigned char fill, int size) {
	int size_org = size;
	bool added_decently;

	/* hack: smally planned islands are always decently done */
	if (size_org <= 4) size_org = 9999;

	added_decently = island_aux(y, x, type, fill, size, size_org);
	return added_decently;
}

static void makeland() {
	int p, i;
	int x, y;
	int density = MAXISLAND;
	p = (MAX_WILD_Y * MAX_WILD_X) / SEADENSITY;
	for(i = 0; i < p; i++){
		do{
			x = rand_int(MAX_WILD_X - 1);
			y = rand_int(MAX_WILD_Y - 1);
		}while(wild_info[y][x].type != WILD_UNDEFINED);
		island(y, x, WILD_GRASSLAND, WILD_UNDEFINED, rand_int(1<<density));
	}
}

static unsigned short makecoast(unsigned char edge, unsigned char new, unsigned char type, unsigned char fill, int y, int x) {
	unsigned short r = 0;
	if(y < 0 || x < 0 || y >= MAX_WILD_Y || x >= MAX_WILD_X) return(0);
	if(wild_info[y][x].type != fill){
		return((wild_info[y][x].type == type));
	}
	wild_info[y][x].type = new;
	if(makecoast(edge, new, type, fill, y, x - 1)) r = 1;
	if(makecoast(edge, new, type, fill, y, x + 1)) r = 1;
	if(makecoast(edge, new, type, fill, y - 1, x)) r = 1;
	if(makecoast(edge, new, type, fill, y + 1, x)) r = 1;
	if(r)
		wild_info[y][x].type = edge;
	return(0);
}

static bool addhills() {
	bool added = FALSE;
	int i, p;
	int x, y;
	p = (MAX_WILD_Y * MAX_WILD_X) / ROCKY;
	for(i = 0; i < p; i++){
		do{
			x = rand_int(MAX_WILD_X - 1);
			y = rand_int(MAX_WILD_Y - 1);
		}while(wild_info[y][x].type != WILD_GRASSLAND);
		if (island(y, x, WILD_MOUNTAIN, WILD_GRASSLAND, rand_int((1<<MAXMOUNT) - 1))) added = TRUE;
	}
	return added;
}

static bool addlakes() {
	bool added = FALSE;
	int i, p;
	int x, y;
	p = (MAX_WILD_Y * MAX_WILD_X) / LAKES;
	for(i = 0; i < p; i++){
		do{
			x = rand_int(MAX_WILD_X - 1);
			y = rand_int(MAX_WILD_Y - 1);
		}while(wild_info[y][x].type != WILD_GRASSLAND);
		if (island(y, x, WILD_LAKE, WILD_GRASSLAND, rand_int((1<<MAXLAKE) - 1))) added = TRUE;
	}
	return added;
}

static bool addwaste() {
	bool added = FALSE;
	int i, p;
	int x, y;
	p = (MAX_WILD_Y * MAX_WILD_X) / WASTE;
	for(i = 0; i < p; i++){
		do{
			x = rand_int(MAX_WILD_X - 1);
			y = rand_int(MAX_WILD_Y - 1);
		}while(wild_info[y][x].type != WILD_GRASSLAND);
		if (island(y, x, WILD_WASTELAND, WILD_GRASSLAND, rand_int((1<<MAXWASTE) - 1))) added = TRUE;
	}
	return added;
}

static bool adddesert() {
	bool added = FALSE;
	int i, p;
	int x, y;
	p = (MAX_WILD_Y * MAX_WILD_X) / DESERT;
	for(i = 0; i < p; i++){
		do{
			x = rand_int(MAX_WILD_X - 1);
			y = rand_int(MAX_WILD_Y - 1);
		}while(wild_info[y][x].type != WILD_GRASSLAND);
		if (island(y, x, WILD_DESERT, WILD_GRASSLAND, (1<<(MAXDESERT-1)) + rand_int((1<<(MAXDESERT-1))) - 1)) added = TRUE;
//		if (island(y, x, WILD_DESERT, WILD_GRASSLAND, rand_int((1<<MAXDESERT) - 1))) added = TRUE;
	}
	return added;
}

static bool addice() {
	bool added = FALSE;
	int i, p;
	int x, y;
	p = (MAX_WILD_Y * MAX_WILD_X) / ICE;
	for(i = 0; i < p; i++){
		do{
			x = rand_int(MAX_WILD_X - 1);
			y = rand_int(MAX_WILD_Y - 1);
		}while(wild_info[y][x].type != WILD_GRASSLAND);
		if (island(y, x, WILD_ICE, WILD_GRASSLAND, (1<<(MAXICE-1)) + rand_int((1<<(MAXICE-1))) - 1)) added = TRUE;
//		if (island(y, x, WILD_ICE, WILD_GRASSLAND, rand_int((1<<MAXICE) - 1))) added = TRUE;
	}
	return added;
}

static bool addislands() {
	bool added = FALSE;
	int i, p;
	int x, y;
	p = (MAX_WILD_Y * MAX_WILD_X) / ISLANDS;
	for (i = 0; i < p; i++) {
		do {
			x = rand_int(MAX_WILD_X - 1);
			y = rand_int(MAX_WILD_Y - 1);
		} while (wild_info[y][x].type != WILD_OCEANBED1);
		if (island(y, x, WILD_GRASSLAND, WILD_OCEANBED1, rand_int((1U << MAXISLANDS) - 1))) added = TRUE;
	}
	return added;
}

static bool addforest() {
	bool added = FALSE;
	int i, p;
	int x, y;
	int size;
	p = (MAX_WILD_Y * MAX_WILD_X) / WOODY;
	for (i = 0; i < p; i++) {
		do {
			x = rand_int(MAX_WILD_X - 1);
			y = rand_int(MAX_WILD_Y - 1);
		} while (wild_info[y][x].type != WILD_GRASSLAND);
		size = rand_int((1U << MAXWOOD) - 1);
		if (island(y, x, WILD_FOREST, WILD_GRASSLAND, size)) added = TRUE;
		if (size > 3)
			if (island(y, x, WILD_DENSEFOREST, WILD_FOREST, size - 3)) added = TRUE;
	}
	return added;
}

static int mvx[] = {0, 1, 1, 1, 0, -1, -1, -1};
static int mvy[] = {1, 1, 0, -1, -1, -1, 0, 1};

static void river(int y, int x) {
	int mx, my;
	int dir, cdir, t;

	dir = rand_int(7);
	while (wild_info[y][x].type != WILD_OCEAN){
		cdir = dir;
		if (y < 0 || x < 0 || y >= MAX_WILD_Y || x >= MAX_WILD_X) break;
		wild_info[y][x].type = WILD_RIVER;
		t = rand_int(31);
		switch (t) {
			case 0: cdir += 4;
				break;
			case 1: cdir += 5;
				break;
			case 2: cdir += 3;
				break;
			case 3:
			case 4:
				cdir += 6;
				break;
			case 5:
			case 6: cdir += 2;
				break;
			case 7:
			case 8:
			case 9: cdir += 1;
				break;
			case 10:
			case 11:
			case 12: cdir += 7;
				break;
		}
		if (cdir > 7) cdir -= 8;
		mx = x + mvx[cdir];
		my = y + mvy[cdir];
		if (mx < 0 || my < 0 || mx >= MAX_WILD_X || my >= MAX_WILD_Y) continue;
		if (wild_info[my][mx].type == WILD_TOWN) continue;
		x = mx;
		y = my;
	}
}

static bool addrivers() {
	bool added = FALSE;
	int i, p;
	int x, y;
	p = (MAX_WILD_Y * MAX_WILD_X) / RIVERS;
	for (i = 0; i < p; i++) {
		do {
			x = rand_int(MAX_WILD_X - 1);
			y = rand_int(MAX_WILD_Y - 1);
		} while (wild_info[y][x].type != WILD_MOUNTAIN);
		river(y, x);
		added = TRUE;
	}
	return added;
}

/* remove coastlines that aren't adjacent to any sort of sea terrain - C. Blue */
static void fix_coasts() {
	int x, y, d;
	bool sea;

	for(x = 0; x < MAX_WILD_X; x++)
	for(y = 0; y < MAX_WILD_Y; y++) {
		if (wild_info[y][x].type != WILD_COAST) continue;

		sea = FALSE;
		for (d = 0; d < 8; d++) {
			int x2 = x + ddx_cyc[d];
			int y2 = y + ddy_cyc[d];
			if (in_bounds_wild(y2, x2)) {
				switch (wild_info[y2][x2].type) {
				case WILD_OCEAN:
				case WILD_OCEANBED1: case WILD_OCEANBED2:
				case WILD_SHORE1: case WILD_SHORE2:
					sea = TRUE;
					break;
				}
			}
		}
		/* if this coast doesn't make sense, reset it to default fill type grassland */
		if (!sea) wild_info[y][x].type = WILD_GRASSLAND;
	}
}

void genwild(bool all_terrains, bool dry_Bree) {
	int j,i;
	bool rand_old = Rand_quick;
	u32b old_seed = Rand_value;
	bool dry, got_everything;

	Rand_quick = TRUE;

	while (TRUE) {
		got_everything = TRUE;

		Rand_value = seed_town;

		island(cfg.town_y, cfg.town_x,WILD_GRASSLAND, WILD_UNDEFINED, 5);
		wild_info[cfg.town_y][cfg.town_x].type = WILD_TOWN;
		makeland();
		for (j = 0; j < MAX_WILD_Y; j++) {
			for (i = 0; i < MAX_WILD_X; i++) {
				if (wild_info[j][i].type == WILD_UNDEFINED) {
					makecoast(WILD_SHORE1, WILD_OCEANBED1, WILD_GRASSLAND, WILD_UNDEFINED, j, i);
				}
			}
		}
		for (j = 0 ; j < MAX_WILD_Y; j++) {
			for (i = 0 ;i < MAX_WILD_X; i++) {
				if (wild_info[j][i].type == WILD_OCEANBED1) {
					makecoast(WILD_SHORE2, WILD_OCEANBED2, WILD_SHORE1, WILD_OCEANBED1, j, i);
				}
			}
		}
		for (j = 0; j < MAX_WILD_Y; j++) {
			for (i = 0; i < MAX_WILD_X; i++) {
				if (wild_info[j][i].type == WILD_OCEANBED2) {
					makecoast(WILD_SHORE1, WILD_OCEANBED1, WILD_SHORE2, WILD_OCEANBED2, j, i);
				}
			}
		}
		addislands();
		for (j = 0; j <MAX_WILD_Y; j++) {
			for (i = 0; i < MAX_WILD_X; i++) {
				if (wild_info[j][i].type == WILD_SHORE1 || wild_info[j][i].type == WILD_SHORE2) {
					wild_info[j][i].type = WILD_OCEANBED1;
				}
			}
		}
		for (j = 0; j < MAX_WILD_Y;j++) {
			for (i = 0; i < MAX_WILD_X; i++) {
				if (wild_info[j][i].type == WILD_OCEANBED1) {
					makecoast(WILD_COAST, WILD_OCEAN, WILD_GRASSLAND, WILD_OCEANBED1, j, i);
				}
			}
		}

		fix_coasts();

		if (all_terrains) {
			got_everything = got_everything && addhills();
			got_everything = got_everything && addrivers();
			got_everything = got_everything && addforest();
			got_everything = got_everything && addlakes();
			got_everything = got_everything && addwaste();
			got_everything = got_everything && adddesert();
			got_everything = got_everything && addice();
		} else {
			addhills();
			addrivers();
			addforest();
			addlakes();
			addwaste();
			adddesert();
			addice();
		}

		/* Check that Bree is surrounded by pretty dry terrain */
		if (dry_Bree) {
			int tol = 0, dist;
			dry = TRUE;
			for (i = cfg.town_x - MAX_TOWNAREA - tol; i <= cfg.town_x + MAX_TOWNAREA + tol; i++) {
				for (j = cfg.town_y - MAX_TOWNAREA - tol; j <= cfg.town_y + MAX_TOWNAREA + tol; j++) {
					/* use a radius, not a square */
					//if (distance(j, i, 32, 32) > MAX_TOWNAREA) continue;
					//if (wild_info[j][i].radius > MAX_TOWNAREA) continue;
					dist = abs(i - cfg.town_x) + abs(j - cfg.town_y);
					if (dist > MAX_TOWNAREA + tol) continue;

					switch (wild_info[j][i].type) {
#if 0
					case WILD_OCEANBED1: case WILD_OCEANBED2:
					case WILD_COAST:
					case WILD_SHORE1: case WILD_SHORE2:
#endif
					/* hack: turn 'dry bree' rather into 'clean bree' */
					case WILD_FOREST:
					case WILD_DENSEFOREST:
					case WILD_SWAMP:
					case WILD_VOLCANO:
					case WILD_MOUNTAIN:
						if (dist <= 1) dry = FALSE;
						break;

					case WILD_DESERT:
					case WILD_ICE:
						/* fall through */

					/* the mainly annoying terrains: */
					case WILD_OCEAN:
					case WILD_RIVER:
					case WILD_LAKE:
					dry = FALSE;
					break;
					}
				}
			}
			got_everything = got_everything && dry;
		}

		if (got_everything) break;

		/* Change wilderness generation seed */
    		seed_town = rand_int(0x10000000);
		/* Kill Bree */
		for (i = 0; i < numtowns; i++) dealloc_stores(i);
	    	C_KILL(town, numtowns, struct town_type);
    		numtowns = 0;
		/* Re-init the wild_info array and try again */
		init_wild_info();
        }


	/* Restore random generator */
	Rand_quick = rand_old;
	Rand_value = old_seed;
}





/* Show a small radius of wilderness around the player */
bool reveal_wilderness_around_player(int Ind, int y, int x, int h, int w)
{
	player_type *p_ptr = Players[Ind];
	int i, j;
	bool shown = FALSE;

	/* Circle or square ? */
	if (h == 0) {
		for (i = x - w; i < x + w; i++) {
			for (j = y - w; j < y + w; j++) {
				/* Bound checking */
				if (!in_bounds_wild(j, i)) continue;

				/* We want a radius, not a "squarus" :) */
				if (distance(y, x, j, i) >= w) continue;

				/* New we know here */
				if (!(p_ptr->wild_map[(i + j * MAX_WILD_X) / 8] &
					(1U << ((i + j * MAX_WILD_X) % 8))))
				{
					p_ptr->wild_map[(i + j * MAX_WILD_X) / 8] |=
						(1U << ((i + j * MAX_WILD_X) % 8));
					shown = TRUE;
				}

#if 0
				wild_map[j][i].known = TRUE;

				/* Only if we are in overview */
				if (p_ptr->wild_mode)
				{
					cave[j][i].info |= (CAVE_GLOW | CAVE_MARK);

					/* Show it */
					lite_spot(j, i);
				}
#endif	// 0
			}
		}
	} else {
		for (i = x; i < x + w; i++) {
			for (j = y; j < y + h; j++) {
				/* Bound checking */
				if (!in_bounds_wild(j, i)) continue;

				/* New we know here */
				if (!(p_ptr->wild_map[(i + j * MAX_WILD_X) / 8] &
					(1U << ((i + j * MAX_WILD_X) % 8))))
				{
					p_ptr->wild_map[(i + j * MAX_WILD_X) / 8] |=
						(1U << ((i + j * MAX_WILD_X) % 8));
					shown = TRUE;
				}

#if 0
				wild_map[j][i].known = TRUE;

				/* Only if we are in overview */
				if (p_ptr->wild_mode) {
					cave[j][i].info |= (CAVE_GLOW | CAVE_MARK);

					/* Show it */
					lite_spot(j, i);
				}
#endif	// 0
			}
		}
	}

	return (shown);
}

/* Add new dungeons/towers that were added to d_info.txt after the server was already initialized - C. Blue */
void wild_add_new_dungeons(int Ind) {
	int i, j, k, x, y, tries, sx, sy;
	bool retry, skip, found;
	dungeon_type *d_ptr;
	worldpos wpos;

	for (i = 1; i < max_d_idx; i++) {
		retry = FALSE;

		/* Skip empty entry */
		if (!d_info[i].name) continue;

		/* Hack -- omit dungeons associated with towns */
		skip = FALSE;
		for (j = 1; j < 6; j++) {
			for (k = 0; k < 2; k++)
				if (town_profile[j].dungeons[k] == i) skip = TRUE;
		}
		if (skip) continue;

		/* Does this dungeon exist yet? */
		found = FALSE;
		for (y = 0; y < MAX_WILD_Y; y++)
		for (x = 0; x < MAX_WILD_X; x++) {
			if ((d_ptr = wild_info[y][x].tower)) {
				if (d_ptr->type == i) found = TRUE;
			}
			if ((d_ptr = wild_info[y][x].dungeon)) {
				if (d_ptr->type == i) found = TRUE;
//				if (!strcmp(d_ptr->name + d_name, d_info[i].name + d_name)) found = TRUE;
//				if (d_ptr->id == i) found = TRUE;
			}
		}
		if (found) continue;

		/* Add it */
		tries = 100;
		while (tries) {
			if (!Ind) {
				y = rand_int(MAX_WILD_Y);
				x = rand_int(MAX_WILD_X);
			} else {
				y = Players[Ind]->wpos.wy;
				x = Players[Ind]->wpos.wx;
				tries = 1;
			}
			retry = FALSE;

			wpos.wy = y;
			wpos.wx = x;

			/* Don't build them too near to towns
			 * (otherwise entrance can be within a house) */
			if (!Ind) for (j = 1; j < 6; j++) {
				if (distance(y, x, town[j].y, town[j].x) <= MAX_TOWNAREA) {
					retry = TRUE;
					break;
				}
			}
			if (!retry) {
				if (wild_info[y][x].dungeon || wild_info[y][x].tower) retry = TRUE;

				/* TODO: easy dungeons around Bree,
				 * hard dungeons around Lorien */
			}
#if 0
			if (retry) {
				if (tries-- > 0) i--;
				continue;
			}
#else
			tries--;
			if (!retry) break;
#endif
		}
		if (!tries) s_printf("Dungeon %d couldn't get added.\n", i);

		add_dungeon(&wpos, 0, 0, 0, 0, 0, FALSE, i, 0, 0, 0);

		/* 0 or MAX_{HGT,WID}-1 are bad places for stairs - mikaelh */
		if (!Ind) {
			sx = 2 + rand_int(MAX_WID - 4);
			sy = 2 + rand_int(MAX_HGT - 4);
		} else {
			sx = Players[Ind]->px;
			sy = Players[Ind]->py;
		}
		if (d_info[i].flags1 & DF1_TOWER) {
			s_printf(" (nldy %d, nldx %d)\n", sy, sx);
			new_level_down_y(&wpos, sy);
			new_level_down_x(&wpos, sx);
		} else {
			s_printf(" (nluy %d, nlux %d)\n", sy, sx);
			new_level_up_y(&wpos, sy);
			new_level_up_x(&wpos, sx);
		}
#if 0
		if ((zcave = getcave(&p_ptr->wpos))) {
			zcave[p_ptr->py][p_ptr->px].feat = FEAT_MORE;
		}
#endif	// 0

//#if DEBUG_LEVEL > 0
		s_printf("Dungeon %d is generated in %s.\n", i, wpos_format(0, &wpos));
//#endif	// 0

		tries = 100;
	}
}

/* manipulate flag of the player's wilderness level;
   accessible from lua */
void wild_flags(int Ind, u32b flags) {
	/* -1 = just display flags */
	if (flags == -1) {
		msg_format(Ind, "Current flags: %d (0x%X).",
		    wild_info[Players[Ind]->wpos.wy][Players[Ind]->wpos.wx].flags,
		    wild_info[Players[Ind]->wpos.wy][Players[Ind]->wpos.wx].flags);
		return;
	}

	msg_format(Ind, "Old flags: %d (0x%X). New flags: %d (0x%X).",
	    wild_info[Players[Ind]->wpos.wy][Players[Ind]->wpos.wx].flags,
	    wild_info[Players[Ind]->wpos.wy][Players[Ind]->wpos.wx].flags,
	    flags, flags);
	wild_info[Players[Ind]->wpos.wy][Players[Ind]->wpos.wx].flags = flags;
}

/* make wilderness more lively again, populating it in various ways - C. Blue
   flags values:
   0 -- season-dependant
   other than 0 -- clear flags in all wilderness (op: wildflags & ~flags)
*/
void lively_wild(u32b flags) {
	int x, y;
	for (x = 0; x < MAX_WILD_X; x++)
	for (y = 0; y < MAX_WILD_Y; y++) {
		/* specific value? */
		if (flags) {
			wild_info[y][x].flags &= ~flags;
			continue;
		}

		/* season-dependant? */
		/* except for winter, regrow gardens.
		   Maybe not very realistic timing, but looks nice.. */
		if (season != SEASON_WINTER) wild_info[y][x].flags &= ~WILD_F_GARDENS;
		/* much to eat from harvesting */
		if (season == SEASON_AUTUMN) wild_info[y][x].flags &= ~WILD_F_FOOD;
		/* bones from last winter o_O - also new people arrive */
		if (season == SEASON_SPRING) wild_info[y][x].flags &= ~(WILD_F_BONES | WILD_F_HOME_OWNERS | WILD_F_OBJECTS | WILD_F_CASH);
		/* they return home and are stocking up for winter */
		if (season == SEASON_WINTER) wild_info[y][x].flags &= ~(WILD_F_HOME_OWNERS | WILD_F_OBJECTS | WILD_F_CASH);
		/* now they come ^^ */
		//if (season == SEASON_SUMMER) wild_info[y][x].flags |= WILD_F_INVADERS;
		wild_info[y][x].flags &= ~WILD_F_INVADERS; /* every time? */
	}
}

#ifdef HOUSE_PAINTING
/* Paints a house at the location with potion in slot 'k' - C. Blue */
void paint_house(int Ind, int x, int y, int k) {
	player_type *p_ptr = Players[Ind];
	cave_type **zcave = getcave(&p_ptr->wpos), *hwc_ptr;
	struct c_special *cs_ptr;
	house_type *h_ptr = NULL;
	object_type *o_ptr;
	int h_idx, c;
	int hwx, hwy;

	/* Sanity check */
	if (k < 0 || k >= INVEN_PACK) return;

	/* see if we have a potion */
	o_ptr = &p_ptr->inventory[k];
	if (!o_ptr->k_idx) {
		msg_print(Ind, "\377oThat inventory slot is empty!");
		return;
	}
	if (o_ptr->tval != TV_POTION && o_ptr->tval != TV_POTION2) { /* please don't waste POTION2 for this oO */
		msg_print(Ind, "\377oThat inventory slot does not hold potions!");
		return;
	}

	/* Check for a house door next to us */
	h_idx = pick_house(&p_ptr->wpos, y, x);
	if (h_idx == -1) {
		msg_print(Ind, "There is no house next to you.");
		return;
	}
	h_ptr = &houses[h_idx];

	/* make sure we own the house */
	if (!(cs_ptr = GetCS(&zcave[y][x], CS_DNADOOR))) {
		msg_print(Ind, "You must have access to the house.");
		return;
	}
	if (!(access_door(Ind, cs_ptr->sc.ptr, FALSE) || is_admin(p_ptr))) {
		msg_print(Ind, "You must have access to the house.");
		return;
	}
	s_printf("HOUSE_PAINTING: %s used potion %d,%d.\n", p_ptr->name, o_ptr->tval, o_ptr->sval);

	/* extract colour from the potion */
	/* acquire potion colour */
	c = potion_col[o_ptr->sval + (o_ptr->tval == TV_POTION2 ? 4 : 0)];
	/* translate flickering colours to static ones */
	switch (c) {
	case TERM_ACID: c = TERM_SLATE; break;
	case TERM_COLD: c = TERM_L_WHITE; break;
	case TERM_ELEC: c = TERM_BLUE; break;
	case TERM_FIRE: c = TERM_ORANGE; break;
	case TERM_POIS: c = TERM_GREEN; break;
	case TERM_LITE: c = TERM_ORANGE; break;
	case TERM_HALF: c = TERM_L_WHITE; break; /* they all mix ;-p */
	case TERM_MULTI: c = TERM_L_WHITE; break; /* ... */
	case TERM_DARKNESS: c = TERM_L_DARK; break;
	case TERM_SOUN: c = TERM_L_UMBER; break;
	case TERM_CONF: c = TERM_UMBER; break;
	case TERM_SHAR: c = TERM_UMBER; break;
	case TERM_SHIELDI: c = TERM_L_WHITE; break; /* mixage again.. */
	case TERM_SHIELDM: c = TERM_VIOLET; break; /* well, either that or red.. */
	}
	/* 0 means 'no colour' so we start at 1 for colour 0 */
	c++;

	/* Hack: water removes paint :) */
	if (o_ptr->sval == SV_POTION_WATER) c = 0;
	/* Hack: add owner mode to colour */
	else if (h_ptr->dna->mode & MODE_EVERLASTING) c += 100;

	/* potion used up */
	inven_item_increase(Ind, k, -1);
	inven_item_describe(Ind, k);
	inven_item_optimize(Ind, k);

	/* paint house, colour adjacent walls accordingly */
	h_ptr->colour = c;

	for (hwx = x - 1; hwx <= x + 1; hwx++)
	for (hwy = y - 1; hwy <= y + 1; hwy++) {
		if (!in_bounds(hwy, hwx)) continue;

		hwc_ptr = &zcave[hwy][hwx];
		/* Only colour house wall grids */
		if (hwc_ptr->feat == FEAT_WALL_HOUSE ||
		    hwc_ptr->feat == FEAT_HOME ||
		    hwc_ptr->feat == FEAT_HOME_OPEN) {
			hwc_ptr->colour = c;

			/* refresh player's view on the freshly applied paint */
			everyone_lite_spot(&p_ptr->wpos, hwy, hwx);
		}
	}

	/* voila */
	if (c)
		msg_print(Ind, "With some effort, you paint your house door..");
	else
		msg_print(Ind, "With some effort, you remove the paint off your house door..");
}
#endif

void knock_house(int Ind, int x, int y) {
	player_type *p_ptr = Players[Ind];
	int h_idx;

	/* Check for a house door next to us */
	h_idx = pick_house(&p_ptr->wpos, y, x);
	if (h_idx == -1) {
		msg_print(Ind, "There is no house next to you.");
		return;
	}

	/* knock on the house door! */
	msg_format_near(Ind, "\377s%s knocks on the house door..", p_ptr->name);
	msg_print(Ind, "\377sYou knock on the house door..");
#ifdef USE_SOUND_2010
	//item_magestaff, block_shield_projectile!, (tunnel_rubble)
	//sound_near_site(y, x, &p_ptr->wpos, 0, (houses[h_idx].flags & HF_MOAT) ? "knock_castle" : "knock", "block_shield_projectile", SFX_TYPE_COMMAND, FALSE);//don't require LOS
	sound_house_knock(h_idx, x, y);
#endif
}

/* Modify grids of an outdoor level:
   Change features depending on season,
   change lighting depending on daytime */
void wpos_apply_season_daytime(worldpos *wpos, cave_type **zcave) {
	int x, y;
	cave_type *c_ptr;

	/* hack - abuse for WILD_ICE */
	if (wild_info[wpos->wy][wpos->wx].type == WILD_ICE) {
		/* Turn all water into ice */
		if (!wpos->wz)
			for (y = 1; y < MAX_HGT - 1; y++)
			for (x = 1; x < MAX_WID - 1; x++) {
				c_ptr = &zcave[y][x];
				if (c_ptr->feat == FEAT_SHAL_WATER ||
				    c_ptr->feat == FEAT_TAINTED_WATER ||
				    c_ptr->feat == FEAT_DEEP_WATER) {
					c_ptr->feat = FEAT_ICE;
				}
			}
	}
	/* apply season-specific FEAT-manipulation */
	else if (season == SEASON_WINTER
	    && wild_info[wpos->wy][wpos->wx].type != WILD_DESERT
	    && wild_info[wpos->wy][wpos->wx].type != WILD_VOLCANO) {
		/* Turn some water into ice */
		if (!wpos->wz)
			for (y = 1; y < MAX_HGT - 1; y++)
			for (x = 1; x < MAX_WID - 1; x++) {
				c_ptr = &zcave[y][x];
				if (c_ptr->feat == FEAT_SHAL_WATER ||
				    c_ptr->feat == FEAT_TAINTED_WATER) {
					c_ptr->feat = FEAT_ICE;
				}
			}
	}

	/* apply nightly darkening or daylight */
	if (!wpos->wz) {
		if (IS_DAY) world_surface_day(wpos);
		else world_surface_night(wpos);
	}
}

/* returns price for a house of a certain area */
s32b house_price_area(int area, bool has_moat, bool random) {
	s32b price = 0;

	// This is the dominant term for large houses
	if (area > 40) price = (area - 40) * (area - 40) * (area - 40) * 3;
	// This is the dominant term for medium houses
	price += area * area * 33;
	// This is the dominant term for small houses
	price += area * (950 + (random ? rand_int(100) : 100));

	/* Castles cost more */
	if (has_moat) price = (price * 5) / 4;

	return price;
}

/* returns buying price of a house */
s32b initial_house_price(house_type *h_ptr) {
	s32b price;
	int area = 0;

	/* 'area' us used for both, HF_RECT and HF_NONE (poly) houses */
	if (h_ptr->flags & HF_RECT)
		area = (h_ptr->coords.rect.width - 2) * (h_ptr->coords.rect.height - 2);
#if 0 /* TODO (only for house-creation-houses) */
	else {
		..curr->cvert
		area =
	}

	/* currently unused since this is only for house-creation houses,
	   and those would require implementing non-HF_RECT area (see code above) first */
	if (h_ptr->flags & HF_SELFBUILT)
		return area * area * 400;
#endif

#ifdef DEVEL_TOWN_COMPATIBILITY /* old */
	/* Setup some "house info" */
	price = (area * area);
	price *= 15;
	price *= 80 + randint(40);
#else
 #ifdef __DISABLE_HOUSEBOOST
	// This is the dominant term for large houses
	if (area > 40) price = (area - 40) * (area - 40) * (area - 40) * 3;
	else price = 0;
	// This is the dominant term for medium houses
	price += area * area * 33;
	// This is the dominant term for small houses
	price += area * (900 + rand_int(200));
 #else
	price = house_price_area(area, (h_ptr->flags & HF_MOAT), TRUE);
 #endif
#endif

	return price;
}

/* Returns town if we're within a town area of radius MAX_TOWNAREA (housing!).
   NOTE: This function assumes that towns have a minimum distance of 6,
   even diagonally! */
int wild_gettown(int x, int y) {
        int i;

    	if (wild_info[y][x].type == WILD_TOWN) {
	        for (i = 0; i < numtowns; i++)
    	        	if (town[i].x == x && town[i].y == y) return i;

	    	/* paranoia - shouldn't be possible */
	    	return -1;
	}

        if (wild_info[y][x].radius > MAX_TOWNAREA) return -1;

        for (i = 0; i < numtowns; i++) {
                if (town[i].x >= x - 3 &&
            	    town[i].x <= x + 3 &&
            	    town[i].y >= y - 3 &&
            	    town[i].y <= y + 3)
            		return i;
        }

	/* paranoia - shouldn't be possible */
        return -1;
}
