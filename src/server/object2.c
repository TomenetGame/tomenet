/* $Id$ */
/* File: object.c */

/* Purpose: misc code for objects */

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
 * Allow to use client option auto_inscribe?
 */
// DGDGDGDG -- na, dont let tehm be lazy
// Alas, I'm too lazy.. see '/at'	- Jir -
#define AUTO_INSCRIBER


/*
 * Excise a dungeon object from any stacks
 * Borrowed from ToME.
 */
void excise_object_idx(int o_idx)
{
	object_type *j_ptr, *o_ptr;

	u16b this_o_idx, next_o_idx = 0;

	u16b prev_o_idx = 0;


	/* Object */
	j_ptr = &o_list[o_idx];

#ifdef MONSTER_INVENTORY
	/* Monster */
	if (j_ptr->held_m_idx)
	{
		monster_type *m_ptr;

		/* Monster */
		m_ptr = &m_list[j_ptr->held_m_idx];

		/* Scan all objects the monster has */
		for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
		{
			/* Acquire object */
			o_ptr = &o_list[this_o_idx];

			/* Acquire next object */
			next_o_idx = o_ptr->next_o_idx;

			/* Done */
			if (this_o_idx == o_idx)
			{
				/* No previous */
				if (prev_o_idx == 0)
				{
					/* Remove from list */
					m_ptr->hold_o_idx = next_o_idx;
				}

				/* Real previous */
				else
				{
					object_type *k_ptr;

					/* Previous object */
					k_ptr = &o_list[prev_o_idx];

					/* Remove from list */
					k_ptr->next_o_idx = next_o_idx;
				}

				/* Forget next pointer */
				o_ptr->next_o_idx = 0;

				/* Done */
				break;
			}

			/* Save prev_o_idx */
			prev_o_idx = this_o_idx;
		}
		return;
	}
	else
#endif	// MONSTER_INVENTORY

	/* Dungeon */
	{
		cave_type *c_ptr;
		cave_type **zcave;

		int y = j_ptr->iy;
		int x = j_ptr->ix;

		/* Grid */
		if (!(zcave=getcave(&j_ptr->wpos))) return;

		/* Somewhere out of this world */
		if (!in_bounds2(&j_ptr->wpos, y, x)) return;
		
		c_ptr=&zcave[y][x];

		/* Scan all objects in the grid */
		for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
		{
			/* Acquire object */
			o_ptr = &o_list[this_o_idx];

			/* Acquire next object */
			next_o_idx = o_ptr->next_o_idx;

			/* Done */
			if (this_o_idx == o_idx)
			{
				/* No previous */
				if (prev_o_idx == 0)
				{
					/* Remove from list */
					if (c_ptr) c_ptr->o_idx = next_o_idx;
				}

				/* Real previous */
				else
				{
					object_type *k_ptr;

					/* Previous object */
					k_ptr = &o_list[prev_o_idx];

					/* Remove from list */
					k_ptr->next_o_idx = next_o_idx;
				}

				/* Forget next pointer */
				o_ptr->next_o_idx = 0;

				/* Done */
				break;
			}

			/* Save prev_o_idx */
			prev_o_idx = this_o_idx;
		}
	}
}



/*
 * Delete a dungeon object
 */
void delete_object_idx(int o_idx, bool unfound_art)
{
	object_type *o_ptr = &o_list[o_idx];
	int i;

	int y = o_ptr->iy;
	int x = o_ptr->ix;
	//cave_type **zcave;
	struct worldpos *wpos=&o_ptr->wpos;

	//cave_type *c_ptr;

	/* Artifact becomes 'not found' status */
	if (true_artifact_p(o_ptr) && unfound_art)
	{
		 a_info[o_ptr->name1].cur_num = 0;
		 a_info[o_ptr->name1].known = FALSE;
	}

	/* Excise */
	excise_object_idx(o_idx);

#if 0
	/* Object is gone */
	if((zcave=getcave(wpos)))
	{
		c_ptr=&zcave[y][x];
		c_ptr->o_idx = 0;
	}
#endif	// 0

	/* No one can see it anymore */
	for (i = 1; i < NumPlayers + 1; i++)
	{
		Players[i]->obj_vis[o_idx] = FALSE;
	}

	/* Visual update */
	/* Dungeon floor */
	if (!(o_ptr->held_m_idx)) everyone_lite_spot(wpos, y, x);

	/* Wipe the object */
	WIPE(o_ptr, object_type);
}

/*
 * Deletes object from given location
 */
void delete_object(struct worldpos *wpos, int y, int x, bool unfound_art) /* maybe */
{
	cave_type *c_ptr;

	cave_type **zcave;
	/* Refuse "illegal" locations */
	if (!in_bounds(y, x)) return;

	if((zcave=getcave(wpos)))
	{
		u16b this_o_idx, next_o_idx = 0;

		c_ptr=&zcave[y][x];
#if 0
	/* Refuse "illegal" locations */
	if (!in_bounds(Depth, y, x)) return;

	if(cave[Depth]){	/* This is fast indexing method first */
		/* Find where it was */
		c_ptr = &cave[Depth][y][x];
#endif	// 0

		/* Delete the object */
//		if (c_ptr->o_idx) delete_object_idx(c_ptr->o_idx);

		/* Scan all objects in the grid */
		for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
		{
			object_type *o_ptr;

			/* Acquire object */
			o_ptr = &o_list[this_o_idx];

			/* Acquire next object */
			next_o_idx = o_ptr->next_o_idx;

			/* Wipe the object */
			delete_object_idx(this_o_idx, unfound_art);
		}

		/* Objects are gone */
		c_ptr->o_idx = 0;

//		everyone_lite_spot(wpos, y, x);
	}
	else{			/* Cave depth not static (houses etc) - do slow method */
		int i;
		for(i=0;i<o_max;i++){
			object_type *o_ptr = &o_list[i];
			if(o_ptr->k_idx && inarea(wpos, &o_ptr->wpos))
			{
				if(y==o_ptr->iy && x==o_ptr->ix){
					delete_object_idx(i, unfound_art);
				}
			}
		}
	}
}



/*
 * Compact and Reorder the object list
 *
 * This function can be very dangerous, use with caution!
 *
 * When actually "compacting" objects, we base the saving throw on a
 * combination of object level, distance from player, and current
 * "desperation".
 *
 * After "compacting" (if needed), we "reorder" the objects into a more
 * compact order, and we reset the allocation info, and the "live" array.
 */
void compact_objects(int size, bool purge)
{
	int i, j, y, x, num, cnt, Ind; // , ny, nx;

	int cur_val, cur_lev, cur_dis, chance;
	struct worldpos *wpos;
	cave_type **zcave;
	object_type *q_ptr;
	cave_type *c_ptr;


	/* Compact */
	if (size)
	{
		/* Message */
		s_printf("Compacting objects...\n");

		/* Redraw map */
		/*p_ptr->redraw |= (PR_MAP);*/

		/* Window stuff */
		/*p_ptr->window |= (PW_OVERHEAD);*/
	}


	/* Compact at least 'size' objects */
	for (num = 0, cnt = 1; num < size; cnt++)
	{
		/* Get more vicious each iteration */
		cur_lev = 5 * cnt;

		/* Destroy more valuable items each iteration */
		cur_val = 500 * (cnt - 1);

		/* Get closer each iteration */
		cur_dis = 5 * (20 - cnt);

		/* Examine the objects */
		for (i = 1; i < o_max; i++)
		{
			object_type *o_ptr = &o_list[i];

			object_kind *k_ptr = &k_info[o_ptr->k_idx];

			/* Skip dead objects */
			if (!o_ptr->k_idx) continue;

			/* Hack -- High level objects start out "immune" */
			if (k_ptr->level > cur_lev) continue;

			/* Get the location */
			y = o_ptr->iy;
			x = o_ptr->ix;

			/* Nearby objects start out "immune" */
			/*if ((cur_dis > 0) && (distance(py, px, y, x) < cur_dis)) continue;*/

			/* Valuable objects start out "immune" */
			if (object_value(0, &o_list[i]) > cur_val) continue;

			/* Saving throw */
			chance = 90;

			/* Hack -- only compact artifacts in emergencies */
			if (artifact_p(o_ptr) && (cnt < 1000)) chance = 100;

			/* Hack -- only compact items in houses in emergencies */
			if (!o_ptr->wpos.wz)
			{
				cave_type **zcave;
				zcave=getcave(&o_ptr->wpos);
				if(zcave[y][x].info&CAVE_ICKY)
//			if (!o_ptr->dun_depth && (cave[0][y][x].info & CAVE_ICKY))
				{
					/* Grant immunity except in emergencies */
					if (cnt < 1000) chance = 100;
				}
			}

			/* Apply the saving throw */
			if (rand_int(100) < chance) continue;

			/* Delete it */
			delete_object_idx(i, TRUE);

			/* Count it */
			num++;
		}
	}


	/* Excise dead objects (backwards!) */
	for (i = o_max - 1; i >= 1; i--)
	{
		/* Get the i'th object */
		object_type *o_ptr = &o_list[i];

		/* Skip real objects */
		/* real objects in unreal location are not skipped. */
		/* hack -- items on wilderness are preserved, since
		 * they can be house contents. */
#if 0
		if (o_ptr->k_idx &&
			((!o_ptr->wpos.wz && (!purge || o_ptr->owner)) ||
			 getcave(&o_ptr->wpos))) continue;
#endif	// 0
		if (o_ptr->k_idx)
		{
			if ((!o_ptr->wpos.wz && (!purge || o_ptr->owner)) ||
					getcave(&o_ptr->wpos)) continue;

			/* Delete it first */
			delete_object_idx(i, TRUE);
		}


		/* One less object */
		o_max--;

		/* Reorder */
		if (i != o_max)
		{
#if 0	// not used
			ny = o_list[o_max].iy;
			nx = o_list[o_max].ix;
#endif	// 0
			wpos=&o_list[o_max].wpos;

			/* Update the cave */
#if 0
			/* Hack -- with wilderness objects, sometimes the cave is not allocated,
			   so check that it is. */
			if ((zcave=getcave(wpos))){
				zcave[ny][nx].o_idx = i;
			}
#endif	// 0

			/* Repair objects */
			for (j = 1; j < o_max; j++)
			{
				/* Acquire object */
				q_ptr = &o_list[j];

				/* Skip "dead" objects */
				if (!q_ptr->k_idx) continue;

				/* Repair "next" pointers */
				if (q_ptr->next_o_idx == o_max)
				{
					/* Repair */
					q_ptr->next_o_idx = i;
				}
			}

			/* Acquire object */
			o_ptr = &o_list[o_max];

#ifdef MONSTER_INVENTORY
			/* Monster */
			if (o_ptr->held_m_idx)
			{
				monster_type *m_ptr;

				/* Acquire monster */
				m_ptr = &m_list[o_ptr->held_m_idx];

				/* Repair monster */
				if (m_ptr->hold_o_idx == o_max)
				{
					/* Repair */
					m_ptr->hold_o_idx = i;
				}
			}

			/* Dungeon */
			else
#endif	// MONSTER_INVENTORY
			{
				/* Acquire location */
				y = o_ptr->iy;
				x = o_ptr->ix;

				/* Acquire grid */
				if ((zcave=getcave(wpos))){
					if (in_bounds2(wpos, y, x)){
						c_ptr=&zcave[y][x];
						//			zcave[ny][nx].o_idx = i;

						/* Repair grid */
						if (c_ptr->o_idx == o_max)
						{
							/* Repair */
							c_ptr->o_idx = i;
						}
					}
					/* Hack -- monster trap maybe */
					else
					{
						y = 255 - y;
						if (in_bounds2(wpos, y, x)){
							c_ptr=&zcave[y][x];
							if (c_ptr->feat == FEAT_MON_TRAP)
							{
								struct c_special *cs_ptr;
								if((cs_ptr=GetCS(c_ptr, CS_MON_TRAP))){
									if (cs_ptr->sc.montrap.trap_kit == o_max)
									{
										cs_ptr->sc.montrap.trap_kit = i;
									}
									
								}
							}
							else
							{
								s_printf("Strange located item detected(dropped)!\n");
								o_ptr->iy = y;
//								o_ptr->ix = x;
							}
						}
						else s_printf("Utterly out-of-bound item detected!\n");
					}
				}
			}

			/* Structure copy */
			o_list[i] = o_list[o_max];

			/* Copy the visiblity flags for each player */
			for (Ind = 1; Ind < NumPlayers + 1; Ind++)
			{
				if (Players[Ind]->conn == NOT_CONNECTED) continue;

				Players[Ind]->obj_vis[i] = Players[Ind]->obj_vis[o_max];
			}

			/* Wipe the hole */
			WIPE(&o_list[o_max], object_type);
		}
	}

	/* Reset "o_nxt" */
	o_nxt = o_max;


	/* Reset "o_top" */
	o_top = 0;

	/* Collect "live" objects */
	for (i = 0; i < o_max; i++)
	{
		/* Collect indexes */
		o_fast[o_top++] = i;
	}
}


/*
 * Delete all the items when player leaves the level
 *
 * Note -- we do NOT visually reflect these (irrelevant) changes
 */
 
void wipe_o_list(struct worldpos *wpos)
{
	int i;
	cave_type **zcave;
	monster_type *m_ptr;
	bool flag = FALSE;

	if((zcave=getcave(wpos))) flag = TRUE;


	/* Delete the existing objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip objects not on this depth */
		if (!inarea(&o_ptr->wpos, wpos))
			continue;

		/* Mega-Hack -- preserve artifacts */
		/* Hack -- Preserve unknown artifacts */
		/* We now preserve ALL artifacts, known or not */
		if (true_artifact_p(o_ptr)/* && !object_known_p(o_ptr)*/)
		{
			/* Info */
			/* s_printf("Preserving artifact %d.\n", o_ptr->name1); */

			/* Mega-Hack -- Preserve the artifact */
			a_info[o_ptr->name1].cur_num = 0;
			a_info[o_ptr->name1].known = FALSE;
		}

#ifdef MONSTER_INVENTORY
		/* Monster */
		if (o_ptr->held_m_idx)
		{
			/* Monster */
			m_ptr = &m_list[o_ptr->held_m_idx];

			/* Hack -- see above */
			m_ptr->hold_o_idx = 0;
		}

		/* Dungeon */
		else
#endif	// MONSTER_INVENTORY
//		if (flag && in_bounds2(wpos, o_ptr->iy, o_ptr->ix))
		if (flag && in_bounds_array(o_ptr->iy, o_ptr->ix))
			zcave[o_ptr->iy][o_ptr->ix].o_idx=0;

		/* Wipe the object */
		WIPE(o_ptr, object_type);
	}

	/* Compact the object list */
	compact_objects(0, FALSE);
}


/*
 * Delete all the items, but except those in houses.  - Jir -
 *
 * Note -- we do NOT visually reflect these (irrelevant) changes
 * (cave[Depth][y][x].info & CAVE_ICKY)
 */
 
void wipe_o_list_safely(struct worldpos *wpos)
{
	int i;

	cave_type **zcave;
	monster_type *m_ptr;

	if(!(zcave=getcave(wpos))) return;

	/* Delete the existing objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip objects not on this depth */
		if(!(inarea(wpos, &o_ptr->wpos)))
			continue;

		/* Skip objects inside a house(or something) */
		if(zcave[o_ptr->iy][o_ptr->ix].info & CAVE_ICKY)
			continue;

		/* Mega-Hack -- preserve artifacts */
		/* Hack -- Preserve unknown artifacts */
		/* We now preserve ALL artifacts, known or not */
		if (artifact_p(o_ptr)/* && !object_known_p(o_ptr)*/)
		{
			/* Info */
			/* s_printf("Preserving artifact %d.\n", o_ptr->name1); */

			/* Mega-Hack -- Preserve the artifact */
			a_info[o_ptr->name1].cur_num = 0;
			a_info[o_ptr->name1].known = FALSE;
		}

#ifdef MONSTER_INVENTORY
		/* Monster */
		if (o_ptr->held_m_idx)
		{
			/* Monster */
			m_ptr = &m_list[o_ptr->held_m_idx];

			/* Hack -- see above */
			m_ptr->hold_o_idx = 0;
		}

		/* Dungeon */
		else if (in_bounds_array(o_ptr->iy, o_ptr->ix))
#endif	// MONSTER_INVENTORY
		{
			zcave[o_ptr->iy][o_ptr->ix].o_idx=0;
		}

		/* Wipe the object */
		WIPE(o_ptr, object_type);
	}

	/* Compact the object list */
	compact_objects(0, FALSE);
}


/*
 * Acquires and returns the index of a "free" object.
 *
 * This routine should almost never fail, but in case it does,
 * we must be sure to handle "failure" of this routine.
 *
 * Note that this function must maintain the special "o_fast"
 * array of pointers to "live" objects.
 */
s16b o_pop(void)
{
	int i, n, k;

	/* Initial allocation */
	if (o_max < MAX_O_IDX)
	{
		/* Get next space */
		i = o_max;

		/* Expand object array */
		o_max++;

		/* Update "o_fast" */
		o_fast[o_top++] = i;

		/* Use this object */
		return (i);
	}


	/* Check for some space */
	for (n = 1; n < MAX_O_IDX; n++)
	{
		/* Get next space */
		i = o_nxt;

		/* Advance (and wrap) the "next" pointer */
		if (++o_nxt >= MAX_O_IDX) o_nxt = 1;

		/* Skip objects in use */
		if (o_list[i].k_idx) continue;

		/* Verify space XXX XXX */
		if (o_top >= MAX_O_IDX) continue;

		/* Verify not allocated */
		for (k = 0; k < o_top; k++)
		{
			/* Hack -- Prevent errors */
			if (o_fast[k] == i) i = 0;
		}

		/* Oops XXX XXX */
		if (!i) continue;

		/* Update "o_fast" */
		o_fast[o_top++] = i;

		/* Use this object */
		return (i);
	}


	/* Warn the player */
	if (server_dungeon) s_printf("Too many objects!");

	/* Oops */
	return (0);
}



/*
 * Apply a "object restriction function" to the "object allocation table"
 */
errr get_obj_num_prep(void)
{
	int i;

	/* Get the entry */
	alloc_entry *table = alloc_kind_table;

	/* Scan the allocation table */
	for (i = 0; i < alloc_kind_size; i++)
	{
		/* Accept objects which pass the restriction, if any */
		if (!get_obj_num_hook || (*get_obj_num_hook)(table[i].index))
		{
			/* Accept this object */
			table[i].prob2 = table[i].prob1;
		}

		/* Do not use this object */
		else
		{
			/* Decline this object */
			table[i].prob2 = 0;
		}
	}

	/* Success */
	return (0);
}



/*
 * Choose an object kind that seems "appropriate" to the given level
 *
 * This function uses the "prob2" field of the "object allocation table",
 * and various local information, to calculate the "prob3" field of the
 * same table, which is then used to choose an "appropriate" object, in
 * a relatively efficient manner.
 *
 * It is (slightly) more likely to acquire an object of the given level
 * than one of a lower level.  This is done by choosing several objects
 * appropriate to the given level and keeping the "hardest" one.
 *
 * Note that if no objects are "appropriate", then this function will
 * fail, and return zero, but this should *almost* never happen.
 */
s16b get_obj_num(int level)
{
	int			i, j, p;

	int			k_idx;

	long		value, total;

	object_kind		*k_ptr;

	alloc_entry		*table = alloc_kind_table;


	/* Boost level */
	if (level > 0)
	{
		/* Occasional "boost" */
		if (rand_int(GREAT_OBJ) == 0)
		{
			/* What a bizarre calculation */
                        level = 1 + (level * MAX_DEPTH_OBJ / randint(MAX_DEPTH_OBJ));
		}
	}


	/* Reset total */
	total = 0L;

	/* Process probabilities */
	for (i = 0; i < alloc_kind_size; i++)
	{
		/* Objects are sorted by depth */
		if (table[i].level > level) break;

		/* Default */
		table[i].prob3 = 0;

		/* Access the index */
		k_idx = table[i].index;

		/* Access the actual kind */
		k_ptr = &k_info[k_idx];

		/* Hack -- prevent embedded chests */
		if (opening_chest && (k_ptr->tval == TV_CHEST)) continue;

		/* Accept */
		table[i].prob3 = table[i].prob2;

		/* Total */
		total += table[i].prob3;
	}

	/* No legal objects */
	if (total <= 0) return (0);


	/* Pick an object */
	value = rand_int(total);

	/* Find the object */
	for (i = 0; i < alloc_kind_size; i++)
	{
		/* Found the entry */
		if (value < table[i].prob3) break;

		/* Decrement */
		value = value - table[i].prob3;
	}


	/* Power boost */
	p = rand_int(100);

	/* Try for a "better" object once (50%) or twice (10%) */
	if (p < 60)
	{
		/* Save old */
		j = i;

		/* Pick a object */
		value = rand_int(total);

		/* Find the monster */
		for (i = 0; i < alloc_kind_size; i++)
		{
			/* Found the entry */
			if (value < table[i].prob3) break;

			/* Decrement */
			value = value - table[i].prob3;
		}

		/* Keep the "best" one */
		if (table[i].level < table[j].level) i = j;
	}

	/* Try for a "better" object twice (10%) */
	if (p < 10)
	{
		/* Save old */
		j = i;

		/* Pick a object */
		value = rand_int(total);

		/* Find the object */
		for (i = 0; i < alloc_kind_size; i++)
		{
			/* Found the entry */
			if (value < table[i].prob3) break;

			/* Decrement */
			value = value - table[i].prob3;
		}

		/* Keep the "best" one */
		if (table[i].level < table[j].level) i = j;
	}


	/* Result */
	return (table[i].index);
}








/*
 * Known is true when the "attributes" of an object are "known".
 * These include tohit, todam, toac, cost, and pval (charges).
 *
 * Note that "knowing" an object gives you everything that an "awareness"
 * gives you, and much more.  In fact, the player is always "aware" of any
 * item of which he has full "knowledge".
 *
 * But having full knowledge of, say, one "wand of wonder", does not, by
 * itself, give you knowledge, or even awareness, of other "wands of wonder".
 * It happens that most "identify" routines (including "buying from a shop")
 * will make the player "aware" of the object as well as fully "know" it.
 *
 * This routine also removes any inscriptions generated by "feelings".
 */
void object_known(object_type *o_ptr)
{
	/* Remove "default inscriptions" */
	if (o_ptr->note && (o_ptr->ident & ID_SENSE))
	{
		/* Access the inscription */
		cptr q = quark_str(o_ptr->note);

		/* Hack -- Remove auto-inscriptions */
		if ((streq(q, "cursed")) ||
		    (streq(q, "broken")) ||
		    (streq(q, "good")) ||
		    (streq(q, "average")) ||
		    (streq(q, "excellent")) ||
		    (streq(q, "worthless")) ||
		    (streq(q, "special")) ||
		    (streq(q, "terrible")))
		{
			/* Forget the inscription */
			o_ptr->note = 0;
		}
	}

	/* Clear the "Felt" info */
	o_ptr->ident &= ~ID_SENSE;

	/* Clear the "Empty" info */
	o_ptr->ident &= ~ID_EMPTY;

	/* Now we know about the item */
	o_ptr->ident |= ID_KNOWN;

	/* Artifact becomes 'found' status - omg it must already become
	'found' if a player picks it up! That gave headaches! */
	if (true_artifact_p(o_ptr))
	{
		 a_info[o_ptr->name1].cur_num = 1;	// parano - indeed
		 a_info[o_ptr->name1].known = TRUE;
	}

}




/*
 * The player is now aware of the effects of the given object.
 */
void object_aware(int Ind, object_type *o_ptr)
{
	/* Fully aware of the effects */
	Players[Ind]->obj_aware[o_ptr->k_idx] = TRUE;
}



/*
 * Something has been "sampled"
 */
void object_tried(int Ind, object_type *o_ptr)
{
	/* Mark it as tried (even if "aware") */
	Players[Ind]->obj_tried[o_ptr->k_idx] = TRUE;
}



/*
 * Return the "value" of an "unknown" item
 * Make a guess at the value of non-aware items
 */
static s32b object_value_base(int Ind, object_type *o_ptr)
{
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Aware item -- use template cost */
	if (Ind == 0 || object_aware_p(Ind, o_ptr)) return (k_ptr->cost);

	/* Analyze the type */
	switch (o_ptr->tval)
	{
		/* Un-aware Food */
		case TV_FOOD: return (5L);

		/* Un-aware Potions */
		case TV_POTION:
		case TV_POTION2: return (20L);

		/* Un-aware Scrolls */
		case TV_SCROLL: return (20L);

		/* Un-aware Staffs */
		case TV_STAFF: return (70L);

		/* Un-aware Wands */
		case TV_WAND: return (50L);

		/* Un-aware Rods */
		case TV_ROD: return (90L);

		/* Un-aware Rings */
		case TV_RING: return (45L);

		/* Un-aware Amulets */
		case TV_AMULET: return (45L);
	}

	/* Paranoia -- Oops */
	return (0L);
}

