/* Purpose: Angband game engine */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define SERVER

#include "angband.h"
#include "externs.h"



/*
 * Return a "feeling" (or NULL) about an item.  Method 1 (Heavy).
 */
static cptr value_check_aux1(object_type *o_ptr)
{
	/* Artifacts */
	if (artifact_p(o_ptr))
	{
		/* Cursed/Broken */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) return "terrible";

		/* Normal */
		return "special";
	}

	/* Ego-Items */
	if (ego_item_p(o_ptr))
	{
		/* Cursed/Broken */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) return "worthless";

		/* Normal */
		return "excellent";
	}

	/* Cursed items */
	if (cursed_p(o_ptr)) return "cursed";

	/* Broken items */
	if (broken_p(o_ptr)) return "broken";

	/* Good "armor" bonus */
	if (o_ptr->to_a > 0) return "good";

	/* Good "weapon" bonus */
	if (o_ptr->to_h + o_ptr->to_d > 0) return "good";

	/* Default to "average" */
	return "average";
}


/*
 * Return a "feeling" (or NULL) about an item.  Method 2 (Light).
 */
static cptr value_check_aux2(object_type *o_ptr)
{
	/* Cursed items (all of them) */
	if (cursed_p(o_ptr)) return "cursed";

	/* Broken items (all of them) */
	if (broken_p(o_ptr)) return "broken";

	/* Artifacts -- except cursed/broken ones */
	if (artifact_p(o_ptr)) return "good";

	/* Ego-Items -- except cursed/broken ones */
	if (ego_item_p(o_ptr)) return "good";

	/* Good armor bonus */
	if (o_ptr->to_a > 0) return "good";

	/* Good weapon bonuses */
	if (o_ptr->to_h + o_ptr->to_d > 0) return "good";

	/* No feeling */
	return (NULL);
}




/*
 * Sense the inventory
 *
 *   Class 0 = Warrior --> fast and heavy
 *   Class 1 = Mage    --> slow and light
 *   Class 2 = Priest  --> fast but light
 *   Class 3 = Rogue   --> okay and heavy
 *   Class 4 = Ranger  --> okay and heavy
 *   Class 5 = Paladin --> slow but heavy
 */
static void sense_inventory(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int		i;

	int		plev = p_ptr->lev;

	bool	heavy = FALSE;

	cptr	feel;

	object_type *o_ptr;

	char o_name[160];


	/*** Check for "sensing" ***/

	/* No sensing when confused */
	if (p_ptr->confused) return;

	/* Analyze the class */
	switch (p_ptr->pclass)
	{
		case CLASS_UNBELIEVER:
		{
			/* Good sensing */
			if (0 != rand_int(8000L / (plev * plev + 40))) return;

			/* Heavy sensing */
			heavy = TRUE;

			/* Done */
			break;
		}
		
		case CLASS_WARRIOR:
		case CLASS_ARCHER:
		{
			/* Good sensing */
			if (0 != rand_int(9000L / (plev * plev + 40))) return;

			/* Heavy sensing */
			heavy = TRUE;

			/* Done */
			break;
		}

		case CLASS_MAGE:
		case CLASS_SORCERER:
		{
			/* Very bad (light) sensing */
			if (0 != rand_int(240000L / (plev + 5))) return;

			/* Done */
			break;
		}

		case CLASS_PRIEST:
		{
			/* Good (light) sensing */
			if (0 != rand_int(10000L / (plev * plev + 40))) return;

			/* Done */
			break;
		}

		case CLASS_ROGUE:
		case CLASS_MIMIC:
		{
			/* Okay sensing */
			if (0 != rand_int(20000L / (plev * plev + 40))) return;

			/* Heavy sensing */
			heavy = TRUE;

			/* Done */
			break;
		}

		case CLASS_RANGER:
		{
			if (0 != rand_int(20000L / (plev * plev + 40))) return;

			/* Heavy sensing */
			heavy = TRUE;

			/* Done */
			break;
		}

		case CLASS_PALADIN:
		{
			/* Bad sensing */
			if (0 != rand_int(80000L / (plev * plev + 40))) return;

			/* Heavy sensing */
			heavy = TRUE;

			/* Done */
			break;
		}

		case CLASS_TELEPATH:
		case CLASS_MONK:
		{
			/* Okay sensing */
			if (0 != rand_int(20000L / (plev * plev + 40))) return;

			/* Heavy sensing */
			heavy = TRUE;

			/* Done */
			break;
		}
	}


	/*** Sense everything ***/

	/* Check everything */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		bool okay = FALSE;

		o_ptr = &p_ptr->inventory[i];

		/* Skip empty slots */
		if (!o_ptr->k_idx) continue;

		/* Valid "tval" codes */
		switch (o_ptr->tval)
		{
			case TV_SHOT:
			case TV_ARROW:
			case TV_BOLT:
			case TV_BOW:
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
			{
				okay = TRUE;
				break;
			}
		}

		/* Skip non-sense machines */
		if (!okay) continue;

		/* We know about it already, do not tell us again */
		if (o_ptr->ident & ID_SENSE) continue;

		/* It is fully known, no information needed */
		if (object_known_p(Ind, o_ptr)) continue;

		/* Occasional failure on inventory items */
		if ((i < INVEN_WIELD) && (0 != rand_int(5))) continue;

		/* Check for a feeling */
		feel = (heavy ? value_check_aux1(o_ptr) : value_check_aux2(o_ptr));

		/* Skip non-feelings */
		if (!feel) continue;

		/* Stop everything */
		if (p_ptr->disturb_minor) disturb(Ind, 0, 0);

		/* Get an object description */
		object_desc(Ind, o_name, o_ptr, FALSE, 0);

		/* Message (equipment) */
		if (i >= INVEN_WIELD)
		{
			msg_format(Ind, "You feel the %s (%c) you are %s %s %s...",
			           o_name, index_to_label(i), describe_use(Ind, i),
			           ((o_ptr->number == 1) ? "is" : "are"), feel);
		}

		/* Message (inventory) */
		else
		{
			msg_format(Ind, "You feel the %s (%c) in your pack %s %s...",
			           o_name, index_to_label(i),
			           ((o_ptr->number == 1) ? "is" : "are"), feel);
		}

		/* We have "felt" it */
		o_ptr->ident |= (ID_SENSE);

		/* Inscribe it textually */
		if (!o_ptr->note) o_ptr->note = quark_add(feel);

		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);
	}
}



/*
 * Regenerate hit points				-RAK-
 */
static void regenhp(int Ind, int percent)
{
	player_type *p_ptr = Players[Ind];

	s32b        new_chp, new_chp_frac;
	int                   old_chp;

	/* Save the old hitpoints */
	old_chp = p_ptr->chp;

	/* Extract the new hitpoints */
	new_chp = ((long)p_ptr->mhp) * percent + PY_REGEN_HPBASE;
	/* Apply the healing */
	hp_player_quiet(Ind, new_chp >> 16);
	//p_ptr->chp += new_chp >> 16;   /* div 65536 */

	/* check for overflow -- this is probably unneccecary */
	if ((p_ptr->chp < 0) && (old_chp > 0)) p_ptr->chp = MAX_SHORT;

	/* handle fractional hit point healing */
	new_chp_frac = (new_chp & 0xFFFF) + p_ptr->chp_frac;	/* mod 65536 */
	if (new_chp_frac >= 0x10000L)
	{
		hp_player_quiet(Ind, 1);
		p_ptr->chp_frac = new_chp_frac - 0x10000L;
	}
	else
	{
		p_ptr->chp_frac = new_chp_frac;
	}
}


/*
 * Regenerate mana points				-RAK-
 */
static void regenmana(int Ind, int percent)
{
	player_type *p_ptr = Players[Ind];

	s32b        new_mana, new_mana_frac;
	int                   old_csp;

	old_csp = p_ptr->csp;
	new_mana = ((long)p_ptr->msp) * percent + PY_REGEN_MNBASE;
	p_ptr->csp += new_mana >> 16;	/* div 65536 */
	/* check for overflow */
	if ((p_ptr->csp < 0) && (old_csp > 0))
	{
		p_ptr->csp = MAX_SHORT;
	}
	new_mana_frac = (new_mana & 0xFFFF) + p_ptr->csp_frac;	/* mod 65536 */
	if (new_mana_frac >= 0x10000L)
	{
		p_ptr->csp_frac = new_mana_frac - 0x10000L;
		p_ptr->csp++;
	}
	else
	{
		p_ptr->csp_frac = new_mana_frac;
	}

	/* Must set frac to zero even if equal */
	if (p_ptr->csp >= p_ptr->msp)
	{
		p_ptr->csp = p_ptr->msp;
		p_ptr->csp_frac = 0;
	}

	/* Redraw mana */
	if (old_csp != p_ptr->csp)
	{
		/* Redraw */
		p_ptr->redraw |= (PR_MANA);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);
	}
}






/*
 * Regenerate the monsters (once per 100 game turns)
 *
 * XXX XXX XXX Should probably be done during monster turns.
 */
/* Note that since this is done in real time, monsters will regenerate
 * faster in game time the deeper they are in the dungeon.
 */
static void regen_monsters(void)
{
	int i, frac;

	/* Regenerate everyone */
	for (i = 1; i < m_max; i++)
	{
		/* Check the i'th monster */
		monster_type *m_ptr = &m_list[i];
                monster_race *r_ptr = race_inf(m_ptr);

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Allow regeneration (if needed) */
		if (m_ptr->hp < m_ptr->maxhp)
		{
			/* Hack -- Base regeneration */
			frac = m_ptr->maxhp / 100;

			/* Hack -- Minimal regeneration rate */
			if (!frac) frac = 1;

			/* Hack -- Some monsters regenerate quickly */
			if (r_ptr->flags2 & RF2_REGENERATE) frac *= 2;

			/* Hack -- Regenerate */
			m_ptr->hp += frac;

			/* Do not over-regenerate */
			if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

			/* Update health bars */
			update_health(i);
		}
	}
}



/*
 * Handle certain things once every 50 game turns
 */

static void process_world(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int		x, y, i, j;

	int		regen_amount, NumPlayers_old=NumPlayers;

	cave_type		*c_ptr;
	byte			*w_ptr;

	object_type		*o_ptr;


	/* Every 50 game turns */
	if (turn % 50) return;


	/*** Check the Time and Load ***/
	/* The server will never quit --KLJ-- */

	/*** Handle the "town" (stores and sunshine) ***/

	/* While in town or wilderness */
#ifdef NEW_DUNGEON
	if (p_ptr->wpos.wz==0)
#else
	if (p_ptr->dun_depth <= 0)
#endif
	{
		/* Hack -- Daybreak/Nighfall in town */
		if (!(turn % ((10L * TOWN_DAWN) / 2)))
		{
			bool dawn;
#ifdef NEW_DUNGEON
			struct worldpos *wpos=&p_ptr->wpos;
			cave_type **zcave;
			if(!(zcave=getcave(wpos))) return;
#else
			int Depth = p_ptr->dun_depth;
#endif

			/* Check for dawn */
			dawn = (!(turn % (10L * TOWN_DAWN)));

			/* Day breaks */
			if (dawn)
			{
				/* Message */
				msg_print(Ind, "The sun has risen.");
	
				/* Hack -- Scan the level */
				for (y = 0; y < MAX_HGT; y++)
				{
					for (x = 0; x < MAX_WID; x++)
					{
						/* Get the cave grid */
#ifdef NEW_DUNGEON
						c_ptr = &zcave[y][x];
#else
						c_ptr = &cave[Depth][y][x];
#endif
						w_ptr = &p_ptr->cave_flag[y][x];

						/* Assume lit */
						c_ptr->info |= CAVE_GLOW;

						/* Hack -- Memorize lit grids if allowed */
#ifdef NEW_DUNGEON
						if ((istown(wpos)) && (p_ptr->view_perma_grids)) *w_ptr |= CAVE_MARK;
#else
						if ((!Depth) && (p_ptr->view_perma_grids)) *w_ptr |= CAVE_MARK;
#endif

						/* Hack -- Notice spot */
						note_spot(Ind, y, x);						
					}			
				}
			}

			/* Night falls */
			else
			{
				/* Message  */
				msg_print(Ind, "The sun has fallen.");

				 /* Hack -- Scan the level */
				for (y = 0; y < MAX_HGT; y++)
				{					
					for (x = 0; x < MAX_WID; x++)
					{
						/*  Get the cave grid */
#ifdef NEW_DUNGEON
						c_ptr = &zcave[y][x];
#else
						c_ptr = &cave[Depth][y][x];
#endif
						w_ptr = &p_ptr->cave_flag[y][x];

						/*  Darken "boring" features */
						if (c_ptr->feat <= FEAT_INVIS && !(c_ptr->info & CAVE_ROOM))
						{
							  /* Forget the grid */ 
							c_ptr->info &= ~CAVE_GLOW;
							*w_ptr &= ~CAVE_MARK;

							  /* Hack -- Notice spot */
							note_spot(Ind, y, x);
						}						
					}
					
				}  				
			}

			/* Update the monsters */
			p_ptr->update |= (PU_MONSTERS);

			/* Redraw map */
			p_ptr->redraw |= (PR_MAP);

			/* Window stuff */
			p_ptr->window |= (PW_OVERHEAD);
		}
	}

	/* While in the dungeon */
	else
	{
		/*** Update the Stores ***/
		/*  Don't do this for each player.  In fact, this might be */
		/*  taken out entirely for now --KLJ-- */
	}


	/*** Process the monsters ***/
	/* Note : since monsters are added at a constant rate in real time,
	 * this corresponds in game time to them appearing at faster rates
	 * deeper in the dungeon.
	 */

	/* Check for creature generation */
#ifdef NEW_DUNGEON
	if ((rand_int(MAX_M_ALLOC_CHANCE) == 0) && ((!istown(&p_ptr->wpos)) || (rand_int(10)<5)))
#else
	if ((rand_int(MAX_M_ALLOC_CHANCE) == 0) && ((p_ptr->dun_depth !=0) || (rand_int(10)<5)))
#endif
	{
		/* Set the monster generation depth */
#ifdef NEW_DUNGEON
		monster_level = getlevel(&p_ptr->wpos);
		if (p_ptr->wpos.wz)
			(void)alloc_monster(&p_ptr->wpos, MAX_SIGHT + 5, FALSE);
		else wild_add_monster(&p_ptr->wpos);
#else
		if (p_ptr->dun_depth >= 0)		
			monster_level = p_ptr->dun_depth;
		else monster_level = 2 + (wild_info[p_ptr->dun_depth].radius / 2);
		if (p_ptr->dun_depth >= 0)
			(void)alloc_monster(p_ptr->dun_depth, MAX_SIGHT + 5, FALSE);
		/* Make a new monster */
		else wild_add_monster(p_ptr->dun_depth);
#endif
	}

	/* Every 1500 turns, warn about any Black Breath not gotten from an equipped
	 * object, and stop any resting. -LM-
	 */
	/* Probably better done in process_player_end?	- Jir - */
	if (!(turn % 3000) && (p_ptr->black_breath))
	{
		msg_print(Ind, "\377WThe Black Breath saps your soul!");

		/* alert to the neighbors also */
		msg_format_near(Ind, "\377WA dark aura seems to surround %s!", p_ptr->name);

		disturb(Ind, 0, 0);
	}
#if 0	// no BB-causing items for now?
	if (!(turn % 3000) && (p_ptr->black_breath))
	{
		u32b f1, f2, f3, f4, f5;

		bool be_silent = FALSE;

		/* check all equipment for the Black Breath flag. */
		for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
		{
			o_ptr = &inventory[i];

			/* Skip non-objects */
			if (!o_ptr->k_idx) continue;

			/* Extract the item flags */
			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

			/* No messages if object has the flag, to avoid annoyance. */
			if (f4 & (TR4_BLACK_BREATH)) be_silent = TRUE;

		}
		/* If we are allowed to speak, warn and disturb. */

		if (!be_silent)
		{
			cmsg_print(TERM_L_DARK, "The Black Breath saps your soul!");
			disturb(0, 0);
		}
	}
#endif	// 0
}



