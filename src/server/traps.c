/* File: traps.c */

/* Purpose: handle traps */

/* the below copyright probably still applies, but it is heavily changed
 * copied, adapted & re-engineered by JK.
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

/*
 * Adapted from PernAngband to PernMangband by Jir.
 * All these changes also take over the Angband-licence
 * (as you see above).
 */
#include "angband.h"


/*
 * Acquires and returns the index of a "free" trap.
 *
 * This routine should almost never fail, but it *can* happen.
 *
 * Note that this function must maintain the special "t_fast"
 * array of indexes of "live" traps.
 */
s16b t_pop(void)
{
	int i, n, k;


	/* Normal allocation */
	if (t_max < MAX_TR_IDX)
	{
		/* Access the next hole */
		i = t_max;

		/* Expand the array */
		t_max++;

		/* Update "m_fast" */
		t_fast[t_top++] = i;

		/* Return the index */
		return (i);
	}

	/* Check for some space */
	for (n = 1; n < MAX_TR_IDX; n++)
	{
		/* Get next space */
		i = t_nxt;

		/* Advance (and wrap) the "next" pointer */
		if (++t_nxt >= MAX_TR_IDX) t_nxt = 1;

		/* Skip traps in use */
		if (t_list[i].t_idx) continue;

		/* Verify space XXX XXX */
		if (t_top >= MAX_TR_IDX) continue;

		/* Verify not allocated */
		for (k = 0; k < t_top; k++)
		{
			/* Hack -- Prevent errors */
			if (t_fast[k] == i) i = 0;
		}

		/* Oops XXX XXX */
		if (!i) continue;

		/* Update "m_fast" */
		t_fast[t_top++] = i;

		/* Use this trap */
		return (i);
	}


	/* Warn the player */
	if (server_dungeon) s_printf("Too many traps!");

	/* Try not to crash */
	return (0);
}


/*
 * *** This note needs rewriting! ***	- Jir -
 *
 * Delete a trap by index.
 *
 * This function causes the given trap to cease to exist for
 * all intents and purposes.  The trap record is left in place
 * but the record is wiped, marking it as "dead" (no type index)
 * so that it can be "skipped" when scanning the trap array,
 * and "excised" from the "t_fast" array when needed.
 *
 * Thus, anyone who makes direct reference to the "t_list[]" array
 * using trap indexes that may have become invalid should be sure
 * to verify that the "t_idx" field is non-zero.
 *
 * references to "t_list[c_ptr->t_idx]" are *not* guaranteed
 * to be valid, see below.
 */
// void delete_trap_idx(int i)
void delete_trap_idx(trap_type *t_ptr)
{
	int x, y, Ind;
#ifdef NEW_DUNGEON
	struct worldpos *wpos;
	cave_type **zcave;
#else
	int Depth;
#endif
//	monster_type *m_ptr = &m_list[i];

//	trap_kind *tk_ptr = &t_info[t_ptr->t_idx];

	/* Get location */
	y = t_ptr->iy;
	x = t_ptr->ix;
#ifdef NEW_DUNGEON
	wpos=&t_ptr->wpos;
#else
	Depth = t_ptr->dun_depth;
#endif

	/* Trap is gone */
	/* Make sure the level is allocated, it won't be if we are
	 * clearing an abandoned wilderness level of traps
	 */
#ifdef NEW_DUNGEON
	if((zcave=getcave(wpos))){
		zcave[y][x].special.type=0;
		zcave[y][x].special.ptr = NULL;
		t_ptr->t_idx=0;
	}

	/* Forget the "field mark" */
//	everyone_forget_spot(wpos, y, x);

	/* Notice */
//	note_spot_depth(wpos, y, x);

	/* Redisplay the grid */
	everyone_lite_spot(wpos, y, x);

#else	// pfft, this never works
	if (cave[Depth])
		cave[Depth][y][x].m_idx = 0;

	/* Visual update */
	everyone_lite_spot(Depth, y, x);
#endif
	/* Wipe the trap */
//	FREE(tt_ptr, trap_kind);
	WIPE(t_ptr, trap_type);
}


/*
 * Compact and Reorder the trap list
 *
 * This function can be very dangerous, use with caution!
 * (Even more dangerous than before		- Jir -)
 * When actually "compacting" traps, we base the saving throw on a
 * combination of trap level, distance from player, and current
 * "desperation".
 *
 * After "compacting" (if needed), we "reorder" the traps into a more
 * compact order, and we reset the allocation info, and the "live" array.
 */
void compact_traps(int size)
{
	int i, y, x, num, cnt, Ind;

	int cur_val, cur_lev, cur_dis, chance;


	/* Compact */
	if (size)
	{
		/* Message */
		s_printf("Compacting traps...\n");

		/* Redraw map */
		/*p_ptr->redraw |= (PR_MAP);*/

		/* Window stuff */
		/*p_ptr->window |= (PW_OVERHEAD);*/
	}


	/* Compact at least 'size' traps */
	for (num = 0, cnt = 1; num < size; cnt++)
	{
		/* Get more vicious each iteration */
//		cur_lev = 5 * cnt;

		/* Destroy more valuable items each iteration */
//		cur_val = 500 * (cnt - 1);

		/* Get closer each iteration */
		cur_dis = 5 * (20 - cnt);

		/* Examine the traps */
		for (i = 1; i < o_max; i++)
		{
			trap_type *t_ptr = &t_list[i];

			trap_kind *tk_ptr = &t_info[t_ptr->t_idx];

			/* Skip dead traps */
			if (!t_ptr->t_idx) continue;

			/* Get the location */
			y = t_ptr->iy;
			x = t_ptr->ix;

			/* Nearby traps start out "immune" */
			/*if ((cur_dis > 0) && (distance(py, px, y, x) < cur_dis)) continue;*/

			/* Valuable traps start out "immune" */
//			if (trap_value(0, &o_list[i]) > cur_val) continue;

			/* Saving throw */
			chance = 90;

			/* Hack -- only compact items in houses in emergencies */

			/* Apply the saving throw */
			if (rand_int(100) < chance) continue;

			/* Delete it */
			delete_trap_idx(t_ptr);

			/* Count it */
			num++;
		}
	}


	/* Excise dead traps (backwards!) */
	for (i = t_max - 1; i >= 1; i--)
	{
		/* Get the i'th trap */
		trap_type *t_ptr = &t_list[i];

		/* Skip real traps */
		if (t_ptr->t_idx) continue;

		/* One less trap */
		t_max--;

		/* Reorder */
		if (i != t_max)
		{
			int ny = t_list[t_max].iy;
			int nx = t_list[t_max].ix;
#ifdef NEW_DUNGEON
//			struct worldpos *wpos=&t_list[t_max].wpos;
//			cave_type **zcave;
#else
			int Depth = t_list[t_max].dun_depth;
#endif
#if 0
			/* Update the cave */
			/* Hack -- with wilderness traps, sometimes the cave is not allocated,
			   so check that it is. */
#ifdef NEW_DUNGEON
			if ((zcave=getcave(wpos))){
				zcave[ny][nx].t_idx = i;
			}
#else
			if (cave[Depth]) cave[Depth][ny][nx].t_idx = i;
#endif
#endif	// 0
			/* Structure copy */
			t_list[i] = t_list[t_max];

			/* Wipe the hole */
			WIPE(&t_list[t_max], trap_type);
		}
	}

	/* Reset "t_nxt" */
	t_nxt = t_max;


	/* Reset "t_top" */
	t_top = 0;

	/* Collect "live" traps */
	for (i = 0; i < t_max; i++)
	{
		/* Collect indexes */
		t_fast[t_top++] = i;
	}
}


/*
 * Delete all the traps when player leaves the level
 *
 * Note -- we do NOT visually reflect these (irrelevant) changes
 */
 
#ifdef NEW_DUNGEON
void wipe_t_list(struct worldpos *wpos)
#else
void wipe_t_list(int Depth)
#endif
{
	int i, x, y;

	/* Delete the existing traps */
	for (i = 1; i < t_max; i++)
	{
		trap_type *t_ptr = &t_list[i];

		/* Skip dead traps */
		if (!t_ptr->t_idx) continue;

		/* Skip traps not on this depth */
#ifdef NEW_DUNGEON
		if (!inarea(&t_ptr->wpos, wpos))
#else
		if (t_ptr->dun_depth != Depth)
#endif
			continue;

		/* Delete it */


		/* Wipe the trap */
		WIPE(t_ptr, trap_type);
	}

	/* Compact the trap list */
	compact_traps(0);
}

/*
 * Set the "t_idx" fields in the cave array to correspond
 * to the traps in the "t_list".
 */
void setup_traps(void)
{
	int i;
#ifdef NEW_DUNGEON
	cave_type **zcave;
#endif

	for (i = 0; i < t_max; i++)
	{
		trap_type *t_ptr = &t_list[i];

		/* Skip dead traps */
		if (!t_ptr->t_idx) continue;

		/* Skip traps on depths that aren't allocated */
#ifdef NEW_DUNGEON
		if (!(zcave=getcave(&t_ptr->wpos))) continue;
#else
		if (!cave[t_ptr->dun_depth]) continue;
#endif

		/* Set the t_idx correctly */
#ifdef NEW_DUNGEON
		zcave[t_ptr->iy][t_ptr->ix].special.type = CS_TRAPS;
		zcave[t_ptr->iy][t_ptr->ix].special.ptr = t_ptr;
#else
		cave[t_ptr->dun_depth][t_ptr->iy][t_ptr->ix].t_idx = i;
#endif
	}
}

/*
 * This function can be *much* shorter if we use functions in
 * party.c; it's written in this way so that all this will be
 * done w/o anyone noticing it.		- Jir -
 */
static bool player_handle_trap_of_hostility(int Ind)
{
	player_type *p_ptr = Players[Ind], *q_ptr;
	int i, id = p_ptr->party;
	hostile_type *h_ptr;
	bool ident = FALSE;

	if (id != 0)
	{
		ident = TRUE;
		/* If (s)he's party owner, everyone leaves it */
		if (streq(parties[id].owner, p_ptr->name))
		{
			//	del_party(party_id);	// check it!!
			/* Remove the party altogether */
			/* (probably it's too severe?) */
//			kill_houses(id, OT_PARTY);

			/* Set the number of people in this party to zero */
			parties[id].num = 0;

			/* Remove everyone else */
			for (i = 1; i <= NumPlayers; i++)
			{
				/* Check if they are in here */
				if (player_in_party(id, i) && i != Ind)
				{
					/* Lose a member */
					parties[id].num--;

					Players[i]->party = 0;
					//					msg_print(i, "Your party has been disbanded.");
					Send_party(i);
				}
			}

			/* Set the creation time to "disbanded time" */
//			parties[id].created = turn;

			/* Empty the name */
//			strcpy(parties[id].name, "");
		}
		/* otherwise, leave it */
		else
		{
			/* Lose a member */
			parties[id].num--;

			/* Set his party number back to "neutral" */
			p_ptr->party = 0;

			/* Resend info */
			Send_party(Ind);
		}
	}

	/* this player is hoe of everyone now */
	for (i = 1; i <= NumPlayers; i++)
	{
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		if (i == Ind) continue;

		q_ptr = Players[i];

		if (!check_hostile(Ind, i))
		{
			ident = TRUE;

			/* Create a new hostility node */
			MAKE(h_ptr, hostile_type);

			/* Set ID in node */
			h_ptr->id = q_ptr->id;

			/* Put this node at the beginning of the list */
			h_ptr->next = p_ptr->hostile;
			p_ptr->hostile = h_ptr;
		}

		if (!check_hostile(i, Ind))
		{
			ident = TRUE;

			/* Create a new hostility node */
			MAKE(h_ptr, hostile_type);

			/* Set ID in node */
			h_ptr->id = p_ptr->id;

			/* Put this node at the beginning of the list */
			h_ptr->next = q_ptr->hostile;
			q_ptr->hostile = h_ptr;
		}
	}


}

bool do_trap_of_silliness(int Ind, int power)
{
	player_type *p_ptr = Players[Ind];
	int i, j;
	bool aware = FALSE;

	for (i = 0; i < power; i++)
	{
		j = rand_int(MAX_K_IDX);
		if (p_ptr->obj_aware[j]) aware = TRUE;
		p_ptr->obj_aware[j] = 0;

		j = rand_int(MAX_T_IDX);
		if (p_ptr->trap_ident[j]) aware = TRUE;
		p_ptr->trap_ident[j] = 0;
	}

	return (aware);

}

bool do_player_drop_items(int Ind, int chance)
{
	player_type *p_ptr = Players[Ind];
	s16b i;
	bool message = FALSE, ident = FALSE;

	for (i=0;i<INVEN_PACK;i++)
	{
		object_type tmp_obj;
		if (!p_ptr->inventory[i].k_idx) continue;
		if (randint(100)>chance) continue;
		if (p_ptr->inventory[i].name1 == ART_POWER) continue;
		if (cfg.anti_arts_horde && (artifact_p(&tmp_obj)) && (!tmp_obj.name3) && (rand_int(100)>9)) continue;
		tmp_obj = p_ptr->inventory[i];
		/* drop carefully */
		drop_near_severe(Ind, &tmp_obj, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px);
		inven_item_increase(Ind, i,-999);
		inven_item_optimize(Ind, i);
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);
		if (!message)
		{
			msg_print(Ind, "You are startled by a sudden sound.");
			message = TRUE;
		}
		ident = TRUE;
	}
	if (!ident)
	{
		msg_print(Ind, "You hear a sudden, strange sound.");
	}

	return (ident);
}

bool do_player_scatter_items(int Ind, int chance)
{
	player_type *p_ptr = Players[Ind];
	s16b i,j;
	bool message = FALSE;
	cave_type **zcave;
	zcave=getcave(&p_ptr->wpos);

	for (i=0;i<INVEN_PACK;i++)
	{
		if (!p_ptr->inventory[i].k_idx) continue;
		if (rand_int(100)>chance) continue;
		for (j=0;j<10;j++)
		{
			object_type tmp_obj;
			s16b cx = p_ptr->px+15-rand_int(30);
			s16b cy = p_ptr->py+15-rand_int(30);
			if (!in_bounds(cy,cx)) continue;
			if (!cave_floor_bold(zcave, cy,cx)) continue;
			tmp_obj = p_ptr->inventory[i];
			if (cfg.anti_arts_horde && (artifact_p(&tmp_obj)) && (!tmp_obj.name3) && (rand_int(100)>9)) return(FALSE);
			inven_item_increase(Ind, i,-999);
			inven_item_optimize(Ind, i);
			p_ptr->notice |= (PN_COMBINE | PN_REORDER);
			//                  (void)floor_carry(cy, cx, &tmp_obj);
			drop_near_severe(Ind, &tmp_obj, 0, &p_ptr->wpos, cy, cx);
			if (!message)
			{
				msg_print(Ind, "You feel light-footed.");
				message = TRUE;
			}
			if (player_has_los_bold(Ind, cy, cx))
			{
				char i_name[80];
				object_desc(Ind, i_name, &tmp_obj, TRUE, 3);
				note_spot(Ind, cy, cx);
				lite_spot(Ind, cy, cx);
				message=TRUE;
				msg_format(Ind, "Suddenly %s appear%s!",i_name, (tmp_obj.number>1)?"":"s");
			}
			break;
		}
	}

	return (message);
}

bool do_player_trap_garbage(int Ind, int times)
{
	player_type *p_ptr = Players[Ind];
	int k, l, lv = getlevel(&p_ptr->wpos);
	bool ident = FALSE;
	object_type *o_ptr, forge;

	for(k = 0; k < times; k++)
	{
		l = rand_int(MAX_K_IDX);

		/* hack -- !ruin, !death cannot be generated */
		if (!k_info[l].tval || k_info[l].cost || k_info[l].level > lv || k_info[l].level > 30) continue;

		o_ptr = &forge;
		object_prep(o_ptr, l);

		ident = TRUE;
		if (inven_carry(Ind, o_ptr) < 0)
			drop_near(o_ptr, -1, &p_ptr->wpos, p_ptr->py, p_ptr->px);
	}
	return (ident);
}

bool do_player_trap_call_out(int Ind)
{
	player_type *p_ptr = Players[Ind];
   s16b          i,sn,cx,cy;
   s16b          h_index = 0;
   s16b          h_level = 0;
   monster_type  *m_ptr;
   monster_race  *r_ptr;
   char          m_name[80];
   bool          ident = FALSE;
	cave_type **zcave;
	zcave=getcave(&p_ptr->wpos);

   for (i = 1; i < m_max; i++)
   {
       m_ptr = &m_list[i];
       r_ptr = race_inf(m_ptr);

       /* Paranoia -- Skip dead monsters */
       if (!m_ptr->r_idx) continue;

	   if (!wpcmp(&p_ptr->wpos, &m_ptr->wpos)) continue;
       if (m_ptr->level>=h_level)
       {
         h_level = m_ptr->level;
         h_index = i;
       }
   }
   /* if the level is empty of monsters, h_index will be 0 */
   if (!h_index) return(FALSE);

   m_ptr = &m_list[h_index];

   sn = 0;
   for (i = 0; i < 8; i++)
   {
      cx = p_ptr->px + ddx[i];
      cy = p_ptr->py + ddy[i];
      /* Skip non-empty grids */
      if (!cave_valid_bold(zcave, cy, cx)) continue;
      if (zcave[cy][cx].feat == FEAT_GLYPH) continue;
      if ((cx==p_ptr->px) && (cy==p_ptr->py)) continue;
      sn++;
      /* Randomize choice */
      if (rand_int(sn) > 0) continue;
      zcave[cy][cx].m_idx=h_index;
      zcave[m_ptr->fy][m_ptr->fx].m_idx=0;
      m_ptr->fx = cx;
      m_ptr->fy = cy;
      /* we do not change the sublevel! */
      ident=TRUE;
      update_mon(h_index, TRUE);
      monster_desc(Ind, m_name, h_index, 0x08);
      msg_format(Ind, "You hear a rapid-shifting wail, and %s appears!",m_name);
      break;
   }
   return (ident);
}

