/* File: cmd2.c */

/* Purpose: Movement commands (part 2) */

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
 * Go up one level                                      -RAK-
 */
void do_cmd_go_up(int Ind)
{
	player_type *p_ptr = Players[Ind];
	cave_type *c_ptr;
#ifdef NEW_DUNGEON        
	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#else
	int Depth = p_ptr->dun_depth;
#endif

	/* Make sure he hasn't just changed depth */
	if (p_ptr->new_level_flag)
		return;

	/* Player grid */
#ifdef NEW_DUNGEON
	c_ptr = &zcave[p_ptr->py][p_ptr->px];
#else
	c_ptr = &cave[Depth][p_ptr->py][p_ptr->px];
#endif

	/* Verify stairs if not a ghost, or admin wizard */
	if (!p_ptr->ghost && (strcmp(p_ptr->name,cfg_admin_wizard)) && c_ptr->feat != FEAT_LESS && !p_ptr->prob_travel)
	{
		msg_print(Ind, "I see no up staircase here.");
		return;
	}
#ifdef NEW_DUNGEON
	if(wpos->wz==0 && !(wild_info[wpos->wy][wpos->wx].flags&WILD_F_UP))
#else
	if (p_ptr->dun_depth <= 0)
#endif                
	{
		msg_print(Ind, "There is nothing above you.");
		return;
	}
#ifdef NEW_DUNGEON
	if(wpos->wz>0 && wild_info[wpos->wy][wpos->wx].tower->maxdepth==wpos->wz){
		msg_print(Ind,"You are at the top of the tower!");
		return;
	}
#endif

	/* Remove the player from the old location */
	c_ptr->m_idx = 0;

	/* Show everyone that's he left */
#ifdef NEW_DUNGEON        
	everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);
#else        
	everyone_lite_spot(Depth, p_ptr->py, p_ptr->px);
#endif

	/* Forget his lite and viewing area */
	forget_lite(Ind);
	forget_view(Ind);

	/* Hack -- take a turn */
#ifdef NEW_DUNGEON
	p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
	p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif
	/* Success */
	if (c_ptr->feat == FEAT_LESS)
	{
		msg_print(Ind, "You enter a maze of up staircases.");
		p_ptr->new_level_method = LEVEL_UP;
	}
	else
	{
		msg_print(Ind, "You float upwards.");
		p_ptr->new_level_method = LEVEL_GHOST;
	}

	/* A player has left this depth */
#ifdef NEW_DUNGEON
	new_players_on_depth(wpos,-1,TRUE);
	wpos->wz++;
	new_players_on_depth(wpos,1,TRUE);
#else
	players_on_depth[p_ptr->dun_depth]--;
	/* Go up the stairs */
	p_ptr->dun_depth--;

	/* And another player has entered this depth */
	players_on_depth[p_ptr->dun_depth]++;
#endif

	p_ptr->new_level_flag = TRUE;

	/* He'll be safe for 2 turns */
	set_invuln_short(Ind, 2);

	/* Create a way back */
	create_down_stair = TRUE;
}


/*
 * Go down one level
 */
void do_cmd_go_down(int Ind)
{
	player_type *p_ptr = Players[Ind];
	cave_type *c_ptr;
#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#else
	int Depth = p_ptr->dun_depth;
#endif

	/* Make sure he hasn't just changed depth */
	if (p_ptr->new_level_flag)
		return;

	/* Player grid */
#ifdef NEW_DUNGEON
	c_ptr = &zcave[p_ptr->py][p_ptr->px];
#else
	c_ptr = &cave[Depth][p_ptr->py][p_ptr->px];
#endif

	/* Verify stairs */
//      if (!p_ptr->ghost && (strcmp(p_ptr->name,cfg_admin_wizard)) && c_ptr->feat != FEAT_MORE && !p_ptr->prob_travel)
	if ((strcmp(p_ptr->name,cfg_admin_wizard)) && c_ptr->feat != FEAT_MORE && !p_ptr->prob_travel)
	{
		if (!p_ptr->ghost)
		{
			msg_print(Ind, "I see no down staircase here.");
			return;
		}
#ifdef NEW_DUNGEON
		else if (p_ptr->max_dlv + 5 <= getlevel(&p_ptr->wpos))
#else
		else if (p_ptr->max_dlv + 5 <= p_ptr->dun_depth)
#endif
		{
			/* anti Ghost-dive */
			msg_print(Ind, "A mysterious force prevents you from going down.");
			return;
		}
	}
#ifdef NEW_DUNGEON
	if (wpos->wz==0 && !(wild_info[wpos->wy][wpos->wx].flags&WILD_F_DOWN))
#else
	/* Can't go down in the wilderness */
	if (p_ptr->dun_depth < 0)
#endif
	{
		msg_print(Ind, "There is nothing below you.");
		return;
	}
	
	/* Verify maximum depth */
#ifdef NEW_DUNGEON
	if(wpos->wz<0 && wild_info[wpos->wy][wpos->wx].dungeon->maxdepth==-wpos->wz)
#else
	if (p_ptr->dun_depth >= MAX_DEPTH - 1)
#endif
	{
		msg_print(Ind, "You are at the bottom of the dungeon.");
		return;
	}

	/* Remove the player from the old location */
	c_ptr->m_idx = 0;

	/* Show everyone that's he left */
#ifdef NEW_DUNGEON
	everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);
#else
	everyone_lite_spot(Depth, p_ptr->py, p_ptr->px);
#endif

	/* Forget his lite and viewing area */
	forget_lite(Ind);
	forget_view(Ind);

	/* Hack -- take a turn */
#ifdef NEW_DUNGEON
	p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
	p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif

	/* Success */
	if (c_ptr->feat == FEAT_MORE)
	{
		msg_print(Ind, "You enter a maze of down staircases.");
		p_ptr->new_level_method = LEVEL_DOWN;
	}
	else
	{
		msg_print(Ind, "You float downwards.");
		p_ptr->new_level_method = LEVEL_GHOST;
	}

	/* A player has left this depth */
#ifdef NEW_DUNGEON
	new_players_on_depth(wpos,-1,TRUE);
	wpos->wz--;
	new_players_on_depth(wpos,1,TRUE);
#else
	players_on_depth[p_ptr->dun_depth]--;
	/* Go up the stairs */
	p_ptr->dun_depth--;

	/* And another player has entered this depth */
	players_on_depth[p_ptr->dun_depth]++;
#endif

	p_ptr->new_level_flag = TRUE;

    /* He'll be safe for 2 turns */
    set_invuln_short(Ind, 2);

	/* Create a way back */
	create_up_stair = TRUE;
}



/*
 * Simple command to "search" for one turn
 */
void do_cmd_search(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		/*command_rep = command_arg - 1;*/

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Take a turn */
#ifdef NEW_DUNGEON
	p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
	p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif

	/* Search */
	search(Ind);
}


/*
 * Hack -- toggle search mode
 */
void do_cmd_toggle_search(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* Stop searching */
	if (p_ptr->searching)
	{
		/* Clear the searching flag */
		p_ptr->searching = FALSE;

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);
	}

	/* Start searching */
	else
	{
		/* Set the searching flag */
		p_ptr->searching = TRUE;

		/* Update stuff */
		p_ptr->update |= (PU_BONUS);

		/* Redraw stuff */
		p_ptr->redraw |= (PR_STATE | PR_SPEED);
	}
}



/*
 * Allocates objects upon opening a chest    -BEN-
 *
 * Disperse treasures from the chest "o_ptr", centered at (x,y).
 *
 * Small chests often contain "gold", while Large chests always contain
 * items.  Wooden chests contain 2 items, Iron chests contain 4 items,
 * and Steel chests contain 6 items.  The "value" of the items in a
 * chest is based on the "power" of the chest, which is in turn based
 * on the level on which the chest is generated.
 */
static void chest_death(int Ind, int y, int x, object_type *o_ptr)
{
	player_type *p_ptr = Players[Ind];
#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
#else
	int Depth = p_ptr->dun_depth;
#endif

	int             i, d, ny, nx;
	int             number, small;

#ifdef NEW_DUNGEON
	if(!(zcave=getcave(wpos))) return;
#endif

	/* Must be a chest */
	if (o_ptr->tval != TV_CHEST) return;

	/* Small chests often hold "gold" */
	small = (o_ptr->sval < SV_CHEST_MIN_LARGE);

	/* Determine how much to drop (see above) */
	number = (o_ptr->sval % SV_CHEST_MIN_LARGE) * 2;

	/* Generate some treasure */
	if (o_ptr->pval && (number > 0))
	{
		/* Drop some objects (non-chests) */
		for (; number > 0; --number)
		{
			/* Try 20 times per item */
			for (i = 0; i < 20; ++i)
			{
				/* Pick a distance */
				d = ((i + 15) / 15);

				/* Pick a location */
#ifdef NEW_DUNGEON
				scatter(wpos, &ny, &nx, y, x, d, 0);
				/* Must be a clean floor grid */
				if (!cave_clean_bold(zcave, ny, nx)) continue;
#else
				scatter(Depth, &ny, &nx, y, x, d, 0);
				/* Must be a clean floor grid */
				if (!cave_clean_bold(Depth, ny, nx)) continue;
#endif

				/* Opening a chest */
				opening_chest = TRUE;

				/* Determine the "value" of the items */
				object_level = ABS(o_ptr->pval) + 10;

				/* Small chests often drop gold */
				if (small && (rand_int(100) < 75))
				{
#ifdef NEW_DUNGEON
					place_gold(wpos, ny, nx);
#else
					place_gold(Depth, ny, nx);
#endif
				}

				/* Otherwise drop an item */
				else
				{
#ifdef NEW_DUNGEON
					place_object(wpos, ny, nx, FALSE, FALSE);
#else
					place_object(Depth, ny, nx, FALSE, FALSE);
#endif
				}

				/* Reset the object level */
#ifdef NEW_DUNGEON
				object_level = getlevel(wpos);
#else
				object_level = Depth;
#endif

				/* No longer opening a chest */
				opening_chest = FALSE;

				/* Notice it */
				note_spot(Ind, ny, nx);

				/* Display it */
#ifdef NEW_DUNGEON
				everyone_lite_spot(wpos, ny, nx);
#else
				everyone_lite_spot(Depth, ny, nx);
#endif

				/* Under the player */
				if ((ny == p_ptr->py) && (nx == p_ptr->px))
				{
					msg_print(Ind, "\377GYou feel something roll beneath your feet.");
				}

				/* Successful placement */
				break;
			}
		}
	}

	/* Empty */
	o_ptr->pval = 0;

	/* Known */
	object_known(o_ptr);
}


/*
 * Chests have traps too.
 *
 * Exploding chest destroys contents (and traps).
 * Note that the chest itself is never destroyed.
 */
static void chest_trap(int Ind, int y, int x, object_type *o_ptr)
{
	player_type *p_ptr = Players[Ind];

	int  i, trap;


	/* Only analyze chests */
	if (o_ptr->tval != TV_CHEST) return;

	/* Ignore disarmed chests */
	if (o_ptr->pval <= 0) return;

	/* Obtain the traps */
	trap = chest_traps[o_ptr->pval];

	/* Lose strength */
	if (trap & CHEST_LOSE_STR)
	{
		msg_print(Ind, "A small needle has pricked you!");
		take_hit(Ind, damroll(1, 4), "a poison needle");
		(void)do_dec_stat(Ind, A_STR);
	}

	/* Lose constitution */
	if (trap & CHEST_LOSE_CON)
	{
		msg_print(Ind, "A small needle has pricked you!");
		take_hit(Ind, damroll(1, 4), "a poison needle");
		(void)do_dec_stat(Ind, A_CON);
	}

	/* Poison */
	if (trap & CHEST_POISON)
	{
		msg_print(Ind, "A puff of green gas surrounds you!");
		if (!(p_ptr->resist_pois || p_ptr->oppose_pois))
		{
			(void)set_poisoned(Ind, p_ptr->poisoned + 10 + randint(20));
		}
	}

	/* Paralyze */
	if (trap & CHEST_PARALYZE)
	{
		msg_print(Ind, "A puff of yellow gas surrounds you!");
		if (!p_ptr->free_act)
		{
			(void)set_paralyzed(Ind, p_ptr->paralyzed + 10 + randint(20));
		}
	}

	/* Summon monsters */
	if (trap & CHEST_SUMMON)
	{
		int num = 2 + randint(3);
		msg_print(Ind, "You are enveloped in a cloud of smoke!");
		for (i = 0; i < num; i++)
		{
#ifdef NEW_DUNGEON
			(void)summon_specific(&p_ptr->wpos, y, x, getlevel(&p_ptr->wpos), 0);
#else
			(void)summon_specific(p_ptr->dun_depth, y, x, p_ptr->dun_depth, 0);
#endif
		}
	}

	/* Explode */
	if (trap & CHEST_EXPLODE)
	{
		msg_print(Ind, "There is a sudden explosion!");
		msg_print(Ind, "Everything inside the chest is destroyed!");
		o_ptr->pval = 0;
		take_hit(Ind, damroll(5, 8), "an exploding chest");
	}
}


/*
 * Return the index of a house given an coordinate pair
 */
