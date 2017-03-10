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

/* Prevent Weapons of Morgul from dropping in Ironman Deep Dive Challenge? - C. Blue */
#define NO_MORGUL_IN_IDDC

/* Allow ego-trapkits that aren't bolt/arrow/shot type? */
#define TRAPKIT_EGO_ALL


//#if FORCED_DROPS == 1  --now also required for tc_bias
static int which_theme(int tval);
//#endif

/* For treasure class fairness */
static int tc_bias_treasure = 100;
static int tc_bias_combat = 100;
static int tc_bias_magic = 100;
static int tc_bias_tools = 100;
static int tc_bias_junk = 100;

static int tc_biasg_treasure = 100;
static int tc_biasg_combat = 100;
static int tc_biasg_magic = 100;
static int tc_biasg_tools = 100;
static int tc_biasg_junk = 100;

static int tc_biasr_treasure = 100;
static int tc_biasr_combat = 100;
static int tc_biasr_magic = 100;
static int tc_biasr_tools = 100;
static int tc_biasr_junk = 100;



/*
 * Excise a dungeon object from any stacks
 * Borrowed from ToME.
 */
void excise_object_idx(int o_idx) {
	object_type *j_ptr, *o_ptr;
	u16b this_o_idx, next_o_idx = 0;
	u16b prev_o_idx = 0;
	int i;

	/* Object */
	j_ptr = &o_list[o_idx];

#ifdef MONSTER_INVENTORY
	/* Monster */
	if (j_ptr->held_m_idx) {
		monster_type *m_ptr;

		/* Monster */
		m_ptr = &m_list[j_ptr->held_m_idx];

		/* Scan all objects the monster has */
		for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx) {
			/* Acquire object */
			o_ptr = &o_list[this_o_idx];

			/* Acquire next object */
			next_o_idx = o_ptr->next_o_idx;

			/* Done */
			if (this_o_idx == o_idx) {
				/* No previous */
				if (prev_o_idx == 0) {
					/* Remove from list */
					m_ptr->hold_o_idx = next_o_idx;
				}

				/* Real previous */
				else {
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
		if (!(zcave = getcave(&j_ptr->wpos))) return;

		/* Somewhere out of this world */
		if (!in_bounds2(&j_ptr->wpos, y, x)) return;

		c_ptr = &zcave[y][x];

		/* Scan all objects in the grid */
		for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
			/* Acquire object */
			o_ptr = &o_list[this_o_idx];

			/* Acquire next object */
			next_o_idx = o_ptr->next_o_idx;

			/* Done */
			if (this_o_idx == o_idx) {
				/* No previous */
				if (prev_o_idx == 0) {
					/* Remove from list */
					if (c_ptr) {
						c_ptr->o_idx = next_o_idx;
						nothing_test2(c_ptr, x, y, &j_ptr->wpos, 0);
					}
				}
				/* Real previous */
				else {
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
#ifdef MAX_ITEMS_STACKING
			else {
				/* decrement it's stack position index */
				if (o_ptr->stack_pos) o_ptr->stack_pos--;
			}
#endif

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
void delete_object_idx(int o_idx, bool unfound_art) {
	object_type *o_ptr = &o_list[o_idx];
	int i;

	int y = o_ptr->iy;
	int x = o_ptr->ix;
	//cave_type **zcave;
	struct worldpos *wpos = &o_ptr->wpos;

#if 1 /* extra logging for artifact timeout debugging */
	if (true_artifact_p(o_ptr) && o_ptr->owner) {
		char o_name[ONAME_LEN];

		object_desc_store(0, o_name, o_ptr, TRUE, 3);

		s_printf("%s owned true artifact (ua=%d) deleted at (%d,%d,%d):\n  %s\n",
		    showtime(), unfound_art,
		    wpos->wx, wpos->wy, wpos->wz,
		    o_name);
	}
#endif

	/* Artifact becomes 'not found' status */
	if (true_artifact_p(o_ptr) && unfound_art)
		handle_art_d(o_ptr->name1);
	/* uh, we abuse this */
	if (unfound_art) questitem_d(o_ptr, o_ptr->number);

#ifdef PLAYER_STORES
	/* Log removal of player store items */
	if (!(o_ptr->held_m_idx) && o_ptr->note && strstr(quark_str(o_ptr->note), "@S")
	    && inside_house(wpos, o_ptr->ix, o_ptr->iy)) {
		char o_name[ONAME_LEN];//, p_name[NAME_LEN];
		object_desc(0, o_name, o_ptr, TRUE, 3);
		//s_printf("PLAYER_STORE_REMOVED (doidx): %s - %s (%d,%d,%d; %d,%d).\n",
		s_printf("PLAYER_STORE_REMOVED (doidx): %s (%d,%d,%d; %d,%d).\n",
		    //p_name, o_name, wpos->wx, wpos->wy, wpos->wz,
		    o_name, wpos->wx, wpos->wy, wpos->wz,
		    o_ptr->ix, o_ptr->iy);
	}
#endif

	/* Excise */
	excise_object_idx(o_idx);

	/* No one can see it anymore */
	for (i = 1; i <= NumPlayers; i++)
		Players[i]->obj_vis[o_idx] = FALSE;

	/* Visual update */
	/* Dungeon floor */
	if (!(o_ptr->held_m_idx)) everyone_lite_spot(wpos, y, x);

	/* log special cases */
	if (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_ARTIFACT_CREATION)
		//note: number is already 0 at this point if it was floor/inven_item_increase'd
		s_printf("%s ARTSCROLL_DELETED (amt:%d) (%d,%d,%d)\n", showtime(), o_ptr->number, o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz);

	/* Wipe the object */
	WIPE(o_ptr, object_type);
}

/*
 * Deletes object from given location
 */
void delete_object(struct worldpos *wpos, int y, int x, bool unfound_art) { /* maybe */
	cave_type *c_ptr;
	cave_type **zcave;

	/* Refuse "illegal" locations */
	if (!in_bounds(y, x)) return;

	if ((zcave = getcave(wpos))) {
		u16b this_o_idx, next_o_idx = 0;

		c_ptr = &zcave[y][x];
#if 0
	/* Refuse "illegal" locations */
	if (!in_bounds(Depth, y, x)) return;

	if (cave[Depth]){	/* This is fast indexing method first */
		/* Find where it was */
		c_ptr = &cave[Depth][y][x];
#endif	// 0

		/* Delete the object */
//		if (c_ptr->o_idx) delete_object_idx(c_ptr->o_idx, unfound_art);

		/* Scan all objects in the grid */
		for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
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
	else {			/* Cave depth not static (houses etc) - do slow method */
		int i;
		for (i = 0; i < o_max; i++) {
			object_type *o_ptr = &o_list[i];
			if (o_ptr->k_idx && inarea(wpos, &o_ptr->wpos)) {
				if (y == o_ptr->iy && x == o_ptr->ix)
					delete_object_idx(i, unfound_art);
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
//#define DONT_COMPACT_NEARBY
void compact_objects(int size, bool purge) {
	int i, y, x, num, cnt, Ind; //, j, ny, nx;

	s32b cur_val, cur_lev, chance;
#ifdef DONT_COMPACT_NEARBY
	s32b cur_dis;
#endif
	struct worldpos *wpos;
//	object_type *q_ptr;
	cave_type *c_ptr, **zcave;

	int tmp_max = o_max;
	quest_info *q_ptr;

	/* Compact */
	if (size) {
		/* Message */
		s_printf("Compacting objects...\n");
	}


	/* Compact at least 'size' objects */
	for (num = 0, cnt = 1; num < size; cnt++) {
		/* Get more vicious each iteration */
		cur_lev = 5 * cnt;

		/* Destroy more valuable items each iteration */
		cur_val = 500 * (cnt - 1);

#ifdef DONT_COMPACT_NEARBY
		/* Get closer each iteration */
		cur_dis = 5 * (20 - cnt);
#endif

		/* Examine the objects */
		for (i = 1; i < o_max; i++) {
			object_type *o_ptr = &o_list[i];

			object_kind *k_ptr = &k_info[o_ptr->k_idx];

			/* Skip dead objects */
			if (!o_ptr->k_idx) continue;

			/* Questors are immune */
			if (o_ptr->questor) continue;

			/* Hack -- High level objects start out "immune" */
			if (k_ptr->level > cur_lev) continue;

			/* Get the location */
			y = o_ptr->iy;
			x = o_ptr->ix;

#ifdef DONT_COMPACT_NEARBY
			/* Nearby objects start out "immune" */
			if ((cur_dis > 0) && (distance(py, px, y, x) < cur_dis)) continue;
#endif

			/* Valuable objects start out "immune" */
			if (object_value(0, &o_list[i]) > cur_val) continue;

			/* Saving throw */
			chance = 90;

			/* Hack -- only compact artifacts in emergencies */
			if (artifact_p(o_ptr) && (cnt < 1000)) chance = 100;

			if ((zcave = getcave(&o_ptr->wpos))) {
				/* Hack -- only compact items in houses (or surface vaults) or inns in emergencies */
				if (!o_ptr->wpos.wz && ((zcave[y][x].info & (CAVE_ICKY | CAVE_PROT)) ||
				    (f_info[zcave[y][x].feat].flags1 & FF1_PROTECTED))) {
#if 0
					if (cnt < 1000) /* Grant immunity except in emergencies */
#endif
					chance = 100;
				}
				/* Don't compact items on protected grids in special locations) */
				else if ((zcave[y][x].info & CAVE_PROT) ||
				    (f_info[zcave[y][x].feat].flags1 & FF1_PROTECTED)) //IDDC town inns!
					chance = 100;
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
	for (i = o_max - 1; i >= 1; i--) {
		/* Get the i'th object */
		object_type *o_ptr = &o_list[i];

		/* Skip real objects */
		/* real objects in unreal location are not skipped. */
		/* hack -- items on wilderness are preserved, since
		 * they can be house contents. */
		if (o_ptr->k_idx) {
			/* Skip questors always
			   NOTE: This will even keep them alive in the dungeon although
			         the dungeon floor might've gotten deallocated already. */
			if (o_ptr->questor) continue;

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
		int z = 1;
		monster_type *m_ptr;
		object_type *o_ptr;

		/* allocate storage for map */
		old_idx = calloc(1, o_max * sizeof(int));

		/* map the list and compress it */
		for (i = 1; i < o_max; i++) {
			if (o_list[i].k_idx) {
				if (z != i) {
					/* Copy structure */
					o_list[z] = o_list[i];

					/* Quests: keep questor_m_idx information consistent */
					if (o_list[z].questor) {
						q_ptr = &q_info[o_list[z].quest - 1];
						/* paranoia check, after server restarted after heavy code changes or sth */
						if (q_ptr->defined && q_ptr->questors > o_list[z].questor_idx) {
							/* fix its index */
#if 0
							s_printf("QUEST_COMPACT_OBJECTS: quest %d - questor %d o_idx %d->%d\n", o_list[z].quest - 1, o_list[z].questor_idx, q_ptr->questor[o_list[z].questor_idx].mo_idx, z);
#endif
							q_ptr->questor[o_list[z].questor_idx].mo_idx = z;
						} else {
							s_printf("QUEST_COMPACT_OBJECTS: deprecated questor, quest %d - questor %d o_idx %d->%d\n", o_list[z].quest - 1, o_list[z].questor_idx, q_ptr->questor[o_list[z].questor_idx].mo_idx, z);
							o_list[z].questor = FALSE;
							o_list[z].quest = 0; //do this too, or questitem_d() will falsely recognise it as a quest item
							/* delete it too? */
						}
					}

					/* this needs to go through all objects - mikaelh */
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
		for (i = tmp_max; i < o_max; i++)
			WIPE(&o_list[i], object_type);

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

			if ((zcave = getcave(wpos))) {
				c_ptr = &zcave[y][x];
				if (in_bounds2(wpos, y, x)) {
					if (old_idx[c_ptr->o_idx] == i) {
						c_ptr->o_idx = i;
						nothing_test2(c_ptr, x, y, wpos, 1);
					}
				}
				else {
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
	for (i = 0; i < o_max; i++) {
		/* Collect indexes */
		o_fast[o_top++] = i;
	}
}


/*
 * Delete all the items when player leaves the level
 *
 * Note -- we do NOT visually reflect these (irrelevant) changes
 */

void wipe_o_list(struct worldpos *wpos) {
	int i;
	cave_type **zcave;
	monster_type *m_ptr;
	bool flag = FALSE;

	if((zcave = getcave(wpos))) flag = TRUE;


	/* Delete the existing objects */
	for (i = 1; i < o_max; i++) {
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip objects not on this depth */
		if (!inarea(&o_ptr->wpos, wpos))
			continue;

		/* Mega-Hack -- preserve artifacts */
		/* Hack -- Preserve unknown artifacts */
		/* We now preserve ALL artifacts, known or not */
		if (true_artifact_p(o_ptr)/* && !object_known_p(o_ptr)*/) {
			/* Info */
			/* s_printf("Preserving artifact %d.\n", o_ptr->name1); */

			/* Mega-Hack -- Preserve the artifact */
			handle_art_d(o_ptr->name1);
		}
		questitem_d(o_ptr, o_ptr->number);

#ifdef MONSTER_INVENTORY
		/* Monster */
		if (o_ptr->held_m_idx) {
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
			zcave[o_ptr->iy][o_ptr->ix].o_idx = 0;

		/* Wipe the object */
		WIPE(o_ptr, object_type);
	}

	/* Compact the object list */
	compact_objects(0, FALSE);
}
/*
 * Delete all the items, but except those in houses.  - Jir -
 * Also skips questors and special quest items now. Note that this is also the
 * command to be used by all admin slash commands. - C. Blue
 *
 * Note -- we do NOT visually reflect these (irrelevant) changes
 * (cave[Depth][y][x].info & CAVE_ICKY)
 */
void wipe_o_list_safely(struct worldpos *wpos) {
	int i;

	cave_type **zcave;
	monster_type *m_ptr;

	if (!(zcave = getcave(wpos))) return;

	/* Delete the existing objects */
	for (i = 1; i < o_max; i++) {
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip questors */
		if (o_ptr->questor ||
		    (o_ptr->quest && o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_QUEST))
			continue;

		/* Skip objects not on this depth */
		if (!(inarea(wpos, &o_ptr->wpos)))
			continue;

/* DEBUG -after getting weird crashes today 2007-12-21 in bree from /clv, and multiplying townies, I added this inbound check- C. Blue */
		//if (in_bounds_array(o_ptr->iy, o_ptr->ix)) {
			/* Skip objects inside a house but not in a vault in dungeon/tower */
			if (!wpos->wz && zcave[o_ptr->iy][o_ptr->ix].info & CAVE_ICKY)
				continue;
		//}

		/* Mega-Hack -- preserve artifacts */
		/* Hack -- Preserve unknown artifacts */
		/* We now preserve ALL artifacts, known or not */
		if (true_artifact_p(o_ptr)/* && !object_known_p(o_ptr)*/) {
			/* Info */
			/* s_printf("Preserving artifact %d.\n", o_ptr->name1); */

			/* Mega-Hack -- Preserve the artifact */
			handle_art_d(o_ptr->name1);
		}

#ifdef MONSTER_INVENTORY
		/* Monster */
		if (o_ptr->held_m_idx) {
			/* Monster */
			m_ptr = &m_list[o_ptr->held_m_idx];

			/* Hack -- see above */
			m_ptr->hold_o_idx = 0;
		}

		/* Dungeon */
		else if (in_bounds_array(o_ptr->iy, o_ptr->ix))
#endif	// MONSTER_INVENTORY
		{
			zcave[o_ptr->iy][o_ptr->ix].o_idx = 0;
		}

		/* Wipe the object */
		WIPE(o_ptr, object_type);
	}

	/* Compact the object list */
	compact_objects(0, FALSE);
}
/* Exactly like wipe_o_list() but actually makes exceptions for special dungeon floors. - C. Blue
   Special means: Static IDDC town floor. (Could maybe be used for quests too in some way.) */
void wipe_o_list_special(struct worldpos *wpos) {
	int i;
	cave_type **zcave;
	monster_type *m_ptr;
	bool flag = FALSE;

	if ((zcave = getcave(wpos))) flag = TRUE;

#if 0 /* actually disabled for now! (anti-cheeze, to be safe) */
	if (sustained_wpos(wpos)) return;
#endif

	/* Delete the existing objects */
	for (i = 1; i < o_max; i++) {
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip objects not on this depth */
		if (!inarea(&o_ptr->wpos, wpos))
			continue;

		/* Mega-Hack -- preserve artifacts */
		/* Hack -- Preserve unknown artifacts */
		/* We now preserve ALL artifacts, known or not */
		if (true_artifact_p(o_ptr)/* && !object_known_p(o_ptr)*/) {
			/* Info */
			/* s_printf("Preserving artifact %d.\n", o_ptr->name1); */

			/* Mega-Hack -- Preserve the artifact */
			handle_art_d(o_ptr->name1);
		}
		questitem_d(o_ptr, o_ptr->number);

#ifdef MONSTER_INVENTORY
		/* Monster */
		if (o_ptr->held_m_idx) {
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
			zcave[o_ptr->iy][o_ptr->ix].o_idx = 0;

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
s16b o_pop(void) {
	int i, n, k;

	/* Initial allocation */
	if (o_max < MAX_O_IDX) {
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
	for (n = 1; n < MAX_O_IDX; n++) {
		/* Get next space */
		i = o_nxt;

		/* Advance (and wrap) the "next" pointer */
		if (++o_nxt >= MAX_O_IDX) o_nxt = 1;

		/* Skip objects in use */
		if (o_list[i].k_idx) continue;

		/* Verify space XXX XXX */
		if (o_top >= MAX_O_IDX) continue;

		/* Verify not allocated */
		for (k = 0; k < o_top; k++) {
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
errr get_obj_num_prep(u32b resf) {
	long	i, n, p, adj;
	long	k_idx;
#if FORCED_DROPS != 0 /* either, way 1 or 2 */
	int tval, sval;
#endif

	/* Get the entry */
	alloc_entry *restrict table = alloc_kind_table;

	/* Copy the hook into a local variable for speed */
	int (*hook)(int k_idx, u32b resf) = get_obj_num_hook;

	/* Scan the allocation table */
	for (i = 0, n = alloc_kind_size; i < n; i++) {
		/* Get the entry */
		alloc_entry *entry = &table[i];

		/* Obtain the base probability */
		p = entry->prob1;

		/* Default probability for this pass */
		entry->prob2 = 0;

		/* Access the index */
		k_idx = entry->index;

		/* Call the hook and adjust the probability */
		if (hook) {
			adj = (*hook)(k_idx, resf);
			p = (adj * p) / 1000;
		}

#if FORCED_DROPS == 1 /* way 1/2 */
		/* Check for special item types */
		tval = k_info[k_idx].tval;
		sval = k_info[k_idx].sval;
		//force generation of a sword, if generating a combat item at all
		if ((resf & RESF_COND_SWORD) && which_theme(tval) == 2) {
			if (tval != TV_SWORD) p = 0;
			else if (sval == SV_DARK_SWORD) p >>= 2; //don't overdo it..
		}
		//force generation of a dark sword, if generating a combat item at all
		if ((resf & RESF_COND_DARKSWORD) && which_theme(tval) == 2 && (tval != TV_SWORD || sval != SV_DARK_SWORD)) p = 0;
		//force generation of a blunt, if generating a combat at all
		if ((resf & RESF_COND_BLUNT) && which_theme(tval) == 2 && tval != TV_BLUNT) p = 0;
		//force generation of a non-sword weapon, if generating a _weapon_ at all
		if ((resf & RESF_CONDF_NOSWORD) && is_melee_weapon(tval) && tval == TV_SWORD) p = 0;
		//force generation of a mage staff:
		if (resf & RESF_CONDF_MSTAFF && tval != TV_MSTAFF) p = 0;
		//force generation of a sling or sling-ammo, if generating a combat item at all
		if ((resf & RESF_COND_SLING) && which_theme(tval) == 2 && (tval != TV_BOW || sval != SV_SLING) && tval != TV_SHOT) p = 0;
		//force generation of a ranged weapon or ammo, if generating a combat item at all
		if ((resf & RESF_COND_RANGED) && which_theme(tval) == 2 && !is_ranged_weapon(tval) && !is_ammo(tval)) p = 0;
		//force generation of a rune, if generating a magic item at all
		if ((resf & RESF_CONDF_RUNE) && which_theme(tval) == 3 && tval != TV_RUNE) p = 0;
#endif
#if FORCED_DROPS == 2 /* way 2/2 */
		/* Check for special item types. - C. Blue
		   Note: The 'p = 10000' lines are for enabling this item for monsters
		   who usually don't have it in their theme table. This is needed for
		   drops we expect from certain ego monster types. Eg ogres don't have
		   magic items in their loot table, but an ogre mage could drop a mage staff! */
		tval = k_info[k_idx].tval;
		sval = k_info[k_idx].sval;
		if (resf & RESF_COND_FORCE) {
			if (resf & RESF_COND_SWORD) { //force generation of a sword
				if (tval != TV_SWORD) p = 0;
				else p = 10000;
			}
			if (resf & RESF_COND_LSWORD) { //force generation of a light sword
				if (tval != TV_SWORD || (k_info[k_idx].flags4 & (TR4_MUST2H | TR4_SHOULD2H))) p = 0;
				else p = 10000;
			}
			if (resf & RESF_COND_DARKSWORD) { //force generation of a dark sword
				if (tval != TV_SWORD || sval != SV_DARK_SWORD) p = 0;
				else p = 10000;
			}
			if (resf & RESF_COND_BLUNT) { //force generation of a blunt weapon
				if (tval != TV_BLUNT) p = 0;
				else p = 10000;
			}
			if ((resf & RESF_CONDF_NOSWORD) && tval == TV_SWORD) p = 0; //force generation of a non-sword if 'generating' a weapon
			if (resf & RESF_CONDF_MSTAFF) { //force generation of a mage staff
				if (tval != TV_MSTAFF) p = 0;
				else p = 10000;
			}
			if (resf & RESF_COND_SLING) { //force generation of a sling (or ammo)
				if (tval != TV_BOW || sval != SV_SLING) p = 0;
				else if (tval == TV_SHOT) p = 3000;
				else p = 10000; //sling
			}
			if (resf & RESF_COND_RANGED) { //force generation of a ranged weapon (or ammo)
				if (!is_ranged_weapon(tval) && !is_ammo(tval)) p = 0;
				else if (is_ammo(tval)) p = 3000;
				else if (tval == TV_BOOMERANG) p = 3000;
				else p = 10000; //sling/bow/crossbow
			}
			if (resf & RESF_CONDF_RUNE) { //force generation of a rune
				if (tval != TV_RUNE) p = 0;
				else p = 10000;
			}
		} else {
			//force generation of a sword, if generating a weapon
			if ((resf & RESF_COND_SWORD) && is_weapon(tval)) {
				if (tval != TV_SWORD) p = 0;
				else if (sval == SV_DARK_SWORD) p >>= 2; //don't overdo it..
				else p = 10000;
			}
			//force generation of a light sword, if generating a weapon
			if ((resf & RESF_COND_LSWORD) && is_weapon(tval)) {
				if (tval != TV_SWORD || (k_info[k_idx].flags4 & (TR4_MUST2H | TR4_SHOULD2H))) p = 0;
				else p = 10000;
			}
			//force generation of a dark sword, if generating a weapon
			if ((resf & RESF_COND_DARKSWORD) && is_weapon(tval)) {
				if (tval != TV_SWORD || sval != SV_DARK_SWORD) p = 0;
				else p = 10000;
			}
			//force generation of a blunt, if generating a weapon
			if ((resf & RESF_COND_BLUNT) && is_weapon(tval)) {
				if (tval != TV_BLUNT) p = 0;
				else p = 10000;
			}
			//force generation of a non-sword, if generating a weapon (note: focusses on melee weapons when clearing)
			if ((resf & RESF_CONDF_NOSWORD) && tval == TV_SWORD) p = 0;
			//force generation of a mage staff, absolutely:
			if (resf & RESF_CONDF_MSTAFF) {
				if (tval != TV_MSTAFF) p = 0;
				else p = 10000;
			}
			//force generation of a sling or sling-ammo, if generating any weapon or ammo (note: focusses on ranged weapons when clearing)
			if ((resf & RESF_COND_SLING) && (is_weapon(tval) || is_ammo(tval))) {
				if ((tval != TV_BOW || sval != SV_SLING) && tval != TV_SHOT) p = 0;
				else if (tval == TV_SHOT) p = 10000;
				else p = 10000; //sling
			}
			//force generation of a ranged weapon or ammo
			if ((resf & RESF_COND_RANGED) && (is_weapon(tval) || is_ammo(tval))) {
				if (!is_ranged_weapon(tval) && !is_ammo(tval)) p = 0;
				else if (is_ammo(tval)) p = 10000;
				else if (tval == TV_BOOMERANG) p = 3000;
				else p = 10000; //sling/bow/crossbow
			}
			//force generation of a rune, absolutely
			if (resf & RESF_CONDF_RUNE) {
				if (tval != TV_RUNE) p = 0;
				else p = 10000;
			}
		}
		//mostly avoid heavy armour
		if ((resf & RESF_COND2_LARMOUR) && is_tough_armour(tval, sval)) p >>= 2;
		//mostly avoid light armour
		if ((resf & RESF_COND2_HARMOUR) && is_flexible_armour(tval, sval)) p >>= 2;
#endif

		/* Dungeon town stores: Even rarer items have same probability of appearing */
		if (p && (resf & RESF_STOREFLAT)) p = 100;

		/* Save the probability */
		entry->prob2 = p;
	}

	/* Success */
	return (0);
}



/*
 * Apply a "object restriction function" to the "object allocation table"
 * This function only takes objects of a certain TVAL! - C. Blue
 * (note that kind_is_legal_special and this function are somewhat redundant)
 * (this function supports STOREFLAT but isn't called by store.c, what gives)
 */
errr get_obj_num_prep_tval(int tval, u32b resf) {
	long i, n, p, adj;
	long k_idx;

	/* Get the entry */
	alloc_entry *restrict table = alloc_kind_table;

	/* Copy the hook into a local variable for speed */
	int (*hook)(int k_idx, u32b resf) = get_obj_num_hook;

	/* Scan the allocation table */
	for (i = 0, n = alloc_kind_size; i < n; i++) {
		/* Get the entry */
		alloc_entry *entry = &table[i];

		/* Obtain the base probability */
		p = entry->prob1;

		/* Default probability for this pass */
		entry->prob2 = 0;

		/* Access the index */
		k_idx = entry->index;

		/* Call the hook and adjust the probability */
		if (hook) {
			adj = (*hook)(k_idx, resf);
			p = (adj * p) / 1000;
		}

		if (p && (resf & RESF_STOREFLAT)) p = 100;

		/* Only accept a specific tval */
		if (k_info[table[i].index].tval != tval)
			continue;

		/* Save the probability */
		entry->prob2 = p;
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
s16b get_obj_num(int max_level, u32b resf) {
	long		i, j, n, p;
	long		value, total;
	long		k_idx;

	object_kind	*k_ptr;
	alloc_entry	*restrict table = alloc_kind_table;


	/* Boost level */
	if (max_level > 0) {
		/* Occasional "boost" */
		if (rand_int(GREAT_OBJ) == 0) {
			/* What a bizarre calculation */
                        max_level = 1 + ((max_level * MAX_DEPTH_OBJ) / randint(MAX_DEPTH_OBJ));
		}
	}

	/* Reset total */
	total = 0L;

	/* Cap maximum level */
	if (max_level > 254) max_level = 254;
	if ((resf & RESF_STOREFLAT)) max_level = 254;

	/* Calculate loop bounds */
	n = alloc_kind_index_level[max_level + 1];

	/* If we're opening a chest, prevent generating another chest from it */
	if (opening_chest) {
		/* Process probabilities */
		for (i = 0; i < n; i++) {
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
		for (i = 0; i < n; i++) {
			/* Default */
			table[i].prob3 = 0;

			/* Accept */
			table[i].prob3 = table[i].prob2;

			if (table[i].prob3 && (resf & RESF_STOREFLAT)) table[i].prob3 = 100;

			/* Total */
			total += table[i].prob3;
		}
	}

	/* No legal objects */
	if (total <= 0) return (0);


	/* Pick an object */
	value = rand_int(total);

	/* Find the object */
	for (i = 0; i < n; i++) {
		/* Found the entry */
		if (value < table[i].prob3) break;

		/* Decrement */
		value = value - table[i].prob3;
	}

	/* don't try for a better object? */
	if ((resf & RESF_STOREFLAT)) return (table[i].index);

	/* Power boost */
	p = rand_int(100);

	/* Try for a "better" object once (50%) or twice (10%) */
	if (p < 60) {
		/* Save old */
		j = i;

		/* Pick a object */
		value = rand_int(total);

		/* Find the object */
		for (i = 0; i < n; i++) {
			/* Found the entry */
			if (value < table[i].prob3) break;

			/* Decrement */
			value = value - table[i].prob3;
		}

		/* Keep the "best" one */
		if (table[i].level < table[j].level) i = j;
	}

	/* Try for a "better" object twice (10%) */
	if (p < 10) {
		/* Save old */
		j = i;

		/* Pick a object */
		value = rand_int(total);

		/* Find the object */
		for (i = 0; i < n; i++) {
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
void object_known(object_type *o_ptr) {
	/* Remove "default inscriptions" */
	if (o_ptr->note && (o_ptr->ident & ID_SENSE)) {
		/* Access the inscription */
		cptr q = quark_str(o_ptr->note);

		/* Hack -- Remove auto-inscriptions */
		if ((streq(q, "bad")) ||
		    //(streq(q, "cursed")) ||//shouldn't get removed, but can't happen on flavoured items anyway
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
	o_ptr->ident &= ~(ID_SENSE | ID_SENSE_HEAVY);

	/* Clear the "Empty" info */
	o_ptr->ident &= ~ID_EMPTY;
	note_toggle_empty(o_ptr, FALSE);

	/* Now we know about the item */
	o_ptr->ident |= (ID_KNOWN | ID_SENSED_ONCE);

	/* Artifact becomes 'found' status - omg it must already become
	'found' if a player picks it up! That gave headaches! */
	if (true_artifact_p(o_ptr)) handle_art_ipara(o_ptr->name1);

}




/*
 * The player is now aware of the effects of the given object.
 */
bool object_aware(int Ind, object_type *o_ptr) {
	int i;

	if (object_aware_p(Ind, o_ptr)) return FALSE;

	/* Fully aware of the effects */
	Players[Ind]->obj_aware[o_ptr->k_idx] = TRUE;

	/* Make it refresh, although the object mem structure didn't change */
	for (i = 0; i < INVEN_TOTAL; i++)
		if (Players[Ind]->inventory[i].k_idx == o_ptr->k_idx)
			Players[Ind]->inventory[i].changed = !Players[Ind]->inventory[i].changed;
	return TRUE;
}



/*
 * Something has been "sampled"
 */
void object_tried(int Ind, object_type *o_ptr, bool flipped) {
	int i;

	if (object_tried_p(Ind, o_ptr)) return;

	/* Mark it as tried (even if "aware") */
	Players[Ind]->obj_tried[o_ptr->k_idx] = TRUE;

	/* Make it refresh, although the object mem structure didn't change */
	if (flipped) return; /* already changed by object_aware()? don't cancel out! */
	for (i = 0; i < INVEN_TOTAL; i++)
		if (Players[Ind]->inventory[i].k_idx == o_ptr->k_idx)
			Players[Ind]->inventory[i].changed = !Players[Ind]->inventory[i].changed;

	return;
}



/*
 * Return the "value" of an "unknown" item
 * Make a guess at the value of non-aware items
 */
static s64b object_value_base(int Ind, object_type *o_ptr) {
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Aware item -- use template cost */
	if (Ind == 0 || object_aware_p(Ind, o_ptr)) return (k_ptr->cost);

	/* Analyze the type */
	switch (o_ptr->tval) {
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

void eliminate_common_ego_flags(object_type *o_ptr, u32b *f1, u32b *f2, u32b *f3, u32b *f4, u32b *f5, u32b *f6, u32b *esp) {
	s16b j;
	ego_item_type *e_ptr;
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Hack -- eliminate Base object powers */
	(*f1) &= ~k_ptr->flags1;
	(*f2) &= ~k_ptr->flags2;
	(*f3) &= ~k_ptr->flags3;
	(*f4) &= ~k_ptr->flags4;
	(*f5) &= ~k_ptr->flags5;
	(*f6) &= ~k_ptr->flags6;
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
			*(f6) &= ~e_ptr->flags6[j];
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
			*(f6) &= ~e_ptr->flags6[j];
			*(esp) &= ~e_ptr->esp[j];
		}
	}
}

/* Return the value of the flags the object has... */
s32b flag_cost(object_type *o_ptr, int plusses) {
	s32b total = 0; //, am;
	u32b f1, f2, f3, f4, f5, f6, esp;


	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	if (f5 & TR5_TEMPORARY) return 0;
	//if (f4 & TR4_CURSE_NO_DROP) return 0;

	/* Hack - This shouldn't be here, still.. */
	eliminate_common_ego_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);


	if (f3 & TR3_WRAITH) total += 250000;
	if (f5 & TR5_INVIS) total += 30000;
	if (!(f4 & TR4_FUEL_LITE)) {
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
	//if (f5 & TR5_SENS_FIRE) total -= 100;
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
	if (esp & ESP_SPIDER) total += 2000;
	//if (esp) total += (12500 * count_bits(esp));
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
	if (f3 & TR3_TELEPORT) {
		if (o_ptr->ident & ID_CURSED)
			total -= 7500;
		else
			total += 500;
	}
	//if (f3 & TR3_AGGRAVATE) total -= 10000; /* penalty 1 of 2 */
	if (f3 & TR3_BLESSED) total += 750;
	if (f3 & TR3_CURSED) total -= 5000;
	if (f3 & TR3_HEAVY_CURSE) total -= 12500;
	if (f3 & TR3_PERMA_CURSE) total -= 15000;
	if (f3 & TR3_FEATHER) total += 1250;

	if (f4 & TR4_LEVITATE) total += 10000;
	if (f4 & TR4_NEVER_BLOW) total -= 15000;
	if (f4 & TR4_PRECOGNITION) total += 250000;
	if (f4 & TR4_BLACK_BREATH) total -= 12500;
	if (f4 & TR4_DG_CURSE) total -= 25000;
	if (f4 & TR4_CLONE) total -= 10000;
	//if (f5 & TR5_LEVELS) total += o_ptr->elevel * 2000;

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
/* Magic devices: Ego powers multiply the price instead of adding? */
#define EGO_MDEV_FACTOR
/* Ego power value overrides negative enchantments?
   This is already done for NEW_SHIELDS_NO_AC below; it means that items such
   as [3,-4] armour can still return a value > 0 thanks to its ego power,
   preventing it from pseudo-iding as 'worthless'. (This hack is required for
   ego powers that don't have a pval.) */
#define EGO_VS_ENCHANTS
/* Price boosts based on enchants, hit/dam (max +30) and ac (max +35): */
#define PB1 9
#define PB2 6
#ifdef TO_AC_CAP_30
 #define PBA1 9
 #define PBA2 6
#else
 #define PBA1 14
 #define PBA2 6
#endif
s64b object_value_real(int Ind, object_type *o_ptr) {
	u32b f1, f2, f3, f4, f5, f6, esp;
	object_kind *k_ptr = &k_info[o_ptr->k_idx];
	bool star = (Ind == 0 || object_fully_known_p(Ind, o_ptr));

	/* Base cost */
	s64b value = k_ptr->cost;
	int i;

	/* Hack -- "worthless" items */
	if (!value) return (0L);

	/* hack: ammo used by newbies, that wasn't level0'ed */
	if (o_ptr->xtra9 == 1 && is_ammo(o_ptr->tval)) return 0L;

	/* Sigil (ignore it) */
	s32b temp_sigil = o_ptr->sigil;
	s32b temp_sseed = o_ptr->sseed;
	o_ptr->sigil = 0;
	o_ptr->sseed = 0;

	/* Extract some flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	/* Sigil (restore it) */
	o_ptr->sigil = temp_sigil;
	o_ptr->sseed = temp_sseed;

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

#ifdef EGO_MDEV_FACTOR
			/* Hack: Ego magic devices cost a multiple of their base price */
			if (is_magic_device(o_ptr->tval)) {
				/* Hack -- Reward the ego-item with a bonus */
				value += e_ptr->cost / 2;

				/* these egos are defined to multiply the price.. */
				switch (o_ptr->name2) {
				case EGO_PLENTY:	value = (value * 3) / 2; break;
				case EGO_RSIMPLICITY:	value *= 2; break;
				case EGO_RCHARGING:	value *= 2; break;
				case EGO_RISTARI:	value *= 3; break;
				/* for undefined egos, just add up to the normal ego cost */
				default:		value += e_ptr->cost / 2;
				}
			} else
#endif
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

#ifdef EGO_MDEV_FACTOR
				/* Hack: Ego magic devices cost a multiple of their base price */
				if (is_magic_device(o_ptr->tval)) {
					/* Hack -- Reward the ego-item with a bonus */
					value += e_ptr->cost / 2;

					/* these egos are defined to multiply the price.. */
					switch (o_ptr->name2b) {
					case EGO_PLENTY:	value = (value * 3) / 2; break;
					case EGO_RSIMPLICITY:	value *= 2; break;
					case EGO_RCHARGING:	value *= 2; break;
					case EGO_RISTARI:	value *= 3; break;
					/* for undefined egos, just add up to the normal ego cost */
					default:		value += e_ptr->cost / 2;
					}
				} else
#endif
				/* Hack -- Reward the ego-item with a bonus */
				value += e_ptr->cost;
			}
		}
	}
	/* Hack */
	if (f3 & TR3_AUTO_CURSE) return 0;

	/* Bad items don't sell. Good items with some bad modifiers DO sell ((*defenders*)). -C. Blue */
	switch (o_ptr->tval) {
	case TV_SHIELD:
#if defined(NEW_SHIELDS_NO_AC) || defined(EGO_VS_ENCHANTS)
		/* Shields of Preservation won't sell anymore without this exception,
		   because they don't have any positive stat left (o_ptr->to_a was it before). */
		if ((((o_ptr->to_h) < 0 && ((o_ptr->to_h - k_ptr->to_h) < 0)) ||
		    ((o_ptr->to_d) < 0 && ((o_ptr->to_d - k_ptr->to_d) < 0 || k_ptr->to_d < 0)) ||
		    ((o_ptr->to_a) < 0 && ((o_ptr->to_a - k_ptr->to_a) < 0 || k_ptr->to_a < 0)) ||
		    (o_ptr->pval < 0) || (o_ptr->bpval < 0)) &&
		    !(((o_ptr->to_h) > 0) ||
		    ((o_ptr->to_d) > 0) ||
		    (value > k_ptr->cost) || /* <- hack: check for ego power value - this is the exception required for shields now */
		    (o_ptr->pval > 0) || (o_ptr->bpval > 0))) return (0L);
	        break;
#endif
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
	case TV_CROWN:
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
#ifdef EGO_VS_ENCHANTS
		    (value > k_ptr->cost) || /* <- hack: check for ego power value - can prevent a negatively enchanted item from conning as 'worthless' */
#endif
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
#ifdef EGO_VS_ENCHANTS
		    (value > k_ptr->cost) || /* <- hack: check for ego power value - can prevent a negatively enchanted item from conning as 'worthless' */
#endif
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

			/* Don't use bpval of costumes - mikaelh */
			if ((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_COSTUME))
				pval = 0;

//			int boost = 1U << pval;

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
				//if (f1 & TR1_STR) value += (pval * 200L);
				//if (f1 & TR1_STR) value += (boost * 200L);
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
				    ) {
					count /= 2;
					if (count) value += count * PRICE_BOOST((count + pval), 2, 1) * 300L;
				/* hack for digging tools too.. */
				} else if (o_ptr->tval == TV_DIGGING) {
					if (count) value += count * PRICE_BOOST((count + pval), 2, 1) * 400L;
				} else {
					if (count) value += count * PRICE_BOOST((count + pval), 2, 1) * 200L;
				}

//				if (f5 & (TR5_CRIT)) value += (PRICE_BOOST(pval, 0, 1)* 300L);//was 500, then 400
//				if (f5 & (TR5_CRIT)) value += pval * pval * 5000L;/* was 20k, but speed is only 10k */
				if (f5 & (TR5_CRIT)) value += (pval + 2) * (pval + 2) * 1500L;/* was 20k, but speed is only 10k */
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
					if (f1 & TR1_BLOWS) value += pval * (pval + 2) * 5000L;
				}

				/* Give credit for extra casting */
				if (f1 & TR1_SPELL) value += (PRICE_BOOST(pval, 0, 1) * 4000L);

				/* Give credit for extra HP bonus */
				if (f1 & TR1_LIFE) value += (PRICE_BOOST(pval, 0, 1) * 3000L);


				/* Flags moved here exclusively from flag_cost */
			        if (f1 & TR1_MANA) value += (200 * pval * (pval + 5));

				/* End of flags, moved here from flag_cost */


				/* Hack -- amulets of speed and rings of speed are
				 * cheaper than other items of speed.
				 */
				if (o_ptr->tval == TV_AMULET) {
					/* Give credit for speed bonus */
					//if (f1 & TR1_SPEED) value += (boost * 25000L);
					if (f1 & TR1_SPEED) value += pval * pval * 5000L;
				} else if (o_ptr->tval == TV_RING) {
					/* Give credit for speed bonus */
					//if (f1 & TR1_SPEED) value += (PRICE_BOOST(pval, 0, 4) * 50000L);
					if (f1 & TR1_SPEED) value += pval * pval * 10000L;
//					if (f1 & TR1_SPEED) value += pval * pval * 7000L;
				}
				/* randarts and speed boots */
//				else if (f1 & TR1_SPEED) value += (PRICE_BOOST(pval, 0, 4) * 100000L);
//				else if (f1 & TR1_SPEED) value += pval * pval * 10000L;
				else if (f1 & TR1_SPEED) value += (pval + 1) * (pval + 1) * 6000L;//7000 -> //5000

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
			byte skill_level = 0;
			if (o_ptr->pval < max_spells) {
				skill_level = school_spells[o_ptr->pval].skill_level;
			}
			int sl = skill_level + 5,
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
#ifdef NEW_MDEV_STACKING
		value += ((value / 20) * o_ptr->pval);
#else
		value += ((value / 20) * o_ptr->pval) / o_ptr->number;
#endif

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

		/* keep consistent with store.c: price_item():
		   This price will be the store-sells price so it must be higher than the store-buys price there. */
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH)) {
			if (o_ptr->pval != 0) {
				monster_race *r_ptr = &r_info[o_ptr->pval];
				int r_val = (r_ptr->level * r_ptr->mexp + (120 + (r_ptr->speed - 90) * 4) * (50000 / ((50000 / (r_ptr->hdice * r_ptr->hside)) + 20))) / 3; /* mimic-like HP calc */

				value += (r_val >= r_ptr->level * 100) ? r_val : r_ptr->level * 100;
			}
		}

		/* Give credit for bonuses */
//		value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);
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
//		value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);
		/* Ignore base boni that come from k_info.txt (eg quarterstaff +10 AC) */
		value += (  ((o_ptr->to_h <= 0 || o_ptr->to_h <= k_ptr->to_h)? 0 :
			    ((k_ptr->to_h < 0)? PRICE_BOOST(o_ptr->to_h, PB1, PB2):
			    PRICE_BOOST((o_ptr->to_h - k_ptr->to_h), PB1, PB2))) +
			    ((o_ptr->to_d <= 0 || o_ptr->to_d <= k_ptr->to_d)? 0 :
			    ((k_ptr->to_d < 0)? PRICE_BOOST(o_ptr->to_d, PB1, PB2):
			    PRICE_BOOST((o_ptr->to_d - k_ptr->to_d), PB1, PB2))) +
			    ((o_ptr->to_a <= 0 || o_ptr->to_a <= k_ptr->to_a)? 0 :
			    ((k_ptr->to_a < 0)? PRICE_BOOST(o_ptr->to_a, PBA1, PBA2):
			    PRICE_BOOST((o_ptr->to_a - k_ptr->to_a), PBA1, PBA2))) ) * 100L;

		/* Costumes */
		if ((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_COSTUME))
			value += r_info[o_ptr->bpval].mexp / 10;

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
//		value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);
		/* Ignore base boni that come from k_info.txt (eg quarterstaff +10 AC) */
		value += (  ((o_ptr->to_h <= 0 || o_ptr->to_h <= k_ptr->to_h)? 0 :
			    ((k_ptr->to_h < 0)? PRICE_BOOST(o_ptr->to_h, PB1, PB2):
			    PRICE_BOOST((o_ptr->to_h - k_ptr->to_h), PB1, PB2))) +
			    ((o_ptr->to_d <= 0 || o_ptr->to_d <= k_ptr->to_d)? 0 :
			    ((k_ptr->to_d < 0)? PRICE_BOOST(o_ptr->to_d, PB1, PB2):
			    PRICE_BOOST((o_ptr->to_d - k_ptr->to_d), PB1, PB2))) +
			    ((o_ptr->to_a <= 0 || o_ptr->to_a <= k_ptr->to_a)? 0 :
			    ((k_ptr->to_a < 0)? PRICE_BOOST(o_ptr->to_a, PBA1, PBA2):
			    PRICE_BOOST((o_ptr->to_a - k_ptr->to_a), PBA1, PBA2))) ) * 100L;

		/* Hack -- Factor in extra damage dice */
		if ((i = o_ptr->dd * (o_ptr->ds + 1) - k_ptr->dd * (k_ptr->ds + 1)))
			value += i * i * i;

		/* Done */
		break;

	/* Ammo */
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
		/* Hack -- negative hit/damage bonuses */
//		if (o_ptr->to_h + o_ptr->to_d < 0) return (0L);

		/* Factor in the bonuses */
//		value += ((o_ptr->to_h + o_ptr->to_d) * 5L);
		/* Ignore base boni that come from k_info.txt (eg quarterstaff +10 AC) */
		value += (  ((o_ptr->to_h <= 0 || o_ptr->to_h <= k_ptr->to_h)? 0 :
			    ((k_ptr->to_h < 0)? PRICE_BOOST(o_ptr->to_h, PB1, PB2):
			    PRICE_BOOST((o_ptr->to_h - k_ptr->to_h), PB1, PB2))) +
			    ((o_ptr->to_d <= 0 || o_ptr->to_d <= k_ptr->to_d)? 0 :
			    ((k_ptr->to_d < 0)? PRICE_BOOST(o_ptr->to_d, PB1, PB2):
			    PRICE_BOOST((o_ptr->to_d - k_ptr->to_d), PB1, PB2)))  ) * 5L;

		/* Hack -- Factor in extra damage dice */
		if ((i = o_ptr->dd * (o_ptr->ds + 1) - k_ptr->dd * (k_ptr->ds + 1)))
			value += i * 5000L;

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

	/* hack for Ethereal ammunition */
	if (o_ptr->name2 == EGO_ETHEREAL || o_ptr->name2b == EGO_ETHEREAL)
		value *= 3; /* in theory 1 eth = 10 normal ammo, but this is appropriate */

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
	u32b f1, f2, f3, f4, f5, f6, esp;
	int res_amass = 0, res_base, imms = 0;

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	if (f5 & TR5_TEMPORARY) return 0;

	/* Hack - This shouldn't be here, still.. */
	eliminate_common_ego_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	/* hack: Artifact ammunition actually uses a_ptr->cost.. somewhat inconsistent, sorry */
	if ((o_ptr->name1 == ART_RANDART) && is_ammo(o_ptr->tval)) {
		a_ptr = randart_make(o_ptr);
		return(a_ptr->cost);
	}

	if (f4 & TR4_AUTO_ID) {
		if (o_ptr->tval == TV_GLOVES) total += 350000; //100k
		else total += 200000; //65k
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
	if (f4 & (TR4_MUST2H | TR4_SHOULD2H))
		total += (slay * ((o_ptr->dd * (o_ptr->ds + 1)) + 80)) / 100;
	else
		total += (slay * ((5 * o_ptr->dd * (o_ptr->ds + 1)) + 50)) / 100;

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
		imms++;
	} else if (f2 & TR2_RES_ACID) total += 1500;
	if (f2 & TR2_IM_ELEC) {
		total += 20000;
		res_amass += 2;
		f2 |= TR2_RES_ELEC;
		imms++;
	} else if (f2 & TR2_RES_ELEC) total += 1500;
	if (f2 & TR2_IM_FIRE) {
		total += 27000;
		res_amass += 2;
		f2 |= TR2_RES_FIRE;
		imms++;
	} else if (f2 & TR2_RES_FIRE) total += 1500;
	if (f2 & TR2_IM_COLD) {
		total += 23000;
		res_amass += 2;
		f2 |= TR2_RES_COLD;
		imms++;
	} else if (f2 & TR2_RES_COLD) total += 1500;
	/* count (semi)complete base res as 1up too */
	res_base =
	    (f2 & (TR2_RES_ACID)) ? 1 : 0 +
	    (f2 & (TR2_RES_ELEC)) ? 1 : 0 +
	    (f2 & (TR2_RES_FIRE)) ? 1 : 0 +
	    (f2 & (TR2_RES_COLD)) ? 1 : 0;
	if (res_base == 3) res_amass++;
	else if (res_base == 4) res_amass += 2;

	/* immunities on armour are worth more than on weapons */
	/* also, double-immunity is especially valuable */
	if (is_armour(o_ptr->tval)) {
		total += imms * 10000;
		if (imms >= 2) total += 40000; /* can't have more than 2 immunities on a randart anyway */
	} else if (imms >= 2) total += 40000; /* can't have more than 2 immunities on a randart anyway */

	if (f2 & TR2_RES_POIS) {
		total += 9000;
		res_amass++;
	}
	if (f2 & TR2_RES_FEAR) total += 2000;
	if (f2 & TR2_RES_LITE) total += 3000;
	if (f2 & TR2_RES_DARK) {
		total += 3000;
		res_amass++;
	}
	if (f2 & TR2_RES_BLIND) total += 5000;
	if (f2 & TR2_RES_CONF) total += 3000;
	if (f2 & TR2_RES_SOUND) {
		total += 10000;
		res_amass += 2;
	}
	if (f2 & TR2_RES_SHARDS) {
		total += 6500;
		res_amass++;
	}
	if (f2 & TR2_RES_NETHER) {
		total += 12000;
		res_amass += 2;
	}
	if (f2 & TR2_RES_NEXUS) {
		total += 9000;
		res_amass += 2;
	}
	if (f2 & TR2_RES_CHAOS) {
		total += 13000;
		res_amass += 2;
	}
	if (f2 & TR2_RES_DISEN) {
		total += 15000;
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
	if (esp & ESP_SPIDER) total += 2000;
	if (esp & ESP_ALL) total += 150000;// + 40000; /* hm, extra bonus */
	if (f3 & TR3_SLOW_DIGEST) total += 750;
	if (f3 & TR3_REGEN) total += 3500;
	if (f5 & TR5_REGEN_MANA) total += 3500;
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
		if (o_ptr->ident & ID_CURSED) total -= 7500;
		else total += 500;
	}
//	if (f3 & TR3_AGGRAVATE) total -= 10000; /* penalty 1 of 2 */
	if (f3 & TR3_BLESSED) total += 750;
	if (f3 & TR3_CURSED) total -= 5000;
	if (f3 & TR3_HEAVY_CURSE) total -= 12500;
	if (f3 & TR3_PERMA_CURSE) total -= 50000;
	if (f3 & TR3_FEATHER) total += 1700;

	if (f4 & TR4_LEVITATE) total += 10000;
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
	u32b f1, f2, f3, f4, f5, f6, esp;
	int imms = 0;

	/* this routine treats armour only */
	if ((o_ptr->name1 != ART_RANDART) || !is_armour(o_ptr->tval)) return 0;

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	if (f5 & TR5_TEMPORARY) return 0;

	/* Hack - This shouldn't be here, still.. */
	eliminate_common_ego_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);


	if (f2 & TR2_IM_ACID) {
		total += 6;
		f2 |= TR2_RES_ACID;
		imms++;
	}
	else if (f2 & TR2_RES_ACID) total++;
	if (f2 & TR2_IM_ELEC) {
		total += 6;
		f2 |= TR2_RES_ELEC;
		imms++;
	}
	else if (f2 & TR2_RES_ELEC) total++;
	if (f2 & TR2_IM_FIRE) {
		total += 6;
		f2 |= TR2_RES_FIRE;
		imms++;
	}
	else if (f2 & TR2_RES_FIRE) total++;
	if (f2 & TR2_IM_COLD) {
		total += 6;
		f2 |= TR2_RES_COLD;
		imms++;
	}
	else if (f2 & TR2_RES_COLD) total++;
	/* count complete base res as 1up too */
	if ((f2 & (TR2_RES_ACID | TR2_RES_ELEC | TR2_RES_FIRE | TR2_RES_COLD))
	    == (TR2_RES_ACID | TR2_RES_ELEC | TR2_RES_FIRE | TR2_RES_COLD))
		total += 4;

	/* already done in artifact_value_real, but ALSO here, since immunities are even much more essential on armour than on weapons: */
	/* double-immunity is especially great */
	if (imms >= 2) total += 6;

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
	if (f4 & TR4_LEVITATE) total += 2;
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

/* Return rating for especially useful weapons, that means a
   weapon that has top +hit and especially +dam, and at the
   same time useful damage mods such as EA/Vamp/Crit. - C. Blue */
static int artifact_flag_rating_weapon(object_type *o_ptr) {
	s32b total = 0;
	u32b f1, f2, f3, f4, f5, f6, esp;
	int slay = 0;

	/* this routine treats armour only */
	if ((o_ptr->name1 != ART_RANDART) || !is_melee_weapon(o_ptr->tval)) return 0;

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	if (f5 & TR5_TEMPORARY) return 0;
	if (f4 & TR4_NEVER_BLOW) return 0;

	/* Hack - This shouldn't be here, still.. */
	eliminate_common_ego_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);


	/* Too useful to ignore, but not really helping damage */
	if (f2 & TR2_IM_ACID) total++;
	if (f2 & TR2_IM_ELEC) total++;
	if (f2 & TR2_IM_FIRE) total++;
	if (f2 & TR2_IM_COLD) total++;
	if (esp & ESP_ALL) total++;

	/* 'The' weapon mods */
	if (f1 & TR1_VAMPIRIC) total += 3;
	//if (f1 & TR1_BLOWS) total += o_ptr->pval * 2;
	if (f1 & TR1_BLOWS) total += (o_ptr->pval * (o_ptr->pval + 1));
	if (f5 & TR5_CRIT) total += (o_ptr->pval + 5) / 3;
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
	if (f4 & (TR4_MUST2H | TR4_SHOULD2H))
		total += (slay * ((o_ptr->dd * (o_ptr->ds + 1)) + 70)) / 80;
	else
		total += (slay * ((o_ptr->dd * (o_ptr->ds + 1)) + 30)) / 35;

	/* for randart Dark Swords */
	if (f4 & TR4_ANTIMAGIC_50) {
		/* more AM is valuable for compensating +hit,+dam. */
		slay = 15; /* abuse 'slay' */
		if (f4 & TR4_ANTIMAGIC_30) slay += 30;
		if (f4 & TR4_ANTIMAGIC_20) slay += 20;
		if (f4 & TR4_ANTIMAGIC_10) slay += 10;
		/* 10 base, 20x, +0, +6  , +16  , +30  , +48  , +70   */
		/* 15 base, 20x, +0, +9.5, +22.5, +37.5, +57.5, +81.5 */
		total += (20 * slay * slay) - 3000;
	}


	if (f4 & TR4_CLONE) total >>= 1;

	return total;
}

/* Return a sensible pricing for randarts, which
   gets added to k_info base item price - C. Blue */
s64b artifact_value_real(int Ind, object_type *o_ptr) {
	u32b f1, f2, f3, f4, f5, f6, esp;
	object_kind *k_ptr = &k_info[o_ptr->k_idx];
	bool star = (Ind == 0 || object_fully_known_p(Ind, o_ptr));

	/* Base cost */
	s64b value = k_ptr->cost;
#ifdef RANDART_PRICE_BONUS
	/* Generate three different values, pick the highest one */
	s64b vx = 0, v1 = 0, v2 = 0;
	int x = 0;
#endif
	int i;
	/* Hack -- "worthless" items */
	if (!value) return (0L);

	/* Extract some flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	/* Artifact */
	if (o_ptr->name1) {
		artifact_type *a_ptr;

		/* Randarts */
		if (o_ptr->name1 == ART_RANDART) {
			a_ptr = randart_make(o_ptr);
			if (a_ptr->flags3 & TR3_AUTO_CURSE) return (0L);
			else {
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
			return (value);
		}

		/* Hack -- "worthless" artifacts */
		if (!value) return (0L);

		/* Hack -- Use the artifact cost instead */
		//value = a_ptr->cost;
	}

	/* Bad items don't sell. Good items with some bad modifiers DO sell ((*defenders*)). -C. Blue */
	// ^ outdated comment, since this function is now for random artifacts only; adjustment is not
	//   needed however, since non-cursed randarts nowadays shouldn't have any negative enchantments anyway!
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
		    //(o_ptr->pval < 0) || (o_ptr->bpval < 0)) &&
		    //(o_ptr->pval < 0) || (o_ptr->bpval < k_ptr->pval)) &&
		    //(o_ptr->pval < 0) || (o_ptr->tval != TV_ROD && o_ptr->bpval < k_ptr->pval)) &&
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

		//int boost = 1U << pval;

		/* Hack -- Negative "pval" is always bad */
		//if (pval < 0) return (0L);

		for (i = 0; i < 2; i++) {
			int count = 0;

			/* No pval */
			//if (!pval)
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
			//if (f1 & TR1_STR) value += (pval * 200L);
			//if (f1 & TR1_STR) value += (boost * 200L);
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
			    (o_ptr->sval == SV_RING_CUNNINGNESS))) {
				count /= 2;
				if (count) value += count * PRICE_BOOST((count + pval), 2, 1) * 300L;
			} else {
				if (count) value += count * PRICE_BOOST((count + pval), 2, 1) * 200L;
			}

			//if (f5 & (TR5_CRIT)) value += (PRICE_BOOST(pval, 0, 1)* 300L);//was 500, then 400
			//if (f5 & (TR5_CRIT)) value += pval * pval * 5000L;/* was 20k, but speed is only 10k */
			if (f5 & (TR5_CRIT)) value += (pval + 2) * (pval + 2) * 1500L;/* was 20k, but speed is only 10k */
			if (f5 & (TR5_LUCK)) value += (PRICE_BOOST(pval, 0, 1)* 10L);

			/* Give credit for stealth and searching */
			//if (f1 & TR1_STEALTH) value += (PRICE_BOOST(pval, 3, 1) * 100L);
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
				//if (f1 & TR1_BLOWS) value += (PRICE_BOOST(pval, 0, 1) * 3000L);
				if (f1 & TR1_BLOWS) value += pval * (pval + 2) * 5000L;
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
				//if (f1 & TR1_SPEED) value += (boost * 25000L);
				if (f1 & TR1_SPEED) value += pval * pval * 5000L;
			} else if (o_ptr->tval == TV_RING) {
				/* Give credit for speed bonus */
				//if (f1 & TR1_SPEED) value += (PRICE_BOOST(pval, 0, 4) * 50000L);
				if (f1 & TR1_SPEED) value += pval * pval * 10000L;
				//if (f1 & TR1_SPEED) value += pval * pval * 7000L;
			}
			/* randarts and speed boots */
			//else if (f1 & TR1_SPEED) value += (PRICE_BOOST(pval, 0, 4) * 100000L);
			//else if (f1 & TR1_SPEED) value += pval * pval * 10000L;
			else if (f1 & TR1_SPEED) value += (pval + 1) * (pval + 1) * 6000L;//7000 -> //5000

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
			byte skill_level = 0;
			if (o_ptr->pval < max_spells) {
				skill_level = school_spells[o_ptr->pval].skill_level;
			}
			int sl = skill_level + 5;
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
#ifdef NEW_MDEV_STACKING
		value += ((value / 20) * o_ptr->pval) / o_ptr->number;
#else
		value += ((value / 20) * o_ptr->pval);
#endif
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
			monster_race *r_ptr = &r_info[o_ptr->pval];
			int r_val = (r_ptr->level * r_ptr->mexp + (120 + (r_ptr->speed - 90) * 4) * (50000 / ((50000 / (r_ptr->hdice * r_ptr->hside)) + 20))) / 3; /* mimic-like HP calc */

			value += (r_val >= r_ptr->level * 100) ? r_val : r_ptr->level * 100;
		}

		/* Give credit for bonuses */
//		value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);
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
		//value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);
		/* Ignore base boni that come from k_info.txt (eg quarterstaff +10 AC) */
		value += (  ((o_ptr->to_h <= 0 || o_ptr->to_h <= k_ptr->to_h)? 0 :
			    ((k_ptr->to_h < 0)? PRICE_BOOST(o_ptr->to_h, PB1, PB2):
			    PRICE_BOOST((o_ptr->to_h - k_ptr->to_h), PB1, PB2))) +
			    ((o_ptr->to_d <= 0 || o_ptr->to_d <= k_ptr->to_d)? 0 :
			    ((k_ptr->to_d < 0)? PRICE_BOOST(o_ptr->to_d, PB1, PB2):
			    PRICE_BOOST((o_ptr->to_d - k_ptr->to_d), PB1, PB2))) +
			    ((o_ptr->to_a <= 0 || o_ptr->to_a <= k_ptr->to_a)? 0 :
			    ((k_ptr->to_a < 0)? PRICE_BOOST(o_ptr->to_a, PBA1, PBA2):
			    PRICE_BOOST((o_ptr->to_a - k_ptr->to_a), PBA1, PBA2))) ) * 100L;

		/* Costumes */
		if ((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_COSTUME))
			value += r_info[o_ptr->bpval].mexp;

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
		//value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);
		/* Ignore base boni that come from k_info.txt (eg quarterstaff +10 AC) */
		value += (  ((o_ptr->to_h <= 0 || o_ptr->to_h <= k_ptr->to_h)? 0 :
			    ((k_ptr->to_h < 0)? PRICE_BOOST(o_ptr->to_h, PB1, PB2):
			    PRICE_BOOST((o_ptr->to_h - k_ptr->to_h), PB1, PB2))) +
			    ((o_ptr->to_d <= 0 || o_ptr->to_d <= k_ptr->to_d)? 0 :
			    ((k_ptr->to_d < 0)? PRICE_BOOST(o_ptr->to_d, PB1, PB2):
			    PRICE_BOOST((o_ptr->to_d - k_ptr->to_d), PB1, PB2))) +
			    ((o_ptr->to_a <= 0 || o_ptr->to_a <= k_ptr->to_a)? 0 :
			    ((k_ptr->to_a < 0)? PRICE_BOOST(o_ptr->to_a, PBA1, PBA2):
			    PRICE_BOOST((o_ptr->to_a - k_ptr->to_a), PBA1, PBA2))) ) * 100L;

		/* Hack -- Factor in extra damage dice */
		if ((i = o_ptr->dd * (o_ptr->ds + 1) - k_ptr->dd * (k_ptr->ds + 1)))
			value += i * i * i;

		/* Done */
		break;

	/* Ammo */
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
		/* Hack -- negative hit/damage bonuses */
		//if (o_ptr->to_h + o_ptr->to_d < 0) return (0L);

		/* Factor in the bonuses */
		//value += ((o_ptr->to_h + o_ptr->to_d) * 5L);
		/* Ignore base boni that come from k_info.txt (eg quarterstaff +10 AC) */
		value += (  ((o_ptr->to_h <= 0 || o_ptr->to_h <= k_ptr->to_h)? 0 :
			    ((k_ptr->to_h < 0)? PRICE_BOOST(o_ptr->to_h, PB1, PB2):
			    PRICE_BOOST((o_ptr->to_h - k_ptr->to_h), PB1, PB2))) +
			    ((o_ptr->to_d <= 0 || o_ptr->to_d <= k_ptr->to_d)? 0 :
			    ((k_ptr->to_d < 0)? PRICE_BOOST(o_ptr->to_d, PB1, PB2):
			    PRICE_BOOST((o_ptr->to_d - k_ptr->to_d), PB1, PB2)))  ) * 5L;

		/* Hack -- Factor in extra damage dice */
		if ((i = o_ptr->dd * (o_ptr->ds + 1) - k_ptr->dd * (k_ptr->ds + 1)))
			value += i * 5000L;

		/* Done */
		break;
	}

#ifdef RANDART_PRICE_BONUS /* just disable in case some randarts end up with outrageous value */
	/* OPTIONAL/EXPERIMENTAL: Add extra bonus for ranged weapon that has absolute top damage */
	if (o_ptr->tval == TV_BOW && (i = o_ptr->to_h + o_ptr->to_d * 2) >= 60) {
 #if 1
		int ultraboost = i * (o_ptr->to_h + 10);
		/* boost excessively for 'end game' dam/usability */
		if ((f3 & TR3_XTRA_SHOTS) && (f3 & TR3_XTRA_MIGHT) &&
		    o_ptr->to_d >= 24) value += (o_ptr->to_d - 23) * ultraboost * 10;
 #endif

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
	else if (is_melee_weapon(o_ptr->tval) && (o_ptr->to_h + o_ptr->to_d * 2) >= 60) {
		int kill = 0;

		/* first bonus prefers weapons with high flag rating */
		i = artifact_flag_rating_weapon(o_ptr) * 4;
		if (i >= 24) {
 #if 1 /* ultraboost for top-end stats? This can result in x2.5 prices for those, up to ~1M Au! */
			int ultraboost = i * (o_ptr->to_h + 10);
			/* boost excessively for 'end game' dam/usability */
			if (o_ptr->to_d > 25) value += (o_ptr->to_d - 25) * ultraboost * 25;
			/* boost even further for being vampiric in addition to being awesome */
			if ((f1 & TR1_VAMPIRIC) && o_ptr->to_d >= 20) value += (o_ptr->to_d - 20) * ultraboost * 15;
 #endif
			/* boost the value for especially good rating */
			v1 = (o_ptr->to_h + o_ptr->to_d * 2 + i - 70) * 2000;
		}

		/* second bonus prefers weapons with just high hit/dam/dice */
		i = o_ptr->to_h + o_ptr->to_d * 2;
		if (i >= 75) {
			int j;

			i = (i - 75) * 2;

			/* extra damage dice? */
			j = (o_ptr->dd * (o_ptr->ds + 1)) - (k_ptr->dd * (k_ptr->ds + 1));
			if (j > 0) i += j;

			v2 = (i + 2) * (i + 2) * 40;
		}

		/* apply the more advantageous bonus */
		if (v2 > v1) v1 = v2;

 #if 1 /* new: boost further by KILL flags? */
		if (f1 & TR1_KILL_DEMON) kill++;
		if (f1 & TR1_KILL_UNDEAD) kill++;
		if (f1 & TR1_KILL_DRAGON) kill++;
		switch (kill) {
		case 1: v1 += (o_ptr->to_h * (o_ptr->to_d + 1)) * 600; //+0..~45k
			break;
		case 2: v1 += (o_ptr->to_h * (o_ptr->to_d + 1)) * 1300; //+0..~95k
			break;
		case 3: v1 += (o_ptr->to_h * (o_ptr->to_d + 1)) * 3000; //+0..~220k
			break;
		}
 #endif

	}

 #if 1 /* new: extra flag price bonus for low-hit/dam arts that otherwise wouldn't get a bonus */
	if (f1 & TR1_BLOWS) x += o_ptr->pval * 4;
	if (f1 & TR1_VAMPIRIC) x += 11;
	if (f5 & TR5_CRIT) x += o_ptr->pval;
	if (x >= 16) vx += 25000;
	if (x >= 20) vx += 25000;
 #endif

	/* apply highest bonus boosted value */
	if (v1 > vx) vx = v1;
	value += vx;
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
s64b object_value(int Ind, object_type *o_ptr) {
	s64b value;

	/* Known items -- acquire the actual value */
	if (Ind == 0 || object_known_p(Ind, o_ptr)) {
		/* Broken items -- worthless */
		if (broken_p(o_ptr)) return (0L);

		/* Cursed items -- worthless */
		if (cursed_p(o_ptr)) return (0L);

		/* Real value (see above) */
		value = object_value_real(Ind, o_ptr);
	}

	/* Unknown items -- acquire a base value */
	else {
		/* Hack -- Felt broken items */
		if ((o_ptr->ident & ID_SENSED_ONCE) && broken_p(o_ptr)) return (0L);

		/* Hack -- Felt cursed items */
		if ((o_ptr->ident & ID_SENSED_ONCE) && cursed_p(o_ptr)) return (0L);

		/* Base value (see above) */
		value = object_value_base(Ind, o_ptr);
	}


	/* Apply discount (if any) */
	if (o_ptr->discount) value -= (value * o_ptr->discount / 100L);

	/* hack: ammo used by newbies, that wasn't level0'ed */
	if (o_ptr->xtra9 == 1 && is_ammo(o_ptr->tval)) return 0L;

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
 * +0x4 - tolerance for discount and inscription
 *        (added for dropping items on top of stacks inside houses)
 * +0x8 - ignore non-matching inscriptions. For player stores, which erase inscriptions on purchase anyway!
 * -- C. Blue
 */
bool object_similar(int Ind, object_type *o_ptr, object_type *j_ptr, s16b tolerance) {
	player_type *p_ptr = NULL;
	int total = o_ptr->number + j_ptr->number;
	bool unknown = !((k_info[o_ptr->k_idx].flags3 & TR3_EASY_KNOW) && (k_info[j_ptr->k_idx].flags3 & TR3_EASY_KNOW))
	    && (!Ind || !object_known_p(Ind, o_ptr) || !object_known_p(Ind, j_ptr));


	/* Hack -- gold always merge */
	if (o_ptr->tval == TV_GOLD && j_ptr->tval == TV_GOLD) {
		/* special exception: pile colour from coin-type monsters is immutable! */
		if (o_ptr->sval != j_ptr->sval && (o_ptr->xtra2 || j_ptr->xtra2)) return FALSE;
		return(TRUE);
	}


	/* Don't EVER stack questors oO */
	if (o_ptr->questor) return FALSE;
	/* Don't ever stack special quest items */
	if (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_QUEST) return FALSE;
	/* Don't stack quest items if not from same quest AND stage! */
	if (o_ptr->quest != j_ptr->quest || o_ptr->quest_stage != j_ptr->quest_stage) return FALSE;


	/* Don't stack potions of blood because of their timeout */
	if ((o_ptr->tval == TV_POTION || o_ptr->tval == TV_FOOD) && o_ptr->timeout) return FALSE;


	/* Require identical object types */
	if (o_ptr->k_idx != j_ptr->k_idx) return (FALSE);

	/* Level 0 items and other items won't merge, since level 0 can't be sold to shops */
	if (!(tolerance & 0x2) &&
	    (!o_ptr->level || !j_ptr->level) &&
	    (o_ptr->level != j_ptr->level))
		return (FALSE);

	/* Require same owner or convertable to same owner */
	/*if (o_ptr->owner != j_ptr->owner) return (FALSE); */
	if (Ind) {
		p_ptr = Players[Ind];
		if (((o_ptr->owner != j_ptr->owner)
		    && ((p_ptr->lev < j_ptr->level)
		    || (j_ptr->level < 1)))
		    && (j_ptr->owner))
			return (FALSE);
		if ((o_ptr->owner != p_ptr->id)
		    && (o_ptr->owner != j_ptr->owner))
			return (FALSE);

		/* Require objects from the same modus! */
		/* A non-everlasting player won't have his items stacked w/ everlasting stuff */
		if (compat_pomode(Ind, j_ptr)) return (FALSE);
	} else {
		/* no stacks of unowned everlasting items in shops after a now-dead
		   everlasting player sold an item to the shop before he died :) */
		if (compat_omode(o_ptr, j_ptr)) return (FALSE);

		/* Hack: Dead owner? Allow to convert to us as owner if our level is high enough */
		if (j_ptr->owner == 65537) {
			if (j_ptr->level > lookup_player_level(o_ptr->owner))
				return (FALSE);
			/* else fall through */
		}
		/* Normal check */
		else if (o_ptr->owner != j_ptr->owner) return (FALSE);
	}

	/* Analyze the items */
	switch (o_ptr->tval) {
		/* quest items */
		case TV_SPECIAL:
			if (o_ptr->sval == SV_QUEST) {
				if ((o_ptr->pval != j_ptr->pval) ||
				    (o_ptr->xtra1 != j_ptr->xtra1) ||
				    (o_ptr->xtra2 != j_ptr->xtra2) ||
				    (o_ptr->weight != j_ptr->weight) ||
				    (o_ptr->quest != j_ptr->quest) ||
				    (o_ptr->quest_stage != j_ptr->quest_stage))
					return FALSE;
				break;
			}
			return FALSE;

		/* Chests */
		case TV_KEY:
		case TV_CHEST:
			/* Never okay */
			return (FALSE);

		/* Food and Potions and Scrolls */
		case TV_SCROLL:
			/* cheques may have different value, so they must not stack */
			if (o_ptr->sval == SV_SCROLL_CHEQUE) return FALSE;
			/* fireworks of different type */
			if (o_ptr->sval == SV_SCROLL_FIREWORK &&
			    (o_ptr->xtra1 != j_ptr->xtra1 ||
			    o_ptr->xtra2 != j_ptr->xtra2))
				return FALSE;
		case TV_FOOD:
		case TV_POTION:
		case TV_POTION2:
			/* Hack for ego foods :) */
			if (o_ptr->name2 != j_ptr->name2) return (FALSE);
			if (o_ptr->name2b != j_ptr->name2b) return (FALSE);

			/* Assume okay */
			break;

		/* Staffs and Wands */
		case TV_WAND:
			/* Require either knowledge or known empty for both wands. */
			if ((!(o_ptr->ident & (ID_EMPTY)) &&
			    (!Ind || !object_known_p(Ind, o_ptr))) ||
			    (!(j_ptr->ident & (ID_EMPTY)) &&
			    (!Ind || !object_known_p(Ind, j_ptr)))) return(FALSE);

			/* Beware artifacts should not combine with "lesser" thing */
			if (o_ptr->name1 != j_ptr->name1) return (FALSE);
			if (!Ind || !p_ptr->stack_allow_wands) return (FALSE);

			/* Do not combine recharged ones with non recharged ones. */
//			if ((f4 & TR4_RECHARGED) != (f14 & TR4_RECHARGED)) return (FALSE);

			/* Do not combine different ego or normal ones */
			if (o_ptr->name2 != j_ptr->name2) return (FALSE);

			/* Do not combine different ego or normal ones */
			if (o_ptr->name2b != j_ptr->name2b) return (FALSE);

			/* Assume okay */
			break;

		case TV_STAFF:
			/* Require knowledge */
			//if (!Ind || !object_known_p(Ind, o_ptr) || !object_known_p(Ind, j_ptr)) return (FALSE);
			if ((!(o_ptr->ident & (ID_EMPTY)) &&
			    (!Ind || !object_known_p(Ind, o_ptr))) ||
			    (!(j_ptr->ident & (ID_EMPTY)) &&
			    (!Ind || !object_known_p(Ind, j_ptr)))) return(FALSE);

			if (o_ptr->name1 != j_ptr->name1) return (FALSE);
			if (!Ind || !p_ptr->stack_allow_wands) return (FALSE);

#ifndef NEW_MDEV_STACKING
			/* Require identical charges */
			if (o_ptr->pval != j_ptr->pval) return (FALSE);
#endif

			if (o_ptr->name2 != j_ptr->name2) return (FALSE);
			if (o_ptr->name2b != j_ptr->name2b) return (FALSE);

			/* Probably okay */
			break;

			/* Fall through */
			/* NO MORE FALLING THROUGH! MUHAHAHA the_sandman */

		/* Staffs and Wands and Rods */
		case TV_ROD:
			/* Overpoweredness, Hello! - the_sandman */
#if 1
			if (o_ptr->sval == SV_ROD_HAVOC) return (FALSE);
#else
			if (o_ptr->sval == SV_ROD_HAVOC && o_ptr->number + j_ptr->number > 3) return (FALSE); //hm
#endif

			/* Require permission */
			if (!Ind || !p_ptr->stack_allow_wands) return (FALSE);
			if (o_ptr->name1 != j_ptr->name1) return (FALSE);

#ifndef NEW_MDEV_STACKING
			/* this is only for rods... the_sandman */
			if (o_ptr->pval == 0 && j_ptr->pval != 0) return (FALSE); //lol :)
#endif

			if (o_ptr->name2 != j_ptr->name2) return (FALSE);
			if (o_ptr->name2b != j_ptr->name2b) return (FALSE);

			/* Probably okay */
			break;

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

		/* Rings, Amulets, Lites */
		case TV_RING:
			/* no more, due to their 'timeout' ! */
			if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH)) return (FALSE);
		case TV_AMULET:
		case TV_LITE:
		case TV_TOOL:
		case TV_BOOK:	/* Books can be 'fireproof' */
			/* custom tomes which appear identical, spell-wise too, may stack */
			if (o_ptr->tval == TV_BOOK && is_custom_tome(o_ptr->sval) &&
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
			if (unknown) return (FALSE);

			/* different bpval? */
			if (o_ptr->bpval != j_ptr->bpval) return(FALSE);

			/* Fall through */

		/* Missiles */
		case TV_BOLT:
		case TV_ARROW:
		case TV_SHOT:
#if 0 /* no, because then if you find a stack of unidentified ammo and you shoot those arrows and pick them up again they won't stack anymore! */
			/* For ammo too! */
			if (unknown) return (FALSE);
#endif

			/* Require identical "boni" -
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
			if (o_ptr->timeout_magic != j_ptr->timeout_magic) return (FALSE);
			if (o_ptr->recharging != j_ptr->recharging) return (FALSE);

			/* Require identical "values" */
			if (o_ptr->ac != j_ptr->ac) return (FALSE);
			if (o_ptr->dd != j_ptr->dd) return (FALSE);
			if (o_ptr->ds != j_ptr->ds) return (FALSE);

			/* Probably okay */
			break;

		case TV_GOLEM:
			if (o_ptr->pval != j_ptr->pval) return(FALSE);
			break;

		/* Various */
		default:
			/* Require knowledge */
			if (!unknown && /* added this just for the EASY_KNOW check it contains */
			    Ind && (!object_known_p(Ind, o_ptr) || !object_known_p(Ind, j_ptr)))
				return (FALSE);

			/* Probably okay */
			break;
	}


	/* Hack -- Require identical "cursed" status */
	if ((o_ptr->ident & ID_CURSED) != (j_ptr->ident & ID_CURSED)) return (FALSE);

	/* Hack -- Require identical "broken" status */
	if ((o_ptr->ident & ID_BROKEN) != (j_ptr->ident & ID_BROKEN)) return (FALSE);

	/* Inscriptions matter not for player-stores (they get erased anyway!) */
	if (!(tolerance & 0x8)) {
		/* Hack -- require semi-matching "inscriptions" */
		/* Hack^2 -- books do merge.. it's to prevent some crashes */
		if (o_ptr->note && j_ptr->note && (o_ptr->note != j_ptr->note)
		    && strcmp(quark_str(o_ptr->note), "on sale")
		    && strcmp(quark_str(j_ptr->note), "on sale")
		    && strcmp(quark_str(o_ptr->note), "stolen")
		    && strcmp(quark_str(j_ptr->note), "stolen")
		    && !is_realm_book(o_ptr)
		    && !check_guard_inscription(o_ptr->note, 'M')
		    && !check_guard_inscription(j_ptr->note, 'M'))
			return (FALSE);

		/* Hack -- normally require matching "inscriptions" */
		if (!(tolerance & 0x4) && (!Ind || !p_ptr->stack_force_notes) && (o_ptr->note != j_ptr->note))
			return (FALSE);
	}

	/* Hack -- normally require matching "discounts" */
	if (!(tolerance & 0x4) && (!Ind || !p_ptr->stack_force_costs) && (o_ptr->discount != j_ptr->discount))
		return (FALSE);


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
 * Allow one item to "absorb" another, assuming they are similar.
 * Note: j_ptr is the 'new' item that gets dropped/added to an existing one.
 */
void object_absorb(int Ind, object_type *o_ptr, object_type *j_ptr) {
	int total = o_ptr->number + j_ptr->number;
#ifndef NEW_MDEV_STACKING
	int onum = o_ptr->number, jnum = j_ptr->number;
#endif

	/* Prepare ammo for possible combining */
	//int o_to_h, o_to_d;
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
#if 0
		/* use 'colour' of the bigger pile */
		if (o_ptr->pval < j_ptr->pval) {
			o_ptr->sval = j_ptr->sval;
			o_ptr->k_idx = j_ptr->k_idx;
		}
		o_ptr->pval += j_ptr->pval;
#else
		o_ptr->pval += j_ptr->pval;
		/* determine new 'colour' depending on the total amount */
		if (j_ptr->xtra1) { /* player-dropped? */
			/* player-dropped piles are compact */
 #ifndef SMELTING /* disable for golem piece smelting feature in project_i(): requires unchanged colour to accumulate the necessary copper/silver (it's ok for gold) */
			o_ptr->k_idx = gold_colour(o_ptr->pval, FALSE, TRUE);
			everyone_lite_spot(&o_ptr->wpos, o_ptr->iy, o_ptr->ix);
 #endif
			o_ptr->sval = k_info[o_ptr->k_idx].sval;
		} else if (!o_ptr->xtra2 && !j_ptr->xtra2) { /* coin-type monster piles don't change type to something else */
			/* standard piles */
			o_ptr->k_idx = gold_colour(o_ptr->pval, TRUE, FALSE);
			o_ptr->sval = k_info[o_ptr->k_idx].sval;
		}
#endif
	}

	/* Hack -- blend "known" status */
	if (Ind && object_known_p(Ind, j_ptr)) object_known(o_ptr);

	/* Hack -- blend "rumour" status */
	if (j_ptr->ident & ID_RUMOUR) o_ptr->ident |= ID_RUMOUR;

	/* Hack -- blend "mental" status */
	if (j_ptr->ident & ID_MENTAL) o_ptr->ident |= ID_MENTAL;

	/* Hack -- could average discounts XXX XXX XXX */
	/* Hack -- save largest discount XXX XXX XXX */
	if (o_ptr->discount < j_ptr->discount) o_ptr->discount = j_ptr->discount;

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
	    (!o_ptr->note || streq(quark_str(o_ptr->note), "handmade") ||
	     (streq(quark_str(o_ptr->note), "stolen") && !streq(quark_str(j_ptr->note), "handmade")))) { /* don't overwrite 'stolen' with 'handmade' */
		o_ptr->note = j_ptr->note;
	}
	else if (merge_inscriptions) {
		if (check_guard_inscription(o_ptr->note, 'M') && (!check_guard_inscription(j_ptr->note, 'M'))
		    && (j_ptr->note) && strcmp(quark_str(j_ptr->note), "handmade") && strcmp(quark_str(j_ptr->note), "stolen"))
			o_ptr->note = j_ptr->note;
	}
	/* hack to fix special case: old item just had an 'on sale' inscription and that doesn't apply anymore */
	if (o_ptr->note && !strcmp(quark_str(o_ptr->note), "on sale") && o_ptr->discount != 50) o_ptr->note = 0;

	/* blend level-req into lower one */
	if (o_ptr->level > j_ptr->level) o_ptr->level = j_ptr->level;

	/* Hack -- if wands are stacking, combine the charges. -LM- */
	if (o_ptr->tval == TV_WAND
#ifdef NEW_MDEV_STACKING
	    || o_ptr->tval == TV_STAFF
#endif
	    )
		o_ptr->pval += j_ptr->pval;

	/* Hack -- if rods are stacking, average out the timeout. the_sandman */
	if (o_ptr->tval == TV_ROD) {
#ifdef NEW_MDEV_STACKING
		o_ptr->pval += j_ptr->pval; //total "uncharge"
		o_ptr->bpval += j_ptr->bpval; //total "fresh rods" counter
#else
		o_ptr->pval = (onum * o_ptr->pval + jnum * j_ptr->pval) / o_ptr->number;
#endif
	}

	/* Hack for dropping items onto stacks that have dead owner */
	if (o_ptr->owner == 65537) o_ptr->owner = j_ptr->owner;
}



/*
 * Find the index of the object_kind with the given tval and sval
 */
s16b lookup_kind(int tval, int sval) {
	int k;

	/* Look for it */
	for (k = 1; k < max_k_idx; k++) {
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
void invwipe(object_type *o_ptr) {
	/* Clear the record */
	WIPE(o_ptr, object_type);
}


/*
 * Make "o_ptr" a "clean" copy of the given "kind" of object
 */
void invcopy(object_type *o_ptr, int k_idx) {
	object_kind *k_ptr = &k_info[k_idx];

	/* Clear the record */
	WIPE(o_ptr, object_type);

	/* Save the kind index */
	o_ptr->k_idx = k_idx;

	/* Efficiency -- tval/sval */
	o_ptr->tval = k_ptr->tval;
	o_ptr->sval = k_ptr->sval;

	/* Default "pval" */
	//o_ptr->pval = k_ptr->pval;
	if (o_ptr->tval == TV_POTION ||
	    o_ptr->tval == TV_POTION2 ||
	    o_ptr->tval == TV_FLASK ||
	    o_ptr->tval == TV_FOOD)
		o_ptr->pval = k_ptr->pval;
	else if (o_ptr->tval == TV_LITE)
		o_ptr->timeout = k_ptr->pval;//hack
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
s16b m_bonus(int max, int level) {
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

static void log_arts(int a_idx, struct worldpos *wpos) {
	switch (a_idx) {
	case ART_DWARVEN_ALE:
		s_printf("ARTIFACT: 'Pint of Ale of the Khazad' created at %d,%d,%d.\n", wpos->wx, wpos->wy, wpos->wz);
		return;
#if 0
	case ART_BILBO:
		s_printf("ARTIFACT: 'Picklock of Bilbo Baggins' created at %d,%d,%d.\n", wpos->wx, wpos->wy, wpos->wz);
		return;
#endif
	case ART_MIRROROFGLORY:
		s_printf("ARTIFACT: 'Mirror of Glory' created at %d,%d,%d.\n", wpos->wx, wpos->wy, wpos->wz);
		return;
	case ART_DREADNOUGHT:
		s_printf("ARTIFACT: 'Dreadnought' created at %d,%d,%d.\n", wpos->wx, wpos->wy, wpos->wz);
		return;
#if 0
	case ART_NARYA:
		s_printf("ARTIFACT: 'Narya' created at %d,%d,%d.\n", wpos->wx, wpos->wy, wpos->wz);
		return;
	case ART_NENYA:
		s_printf("ARTIFACT: 'Nenya' created at %d,%d,%d.\n", wpos->wx, wpos->wy, wpos->wz);
		return;
	case ART_VILYA:
		s_printf("ARTIFACT: 'Vilya' created at %d,%d,%d.\n", wpos->wx, wpos->wy, wpos->wz);
		return;
#endif
#if 0
	default:
		s_printf("ARTIFACT: '%s' created at %d,%d,%d.\n", a_name + a_info[a_idx].name, wpos->wx, wpos->wy, wpos->wz);
		return;
#endif
	}
	return;
}

static bool true_artifact_ood(int aidx, int dlev) {
	artifact_type *a_ptr = &a_info[aidx];
	int d, alev = a_ptr->level;

	/* Enforce minimum "depth" (loosely) (out-of-depth) */
	if (alev > dlev + 5) {
		//low level artifacts up to 30 are especially less restricted; level differences up to 17 are also less restricted
		if ((d = alev - dlev - 5) <= 12 || alev < 30) { //slow linear increase
			if (alev < 30) d = d * 2;
			else d = d * 5; //60% max for 30 vs 1 (should be higher than below min %)
		} else { //steeply growing increase
			d = 100 - (100 * dlev) / alev; //54% min. for 48 vs 30 (should be lower than above max %)
			d = (d * d) / 25; //at double dungeon level, chance to see arts becomes 0..
		}
		if (d > 95) d = 95;
		if (magik(d)) {
s_printf("TRUEART_OOD: %d failed, a %d vs d %d (%d%%)\n", aidx, alev, dlev, d);
			return TRUE;
		}
	}

	/* Skip low-level check for artifacts that are definitely very high level :-p.
	   This is sort of required for super-deep (WINNERS_ONLY) artifacts, since this function
	   actually checks against true dungeon level, not object_level (some sort of average
	   of dungeon level and monster level). So some very high level items will actually
	   require a much higher dungeon level since we usually kill rather lower-level monsters
	   (eg GWoP is only 85), so this reverse-ood-check would kill them all >_<. */
	if (alev >= 76) return FALSE; //arbitrary level cut: skip Ringil, since Angmar is only 70 (for luring purposes =P)

	/* New: Prevent too many low-level artifacts on high dungeon levels (inverse out-of-depth).
	   They tend to spawn very often due to their relatively low rarity,
	   and are simply a small piece of cash, or even annoying if unsellable (Gorlim). */
	/* Don't start checking before dungeon level 36 (arbitrarily, just so Barrow-Downs remains unaffected, pft..) */
	if (alev < dlev - 5 && dlev >= 36) {
		d = 100 - (100 * alev) / dlev;
		d = (d * d) / 25; //at half dungeon level, chance to see arts becomes 0..
		if (d > 95) d = 95;
		if (magik(d)) {
s_printf("TRUEART_R-OOD: %d failed, a %d vs d %d (%d%%)\n", aidx, alev, dlev, d);
			return TRUE;
		}
	}

	/* Allow artifact generation! */
	return FALSE;
}

/*
 * Mega-Hack -- Attempt to create one of the "Special Objects" (INSTA_ART only)
 *
 * We are only called from "place_object()", and we assume that
 * "apply_magic()" is called immediately after we return.
 *
 * Note -- see "make_artifact()" and "apply_magic()"
 */
static bool make_artifact_special(struct worldpos *wpos, object_type *o_ptr, u32b resf) {
	int	i, dlev = getlevel(wpos);
	int	k_idx = 0;
	bool winner_arts_only = ((resf & RESF_NOTRUEART) && (resf & RESF_WINNER));
#ifdef IDDC_EASY_TRUE_ARTIFACTS
	int	difficulty = in_irondeepdive(wpos) ? 1 : 0;
#endif
	artifact_type *a_ptr;
	int im, a_map[MAX_A_IDX];

	/* Check if artifact generation is currently disabled -
	   added this for maintenance reasons -C. Blue */
	if (cfg.arts_disabled ||
	    ((resf & RESF_NOTRUEART) && !(resf & RESF_WINNER)))
		 return (FALSE);

	/* No artifacts in the town */
	if (istown(wpos)) return (FALSE);

	/* shuffle art indices for fairness */
	for (i = 0; i < MAX_A_IDX; i++) a_map[i] = i;
	intshuffle(a_map, MAX_A_IDX);

	/* Check the artifact list (just the "specials") */
	for (im = 0; im < MAX_A_IDX; im++) {
		i = a_map[im];
		a_ptr = &a_info[i];

		/* Skip "empty" artifacts */
		if (!a_ptr->name) continue;

		/* Hack: "Disabled" */
		if (a_ptr->rarity == 255) continue;

		/* Cannot make an artifact twice */
		if (a_ptr->cur_num) continue;

		/* Cannot generate non special ones */
		if (!(a_ptr->flags3 & TR3_INSTA_ART)) continue;

		/* Cannot generate some artifacts because they can only exists in special dungeons/quests/... */
		//if ((a_ptr->flags4 & TR4_SPECIAL_GENE) && (!a_allow_special[i]) && (!vanilla_town)) continue;
		if (a_ptr->flags4 & TR4_SPECIAL_GENE) continue;

		/* Allow non-dropchosen/specialgene winner arts */
		if (winner_arts_only && !(a_ptr->flags5 & TR5_WINNERS_ONLY)) continue;

		/* Sauron-slayers can't find The One Ring anymore */
		if (i == ART_POWER && (resf & RESF_SAURON)) continue;

		/* Artifact "rarity roll" */
#ifdef IDDC_EASY_TRUE_ARTIFACTS
		if (rand_int(a_ptr->rarity >> difficulty) != 0) continue;
#else
		if (rand_int(a_ptr->rarity) != 0) continue;
#endif

		/* enforce minimum/maximum ood */
		if (true_artifact_ood(i, dlev)) continue;

		/* Find the base object */
		k_idx = lookup_kind(a_ptr->tval, a_ptr->sval);

#if 0 /* although this makes level in k_info pointless, it just doesn't make sense to check an insta-art's level twice. \
         NOTE: This also fixes the problem of the k-level differring from the a-level, which is true for a lot of top-levle insta-art jewelry! (ew) */
		/* XXX XXX Enforce minimum "object" level (loosely) */
		if (k_info[k_idx].level > object_level) {
			/* Acquire the "out-of-depth factor" */
			int d = (k_info[k_idx].level - object_level) * 5;

			/* Roll for out-of-depth creation */
			if (rand_int(d) != 0) continue;
		}
#endif

		/* Assign the template */
		invcopy(o_ptr, k_idx);

		/* Mega-Hack -- mark the item as an artifact */
		o_ptr->name1 = i;

		/* Hack -- Mark the artifact as "created" */
		handle_art_inum(o_ptr->name1);

		log_arts(i, wpos);

		/* Success */
		return (TRUE);
	}

	/* Failure */
	return (FALSE);
}


/*
 * Attempt to change an object into an artifact (true arts except for INSTA_ART)
 *
 * This routine should only be called by "apply_magic()"
 *
 * Note -- see "make_artifact_special()" and "apply_magic()"
 */
static bool make_artifact(struct worldpos *wpos, object_type *o_ptr, u32b resf) {
	int i, tries = 0, dlev = getlevel(wpos);
	bool winner_arts_only = ((resf & RESF_NOTRUEART) && (resf & RESF_WINNER));
#ifdef IDDC_EASY_TRUE_ARTIFACTS
	int difficulty = in_irondeepdive(wpos) ? 1 : 0;
#endif
	artifact_type *a_ptr;
	int im, a_map[MAX_A_IDX];

	/* No artifacts in the town, except if it's specifically requested */
	if (istown(wpos) && !(resf & RESF_FORCERANDART)) return (FALSE);

	/* Paranoia -- no "plural" artifacts */
	if (o_ptr->number != 1) return (FALSE);

	/* shuffle art indices for fairness */
	for (i = 0; i < MAX_A_IDX; i++) a_map[i] = i;
	intshuffle(a_map, MAX_A_IDX);

        /* Check if true artifact generation is currently disabled -
	   added this for maintenance reasons -C. Blue */
	if (!cfg.arts_disabled &&
	    !((resf & RESF_NOTRUEART) && !(resf & RESF_WINNER))) {
		/* Check the artifact list (skip the "specials") */
		for (im = 0; im < MAX_A_IDX; im++) {
			i = a_map[im];
			a_ptr = &a_info[i];

			/* Skip "empty" items */
			if (!a_ptr->name) continue;

			/* Hack: "Disabled" */
			if (a_ptr->rarity == 255) continue;

			/* Cannot make an artifact twice */
			if (a_ptr->cur_num) continue;

			/* Cannot generate special ones */
			if (a_ptr->flags3 & TR3_INSTA_ART) continue;

			/* Cannot generate some artifacts because they can only exists in special dungeons/quests/... */
	//		if ((a_ptr->flags4 & TR4_SPECIAL_GENE) && (!a_allow_special[i]) && (!vanilla_town)) continue;
			if (a_ptr->flags4 & TR4_SPECIAL_GENE) continue;

			/* Allow non-dropchosen/specialgene winner arts */
			if (winner_arts_only && !(a_ptr->flags5 & TR5_WINNERS_ONLY))
				continue;

			/* Must have the correct fields */
			if (a_ptr->tval != o_ptr->tval) continue;
			if (a_ptr->sval != o_ptr->sval) continue;

			/* We must make the "rarity roll" */
#ifdef IDDC_EASY_TRUE_ARTIFACTS
			if (rand_int(a_ptr->rarity >> difficulty) != 0) continue;
#else
			if (rand_int(a_ptr->rarity) != 0) continue;
#endif

			/* enforce minimum/maximum ood */
			if (true_artifact_ood(i, dlev)) continue;

			/* Hack -- mark the item as an artifact */
			o_ptr->name1 = i;

			/* Hack -- Mark the artifact as "created" */
			handle_art_inum(o_ptr->name1);

			log_arts(i, wpos);

			/* Success */
			return (TRUE);
		}
	}

	/* Break here if randarts aren't allowed */
	if (resf & RESF_NORANDART) return (FALSE);

	/* An extra chance at being a randart. XXX RANDART */
	if (!rand_int(RANDART_RARITY) || (resf & RESF_FORCERANDART)) {
		/* Randart ammo should be very rare! */
		if (!(resf & RESF_FORCERANDART) && is_ammo(o_ptr->tval) && magik(80)) return(FALSE); /* was 95 */

		/* We turn this item into a randart! */
		o_ptr->name1 = ART_RANDART;

/* NOTE: MAKE SURE FOLLOWING CODE IS CONSISTENT WITH create_artifact_aux() IN spells2.c! */

		/* Start loop. Break when artifact is allowed */
		while (tries < 10) {
			tries++;

			/* Piece together a 32-bit random seed */
			o_ptr->name3 = (u32b)rand_int(0xFFFF) << 16;
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
static bool make_ego_item(int level, object_type *o_ptr, bool good, u32b resf) {
	int i = 0, j, n;
	int *ok_ego, ok_num = 0;
	bool ret = FALSE, double_ok = !(resf & RESF_NODOUBLEEGO);
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
#ifdef NO_MORGUL_IN_IDDC
		if (i == EGO_MORGUL && in_irondeepdive(&o_ptr->wpos)) continue;
#endif

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
		if (e_ptr->fego1[0] & ETR1_NO_SEED) o_ptr->name3 = 0;
		else {
			o_ptr->name3 = (u32b)rand_int(0xFFFF) << 16;
			o_ptr->name3 += rand_int(0xFFFF);
		}

		if ((e_ptr->fego1[0] & ETR1_NO_DOUBLE_EGO)) double_ok = FALSE;

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
	if (ret && double_ok && magik(7) && (!o_ptr->name2b)) {
		/* Now test them a few times */
		for (j = 0; j < ok_num * 10; j++) {
			ego_item_type *e_ptr;

			i = ok_ego[rand_int(ok_num)];
			e_ptr = &e_info[i];

			if (i == EGO_ETHEREAL && (resf & RESF_NOETHEREAL)) continue;
#ifdef NO_MORGUL_IN_IDDC
			if (i == EGO_MORGUL && in_irondeepdive(&o_ptr->wpos)) continue;
#endif
			if ((e_ptr->fego1[0] & ETR1_NO_DOUBLE_EGO)) continue;

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
			if (!(e_ptr->fego1[0] & ETR1_NO_SEED) && !o_ptr->name3) {
				o_ptr->name3 = (u32b)rand_int(0xFFFF) << 16;
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
static void charge_wand(object_type *o_ptr) {
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
static void charge_staff(object_type *o_ptr) {
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
	case SV_STAFF_CURE_SERIOUS:		o_ptr->pval = randint(5)  + 6; break;
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
static void a_m_aux_1(object_type *o_ptr, int level, int power, u32b resf) {
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
					int power[27] = { GF_ELEC, GF_POIS, GF_ACID,
						GF_COLD, GF_FIRE, GF_PLASMA, GF_LITE,
						GF_DARK, GF_SHARDS, GF_SOUND,
						GF_CONFUSION, GF_FORCE, GF_INERTIA,
						GF_MANA, GF_METEOR, GF_ICE, GF_CHAOS,
						GF_NETHER, GF_NEXUS, GF_TIME,
						GF_GRAVITY, GF_KILL_WALL, GF_AWAY_ALL,
						GF_TURN_ALL, GF_NUKE, //GF_STUN,
						GF_DISINTEGRATE, GF_HELL_FIRE };

						//                                o_ptr->pval2 = power[rand_int(25)];
					o_ptr->pval = power[rand_int(27)];
				}
			}
			break;
		case TV_BOOMERANG:
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
static void a_m_aux_2(object_type *o_ptr, int level, int power, u32b resf) {
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

#ifdef NEW_SHIELDS_NO_AC
	/* shields cannot be cursed (aka getting ac malus) or get an ac bonus, if they aren't egos */
	if ((k_info[o_ptr->k_idx].flags3 & TR3_EASY_KNOW)) ;
	else
#endif
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
 #endif
		break;
	case TV_DRAG_ARMOR:
		if (o_ptr->sval == SV_DRAGON_MULTIHUED) {
			/* give 2 random immunities */
			int imm1 = rand_int(5), imm2 = rand_int(4);
			if (imm2 == imm1) imm2 = 4;
			o_ptr->xtra2 |= 0x1 << imm1;
			o_ptr->xtra2 |= 0x1 << imm2;
		}
		break;
	case TV_SOFT_ARMOR:
		/* Costumes */
		if (o_ptr->sval == SV_COSTUME) {
			int i, tries = 0;
			monster_race *r_ptr;

			/* Santa Claus costumes during xmas */
			if (season_xmas) {
				o_ptr->bpval = RI_SANTA1; /* JOKEBAND Santa Claus */
				o_ptr->level = 1;
			} else {
				/* Default to the "player" */
				o_ptr->bpval = 0;
				o_ptr->level = 1;

				while (tries++ != 1000) {
					i = randint(MAX_R_IDX - 2); /* skip 0, ie player, and the 'undefined ghost' (MAX_R_IDX - 1) */
					r_ptr = &r_info[i];

					if (!r_ptr->name) continue;
					if (!r_ptr->rarity) continue;
					// if (r_ptr->flags1 & RF1_UNIQUE) continue;
					// if (r_ptr->level >= level + (power * 5)) continue;
					//if (!mon_allowed(r_ptr)) continue;
					if (!mon_allowed_chance(r_ptr)) continue;

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
//			dragon_resist(o_ptr);
			break;
		}

 #if 1
		/* Set the orcish shield's STR and CON bonus */
		if (o_ptr->sval == SV_ORCISH_SHIELD) {
			o_ptr->bpval = randint(2);

			/* Cursed orcish shield */
			if (power < 0) {
				o_ptr->bpval = -o_ptr->bpval;
  #ifdef NEW_SHIELDS_NO_AC
				/* Cursed (if "bad") */
				o_ptr->ident |= ID_CURSED;
  #endif
			}
			break;
		}
 #endif
 #if 1
	case TV_BOOTS:
		/* Set the Witan Boots stealth penalty */
		if (o_ptr->sval == SV_PAIR_OF_WITAN_BOOTS)
			o_ptr->bpval = -2;
 #endif
	}
#endif

	/* CAP_ITEM_BONI */
#ifdef USE_NEW_SHIELDS  /* should actually be USE_BLOCKING, but could be too */
                        /* dramatic a change if it gets enabled temporarily - C. Blue */
	if (o_ptr->tval == TV_SHIELD) {
 #ifndef NEW_SHIELDS_NO_AC
		if (o_ptr->to_a > 15) o_ptr->to_a = 15;
 #else
		o_ptr->to_a = 0;
 #endif
	} else
#endif
	{
//		if (o_ptr->to_a > 50) o_ptr->to_a = 50;
#ifndef TO_AC_CAP_30
		if (o_ptr->to_a > 35) o_ptr->to_a = 35;
#else
		if (o_ptr->to_a > 30) o_ptr->to_a = 30;
#endif
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
static void a_m_aux_3(object_type *o_ptr, int level, int power, u32b resf) {
	int tries = 0, i;
	artifact_bias = 0;

	if (!power && !(resf & RESF_NO_ENCHANT) && (rand_int(100) < CURSED_JEWELRY_CHANCE)) power = -1;

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
	case TV_RING:
		/* Analyze */
		switch (o_ptr->sval) {
		case SV_RING_POLYMORPH:
			if (power < 1) power = 1;

			/* Be sure to be a player */
			o_ptr->pval = 0;
			o_ptr->timeout_magic = 0;

			if (magik(45)) {
				monster_race *r_ptr;

				while (tries++ != 1000) {
					i = randint(MAX_R_IDX - 2); /* skip 0, ie player and the 'undefined ghost' (MAX_R_IDX - 1) */
					r_ptr = &r_info[i];

					if (!r_ptr->name) continue;
					if (!r_ptr->rarity) continue;
					if (r_ptr->flags1 & RF1_UNIQUE) continue;
					if (r_ptr->level >= level + (power * 5)) continue;
					//if (!mon_allowed(r_ptr)) continue;
					if (!mon_allowed_chance(r_ptr)) continue;

					break;
				}
				if (tries < 1000) {
					o_ptr->pval = i;
					o_ptr->level = ring_of_polymorph_level(r_info[i].level);
					o_ptr->timeout_magic = 3000 + rand_int(3001);
				} else o_ptr->level = 1;
			} else o_ptr->level = 1;
			break;

		/* Strength, Constitution, Dexterity, Intelligence */
		case SV_RING_ATTACKS:
			/* Stat bonus */
			o_ptr->bpval = m_bonus(3, level);
			if (o_ptr->bpval < 1) o_ptr->bpval = 1;

			/* Cursed */
			if (power < 0) {
				/* Cursed */
				o_ptr->ident |= (ID_CURSED);

				/* Reverse bpval */
				o_ptr->bpval = 0 - (o_ptr->bpval);
			}
			break;

		/* Critical hits */
		case SV_RING_CRIT:
			/* Stat bonus */
			o_ptr->bpval = m_bonus(10, level);
			if (o_ptr->bpval < 1) o_ptr->bpval = 1;

			/* Cursed */
			if (power < 0) {
				/* Cursed */
				o_ptr->ident |= (ID_CURSED);

				/* Reverse bpval */
				o_ptr->bpval = 0 - (o_ptr->bpval);
			}
			break;

		case SV_RING_MIGHT:
		case SV_RING_READYWIT:
		case SV_RING_TOUGHNESS:
		case SV_RING_CUNNINGNESS:
			/* Stat bonus */
			o_ptr->bpval = 1 + m_bonus(4, level); /* (5, level) for single-stat rings (traditional) */

			/* Cursed */
			if (power < 0) {
				/* Cursed */
				o_ptr->ident |= (ID_CURSED);

				/* Reverse bpval */
				o_ptr->bpval = 0 - (o_ptr->bpval);
			}
			break;

		case SV_RING_SEARCHING:
		case SV_RING_STEALTH:
			/* Stat bonus */
			o_ptr->bpval = 1 + m_bonus(5, level);

			/* Cursed */
			if (power < 0) {
				/* Cursed */
				o_ptr->ident |= (ID_CURSED);

				/* Reverse bpval */
				o_ptr->bpval = 0 - (o_ptr->bpval);
			}
			break;

		/* Ring of Speed! */
		case SV_RING_SPEED:
			/* Base speed (1 to 10) */
			o_ptr->bpval = randint(5) + m_bonus(5, level);

			/* Super-charge the ring */
			while (rand_int(100) < 50 && o_ptr->bpval < 15) o_ptr->bpval++;

			/* Paranoia - Limit */
			if (o_ptr->bpval > 15) o_ptr->bpval = 15;

			/* Cursed Ring */
			if (power < 0) {
				/* Cursed */
				o_ptr->ident |= (ID_CURSED);

				/* Reverse bpval */
				o_ptr->bpval = 0 - (o_ptr->bpval);

				break;
			}

			break;

		case SV_RING_LORDLY:
#if 0	/* lordly pfft ring.. */
			do {
				random_resistance(o_ptr, FALSE, ((randint(20))+18));
			} while (randint(4) == 1);
#endif

			/* Bonus to armor class */
			o_ptr->to_a = 10 + randint(5) + m_bonus(10, level);
			break;

		/* Flames, Acid, Ice */
		case SV_RING_FLAMES:
		case SV_RING_ACID:
		case SV_RING_ICE:
		case SV_RING_ELEC:
			/* Bonus to armor class */
			o_ptr->to_a = 5 + randint(5) + m_bonus(10, level);
			break;

		/* Weakness, Stupidity */
		case SV_RING_WEAKNESS:
		case SV_RING_STUPIDITY:
			/* Cursed */
			o_ptr->ident |= (ID_CURSED);

			/* Penalize */
			o_ptr->bpval = 0 - (1 + m_bonus(5, level));

			break;

		/* WOE, Stupidity */
		case SV_RING_WOE:
			/* Cursed */
			o_ptr->ident |= (ID_CURSED);

			/* Penalize */
			o_ptr->to_a = 0 - (5 + m_bonus(10, level));
			o_ptr->bpval = 0 - (1 + m_bonus(5, level));
			break;

		/* Ring of damage */
		case SV_RING_DAMAGE:
			/* Bonus to damage */
			o_ptr->to_d = 5 + randint(8) + m_bonus(10, level);

			/* Cursed */
			if (power < 0) {
				/* Cursed */
				o_ptr->ident |= (ID_CURSED);

				/* Reverse bonus */
				o_ptr->to_d = 0 - (o_ptr->to_d);
			}
			break;

		/* Ring of Accuracy */
		case SV_RING_ACCURACY:
			/* Bonus to hit */
//					o_ptr->to_h = 5 + randint(8) + m_bonus(10, level);
			o_ptr->to_h = 10 + rand_int(11) + m_bonus(5, level);

			/* Cursed */
			if (power < 0) {
				/* Cursed */
				o_ptr->ident |= (ID_CURSED);

				/* Reverse tohit */
				o_ptr->to_h = 0 - (o_ptr->to_h);
			}
			break;

		/* Ring of Protection */
		case SV_RING_PROTECTION:
			/* Bonus to armor class */
			o_ptr->to_a = 5 + randint(8) + m_bonus(10, level);

			/* Cursed */
			if (power < 0) {
				/* Cursed */
				o_ptr->ident |= (ID_CURSED);

				/* Reverse toac */
				o_ptr->to_a = 0 - (o_ptr->to_a);
			}

			break;

		/* Ring of Slaying */
		case SV_RING_SLAYING:
			/* Bonus to damage and to hit */
			o_ptr->to_h = 3 + randint(6) + m_bonus(10, level);
			o_ptr->to_d = 3 + randint(5) + m_bonus(9, level);

			/* Cursed */
			if (power < 0) {
				/* Cursed */
				o_ptr->ident |= (ID_CURSED);

				/* Reverse bonuses */
				o_ptr->to_h = 0 - (o_ptr->to_h);
				o_ptr->to_d = 0 - (o_ptr->to_d);
			}
			break;
		}
		break;

	case TV_AMULET:
		/* Analyze */
		switch (o_ptr->sval) {
		/* Old good Mangband ones */
		/* Amulet of Terken -- never cursed */
		case SV_AMULET_TERKEN:
			o_ptr->bpval = randint(5) + m_bonus(5, level);
			//o_ptr->to_h = randint(5);
			//o_ptr->to_d = randint(5);

			/* Sorry.. */
//					o_ptr->xtra1 = EGO_XTRA_ABILITY;
//					o_ptr->xtra2 = randint(256);
			break;

		/* Amulet of the Moon -- never cursed */
		case SV_AMULET_THE_MOON:
			o_ptr->bpval = randint(5) + m_bonus(5, level);
			o_ptr->to_h = randint(5);
			o_ptr->to_d = randint(5);

			// o_ptr->xtra1 = EGO_XTRA_ABILITY;
			//o_ptr->xtra2 = randint(256);
			break;

		/* Amulet of the Magi -- never cursed */
		case SV_AMULET_THE_MAGI:
//					if (randint(3) == 1) o_ptr->art_flags3 |= TR3_SLOW_DIGEST;
		case SV_AMULET_TRICKERY:
		case SV_AMULET_DEVOTION:
			o_ptr->bpval = 1 + m_bonus(3, level);
			break;

		case SV_AMULET_WEAPONMASTERY:
			o_ptr->bpval = 1 + m_bonus(2, level);
			o_ptr->to_a = 1 + m_bonus(4, level);
			o_ptr->to_h = 1 + m_bonus(5, level);
			o_ptr->to_d = 1 + m_bonus(5, level);
			break;

		/* Amulet of wisdom/charisma */
		case SV_AMULET_BRILLIANCE:
		case SV_AMULET_CHARISMA:
		case SV_AMULET_WISDOM:
		case SV_AMULET_INFRA:
			o_ptr->bpval = 1 + m_bonus(5, level);

			/* Cursed */
			if (power < 0) {
				/* Cursed */
				o_ptr->ident |= (ID_CURSED);

				/* Reverse bonuses */
				o_ptr->bpval = 0 - (o_ptr->bpval);
			}

			break;

		/* Amulet of the Serpents */
		case SV_AMULET_SERPENT:
			o_ptr->bpval = 1 + m_bonus(5, level);
			o_ptr->to_a = 1 + m_bonus(6, level);

			/* Cursed */
			if (power < 0) {
				/* Cursed */
				o_ptr->ident |= (ID_CURSED);

				/* Reverse bonuses */
				o_ptr->bpval = 0 - (o_ptr->bpval);
			}

			break;

		case SV_AMULET_NO_MAGIC:
			/* Never cursed - C. Blue */
			break;
		case SV_AMULET_NO_TELE:
			if (power < 0) o_ptr->ident |= (ID_CURSED);
			break;

		case SV_AMULET_RESISTANCE:
#if 0
			if (randint(3) == 1) random_resistance(o_ptr, FALSE, ((randint(34)) + 4));
			if (randint(5) == 1) o_ptr->art_flags2 |= TR2_RES_POIS;
#endif	// 0
		break;

		/* Amulet of searching */
		case SV_AMULET_SEARCHING:
			o_ptr->bpval = randint(5) + m_bonus(5, level);

			/* Cursed */
			if (power < 0) {
				/* Cursed */
				o_ptr->ident |= (ID_CURSED);

				/* Reverse bonuses */
				o_ptr->bpval = 0 - (o_ptr->bpval);
			}

			break;

		/* Amulet of Doom -- always cursed */
		case SV_AMULET_DOOM:
			/* Cursed */
			o_ptr->ident |= (ID_CURSED);

			/* Penalize */
			o_ptr->bpval = 0 - (randint(5) + m_bonus(5, level));
			o_ptr->to_a = 0 - (randint(5) + m_bonus(5, level));

			break;

		/* Amulet of Rage, formerly 'Suspicion' */
		case SV_AMULET_RAGE:
			o_ptr->bpval = 1 + m_bonus(2, level);
			o_ptr->to_a = -1 - m_bonus(13, level);
			o_ptr->to_h = -1 - m_bonus(10, level);
			o_ptr->to_d = 1 + m_bonus(8, level);//was 15,..
			if (rand_int(100) < 33) {
//						o_ptr->xtra1 = EGO_XTRA_POWER;
//						o_ptr->xtra2 = rand_int(255);
			}
			break;

		/* Amulet of speed */
		case SV_AMULET_SPEED:
//			o_ptr->bpval = randint(5);1/2*1/4

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

		/* Talisman (Amulet of Luck) */
		case SV_AMULET_LUCK:
			o_ptr->bpval = magik(40)?randint(3):(magik(40)?randint(4):randint(5));

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


/*
 * Apply magic to an item known to be "boring"
 *
 * Hack -- note the special code for various items
 */
static void a_m_aux_4(object_type *o_ptr, int level, int power, u32b resf) {
	u32b f1, f2, f3, f4, f5, f6, esp;

	/* Very good */
	if (power > 1) {
		/* Make ego item */
		//if (!rand_int(RANDART_JEWEL) && (o_ptr->tval == TV_LITE)) create_artifact(o_ptr, FALSE, TRUE);	else
		make_ego_item(level, o_ptr, TRUE, resf);
	} else if (power < -1) {
		/* Make ego item */
		make_ego_item(level, o_ptr, FALSE, resf);
	}

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	/* Apply magic (good or bad) according to type */
	switch (o_ptr->tval) {
	case TV_BOOK:
		/* Randomize random books */
		if (o_ptr->sval == SV_SPELLBOOK) {
			int  i = 0, tries = 1000;

			while (tries) {
				tries--;

				/* Pick a spell */
				i = rand_int(max_spells);

				/* Only random ones */
				//if (exec_lua(format("return can_spell_random(%d)", i)) == FALSE) continue;

				/* Test if it passes the level check */
				if (rand_int(school_spells[i].skill_level * 3) <= level) {
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

	case TV_LITE:

		o_ptr->to_h = o_ptr->to_d = o_ptr->to_a = 0;

		/* Hack -- Torches -- random fuel */
		if (f4 & TR4_FUEL_LITE) {
			if (o_ptr->sval == SV_LITE_TORCH) {
				//if (o_ptr->pval) o_ptr->pval = randint(o_ptr->pval);
				o_ptr->timeout = randint(FUEL_TORCH);
			}

			/* Hack -- Lanterns -- random fuel */
			else if (o_ptr->sval == SV_LITE_LANTERN) {
				o_ptr->timeout = randint(FUEL_LAMP);
				//if (o_ptr->pval) o_ptr->pval = randint(o_ptr->pval);
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
		switch (o_ptr->sval) {
		case SV_GOLEM_ARM:
			o_ptr->pval = 5 + m_bonus(10, level);
			break;
		case SV_GOLEM_LEG:
			o_ptr->pval = 5 + m_bonus(10, level);
			break;
		}
		break;

	/* Hack -- consider using 'timeout' inplace */
	case TV_ROD:
		/* Hack -- charge rods */
		o_ptr->pval = 0;
		break;

	case TV_SCROLL:
		if (o_ptr->sval == SV_SCROLL_FIREWORK) {
			o_ptr->xtra1 = rand_int(3); //size
			o_ptr->xtra2 = rand_int(7); //colour
			o_ptr->level = 1;
		}
		break;

	case TV_TOOL:
		if (o_ptr->sval == SV_TOOL_PICKLOCK) o_ptr->bpval = randint(3);
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
 * 'wpos' only has effect for calculating 'verygreat' minimum value and for
 * ood checks of true artifacts created here.
 * "verygreat" makes sure that ego items aren't just resist fire etc.
 * Has no influence on artifacts. - C. Blue
 */
void apply_magic(struct worldpos *wpos, object_type *o_ptr, int lev, bool okay, bool good, bool great, bool verygreat, u32b resf) {
	/* usually lev = dungeonlevel (sometimes more, if in vault) */
	object_type forge_bak, forge_highest, forge_lowest;
	object_type *o_ptr_bak = NULL, *o_ptr_highest = &forge_highest;
	object_type *o_ptr_lowest = &forge_lowest;
	bool resf_fallback = TRUE;
	s32b ego_value1, ego_value2, ovr, fc;
	long depth = ABS(getlevel(wpos)), depth_value;
	int i, rolls, chance1, chance2, power; //, j;
        char o_name[ONAME_LEN];
	u32b f1, f2, f3, f4, f5, f6, esp; /* for RESF checks */

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


	if (resf & RESF_NO_ENCHANT) {
		okay = good = great = verygreat = FALSE;
		chance1 = chance2 = 0;
	}


	/* Assume normal */
	power = 0;

	/* Roll for "good" */
	if (good || magik(chance1)) {
		/* Assume "good" */
		power = 1;

		/* Higher chance2 for super heavy armours are already very rare
		   and also for normal mithril/adamantite armour, since they're pretty deep level yet just sell loot. */
		//if (k_info[o_ptr->k_idx].flags6 & TR6_OFTEN_EGO) chance2 += 10;
		if (k_info[o_ptr->k_idx].flags6 & TR6_OFTEN_EGO) chance2 = chance2 / 2 + 23; //(this calc treats non-royal armour especially nice)

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

	/* insta-ego items can never be random artifacts */
	if ((k_info[o_ptr->k_idx].flags6 & TR6_INSTA_EGO)) {
		if (power < 0) power = -2; //cursed ego
		else power = 2; //great ego
		resf &= ~RESF_FORCERANDART;
		resf |= RESF_NORANDART;
	}


	/* Assume no rolls */
	rolls = 0;

	/* Get one roll if excellent */
	if (power >= 2) rolls = 1;

	/* Hack -- Get four rolls if forced great */
	if (great) rolls = 2; // 4

	/* Hack -- Get no rolls if not allowed */
	if (!okay || o_ptr->name1) rolls = 0;


	/* virgin */
	o_ptr->owner = 0;


	/* Hack for possible randarts, to be created in next for loop:
	   Jewelry can keep +hit,+dam,+ac through artifying process!
	   That means, it must be applied before arting it, because the
	   o_ptr->name1 check below will exit apply_magic() via return().
	   Won't affect normal items that fail randart check anyway. ----------------------- */
	if ((o_ptr->tval == TV_RING || o_ptr->tval == TV_AMULET)
	    && !o_ptr->name1) { /* if already art, do not reroll hit/dam/ac! */
		o_ptr_bak = &forge_bak;
		object_copy(o_ptr_bak, o_ptr);
		a_m_aux_3(o_ptr_bak, lev, 1, resf); /* create a good, non-ego version for arting */
	}
	/* --------------------------------------------------------------------------------- */

	if ((resf & RESF_FORCERANDART)) rolls = 2;

	/* Roll for artifacts if allowed */
	for (i = 0; i < rolls; i++) {
		/* Roll for an artifact -
		   on original object, since rings/amulets might already have gotten
		   an ego power from a_m_aux_3() above. */
		if (make_artifact(wpos, o_ptr_bak ? o_ptr_bak : o_ptr, resf)) {
			if (o_ptr_bak) object_copy(o_ptr, o_ptr_bak);
			break;
		}
	}
	/* Hack -- analyze artifacts */
	if (o_ptr->name1) {
		artifact_type *a_ptr;

		/* Randart */
		if (o_ptr->name1 == ART_RANDART) {
			/* generate it finally, after those preparations above */
			a_ptr = randart_make(o_ptr);
		}
		/* Normal artifacts */
		else a_ptr = &a_info[o_ptr->name1];

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
		if (true_artifact_p(o_ptr)) handle_art_inumpara(o_ptr->name1);

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

		//o_ptr->timeout = 0;
		o_ptr->timeout_magic = 0;
		o_ptr->recharging = 0;

		/* Fuelable artifact lights shouldn't always start at 0 energy */
		if (o_ptr->tval == TV_LITE) a_m_aux_4(o_ptr, lev, power, resf);

		/* clear flags from pre-artified item, simulating
		   generation of a brand new object. */
		o_ptr->ident &= ~(ID_MENTAL | ID_BROKEN | ID_CURSED);

		/* Hack -- extract the "broken" flag */
		if (!a_ptr->cost) o_ptr->ident |= ID_BROKEN;

		/* Hack -- extract the "cursed" flag */
		if (a_ptr->flags3 & TR3_CURSED) o_ptr->ident |= ID_CURSED;

		/* Done */
		return;
	} else if ((resf & RESF_FORCERANDART)) {
		invwipe(o_ptr);
		return; /* failed to generate */
	}

	/* Hack - for NO_MORGUL_IN_IDDC check in a_m_aux_1().  - C. Blue
	   (Usually, o_ptr->wpos is only set in drop_near(), which happens _afterwards_.) */
	wpcopy(&o_ptr->wpos, wpos);

	/* In case we get an ego item, check "verygreat" flag and retry a few times if needed */
	if (verygreat) s_printf("verygreat apply_magic:\n");
	/* for other items: */
	o_ptr_bak = &forge_bak;
	object_copy(o_ptr_bak, o_ptr);
	object_copy(o_ptr_highest, o_ptr);

	depth_value = (depth < 60 ? depth * 150 : 9000) + randint(depth) * 100;
	//  for (i = 0; i < (!is_ammo(o_ptr->tval) ? 2 + depth / 7 : 4 + depth / 5); i++) {
	//  for (i = 0; i < (!is_ammo(o_ptr->tval) ? 2 + depth / 5 : 4 + depth / 5); i++) {
	for (i = 0; i < 25; i++) {
		object_copy(o_ptr, o_ptr_bak);

		/* Apply magic */
		switch (o_ptr->tval) {
		case TV_TRAPKIT:
			if (!is_firearm_trapkit(o_ptr->sval)) {
#ifdef TRAPKIT_EGO_ALL
				/* 'boring' trapkit types (ie no +hit/+dam) */
				a_m_aux_4(o_ptr, lev, power, resf);
#endif
				break;
			}
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

#if 1		// tweaked pernA ego..
		/* Hack -- analyze ego-items */
		//else if (o_ptr->name2)
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
			if (a_ptr->to_a < 0) o_ptr->to_a = a_ptr->to_a; /* <- special for 'bad' ego powers, vs high-ac armour such as DSM */
			else o_ptr->to_a += a_ptr->to_a;
			o_ptr->to_h += a_ptr->to_h;
			o_ptr->to_d += a_ptr->to_d;

			apply_enchantment_limits(o_ptr); /* new: paranoia? worked fine without so far */

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
		if (i == 0)
			object_copy(o_ptr_lowest, o_ptr);
		else if (object_value_real(0, o_ptr) < object_value_real(0, o_ptr_lowest))
			object_copy(o_ptr_lowest, o_ptr);

		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
		if ((resf & RESF_LOWVALUE) && (object_value_real(0, o_ptr) > 35000)) continue;
		if ((resf & RESF_MIDVALUE) && (object_value_real(0, o_ptr) > 50000)) continue;
		if ((resf & RESF_NOHIVALUE) && (object_value_real(0, o_ptr) > 100000)) continue;
		if ((resf & RESF_LOWSPEED) && (f1 & TR1_SPEED) && (o_ptr->bpval > 4 || o_ptr->pval > 4)) continue;
		if ((resf & RESF_NOHISPEED) && (f1 & TR1_SPEED) && (o_ptr->bpval > 6 || o_ptr->pval > 6)) continue;

		if (o_ptr->name2) ego_value1 = e_info[o_ptr->name2].cost; else ego_value1 = 0;
		if (o_ptr->name2b) ego_value2 = e_info[o_ptr->name2b].cost; else ego_value2 = 0;
		if ((resf & RESF_EGOHI) && ego_value1 + ego_value2 < 9000) continue;

		/* "verygreat" check: */
		/* 2000 to exclude res, light, reg, etc */
		/* 5000+objval to exclude brands/slays */
		//NO:	if (!verygreat || object_value_real(0, o_ptr) >= 7000) break; <- arrows (+36,+42) -> lol. - C. Blue
		if (!verygreat) break;

		object_desc(0, o_name, o_ptr, FALSE, 3);
		ovr = object_value_real(0, o_ptr);
		fc = flag_cost(o_ptr, o_ptr->pval);

		/* remember most expensive object we rolled, in case we don't find any better we can fallback to it */
		if (ovr > object_value_real(0, o_ptr_highest)) {
			object_copy(o_ptr_highest, o_ptr);
			/* No fallback because of resf necessary */
			resf_fallback = FALSE;
		} else continue;

		s_printf("dpt %ld, dptval %ld, egoval %d / %d, realval %d, flags %d (%s), resf %d\n",
		    depth, depth_value, ego_value1, ego_value2, ovr, fc, o_name, resf);

		if (!is_ammo(o_ptr->tval)) {
			if ((ego_value1 >= depth_value) || (ego_value2 >= depth_value) ||
			    (object_value_real(0, o_ptr) >= depth * 300)) break;
		} else {
			/* Ammo amount is increased in place_object */
			if (object_value_real(0, o_ptr) >= depth + 150) break;
		}
	} /* verygreat-loop end */

	/* verify! (could fail if drop is forced 'great' but only ego power
	   available is a bad one (0 value) or vice versa.. */
	if ((k_info[o_ptr->k_idx].flags6 & TR6_INSTA_EGO) && !o_ptr->name2 && !o_ptr->name2b) {
		invwipe(o_ptr);
		return; /* failed to generate */
	}

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
			s_printf("dpt %ld, dptval %ld, egoval %d / %d, realval %d, flags %d (%s)\n",
			depth, depth_value, ego_value1, ego_value2, ovr, fc, o_name);

			s_printf("taken\n");
		} else {
		        s_printf("taken\n");
			object_copy(o_ptr, o_ptr_highest);
		}
	}
}

/*
 * This 'utter hack' function is to allow item-generation w/o specifing
 * worldpos.
 */
void apply_magic_depth(int Depth, object_type *o_ptr, int lev, bool okay, bool good, bool great, bool verygreat, u32b resf) {
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
void determine_level_req(int level, object_type *o_ptr) {
	int i, j, klev = k_info[o_ptr->k_idx].level, base = klev / 2;
	artifact_type *a_ptr = NULL;


	/* -------------------- Dungeon level hacks -------------------- */


	switch (o_ptr->tval) {
	case TV_RING:
		switch (o_ptr->sval) {
		case SV_RING_SPEED:
	                if (level < 75) level = 75;
		        break;
		case SV_RING_MIGHT:
		case SV_RING_TOUGHNESS:
		case SV_RING_READYWIT:
		case SV_RING_CUNNINGNESS:
			if (level < 25) level = 25;
		        break;
		}
		break;
	case TV_DRAG_ARMOR:
	        if (o_ptr->sval == SV_DRAGON_POWER && level < 100) level = 100;
	        break;
	case TV_POTION:
		switch (o_ptr->sval) {
		case SV_POTION_INC_STR:
		case SV_POTION_INC_INT:
		case SV_POTION_INC_WIS:
		case SV_POTION_INC_DEX:
		case SV_POTION_INC_CON:
		case SV_POTION_INC_CHR:
			if (level < 20) level = 20;
			break;
	        }
	        break;
	}


	/* -------------------- Exceptions -------------------- */


	switch (o_ptr->tval) {
	case TV_RING:
		if (o_ptr->sval == SV_RING_POLYMORPH) {
			o_ptr->level = ring_of_polymorph_level(r_info[o_ptr->pval].level);
			return;
		}
		break;
	case TV_SOFT_ARMOR:
		if (o_ptr->sval == SV_COSTUME) {
			o_ptr->level = 1;
			return;
		}
		break;
	case TV_SCROLL:
		switch (o_ptr->sval) {
		case SV_SCROLL_CHEQUE:
		case SV_SCROLL_FIREWORK:
			o_ptr->level = 1;
			return;
		}
		break;
	case TV_CHEST:
		/* chest level is base for calculating the item level,
		   so it must be like a dungeon level - C. Blue
		   (base = k_info level / 2, level = dungeonlevel usually) */
		o_ptr->level = base + (level * 2) / 4;
		return;
	}

	/* artifact */
	if (o_ptr->name1) {
	 	/* Randart */
		if (o_ptr->name1 == ART_RANDART) {
			a_ptr = randart_make(o_ptr);
			if (a_ptr == (artifact_type*)NULL) {
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


	/* ---------- Base level boost depending on item type ---------- */


	/* stat/heal potions harder to cheeze-transfer */
	if (o_ptr->tval == TV_POTION) {
		switch(o_ptr->sval) {
		case SV_POTION_HEALING:
			base += 15 + 10;
			break;
		case SV_POTION_RESTORE_MANA:
			base += 10 + 10;
			break;
		case SV_POTION_INC_STR:
		case SV_POTION_INC_INT:
		case SV_POTION_INC_WIS:
		case SV_POTION_INC_DEX:
		case SV_POTION_INC_CON:
		case SV_POTION_INC_CHR:
			base += 40 + 30;
			break;
		case SV_POTION_AUGMENTATION:
			base += 45 + 20;
			break;
		case SV_POTION_EXPERIENCE:
			base += 20 + 20;
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
		case SV_AMULET_THE_MAGI:
		case SV_AMULET_DEVOTION:
			base += 16 + o_ptr->bpval * 3;
			break;
		/* These are as drops just too low level */
		case SV_AMULET_WEAPONMASTERY:
		case SV_AMULET_RAGE:
			base += 10;
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

	/* prevent exorbitantly high-level lamp randarts:
	   (base item targets were: dwarven ~20+ , fean ~32+) */
	if (o_ptr->tval == TV_LITE && o_ptr->name1 != ART_RANDART) {
		switch (o_ptr->sval) {
		case SV_LITE_DWARVEN: base += 35; break;
		case SV_LITE_FEANORIAN: base += 55; break;
		default: if (o_ptr->name2) base += 20;
		}
	}

	/* Hack -- analyze ego-items */
	if (o_ptr->name2) {
		i = e_info[o_ptr->name2].rating;
		base += i;

		if (o_ptr->name2b) {
			j = e_info[o_ptr->name2b].rating;
			base += j;
		} else j = 0;

		/* Extremes: Give priority to either bad or especially rare good power */
		if ((j && !e_info[j].cost) || j > i) i = o_ptr->name2b;
		else i = o_ptr->name2;

		/* general level boost for ego items!
		   except very basic ones */
		switch (i) {
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

		case EGO_BRAND_ACID:	case EGO_BRAND_COLD:	case EGO_BRAND_FIRE:
		case EGO_BRAND_ELEC:	case EGO_BRAND_POIS:

		case EGO_SLAY_ANIMAL:	case EGO_SLAY_EVIL:	case EGO_SLAY_UNDEAD:
		case EGO_SLAY_DEMON:	case EGO_SLAY_ORC:	case EGO_SLAY_TROLL:
		case EGO_SLAY_GIANT:	case EGO_SLAY_DRAGON:
			base += 10;
			break;

		case EGO_KILL_EVIL:
		case EGO_HA: /* 'Aman' */
		case EGO_GONDOLIN:
			base += 20;
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


	/* --------------- Adjust, randomize and sanitize --------------- */


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


	/* --------------- Final bottom limits --------------- */


	/* Anti-cheeze hacks */
	if ((o_ptr->tval == TV_POTION) && ( /* potions that mustn't be transferred, otherwise resulting in 1 out-of-line char */
	    (o_ptr->sval == SV_POTION_EXPERIENCE) ||
	    (o_ptr->sval == SV_POTION_LEARNING) ||
	    (o_ptr->sval == SV_POTION_INVULNERABILITY))) o_ptr->level = 0;
	if ((o_ptr->tval == TV_SCROLL) && (o_ptr->sval == SV_SCROLL_TRAP_CREATION) && (o_ptr->level < 20)) o_ptr->level = 20;
	if ((o_ptr->tval == TV_SCROLL) && (o_ptr->sval == SV_SCROLL_FIRE) && (o_ptr->level < 30)) o_ptr->level = 30;
	if ((o_ptr->tval == TV_SCROLL) && (o_ptr->sval == SV_SCROLL_ICE) && (o_ptr->level < 30)) o_ptr->level = 30;
	if ((o_ptr->tval == TV_SCROLL) && (o_ptr->sval == SV_SCROLL_CHAOS) && (o_ptr->level < 30)) o_ptr->level = 30;
	if (o_ptr->tval == TV_RING && o_ptr->sval == SV_RING_SPEED
	    && o_ptr->level && o_ptr->bpval > 0
	    && o_ptr->level != SPEED_RING_BASE_LEVEL + o_ptr->bpval)
		o_ptr->level = SPEED_RING_BASE_LEVEL + o_ptr->bpval;
	if ((o_ptr->tval == TV_DRAG_ARMOR) && (o_ptr->sval == SV_DRAGON_POWER) && (o_ptr->level < 45)) o_ptr->level = 44 + randint(5);

	if (o_ptr->tval == TV_LITE) {
		if (o_ptr->name1 == ART_RANDART) {
			switch (o_ptr->sval) {
			case SV_LITE_DWARVEN: if (o_ptr->level < 30) o_ptr->level = 30; break;//ego powered lower limit is ~28, going slightly above that..
			case SV_LITE_FEANORIAN: if (o_ptr->level < 38) o_ptr->level = 38; break;
			}
		} else {
			switch (o_ptr->sval) {
			case SV_LITE_DWARVEN: if (o_ptr->level < 20) o_ptr->level = 20; break;
			case SV_LITE_FEANORIAN: if (o_ptr->level < 32) o_ptr->level = 32; break;
			}
		}
	}

	/* Fix cheap but high +dam weaponry: */
	if (o_ptr->level * 10 < o_ptr->to_d * 12) o_ptr->level = (o_ptr->to_d * 12) / 10;


	/* --------------- Reduce excessive level --------------- */


	/* Slightly reduce high levels */
	if (o_ptr->level > 55) o_ptr->level--;
	if (o_ptr->level > 50) o_ptr->level--;
	if (o_ptr->tval != TV_RING || o_ptr->sval != SV_RING_SPEED) {
		if (o_ptr->level > 45) o_ptr->level--;
		if (o_ptr->level > 40) o_ptr->level--;
	}

#if 0
	/* tone down deep randarts a bit to allow winner-trading */
	if (o_ptr->name1 == ART_RANDART) {
		if (o_ptr->level > 51) o_ptr->level = 51 + ((o_ptr->level - 51) / 3);
	}

	/* tone down deep winners_only items to allow winner-trading */
	else if (k_info[o_ptr->k_idx].flags5 & TR5_WINNERS_ONLY) {
		if (o_ptr->level > 51) o_ptr->level = 51 + ((o_ptr->level - 51) / 2);
	}

	/* done above instead, where EGO_ are tested */
	/* Reduce outrageous ego item levels (double-ego adamantite of immunity for example */
	else if (o_ptr->name2) {
		if (o_ptr->level > 51) o_ptr->level = 48 + rand_int(4);
		else if (o_ptr->level > 48) o_ptr->level = 48 + rand_int(2);
	}
#else /* unify.. */
	/* tone down very-high-level items for trading */
	if (o_ptr->level > 51) o_ptr->level = 51 + ((o_ptr->level - 51) / 3);
	/* further tone down if no randart and no winners-only */
	if (o_ptr->level > 50 &&
	    o_ptr->name1 != ART_RANDART && !(k_info[o_ptr->k_idx].flags5 & TR5_WINNERS_ONLY))
		o_ptr->level -= rand_int(3);
#endif

	/* Special limit for +LIFE randarts */
	if ((o_ptr->name1 == ART_RANDART) &&
	    (a_ptr->flags1 & TR1_LIFE) && (o_ptr->level <= 50))
		o_ptr->level = 51 + rand_int(2);

#if 1 /* experimental */
	/* Tone down some very high true artifact levels to make them more tradable */
	if (true_artifact_p(o_ptr) && !winner_artifact_p(o_ptr)) {
		int alev = a_info[o_ptr->name1].level;

		/* olev depends directly on alev */
		o_ptr->level = 10 + (alev * 2) / 5;
		/* Add malus (level increase) for deeper dungeon floors (up to +3 for level 100 arts: lvl 127 bottom Angband) */
		if (level > alev) o_ptr->level += (level - alev) / 8;

		/* Adjust level > 50 arts back to at most 50 (level 100 arts which got +3 malus above) */
		if (o_ptr->level > 50) o_ptr->level--;
		if (o_ptr->level > 50) o_ptr->level--;
		if (o_ptr->level > 50) o_ptr->level--;
	}
#endif

	/* Ring of Phasing (not set here, so this is 'dead' code, but keep consistent with xtra2.c anyway) */
	if (o_ptr->name1 == ART_PHASING && (o_ptr->level < 60 || o_ptr->level > 65)) o_ptr->level = 60 + rand_int(6);

	/* Special limit for WINNERS_ONLY items */
	if ((k_info[o_ptr->k_idx].flags5 & TR5_WINNERS_ONLY) && (o_ptr->level <= 50))
		o_ptr->level = 51 + rand_int(5);
}
#else /* new way, quite reworked */
void determine_level_req(int level, object_type *o_ptr) {
}
#endif

/* Purpose: fix old items' level requirements.
   What it does: Check current level against theoretical minimum level of
   that item according to current determine_level_req() values.
   Fix if below, by applying bottom cap value. */
void verify_level_req(object_type *o_ptr) {
	int lv = o_ptr->level;

	if (!lv) return;

	determine_level_req(0, o_ptr);
	if (lv > o_ptr->level) o_ptr->level = lv;
}

/* Set level req for polymorph ring on its creation - C. Blue */
int ring_of_polymorph_level(int r_lev) {
#if 0
	if (r_lev == 0) return 5;
	/* 0->5..1->6..30->26..60->42..80->51..85->53..100->58 */
	//return 5 + (1600 / ((2000 / (r_lev + 1)) + 10));
	//return 10 + (1000 / ((2000 / r_lev) + 10));
	//return 5 + (1000 / ((1500 / r_lev) + 5));
	//return 5 + (1000 / ((1500 / r_lev) + 7));
#else /* use two curves, low-lev and rest */
	if (r_lev < 7) return 5;
	else if (r_lev < 30) return (850 / (1170 / r_lev)); /* 7 -> 5, 15 -> 10, 23 -> 16, 29 -> _21_ */
	else return 5 + (1000 / ((1500 / r_lev) + 7)); /* 30 -> _22_, 60 -> 36, 80 -> 43, 100 -> 50 */
#endif
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
void init_match_theme(obj_theme theme) {
	/* Save the theme */
	match_theme = theme;
}

#if 0
/*
 * Ditto XXX XXX XXX
 */
static bool theme_changed(obj_theme theme) {
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
static int kind_is_theme(int k_idx) {
	object_kind *k_ptr = &k_info[k_idx];
	/* assume 'junk' class */
	int p = 100 - (match_theme.treasure + match_theme.combat + match_theme.magic + match_theme.tools);

	/*
	 * Paranoia -- Prevent accidental "(Nothing)"
	 * that are results of uninitialised theme structs.
	 *
	 * Caution: Junks go into the allocation table.
	 */
	if (p == 100) return 100;


	/* Pick probability to use */
	switch (k_ptr->tval) {
	case TV_SKELETON:
	case TV_BOTTLE:
	case TV_JUNK:
	case TV_CORPSE:
	case TV_EGG:
	case TV_GOLEM: /* hm, this was missing here, for a long time - C. Blue */
		/*
		 * Degree of junk is defined in terms of the other
		 * 4 quantities XXX XXX XXX
		 * The type of prob should be *signed* as well as
		 * larger than theme components, or we would see
		 * unexpected, well, junks.
		 */
		break;//junk (p default value)

	case TV_CHEST:		p = match_theme.treasure; break;
	case TV_CROWN:		p = match_theme.treasure; break;
	case TV_DRAG_ARMOR:	p = match_theme.treasure; break;
	case TV_AMULET:		p = match_theme.treasure; break;
	case TV_RING:		p = match_theme.treasure; break;

	case TV_SHOT:		p = match_theme.combat; break;
	case TV_ARROW:		p = match_theme.combat; break;
	case TV_BOLT:		p = match_theme.combat; break;
	case TV_BOOMERANG:	p = match_theme.combat; break;
	case TV_BOW:		p = match_theme.combat; break;
	case TV_BLUNT:		p = match_theme.combat; break;
	case TV_POLEARM:	p = match_theme.combat; break;
	case TV_SWORD:		p = match_theme.combat; break;
	case TV_AXE:		p = match_theme.combat; break;
	case TV_GLOVES:		p = match_theme.combat; break;
	case TV_HELM:		p = match_theme.combat; break;
	case TV_SHIELD:		p = match_theme.combat; break;
	case TV_SOFT_ARMOR:	p = match_theme.combat; break;
	case TV_HARD_ARMOR:	p = match_theme.combat; break;

	case TV_MSTAFF:		p = match_theme.magic; break;
	case TV_STAFF:		p = match_theme.magic; break;
	case TV_WAND:		p = match_theme.magic; break;
	case TV_ROD:		p = match_theme.magic; break;
	case TV_ROD_MAIN:	p = match_theme.magic; break;
	case TV_SCROLL:		p = match_theme.magic; break;
	case TV_PARCHMENT:	p = match_theme.magic; break;
	case TV_POTION:		p = match_theme.magic; break;
	case TV_POTION2:	p = match_theme.magic; break;

	case TV_RUNE:		p = match_theme.magic; break;
#if 0
	case TV_BATERIE:	p = match_theme.magic; break;
	case TV_RANDART:	p = match_theme.magic; break;
	case TV_BOOK:		p = match_theme.magic; break;
	case TV_SYMBIOTIC_BOOK: p = match_theme.magic; break;
	case TV_MUSIC_BOOK:	p = match_theme.magic; break;
	case TV_DRUID_BOOK:	p = match_theme.magic; break;
	case TV_DAEMON_BOOK:	p = match_theme.magic; break;
#endif
	case TV_BOOK:		p = match_theme.magic; break;

	case TV_LITE:		p = match_theme.tools; break;
	case TV_CLOAK:		p = match_theme.tools; break;
	case TV_BOOTS:		p = match_theme.tools; break;
	case TV_SPIKE:		p = match_theme.tools; break;
	case TV_DIGGING:	p = match_theme.tools; break;
	case TV_FLASK:		p = match_theme.tools; break;
	case TV_TOOL:		p = match_theme.tools; break;
	case TV_INSTRUMENT:	p = match_theme.tools; break;
	case TV_TRAPKIT:	p = match_theme.tools; break;

	case TV_FIRESTONE:
	case TV_FOOD:
#if 0
		/* Hybrid: Can be junk or tools (while being neither really..)! */
		if (match_theme.tools > p) p = match_theme.tools;
#else
		/* They count as junk, just because 'tools' just sounds so unfitting. :p
		   And two treasure classes at once is just too messy.
		   Note: Wow, an entire #else branch just for a comment! */
#endif
		break;
	}

	/* Return the percentage */
	return p;
}

//#if FORCED_DROPS == 1  --now also used for tc_bias
static int which_theme(int tval) {
	/* Pick probability to use */
	switch (tval) {
	case TV_SKELETON:
	case TV_BOTTLE:
	case TV_JUNK:
	case TV_CORPSE:
	case TV_EGG:
	case TV_GOLEM:

	case TV_FIRESTONE:
	case TV_FOOD:
		return 0; //junk

	case TV_CHEST:
	case TV_CROWN:
	case TV_DRAG_ARMOR:
	case TV_AMULET:
	case TV_RING:
		return 1; //treasure

	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
	case TV_BOOMERANG:
	case TV_BOW:
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_AXE:
	case TV_GLOVES:
	case TV_HELM:
	case TV_SHIELD:
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
		return 2; //combat

	case TV_MSTAFF:
	case TV_STAFF:
	case TV_WAND:
	case TV_ROD:
	case TV_ROD_MAIN:
	case TV_SCROLL:
	case TV_PARCHMENT:
	case TV_POTION:
	case TV_POTION2:

	case TV_RUNE:
 #if 0
	case TV_BATERIE:
	case TV_RANDART:
	case TV_BOOK:
	case TV_SYMBIOTIC_BOOK:
	case TV_MUSIC_BOOK:
	case TV_DRUID_BOOK:
	case TV_DAEMON_BOOK:
 #endif
	case TV_BOOK:
		return 3; //magic

	case TV_LITE:
	case TV_CLOAK:
	case TV_BOOTS:
	case TV_SPIKE:
	case TV_DIGGING:
	case TV_FLASK:
	case TV_TOOL:
	case TV_INSTRUMENT:
	case TV_TRAPKIT:
		return 4; //tools

	default: //paranoia
		s_printf("ERROR: uncategorized item %d\n", tval);
		return -1;
	}
}
//#endif

/*
 * Determine if an object must not be generated.
 * Note: Returns PERMILLE.
 */
int kind_is_legal_special = -1;
int kind_is_legal(int k_idx, u32b resf) {
	object_kind *k_ptr = &k_info[k_idx];
	int p = kind_is_theme(k_idx);

	/* Used only for the Nazgul rings */
	if ((k_ptr->tval == TV_RING) && (k_ptr->sval == SV_RING_SPECIAL)) p = 0;

	/* Are we forced to one tval ? */
	if ((kind_is_legal_special != -1) && (kind_is_legal_special != k_ptr->tval)) p = 0;

	/* If legal store item and 'flat', make it equal to all other items */
	if (p && (resf & RESF_STOREFLAT)) p = 1000;

	/* Return the percentage */
	return p * 10;
}

/* Added for applying tc_bias values to kind_is_legal output */
static int kind_is_normal(int k_idx, u32b resf) {
	int tc_p = kind_is_legal(k_idx, resf);

	switch (which_theme(k_info[k_idx].tval)) {
	case 1: return (tc_p * tc_bias_treasure) / 500;
	case 2: return (tc_p * tc_bias_combat) / 500;
	case 3: return (tc_p * tc_bias_magic) / 500;
	case 4: return (tc_p * tc_bias_tools) / 500;
	case 0: return (tc_p * tc_bias_junk) / 500;
	}
	//-1, unclassified item (paranoia)
	return tc_p / 5;
}

/*
 * Hack -- determine if a template is "good"
 *   ugh, this is pretty bad hard-coding, and probably not even needed anymore?
 *   Note: Removing the speed-ring hardcode will probably lower their drop rate from wyrms and other good-droppers!
 *   Note: Returns PERMILLE.
 */
static int kind_is_good(int k_idx, u32b resf) {
	object_kind *k_ptr = &k_info[k_idx];
	int tc_p = kind_is_legal(k_idx, resf);

	/* DROP_GOOD also obeys the monster's treasure class
	   (we basically just need its kind_is_theme() call) */
	if (!tc_p) return 0;

	/* Analyze the item type */
	switch (k_ptr->tval) {
	/* Armor -- Good unless damaged */
	case TV_HARD_ARMOR:
	case TV_SOFT_ARMOR:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
		if (k_ptr->to_a < 0) return 0;
		return (tc_p * tc_biasg_combat) / 500;
	case TV_DRAG_ARMOR:
	case TV_CROWN:
		if (k_ptr->to_a < 0) return 0;
		return (tc_p * tc_biasg_treasure) / 500;

	/* Weapons -- Good unless damaged */
	case TV_BOW:
	case TV_SWORD:
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_AXE:
	case TV_BOOMERANG:
		if (k_ptr->to_h < 0) return 0;
		if (k_ptr->to_d < 0) return 0;
		return (tc_p * tc_biasg_combat) / 500;
	/* Diggers are similar to weapons in this regard */
	case TV_DIGGING:
		if (k_ptr->to_h < 0) return 0;
		if (k_ptr->to_d < 0) return 0;
		return (tc_p * tc_biasg_tools) / 500;

	/* Ammo -- Arrows/Bolts are good */
	case TV_BOLT:
	case TV_ARROW:
	case TV_SHOT:
		if (k_ptr->sval == SV_AMMO_CHARRED) return 0;
		return (tc_p * tc_biasg_combat) / 500;
	case TV_MSTAFF:
		return (tc_p * tc_biasg_magic) / 500;

	/* Trap kits are good now, since weapons are, too. */
	case TV_TRAPKIT:
		return (tc_p * tc_biasg_tools) / 500;

	/* Rings -- Rings of Speed are good */
	case TV_RING:
		switch (k_ptr->sval) {
		case SV_RING_SPEED:
		case SV_RING_BARAHIR:
		case SV_RING_TULKAS:
		case SV_RING_NARYA:
		case SV_RING_NENYA:
		case SV_RING_VILYA:
		case SV_RING_POWER:

		case SV_RING_LORDLY:
		case SV_RING_ATTACKS:
		case SV_RING_FLAR:
		case SV_RING_CRIT:
		case SV_RING_DURIN:

#if 1 /* 4.6.2; lesser rings, but still good: */
		//omitted +stat rings, but they _could_ turn out great
		case SV_RING_ACCURACY:
		case SV_RING_DAMAGE:
		case SV_RING_SLAYING:
		case SV_RING_RES_NETHER:
		case SV_RING_RES_DISENCHANT:
		case SV_RING_RES_CHAOS:
		case SV_RING_INVIS:
#endif
#if 1 /* 4.6.2; lesser rings, but still ok for when we're matching monster themes (kind_is_legal() call above): */
		case SV_RING_RESIST_POIS:
		case SV_RING_RES_BLINDNESS:
		case SV_RING_LEVITATION:
		case SV_RING_FLAMES:
		case SV_RING_ICE:
		case SV_RING_ACID:
		case SV_RING_ELEC:
#endif
			return (tc_p * tc_biasg_magic) / 500;
		}
		break;

	case TV_LITE:
		switch (k_ptr->sval) {
		case SV_LITE_TORCH_EVER:
		case SV_LITE_DWARVEN:
		case SV_LITE_FEANORIAN:
		case SV_LITE_GALADRIEL:
		case SV_LITE_ELENDIL:
		case SV_LITE_THRAIN:
		case SV_LITE_PALANTIR:
		case SV_ANCHOR_SPACETIME:
		case SV_STONE_LORE:
			return (tc_p * tc_biasg_tools) / 500;
		}
		break;

	case TV_AMULET:
		switch (k_ptr->sval) {
		case SV_AMULET_BRILLIANCE:
		case SV_AMULET_REFLECTION:
		case SV_AMULET_CARLAMMAS:
		case SV_AMULET_INGWE:
		case SV_AMULET_DWARVES:
		case SV_AMULET_RESISTANCE:
		case SV_AMULET_SERPENT:
		case SV_AMULET_TORIS_MEJISTOS:
		case SV_AMULET_ELESSAR:
		case SV_AMULET_EVENSTAR:
		case SV_AMULET_SUSTENANCE:
		case SV_AMULET_ESP:
		case SV_AMULET_THE_MAGI:
		case SV_AMULET_TRICKERY:
		case SV_AMULET_DEVOTION:
		case SV_AMULET_THE_MOON:
		case SV_AMULET_WEAPONMASTERY:
		case SV_AMULET_RAGE:
		case SV_AMULET_GROM:
		case SV_AMULET_SSHARD:
		case SV_AMULET_SPEED:
		case SV_AMULET_TERKEN:
			return (tc_p * tc_biasg_magic) / 500;
		}
		break;

	case TV_STAFF:
		switch (k_ptr->sval) {
		case SV_STAFF_CURING:
		case SV_STAFF_HEALING:
		case SV_STAFF_THE_MAGI:
		case SV_STAFF_SPEED:
		case SV_STAFF_PROBING:
		case SV_STAFF_DISPEL_EVIL:
		case SV_STAFF_POWER:
		case SV_STAFF_HOLINESS:
		case SV_STAFF_GENOCIDE:
		case SV_STAFF_EARTHQUAKES:
		case SV_STAFF_DESTRUCTION:
		case SV_STAFF_STAR_IDENTIFY:
			return (tc_p * tc_biasg_magic) / 500;
		}
		break;
	case TV_WAND:
		switch (k_ptr->sval) {
		case SV_WAND_ACID_BOLT:
		case SV_WAND_FIRE_BOLT:
		case SV_WAND_COLD_BOLT:
		case SV_WAND_ELEC_BOLT:
		case SV_WAND_ACID_BALL:
		case SV_WAND_ELEC_BALL:
		case SV_WAND_COLD_BALL:
		case SV_WAND_FIRE_BALL:
		case SV_WAND_DRAIN_LIFE:
		case SV_WAND_ANNIHILATION:
		case SV_WAND_DRAGON_FIRE:
		case SV_WAND_DRAGON_COLD:
		case SV_WAND_DRAGON_BREATH:
		case SV_WAND_ROCKETS:
		case SV_WAND_TELEPORT_AWAY:
		//case SV_WAND_WALL_CREATION:
			return (tc_p * tc_biasg_magic) / 500;
		}
		break;
	case TV_ROD:
		switch (k_ptr->sval) {
		case SV_ROD_IDENTIFY:
		case SV_ROD_RECALL:
		case SV_ROD_MAPPING:
		case SV_ROD_DETECTION:
		case SV_ROD_PROBING:
		case SV_ROD_CURING:
		case SV_ROD_HEALING:
		case SV_ROD_RESTORATION:
		case SV_ROD_SPEED:
		case SV_ROD_TELEPORT_AWAY:
		case SV_ROD_DRAIN_LIFE:
		case SV_ROD_ACID_BOLT:
		case SV_ROD_ELEC_BOLT:
		case SV_ROD_COLD_BOLT:
		case SV_ROD_FIRE_BOLT:
		case SV_ROD_ACID_BALL:
		case SV_ROD_ELEC_BALL:
		case SV_ROD_COLD_BALL:
		case SV_ROD_FIRE_BALL:
		case SV_ROD_HAVOC:
			return (tc_p * tc_biasg_magic) / 500;
		}
		break;
	default:
		/* Specialty: Left over tvals.
		   Probably potions and scrolls mostly, these don't need any special treatment:
		   Monsters specialized on dropping them are already highly valued for that.
		   Further, notably are food and golem items, books and runes.

		   All left over tvals used to be chance=0, but now that we're matching
		   kind_is_theme(), any item tval left over here must be possible to spawn: */
		if (k_ptr->cost < 200) return 0; //except items that are really not GOOD

		switch (which_theme(k_ptr->tval)) {
		case 1: return (tc_p * tc_biasg_treasure) / 500;
		case 2: return (tc_p * tc_biasg_combat) / 500;
		case 3: return (tc_p * tc_biasg_magic) / 500;
		case 4: return (tc_p * tc_biasg_tools) / 500;
		case 0: return (tc_p * tc_biasg_junk) / 500;
		}
		//-1, unclassified item (paranoia)
		return tc_p / 5;
	}
	//note: no tools atm :/ could add +2/+3 diggers?

	/* Left over svals (belonging to tvals treated in dedicated switch-cases above),
	   that we assume are definitely 'not good', but let's give them a tiny
	   chance nevertheless, to smooth out the item drop choices. */
	if (k_ptr->cost < 200) return 0; //except items that are really not GOOD

#if 0
	switch (which_theme(k_ptr->tval)) {
	case 1: return (tc_p * tc_biasg_treasure) / 500;
	case 2: return (tc_p * tc_biasg_combat) / 500;
	case 3: return (tc_p * tc_biasg_magic) / 500;
	case 4: return (tc_p * tc_biasg_tools) / 500;
	case 0: return (tc_p * tc_biasg_junk) / 500;
	}
	//-1, unclassified item (paranoia)
	return tc_p / 5;
#endif
#if 1
	return (tc_p * 1) / 100; //absolute minimum chance that is guaranteed to be not 0 for any rarest item: 1%.
#endif
#if 0
	return 0;
#endif
}
/* A variant of kind_is_good() for DROP_GREAT monsters.
   The main difference is, that flavoured objects do not have 'great' enchantments,
   except maybe for ego rods and ego lamps,
   so instead their sval must be picked so that they can be considered 'great'.
   For non-flavoured objects this function is the same as kind_is_good().
   (Note: 'power' in apply_magic() has actually no effect on jewelry boni (stat rings).
   it is ONLY used for determining ego/art.) */
static int kind_is_great(int k_idx, u32b resf) {
	object_kind *k_ptr = &k_info[k_idx];
	int tc_p = kind_is_legal(k_idx, resf);

	/* DROP_GREAT also obeys the monster's treasure class
	   (we basically just need its kind_is_theme() call) */
	if (!tc_p) return 0;

	/* Analyze the item type */
	switch (k_ptr->tval) {
	/* Armor -- Good unless damaged */
	case TV_HARD_ARMOR:
	case TV_SOFT_ARMOR:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
		if (k_ptr->to_a < 0) return 0;
		return (tc_p * tc_biasr_combat) / 500;
	case TV_DRAG_ARMOR:
	case TV_CROWN:
		if (k_ptr->to_a < 0) return 0;
		return (tc_p * tc_biasr_magic) / 500;

	/* Weapons -- Good unless damaged */
	case TV_BOW:
	case TV_SWORD:
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_AXE:
	case TV_BOOMERANG:
		if (k_ptr->to_h < 0) return 0;
		if (k_ptr->to_d < 0) return 0;
		return (tc_p * tc_biasr_combat) / 500;
	/* Diggers are similar to weapons in this regard */
	case TV_DIGGING:
		if (k_ptr->to_h < 0) return 0;
		if (k_ptr->to_d < 0) return 0;
		return (tc_p * tc_biasr_tools) / 500;

	/* Ammo -- Arrows/Bolts are good */
	case TV_BOLT:
	case TV_ARROW:
	case TV_SHOT:
		if (k_ptr->sval == SV_AMMO_CHARRED) return 0;
		return (tc_p * tc_biasr_combat) / 500;
	case TV_MSTAFF:
		return (tc_p * tc_biasr_magic) / 500;

	/* Trap kits are good now, since weapons are, too. */
	case TV_TRAPKIT:
		return (tc_p * tc_biasr_tools) / 500;

	/* Rings -- Rings of Speed are good */
	case TV_RING:
		switch (k_ptr->sval) {
		case SV_RING_SPEED:
		case SV_RING_BARAHIR:
		case SV_RING_TULKAS:
		case SV_RING_NARYA:
		case SV_RING_NENYA:
		case SV_RING_VILYA:
		case SV_RING_POWER:

		case SV_RING_LORDLY:
		case SV_RING_ATTACKS:
		case SV_RING_FLAR:
		case SV_RING_CRIT:
		case SV_RING_DURIN:
#if 0
		case SV_RING_RES_NETHER:
		case SV_RING_RES_DISENCHANT:
		case SV_RING_RES_CHAOS:
		case SV_RING_INVIS:
#endif
			return (tc_p * tc_biasr_magic) / 500;
		}
		break;

	case TV_LITE:
		switch (k_ptr->sval) {
#if 0
		case SV_LITE_TORCH_EVER:
#endif
#if 1 /* not so great, but hoping for ego power */
		case SV_LITE_DWARVEN:
#endif
		case SV_LITE_FEANORIAN:
		case SV_LITE_GALADRIEL:
		case SV_LITE_ELENDIL:
		case SV_LITE_THRAIN:
		case SV_LITE_PALANTIR:
		case SV_ANCHOR_SPACETIME:
		case SV_STONE_LORE:
			return (tc_p * tc_biasr_tools) / 500;
		}
		break;

	case TV_AMULET:
		switch (k_ptr->sval) {
#if 0
		case SV_AMULET_BRILLIANCE:
		case SV_AMULET_REFLECTION:
		case SV_AMULET_RESISTANCE:
		case SV_AMULET_SERPENT:
		case SV_AMULET_SUSTENANCE:
		case SV_AMULET_THE_MOON:
		case SV_AMULET_TERKEN:
#endif
		case SV_AMULET_SPEED:
		case SV_AMULET_CARLAMMAS:
		case SV_AMULET_INGWE:
		case SV_AMULET_DWARVES:
		case SV_AMULET_TORIS_MEJISTOS:
		case SV_AMULET_ELESSAR:
		case SV_AMULET_EVENSTAR:
		case SV_AMULET_ESP:
		case SV_AMULET_THE_MAGI:
		case SV_AMULET_TRICKERY:
		case SV_AMULET_DEVOTION:
		case SV_AMULET_WEAPONMASTERY:
		case SV_AMULET_RAGE:
		case SV_AMULET_GROM:
		case SV_AMULET_SSHARD:
			return (tc_p * tc_biasr_magic) / 500;
		}
		break;

	case TV_STAFF:
		switch (k_ptr->sval) {
#if 0
		case SV_STAFF_CURING:
		case SV_STAFF_HEALING:
		case SV_STAFF_SPEED:
		case SV_STAFF_PROBING:
		case SV_STAFF_EARTHQUAKES:
#endif
#if 0
		case SV_STAFF_HEALING:
		case SV_STAFF_DESTRUCTION:
		case SV_STAFF_DISPEL_EVIL:
		case SV_STAFF_GENOCIDE:
#endif
		case SV_STAFF_POWER:
		case SV_STAFF_THE_MAGI:
		case SV_STAFF_HOLINESS:
		case SV_STAFF_STAR_IDENTIFY:
			return (tc_p * tc_biasr_magic) / 500;
		}
		break;
	case TV_WAND:
		switch (k_ptr->sval) {
#if 0
		case SV_WAND_TELEPORT_AWAY:
		//case SV_WAND_WALL_CREATION:
		case SV_WAND_ACID_BOLT:
		case SV_WAND_FIRE_BOLT:
		case SV_WAND_COLD_BOLT:
		case SV_WAND_ELEC_BOLT:
		case SV_WAND_ACID_BALL:
		case SV_WAND_ELEC_BALL:
		case SV_WAND_COLD_BALL:
		case SV_WAND_FIRE_BALL:
		case SV_WAND_DRAGON_FIRE:
		case SV_WAND_DRAGON_COLD:
		case SV_WAND_DRAGON_BREATH:
#endif
		case SV_WAND_DRAIN_LIFE:
		case SV_WAND_ANNIHILATION:
		case SV_WAND_ROCKETS:
			return (tc_p * tc_biasr_magic) / 500;
		}
		break;
	case TV_ROD:
		switch (k_ptr->sval) {
#if 1 /* not so great base item, but hoping for ego power! */
		case SV_ROD_DETECTION:
		case SV_ROD_PROBING:
		case SV_ROD_CURING:
		case SV_ROD_TELEPORT_AWAY:
		case SV_ROD_ACID_BOLT:
		case SV_ROD_ELEC_BOLT:
		case SV_ROD_COLD_BOLT:
		case SV_ROD_FIRE_BOLT:

		case SV_ROD_ACID_BALL:
		case SV_ROD_ELEC_BALL:
		case SV_ROD_COLD_BALL:
		case SV_ROD_FIRE_BALL:
#endif
#if 1 /* not so great base item, but hoping for ego power! */
		case SV_ROD_RECALL:
		case SV_ROD_MAPPING:
#endif
		case SV_ROD_IDENTIFY:
		case SV_ROD_HEALING:
		case SV_ROD_RESTORATION:
		case SV_ROD_SPEED:
		case SV_ROD_DRAIN_LIFE:
		case SV_ROD_HAVOC:
			return (tc_p * tc_biasr_magic) / 500;
		}
		break;

	/* Handle some tvals that in most cases shouldn't count as 'great' due to their low usability */
	case TV_RUNE:
		/* Even though runes can cost 5000 Au, we'll exclude them from 'great' for now. */
		return 0;
	case TV_BOOK:
		/* No handbooks, even though they're pretty costly. */
		if (k_ptr->cost <= 20000) return 0; //Tomes+Grimoires
		return (tc_p * tc_biasr_magic) / 500;
	case TV_GOLEM:
		/* Only rare massive pieces (gold+), no arms/legs/scrolls */
		if (k_ptr->cost < 20000) return 0;
		return (tc_p * tc_biasr_junk) / 500;

	default:
		/* Specialty: Left over tvals.
		   Probably potions and scrolls mostly, these don't need any special treatment:
		   Monsters specialized on dropping them are already highly valued for that.
		   Further, notably are food and golem items, books and runes.

		   However, low-level DROP_GREAT monsters *might* need treasure class changes
		   if they're really expected to provide great starter weapons/armour.
		   Then again the potions/scrolls passing the price check shouldn't occur on
		   low levels anyway except for very rare OoD rolls.

		   All left over tvals used to be chance=0, but now that we're matching
		   kind_is_theme(), any item tval left over here must be possible to spawn: */
		if (k_ptr->cost <= 7000) return 0; //except items that are really not GREAT (Note though: Artifact Ale is 5k :/)

		switch (which_theme(k_ptr->tval)) {
		case 1: return (tc_p * tc_biasr_treasure) / 500;
		case 2: return (tc_p * tc_biasr_combat) / 500;
		case 3: return (tc_p * tc_biasr_magic) / 500;
		case 4: return (tc_p * tc_biasr_tools) / 500;
		case 0: return (tc_p * tc_biasr_junk) / 500;
		}
		//-1, unclassified item (paranoia)
		return tc_p / 5;
	}
	//note: no tools atm :/ could add +2/+3 diggers?

	/* Left over svals (belonging to tvals treated in dedicated switch-cases above),
	   that we assume are definitely 'not good', but let's give them a tiny
	   chance nevertheless, to smooth out the item drop choices. */
	if (k_ptr->cost <= 7000) return 0; //except items that are really not GREAT

#if 0
	switch (which_theme(k_ptr->tval)) {
	case 1: return (tc_p * tc_biasr_treasure) / 500;
	case 2: return (tc_p * tc_biasr_combat) / 500;
	case 3: return (tc_p * tc_biasr_magic) / 500;
	case 4: return (tc_p * tc_biasr_tools) / 500;
	case 0: return (tc_p * tc_biasr_junk) / 500;
	}
	//-1, unclassified item (paranoia)
	return tc_p / 5;
#endif
#if 1
	return (tc_p * 1) / 100; //absolute minimum chance that is guaranteed to be not 0 for any rarest item: 1%.
#endif
#if 0
	return 0;
#endif
}

/* Variant of kind_is_good() that includes trap kits,
   specifically made for create_reward().
   Note: Returns PERMILLE. */
static int kind_is_good_reward(int k_idx, u32b resf) {
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	switch (k_ptr->tval) {
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
		if (k_ptr->to_a < 0) return 0;
		return 1000;

	/* Weapons -- Good unless damaged */
	case TV_BOW:
	case TV_SWORD:
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_DIGGING:
	case TV_AXE:
	case TV_BOOMERANG:
		if (k_ptr->to_h < 0) return 0;
		if (k_ptr->to_d < 0) return 0;
		return 1000;

	/* Ammo -- Arrows/Bolts are good */
	case TV_BOLT:
	case TV_ARROW:
	case TV_SHOT:	/* are Shots bad? */
		if (k_ptr->sval == SV_AMMO_CHARRED) return 0;
	case TV_MSTAFF:
		return 1000;

	/* Trap kits are good now, since weapons are, too (required for dungeon keeper reward sval generation..) */
	case TV_TRAPKIT:
		return 1000;
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
void place_object(struct worldpos *wpos, int y, int x, bool good, bool great, bool verygreat, u32b resf, obj_theme theme, int luck, byte removal_marker) {
	int prob, base, tmp_luck, i, dlev;
	int tries = 0, k_idx, debug_k_idx = 0;

	object_type		forge;
	dun_level *l_ptr = getfloor(wpos);
	dungeon_type *d_ptr;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;
	dlev = getlevel(wpos);
	d_ptr = getdungeon(wpos);

	/* Paranoia -- check bounds */
	if (!in_bounds(y, x)) return;

	/* Hack - No l00t in Valinor */
	if (in_valinor(wpos)) return;

#ifdef RPG_SERVER /* no objects are generated in Training Tower */
	if (in_trainingtower(wpos)) return;
#endif

	/* Require clean floor space */
//	if (!cave_clean_bold(zcave, y, x)) return;

	if (resf & RESF_DEBUG_ITEM) {
		debug_k_idx = luck;
		luck = 0;
	}

	/* place_object_restrictor overrides resf */
	resf |= place_object_restrictor;

	/* Luck does not affect items placed at level creation time */
	if (!level_generation_time) {
		luck += global_luck;
		if (d_ptr) {
			if ((d_ptr->flags3 & DF3_LUCK_PROG_IDDC)) {
				/* progressive luck bonus past Menegroth */
				luck += dlev > 40 ? dlev / 20 + 1 : 0;
			}
			if ((d_ptr->flags3 & DF3_LUCK_1)) luck++;
			if ((d_ptr->flags3 & DF3_LUCK_5)) luck += 5;
			if ((d_ptr->flags3 & DF3_LUCK_20)) luck += 20;
		}
	}

	if (luck < -10) luck = -10;
	if (luck > 40) luck = 40;

	//200-(8000/(luck+40))		old way (0..100)
	//(2000-(11250/(5+1)))*4/70	->0..100
	//(1125-(11250/(10+40)))/10	->0..90	(only prob is, low luck values result in too high factors: 4->32 instead of 19
	//(1125-(11250/(10+40)))/20	->0..45 ^problem solved, but no diffs 0->1 and 39->40
	//(1125-(11250/(10+40)))/15	->0..60 ^all solved. low luck values are a bit more effective now than they originally were ('old way'), seems ok.
	if (luck > 0) {
		/* max luck = 40 */
		tmp_luck = (1125 - (11250 / (luck + 10))) / 15;
		if (!good && !great && magik(tmp_luck / 6)) good = TRUE;
		else if (good && !great && magik(tmp_luck / 20)) {great = TRUE; good = TRUE;}
	} else if (luck < 0) {
		/* min luck = -10 */
		tmp_luck = 200 - (2000 / (-luck + 10));
		if (great && magik(tmp_luck / 3)) {great = FALSE; good = TRUE;}
		else if (!great && good && magik(tmp_luck / 2)) good = FALSE;
	}

	/* Chance of "special object" */
	prob = (good || great ? 300 : 10000); // 10 : 1000; 30 : 1000

	/* Base level for the object */
	base = (good || great ? (object_level + 10) : object_level);


	/* Hack -- clear out the forgery */
	invwipe(&forge);

	/* Generate an item for debugging purpose */
	if (resf & RESF_DEBUG_ITEM) {
		k_idx = debug_k_idx;

		/* Prepare the object */
		invcopy(&forge, k_idx);
		forge.number = 1;
	}
	/* Generate a special object, or a normal object */
	else if ((rand_int(prob) != 0) || !make_artifact_special(wpos, &forge, resf)) {
		/* Check global variable, if some base types are forbidden */
		do {
			tries++;
			k_idx = 0;

			/* Select items based on "theme" */
			init_match_theme(theme);

			/* Good objects */
			if (great) {
				/* Activate restriction */
				get_obj_num_hook = kind_is_great;

				/* Prepare allocation table */
				get_obj_num_prep(resf);
			}
			/* Good objects */
			else if (good) {
				/* Activate restriction */
				get_obj_num_hook = kind_is_good;

				/* Prepare allocation table */
				get_obj_num_prep(resf);
			}
			/* Normal objects */
			else {
				/* Activate normal restriction */
				get_obj_num_hook = kind_is_normal;

				/* Prepare allocation table */
				get_obj_num_prep(resf);

				/* The table is synchronised */
				//alloc_kind_table_valid = TRUE;
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

			if ((resf & RESF_NOHIDSM) && (k_info[k_idx].tval == TV_DRAG_ARMOR) &&
			    !sv_dsm_low(k_info[k_idx].sval) && !sv_dsm_mid(k_info[k_idx].sval))
				continue;

			if ((resf & RESF_LOWVALUE) && (k_info[k_idx].cost > 35000)) continue;
			if ((resf & RESF_MIDVALUE) && (k_info[k_idx].cost > 50000)) continue;
			if ((resf & RESF_NOHIVALUE) && (k_info[k_idx].cost > 100000)) continue;

			if ((resf & RESF_NOTRUEART) && (k_info[k_idx].flags3 & TR3_INSTA_ART)) continue;

			if (!(resf & RESF_WINNER) && k_info[k_idx].flags5 & TR5_WINNERS_ONLY) continue;

			if ((k_info[k_idx].flags5 & TR5_FORCE_DEPTH) && dlev < k_info[k_idx].level) continue;

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

	if (opening_chest) {
		forge.owner = opening_chest_owner;
		forge.mode = opening_chest_mode;
		if (true_artifact_p(&forge)) determine_artifact_timeout(forge.name1, wpos);
	}

#ifdef IDDC_ID_BOOST /* experimental */
	if ((resf & RESF_COND_MASK) == 0x0 && in_irondeepdive(wpos) && !forge.name1 && !forge.name2
	    && k_info[forge.k_idx].cost <= 1000
	    && forge.level && !forge.owner && !forge.questor) {// && forge.tval == TV_SCROLL) {
		if (!rand_int(20)) {
			invwipe(&forge);
			forge.tval = TV_SCROLL;
			forge.sval = SV_SCROLL_IDENTIFY;
			forge.k_idx = lookup_kind(TV_SCROLL, SV_SCROLL_IDENTIFY);
			forge.number = 1;
			determine_level_req(dlev, &forge);
			s_printf("<<ID\n");
		} else if (!rand_int(500)) {
			invwipe(&forge);
			forge.tval = TV_SCROLL;
			forge.sval = SV_SCROLL_ID_ALL;
			forge.k_idx = lookup_kind(TV_SCROLL, SV_SCROLL_ID_ALL);
			forge.number = 1;
			determine_level_req(dlev, &forge);
			s_printf("<<IDE\n");
		} else if (!rand_int(200)) {
			invwipe(&forge);
			forge.tval = TV_SCROLL;
			forge.sval = SV_SCROLL_STAR_IDENTIFY;
			forge.k_idx = lookup_kind(TV_SCROLL, SV_SCROLL_STAR_IDENTIFY);
			forge.number = 1;
			determine_level_req(dlev, &forge);
			s_printf("<<*ID*\n");
		}
	}
#endif

	forge.marked2 = removal_marker;
	forge.discount = object_discount; /* usually 0, except for creation from stolen acquirement scrolls */
	drop_near(0, &forge, -1, wpos, y, x);

	/* for now ignore live-spawns. change that maybe? */
	if (level_generation_time) {
		/* Check that we're in a dungeon */
		if (l_ptr) {
			if (forge.name1) l_ptr->flags2 |= LF2_ARTIFACT;
			if (k_info[forge.k_idx].level >= dlev + 8) l_ptr->flags2 |= LF2_ITEM_OOD;
		}
	}

	/* Forced type? Hack: Abuse place_object_restrictor to report success */
	if ((resf & RESF_COND_SWORD) && forge.tval == TV_SWORD) place_object_restrictor |= RESF_COND_SWORD;
	if ((resf & RESF_COND_LSWORD) && forge.tval == TV_SWORD && !(k_info[forge.k_idx].flags4 & (TR4_MUST2H | TR4_SHOULD2H))) place_object_restrictor |= RESF_COND_LSWORD;
	if ((resf & RESF_COND_DARKSWORD) && forge.tval == TV_SWORD && forge.sval == SV_DARK_SWORD) place_object_restrictor |= RESF_COND_DARKSWORD;
	if ((resf & RESF_COND_BLUNT) && forge.tval == TV_BLUNT) place_object_restrictor |= RESF_COND_BLUNT;
	if ((resf & RESF_CONDF_NOSWORD) && is_melee_weapon(forge.tval) && forge.tval != TV_SWORD) place_object_restrictor |= RESF_CONDF_NOSWORD; //note: only melee weapons can clear this, not ranged weapons
	if ((resf & RESF_CONDF_MSTAFF) && forge.tval == TV_MSTAFF) place_object_restrictor |= RESF_CONDF_MSTAFF;
	if ((resf & RESF_COND_SLING) && forge.tval == TV_BOW && forge.sval == SV_SLING) place_object_restrictor |= RESF_COND_SLING; //note: sling ammo can't clear it, need an actual sling
	if ((resf & RESF_COND_RANGED) && is_ranged_weapon(forge.tval)) place_object_restrictor |= RESF_COND_RANGED; //note: ammo can't clear it, need a ranged weapon
	if ((resf & RESF_CONDF_RUNE) && forge.tval == TV_RUNE) place_object_restrictor |= RESF_CONDF_RUNE;
}

/* Like place_object(), but doesn't actually drop the object to the floor -  C. Blue */
void generate_object(object_type *o_ptr, struct worldpos *wpos, bool good, bool great, bool verygreat, u32b resf, obj_theme theme, int luck) {
	int prob, base, tmp_luck, i, dlev;;
	int tries = 0, k_idx;
	dungeon_type *d_ptr;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;
	dlev = getlevel(wpos);
	d_ptr = getdungeon(wpos);

	/* Hack - No l00t in Valinor */
	if (in_valinor(wpos)) return;

	/* place_object_restrictor overrides resf */
	resf |= place_object_restrictor;

	/* Luck does not affect items placed at level creation time */
	if (!level_generation_time) {
		luck += global_luck;
		if (d_ptr) {
			if ((d_ptr->flags3 & DF3_LUCK_PROG_IDDC)) {
				/* progressive luck bonus past Menegroth */
				luck += dlev > 40 ? dlev / 20 + 1 : 0;
			}
			if ((d_ptr->flags3 & DF3_LUCK_1)) luck++;
			if ((d_ptr->flags3 & DF3_LUCK_5)) luck += 5;
			if ((d_ptr->flags3 & DF3_LUCK_20)) luck += 20;
		}
	}

	if (luck < -10) luck = -10;
	if (luck > 40) luck = 40;

	if (luck > 0) {
		/* max luck = 40 */
		tmp_luck = (1125 - (11250 / (luck + 10))) / 15;
		if (!good && !great && magik(tmp_luck / 6)) good = TRUE;
		else if (good && !great && magik(tmp_luck / 20)) {great = TRUE; good = TRUE;}
	} else if (luck < 0) {
		/* min luck = -10 */
		tmp_luck = 200 - (2000 / (-luck + 10));
		if (great && magik(tmp_luck / 3)) {great = FALSE; good = TRUE;}
		else if (!great && good && magik(tmp_luck / 2)) good = FALSE;
	}

	/* Chance of "special object" */
	prob = (good || great ? 300 : 10000); // 10 : 1000; 30 : 1000

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

			/* Select items based on "theme" */
			init_match_theme(theme);

			/* Great objects */
			if (great) {
				/* Activate restriction */
				get_obj_num_hook = kind_is_great;

				/* Prepare allocation table */
				get_obj_num_prep(resf);
			}
			/* Good objects */
			else if (good) {
				/* Activate restriction */
				get_obj_num_hook = kind_is_good;

				/* Prepare allocation table */
				get_obj_num_prep(resf);
			}
			/* Normal objects */
			else {
				/* Activate normal restriction */
				get_obj_num_hook = kind_is_normal;

				/* Prepare allocation table */
				get_obj_num_prep(resf);

				/* The table is synchronised */
				//alloc_kind_table_valid = TRUE;
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
		switch (o_ptr->tval) {
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
void acquirement(struct worldpos *wpos, int y1, int x1, int num, bool great, bool verygreat, u32b resf) {
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Scatter some objects */
	for (; num > 0; --num) {
		/* Place a good (or great) object */
		place_object_restrictor = RESF_NONE;
		place_object(wpos, y1, x1, TRUE, great, verygreat, resf, acquirement_obj_theme, 0, ITEM_REMOVAL_NORMAL);
		/* Notice */
		note_spot_depth(wpos, y1, x1);
		/* Redraw */
		everyone_lite_spot(wpos, y1, x1);
	}
}
/*
 * Same as acquirement, except it doesn't drop the item to the floor. Creates one "great" object.
 */
void acquirement_direct(object_type *o_ptr, struct worldpos *wpos, bool great, bool verygreat, u32b resf) {
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Place a good (or great) object */
	place_object_restrictor = RESF_NONE;
	generate_object(o_ptr, wpos, TRUE, great, verygreat, resf, acquirement_obj_theme, 0);
}



/* for create_reward() ... */
static int reward_melee_check(player_type *p_ptr, long int treshold) {
	long int rnd_result = 0, selection = 0;
	long int choice1 = 0, choice2 = 0, choice3 = 0, choice4 = 0, choice5 = 0;
	if (p_ptr->s_info[SKILL_SWORD].value >= treshold
	    /* hack: critical-hits skill only affects swords! */
	    || p_ptr->s_info[SKILL_CRITS].value >= treshold)
		choice1 = p_ptr->s_info[SKILL_SWORD].value > p_ptr->s_info[SKILL_CRITS].value ?
		    p_ptr->s_info[SKILL_SWORD].value : p_ptr->s_info[SKILL_CRITS].value;
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
	/* ..not if we're dual-wielding */
	if (p_ptr->inventory[INVEN_WIELD].k_idx &&
	    p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD) return selection;
	/* ..not if we're rogues or rangers (help the lazy ones who forgot to spend skill points) anyway */
	if (p_ptr->pclass == CLASS_ROGUE || p_ptr->pclass == CLASS_ARCHER) return selection;
	/* ..help lazy rangers somewhat: if he forgot to equip proper weaponry and hasn't skilled ranged weapon
	   yet, he might get a shield otherwise, which is 'unusual' for rangers..
	   (Make an exception if he WANTS to use a shield apparently.) */
	if (p_ptr->pclass == CLASS_RANGER &&
	    !(p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD)) return selection;
	/* player's form cannot equip shields? */
	if (!item_tester_hook_wear(p_ptr->Ind, INVEN_ARM)) return selection;
//Nope, they can!	if (p_ptr->pclass == CLASS_SHAMAN) return(selection); /* shamans cannot cast magic well with shield. */
	switch (selection) {
	case 1: if magik(35) selection = 6; break;
	case 2: if magik(30) selection = 6; break;
	case 3: if magik(20) selection = 6; break;
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
	if (maxweight < 240 || mha || rha) choice1 = 10;
	if (maxweight >= 240 && !mha && !rha) {
		choice2 = 30; /* If player can make good use of heavy armour, make him likely to get one! */
		choice1 = 0; /* ..and don't give him too light armour */
	}
	if (maxweight >= 200 && !mha && !rha) choice3 = 4; /* Only slim chance to get a Dragon Scale Mail! */
	choice4 = 10;
	choice5 = 10;
	choice6 = 10;
	/* actually funny to give the "Highlander Tournament" winner a crown ;) */
	if (maxweight <= 50) choice7 = 10;//magic classes (aka 'low STR' a bit more likely to get one
	else choice7 = 5;
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

	/* actually exempt for mimics! Increasing mimicry will increase Magic skill too! */
	if (p_ptr->pclass == CLASS_MIMIC) return 0;

	/* get base value of MAGIC skill */
	compute_skills(p_ptr, &value, &mod, SKILL_MAGIC);
	/* check whether player increased magic skill above its base value */
	if (p_ptr->s_info[SKILL_MAGIC].value > value) selection = 1;

	return (selection);
}
static int reward_misc_check(player_type *p_ptr, long int treshold) {
	/* TV_TRAPKIT */
	if (p_ptr->s_info[SKILL_TRAPPING].value >= treshold) return 4;

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
void create_reward(int Ind, object_type *o_ptr, int min_lv, int max_lv, bool great, bool verygreat, u32b resf, long int treshold) {
	player_type *p_ptr = Players[Ind];
	bool good = TRUE;
	int base = (min_lv + max_lv) / 2; /* base object level */
	//int base = 100;
	int tries = 0, i = 0, j = 0;
	char o_name[ONAME_LEN];
	u32b f1, f2, f3, f4, f5, f6, esp;
	bool mha, rha; /* monk heavy armor, rogue heavy armor */
	bool go_heavy = TRUE; /* new special thingy: don't pick super light cloth armour if we're not specifically light-armour oriented */
	bool caster = FALSE;

	/* for analysis functions and afterwards for determining concrete reward */
	int maxweight_melee = adj_str_hold[p_ptr->stat_ind[A_STR]] * 10;
	int maxweight_ranged = adj_str_hold[p_ptr->stat_ind[A_STR]] * 10;
	int maxweight_shield = ((adj_str_hold[p_ptr->stat_ind[A_STR]] / 7) + 4) * 10;
	//int maxweight_armor = (adj_str_hold[p_ptr->stat_ind[A_STR]] - 10) * 10; /* not really directly calculatable, since cumber_armor uses TOTAL armor weight, so just estimate something.. pft */
	int maxweight_armor = adj_str_armor[p_ptr->stat_ind[A_STR]] * 10;
	object_type tmp_obj;
	int wearable[5], wearables; /* 5 pure (no shields) armour slots, 3 misc slots (tools omitted) */

	/* analysis results: */
	int melee_choice, ranged_choice, armor_choice, spell_choice, misc_choice;
	int final_choice = 0; /* 1 = melee, 2 = ranged, 3 = armor, 4 = misc item */

	/* concrete reward */
	int reward_tval = 0, reward_sval = 0, k_idx = 0, reward_maxweight = 500;
	int force_tval = o_ptr->tval;

	invwipe(o_ptr);

	/* fix reasonable limits */
	if (maxweight_armor < 30) maxweight_armor = 30;

	/* analyze skills */
	if (p_ptr->skill_points != (p_ptr->max_plv - 1) * SKILL_NB_BASE) {
		melee_choice = reward_melee_check(p_ptr, treshold);
		mha = (melee_choice == 5); /* monk heavy armor */
		rha = (get_skill(p_ptr, SKILL_DODGE)); /* rogue heavy armor; pclass == rogue or get_skill(SKILL_CRITS) are implied by this one due to current tables.c. dual_wield is left out on purpose. */
		/* analyze current setup (for reward_armor_check) */
		if (p_ptr->inventory[INVEN_WIELD].k_idx &&
		    p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)
			rha = TRUE; /* we're dual-wielding */
		/* make choices */
		ranged_choice = reward_ranged_check(p_ptr, treshold);
		armor_choice = reward_armor_check(p_ptr, mha, rha);
		spell_choice = reward_spell_check(p_ptr, treshold);
		misc_choice = reward_misc_check(p_ptr, treshold);
		/* newbie druids who didn't spend points on MA (this prevents pure caster druid rewards, but w/e) */
		if (p_ptr->pclass == CLASS_DRUID) {
			melee_choice = 5;
			mha = rha = TRUE; //Training MA causes Dodging skill spillover
		}
		/* martial arts -> no heavy armor (paranoia, shouldn't happen, already cought in reward_armor_check) */
		if (melee_choice == 5 &&
		    (armor_choice == 2 || armor_choice == 3))
			armor_choice = 1;
	} else { /* player didn't distribute ANY skill points, sigh */
		s_printf("CREATE_REWARD: no skills\n");
		melee_choice = 0;
		mha = FALSE;
		rha = FALSE;
		ranged_choice = 0;
		spell_choice = 0;
		switch (p_ptr->pclass) {
		case CLASS_WARRIOR:
		case CLASS_MIMIC:
		case CLASS_PALADIN:
#ifdef ENABLE_DEATHKNIGHT
		case CLASS_DEATHKNIGHT:
#endif
#ifdef ENABLE_HELLKNIGHT
		case CLASS_HELLKNIGHT:
#endif
		case CLASS_MINDCRAFTER:
			if (item_tester_hook_wear(Ind, INVEN_WIELD) && !rand_int(4)) melee_choice = 6;
			break;
		case CLASS_DRUID:
			melee_choice = 5;
		case CLASS_ADVENTURER:
#ifdef ENABLE_CPRIEST
		case CLASS_CPRIEST:
#endif
		case CLASS_PRIEST:
			mha = TRUE;
			break;
		case CLASS_ROGUE:
			if (item_tester_hook_wear(Ind, INVEN_WIELD)) melee_choice = 1;
			rha = TRUE;
			break;
		case CLASS_ARCHER:
		case CLASS_RANGER:
			if (item_tester_hook_wear(Ind, INVEN_WIELD)) ranged_choice = 1;
			break;
		case CLASS_MAGE:
		case CLASS_RUNEMASTER:
		case CLASS_SHAMAN:
			if (item_tester_hook_wear(Ind, INVEN_WIELD)) spell_choice = 1;
			break;
		}
		/* analyze current setup (for reward_armor_check) */
		if (p_ptr->inventory[INVEN_WIELD].k_idx &&
		    p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)
			rha = TRUE; /* we're dual-wielding */
		/* make choices */
		armor_choice = reward_armor_check(p_ptr, mha, rha);
		misc_choice = randint(3);
	}

	/* Special low limits for Martial Arts or Rogue-skill users: */
	if (rha) {
		switch (armor_choice) {
		case 1: maxweight_armor = 100; break;
		case 2: /* cannot happen */ break;
		case 3: /* cannot happen */ break;
		case 4: maxweight_armor = 40; break;
		case 5: maxweight_armor = 25; break;
		case 6: maxweight_armor = 40; break;//4.0 helmets don't exist (jewel encrusted crown has it though, so we just pretend..)
		case 7: maxweight_armor = 30; break;
		case 8: maxweight_armor = 15; break;
		}
		go_heavy = FALSE;
	}
	if (mha) {
		switch (armor_choice) {
		case 1: maxweight_armor = 30; break;
		case 2: /* cannot happen */ break;
		case 3: /* cannot happen */ break;
		case 4: maxweight_armor = 20; break;
		case 5: maxweight_armor = 20; break;
		case 6: maxweight_armor = 40; break;//4.0 helmets don't exist (jewel encrusted crown has it though, so we just pretend..)
		case 7: maxweight_armor = 30; break;
		case 8: maxweight_armor = 15; break;
		}
		go_heavy = FALSE;
	}
	if (spell_choice) {
		/* no heavy boots/gauntlets/helmets for casters */
		switch (armor_choice) {
		case 4: maxweight_armor = 40; break;
		case 5: maxweight_armor = 15; break;//1.5 gloves don't exist, so it's just leather+elven here
		case 6: maxweight_armor = 40; break;//4.0 helmets don't exist (jewel encrusted crown has it though, so we just pretend..)
		}
		go_heavy = FALSE;
	}
	//Critical-strike skill isn't applied to weapons heavier than 100 lbs
	if (p_ptr->s_info[SKILL_CRITS].value >= treshold) maxweight_melee = 100;
	//Mindcrafters can't wear helmets > 4.0lbs without MP-reducing encumberment
	if (p_ptr->pclass == CLASS_MINDCRAFTER && armor_choice == 6) maxweight_armor = 40;

	/* Fruit bats/monster form hack for unwieldable mage staff */
	if (spell_choice && /* aka TV_MSTAFF, exlusively */
	    !item_tester_hook_wear(Ind, INVEN_WIELD))
		spell_choice = 0;
	/* Someone deliberately trained a melee/ranged skill even though he cannot equip that item type? Give him armour instead.
	   Note that this might also happen if someone just temporarily changed form even though his skill choice would've been correct later on;
	   we need to be strict though since he might be a fruit bat who spent skill points erroneously ;-p. */
	if (!item_tester_hook_wear(Ind, INVEN_WIELD) && melee_choice && melee_choice != 5) melee_choice = 0;
	if (!item_tester_hook_wear(Ind, INVEN_BOW) && ranged_choice) ranged_choice = 0;

	/* Choose between possible rewards we gathered from analyzing so far */
	/* Priority: Weapon -> Ranged -> Armor -> Misc */
	if (!melee_choice && spell_choice && ((!armor_choice && !ranged_choice) || magik(50))) final_choice = 4;
	if (melee_choice && ((!armor_choice && !ranged_choice) || magik(50))) final_choice = 1;
	if (ranged_choice && !final_choice && (!armor_choice || magik(50))) final_choice = 2;
	if (melee_choice == 5) final_choice = 3;
	if (misc_choice == 4 && (!final_choice || (final_choice && magik(75))) && (!armor_choice || magik(50))) final_choice = 6;
	if (armor_choice && !final_choice) final_choice = 3;
	/*if (final_choice == 3 && magik(25)) final_choice = 5; <- no misc items for now, won't be good if not (rand)arts anyway! */
	/* to catch cases where NO result has been chosen at all (paranoia): */
	if (!final_choice) final_choice = 5;

	/* Generate TV_ from raw final choice, and choose an appropriate SV_ sub-type now */
	switch (final_choice) {
	case 1: reward_maxweight = maxweight_melee;
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
		//default: reward_tval = TV_CROWN; reward_maxweight = 40; break;
		}
		break;
	case 4: reward_maxweight = 500;
		reward_tval = TV_MSTAFF; reward_sval = SV_MSTAFF;
		break;
	/* paranoia: */
	case 5: reward_maxweight = 500; /* no use, so just use any high value.. */
		switch (misc_choice) {
		case 1: reward_tval = TV_LITE; break;
		case 2: reward_tval = TV_AMULET; break;
		case 3: reward_tval = TV_RING; break;
		}
		break;
	case 6: reward_maxweight = 500;
		reward_tval = TV_TRAPKIT;
		break;
	}

	/* admin-hacked? */
	if (force_tval) {
		s_printf("CREATE_REWARD: forced tval %d.\n", force_tval);
		reward_tval = force_tval;

		/* reverse-hack choice parms just for cleanliness */
		melee_choice = ranged_choice = armor_choice = spell_choice = misc_choice = 0;
		switch (reward_tval) {
		case TV_SWORD:
			if (get_skill(p_ptr, SKILL_ANTIMAGIC)) reward_sval = SV_DARK_SWORD;
			reward_maxweight = maxweight_melee;
			final_choice = 1;
			melee_choice = 1;
			break;
		case TV_BLUNT:
			reward_maxweight = maxweight_melee;
			final_choice = 1;
			melee_choice = 2;
			break;
		case TV_AXE:
			reward_maxweight = maxweight_melee;
			final_choice = 1;
			melee_choice = 3;
			break;
		case TV_POLEARM:
			reward_maxweight = maxweight_melee;
			final_choice = 1;
			melee_choice = 4;
			break;
		case TV_SHIELD:
			reward_maxweight = maxweight_shield;
			final_choice = 1;
			melee_choice = 6;
			break;
		case TV_BOW:
			reward_maxweight = maxweight_ranged;
			final_choice = 2;
			ranged_choice = 0;
			break;
		case TV_BOOMERANG:
			reward_maxweight = maxweight_ranged;
			final_choice = 2;
			ranged_choice = 4;
			break;
		case TV_MSTAFF:
			reward_maxweight = 500;
			reward_sval = SV_MSTAFF;
			final_choice = 4;
			spell_choice = 1;
			break;
		case TV_SOFT_ARMOR:
			reward_maxweight = maxweight_armor;
			final_choice = 3;
			armor_choice = 1;
			break;
		case TV_HARD_ARMOR:
			reward_maxweight = maxweight_armor;
			final_choice = 3;
			armor_choice = 2;
			break;
		case TV_DRAG_ARMOR:
			reward_maxweight = maxweight_armor;
			final_choice = 3;
			armor_choice = 3;
			break;
		case TV_BOOTS:
			reward_maxweight = maxweight_armor;
			final_choice = 3;
			armor_choice = 4;
			break;
		case TV_GLOVES:
			reward_maxweight = maxweight_armor;
			final_choice = 3;
			armor_choice = 5;
			break;
		case TV_HELM:
			reward_maxweight = maxweight_armor;
			final_choice = 3;
			armor_choice = 6;
			break;
		case TV_CROWN:
			reward_maxweight = maxweight_armor;
			final_choice = 3;
			armor_choice = 7;
			break;
		case TV_CLOAK:
			reward_maxweight = maxweight_armor;
			final_choice = 3;
			armor_choice = 8;
			break;
		case TV_LITE:
			reward_maxweight = 500;
			final_choice = 5;
			misc_choice = 1;
			break;
		case TV_AMULET:
			reward_maxweight = 500;
			final_choice = 5;
			misc_choice = 2;
			break;
		case TV_RING:
			reward_maxweight = 500;
			final_choice = 5;
			misc_choice = 3;
			break;
		case TV_TRAPKIT:
			reward_maxweight = 500;
			final_choice = 6;
			break;
		}
	}

	/* respect unusuable slots depending on monster form */
	invwipe(&tmp_obj);
	tmp_obj.tval = reward_tval;
	/* we cannot equip the selected reward? */
	if (!item_tester_hook_wear(Ind, i = wield_slot(Ind, &tmp_obj))) {
		/* for weapons and TV_SHIELD: this shouldn't happen */
		if (tmp_obj.tval == TV_MSTAFF
#if 0 /* if someone on purpose trains a skill that doesn't fit his form (bat/mimicry), still give him the desired item */
		    || is_weapon(tmp_obj.tval)
#endif
		    ) {
			/* shouldn't happen, but.. */
			s_printf("CREATE_REWARD_PARANOIA: body %d, tv,sv = %d,%d; final %d, rha %d, mha %d, maxweights: a,m,r,s %d,%d,%d,%d, choices: m,r,a,s,i %d,%d,%d,%d,%d\n",
			    p_ptr->body_monster, tmp_obj.tval, tmp_obj.sval, final_choice, rha, mha,
			    maxweight_armor, maxweight_melee, maxweight_ranged, maxweight_shield,
			    melee_choice, ranged_choice, armor_choice, spell_choice, misc_choice);
			invcopy(o_ptr, lookup_kind(TV_SPECIAL, SV_CUSTOM_OBJECT));
			o_ptr->note = quark_add("a Cake");
			o_ptr->xtra1 = 15;
			s_printf("REWARD_CREATED: (%s) a Cake (1)\n", p_ptr->name);
			/* serious alternatives: digger, magic device, a set of consumables */
			return;
		}

		/* for armour being selected */
		else if (is_armour(tmp_obj.tval)) {
			wearables = 0;
			wearable[0] = item_tester_hook_wear(Ind, INVEN_BODY) ? 10 + 30 + 3 : 0;
			wearable[1] = item_tester_hook_wear(Ind, INVEN_OUTER) ? 10 : 0;
			wearable[2] = item_tester_hook_wear(Ind, INVEN_HEAD) ? 10 + 5 : 0;
			wearable[3] = item_tester_hook_wear(Ind, INVEN_HANDS) ? 10 : 0;
			wearable[4] = item_tester_hook_wear(Ind, INVEN_FEET) ? 10 : 0;
			wearables = wearable[0] + wearable[1] + wearable[2] + wearable[3] + wearable[4];

			/* we cannot wear ANY armour? */
			if (!wearables) {
				/* select a different item class */
				wearables = 0;
				wearable[0] = item_tester_hook_wear(Ind, INVEN_LITE) ? 1 : 0;
				wearable[1] = item_tester_hook_wear(Ind, INVEN_NECK) ? 1 : 0;
				wearable[2] = item_tester_hook_wear(Ind, INVEN_RIGHT) ? 1 : 0;
				wearables = wearable[0] + wearable[1] + wearable[2];

				/* we cannot wear ANY item? =P */
				if (!wearables) {
					/* this is silly, someone polymorphed into limbless form to turn in the reward deed?.. */
					s_printf("CREATE_REWARD: body %d, tv,sv = %d,%d; final %d, rha %d, mha %d, maxweights: a,m,r,s %d,%d,%d,%d, choices: m,r,a,s,i %d,%d,%d,%d,%d\n",
					    p_ptr->body_monster, tmp_obj.tval, tmp_obj.sval, final_choice, rha, mha,
					    maxweight_armor, maxweight_melee, maxweight_ranged, maxweight_shield,
					    melee_choice, ranged_choice, armor_choice, spell_choice, misc_choice);
#if 0
					invcopy(o_ptr, lookup_kind(TV_SPECIAL, SV_CUSTOM_OBJECT));
					o_ptr->note = quark_add("a Cake");
					o_ptr->xtra1 = 15;
					s_printf("REWARD_CREATED: (%s) a Cake (2)\n", p_ptr->name);
					/* serious alternatives: digger, magic device, a set of consumables */
					return;
				}
#else
					/* let's not make this 'exploitable' to obtain special things */
					reward_tval = TV_CLOAK;
					s_printf("REWARD_CANNOT_WEAR: default to TV_CLOAK.\n");
				} else
#endif
				{
					i = randint(wearables);
					if (i <= wearable[0]) reward_tval = TV_LITE;
					else if (i <= wearable[0] + wearable[1]) reward_tval = TV_AMULET;
					else if (i <= wearable[0] + wearable[1] + wearable[2]) reward_tval = TV_RING;
					s_printf("REWARD_CANNOT_WEAR: changed to %d\n", reward_tval);
				}
			} else { /* we can wear some sort of armour */
				i = randint(wearables);
				if (i <= wearable[0]) {
					wearables = 0;
					wearable[0] = (reward_maxweight < 240 || mha || rha) ? 10 : 0;
					wearable[1] = (reward_maxweight >= 240 && !mha && !rha) ? 30 : 0;
					wearable[2] = (reward_maxweight >= 200 && !mha && !rha) ? 3 : 0;
					wearables = wearable[0] + wearable[1] + wearable[2];
					i = randint(wearables);
					if (i <= wearable[0]) reward_tval = TV_SOFT_ARMOR;
					else if (i <= wearable[0] + wearable[1]) reward_tval = TV_HARD_ARMOR;
					else if (i <= wearable[0] + wearable[1] + wearable[2]) reward_tval = TV_DRAG_ARMOR;
				}
				else if (i <= wearable[0] + wearable[1]) reward_tval = TV_CLOAK;
				else if (i <= wearable[0] + wearable[1] + wearable[2]) reward_tval = (rand_int(3) ? TV_HELM : TV_CROWN);
				else if (i <= wearable[0] + wearable[1] + wearable[2] + wearable[3]) reward_tval = TV_GLOVES;
				else if (i <= wearable[0] + wearable[1] + wearable[2] + wearable[3] + wearable[4]) reward_tval = TV_BOOTS;
				s_printf("REWARD_CANNOT_WEAR: changed to %d\n", reward_tval);
			}
		}

		/* of course we cannot wield trapping kits, nothing to do here */
		else if (tmp_obj.tval == TV_TRAPKIT) ;

		/* for other items: */
		else {
			/* shouldn't happen, but.. */
			s_printf("CREATE_REWARD_PARANOIA: body %d, tv,sv = %d,%d; final %d, rha %d, mha %d, maxweights: a,m,r,s %d,%d,%d,%d, choices: m,r,a,s,i %d,%d,%d,%d,%d\n",
			    p_ptr->body_monster, tmp_obj.tval, tmp_obj.sval, final_choice, rha, mha,
			    maxweight_armor, maxweight_melee, maxweight_ranged, maxweight_shield,
			    melee_choice, ranged_choice, armor_choice, spell_choice, misc_choice);
			invcopy(o_ptr, lookup_kind(TV_SPECIAL, SV_CUSTOM_OBJECT));
			o_ptr->note = quark_add("a Cake");
			o_ptr->xtra1 = 15;
			s_printf("REWARD_CREATED: (%s) a Cake (3)\n", p_ptr->name);
			/* serious alternatives: digger, magic device, a set of consumables */
			return;
		}
	} else
		s_printf("CREATE_REWARD: body %d, tv,sv = %d,%d; final %d, rha %d, mha %d, maxweights: a,m,r,s %d,%d,%d,%d, choices: m,r,a,s,i %d,%d,%d,%d,%d\n",
		    p_ptr->body_monster, tmp_obj.tval, tmp_obj.sval, final_choice, rha, mha,
		    maxweight_armor, maxweight_melee, maxweight_ranged, maxweight_shield,
		    melee_choice, ranged_choice, armor_choice, spell_choice, misc_choice);

	/* In case no SVAL has been defined yet:
	   Choose a random SVAL while paying attention to maxweight limit! */
	if (!reward_sval) {
		int weapon_bpr = 0; /* try that the reward weapon does not reduce bpr compared to current bpr */
		bool weapon_2h = FALSE; /* make the reward weapon 2-handed if we're using 2-handed */

		if (is_melee_weapon(reward_tval)) { /* melee weapon */
			if (p_ptr->inventory[INVEN_WIELD].k_idx) {
				i = calc_blows_obj(Ind, &p_ptr->inventory[INVEN_WIELD]);

				/* If player purposedly uses 2h weapon, give him one.
				   We ignore 1.5h weapons (TR4_SHOULD2H) for now, since player might
				   just be using such a startup weapon without deeper thought */
				if ((k_info[p_ptr->inventory[INVEN_WIELD].k_idx].flags4 & TR4_MUST2H)) weapon_2h = TRUE;
			}
			if (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD) j = calc_blows_obj(Ind, &p_ptr->inventory[INVEN_ARM]);
			if (j > i) i = j; /* for dual-wielders, use the faster one */
			if (!i) i = 3; /* if player doesn't hold a weapon atm (why!?) just start trying for 3 bpr */
			weapon_bpr = i;
		}

		/* Check global variable, if some base types are forbidden */
		do {
			tries++;
			k_idx = 0;

			/* rings, amulets and lights don't count as good so they won't be generated (see kind_is_good).
			   Note: the whole kind_is_good_reward stuff is kinda pointless.. */
			if (reward_tval != TV_AMULET && reward_tval != TV_RING && reward_tval != TV_LITE) {
				get_obj_num_hook = kind_is_good_reward;
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
#ifndef TRAPKIT_EGO_ALL
					/* only generate bolt/arrow/shot type trapkits because others cannot gain ego powers? */
					if (k_info[k_idx].tval == TV_TRAPKIT && !is_firearm_trapkit(k_info[k_idx].sval)) continue;
#endif
					break;
				}
			else
				k_idx = get_obj_num(base, resf);

			/* Prepare the object */
			invcopy(o_ptr, k_idx);
			reward_sval = o_ptr->sval;

			/* Apply value restrictions for base item type too */
			if ((resf & RESF_NOHIDSM) && (reward_tval == TV_DRAG_ARMOR) &&
			    !sv_dsm_low(reward_sval) && !sv_dsm_mid(reward_sval))
				continue;
			/* Hack: +3000 Au to accomodate for ego power/enchantments */
			if ((resf & RESF_LOWVALUE) && (k_info[k_idx].cost + 3000 > 35000)) continue;
			if ((resf & RESF_MIDVALUE) && (k_info[k_idx].cost + 3000 > 50000)) continue;
			if ((resf & RESF_NOHIVALUE) && (k_info[k_idx].cost + 3000 > 100000)) continue;

			/* Note that in theory the item's weight might change depending on it's
			   apply_magic_depth outcome, we're ignoring that here for now though. */

			/* Check for weight limit! */
			if (o_ptr->weight > reward_maxweight) continue;

			/* No weapon that reduces bpr compared to what weapon the person currently holds! */
			if (weapon_bpr) {
				if (calc_blows_obj(Ind, o_ptr) < weapon_bpr) {
					if (tries < 70) continue; /* try hard to not lose a single bpr at first */
					if (weapon_bpr < 4 && tries < 90) continue; /* try to at least not lose a bpr if it drops us below 3 */
					if (weapon_bpr < 3) continue; /* 1 bpr is simply the worst, gotta keep trying */
				}

				/* new: try to stay with 2h weapons if we're using one
				   (except if the only available choices keep giving less bpr for some reason after 50 tries huh) */
				if (weapon_2h && !(k_info[k_idx].flags4 & TR4_MUST2H) && (tries < 50)) continue;
			}

			/* avoid super light caster armour? */
			if (go_heavy)
				switch (reward_tval) {
				case TV_HELM:
					if (reward_sval == SV_CLOTH_CAP) continue;
					break;
				case TV_SOFT_ARMOR:
					/* Note: we allow SV_ROBE because it could be permanence! always great to have.
					   Could potentially add: SV_LEATHER_FROCK (3.0), SV_PAPER_ARMOR (4.0) */
					if (reward_sval == SV_GOWN || reward_sval == SV_TUNIC || reward_sval == SV_FROCK) continue;
				}

			/* success! */
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
		invcopy(o_ptr, k_idx = lookup_kind(reward_tval, reward_sval));
	}

	/* debug log */
	s_printf("REWARD_RAW: final_choice %d, reward_tval %d, k_idx %d, tval %d, sval %d, weight %d(%d%s)\n", final_choice, reward_tval, k_idx, o_ptr->tval, o_ptr->sval, o_ptr->weight, reward_maxweight, go_heavy ? " go_heavy" : "");
	if (is_admin(p_ptr))
		msg_format(Ind, "Reward: final_choice %d, reward_tval %d, k_idx %d, tval %d, sval %d, weight %d(%d%s)", final_choice, reward_tval, k_idx, o_ptr->tval, o_ptr->sval, o_ptr->weight, reward_maxweight, go_heavy ? " go_heavy" : "");

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

	/* are we definitely going to use spells? (used for AM/MPDrain check) */
	switch (p_ptr->pclass) {
	case CLASS_SHAMAN:
	case CLASS_MAGE:
	case CLASS_RUNEMASTER:
	case CLASS_MINDCRAFTER:
#ifdef ENABLE_CPRIEST
	case CLASS_CPRIEST:
#endif
	case CLASS_PRIEST:
	case CLASS_PALADIN:
#ifdef ENABLE_DEATHKNIGHT
	case CLASS_DEATHKNIGHT:
#endif
#ifdef ENABLE_HELLKNIGHT
	case CLASS_HELLKNIGHT:
#endif
	case CLASS_DRUID:
		caster = TRUE;
	}
	if (spell_choice) caster = TRUE;

	/* apply_magic to that item, until we find a fitting one */
	tries = 0;
	i = o_ptr->k_idx;
	do {
		tries++;
		invwipe(o_ptr);
		invcopy(o_ptr, i);

		/* Apply magic (allow artifacts) */
		apply_magic_depth(base, o_ptr, base, TRUE, good, great, verygreat, resf);
		s_printf("REWARD_REAL: final_choice %d, reward_tval %d, k_idx %d, tval %d, sval %d, weight %d(%d), resf %d\n", final_choice, reward_tval, k_idx, o_ptr->tval, o_ptr->sval, o_ptr->weight, reward_maxweight, resf);
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

		/* Avoid simple Dragon Helmets etc */
		if (!o_ptr->name2 && !o_ptr->name2b && o_ptr->name1 &&
		    o_ptr->tval != TV_DRAG_ARMOR && o_ptr->tval != TV_RING && o_ptr->tval != TV_AMULET) continue;

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

		/* melee/ranged ego (ie non-spellonly): */
		switch (o_ptr->name2) {
		case EGO_AGILITY:
			/* exception: martial artists can't benefit from +BPR */
			if (melee_choice == 5) continue;
		case EGO_COMBAT:
		case EGO_SLAYING:
		case EGO_THIEVERY:
			switch (p_ptr->pclass) {
			case CLASS_WARRIOR:
			case CLASS_MIMIC:
			case CLASS_RANGER:
			case CLASS_ROGUE:
			case CLASS_MINDCRAFTER:
#ifdef ENABLE_CPRIEST
			case CLASS_CPRIEST:
#endif
			case CLASS_PRIEST:
			case CLASS_PALADIN:
#ifdef ENABLE_DEATHKNIGHT
			case CLASS_DEATHKNIGHT:
#endif
#ifdef ENABLE_HELLKNIGHT
			case CLASS_HELLKNIGHT:
#endif
			case CLASS_DRUID:
				/* ok, we pay heed to weird skilling aka all-spells MC */
				if (spell_choice && !melee_choice && !ranged_choice
				    && !o_ptr->name2b) //no double-egos exist for this combo though, iirc, w/e
					continue;
				break;
			default:
				if (!o_ptr->name2b) //no double-egos exist for this combo though, iirc, w/e
					continue;
			}
		//EGO_POWER is treated as generically beneficial
		}

		/* analyze class (so far nothing is done here, but everything is determined by skills instead) -
		   headgear ego: */
		switch (p_ptr->pclass) {
		case CLASS_WARRIOR:
		case CLASS_ARCHER:
			break;
		case CLASS_SHAMAN:
			if (!melee_choice && o_ptr->name2 == EGO_MIGHT && !o_ptr->name2b) continue;
		case CLASS_ADVENTURER:
			if ((p_ptr->stat_max[A_INT] > p_ptr->stat_max[A_WIS]) &&
			    (o_ptr->name2 == EGO_WISDOM && !o_ptr->name2b)) continue;
			if ((p_ptr->stat_max[A_WIS] > p_ptr->stat_max[A_INT]) &&
			    (o_ptr->name2 == EGO_INTELLIGENCE && !o_ptr->name2b)) continue;
			break;
		case CLASS_MAGE:
			if ((o_ptr->name2 == EGO_MIGHT || o_ptr->name2 == EGO_LORDLINESS) && !o_ptr->name2b) continue;
		case CLASS_RUNEMASTER:
			if (!melee_choice && o_ptr->name2 == EGO_MIGHT && !o_ptr->name2b) continue;
		case CLASS_RANGER:
		case CLASS_ROGUE:
		case CLASS_MINDCRAFTER:
			if (o_ptr->name2 == EGO_WISDOM && !o_ptr->name2b) continue;
			break;
#ifdef ENABLE_CPRIEST
		case CLASS_CPRIEST:
#endif
		case CLASS_PRIEST:
			if (!melee_choice && o_ptr->name2 == EGO_MIGHT && !o_ptr->name2b) continue;
		case CLASS_PALADIN:
#ifdef ENABLE_DEATHKNIGHT
		case CLASS_DEATHKNIGHT:
#endif
#ifdef ENABLE_HELLKNIGHT
		case CLASS_HELLKNIGHT:
#endif
		case CLASS_DRUID:
			if (o_ptr->name2 == EGO_INTELLIGENCE && !o_ptr->name2b) continue;
			break;
		}

#if 0 /* let the hres/pres check take care of this */
		if (o_ptr->name2 == EGO_MIRKWOOD && !o_ptr->name2b) {
			if (p_ptr->prace == RACE_VAMPIRE ||
			    (p_ptr->prace == RACE_DRACONIAN && p_ptr->ptrait == TRAIT_GREEN))
				continue;
		}
#endif

		/* no anti-undead items for vampires */
		if (p_ptr->prace == RACE_VAMPIRE && anti_undead(o_ptr)) continue;
#ifdef ENABLE_HELLKNIGHT
		/* no anti-demon items for hell knights */
		if (p_ptr->pclass == CLASS_HELLKNIGHT && anti_demon(o_ptr)) continue;
#endif

		/* Don't generate NO_MAGIC or DRAIN_MANA items if we do use magic */
		if (caster) {
			if (f5 & TR5_DRAIN_MANA) continue;
			if (f3 & TR3_NO_MAGIC) continue;
		}
		/* Don't generate DRAIN_HP items (Spectral) - except for Vampires who are unaffected */
		if (f5 & TR5_DRAIN_HP) {
			if ((o_ptr->name2 != EGO_SPECTRAL && o_ptr->name2b != EGO_SPECTRAL) || p_ptr->prace != RACE_VAMPIRE) continue;
		}
		/* Don't generate problematic items at all */
		if (f3 & (TR3_AGGRAVATE | TR3_DRAIN_EXP | TR3_NO_TELE)) continue;

		/* Don't generate too useless rewards depending on intrinsic character abilities */
		if ((p_ptr->hold_life || p_ptr->free_act) &&
		    ((o_ptr->name2 == EGO_CLOAK_TELERI && (!o_ptr->name2b || o_ptr->name2b == EGO_GNOMISH)) ||
		    (o_ptr->name2b == EGO_CLOAK_TELERI && o_ptr->name2 == EGO_GNOMISH)))
			continue;
		if (p_ptr->hold_life && p_ptr->free_act &&
		    (o_ptr->name2 == EGO_CLOAK_TELERI || o_ptr->name2b == EGO_CLOAK_TELERI))
			continue;

		/* Don't generate mage-only benefitting reward if we don't use magic */
		if (!spell_choice && !o_ptr->name2b) { /* as _double ego_, it should be acceptable :-p */
			switch (o_ptr->name2) {
			//case EGO_MAGI: /* crown of magi, it's not bad for anyone actually */
			//case EGO_CLOAK_MAGI: /* well, it does provide speed.. */
			case EGO_INTELLIGENCE:
			case EGO_WISDOM:
			case EGO_BRILLIANCE:
			case EGO_OFTHEMAGI:
			case EGO_ISTARI:
				continue;
			}
		}

		/* single-ego restrictions */
		if (!o_ptr->name2b) {
			switch (o_ptr->name2) {
			/* don't generate items that are too worthless at low pval */
			case EGO_INTELLIGENCE:
			case EGO_WISDOM:
				if (o_ptr->pval < 4) o_ptr->pval = 4 + rand_int(2);
				break;
			case EGO_BRILLIANCE:
				if (o_ptr->pval < 3) o_ptr->pval = 3 + (rand_int(3) ? 0 : 1);
				break;
			case EGO_OFTHEMAGI:
				if (o_ptr->pval < 5) o_ptr->pval = 5; //higher pvals are commonly used towards end-game, no need to hack it up further now
				break;
			case EGO_AGILITY:
				if (o_ptr->pval < 4) o_ptr->pval = 4 + rand_int(2);
				break;
			/* Don't generate (possibly expensive due to high bpval or high +ac, hence passed up till here) definite crap */
			case EGO_CONCENTRATION:
			case EGO_INFRAVISION:
			case EGO_BEAUTY:
			case EGO_CHARMING:
			case EGO_NOLDOR: //well, could give +1 BPR and useful in Orc Cave actually =P -- still leaving it here though because pval is limited to just +2 in e_info
			case EGO_SLOW_DESCENT: //has a high res, but still maybe not helpful enough
			case EGO_STABILITY: //since EGO_SLOW_DESCENT was added too..

			case EGO_MARTIAL:
			case EGO_FROCK_PIETY:
			case EGO_ROBE_MAGI:
				continue;
			}
		}

#ifdef TRAPKIT_EGO_ALL
		/* actually prevent restrictive ego powers! would be too useless in early game stages! */
		if (o_ptr->tval == TV_TRAPKIT) {
			/* note about this hack:
			   we assume that the stat-changes done to the base item by ego'ing it
			   are the same for TEVIL as for the others, so we don't have to adjust
			   them and can just hack the ego type here simply. */
			switch (o_ptr->name2) {
			case EGO_TDRAGON:
			case EGO_TDEMON:
			case EGO_TANIMAL:
			case EGO_TUNDEAD:
				o_ptr->name2 = EGO_TEVIL;
				break;
			default:
				switch (o_ptr->name2b) {
				case EGO_TDRAGON:
				case EGO_TDEMON:
				case EGO_TANIMAL:
				case EGO_TUNDEAD:
					o_ptr->name2b = EGO_TEVIL;
				}
			}
		}
#endif

		/* a reward should have some value: */
		if (object_value_real(0, o_ptr) < 5000) continue;
#if 0 /* these no-weak-ego checks are basically already covered by above EGO_ checks. Here they actually hinder trap kit generation badly, so these should just remain disabled. */
		if (o_ptr->name2) { /* should always be true actually! just paranoia */
			if (e_info[o_ptr->name2].cost <= 2000) {
				if (o_ptr->name2b) {
					if (e_info[o_ptr->name2b].cost <= 2000) continue;
				} else continue;
			}
		}
#endif

		/* prevent rewards that pass the min price check but are still not useful enough: */
#if 1
		/* no pval +1 items, as the bonus is maybe too weak - especially: Cloak of the Magi
		   -- note: this prevents 1h +LIFE weapons as a side-effect */
		if (resf & RESF_NOHIVALUE) { /* eg Dungeon Keeper */
			/* prevent */
			if (o_ptr->pval == 1) continue;
		} else { /* eg Highlander */
			/* boost */
			if (o_ptr->pval == 1 && o_ptr->name2 != EGO_LIFE && o_ptr->name2b != EGO_LIFE) o_ptr->pval = 2;
		}
#else
		/* - prevent +1 speed boots! */
		if ((o_ptr->name2 == EGO_SPEED || o_ptr->name2b == EGO_SPEED) && o_ptr->pval == 1) continue;
#endif
		/* - prevent shields of reflection */
		if (o_ptr->name2 == EGO_REFLECT || o_ptr->name2b == EGO_REFLECT) continue;
		/* - it's a bit sad to get super-low mage staves (of Mana, +1..4) */
		if (o_ptr->name2 == EGO_MMANA || o_ptr->name2b == EGO_MMANA)
			//continue;
			if (o_ptr->pval < 4) o_ptr->pval = 4;
		//note: thievery gloves +1 speed are ok!

		/* Limit for mage staves so they don't end up as top end gear (Wiz: 8..10) */
		if ((o_ptr->name2 == EGO_MWIZARDRY || o_ptr->name2b == EGO_MWIZARDRY) && o_ptr->pval > 8) o_ptr->pval = 8;

		/* specialty: for runemasters, if it's armour, make sure it resists (backlash) at least one of the elements we can cast :) */
		if (p_ptr->pclass == CLASS_RUNEMASTER && is_armour(o_ptr->tval)) {
			bool rlite = (p_ptr->s_info[SKILL_R_LITE].value > 0);
			bool rdark = (p_ptr->s_info[SKILL_R_DARK].value > 0);
			bool rnexu = (p_ptr->s_info[SKILL_R_NEXU].value > 0);
			bool rneth = (p_ptr->s_info[SKILL_R_NETH].value > 0);
			bool rchao = (p_ptr->s_info[SKILL_R_CHAO].value > 0);
			bool rmana = (p_ptr->s_info[SKILL_R_MANA].value > 0);
			bool rconf = (rlite && rdark);
			bool relec = (rlite && rneth);
			bool rfire = (rlite && rchao);
			bool rcold = (rdark && rneth);
			bool racid = (rdark && rchao);
			bool rpois = (rdark && rmana);
			bool rsoun = (rnexu && rchao);
			bool rshar = (rnexu && rmana);
			bool rdise = (rneth && rchao);
			u32b relem2, relem5;

			if (p_ptr->resist_fire || p_ptr->immune_fire) rfire = FALSE;
			if (p_ptr->resist_cold || p_ptr->immune_cold) rcold = FALSE;
			if (p_ptr->resist_acid || p_ptr->immune_acid) racid = FALSE;
			if (p_ptr->resist_elec || p_ptr->immune_elec) relec = FALSE;
			if (p_ptr->resist_pois || p_ptr->immune_poison) rpois = FALSE;
			if (p_ptr->resist_lite) rlite = FALSE;
			if (p_ptr->resist_dark) rdark = FALSE;
			if (p_ptr->resist_nexus) rnexu = FALSE;
			if (p_ptr->resist_neth) rneth = FALSE;
			if (p_ptr->resist_chaos) rchao = rconf = FALSE;
			if (p_ptr->resist_conf) rconf = FALSE;
			if (p_ptr->resist_sound) rsoun = FALSE;
			if (p_ptr->resist_shard) rshar = FALSE;
			if (p_ptr->resist_disen) rdise = FALSE;

			/* note: skip hard/impossible to acquire elements */
			relem2 = ((rlite ? TR2_RES_LITE : 0x0) | (rdark ? TR2_RES_DARK : 0x0) |
			    (rnexu ? TR2_RES_NEXUS : 0x0) | (rneth ? TR2_RES_NETHER : 0x0) |
			    (rchao ? TR2_RES_CHAOS : 0x0) |
			    (rfire ? TR2_RES_FIRE | TR2_IM_FIRE : 0x0) | (rcold ? TR2_RES_COLD | TR2_IM_COLD : 0x0) |
			    (relec ? TR2_RES_ELEC | TR2_IM_ELEC : 0x0) | (racid ? TR2_RES_ACID | TR2_IM_ACID : 0x0) |
			    (rpois ? TR2_RES_POIS : 0x0) |
			    (rconf ? TR2_RES_CONF | TR2_RES_CHAOS : 0x0) | (rsoun ? TR2_RES_SOUND : 0x0) |
			    (rshar ? TR2_RES_SHARDS : 0x0) | (rdise ? TR2_RES_DISEN : 0x0));
			relem5 = (rpois ? TR5_IM_POISON : 0x0);

			if ((relem2 || relem5) && !(f2 & relem2) && !(f5 & relem5)) continue;
		}

		/* If the item only has high resistance, and no immunity, make sure that we don't already resist it. */
		if (!(f2 & (TR2_IM_FIRE | TR2_IM_COLD | TR2_IM_ACID | TR2_IM_ELEC)) && !(f5 & TR5_IM_POISON)) {
			int hres = 0;
			int pres = 0;

			if (f2 & TR2_RES_NEXUS) {
				hres++;
				if (p_ptr->resist_nexus) pres++;
			}
			if (f2 & TR2_RES_NETHER) {
				hres++;
				if (p_ptr->resist_neth) pres++;
			}
			if (f2 & TR2_RES_CHAOS) {
				hres++;
				if (p_ptr->resist_chaos) pres++;
			}
			if (f2 & TR2_RES_POIS) {
				hres++;
				if (p_ptr->resist_pois) pres++;
			}
			if (f2 & TR2_RES_SOUND) {
				hres++;
				if (p_ptr->resist_sound) pres++;
			}
			if (f2 & TR2_RES_SHARDS) {
				hres++;
				if (p_ptr->resist_shard) pres++;
			}
			if (f2 & TR2_RES_DISEN) {
				hres++;
				if (p_ptr->resist_disen) pres++;
			}

			/* semi-great */
			if (f2 & TR2_RES_DARK) {
				hres++;
				if (p_ptr->resist_dark) pres++;
			}
			if (f2 & TR2_RES_LITE) {
				hres++;
				if (p_ptr->resist_lite) pres++;
			}

			/* less great */
			if (f2 & TR2_RES_CONF) {
				hres++;
				if (p_ptr->resist_conf) pres++;
			}
			if (f2 & TR2_RES_BLIND) {
				hres++;
				if (p_ptr->resist_blind) pres++;
			}

			/* misc */
			//if (f5 & TR5_RES_WATER) hres++;
			//PLASMA, MANA, TIME, FEAR
			//don't check for FA, to make sure Vampires/GreenD don't get mirkwood boots

			if (hres && pres == hres) continue;
		}

		/* If the item is a dragon scale mail and we're draconian, prevent redundant immunity.
		   (Very ugly hacking, since we change sval and k_idx on the fly and
		    just assume that no other base item features need to be modified.) */
		if (o_ptr->tval == TV_DRAG_ARMOR) {
			switch (p_ptr->prace) {
			case RACE_DRACONIAN:
				//TODO - switch for trait instead of dsm-type: switch (p_ptr->ptrait) {
				switch (o_ptr->sval) {
				case SV_DRAGON_BLUE:
					if (p_ptr->ptrait == TRAIT_BLUE) {
						o_ptr->sval = randint(4);
						if (o_ptr->sval >= SV_DRAGON_BLUE) o_ptr->sval++;
					}
					break;
				case SV_DRAGON_WHITE:
					if (p_ptr->ptrait == TRAIT_WHITE) {
						o_ptr->sval = randint(4);
						if (o_ptr->sval >= SV_DRAGON_WHITE) o_ptr->sval++;
					}
					break;
				case SV_DRAGON_RED:
					if (p_ptr->ptrait == TRAIT_RED) {
						o_ptr->sval = randint(4);
						if (o_ptr->sval >= SV_DRAGON_RED) o_ptr->sval++;
					}
					break;
				case SV_DRAGON_BLACK:
					if (p_ptr->ptrait == TRAIT_BLACK) {
						o_ptr->sval = randint(4);
						if (o_ptr->sval >= SV_DRAGON_BLACK) o_ptr->sval++;
					}
					break;
				case SV_DRAGON_GREEN:
					if (p_ptr->ptrait == TRAIT_GREEN) o_ptr->sval = randint(4);
					break;
				case SV_DRAGON_BRONZE:
					if (p_ptr->ptrait == TRAIT_BRONZE)
						switch (rand_int(2)) {
						case 0: o_ptr->sval = SV_DRAGON_SILVER; break;
						case 1: o_ptr->sval = SV_DRAGON_GOLD; break;
						}
					break;
				case SV_DRAGON_SILVER:
					if (p_ptr->ptrait == TRAIT_WHITE)
						switch (rand_int(2)) {
						case 0: o_ptr->sval = SV_DRAGON_BRONZE; break;
						case 1: o_ptr->sval = SV_DRAGON_GOLD; break;
						}
					//still ok for silver lineage, we gain IM+FA
					break;
				case SV_DRAGON_GOLD:
					if (p_ptr->ptrait == TRAIT_GOLD)
						switch (rand_int(2)) {
						case 0: o_ptr->sval = SV_DRAGON_BRONZE; break;
						case 1: o_ptr->sval = SV_DRAGON_SILVER; break;
						}
					break;
				case SV_DRAGON_LAW:
					if (p_ptr->ptrait == TRAIT_LAW) o_ptr->sval = SV_DRAGON_CHAOS;
					break;
				case SV_DRAGON_CHAOS:
					if (p_ptr->ptrait == TRAIT_CHAOS) o_ptr->sval = SV_DRAGON_LAW;
					break;
				//case SV_DRAGON_BALANCE: //it's fine, we gain shard+chaos!
				case SV_DRAGON_DRACOLICH:
					if (p_ptr->ptrait == TRAIT_WHITE)
						switch (rand_int(4)) {
						case 0: o_ptr->sval = SV_DRAGON_LAW; break;
						case 1: o_ptr->sval = SV_DRAGON_CHAOS; break;
						case 2: o_ptr->sval = SV_DRAGON_CRYSTAL; break;
						case 3: o_ptr->sval = SV_DRAGON_DRACOLISK; break;
						}//SKY might exceed the reward value (actually CRYSTAL is also worth a bit more than DRACOLICH already
					break;
				case SV_DRAGON_DRACOLISK:
					if (p_ptr->ptrait == TRAIT_RED)
						switch (rand_int(4)) {
						case 0: o_ptr->sval = SV_DRAGON_LAW; break;
						case 1: o_ptr->sval = SV_DRAGON_CHAOS; break;
						case 2: o_ptr->sval = SV_DRAGON_CRYSTAL; break;
						case 3: o_ptr->sval = SV_DRAGON_DRACOLICH; break;
						}//SKY might exceed the reward value (actually CRYSTAL is also worth a bit more than DRACOLICH already
					break;
				}
				break;
			case RACE_VAMPIRE:
				switch (o_ptr->sval) {
				case SV_DRAGON_DRACOLICH:
					switch (rand_int(4)) {
					case 0: o_ptr->sval = SV_DRAGON_LAW; break;
					case 1: o_ptr->sval = SV_DRAGON_CHAOS; break;
					case 2: o_ptr->sval = SV_DRAGON_CRYSTAL; break;
					case 3: o_ptr->sval = SV_DRAGON_DRACOLISK; break;
					}//SKY might exceed the reward value (actually CRYSTAL is also worth a bit more than DRACOLICH already
					break;
				}
				break;
			}

			/* finally hack k_idx accordingly */
			o_ptr->k_idx = lookup_kind(o_ptr->tval, o_ptr->sval);
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
void give_reward(int Ind, u32b resf, cptr quark, int level, int discount) {
	object_type forge, *o_ptr = &forge;
	create_reward(Ind, o_ptr, 95, 95, TRUE, TRUE, resf, 3000);
	object_aware(Ind, o_ptr);
	object_known(o_ptr);
	if (o_ptr->tval != TV_SPECIAL) o_ptr->discount = discount;
	o_ptr->level = level;
	o_ptr->ident |= ID_MENTAL;
	if (quark && !o_ptr->note) o_ptr->note = quark_add(quark);
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
/*note: This function uses completely bad values for picking a gold 'colour' at first and should be rewritten.
  I added a hack that resets the colour to something feasible so not almost every high level pile is adamantite. */
void place_gold(struct worldpos *wpos, int y, int x, int bonus) {
	int i, k_idx;
	s32b base;
	object_type forge;
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return;

	/* Paranoia -- check bounds */
	if (!in_bounds(y, x)) return;
	
	/* not in Valinor */
	if (in_valinor(wpos)) return;

	/* Require clean floor grid */
//	if (!cave_clean_bold(zcave, y, x)) return;

	/* Hack -- Pick a Treasure variety */
	i = ((randint(object_level + 2) + 2) / 2);

	/* Apply "extra" magic */
	if (rand_int(GREAT_OBJ) == 0)
		i += randint(object_level + 1);

	/* Hack -- Creeping Coins only generate "themselves" */
	if (coin_type) i = coin_type;

//s_printf("pg: ol=%d,i=%d,ct=%d\n", object_level, i, coin_type);
	/* Do not create "illegal" Treasure Types */
	if (i > SV_GOLD_MAX) i = SV_GOLD_MAX;

	k_idx = lookup_kind(TV_GOLD, i);
	invcopy(&forge, k_idx);

	/* Hack -- Base coin cost */
//	base = k_info[OBJ_GOLD_LIST+i].cost;
	base = k_info[k_idx].cost;

	/* Determine how much the treasure is "worth" */
	forge.pval = (base + (8L * randint(base)) + randint(8)) + bonus;

	/* hacking this mess of an outdated function: pick a 'colour' */
	/* hack -- Creeping Coins only generate "themselves" */
	if (coin_type) {
		forge.sval = coin_type;
		forge.xtra2 = 1; //mark as "don't change colour on stacking up"
	} else {
		forge.k_idx = gold_colour(forge.pval, TRUE, FALSE);
		forge.sval = k_info[forge.k_idx].sval;
	}

	if (opening_chest) {
		forge.owner = opening_chest_owner;
		forge.mode = opening_chest_mode;
	}

	/* Drop it */
	drop_near(0, &forge, -1, wpos, y, x);
}



/* Test whether The One Ring just got dropped into lava in Mt. Doom.
   If so, erase it and weaken Sauron for his next encounter.  - C. Blue

   A bit silly side effect: If the ring gets dropped by a monster,
   the same thing happens :-p.

   (Note that an 'item crash' ie an art destroying everything below it
   can only happen at !wpos->wz && CAVE_ICKY ie in actual houses, so we
   don't need to check for it in any way.)
*/
static bool dropped_the_one_ring(struct worldpos *wpos, cave_type *c_ptr) {
	dungeon_type *d_ptr = getdungeon(wpos);

	/* not in Mt Doom? */
	if (in_irondeepdive(wpos)) {
		if (iddc[ABS(wpos->wz)].type != DI_MT_DOOM) return FALSE;
	} else if (!d_ptr || d_ptr->type != DI_MT_DOOM) return FALSE;

	/* grid isn't lava or 'fire'? */
	switch (c_ptr->feat) {
	case FEAT_SHAL_LAVA:
	case FEAT_DEEP_LAVA:
	case FEAT_FIRE:
	case FEAT_GREAT_FIRE:
		break;
	default:
		return FALSE;
	}

	/* lands safely on top of a loot pile? :-p */
	if (c_ptr->o_idx) return FALSE;

	/* doesn't land in lava? */
	if (c_ptr->feat != FEAT_SHAL_LAVA && c_ptr->feat != FEAT_DEEP_LAVA) return FALSE;

	/* destroy it and weaken Sauron! */
	handle_art_d(ART_POWER);
	msg_broadcast(0, "\374\377f** \377oSauron, the Sorceror has been greatly weakened! \377f**");
#ifdef USE_SOUND_2010
	/* :-o double sfx! */
	sound_floor_vol(wpos, "thunder", NULL, SFX_TYPE_MISC, 100);
	sound_floor_vol(wpos, "detonation", NULL, SFX_TYPE_MISC, 100);
#endif
	if (in_irondeepdive(wpos)) {
		/* check if Sauron is already spawned */
		int k;
		bool found = FALSE;
		monster_type *m_ptr;

		for (k = m_top - 1; k >= 0; k--) {
			m_ptr = &m_list[m_fast[k]];
			if (m_ptr->r_idx != RI_SAURON) continue;

			found = TRUE;
			m_ptr->speed -= 5;
			m_ptr->mspeed -= 5;
			m_ptr->hp = (m_ptr->hp * 1) / 2;
			m_ptr->maxhp = (m_ptr->maxhp * 1) / 2;
			break;
		}
		if (!found) sauron_weakened_iddc = TRUE;
	} else {
		/* check if Sauron is already spawned */
		int k;
		bool found = FALSE;
		monster_type *m_ptr;

		for (k = m_top - 1; k >= 0; k--) {
			m_ptr = &m_list[m_fast[k]];
			if (m_ptr->r_idx != RI_SAURON) continue;

			found = TRUE;
			m_ptr->speed -= 5;
			m_ptr->mspeed -= 5;
			m_ptr->hp = (m_ptr->hp * 1) / 2;
			m_ptr->maxhp = (m_ptr->maxhp * 1) / 2;
			break;
		}
		if (!found) sauron_weakened = TRUE;
	}
	return TRUE;
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
//#define DROP_KILL_NOTE /* todo: needs adjustments - see below */
#define DROP_ON_STAIRS_IN_EMERGENCY
s16b drop_near(int Ind, object_type *o_ptr, int chance, struct worldpos *wpos, int y, int x) {
	int k, d, ny, nx, i, s;	// , y1, x1
	int bs, bn;
	int by, bx;
	//int ty, tx;
	int o_idx = -1;
	int flag = 0;	// 1 = normal, 2 = combine, 3 = crash

	cave_type	*c_ptr;

	bool comb;
	/* for destruction checks */
	bool do_kill = FALSE;
#ifdef DROP_KILL_NOTE
	bool is_potion = FALSE, plural = FALSE;
	cptr note_kill = NULL;
#endif
#ifdef DROP_ON_STAIRS_IN_EMERGENCY
	bool allow_stairs = FALSE;
#endif
	u32b f1, f2, f3, f4, f5, f6, esp;

	bool arts = artifact_p(o_ptr), crash;
	u16b this_o_idx, next_o_idx = 0;

	cave_type **zcave;
	monster_race *r_ptr;


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
		comb = FALSE;

		if (i >= tdi[d]) {
#ifdef DROP_ON_STAIRS_IN_EMERGENCY
			/* New hack for vaults that have mountain separators:
			   If player spawns in them on a staircase he took and gets disarmed right away,
			   the weapon can't drop onto the staircase and will get erased!
			   Similar happens for emerging from an enclosed void jump gate.
			   So allow dropping items onto stairs in emergency cases. */

			/* Scan once more in rad 0 and 1, but this time allow staircase grids too */
			if (d == 1) {
				if (!flag && !allow_stairs) {
					allow_stairs = TRUE;
					d = 0;
					i = tdi[d] - 1;
					continue;
				}
				allow_stairs = FALSE;
			}
#endif
			d++;
		}

		/* Location */
		ny = y + tdy[i];
		nx = x + tdx[i];

		/* Skip illegal grids */
		if (!in_bounds(ny, nx)) continue;

		/* Require line of sight */
		if (!los(wpos, y, x, ny, nx)) continue;

		/* Obtain grid */
		c_ptr = &zcave[ny][nx];

		/* Require floor space (or shallow terrain) -KMW- */
		//if (!(f_info[c_ptr->feat].flags1 & FF1_FLOOR)) continue;
		if (!cave_floor_bold(zcave, ny, nx) ||
		    /* Usually cannot drop items on permanent features,
		       exception for stairs/gates though in case of emergency */
		    (cave_perma_bold(zcave, ny, nx)
#ifdef DROP_ON_STAIRS_IN_EMERGENCY
		     && !(allow_stairs && is_stair(c_ptr->feat))
#endif
		     ))
		        continue;

		/* not on open house doors! -
		   added this to prevent items landing ON an open door of a list house,
		   making it impossible to pick up the item again because the character
		   would enter the house when trying to step onto the grid with the item. - C. Blue */
		if (zcave[ny][nx].feat == FEAT_HOME_OPEN) continue;

		/* Hack: Don't drop items below immovable unkillable monsters aka the
		   Target Dummy, so players can get their items (ammo) back - C. Blue */
		if (c_ptr->m_idx > 0) {
			r_ptr = race_inf(&m_list[c_ptr->m_idx]);
			if (((r_ptr->flags1 & RF1_NEVER_MOVE) ||
			    (r_ptr->flags7 & RF7_NEVER_ACT)) &&
			    (r_ptr->flags7 & RF7_NO_DEATH))
			continue;
		}

		/* No traps */
		//if (c_ptr->t_idx) continue;

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
			if (object_similar(Ind, o_ptr, j_ptr, 0x4)) comb = TRUE;

			/* Count objects */
			k++;
		}

		/* Add new object */
		if (!comb) k++;

		/* No stacking (allow combining) */
		//if (!testing_stack && (k > 1)) continue;

		/* Hack -- no stacking inside houses - nor inside the inn */
		/* XXX this can cause 'arts crashes arts' */
		crash = (!wpos->wz && k > 1 && !comb && ((c_ptr->info & (CAVE_ICKY | CAVE_PROT)) || (f_info[c_ptr->feat].flags1 & FF1_PROTECTED)));
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

#if 1 /* extra logging for artifact timeout debugging */
		if (true_artifact_p(o_ptr) && o_ptr->owner) {
			cptr name = lookup_player_name(o_ptr->owner);
			int lev = lookup_player_level(o_ptr->owner);
			char o_name[ONAME_LEN];

			object_desc_store(Ind, o_name, o_ptr, TRUE, 3);

			s_printf("%s owned true artifact failed to drop by %s(%d) at (%d,%d,%d):\n  %s\n",
			    showtime(), name ? name : "(Dead player)", lev,
			    wpos->wx, wpos->wy, wpos->wz,
			    o_name);
		}
#endif

		if (true_artifact_p(o_ptr)) handle_art_d(o_ptr->name1);
		questitem_d(o_ptr, o_ptr->number);
		return (-1);
	}

	ny = by;
	nx = bx;
	c_ptr = &zcave[ny][nx];

	if (o_ptr->name1 == ART_POWER && dropped_the_one_ring(wpos, c_ptr)) return -1;

	/* some objects get destroyed by falling on certain floor type - C. Blue */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
#ifdef DROP_KILL_NOTE
	if (o_ptr->tval == TV_POTION) is_potion = TRUE;
	if (o_ptr->number > 1) plural = TRUE;
#endif
	switch (c_ptr->feat) {
	case FEAT_SHAL_WATER:
	case FEAT_DEEP_WATER:
	case FEAT_GLIT_WATER:
	//case FEAT_WATER:
	//case FEAT_TAINTET_WATER:
		if (hates_water(o_ptr)) {
			do_kill = TRUE;
#ifdef DROP_KILL_NOTE
			note_kill = (plural ? " are soaked!" : " is soaked!");
#endif
			if (f5 & TR5_IGNORE_WATER) do_kill = FALSE;
		}
		break;
	case FEAT_SHAL_LAVA:
	case FEAT_DEEP_LAVA:
	case FEAT_FIRE:
	case FEAT_GREAT_FIRE:
		if (hates_fire(o_ptr)) {
			do_kill = TRUE;
#ifdef DROP_KILL_NOTE
			note_kill = is_potion ? (plural ? " evaporate!" : " evaporates!") : (plural ? " burn up!" : " burns up!");
#endif
			if (f3 & TR3_IGNORE_FIRE) do_kill = FALSE;
		}
		break;
	}

	if (do_kill) {
#ifdef DROP_KILL_NOTE //needs adjustments
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
		questitem_d(o_ptr, o_ptr->number);
		return (-1);
	}

	/* Artifact always disappears, depending on tomenet.cfg flags */
	/* this should be in drop_near_severe, would be cleaner sometime in the future.. */
	if (wpos->wz == 0) { /* Assume houses are always on surface */
		if (undepositable_artifact_p(o_ptr) && cfg.anti_arts_house && inside_house(wpos, nx, ny)) {
			//char o_name[ONAME_LEN];
			//object_desc(Ind, o_name, o_ptr, TRUE, 0);
			//msg_format(Ind, "%s fades into the air!", o_name);
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
		if (object_similar(Ind, o_ptr, q_ptr, 0x4)) {
			/* Combine the items */
			object_absorb(0, q_ptr, o_ptr);

			/* for player-store 'offer' check */
			o_idx = this_o_idx;

			/* Done */
			break;
		}
	}

	/* Successful drop */
	//if (flag)
	else {
		/* Assume fails */
		//flag = FALSE;

		/* XXX XXX XXX */

		//c_ptr = &zcave[ny][nx];

		/* Crush anything under us (for artifacts) */
		if (flag == 3) delete_object(wpos, ny, nx, TRUE);


#ifdef MAX_ITEMS_STACKING
		/* limit max stack size */
		if (c_ptr->o_idx &&
		    MAX_ITEMS_STACKING != 0 &&
		    o_list[c_ptr->o_idx].stack_pos >= MAX_ITEMS_STACKING - 1) {
			/* unique monster drops get priority and 'crash' all previous objects.
			   This should be preferable over just deleting the top object, if the
			   unique monster drops multiple objects, which is true in most cases. */
 #if 0 /* rough way */
			//TODO: better handling of unique loot: pick one normal item and destroy it, to make room for one unique boss item.
			if (o_ptr->note_utag &&
			    o_list[c_ptr->o_idx].note_utag == 0) { /* hold back a bit if we'd have to destroy other unique loot;
								    note: this ignores cases of unique loot below a normal item though. */
				delete_object(wpos, ny, nx, TRUE);
			}
			/* can't drop! */
			else
 #else /* more refined way */
			/* if we drop unique monster loot, look for a normal item and destroy it to make room */
			do_kill = TRUE;
			if (o_ptr->note_utag) {
				/* scan pile from top to bottom */
				for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
					/* found a non-unique-loot item? */
					if (!o_list[this_o_idx].note_utag) {
						/* erase it */
						delete_object_idx(this_o_idx, TRUE);
						/* done */
						do_kill = FALSE;
						break;
					}
					/* Acquire next object */
					next_o_idx = o_list[this_o_idx].next_o_idx;
				}
			}
			if (do_kill)
 #endif
			{
				if (true_artifact_p(o_ptr)) handle_art_d(o_ptr->name1); /* just paranoia here */
				questitem_d(o_ptr, o_ptr->number);
				return (-1);
			}
		}
#endif

		/* Make a new object */
		o_idx = o_pop();

		/* Sigil (reset it) */
		if (o_ptr->sigil) {
			//msg_print(Ind, "The sigil fades away.");
			o_ptr->sigil = 0;
			o_ptr->sseed = 0;
		}

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
			if (o_ptr->tval == TV_GAME) o_ptr->marked2 = ITEM_REMOVAL_NEVER;

			/* items dropped into a house (well or a vault
			   on surface if such exists) are marked to not
			   get removed by timeout check, allowing us to
			   additionally check and delete objects on
			   unallocated levels - C. Blue */
			if (o_ptr->marked2 != ITEM_REMOVAL_NEVER) {
				if (wpos->wz == 0 && (c_ptr->info & CAVE_ICKY) && !(c_ptr->info & CAVE_JAIL)) {
					/* mark as 'inside a house' */
					o_ptr->marked2 = ITEM_REMOVAL_HOUSE;
				} else if (o_ptr->marked2 != ITEM_REMOVAL_DEATH_WILD &&
				    o_ptr->marked2 != ITEM_REMOVAL_LONG_WILD) {
					/* clear possible previous ITEM_REMOVAL_HOUSE mark */
					o_ptr->marked2 = ITEM_REMOVAL_NORMAL;
				}
			}

			/* items dropped in pvp arena are deleted quickly - C. Blue */
			if (in_pvparena(wpos)) o_ptr->marked2 = ITEM_REMOVAL_QUICK;

#ifdef ALLOW_NR_CROSS_ITEMS
			/* Allow the item to be traded as long as it doesn't leave the Nether Realm - C. Blue */
			if (!o_ptr->owner && in_netherrealm(wpos)) o_ptr->NR_tradable = TRUE;
#endif

			/* No monster */
			o_ptr->held_m_idx = 0;

			/* Build a stack */
			o_ptr->next_o_idx = c_ptr->o_idx;
#ifdef MAX_ITEMS_STACKING
			if (c_ptr->o_idx && MAX_ITEMS_STACKING != 0)
				o_ptr->stack_pos = o_list[c_ptr->o_idx].stack_pos + 1;
			else
#endif
				o_ptr->stack_pos = 0; /* first object on this grid */

			/* Place */
			//c_ptr = &zcave[ny][nx];
			c_ptr->o_idx = o_idx;
			nothing_test2(c_ptr, nx, ny, wpos, 2);

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
				msg_print("You feel something roll beneath your feet.");*/
			if (chance && c_ptr->m_idx < 0)
				msg_print(0 - c_ptr->m_idx, "You feel something roll beneath your feet.");

			/* Success */
			//flag = TRUE;
		} else /* paranoia: couldn't allocate a new object */ {
			if (true_artifact_p(o_ptr)) handle_art_d(o_ptr->name1);
			questitem_d(o_ptr, o_ptr->number);
		}
	}

#if 1 /* extra logging for artifact timeout debugging */
	if (true_artifact_p(o_ptr) && o_ptr->owner) {
		cptr name = lookup_player_name(o_ptr->owner);
		int lev = lookup_player_level(o_ptr->owner);
		char o_name[ONAME_LEN];

		object_desc_store(Ind, o_name, o_ptr, TRUE, 3);

		s_printf("%s owned true artifact (oidx=%d) dropped by %s(%d) at (%d,%d,%d):\n  %s\n",
		    showtime(), o_idx, name ? name : "(Dead player)", lev,
		    wpos->wx, wpos->wy, wpos->wz,
		    o_name);
	}
#endif

	/* Result */
	return (o_idx);
}
/* This function make the artifact disapper at once (cept randarts),
 * and call the normal dropping function otherwise.
 */

s16b drop_near_severe(int Ind, object_type *o_ptr, int chance, struct worldpos *wpos, int y, int x) {
	player_type *p_ptr = Players[Ind];

	/* Items dropped by admins never disappear by 'time out' */
	if (is_admin(p_ptr)) o_ptr->marked2 = ITEM_REMOVAL_NEVER;

	/* Artifact always disappears, depending on tomenet.cfg flags */
	/* hm for now we also allow ring of phasing to be traded between winners. not needed though. */
	if (true_artifact_p(o_ptr) && !is_admin(p_ptr) &&
	    ((cfg.anti_arts_hoard && undepositable_artifact_p(o_ptr)) || (p_ptr->total_winner && !winner_artifact_p(o_ptr) && cfg.kings_etiquette)))
	    //(cfg.anti_arts_hoard || (cfg.anti_arts_house && 0)) would be cleaner sometime in the future..
	{
		char	o_name[ONAME_LEN];
		object_desc(Ind, o_name, o_ptr, TRUE, 0);

		msg_format(Ind, "%s fades into the air!", o_name);
		handle_art_d(o_ptr->name1);
		return -1;
	}
	else return (drop_near(Ind, o_ptr, chance, wpos, y, x));
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
	if (!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[y][x];

	/* Paranoia */
	if (!(cs_ptr = GetCS(c_ptr, CS_TRAPS))) return;

	cs_ptr->sc.trap.found = TRUE;

	/* Notice */
	note_spot_depth(wpos, y, x);

	/* Redraw */
	everyone_lite_spot(wpos, y, x);
}

void discharge_rod(object_type *o_ptr, int c) {
#ifndef NEW_MDEV_STACKING
	o_ptr->pval += c;
#else
	o_ptr->pval += c * o_ptr->number;
	o_ptr->bpval = o_ptr->number; //discharge whole stack
#endif

	//limit against rod-specific max:
	 //todo: make a function from the cmd6.c ..zap_rod.. code that returns the default recharge time of a rod or sth..

}

/*
 * Divide 'stacked' wands.	- Jir -
 * o_ptr->number is not changed here!
 * Note: onew_ptr already has the correct number (amt), o_ptr has still the full number (amt not yet subtracted).
 *       Our job is not to set the numbers, but just to handle charges et al.
 */
void divide_charged_item(object_type *onew_ptr, object_type *o_ptr, int amt) {
	/* Paranoia */
	if (o_ptr->number < amt) return;

	if (o_ptr->tval == TV_WAND
#ifdef NEW_MDEV_STACKING
	    || o_ptr->tval == TV_STAFF
#endif
	    ) {
		int charge = (o_ptr->pval * amt) / o_ptr->number;
		if (amt < o_ptr->number) o_ptr->pval -= charge;
		if (onew_ptr) onew_ptr->pval = charge;
	}
#ifdef NEW_MDEV_STACKING
	else if (o_ptr->tval == TV_ROD) {
		//only drop [some] charging rods? (priority)
		if (amt <= o_ptr->bpval) {
			//some or all charging rods
			if (onew_ptr) {
 #if 0 /* looks less weird, since "uncharge" of separated rods cannot grow higher than "uncharge" of original stack */
				onew_ptr->pval = (o_ptr->pval * amt + o_ptr->number - 1) / o_ptr->number;
 #else /* allow clean separation of fresh rods? [recommended]*/
				onew_ptr->pval = (o_ptr->pval * amt + o_ptr->bpval - 1) / o_ptr->bpval;
 #endif
				onew_ptr->bpval = amt;
			}
			//our old ones retain the "uncharge" and the used counter, since we only removed unused (fresh) ones
 #if 0 /* looks less weird, since "uncharge" of separated rods cannot grow higher than "uncharge" of original stack */
			o_ptr->pval = (o_ptr->pval * (o_ptr->number - amt)) / o_ptr->number;
 #else /* allow clean separation of fresh rods? [recommended]*/
			o_ptr->pval = (o_ptr->pval * (o_ptr->bpval - amt)) / o_ptr->bpval;
 #endif
			o_ptr->bpval -= amt;
		}
		//drop charging + fresh rods?
		else if (o_ptr->bpval) {
			//dump all "uncharge" and the whole used-counter into the dropped rods
			if (onew_ptr) {
				onew_ptr->pval = o_ptr->pval;
				onew_ptr->bpval = o_ptr->bpval;
			}
			//the left-over rods are now clean *sparkle*
			o_ptr->pval = 0;
			o_ptr->bpval = 0;
		}
		//only drop fresh rods?
		else { //(note: o_ptr->bpval == 0 implies o_ptr->pval == 0)
			//fresh rods
			if (onew_ptr) {
				onew_ptr->pval = 0;
				onew_ptr->bpval = 0;
			}
			//our old ones retain the "uncharge" and the used counter, since we only removed unused (fresh) ones
		}
	}
#endif
}

/*
 * Describe the charges on an item in the inventory.
 */
void inven_item_charges(int Ind, int item) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr = &p_ptr->inventory[item];

	/* Require staff/wand */
	if ((o_ptr->tval != TV_STAFF) && (o_ptr->tval != TV_WAND)) return;

	/* Require known item */
	if (!object_known_p(Ind, o_ptr)) return;

	/* Multiple charges */
	if (o_ptr->pval != 1) {
		/* Print a message */
		msg_format(Ind, "You have %d charges remaining.", o_ptr->pval);
	}
	/* Single charge */
	else {
		/* Print a message */
		msg_format(Ind, "You have %d charge remaining.", o_ptr->pval);
	}
}


/*
 * Describe an item in the inventory.
 */
void inven_item_describe(int Ind, int item) {
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
void inven_item_increase(int Ind, int item, int num) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr = &p_ptr->inventory[item];

	/* Lost all 'item_newest'? */
	if (-num >= o_ptr->number && item == p_ptr->item_newest) Send_item_newest(Ind, -1);

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
		/* Recalculate torch */
		p_ptr->update |= (PU_TORCH);
		/* Recalculate mana */
		p_ptr->update |= (PU_MANA | PU_HP | PU_SANITY);
		/* Redraw */
		p_ptr->redraw |= (PR_PLUSSES | PR_ARMOR);

		/* Combine the pack */
		p_ptr->notice |= (PN_COMBINE);
		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
	}

	/* If losing quest items, the quest goal might get unset again! */
	if ((p_ptr->quest_any_r || p_ptr->quest_any_r_target) && num < 0) quest_check_ungoal_r(Ind, o_ptr, -num);
}


/*
 * Erase an inventory slot if it has no more items
 * WARNING: Since this slides down following items, DON'T use this in a loop that
 * processes items and goes from lower value upwards to higher value if you don't
 * intend that result for some reason!! - C. Blue
 */
bool inven_item_optimize(int Ind, int item) {
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
	if (item < INVEN_WIELD) {
		int i;

		/* One less item */
		p_ptr->inven_cnt--;

		/* Slide everything down */
		for (i = item; i < INVEN_PACK; i++) {
			/* Structure copy */
			p_ptr->inventory[i] = p_ptr->inventory[i+1];

			if (i == p_ptr->item_newest) Send_item_newest(Ind, i - 1);
		}

		/* Update inventory indeces - mikaelh */
		inven_index_erase(Ind, item);
		inven_index_slide(Ind, item + 1, -1, INVEN_PACK);

		/* Erase the "final" slot */
		invwipe(&p_ptr->inventory[i]);
	}

	/* The item is being wielded */
	else {
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
void floor_item_charges(int item) {
	object_type *o_ptr = &o_list[item];

	/* Require staff/wand */
	if ((o_ptr->tval != TV_STAFF) && (o_ptr->tval != TV_WAND)) return;

	/* Require known item */
	/*if (!object_known_p(o_ptr)) return;*/

	/* Multiple charges */
	if (o_ptr->pval != 1) {
		/* Print a message */
		/*msg_format("There are %d charges remaining.", o_ptr->pval);*/
	}
	/* Single charge */
	else {
		/* Print a message */
		/*msg_format("There is %d charge remaining.", o_ptr->pval);*/
	}
}



/*
 * Describe an item in the inventory.
 */
void floor_item_describe(int item) {
	/* Get a description */
	/*object_desc(o_name, o_ptr, TRUE, 3);*/

	/* Print a message */
	/*msg_format("You see %s.", o_name);*/
}


/*
 * Increase the "number" of an item on the floor
 */
void floor_item_increase(int item, int num) {
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
void floor_item_optimize(int item) {
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
void auto_inscribe(int Ind, object_type *o_ptr, int flags) {
	player_type *p_ptr = Players[Ind];
#if 0
	char c[] = "@m ";
#endif

	if (!o_ptr->tval) return;

	/* skip inscribed items */
	if (!flags && o_ptr->note &&
	    strcmp(quark_str(o_ptr->note), "on sale") &&
	    strcmp(quark_str(o_ptr->note), "stolen"))
		return;

	if (!p_ptr->obj_aware[o_ptr->k_idx]) return;

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
			o_ptr->note = quark_add("@r5");//@r5!X
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
bool inven_carry_okay(int Ind, object_type *o_ptr, byte tolerance) {
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
		if (object_similar(Ind, j_ptr, o_ptr, tolerance)) return (TRUE);
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
s16b inven_carry(int Ind, object_type *o_ptr) {
	player_type *p_ptr = Players[Ind];

	int         i, j, k;
	int		n = -1;

	object_type	*j_ptr;
	u32b f1 = 0, f2 = 0, f3 = 0, f4 = 0, f5, f6 = 0, esp = 0;

	/* Check for combining */
	for (j = 0; j < INVEN_PACK; j++) {
		j_ptr = &p_ptr->inventory[j];

		/* Skip empty items */
		if (!j_ptr->k_idx) continue;

		/* Hack -- track last item */
		n = j;

		/* Check if the two items can be combined */
		if (object_similar(Ind, j_ptr, o_ptr, 0x0)) {
			/* Check whether this item was requested by an item-retrieval quest.
			   Note about quest_credited check: inven_carry() is also called by carry(),
			   resulting in double crediting otherwise! */
			if (p_ptr->quest_any_r_within_target && !o_ptr->quest_credited) quest_check_goal_r(Ind, o_ptr);

			/* Combine the items */
			object_absorb(Ind, j_ptr, o_ptr);

			/* Increase the weight */
			p_ptr->total_weight += (o_ptr->number * o_ptr->weight);

			/* Recalculate bonuses */
			p_ptr->update |= (PU_BONUS);

			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

			/* Success */
			p_ptr->inventory[j].auto_insc = TRUE;
#ifdef USE_SOUND_2010
			sound_item(Ind, o_ptr->tval, o_ptr->sval, "pickup_");
#endif
			Send_item_newest(Ind, j);
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

	/* Check whether this item was requested by an item-retrieval quest
	   Note about quest_credited check: inven_carry() is also called by carry(),
	   resulting in double crediting otherwise! */
	if (p_ptr->quest_any_r_within_target && !o_ptr->quest_credited) quest_check_goal_r(Ind, o_ptr);

	if (!o_ptr->owner && !p_ptr->admin_dm) {
		o_ptr->owner = p_ptr->id;
		o_ptr->mode = p_ptr->mode;
		if (true_artifact_p(o_ptr)) determine_artifact_timeout(o_ptr->name1, &o_ptr->wpos); /* paranoia? */
	}

	/* Auto id ? */
	if (p_ptr->auto_id) {
		object_aware(Ind, o_ptr);
		object_known(o_ptr);
	}

	/* Auto-inscriber */
#ifdef	AUTO_INSCRIBER
	if (p_ptr->auto_inscribe) auto_inscribe(Ind, o_ptr, 0);
#endif

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	/* Auto Curse */
	if (f3 & TR3_AUTO_CURSE) {
		/* The object recurse itself ! */
		if (!(o_ptr->ident & ID_CURSED)) {
			o_ptr->ident |= ID_CURSED;
			o_ptr->ident |= ID_SENSE | ID_SENSED_ONCE;
			note_toggle_cursed(o_ptr, TRUE);
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
	p_ptr->inventory[i].auto_insc = TRUE;

#ifdef USE_SOUND_2010
	sound_item(Ind, o_ptr->tval, o_ptr->sval, "pickup_");
#endif
	Send_item_newest(Ind, i);
	return (i);
}

/* Helper function for character birth: Equip starter items automatically. */
void inven_carry_equip(int Ind, object_type *o_ptr) {
#if 1
	int item = inven_carry(Ind, o_ptr);

	if (wearable_p(o_ptr)
	    /* default starter ranged weapon is a bow, so equip magic arrow together with it */
	    && (!is_ammo(o_ptr->tval) || o_ptr->tval == TV_ARROW)) {
		suppress_message = TRUE;
		do_cmd_wield(Ind, item, 0x0);
		suppress_message = FALSE;
	}
#else
	inven_carry(Ind, o_ptr);
#endif
}



/*
 * Combine items in the pack
 *
 * Note special handling of the "overflow" slot
 */
void combine_pack(int Ind) {
	player_type *p_ptr = Players[Ind];

	int		i, j, k;

	object_type	*o_ptr;
	object_type	*j_ptr;

	bool	flag = FALSE;


	/* Combine the pack (backwards) */
	for (i = INVEN_PACK; i > 0; i--) {
		/* Get the item */
		o_ptr = &p_ptr->inventory[i];

		/* Skip empty items */
		if (!o_ptr->k_idx) continue;


		/* Auto id ? */
		if (p_ptr->auto_id) {
			object_aware(Ind, o_ptr);
			object_known(o_ptr);

			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP);
		}

		/* Scan the items above that item */
		for (j = 0; j < i; j++) {
			/* Get the item */
			j_ptr = &p_ptr->inventory[j];

			/* Skip empty items */
			if (!j_ptr->k_idx) continue;

			/* Can we drop "o_ptr" onto "j_ptr"? */
			if (object_similar(Ind, j_ptr, o_ptr, p_ptr->current_force_stack - 1 == i ? 0x2 : 0x0)) {
				/* clear if used */
				if (p_ptr->current_force_stack - 1 == i) p_ptr->current_force_stack = 0;

				/* Take note */
				flag = TRUE;

				/* Add together the item counts */
				object_absorb(Ind, j_ptr, o_ptr);

				/* One object is gone */
				p_ptr->inven_cnt--;

				/* Slide everything down */
				for (k = i; k < INVEN_PACK; k++) {
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

				if (p_ptr->inventory[j].auto_insc) {
					p_ptr->inventory[i].auto_insc = TRUE;
					p_ptr->inventory[j].auto_insc = FALSE;
				}

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
void reorder_pack(int Ind) {
	player_type *p_ptr = Players[Ind];

	int		i, j, k;

	s64b	o_value;
	s64b	j_value;

	object_type *o_ptr;
	object_type *j_ptr;

	object_type	temp;

	bool	flag = FALSE;


	/* Re-order the pack (forwards) */
	for (i = 0; i < INVEN_PACK; i++) {
		/* Mega-Hack -- allow "proper" over-flow */
		if ((i == INVEN_PACK) && (p_ptr->inven_cnt == INVEN_PACK)) break;

		/* Get the item */
		o_ptr = &p_ptr->inventory[i];

		/* Skip empty slots */
		if (!o_ptr->k_idx) continue;

		/* Get the "value" of the item */
		o_value = object_value(Ind, o_ptr);

		/* Scan every occupied slot */
		for (j = 0; j < INVEN_PACK; j++) {
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
		for (k = i; k > j; k--) {
			/* Slide the item */
			p_ptr->inventory[k] = p_ptr->inventory[k-1];
		}

		/* Insert the moved item */
		p_ptr->inventory[j] = temp;

		if (p_ptr->item_newest == i) Send_item_newest(Ind, j);

		/* Update inventory indeces - mikaelh */
		inven_index_slide(Ind, j, 1, i - 1);
		inven_index_move(Ind, i, j);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		if (p_ptr->inventory[i].auto_insc) {
			p_ptr->inventory[j].auto_insc = TRUE;
			p_ptr->inventory[i].auto_insc = FALSE;
		}
	}

	/* Message */
	if (flag) msg_print(Ind, "You reorder some items in your pack.");
}




/*
 * Hack -- process the objects (called every turn)
 */
/* TODO: terrain effects (lava burns scrolls etc) -- those are done elsewhere */
void process_objects(void) {
	int i, k, Ind;
	object_type *o_ptr;
	house_type *h_ptr;

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

		/* timing fix - see description in dungeon() */
		if (turn % (level_speed(&o_ptr->wpos) / 120)) continue;

		/* Handle Timeouts */
		if (o_ptr->timeout) {
			switch (o_ptr->tval) {
			case TV_LITE:
				//don't decrement, they don't time out in player inventory either
				break;
			case TV_POTION: case TV_FOOD: //SV_POTION_BLOOD going bad
				o_ptr->timeout--;
				/* poof */
				if (!(o_ptr->timeout)) delete_object_idx(i, TRUE);
				continue;
			}
		}
		if (o_ptr->timeout_magic) { //polymorph ring only atm
			switch (o_ptr->tval) {
			case TV_RING:
				//don't decrement, they don't time out in player inventory either
				break;
			}
		}
		/* SV_SNOWBALL melts */
		if (o_ptr->tval == TV_GAME && o_ptr->pval) {
			if (cold_place(&o_ptr->wpos)) continue;

			o_ptr->pval--;
			/* poof */
			if (!(o_ptr->pval)) delete_object_idx(i, TRUE);
			continue;
		}
		/* Recharge activatable items on the ground */
		if (o_ptr->recharging) o_ptr->recharging--;
		/* Recharge rods on the ground and inside trap kits */
		if ((o_ptr->tval == TV_ROD) && (o_ptr->pval)) {
#ifndef NEW_MDEV_STACKING
			o_ptr->pval--;
#else
			o_ptr->pval -= o_ptr->number;
			if (o_ptr->pval < 0) o_ptr->pval = 0; //can happen by rod-stack-splitting (divide_charged_item())
			/* Reset it from 'charging' state to charged state */
			if (!o_ptr->pval) o_ptr->bpval = 0;
#endif
		}
	}

#if 1 /* experimental: also process items in list houses */

	/* timing fix - see description in dungeon():
	   Since all we do here for now is handling recharging/timeouting,
	   we may just as well return if it's not yet time to. */
	if (turn % ((level_speeds[0] * 5) / 120)) return; //standard world surface speed)

	/* process items in list houses */
	for (k = 0; k < num_houses; k++) {
		h_ptr = &houses[k];
		if (!(houses[k].flags & HF_TRAD)) continue; /* skip non-list houses */

		for (i = 0; i < h_ptr->stock_num; i++) {
			o_ptr = &h_ptr->stock[i];

			/* Handle Timeouts */
			if (o_ptr->timeout) {
				switch (o_ptr->tval) {
				case TV_LITE:
					//don't decrement, they don't time out in player inventory either
					break;
				case TV_POTION: case TV_FOOD: //basically just SV_POTION_BLOOD
					Ind = pick_player(h_ptr);
					o_ptr->timeout--;
					/* poof */
					if (!(o_ptr->timeout)) {
						home_item_increase(h_ptr, i, -o_ptr->number);
						home_item_optimize(h_ptr, i);
						if (Ind) display_trad_house(Ind, h_ptr); //display_house_inventory(Ind, h_ptr);
						continue;
					}
#ifdef LIVE_TIMEOUTS
					else if (Ind && Players[Ind]->live_timeouts) display_house_entry(Ind, i, h_ptr);
#endif
				}
			}
			if (o_ptr->timeout_magic) { //Polymorph rings only, atm
				switch (o_ptr->tval) {
				case TV_RING:
					//don't decrement, they don't time out in player inventory either
					break;
				}
			}
			/* SV_SNOWBALL melts */
			if (o_ptr->tval == TV_GAME && o_ptr->pval) {
				if (cold_place(&o_ptr->wpos)) continue;

				Ind = pick_player(h_ptr);
				o_ptr->pval--;
				/* poof */
				if (!(o_ptr->pval)) {
					home_item_increase(h_ptr, i, -o_ptr->number);
					home_item_optimize(h_ptr, i);
					if (Ind) display_trad_house(Ind, h_ptr); //display_house_inventory(Ind, h_ptr);
					continue;
				}
#ifdef LIVE_TIMEOUTS
				else if (Ind && Players[Ind]->live_timeouts) display_house_entry(Ind, i, h_ptr);
#endif
				continue;
			}
			/* Recharge activatable items in list house */
			if (o_ptr->recharging) o_ptr->recharging--;
			/* Recharge rods in the list house */
			if ((o_ptr->tval == TV_ROD) && (o_ptr->pval)) {
#ifndef NEW_MDEV_STACKING
				o_ptr->pval--;
#else
				o_ptr->pval -= o_ptr->number;
				if (o_ptr->pval < 0) o_ptr->pval = 0; //can happen by rod-stack-splitting (divide_charged_item())
				/* Reset it from 'charging' state to charged state */
				if (!o_ptr->pval) o_ptr->bpval = 0;
#endif
			}
		}
	}
#endif
}

/*
 * Set the "o_idx" fields in the cave array to correspond
 * to the objects in the "o_list".
 */
void setup_objects(void) {
	int i;
	cave_type **zcave;
	object_type *o_ptr;

	for (i = 0; i < o_max; i++) {
		o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip objects on depths that aren't allocated */
		if (!(zcave = getcave(&o_ptr->wpos))) continue;

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
void object_wipe(object_type *o_ptr) {
	/* Wipe the structure */
	o_ptr = WIPE(o_ptr, object_type);
}


/*
 * Prepare an object based on an existing object: dest, src
 */
void object_copy(object_type *o_ptr, object_type *j_ptr) {
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

/* Easier unified artifact handling.
   Note: ONLY call these if SURE that aidx isn't ART_RANDART but a true_artifact_p()! */
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
#ifdef FLUENT_ARTIFACT_RESETS
	if (a_info[aidx].cur_num == 0) {
s_printf("A_TIMEOUT: handle_art_dnum (%d)\n", aidx);
		a_info[aidx].timeout = 0;
	}
#endif
}
void handle_art_d(int aidx) {
	if (a_info[aidx].cur_num > 0) {
		a_info[aidx].cur_num--;
		if (!a_info[aidx].cur_num) {
			a_info[aidx].known = FALSE;
#ifdef FLUENT_ARTIFACT_RESETS
s_printf("A_TIMEOUT: handle_art_d 1 (%d)\n", aidx);
			a_info[aidx].timeout = 0;
#endif
		}
	} else {
		a_info[aidx].cur_num = 0;
		a_info[aidx].known = FALSE;//semi-paranoia: fixes old arts from before fluent reset mechanism!
#ifdef FLUENT_ARTIFACT_RESETS
s_printf("A_TIMEOUT: handle_art_d 2 (%d)\n", aidx);
		a_info[aidx].timeout = 0;
#endif
	}
}

/* Check whether an item causes HP drain on an undead player (vampire) who wears/wields it */
bool anti_undead(object_type *o_ptr) {
	u32b f1, f2, f3, f4, f5, f6, esp;
	int l = 0;

	/* only concerns wearable items */
	if (wield_slot(0, o_ptr) == -1) return FALSE;

	if (cursed_p(o_ptr)) return(FALSE);

	/* hack: it's carried by the wight-king! */
	if (o_ptr->name1 == ART_STONE_LORE) return FALSE;

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
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
#ifdef ENABLE_HELLKNIGHT
/* Check whether an item causes HP drain on a demonic player (hell knight) who wears/wields it */
bool anti_demon(object_type *o_ptr) {
	u32b f1, f2, f3, f4, f5, f6, esp;
	int l = 0;

	/* only concerns wearable items */
	if (wield_slot(0, o_ptr) == -1) return FALSE;

	if (cursed_p(o_ptr)) return(FALSE);

	/* hack: it's carried by the wight-king! */
	if (o_ptr->name1 == ART_STONE_LORE) return FALSE;

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
	if (f3 & TR3_LITE1) l++;
	if (f4 & TR4_LITE2) l += 2;
	if (f4 & TR4_LITE3) l += 3;
	if ((f4 & TR4_FUEL_LITE) && (o_ptr->timeout < 1)) l = 0;

	/* powerful lights and anti-undead/evil items damage vampires */
	if (l) { /* light sources, or other items that provide light */
		if ((l > 2) || o_ptr->name1 || (f3 & TR3_BLESSED) ||
		    (f1 & TR1_SLAY_EVIL) || (f1 & TR1_SLAY_DEMON) || (f1 & TR1_KILL_DEMON))
			return(TRUE);
	} else {
		if ((f3 & TR3_BLESSED) || (f1 & TR1_KILL_DEMON))
			return(TRUE);
	}

	return(FALSE);
}
#endif

/*
 * Generate default item-generation restriction flags for a given player - C. Blue
 */
u32b make_resf(player_type *p_ptr) {
	u32b f = RESF_NONE;
	if (p_ptr == NULL) return(f);

	/* winner handling */
	if (p_ptr->total_winner) {
		f |= RESF_WINNER; /* allow generation of WINNERS_ONLY items */
		f |= RESF_LIFE; /* allowed to find +LIFE artifacts; hack: and WINNERS_ONLY artifacts too */
		if (cfg.kings_etiquette) f |= RESF_NOTRUEART; /* player is currently a winner? Then don't find true arts! */
	}

	/* fallen winner handling */
	if (p_ptr->once_winner) {
		if (cfg.fallenkings_etiquette) {
			f |= RESF_NOTRUEART; /* player is a fallen winner? Then don't find true arts! */
			/* since fallen kings are already kinda punished by their status, cut them some slack here?: */
			if (p_ptr->lev >= 50) {
//				f |= RESF_WINNER; /* allow generation of WINNERS_ONLY items, if player won once but can't get true arts */
//				f |= RESF_LIFE; /* allowed to find +LIFE artifacts; hack: and WINNERS_ONLY artifacts too */
			}
		}
	}

	/* anyone who cannot use true arts can still _find_ them? (should usually be disabled) */
	if (!cfg.winners_find_randarts) f &= ~RESF_NOTRUEART; /* monsters killed by [fallen] winners can drop no true arts but randarts only instead? */

	/* special mode handling */
	if (p_ptr->mode & MODE_PVP) f |= RESF_NOTRUEART; /* PvP mode chars can't find true arts, since true arts are for kinging! */

	/* don't generate The One Ring if player killed Sauron */
	if (p_ptr->r_killed[RI_SAURON] == 1) f |= RESF_SAURON;

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

/* Set timeout for a newly found artifact, for fluent artifact reset system
   to counter long-time hoarding of artifacts. - C. Blue */
void determine_artifact_timeout(int a_idx, struct worldpos *wpos) {
#ifndef FLUENT_ARTIFACT_RESETS
	a_info[a_idx].timeout = -2; /* marker for when it gets reactivated */
#else
	object_type forge;
	int i;

//debug
s_printf("A_TIMEOUT: Called (%d)!\n", a_idx);

	i = lookup_kind(a_info[a_idx].tval, a_info[a_idx].sval);
        if (i) invcopy(&forge, i);
        else { /* paranoia */
		s_printf("DETERMINE_ARTIFACT_TIMEOUT: Cannot find item %d,%d (aidx %d)!\n", a_info[a_idx].tval, a_info[a_idx].sval, a_idx);
		/* try to hack it manually, really paranoid */
		forge.k_idx = 0;
		a_idx = 0; //artifact #0 has tval,sval = 0,0 - for the paranoid code below.. (side note: true_artifact_p() doesn't return true for aidx 0 ^^ but who cares..)
		/* atm this code has no actual effect.. */
		forge.tval = a_info[a_idx].tval;
		forge.sval = a_info[a_idx].sval;
	}
	forge.name1 = a_idx;

 #ifdef RING_OF_PHASING_NO_TIMEOUT
	if (a_idx == ART_PHASING) {
		/* special treatment: it's pseudo-permanent, but gets erased when someone else kills Zu-Aon */
		a_info[a_idx].timeout = -1;
	} else
 #endif
	if (multiple_artifact_p(&forge)) {
		a_info[a_idx].timeout = -1; /* grond/crown don't expire */
		return;
	} else if (winner_artifact_p(&forge)) a_info[a_idx].timeout = FLUENT_ARTIFACT_WEEKS * 10080 * 2; /* mirror of glory */
	else if (a_idx != ART_RANDART) a_info[a_idx].timeout = FLUENT_ARTIFACT_WEEKS * 10080;
	else {
		/* paranoia */
		s_printf("DETERMINE_ARTIFACT_TIMEOUT: For some reason a randart was specified!\n");
		return;
	}

 #ifdef RPG_SERVER
	a_info[a_idx].timeout *= 2;
 #endif
#endif
	//for IDDC_ARTIFACT_FAST_TIMEOUT
	if (wpos) a_info[a_idx].iddc = in_irondeepdive(wpos);

	/* assume winner-artifact or non-winner, for WINNER_ARTIFACT_FAST_TIMEOUT */
	a_info[a_idx].winner = FALSE;
}

/* Similarly to erase_guild_key() this function searches *everywhere* for a
   true artifact to erase it. Used for FLUENT_ARTIFACT_RESETS. - C. Blue */
void erase_artifact(int a_idx) {
	int i, this_o_idx, next_o_idx;
	monster_type *m_ptr;
	object_type *o_ptr, *q_ptr;
	char m_name[MNAME_LEN], o_name[ONAME_LEN], o_name_short[ONAME_LEN];

	int slot;
	hash_entry *ptr;
	player_type *p_ptr;

	object_type forge;
	i = lookup_kind(a_info[a_idx].tval, a_info[a_idx].sval);
	if (i) invcopy(&forge, i);
	forge.name1 = a_idx;
	object_desc(0, o_name, &forge, TRUE, 0);//fixed diz for admins
	object_desc(0, o_name_short, &forge, TRUE, 256);//short name for telling people

	/* objects on the floor/in monster inventories */
	for (i = 0; i < o_max; i++) {
		o_ptr = &o_list[i];
		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;
		/* Look for specific true artifact */
		if (o_ptr->name1 != a_idx) continue;

		/* in monster inventory */
		if (o_ptr->held_m_idx) {
			m_ptr = &m_list[o_ptr->held_m_idx];
			/* 1st object held is the artifact? */
			if (m_ptr->hold_o_idx == i) {
				m_ptr->hold_o_idx = o_ptr->next_o_idx;
				monster_desc(0, m_name, o_ptr->held_m_idx, 0);
				s_printf("FLUENT_ARTIFACT_RESETS: %d - monster inventory (%d, '%s', #1)\n  '%s'\n", a_idx, o_ptr->held_m_idx, m_name, o_name);
				delete_object_idx(i, TRUE);
				msg_broadcast_format(0, "\374\377M* \377U%s has been lost once more. \377M*", o_name_short);
				return;
			} else {
				i = 1;
				q_ptr = &o_list[m_ptr->hold_o_idx];//compiler warning
				for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx) {
					if (this_o_idx == i) {
						q_ptr->next_o_idx = o_list[this_o_idx].next_o_idx;
						monster_desc(0, m_name, o_ptr->held_m_idx, 0);
						s_printf("FLUENT_ARTIFACT_RESETS: %d - monster inventory (%d, '%s', #%d)\n  '%s'\n", a_idx, o_ptr->held_m_idx, m_name, i, o_name);
						delete_object_idx(this_o_idx, TRUE);
						msg_broadcast_format(0, "\374\377M* \377U%s has been lost once more. \377M*", o_name_short);
						return;
					}
					q_ptr = &o_list[this_o_idx];
					next_o_idx = q_ptr->next_o_idx;
					i++;
				}
				/* paranoid fail */
				s_printf("FLUENT_ARTIFACT_RESETS_ERROR: %d - monster inventory (%d, '%s', #%d)\n  '%s'\n", a_idx, o_ptr->held_m_idx, m_name, i, o_name);
				return;
			}
		}

#ifdef PLAYER_STORES
		/* Log removal of player store items - this code only applies if server rules allow dropping true artifacts in houses.
		   In case the cave wasn't allocated, the delete_object_idx() call below won't remove it from pstore lists, so we have to do it now.
		   Note: This can be a false alarm in case the item is inscribed '@S' but is not actually inside a player house. */
		if (!getcave(&o_ptr->wpos) &&
		    o_ptr->note && strstr(quark_str(o_ptr->note), "@S")) {
			//char o_name[ONAME_LEN];//, p_name[NAME_LEN];
			//object_desc(0, o_name, o_ptr, TRUE, 3);
			//s_printf("PLAYER_STORE_REMOVED (maybe): %s - %s (%d,%d,%d; %d,%d).\n",
			s_printf("PLAYER_STORE_REMOVED (maybe): %s (%d,%d,%d; %d,%d).\n",
			    //p_name, o_name, wpos->wx, wpos->wy, wpos->wz,
			    o_name, o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz,
			    o_ptr->ix, o_ptr->iy);
		}
#endif

		s_printf("FLUENT_ARTIFACT_RESETS: %d - floor '%s'\n", a_idx, o_name);
		delete_object_idx(i, TRUE);
		msg_broadcast_format(0, "\374\377M* \377U%s has been lost once more. \377M*", o_name_short);
		return;
	}

	/* Players online */
	for (this_o_idx = 1; this_o_idx <= NumPlayers; this_o_idx++) {
		p_ptr = Players[this_o_idx];
		/* scan his inventory */
		for (i = 0; i < INVEN_TOTAL; i++) {
			o_ptr = &p_ptr->inventory[i];
			if (!o_ptr->k_idx) continue;

			if (o_ptr->name1 == a_idx) {
				s_printf("FLUENT_ARTIFACT_RESETS: %d - player '%s'\n  '%s'\n", a_idx, p_ptr->name, o_name);
				//object_desc(this_o_idx, o_name, o_ptr, FALSE, 3);
				msg_format(this_o_idx, "\374\377R%s bids farewell to you...", o_name_short);
				handle_art_d(a_idx);
				inven_item_increase(this_o_idx, i, -99);
				inven_item_describe(this_o_idx, i);
				inven_item_optimize(this_o_idx, i);
				msg_broadcast_format(this_o_idx, "\374\377M* \377U%s has been lost once more. \377M*", o_name_short);
				return;
			}
		}
	}

	/* hack */
	NumPlayers++;
	MAKE(Players[NumPlayers], player_type);
	p_ptr = Players[NumPlayers];
	p_ptr->inventory = C_NEW(INVEN_TOTAL, object_type);
	for (slot = 0; slot < NUM_HASH_ENTRIES; slot++) {
		ptr = hash_table[slot];
		while (ptr) {
			/* not the holder of the artifact? */
			if (ptr->id != a_info[a_idx].carrier) {
				/* advance to next character */
				ptr = ptr->next;
				continue;
			}

			/* clear his data (especially inventory) */
			o_ptr = p_ptr->inventory;
			WIPE(p_ptr, player_type);
			p_ptr->inventory = o_ptr;
			p_ptr->Ind = NumPlayers;
			C_WIPE(p_ptr->inventory, INVEN_TOTAL, object_type);
			/* set his supposed name */
			strcpy(p_ptr->name, ptr->name);
			/* generate savefile name */
			process_player_name(NumPlayers, TRUE);
			/* try to load him! */
			if (!load_player(NumPlayers)) {
				/* bad fail */
				s_printf("FLUENT_ARTIFACT_RESETS_ERROR: %d - load_player '%s' failed\n  '%s'\n", a_idx, p_ptr->name, o_name);
				/* unhack */
				C_FREE(p_ptr->inventory, INVEN_TOTAL, object_type);
				KILL(p_ptr, player_type);
				NumPlayers--;
				return;
			}
			/* scan his inventory */
			for (i = 0; i < INVEN_TOTAL; i++) {
				o_ptr = &p_ptr->inventory[i];
				if (!o_ptr->k_idx) continue;

				if (o_ptr->name1 == a_idx) {
					s_printf("FLUENT_ARTIFACT_RESETS: %d - savegame '%s'\n  '%s'\n", a_idx, p_ptr->name, o_name);
					handle_art_d(a_idx);
					o_ptr->tval = o_ptr->sval = o_ptr->k_idx = o_ptr->name1 = 0;
					p_ptr->fluent_artifact_reset = TRUE; /* hack to notify him next time he logs on */
					/* write savegame back */
					save_player(NumPlayers);
					/* unhack */
					C_FREE(p_ptr->inventory, INVEN_TOTAL, object_type);
					KILL(p_ptr, player_type);
					NumPlayers--;
					msg_broadcast_format(0, "\374\377M* \377U%s has been lost once more. \377M*", o_name_short);
					return;
				}
			}

			/* exit with failure */
			slot = NUM_HASH_ENTRIES;
			break;
		}
	}
	/* unhack */
        C_FREE(p_ptr->inventory, INVEN_TOTAL, object_type);
        KILL(p_ptr, player_type);
	NumPlayers--;

	/* Paranoia: Failed to locate the artifact. Shouldn't happen! */
	s_printf("FLUENT_ARTIFACT_RESETS_ERROR: %d - not found '%s'\n", a_idx, o_name);

	/* It can actually happen if the savegame was deleted manually.
	   In such cases, free the artifact again. This might cause problems
	   with duplicate artifacts if the savegames are reinstantiated, so
	   on loading a character, the artifact owner should be compared. */
	handle_art_d(a_idx);
	msg_broadcast_format(0, "\374\377M* \377U%s has been lost once more. \377M*", o_name_short);
}

/* Modify a particular existing type of item in the whole game world. - C. Blue
   (added for Living Lightning drop 'Sky DSM of Imm' to become a canonical randart Sky DSM instead) */
/* Helper function - hard-coded stuff - replacement item parameters */
static void hack_particular_item_aux(object_type *qq_ptr, struct worldpos xwpos) {
	int tries;
	artifact_type *xa_ptr;

	object_wipe(qq_ptr);
	invcopy(qq_ptr, lookup_kind(TV_DRAG_ARMOR, SV_DRAGON_SKY));
	qq_ptr->number = 1;
	qq_ptr->name1 = ART_RANDART;
	tries = 500;
	while (tries) {
		/* Piece together a 32-bit random seed */
		qq_ptr->name3 = (u32b)rand_int(0xFFFF) << 16;
		qq_ptr->name3 += rand_int(0xFFFF);
		apply_magic(&xwpos, qq_ptr, 150, TRUE, TRUE, TRUE, TRUE, RESF_FORCERANDART | RESF_NOTRUEART | RESF_LIFE);

		xa_ptr = randart_make(qq_ptr);
		if (artifact_power(xa_ptr) >= 105 + 5 && /* at least +1 new mod gained; and +extra bonus boost */
		    qq_ptr->to_a > 0 && /* not cursed */
		    !(xa_ptr->flags3 & (TR3_AGGRAVATE | TR3_NO_MAGIC)))
			break;
		tries--;
	}
	qq_ptr->level = 0;
	if (!tries) s_printf("hack_particular_item_aux: Re-rolling out of tries!\n");
}
static void hack_particular_item_prepare_wpos(struct worldpos *xwpos) {
	int x, y;
	dungeon_type *d_ptr;

	for (y = 0; y < MAX_WILD_Y; y++)
	for (x = 0; x < MAX_WILD_X; x++) {
		if ((d_ptr = wild_info[y][x].tower)) {
			if (d_ptr->type == DI_CLOUD_PLANES) {
				xwpos->wx = x;
				xwpos->wy = y;
				xwpos->wz = 20;
			}
		}
		if ((d_ptr = wild_info[y][x].dungeon)) {
			if (d_ptr->type == DI_CLOUD_PLANES) {
				xwpos->wx = x;
				xwpos->wy = y;
				xwpos->wz = 20;
			}
		}
	}
}
static bool hack_particular_item_cmp(object_type *o_ptr) {
	//return (o_ptr->tval != TV_DRAG_ARMOR || o_ptr->sval != SV_DRAGON_SKY || o_ptr->name2 != EGO_IMMUNE);
	return (o_ptr->tval != TV_DRAG_ARMOR || o_ptr->sval != SV_DRAGON_SKY || o_ptr->name1 != ART_RANDART || o_ptr->level != 0);
}
void hack_particular_item(void) {
	int i, this_o_idx, next_o_idx;
	monster_type *m_ptr;
	object_type *o_ptr, *q_ptr;

	int slot;
	hash_entry *ptr;
	player_type *p_ptr;

	int found = 0;

	struct worldpos xwpos;

	xwpos.wx = xwpos.wy = xwpos.wz = 0;
	hack_particular_item_prepare_wpos(&xwpos);
	if (!xwpos.wz) {
		s_printf("hack_particular_item(): failed to prepare wpos!\n");
		return;
	}

	s_printf("hack_particular_item(): commencing check..\n");

	/* objects on the floor/in monster inventories */
	for (i = 0; i < o_max; i++) {
		o_ptr = &o_list[i];
		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;
		/* Look for specific item */
		if (hack_particular_item_cmp(o_ptr)) continue;

		/* in monster inventory */
		if (o_ptr->held_m_idx) {
			m_ptr = &m_list[o_ptr->held_m_idx];
			/* 1st object held matches? */
			q_ptr = &o_list[m_ptr->hold_o_idx];
			if (!hack_particular_item_cmp(q_ptr)) {
				s_printf(" found in monster inventory (1st)\n");
				hack_particular_item_aux(q_ptr, xwpos);
				found++;
			} else {
				i = 1;
				q_ptr = &o_list[m_ptr->hold_o_idx];//compiler warning
				for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx) {
					q_ptr = &o_list[this_o_idx];
					if (!hack_particular_item_cmp(q_ptr)) {
						s_printf(" found in monster inventory\n");
						hack_particular_item_aux(&o_list[this_o_idx], xwpos);
						found++;
					}
					next_o_idx = q_ptr->next_o_idx;
					i++;
				}
			}
		} else {
			q_ptr = &o_list[i];
			s_printf(" found on the floor\n");
			hack_particular_item_aux(q_ptr, xwpos);
			found++;
		}
	}

	/* Players online */
	for (this_o_idx = 1; this_o_idx <= NumPlayers; this_o_idx++) {
		p_ptr = Players[this_o_idx];
		/* scan his inventory */
		for (i = 0; i < INVEN_TOTAL; i++) {
			o_ptr = &p_ptr->inventory[i];
			if (!o_ptr->k_idx) continue;

			if (!hack_particular_item_cmp(o_ptr)) {
				s_printf(" found in live player '%s'\n", p_ptr->name);
				hack_particular_item_aux(o_ptr, xwpos);
				found++;
			}
		}
	}

	/* hack */
	NumPlayers++;
	MAKE(Players[NumPlayers], player_type);
	p_ptr = Players[NumPlayers];
	p_ptr->inventory = C_NEW(INVEN_TOTAL, object_type);
	for (slot = 0; slot < NUM_HASH_ENTRIES; slot++) {
		ptr = hash_table[slot];
		while (ptr) {
			/* clear his data (especially inventory) */
			o_ptr = p_ptr->inventory;
			WIPE(p_ptr, player_type);
			p_ptr->inventory = o_ptr;
			p_ptr->Ind = NumPlayers;
			C_WIPE(p_ptr->inventory, INVEN_TOTAL, object_type);
			/* set his supposed name */
			strcpy(p_ptr->name, ptr->name);
			/* generate savefile name */
			process_player_name(NumPlayers, TRUE);
			/* try to load him! */
			if (!load_player(NumPlayers)) {
				/* bad fail */
				s_printf(" load_player '%s' failed\n", p_ptr->name);
				/* unhack */
				C_FREE(p_ptr->inventory, INVEN_TOTAL, object_type);
				KILL(p_ptr, player_type);
				NumPlayers--;
				return;
			}
			/* scan his inventory */
			for (i = 0; i < INVEN_TOTAL; i++) {
				o_ptr = &p_ptr->inventory[i];
				if (!o_ptr->k_idx) continue;

				if (!hack_particular_item_cmp(o_ptr)) {
					s_printf(" found in load_player '%s'\n", p_ptr->name);
					hack_particular_item_aux(o_ptr, xwpos);
					found++;
					/* write savegame back */
					save_player(NumPlayers);
				}
			}
			ptr = ptr->next;
		}
	}
	/* unhack */
        C_FREE(p_ptr->inventory, INVEN_TOTAL, object_type);
        KILL(p_ptr, player_type);
	NumPlayers--;

	s_printf("hack_particular_item: found+replaced %d occurances\n", found);
}

#ifdef VAMPIRES_INV_CURSED
/* Reverse negative boni on a cursed item while equipped by a true undead (RACE_VAMPIRE) player,
   provided the item is eligible (HEAVY_CURSE). - C. Blue */
void inverse_cursed(object_type *o_ptr) {
	u32b f1, f2, f3, f4, f5, f6, esp;

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
	if (!(f3 & TR3_HEAVY_CURSE)) return;
	if ((f4 & TR4_NEVER_BLOW)) return; /* Weapons of Nothingness */

	/* reverse to-hit, but body armour reverts to its normal "bulkiness" to-hit penalty */
	if (o_ptr->tval == TV_DRAG_ARMOR || o_ptr->tval == TV_HARD_ARMOR || o_ptr->tval == TV_SOFT_ARMOR) {
		o_ptr->to_h_org = o_ptr->to_h;
		o_ptr->to_h = k_info[o_ptr->k_idx].to_h;
	}
	else if (o_ptr->to_h < 0) {
		o_ptr->to_h_org = o_ptr->to_h;
		o_ptr->to_h = -o_ptr->to_h;
		if (o_ptr->to_h > 30) o_ptr->to_h = 30;
	}
	/* reverse to-dam */
	if (o_ptr->to_d < 0) {
		o_ptr->to_d_org = o_ptr->to_d;
		o_ptr->to_d = -o_ptr->to_d;
		if (o_ptr->to_d > 30) o_ptr->to_d = 30;
	}
	/* gloves don't get exaggerating hit/dam boni */
	if (o_ptr->tval == TV_GLOVES) {
		if (o_ptr->to_h > 10) o_ptr->to_h = 10;
		if (o_ptr->to_d > 10) o_ptr->to_d = 10;
	}

	/* reverse AC */
	if (o_ptr->to_a < 0) {
		o_ptr->to_a_org = o_ptr->to_a;
		o_ptr->to_a = -o_ptr->to_a;
 #ifndef TO_AC_CAP_30
		if (o_ptr->to_a > 35) o_ptr->to_a = 35;
 #else
		if (o_ptr->to_a > 30) o_ptr->to_a = 30;
 #endif
	}
	/* non-armour doesn't get exaggerating ac boni */
	if (!is_armour(o_ptr->tval) && o_ptr->to_a > 15) o_ptr->to_a = 15;

	/* reverse +pval/bpval */
	if (o_ptr->pval < 0) {
		o_ptr->pval_org = o_ptr->pval;
		o_ptr->pval = -(o_ptr->pval - 3) / 4; //the evil gods are pleased '>_>.. (thinking of +LUCK)
		if (o_ptr->pval > 3) o_ptr->pval = 3; //thinking EA/Life, but just paranoia really..
	}
	if (o_ptr->bpval < 0) {
		o_ptr->bpval_org = o_ptr->bpval;
		o_ptr->bpval = -(o_ptr->bpval - 3) / 4; //the evil gods are pleased '>_>.. (thinking of +LUCK)
		if (o_ptr->bpval > 3) o_ptr->bpval = 3; //thinking EA/Life, but just paranoia really..
	}
}
/* Reverse the boni back to negative when a vampire takes off a heavily cursed item (or its curse gets broken),
   counterpart function to inverse_cursed(). */
void reverse_cursed(object_type *o_ptr) {
	u32b f1, f2, f3, f4, f5, f6, esp;

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
	if (!(f3 & TR3_HEAVY_CURSE)) return;
	if ((f4 & TR4_NEVER_BLOW)) return; /* Weapons of Nothingness */

	/* actually a bit special: account for (dis)enchantments that might have happened meanwhile or at some point.
	   !!! NOTE: enchanting/discharging this way (the .._org vars too) has not yet been implemented! */
 #if (VAMPIRES_INV_CURSED == 0) /* this way, enchant would actually further increase the bonus, while it's equipped! (not so consistent) */
	if (o_ptr->to_h > 0 && o_ptr->to_h_org < 0) {
		o_ptr->to_h += o_ptr->to_h_org * 2;
		if (o_ptr->to_h < o_ptr->to_h_org) o_ptr->to_h = o_ptr->to_h_org;
	}
	if (o_ptr->to_d > 0 && o_ptr->to_d_org < 0) {
		o_ptr->to_d += o_ptr->to_d_org * 2;
		if (o_ptr->to_h < o_ptr->to_h_org) o_ptr->to_h = o_ptr->to_h_org;
	}
	if (o_ptr->to_a > 0 && o_ptr->to_a_org < 0) {
		o_ptr->to_a += o_ptr->to_a_org * 2;
		if (o_ptr->to_h < o_ptr->to_h_org) o_ptr->to_h = o_ptr->to_h_org;
	}
	if (o_ptr->pval > -o_ptr->pval_org / 4 && o_ptr->pval_org < 0) o_ptr->pval = -o_ptr->pval;
	if (o_ptr->bpval > -o_ptr->bpval_org / 4 && o_ptr->bpval_org < 0) o_ptr->bpval = -o_ptr->bpval;
 #else /* this way, enchant and disenchant would BOTH reduce the boni! (recommended, more consistent) */
	/* ..the drawback here (that we accept for now) is that you can actually 'disenchant' negative boni up to 0,
	   thereby improving the item that way, which usually isn't possible.. However, usually items that resist
	   enchantment do also ignore disenchantment, so probably we're not opening a loophole here. */
	if (o_ptr->to_h > 0 && o_ptr->to_h_org < 0) o_ptr->to_h = -o_ptr->to_h;
	if (o_ptr->to_d > 0 && o_ptr->to_d_org < 0) o_ptr->to_d = -o_ptr->to_d;
	if (o_ptr->to_a > 0 && o_ptr->to_a_org < 0) o_ptr->to_a = -o_ptr->to_a;
	if (o_ptr->pval > 0 && o_ptr->pval_org < 0) {
		o_ptr->pval = -o_ptr->pval * 4;
		if (o_ptr->pval < o_ptr->pval_org) o_ptr->pval = o_ptr->pval_org;
	}
	if (o_ptr->bpval > 0 && o_ptr->bpval_org < 0) {
		o_ptr->bpval = -o_ptr->bpval * 4;
		if (o_ptr->bpval < o_ptr->bpval_org) o_ptr->bpval = o_ptr->bpval_org;
	}
 #endif
	/* body armour hack: we can ignore possible to-hit disenchantment here, since on body armour it's always negative (bulkiness) */
	if (o_ptr->tval == TV_DRAG_ARMOR || o_ptr->tval == TV_HARD_ARMOR || o_ptr->tval == TV_SOFT_ARMOR)
		o_ptr->to_h = o_ptr->to_h_org;
}
#endif

void init_treasure_classes(void) {
	int i, n, t;
	alloc_entry *restrict table = alloc_kind_table;
	int total, total_treasure, total_combat, total_magic, total_tools, total_junk;
	obj_theme theme;

	/* Max object level, 5*max bias factor */
	int level = 127, max_bias = 25;

	/* We need a balanced theme for finding any bias */
	theme.treasure = 20;
	theme.combat = 20;
	theme.magic = 20;
	theme.tools = 20;
	//and the remaining 20% of 'junk'
	init_match_theme(theme);


	/* Set filter: Monster is assumed to drop normal / good / great. */
	get_obj_num_hook = kind_is_normal; //bare, normal allocation. No extra flags.

	total = 0;
	total_treasure = 0;
	total_combat = 0;
	total_magic = 0;
	total_tools = 0;
	total_junk = 0;

	get_obj_num_prep(RESF_NONE);
	n = alloc_kind_index_level[level + 1];
	total = 0;
	for (i = 0; i < n; i++) {
		total += table[i].prob2;
		t = which_theme(k_info[table[i].index].tval);
		switch (t) {
		case 1:
			total_treasure += table[i].prob2;
			//s_printf("1: %s, ", k_name + k_info[table[i].index].name);
			continue;
		case 2:
			total_combat += table[i].prob2;
			//s_printf("2: %s, ", k_name + k_info[table[i].index].name);
			continue;
		case 3:
			total_magic += table[i].prob2;
			//s_printf("3: %s, ", k_name + k_info[table[i].index].name);
			continue;
		case 4:
			total_tools += table[i].prob2;
			//s_printf("4: %s, ", k_name + k_info[table[i].index].name);
			continue;
		case 0:
			total_junk += table[i].prob2;
			//s_printf("5: %s, ", k_name + k_info[table[i].index].name);
			continue;
		default:
			continue; //-1, unclassified item (paranoia)
		}
	}

	s_printf("Accumulated Treasure Class Biasses (normal) : %5d, %5d, %5d, %5d, %5d.\n", total_treasure, total_combat, total_magic, total_tools, total_junk);

	/* Limit to x10; limiting is needed to prevent div0 anyway. */
	if (total_treasure < total / max_bias) total_treasure = total / max_bias;
	if (total_combat < total / max_bias) total_combat = total / max_bias;
	if (total_magic < total / max_bias) total_magic = total / max_bias;
	if (total_tools < total / max_bias) total_tools = total / max_bias;
	if (total_junk < total / max_bias) total_junk = total / max_bias;

	/* Build bias multipliers (percentages) to even out all treasure classes. */
	tc_bias_treasure = (100 * total) / (total_treasure * 5);
	tc_bias_combat = (100 * total) / (total_combat * 5);
	tc_bias_magic = (100 * total) / (total_magic * 5);
	tc_bias_tools = (100 * total) / (total_tools * 5);
	tc_bias_junk = (100 * total) / (total_junk * 5);

	s_printf("Initialized Treasure Class Biasses (normal) : %4d%%, %4d%%, %4d%%, %4d%%, %4d%%.\n", tc_bias_treasure, tc_bias_combat, tc_bias_magic, tc_bias_tools, tc_bias_junk);


	/* Set filter: Monster is assumed to drop normal / good / great. */
	get_obj_num_hook = kind_is_good; //DROP_GOOD monsters

	total = 0;
	total_treasure = 0;
	total_combat = 0;
	total_magic = 0;
	total_tools = 0;
	total_junk = 0;

	get_obj_num_prep(RESF_NONE);
	n = alloc_kind_index_level[level + 1];
	total = 0;
	for (i = 0; i < n; i++) {
		total += table[i].prob2;
		t = which_theme(k_info[table[i].index].tval);
		switch (t) {
		case 1:
			total_treasure += table[i].prob2;
			//s_printf("1: %s, ", k_name + k_info[table[i].index].name);
			continue;
		case 2:
			total_combat += table[i].prob2;
			//s_printf("2: %s, ", k_name + k_info[table[i].index].name);
			continue;
		case 3:
			total_magic += table[i].prob2;
			//s_printf("3: %s, ", k_name + k_info[table[i].index].name);
			continue;
		case 4:
			total_tools += table[i].prob2;
			//s_printf("4: %s, ", k_name + k_info[table[i].index].name);
			continue;
		case 0:
			total_junk += table[i].prob2;
			//s_printf("5: %s, ", k_name + k_info[table[i].index].name);
			continue;
		default:
			continue; //-1, unclassified item (paranoia)
		}
	}

	s_printf("Accumulated Treasure Class Biasses (good)   : %5d, %5d, %5d, %5d, %5d.\n", total_treasure, total_combat, total_magic, total_tools, total_junk);

	/* Limit to x10; limiting is needed to prevent div0 anyway. */
	if (total_treasure < total / max_bias) total_treasure = total / max_bias;
	if (total_combat < total / max_bias) total_combat = total / max_bias;
	if (total_magic < total / max_bias) total_magic = total / max_bias;
	if (total_tools < total / max_bias) total_tools = total / max_bias;
	if (total_junk < total / max_bias) total_junk = total / max_bias;

	/* Build bias multipliers (percentages) to even out all treasure classes. */
	tc_biasg_treasure = (100 * total) / (total_treasure * 5);
	tc_biasg_combat = (100 * total) / (total_combat * 5);
	tc_biasg_magic = (100 * total) / (total_magic * 5);
	tc_biasg_tools = (100 * total) / (total_tools * 5);
	tc_biasg_junk = (100 * total) / (total_junk * 5);

	s_printf("Initialized Treasure Class Biasses (good)   : %4d%%, %4d%%, %4d%%, %4d%%, %4d%%.\n", tc_biasg_treasure, tc_biasg_combat, tc_biasg_magic, tc_biasg_tools, tc_biasg_junk);


	/* Set filter: Monster is assumed to drop normal / good / great. */
	get_obj_num_hook = kind_is_great; //DROP_GREAT monsters

	total = 0;
	total_treasure = 0;
	total_combat = 0;
	total_magic = 0;
	total_tools = 0;
	total_junk = 0;

	get_obj_num_prep(RESF_NONE);
	n = alloc_kind_index_level[level + 1];
	total = 0;
	for (i = 0; i < n; i++) {
		total += table[i].prob2;
		t = which_theme(k_info[table[i].index].tval);
		switch (t) {
		case 1:
			total_treasure += table[i].prob2;
			//s_printf("1: %s, ", k_name + k_info[table[i].index].name);
			continue;
		case 2:
			total_combat += table[i].prob2;
			//s_printf("2: %s, ", k_name + k_info[table[i].index].name);
			continue;
		case 3:
			total_magic += table[i].prob2;
			//s_printf("3: %s, ", k_name + k_info[table[i].index].name);
			continue;
		case 4:
			total_tools += table[i].prob2;
			//s_printf("4: %s, ", k_name + k_info[table[i].index].name);
			continue;
		case 0:
			total_junk += table[i].prob2;
			//s_printf("5: %s, ", k_name + k_info[table[i].index].name);
			continue;
		default:
			continue; //-1, unclassified item (paranoia)
		}
	}

	s_printf("Accumulated Treasure Class Biasses (great)  : %5d, %5d, %5d, %5d, %5d.\n", total_treasure, total_combat, total_magic, total_tools, total_junk);

	/* Limit to x10; limiting is needed to prevent div0 anyway. */
	if (total_treasure < total / max_bias) total_treasure = total / max_bias;
	if (total_combat < total / max_bias) total_combat = total / max_bias;
	if (total_magic < total / max_bias) total_magic = total / max_bias;
	if (total_tools < total / max_bias) total_tools = total / max_bias;
	if (total_junk < total / max_bias) total_junk = total / max_bias;

	/* Build bias multipliers (percentages) to even out all treasure classes. */
	tc_biasr_treasure = (100 * total) / (total_treasure * 5);
	tc_biasr_combat = (100 * total) / (total_combat * 5);
	tc_biasr_magic = (100 * total) / (total_magic * 5);
	tc_biasr_tools = (100 * total) / (total_tools * 5);
	tc_biasr_junk = (100 * total) / (total_junk * 5);

	s_printf("Initialized Treasure Class Biasses (great)  : %4d%%, %4d%%, %4d%%, %4d%%, %4d%%.\n", tc_biasr_treasure, tc_biasr_combat, tc_biasr_magic, tc_biasr_tools, tc_biasr_junk);
}
