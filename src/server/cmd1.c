/* File: cmd1.c */

/* Purpose: Movement commands (part 1) */

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
 * Determine if the player "hits" a monster (normal combat).
 * Note -- Always miss 5%, always hit 5%, otherwise random.
 */
bool test_hit_fire(int chance, int ac, int vis)
{
	int k;

	/* Percentile dice */
	k = rand_int(100);

	/* Hack -- Instant miss or hit */
	if (k < 10) return (k < 5);

	/* Never hit */
	if (chance <= 0) return (FALSE);

	/* Invisible monsters are harder to hit */
	if (!vis) chance = (chance + 1) / 2;

	/* Power competes against armor */
	if (rand_int(chance) < (ac * 3 / 4)) return (FALSE);

	/* Assume hit */
	return (TRUE);
}



/*
 * Determine if the player "hits" a monster (normal combat).
 *
 * Note -- Always miss 5%, always hit 5%, otherwise random.
 */
bool test_hit_norm(int chance, int ac, int vis)
{
	int k;

	/* Percentile dice */
	k = rand_int(100);

	/* Hack -- Instant miss or hit */
	if (k < 10) return (k < 5);

	/* Wimpy attack never hits */
	if (chance <= 0) return (FALSE);

	/* Penalize invisible targets */
	if (!vis) chance = (chance + 1) / 2;

	/* Power must defeat armor */
	if (rand_int(chance) < (ac * 3 / 4)) return (FALSE);

	/* Assume hit */
	return (TRUE);
}



/*
 * Critical hits (from objects thrown by player)
 * Factor in item weight, total plusses, and player level.
 */
s16b critical_shot(int Ind, int weight, int plus, int dam)
{
	player_type *p_ptr = Players[Ind];

	int i, k;

	/* Extract "shot" power */
	i = (weight + ((p_ptr->to_h + plus) * 4) + (p_ptr->lev * 2));

	/* Critical hit */
	if (randint(5000) <= i)
	{
		k = weight + randint(500);

		if (k < 500)
		{
			msg_print(Ind, "It was a good hit!");
			dam = 2 * dam + 5;
		}
		else if (k < 1000)
		{
			msg_print(Ind, "It was a great hit!");
			dam = 2 * dam + 10;
		}
		else
		{
			msg_print(Ind, "It was a superb hit!");
			dam = 3 * dam + 15;
		}
	}

	return (dam);
}



/*
 * Critical hits (by player)
 *
 * Factor in weapon weight, total plusses, player level.
 */
s16b critical_norm(int Ind, int weight, int plus, int dam)
{
	player_type *p_ptr = Players[Ind];

	int i, k;

	/* Extract "blow" power */
	i = (weight + ((p_ptr->to_h + plus) * 5) + (p_ptr->lev * 3));

	/* Chance */
	if (randint(5000) <= i)
	{
		k = weight + randint(650);

		if (k < 400)
		{
			msg_print(Ind, "It was a good hit!");
			dam = 2 * dam + 5;
		}
		else if (k < 700)
		{
			msg_print(Ind, "It was a great hit!");
			dam = 2 * dam + 10;
		}
		else if (k < 900)
		{
			msg_print(Ind, "It was a superb hit!");
			dam = 3 * dam + 15;
		}
		else if (k < 1300)
		{
			msg_print(Ind, "It was a *GREAT* hit!");
			dam = 3 * dam + 20;
		}
		else
		{
			msg_print(Ind, "It was a *SUPERB* hit!");
			dam = ((7 * dam) / 2) + 25;
		}
	}

	return (dam);
}



/*
 * Extract the "total damage" from a given object hitting a given monster.
 *
 * Note that "flasks of oil" do NOT do fire damage, although they
 * certainly could be made to do so.  XXX XXX
 *
 * Note that most brands and slays are x3, except Slay Animal (x2),
 * Slay Evil (x2), and Kill dragon (x5).
 */
s16b tot_dam_aux(int Ind, object_type *o_ptr, int tdam, monster_type *m_ptr)
{
	player_type *p_ptr = Players[Ind];

	int mult = 1;

        monster_race *r_ptr = R_INFO(m_ptr);

	u32b f1, f2, f3;
	bool brand_pois = FALSE;

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3);

	/* Hack -- extract temp branding */
	if (p_ptr->bow_brand)
	  {
	    switch (p_ptr->bow_brand_t)
	      {
	      case BOW_BRAND_ELEC:
		f1 |= TR1_BRAND_ELEC;
		break;
	      case BOW_BRAND_COLD:
		f1 |= TR1_BRAND_COLD;
		break;
	      case BOW_BRAND_FIRE:
		f1 |= TR1_BRAND_FIRE;
		break;
	      case BOW_BRAND_ACID:
		f1 |= TR1_BRAND_ACID;
		break;
	      case BOW_BRAND_POIS:
		brand_pois = TRUE;
		break;
	      }
	  }

	/* Some "weapons" and "ammo" do extra damage */
	switch (o_ptr->tval)
	{
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_DIGGING:
		{
			/* Slay Animal */
			if ((f1 & TR1_SLAY_ANIMAL) &&
			    (r_ptr->flags3 & RF3_ANIMAL))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_ANIMAL;*/

				if (mult < 2) mult = 2;
			}

			/* Slay Evil */
			if ((f1 & TR1_SLAY_EVIL) &&
			    (r_ptr->flags3 & RF3_EVIL))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_EVIL;*/

				if (mult < 2) mult = 2;
			}

			/* Slay Undead */
			if ((f1 & TR1_SLAY_UNDEAD) &&
			    (r_ptr->flags3 & RF3_UNDEAD))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_UNDEAD;*/

				if (mult < 3) mult = 3;
			}

			/* Slay Demon */
			if ((f1 & TR1_SLAY_DEMON) &&
			    (r_ptr->flags3 & RF3_DEMON))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_DEMON;*/

				if (mult < 3) mult = 3;
			}

			/* Slay Orc */
			if ((f1 & TR1_SLAY_ORC) &&
			    (r_ptr->flags3 & RF3_ORC))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_ORC;*/

				if (mult < 3) mult = 3;
			}

			/* Slay Troll */
			if ((f1 & TR1_SLAY_TROLL) &&
			    (r_ptr->flags3 & RF3_TROLL))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_TROLL;*/

				if (mult < 3) mult = 3;
			}

			/* Slay Giant */
			if ((f1 & TR1_SLAY_GIANT) &&
			    (r_ptr->flags3 & RF3_GIANT))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_GIANT;*/

				if (mult < 3) mult = 3;
			}

			/* Slay Dragon  */
			if ((f1 & TR1_SLAY_DRAGON) &&
			    (r_ptr->flags3 & RF3_DRAGON))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_DRAGON;*/

				if (mult < 3) mult = 3;
			}

			/* Execute Dragon */
			if ((f1 & TR1_KILL_DRAGON) &&
			    (r_ptr->flags3 & RF3_DRAGON))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_DRAGON;*/

				if (mult < 5) mult = 5;
			}


			/* Brand (Acid) */
			if (f1 & TR1_BRAND_ACID)
			{
				/* Notice immunity */
				if (r_ptr->flags3 & RF3_IM_ACID)
				{
					/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_IM_ACID;*/
				}

				/* Otherwise, take the damage */
				else
				{
					if (mult < 3) mult = 3;
				}
			}

			/* Brand (Elec) */
			if (f1 & TR1_BRAND_ELEC)
			{
				/* Notice immunity */
				if (r_ptr->flags3 & RF3_IM_ELEC)
				{
					/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_IM_ELEC;*/
				}

				/* Otherwise, take the damage */
				else
				{
					if (mult < 3) mult = 3;
				}
			}

			/* Brand (Fire) */
			if (f1 & TR1_BRAND_FIRE)
			{
				/* Notice immunity */
				if (r_ptr->flags3 & RF3_IM_FIRE)
				{
					/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_IM_FIRE;*/
				}

				/* Otherwise, take the damage */
				else
				{
					if (mult < 3) mult = 3;
				}
			}

			/* Brand (Cold) */
			if (f1 & TR1_BRAND_COLD)
			{
				/* Notice immunity */
				if (r_ptr->flags3 & RF3_IM_COLD)
				{
					/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_IM_COLD;*/
				}

				/* Otherwise, take the damage */
				else
				{
					if (mult < 3) mult = 3;
				}
			}


			/* Brand (Pois) */
			if (brand_pois)
			{
				/* Notice immunity */
				if (r_ptr->flags3 & RF3_IM_POIS)
				{
					/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_IM_POIS;*/
				}

				/* Otherwise, take the damage */
				else
				{
					if (mult < 3) mult = 3;
				}
			}

			break;
		}
	}


	/* Return the total damage */
	return (tdam * mult);
}

/*
 * Extract the "total damage" from a given object hitting a given player.
 *
 * Note that "flasks of oil" do NOT do fire damage, although they
 * certainly could be made to do so.  XXX XXX
 */
s16b tot_dam_aux_player(object_type *o_ptr, int tdam, player_type *p_ptr)
{
	int mult = 1;

	u32b f1, f2, f3;

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3);

	/* Some "weapons" and "ammo" do extra damage */
	switch (o_ptr->tval)
	{
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_DIGGING:
		{
			/* Brand (Acid) */
			if (f1 & TR1_BRAND_ACID)
			{
				/* Notice immunity */
				if (p_ptr->immune_acid)
				{
				}

				/* Otherwise, take the damage */
				else
				{
					if (mult < 3) mult = 3;
				}
			}

			/* Brand (Elec) */
			if (f1 & TR1_BRAND_ELEC)
			{
				/* Notice immunity */
				if (p_ptr->immune_elec)
				{
				}

				/* Otherwise, take the damage */
				else
				{
					if (mult < 3) mult = 3;
				}
			}

			/* Brand (Fire) */
			if (f1 & TR1_BRAND_FIRE)
			{
				/* Notice immunity */
				if (p_ptr->immune_fire)
				{
				}

				/* Otherwise, take the damage */
				else
				{
					if (mult < 3) mult = 3;
				}
			}

			/* Brand (Cold) */
			if (f1 & TR1_BRAND_COLD)
			{
				/* Notice immunity */
				if (p_ptr->immune_cold)
				{
				}

				/* Otherwise, take the damage */
				else
				{
					if (mult < 3) mult = 3;
				}
			}

			break;
		}
	}


	/* Return the total damage */
	return (tdam * mult);
}

/*
 * Searches for hidden things.			-RAK-
 */
 
