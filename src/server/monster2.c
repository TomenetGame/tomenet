/* File: monster.c */

/* Purpose: misc code for monsters */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define SERVER

#include "angband.h"


/* Monster gain a few levels ? */
void monster_check_experience(int m_idx, bool silent)
{
        monster_type    *m_ptr = &m_list[m_idx];
        monster_race    *r_ptr = R_INFO(m_ptr);

	/* Gain levels while possible */
        while ((m_ptr->level < MONSTER_LEVEL_MAX) &&
               (m_ptr->exp >= (MONSTER_EXP(m_ptr->level + 1))))
	{
		/* Gain a level */
                m_ptr->level++;

                /* Gain hp */
                if (magik(90))
                {
                        m_ptr->maxhp += r_ptr->hside;
                        m_ptr->hp += r_ptr->hside;
                }

                /* Gain speed */
                if (magik(40))
                {
                        int speed = randint(3);
                        m_ptr->speed += speed;
                        m_ptr->mspeed += speed;
                }

                /* Gain ac */
                if (magik(50))
                {
                        m_ptr->ac += (r_ptr->ac / 10)?r_ptr->ac / 10:1;
                }

                /* Gain melee power */
                if (magik(50))
                {
                        int i = rand_int(4), try = 20;

                        while ((try--) && !m_ptr->blow[i].d_dice) i = rand_int(4);

                        m_ptr->blow[i].d_dice++;
                }
        }
}

/* Monster gain some xp */
void monster_gain_exp(int m_idx, u32b exp, bool silent)
{
        monster_type *m_ptr = &m_list[m_idx];

        m_ptr->exp += exp;

        monster_check_experience(m_idx, silent);
}


/*
 * Delete a monster by index.
 *
 * This function causes the given monster to cease to exist for
 * all intents and purposes.  The monster record is left in place
 * but the record is wiped, marking it as "dead" (no race index)
 * so that it can be "skipped" when scanning the monster array,
 * and "excised" from the "m_fast" array when needed.
 *
 * Thus, anyone who makes direct reference to the "m_list[]" array
 * using monster indexes that may have become invalid should be sure
 * to verify that the "r_idx" field is non-zero.  All references
 * to "m_list[c_ptr->m_idx]" are guaranteed to be valid, see below.
 */
void delete_monster_idx(int i)
{
	int x, y, Depth, Ind;

	monster_type *m_ptr = &m_list[i];

        monster_race *r_ptr = R_INFO(m_ptr);

	/* Get location */
	y = m_ptr->fy;
	x = m_ptr->fx;
	Depth = m_ptr->dun_depth;


	/* Hack -- Reduce the racial counter */
	r_ptr->cur_num--;

	/* Hack -- count the number of "reproducers" */
	if (r_ptr->flags2 & RF2_MULTIPLY) num_repro--;


	/* Remove him from everybody's view */
	for (Ind = 1; Ind < NumPlayers + 1; Ind++)
	{
		/* Skip this player if he isn't playing */
		if (Players[Ind]->conn == NOT_CONNECTED) continue;

		Players[Ind]->mon_vis[i] = FALSE;
		Players[Ind]->mon_los[i] = FALSE;

		/* Hack -- remove target monster */
		if (i == Players[Ind]->target_who) Players[Ind]->target_who = 0;

		/* Hack -- remove tracked monster */
		if (i == Players[Ind]->health_who) health_track(Ind, 0);
	}


	/* Monster is gone */
	/* Make sure the level is allocated, it won't be if we are
	 * clearing an abandoned wilderness level of monsters
	 */
	if (cave[Depth])
		cave[Depth][y][x].m_idx = 0;

	/* Visual update */
	everyone_lite_spot(Depth, y, x);

	/* Wipe the Monster */
        FREE(m_ptr->r_ptr, monster_race);
	WIPE(m_ptr, monster_type);
}


/*
 * Delete the monster, if any, at a given location
 */
void delete_monster(int Depth, int y, int x)
{
	cave_type *c_ptr;

	/* Paranoia */
	if (!in_bounds(Depth, y, x)) return;

	if (cave[Depth]){
		/* Check the grid */
		c_ptr = &cave[Depth][y][x];
		/* Delete the monster (if any) */
		if (c_ptr->m_idx > 0) delete_monster_idx(c_ptr->m_idx);
	}
	else{				/* still delete the monster, just slower method */
		int i;
		for(i=0;i<m_max;i++){
			monster_type *m_ptr=&m_list[i];
			if(m_ptr->r_idx && Depth==m_ptr->dun_depth){
				if(y==m_ptr->fy && x==m_ptr->fx)
					delete_monster_idx(i);
			}
		}
	}
}


/*
 * Compact and Reorder the monster list
 *
 * This function can be very dangerous, use with caution!
 *
 * When actually "compacting" monsters, we base the saving throw
 * on a combination of monster level, distance from player, and
 * current "desperation".
 *
 * After "compacting" (if needed), we "reorder" the monsters into a more
 * compact order, and we reset the allocation info, and the "live" array.
 */
void compact_monsters(int size)
{
	int		i, num, cnt, Ind;

	int		cur_lev, cur_dis, chance;


	/* Message (only if compacting) */
	if (size) s_printf("Compacting monsters...\n");


	/* Compact at least 'size' objects */
	for (num = 0, cnt = 1; num < size; cnt++)
	{
		/* Get more vicious each iteration */
		cur_lev = 5 * cnt;

		/* Get closer each iteration */
		cur_dis = 5 * (20 - cnt);

		/* Check all the monsters */
		for (i = 1; i < m_max; i++)
		{
			monster_type *m_ptr = &m_list[i];

                        monster_race *r_ptr = R_INFO(m_ptr);

			/* Paranoia -- skip "dead" monsters */
			if (!m_ptr->r_idx) continue;

			/* Hack -- High level monsters start out "immune" */
			if (r_ptr->level > cur_lev) continue;

			/* Ignore nearby monsters */
			if ((cur_dis > 0) && (m_ptr->cdis < cur_dis)) continue;

			/* Saving throw chance */
			chance = 90;

			/* Only compact "Quest" Monsters in emergencies */
			if ((r_ptr->flags1 & RF1_QUESTOR) && (cnt < 1000)) chance = 100;

			/* Try not to compact Unique Monsters */
			if (r_ptr->flags1 & RF1_UNIQUE) chance = 99;

			/* Monsters in town don't have much of a chance */
			if (!m_ptr->dun_depth) chance = 70;

			/* All monsters get a saving throw */
			if (rand_int(100) < chance) continue;

			/* Delete the monster */
			delete_monster_idx(i);

			/* Count the monster */
			num++;
		}
	}


	/* Excise dead monsters (backwards!) */
	for (i = m_max - 1; i >= 1; i--)
	{
		/* Get the i'th monster */
		monster_type *m_ptr = &m_list[i];

		/* Skip real monsters */
		if (m_ptr->r_idx) continue;

		/* One less monster */
		m_max--;

		/* Reorder */
		if (i != m_max)
		{
			int ny = m_list[m_max].fy;
			int nx = m_list[m_max].fx;
			int Depth = m_list[m_max].dun_depth;

			/* Update the cave */
			/* Hack -- make sure the level is allocated, as in the wilderness
			   it sometimes will not be */
			if (cave[Depth])			
				cave[Depth][ny][nx].m_idx = i;

			/* Structure copy */
			m_list[i] = m_list[m_max];

			/* Copy the visibility and los flags for the players */
			for (Ind = 1; Ind < NumPlayers + 1; Ind++)
			{
				if (Players[Ind]->conn == NOT_CONNECTED) continue;

				Players[Ind]->mon_vis[i] = Players[Ind]->mon_vis[m_max];
				Players[Ind]->mon_los[i] = Players[Ind]->mon_los[m_max];

				/* Hack -- Update the target */
				if (Players[Ind]->target_who == (int)(m_max)) Players[Ind]->target_who = i;

				/* Hack -- Update the health bar */
				if (Players[Ind]->health_who == (int)(m_max)) health_track(Ind, i);
			}

			/* Wipe the hole */
			WIPE(&m_list[m_max], monster_type);
		}
	}

	/* Reset "m_nxt" */
	m_nxt = m_max;


	/* Reset "m_top" */
	m_top = 0;

	/* Collect "live" monsters */
	for (i = 0; i < m_max; i++)
	{
		/* Collect indexes */
		m_fast[m_top++] = i;
	}
}


/*
 * Delete/Remove all the monsters when the player leaves the level
 *
 * This is an efficient method of simulating multiple calls to the
 * "delete_monster()" function, with no visual effects.
 *
 * Note that this only deletes monsters that on the specified depth
 */
void wipe_m_list(int Depth)
{
	int i;

#if 0
	/* Delete all the monsters */
	for (i = m_max - 1; i >= 1; i--)
	{
		monster_type *m_ptr = &m_list[i];

                monster_race *r_ptr = R_INFO(m_ptr);

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Mega-Hack -- preserve Unique's XXX XXX XXX */

		/* Hack -- Reduce the racial counter */
		r_ptr->cur_num--;

		/* Monster is gone */
		cave[m_ptr->fy][m_ptr->fx].m_idx = 0;

		/* Wipe the Monster */
		WIPE(m_ptr, monster_type);
	}

	/* Reset the monster array */
	m_nxt = m_max = 1;

	/* No live monsters */
	m_top = 0;

	/* Hack -- reset "reproducer" count */
	num_repro = 0;

	/* Hack -- no more target */
	target_who = 0;

	/* Hack -- no more tracking */
	health_track(0);
#endif

	/* Delete all the monsters */
	for (i = m_max - 1; i >= 1; i--)
	{
		monster_type *m_ptr = &m_list[i];

		if (m_ptr->dun_depth == Depth)
			delete_monster_idx(i);
	}

	/* Compact the monster list */
	compact_monsters(0);
}