/* done */
static bool do_trap_teleport_away(int Ind, object_type *i_ptr, s16b y, s16b x)
{
	player_type *p_ptr = Players[Ind];
	bool ident = FALSE;
	char o_name[80];

        s16b  o_idx = 0;
	object_type *o_ptr;
	cave_type *c_ptr;

	s16b  x1;
	s16b  y1;

	/* Paranoia */
	cave_type **zcave;
	if (!in_bounds(y, x)) return;
	if((zcave=getcave(&p_ptr->wpos)))
	{
		/* naught */
//		c_ptr=&zcave[y][x];
	}
	else return(FALSE);

	if (i_ptr == NULL) return(FALSE);

	/* God save the arts :) */
	if (i_ptr->name1 == ART_POWER) return (FALSE);
	if (cfg.anti_arts_horde && (artifact_p(o_ptr)) && (!o_ptr->name3) && (rand_int(100)>9)) return(FALSE);

	while (o_idx == 0)
	{
		x1 = rand_int(p_ptr->cur_wid);
		y1 = rand_int(p_ptr->cur_hgt);

		/* Obtain grid */
#if 0	// pfft, put off till f_info is revised
		c_ptr=&zcave[y][x];
		/* Require floor space (or shallow terrain) -KMW- */
		if (!(f_info[c_ptr->feat].flags1 & FF1_FLOOR)) continue;
#endif
		if (!cave_clean_bold(zcave, y1, x1)) continue;
		o_idx = drop_near_severe(Ind, i_ptr, 0, &p_ptr->wpos, y1, x1);
	}

	o_ptr = &o_list[o_idx];

	x1 = o_ptr->ix;
	y1 = o_ptr->iy;

	if (!p_ptr->blind)
	{
		note_spot(Ind, y,x);
		lite_spot(Ind, y,x);
		ident=TRUE;
		object_desc(Ind, o_name, i_ptr, FALSE, 0);
		if (player_has_los_bold(Ind, y1, x1))
		{
			lite_spot(Ind, y1, x1);
			msg_format(Ind, "The %s suddenly stands elsewhere", o_name);

		}
		else
		{
			msg_format(Ind, "You suddenly don't see the %s anymore!", o_name);
		}
	}
	else
	{
		msg_print(Ind, "You hear something move");
	}
	return (ident);
}

/*
 * this handles a trap that places walls around the player
 */
#if 0	// after f_info reform! :)
static bool player_handle_trap_of_walls(void)
{
   bool ident;

   {
      s16b dx,dy,cx,cy;
      s16b sx=0,sy=0,sn,i;
      cave_type *cv_ptr;
      bool map[5][5] = { {FALSE,FALSE,FALSE,FALSE,FALSE},
                         {FALSE,FALSE,FALSE,FALSE,FALSE},
                         {FALSE,FALSE,FALSE,FALSE,FALSE},
                         {FALSE,FALSE,FALSE,FALSE,FALSE},
                         {FALSE,FALSE,FALSE,FALSE,FALSE} };
      for (dy = -2; dy <= 2; dy++)
      {
         for (dx = -2; dx <= 2; dx++)
         {
            cx = px+dx;
            cy = py+dy;
            if (!in_bounds(cy, cx)) continue;
            cv_ptr = &cave[cy][cx];

// DGDGDGDG
            if (cv_ptr->m_idx) continue;

            /* Lose room and vault */
            cv_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);
            /* Lose light and knowledge */
            cv_ptr->info &= ~(CAVE_GLOW | CAVE_MARK);
            /* Skip the center */
            if (!dx && !dy) continue;
            /* test for dungeon level */
            if (randint(100) > 10+max_dlv[dungeon_type]) continue;
            /* Damage this grid */
            map[2+dx][2+dy] = TRUE;
         }
      }
      for (dy = -2; dy <= 2; dy++)
      {
         for (dx = -2; dx <= 2; dx++)
         {
            cx = px+dx;
            cy = py+dy;
            if (!map[2+dx][2+dy]) continue;
            cv_ptr = &cave[cy][cx];

            if (cv_ptr->m_idx)
            {
               monster_type *m_ptr = &m_list[cv_ptr->m_idx];
               monster_race *r_ptr = race_inf(m_ptr);
               /* Most monsters cannot co-exist with rock */
               if (!(r_ptr->flags2 & RF2_KILL_WALL) &&
                   !(r_ptr->flags2 & RF2_PASS_WALL))
               {
                  char m_name[80];

                  /* Assume not safe */
                  sn = 0;
                  /* Monster can move to escape the wall */
                  if (!(r_ptr->flags1 & RF1_NEVER_MOVE))
                  {
                     /* Look for safety */
                     for (i = 0; i < 8; i++)
                     {
                        /* Access the grid */
                        cy = py + ddy[i];
                        cx = px + ddx[i];

                        /* Skip non-empty grids */
                        if (!cave_clean_bold(cy, cx)) continue;

                        /* Hack -- no safety on glyph of warding */
			if (cave[cy][cx].feat == FEAT_GLYPH) continue;

                        /* Important -- Skip "quake" grids */
                        if (map[2+(cx-px)][2+(cy-py)]) continue;
                        /* Count "safe" grids */
                        sn++;
                        /* Randomize choice */
                        if (rand_int(sn) > 0) continue;
                        /* Save the safe grid */
                        sx = cx; sy = cy;
                        ident=TRUE;
                        break; /* discontinue for loop - safe grid found */
                     }
                  }
                  /* Describe the monster */
                  monster_desc(m_name, cv_ptr->m_idx, 0);
                  /* Scream in pain */
                  msg_format("%^s wails out in pain!", m_name);
                  /* Monster is certainly awake */
                  m_ptr->csleep = 0;
                  /* Apply damage directly */
                  m_ptr->hp -= (sn ? damroll(4, 8) : 200);
                  /* Delete (not kill) "dead" monsters */
                  if (m_ptr->hp < 0)
                  {
                     /* Message */
                     msg_format("%^s is entombed in the rock!", m_name);
                     /* Delete the monster */
                     delete_monster_idx(cave[cy][cx].m_idx);
                     /* No longer safe */
                     sn = 0;
                  }
                  /* Hack -- Escape from the rock */
                  if (sn)
                  {
                     s16b m_idx = cave[cy][cx].m_idx;

                     /* Update the new location */
                     cave[sy][sx].m_idx = m_idx;

                     /* Update the old location */
                     cave[cy][cx].m_idx = 0;

                     /* Move the monster */
                     m_ptr->fy = sy;
                     m_ptr->fx = sx;

                     /* do not change fz */
                     /* don't make rock on that square! */
                     if ( (sx>=(px-2)) &&
                        (sx<=(px+2)) &&
                        (sy>=(py-2)) &&
                        (sy<=(py+2)) )
                     {
                        map[2+(sx-px)][2+(sy-py)]=FALSE;
                     }
                     /* Update the monster (new location) */
                     update_mon(m_idx, TRUE);

                     /* Redraw the old grid */
                     lite_spot(cy, cx);

                     /* Redraw the new grid */
                     lite_spot(sy, sx);
                  } /* if sn */
               } /* if monster can co-exist with rock */
            } /* if monster on square */
         } /* for dx */
      } /* for dy */

      /* Examine the quaked region */
      for (dy = -2; dy <= 2; dy++)
      {
         for (dx = -2; dx <= 2; dx++)
         {
            /* Extract the location */
            cx = px + dx;
            cy = py + dy;

            /* Skip unaffected grids */
            if (!map[2+dx][2+dy]) continue;

            /* Access the cave grid */
            cv_ptr = &cave[cy][cx];

            /* Paranoia -- never affect player */
            if (!dy && !dx) continue;

            /* Destroy location (if valid) */
            if ((cx < cur_wid) && (cy < cur_hgt) && cave_valid_bold(cy, cx))
            {
               bool floor = (f_info[cave[cy][cx].feat].flags1 & FF1_FLOOR);

               /* Delete any object that is still there */
               delete_object(cy, cx);

               if (floor)
               {
		  cave[cy][cx].feat = FEAT_WALL_OUTER;
               }
               else
               {
                  /* Clear previous contents, add floor */
		  cave[cy][cx].feat = FEAT_FLOOR;
               }
            } /* valid */
         } /* dx */
      } /* dy */
      /* Mega-Hack -- Forget the view and lite */
      p_ptr->update |= PU_UN_VIEW;

      /* Update stuff */
      p_ptr->update |= (PU_VIEW | PU_FLOW);

      /* Update the monsters */
      p_ptr->update |= (PU_DISTANCE);

      /* Update the health bar */
      p_ptr->redraw |= (PR_HEALTH);

      /* Redraw map */
      p_ptr->redraw |= (PR_MAP);

      /* Window stuff */
      p_ptr->window |= (PW_OVERHEAD);
      handle_stuff();
      msg_print("Suddenly the cave shifts around you. The air is getting stale!");
      ident=TRUE;
   }
   return (ident);
}
#endif


/*
 * this function handles arrow & dagger traps, in various types.
 * num = number of missiles
 * tval, sval = kind of missiles
 * dd,ds = damage roll for missiles
 * poison_dam = additional poison damage
 * name = name given if you should die from it...
 *
 * return value = ident (always TRUE)
 */
static bool player_handle_missile_trap(int Ind, s16b num, s16b tval, s16b sval, s16b dd, s16b ds,
                                     s16b pdam, cptr name)
{
	player_type *p_ptr = Players[Ind];
   object_type *o_ptr, forge;
   s16b        i, k_idx = lookup_kind(tval, sval);
   char        i_name[80];

   o_ptr = &forge;
   object_prep(o_ptr, k_idx);
   o_ptr->number = num;
   apply_magic(&p_ptr->wpos, o_ptr, getlevel(&p_ptr->wpos), FALSE, FALSE, FALSE);
   object_desc(Ind, i_name, o_ptr, TRUE, 0);

   if (num == 1)
      msg_format(Ind, "Suddenly %s hits you!", i_name);
   else
      msg_format(Ind, "Suddenly %s hit you!", i_name);

   for (i=0; i < num; i++)
   {
      take_hit(Ind, damroll(dd, ds), name);
      redraw_stuff(Ind);
      if (pdam > 0)
      {
         if (!(p_ptr->resist_pois || p_ptr->oppose_pois))
         {
            (void)set_poisoned(Ind, p_ptr->poisoned + pdam);
         }
      }
   }

   drop_near(o_ptr, -1, &p_ptr->wpos, p_ptr->py, p_ptr->px);

   return TRUE;
}

/*
 * this function handles a "breath" type trap - acid bolt, lightning balls etc.
 */
static bool player_handle_breath_trap(int Ind, s16b rad, s16b type, u16b trap)
{
	player_type *p_ptr = Players[Ind];
   trap_kind *t_ptr = &t_info[trap];
   bool       ident;
   s16b       my_dd, my_ds, dam;

   my_dd = t_ptr->dd;
   my_ds = t_ptr->ds;

   /* these traps gets nastier as levels progress */
   if (getlevel(&p_ptr->wpos)> (2* t_ptr->minlevel))
   {
      my_dd += (getlevel(&p_ptr->wpos) / 15);
      my_ds += (getlevel(&p_ptr->wpos) / 15);
   }
   dam = damroll(my_dd, my_ds);

   ident = project(-999, rad, &p_ptr->wpos, p_ptr->py, p_ptr->px, dam, type, PROJECT_KILL | PROJECT_JUMP);
   return (ident);
}

/*
 * This function damages the player by a trap
 */
static void trap_hit(int Ind, s16b trap)
{
   s16b dam;
   trap_kind *t_ptr = &t_info[trap];

   dam = damroll(t_ptr->dd, t_ptr->ds);

   take_hit(Ind, dam, t_name + t_ptr->name);
}

/*
 * this function activates one trap type, and returns
 * a bool indicating if this trap is now identified
 */
/*
 * 'never_id' and 'vanish' flags should be handled in tr_info.txt
 * in the future.		- Jir -
 */