void search(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int Depth = p_ptr->dun_depth;

	int           y, x, chance;

	cave_type    *c_ptr;
	object_type  *o_ptr;


	/* Start with base search ability */
	chance = p_ptr->skill_srh;

	/* Penalize various conditions */
	if (p_ptr->blind || no_lite(Ind)) chance = chance / 10;
	if (p_ptr->confused || p_ptr->image) chance = chance / 10;

	/* Search the nearby grids, which are always in bounds */
	
	for (y = (p_ptr->py - 1); y <= (p_ptr->py + 1); y++)
	{
		for (x = (p_ptr->px - 1); x <= (p_ptr->px + 1); x++)
		{
			/* Sometimes, notice things */
			if (rand_int(100) < chance)
			{
				/* Access the grid */
				c_ptr = &cave[Depth][y][x];

				/* Access the object */
				o_ptr = &o_list[c_ptr->o_idx];

				/* Invisible trap */
				if (c_ptr->feat == FEAT_INVIS)
				{
					/* Pick a trap */
					pick_trap(Depth, y, x);

					/* Message */
					msg_print(Ind, "You have found a trap.");

					/* Disturb */
					disturb(Ind, 0, 0);
				}

				/* Secret door */
				else if (c_ptr->feat == FEAT_SECRET)
				{
					/* Message */
					msg_print(Ind, "You have found a secret door.");

					/* Pick a door XXX XXX XXX */
					c_ptr->feat = FEAT_DOOR_HEAD + 0x00;

					/* Notice */
					note_spot_depth(Depth, y, x);

					/* Redraw */
					everyone_lite_spot(Depth, y, x);

					/* Disturb */
					disturb(Ind, 0, 0);
				}

				/* Search chests */
				else if (o_ptr->tval == TV_CHEST)
				{
					/* Examine chests for traps */
					if (!object_known_p(Ind, o_ptr) && (chest_traps[o_ptr->pval]))
					{
						/* Message */
						msg_print(Ind, "You have discovered a trap on the chest!");

						/* Know the trap */
						object_known(o_ptr);

						/* Notice it */
						disturb(Ind, 0, 0);
					}
				}
			}
		}
	}
}




/*
 * Player "wants" to pick up an object or gold.
 * Note that we ONLY handle things that can be picked up.
 * See "move_player()" for handling of other things.
 */
void carry(int Ind, int pickup, int confirm)
{
	player_type *p_ptr = Players[Ind];
	int Depth = p_ptr->dun_depth;

	cave_type  *c_ptr = &cave[Depth][p_ptr->py][p_ptr->px];

	object_type *o_ptr;

	char	o_name[160];


	/* Hack -- nothing here to pick up */
	if (!(c_ptr->o_idx)) return;

	/* Ghosts cannot pick things up */
	if ((p_ptr->ghost)) return;

	/* Get the object */
	o_ptr = &o_list[c_ptr->o_idx];

	/* Auto id ? */
	if (p_ptr->auto_id)
	  {
	    object_aware(Ind, o_ptr);
	    object_known(o_ptr);
	  }

	/* Describe the object */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);

	/* Pick up gold */
	if (o_ptr->tval == TV_GOLD)
	{
		/* Disturb */
		disturb(Ind, 0, 0);

		/* Message */
		msg_format(Ind, "You have found %ld gold pieces worth of %s.",
		           (long)o_ptr->pval, o_name);

		/* Collect the gold */
		p_ptr->au += o_ptr->pval;

		/* Redraw gold */
		p_ptr->redraw |= (PR_GOLD);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		/* Delete gold */
		delete_object(Depth, p_ptr->py, p_ptr->px);
	}

	/* Pick it up */
	else
	{
		/* Hack -- disturb */
		disturb(Ind, 0, 0);

		/* Describe the object */
		if (!pickup)
		{
			msg_format(Ind, "You see %s.", o_name);
			Send_floor(Ind, o_ptr->tval);
		}

		/* Note that the pack is too full */
		else if (!inven_carry_okay(Ind, o_ptr))
		{
			msg_format(Ind, "You have no room for %s.", o_name);
			Send_floor(Ind, o_ptr->tval);
		}

		/* Pick up the item (if requested and allowed) */
		else
		{
			int okay = TRUE;

			/* Hack -- query every item */
			if (p_ptr->carry_query_flag && !confirm)
			{
				char out_val[160];
				sprintf(out_val, "Pick up %s? ", o_name);
				Send_pickup_check(Ind, out_val);
				return;
			}

			/* Attempt to pick up an object. */
			if (okay)
			{
				int slot;

                                /* Own it */
                                if (!o_ptr->owner) o_ptr->owner = p_ptr->id;

				/* Carry the item */
				slot = inven_carry(Ind, o_ptr);

				/* Get the item again */
				o_ptr = &(p_ptr->inventory[slot]);

                                if (!o_ptr->level)
                                {
                                        if (p_ptr->dun_depth > 0) o_ptr->level = p_ptr->dun_depth;
                                        else o_ptr->level = -p_ptr->dun_depth;
                                        if (o_ptr->level > 100) o_ptr->level = 100;
                                }

				/* Describe the object */
				object_desc(Ind, o_name, o_ptr, TRUE, 3);

				/* Message */
				msg_format(Ind, "You have %s (%c).", o_name, index_to_label(slot));


#ifndef NEWHOUSES
				/*  hack so it doesnt clear the house -APD- */
				if (o_ptr->tval == TV_KEY) houses[o_ptr->pval].owned = 2;
#endif

				/* Delete original */
				delete_object(Depth, p_ptr->py, p_ptr->px);

				/* Tell the client */
				Send_floor(Ind, 0);
			}
		}
	}
}





/*
 * Determine if a trap affects the player.
 * Always miss 5% of the time, Always hit 5% of the time.
 * Otherwise, match trap power against player armor.
 */
static int check_hit(int Ind, int power)
{
	player_type *p_ptr = Players[Ind];

	int k, ac;

	/* Percentile dice */
	k = rand_int(100);

	/* Hack -- 5% hit, 5% miss */
	if (k < 10) return (k < 5);

	/* Paranoia -- No power */
	if (power <= 0) return (FALSE);

	/* Total armor */
	ac = p_ptr->ac + p_ptr->to_a;

	/* Power competes against Armor */
	if (randint(power) > ((ac * 3) / 4)) return (TRUE);

	/* Assume miss */
	return (FALSE);
}



/*
 * Handle player hitting a real trap
 */
static void hit_trap(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int Depth = p_ptr->dun_depth;

	int			i, num, dam;

	cave_type		*c_ptr;
	byte			*w_ptr;

	cptr		name = "a trap";


	/* Ghosts ignore traps */
	if ((p_ptr->ghost) || (p_ptr->tim_traps)) return;

	/* Disturb the player */
	disturb(Ind, 0, 0);

	/* Get the cave grid */
	c_ptr = &cave[Depth][p_ptr->py][p_ptr->px];
	w_ptr = &p_ptr->cave_flag[p_ptr->py][p_ptr->px];

	/* Analyze XXX XXX XXX */
	switch (c_ptr->feat)
	{
		case FEAT_TRAP_HEAD + 0x00:
		{
			/* MEGAHACK: Ignore Wilderness trap doors. */
			if( p_ptr->dun_depth<0) {
				msg_print(Ind, "\377GYou feel quite certain something really awfull just happened..");
				break;
			}

			msg_print(Ind, "You fell through a trap door!");
			if (p_ptr->feather_fall)
			{
				msg_print(Ind, "You float gently down to the next level.");
			}
			else
			{
				dam = damroll(2, 8);
				take_hit(Ind, dam, name);
			}
			p_ptr->new_level_flag = TRUE;
			p_ptr->new_level_method = LEVEL_RAND;
			
			/* The player is gone */
			cave[p_ptr->dun_depth][p_ptr->py][p_ptr->px].m_idx = 0;

			/* Erase his light */
			forget_lite(Ind);

			/* Show everyone that he's left */
			everyone_lite_spot(p_ptr->dun_depth, p_ptr->py, p_ptr->px);

			/* Reduce the number of players on this depth */
			players_on_depth[p_ptr->dun_depth]--;

			p_ptr->dun_depth++;

			/* Increase the number of players on this next depth */
			players_on_depth[p_ptr->dun_depth]++;

			break;
		}

		case FEAT_TRAP_HEAD + 0x01:
		{
			msg_print(Ind, "You fell into a pit!");
			if (p_ptr->feather_fall)
			{
				msg_print(Ind, "You float gently to the bottom of the pit.");
			}
			else
			{
				dam = damroll(2, 6);
				take_hit(Ind, dam, name);
			}
			break;
		}

		case FEAT_TRAP_HEAD + 0x02:
		{
			msg_print(Ind, "You fall into a spiked pit!");

			if (p_ptr->feather_fall)
			{
				msg_print(Ind, "You float gently to the floor of the pit.");
				msg_print(Ind, "You carefully avoid touching the spikes.");
			}

			else
			{
				/* Base damage */
				dam = damroll(2, 6);

				/* Extra spike damage */
				if (rand_int(100) < 50)
				{
					msg_print(Ind, "You are impaled!");

					dam = dam * 2;
					(void)set_cut(Ind, p_ptr->cut + randint(dam));
				}

				/* Take the damage */
				take_hit(Ind, dam, name);
			}
			break;
		}

		case FEAT_TRAP_HEAD + 0x03:
		{
			msg_print(Ind, "You fall into a spiked pit!");

			if (p_ptr->feather_fall)
			{
				msg_print(Ind, "You float gently to the floor of the pit.");
				msg_print(Ind, "You carefully avoid touching the spikes.");
			}

			else
			{
				/* Base damage */
				dam = damroll(2, 6);

				/* Extra spike damage */
				if (rand_int(100) < 50)
				{
					msg_print(Ind, "You are impaled on poisonous spikes!");

					dam = dam * 2;
					(void)set_cut(Ind, p_ptr->cut + randint(dam));

					if (p_ptr->resist_pois || p_ptr->oppose_pois)
					{
						msg_print(Ind, "The poison does not affect you!");
					}

					else
					{
						dam = dam * 2;
						(void)set_poisoned(Ind, p_ptr->poisoned + randint(dam));
					}
				}

				/* Take the damage */
				take_hit(Ind, dam, name);
			}

			break;
		}

		case FEAT_TRAP_HEAD + 0x04:
		{
			msg_print(Ind, "You are enveloped in a cloud of smoke!");
			c_ptr->feat = FEAT_FLOOR;
			*w_ptr &= ~CAVE_MARK;
			note_spot_depth(Depth, p_ptr->py, p_ptr->px);
			everyone_lite_spot(Depth, p_ptr->py, p_ptr->px);
			num = 2 + randint(3);
			for (i = 0; i < num; i++)
			{
				(void)summon_specific(Depth, p_ptr->py, p_ptr->px, Depth, 0);
			}
			break;
		}

		case FEAT_TRAP_HEAD + 0x05:
		{
			msg_print(Ind, "You hit a teleport trap!");
			teleport_player(Ind, 100);
			break;
		}

		case FEAT_TRAP_HEAD + 0x06:
		{
			msg_print(Ind, "You are enveloped in flames!");
			dam = damroll(4, 6);
			fire_dam(Ind, dam, "a fire trap");
			break;
		}

		case FEAT_TRAP_HEAD + 0x07:
		{
			msg_print(Ind, "You are splashed with acid!");
			dam = damroll(4, 6);
			acid_dam(Ind, dam, "an acid trap");
			break;
		}

		case FEAT_TRAP_HEAD + 0x08:
		{
			if (check_hit(Ind, 125))
			{
				msg_print(Ind, "A small dart hits you!");
				dam = damroll(1, 4);
				take_hit(Ind, dam, name);
				(void)set_slow(Ind, p_ptr->slow + rand_int(20) + 20);
			}
			else
			{
				msg_print(Ind, "A small dart barely misses you.");
			}
			break;
		}

		case FEAT_TRAP_HEAD + 0x09:
		{
			if (check_hit(Ind, 125))
			{
				msg_print(Ind, "A small dart hits you!");
				dam = damroll(1, 4);
				take_hit(Ind, dam, name);
				(void)do_dec_stat(Ind, A_STR);
			}
			else
			{
				msg_print(Ind, "A small dart barely misses you.");
			}
			break;
		}

		case FEAT_TRAP_HEAD + 0x0A:
		{
			if (check_hit(Ind, 125))
			{
				msg_print(Ind, "A small dart hits you!");
				dam = damroll(1, 4);
				take_hit(Ind, dam, name);
				(void)do_dec_stat(Ind, A_DEX);
			}
			else
			{
				msg_print(Ind, "A small dart barely misses you.");
			}
			break;
		}

		case FEAT_TRAP_HEAD + 0x0B:
		{
			if (check_hit(Ind, 125))
			{
				msg_print(Ind, "A small dart hits you!");
				dam = damroll(1, 4);
				take_hit(Ind, dam, name);
				(void)do_dec_stat(Ind, A_CON);
			}
			else
			{
				msg_print(Ind, "A small dart barely misses you.");
			}
			break;
		}

		case FEAT_TRAP_HEAD + 0x0C:
		{
			msg_print(Ind, "A black gas surrounds you!");
			if (!p_ptr->resist_blind)
			{
				(void)set_blind(Ind, p_ptr->blind + rand_int(50) + 25);
			}
			break;
		}

		case FEAT_TRAP_HEAD + 0x0D:
		{
			msg_print(Ind, "A gas of scintillating colors surrounds you!");
			if (!p_ptr->resist_conf)
			{
				(void)set_confused(Ind, p_ptr->confused + rand_int(20) + 10);
			}
			break;
		}

		case FEAT_TRAP_HEAD + 0x0E:
		{
			msg_print(Ind, "A pungent green gas surrounds you!");
			if (!p_ptr->resist_pois && !p_ptr->oppose_pois)
			{
				(void)set_poisoned(Ind, p_ptr->poisoned + rand_int(20) + 10);
			}
			break;
		}

		case FEAT_TRAP_HEAD + 0x0F:
		{
			msg_print(Ind, "A strange white mist surrounds you!");
			if (!p_ptr->free_act)
			{
				(void)set_paralyzed(Ind, p_ptr->paralyzed + rand_int(10) + 5);
			}
			break;
		}
	}
}