static void eliminate_common_ego_flags(object_type *o_ptr, u32b *f1, u32b *f2, u32b *f3, u32b *f4, u32b *f5, u32b *esp)
{
	s16b j, e_idx = o_ptr->name2;
	ego_item_type *e_ptr = &e_info[e_idx];
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Hack -- eliminate Base object powers */
	(*f1) &= ~k_ptr->flags1;
	(*f2) &= ~k_ptr->flags2;
	(*f3) &= ~k_ptr->flags3;
        (*f4) &= ~k_ptr->flags4;
        (*f5) &= ~k_ptr->flags5;
        (*esp) &= ~k_ptr->esp;

	/* Hack -- eliminate 'promised' extra powers */
	for (j = 0; j < 5; j++)
	{
		/* Rarity check */
		if (e_ptr->rar[j] > 99)
		{
			*(f1) &= ~e_ptr->flags1[j];
			*(f2) &= ~e_ptr->flags2[j];
			*(f3) &= ~e_ptr->flags3[j];
			*(f4) &= ~e_ptr->flags4[j];
			*(f5) &= ~e_ptr->flags5[j];
			*(esp) &= ~e_ptr->esp[j];
		}
	}
}

/* Return the value of the flags the object has... */
static s32b flag_cost(object_type * o_ptr, int plusses)
{
	s32b total = 0;
	u32b f1, f2, f3, f4, f5, esp, am;

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	if (f5 & TR5_TEMPORARY)
	{
		return 0;
	}
	/*
	if (f4 & TR4_CURSE_NO_DROP)
	{
		return 0;
	}
	*/

	/* Hack - This shouldn't be here, still.. */
	eliminate_common_ego_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

#if 0	// stat changes are observable w/o *ID*
	if (f1 & TR1_STR) total += (1000 * plusses);
	if (f1 & TR1_INT) total += (1000 * plusses);
	if (f1 & TR1_WIS) total += (1000 * plusses);
	if (f1 & TR1_DEX) total += (1000 * plusses);
	if (f1 & TR1_CON) total += (1000 * plusses);
	if (f1 & TR1_CHR) total += (250 * plusses);
#endif	// 0
	if (f5 & TR5_CHAOTIC) total += 10000;
	if (f1 & TR1_VAMPIRIC) total += 13000;
	if (f1 & TR1_STEALTH) total += (250 * plusses);
	if (f5 & TR5_DISARM) total += (100 * plusses);
	if (f1 & TR1_SEARCH) total += (500 * plusses);
	if (f1 & TR1_INFRA) total += (150 * plusses);
	if (f1 & TR1_TUNNEL) total += (175 * plusses);
	if ((f1 & TR1_SPEED) && (plusses > 0))
		total += (10000 + (2500 * plusses));
	if ((f1 & TR1_BLOWS) && (plusses > 0))
		total += (10000 + (2500 * plusses));
        if (f1 & TR1_MANA) total += (1000 * plusses);
        if (f1 & TR1_SPELL) total += (2000 * plusses);
	if (f1 & TR1_SLAY_ANIMAL) total += 3500;
	if (f1 & TR1_SLAY_EVIL) total += 4500;
	if (f1 & TR1_SLAY_UNDEAD) total += 3500;
	if (f1 & TR1_SLAY_DEMON) total += 3500;
	if (f1 & TR1_SLAY_ORC) total += 3000;
	if (f1 & TR1_SLAY_TROLL) total += 3500;
	if (f1 & TR1_SLAY_GIANT) total += 3500;
	if (f1 & TR1_SLAY_DRAGON) total += 3500;
        if (f5 & TR5_KILL_DEMON) total += 5500;
        if (f5 & TR5_KILL_UNDEAD) total += 5500;
	if (f1 & TR1_KILL_DRAGON) total += 5500;
	if (f1 & TR1_VORPAL) total += 5000;
	if (f1 & TR1_IMPACT) total += 5000;
	if (f1 & TR1_BRAND_POIS) total += 7500;
	if (f1 & TR1_BRAND_ACID) total += 7500;
	if (f1 & TR1_BRAND_ELEC) total += 7500;
	if (f1 & TR1_BRAND_FIRE) total += 5000;
	if (f1 & TR1_BRAND_COLD) total += 5000;
	if (f2 & TR2_SUST_STR) total += 850;
	if (f2 & TR2_SUST_INT) total += 850;
	if (f2 & TR2_SUST_WIS) total += 850;
	if (f2 & TR2_SUST_DEX) total += 850;
	if (f2 & TR2_SUST_CON) total += 850;
	if (f2 & TR2_SUST_CHR) total += 250;
        if (f5 & TR5_INVIS) total += 30000;
//        if (f5 & TR5_LIFE) total += (5000 * plusses);
        if (f1 & TR1_LIFE) total += (5000 * plusses);
	if (f2 & TR2_IM_ACID) total += 10000;
	if (f2 & TR2_IM_ELEC) total += 10000;
	if (f2 & TR2_IM_FIRE) total += 10000;
	if (f2 & TR2_IM_COLD) total += 10000;
        if (f5 & TR5_SENS_FIRE) total -= 100;
/* f6 Not yet implemented in object_flags/eliminate_common_ego_flags etc. Really needed??
        if (f6 & TR6_SENS_COLD) total -= 100;
        if (f6 & TR6_SENS_ACID) total -= 100;
        if (f6 & TR6_SENS_ELEC) total -= 100;
        if (f6 & TR6_SENS_POIS) total -= 100; */
	if (f5 & TR5_REFLECT) total += 10000;
	if (f2 & TR2_FREE_ACT) total += 4500;
	if (f2 & TR2_HOLD_LIFE) total += 8500;
	if (f2 & TR2_RES_ACID) total += 1250;
	if (f2 & TR2_RES_ELEC) total += 1250;
	if (f2 & TR2_RES_FIRE) total += 1250;
	if (f2 & TR2_RES_COLD) total += 1250;
	if (f2 & TR2_RES_POIS) total += 2500;
	if (f2 & TR2_RES_FEAR) total += 2500;
	if (f2 & TR2_RES_LITE) total += 1750;
	if (f2 & TR2_RES_DARK) total += 1750;
	if (f2 & TR2_RES_BLIND) total += 2000;
	if (f2 & TR2_RES_CONF) total += 2000;
	if (f2 & TR2_RES_SOUND) total += 2000;
	if (f2 & TR2_RES_SHARDS) total += 2000;
	if (f2 & TR2_RES_NETHER) total += 2000;
	if (f2 & TR2_RES_NEXUS) total += 2000;
	if (f2 & TR2_RES_CHAOS) total += 2000;
	if (f2 & TR2_RES_DISEN) total += 10000;
	if (f3 & TR3_SH_FIRE) total += 5000;
	if (f3 & TR3_SH_ELEC) total += 5000;
        if (f3 & TR3_DECAY) total += 0;
	if (f3 & TR3_NO_TELE) total += 2500;
	if (f3 & TR3_NO_MAGIC) total += 2500;
	if (f3 & TR3_WRAITH) total += 250000;
	if (f3 & TR3_TY_CURSE) total -= 15000;
	if (f3 & TR3_EASY_KNOW) total += 0;
	if (f3 & TR3_HIDE_TYPE) total += 0;
	if (f3 & TR3_SHOW_MODS) total += 0;
	if (f3 & TR3_INSTA_ART) total += 0;
	if (!(f4 & TR4_FUEL_LITE))
	{
        if (f3 & TR3_LITE1) total += 750;
        if (f4 & TR4_LITE2) total += 1250;
        if (f4 & TR4_LITE3) total += 2750;
		if (f3 & TR3_IGNORE_FIRE) total += 100;
	}
	if (f3 & TR3_SEE_INVIS) total += 2000;
	if (esp & ESP_ORC) total += 5000;
	if (esp & ESP_TROLL) total += 10000;
	if (esp & ESP_DRAGON) total += 20000;
	if (esp & ESP_GIANT) total += 15000;
	if (esp & ESP_DEMON) total += 20000;
	if (esp & ESP_UNDEAD) total += 20000;
	if (esp & ESP_EVIL) total += 50000;
	if (esp & ESP_ANIMAL) total += 15000;
	if (esp & ESP_DRAGONRIDER) total += 10000;
	if (esp & ESP_GOOD) total += 10000;
	if (esp & ESP_NONLIVING) total += 20000;
	if (esp & ESP_UNIQUE) total += 20000;
	if (esp & ESP_SPIDER) total += 10000;
//        if (esp) total += (12500 * count_bits(esp));
        if (esp & ESP_ALL) total += 125000;
	if (f3 & TR3_SLOW_DIGEST) total += 750;
	if (f3 & TR3_REGEN) total += 2500;
	if (f5 & TR5_REGEN_MANA) total += 2500;
	if (f3 & TR3_XTRA_MIGHT) total += 2250;
	if (f3 & TR3_XTRA_SHOTS) total += 10000;
	if (f3 & TR5_IGNORE_WATER) total += 0;
	if (f3 & TR3_IGNORE_ACID) total += 100;
	if (f3 & TR3_IGNORE_ELEC) total += 100;
	if (f3 & TR3_IGNORE_COLD) total += 100;
	if (f3 & TR3_ACTIVATE) total += 100;
	if (f3 & TR3_DRAIN_EXP) total -= 12500;
	if (f3 & TR3_TELEPORT)
	{
		if (o_ptr->ident & ID_CURSED)
			total -= 7500;
		else
			total += 250;
	}
	if (f3 & TR3_AGGRAVATE) total -= 10000;
	if (f3 & TR3_BLESSED) total += 750;
	if (f3 & TR3_CURSED) total -= 5000;
	if (f3 & TR3_HEAVY_CURSE) total -= 12500;
	if (f3 & TR3_PERMA_CURSE) total -= 15000;
	if (f3 & TR3_FEATHER) total += 1250;

	if (f4 & TR4_FLY) total += 10000;
	if (f4 & TR4_NEVER_BLOW) total -= 15000;
	if (f4 & TR4_PRECOGNITION) total += 250000;
	if (f4 & TR4_BLACK_BREATH) total -= 12500;
	if (f4 & TR4_DG_CURSE) total -= 25000;
	if (f4 & TR4_CLONE) total -= 10000;
	//        if (f4 & TR4_LEVELS) total += o_ptr->elevel * 2000;

	am = ((f4 & (TR4_ANTIMAGIC_50)) ? 5 : 0)
		+ ((f4 & (TR4_ANTIMAGIC_30)) ? 3 : 0)
		+ ((f4 & (TR4_ANTIMAGIC_20)) ? 2 : 0)
		+ ((f4 & (TR4_ANTIMAGIC_10)) ? 1 : 0)
		+ ((o_ptr->tval == TV_SWORD && o_ptr->sval == SV_DARK_SWORD) ? -5 : 0);
	if (am > 0) total += PRICE_BOOST(am, 1, 1)* 2000L;

	/* Also, give some extra for activatable powers... */

#if 0
	if ((o_ptr->art_name) && (o_ptr->art_flags3 & (TR3_ACTIVATE)))
	{
		int type = o_ptr->xtra2;

		if (type == ACT_SUNLIGHT) total += 250;
		else if (type == ACT_BO_MISS_1) total += 250;
		else if (type == ACT_BA_POIS_1) total += 300;
		else if (type == ACT_BO_ELEC_1) total += 250;
		else if (type == ACT_BO_ACID_1) total += 250;
		else if (type == ACT_BO_COLD_1) total += 250;
		else if (type == ACT_BO_FIRE_1) total += 250;
		else if (type == ACT_BA_COLD_1) total += 750;
		else if (type == ACT_BA_FIRE_1) total += 1000;
		else if (type == ACT_DRAIN_1) total += 500;
		else if (type == ACT_BA_COLD_2) total += 1250;
		else if (type == ACT_BA_ELEC_2) total += 1500;
		else if (type == ACT_DRAIN_2) total += 750;
		else if (type == ACT_VAMPIRE_1) total = 1000;
		else if (type == ACT_BO_MISS_2) total += 1000;
		else if (type == ACT_BA_FIRE_2) total += 1750;
		else if (type == ACT_BA_COLD_3) total += 2500;
		else if (type == ACT_BA_ELEC_3) total += 2500;
		else if (type == ACT_WHIRLWIND) total += 7500;
		else if (type == ACT_VAMPIRE_2) total += 2500;
		else if (type == ACT_CALL_CHAOS) total += 5000;
		else if (type == ACT_ROCKET) total += 5000;
		else if (type == ACT_DISP_EVIL) total += 4000;
		else if (type == ACT_DISP_GOOD) total += 3500;
		else if (type == ACT_BA_MISS_3) total += 5000;
		else if (type == ACT_CONFUSE) total += 500;
		else if (type == ACT_SLEEP) total += 750;
		else if (type == ACT_QUAKE) total += 600;
		else if (type == ACT_TERROR) total += 2500;
		else if (type == ACT_TELE_AWAY) total += 2000;
		else if (type == ACT_GENOCIDE) total += 10000;
		else if (type == ACT_MASS_GENO) total += 10000;
		else if (type == ACT_CHARM_ANIMAL) total += 7500;
		else if (type == ACT_CHARM_UNDEAD) total += 10000;
		else if (type == ACT_CHARM_OTHER) total += 10000;
		else if (type == ACT_CHARM_ANIMALS) total += 12500;
		else if (type == ACT_CHARM_OTHERS) total += 17500;
		else if (type == ACT_SUMMON_ANIMAL) total += 10000;
		else if (type == ACT_SUMMON_PHANTOM) total += 12000;
		else if (type == ACT_SUMMON_ELEMENTAL) total += 15000;
		else if (type == ACT_SUMMON_DEMON) total += 20000;
		else if (type == ACT_SUMMON_UNDEAD) total += 20000;
		else if (type == ACT_CURE_LW) total += 500;
		else if (type == ACT_CURE_MW) total += 750;
		else if (type == ACT_REST_LIFE) total += 7500;
		else if (type == ACT_REST_ALL) total += 15000;
		else if (type == ACT_CURE_700) total += 10000;
		else if (type == ACT_CURE_1000) total += 15000;
		else if (type == ACT_ESP) total += 1500;
		else if (type == ACT_BERSERK) total += 800;
		else if (type == ACT_PROT_EVIL) total += 5000;
		else if (type == ACT_RESIST_ALL) total += 5000;
		else if (type == ACT_SPEED) total += 15000;
		else if (type == ACT_XTRA_SPEED) total += 25000;
		else if (type == ACT_WRAITH) total += 25000;
		else if (type == ACT_INVULN) total += 25000;
		else if (type == ACT_LIGHT) total += 150;
		else if (type == ACT_MAP_LIGHT) total += 500;
		else if (type == ACT_DETECT_ALL) total += 1000;
		else if (type == ACT_DETECT_XTRA) total += 12500;
		else if (type == ACT_ID_FULL) total += 10000;
		else if (type == ACT_ID_PLAIN) total += 1250;
		else if (type == ACT_RUNE_EXPLO) total += 4000;
		else if (type == ACT_RUNE_PROT) total += 10000;
		else if (type == ACT_SATIATE) total += 2000;
		else if (type == ACT_DEST_DOOR) total += 100;
		else if (type == ACT_STONE_MUD) total += 1000;
		else if (type == ACT_RECHARGE) total += 1000;
		else if (type == ACT_ALCHEMY) total += 10000;
		else if (type == ACT_DIM_DOOR) total += 10000;
		else if (type == ACT_TELEPORT) total += 2000;
		else if (type == ACT_RECALL) total += 7500;
	}
#endif	// 0

	/* Hack -- ammos shouldn't be that expensive */
	if (o_ptr->tval == TV_ARROW ||
		o_ptr->tval == TV_SHOT ||
		o_ptr->tval == TV_BOLT)
		total >>= 2;

	return total;
}


/*
 * Return the "real" price of a "known" item, not including discounts
 *
 * Wand and staffs get cost for each charge
 *
 * Armor is worth an extra 100 gold per bonus point to armor class.
 *
 * Weapons are worth an extra 100 gold per bonus point (AC,TH,TD).
 *
 * Missiles are only worth 5 gold per bonus point, since they
 * usually appear in groups of 20, and we want the player to get
 * the same amount of cash for any "equivalent" item.  Note that
 * missiles never have any of the "pval" flags, and in fact, they
 * only have a few of the available flags, primarily of the "slay"
 * and "brand" and "ignore" variety.
 *
 * Armor with a negative armor bonus is worthless.
 * Weapons with negative hit+damage bonuses are worthless.
 *
 * Every wearable item with a "pval" bonus is worth extra (see below).
 */
/*
 * Now Arrows can explode, so the pval counts.		- Jir -
 *
 * pval brings exponensial price boost, so that =int+6 is *much*
 * more expensive than =int+2.
 * Probably, it's not formula job but that of table..?
 *
 * XXX: 'Ego randarts' are not handled correltly, so be careful!
 */
s32b object_value_real(int Ind, object_type *o_ptr)
{
	u32b f1, f2, f3, f4, f5, esp, i;

	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	bool star = (Ind == 0 || object_fully_known_p(Ind, o_ptr));

	/* Base cost */
	s32b value = k_ptr->cost;

	/* Hack -- "worthless" items */
	if (!value) return (0L);


	/* Extract some flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);


	/* Artifact */
	if (o_ptr->name1)
	{
		artifact_type *a_ptr;
	
	 	/* Randarts */
		if (o_ptr->name1 == ART_RANDART)
		{
			a_ptr = randart_make(o_ptr);
			value = a_ptr->cost;

			if (value && !star) value = (object_value_base(Ind, o_ptr) << 3) + 15000;
			if (value > a_ptr->cost) value = a_ptr->cost;
		}
		else
		{	
			a_ptr = &a_info[o_ptr->name1];
			value = a_ptr->cost;
			
			/* Let true arts' prices be totally determined in a_info.txt */
			return(value);
		}

		/* Hack -- "worthless" artifacts */
		if (!value) return (0L);

		/* Hack -- Use the artifact cost instead */
//		value = a_ptr->cost;
	}

	else
	{
		/* Ego-Item */
		if (o_ptr->name2)
		{
			ego_item_type *e_ptr = &e_info[o_ptr->name2];

			/* Hack -- "worthless" ego-items */
			if (!e_ptr->cost) return (0L);

			/* Hack -- Reward the ego-item with a bonus */
			value += e_ptr->cost;

			/* Hope this won't cause inflation.. */
			if (star) value += flag_cost(o_ptr, o_ptr->pval);


			if (o_ptr->name2b)
			{
				ego_item_type *e_ptr = &e_info[o_ptr->name2b];

				/* Hack -- "worthless" ego-items */
				if (!e_ptr->cost) return (0L);

				/* Hack -- Reward the ego-item with a bonus */
				value += e_ptr->cost;
			}
		}
	}

	/* Hack */
	if ((f4 & TR4_CURSE_NO_DROP) || (f3 & TR3_AUTO_CURSE))
	{
		return 0;
	}

	/* Analyze pval bonus */
	switch (o_ptr->tval)
	{
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_BOW:
		case TV_BOOMERANG:
		case TV_AXE:
		case TV_MSTAFF:
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		case TV_LITE:
		case TV_AMULET:
		case TV_RING:
		case TV_TRAPKIT:
		{
			/* they should be of bpval.. hopefully. */
			int pval = o_ptr->bpval;
//			int boost = 1 << pval;

			/* Hack -- Negative "pval" is always bad */
//			if (pval < 0) return (0L);

			for (i = 0; i < 2; i++)
			{
				int count = 0;

				/* No pval */
//				if (!pval)
				if (pval <= 0)
				{
					pval = o_ptr->pval;
					continue;
				}

				/* Give credit for stat bonuses */
				//			if (f1 & TR1_STR) value += (pval * 200L);
				//			if (f1 & TR1_STR) value += (boost * 200L);
				if (f1 & TR1_STR) count++;
				if (f1 & TR1_INT) count++;
				if (f1 & TR1_WIS) count++;
				if (f1 & TR1_DEX) count++;
				if (f1 & TR1_CON) count++;
				if (f1 & TR1_CHR) count++;

				if (count) value += count * PRICE_BOOST((count + pval), 2, 1)* 200L;

				if (f5 & (TR5_CRIT)) value += (PRICE_BOOST(pval, 0, 1)* 500L);
//				if (f5 & (TR5_LUCK)) value += (PRICE_BOOST(pval, 0, 1)* 500L);

				/* Give credit for stealth and searching */
//				if (f1 & TR1_STEALTH) value += (PRICE_BOOST(pval, 3, 1) * 100L);
				if (f1 & TR1_STEALTH) value += pval * pval * 100L;
				if (f1 & TR1_SEARCH) value += pval * pval * 200L;
				if (f5 & TR5_DISARM) value += pval * pval * 100L;

				/* Give credit for infra-vision and tunneling */
				if (f1 & TR1_INFRA) value += pval * pval * 100L;
				if (f1 & TR1_TUNNEL) value += pval * pval * 50L;

				/* Give credit for extra attacks */
				if (f1 & TR1_BLOWS) value += (PRICE_BOOST(pval, 0, 1) * 3000L);

				/* Give credit for extra casting */
				if (f1 & TR1_SPELL) value += (PRICE_BOOST(pval, 0, 1) * 4000L);

				/* Hack -- amulets of speed and rings of speed are
				 * cheaper than other items of speed.
				 */
				if (o_ptr->tval == TV_AMULET)
				{
					/* Give credit for speed bonus */
					//				if (f1 & TR1_SPEED) value += (boost * 25000L);
					if (f1 & TR1_SPEED) value += pval * pval * 5000L;
				}
				else if (o_ptr->tval == TV_RING)
				{
					/* Give credit for speed bonus */
					//				if (f1 & TR1_SPEED) value += (PRICE_BOOST(pval, 0, 4) * 50000L);
					if (f1 & TR1_SPEED) value += pval * pval * 10000L;
				}
				//			else if (f1 & TR1_SPEED) value += (PRICE_BOOST(pval, 0, 4) * 100000L);
				else if (f1 & TR1_SPEED) value += pval * pval * 20000L;

				pval = o_ptr->pval;

				if (o_ptr->name2)
				{
					artifact_type *a_ptr;

					a_ptr =	ego_make(o_ptr);
					f1 &= ~(k_ptr->flags1 & TR1_PVAL_MASK & ~a_ptr->flags1);
					f5 &= ~(k_ptr->flags5 & TR5_PVAL_MASK & ~a_ptr->flags5);
				}
			}
			break;
		}
	}


	/* Analyze the item */
	switch (o_ptr->tval)
	{
		case TV_BOOK:
		{
			if (o_ptr->sval == 255)
			{
				/* Pay extra for the spell */
				value = value * school_spells[o_ptr->pval].skill_level;
			}
			/* Done */
			break;
                }

		/* Wands/Staffs */
		case TV_WAND:
		{
			/* Pay extra for charges */
			value += ((value / 20) * o_ptr->pval) / o_ptr->number;

			/* Done */
			break;
		}
		case TV_STAFF:
		{
			/* Pay extra for charges */
			value += ((value / 20) * o_ptr->pval);

			/* Done */
			break;
		}

		/* Rings/Amulets */
		case TV_RING:
		case TV_AMULET:
		{
			/* Hack -- negative bonuses are bad */
			if (o_ptr->to_a < 0) return (0L);
			if (o_ptr->to_h < 0) return (0L);
			if (o_ptr->to_d < 0) return (0L);

			if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH))
			{
				value += r_info[o_ptr->pval].level * r_info[o_ptr->pval].mexp;
			}

			/* Give credit for bonuses */
//			value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);
			value += ((PRICE_BOOST(o_ptr->to_h, 12, 4) + PRICE_BOOST(o_ptr->to_d, 7, 3) + PRICE_BOOST(o_ptr->to_a, 11, 4)) * 100L);

			/* Done */
			break;
		}

		/* Armor */
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_CLOAK:
		case TV_CROWN:
		case TV_HELM:
		case TV_SHIELD:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		{
			/* Hack -- negative armor bonus */
			if (o_ptr->to_a < 0) return (0L);

			/* Give credit for bonuses */
//			value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);
			value += ((PRICE_BOOST(o_ptr->to_h, 9, 4) + PRICE_BOOST(o_ptr->to_d, 9, 4) + PRICE_BOOST(o_ptr->to_a, 9, 4)) * 100L);

			/* Done */
			break;
		}

		/* Bows/Weapons */
		case TV_BOW:
		case TV_BOOMERANG:
		case TV_AXE:
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_SWORD:
		case TV_POLEARM:
		case TV_MSTAFF:
		case TV_TRAPKIT:
		{
			/* Hack -- negative hit/damage bonuses */
			if (o_ptr->to_h + o_ptr->to_d < 0)
			{
				/* Hack -- negative hit/damage are of no importance */
				if (o_ptr->tval == TV_MSTAFF) break;
				if (o_ptr->name2 == EGO_STAR_DF) break;
				else return (0L);
			}

			/* Factor in the bonuses */
//			value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);
			value += ((PRICE_BOOST(o_ptr->to_h, 9, 4) + PRICE_BOOST(o_ptr->to_d, 9, 4) + PRICE_BOOST(o_ptr->to_a, 9, 4)) * 100L);

			/* Hack -- Factor in extra damage dice */
			if ((o_ptr->dd > k_ptr->dd) && (o_ptr->ds == k_ptr->ds))
			{
				value += PRICE_BOOST((o_ptr->dd - k_ptr->dd), 1, 4) * o_ptr->ds * 100L;
			}

			/* Done */
			break;
		}

		/* Ammo */
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		{
			/* Hack -- negative hit/damage bonuses */
			if (o_ptr->to_h + o_ptr->to_d < 0) return (0L);

			/* Factor in the bonuses */
//			value += ((o_ptr->to_h + o_ptr->to_d) * 5L);
			value += ((PRICE_BOOST(o_ptr->to_h, 9, 4) + PRICE_BOOST(o_ptr->to_d, 9, 4)) * 5L);

			/* Hack -- Factor in extra damage dice */
			if ((o_ptr->dd > k_ptr->dd) && (o_ptr->ds == k_ptr->ds))
			{
				value += (o_ptr->dd - k_ptr->dd) * o_ptr->ds * 5L;
			}

			/* Special attack (exploding arrow) */
			if (o_ptr->pval != 0) value *= 14;

			/* Done */
			break;
		}
	}


	/* Return the value */
	return (value);
}