//bool player_activate_trap_type(s16b y, s16b x, object_type *i_ptr, s16b item)
bool player_activate_trap_type(int Ind, s16b y, s16b x, object_type *i_ptr, s16b item)
{
	player_type *p_ptr = Players[Ind];
	bool ident = FALSE, never_id = FALSE, vanish = FALSE;
	s16b trap;
	//   dungeon_info_type *d_ptr = &d_info[dungeon_type];

	s16b k, l;
	cave_type *c_ptr;
	trap_type *tt_ptr;
//	trap_kind *t_ptr;

	/* Paranoia */
	cave_type **zcave;
	if (!in_bounds(y, x)) return;
	if((zcave=getcave(&p_ptr->wpos)))
	{
		c_ptr=&zcave[y][x];

		if (item < 0)
		{
			tt_ptr = c_ptr->special.ptr;
			trap=tt_ptr->t_idx;
		}
	}


	if (i_ptr == NULL)
	{
		if (c_ptr->o_idx==0) i_ptr=NULL;
		else i_ptr=&o_list[c_ptr->o_idx];
	}
	else
		trap=i_ptr->pval;

	if (trap == TRAP_OF_RANDOM_EFFECT)
	{
		never_id = TRUE;
		trap = TRAP_OF_ALE;
		for (l = 0; l < 99 ; l++)
		{
			k = rand_int(MAX_T_IDX);
			if (!t_info[k].name || t_info[k].minlevel > getlevel(&p_ptr->wpos) || k == TRAP_OF_ACQUIREMENT) continue;
			trap = k;
		}
	}

   switch(trap)
   {
      /* stat traps */

      case TRAP_OF_WEAKNESS_I:       ident=do_dec_stat(Ind, A_STR, STAT_DEC_TEMPORARY); break;
      case TRAP_OF_WEAKNESS_II:      ident=do_dec_stat(Ind, A_STR, STAT_DEC_NORMAL); break;
      case TRAP_OF_WEAKNESS_III:     ident=do_dec_stat(Ind, A_STR, STAT_DEC_PERMANENT); break;
      case TRAP_OF_INTELLIGENCE_I:   ident=do_dec_stat(Ind, A_INT, STAT_DEC_TEMPORARY); break;
      case TRAP_OF_INTELLIGENCE_II:  ident=do_dec_stat(Ind, A_INT, STAT_DEC_NORMAL); break;
      case TRAP_OF_INTELLIGENCE_III: ident=do_dec_stat(Ind, A_INT, STAT_DEC_PERMANENT); break;
      case TRAP_OF_WISDOM_I:         ident=do_dec_stat(Ind, A_WIS, STAT_DEC_TEMPORARY); break;
      case TRAP_OF_WISDOM_II:        ident=do_dec_stat(Ind, A_WIS, STAT_DEC_NORMAL); break;
      case TRAP_OF_WISDOM_III:       ident=do_dec_stat(Ind, A_WIS, STAT_DEC_PERMANENT); break;
      case TRAP_OF_FUMBLING_I:       ident=do_dec_stat(Ind, A_DEX, STAT_DEC_TEMPORARY); break;
      case TRAP_OF_FUMBLING_II:      ident=do_dec_stat(Ind, A_DEX, STAT_DEC_NORMAL); break;
      case TRAP_OF_FUMBLING_III:     ident=do_dec_stat(Ind, A_DEX, STAT_DEC_PERMANENT); break;
      case TRAP_OF_WASTING_I:        ident=do_dec_stat(Ind, A_CON, STAT_DEC_TEMPORARY); break;
      case TRAP_OF_WASTING_II:       ident=do_dec_stat(Ind, A_CON, STAT_DEC_NORMAL); break;
      case TRAP_OF_WASTING_III:      ident=do_dec_stat(Ind, A_CON, STAT_DEC_PERMANENT); break;
      case TRAP_OF_BEAUTY_I:         ident=do_dec_stat(Ind, A_CHR, STAT_DEC_TEMPORARY); break;
      case TRAP_OF_BEAUTY_II:        ident=do_dec_stat(Ind, A_CHR, STAT_DEC_NORMAL); break;
      case TRAP_OF_BEAUTY_III:       ident=do_dec_stat(Ind, A_CHR, STAT_DEC_PERMANENT); break;

      /* Trap of Curse Weapon */
      case TRAP_OF_CURSE_WEAPON:
         ident = curse_weapon(Ind);
         break;
      /* Trap of Curse Armor */
      case TRAP_OF_CURSE_ARMOR:
         ident = curse_armor(Ind);
         break;
      /* Earthquake Trap */
      case TRAP_OF_EARTHQUAKE:
         msg_print(Ind, "As you touch the trap, the ground starts to shake.");
         earthquake(&p_ptr->wpos, y, x, 10);
         ident=TRUE;
         break;
      /* Poison Needle Trap */
      case TRAP_OF_POISON_NEEDLE:
         if (!(p_ptr->resist_pois || p_ptr->oppose_pois))
         {
            msg_print(Ind, "You prick yourself on a poisoned needle.");
            (void)set_poisoned(Ind, p_ptr->poisoned + rand_int(15) + 10);
            ident=TRUE;
         }
         else
         {
            msg_print(Ind, "You prick yourself on a needle.");
         }
         break;
      /* Summon Monster Trap */
      case TRAP_OF_SUMMON_MONSTER:
         {
            msg_print(Ind, "A spell hangs in the air.");
//            for (k = 0; k < randint(3); k++) ident |= summon_specific(y, x, max_dlv[dungeon_type], SUMMON_UNDEAD);	// max?
            for (k = 0; k < randint(3); k++) ident |= summon_specific(&p_ptr->wpos, y, x, getlevel(&p_ptr->wpos), 0);
			if (rand_int(100)<50) vanish = TRUE;
            break;
         }
      /* Summon Undead Trap */
      case TRAP_OF_SUMMON_UNDEAD:
         {
            msg_print(Ind, "A mighty spell hangs in the air.");
            for (k = 0; k < randint(3); k++) ident |= summon_specific(&p_ptr->wpos, y, x, getlevel(&p_ptr->wpos), SUMMON_UNDEAD);
			if (rand_int(100)<50) vanish = TRUE;
            break;
         }
      /* Summon Greater Undead Trap */
      case TRAP_OF_SUMMON_GREATER_UNDEAD:
         {
            msg_print(Ind, "An old and evil spell hangs in the air.");
            for (k = 0; k < randint(3); k++) ident |= summon_specific(&p_ptr->wpos, y, x, getlevel(&p_ptr->wpos), SUMMON_HI_UNDEAD);
			if (rand_int(100)<50) vanish = TRUE;
            break;
         }
      /* Teleport Trap */
      case TRAP_OF_TELEPORT:
         msg_print(Ind, "The world whirls around you.");
         teleport_player(Ind, RATIO*67);
         ident=TRUE;
         break;
      /* Paralyzing Trap */
      case TRAP_OF_PARALYZING:
         if (!p_ptr->free_act)
         {
            msg_print(Ind, "You touch a poisoned part and can't move.");
            (void)set_paralyzed(Ind, p_ptr->paralyzed + rand_int(10) + 10);
            ident=TRUE;
         }
         else
         {
            msg_print(Ind, "You prick yourself on a needle.");
         }
         break;
      /* Explosive Device */
      case TRAP_OF_EXPLOSIVE_DEVICE:
         msg_print(Ind, "A hidden explosive device explodes in your face.");
         take_hit(Ind, damroll(5, 8), "an explosion");
         ident=TRUE;
         break;
      /* Teleport Away Trap */
      case TRAP_OF_TELEPORT_AWAY:
         {
            int item, amt;
            object_type *o_ptr;

            /* teleport away all items */
            while (c_ptr->o_idx != 0)
            {
                item = c_ptr->o_idx;
                amt = o_list[item].number;
                o_ptr = &o_list[item];

                ident = do_trap_teleport_away(Ind, o_ptr, y, x);

                floor_item_increase(item, -amt);
                floor_item_optimize(item);
            }
         }
         break;
      /* Lose Memory Trap */
      case TRAP_OF_LOSE_MEMORY:
         lose_exp(Ind, p_ptr->exp / 4);
         ident |= dec_stat(Ind, A_WIS, rand_int(20)+10, STAT_DEC_NORMAL);
         ident |= dec_stat(Ind, A_INT, rand_int(20)+10, STAT_DEC_NORMAL);
         if (!p_ptr->resist_conf)
         {
            if (set_confused(Ind, p_ptr->confused + rand_int(100) + 50))
            ident=TRUE;
         }
         if (ident)
            msg_print(Ind, "You suddenly don't remember what you were doing.");
         else
            msg_print(Ind, "You feel an alien force probing your mind.");
         break;
      /* Bitter Regret Trap */
      case TRAP_OF_BITTER_REGRET:
         msg_print(Ind, "An age-old and hideous sounding spell reverbs of the walls.");
//		ident |= dec_stat(Ind, A_DEX, 25, TRUE);	// TRUE..!?
         ident |= dec_stat(Ind, A_DEX, 25, STAT_DEC_NORMAL);
         ident |= dec_stat(Ind, A_WIS, 25, STAT_DEC_NORMAL);
         ident |= dec_stat(Ind, A_CON, 25, STAT_DEC_NORMAL);
         ident |= dec_stat(Ind, A_STR, 25, STAT_DEC_NORMAL);
         ident |= dec_stat(Ind, A_CHR, 25, STAT_DEC_NORMAL);
         ident |= dec_stat(Ind, A_INT, 25, STAT_DEC_NORMAL);
         break;
      /* Bowel Cramps Trap */
      case TRAP_OF_BOWEL_CRAMPS:
         msg_print(Ind, "A wretched smelling gas cloud upsets your stomach.");
         (void)set_food(Ind, PY_FOOD_STARVE - 1);
         (void)set_poisoned(Ind, 0);
         if (!p_ptr->free_act)
         {
            (void)set_paralyzed(Ind, p_ptr->paralyzed + rand_int(10) + 10);
         }
         ident=TRUE;
         break;
      /* Blindness/Confusion Trap */
      case TRAP_OF_BLINDNESS_CONFUSION:
         msg_print(Ind, "A powerful magic protected this.");
         if (!p_ptr->resist_blind)
         {
            (void)set_blind(Ind, p_ptr->blind + rand_int(100) + 100);
            ident=TRUE;
         }
         if (!p_ptr->resist_conf)
         {
            if (set_confused(Ind, p_ptr->confused + rand_int(20) + 15))
            ident=TRUE;
         }
         break;
      /* Aggravation Trap */
      case TRAP_OF_AGGRAVATION:
         msg_print(Ind, "You hear a hollow noise echoing through the dungeons.");
         aggravate_monsters(Ind, 1);
         break;
      /* Multiplication Trap */
      case TRAP_OF_MULTIPLICATION:
      {
         msg_print(Ind, "You hear a loud click.");
         for(k = -1; k <= 1; k++)
         for(l = -1; l <= 1; l++)
         {
			cave_type *c2_ptr=&zcave[y + l][x + k];
//                if(in_bounds(y + l, px + k) && !cave[y + l][px + k].t_idx)
                if(in_bounds(y + l, x + k) && (zcave[y + l][x + k].special.type != CS_TRAPS))
                {
                        place_trap(&p_ptr->wpos, y + l, x + k);
                }
         }
         ident = TRUE;
         break;
      }
      /* Steal Item Trap */
      case TRAP_OF_STEAL_ITEM:
         {
            /* please note that magical stealing is not so easily circumvented */
            if (!p_ptr->paralyzed &&
                (rand_int(160) < (adj_dex_safe[p_ptr->stat_ind[A_DEX]] +
                                  p_ptr->lev)))
            {
               /* Saving throw message */
               msg_print(Ind, "Your backpack seems to vibrate strangely!");
               break;
            }
            /* Find an item */
            for (k = 0; k < rand_int(10); k++)
            {
               char i_name[80];

               /* Pick an item */
               s16b i = rand_int(INVEN_PACK);

               /* Obtain the item */
               object_type *j_ptr = &p_ptr->inventory[i], *q_ptr, forge;

               /* Accept real items */
               if (!j_ptr->k_idx) continue;

               /* Don't steal artifacts  -CFT */
               if (artifact_p(j_ptr)) continue;

               /* Get a description */
               object_desc(Ind, i_name, j_ptr, FALSE, 3);

               /* Message */
               msg_format(Ind, "%sour %s (%c) was stolen!",
                          ((j_ptr->number > 1) ? "One of y" : "Y"),
                          i_name, index_to_label(i));

               /* Create the item */
	       q_ptr = &forge;
	       object_copy(q_ptr, j_ptr);
	       q_ptr->number = 1;

	       /* Drop it somewhere */
               do_trap_teleport_away(Ind, q_ptr, y, x);

	       inven_item_increase(Ind, i,-1);
               inven_item_optimize(Ind, i);
               ident=TRUE;
            }
            break;
         }
      /* Summon Fast Quylthulgs Trap */
      case TRAP_OF_SUMMON_FAST_QUYLTHULGS:
         for (k = 0; k < randint(3); k++)
         {
            ident |= summon_specific(&p_ptr->wpos, y, x, getlevel(&p_ptr->wpos), SUMMON_QUYLTHULG);
         }
         if (ident)
         {
            msg_print(Ind, "You suddenly have company.");
            (void)set_slow(Ind, p_ptr->slow + randint(25) + 15);
         }
         break;
      /* Trap of Sinking */
      case TRAP_OF_SINKING:
		 /* MEGAHACK: Ignore Wilderness trap doors. */
#ifndef NEW_DUNGEON
		 if( p_ptr->dun_depth<0) {
			 msg_print(Ind, "\377GYou feel quite certain something really awful just happened..");
			 break;
		 }
#else
		 if(!can_go_down(&p_ptr->wpos)){
			 msg_print(Ind, "\377GYou feel quite certain something really awful just happened..");
			 break;
		 }
#endif
		 ident = TRUE;
		 msg_print(Ind, "You fell through a trap door!");
		 if (p_ptr->feather_fall)
		 {
			 msg_print(Ind, "You float gently down to the next level.");
		 }
		 else
		 {
//			 int dam = damroll(2, 8);
//			 take_hit(Ind, dam, name);
			 take_hit(Ind, damroll(2, 8), "trap door");
		 }
		 p_ptr->new_level_flag = TRUE;
		 p_ptr->new_level_method = LEVEL_RAND;

		 /* The player is gone */
#ifdef NEW_DUNGEON
		 c_ptr->m_idx=0;
#else
		 cave[p_ptr->dun_depth][p_ptr->py][p_ptr->px].m_idx = 0;
#endif
		 /* Erase his light */
		 forget_lite(Ind);

		 /* Show everyone that he's left */
#ifdef NEW_DUNGEON
		 everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);
		 new_players_on_depth(&p_ptr->wpos,-1,TRUE);
		 p_ptr->wpos.wz--;
		 new_players_on_depth(&p_ptr->wpos,1,TRUE);
#else
		 everyone_lite_spot(p_ptr->dun_depth, p_ptr->py, p_ptr->px);

		 /* Reduce the number of players on this depth */
		 players_on_depth[p_ptr->dun_depth]--;

		 p_ptr->dun_depth++;

		 /* Increase the number of players on this next depth */
		 players_on_depth[p_ptr->dun_depth]++;
#endif
		 break;
      /* Trap of Mana Drain */
      case TRAP_OF_MANA_DRAIN:
         if (p_ptr->csp>0)
         {
            p_ptr->csp = 0;
            p_ptr->csp_frac = 0;
            p_ptr->redraw |= (PR_MANA);
            msg_print(Ind, "You sense a great loss.");
            ident=TRUE;
         }
         else
         {
            if (p_ptr->msp==0) /* no sense saying this unless you never have mana */
            {
               msg_format(Ind, "Suddenly you feel glad you're only a %s",p_ptr->cp_ptr->title);
            }
            else
            {
               msg_print(Ind, "Your head feels dizzy for a moment.");
            }
         }
         break;
      /* Trap of Missing Money */
      case TRAP_OF_MISSING_MONEY:
         {
            u32b gold = (p_ptr->au / 10) + randint(25);

            if (gold < 2) gold = 2;
            if (gold > 5000) gold = (p_ptr->au / 20) + randint(3000);
            if (gold > p_ptr->au) gold = p_ptr->au;
            p_ptr->au -= gold;
            if (gold <= 0)
            {
                msg_print(Ind, "You feel something touching you.");
            }
            else if (p_ptr->au)
            {
                msg_print(Ind, "Your purse feels lighter.");
                msg_format(Ind, "%ld coins were stolen!", (long)gold);
                ident=TRUE;
            }
            else
            {
                msg_print(Ind, "Your purse feels empty.");
                msg_print(Ind, "All of your coins were stolen!");
                ident=TRUE;
            }
            p_ptr->redraw |= (PR_GOLD);
         }
         break;
      /* Trap of No Return */
      case TRAP_OF_NO_RETURN:
         {
            object_type *j_ptr;
            s16b j;

            for (j=0;j<INVEN_WIELD;j++)
            {
               if (!p_ptr->inventory[j].k_idx) continue;
               j_ptr = &p_ptr->inventory[j];
               if ((j_ptr->tval==TV_SCROLL)
                    && (j_ptr->sval==SV_SCROLL_WORD_OF_RECALL))
               {
		  inven_item_increase(Ind, j, -j_ptr->number);
                  inven_item_optimize(Ind, j);
                  combine_pack(Ind);
                  reorder_pack(Ind);
                  if (!ident)
                     msg_print(Ind, "A small fire works it's way through your backpack. Some scrolls are burnt.");
                  else
                     msg_print(Ind, "The fire hasn't finished.");
                  ident=TRUE;
               }
//               else if ((j_ptr->tval==TV_ROD_MAIN) && (j_ptr->pval == SV_ROD_RECALL))
               else if ((j_ptr->tval==TV_ROD) && (j_ptr->pval == SV_ROD_RECALL))
               {
                  j_ptr->timeout = 0; /* a long time */
                  if (!ident) msg_print(Ind, "You feel the air stabilize around you.");
                  ident=TRUE;
               }
            }
            if ((!ident) && (p_ptr->word_recall))
            {
               msg_print(Ind, "You feel like staying around.");
               p_ptr->word_recall = 0;
               ident=TRUE;
            }
         }
         break;
      /* Trap of Silent Switching */
      case TRAP_OF_SILENT_SWITCHING:
         {
            s16b i,j,slot1,slot2;
            object_type *j_ptr, *k_ptr;
            for (i=INVEN_WIELD;i<INVEN_TOTAL;i++)
            {
               j_ptr = &p_ptr->inventory[i];
               if (!j_ptr->k_idx) continue;
               slot1=wield_slot(Ind, j_ptr);
               for (j=0;j<INVEN_WIELD;j++)
               {
                  k_ptr = &p_ptr->inventory[j];
                  if (!k_ptr->k_idx) continue;
                  /* this is a crude hack, but it prevent wielding 6 torches... */
                  if (k_ptr->number > 1) continue;
                  slot2=wield_slot(Ind, k_ptr);
                  /* a chance of 4 in 5 of switching something, then 2 in 5 to do it again */
                  if ((slot1==slot2) && (rand_int(100)<(80-ident*40)))
                  {
                     object_type tmp_obj;
                     tmp_obj = p_ptr->inventory[j];
                     p_ptr->inventory[j] = p_ptr->inventory[i];
                     p_ptr->inventory[i] = tmp_obj;
                     ident=TRUE;
                  }
               }
            }
            if (ident)
            {
               p_ptr->update |= (PU_BONUS);
               p_ptr->update |= (PU_TORCH);
               p_ptr->update |= (PU_MANA);
               msg_print(Ind, "You somehow feel an other person.");
            }
            else
            {
               msg_print(Ind, "You feel a lack of useful items.");
            }
         }
         break;
      /* Trap of Walls */
      case TRAP_OF_WALLS:
//         ident = player_handle_trap_of_walls();
         break;
      /* Trap of Calling Out */
      case TRAP_OF_CALLING_OUT:
         {
            ident=do_player_trap_call_out(Ind);
            if (!ident)
            {
               /* Increase "afraid" */
               if (p_ptr->resist_fear)
               {
                   msg_print(Ind, "You feel as if you had a nightmare!");
               }
               else if (rand_int(100) < p_ptr->skill_sav)
               {
                   msg_print(Ind, "You remember having a nightmare!");
               }
               else
               {
                  if (set_afraid(Ind, p_ptr->afraid + 3 + randint(40)))
                  {
                     msg_print(Ind, "You have a vision of a powerful enemy.");
                  }
               }
            }
         }
         break;
      /* Trap of Sliding */
      case TRAP_OF_SLIDING:
		 /* ? */
         break;
      /* Trap of Charges Drain */
      case TRAP_OF_CHARGES_DRAIN:
         {
            s16b         i;
            object_type *j_ptr;
            /* Find an item */
            for (k = 0; k < 10; k++)
            {
               i = rand_int(INVEN_PACK);
               j_ptr = &p_ptr->inventory[i];
               /* Drain charged wands/staffs */
               if (((j_ptr->tval == TV_STAFF) || (j_ptr->tval == TV_WAND)) &&
                   (j_ptr->pval))
               {
                  ident = TRUE;
                  j_ptr->pval = j_ptr->pval / (randint(4)+1);
                  /* Window stuff */
                  p_ptr->window |= PW_INVEN;
                  /* Combine / Reorder the pack */
                  p_ptr->notice |= (PN_COMBINE | PN_REORDER);
                  if (randint(10)>3) break; /* 60% chance of only 1 */
               }
            }
            if (ident)
              msg_print(Ind, "Your backpack seems to be turned upside down.");
            else
              msg_print(Ind, "You hear a wail of great disappointment.");
         }
         break;
      /* Trap of Stair Movement */
      case TRAP_OF_STAIR_MOVEMENT:
#if 0	// after f_info!
         {
            s16b cx,cy,i,j;
            s16b cnt = 0;
            s16b cnt_seen = 0;
            s16b tmps, tmpx;
            u32b tmpf;
            bool seen = FALSE;
            s16b index_x[20],index_y[20]; /* 20 stairs per level is enough? */
            cave_type *cv_ptr;
            if (max_dlv[dungeon_type]!=99) /* no sense in relocating that stair! */
            {
               for (cx=0;cx<p_ptr->cur_wid;cx++)
               {
                  for (cy=0;cy<p_ptr->cur_hgt;cy++)
                  {
                     cv_ptr = &zcave[cy][cx];
#if 0
                     if ((cv_ptr->feat != FEAT_LESS) &&
			 (cv_ptr->feat != FEAT_MORE) &&
			 (cv_ptr->feat != FEAT_SHAFT_UP) &&
			 (cv_ptr->feat != FEAT_SHAFT_DOWN)) continue;
#endif
                     if ((cv_ptr->feat != FEAT_LESS) &&
			 (cv_ptr->feat != FEAT_MORE)) continue;

                     index_x[cnt]=cx;
                     index_y[cnt]=cy;
                     cnt++;
                  }
               }
               if (cnt==0)
               {
//                  quit("Executing moving stairs trap on level with no stairs!");
                  s_printf("Executing moving stairs trap on level with no stairs!\n");
				  break;
               }
               else
               {
                  for (i=0;i<cnt;i++)
                  {
                     for (j=0;j<10;j++) /* try 10 times to relocate */
                     {
                        cave_type *c_ptr2 = &zcave[index_y[i]][index_x[i]];

                        cx=rand_int(p_ptr->cur_wid);
                        cy=rand_int(p_ptr->cur_hgt);

                        if ((cx==index_x[i]) || (cy==index_y[i])) continue;
                        if (!cave_valid_bold(zcave, cy, cx) || zcave[cy][cx].o_idx!=0) continue;

                        /* don't put anything in vaults */
                        if (zcave[cy][cx].info & CAVE_ICKY) continue;

                        tmpf = zcave[cy][cx].feat;
                        tmps = zcave[cy][cx].info;
                        tmpx = zcave[cy][cx].mimic;
                        zcave[cy][cx].feat = c_ptr2->feat;
                        zcave[cy][cx].info = c_ptr2->info;
                        zcave[cy][cx].mimic = c_ptr2->mimic;
                        c_ptr2->feat  = tmpf;
                        c_ptr2->info  = tmps;
                        c_ptr2->mimic = tmpx;

                        /* if we are placing walls in rooms, make them rubble instead */
                        if ((c_ptr2->info & CAVE_ROOM) &&
			    (c_ptr2->feat >= FEAT_WALL_EXTRA) &&
			    (c_ptr2->feat <= FEAT_PERM_SOLID))
                        {
			   cave[index_y[i]][index_x[i]].feat = FEAT_RUBBLE;
                        }
                        if (player_has_los_bold(Ind, cy, cx))
                        {
                           note_spot(Ind, cy,cx);
                           lite_spot(Ind, cy,cx);
                           seen=TRUE;
                        }
                        else
                        {
                           cave[cy][cx].info &=~CAVE_MARK;
                        }
                        if (player_has_los_bold(index_y[i],index_x[i]))
                        {
                           note_spot(index_y[i],index_x[i]);
                           lite_spot(index_y[i],index_x[i]);
                           seen = TRUE;
                        }
                        else
                        {
                           cave[index_y[i]][index_x[i]].info &=~CAVE_MARK;
                        }
                        break;
                     }
                     if (seen) cnt_seen++;
                     seen = FALSE;
                  } /* cnt loop */
                  ident = (cnt_seen>0);
                  if ((ident) && (cnt_seen>1))
                  {
                     msg_print(Ind, "You see some stairs move.");
                  }
                  else if (ident)
                  {
                     msg_print(Ind, "You see a stair move.");
                  }
                  else
                  {
                     msg_print(Ind, "You hear distant scraping noises.");
                  }
		  p_ptr->redraw |= PR_MAP;
               } /* any stairs found */
            }
            else /* are we on level 99 */
            {
               msg_print(Ind, "You have a feeling that this trap could be dangerous.");
            }
         }
#endif	// 0
         break;
      /* Trap of New Trap */
      case TRAP_OF_NEW:
         {
            /* if we're on a floor or on a door, place a new trap */
            if ((item == -1) || (item == -2))
            {
				delete_trap_idx(tt_ptr);
				place_trap(&p_ptr->wpos, y , x);
                  if (player_has_los_bold(Ind, y, x))
                  {
                     note_spot(Ind, y, x);
                     lite_spot(Ind, y, x);
                  }
               }
            else
            {
               /* re-trap the chest */
//				place_trap(&p_ptr->wpos, y , x);
				 place_trap_object(i_ptr);
            }
            msg_print(Ind, "You hear a noise, and then it's echo.");
            ident=FALSE;
         }
         break;

                /* Trap of Acquirement */
                case TRAP_OF_ACQUIREMENT:
		 {
			 /* Get a nice thing */
			 msg_print(Ind, "You notice something falling off the trap.");
			 acquirement(&p_ptr->wpos, y, x, 1, TRUE);
//			 acquirement(&p_ptr->wpos, y, x, 1, TRUE, FALSE);	// last is 'known' flag

			 delete_trap_idx(tt_ptr);
			 /* if we're on a floor or on a door, place a new trap */
			 if ((item == -1) || (item == -2))
			 {
				 place_trap(&p_ptr->wpos, y , x);
				 if (player_has_los_bold(Ind, y, x))
				 {
					 note_spot(Ind, y, x);
					 lite_spot(Ind, y, x);
				 }
			 }
			 else
			 {
				 /* re-trap the chest (??) */
//				 place_trap(&p_ptr->wpos, y , x);
				 place_trap_object(i_ptr);
			 }
			 msg_print(Ind, "You hear a noise, and then it's echo.");
			 /* Never known */
			 ident = FALSE;
		 }
                break;

      /* Trap of Scatter Items */
      case TRAP_OF_SCATTER_ITEMS:
				ident = do_player_scatter_items(Ind, 70);
         break;
      /* Trap of Decay */
      case TRAP_OF_DECAY:
         break;
      /* Trap of Wasting Wands */
      case TRAP_OF_WASTING_WANDS:
         {
            s16b i;
            object_type *j_ptr;
            for (i=0;i<INVEN_PACK;i++)
            {
               if (!p_ptr->inventory[i].k_idx) continue;
               j_ptr = &p_ptr->inventory[i];
               if (j_ptr->tval==TV_WAND)
               {
                  if ((j_ptr->sval>=SV_WAND_NASTY_WAND) && (rand_int(5)==1))
                  {
                     if (object_known_p(Ind, j_ptr)) ident=TRUE;
                     j_ptr->sval = rand_int(SV_WAND_NASTY_WAND);
                     j_ptr->k_idx = lookup_kind(j_ptr->tval, j_ptr->sval);
                     p_ptr->notice |= (PN_COMBINE | PN_REORDER);
                  }
                  if ((j_ptr->sval>=SV_STAFF_NASTY_STAFF) && (rand_int(5)==1))
                  {
                     if (object_known_p(Ind, j_ptr)) ident=TRUE;
                     j_ptr->sval = rand_int(SV_STAFF_NASTY_STAFF);
                     j_ptr->k_idx = lookup_kind(j_ptr->tval, j_ptr->sval);
                     p_ptr->notice |= (PN_COMBINE | PN_REORDER);
                  }
               }
            }
         }
         if (ident)
         {
            msg_print(Ind, "You have lost trust in your backpack!");
         }
         else
         {
            msg_print(Ind, "You hear an echoing cry of rage.");
         }
         break;
      /* Trap of Filling */
      case TRAP_OF_FILLING:
         {
            s16b nx, ny;
            for (nx=x-8;nx<=x+8;nx++)
            {
               for (ny=y-8;ny<=y+8;ny++)
               {
                  if (!in_bounds (ny, nx)) continue;

                  if (rand_int(distance(ny,nx,y,x))>3)
                  {
                     place_trap(&p_ptr->wpos, ny,nx);
                  }
               }
            }
            msg_print(Ind, "The floor vibrates in a strange way.");
            ident = FALSE;
         }
         break;
      case TRAP_OF_DRAIN_SPEED:
         {
            object_type *j_ptr;
            s16b j, chance = 75;
            u32b f1, f2, f3, f4, f5, esp;

//            for (j=0;j<INVEN_TOTAL;j++)
			/* From the foot ;D */
            for (j = INVEN_TOTAL - 1; j >= 0 ; j--)
            {
               /* don't bother the overflow slot */
               if (j==INVEN_PACK) continue;

               if (!p_ptr->inventory[j].k_idx) continue;
               j_ptr = &p_ptr->inventory[j];
               object_flags(j_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
//               object_flags(j_ptr, &f1, &f2, &f3);

               /* is it a non-artifact speed item? */
               if ((!j_ptr->name1) && (f1 & TR1_SPEED) && (j_ptr->pval > 0))
               {
                  if (randint(100)<chance)
                  {
                     j_ptr->pval = j_ptr->pval / 2;
                     if (j_ptr->pval == 0)
                     {
                        j_ptr->pval--;
                     }
                     chance /= 2;
                     ident = TRUE;
                  }
                  inven_item_optimize(Ind, j);
               }
            }
            if (!ident)
               msg_print(Ind, "You feel some things in your pack vibrating.");
            else
            {
               combine_pack(Ind);
               reorder_pack(Ind);
               msg_print(Ind, "You suddenly feel you have time for self-reflection.");
               /* Recalculate bonuses */
               p_ptr->update |= (PU_BONUS);

               /* Recalculate mana */
               p_ptr->update |= (PU_MANA);

               /* Window stuff */
               p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
            }
         }
         break;

      /*
       * single missile traps
       */

      case TRAP_OF_ARROW_I:
         ident = player_handle_missile_trap(Ind, 1, TV_ARROW, SV_AMMO_NORMAL, 4, 8, 0, "Arrow Trap"); break;
      case TRAP_OF_ARROW_II:
         ident = player_handle_missile_trap(Ind, 1, TV_BOLT, SV_AMMO_NORMAL, 10, 8, 0, "Bolt Trap"); break;
      case TRAP_OF_ARROW_III:
         ident = player_handle_missile_trap(Ind, 1, TV_ARROW, SV_AMMO_HEAVY, 12, 12, 0, "Seeker Arrow Trap"); break;
      case TRAP_OF_ARROW_IV:
         ident = player_handle_missile_trap(Ind, 1, TV_BOLT, SV_AMMO_HEAVY, 12, 16, 0, "Seeker Bolt Trap"); break;
      case TRAP_OF_POISON_ARROW_I:
         ident = player_handle_missile_trap(Ind, 1, TV_ARROW, SV_AMMO_NORMAL, 4, 8, 10+randint(20), "Poison Arrow Trap"); break;
      case TRAP_OF_POISON_ARROW_II:
         ident = player_handle_missile_trap(Ind, 1, TV_BOLT, SV_AMMO_NORMAL, 10, 8, 15+randint(30), "Poison Bolt Trap"); break;
      case TRAP_OF_POISON_ARROW_III:
         ident = player_handle_missile_trap(Ind, 1, TV_ARROW, SV_AMMO_HEAVY, 12, 12, 30+randint(50), "Poison Seeker Arrow Trap"); break;
      case TRAP_OF_POISON_ARROW_IV:
         ident = player_handle_missile_trap(Ind, 1, TV_BOLT, SV_AMMO_HEAVY, 12, 16, 40+randint(70), "Poison Seeker Bolt Trap"); break;
      case TRAP_OF_DAGGER_I:
         ident = player_handle_missile_trap(Ind, 1, TV_SWORD, SV_BROKEN_DAGGER, 4, 8, 0, "Dagger Trap"); break;
      case TRAP_OF_DAGGER_II:
         ident = player_handle_missile_trap(Ind, 1, TV_SWORD, SV_DAGGER, 10, 8, 0, "Dagger Trap"); break;
      case TRAP_OF_POISON_DAGGER_I:
         ident = player_handle_missile_trap(Ind, 1, TV_SWORD, SV_BROKEN_DAGGER, 4, 8, 15+randint(20), "Poison Dagger Trap"); break;
      case TRAP_OF_POISON_DAGGER_II:
         ident = player_handle_missile_trap(Ind, 1, TV_SWORD, SV_DAGGER, 10, 8, 20+randint(30), "Poison Dagger Trap"); break;

      /*
       * multiple missile traps
       * numbers range from 2 (level 0 to 14) to 10 (level 120 and up)
       */

      case TRAP_OF_ARROWS_I:
         ident = player_handle_missile_trap(Ind, 2+(getlevel(&p_ptr->wpos) / 15), TV_ARROW, SV_AMMO_NORMAL, 4, 8, 0, "Arrow Trap"); break;
      case TRAP_OF_ARROWS_II:
         ident = player_handle_missile_trap(Ind, 2+(getlevel(&p_ptr->wpos) / 15), TV_BOLT, SV_AMMO_NORMAL, 10, 8, 0, "Bolt Trap"); break;
      case TRAP_OF_ARROWS_III:
         ident = player_handle_missile_trap(Ind, 2+(getlevel(&p_ptr->wpos) / 15), TV_ARROW, SV_AMMO_HEAVY, 12, 12, 0, "Seeker Arrow Trap"); break;
      case TRAP_OF_ARROWS_IV:
         ident = player_handle_missile_trap(Ind, 2+(getlevel(&p_ptr->wpos) / 15), TV_BOLT, SV_AMMO_HEAVY, 12, 16, 0, "Seeker Bolt Trap"); break;
      case TRAP_OF_POISON_ARROWS_I:
         ident = player_handle_missile_trap(Ind, 2+(getlevel(&p_ptr->wpos) / 15), TV_ARROW, SV_AMMO_NORMAL, 4, 8, 10+randint(20), "Poison Arrow Trap"); break;
      case TRAP_OF_POISON_ARROWS_II:
         ident = player_handle_missile_trap(Ind, 2+(getlevel(&p_ptr->wpos) / 15), TV_BOLT, SV_AMMO_NORMAL, 10, 8, 15+randint(30), "Poison Bolt Trap"); break;
      case TRAP_OF_POISON_ARROWS_III:
         ident = player_handle_missile_trap(Ind, 2+(getlevel(&p_ptr->wpos) / 15), TV_ARROW, SV_AMMO_HEAVY, 12, 12, 30+randint(50), "Poison Seeker Arrow Trap"); break;
      case TRAP_OF_POISON_ARROWS_IV:
         ident = player_handle_missile_trap(Ind, 2+(getlevel(&p_ptr->wpos) / 15), TV_BOLT, SV_AMMO_HEAVY, 12, 16, 40+randint(70), "Poison Seeker Bolt Trap"); break;
      case TRAP_OF_DAGGERS_I:
         ident = player_handle_missile_trap(Ind, 2+(getlevel(&p_ptr->wpos) / 15), TV_SWORD, SV_BROKEN_DAGGER, 4, 8, 0, "Dagger Trap"); break;
      case TRAP_OF_DAGGERS_II:
         ident = player_handle_missile_trap(Ind, 2+(getlevel(&p_ptr->wpos) / 15), TV_SWORD, SV_DAGGER, 10, 8, 0, "Dagger Trap"); break;
      case TRAP_OF_POISON_DAGGERS_I:
         ident = player_handle_missile_trap(Ind, 2+(getlevel(&p_ptr->wpos) / 15), TV_SWORD, SV_BROKEN_DAGGER, 4, 8, 15+randint(20), "Poison Dagger Trap"); break;
      case TRAP_OF_POISON_DAGGERS_II:
         ident = player_handle_missile_trap(Ind, 2+(getlevel(&p_ptr->wpos) / 15), TV_SWORD, SV_DAGGER, 10, 8, 20+randint(30), "Poison Dagger Trap"); break;

		 /* it was '20,90,70'... */
      case TRAP_OF_DROP_ITEMS:
		 ident = do_player_drop_items(Ind, 20);
         break;
      case TRAP_OF_DROP_ALL_ITEMS:
		 ident = do_player_drop_items(Ind, 70);
         break;
      case TRAP_OF_DROP_EVERYTHING:
		 ident = do_player_drop_items(Ind, 90);
         break;

      /* Bolt Trap */
      case TRAP_OF_ELEC_BOLT:       ident=player_handle_breath_trap(Ind, 1, GF_ELEC, TRAP_OF_ELEC_BOLT); break;
      case TRAP_OF_POIS_BOLT:       ident=player_handle_breath_trap(Ind, 1, GF_POIS, TRAP_OF_POIS_BOLT); break;
      case TRAP_OF_ACID_BOLT:       ident=player_handle_breath_trap(Ind, 1, GF_ACID, TRAP_OF_ACID_BOLT); break;
      case TRAP_OF_COLD_BOLT:       ident=player_handle_breath_trap(Ind, 1, GF_COLD, TRAP_OF_COLD_BOLT); break;
      case TRAP_OF_FIRE_BOLT:       ident=player_handle_breath_trap(Ind, 1, GF_FIRE, TRAP_OF_FIRE_BOLT); break;
      case TRAP_OF_PLASMA_BOLT:     ident=player_handle_breath_trap(Ind, 1, GF_PLASMA, TRAP_OF_PLASMA_BOLT); break;
      case TRAP_OF_WATER_BOLT:      ident=player_handle_breath_trap(Ind, 1, GF_WATER, TRAP_OF_WATER_BOLT); break;
      case TRAP_OF_LITE_BOLT:       ident=player_handle_breath_trap(Ind, 1, GF_LITE, TRAP_OF_LITE_BOLT); break;
      case TRAP_OF_DARK_BOLT:       ident=player_handle_breath_trap(Ind, 1, GF_DARK, TRAP_OF_DARK_BOLT); break;
      case TRAP_OF_SHARDS_BOLT:     ident=player_handle_breath_trap(Ind, 1, GF_SHARDS, TRAP_OF_SHARDS_BOLT); break;
      case TRAP_OF_SOUND_BOLT:      ident=player_handle_breath_trap(Ind, 1, GF_SOUND, TRAP_OF_SOUND_BOLT); break;
      case TRAP_OF_CONFUSION_BOLT:  ident=player_handle_breath_trap(Ind, 1, GF_CONFUSION, TRAP_OF_CONFUSION_BOLT); break;
      case TRAP_OF_FORCE_BOLT:      ident=player_handle_breath_trap(Ind, 1, GF_FORCE, TRAP_OF_FORCE_BOLT); break;
      case TRAP_OF_INERTIA_BOLT:    ident=player_handle_breath_trap(Ind, 1, GF_INERTIA, TRAP_OF_INERTIA_BOLT); break;
      case TRAP_OF_MANA_BOLT:       ident=player_handle_breath_trap(Ind, 1, GF_MANA, TRAP_OF_MANA_BOLT); break;
      case TRAP_OF_ICE_BOLT:        ident=player_handle_breath_trap(Ind, 1, GF_ICE, TRAP_OF_ICE_BOLT); break;
      case TRAP_OF_CHAOS_BOLT:      ident=player_handle_breath_trap(Ind, 1, GF_CHAOS, TRAP_OF_CHAOS_BOLT); break;
      case TRAP_OF_NETHER_BOLT:     ident=player_handle_breath_trap(Ind, 1, GF_NETHER, TRAP_OF_NETHER_BOLT); break;
      case TRAP_OF_DISENCHANT_BOLT: ident=player_handle_breath_trap(Ind, 1, GF_DISENCHANT, TRAP_OF_DISENCHANT_BOLT); break;
      case TRAP_OF_NEXUS_BOLT:      ident=player_handle_breath_trap(Ind, 1, GF_NEXUS, TRAP_OF_NEXUS_BOLT); break;
      case TRAP_OF_TIME_BOLT:       ident=player_handle_breath_trap(Ind, 1, GF_TIME, TRAP_OF_TIME_BOLT); break;
      case TRAP_OF_GRAVITY_BOLT:    ident=player_handle_breath_trap(Ind, 1, GF_GRAVITY, TRAP_OF_GRAVITY_BOLT); break;

      /* Ball Trap */
      case TRAP_OF_ELEC_BALL:       ident=player_handle_breath_trap(Ind, 3, GF_ELEC, TRAP_OF_ELEC_BALL); break;
      case TRAP_OF_POIS_BALL:       ident=player_handle_breath_trap(Ind, 3, GF_POIS, TRAP_OF_POIS_BALL); break;
      case TRAP_OF_ACID_BALL:       ident=player_handle_breath_trap(Ind, 3, GF_ACID, TRAP_OF_ACID_BALL); break;
      case TRAP_OF_COLD_BALL:       ident=player_handle_breath_trap(Ind, 3, GF_COLD, TRAP_OF_COLD_BALL); break;
      case TRAP_OF_FIRE_BALL:       ident=player_handle_breath_trap(Ind, 3, GF_FIRE, TRAP_OF_FIRE_BALL); break;
      case TRAP_OF_PLASMA_BALL:     ident=player_handle_breath_trap(Ind, 3, GF_PLASMA, TRAP_OF_PLASMA_BALL); break;
      case TRAP_OF_WATER_BALL:      ident=player_handle_breath_trap(Ind, 3, GF_WATER, TRAP_OF_WATER_BALL); break;
      case TRAP_OF_LITE_BALL:       ident=player_handle_breath_trap(Ind, 3, GF_LITE, TRAP_OF_LITE_BALL); break;
      case TRAP_OF_DARK_BALL:       ident=player_handle_breath_trap(Ind, 3, GF_DARK, TRAP_OF_DARK_BALL); break;
      case TRAP_OF_SHARDS_BALL:     ident=player_handle_breath_trap(Ind, 3, GF_SHARDS, TRAP_OF_SHARDS_BALL); break;
      case TRAP_OF_SOUND_BALL:      ident=player_handle_breath_trap(Ind, 3, GF_SOUND, TRAP_OF_SOUND_BALL); break;
      case TRAP_OF_CONFUSION_BALL:  ident=player_handle_breath_trap(Ind, 3, GF_CONFUSION, TRAP_OF_CONFUSION_BALL); break;
      case TRAP_OF_FORCE_BALL:      ident=player_handle_breath_trap(Ind, 3, GF_FORCE, TRAP_OF_FORCE_BALL); break;
      case TRAP_OF_INERTIA_BALL:    ident=player_handle_breath_trap(Ind, 3, GF_INERTIA, TRAP_OF_INERTIA_BALL); break;
      case TRAP_OF_MANA_BALL:       ident=player_handle_breath_trap(Ind, 3, GF_MANA, TRAP_OF_MANA_BALL); break;
      case TRAP_OF_ICE_BALL:        ident=player_handle_breath_trap(Ind, 3, GF_ICE, TRAP_OF_ICE_BALL); break;
      case TRAP_OF_CHAOS_BALL:      ident=player_handle_breath_trap(Ind, 3, GF_CHAOS, TRAP_OF_CHAOS_BALL); break;
      case TRAP_OF_NETHER_BALL:     ident=player_handle_breath_trap(Ind, 3, GF_NETHER, TRAP_OF_NETHER_BALL); break;
      case TRAP_OF_DISENCHANT_BALL: ident=player_handle_breath_trap(Ind, 3, GF_DISENCHANT, TRAP_OF_DISENCHANT_BALL); break;
      case TRAP_OF_NEXUS_BALL:      ident=player_handle_breath_trap(Ind, 3, GF_NEXUS, TRAP_OF_NEXUS_BALL); break;
      case TRAP_OF_TIME_BALL:       ident=player_handle_breath_trap(Ind, 3, GF_TIME, TRAP_OF_TIME_BALL); break;
      case TRAP_OF_GRAVITY_BALL:    ident=player_handle_breath_trap(Ind, 3, GF_GRAVITY, TRAP_OF_GRAVITY_BALL); break;

      /* -SC- */
      case TRAP_OF_FEMINITY:
      {
         msg_print(Ind, "Gas sprouts out... you feel you transmute.");
	 trap_hit(Ind, trap);
	 if (!p_ptr->male) break;
//	 p_ptr->psex = SEX_FEMALE;
//	 sp_ptr = &sex_info[p_ptr->psex];
	 p_ptr->male = FALSE;
	 ident = TRUE;
	 break;
      }
      case TRAP_OF_MASCULINITY:
      {
         msg_print(Ind, "Gas sprouts out... you feel you transmute.");
	 trap_hit(Ind, trap);
	 if (p_ptr->male) break;
//	 p_ptr->psex = SEX_MALE;
//	 sp_ptr = &sex_info[p_ptr->psex];
	 p_ptr->male = FALSE;
	 ident = TRUE;
	 break;
      }
      case TRAP_OF_NEUTRALITY:
      {
#if 0
         msg_print(Ind, "Gas sprouts out... you feel you transmute.");
//	 p_ptr->psex = SEX_NEUTER;
//	 sp_ptr = &sex_info[p_ptr->psex];
	 p_ptr->male = FALSE;
	 ident = TRUE;
	 trap_hit(Ind, trap);
#endif
	 break;
      }
      case TRAP_OF_AGING:
      {
         msg_print(Ind, "Colors are scintillating around you, you see your past running before your eyes.");
//         p_ptr->age += randint((rp_ptr->b_age + rmp_ptr->b_age) / 2);
         p_ptr->age += randint((p_ptr->rp_ptr->b_age));
	 ident = TRUE;
	 trap_hit(Ind, trap);
	 break;
      }
      case TRAP_OF_GROWING:
      {
	 s16b tmp;

         msg_print(Ind, "Heavy fumes sprout out... you feel you transmute.");
//         if (p_ptr->psex == SEX_FEMALE) tmp = rp_ptr->f_b_ht + rmp_ptr->f_b_ht;
//         else tmp = rp_ptr->m_b_ht + rmp_ptr->m_b_ht;

         if (!p_ptr->male) tmp = p_ptr->rp_ptr->f_b_ht;
         else tmp = p_ptr->rp_ptr->m_b_ht;

	 p_ptr->ht += randint(tmp/4);
	 ident = TRUE;
	 trap_hit(Ind, trap);
	 break;
      }
      case TRAP_OF_SHRINKING:
      {
	 s16b tmp;

         msg_print(Ind, "Heavy fumes sprout out... you feel you transmute.");
//         if (p_ptr->psex == SEX_FEMALE) tmp = rp_ptr->f_b_ht + rmp_ptr->f_b_ht;
//         else tmp = rp_ptr->m_b_ht + rmp_ptr->m_b_ht;
         if (!p_ptr->male) tmp = p_ptr->rp_ptr->f_b_ht;
         else tmp = p_ptr->rp_ptr->m_b_ht;

	 p_ptr->ht -= randint(tmp/4);
	 if (p_ptr->ht <= tmp/4) p_ptr->ht = tmp/4;
	 ident = TRUE;
	 trap_hit(Ind, trap);
	 break;
      }

      /* Trap of Tanker Drain */
      case TRAP_OF_TANKER_DRAIN:
#if 0	// ?_?
         if (p_ptr->ctp>0)
         {
            p_ptr->ctp = 0;
            p_ptr->redraw |= (PR_TANK);
            msg_print(Ind, "You sense a great loss.");
            ident=TRUE;
         }
         else
         {
	    /* no sense saying this unless you never have tanker point */
	    if (p_ptr->mtp==0)
            {
               msg_format(Ind, "Suddenly you feel glad you're only a %s", rp_ptr->title + rp_name);
            }
            else
            {
               msg_print(Ind, "Your head feels dizzy for a moment.");
            }
         }
#endif
         break;

      /* Trap of Divine Anger */
      case TRAP_OF_DIVINE_ANGER:
#if 0	// No hell below us above us only sky o/~
      {
         if (p_ptr->pgod == 0)
	 {
	    msg_format(Ind, "Suddenly you feel glad you're only a %s", cp_ptr->title);
	 }
	 else
	 {
	    cptr name;

	    name=deity_info[p_ptr->pgod-1].name;
	    msg_format(Ind, "You feel you have angered %s.", name);
	    set_grace(p_ptr->grace - 3000);
	 }
      }
#endif	// 0
      break;

      /* Trap of Divine Wrath */
      case TRAP_OF_DIVINE_WRATH:
#if 0	// No hell below us above us only sky o/~
      {
         if (p_ptr->pgod == 0)
	 {
	    msg_format(Ind, "Suddenly you feel glad you're only a %s", cp_ptr->title);
	 }
	 else
	 {
	    cptr name;

	    name=deity_info[p_ptr->pgod-1].name;

	    msg_format(Ind, "%s quakes in rage: ``Thou art supremely insolent, mortal!!''", name);
	    nasty_side_effect();
	    set_grace(p_ptr->grace - 5000);
	 }
      }
#endif
      break;

      /* Trap of hallucination */
      case TRAP_OF_HALLUCINATION:
      {
	 msg_print(Ind, "Scintillating colors hypnotize you for a moment.");

	 set_image(Ind, 80);
      }
      break;

      /* Bolt Trap */
#if 0	// coming..when it comes :)
      case TRAP_OF_ROCKET: ident=player_handle_breath_trap(1, GF_ROCKET, trap); break;
      case TRAP_OF_NUKE_BOLT: ident=player_handle_breath_trap(1, GF_NUKE, trap); break;
      case TRAP_OF_HOLY_FIRE: ident=player_handle_breath_trap(1, GF_HOLY_FIRE, trap); break;
      case TRAP_OF_HELL_FIRE: ident=player_handle_breath_trap(1, GF_HELL_FIRE, trap); break;
#endif	// 0
      case TRAP_OF_PSI_BOLT: ident=player_handle_breath_trap(Ind, 1, GF_PSI, trap); break;
#if 0
      case TRAP_OF_PSI_DRAIN: ident=player_handle_breath_trap(1, GF_PSI_DRAIN, trap); break;

      /* Ball Trap */
      case TRAP_OF_NUKE_BALL: ident=player_handle_breath_trap(3, GF_NUKE, TRAP_OF_NUKE_BALL); break;
      case TRAP_OF_PSI_BALL: ident=player_handle_breath_trap(3, GF_PSI, TRAP_OF_NUKE_BALL); break;
#endif	// 0

	/* PernMangband additions */
      /* Trap? of Ale vendor */
      case TRAP_OF_ALE:
         {
            u32b price = getlevel(&p_ptr->wpos), amt;

			price *= price;
			if (price < 5) price = 5;
			amt = (p_ptr->au / price);

			if (amt < 1)
			{
				msg_print(Ind, "You feel as if it's the day before the payday.");
			}
			else
			{
				object_type *o_ptr, forge;
				o_ptr = &forge;
				object_prep(o_ptr, lookup_kind(TV_FOOD, SV_FOOD_PINT_OF_ALE));

            if (amt > 10) amt = 10;
			amt = randint(amt);

            p_ptr->au -= amt * price;
			o_ptr->number = amt;
			drop_near(o_ptr, -1, &p_ptr->wpos, p_ptr->py, p_ptr->px);

			msg_print(Ind, "You feel like having a shot.");
			ident=TRUE;

            p_ptr->redraw |= (PR_GOLD);
			}
         }
         break;
      /* Trap of Garbage */
      case TRAP_OF_GARBAGE:
		 ident = do_player_trap_garbage(Ind, 300);
		 break;
      /* Trap of discordance */
	  case TRAP_OF_HOSTILITY:
		 {
			 ident = player_handle_trap_of_hostility(Ind);
			 ident = FALSE;		// never known
		 }
      /* Voluminous cuisine Trap */
      case TRAP_OF_CUISINE:
         msg_print(Ind, "You are treated to a marvelous elven cuisine!");
		 /* 1turn = 100 food value when satiated */
         (void)set_food(Ind, PY_FOOD_MAX + getlevel(&p_ptr->wpos)*10 + 1000 + rand_int(1000));
         ident=TRUE;
         break;
      /* Trap of unmagic */
	  case TRAP_OF_UNMAGIC:
		 {
			 ident = unmagic(Ind);
			 break;
		 }
      /* Vermin Trap */
      case TRAP_OF_VERMIN:
		 l = randint(50 + getlevel(&p_ptr->wpos)) + 100;
         for (k = 0; k < l; k++)
         {
			 s16b cx = x+20-rand_int(40);
			 s16b cy = y+20-rand_int(40);

			 if (!in_bounds(cy,cx)) 
			 {
				 cx = x;
				 cy = y;
			 }

			 /* paranoia */
			 if (!cave_floor_bold(zcave, cy, cx))
			 {
				 cx = x;
				 cy = y;
			 }

			 ident |= summon_specific(&p_ptr->wpos, cy, cx, getlevel(&p_ptr->wpos), SUMMON_VERMIN);
         }
         if (ident)
         {
            msg_print(Ind, "You suddenly feel itchy.");
         }
         break;
      /* Trap of amnesia (and not lose_memory) */
	  case TRAP_OF_AMNESIA:
		 {
			 if (rand_int(100) < p_ptr->skill_sav)
			 {
				 msg_print(Ind, "You resist the effects!");
			 }
			 else if (lose_all_info(Ind))
			 {
				 msg_print(Ind, "Your memories fade away.");
//				 ident = TRUE;	// haha you forget this also :)
			 }
			 break;
		 }
	  case TRAP_OF_SILLINESS:
		 {
			 if (rand_int(100) < p_ptr->skill_sav)
			 {
				 msg_print(Ind, "You resist the effects!");
			 }
			 else if (do_trap_of_silliness(Ind, 50 + getlevel(&p_ptr->wpos)))
			 {
				 msg_print(Ind, "You feel somewhat silly.");
//				 ident = TRUE;	// haha you forget this also :)
			 }
			 break;
		 }
      case TRAP_OF_GOODBYE_CHARLIE:
         {
             s16b i,j;
            bool message = FALSE;

			/* Send him/her back to home :) */
			p_ptr->recall_pos.wx = p_ptr->wpos.wx;
			p_ptr->recall_pos.wy = p_ptr->wpos.wy;
			p_ptr->recall_pos.wz = 0;

			if (!p_ptr->word_recall)
			{
				p_ptr->word_recall = rand_int(20) + 15;
				msg_print(Ind, "\377oThe air about you becomes charged...");
				message = TRUE;
			}
			ident |= do_player_scatter_items(Ind, 50);

         }
         break;
      case TRAP_OF_PRESENT_EXCHANGE:
		 ident = do_player_drop_items(Ind, 50);
		 ident |= do_player_trap_garbage(Ind, 300);
		 break;
      default:
      {
         s_printf("Executing unknown trap %d", trap);
      }
   }

   /* some traps vanish */
   if (vanish)
   {
	   if (item < 0) delete_trap_idx(tt_ptr);
	   else i_ptr->pval = 0;
   }

   if (never_id) return (FALSE);
   else return ident;
}