/*
 * Player attacks another player!
 *
 * If no "weapon" is available, then "punch" the player one time.
 */
void py_attack_player(int Ind, int y, int x)
{
	player_type *p_ptr = Players[Ind];
	int Depth = p_ptr->dun_depth;

	int num = 0, k, bonus, chance;

	cave_type *c_ptr = &cave[Depth][y][x];

	player_type *q_ptr = Players[0 - c_ptr->m_idx];

	object_type *o_ptr;

	char p_name[80];

	bool do_quake = FALSE;

	/* Disturb both players */
	disturb(Ind, 0, 0);
	disturb(0 - c_ptr->m_idx, 0, 0);

	/* Extract name */
	strcpy(p_name, q_ptr->name);

	/* Track player health */
	if (p_ptr->play_vis[0 - c_ptr->m_idx]) health_track(Ind, c_ptr->m_idx);

	/* If target isn't already hostile toward attacker, make it so */
	if (!check_hostile(0 - c_ptr->m_idx, Ind))
	{
		/* Make hostile */
		add_hostility(0 - c_ptr->m_idx, p_ptr->name);
	}

	/* Handle attacker fear */
	if (p_ptr->afraid)
	{
		/* Message */
		msg_format(Ind, "You are too afraid to attack %s!", p_name);

		/* Done */
		return;
	}

	/* Access the weapon */
	o_ptr = &(p_ptr->inventory[INVEN_WIELD]);

	/* Calculate the "attack quality" */
	bonus = p_ptr->to_h + o_ptr->to_h;
	chance = (p_ptr->skill_thn + (bonus * BTH_PLUS_ADJ));


	/* Attack once for each legal blow */
	while (num++ < p_ptr->num_blow)
	{
		/* Test for hit */
		if (test_hit_norm(chance, q_ptr->ac + q_ptr->to_a, 1))
		{
			/* Sound */
			sound(Ind, SOUND_HIT);

			/* Messages */
			msg_format(Ind, "You hit %s.", p_name);
			msg_format(0 - c_ptr->m_idx, "%s hits you.", p_ptr->name);

			/* Hack -- bare hands do one damage */
			k = 1;

			/* Ghosts do damages relative to level */
			if (p_ptr->ghost)
				k = p_ptr->lev;
			if (p_ptr->fruit_bat)
				k = p_ptr->lev * ((p_ptr->lev / 10) + 1);

			if (p_ptr->pclass == CLASS_MONK)
			{
				int special_effect = 0, stun_effect = 0, times = 0;
				martial_arts *ma_ptr = &ma_blows[0], *old_ptr = &ma_blows[0];
				int resist_stun = 0;
				if (q_ptr->resist_conf) resist_stun += 44;
				if (q_ptr->free_act) resist_stun += 44;

				for (times = 0; times < (p_ptr->lev<7?1:p_ptr->lev/7); times++)
				/* Attempt 'times' */
				{
					do
					{
						ma_ptr = &ma_blows[rand_int(MAX_MA)];
					}
					while ((ma_ptr->min_level > p_ptr->lev)
					    || (randint(p_ptr->lev)<ma_ptr->chance));

					/* keep the highest level attack available we found */
					if ((ma_ptr->min_level > old_ptr->min_level) &&
					    !(p_ptr->stun || p_ptr->confused))
					{
						old_ptr = ma_ptr;
					}
					else
					{
						ma_ptr = old_ptr;
					}
				}

				k = damroll(ma_ptr->dd, ma_ptr->ds);

				if (ma_ptr->effect == MA_KNEE)
				{
					if (q_ptr->male)
					{
						msg_format(Ind, "You hit %s in the groin with your knee!", q_ptr->name);
						special_effect = MA_KNEE;
					}
					else
						msg_format(Ind, ma_ptr->desc, q_ptr->name);
				}

				else
				{
					if (ma_ptr->effect)
					{
						stun_effect = (ma_ptr->effect/2) + randint(ma_ptr->effect/2);
					}

					msg_format(Ind, ma_ptr->desc, q_ptr->name);
				}

				k = critical_norm(Ind, p_ptr->lev * (randint(10)), ma_ptr->min_level, k);

				if ((special_effect == MA_KNEE) && ((k + p_ptr->to_d) < q_ptr->chp))
				{
					msg_format(Ind, "%^s moans in agony!", q_ptr->name);
					stun_effect = 7 + randint(13);
					resist_stun /= 3;
				}

				if (stun_effect && ((k + p_ptr->to_d) < q_ptr->chp))
				{
					if (p_ptr->lev > randint((q_ptr->lev * 2) + resist_stun + 10))
					{
						msg_format(Ind, "\377o%^s is stunned.", q_ptr->name);

						set_stun(0 - c_ptr->m_idx, q_ptr->stun + stun_effect);
					}
				}
			}

			/* Handle normal weapon */
			else if (o_ptr->k_idx)
			{
				k = damroll(o_ptr->dd, o_ptr->ds);
				k = tot_dam_aux_player(o_ptr, k, q_ptr);
				if (p_ptr->impact && (k > 50)) do_quake = TRUE;
				k = critical_norm(Ind, o_ptr->weight, o_ptr->to_h, k);
				k += o_ptr->to_d;
			}

			/* Apply the player damage bonuses */
			k += p_ptr->to_d;

			/* No negative damage */
			if (k < 0) k = 0;

			/* XXX Reduce damage by 1/3 */
			k = (k + 2) / 3;

			/* Damage */
			take_hit(0 - c_ptr->m_idx, k, p_ptr->name);

			if(!c_ptr->m_idx) break;

			/* Check for death */
			if (q_ptr->death) break;

			/* Confusion attack */
			if (p_ptr->confusing)
			{
				/* Cancel glowing hands */
				p_ptr->confusing = FALSE;

				/* Message */
				msg_print(Ind, "Your hands stop glowing.");

				/* Confuse the monster */
				if (q_ptr->resist_conf)
				{
					msg_format(Ind, "%^s is unaffected.", p_name);
				}
				else if (rand_int(100) < q_ptr->lev)
				{
					msg_format(Ind, "%^s is unaffected.", p_name);
				}
				else
				{
					msg_format(Ind, "%^s appears confused.", p_name);
					set_confused(0 - c_ptr->m_idx, q_ptr->confused + 10 + rand_int(p_ptr->lev) / 5);
				}
			}

			/* Stunning attack */
			if (p_ptr->stunning)
			{
				/* Cancel heavy hands */
				p_ptr->stunning = FALSE;

				/* Message */
				msg_print(Ind, "Your hands feel less heavy.");

				/* Stun the monster */
				if (rand_int(100) < q_ptr->lev)
				{
					msg_format(Ind, "%^s is unaffected.", p_name);
				}
				else
				{
					msg_format(Ind, "\377o%^s appears stunned.", p_name);
					set_stun(0 - c_ptr->m_idx, q_ptr->stun + 20 + rand_int(p_ptr->lev) / 5);
				}
			}

			/* Ghosts get fear attacks */
			if (p_ptr->ghost)
			{
				int fear_chance = 50 + (p_ptr->lev - q_ptr->lev) * 5;

				if (rand_int(100) < fear_chance)
				{
					msg_format(Ind, "%^s appears afraid.", p_name);
					set_afraid(0 - c_ptr->m_idx, q_ptr->afraid + 4 + rand_int(p_ptr->lev) / 5);
				}
			}

			/* Fruit bats get life stealing */
			if (p_ptr->fruit_bat)
			{
				int leech;
				
				leech = q_ptr->chp;
				if (k < leech) leech = k;
				leech /= 10;
			
				hp_player(Ind, rand_int(leech));
			}
		}

		/* Player misses */
		else
		{
			/* Sound */
			sound(Ind, SOUND_MISS);

			/* Messages */
			msg_format(Ind, "You miss %s.", p_name);
			msg_format(0 - c_ptr->m_idx, "%s misses you.", p_ptr->name);
		}
	}

	/* Mega-Hack -- apply earthquake brand */
	if (do_quake)
	  {
	    if (o_ptr->k_idx && check_guard_inscription(o_ptr->note, 'E' ))
	      earthquake(p_ptr->dun_depth, p_ptr->py, p_ptr->px, 10);
	  }
}