#ifdef NEW_DUNGEON
int pick_house(struct worldpos *wpos, int y, int x)
#else
int pick_house(int Depth, int y, int x)
#endif
{
	int i;

	/* Check each house */
	for (i = 0; i < num_houses; i++)
	{
		/* Check this one */
#ifdef NEWHOUSES
#ifdef NEW_DUNGEON
		if (houses[i].dx == x && houses[i].dy == y && inarea(&houses[i].wpos, wpos))
#else
		if (houses[i].dx == x && houses[i].dy == y && houses[i].depth == Depth)
#endif
#else
		if (houses[i].door_x == x && houses[i].door_y == y && houses[i].depth == Depth)
#endif
		{
			/* Return */
			return i;
		}
	}

	/* Failure */
	return -1;
}

/* Door change permissions */
bool chmod_door(int Ind, struct dna_type *dna, char *args){
	player_type *p_ptr=Players[Ind];
	if(strcmp(p_ptr->name, cfg_admin_wizard) && strcmp(p_ptr->name, cfg_dungeon_master)){
#ifdef NEWHOUSES
		if(dna->creator!=p_ptr->dna) return(FALSE);
#endif
		/* more security needed... */
	}

	if((dna->a_flags=args[1])){
		dna->min_level=atoi(&args[2]);
	}
	return(TRUE);
}

/* Door change ownership */
bool chown_door(int Ind, struct dna_type *dna, char *args){
	player_type *p_ptr=Players[Ind];
	int newowner=-1;
	int i;
	if(strcmp(p_ptr->name, cfg_admin_wizard) && strcmp(p_ptr->name, cfg_dungeon_master)){
#ifdef NEWHOUSES
		if(dna->creator!=p_ptr->dna) return(FALSE);
#endif
		if(args[1]>='3') return(FALSE);
		/* maybe make party leader only chown party */
	}
	switch(args[1]){
		case '1':
			newowner=lookup_player_id(&args[2]);
			if(!newowner) newowner=-1;
			break;
		case '2':
			newowner=party_lookup(&args[2]);
			break;
		case '3':
			for(i=0;i<MAX_CLASS;i++){
				if(!strcmp(&args[2],class_info[i].title))
					newowner=i;
			}
			break;
		case '4':
			for(i=0;i<MAX_RACES;i++){
				if(!strcmp(&args[2],race_info[i].title))
					newowner=i;
			}
			break;
	}
	if(newowner!=-1){
		if(args[1]=='1'){
			for(i=1;i<=NumPlayers;i++){     /* in game? maybe long winded */
				if(Players[i]->id==newowner){
					dna->creator=Players[i]->dna;
					dna->owner=newowner;
					dna->owner_type=args[1]-'0';
					dna->a_flags=ACF_NONE;
					return(TRUE);
				}
			}
			return(FALSE);
		}
		dna->owner=newowner;
		dna->owner_type=args[1]-'0';
		return(TRUE);
	}
	return FALSE;
}

bool access_door(int Ind, struct dna_type *dna){
	player_type *p_ptr=Players[Ind];
	if(!(strcmp(p_ptr->name,cfg_dungeon_master)) || !(strcmp(p_ptr->name,cfg_admin_wizard)))
		return(TRUE);
#ifdef NEWHOUSES
	if(dna->a_flags&ACF_LEVEL && p_ptr->lev<dna->min_level && p_ptr->dna!=dna->creator)
		return(FALSE); /* defies logic a bit, but for speed */
#endif
	switch(dna->owner_type){
	int i;
		case OT_PLAYER:
			/* new doors in new server different */
#ifdef NEWHOUSES
			if(p_ptr->id==dna->owner && p_ptr->dna==dna->creator)
				return(TRUE);
			if(dna->a_flags & ACF_PARTY){
				if(!strcmp(parties[p_ptr->party].owner,lookup_player_name(dna->owner)))
					return(TRUE);
			}
			if((dna->a_flags & ACF_CLASS) && (p_ptr->pclass==(dna->creator&0xff)))
				return(TRUE);
			if((dna->a_flags & ACF_RACE) && (p_ptr->prace==((dna->creator>>8)&0xff)))
				return(TRUE);
#else
			if(p_ptr->id==dna->owner) return(TRUE);
#endif
			break;
		case OT_PARTY:
			if(player_in_party(dna->owner, Ind)) return(TRUE);
			break;
		case OT_CLASS:
			if(p_ptr->pclass==dna->owner) return(TRUE);
			break;
		case OT_RACE:
			if(p_ptr->prace==dna->owner) return(TRUE);
			break;
	}
	return(FALSE);
}


/*
 * Open a closed door or closed chest.
 *
 * Note unlocking a locked door/chest is worth one experience point.
 */
void do_cmd_open(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];
#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
#else
	int Depth = p_ptr->dun_depth;
#endif

	int                             y, x, i, j;
	int                             flag;

	cave_type               *c_ptr;
	object_type             *o_ptr;

	bool more = FALSE;
#ifdef NEW_DUNGEON
	if(!(zcave=getcave(wpos))) return;
#endif


	/* Ghosts cannot open doors */
	if ((p_ptr->ghost))
	{
		msg_print(Ind, "You cannot open things!");
		return;
	}

	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		/*command_rep = command_arg - 1;*/

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a "repeated" direction */
	if (dir)
	{
		/* Get requested location */
		y = p_ptr->py + ddy[dir];
		x = p_ptr->px + ddx[dir];

		/* Get requested grid */
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x];
#else
		c_ptr = &cave[Depth][y][x];
#endif

		/* Get the object (if any) */
		o_ptr = &o_list[c_ptr->o_idx];

		/* Nothing useful */
		if (!((c_ptr->feat >= FEAT_DOOR_HEAD) &&
		      (c_ptr->feat <= FEAT_DOOR_TAIL)) &&
		    !((c_ptr->feat >= FEAT_HOME_HEAD) &&
		      (c_ptr->feat <= FEAT_HOME_TAIL)) &&
		    (o_ptr->tval != TV_CHEST))
		{
			/* Message */
			msg_print(Ind, "You see nothing there to open.");
		}

		/* Monster in the way */
		else if (c_ptr->m_idx > 0)
		{
			/* Take a turn */
#ifdef NEW_DUNGEON
			p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
			p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif

			/* Message */
			msg_print(Ind, "There is a monster in the way!");

			/* Attack */
			py_attack(Ind, y, x, TRUE);
		}

		/* Open a closed chest. */
		else if (o_ptr->tval == TV_CHEST)
		{
			/* Take a turn */
#ifdef NEW_DUNGEON
			p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
			p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif

			/* Assume opened successfully */
			flag = TRUE;

			/* Attempt to unlock it */
			if (o_ptr->pval > 0)
			{
				/* Assume locked, and thus not open */
				flag = FALSE;

				/* Get the "disarm" factor */
				i = p_ptr->skill_dis;

				/* Penalize some conditions */
				if (p_ptr->blind || no_lite(Ind)) i = i / 10;
				if (p_ptr->confused || p_ptr->image) i = i / 10;

				/* Extract the difficulty */
				j = i - o_ptr->pval;

				/* Always have a small chance of success */
				if (j < 2) j = 2;

				/* Success -- May still have traps */
				if (rand_int(100) < j)
				{
					msg_print(Ind, "You have picked the lock.");
					gain_exp(Ind, 1);
					flag = TRUE;
				}

				/* Failure -- Keep trying */
				else
				{
					/* We may continue repeating */
					more = TRUE;
					if (flush_failure) flush();
					msg_print(Ind, "You failed to pick the lock.");
				}
			}

			/* Allowed to open */
			if (flag)
			{
				/* Apply chest traps, if any */
				chest_trap(Ind, y, x, o_ptr);

				/* Let the Chest drop items */
				chest_death(Ind, y, x, o_ptr);
			}
		}

		/* Jammed door */
		else if (c_ptr->feat >= FEAT_DOOR_HEAD + 0x08)
		{
			/* Take a turn */
#ifdef NEW_DUNGEON
			p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
			p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif

			/* Stuck */
			msg_print(Ind, "The door appears to be stuck.");
		}

		/* Locked door */
		else if (c_ptr->feat >= FEAT_DOOR_HEAD + 0x01)
		{
			/* Take a turn */
#ifdef NEW_DUNGEON
			p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
			p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif

			/* Disarm factor */
			i = p_ptr->skill_dis;

			/* Penalize some conditions */
			if (p_ptr->blind || no_lite(Ind)) i = i / 10;
			if (p_ptr->confused || p_ptr->image) i = i / 10;

			/* Extract the lock power */
			j = c_ptr->feat - FEAT_DOOR_HEAD;

			/* Extract the difficulty XXX XXX XXX */
			j = i - (j * 4);

			/* Always have a small chance of success */
			if (j < 2) j = 2;

			/* Success */
			if (rand_int(100) < j)
			{
				/* Message */
				msg_print(Ind, "You have picked the lock.");

				/* Experience */
				gain_exp(Ind, 1);

				/* Open the door */
				c_ptr->feat = FEAT_OPEN;

#ifdef NEW_DUNGEON
				/* Notice */
				note_spot_depth(wpos, y, x);

				/* Redraw */
				everyone_lite_spot(wpos, y, x);
#else
				/* Notice */
				note_spot_depth(Depth, y, x);

				/* Redraw */
				everyone_lite_spot(Depth, y, x);
#endif

				/* Update some things */
				p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);
			}

			/* Failure */
			else
			{
				/* Failure */
				if (flush_failure) flush();

				/* Message */
				msg_print(Ind, "You failed to pick the lock.");

				/* We may keep trying */
				more = TRUE;
			}
		}

		/* Home */
		else if (c_ptr->feat >= FEAT_HOME_HEAD && c_ptr->feat <= FEAT_HOME_TAIL)
		{
#ifndef NEWHOUSES
			i = pick_house(Depth, y, x);
			/* evileye hack new houses -demo */
			if(i==-1 && c_ptr->special){ /* orig house failure */
#else
			if(c_ptr->special.type==DNA_DOOR){ /* orig house failure */
#endif /* NEWHOUSES */
				if(access_door(Ind, c_ptr->special.ptr)){
					/* Open the door */
					c_ptr->feat=FEAT_HOME_OPEN;
					/* Take half a turn */
#ifdef NEW_DUNGEON
					p_ptr->energy -= level_speed(&p_ptr->wpos)/2;
					/* Notice */
					note_spot_depth(wpos, y, x);

					/* Redraw */
					everyone_lite_spot(wpos, y, x);
#else
					p_ptr->energy -= level_speed(p_ptr->dun_depth)/2;
					/* Notice */
					note_spot_depth(Depth, y, x);

					/* Redraw */
					everyone_lite_spot(Depth, y, x);
#endif
					
					/* Update some things */
					p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);

				}
				else{
					struct dna_type *dna=c_ptr->special.ptr;
					if(dna->owner){
						char string[80];
						char *name;
						int i;
						strcpy(string,"nobody.");
						switch(dna->owner_type){
							case OT_PLAYER:
								if((name=lookup_player_name(dna->owner)))
									strcpy(string,name);
								break;
							case OT_PARTY:
								strcpy(string, parties[dna->owner].name);
								break;
							case OT_CLASS:
								strcpy(string,class_info[dna->owner].title);
								strcat(string,"s");
								break;
							case OT_RACE:
								strcpy(string,race_info[dna->owner].title);
								strcat(string,"s");
								break;
						}
						msg_format(Ind,"\377sThat house is owned by %s.",string);
					}
					else{
						int factor,price;
						factor = adj_chr_gold[p_ptr->stat_ind[A_CHR]];
						price = dna->price * factor / 100;
						msg_format(Ind,"\377oThat house costs %ld gold.",price);
					}
				}
				return;
			}
			else if(c_ptr->special.type==KEY_DOOR){
				struct key_type *key=c_ptr->special.ptr;
				for(j=0; j<INVEN_PACK; j++){
					object_type *o_ptr=&p_ptr->inventory[j];
					if(o_ptr->tval==TV_KEY && o_ptr->pval==key->id){
						c_ptr->feat=FEAT_HOME_OPEN;
#ifdef NEW_DUNGEON
						p_ptr->energy-=level_speed(&p_ptr->wpos)/2;
						note_spot_depth(wpos, y, x);
						everyone_lite_spot(wpos, y, x);
#else
						p_ptr->energy-=level_speed(p_ptr->dun_depth)/2;
						note_spot_depth(Depth, y, x);
						everyone_lite_spot(Depth, y, x);
#endif
						p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);
						msg_format(Ind, "\377gThe key fits in the lock. %d:%d",key->id, o_ptr->pval);
						return;
					}
				}
				msg_print(Ind,"\377rYou need a key to open this door.");
			}

