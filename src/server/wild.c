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

#define SERVER

#include "angband.h"


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
	if (!ring)
	{
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

/* returns the neighbor index, valid or invalid. */
int neighbor_index(int Depth, char dir)
{
	int cur_x, cur_y, neigh_idx;
	
	cur_x = wild_info[Depth].world_x;
	cur_y = wild_info[Depth].world_y;
		
	switch (dir)
	{				
		case DIR_NORTH: neigh_idx = world_index(cur_x, cur_y+1); break;
		case DIR_EAST:  neigh_idx = world_index(cur_x+1, cur_y); break;
		case DIR_SOUTH: neigh_idx = world_index(cur_x, cur_y-1); break;
		case DIR_WEST:  neigh_idx = world_index(cur_x-1, cur_y); break;
		/* invalid */
		default: neigh_idx = 1;
	}
	return neigh_idx;
}



/* Initialize the wild_info coordinates and radius. Uses a recursive fill algorithm.
   This may seem a bit out of place, but I think it is too complex to go in init2.c.
   Note that the flags for these structures are loaded from the server savefile.
   
   Note that this has to be initially called with 0,0 to work properly. 
*/
void init_wild_info_aux(int x, int y)
{
	int depth = world_index(x,y), neigh_idx;
	char dir;
	
	/* if we are a valid index, initialize */
	if (depth > -MAX_WILD)
	{
		wild_info[depth].world_x = x;
		wild_info[depth].world_y = y;
		wild_info[depth].radius = abs(x) + abs(y);
		wild_info[depth].type =  (!depth ? WILD_TOWN : WILD_UNDEFINED);
	}
	
	/* Initialize each of our uninitialized neighbors */
	
	/* north */
	neigh_idx = neighbor_index(depth, DIR_NORTH);
	/* if a valid neighbor */
	if (neigh_idx > -MAX_WILD)
		/* if it has not been set */
		if (!wild_info[neigh_idx].radius)
			init_wild_info_aux(x,y+1);
		
	/* east */
	neigh_idx = neighbor_index(depth, DIR_EAST);
	if (neigh_idx > -MAX_WILD)
		if (!wild_info[neigh_idx].radius)
			init_wild_info_aux(x+1,y);
	
		
	 /* south */
	neigh_idx = neighbor_index(depth, DIR_SOUTH);
	if (neigh_idx > -MAX_WILD) 
		if (!wild_info[neigh_idx].radius)
			init_wild_info_aux(x,y-1);
	
	/* west */
	neigh_idx = neighbor_index(depth, DIR_WEST);
	if (neigh_idx > -MAX_WILD) 
		if (!wild_info[neigh_idx].radius)
			init_wild_info_aux(x-1,y);
}

/* Initialize the wild_info coordinates and radius. Uses a recursive fill algorithm.
   This may seem a bit out of place, but I think it is too complex to go in init2.c.
   Note that the flags for these structures are loaded from the server savefile.
   
   Note that this has to be initially called with 0,0 to work properly. 
*/

void init_wild_info()
{
	memset(&wild_info[-MAX_WILD],0,sizeof(wilderness_type)*MAX_WILD);
	init_wild_info_aux(0,0);
}


/* Called when the player goes onto a wilderness level, to 
   make sure the lighting information is up to date with
   the time of day.
*/ 

void wild_apply_day(int Depth)
{
	int x,y;
	cave_type *c_ptr;
	
	/* scan the level */
	for (y = 0; y < MAX_HGT; y++)
	{
		for (x = 0; x < MAX_WID; x++)
		{
			c_ptr = &cave[Depth][y][x];
			c_ptr->info |= CAVE_GLOW;
		}
	}
}

void wild_apply_night(int Depth)
{
	int x,y;
	cave_type *c_ptr;

	/* scan the level */
	for (y = 0; y < MAX_HGT; y++)
	{
		for (x = 0; x < MAX_WID; x++)
		{
			c_ptr = &cave[Depth][y][x];
			
			/* Darken the features */
			if (!(c_ptr->info & CAVE_ROOM))
			{
				/* Darken the grid */
				c_ptr->info &= ~CAVE_GLOW;
			}
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
 */
static bool wild_monst_aux_lake(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* no reproducing monsters allowed */
	if (r_ptr->flags3 & RF2_MULTIPLY) return FALSE;

	/* animals are OK */
	if (r_ptr->flags3 & RF3_ANIMAL) return TRUE;
	/* humanoids and other races are OK */
	if (strchr("ph", r_ptr->d_char)) return TRUE;

	/* OK */
	return FALSE;
}


/*
 * Helper function for wild_add_monster
 */
static bool wild_monst_aux_grassland(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* no reproducing monsters allowed */
	if (r_ptr->flags3 & RF2_MULTIPLY) return FALSE;

	/* animals are OK */
	if (r_ptr->flags3 & RF3_ANIMAL) return TRUE; 

	/* what exactly is a yeek? */
	if (strchr("CEGOPTWYdhmpqvy", r_ptr->d_char)) return TRUE;
	
	if (!strcmp(&r_name[r_ptr->name],"Hill orc")) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Uruk")) return TRUE;

	/* town monsters are OK */
	if (!r_ptr->level) return TRUE;

	return FALSE;
}

/*
 * Helper function for wild_add_monster
 */
static bool wild_monst_aux_forest(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];
	
	/* snakes, wolves, beetles, and felines are OK */
	if (strchr("JCKf", r_ptr->d_char)) return (TRUE);
	if (!strcmp(&r_name[r_ptr->name],"Wood spider")) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Novice ranger")) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Novice archer")) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Druid")) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Forest wight")) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Forest troll")) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Dark elven druid")) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Mystic")) return TRUE;
	
	return FALSE;
}

/*
 * Helper function for wild_add_monster
 */
static bool wild_monst_aux_swamp(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];
		
	/* swamps are full of annoying monsters */
	if (strchr("Jwj,FGILMQRSVWceilmsz", r_ptr->d_char)) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Dark elven mage")) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Dark elven lord")) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Dark elven druid")) return TRUE;
	
	return FALSE;
}

/*
 * Helper function for wild_add_monster
 */
static bool wild_monst_aux_denseforest(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];
	
	if (!strcmp(&r_name[r_ptr->name],"Forest Troll")) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Mirkwood spider")) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Forest wight")) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Dark elven druid")) return TRUE;
	
	return FALSE;
		
}

/*
 * Helper function for wild_add_monster
 */
static bool wild_monst_aux_wasteland(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* wastelands are full of tough monsters */
	if (strchr("ABCDEFHLMOPTUVWXYZdefghopqv", r_ptr->d_char)) return TRUE;

	/* town monsters are OK ;-) */
	if (!r_ptr->level) return TRUE;
		
	return FALSE;	
		
}