/*
 * Player attacks a (poor, defenseless) creature	-RAK-
 *
 * If no "weapon" is available, then "punch" the monster one time.
 */
void py_attack_mon(int Ind, int y, int x)
{
	player_type *p_ptr = Players[Ind];
	int Depth = p_ptr->dun_depth;

	int			num = 0, k, bonus, chance;

	cave_type		*c_ptr = &cave[Depth][y][x];

	monster_type	*m_ptr = &m_list[c_ptr->m_idx];
        monster_race    *r_ptr = R_INFO(m_ptr);

	object_type		*o_ptr;

	char		m_name[80];

	bool		fear = FALSE;

	bool		do_quake = FALSE;

	bool		backstab = FALSE, stab_fleeing = FALSE;

	/* Disturb the player */
	disturb(Ind, 0, 0);


	/* Extract monster name (or "it") */
	monster_desc(Ind, m_name, c_ptr->m_idx, 0);

        if (m_ptr->owner == p_ptr->id)
        {
                int ox = m_ptr->fx, oy = m_ptr->fy, nx = p_ptr->px, ny = p_ptr->py;

                msg_format(Ind, "You swap positions with %s.", m_name);
                
                /* Update the new location */
                cave[Depth][ny][nx].m_idx = c_ptr->m_idx;

                /* Update the old location */
                cave[Depth][oy][ox].m_idx = 0;

                /* Move the monster */
                m_ptr->fy = ny;
                m_ptr->fx = nx;
                p_ptr->py = oy;
                p_ptr->px = ox;

                /* Update the monster (new location) */
                update_mon(cave[Depth][ny][nx].m_idx, TRUE);

                /* Redraw the old grid */
                everyone_lite_spot(Depth, oy, ox);

                /* Redraw the new grid */
                everyone_lite_spot(Depth, ny, nx);

                /* Check for new panel (redraw map) */
                verify_panel(Ind);

                /* Update stuff */
                p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

                /* Update the monsters */
                p_ptr->update |= (PU_DISTANCE);

                /* Window stuff */
                p_ptr->window |= (PW_OVERHEAD);

                /* Handle stuff XXX XXX XXX */
                handle_stuff(Ind);

                return;
        }

	/* Auto-Recall if possible and visible */
	if (p_ptr->mon_vis[c_ptr->m_idx]) recent_track(m_ptr->r_idx);

	/* Track a new monster */
	if (p_ptr->mon_vis[c_ptr->m_idx]) health_track(Ind, c_ptr->m_idx);


	/* Handle player fear */
	if (p_ptr->afraid)
	{
		/* Message */
		msg_format(Ind, "You are too afraid to attack %s!", m_name);

		/* Done */
		return;
	}

	if (p_ptr->pclass == CLASS_ROGUE)
	{
		if(m_ptr->csleep /*&& m_ptr->ml*/)
			backstab = TRUE;
		else if (m_ptr->monfear /*&& m_ptr->ml)*/)
			stab_fleeing = TRUE;
	}
		
	
	/* Disturb the monster */
	m_ptr->csleep = 0;


	/* Access the weapon */
	o_ptr = &(p_ptr->inventory[INVEN_WIELD]);

	/* Calculate the "attack quality" */
	bonus = p_ptr->to_h + o_ptr->to_h;
	chance = (p_ptr->skill_thn + (bonus * BTH_PLUS_ADJ));


	/* Attack once for each legal blow */
	while (num++ < p_ptr->num_blow)
	{
		/* Test for hit */
                if (test_hit_norm(chance, m_ptr->ac, p_ptr->mon_vis[c_ptr->m_idx]))
		{
			/* Sound */
			sound(Ind, SOUND_HIT);

			/* Message */
			if ((!backstab) && (!stab_fleeing))
				msg_format(Ind, "You hit %s.", m_name);
			else if(backstab)
                                msg_format(Ind, "You cruelly stab the helpless, sleeping %s!", r_name_get(m_ptr));
			else
                                msg_format(Ind, "You backstab the fleeing %s!", r_name_get(m_ptr));

			/* Hack -- bare hands do one damage */
			k = 1;

			/* Ghosts get damage relative to level */
			if (p_ptr->ghost)
				k = p_ptr->lev;
				
			if (p_ptr->fruit_bat)
				k = p_ptr->lev * ((p_ptr->lev / 10) + 1);

			if (p_ptr->pclass == CLASS_MONK)
			{
				int special_effect = 0, stun_effect = 0, times = 0;
				martial_arts * ma_ptr = &ma_blows[0], * old_ptr = &ma_blows[0];
				int resist_stun = 0;
				if (r_ptr->flags1 & RF1_UNIQUE) resist_stun += 88;
				if (r_ptr->flags3 & RF3_NO_CONF) resist_stun += 44;
				if (r_ptr->flags3 & RF3_NO_SLEEP) resist_stun += 44;
				if (r_ptr->flags3 & RF3_UNDEAD)
					resist_stun += 88;

				for (times = 0; times < (p_ptr->lev<7?1:p_ptr->lev/7); times++)
				/* Attempt 'times' */
				{
					do
					{
						ma_ptr = &ma_blows[(randint(MAX_MA))-1];
					}
					while ((ma_ptr->min_level > p_ptr->lev)
					    || (randint(p_ptr->lev)<ma_ptr->chance));

					/* keep the highest level attack available we found */
					if ((ma_ptr->min_level > old_ptr->min_level) &&
					    !(p_ptr->stun || p_ptr->confused))
					{
						old_ptr = ma_ptr;
					}
					else
					{
						ma_ptr = old_ptr;
					}
				}

				k = damroll(ma_ptr->dd, ma_ptr->ds);

				if (ma_ptr->effect == MA_KNEE)
				{
					if (r_ptr->flags1 & RF1_MALE)
					{
						msg_format(Ind, "You hit %s in the groin with your knee!", m_name);
						special_effect = MA_KNEE;
					}
					else
						msg_format(Ind, ma_ptr->desc, m_name);
				}

				else if (ma_ptr->effect == MA_SLOW)
				{
					if (!((r_ptr->flags1 & RF1_NEVER_MOVE)
					    || strchr("UjmeEv$,DdsbBFIJQSXclnw!=?", r_ptr->d_char)))
					{
						msg_format(Ind, "You kick %s in the ankle.", m_name);
						special_effect = MA_SLOW;
					}
					else msg_format(Ind, ma_ptr->desc, m_name);
				}
				else
				{
					if (ma_ptr->effect)
					{
						stun_effect = (ma_ptr->effect/2) + randint(ma_ptr->effect/2);
					}

					msg_format(Ind, ma_ptr->desc, m_name);
				}

				k = critical_norm(Ind, p_ptr->lev * (randint(10)), ma_ptr->min_level, k);

				if ((special_effect == MA_KNEE) && ((k + p_ptr->to_d) < m_ptr->hp))
				{
					msg_format(Ind, "%^s moans in agony!", m_name);
					stun_effect = 7 + randint(13);
					resist_stun /= 3;
				}

				else if ((special_effect == MA_SLOW) && ((k + p_ptr->to_d) < m_ptr->hp))
				{
					if (!(r_ptr->flags1 & RF1_UNIQUE) &&
                                            (randint(p_ptr->lev * 2) > r_ptr->level) &&
					    m_ptr->mspeed > 60)
					{
						msg_format(Ind, "\377o%^s starts limping slower.", m_name);
						m_ptr->mspeed -= 10;
					}
				}

				if (stun_effect && ((k + p_ptr->to_d) < m_ptr->hp))
				{
					if (p_ptr->lev > randint(r_ptr->level + resist_stun + 10))
					{
						if (m_ptr->stunned)
                                                        msg_format(Ind, "\377o%^s is still stunned.", m_name);
						else
							msg_format(Ind, "\377o%^s is stunned.", m_name);

						m_ptr->stunned += (stun_effect);
					}
				}
			}

			/* Handle normal weapon */
			else if (o_ptr->k_idx)
			{
				k = damroll(o_ptr->dd, o_ptr->ds);
				k = tot_dam_aux(Ind, o_ptr, k, m_ptr);
				if (backstab)
				{
					backstab = FALSE;
					k *= (3 + (p_ptr->lev / 40));
				}
				else if (stab_fleeing)
				{
					k = ((3 * k) / 2);
				}
				if (p_ptr->impact && (k > 50)) do_quake = TRUE;
				k = critical_norm(Ind, o_ptr->weight, o_ptr->to_h, k);
				k += o_ptr->to_d;
			}

			/* Apply the player damage bonuses */
			k += p_ptr->to_d;

			/* No negative damage */
			if (k < 0) k = 0;

			/* Complex message */
			if (wizard)
			{
				msg_format(Ind, "You do %d (out of %d) damage.", k, m_ptr->hp);
			}

			/* Damage, check for fear and death */
			if (mon_take_hit(Ind, c_ptr->m_idx, k, &fear, NULL)) break;

			/* Confusion attack */
			if (p_ptr->confusing)
			{
				/* Cancel glowing hands */
				p_ptr->confusing = FALSE;

				/* Message */
				msg_print(Ind, "Your hands stop glowing.");

				/* Confuse the monster */
				if (r_ptr->flags3 & RF3_NO_CONF)
				{
					if (p_ptr->mon_vis[c_ptr->m_idx]) r_ptr->r_flags3 |= RF3_NO_CONF;
					msg_format(Ind, "%^s is unaffected.", m_name);
				}
				else if (rand_int(100) < r_ptr->level)
				{
					msg_format(Ind, "%^s is unaffected.", m_name);
				}
				else
				{
					msg_format(Ind, "%^s appears confused.", m_name);
					m_ptr->confused += 10 + rand_int(p_ptr->lev) / 5;
				}
			}

			/* Stunning attack */
			if (p_ptr->stunning)
			{
				/* Cancel heavy hands */
				p_ptr->stunning = FALSE;

				/* Message */
				msg_print(Ind, "Your hands feel less heavy.");

				/* Stun the monster */
				if (r_ptr->flags3 & RF3_NO_STUN)
				{
					if (p_ptr->mon_vis[c_ptr->m_idx]) r_ptr->r_flags3 |= RF3_NO_STUN;
					msg_format(Ind, "%^s is unaffected.", m_name);
				}
				else if (rand_int(115) < r_ptr->level)
				{
					msg_format(Ind, "%^s is unaffected.", m_name);
				}
				else
				{
					if (!m_ptr->stunned)
						msg_format(Ind, "\377o%^s appears stuned.", m_name);
					else
						msg_format(Ind, "\377o%^s appears more stunned.", m_name);
					m_ptr->stunned += 20 + rand_int(p_ptr->lev) / 5;
				}
			}

			/* Ghosts get fear attacks */
			if (p_ptr->ghost)
			{
				int fear_chance = 50 + (p_ptr->lev - r_ptr->level) * 5;

				if (!(r_ptr->flags3 & RF3_NO_FEAR) && rand_int(100) < fear_chance)
				{
					msg_format(Ind, "%^s appears afraid.", m_name);
					m_ptr->monfear = m_ptr->monfear + 4 + rand_int(p_ptr->lev) / 5;
				}
			}


			/* Fruit bats get life stealing */
			if (p_ptr->fruit_bat)
			{
				int leech;
				
				leech = m_ptr->hp;
				if (k < leech) leech = k;
				leech /= 10;
			
				hp_player(Ind, rand_int(leech));
			}
		}

		/* Player misses */
		else
		{
			/* Sound */
			sound(Ind, SOUND_MISS);

			backstab = FALSE;

			/* Message */
			msg_format(Ind, "You miss %s.", m_name);
		}
	}


	/* Hack -- delay fear messages */
	if (fear && p_ptr->mon_vis[c_ptr->m_idx])
	{
		/* Sound */
		sound(Ind, SOUND_FLEE);

		/* Message */
		msg_format(Ind, "%^s flees in terror!", m_name);
	}


	/* Mega-Hack -- apply earthquake brand */
	if (do_quake)
	  {
	    if (o_ptr->k_idx && check_guard_inscription(o_ptr->note, 'E' ))
	      earthquake(p_ptr->dun_depth, p_ptr->py, p_ptr->px, 10);
	  }
}


