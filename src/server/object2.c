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

/* At 50% there were too many cursed jewelry in general in my opinion, using a macro now - C. Blue */
#define CURSED_JEWELRY_CHANCE	25

/* Add extra price bonus to randart armour/weapon which has especially 'useful' mod combo.
   Pretty experimental and totally optional. Just disable in case some randarts end up with
   outrageous prices. - C. Blue */
#define RANDART_PRICE_BONUS


/*
 * Excise a dungeon object from any stacks
 * Borrowed from ToME.
 */
void excise_object_idx(int o_idx)
{
	object_type *j_ptr, *o_ptr;

	u16b this_o_idx, next_o_idx = 0;

	u16b prev_o_idx = 0;
	
	int i;


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
		
	        /* Fix visibility of item pile - C. Blue
		   If object was visible to a player, then the top object of that pile becomes visible now! */
		if (c_ptr->o_idx) /* any object left at all? */
	    		for (i = 1; i <= NumPlayers; i++) /* FIX_PILE_VISIBILITY_DEBUG */
	    			if (Players[i]->obj_vis[o_idx])
					Players[i]->obj_vis[c_ptr->o_idx] = TRUE;
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
	struct worldpos *wpos = &o_ptr->wpos;

	//cave_type *c_ptr;

	/* Artifact becomes 'not found' status */
	if (true_artifact_p(o_ptr) && unfound_art)
	{
		handle_art_d(o_ptr->name1);
	}

	/* Excise */
	excise_object_idx(o_idx);

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
/*
 * Debugging is twice as hard as writing the code in the first
 * place. Therefore, if you write the code as cleverly as possible,
 * you are, by definition, not smart enough to debug it.
 * -- Brian W. Kernighan
 */
void compact_objects(int size, bool purge)
{
	int i, y, x, num, cnt, Ind; //, j, ny, nx;

	s32b cur_val, cur_lev, cur_dis, chance;
	struct worldpos *wpos;
	cave_type **zcave;
//	object_type *q_ptr;
	cave_type *c_ptr;

	int tmp_max=o_max;


	/* Compact */
	if (size)
	{
		/* Message */
		s_printf("Compacting objects...\n");
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

			/* Hack -- only compact items in houses or vaults in emergencies */
			if (!o_ptr->wpos.wz)
			{
				cave_type **zcave;
				zcave=getcave(&o_ptr->wpos);
				if (zcave[y][x].info & CAVE_ICKY)
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
		if (o_ptr->k_idx)
		{
			if ((!o_ptr->wpos.wz && (!purge || o_ptr->owner)) ||
					getcave(&o_ptr->wpos)) continue;

			/* Delete it first */
			delete_object_idx(i, TRUE);
		}

		/* One less object */
		tmp_max--;
	}

	/*
	 * Now, all objects that wanted removed have been wiped.
	 * Their spaces remain. Fill in the gaps and pull back the list.
	 */

	/* were any removed? */
	if (o_max != tmp_max) {
		int *old_idx;
		int z=1;
		monster_type *m_ptr;
		object_type *o_ptr;

		/* allocate storage for map */
		old_idx = calloc(1, o_max * sizeof(int));

		/* map the list and compress it */
		for (i = 1; i < o_max; i++) {
			if (o_list[i].k_idx) {
				/* Copy structure */
				o_list[z] = o_list[i];

				/* this needs to go through all objects - mikaelh */
				if (z != i) {
					for (Ind = 1; Ind <= NumPlayers; Ind++) {
						if (Players[Ind]->conn == NOT_CONNECTED) continue;
						Players[Ind]->obj_vis[z] = Players[Ind]->obj_vis[i];

						/* clear the old stuff */
						Players[Ind]->obj_vis[i] = FALSE;
					}
				}

				old_idx[i] = z;
				z++;
			}
		}

		/* wipe any cleared spaces */
		for (i = tmp_max; i < o_max; i++) {
			WIPE(&o_list[i], object_type);
		}

		/* now, we can fix o_max */
		o_max = tmp_max;

		/* relink objects correctly */
		for (i = 1; i < o_max; i++) {
			if (o_list[i].next_o_idx) {
				/* get the new mapping */
				o_list[i].next_o_idx = old_idx[o_list[i].next_o_idx];
			}
		}

#ifdef MONSTER_INVENTORY
		/* fix monsters' inventories */
		for (i = 1; i < m_max; i++) {
			m_ptr = &m_list[i];
			if (m_ptr->hold_o_idx) {
				m_ptr->hold_o_idx = old_idx[m_ptr->hold_o_idx];
			}
		}
#endif	/* MONSTER_INVENTORY */

		/* update cave grid object indices to still point to
		   the correct objects in our newly resorted o_list */
		for (i = 1; i < o_max; i++) {
			o_ptr = &o_list[i];
			wpos = &o_ptr->wpos;
			x = o_ptr->ix;
			y = o_ptr->iy;

			if ((zcave=getcave(wpos))) {
				c_ptr = &zcave[y][x];
				if (in_bounds2(wpos, y, x)) {
					if (old_idx[c_ptr->o_idx] == i) {
						c_ptr->o_idx = i;
					}
				}
				else{
					y = 255 - y;
					if (in_bounds2(wpos, y, x)) {
						c_ptr = &zcave[y][x];
						if (c_ptr->feat == FEAT_MON_TRAP) {
							struct c_special *cs_ptr;
							if ((cs_ptr = GetCS(c_ptr, CS_MON_TRAP))) {
								if (old_idx[cs_ptr->sc.montrap.trap_kit] == i) {
									cs_ptr->sc.montrap.trap_kit = i;
								}
							}
						}
					}
				}
			}
		}

		/* free the allocated memory!! - mikaelh */
		free(old_idx);
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
			handle_art_d(o_ptr->name1);
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

/* DEBUG -after getting weird crashes today 2007-12-21 in bree from /clv, and multiplying townies, I added this inbound check- C. Blue */
//		if (in_bounds_array(o_ptr->iy, o_ptr->ix)) {
			/* Skip objects inside a house but not in a vault in dungeon/tower */
			if (!wpos->wz && zcave[o_ptr->iy][o_ptr->ix].info & CAVE_ICKY)
				continue;
//		}

		/* Mega-Hack -- preserve artifacts */
		/* Hack -- Preserve unknown artifacts */
		/* We now preserve ALL artifacts, known or not */
		if (artifact_p(o_ptr)/* && !object_known_p(o_ptr)*/)
		{
			/* Info */
			/* s_printf("Preserving artifact %d.\n", o_ptr->name1); */

			/* Mega-Hack -- Preserve the artifact */
			handle_art_d(o_ptr->name1);
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
	if (server_dungeon) s_printf("Too many objects!\n");

	/* Oops */
	return (0);
}



/*
 * Apply a "object restriction function" to the "object allocation table"
 */
errr get_obj_num_prep(u32b resf)
{
	long	i, n, p, adj;
	long	k_idx;

	/* Get the entry */
	alloc_entry *table = alloc_kind_table;

	/* Copy the hook into a local variable for speed */
	int (*hook)(int k_idx, u32b resf) = get_obj_num_hook;

	if (hook) {
		/* Scan the allocation table */
		for (i = 0, n = alloc_kind_size; i < n; i++)
		{
			/* Get the entry */
			alloc_entry *entry = &table[i];

			/* Obtain the base probability */
			p = entry->prob1;

			/* Default probability for this pass */
			entry->prob2 = 0;

			/* Access the index */
			k_idx = entry->index;

			/* Call the hook and adjust the probability */
			adj = (*hook)(k_idx, resf);
			p = adj * p / 100;

			/* Save the probability */
			entry->prob2 = p;
		}
	} else {
		/* Scan the allocation table */
		for (i = 0, n = alloc_kind_size; i < n; i++)
		{
			/* Get the entry */
			alloc_entry *entry = &table[i];

			/* Obtain the base probability */
			p = entry->prob1;

			/* Default probability for this pass */
			entry->prob2 = 0;

			/* Save the probability */
			entry->prob2 = p;
		}
	}

	/* Success */
	return (0);
}



/*
 * Apply a "object restriction function" to the "object allocation table"
 * This function only takes objects of a certain TVAL! - C. Blue
 * (note that kind_is_legal_special and this function are somewhat redundant)
 */
errr get_obj_num_prep_tval(int tval, u32b resf)
{
	long i, n, p, adj;
	long k_idx;

	/* Get the entry */
	alloc_entry *table = alloc_kind_table;

	/* Copy the hook into a local variable for speed */
	int (*hook)(int k_idx, u32b resf) = get_obj_num_hook;

	if (hook) {
		/* Scan the allocation table */
		for (i = 0, n = alloc_kind_size; i < n; i++)
		{
			/* Get the entry */
			alloc_entry *entry = &table[i];

			/* Obtain the base probability */
			p = entry->prob1;

			/* Default probability for this pass */
			entry->prob2 = 0;

			/* Access the index */
			k_idx = entry->index;

			/* Call the hook and adjust the probability */
			adj = (*hook)(k_idx, resf);
			p = adj * p / 100;

			/* Only accept a specific tval */
			if (k_info[table[i].index].tval != tval)
			{
				continue;
			}

			/* Save the probability */
			entry->prob2 = p;
		}
	} else {
		/* Scan the allocation table */
		for (i = 0, n = alloc_kind_size; i < n; i++)
		{
			/* Get the entry */
			alloc_entry *entry = &table[i];

			/* Obtain the base probability */
			p = entry->prob1;

			/* Default probability for this pass */
			entry->prob2 = 0;

			/* Access the index */
			k_idx = entry->index;

			/* Only accept a specific tval */
			if (k_info[table[i].index].tval != tval)
			{
				continue;
			}

			/* Save the probability */
			entry->prob2 = p;
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
s16b get_obj_num(int max_level, u32b resf)
{
	long		i, j, n, p;

	long		value, total;

	long		k_idx;

	object_kind	*k_ptr;

	alloc_entry	*table = alloc_kind_table;


	/* Boost level */
	if (max_level > 0)
	{
		/* Occasional "boost" */
		if (rand_int(GREAT_OBJ) == 0)
		{
			/* What a bizarre calculation */
                        max_level = 1 + ((max_level * MAX_DEPTH_OBJ) / randint(MAX_DEPTH_OBJ));
		}
	}


	/* Reset total */
	total = 0L;

	/* Cap maximum level */
	if (max_level > 254)
	{
		max_level = 254;
	}

	/* Calculate loop bounds */
	n = alloc_kind_index_level[max_level + 1];

	if (opening_chest) {		
		/* Process probabilities */
		for (i = 0; i < n; i++)
		{
			/* Default */
			table[i].prob3 = 0;

			/* Access the index */
			k_idx = table[i].index;

			/* Access the actual kind */
			k_ptr = &k_info[k_idx];

			/* Hack -- prevent embedded chests */
			if (k_ptr->tval == TV_CHEST) continue;

			/* Accept */
			table[i].prob3 = table[i].prob2;

			/* Total */
			total += table[i].prob3;
		}
	} else {
		/* Process probabilities */
		for (i = 0; i < n; i++)
		{
			/* Default */
			table[i].prob3 = 0;

			/* Accept */
			table[i].prob3 = table[i].prob2;

			/* Total */
			total += table[i].prob3;
		}
	}

	/* No legal objects */
	if (total <= 0) return (0);


	/* Pick an object */
	value = rand_int(total);

	/* Find the object */
	for (i = 0; i < n; i++)
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
		for (i = 0; i < n; i++)
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
		for (i = 0; i < n; i++)
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
	if (o_ptr->note && (o_ptr->ident & ID_SENSE)) {
		/* Access the inscription */
		cptr q = quark_str(o_ptr->note);

		/* Hack -- Remove auto-inscriptions */
		if ((streq(q, "bad")) ||
		    (streq(q, "cursed")) ||
		    (streq(q, "broken")) ||
		    (streq(q, "good")) ||
		    (streq(q, "average")) ||
		    (streq(q, "excellent")) ||
		    (streq(q, "worthless")) ||
		    (streq(q, "special")) ||
		    (streq(q, "terrible"))) {
			/* Forget the inscription */
			o_ptr->note = 0;
		}
	}

	/* Clear the "Felt" info */
	o_ptr->ident &= ~ID_SENSE;

	/* Clear the "Empty" info */
	o_ptr->ident &= ~ID_EMPTY;

	/* Now we know about the item */
	o_ptr->ident |= (ID_KNOWN | ID_SENSED_ONCE);

	/* Artifact becomes 'found' status - omg it must already become
	'found' if a player picks it up! That gave headaches! */
	if (true_artifact_p(o_ptr)) handle_art_ipara(o_ptr->name1);

}




/*
 * The player is now aware of the effects of the given object.
 */
void object_aware(int Ind, object_type *o_ptr) {
	int i;

	if (object_aware_p(Ind, o_ptr)) return;

	/* Fully aware of the effects */
	Players[Ind]->obj_aware[o_ptr->k_idx] = TRUE;

	/* Make it refresh, although the object mem structure didn't change */
	for (i = 0; i < INVEN_TOTAL; i++)
		if (Players[Ind]->inventory[i].k_idx == o_ptr->k_idx)
			Players[Ind]->inventory[i].changed = !Players[Ind]->inventory[i].changed;
}



/*
 * Something has been "sampled"
 */
void object_tried(int Ind, object_type *o_ptr) {
	int i;

	if (object_tried_p(Ind, o_ptr)) return;

	/* Mark it as tried (even if "aware") */
	Players[Ind]->obj_tried[o_ptr->k_idx] = TRUE;

	/* Make it refresh, although the object mem structure didn't change */
	for (i = 0; i < INVEN_TOTAL; i++)
		if (Players[Ind]->inventory[i].k_idx == o_ptr->k_idx)
			Players[Ind]->inventory[i].changed = !Players[Ind]->inventory[i].changed;
}



/*
 * Return the "value" of an "unknown" item
 * Make a guess at the value of non-aware items
 */
static s64b object_value_base(int Ind, object_type *o_ptr)
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

void eliminate_common_ego_flags(object_type *o_ptr, u32b *f1, u32b *f2, u32b *f3, u32b *f4, u32b *f5, u32b *esp) {
	s16b j;
	ego_item_type *e_ptr;
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Hack -- eliminate Base object powers */
	(*f1) &= ~k_ptr->flags1;
	(*f2) &= ~k_ptr->flags2;
	(*f3) &= ~k_ptr->flags3;
	(*f4) &= ~k_ptr->flags4;
	(*f5) &= ~k_ptr->flags5;
	(*esp) &= ~k_ptr->esp;

	if (o_ptr->name1) return; /* if used on artifacts, we're done here */

	/* Hack -- eliminate 'promised' (ie 100% occurring) ego powers */
	if (!o_ptr->name2) return;
	e_ptr = &e_info[o_ptr->name2];
	for (j = 0; j < 5; j++) {
		/* Rarity check */
		if (e_ptr->rar[j] > 99) {
			*(f1) &= ~e_ptr->flags1[j];
			*(f2) &= ~e_ptr->flags2[j];
			*(f3) &= ~e_ptr->flags3[j];
			*(f4) &= ~e_ptr->flags4[j];
			*(f5) &= ~e_ptr->flags5[j];
			*(esp) &= ~e_ptr->esp[j];
		}
	}

	/* Hack -- eliminate 'promised' (ie 100% occurring) ego powers */
	if (!o_ptr->name2b) return;
	e_ptr = &e_info[o_ptr->name2b];
	for (j = 0; j < 5; j++) {
		/* Rarity check */
		if (e_ptr->rar[j] > 99) {
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
s32b flag_cost(object_type * o_ptr, int plusses)
{
	s32b total = 0; //, am;
	u32b f1, f2, f3, f4, f5, esp;

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


	if (f3 & TR3_WRAITH) total += 250000;
	if (f5 & TR5_INVIS) total += 30000;
	if (!(f4 & TR4_FUEL_LITE))
	{
	        if (f3 & TR3_LITE1) total += 750;
	        if (f4 & TR4_LITE2) total += 1250;
	        if (f4 & TR4_LITE3) total += 2750;
	}

	if ((!(f4 & TR4_FUEL_LITE)) && (f3 & TR3_IGNORE_FIRE)) total += 100;

	if (f5 & TR5_CHAOTIC) total += 10000;
	if (f1 & TR1_VAMPIRIC) total += 13000;
	if (f1 & TR1_SLAY_ANIMAL) total += 3500;
	if (f1 & TR1_SLAY_EVIL) total += 4500;
	if (f1 & TR1_SLAY_UNDEAD) total += 3500;
	if (f1 & TR1_SLAY_DEMON) total += 3500;
	if (f1 & TR1_SLAY_ORC) total += 3000;
	if (f1 & TR1_SLAY_TROLL) total += 3500;
	if (f1 & TR1_SLAY_GIANT) total += 3500;
	if (f1 & TR1_SLAY_DRAGON) total += 3500;
        if (f1 & TR1_KILL_DEMON) total += 5500;
        if (f1 & TR1_KILL_UNDEAD) total += 5500;
	if (f1 & TR1_KILL_DRAGON) total += 5500;
	if (f5 & TR5_VORPAL) total += 5000;
	if (f5 & TR5_IMPACT) total += 5000;
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
/* f6 Not yet implemented in object_flags/eliminate_common_ego_flags etc. Really needed??
//        if (f5 & TR5_SENS_FIRE) total -= 100;
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
	if (f2 & TR2_RES_POIS) total += 5000;
	if (f2 & TR2_RES_FEAR) total += 2000;
	if (f2 & TR2_RES_LITE) total += 2750;
	if (f2 & TR2_RES_DARK) total += 2750;
	if (f2 & TR2_RES_BLIND) total += 8000;
	if (f2 & TR2_RES_CONF) total += 3500;
	if (f2 & TR2_RES_SOUND) total += 10000;
	if (f2 & TR2_RES_SHARDS) total += 4000;
	if (f2 & TR2_RES_NETHER) total += 15000;
	if (f2 & TR2_RES_NEXUS) total += 7000;
	if (f2 & TR2_RES_CHAOS) total += 15000;
	if (f2 & TR2_RES_DISEN) total += 20000;
	if (f3 & TR3_SH_FIRE) total += 3000;
	if (f5 & TR5_SH_COLD) total += 3000;
	if (f3 & TR3_SH_ELEC) total += 3000;
        if (f3 & TR3_DECAY) total += 0;
	if (f3 & TR3_NO_TELE) total += 2500;
	if (f3 & TR3_NO_MAGIC) total += 2500;
	if (f3 & TR3_TY_CURSE) total -= 15000;
	if (f3 & TR3_EASY_KNOW) total += 0;
	if (f3 & TR3_HIDE_TYPE) total += 0;
	if (f3 & TR3_SHOW_MODS) total += 0;
	if (f3 & TR3_INSTA_ART) total += 0;
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
        if (esp & ESP_ALL) total += 150000;/* was 125k, but ESP crowns cost 150k */
	if (f3 & TR3_SLOW_DIGEST) total += 750;
	if (f3 & TR3_REGEN) total += 2500;
	if (f5 & TR5_REGEN_MANA) total += 2500;
	if (f3 & TR3_XTRA_MIGHT) total += 2250;
	if (f3 & TR3_XTRA_SHOTS) total += 10000;
	if (f5 & TR5_IGNORE_WATER) total += 0;
	if (f5 & TR5_IGNORE_MANA) total += 0;
	if (f5 & TR5_IGNORE_DISEN) total += 0;
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
			total += 500;
	}
//	if (f3 & TR3_AGGRAVATE) total -= 10000; /* penalty 1 of 2 */
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
	//        if (f5 & TR5_LEVELS) total += o_ptr->elevel * 2000;

#if 1 /* experimentally like this (see above for original position/code) */
 #if 1
	if (f2 & TR2_IM_ACID) total += ((total + 10000) * 3) / 2;
	if (f2 & TR2_IM_ELEC) total += ((total + 10000) * 3) / 2;
	if (f2 & TR2_IM_FIRE) total += ((total + 10000) * 3) / 2;
	if (f2 & TR2_IM_COLD) total += ((total + 10000) * 3) / 2;
 #else
	if (f2 & TR2_IM_ACID) total += 10000;
	if (f2 & TR2_IM_ELEC) total += 10000;
	if (f2 & TR2_IM_FIRE) total += 10000;
	if (f2 & TR2_IM_COLD) total += 10000;
	if (f2 & (TR2_IM_ACID | TR2_IM_ELEC | TR2_IM_FIRE | TR2_IM_COLD))
		total = (total * 3) / 2;
 #endif
#endif

	/* Hack -- ammos shouldn't be that expensive */
	if (is_ammo(o_ptr->tval))
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
s64b object_value_real(int Ind, object_type *o_ptr)
{
	u32b f1, f2, f3, f4, f5, esp;
	object_kind *k_ptr = &k_info[o_ptr->k_idx];
	bool star = (Ind == 0 || object_fully_known_p(Ind, o_ptr));

	/* Base cost */
	s64b value = k_ptr->cost, i;

	/* Hack -- "worthless" items */
	if (!value) return (0L);


	/* Extract some flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);


	/* Artifact */
	if (o_ptr->name1) {
		artifact_type *a_ptr;

		/* Randarts */
		if (o_ptr->name1 == ART_RANDART) {
			/* use dedicated artifact pricing function - C. Blue */
			return(artifact_value_real(Ind, o_ptr));
		} else {
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

	else {
		/* Ego-Item */
		if (o_ptr->name2) {
			ego_item_type *e_ptr = &e_info[o_ptr->name2];

			/* Hack -- "worthless" ego-items */
			if (!e_ptr->cost) return (0L);

			/* Hack -- Reward the ego-item with a bonus */
			value += e_ptr->cost;

			/* Hope this won't cause inflation.. */
			if (star) value += flag_cost(o_ptr, o_ptr->pval);
			/* Note: flag_cost contains all the resistances and other flags,
			   while down here in object_value_real only the 'visible' boni
			   are checked (pval, bpval, tohit, todam, toac), all those stats
			   whose changes are observable without *ID* - C. Blue */

			if (o_ptr->name2b) {
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
		return 0;

	/* Bad items don't sell. Good items with some bad modifiers DO sell ((*defenders*)). -C. Blue */
	switch (o_ptr->tval) {
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
	case TV_CROWN:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_DRAG_ARMOR:
		if ((((o_ptr->to_h) < 0 && ((o_ptr->to_h - k_ptr->to_h) < 0)) ||
		    ((o_ptr->to_d) < 0 && ((o_ptr->to_d - k_ptr->to_d) < 0 || k_ptr->to_d < 0)) ||
		    ((o_ptr->to_a) < 0 && ((o_ptr->to_a - k_ptr->to_a) < 0 || k_ptr->to_a < 0)) ||
		    (o_ptr->pval < 0) || (o_ptr->bpval < 0)) &&
		    !(((o_ptr->to_h) > 0) ||
		    ((o_ptr->to_d) > 0) ||
		    ((o_ptr->to_a) > 0) ||
		    (o_ptr->pval > 0) || (o_ptr->bpval > 0))) return (0L);
	        break;

	case TV_DIGGING:
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_AXE:
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
	case TV_BOW:
	case TV_BOOMERANG:

	case TV_MSTAFF:
	case TV_LITE:
	case TV_AMULET:
	case TV_RING:
	case TV_TRAPKIT:

	default:
		if ((((o_ptr->to_h) < 0 && ((o_ptr->to_h - k_ptr->to_h) < 0 || k_ptr->to_h < 0)) ||
		    ((o_ptr->to_d) < 0 && ((o_ptr->to_d - k_ptr->to_d) < 0 || k_ptr->to_d < 0)) ||
		    ((o_ptr->to_a) < 0 && ((o_ptr->to_a - k_ptr->to_a) < 0 || k_ptr->to_a < 0)) ||
/* to allow Mummy Wrappings in bm! - C. Blue */
//		    (o_ptr->pval < 0) || (o_ptr->bpval < 0)) &&
//		    (o_ptr->pval < 0) || (o_ptr->bpval < k_ptr->pval)) &&
//		    (o_ptr->pval < 0) || (o_ptr->tval != TV_ROD && o_ptr->bpval < k_ptr->pval)) &&
		    (o_ptr->pval < 0) || (o_ptr->bpval < 0 && o_ptr->bpval < k_ptr->pval)) &&
		    !(((o_ptr->to_h) > 0) ||
		    ((o_ptr->to_d) > 0) ||
		    ((o_ptr->to_a) > 0) ||
		    (o_ptr->pval > 0) || (o_ptr->bpval > 0))) return (0L);
	        break;
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
		case TV_BLUNT:
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
			int pval = o_ptr->bpval, kpval = k_ptr->pval;
			/* If the bpval has been set to the k_info pval,
			   don't increase the item's value for this
			   granted pval, since it's already included in
			   the k_info price! */
			if (pval >= kpval) {
				pval -= kpval;
				kpval = 0;
			}

			/* Don't use bpval of costumes - mikaelh */
			if ((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_COSTUME)) {
				pval = 0;
			}

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
				/* If the k_info pval became the object's
				   pval instead of bpval (shouldn't happen)
				   then take care of it and again don't
				   increase the value for this granted pval: */
				else if (pval >= kpval) {
					pval -= kpval;
					kpval = 0;
				}

				/* Give credit for stat bonuses */
				//			if (f1 & TR1_STR) value += (pval * 200L);
				//			if (f1 & TR1_STR) value += (boost * 200L);
				if (f1 & TR1_STR) count++;
				if (f1 & TR1_INT) count++;
				if ((f1 & TR1_WIS) && !(f1 & TR1_INT)) count++; /* slightly useless combination */
				if (f1 & TR1_DEX) count++;
				if (f1 & TR1_CON) count++;
#if 1 /* make CHR cheaper? */
				if (f1 & TR1_CHR) {
					if (count <= 1) count++;
					else value += pval * 1000;
				}
#else
				if (f1 & TR1_CHR) value += pval * 1000;
#endif
				/* hack for double-stat rings - C. Blue */
				if ((o_ptr->tval == TV_RING) && (
				    (o_ptr->sval == SV_RING_MIGHT) ||
				    (o_ptr->sval == SV_RING_READYWIT) ||
				    (o_ptr->sval == SV_RING_TOUGHNESS) ||
				    (o_ptr->sval == SV_RING_CUNNINGNESS))
				    )	{
					count /= 2;
					if (count) value += count * PRICE_BOOST((count + pval), 2, 1)* 300L;
				} else {
					if (count) value += count * PRICE_BOOST((count + pval), 2, 1)* 200L;
				}

//				if (f5 & (TR5_CRIT)) value += (PRICE_BOOST(pval, 0, 1)* 300L);//was 500, then 400
				if (f5 & (TR5_CRIT)) value += pval * pval * 5000L;/* was 20k, but speed is only 10k */
				if (f5 & (TR5_LUCK)) value += (PRICE_BOOST(pval, 0, 1)* 10L);

				/* Give credit for stealth and searching */
//				if (f1 & TR1_STEALTH) value += (PRICE_BOOST(pval, 3, 1) * 100L);
				if (f1 & TR1_STEALTH) value += pval * pval * 250L;//100
				if (f1 & TR1_SEARCH) value += pval * pval * 200L;//200
				if (f5 & TR5_DISARM) value += pval * pval * 100L;

				/* Give credit for infra-vision and tunneling */
				if (f1 & TR1_INFRA) value += pval * pval * 150L;//100
				if (f1 & TR1_TUNNEL) value += pval * pval * 175L;//50

				/* Give credit for extra attacks */
				if (o_ptr->tval == TV_RING) {
					if (f1 & TR1_BLOWS) value += (PRICE_BOOST(pval, 0, 1) * 2000L);//1500
				} else {
//					if (f1 & TR1_BLOWS) value += (PRICE_BOOST(pval, 0, 1) * 3000L);
					if (f1 & TR1_BLOWS) value += 10000 + pval * 20000L;
				}

				/* Give credit for extra casting */
				if (f1 & TR1_SPELL) value += (PRICE_BOOST(pval, 0, 1) * 4000L);

				/* Give credit for extra HP bonus */
				if (f1 & TR1_LIFE) value += (PRICE_BOOST(pval, 0, 1) * 3000L);


				/* Flags moved here exclusively from flag_cost */
			        if (f1 & TR1_MANA) value += (1000 * pval);

				/* End of flags, moved here from flag_cost */


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
//					if (f1 & TR1_SPEED) value += pval * pval * 7000L;
				}
//				else if (f1 & TR1_SPEED) value += (PRICE_BOOST(pval, 0, 4) * 100000L);
//				else if (f1 & TR1_SPEED) value += pval * pval * 10000L;
				else if (f1 & TR1_SPEED) value += (pval + 1) * (pval + 1) * 7000L;

				pval = o_ptr->pval;

				if (o_ptr->name2) {
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
	switch (o_ptr->tval) {
		case TV_BOOK:
			if (o_ptr->sval == SV_SPELLBOOK) {
				/* 1: 145, 2: 240, 3: 375, 4: 540, 5: 735 */
				/*  */
				int sl = school_spells[o_ptr->pval].skill_level + 5,
				    ego_value = value - k_ptr->cost,
				    ev = ego_value > 700 ? 700 : ego_value;
				/* override k_info.txt to have easier handling of possible changes here */
				value = 4;
				/* Pay extra for the spell */
				value = value * (sl * sl);
				/* Add up 'fireproof' etc cost, but related it to the actual scroll cost. */
				value += value < ego_value ? (value < ev ? ev : value) : ego_value;
			}
			/* Done */
			break;

		/* Wands/Staffs */
		case TV_WAND:
			/* Pay extra for charges */
			value += ((value / 20) * o_ptr->pval) / o_ptr->number;

			/* Done */
			break;

		case TV_STAFF:
			/* Pay extra for charges */
			value += ((value / 20) * o_ptr->pval);

			/* Done */
			break;

		/* Rings/Amulets */
		case TV_RING:
		case TV_AMULET:
#if 0
			/* Hack -- negative bonuses are bad */
			if (o_ptr->to_a < 0) return (0L);
			if (o_ptr->to_h < 0) return (0L);
			if (o_ptr->to_d < 0) return (0L);
#endif

			/* keep consistent with store.c: price_item() */
			if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH)) {
				if (o_ptr->pval != 0)
					value += (r_info[o_ptr->pval].level * r_info[o_ptr->pval].mexp >= r_info[o_ptr->pval].level * 100) ?
					    r_info[o_ptr->pval].level * r_info[o_ptr->pval].mexp :
					    r_info[o_ptr->pval].level * 100;
			}

			/* Give credit for bonuses */
//			value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);
			/* Ignore base boni that come from k_info.txt (eg quarterstaff +10 AC) */
			value += ((PRICE_BOOST(o_ptr->to_h, 12, 4) + 
				PRICE_BOOST(o_ptr->to_d, 7, 3) + 
				PRICE_BOOST(o_ptr->to_a, 11, 4)) * 100L);

			/* Done */
			break;

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
#if 0
			/* Hack -- negative armor bonus */
			if (o_ptr->to_a < 0) return (0L);
#endif
			/* Give credit for bonuses */
//			value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);
			/* Ignore base boni that come from k_info.txt (eg quarterstaff +10 AC) */
			value += (  ((o_ptr->to_h <= 0 || o_ptr->to_h <= k_ptr->to_h)? 0 : 
				    ((k_ptr->to_h < 0)? PRICE_BOOST(o_ptr->to_h, 9, 5): 
				    PRICE_BOOST((o_ptr->to_h - k_ptr->to_h), 9, 5))) + 
				    ((o_ptr->to_d <= 0 || o_ptr->to_d <= k_ptr->to_d)? 0 :
				    ((k_ptr->to_d < 0)? PRICE_BOOST(o_ptr->to_d, 9, 5):
				    PRICE_BOOST((o_ptr->to_d - k_ptr->to_d), 9, 5))) + 
				    ((o_ptr->to_a <= 0 || o_ptr->to_a <= k_ptr->to_a)? 0 :
				    ((k_ptr->to_a < 0)? PRICE_BOOST(o_ptr->to_a, 9, 5):
				    PRICE_BOOST((o_ptr->to_a - k_ptr->to_a), 9, 5))) ) * 100L;
	    
			/* Costumes */
			if ((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_COSTUME)) {
				value += r_info[o_ptr->bpval].mexp / 10;
			}

			/* Done */
			break;

		/* Bows/Weapons */
		case TV_BOW:
		case TV_BOOMERANG:
		case TV_AXE:
		case TV_DIGGING:
		case TV_BLUNT:
		case TV_SWORD:
		case TV_POLEARM:
		case TV_MSTAFF:
		case TV_TRAPKIT:
#if 0
			/* Hack -- negative hit/damage bonuses */
			if (o_ptr->to_h + o_ptr->to_d < 0)
			{
				/* Hack -- negative hit/damage are of no importance */
				if (o_ptr->tval == TV_MSTAFF) break;
				if (o_ptr->name2 == EGO_STAR_DF) break;
				else return (0L);
			}
#endif
			/* Factor in the bonuses */
//			value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);
			/* Ignore base boni that come from k_info.txt (eg quarterstaff +10 AC) */
			value += (  ((o_ptr->to_h <= 0 || o_ptr->to_h <= k_ptr->to_h)? 0 :
				    ((k_ptr->to_h < 0)? PRICE_BOOST(o_ptr->to_h, 9, 5):
				    PRICE_BOOST((o_ptr->to_h - k_ptr->to_h), 9, 5))) + 
				    ((o_ptr->to_d <= 0 || o_ptr->to_d <= k_ptr->to_d)? 0 :
				    ((k_ptr->to_d < 0)? PRICE_BOOST(o_ptr->to_d, 9, 5):
				    PRICE_BOOST((o_ptr->to_d - k_ptr->to_d), 9, 5))) + 
				    ((o_ptr->to_a <= 0 || o_ptr->to_a <= k_ptr->to_a)? 0 :
				    ((k_ptr->to_a < 0)? PRICE_BOOST(o_ptr->to_a, 9, 5):
				    PRICE_BOOST((o_ptr->to_a - k_ptr->to_a), 9, 5))) ) * 100L;

			/* Hack -- Factor in extra damage dice */
			if (((o_ptr->dd > k_ptr->dd) && (o_ptr->ds == k_ptr->ds)) &&
			    (o_ptr->dd > 0 && o_ptr->ds > 0))
			{
				value += PRICE_BOOST((o_ptr->dd - k_ptr->dd), 1, 4) * o_ptr->ds * 100L;
			}

			/* Done */
			break;

		/* Ammo */
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
			/* Hack -- negative hit/damage bonuses */
//			if (o_ptr->to_h + o_ptr->to_d < 0) return (0L);

			/* Factor in the bonuses */
//			value += ((o_ptr->to_h + o_ptr->to_d) * 5L);
			/* Ignore base boni that come from k_info.txt (eg quarterstaff +10 AC) */
			value += (  ((o_ptr->to_h <= 0 || o_ptr->to_h <= k_ptr->to_h)? 0 :
				    ((k_ptr->to_h < 0)? PRICE_BOOST(o_ptr->to_h, 9, 5):
				    PRICE_BOOST((o_ptr->to_h - k_ptr->to_h), 9, 5))) + 
				    ((o_ptr->to_d <= 0 || o_ptr->to_d <= k_ptr->to_d)? 0 :
				    ((k_ptr->to_d < 0)? PRICE_BOOST(o_ptr->to_d, 9, 5):
				    PRICE_BOOST((o_ptr->to_d - k_ptr->to_d), 9, 5)))  ) * 5L;

			/* Hack -- Factor in extra damage dice */
			if (((o_ptr->dd > k_ptr->dd) && (o_ptr->ds == k_ptr->ds)) &&
			    (o_ptr->dd > 0 && o_ptr->ds > 0))
			{
				value += (o_ptr->dd - k_ptr->dd) * (o_ptr->ds - k_ptr->ds) * 5L;
			}

			/* Special attack (exploding arrow) */
			if (o_ptr->pval != 0) {
				if (o_ptr->name1 != ART_RANDART) value *= 8;
				else value *= 2;
			}

			/* Done */
			break;
	}

	/* hack against those 500k randarts */
	if (o_ptr->name1 == ART_RANDART) {
		value >>= 1; /* general randart value nerf */
//		if (f3 & TR3_AGGRAVATE) value >>= 1; /* aggravate penalty 2 of 2 */
	}

	if (f3 & TR3_AGGRAVATE) value >>= 1; /* one generic aggravate penalty fits it all */

	/* Return the value */
	return (value);
}


/* Return a sensible pricing for randarts, which
   gets added to k_info base item price - C. Blue
   Note: Some pretty unimportant flags are missing. */
s32b artifact_flag_cost(object_type *o_ptr, int plusses) {
	artifact_type *a_ptr;
	s32b total = 0, am, minus, slay = 0;
	u32b f1, f2, f3, f4, f5, esp;
	int res_amass = 0;

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	if (f5 & TR5_TEMPORARY) return 0;

	/* Hack - This shouldn't be here, still.. */
	eliminate_common_ego_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	/* hack: Artifact ammunition actually uses a_ptr->cost.. somewhat inconsistent, sorry */
	if ((o_ptr->name1 == ART_RANDART) && is_ammo(o_ptr->tval)) {
		a_ptr = randart_make(o_ptr);
		return(a_ptr->cost);
	}

	if (f4 & TR4_AUTO_ID) {
		if (o_ptr->tval == TV_GLOVES)
			total += 100000;
		else
			total += 50000;
	}
	if (f3 & TR3_WRAITH) total += 100000;
	if (f5 & TR5_INVIS) total += 10000;
	if (!(f4 & TR4_FUEL_LITE)) {
		if (f3 & TR3_LITE1) total += 750;
		if (f4 & TR4_LITE2) total += 1250;
		if (f4 & TR4_LITE3) total += 2750;
	}

	if ((!(f4 & TR4_FUEL_LITE)) && (f3 & TR3_IGNORE_FIRE)) total += 100;
	if (f5 & TR5_CHAOTIC) total += 0;
	if (f1 & TR1_VAMPIRIC) total += 30000;

	if (f1 & TR1_SLAY_ANIMAL) slay += 1500;
	if (f1 & TR1_SLAY_EVIL) slay += 5500;
	if (f1 & TR1_SLAY_UNDEAD) slay += 3000;
	if (f1 & TR1_SLAY_DEMON) slay += 3500;
	if (f1 & TR1_SLAY_ORC) slay += 1000;
	if (f1 & TR1_SLAY_TROLL) slay += 1500;
	if (f1 & TR1_SLAY_GIANT) slay += 2000;
	if (f1 & TR1_SLAY_DRAGON) slay += 3000;
	if (f1 & TR1_KILL_DEMON) slay += 7500;
	if (f1 & TR1_KILL_UNDEAD) slay += 7500;
	if (f1 & TR1_KILL_DRAGON) slay += 7500;
	if (f1 & TR1_BRAND_POIS) slay += 2500;
	if (f1 & TR1_BRAND_ACID) slay += 4500;
	if (f1 & TR1_BRAND_ELEC) slay += 4500;
	if (f1 & TR1_BRAND_FIRE) slay += 3000;
	if (f1 & TR1_BRAND_COLD) slay += 2500;

	/* slay value depends on weapon dice :-o */
	if (f4 & (TR4_MUST2H | TR4_SHOULD2H)) {
		total += (slay * ((o_ptr->dd * (o_ptr->ds + 1)) + 80)) / 100;
	} else {
		total += (slay * ((5 * o_ptr->dd * (o_ptr->ds + 1)) + 50)) / 100;
	}

	if (f5 & TR5_VORPAL) total += 20000;
	if (f5 & TR5_IMPACT) total += 5000;
	if (f2 & TR2_SUST_STR) total += 2850;
	if (f2 & TR2_SUST_INT) total += 2850;
	if (f2 & TR2_SUST_WIS) total += 2850;
	if (f2 & TR2_SUST_DEX) total += 2850;
	if (f2 & TR2_SUST_CON) total += 2850;
	if (f2 & TR2_SUST_CHR) total += 1250;
	if (f5 & TR5_REFLECT) total += 10000;
	if (f2 & TR2_FREE_ACT) {
		total += 4500;
		res_amass++;
	}
	if (f2 & TR2_HOLD_LIFE) {
		total += 8500;
		res_amass++;
	}
/* f6 Not yet implemented in object_flags/eliminate_common_ego_flags etc. Really needed??
//	if (f5 & TR5_SENS_FIRE) total -= 100;
	if (f6 & TR6_SENS_COLD) total -= 100;
	if (f6 & TR6_SENS_ACID) total -= 100;
	if (f6 & TR6_SENS_ELEC) total -= 100;
	if (f6 & TR6_SENS_POIS) total -= 100; */
	if (f2 & TR2_IM_ACID) {
		total += 30000;
		res_amass += 2;
		f2 |= TR2_RES_ACID;
	} else if (f2 & TR2_RES_ACID) total += 1250;
	if (f2 & TR2_IM_ELEC) {
		total += 20000;
		res_amass += 2;
		f2 |= TR2_RES_ELEC;
	} else if (f2 & TR2_RES_ELEC) total += 1250;
	if (f2 & TR2_IM_FIRE) {
		total += 30000;
		res_amass += 2;
		f2 |= TR2_RES_FIRE;
	} else if (f2 & TR2_RES_FIRE) total += 1250;
	if (f2 & TR2_IM_COLD) {
		total += 20000;
		res_amass += 2;
		f2 |= TR2_RES_COLD;
	} else if (f2 & TR2_RES_COLD) total += 1250;
	/* count complete base res as 1up too */
	if ((f2 & (TR2_RES_ACID | TR2_RES_ELEC | TR2_RES_FIRE | TR2_RES_COLD))
	    == (TR2_RES_ACID | TR2_RES_ELEC | TR2_RES_FIRE | TR2_RES_COLD))
		res_amass++;
	if (f2 & TR2_RES_POIS) {
		total += 5000;
		res_amass++;
	}
	if (f2 & TR2_RES_FEAR) total += 2000;
	if (f2 & TR2_RES_LITE) total += 2750;
	if (f2 & TR2_RES_DARK) {
		total += 2750;
		res_amass++;
	}
	if (f2 & TR2_RES_BLIND) total += 6000;
	if (f2 & TR2_RES_CONF) total += 1500;
	if (f2 & TR2_RES_SOUND) {
		total += 7000;
		res_amass += 2;
	}
	if (f2 & TR2_RES_SHARDS) {
		total += 4000;
		res_amass++;
	}
	if (f2 & TR2_RES_NETHER) {
		total += 10000;
		res_amass += 2;
	}
	if (f2 & TR2_RES_NEXUS) {
		total += 7000;
		res_amass += 2;
	}
	if (f2 & TR2_RES_CHAOS) {
		total += 10000;
		res_amass += 2;
	}
	if (f2 & TR2_RES_DISEN) {
		total += 10000;
		res_amass += 2;
	}
	if (f5 & TR5_RES_MANA) {
		total += 15000;
		res_amass += 2;
	}
	if (f3 & TR3_SH_FIRE) total += 2000;
	if (f5 & TR5_SH_COLD) total += 2000;
	if (f3 & TR3_SH_ELEC) total += 2000;
        if (f3 & TR3_DECAY) total += 0;
//done elsewhere	if (f3 & TR3_NO_TELE) total -= 50000;
	if (f3 & TR3_NO_MAGIC) total += 0;
	if (f3 & TR3_TY_CURSE) total -= 15000;
	if (f3 & TR3_EASY_KNOW) total += 0;
	if (f3 & TR3_HIDE_TYPE) total += 0;
	if (f3 & TR3_SHOW_MODS) total += 0;
	if (f3 & TR3_INSTA_ART) total += 0;
	if (f3 & TR3_SEE_INVIS) total += 2000;
	if (esp & ESP_ORC) total += 1000;
	if (esp & ESP_TROLL) total += 2000;
	if (esp & ESP_DRAGON) total += 5000;
	if (esp & ESP_GIANT) total += 3000;
	if (esp & ESP_DEMON) total += 6000;
	if (esp & ESP_UNDEAD) total += 6000;
	if (esp & ESP_EVIL) total += 20000;
	if (esp & ESP_ANIMAL) total += 6000;
	if (esp & ESP_DRAGONRIDER) total += 3000;
	if (esp & ESP_GOOD) total += 8000;
	if (esp & ESP_NONLIVING) total += 5000;
	if (esp & ESP_UNIQUE) total += 4000;
	if (esp & ESP_SPIDER) total += 3000;
	if (esp & ESP_ALL) total += 150000 + 40000; /* hm, extra bonus */
	if (f3 & TR3_SLOW_DIGEST) total += 750;
	if (f3 & TR3_REGEN) total += 3500;
	if (f5 & TR5_REGEN_MANA) total += 2500;
	if (f3 & TR3_XTRA_MIGHT) total += 10000;
	if (f3 & TR3_XTRA_SHOTS) total += 10000;
	if (f5 & TR5_IGNORE_WATER) total += 0;
	if (f5 & TR5_IGNORE_MANA) total += 0;
	if (f5 & TR5_IGNORE_DISEN) total += 0;
	if (f3 & TR3_IGNORE_ACID) total += 0;
	if (f3 & TR3_IGNORE_ELEC) total += 0;
	if (f3 & TR3_IGNORE_COLD) total += 0;
	if (f3 & TR3_ACTIVATE) total += 100;
	if (f3 & TR3_DRAIN_EXP) total -= 20000;
	if (f3 & TR3_TELEPORT) {
		if (o_ptr->ident & ID_CURSED)
			total -= 7500;
		else
			total += 500;
	}
//	if (f3 & TR3_AGGRAVATE) total -= 10000; /* penalty 1 of 2 */
	if (f3 & TR3_BLESSED) total += 750;
	if (f3 & TR3_CURSED) total -= 5000;
	if (f3 & TR3_HEAVY_CURSE) total -= 12500;
	if (f3 & TR3_PERMA_CURSE) total -= 50000;
	if (f3 & TR3_FEATHER) total += 1700;

	if (f4 & TR4_FLY) total += 10000;
	if (f4 & TR4_NEVER_BLOW) total -= 15000;
	if (f4 & TR4_PRECOGNITION) total += 250000;
	if (f4 & TR4_BLACK_BREATH) total -= 40000;
	if (f4 & TR4_DG_CURSE) total -= 25000;
	if (f4 & TR4_CLONE) total -= 20000;
	//        if (f5 & TR5_LEVELS) total += o_ptr->elevel * 2000;

	am = ((f4 & (TR4_ANTIMAGIC_50)) ? 50 : 0)
		+ ((f4 & (TR4_ANTIMAGIC_30)) ? 30 : 0)
		+ ((f4 & (TR4_ANTIMAGIC_20)) ? 20 : 0)
		+ ((f4 & (TR4_ANTIMAGIC_10)) ? 10 : 0);
	minus = o_ptr->to_h + o_ptr->to_d; // + pval;// + (o_ptr->to_a /
	if (minus < 0) minus = 0;
//		+ ((o_ptr->tval == TV_SWORD && o_ptr->sval == SV_DARK_SWORD) ? -5 : 0);
//	if (am > 0) total += (PRICE_BOOST(am, 1, 1)* 2000L);
	am -= minus;
	if (am > 50) am = 50; /* paranoia, mustn't happen */
	if (am > 37) {
		am -= 37;
		total += (am * am) * 100;
	}

	/* accumulation of many resistances is extra valuable (like PDSM ;)) - C. Blue */
	if (res_amass >= 6) {
		if (res_amass > 10) res_amass = 10; /* limit to 3..5 powerful resses */
		res_amass -= 3;
		total += (res_amass * res_amass) * 2000;
	}

	/* Hack -- ammos shouldn't be that expensive */
	if (is_ammo(o_ptr->tval))
		total >>= 2;

	return total;
}

/* Return rating for especially useful armour, that means
   armour that has top +AC and at the same time useful resistances.
   We don't discriminate between pre/postking though. - C. Blue */
static int artifact_flag_rating_armour(object_type *o_ptr) {
	s32b total = 0, slay = 0;
	u32b f1, f2, f3, f4, f5, esp;

	/* this routine treats armour only */
	if ((o_ptr->name1 != ART_RANDART) || !is_armour(o_ptr->tval)) return 0;

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	if (f5 & TR5_TEMPORARY) return 0;

	/* Hack - This shouldn't be here, still.. */
	eliminate_common_ego_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);


	if (f2 & TR2_IM_ACID) {
		total += 6;
		f2 |= TR2_RES_ACID;
	}
	else if (f2 & TR2_RES_ACID) total++;
	if (f2 & TR2_IM_ELEC) {
		total += 6;
		f2 |= TR2_RES_ELEC;
	}
	else if (f2 & TR2_RES_ELEC) total++;
	if (f2 & TR2_IM_FIRE) {
		total += 6;
		f2 |= TR2_RES_FIRE;
	}
	else if (f2 & TR2_RES_FIRE) total++;
	if (f2 & TR2_IM_COLD) {
		total += 6;
		f2 |= TR2_RES_COLD;
	}
	else if (f2 & TR2_RES_COLD) total++;
	/* count complete base res as 1up too */
	if ((f2 & (TR2_RES_ACID | TR2_RES_ELEC | TR2_RES_FIRE | TR2_RES_COLD))
	    == (TR2_RES_ACID | TR2_RES_ELEC | TR2_RES_FIRE | TR2_RES_COLD))
		total += 4;
	if (f2 & TR2_RES_POIS) total += 4;
	if (f2 & TR2_RES_LITE) total += 2;
	if (f2 & TR2_RES_DARK) total += 2;
	if (f2 & TR2_RES_BLIND) total += 2;
	if (f2 & TR2_RES_SOUND) total += 4;
	if (f2 & TR2_RES_SHARDS) total += 4;
	if (f2 & TR2_RES_NETHER) total += 4;
	if (f2 & TR2_RES_NEXUS) total += 3;
	if (f2 & TR2_RES_CHAOS) total += 4;
	if (f2 & TR2_RES_DISEN) total += 4;
	if (f5 & TR5_RES_MANA) total += 2;
	if (f5 & TR5_REGEN_MANA) total += 2;
	if (f4 & TR4_FLY) total += 2;
	if (f2 & TR2_FREE_ACT) total += 2;
	if (f2 & TR2_HOLD_LIFE) total += 4;
	/* Give credit for extra HP bonus */
	if (f1 & TR1_LIFE) total += o_ptr->pval * 2;
	/* Too good to ignore */
	if (esp & ESP_EVIL) total += 1;
	if (esp & ESP_ALL) total += 3;

	/* Glove mods mostly */
	if (f1 & TR1_VAMPIRIC) total += 10;
	if (f1 & TR1_BLOWS) total += o_ptr->pval * 5;
	if (f5 & TR5_CRIT) total += o_ptr->pval;
	if (f1 & TR1_KILL_DEMON) slay++;
	if (f1 & TR1_KILL_UNDEAD) slay++;
	if (f1 & TR1_KILL_DRAGON) slay++;
	if (f1 & TR1_SLAY_EVIL) {
		if (slay <= 1) slay += 2;
		else if (slay <= 2) slay += 1;
	}
	if (f1 & TR1_BRAND_ACID) {
		if (slay <= 1) slay += 2;
		else if (slay <= 2) slay += 1;
	}
	if (f1 & TR1_BRAND_ELEC) {
		if (slay <= 1) slay += 2;
		else if (slay <= 2) slay += 1;
	}
	total += slay * 4;


	if (f3 & TR3_DRAIN_EXP) total >>= 1;

	return total;
}

/* Return rating for especially useful weapon, that means
   armour that has top +hit and especially +dam, and at the
   same time useful damage mods such as EA/Vamp/Crit. - C. Blue */
static int artifact_flag_rating_weapon(object_type *o_ptr) {
	s32b total = 0;
	u32b f1, f2, f3, f4, f5, esp;
	int slay = 0;

	/* this routine treats armour only */
	if ((o_ptr->name1 != ART_RANDART) || !is_weapon(o_ptr->tval)) return 0;

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	if (f5 & TR5_TEMPORARY) return 0;
	if (f4 & TR4_NEVER_BLOW) return 0;

	/* Hack - This shouldn't be here, still.. */
	eliminate_common_ego_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);


	/* Too useful to ignore, but not really helping damage */
	if (f2 & TR2_IM_ACID) total++;
	if (f2 & TR2_IM_ELEC) total++;
	if (f2 & TR2_IM_FIRE) total++;
	if (f2 & TR2_IM_COLD) total++;
	if (esp & ESP_ALL) total++;

	/* 'The' weapon mods */
	if (f1 & TR1_VAMPIRIC) total += 3;
	if (f1 & TR1_BLOWS) total += o_ptr->pval * 2;
	if (f5 & TR5_CRIT) total += (o_ptr->pval + 4) / 2;
	else if (f5 & TR5_VORPAL) total += 2;

	if (f1 & TR1_KILL_DEMON) slay++;
	if (f1 & TR1_KILL_UNDEAD) slay++;
	if (f1 & TR1_KILL_DRAGON) slay++;
	if (f1 & TR1_SLAY_EVIL) {
		if (slay <= 1) slay += 2;
		else if (slay <= 2) slay += 1;
	}
	if (f1 & TR1_BRAND_ACID) {
		if (slay <= 1) slay += 2;
		else if (slay <= 2) slay += 1;
	}
	if (f1 & TR1_BRAND_ELEC) {
		if (slay <= 1) slay += 2;
		else if (slay <= 2) slay += 1;
	}
	/* slay value depends on weapon dice :-o */
	if (f4 & (TR4_MUST2H | TR4_SHOULD2H)) {
		total += (slay * ((o_ptr->dd * (o_ptr->ds + 1)) + 70)) / 80;
	} else {
		total += (slay * ((o_ptr->dd * (o_ptr->ds + 1)) + 30)) / 35;
	}


	if (f4 & TR4_CLONE) total >>= 1;

	return total;
}

/* Return a sensible pricing for randarts, which
   gets added to k_info base item price - C. Blue */
s64b artifact_value_real(int Ind, object_type *o_ptr)
{
	u32b f1, f2, f3, f4, f5, esp;
	object_kind *k_ptr = &k_info[o_ptr->k_idx];
	bool star = (Ind == 0 || object_fully_known_p(Ind, o_ptr));

	/* Base cost */
	s64b value = k_ptr->cost, i;
	/* Hack -- "worthless" items */
	if (!value) return (0L);

	/* Extract some flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	/* Artifact */
	if (o_ptr->name1) {
		artifact_type *a_ptr;

		/* Randarts */
		if (o_ptr->name1 == ART_RANDART) {
			a_ptr = randart_make(o_ptr);
			if ((a_ptr->flags4 & TR4_CURSE_NO_DROP) || (a_ptr->flags3 & TR3_AUTO_CURSE)) {
				value = 0;
			} else {
				/* start with base item kind price (k_info) */
				value = object_value_base(0, o_ptr);
				/* randarts get a base value boost */
				value += 5000;// + o_ptr->level * 200;
				/* randart ammo is always very useful so it gets an extra base value boost */
				if (is_ammo(o_ptr->tval)) value += 5000;
				/* add full value if *ID*ed */
				if (star) value += artifact_flag_cost(o_ptr, o_ptr->pval);

				/* maybe todo (see function below):
				value = artifact_value_real(a_ptr);
				*/
			}
			if (value < 0) value = 0;
		} else {
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

	/* Hack */
	if ((f4 & TR4_CURSE_NO_DROP) || (f3 & TR3_AUTO_CURSE))
		return 0;

	/* Bad items don't sell. Good items with some bad modifiers DO sell ((*defenders*)). -C. Blue */
	switch (o_ptr->tval) {
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
	case TV_CROWN:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_DRAG_ARMOR:
		if ((((o_ptr->to_h) < 0 && ((o_ptr->to_h - k_ptr->to_h) < 0)) ||
		    ((o_ptr->to_d) < 0 && ((o_ptr->to_d - k_ptr->to_d) < 0 || k_ptr->to_d < 0)) ||
		    ((o_ptr->to_a) < 0 && ((o_ptr->to_a - k_ptr->to_a) < 0 || k_ptr->to_a < 0)) ||
		    (o_ptr->pval < 0) || (o_ptr->bpval < 0)) &&
		    !(((o_ptr->to_h) > 0) ||
		    ((o_ptr->to_d) > 0) ||
		    ((o_ptr->to_a) > 0) ||
		    (o_ptr->pval > 0) || (o_ptr->bpval > 0))) return (0L);
		break;

	case TV_DIGGING:
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_AXE:
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
	case TV_BOW:
	case TV_BOOMERANG:

	case TV_MSTAFF:
	case TV_LITE:
	case TV_AMULET:
	case TV_RING:
	case TV_TRAPKIT:

	default:
		if ((((o_ptr->to_h) < 0 && ((o_ptr->to_h - k_ptr->to_h) < 0 || k_ptr->to_h < 0)) ||
		    ((o_ptr->to_d) < 0 && ((o_ptr->to_d - k_ptr->to_d) < 0 || k_ptr->to_d < 0)) ||
		    ((o_ptr->to_a) < 0 && ((o_ptr->to_a - k_ptr->to_a) < 0 || k_ptr->to_a < 0)) ||
/* to allow Mummy Wrappings in bm! - C. Blue */
//		    (o_ptr->pval < 0) || (o_ptr->bpval < 0)) &&
//		    (o_ptr->pval < 0) || (o_ptr->bpval < k_ptr->pval)) &&
//		    (o_ptr->pval < 0) || (o_ptr->tval != TV_ROD && o_ptr->bpval < k_ptr->pval)) &&
		    (o_ptr->pval < 0) || (o_ptr->bpval < 0 && o_ptr->bpval < k_ptr->pval)) &&
		    !(((o_ptr->to_h) > 0) ||
		    ((o_ptr->to_d) > 0) ||
		    ((o_ptr->to_a) > 0) ||
		    (o_ptr->pval > 0) || (o_ptr->bpval > 0))) return (0L);
		break;
	}

	/* Analyze pval bonus */
	switch (o_ptr->tval) {
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_BOW:
		case TV_BOOMERANG:
		case TV_AXE:
		case TV_MSTAFF:
		case TV_DIGGING:
		case TV_BLUNT:
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
			int pval = o_ptr->bpval, kpval = k_ptr->pval;
			/* If the bpval has been set to the k_info pval,
			   don't increase the item's value for this
			   granted pval, since it's already included in
			   the k_info price! */
			if (pval >= kpval) {
				pval -= kpval;
				kpval = 0;
			}

//			int boost = 1 << pval;

			/* Hack -- Negative "pval" is always bad */
//			if (pval < 0) return (0L);

			for (i = 0; i < 2; i++) {
				int count = 0;

				/* No pval */
//				if (!pval)
				if (pval <= 0) {
					pval = o_ptr->pval;
					continue;
				}
				/* If the k_info pval became the object's
				   pval instead of bpval (shouldn't happen)
				   then take care of it and again don't
				   increase the value for this granted pval: */
				else if (pval >= kpval) {
					pval -= kpval;
					kpval = 0;
				}

				/* Give credit for stat bonuses */
				//			if (f1 & TR1_STR) value += (pval * 200L);
				//			if (f1 & TR1_STR) value += (boost * 200L);
				if (f1 & TR1_STR) count++;
				if (f1 & TR1_INT) count++;
				if ((f1 & TR1_WIS) && !(f1 & TR1_INT)) count++; /* slightly useless combination */
				if (f1 & TR1_DEX) count++;
				if (f1 & TR1_CON) count++;
				if (f1 & TR1_CHR) value += pval * 1000;

				/* hack for double-stat rings - C. Blue */
				if ((o_ptr->tval == TV_RING) && (
				    (o_ptr->sval == SV_RING_MIGHT) ||
				    (o_ptr->sval == SV_RING_READYWIT) ||
				    (o_ptr->sval == SV_RING_TOUGHNESS) ||
				    (o_ptr->sval == SV_RING_CUNNINGNESS))
				    ) {
					count /= 2;
					if (count) value += count * PRICE_BOOST((count + pval), 2, 1)* 300L;
				} else {
					if (count) value += count * PRICE_BOOST((count + pval), 2, 1)* 200L;
				}

//				if (f5 & (TR5_CRIT)) value += (PRICE_BOOST(pval, 0, 1)* 300L);//was 500, then 400
				if (f5 & (TR5_CRIT)) value += pval * pval * 5000L;/* was 20k, but speed is only 10k */
				if (f5 & (TR5_LUCK)) value += (PRICE_BOOST(pval, 0, 1)* 10L);

				/* Give credit for stealth and searching */
//				if (f1 & TR1_STEALTH) value += (PRICE_BOOST(pval, 3, 1) * 100L);
				if (f1 & TR1_STEALTH) value += (pval + 1) * (pval + 1) * 400L;//100
				if (f1 & TR1_SEARCH) value += pval * pval * 200L;//200
				if (f5 & TR5_DISARM) value += pval * pval * 100L;

				/* Give credit for infra-vision and tunneling */
				if (f1 & TR1_INFRA) value += pval * pval * 150L;//100
				if (f1 & TR1_TUNNEL) value += pval * pval * 175L;//50

				/* Give credit for extra attacks */
				if (o_ptr->tval == TV_RING) {
					if (f1 & TR1_BLOWS) value += (PRICE_BOOST(pval, 0, 1) * 2000L);//1500
				} else {
//					if (f1 & TR1_BLOWS) value += (PRICE_BOOST(pval, 0, 1) * 3000L);
					if (f1 & TR1_BLOWS) value += pval * 20000L;
				}

				/* Give credit for extra casting */
				if (f1 & TR1_SPELL) value += (PRICE_BOOST(pval, 0, 1) * 4000L);

				/* Give credit for extra HP bonus */
				if (f1 & TR1_LIFE) value += (PRICE_BOOST(pval, 0, 1) * 3000L);


				/* Flags moved here exclusively from flag_cost */
				if (f1 & TR1_MANA) value += (700 * pval * pval);
				/* End of flags, moved here from flag_cost */


				/* Hack -- amulets of speed and rings of speed are
				 * cheaper than other items of speed.
				 */
				if (o_ptr->tval == TV_AMULET) {
					/* Give credit for speed bonus */
					//				if (f1 & TR1_SPEED) value += (boost * 25000L);
					if (f1 & TR1_SPEED) value += pval * pval * 5000L;
				} else if (o_ptr->tval == TV_RING) {
					/* Give credit for speed bonus */
					//				if (f1 & TR1_SPEED) value += (PRICE_BOOST(pval, 0, 4) * 50000L);
					if (f1 & TR1_SPEED) value += pval * pval * 10000L;
//					if (f1 & TR1_SPEED) value += pval * pval * 7000L;
				}
//				else if (f1 & TR1_SPEED) value += (PRICE_BOOST(pval, 0, 4) * 100000L);
//				else if (f1 & TR1_SPEED) value += pval * pval * 10000L;
				else if (f1 & TR1_SPEED) value += (pval + 1) * (pval + 1) * 7000L;

				pval = o_ptr->pval;

				if (o_ptr->name2) {
					artifact_type *a_ptr;

					a_ptr = ego_make(o_ptr);
					f1 &= ~(k_ptr->flags1 & TR1_PVAL_MASK & ~a_ptr->flags1);
					f5 &= ~(k_ptr->flags5 & TR5_PVAL_MASK & ~a_ptr->flags5);
				}
			}
			break;
		}
	}


	/* Analyze the item */
	switch (o_ptr->tval) {
		case TV_BOOK:
			if (o_ptr->sval == SV_SPELLBOOK) {
				/* 1: 145, 2: 240, 3: 375, 4: 540, 5: 735 */
				int sl = school_spells[o_ptr->pval].skill_level + 5;
				/* override k_info.txt to have easier handling of possible changes here */
				value = 4;
				/* Pay extra for the spell */
				value = value * (sl * sl);
			}
			/* Done */
			break;

		/* Wands/Staffs */
		case TV_WAND:
			/* Pay extra for charges */
			value += ((value / 20) * o_ptr->pval) / o_ptr->number;
			/* Done */
			break;

		case TV_STAFF:
			/* Pay extra for charges */
			value += ((value / 20) * o_ptr->pval);
			/* Done */
			break;

		/* Rings/Amulets */
		case TV_RING:
		case TV_AMULET:
#if 0
			/* Hack -- negative bonuses are bad */
			if (o_ptr->to_a < 0) return (0L);
			if (o_ptr->to_h < 0) return (0L);
			if (o_ptr->to_d < 0) return (0L);
#endif
			if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH)) {
				value += r_info[o_ptr->pval].level * r_info[o_ptr->pval].mexp;
			}

			/* Give credit for bonuses */
//			value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);
			/* Ignore base boni that come from k_info.txt (eg quarterstaff +10 AC) */
			value += ((PRICE_BOOST(o_ptr->to_h, 12, 4) + 
				PRICE_BOOST(o_ptr->to_d, 7, 3) + 
				PRICE_BOOST(o_ptr->to_a, 11, 4)) * 100L);

			/* Done */
			break;

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
#if 0
			/* Hack -- negative armor bonus */
			if (o_ptr->to_a < 0) return (0L);
#endif
			/* Give credit for bonuses */
//			value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);
			/* Ignore base boni that come from k_info.txt (eg quarterstaff +10 AC) */
			value += (  ((o_ptr->to_h <= 0 || o_ptr->to_h <= k_ptr->to_h)? 0 : 
				    ((k_ptr->to_h < 0)? PRICE_BOOST(o_ptr->to_h, 9, 5): 
				    PRICE_BOOST((o_ptr->to_h - k_ptr->to_h), 9, 5))) + 
				    ((o_ptr->to_d <= 0 || o_ptr->to_d <= k_ptr->to_d)? 0 :
				    ((k_ptr->to_d < 0)? PRICE_BOOST(o_ptr->to_d, 9, 5):
				    PRICE_BOOST((o_ptr->to_d - k_ptr->to_d), 9, 5))) + 
				    ((o_ptr->to_a <= 0 || o_ptr->to_a <= k_ptr->to_a)? 0 :
				    ((k_ptr->to_a < 0)? PRICE_BOOST(o_ptr->to_a, 9, 5):
				    PRICE_BOOST((o_ptr->to_a - k_ptr->to_a), 9, 5))) ) * 100L;

			/* Costumes */
			if ((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_COSTUME)) {
				value += r_info[o_ptr->bpval].mexp;
			}

			/* Done */
			break;

		/* Bows/Weapons */
		case TV_BOW:
		case TV_BOOMERANG:
		case TV_AXE:
		case TV_DIGGING:
		case TV_BLUNT:
		case TV_SWORD:
		case TV_POLEARM:
		case TV_MSTAFF:
		case TV_TRAPKIT:
#if 0
			/* Hack -- negative hit/damage bonuses */
			if (o_ptr->to_h + o_ptr->to_d < 0) {
				/* Hack -- negative hit/damage are of no importance */
				if (o_ptr->tval == TV_MSTAFF) break;
				if (o_ptr->name2 == EGO_STAR_DF) break;
				else return (0L);
			}
#endif
			/* Factor in the bonuses */
//			value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);
			/* Ignore base boni that come from k_info.txt (eg quarterstaff +10 AC) */
			value += (  ((o_ptr->to_h <= 0 || o_ptr->to_h <= k_ptr->to_h)? 0 :
				    ((k_ptr->to_h < 0)? PRICE_BOOST(o_ptr->to_h, 9, 5):
				    PRICE_BOOST((o_ptr->to_h - k_ptr->to_h), 9, 5))) + 
				    ((o_ptr->to_d <= 0 || o_ptr->to_d <= k_ptr->to_d)? 0 :
				    ((k_ptr->to_d < 0)? PRICE_BOOST(o_ptr->to_d, 9, 5):
				    PRICE_BOOST((o_ptr->to_d - k_ptr->to_d), 9, 5))) + 
				    ((o_ptr->to_a <= 0 || o_ptr->to_a <= k_ptr->to_a)? 0 :
				    ((k_ptr->to_a < 0)? PRICE_BOOST(o_ptr->to_a, 9, 5):
				    PRICE_BOOST((o_ptr->to_a - k_ptr->to_a), 9, 5))) ) * 100L;

			/* Hack -- Factor in extra damage dice */
			if (((o_ptr->dd > k_ptr->dd) && (o_ptr->ds == k_ptr->ds)) &&
			    (o_ptr->dd > 0 && o_ptr->ds > 0))
			{
				value += PRICE_BOOST((o_ptr->dd - k_ptr->dd), 1, 4) * o_ptr->ds * 100L;
			}

			/* Done */
			break;

		/* Ammo */
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
			/* Hack -- negative hit/damage bonuses */
//			if (o_ptr->to_h + o_ptr->to_d < 0) return (0L);

			/* Factor in the bonuses */
//			value += ((o_ptr->to_h + o_ptr->to_d) * 5L);
			/* Ignore base boni that come from k_info.txt (eg quarterstaff +10 AC) */
			value += (  ((o_ptr->to_h <= 0 || o_ptr->to_h <= k_ptr->to_h)? 0 :
				    ((k_ptr->to_h < 0)? PRICE_BOOST(o_ptr->to_h, 9, 5):
				    PRICE_BOOST((o_ptr->to_h - k_ptr->to_h), 9, 5))) + 
				    ((o_ptr->to_d <= 0 || o_ptr->to_d <= k_ptr->to_d)? 0 :
				    ((k_ptr->to_d < 0)? PRICE_BOOST(o_ptr->to_d, 9, 5):
				    PRICE_BOOST((o_ptr->to_d - k_ptr->to_d), 9, 5)))  ) * 5L;

			/* Hack -- Factor in extra damage dice */
			if (((o_ptr->dd > k_ptr->dd) && (o_ptr->ds == k_ptr->ds)) &&
			    (o_ptr->dd > 0 && o_ptr->ds > 0))
			{
				value += (o_ptr->dd - k_ptr->dd) * (o_ptr->ds - k_ptr->ds) * 5L;
			}

			/* Done */
			break;
	}

#ifdef RANDART_PRICE_BONUS /* just disable in case some randarts end up with outrageous value */
	/* OPTIONAL/EXPERIMENTAL: Add extra bonus for ranged weapon that has absolute top damage */
	if (o_ptr->tval == TV_BOW && (i = o_ptr->to_h + o_ptr->to_d * 2) >= 60) {
		i = i - 30;
		if (f3 & TR3_XTRA_SHOTS) i *= 2;
		if (f3 & TR3_XTRA_MIGHT) i *= 4;
		value += i * 500;
	}
	/* OPTIONAL/EXPERIMENTAL: Add extra bonus for armour that has both, top AC and resists */
	else if (is_armour(o_ptr->tval) && o_ptr->to_a >= 25) {
		i = artifact_flag_rating_armour(o_ptr);
		if (i >= 15) value += (o_ptr->to_a + i - 30) * 3000;
	}
	/* OPTIONAL/EXPERIMENTAL: Add extra bonus for weapon that has both, top hit/_dam_ and ea/crit/vamp */
	else if (is_weapon(o_ptr->tval) && (i = o_ptr->to_h + o_ptr->to_d * 2) >= 60) {
		i = artifact_flag_rating_weapon(o_ptr) * 4;
		if (i >= 24) value += (o_ptr->to_h + o_ptr->to_d * 2 + i - 70) * 2000;
	}
#endif

	if (f3 & TR3_AGGRAVATE) value >>= 1; /* one generic aggravate penalty fits it all */
	if (f3 & TR3_NO_TELE) value >>= 1; /* generic no-tele penalty fits too ^^ */

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
s64b object_value(int Ind, object_type *o_ptr)
{
	s64b value;


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
		if ((o_ptr->ident & ID_SENSED_ONCE) && broken_p(o_ptr)) return (0L);

		/* Hack -- Felt cursed items */
		if ((o_ptr->ident & ID_SENSED_ONCE) && cursed_p(o_ptr)) return (0L);

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
 *
 * 'tolerance' flag:
 * 0	- no tolerance
 * +0x1	- tolerance for ammo to_h and to_d enchantment
 * +0x2	- tolerance for level 0 items
 * -- C. Blue
 */
bool object_similar(int Ind, object_type *o_ptr, object_type *j_ptr, s16b tolerance)
{
	player_type *p_ptr = NULL;
	int total = o_ptr->number + j_ptr->number;

	/* Hack -- gold always merge */
//	if (o_ptr->tval == TV_GOLD && j_ptr->tval == TV_GOLD) return(TRUE);

	/* Require identical object types */
	if (o_ptr->k_idx != j_ptr->k_idx) return (FALSE);

	/* Level 0 items and other items won't merge, since level 0 can't be sold to shops */
	if (!(tolerance & 0x2) &&
	    (!o_ptr->level || !j_ptr->level) &&
	    (o_ptr->level != j_ptr->level))
		return (FALSE);

	/* Require same owner or convertable to same owner */
//
/*	if (o_ptr->owner != j_ptr->owner) return (FALSE); */
	if (Ind) {
		p_ptr = Players[Ind];
		if (((o_ptr->owner != j_ptr->owner)
			&& ((p_ptr->lev < j_ptr->level)
			|| (j_ptr->level < 1)))
			&& (j_ptr->owner)) return (FALSE);
		if ((o_ptr->owner != p_ptr->id)
			&& (o_ptr->owner != j_ptr->owner)) return (FALSE);

		/* Require objects from the same modus! */
		/* A non-everlasting player won't have his items stacked w/ everlasting stuff */
		if (compat_pomode(Ind, j_ptr)) return(FALSE);
	} else {
		if (o_ptr->owner != j_ptr->owner) return (FALSE);
		/* no stacks of unowned everlasting items in shops after a now-dead
		   everlasting player sold an item to the shop before he died :) */
		if (compat_omode(o_ptr, j_ptr)) return(FALSE);
	}

	/* Analyze the items */
	switch (o_ptr->tval) {
		/* Chests */
		case TV_KEY:
		case TV_CHEST:
		{
			/* Never okay */
			return (FALSE);
		}

		/* Food and Potions and Scrolls */
		case TV_SCROLL:
			/* cheques may have different value, so they must not stack */
			if (o_ptr->sval == SV_SCROLL_CHEQUE) return FALSE;
		case TV_FOOD:
		case TV_POTION:
		case TV_POTION2:
		{
			/* Hack for ego foods :) */
			if (o_ptr->name2 != j_ptr->name2) return (FALSE);
			if (o_ptr->name2b != j_ptr->name2b) return (FALSE);

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
				!object_known_p(Ind, j_ptr))) return(FALSE);

			/* Beware artifatcs should not combine with "lesser" thing */
			if (o_ptr->name1 != j_ptr->name1) return (FALSE);

			/* Do not combine recharged ones with non recharged ones. */
//			if ((f4 & TR4_RECHARGED) != (f14 & TR4_RECHARGED)) return (FALSE);

			/* Do not combine different ego or normal ones */
			if (o_ptr->name2 != j_ptr->name2) return (FALSE);

			/* Do not combine different ego or normal ones */
			if (o_ptr->name2b != j_ptr->name2b) return (FALSE);

			/* Assume okay */
			break;
		}

		case TV_STAFF:
		{
			/* Require knowledge */
			if (!Ind || !object_known_p(Ind, o_ptr) || !object_known_p(Ind, j_ptr)) return (FALSE);

			if (!Ind || !p_ptr->stack_allow_wands) return (FALSE);

			/* Require identical charges */
			if (o_ptr->pval != j_ptr->pval) return (FALSE);
			
			if (o_ptr->name2 != j_ptr->name2) return (FALSE);
			if (o_ptr->name2b != j_ptr->name2b) return (FALSE);

			/* Probably okay */
			break;

			/* Fall through */
			/* NO MORE FALLING THROUGH! MUHAHAHA the_sandman */
		}

		/* Staffs and Wands and Rods */
		case TV_ROD:
		{
			/* Overpoweredness, Hello! - the_sandman */
			if (o_ptr->sval == SV_ROD_HAVOC) return (FALSE);

			/* Require permission */
			if (!Ind || !p_ptr->stack_allow_wands) return (FALSE);

			/* this is only for rods... the_sandman */
			if (o_ptr->pval == 0 && j_ptr->pval != 0) return (FALSE); //lol :)
			
			if (o_ptr->name2 != j_ptr->name2) return (FALSE);
			if (o_ptr->name2b != j_ptr->name2b) return (FALSE);

			/* Probably okay */
			break;
		}

		/* Weapons and Armor */
		case TV_DRAG_ARMOR:	return(FALSE);
		case TV_BOW:
		case TV_BOOMERANG:
		case TV_DIGGING:
		case TV_BLUNT:
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
		case TV_TRAPKIT: /* so they don't stack carelessly - the_sandman */
		{
			/* Require permission */
			if (!Ind || !p_ptr->stack_allow_items) return (FALSE);

			/* XXX XXX XXX Require identical "sense" status */
			/* if ((o_ptr->ident & ID_SENSE) != */
			/*     (j_ptr->ident & ID_SENSE)) return (FALSE); */

			/* Costumes must be for same monster */
			if ((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_COSTUME)) {
				if (o_ptr->bpval != j_ptr->bpval) return(FALSE);
			}

			/* Fall through */
		}

		/* Rings, Amulets, Lites */
		case TV_RING:
			/* no more, due to their 'timeout' ! */
			if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH)) return (FALSE);
		case TV_AMULET:
		case TV_LITE:
		case TV_TOOL:
		case TV_BOOK:	/* Books can be 'fireproof' */
		{
			/* custom tomes which appear identical, spell-wise too, may stack */
			if (o_ptr->tval == TV_BOOK &&
			    o_ptr->sval >= SV_CUSTOM_TOME_1 &&
			    o_ptr->sval < SV_SPELLBOOK &&
			    ((o_ptr->xtra1 != j_ptr->xtra1) ||
			    (o_ptr->xtra2 != j_ptr->xtra2) ||
			    (o_ptr->xtra3 != j_ptr->xtra3) ||
			    (o_ptr->xtra4 != j_ptr->xtra4) ||
			    (o_ptr->xtra5 != j_ptr->xtra5) ||
			    (o_ptr->xtra6 != j_ptr->xtra6) ||
			    (o_ptr->xtra7 != j_ptr->xtra7) ||
			    (o_ptr->xtra8 != j_ptr->xtra8) ||
			    (o_ptr->xtra9 != j_ptr->xtra9)))
				return(FALSE);

//			if (o_ptr->tval == TV_BOOK) { /* this was probably only meant for books..? */
			/* Require full knowledge of both items */
			if (!Ind || !object_known_p(Ind, o_ptr) ||
//			    !object_known_p(Ind, j_ptr) || (o_ptr->name3)) return (FALSE);
			    !object_known_p(Ind, j_ptr))
				return (FALSE);
//			}

			/* different bpval? */
			if (o_ptr->bpval != j_ptr->bpval) return(FALSE);

			/* Fall through */
		}

		/* Missiles */
		case TV_BOLT:
		case TV_ARROW:
		case TV_SHOT:
		{
			/* Require identical "bonuses" -
			   except for ammunition which carries special inscription (will merge!) - C. Blue */
			if (!((tolerance & 0x1) && !(cursed_p(o_ptr) || cursed_p(j_ptr) ||
						    artifact_p(o_ptr) || artifact_p(j_ptr))) ||
			    (!is_ammo(o_ptr->tval) ||
			    (!check_guard_inscription(o_ptr->note, 'M') && !check_guard_inscription(j_ptr->note, 'M')))) {
				if (o_ptr->to_h != j_ptr->to_h) return (FALSE);
				if (o_ptr->to_d != j_ptr->to_d) return (FALSE);
			}
			if (o_ptr->to_a != j_ptr->to_a) return (FALSE);

			/* Require identical "pval" code */
			if (o_ptr->pval != j_ptr->pval) return (FALSE);

			/* Require identical "artifact" names <- this shouldnt happen right? */
			if (o_ptr->name1 != j_ptr->name1) return (FALSE);

			/* Require identical "ego-item" names.
			   Allow swapped ego powers: Ie Arrow (SlayDragon,Ethereal) combines with Arrow (Ethereal,SlayDragon).
			   Note: This code assumes there's no ego power which can be both prefix and postfix. */
#if 0
			/* This is buggy, it allows stacking of normal items with items that only have one ego power - mikaelh */
			if ((o_ptr->name2 != j_ptr->name2) && (o_ptr->name2 != j_ptr->name2b)) return (FALSE);
			if ((o_ptr->name2b != j_ptr->name2) && (o_ptr->name2b != j_ptr->name2b)) return (FALSE);
#else
			/* This one only allows name2 and name2b to be swapped */
			if (! ((o_ptr->name2 == j_ptr->name2b) && (o_ptr->name2b == j_ptr->name2)))
			{
				if (o_ptr->name2 != j_ptr->name2) return (FALSE);
				if (o_ptr->name2b != j_ptr->name2b) return (FALSE);
			}
#endif

			/* Require identical random seeds
			   hack: fix 'old' ammo that didn't have NO_SEED flag yet.. */
			if (!is_ammo(o_ptr->tval) && o_ptr->name3 != j_ptr->name3) return (FALSE);

			/* Hack -- Never stack "powerful" items */
//			if (o_ptr->xtra1 || j_ptr->xtra1) return (FALSE);

			/* Hack -- Never stack recharging items */
			if (o_ptr->timeout != j_ptr->timeout) return (FALSE);

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
				    !object_known_p(Ind, j_ptr))) return (FALSE);

			/* Probably okay */
			break;
		}
	}


	/* Hack -- Require identical "cursed" status */
	if ((o_ptr->ident & ID_CURSED) != (j_ptr->ident & ID_CURSED)) return (FALSE);

	/* Hack -- Require identical "broken" status */
	if ((o_ptr->ident & ID_BROKEN) != (j_ptr->ident & ID_BROKEN)) return (FALSE);


	/* Hack -- require semi-matching "inscriptions" */
	/* Hack^2 -- books do merge.. it's to prevent some crashes */
	if (o_ptr->note && j_ptr->note && (o_ptr->note != j_ptr->note)
		&& strcmp(quark_str(o_ptr->note), "on sale")
		&& strcmp(quark_str(j_ptr->note), "on sale")
		&& !is_realm_book(o_ptr)
		&& !check_guard_inscription(o_ptr->note, 'M')
		&& !check_guard_inscription(j_ptr->note, 'M')) return (FALSE);

	/* Hack -- normally require matching "inscriptions" */
	if ((!Ind || !p_ptr->stack_force_notes) && (o_ptr->note != j_ptr->note)) return (FALSE);

	/* Hack -- normally require matching "discounts" */
	if ((!Ind || !p_ptr->stack_force_costs) && (o_ptr->discount != j_ptr->discount)) return (FALSE);


	/* Maximal "stacking" limit */
	if (total >= MAX_STACK_SIZE) return (FALSE);

	/* An everlasting player will have _his_ items stack w/ non-everlasting stuff
	   (especially new items bought in the shops) and convert them all to everlasting */
	if (Ind && (p_ptr->mode & MODE_EVERLASTING)) {
		o_ptr->mode = MODE_EVERLASTING;
		j_ptr->mode = MODE_EVERLASTING;
	}

	/* A PvP-player will get his items convert to pvp-mode */
	if (Ind && (p_ptr->mode & MODE_PVP)) {
		o_ptr->mode = MODE_PVP;
		j_ptr->mode = MODE_PVP;
	}

	/* They match, so they must be similar */
	return (TRUE);
}


/*
 * Allow one item to "absorb" another, assuming they are similar
 */
void object_absorb(int Ind, object_type *o_ptr, object_type *j_ptr)
{
	int total = o_ptr->number + j_ptr->number;

        /* Prepare ammo for possible combining */
//	int o_to_h, o_to_d;
	bool merge_inscriptions = check_guard_inscription(o_ptr->note, 'M') || check_guard_inscription(j_ptr->note, 'M');
	bool merge_ammo = (is_ammo(o_ptr->tval) && merge_inscriptions);

	/* Combine ammo even of different enchantment grade! - C. Blue */
	if (merge_ammo) {
		o_ptr->to_h = ((o_ptr->to_h * o_ptr->number) + (j_ptr->to_h * j_ptr->number)) / (o_ptr->number + j_ptr->number);
		o_ptr->to_d = ((o_ptr->to_d * o_ptr->number) + (j_ptr->to_d * j_ptr->number)) / (o_ptr->number + j_ptr->number);
	}

	/* Add together the item counts */
	o_ptr->number = ((total < MAX_STACK_SIZE) ? total : (MAX_STACK_SIZE - 1));

	/* NEVER clone gold!!! - mikaelh
	 * o_ptr->number > 1 gold could be seperated by eg. bashing
	 * creating/destroying (o_ptr->pval - j_ptr->pval) gold.
	 * Colour won't always be right but let's make one item with combined
	 * amount of gold?
	 */
	if (o_ptr->tval == TV_GOLD) {
		o_ptr->number = 1;
		o_ptr->pval += j_ptr->pval;
	}

	/* Hack -- blend "known" status */
	if (Ind && object_known_p(Ind, j_ptr)) object_known(o_ptr);

	/* Hack -- blend "rumour" status */
	if (j_ptr->ident & ID_RUMOUR) o_ptr->ident |= ID_RUMOUR;

	/* Hack -- blend "mental" status */
	if (j_ptr->ident & ID_MENTAL) o_ptr->ident |= ID_MENTAL;

	/* Hack -- blend "inscriptions" */
//	if (j_ptr->note) o_ptr->note = j_ptr->note;
//	if (o_ptr->note) j_ptr->note = o_ptr->note;

	/* Usually, the old object 'j_ptr' takes over the inscription of added object 'o_ptr'.
	   However, in some cases it's reversed, if..
	   o_ptr's inscription is empty or one of the automatic inscriptions AND j_ptr's
	   inscription is not empty, or..
	   o_ptr's inscription contains the !M tag and j_ptr does not and j_ptr isnt one of
	   the automatic inscriptions. */
	if (j_ptr->note &&
	    (!o_ptr->note || streq(quark_str(o_ptr->note), "handmade") || streq(quark_str(o_ptr->note), "stolen"))) {
		o_ptr->note = j_ptr->note;
	}
	else if (merge_inscriptions) {
		if (check_guard_inscription(o_ptr->note, 'M') && (!check_guard_inscription(j_ptr->note, 'M'))
		    && (j_ptr->note) && strcmp(quark_str(j_ptr->note), "handmade") && strcmp(quark_str(j_ptr->note), "stolen"))
			o_ptr->note = j_ptr->note;
	}

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

	/* Hack -- if rods are stacking, average out the timeout. the_sandman */
	if (o_ptr->tval == TV_ROD)
	{
		o_ptr->pval = (o_ptr->number) * (o_ptr->pval) + (j_ptr->pval);
		o_ptr->pval = (o_ptr->pval) / (o_ptr->number + 1);
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
	s_printf("No object (%d,%d)\n", tval, sval);
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
static bool make_artifact_special(struct worldpos *wpos, object_type *o_ptr, u32b resf)
{
	int	i, d, dlev = getlevel(wpos);
	int	k_idx = 0;

	/* Check if artifact generation is currently disabled -
	   added this for maintenance reasons -C. Blue */
	if (cfg.arts_disabled) return (FALSE);

	/* No artifacts in the town */
	if (istown(wpos)) return (FALSE);

	/* Check the artifact list (just the "specials") */
//	for (i = 0; i < ART_MIN_NORMAL; i++)
	for (i = 0; i < MAX_A_IDX; i++) {
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
		if (a_ptr->level > dlev)
		{
			/* Acquire the "out-of-depth factor" */
			d = (a_ptr->level - dlev) * 2;

			/* Roll for out-of-depth creation */
			if (rand_int(d) != 0) continue;
		}

		/* New: Prevent too many low-level artifacts on high dungeon levels.
		   They tend to spawn very often due to their relatively low rarity,
		   and are simply a small piece of cash, or even annoying if unsellable (Gorlim). */
		d = ((dlev - 20) / 2) - a_ptr->level;
		/* Don't start checking before dungeon level 40. */
		if ((dlev >= 40) && (d > 0) && !magik(350 / (d + 3))) continue;

		/* Artifact "rarity roll" */
		if (rand_int(a_ptr->rarity) != 0) return (FALSE);

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

		/* swallow true artifacts if true_art isn't allowed
		   (meaning that a king/queen did the monster kill!) */
		if (resf & RESF_NOTRUEART) {
			/* add exception for WINNERS_ONLY true arts
			   (this is required for non DROP_CHOSEN/SPECIAL_GENE winner arts): */
#if 0 /* basic way */
			if (!((a_ptr->flags5 & TR5_WINNERS_ONLY) && (resf & RESF_WINNER)))
#else /* restrict to level 50+ in case of _fallen_ winners */
			if (!((a_ptr->flags5 & TR5_WINNERS_ONLY) && (resf & RESF_LIFE)))
#endif
				return (FALSE); /* Don't replace them with randarts! */
		}

		/* Assign the template */
		invcopy(o_ptr, k_idx);

		/* Mega-Hack -- mark the item as an artifact */
		o_ptr->name1 = i;

		/* Hack -- Mark the artifact as "created" */
		handle_art_inum(o_ptr->name1);

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
static bool make_artifact(struct worldpos *wpos, object_type *o_ptr, u32b resf)
{
	int i, tries = 0, d, dlev = getlevel(wpos);
	artifact_type *a_ptr;

	/* No artifacts in the town */
	if (istown(wpos)) return (FALSE);

	/* Paranoia -- no "plural" artifacts */
	if (o_ptr->number != 1) return (FALSE);

        /* Check if true artifact generation is currently disabled -
	added this for maintenance reasons -C. Blue */
	if (!cfg.arts_disabled) {
		/* Check the artifact list (skip the "specials") */
	//	for (i = ART_MIN_NORMAL; i < MAX_A_IDX; i++)
	        for (i = 0; i < MAX_A_IDX; i++) {
			artifact_type *a_ptr = &a_info[i];

			/* Skip "empty" items */
			if (!a_ptr->name) continue;

			/* Cannot make an artifact twice */
			if (a_ptr->cur_num) continue;

			/* Cannot generate special ones */
			if (a_ptr->flags3 & TR3_INSTA_ART) continue;

			/* Cannot generate some artifacts because they can only exists in special dungeons/quests/... */
	//		if ((a_ptr->flags4 & TR4_SPECIAL_GENE) && (!a_allow_special[i]) && (!vanilla_town)) continue;
			if (a_ptr->flags4 & TR4_SPECIAL_GENE) continue;

			/* Must have the correct fields */
			if (a_ptr->tval != o_ptr->tval) continue;
			if (a_ptr->sval != o_ptr->sval) continue;

			/* XXX XXX Enforce minimum "depth" (loosely) */
			if (a_ptr->level > dlev) {
				/* Acquire the "out-of-depth factor" */
				d = (a_ptr->level - dlev) * 2;

				/* Roll for out-of-depth creation */
				if (rand_int(d) != 0) continue;
			}

			/* New: Prevent too many low-level artifacts on high dungeon levels.
			   They tend to spawn very often due to their relatively low rarity,
			   and are simply a small piece of cash, or even annoying if unsellable (Gorlim). */
			d = ((dlev - 20) / 2) - a_ptr->level;
			/* Don't start checking before dungeon level 40. */
			if ((dlev >= 40) && (d > 0) && !magik(350 / (d + 3))) continue;

			/* We must make the "rarity roll" */
			if (rand_int(a_ptr->rarity) != 0) continue;

			/* swallow true artifacts if true_art isn't allowed
			   (meaning that a king/queen did the monster kill!) */
			if (resf & RESF_NOTRUEART) {
				/* add exception for WINNERS_ONLY true arts
				   (this is required for non DROP_CHOSEN/SPECIAL_GENE winner arts): */
#if 0 /* basic way */
				if (!((a_ptr->flags5 & TR5_WINNERS_ONLY) && (resf & RESF_WINNER)))
#else /* restrict to level 50+ in case of _fallen_ winners */
				if (!((a_ptr->flags5 & TR5_WINNERS_ONLY) && (resf & RESF_LIFE)))
#endif
					return (FALSE); /* Don't replace them with randarts! */
			}

			/* Hack -- mark the item as an artifact */
			o_ptr->name1 = i;

			/* Hack -- Mark the artifact as "created" */
			handle_art_inum(o_ptr->name1);

			/* Success */
			return (TRUE);
		}
	}

	/* Break here if randarts aren't allowed */
	if (resf & RESF_NORANDART) return (FALSE);

	/* An extra chance at being a randart. XXX RANDART */
	if (!rand_int(RANDART_RARITY)) {
		/* Randart ammo should be very rare! */
		if (is_ammo(o_ptr->tval) && magik(80)) return(FALSE); /* was 95 */

		/* We turn this item into a randart! */
		o_ptr->name1 = ART_RANDART;

/* NOTE: MAKE SURE FOLLOWING CODE IS CONSISTENT WITH create_artifact_aux() IN spells2.c! */

		/* Start loop. Break when artifact is allowed */
		while (tries < 10) {
			tries++;

			/* Piece together a 32-bit random seed */
			o_ptr->name3 = rand_int(0xFFFF) << 16;
			o_ptr->name3 += rand_int(0xFFFF);

			/* Check the tval is allowed */
			if (randart_make(o_ptr) == NULL) {
				/* If not, wipe seed. No randart today */
				o_ptr->name1 = 0;
				o_ptr->name3 = 0L;

				return (FALSE);
			}

			/* Check if we can break the loop (artifact is allowed) */
			a_ptr = randart_make(o_ptr);
			if ((resf & RESF_LIFE) || !(a_ptr->flags1 & TR1_LIFE)) break;
		}

		/* after too many tries, in theory allow non-winner to find a +LIFE randart.
		   shouldn't matter much in reality though. */
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
static bool make_ego_item(int level, object_type *o_ptr, bool good, u32b resf)
{
	int i = 0, j, n;
	int *ok_ego, ok_num = 0;
	bool ret = FALSE;
	byte tval = o_ptr->tval;
#if 0 /* make_ego_item() is called BEFORE the book is set to a specific school spell! */
	bool crystal =
	    o_ptr->tval == TV_BOOK &&
	    o_ptr->sval == SV_SPELLBOOK &&
	    get_spellbook_name_colour(o_ptr->pval) == TERM_YELLOW;
#endif

	if (artifact_p(o_ptr) || o_ptr->name2) return (FALSE);

	C_MAKE(ok_ego, e_tval_size[tval], int);

	/* Grab the ok ego */
	for (i = 0, n = e_tval_size[tval]; i < n; i++) {
		ego_item_type *e_ptr = &e_info[e_tval[tval][i]];
		bool ok = FALSE;
#if 0 /* done in e_info */
                bool cursed = FALSE;
#endif

		/* Must have the correct fields */
		for (j = 0; j < MAX_EGO_BASETYPES; j++) {
			if ((e_ptr->tval[j] == o_ptr->tval) && (e_ptr->min_sval[j] <= o_ptr->sval) && (e_ptr->max_sval[j] >= o_ptr->sval)) ok = TRUE;

#if 0 /* done in e_info */
                        for (k = 0; k < 5-4; k++) if (e_ptr->flags3[k] & TR3_CURSED) cursed = TRUE;
#endif
			if (ok) break;
		}
#if 0 /* done in e_info */
                /* No curse-free ego powers on broken or rusty items; no elven filthy rags */
                if (!cursed && (o_ptr->k_idx == 30 || o_ptr->k_idx == 47 || o_ptr->k_idx == 110 || (o_ptr->k_idx == 102 && i == EGO_ELVENKIND))) ok = FALSE;
#endif
		if (!ok) {
			/* Doesnt count as a try*/
			continue;
		}

		/* Good should be good, bad should be bad */
		if (good && (!e_ptr->cost)) continue;
		if ((!good) && e_ptr->cost) continue;

		/* ok */
		ok_ego[ok_num++] = e_tval[tval][i];
	}
	
	if (!ok_num) {
		/* Fix memory leak - mikaelh */
		C_FREE(ok_ego, e_tval_size[tval], int);

		return(FALSE);
	}

	/* Now test them a few times */
//	for (i = 0; i < ok_num * 10; i++)	// I wonder.. 
	for (j = 0; j < ok_num * 10; j++) {
		ego_item_type *e_ptr;

		i = ok_ego[rand_int(ok_num)];
		e_ptr = &e_info[i];

		if (i == EGO_ETHEREAL && (resf & RESF_NOETHEREAL)) continue;

		/* XXX XXX Enforce minimum "depth" (loosely) */
		if (e_ptr->level > level) {
			/* Acquire the "out-of-depth factor" */
			int d = (e_ptr->level - level);

			/* Roll for out-of-depth creation */
			if (rand_int(d) != 0) continue;
		}

		/* We must make the "rarity roll" */
//		if (rand_int(e_ptr->mrarity - luck(-(e_ptr->mrarity / 2), e_ptr->mrarity / 2)) > e_ptr->rarity)
#if 0
		k = e_ptr->mrarity - e_ptr->rarity;
		if (k && rand_int(k)) continue;
#endif	// 0

		if (e_ptr->mrarity == 255) continue;
		if (rand_int(e_ptr->mrarity) > e_ptr->rarity) continue;
//		if (rand_int(e_ptr->rarity) < e_ptr->mrarity) continue;

		/* Hack -- mark the item as an ego */
		o_ptr->name2 = i;

		/* Piece together a 32-bit random seed */
		if (e_ptr->fego[0] & ETR4_NO_SEED) o_ptr->name3 = 0;
		else {
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
	if (ret && magik(7) && (!o_ptr->name2b) && !(resf & RESF_NODOUBLEEGO)) {
		/* Now test them a few times */
		for (j = 0; j < ok_num * 10; j++) {
			ego_item_type *e_ptr;

			i = ok_ego[rand_int(ok_num)];
			e_ptr = &e_info[i];

			if (i == EGO_ETHEREAL && (resf & RESF_NOETHEREAL)) continue;

			/* Cannot be a double ego of the same ego type */
			if (i == o_ptr->name2) continue;

			/* Cannot have 2 suffixes or 2 prefixes */
			if (e_info[o_ptr->name2].before && e_ptr->before) continue;
			if ((!e_info[o_ptr->name2].before) && (!e_ptr->before)) continue;

			/* XXX XXX Enforce minimum "depth" (loosely) */
			if (e_ptr->level > level) {
				/* Acquire the "out-of-depth factor" */
				int d = (e_ptr->level - level);

				/* Roll for out-of-depth creation */
				if (rand_int(d) != 0) continue;
			}

			/* We must make the "rarity roll" */
//			if (rand_int(e_ptr->mrarity - luck(-(e_ptr->mrarity / 2), e_ptr->mrarity / 2)) > e_ptr->rarity)
			if (e_ptr->mrarity == 255) continue;
			if (rand_int(e_ptr->mrarity) > e_ptr->rarity) continue;
//			if (rand_int(e_ptr->rarity) < e_ptr->mrarity) continue;

			/* Don't allow silyl combinations that either don't
			   make sense or that cause technical problems: */

			/* Prevent redundant resistance flags (elven armour of resist xxxx) */
			switch (o_ptr->name2) {
			case EGO_RESIST_FIRE:
				if (i == EGO_DWARVEN_ARMOR) {
					/* upgrade first ego power! */
					o_ptr->name2 = i;
					i = 0;
					break;
				}
			case EGO_RESIST_COLD:
			case EGO_RESIST_ELEC:
			case EGO_RESIST_ACID:
				if (i == EGO_ELVENKIND) {
					/* upgrade first ego power! */
					o_ptr->name2 = i;
					i = 0;
					break;
				}
				break;
			case EGO_ELVENKIND:
				switch (i) {
				case EGO_RESIST_FIRE:	case EGO_RESIST_COLD:
				case EGO_RESIST_ELEC:	case EGO_RESIST_ACID:
					continue;
				}
				break;
			case EGO_DWARVEN_ARMOR:
				switch (i) {
				case EGO_RESIST_FIRE:
					continue;
				}
				break;
			}

			/* Prevent two ego powers that can both be activated */
			switch (o_ptr->name2) {
			case EGO_CLOAK_LORDLY_RES:
				switch (i) {
				case EGO_AURA_ELEC2:
				case EGO_AURA_COLD2:
				case EGO_AURA_FIRE2:
					continue;
				}
				break;
			case EGO_AURA_ELEC2:
			case EGO_AURA_COLD2:
			case EGO_AURA_FIRE2:
				if (i == EGO_CLOAK_LORDLY_RES) {
					/* upgrade first ego power! */
					o_ptr->name2 = i;
					i = 0;
					break;
				}
				break;
			case EGO_SPECTRAL:
				switch (i) {
				case EGO_FURY:
					/* upgrade first ego power! */
					o_ptr->name2 = i;
					i = 0;
					break;
				case EGO_DRAGON: /* of the Thunderlords */
					/* upgrade first ego power! */
					o_ptr->name2 = i;
					i = 0;
					break;
				}
				break;
			case EGO_FURY:
			case EGO_DRAGON: /* of the Thunderlords */
				if (i == EGO_SPECTRAL) continue;
			}

			/* Prevent contradicting ego powers */
			switch (o_ptr->name2) {
			case EGO_CLOAK_INVIS:
				switch (i) {
				case EGO_AURA_ELEC2:
				case EGO_AURA_COLD2:
				case EGO_AURA_FIRE2:
					continue;
				}
				break;
			case EGO_AURA_ELEC2:
			case EGO_AURA_COLD2:
			case EGO_AURA_FIRE2:
				if (i == EGO_CLOAK_INVIS) {
					/* upgrade first ego power! */
					o_ptr->name2 = i;
					i = 0;
					break;
				}
				break;
			}

			/* Hack: If we upgraded the first ego power, the second one hasn't become set.
			   In that case i == 0 here. */
			if (i == 0) break;

			/* Hack -- mark the item as an ego */
			o_ptr->name2b = i;

			/* Piece together a 32-bit random seed */
			if (!(e_ptr->fego[0] & ETR4_NO_SEED) && !o_ptr->name3) {
				o_ptr->name3 = rand_int(0xFFFF) << 16;
				o_ptr->name3 += rand_int(0xFFFF);
			}
			break;
		}
	}
	C_FREE(ok_ego, e_tval_size[tval], int);

	/* Return */
	return (ret);
}


/*
 * Charge a new wand.
 */
static void charge_wand(object_type *o_ptr)
{
	switch (o_ptr->sval) {
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
	case SV_WAND_TELEPORT_TO:		o_ptr->pval = randint(3)  + 3; break;
	}
}



/*
 * Charge a new staff.
 */
static void charge_staff(object_type *o_ptr)
{
	switch (o_ptr->sval) {
	case SV_STAFF_DARKNESS:			o_ptr->pval = randint(8)  + 8; break;
	case SV_STAFF_SLOWNESS:			o_ptr->pval = randint(8)  + 8; break;
	case SV_STAFF_HASTE_MONSTERS:		o_ptr->pval = randint(8)  + 8; break;
	case SV_STAFF_SUMMONING:		o_ptr->pval = randint(3)  + 1; break;
	case SV_STAFF_TELEPORTATION:		o_ptr->pval = randint(4)  + 5; break;
	case SV_STAFF_IDENTIFY:			o_ptr->pval = randint(15) + 5; break;
	case SV_STAFF_REMOVE_CURSE:		o_ptr->pval = randint(3)  + 4; break;
	case SV_STAFF_STARLITE:			o_ptr->pval = randint(5)  + 6; break;
	case SV_STAFF_LITE:			o_ptr->pval = randint(20) + 8; break;
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
	case SV_STAFF_SLEEP_MONSTERS:		o_ptr->pval = randint(5)  + 6; break;
	case SV_STAFF_SLOW_MONSTERS:		o_ptr->pval = randint(5)  + 6; break;
	case SV_STAFF_SPEED:			o_ptr->pval = randint(3)  + 4; break;
	case SV_STAFF_PROBING:			o_ptr->pval = randint(6)  + 2; break;
	case SV_STAFF_DISPEL_EVIL:		o_ptr->pval = randint(3)  + 4; break;
	case SV_STAFF_POWER:			o_ptr->pval = randint(3)  + 1; break;
	case SV_STAFF_HOLINESS:			o_ptr->pval = randint(2)  + 2; break;
	case SV_STAFF_GENOCIDE:			o_ptr->pval = randint(2)  + 1; break;
	case SV_STAFF_EARTHQUAKES:		o_ptr->pval = randint(5)  + 3; break;
	case SV_STAFF_DESTRUCTION:		o_ptr->pval = randint(3)  + 1; break;
	case SV_STAFF_STAR_IDENTIFY:		o_ptr->pval = randint(5)  + 3; break;
	}
}



/*
 * Apply magic to an item known to be a "weapon"
 *
 * Hack -- note special base damage dice boosting
 * Hack -- note special processing for weapon/digger
 * Hack -- note special rating boost for dragon scale mail
 */
static void a_m_aux_1(object_type *o_ptr, int level, int power, u32b resf)
{
	int tohit1 = randint(5) + m_bonus(5, level);
	int todam1 = randint(5) + m_bonus(5, level);

	int tohit2 = m_bonus(10, level);
	int todam2 = m_bonus(10, level);
	//int tries;

	artifact_bias = 0;

	/* Very good */
	if (power > 1) {
		/* Make ego item */
//		if (!rand_int(RANDART_WEAPON) && (o_ptr->tval != TV_TRAPKIT)) create_artifact(o_ptr, FALSE, TRUE);	else
		make_ego_item(level, o_ptr, TRUE, resf);
	} else if (power < -1) {
		/* Make ego item */
		make_ego_item(level, o_ptr, FALSE, resf);
	}

	/* Good */
	if ((power > 0) && (o_ptr->tval != TV_MSTAFF)) {
		/* Enchant */
		o_ptr->to_h += tohit1;
		o_ptr->to_d += todam1;

		/* Very good */
		if (power > 1) {
			/* Enchant again */
			o_ptr->to_h += tohit2;
			o_ptr->to_d += todam2;
		}
	}
	/* Cursed */
	else if (power < 0) {
		/* Penalize */
		o_ptr->to_h -= tohit1;
		o_ptr->to_d -= todam1;

		/* Very cursed */
		if (power < -1) {
			/* Penalize again */
			o_ptr->to_h -= tohit2;
			o_ptr->to_d -= todam2;
		}

		/* Cursed (if "bad") */
#ifdef PREVENT_CURSED_TOOLS
		/* little hack - no cursed diggers/tools */
		if ((o_ptr->tval != TV_DIGGING && o_ptr->tval != TV_TOOL) &&
		    (o_ptr->to_h + o_ptr->to_d < 0))
#else
		if (o_ptr->to_h + o_ptr->to_d < 0)
#endif
			o_ptr->ident |= ID_CURSED;
	}


	/* Some special cases */
	switch (o_ptr->tval) {
		case TV_MSTAFF:
			break;
		case TV_BOLT:
		case TV_ARROW:
		case TV_SHOT:
			if (o_ptr->sval == SV_AMMO_MAGIC) {
				o_ptr->to_h = o_ptr->to_d = o_ptr->pval = o_ptr->name2 = o_ptr->name3 = 0;
				break;
			}

//			if ((power == 1) && !o_ptr->name2 && o_ptr->sval != SV_AMMO_MAGIC)
			else if ((power == 1) && !o_ptr->name2) {
//				if (randint(100) < 7)
				if (randint(500) < level + 5) {
					/* Exploding missile */
					int power[28] = { GF_ELEC, GF_POIS, GF_ACID,
						GF_COLD, GF_FIRE, GF_PLASMA, GF_LITE,
						GF_DARK, GF_SHARDS, GF_SOUND,
						GF_CONFUSION, GF_FORCE, GF_INERTIA,
						GF_MANA, GF_METEOR, GF_ICE, GF_CHAOS,
						GF_NETHER, GF_NEXUS, GF_TIME,
						GF_GRAVITY, GF_KILL_WALL, GF_AWAY_ALL,
						GF_TURN_ALL, GF_NUKE, GF_STUN,
						GF_DISINTEGRATE, GF_HELL_FIRE };

						//                                o_ptr->pval2 = power[rand_int(25)];
					o_ptr->pval = power[rand_int(28)];
				}
			}
			break;
		case TV_BOW:
			if (o_ptr->name2 == EGO_ACCURACY || o_ptr->name2b == EGO_ACCURACY) {
				if (o_ptr->to_h < 18) o_ptr->to_h = 18;
			}
			if (o_ptr->name2 == EGO_VELOCITY || o_ptr->name2b == EGO_VELOCITY) {
				if (o_ptr->to_d < 18) o_ptr->to_d = 18;
			}
			break;
	}

	/* CAP_ITEM_BONI */
	switch (o_ptr->tval) {
	case TV_BOLT:
	case TV_ARROW:
	case TV_SHOT:
		if (o_ptr->to_h > 15) o_ptr->to_h = 15;
		if (o_ptr->to_d > 15) o_ptr->to_d = 15;
		break;
	case TV_BOW:
	case TV_BOOMERANG:
	default:
		if (o_ptr->to_h > 30) o_ptr->to_h = 30;
		if (o_ptr->to_d > 30) o_ptr->to_d = 30;
		break;
	}
}


/*
 * Apply magic to an item known to be "armor"
 *
 * Hack -- note special processing for crown/helm
 * Hack -- note special processing for robe of permanence
 */
static void a_m_aux_2(object_type *o_ptr, int level, int power, u32b resf)
{
	int toac1 = randint(5) + m_bonus(5, level);
	int toac2 = m_bonus(10, level);

	artifact_bias = 0;

	/* Very good */
	if (power > 1) {
		/* Make ego item */
//		if (!rand_int(RANDART_ARMOR)) create_artifact(o_ptr, FALSE, TRUE);	else
		make_ego_item(level, o_ptr, TRUE, resf);
	} else if (power < -1) {
		/* Make ego item */
		make_ego_item(level, o_ptr, FALSE, resf);
	}

	/* Good */
	if (power > 0) {
		/* Enchant */
		o_ptr->to_a += toac1;

		/* Very good */
		if (power > 1) {
			/* Enchant again */
			o_ptr->to_a += toac2;
		}
	}

	/* Cursed */
	else if (power < 0) {
		/* Penalize */
		o_ptr->to_a -= toac1;

		/* Very cursed */
		if (power < -1) {
			/* Penalize again */
			o_ptr->to_a -= toac2;
		}

		/* Cursed (if "bad") */
		if (o_ptr->to_a < 0) o_ptr->ident |= ID_CURSED;
	}

#if 1	// once..
	/* Analyze type */
	switch (o_ptr->tval) {
		case TV_CLOAK:
			if (o_ptr->sval == SV_ELVEN_CLOAK) {
//experimentally changed:	o_ptr->bpval = randint(4);       /* No cursed elven cloaks...? */
				o_ptr->bpval = randint(3);       /* No cursed elven cloaks...? */
                        }
#if 1
			/* Set the Kolla cloak's base bonuses*/
			if (o_ptr->sval == SV_KOLLA) {
				o_ptr->bpval = randint(2);
			}
#endif	// 0
			break;
		case TV_DRAG_ARMOR:
			if (o_ptr->sval == SV_DRAGON_MULTIHUED) {
				int i = 2, tries = 100; /* give 2 random immunities */
				while (i && tries) {
					switch(rand_int(5)){
					case 0:if (!(o_ptr->xtra2 & 0x01)){
							o_ptr->xtra2 |= 0x01;
							i--;}
							break;
					case 1:if (!(o_ptr->xtra2 & 0x02)){
							o_ptr->xtra2 |= 0x02;
							i--;}
							break;
					case 2:if (!(o_ptr->xtra2 & 0x04)){
							o_ptr->xtra2 |= 0x04;
							i--;}
							break;
					case 3:if (!(o_ptr->xtra2 & 0x08)){
							o_ptr->xtra2 |= 0x08;
							i--;}
							break;
					case 4:if (!(o_ptr->xtra2 & 0x10)){
							o_ptr->xtra2 |= 0x10;
							i--;}
							break;
					}
					tries--;
				}
			}
			break;
		case TV_SOFT_ARMOR:
			/* Costumes */
			if (o_ptr->sval == SV_COSTUME) {
				int i, tries = 0;
				monster_race *r_ptr;

				/* Santa Claus costumes during xmas */
				if (season_xmas) {
					o_ptr->bpval = 733; /* JOKEBAND Santa Claus */
					o_ptr->level = 1;
				} else {
					/* Default to the "player" */
					o_ptr->bpval = 0;
					o_ptr->level = 1;

					while (tries++ != 1000) {
						i = randint(MAX_R_IDX - 1); /* skip 0, ie player */
						r_ptr = &r_info[i];

						if (!r_ptr->name) continue;
						// if (r_ptr->flags1 & RF1_UNIQUE) continue;
						// if (r_ptr->level >= level + (power * 5)) continue;
//						if (!mon_allowed(r_ptr)) continue;
						if (!mon_allowed_chance(r_ptr)) continue;
						if (r_ptr->rarity == 255) continue;

						break;
					}
					if (tries < 1000) {
						o_ptr->bpval = i;
						o_ptr->level = r_info[i].level / 4;
						if (o_ptr->level < 1) o_ptr->level = 1;
					}
				}
			}
			break;
		case TV_SHIELD:
			if (o_ptr->sval == SV_DRAGON_SHIELD) {
				/* pfft */
//				dragon_resist(o_ptr);
				break;
			}

#if 1
			/* Set the orcish shield's STR and CON bonus
			*/
			if(o_ptr->sval == SV_ORCISH_SHIELD) {
				o_ptr->bpval = randint(2);

				/* Cursed orcish shield */
				if (power < 0) o_ptr->bpval = -o_ptr->bpval;
				break;
			}
#endif
#if 1
		case TV_BOOTS:
			/* Set the Witan Boots stealth penalty */
			if (o_ptr->sval == SV_PAIR_OF_WITAN_BOOTS) {
				o_ptr->bpval = -2;
			}
#endif	// 0
	}
#endif	// 0

	/* CAP_ITEM_BONI */
#ifdef USE_NEW_SHIELDS  /* should actually be USE_BLOCKING, but could be too */
                        /* dramatic a change if it gets enabled temporarily - C. Blue */
	if (o_ptr->tval == TV_SHIELD) {
		if (o_ptr->to_a > 15) o_ptr->to_a = 15;
	} else
#endif
	{
//		if (o_ptr->to_a > 50) o_ptr->to_a = 50;
		if (o_ptr->to_a > 35) o_ptr->to_a = 35;
	}
}



/*
 * Apply magic to an item known to be a "ring" or "amulet"
 *
 * Hack -- note special rating boost for ring of speed
 * Hack -- note special rating boost for amulet of the magi
 * Hack -- note special "pval boost" code for ring of speed
 * Hack -- note that some items must be cursed (or blessed)
 */
static void a_m_aux_3(object_type *o_ptr, int level, int power, u32b resf)
{
	int tries = 0, i;
	artifact_bias = 0;

	/* Very good */
	if (power > 1) {
#if 0
		if (!rand_int(RANDART_JEWEL)) create_artifact(o_ptr, FALSE, TRUE);
		else
#endif
		/* Make ego item */
		make_ego_item(level, o_ptr, TRUE, resf);
	} else if (power < -1) {
		/* Make ego item */
		make_ego_item(level, o_ptr, FALSE, resf);
	}


	/* prolly something should be done..	- Jir - */
	/* Apply magic (good or bad) according to type */
	switch (o_ptr->tval) {
		case TV_RING: {
			/* Analyze */
			switch (o_ptr->sval) {
				case SV_RING_POLYMORPH:
					if (power < 1) power = 1;

					/* Be sure to be a player */
					o_ptr->pval = 0;
					o_ptr->timeout = 0;

					if (magik(45)) {
						monster_race *r_ptr;

						while (tries++ != 1000) {
							i = randint(MAX_R_IDX - 1); /* skip 0, ie player */
							r_ptr = &r_info[i];

							if (!r_ptr->name) continue;
							if (r_ptr->flags1 & RF1_UNIQUE) continue;
							if (r_ptr->level >= level + (power * 5)) continue;
//							if (!mon_allowed(r_ptr)) continue;
							if (!mon_allowed_chance(r_ptr)) continue;
							if (r_ptr->rarity == 255) continue;

							break;
						}
						if (tries < 1000) {
							o_ptr->pval = i;
							/* Let's have the following level req code commented out
							to give found poly rings random levels to allow surprises :)
							Nah my idea was too cheezy, Blue DR at 21 -C. Blue */
							if (r_info[i].level > 0) {
								//o_ptr->level = 10 + (1000 / ((2000 / r_info[i].level) + 10));
								//o_ptr->level = 5 + (1000 / ((1500 / r_info[i].level) + 5));
								/* keep consistent with cmd6.c ! */
								o_ptr->level = 5 + (1000 / ((1500 / r_info[i].level) + 7));
							} else {
								//o_ptr->level = 10;
								o_ptr->level = 5;
							}
							/* Make the ring last only over a certain period of time >:) - C. Blue */
							o_ptr->timeout = 3000 + rand_int(3001);
						} else o_ptr->level = 1;
					} else o_ptr->level = 1;
					break;

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


				case SV_RING_MIGHT:
				case SV_RING_READYWIT:
				case SV_RING_TOUGHNESS:
				case SV_RING_CUNNINGNESS:
				{
					/* Stat bonus */
					o_ptr->bpval = 1 + m_bonus(4, level); /* (5, level) for single-stat rings (traditional) */

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
					
					/* Limit */
					if (o_ptr->bpval > 15) o_ptr->bpval = 15;

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
#if 0	/* lordly pfft ring.. */
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
				case SV_RING_ELEC:
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
//					o_ptr->to_h = 5 + randint(8) + m_bonus(10, level);
					o_ptr->to_h = 10 + rand_int(11) + m_bonus(5, level);

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
					o_ptr->to_h = 3 + randint(6) + m_bonus(10, level);
					o_ptr->to_d = 3 + randint(5) + m_bonus(9, level);

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

				/* Amulet of the Magi -- never cursed */
				case SV_AMULET_THE_MAGI:
//					if (randint(3)==1) o_ptr->art_flags3 |= TR3_SLOW_DIGEST;
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

				case SV_AMULET_NO_MAGIC:
					/* Never cursed - C. Blue */
					break;
				case SV_AMULET_NO_TELE:
				{
					if (power < 0) o_ptr->ident |= (ID_CURSED);
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
					o_ptr->bpval = 1 + m_bonus(2, level);
					o_ptr->to_a = -1 - m_bonus(13, level);
					o_ptr->to_h = -1 - m_bonus(10, level);
					o_ptr->to_d = 1 + m_bonus(8, level);//was 15,..
					if (rand_int(100) < 33) {
//	                                        o_ptr->xtra1 = EGO_XTRA_POWER;
//    		                                o_ptr->xtra2 = rand_int(255);
					}
					break;
				}

				/* Amulet of speed */
				case SV_AMULET_SPEED:
				{
//					o_ptr->bpval = randint(5);1/2*1/4

/* chances:
					o_ptr->bpval = rand_int(4) + randint(randint(2));
    +1: 1/2*1/4 + 1/2*1/2*1/4	= 3/16
    +2: 1/2*1/2*1/4 + 1/2*1/2*1/4 + 1/2*1/4	= 4/16
    +3: 1/2*1/2*1/4 + 1/2*1/2*1/4 + 1/2*1/4	= 4/16
    +4: 1/2*1/2*1/4 + 1/2*1/2*1/4 + 1/2*1/4	= 4/16
    +5: 1/2*1/2*1/4	= 1/16
*/

/* chances:
					o_ptr->bpval = rand_int(3) + randint(3);
    +1: 1/3*1/3 = 1/9
    +2: 1/3*1/3 + 1/3*1/3 = 2/9
    +3: 1/3*1/3 + 1/3*1/3 + 1/3*1/3 = 3/9
    +4: 1/3*1/3 + 1/3*1/3 = 2/9
    +5: 1/3*1/3 = 1/9
*/

					o_ptr->bpval = randint(3 + randint(2));
/* chances:
    +1: 1/2*1/4 + 1/2*1/5 = 9/40
    +2: 1/2*1/4 + 1/2*1/5 = 9/40
    +3: 1/2*1/4 + 1/2*1/5 = 9/40
    +4: 1/2*1/4 + 1/2*1/5 = 9/40
    +5: 1/2*1/5 = 4/40
*/

					/* Cursed */
					if (power < 0) {
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
			                make_ego_item(level, o_ptr, TRUE, resf);
					break;
				}

				/* Talisman (Amulet of Luck) */
				case SV_AMULET_LUCK:
				{
					o_ptr->bpval = magik(40)?randint(3):(magik(40)?randint(4):randint(5));

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
				case SV_AMULET_REFLECTION:
					o_ptr->to_a = 5 + rand_int(11);

					/* Cursed */
					if (power < 0) {
						/* Broken */
						o_ptr->ident |= ID_BROKEN;
						/* Cursed */
						o_ptr->ident |= ID_CURSED;
						/* Reverse bonuses */
						o_ptr->to_a = -o_ptr->to_a;
					}
					break;
			}

			break;
		}
	}
}


/*
 * Apply magic to an item known to be "boring"
 *
 * Hack -- note the special code for various items
 */
static void a_m_aux_4(object_type *o_ptr, int level, int power, u32b resf)
{
        u32b f1, f2, f3, f4, f5, esp;

        /* Very good */
        if (power > 1)
        {
                /* Make ego item */
//                if (!rand_int(RANDART_JEWEL) && (o_ptr->tval == TV_LITE)) create_artifact(o_ptr, FALSE, TRUE);	else
                make_ego_item(level, o_ptr, TRUE, resf);
        }
        else if (power < -1)
        {
                /* Make ego item */
                make_ego_item(level, o_ptr, FALSE, resf);
        }

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	/* Apply magic (good or bad) according to type */
	switch (o_ptr->tval)
	{
		case TV_BOOK:
		{
			/* Randomize random books */
			if (o_ptr->sval == SV_SPELLBOOK)
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
 * 
 * Added "true_art" to disallow true artifacts in case a king/queen kills a
 * monster, they cannot carry true artifacts anyways (but they would usually
 * find heaps of them..) - C. Blue
 *
 * "verygreat" makes sure that ego items aren't just resist fire etc.
 * Has no influence on artifacts. - C. Blue
 */
void apply_magic(struct worldpos *wpos, object_type *o_ptr, int lev, bool okay, bool good, bool great, bool verygreat, u32b resf)
{
	/* usually lev = dungeonlevel (sometimes more, if in vault) */
	object_type forge_bak, forge_highest, forge_lowest;
	object_type *o_ptr_bak = &forge_bak, *o_ptr_highest = &forge_highest;
	object_type *o_ptr_lowest = &forge_lowest;
	bool resf_fallback = TRUE;
	s32b ego_value1, ego_value2, ovr, fc;
	long depth = ABS(getlevel(wpos)), depth_value;
	int i, rolls, chance1, chance2, power; //, j;
        char o_name[ONAME_LEN];
	u32b f1, f2, f3, f4, f5, esp; /* for RESF checks */

	/* Fix for reasonable level reqs on DROP_CHOSEN/SPECIAL_GENE items -C. Blue */
	if (lev == -2) lev = getlevel(wpos);

	/* Maximum "level" for various things */
        if (lev > MAX_DEPTH_OBJ - 1) lev = MAX_DEPTH_OBJ - 1;


	/* Base chance of being "good" */
	/* Hack: Way too many fire/waterproof books in high level towns! */
	if (o_ptr->tval == TV_BOOK) chance1 = 10;
	else chance1 = lev + 10;

	/* Maximal chance of being "good" */
	if (chance1 > 75) chance1 = 75;

	/* Base chance of being "great" */
	chance2 = chance1 / 2;

	/* Maximal chance of being "great" */
	if (chance2 > 20) chance2 = 20;


	/* Assume normal */
	power = 0;

	/* Roll for "good" */
	if (good || magik(chance1)) {
		/* Assume "good" */
		power = 1;

		/* Higher chance2 for super heavy armours are already very rare */
		if (k_info[o_ptr->k_idx].flags5 & TR5_WINNERS_ONLY) chance2 += 10;

		/* Roll for "great" */
		if (great || magik(chance2)) power = 2;
	}

	/* Roll for "cursed" */
	else if (magik(chance1)) {
		/* Assume "cursed" */
		power = -1;

		/* Roll for "broken" */
		if (magik(chance2)) power = -2;
	}


	/* Assume no rolls */
	rolls = 0;

	/* Get one roll if excellent */
	if (power >= 2) rolls = 1;

	/* Hack -- Get four rolls if forced great */
	if (great) rolls = 2; // 4

	/* Hack -- Get no rolls if not allowed */
	if (!okay || o_ptr->name1) rolls = 0;


	/* Hack for possible randarts, to be created in next for loop:
	   Jewelry can keep +hit,+dam,+ac through artifying process!
	   That means, it must be applied beforehand already, because the
	   o_ptr->name1 check below will exit apply_magic() via return().
	   Won't affect normal items that fail randart check anyway. ----------------------- */
	/* Allow mods on non-artified randart jewelry! */
	if (o_ptr->tval == TV_RING || o_ptr->tval == TV_AMULET) {
		/* hack: if we apply_magic() again on _already created_ jewelry,
		   we don't want to reset its hit/dam/ac, because we're also called
		   when we create an artifact out of an item. */
		if (!o_ptr->name1) {
			if (!power && (rand_int(100) < CURSED_JEWELRY_CHANCE)) power = -1;
			a_m_aux_3(o_ptr, lev, power, resf);
		}
		o_ptr->name2 = o_ptr->name2b = 0; /* required? */
	}
	/* --------------------------------------------------------------------------------- */

	/* Roll for artifacts if allowed */
	for (i = 0; i < rolls; i++) {
		/* Roll for an artifact */
		if (make_artifact(wpos, o_ptr, resf)) break;
	}

	/* virgin */
	o_ptr->owner = 0;

	/* Hack -- analyze artifacts */
	if (o_ptr->name1) {
		artifact_type *a_ptr;

		/* Randart */
		if (o_ptr->name1 == ART_RANDART) {
			/* generate it finally, after those preparations above */
			a_ptr = randart_make(o_ptr);
		}
		/* Normal artifacts */
		else {
			a_ptr = &a_info[o_ptr->name1];
		}

		/* ?catch impossible randart types? */
		if (a_ptr == (artifact_type*)NULL) {
			o_ptr->name1 = 0;
			s_printf("RANDART_FAIL in apply_magic().\n");
			return;
		}

		/* determine level-requirement */
		determine_level_req(lev, o_ptr);

		/* Override level requirements? */
		if ((o_ptr->name1 == ART_RANDART) &&
		    (cfg.arts_level_req >= 3))
			o_ptr->level = 0;
		else if ((a_ptr->flags4 & TR4_SPECIAL_GENE) &&
		    (cfg.arts_level_req >= 1))
			o_ptr->level = 0;
		else if (cfg.arts_level_req >= 2)
			o_ptr->level = 0;

		/* Hack -- Mark the artifact as "created" */
		handle_art_inumpara(o_ptr->name1);

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

		/* clear flags from pre-artified item, simulating
		   generation of a brand new object. */
		o_ptr->ident &= ~(ID_MENTAL | ID_BROKEN | ID_CURSED);

		/* Hack -- extract the "broken" flag */
		if (!a_ptr->cost) o_ptr->ident |= ID_BROKEN;

		/* Hack -- extract the "cursed" flag */
		if (a_ptr->flags3 & TR3_CURSED) o_ptr->ident |= ID_CURSED;

		/* Done */
		return;
	}


	/* In case we get an ego item, check "verygreat" flag and retry a few times if needed */
if (verygreat) s_printf("verygreat apply_magic:\n");
	object_copy(o_ptr_bak, o_ptr);
	object_copy(o_ptr_highest, o_ptr);
	depth_value = (depth < 60 ? depth * 150 : 9000) + randint(depth) * 100;
//  for (i = 0; i < (!is_ammo(o_ptr->tval) ? 2 + depth / 7 : 4 + depth / 5); i++) {
//  for (i = 0; i < (!is_ammo(o_ptr->tval) ? 2 + depth / 5 : 4 + depth / 5); i++) {
for (i = 0; i < 25; i++) {
	object_copy(o_ptr, o_ptr_bak);


	/* Apply magic */
	switch (o_ptr->tval) {
		case TV_DIGGING:
		case TV_BLUNT:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_BOW:
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_BOOMERANG:
		case TV_AXE:
		case TV_MSTAFF:
			if (power) a_m_aux_1(o_ptr, lev, power, resf);
			break;

		case TV_DRAG_ARMOR:
		case TV_HARD_ARMOR:
		case TV_SOFT_ARMOR:
		case TV_SHIELD:
		case TV_HELM:
		case TV_CROWN:
		case TV_CLOAK:
		case TV_GLOVES:
		case TV_BOOTS:
			// Power is no longer required since things such as
			// Kollas need magic applied to finish their normal
			// generation.
			//if (power) a_m_aux_2(o_ptr, lev, power, resf);
			a_m_aux_2(o_ptr, lev, power, resf);
			break;

		case TV_RING:
		case TV_AMULET:
			if (!power && (rand_int(100) < CURSED_JEWELRY_CHANCE)) power = -1;
			a_m_aux_3(o_ptr, lev, power, resf);
			break;

		case TV_CHEST:
	                /* Traps (placed in a_m_aux_4) won't be placed on a level 0 object */
	                determine_level_req(lev, o_ptr); /* usually lev == dungeonlevel (+ x for vaults) */
			a_m_aux_4(o_ptr, lev, power, resf);
			/* that's it already */
			return;

		default:
			a_m_aux_4(o_ptr, lev, power, resf);
			break;
	}

	/* Bad hack: Un-ego mindcrafter spell scrolls if they got fireproof/waterproof ego,
	   since they (by another bad hack) already ignore those. */
	if (o_ptr->tval == TV_BOOK && o_ptr->sval == SV_SPELLBOOK &&
	    get_spellbook_name_colour(o_ptr->pval) == TERM_YELLOW) {
	    if (o_ptr->name2 == EGO_FIREPROOF_BOOK || o_ptr->name2 == EGO_WATERPROOF_BOOK) o_ptr->name2 = 0;
	    if (o_ptr->name2b == EGO_FIREPROOF_BOOK || o_ptr->name2b == EGO_WATERPROOF_BOOK) o_ptr->name2b = 0;
	}

#if 1	// tweaked pernA ego.. 
	/* Hack -- analyze ego-items */
//	else if (o_ptr->name2)
	if (o_ptr->name2 && !o_ptr->name1) {
		artifact_type *a_ptr;

		a_ptr = ego_make(o_ptr);

		/* Extract the other fields */

		if ((o_ptr->tval == TV_RING && o_ptr->sval == SV_RING_POLYMORPH)
		    || o_ptr->tval == TV_BOOK
		    || is_ammo(o_ptr->tval))
			; /* keep o_ptr->pval! */
		else if (!is_magic_device(o_ptr->tval)) /* don't kill charges on EGO (of plenty) devices! */
			o_ptr->pval = a_ptr->pval; /* paranoia?-> pval might've been limited in ego_make(), so set it here, instead of adding it */
		else
			o_ptr->pval += a_ptr->pval;

		o_ptr->ac += a_ptr->ac;
		o_ptr->dd += a_ptr->dd;
		o_ptr->ds += a_ptr->ds;
		o_ptr->to_a += a_ptr->to_a;
		o_ptr->to_h += a_ptr->to_h;
		o_ptr->to_d += a_ptr->to_d;

		/* Reduce enchantment boni for ego Dark Swords - C. Blue
		   (since they're no more (dis)enchantable, make work easier for unbelievers..) */
		if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DARK_SWORD)) {
			/* Don't reduce negative boni, of *Defender*s for example */
			if ((o_ptr->to_h > 0) && (o_ptr-> to_d > 0)) {
				o_ptr->to_h /= 2;
				o_ptr->to_d /= 2;
			}
		}

		/* Hack -- acquire "cursed" flag */
		//                if (f3 & TR3_CURSED) o_ptr->ident |= (ID_CURSED);	// this should be done here!
		if (a_ptr->flags3 & TR3_CURSED) o_ptr->ident |= (ID_CURSED);
	}
#endif	// 1

	/* Hack: determine level-requirement - here AGAIN because ego-item
	   routine wasnt called before we called det_l_r the first time */
	determine_level_req(lev, o_ptr);

	/* Examine real objects */
	if (o_ptr->k_idx) {
		object_kind *k_ptr = &k_info[o_ptr->k_idx];

		/* Hack -- acquire "broken" flag */
		if (!k_ptr->cost) o_ptr->ident |= ID_BROKEN;

		/* Hack -- acquire "cursed" flag */
		if (k_ptr->flags3 & TR3_CURSED) o_ptr->ident |= ID_CURSED;
	}

	/* Pick the lowest value item */
	if (i == 0) {
		object_copy(o_ptr_lowest, o_ptr);
	} else {
		if (object_value_real(0, o_ptr) < object_value_real(0, o_ptr_lowest)) {
			object_copy(o_ptr_lowest, o_ptr);
		}
	}

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
	if ((resf & RESF_LOWVALUE) && (object_value_real(0, o_ptr) > 35000)) continue;
	if ((resf & RESF_MIDVALUE) && (object_value_real(0, o_ptr) > 50000)) continue;
	if ((resf & RESF_NOHIVALUE) && (object_value_real(0, o_ptr) > 100000)) continue;
	if ((resf & RESF_LOWSPEED) && (f1 & TR1_SPEED) && (o_ptr->bpval > 4 || o_ptr->pval > 4)) continue;
	if ((resf & RESF_NOHISPEED) && (f1 & TR1_SPEED) && (o_ptr->bpval > 6 || o_ptr->pval > 6)) continue;

	/* "verygreat" check: */
	/* 2000 to exclude res, light, reg, etc */
	/* 5000+objval to exclude brands/slays */
//NO:	if (!verygreat || object_value_real(0, o_ptr) >= 7000) break; <- arrows (+36,+42) -> lol. - C. Blue
	if (!verygreat) break;
	if (o_ptr->name2) ego_value1 = e_info[o_ptr->name2].cost; else ego_value1 = 0;
	if (o_ptr->name2b) ego_value2 = e_info[o_ptr->name2b].cost; else ego_value2 = 0;

        object_desc(0, o_name, o_ptr, FALSE, 3);
	ovr = object_value_real(0, o_ptr);
	fc = flag_cost(o_ptr, o_ptr->pval);

	/* remember most expensive object we rolled, in case we don't find any better we can fallback to it */
	if (ovr > object_value_real(0, o_ptr_highest)) {
		object_copy(o_ptr_highest, o_ptr);

		/* No fallback because of resf necessary */
		resf_fallback = FALSE;
	}
	else
		continue;

	s_printf("dpt %d, dptval %d, egoval %d / %d, realval %d, flags %d (%s)\n", 
		depth, depth_value, ego_value1, ego_value2, ovr, fc, o_name);

	if (!is_ammo(o_ptr->tval)) {
		if ((ego_value1 >= depth_value) || (ego_value2 >= depth_value) ||
		    (object_value_real(0, o_ptr) >= depth * 300)) break;
	} else {
		/* Ammo amount is increased in place_object */
		if (object_value_real(0, o_ptr) >= depth + 150) break;
	}	
    } /* verygreat-loop end */

	if (verygreat) {
		if (resf_fallback) {
			/* Fallback to lowest value item in case resf proved too strict - mikaelh */
			s_printf("lowest value fallback used in apply_magic (resf = %#x)\n", resf);

			object_copy(o_ptr, o_ptr_lowest);

			if (o_ptr->name2) ego_value1 = e_info[o_ptr->name2].cost; else ego_value1 = 0;
			if (o_ptr->name2b) ego_value2 = e_info[o_ptr->name2b].cost; else ego_value2 = 0;

			object_desc(0, o_name, o_ptr, FALSE, 3);
			ovr = object_value_real(0, o_ptr);
			fc = flag_cost(o_ptr, o_ptr->pval);

			/* Dump information about the item */
			s_printf("dpt %d, dptval %d, egoval %d / %d, realval %d, flags %d (%s)\n", 
			depth, depth_value, ego_value1, ego_value2, ovr, fc, o_name);

			s_printf("taken\n");
		}
		else {
		        s_printf("taken\n");
			object_copy(o_ptr, o_ptr_highest);
		}
	}
}

/*
 * This 'utter hack' function is to allow item-generation w/o specifing
 * worldpos.
 */
void apply_magic_depth(int Depth, object_type *o_ptr, int lev, bool okay, bool good, bool great, bool verygreat, u32b resf)
{
	worldpos wpos;

	/* CHANGEME */
	wpos.wx = cfg.town_x;
	wpos.wy = cfg.town_y;
	wpos.wz = Depth > 0 ? 0 - Depth : Depth;
	apply_magic(&wpos, o_ptr, lev, okay, good, great, verygreat, resf);
}


/*
 * determine level requirement.
 * based on C.Blue's idea.	- Jir -
 */
//#ifndef TEST_SERVER /* cloned the function below, for reworking - C. Blue */
#if 1
void determine_level_req(int level, object_type *o_ptr)
{
	int i, j, klev = k_info[o_ptr->k_idx].level, base;
	artifact_type *a_ptr = NULL;
	base = klev / 2;

	/* Exception */
	if ((o_ptr->tval == TV_RING && o_ptr->sval == SV_RING_POLYMORPH) ||
		(o_ptr->tval == TV_SOFT_ARMOR && o_ptr->sval == SV_COSTUME)) return;

	/* Unowned yet */
//	o_ptr->owner = 0;

	/* artifact */
	if (o_ptr->name1) {
	 	/* Randart */
		if (o_ptr->name1 == ART_RANDART) {
			a_ptr = randart_make(o_ptr);
			if (a_ptr == (artifact_type*)NULL){
				o_ptr->name1 = 0;
				o_ptr->level = 0;
				return;
			}

			/* level of randarts tends to be outrageous */
			base = a_ptr->level / 1;/* was 2*/
		}
		/* Normal artifacts */
		else {
			a_ptr = &a_info[o_ptr->name1];
			base = a_ptr->level;
			base += 30; /*general increase for artifacts! */
		}
	}


	if (o_ptr->tval == TV_CHEST) {
		o_ptr->level = base + (level * 2) / 4;	/* chest level is base for calculating the item level,
							so it must be like a dungeon level - C. Blue
							(base = k_info level / 2, level = dungeonlevel usually) */
		return;
	}
	
	/* stat/heal potions harder to cheeze-transfer */
	if (o_ptr->tval == TV_POTION) {
		switch(o_ptr->sval) {
		case SV_POTION_HEALING:
			base += 15+10;
			break;
		case SV_POTION_RESTORE_MANA:
			base += 10+10;
			break;
		case SV_POTION_INC_STR:
		case SV_POTION_INC_INT:
		case SV_POTION_INC_WIS:
		case SV_POTION_INC_DEX:
		case SV_POTION_INC_CON:
		case SV_POTION_INC_CHR:
			base += 40+30;
			break;
		case SV_POTION_AUGMENTATION:
			base += 45+20;
			break;
		case SV_POTION_EXPERIENCE:
			base += 20+20;
			break;
		}
	}
	/* Certain items harder to cheeze-transfer */
	if ((o_ptr->tval == TV_RING) && (o_ptr->bpval > 0)) {
		switch(o_ptr->sval) {
		case SV_RING_CRIT:
		case SV_RING_SPEED:
			base += o_ptr->bpval * 2;
			break;
		case SV_RING_ATTACKS:
			base += o_ptr->bpval * 5;
			break;
		case SV_RING_MIGHT:
		case SV_RING_READYWIT:
		case SV_RING_TOUGHNESS:
		case SV_RING_CUNNINGNESS:
			base += o_ptr->bpval * 9;
			break;
		}
	}

	/* jewelry shop has too low levels on powerful amulets */
	if (o_ptr->tval == TV_AMULET) {
		switch (o_ptr->sval) {
		case SV_AMULET_SPEED:
			base += 20 + o_ptr->bpval * 2;
			break;
		case SV_AMULET_TRICKERY:
			base += 16 + o_ptr->bpval * 3;
			break;
		}
	}

	if (o_ptr->tval == TV_DRAG_ARMOR) {
		switch(o_ptr->sval) {
		case SV_DRAGON_MULTIHUED:
		case SV_DRAGON_SHINING:
		case SV_DRAGON_DEATH:
			base += 20;
			break;
		case SV_DRAGON_POWER:
			base += 20;
			break;
		default:
			base += 20;//was 5, but then chaos dsm had level 28
		}
	}

	if (o_ptr->tval == TV_LITE) {
		switch (o_ptr->sval) {
		case SV_LITE_DWARVEN: base += 35; break;
		case SV_LITE_FEANORIAN: base += 55; break;
		default: if (o_ptr->name2) base += 20;
		}
	}

	/* Hack -- analyze ego-items */
	if (o_ptr->name2 || o_ptr->name2b) {
		if (o_ptr->name2) base += e_info[o_ptr->name2].rating;
		if (o_ptr->name2b) base += e_info[o_ptr->name2b].rating;
		/* general level boost for ego items!
		   except very basic ones */
		switch (o_ptr->name2) {
		case EGO_LEPROUS:
		case EGO_STUPIDITY:	case EGO_NAIVETY:
		case EGO_UGLINESS:	case EGO_SICKLINESS:
		case EGO_ENVELOPING:	case EGO_VULNERABILITY:
		case EGO_IRRITATION:
		case EGO_WEAKNESS:	case EGO_CLUMSINESS:
		case EGO_PEACE:
		case EGO_NOISE:		case EGO_SLOWNESS:
		case EGO_ANNOYANCE:
		case EGO_MORGUL:	case EGO_NOTHINGNESS:
		case EGO_BACKBITING:	case EGO_SHATTERED:
		case EGO_BLASTED:
		case EGO_LFADING:
		case EGO_INDESTRUCTIBLE:case EGO_CURSED:
		case EGO_FIREPROOF:	case EGO_WATERPROOF:
		case EGO_FIREPROOF_BOOK:case EGO_WATERPROOF_BOOK:
		case EGO_PLENTY:
		case EGO_TOBVIOUS:
		case EGO_VULNERABILITY2:case EGO_VULNERABILITY3:
		case EGO_BUDWEISER:	case EGO_HEINEKEN:
		case EGO_GUINNESS:
			break;

		case EGO_RESIST_ACID:	case EGO_RESIST_ELEC:
		case EGO_RESIST_FIRE:	case EGO_RESIST_COLD:
		case EGO_ENDURE_ACID:	case EGO_ENDURE_ELEC:
		case EGO_ENDURE_FIRE:	case EGO_ENDURE_COLD:
		case EGO_NOLDOR:	case EGO_WISDOM:
		case EGO_BEAUTY:	case EGO_INFRAVISION:
		case EGO_REGENERATION:	case EGO_TELEPORTATION:
		case EGO_PROTECTION:	case EGO_STEALTH:
		case EGO_CHARMING:
		case EGO_SLOW_DESCENT:	case EGO_QUIET:
		case EGO_DIGGING:
		case EGO_RQUICKNESS:	case EGO_RCHARGING:
		case EGO_LBOLDNESS:	case EGO_LBRIGHTNESS:
		case EGO_LSTAR_BRIGHTNESS:	case EGO_LINFRAVISION:
		case EGO_RSIMPLICITY:
		case EGO_CONCENTRATION:
			base += 5;
			break;

		case EGO_FREE_ACTION:
		case EGO_SLAYING:	case EGO_AGILITY:
		case EGO_MOTION:
		case EGO_RISTARI:
		case EGO_AURA_COLD2:	case EGO_AURA_FIRE2:	case EGO_AURA_ELEC2:
			base += 10;
			break;

		case EGO_KILL_EVIL:
		case EGO_HA: /* 'Aman' */
		case EGO_GONDOLIN:
			base += 30;
			break;

		case EGO_ELVENKIND:
			base += o_ptr->bpval * 2 + 15;
			break;
		case EGO_SPEED:
			base += o_ptr->bpval * 2 + 15;
			break;
		case EGO_TELEPATHY:
			base += 25;
			break;

		case EGO_IMMUNE:
			/* only occurs on mithril/adamantite plate, which is already quite
			   high level -> need reduction (usually like level 56 without it) */
			base -= 10;
			break;

		default:
			base += 15;
		}
	}

	/* '17/72' == 0.2361... < 1/4 :) */
	base >>= 1;

#if 0 /* would need rework in conjunction with ego powers I'm afraid - C. Blue ;/ */
	/* increase plain items' levels -> no more level 21 red dsm or level 18 thunder axe! - C. Blue */
	if (klev <= 40) base += (klev / 10);
	else if (klev <= 50) base += (klev / 6);
	else if (klev <= 60) base += (klev / 7);
	else if (klev <= 70) base += (klev / 6);
	else if (klev <= 80) base += (klev / 5);
	else if (klev <= 90) base += (klev / 4);
	else base += (klev / 3);
#endif

	/* Hack: level -9999 means: use unmodified base value and don't use randomizer: */
	if (level == -9999) {
		i = 0;
		j = base;
	} else {
		i = level - base;
		j = (((i * (i > 0 ? 2 : 2)) / 12  + base) * rand_range(95,105)) / 100;/* was 1:2 / 4 */
	}

	/* Level must be between 1 and 100 inclusively */
	o_ptr->level = (j < 100) ? ((j > 1) ? j : 1) : 100;

	/* Anti-cheeze hacks */
	if ((o_ptr->tval == TV_POTION) && ( /* potions that mustn't be transferred, otherwise resulting in 1 out-of-line char */
	    (o_ptr->sval == SV_POTION_EXPERIENCE) ||
	    (o_ptr->sval == SV_POTION_LEARNING) ||
	    (o_ptr->sval == SV_POTION_INVULNERABILITY))) o_ptr->level = 0;
	if ((o_ptr->tval == TV_SCROLL) && (o_ptr->sval == SV_SCROLL_TRAP_CREATION) && (o_ptr->level < 20)) o_ptr->level = 20;
	if ((o_ptr->tval == TV_SCROLL) && (o_ptr->sval == SV_SCROLL_FIRE) && (o_ptr->level < 30)) o_ptr->level = 30;
	if ((o_ptr->tval == TV_SCROLL) && (o_ptr->sval == SV_SCROLL_ICE) && (o_ptr->level < 30)) o_ptr->level = 30;
	if ((o_ptr->tval == TV_SCROLL) && (o_ptr->sval == SV_SCROLL_CHAOS) && (o_ptr->level < 30)) o_ptr->level = 30;
	if (o_ptr->tval == TV_RING && o_ptr->sval == SV_RING_SPEED && (o_ptr->level < 30 + o_ptr->bpval - 1) && (o_ptr->bpval > 0))
		o_ptr->level = 30 + o_ptr->bpval - 1 + rand_int(3);
	if ((o_ptr->tval == TV_DRAG_ARMOR) && (o_ptr->sval == SV_DRAGON_POWER) && (o_ptr->level < 45)) o_ptr->level = 44 + randint(5);

	if (o_ptr->tval == TV_LITE) {
		switch (o_ptr->sval) {
		case SV_LITE_DWARVEN: if (o_ptr->level < 20) o_ptr->level = 20; break;
		case SV_LITE_FEANORIAN: if (o_ptr->level < 32) o_ptr->level = 32; break;
		}
	}

#if 0 /* done above instead, where EGO_ are tested */
	/* Reduce outrageous ego item levels (double-ego adamantite of immunity for example */
	if (o_ptr->name2) {
		if (o_ptr->level > 51) o_ptr->level = 48 + rand_int(4);
		else if (o_ptr->level > 48) o_ptr->level = 48 + rand_int(2);
	}
//	else {
#endif
	/* Slightly reduce high levels */
	if (o_ptr->level > 55) o_ptr->level--;
	if (o_ptr->level > 50) o_ptr->level--;
	if (o_ptr->level > 45) o_ptr->level--;
	if (o_ptr->level > 40) o_ptr->level--;

	/* tone down deep randarts a bit to allow winner-trading */
	if (o_ptr->name1 == ART_RANDART) {
		if (o_ptr->level > 51) o_ptr->level = 51 + ((o_ptr->level - 51) / 3);
	}

	/* tone down deep winners_only items to allow winner-trading */
	if (k_info[o_ptr->k_idx].flags5 & TR5_WINNERS_ONLY) {
		if (o_ptr->level > 51) o_ptr->level -= ((o_ptr->level - 51) / 2);
	}

	/* Special limit for +LIFE randarts */
	if ((o_ptr->name1 == ART_RANDART) &&
	    (a_ptr->flags1 & TR1_LIFE) && (o_ptr->level <= 50))
		o_ptr->level = 51 + rand_int(2);

	/* Special limit for WINNERS_ONLY items */
	if ((k_info[o_ptr->k_idx].flags5 & TR5_WINNERS_ONLY) && (o_ptr->level <= 50))
		o_ptr->level = 51 + rand_int(5);

	/* Fix cheap but high +dam weaponry: */
//	if ((o_ptr->to_d * 12) / 10) { /* provided it's greater 0 - PARANOIA */
		if (o_ptr->level * 10 < o_ptr->to_d * 12) o_ptr->level = (o_ptr->to_d * 12) / 10;
//	}
}
#else /* new way, quite reworked */
void determine_level_req(int level, object_type *o_ptr)
{
}
#endif

/* Purpose: fix old items' level requirements.
   What it does: Check current level against theoretical minimum level of
   that item according to current determine_level_req() values.
   Fix if below, by applying bottom cap value. */
void verify_level_req(object_type *o_ptr)
{
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
static int kind_is_theme(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	int p = 0;

	/*
	 * Paranoia -- Prevent accidental "(Nothing)"
	 * that are results of uninitialised theme structs.
	 *
	 * Caution: Junks go into the allocation table.
	 */
	if (match_theme.treasure + match_theme.combat +
	    match_theme.magic + match_theme.tools == 0) return (TRUE);


	/* Pick probability to use */
	switch (k_ptr->tval) {
		case TV_SKELETON:
		case TV_BOTTLE:
		case TV_JUNK:
		case TV_FIRESTONE:
		case TV_CORPSE:
		case TV_EGG:
		/* hm, this was missing here, for a long time - C. Blue */
		case TV_GOLEM:
			/*
			 * Degree of junk is defined in terms of the other
			 * 4 quantities XXX XXX XXX
			 * The type of prob should be *signed* as well as
			 * larger than theme components, or we would see
			 * unexpected, well, junks.
			 */
			p = 100 - (match_theme.treasure + match_theme.combat +
			              match_theme.magic + match_theme.tools);
			break;
		case TV_CHEST:          p = match_theme.treasure; break;
		case TV_CROWN:          p = match_theme.treasure; break;
		case TV_DRAG_ARMOR:     p = match_theme.treasure; break;
		case TV_AMULET:         p = match_theme.treasure; break;
		case TV_RING:           p = match_theme.treasure; break;

		case TV_SHOT:           p = match_theme.combat; break;
		case TV_ARROW:          p = match_theme.combat; break;
		case TV_BOLT:           p = match_theme.combat; break;
		case TV_BOOMERANG:      p = match_theme.combat; break;
		case TV_BOW:            p = match_theme.combat; break;
		case TV_BLUNT:          p = match_theme.combat; break;
		case TV_POLEARM:        p = match_theme.combat; break;
		case TV_SWORD:          p = match_theme.combat; break;
		case TV_AXE:            p = match_theme.combat; break;
		case TV_GLOVES:         p = match_theme.combat; break;
		case TV_HELM:           p = match_theme.combat; break;
		case TV_SHIELD:         p = match_theme.combat; break;
		case TV_SOFT_ARMOR:     p = match_theme.combat; break;
		case TV_HARD_ARMOR:     p = match_theme.combat; break;

		case TV_MSTAFF:         p = match_theme.magic; break;
		case TV_STAFF:          p = match_theme.magic; break;
		case TV_WAND:           p = match_theme.magic; break;
		case TV_ROD:            p = match_theme.magic; break;
		case TV_ROD_MAIN:       p = match_theme.magic; break;
		case TV_SCROLL:         p = match_theme.magic; break;
		case TV_PARCHMENT:      p = match_theme.magic; break;
		case TV_POTION:         p = match_theme.magic; break;
		case TV_POTION2:        p = match_theme.magic; break;

		case TV_RUNE1:          p = match_theme.magic; break;
		case TV_RUNE2:          p = match_theme.magic; break;
#if 0
		case TV_BATERIE:        p = match_theme.magic; break;
		case TV_RANDART:        p = match_theme.magic; break;
		case TV_BOOK:           p = match_theme.magic; break;
		case TV_SYMBIOTIC_BOOK: p = match_theme.magic; break;
		case TV_MUSIC_BOOK:     p = match_theme.magic; break;
		case TV_DRUID_BOOK:     p = match_theme.magic; break;
		case TV_DAEMON_BOOK:    p = match_theme.magic; break;
#endif	// 0
		case TV_BOOK:           p = match_theme.magic; break;
		case TV_LITE:           p = match_theme.tools; break;
		case TV_CLOAK:          p = match_theme.tools; break;
		case TV_BOOTS:          p = match_theme.tools; break;
		case TV_SPIKE:          p = match_theme.tools; break;
		case TV_DIGGING:        p = match_theme.tools; break;
		case TV_FLASK:          p = match_theme.tools; break;
		case TV_FOOD:           p = match_theme.tools; break;
		case TV_TOOL:           p = match_theme.tools; break;
		case TV_INSTRUMENT:     p = match_theme.tools; break;
		case TV_TRAPKIT:        p = match_theme.tools; break;
	}

	/* Return the percentage */
	return p;
}

/*
 * Determine if an object must not be generated.
 */
int kind_is_legal_special = -1;
int kind_is_legal(int k_idx, u32b resf)
{
	object_kind *k_ptr = &k_info[k_idx];

	int p = kind_is_theme(k_idx);

	/* Used only for the Nazgul rings */
	if ((k_ptr->tval == TV_RING) && (k_ptr->sval == SV_RING_SPECIAL)) p = 0;

	/* Are we forced to one tval ? */
	if ((kind_is_legal_special != -1) && (kind_is_legal_special != k_ptr->tval)) p = 0;

	/* Return the percentage */
	return p;
}




/*
 * Hack -- determine if a template is "good"
 */
static int kind_is_good(int k_idx, u32b resf)
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
			if (k_ptr->to_a < 0) return 0;
			return 100;
		}

		/* Weapons -- Good unless damaged */
		case TV_BOW:
		case TV_SWORD:
		case TV_BLUNT:
		case TV_POLEARM:
		case TV_DIGGING:
		case TV_AXE:
		case TV_BOOMERANG:
		{
			if (k_ptr->to_h < 0) return 0;
			if (k_ptr->to_d < 0) return 0;
			return 100;
		}

		/* Ammo -- Arrows/Bolts are good */
		case TV_BOLT:
		case TV_ARROW:
		case TV_SHOT:	/* are Shots bad? */
			if (k_ptr->sval == SV_AMMO_CHARRED) return 0;
		case TV_MSTAFF:
		{
			return 100;
		}

		/* Rings -- Rings of Speed are good */
		case TV_RING:
		{
			if (k_ptr->sval == SV_RING_SPEED) return 100;
			if (k_ptr->sval == SV_RING_BARAHIR) return 100;
			if (k_ptr->sval == SV_RING_TULKAS) return 100;
			if (k_ptr->sval == SV_RING_NARYA) return 100;
			if (k_ptr->sval == SV_RING_NENYA) return 100;
			if (k_ptr->sval == SV_RING_VILYA) return 100;
			if (k_ptr->sval == SV_RING_POWER) return 100;
			return 0;
		}

		/* Amulets -- Amulets of the Magi are good */
		case TV_AMULET:
		{
#if 0
			if (k_ptr->sval == SV_AMULET_THE_MAGI) return 100;
			if (k_ptr->sval == SV_AMULET_THE_MOON) return 100;
			if (k_ptr->sval == SV_AMULET_SPEED) return 100;
			if (k_ptr->sval == SV_AMULET_TERKEN) return 100;
#endif
			return 0;
		}
	}

	/* Assume not good */
	return 0;
}


/* Hack -- inscribe items that a unique drops */
s16b unique_quark = 0;

/* Restrict the type of placed objects */
u32b place_object_restrictor = RESF_NONE;

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
void place_object(struct worldpos *wpos, int y, int x, bool good, bool great, bool verygreat, u32b resf, obj_theme theme, int luck, byte removal_marker)
{
	int prob, base, tmp_luck, i;
	int tries = 0, k_idx, debug_k_idx = 0;

	object_type		forge;
	dun_level *l_ptr = getfloor(wpos);
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;

	/* Paranoia -- check bounds */
	if (!in_bounds(y, x)) return;
	
	/* Hack - No l00t in Valinor */
	if (getlevel(wpos) == 200) return;

#ifdef RPG_SERVER /* no objects are generated in Training Tower */                                        
	if (wpos->wx == cfg.town_x && wpos->wy == cfg.town_y && wpos->wz > 0 && cave_set_quietly) return;
#endif

	/* Require clean floor space */
//	if (!cave_clean_bold(zcave, y, x)) return;

	if (resf & RESF_DEBUG_ITEM) {
		debug_k_idx = luck;
		luck = 0;
	}

	/* place_object_restrictor overrides resf */
	resf |= place_object_restrictor;

	/* Check luck */
	luck += global_luck;
	if (luck < -10) luck = -10;
	if (luck > 40) luck = 40;

	if (luck > 0) {
		/* max luck = 40 */
		tmp_luck = 200 - (8000 / (luck + 40));//+4 luck -> tmp_luck == 19
		if (!good && !great && magik(tmp_luck / 6)) good = TRUE;//was tmp_luck / 2
		else if (good && !great && magik(tmp_luck / 20)) {great = TRUE; good = TRUE;}//was tmp_luck / 15
	} else if (luck < 0) {
		/* min luck = -10 */
		tmp_luck = 200 - (2000 / (-luck + 10));
		if (great && magik(tmp_luck / 3)) {great = FALSE; good = TRUE;}
		else if (!great && good && magik(tmp_luck / 2)) good = FALSE;
	}

	/* Chance of "special object" */
	prob = (good || great ? 30 : 1000); // 10 : 1000

	/* Base level for the object */
	base = (good || great ? (object_level + 10) : object_level);


	/* Hack -- clear out the forgery */
	invwipe(&forge);

	if (resf & RESF_DEBUG_ITEM) {
		k_idx = debug_k_idx;

		/* Prepare the object */
		invcopy(&forge, k_idx);
		forge.number = 1;
	}
	/* Generate a special object, or a normal object */
	else if ((rand_int(prob) != 0) || !make_artifact_special(wpos, &forge, resf))
	{
	
		/* Check global variable, if some base types are forbidden */
		do {
			tries++;
			k_idx = 0;

			/* Good objects */
			if (good)
			{
				/* Activate restriction */
				get_obj_num_hook = kind_is_good;

				/* Prepare allocation table */
				get_obj_num_prep(resf);
			}
			/* Normal objects */
			else
			{
				/* Select items based on "theme" */
				init_match_theme(theme);

				/* Activate normal restriction */
				get_obj_num_hook = kind_is_legal;

				/* Prepare allocation table */
				get_obj_num_prep(resf);

				/* The table is synchronised */
	//			alloc_kind_table_valid = TRUE;
			}


			/* Pick a random object */
			/* Magic arrows from DROP_GREAT monsters are annoying.. - C. Blue */
			/* Added lines for the other magic ammos - the_sandman */
			if (great)
				for (i = 0; i < 20; i++) {
					k_idx = get_obj_num(base, resf);
					if (is_ammo(k_info[k_idx].tval) && k_info[k_idx].sval == SV_AMMO_MAGIC) continue;
					break;
				}
			else
				k_idx = get_obj_num(base, resf);

			/* Good objects */
#if 0	// commented out for efficiency

			if (good)
			{
				/* Clear restriction */
				get_obj_num_hook = NULL;

				/* Prepare allocation table */
				get_obj_num_prep(resf);
			}
#endif	// 0

			if ((resf & RESF_NOHIDSM) && (k_info[k_idx].tval == TV_DRAG_ARMOR) &&
			    !sv_dsm_low(k_info[k_idx].sval) && !sv_dsm_mid(k_info[k_idx].sval))
				continue;
			
			if ((resf & RESF_LOWVALUE) && (k_info[k_idx].cost > 35000)) continue;
			if ((resf & RESF_MIDVALUE) && (k_info[k_idx].cost > 50000)) continue;
			if ((resf & RESF_NOHIVALUE) && (k_info[k_idx].cost > 100000)) continue;
			
			if ((resf & RESF_NOTRUEART) && (k_info[k_idx].flags3 & TR3_INSTA_ART)) continue;

			if (!(resf & RESF_WINNER) && k_info[k_idx].flags5 & TR5_WINNERS_ONLY) continue;
			
			if ((k_info[k_idx].flags5 & TR5_FORCE_DEPTH) && getlevel(wpos) < k_info[k_idx].level) continue;

			/* Allow all other items here - mikaelh */
			break;
		} while(tries < 10);

		/* Note that if we run out of 'tries', the last tested object WILL be used,
		   except if we clear k_idx now. */
		if (tries == 10) k_idx = 0;

		/* Handle failure */
		if (!k_idx) return;

		/* Prepare the object */
		invcopy(&forge, k_idx);
	}

	/* Apply magic (allow artifacts) */
	apply_magic(wpos, &forge, object_level, TRUE, good, great, verygreat, resf);

	/* Hack -- generate multiple spikes/missiles */
	if (!forge.name1)
		switch (forge.tval) {
			case TV_SPIKE:
				forge.number = damroll(6, 7);
				break;
			case TV_SHOT:
			case TV_ARROW:
			case TV_BOLT:
				/* luck has influence on ammo stack size, heh */
				if (luck >= 0)
					forge.number = damroll(6, (forge.sval == SV_AMMO_MAGIC) ? 2 : (7 * (40 + randint(luck)) / 40));
				else
					forge.number = damroll(6, (forge.sval == SV_AMMO_MAGIC) ? 2 : (7 * (20 - randint(-luck)) / 20));
				/* Stacks of ethereal ammo are smaller */
				if (forge.name2 == EGO_ETHEREAL || forge.name2b == EGO_ETHEREAL) forge.number /= ETHEREAL_AMMO_REDUCTION;
				/* Ammo from acquirement scrolls comes in more generous numbers :) */
				if (verygreat) forge.number *= 2;
				if (forge.sval == SV_AMMO_CHARRED) forge.number = randint(6);
				break;
		}

	/* Hack -- inscribe items that a unique drops */
	if (unique_quark) {
		forge.note = unique_quark;
		forge.note_utag = strlen(quark_str(unique_quark)); /* mark this note as 'unique monster quark' */
	}

	forge.marked2 = removal_marker;
	forge.discount = object_discount; /* usually 0, except for creation from stolen acquirement scrolls */
	drop_near(&forge, -1, wpos, y, x);

	/* for now ignore live-spawns. change that maybe? */
	if (cave_set_quietly) {
		if (forge.name1) l_ptr->flags2 |= LF2_ARTIFACT;
		if (k_info[forge.k_idx].level >= getlevel(wpos) + 8) l_ptr->flags2 |= LF2_ITEM_OOD;
	}

}

/* Like place_object(), but doesn't actually drop the object to the floor -  C. Blue */
void generate_object(object_type *o_ptr, struct worldpos *wpos, bool good, bool great, bool verygreat, u32b resf, obj_theme theme, int luck) {
	int prob, base, tmp_luck, i;
	int tries = 0, k_idx;

	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;

	/* Hack - No l00t in Valinor */
	if (getlevel(wpos) == 200) return;

	/* place_object_restrictor overrides resf */
	resf |= place_object_restrictor;

	/* Check luck */
	luck += global_luck;
	if (luck < -10) luck = -10;
	if (luck > 40) luck = 40;

	if (luck > 0) {
		/* max luck = 40 */
		tmp_luck = 200 - (8000 / (luck + 40));//+4 luck -> tmp_luck == 19
		if (!good && !great && magik(tmp_luck / 6)) good = TRUE;//was tmp_luck / 2
		else if (good && !great && magik(tmp_luck / 20)) {great = TRUE; good = TRUE;}//was tmp_luck / 15
	} else if (luck < 0) {
		/* min luck = -10 */
		tmp_luck = 200 - (2000 / (-luck + 10));
		if (great && magik(tmp_luck / 3)) {great = FALSE; good = TRUE;}
		else if (!great && good && magik(tmp_luck / 2)) good = FALSE;
	}

	/* Chance of "special object" */
	prob = (good || great ? 30 : 1000); // 10 : 1000

	/* Base level for the object */
	base = (good || great ? (object_level + 10) : object_level);


	/* Hack -- clear out the forgery */
	invwipe(o_ptr);

	/* Generate a special object, or a normal object */
	if ((rand_int(prob) != 0) || !make_artifact_special(wpos, o_ptr, resf)) {
		/* Check global variable, if some base types are forbidden */
		do {
			tries++;
			k_idx = 0;

			/* Good objects */
			if (good) {
				/* Activate restriction */
				get_obj_num_hook = kind_is_good;

				/* Prepare allocation table */
				get_obj_num_prep(resf);
			}
			/* Normal objects */
			else {
				/* Select items based on "theme" */
				init_match_theme(theme);

				/* Activate normal restriction */
				get_obj_num_hook = kind_is_legal;

				/* Prepare allocation table */
				get_obj_num_prep(resf);

				/* The table is synchronised */
	//			alloc_kind_table_valid = TRUE;
			}


			/* Pick a random object */
			/* Magic arrows from DROP_GREAT monsters are annoying.. - C. Blue */
			/* Added lines for the other magic ammos - the_sandman */
			if (great)
				for (i = 0; i < 20; i++) {
					k_idx = get_obj_num(base, resf);
					if (is_ammo(k_info[k_idx].tval) && k_info[k_idx].sval == SV_AMMO_MAGIC) continue;
					break;
				}
			else
				k_idx = get_obj_num(base, resf);

			/* Good objects */
#if 0	// commented out for efficiency

			if (good)
			{
				/* Clear restriction */
				get_obj_num_hook = NULL;

				/* Prepare allocation table */
				get_obj_num_prep(resf);
			}
#endif	// 0

			if ((resf & RESF_NOHIDSM) && (k_info[k_idx].tval == TV_DRAG_ARMOR) &&
			    !sv_dsm_low(k_info[k_idx].sval) && !sv_dsm_mid(k_info[k_idx].sval))
				continue;
			
			if ((resf & RESF_LOWVALUE) && (k_info[k_idx].cost > 35000)) continue;
			if ((resf & RESF_MIDVALUE) && (k_info[k_idx].cost > 50000)) continue;
			if ((resf & RESF_NOHIVALUE) && (k_info[k_idx].cost > 100000)) continue;
			
			if ((resf & RESF_NOTRUEART) && (k_info[k_idx].flags3 & TR3_INSTA_ART)) continue;

			if (!(resf & RESF_WINNER) && k_info[k_idx].flags5 & TR5_WINNERS_ONLY) continue;

			/* Allow all other items here - mikaelh */
			break;
		} while(tries < 1000);

		/* Note that if we run out of 'tries', the last tested object WILL be used,
		   except if we clear k_idx now. */
		if (tries == 1000) k_idx = 0;

		/* Handle failure */
		if (!k_idx) { /* always generate a reward. in case of failure, make a lamp for now.. */
			switch (rand_int(3)){
			case 0:	k_idx = 527; /* everburning torch */
			case 1:	k_idx = 525; /* dwarven lantern */
			case 2:	k_idx = 530; /* feanorian lamp */
			}
		}

		/* Prepare the object */
		invcopy(o_ptr, k_idx);
	}

	/* Apply magic (allow artifacts) */
	apply_magic(wpos, o_ptr, object_level, TRUE, good, great, verygreat, resf);

	/* Hack -- generate multiple spikes/missiles */
	if (!o_ptr->name1)
		switch (o_ptr->tval)
		{
			case TV_SPIKE:
				o_ptr->number = damroll(6, 7);
				break;
			case TV_SHOT:
			case TV_ARROW:
			case TV_BOLT:
				/* luck has influence on ammo stack size, heh */
				if (luck >= 0)
					o_ptr->number = damroll(6, (o_ptr->sval == SV_AMMO_MAGIC) ? 2 : (7 * (40 + randint(luck)) / 40));
				else
					o_ptr->number = damroll(6, (o_ptr->sval == SV_AMMO_MAGIC) ? 2 : (7 * (20 - randint(-luck)) / 20));
				/* Stacks of ethereal ammo are smaller */
				if (o_ptr->name2 == EGO_ETHEREAL || o_ptr->name2b == EGO_ETHEREAL) o_ptr->number /= ETHEREAL_AMMO_REDUCTION;
				/* Ammo from acquirement scrolls comes in more generous numbers :) */
				if (verygreat) o_ptr->number *= 2;
				if (o_ptr->sval == SV_AMMO_CHARRED) o_ptr->number = randint(6);
				break;
		}

	/* Hack -- inscribe items that a unique drops */
	if (unique_quark) {
		o_ptr->note = unique_quark;
		o_ptr->note_utag = strlen(quark_str(unique_quark)); /* mark this note as 'unique monster quark' */
	}
//s_printf("object generated %d,%d,%d\n", o_ptr->tval, o_ptr->sval, o_ptr->k_idx);
}


/*
 * Scatter some "great" objects near the player
 */
void acquirement(struct worldpos *wpos, int y1, int x1, int num, bool great, bool verygreat, u32b resf)
{
//	int        y, x, i, d;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;

	/* Scatter some objects */
	for (; num > 0; --num) {
		/* Place a good (or great) object */
		place_object_restrictor = RESF_NONE;
		place_object(wpos, y1, x1, TRUE, great, verygreat, resf, default_obj_theme, 0, ITEM_REMOVAL_NORMAL);
		/* Notice */
		note_spot_depth(wpos, y1, x1);
		/* Redraw */
		everyone_lite_spot(wpos, y1, x1);
	}
}
/*
 * Same as acquirement, except it doesn't drop the item to the floor. Creates one "great" object.
 */
void acquirement_direct(object_type *o_ptr, struct worldpos *wpos, bool great, bool verygreat, u32b resf)
{
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;

	/* Place a good (or great) object */
	place_object_restrictor = RESF_NONE;
//s_printf("generating object...");
	generate_object(o_ptr, wpos, TRUE, great, verygreat, resf, default_obj_theme, 0);
//s_printf("object acquired %d,%d,%d\n", o_ptr->tval, o_ptr->sval, o_ptr->k_idx);
}



/* for create_reward() ... */
static int reward_melee_check(player_type *p_ptr, long int treshold) {
	long int rnd_result = 0, selection = 0;
	long int choice1 = 0, choice2 = 0, choice3 = 0, choice4 = 0, choice5 = 0;
	if (p_ptr->s_info[SKILL_SWORD].value >= treshold) choice1 = p_ptr->s_info[SKILL_SWORD].value;
	if (p_ptr->s_info[SKILL_BLUNT].value >= treshold) choice2 = p_ptr->s_info[SKILL_BLUNT].value;
	if (p_ptr->s_info[SKILL_AXE].value >= treshold) choice3 = p_ptr->s_info[SKILL_AXE].value;
	if (p_ptr->s_info[SKILL_POLEARM].value >= treshold) choice4 = p_ptr->s_info[SKILL_POLEARM].value;
	if (p_ptr->s_info[SKILL_MARTIAL_ARTS].value >= treshold) choice5 = p_ptr->s_info[SKILL_MARTIAL_ARTS].value;
	rnd_result = randint(choice1 + choice2 + choice3 + choice4 + choice5);
	/* manipulation concerning magic/melee hybrids but also priests:
	   melee takes precedence over magic, since someone wouldn't skill melee otherwise */
/* TV_MSTAFF	*/
	if (!rnd_result) return 0;
/*  TV_SWORD
    TV_BLUNT
    TV_AXE
    TV_POLEARM
    TV_SHIELD	*/
	if (rnd_result <= choice1) selection = 1;
	else if (rnd_result - choice1 <= choice2) selection = 2;
	else if (rnd_result - choice1 - choice2 <= choice3) selection = 3;
	else if (rnd_result - choice1 - choice2 - choice3 <= choice4) selection = 4;
/* Martial Arts	- will get some light armor or any misc item instead! */
	else if (rnd_result - choice1 - choice2 - choice3 - choice4 <= choice5) selection = 5;

/* Receive a shield instead of a weapon? Depends on actual weapon type! */
	if (p_ptr->pclass == CLASS_ROGUE) return(selection); /* rogues dual-wield, shields are bad for them. */
//Nope, they can!	if (p_ptr->pclass == CLASS_SHAMAN) return(selection); /* shamans cannot cast magic well with shield. */
	switch (selection) {
	case 1: if magik(50) selection = 6; break;
	case 2: if magik(35) selection = 6; break;
	case 3: if magik(15) selection = 6; break;
	case 4: if magik(10) selection = 6; break;
	}
	return (selection);
}
static int reward_ranged_check(player_type *p_ptr, long int treshold) {
	long int rnd_result = 0, selection = 0;
	long int choice1 = 0, choice2 = 0, choice3 = 0, choice4 = 0;
	if (p_ptr->s_info[SKILL_BOW].value >= treshold) choice1 = p_ptr->s_info[SKILL_BOW].value;
	if (p_ptr->s_info[SKILL_XBOW].value >= treshold) choice2 = p_ptr->s_info[SKILL_XBOW].value;
	if (p_ptr->s_info[SKILL_SLING].value >= treshold) choice3 = p_ptr->s_info[SKILL_SLING].value;
	if (p_ptr->s_info[SKILL_BOOMERANG].value >= treshold) choice4 = p_ptr->s_info[SKILL_BOOMERANG].value;
	rnd_result = randint(choice1 + choice2 + choice3 + choice4);
	if (!rnd_result) return 0;
/*  TV_BOW:
    SV_SHORT_BOW, SV_LONG_BOW
    SV_LIGHT_XBOW, SV_HEAVY_XBOW
    SV_SLING
    TV_BOOMERANG	*/
	if (rnd_result <= choice1) selection = 1;
	else if (rnd_result - choice1 <= choice2) selection = 2;
	else if (rnd_result - choice1 - choice2 <= choice3) selection = 3;
	else if (rnd_result - choice1 - choice2 - choice3 <= choice4) selection = 4;
	/* For now no shooter reward for unskilled players (not necessarily needed though..everyone wants a good shooter!) */
	if (!(choice1 + choice2 + choice3 + choice4)) selection = 0;
	return (selection);
}
static int reward_armor_check(player_type *p_ptr, bool mha, bool rha) {
//	int maxweight = (adj_str_hold[p_ptr->stat_ind[A_STR]] - 10) * 10;
	int maxweight = adj_str_armor[p_ptr->stat_ind[A_STR]] * 10;
	long int rnd_result = 0, selection = 0;
	int choice1 = 0, choice2 = 0, choice3 = 0, choice4 = 0, choice5 = 0, choice6 = 0, choice7 = 0, choice8 = 0;
/*  TV_SOFT_ARMOR
    TV_HARD_ARMOR
    TV_DRAG_ARMOR
    TV_BOOTS
    TV_GLOVES
    TV_HELM
    TV_CROWN
    TV_CLOAK	*/
	if (maxweight < 240) choice1 = 10;
	if (maxweight >= 240 && !mha && !rha) choice2 = 30; /* If player can make good use of heavy armour, make him likely to get one! */
	if (maxweight >= 200 && !mha && !rha) choice3 = 3; /* Only slim chance to get a Dragon Scale Mail! */
	choice4 = 10;
	choice5 = 10;
	choice6 = 10;
	choice7 = 5;/* actually funny to give the "Highlander Tournament" winner a crown ;) */
	choice8 = 10;
	rnd_result = randint(choice1 + choice2 + choice3 + choice4 + choice5 + choice6 + choice7 + choice8);
	if (!rnd_result) return 0;
	if (rnd_result <= choice1) selection = 1;
	else if (rnd_result - choice1 <= choice2) selection = 2;
	else if (rnd_result - choice1 - choice2 <= choice3) selection = 3;
	else if (rnd_result - choice1 - choice2 - choice3 <= choice4) selection = 4;
	else if (rnd_result - choice1 - choice2 - choice3 - choice4 <= choice5) selection = 5;
	else if (rnd_result - choice1 - choice2 - choice3 - choice4 - choice5 <= choice6) selection = 6;
	else if (rnd_result - choice1 - choice2 - choice3 - choice4 - choice5 - choice6 <= choice7) selection = 7;
	else selection = 8;
	return (selection);
}
static int reward_spell_check(player_type *p_ptr, long int treshold) {
	int selection = 0;
	s32b value = 0, mod = 0;

	/* get base value of MAGIC skill */
	compute_skills(p_ptr, &value, &mod, SKILL_MAGIC);
	/* check whether player increased magic skill above its base value */
	if (p_ptr->s_info[SKILL_MAGIC].value > value) selection = 1;

#if 0 /* players won't increase this during tourney maybe, and only runemasters can actually have this skill atm */
	/* get base value of RUNEMASTERY skill (doesn't increase magic skill atm) */
	compute_skills(p_ptr, &value, &mod, SKILL_RUNEMASTERY);
	/* check whether player increased magic skill above its base value */
	if (p_ptr->s_info[SKILL_RUNEMASTERY].value > value) selection = 1;
#else
	if (get_skill(p_ptr, SKILL_RUNEMASTERY)) selection = 1;
#endif

	return (selection);
}
static int reward_misc_check(player_type *p_ptr) {
/*  TV_LITE
    TV_AMULET
    TV_RING	*/
	return (randint(3));
}

/*
 * Create a reward (form quests etc) for the player //UNFINISHED, just started to code a few lines, o laziness
 * <min_lv> and <max_lv> restrict the object's base level when choosing from k_info.txt.
 *    currently, they are just averaged to form a 'base object level' for get_obj_num() though.
 * <treshold> is the skill treshold a player must have for a skill to be considered for choosing a reward. - C. Blue
 */
void create_reward(int Ind, object_type *o_ptr, int min_lv, int max_lv, bool great, bool verygreat, u32b resf, long int treshold)
{
	player_type *p_ptr = Players[Ind];
	bool good = TRUE;
	int base = (min_lv + max_lv) / 2; /* base object level */
//	int base = 100;
	int tries = 0, i = 0, j = 0;
	char o_name[ONAME_LEN];
	u32b f1, f2, f3, f4, f5, esp;
	bool mha, rha; /* monk heavy armor, rogue heavy armor */

	/* for analysis functions and afterwards for determining concrete reward */
	int maxweight_melee = adj_str_hold[p_ptr->stat_ind[A_STR]] * 10;
	int maxweight_ranged = adj_str_hold[p_ptr->stat_ind[A_STR]] * 10;
	int maxweight_shield = ((adj_str_hold[p_ptr->stat_ind[A_STR]] / 7) + 4) * 10;
//	int maxweight_armor = (adj_str_hold[p_ptr->stat_ind[A_STR]] - 10) * 10; /* not really directly calculatable, since cumber_armor uses TOTAL armor weight, so just estimate something.. pft */
	int maxweight_armor = adj_str_armor[p_ptr->stat_ind[A_STR]] * 10;

	/* analysis results: */
	int melee_choice, ranged_choice, armor_choice, spell_choice, misc_choice;
	int final_choice = 0; /* 1 = melee, 2 = ranged, 3 = armor, 4 = misc item */

	/* concrete reward */
	int reward_tval = 0, reward_sval = 0, k_idx = 0, reward_maxweight = 500;
	invwipe(o_ptr);

	/* fix reasonable limits */
	if (maxweight_armor < 30) maxweight_armor = 30;

	/* analyze skills */
	melee_choice = reward_melee_check(p_ptr, treshold);
	mha = (melee_choice == 5); /* monk heavy armor */
	rha = (get_skill(p_ptr, SKILL_DODGE)); /* rogue heavy armor; pclass == rogue or get_skill(skill_critical) are implied by this one due to current tables.c. dual_wield is left out on purpose. */
	ranged_choice = reward_ranged_check(p_ptr, treshold);
	armor_choice = reward_armor_check(p_ptr, mha, rha);
	spell_choice = reward_spell_check(p_ptr, treshold);
	misc_choice = reward_misc_check(p_ptr);
	/* martial arts -> no heavy armor (paranoia, shouldn't happen, already cought in reward_armor_check) */
	if (melee_choice == 5 &&
	    (armor_choice == 2 || armor_choice == 3))
		armor_choice = 1;

	/* Special low limits for Martial Arts or Rogue-skill users: */
	if (rha) {
		switch (armor_choice) {
		case 1: maxweight_armor = 100; break;
		case 2: /* cannot happen */ break;
		case 3: /* cannot happen */ break;
		case 4: maxweight_armor = 40; break;
		case 5: maxweight_armor = 25; break;
		case 6: maxweight_armor = 30; break;
		case 7: maxweight_armor = 30; break;
		case 8: maxweight_armor = 15; break;
		}
	}
	if (mha) {
		switch (armor_choice) {
		case 1: maxweight_armor = 30; break;
		case 2: /* cannot happen */ break;
		case 3: /* cannot happen */ break;
		case 4: maxweight_armor = 20; break;
		case 5: maxweight_armor = 20; break;
		case 6: maxweight_armor = 30; break;
		case 7: maxweight_armor = 30; break;
		case 8: maxweight_armor = 15; break;
		}
	}
	if (p_ptr->s_info[SKILL_CRITS].value >= treshold) maxweight_melee = 100;

	/* Choose between possible rewards we gathered from analyzing so far */
	/* Priority: Weapon -> Ranged -> Armor -> Misc */
	if (!melee_choice && spell_choice && ((!armor_choice && !ranged_choice) || magik(50))) final_choice = 4;
	if (melee_choice && ((!armor_choice && !ranged_choice) || magik(50))) final_choice = 1;
	if (ranged_choice && !final_choice && (!armor_choice || magik(50))) final_choice = 2;
	if (melee_choice == 5) final_choice = 3;
	if (armor_choice && !final_choice) final_choice = 3;
/*	if (final_choice == 3 && magik(25)) final_choice = 4; <- no misc items for now, won't be good if not (rand)arts anyway! */
	/* to catch cases where NO result has been chose at all (paranoia): */
	if (!final_choice) final_choice = 3;

	/* Generate TV_ from raw final choice, and choose an appropriate SV_ sub-type now */
	switch (final_choice) {
	case 1:  reward_maxweight = maxweight_melee;
		switch (melee_choice) {
		case 1: reward_tval = TV_SWORD;
			/* Antimagic-users with Sword-skill? Let's be nice and generate a Dark Sword :/ */
			if (get_skill(p_ptr, SKILL_ANTIMAGIC)) reward_sval = SV_DARK_SWORD;
			break;
		case 2: reward_tval = TV_BLUNT; break;
		case 3: reward_tval = TV_AXE; break;
		case 4: reward_tval = TV_POLEARM; break;
		/* 5 -> Martial Arts, handled above */
		case 6: reward_tval = TV_SHIELD; reward_maxweight = maxweight_shield; break;
		}
		break;
	case 2: reward_tval = TV_BOW;
		reward_maxweight = maxweight_ranged;
		switch (ranged_choice) {
		case 1: if (magik(50)) reward_sval = SV_SHORT_BOW; else reward_sval = SV_LONG_BOW; break;
		case 2: if (magik(50) || maxweight_ranged < 200) reward_sval = SV_LIGHT_XBOW; else reward_sval = SV_HEAVY_XBOW; break;
		case 3: reward_sval = SV_SLING; break;
		case 4: reward_tval = TV_BOOMERANG; break;
		}
		break;
	case 3: reward_maxweight = maxweight_armor;
		switch (armor_choice) {
		case 1: reward_tval = TV_SOFT_ARMOR; break;
		case 2: reward_tval = TV_HARD_ARMOR; break;
		case 3: reward_tval = TV_DRAG_ARMOR; break;
		case 4: reward_tval = TV_BOOTS; break;
		case 5: reward_tval = TV_GLOVES; break;
		case 6: reward_tval = TV_HELM; break;
		case 7: reward_tval = TV_CROWN; break;
		case 8: reward_tval = TV_CLOAK; break;
		/* to catch cases where NO result has been chosen at all (paranoia): */
//		default: reward_tval = TV_CROWN; reward_maxweight = 40; break;
		}
		break;
	case 4: reward_maxweight = 500;
		reward_tval = TV_MSTAFF; reward_sval = SV_MSTAFF; break;
	case 5: reward_maxweight = 500; /* no use, so just use any high value.. */
		switch (misc_choice) {
		case 1: reward_tval = TV_LITE; break;
		case 2: reward_tval = TV_AMULET; break;
		case 3: reward_tval = TV_RING; break;
		}
		break;
	}


	/* In case no SVAL has been defined yet: 
	   Choose a random SVAL while paying attention to maxweight limit! */
	if (!reward_sval) {
		/* Check global variable, if some base types are forbidden */
		do {
			tries++;
			k_idx = 0;

			if (reward_tval != TV_AMULET && reward_tval != TV_RING) { /* rings+amulets don't count as good so they won't be generated (see kind_is_good) */
				get_obj_num_hook = kind_is_good;
				get_obj_num_prep_tval(reward_tval, resf);
			} else {
				get_obj_num_hook = NULL;
				get_obj_num_prep_tval(reward_tval, resf);
			}

			/* Pick a random object */
			/* Magic arrows from DROP_GREAT monsters are annoying.. - C. Blue */
			/* Added lines for the other magic ammos - the_sandman */
			if (great)
				for (i = 0; i < 20; i++) {
					k_idx = get_obj_num(base, resf);
					if (is_ammo(k_info[k_idx].tval) && k_info[k_idx].sval == SV_AMMO_MAGIC) continue;
					break;
				}
			else
				k_idx = get_obj_num(base, resf);

			/* HACK - Kollas won't pass RESF_LOWVALUE or RESF_MIDVALUE checks in apply_magic - mikaelh */
			if ((k_info[k_idx].tval == TV_CLOAK) && (k_info[k_idx].sval == SV_KOLLA) && ((resf & (RESF_LOWVALUE | RESF_MIDVALUE)))) {
				continue;
			}

			/* Prepare the object */
			invcopy(o_ptr, k_idx);
			reward_sval = o_ptr->sval;

			/* Note that in theory the item's weight might change depending on it's
			   apply_magic_depth outcome, we're ignoring that here for now though. */

			/* Check for weight limit! */
			if (k_info[k_idx].weight > reward_maxweight) continue;

			/* No weapon that reduces bpr compared to what weapon the person currently holds! */
			if (is_weapon(reward_tval)) { /* melee weapon */
				if (p_ptr->inventory[INVEN_WIELD].k_idx) i = calc_blows_obj(Ind, &p_ptr->inventory[INVEN_WIELD]);
				if (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD) j = calc_blows_obj(Ind, &p_ptr->inventory[INVEN_ARM]);
				if (j > i) i = j; /* for dual-wielders, use the faster one */
				if (!i) i = 3; /* if player doesn't hold a weapon atm (why!?) just start trying for 3 bpr */
				if ((calc_blows_obj(Ind, o_ptr) < i) && (tries < 70)) continue; /* try hard to not lose a single bpr at first */
				if ((calc_blows_obj(Ind, o_ptr) < i) && (i < 4) && (tries < 90)) continue; /* try to at least not lose a bpr if it drops us below 3 */
				if ((calc_blows_obj(Ind, o_ptr) < i) && (i < 3)) continue; /* 1 bpr is simply the worst, gotta keep trying */
			}

			if ((resf & RESF_NOHIDSM) &&
			    (k_info[k_idx].tval == TV_DRAG_ARMOR) &&
			    !sv_dsm_low(k_info[k_idx].sval) && !sv_dsm_mid(k_info[k_idx].sval))
				continue;
				
			break;
		} while (tries < 100); /* need more tries here than in place_object() because of additional weight limit check */

#if 0 /* Let's just take the last attempt then. We NEED a reward. */
		/* Note that if we run out of 'tries', the last tested object WILL be used,
		   except if we clear k_idx now. */
		if (tries == 100) k_idx = 0;
		/* Handle failure */
		if (!k_idx) {
			invwipe(o_ptr);
			return;
		}
#endif

	} else {
		/* Prepare the object */
		invcopy(o_ptr, lookup_kind(reward_tval, reward_sval));
	}

	/* debug log */
	s_printf("REWARD_RAW: final_choice %d, reward_tval %d, k_idx %d, tval %d, sval %d, weight %d(%d)\n", final_choice, reward_tval, k_idx, o_ptr->tval, o_ptr->sval, o_ptr->weight, reward_maxweight);
	if (is_admin(p_ptr))
		msg_format(Ind, "Reward: final_choice %d, reward_tval %d, k_idx %d, tval %d, sval %d, weight %d(%d)", final_choice, reward_tval, k_idx, o_ptr->tval, o_ptr->sval, o_ptr->weight, reward_maxweight);

	/* hack - fix the shit with an ugly workaround for now (shouldn't happen anymore) */
	if (!o_ptr->sval) {
		for (i = 1; i < max_k_idx; i++)
			if (k_info[i].tval == reward_tval && k_info[k_idx].weight <= reward_maxweight) {
				reward_sval = k_info[i].sval;
				break;
			}
		invcopy(o_ptr, lookup_kind(reward_tval, reward_sval));
		s_printf("REWARD_UGLY (%d,%d)\n", o_ptr->tval, o_ptr->sval);
	}

	/* apply_magic to that item, until we find a fitting one */
	tries = 0;
	i = o_ptr->k_idx;
	do {
		tries++;
		invwipe(o_ptr);
		invcopy(o_ptr, i);

		/* Apply magic (allow artifacts) */
		apply_magic_depth(base, o_ptr, base, TRUE, good, great, verygreat, resf);
		s_printf("REWARD_REAL: final_choice %d, reward_tval %d, k_idx %d, tval %d, sval %d, weight %d(%d)\n", final_choice, reward_tval, k_idx, o_ptr->tval, o_ptr->sval, o_ptr->weight, reward_maxweight);
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

		/* This should have already been checked in apply_magic_depth above itself,
		   but atm it seems not to work: */
		if ((o_ptr->name1 == ART_RANDART) && (resf & RESF_NORANDART)) continue;
		if (o_ptr->name1 && (o_ptr->name1 != ART_RANDART) && (resf & RESF_NOTRUEART)) continue;
		if ((o_ptr->name2 && o_ptr->name2b) && (resf & RESF_NODOUBLEEGO)) continue;
		if ((resf & RESF_LOWSPEED) && (f1 & TR1_SPEED) && (o_ptr->bpval > 4 || o_ptr->pval > 4)) continue;
		if ((resf & RESF_NOHISPEED) && (f1 & TR1_SPEED) && (o_ptr->bpval > 6 || o_ptr->pval > 6)) continue;
		if (!(f1 & TR1_SPEED)) {
			if ((resf & RESF_LOWVALUE) && (object_value_real(0, o_ptr) > 35000)) continue;
			if ((resf & RESF_MIDVALUE) && (object_value_real(0, o_ptr) > 50000)) continue;
			if ((resf & RESF_NOHIVALUE) && (object_value_real(0, o_ptr) > 100000)) continue;
		} else {
			if ((resf & RESF_LOWVALUE) && (object_value_real(0, o_ptr) > 200000)) continue;
			if ((resf & RESF_MIDVALUE) && (object_value_real(0, o_ptr) > 200000)) continue;
			if ((resf & RESF_NOHIVALUE) && (object_value_real(0, o_ptr) > 250000)) continue;
		}

		/* Don't generate cursed randarts.. */
		if (cursed_p(o_ptr)) continue;

		/* Don't generate mage-only benefitting reward if we don't use magic */
		if (!spell_choice) {
			switch (o_ptr->name2) {
			case EGO_MAGI: /* crown of magi, it's not bad for anyone actually */
			case EGO_CLOAK_MAGI: /* well, it does provide speed.. */
			case EGO_CONCENTRATION:
			case EGO_INTELLIGENCE:
			case EGO_WISDOM:
			case EGO_BRILLIANCE:
			case EGO_ISTARI:
			case EGO_OFTHEMAGI: continue;
			}
			switch (o_ptr->name2b) {
			case EGO_MAGI: /* crown of magi, it's not bad for anyone actually */
			case EGO_CLOAK_MAGI: /* well, it does provide speed.. */
			case EGO_CONCENTRATION:
			case EGO_INTELLIGENCE:
			case EGO_WISDOM:
			case EGO_BRILLIANCE:
			case EGO_ISTARI:
			case EGO_OFTHEMAGI: continue;
			}
		}
		
		/* analyze class (so far nothing is done here, but everything is determined by skills instead) */
		switch (p_ptr->pclass) {
		case CLASS_WARRIOR:
		case CLASS_ARCHER:
			break;
		case CLASS_ADVENTURER:
		case CLASS_SHAMAN:
			if ((p_ptr->stat_max[A_INT] > p_ptr->stat_max[A_WIS]) &&
			    (o_ptr->name2 == EGO_WISDOM || o_ptr->name2b == EGO_WISDOM)) continue;
			if ((p_ptr->stat_max[A_WIS] > p_ptr->stat_max[A_INT]) &&
			    (o_ptr->name2 == EGO_INTELLIGENCE || o_ptr->name2b == EGO_INTELLIGENCE)) continue;
			break;
		case CLASS_MAGE:
		case CLASS_RANGER:
		case CLASS_ROGUE:
		case CLASS_RUNEMASTER:
		case CLASS_MINDCRAFTER:
			if (o_ptr->name2 == EGO_WISDOM || o_ptr->name2b == EGO_WISDOM) continue;
			break;
		case CLASS_PRIEST:
		case CLASS_PALADIN:
		case CLASS_DRUID:
			if (o_ptr->name2 == EGO_INTELLIGENCE || o_ptr->name2b == EGO_INTELLIGENCE) continue;
			break;
		}
		
		/* analyze race */
		if (p_ptr->prace == RACE_VAMPIRE && anti_undead(o_ptr)) continue;

		/* Don't generate NO_MAGIC or DRAIN_MANA items if we do use magic */
		if (spell_choice) {
			if (f5 & TR5_DRAIN_MANA) continue;
			if (f3 & TR3_NO_MAGIC) continue;
		}

		/* Don't generate (possibly expensive due to high bpval, hence passed up till here) crap */
		switch (o_ptr->name2) {
		case EGO_INFRAVISION:
		case EGO_BEAUTY:
		case EGO_CHARMING: continue;
		default: break;
		}

		/* a reward should have some value: */
		if (object_value_real(0, o_ptr) < 5000) continue;
		if (o_ptr->name2) { /* should always be true actually! just paranoia */
			if (e_info[o_ptr->name2].cost <= 2000) {
				if (o_ptr->name2b) {
					if (e_info[o_ptr->name2b].cost <= 2000) continue;
				} else continue;
			}
		}

		break;
	} while (tries < (verygreat ? 20 : 100));

	/* more loggin' */
	object_desc(0, o_name, o_ptr, TRUE, 2+8+16);
	s_printf("REWARD_CREATED: (%s) %s\n", p_ptr->name, o_name);

	/* Give the object to the player who is to be rewarded */
/*	inven_carry(Ind, o_ptr); <- not neccessarily >;) */
}

/* shorten the process of creating a standard-parm reward */
void give_reward(int Ind, u32b resf, cptr quark, int level, int discount)
{
	object_type forge, *o_ptr = &forge;
        create_reward(Ind, o_ptr, 95, 95, TRUE, TRUE, resf, 3000);
	object_aware(Ind, o_ptr);
	object_known(o_ptr);
	o_ptr->discount = discount;
	o_ptr->level = level;
	o_ptr->ident |= ID_MENTAL;
	if (quark) o_ptr->note = quark_add(quark);
	inven_carry(Ind, o_ptr);
}


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
void place_gold(struct worldpos *wpos, int y, int x, int bonus)
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
	
	/* not in Valinor */
	if (getlevel(wpos) == 200) return;

	/* Require clean floor grid */
//	if (!cave_clean_bold(zcave, y, x)) return;

	/* Hack -- Pick a Treasure variety */
	i = ((randint(object_level + 2) + 2) / 2);

	/* Apply "extra" magic */
	if (rand_int(GREAT_OBJ) == 0)
		i += randint(object_level + 1);

	/* Hack -- Creeping Coins only generate "themselves" */
	if (coin_type) i = coin_type + 1;

	/* Do not create "illegal" Treasure Types */
	if (i > SV_GOLD_MAX) i = SV_GOLD_MAX;

	invcopy(&forge, lookup_kind(TV_GOLD, i));

	/* Hack -- Base coin cost */
//	base = k_info[OBJ_GOLD_LIST+i].cost;
	base = k_info[lookup_kind(TV_GOLD, i)].cost;

	/* Determine how much the treasure is "worth" */
	forge.pval = (base + (8L * randint(base)) + randint(8)) + bonus;

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
	int k, d, ny, nx, i, s;	// , y1, x1
	int bs, bn;
	int by, bx;
	//int ty, tx;
	int o_idx = -1;
	int flag = 0;	// 1 = normal, 2 = combine, 3 = crash

	cave_type	*c_ptr;

	/* for destruction checks */
	bool is_potion = FALSE, do_kill = FALSE, plural = FALSE;
	cptr note_kill = NULL;
	u32b f1, f2, f3, f4, f5, esp;


	bool arts = artifact_p(o_ptr), crash, done = FALSE;
	u16b this_o_idx, next_o_idx = 0;

	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return(-1);

	/* Handle normal "breakage" */
	if (!arts && magik(chance)) {
		/* Failure */
		return (-1);
	}

	/* Score */
	bs = -1;

	/* Picker */
	bn = 0;

	/* Default */
	by = y;
	bx = x;

	d = 0;

	/* Scan local grids */
	for (i = 0; i < tdi[3]; i++) {
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

		/* Hack: Don't drop items below immovable unkillable monsters aka the
		   Target Dummy, so players can get their items (ammo) back - C. Blue */
		if (c_ptr->m_idx > 0) {
			k = m_list[c_ptr->m_idx].r_idx;
			if (((r_info[k].flags1 & RF1_NEVER_MOVE) ||
			    (r_info[k].flags7 & RF7_NEVER_ACT)) &&
			    (r_info[k].flags7 & RF7_NO_DEATH))
			continue;
		}

		/* No traps */
//		if (c_ptr->t_idx) continue;

		/* No objects */
		k = 0;

		/* Scan objects in that grid */
		for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
			object_type *j_ptr;

			/* Acquire object */
			j_ptr = &o_list[this_o_idx];

			/* Acquire next object */
			next_o_idx = j_ptr->next_o_idx;

			/* Check for possible combination */
			if (object_similar(0, o_ptr, j_ptr, 0x0)) comb = TRUE;

			/* Count objects */
			k++;
		}

		/* Add new object */
		if (!comb) k++;

		/* No stacking (allow combining) */
//		if (!testing_stack && (k > 1)) continue;

		/* Hack -- no stacking inside houses */
		/* XXX this can cause 'arts crashes arts' */
		crash = (!wpos->wz && k > 1 && !comb && (c_ptr->info & CAVE_ICKY));
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
	if (!flag) {
		/* Describe */
		/*object_desc(o_name, o_ptr, FALSE, 0);*/

		/* Message */
		/*msg_format("The %s disappear%s.",
		           o_name, ((o_ptr->number == 1) ? "s" : ""));*/

		if (true_artifact_p(o_ptr)) handle_art_d(o_ptr->name1);
		return (-1);
	}

	ny = by;
	nx = bx;
	c_ptr = &zcave[ny][nx];

	/* some objects get destroyed by falling on certain floor type - C. Blue */
        object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
	if (o_ptr->tval == TV_POTION) is_potion = TRUE;
	if (o_ptr->number > 1) plural = TRUE;
	switch (c_ptr->feat) {
	case FEAT_SHAL_WATER:
	case FEAT_DEEP_WATER:
	case FEAT_GLIT_WATER:
//	case FEAT_WATER:
//	case FEAT_TAINTET_WATER:
		if (hates_water(o_ptr)) {
			do_kill = TRUE;
			note_kill = (plural ? " are soaked!" : " is soaked!");
			if (f5 & TR5_IGNORE_WATER) do_kill = FALSE;
		}
		break;
	case FEAT_SHAL_LAVA:
	case FEAT_DEEP_LAVA:
	case FEAT_FIRE:
	case FEAT_GREAT_FIRE:
		if (hates_fire(o_ptr)) {
			do_kill = TRUE;
			note_kill = is_potion ? (plural ? " evaporate!" : " evaporates!") : (plural ? " burn up!" : " burns up!");
			if (f3 & TR3_IGNORE_FIRE) do_kill = FALSE;
		}
		break;
	}

	if (do_kill) {
#if 0 //needs adjustments
		/* Effect "observed" */
		if (!quiet && p_ptr->obj_vis[this_o_idx]) obvious = TRUE;
		/* Artifacts, and other objects, get to resist */
		if (is_art || ignore) {
			/* Observe the resist */
			if (!quiet && p_ptr->obj_vis[this_o_idx]) {
				msg_format(Ind, "The %s %s unaffected!",
				o_name, (plural ? "are" : "is"));
			}
		/* Kill it */
		} else {
			/* Describe if needed */
			if (!quiet && p_ptr->obj_vis[this_o_idx] && note_kill)
				msg_format(Ind, "\377oThe %s%s", o_name, note_kill);
			/* Potions produce effects when 'shattered' */
			if (is_potion) (void)potion_smash_effect(who, wpos, ny, nx, o_sval);
			if (!quiet) everyone_lite_spot(wpos, ny, nx);
		}
#endif
		if (true_artifact_p(o_ptr)) handle_art_d(o_ptr->name1); /* just paranoia here */
		return (-1);
	}

	/* Artifact always disappears, depending on tomenet.cfg flags */
	/* this should be in drop_near_severe, would be cleaner sometime in the future.. */
	if (wpos->wz == 0) { /* Assume houses are always on surface */
		if (undepositable_artifact_p(o_ptr) && cfg.anti_arts_house && inside_house(wpos, nx, ny)) {
//			char	o_name[ONAME_LEN];
//			object_desc(Ind, o_name, o_ptr, TRUE, 0);
//			msg_format(Ind, "%s fades into the air!", o_name);
			handle_art_d(o_ptr->name1);
			return (-1);
		}
	}

	/* Scan objects in that grid for combination */
	if (flag == 2) for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
		object_type *q_ptr;

		/* Acquire object */
		q_ptr = &o_list[this_o_idx];

		/* Acquire next object */
		next_o_idx = q_ptr->next_o_idx;

		/* Check for combination */
		if (object_similar(0, q_ptr, o_ptr, 0x0)) {
			/* Combine the items */
			object_absorb(0, q_ptr, o_ptr);

			/* Success */
			done = TRUE;

			/* for player-store 'offer' check */
			o_idx = this_o_idx;

			/* Done */
			break;
		}
	}

	/* Successful drop */
//	if (flag)
	else {
		/* Assume fails */
//		flag = FALSE;

		/* XXX XXX XXX */

//		c_ptr = &zcave[ny][nx];

		/* Crush anything under us (for artifacts) */
		if (flag == 3) delete_object(wpos, ny, nx, TRUE);

		/* Make a new object */
		o_idx = o_pop();

		/* Success */
		if (o_idx) {
			/* Structure copy */
			o_list[o_idx] = *o_ptr;

			/* Access */
			o_ptr = &o_list[o_idx];

			/* Locate */
			o_ptr->iy = ny;
			o_ptr->ix = nx;
			wpcopy(&o_ptr->wpos, wpos);

			/* reset scan_objs timer */
			o_ptr->marked = 0;

			/* Keep game pieces from disappearing */
			if ((o_ptr->tval == 1) && (o_ptr->sval >= 9))
				o_ptr->marked2 = ITEM_REMOVAL_NEVER;

			/* items dropped into a house (well or a vault
			   on surface if such exists) are marked to not
			   get removed by timeout check, allowing us to
			   additionally check and delete objects on
			   unallocated levels - C. Blue */
			if (o_ptr->marked2 != ITEM_REMOVAL_NEVER) {
				if (wpos->wz == 0 && (c_ptr->info & CAVE_ICKY)) {
					/* mark as 'inside a house' */
					o_ptr->marked2 = ITEM_REMOVAL_HOUSE;
				} else if (o_ptr->marked2 != ITEM_REMOVAL_DEATH_WILD &&
				    o_ptr->marked2 != ITEM_REMOVAL_LONG_WILD) {
					/* clear possible previous ITEM_REMOVAL_HOUSE mark */
					o_ptr->marked2 = ITEM_REMOVAL_NORMAL;
				}
			}

			/* items dropped in pvp arena are deleted quickly - C. Blue */
			if (wpos->wz == WPOS_PVPARENA_Z && wpos->wy == WPOS_PVPARENA_Y && wpos->wx == WPOS_PVPARENA_X)
				o_ptr->marked2 = ITEM_REMOVAL_QUICK;


			/* No monster */
			o_ptr->held_m_idx = 0;

			/* ToDo maybe: limit max stack size (by something like o_ptr->stack_pos) */
#if 0 /* under construction; MAX_ITEMS_STACKING */
			if (o_list[c_ptr->o_idx]->stack_pos >= MAX_ITEMS_STACKING) {
				if (true_artifact_p(o_ptr)) handle_art_d(o_ptr->name1); /* just paranoia here */
				return (-1);
			}
			o_ptr->stack_pos = o_list[c_ptr->o_idx]->stack_pos + 1;
#endif

			/* Build a stack */
			o_ptr->next_o_idx = c_ptr->o_idx;

			/* Place */
//			c_ptr = &zcave[ny][nx];
			c_ptr->o_idx = o_idx;

			/* Clear visibility flags */
			for (k = 1; k <= NumPlayers; k++) {
				/* This player cannot see it */
				Players[k]->obj_vis[o_idx] = FALSE;
			}

			/* Note the spot */
			note_spot_depth(wpos, ny, nx);

			/* Draw the spot */
			everyone_lite_spot(wpos, ny, nx);

#ifdef USE_SOUND_2010
			/* done in do_cmd_drop() atm */
#else
			/*sound(SOUND_DROP);*/
#endif

			/* Mega-Hack -- no message if "dropped" by player */
			/* Message when an object falls under the player */
			/*if (chance && (ny == py) && (nx == px))
			{
				msg_print("You feel something roll beneath your feet.");
			}*/

			if (chance && c_ptr->m_idx < 0) {
				msg_print(0 - c_ptr->m_idx, "You feel something roll beneath your feet.");
			}

			/* Success */
//			flag = TRUE;
		} else /* paranoia: couldn't allocate a new object */ {
			if (true_artifact_p(o_ptr)) handle_art_d(o_ptr->name1);
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

	/* Items dropped by admins never disappear by 'time out' */
	if (is_admin(p_ptr)) o_ptr->marked2 = ITEM_REMOVAL_NEVER;

	/* Artifact always disappears, depending on tomenet.cfg flags */
	/* hm for now we also allow ring of phasing to be traded between winners. not needed though. */
	if (true_artifact_p(o_ptr) && !is_admin(p_ptr) &&
	    (cfg.anti_arts_hoard || (p_ptr->total_winner && !winner_artifact_p(o_ptr))))
	    //(cfg.anti_arts_hoard || (cfg.anti_arts_house && 0)) would be cleaner sometime in the future..
	{
		char	o_name[ONAME_LEN];
		object_desc(Ind, o_name, o_ptr, TRUE, 0);

		msg_format(Ind, "%s fades into the air!", o_name);
		handle_art_d(o_ptr->name1);
		return -1;
	}
	else return (drop_near(o_ptr, chance, wpos, y, x));
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
int divide_charged_item(object_type *o_ptr, int amt)
{
	int charge = 0;

	/* Paranoia */
	if (o_ptr->number < amt) return (-1);

	if (o_ptr->tval == TV_WAND) {
		charge = (o_ptr->pval * amt) / o_ptr->number;
		if (amt < o_ptr->number) o_ptr->pval -= charge;
	}

	return (charge);
}


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

	char	o_name[ONAME_LEN];

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
	if (num) {
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
 * WARNING: Since this slides down following items, DON'T use this in a loop that
 * processes items and goes from lower value upwards to higher value if you don't
 * intend that result for some reason!! - C. Blue
 */
bool inven_item_optimize(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];

	object_type *o_ptr = &p_ptr->inventory[item];

	/* Only optimize real items */
	if (!o_ptr->k_idx) {
		invwipe(o_ptr); /* hack just for paranoia: make sure it's erased */
		return (FALSE);
	}

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

		/* Update inventory indeces - mikaelh */
		inven_index_erase(Ind, item);
		inven_index_slide(Ind, item + 1, -1, INVEN_PACK);

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
//TODO: change completely to be defined by client individually! - C. Blue
void auto_inscribe(int Ind, object_type *o_ptr, int flags)
{
	player_type *p_ptr = Players[Ind];
#if 0
	char c[] = "@m ";
#endif

	if (!o_ptr->tval) return;

	/* skip inscribed items */
	if (!flags && o_ptr->note &&
	    strcmp(quark_str(o_ptr->note), "on sale") &&
	    !strstr(quark_str(o_ptr->note), "% off") &&
	    !strstr(quark_str(o_ptr->note), "stolen"))
		return;

	if (p_ptr->obj_aware[o_ptr->k_idx]) {
		if (o_ptr->tval == TV_SCROLL &&
		    o_ptr->sval == SV_SCROLL_WORD_OF_RECALL) {
			o_ptr->note = quark_add("@r3@R");
			return;
		} else if (o_ptr->tval == TV_ROD &&
		    o_ptr->sval == SV_ROD_RECALL) {
			o_ptr->note = quark_add("@z3@R");
			return;
		}
		else if (o_ptr->tval == TV_SCROLL) {
			if (o_ptr->sval == SV_SCROLL_PHASE_DOOR) {
				o_ptr->note = quark_add("@r1");
				return;
			}
			if (o_ptr->sval == SV_SCROLL_TELEPORT) {
				o_ptr->note = quark_add("@r2");
				return;
			}
			if (o_ptr->sval == SV_SCROLL_IDENTIFY) {
				o_ptr->note = quark_add("@r5");
				return;
			}
			if (o_ptr->sval == SV_SCROLL_TRAP_DOOR_DESTRUCTION) {
				o_ptr->note = quark_add("@r8");
				return;
			}
			if (o_ptr->sval == SV_SCROLL_MAPPING) {
				o_ptr->note = quark_add("@r9");
				return;
			}
			if (o_ptr->sval == SV_SCROLL_SATISFY_HUNGER) {
				o_ptr->note = quark_add("@r0");
				return;
			}
		}
		else if (o_ptr->tval == TV_POTION) {
			if (o_ptr->sval == SV_POTION_HEALING) {
				o_ptr->note = quark_add("@q1");
				return;
			}
			if (o_ptr->sval == SV_POTION_SPEED) {
				o_ptr->note = quark_add("@q2");
				return;
			}
			if (o_ptr->sval == SV_POTION_RESISTANCE) {
				o_ptr->note = quark_add("@q3");
				return;
			}
			if (o_ptr->sval == SV_POTION_RESTORE_EXP) {
				o_ptr->note = quark_add("@q4");
				return;
			}
		}
	}

#if 0	/* disabled till new spell system is done */
	if (!is_realm_book(o_ptr) && o_ptr->tval != TV_BOOK) return;

	/* XXX though it's ok with 'm' for everything.. */
	c[2] = o_ptr->sval +1 +48;
	o_ptr->note = quark_add(c);
#endif
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
		if (object_similar(Ind, j_ptr, o_ptr, 0x0)) return (TRUE);
	}

	/* Hack -- try quiver slot (see inven_carry) */
//	if (object_similar(Ind, &p_ptr->inventory[INVEN_AMMO], o_ptr, 0x0)) return (TRUE);

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
	u32b f1 = 0, f2 = 0, f3 = 0, f4 = 0, f5 = 0, esp = 0;

	/* Check for combining */
	for (j = 0; j < INVEN_PACK; j++) {
		j_ptr = &p_ptr->inventory[j];

		/* Skip empty items */
		if (!j_ptr->k_idx) continue;

		/* Hack -- track last item */
		n = j;

		/* Check if the two items can be combined */
		if (object_similar(Ind, j_ptr, o_ptr, 0x0)) {
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
	for (j = 0; j < INVEN_PACK; j++) {
		j_ptr = &p_ptr->inventory[j];

		/* Use it if found */
		if (!j_ptr->k_idx) break;
	}

	/* Check if if the overflow slot is already occupied */
	if (j == INVEN_PACK && p_ptr->inventory[INVEN_PACK].k_idx) {
		/* Force a pack overflow now to clear the overflow slot */
		s_printf("WARNING: Forcing a pack overflow for player %s.\n", p_ptr->name);
		pack_overflow(Ind);
	}

	/* Use that slot */
	i = j;


	/* Hack -- pre-reorder the pack */
	if (i < INVEN_PACK) {
		s64b		o_value, j_value;

		/* Get the "value" of the item */
		o_value = object_value(Ind, o_ptr);

		/* Scan every occupied slot */
		for (j = 0; j < INVEN_PACK; j++) {
			j_ptr = &p_ptr->inventory[j];

			/* Use empty slots */
			if (!j_ptr->k_idx) break;

			/* Objects sort by decreasing type */
			if (o_ptr->tval > j_ptr->tval) break;
			if (o_ptr->tval < j_ptr->tval) continue;

			/* Hack: Don't sort ammo any further, to allow players
			   a custom order of usage for !L inscription - C. Blue */
			if (is_ammo(o_ptr->tval)) continue;

			/* Non-aware (flavored) items always come last */
			if (!object_aware_p(Ind, o_ptr)) continue;
			if (!object_aware_p(Ind, j_ptr)) break;

			/* Objects sort by increasing sval */
			if (o_ptr->sval < j_ptr->sval) break;
			if (o_ptr->sval > j_ptr->sval) continue;

			/* Level 0 items owned by the player come first */
			if (o_ptr->level == 0 && o_ptr->owner == p_ptr->id && j_ptr->level != 0) break;
			if (j_ptr->level == 0 && j_ptr->owner == p_ptr->id && o_ptr->level != 0) continue;

			/* Level 0 items owned by other players always come last */
			if (o_ptr->level == 0 && o_ptr->owner && o_ptr->owner != p_ptr->id && !(j_ptr->level == 0 && j_ptr->owner && j_ptr->owner != p_ptr->id)) continue;
			if (j_ptr->level == 0 && j_ptr->owner && j_ptr->owner != p_ptr->id && !(o_ptr->level == 0 && o_ptr->owner && o_ptr->owner != p_ptr->id)) break;

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
		for (k = n; k >= i; k--) {
			/* Hack -- Slide the item */
			p_ptr->inventory[k+1] = p_ptr->inventory[k];
		}

		/* Update inventory indeces - mikaelh */
		inven_index_slide(Ind, i, 1, n);

		/* Paranoia -- Wipe the new slot */
		invwipe(&p_ptr->inventory[i]);
	}

	if (!o_ptr->owner && !p_ptr->admin_dm) {
		o_ptr->owner = p_ptr->id;
		o_ptr->mode = p_ptr->mode;
	}

	/* Auto-inscriber */
#ifdef	AUTO_INSCRIBER
	if (p_ptr->auto_inscribe) auto_inscribe(Ind, o_ptr, 0);
#endif


	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	/* Auto Curse */
	if (f3 & TR3_AUTO_CURSE) {
		/* The object recurse itself ! */
		if (!(o_ptr->ident & ID_CURSED)) {
			o_ptr->ident |= ID_CURSED;
			o_ptr->ident |= ID_SENSE | ID_SENSED_ONCE;
			o_ptr->note = quark_add("cursed");
		}
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
			if (object_similar(Ind, j_ptr, o_ptr, p_ptr->current_force_stack - 1 == i ? 0x2 : 0x0))
			{
				/* clear if used */
				if (p_ptr->current_force_stack - 1 == i) p_ptr->current_force_stack = 0;

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

				/* Update inventory indeces - mikaelh */
				inven_index_move(Ind, j, i);
				inven_index_slide(Ind, i + 1, -1, INVEN_PACK);

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

	/* clear */
	if (p_ptr->current_force_stack) {
		if (!flag) msg_print(Ind, "Nothing to combine.");
		p_ptr->current_force_stack = 0;
	}
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

	s64b	o_value;
	s64b	j_value;

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

			/* Objects sort by decreasing type */
			if (o_ptr->tval > j_ptr->tval) break;
			if (o_ptr->tval < j_ptr->tval) continue;

			/* Hack: Don't sort ammo any further, to allow players
			   a custom order of usage for !L inscription - C. Blue */
			if (is_ammo(o_ptr->tval)) continue;

			/* Non-aware (flavored) items always come last */
			if (!object_aware_p(Ind, o_ptr)) continue;
			if (!object_aware_p(Ind, j_ptr)) break;

			/* Objects sort by increasing sval */
			if (o_ptr->sval < j_ptr->sval) break;
			if (o_ptr->sval > j_ptr->sval) continue;

			/* Level 0 items owned by the player come first */
			if (o_ptr->level == 0 && o_ptr->owner == p_ptr->id && j_ptr->level != 0) break;
			if (j_ptr->level == 0 && j_ptr->owner == p_ptr->id && o_ptr->level != 0) continue;

			/* Level 0 items owned by other players always come last */
			if (o_ptr->level == 0 && o_ptr->owner && o_ptr->owner != p_ptr->id && !(j_ptr->level == 0 && j_ptr->owner && j_ptr->owner != p_ptr->id)) continue;
			if (j_ptr->level == 0 && j_ptr->owner && j_ptr->owner != p_ptr->id && !(o_ptr->level == 0 && o_ptr->owner && o_ptr->owner != p_ptr->id)) break;

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

		/* Update inventory indeces - mikaelh */
		inven_index_slide(Ind, j, 1, i - 1);
		inven_index_move(Ind, i, j);

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
	for (k = o_top - 1; k >= 0; k--) {
		/* Access index */
		i = o_fast[k];

		/* Access object */
		o_ptr = &o_list[i];

		/* Excise dead objects */
		if (!o_ptr->k_idx) {
			/* Excise it */
			o_fast[k] = o_fast[--o_top];

			/* Skip */
			continue;
		}

		/* Recharge rods on the ground */
		if ((o_ptr->tval == TV_ROD) && (o_ptr->pval)) o_ptr->pval--;
		
		/* Recharge rod trap sets */
		if (o_ptr->tval == TV_TRAPKIT && o_ptr->sval == SV_TRAPKIT_DEVICE &&
		    o_ptr->timeout) o_ptr->timeout--;
	}
}

/*
 * Set the "o_idx" fields in the cave array to correspond
 * to the objects in the "o_list".
 */
void setup_objects(void)
{
	int i;
	cave_type **zcave;
	object_type *o_ptr;

	for (i = 0; i < o_max; i++)
	{
		o_ptr = &o_list[i];

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
 * Prepare an object based on an existing object: dest, src
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
		if (object_similar(o_ptr, j_ptr, 0x0))
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

/* Easier unified artifact handling */
void handle_art_i(int aidx) {
	if (a_info[aidx].cur_num < 255) a_info[aidx].cur_num++;
	a_info[aidx].known = TRUE;
}
void handle_art_ipara(int aidx) { /* only for paranoia.. could be removed basically (except for the known=TRUE part) */
	if (!a_info[aidx].cur_num && (a_info[aidx].cur_num < 255)) a_info[aidx].cur_num++;
	a_info[aidx].known = TRUE;
}
void handle_art_inum(int aidx) {
	if (a_info[aidx].cur_num < 255) a_info[aidx].cur_num++;
}
void handle_art_inumpara(int aidx) {
	if (!a_info[aidx].cur_num && (a_info[aidx].cur_num < 255)) a_info[aidx].cur_num++;
}
void handle_art_dnum(int aidx) {
	if (a_info[aidx].cur_num > 0) a_info[aidx].cur_num--;
}
void handle_art_d(int aidx) {
	if (a_info[aidx].cur_num > 0) {
		a_info[aidx].cur_num--;
		if (!a_info[aidx].cur_num) a_info[aidx].known = FALSE;
	} else {
		a_info[aidx].cur_num = 0;
	}
}

/* Check whether an item causes HP drain on an undead player (vampire) who wears/wields it */
bool anti_undead(object_type *o_ptr) {
	u32b f1, f2, f3, f4, f5, esp;
	int l = 0;

	if (cursed_p(o_ptr)) return(FALSE);

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
        if (f3 & TR3_LITE1) l++;
        if (f4 & TR4_LITE2) l += 2;
        if (f4 & TR4_LITE3) l += 3;
	if ((f4 & TR4_FUEL_LITE) && (o_ptr->timeout < 1)) l = 0;

	/* powerful lights and anti-undead/evil items damage vampires */
	if (l) { /* light sources, or other items that provide light */
		if ((l > 2) || o_ptr->name1 || (f3 & TR3_BLESSED) ||
		    (f1 & TR1_SLAY_EVIL) || (f1 & TR1_SLAY_UNDEAD) || (f1 & TR1_KILL_UNDEAD))
			return(TRUE);
	} else {
		if ((f3 & TR3_BLESSED) || (f1 & TR1_KILL_UNDEAD))
			return(TRUE);
	}

	return(FALSE);
}

/* 
 * Generate default item-generation restriction flags for a given player - C. Blue
 */
u32b make_resf(player_type *p_ptr) {
	u32b f = RESF_NONE;
	if (p_ptr == NULL) return(f);

	/* winner type handling */
	if (p_ptr->once_winner) {
		if (cfg.fallenkings_etiquette) {
			f |= RESF_NOTRUEART; /* player is a fallen winner? Then don't find true arts! */
			/* since fallen kings are already kinda punished by their status, cut them some slack here: */
			f |= RESF_WINNER; /* allow generation of WINNERS_ONLY items, if player won once but can't get true arts */
		}
	}
	if (p_ptr->total_winner) {
		f |= RESF_WINNER; /* allow generation of WINNERS_ONLY items */
		if (cfg.kings_etiquette) f |= RESF_NOTRUEART; /* player is currently a winner? Then don't find true arts! */
	}
	if (!cfg.winners_find_randarts) f |= ~RESF_NOTRUEART; /* monsters killed by [fallen] winners can drop no true arts but randarts only instead? */
	if (p_ptr->total_winner ||
	    (p_ptr->once_winner && p_ptr->lev >= 50 && cfg.fallenkings_etiquette))
		f |= RESF_LIFE; /* allowed to find +LIFE artifacts */

	/* special mode handling */
	if (p_ptr->mode & MODE_PVP) f |= RESF_NOTRUEART; /* PvP mode chars can't use true arts, since true arts are for kinging! */

	return (f);
}

/*
 * Items have been slided in the inventory - mikaelh
 */
void inven_index_slide(int Ind, s16b begin, s16b mod, s16b end)
{
	player_type *p_ptr = Players[Ind];
	inventory_change_type *inv_change, *list;

	/* Update index numbers for p_ptr->current_* items when sliding items in the inventory */

	if (p_ptr->current_rod != -1 && p_ptr->current_rod >= begin && p_ptr->current_rod <= end)
		p_ptr->current_rod += mod;

	if (p_ptr->current_activation != -1 && p_ptr->current_activation >= begin && p_ptr->current_activation <= end)
		p_ptr->current_activation += mod;

	/* Record the change if the client supports inventory IDs */
	if (is_newer_than(&p_ptr->version, 4, 4, 2, 1, 0, 0)) {
		MAKE(inv_change, inventory_change_type);
		inv_change->revision = ++p_ptr->inventory_revision;
		inv_change->type = INVENTORY_CHANGE_SLIDE;
		inv_change->begin = begin;
		inv_change->end = end;
		inv_change->mod = mod;

		/* Add it to the end of the list */
		list = p_ptr->inventory_changes;
		if (list) {
			while (list->next) list = list->next;
			list->next = inv_change;
		} else {
			p_ptr->inventory_changes = inv_change;
		}

		p_ptr->inventory_changed = TRUE;
	}
}

/*
 * Items have been moved in the inventory - mikaelh
 */
void inven_index_move(int Ind, s16b slot, s16b new_slot)
{
	player_type *p_ptr = Players[Ind];
	inventory_change_type *inv_change, *list;

	/* Update index numbers for p_ptr->current_* items when moving items in the inventory */

	if (p_ptr->current_rod == slot) p_ptr->current_rod = new_slot;
	if (p_ptr->current_activation == slot) p_ptr->current_activation = new_slot;

	/* Record the change if the client supports inventory IDs */
	if (is_newer_than(&p_ptr->version, 4, 4, 2, 1, 0, 0)) {
		MAKE(inv_change, inventory_change_type);
		inv_change->revision = ++p_ptr->inventory_revision;
		inv_change->type = INVENTORY_CHANGE_MOVE;
		inv_change->begin = slot;
		inv_change->end = new_slot;
		inv_change->mod = 0;

		/* Add it to the end of the list */
		list = p_ptr->inventory_changes;
		if (list) {
			while (list->next) list = list->next;
			list->next = inv_change;
		} else {
			p_ptr->inventory_changes = inv_change;
		}

		p_ptr->inventory_changed = TRUE;
	}
}

/*
 * Items have been erased from the inventory - mikaelh
 */
void inven_index_erase(int Ind, s16b slot)
{
	player_type *p_ptr = Players[Ind];
	inventory_change_type *inv_change, *list;

	/* Update index numbers for p_ptr->current_* items when erasing items in the inventory */

	if (p_ptr->current_rod == slot) p_ptr->current_rod = -1;
	if (p_ptr->current_activation == slot) p_ptr->current_activation = -1;

	/* Record the change if the client supports inventory IDs */
	if (is_newer_than(&p_ptr->version, 4, 4, 2, 1, 0, 0)) {
		MAKE(inv_change, inventory_change_type);
		inv_change->revision = ++p_ptr->inventory_revision;
		inv_change->type = INVENTORY_CHANGE_ERASE;
		inv_change->begin = slot;
		inv_change->end = -1;
		inv_change->mod = 0;

		/* Add it to the end of the list */
		list = p_ptr->inventory_changes;
		if (list) {
			while (list->next) list = list->next;
			list->next = inv_change;
		} else {
			p_ptr->inventory_changes = inv_change;
		}

		p_ptr->inventory_changed = TRUE;
	}
}

/*
 * Apply recorded changes to an inventory slot number - mikaelh
 */
s16b replay_inven_changes(int Ind, s16b slot)
{
	player_type *p_ptr = Players[Ind];
	inventory_change_type *inv_change;

	inv_change = p_ptr->inventory_changes;
	while (inv_change) {
		switch (inv_change->type) {
			case INVENTORY_CHANGE_SLIDE:
				if (slot >= inv_change->begin && slot <= inv_change->end)
					slot += inv_change->mod;
				break;
			case INVENTORY_CHANGE_MOVE:
				if (slot == inv_change->begin) slot = inv_change->end;
				break;
			case INVENTORY_CHANGE_ERASE:
				if (slot == inv_change->begin) return 0xFF;
				break;
		}
		inv_change = inv_change->next;
	}

	return slot;
}

/*
 * Client is now aware of the inventory changes so they can be removed - mikaelh
 */
void inven_confirm_revision(int Ind, int revision)
{
	player_type *p_ptr = Players[Ind];
	inventory_change_type *inv_change, *prev_change, *next_change;

	inv_change = p_ptr->inventory_changes;
	prev_change = NULL;
	while (inv_change) {
		/* Don't delete everything in case of an overflow */
		if (((revision > 0 && inv_change->revision > 0) || (revision <= 0)) &&
		    inv_change->revision <= revision) {
			/* Delete the record */

			if (prev_change) prev_change->next = inv_change->next;
			else p_ptr->inventory_changes = inv_change->next;

			next_change = inv_change->next;
			KILL(inv_change, inventory_change_type);
			inv_change = next_change;
		}
		else {
			prev_change = inv_change;
			inv_change = inv_change->next;
		}
	}
}