/*
 * though door traps cannot be created for now...	- Jir -
 */
void player_activate_door_trap(int Ind, s16b y, s16b x)
{
	player_type *p_ptr = Players[Ind];
	cave_type *c_ptr;
	trap_type *t_ptr;
	bool ident = FALSE;
	int trap = 0;

	/* Paranoia */
	cave_type **zcave;
	if (!in_bounds(y, x)) return;
	if((zcave=getcave(&p_ptr->wpos)))
	{
		c_ptr=&zcave[y][x];
		t_ptr = c_ptr->special.ptr;
		trap=t_ptr->t_idx;
	}


	/* Return if trap or door not found */
//	if ((c_ptr->t_idx == 0) ||
//	    !(f_info[c_ptr->feat].flags1 & FF1_DOOR)) return;

	if (!trap) return;

	/* Disturb */
	disturb(Ind, 0, 0);

	/* Message */
	msg_print(Ind, "You found a trap!");

	/* Pick a trap */
	pick_trap(&p_ptr->wpos, y, x);

	/* Hit the trap */
	ident = player_activate_trap_type(Ind, y, x, NULL, -1);
	if (ident && !p_ptr->trap_ident[trap])
	{
		p_ptr->trap_ident[trap] = TRUE;
                msg_format(Ind, "You identified that trap as %s.",
			   t_name + t_info[trap].name);
	}
}