/*
 * Verify use of "wizard" mode
 */
#if 0
static bool enter_wizard_mode(void)
{
#ifdef ALLOW_WIZARD
	/* No permission */
	if (!can_be_wizard) return (FALSE);

	/* Ask first time */
	if (!(noscore & 0x0002))
	{
		/* Mention effects */
		msg_print("Wizard mode is for debugging and experimenting.");
		msg_print("The game will not be scored if you enter wizard mode.");
		msg_print(NULL);

		/* Verify request */
		if (!get_check("Are you sure you want to enter wizard mode? "))
		{
			return (FALSE);
		}

		/* Mark savefile */
		noscore |= 0x0002;
	}

	/* Success */
	return (TRUE);
#endif /* ALLOW_WIZARD */

	/* XXX XXX XXX Return FALSE if wizard mode is compiled out --KLJ-- */
	return (FALSE);
}
#endif


#ifdef ALLOW_WIZARD

/*
 * Verify use of "debug" commands
 */
static bool enter_debug_mode(void)
{
	/* No permission */
	if (!can_be_wizard) return (FALSE);

	/* Ask first time */
	if (!(noscore & 0x0008))
	{
		/* Mention effects */
		msg_print("The debug commands are for debugging and experimenting.");
		msg_print("The game will not be scored if you use debug commands.");
		msg_print(NULL);

		/* Verify request */
		if (!get_check("Are you sure you want to use debug commands? "))
		{
			return (FALSE);
		}

		/* Mark savefile */
		noscore |= 0x0008;
	}

	/* Success */
	return (TRUE);
}

/*
 * Hack -- Declare the Wizard Routines
 */
extern int do_cmd_wizard(void);

#endif


#ifdef ALLOW_BORG

/*
 * Verify use of "borg" commands
 */
static bool enter_borg_mode(void)
{
	/* No permission */
	if (!can_be_wizard) return (FALSE);

	/* Ask first time */
	if (!(noscore & 0x0010))
	{
		/* Mention effects */
		msg_print("The borg commands are for debugging and experimenting.");
		msg_print("The game will not be scored if you use borg commands.");
		msg_print(NULL);

		/* Verify request */
		if (!get_check("Are you sure you want to use borg commands? "))
		{
			return (FALSE);
		}

		/* Mark savefile */
		noscore |= 0x0010;
	}

	/* Success */
	return (TRUE);
}

/*
 * Hack -- Declare the Ben Borg
 */
extern void do_cmd_borg(void);

#endif



/*
 * Parse and execute the current command
 * Give "Warning" on illegal commands.
 *
 * XXX XXX XXX Make some "blocks"
 *
 * This all happens "automagically" by the Input() function in netserver.c
 */
#if 0
static void process_command(void)
{
}
#endif

/*
 * Handle items for auto-retaliation  - Jir -
 * use_old_target is *strongly* recommended to actually make use of it.
 */
static bool retaliate_item(int Ind, int item, cptr inscription)
{
	player_type *p_ptr = Players[Ind];
	int spell = 0;

	if (item < 0) return FALSE;

	/* 'Do nothing' inscription */
	if (*inscription == 'x') return TRUE;

	/* Fighter classes can use various items for this */
	if (is_fighter(p_ptr))
	{
#if 0
		/* item with {@O-} is used only when in danger */
		if (*inscription == '-' && p_ptr->chp > p_ptr->mhp / 2) return FALSE;
#endif

		switch (p_ptr->inventory[item].tval)
		{
			/* non-directional ones */
			case TV_SCROLL:
			{
				do_cmd_read_scroll(Ind, item);
				return TRUE;
			}

			case TV_POTION:
			{
				do_cmd_quaff_potion(Ind, item);
				return TRUE;
			}

			case TV_STAFF:
			{
				do_cmd_use_staff(Ind, item);
				return TRUE;
			}

			/* ambiguous one */
			/* basically using rod for retaliation is not so good idea :) */
			case TV_ROD:
			{
				p_ptr->current_rod = item;
				do_cmd_zap_rod(Ind, item);
				return TRUE;
			}

			case TV_WAND:
			{
				do_cmd_aim_wand(Ind, item, 5);
				return TRUE;
			}
		}
	}

	/* Accept reasonable targets:
	 * This prevents a player from getting stuck when facing a
	 * monster inside a wall.
	 */
	if (!target_able(Ind, p_ptr->target_who)) return FALSE;

	/* Spell to cast */
	if (inscription != NULL)
	{
		spell = *inscription - 96 - 1;
		if (spell < 0 || spell > 9) spell = 0;
	}

	switch (p_ptr->inventory[item].tval)
	{
		/* directional ones */
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		{
			do_cmd_fire(Ind, 5, item);
			return TRUE;
		}

		case TV_PSI_BOOK:
		{
			do_cmd_psi(Ind, item, spell);
			return TRUE;
		}

		case TV_MAGIC_BOOK:
		{
			do_cmd_cast(Ind, item, spell);
			return TRUE;
		}

		case TV_PRAYER_BOOK:
		{
			do_cmd_pray(Ind, item, spell);
			return TRUE;
		}

		case TV_SORCERY_BOOK:
		{
			do_cmd_sorc(Ind, item, spell);
			return TRUE;
		}

		/* not likely :); */
		case TV_FIGHT_BOOK:
		{
			do_cmd_fight(Ind, item, spell);
			return TRUE;
		}

		case TV_SHADOW_BOOK:
		{
			do_cmd_shad(Ind, item, spell);
			return TRUE;
		}

		case TV_HUNT_BOOK:
		{
			do_cmd_hunt(Ind, item, spell);
			return TRUE;
		}
	}

	/* If all fails, then melee */
	return FALSE;
}


/*
 * Check for nearby players or monsters and attempt to do something useful.
 *
 * This function should only be called if the player is "lagging" and helpless
 * to do anything about the situation.  This is not intended to be incredibly
 * useful, merely to prevent deaths due to extreme lag.
 */
 /* This function returns a 0 if no attack has been performed, a 1 if an attack
  * has been performed and there are still monsters around, and a 2 if an attack
  * has been performed and all of the surrounding monsters are dead.
  * (This difference between 1 and 2 isn't used anywhere... but I leave it for
  *  future use.	Jir)
  */
  /* We now intelligently try to decide which monster to autoattack.  Our current
   * algorithm is to fight first Q's, then any monster that is 20 levels higher
   * than its peers, then the most proportionatly wounded monster, then the highest
   * level monster, then the monster with the least hit points.
   */ 
/* Now a player can choose the way of auto-retaliating;		- Jir -
 * Item marked with inscription {@O} will be used automatically.
 * For spellbooks this should be {@Oa} to specify which spell to use.
 * You can also choose to 'do nothing' by inscribing it on not-suitable item.
 *
 * Fighters are allowed to use {@O-}, which is only used if his HP is 1/2 or less.
 * ({@O-} feature is commented out)
 */
static int auto_retaliate(int Ind)
{
	player_type *p_ptr = Players[Ind], *q_ptr, *p_target_ptr = NULL, *prev_p_target_ptr = NULL;
	int d, i, tx, ty, target, prev_target, item = -1;
//	char friends = 0;
	monster_type *m_ptr, *m_target_ptr = NULL, *prev_m_target_ptr = NULL;
	monster_race *r_ptr = NULL, *r_ptr2;
	unsigned char * inscription;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return(FALSE);
#endif

	if (p_ptr->new_level_flag) return 0;

	for (d = 1; d <= 9; d++)
	{
		if (d == 5) continue;

		tx = p_ptr->px + ddx[d];
		ty = p_ptr->py + ddy[d];

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
			m_ptr = &m_list[i];

			/* Paranoia -- Skip dead monsters */
			if (!m_ptr->r_idx) continue;

			/* Make sure that the player can see this monster */
			if (!p_ptr->mon_vis[i]) continue;

                if (p_ptr->id == m_ptr->owner) continue;

			/* Figure out if this is the best target so far */
			if (!m_target_ptr)
			{
				prev_m_target_ptr = m_target_ptr;
				m_target_ptr = m_ptr;
				r_ptr = race_inf(m_ptr);
				prev_target = target;
				target = i;
			}
			else
			{
				r_ptr2 = r_ptr;
				r_ptr = race_inf(m_ptr);

				/* If it is a Q, then make it our new target. */
				/* We don't handle the case of choosing between two
				 * Q's because if the player is standing next to two Q's
				 * he deserves whatever punishment he gets.
				 */
                                if (r_ptr->d_char == 'Q')
				{
					prev_m_target_ptr = m_target_ptr;
					m_target_ptr = m_ptr;
					prev_target = target;
					target = i;
				}
				/* Otherwise if it is 20 levels higher than everything
				 * else attack it.
				 */
				else if ((r_ptr->level - 20) >= r_ptr2->level)
				{
					prev_m_target_ptr = m_target_ptr;
					m_target_ptr = m_ptr;
					prev_target = target;
					target = i;
				}
				/* Otherwise if it is the most proportionatly wounded monster
				 * attack it.
				 */
				else if (m_ptr->hp * m_target_ptr->maxhp < m_target_ptr->hp * m_ptr->maxhp)
				{
					prev_m_target_ptr = m_target_ptr;
					m_target_ptr = m_ptr;
					prev_target = target;
					target = i;
				}
				/* If it is a tie attack the higher level monster */
				else if (m_ptr->hp * m_target_ptr->maxhp == m_target_ptr->hp * m_ptr->maxhp)
				{
                                        if (r_ptr->level > r_ptr2->level)
					{
						prev_m_target_ptr = m_target_ptr;
						m_target_ptr = m_ptr;
						prev_target = target;
						target = i;
					}
					/* If it is a tie attack the monster with less hit points */
                                        else if (r_ptr->level == r_ptr2->level)
					{
						if (m_ptr->hp < m_target_ptr->hp)
						{
							prev_m_target_ptr = m_target_ptr;
							m_target_ptr = m_ptr;
							prev_target = target;
							target = i;
						}
					}
				}
			}
		}
		else
		{
			i = -i;
			if (!(q_ptr = Players[i])) continue;

			/* Skip non-connected players */
			if (q_ptr->conn == NOT_CONNECTED) continue;

			/* Skip players we aren't hostile to */
			if (!check_hostile(Ind, i)) continue;

			/* Skip players we cannot see */
			if (!p_ptr->play_vis[i]) continue;

			/* Figure out if this is the best target so far */
			if (p_target_ptr)
			{
				/* If we are 15 levels over the old target, make this
				 * player our new target.
				 */
				if ((q_ptr->lev - 15) >= p_target_ptr->lev)
				{
					prev_p_target_ptr = p_target_ptr;
					p_target_ptr = q_ptr;
					prev_target = target;
					target = -i;
				}
				/* Otherwise attack this player if he is more proportionatly
				 * wounded than our old target.
				 */
				else if (q_ptr->chp * p_target_ptr->mhp < p_target_ptr->chp * q_ptr->mhp)
				{
					prev_p_target_ptr = p_target_ptr;
					p_target_ptr = q_ptr;
					prev_target = target;
					target = -i;
				}
				/* If it is a tie attack the higher level player */
				else if (q_ptr->chp * p_target_ptr->mhp == p_target_ptr->chp * q_ptr->mhp)
				{
					if (q_ptr->lev > p_target_ptr->lev)
					{
						prev_p_target_ptr = p_target_ptr;
						p_target_ptr = q_ptr;
						prev_target = target;
						target = -i;
					}
					/* If it is a tie attack the player with less hit points */
					else if (q_ptr->lev == p_target_ptr->lev)
					{
						if (q_ptr->chp < p_target_ptr->chp)
						{
							prev_p_target_ptr = p_target_ptr;
							p_target_ptr = q_ptr;
							prev_target = target;
							target = -i;
						}
					}
				}
			}
			else
			{
				prev_p_target_ptr = p_target_ptr;
				p_target_ptr = q_ptr;
				prev_target = target;
				target = -i;
			}
		}
	}
	