#ifndef NEWHOUSES
			/* See if he has the key in his inventory */
			for (j = 0; j < INVEN_PACK; j++)
			{
				object_type *o_ptr = &p_ptr->inventory[j];

				/* Check for a key */
				if ( (o_ptr->tval == TV_KEY && o_ptr->pval == i) || (!(strcmp(p_ptr->name,cfg_admin_wizard))) )
				{
					/* Open the door */
					c_ptr->feat = FEAT_HOME_OPEN;

					/* Take half a turn */
#ifdef NEW_DUNGEON
					p_ptr->energy -= level_speed(&p_ptr->wpos)/2;
#else
					p_ptr->energy -= level_speed(p_ptr->dun_depth)/2;
#endif

					/* Notice */
					note_spot_depth(Depth, y, x);

					/* Redraw */
					everyone_lite_spot(Depth, y, x);

					/* XXX -- Paranoia */
					if (!houses[i].owned)
					{
						/* Assume one key */
						if (strcmp(p_ptr->name, cfg_admin_wizard)) houses[i].owned = 1;
					} 

					/* Update some things */
					p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);
					
					/* Done */
					return;
				}
			}

			/* He's not the owner, check if owned */
			if (houses[i].owned)
			{
				msg_format(Ind, "This house is already owned.");
			}
			else
			{
				int price, factor;

				/* Take CHR into account */
				factor = adj_chr_gold[p_ptr->stat_ind[A_CHR]];
				price = houses[i].price * factor / 100;

				/* Tell him the price */
				msg_format(Ind, "This house costs %ld gold.", price);
			}
#endif /* !NEWHOUSES */
		}

		/* Closed door */
		else
		{
			/* Take half a turn */
#ifdef NEW_DUNGEON
			p_ptr->energy -= level_speed(&p_ptr->wpos)/2;
#else
			p_ptr->energy -= level_speed(p_ptr->dun_depth)/2;
#endif

			/* Open the door */
			c_ptr->feat = FEAT_OPEN;

#ifdef NEW_DUNGEON
			/* Notice */
			note_spot_depth(wpos, y, x);

			/* Redraw */
			everyone_lite_spot(wpos, y, x);
#else
			/* Notice */
			note_spot_depth(Depth, y, x);

			/* Redraw */
			everyone_lite_spot(Depth, y, x);
#endif

			/* Update some things */
			p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);
		}
	}

	/* Cancel repeat unless we may continue */
	if (!more) disturb(Ind, 0, 0);
}


/*
 * Close an open door.
 */
void do_cmd_close(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];
#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
#else
	int Depth = p_ptr->dun_depth;
#endif

	int                     y, x, i;
	cave_type               *c_ptr;

	bool more = FALSE;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif


	/* Ghosts cannot close */
	if ( (p_ptr->ghost))
	{
		msg_print(Ind, "You cannot close things!");
		return;
	}

	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		/*command_rep = command_arg - 1;*/

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a "repeated" direction */
	if (dir)
	{
		/* Get requested location */
		y = p_ptr->py + ddy[dir];
		x = p_ptr->px + ddx[dir];

		/* Get grid and contents */
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x];
#else
		c_ptr = &cave[Depth][y][x];
#endif

		/* Broken door */
		if (c_ptr->feat == FEAT_BROKEN)
		{
			/* Message */
			msg_print(Ind, "The door appears to be broken.");
		}

		/* Require open door */
		else if (c_ptr->feat != FEAT_OPEN && c_ptr->feat != FEAT_HOME_OPEN)
		{
			/* Message */
			msg_print(Ind, "You see nothing there to close.");
		}

		/* Monster in the way */
		else if (c_ptr->m_idx > 0)
		{
			/* Take a turn */
#ifdef NEW_DUNGEON
			p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
			p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif

			/* Message */
			msg_print(Ind, "There is a monster in the way!");

			/* Attack */
			py_attack(Ind, y, x, TRUE);
		}

		/* House door, close it */
		else if (c_ptr->feat == FEAT_HOME_OPEN)
		{
			/* Find this house */
#ifdef NEW_DUNGEON
			i = pick_house(wpos, y, x);
#else
			i = pick_house(Depth, y, x);
#endif

			/* Take a turn */
#ifdef NEW_DUNGEON
			p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
			p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif

			/* Close the door */
#ifdef NEWHOUSES
			c_ptr->feat = FEAT_HOME_HEAD;
#else
			c_ptr->feat = FEAT_HOME_HEAD + houses[i].strength;
#endif

#ifdef NEW_DUNGEON
			/* Notice */
			note_spot_depth(wpos, y, x);

			/* Redraw */
			everyone_lite_spot(wpos, y, x);
#else
			/* Notice */
			note_spot_depth(Depth, y, x);

			/* Redraw */
			everyone_lite_spot(Depth, y, x);
#endif

			/* Update some things */
			p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);
		}

		/* Close the door */
		else
		{
			/* Take a turn */
#ifdef NEW_DUNGEON
			p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
			p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif

			/* Close the door */
			c_ptr->feat = FEAT_DOOR_HEAD + 0x00;

#ifdef NEW_DUNGEON
			/* Notice */
			note_spot_depth(wpos, y, x);

			/* Redraw */
			everyone_lite_spot(wpos, y, x);
#else
			/* Notice */
			note_spot_depth(Depth, y, x);

			/* Redraw */
			everyone_lite_spot(Depth, y, x);
#endif

			/* Update some things */
			p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);
		}
	}

	/* Cancel repeat unless we may continue */
	if (!more) disturb(Ind, 0, 0);
}



/*
 * Tunnel through wall.  Assumes valid location.
 *
 * Note that it is impossible to "extend" rooms past their
 * outer walls (which are actually part of the room).
 *
 * This will, however, produce grids which are NOT illuminated
 * (or darkened) along with the rest of the room.
 */
static bool twall(int Ind, int y, int x)
{
	player_type *p_ptr = Players[Ind];
	byte            *w_ptr = &p_ptr->cave_flag[y][x];
#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
	cave_type *c_ptr;
	if(!(zcave=getcave(wpos))) return(FALSE);
	c_ptr=&zcave[y][x];
#else
	int Depth = p_ptr->dun_depth;
	cave_type       *c_ptr = &cave[Depth][y][x];
#endif

	/* Paranoia -- Require a wall or door or some such */
#ifdef NEW_DUNGEON
	if (cave_floor_bold(zcave, y, x)) return (FALSE);
#else
	if (cave_floor_bold(Depth, y, x)) return (FALSE);
#endif

	/* Remove the feature */
	c_ptr->feat = FEAT_FLOOR;

	/* Forget the "field mark" */
	*w_ptr &= ~CAVE_MARK;

#ifdef NEW_DUNGEON
	/* Notice */
	note_spot_depth(wpos, y, x);

	/* Redisplay the grid */
	everyone_lite_spot(wpos, y, x);
#else
	/* Notice */
	note_spot_depth(Depth, y, x);

	/* Redisplay the grid */
	everyone_lite_spot(Depth, y, x);
#endif

	/* Update some things */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS);

	/* Result */
	return (TRUE);
}



/*
 * Tunnels through "walls" (including rubble and closed doors)
 *
 * Note that tunneling almost always takes time, since otherwise
 * you can use tunnelling to find monsters.  Also note that you
 * must tunnel in order to hit invisible monsters in walls (etc).
 *
 * Digging is very difficult without a "digger" weapon, but can be
 * accomplished by strong players using heavy weapons.
 */
void do_cmd_tunnel(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];
#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
#else
	int Depth = p_ptr->dun_depth;
#endif

	int                     y, x;

	cave_type               *c_ptr;

	bool old_floor = FALSE;

	bool more = FALSE;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif


	/* Ghosts have no need to tunnel */
	if ( (p_ptr->ghost))
	{
		/* Message */
		msg_print(Ind, "You cannot tunnel.");

		return;
	}

	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		/*command_rep = command_arg - 1;*/

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a direction to tunnel, or Abort */
	if (dir)
	{
		/* Get location */
		y = p_ptr->py + ddy[dir];
		x = p_ptr->px + ddx[dir];

		/* Get grid */
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x];
		/* Check the floor-hood */
		old_floor = cave_floor_bold(zcave, y, x);

		/* No tunnelling through emptiness */
		if ( (cave_floor_bold(zcave, y, x)) || (c_ptr->feat == FEAT_PERM_CLEAR) )
#else
		c_ptr = &cave[Depth][y][x];
		/* Check the floor-hood */
		old_floor = cave_floor_bold(Depth, y, x);

		/* No tunnelling through emptiness */
		if ( (cave_floor_bold(Depth, y, x)) || (c_ptr->feat == FEAT_PERM_CLEAR) )
#endif

		{
			/* Message */
			msg_print(Ind, "You see nothing there to tunnel through.");
		}

		/* No tunnelling through doors */
		else if (c_ptr->feat < FEAT_SECRET && c_ptr->feat >= FEAT_HOME_HEAD)
		{
			/* Message */
//                      msg_print(Ind, "You cannot tunnel through doors.");

			/* Try opening it instead */
			do_cmd_open(Ind, dir);
			return;
		}

		/* A monster is in the way */
		else if (c_ptr->m_idx > 0)
		{
			/* Take a turn */
#ifdef NEW_DUNGEON
			p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
			p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif

			/* Message */
			msg_print(Ind, "There is a monster in the way!");

			/* Attack */
			py_attack(Ind, y, x, TRUE);
		}

		/* Okay, try digging */
		else
		{
			/* Take a turn */
#ifdef NEW_DUNGEON
			p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
			p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif

			/* Titanium */
			if (c_ptr->feat >= FEAT_PERM_EXTRA)
			{
				msg_print(Ind, "This seems to be permanent rock.");
			}

			/* Granite */
			else if (c_ptr->feat >= FEAT_WALL_EXTRA)
			{
				/* Tunnel */
				if ((p_ptr->skill_dig > 40 + rand_int(800)) && twall(Ind, y, x))        /* 1600 */
				{
					msg_print(Ind, "You have finished the tunnel.");
				}

				/* Keep trying */
				else
				{
					/* We may continue tunelling */
					msg_print(Ind, "You tunnel into the granite wall.");
					more = TRUE;
				}
			}

			/* Quartz / Magma */
			else if (c_ptr->feat >= FEAT_MAGMA)
			{
				bool okay = FALSE;
				bool gold = FALSE;
				bool hard = FALSE;

				/* Found gold */
				if (c_ptr->feat >= FEAT_MAGMA_H) gold = TRUE;

				/* Extract "quartz" flag XXX XXX XXX */
				if ((c_ptr->feat - FEAT_MAGMA) & 0x01) hard = TRUE;

				/* Quartz */
				if (hard)
				{
					okay = (p_ptr->skill_dig > 20 + rand_int(400)); /* 800 */
				}

				/* Magma */
				else
				{
					okay = (p_ptr->skill_dig > 10 + rand_int(250)); /* 400 */
				}

				/* Success */
				if (okay && twall(Ind, y, x))
				{
					/* Found treasure */
					if (gold)
					{
#ifdef NEW_DUNGEON
						/* Place some gold */
						place_gold(wpos, y, x);

						/* Notice it */
						note_spot_depth(wpos, y, x);

						/* Display it */
						everyone_lite_spot(wpos, y, x);
#else
						/* Place some gold */
						place_gold(Depth, y, x);

						/* Notice it */
						note_spot_depth(Depth, y, x);

						/* Display it */
						everyone_lite_spot(Depth, y, x);
#endif

						/* Message */
						msg_print(Ind, "You have found something!");
					}

					/* Found nothing */
					else
					{
						/* Message */
						msg_print(Ind, "You have finished the tunnel.");
					}
				}

				/* Failure (quartz) */
				else if (hard)
				{
					/* Message, continue digging */
					msg_print(Ind, "You tunnel into the quartz vein.");
					more = TRUE;
				}

				/* Failure (magma) */
				else
				{
					/* Message, continue digging */
					msg_print(Ind, "You tunnel into the magma vein.");
					more = TRUE;
				}
			}

			/* Rubble */
			else if (c_ptr->feat == FEAT_RUBBLE)
			{
				/* Remove the rubble */
				if ((p_ptr->skill_dig > rand_int(200)) && twall(Ind, y, x))
				{
					/* Message */
					msg_print(Ind, "You have removed the rubble.");

					/* Hack -- place an object */
					if (rand_int(100) < 10)
					{
#ifdef NEW_DUNGEON
						place_object(wpos, y, x, FALSE, FALSE);
#else
						place_object(Depth, y, x, FALSE, FALSE);
#endif
						if (player_can_see_bold(Ind, y, x))
						{
							msg_print(Ind, "You have found something!");
						}
					}

#ifdef NEW_DUNGEON
					/* Notice */
					note_spot_depth(wpos, y, x);

					/* Display */
					everyone_lite_spot(wpos, y, x);
#else
					/* Notice */
					note_spot_depth(Depth, y, x);

					/* Display */
					everyone_lite_spot(Depth, y, x);
#endif
				}

				else
				{
					/* Message, keep digging */
					msg_print(Ind, "You dig in the rubble.");
					more = TRUE;
				}
			}
			
			else if (c_ptr->feat == FEAT_TREE)
			{
				/* mow down the vegetation */
				if ((p_ptr->skill_dig > rand_int(250)) && twall(Ind, y, x)) /* 400 */
				{
					/* Message */
					msg_print(Ind, "You hack your way through the vegetation.");
					
#ifdef NEW_DUNGEON
					/* Notice */
					note_spot_depth(wpos, y, x);

					/* Display */
					everyone_lite_spot(wpos, y, x);
#else
					/* Notice */
					note_spot_depth(Depth, y, x);

					/* Display */
					everyone_lite_spot(Depth, y, x);
#endif
				}
				else
				{
					/* Message, keep digging */
					msg_print(Ind, "You attempt to clear a path.");
					more = TRUE;
				}
			}
			
			else if (c_ptr->feat == FEAT_EVIL_TREE)
			{
				/* mow down the vegetation */
				if ((p_ptr->skill_dig > rand_int(400)) && twall(Ind, y, x)) /* 600 */
				{
					/* Message */
					msg_print(Ind, "You hack your way through the vegetation.");
					
#ifdef NEW_DUNGEON
					/* Notice */
					note_spot_depth(wpos, y, x);

					/* Display */
					everyone_lite_spot(wpos, y, x);
#else
					/* Notice */
					note_spot_depth(Depth, y, x);

					/* Display */
					everyone_lite_spot(Depth, y, x);
#endif
				}
				else
				{
					/* Message, keep digging */
					msg_print(Ind, "You attempt to clear a path.");
					more = TRUE;
				}
			}

			/* Default to secret doors */
			else /* if (c_ptr->feat == FEAT_SECRET) */
			{
				/* Message, keep digging */
				msg_print(Ind, "You tunnel into the granite wall.");
				more = TRUE;

				/* Hack -- Search */
				search(Ind);
			}
		}

		/* Notice "blockage" changes */