/*
 * Acquires and returns the index of a "free" monster.
 *
 * This routine should almost never fail, but it *can* happen.
 *
 * Note that this function must maintain the special "m_fast"
 * array of indexes of "live" monsters.
 */
s16b m_pop(void)
{
	int i, n, k;


	/* Normal allocation */
	if (m_max < MAX_M_IDX)
	{
		/* Access the next hole */
		i = m_max;

		/* Expand the array */
		m_max++;

		/* Update "m_fast" */
		m_fast[m_top++] = i;

		/* Return the index */
		return (i);
	}


	/* Check for some space */
	for (n = 1; n < MAX_M_IDX; n++)
	{
		/* Get next space */
		i = m_nxt;

		/* Advance (and wrap) the "next" pointer */
		if (++m_nxt >= MAX_M_IDX) m_nxt = 1;

		/* Skip monsters in use */
		if (m_list[i].r_idx) continue;

		/* Verify space XXX XXX */
		if (m_top >= MAX_M_IDX) continue;

		/* Verify not allocated */
		for (k = 0; k < m_top; k++)
		{
			/* Hack -- Prevent errors */
			if (m_fast[k] == i) i = 0;
		}

		/* Oops XXX XXX */
		if (!i) continue;

		/* Update "m_fast" */
		m_fast[m_top++] = i;

		/* Use this monster */
		return (i);
	}


	/* Warn the player */
	if (server_dungeon) s_printf("Too many monsters!");

	/* Try not to crash */
	return (0);
}




/*
 * Apply a "monster restriction function" to the "monster allocation table"
 */
errr get_mon_num_prep(void)
{
	int i;

	/* Scan the allocation table */
	for (i = 0; i < alloc_race_size; i++)
	{
		/* Get the entry */
		alloc_entry *entry = &alloc_race_table[i];

		/* Accept monsters which pass the restriction, if any */
		if (!get_mon_num_hook || (*get_mon_num_hook)(entry->index))
		{
			/* Accept this monster */
			entry->prob2 = entry->prob1;
		}

		/* Do not use this monster */
		else
		{
			/* Decline this monster */
			entry->prob2 = 0;
		}
	}

	/* Success */
	return (0);
}



/*
 * Choose a monster race that seems "appropriate" to the given level
 *
 * This function uses the "prob2" field of the "monster allocation table",
 * and various local information, to calculate the "prob3" field of the
 * same table, which is then used to choose an "appropriate" monster, in
 * a relatively efficient manner.
 *
 * Note that "town" monsters will *only* be created in the town, and
 * "normal" monsters will *never* be created in the town, unless the
 * "level" is "modified", for example, by polymorph or summoning.
 *
 * There is a small chance (1/50) of "boosting" the given depth by
 * a small amount (up to four levels), except in the town.
 *
 * It is (slightly) more likely to acquire a monster of the given level
 * than one of a lower level.  This is done by choosing several monsters
 * appropriate to the given level and keeping the "hardest" one.
 *
 * Note that if no monsters are "appropriate", then this function will
 * fail, and return zero, but this should *almost* never happen.
 */

/* APD I am changing this to support "negative" levels, so we can have
 * wilderness only monsters.
 *
 * XXX I am temporarily disabling this, as it appears to be messing
 * things up.
 *
 * This function is kind of a mess right now.
 */

s16b get_mon_num(int level)
{
	int			i, j, p, d1 = 0, d2 = 0;

	int			r_idx;

	long		value, total;

	monster_race	*r_ptr;

	alloc_entry		*table = alloc_race_table;

        /* Warp level around */
        if (level > 100)
        {
                level = level - 100;
        }

	if (level > 0)
	{

		/* Occasional "nasty" monster */
		if (rand_int(NASTY_MON) == 0)
		{
			/* Pick a level bonus */
			d1 = level / 4 + 2;

			/* Boost the level */
			level += ((d1 < 5) ? d1 : 5);
		}

		/* Occasional "nasty" monster */
		if (rand_int(NASTY_MON) == 0)
		{
			/* Pick a level bonus */
			d2 = level / 4 + 2;

			/* Boost the level */
			level += ((d2 < 5) ? d2 : 5);
		}
	}	

	/* Boost the level -- but not in town square. 
	if (level)
	{
		level += ((d1 < 5) ? d1 : 5);
		level += ((d2 < 5) ? d2 : 5);
	} */

	/* Reset total */
	total = 0L;

	/* Process probabilities */
	for (i = 0; i < alloc_race_size; i++)
	{
		/* Monsters are sorted by depth */
		if (table[i].level > level) break;


		/* XXX disabling this 
		{
			 ceiling 
			if (table[i].level > -level) break;
			 floor 
			if (table[i].level < level) continue; 
		}
		*/

		/* Default */
		table[i].prob3 = 0;

		/* Hack -- No town monsters in the dungeon */
		if ((level > 0) && (table[i].level <= 0)) continue;

		/* Access the "r_idx" of the chosen monster */
		r_idx = table[i].index;

		/* Access the actual race */
		r_ptr = &r_info[r_idx];

		/* Hack -- "unique" monsters must be "unique" */
		/* For each players -- DG */
/*		if ((r_ptr->flags1 & RF1_UNIQUE) &&
		    (unique_allowed(r_idx, level))
		{
			continue;
		}*/

		/* Depth Monsters never appear out of depth */
		/* FIXME: This might cause FORCE_DEPTH monsters to appear out of depth */
		if ((r_ptr->flags1 & RF1_FORCE_DEPTH) && (r_ptr->level > level))
		{
			continue;
		}

		/* Accept */
		table[i].prob3 = table[i].prob2;

		/* Total */
		total += table[i].prob3;
	}

	/* No legal monsters */
	if (total <= 0) return (0);


	/* Pick a monster */
	value = rand_int(total);

	/* Find the monster */
	for (i = 0; i < alloc_race_size; i++)
	{
		/* Found the entry */
		if (value < table[i].prob3) break;

		/* Decrement */
		value = value - table[i].prob3;
	}


	/* Power boost */
	p = rand_int(100);

	/* Try for a "harder" monster once (50%) or twice (10%) */
	if (p < 60)
	{
		/* Save old */
		j = i;

		/* Pick a monster */
		value = rand_int(total);

		/* Find the monster */
		for (i = 0; i < alloc_race_size; i++)
		{
			/* Found the entry */
			if (value < table[i].prob3) break;

			/* Decrement */
			value = value - table[i].prob3;
		}

		/* Keep the "best" one */
		if (abs(table[i].level) < abs(table[j].level)) i = j;
	}

	/* Try for a "harder" monster twice (10%) */
	if (p < 10)
	{
		/* Save old */
		j = i;

		/* Pick a monster */
		value = rand_int(total);

		/* Find the monster */
		for (i = 0; i < alloc_race_size; i++)
		{
			/* Found the entry */
			if (value < table[i].prob3) break;

			/* Decrement */
			value = value - table[i].prob3;
		}

		/* Keep the "best" one */
		if (abs(table[i].level) < abs(table[j].level)) i = j;
	}


	/* Result */
	return (table[i].index);
}





/*
 * Build a string describing a monster in some way.
 *
 * We can correctly describe monsters based on their visibility.
 * We can force all monsters to be treated as visible or invisible.
 * We can build nominatives, objectives, possessives, or reflexives.
 * We can selectively pronominalize hidden, visible, or all monsters.
 * We can use definite or indefinite descriptions for hidden monsters.
 * We can use definite or indefinite descriptions for visible monsters.
 *
 * Pronominalization involves the gender whenever possible and allowed,
 * so that by cleverly requesting pronominalization / visibility, you
 * can get messages like "You hit someone.  She screams in agony!".
 *
 * Reflexives are acquired by requesting Objective plus Possessive.
 *
 * If no m_ptr arg is given (?), the monster is assumed to be hidden,
 * unless the "Assume Visible" mode is requested.
 *
 * If no r_ptr arg is given, it is extracted from m_ptr and r_info
 * If neither m_ptr nor r_ptr is given, the monster is assumed to
 * be neuter, singular, and hidden (unless "Assume Visible" is set),
 * in which case you may be in trouble... :-)
 *
 * I am assuming that no monster name is more than 70 characters long,
 * so that "char desc[80];" is sufficiently large for any result.
 *
 * Mode Flags:
 *   0x01 --> Objective (or Reflexive)
 *   0x02 --> Possessive (or Reflexive)
 *   0x04 --> Use indefinites for hidden monsters ("something")
 *   0x08 --> Use indefinites for visible monsters ("a kobold")
 *   0x10 --> Pronominalize hidden monsters
 *   0x20 --> Pronominalize visible monsters
 *   0x40 --> Assume the monster is hidden
 *   0x80 --> Assume the monster is visible
 *
 * Useful Modes:
 *   0x00 --> Full nominative name ("the kobold") or "it"
 *   0x04 --> Full nominative name ("the kobold") or "something"
 *   0x80 --> Genocide resistance name ("the kobold")
 *   0x88 --> Killing name ("a kobold")
 *   0x22 --> Possessive, genderized if visable ("his") or "its"
 *   0x23 --> Reflexive, genderized if visable ("himself") or "itself"
 */
