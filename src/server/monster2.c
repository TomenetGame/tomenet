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

/*
 * Descriptions from PernAngband.	- Jir -
 */
#define MAX_HORROR 20
#define MAX_FUNNY 22
#define MAX_COMMENT 5

static cptr horror_desc[MAX_HORROR] =
{
	"abominable",
	"abysmal",
	"appalling",
	"baleful",
	"blasphemous",

	"disgusting",
	"dreadful",
	"filthy",
	"grisly",
	"hideous",

	"hellish",
	"horrible",
	"infernal",
	"loathsome",
	"nightmarish",

	"repulsive",
	"sacrilegious",
	"terrible",
	"unclean",
	"unspeakable",
};

static cptr funny_desc[MAX_FUNNY] =
{
	"silly",
	"hilarious",
	"absurd",
	"insipid",
	"ridiculous",

	"laughable",
	"ludicrous",
	"far-out",
	"groovy",
	"postmodern",

	"fantastic",
	"dadaistic",
	"cubistic",
	"cosmic",
	"awesome",

	"incomprehensible",
	"fabulous",
	"amazing",
	"incredible",
	"chaotic",

	"wild",
	"preposterous",
};

static cptr funny_comments[MAX_COMMENT] =
{
	"Wow, cosmic, man!",
	"Rad!",
	"Groovy!",
	"Cool!",
	"Far out!"
};

static monster_race* race_info_idx(int r_idx, int ego, int randuni);

/* Monster gain a few levels ? */
void monster_check_experience(int m_idx, bool silent)
{
        monster_type    *m_ptr = &m_list[m_idx];
        monster_race    *r_ptr = race_inf(m_ptr);

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

			/* fix -- never back to 0 */
			if (m_ptr->speed < speed) m_ptr->speed = 255;
			if (m_ptr->mspeed < speed) m_ptr->mspeed = 255;
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
	int x, y, Ind;
	struct worldpos *wpos;
	cave_type **zcave;
	monster_type *m_ptr = &m_list[i];

        monster_race *r_ptr = race_inf(m_ptr);

	/* Get location */
	y = m_ptr->fy;
	x = m_ptr->fx;
	wpos=&m_ptr->wpos;

	/* Hack -- Reduce the racial counter */
	r_ptr->cur_num--;

	/* Hack -- count the number of "reproducers" */
	if (r_ptr->flags4 & RF4_MULTIPLY) num_repro--;


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
	if((zcave=getcave(wpos))){
		zcave[y][x].m_idx=0;
	}
	/* Visual update */
	everyone_lite_spot(wpos, y, x);

	/* Wipe the Monster */
	FREE(m_ptr->r_ptr, monster_race);
	WIPE(m_ptr, monster_type);
}


/*
 * Delete the monster, if any, at a given location
 */