/*
 * Places a random trap at the given location.
 *
 * The location must be a valid, empty, clean, floor grid.
 */
/*
 * pfft, no trapped-doors till f_info reform comes	- Jir -
 */
//void place_trap(int y, int x)
void place_trap(struct worldpos *wpos, int y, int x)
{
	bool           more       = TRUE;
	s16b           trap, t_idx;
	trap_kind	*t_ptr;
	trap_type	*tt_ptr;

	s16b           cnt        = 0;
	u32b flags;
	cave_type *c_ptr;
	//	dungeon_info_type *d_ptr = &d_info[dungeon_type];

	/* Paranoia -- verify location */
	cave_type **zcave;

	if(!(zcave=getcave(wpos))) return;
	if (!in_bounds(y, x)) return;
	c_ptr = &zcave[y][x];

	/* Require empty, clean, floor grid */
	if (!cave_floor_grid(c_ptr) &&
		((c_ptr->feat < FEAT_DOOR_HEAD) ||
		(c_ptr->feat > FEAT_DOOR_TAIL))) return;

	/* No traps over traps/house doors etc */
	if (c_ptr->special.type) return;

//	if (!cave_naked_bold(zcave, y, x)) return;
//	if (!cave_floor_bold(zcave, y, x)) return;


	/* no traps in town or on first level */
	//   if (dun_level<=1) return;

	/* traps only appears on empty floor */
	//   if (!cave_floor_grid(c_ptr) && (!(f_info[c_ptr->feat].flags1 & FF1_DOOR))) return;

	/* set flags */
	/*
	   if (((c_ptr->feat >= FEAT_DOOR_HEAD) && 
	   (c_ptr->feat <= FEAT_DOOR_TAIL)) ||
	   */
#if 0
	if (f_info[c_ptr->feat].flags1 & FF1_DOOR)
		flags = FTRAP_DOOR;
	else flags = FTRAP_FLOOR;
#endif	// 0
	if ((c_ptr->feat >= FEAT_DOOR_HEAD) && 
		(c_ptr->feat <= FEAT_DOOR_TAIL))
		flags = FTRAP_DOOR;
	else flags = FTRAP_FLOOR;

	/* try 100 times */
	while ((more) && (cnt++)<100)
	{
		trap = randint(MAX_T_IDX - 1);
		t_ptr = &t_info[trap];

		/* no traps below their minlevel */
		if (t_ptr->minlevel > getlevel(wpos)) continue;

		/* is this a correct trap now?   */
		if (!(t_ptr->flags & flags)) continue;

		/* hack, no trap door at the bottom of dungeon or in flat(non dungeon) places */
		//      if (((d_ptr->maxdepth == dun_level) || (d_ptr->flags1 & DF1_FLAT)) && (trap == TRAP_OF_SINKING)) continue;

		/* how probable is this trap   */
		if (rand_int(100)<t_ptr->probability)
		{
			//	 c_ptr->t_idx = trap;
			more = FALSE;

			/* Make a trap */
			t_idx = t_pop();

			/* Success */
			if (t_idx)
			{
				tt_ptr = &t_list[t_idx];
				c_ptr->special.type = CS_TRAPS;
				c_ptr->special.ptr = tt_ptr;

				tt_ptr->iy = y;
				tt_ptr->ix = x;
				tt_ptr->t_idx = trap;
				tt_ptr->found = FALSE;

				wpcopy(&tt_ptr->wpos, wpos);
//				c_ptr=&zcave[y][x];
			}
		}
	}

	return;
}