/*
 * Return the price of an item including plusses (and charges)
 *
 * This function returns the "value" of the given item (qty one)
 *
 * Never notice "unknown" bonuses or properties, including "curses",
 * since that would give the player information he did not have.
 *
 * Note that discounted items stay discounted forever, even if
 * the discount is "forgotten" by the player via memory loss.
 */
s32b object_value(int Ind, object_type *o_ptr)
{
	s32b value;


	/* Known items -- acquire the actual value */
	if (Ind == 0 || object_known_p(Ind, o_ptr))
	{
		/* Broken items -- worthless */
		if (broken_p(o_ptr)) return (0L);

		/* Cursed items -- worthless */
		if (cursed_p(o_ptr)) return (0L);

		/* Real value (see above) */
		value = object_value_real(Ind, o_ptr);
	}

	/* Unknown items -- acquire a base value */
	else
	{
		/* Hack -- Felt broken items */
		if ((o_ptr->ident & ID_SENSE) && broken_p(o_ptr)) return (0L);

		/* Hack -- Felt cursed items */
		if ((o_ptr->ident & ID_SENSE) && cursed_p(o_ptr)) return (0L);

		/* Base value (see above) */
		value = object_value_base(Ind, o_ptr);
	}


	/* Apply discount (if any) */
	if (o_ptr->discount) value -= (value * o_ptr->discount / 100L);


	/* Return the final value */
	return (value);
}





/*
 * Determine if an item can "absorb" a second item
 *
 * See "object_absorb()" for the actual "absorption" code.
 *
 * If permitted, we allow wands/staffs (if they are known to have equal
 * charges) and rods (if fully charged) to combine.
 *
 * Note that rods/staffs/wands are then unstacked when they are used.
 *
 * If permitted, we allow weapons/armor to stack, if they both known.
 *
 * Food, potions, scrolls, and "easy know" items always stack.
 *
 * Chests never stack (for various reasons).
 *
 * We do NOT allow activatable items (artifacts or dragon scale mail)
 * to stack, to keep the "activation" code clean.  Artifacts may stack,
 * but only with another identical artifact (which does not exist).
 *
 * Ego items may stack as long as they have the same ego-item type.
 * This is primarily to allow ego-missiles to stack.
 *
 * o_ptr and j_ptr no longer are simmetric;
 * j_ptr should be the new item or level-reqs gets meanless.
 */
bool object_similar(int Ind, object_type *o_ptr, object_type *j_ptr)
{
	player_type *p_ptr = NULL;
	int total = o_ptr->number + j_ptr->number;


	/* Hack -- gold always merge */
//	if (o_ptr->tval == TV_GOLD && j_ptr->tval == TV_GOLD) return(TRUE);

	/* Require identical object types */
	if (o_ptr->k_idx != j_ptr->k_idx) return (0);

		/* Require same owner or convertable to same owner */
//
/*		if (o_ptr->owner != j_ptr->owner) return (0); */
	if (Ind)
	{
		p_ptr = Players[Ind];
		if (((o_ptr->owner != j_ptr->owner)
			&& ((p_ptr->lev < j_ptr->level)
			|| (j_ptr->level < 1)))
			&& (j_ptr->owner)) return (0);
		if ((o_ptr->owner != p_ptr->id)
			&& (o_ptr->owner != j_ptr->owner)) return (0);
	}
	else if (o_ptr->owner != j_ptr->owner) return (0);

	/* Analyze the items */
	switch (o_ptr->tval)
	{
		/* Chests */
	case TV_KEY:
		case TV_CHEST:
		{
			/* Never okay */
			return (0);
		}

		/* Food and Potions and Scrolls */
		case TV_FOOD:
		case TV_POTION:
		case TV_POTION2:
		case TV_SCROLL:
		{
			/* Hack for ego foods :) */
			if (o_ptr->name2 != j_ptr->name2) return (0);
			if (o_ptr->name2b != j_ptr->name2b) return (0);

			/* Assume okay */
			break;
		}

		/* Staffs and Wands */
		case TV_WAND:
		{
			/* Require either knowledge or known empty for both wands. */
			if ((!(o_ptr->ident & (ID_EMPTY)) && 
				!object_known_p(Ind, o_ptr)) || 
				(!(j_ptr->ident & (ID_EMPTY)) && 
				!object_known_p(Ind, j_ptr))) return(0);

			/* Beware artifatcs should not combine with "lesser" thing */
			if (o_ptr->name1 != j_ptr->name1) return (0);

			/* Do not combine recharged ones with non recharged ones. */
//			if ((f4 & TR4_RECHARGED) != (f14 & TR4_RECHARGED)) return (0);

			/* Do not combine different ego or normal ones */
			if (o_ptr->name2 != j_ptr->name2) return (0);

			/* Do not combine different ego or normal ones */
			if (o_ptr->name2b != j_ptr->name2b) return (0);

			/* Assume okay */
			break;
		}

		case TV_STAFF:
		{
			/* Require knowledge */
			if (!Ind || !object_known_p(Ind, o_ptr) || !object_known_p(Ind, j_ptr)) return (0);

			/* Fall through */
		}

		/* Staffs and Wands and Rods */
		case TV_ROD:
		{
			/* Require permission */
			if (!Ind || !p_ptr->stack_allow_wands) return (0);

			/* Require identical charges */
			if (o_ptr->pval != j_ptr->pval) return (0);
			
			if (o_ptr->name2 != j_ptr->name2) return (0);
			if (o_ptr->name2b != j_ptr->name2b) return (0);

			/* Probably okay */
			break;
		}

		/* Weapons and Armor */
		case TV_BOW:
		case TV_BOOMERANG:
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_AXE:
		case TV_MSTAFF:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		{
			/* Require permission */
			if (!Ind || !p_ptr->stack_allow_items) return (0);

			/* XXX XXX XXX Require identical "sense" status */
			/* if ((o_ptr->ident & ID_SENSE) != */
			/*     (j_ptr->ident & ID_SENSE)) return (0); */

			/* Fall through */
		}

		/* Rings, Amulets, Lites */
		case TV_RING:
		case TV_AMULET:
		case TV_LITE:
		case TV_TOOL:
		case TV_BOOK:	/* Books can be 'fireproof' */
		{
			/* Require full knowledge of both items */
			if (!Ind || !object_known_p(Ind, o_ptr) ||
//					!object_known_p(Ind, j_ptr) || (o_ptr->name3)) return (0);
					!object_known_p(Ind, j_ptr)) return (FALSE);
			if (o_ptr->bpval != j_ptr->bpval) return(FALSE);

			/* Fall through */
		}

		/* Missiles */
		case TV_BOLT:
		case TV_ARROW:
		case TV_SHOT:
		{
			/* Require identical "bonuses" */
			if (o_ptr->to_h != j_ptr->to_h) return (FALSE);
			if (o_ptr->to_d != j_ptr->to_d) return (FALSE);
			if (o_ptr->to_a != j_ptr->to_a) return (FALSE);

			/* Require identical "pval" code */
			if (o_ptr->pval != j_ptr->pval) return (FALSE);

			/* Require identical "artifact" names */
			if (o_ptr->name1 != j_ptr->name1) return (FALSE);

			/* Require identical "ego-item" names */
			if (o_ptr->name2 != j_ptr->name2) return (FALSE);
			if (o_ptr->name2b != j_ptr->name2b) return (FALSE);

			/* Require identical random seeds */
			if (o_ptr->name3 != j_ptr->name3) return (FALSE);

			/* Hack -- Never stack "powerful" items */
//			if (o_ptr->xtra1 || j_ptr->xtra1) return (FALSE);

			/* Hack -- Never stack recharging items */
			if (o_ptr->timeout != j_ptr->timeout) return (FALSE);
#if 0
			if (o_ptr->timeout || j_ptr->timeout)
			{
				if ((o_ptr->timeout != j_ptr->timeout) ||
					(o_ptr->tval != TV_LITE)) return (FALSE);
			}
#endif	// 0

			/* Require identical "values" */
			if (o_ptr->ac != j_ptr->ac) return (FALSE);
			if (o_ptr->dd != j_ptr->dd) return (FALSE);
			if (o_ptr->ds != j_ptr->ds) return (FALSE);

			/* Probably okay */
			break;
		}
		case TV_GOLEM:
			if (o_ptr->pval != j_ptr->pval) return(FALSE);
			break;

		/* Various */
		default:
		{
			/* Require knowledge */
			if (Ind && (!object_known_p(Ind, o_ptr) ||
					!object_known_p(Ind, j_ptr))) return (0);

			/* Probably okay */
			break;
		}
	}


	/* Hack -- Require identical "cursed" status */
	if ((o_ptr->ident & ID_CURSED) != (j_ptr->ident & ID_CURSED)) return (0);

	/* Hack -- Require identical "broken" status */
	if ((o_ptr->ident & ID_BROKEN) != (j_ptr->ident & ID_BROKEN)) return (0);


	/* Hack -- require semi-matching "inscriptions" */
	/* Hack^2 -- books do merge.. it's to prevent some crashes */
	if (o_ptr->note && j_ptr->note && (o_ptr->note != j_ptr->note)
		&& strcmp(quark_str(o_ptr->note), "on sale")
		&& strcmp(quark_str(j_ptr->note), "on sale")
		&& !is_book(o_ptr)) return (0);

	/* Hack -- normally require matching "inscriptions" */
	if ((!Ind || !p_ptr->stack_force_notes) && (o_ptr->note != j_ptr->note)) return (0);

	/* Hack -- normally require matching "discounts" */
	if ((!Ind || !p_ptr->stack_force_costs) && (o_ptr->discount != j_ptr->discount)) return (0);


	/* Maximal "stacking" limit */
	if (total >= MAX_STACK_SIZE) return (0);


	/* They match, so they must be similar */
	return (TRUE);
}


/*
 * Allow one item to "absorb" another, assuming they are similar
 */
void object_absorb(int Ind, object_type *o_ptr, object_type *j_ptr)
{
	int total = o_ptr->number + j_ptr->number;

	/* Add together the item counts */
	o_ptr->number = ((total < MAX_STACK_SIZE) ? total : (MAX_STACK_SIZE - 1));

	/* Hack -- blend "known" status */
	if (Ind && object_known_p(Ind, j_ptr)) object_known(o_ptr);

	/* Hack -- blend "rumour" status */
	if (j_ptr->ident & ID_RUMOUR) o_ptr->ident |= ID_RUMOUR;

	/* Hack -- blend "mental" status */
	if (j_ptr->ident & ID_MENTAL) o_ptr->ident |= ID_MENTAL;

	/* Hack -- blend "inscriptions" */
//	if (j_ptr->note) o_ptr->note = j_ptr->note;
//	if (o_ptr->note) j_ptr->note = o_ptr->note;
	if (!o_ptr->note && j_ptr->note) o_ptr->note = j_ptr->note;

	/* Hack -- could average discounts XXX XXX XXX */
	/* Hack -- save largest discount XXX XXX XXX */
	if (o_ptr->discount < j_ptr->discount) o_ptr->discount = j_ptr->discount;

	/* blend level-req into lower one */
	if (o_ptr->level > j_ptr->level) o_ptr->level = j_ptr->level;

	/* Hack -- if wands are stacking, combine the charges. -LM- */
	if (o_ptr->tval == TV_WAND)
	{
		o_ptr->pval += j_ptr->pval;
	}
}



/*
 * Find the index of the object_kind with the given tval and sval
 */
s16b lookup_kind(int tval, int sval)
{
	int k;

	/* Look for it */
	for (k = 1; k < max_k_idx; k++)
	{
		object_kind *k_ptr = &k_info[k];

		/* Found a match */
		if ((k_ptr->tval == tval) && (k_ptr->sval == sval)) return (k);
	}

	/* Oops */
#if DEBUG_LEVEL > 2
	s_printf("No object (%d,%d)", tval, sval);
#endif	// DEBUG_LEVEL

	/* Oops */
	return (0);
}


/*
 * Clear an item
 */
void invwipe(object_type *o_ptr)
{
	/* Clear the record */
	WIPE(o_ptr, object_type);
}


/*
 * Make "o_ptr" a "clean" copy of the given "kind" of object
 */
void invcopy(object_type *o_ptr, int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Clear the record */
	WIPE(o_ptr, object_type);

	/* Save the kind index */
	o_ptr->k_idx = k_idx;

	/* Efficiency -- tval/sval */
	o_ptr->tval = k_ptr->tval;
	o_ptr->sval = k_ptr->sval;

	/* Default "pval" */
//	o_ptr->pval = k_ptr->pval;
	if (o_ptr->tval == TV_POTION ||
		o_ptr->tval == TV_POTION2 ||
		o_ptr->tval == TV_FLASK ||
		o_ptr->tval == TV_FOOD)
		o_ptr->pval = k_ptr->pval;
	else if (o_ptr->tval == TV_LITE)
		o_ptr->timeout = k_ptr->pval;

	else if (o_ptr->tval != TV_ROD)
		o_ptr->bpval = k_ptr->pval;

	/* Default number */
	o_ptr->number = 1;

	/* Default weight */
	o_ptr->weight = k_ptr->weight;

	/* Default magic */
	o_ptr->to_h = k_ptr->to_h;
	o_ptr->to_d = k_ptr->to_d;
	o_ptr->to_a = k_ptr->to_a;

	/* Default power */
	o_ptr->ac = k_ptr->ac;
	o_ptr->dd = k_ptr->dd;
	o_ptr->ds = k_ptr->ds;

	/* Hack -- worthless items are always "broken" */
	if (k_ptr->cost <= 0) o_ptr->ident |= ID_BROKEN;

	/* Hack -- cursed items are always "cursed" */
	if (k_ptr->flags3 & TR3_CURSED) o_ptr->ident |= ID_CURSED;
}





/*
 * Help determine an "enchantment bonus" for an object.
 *
 * To avoid floating point but still provide a smooth distribution of bonuses,
 * we simply round the results of division in such a way as to "average" the
 * correct floating point value.
 *
 * This function has been changed.  It uses "randnor()" to choose values from
 * a normal distribution, whose mean moves from zero towards the max as the
 * level increases, and whose standard deviation is equal to 1/4 of the max,
 * and whose values are forced to lie between zero and the max, inclusive.
 *
 * Since the "level" rarely passes 100 before Morgoth is dead, it is very
 * rare to get the "full" enchantment on an object, even a deep levels.
 *
 * It is always possible (albeit unlikely) to get the "full" enchantment.
 *
 * A sample distribution of values from "m_bonus(10, N)" is shown below:
 *
 *   N       0     1     2     3     4     5     6     7     8     9    10
 * ---    ----  ----  ----  ----  ----  ----  ----  ----  ----  ----  ----
 *   0   66.37 13.01  9.73  5.47  2.89  1.31  0.72  0.26  0.12  0.09  0.03
 *   8   46.85 24.66 12.13  8.13  4.20  2.30  1.05  0.36  0.19  0.08  0.05
 *  16   30.12 27.62 18.52 10.52  6.34  3.52  1.95  0.90  0.31  0.15  0.05
 *  24   22.44 15.62 30.14 12.92  8.55  5.30  2.39  1.63  0.62  0.28  0.11
 *  32   16.23 11.43 23.01 22.31 11.19  7.18  4.46  2.13  1.20  0.45  0.41
 *  40   10.76  8.91 12.80 29.51 16.00  9.69  5.90  3.43  1.47  0.88  0.65
 *  48    7.28  6.81 10.51 18.27 27.57 11.76  7.85  4.99  2.80  1.22  0.94
 *  56    4.41  4.73  8.52 11.96 24.94 19.78 11.06  7.18  3.68  1.96  1.78
 *  64    2.81  3.07  5.65  9.17 13.01 31.57 13.70  9.30  6.04  3.04  2.64
 *  72    1.87  1.99  3.68  7.15 10.56 20.24 25.78 12.17  7.52  4.42  4.62
 *  80    1.02  1.23  2.78  4.75  8.37 12.04 27.61 18.07 10.28  6.52  7.33
 *  88    0.70  0.57  1.56  3.12  6.34 10.06 15.76 30.46 12.58  8.47 10.38
 *  96    0.27  0.60  1.25  2.28  4.30  7.60 10.77 22.52 22.51 11.37 16.53
 * 104    0.22  0.42  0.77  1.36  2.62  5.33  8.93 13.05 29.54 15.23 22.53
 * 112    0.15  0.20  0.56  0.87  2.00  3.83  6.86 10.06 17.89 27.31 30.27
 * 120    0.03  0.11  0.31  0.46  1.31  2.48  4.60  7.78 11.67 25.53 45.72
 * 128    0.02  0.01  0.13  0.33  0.83  1.41  3.24  6.17  9.57 14.22 64.07
 */
s16b m_bonus(int max, int level)
{
	int bonus, stand, extra, value;


	/* Paranoia -- enforce maximal "level" */
        if (level > MAX_DEPTH_OBJ - 1) level = MAX_DEPTH_OBJ - 1;


	/* The "bonus" moves towards the max */
        bonus = ((max * level) / MAX_DEPTH_OBJ);

	/* Hack -- determine fraction of error */
        extra = ((max * level) % MAX_DEPTH_OBJ);

	/* Hack -- simulate floating point computations */
        if (rand_int(MAX_DEPTH_OBJ) < extra) bonus++;


	/* The "stand" is equal to one quarter of the max */
	stand = (max / 4);

	/* Hack -- determine fraction of error */
	extra = (max % 4);

	/* Hack -- simulate floating point computations */
	if (rand_int(4) < extra) stand++;


	/* Choose an "interesting" value */
	value = randnor(bonus, stand);

	/* Enforce the minimum value */
	if (value < 0) return (0);

	/* Enforce the maximum value */
	if (value > max) return (max);

	/* Result */
	return (value);
}

/*
 * Mega-Hack -- Attempt to create one of the "Special Objects"
 *
 * We are only called from "place_object()", and we assume that
 * "apply_magic()" is called immediately after we return.
 *
 * Note -- see "make_artifact()" and "apply_magic()"
 */
static bool make_artifact_special(struct worldpos *wpos, object_type *o_ptr)
{
	int			i;

	int			k_idx = 0;


	/* No artifacts in the town */
	if (istown(wpos)) return (FALSE);

	/* Check the artifact list (just the "specials") */
//	for (i = 0; i < ART_MIN_NORMAL; i++)
	for (i = 0; i < MAX_A_IDX; i++)
	{
		artifact_type *a_ptr = &a_info[i];

		/* Skip "empty" artifacts */
		if (!a_ptr->name) continue;

		/* Cannot make an artifact twice */
		if (a_ptr->cur_num) continue;

                /* Cannot generate non special ones */
                if (!(a_ptr->flags3 & TR3_INSTA_ART)) continue;

                /* Cannot generate some artifacts because they can only exists in special dungeons/quests/... */
//                if ((a_ptr->flags4 & TR4_SPECIAL_GENE) && (!a_allow_special[i]) && (!vanilla_town)) continue;
                if (a_ptr->flags4 & TR4_SPECIAL_GENE) continue;

		/* XXX XXX Enforce minimum "depth" (loosely) */
		if (a_ptr->level > getlevel(wpos))
		{
			/* Acquire the "out-of-depth factor" */
			int d = (a_ptr->level - getlevel(wpos)) * 2;

			/* Roll for out-of-depth creation */
			if (rand_int(d) != 0) continue;
		}

		/* Artifact "rarity roll" */
		if (rand_int(a_ptr->rarity) != 0) return (0);

		/* Find the base object */
		k_idx = lookup_kind(a_ptr->tval, a_ptr->sval);

		/* XXX XXX Enforce minimum "object" level (loosely) */
		if (k_info[k_idx].level > object_level)
		{
			/* Acquire the "out-of-depth factor" */
			int d = (k_info[k_idx].level - object_level) * 5;

			/* Roll for out-of-depth creation */
			if (rand_int(d) != 0) continue;
		}

		/* Assign the template */
		invcopy(o_ptr, k_idx);

		/* Mega-Hack -- mark the item as an artifact */
		o_ptr->name1 = i;

		/* Hack -- Mark the artifact as "created" */
		a_ptr->cur_num = 1;

		/* Success */
		return (TRUE);
	}

	/* Failure */
	return (FALSE);
}


/*
 * Attempt to change an object into an artifact
 *
 * This routine should only be called by "apply_magic()"
 *
 * Note -- see "make_artifact_special()" and "apply_magic()"
 */
static bool make_artifact(struct worldpos *wpos, object_type *o_ptr)
{
	int i;


	/* No artifacts in the town */
	if (istown(wpos)) return (FALSE);

	/* Paranoia -- no "plural" artifacts */
	if (o_ptr->number != 1) return (FALSE);

	/* Check the artifact list (skip the "specials") */
//	for (i = ART_MIN_NORMAL; i < MAX_A_IDX; i++)
        for (i = 0; i < MAX_A_IDX; i++)
	{
		artifact_type *a_ptr = &a_info[i];

		/* Skip "empty" items */
		if (!a_ptr->name) continue;

		/* Cannot make an artifact twice */
		if (a_ptr->cur_num) continue;

                /* Cannot generate special ones */
                if (a_ptr->flags3 & TR3_INSTA_ART) continue;

                /* Cannot generate some artifacts because they can only exists in special dungeons/quests/... */
//                if ((a_ptr->flags4 & TR4_SPECIAL_GENE) && (!a_allow_special[i]) && (!vanilla_town)) continue;
                if (a_ptr->flags4 & TR4_SPECIAL_GENE) continue;

		/* Must have the correct fields */
		if (a_ptr->tval != o_ptr->tval) continue;
		if (a_ptr->sval != o_ptr->sval) continue;

		/* XXX XXX Enforce minimum "depth" (loosely) */
		if (a_ptr->level > getlevel(wpos))
		{
			/* Acquire the "out-of-depth factor" */
			int d = (a_ptr->level - getlevel(wpos)) * 2;

			/* Roll for out-of-depth creation */
			if (rand_int(d) != 0) continue;
		}

		/* We must make the "rarity roll" */
		if (rand_int(a_ptr->rarity) != 0) continue;

		/* Hack -- mark the item as an artifact */
		o_ptr->name1 = i;

		/* Hack -- Mark the artifact as "created" */
		a_ptr->cur_num = 1;

		/* Success */
		return (TRUE);
	}

	/* An extra chance at being a randart. XXX RANDART */
	if (!rand_int(RANDART_RARITY))
	{
	        /* Randart ammo should be very rare! */
	        if (((o_ptr->tval == TV_SHOT) || (o_ptr->tval == TV_ARROW) ||
	            (o_ptr->tval == TV_BOLT)) && (rand_int(100) < 95)) return(FALSE);

    		o_ptr->name1 = ART_RANDART;

		/* Piece together a 32-bit random seed */
		o_ptr->name3 = rand_int(0xFFFF) << 16;
		o_ptr->name3 += rand_int(0xFFFF);

		/* Check the tval is allowed */
		if (randart_make(o_ptr) == NULL)
		{
			/* If not, wipe seed. No randart today */
			o_ptr->name1 = 0;
			o_ptr->name3 = 0L;

			return (FALSE);
		}
		
		return (TRUE);
	}

	/* Failure */
	return (FALSE);
}


/*
 * Ported from PernAngband		- Jir -
 *
 * 'level' is not depth of dungeon but an index of item value.
 *
 * In PernMangband, ego-items use random-seed 'name3' just
 * as same as randarts.
 * (Be careful not to allow randarts ego!)
 */
/*
 * Attempt to change an object into an ego
 *
 * This routine should only be called by "apply_magic()"
 */