#if 0
	/* This code is not so efficient when many monsters are on the floor..
	 * However it can be 'recycled' for automatons in the future.	- Jir -
	 */
	/* Check each monster */
	for (i = 1; i < m_max; i++)
	{
		m_ptr = &m_list[i];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip monsters that aren't at this depth */
#ifdef NEW_DUNGEON
		if(!inarea(&p_ptr->wpos, &m_ptr->wpos)) continue;
#else
		if (p_ptr->dun_depth != m_ptr->dun_depth) continue;
#endif

		/* Make sure that the player can see this monster */
		if (!p_ptr->mon_vis[i]) continue;

                if (p_ptr->id == m_ptr->owner) continue;

		/* A monster is next to us */
		if (distance(p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx) == 1)
		{
			/* Figure out if this is the best target so far */
			if (m_target_ptr)
			{
				/* If it is a Q, then make it our new target. */
				/* We don't handle the case of choosing between two
				 * Q's because if the player is standing next to two Q's
				 * he deserves whatever punishment he gets.
				 */
                                if (race_inf(m_ptr)->d_char == 'Q')
				{
					prev_m_target_ptr = m_target_ptr;
					m_target_ptr = m_ptr;
					prev_target = target;
					target = i;
				}
				/* Otherwise if it is 20 levels higher than everything
				 * else attack it.
				 */
                                else if ((race_inf(m_ptr)->level - 20) >=
                                                R_INFO(m_target_ptr)->level)
				{
					prev_m_target_ptr = m_target_ptr;
					m_target_ptr = m_ptr;
					prev_target = target;
					target = i;
				}
				/* Otherwise if it is the most proportionatly wounded monster
				 * attack it.
				 */
				else if (m_ptr->hp * m_target_ptr->maxhp < m_target_ptr->hp * m_ptr->maxhp)
				{
					prev_m_target_ptr = m_target_ptr;
					m_target_ptr = m_ptr;
					prev_target = target;
					target = i;
				}
				/* If it is a tie attack the higher level monster */
				else if (m_ptr->hp * m_target_ptr->maxhp == m_target_ptr->hp * m_ptr->maxhp)
				{
                                        if (race_inf(m_ptr)->level > R_INFO(m_target_ptr)->level)
					{
						prev_m_target_ptr = m_target_ptr;
						m_target_ptr = m_ptr;
						prev_target = target;
						target = i;
					}
					/* If it is a tie attack the monster with less hit points */
                                        else if (race_inf(m_ptr)->level == R_INFO(m_target_ptr)->level)
					{
						if (m_ptr->hp < m_target_ptr->hp)
						{
							prev_m_target_ptr = m_target_ptr;
							m_target_ptr = m_ptr;
							prev_target = target;
							target = i;
						}
					}
				}
			}
			else
			{
				prev_m_target_ptr = m_target_ptr;
				m_target_ptr = m_ptr;
				prev_target = target;
				target = i;
			}
		}
	}

	/* Check each player */
	for (i = 1; i <= NumPlayers; i++)
	{
		q_ptr = Players[i];

		/* Skip non-connected players */
		if (q_ptr->conn == NOT_CONNECTED) continue;

		/* Skip players not at this depth */
#ifdef NEW_DUNGEON
		if(!inarea(&p_ptr->wpos, &q_ptr->wpos)) continue;
#else
		if (p_ptr->dun_depth != q_ptr->dun_depth) continue;
#endif

		/* Skip ourselves */
		if (Ind == i) continue;

		/* Skip players we aren't hostile to */
		if (!check_hostile(Ind, i)) continue;

		/* Skip players we cannot see */
		if (!p_ptr->play_vis[i]) continue;

		/* A hostile player is next to us */
		if (distance(p_ptr->py, p_ptr->px, q_ptr->py, q_ptr->px) == 1)
		{
			/* Figure out if this is the best target so far */
			if (p_target_ptr)
			{
				/* If we are 15 levels over the old target, make this
				 * player our new target.
				 */
				if ((q_ptr->lev - 15) >= p_target_ptr->lev)
				{
					prev_p_target_ptr = p_target_ptr;
					p_target_ptr = q_ptr;
					prev_target = target;
					target = -i;
				}
				/* Otherwise attack this player if he is more proportionatly
				 * wounded than our old target.
				 */
				else if (q_ptr->chp * p_target_ptr->mhp < p_target_ptr->chp * q_ptr->mhp)
				{
					prev_p_target_ptr = p_target_ptr;
					p_target_ptr = q_ptr;
					prev_target = target;
					target = -i;
				}
				/* If it is a tie attack the higher level player */
				else if (q_ptr->chp * p_target_ptr->mhp == p_target_ptr->chp * q_ptr->mhp)
				{
					if (q_ptr->lev > p_target_ptr->lev)
					{
						prev_p_target_ptr = p_target_ptr;
						p_target_ptr = q_ptr;
						prev_target = target;
						target = -i;
					}
					/* If it is a tie attack the player with less hit points */
					else if (q_ptr->lev == p_target_ptr->lev)
					{
						if (q_ptr->chp < p_target_ptr->chp)
						{
							prev_p_target_ptr = p_target_ptr;
							p_target_ptr = q_ptr;
							prev_target = target;
							target = -i;
						}
					}
				}
			}
			else
			{
				prev_p_target_ptr = p_target_ptr;
				p_target_ptr = q_ptr;
				prev_target = target;
				target = -i;
			}
		}
	}
#endif	// 0

	/* Pick an item with {@O} inscription */
	for (i = 0; i < INVEN_PACK; i++)
	{
		if (!p_ptr->inventory[i].tval) break;

//		unsigned char * inscription = (unsigned char *) quark_str(o_ptr->note);
		inscription = (unsigned char *) quark_str(p_ptr->inventory[i].note);

		/* check for a valid inscription */
		if (inscription == NULL) continue;

		/* scan the inscription for @O */
		while (*inscription != '\0')
		{
			if (*inscription == '@')
			{
				inscription++;

				/* a valid @O has been located */
				if (*inscription == 'O')
				{                       
					inscription++;
					item = i;
					i = INVEN_PACK;
					break;
				}
			}
			inscription++;
		}
	}

	/* If we have a player target, attack him. */
	if (p_target_ptr)
	{
		/* set the target */
		p_ptr->target_who = target;

		/* Attack him */
//		py_attack(Ind, p_target_ptr->py, p_target_ptr->px);
		if (!retaliate_item(Ind, item, inscription))
		{
			py_attack(Ind, p_target_ptr->py, p_target_ptr->px, FALSE);
		}

		/* Check if he is still alive or another targets exists */
		if ((!p_target_ptr->death) || (prev_p_target_ptr) || (m_target_ptr))
		{
			/* We attacked something */
			return 1;
		}
		else
		{
			/* Otherwise return 2 to indicate we are no longer
			 * autoattacking anything.
			 */
			return 2;
		}
	}

	/* The dungeon master does not fight his or her offspring */
	if (p_ptr->admin_dm) return FALSE;

	/* If we have a target to attack, attack it! */
	if (m_target_ptr)
	{
		/* set the target */
		p_ptr->target_who = target;

		/* Attack it */
//		py_attack(Ind, m_target_ptr->fy, m_target_ptr->fx);
		if (!retaliate_item(Ind, item, inscription))
		{
			py_attack(Ind, m_target_ptr->fy, m_target_ptr->fx, FALSE);
		}

		/* Check if it is still alive or another targets exists */
		if ((m_target_ptr->r_idx) || (prev_m_target_ptr) || (p_target_ptr))
		{
			/* We attacked something */
			return 1;
		}
		else
		{
			/* Otherwise return 2 to indicate we are no longer
			 * autoattacking anything.
			 */
			return 2;
		}
	}

	/* Nothing was attacked. */
	return 0;
}

/*
 * Player processing that occurs at the beginning of a new turn
 */
static void process_player_begin(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int			i;

	object_type		*o_ptr;


	/* Give the player some energy */
	p_ptr->energy += extract_energy[p_ptr->pspeed];
#ifdef DUMB_WIN
        p_ptr->energy = 32000;
#endif

	/* Make sure they don't have too much */
	/* But let them store up some extra */
	/* Storing up extra energy lets us perform actions while we are running */
	//if (p_ptr->energy > (level_speed(p_ptr->dun_depth)*6)/5)
	//	p_ptr->energy = (level_speed(p_ptr->dun_depth)*6)/5;
#ifdef NEW_DUNGEON
	if (p_ptr->energy > (level_speed(&p_ptr->wpos)*2) - 1)
		p_ptr->energy = (level_speed(&p_ptr->wpos)*2) - 1;
#else
	if (p_ptr->energy > (level_speed(p_ptr->dun_depth)*2) - 1)
		p_ptr->energy = (level_speed(p_ptr->dun_depth)*2) - 1;
#endif

	/* Check "resting" status */
	if (p_ptr->resting)
	{
		/* No energy availiable while resting */
		// This prevents us from instantly waking up.
		p_ptr->energy = 0;
	}

	/* Handle paralysis here */
       	if (p_ptr->paralyzed || p_ptr->stun >= 100)
       		p_ptr->energy = 0;

	/* Hack -- semi-constant hallucination */
	if (p_ptr->image && (randint(10) < 1)) p_ptr->redraw |= (PR_MAP);

	/* Mega-Hack -- Random teleportation XXX XXX XXX */
	if ((p_ptr->teleport) && (rand_int(100) < 1))
	{
		/* Teleport player */
		teleport_player(Ind, 40);
	}

}


/*
 * Player processing that occurs at the end of a turn
 */