/*
 * Attacking something, figure out what and spawn appropriately.
 */
void py_attack(int Ind, int y, int x)
{
	player_type *p_ptr = Players[Ind];
	int Depth = p_ptr->dun_depth;
	cave_type *c_ptr = &cave[Depth][y][x];

	if (r_info[p_ptr->body_monster].flags1 & RF1_NEVER_BLOW) return;

	/* Check for monster */
	if (c_ptr->m_idx > 0)
		py_attack_mon(Ind, y, x);

	/* Check for player */
	if (c_ptr->m_idx < 0)
		py_attack_player(Ind, y, x);
}

/* Do a probability travel in a wall */
void do_prob_travel(int Ind, int dir)
{
  player_type *p_ptr = Players[Ind];
  int x = p_ptr->px, y = p_ptr->py;
  bool do_move = TRUE;

  /* Paranoia */
  if (dir == 5) return;
  if ((dir < 1) || (dir > 9)) return;

  x += ddx[dir];
  y += ddy[dir];

  while (TRUE)
    {
      /* Do not get out of the level */
      if (!in_bounds(p_ptr->dun_depth, y, x))
	{
	  do_move = FALSE;
	  break;
	}

      /* Still in rock ? continue */
      if ((!cave_empty_bold(p_ptr->dun_depth, y, x)) || (cave[p_ptr->dun_depth][y][x].info & CAVE_ICKY))
	{
	  y += ddy[dir];
	  x += ddx[dir];
	  continue;
	}

      /* Everything is ok */
      do_move = TRUE;
      break;
    }

  if (do_move)
    {
      int oy, ox;

      /* Save old location */
      oy = p_ptr->py;
      ox = p_ptr->px;

      /* Move the player */
      p_ptr->py = y;
      p_ptr->px = x;

      /* Update the player indices */
      cave[p_ptr->dun_depth][oy][ox].m_idx = 0;
      cave[p_ptr->dun_depth][y][x].m_idx = 0 - Ind;

      /* Redraw new spot */
      everyone_lite_spot(p_ptr->dun_depth, p_ptr->py, p_ptr->px);

      /* Redraw old spot */
      everyone_lite_spot(p_ptr->dun_depth, oy, ox);

      /* Check for new panel (redraw map) */
      verify_panel(Ind);

      /* Update stuff */
      p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

      /* Update the monsters */
      p_ptr->update |= (PU_DISTANCE);

      /* Window stuff */
      p_ptr->window |= (PW_OVERHEAD);

      /* Hack -- quickly update the view, to reduce perceived lag */
      redraw_stuff(Ind);
      window_stuff(Ind);
    }
}

/*
 * Move player in the given direction, with the given "pickup" flag.
 *
 * This routine should (probably) always induce energy expenditure.
 *
 * Note that moving will *always* take a turn, and will *always* hit
 * any monster which might be in the destination grid.  Previously,
 * moving into walls was "free" and did NOT hit invisible monsters.
 */
 
 /* Bounds checking is used in dungeon levels <= 0, which is used
    to move between wilderness levels. 
    
    The wilderness levels are stored in rings radiating from the town,
    see calculate_world_index for more information.

    Diagonals aren't handled properly, but I don't feel that is important.
        
    -APD- 
 */
 