#ifdef NEW_DUNGEON
		if (old_floor != cave_floor_bold(zcave, y, x))
#else
		if (old_floor != cave_floor_bold(Depth, y, x))
#endif
		{
			/* Update some things */
			p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS);
		}
	}

	/* Cancel repetition unless we can continue */
	if (!more) disturb(Ind, 0, 0);
}


/*
 * Disarms a trap, or chest     -RAK-
 */
void do_cmd_disarm(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];
#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
#else
	int Depth = p_ptr->dun_depth;
#endif

	int                 y, x, i, j, power;

	cave_type               *c_ptr;
	byte                    *w_ptr;
	object_type             *o_ptr;

	bool            more = FALSE;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif


	/* Ghosts cannot disarm */
	if ( (p_ptr->ghost))
	{
		/* Message */
		msg_print(Ind, "You cannot disarm things!");

		return;
	}

	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		/*command_rep = command_arg - 1;*/

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a direction (or abort) */
	if (dir)
	{
		/* Get location */
		y = p_ptr->py + ddy[dir];
		x = p_ptr->px + ddx[dir];

		/* Get grid and contents */
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x];
#else
		c_ptr = &cave[Depth][y][x];
#endif
		w_ptr = &p_ptr->cave_flag[y][x];

		/* Access the item */
		o_ptr = &o_list[c_ptr->o_idx];

		/* Nothing useful */
		if (!((c_ptr->feat >= FEAT_TRAP_HEAD) &&
		      (c_ptr->feat <= FEAT_TRAP_TAIL)) &&
		    (o_ptr->tval != TV_CHEST))
		{
			/* Message */
			msg_print(Ind, "You see nothing there to disarm.");
		}

		/* Monster in the way */
		else if (c_ptr->m_idx > 0)
		{
			/* Take a turn */
#ifdef NEW_DUNGEON
			p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
			p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif

			/* Message */
			msg_print(Ind, "There is a monster in the way!");

			/* Attack */
			py_attack(Ind, y, x, TRUE);
		}

		/* Normal disarm */
		else if (o_ptr->tval == TV_CHEST)
		{
			/* Take a turn */
#ifdef NEW_DUNGEON
			p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
			p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif

			/* Get the "disarm" factor */
			i = p_ptr->skill_dis;

			/* Penalize some conditions */
			if (p_ptr->blind || no_lite(Ind)) i = i / 10;
			if (p_ptr->confused || p_ptr->image) i = i / 10;

			/* Extract the difficulty */
			j = i - o_ptr->pval;

			/* Always have a small chance of success */
			if (j < 2) j = 2;

			/* Must find the trap first. */
			if (!object_known_p(Ind, o_ptr))
			{
				msg_print(Ind, "I don't see any traps.");
			}

			/* Already disarmed/unlocked */
			else if (o_ptr->pval <= 0)
			{
				msg_print(Ind, "The chest is not trapped.");
			}

			/* No traps to find. */
			else if (!chest_traps[o_ptr->pval])
			{
				msg_print(Ind, "The chest is not trapped.");
			}

			/* Success (get a lot of experience) */
			else if (rand_int(100) < j)
			{
				msg_print(Ind, "You have disarmed the chest.");
				gain_exp(Ind, o_ptr->pval);
				o_ptr->pval = (0 - o_ptr->pval);
			}

			/* Failure -- Keep trying */
			else if ((i > 5) && (randint(i) > 5))
			{
				/* We may keep trying */
				more = TRUE;
				if (flush_failure) flush();
				msg_print(Ind, "You failed to disarm the chest.");
			}

			/* Failure -- Set off the trap */
			else
			{
				msg_print(Ind, "You set off a trap!");
				chest_trap(Ind, y, x, o_ptr);
			}
		}

		/* Disarm a trap */
		else
		{
			cptr name = (f_name + f_info[c_ptr->feat].name);

			/* Take a turn */
#ifdef NEW_DUNGEON
			p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
			p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif

			/* Get the "disarm" factor */
			i = p_ptr->skill_dis;

			/* Penalize some conditions */
			if (p_ptr->blind || no_lite(Ind)) i = i / 10;
			if (p_ptr->confused || p_ptr->image) i = i / 10;

			/* XXX XXX XXX Variable power? */

			/* Extract trap "power" */
			power = 5;

			/* Extract the difficulty */
			j = i - power;

			/* Always have a small chance of success */
			if (j < 2) j = 2;

			/* Success */
			if (rand_int(100) < j)
			{
				/* Message */
				msg_format(Ind, "You have disarmed the %s.", name);

				/* Reward */
				gain_exp(Ind, power);

				/* Remove the trap */
				c_ptr->feat = FEAT_FLOOR;

#ifdef NEW_DUNGEON
				/* Forget the "field mark" */
				everyone_forget_spot(wpos, y, x);

				/* Notice */
				note_spot_depth(wpos, y, x);

				/* Redisplay the grid */
				everyone_lite_spot(wpos, y, x);
#else
				/* Forget the "field mark" */
				everyone_forget_spot(Depth, y, x);

				/* Notice */
				note_spot_depth(Depth, y, x);

				/* Redisplay the grid */
				everyone_lite_spot(Depth, y, x);
#endif

				/* move the player onto the trap grid */
				move_player(Ind, dir, FALSE);
			}

			/* Failure -- Keep trying */
			else if ((i > 5) && (randint(i) > 5))
			{
				/* Failure */
				if (flush_failure) flush();

				/* Message */
				msg_format(Ind, "You failed to disarm the %s.", name);

				/* We may keep trying */
				more = TRUE;
			}

			/* Failure -- Set off the trap */
			else
			{
				/* Message */
				msg_format(Ind, "You set off the %s!", name);

				/* Move the player onto the trap */
				move_player(Ind, dir, FALSE);
			}
		}
	}

	/* Cancel repeat unless told not to */
	if (!more) disturb(Ind, 0, 0);
}


/*
 * Bash open a door, success based on character strength
 *
 * For a closed door, pval is positive if locked; negative if stuck.
 *
 * For an open door, pval is positive for a broken door.
 *
 * A closed door can be opened - harder if locked. Any door might be
 * bashed open (and thereby broken). Bashing a door is (potentially)
 * faster! You move into the door way. To open a stuck door, it must
 * be bashed. A closed door can be jammed (see do_cmd_spike()).
 *
 * Breatures can also open or bash doors, see elsewhere.
 *
 * We need to use character body weight for something, or else we need
 * to no longer give female characters extra starting gold.
 */
void do_cmd_bash(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];
#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
#else
	int Depth = p_ptr->dun_depth;
#endif

	int                 y, x;

	int                     bash, temp;

	cave_type               *c_ptr;

	bool            more = FALSE;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif


	/* Ghosts cannot bash */
	if ( (p_ptr->ghost) || (p_ptr->fruit_bat) )
	{
		/* Message */
		msg_print(Ind, "You cannot bash things!");

		return;
	}

	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		/*command_rep = command_arg - 1;*/

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a "repeated" direction */
	if (dir)
	{
		/* Bash location */
		y = p_ptr->py + ddy[dir];
		x = p_ptr->px + ddx[dir];

		/* Get grid */
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x];
#else
		c_ptr = &cave[Depth][y][x];
#endif

		/* Nothing useful */
		if (!((c_ptr->feat >= FEAT_DOOR_HEAD) &&
		      (c_ptr->feat <= FEAT_DOOR_TAIL)))
		{
			/* Message */
			msg_print(Ind, "You see nothing there to bash.");
		}

		/* Monster in the way */
		else if (c_ptr->m_idx > 0)
		{
			/* Take a turn */
#ifdef NEW_DUNGEON
			p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
			p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif

			/* Message */
			msg_print(Ind, "There is a monster in the way!");

			/* Attack */
			py_attack(Ind, y, x, TRUE);
		}

		/* Bash a closed door */
		else
		{
			/* Take a turn */
#ifdef NEW_DUNGEON
			p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
			p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif

			/* Message */
			msg_print(Ind, "You smash into the door!");

			/* Hack -- Bash power based on strength */
			/* (Ranges from 3 to 20 to 100 to 200) */
			bash = adj_str_blow[p_ptr->stat_ind[A_STR]];

			/* Extract door power */
			temp = ((c_ptr->feat - FEAT_DOOR_HEAD) & 0x07);

			/* Compare bash power to door power XXX XXX XXX */
			temp = (bash - (temp * 10));

			/* Hack -- always have a chance */
			if (temp < 1) temp = 1;

			/* Hack -- attempt to bash down the door */
			if (rand_int(100) < temp)
			{
				/* Message */
				msg_print(Ind, "The door crashes open!");

				/* Break down the door */
				if (rand_int(100) < 50)
				{
					c_ptr->feat = FEAT_BROKEN;
				}

				/* Open the door */
				else
				{
					c_ptr->feat = FEAT_OPEN;
				}

#ifdef NEW_DUNGEON
				/* Notice */
				note_spot_depth(wpos, y, x);

				/* Redraw */
				everyone_lite_spot(wpos, y, x);
#else
				/* Notice */
				note_spot_depth(Depth, y, x);

				/* Redraw */
				everyone_lite_spot(Depth, y, x);

#endif
				/* Hack -- Fall through the door */
				move_player(Ind, dir, FALSE);

				/* Update some things */
				p_ptr->update |= (PU_VIEW | PU_LITE);
				p_ptr->update |= (PU_DISTANCE);
			}

			/* Saving throw against stun */
			else if (rand_int(100) < adj_dex_safe[p_ptr->stat_ind[A_DEX]] +
				 p_ptr->lev)
			{
				/* Message */
				msg_print(Ind, "The door holds firm.");

				/* Allow repeated bashing */
				more = TRUE;
			}

			/* High dexterity yields coolness */
			else
			{
				/* Message */
				msg_print(Ind, "You are off-balance.");

				/* Hack -- Lose balance ala paralysis */
				(void)set_paralyzed(Ind, p_ptr->paralyzed + 2 + rand_int(2));
			}
		}
	}

	/* Unless valid action taken, cancel bash */
	if (!more) disturb(Ind, 0, 0);
}



/*
 * Find the index of some "spikes", if possible.
 *
 * XXX XXX XXX Let user choose a pile of spikes, perhaps?
 */
static bool get_spike(int Ind, int *ip)
{
	player_type *p_ptr = Players[Ind];

	int i;

	/* Check every item in the pack */
	for (i = 0; i < INVEN_PACK; i++)
	{
		object_type *o_ptr = &(p_ptr->inventory[i]);

		/* Check the "tval" code */
		if (o_ptr->tval == TV_SPIKE)
		{
			/* Save the spike index */
			(*ip) = i;

			/* Success */
			return (TRUE);
		}
	}

	/* Oops */
	return (FALSE);
}



/*
 * Jam a closed door with a spike
 *
 * This command may NOT be repeated
 */
void do_cmd_spike(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];
#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
#else
	int Depth = p_ptr->dun_depth;
#endif

	int                  y, x, item;

	cave_type               *c_ptr;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif


	/* Ghosts cannot spike */
	if ( (p_ptr->ghost))
	{
		/* Message */
		msg_print(Ind, "You cannot spike doors!");

		return;
	}

	/* Get a "repeated" direction */
	if (dir)
	{
		/* Get location */
		y = p_ptr->py + ddy[dir];
		x = p_ptr->px + ddx[dir];

		/* Get grid and contents */
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x];
#else
		c_ptr = &cave[Depth][y][x];
#endif

		/* Require closed door */
		if (!((c_ptr->feat >= FEAT_DOOR_HEAD) &&
		      (c_ptr->feat <= FEAT_DOOR_TAIL)))
		{
			/* Message */
			msg_print(Ind, "You see nothing there to spike.");
		}

		/* Get a spike */
		else if (!get_spike(Ind, &item))
		{
			/* Message */
			msg_print(Ind, "You have no spikes!");
		}

		/* Is a monster in the way? */
		else if (c_ptr->m_idx > 0)
		{
			/* Take a turn */
#ifdef NEW_DUNGEON
			p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
			p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif

			/* Message */
			msg_print(Ind, "There is a monster in the way!");

			/* Attack */
			py_attack(Ind, y, x, TRUE);
		}

		/* Go for it */
		else
		{
			/* Take a turn */
#ifdef NEW_DUNGEON
			p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
			p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif

			/* Successful jamming */
			msg_print(Ind, "You jam the door with a spike.");

			/* Convert "locked" to "stuck" XXX XXX XXX */
			if (c_ptr->feat < FEAT_DOOR_HEAD + 0x08) c_ptr->feat += 0x08;

			/* Add one spike to the door */
			if (c_ptr->feat < FEAT_DOOR_TAIL) c_ptr->feat++;

			/* Use up, and describe, a single spike, from the bottom */
			inven_item_increase(Ind, item, -1);
			inven_item_describe(Ind, item);
			inven_item_optimize(Ind, item);
		}
	}
}