void delete_monster(struct worldpos *wpos, int y, int x)
{
	cave_type *c_ptr;

	/* Paranoia */
	cave_type **zcave;
	if (!in_bounds(y, x)) return;
	if((zcave=getcave(wpos))){
		/* Check the grid */
		c_ptr=&zcave[y][x];
		/* Delete the monster (if any) */
		if (c_ptr->m_idx > 0) delete_monster_idx(c_ptr->m_idx);
	}
	else{                           /* still delete the monster, just slower method */
		int i;
		for(i=0;i<m_max;i++){
			monster_type *m_ptr=&m_list[i];
			if(m_ptr->r_idx && inarea(wpos, &m_ptr->wpos))
			{
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
	int             i, num, cnt, Ind;

	int             cur_lev, cur_dis, chance;
			cave_type **zcave;
			struct worldpos *wpos;


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

                        monster_race *r_ptr = race_inf(m_ptr);

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
			if (istown(&m_ptr->wpos))
				chance = 70;

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
		/* real monsters in unreal location are not skipped. */
		if (m_ptr->r_idx && (!m_ptr->wpos.wz || getcave(&m_ptr->wpos))) continue;

		/* One less monster */
		m_max--;

		/* Reorder */
		if (i != m_max)
		{
			int ny = m_list[m_max].fy;
			int nx = m_list[m_max].fx;
			wpos = &m_list[m_max].wpos;

			/* Update the cave */
			/* Hack -- make sure the level is allocated, as in the wilderness
			   it sometimes will not be */
			if((zcave=getcave(wpos)))
				zcave[ny][nx].m_idx = i;

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
void wipe_m_list(struct worldpos *wpos)
{
	int i;

#if 0
	/* Delete all the monsters */
	for (i = m_max - 1; i >= 1; i--)
	{
		monster_type *m_ptr = &m_list[i];

                monster_race *r_ptr = race_inf(m_ptr);

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

		if (inarea(&m_ptr->wpos,wpos))
			delete_monster_idx(i);
	}

	/* Compact the monster list */
	compact_monsters(0);
}

/* 
 * Heal up every monster on the depth, so that a player
 * cannot abuse stair-GoI and anti-scum.	- Jir -
 */
void heal_m_list(struct worldpos *wpos)
{
	int i;

	/* Heal all the monsters */
	for (i = m_max - 1; i >= 1; i--)
	{
		monster_type *m_ptr = &m_list[i];

		if (inarea(&m_ptr->wpos,wpos))
			m_ptr->hp = m_ptr->maxhp;
//			delete_monster_idx(i);
	}

	/* Compact the monster list */
//	compact_monsters(0);
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
	int                     i, j, p, d1 = 0, d2 = 0;

	int                     r_idx;

	long            value, total;

	monster_race    *r_ptr;

	alloc_entry             *table = alloc_race_table;

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
/*              if ((r_ptr->flags1 & RF1_UNIQUE) &&
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

		/* Depth Monsters never appear out of their depth */
		/* level = base_depth + depth, be careful(esp.wilderness) */
		/* Seemingly it's only for Moldoux */
		if ((r_ptr->flags9 & (RF9_ONLY_DEPTH)) && (r_ptr->level != level))
		{
			continue;
		}

		/* Zangbandish monsters allowed ? or not ? */
		if(!cfg.zang_monsters && (r_ptr->flags8 & RF8_ZANGBAND)) continue;

		/* Pernian monsters allowed ? or not ? */
		if(!cfg.pern_monsters && (r_ptr->flags8 & RF8_PERNANGBAND)) continue;

		/* Lovercraftian monsters allowed ? or not ? */
		if(!cfg.cth_monsters && (r_ptr->flags8 & RF8_CTHANGBAND)) continue;

		/* Joke monsters allowed ? or not ? */
		if(!cfg.joke_monsters && (r_ptr->flags8 & RF8_JOKEANGBAND)) continue;

		/* Base monsters allowed ? or not ? */
		if(!cfg.vanilla_monsters && (r_ptr->flags8 & RF8_ANGBAND)) continue;

		/* Some dungeon types restrict the possible monsters */
//		if(!summon_hack && !restrict_monster_to_dungeon(r_idx) && dun_level) continue;


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

	cptr            res;

        monster_type    *m_ptr = &m_list[m_idx];
        monster_race    *r_ptr = race_inf(m_ptr);

	cptr            name = r_name_get(m_ptr);

	bool            seen, pron;

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

        monster_race *r_ptr = race_inf(m_ptr);

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

        monster_race *r_ptr = race_inf(m_ptr);

	/* Note the number of things dropped */
	if (num_item > r_ptr->r_drop_item) r_ptr->r_drop_item = num_item;
	if (num_gold > r_ptr->r_drop_gold) r_ptr->r_drop_gold = num_gold;

	/* Hack -- memorize the good/great flags */
	if (r_ptr->flags1 & RF1_DROP_GOOD) r_ptr->r_flags1 |= RF1_DROP_GOOD;
	if (r_ptr->flags1 & RF1_DROP_GREAT) r_ptr->r_flags1 |= RF1_DROP_GREAT;
}

/*
 * Here comes insanity from PernAngband.. hehehe.. - Jir -
 */
//void sanity_blast(int Ind, monster_type * m_ptr, bool necro)
void sanity_blast(int Ind, int m_idx, bool necro)
{
	player_type *p_ptr = Players[Ind];
	monster_type    *m_ptr = &m_list[m_idx];
	bool happened = FALSE;
	int power = 100;

	if (!necro)
	{
		char            m_name[80];
		monster_race    *r_ptr;

		if (m_ptr != NULL) r_ptr = race_inf(m_ptr);
		else return;

		power = (m_ptr->level)+10;

		if (m_ptr != NULL)
		{
			monster_desc(Ind, m_name, m_idx, 0);

			if (!(r_ptr->flags1 & RF1_UNIQUE))
			{
				if (r_ptr->flags1 & RF1_FRIENDS)
					power /= 2;
			}
			else power *= 2;

			//		if (!hack_mind)
			return; /* No effect yet, just loaded... */

			//		if (!(m_ptr->ml))
			if (!p_ptr->mon_vis[m_idx])
				return; /* Cannot see it for some reason */

			if (!(r_ptr->flags2 & RF2_ELDRITCH_HORROR))
				return; /* oops */



			//                if ((is_friend(m_ptr) > 0) && (randint(8)!=1))
			return; /* Pet eldritch horrors are safe most of the time */


			if (randint(power)<p_ptr->skill_sav)
			{
				return; /* Save, no adverse effects */
			}


			if (p_ptr->image)
			{
				/* Something silly happens... */
				msg_format(Ind, "You behold the %s visage of %s!",
						funny_desc[(randint(MAX_FUNNY))-1], m_name);
				if (randint(3)==1)
				{
					msg_print(Ind, funny_comments[randint(MAX_COMMENT)-1]);
					p_ptr->image = (p_ptr->image + randint(m_ptr->level));
				}
				return; /* Never mind; we can't see it clearly enough */
			}

			/* Something frightening happens... */
			msg_format(Ind, "You behold the %s visage of %s!",
					horror_desc[(randint(MAX_HORROR))-1], m_name);

			r_ptr->r_flags2 |= RF2_ELDRITCH_HORROR;

		}

		/* Undead characters are 50% likely to be unaffected */
#if 0
		if ((PRACE_FLAG(PR1_UNDEAD))||(p_ptr->mimic_form == MIMIC_VAMPIRE))
		{
			if (randint(100) < (25 + (p_ptr->lev))) return;
		}
#endif	// 0
	}
	else
	{
		msg_print(Ind, "Your sanity is shaken by reading the Necronomicon!");
	}

	if (randint(power)<p_ptr->skill_sav) /* Mind blast */
	{
		if (!p_ptr->resist_conf)
		{
			(void)set_confused(Ind, p_ptr->confused + rand_int(4) + 4);
		}
		if ((!p_ptr->resist_chaos) && (randint(3)==1))
		{
			(void) set_image(Ind, p_ptr->image + rand_int(250) + 150);
		}
		return;
	}

	if (randint(power)<p_ptr->skill_sav) /* Lose int & wis */
	{
		do_dec_stat (Ind, A_INT, STAT_DEC_NORMAL);
		do_dec_stat (Ind, A_WIS, STAT_DEC_NORMAL);
		return;
	}


	if (randint(power)<p_ptr->skill_sav) /* Brain smash */
	{
		if (!p_ptr->resist_conf)
		{
			(void)set_confused(Ind, p_ptr->confused + rand_int(4) + 4);
		}
		if (!p_ptr->free_act)
		{
			(void)set_paralyzed(Ind, p_ptr->paralyzed + rand_int(4) + 4);
		}
		while (rand_int(100) > p_ptr->skill_sav)
			(void)do_dec_stat(Ind, A_INT, STAT_DEC_NORMAL);	
		while (rand_int(100) > p_ptr->skill_sav)
			(void)do_dec_stat(Ind, A_WIS, STAT_DEC_NORMAL);
		if (!p_ptr->resist_chaos)
		{
			(void) set_image(Ind, p_ptr->image + rand_int(250) + 150);
		}
		return;
	}

	if (randint(power)<p_ptr->skill_sav) /* Permanent lose int & wis */
	{
		if (dec_stat(Ind, A_INT, 10, TRUE)) happened = TRUE;
		if (dec_stat(Ind, A_WIS, 10, TRUE)) happened = TRUE;
		if (happened)
			msg_print(Ind, "You feel much less sane than before.");
		return;
	}


	if (randint(power)<p_ptr->skill_sav) /* Amnesia */
	{

		if (lose_all_info(Ind))
			msg_print(Ind, "You forget everything in your utmost terror!");
		return;
	}

	/* Else gain permanent insanity */
#if 0
	if ((p_ptr->muta3 & MUT3_MORONIC) && (p_ptr->muta2 & MUT2_BERS_RAGE) &&
			((p_ptr->muta2 & MUT2_COWARDICE) || (p_ptr->resist_fear)) &&
			((p_ptr->muta2 & MUT2_HALLU) || (p_ptr->resist_chaos)))
	{
		/* The poor bastard already has all possible insanities! */
		return;
	}

	while (!happened)
	{
		switch(randint(4))
		{
			case 1:
				if (!(p_ptr->muta3 & MUT3_MORONIC))
				{
					msg_print("You turn into an utter moron!");
					if (p_ptr->muta3 & MUT3_HYPER_INT)
					{
						msg_print("Your brain is no longer a living computer.");
						p_ptr->muta3 &= ~(MUT3_HYPER_INT);
					}
					p_ptr->muta3 |= MUT3_MORONIC;
					happened = TRUE;
				}
				break;
			case 2:
				if (!(p_ptr->muta2 & MUT2_COWARDICE) && !(p_ptr->resist_fear))
				{
					msg_print("You become paranoid!");

					/* Duh, the following should never happen, but anyway... */
					if (p_ptr->muta3 & MUT3_FEARLESS)
					{
						msg_print("You are no longer fearless.");
						p_ptr->muta3 &= ~(MUT3_FEARLESS);
					}

					p_ptr->muta2 |= MUT2_COWARDICE;
					happened = TRUE;
				}
				break;
			case 3:
				if (!(p_ptr->muta2 & MUT2_HALLU) && !(p_ptr->resist_chaos))
				{
					msg_print("You are afflicted by a hallucinatory insanity!");
					p_ptr->muta2 |= MUT2_HALLU;
					happened = TRUE;
				}
				break;
			default:
				if (!(p_ptr->muta2 & MUT2_BERS_RAGE))
				{
					msg_print("You become subject to fits of berserk rage!");
					p_ptr->muta2 |= MUT2_BERS_RAGE;
					happened = TRUE;
				}
				break;
		}
	}
#endif	// 0

	p_ptr->update |= PU_BONUS;
	handle_stuff(Ind);
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

        monster_race *r_ptr = race_inf(m_ptr);

	player_type *p_ptr;

	/* The current monster location */
	int fy = m_ptr->fy;
	int fx = m_ptr->fx;

	struct worldpos *wpos=&m_ptr->wpos;
	cave_type **zcave;

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
		if(!inarea(wpos, &p_ptr->wpos))
		{
			p_ptr->mon_vis[m_idx] = FALSE;
			p_ptr->mon_los[m_idx] = FALSE;
			continue;
		}

		/* If our wilderness level has been deallocated, stop here...
		 * we are "detatched" monsters.
		 */

		if(!(zcave=getcave(wpos))) continue;

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
			cave_type *c_ptr = &zcave[fy][fx];
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

				/* Different ESP */
				if ((p_ptr->telepathy & ESP_ORC) && (r_ptr->flags3 & RF3_ORC)) see = TRUE;
				if ((p_ptr->telepathy & ESP_SPIDER) && (r_ptr->flags7 & RF7_SPIDER)) see = TRUE;
				if ((p_ptr->telepathy & ESP_TROLL) && (r_ptr->flags3 & RF3_TROLL)) see = TRUE;
				if ((p_ptr->telepathy & ESP_DRAGON) && (r_ptr->flags3 & RF3_DRAGON)) see = TRUE;
				if ((p_ptr->telepathy & ESP_GIANT) && (r_ptr->flags3 & RF3_GIANT)) see = TRUE;
				if ((p_ptr->telepathy & ESP_DEMON) && (r_ptr->flags3 & RF3_DEMON)) see = TRUE;
				if ((p_ptr->telepathy & ESP_UNDEAD) && (r_ptr->flags3 & RF3_UNDEAD)) see = TRUE;
				if ((p_ptr->telepathy & ESP_EVIL) && (r_ptr->flags3 & RF3_EVIL)) see = TRUE;
				if ((p_ptr->telepathy & ESP_ANIMAL) && (r_ptr->flags3 & RF3_ANIMAL)) see = TRUE;
				if ((p_ptr->telepathy & ESP_DRAGONRIDER) && (r_ptr->flags3 & RF3_DRAGONRIDER)) see = TRUE;
				if ((p_ptr->telepathy & ESP_GOOD) && (r_ptr->flags3 & RF3_GOOD)) see = TRUE;
				if ((p_ptr->telepathy & ESP_NONLIVING) && (r_ptr->flags3 & RF3_NONLIVING)) see = TRUE;
				if ((p_ptr->telepathy & ESP_UNIQUE) && ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags3 & RF3_UNIQUE_4))) see = TRUE;
				if (p_ptr->telepathy & ESP_ALL) see = TRUE;


//				if (p_ptr->mode == MODE_NORMAL) see = TRUE;
				if (see && (p_ptr->mode == MODE_HELL) && (m_ptr->cdis < MAX_SIGHT)) see = TRUE;
				if (see && !p_ptr->telepathy && (p_ptr->prace == RACE_DRIDER) && (m_ptr->cdis > (p_ptr->lev / 2))) see = FALSE;

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
			if (p_ptr->admin_dm) flag = TRUE;
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
				if(!m_list[m_idx].special && r_ptr->d_char != 't')
					if (p_ptr->disturb_move) disturb(Ind, 1, 0);
			}

			/* Memorize various observable flags */
			if (do_empty_mind) r_ptr->r_flags2 |= RF2_EMPTY_MIND;
			if (do_weird_mind) r_ptr->r_flags2 |= RF2_WEIRD_MIND;
			if (do_cold_blood) r_ptr->r_flags2 |= RF2_COLD_BLOOD;
			if (do_invisible) r_ptr->r_flags2 |= RF2_INVISIBLE;

			/* Efficiency -- Notice multi-hued monsters */
//			if (r_ptr->flags1 & RF1_ATTR_MULTI) scan_monsters = TRUE;
			if ((r_ptr->flags1 & RF1_ATTR_MULTI) ||
					(r_ptr->flags1 & RF1_UNIQUE) ||
					(m_ptr->ego ) ) scan_monsters = TRUE;
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
				if(!m_list[m_idx].special && r_ptr->d_char != 't')
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
				if(!m_list[m_idx].special && r_ptr->d_char != 't')
					if (p_ptr->disturb_near) disturb(Ind, 1, 0);

				/* well, is it the right place to be? */
				if (r_ptr->flags2 & RF2_ELDRITCH_HORROR)
				{
					sanity_blast(Ind, m_idx, FALSE);
				}

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
				if(!m_list[m_idx].special && r_ptr->d_char != 't')
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
	cave_type **zcave;

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
		zcave=getcave(&p_ptr->wpos);

		/* Reset the flags */
		flag = easy = hard = FALSE;

		/* Skip disconnected players */
		if (p_ptr->conn == NOT_CONNECTED) continue;

		/* Skip players not on this depth */
		if(!inarea(&p_ptr->wpos, &q_ptr->wpos)) continue;

		/* Player can always see himself */
		if (Ind == i) continue;

		/* Compute distance */
		dis = distance(py, px, p_ptr->py, p_ptr->px);

		/* Process players on current panel */
		if (panel_contains(py, px) && zcave)
		{
			cave_type *c_ptr = &zcave[py][px];
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
			if (p_ptr->telepathy & ESP_ALL || (p_ptr->prace == RACE_DRIDER))
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
			if (p_ptr->admin_dm) flag = TRUE;
			if (q_ptr->admin_dm) flag = FALSE;
			
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
bool allow_unique_level(int r_idx, struct worldpos *wpos)
{
	int i;
	
	for (i = 1; i <= NumPlayers; i++)
	{
		player_type *p_ptr = Players[i];

		/* Is the player on the level and did he killed the unique already ? */
		if (!p_ptr->r_killed[r_idx] && (inarea(wpos, &p_ptr->wpos)))
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
static bool place_monster_one(struct worldpos *wpos, int y, int x, int r_idx, int ego, int randuni, bool slp, bool clo)
{
	int                     i, Ind, j;

	cave_type               *c_ptr;

	monster_type    *m_ptr;

	monster_race    *r_ptr = &r_info[r_idx];

	char buf[80];

#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return (FALSE);
	/* Verify location */
	if (!in_bounds(y, x)) return (FALSE);
	/* Require empty space */
	if (!cave_empty_bold(zcave, y, x)) return (FALSE);
	/* Hack -- no creation on glyph of warding */
	if (zcave[y][x].feat == FEAT_GLYPH) return (FALSE);

	if(!wpos->wz && wild_info[wpos->wy][wpos->wx].radius < 3 && zcave[y][x].info & CAVE_ICKY) return(FALSE);

#if 0
	/* should be sorted - look above */

	/* Hack -- no creation in town inside house */
	if (!Depth && (cave[Depth][y][x].info & CAVE_ICKY)) return (FALSE);
	/* Hack -- or close to town in wilderness areas */
	if ((Depth<0 && Depth >-16) && (cave[Depth][y][x].info & CAVE_ICKY)) return (FALSE);
#endif
#else
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
#endif



	/* Paranoia */
	if (!r_idx) return (FALSE);

	/* Paranoia */
	if (!r_ptr->name) return (FALSE);


	/* Hack -- "unique" monsters must be "unique" */
	if ((r_ptr->flags1 & RF1_UNIQUE) && ((!allow_unique_level(r_idx, wpos)) || (r_ptr->cur_num >= r_ptr->max_num)))
	{
		/* Cannot create */
		return (FALSE);
	}


	/* Depth monsters may NOT be created out of depth */
	if ((r_ptr->flags1 & RF1_FORCE_DEPTH) && (getlevel(wpos) < r_ptr->level))
	{
		/* Cannot create */
		return (FALSE);
	}

        /* Ego Uniques are NOT to be created */
        if ((r_ptr->flags1 & RF1_UNIQUE) && (ego || randuni)) return 0;

        /* Now could we generate an Ego Monster */
        r_ptr = race_info_idx(r_idx, ego, randuni);



	/* Powerful monster */
	if (r_ptr->level > getlevel(wpos))
	{
		/* Unique monsters */
		if (r_ptr->flags1 & RF1_UNIQUE)
		{
			/* Message for cheaters */
			/*if (cheat_hear) msg_format("Deep Unique (%s).", name);*/
		}

		/* Normal monsters */
		else
		{
			/* Message for cheaters */
			/*if (cheat_hear) msg_format("Deep Monster (%s).", name);*/
		}
	}
#if 0
	/* Note the monster */
	else if (r_ptr->flags1 & RF1_UNIQUE)
	{
		/* Unique monsters induce message */
		/*if (cheat_hear) msg_format("Unique (%s).", name);*/
	}
#endif	// 0


	/* Access the location */
	c_ptr = &zcave[y][x];

	if((r_ptr->flags7 & RF7_AQUATIC) && c_ptr->feat!=FEAT_WATER) return FALSE;

	/* Make a new monster */
	c_ptr->m_idx = m_pop();

	/* Mega-Hack -- catch "failure" */
	if (!c_ptr->m_idx) return (FALSE);


	/* Get a new monster record */
	m_ptr = &m_list[c_ptr->m_idx];

	/* Save the race */
	m_ptr->r_idx = r_idx;
        m_ptr->ego = ego;
//	m_ptr->name3 = randuni;		

	/* Place the monster at the location */
	m_ptr->fy = y;
	m_ptr->fx = x;
	wpcopy(&m_ptr->wpos, wpos);

	m_ptr->special = FALSE;

	/* Hack -- Count the monsters on the level */
	r_ptr->cur_num++;


	/* Hack -- count the number of "reproducers" */
	if (r_ptr->flags4 & RF4_MULTIPLY) num_repro++;


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
		if (i)
		{
			j = rand_spread(0, i);
			m_ptr->mspeed += j;

			if (m_ptr->mspeed < j) m_ptr->mspeed = 255;
		}

//		if (i) m_ptr->mspeed += rand_spread(0, i);
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
	if (getlevel(wpos) >100)
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
#define GROUP_MAX       32


/*
 * Attempt to place a "group" of monsters around the given location
 */
static bool place_monster_group(struct worldpos *wpos, int y, int x, int r_idx, bool slp)
{
	monster_race *r_ptr = &r_info[r_idx];

	int old, n, i;
	int total = 0, extra = 0;

	int hack_n = 0;

	byte hack_y[GROUP_MAX];
	byte hack_x[GROUP_MAX];
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);

	/* Pick a group size */
	total = randint(13);

	/* Hard monsters, small groups */
	if (r_ptr->level > getlevel(wpos))
	{
		extra = r_ptr->level - getlevel(wpos);
		extra = 0 - randint(extra);
	}

	/* Easy monsters, large groups */
	else if (r_ptr->level < getlevel(wpos))
	{
		extra = getlevel(wpos) - r_ptr->level;
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
			if (!cave_empty_bold(zcave, my, mx)) continue;

			/* Attempt to place another monster */
			if (place_monster_one(wpos, my, mx, r_idx, pick_ego_monster(r_idx, getlevel(wpos)), 0, slp, FALSE))
			{
				/* Add it to the "hack" set */
				hack_y[hack_n] = my;
				hack_x[hack_n] = mx;
				hack_n++;
			}
		}
	}

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
bool place_monster_aux(struct worldpos *wpos, int y, int x, int r_idx, bool slp, bool grp, bool clo)
{
	int                     i;

	monster_race    *r_ptr = &r_info[r_idx];

	cave_type **zcave;
	int level=getlevel(wpos);
	if(!(zcave=getcave(wpos))) return(FALSE);

	/* Place one monster, or fail */
	if (!place_monster_one(wpos, y, x, r_idx, pick_ego_monster(r_idx, level), 0, slp, clo)) return (FALSE);

	/* Require the "group" flag */
	if (!grp) return (TRUE);


	/* Friends for certain monsters */
	if (r_ptr->flags1 & RF1_FRIENDS)
	{
		/* Attempt to place a group */
		(void)place_monster_group(wpos, y, x, r_idx, slp);
	}


	/* Escorts for certain monsters */
	if (r_ptr->flags1 & RF1_ESCORT)
	{
		/* Try to place several "escorts" */
		for (i = 0; i < 50; i++)
		{
			int nx, ny, z, d = 3;

			/* Pick a location */
			scatter(wpos, &ny, &nx, y, x, d, 0);
			/* Require empty grids */
			if (!cave_empty_bold(zcave, ny, nx)) continue;

			/* Set the escort index */
			place_monster_idx = r_idx;

			/* Set the escort hook */
			get_mon_num_hook = place_monster_okay;

			/* Prepare allocation table */
			get_mon_num_prep();


			/* Pick a random race */
			z = get_mon_num(r_ptr->level);


			/* Remove restriction */
			get_mon_num_hook = dungeon_aux;

			/* Prepare allocation table */
			get_mon_num_prep();


			/* Handle failure */
			if (!z) break;

			/* Place a single escort */
			(void)place_monster_one(wpos, ny, nx, z, pick_ego_monster(z, getlevel(wpos)), 0, slp, FALSE);

			/* Place a "group" of escorts if needed */
			if ((r_info[z].flags1 & RF1_FRIENDS) ||
			    (r_ptr->flags1 & RF1_ESCORTS))
			{
				/* Place a group of monsters */
				(void)place_monster_group(wpos, ny, nx, z, slp);
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
bool place_monster(struct worldpos *wpos, int y, int x, bool slp, bool grp)
{
	int r_idx;
	struct dungeon_type *d_ptr;

	d_ptr=getdungeon(wpos);
	/* Specific filter - should be made more useful */
	/* */
	 
	if(d_ptr && (d_ptr->r_char[0] || d_ptr->nr_char[0])){
		int i;
		monster_race *r_ptr;
		while((r_idx=get_mon_num(monster_level))){
			r_ptr=&r_info[r_idx];
			if(d_ptr->r_char[0]){
                		for (i = 0; i < 10; i++)
                		{
                        		if (r_ptr->d_char == d_ptr->r_char[i]) break;
                		}
				if(i<10) break;
				continue;
			}
			if(d_ptr->nr_char[0]){
                		for (i = 0; i < 10; i++)
                		{
                        		if (r_ptr->d_char == d_ptr->r_char[i]) continue;
                		}
				break;
			}
		}
	}
	else{
		/* Pick a monster */
		r_idx = get_mon_num(monster_level);
	}

	/* Handle failure */
	if (!r_idx) return (FALSE);

	/* Attempt to place the monster */
	if (place_monster_aux(wpos, y, x, r_idx, slp, grp, FALSE)) return (TRUE);

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
bool alloc_monster(struct worldpos *wpos, int dis, int slp)
{
	int                     y, x, i, d, min_dis = 999;
	int                     tries = 0;
	player_type *p_ptr;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);

	/* Find a legal, distant, unoccupied, space */
	while (tries < 50)
	{
		/* Increase the counter */
		tries++;

		/* Pick a location */
		y = rand_int(getlevel(wpos) ? MAX_HGT : MAX_HGT);
		x = rand_int(getlevel(wpos) ? MAX_WID : MAX_WID);
		
		/* Require "naked" floor grid */
		if (!cave_naked_bold(zcave, y, x)) continue;

		/* Accept far away grids */
		for (i = 1; i < NumPlayers + 1; i++)
		{
			p_ptr = Players[i];

			/* Skip him if he's not playing */
			if (p_ptr->conn == NOT_CONNECTED)
				continue;

			/* Skip him if he's not on this depth */
			if(!inarea(wpos, &p_ptr->wpos))
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
	if (place_monster(wpos, y, x, slp, TRUE)) return (TRUE);

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

		/* PernA-addition
		 * many of them are unused for now.	- Jir -
		 * */
		case SUMMON_BIZARRE1:
		{
			okay = ((r_ptr->d_char == 'm') &&
			       !(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}
		case SUMMON_BIZARRE2:
		{
			okay = ((r_ptr->d_char == 'b') &&
			       !(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}
		case SUMMON_BIZARRE3:
		{
			okay = ((r_ptr->d_char == 'Q') &&
			       !(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_BIZARRE4:
		{
			okay = ((r_ptr->d_char == 'v') &&
			       !(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_BIZARRE5:
		{
			okay = ((r_ptr->d_char == '$') &&
			       !(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_BIZARRE6:
		{
			okay = (((r_ptr->d_char == '!') ||
			         (r_ptr->d_char == '?') ||
			         (r_ptr->d_char == '=') ||
			         (r_ptr->d_char == '$') ||
			         (r_ptr->d_char == '|')) &&
			        !(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

                case SUMMON_HI_DEMON:
		{
			okay = ((r_ptr->flags3 & (RF3_DEMON)) &&
                                (r_ptr->d_char == 'U') &&
			       !(r_ptr->flags1 & RF1_UNIQUE));
			break;
		}

#if 1	// welcome, Julian Lighton Hack ;)
		case SUMMON_KIN:
		{
			okay = ((r_ptr->d_char == summon_kin_type) &&
			       !(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}
#endif	// 0

		case SUMMON_DAWN:
		{
			okay = ((strstr((r_name + r_ptr->name),"the Dawn")) &&
			       !(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_ANIMAL:
		{
			okay = ((r_ptr->flags3 & (RF3_ANIMAL)) &&
			       !(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_ANIMAL_RANGER:
		{
			okay = ((r_ptr->flags3 & (RF3_ANIMAL)) &&
			       (strchr("abcflqrwBCIJKMRS", r_ptr->d_char)) &&
			       !(r_ptr->flags3 & (RF3_DRAGON))&&
			       !(r_ptr->flags3 & (RF3_EVIL)) &&
			       !(r_ptr->flags3 & (RF3_UNDEAD))&&
			       !(r_ptr->flags3 & (RF3_DEMON)) &&
			       !(r_ptr->flags4 || r_ptr->flags5 || r_ptr->flags6) &&
			       !(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_HI_UNDEAD_NO_UNIQUES:
		{
			okay = (((r_ptr->d_char == 'L') ||
			         (r_ptr->d_char == 'V') ||
			         (r_ptr->d_char == 'W')) &&
			        !(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_HI_DRAGON_NO_UNIQUES:
		{
			okay = ((r_ptr->d_char == 'D') &&
			       !(r_ptr->flags1 &(RF1_UNIQUE)));
			break;
		}

		case SUMMON_NO_UNIQUES:
		{
			okay = (!(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_PHANTOM:
		{
			okay = ((strstr((r_name + r_ptr->name),"Phantom")) &&
			       !(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_ELEMENTAL:
		{
			okay = ((strstr((r_name + r_ptr->name),"lemental")) &&
			       !(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}
                case SUMMON_DRAGONRIDER:
		{
                        okay = (r_ptr->flags3 & RF3_DRAGONRIDER)?TRUE:FALSE;
			break;
		}

		case SUMMON_BLUE_HORROR:
		{
			okay = ((strstr((r_name + r_ptr->name),"lue horror")) &&
			       !(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

                case SUMMON_BUG:
		{
                        okay = ((strstr((r_name + r_ptr->name),"Software bug")) &&
			       !(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

                case SUMMON_RNG:
		{
                        okay = ((strstr((r_name + r_ptr->name),"Random Number Generator")) &&
			       !(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}
                case SUMMON_MINE:
		{
                        okay = (r_ptr->flags1 & RF1_NEVER_MOVE)?TRUE:FALSE;
			break;
		}

                case SUMMON_HUMAN:
		{
                        okay = ((r_ptr->d_char == 'p') &&
			        !(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_SHADOWS:
		{
			okay = ((r_ptr->d_char == 'G') &&
			        !(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_QUYLTHULG:
		{
			okay = ((r_ptr->d_char == 'Q') &&
				!(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_VERMIN:
		{
			okay = (r_ptr->flags4 & RF4_MULTIPLY)?TRUE:FALSE;
			break;
		}

#ifdef USE_LUA
#if 0	// let's leave it to DG :)
		case SUMMON_LUA:
		{
			okay = summon_lua_okay(r_idx);
			break;
		}
#endif
#endif

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
bool summon_specific(struct worldpos *wpos, int y1, int x1, int lev, int type)
{
	int i, x, y, r_idx;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);

	/* Look for a location */
	for (i = 0; i < 20; ++i)
	{
		/* Pick a distance */
		int d = (i / 15) + 1;

		/* Pick a location */
		scatter(wpos, &y, &x, y1, x1, d, 0);

		/* Require "empty" floor grid */
		if (!cave_empty_bold(zcave, y, x)) continue;

		/* Hack -- no summon on glyph of warding */
		if (zcave[y][x].feat == FEAT_GLYPH) continue;

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
#ifdef NEW_DUNGEON
//	r_idx = (lev != 100)?get_mon_num((getlevel(wpos) + lev) / 2 + 5) : 100;
	r_idx = get_mon_num((lev != 100)?(getlevel(wpos) + lev) / 2 + 5 : 100);
#else
	r_idx = (lev != 100)?get_mon_num((Depth + lev) / 2 + 5) : 100;
#endif


	/* Remove restriction */
	get_mon_num_hook = dungeon_aux;

	/* Prepare allocation table */
	get_mon_num_prep();


	/* Handle failure */
	if (!r_idx) return (FALSE);

	/* Attempt to place the monster (awake, allow groups) */
	if (!place_monster_aux(wpos, y, x, r_idx, FALSE, TRUE, FALSE)) return (FALSE);

	/* Success */
	return (TRUE);
}

/* summon a specific race near this location */
/* summon until we can't find a location or we have summoned size */
bool summon_specific_race(struct worldpos *wpos, int y1, int x1, int r_idx, unsigned char size)
{
	int c, i, x, y;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);

	/* Handle failure */
	if (!r_idx) return (FALSE);

	/* for each monster we are summoning */

	for (c = 0; c < size; c++)
	{       

		/* Look for a location */
		for (i = 0; i < 200; ++i)
		{
			/* Pick a distance */
			int d = (i / 15) + 1;

			/* Pick a location */
			scatter(wpos, &y, &x, y1, x1, d, 0);

			/* Require "empty" floor grid */
			if (!cave_empty_bold(zcave, y, x)) continue;

			/* Hack -- no summon on glyph of warding */
			if (zcave[y][x].feat == FEAT_GLYPH) continue;

			/* Okay */
			break;
		}

		/* Failure */
		if (i == 20) return (FALSE);

		/* Attempt to place the monster (awake, don't allow groups) */
		if (!place_monster_aux(wpos, y, x, r_idx, FALSE, FALSE, FALSE))
			return (FALSE);

	}

	/* Success */
	return (TRUE);
}


/* summon a specific race at a random location */
bool summon_specific_race_somewhere(struct worldpos *wpos, int r_idx, unsigned char size)
{
	int                     y, x;
	int                     tries = 0;

	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);

	/* Find a legal, distant, unoccupied, space */
	while (tries < 50)
	{
		/* Increase the counter */
		tries++;

		/* Pick a location */
		y = rand_int(getlevel(wpos) ? MAX_HGT : MAX_HGT);
		x = rand_int(getlevel(wpos) ? MAX_WID : MAX_WID);

		/* Require "naked" floor grid */
		if (!cave_naked_bold(zcave, y, x)) continue;


		/* Abort */
		if (tries >= 50)
			return (FALSE);

		/* We have a valid location */
		break;
	}

	/* Attempt to place the monster */
	if (summon_specific_race(wpos, y, x, r_idx, size)) return TRUE;
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
        monster_race    *r_ptr = race_inf(m_ptr);

	int                     i, y, x;

	bool result = FALSE;
	struct worldpos *wpos=&m_ptr->wpos;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);

	/* NO UNIQUES */
	if (r_ptr->flags1 & RF1_UNIQUE) return FALSE;

	/* Try up to 18 times */
	for (i = 0; i < 18; i++)
	{
		int d = 1;

		/* Pick a location */
		scatter(&m_ptr->wpos, &y, &x, m_ptr->fy, m_ptr->fx, d, 0);
		/* Require an "empty" floor grid */
		if (!cave_empty_bold(zcave, y, x)) continue;
		/* Create a new monster (awake, no groups) */
		result = place_monster_aux(&m_ptr->wpos, y, x, m_ptr->r_idx, FALSE, FALSE, TRUE);

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
	long                    oldhp, newhp, tmp;
	int                             percentage;

	monster_type		*m_ptr = &m_list[m_idx];
        monster_race            *r_ptr = race_inf(m_ptr);

	char                    m_name[80];


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

        monster_race *r_ptr = race_inf(m_ptr);


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
		if (p_ptr->immune_neth) m_ptr->smart |= SM_RES_NETH;
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
	cave_type **zcave;
	
	for (i = 0; i < m_max; i++)
	{
		r_ptr = &m_list[i];
		/* setup the cave m_idx if the level has been 
		 * allocated.
		 */
		if((zcave=getcave(&r_ptr->wpos)))
			zcave[r_ptr->fy][r_ptr->fx].m_idx = i;
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
	/* golem? */
	if (m_ptr->special) return (m_ptr->r_ptr);
//	else return (&r_info[m_ptr->r_idx]);
	else return (race_info_idx((m_ptr)->r_idx, (m_ptr)->ego, (m_ptr)->name3));
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
#ifdef RANDUNIS
		else if(m_ptr->ego)
		{
			if (re_info[m_ptr->ego].before)
			{
				sprintf(buf, "%s %s", re_name + re_info[m_ptr->ego].name,
						r_name + r_info[m_ptr->r_idx].name);
			}
			else
			{
				sprintf(buf, "%s %s", r_name + r_info[m_ptr->r_idx].name,
						re_name + re_info[m_ptr->ego].name);
			}
			return (buf);
		}
#endif	// RANDUNIS
        else return (r_name + r_info[m_ptr->r_idx].name);
}

#ifdef RANDUNIS
/* Will add, sub, .. */
static s32b modify_aux(s32b a, s32b b, char mod)
{
        switch (mod)
        {
                case MEGO_ADD:
                        return (a + b);
                        break;
                case MEGO_SUB:
                        return (a - b);
                        break;
                case MEGO_FIX:
                        return (b);
                        break;
                case MEGO_PRC:
                        return (a * b / 100);
                        break;
                default:
                        s_printf("WARNING, unmatching MEGO(%d).", mod);
                        return (0);
        }
}

#define MODIFY_AUX(o, n) ((o) = modify_aux((o), (n) >> 2, (n) & 3))
#define MODIFY(o, n, min) MODIFY_AUX(o, n); (o) = ((o) < (min))?(min):(o)

/* Is this ego ok for this monster ? */
bool mego_ok(int r_idx, int ego)
{
        monster_ego *re_ptr = &re_info[ego];
        monster_race *r_ptr = &r_info[r_idx];
        bool ok = FALSE;
        int i;

		/* missing number */
		if (!re_ptr->name) return FALSE;

        /* needed flags */
        if (re_ptr->flags1 && ((re_ptr->flags1 & r_ptr->flags1) != re_ptr->flags1)) return FALSE;
        if (re_ptr->flags2 && ((re_ptr->flags2 & r_ptr->flags2) != re_ptr->flags2)) return FALSE;
        if (re_ptr->flags3 && ((re_ptr->flags3 & r_ptr->flags3) != re_ptr->flags3)) return FALSE;
#if 1
        if (re_ptr->flags7 && ((re_ptr->flags7 & r_ptr->flags7) != re_ptr->flags7)) return FALSE;
        if (re_ptr->flags8 && ((re_ptr->flags8 & r_ptr->flags8) != re_ptr->flags8)) return FALSE;
        if (re_ptr->flags9 && ((re_ptr->flags9 & r_ptr->flags9) != re_ptr->flags9)) return FALSE;
#endif

        /* unwanted flags */
        if (re_ptr->hflags1 && (re_ptr->hflags1 & r_ptr->flags1)) return FALSE;
        if (re_ptr->hflags2 && (re_ptr->hflags2 & r_ptr->flags2)) return FALSE;
        if (re_ptr->hflags3 && (re_ptr->hflags3 & r_ptr->flags3)) return FALSE;
#if 1
        if (re_ptr->hflags7 && (re_ptr->hflags7 & r_ptr->flags7)) return FALSE;
        if (re_ptr->hflags8 && (re_ptr->hflags8 & r_ptr->flags8)) return FALSE;
        if (re_ptr->hflags9 && (re_ptr->hflags9 & r_ptr->flags9)) return FALSE;
#endif

        /* Need good race -- IF races are specified */
        if (re_ptr->r_char[0])
        {
                for (i = 0; i < 10; i++)
                {
                        if (r_ptr->d_char == re_ptr->r_char[i]) ok = TRUE;
                }
                if (!ok) return FALSE;
        }
        if (re_ptr->nr_char[0])
        {
                for (i = 0; i < 10; i++)
                {
                        if (r_ptr->d_char == re_ptr->nr_char[i]) return (FALSE);
                }
        }

        /* Passed all tests ? */
        return TRUE;
}

/* Choose an ego type */
/* 'Level' is a transitional index for dun_depth.	- Jir -
 */
int pick_ego_monster(int r_idx, int Level)
{
        /* Assume no ego */
        int ego = 0, lvl;
        int tries = MAX_RE_IDX + 10;
        monster_ego *re_ptr;

        /* No townspeople ego */
        if (!r_info[r_idx].level) return 0;

        /* First are we allowed to find an ego */
        if (!magik(MEGO_CHANCE)) return 0;

        /* Ego Uniques are NOT to be created */
        if ((r_info[r_idx].flags1 & RF1_UNIQUE)) return 0;

        /* Lets look for one */
        while(tries--)
        {
                /* Pick one */
                ego = rand_range(1, MAX_RE_IDX - 1);
                re_ptr = &re_info[ego];

                /*  No hope so far */
                if (!mego_ok(r_idx, ego)) continue;

                /* Not too much OoD */
                lvl = r_info[r_idx].level;
                MODIFY(lvl, re_ptr->level, 0);
                lvl -= ((Level / 2) + (rand_int(Level / 2)));
                if (lvl < 1) lvl = 1;
                if (rand_int(lvl)) continue;

				/* Hack -- Depth monsters may NOT be created out of depth */
				if ((re_ptr->mflags1 & RF1_FORCE_DEPTH) && (lvl > 1))
				{
					/* Cannot create */
					continue;
				}

                /* Each ego types have a rarity */
                if (rand_int(re_ptr->rarity)) continue;

				/* (Remove me) */
				/* s_printf("ego %d(%s)(%s) is generated.\n", ego, re_name + re_ptr->name, r_name + r_info[r_idx].name); */

                /* We finanly got one ? GREAT */
                return ego;
        }

        /* Found none ? so sad, well no ego for the time being */
        return 0;
}

/* Pick a randuni (not implemented yet) */
int pick_randuni(int r_idx, int Level)
{
	return 0;
#if 0
        /* Assume no ego */
        int ego = 0, lvl;
        int tries = max_re_idx + 10;
        monster_ego *re_ptr;

        /* No townspeople ego */
        if (!r_info[r_idx].level) return 0;

        /* First are we allowed to find an ego */
        if (!magik(MEGO_CHANCE)) return 0;


        /* Lets look for one */
        while(tries--)
        {
                /* Pick one */
                ego = rand_range(1, max_re_idx - 1);
                re_ptr = &re_info[ego];

                /*  No hope so far */
                if (!mego_ok(r_idx, ego)) continue;

                /* Not too much OoD */
                lvl = r_info[r_idx].level;
                MODIFY(lvl, re_ptr->level, 0);
                lvl -= ((dun_level / 2) + (rand_int(dun_level / 2)));
                if (lvl < 1) lvl = 1;
                if (rand_int(lvl)) continue;

                /* Each ego types have a rarity */
                if (rand_int(re_ptr->rarity)) continue;

                /* We finanly got one ? GREAT */
                return ego;
        }

        /* Found none ? so sad, well no ego for the time being */
        return 0;
#endif
}

/*
 * Return a (monster_race*) with the combinaison of the monster
 * proprieties, the ego type and randuni id.
 * (randuni parts are not done yet..	- Jir -)
 */
static monster_race* race_info_idx(int r_idx, int ego, int randuni)
{
        static monster_race race;
        monster_ego *re_ptr = &re_info[ego];
        monster_race *r_ptr = &r_info[r_idx], *nr_ptr = &race;
        int i;

        /* No work needed */
        if (!ego) return r_ptr;

        /* Copy the base monster */
        COPY(nr_ptr, r_ptr, monster_race);

        /* Adjust the values */
        for (i = 0; i < 4; i++)
        {
                s32b j, k;

                j = modify_aux(nr_ptr->blow[i].d_dice, re_ptr->blow[i].d_dice, re_ptr->blowm[i][0]);
                if (j < 0) j = 0;
                if (j > 255) j = 255;
                k = modify_aux(nr_ptr->blow[i].d_side, re_ptr->blow[i].d_side, re_ptr->blowm[i][1]);
                if (k < 0) k = 0;
                if (k > 255) k = 255;

                nr_ptr->blow[i].d_dice = j;
                nr_ptr->blow[i].d_side = k;

                if (re_ptr->blow[i].method) nr_ptr->blow[i].method = re_ptr->blow[i].method;
                if (re_ptr->blow[i].effect) nr_ptr->blow[i].effect = re_ptr->blow[i].effect;
        }

        MODIFY(nr_ptr->hdice, re_ptr->hdice, 1);
        MODIFY(nr_ptr->hside, re_ptr->hside, 1);

        MODIFY(nr_ptr->ac, re_ptr->ac, 0);

        MODIFY(nr_ptr->sleep, re_ptr->sleep, 0);

        MODIFY(nr_ptr->aaf, re_ptr->aaf, 1);

		/* Hack - avoid to be back to 0 (consider using s16b) */
		i = nr_ptr->speed;
        MODIFY(i, re_ptr->speed, 50);
        MODIFY(nr_ptr->speed, re_ptr->speed, 50);

		if (nr_ptr->speed < i) nr_ptr->speed = 255;

        MODIFY(nr_ptr->mexp, re_ptr->mexp, 0);

//        MODIFY(nr_ptr->weight, re_ptr->weight, 10);

        nr_ptr->freq_inate = re_ptr->freq_inate;
        nr_ptr->freq_spell = re_ptr->freq_spell;

        MODIFY(nr_ptr->level, re_ptr->level, 1);

        /* Take off some flags */
        nr_ptr->flags1 &= ~(re_ptr->nflags1);
        nr_ptr->flags2 &= ~(re_ptr->nflags2);
        nr_ptr->flags3 &= ~(re_ptr->nflags3);
        nr_ptr->flags4 &= ~(re_ptr->nflags4);
        nr_ptr->flags5 &= ~(re_ptr->nflags5);
        nr_ptr->flags6 &= ~(re_ptr->nflags6);
        nr_ptr->flags7 &= ~(re_ptr->nflags7);
        nr_ptr->flags8 &= ~(re_ptr->nflags8);
        nr_ptr->flags9 &= ~(re_ptr->nflags9);

        /* Add some flags */
        nr_ptr->flags1 |= re_ptr->mflags1;
        nr_ptr->flags2 |= re_ptr->mflags2;
        nr_ptr->flags3 |= re_ptr->mflags3;
        nr_ptr->flags4 |= re_ptr->mflags4;
        nr_ptr->flags5 |= re_ptr->mflags5;
        nr_ptr->flags6 |= re_ptr->mflags6;
        nr_ptr->flags7 |= re_ptr->mflags7;
        nr_ptr->flags8 |= re_ptr->mflags8;
        nr_ptr->flags9 |= re_ptr->mflags9;

        /* Change the char/attr is needed */
        if (re_ptr->d_char != MEGO_CHAR_ANY)
        {
                nr_ptr->d_char = re_ptr->d_char;
                nr_ptr->x_char = re_ptr->d_char;
        }
        if (re_ptr->d_attr != MEGO_CHAR_ANY)
        {
                nr_ptr->d_attr = re_ptr->d_attr;
                nr_ptr->x_attr = re_ptr->d_attr;
        }

        /* And finanly return a pointer to a fully working monster race */
        return nr_ptr;
}

#endif	// RANDUNIS