void move_player(int Ind, int dir, int do_pickup)
{
	player_type *p_ptr = Players[Ind];
	int Depth = p_ptr->dun_depth;

	int			y, x, old_world_x, old_world_y, oldx, oldy;

	cave_type		*c_ptr;
	object_type		*o_ptr;
	monster_type	*m_ptr;
	byte			*w_ptr;
	monster_race *r_ptr = &r_info[p_ptr->body_monster];


	/* Can we move ? */
	if (r_ptr->flags1 & RF1_NEVER_MOVE) return;

	/* Find the result of moving */
	if ((r_ptr->flags1 & RF1_RAND_50) && magik(50))
	  {
	    int i = rand_int(10);
	    y = p_ptr->py + ddy[i];
	    x = p_ptr->px + ddx[i];
	  }
	else if ((r_ptr->flags1 & RF1_RAND_25) && magik(25))
	  {
	    int i = rand_int(10);
	    y = p_ptr->py + ddy[i];
	    x = p_ptr->px + ddx[i];
	  }
	else
	  {
	    y = p_ptr->py + ddy[dir];
	    x = p_ptr->px + ddx[dir];
	  }

	/* Update wilderness positions */
	if (p_ptr->dun_depth <= 0)
	{
		/* Make sure he hasn't just changed depth */
		if (p_ptr->new_level_flag) return;
		
		/* save his old location */
		old_world_x = p_ptr->world_x; old_world_y = p_ptr->world_y;
		oldx = p_ptr->px; oldy = p_ptr->py;
		
		/* we have gone off the map */
		if (!in_bounds(Depth, y, x))
		{
			/* find his new location */
			if (y <= 0)
			{	
				/* new player location */
				p_ptr->world_y++;
				p_ptr->py = MAX_HGT-2;
			}
			if (y >= 65)
			{			
				/* new player location */  
				p_ptr->world_y--;
				p_ptr->py = 1;
			}
			if (x <= 0)
			{
				/* new player location */
				p_ptr->world_x--;
				p_ptr->px = MAX_WID-2;
			}
			if (x >= 197)
			{
				/* new player location */
				p_ptr->world_x++;			
				p_ptr->px = 1;
			}
		
			/* check to make sure he hasnt hit the edge of the world */
			if (world_index(p_ptr->world_x, p_ptr->world_y) <= -MAX_WILD) 
			{
				p_ptr->world_x = old_world_x;
				p_ptr->world_y = old_world_y;
				p_ptr->px = oldx;
				p_ptr->py = oldy;
				return;
			}
			
			/* Remove the player from the old location */
			cave[Depth][oldy][oldx].m_idx = 0;
			
			/* Show everyone that's he left */
			everyone_lite_spot(Depth, oldy, oldx);
		
			/* forget his light and viewing area */
			forget_lite(Ind);
			forget_view(Ind);			
		
			/* Hack -- take a turn */
			p_ptr->energy -= level_speed(p_ptr->dun_depth);
			
			/* A player has left this depth */
			players_on_depth[p_ptr->dun_depth]--;
			
			/* (required) paranoia, allows integration of old wilderness
			   saves onto new severs.
			*/
			if (players_on_depth[p_ptr->dun_depth] < 0)
				players_on_depth[p_ptr->dun_depth] = 0;
			
			/* Calculate the new level index */
			p_ptr->dun_depth = world_index(p_ptr->world_x, p_ptr->world_y);
		
			/* update the wilderness map */
			p_ptr->wild_map[(-p_ptr->dun_depth)/8] |= (1<<((-p_ptr->dun_depth)%8));
			
			players_on_depth[p_ptr->dun_depth]++;
			p_ptr->new_level_flag = TRUE;
			p_ptr->new_level_method = LEVEL_OUTSIDE;

			return;
		}
	}

	

	/* Examine the destination */
	c_ptr = &cave[Depth][y][x];
	w_ptr = &p_ptr->cave_flag[y][x];

	/* Get the object */
	o_ptr = &o_list[c_ptr->o_idx];

	/* Get the monster */
	m_ptr = &m_list[c_ptr->m_idx];

	/* Save "last direction moved" */
	p_ptr->last_dir = dir;

	/* Bump into other players */
	if (c_ptr->m_idx < 0)
	{
		player_type *q_ptr = Players[0 - c_ptr->m_idx];
		int Ind2 = 0 - c_ptr->m_idx;

#ifdef PLAYER_INTERACTION
		/* Check for an attack */
		if (check_hostile(Ind, Ind2))
			py_attack(Ind, y, x);
#else
		/* XXX */
		if (0);
#endif

		/* If both want to switch, do it */
		else if ((!p_ptr->ghost && !q_ptr->ghost &&
		         (ddy[q_ptr->last_dir] == -(ddy[dir])) &&
		         (ddx[q_ptr->last_dir] == (-ddx[dir]))) ||
			(!strcmp(q_ptr->name,cfg_dungeon_master)))
		{
		  if (!((Depth <= 0) && (p_ptr->tim_wraith || q_ptr->tim_wraith)))
		    {

			c_ptr->m_idx = 0 - Ind;
			cave[Depth][p_ptr->py][p_ptr->px].m_idx = 0 - Ind2;

			q_ptr->py = p_ptr->py;
			q_ptr->px = p_ptr->px;

			p_ptr->py = y;
			p_ptr->px = x;

			/* Tell both of them */
			/* Don't tell people they bumped into the Dungeon Master */
			if (strcmp(q_ptr->name,cfg_dungeon_master))
			{
				/* Hack if invisible */
				if (p_ptr->play_vis[Ind2])				
					msg_format(Ind, "You switch places with %s.", q_ptr->name);
				else
					msg_format(Ind, "You switch places with it.");
				
				/* Hack if invisible */
				if (q_ptr->play_vis[Ind])
					msg_format(Ind2, "You switch places with %s.", p_ptr->name);
				else
					msg_format(Ind2, "You switch places with it.");
			}

			/* Disturb both of them */
			disturb(Ind, 1, 0);
			disturb(Ind2, 1, 0);

			/* Re-show both grids */
			everyone_lite_spot(Depth, p_ptr->py, p_ptr->px);
			everyone_lite_spot(Depth, q_ptr->py, q_ptr->px);

			p_ptr->update |= PU_LITE;
			q_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);
		    }		
		}

		/* Hack -- the Dungeon Master cannot bump people */
		else if (strcmp(p_ptr->name,cfg_dungeon_master))
		{
			/* Tell both about it */
			/* Hack if invisible */
			if (p_ptr->play_vis[Ind2])
				msg_format(Ind, "You bump into %s.", q_ptr->name);
			else
				msg_format(Ind, "You bump into it.");
			
			/* Hack if invisible */
			if (q_ptr->play_vis[Ind])
				msg_format(Ind2, "%s bumps into you.", p_ptr->name);
			else
				msg_format(Ind2, "It bumps into you.");

			/* Disturb both parties */
			disturb(Ind, 1, 0);
			disturb(Ind2, 1, 0);
		}
	}

	/* Hack -- attack monsters */
	else if (c_ptr->m_idx > 0)
	{
		/* Hack -- the dungeon master switches places with his monsters */
		if (!strcmp(p_ptr->name,cfg_dungeon_master))
		{
			/* save old player location */
			oldx = p_ptr->px;
			oldy = p_ptr->py;
			/* update player location */
			p_ptr->px = m_list[c_ptr->m_idx].fx;
			p_ptr->py = m_list[c_ptr->m_idx].fy;
			/* update monster location */
			m_list[c_ptr->m_idx].fx = oldx;
			m_list[c_ptr->m_idx].fy = oldy;
			/* update cave monster indexes */
			cave[Depth][oldy][oldx].m_idx = c_ptr->m_idx;
			c_ptr->m_idx = -Ind;

			/* Re-show both grids */
			everyone_lite_spot(Depth, p_ptr->py, p_ptr->px);
			everyone_lite_spot(Depth, oldy, oldx);
		}
		/* Attack */
		else py_attack(Ind, y, x);
	}

	/* Prob travel */
	else if (p_ptr->prob_travel && (!cave_floor_bold(Depth, y, x)))
	  {
	    do_prob_travel(Ind, dir);
	  }

	/* Player can not walk through "walls", but ghosts can */
	else if ((!p_ptr->ghost) && (!p_ptr->tim_wraith) && (!cave_floor_bold(Depth, y, x)))
	{
		/* Disturb the player */
		disturb(Ind, 0, 0);

		/* Notice things in the dark */
		if (!(*w_ptr & CAVE_MARK) &&
		    (p_ptr->blind || !(*w_ptr & CAVE_LITE)))
		{
			/* Rubble */
			if (c_ptr->feat == FEAT_RUBBLE)
			{
				msg_print(Ind, "\377GYou feel some rubble blocking your way.");
				*w_ptr |= CAVE_MARK;
				everyone_lite_spot(Depth, y, x);
			}

			/* Closed door */
			else if ((c_ptr->feat < FEAT_SECRET && c_ptr->feat >= FEAT_DOOR_HEAD) ||
			         (c_ptr->feat >= FEAT_HOME_HEAD && c_ptr->feat <= FEAT_HOME_TAIL))
			{
				msg_print(Ind, "\377GYou feel a closed door blocking your way.");
				*w_ptr |= CAVE_MARK;
				everyone_lite_spot(Depth, y, x);
			}

			/* Tree */
			else if (c_ptr->feat == FEAT_TREE)
			{
				msg_print(Ind, "\377GYou feel a tree blocking your way.");
				*w_ptr |= CAVE_MARK;
				everyone_lite_spot(Depth, y, x);
			}

			/* Wall (or secret door) */
			else
			{
				msg_print(Ind, "\377GYou feel a wall blocking your way.");
				*w_ptr |= CAVE_MARK;
				everyone_lite_spot(Depth, y, x);
			}
		}

		/* Notice things */
		else
		{
			/* Rubble */
			if (c_ptr->feat == FEAT_RUBBLE)
			{
				msg_print(Ind, "There is rubble blocking your way.");
			}

			/* Closed doors */
			else if ((c_ptr->feat < FEAT_SECRET && c_ptr->feat >= FEAT_DOOR_HEAD) || 
			         (c_ptr->feat >= FEAT_HOME_HEAD && c_ptr->feat <= FEAT_HOME_TAIL))
			{
				msg_print(Ind, "There is a closed door blocking your way.");
			}

			/* Tree */
			else if (c_ptr->feat == FEAT_TREE)
			{
				msg_print(Ind, "There is a tree blocking your way.");
			}

			/* Wall (or secret door) */
			else
			{
				msg_print(Ind, "There is a wall blocking your way.");
			}
		}
	}

	/* Wraiths trying to walk into a house */
	else if (p_ptr->tim_wraith && (((c_ptr->feat >= FEAT_HOME_HEAD) && (c_ptr->feat <= FEAT_HOME_TAIL)) ||
	         ((cave[Depth][y][x].info & CAVE_ICKY) && (Depth <= 0))))
	{
		disturb(Ind, 0, 0);
	}

	/* Wraiths can't enter vaults so easily :) trying to walk into a permanent wall */
	else if (p_ptr->tim_wraith && c_ptr->feat >= FEAT_PERM_EXTRA && (Depth > 0))
	{
		/* Message */
		msg_print(Ind, "The wall blocks your movement.");

		disturb(Ind, 0, 0);
	}

	/* Ghost trying to walk into a permanent wall */
	else if ((p_ptr->ghost || p_ptr->tim_wraith) && c_ptr->feat >= FEAT_PERM_SOLID)
	{
		/* Message */
		msg_print(Ind, "The wall blocks your movement.");

		disturb(Ind, 0, 0);
	}

	/* Normal movement */
	else
	{
		int oy, ox;

		/* Save old location */
		oy = p_ptr->py;
		ox = p_ptr->px;

		/* Move the player */
		p_ptr->py = y;
		p_ptr->px = x;

		/* Update the player indices */
		cave[Depth][oy][ox].m_idx = 0;
		cave[Depth][y][x].m_idx = 0 - Ind;

		

		/* Redraw new spot */
		everyone_lite_spot(Depth, p_ptr->py, p_ptr->px);

		/* Redraw old spot */
		everyone_lite_spot(Depth, oy, ox);

		/* Check for new panel (redraw map) */
		verify_panel(Ind);

		/* Update stuff */
		p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

		/* Update the monsters */
		p_ptr->update |= (PU_DISTANCE);

		/* Window stuff */
		p_ptr->window |= (PW_OVERHEAD);

		/* Hack -- quickly update the view, to reduce perceived lag */
		
		redraw_stuff(Ind);
		window_stuff(Ind);

		/* Spontaneous Searching */
		if ((p_ptr->skill_fos >= 50) ||
		    (0 == rand_int(50 - p_ptr->skill_fos)))
		{
			search(Ind);
		}

		/* Continuous Searching */
		if (p_ptr->searching)
		{
			search(Ind);
		}

		/* Handle "objects" */
		if (c_ptr->o_idx) carry(Ind, do_pickup, 0);
		else Send_floor(Ind, 0);

		/* Handle "store doors" */
		if ((!p_ptr->ghost) &&
		    (c_ptr->feat >= FEAT_SHOP_HEAD) &&
		    (c_ptr->feat <= FEAT_SHOP_TAIL))
		{
			/* Disturb */
			disturb(Ind, 0, 0);

			/* Hack -- Enter store */
			command_new = '_';
			do_cmd_store(Ind);
		}

		/* Handle resurrection */
		else if (p_ptr->ghost && c_ptr->feat == FEAT_SHOP_HEAD + 3)
		{
			/* Resurrect him */
			resurrect_player(Ind);
		}

		/* Discover invisible traps */
		else if (c_ptr->feat == FEAT_INVIS)
		{
			/* Disturb */
			disturb(Ind, 0, 0);

			/* Message */
			msg_print(Ind, "You found a trap!");

			/* Pick a trap */
			pick_trap(p_ptr->dun_depth, p_ptr->py, p_ptr->px);

			/* Hit the trap */
			hit_trap(Ind);
		}

		/* Set off an visible trap */
		else if ((c_ptr->feat >= FEAT_TRAP_HEAD) &&
		         (c_ptr->feat <= FEAT_TRAP_TAIL))
		{
			/* Disturb */
			disturb(Ind, 0, 0);

			/* Hit the trap */
			hit_trap(Ind);
		}

		/* Mega-hack -- if we are the dungeon master, and our movement hook
		 * is set, call it.  This is used to make things like building walls
		 * and summoning monster armies easier.
		 */

#if 0
		if ((!strcmp(p_ptr->name,cfg_dungeon_master) || player_is_king(Ind)) && p_ptr->master_move_hook)
#endif
		/* Check BEFORE setting ;) */
		if (p_ptr->master_move_hook)
			p_ptr->master_move_hook(Ind, NULL);
	}
}