/*
 * Support code for the "Walk" and "Jump" commands
 */
void do_cmd_walk(int Ind, int dir, int pickup)
{
	player_type *p_ptr = Players[Ind];
	cave_type *c_ptr;

	bool more = FALSE;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return;
#endif


	/* Make sure he hasn't just switched levels */
	if (p_ptr->new_level_flag) return;

	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		/*command_rep = command_arg - 1;*/

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a "repeated" direction */
	if (dir)
	{
		/* Hack -- handle confusion */
		if (p_ptr->confused)
		{
			dir = 5;

			/* Prevent walking nowhere */
			while (dir == 5)
				dir = rand_int(9) + 1;
		}

		/* Handle the cfg_door_bump_open option */
		if (cfg_door_bump_open)
		{
			/* Get requested grid */
#ifdef NEW_DUNGEON
			c_ptr = &zcave[p_ptr->py+ddy[dir]][p_ptr->px+ddx[dir]];
#else
			c_ptr = &cave[p_ptr->dun_depth][p_ptr->py+ddy[dir]][p_ptr->px+ddx[dir]];
#endif

			if ((c_ptr->feat >= FEAT_DOOR_HEAD) && 
				(c_ptr->feat <= FEAT_DOOR_TAIL))
			{
				do_cmd_open(Ind, dir);
				return;
			}
			else
			if ((c_ptr->feat >= FEAT_HOME_HEAD) &&
				(c_ptr->feat <= FEAT_HOME_TAIL)) 
			{
#ifndef NEWHOUSES
				i = pick_house(Depth, y, x);
				/* evileye hack new houses -demo */
				if(i==-1 && c_ptr->special){ /* orig house failure */
#else
				if(c_ptr->special.type==DNA_DOOR){ /* orig house failure */
#endif /* NEWHOUSES */
					if(!access_door(Ind, c_ptr->special.ptr))
					{
						do_cmd_open(Ind, dir);
						return;
					}
				}
			}
		}
		/* Actually move the character */
		move_player(Ind, dir, pickup);

		/* Take a turn */
#ifdef NEW_DUNGEON
		p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
		p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif

		/* Allow more walking */
		more = TRUE;
	}

	/* Cancel repeat unless we may continue */
	if (!more) disturb(Ind, 0, 0);
}



/*
 * Start running.
 */
/* Hack -- since this command has different cases of energy requirements and
 * if we don't have enough energy sometimes we want to queue and sometimes we
 * don't, we do all of the energy checking within this function.  If after all
 * is said and done we want to queue the command, we return a 0.  If we don't,
 * we return a 2.
 */
int do_cmd_run(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];
	cave_type *c_ptr;
	int i;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return(FALSE);
#endif

	/* Get a "repeated" direction */
	if (dir)
	{
		/* Make sure we have an empty space to run into */
		if (see_wall(Ind, dir, p_ptr->py, p_ptr->px))
		{
			/* Handle the cfg_door_bump option */
			if (cfg_door_bump_open)
			{
				/* Get requested grid */
#ifdef NEW_DUNGEON
				c_ptr = &zcave[p_ptr->py+ddy[dir]][p_ptr->px+ddx[dir]];
#else
				c_ptr = &cave[p_ptr->dun_depth][p_ptr->py+ddy[dir]][p_ptr->px+ddx[dir]];
#endif

				if (((c_ptr->feat >= FEAT_DOOR_HEAD) && 
				      (c_ptr->feat <= FEAT_DOOR_TAIL)) ||
				    ((c_ptr->feat >= FEAT_HOME_HEAD) &&
				      (c_ptr->feat <= FEAT_HOME_TAIL))) 
				{
					/* Check if we have enough energy to open the door */
#ifdef NEW_DUNGEON
					if (p_ptr->energy >= level_speed(&p_ptr->wpos))
#else
					if (p_ptr->energy >= level_speed(p_ptr->dun_depth))
#endif
					{
						/* If so, open it. */
						do_cmd_open(Ind, dir);
					}
					return 2;
				}
			}

			/* Message */
			msg_print(Ind, "You cannot run in that direction.");

			/* Disturb */
			disturb(Ind, 0, 0);

			return 2;
		}

		/* Make sure we have enough energy to start running */
#ifdef NEW_DUNGEON
		if (p_ptr->energy >= (level_speed(&p_ptr->wpos)*6)/5)
#else
		if (p_ptr->energy >= (level_speed(p_ptr->dun_depth)*6)/5)
#endif
		{
			/* Hack -- Set the run counter */
			p_ptr->running = (command_arg ? command_arg : 1000);

			/* First step */
			run_step(Ind, dir);

			/* Reset the player's energy so he can't sprint several spaces
			 * in the first round of running.  */
#ifdef NEW_DUNGEON
			p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
			p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif
			return 2;
		}
		/* If we don't have enough energy to run and monsters aren't around,
		 * try to queue the run command.
		 */
		else return 0;
	}
	return 2;
}



/*
 * Stay still.  Search.  Enter stores.
 * Pick up treasure if "pickup" is true.
 */
void do_cmd_stay(int Ind, int pickup)
{
	player_type *p_ptr = Players[Ind];
#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
#else
	int Depth = p_ptr->dun_depth;
#endif
	cave_type *c_ptr;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif
	
	if (p_ptr->new_level_flag) return;

#ifdef NEW_DUNGEON
	c_ptr = &zcave[p_ptr->py][p_ptr->px];
#else
	c_ptr = &cave[Depth][p_ptr->py][p_ptr->px];
#endif


	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		/*command_rep = command_arg - 1;*/

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}


/* We don't want any of this */
#if 0
	/* Take a turn */
#ifdef NEW_DUNGEON
	p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
	p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif


	/* Spontaneous Searching */
	if ((p_ptr->skill_fos >= 50) || (0 == rand_int(50 - p_ptr->skill_fos)))
	{
		search(Ind);
	}

	/* Continuous Searching */
	if (p_ptr->searching)
	{
		search(Ind);
	}
#endif


	/* Hack -- enter a store if we are on one */
	if ((c_ptr->feat >= FEAT_SHOP_HEAD) &&
	    (c_ptr->feat <= FEAT_SHOP_TAIL))
	{
		/* Disturb */
		disturb(Ind, 0, 0);

		/* Hack -- enter store */
		command_new = '_';
	}


	/* Try to Pick up anything under us */
	carry(Ind, pickup, 1);
}






/*
 * Resting allows a player to safely restore his hp     -RAK-
 */
#if 0
void do_cmd_rest(void)
{
	/* Prompt for time if needed */
	if (command_arg <= 0)
	{
		cptr p = "Rest (0-9999, '*' for HP/SP, '&' as needed): ";

		char out_val[80];

		/* Default */
		strcpy(out_val, "&");

		/* Ask for duration */
		if (!get_string(p, out_val, 4)) return;

		/* Rest until done */
		if (out_val[0] == '&')
		{
			command_arg = (-2);
		}

		/* Rest a lot */
		else if (out_val[0] == '*')
		{
			command_arg = (-1);
		}

		/* Rest some */
		else
		{
			command_arg = atoi(out_val);
			if (command_arg <= 0) return;
		}
	}


	/* Paranoia */
	if (command_arg > 9999) command_arg = 9999;


	/* Take a turn XXX XXX XXX (?) */
#ifdef NEW_DUNGEON
	energy -= level_speed(&p_ptr->wpos);
#else
	energy -= level_speed(p_ptr->dun_depth);
#endif

	/* Save the rest code */
	resting = command_arg;

	/* Cancel searching */
	p_ptr->searching = FALSE;

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Redraw the state */
	p_ptr->redraw |= (PR_STATE);

	/* Handle stuff */
	handle_stuff();

	/* Refresh */
	Term_fresh();
}
#endif






/*
 * Determines the odds of an object breaking when thrown at a monster
 *
 * Note that artifacts never break, see the "drop_near()" function.
 */
static int breakage_chance(object_type *o_ptr)
{
	/* Examine the item type */
	switch (o_ptr->tval)
	{
		/* Always break */
		case TV_FLASK:
		case TV_POTION:
		case TV_BOTTLE:
		case TV_FOOD:
		case TV_JUNK:
		{
			return (100);
		}

		/* Often break */
		case TV_LITE:
		case TV_SCROLL:
		case TV_ARROW:
		case TV_SKELETON:
		{
			return (50);
		}

		/* Sometimes break */
		case TV_WAND:
		case TV_SHOT:
		case TV_BOLT:
		case TV_SPIKE:
		{
			return (25);
		}
	}

	/* Rarely break */
	return (10);
}


/*
 * Fire an object from the pack or floor.
 *
 * You may only fire items that "match" your missile launcher.
 *
 * You must use slings + pebbles/shots, bows + arrows, xbows + bolts.
 *
 * See "calc_bonuses()" for more calculations and such.
 *
 * Note that "firing" a missile is MUCH better than "throwing" it.
 *
 * Note: "unseen" monsters are very hard to hit.
 *
 * Objects are more likely to break if they "attempt" to hit a monster.
 *
 * Rangers (with Bows) and Anyone (with "Extra Shots") get extra shots.
 *
 * The "extra shot" code works by decreasing the amount of energy
 * required to make each shot, spreading the shots out over time.
 *
 * Note that when firing missiles, the launcher multiplier is applied
 * after all the bonuses are added in, making multipliers very useful.
 *
 * Note that Bows of "Extra Might" get extra range and an extra bonus
 * for the damage multiplier.
 *
 * Note that Bows of "Extra Shots" give an extra shot.
 */