static bool make_ego_item(int level, object_type *o_ptr, bool good)
{
	int i = 0, j;
	int *ok_ego, ok_num = 0;
	bool ret = FALSE;

	if (artifact_p(o_ptr) || o_ptr->name2) return (FALSE);

	C_MAKE(ok_ego, MAX_E_IDX, int);

	/* Grab the ok ego */
	for (i = 0; i < MAX_E_IDX; i++)
	{
		ego_item_type *e_ptr = &e_info[i];
		bool ok = FALSE;

		/* Skip "empty" items */
		if (!e_ptr->name) continue;

		/* Must have the correct fields */
		for (j = 0; j < 6; j++)
		{
			if (e_ptr->tval[j] == o_ptr->tval)
			{
				if ((e_ptr->min_sval[j] <= o_ptr->sval) && (e_ptr->max_sval[j] >= o_ptr->sval)) ok = TRUE;
			}

			if (ok) break;
		}
		if (!ok)
		{
			/* Doesnt count as a try*/
			continue;
		}

		/* Good should be good, bad should be bad */
		if (good && (!e_ptr->cost)) continue;
		if ((!good) && e_ptr->cost) continue;

		/* ok */
		ok_ego[ok_num++] = i;
	}

	/* Now test them a few times */
//	for (i = 0; i < ok_num * 10; i++)	// I wonder.. 
	for (j = 0; j < ok_num * 10; j++)
	{
		ego_item_type *e_ptr;

		i = ok_ego[rand_int(ok_num)];
		e_ptr = &e_info[i];

		/* XXX XXX Enforce minimum "depth" (loosely) */
		if (e_ptr->level > level)
		{
			/* Acquire the "out-of-depth factor" */
			int d = (e_ptr->level - level);

			/* Roll for out-of-depth creation */
			if (rand_int(d) != 0)
			{
				continue;
			}
		}

		/* We must make the "rarity roll" */
//		if (rand_int(e_ptr->mrarity - luck(-(e_ptr->mrarity / 2), e_ptr->mrarity / 2)) > e_ptr->rarity)
#if 0
		k = e_ptr->mrarity - e_ptr->rarity;
		if (k && rand_int(k))
		{
			continue;
		}
#endif	// 0
		if (rand_int(e_ptr->mrarity) > e_ptr->rarity) continue;

		/* Hack -- mark the item as an ego */
		o_ptr->name2 = i;

		/* Piece together a 32-bit random seed */
		if (e_ptr->fego[0] & ETR4_NO_SEED) o_ptr->name3 = 0;
		else
		{
			o_ptr->name3 = rand_int(0xFFFF) << 16;
			o_ptr->name3 += rand_int(0xFFFF);
		}

		/* Success */
		ret = TRUE;
		break;
	}

	/*
	 * Sometimes(rarely) tries for a double ego
	 * Also make sure we dont already have a name2b, wchih would mean a special ego item
	 */
	/* try only when it's already ego	- Jir - */
//	if (magik(7 + luck(-7, 7)) && (!o_ptr->name2b))
	if (ret && magik(7) && (!o_ptr->name2b))
	{
		/* Now test them a few times */
		for (j = 0; j < ok_num * 10; j++)
		{
			ego_item_type *e_ptr;

			i = ok_ego[rand_int(ok_num)];
			e_ptr = &e_info[i];

			/* Cannot be a double ego of the same ego type */
			if (i == o_ptr->name2) continue;

			/* Cannot have 2 suffixes or 2 prefixes */
			if (e_info[o_ptr->name2].before && e_ptr->before) continue;
			if ((!e_info[o_ptr->name2].before) && (!e_ptr->before)) continue;

			/* XXX XXX Enforce minimum "depth" (loosely) */
			if (e_ptr->level > level)
			{
				/* Acquire the "out-of-depth factor" */
				int d = (e_ptr->level - level);

				/* Roll for out-of-depth creation */
				if (rand_int(d) != 0)
				{
					continue;
				}
			}

			/* We must make the "rarity roll" */
//			if (rand_int(e_ptr->mrarity - luck(-(e_ptr->mrarity / 2), e_ptr->mrarity / 2)) > e_ptr->rarity)
			if (rand_int(e_ptr->mrarity) > e_ptr->rarity)
			{
				continue;
			}

			/* Hack -- mark the item as an ego */
			o_ptr->name2b = i;

			/* Piece together a 32-bit random seed */
			if (!(e_ptr->fego[0] & ETR4_NO_SEED) && !o_ptr->name3)
			{
				o_ptr->name3 = rand_int(0xFFFF) << 16;
				o_ptr->name3 += rand_int(0xFFFF);
			}

			/* Success */
			ret = TRUE;
			break;
		}
	}
	C_FREE(ok_ego, MAX_E_IDX, int);

	/* Return */
	return (ret);
}


/*
 * Charge a new wand.
 */
static void charge_wand(object_type *o_ptr)
{
	switch (o_ptr->sval)
	{
#if 0 // oh, do I love thee.. the paresse!
		case SV_WAND_HEAL_MONSTER:		o_ptr->pval = randint(20) + 8; break;
		case SV_WAND_HASTE_MONSTER:		o_ptr->pval = randint(20) + 8; break;
		case SV_WAND_CLONE_MONSTER:		o_ptr->pval = randint(5)  + 3; break;
		case SV_WAND_TELEPORT_AWAY:		o_ptr->pval = randint(5)  + 6; break;
		case SV_WAND_DISARMING:			o_ptr->pval = randint(5)  + 4; break;
		case SV_WAND_TRAP_DOOR_DEST:	o_ptr->pval = randint(8)  + 6; break;
		case SV_WAND_STONE_TO_MUD:		o_ptr->pval = randint(4)  + 3; break;
		case SV_WAND_LITE:				o_ptr->pval = randint(10) + 6; break;
		case SV_WAND_SLEEP_MONSTER:		o_ptr->pval = randint(15) + 8; break;
		case SV_WAND_SLOW_MONSTER:		o_ptr->pval = randint(10) + 6; break;
		case SV_WAND_CONFUSE_MONSTER:	o_ptr->pval = randint(12) + 6; break;
		case SV_WAND_FEAR_MONSTER:		o_ptr->pval = randint(5)  + 3; break;
		case SV_WAND_DRAIN_LIFE:		o_ptr->pval = randint(3)  + 3; break;
		case SV_WAND_POLYMORPH:			o_ptr->pval = randint(8)  + 6; break;
		case SV_WAND_STINKING_CLOUD:	o_ptr->pval = randint(8)  + 6; break;
		case SV_WAND_MAGIC_MISSILE:		o_ptr->pval = randint(10) + 6; break;
		case SV_WAND_ACID_BOLT:			o_ptr->pval = randint(8)  + 6; break;
		case SV_WAND_ELEC_BOLT:			o_ptr->pval = randint(8)  + 6; break;
		case SV_WAND_FIRE_BOLT:			o_ptr->pval = randint(8)  + 6; break;
		case SV_WAND_COLD_BOLT:			o_ptr->pval = randint(5)  + 6; break;
		case SV_WAND_ACID_BALL:			o_ptr->pval = randint(5)  + 2; break;
		case SV_WAND_ELEC_BALL:			o_ptr->pval = randint(8)  + 4; break;
		case SV_WAND_FIRE_BALL:			o_ptr->pval = randint(4)  + 2; break;
		case SV_WAND_COLD_BALL:			o_ptr->pval = randint(6)  + 2; break;
		case SV_WAND_WONDER:			o_ptr->pval = randint(15) + 8; break;
		case SV_WAND_ANNIHILATION:		o_ptr->pval = randint(2)  + 1; break;
		case SV_WAND_DRAGON_FIRE:		o_ptr->pval = randint(3)  + 1; break;
		case SV_WAND_DRAGON_COLD:		o_ptr->pval = randint(3)  + 1; break;
		case SV_WAND_DRAGON_BREATH:		o_ptr->pval = randint(3)  + 1; break;
#endif	// 0
		case SV_WAND_HEAL_MONSTER:              o_ptr->pval = randint(20) + 8; break;
		case SV_WAND_HASTE_MONSTER:             o_ptr->pval = randint(20) + 8; break;
		case SV_WAND_CLONE_MONSTER:             o_ptr->pval = randint(5)  + 3; break;
		case SV_WAND_TELEPORT_AWAY:             o_ptr->pval = randint(5)  + 6; break;
		case SV_WAND_DISARMING:                 o_ptr->pval = randint(5)  + 4; break;
		case SV_WAND_TRAP_DOOR_DEST:            o_ptr->pval = randint(8)  + 6; break;
		case SV_WAND_STONE_TO_MUD:              o_ptr->pval = randint(8)  + 3; break;
		case SV_WAND_LITE:                      o_ptr->pval = randint(10) + 6; break;
		case SV_WAND_SLEEP_MONSTER:             o_ptr->pval = randint(15) + 8; break;
		case SV_WAND_SLOW_MONSTER:              o_ptr->pval = randint(10) + 6; break;
		case SV_WAND_CONFUSE_MONSTER:           o_ptr->pval = randint(12) + 6; break;
		case SV_WAND_FEAR_MONSTER:              o_ptr->pval = randint(5)  + 3; break;
		case SV_WAND_DRAIN_LIFE:                o_ptr->pval = randint(3)  + 3; break;
                case SV_WAND_WALL_CREATION:             o_ptr->pval = randint(4)  + 3; break;
		case SV_WAND_POLYMORPH:                 o_ptr->pval = randint(8)  + 6; break;
		case SV_WAND_STINKING_CLOUD:            o_ptr->pval = randint(8)  + 6; break;
		case SV_WAND_MAGIC_MISSILE:             o_ptr->pval = randint(10) + 6; break;
		case SV_WAND_ACID_BOLT:                 o_ptr->pval = randint(8)  + 6; break;
		case SV_WAND_CHARM_MONSTER:             o_ptr->pval = randint(6)  + 2; break;
		case SV_WAND_FIRE_BOLT:                 o_ptr->pval = randint(8)  + 6; break;
		case SV_WAND_COLD_BOLT:                 o_ptr->pval = randint(5)  + 6; break;
		case SV_WAND_ACID_BALL:                 o_ptr->pval = randint(5)  + 2; break;
		case SV_WAND_ELEC_BALL:                 o_ptr->pval = randint(8)  + 4; break;
		case SV_WAND_FIRE_BALL:                 o_ptr->pval = randint(4)  + 2; break;
		case SV_WAND_COLD_BALL:                 o_ptr->pval = randint(6)  + 2; break;
		case SV_WAND_WONDER:                    o_ptr->pval = randint(15) + 8; break;
		case SV_WAND_ANNIHILATION:              o_ptr->pval = randint(2)  + 1; break;
		case SV_WAND_DRAGON_FIRE:               o_ptr->pval = randint(3)  + 1; break;
		case SV_WAND_DRAGON_COLD:               o_ptr->pval = randint(3)  + 1; break;
		case SV_WAND_DRAGON_BREATH:             o_ptr->pval = randint(3)  + 1; break;
		case SV_WAND_ROCKETS:                   o_ptr->pval = randint(2)  + 1; break;

		case SV_WAND_ELEC_BOLT:			o_ptr->pval = randint(8)  + 6; break;
		case SV_WAND_TELEPORT_TO:			o_ptr->pval = randint(3)  + 3; break;
	}
}



/*
 * Charge a new staff.
 */
static void charge_staff(object_type *o_ptr)
{
	switch (o_ptr->sval)
	{
		case SV_STAFF_DARKNESS:			o_ptr->pval = randint(8)  + 8; break;
		case SV_STAFF_SLOWNESS:			o_ptr->pval = randint(8)  + 8; break;
		case SV_STAFF_HASTE_MONSTERS:	o_ptr->pval = randint(8)  + 8; break;
		case SV_STAFF_SUMMONING:		o_ptr->pval = randint(3)  + 1; break;
		case SV_STAFF_TELEPORTATION:	o_ptr->pval = randint(4)  + 5; break;
		case SV_STAFF_IDENTIFY:			o_ptr->pval = randint(15) + 5; break;
		case SV_STAFF_REMOVE_CURSE:		o_ptr->pval = randint(3)  + 4; break;
		case SV_STAFF_STARLITE:			o_ptr->pval = randint(5)  + 6; break;
		case SV_STAFF_LITE:				o_ptr->pval = randint(20) + 8; break;
		case SV_STAFF_MAPPING:			o_ptr->pval = randint(5)  + 5; break;
		case SV_STAFF_DETECT_GOLD:		o_ptr->pval = randint(20) + 8; break;
		case SV_STAFF_DETECT_ITEM:		o_ptr->pval = randint(15) + 6; break;
		case SV_STAFF_DETECT_TRAP:		o_ptr->pval = randint(5)  + 6; break;
		case SV_STAFF_DETECT_DOOR:		o_ptr->pval = randint(8)  + 6; break;
		case SV_STAFF_DETECT_INVIS:		o_ptr->pval = randint(15) + 8; break;
		case SV_STAFF_DETECT_EVIL:		o_ptr->pval = randint(15) + 8; break;
		case SV_STAFF_CURE_LIGHT:		o_ptr->pval = randint(5)  + 6; break;
		case SV_STAFF_CURING:			o_ptr->pval = randint(3)  + 4; break;
		case SV_STAFF_HEALING:			o_ptr->pval = randint(2)  + 1; break;
		case SV_STAFF_THE_MAGI:			o_ptr->pval = randint(2)  + 2; break;
		case SV_STAFF_SLEEP_MONSTERS:	o_ptr->pval = randint(5)  + 6; break;
		case SV_STAFF_SLOW_MONSTERS:	o_ptr->pval = randint(5)  + 6; break;
		case SV_STAFF_SPEED:			o_ptr->pval = randint(3)  + 4; break;
		case SV_STAFF_PROBING:			o_ptr->pval = randint(6)  + 2; break;
		case SV_STAFF_DISPEL_EVIL:		o_ptr->pval = randint(3)  + 4; break;
		case SV_STAFF_POWER:			o_ptr->pval = randint(3)  + 1; break;
		case SV_STAFF_HOLINESS:			o_ptr->pval = randint(2)  + 2; break;
		case SV_STAFF_GENOCIDE:			o_ptr->pval = randint(2)  + 1; break;
		case SV_STAFF_EARTHQUAKES:		o_ptr->pval = randint(5)  + 3; break;
		case SV_STAFF_DESTRUCTION:		o_ptr->pval = randint(3)  + 1; break;
		case SV_STAFF_STAR_IDENTIFY:	o_ptr->pval = randint(5)  + 3; break;
	}
}



/*
 * Apply magic to an item known to be a "weapon"
 *
 * Hack -- note special base damage dice boosting
 * Hack -- note special processing for weapon/digger
 * Hack -- note special rating boost for dragon scale mail
 */
static void a_m_aux_1(object_type *o_ptr, int level, int power)
{
	int tohit1 = randint(5) + m_bonus(5, level);
	int todam1 = randint(5) + m_bonus(5, level);

	int tohit2 = m_bonus(10, level);
	int todam2 = m_bonus(10, level);
	//int tries;

	artifact_bias = 0;

        /* Very good */
        if (power > 1)
        {
                /* Make ego item */
//                if (!rand_int(RANDART_WEAPON) && (o_ptr->tval != TV_TRAPKIT)) create_artifact(o_ptr, FALSE, TRUE);	else
                make_ego_item(level, o_ptr, TRUE);
        }
        else if (power < -1)
        {
                /* Make ego item */
                make_ego_item(level, o_ptr, FALSE);
        }

	/* Good */
	if (power > 0)
	{
		/* Enchant */
		o_ptr->to_h += tohit1;
		o_ptr->to_d += todam1;

		/* Very good */
		if (power > 1)
		{
			/* Enchant again */
			o_ptr->to_h += tohit2;
			o_ptr->to_d += todam2;
		}
	}

	/* Cursed */
	else if (power < 0)
	{
		/* Penalize */
		o_ptr->to_h -= tohit1;
		o_ptr->to_d -= todam1;

		/* Very cursed */
		if (power < -1)
		{
			/* Penalize again */
			o_ptr->to_h -= tohit2;
			o_ptr->to_d -= todam2;
		}

		/* Cursed (if "bad") */
		if (o_ptr->to_h + o_ptr->to_d < 0) o_ptr->ident |= ID_CURSED;
	}


	/* Some special cases */
	switch (o_ptr->tval)
	{
#if 0
		case TV_TRAPKIT:
		{
			/* Good */
			if (power > 0) o_ptr->to_a += randint(5);

			/* Very good */
			if (power > 1) o_ptr->to_a += randint(5);

			/* Bad */
			if (power < 0) o_ptr->to_a -= randint(5);

			/* Very bad */
			if (power < -1) o_ptr->to_a -= randint(5);

			break;
		}
		case TV_MSTAFF:
		{
			if (is_ego_p(o_ptr, EGO_MSTAFF_SPELL))
			{
				int gf[2], i;

				for (i = 0; i < 2; i++)
				{
					int k = 0;

					gf[i] = 0;
					while (!k)
					{                                                
						k = lookup_kind(TV_RUNE1, (gf[i] = rand_int(MAX_GF)));
					}
				}

				o_ptr->pval = gf[0] + (gf[1] << 16);
				o_ptr->pval3 = rand_int(RUNE_MOD_MAX) + (rand_int(RUNE_MOD_MAX) << 16);
				o_ptr->pval2 = randint(70) + (randint(70) << 8);
			}
			break;
		}
#endif	// 0
		case TV_BOLT:
		case TV_ARROW:
		case TV_SHOT:
		{
			if (o_ptr->sval == SV_AMMO_MAGIC)
			{
				o_ptr->to_h = o_ptr->to_d = o_ptr->pval = o_ptr->name2 = o_ptr->name3 = 0;
				break;
			}

//			if ((power == 1) && !o_ptr->name2 && o_ptr->sval != SV_AMMO_MAGIC)
			else if ((power == 1) && !o_ptr->name2)
			{
//				if (randint(100) < 7)
				if (randint(500) < level + 5)
				{
					/* Exploding missile */
					int power[27]={GF_ELEC, GF_POIS, GF_ACID,
						GF_COLD, GF_FIRE, GF_PLASMA, GF_LITE,
						GF_DARK, GF_SHARDS, GF_SOUND,
						GF_CONFUSION, GF_FORCE, GF_INERTIA,
						GF_MANA, GF_METEOR, GF_ICE, GF_CHAOS,
						GF_NETHER, GF_NEXUS, GF_TIME,
						GF_GRAVITY, GF_KILL_WALL, GF_AWAY_ALL,
						GF_TURN_ALL, GF_NUKE, GF_STUN,
						GF_DISINTEGRATE};

						//                                o_ptr->pval2 = power[rand_int(25)];
					o_ptr->pval = power[rand_int(27)];
				}
			}
			break;
		}
	}
#if 0	// heheh
	/* Analyze type */
	switch (o_ptr->tval)
	{
		case TV_DIGGING:
		{
			/* Very good */
			if (power > 1)
			{
				/* Special Ego-item */
				o_ptr->name2 = EGO_DIGGING;
			}

			/* Bad */
			else if (power < 0)
			{
				/* Hack -- Reverse digging bonus */
				o_ptr->pval = 0 - (o_ptr->pval);
			}

			/* Very bad */
			else if (power < -1)
			{
				/* Hack -- Horrible digging bonus */
				o_ptr->pval = 0 - (5 + randint(5));
			}

			break;
		}


		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		{
			/* Very Good */
			if (power > 1)
			{
			  if ((o_ptr->tval == TV_HAFTED) && (o_ptr->sval == SV_MSTAFF))
			    {
			      o_ptr->name2 = EGO_MSTAFF_POWER;
			      break;
			    }

				/* Roll for an ego-item */
				switch (randint(29))
				{
					case 1:
					o_ptr->name2 = EGO_HA;
					break;

					case 2:
					o_ptr->name2 = EGO_DF;
					break;

					case 3:
					o_ptr->name2 = EGO_BRAND_ACID;
					break;

					case 4:
					o_ptr->name2 = EGO_BRAND_ELEC;
					break;

					case 5:
					o_ptr->name2 = EGO_BRAND_FIRE;
					break;

					case 6:
					o_ptr->name2 = EGO_BRAND_COLD;
					break;

					case 7: case 8:
					o_ptr->name2 = EGO_SLAY_ANIMAL;
					if (rand_int(100) < 20)
					{
						o_ptr->name2 = EGO_KILL_ANIMAL;
					}
					break;

					case 9: case 10:
					o_ptr->name2 = EGO_SLAY_DRAGON;
					if (rand_int(100) < 20)
					{
						o_ptr->name2 = EGO_KILL_DRAGON;
					}
					break;

					case 11: case 12:
					o_ptr->name2 = EGO_SLAY_EVIL;
					if (rand_int(100) < 20)
					{
						o_ptr->name2 = EGO_KILL_EVIL;
					}
					break;

					case 13: case 14:
					o_ptr->name2 = EGO_SLAY_UNDEAD;
					if (rand_int(100) < 20)
					{
						o_ptr->name2 = EGO_KILL_UNDEAD;
					}
					break;

					case 15: case 16: case 17:
					o_ptr->name2 = EGO_SLAY_ORC;
					if (rand_int(100) < 20)
					{
						o_ptr->name2 = EGO_KILL_ORC;
					}
					break;

					case 18: case 19: case 20:
					o_ptr->name2 = EGO_SLAY_TROLL;
					if (rand_int(100) < 20)
					{
						o_ptr->name2 = EGO_KILL_TROLL;
					}
					break;

					case 21: case 22: case 23:
					o_ptr->name2 = EGO_SLAY_GIANT;
					if (rand_int(100) < 20)
					{
						o_ptr->name2 = EGO_KILL_GIANT;
					}
					break;

					case 24: case 25: case 26:
					o_ptr->name2 = EGO_SLAY_DEMON;
					if (rand_int(100) < 20)
					{
						o_ptr->name2 = EGO_KILL_DEMON;
					}
					break;

					case 27:
					o_ptr->name2 = EGO_WEST;
					break;

					case 28:
					o_ptr->name2 = EGO_BLESS_BLADE;
					break;

					case 29:
					o_ptr->name2 = EGO_ATTACKS;
					break;
				}

				/* Hack -- Super-charge the damage dice */
tries = 100;				
while ((--tries) && (rand_int(10L * o_ptr->dd * o_ptr->ds) == 0)) o_ptr->dd++;

				/* Hack -- Lower the damage dice */
				if (o_ptr->dd > 9) o_ptr->dd = 9;
			}

			/* Very cursed */
			else if (power < -1)
			{
				/* Roll for ego-item */
				if (rand_int(MAX_DEPTH_OBJ * 4) < level)
				{
					o_ptr->name2 = EGO_MORGUL;
				}
			}

			break;
		}


		case TV_BOW:
		{
			/* Very good */
			if (power > 1)
			{
				/* Roll for ego-item */
                                switch (randint(33)) 
				{
					case 1: case 2: case 3: case 4:
					{
						o_ptr->name2 = EGO_EXTRA_MIGHT;
						break;
					}

					case 5: case 6: case 7: case 8:
					{
						o_ptr->name2 = EGO_EXTRA_SHOTS;
						break;
					}

					case 9: case 10: case 11: case 12: case 13: case 14: case 15: case 16: 
					case 17: case 18:
					{
						o_ptr->name2 = EGO_VELOCITY;
						break;
					}

					case 19: case 20: case 21: case 22: case 23: case 24: case 25: case 26: 
					case 27: case 28:
					{
						o_ptr->name2 = EGO_ACCURACY;
						break;
					}

					case 29: case 30: case 31:
					{
                                                o_ptr->name2 = EGO_LORIEN;
						break;
					}

					case 32: case 33:
					{
                                                o_ptr->name2 = EGO_NUMENOR;
						break;
					}
				}
			}

			break;
		}


		case TV_BOLT:
		case TV_ARROW:
		case TV_SHOT:
		{
		  /* Mega hack -- Magic Arrows are always +0+0 */
		  if (o_ptr->sval == SV_AMMO_MAGIC) break;

			/* Very good */
			if (power > 1)
			{
				/* Roll for ego-item */
				switch (randint(10))
				{
					case 1: case 2: case 3:
					o_ptr->name2 = EGO_WOUNDING;
					break;

					case 4:
					o_ptr->name2 = EGO_FLAME;
					break;

					case 5:
					o_ptr->name2 = EGO_FROST;
					break;

					case 6: case 7:
					o_ptr->name2 = EGO_HURT_ANIMAL;
					break;

					case 8: case 9:
					o_ptr->name2 = EGO_HURT_EVIL;
					break;

					case 10:
					o_ptr->name2 = EGO_HURT_DRAGON;
					break;
				}

				/* Hack -- super-charge the damage dice */
tries = 100;
				while ((--tries) && (rand_int(10L * o_ptr->dd * o_ptr->ds) == 0)) o_ptr->dd++;

				/* Hack -- restrict the damage dice */
				if (o_ptr->dd > 9) o_ptr->dd = 9;
			}

			/* Very cursed */
			else if (power < -1)
			{
				/* Roll for ego-item */
                                if (rand_int(MAX_DEPTH_OBJ) < level)
				{
					o_ptr->name2 = EGO_BACKBITING;
				}
			}

			break;
		}
	}
#endif	// 0
}


/*
 * Apply magic to an item known to be "armor"
 *
 * Hack -- note special processing for crown/helm
 * Hack -- note special processing for robe of permanence
 */