void monster_desc(int Ind, char *desc, int m_idx, int mode)
{
	player_type *p_ptr;

	cptr		res;

        monster_type    *m_ptr = &m_list[m_idx];
        monster_race    *r_ptr = R_INFO(m_ptr);

        cptr            name = r_name_get(m_ptr);

	bool		seen, pron;

	/* Check for bad player number */
	if (Ind > 0)
	{
		p_ptr = Players[Ind];
	
		/* Can we "see" it (exists + forced, or visible + not unforced) */
		seen = (m_ptr && ((mode & 0x80) || (!(mode & 0x40) && p_ptr->mon_vis[m_idx])));
        }
	else
	{
		seen = (m_ptr && ((mode & 0x80) || (!(mode & 0x40))));
	}

	/* Sexed Pronouns (seen and allowed, or unseen and allowed) */
	pron = (m_ptr && ((seen && (mode & 0x20)) || (!seen && (mode & 0x10))));


	/* First, try using pronouns, or describing hidden monsters */
	if (!seen || pron)
	{
		/* an encoding of the monster "sex" */
		int kind = 0x00;

		/* Extract the gender (if applicable) */
		if (r_ptr->flags1 & RF1_FEMALE) kind = 0x20;
		else if (r_ptr->flags1 & RF1_MALE) kind = 0x10;

		/* Ignore the gender (if desired) */
		if (!m_ptr || !pron) kind = 0x00;


		/* Assume simple result */
		res = "it";

		/* Brute force: split on the possibilities */
		switch (kind + (mode & 0x07))
		{
			/* Neuter, or unknown */
			case 0x00: res = "it"; break;
			case 0x01: res = "it"; break;
			case 0x02: res = "its"; break;
			case 0x03: res = "itself"; break;
			case 0x04: res = "something"; break;
			case 0x05: res = "something"; break;
			case 0x06: res = "something's"; break;
			case 0x07: res = "itself"; break;

			/* Male (assume human if vague) */
			case 0x10: res = "he"; break;
			case 0x11: res = "him"; break;
			case 0x12: res = "his"; break;
			case 0x13: res = "himself"; break;
			case 0x14: res = "someone"; break;
			case 0x15: res = "someone"; break;
			case 0x16: res = "someone's"; break;
			case 0x17: res = "himself"; break;

			/* Female (assume human if vague) */
			case 0x20: res = "she"; break;
			case 0x21: res = "her"; break;
			case 0x22: res = "her"; break;
			case 0x23: res = "herself"; break;
			case 0x24: res = "someone"; break;
			case 0x25: res = "someone"; break;
			case 0x26: res = "someone's"; break;
			case 0x27: res = "herself"; break;
		}

		/* Copy the result */
		(void)strcpy(desc, res);
	}


	/* Handle visible monsters, "reflexive" request */
	else if ((mode & 0x02) && (mode & 0x01))
	{
		/* The monster is visible, so use its gender */
		if (r_ptr->flags1 & RF1_FEMALE) strcpy(desc, "herself");
		else if (r_ptr->flags1 & RF1_MALE) strcpy(desc, "himself");
		else strcpy(desc, "itself");
	}


	/* Handle all other visible monster requests */
	else
	{
		/* It could be a Unique */
		if (r_ptr->flags1 & RF1_UNIQUE)
		{
			/* Start with the name (thus nominative and objective) */
			(void)strcpy(desc, name);
		}

		/* It could be an indefinite monster */
		else if (mode & 0x08)
		{
			/* XXX Check plurality for "some" */

			/* Indefinite monsters need an indefinite article */
			(void)strcpy(desc, is_a_vowel(name[0]) ? "an " : "a ");
			(void)strcat(desc, name);
		}

		/* It could be a normal, definite, monster */
		else
		{
			/* Definite monsters need a definite article */
			(void)strcpy(desc, "the ");
			(void)strcat(desc, name);
		}

		/* Handle the Possessive as a special afterthought */
		if (mode & 0x02)
		{
			/* XXX Check for trailing "s" */

			/* Simply append "apostrophe" and "s" */
			(void)strcat(desc, "'s");
		}
	}
}




/*
 * Learn about a monster (by "probing" it)
 */
void lore_do_probe(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];

        monster_race *r_ptr = R_INFO(m_ptr);

	/* Hack -- Memorize some flags */
	r_ptr->r_flags1 = r_ptr->flags1;
	r_ptr->r_flags2 = r_ptr->flags2;
	r_ptr->r_flags3 = r_ptr->flags3;

	/* Window stuff */
	/*p_ptr->window |= (PW_MONSTER);*/
}


/*
 * Take note that the given monster just dropped some treasure
 *
 * Note that learning the "GOOD"/"GREAT" flags gives information
 * about the treasure (even when the monster is killed for the first
 * time, such as uniques, and the treasure has not been examined yet).
 *
 * This "indirect" method is used to prevent the player from learning
 * exactly how much treasure a monster can drop from observing only
 * a single example of a drop.  This method actually observes how much
 * gold and items are dropped, and remembers that information to be
 * described later by the monster recall code.
 */
void lore_treasure(int m_idx, int num_item, int num_gold)
{
	monster_type *m_ptr = &m_list[m_idx];

        monster_race *r_ptr = R_INFO(m_ptr);

	/* Note the number of things dropped */
	if (num_item > r_ptr->r_drop_item) r_ptr->r_drop_item = num_item;
	if (num_gold > r_ptr->r_drop_gold) r_ptr->r_drop_gold = num_gold;

	/* Hack -- memorize the good/great flags */
	if (r_ptr->flags1 & RF1_DROP_GOOD) r_ptr->r_flags1 |= RF1_DROP_GOOD;
	if (r_ptr->flags1 & RF1_DROP_GREAT) r_ptr->r_flags1 |= RF1_DROP_GREAT;
}



/*
 * This function updates the monster record of the given monster
 *
 * This involves extracting the distance to the player, checking
 * for visibility (natural, infravision, see-invis, telepathy),
 * updating the monster visibility flag, redrawing or erasing the
 * monster when the visibility changes, and taking note of any
 * "visual" features of the monster (cold-blooded, invisible, etc).
 *
 * The only monster fields that are changed here are "cdis" (the
 * distance from the player), "los" (clearly visible to player),
 * and "ml" (visible to the player in any way).
 *
 * There are a few cases where the calling routine knows that the
 * distance from the player to the monster has not changed, and so
 * we have a special parameter to request distance computation.
 * This lets many calls to this function run very quickly.
 *
 * Note that every time a monster moves, we must call this function
 * for that monster, and update distance.  Note that every time the
 * player moves, we must call this function for every monster, and
 * update distance.  Note that every time the player "state" changes
 * in certain ways (including "blindness", "infravision", "telepathy",
 * and "see invisible"), we must call this function for every monster.
 *
 * The routines that actually move the monsters call this routine
 * directly, and the ones that move the player, or notice changes
 * in the player state, call "update_monsters()".
 *
 * Routines that change the "illumination" of grids must also call
 * this function, since the "visibility" of some monsters may be
 * based on the illumination of their grid.
 *
 * Note that this function is called once per monster every time the
 * player moves, so it is important to optimize it for monsters which
 * are far away.  Note the optimization which skips monsters which
 * are far away and were completely invisible last turn.
 *
 * Note the optimized "inline" version of the "distance()" function.
 *
 * Note that only monsters on the current panel can be "visible",
 * and then only if they are (1) in line of sight and illuminated
 * by light or infravision, or (2) nearby and detected by telepathy.
 *
 * The player can choose to be disturbed by several things, including
 * "disturb_move" (monster which is viewable moves in some way), and
 * "disturb_near" (monster which is "easily" viewable moves in some
 * way).  Note that "moves" includes "appears" and "disappears".
 */