void do_cmd_fire(int Ind, int dir, int item)
{
	player_type *p_ptr = Players[Ind], *q_ptr;
#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
#else
	int Depth = p_ptr->dun_depth;
#endif

	int                     i, j, y, x, ny, nx, ty, tx;
	int                     tdam, tdis, thits, tmul;
	int                     bonus, chance;
	int                     cur_dis, visible;

	object_type         throw_obj;
	object_type             *o_ptr;

	object_type             *j_ptr;

	bool            hit_body = FALSE;

	int                     missile_attr;
	int                     missile_char;

	char            o_name[160];
	bool magic = FALSE;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif


	/* Get the "bow" (if any) */
	j_ptr = &(p_ptr->inventory[INVEN_BOW]);

	/* Require a launcher */
	if (!j_ptr->tval)
	{
		msg_print(Ind, "You have nothing to fire with.");
		return;
	}


	/* Require proper missile */
	item_tester_tval = p_ptr->tval_ammo;

	/* Access the item (if in the pack) */
	if (item >= 0)
	{
		o_ptr = &(p_ptr->inventory[item]);
	}
	else
	{
		o_ptr = &o_list[0 - item];
	}

	if( check_guard_inscription( o_ptr->note, 'f' )) {
		msg_print(Ind, "The item's inscription prevents it");
		return;
	}

    if (!can_use(Ind, o_ptr))
    {
	    msg_print(Ind, "You are not high level enough.");
			    return;
    }


	if (o_ptr->tval != p_ptr->tval_ammo)
	{
		msg_print(Ind, "You cannot fire that!");
		return;
	}

	/* Only fire in direction 5 if we have a target */
	if ((dir == 5) && !target_okay(Ind))
		return;

	/* Use the proper number of shots */
	thits = p_ptr->num_fire;

	/* Take a (partial) turn */
#ifdef NEW_DUNGEON
	p_ptr->energy -= (level_speed(&p_ptr->wpos) / thits);
#else
	p_ptr->energy -= (level_speed(p_ptr->dun_depth) / thits);
#endif

	/* Check if monsters around him/her hinder this */
	if (interfere(Ind, p_ptr->pclass == CLASS_ARCHER ? 12 : 15)) return;

	/* Is this Magic Arrow? */
	magic = ((o_ptr->sval == SV_AMMO_MAGIC) && !cursed_p(o_ptr))?TRUE:FALSE;

	/* Create a "local missile object" */
	throw_obj = *o_ptr;
	throw_obj.number = 1;

	/* Reduce and describe inventory */
	if (!magic)
	  {
	    if (item >= 0)
	      {
		inven_item_increase(Ind, item, -1);
		inven_item_describe(Ind, item);
		inven_item_optimize(Ind, item);
	      }

	    /* Reduce and describe floor item */
	    else
	      {
		floor_item_increase(0 - item, -1);
		floor_item_optimize(0 - item);
	      }
	  }
	else
	  /* Magic Ammo are NOT allowed to be enchanted */
	  {
	    o_ptr->to_h = o_ptr->to_d = o_ptr->name1 = o_ptr->name2 = 0;
	    if (item >= 0)
	      {
		inven_item_describe(Ind, item);
		inven_item_optimize(Ind, item);
	      }

	    /* Reduce and describe floor item */
	    else
	      {
		floor_item_optimize(0 - item);
	      }
	  }

	/* Use the missile object */
	o_ptr = &throw_obj;

	/* Describe the object */
	object_desc(Ind, o_name, o_ptr, FALSE, 3);

	/* Find the color and symbol for the object for throwing */
	missile_attr = object_attr(o_ptr);
	missile_char = object_char(o_ptr);


	/* Use a base distance */
	tdis = 10;

	/* Base damage from thrown object plus launcher bonus */
	tdam = damroll(o_ptr->dd, o_ptr->ds) + o_ptr->to_d + j_ptr->to_d;

	if (p_ptr->bow_brand) tdam += p_ptr->bow_brand_d;

	/* Actually "fire" the object */
	bonus = (p_ptr->to_h + o_ptr->to_h + j_ptr->to_h);
	chance = (p_ptr->skill_thb + (bonus * BTH_PLUS_ADJ));

	/* Assume a base multiplier */
	tmul = 1;

	/* Analyze the launcher */
	switch (j_ptr->sval)
	{
		/* Sling and ammo */
		case SV_SLING:
		{
			tmul = 2;
			break;
		}

		/* Short Bow and Arrow */
		case SV_SHORT_BOW:
		{
			tmul = 2;
			break;
		}

		/* Long Bow and Arrow */
		case SV_LONG_BOW:
		{
			tmul = 3;
			break;
		}

		/* Light Crossbow and Bolt */
		case SV_LIGHT_XBOW:
		{
			tmul = 3;
			break;
		}

		/* Heavy Crossbow and Bolt */
		case SV_HEAVY_XBOW:
		{
			tmul = 4;
			break;
		}
	}

	/* Get extra "power" from "extra might" */
	if (p_ptr->xtra_might) tmul++;

	/* Boost the damage */
	tdam *= tmul;

	/* Base range */
	tdis = 10 + 5 * tmul;


	/* Start at the player */
	y = p_ptr->py;
	x = p_ptr->px;

	/* Predict the "target" location */
	tx = p_ptr->px + 99 * ddx[dir];
	ty = p_ptr->py + 99 * ddy[dir];

	/* Check for "target request" */
	if ((dir == 5) && target_okay(Ind))
	{
		tx = p_ptr->target_col;
		ty = p_ptr->target_row;
	}


	/* Hack -- Handle stuff */
	handle_stuff(Ind);


	/* Travel until stopped */
	for (cur_dis = 0; cur_dis <= tdis; )
	{
		/* Hack -- Stop at the target */
		if ((y == ty) && (x == tx)){
			break;
		}

		/* Calculate the new location (see "project()") */
		ny = y;
		nx = x;
		mmove2(&ny, &nx, p_ptr->py, p_ptr->px, ty, tx);

		/* Stopped by walls/doors */
#ifdef NEW_DUNGEON
		if (!cave_floor_bold(zcave, ny, nx)) break;
#else
		if (!cave_floor_bold(Depth, ny, nx)) break;
#endif

		/* Advance the distance */
		cur_dis++;

		/* Save the new location */
		x = nx;
		y = ny;

		/* Save the old "player pointer" */
		q_ptr = p_ptr;

		/* Display it for each player */
		for (i = 1; i < NumPlayers + 1; i++)
		{
			int dispx, dispy;

			/* Use this player */
			p_ptr = Players[i];

			/* If he's not playing, skip him */
			if (p_ptr->conn == NOT_CONNECTED)
				continue;

			/* If he's not here, skip him */
#ifdef NEW_DUNGEON
			if(!inarea(&p_ptr->wpos, wpos))
#else
			if (p_ptr->dun_depth != Depth)
#endif
				continue;

			/* The player can see the (on screen) missile */
			if (panel_contains(y, x) && player_can_see_bold(i, y, x))
			{
				/* Draw, Hilite, Fresh, Pause, Erase */
				dispy = y - p_ptr->panel_row_prt;
				dispx = x - p_ptr->panel_col_prt;

				/* Remember the projectile */
				p_ptr->scr_info[dispy][dispx].c = missile_char;
				p_ptr->scr_info[dispy][dispx].a = missile_attr;

				/* Tell the client */
				Send_char(i, dispx, dispy, missile_attr, missile_char);

				/* Flush and wait */
				if (cur_dis % tmul) Send_flush(i);

				/* Restore */
				lite_spot(i, y, x);
			}

			/* The player cannot see the missile */
			else
			{
				/* Pause anyway, for consistancy */
				/*Term_xtra(TERM_XTRA_DELAY, msec);*/
			}
		}

		/* Restore the player pointer */
		p_ptr = q_ptr;

		/* Player here, hit him */
#ifdef NEW_DUNGEON
		if (zcave[y][x].m_idx < 0)
#else
		if (cave[Depth][y][x].m_idx < 0)
#endif
		{
#ifdef PLAYER_INTERACTION
#ifdef NEW_DUNGEON
			cave_type *c_ptr = &zcave[y][x];
#else
			cave_type *c_ptr = &cave[Depth][y][x];
#endif

			q_ptr = Players[0 - c_ptr->m_idx];

			/* AD hack -- "pass over" players in same party */
			if ((!player_in_party(p_ptr->party, 0 - c_ptr->m_idx)) || (p_ptr->party == 0)){ 

			/* Check the visibility */
			visible = p_ptr->play_vis[0 - c_ptr->m_idx];

			/* Note the collision */
			hit_body = TRUE;

			/* Did we hit it (penalize range) */
			if (test_hit_fire(chance - cur_dis, q_ptr->ac + q_ptr->to_a, visible))
			{
				char p_name[80];

				/* Get the name */
				strcpy(p_name, q_ptr->name);

				/* Handle unseen player */
				if (!visible)
				{
					/* Invisible player */
					msg_format(Ind, "The %s finds a mark.", o_name);
					msg_format(0 - c_ptr->m_idx, "You are hit by a %s!", o_name);
				}

				/* Handle visible player */
				else
				{
					/* Messages */
					msg_format(Ind, "The %s hits %s.", o_name, p_name);
					msg_format(0 - c_ptr->m_idx, "%^s hits you with a %s.", p_ptr->name, o_name);

					/* Track this player's health */
					health_track(Ind, c_ptr->m_idx);
				}

				/* If this was intentional, make target hostile */
				if (check_hostile(Ind, 0 - c_ptr->m_idx))
				{
					/* Make target hostile if not already */
					if (!check_hostile(0 - c_ptr->m_idx, Ind))
					{
						add_hostility(0 - c_ptr->m_idx, p_ptr->name);
					}
				}

				/* Apply special damage XXX XXX XXX */
				tdam = tot_dam_aux_player(o_ptr, tdam, q_ptr);
				tdam = critical_shot(Ind, o_ptr->weight, o_ptr->to_h, tdam);

				/* No negative damage */
				if (tdam < 0) tdam = 0;

				/* XXX Reduce damage by 1/3 */
				tdam = (tdam + 2) / 3;


				if ((p_ptr->bow_brand && (p_ptr->bow_brand_t == BOW_BRAND_CONF)) && !q_ptr->resist_conf)
				  {
				    (void)set_confused(0 - c_ptr->m_idx, q_ptr->confused + q_ptr->lev);
				  }

				/* Take damage */
				take_hit(0 - c_ptr->m_idx, tdam, p_ptr->name);

				/* Add a nice ball if needed */
				switch (p_ptr->bow_brand_t)
				{
					case BOW_BRAND_BALL_FIRE:
#ifdef NEW_DUNGEON
						project(0 - Ind, 2, &p_ptr->wpos, y, x, 30, GF_FIRE, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#else
						project(0 - Ind, 2, p_ptr->dun_depth, y, x, 30, GF_FIRE, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#endif
						break;
					case BOW_BRAND_BALL_COLD:
#ifdef NEW_DUNGEON
						project(0 - Ind, 2, &p_ptr->wpos, y, x, 35, GF_COLD, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#else
						project(0 - Ind, 2, p_ptr->dun_depth, y, x, 35, GF_COLD, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#endif
						break;
					case BOW_BRAND_BALL_ELEC:
#ifdef NEW_DUNGEON
						project(0 - Ind, 2, &p_ptr->wpos, y, x, 40, GF_ELEC, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#else
						project(0 - Ind, 2, p_ptr->dun_depth, y, x, 40, GF_ELEC, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#endif
						break;
					case BOW_BRAND_BALL_ACID:
#ifdef NEW_DUNGEON
						project(0 - Ind, 2, &p_ptr->wpos, y, x, 45, GF_ACID, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#else
						project(0 - Ind, 2, p_ptr->dun_depth, y, x, 45, GF_ACID, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#endif
						break;
					case BOW_BRAND_BALL_SOUND:
#ifdef NEW_DUNGEON
						project(0 - Ind, 2, &p_ptr->wpos, y, x, 30, GF_SOUND, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#else
						project(0 - Ind, 2, p_ptr->dun_depth, y, x, 30, GF_SOUND, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#endif
						break;
				}

				/* Stop looking */
				if (!p_ptr->bow_brand || (p_ptr->bow_brand_t != BOW_BRAND_SHARP)) break;
			}
			
			} /* end hack */
#else

			/* Stop looking */
			if (!p_ptr->bow_brand || (p_ptr->bow_brand_t != BOW_BRAND_SHARP)) break;
#endif
		}

		/* Monster here, Try to hit it */
#ifdef NEW_DUNGEON
		if (zcave[y][x].m_idx > 0)
#else
		if (cave[Depth][y][x].m_idx > 0)
#endif
		{
#ifdef NEW_DUNGEON
			cave_type *c_ptr = &zcave[y][x];
#else
			cave_type *c_ptr = &cave[Depth][y][x];
#endif

			monster_type *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race *r_ptr = race_inf(m_ptr);

			/* Check the visibility */
			visible = p_ptr->mon_vis[c_ptr->m_idx];

			/* Note the collision */
			hit_body = TRUE;

			/* Did we hit it (penalize range) */
			if (test_hit_fire(chance - cur_dis, m_ptr->ac, visible))
			{
				bool fear = FALSE;

				/* Assume a default death */
				cptr note_dies = " dies.";

				/* Some monsters get "destroyed" */
				if ((r_ptr->flags3 & RF3_DEMON) ||
				    (r_ptr->flags3 & RF3_UNDEAD) ||
				    (r_ptr->flags2 & RF2_STUPID) ||
				    (strchr("Evg", r_ptr->d_char)))
				{
					/* Special note at death */
					note_dies = " is destroyed.";
				}


				/* Handle unseen monster */
				if (!visible)
				{
					/* Invisible monster */
					msg_format(Ind, "The %s finds a mark.", o_name);
				}

				/* Handle visible monster */
				else
				{
					char m_name[80];

					/* Get "the monster" or "it" */
					monster_desc(Ind, m_name, c_ptr->m_idx, 0);

					/* Message */
					msg_format(Ind, "The %s hits %s.", o_name, m_name);

					/* Hack -- Track this monster race */
					if (visible) recent_track(m_ptr->r_idx);

					/* Hack -- Track this monster */
					if (visible) health_track(Ind, c_ptr->m_idx);
				}

				/* Apply special damage XXX XXX XXX */
				tdam = tot_dam_aux(Ind, o_ptr, tdam, m_ptr);
				tdam = critical_shot(Ind, o_ptr->weight, o_ptr->to_h, tdam);

				/* No negative damage */
				if (tdam < 0) tdam = 0;

				/* Complex message */
				if (wizard)
				{
					msg_format(Ind, "You do %d (out of %d) damage.",
						   tdam, m_ptr->hp);
				}

				if ((p_ptr->bow_brand && (p_ptr->bow_brand_t == BOW_BRAND_CONF)) &&
				    !(r_ptr->flags3 & RF3_NO_CONF) &&
				    !(r_ptr->flags4 & RF4_BR_CONF) &&
				    !(r_ptr->flags4 & RF4_BR_CHAO))
				  {
				    int i;

				    /* Already partially confused */
				    if (m_ptr->confused)
				      {
					i = m_ptr->confused + p_ptr->lev;
				      }

				    /* Was not confused */
				    else
				      {
					i = p_ptr->lev;
				      }

				    /* Apply confusion */
				    m_ptr->confused = (i < 200) ? i : 200;

				    if (visible)
				      {
					char m_name[80];

					/* Get "the monster" or "it" */
					monster_desc(Ind, m_name, c_ptr->m_idx, 0);

					/* Message */
					msg_format(Ind, "%s appears confused.", m_name);
				      }
				  }


				/* Hit the monster, check for death */
				if (mon_take_hit(Ind, c_ptr->m_idx, tdam, &fear, note_dies))
				{
					/* Dead monster */
				}

				/* No death */
				else
				{
					/* Message */
					message_pain(Ind, c_ptr->m_idx, tdam);

					/* Take note */
					if (fear && visible)
					{
						char m_name[80];

						/* Sound */
						sound(Ind, SOUND_FLEE);

						/* Get the monster name (or "it") */
						monster_desc(Ind, m_name, c_ptr->m_idx, 0);

						/* Message */
						msg_format(Ind, "%^s flees in terror!", m_name);
					}
				}

				/* Add a nice ball if needed */
				switch (p_ptr->bow_brand_t)
				{
					case BOW_BRAND_BALL_FIRE:
#ifdef NEW_DUNGEON
						project(0 - Ind, 2, &p_ptr->wpos, y, x, 30, GF_FIRE, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#else
						project(0 - Ind, 2, p_ptr->dun_depth, y, x, 30, GF_FIRE, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#endif
						break;
					case BOW_BRAND_BALL_COLD:
#ifdef NEW_DUNGEON
						project(0 - Ind, 2, &p_ptr->wpos, y, x, 35, GF_COLD, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#else
						project(0 - Ind, 2, p_ptr->dun_depth, y, x, 35, GF_COLD, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#endif
						break;
					case BOW_BRAND_BALL_ELEC:
#ifdef NEW_DUNGEON
						project(0 - Ind, 2, &p_ptr->wpos, y, x, 40, GF_ELEC, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#else
						project(0 - Ind, 2, p_ptr->dun_depth, y, x, 40, GF_ELEC, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#endif
						break;
					case BOW_BRAND_BALL_ACID:
#ifdef NEW_DUNGEON
						project(0 - Ind, 2, &p_ptr->wpos, y, x, 45, GF_ACID, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#else
						project(0 - Ind, 2, p_ptr->dun_depth, y, x, 45, GF_ACID, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#endif
						break;
					case BOW_BRAND_BALL_SOUND:
#ifdef NEW_DUNGEON
						project(0 - Ind, 2, &p_ptr->wpos, y, x, 30, GF_SOUND, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#else
						project(0 - Ind, 2, p_ptr->dun_depth, y, x, 30, GF_SOUND, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
#endif
						break;
				}

				/* Stop looking */
				if (!p_ptr->bow_brand || (p_ptr->bow_brand_t != BOW_BRAND_SHARP)) break;
			}
		}
	}

	/* Chance of breakage (during attacks) */
	j = (hit_body ? breakage_chance(o_ptr) : 0);

	/* Drop (or break) near that location */
#ifdef NEW_DUNGEON
	if (!magic) drop_near(o_ptr, j, wpos, y, x);
#else
	if (!magic) drop_near(o_ptr, j, Depth, y, x);
#endif
}

/*
 * Check if neighboring monster(s) interferes player's action.  - Jir -
 */
bool interfere(int Ind, int chance)
{
	player_type *p_ptr = Players[Ind];
	monster_race *r_ptr;
	int d, i, tx, ty, x = p_ptr->px, y = p_ptr->py;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return(FALSE);
#endif

	/* Check if monsters around him/her hinder the action */
	for (d = 1; d <= 9; d++)
	{
		if (d == 5) continue;

		tx = x + ddx[d];
		ty = y + ddy[d];

#ifdef NEW_DUNGEON
		if (!in_bounds(ty, tx)) continue;
#else
		if (!in_bounds(p_ptr->dun_depth, ty, tx)) continue;
#endif

#ifdef NEW_DUNGEON
		if (!(i = zcave[ty][tx].m_idx)) continue;
#else
		if (!(i = cave[p_ptr->dun_depth][ty][tx].m_idx)) continue;
#endif
		if (i > 0)
		{
			r_ptr = race_inf(&m_list[i]);
//			if (r_info[m_list[i].r_idx].flags1 & RF1_NEVER_MOVE)
			if (r_ptr->flags1 & RF1_NEVER_MOVE)
				continue;
		}
		else
		{
			/* hostile player? */
			if (!check_hostile(Ind, -i) ||
				Players[-i]->paralyzed ||
				r_info[Players[-i]->body_monster].flags1 & RF1_NEVER_MOVE)
				continue;
		}

		if (rand_int(100) < chance)
		{
			char m_name[80];
			if (i > 0)
			{
				monster_desc(Ind, m_name, i, 0);
			}
			else
			{
				/* even not visible... :( */
				strcpy(m_name, Players[-i]->name);
			}
			msg_format(Ind, "\377o%^s interferes your attempt!", m_name);
			return TRUE;
		}
	}
#if 0
	for (tx = x - 1; tx <= x + 1; tx++)
	{
		for (ty = y - 1; ty <= y + 1; ty++)
		{
			if (!(i = cave[p_ptr->dun_depth][ty][tx].m_idx)) continue;
			if (i > 0)
			{
				if (r_info[m_list[i].r_idx].flags1 & RF1_NEVER_MOVE)
					continue;
			}
			else
			{
				/* hostile player? */
				if (!check_hostile(Ind, -i) ||
					Players[-i]->paralyzed ||
					r_info[Players[-i]->body_monster].flags1 & RF1_NEVER_MOVE)
					continue;
			}

			if (rand_int(100) < chance)
			{
				char m_name[80];
				if (i > 0)
				{
					monster_desc(Ind, m_name, i, 0);
				}
				else
				{
					/* even not visible... :( */
					strcpy(m_name, Players[-i]->name);
				}
				msg_format(Ind, "\377o%^s interferes your attempt!", m_name);
				return TRUE;
			}
		}
	}
#endif

	return FALSE;
}


/*
 * Throw an object from the pack or floor.
 *
 * Note: "unseen" monsters are very hard to hit.
 *
 * Should throwing a weapon do full damage?  Should it allow the magic
 * to hit bonus of the weapon to have an effect?  Should it ever cause
 * the item to be destroyed?  Should it do any damage at all?
 */
void do_cmd_throw(int Ind, int dir, int item)
{
	player_type *p_ptr = Players[Ind], *q_ptr;
#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
#else
	int Depth = p_ptr->dun_depth;
#endif

	int                     i, j, y, x, ny, nx, ty, tx;
	int                     chance, tdam, tdis;
	int                     mul, div;
	int                     cur_dis, visible;

	object_type         throw_obj;
	object_type             *o_ptr;

	bool            hit_body = FALSE;

	int                     missile_attr;
	int                     missile_char;

	char            o_name[160];

	/*int                   msec = delay_factor * delay_factor * delay_factor;*/
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif

	/* Handle the newbies_cannot_drop option */
	if ((p_ptr->lev < 5) && (cfg_newbies_cannot_drop))
	{
		msg_format(Ind, "Please don't litter the %s.",
#ifdef NEW_DUNGEON
			istown(wpos) ? "town":(wpos->wz ? "dungeon":"Nature"));
#else
			Depth != 0 ? (Depth > 0 ? "dungeon" : "Nature") : "town");
#endif
		return;
	}

	/* Access the item (if in the pack) */
	if (item >= 0)
	{
		o_ptr = &(p_ptr->inventory[item]);
	}
	else
	{
		o_ptr = &o_list[0 - item];
	}

	if(o_ptr->tval==0){
		char temp[160];
		msg_print(Ind, "\377rYou cannot throw that");
		return;
	}
	if( check_guard_inscription( o_ptr->note, 'v' )) {
		msg_print(Ind, "The item's inscription prevents it");
		return;
	};

	/* Create a "local missile object" */
	throw_obj = *o_ptr;
	throw_obj.number = 1;

	/* Reduce and describe inventory */
	if (item >= 0)
	{
		inven_item_increase(Ind, item, -1);
		inven_item_describe(Ind, item);
		inven_item_optimize(Ind, item);
	}

	/* Reduce and describe floor item */
	else
	{
		floor_item_increase(0 - item, -1);
		floor_item_optimize(0 - item);
	}

	/* Use the local object */
	o_ptr = &throw_obj;

	/* Description */
	object_desc(Ind, o_name, o_ptr, FALSE, 3);

	/* Find the color and symbol for the object for throwing */
	missile_attr = object_attr(o_ptr);
	missile_char = object_char(o_ptr);


	/* Extract a "distance multiplier" */
	mul = 10;

	/* Enforce a minimum "weight" of one pound */
	div = ((o_ptr->weight > 10) ? o_ptr->weight : 10);

	/* Hack -- Distance -- Reward strength, penalize weight */
	tdis = (adj_str_blow[p_ptr->stat_ind[A_STR]] + 20) * mul / div;

	/* Max distance of 10 */
	if (tdis > 10) tdis = 10;

	/* Hack -- Base damage from thrown object */
	tdam = damroll(o_ptr->dd, o_ptr->ds) + o_ptr->to_d;

	/* Chance of hitting */
	chance = (p_ptr->skill_tht + (p_ptr->to_h * BTH_PLUS_ADJ));


	/* Take a turn */
#ifdef NEW_DUNGEON
	p_ptr->energy -= level_speed(&p_ptr->wpos);
#else
	p_ptr->energy -= level_speed(p_ptr->dun_depth);
#endif


	/* Start at the player */
	y = p_ptr->py;
	x = p_ptr->px;

	/* Predict the "target" location */
	tx = p_ptr->px + 99 * ddx[dir];
	ty = p_ptr->py + 99 * ddy[dir];

	/* Check for "target request" */
	if ((dir == 5) && target_okay(Ind))
	{
		tx = p_ptr->target_col;
		ty = p_ptr->target_row;
	}


	/* Hack -- Handle stuff */
	handle_stuff(Ind);


	/* Travel until stopped */
	for (cur_dis = 0; cur_dis <= tdis; )
	{
		/* Hack -- Stop at the target */
		if ((y == ty) && (x == tx)) break;

		/* Calculate the new location (see "project()") */
		ny = y;
		nx = x;
		mmove2(&ny, &nx, p_ptr->py, p_ptr->px, ty, tx);

		/* Stopped by walls/doors */
#ifdef NEW_DUNGEON
		if (!cave_floor_bold(zcave, ny, nx)) break;
#else
		if (!cave_floor_bold(Depth, ny, nx)) break;
#endif

		/* Advance the distance */
		cur_dis++;

		/* Save the new location */
		x = nx;
		y = ny;


		/* Save the old "player pointer" */
		q_ptr = p_ptr;

		/* Display it for each player */
		for (i = 1; i < NumPlayers + 1; i++)
		{
			int dispx, dispy;

			/* Use this player */
			p_ptr = Players[i];

			/* If he's not playing, skip him */
			if (p_ptr->conn == NOT_CONNECTED)
				continue;

			/* If he's not here, skip him */
#ifdef NEW_DUNGEON
			if(!inarea(wpos, &p_ptr->wpos))
#else
			if (p_ptr->dun_depth != Depth)
#endif
				continue;

			/* The player can see the (on screen) missile */
			if (panel_contains(y, x) && player_can_see_bold(i, y, x))
			{
				/* Draw, Hilite, Fresh, Pause, Erase */
				dispy = y - p_ptr->panel_row_prt;
				dispx = x - p_ptr->panel_col_prt;

				/* Remember the projectile */
				p_ptr->scr_info[dispy][dispx].c = missile_char;
				p_ptr->scr_info[dispy][dispx].a = missile_attr;

				/* Tell the client */
				Send_char(i, dispx, dispy, missile_attr, missile_char);

				/* Flush and wait */
				if (cur_dis % 2) Send_flush(i);

				/* Restore */
				lite_spot(i, y, x);
			}

			/* The player cannot see the missile */
			else
			{
				/* Pause anyway, for consistancy */
				/*Term_xtra(TERM_XTRA_DELAY, msec);*/
			}
		}

		/* Restore the player pointer */
		p_ptr = q_ptr;


		/* Player here, try to hit him */
#ifdef NEW_DUNGEON
		if (zcave[y][x].m_idx < 0)
#else
		if (cave[Depth][y][x].m_idx < 0)
#endif
		{
#ifdef PLAYER_INTERACTION
#ifdef NEW_DUNGEON
			cave_type *c_ptr = &zcave[y][x];
#else
			cave_type *c_ptr = &cave[Depth][y][x];
#endif

			q_ptr = Players[0 - c_ptr->m_idx];

			/* Check the visibility */
			visible = p_ptr->play_vis[0 - c_ptr->m_idx];

			/* Note the collision */
			hit_body = TRUE;

			/* Did we hit him (penalize range) */
			if (test_hit_fire(chance - cur_dis, q_ptr->ac + q_ptr->to_a, visible))
			{
				char p_name[80];

				/* Get his name */
				strcpy(p_name, q_ptr->name);

				/* Handle unseen player */
				if (!visible)
				{
					/* Messages */
					msg_format(Ind, "The %s finds a mark!", o_name);
					msg_format(0 - c_ptr->m_idx, "You are hit by a %s!", o_name);
				}
				
				/* Handle visible player */
				else
				{
					/* Messages */
					msg_format(Ind, "The %s hits %s.", o_name, p_name);
					msg_format(0 - c_ptr->m_idx, "%s hits you with a %s!", p_ptr->name, o_name);

					/* Track player's health */
					health_track(Ind, c_ptr->m_idx);
				}

				/* Apply special damage XXX XXX XXX */
				tdam = tot_dam_aux_player(o_ptr, tdam, q_ptr);
				tdam = critical_shot(Ind, o_ptr->weight, o_ptr->to_h, tdam);

				/* No negative damage */
				if (tdam < 0) tdam = 0;

				/* XXX Reduce damage by 1/3 */
				tdam = (tdam + 2) / 3;

				/* Take damage */
				take_hit(0 - c_ptr->m_idx, tdam, p_ptr->name);

				/* Stop looking */
				break;
			}
#else

			/* Stop looking */
			break;
#endif
		}

		/* Monster here, Try to hit it */
#ifdef NEW_DUNGEON
		if (zcave[y][x].m_idx > 0)
		{
			cave_type *c_ptr = &zcave[y][x];
#else
		if (cave[Depth][y][x].m_idx > 0)
		{
			cave_type *c_ptr = &cave[Depth][y][x];
#endif

			monster_type *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race *r_ptr = race_inf(m_ptr);

			/* Check the visibility */
			visible = p_ptr->mon_vis[c_ptr->m_idx];

			/* Note the collision */
			hit_body = TRUE;

			/* Did we hit it (penalize range) */
			if (test_hit_fire(chance - cur_dis, m_ptr->ac, visible))
			{
				bool fear = FALSE;

				/* Assume a default death */
				cptr note_dies = " dies.";

				/* Some monsters get "destroyed" */
				if ((r_ptr->flags3 & RF3_DEMON) ||
				    (r_ptr->flags3 & RF3_UNDEAD) ||
				    (r_ptr->flags2 & RF2_STUPID) ||
				    (strchr("Evg", r_ptr->d_char)))
				{
					/* Special note at death */
					note_dies = " is destroyed.";
				}


				/* Handle unseen monster */
				if (!visible)
				{
					/* Invisible monster */
					msg_format(Ind, "The %s finds a mark.", o_name);
				}

				/* Handle visible monster */
				else
				{
					char m_name[80];

					/* Get "the monster" or "it" */
					monster_desc(Ind, m_name, c_ptr->m_idx, 0);

					/* Message */
					msg_format(Ind, "The %s hits %s.", o_name, m_name);

					/* Hack -- Track this monster race */
					if (visible) recent_track(m_ptr->r_idx);

					/* Hack -- Track this monster */
					if (visible) health_track(Ind, c_ptr->m_idx);
				}

				/* Apply special damage XXX XXX XXX */
				tdam = tot_dam_aux(Ind, o_ptr, tdam, m_ptr);
				tdam = critical_shot(Ind, o_ptr->weight, o_ptr->to_h, tdam);

				/* No negative damage */
				if (tdam < 0) tdam = 0;

				/* Complex message */
				if (wizard)
				{
					msg_format(Ind, "You do %d (out of %d) damage.",
						   tdam, m_ptr->hp);
				}

				/* Hit the monster, check for death */
				if (mon_take_hit(Ind, c_ptr->m_idx, tdam, &fear, note_dies))
				{
					/* Dead monster */
				}

				/* No death */
				else
				{
					/* Message */
					message_pain(Ind, c_ptr->m_idx, tdam);

					/* Take note */
					if (fear && visible)
					{
						char m_name[80];

						/* Sound */
						sound(Ind, SOUND_FLEE);

						/* Get the monster name (or "it") */
						monster_desc(Ind, m_name, c_ptr->m_idx, 0);

						/* Message */
						msg_format(Ind, "%^s flees in terror!", m_name);
					}
				}

				/* Stop looking */
				break;
			}
		}
	}

	/* Chance of breakage (during attacks) */
	j = (hit_body ? breakage_chance(o_ptr) : 0);

	/* Drop (or break) near that location */
#ifdef NEW_DUNGEON
	drop_near_severe(Ind, o_ptr, j, wpos, y, x);
#else
	drop_near_severe(Ind, o_ptr, j, Depth, y, x);
#endif
}

void destroy_house(int Ind, struct dna_type *dna){
	player_type *p_ptr=Players[Ind];
	int i;
	if(strcmp(p_ptr->name, cfg_admin_wizard)){
		msg_print(Ind,"\377rYour attempts to destroy the house fail.");
		return;
	}
	for(i=0;i<num_houses;i++){
		if(houses[i].dna==dna){
			if(houses[i].flags&HF_STOCK){
				msg_print(Ind,"\377oThat house may not be destroyed");
				return;
			}
			/* quicker than copying back an array. */
			msg_print(Ind,"\377DThe house crumbles away.");
			fill_house(&houses[i], 2, NULL);
			houses[i].flags|=HF_DELETED;
			break;
		}
	}
}

void house_admin(int Ind, int dir, char *args){
	player_type *p_ptr=Players[Ind];
#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
#else
	int Depth=p_ptr->dun_depth;
#endif
	int x,y;
	int success=0;
	cave_type *c_ptr;
	struct dna_type *dna;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);
#endif

	if(dir && args){
		/* Get requested direction */
		y = p_ptr->py + ddy[dir];
		x = p_ptr->px + ddx[dir];
		/* Get requested grid */
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x];
#else
		c_ptr = &cave[Depth][y][x];
#endif
		if(c_ptr->feat>=FEAT_HOME_HEAD && c_ptr->feat<=FEAT_HOME_TAIL)
		{
			if(c_ptr->special.type==DNA_DOOR){
				dna=c_ptr->special.ptr;
				if(access_door(Ind, dna)){
					switch(args[0]){
						case 'O':
							success=chown_door(Ind, dna, args);
							break;
						case 'M':
							success=chmod_door(Ind, dna, args);
							break;
						case 'K':
							destroy_house(Ind, dna);
							return;
					}
					if(success){
						msg_format(Ind,"\377gDoor change successful");
					}
				}
				else msg_format(Ind,"\377oDoor change failed");
			}
			else msg_print(Ind,"\377rYou cant modify that door");
		}
		else msg_print(Ind,"\377sThere is no door there");
	}
	return;
}

/*
 * Buy a house.  It is assumed that the player already knows the
 * price.
 
 Hacked to sell houses for half price. -APD-
 
 */
void do_cmd_purchase_house(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];
#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
#else
	int Depth = p_ptr->dun_depth;
#endif

	int y, x, i, j;
	int factor;
	int64_t price; /* I'm hoping this will be 64 bits.  I dont know if it will be portable. */
	cave_type *c_ptr;
#ifndef NEWHOUSES
	object_type key;
#else
	struct dna_type *dna;
#endif
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif

	/* Ghosts cannot buy houses */
	if ( (p_ptr->ghost))
	{
		/* Message */
		msg_print(Ind, "You cannot buy a house.");

		return;
	}       

	/* Be sure we have a direction */
	if (dir)
	{
		/* Get requested direction */
		y = p_ptr->py + ddy[dir];
		x = p_ptr->px + ddx[dir];

		/* Get requested grid */
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x];
#else
		c_ptr = &cave[Depth][y][x];
#endif

#ifdef NEWHOUSES
		if(!(c_ptr->feat>=FEAT_HOME_HEAD && c_ptr->feat<=FEAT_HOME_TAIL && c_ptr->special.type==DNA_DOOR))
#else
		/* Check for a house */
		if ((i = pick_house(Depth, y, x)) == -1)
#endif
		{
			/* No house, message */
			msg_print(Ind, "You see nothing to buy there.");
			return;
		}

#ifdef NEWHOUSES
		dna=c_ptr->special.ptr;
#endif
		/* Take player's CHR into account */
		factor = adj_chr_gold[p_ptr->stat_ind[A_CHR]];
#if 0
		if (houses[i].price < 3000000)
#endif
#ifdef NEWHOUSES
		price = dna->price * factor / 100;
#else
			price = houses[i].price * factor / 100;
#endif
		/* Hack -- ignore CHR to prevent overflow */
#if 0
		else price = houses[i].price;
#endif

		/* Check for already-owned house */
#ifdef NEWHOUSES
		if(dna->owner){
			if(access_door(Ind,dna)){
				if(p_ptr->dna==dna->creator){
					/* sell house */
					p_ptr->au+=price/2;
					p_ptr->redraw|=PR_GOLD;
					msg_format(Ind, "You sell your house for %ld gold.", price/2);
				}
				else{
					if(strcmp(p_ptr->name,cfg_admin_wizard)){
						msg_print(Ind,"You cannot sell that house");
						return;
					}
					else
						msg_print(Ind,"The house is reset");
				}
				dna->creator=0L;
				dna->owner=0L;
				dna->a_flags=ACF_NONE;
				return;
			}
			msg_print(Ind,"That house does not belong to you!");
			return;
		}
		if(price>p_ptr->au){
			msg_print(Ind,"You do not have enough gold");
			return;
		}
		msg_format(Ind, "You buy the house for %ld gold.", price);
		p_ptr->au-=price;
		dna->creator=p_ptr->dna;
		dna->owner=p_ptr->id;
		dna->owner_type=OT_PLAYER;
		dna->a_flags=ACF_NONE;
		dna->min_level=1;
#else
		if (houses[i].owned)
		{
			/*
			
			NO MORE DUPLICATE KEYS HAHAHHA
			
			OK, so now this sells the house
			*/
			
			 /* See if he has the key in his inventory */
			for (j = 0; j < INVEN_PACK; j++)
			{
				object_type *o_ptr = &p_ptr->inventory[j];

				/* Check for a key */
				if (o_ptr->tval == TV_KEY && o_ptr->pval == i)
				{

					if( check_guard_inscription( o_ptr->note, 'h' )) {
						msg_print(Ind, "The item's inscription prevents it");
						return;
					};
					
					/* Take away a key (do) */
					inven_item_increase(Ind,j,-1);
					inven_item_describe(Ind,j);
					inven_item_optimize(Ind,j);

					/* house is no longer owned */
					houses[i].owned = 0;
					
					msg_format(Ind, "You sell your house for %ld gold.", price/2);

					 /* Get the money */
					p_ptr->au += price / 2;

					/* Window */
					p_ptr->window |= (PW_INVEN);

					/* Redraw */
					p_ptr->redraw |= (PR_GOLD);

					/* Done */
					return;
				}
				
			}
		
			if (!strcmp(p_ptr->name,cfg_admin_wizard))
			{
				houses[i].owned = 0;
				
				msg_format(Ind, "The house has been reset.");
				
				return;
			}
		
			/* Message */
			msg_print(Ind, "That house is already owned.");
			
			/* No sale */
			return;
		}

		/* Check for enough funds */
		if (price > p_ptr->au)
		{
			/* Not enough money, message */
			msg_print(Ind, "You do not have enough money.");
			return;
		}

		/* Open the door */
		c_ptr->feat = FEAT_HOME_OPEN;

		/* Reshow */
		everyone_lite_spot(Depth, y, x);

		/* Make a key */
		invcopy(&key, lookup_kind(TV_KEY, 1));

		/* Set the pval to the house that this key opens */
		key.pval = i;

		/* Drop it */
		drop_near(&key, -1, Depth, y, x);
		
		/* Take some of the player's money */
		p_ptr->au -= price;
			
		/* The house is now owned */
		houses[i].owned = 1;
#endif /* !NEWHOUSES */

		/* Redraw */
		p_ptr->redraw |= (PR_GOLD);
	}
}

/* King own a territory */
void do_cmd_own(int Ind)
{
	player_type *p_ptr = Players[Ind];
	char buf[100];
	
	if (!p_ptr->total_winner)
	{
		msg_format(Ind, "You must be a %s to own a land!", (p_ptr->male)?"king":"queen");
		return;
	}

#ifdef NEW_DUNGEON
	if (!p_ptr->own1.wx && !p_ptr->own2.wx && !p_ptr->own1.wy && !p_ptr->own2.wy && !p_ptr->own1.wz && !p_ptr->own2.wz)
#else
	if (p_ptr->own1 && p_ptr->own2)
#endif
	{
		msg_format(Ind, "You can't own more than 2 terrains.");
		return;
	}
	
#ifdef NEW_DUNGEON
	if (wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].own)
#else
	if (wild_info[p_ptr->dun_depth].own)
#endif
	{
		msg_format(Ind, "Sorry this land is owned by someone else.");
		return;
	}
#ifdef NEW_DUNGEON
	if(p_ptr->wpos.wz)
#else
	if (p_ptr->dun_depth > 0)
#endif
	{
		msg_format(Ind, "Sorry you can't own the dungeon");
		return;
	}
	
#ifdef NEW_DUNGEON
	if(istown(&p_ptr->wpos))
#else
	if (p_ptr->dun_depth > -13)
#endif
	{
#ifdef NEW_DUNGEON
		msg_format(Ind, "Sorry this land is owned by the town.");
#else
		msg_format(Ind, "Sorry this land(%d) is owned by the town.", p_ptr->dun_depth);
#endif
		return;
	}
	
	/* Ok all check did lets appropriate */
#ifdef NEW_DUNGEON
	if(p_ptr->own1.wx || p_ptr->own1.wy || p_ptr->own1.wz)
#else
	if (p_ptr->own1)
#endif
	{
#ifdef NEW_DUNGEON
		wpcopy(&p_ptr->own2, &p_ptr->wpos);
#else
		p_ptr->own2 = p_ptr->dun_depth;
#endif
	}
	else
	{
#ifdef NEW_DUNGEON
		wpcopy(&p_ptr->own1, &p_ptr->wpos);
#else
		p_ptr->own1 = p_ptr->dun_depth;
#endif
	}
#ifdef NEW_DUNGEON
	wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].own = p_ptr->id;
#else
	wild_info[p_ptr->dun_depth].own = p_ptr->id;
#endif
	
	sprintf(buf, "%s %s now owns (%d,%d).", (p_ptr->male)?"King":"Queen", p_ptr->name, p_ptr->wpos.wx, p_ptr->wpos.wy);
	msg_broadcast(Ind, buf);
	msg_print(Ind, buf);
}