/*
 * Hack -- Check for a "motion blocker" (see below)
 */
int see_wall(int Ind, int dir, int y, int x)
{
	player_type *p_ptr = Players[Ind];
	int Depth = p_ptr->dun_depth;

	/* Get the new location */
	y += ddy[dir];
	x += ddx[dir];

	/* Ghosts run right through everything */
	if ((p_ptr->ghost || p_ptr->tim_wraith)) return (FALSE);

	/* Do wilderness hack, keep running from one outside level to another */
	if ( (!in_bounds(Depth, y, x)) && (Depth <= 0) ) return FALSE;

	/* Illegal grids are blank */
	if (!in_bounds2(Depth, y, x)) return (FALSE);

	/* Must actually block motion */
	if (cave_floor_bold(Depth, y, x)) return (FALSE);

	/* Must be known to the player */
	if (!(p_ptr->cave_flag[y][x] & CAVE_MARK)) return (FALSE);

	/* Default */
	return (TRUE);
}


/*
 * Hack -- Check for an "unknown corner" (see below)
 */
static int see_nothing(int dir, int Ind, int y, int x)
{
	player_type *p_ptr = Players[Ind];
	int Depth = p_ptr->dun_depth;

	/* Get the new location */
	y += ddy[dir];
	x += ddx[dir];

	/* Illegal grids are unknown */
	if (!in_bounds2(Depth, y, x)) return (TRUE);

	/* Memorized grids are known */
	if (p_ptr->cave_flag[y][x] & CAVE_MARK) return (FALSE);

	/* Non-floor grids are unknown */
	if (!cave_floor_bold(Depth, y, x)) return (TRUE);

	/* Viewable grids are known */
	if (player_can_see_bold(Ind, y, x)) return (FALSE);

	/* Default */
	return (TRUE);
}





/*
 * The running algorithm:			-CJS-
 *
 * In the diagrams below, the player has just arrived in the
 * grid marked as '@', and he has just come from a grid marked
 * as 'o', and he is about to enter the grid marked as 'x'.
 *
 * Of course, if the "requested" move was impossible, then you
 * will of course be blocked, and will stop.
 *
 * Overview: You keep moving until something interesting happens.
 * If you are in an enclosed space, you follow corners. This is
 * the usual corridor scheme. If you are in an open space, you go
 * straight, but stop before entering enclosed space. This is
 * analogous to reaching doorways. If you have enclosed space on
 * one side only (that is, running along side a wall) stop if
 * your wall opens out, or your open space closes in. Either case
 * corresponds to a doorway.
 *
 * What happens depends on what you can really SEE. (i.e. if you
 * have no light, then running along a dark corridor is JUST like
 * running in a dark room.) The algorithm works equally well in
 * corridors, rooms, mine tailings, earthquake rubble, etc, etc.
 *
 * These conditions are kept in static memory:
 * find_openarea	 You are in the open on at least one
 * side.
 * find_breakleft	 You have a wall on the left, and will
 * stop if it opens
 * find_breakright	 You have a wall on the right, and will
 * stop if it opens
 *
 * To initialize these conditions, we examine the grids adjacent
 * to the grid marked 'x', two on each side (marked 'L' and 'R').
 * If either one of the two grids on a given side is seen to be
 * closed, then that side is considered to be closed. If both
 * sides are closed, then it is an enclosed (corridor) run.
 *
 * LL		L
 * @x	       LxR
 * RR	       @R
 *
 * Looking at more than just the immediate squares is
 * significant. Consider the following case. A run along the
 * corridor will stop just before entering the center point,
 * because a choice is clearly established. Running in any of
 * three available directions will be defined as a corridor run.
 * Note that a minor hack is inserted to make the angled corridor
 * entry (with one side blocked near and the other side blocked
 * further away from the runner) work correctly. The runner moves
 * diagonally, but then saves the previous direction as being
 * straight into the gap. Otherwise, the tail end of the other
 * entry would be perceived as an alternative on the next move.
 *
 * #.#
 * ##.##
 * .@x..
 * ##.##
 * #.#
 *
 * Likewise, a run along a wall, and then into a doorway (two
 * runs) will work correctly. A single run rightwards from @ will
 * stop at 1. Another run right and down will enter the corridor
 * and make the corner, stopping at the 2.
 *
 * #@x	  1
 * ########### ######
 * 2	    #
 * #############
 * #
 *
 * After any move, the function area_affect is called to
 * determine the new surroundings, and the direction of
 * subsequent moves. It examines the current player location
 * (at which the runner has just arrived) and the previous
 * direction (from which the runner is considered to have come).
 *
 * Moving one square in some direction places you adjacent to
 * three or five new squares (for straight and diagonal moves
 * respectively) to which you were not previously adjacent,
 * marked as '!' in the diagrams below.
 *
 * ...!	  ...
 * .o@!	  .o.!
 * ...!	  ..@!
 * !!!
 *
 * You STOP if any of the new squares are interesting in any way:
 * for example, if they contain visible monsters or treasure.
 *
 * You STOP if any of the newly adjacent squares seem to be open,
 * and you are also looking for a break on that side. (that is,
 * find_openarea AND find_break).
 *
 * You STOP if any of the newly adjacent squares do NOT seem to be
 * open and you are in an open area, and that side was previously
 * entirely open.
 *
 * Corners: If you are not in the open (i.e. you are in a corridor)
 * and there is only one way to go in the new squares, then turn in
 * that direction. If there are more than two new ways to go, STOP.
 * If there are two ways to go, and those ways are separated by a
 * square which does not seem to be open, then STOP.
 *
 * Otherwise, we have a potential corner. There are two new open
 * squares, which are also adjacent. One of the new squares is
 * diagonally located, the other is straight on (as in the diagram).
 * We consider two more squares further out (marked below as ?).
 *
 * We assign "option" to the straight-on grid, and "option2" to the
 * diagonal grid, and "check_dir" to the grid marked 's'.
 *
 * .s
 * @x?
 * #?
 *
 * If they are both seen to be closed, then it is seen that no
 * benefit is gained from moving straight. It is a known corner.
 * To cut the corner, go diagonally, otherwise go straight, but
 * pretend you stepped diagonally into that next location for a
 * full view next time. Conversely, if one of the ? squares is
 * not seen to be closed, then there is a potential choice. We check
 * to see whether it is a potential corner or an intersection/room entrance.
 * If the square two spaces straight ahead, and the space marked with 's'
 * are both blank, then it is a potential corner and enter if find_examine
 * is set, otherwise must stop because it is not a corner.
 */




/*
 * Hack -- allow quick "cycling" through the legal directions
 */
static byte cycle[] =
{ 1, 2, 3, 6, 9, 8, 7, 4, 1, 2, 3, 6, 9, 8, 7, 4, 1 };

/*
 * Hack -- map each direction into the "middle" of the "cycle[]" array
 */
static byte chome[] =
{ 0, 8, 9, 10, 7, 0, 11, 6, 5, 4 };

/*
 * The direction we are running
 */
/*static byte find_current;*/

/*
 * The direction we came from
 */
/*static byte find_prevdir;*/

/*
 * We are looking for open area
 */
/*static bool find_openarea;*/

/*
 * We are looking for a break
 */
/*static bool find_breakright;
static bool find_breakleft;*/



/*
 * Initialize the running algorithm for a new direction.
 *
 * Diagonal Corridor -- allow diaginal entry into corridors.
 *
 * Blunt Corridor -- If there is a wall two spaces ahead and
 * we seem to be in a corridor, then force a turn into the side
 * corridor, must be moving straight into a corridor here. ???
 *
 * Diagonal Corridor    Blunt Corridor (?)
 *       # #                  #
 *       #x#                 @x#
 *       @p.                  p
 */
static void run_init(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];

	int		row, col, deepleft, deepright;
	int		i, shortleft, shortright;


	/* Save the direction */
	p_ptr->find_current = dir;

	/* Assume running straight */
	p_ptr->find_prevdir = dir;

	/* Assume looking for open area */	
	p_ptr->find_openarea = TRUE;

	/* Assume not looking for breaks */
	p_ptr->find_breakright = p_ptr->find_breakleft = FALSE;

	/* Assume no nearby walls */
	deepleft = deepright = FALSE;
	shortright = shortleft = FALSE;

	/* Find the destination grid */
	row = p_ptr->py + ddy[dir];
	col = p_ptr->px + ddx[dir];

	/* Extract cycle index */
	i = chome[dir];

	/* Check for walls */
	/* When in the town/wilderness, don't break left/right. -APD- */
	if (see_wall(Ind, cycle[i+1], p_ptr->py, p_ptr->px))
	{
		/* if in the dungeon */
		if (p_ptr->dun_depth > 0)
		{
			p_ptr->find_breakleft = TRUE;
			shortleft = TRUE;
		}
	}
	else if (see_wall(Ind, cycle[i+1], row, col))
	{
		/* if in the dungeon */
		if (p_ptr->dun_depth > 0)
		{
			p_ptr->find_breakleft = TRUE;
			deepleft = TRUE;
		}
	}

	/* Check for walls */	
	if (see_wall(Ind, cycle[i-1], p_ptr->py, p_ptr->px))
	{
		/* if in the dungeon */
		if (p_ptr->dun_depth > 0)
		{
			p_ptr->find_breakright = TRUE;
			shortright = TRUE;
		}
	}
	else if (see_wall(Ind, cycle[i-1], row, col))
	{
		/* if in the dungeon */
		if (p_ptr->dun_depth > 0)
		{
			p_ptr->find_breakright = TRUE;
			deepright = TRUE;
		}
	}

	if (p_ptr->find_breakleft && p_ptr->find_breakright)
	{
		/* Not looking for open area */
		/* In the town/wilderness, always in an open area */
		if (p_ptr->dun_depth > 0) 
			p_ptr->find_openarea = FALSE;	

		/* Hack -- allow angled corridor entry */
		if (dir & 0x01)
		{
			if (deepleft && !deepright)
			{
				p_ptr->find_prevdir = cycle[i - 1];
			}
			else if (deepright && !deepleft)
			{
				p_ptr->find_prevdir = cycle[i + 1];
			}
		}

		/* Hack -- allow blunt corridor entry */
		else if (see_wall(Ind, cycle[i], row, col))
		{
			if (shortleft && !shortright)
			{
				p_ptr->find_prevdir = cycle[i - 2];
			}
			else if (shortright && !shortleft)
			{
				p_ptr->find_prevdir = cycle[i + 2];
			}
		}
	}
}