void update_mon(int m_idx, bool dist)
{
	monster_type *m_ptr = &m_list[m_idx];

        monster_race *r_ptr = R_INFO(m_ptr);

	player_type *p_ptr;

	/* The current monster location */
	int fy = m_ptr->fy;
	int fx = m_ptr->fx;

	int Depth = m_ptr->dun_depth;

	int Ind = m_ptr->closest_player;

	/* Seen at all */
	bool flag = FALSE;

	/* Seen by vision */
	bool easy = FALSE;

	/* Seen by telepathy */
	bool hard = FALSE;

	/* Various extra flags */
	bool do_empty_mind = FALSE;
	bool do_weird_mind = FALSE;
	bool do_invisible = FALSE;
	bool do_cold_blood = FALSE;

	/* Check for each player */
	for (Ind = 1; Ind < NumPlayers + 1; Ind++)
	{
		p_ptr = Players[Ind];

		/* Reset the flags */
		flag = easy = hard = FALSE;

		/* If he's not playing, skip him */
		if (p_ptr->conn == NOT_CONNECTED)
			continue;

		/* If he's not on this depth, skip him */
		if (p_ptr->dun_depth != Depth)
		{
			p_ptr->mon_vis[m_idx] = FALSE;
			p_ptr->mon_los[m_idx] = FALSE;
			continue;
		}

		/* If our wilderness level has been deallocated, stop here...
		 * we are "detatched" monsters.
		 */

		if (!cave[Depth]) continue;

		/* Calculate distance */
		if (dist)
		{
			int d, dy, dx;

			/* Distance components */
			dy = (p_ptr->py > fy) ? (p_ptr->py - fy) : (fy - p_ptr->py);
			dx = (p_ptr->px > fx) ? (p_ptr->px - fx) : (fx - p_ptr->px);

			/* Approximate distance */
			d = (dy > dx) ? (dy + (dx>>1)) : (dx + (dy>>1));

			/* Save the distance (in a byte) */
			m_ptr->cdis = (d < 255) ? d : 255;
		}


		/* Process "distant" monsters */
#if 0
		if (m_ptr->cdis > MAX_SIGHT)
		{
			/* Ignore unseen monsters */
			if (!p_ptr->mon_vis[m_idx]) return;
		}
#endif

		/* Process "nearby" monsters on the current "panel" */
		if (panel_contains(fy, fx))
		{
			cave_type *c_ptr = &cave[Depth][fy][fx];
			byte *w_ptr = &p_ptr->cave_flag[fy][fx];

			/* Normal line of sight, and player is not blind */
			if ((*w_ptr & CAVE_VIEW) && (!p_ptr->blind))
			{
				/* Use "infravision" */
				if (m_ptr->cdis <= (byte)(p_ptr->see_infra))
				{
					/* Infravision only works on "warm" creatures */
					/* Below, we will need to know that infravision failed */
					if (r_ptr->flags2 & RF2_COLD_BLOOD) do_cold_blood = TRUE;

					/* Infravision works */
					if (!do_cold_blood) easy = flag = TRUE;
				}

				/* Use "illumination" */
				if ((c_ptr->info & CAVE_LITE) || (c_ptr->info & CAVE_GLOW))
				{
					/* Take note of invisibility */
					if (r_ptr->flags2 & RF2_INVISIBLE) do_invisible = TRUE;

					/* Visible, or detectable, monsters get seen */
					if (!do_invisible || p_ptr->see_inv) easy = flag = TRUE;
				}
			}

			/* Telepathy can see all "nearby" monsters with "minds" */
			if (p_ptr->telepathy || (p_ptr->prace == RACE_DRIDER))
			{
                                bool see = FALSE;

                                if (p_ptr->mode == MODE_NORMAL) see = TRUE;
                                if ((p_ptr->mode == MODE_HELL) && (m_ptr->cdis < MAX_SIGHT)) see = TRUE;
                                if (!p_ptr->telepathy && (p_ptr->prace == RACE_DRIDER) && (m_ptr->cdis > (p_ptr->lev / 2))) see = FALSE;

                                if (see)
                                {
				/* Empty mind, no telepathy */
				if (r_ptr->flags2 & RF2_EMPTY_MIND)
				{
					do_empty_mind = TRUE;
				}
	
				/* Weird mind, occasional telepathy */
				else if (r_ptr->flags2 & RF2_WEIRD_MIND)
				{
					do_weird_mind = TRUE;
					if (rand_int(100) < 10) hard = flag = TRUE;
				}

				/* Normal mind, allow telepathy */
				else
				{
					hard = flag = TRUE;
				}

				/* Apply telepathy */
				if (hard)
				{
					/* Hack -- Memorize mental flags */
					if (r_ptr->flags2 & RF2_SMART) r_ptr->r_flags2 |= RF2_SMART;
					if (r_ptr->flags2 & RF2_STUPID) r_ptr->r_flags2 |= RF2_STUPID;
				}
                                }
			}

			/* Hack -- Wizards have "perfect telepathy" */
			/* if (p_ptr->wizard) flag = TRUE; */
			if (!strcmp(p_ptr->name, cfg_dungeon_master)) flag = TRUE;
		}


		/* The monster is now visible */
		if (flag)
		{
			/* It was previously unseen */
			if (!p_ptr->mon_vis[m_idx])
			{
				/* Mark as visible */
				p_ptr->mon_vis[m_idx] = TRUE;

				/* Draw the monster */
				lite_spot(Ind, fy, fx);

				/* Update health bar as needed */
				if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);

				/* Hack -- Count "fresh" sightings */
				if (r_ptr->r_sights < MAX_SHORT) r_ptr->r_sights++;

				/* Disturb on appearance */
				if(!m_list[m_idx].special)
					if (p_ptr->disturb_move) disturb(Ind, 1, 0);
			}

			/* Memorize various observable flags */
			if (do_empty_mind) r_ptr->r_flags2 |= RF2_EMPTY_MIND;
			if (do_weird_mind) r_ptr->r_flags2 |= RF2_WEIRD_MIND;
			if (do_cold_blood) r_ptr->r_flags2 |= RF2_COLD_BLOOD;
			if (do_invisible) r_ptr->r_flags2 |= RF2_INVISIBLE;

			/* Efficiency -- Notice multi-hued monsters */
			if (r_ptr->flags1 & RF1_ATTR_MULTI) scan_monsters = TRUE;
		}

		/* The monster is not visible */
		else
		{
			/* It was previously seen */
			if (p_ptr->mon_vis[m_idx])
			{
				/* Mark as not visible */
				p_ptr->mon_vis[m_idx] = FALSE;

				/* Erase the monster */
				lite_spot(Ind, fy, fx);

				/* Update health bar as needed */
				if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);

				/* Disturb on disappearance*/
				if(!m_list[m_idx].special)
					if (p_ptr->disturb_move) disturb(Ind, 1, 0);
			}
		}


		/* The monster is now easily visible */
		if (easy)
		{
			/* Change */
			if (!p_ptr->mon_los[m_idx])
			{
				/* Mark as easily visible */
				p_ptr->mon_los[m_idx] = TRUE;

				/* Disturb on appearance */
				if(!m_list[m_idx].special)
					if (p_ptr->disturb_near) disturb(Ind, 1, 0);
			}
		}

		/* The monster is not easily visible */
		else
		{
			/* Change */
			if (p_ptr->mon_los[m_idx])
			{
				/* Mark as not easily visible */
				p_ptr->mon_los[m_idx] = FALSE;

				/* Disturb on disappearance */
				if(!m_list[m_idx].special)
					if (p_ptr->disturb_near) disturb(Ind, 1, 0);
			}
		}
	}
}




/*
 * This function simply updates all the (non-dead) monsters (see above).
 */
void update_monsters(bool dist)
{
	int          i;

	/* Efficiency -- Clear multihued flag */
	scan_monsters = FALSE;

	/* Update each (live) monster */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Update the monster */
		update_mon(i, dist);
	}
}


/*
 * This function updates the visiblity flags for everyone who may see
 * this player.
 */
void update_player(int Ind)
{
	player_type *p_ptr, *q_ptr = Players[Ind];

	int i;

	/* Current player location */
	int py = q_ptr->py;
	int px = q_ptr->px;

	/* Distance */
	int dis;

	/* Seen at all */
	bool flag = FALSE;

	/* Seen by vision */
	bool easy = FALSE;

	/* Seen by telepathy */
	bool hard = FALSE;

	/* Check for every other player */
	for (i = 1; i <= NumPlayers; i++)
	{
		p_ptr = Players[i];

		/* Reset the flags */
		flag = easy = hard = FALSE;

		/* Skip disconnected players */
		if (p_ptr->conn == NOT_CONNECTED) continue;

		/* Skip players not on this depth */
		if (p_ptr->dun_depth != q_ptr->dun_depth) continue;

		/* Player can always see himself */
		if (Ind == i) continue;

		/* Compute distance */
		dis = distance(py, px, p_ptr->py, p_ptr->px);

		/* Process players on current panel */
		if (panel_contains(py, px))
		{
			cave_type *c_ptr = &cave[p_ptr->dun_depth][py][px];
			byte *w_ptr = &p_ptr->cave_flag[py][px];

			/* Normal line of sight, and player is not blind */
			
			/* -APD- this line needs to be seperated to support seeing party
			   members who are out of line of sight */
			/* if ((*w_ptr & CAVE_VIEW) && (!p_ptr->blind)) */
			
			if (!p_ptr->blind)			
			{
			if ((player_in_party(q_ptr->party, i)) && (q_ptr->party)) easy = flag = TRUE;
			
			if (*w_ptr & CAVE_VIEW) {
				/* Check infravision */
				if (dis <= (byte)(p_ptr->see_infra))
				{
					/* Visible */
					easy = flag = TRUE;
				}

				/* Check illumination */
				if ((c_ptr->info & CAVE_LITE) || (c_ptr->info & CAVE_GLOW))
				{
					/* Check for invisibility */
					if (!q_ptr->ghost || p_ptr->see_inv)
					{
						/* Visible */
						easy = flag = TRUE;
					}
				}
				
				} /* end yucky hack */
			}

			/* Telepathy can see all players */
			if (p_ptr->telepathy || (p_ptr->prace == RACE_DRIDER))
			{
			  bool see = FALSE;

			  if (p_ptr->mode == MODE_NORMAL) see = TRUE;
			  if ((p_ptr->mode == MODE_HELL) && (dis < MAX_SIGHT)) see = TRUE;
			  if (!p_ptr->telepathy && (p_ptr->prace == RACE_DRIDER) && (dis > (p_ptr->lev / 2))) see = FALSE;

			  if (see)
			    {
				/* Visible */
				hard = flag = TRUE;
			    }
			}
			
			/* hack -- dungeon masters are invisible */
			if (!strcmp(q_ptr->name,cfg_dungeon_master)) flag = FALSE;
			
			/* Can we see invisibvle players ? */
			if ((!p_ptr->see_inv || ((q_ptr->inventory[INVEN_OUTER].k_idx) && (q_ptr->inventory[INVEN_OUTER].tval == TV_CLOAK) && (q_ptr->inventory[INVEN_OUTER].sval == SV_SHADOW_CLOAK))) && q_ptr->invis)
			{
				if ((q_ptr->lev > p_ptr->lev) || (randint(p_ptr->lev) > (q_ptr->lev / 2)))
					flag = FALSE;
			}
		}

		/* Player is now visible */
		if (flag)
		{
			/* It was previously unseen */
			if (!p_ptr->play_vis[Ind])
			{
				/* Mark as visible */
				p_ptr->play_vis[Ind] = TRUE;

				/* Draw the player */
				lite_spot(i, py, px);

				/* Disturb on appearance */
				if (p_ptr->disturb_move && check_hostile(i, Ind))
				{
					/* Disturb */
					disturb(i, 1, 0);
				}
			}
		}

		/* The player is not visible */
		else
		{
			/* It was previously seen */
			if (p_ptr->play_vis[Ind])
			{
				/* Mark as not visible */
				p_ptr->play_vis[Ind] = FALSE;

				/* Erase the player */
				lite_spot(i, py, px);

				/* Disturb on disappearance */
				if (p_ptr->disturb_move && check_hostile(i, Ind))
				{
					/* Disturb */
					disturb(i, 1, 0);
				}
			}
		}

		/* The player is now easily visible */
		if (easy)
		{
			/* Change */
			if (!p_ptr->play_los[Ind])
			{
				/* Mark as easily visible */
				p_ptr->play_los[Ind] = TRUE;

				/* Disturb on appearance */
				if (p_ptr->disturb_near && check_hostile(i, Ind))
				{
					/* Disturb */
					disturb(i, 1, 0);
				}
			}
		}

		/* The player is not easily visible */
		else
		{
			/* Change */
			if (p_ptr->play_los[Ind])
			{
				/* Mark as not easily visible */
				p_ptr->play_los[Ind] = FALSE;

				/* Disturb on disappearance */
				if (p_ptr->disturb_near && check_hostile(i, Ind))
				{
					/* Disturb */
					disturb(i, 1, 0);
				}
			}
		}
	}
}