static void process_player_end(int Ind)
{
	player_type *p_ptr = Players[Ind];

//	int		x, y, i, j, new_depth, new_world_x, new_world_y;
	int		x, y, i, j;
	int		regen_amount, NumPlayers_old=NumPlayers;
	char		attackstatus;
	int 		minus;

	cave_type		*c_ptr;
	byte			*w_ptr;
	object_type		*o_ptr;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return;
#endif

	if (Players[Ind]->conn == NOT_CONNECTED)
		return;

	/* Try to execute any commands on the command queue. */
	process_pending_commands(p_ptr->conn);

	/* Mind Fusion/Control disable the char */
	if (p_ptr->esp_link && p_ptr->esp_link_type && (p_ptr->esp_link_flags & LINKF_OBJ)) return;

	/* Check for auto-retaliate */
#ifdef NEW_DUNGEON
	if ((p_ptr->energy >= level_speed(&p_ptr->wpos)) && !p_ptr->confused)
#else
	if ((p_ptr->energy >= level_speed(p_ptr->dun_depth)) && !p_ptr->confused)
#endif
	{
		/* Check for nearby monsters and try to kill them */
		/* If auto_retaliate returns nonzero than we attacked
		 * something and so should use energy.
		 */
		if ((attackstatus = auto_retaliate(Ind)))
		{
			/* Use energy */
//			p_ptr->energy -= level_speed(p_ptr->dun_depth);
		}
	}

	/* Handle running -- 5 times the speed of walking */
#ifdef NEW_DUNGEON
//	while (p_ptr->running && p_ptr->energy >= (level_speed(&p_ptr->wpos)*6)/5)
	while (p_ptr->running && p_ptr->energy >= (level_speed(&p_ptr->wpos)*(cfg.running_speed + 1))/cfg.running_speed)
#else
	while (p_ptr->running && p_ptr->energy >= (level_speed(p_ptr->dun_depth)*6)/5)
#endif
	{
		run_step(Ind, 0);
#ifdef NEW_DUNGEON
		p_ptr->energy -= level_speed(&p_ptr->wpos) / cfg.running_speed;
#else
		p_ptr->energy -= level_speed(p_ptr->dun_depth) / 5;
#endif
	}


	/* Notice stuff */
	if (p_ptr->notice) notice_stuff(Ind);

	/* XXX XXX XXX Pack Overflow */
	pack_overflow(Ind);
#if 0
	if (p_ptr->inventory[INVEN_PACK].k_idx)
	{
		int		amt, j = 0;

		char	o_name[160];

		/* Choose an item to spill */
//		i = INVEN_PACK;

		for(i = INVEN_PACK; i >= 0; i--)
		{
			if(!check_guard_inscription( (p_ptr->inventory[i]).note, 'd' ))
			{
				j = 1;
				break;
			}
		}

		if (!j) i = INVEN_PACK;

		/* Access the slot to be dropped */
		o_ptr = &p_ptr->inventory[i];

		/* Drop all of that item */
		amt = o_ptr->number;

		/* Disturbing */
		disturb(Ind, 0, 0);

		/* Warning */
		msg_print(Ind, "Your pack overflows!");

		/* Describe */
		object_desc(Ind, o_name, o_ptr, TRUE, 3);

		/* Message */
		msg_format(Ind, "You drop %s.", o_name);

		/* Drop it (carefully) near the player */
#ifdef NEW_DUNGEON
		drop_near_severe(Ind, o_ptr, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px);
#else
		drop_near_severe(Ind, o_ptr, 0, p_ptr->dun_depth, p_ptr->py, p_ptr->px);
#endif

		/* Decrease the item, optimize. */
		inven_item_increase(Ind, i, -amt);
		inven_item_optimize(Ind, i);
	}
#endif /* 0 */


	/* Process things such as regeneration. */
	/* This used to be processed every 10 turns, but I am changing it to be
	 * processed once every 5/6 of a "dungeon turn". This will make healing
	 * and poison faster with respect to real time < 1750 feet and slower >
	 * 1750 feet.
	 */
#ifdef NEW_DUNGEON
	if (!(turn%(level_speed(&p_ptr->wpos)/12)))
#else
	if (!(turn%(level_speed(p_ptr->dun_depth)/12)))
#endif
	{
		/*** Damage over Time ***/

		/* Take damage from poison */
		if (p_ptr->poisoned)
		{
			/* Take damage */
			take_hit(Ind, 1, "poison");
		}

		/* Take damage from cuts */
		if (p_ptr->cut)
		{
			/* Mortal wound or Deep Gash */
			if (p_ptr->cut > 200)
			{
				i = 3;
			}

			/* Severe cut */
			else if (p_ptr->cut > 100)
			{
				i = 2;
			}

			/* Other cuts */
			else
			{
				i = 1;
			}

			/* Take damage */
			take_hit(Ind, i, "a fatal wound");
		}

		/*** Check the Food, and Regenerate ***/

		/* Ghosts don't need food */
		/* Allow AFK-hivernation */
		if (!p_ptr->ghost || !p_ptr->afk)
		{
			/* Digest normally */
			if (p_ptr->food < PY_FOOD_MAX)
			{
				/* Every 50/6 level turns */
#ifdef NEW_DUNGEON
			        if (!(turn%((level_speed((&p_ptr->wpos))*10)/12)))
#else
			        if (!(turn%((level_speed((p_ptr->dun_depth))*10)/12)))
#endif
				{
					/* Basic digestion rate based on speed */
					i = extract_energy[p_ptr->pspeed] * 2;

					/* Adrenaline takes more food */
					if (p_ptr->adrenaline) i *= 5;
			
					/* Biofeedback takes more food */
					if (p_ptr->biofeedback) i *= 2;

					/* Regeneration takes more food */
					if (p_ptr->regenerate) i += 30;

					/* Slow digestion takes less food */
					if (p_ptr->slow_digest) i -= 10;

					/* Digest some food */
					(void)set_food(Ind, p_ptr->food - i);

					/* Hack -- check to see if we have been kicked off
					 * due to starvation 
					 */

					if (NumPlayers != NumPlayers_old) return;
				}
			}

			/* Digest quickly when gorged */
			else
			{
				/* Digest a lot of food */
				(void)set_food(Ind, p_ptr->food - 100);
			}

			/* Starve to death (slowly) */
			if (p_ptr->food < PY_FOOD_STARVE)
			{
				/* Calculate damage */
				i = (PY_FOOD_STARVE - p_ptr->food) / 10;

				/* Take damage */
				take_hit(Ind, i, "starvation");
			}
		}

		/* Default regeneration */
		regen_amount = PY_REGEN_NORMAL;

		/* Getting Weak */
		if (p_ptr->food < PY_FOOD_WEAK)
		{
			/* Lower regeneration */
			if (p_ptr->food < PY_FOOD_STARVE)
			{
				regen_amount = 0;
			}
			else if (p_ptr->food < PY_FOOD_FAINT)
			{
				regen_amount = PY_REGEN_FAINT;
			}
			else
			{
				regen_amount = PY_REGEN_WEAK;
			}

			/* Getting Faint */
			if (!p_ptr->ghost && p_ptr->food < PY_FOOD_FAINT)
			{
				/* Faint occasionally */
				if (!p_ptr->paralyzed && (rand_int(100) < 10))
				{
					/* Message */
					msg_print(Ind, "You faint from the lack of food.");
					disturb(Ind, 1, 0);

					/* Hack -- faint (bypass free action) */
					(void)set_paralyzed(Ind, p_ptr->paralyzed + 1 + rand_int(5));
				}
			}
		}

		/* Regeneration ability */
		if (p_ptr->regenerate)
		{
			regen_amount = regen_amount * 2;
		}

		/* Resting */
		if (p_ptr->resting && !p_ptr->searching)
		{
			regen_amount = regen_amount * 3;
		}

		/* Regenerate the mana */
		/* Hack -- regenerate mana 5/3 times faster */
		if (p_ptr->csp < p_ptr->msp)
		{
			regenmana(Ind, (regen_amount * 5) / 3 );
		}

		/* Poisoned or cut yields no healing */
		if (p_ptr->poisoned) regen_amount = 0;
		if (p_ptr->cut) regen_amount = 0;

		/* But Biofeedback always helps */
		if (p_ptr->biofeedback) regen_amount += randint(0x400) + regen_amount;

		/* Regenerate Hit Points if needed */
		if (p_ptr->chp < p_ptr->mhp)
		{
			regenhp(Ind, regen_amount);
		}

		/* Disturb if we are done resting */
		if ((p_ptr->resting) && (p_ptr->chp == p_ptr->mhp) && (p_ptr->csp == p_ptr->msp))
		{
			disturb(Ind, 0, 0);
		}

		/* Unbelievers "resist" magic */
		minus = (p_ptr->anti_magic)?3:1;
		
		/* Finally, at the end of our turn, update certain counters. */
		/*** Timeout Various Things ***/

		/* Handle temporary stat drains */
		for (i = 0; i < 6; i++)
		{
			if (p_ptr->stat_cnt[i] > 0)
			{
				p_ptr->stat_cnt[i]--;
				if (p_ptr->stat_cnt[i] == 0)
				{
					do_res_stat_temp(Ind, i);
				}
			}
		}

		/* Adrenaline */
		if (p_ptr->adrenaline)
		{
		  (void)set_adrenaline(Ind, p_ptr->adrenaline - 1);
		}
		
		/* Biofeedback */				
		if (p_ptr->biofeedback)
                {
		  (void)set_biofeedback(Ind, p_ptr->biofeedback - 1);
                }

		/* Hack -- Bow Branding */
		if (p_ptr->bow_brand)
		{
			(void)set_bow_brand(Ind, p_ptr->bow_brand - minus, p_ptr->bow_brand_t, p_ptr->bow_brand_d);
		}

		/* Hack -- Timed ESP */
		if (p_ptr->tim_esp)
		{
			(void)set_tim_esp(Ind, p_ptr->tim_esp - minus);
		}

		/* Hack -- Space/Time Anchor */
		if (p_ptr->st_anchor)
		{
			(void)set_st_anchor(Ind, p_ptr->st_anchor - minus);
		}

		/* Hack -- Prob travel */
		if (p_ptr->prob_travel)
		{
			(void)set_prob_travel(Ind, p_ptr->prob_travel - minus);
		}

		/* Hack -- Avoid traps */
		if (p_ptr->tim_traps)
		{
			(void)set_tim_traps(Ind, p_ptr->tim_traps - minus);
		}

		/* Hack -- Mimicry */
		if (p_ptr->tim_mimic)
		{
			(void)set_mimic(Ind, p_ptr->tim_mimic - 1, p_ptr->tim_mimic_what);
		}

		/* Hack -- Timed manashield */
		if (p_ptr->tim_manashield)
		  {
		    set_tim_manashield(Ind, p_ptr->tim_manashield - minus);
		  }

		/* Hack -- Tunnel */
		if (p_ptr->auto_tunnel)
		{
			p_ptr->auto_tunnel--;
		}

		/* Hack -- Meditation */
		if (p_ptr->tim_meditation)
		{
			(void)set_tim_meditation(Ind, p_ptr->tim_meditation - minus);
		}

		/* Hack -- Wraithform */
		if (p_ptr->tim_wraith)
		{
			/* In town it only runs out if you are not on a wall
			   To prevent breaking into houses */
#ifdef NEW_DUNGEON
			if (players_on_depth(&p_ptr->wpos) != 0) {
#else
			if (players_on_depth[p_ptr->dun_depth] != 0) {
#endif
				/* important! check for illegal spaces */
#ifdef NEW_DUNGEON
				if (in_bounds(p_ptr->py, p_ptr->px)) {
#else
				if (in_bounds(p_ptr->dun_depth, p_ptr->py, p_ptr->px)) {
#endif
#ifdef NEW_DUNGEON
					if ((p_ptr->wpos.wz) || (cave_floor_bold(zcave, p_ptr->py, p_ptr->px)))
#else
					if ((p_ptr->dun_depth > 0) || (cave_floor_bold(p_ptr->dun_depth, p_ptr->py, p_ptr->px)))
#endif
					{
						(void)set_tim_wraith(Ind, p_ptr->tim_wraith - minus);
					}
				}
			}
		}

		/* Hack -- Hallucinating */
		if (p_ptr->image)
		{
			(void)set_image(Ind, p_ptr->image - 1);
		}

		/* Blindness */
		if (p_ptr->blind)
		{
			(void)set_blind(Ind, p_ptr->blind - 1);
		}

		/* Times see-invisible */
		if (p_ptr->tim_invis)
		{
			(void)set_tim_invis(Ind, p_ptr->tim_invis - minus);
		}

		/* Times invisibility */
		if (p_ptr->tim_invisibility)
		{
		  if (p_ptr->aggravate)
		    {
		      msg_print(Ind, "Your invisibility shield is broken by your aggravation.");
		      (void)set_invis(Ind, 0, 0);
		    }		    
		  else
		    {
		      (void)set_invis(Ind, p_ptr->tim_invisibility - minus, p_ptr->tim_invis_power);
		    }
		}

		/* Timed infra-vision */
		if (p_ptr->tim_infra)
		{
			(void)set_tim_infra(Ind, p_ptr->tim_infra - minus);
		}

		/* Paralysis */
		if (p_ptr->paralyzed)
		{
			(void)set_paralyzed(Ind, p_ptr->paralyzed - 1);
		}

		/* Confusion */
		if (p_ptr->confused)
		{
			(void)set_confused(Ind, p_ptr->confused - minus);
		}

		/* Afraid */
		if (p_ptr->afraid)
		{
			(void)set_afraid(Ind, p_ptr->afraid - minus);
		}

		/* Fast */
		if (p_ptr->fast)
		{
			(void)set_fast(Ind, p_ptr->fast - minus);
		}

		/* Slow */
		if (p_ptr->slow)
		{
			(void)set_slow(Ind, p_ptr->slow - minus);
		}

		/* Protection from evil */
		if (p_ptr->protevil)
		{
			(void)set_protevil(Ind, p_ptr->protevil - 1);
		}

		/* Invulnerability */
		/* Hack -- make -1 permanent invulnerability */
		if (p_ptr->invuln)
		{
			if (p_ptr->invuln > 0)
				(void)set_invuln(Ind, p_ptr->invuln - minus);
		}

		/* Heroism */
		if (p_ptr->hero)
		{
			(void)set_hero(Ind, p_ptr->hero - 1);
		}

		/* Super Heroism */
		if (p_ptr->shero)
		{
			(void)set_shero(Ind, p_ptr->shero - 1);
		}

		/* Furry */
		if (p_ptr->furry)
		{
			(void)set_furry(Ind, p_ptr->furry - 1);
		}

		/* Blessed */
		if (p_ptr->blessed)
		{
			(void)set_blessed(Ind, p_ptr->blessed - 1);
		}

		/* Shield */
		if (p_ptr->shield)
		{
			(void)set_shield(Ind, p_ptr->shield - minus);
		}

		/* Oppose Acid */
		if (p_ptr->oppose_acid)
		{
			(void)set_oppose_acid(Ind, p_ptr->oppose_acid - 1);
		}

		/* Oppose Lightning */
		if (p_ptr->oppose_elec)
		{
			(void)set_oppose_elec(Ind, p_ptr->oppose_elec - 1);
		}

		/* Oppose Fire */
		if (p_ptr->oppose_fire)
		{
			(void)set_oppose_fire(Ind, p_ptr->oppose_fire - 1);
		}

		/* Oppose Cold */
		if (p_ptr->oppose_cold)
		{
			(void)set_oppose_cold(Ind, p_ptr->oppose_cold - 1);
		}

		/* Oppose Poison */
		if (p_ptr->oppose_pois)
		{
			(void)set_oppose_pois(Ind, p_ptr->oppose_pois - 1);
		}

		/*** Poison and Stun and Cut ***/

		/* Poison */
		if (p_ptr->poisoned)
		{
			int adjust = (adj_con_fix[p_ptr->stat_ind[A_CON]] + minus);

			/* Apply some healing */
			(void)set_poisoned(Ind, p_ptr->poisoned - adjust);
		}

		/* Stun */
		if (p_ptr->stun)
		{
			int adjust = (adj_con_fix[p_ptr->stat_ind[A_CON]] + minus);

			/* Apply some healing */
			(void)set_stun(Ind, p_ptr->stun - adjust);
		}

		/* Cut */
		if (p_ptr->cut)
		{
			int adjust = (adj_con_fix[p_ptr->stat_ind[A_CON]] + minus);

			/* Hack -- Truly "mortal" wound */
			if (p_ptr->cut > 1000) adjust = 0;

			/* Biofeedback always helps */
			if (p_ptr->biofeedback) adjust += adjust + 10;

			/* Apply some healing */
			(void)set_cut(Ind, p_ptr->cut - adjust);
		}

		/*** Process Light ***/

		/* Check for light being wielded */
		o_ptr = &p_ptr->inventory[INVEN_LITE];

		/* Burn some fuel in the current lite */
		if (o_ptr->tval == TV_LITE)
		{
			u32b f1 = 0 , f2 = 0 , f3 = 0, f4 = 0, f5 = 0, esp = 0;
			/* Hack -- Use some fuel (sometimes) */
#if 0
			if (!artifact_p(o_ptr) && !(o_ptr->sval == SV_LITE_DWARVEN)
					&& !(o_ptr->sval == SV_LITE_FEANOR) && (o_ptr->pval > 0) && (!o_ptr->name3))
#endif	// 0

			/* Extract the item flags */
			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

			/* Hack -- Use some fuel */
			if ((f4 & TR4_FUEL_LITE) && (o_ptr->timeout > 0))
			{
				/* Decrease life-span */
				o_ptr->timeout--;

				/* Hack -- notice interesting fuel steps */
				if ((o_ptr->timeout < 100) || (!(o_ptr->timeout % 100)))
				{
					/* Window stuff */
					p_ptr->window |= (PW_INVEN | PW_EQUIP);
				}

				/* Hack -- Special treatment when blind */
				if (p_ptr->blind)
				{
					/* Hack -- save some light for later */
					if (o_ptr->timeout == 0) o_ptr->timeout++;
				}

				/* The light is now out */
				else if (o_ptr->timeout == 0)
				{
					disturb(Ind, 0, 0);
					msg_print(Ind, "Your light has gone out!");

					/* Torch disappears */
					if (o_ptr->sval == SV_LITE_TORCH)
					{
						/* Decrease the item, optimize. */
						inven_item_increase(Ind, INVEN_LITE, -1);
						//						inven_item_describe(Ind, INVEN_LITE);
						inven_item_optimize(Ind, INVEN_LITE);
					}
				}

				/* The light is getting dim */
				else if ((o_ptr->timeout < 100) && (!(o_ptr->timeout % 10)))
				{
					if (p_ptr->disturb_minor) disturb(Ind, 0, 0);
					msg_print(Ind, "Your light is growing faint.");
				}
			}
		}

		/* Calculate torch radius */
		p_ptr->update |= (PU_TORCH);

		/*** Process Inventory ***/

		/* Handle experience draining */
		if (p_ptr->exp_drain)
		{
			if ((rand_int(100) < 10) && (p_ptr->exp > 0))
			{
				p_ptr->exp--;
				p_ptr->max_exp--;
				check_experience(Ind);
			}
		}

        /* Handle experience draining.  In Oangband, the effect
		 * is worse, especially for high-level characters.
		 * As per Tolkien, hobbits are resistant.
         */
        if (p_ptr->black_breath)
		{
			byte chance = (p_ptr->prace == RACE_HOBBIT) ? 2 : 5;
			int plev = p_ptr->lev;

			//                if (PRACE_FLAG(PR1_RESIST_BLACK_BREATH)) chance = 2;

			//                if ((rand_int(100) < chance) && (p_ptr->exp > 0))
			/* Toned down a little, considering it's realtime. */
			if ((rand_int(200) < chance))
			{
				int reduce = 1 + plev / 5 + p_ptr->max_exp / 10000L;
				p_ptr->exp -= reduce;
				p_ptr->max_exp -= reduce;
				//                        (void)do_dec_stat(Ind, randint(6)+1, STAT_DEC_NORMAL);
				(void)do_dec_stat(Ind, rand_int(6), STAT_DEC_NORMAL);
				check_experience(Ind);

				/* Now you can die from this :) */
				if (p_ptr->exp < 1 && !p_ptr->ghost)
				{
					p_ptr->chp=-3;
					strcpy(p_ptr->died_from, "Black Breath");
					player_death(Ind);
				}
			}
		}

		/* Drain Mana */
		if (p_ptr->drain_mana && p_ptr->csp)
		{
			p_ptr->csp -= p_ptr->drain_mana;
			if (magik(30)) p_ptr->csp -= p_ptr->drain_mana;

			if (p_ptr->csp < 0) p_ptr->csp = 0;

			/* Redraw */
			p_ptr->redraw |= (PR_MANA);

			/* Window stuff */
			p_ptr->window |= (PW_PLAYER);
		}

		/* Drain Hitpoints */
		if (p_ptr->drain_life)
		{
			int drain = p_ptr->drain_life + rand_int(p_ptr->mhp / 100);

			take_hit(Ind, drain < p_ptr->chp ? drain : p_ptr->chp, "life draining");
#if 0
			p_ptr->chp -= (drain < p_ptr->chp ? drain : p_ptr->chp);

			/* Redraw */
			p_ptr->redraw |= (PR_HP);

			/* Window stuff */
			p_ptr->window |= (PW_PLAYER);
#endif	// 0
		}

		/* Note changes */
		j = 0;

		/* Process equipment */
		for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
		{
			/* Get the object */
			o_ptr = &p_ptr->inventory[i];

			/* Skip non-objects */
			if (!o_ptr->k_idx) continue;

			/* Recharge activatable objects */
			/* (well, light-src should be handled here too? -Jir- */
			if ((o_ptr->timeout > 0) && (o_ptr->tval != TV_LITE))
			{
				/* Recharge */
				o_ptr->timeout--;

				/* Notice changes */
				if (!(o_ptr->timeout)) j++;
			}
		}

		/* Recharge rods */
		/* this should be moved to 'timeout'? */
		for (i = 0; i < INVEN_PACK; i++)
		{
			o_ptr = &p_ptr->inventory[i];

			/* Examine all charging rods */
			if ((o_ptr->tval == TV_ROD) && (o_ptr->pval))
			{
				/* Charge it */
				o_ptr->pval--;

				/* Charge it further */
				if (o_ptr->pval && magik(p_ptr->skill_dev))
					o_ptr->pval--;

				/* Notice changes */
				if (!(o_ptr->pval)) j++;
			}
		}

		/* Notice changes */
		if (j)
		{
			/* Combine pack */
			p_ptr->notice |= (PN_COMBINE);

			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP);
		}

		/* Feel the inventory */
		sense_inventory(Ind);

		/*** Involuntary Movement ***/


		/* Delayed mind link */
		if (p_ptr->esp_link_end)
		{
			p_ptr->esp_link_end--;

			/* Activate the break */
      			if (!p_ptr->esp_link_end)
			{
			  int Ind2;

			  if (p_ptr->esp_link_type && p_ptr->esp_link)
			    {
			      Ind2 = find_player(p_ptr->esp_link);
		    
			      if (!Ind2)
				end_mind(Ind, TRUE);
			      else
				{
				  player_type *p_ptr2 = Players[Ind2];

				  msg_format(Ind, "\377RYou break the mind link with %s.", p_ptr2->name);
				  msg_format(Ind2, "\377R%s breaks the mind link with you.", p_ptr->name);
				  p_ptr->esp_link = 0;
				  p_ptr->esp_link_type = 0;
				  p_ptr->esp_link_flags = 0;
				  p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER);
				  p_ptr->update |= (PU_BONUS | PU_VIEW | PU_MANA | PU_HP);
				  p_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_MAP);
				  p_ptr2->esp_link = 0;
				  p_ptr2->esp_link_type = 0;
				  p_ptr2->esp_link_flags = 0;
				  p_ptr2->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER);
				  p_ptr2->update |= (PU_BONUS | PU_VIEW | PU_MANA | PU_HP);
				  p_ptr2->redraw |= (PR_BASIC | PR_EXTRA | PR_MAP);
				}
			    }
			}
		}

		/* Delayed Word-of-Recall */
		if (p_ptr->word_recall)
		{
			/* Count down towards recall */
			p_ptr->word_recall--;

		       /* MEGA HACK: no recall if icky, or in a shop */
			if( ! p_ptr->word_recall ) 
			{
#ifdef NEW_DUNGEON
				if(character_icky || (p_ptr->store_num > 0) || check_st_anchor(&p_ptr->wpos))
#else
				if(character_icky || (p_ptr->store_num > 0) || check_st_anchor(p_ptr->dun_depth))
#endif
				{
				    p_ptr->word_recall++;
				}
			}

			/* Rework needed! */
			/* Activate the recall */
			if (!p_ptr->word_recall)
			{
				/* sorta circumlocution? */
				worldpos new_pos;

				/* Disturbing! */
				disturb(Ind, 0, 0);

				/* Determine the level */
				/* recalling to surface */
#ifdef NEW_DUNGEON
				if(p_ptr->wpos.wz)
#else
				if (p_ptr->dun_depth > 0)
#endif
				{
					/* Messages */
					if(p_ptr->wpos.wz > 0)
					{
						msg_print(Ind, "You feel yourself yanked downwards!");
						msg_format_near(Ind, "%s is yanked downwards!", p_ptr->name);
					}
					else
					{
						msg_print(Ind, "You feel yourself yanked upwards!");
						msg_format_near(Ind, "%s is yanked upwards!", p_ptr->name);
					}

					/* New location */
#ifdef NEW_DUNGEON
					//					p_ptr->wpos.wz=0;
					new_pos.wx = p_ptr->wpos.wx;
					new_pos.wy = p_ptr->wpos.wy;
					new_pos.wz = 0;
#else
					new_depth = 0;
					new_world_x = p_ptr->world_x;
					new_world_y = p_ptr->world_y;
#endif
					
					p_ptr->new_level_method = LEVEL_RAND;
				}
				/* beware! bugs inside! (jir) */
				/* world travel */
#ifdef NEW_DUNGEON
				/* why wz again? (jir) */
//				else if ((p_ptr->wpos.wz) || (p_ptr->recall_depth < 0))
				else if (!(p_ptr->recall_pos.wz))
#else
				else if ((p_ptr->dun_depth < 0) || (p_ptr->recall_depth < 0))
#endif
				{
					if ((!(p_ptr->wild_map[(wild_idx(&p_ptr->recall_pos))/8] & (1 << (wild_idx(&p_ptr->recall_pos))%8))) ||
						wpcmp(&p_ptr->wpos, &p_ptr->recall_pos))
					{
						/* lazy -- back to the centre of the world
						 * this should be changed so that it lands him/her
						 * back to the last town (s)he visited.	*/
						p_ptr->recall_pos.wx = MAX_WILD_X/2;
						p_ptr->recall_pos.wy = MAX_WILD_Y/2;
					}

					new_pos.wx = p_ptr->recall_pos.wx;
					new_pos.wy = p_ptr->recall_pos.wy;
					new_pos.wz = 0;


					/* Messages */
					msg_print(Ind, "You feel yourself yanked sideways!");
					msg_format_near(Ind, "%s is yanked sideways!", p_ptr->name);
					
					/* New location */
#if 0
#ifdef NEW_DUNGEON
					if (p_ptr->wpos.wz==0 && !istown(&p_ptr->wpos))
#else
					if (p_ptr->dun_depth < 0) 
#endif
					{
						new_depth = 0;
						new_world_x = 0;
						new_world_y = 0;										
					}
					else 
					{ 
						new_depth = p_ptr->recall_depth;
#ifdef NEW_DUNGEON
						fail;
#else
						new_world_x = wild_info[new_depth].world_x;
						new_world_y = wild_info[new_depth].world_y;
#endif
					}
#endif	// 0
					p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;												
				}

				/* into dungeon/tower */
				else
				{
					wilderness_type *w_ptr = &wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx];
					/* Messages */
					if(p_ptr->recall_pos.wz < 0 && w_ptr->flags & WILD_F_DOWN)
					{
//						if (p_ptr->max_dlv < getlevel(p_ptr->wpos))
						if (p_ptr->max_dlv < w_ptr->dungeon->baselevel - p_ptr->recall_pos.wz + 1)
							p_ptr->recall_pos.wz = w_ptr->dungeon->baselevel - p_ptr->max_dlv - 1;

						if (-p_ptr->recall_pos.wz > w_ptr->dungeon->maxdepth)
							p_ptr->recall_pos.wz = 0 - w_ptr->dungeon->maxdepth;

						if (p_ptr->recall_pos.wz >= 0)
						{
							p_ptr->recall_pos.wz = 0;
						}
						else
						{
							msg_print(Ind, "You feel yourself yanked downwards!");
							msg_format_near(Ind, "%s is yanked downwards!", p_ptr->name);
						}
					}
					else if(p_ptr->recall_pos.wz > 0 && w_ptr->flags & WILD_F_UP)
					{
//						if (p_ptr->max_dlv < getlevel(p_ptr->wpos))
						if (p_ptr->max_dlv < w_ptr->tower->baselevel + p_ptr->recall_pos.wz + 1)
							p_ptr->recall_pos.wz = 0 - w_ptr->tower->baselevel + p_ptr->max_dlv + 1;

						if (p_ptr->recall_pos.wz > w_ptr->tower->maxdepth)
							p_ptr->recall_pos.wz = w_ptr->tower->maxdepth;

						if (p_ptr->recall_pos.wz <= 0)
						{
							p_ptr->recall_pos.wz = 0;
						}
						else
						{
							msg_print(Ind, "You feel yourself yanked upwards!");
							msg_format_near(Ind, "%s is yanked upwards!", p_ptr->name);
						}
					}
					else
					{
					p_ptr->recall_pos.wz = 0;
					}
					
#ifdef NEW_DUNGEON
//					p_ptr->wpos.wz=p_ptr->recall_depth;
					new_pos.wx = p_ptr->wpos.wx;
					new_pos.wy = p_ptr->wpos.wy;
					new_pos.wz = p_ptr->recall_pos.wz;
#else
					new_depth = p_ptr->recall_depth;
					new_world_x = p_ptr->world_x;
					new_world_y = p_ptr->world_y;
#endif
					if (p_ptr->recall_pos.wz == 0)
					{
						msg_print(Ind, "You feel yourself yanked toward nowhere...");
						p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
					}
					else p_ptr->new_level_method = LEVEL_RAND;
				}
				
				/* One less person here */
#ifdef NEW_DUNGEON
				new_players_on_depth(&p_ptr->wpos,-1,TRUE);
#else
				players_on_depth[p_ptr->dun_depth]--;
#endif
				
				/* paranoia, required for adding old wilderness saves to new servers */
#ifdef NEW_DUNGEON
				if (players_on_depth(&p_ptr->wpos) < 0) new_players_on_depth(&p_ptr->wpos,0,FALSE);
#else
				if (players_on_depth[p_ptr->dun_depth] < 0) players_on_depth[p_ptr->dun_depth] = 0;
#endif

				/* Remove the player */
#ifdef NEW_DUNGEON
				zcave[p_ptr->py][p_ptr->px].m_idx = 0;
				/* Show everyone that he's left */
				everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);