/*
 * Update the current "run" path
 *
 * Return TRUE if the running should be stopped
 */
static bool run_test(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int Depth = p_ptr->dun_depth;

	int			prev_dir, new_dir, check_dir = 0;

	int			row, col;
	int			i, max, inv;
	int			option, option2;

	cave_type		*c_ptr;
	byte			*w_ptr;


	/* XXX -- Ghosts never stop running */
	if (p_ptr->ghost || p_ptr->tim_wraith) return (FALSE);

	/* No options yet */
	option = 0;
	option2 = 0;

	/* Where we came from */
	prev_dir = p_ptr->find_prevdir;


	/* Range of newly adjacent grids */
	max = (prev_dir & 0x01) + 1;


	/* Look at every newly adjacent square. */
	for (i = -max; i <= max; i++)
	{
		new_dir = cycle[chome[prev_dir] + i];

		row = p_ptr->py + ddy[new_dir];
		col = p_ptr->px + ddx[new_dir];

		c_ptr = &cave[Depth][row][col];
		w_ptr = &p_ptr->cave_flag[row][col];


		/* Visible monsters abort running */
		if (c_ptr->m_idx > 0)
		{
			/* Visible monster */
			if (p_ptr->mon_vis[c_ptr->m_idx] && !(m_list[c_ptr->m_idx].special)) return (TRUE);
		}

		/* Visible objects abort running */
		if (c_ptr->o_idx)
		{
			/* Visible object */
			if (p_ptr->obj_vis[c_ptr->o_idx]) return (TRUE);
		}

		/* Hack -- always stop in water */
		if (c_ptr->feat == FEAT_WATER) return TRUE;

		/* Assume unknown */
		inv = TRUE;

		/* Check memorized grids */
		if (*w_ptr & CAVE_MARK)
		{
			bool notice = TRUE;

			/* Examine the terrain */
			switch (c_ptr->feat)
			{
				/* Floors */
			        case FEAT_FLOOR:

				/* Invis traps */
				case FEAT_INVIS:

				/* Secret doors */
				case FEAT_SECRET:

				/* Normal veins */
				case FEAT_MAGMA:
				case FEAT_QUARTZ:

				/* Hidden treasure */
				case FEAT_MAGMA_H:
				case FEAT_QUARTZ_H:

				/* Grass, trees, and dirt */
				case FEAT_GRASS:
				case FEAT_TREE:
				case FEAT_DIRT:

				/* Walls */
				case FEAT_WALL_EXTRA:
				case FEAT_WALL_INNER:
				case FEAT_WALL_OUTER:
				case FEAT_WALL_SOLID:
				case FEAT_PERM_EXTRA:
				case FEAT_PERM_INNER:
				case FEAT_PERM_OUTER:
				case FEAT_PERM_SOLID:
				case FEAT_PERM_CLEAR:
				{
					/* Ignore */
					notice = FALSE;

					/* Done */
					break;
				}

				/* Open doors */
				case FEAT_OPEN:
				case FEAT_BROKEN:
				{
					/* Option -- ignore */
					if (p_ptr->find_ignore_doors) notice = FALSE;

					/* Done */
					break;
				}

				/* Stairs */
				case FEAT_LESS:
				case FEAT_MORE:
				{
					/* Option -- ignore */
					if (p_ptr->find_ignore_stairs) notice = FALSE;

					/* Done */
					break;
				}
			}

			/* Interesting feature */
			if (notice) return (TRUE);

			/* The grid is "visible" */
			inv = FALSE;
		}

		/* Analyze unknown grids and floors */
		/* wilderness hack to run from one level to the next */
		if (inv || cave_floor_bold(Depth, row, col) || ((!in_bounds(Depth, row, col)) && (Depth <= 0)) )
		{
			/* Looking for open area */
			if (p_ptr->find_openarea)
			{
				/* Nothing */
			}

			/* The first new direction. */
			else if (!option)
			{
				option = new_dir;
			}

			/* Three new directions. Stop running. */
			else if (option2)
			{
				return (TRUE);
			}

			/* Two non-adjacent new directions.  Stop running. */
			else if (option != cycle[chome[prev_dir] + i - 1])
			{
				return (TRUE);
			}

			/* Two new (adjacent) directions (case 1) */
			else if (new_dir & 0x01)
			{
				check_dir = cycle[chome[prev_dir] + i - 2];
				option2 = new_dir;
			}

			/* Two new (adjacent) directions (case 2) */
			else
			{
				check_dir = cycle[chome[prev_dir] + i + 1];
				option2 = option;
				option = new_dir;
			}
		}

		/* Obstacle, while looking for open area */
		/* When in the town/wilderness, don't break left/right. */
		else
		{
			if (p_ptr->find_openarea)
			{
				if (i < 0)
				{
					/* Break to the right */
					if (p_ptr->dun_depth > 0)
						p_ptr->find_breakright = (TRUE);
				}

				else if (i > 0)
				{
					/* Break to the left */
					if (p_ptr->dun_depth > 0)
						p_ptr->find_breakleft = (TRUE);
				}
			}
		}
	}


	/* Looking for open area */
	if (p_ptr->find_openarea)
	{
		/* Hack -- look again */
		for (i = -max; i < 0; i++)
		{
			new_dir = cycle[chome[prev_dir] + i];

			row = p_ptr->py + ddy[new_dir];
			col = p_ptr->px + ddx[new_dir];

			/* Unknown grid or floor */
			if (!(p_ptr->cave_flag[row][col] & CAVE_MARK) || cave_floor_bold(Depth, row, col))
			{
				/* Looking to break right */
				if (p_ptr->find_breakright)
				{									
					return (TRUE);
				}
			}

			/* Obstacle */
			else
			{
				/* Looking to break left */
				if (p_ptr->find_breakleft)
				{					
					return (TRUE);
				}
			}
		}

		/* Hack -- look again */
		for (i = max; i > 0; i--)
		{
			new_dir = cycle[chome[prev_dir] + i];

			row = p_ptr->py + ddy[new_dir];
			col = p_ptr->px + ddx[new_dir];

			/* Unknown grid or floor */
			if (!(p_ptr->cave_flag[row][col] & CAVE_MARK) || cave_floor_bold(Depth, row, col))
			{
				/* Looking to break left */
				if (p_ptr->find_breakleft)
				{				
					return (TRUE);
				}
			}

			/* Obstacle */
			else
			{
				/* Looking to break right */
				if (p_ptr->find_breakright)
				{
					return (TRUE);
				}
			}
		}
	}


	/* Not looking for open area */
	else
	{
		/* No options */
		if (!option)
		{
			return (TRUE);
		}

		/* One option */
		else if (!option2)
		{
			/* Primary option */
			p_ptr->find_current = option;

			/* No other options */
			p_ptr->find_prevdir = option;
		}

		/* Two options, examining corners */
		else if (p_ptr->find_examine && !p_ptr->find_cut)
		{
			/* Primary option */
			p_ptr->find_current = option;

			/* Hack -- allow curving */
			p_ptr->find_prevdir = option2;
		}

		/* Two options, pick one */
		else
		{
			/* Get next location */
			row = p_ptr->py + ddy[option];
			col = p_ptr->px + ddx[option];

			/* Don't see that it is closed off. */
			/* This could be a potential corner or an intersection. */
			if (!see_wall(Ind, option, row, col) ||
			    !see_wall(Ind, check_dir, row, col))
			{
				/* Can not see anything ahead and in the direction we */
				/* are turning, assume that it is a potential corner. */
				if (p_ptr->find_examine &&
				    see_nothing(option, Ind, row, col) &&
				    see_nothing(option2, Ind, row, col))
				{
					p_ptr->find_current = option;
					p_ptr->find_prevdir = option2;
				}

				/* STOP: we are next to an intersection or a room */
				else
				{
					return (TRUE);
				}
			}

			/* This corner is seen to be enclosed; we cut the corner. */
			else if (p_ptr->find_cut)
			{
				p_ptr->find_current = option2;
				p_ptr->find_prevdir = option2;
			}

			/* This corner is seen to be enclosed, and we */
			/* deliberately go the long way. */
			else
			{
				p_ptr->find_current = option;
				p_ptr->find_prevdir = option2;
			}
		}
	}


	/* About to hit a known wall, stop */
	if (see_wall(Ind, p_ptr->find_current, p_ptr->py, p_ptr->px))
	{
		return (TRUE);
	}


	/* Failure */
	return (FALSE);
}



/*
 * Take one step along the current "run" path
 */
void run_step(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];
	cave_type *c_ptr;

	/* Check for just changed level */
	if (p_ptr->new_level_flag) return;

	/* Start running */
	if (dir)
	{
		/* Running into walls and doors is now checked in do_cmd_run.*/
		#if 0
		/* Hack -- do not start silly run */
		if (see_wall(Ind, dir, p_ptr->py, p_ptr->px))
		{
			/* If we are trying to run into a door and bump_open is enabled,
			 * try to open the door
			 */
			if (cfg_door_bump_open)
			{
				/* Get requested grid */
				c_ptr = &cave[p_ptr->dun_depth][p_ptr->py+ddy[dir]][p_ptr->px+ddx[dir]];

				/* If a door, open it */
				if (((c_ptr->feat >= FEAT_DOOR_HEAD) && 
				      (c_ptr->feat <= FEAT_DOOR_TAIL)) ||
				    ((c_ptr->feat >= FEAT_HOME_HEAD) &&
				      (c_ptr->feat <= FEAT_HOME_TAIL))) 
					{
						do_cmd_open(Ind, dir);
						return;
					}
			}

			/* Message */
			msg_print(Ind, "You cannot run in that direction.");

			/* Disturb */
			disturb(Ind, 0, 0);

			/* Done */
			return;
		}
		#endif

		/* Calculate torch radius */
		p_ptr->update |= (PU_TORCH);

		/* Initialize */
		run_init(Ind, dir);
		/* check if we have enough energy to move */
		if (p_ptr->energy < (level_speed(p_ptr->dun_depth)/5))
			return;
	}

	/* Keep running */
	else
	{
		/* Update run */
		if (run_test(Ind))
		{
			/* Disturb */
			disturb(Ind, 0, 0);

			/* Done */
			return;
		}
	}

	/* Decrease the run counter */
	if (--(p_ptr->running) <= 0) return;

	/* Move the player, using the "pickup" flag */
	move_player(Ind, p_ptr->find_current, p_ptr->always_pickup);
}