/*
 * Places a random trap on the given chest.
 *
 * The object must be a valid chest.
 */
void place_trap_object(object_type *o_ptr)
{
   bool           more       = TRUE;
   s16b           trap;
   trap_kind	*t_ptr;
   s16b           cnt        = 0;

   /* no traps in town or on first level */
//   if (dun_level<=1) return;

   /* try 100 times */
   while ((more) && (cnt++)<100)
   {
      trap = randint(MAX_T_IDX - 1);
      t_ptr = &t_info[trap];

      /* no traps below their minlevel */
      if (t_ptr->minlevel>getlevel(&o_ptr->wpos)) continue;

      /* is this a correct trap now?   */
      if (!(t_ptr->flags & FTRAP_CHEST)) continue;

      /* how probable is this trap   */
      if (rand_int(100)<t_ptr->probability)
	  {
		  o_ptr->pval = trap;
		  more = FALSE;
      }
   }

   return;
}

/* Dangerous trap placing function */
/* (Not so dangerous as was once	- Jir -) */
//void wiz_place_trap(int y, int x, int idx)
void wiz_place_trap(int Ind, int trap)
{
	player_type *p_ptr = Players[Ind];
	int x = p_ptr->px, y = p_ptr->py;
	worldpos *wpos = &p_ptr->wpos;
	s16b           t_idx;
	trap_kind	*t_ptr;
	trap_type	*tt_ptr;

	s16b           cnt        = 0;
	u32b flags;
	cave_type *c_ptr;
	//	dungeon_info_type *d_ptr = &d_info[dungeon_type];

	/* Paranoia -- verify location */
	cave_type **zcave;

	if(!(zcave=getcave(wpos))) return;
	if (!in_bounds(y, x)) return;
	c_ptr = &zcave[y][x];

	/* Require empty, clean, floor grid */
	if (!cave_floor_grid(c_ptr) &&
			((c_ptr->feat < FEAT_DOOR_HEAD) ||
			 (c_ptr->feat > FEAT_DOOR_TAIL)))
	{
		msg_print(Ind, "Inappropriate grid feature type!");
		return;
	}

	/* No traps over traps/house doors etc */
	if (c_ptr->special.type)
	{
		msg_print(Ind, "Cave-Special already exists!");
		return;
	}


	//	if (!cave_naked_bold(zcave, y, x)) return;
	//	if (!cave_floor_bold(zcave, y, x)) return;


	/* no traps in town or on first level */
	//   if (dun_level<=1) return;

	/* traps only appears on empty floor */
	//   if (!cave_floor_grid(c_ptr) && (!(f_info[c_ptr->feat].flags1 & FF1_DOOR))) return;

	/* set flags */
	/*
	   if (((c_ptr->feat >= FEAT_DOOR_HEAD) && 
	   (c_ptr->feat <= FEAT_DOOR_TAIL)) ||
	   */
#if 0
	if (f_info[c_ptr->feat].flags1 & FF1_DOOR)
		flags = FTRAP_DOOR;
	else flags = FTRAP_FLOOR;
#endif	// 0
	/* is this a correct trap now?   */
	if (trap < 0 || MAX_T_IDX < trap)
	{
		msg_print(Ind, "Trap index is out of range!");
		return;
	}

	t_ptr = &t_info[trap];

	/* is this a correct trap now?   */
	if (!t_ptr->name)
	{
		msg_print(Ind, "Specified no. of trap does not exist!");
		return;
	}

	/* no traps below their minlevel */
	if (t_ptr->minlevel > getlevel(wpos))
	{
		msg_print(Ind, "The trap is out of depth!");
		return;
	}

	if ((c_ptr->feat >= FEAT_DOOR_HEAD) && 
			(c_ptr->feat <= FEAT_DOOR_TAIL))
		flags = FTRAP_DOOR;
	else flags = FTRAP_FLOOR;

	/* is this a correct trap now?   */
	if (!(t_ptr->flags & flags))
	{
		msg_print(Ind, "Feature type(door/floor) is not proper!");
		return;
	}

	/* hack, no trap door at the bottom of dungeon or in flat(non dungeon) places */
	//      if (((d_ptr->maxdepth == dun_level) || (d_ptr->flags1 & DF1_FLAT)) && (trap == TRAP_OF_SINKING)) continue;

	//	 c_ptr->t_idx = trap;

	/* Make a trap */
	t_idx = t_pop();

	/* Success */
	if (t_idx)
	{
		tt_ptr = &t_list[t_idx];
		c_ptr->special.type = CS_TRAPS;
		c_ptr->special.ptr = tt_ptr;

		tt_ptr->iy = y;
		tt_ptr->ix = x;
		tt_ptr->t_idx = trap;
		tt_ptr->found = FALSE;

		wpcopy(&tt_ptr->wpos, wpos);
		//				c_ptr=&zcave[y][x];
	}
	else
	{
		msg_print(Ind, "Failed to allocate t_list[]!");
		return;
	}

	return;

#if 0
	cave_type *c_ptr = &cave[y][x];

	/* Dangerous enough as it is... */
	if (!cave_floor_grid(c_ptr) && (!(f_info[c_ptr->feat].flags1 & FF1_DOOR))) return;

	c_ptr->t_idx = idx;
#endif	// 0
}

#if 0	// soon
/*
 * Here begin monster traps code
 */

/*
 * Hook to determine if an object is a device
 */
static bool item_tester_hook_device(object_type *o_ptr)
{
        if ((o_ptr->tval == TV_ROD) ||
            (o_ptr->tval == TV_STAFF) ||
            (o_ptr->tval == TV_WAND)) return (TRUE);

	/* Assume not */
	return (FALSE);
}

/*
 * Hook to determine if an object is a potion
 */
static bool item_tester_hook_potion(object_type *o_ptr)
{
        if ((o_ptr->tval == TV_POTION) ||
            (o_ptr->tval == TV_POTION2)) return (TRUE);

	/* Assume not */
	return (FALSE);
}

/*
 * The trap setting code for rogues -MWK- 
 *
 * Also, it will fail or give weird results if the tvals are resorted!
 */
void do_cmd_set_trap(void)                
{
        int item_kit, item_load, i;
	int num;

	object_type *o_ptr, *j_ptr, *i_ptr;
	
	cptr q,s,c;
	
	object_type object_type_body;

        u32b f1, f2, f3, f4, f5, esp;

	/* Check some conditions */
	if (p_ptr->blind)            
	{
		msg_print("You can't see anything.");
		return;
	}
        if (no_lite())
	{
		msg_print("You don't dare to set a trap in the darkness.");
		return;
	}
	if (p_ptr->confused)
	{
		msg_print("You are too confused!");
		return;
	}

	/* Only set traps on clean floor grids */
        if (!cave_clean_bold(py, px))
	{
		msg_print("You cannot set a trap on this.");
		return;
	}

	/* Restrict choices to trapkits */
	item_tester_tval = TV_TRAPKIT;

	/* Get an item */
	q = "Use which trapping kit? ";
	s = "You have no trapping kits.";
	if (!get_item(&item_kit, q, s, USE_INVEN)) return;
	
	o_ptr = &inventory[item_kit];
	
	/* Trap kits need a second object */
	switch (o_ptr->sval)
	{
                case SV_TRAPKIT_BOW:
			item_tester_tval = TV_ARROW;
			break;
                case SV_TRAPKIT_XBOW:
			item_tester_tval = TV_BOLT;
			break;
                case SV_TRAPKIT_SLING:
			item_tester_tval = TV_SHOT;
			break;
                case SV_TRAPKIT_POTION:
                        item_tester_hook = item_tester_hook_potion;
			break;
                case SV_TRAPKIT_SCROLL:
			item_tester_tval = TV_SCROLL;
			break;
                case SV_TRAPKIT_DEVICE:
                        item_tester_hook = item_tester_hook_device;
			break;
		default:
			msg_print("Unknown trapping kit type!");
			break;
	}
	
	/* Get the second item */
	q = "Load with what? ";
	s = "You have nothing to load that trap with.";
	if (!get_item(&item_load, q, s, USE_INVEN)) return;
	
	/* Get the second object */
	j_ptr = &inventory[item_load];

	/* Assume a single object */
	num = 1;
				
	/* In some cases, take multiple objects to load */
        if (o_ptr->sval != SV_TRAPKIT_DEVICE)
	{
                object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

                if ((f3 & TR3_XTRA_SHOTS) && (o_ptr->pval > 0)) num += o_ptr->pval;
	
		if (f2 & (TRAP2_AUTOMATIC_5 | TRAP2_AUTOMATIC_99)) num = 99;
		
		if (num > j_ptr->number) num = j_ptr->number;

		c = format("How many (1-%d)? ", num);
				
		/* Ask for number of items to use */
		num = get_quantity(c, num);
	}

	/* Canceled */
	if (!num) return; 
		
	/* Take a turn */
        energy_use = 100;

	/* Get local object */
	i_ptr = &object_type_body;
	
	/* Obtain local object for trap content */
	object_copy(i_ptr, j_ptr);

	/* Set number */
	i_ptr->number = num;
	
	/* Drop it here */
        cave[py][px].special = floor_carry(py, px, i_ptr);
	
	/* Obtain local object for trap trigger kit */
	object_copy(i_ptr, o_ptr);
	
	/* Set number */
	i_ptr->number = 1;

	/* Drop it here */
        cave[py][px].special2 = floor_carry(py, px, i_ptr);

	/* Modify, Describe, Optimize */
	inven_item_increase(item_kit, -1);
	inven_item_describe(item_kit);
	inven_item_increase(item_load, -num);
	inven_item_describe(item_load);

        for (i = 0; i < INVEN_WIELD; i++)
        {
                if (inven_item_optimize(i)) break;
        }
        for (i = 0; i < INVEN_WIELD; i++)
        {
                inven_item_optimize(i);
        }

	/* Actually set the trap */
	cave_set_feat(py, px, FEAT_MON_TRAP);
}

/* 
 * Monster hitting a rod trap -MWK-
 *
 * Return TRUE if the monster died
 */ 
bool mon_hit_trap_aux_rod(int m_idx, object_type *o_ptr)
{
	int dam = 0, typ = 0;
        int rad = 0;
	monster_type *m_ptr = &m_list[m_idx];
	int y = m_ptr->fy;
        int x = m_ptr->fx;
        u32b f1, f2, f3, f4, f5, esp;
        object_kind *tip_ptr = &k_info[lookup_kind(TV_ROD, o_ptr->pval)];

        object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	/* Depend on rod type */
        switch (o_ptr->pval)
	{
		case SV_ROD_DETECT_TRAP:
                        m_ptr->smart |= SM_NOTE_TRAP;
			break;
		case SV_ROD_DETECTION:
                        m_ptr->smart |= SM_NOTE_TRAP;
			break;
		case SV_ROD_ILLUMINATION:
			typ = GF_LITE_WEAK;
			dam = damroll(2, 15);
			rad = 3;
			lite_room(y, x);
			break;
		case SV_ROD_CURING:
			typ = GF_OLD_HEAL;
			dam = damroll(3, 4); /* and heal conf? */
			break;
		case SV_ROD_HEALING:
			typ = GF_OLD_HEAL;
			dam = 300;
			break;
		case SV_ROD_SPEED:
			typ = GF_OLD_SPEED;
			dam = 50;
			break;
		case SV_ROD_TELEPORT_AWAY:
		        typ = GF_AWAY_ALL;
		        dam = MAX_SIGHT * 5;
		        break;
		case SV_ROD_DISARMING:
			break;
		case SV_ROD_LITE:
			typ = GF_LITE_WEAK;
			dam = damroll(6, 8);
			break;
		case SV_ROD_SLEEP_MONSTER:
			typ = GF_OLD_SLEEP;
			dam = 50;
			break;
		case SV_ROD_SLOW_MONSTER:
			typ = GF_OLD_SLOW;
			dam = 50;
			break;
		case SV_ROD_DRAIN_LIFE:
			typ = GF_OLD_DRAIN;
			dam = 75;
			break;
		case SV_ROD_POLYMORPH:
			typ = GF_OLD_POLY;
			dam = 50;
			break;
		case SV_ROD_ACID_BOLT:
			typ = GF_ACID;
			dam = damroll(6, 8);
			break;
		case SV_ROD_ELEC_BOLT:
			typ = GF_ELEC;
			dam = damroll(3, 8);
			break;
		case SV_ROD_FIRE_BOLT:
			typ = GF_FIRE;
			dam = damroll(8, 8);
			break;
		case SV_ROD_COLD_BOLT:
			typ = GF_COLD;
			dam = damroll(5, 8);
			break;
		case SV_ROD_ACID_BALL:
			typ = GF_ACID;
			dam = 60;
			rad = 2;
			break;
		case SV_ROD_ELEC_BALL:
			typ = GF_ELEC;
			dam = 32;
			rad = 2;
			break;
		case SV_ROD_FIRE_BALL:
			typ = GF_FIRE;
			dam = 72;
			rad = 2;
			break;
		case SV_ROD_COLD_BALL:
			typ = GF_COLD;
			dam = 48;
			rad = 2;
			break;		
		default:
			return (FALSE);
	}
	
	/* Actually hit the monster */
	if (typ) (void) project(-999, rad, y, x, dam, typ, PROJECT_KILL | PROJECT_ITEM | PROJECT_JUMP);

	/* Set rod recharge time */
        o_ptr->timeout -= (f4 & TR4_CHEAPNESS)?tip_ptr->pval / 2:tip_ptr->pval;

        return (cave[y][x].m_idx == 0 ? TRUE : FALSE);
}