#else
				cave[p_ptr->dun_depth][p_ptr->py][p_ptr->px].m_idx = 0;
				/* Show everyone that he's left */
				everyone_lite_spot(p_ptr->dun_depth, p_ptr->py, p_ptr->px);
#endif
					

				/* Forget his lite and view */
				forget_lite(Ind);
				forget_view(Ind);

#ifdef NEW_DUNGEON
				wpcopy(&p_ptr->wpos, &new_pos);
#else
				p_ptr->dun_depth = new_depth;
				p_ptr->world_x = new_world_x;
				p_ptr->world_y = new_world_y;
#endif

				/* One more person here */
#ifdef NEW_DUNGEON
				new_players_on_depth(&p_ptr->wpos,1,TRUE);
#else
				players_on_depth[p_ptr->dun_depth]++;
#endif

				/* He'll be safe for 2 turn */
				set_invuln_short(Ind, 2);

				p_ptr->new_level_flag = TRUE;

				/* He'll be safe for 2 turns */
				set_invuln_short(Ind, 2);
			}

			/* EE's original */
#if 0 /* sorry - evileye */
			/* Activate the recall */
      			if (!p_ptr->word_recall)
			{
				/* Disturbing! */
				disturb(Ind, 0, 0);

				/* Determine the level */
#ifdef NEW_DUNGEON
				if(p_ptr->wpos.wz)
#else
				if (p_ptr->dun_depth > 0)
#endif
				{
					/* Messages */
					msg_print(Ind, "You feel yourself yanked upwards!");
					msg_format_near(Ind, "%s is yanked upwards!", p_ptr->name);
					
					/* New location */
#ifdef NEW_DUNGEON
					p_ptr->wpos.wz=0;
#else
					new_depth = 0;
					new_world_x = p_ptr->world_x;
					new_world_y = p_ptr->world_y;
#endif
					
					p_ptr->new_level_method = LEVEL_RAND;
				}
#ifdef NEW_DUNGEON
				else if ((p_ptr->wpos.wz) || (p_ptr->recall_depth < 0))
#else
				else if ((p_ptr->dun_depth < 0) || (p_ptr->recall_depth < 0))
#endif
				{
					/* Messages */
					msg_print(Ind, "You feel yourself yanked sideways!");
					msg_format_near(Ind, "%s is yanked sideways!", p_ptr->name);
					
					/* New location */
#ifdef NEW_DUNGEON
					if (p_ptr->wpos.wz==0 && !istown(&p_ptr->wpos))
#else
					if (p_ptr->dun_depth < 0) 
#endif
					{
						new_depth = 0;
						new_world_x = 0;
						new_world_y = 0;										
					}
					else 
					{ 
						new_depth = p_ptr->recall_depth;
#ifdef NEW_DUNGEON
						fail;
#else
						new_world_x = wild_info[new_depth].world_x;
						new_world_y = wild_info[new_depth].world_y;
#endif
					}
					p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;												
				}
				else
				{
					/* Messages */
					msg_print(Ind, "You feel yourself yanked downwards!");
					msg_format_near(Ind, "%s is yanked downwards!", p_ptr->name);
#ifdef NEW_DUNGEON
					p_ptr->wpos.wz=p_ptr->recall_depth;
#else
					new_depth = p_ptr->recall_depth;
					new_world_x = p_ptr->world_x;
					new_world_y = p_ptr->world_y;
#endif
					p_ptr->new_level_method = LEVEL_RAND;
				}
				
				/* One less person here */
#ifdef NEW_DUNGEON
				new_players_on_depth(&p_ptr->wpos,-1,TRUE);
#else
				players_on_depth[p_ptr->dun_depth]--;
#endif
				
				/* paranoia, required for adding old wilderness saves to new servers */
#ifdef NEW_DUNGEON
				if (players_on_depth(&p_ptr->wpos) < 0) new_players_on_depth(&p_ptr->wpos,0,FALSE);
#else
				if (players_on_depth[p_ptr->dun_depth] < 0) players_on_depth[p_ptr->dun_depth] = 0;
#endif

				/* Remove the player */
#ifdef NEW_DUNGEON
				zcave[p_ptr->py][p_ptr->px].m_idx = 0;
				/* Show everyone that he's left */
				everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);
#else
				cave[p_ptr->dun_depth][p_ptr->py][p_ptr->px].m_idx = 0;
				/* Show everyone that he's left */
				everyone_lite_spot(p_ptr->dun_depth, p_ptr->py, p_ptr->px);
#endif
					

				/* Forget his lite and view */
				forget_lite(Ind);
				forget_view(Ind);

#ifdef NEW_DUNGEON
				wpcopy(&p_ptr->wpos, &new_depth);
#else
				p_ptr->dun_depth = new_depth;
				p_ptr->world_x = new_world_x;
				p_ptr->world_y = new_world_y;
#endif

				/* One more person here */
#ifdef NEW_DUNGEON
				new_players_on_depth(&p_ptr->wpos,1,TRUE);
#else
				players_on_depth[p_ptr->dun_depth]++;
#endif

				/* He'll be safe for 2 turn */
				set_invuln_short(Ind, 2);

				p_ptr->new_level_flag = TRUE;

				/* He'll be safe for 2 turns */
				set_invuln_short(Ind, 2);
			}
#endif /* if 0 recall out (sorry - evileye */
		}
	}


	/* HACK -- redraw stuff a lot, this should reduce perceived latency. */
	/* This might not do anything, I may have been silly when I added this. -APD */
	/* Notice stuff (if needed) */
	if (p_ptr->notice) notice_stuff(Ind);

	/* Update stuff (if needed) */
	if (p_ptr->update) update_stuff(Ind);
	
	/* Redraw stuff (if needed) */
	if (p_ptr->redraw) redraw_stuff(Ind);

	/* Redraw stuff (if needed) */
	if (p_ptr->window) window_stuff(Ind);
}

#ifdef NEW_DUNGEON
void do_unstat(struct worldpos *wpos)
{
	int num_on_depth = 0;
	int j;
	player_type *p_ptr;
	// Count the number of players actually in game on this depth
	for (j = 1; j < NumPlayers + 1; j++)
	{
		p_ptr = Players[j];
		if (inarea(&p_ptr->wpos, wpos)) num_on_depth++;
	}
	// If this level is static and no one is actually on it
	if (!num_on_depth)
	{
		/* makes levels between 50ft and min_unstatic_level unstatic on player saving/quiting game/leaving level DEG */
		if (( getlevel(wpos) < cfg.min_unstatic_level) && (0 < cfg.min_unstatic_level))
		{
			new_players_on_depth(wpos,0,FALSE);
		}
		// random chance of the level unstaticing
		// the chance is one in (base_chance * depth)/250 feet.
		if (!rand_int(((cfg.level_unstatic_chance * (getlevel(wpos)+5))/5)-1))
		{
			// unstatic the level
			new_players_on_depth(wpos,0,FALSE);
		}
	}
}
#endif