static void a_m_aux_2(object_type *o_ptr, int level, int power)
{
	int toac1 = randint(5) + m_bonus(5, level);

	int toac2 = m_bonus(10, level);

	artifact_bias = 0;

        /* Very good */
        if (power > 1)
        {
                /* Make ego item */
//                if (!rand_int(RANDART_ARMOR)) create_artifact(o_ptr, FALSE, TRUE);	else
                make_ego_item(level, o_ptr, TRUE);
        }
        else if (power < -1)
        {
                /* Make ego item */
                make_ego_item(level, o_ptr, FALSE);
        }


	/* Good */
	if (power > 0)
	{
		/* Enchant */
		o_ptr->to_a += toac1;

		/* Very good */
		if (power > 1)
		{
			/* Enchant again */
			o_ptr->to_a += toac2;
		}
	}

	/* Cursed */
	else if (power < 0)
	{
		/* Penalize */
		o_ptr->to_a -= toac1;

		/* Very cursed */
		if (power < -1)
		{
			/* Penalize again */
			o_ptr->to_a -= toac2;
		}

		/* Cursed (if "bad") */
		if (o_ptr->to_a < 0) o_ptr->ident |= ID_CURSED;
	}

#if 1	// once..
	/* Analyze type */
	switch (o_ptr->tval)
	{
		case TV_CLOAK:
		{
			if (o_ptr->sval == SV_ELVEN_CLOAK)
                                o_ptr->bpval = randint(4);       /* No cursed elven cloaks...? */
#if 1
			/* Set the Kolla cloak's base bonuses*/
			if(o_ptr->sval == SV_KOLLA)
			{
				o_ptr->bpval = randint(2);
			}
#endif	// 0

                        break;
                }
		case TV_DRAG_ARMOR:
		{

			break;
		}
		case TV_SHIELD:
		{
			if (o_ptr->sval == SV_DRAGON_SHIELD)
			{

				/* pfft */
//				dragon_resist(o_ptr);
				break;
			}

#if 1
			/* Set the orcish shield's STR and CON bonus
			*/
			if(o_ptr->sval == SV_ORCISH_SHIELD)
			{
				o_ptr->bpval = randint(2);

				/* Cursed orcish shield */
				if (power < 0) o_ptr->bpval = -o_ptr->bpval;
				break;
			}
#endif
		}
#if 1
		case TV_BOOTS:
		{
			/* Set the Witan Boots stealth penalty */
                        if(o_ptr->sval == SV_PAIR_OF_WITAN_BOOTS)
                        {
                                o_ptr->bpval = -2;
                        }

		}
#endif	// 0
	}
#endif	// 0

#if 0	// former ones..
	/* Analyze type */
	switch (o_ptr->tval)
	{
		case TV_DRAG_ARMOR:
		{
			break;
		}


		case TV_HARD_ARMOR:
		case TV_SOFT_ARMOR:
		{
			/* Very good */
			if (power > 1)
			{
				/* Hack -- Try for "Robes of the Magi" */
				if ((o_ptr->tval == TV_SOFT_ARMOR) &&
				    (o_ptr->sval == SV_ROBE) &&
				    (rand_int(100) < 10))
				{
					o_ptr->name2 = EGO_PERMANENCE;
					break;
				}

				/* Roll for ego-item */
				switch (randint(19))
				{
					case 1: case 2: case 3: case 4:
					{
						o_ptr->name2 = EGO_RESIST_ACID;
						break;
					}

					case 5: case 6: case 7: case 8:
					{
						o_ptr->name2 = EGO_RESIST_ELEC;
						break;
					}

					case 9: case 10: case 11: case 12:
					{
						o_ptr->name2 = EGO_RESIST_FIRE;
						break;
					}

					case 13: case 14: case 15: case 16:
					{
						o_ptr->name2 = EGO_RESIST_COLD;
						break;
					}

					case 17: case 18:
					{
						o_ptr->name2 = EGO_RESISTANCE;
						break;
					}

					default:
					{
						o_ptr->name2 = EGO_ELVENKIND;
						break;
					}
				}
			}

			break;
		}


		case TV_SHIELD:
		{
			/* Set the orcish shield's STR and CON bonus
			 */
                        if(o_ptr->sval == SV_ORCISH_SHIELD)
                        {
                                o_ptr->bpval = randint(2);

				/* Cursed orcish shield */
				if (power < 0) o_ptr->bpval = -o_ptr->bpval;
                        }

			/* Very good */
			if (power > 1)
			{
				/* Roll for ego-item */
                                switch (randint(20))
				{
					case 1: case 2: case 3:
					{
						o_ptr->name2 = EGO_ENDURE_ACID;
						break;
					}

					case 4: case 5: case 6: case 7: case 8:
					{
						o_ptr->name2 = EGO_ENDURE_ELEC;
						break;
					}

					case 9: case 10: case 11: case 12:
					{
						o_ptr->name2 = EGO_ENDURE_FIRE;
						break;
					}

					case 13: case 14: case 15: case 16: case 17:
					{
						o_ptr->name2 = EGO_ENDURE_COLD;
						break;
					}

					case 18: case 19:
					{
						o_ptr->name2 = EGO_ENDURANCE;
						break;
					}

					default:
					{
                                                o_ptr->name2 = EGO_AVARI;
						break;
					}
				}
			}

			break;
		}


		case TV_GLOVES:
		{
			/* Very good */
			if (power > 1)
			{
				/* Roll for ego-item */
                                switch (randint(18))
				{
					case 1: case 2: case 3: case 4: case 5: 
					{
						o_ptr->name2 = EGO_FREE_ACTION;
						break;
					}

                                        case 6: case 7: case 8: case 9: 
					{
						o_ptr->name2 = EGO_SLAYING;
						break;
					}

                                        case 10: case 11: case 12: 
					{
						o_ptr->name2 = EGO_AGILITY;
						break;
					}

                                        case 13: case 14: 
					{
						o_ptr->name2 = EGO_POWER;
						break;
					}

                                        case 16: case 17:
					{
						o_ptr->name2 = EGO_MAGIC;
						break;
					}

                                        case 15: case 18:
					{
                                                o_ptr->name2 = EGO_ISTARI;
						break;
					}
				}
			}

			/* Very cursed */
			else if (power < -1)
			{
				/* Roll for ego-item */
				switch (randint(2))
				{
					case 1:
					{
						o_ptr->name2 = EGO_CLUMSINESS;
						break;
					}
					default:
					{
						o_ptr->name2 = EGO_WEAKNESS;
						break;
					}
				}
			}

			break;
		}


		case TV_BOOTS:
		{
			/* Set the Witan Boots stealth penalty */
                        if(o_ptr->sval == SV_PAIR_OF_WITAN_BOOTS)
                        {
                                o_ptr->bpval = -2;
                        }

			/* Very good */
			if (power > 1)
			{
				/* Roll for ego-item */
				switch (randint(24))
				{
					case 1:
					{
						o_ptr->name2 = EGO_SPEED;
						break;
					}

					case 2: case 3: case 4: case 5:
					{
						o_ptr->name2 = EGO_MOTION;
						break;
					}

					case 6: case 7: case 8: case 9:
					case 10: case 11: case 12: case 13:
					{
						o_ptr->name2 = EGO_QUIET;
						break;
					}

                         		case 14: case 15:
					{
                                                o_ptr->name2 = EGO_MIRKWOOD;
						break;
					}

					default:
					{
						o_ptr->name2 = EGO_SLOW_DESCENT;
						break;
					}
				}
			}

			/* Very cursed */
			else if (power < -1)
			{
				/* Roll for ego-item */
				switch (randint(3))
				{
					case 1:
					{
						o_ptr->name2 = EGO_NOISE;
						break;
					}
					case 2:
					{
						o_ptr->name2 = EGO_SLOWNESS;
						break;
					}
					case 3:
					{
						o_ptr->name2 = EGO_ANNOYANCE;
						break;
					}
				}
			}

			break;
		}


		case TV_CROWN:
		{
			/* Very good */
			if (power > 1)
			{
				/* Roll for ego-item */
				switch (randint(8))
				{
					case 1:
					{
						o_ptr->name2 = EGO_MAGI;
						break;
					}
					case 2:
					{
						o_ptr->name2 = EGO_MIGHT;
						break;
					}
					case 3:
					{
						o_ptr->name2 = EGO_TELEPATHY;
						break;
					}
					case 4:
					{
						o_ptr->name2 = EGO_REGENERATION;
						break;
					}
					case 5: case 6:
					{
						o_ptr->name2 = EGO_LORDLINESS;
						break;
					}
					default:
					{
						o_ptr->name2 = EGO_SEEING;
						break;
					}
				}
			}

			/* Very cursed */
			else if (power < -1)
			{
				/* Roll for ego-item */
				switch (randint(7))
				{
					case 1: case 2:
					{
						o_ptr->name2 = EGO_STUPIDITY;
						break;
					}
					case 3: case 4:
					{
						o_ptr->name2 = EGO_NAIVETY;
						break;
					}
					case 5:
					{
						o_ptr->name2 = EGO_UGLINESS;
						break;
					}
					case 6:
					{
						o_ptr->name2 = EGO_SICKLINESS;
						break;
					}
					case 7:
					{
						o_ptr->name2 = EGO_TELEPORTATION;
						break;
					}
				}
			}

			break;
		}


		case TV_HELM:
		{
			/* Very good */
			if (power > 1)
			{
				/* Roll for ego-item */
				switch (randint(14))
				{
					case 1: case 2:
					{
						o_ptr->name2 = EGO_INTELLIGENCE;
						break;
					}
					case 3: case 4:
					{
						o_ptr->name2 = EGO_WISDOM;
						break;
					}
					case 5: case 6:
					{
						o_ptr->name2 = EGO_BEAUTY;
						break;
					}
					case 7: case 8:
					{
						o_ptr->name2 = EGO_SEEING;
						break;
					}
					case 9: case 10:
					{
						o_ptr->name2 = EGO_LITE;
						break;
					}
					default:
					{
						o_ptr->name2 = EGO_INFRAVISION;
						break;
					}
				}
			}

			/* Very cursed */
			else if (power < -1)
			{
				/* Roll for ego-item */
				switch (randint(7))
				{
					case 1: case 2:
					{
						o_ptr->name2 = EGO_STUPIDITY;
						break;
					}
					case 3: case 4:
					{
						o_ptr->name2 = EGO_NAIVETY;
						break;
					}
					case 5:
					{
						o_ptr->name2 = EGO_UGLINESS;
						break;
					}
					case 6:
					{
						o_ptr->name2 = EGO_SICKLINESS;
						break;
					}
					case 7:
					{
						o_ptr->name2 = EGO_TELEPORTATION;
						break;
					}
				}
			}

			break;
		}


		case TV_CLOAK:
		{
                        /* Set the Kolla cloak's base bonuses*/
                        if(o_ptr->sval == SV_KOLLA)
                        {
                                o_ptr->bpval = randint(2);
                        }

			/* Very good */
			if (power > 1)
			{
				/* Roll for ego-item */
				switch (randint(34))
				{
					case 1: case 2: case 3: case 4: case 5:
					case 6: case 7: case 8: case 9: case 10:
					o_ptr->name2 = EGO_PROTECTION;
					break;

                                   	case 11: case 12: case 13: case 14: case 15: 
					case 16: case 17: case 18: case 19: case 20: case 21:
					o_ptr->name2 = EGO_STEALTH;
					break;

					case 22: case 23: case 24: case 25: case 26:
					o_ptr->name2 = EGO_CLOAK_RES;
					break;

					case 27: case 28: case 29: case 30:
	 				o_ptr->name2 = EGO_TELERI;
					break;

					case 31: case 32:
                                        o_ptr->name2 = EGO_CLOAK_LORDLY_RES;
					break;

					case 33: case 34:
                                        o_ptr->name2 = EGO_AMAN;
					break;
				}
			}

			/* Very cursed */
			else if (power < -1)
			{
				/* Choose some damage */
				switch (randint(3))
				{
					case 1:
					o_ptr->name2 = EGO_IRRITATION;
					break;
					case 2:
					o_ptr->name2 = EGO_VULNERABILITY;
					break;
					case 3:
					o_ptr->name2 = EGO_ENVELOPING;
					break;
				}
			}

			break;
		}
	}
#endif	// 0
}



/*
 * Apply magic to an item known to be a "ring" or "amulet"
 *
 * Hack -- note special rating boost for ring of speed
 * Hack -- note special rating boost for amulet of the magi
 * Hack -- note special "pval boost" code for ring of speed
 * Hack -- note that some items must be cursed (or blessed)
 */
static void a_m_aux_3(object_type *o_ptr, int level, int power)
{
	//int tries;
	artifact_bias = 0;

        /* Very good */
        if (power > 1)
        {
#if 0
		if (!rand_int(RANDART_JEWEL)) create_artifact(o_ptr, FALSE, TRUE);
			else
#endif
		/* Make ego item */
                make_ego_item(level, o_ptr, TRUE);
        }
        else if (power < -1)
        {
                /* Make ego item */
                make_ego_item(level, o_ptr, FALSE);
        }



	/* prolly something should be done..	- Jir - */
	/* Apply magic (good or bad) according to type */
	switch (o_ptr->tval)
	{
		case TV_RING:
		{
			/* Analyze */
			switch (o_ptr->sval)
			{
				case SV_RING_POLYMORPH:
				{
					if (power < 1) power = 1;

					/* Be sure to be a player */
					o_ptr->pval = 0;
					if (magik(45))
					{
						int i;
						monster_race *r_ptr;

						while (TRUE)
						{
							i = rand_int(MAX_R_IDX - 1);
							r_ptr = &r_info[i];

							if (!r_ptr->name) continue;
							if (r_ptr->flags1 & RF1_UNIQUE) continue;
							if (r_ptr->level >= level + (power * 5)) continue;
							if (!mon_allowed(r_ptr)) continue;

							break;
						}
						o_ptr->pval = i;
						/* Let's have the following level req code commented out
						to give found poly rings random levels to allow surprises :)
						if (r_info[i].level > 0) {
							o_ptr->level = 15 + (1000 / ((2000 / r_info[i].level) + 10));
						} else {
							o_ptr->level = 15;
						}*/
					}
					else o_ptr->level=1;
					break;
				}

				/* Strength, Constitution, Dexterity, Intelligence */
				case SV_RING_ATTACKS:
				{
					/* Stat bonus */
					o_ptr->bpval = m_bonus(3, level);
					if (o_ptr->bpval < 1) o_ptr->bpval = 1;

					/* Cursed */
					if (power < 0)
					{
						/* Cursed */
						o_ptr->ident |= (ID_CURSED);

						/* Reverse bpval */
						o_ptr->bpval = 0 - (o_ptr->bpval);
					}

					break;
				}

                                /* Critical hits */
                                case SV_RING_CRIT:
				{
					/* Stat bonus */
                                        o_ptr->bpval = m_bonus(10, level);
					if (o_ptr->bpval < 1) o_ptr->bpval = 1;

					/* Cursed */
					if (power < 0)
					{
						/* Cursed */
						o_ptr->ident |= (ID_CURSED);

						/* Reverse bpval */
						o_ptr->bpval = 0 - (o_ptr->bpval);
					}

					break;
				}


				case SV_RING_STR:
				case SV_RING_CON:
				case SV_RING_DEX:
				case SV_RING_INT:
				case SV_RING_SEARCHING:
				case SV_RING_STEALTH:
				{
					/* Stat bonus */
					o_ptr->bpval = 1 + m_bonus(5, level);

					/* Cursed */
					if (power < 0)
					{
						/* Cursed */
						o_ptr->ident |= (ID_CURSED);

						/* Reverse bpval */
						o_ptr->bpval = 0 - (o_ptr->bpval);
					}

					break;
				}

				/* Ring of Speed! */
				case SV_RING_SPEED:
				{
					/* Base speed (1 to 10) */
					o_ptr->bpval = randint(5) + m_bonus(5, level);

					/* Super-charge the ring */
					while (rand_int(100) < 50) o_ptr->bpval++;

					/* Cursed Ring */
					if (power < 0)
					{
						/* Cursed */
						o_ptr->ident |= (ID_CURSED);

						/* Reverse bpval */
						o_ptr->bpval = 0 - (o_ptr->bpval);

						break;
					}


					break;
				}

				case SV_RING_LORDLY:
				{
#if 0	// lordly pfft ring..
					do
					{
						random_resistance(o_ptr, FALSE, ((randint(20))+18));
					}
					while(randint(4)==1);
#endif

					/* Bonus to armor class */
					o_ptr->to_a = 10 + randint(5) + m_bonus(10, level);
				}
				break;

				/* Flames, Acid, Ice */
				case SV_RING_FLAMES:
				case SV_RING_ACID:
				case SV_RING_ICE:
				{
					/* Bonus to armor class */
					o_ptr->to_a = 5 + randint(5) + m_bonus(10, level);
					break;
				}

				/* Weakness, Stupidity */
				case SV_RING_WEAKNESS:
				case SV_RING_STUPIDITY:
				{
					/* Cursed */
					o_ptr->ident |= (ID_CURSED);

					/* Penalize */
					o_ptr->bpval = 0 - (1 + m_bonus(5, level));

					break;
				}

				/* WOE, Stupidity */
				case SV_RING_WOE:
				{
					/* Cursed */
					o_ptr->ident |= (ID_CURSED);

					/* Penalize */
					o_ptr->to_a = 0 - (5 + m_bonus(10, level));
					o_ptr->bpval = 0 - (1 + m_bonus(5, level));

					break;
				}

				/* Ring of damage */
				case SV_RING_DAMAGE:
				{
					/* Bonus to damage */
					o_ptr->to_d = 5 + randint(8) + m_bonus(10, level);

					/* Cursed */
					if (power < 0)
					{
						/* Cursed */
						o_ptr->ident |= (ID_CURSED);

						/* Reverse bonus */
						o_ptr->to_d = 0 - (o_ptr->to_d);
					}

					break;
				}

				/* Ring of Accuracy */
				case SV_RING_ACCURACY:
				{
					/* Bonus to hit */
					o_ptr->to_h = 5 + randint(8) + m_bonus(10, level);

					/* Cursed */
					if (power < 0)
					{
						/* Cursed */
						o_ptr->ident |= (ID_CURSED);

						/* Reverse tohit */
						o_ptr->to_h = 0 - (o_ptr->to_h);
					}

					break;
				}

				/* Ring of Protection */
				case SV_RING_PROTECTION:
				{
					/* Bonus to armor class */
					o_ptr->to_a = 5 + randint(8) + m_bonus(10, level);

					/* Cursed */
					if (power < 0)
					{
						/* Cursed */
						o_ptr->ident |= (ID_CURSED);

						/* Reverse toac */
						o_ptr->to_a = 0 - (o_ptr->to_a);
					}

					break;
				}

				/* Ring of Slaying */
				case SV_RING_SLAYING:
				{
					/* Bonus to damage and to hit */
					o_ptr->to_d = randint(7) + m_bonus(10, level);
					o_ptr->to_h = randint(7) + m_bonus(10, level);

					/* Cursed */
					if (power < 0)
					{
						/* Cursed */
						o_ptr->ident |= (ID_CURSED);

						/* Reverse bonuses */
						o_ptr->to_h = 0 - (o_ptr->to_h);
						o_ptr->to_d = 0 - (o_ptr->to_d);
					}

					break;
				}
			}

			break;
		}

		case TV_AMULET:
		{
			/* Analyze */
			switch (o_ptr->sval)
			{
				/* Old good Mangband ones */
				/* Amulet of Terken -- never cursed */
				case SV_AMULET_TERKEN:
				{
					o_ptr->bpval = randint(5) + m_bonus(5, level);
					//o_ptr->to_h = randint(5);
					//o_ptr->to_d = randint(5);

					/* Sorry.. */
//					o_ptr->xtra1 = EGO_XTRA_ABILITY;
//					o_ptr->xtra2 = randint(256);


					break;
				}

				/* Amulet of the Moon -- never cursed */
				case SV_AMULET_THE_MOON:
				{
					o_ptr->bpval = randint(5) + m_bonus(5, level);
					o_ptr->to_h = randint(5);
					o_ptr->to_d = randint(5);

					// o_ptr->xtra1 = EGO_XTRA_ABILITY;
					//o_ptr->xtra2 = randint(256);

					break;
				}

				/* Amulet of Trickery */
				case SV_AMULET_TRICKERY:
				case SV_AMULET_DEVOTION:
				{
					o_ptr->bpval = 1 + m_bonus(3, level);

					break;
				}

				case SV_AMULET_WEAPONMASTERY:
				{
					o_ptr->bpval = 1 + m_bonus(2, level);
					o_ptr->to_a = 1 + m_bonus(4, level);
					o_ptr->to_h = 1 + m_bonus(5, level);
					o_ptr->to_d = 1 + m_bonus(5, level);

					break;
				}

				/* Amulet of wisdom/charisma */
				case SV_AMULET_BRILLANCE:
				case SV_AMULET_CHARISMA:
				case SV_AMULET_WISDOM:
				case SV_AMULET_INFRA:
				{
					o_ptr->bpval = 1 + m_bonus(5, level);

					/* Cursed */
					if (power < 0)
					{
						/* Cursed */
						o_ptr->ident |= (ID_CURSED);

						/* Reverse bonuses */
						o_ptr->bpval = 0 - (o_ptr->bpval);
					}

					break;
				}

				/* Amulet of the Serpents */
				case SV_AMULET_SERPENT:
				{
					o_ptr->bpval = 1 + m_bonus(5, level);
					o_ptr->to_a = 1 + m_bonus(6, level);

					/* Cursed */
					if (power < 0)
					{
						/* Cursed */
						o_ptr->ident |= (ID_CURSED);

						/* Reverse bonuses */
						o_ptr->bpval = 0 - (o_ptr->bpval);
					}

					break;
				}

				case SV_AMULET_NO_MAGIC: case SV_AMULET_NO_TELE:
				{
					if (power < 0)
					{
						o_ptr->ident |= (ID_CURSED);
					}
					break;
				}

				case SV_AMULET_RESISTANCE:
				{
#if 0
					if (randint(3)==1) random_resistance(o_ptr, FALSE, ((randint(34))+4));
					if (randint(5)==1) o_ptr->art_flags2 |= TR2_RES_POIS;
#endif	// 0
				}
				break;

				/* Amulet of searching */
				case SV_AMULET_SEARCHING:
				{
					o_ptr->bpval = randint(5) + m_bonus(5, level);

					/* Cursed */
					if (power < 0)
					{
						/* Cursed */
						o_ptr->ident |= (ID_CURSED);

						/* Reverse bonuses */
						o_ptr->bpval = 0 - (o_ptr->bpval);
					}

					break;
				}

				/* Amulet of the Magi -- never cursed */
				case SV_AMULET_THE_MAGI:
				{
					o_ptr->bpval = 1 + m_bonus(3, level);

					//					if (randint(3)==1) o_ptr->art_flags3 |= TR3_SLOW_DIGEST;

					break;
				}

				/* Amulet of Doom -- always cursed */
				case SV_AMULET_DOOM:
				{
					/* Cursed */
					o_ptr->ident |= (ID_CURSED);

					/* Penalize */
					o_ptr->bpval = 0 - (randint(5) + m_bonus(5, level));
					o_ptr->to_a = 0 - (randint(5) + m_bonus(5, level));

					break;
				}

				/* Amulet of Rage, formerly 'Suspicion' */
				case SV_AMULET_RAGE:
				{
					o_ptr->bpval = 1 + m_bonus(4, level);
					//o_ptr->to_a = -1 - m_bonus(15, level);
					o_ptr->to_h = -1 - m_bonus(10, level);//15
					o_ptr->to_d = 1 + m_bonus(15, level);
					
					if (rand_int(100) < 33) {
	                                        o_ptr->xtra1 = EGO_XTRA_POWER;
    		                                o_ptr->xtra2 = rand_int(255);
					}
					break;
				}

				/* Amulet of speed */
				case SV_AMULET_SPEED:
				{
					// Amulets of speed can't give very
					// much, and are rarely +5.
					o_ptr->bpval = randint(randint(5)); 

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= ID_BROKEN;

						/* Cursed */
						o_ptr->ident |= ID_CURSED;

						/* Reverse bonuses */
						o_ptr->bpval = 0 - (o_ptr->bpval);
					}

					break;
				}
				
				/* Formerly Amulet of ESP */
				case SV_AMULET_ESP:
				{
//					o_ptr->name2 = EGO_ESP;
			                make_ego_item(level, o_ptr, TRUE);
					break;
				}
			}

			break;
		}
	}