/* 
 * Monster hitting a device trap -MWK-
 *
 * Return TRUE if the monster died
 */ 
bool mon_hit_trap_aux_staff(int m_idx, int sval)
{
	monster_type *m_ptr = &m_list[m_idx];
	int dam = 0, typ = 0;
	int rad = 0;
	int k;
	int y = m_ptr->fy;
	int x = m_ptr->fx;	
	
	/* Depend on staff type */
	switch (sval)
	{
		case SV_STAFF_IDENTIFY:		
		case SV_STAFF_DETECT_DOOR:	
		case SV_STAFF_DETECT_INVIS:	
		case SV_STAFF_DETECT_EVIL:	
		case SV_STAFF_DETECT_GOLD:
		case SV_STAFF_DETECT_ITEM:	
		case SV_STAFF_MAPPING:
		case SV_STAFF_PROBING:
		case SV_STAFF_REMOVE_CURSE:	
		case SV_STAFF_THE_MAGI:
			return (FALSE);		
			
		case SV_STAFF_DARKNESS:
			unlite_room(y, x);
			typ = GF_DARK_WEAK;
			dam = 10;
			rad = 3;
			break;
		case SV_STAFF_SLOWNESS:
			typ = GF_OLD_SLOW;
			dam = damroll(5, 10);
			break;		
		case SV_STAFF_HASTE_MONSTERS:
			typ = GF_OLD_SPEED;
			dam = damroll(5, 10);
			rad = 5;	/* hack */
			break;
		case SV_STAFF_SUMMONING:
			for (k = 0; k < randint(4) ; k++)
                                (void)summon_specific(y, x, dun_level, 0);
			return (FALSE);
		case SV_STAFF_TELEPORTATION:	
			typ = GF_AWAY_ALL;
			dam = 100;
			break;
		case SV_STAFF_STARLITE:
			/* Hack */
			typ = GF_LITE_WEAK;
			dam = damroll(6, 8);
			rad = 3;
			break;
		case SV_STAFF_LITE:
			lite_room(y, x);
			typ = GF_LITE_WEAK;
			dam = damroll(2, 8);
			rad = 2;
			break;			
		case SV_STAFF_DETECT_TRAP:
                        m_ptr->smart |= SM_NOTE_TRAP;
			return (FALSE);
		case SV_STAFF_CURE_LIGHT:
			typ = GF_OLD_HEAL;
			dam = randint(8);
			break;
		case SV_STAFF_CURING:
			typ = GF_OLD_HEAL;
			dam = randint(4); /* hack */
			break;
		case SV_STAFF_HEALING:		
			typ = GF_OLD_HEAL;
			dam = 300;
			break;
		case SV_STAFF_SLEEP_MONSTERS:
			typ = GF_OLD_SLEEP;
			dam = damroll(5, 10);
			rad = 5;
			break;
		case SV_STAFF_SLOW_MONSTERS:
			typ = GF_OLD_SLOW;
			dam = damroll(5, 10);
			rad = 5;
			break;
		case SV_STAFF_SPEED:
			typ = GF_OLD_SPEED;
			dam = damroll(5, 10);
			break;
		case SV_STAFF_DISPEL_EVIL:
			typ = GF_DISP_EVIL;
			dam = 60;
			rad = 5;
			break;
		case SV_STAFF_POWER:
		        typ = GF_DISP_ALL;
		        dam = 120;
		        rad = 5;
		        break;
		case SV_STAFF_HOLINESS:
		        typ = GF_DISP_EVIL;
		        dam = 120;
		        rad = 5;
		        break;
		case SV_STAFF_GENOCIDE:	
		{
			monster_race *r_ptr = &r_info[m_ptr->r_idx];
                        genocide_aux(FALSE, r_ptr->d_char);
			/* although there's no point in a multiple genocide trap... */
                        return (cave[y][x].m_idx == 0 ? TRUE : FALSE);
		}
		case SV_STAFF_EARTHQUAKES:
			earthquake(y, x, 10);
			return (FALSE);
		case SV_STAFF_DESTRUCTION:
			destroy_area(y, x, 15, TRUE);
			return (FALSE);
		default:
			return (FALSE);
	}
	
	/* Actually hit the monster */
	(void) project(-2, rad, y, x, dam, typ, PROJECT_KILL | PROJECT_ITEM | PROJECT_JUMP);
        return (cave[y][x].m_idx == 0 ? TRUE : FALSE); 
}

/* 
 * Monster hitting a scroll trap -MWK-
 *
 * Return TRUE if the monster died
 */ 
bool mon_hit_trap_aux_scroll(int m_idx, int sval)
{
	monster_type *m_ptr = &m_list[m_idx];
	int dam = 0, typ = 0;
	int rad = 0;
        int y = m_ptr->fy;
        int x = m_ptr->fx;
        int k;
		
	/* Depend on scroll type */
	switch (sval)
	{
		case SV_SCROLL_CURSE_ARMOR:	
		case SV_SCROLL_CURSE_WEAPON:		
		case SV_SCROLL_TRAP_CREATION: /* these don't work :-( */
		case SV_SCROLL_WORD_OF_RECALL: /* should these? */
		case SV_SCROLL_IDENTIFY:
		case SV_SCROLL_STAR_IDENTIFY:
		case SV_SCROLL_MAPPING:			
		case SV_SCROLL_DETECT_GOLD:
		case SV_SCROLL_DETECT_ITEM:
		case SV_SCROLL_REMOVE_CURSE:
		case SV_SCROLL_STAR_REMOVE_CURSE:
		case SV_SCROLL_ENCHANT_ARMOR:	
		case SV_SCROLL_ENCHANT_WEAPON_TO_HIT:
		case SV_SCROLL_ENCHANT_WEAPON_TO_DAM:	
		case SV_SCROLL_STAR_ENCHANT_ARMOR:
		case SV_SCROLL_STAR_ENCHANT_WEAPON:	
		case SV_SCROLL_RECHARGING:	
		case SV_SCROLL_DETECT_DOOR:
		case SV_SCROLL_DETECT_INVIS:
		case SV_SCROLL_SATISFY_HUNGER:
		case SV_SCROLL_RUNE_OF_PROTECTION:
		case SV_SCROLL_TRAP_DOOR_DESTRUCTION:
		case SV_SCROLL_PROTECTION_FROM_EVIL:
			return (FALSE);
		case SV_SCROLL_DARKNESS:
			unlite_room(y, x);
			typ = GF_DARK_WEAK;
			dam = 10;
			rad = 3;
			break;
		case SV_SCROLL_AGGRAVATE_MONSTER:	
		      	aggravate_monsters(m_idx);
		      	return (FALSE);
		case SV_SCROLL_SUMMON_MONSTER:
                        for (k = 0; k < randint(3) ; k++) summon_specific(y, x, dun_level, 0);
			return (FALSE);	
		case SV_SCROLL_SUMMON_UNDEAD:	
                        for (k = 0; k < randint(3) ; k++) summon_specific(y, x, dun_level, SUMMON_UNDEAD);
			return (FALSE);	
		case SV_SCROLL_PHASE_DOOR:
			typ = GF_AWAY_ALL;
			dam = 10;
			break;	
		case SV_SCROLL_TELEPORT:
			typ = GF_AWAY_ALL;
			dam = 100;
			break;
		case SV_SCROLL_TELEPORT_LEVEL:
			delete_monster(y, x);
			return (TRUE);
		case SV_SCROLL_LIGHT:		
			lite_room(y, x);
			typ = GF_LITE_WEAK;
			dam = damroll(2, 8);
			rad = 2;
			break;			
		case SV_SCROLL_DETECT_TRAP:
                        m_ptr->smart |= SM_NOTE_TRAP;
			return (FALSE);
		case SV_SCROLL_BLESSING:
                        typ = GF_HOLY_FIRE;
			dam = damroll(1, 4);
			break;
		case SV_SCROLL_HOLY_CHANT:
                        typ = GF_HOLY_FIRE;
			dam = damroll(2, 4);
			break;
		case SV_SCROLL_HOLY_PRAYER:		
                        typ = GF_HOLY_FIRE;
			dam = damroll(4, 4);
			break;
		case SV_SCROLL_MONSTER_CONFUSION:
			typ = GF_OLD_CONF;
			dam = damroll(5, 10);
			break;
		case SV_SCROLL_STAR_DESTRUCTION:
			destroy_area(y, x, 15, TRUE);
			return (FALSE);			
		case SV_SCROLL_DISPEL_UNDEAD:
			typ = GF_DISP_UNDEAD;
			rad = 5;
			dam = 60;
			break;
		case SV_SCROLL_GENOCIDE:
		{
			monster_race *r_ptr = &r_info[m_ptr->r_idx];
                        genocide_aux(FALSE, r_ptr->d_char);
			/* although there's no point in a multiple genocide trap... */
			return (!(r_ptr->flags1 & RF1_UNIQUE));
		}
		case SV_SCROLL_MASS_GENOCIDE:         
			for (k = 0; k < 8; k++)
				delete_monster(y+ddy[k], x+ddx[k]);
			delete_monster(y,x);
			return(TRUE);
		case SV_SCROLL_ACQUIREMENT:
                        acquirement(y, x, 1, TRUE, FALSE);
			return (FALSE);
		case SV_SCROLL_STAR_ACQUIREMENT:
                        acquirement(y, x, randint(2) + 1, TRUE, FALSE);
			return (FALSE);
		default:
			return (FALSE);
	}
	
	/* Actually hit the monster */
	(void) project(-2, rad, y, x, dam, typ, PROJECT_KILL | PROJECT_ITEM | PROJECT_JUMP);
        return (cave[y][x].m_idx == 0 ? TRUE : FALSE); 
}

/* 
 * Monster hitting a wand trap -MWK-
 *
 * Return TRUE if the monster died
 */ 
bool mon_hit_trap_aux_wand(int m_idx, int sval)
{
	monster_type *m_ptr = &m_list[m_idx];
	int dam = 0, typ = 0;
	int rad = 0;
	int y = m_ptr->fy;
	int x = m_ptr->fx;
	
	if (sval == SV_WAND_WONDER) sval = rand_int(SV_WAND_WONDER);
	/* Depend on wand type */
	switch (sval)
	{
		case SV_WAND_HEAL_MONSTER:
			typ = GF_OLD_HEAL;
			dam = damroll(4, 6);
			break;
		case SV_WAND_HASTE_MONSTER:
			typ = GF_OLD_SPEED;
			dam = damroll(5, 10);
			break;
		case SV_WAND_CLONE_MONSTER:
		        typ = GF_OLD_CLONE;
		        break;
		case SV_WAND_TELEPORT_AWAY:
		        typ = GF_AWAY_ALL;
		        dam = MAX_SIGHT * 5;
		        break;
		case SV_WAND_DISARMING:
			return (FALSE);
		case SV_WAND_TRAP_DOOR_DEST:
			return (FALSE);
		case SV_WAND_STONE_TO_MUD:
			typ = GF_KILL_WALL;
			dam = 20 + randint(30);
			break;
		case SV_WAND_LITE:
			typ = GF_LITE_WEAK;
			dam = damroll(6, 8);
			break;
		case SV_WAND_SLEEP_MONSTER:
			typ = GF_OLD_SLEEP;
			dam = damroll(5, 10);
			break;
		case SV_WAND_SLOW_MONSTER:
			typ = GF_OLD_SLOW;
			dam = damroll(5, 10);
			break;
		case SV_WAND_CONFUSE_MONSTER:
			typ = GF_OLD_CONF;
			dam = damroll(5, 10);
			break;
		case SV_WAND_FEAR_MONSTER:
			typ = GF_TURN_ALL;
			dam = damroll(5, 10);
			break;
		case SV_WAND_DRAIN_LIFE:
			typ = GF_OLD_DRAIN;
			dam = 75;
			break;
		case SV_WAND_POLYMORPH:
			typ = GF_OLD_POLY;
			dam = damroll(5, 10);
			break;
		case SV_WAND_STINKING_CLOUD:
			typ = GF_POIS;
			dam = 12;
			rad = 2;
			break;
		case SV_WAND_MAGIC_MISSILE:
			typ = GF_MISSILE;
			dam = damroll(2, 6);
			break;
		case SV_WAND_ACID_BOLT:
		        typ = GF_ACID;
		        dam = damroll(5, 8);
		        break;
		case SV_WAND_FIRE_BOLT:
			typ = GF_FIRE;
			dam = damroll(6, 8);
			break;
		case SV_WAND_COLD_BOLT:
			typ = GF_COLD;
			dam = damroll(3, 8);
			break;
		case SV_WAND_ACID_BALL:
			typ = GF_ACID;
			rad = 2;
			dam = 60;
			break;
		case SV_WAND_ELEC_BALL:
			typ = GF_ELEC;
			rad = 2;
			dam = 32;
			break;
		case SV_WAND_FIRE_BALL:
			typ = GF_FIRE;
			rad = 2;
			dam = 72;
			break;
		case SV_WAND_COLD_BALL:
			typ = GF_COLD;
			dam = 48;
			rad = 2;
			break;
		case SV_WAND_ANNIHILATION:
			typ = GF_OLD_DRAIN;
			dam = 125;
			break;
		case SV_WAND_DRAGON_FIRE:
			typ = GF_FIRE;
			dam = 100;
			rad = 3;
			break;
		case SV_WAND_DRAGON_COLD:
			typ = GF_COLD;
			dam = 80;
			rad = 3;
			break;
		case SV_WAND_DRAGON_BREATH:
			switch(randint(5))
			{
				case 1: typ = GF_FIRE; break;
				case 2: typ = GF_ELEC; break;
				case 3: typ = GF_ACID; break;
				case 4: typ = GF_COLD; break;
				case 5: typ = GF_POIS; break;
			}
			dam = 100; /* hack */
			rad = 3;
			break;
		default:
			return (FALSE);
	}
	
	/* Actually hit the monster */
	(void) project(-2, rad, y, x, dam, typ, PROJECT_KILL | PROJECT_ITEM | PROJECT_JUMP);
        return (cave[y][x].m_idx == 0 ? TRUE : FALSE); 
}

/* 
 * Monster hitting a potions trap -MWK-
 *
 * Return TRUE if the monster died
 */ 