/*
 * 24 hourly scan of houses - should the odd house be owned by
 * a non player. Hopefully never, but best to save admin work.
 */
void scan_houses()
{
	int i;
	int lval;
	s_printf("Doing house maintenance\n");
	for(i=0;i<num_houses;i++)
	{
		switch(houses[i].dna->owner_type)
		{
			case OT_PLAYER:
				if(!lookup_player_name(houses[i].dna->owner))
				{
					s_printf("Found old player houses. ID: %d\n", houses[i].dna->owner);
					kill_houses(houses[i].dna->owner, OT_PLAYER);
				}
				break;
			case OT_PARTY:
				if(!strlen(parties[houses[i].dna->owner].name))
				{
					s_printf("Found old party houses. ID: %d\n", houses[i].dna->owner);
					kill_houses(houses[i].dna->owner, OT_PARTY);
				}
				break;
		}
	}
	s_printf("Finished house maintenance\n");
}

/*
 * This function handles "global" things such as the stores,
 * day/night in the town, etc.
 */
 
/* Added the japanese unique respawn patch -APD- 
   It appears that each moment of time is equal to 10 minutes?
*/
static void process_various(void)
{
	int i, j, y, x, num_on_depth;
	cave_type *c_ptr;
	player_type *p_ptr;

	char buf[1024];

	/* Save the server state occasionally */
	if (!(turn % ((NumPlayers ? 10L : 1000L) * SERVER_SAVE)))
	{
		save_server_info();

		/* Save each player */
		for (i = 1; i <= NumPlayers; i++)
		{
			/* Save this player */
			save_player(i);
		}
	}

	/* daily maintenance */
	if (!(turn % (cfg.fps * 86400)))
	{
		s_printf("24 hours maintenance cycle\n");
		scan_players();
		scan_houses();
		s_printf("Finished maintenance\n");
	}

	/* Handle certain things once a minute */
	if (!(turn % (cfg.fps * 60)))
	{
		monster_race *r_ptr;

		/* Update the player retirement timers */
		for (i = 1; i <= NumPlayers; i++)
		{
			p_ptr = Players[i];

			// If our retirement timer is set
			if (p_ptr->retire_timer > 0)
			{
				// Decrement our retire timer
				j = --p_ptr->retire_timer;

				// Alert him
				if (j <= 60)
				{
					msg_format(i, "\377rYou have %d minute%s of tenure left.", j, j > 1 ? "s" : "");
				}
				else if (j <= 1440 && !(p_ptr->retire_timer % 60))
				{
					msg_format(i, "\377yYou have %d hours of tenure left.", j / 60);
				}
				else if (!(j % 1440))
				{
					msg_format(i, "\377GYou have %d days of tenure left.", j / 1440);
				}


				// If the timer runs out, forcibly retire
				// this character.
				if (!j)
				{
					do_cmd_suicide(i);
				}
			}

			/* Reswpan for kings' joy  -Jir- */
			/* Update the unique respawn timers */
			for (j = 1; j <= NumPlayers; j++)
			{
				p_ptr = Players[j];
				if (!p_ptr->total_winner) continue;

				/* Hack -- never Maggot and his dogs :) */
				i = rand_range(50,MAX_R_IDX-2);
				r_ptr = &r_info[i];

				/* Make sure we are looking at a dead unique */
				if (!(r_ptr->flags1 & RF1_UNIQUE))
				{
					j--;
					continue;
				}

				if (!p_ptr->r_killed[i]) continue;

				/* Hack -- Sauron and Morgoth are exceptions */
				if (r_ptr->flags1 & RF1_QUESTOR) continue;

				//				if (r_ptr->max_num > 0) continue;
				if (rand_int(cfg.unique_respawn_time * (r_ptr->level + 1)) > 9)
					continue;

				/* "Ressurect" the unique */
				p_ptr->r_killed[i] = 0;
				// 				r_ptr->max_num = 1;
				//				r_ptr->respawn_timer = -1;

				/* Tell every player */
				msg_format(j,"%s rises from the dead!",(r_name + r_ptr->name));
				//				msg_broadcast(0,buf); 
			}
		}

#if 0 /* No more reswpan */
		/* Update the unique respawn timers */
		for (i = 1; i < MAX_R_IDX-1; i++)
		{
			monster_race *r_ptr = &r_info[i];

			/* Make sure we are looking at a dead unique */
			if (!(r_ptr->flags1 & RF1_UNIQUE)) continue;
			if (r_ptr->max_num > 0) continue;

			/* Hack -- Initially set a newly killed uniques respawn timer */
			/* -1 denotes that the respawn timer is unset */
			if (r_ptr->respawn_timer < 0) 
			{
				r_ptr->respawn_timer = cfg_unique_respawn_time * (r_ptr->level + 1);
				if (r_ptr->respawn_timer > cfg_unique_max_respawn_time)
					r_ptr->respawn_timer = cfg_unique_max_respawn_time;
			}
			// Decrament the counter 
			else r_ptr->respawn_timer--; 
			// Once the timer hits 0, ressurect the unique.
			if (!r_ptr->respawn_timer)
			{
				/* "Ressurect" the unique */
				r_ptr->max_num = 1;
				r_ptr->respawn_timer = -1;

				/* Tell every player */
				sprintf(buf,"%s rises from the dead!",(r_name + r_ptr->name));
				msg_broadcast(0,buf); 
			}	    			
		}
#endif


		/* If the level unstaticer is not disabled, try to.
		 * Now it's done every 60*fps, so better raise the
		 * rate as such.	- Jir -
		 */
		
//		if (cfg.level_unstatic_chance > 0)
		{
#ifdef NEW_DUNGEON
			struct worldpos twpos;
			twpos.wz=0;
			for(y=0;y<MAX_WILD_Y;y++)
			{
				twpos.wy=y;
				for(x=0;x<MAX_WILD_X;x++)
				{
					struct wilderness_type *w_ptr;
					struct dungeon_type *d_ptr;
					twpos.wx=x;
					w_ptr=&wild_info[twpos.wy][twpos.wx];

					if (cfg.level_unstatic_chance > 0 &&
						players_on_depth(&twpos))
						do_unstat(&twpos);

					if(!players_on_depth(&twpos) && !istown(&twpos) && getcave(&twpos))
						dealloc_dungeon_level_maybe(&twpos);

					if(w_ptr->flags & WILD_F_UP)
					{
						d_ptr=w_ptr->tower;
						for(i=1;i<=d_ptr->maxdepth;i++)
						{
							twpos.wz=i;
							if (cfg.level_unstatic_chance > 0 &&
								players_on_depth(&twpos))
								do_unstat(&twpos);
							
							if(!players_on_depth(&twpos) && getcave(&twpos))
								dealloc_dungeon_level_maybe(&twpos);
						}
					}
					if(w_ptr->flags & WILD_F_DOWN)
					{
						d_ptr=w_ptr->dungeon;
						for(i=1;i<=d_ptr->maxdepth;i++)
						{
							twpos.wz=-i;
							if (cfg.level_unstatic_chance > 0 &&
								players_on_depth(&twpos))
								do_unstat(&twpos);

							if(!players_on_depth(&twpos) && getcave(&twpos))
								dealloc_dungeon_level_maybe(&twpos);
						}
					}
					twpos.wz=0;
				}
			}
#else
			// For each dungeon level
			for (i = 1; i < MAX_DEPTH; i++)
			{
				// If this depth is static
				if (players_on_depth[i])
				{
					num_on_depth = 0;
					// Count the number of players actually in game on this depth
					for (j = 1; j < NumPlayers + 1; j++)
					{
						p_ptr = Players[j];
						if (p_ptr->dun_depth == i) num_on_depth++;
					}
					// If this level is static and no one is actually on it
					if (!num_on_depth)
					{
						/* makes levels between 50ft and min_unstatic_level unstatic on player saving/quiting game/leaving level DEG */
						if (( i < cfg.min_unstatic_level) && (0 < cfg.min_unstatic_level))
						{
							players_on_depth[i] = 0;
						}
						// random chance of the level unstaticing
						// the chance is one in (base_chance * depth)/250 feet.
						if (!rand_int(((cfg.level_unstatic_chance * (i+5))/5)-1))
						{
							// unstatic the level
							players_on_depth[i] = 0;
						}
					}
				}
			}
#endif
		}
#if 0
		if (cfg.level_unstatic_chance > 0)
		{
#ifdef NEW_DUNGEON
			struct worldpos twpos;
			twpos.wz=0;
			for(y=0;y<MAX_WILD_Y;y++)
			{
				twpos.wy=y;
				for(x=0;x<MAX_WILD_X;x++)
				{
					struct wilderness_type *w_ptr;
					struct dungeon_type *d_ptr;
					twpos.wx=x;
					w_ptr=&wild_info[twpos.wy][twpos.wx];
					if(players_on_depth(&twpos))
					{
						do_unstat(&twpos);
					}
					if(w_ptr->flags & WILD_F_UP)
					{
						d_ptr=w_ptr->tower;
						for(i=1;i<=d_ptr->maxdepth;i++)
						{
							twpos.wz=i;
							if(players_on_depth(&twpos))
							{
								do_unstat(&twpos);
							}
						}
					}
					if(w_ptr->flags & WILD_F_DOWN)
					{
						d_ptr=w_ptr->dungeon;
						for(i=1;i<=d_ptr->maxdepth;i++)
						{
							twpos.wz=-i;
							if(players_on_depth(&twpos))
							{
								do_unstat(&twpos);
							}
						}
					}
					twpos.wz=0;
				}
			}
#else
			// For each dungeon level
			for (i = 1; i < MAX_DEPTH; i++)
			{
				// If this depth is static
				if (players_on_depth[i])
				{
					num_on_depth = 0;
					// Count the number of players actually in game on this depth
					for (j = 1; j < NumPlayers + 1; j++)
					{
						p_ptr = Players[j];
						if (p_ptr->dun_depth == i) num_on_depth++;
					}
					// If this level is static and no one is actually on it
					if (!num_on_depth)
					{
						/* makes levels between 50ft and min_unstatic_level unstatic on player saving/quiting game/leaving level DEG */
						if (( i < cfg.min_unstatic_level) && (0 < cfg.min_unstatic_level))
						{
							players_on_depth[i] = 0;
						}
						// random chance of the level unstaticing
						// the chance is one in (base_chance * depth)/250 feet.
						if (!rand_int(((cfg.level_unstatic_chance * (i+5))/5)-1))
						{
							// unstatic the level
							players_on_depth[i] = 0;
						}
					}
				}
			}
#endif	// NEW_DUNGEON
		}
#endif	// 0
	}

#if 0
	/* Grow trees very occasionally */
	if (!(turn % (10L * GROW_TREE)))
	{
		/* Find a suitable location */
		for (i = 1; i < 1000; i++)
		{
			cave_type *c_ptr;

			/* Pick a location */
			y = rand_range(1, MAX_HGT - 1);
			x = rand_range(1, MAX_WID - 1);

			/* Acquire pointer */
#ifdef NEW_DUNGEON
			c_ptr = &zcave[0][y][x];
#else
			c_ptr = &cave[0][y][x];
#endif

			/* Only allow "dirt" */
			if (c_ptr->feat != FEAT_DIRT) continue;

			/* Never grow on top of objects or monsters */
			if (c_ptr->m_idx) continue;
			if (c_ptr->o_idx) continue;

			/* Grow a tree here */
			c_ptr->feat = FEAT_TREE;

			/* Show it */
			everyone_lite_spot(0, y, x);

			/* Done */
			break;
		}
	}
#endif /* if 0 */

	/* Update the stores */
	if (!(turn % (10L * STORE_TURNS)))
	{
		int n;

		for(i=0;i<numtowns;i++)
		{
			/* Maintain each shop (except home and auction house) */
			for (n = 0; n < MAX_STORES - 2; n++)
			{
				/* Maintain */
				store_maint(&town[i].townstore[n]);
			}

			/* Sometimes, shuffle the shopkeepers */
			if (rand_int(STORE_SHUFFLE) == 0)
			{
				/* Shuffle a random shop (except home and auction house) */
				store_shuffle(&town[i].townstore[rand_int(MAX_STORES - 2)]);
			}
		}
	}

#if 0
	/* Hack -- Daybreak/Nightfall outside the dungeon */
	if (!(turn % ((10L * TOWN_DAWN) / 2)))
	{
		bool dawn;

		/* Check for dawn */
		dawn = (!(turn % (10L * TOWN_DAWN)));
		/* Day breaks */
		if (dawn)
		{
			/* Mega-Hack -- erase all wilderness monsters.
			 * This should prevent wilderness monster "buildup",
			 * massive worm infestations, and uniques getting
			 * lost out there.
			 */
#ifdef NEW_DUNGEON
			struct worldpos twpos;
			twpos.wz=0;
			for(y=0;y<MAX_WILD_Y;y++){
				twpos.wy=y;
				for(x=0;x<MAX_WILD_X;x++){
					twpos.wx=x;
					if(!players_on_depth(&twpos)) wipe_m_list(&twpos);
					wild_info[twpos.wy][twpos.wx].flags&=~(WILD_F_INHABITED);
				}
			}
#else
			for (i = 1; i < MAX_WILD; i++) 
				/* if no one is here the monsters 'migrate'.*/
				if (!players_on_depth[-i]) wipe_m_list(-i);
			/* another day, more stuff to kill... */
			for (i = 1; i < MAX_WILD; i++) wild_info[-i].flags &= ~(WILD_F_INHABITED);
#endif
		
			/* Hack -- Scan the town */
			for (y = 0; y < MAX_HGT; y++)
			{
				for (x = 0; x < MAX_WID; x++)
				{
					 /* Get the cave grid */
#ifdef NEW_DUNGEON
					c_ptr = &zcave[0][y][x];
#else
					c_ptr = &cave[0][y][x];
#endif

					 /* Assume lit */
					c_ptr->info |= CAVE_GLOW;

					 /* Hack -- Notice spot */
#ifdef NEW_DUNGEON
					note_spot_depth(townpos, y, x);
#else
					note_spot_depth(0, y, x);
#endif
				}
			} 
		}	
		else
		{
			/* Hack -- Scan the town */
			for (y = 0; y < MAX_HGT; y++)
			{
				for (x = 0; x < MAX_WID; x++)
				{
					 /* Get the cave grid */
#ifdef NEW_DUNGEON
					c_ptr = &zcave[0][y][x];
#else
					c_ptr = &cave[0][y][x];
#endif

					 /* Darken "boring" features */
					if (c_ptr->feat <= FEAT_INVIS && !(c_ptr->info & CAVE_ROOM))
					{
						 /* Darken the grid */
						c_ptr->info &= ~CAVE_GLOW;
					}
				}
			}
			/* hack -- make fruit bat wear off */
			/* No more -- DG */
#if 0			
			for (x = 1; x < NumPlayers + 1; x++)
			{
				if (Players[x]->fruit_bat)
				{
					Players[x]->fruit_bat--;
					if (!Players[x]->fruit_bat)
					msg_print(x, "Your form feels much more familliar.");
				}
			}
#endif
		}
	}
#endif /* if 0 */
}
			