#if 0
	switch (o_ptr->tval)
	{
		case TV_RING:
		{
			/* Analyze */
			switch (o_ptr->sval)
			{
			case SV_RING_POLYMORPH:
			  {
			    if (power < 1) power = 1;

			    /* Be sure to be a player */
			    o_ptr->pval = 0;
			    if (magik(45))
			      {
				int i;
				monster_race *r_ptr;

				while (TRUE)
				  {
				    i = rand_int(MAX_R_IDX - 1);
				    r_ptr = &r_info[i];

				    if (r_ptr->flags1 & RF1_UNIQUE) continue;
				    if (r_ptr->level >= level + (power * 5)) continue;

				    break;
				  }
				o_ptr->pval = i;
			      }
			    break;
			  }

				/* Strength, Constitution, Dexterity, Intelligence */
				case SV_RING_STR:
				case SV_RING_CON:
				case SV_RING_DEX:
				case SV_RING_INT:
				{
					/* Stat bonus */
					o_ptr->pval = 1 + m_bonus(5, level);

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= ID_BROKEN;

						/* Cursed */
						o_ptr->ident |= ID_CURSED;

						/* Reverse pval */
						o_ptr->pval = 0 - (o_ptr->pval);
					}

					break;
				}

				/* Ring of Speed! */
				case SV_RING_SPEED:
				{
					/* Base speed (1 to 10) */
					o_ptr->pval = randint(5) + m_bonus(5, level);

					/* Super-charge the ring */
					tries = 100;
					while ((--tries) && rand_int(100) < 50) o_ptr->pval++;

					/* Cursed Ring */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= ID_BROKEN;

						/* Cursed */
						o_ptr->ident |= ID_CURSED;

						/* Reverse pval */
						o_ptr->pval = 0 - (o_ptr->pval);

						break;
					}

					break;
				}

				/* Searching */
				case SV_RING_SEARCHING:
				{
					/* Bonus to searching */
					o_ptr->pval = 1 + m_bonus(5, level);

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= ID_BROKEN;

						/* Cursed */
						o_ptr->ident |= ID_CURSED;

						/* Reverse pval */
						o_ptr->pval = 0 - (o_ptr->pval);
					}

					break;
				}

				/* Flames, Acid, Ice */
				case SV_RING_FLAMES:
				case SV_RING_ACID:
				case SV_RING_ICE:
				{
					/* Bonus to armor class */
					o_ptr->to_a = 5 + randint(5) + m_bonus(10, level);
					break;
				}

				/* Weakness, Stupidity */
				case SV_RING_WEAKNESS:
				case SV_RING_STUPIDITY:
				{
					/* Broken */
					o_ptr->ident |= ID_BROKEN;

					/* Cursed */
					o_ptr->ident |= ID_CURSED;

					/* Penalize */
					o_ptr->pval = 0 - (1 + m_bonus(5, level));

					break;
				}

				/* WOE, Stupidity */
				case SV_RING_WOE:
				{
					/* Broken */
					o_ptr->ident |= ID_BROKEN;

					/* Cursed */
					o_ptr->ident |= ID_CURSED;

					/* Penalize */
					o_ptr->to_a = 0 - (5 + m_bonus(10, level));
					o_ptr->pval = 0 - (1 + m_bonus(5, level));

					break;
				}

				/* Ring of damage */
				case SV_RING_DAMAGE:
				{
					/* Bonus to damage */
					o_ptr->to_d = 5 + randint(5) + m_bonus(10, level);

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= ID_BROKEN;

						/* Cursed */
						o_ptr->ident |= ID_CURSED;

						/* Reverse bonus */
						o_ptr->to_d = 0 - (o_ptr->to_d);
					}

					break;
				}

				/* Ring of Accuracy */
				case SV_RING_ACCURACY:
				{
					/* Bonus to hit */
					o_ptr->to_h = 5 + randint(5) + m_bonus(10, level);

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= ID_BROKEN;

						/* Cursed */
						o_ptr->ident |= ID_CURSED;

						/* Reverse tohit */
						o_ptr->to_h = 0 - (o_ptr->to_h);
					}

					break;
				}

				/* Ring of Protection */
				case SV_RING_PROTECTION:
				{
					/* Bonus to armor class */
					o_ptr->to_a = 5 + randint(5) + m_bonus(10, level);

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= ID_BROKEN;

						/* Cursed */
						o_ptr->ident |= ID_CURSED;

						/* Reverse toac */
						o_ptr->to_a = 0 - (o_ptr->to_a);
					}

					break;
				}

				/* Ring of Slaying */
				case SV_RING_SLAYING:
				{
					/* Bonus to damage and to hit */
					o_ptr->to_d = randint(5) + m_bonus(10, level);
					o_ptr->to_h = randint(5) + m_bonus(10, level);

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= ID_BROKEN;

						/* Cursed */
						o_ptr->ident |= ID_CURSED;

						/* Reverse bonuses */
						o_ptr->to_h = 0 - (o_ptr->to_h);
						o_ptr->to_d = 0 - (o_ptr->to_d);
					}

					break;
				}
			}

			break;
		}

		case TV_AMULET:
		{
			/* Analyze */
			switch (o_ptr->sval)
			{
				/* Amulet of wisdom/charisma */
				case SV_AMULET_WISDOM:
				case SV_AMULET_CHARISMA:
				{
					o_ptr->pval = 1 + m_bonus(5, level);

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= ID_BROKEN;

						/* Cursed */
						o_ptr->ident |= ID_CURSED;

						/* Reverse bonuses */
						o_ptr->pval = 0 - (o_ptr->pval);
					}

					break;
				}

				/* Amulet of searching */
				case SV_AMULET_SEARCHING:
				{
					o_ptr->pval = randint(5) + m_bonus(5, level);

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= ID_BROKEN;

						/* Cursed */
						o_ptr->ident |= ID_CURSED;

						/* Reverse bonuses */
						o_ptr->pval = 0 - (o_ptr->pval);
					}

					break;
				}

				/* Amulet of speed */
				case SV_AMULET_SPEED:
				{
					// Amulets of speed can't give very
					// much, and are rarely +5.
					o_ptr->pval = randint(randint(5)); 

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= ID_BROKEN;

						/* Cursed */
						o_ptr->ident |= ID_CURSED;

						/* Reverse bonuses */
						o_ptr->pval = 0 - (o_ptr->pval);
					}

					break;
				}

#if 1	// those items should be handled in other ways(INSTA_EGO?)	- Jir -
         			/* Amulet of Terken -- never cursed */
                                case SV_AMULET_TERKEN:
				{
					o_ptr->pval = randint(5) + m_bonus(5, level);
                                        //o_ptr->to_h = randint(5);
                                        //o_ptr->to_d = randint(5);

                                        o_ptr->xtra1 = EGO_XTRA_ABILITY;
                                        o_ptr->xtra2 = randint(256);

					break;
				}

         			/* Amulet of the Moon -- never cursed */
                                case SV_AMULET_THE_MOON:
				{
					o_ptr->pval = randint(5) + m_bonus(5, level);
                                        o_ptr->to_h = randint(5);
                                        o_ptr->to_d = randint(5);

                                       // o_ptr->xtra1 = EGO_XTRA_ABILITY;
                                        //o_ptr->xtra2 = randint(256);

					break;
				}

				/* Amulet of the Magi -- never cursed */
				case SV_AMULET_THE_MAGI:
				{
					o_ptr->pval = randint(5) + m_bonus(5, level);
					o_ptr->to_a = randint(5) + m_bonus(5, level);

                 			o_ptr->xtra1 = EGO_XTRA_POWER;
                                        o_ptr->xtra2 = randint(256);

					break;
				}
#endif	// 0

				/* Amulet of Doom -- always cursed */
				case SV_AMULET_DOOM:
				{
					/* Broken */
					o_ptr->ident |= ID_BROKEN;

					/* Cursed */
					o_ptr->ident |= ID_CURSED;

					/* Penalize */
					o_ptr->pval = 0 - (randint(5) + m_bonus(5, level));
					o_ptr->to_a = 0 - (randint(5) + m_bonus(5, level));

					break;
				}
			}

			break;
		}
	}
#endif	// 0
}


/*
 * Apply magic to an item known to be "boring"
 *
 * Hack -- note the special code for various items
 */
static void a_m_aux_4(object_type *o_ptr, int level, int power)
{
        u32b f1, f2, f3, f4, f5, esp;

        /* Very good */
        if (power > 1)
        {
                /* Make ego item */
//                if (!rand_int(RANDART_JEWEL) && (o_ptr->tval == TV_LITE)) create_artifact(o_ptr, FALSE, TRUE);	else
                make_ego_item(level, o_ptr, TRUE);
        }
        else if (power < -1)
        {
                /* Make ego item */
                make_ego_item(level, o_ptr, FALSE);
        }

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	/* Apply magic (good or bad) according to type */
	switch (o_ptr->tval)
	{
		case TV_BOOK:
		{
			/* Randomize random books */
			if (o_ptr->sval == 255)
			{
				int  i = 0, tries = 1000;

				while (tries)
				{
					tries--;

					/* Pick a spell */
					i = rand_int(max_spells);

                                        /* Only random ones */
//                                        if (exec_lua(format("return can_spell_random(%d)", i)) == FALSE)
//                                                continue;

					/* Test if it passes the level check */
                                        if (rand_int(school_spells[i].skill_level * 3) <= level)
                                        {
                                                /* Ok */
                                                break;
                                        }
				}
				/* Use globe of light(or the first one) */
				if (!tries)
					o_ptr->pval = 0;
				else
					o_ptr->pval = i;
			}

			break;
		}

		case TV_LITE:

			o_ptr->to_h = o_ptr->to_d = o_ptr->to_a = 0;

		/* Hack -- Torches -- random fuel */
			if (f4 & TR4_FUEL_LITE)
			{
				if (o_ptr->sval == SV_LITE_TORCH)
				{
//					if (o_ptr->pval) o_ptr->pval = randint(o_ptr->pval);
					o_ptr->timeout = randint(FUEL_TORCH);
				}

				/* Hack -- Lanterns -- random fuel */
				else if (o_ptr->sval == SV_LITE_LANTERN)
				{
					o_ptr->timeout = randint(FUEL_LAMP);
//					if (o_ptr->pval) o_ptr->pval = randint(o_ptr->pval);
				}
			}

		break;


		case TV_WAND:

		/* Hack -- charge wands */
		charge_wand(o_ptr);

		break;


		case TV_STAFF:

		/* Hack -- charge staffs */
		charge_staff(o_ptr);

		break;


		case TV_CHEST:

		/* Hack -- skip ruined chests */
		if (k_info[o_ptr->k_idx].level <= 0) break;

		/* Pick a trap */
		place_trap_object(o_ptr);
#if 0					
		/* Hack -- pick a "difficulty" */
		o_ptr->pval = randint(k_info[o_ptr->k_idx].level);

		/* Never exceed "difficulty" of 55 to 59 */
		if (o_ptr->pval > 55) o_ptr->pval = 55 + rand_int(5);
#endif	// 0

		break;

		case TV_GOLEM:
		{
			switch (o_ptr->sval)
			{
				case SV_GOLEM_ARM:
					o_ptr->pval = 1 + m_bonus(15, level);
					break;
				case SV_GOLEM_LEG:
					o_ptr->pval = 1 + m_bonus(8, level);
					break;
			}
			break;
		}


		/* Hack -- consider using 'timeout' inplace */
		case TV_ROD:

		/* Hack -- charge rods */
		o_ptr->pval = 0;

		break;


	}
}


/*
 * Complete the "creation" of an object by applying "magic" to the item
 *
 * This includes not only rolling for random bonuses, but also putting the
 * finishing touches on ego-items and artifacts, giving charges to wands and
 * staffs, giving fuel to lites, and placing traps on chests.
 *
 * In particular, note that "Instant Artifacts", if "created" by an external
 * routine, must pass through this function to complete the actual creation.
 *
 * The base "chance" of the item being "good" increases with the "level"
 * parameter, which is usually derived from the dungeon level, being equal
 * to the level plus 10, up to a maximum of 75.  If "good" is true, then
 * the object is guaranteed to be "good".  If an object is "good", then
 * the chance that the object will be "great" (ego-item or artifact), also
 * increases with the "level", being equal to half the level, plus 5, up to
 * a maximum of 20.  If "great" is true, then the object is guaranteed to be
 * "great".  At dungeon level 65 and below, 15/100 objects are "great".
 *
 * If the object is not "good", there is a chance it will be "cursed", and
 * if it is "cursed", there is a chance it will be "broken".  These chances
 * are related to the "good" / "great" chances above.
 *
 * Otherwise "normal" rings and amulets will be "good" half the time and
 * "cursed" half the time, unless the ring/amulet is always good or cursed.
 *
 * If "okay" is true, and the object is going to be "great", then there is
 * a chance that an artifact will be created.  This is true even if both the
 * "good" and "great" arguments are false.  As a total hack, if "great" is
 * true, then the item gets 3 extra "attempts" to become an artifact.
 */
void apply_magic(struct worldpos *wpos, object_type *o_ptr, int lev, bool okay, bool good, bool great)
{
	int i, rolls, f1, f2, power;


	/* Maximum "level" for various things */
        if (lev > MAX_DEPTH_OBJ - 1) lev = MAX_DEPTH_OBJ - 1;


	/* Base chance of being "good" */
	f1 = lev + 10;

	/* Maximal chance of being "good" */
	if (f1 > 75) f1 = 75;

	/* Base chance of being "great" */
	f2 = f1 / 2;

	/* Maximal chance of being "great" */
	if (f2 > 20) f2 = 20;


	/* Assume normal */
	power = 0;

	/* Roll for "good" */
	if (good || magik(f1))
	{
		/* Assume "good" */
		power = 1;

		/* Roll for "great" */
		if (great || magik(f2)) power = 2;
	}

	/* Roll for "cursed" */
	else if (magik(f1))
	{
		/* Assume "cursed" */
		power = -1;

		/* Roll for "broken" */
		if (magik(f2)) power = -2;
	}


	/* Assume no rolls */
	rolls = 0;

	/* Get one roll if excellent */
	if (power >= 2) rolls = 1;

	/* Hack -- Get four rolls if forced great */
	if (great) rolls = 4;

	/* Hack -- Get no rolls if not allowed */
	if (!okay || o_ptr->name1) rolls = 0;

	/* Roll for artifacts if allowed */
	for (i = 0; i < rolls; i++)
	{
		/* Roll for an artifact */
		if (make_artifact(wpos, o_ptr)) break;
	}

	/* virgin */
	o_ptr->owner = 0;

	/* determine level-requirement */
	determine_level_req(lev, o_ptr);

	/* Hack -- analyze artifacts */
	if (o_ptr->name1)
	{
	 	artifact_type *a_ptr;
	 	
	 	/* Randart */
		if (o_ptr->name1 == ART_RANDART)
		{
			a_ptr =	randart_make(o_ptr);
		}
		/* Normal artifacts */
		else
		{
			a_ptr = &a_info[o_ptr->name1];
		}
		if(a_ptr==(artifact_type*)NULL){
			o_ptr->name1=0;
			return;
		}

		/* Hack -- Mark the artifact as "created" */
		a_ptr->cur_num = 1;

		/* Info */
		/* s_printf("Created artifact %d.\n", o_ptr->name1); */

		/* Extract the other fields */
		o_ptr->pval = a_ptr->pval;
		o_ptr->ac = a_ptr->ac;
		o_ptr->dd = a_ptr->dd;
		o_ptr->ds = a_ptr->ds;
		o_ptr->to_a = a_ptr->to_a;
		o_ptr->to_h = a_ptr->to_h;
		o_ptr->to_d = a_ptr->to_d;
		o_ptr->weight = a_ptr->weight;

		/* Hack -- no bundled arts (esp.missiles) */
		o_ptr->number = 1;
		o_ptr->timeout = 0;

		/* Hack -- extract the "broken" flag */
		if (!a_ptr->cost) o_ptr->ident |= ID_BROKEN;

		/* Hack -- extract the "cursed" flag */
		if (a_ptr->flags3 & TR3_CURSED) o_ptr->ident |= ID_CURSED;

		/* Done */
		return;
	}


	/* Apply magic */
	switch (o_ptr->tval)
	{
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_BOW:
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_BOOMERANG:
		case TV_AXE:
		case TV_MSTAFF:
		{
			if (power) a_m_aux_1(o_ptr, lev, power);
			break;
		}

		case TV_DRAG_ARMOR:
#if 0
			if (o_ptr->sval == 6)
			{
				switch rand_int(4)
				{
				        case 0:
					{
						o_ptr->art_flags2 |= TR2_IM_FIRE;
						break;
					}
					case 1:
					{
						o_ptr->art_flags2 |= TR2_IM_COLD;
						break;
					}
					case 2:
					{
						o_ptr->art_flags2 |= TR2_IM_ELEC;
						break;
					}
					case 3:
					{
						o_ptr->art_flags2 |= TR2_IM_ACID;
						break;
					}
				}
			}
#endif
		case TV_HARD_ARMOR:
		case TV_SOFT_ARMOR:
		case TV_SHIELD:
		case TV_HELM:
		case TV_CROWN:
		case TV_CLOAK:
		case TV_GLOVES:
		case TV_BOOTS:
		{
			// Power is no longer required since things such as
			// Kollas need magic applied to finish their normal
			// generation.
			//if (power) a_m_aux_2(o_ptr, lev, power);
			a_m_aux_2(o_ptr, lev, power);
			break;
		}

		case TV_RING:
		case TV_AMULET:
		{
			if (!power && (rand_int(100) < 50)) power = -1;
			a_m_aux_3(o_ptr, lev, power);
			break;
		}

		default:
		{
			a_m_aux_4(o_ptr, lev, power);
			break;
		}
	}

#if 1	// tweaked pernA ego.. 
	/* Hack -- analyze ego-items */
//	else if (o_ptr->name2)
	if (o_ptr->name2 && !o_ptr->name1)
	{
#if 0
		ego_item_type *e_ptr;
		int j;
		bool limit_blows = FALSE;
		u32b f1, f2, f3, f4, f5, esp;
		s16b e_idx;
#endif	// 0

		artifact_type *a_ptr;

		a_ptr =	ego_make(o_ptr);

		/* Extract the other fields */
		o_ptr->pval += a_ptr->pval;
		//		o_ptr->pval = a_ptr->pval;
		o_ptr->ac += a_ptr->ac;
		o_ptr->dd += a_ptr->dd;
		o_ptr->ds += a_ptr->ds;
		o_ptr->to_a += a_ptr->to_a;
		o_ptr->to_h += a_ptr->to_h;
		o_ptr->to_d += a_ptr->to_d;

		/* Hack -- acquire "cursed" flag */
		//                if (f3 & TR3_CURSED) o_ptr->ident |= (ID_CURSED);	// this should be done here!
		if (a_ptr->flags3 & TR3_CURSED) o_ptr->ident |= (ID_CURSED);
	}
#endif	// 0

#if 0	// mangband ego...
	/* Hack -- analyze ego-items */
	if (o_ptr->name2)
	{
		ego_item_type *e_ptr = &e_info[o_ptr->name2];

		/* Hack -- extra powers */
		switch (o_ptr->name2)
		{
			/* Weapon (Holy Avenger) */
			case EGO_HA:
			{
				o_ptr->xtra1 = EGO_XTRA_SUSTAIN;
				break;
			}

			/* Weapon (Defender) */
			case EGO_DF:
			{
				o_ptr->xtra1 = EGO_XTRA_SUSTAIN;
				break;
			}

			/* Weapon (Blessed) */
			case EGO_BLESS_BLADE:
			{
				o_ptr->xtra1 = EGO_XTRA_ABILITY;
				break;
			}

			/* Robe of Permanance */
			case EGO_PERMANENCE:
			{
				o_ptr->xtra1 = EGO_XTRA_POWER;
				break;
			}

			/* Armor of Elvenkind */
			case EGO_ELVENKIND:
			{
				o_ptr->xtra1 = EGO_XTRA_POWER;
				break;
			}

			/* Crown of the Magi */
			case EGO_MAGI:
			{
				o_ptr->xtra1 = EGO_XTRA_ABILITY;
				break;
			}

			/* Cloak of Aman */
			case EGO_AMAN:
			{
				o_ptr->xtra1 = EGO_XTRA_POWER;
				break;
			}

                        /* Shield of the Avari */
                        case EGO_AVARI:
			{
				o_ptr->xtra1 = EGO_XTRA_POWER;
				break;
			}

                        /* Gloves of the Istari */
                        case EGO_ISTARI:
			{
                                o_ptr->xtra1 = EGO_XTRA_POWER;
				break;
			}

                        /* Gloves of Magic */
                        case EGO_MAGIC:
			{
                                o_ptr->xtra1 = EGO_XTRA_ABILITY;
				break;
			}
		}

		/* Randomize the "xtra" power */
		if (o_ptr->xtra1) o_ptr->xtra2 = randint(256);

		/* Hack -- acquire "broken" flag */
		if (!e_ptr->cost) o_ptr->ident |= ID_BROKEN;

		/* Hack -- acquire "cursed" flag */
		if (e_ptr->flags3 & TR3_CURSED) o_ptr->ident |= ID_CURSED;

		/* Hack -- apply extra penalties if needed */
		if (cursed_p(o_ptr) || broken_p(o_ptr))
		{
			/* Hack -- obtain bonuses */
			if (e_ptr->max_to_h > 0) o_ptr->to_h -= randint(e_ptr->max_to_h);
			else if (e_ptr->max_to_h < 0) o_ptr->to_h += randint(-e_ptr->max_to_h);
			if (e_ptr->max_to_d > 0) o_ptr->to_d -= randint(e_ptr->max_to_d);
			else if (e_ptr->max_to_d < 0) o_ptr->to_d += randint(-e_ptr->max_to_d);
			if (e_ptr->max_to_a > 0) o_ptr->to_a -= randint(e_ptr->max_to_a);
			else if (e_ptr->max_to_a < 0) o_ptr->to_a += randint(-e_ptr->max_to_a);

			/* Hack -- obtain pval */
			if (e_ptr->max_pval > 0) o_ptr->pval -= randint(e_ptr->max_pval);
			else if (e_ptr->max_pval < 0) o_ptr->pval += randint(-e_ptr->max_pval);
		}

		/* Hack -- apply extra bonuses if needed */
		else
		{
			/* Hack -- obtain bonuses */
			if (e_ptr->max_to_h > 0) o_ptr->to_h += randint(e_ptr->max_to_h);
			else if (e_ptr->max_to_h < 0) o_ptr->to_h -= randint(-e_ptr->max_to_h);
			if (e_ptr->max_to_d > 0) o_ptr->to_d += randint(e_ptr->max_to_d);
			else if (e_ptr->max_to_d < 0) o_ptr->to_d -= randint(-e_ptr->max_to_d);
			if (e_ptr->max_to_a > 0) o_ptr->to_a += randint(e_ptr->max_to_a);
			else if (e_ptr->max_to_a < 0) o_ptr->to_a -= randint(-e_ptr->max_to_a);

			/* Hack -- obtain pval */
			if (e_ptr->max_pval > 0) o_ptr->pval += randint(e_ptr->max_pval);
			else if (e_ptr->max_pval < 0) o_ptr->pval -= randint(-e_ptr->max_pval);
		}

		/* Done */
		return;
	}
#endif	// 0


	/* Examine real objects */
	if (o_ptr->k_idx)
	{
		object_kind *k_ptr = &k_info[o_ptr->k_idx];

		/* Hack -- acquire "broken" flag */
		if (!k_ptr->cost) o_ptr->ident |= ID_BROKEN;

		/* Hack -- acquire "cursed" flag */
		if (k_ptr->flags3 & TR3_CURSED) o_ptr->ident |= ID_CURSED;
	}
}

/*
 * This 'utter hack' function is to allow item-generation w/o specifing
 * worldpos.
 */
void apply_magic_depth(int Depth, object_type *o_ptr, int lev, bool okay, bool good, bool great)
{
	worldpos wpos;

	/* CHANGEME */
	wpos.wx = cfg.town_x;
	wpos.wy = cfg.town_y;
	wpos.wz = Depth > 0 ? 0 - Depth : Depth;
	apply_magic(&wpos, o_ptr, lev, okay, good, great);
}


/*
 * determine level requirement.
 * based on C.Blue's idea.	- Jir -
 */
void determine_level_req(int level, object_type *o_ptr)
{
	int i, j, base = k_info[o_ptr->k_idx].level;

	/* Unowned yet */
//	o_ptr->owner = 0;

#if 0	/* older routine */
	/* Wilderness? (Obviously ridiculous) */
#ifndef NEW_DUNGEON
	Depth = (Depth > 0) ? Depth : -Depth;
#endif

//	o_ptr->level = ((Depth * 2 / 4) > 100)?100:(Depth * 2 / 4);
	if (Depth > 0) o_ptr->level = ((Depth * 2 / 4) > 100)?100:(Depth * 2 / 4);
	else o_ptr->level = -((Depth * 2 / 4) > 100)?100:(Depth * 2 / 4);
	if (o_ptr->level < 1) o_ptr->level = 1;
#endif

	/* artifact */
	if (o_ptr->name1)
	{
	 	artifact_type *a_ptr;
	 	
	 	/* Randart */
		if (o_ptr->name1 == ART_RANDART)
		{
			a_ptr =	randart_make(o_ptr);
			if(a_ptr==(artifact_type*)NULL){
				o_ptr->name1=0;
				o_ptr->level=0;
				return;
			}

			/* level of randarts tends to be outrageous */
			base = a_ptr->level / 2;
		}
		/* Normal artifacts */
		else
		{
			a_ptr = &a_info[o_ptr->name1];
			base = a_ptr->level;
		}
	}

	/* Hack -- analyze ego-items */
	else if (o_ptr->name2)
	{
//		ego_item_type *e_ptr = &e_info[o_ptr->name2];
		base += e_info[o_ptr->name2].rating;

		if (o_ptr->name2b) base += e_info[o_ptr->name2b].rating;
	}

	/* '17/72' == 0.2361... < 1/4 :) */
	base >>= 1;
	i = level - base;
	j = (i * (i > 0 ? 1 : 2) / 4  + base) * rand_range(95,105) / 100;
	o_ptr->level = j < 100 ? (j > 1 ? j : 1) : 100;

}


/*
 * Object-theme codes borrowed from ToME.	- Jir -
 */

/* The themed objects to use */
static obj_theme match_theme;

/*
 * XXX XXX XXX It relies on the fact that obj_theme is a four byte structure
 * for its efficient operation. A horrendous hack, I'd say.
 */
void init_match_theme(obj_theme theme)
{
	/* Save the theme */
	match_theme = theme;
}

#if 0
/*
 * Ditto XXX XXX XXX
 */
static bool theme_changed(obj_theme theme)
{
	/* Any of the themes has been changed */
	if (theme.treasure != match_theme.treasure) return (TRUE);
	if (theme.combat != match_theme.combat) return (TRUE);
	if (theme.magic != match_theme.magic) return (TRUE);
	if (theme.tools != match_theme.tools) return (TRUE);

	/* No changes */
	return (FALSE);
}
#endif	// 0


/*
 * Maga-Hack -- match certain types of object only.
 */