/*
 * This function simply updates all the players (see above).
 */
void update_players(void)
{
	int i;

	/* Update each player */
	for (i = 1; i <= NumPlayers; i++)
	{
		player_type *p_ptr = Players[i];

		/* Skip disconnected players */
		if (p_ptr->conn == NOT_CONNECTED) continue;

		/* Update the player */
		update_player(i);
	}
}


/* Scan all players on the level and see if at least one can find the unique */
bool allow_unique_level(int r_idx, int Depth)
{
	int i;
	
	for (i = 1; i <= NumPlayers; i++)
	{
		player_type *p_ptr = Players[i];

		/* Is the player on the level and did he killed the unique already ? */
		if (!p_ptr->r_killed[r_idx] && (p_ptr->dun_depth == Depth))
		{
			/* One is enough */
			return (TRUE);
		}
	}
	
	/* Yeah but we need at least ONE */
	return (FALSE);
}

/*
 * Attempt to place a monster of the given race at the given location.
 *
 * To give the player a sporting chance, any monster that appears in
 * line-of-sight and is extremely dangerous can be marked as
 * "FORCE_SLEEP", which will cause them to be placed with low energy,
 * which often (but not always) lets the player move before they do.
 *
 * This routine refuses to place out-of-depth "FORCE_DEPTH" monsters.
 *
 * XXX XXX XXX Use special "here" and "dead" flags for unique monsters,
 * remove old "cur_num" and "max_num" fields.
 *
 * XXX XXX XXX Actually, do something similar for artifacts, to simplify
 * the "preserve" mode, and to make the "what artifacts" flag more useful.
 */
static bool place_monster_one(int Depth, int y, int x, int r_idx, bool slp, bool clo)
{
        int                     i, Ind, j;

	cave_type		*c_ptr;

	monster_type	*m_ptr;

        monster_race    *r_ptr = &r_info[r_idx];

	char buf[80];

	/* Verify location */
	if (!in_bounds(Depth, y, x)) return (FALSE);

	/* Require empty space */
	if (!cave_empty_bold(Depth, y, x)) return (FALSE);

	/* Hack -- no creation on glyph of warding */
	if (cave[Depth][y][x].feat == FEAT_GLYPH) return (FALSE);

	/* Hack -- no creation in town inside house */
	if (!Depth && (cave[Depth][y][x].info & CAVE_ICKY)) return (FALSE);
	/* Hack -- or close to town in wilderness areas */
	if ((Depth<0 && Depth >-16) && (cave[Depth][y][x].info & CAVE_ICKY)) return (FALSE);


	/* Paranoia */
	if (!r_idx) return (FALSE);

	/* Paranoia */
	if (!r_ptr->name) return (FALSE);


	/* Hack -- "unique" monsters must be "unique" */
	if ((r_ptr->flags1 & RF1_UNIQUE) && ((!allow_unique_level(r_idx, Depth)) || (r_ptr->cur_num >= r_ptr->max_num)))
	{
		/* Cannot create */
		return (FALSE);
	}


	/* Depth monsters may NOT be created out of depth */
	if ((r_ptr->flags1 & RF1_FORCE_DEPTH) && (Depth < r_ptr->level))
	{
		/* Cannot create */
		return (FALSE);
	}


	/* Powerful monster */
	if (r_ptr->level > Depth)
	{
		/* Unique monsters */
		if (r_ptr->flags1 & RF1_UNIQUE)
		{
			/* Message for cheaters */
			/*if (cheat_hear) msg_format("Deep Unique (%s).", name);*/

			/* Boost rating by twice delta-depth */
			rating += (r_ptr->level - Depth) * 2;
		}

		/* Normal monsters */
		else
		{
			/* Message for cheaters */
			/*if (cheat_hear) msg_format("Deep Monster (%s).", name);*/

			/* Boost rating by delta-depth */
			rating += (r_ptr->level - Depth);
		}
	}

	/* Note the monster */
	else if (r_ptr->flags1 & RF1_UNIQUE)
	{
		/* Unique monsters induce message */
		/*if (cheat_hear) msg_format("Unique (%s).", name);*/
	}


	/* Access the location */
	c_ptr = &cave[Depth][y][x];

	/* Make a new monster */
	c_ptr->m_idx = m_pop();

	/* Mega-Hack -- catch "failure" */
	if (!c_ptr->m_idx) return (FALSE);


	/* Get a new monster record */
	m_ptr = &m_list[c_ptr->m_idx];

	/* Save the race */
	m_ptr->r_idx = r_idx;

	/* Place the monster at the location */
	m_ptr->fy = y;
	m_ptr->fx = x;
	m_ptr->dun_depth = Depth;

        m_ptr->special = FALSE;

	/* Hack -- Count the monsters on the level */
	r_ptr->cur_num++;


	/* Hack -- count the number of "reproducers" */
	if (r_ptr->flags2 & RF2_MULTIPLY) num_repro++;


	/* Assign maximal hitpoints */
	if (r_ptr->flags1 & RF1_FORCE_MAXHP)
	{
		m_ptr->maxhp = maxroll(r_ptr->hdice, r_ptr->hside);
	}
	else
	{
		m_ptr->maxhp = damroll(r_ptr->hdice, r_ptr->hside);
	}

	/* And start out fully healthy */
	m_ptr->hp = m_ptr->maxhp;


	/* Extract the monster base speed */
        m_ptr->speed = r_ptr->speed;
        m_ptr->mspeed = m_ptr->speed;

        /* Extract base ac and  other things */
        m_ptr->ac = r_ptr->ac;

        for (j = 0; j < 4; j++)
        {
                m_ptr->blow[j].effect = r_ptr->blow[j].effect;
                m_ptr->blow[j].method = r_ptr->blow[j].method;
                m_ptr->blow[j].d_dice = r_ptr->blow[j].d_dice;
                m_ptr->blow[j].d_side = r_ptr->blow[j].d_side;
        }
        m_ptr->level = r_ptr->level;
        m_ptr->exp = MONSTER_EXP(m_ptr->level);
        m_ptr->owner = 0;

	/* Hack -- small racial variety */
	if (!(r_ptr->flags1 & RF1_UNIQUE))
	{
		/* Allow some small variation per monster */
                i = extract_energy[m_ptr->speed] / 10;
		if (i) m_ptr->mspeed += rand_spread(0, i);
	}


	/* Give a random starting energy */
	m_ptr->energy = rand_int(100);

	/* Hack -- Reduce risk of "instant death by breath weapons" */
	if (r_ptr->flags1 & RF1_FORCE_SLEEP)
	{
		/* Start out with minimal energy */
		m_ptr->energy = rand_int(10);
	}


	/* No "damage" yet */
	m_ptr->stunned = 0;
	m_ptr->confused = 0;
	m_ptr->monfear = 0;

	/* No knowledge */
	m_ptr->cdis = 0;

	/* clone value */
	m_ptr->clone=clo;

	for (Ind = 1; Ind < NumPlayers + 1; Ind++)
	{
		if (Players[Ind]->conn == NOT_CONNECTED)
			continue;

		Players[Ind]->mon_los[c_ptr->m_idx] = FALSE;
		Players[Ind]->mon_vis[c_ptr->m_idx] = FALSE;
	}

        /* Should we gain levels ? */
        if (Depth > 100)
        {
                int l = m_ptr->level + 100;

                m_ptr->exp = MONSTER_EXP(l);
                monster_check_experience(c_ptr->m_idx, TRUE);
        }

	strcpy(buf, (r_name + r_ptr->name));

	/* Update the monster */
	update_mon(c_ptr->m_idx, TRUE);


	/* Assume no sleeping */
	m_ptr->csleep = 0;

	/* Enforce sleeping if needed */
	if (slp && r_ptr->sleep)
	{
		int val = r_ptr->sleep;
		m_ptr->csleep = ((val * 2) + randint(val * 10));
	}


	/* Success */
	return (TRUE);
}