/* this may not be the most efficient way of doing things... */
void wild_add_monster(int Depth)
{
	int monst_x, monst_y, r_idx;
	int tries = 0;

	/* reset the monster sorting function */	
	switch (wild_info[Depth].type)
	{
		case WILD_LAKE: get_mon_num_hook = wild_monst_aux_lake; break;
		case WILD_GRASSLAND: get_mon_num_hook = wild_monst_aux_grassland; break;
		case WILD_FOREST: get_mon_num_hook = wild_monst_aux_forest; break;
		case WILD_SWAMP: get_mon_num_hook = wild_monst_aux_swamp; break;
		case WILD_DENSEFOREST: get_mon_num_hook = wild_monst_aux_denseforest; break;
		case WILD_WASTELAND: get_mon_num_hook = wild_monst_aux_wasteland; break;
		default: get_mon_num_hook = NULL;	
	}
	get_mon_num_prep();
	
	/* find a legal, unoccupied space */
	while (tries < 50)
	{
		monst_x = rand_int(MAX_WID);
		monst_y = rand_int(MAX_HGT);
		
		if (cave_naked_bold(Depth, monst_y, monst_x)) break;
		tries++;
	}
	
	/* get the monster */
	r_idx = get_mon_num(monster_level);
	
	/* place the monster */
	place_monster_aux(Depth, monst_y, monst_x, r_idx, FALSE, TRUE, FALSE);
	
	/* hack -- restore the monster selection function */
	get_mon_num_hook = NULL;
	get_mon_num_prep();
}




/* chooses a clear building location, possibly specified by xcen, ycen, and "reserves" it so
 * nothing else can choose any of its squares for building again */