bool mon_hit_trap_aux_potion(int m_idx, object_type *o_ptr)
{
	monster_type *m_ptr = &m_list[m_idx];
        int dam = 0, typ = 0;
	int y = m_ptr->fy;
	int x = m_ptr->fx;
        int sval = o_ptr->sval;
	
	/* Depend on potion type */
        if (o_ptr->tval == TV_POTION)
        {
	switch (sval)
	{
		/* Nothing happens */
		case SV_POTION_WATER:
		case SV_POTION_APPLE_JUICE:
		case SV_POTION_SLIME_MOLD:
		case SV_POTION_SALT_WATER:
		case SV_POTION_DEC_STR:
		case SV_POTION_DEC_INT:	
		case SV_POTION_DEC_WIS:
		case SV_POTION_DEC_DEX:
		case SV_POTION_DEC_CON:
		case SV_POTION_DEC_CHR:
		case SV_POTION_INFRAVISION:
		case SV_POTION_DETECT_INVIS:
		case SV_POTION_SLOW_POISON:
		case SV_POTION_CURE_POISON:
		case SV_POTION_RESIST_HEAT:
		case SV_POTION_RESIST_COLD:
		case SV_POTION_RESTORE_MANA:
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
		case SV_POTION_RUINATION:	/* ??? */		
		case SV_POTION_ENLIGHTENMENT:
		case SV_POTION_STAR_ENLIGHTENMENT:
		case SV_POTION_SELF_KNOWLEDGE:
			return (FALSE);

		case SV_POTION_EXPERIENCE:
                        if (m_ptr->level < MONSTER_LEVEL_MAX)
                        {
                                m_ptr->exp = MONSTER_EXP(m_ptr->level + 1);
                                monster_check_experience(m_idx, FALSE);
                        }
                        return (FALSE);
		case SV_POTION_SLOWNESS:
			typ = GF_OLD_SLOW;
			dam = damroll(4, 6);
			break;
		case SV_POTION_POISON:
			typ = GF_POIS;
			dam = damroll(8, 6);
			break;
		case SV_POTION_CONFUSION:
			typ = GF_CONFUSION;
			dam = damroll(4, 6);
			break;
		case SV_POTION_BLINDNESS:
			typ = GF_DARK;
			dam = 10;
			break;
		case SV_POTION_SLEEP:
			typ = GF_OLD_SLEEP;
			dam = damroll (4, 6);
			break;
		case SV_POTION_LOSE_MEMORIES:
                        typ = GF_OLD_CONF;
			dam = damroll(10, 10);
			break;			
		case SV_POTION_DETONATIONS:
			typ = GF_DISINTEGRATE;
			dam = damroll(20, 20);
			break;
		case SV_POTION_DEATH:
			typ = GF_NETHER;
                        dam = damroll(100, 20);
			break;
		case SV_POTION_BOLDNESS:
			m_ptr->monfear = 0;
			return (FALSE);
		case SV_POTION_SPEED:
			typ = GF_OLD_SPEED;
			dam = damroll(5, 10);
			break;
		case SV_POTION_HEROISM:
		case SV_POTION_BESERK_STRENGTH:
			m_ptr->monfear = 0;
			typ = GF_OLD_HEAL;
			dam = damroll(2, 10);
			break;
		case SV_POTION_CURE_LIGHT:
			typ = GF_OLD_HEAL;
			dam = damroll(3, 4);
			break;
		case SV_POTION_CURE_SERIOUS:
			typ = GF_OLD_HEAL;
			dam = damroll(4, 6);
			break;
		case SV_POTION_CURE_CRITICAL:
			typ = GF_OLD_HEAL;
			dam = damroll(6, 8);
			break;
		case SV_POTION_HEALING:
			typ = GF_OLD_HEAL;
			dam = 300;
			break;
		case SV_POTION_STAR_HEALING:
			typ = GF_OLD_HEAL;
			dam = 1000;
			break;
		case SV_POTION_LIFE:
		{
			monster_race *r_ptr = &r_info[m_ptr->r_idx];
			if (r_ptr->flags3 & RF3_UNDEAD)
			{
                                typ = GF_HOLY_FIRE;
				dam = damroll(20, 20);
			}
			else
			{	
				typ = GF_OLD_HEAL;
				dam = 5000;
			}
			break;
		}				
		default:
			return (FALSE);
			
	}
        }
        else
        {
        }
	
	/* Actually hit the monster */
	(void) project_m(-2, 0, y, x, dam, typ);
        return (cave[y][x].m_idx == 0 ? TRUE : FALSE);
}
 
/*
 * Monster hitting a monster trap -MWK-
 * Returns True if the monster died, false otherwise
 */  
bool mon_hit_trap(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
#if 0 /* DGDGDGDG */
        monster_lore *l_ptr = &l_list[m_ptr->r_idx];
#endif

        object_type *kit_o_ptr, *load_o_ptr, *j_ptr;
		
        u32b f1, f2, f3, f4, f5, esp;
        
        object_type object_type_body;
               
	int mx = m_ptr->fx;
	int my = m_ptr->fy;

	int difficulty;
	int smartness;
	
	char m_name[80];
		
	bool notice = FALSE;
	bool disarm = FALSE;
	bool remove = FALSE;
	bool dead = FALSE;
	bool fear = FALSE;
        s32b special = 0;

	int dam, chance, shots;
        int mul = 0;
			
	/* Get the trap objects */
        kit_o_ptr = &o_list[cave[my][mx].special2];
        load_o_ptr = &o_list[cave[my][mx].special];
	j_ptr = &object_type_body;

	/* Get trap properties */
        object_flags(kit_o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	/* Can set off check */
	/* Ghosts only set off Ghost traps */
	if ((r_ptr->flags2 & RF2_PASS_WALL) && !(f2 & TRAP2_KILL_GHOST)) return (FALSE);
	
	/* Some traps are specialized to some creatures */
	if (f2 & TRAP2_ONLY_MASK)
	{
		bool affect = FALSE;
		if ((f2 & TRAP2_ONLY_DRAGON) && (r_ptr->flags3 & RF3_DRAGON)) affect = TRUE;
		if ((f2 & TRAP2_ONLY_DEMON)  && (r_ptr->flags3 & RF3_DEMON))  affect = TRUE;
		if ((f2 & TRAP2_ONLY_UNDEAD) && (r_ptr->flags3 & RF3_UNDEAD)) affect = TRUE;
		if ((f2 & TRAP2_ONLY_EVIL)   && (r_ptr->flags3 & RF3_EVIL))   affect = TRUE;
		if ((f2 & TRAP2_ONLY_ANIMAL) && (r_ptr->flags3 & RF3_ANIMAL)) affect = TRUE;
		
		/* Don't set it off if forbidden */
		if (!affect) return (FALSE);
	}
		
	/* Get detection difficulty */
	/* High level players hide traps better */
	difficulty = p_ptr->lev;
	
	/* Some traps are well-hidden */
	if (f1 & TR1_STEALTH)
	{
		difficulty += 10 * (kit_o_ptr->pval);
	}
	
	/* Get monster smartness for trap detection */
	/* Higher level monsters are smarter */
	smartness = r_ptr->level;
	
	/* Smart monsters are better at detecting traps */
	if (r_ptr->flags2 & RF2_SMART) smartness += 10;
	
	/* Some monsters are great at detecting traps */
#if 0 /* DGDGDGDG */
        if (r_ptr->flags2 & RF2_NOTICE_TRAP) smartness += 20;
#endif 
	/* Some monsters have already noticed one of out traps */
        if (m_ptr->smart & SM_NOTE_TRAP) smartness += 20;

	/* Stupid monsters are no good at detecting traps */
	if (r_ptr->flags2 & (RF2_STUPID | RF2_EMPTY_MIND)) smartness = -150;
				
	/* Check if the monster notices the trap */
	if (randint(300) > (difficulty - smartness + 150)) notice = TRUE;
					
	/* Disarm check */
	if (notice)
	{
	        /* The next traps will be easier to spot! */
                m_ptr->smart |= SM_NOTE_TRAP;
	        
	        /* Tell the player about it */
#if 0 /* DGDGDGDG */
                if (m_ptr->ml) l_ptr->r_flags2 |= (RF2_NOTICE_TRAP & r_ptr->flags2);
#endif         
		/* Get trap disarming difficulty */
		difficulty = (kit_o_ptr->ac + kit_o_ptr->to_a);
		
		/* Get monster disarming ability */
		/* Higher level monsters are better */
		smartness = r_ptr->level / 5;
		
		/* Some monsters are great at disarming */
#if 0 /* DGDGDGDG */
                if (r_ptr->flags2 & RF2_DISARM_TRAP) smartness += 20;
#endif         
                /* After disarming one trap, the next is easier */
#if 0 /* DGDGDGDG */
                if (m_ptr->status & STATUS_DISARM_TRAP) smartness += 20;
#endif         
		/* Smart monsters are better at disarming */
		if (r_ptr->flags2 & RF2_SMART) smartness *= 2;
				
		/* Stupid monsters never disarm traps */
		if (r_ptr->flags2 & RF2_STUPID) smartness = -150;		
		
		/* Nonsmart animals never disarm traps */
		if ((r_ptr->flags3 & RF3_ANIMAL) && !(r_ptr->flags2 & RF2_SMART)) smartness = -150;
		
		/* Check if the monster disarms the trap */
		if (randint(120) > (difficulty - smartness + 80)) disarm = TRUE;
	}
	
	/* If disarmed, remove the trap and print a message */
	if (disarm)
	{
		remove = TRUE;
		
                /* Next time disarming will be easier */
#if 0 /* DGDGDGDG */
                m_ptr->status |= STATUS_DISARM_TRAP;
#endif                         
		if (m_ptr->ml) 
		{
			/* Get the name */
			monster_desc(m_name, m_idx, 0);
			
		        /* Tell the player about it */
#if 0 /* DGDGDGDG */
                        l_ptr->r_flags2 |= (RF2_DISARM_TRAP & r_ptr->flags2);
#endif                 
			/* Print a message */
			msg_format("%^s disarms a trap!", m_name);
		}
	}
	
	/* Otherwise, activate the trap! */
	else
	{
		/* Message for visible monster */		
		if (m_ptr->ml) 
		{
			/* Get the name */
			monster_desc(m_name, m_idx, 0);
			
			/* Print a message */
			msg_format("%^s sets off a trap!", m_name);
		}
		else
		{
			/* No message if monster isn't visible ? */
		}
				
		/* Next time be more careful */
                if (randint(100) < 80) m_ptr->smart |= SM_NOTE_TRAP;
		
		/* Actually activate the trap */
		switch (kit_o_ptr->sval)
		{
                        case SV_TRAPKIT_BOW:
                        case SV_TRAPKIT_XBOW:
                        case SV_TRAPKIT_SLING:
			{
				/* Get number of shots */
				shots = 1;
                                if (f3 & TR3_XTRA_SHOTS) shots += kit_o_ptr->pval;
				if (shots <= 0) shots = 1;
				if (shots > load_o_ptr->number) shots = load_o_ptr->number;
				
				while (shots-- && !dead)
				{
					/* Total base damage */
					dam = damroll(load_o_ptr->dd, load_o_ptr->ds) + load_o_ptr->to_d + kit_o_ptr->to_d;
					
					/* Total hit probability */
					chance = (kit_o_ptr->to_h + load_o_ptr->to_h + 20) * BTH_PLUS_ADJ;
					
					/* Damage multiplier */
                                        if (kit_o_ptr->sval == SV_TRAPKIT_BOW) mul = 3;
                                        if (kit_o_ptr->sval == SV_TRAPKIT_XBOW) mul = 4;
                                        if (kit_o_ptr->sval == SV_TRAPKIT_SLING) mul = 2;
                                        if (f3 & TR3_XTRA_MIGHT) mul += kit_o_ptr->pval;
					if (mul < 0) mul = 0;
					
					/* Multiply damage */
					dam *= mul;
					
					/* Check if we hit the monster */
					if (test_hit_fire(chance, r_ptr->ac, TRUE))
					{
		                               /* Assume a default death */
		                                cptr note_dies = " dies.";
	
	                         	       /* Some monsters get "destroyed" */
		                                if ((r_ptr->flags3 & (RF3_DEMON)) ||
	        	                            (r_ptr->flags3 & (RF3_UNDEAD)) ||
                        	                    (r_ptr->flags2 & (RF2_STUPID)) ||
                                	            (strchr("Evg", r_ptr->d_char)))
                                        	{
                                                	/* Special note at death */
                                                	note_dies = " is destroyed.";
                                        	}
                                        	
                                        	/* Message if visible */
                                        	if (m_ptr->ml)
                                        	{
                                        		/* describe the monster (again, just in case :-) */
                                        		monster_desc(m_name, m_idx, 0);
                                        		
        						/* Print a message */
							msg_format("%^s is hit by a missile.", m_name);           		
	                                	}
                                        	
                                        	/* Apply slays, brand, critical hits */
                                                dam = tot_dam_aux(load_o_ptr, dam, m_ptr, &special);
						dam = critical_shot(load_o_ptr->weight, load_o_ptr->to_h, dam);
						
						/* No negative damage */
						if (dam < 0) dam = 0;
					
						/* Hit the monster, check for death */
						if (mon_take_hit(m_idx, dam, &fear, note_dies))
						{
							/* Dead monster */
							dead = TRUE;						
						}
	
						/* No death */
						else
						{
							/* Message */
							message_pain(m_idx, dam);

                                                        if (special) attack_special(m_ptr, special, dam);

							/* Take note */
							if (fear && m_ptr->ml)
							{
								/* Message */
                                                                msg_format("%^s flees in terror!", m_name);
							}
						}
						
					}
					
					/* Copy and decrease ammo */
		                        object_copy(j_ptr, load_o_ptr);
		                        
		                        j_ptr->number = 1;
		                        
		                        load_o_ptr->number--;
		                        	                        
					if (load_o_ptr->number <= 0)
					{
						remove = TRUE;
						delete_object_idx(kit_o_ptr->next_o_idx);
						kit_o_ptr->next_o_idx = 0;
					}
					
					/* Drop (or break) near that location */
					drop_near(j_ptr, breakage_chance(j_ptr), my, mx);				
					
				}
				
				break;
			}
			
                        case SV_TRAPKIT_POTION:
			{
				/* Get number of shots */
				shots = 1;
                                if (f3 & TR3_XTRA_SHOTS) shots += kit_o_ptr->pval;
				if (shots <= 0) shots = 1;
				if (shots > load_o_ptr->number) shots = load_o_ptr->number;
				
				while (shots-- && !dead)
				{
					                                        	
                                       	/* Message if visible */
                                       	if (m_ptr->ml)
                                       	{
                                       		/* describe the monster (again, just in case :-) */
                                       		monster_desc(m_name, m_idx, 0);
                                       		
        					/* Print a message */
						msg_format("%^s is hit by fumes.", m_name);
					}
					
					/* Get the potion effect */
                                        dead = mon_hit_trap_aux_potion(m_idx, load_o_ptr);

					/* Copy and decrease ammo */
		                        object_copy(j_ptr, load_o_ptr);
		                        
		                        j_ptr->number = 1;
		                        
		                        load_o_ptr->number--;
		                        	                        
					if (load_o_ptr->number <= 0)
					{
						remove = TRUE;
						delete_object_idx(kit_o_ptr->next_o_idx);
						kit_o_ptr->next_o_idx = 0;
					}
			        }
			        
				break;
			}
			
                        case SV_TRAPKIT_SCROLL:
			{
				/* Get number of shots */
				shots = 1;
                                if (f3 & TR3_XTRA_SHOTS) shots += kit_o_ptr->pval;
				if (shots <= 0) shots = 1;
				if (shots > load_o_ptr->number) shots = load_o_ptr->number;
				
				while (shots-- && !dead)
				{
					                                        	
                                       	/* Message if visible */
                                       	if (m_ptr->ml)
                                       	{
                                       		/* describe the monster (again, just in case :-) */
                                       		monster_desc(m_name, m_idx, 0);
                                       		
        					/* Print a message */
						msg_format("%^s activates a spell!", m_name);
					}
					
					/* Get the potion effect */
					dead = mon_hit_trap_aux_scroll(m_idx, load_o_ptr->sval);

					/* Copy and decrease ammo */
		                        object_copy(j_ptr, load_o_ptr);
		                        
		                        j_ptr->number = 1;
		                        
		                        load_o_ptr->number--;
		                        	                        
					if (load_o_ptr->number <= 0)
					{
						remove = TRUE;
						delete_object_idx(kit_o_ptr->next_o_idx);
						kit_o_ptr->next_o_idx = 0;
					}
			        }
			        
				break;
			}
			
                        case SV_TRAPKIT_DEVICE:
			{
				/* Get number of shots */
				shots = 1;
				if (load_o_ptr->tval == TV_ROD)
				{
					if (load_o_ptr->pval) shots = 0;
				}
				else
				{
                                        if (f3 & TR3_XTRA_SHOTS) shots += kit_o_ptr->pval;
					if (shots <= 0) shots = 1;
					if (shots > load_o_ptr->pval) shots = load_o_ptr->pval;
				}				
				while (shots-- && !dead)
				{
#if 0					                                        	
                                       	/* Message if visible */
                                       	if (m_ptr->ml)
                                       	{
                                       		/* describe the monster (again, just in case :-) */
                                       		monster_desc(m_name, m_idx, 0);
                                       		
        					/* Print a message */
						msg_format("%^s is hit by some magic.", m_name);
					}
#endif					
					/* Get the effect effect */
					switch(load_o_ptr->tval)
					{
						case TV_ROD:
							dead = mon_hit_trap_aux_rod(m_idx, load_o_ptr);
					 		break;
					        case TV_WAND:
							dead = mon_hit_trap_aux_wand(m_idx, load_o_ptr->sval);
					 		break;
					        case TV_STAFF:
							dead = mon_hit_trap_aux_staff(m_idx, load_o_ptr->sval);
					 		break;					        
                                        }
					/* Decrease charges */
					if (load_o_ptr->tval != TV_ROD)
					{
						load_o_ptr->pval--;
					}
			        }
			        
				break;
			}
			
			default:
				msg_print("oops! nonexisting trap!");
	
		}		
		
		/* Non-automatic traps are removed */
		if (!(f2 & (TRAP2_AUTOMATIC_5 | TRAP2_AUTOMATIC_99)))
		{
			remove = TRUE;	
		}
		else if (f2 & TRAP2_AUTOMATIC_5) remove = (randint(5) == 1);
		
	}

	/* Special trap effect -- teleport to */
        if ((f2 & TRAP2_TELEPORT_TO) && (!disarm) && (!dead))
	{
                teleport_monster_to(m_idx, py, px);
	}
					
	/* Remove the trap if inactive now */	
	if (remove) cave_set_feat(my, mx, FEAT_FLOOR);
	
	/* did it die? */
	return (dead);
}
#endif	// 0