/*
 * Maximum size of a group of monsters
 */
#define GROUP_MAX	32


/*
 * Attempt to place a "group" of monsters around the given location
 */
static bool place_monster_group(int Depth, int y, int x, int r_idx, bool slp)
{
	monster_race *r_ptr = &r_info[r_idx];

	int old, n, i;
	int total = 0, extra = 0;

	int hack_n = 0;

	byte hack_y[GROUP_MAX];
	byte hack_x[GROUP_MAX];


	/* Pick a group size */
	total = randint(13);

	/* Hard monsters, small groups */
	if (r_ptr->level > Depth)
	{
		extra = r_ptr->level - Depth;
		extra = 0 - randint(extra);
	}

	/* Easy monsters, large groups */
	else if (r_ptr->level < Depth)
	{
		extra = Depth - r_ptr->level;
		extra = randint(extra);
	}

	/* Hack -- limit group reduction */
	if (extra > 12) extra = 12;

	/* Modify the group size */
	total += extra;

	/* Minimum size */
	if (total < 1) total = 1;

	/* Maximum size */
	if (total > GROUP_MAX) total = GROUP_MAX;


	/* Save the rating */
	old = rating;

	/* Start on the monster */
	hack_n = 1;
	hack_x[0] = x;
	hack_y[0] = y;

	/* Puddle monsters, breadth first, up to total */
	for (n = 0; (n < hack_n) && (hack_n < total); n++)
	{
		/* Grab the location */
		int hx = hack_x[n];
		int hy = hack_y[n];

		/* Check each direction, up to total */
		for (i = 0; (i < 8) && (hack_n < total); i++)
		{
			int mx = hx + ddx_ddd[i];
			int my = hy + ddy_ddd[i];

			/* Walls and Monsters block flow */
			if (!cave_empty_bold(Depth, my, mx)) continue;

			/* Attempt to place another monster */
			if (place_monster_one(Depth, my, mx, r_idx, slp, FALSE))
			{
				/* Add it to the "hack" set */
				hack_y[hack_n] = my;
				hack_x[hack_n] = mx;
				hack_n++;
			}
		}
	}

	/* Hack -- restore the rating */
	rating = old;


	/* Success */
	return (TRUE);
}


/*
 * Hack -- help pick an escort type
 */
static int place_monster_idx = 0;

/*
 * Hack -- help pick an escort type
 */