void reserve_building_plot(int Depth, int *x1, int *y1, int *x2, int *y2, int xlen, int ylen, int xcen, int ycen)
{
	int x,y, attempts = 0, plot_clear;

#ifdef DEVEL_TOWN_COMPATIBILITY
	while (attempts < 200)
#else
	while (attempts < 20)
#endif
	{
	
		/* if xcen, ycen have not been specified */
		if (!in_bounds(Depth,ycen,xcen))
		{
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
		}
		else
		{
			*x1 = xcen - xlen/2;
			*y1 = ycen - ylen/2;
			*x2 = *x1 + xlen-1;
			*y2 = *y1 + ylen-1;
			
			if ( (!in_bounds(Depth, *y1, *x1)) ||
			     (!in_bounds(Depth, *y2, *x2)) )
			{
				*x1 = *y1 = *x2 = *y2 = -1;
				return;
			}
		}
		
		plot_clear = 1;
		
		/* check if its clear */
		for (y = *y1; y <= *y2; y++)
		{
			for (x = *x1; x <= *x2; x++)
			{
				switch (cave[Depth][y][x].feat)
				{
					/* Don't build on other buildings or farms */
					case FEAT_LOOSE_DIRT:
					case FEAT_CROP:
					case FEAT_WALL_EXTRA:
					case FEAT_PERM_EXTRA:
					case FEAT_LOGS:
						plot_clear = 0;
						break;
					default: break;
				}
#ifndef DEVEL_TOWN_COMPATIBILITY
				/* any ickiness on the plot is NOT allowed */
				if (cave[Depth][y][x].info & CAVE_ICKY) plot_clear = 0;
				/* spaces that have already been reserved are NOT allowed */
				if (cave[Depth][y][x].info & CAVE_XTRA) plot_clear = 0;
#endif
			}
		}	
		
		/* hack -- buildings and farms can partially, but not completly,
		   be built on water. */
		if ( (cave[Depth][*y1][*x1].feat == FEAT_WATER) &&
		     (cave[Depth][*y2][*x2].feat == FEAT_WATER) ) plot_clear = 0;
			
		/* if we have a clear plot, reserve it and return */
		if (plot_clear) 
		{
			for (y = *y1; y <= *y2; y++)
			{
				for (x = *x1; x <= *x2; x++)
				{
					cave[Depth][y][x].info |= CAVE_XTRA; 
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

static void wild_add_garden(int Depth, int x, int y)
{
	int x1, y1, x2, y2, type, xlen, ylen, attempts = 0;
	char orientation;	
	object_type food;
	wilderness_type *w_ptr = &wild_info[Depth];
	int tmp_seed;
	
	x1 = x2 = y1 = y2 = -1;
	
	/* choose which type of garden it is */
	type = rand_int(7);
		
	/* choose a 'good' location for the garden */
	
	xlen = rand_int(rand_int(60)) + 15;
	ylen = rand_int(rand_int(20)) + 7;

#ifdef DEVEL_TOWN_COMPATIBILITY
	/* hack -- maximum distance to house -- 30 */
	while (attempts < 100)
	{
#endif
		
		reserve_building_plot(Depth, &x1,&y1, &x2,&y2, xlen, ylen, -1, -1);
#ifdef DEVEL_TOWN_COMPATIBILITY
		/* we have obtained a valid plot */
		if (x1 > 0)
		{
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
	for (y = y1; y <= y2; y++)
	{	
		for (x = x1; x <= x2; x++)
		{
			cave[Depth][y][x].feat = FEAT_LOOSE_DIRT;
		}	
	}
	
	/* save the RNG */
	tmp_seed = Rand_value;
	
	/* alternating rows of crops */
	for (y = y1+1; y <= y2-1; y ++)
	{
		for (x = x1+1; x <= x2-1; x++)
		{				
			/* different orientations */
			if (((!orientation) && (y%2)) || ((orientation) && (x%2)))
			{					 						
				/* set to crop */
				cave[Depth][y][x].feat = FEAT_CROP;	
				/* random chance of food */
				if (rand_int(100) < 40)
				{
					switch (type)							
					{
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
					if (!(w_ptr->flags & WILD_F_GENERATED)) drop_near(&food, -1, Depth, y, x);
				}				
			}
		}
	}
	/* restore the RNG */
	Rand_value = tmp_seed;
}


static bool wild_monst_aux_invaders(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];
		
	/* invader species */
	if (strchr("oTpOKbrm", r_ptr->d_char)) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Dark elven mage")) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Dark elven priest")) return TRUE;
	if (!strcmp(&r_name[r_ptr->name],"Dark elven warrior")) return TRUE;
	
	return FALSE;
}

static bool wild_monst_aux_home_owner(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];
	
	/* home owner species */
	if (strchr("hpP", r_ptr->d_char)) return TRUE;
	
	return FALSE;
}

static bool wild_obj_aux_bones(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* paranoia */
	if (k_idx < 0) return FALSE;

	if (k_ptr->tval == TV_SKELETON) return TRUE;		
	return FALSE;
}

/* make a dwelling 'interesting'.
*/
void wild_furnish_dwelling(int Depth, int x1, int y1, int x2, int y2, int type)
{
	int x,y, cash, num_food, num_objects, num_bones, trys, r_idx, k_idx, food_sval;
	bool inhabited, at_home, taken_over;
	object_type forge;
	wilderness_type *w_ptr = &wild_info[Depth];
	u32b old_seed = Rand_value;
	
	trys = cash = num_food = num_objects = num_bones = 0;
	inhabited = at_home = taken_over = FALSE;
	
	/* hack -- 75% of buildings are inhabited */
	if (rand_int(100) < 75) inhabited = TRUE;
	
	switch (type)
	{
		case WILD_LOG_CABIN:
			/* possibly add a farm */
			/* hack -- no farms near the town */
			if (w_ptr->radius > 1)
			{
				/* are we a farmer? */
				if (rand_int(100) < 50) wild_add_garden(Depth, (x1+x2)/2,(y1+y2)/2);
			}
		case WILD_ROCK_HOME:
			/* hack -- no farms near the town */
			if (w_ptr->radius > 1)
			{
				/* are we a farmer? */
				if (rand_int(100) < 40) wild_add_garden(Depth, (x1+x2)/2,(y1+y2)/2);
			}
		
		case WILD_PERM_HOME:		
			if (inhabited)			
			{
				/* is someone to be found at this house */
				if (rand_int(100) < 80) at_home = TRUE;
								
				/* is there any food inside */
				if (rand_int(100) < 80) num_food = rand_int(rand_int(20));
				
				/* is there any cash inside */
				if (rand_int(100) < 40) cash = rand_int(10);
				
				/* are there objects to be found */
				if (rand_int(100) < 50) num_objects = rand_int(rand_int(10));
			}
			else
			{
				if (rand_int(100) < 50) 
				{
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
	
	/* Hack -- if we have created this level before, do not add
	   anything to it.
	*/
	if (w_ptr->flags & WILD_F_GENERATED) 
	{
		/* hack -- restore the RNG */
		Rand_value = old_seed;	
		return;
	}
	
	/* add the cash */
	
	if (cash)
	{	
		/* try to place the cash */
		while (trys < 50)
		{
			x = rand_range(x1,x2);
			y = rand_range(y1,y2);
		
			if (cave_clean_bold(Depth,y,x))
			{
				object_level = cash;			
				place_gold(Depth,y,x);						
				break;
			}
		trys++;
		}		
	}
	
	/* add the objects */
	
	trys = 0;
	while ((num_objects) && (trys < 300))
	{
		x = rand_range(x1,x2);
		y = rand_range(y1,y2);
		
		if (cave_clean_bold(Depth,y,x))
		{			
			object_level = w_ptr->radius/2 +1;
			place_object(Depth,y,x,FALSE,FALSE);
			num_objects--;
		}
		trys++;	
	}
	
	/* add the food */
	
	trys = 0;
	
	while ((num_food) && (trys < 100))
	{
		x = rand_range(x1,x2);
		y = rand_range(y1,y2);
		
		if (cave_clean_bold(Depth,y,x))
		{		
			food_sval = SV_FOOD_MIN_FOOD+rand_int(12);
			/* hack -- currently no food svals between 25 and 32 */
			if (food_sval > 25) food_sval += 6;
			/* hack -- currently no sval 34 */
			if (food_sval > 33) food_sval++;
			
			k_idx = lookup_kind(TV_FOOD,food_sval);
			invcopy(&forge, k_idx);
			drop_near(&forge, -1, Depth, y, x);
			
			num_food--;
		}
		trys++;	
	}
	
	/* add the bones */
	
	trys = 0;
	
	get_obj_num_hook = wild_obj_aux_bones;
	get_obj_num_prep();
	
	while ((num_bones) && (trys < 100))
	{
		x = rand_range(x1,x2);
		y = rand_range(y1,y2);
		
		if (cave_clean_bold(Depth,y,x))
		{		
			/* base of 500 feet for the bones */
			k_idx = get_obj_num(10);
			invcopy(&forge, k_idx);
			drop_near(&forge, -1, Depth, y, x);
			
			num_bones--;
		}
		trys++;	
		
	}


	/* hack -- restore the old object selection function */
		
	get_obj_num_hook = NULL;
	get_obj_num_prep();
	
	/* add the inhabitants */
	
	if (at_home)
	{
		/* determine the home owners species*/
		get_mon_num_hook = wild_monst_aux_home_owner;
		get_mon_num_prep();		
		/* homeowners can be tough */
		r_idx = get_mon_num(w_ptr->radius);
		
		/* get the owners location */
		x = rand_range(x1,x2)+rand_int(40)-20;
		y = rand_range(y1,y2)+rand_int(16)-8;
		
		/* place the owner */
		
		place_monster_aux(Depth, y,x, r_idx, FALSE, FALSE, FALSE);
	}
	
	
	/* add the invaders */	
	if (taken_over)
	{	
		/* determine the invaders species*/
		get_mon_num_hook = wild_monst_aux_invaders;
		get_mon_num_prep();	 
		r_idx = get_mon_num((w_ptr->radius/2)+1);
	
		/* add the monsters */
		for (y = y1; y <= y2; y++)
		{
			for (x = x1; x <= x2; x++)
			{
				place_monster_aux(Depth, y,x, r_idx, FALSE, FALSE, FALSE);
			}
		}
	}
	
	/* hack -- restore the RNG */
	Rand_value = old_seed;
}


/* adds a building to the wilderness. if the coordinate is not given,
   find it randomly.

 for now will make a simple box,
   but we could do really fun stuff with this later.
*/
static void wild_add_dwelling(int Depth, int x, int y)
{
	int	h_x1,h_y1,h_x2,h_y2, p_x1,p_y1,p_x2,p_y2, 
		plot_xlen, plot_ylen, house_xlen, house_ylen, 
		door_x, door_y, drawbridge_x[3], drawbridge_y[3], 
		tmp, type, area, price, num_door_attempts;
	char wall_feature, door_feature, has_moat = 0;
	cave_type *c_ptr;
	wilderness_type *w_ptr=&wild_info[Depth];
	bool rand_old = Rand_quick;
	
	/* Hack -- Use the "simple" RNG */
	Rand_quick = TRUE;
	
	/* Hack -- Induce consistant wilderness */
	/* Rand_value = seed_town + (Depth * 600) + (w_ptr->dwellings * 200);*/
	
#ifdef DEVEL_TOWN_COMPATIBILITY
	house_xlen = rand_int(10) + 3;
	house_ylen = rand_int(5) + 3;
	plot_xlen = house_xlen;
	plot_ylen = house_ylen;
#else
	/* find the dimensions of the house */
	/* chance of being a "large" house */
	if (!rand_int(2))
	{
		house_xlen = rand_int(10) + rand_int(rand_int(10)) + 9;
		house_ylen = rand_int(5) + rand_int(rand_int(5)) + 6;
	}
	/* chance of being a "small" house */
	else if (!rand_int(2))
	{
		house_xlen = rand_int(4) + 3;
		house_ylen = rand_int(2) + 3;
	}
	/* a "normal" house */
	else
	{
		house_xlen = rand_int(10) + 3;
		house_ylen = rand_int(5) + 3;
	}
	area = (house_xlen-2) * (house_ylen-2);

	/* find the dimensions of the "lawn" the house is built on */
	if (area < 30)
	{
		plot_xlen = house_xlen; 
		plot_ylen = house_ylen;
	}
	else if (area < 60)
	{
		plot_xlen = house_xlen + (area/15)*2;
		plot_ylen = house_ylen + (area/25)*2;
		//plot_xlen = house_xlen + (area/10)*2;
		//plot_ylen = house_ylen + (area/16)*2;
	}
	else
	{
		plot_xlen = house_xlen + (area/8)*2;
		plot_ylen = house_ylen + (area/14)*2;
	}

	/* Hack -- sometimes large buildings get moats */
	if ((area >= 70) && (!rand_int(16))) has_moat = 1;
	if ((area >= 80) && (!rand_int(6))) has_moat = 1;
	if ((area >= 100) && (!rand_int(2))) has_moat = 1;
	if ((area >= 130) && (rand_int(4) < 3)) has_moat = 1;
	if (has_moat) plot_xlen += 8; 
	if (has_moat) plot_ylen += 8; 
#endif
	
	/* Determine the plot's boundaries */
	reserve_building_plot(Depth, &p_x1, &p_y1, &p_x2, &p_y2, plot_xlen, plot_ylen, x, y);
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
	if (w_ptr->radius == 1) 
	{
		/* hack -- not many log cabins near town */
		if (type == WILD_LOG_CABIN)
		{
			if (rand_int(100) < 80) type = WILD_ROCK_HOME;
		}
#ifdef DEVEL_TOWN_COMPATIBILITY
		if (rand_int(100) < 40) type = WILD_TOWN_HOME;
#else
		if (rand_int(100) < 90) type = WILD_TOWN_HOME;
#endif
	}
	if (w_ptr->radius == 2) 
#ifdef DEVEL_TOWN_COMPATIBILITY
		if (rand_int(100) < 10) type = WILD_TOWN_HOME;
#else
		if (rand_int(100) < 80) type = WILD_TOWN_HOME;
#endif
	
	switch (type)
	{
		case WILD_LOG_CABIN:
			wall_feature = FEAT_LOGS;
			
			/* doors are locked 1/3 of the time */
			if (rand_int(100) < 33) door_feature = FEAT_DOOR_HEAD + rand_int(7);
			else door_feature = FEAT_DOOR_HEAD;
						
			break;
		case WILD_PERM_HOME:
			wall_feature = FEAT_PERM_EXTRA;
			
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
			wall_feature = FEAT_PERM_EXTRA;
			door_feature = FEAT_HOME_HEAD;

#ifdef	DEVEL_TOWN_COMPATIBILITY
			/* Setup some "house info" */
			price = (h_x2 - h_x1 - 1) * (h_y2 - h_y1 - 1);
			price *= 15;
			price *= 80 + randint(40);
#else
			// This is the dominant term for large houses
			if (area > 40) price = (area-40)*(area-40)*(area-40)*3;
			else price = 0;
			//price = area*area*area*area/190;
			// This is the dominant term for medium houses
		  	price += area*area*33;
			// This is the dominant term for small houses
			price += area * (900 + rand_int(200)); 
#endif

			/* Remember price */
			
			/* hack -- setup next possibile house addition */
#ifdef NEWHOUSES
			printf("Making wild house %d\n",num_houses);
			MAKE(houses[num_houses].dna, struct dna_type);
			houses[num_houses].dna->price = price;
			houses[num_houses].x = h_x1+1;
			houses[num_houses].y = h_y1+1;
			houses[num_houses].flags = HF_RECT;
			if(has_moat)
				houses[num_houses].flags = HF_MOAT;
			houses[num_houses].coords.rect.width = h_x1-h_x2;
			houses[num_houses].coords.rect.height = h_y1-h_y2;
#else
			houses[num_houses].price = price;
			houses[num_houses].x_1 = h_x1+1;
			houses[num_houses].y_1 = h_y1+1;
			houses[num_houses].x_2 = h_x2-1;
			houses[num_houses].y_2 = h_y2-1;
#endif
			houses[num_houses].depth = Depth;
			break;
	}


	/* select the door location... done here so we can
	   try to prevent it form being put on water. */

	/* hack -- avoid doors in water */
	num_door_attempts = 0;
	do
	{
		/* Pick a door direction (S,N,E,W) */
		tmp = rand_int(4);	
	
		/* Extract a "door location" */
		switch (tmp)
		{
			/* Bottom side */
			case DIR_SOUTH:
				door_y = h_y2;
				door_x = rand_range(h_x1, h_x2);
				if (has_moat){
					drawbridge_y[0] = h_y2+1; drawbridge_y[1] = h_y2+2;
					drawbridge_y[2] = h_y2+3;
					drawbridge_x[0] = door_x; drawbridge_x[1] = door_x;
					drawbridge_x[2] = door_x;
					}
				break;
			/* Top side */
			case DIR_NORTH:
				door_y = h_y1;
				door_x = rand_range(h_x1, h_x2);
				if (has_moat){
					drawbridge_y[0] = h_y1-1; drawbridge_y[1] = h_y1-2;
					drawbridge_y[2] = h_y1-3;
					drawbridge_x[0] = door_x; drawbridge_x[1] = door_x;
					drawbridge_x[2] = door_x;
					}
				break;
			/* Right side */
			case DIR_EAST:
				door_y = rand_range(h_y1, h_y2);
				door_x = h_x2;
				if (has_moat){
					drawbridge_y[0] = door_y; drawbridge_y[1] = door_y;
					drawbridge_y[2] = door_y; 
					drawbridge_x[0] = h_x2+1; drawbridge_x[1] = h_x2+2;
					drawbridge_x[2] = h_x2+3; 
					}
				break;

			/* Left side */
			default:
				door_y = rand_range(h_y1, h_y2);
				door_x = h_x1;
				if (has_moat){
					drawbridge_y[0] = door_y; drawbridge_y[1] = door_y;
					drawbridge_y[2] = door_y; 
					drawbridge_x[0] = h_x1-1; drawbridge_x[1] = h_x1-2;
					drawbridge_x[2] = h_x1-3; 
					}
			break;
		}
		/* Access the grid */
		c_ptr = &cave[Depth][door_y][door_x];
		num_door_attempts++;
	}	
	while ((c_ptr->feat == FEAT_WATER) && (num_door_attempts < 30));
				
	/* Build a rectangular building */
	for (y = h_y1; y <= h_y2; y++)
	{
		for (x = h_x1; x <= h_x2; x++)
		{
			/* Get the grid */
			c_ptr = &cave[Depth][y][x];

			/* Clear previous contents, add "basic" perma-wall */
			c_ptr->feat = wall_feature;
		}
	}		
		
	/* make it hollow */
	for (y = h_y1 + 1; y < h_y2; y++)
	{
		for (x = h_x1 + 1; x < h_x2; x++)
		{
			/* Get the grid */
			c_ptr = &cave[Depth][y][x];

			/* Fill with floor */
			c_ptr->feat = FEAT_FLOOR;

			/* Make it "icky" */
			c_ptr->info |= CAVE_ICKY;			
		}
	}
		
	
	/* add the door */
	c_ptr = &cave[Depth][door_y][door_x];
	c_ptr->feat = door_feature;

	/* Build the moat */
	if (has_moat)
	{
		/* North / South */
		for (x = h_x1-2; x <= h_x2+2; x++)
		{
			cave[Depth][h_y1-2][x].feat = FEAT_WATER; cave[Depth][h_y1-2][x].info |= CAVE_ICKY;
			cave[Depth][h_y1-3][x].feat = FEAT_WATER; cave[Depth][h_y1-3][x].info |= CAVE_ICKY;
			cave[Depth][h_y2+2][x].feat = FEAT_WATER; cave[Depth][h_y2+2][x].info |= CAVE_ICKY;
			cave[Depth][h_y2+3][x].feat = FEAT_WATER; cave[Depth][h_y2+3][x].info |= CAVE_ICKY;
		}		
		/* East / West */
		for (y = h_y1-2; y <= h_y2+2; y++)
		{
			/* Get the grid */
			cave[Depth][y][h_x1-2].feat = FEAT_WATER; cave[Depth][y][h_x1-2].info |= CAVE_ICKY;
			cave[Depth][y][h_x1-3].feat = FEAT_WATER; cave[Depth][y][h_x1-3].info |= CAVE_ICKY;
			cave[Depth][y][h_x2+2].feat = FEAT_WATER; cave[Depth][y][h_x2+2].info |= CAVE_ICKY;
			cave[Depth][y][h_x2+3].feat = FEAT_WATER; cave[Depth][y][h_x2+3].info |= CAVE_ICKY;
		}		
		cave[Depth][drawbridge_y[0]][drawbridge_x[0]].feat = FEAT_DRAWBRIDGE;
		cave[Depth][drawbridge_y[0]][drawbridge_x[0]].info |= CAVE_ICKY;
		cave[Depth][drawbridge_y[1]][drawbridge_x[1]].feat = FEAT_DRAWBRIDGE;
		cave[Depth][drawbridge_y[1]][drawbridge_x[1]].info |= CAVE_ICKY;
		cave[Depth][drawbridge_y[2]][drawbridge_x[2]].feat = FEAT_DRAWBRIDGE;
		cave[Depth][drawbridge_y[2]][drawbridge_x[2]].info |= CAVE_ICKY;
	}
	
 	/* Hack -- finish making a town house */
	
	if (type == WILD_TOWN_HOME)
	{
		/* hack -- only add a house if it is not already in memory */
		if ((tmp=pick_house(Depth, door_y, door_x)) == -1)
		{
#ifdef NEWHOUSES
			printf("Finishing wild house %d\n",num_houses);
			c_ptr->special=houses[num_houses].dna;
			houses[num_houses].dx = door_x;
			houses[num_houses].dy = door_y;
			houses[num_houses].dna->creator=0L;
			houses[num_houses].dna->owner=0L;
#else
			houses[num_houses].door_y = door_y;
			houses[num_houses].door_x = door_x;
			houses[num_houses].owned = 0;
#endif
			num_houses++;
		}
#ifdef NEWHOUSES
		else{
			/* malloc madness otherwise */
			printf("Deleting invalid wild house %d\n",num_houses);
			KILL(houses[num_houses].dna, struct dna_type);
			c_ptr->special=houses[tmp].dna;
		}
#endif
	}
		
	/* make the building interesting */
	wild_furnish_dwelling(Depth, h_x1+1,h_y1+1,h_x2-1,h_y2-1, type);
	
	/* Hack -- use the "complex" RNG */
	Rand_quick = rand_old;
}




/* auxillary function to determine_wilderness_type, used for terminating 
   infinite loops of clones pointing at eachother.  see below. originially
   counted the length of the loop, but as virtually all loops turned out
   to be 2 in length, it was revised to find the total depth of the loop.
*/

int wild_clone_closed_loop_total(int cur_depth)
{
	int start_depth, total_depth, neigh_idx;
	wilderness_type *w_ptr = &wild_info[cur_depth];
		
	total_depth = 0;
	
	/* save our initial position */
	start_depth = cur_depth;
	
	/* until we arrive back at our initial position */
	do
	{
		/* seed the number generator */
		Rand_value = seed_town + cur_depth * 600;
		/* HACK -- the second rand after the seed is used for the beginning of the clone
		   directions (see below function).  This rand sets things up. */
		   rand_int(100); 
	
		/* get a valid neighbor location */
		do
		{
			neigh_idx = neighbor_index(cur_depth, rand_int(4));
 		} while ((neigh_idx >= 0) || (neigh_idx <= -MAX_WILD));
 		
 		/* move to this new location */
 		cur_depth = neigh_idx;
 		
 		/* increase our loop total depth */
		total_depth += cur_depth;
		
	} while (cur_depth != start_depth);
	
	return total_depth;	
}


/* figure out what kind of terrain a depth is */
/* this function assumes that wild_info's world_x and world_y values have been set. */ 

/* Hack -- Read this for an explenation of the wilderness generation. Each square
   is seeded with a seed dependent on its depth, and this is used to find its terrain type.
   If it is of type 'clone', then a random direction is picked, and it becomes the type
   of terrain that its neighbor is, using recursion if neccecary.  This was causing 
   problems with closed loops of clones, so I came up with a mega-hack solution : 
   if we notice we are in a closed loop, find the total depth of the loop by adding 
   all its components, and use this to seed the pseudorandom number generator and set 
   the loops terrain.
   
   Note that a lot of this craziness is performed to keep the wilderness' terrain
   types independent of the order in which they are explored; they are completly defiend 
   by the pseudorandom seed seed_town.
*/

int determine_wilderness_type(int Depth)
{
	int neighbor_idx, dir, closed_loop = -0xFFF;
	wilderness_type *w_ptr = &wild_info[Depth];
	bool rand_old = Rand_quick;
	u32b old_seed = Rand_value;	
				
	/* Hack -- Use the "simple" RNG */
	Rand_quick = TRUE;

	/* Hack -- Induce consistant wilderness */
	Rand_value = seed_town + Depth * 600;
		
	/* check if the town */
	if (!Depth) return WILD_TOWN;		
		
	/* check if already defined */
 	if ((w_ptr->type != WILD_UNDEFINED) && (w_ptr->type != WILD_CLONE)) return w_ptr->type;

	/* check for infinite loops */
	if (w_ptr->type == WILD_CLONE)
	{
		/* Mega-Hack -- we are in a closed loop of clones, find the length of the loop
		and use this to seed the pseudorandom number generator. */
		closed_loop = wild_clone_closed_loop_total(Depth);
		Rand_value = seed_town + closed_loop * 8973;
	}

	/* randomly determine the level type */
	/* hack -- if already a clone at this point, we are in a closed loop.  We 
	terminate the loop by picking a nonclone terrain type.  Yes, this prevents
	"large" features from forming, but the resulting terrain is still rather
	pleasing.
	*/
	if ((rand_int(100) <  101) && (w_ptr->type != WILD_CLONE)) w_ptr->type = WILD_CLONE;
	else if (rand_int(100) < 3) w_ptr->type = WILD_WASTELAND;
	else if (rand_int(100) < 5) w_ptr->type = WILD_DENSEFOREST;
	else if (rand_int(100) < 40) w_ptr->type = WILD_FOREST;
	else if (rand_int(100) < 10) w_ptr->type = WILD_SWAMP;
	else if (rand_int(100) < 15) w_ptr->type = WILD_LAKE;
	else w_ptr->type = WILD_GRASSLAND; 								
	

#ifdef	DEVEL_TOWN_COMPATIBILITY
	/* hack -- grassland is likely next to the town */
	if (closed_loop > -20) 
		if (rand_int(100) < 60) w_ptr->type = WILD_GRASSLAND;
#endif
	
	/* if a "clone", copy the terrain type from a neighbor, and recurse if neccecary. */
	if (w_ptr->type == WILD_CLONE)
	{
		neighbor_idx = 0;
		
		/* get a legal neighbor index */
		/* illegal locations -- the town and off the edge */
		
		while ((neighbor_idx >= 0) || (neighbor_idx <= -MAX_WILD))
		{
			/* pick a random direction */
			neighbor_idx = neighbor_index(Depth, rand_int(4));
		}
		
		/* recursively figure out our terrain type */	   	
		w_ptr->type = determine_wilderness_type(neighbor_idx);
		
#ifndef	DEVEL_TOWN_COMPATIBILITY
		if (w_ptr->radius <= 2)
			switch (w_ptr->type)
			{
				/* no wastelands next to town */
				case WILD_WASTELAND : 
					w_ptr->type = WILD_GRASSLAND; break;
				/* dense forest is rarly next to town */
				case WILD_DENSEFOREST : 
					if (rand_int(100) < 80) w_ptr->type = WILD_GRASSLAND; break;
				/* people usually don't build towns next to a swamp */
				case WILD_SWAMP : 
					if (rand_int(100) < 50) w_ptr->type = WILD_GRASSLAND; break;
				/* forest is slightly less common near a town */
				case WILD_FOREST : 
					if (rand_int(100) < 30) w_ptr->type = WILD_GRASSLAND; break;
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
	int eviltree;
	int dwelling;
	int hotspot;
	int monst_lev;
};


/* determines terrain composition. seperated from gen_wilderness_aux for bleed functions.*/
void init_terrain(terrain_type *t_ptr, int radius)
{
	/* not many terrain types have evil trees */
	t_ptr->eviltree = t_ptr->mud = 0;

	switch (t_ptr->type)
	{
		/* wasteland */
		case WILD_WASTELAND:
		{
			t_ptr->grass = rand_int(100);
			t_ptr->tree = 0;
			t_ptr->water = 0;
			t_ptr->dwelling = 0;
			t_ptr->eviltree = rand_int(4);
			t_ptr->hotspot = rand_int(15) + 4;
			t_ptr->monst_lev = 20 + (radius / 2); break;
			break;
		}
		/*  dense forest */
		case WILD_DENSEFOREST:
		{
			t_ptr->grass = rand_int(100)+850;
			t_ptr->tree = rand_int(150)+600;
			t_ptr->eviltree = rand_int(10)+5;
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

char terrain_spot(terrain_type * terrain)
{
	char feat;
	
	feat = FEAT_DIRT;

	if (rand_int(1000) < terrain->grass) feat = FEAT_GRASS;
	if (rand_int(1000) < terrain->tree) feat = FEAT_TREE;
	if (rand_int(1000) < terrain->eviltree) feat = FEAT_EVIL_TREE;
	if (rand_int(1000) < terrain->water) feat = FEAT_WATER;
	if (rand_int(1000) < terrain->mud) feat = FEAT_MUD;
	return feat;
}


/* adds an island in a lake, or a clearing in a forest, or a glade in a plain.
   done to make the levels a bit more interesting. 
   
   chopiness defines the randomness of the circular shape.
   
   */   
/* XXX -- I should make this use the new terrain structure, and terrain_spot. */

static void wild_add_hotspot(int Depth)
{
	int x_cen,y_cen, max_mag, magnitude, magsqr, chopiness, x, y;
	terrain_type hot_terrain;
	bool add_dwelling = FALSE;
	wilderness_type *w_ptr=&wild_info[Depth];
	
	magnitude = 0;
	/* set the terrain features to 0 by default */	
	memset(&hot_terrain,0,sizeof(terrain_type));
	
	/* hack -- minimum hotspot radius of 3 */
	while (magnitude < 3)
	{
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
	switch (wild_info[Depth].type)
	{
		case WILD_GRASSLAND:
			/* sometimes a pond */
			if (rand_int(100) < 50) 
			{
				hot_terrain.water = 1000;
			}
			/* otherwise a glade */
			else
			{
				hot_terrain.grass = rand_int(200) + 850;
				hot_terrain.tree = rand_int(600) + 300;
			}
			break;
		case WILD_FOREST:
			/* sometimes a pond */
			if (rand_int(100) < 60)
			{
				hot_terrain.water = 1000;
			}
			/* otherwise a clearing */
			else
			{
				hot_terrain.grass = rand_int(150)+900;
				hot_terrain.tree = rand_int(6)-3;
				
				/* if a large clearing, maybe a house */
				if (magnitude > 8)
				{
					if (rand_int(100) < 25) add_dwelling = TRUE;
				}
			}
			break;
		case WILD_DENSEFOREST:
			/* 80% chance of being nothing */
			if (rand_int(100) < 80)
			{
			}
			/* otherwise 70% chance of being a pond */
			else if (rand_int(100) < 70)
			{
				hot_terrain.water = 1000;
			}
			/* otherwise a rare clearing */
			else
			{
				hot_terrain.tree = rand_int(30)+7;
				
				/* sometimes a dwelling. wood-elves? */
				if (magnitude > 8)
				{
					if (rand_int(100) < 50) add_dwelling = TRUE;
				}
			}
			break;
		case WILD_SWAMP:
			/* sometimes a pond */
			if (rand_int(100) < 40)
			{
				hot_terrain.water = 1000;
			}
			/* otherwise a mud pit */
			else
			{				
				hot_terrain.type = WILD_SWAMP;
				init_terrain(&hot_terrain, w_ptr->radius);
				hot_terrain.mud = rand_int(150) + 700;
			}
			break;				
			
		case WILD_LAKE:
			/* island */
			hot_terrain.type = WILD_GRASSLAND;
			init_terrain(&hot_terrain, w_ptr->radius);
			break;
			
		default: hot_terrain.eviltree = rand_int(800)+100;
	}
	
	/* create the hotspot */
	for (y = y_cen - magnitude; y <= y_cen + magnitude; y++)
	{
		for (x = x_cen - magnitude; x <= x_cen + magnitude; x++)
		{
			/* a^2 + b^2 = c^2... the rand makes the edge less defined */
			/* HACK -- multiply the y's by 4 to "squash" the shape */
			if (((x - x_cen) * (x - x_cen)) + (((y - y_cen) * (y - y_cen))*4) < magsqr + rand_int(chopiness))
			{
				cave[Depth][y][x].feat = terrain_spot(&hot_terrain);
			}
		}
	}
	
	/* add inhabitants */
	if (add_dwelling) wild_add_dwelling(Depth, x_cen, y_cen );
		
}


/* helper function to wild_gen_bleedmap */
void wild_gen_bleedmap_aux(int *bleedmap, int span, char dir)
{
	int c = 0, above, below, noise_mag, rand_noise, bleedmag;
	
	/* make a pass of the bleedmap */
	while (c < MAX_WID)
	{
		/* check that its clear */
		if (bleedmap[c] == 0xFFFF)
		{
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
void wild_gen_bleedmap(int *bleedmap, char dir, int start, int end)
{
	int c = 0, bound;
	
	/* initialize the bleedmap */
	for (c = 0; c <= 256; c++)
	{
		bleedmap[c] = 0xFFFF;
	}

	/* initialize the "top" and "bottom" */
	if (start < 0) bleedmap[0] = rand_int(((dir%2) ? 70 : 25));
	else bleedmap[0] = start;
	if (end < 0) bleedmap[256] = rand_int(((dir%2) ? 70 : 25));	
	else
	{
		bound = (dir%2) ? MAX_HGT-3 : MAX_WID-3;
		for (c = bound; c <= 256; c++) bleedmap[c] = end;
	}
	
	/* hack -- if the start and end are zeroed, add something in the middle
	   to make exciting stuff happen. */
	if ((!start) && (!end))
	{
		/* east or west */
		if (dir%2) bleedmap[32] = rand_int(40) + 15;
		/* north or south */
		else 
		{
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
	for (c = 8; c < bound - 8; c++) 
	{
		if (bleedmap[c] < 8) bleedmap[c] = rand_int(3) + 8;
	}	
	/* middle to end */
	for (c = bound - 8; c < bound; c++)
	{
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

void wild_bleed_level(int bleed_to, int bleed_from, char dir, int start, int end)
{
	int x, y, c;
	int bleedmap[256+1], bleed_begin[MAX_WID], bleed_end[MAX_WID];
	terrain_type terrain;
	
	/* sanity check */
	if (wild_info[bleed_from].type == wild_info[bleed_to].type) return;
	
	/* initiliaze the terrain type */
	terrain.type = wild_info[bleed_from].type;
	
	/* determine the terrain components */
	init_terrain(&terrain,-1);

	/* generate the bleedmap */	
	wild_gen_bleedmap(bleedmap, dir, start, end);

	/* initialize the bleedruns */
	switch (dir)
	{
		case DIR_EAST:
			for (y = 1; y < MAX_HGT-1; y++) 
			{
				bleed_begin[y] = MAX_WID - bleedmap[y];
				bleed_end[y] = MAX_WID - 1;
			}
			break;
		case DIR_WEST:
			for (y = 1; y < MAX_HGT-1; y++) 
			{
				bleed_begin[y] = 1;
				bleed_end[y] = bleedmap[y];
			}
			break;
		case DIR_NORTH:
			for (x = 1; x < MAX_WID-1; x++) 
			{
				bleed_begin[x] = 1;
				bleed_end[x] = bleedmap[x];
			}	
			break;
		case DIR_SOUTH:
			for (x = 1; x < MAX_WID-1; x++) 
			{
				bleed_begin[x] = MAX_HGT - bleedmap[x];
				bleed_end[x] = MAX_HGT - 1;
			}	
			break;			
	}
	
	if ((dir == DIR_EAST) || (dir == DIR_WEST))
	{	
		for (y = 1; y < MAX_HGT-1; y++)
		{
			for (x = bleed_begin[y]; x < bleed_end[y]; x++)
			{
				cave_type *c_ptr = &cave[bleed_to][y][x];
				c_ptr->feat = terrain_spot(&terrain);								
			}
		}	
	}
	else
	{	
		for (x = 1; x < MAX_WID-1; x++)
		{
			for (y = bleed_begin[x]; y < bleed_end[x]; y++)
			{
				cave_type *c_ptr = &cave[bleed_to][y][x];
				c_ptr->feat = terrain_spot(&terrain);								
			}
		}	
	}
}

/* determines whether or not to bleed from a given depth in a given direction.
   useful for initial determination, as well as shared bleed points.
*/   
bool should_we_bleed(int Depth, char dir)
{
	int neigh_idx = 0, tmp;
	wilderness_type *w_ptr = &wild_info[Depth];	
	
	/* get our neighbors index */
	neigh_idx = neighbor_index(Depth, dir);
	
	/* determine whether to bleed or not */
	/* if a valid location */
	if ((neigh_idx > -MAX_WILD) && (neigh_idx < 0))
	{	
		/* make sure the level type is defined */
		wild_info[neigh_idx].type = determine_wilderness_type(neigh_idx);
	
		/* check if our neighbor is of a different type */
		if (wild_info[Depth].type != wild_info[neigh_idx].type)
		{
			/* determine whether to bleed or not */
			Rand_value = seed_town + (Depth + neigh_idx) * (93754);
			tmp = rand_int(2);
			if (tmp && (Depth < neigh_idx)) return TRUE;
			else if (!tmp && (Depth > neigh_idx)) return TRUE;
			else return FALSE;
		}
		else return FALSE;
	}
	else return FALSE;
}


/* to determine whether we bleed into our neighbor or whether our neighbor
   bleeds into us, we seed the random number generator with our combined
   depth.  If the resulting number is 0, we bleed into the greater (negative
   wise) level.  Other wise we bleed into the lesser (negative wise) level.
   
   I added in shared points.... turning this function into something extremly
   gross. This will be extremly anoying to get working. I wish I had a simpler
   way of doing this.
   
*/
   
void bleed_with_neighbors(int Depth)
{
	int c, d, neigh_idx[4], tmp, side[2], start, end, opposite;
	wilderness_type *w_ptr = &wild_info[Depth];
	bool do_bleed[4], bleed_zero[4];
	int share_point[4][2]; 
	int old_seed = Rand_value;
	bool rand_old = Rand_quick;

	/* Hack -- Use the "simple" RNG */
	Rand_quick = TRUE;
	
	/* get our neighbors indices */
	for (c = 0; c < 4; c++) neigh_idx[c] = neighbor_index(Depth,c);
	
	/* for each neighbor, determine whether to bleed or not */
	for (c = 0; c < 4; c++) do_bleed[c] = should_we_bleed(Depth,c);
	
	/* calculate the bleed_zero values */
	for (c = 0; c < 4; c++)
	{
		tmp = c-1; if (tmp < 0) tmp = 3;
		
		if ((neigh_idx[tmp] > -MAX_WILD) && (neigh_idx[tmp] < 0) && (neigh_idx[c] > -MAX_WILD) && (neigh_idx[c] < 0) )
		{
			if (wild_info[neigh_idx[tmp]].type == wild_info[neigh_idx[c]].type) 
			{
				/* calculate special case bleed zero values. */
			
				if (do_bleed[c])
				{
					/* if get the opposite direction from tmp */
					opposite = tmp - 2; if (opposite < 0) opposite += 4;
				
					/* if the other one is bleeding towards us */
					if (should_we_bleed(neigh_idx[tmp], opposite)) bleed_zero[c] = TRUE;
					else bleed_zero[c] = FALSE;	
				
				}
				else if (do_bleed[tmp])
				{
					/* get the opposite direction from c */
					opposite = c - 2; if (opposite < 0) opposite += 4;
				
					/* if the other one is bleeding towards us */
					if (should_we_bleed(neigh_idx[c], opposite)) bleed_zero[c] = TRUE;
					else bleed_zero[c] = FALSE;				
				}
				
				else bleed_zero[c] = FALSE;
			}
			else bleed_zero[c] = TRUE;
		}
		else bleed_zero[c] = FALSE;
	}
	
	
	/* calculate bleed shared points */
	for (c = 0; c < 4; c++)
	{
		side[0] = c - 1; if (side[0] < 0) side[0] = 3;
		side[1] = c + 1; if (side[1] > 3) side[1] = 0;
		
		/* if this direction is bleeding */
		if (do_bleed[c])
		{
			/* for the left and right sides */
			for (d = 0; d <= 1; d++)
			{
				/* if we have a valid neighbor */
				if ((neigh_idx[side[d]] < 0) && (neigh_idx[side[d]] > -MAX_WILD))
				{
					/* if our neighbor is bleeding in a simmilar way */
					if (should_we_bleed(neigh_idx[side[d]],c))
					{
						/* are we a simmilar type of terrain */
						if (wild_info[neigh_idx[side[d]]].type == w_ptr->type)
						{
							/* share a point */
							/* seed the number generator */
							Rand_value = seed_town + (Depth + neigh_idx[side[d]]) * (89791);
							share_point[c][d] = rand_int(((c%2) ? 70 : 25));
						}
						else share_point[c][d] = 0;
					}
					else share_point[c][d] = 0;
				}
				else share_point[c][d] = 0;
			}
		}
		else 
		{
			share_point[c][0] = 0; 
			share_point[c][1] = 0;
		}
	}	
	
	/* do the bleeds */
	for (c = 0; c < 4; c++)
	{
		tmp = c+1; if (tmp > 3) tmp = 0;
		if (do_bleed[c])
		{
			
			if ((!share_point[c][0]) && (!bleed_zero[c])) start = -1;
			else if (share_point[c][0]) start = share_point[c][0];
			else start = 0;
			
			if ((!share_point[c][1]) && (!bleed_zero[tmp])) end = -1;
			else if (share_point[c][1]) end = share_point[c][1];
			else end = 0;
			
			if (c < 2)
			{
			
				wild_bleed_level(Depth, neigh_idx[c], c, start, end);
			}
			else 
			{
				wild_bleed_level(Depth, neigh_idx[c], c, end, start);
			}
		}
	}   
		
	/* hack -- restore the random number generator */	
	Rand_value = old_seed;
	Rand_quick = rand_old;
}

static void wilderness_gen_hack(int Depth)
{
	int y, x, x1, x2, y1, y2;
	terrain_type terrain;
	bool rand_old = Rand_quick;

	wilderness_type *w_ptr = &wild_info[Depth];

	/* Hack -- Use the "simple" RNG */
	Rand_quick = TRUE;

	/* Hack -- Induce consistant wilderness */
	Rand_value = seed_town + Depth * 600;


	/* if not already set, determine the type of terrain */
	if (w_ptr->type == WILD_UNDEFINED) w_ptr->type = determine_wilderness_type(Depth);


	/* initialize the terrain */
	terrain.type = w_ptr->type;	
	init_terrain(&terrain,w_ptr->radius);	
	
	/* hack -- set the monster level */
	monster_level = terrain.monst_lev;	
			
	/* Hack -- Start with basic floors */
	for (y = 1; y < MAX_HGT - 1; y++)
	{
		for (x = 1; x < MAX_WID - 1; x++)
		{
			cave_type *c_ptr = &cave[Depth][y][x];
			c_ptr->feat = terrain_spot(&terrain);			
		}
	}

	/* to make the borders between wilderness levels more seamless, "bleed"
	   the levels together */
	
	bleed_with_neighbors(Depth); 

	/* hack -- reseed, just to make sure everything stays consistent. */

	Rand_value = seed_town + Depth * 287 + 490836;

	/* to make the level more interesting, add some "hotspots" */
	for (y = 0; y < terrain.hotspot; y++) wild_add_hotspot(Depth);
	   
	/* HACK -- if close to the town, make dwellings more likely */
#ifdef DEVEL_TOWN_COMPATIBILITY
	if (w_ptr->radius == 1) terrain.dwelling *= 21;
	if (w_ptr->radius == 2) terrain.dwelling *= 9;
	if (w_ptr->radius == 3) terrain.dwelling *= 3;
#else
	if (w_ptr->radius == 1) terrain.dwelling *= 100;
	if (w_ptr->radius == 2) terrain.dwelling *= 20;
	if (w_ptr->radius == 3) terrain.dwelling *= 3;
#endif
	      

#ifndef DEVEL_TOWN_COMPATIBILITY
	/* Hack -- 50% of the time on a radius 1 level there will be a "park" which will make
	 * the rest of the level more densly packed together */
	if ((w_ptr->radius == 1) && !rand_int(2))
	{
		reserve_building_plot(Depth, &x1,&y1, &x2,&y2, rand_int(30)+15, rand_int(20)+10, -1, -1);
	}
#endif
		
	   
	/* add wilderness dwellings */
	/* hack -- the number of dwellings is proportional to their chance of existing */
	while (terrain.dwelling > 0)
	{
		if (rand_int(1000) < terrain.dwelling)
		{
			wild_add_dwelling(Depth, -1, -1);
		}
		terrain.dwelling -= 50;
	}		
	
	/* Hack -- use the "complex" RNG */
	Rand_quick = rand_old;
		
	/* Hack -- reattach existing objects to the map */
	setup_objects();
	/* Hack -- reattach existing monsters to the map */
	setup_monsters();
}




/* Generates a wilderness level. */
          
void wilderness_gen(int Depth)
{
	int        i, y, x;
	cave_type *c_ptr;
	wilderness_type *w_ptr = &wild_info[Depth];

	/* Perma-walls -- North/South*/
	for (x = 0; x < MAX_WID; x++)
	{
		/* North wall */
		c_ptr = &cave[Depth][0][x];

		/* Clear previous contents, add "clear" perma-wall */
		c_ptr->feat = FEAT_PERM_CLEAR;

		/* South wall */
		c_ptr = &cave[Depth][MAX_HGT-1][x];

		/* Clear previous contents, add "clear" perma-wall */
		c_ptr->feat = FEAT_PERM_CLEAR;

		/* Illuminate and memorize the walls 
		c_ptr->info |= (CAVE_GLOW);*/
	}

	/* Perma-walls -- West/East */
	for (y = 0; y < MAX_HGT; y++)
	{
		/* West wall */
		c_ptr = &cave[Depth][y][0];

		/* Clear previous contents, add "clear" perma-wall */
		c_ptr->feat = FEAT_PERM_CLEAR;

		/* Illuminate and memorize the walls
		c_ptr->info |= (CAVE_GLOW);*/

		/* East wall */
		c_ptr = &cave[Depth][y][MAX_WID-1];

		/* Clear previous contents, add "clear" perma-wall */
		c_ptr->feat = FEAT_PERM_CLEAR;

		/* Illuminate and memorize the walls 
		c_ptr->info |= (CAVE_GLOW);*/
	}
	

	/* Hack -- Build some wilderness (from memory) */
	wilderness_gen_hack(Depth);


	/* Day Light */
	
	if (IS_DAY) 
	{
		wild_apply_day(Depth);	

		/* Make some day-time residents */
		if (!(w_ptr->flags & WILD_F_INHABITED))
			for (i = 0; i < wild_info[Depth].type; i++) wild_add_monster(Depth);
		
	}

	/* Night Time */
	else
	{
		/* Make some night-time residents */
		
		if (!(w_ptr->flags & WILD_F_INHABITED))
			for (i = 0; i < wild_info[Depth].type; i++) wild_add_monster(Depth);
		
	}
	
	/* Set if we have generated the level before, to determine
	   whether or not to respawn objects and monsters */
	w_ptr->flags |= (WILD_F_GENERATED | WILD_F_INHABITED);
	
}