int find_player(s32b id)
{
	int i;

	for (i = 1; i < NumPlayers + 1; i++)
	{
		player_type *p_ptr = Players[i];
		
		if (p_ptr->id == id) return i;
	}
	
	/* assume none */
	return 0;
}		
			
int find_player_name(char *name)
{
	int i;

	for (i = 1; i < NumPlayers + 1; i++)
	{
		player_type *p_ptr = Players[i];
		
		if (!strcmp(p_ptr->name, name)) return i;
	}
	
	/* assume none */
	return 0;
}		

/*
 * Main loop --KLJ--
 *
 * This is main loop; it is called every 1/FPS seconds.  Usually FPS is about
 * 10, so that a normal unhasted unburdened character gets 1 player turn per
 * second.  Note that we process every player and the monsters, then quit.
 * The "scheduling" code (see sched.c) is the REAL main loop, which handles
 * various inputs and timings.
 */

void dungeon(void)
{
	int i, d;
	byte *w_ptr;
	cave_type *c_ptr;

	/* Return if no one is playing */
	/* if (!NumPlayers) return; */

	/* Check for death.  Go backwards (very important!) */
	for (i = NumPlayers; i > 0; i--)
	{
		/* Check connection first */
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* Check for death */
		if (Players[i]->death)
		{
			player_type *p_ptr = Players[i];
			if (p_ptr->mode == MODE_HELL)
			{
				/* Kill him */
				player_death(i);
			}
			else
			{
				/* Kill him */
				player_death(i);

				/* Kill them again so that they Die! DEG */
				if (cfg.no_ghost)
				{
					player_death(i);
				}
			}
		}
	}

	/* Check player's depth info */
	for (i = 1; i < NumPlayers + 1; i++)
	{
		player_type *p_ptr = Players[i];
#ifdef NEW_DUNGEON
		struct worldpos *wpos=&p_ptr->wpos;
		cave_type **zcave;
		struct worldpos twpos;
#else
		int Depth = p_ptr->dun_depth;
#endif
                int j, x, y, startx, starty, m_idx, my, mx;

		if (p_ptr->conn == NOT_CONNECTED)
			continue;

		if (!p_ptr->new_level_flag)
			continue;

		/* Check "maximum depth" to make sure it's still correct */
#ifdef NEW_DUNGEON
		if ((!p_ptr->ghost) && (getlevel(wpos) > p_ptr->max_dlv))
			p_ptr->max_dlv = getlevel(wpos);
#else
		if ((!p_ptr->ghost) && (Depth > p_ptr->max_dlv))
			p_ptr->max_dlv = Depth;
#endif

		/* Make sure the server doesn't think the player is in a store */
		p_ptr->store_num = -1;

		/* Hack -- artifacts leave the queen/king */
//		if (!p_ptr->admin_dm && !p_ptr->admin_wiz && player_is_king(Ind))
		if (cfg.kings_etiquette && p_ptr->total_winner &&
			!p_ptr->admin_dm && !p_ptr->admin_wiz)
		{
			int j;
			object_type *o_ptr;
			char		o_name[160];

			for (j = 0; j < INVEN_TOTAL; j++)
			{
				o_ptr = &p_ptr->inventory[j];
				if (!o_ptr->k_idx || !true_artifact_p(o_ptr)) continue;

				if (strstr((a_name + a_info[o_ptr->name1].name),"Grond") ||
					strstr((a_name + a_info[o_ptr->name1].name),"Morgoth"))
					continue;

				/* Describe the object */
				object_desc(i, o_name, o_ptr, TRUE, 0);

				/* Message */
				msg_format(i, "\377y%s bids farewell to you...", o_name);
				a_info[o_ptr->name1].cur_num = 0;
				a_info[o_ptr->name1].known = FALSE;

				/* Eliminate the item  */
				inven_item_increase(i, j, -99);
				inven_item_describe(i, j);
				inven_item_optimize(i, j);

				j--;
			}
		}

		/* Somebody has entered an ungenerated level */
#ifdef NEW_DUNGEON
		if (players_on_depth(wpos) && !getcave(wpos))
#else
		if (players_on_depth[Depth] && !cave[Depth])
#endif
		{
#ifdef NEW_DUNGEON
			/* Allocate space for it */
			alloc_dungeon_level(wpos);

			/* Generate a dungeon level there */
			generate_cave(wpos);
#else
			/* Allocate space for it */
			alloc_dungeon_level(Depth);

			/* Generate a dungeon level there */
			generate_cave(Depth);
#endif
		}
		zcave=getcave(wpos);

		/* Clear the "marked" and "lit" flags for each cave grid */
		for (y = 0; y < MAX_HGT; y++)
		{
			for (x = 0; x < MAX_WID; x++)
			{
				w_ptr = &p_ptr->cave_flag[y][x];

				*w_ptr = 0;
			}
		}

		/* hack -- update night/day in wilderness levels */
#ifdef NEW_DUNGEON
		if ((!wpos->wz) && (IS_DAY)) wild_apply_day(wpos); 
		if ((!wpos->wz) && (IS_NIGHT)) wild_apply_night(wpos);
#else
		if ((Depth < 0) && (IS_DAY)) wild_apply_day(Depth); 
		if ((Depth < 0) && (IS_NIGHT)) wild_apply_night(Depth);
#endif

		/* Memorize the town and all wilderness levels close to town */
#ifdef NEW_DUNGEON
		/* EE, didn't you forget to add suburbs? */
		if (istown(wpos))
#else
		if (Depth <= 0 ? (wild_info[Depth].radius <= 2) : 0)
#endif
		{
			bool dawn = ((turn % (10L * TOWN_DAWN)) < (10L * TOWN_DAWN / 2)); 

			p_ptr->max_panel_rows = (MAX_HGT / SCREEN_HGT) * 2 - 2;
			p_ptr->max_panel_cols = (MAX_WID / SCREEN_WID) * 2 - 2;

			p_ptr->cur_hgt = MAX_HGT;
			p_ptr->cur_wid = MAX_WID;

			/* Memorize the town for this player (if daytime) */
			for (y = 0; y < MAX_HGT; y++)
			{
				for (x = 0; x < MAX_WID; x++)
				{
					w_ptr = &p_ptr->cave_flag[y][x];
#ifdef NEW_DUNGEON
					c_ptr = &zcave[y][x];
#else
					c_ptr = &cave[Depth][y][x];
#endif

					/* Memorize if daytime or "interesting" */
					if (dawn || c_ptr->feat > FEAT_INVIS || c_ptr->info & CAVE_ROOM)
						*w_ptr |= CAVE_MARK;
				}
			}
		}
		else
		{
			p_ptr->max_panel_rows = (MAX_HGT / SCREEN_HGT) * 2 - 2;
			p_ptr->max_panel_cols = (MAX_WID / SCREEN_WID) * 2 - 2;

			p_ptr->cur_hgt = MAX_HGT;
			p_ptr->cur_wid = MAX_WID;
		}

		/* Determine starting location */
		switch (p_ptr->new_level_method)
		{
			/* Climbed down */
#ifdef NEW_DUNGEON
			case LEVEL_DOWN:  starty = level_down_y(wpos);
					  startx = level_down_x(wpos);
#else
			case LEVEL_DOWN:  starty = level_down_y[Depth];
					  startx = level_down_x[Depth];
#endif
					  break;

			/* Climbed up */
#ifdef NEW_DUNGEON
			case LEVEL_UP:    starty = level_up_y(wpos);
					  startx = level_up_x(wpos);
#else
			case LEVEL_UP:    starty = level_up_y[Depth];
					  startx = level_up_x[Depth];
#endif
					  break;
			
			/* Teleported level */
#ifdef NEW_DUNGEON
			case LEVEL_RAND:  starty = level_rand_y(wpos);
					  startx = level_rand_x(wpos);
#else
			case LEVEL_RAND:  starty = level_rand_y[Depth];
					  startx = level_rand_x[Depth];
#endif
					  break;
			
			/* Used ghostly travel */
			case LEVEL_GHOST: starty = p_ptr->py;
					  startx = p_ptr->px;
					  break;
					  
			/* Over the river and through the woods */			  
			case LEVEL_OUTSIDE: starty = p_ptr->py;
				            startx = p_ptr->px;
				            break;
			/* this is used instead of extending the level_rand_y/x
			   into the negative direction to prevent us from
			   alocing so many starting locations.  Although this does
			   not make players teleport to simmilar locations, this
			   could be achieved by seeding the RNG with the depth.
			*/
			case LEVEL_OUTSIDE_RAND: 
			
				/* make sure we aren't in an "icky" location */
				do
				{
					starty = rand_int(MAX_HGT-3)+1;
					startx = rand_int(MAX_WID-3)+1;
				}
#ifdef NEW_DUNGEON
				while (  (zcave[starty][startx].info & CAVE_ICKY)
				      || (!cave_floor_bold(zcave, starty, startx)) );
#else
				while (  (cave[Depth][starty][startx].info & CAVE_ICKY)
				      || (!cave_floor_bold(Depth, starty, startx)) );
#endif
				break;
		}

		//printf("finding area (%d,%d)\n",startx,starty);
		/* Place the player in an empty space */
		//for (j = 0; j < 1500; ++j)
		for (j = 0; j < 1500; j++)
		{
			/* Increasing distance */
			d = (j + 149) / 150;

			/* Pick a location */
#ifdef NEW_DUNGEON
			scatter(wpos, &y, &x, starty, startx, d, 1);
			/* Must have an "empty" grid */
			if (!cave_empty_bold(zcave, y, x))
			{
				continue;
			}

			/* Not allowed to go onto a icky location (house) if Depth <= 0 */
			if ((wpos->wz==0) && (zcave[y][x].info & CAVE_ICKY))
			{
				continue;
			}
#else
			scatter(Depth, &y, &x, starty, startx, d, 1);
			/* Must have an "empty" grid */
			if (!cave_empty_bold(Depth, y, x)) continue;

			/* Not allowed to go onto a icky location (house) if Depth <= 0 */
			if ((Depth <= 0) && (cave[Depth][y][x].info & CAVE_ICKY))
				continue;
#endif

			break;
		}

#if 0
		/* Place the player in an empty space */
		for (j = 0; j < 1500; ++j)
		{
			/* Increasing distance */
			d = (j + 149) / 150;

			/* Pick a location */
#ifdef NEW_DUNGEON
			scatter(wpos, &y, &x, starty, startx, d, 1);

			/* Must have an "empty" grid */
			if (!cave_empty_bold(zcave, y, x)) continue;

			/* Not allowed to go onto a icky location (house) if Depth <= 0 */
			if ((wpos->wz==0) && (zcave[y][x].info & CAVE_ICKY))
#else
			scatter(Depth, &y, &x, starty, startx, d, 1);

			/* Must have an "empty" grid */
			if (!cave_empty_bold(Depth, y, x)) continue;

			/* Not allowed to go onto a icky location (house) if Depth <= 0 */
			if ((Depth <= 0) && (cave[Depth][y][x].info & CAVE_ICKY))
				continue;
#endif

			break;
		}
#endif /*0*/
		p_ptr->py = y;
		p_ptr->px = x;

		/* Update the player location */
#ifdef NEW_DUNGEON
		zcave[y][x].m_idx = 0 - i;
#else
		cave[Depth][y][x].m_idx = 0 - i;
#endif

		for (m_idx = m_top - 1; m_idx >= 0; m_idx--)
		{
			monster_type *m_ptr = &m_list[m_fast[m_idx]];
			cave_type **mcave;
			mcave=getcave(&m_ptr->wpos);

			if (!m_fast[m_idx]) continue;

			/* Excise "dead" monsters */
			if (!m_ptr->r_idx) continue;

			if (m_ptr->owner != p_ptr->id) continue;

			if (m_ptr->mind & GOLEM_GUARD && !(m_ptr->mind&GOLEM_FOLLOW)) continue;

			starty = m_ptr->fy;
			startx = m_ptr->fx;
			starty = y;
			startx = x;

			/* Place the golems in an empty space */
			for (j = 0; j < 1500; ++j)
			{
				/* Increasing distance */
				d = (j + 149) / 150;

				/* Pick a location */
#ifdef NEW_DUNGEON
				scatter(wpos, &my, &mx, starty, startx, d, 0);

				/* Must have an "empty" grid */
				if (!cave_empty_bold(zcave, my, mx)) continue;

				/* Not allowed to go onto a icky location (house) if Depth <= 0 */
				if ((wpos->wz==0) && (zcave[my][mx].info & CAVE_ICKY))
					continue;
#else
				scatter(Depth, &my, &mx, starty, startx, d, 0);

				/* Must have an "empty" grid */
				if (!cave_empty_bold(Depth, my, mx)) continue;

				/* Not allowed to go onto a icky location (house) if Depth <= 0 */
				if ((Depth <= 0) && (cave[Depth][my][mx].info & CAVE_ICKY))
					continue;
#endif

				break;
			}
#ifdef NEW_DUNGEON
			if(mcave)
			{
				mcave[m_ptr->fy][m_ptr->fx].m_idx = 0;
				everyone_lite_spot(&m_ptr->wpos, m_ptr->fy, m_ptr->fx);
			}
			wpcopy(&m_ptr->wpos,wpos);
			m_ptr->fx = mx;
			m_ptr->fy = my;
			if(mcave)
			{
				mcave[m_ptr->fy][m_ptr->fx].m_idx = m_fast[m_idx];
				everyone_lite_spot(&m_ptr->wpos, m_ptr->fy, m_ptr->fx);
			}
#else
			cave[m_ptr->dun_depth][m_ptr->fy][m_ptr->fx].m_idx = 0;
			everyone_lite_spot(m_ptr->dun_depth, m_ptr->fy, m_ptr->fx);
			m_ptr->dun_depth = Depth;
			m_ptr->fx = mx;
			m_ptr->fy = my;
			cave[m_ptr->dun_depth][m_ptr->fy][m_ptr->fx].m_idx = m_fast[m_idx];
			everyone_lite_spot(m_ptr->dun_depth, m_ptr->fy, m_ptr->fx);
#endif

			/* Update the monster (new location) */
			update_mon(m_fast[m_idx], TRUE);
		}
#if 0
		while (TRUE)
		{
			y = rand_range(1, ((Depth) ? (MAX_HGT - 2) : (SCREEN_HGT - 2)));
			x = rand_range(1, ((Depth) ? (MAX_WID - 2) : (SCREEN_WID - 2)));

#ifdef NEW_DUNGEON
			/* Must be a "naked" floor grid */
			if (!cave_naked_bold(zcave, y, x)) continue;

			/* Refuse to start on anti-teleport grids */
			if (zcave[y][x].info & CAVE_ICKY) continue;
#else
			/* Must be a "naked" floor grid */
			if (!cave_naked_bold(Depth, y, x)) continue;

			/* Refuse to start on anti-teleport grids */
			if (cave[Depth][y][x].info & CAVE_ICKY) continue;
#endif

			break;
		}
#endif

		/* Recalculate panel */
		p_ptr->panel_row = ((p_ptr->py - SCREEN_HGT / 4) / (SCREEN_HGT / 2));
		if (p_ptr->panel_row > p_ptr->max_panel_rows) p_ptr->panel_row = p_ptr->max_panel_rows;
		else if (p_ptr->panel_row < 0) p_ptr->panel_row = 0;

		p_ptr->panel_col = ((p_ptr->px - SCREEN_WID / 4) / (SCREEN_WID / 2));
		if (p_ptr->panel_col > p_ptr->max_panel_cols) p_ptr->panel_col = p_ptr->max_panel_cols;
		else if (p_ptr->panel_col < 0) p_ptr->panel_col = 0;
	
		p_ptr->redraw |= (PR_MAP);
		p_ptr->redraw |= (PR_DEPTH);

		panel_bounds(i);
		forget_view(i);
		forget_lite(i);
		update_view(i);
		update_lite(i);
		update_monsters(TRUE);
		
		/* Tell him that he should beware */
#ifdef NEW_DUNGEON
		if (wpos->wz==0)
		{
			if (wild_info[wpos->wy][wpos->wx].own)
			{
				cptr p = lookup_player_name(wild_info[wpos->wy][wpos->wx].own);
				if (p == NULL) p = "Someone";

				msg_format(i, "You enter the land of %s.", p);
			}
		}

#else
		if (Depth < 0)
		{
			if (wild_info[Depth].own)
			{
				cptr p = lookup_player_name(wild_info[Depth].own);
				if (p == NULL) p = "Someone";

				msg_format(i, "You enter the land of %s.", p);
			}
		}

#endif
		/* Clear the flag */
		p_ptr->new_level_flag = FALSE;

#if 0	// moved to 'process_various'
		/* Check to see which if the level needs generation or destruction */
		/* Note that "town" is excluded */
#ifdef NEW_DUNGEON
		twpos.wz=0;
		for(y=0;y<MAX_WILD_Y;y++)
		{
			twpos.wy=y;
			for(x=0;x<MAX_WILD_X;x++)
			{
				wilderness_type *w_ptr;
				struct dungeon_type *d_ptr;
				twpos.wx=x;
				w_ptr=&wild_info[twpos.wy][twpos.wx];
				if(!players_on_depth(&twpos) && !istown(&twpos) && getcave(&twpos))
					dealloc_dungeon_level_maybe(&twpos);
				if(w_ptr->flags & WILD_F_UP)
				{
					d_ptr=w_ptr->tower;
					for(j=1;j<=d_ptr->maxdepth;j++)
					{
						twpos.wz=j;
						if(!players_on_depth(&twpos) && getcave(&twpos))
							dealloc_dungeon_level_maybe(&twpos);
					}
				}
				if(w_ptr->flags & WILD_F_DOWN)
				{
					d_ptr=w_ptr->dungeon;
					for(j=1;j<=d_ptr->maxdepth;j++)
					{
						twpos.wz=-j;
						if(!players_on_depth(&twpos) && getcave(&twpos))
							dealloc_dungeon_level_maybe(&twpos);
					}
				}
				twpos.wz=0;
			}
		}
#else
		for (j = -MAX_WILD+1; j < MAX_DEPTH; j++)
		{
			/* Everybody has left a level that is still generated */
			if ((players_on_depth[j] == 0) && cave[j])
			  //			if ((players_on_depth[j] == 0) && (!wild_info[j].own) && cave[j])
			{
				/* Destroy the level */
				/* Hack -- don't dealloc the town */
				if (j)
					dealloc_dungeon_level(j);
			}

		}
#endif	// NEW_DUNGEON
#endif	// 0
	}

	/* Handle any network stuff */
	Net_input();

	/* Hack -- Compact the object list occasionally */
	if (o_top + 16 > MAX_O_IDX) compact_objects(32);

	/* Hack -- Compact the monster list occasionally */
	if (m_top + 32 > MAX_M_IDX) compact_monsters(64);

	/* Hack -- Compact the trap list occasionally */
	if (t_top + 16 > MAX_TR_IDX) compact_traps(32);


	// Note -- this is the END of the last turn

	/* Do final end of turn processing for each player */
	for (i = 1; i < NumPlayers + 1; i++)
	{
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* Actually process that player */
		process_player_end(i);
	}

	/* Check for death.  Go backwards (very important!) */
	for (i = NumPlayers; i > 0; i--)
	{
		/* Check connection first */
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* Check for death */
		if (Players[i]->death)
		{
			player_type *p_ptr = Players[i];
			if (p_ptr->mode == MODE_HELL)
			{
				/* Kill him */
				player_death(i);
			}
			else
			{
				/* Kill him */
				player_death(i);

				/* Kill them again so that they Die! DEG */
				if (cfg.no_ghost)
				{
					player_death(i);
				}
			}


		}
	}



	///*** BEGIN NEW TURN ***///
	turn++;

	/* Do some beginning of turn processing for each player */
	for (i = 1; i < NumPlayers + 1; i++)
	{
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* Actually process that player */
		process_player_begin(i);
	}

	/* Process all of the monsters */
	process_monsters();

	/* Process all of the objects */
	process_objects();

	/* Probess the world */
	for (i = 1; i < NumPlayers + 1; i++)
	{
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* Process the world of that player */
		process_world(i);
	}

	/* Process everything else */
	process_various();

	/* Hack -- Regenerate the monsters every hundred game turns */
	if (!(turn % 100)) regen_monsters();

	/* Refresh everybody's displays */
	for (i = 1; i < NumPlayers + 1; i++)
	{
		player_type *p_ptr = Players[i];

		if (p_ptr->conn == NOT_CONNECTED)
			continue;

		/* Notice stuff */
		if (p_ptr->notice) notice_stuff(i);

		/* Update stuff */
		if (p_ptr->update) update_stuff(i);

		/* Redraw stuff */
		if (p_ptr->redraw) redraw_stuff(i);

		/* Window stuff */
		if (p_ptr->window) window_stuff(i);
	}

	/* Send any information over the network */
	Net_output();
}

		
/*
 * Load the various "user pref files"
 */