static bool kind_is_theme(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	s32b prob = 0;


	/*
	 * Paranoia -- Prevent accidental "(Nothing)"
	 * that are results of uninitialised theme structs.
	 *
	 * Caution: Junks go into the allocation table.
	 */
	if (match_theme.treasure + match_theme.combat +
	    match_theme.magic + match_theme.tools == 0) return (TRUE);


	/* Pick probability to use */
	switch (k_ptr->tval)
	{
		case TV_SKELETON:
		case TV_BOTTLE:
		case TV_JUNK:
		case TV_FIRESTONE:
		case TV_CORPSE:
		case TV_EGG:
		{
			/*
			 * Degree of junk is defined in terms of the other
			 * 4 quantities XXX XXX XXX
			 * The type of prob should be *signed* as well as
			 * larger than theme components, or we would see
			 * unexpected, well, junks.
			 */
			prob = 100 - (match_theme.treasure + match_theme.combat +
			              match_theme.magic + match_theme.tools);
			break;
		}
		case TV_CHEST:          prob = match_theme.treasure; break;
		case TV_CROWN:          prob = match_theme.treasure; break;
		case TV_DRAG_ARMOR:     prob = match_theme.treasure; break;
		case TV_AMULET:         prob = match_theme.treasure; break;
		case TV_RING:           prob = match_theme.treasure; break;

		case TV_SHOT:           prob = match_theme.combat; break;
		case TV_ARROW:          prob = match_theme.combat; break;
		case TV_BOLT:           prob = match_theme.combat; break;
		case TV_BOOMERANG:      prob = match_theme.combat; break;
		case TV_BOW:            prob = match_theme.combat; break;
		case TV_HAFTED:         prob = match_theme.combat; break;
		case TV_POLEARM:        prob = match_theme.combat; break;
		case TV_SWORD:          prob = match_theme.combat; break;
		case TV_AXE:            prob = match_theme.combat; break;
		case TV_GLOVES:         prob = match_theme.combat; break;
		case TV_HELM:           prob = match_theme.combat; break;
		case TV_SHIELD:         prob = match_theme.combat; break;
		case TV_SOFT_ARMOR:     prob = match_theme.combat; break;
		case TV_HARD_ARMOR:     prob = match_theme.combat; break;

		case TV_MSTAFF:         prob = match_theme.magic; break;
		case TV_STAFF:          prob = match_theme.magic; break;
		case TV_WAND:           prob = match_theme.magic; break;
		case TV_ROD:            prob = match_theme.magic; break;
		case TV_ROD_MAIN:       prob = match_theme.magic; break;
		case TV_SCROLL:         prob = match_theme.magic; break;
		case TV_PARCHEMENT:     prob = match_theme.magic; break;
		case TV_POTION:         prob = match_theme.magic; break;
		case TV_POTION2:        prob = match_theme.magic; break;
#if 0
		case TV_BATERIE:        prob = match_theme.magic; break;
		case TV_RANDART:        prob = match_theme.magic; break;
		case TV_RUNE1:          prob = match_theme.magic; break;
		case TV_RUNE2:          prob = match_theme.magic; break;
		case TV_BOOK:           prob = match_theme.magic; break;
		case TV_SYMBIOTIC_BOOK: prob = match_theme.magic; break;
		case TV_MUSIC_BOOK:     prob = match_theme.magic; break;
		case TV_DRUID_BOOK:     prob = match_theme.magic; break;
		case TV_DAEMON_BOOK:    prob = match_theme.magic; break;
#endif	// 0

		case TV_LITE:           prob = match_theme.tools; break;
		case TV_CLOAK:          prob = match_theme.tools; break;
		case TV_BOOTS:          prob = match_theme.tools; break;
		case TV_SPIKE:          prob = match_theme.tools; break;
		case TV_DIGGING:        prob = match_theme.tools; break;
		case TV_FLASK:          prob = match_theme.tools; break;
		case TV_FOOD:           prob = match_theme.tools; break;
		case TV_TOOL:           prob = match_theme.tools; break;
		case TV_INSTRUMENT:     prob = match_theme.tools; break;
		case TV_TRAPKIT:        prob = match_theme.tools; break;
	}

	/* Roll to see if it can be made */
	if (rand_int(100) < prob) return (TRUE);

	/* Not a match */
	return (FALSE);
}

/*
 * Determine if an object must not be generated.
 */
int kind_is_legal_special = -1;
bool kind_is_legal(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	if (!kind_is_theme(k_idx)) return FALSE;

#if 0
	if (k_ptr->flags4 & TR4_SPECIAL_GENE)
	{
		if (k_allow_special[k_idx]) return TRUE;
		else return FALSE;
	}

	/* No 2 times the same normal artifact */
	if ((k_ptr->flags3 & TR3_NORM_ART) && (k_ptr->artifact))
	{
		return FALSE;
	}

	if (k_ptr->tval == TV_CORPSE)
	{
		if (k_ptr->sval != SV_CORPSE_SKULL && k_ptr->sval != SV_CORPSE_SKELETON &&
			 k_ptr->sval != SV_CORPSE_HEAD && k_ptr->sval != SV_CORPSE_CORPSE)
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	if (k_ptr->tval == TV_HYPNOS) return FALSE;

	if ((k_ptr->tval == TV_SCROLL) && (k_ptr->sval == SV_SCROLL_SPELL)) return FALSE;
#endif	// 0

	/* Used only for the Nazgul rings */
	if ((k_ptr->tval == TV_RING) && (k_ptr->sval == SV_RING_SPECIAL)) return FALSE;

	/* Are we forced to one tval ? */
	if ((kind_is_legal_special != -1) && (kind_is_legal_special != k_ptr->tval)) return (FALSE);

	/* Assume legal */
	return TRUE;
}




/*
 * Hack -- determine if a template is "good"
 */
static bool kind_is_good(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	switch (k_ptr->tval)
	{
		/* Armor -- Good unless damaged */
		case TV_HARD_ARMOR:
		case TV_SOFT_ARMOR:
		case TV_DRAG_ARMOR:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		{
			if (k_ptr->to_a < 0) return (FALSE);
			return (TRUE);
		}

		/* Weapons -- Good unless damaged */
		case TV_BOW:
		case TV_SWORD:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_DIGGING:
		case TV_AXE:
		case TV_BOOMERANG:
		{
			if (k_ptr->to_h < 0) return (FALSE);
			if (k_ptr->to_d < 0) return (FALSE);
			return (TRUE);
		}

		/* Ammo -- Arrows/Bolts are good */
		case TV_BOLT:
		case TV_ARROW:
//		case TV_SHOT:	/* are Shots bad? */
		case TV_MSTAFF:
		{
			return (TRUE);
		}

		/* Rings -- Rings of Speed are good */
		case TV_RING:
		{
			if (k_ptr->sval == SV_RING_SPEED) return (TRUE);
			if (k_ptr->sval == SV_RING_BARAHIR) return (TRUE);
			if (k_ptr->sval == SV_RING_TULKAS) return (TRUE);
			if (k_ptr->sval == SV_RING_NARYA) return (TRUE);
			if (k_ptr->sval == SV_RING_NENYA) return (TRUE);
			if (k_ptr->sval == SV_RING_VILYA) return (TRUE);
			if (k_ptr->sval == SV_RING_POWER) return (TRUE);
			return (FALSE);
		}

		/* Amulets -- Amulets of the Magi are good */
		case TV_AMULET:
		{
#if 0
			if (k_ptr->sval == SV_AMULET_THE_MAGI) return (TRUE);
			if (k_ptr->sval == SV_AMULET_THE_MOON) return (TRUE);
			if (k_ptr->sval == SV_AMULET_SPEED) return (TRUE);
			if (k_ptr->sval == SV_AMULET_TERKEN) return (TRUE);
#endif
			return (FALSE);
		}
	}

	/* Assume not good */
	return (FALSE);
}


/* Hack -- inscribe items that a unique drops */
s16b unique_quark = 0;

/*
 * Attempt to place an object (normal or good/great) at the given location.
 *
 * This routine plays nasty games to generate the "special artifacts".
 *
 * This routine uses "object_level" for the "generation level".
 *
 * This routine requires a clean floor grid destination.
 */
//void place_object(struct worldpos *wpos, int y, int x, bool good, bool great)
void place_object(struct worldpos *wpos, int y, int x, bool good, bool great, obj_theme theme, int luck)
{
	int prob, base;

	object_type		forge;

	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;

	/* Paranoia -- check bounds */
	if (!in_bounds(y, x)) return;

	/* Require clean floor space */
//	if (!cave_clean_bold(zcave, y, x)) return;

	/* max luck = 40 */
	luck = 200 - (8000 / (luck + 40));
	if (!good && magik(luck / 2)) good = TRUE;
	else if (good && !great && magik(luck / 5)) {great = TRUE; good = TRUE;}

	/* Chance of "special object" */
	prob = (good ? 10 : 1000);

	/* Base level for the object */
	base = (good ? (object_level + 10) : object_level);


	/* Hack -- clear out the forgery */
	invwipe(&forge);

	/* Generate a special object, or a normal object */
	if ((rand_int(prob) != 0) || !make_artifact_special(wpos, &forge))
	{
		int k_idx;

		/* Good objects */
		if (good)
		{
			/* Activate restriction */
			get_obj_num_hook = kind_is_good;

			/* Prepare allocation table */
			get_obj_num_prep();
		}
		/* Normal objects */
		else
		{
			/* Select items based on "theme" */
			init_match_theme(theme);

			/* Activate normal restriction */
			get_obj_num_hook = kind_is_legal;

			/* Prepare allocation table */
			get_obj_num_prep();

			/* The table is synchronised */
//			alloc_kind_table_valid = TRUE;
		}


		/* Pick a random object */
		k_idx = get_obj_num(base);

		/* Good objects */
#if 0	// commented out for efficiency
		if (good)
		{
			/* Clear restriction */
			get_obj_num_hook = NULL;

			/* Prepare allocation table */
			get_obj_num_prep();
		}
#endif	// 0

		/* Handle failure */
		if (!k_idx) return;

		/* Prepare the object */
		invcopy(&forge, k_idx);
	}

	/* Apply magic (allow artifacts) */
	apply_magic(wpos, &forge, object_level, TRUE, good, great);

	/* Hack -- generate multiple spikes/missiles */
	if (!forge.name1)
		switch (forge.tval)
		{
			case TV_SPIKE:
				forge.number = damroll(6, 7);
				break;
			case TV_SHOT:
			case TV_ARROW:
			case TV_BOLT:
				forge.number = damroll(6,
						(forge.sval == SV_AMMO_MAGIC) ? 2 : (7 * (40 + randint(luck)) / 40));
		}

	/* Hack -- inscribe items that a unique drops */
	if (unique_quark) forge.note = unique_quark;

	drop_near(&forge, -1, wpos, y, x);

#if 0
	/* Make an object */
	o_idx = o_pop();

	/* Success */
	if (o_idx)
	{
		cave_type		*c_ptr;
		object_type		*o_ptr;
		int			i;

		o_ptr = &o_list[o_idx];

		(*o_ptr) = (forge);

		drop_near(o_ptr, -1, wpos, y, x);
		o_ptr->iy = y;
		o_ptr->ix = x;
		wpcopy(&o_ptr->wpos, wpos);

		c_ptr=&zcave[y][x];

		/* Build a stack */
		o_ptr->next_o_idx = c_ptr->o_idx;

		/* Place the object */
		c_ptr->o_idx = o_idx;

		/* Make sure no one sees it at first */
		for (i = 1; i < NumPlayers + 1; i++)
		{
			if (Players[i]->conn == NOT_CONNECTED)
				continue;

			/* He can't see it */
			Players[i]->obj_vis[o_idx] = FALSE;
		}
	}
#endif	// 0
}



/*
 * Scatter some "great" objects near the player
 */
void acquirement(struct worldpos *wpos, int y1, int x1, int num, bool great)
{
	int        y, x, i, d;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;

	/* Scatter some objects */
	for (; num > 0; --num)
	{
		/* Check near the player for space */
		for (i = 0; i < 25; ++i)
		{
			/* Increasing Distance */
			d = (i + 4) / 5;

			/* Pick a location */
			scatter(wpos, &y, &x, y1, x1, d, 0);

			/* Must have a clean grid */
			if (!cave_clean_bold(zcave, y, x)) continue;
			/* Place a good (or great) object */
			place_object(wpos, y, x, TRUE, great, default_obj_theme, 0);
			/* Notice */
			note_spot_depth(wpos, y, x);

			/* Redraw */
			everyone_lite_spot(wpos, y, x);

			/* Under the player */
			/*if ((y == py) && (x == px))
			{*/
				/* Message */
				/*msg_print ("You feel something roll beneath your feet.");
			}*/

			/* Placement accomplished */
			break;
		}
	}
}


/*
 * Places a random trap at the given location.
 *
 * The location must be a valid, empty, clean, floor grid.
 *
 * Note that all traps start out as "invisible" and "untyped", and then
 * when they are "discovered" (by detecting them or setting them off),
 * the trap is "instantiated" as a visible, "typed", trap.
 */
/*
 * obsoleted - please see traps.c	- Jir -
 */
#if 0
void place_trap(struct worldpos *wpos, int y, int x)
{
	cave_type *c_ptr;

	/* Paranoia -- verify location */
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
	if (!in_bounds(y, x)) return;
	/* Require empty, clean, floor grid */
	if (!cave_naked_bold(zcave, y, x)) return;

	/* Access the grid */
	c_ptr = &zcave[y][x];

	/* Place an invisible trap */
	c_ptr->feat = FEAT_INVIS;
}
#endif	// 0


/*
 * XXX XXX XXX Do not use these hard-coded values.
 */
#if 0	// ok so I don't use them :)
#define OBJ_GOLD_LIST	480	/* First "gold" entry */
#define MAX_GOLD	18	/* Number of "gold" entries */
#endif	// 0

/*
 * Places a treasure (Gold or Gems) at given location
 * The location must be a valid, empty, floor grid.
 */
void place_gold(struct worldpos *wpos, int y, int x)
{
	int		i;

	s32b	base;

	//cave_type	*c_ptr;
	//object_type *o_ptr;
	object_type	forge;

	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;

	/* Paranoia -- check bounds */
	if (!in_bounds(y, x)) return;

	/* Require clean floor grid */
//	if (!cave_clean_bold(zcave, y, x)) return;

	/* Hack -- Pick a Treasure variety */
	i = ((randint(object_level + 2) + 2) / 2);

	/* Apply "extra" magic */
	if (rand_int(GREAT_OBJ) == 0)
	{
		i += randint(object_level + 1);
	}

	/* Hack -- Creeping Coins only generate "themselves" */
	if (coin_type) i = coin_type + 1;

	/* Do not create "illegal" Treasure Types */
	if (i > SV_GOLD_MAX) i = SV_GOLD_MAX;

	invcopy(&forge, lookup_kind(TV_GOLD, i));

	/* Hack -- Base coin cost */
//	base = k_info[OBJ_GOLD_LIST+i].cost;
	base = k_info[lookup_kind(TV_GOLD, i)].cost;

	/* Determine how much the treasure is "worth" */
	forge.pval = (base + (8L * randint(base)) + randint(8));

	/* Drop it */
	drop_near(&forge, -1, wpos, y, x);
}



/*
 * Let an item 'o_ptr' fall to the ground at or near (y,x).
 * The initial location is assumed to be "in_bounds()".
 *
 * This function takes a parameter "chance".  This is the percentage
 * chance that the item will "disappear" instead of drop.  If the object
 * has been thrown, then this is the chance of disappearance on contact.
 *
 * Hack -- this function uses "chance" to determine if it should produce
 * some form of "description" of the drop event (under the player).
 *
 * This function should probably be broken up into a function to determine
 * a "drop location", and several functions to actually "drop" an object.
 *
 * XXX XXX XXX Consider allowing objects to combine on the ground.
 */
/* XXX XXX XXX DIRTY! DIRTY! DIRTY!		- Jir - */
s16b drop_near(object_type *o_ptr, int chance, struct worldpos *wpos, int y, int x)
{
	int		k, d, ny, nx, i, s;	// , y1, x1
	int bs, bn;
	int by, bx;
	//int ty, tx;
	int o_idx=-1;
	int flag = 0;	// 1 = normal, 2 = combine, 3 = crash

	bool inside_house = FALSE;

	cave_type	*c_ptr;

	bool arts = artifact_p(o_ptr), crash, done = FALSE;
	u16b this_o_idx, next_o_idx = 0;

	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(-1);

	/* Handle normal "breakage" */
	if (!arts && magik(chance))
	{
#if 0
		/* Message */
		msg_format("The %s disappear%s.",
			   o_name, (plural ? "" : "s"));

		/* Debug */
		if (wizard) msg_print("(breakage)");
#endif	// 0
		/* Failure */
		return (-1);
	}

#if 0
	/* Start at the drop point */
	ny = y1 = y;  nx = x1 = x;

	/* See if the object "survives" the fall */
//	if (artifact_p(o_ptr) || (rand_int(100) >= chance))
	{
		/* Start at the drop point */
		ny = y1 = y; nx = x1 = x;

		/* Try (20 times) to find an adjacent usable location */
		for (k = 0; !flag && (k < 50); ++k)
		{
			/* Distance distribution */
			d = ((k + 14) / 15);

			/* Pick a "nearby" location */
			scatter(wpos, &ny, &nx, y1, x1, d, 0);
			/* Require clean floor space */
			if (!cave_clean_bold(zcave, ny, nx)) continue;

			/* Here looks good */
			flag = TRUE;
		}
	}

	/* Try really hard to place an artifact */
//	if (!flag && artifact_p(o_ptr))
	{
		/* Start at the drop point */
		ny = y1 = y;  nx = x1 = x;

		/* Try really hard to drop it */
		for (k = 0; !flag && (k < (artifact_p(o_ptr) ? 1000 : 20)); k++)
		{
			d = 1;

			/* Pick a location */
			scatter(wpos, &ny, &nx, y1, x1, d, 0);

			/* Do not move through walls */
			if (!cave_floor_bold(zcave, ny, nx)) continue;

			/* Hack -- "bounce" to that location */
			y1 = ny; x1 = nx;

			/* Get the cave grid */
			c_ptr = &zcave[ny][nx];

			/* XXX XXX XXX */

			/* Nothing here?  Use it */
			if (!(c_ptr->o_idx)) flag = TRUE;

			/* After trying 99 places, crush any (normal) object */
			else if ((k>99) && cave_valid_bold(zcave, ny, nx)) flag = TRUE;
		}

		/* Hack -- Artifacts will destroy ANYTHING to stay alive */
		if (!flag)
		{
			/* Location */
			ny = y;
			nx = x;

			/* Always okay */
			flag = TRUE;

			/* Description */
			/*object_desc(o_name, o_ptr, FALSE, 0);*/

			/* Message */
			/*msg_format("The %s crashes to the floor.", o_name);*/
		}
	}
#endif	// 0

	/* Score */
	bs = -1;

	/* Picker */
	bn = 0;

	/* Default */
	by = y;
	bx = x;

	d = 0;

	/* Scan local grids */
	for (i = 0; i < tdi[3]; i++)
	{
		bool comb = FALSE;

		if (i >= tdi[d]) d++;

		/* Location */
		ny = y + tdy[i];
		nx = x + tdx[i];

		/* Skip illegal grids */
		if (!in_bounds(ny, nx)) continue;

		/* Require line of sight */
		if (!los(wpos, y, x, ny, nx)) continue;

		/* Require floor space (or shallow terrain) -KMW- */
//		if (!(f_info[c_ptr->feat].flags1 & FF1_FLOOR)) continue;
		if (!cave_floor_bold(zcave, ny, nx) ||
				cave_perma_bold(zcave, ny, nx)) continue;

		/* Obtain grid */
		c_ptr = &zcave[ny][nx];

		/* No traps */
//		if (c_ptr->t_idx) continue;

		/* No objects */
		k = 0;

		/* Scan objects in that grid */
		for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
		{
			object_type *j_ptr;

			/* Acquire object */
			j_ptr = &o_list[this_o_idx];

			/* Acquire next object */
			next_o_idx = j_ptr->next_o_idx;

			/* Check for possible combination */
			if (object_similar(0, o_ptr, j_ptr)) comb = TRUE;

			/* Count objects */
			k++;
		}

		/* Add new object */
		if (!comb) k++;

		/* No stacking (allow combining) */
//		if (!testing_stack && (k > 1)) continue;

		/* Hack -- no stacking inside houses */
		/* XXX this can cause 'arts crashes arts' */
		crash = (!wpos->wz && k > 1 && !comb && (c_ptr->info&CAVE_ICKY));
		if (!arts && crash) continue;

		/* Paranoia */
		if (k > 99) continue;

		/* Calculate score */
		s = 10000 - (d + k * 5 + (crash ? 2000 : 0));

		/* Skip bad values */
		if (s < bs) continue;

		/* New best value */
		if (s > bs) bn = 0;

		/* Apply the randomizer to equivalent values */
		if ((++bn >= 2) && (rand_int(bn) != 0)) continue;

		/* Keep score */
		bs = s;

		/* Track it */
		by = ny;
		bx = nx;

		/* Okay */
		flag = crash ? 3 : (comb ? 2 : 1);
	}

	/* Poor little object */
	if (!flag)
	{
		/* Describe */
		/*object_desc(o_name, o_ptr, FALSE, 0);*/

		/* Message */
		/*msg_format("The %s disappear%s.",
		           o_name, ((o_ptr->number == 1) ? "s" : ""));*/
		return (-1);
	}

	ny = by;
	nx = bx;
	c_ptr = &zcave[ny][nx];

	/* Artifact always disappears, depending on tomenet.cfg flags */
	/* this should be in drop_near_severe, would be cleaner sometime in the future.. */
	//if (true_artifact_p(o_ptr) && !(p_ptr->admin_dm || p_ptr->admin_wiz) && cfg.anti_arts_house && ((pick_house(wpos, ny, nx) != -1))
	for (i = 0; i < num_houses; i++)
		/* anyone willing to implement non-rectangular houses? */
		if ((houses[i].flags & HF_RECT) && inarea(&houses[i].wpos, wpos))
			for (bx = houses[i].x; bx < (houses[i].x + houses[i].coords.rect.width - 1); bx++)
				for (by = houses[i].y; by < (houses[i].y + houses[i].coords.rect.height - 1); by++)
					if ((bx == nx) && (by == ny)) inside_house = TRUE;
	if (true_artifact_p(o_ptr) && cfg.anti_arts_house && inside_house)
	{
		//char	o_name[160];
		//object_desc(Ind, o_name, o_ptr, TRUE, 0);
		//msg_format(Ind, "%s fades into the air!", o_name);
		a_info[o_ptr->name1].cur_num = 0;
		a_info[o_ptr->name1].known = FALSE;
		return (-1);
	}

	/* Scan objects in that grid for combination */
	if (flag == 2)
	for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *q_ptr;

		/* Acquire object */
		q_ptr = &o_list[this_o_idx];

		/* Acquire next object */
		next_o_idx = q_ptr->next_o_idx;

		/* Check for combination */
		if (object_similar(0, q_ptr, o_ptr))
		{
			/* Combine the items */
			object_absorb(0, q_ptr, o_ptr);

			/* Success */
			done = TRUE;

			/* Done */
			break;
		}
	}

	/* Successful drop */
//	if (flag)
	else
	{
		/* Assume fails */
//		flag = FALSE;

		/* XXX XXX XXX */

//		c_ptr = &zcave[ny][nx];

		/* Crush anything under us (for artifacts) */
		if (flag == 3) delete_object(wpos, ny, nx, TRUE);

		/* Make a new object */
		o_idx = o_pop();

		/* Success */
		if (o_idx)
		{
			/* Structure copy */
			o_list[o_idx] = *o_ptr;

			/* Access */
			o_ptr = &o_list[o_idx];

			/* Locate */
			o_ptr->iy = ny;
			o_ptr->ix = nx;
			wpcopy(&o_ptr->wpos,wpos);

			/* reset scan_objs timer */
			o_ptr->marked = 0;

			/* No monster */
			o_ptr->held_m_idx = 0;

			/* Build a stack */
			o_ptr->next_o_idx = c_ptr->o_idx;

			/* Place */
//			c_ptr = &zcave[ny][nx];
			c_ptr->o_idx = o_idx;

			/* Clear visibility flags */
			for (k = 1; k <= NumPlayers; k++)
			{
				/* This player cannot see it */
				Players[k]->obj_vis[o_idx] = FALSE;
			}

			/* Note the spot */
			note_spot_depth(wpos, ny, nx);

			/* Draw the spot */
			everyone_lite_spot(wpos, ny, nx);

			/* Sound */
			/*sound(SOUND_DROP);*/

			/* Mega-Hack -- no message if "dropped" by player */
			/* Message when an object falls under the player */
			/*if (chance && (ny == py) && (nx == px))
			{
				msg_print("You feel something roll beneath your feet.");
			}*/

			if (chance && c_ptr->m_idx < 0)
			{
				msg_print(0 - c_ptr->m_idx, "You feel something roll beneath your feet.");
			}

			/* Success */
//			flag = TRUE;
		}
	}

	/* Result */
	return (o_idx);
}
/* This function make the artifact disapper at once (cept randarts),
 * and call the normal dropping function otherwise.
 */