static bool place_monster_okay(int r_idx)
{
	monster_race *r_ptr = &r_info[place_monster_idx];

	monster_race *z_ptr = &r_info[r_idx];

	/* Require similar "race" */
	if (z_ptr->d_char != r_ptr->d_char) return (FALSE);

	/* Skip more advanced monsters */
	if (z_ptr->level > r_ptr->level) return (FALSE);

	/* Skip unique monsters */
	if (z_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* Paranoia -- Skip identical monsters */
	if (place_monster_idx == r_idx) return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Attempt to place a monster of the given race at the given location
 *
 * Note that certain monsters are now marked as requiring "friends".
 * These monsters, if successfully placed, and if the "grp" parameter
 * is TRUE, will be surrounded by a "group" of identical monsters.
 *
 * Note that certain monsters are now marked as requiring an "escort",
 * which is a collection of monsters with similar "race" but lower level.
 *
 * Some monsters induce a fake "group" flag on their escorts.
 *
 * Note the "bizarre" use of non-recursion to prevent annoying output
 * when running a code profiler.
 *
 * Note the use of the new "monster allocation table" code to restrict
 * the "get_mon_num()" function to "legal" escort types.
 */
bool place_monster_aux(int Depth, int y, int x, int r_idx, bool slp, bool grp, bool clo)
{
	int			i;

	monster_race	*r_ptr = &r_info[r_idx];


	/* Place one monster, or fail */
	if (!place_monster_one(Depth, y, x, r_idx, slp, clo)) return (FALSE);


	/* Require the "group" flag */
	if (!grp) return (TRUE);


	/* Friends for certain monsters */
	if (r_ptr->flags1 & RF1_FRIENDS)
	{
		/* Attempt to place a group */
		(void)place_monster_group(Depth, y, x, r_idx, slp);
	}


	/* Escorts for certain monsters */
	if (r_ptr->flags1 & RF1_ESCORT)
	{
		/* Try to place several "escorts" */
		for (i = 0; i < 50; i++)
		{
			int nx, ny, z, d = 3;

			/* Pick a location */
			scatter(Depth, &ny, &nx, y, x, d, 0);

			/* Require empty grids */
			if (!cave_empty_bold(Depth, ny, nx)) continue;


			/* Set the escort index */
			place_monster_idx = r_idx;


			/* Set the escort hook */
			get_mon_num_hook = place_monster_okay;

			/* Prepare allocation table */
			get_mon_num_prep();


			/* Pick a random race */
			z = get_mon_num(r_ptr->level);


			/* Remove restriction */
			get_mon_num_hook = NULL;

			/* Prepare allocation table */
			get_mon_num_prep();


			/* Handle failure */
			if (!z) break;

			/* Place a single escort */
			(void)place_monster_one(Depth, ny, nx, z, slp, FALSE);

			/* Place a "group" of escorts if needed */
			if ((r_info[z].flags1 & RF1_FRIENDS) ||
			    (r_ptr->flags1 & RF1_ESCORTS))
			{
				/* Place a group of monsters */
				(void)place_monster_group(Depth, ny, nx, z, slp);
			}
		}
	}


	/* Success */
	return (TRUE);
}


/*
 * Hack -- attempt to place a monster at the given location
 *
 * Attempt to find a monster appropriate to the "monster_level"
 */
bool place_monster(int Depth, int y, int x, bool slp, bool grp)
{
	int r_idx;

	/* Pick a monster */
	r_idx = get_mon_num(monster_level);

	/* Handle failure */
	if (!r_idx) return (FALSE);

	/* Attempt to place the monster */
	if (place_monster_aux(Depth, y, x, r_idx, slp, grp, FALSE)) return (TRUE);

	/* Oops */
	return (FALSE);
}




/*
 * XXX XXX XXX Player Ghosts are such a hack, they have been completely
 * removed until Angband 2.8.0, in which there will actually be a small
 * number of "unique" monsters which will serve as the "player ghosts".
 * Each will have a place holder for the "name" of a deceased player,
 * which will be extracted from a "bone" file, or replaced with a
 * "default" name if a real name is not available.  Each ghost will
 * appear exactly once and will not induce a special feeling.
 *
 * Possible methods:
 *   (s) 1 Skeleton
 *   (z) 1 Zombie
 *   (M) 1 Mummy
 *   (G) 1 Polterguiest, 1 Spirit, 1 Ghost, 1 Shadow, 1 Phantom
 *   (W) 1 Wraith
 *   (V) 1 Vampire, 1 Vampire Lord
 *   (L) 1 Lich
 *
 * Possible change: Lose 1 ghost, Add "Master Lich"
 *
 * Possible change: Lose 2 ghosts, Add "Wraith", Add "Master Lich"
 *
 * Possible change: Lose 4 ghosts, lose 1 vampire lord
 *
 * Note that ghosts should never sleep, should be very attentive, should
 * have maximal hitpoints, drop only good (or great) items, should be
 * cold blooded, evil, undead, immune to poison, sleep, confusion, fear.
 *
 * Base monsters:
 *   Skeleton
 *   Zombie
 *   Mummy
 *   Poltergeist
 *   Spirit
 *   Ghost
 *   Vampire
 *   Wraith
 *   Vampire Lord
 *   Shadow
 *   Phantom
 *   Lich
 *
 * This routine will simply extract ghost names from files, and
 * attempt to allocate a player ghost somewhere in the dungeon,
 * note that normal allocation may also attempt to place ghosts,
 * so we must work with some form of default names.
 *
 * XXX XXX XXX XXX
 */





/*
 * Attempt to allocate a random monster in the dungeon.
 *
 * Place the monster at least "dis" distance from the player.
 *
 * Use "slp" to choose the initial "sleep" status
 *
 * Use "monster_level" for the monster level
 */
bool alloc_monster(int Depth, int dis, int slp)
{
	int			y, x, i, d, min_dis = 999;
	int			tries = 0;
	player_type *p_ptr;

	/* paranoia, make sure the level is allocated */
	if (!cave[Depth]) return (FALSE);

	/* Find a legal, distant, unoccupied, space */
	while (tries < 50)
	{
		/* Increase the counter */
		tries++;

		/* Pick a location */
		y = rand_int(Depth ? MAX_HGT : MAX_HGT);
		x = rand_int(Depth ? MAX_WID : MAX_WID);

		/* Require "naked" floor grid */
		if (!cave_naked_bold(Depth, y, x)) continue;

		/* Accept far away grids */
		for (i = 1; i < NumPlayers + 1; i++)
		{
			p_ptr = Players[i];

			/* Skip him if he's not playing */
			if (p_ptr->conn == NOT_CONNECTED)
				continue;

			/* Skip him if he's not on this depth */
			if (p_ptr->dun_depth != Depth)
				continue;

			if ((d = distance(y, x, p_ptr->py, p_ptr->px)) < min_dis)
				min_dis = d;
		}

		if (min_dis >= dis)
			break;
	}

	/* Abort */
	if (tries >= 50)
		return (FALSE);

	/*printf("Trying to place a monster at %d, %d.\n", y, x);*/

	/* Attempt to place the monster, allow groups */
	if (place_monster(Depth, y, x, slp, TRUE)) return (TRUE);

	/* Nope */
	return (FALSE);
}




/*
 * Hack -- the "type" of the current "summon specific"
 */
static int summon_specific_type = 0;


/*
 * Hack -- help decide if a monster race is "okay" to summon
 */
static bool summon_specific_okay(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	bool okay = FALSE;


	/* Hack -- no specific type specified */
	if (!summon_specific_type) return (TRUE);


	/* Check our requirements */
	switch (summon_specific_type)
	{
		case SUMMON_ANT:
		{
			okay = ((r_ptr->d_char == 'a') &&
			        !(r_ptr->flags1 & RF1_UNIQUE));
			break;
		}

		case SUMMON_SPIDER:
		{
			okay = ((r_ptr->d_char == 'S') &&
			        !(r_ptr->flags1 & RF1_UNIQUE));
			break;
		}

		case SUMMON_HOUND:
		{
			okay = (((r_ptr->d_char == 'C') || (r_ptr->d_char == 'Z')) &&
			        !(r_ptr->flags1 & RF1_UNIQUE));
			break;
		}

		case SUMMON_HYDRA:
		{
			okay = ((r_ptr->d_char == 'M') &&
			        !(r_ptr->flags1 & RF1_UNIQUE));
			break;
		}

		case SUMMON_ANGEL:
		{
			okay = ((r_ptr->d_char == 'A') &&
			        !(r_ptr->flags1 & RF1_UNIQUE));
			break;
		}

		case SUMMON_DEMON:
		{
			okay = ((r_ptr->flags3 & RF3_DEMON) &&
			        !(r_ptr->flags1 & RF1_UNIQUE));
			break;
		}

		case SUMMON_UNDEAD:
		{
			okay = ((r_ptr->flags3 & RF3_UNDEAD) &&
			        !(r_ptr->flags1 & RF1_UNIQUE));
			break;
		}

		case SUMMON_DRAGON:
		{
			okay = ((r_ptr->flags3 & RF3_DRAGON) &&
			        !(r_ptr->flags1 & RF1_UNIQUE));
			break;
		}

		case SUMMON_HI_UNDEAD:
		{
			okay = ((r_ptr->d_char == 'L') ||
			        (r_ptr->d_char == 'V') ||
			        (r_ptr->d_char == 'W'));
			break;
		}

		case SUMMON_HI_DRAGON:
		{
			okay = (r_ptr->d_char == 'D');
			break;
		}

		case SUMMON_WRAITH:
		{
			okay = ((r_ptr->d_char == 'W') &&
			        (r_ptr->flags1 & RF1_UNIQUE));
			break;
		}

		case SUMMON_UNIQUE:
		{
			okay = (r_ptr->flags1 & RF1_UNIQUE);
			break;
		}
	}

	/* Result */
	return (okay);
}


/*
 * Place a monster (of the specified "type") near the given
 * location.  Return TRUE iff a monster was actually summoned.
 *
 * We will attempt to place the monster up to 10 times before giving up.
 *
 * Note: SUMMON_UNIQUE and SUMMON_WRAITH (XXX) will summon Unique's
 * Note: SUMMON_HI_UNDEAD and SUMMON_HI_DRAGON may summon Unique's
 * Note: None of the other summon codes will ever summon Unique's.
 *
 * This function has been changed.  We now take the "monster level"
 * of the summoning monster as a parameter, and use that, along with
 * the current dungeon level, to help determine the level of the
 * desired monster.  Note that this is an upper bound, and also
 * tends to "prefer" monsters of that level.  Currently, we use
 * the average of the dungeon and monster levels, and then add
 * five to allow slight increases in monster power.
 *
 * Note that we use the new "monster allocation table" creation code
 * to restrict the "get_mon_num()" function to the set of "legal"
 * monsters, making this function much faster and more reliable.
 *
 * Note that this function may not succeed, though this is very rare.
 */
bool summon_specific(int Depth, int y1, int x1, int lev, int type)
{
	int i, x, y, r_idx;


	/* Look for a location */
	for (i = 0; i < 20; ++i)
	{
		/* Pick a distance */
		int d = (i / 15) + 1;

		/* Pick a location */
		scatter(Depth, &y, &x, y1, x1, d, 0);

		/* Require "empty" floor grid */
		if (!cave_empty_bold(Depth, y, x)) continue;

		/* Hack -- no summon on glyph of warding */
		if (cave[Depth][y][x].feat == FEAT_GLYPH) continue;

		/* Okay */
		break;
	}

	/* Failure */
	if (i == 20) return (FALSE);


	/* Save the "summon" type */
	summon_specific_type = type;


	/* Require "okay" monsters */
	get_mon_num_hook = summon_specific_okay;

	/* Prepare allocation table */
	get_mon_num_prep();


	/* Pick a monster, using the level calculation */
	/* Exception for Morgoth (so that he won't summon townies)
	 * This fix presumes Morgie and Morgie only has level 100 */
//	r_idx = get_mon_num((Depth + lev) / 2 + 5);
	r_idx = (lev != 100)?get_mon_num((Depth + lev) / 2 + 5) : 100;


	/* Remove restriction */
	get_mon_num_hook = NULL;

	/* Prepare allocation table */
	get_mon_num_prep();


	/* Handle failure */
	if (!r_idx) return (FALSE);

	/* Attempt to place the monster (awake, allow groups) */
	if (!place_monster_aux(Depth, y, x, r_idx, FALSE, TRUE, FALSE)) return (FALSE);

	/* Success */
	return (TRUE);
}

/* summon a specific race near this location */
/* summon until we can't find a location or we have summoned size */
bool summon_specific_race(int Depth, int y1, int x1, int r_idx, unsigned char size)
{
	int c, i, x, y;

	/* for each monster we are summoning */

	for (c = 0; c < size; c++)
	{	

		/* Look for a location */
		for (i = 0; i < 200; ++i)
		{
			/* Pick a distance */
			int d = (i / 15) + 1;

			/* Pick a location */
			scatter(Depth, &y, &x, y1, x1, d, 0);

			/* Require "empty" floor grid */
			if (!cave_empty_bold(Depth, y, x)) continue;

			/* Hack -- no summon on glyph of warding */
			if (cave[Depth][y][x].feat == FEAT_GLYPH) continue;

			/* Okay */
			break;
		}

		/* Failure */
		if (i == 20) return (FALSE);

		/* Handle failure */
		if (!r_idx) return (FALSE);

		/* Attempt to place the monster (awake, don't allow groups) */
		if (!place_monster_aux(Depth, y, x, r_idx, FALSE, FALSE, FALSE)) return (FALSE);
	}

	/* Success */
	return (TRUE);
}


/* summon a specific race at a random location */
bool summon_specific_race_somewhere(int Depth, int r_idx, unsigned char size)
{
	int			y, x, i, d, min_dis = 999;
	int			tries = 0;
	player_type *p_ptr;

	/* paranoia, make sure the level is allocated */
	if (!cave[Depth]) return (FALSE);

	/* Find a legal, distant, unoccupied, space */
	while (tries < 50)
	{
		/* Increase the counter */
		tries++;

		/* Pick a location */
		y = rand_int(Depth ? MAX_HGT : MAX_HGT);
		x = rand_int(Depth ? MAX_WID : MAX_WID);

		/* Require "naked" floor grid */
		if (!cave_naked_bold(Depth, y, x)) continue;


		/* Abort */
		if (tries >= 50)
			return (FALSE);

		/* We have a valid location */
		break;
	}

	/* Attempt to place the monster */
	if (summon_specific_race(Depth, y, x, r_idx, size)) return TRUE;
	return (FALSE);
}





/*
 * Let the given monster attempt to reproduce.
 *
 * Note that "reproduction" REQUIRES empty space.
 */
bool multiply_monster(int m_idx)
{
	monster_type	*m_ptr = &m_list[m_idx];
        monster_race    *r_ptr = R_INFO(m_ptr);

	int			i, y, x;

	bool result = FALSE;

	/* NO UNIQUES */
	if (r_ptr->flags1 & RF1_UNIQUE) return FALSE;

	/* Try up to 18 times */
	for (i = 0; i < 18; i++)
	{
		int d = 1;

		/* Pick a location */
		scatter(m_ptr->dun_depth, &y, &x, m_ptr->fy, m_ptr->fx, d, 0);

		/* Require an "empty" floor grid */
		if (!cave_empty_bold(m_ptr->dun_depth, y, x)) continue;

		/* Create a new monster (awake, no groups) */
		result = place_monster_aux(m_ptr->dun_depth, y, x, m_ptr->r_idx, FALSE, FALSE, TRUE);

		/* Done */
		break;
	}

	/* Result */
	return (result);
}





/*
 * Dump a message describing a monster's reaction to damage
 *
 * Technically should attempt to treat "Beholder"'s as jelly's
 */
void message_pain(int Ind, int m_idx, int dam)
{
	long			oldhp, newhp, tmp;
	int				percentage;

	monster_type		*m_ptr = &m_list[m_idx];
        monster_race            *r_ptr = R_INFO(m_ptr);

	char			m_name[80];


	/* Get the monster name */
	monster_desc(Ind, m_name, m_idx, 0);

	/* Notice non-damage */
	if (dam == 0)
	{
		msg_format(Ind, "%^s is unharmed.", m_name);
		return;
	}

	/* Note -- subtle fix -CFT */
	newhp = (long)(m_ptr->hp);
	oldhp = newhp + (long)(dam);
	tmp = (newhp * 100L) / oldhp;
	percentage = (int)(tmp);


	/* Jelly's, Mold's, Vortex's, Quthl's */
	if (strchr("jmvQ", r_ptr->d_char))
	{
		if (percentage > 95)
			msg_format(Ind, "%^s barely notices.", m_name);
		else if (percentage > 75)
			msg_format(Ind, "%^s flinches.", m_name);
		else if (percentage > 50)
			msg_format(Ind, "%^s squelches.", m_name);
		else if (percentage > 35)
			msg_format(Ind, "%^s quivers in pain.", m_name);
		else if (percentage > 20)
			msg_format(Ind, "%^s writhes about.", m_name);
		else if (percentage > 10)
			msg_format(Ind, "%^s writhes in agony.", m_name);
		else
			msg_format(Ind, "%^s jerks limply.", m_name);
	}

	/* Dogs and Hounds */
	else if (strchr("CZ", r_ptr->d_char))
	{
		if (percentage > 95)
			msg_format(Ind, "%^s shrugs off the attack.", m_name);
		else if (percentage > 75)
			msg_format(Ind, "%^s snarls with pain.", m_name);
		else if (percentage > 50)
			msg_format(Ind, "%^s yelps in pain.", m_name);
		else if (percentage > 35)
			msg_format(Ind, "%^s howls in pain.", m_name);
		else if (percentage > 20)
			msg_format(Ind, "%^s howls in agony.", m_name);
		else if (percentage > 10)
			msg_format(Ind, "%^s writhes in agony.", m_name);
		else
			msg_format(Ind, "%^s yelps feebly.", m_name);
	}

	/* One type of monsters (ignore,squeal,shriek) */
	else if (strchr("FIKMRSXabclqrst", r_ptr->d_char))
	{
		if (percentage > 95)
			msg_format(Ind, "%^s ignores the attack.", m_name);
		else if (percentage > 75)
			msg_format(Ind, "%^s grunts with pain.", m_name);
		else if (percentage > 50)
			msg_format(Ind, "%^s squeals in pain.", m_name);
		else if (percentage > 35)
			msg_format(Ind, "%^s shrieks in pain.", m_name);
		else if (percentage > 20)
			msg_format(Ind, "%^s shrieks in agony.", m_name);
		else if (percentage > 10)
			msg_format(Ind, "%^s writhes in agony.", m_name);
		else
			msg_format(Ind, "%^s cries out feebly.", m_name);
	}

	/* Another type of monsters (shrug,cry,scream) */
	else
	{
		if (percentage > 95)
			msg_format(Ind, "%^s shrugs off the attack.", m_name);
		else if (percentage > 75)
			msg_format(Ind, "%^s grunts with pain.", m_name);
		else if (percentage > 50)
			msg_format(Ind, "%^s cries out in pain.", m_name);
		else if (percentage > 35)
			msg_format(Ind, "%^s screams in pain.", m_name);
		else if (percentage > 20)
			msg_format(Ind, "%^s screams in agony.", m_name);
		else if (percentage > 10)
			msg_format(Ind, "%^s writhes in agony.", m_name);
		else
			msg_format(Ind, "%^s cries out feebly.", m_name);
	}
}



/*
 * Learn about an "observed" resistance.
 */
void update_smart_learn(int m_idx, int what)
{

#ifdef DRS_SMART_OPTIONS

	monster_type *m_ptr = &m_list[m_idx];

        monster_race *r_ptr = R_INFO(m_ptr);


	/* Not allowed to learn */
	if (!smart_learn) return;

	/* Too stupid to learn anything */
	if (r_ptr->flags2 & RF2_STUPID) return;

	/* Not intelligent, only learn sometimes */
	if (!(r_ptr->flags2 & RF2_SMART) && (rand_int(100) < 50)) return;


	/* XXX XXX XXX */

	/* Analyze the knowledge */
	switch (what)
	{
		case DRS_ACID:
		if (p_ptr->resist_acid) m_ptr->smart |= SM_RES_ACID;
		if (p_ptr->oppose_acid) m_ptr->smart |= SM_OPP_ACID;
		if (p_ptr->immune_acid) m_ptr->smart |= SM_IMM_ACID;
		break;

		case DRS_ELEC:
		if (p_ptr->resist_elec) m_ptr->smart |= SM_RES_ELEC;
		if (p_ptr->oppose_elec) m_ptr->smart |= SM_OPP_ELEC;
		if (p_ptr->immune_elec) m_ptr->smart |= SM_IMM_ELEC;
		break;

		case DRS_FIRE:
		if (p_ptr->resist_fire) m_ptr->smart |= SM_RES_FIRE;
		if (p_ptr->oppose_fire) m_ptr->smart |= SM_OPP_FIRE;
		if (p_ptr->immune_fire) m_ptr->smart |= SM_IMM_FIRE;
		break;

		case DRS_COLD:
		if (p_ptr->resist_cold) m_ptr->smart |= SM_RES_COLD;
		if (p_ptr->oppose_cold) m_ptr->smart |= SM_OPP_COLD;
		if (p_ptr->immune_cold) m_ptr->smart |= SM_IMM_COLD;
		break;

		case DRS_POIS:
		if (p_ptr->resist_pois) m_ptr->smart |= SM_RES_POIS;
		if (p_ptr->oppose_pois) m_ptr->smart |= SM_OPP_POIS;
		break;


		case DRS_NETH:
		if (p_ptr->resist_neth) m_ptr->smart |= SM_RES_NETH;
		break;

		case DRS_LITE:
		if (p_ptr->resist_lite) m_ptr->smart |= SM_RES_LITE;
		break;

		case DRS_DARK:
		if (p_ptr->resist_dark) m_ptr->smart |= SM_RES_DARK;
		break;

		case DRS_FEAR:
		if (p_ptr->resist_fear) m_ptr->smart |= SM_RES_FEAR;
		break;

		case DRS_CONF:
		if (p_ptr->resist_conf) m_ptr->smart |= SM_RES_CONF;
		break;

		case DRS_CHAOS:
		if (p_ptr->resist_chaos) m_ptr->smart |= SM_RES_CHAOS;
		break;

		case DRS_DISEN:
		if (p_ptr->resist_disen) m_ptr->smart |= SM_RES_DISEN;
		break;

		case DRS_BLIND:
		if (p_ptr->resist_blind) m_ptr->smart |= SM_RES_BLIND;
		break;

		case DRS_NEXUS:
		if (p_ptr->resist_nexus) m_ptr->smart |= SM_RES_NEXUS;
		break;

		case DRS_SOUND:
		if (p_ptr->resist_sound) m_ptr->smart |= SM_RES_SOUND;
		break;

		case DRS_SHARD:
		if (p_ptr->resist_shard) m_ptr->smart |= SM_RES_SHARD;
		break;


		case DRS_FREE:
		if (p_ptr->free_act) m_ptr->smart |= SM_IMM_FREE;
		break;

		case DRS_MANA:
		if (!p_ptr->msp) m_ptr->smart |= SM_IMM_MANA;
		break;
	}

#endif /* DRS_SMART_OPTIONS */

}

/*
* Set the "m_idx" fields in the cave array to correspond
* to the objects in the "m_list".
*/
void setup_monsters(void)
{
	int i;
	monster_type *r_ptr;
	
	for (i = 0; i < m_max; i++)
	{
		r_ptr = &m_list[i];
		/* setup the cave m_idx if the level has been 
		 * allocated.
		 */
		if (cave[r_ptr->dun_depth]) 
			cave[r_ptr->dun_depth][r_ptr->fy][r_ptr->fx].m_idx = i;
	}
}

/* Takes a monster name and returns an index, or 0 if no such monster
 * was found.
 */
int race_index(char * name)
{
	int i;

	/* for each monster race */
	for (i = 1; i <= alloc_race_size; i++)
	{
		if (!strcmp(&r_name[r_info[i].name],name)) return i;
	}
	return 0;
}

monster_race* r_info_get(monster_type *m_ptr)
{
        if (m_ptr->special) return (m_ptr->r_ptr);
        else return (&r_info[m_ptr->r_idx]);
}

cptr r_name_get(monster_type *m_ptr)
{
        static char buf[100];

        if (m_ptr->special)
        {
                cptr p = (m_ptr->owner)?lookup_player_name(m_ptr->owner):"**INTERNAL BUG**";
                if (p == NULL) p = "**INTERNAL BUG**";
                switch (m_ptr->r_idx - 1)
                {
                        case SV_GOLEM_WOOD:
                                sprintf(buf, "%s's Wood Golem", p);
                                break;
                        case SV_GOLEM_COPPER:
                                sprintf(buf, "%s's Copper Golem", p);
                                break;
                        case SV_GOLEM_IRON:
                                sprintf(buf, "%s's Iron Golem", p);
                                break;
                        case SV_GOLEM_ALUM:
                                sprintf(buf, "%s's Aluminium Golem", p);
                                break;
                        case SV_GOLEM_SILVER:
                                sprintf(buf, "%s's Silver Golem", p);
                                break;
                        case SV_GOLEM_GOLD:
                                sprintf(buf, "%s's Gold Golem", p);
                                break;
                        case SV_GOLEM_MITHRIL:
                                sprintf(buf, "%s's Mithril Golem", p);
                                break;
                        case SV_GOLEM_ADAM:
                                sprintf(buf, "%s's Adamantite Golem", p);
                                break;
                }
                return (buf);
        }
        else return (r_name + r_info[m_ptr->r_idx].name);
}