static void load_all_pref_files(void)
{
	char buf[1024];


	/* Access the "basic" pref file */
	strcpy(buf, "pref.prf");

	/* Process that file */
	process_pref_file(buf);

	/* Access the "user" pref file */
	sprintf(buf, "user.prf");

	/* Process that file */
	process_pref_file(buf);



}

/*
 * Actually play a game
 *
 * If the "new_game" parameter is true, then, after loading the
 * server-specific savefiles, we will start anew.
 */
void play_game(bool new_game)
{
	int i, n;


	/* Hack -- Character is "icky" */
	/*character_icky = TRUE;*/


	/* Hack -- turn off the cursor */
	/*(void)Term_set_cursor(0);*/


	/* Attempt to load the server state information */
	if (!load_server_info())
	{
		/* Oops */
		quit("broken server savefile(s)");
	}

	/* UltraHack -- clear each wilderness levels inhabited flag, so
	   monsters will respawn.
	   hack -- clear the wild_f_in_memory flag, so house objects are added
	   once and only once.
	
	   I believe this is no longer neccecary.
	for (i = 1; i < MAX_WILD; i++) wild_info[-i].flags &= ~(WILD_F_IN_MEMORY);
	*/

	/* Nothing loaded */
	if (!server_state_loaded)
	{
		/* Make server state info */
		new_game = TRUE;

		/* Create a new dungeon */
		server_dungeon = FALSE;
	}

	/* Process old character */
	if (!new_game)
	{
		/* Process the player name */
		/*process_player_name(FALSE);*/
	}

	/* Init the RNG */
	// Is it a good idea ? DGDGDGD --  maybe FIXME
	//	if (Rand_quick)
	{
		u32b seed;

		/* Basic seed */
		seed = (time(NULL));

#ifdef SET_UID

		/* Mutate the seed on Unix machines */
		seed = ((seed >> 3) * (getpid() << 1));

#endif

		/* Use the complex RNG */
		Rand_quick = FALSE;

		/* Seed the "complex" RNG */
		Rand_state_init(seed);
	}

	/* Extract the options */
	for (i = 0; option_info[i].o_desc; i++)
	{
		int os = option_info[i].o_set;
		int ob = option_info[i].o_bit;

		/* Set the "default" options */
		if (option_info[i].o_var)
		{
			/* Set */
			if (option_flag[os] & (1L << ob))
			{
				/* Set */
				(*option_info[i].o_var) = TRUE;
			}
			
			/* Clear */
			else
			{
				/* Clear */
				(*option_info[i].o_var) = FALSE;
			}
		}
	}

	/* Roll new town */
	if (new_game)
	{
		/* Ignore the dungeon */
		server_dungeon = FALSE;

		/* Start in town */
		/*dun_level = 0;*/

		/* Hack -- seed for flavors */
		seed_flavor = rand_int(0x10000000);

		/* Hack -- seed for town layout */
		seed_town = rand_int(0x10000000);

		/* Initialize server state information */
		/*player_birth();*/
		server_birth();

		/* Hack -- enter the world */
		turn = 1;

#if 0
		/* Initialize the stores */
		for (n = 0; n < MAX_STORES; n++)
		{
			/* Initialize */
			store_init(n);
	
			/* Ignore home and auction house */
			if ((n == MAX_STORES - 2) || (n == MAX_STORES - 1)) continue;
	
			/* Maintain the shop */
			for (i = 0; i < 10; i++) store_maint(n);
		}
#endif
	}


	/* Flash a message */
	s_printf("Please wait...\n");

	/* Flush the message */
	/*Term_fresh();*/


	/* Hack -- Enter wizard mode */
	/*if (arg_wizard && enter_wizard_mode()) wizard = TRUE;*/


	/* Flavor the objects */
	flavor_init();

	s_printf("Object flavors initialized...\n");

	/* Reset the visual mappings */
	reset_visuals();

	/* Window stuff */
	/*p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER);*/

	/* Window stuff */
	/*p_ptr->window |= (PW_MONSTER);*/

	/* Window stuff */
	/*window_stuff();*/


	/* Load the "pref" files */
	load_all_pref_files();

	/* Set or clear "rogue_like_commands" if requested */
	/*if (arg_force_original) rogue_like_commands = FALSE;
	if (arg_force_roguelike) rogue_like_commands = TRUE;*/

	/* Verify the keymap */
	/*keymap_init();*/

	/* React to changes */
	/*Term_xtra(TERM_XTRA_REACT, 0);*/


	/* Make a town if necessary */
	if (!server_dungeon)
	{
#ifdef NEW_DUNGEON
		struct worldpos twpos;
		twpos.wx=MAX_WILD_X/2;
		twpos.wy=MAX_WILD_Y/2;
		twpos.wz=0;
		/* Allocate space for it */
		alloc_dungeon_level(&twpos);

		/* Actually generate the town */
		generate_cave(&twpos);
#else
		/* Allocate space for it */
		alloc_dungeon_level(0);

		/* Actually generate the town */
		generate_cave(0);
#endif
	}

	/* Finish initializing dungeon monsters */
	setup_monsters();

	/* Finish initializing dungeon objects */
	setup_objects();

	/* Finish initializing dungeon objects */
	setup_traps();

	/* Server initialization is now "complete" */
	server_generated = TRUE;


	/* Hack -- Character is no longer "icky" */
	/*character_icky = FALSE;*/


	/* Start game */
	/*alive = TRUE;*/

	/* Hack -- Enforce "delayed death" */
	/*if (p_ptr->chp < 0) death = TRUE;*/

	/* Set up the contact socket, so we can allow players to connect */
	setup_contact_socket();

	/* Set up the network server */
	if (Setup_net_server() == -1)
		quit("Couldn't set up net server");

	/* scan for inactive players */
	scan_players();
	scan_houses();

	/* Set up the main loop */
	install_timer_tick(dungeon, cfg.fps);

	/* Loop forever */
	sched();

	/* This should never, ever happen */
	s_printf("sched returned!!!\n");

	/* Close stuff */
	close_game();

	/* Quit */
	quit(NULL);
}


void shutdown_server(void)
{
	int i;

	s_printf("Shutting down.\n");

	/* Kick every player out and save his game */
	while(NumPlayers > 0)
	{
		/* Note the we always save the first player */
		player_type *p_ptr = Players[1];

		/* Indicate cause */
		strcpy(p_ptr->died_from, "server shutdown");

		/* Try to save */
		if (!save_player(1)) Destroy_connection(p_ptr->conn, "Server shutdown (save failed)");

		/* Successful save */
		Destroy_connection(p_ptr->conn, "Server shutdown (save succeeded)");
	}

#ifdef NEW_DUNGEON
#else
	/* Now wipe every object, to preserve artifacts on the ground */
	for (i = 1; i < MAX_DEPTH; i++)
	{
		/* Wipe this depth */
		wipe_o_list(i);
	}
#endif

	/* Stop the timer */
	teardown_timer();

	/* Save the server state */
	if (!save_server_info()) quit("Server state save failed!");

	/* Tell the metaserver that we're gone */
	Report_to_meta(META_DIE);

	quit("Server state saved");
}

void pack_overflow(int Ind)
{
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	int i;
        u32b f1, f2, f3, f4, f5, esp;
	
	/* XXX XXX XXX Pack Overflow */
	if (p_ptr->inventory[INVEN_PACK].k_idx)
	{
		int		amt, j = 0;

		char	o_name[160];

		/* Choose an item to spill */
//		i = INVEN_PACK;

		for(i = INVEN_PACK; i >= 0; i--)
		{
			o_ptr = &p_ptr->inventory[i];
			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

			if(!check_guard_inscription( o_ptr->note, 'd' ) &&
				!(f4 & TR4_CURSE_NO_DROP) && cursed_p(o_ptr)) 
			{
				j = 1;
				break;
			}
		}

		if (!j) i = INVEN_PACK;

		/* Access the slot to be dropped */
		o_ptr = &p_ptr->inventory[i];

		/* Drop all of that item */
		amt = o_ptr->number;

		/* Disturbing */
		disturb(Ind, 0, 0);

		/* Warning */
		msg_print(Ind, "Your pack overflows!");

		/* Describe */
		object_desc(Ind, o_name, o_ptr, TRUE, 3);

		/* Message */
		msg_format(Ind, "You drop %s.", o_name);

		/* Drop it (carefully) near the player */
#ifdef NEW_DUNGEON
		drop_near_severe(Ind, o_ptr, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px);
#else
		drop_near_severe(Ind, o_ptr, 0, p_ptr->dun_depth, p_ptr->py, p_ptr->px);
#endif

		/* Decrease the item, optimize. */
		inven_item_increase(Ind, i, -amt);
		inven_item_optimize(Ind, i);
	}
}