s16b drop_near_severe(int Ind, object_type *o_ptr, int chance, struct worldpos *wpos, int y, int x)
{
	player_type *p_ptr=Players[Ind];

	/* Artifact always disappears, depending on tomenet.cfg flags */
	if (true_artifact_p(o_ptr) && !(p_ptr->admin_dm || p_ptr->admin_wiz) && cfg.anti_arts_hoard)
	    //(cfg.anti_arts_hoard || (cfg.anti_arts_house && 0)) would be cleaner sometime in the future..
	{
		char	o_name[160];
		object_desc(Ind, o_name, o_ptr, TRUE, 0);

		msg_format(Ind, "%s fades into the air!", o_name);
		a_info[o_ptr->name1].cur_num = 0;
		a_info[o_ptr->name1].known = FALSE;
		return -1;
	}
	else return(drop_near(o_ptr,chance,wpos,y,x));
}



/*
 * Hack -- instantiate a trap
 *
 * XXX XXX XXX This routine should be redone to reflect trap "level".
 * That is, it does not make sense to have spiked pits at 50 feet.
 * Actually, it is not this routine, but the "trap instantiation"
 * code, which should also check for "trap doors" on quest levels.
 */
/* The note above is completely obsoleted.	- Jir -	*/
void pick_trap(struct worldpos *wpos, int y, int x)
{
//	int feat;
//	int tries = 100;

	cave_type **zcave;
	cave_type *c_ptr;
	struct c_special *cs_ptr;
	if(!(zcave=getcave(wpos))) return;
	c_ptr = &zcave[y][x];

#if 0
	/* Paranoia */
	if (c_ptr->feat != FEAT_INVIS) return;

	/* Pick a trap */
	while (--tries)
	{
		/* Hack -- pick a trap */
		feat = FEAT_TRAP_HEAD + rand_int(16);

		/* Hack -- no trap doors on quest levels */
		if ((feat == FEAT_TRAP_HEAD + 0x00) && is_quest(wpos)) continue;

		/* Hack -- no trap doors on the deepest level */
                if ((feat == FEAT_TRAP_HEAD + 0x00) && can_go_down(wpos))
		continue;

		/* Done */
		break;
	}

	/* Activate the trap */
	c_ptr->feat = feat;
#endif	// 0

	/* Paranoia */
	if (!(cs_ptr=GetCS(c_ptr, CS_TRAPS))) return;

	cs_ptr->sc.trap.found = TRUE;

	/* Notice */
	note_spot_depth(wpos, y, x);

	/* Redraw */
	everyone_lite_spot(wpos, y, x);
}

/*
 * Divide 'stacked' wands.	- Jir -
 * o_ptr->number is not changed here!
 */
#if 0
object_type *divide_charged_item(object_type *o_ptr, int amt)
{
	object_type forge;
	object_type *q_ptr = &forge;

	/* Paranoia */
	if (o_ptr->number < amt) return (NULL);

	/* Obtain local object */
	object_copy(q_ptr, o_ptr);

	/* Modify quantity */
	q_ptr->number = amt;

	if (o_ptr->tval == TV_WAND)
	{
		q_ptr->pval = o_ptr->pval * amt / o_ptr->number;
		if (amt < o_ptr->number) o_ptr->pval -= q_ptr->pval;
	}

	return (q_ptr);
}
#else	// 0
int divide_charged_item(object_type *o_ptr, int amt)
{
	int charge = 0;

	/* Paranoia */
	if (o_ptr->number < amt) return (-1);

	if (o_ptr->tval == TV_WAND)
	{
		charge = o_ptr->pval * amt / o_ptr->number;
		if (amt < o_ptr->number) o_ptr->pval -= charge;
	}

	return (charge);
}
#endif	// 0


/*
 * Describe the charges on an item in the inventory.
 */
void inven_item_charges(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];

	object_type *o_ptr = &p_ptr->inventory[item];

	/* Require staff/wand */
	if ((o_ptr->tval != TV_STAFF) && (o_ptr->tval != TV_WAND)) return;

	/* Require known item */
	if (!object_known_p(Ind, o_ptr)) return;

	/* Multiple charges */
	if (o_ptr->pval != 1)
	{
		/* Print a message */
		msg_format(Ind, "You have %d charges remaining.", o_ptr->pval);
	}

	/* Single charge */
	else
	{
		/* Print a message */
		msg_format(Ind, "You have %d charge remaining.", o_ptr->pval);
	}
}


/*
 * Describe an item in the inventory.
 */
void inven_item_describe(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];

	object_type	*o_ptr = &p_ptr->inventory[item];

	char	o_name[160];

	/* Hack -- suppress msg */
	if (p_ptr->taciturn_messages) return;

	/* Get a description */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);

	/* Print a message */
	msg_format(Ind, "You have %s.", o_name);
}


/*
 * Increase the "number" of an item in the inventory
 */
void inven_item_increase(int Ind, int item, int num)
{
	player_type *p_ptr = Players[Ind];

	object_type *o_ptr = &p_ptr->inventory[item];

	/* Apply */
	num += o_ptr->number;

	/* Bounds check */
	if (num > 255) num = 255;
	else if (num < 0) num = 0;

	/* Un-apply */
	num -= o_ptr->number;

	/* Change the number and weight */
	if (num)
	{
		/* Add the number */
		o_ptr->number += num;

		/* Add the weight */
		p_ptr->total_weight += (num * o_ptr->weight);

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Recalculate mana XXX */
		p_ptr->update |= (PU_MANA);

		/* Combine the pack */
		p_ptr->notice |= (PN_COMBINE);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
	}
}


/*
 * Erase an inventory slot if it has no more items
 */
bool inven_item_optimize(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];

	object_type *o_ptr = &p_ptr->inventory[item];

	/* Only optimize real items */
	if (!o_ptr->k_idx) return (FALSE);

	/* Only optimize empty items */
	if (o_ptr->number) return (FALSE);

	/* The item is in the pack */
	if (item < INVEN_WIELD)
	{
		int i;

		/* One less item */
		p_ptr->inven_cnt--;

		/* Slide everything down */
		for (i = item; i < INVEN_PACK; i++)
		{
			/* Structure copy */
			p_ptr->inventory[i] = p_ptr->inventory[i+1];
		}

		/* Erase the "final" slot */
		invwipe(&p_ptr->inventory[i]);
	}

	/* The item is being wielded */
	else
	{
		/* One less item */
		p_ptr->equip_cnt--;

		/* Erase the empty slot */
		invwipe(&p_ptr->inventory[item]);

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Recalculate torch */
		p_ptr->update |= (PU_TORCH);

		/* Recalculate mana XXX */
		p_ptr->update |= (PU_MANA);
	}

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	return (TRUE);
}


/*
 * Describe the charges on an item on the floor.
 */
void floor_item_charges(int item)
{
	object_type *o_ptr = &o_list[item];

	/* Require staff/wand */
	if ((o_ptr->tval != TV_STAFF) && (o_ptr->tval != TV_WAND)) return;

	/* Require known item */
	/*if (!object_known_p(o_ptr)) return;*/

	/* Multiple charges */
	if (o_ptr->pval != 1)
	{
		/* Print a message */
		/*msg_format("There are %d charges remaining.", o_ptr->pval);*/
	}

	/* Single charge */
	else
	{
		/* Print a message */
		/*msg_format("There is %d charge remaining.", o_ptr->pval);*/
	}
}



/*
 * Describe an item in the inventory.
 */
void floor_item_describe(int item)
{
	/* Get a description */
	/*object_desc(o_name, o_ptr, TRUE, 3);*/

	/* Print a message */
	/*msg_format("You see %s.", o_name);*/
}


/*
 * Increase the "number" of an item on the floor
 */
void floor_item_increase(int item, int num)
{
	object_type *o_ptr = &o_list[item];

	/* Apply */
	num += o_ptr->number;

	/* Bounds check */
	if (num > 255) num = 255;
	else if (num < 0) num = 0;

	/* Un-apply */
	num -= o_ptr->number;

	/* Change the number */
	o_ptr->number += num;
}


/*
 * Optimize an item on the floor (destroy "empty" items)
 */
void floor_item_optimize(int item)
{
	object_type *o_ptr = &o_list[item];

	/* Paranoia -- be sure it exists */
	if (!o_ptr->k_idx) return;

	/* Only optimize empty items */
	if (o_ptr->number) return;

	/* Delete it */
	delete_object_idx(item, TRUE);
}


/*
 * Inscribe the items automatically.	- Jir -
 * if 'flags' is non-0, overwrite existing inscriptions.
 *
 * TODO: inscribe item's power like {+StCo[FiAc;FASI}
 */
void auto_inscribe(int Ind, object_type *o_ptr, int flags)
{
	player_type *p_ptr = Players[Ind];
#if 0
	char c[] = "@m ";
#endif

	if (!o_ptr->tval) return;

	/* skip inscribed items */
	if (!flags && o_ptr->note &&
			strcmp(quark_str(o_ptr->note), "on sale"))
		return;

	if (p_ptr->obj_aware[o_ptr->k_idx] &&
			((o_ptr->tval == TV_SCROLL &&
			  o_ptr->sval == SV_SCROLL_WORD_OF_RECALL) ||
			 (o_ptr->tval == TV_ROD &&
			  o_ptr->sval == SV_ROD_RECALL)))
	{
		o_ptr->note = quark_add("@R");
		return;
	}

#if 0	/* disabled till new spell system is done */
	if (!is_book(o_ptr) && o_ptr->tval != TV_BOOK) return;

	/* XXX though it's ok with 'm' for everything.. */
	c[2] = o_ptr->sval +1 +48;
	o_ptr->note = quark_add(c);
#endif	// 0

	/* auto-pickup inscription for ammo */
	if ((o_ptr->tval == TV_ARROW ||
		o_ptr->tval == TV_BOLT ||
		o_ptr->tval == TV_SHOT) &&
		object_known_p(Ind, o_ptr))
	{
		o_ptr->note = quark_add("!=");
		return;
	}
}



/*
 * Check if we have space for an item in the pack without overflow
 */
bool inven_carry_okay(int Ind, object_type *o_ptr)
{
	player_type *p_ptr = Players[Ind];

	int i;

	/* Empty slot? */
	if (p_ptr->inven_cnt < INVEN_PACK) return (TRUE);

	/* Similar slot? */
	for (i = 0; i < INVEN_PACK; i++)
	{
		/* Get that item */
		object_type *j_ptr = &p_ptr->inventory[i];

		/* Check if the two items can be combined */
		if (object_similar(Ind, j_ptr, o_ptr)) return (TRUE);
	}

	/* Hack -- try quiver slot (see inven_carry) */
//	if (object_similar(Ind, &p_ptr->inventory[INVEN_AMMO], o_ptr)) return (TRUE);

	/* Nope */
	return (FALSE);
}


/*
 * Add an item to the players inventory, and return the slot used.
 *
 * If the new item can combine with an existing item in the inventory,
 * it will do so, using "object_similar()" and "object_absorb()", otherwise,
 * the item will be placed into the "proper" location in the inventory.
 *
 * This function can be used to "over-fill" the player's pack, but only
 * once, and such an action must trigger the "overflow" code immediately.
 * Note that when the pack is being "over-filled", the new item must be
 * placed into the "overflow" slot, and the "overflow" must take place
 * before the pack is reordered, but (optionally) after the pack is
 * combined.  This may be tricky.  See "dungeon.c" for info.
 */
s16b inven_carry(int Ind, object_type *o_ptr)
{
	player_type *p_ptr = Players[Ind];

	int         i, j, k;
	int		n = -1;

	object_type	*j_ptr;
        u32b f1 = 0 , f2 = 0 , f3 = 0, f4 = 0, f5 = 0, esp = 0;

#if 0	// This code makes it impossible to take ammos off, pfft
	/* Try to add to the quiver */
	if (object_similar(Ind, &p_ptr->inventory[INVEN_AMMO], o_ptr))
	{
		msg_print(Ind, "You add the ammo to your quiver.");

		/* Combine the items */
		object_absorb(Ind, &p_ptr->inventory[INVEN_AMMO], o_ptr);

		/* Increase the weight */
		p_ptr->total_weight += (o_ptr->number * o_ptr->weight);

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

		/* Success */
		return (INVEN_AMMO);
	}
#endif	// 0

	/* Check for combining */
	for (j = 0; j < INVEN_PACK; j++)
	{
		j_ptr = &p_ptr->inventory[j];

		/* Skip empty items */
		if (!j_ptr->k_idx) continue;

		/* Hack -- track last item */
		n = j;

		/* Check if the two items can be combined */
		if (object_similar(Ind, j_ptr, o_ptr))
		{
			/* Combine the items */
			object_absorb(Ind, j_ptr, o_ptr);

			/* Increase the weight */
			p_ptr->total_weight += (o_ptr->number * o_ptr->weight);

			/* Recalculate bonuses */
			p_ptr->update |= (PU_BONUS);

			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

			/* Success */
			return (j);
		}
	}


	/* Paranoia */
	if (p_ptr->inven_cnt > INVEN_PACK) return (-1);


	/* Find an empty slot */
	for (j = 0; j <= INVEN_PACK; j++)
	{
		j_ptr = &p_ptr->inventory[j];

		/* Use it if found */
		if (!j_ptr->k_idx) break;
	}

	/* Use that slot */
	i = j;


	/* Hack -- pre-reorder the pack */
	if (i < INVEN_PACK)
	{
		s32b		o_value, j_value;

		/* Get the "value" of the item */
		o_value = object_value(Ind, o_ptr);

		/* Scan every occupied slot */
		for (j = 0; j < INVEN_PACK; j++)
		{
			j_ptr = &p_ptr->inventory[j];

			/* Use empty slots */
			if (!j_ptr->k_idx) break;
#if 0 // can read all
			/* Hack -- readable books always come first */
			if ((o_ptr->tval == p_ptr->mp_ptr->spell_book) &&
			    (j_ptr->tval != p_ptr->mp_ptr->spell_book)) break;
			if ((j_ptr->tval == p_ptr->mp_ptr->spell_book) &&
			    (o_ptr->tval != p_ptr->mp_ptr->spell_book)) continue;
#endif
			/* Objects sort by decreasing type */
			if (o_ptr->tval > j_ptr->tval) break;
			if (o_ptr->tval < j_ptr->tval) continue;

			/* Non-aware (flavored) items always come last */
			if (!object_aware_p(Ind, o_ptr)) continue;
			if (!object_aware_p(Ind, j_ptr)) break;

			/* Objects sort by increasing sval */
			if (o_ptr->sval < j_ptr->sval) break;
			if (o_ptr->sval > j_ptr->sval) continue;

			/* Unidentified objects always come last */
			if (!object_known_p(Ind, o_ptr)) continue;
			if (!object_known_p(Ind, j_ptr)) break;

			/* Determine the "value" of the pack item */
			j_value = object_value(Ind, j_ptr);

			/* Objects sort by decreasing value */
			if (o_value > j_value) break;
			if (o_value < j_value) continue;
		}

		/* Use that slot */
		i = j;

		/* Structure slide (make room) */
		for (k = n; k >= i; k--)
		{
			/* Hack -- Slide the item */
			p_ptr->inventory[k+1] = p_ptr->inventory[k];
		}

		/* Paranoia -- Wipe the new slot */
		invwipe(&p_ptr->inventory[i]);
	}

        if (!o_ptr->owner && !p_ptr->admin_dm) o_ptr->owner = p_ptr->id;
#if 0
        if (!o_ptr->level)
        {
                if (p_ptr->dun_depth > 0) o_ptr->level = p_ptr->dun_depth;
                else o_ptr->level = -p_ptr->dun_depth;
                if (o_ptr->level > 100) o_ptr->level = 100;
        }
#endif

	/* Auto-inscriber */
#ifdef	AUTO_INSCRIBER
	if (p_ptr->auto_inscribe)
	{
		auto_inscribe(Ind, o_ptr, 0);
	}
#endif	// AUTO_INSCRIBER
			


	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	/* Auto Curse */
	if (f3 & TR3_AUTO_CURSE)
	{
		/* The object recurse itself ! */
		o_ptr->ident |= ID_CURSED;
	}

	/* Structure copy to insert the new item */
	p_ptr->inventory[i] = (*o_ptr);

	/* Forget the old location */
	p_ptr->inventory[i].iy = p_ptr->inventory[i].ix = 0;
	p_ptr->inventory[i].wpos.wx = 0;
	p_ptr->inventory[i].wpos.wy = 0;
	p_ptr->inventory[i].wpos.wz = 0;
	/* Clean out unused fields */
	p_ptr->inventory[i].next_o_idx = 0;
	p_ptr->inventory[i].held_m_idx = 0;


	/* Increase the weight, prepare to redraw */
	p_ptr->total_weight += (o_ptr->number * o_ptr->weight);

	/* Count the items */
	p_ptr->inven_cnt++;

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Reorder pack */
	p_ptr->notice |= (PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Return the slot */
	return (i);
}




/*
 * Combine items in the pack
 *
 * Note special handling of the "overflow" slot
 */
void combine_pack(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int		i, j, k;

	object_type	*o_ptr;
	object_type	*j_ptr;

	bool	flag = FALSE;


	/* Combine the pack (backwards) */
	for (i = INVEN_PACK; i > 0; i--)
	{
		/* Get the item */
		o_ptr = &p_ptr->inventory[i];

		/* Skip empty items */
		if (!o_ptr->k_idx) continue;


		/* Auto id ? */
		if (p_ptr->auto_id)
		  {
		    object_aware(Ind, o_ptr);
		    object_known(o_ptr);

		    /* Window stuff */
		    p_ptr->window |= (PW_INVEN | PW_EQUIP);
		  }

		/* Scan the items above that item */
		for (j = 0; j < i; j++)
		{
			/* Get the item */
			j_ptr = &p_ptr->inventory[j];

			/* Skip empty items */
			if (!j_ptr->k_idx) continue;

			/* Can we drop "o_ptr" onto "j_ptr"? */
			if (object_similar(Ind, j_ptr, o_ptr))
			{
				/* Take note */
				flag = TRUE;

				/* Add together the item counts */
				object_absorb(Ind, j_ptr, o_ptr);

				/* One object is gone */
				p_ptr->inven_cnt--;

				/* Slide everything down */
				for (k = i; k < INVEN_PACK; k++)
				{
					/* Structure copy */
					p_ptr->inventory[k] = p_ptr->inventory[k+1];
				}

				/* Erase the "final" slot */
				invwipe(&p_ptr->inventory[k]);

				/* Window stuff */
				p_ptr->window |= (PW_INVEN | PW_EQUIP);

				/* Done */
				break;
			}
		}
	}

	/* Message */
	if (flag) msg_print(Ind, "You combine some items in your pack.");
}


/*
 * Reorder items in the pack
 *
 * Note special handling of the "overflow" slot
 *
 * Note special handling of empty slots  XXX XXX XXX XXX
 */
void reorder_pack(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int		i, j, k;

	s32b	o_value;
	s32b	j_value;

	object_type *o_ptr;
	object_type *j_ptr;

	object_type	temp;

	bool	flag = FALSE;


	/* Re-order the pack (forwards) */
	for (i = 0; i < INVEN_PACK; i++)
	{
		/* Mega-Hack -- allow "proper" over-flow */
		if ((i == INVEN_PACK) && (p_ptr->inven_cnt == INVEN_PACK)) break;

		/* Get the item */
		o_ptr = &p_ptr->inventory[i];

		/* Skip empty slots */
		if (!o_ptr->k_idx) continue;

		/* Get the "value" of the item */
		o_value = object_value(Ind, o_ptr);

		/* Scan every occupied slot */
		for (j = 0; j < INVEN_PACK; j++)
		{
			/* Get the item already there */
			j_ptr = &p_ptr->inventory[j];

			/* Use empty slots */
			if (!j_ptr->k_idx) break;
#if 0 // all can read all
			/* Hack -- readable books always come first */
			if ((o_ptr->tval == p_ptr->mp_ptr->spell_book) &&
			    (j_ptr->tval != p_ptr->mp_ptr->spell_book)) break;
			if ((j_ptr->tval == p_ptr->mp_ptr->spell_book) &&
			    (o_ptr->tval != p_ptr->mp_ptr->spell_book)) continue;
#endif
			/* Objects sort by decreasing type */
			if (o_ptr->tval > j_ptr->tval) break;
			if (o_ptr->tval < j_ptr->tval) continue;

			/* Non-aware (flavored) items always come last */
			if (!object_aware_p(Ind, o_ptr)) continue;
			if (!object_aware_p(Ind, j_ptr)) break;

			/* Objects sort by increasing sval */
			if (o_ptr->sval < j_ptr->sval) break;
			if (o_ptr->sval > j_ptr->sval) continue;

			/* Unidentified objects always come last */
			if (!object_known_p(Ind, o_ptr)) continue;
			if (!object_known_p(Ind, j_ptr)) break;

			/* Determine the "value" of the pack item */
			j_value = object_value(Ind, j_ptr);

			/* Objects sort by decreasing value */
			if (o_value > j_value) break;
			if (o_value < j_value) continue;
		}

		/* Never move down */
		if (j >= i) continue;

		/* Take note */
		flag = TRUE;

		/* Save the moving item */
		temp = p_ptr->inventory[i];

		/* Structure slide (make room) */
		for (k = i; k > j; k--)
		{
			/* Slide the item */
			p_ptr->inventory[k] = p_ptr->inventory[k-1];
		}

		/* Insert the moved item */
		p_ptr->inventory[j] = temp;

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);
	}

	/* Message */
	if (flag) msg_print(Ind, "You reorder some items in your pack.");
}




/*
 * Hack -- process the objects
 */
/* TODO: terrain effects (lava burns scrolls etc) */
void process_objects(void)
{
	int i, k;

	object_type *o_ptr;


	/* Hack -- only every ten game turns */
//	if ((turn % 10) != 5) return;


	/* Process objects */
	for (k = o_top - 1; k >= 0; k--)
	{
		/* Access index */
		i = o_fast[k];

		/* Access object */
		o_ptr = &o_list[i];

		/* Excise dead objects */
		if (!o_ptr->k_idx)
		{
			/* Excise it */
			o_fast[k] = o_fast[--o_top];

			/* Skip */
			continue;
		}

		/* Recharge rods on the ground */
		if ((o_ptr->tval == TV_ROD) && (o_ptr->pval)) o_ptr->pval--;
	}


#ifdef SHIMMER_OBJECTS

#if 0
	/* Optimize */
	if (!avoid_other && scan_objects)
	{
		/* Process the objects */
		for (i = 1; i < o_max; i++)
		{
			o_ptr = &o_list[i];

			/* Skip dead objects */
			if (!o_ptr->k_idx) continue;

			/* XXX XXX XXX Skip unseen objects */

			/* Shimmer Multi-Hued Objects XXX XXX XXX */
			lite_spot(o_ptr->iy, o_ptr->ix);
		}

		/* Clear the flag */
		scan_objects = FALSE;
	}
#endif

#endif

}

/*
 * Set the "o_idx" fields in the cave array to correspond
 * to the objects in the "o_list".
 */
void setup_objects(void)
{
	int i;
	cave_type **zcave;

	for (i = 0; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip objects on depths that aren't allocated */
		if (!(zcave=getcave(&o_ptr->wpos))) continue;

		/* Skip carried objects */
		if (o_ptr->held_m_idx) continue;

//		if (!in_bounds2(&o_ptr->wpos, o_ptr->iy, o_ptr->ix)) continue;
		if (in_bounds_array(o_ptr->iy, o_ptr->ix))

#if 0	// excise_object_idx() should do this
		/* Build the stack */
		if (j = zcave[o_ptr->iy][o_ptr->ix].o_idx)
			o_ptr->next_o_idx = j;
		else o_ptr->next_o_idx = 0;
#endif	// 0

		/* Set the o_idx correctly */
		zcave[o_ptr->iy][o_ptr->ix].o_idx = i;
	}
}



/*
 * Wipe an object clean.
 */
void object_wipe(object_type *o_ptr)
{
	/* Wipe the structure */
	o_ptr=WIPE(o_ptr, object_type);
}


/*
 * Prepare an object based on an existing object
 */
void object_copy(object_type *o_ptr, object_type *j_ptr)
{
	/* Copy the structure */
	COPY(o_ptr, j_ptr, object_type);
}


/* ToME function -- not used for now */
#if 0
/*
 * Let the floor carry an object
 */
s16b floor_carry(worldpos *wpos, int y, int x, object_type *j_ptr)
{
	int n = 0;

	s16b o_idx;

	s16b this_o_idx, next_o_idx = 0;


	/* Scan objects in that grid for combination */
	for (this_o_idx = cave[y][x].o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Acquire object */
		o_ptr = &o_list[this_o_idx];

		/* Acquire next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Check for combination */
		if (object_similar(o_ptr, j_ptr))
		{
			/* Combine the items */
			object_absorb(o_ptr, j_ptr);

			/* Result */
			return (this_o_idx);
		}

		/* Count objects */
		n++;
	}


	/* Make an object */
	o_idx = o_pop();

	/* Success */
	if (o_idx)
	{
		object_type *o_ptr;

		/* Acquire object */
		o_ptr = &o_list[o_idx];

		/* Structure Copy */
		object_copy(o_ptr, j_ptr);

		/* Location */
		o_ptr->iy = y;
		o_ptr->ix = x;

		/* Forget monster */
		o_ptr->held_m_idx = 0;

		/* Build a stack */
		o_ptr->next_o_idx = cave[y][x].o_idx;

		/* Place the object */
		cave[y][x].o_idx = o_idx;

		/* Notice */
		note_spot(y, x);

		/* Redraw */
		lite_spot(y, x);
	}

	/* Result */
	return (o_idx);
}
#endif	// 0
