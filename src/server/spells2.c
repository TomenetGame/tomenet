/* File: spells2.c */

/* Purpose: Spell code (part 2) */

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
 * Increase players hit points, notice effects, and tell the player about it.
 */
bool hp_player(int Ind, int num)
{
	player_type *p_ptr = Players[Ind];

	// The "number" that the character is displayed as before healing
	int old_num = (p_ptr->chp * 95) / (p_ptr->mhp*10); 
	int new_num; 

	/* Hell mode is .. hard */
	if (p_ptr->mode == MODE_HELL)
	  {
            num = num * 3 / 4;
	  }

	if (p_ptr->chp < p_ptr->mhp)
	{
		p_ptr->chp += num;

		if (p_ptr->chp > p_ptr->mhp)
		{
			p_ptr->chp = p_ptr->mhp;
			p_ptr->chp_frac = 0;
		}

		/* Update health bars */
		update_health(0 - Ind);

		/* Redraw */
		p_ptr->redraw |= (PR_HP);

		/* Figure out of if the player's "number" has changed */
		new_num = (p_ptr->chp * 95) / (p_ptr->mhp*10); 
		if (new_num >= 7) new_num = 10;

		/* If so then refresh everyone's view of this player */
		if (new_num != old_num)
#ifdef NEW_DUNGEON
			everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);
#else
			everyone_lite_spot(p_ptr->dun_depth, p_ptr->py, p_ptr->px);
#endif

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		num = num / 5;
		if (num < 3)
		{
			if (num == 0)
			{
				msg_print(Ind, "You feel a little better.");
			}
			else
			{
				msg_print(Ind, "You feel better.");
			}
		}
		else
		{
			if (num < 7)
			{
				msg_print(Ind, "You feel much better.");
			}
			else
			{
				msg_print(Ind, "You feel very good.");
			}
		}

		return (TRUE);
	}

	return (FALSE);
}

/*
 * Increase players hit points, notice effects, and don't tell the player it.
 */
bool hp_player_quiet(int Ind, int num)
{
	player_type *p_ptr = Players[Ind];

	// The "number" that the character is displayed as before healing
	int old_num = (p_ptr->chp * 95) / (p_ptr->mhp*10); 
	int new_num; 


	if (p_ptr->chp < p_ptr->mhp)
	{
		p_ptr->chp += num;

		if (p_ptr->chp > p_ptr->mhp)
		{
			p_ptr->chp = p_ptr->mhp;
			p_ptr->chp_frac = 0;
		}

		/* Update health bars */
		update_health(0 - Ind);

		/* Redraw */
		p_ptr->redraw |= (PR_HP);

		/* Figure out of if the player's "number" has changed */
		new_num = (p_ptr->chp * 95) / (p_ptr->mhp*10); 
		if (new_num >= 7) new_num = 10;

		/* If so then refresh everyone's view of this player */
		if (new_num != old_num)
#ifdef NEW_DUNGEON
			everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);
#else
			everyone_lite_spot(p_ptr->dun_depth, p_ptr->py, p_ptr->px);
#endif

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		return (TRUE);
	}

	return (FALSE);
}




/*
 * Leave a "glyph of warding" which prevents monster movement
 */
void warding_glyph(int Ind)
{
	player_type *p_ptr = Players[Ind];

	cave_type *c_ptr;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return;

	/* Require clean space */
	if (!cave_clean_bold(zcave, p_ptr->py, p_ptr->px)) return;

	/* Access the player grid */
	c_ptr = &zcave[p_ptr->py][p_ptr->px];
#else
	/* Require clean space */
	if (!cave_clean_bold(p_ptr->dun_depth, p_ptr->py, p_ptr->px)) return;

	/* Access the player grid */
	c_ptr = &cave[p_ptr->dun_depth][p_ptr->py][p_ptr->px];
#endif

	/* Create a glyph of warding */
	c_ptr->feat = FEAT_GLYPH;
}




/*
 * Array of stat "descriptions"
 */
static cptr desc_stat_pos[] =
{
	"strong",
	"smart",
	"wise",
	"dextrous",
	"healthy",
	"cute"
};


/*
 * Array of stat "descriptions"
 */
static cptr desc_stat_neg[] =
{
	"weak",
	"stupid",
	"naive",
	"clumsy",
	"sickly",
	"ugly"
};


/*
 * Lose a "point"
 */
bool do_dec_stat(int Ind, int stat, int mode)
{
	player_type *p_ptr = Players[Ind];

	bool sust = FALSE;

	/* Access the "sustain" */
	switch (stat)
	{
		case A_STR: if (p_ptr->sustain_str) sust = TRUE; break;
		case A_INT: if (p_ptr->sustain_int) sust = TRUE; break;
		case A_WIS: if (p_ptr->sustain_wis) sust = TRUE; break;
		case A_DEX: if (p_ptr->sustain_dex) sust = TRUE; break;
		case A_CON: if (p_ptr->sustain_con) sust = TRUE; break;
		case A_CHR: if (p_ptr->sustain_chr) sust = TRUE; break;
	}

	/* Sustain */
	if (sust)
	{
		/* Message */
		msg_format(Ind, "You feel %s for a moment, but the feeling passes.",
		           desc_stat_neg[stat]);

		/* Notice effect */
		return (TRUE);
	}

	/* Attempt to reduce the stat */
	if (dec_stat(Ind, stat, 10, mode))
	{
		/* Message */
		msg_format(Ind, "You feel very %s.", desc_stat_neg[stat]);

		/* Notice effect */
		return (TRUE);
	}

	/* Nothing obvious */
	return (FALSE);
}


/*
 * Restore lost "points" in a stat
 */
bool do_res_stat(int Ind, int stat)
{
	/* Attempt to increase */
	if (res_stat(Ind, stat))
	{
		/* Message */
		msg_format(Ind, "You feel less %s.", desc_stat_neg[stat]);

		/* Notice */
		return (TRUE);
	}

	/* Nothing obvious */
	return (FALSE);
}

/*
 * Restore temporary-lost "points" in a stat
 */
bool do_res_stat_temp(int Ind, int stat)
{
	player_type *p_ptr = Players[Ind];

	/* Restore if needed */
	if (p_ptr->stat_cur[stat] != p_ptr->stat_max[stat])
	{
		/* Restore */
		p_ptr->stat_cur[stat] += p_ptr->stat_los[stat];

		if (p_ptr->stat_cur[stat] > p_ptr->stat_max[stat])
			p_ptr->stat_cur[stat] = p_ptr->stat_max[stat];

		p_ptr->stat_los[stat] = 0;

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Message */
		msg_format(Ind, "You feel less %s.", desc_stat_neg[stat]);
		/* Success */
		return (TRUE);
	}

	/* Nothing to restore */
	return (FALSE);
}

/*
 * Gain a "point" in a stat
 */
bool do_inc_stat(int Ind, int stat)
{
	bool res;

	/* Restore strength */
	res = res_stat(Ind, stat);

	/* Attempt to increase */
	if (inc_stat(Ind, stat))
	{
		/* Message */
		msg_format(Ind, "Wow!  You feel very %s!", desc_stat_pos[stat]);

		/* Notice */
		return (TRUE);
	}

	/* Restoration worked */
	if (res)
	{
		/* Message */
		msg_format(Ind, "You feel less %s.", desc_stat_neg[stat]);

		/* Notice */
		return (TRUE);
	}

	/* Nothing obvious */
	return (FALSE);
}



/*
 * Identify everything being carried.
 * Done by a potion of "self knowledge".
 */
void identify_pack(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int                 i;
	object_type        *o_ptr;

	/* Simply identify and know every item */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		o_ptr = &p_ptr->inventory[i];
		if (o_ptr->k_idx)
		{
			object_aware(Ind, o_ptr);
			object_known(o_ptr);
		}
	}
}






/*
 * Used by the "enchant" function (chance of failure)
 */
static int enchant_table[16] =
{
	0, 10,  50, 100, 200,
	300, 400, 500, 700, 950,
	990, 992, 995, 997, 999,
	1000
};


/*
 * Removes curses from items in inventory
 *
 * Note that Items which are "Perma-Cursed" (The One Ring,
 * The Crown of Morgoth) can NEVER be uncursed.
 *
 * Note that if "all" is FALSE, then Items which are
 * "Heavy-Cursed" (Mormegil, Calris, and Weapons of Morgul)
 * will not be uncursed.
 */
static int remove_curse_aux(int Ind, int all)
{
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;

	int		i, j, cnt = 0;
	u32b f1, f2, f3, f4, f5, esp;

	/* Attempt to uncurse items being worn */
	/* Now this can uncurse ones in inventory */
//	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	for (j = 0; j < INVEN_TOTAL; j++)
	{

		i = (j + INVEN_WIELD) % INVEN_TOTAL;

		o_ptr = &p_ptr->inventory[i];

		/* Uncursed already */
		if (!cursed_p(o_ptr)) continue;

		/* Extract the flags */
			  object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

		/* Heavily Cursed Items need a special spell */
		if (!all && (f3 & TR3_HEAVY_CURSE)) continue;

		/* Perma-Cursed Items can NEVER be uncursed */
		if (f3 & TR3_PERMA_CURSE) continue;

		/* Uncurse it */
		o_ptr->ident &= ~ID_CURSED;

		/* Hack -- Assume felt */
		o_ptr->ident |= ID_SENSE;

		/* Take note */
		o_ptr->note = quark_add("uncursed");

		/* Reverse the curse effect */
		/* jk - scrolls of *remove curse* have a 1 in (55-level chance to */
		/* reverse the curse effects - a ring of damage(-15) {cursed} then */
		/* becomes a ring of damage (+15) */
		/* this does not go for artifacts - a Sword of Mormegil +40,+60 would */
		/* be somewhat unbalancing */
		/* due to the nature of this procedure, it only works on cursed items */
		/* ie you get only one chance! */
		if (((randint(p_ptr->lev > 50) ? 5 : 55 - p_ptr->lev)==1) && !artifact_p(o_ptr))
		{
			if (o_ptr->to_a<0) o_ptr->to_a=-o_ptr->to_a;
			if (o_ptr->to_h<0) o_ptr->to_h=-o_ptr->to_h;
			if (o_ptr->to_d<0) o_ptr->to_d=-o_ptr->to_d;
			if (o_ptr->pval<0) o_ptr->pval=-o_ptr->pval;
		}

		/* Recalculate the bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Count the uncursings */
		cnt++;
		
		/* Not everything at once. */
		if (!all && magik(50)) break;
	}

	/* Return "something uncursed" */
	return (cnt);
}


/*
 * Remove most curses
 */
bool remove_curse(int Ind)
{
	return (remove_curse_aux(Ind, FALSE));
}

/*
 * Remove all curses
 */
bool remove_all_curse(int Ind)
{
	return (remove_curse_aux(Ind, TRUE));
}



/*
 * Restores any drained experience
 */
bool restore_level(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* Restore experience */
	if (p_ptr->exp < p_ptr->max_exp)
	{
		/* Message */
		msg_print(Ind, "You feel your life energies returning.");

		/* Restore the experience */
		p_ptr->exp = p_ptr->max_exp;

		/* Check the experience */
		check_experience(Ind);

		/* Did something */
		return (TRUE);
	}

	/* No effect */
	return (FALSE);
}


/*
 * self-knowledge... idea from nethack.  Useful for determining powers and
 * resistences of items.  It saves the screen, clears it, then starts listing
 * attributes, a screenful at a time.  (There are a LOT of attributes to
 * list.  It will probably take 2 or 3 screens for a powerful character whose
 * using several artifacts...) -CFT
 *
 * It is now a lot more efficient. -BEN-
 *
 * See also "identify_fully()".
 *
 * XXX XXX XXX Use the "show_file()" method, perhaps.
 */
void self_knowledge(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int		i = 0, k;

	u32b	f1 = 0L, f2 = 0L, f3 = 0L, f4 = 0L, f5 = 0L, esp = 0L;

	object_type	*o_ptr;

	cptr	*info = p_ptr->info;

	/* Let the player scroll through the info */
	p_ptr->special_file_type = TRUE;

	/* Acquire item flags from equipment */
	for (k = INVEN_WIELD; k < INVEN_TOTAL; k++)
	{
		u32b t1, t2, t3, t4, t5, tesp;

		o_ptr = &p_ptr->inventory[k];

		/* Skip empty items */
		if (!o_ptr->k_idx) continue;

		/* Extract the flags */
		object_flags(o_ptr, &t1, &t2, &t3, &t4, &t5, &tesp);

		/* Extract flags */
		f1 |= t1;
		f2 |= t2;
		f3 |= t3;
		f4 |= t4;
		f5 |= t5;
	}


	if (p_ptr->blind)
	{
		info[i++] = "You cannot see.";
	}
	if (p_ptr->confused)
	{
		info[i++] = "You are confused.";
	}
	if (p_ptr->afraid)
	{
		info[i++] = "You are terrified.";
	}
	if (p_ptr->cut)
	{
		info[i++] = "\377rYou are bleeding.";
	}
	if (p_ptr->stun)
	{
		info[i++] = "\377oYou are stunned.";
	}
	if (p_ptr->poisoned)
	{
		info[i++] = "\377GYou are poisoned.";
	}
	if (p_ptr->image)
	{
		info[i++] = "You are \377rha\377oll\377yuc\377gin\377bat\377ving.";
	}

	if (p_ptr->aggravate)
	{
		info[i++] = "You aggravate monsters.";
	}
	if (p_ptr->teleport)
	{
		info[i++] = "Your position is very uncertain.";
	}

	if (p_ptr->blessed)
	{
		info[i++] = "You feel rightous.";
	}
	if (p_ptr->hero)
	{
		info[i++] = "You feel heroic.";
	}
	if (p_ptr->shero)
	{
		info[i++] = "You are in a battle rage.";
	}
	if (p_ptr->protevil)
	{
		info[i++] = "You are protected from evil.";
	}
#if 0
	if (p_ptr->protgood)
	{
		info[i++] = "You are protected from good.";
	}
#endif	// 0
	if (p_ptr->shield)
	{
		info[i++] = "You are protected by a mystic shield.";
	}
	if (p_ptr->invuln)
	{
		info[i++] = "You are temporarily invulnerable.";
	}
	if (p_ptr->confusing)
	{
		info[i++] = "Your hands are glowing dull red.";
	}
	if (p_ptr->stunning)
	{
		info[i++] = "Your hands are very heavy.";
	}
	if (p_ptr->searching)
	{
		info[i++] = "You are looking around very carefully.";
	}
	if (p_ptr->new_spells)
	{
		info[i++] = "You can learn some more spells.";
	}
	if (p_ptr->word_recall)
	{
		info[i++] = "You will soon be recalled.";
	}
	if (p_ptr->see_infra)
	{
		info[i++] = "Your eyes are sensitive to infrared light.";
	}
	if (p_ptr->see_inv)
	{
		info[i++] = "You can see invisible creatures.";
	}
	if (p_ptr->feather_fall)
	{
		info[i++] = "You land gently.";
	}
#if 0
	if (p_ptr->climb)
	{
		info[i++] = "You can climb hight mountains.";
	}
#endif	// 0
	if (p_ptr->free_act)
	{
		info[i++] = "You have free action.";
	}
	if (p_ptr->regenerate)
	{
		info[i++] = "You regenerate quickly.";
	}
	if (p_ptr->slow_digest)
	{
		info[i++] = "Your appetite is small.";
	}
	if (p_ptr->telepathy)
	{
		//		info[i++] = "You have ESP.";
		if (p_ptr->telepathy & ESP_ALL) info[i++] = "You have ESP.";
		else
		{
			if (p_ptr->telepathy & ESP_ORC) info[i++] = "You can sense the presence of orcs.";
			if (p_ptr->telepathy & ESP_TROLL) info[i++] = "You can sense the presence of trolls.";
			if (p_ptr->telepathy & ESP_DRAGON) info[i++] = "You can sense the presence of dragons.";
			if (p_ptr->telepathy & ESP_SPIDER) info[i++] = "You can sense the presence of spiders.";
			if (p_ptr->telepathy & ESP_GIANT) info[i++] = "You can sense the presence of giants.";
			if (p_ptr->telepathy & ESP_DEMON) info[i++] = "You can sense the presence of demons.";
			if (p_ptr->telepathy & ESP_UNDEAD) info[i++] = "You can sense presence of undead.";
			if (p_ptr->telepathy & ESP_EVIL) info[i++] = "You can sense the presence of evil beings.";
			if (p_ptr->telepathy & ESP_ANIMAL) info[i++] = "You can sense the presence of animals.";
			if (p_ptr->telepathy & ESP_DRAGONRIDER) info[i++] = "You can sense the presence of dragonriders.";
			if (p_ptr->telepathy & ESP_GOOD) info[i++] = "You can sense the presence of good beings.";
			if (p_ptr->telepathy & ESP_NONLIVING) info[i++] = "You can sense the presence of non-living things.";
			if (p_ptr->telepathy & ESP_UNIQUE) info[i++] = "You can sense the presence of unique beings.";
		}
	}
	if (p_ptr->anti_magic)	// older
	{
		info[i++] = "You are surrounded by an anti-magic shield.";
	}
#if 0
        if (p_ptr->antimagic)	// newer
	{
                info[i++] = "You are surrounded by an anti-magic field.";
	}
#endif	// 0
	if (p_ptr->hold_life)
	{
		info[i++] = "You have a firm hold on your life force.";
	}
	if (p_ptr->lite)
	{
		info[i++] = "You are carrying a permanent light.";
	}
	if (p_ptr->auto_id)
	{
//		info[i++] = "You are able to sense magic.";
		info[i++] = "You can sense magic.";
	}
#if 1
	if (p_ptr->reflect)
	{
		info[i++] = "You reflect arrows and bolts.";
	}
	if (p_ptr->sh_fire)
	{
		info[i++] = "You are surrounded with a fiery aura.";
	}
	if (p_ptr->sh_elec)
	{
		info[i++] = "You are surrounded with electricity.";
	}
	if (p_ptr->anti_tele)
	{
		info[i++] = "You cannot teleport.";
	}
#endif	// 0

	if (p_ptr->immune_acid)
	{
		info[i++] = "You are completely immune to acid.";
	}
	else if ((p_ptr->resist_acid) && (p_ptr->oppose_acid))
	{
		info[i++] = "You resist acid exceptionally well.";
	}
	else if ((p_ptr->resist_acid) || (p_ptr->oppose_acid))
	{
		info[i++] = "You are resistant to acid.";
	}

	if (p_ptr->immune_elec)
	{
		info[i++] = "You are completely immune to lightning.";
	}
	else if ((p_ptr->resist_elec) && (p_ptr->oppose_elec))
	{
		info[i++] = "You resist lightning exceptionally well.";
	}
	else if ((p_ptr->resist_elec) || (p_ptr->oppose_elec))
	{
		info[i++] = "You are resistant to lightning.";
	}

	if (p_ptr->immune_fire)
	{
		info[i++] = "You are completely immune to fire.";
	}
	else if ((p_ptr->resist_fire) && (p_ptr->oppose_fire))
	{
		info[i++] = "You resist fire exceptionally well.";
	}
	else if ((p_ptr->resist_fire) || (p_ptr->oppose_fire))
	{
		info[i++] = "You are resistant to fire.";
	}

	if (p_ptr->immune_cold)
	{
		info[i++] = "You are completely immune to cold.";
	}
	else if ((p_ptr->resist_cold) && (p_ptr->oppose_cold))
	{
		info[i++] = "You resist cold exceptionally well.";
	}
	else if ((p_ptr->resist_cold) || (p_ptr->oppose_cold))
	{
		info[i++] = "You are resistant to cold.";
	}

	if ((p_ptr->resist_pois) && (p_ptr->oppose_pois))
	{
		info[i++] = "You resist poison exceptionally well.";
	}
	else if ((p_ptr->resist_pois) || (p_ptr->oppose_pois))
	{
		info[i++] = "You are resistant to poison.";
	}

	if (p_ptr->resist_lite)
	{
		info[i++] = "You are resistant to bright light.";
	}
	if (p_ptr->resist_dark)
	{
		info[i++] = "You are resistant to darkness.";
	}
	if (p_ptr->resist_conf)
	{
		info[i++] = "You are resistant to confusion.";
	}
	if (p_ptr->resist_sound)
	{
		info[i++] = "You are resistant to sonic attacks.";
	}
	if (p_ptr->resist_disen)
	{
		info[i++] = "You are resistant to disenchantment.";
	}
	if (p_ptr->resist_chaos)
	{
		info[i++] = "You are resistant to chaos.";
	}
	if (p_ptr->resist_shard)
	{
		info[i++] = "You are resistant to blasts of shards.";
	}
	if (p_ptr->resist_nexus)
	{
		info[i++] = "You are resistant to nexus attacks.";
	}
	if (p_ptr->immune_neth)
	{
		info[i++] = "You are immune to nether forces.";
	}
	else if (p_ptr->resist_neth)
	{
		info[i++] = "You are resistant to nether forces.";
	}
	if (p_ptr->resist_fear)
	{
		info[i++] = "You are completely fearless.";
	}
	if (p_ptr->resist_blind)
	{
		info[i++] = "Your eyes are resistant to blindness.";
	}

	if (p_ptr->sustain_str)
	{
		info[i++] = "Your strength is sustained.";
	}
	if (p_ptr->sustain_int)
	{
		info[i++] = "Your intelligence is sustained.";
	}
	if (p_ptr->sustain_wis)
	{
		info[i++] = "Your wisdom is sustained.";
	}
	if (p_ptr->sustain_con)
	{
		info[i++] = "Your constitution is sustained.";
	}
	if (p_ptr->sustain_dex)
	{
		info[i++] = "Your dexterity is sustained.";
	}
	if (p_ptr->sustain_chr)
	{
		info[i++] = "Your charisma is sustained.";
	}
	if (p_ptr->black_breath)
	{
		info[i++] = "You suffer from Black Breath.";
	}


	if (f1 & TR1_STR)
	{
		info[i++] = "Your strength is affected by your equipment.";
	}
	if (f1 & TR1_INT)
	{
		info[i++] = "Your intelligence is affected by your equipment.";
	}
	if (f1 & TR1_WIS)
	{
		info[i++] = "Your wisdom is affected by your equipment.";
	}
	if (f1 & TR1_DEX)
	{
		info[i++] = "Your dexterity is affected by your equipment.";
	}
	if (f1 & TR1_CON)
	{
		info[i++] = "Your constitution is affected by your equipment.";
	}
	if (f1 & TR1_CHR)
	{
		info[i++] = "Your charisma is affected by your equipment.";
	}

	if (f1 & TR1_STEALTH)
	{
		info[i++] = "Your stealth is affected by your equipment.";
	}
	if (f1 & TR1_SEARCH)
	{
		info[i++] = "Your searching ability is affected by your equipment.";
	}
	if (f1 & TR1_INFRA)
	{
		info[i++] = "Your infravision is affected by your equipment.";
	}
	if (f1 & TR1_TUNNEL)
	{
		info[i++] = "Your digging ability is affected by your equipment.";
	}
	if (f1 & TR1_SPEED)
	{
		info[i++] = "Your speed is affected by your equipment.";
	}
	if (f1 & TR1_BLOWS)
	{
		info[i++] = "Your attack speed is affected by your equipment.";
	}
        if (f5 & (TR5_CRIT))
	{
                info[i++] = "Your ability to score critical hits is affected by your equipment.";
	}



	/* Access the current weapon */
	o_ptr = &p_ptr->inventory[INVEN_WIELD];

	/* Analyze the weapon */
	if (o_ptr->k_idx)
	{
		/* Indicate Blessing */
		if (f3 & TR3_BLESSED)
		{
			info[i++] = "Your weapon has been blessed by the gods.";
		}

		if (f5 & (TR5_CHAOTIC))
		{
                        info[i++] = "Your weapon is branded with the Sign of Chaos.";
		}

		/* Hack */
		if (f1 & TR1_IMPACT)
		{
			info[i++] = "The impact of your weapon can cause earthquakes.";
		}

		if (f1 & (TR1_VORPAL))
		{
			info[i++] = "Your weapon is very sharp.";
		}

		if (f1 & (TR1_VAMPIRIC))
		{
			info[i++] = "Your weapon drains life from your foes.";
		}

		/* Special "Attack Bonuses" */
		if (f1 & TR1_BRAND_ACID)
		{
			info[i++] = "Your weapon melts your foes.";
		}
		if (f1 & TR1_BRAND_ELEC)
		{
			info[i++] = "Your weapon shocks your foes.";
		}
		if (f1 & TR1_BRAND_FIRE)
		{
			info[i++] = "Your weapon burns your foes.";
		}
		if (f1 & TR1_BRAND_COLD)
		{
			info[i++] = "Your weapon freezes your foes.";
		}
		if (f1 & (TR1_BRAND_POIS))
		{
			info[i++] = "Your weapon poisons your foes.";
		}


		/* Special "slay" flags */
		if (f1 & TR1_SLAY_ANIMAL)
		{
			info[i++] = "Your weapon strikes at animals with extra force.";
		}
		if (f1 & TR1_SLAY_EVIL)
		{
			info[i++] = "Your weapon strikes at evil with extra force.";
		}
		if (f1 & TR1_SLAY_UNDEAD)
		{
			info[i++] = "Your weapon strikes at undead with holy wrath.";
		}
		if (f1 & TR1_SLAY_DEMON)
		{
			info[i++] = "Your weapon strikes at demons with holy wrath.";
		}
		if (f1 & TR1_SLAY_ORC)
		{
			info[i++] = "Your weapon is especially deadly against orcs.";
		}
		if (f1 & TR1_SLAY_TROLL)
		{
			info[i++] = "Your weapon is especially deadly against trolls.";
		}
		if (f1 & TR1_SLAY_GIANT)
		{
			info[i++] = "Your weapon is especially deadly against giants.";
		}
		if (f1 & TR1_SLAY_DRAGON)
		{
			info[i++] = "Your weapon is especially deadly against dragons.";
		}

		/* Special "kill" flags */
		if (f1 & TR1_KILL_DRAGON)
		{
			info[i++] = "Your weapon is a great bane of dragons.";
		}
	}
	info[i]=NULL;

	/* Let the client know to expect some info */
	Send_special_other(Ind);
}






/*
 * Forget everything
 */
bool lose_all_info(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int                 i;

	/* Forget info about objects */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &p_ptr->inventory[i];

		/* Skip non-items */
		if (!o_ptr->k_idx) continue;

		/* Allow "protection" by the MENTAL flag */
		if (o_ptr->ident & ID_MENTAL) continue;

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

		/* Hack -- Clear the "empty" flag */
		o_ptr->ident &= ~ID_EMPTY;

		/* Hack -- Clear the "known" flag */
		o_ptr->ident &= ~ID_KNOWN;

		/* Hack -- Clear the "felt" flag */
		o_ptr->ident &= ~ID_SENSE;
	}

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Mega-Hack -- Forget the map */
	wiz_dark(Ind);

	/* It worked */
	return (TRUE);
}


/*
 * Detect any treasure on the current panel		-RAK-
 *
 * We do not yet create any "hidden gold" features XXX XXX XXX
 */
bool detect_treasure(int Ind)
{
	player_type *p_ptr = Players[Ind];

#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
#else
	int Depth = p_ptr->dun_depth;
#endif

	int		y, x;
	bool	detect = FALSE;

	cave_type	*c_ptr;
	byte		*w_ptr;

	object_type	*o_ptr;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);
#endif


	/* Scan the current panel */
	for (y = p_ptr->panel_row_min; y <= p_ptr->panel_row_max; y++)
	{
		for (x = p_ptr->panel_col_min; x <= p_ptr->panel_col_max; x++)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][x];
#else
			c_ptr = &cave[Depth][y][x];
#endif
			w_ptr = &p_ptr->cave_flag[y][x];

			o_ptr = &o_list[c_ptr->o_idx];

			/* Magma/Quartz + Known Gold */
			if ((c_ptr->feat == FEAT_MAGMA_K) ||
			    (c_ptr->feat == FEAT_QUARTZ_K))
			{
				/* Notice detected gold */
				if (!(*w_ptr & CAVE_MARK))
				{
					/* Detect */
					detect = TRUE;

					/* Hack -- memorize the feature */
					*w_ptr |= CAVE_MARK;

					/* Redraw */
					lite_spot(Ind, y, x);
				}
			}

			/* Notice embedded gold */
			if ((c_ptr->feat == FEAT_MAGMA_H) ||
			    (c_ptr->feat == FEAT_QUARTZ_H))
			{
				/* Expose the gold */
				c_ptr->feat += 0x02;

				/* Detect */
				detect = TRUE;

				/* Hack -- memorize the item */
				*w_ptr |= CAVE_MARK;

				/* Redraw */
				lite_spot(Ind, y, x);
			}

			/* Notice gold */
			if (o_ptr->tval == TV_GOLD)
			{
				/* Notice new items */
				if (!(p_ptr->obj_vis[c_ptr->o_idx]))
				{
					/* Detect */
					detect = TRUE;

					/* Hack -- memorize the item */
					p_ptr->obj_vis[c_ptr->o_idx] = TRUE;

					/* Redraw */
					lite_spot(Ind, y, x);
				}
			}
		}
	}

	return (detect);
}



/*
 * Detect magic items.
 *
 * This will light up all spaces with "magic" items, including artifacts,
 * ego-items, potions, scrolls, books, rods, wands, staves, amulets, rings,
 * and "enchanted" items of the "good" variety.
 *
 * It can probably be argued that this function is now too powerful.
 */
bool detect_magic(int Ind)
{
	player_type *p_ptr = Players[Ind];

#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
#else
	int Depth = p_ptr->dun_depth;
#endif

	int		i, j, tv;
	bool	detect = FALSE;

	cave_type	*c_ptr;
	object_type	*o_ptr;

#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);
#endif

	/* Scan the current panel */
	for (i = p_ptr->panel_row_min; i <= p_ptr->panel_row_max; i++)
	{
		for (j = p_ptr->panel_col_min; j <= p_ptr->panel_col_max; j++)
		{
			/* Access the grid and object */
#ifdef NEW_DUNGEON
			c_ptr = &zcave[i][j];
#else
			c_ptr = &cave[Depth][i][j];
#endif
			o_ptr = &o_list[c_ptr->o_idx];

			/* Nothing there */
			if (!(c_ptr->o_idx)) continue;

			/* Examine the tval */
			tv = o_ptr->tval;

			/* Artifacts, misc magic items, or enchanted wearables */
			if (artifact_p(o_ptr) || ego_item_p(o_ptr) ||
			    (tv == TV_AMULET) || (tv == TV_RING) ||
			    (tv == TV_STAFF) || (tv == TV_WAND) || (tv == TV_ROD) ||
			    (tv == TV_SCROLL) || (tv == TV_POTION) ||
			    (tv == TV_MAGIC_BOOK) || (tv == TV_PRAYER_BOOK) ||
			    ((o_ptr->to_a > 0) || (o_ptr->to_h + o_ptr->to_d > 0)))
			{
				/* Note new items */
				if (!(p_ptr->obj_vis[c_ptr->o_idx]))
				{
					/* Detect */
					detect = TRUE;

					/* Memorize the item */
					p_ptr->obj_vis[c_ptr->o_idx] = TRUE;

					/* Redraw */
					lite_spot(Ind, i, j);
				}
			}
		}
	}

	/* Return result */
	return (detect);
}





/*
 * Locates and displays all invisible creatures on current panel -RAK-
 */
bool detect_invisible(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int		i;
	bool	flag = FALSE;


	/* Detect all invisible monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
                monster_race *r_ptr = race_inf(m_ptr);

		int fy = m_ptr->fy;
		int fx = m_ptr->fx;

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip visible monsters */
		if (p_ptr->mon_vis[i]) continue;

		/* Skip monsters not on this depth */
#ifdef NEW_DUNGEON
		if (!inarea(&m_ptr->wpos, &p_ptr->wpos)) continue;
#else
		if (m_ptr->dun_depth != p_ptr->dun_depth) continue;
#endif

		/* Detect all invisible monsters */
		if (panel_contains(fy, fx) && (r_ptr->flags2 & RF2_INVISIBLE))
		{
			/* Take note that they are invisible */
			r_ptr->r_flags2 |= RF2_INVISIBLE;

			/* Mega-Hack -- Show the monster */
			p_ptr->mon_vis[i] = TRUE;
			lite_spot(Ind, fy, fx);
			flag = TRUE;
		}
	}

	/* Detect all invisible players */
	for (i = 1; i <= NumPlayers; i++)
	{
		player_type *q_ptr = Players[i];

		int py = q_ptr->py;
		int px = q_ptr->px;

		/* Skip disconnected players */
		if (q_ptr->conn == NOT_CONNECTED) continue;

		/* Skip visible players */
#ifdef NEW_DUNGEON
		if (!inarea(&p_ptr->wpos, &q_ptr->wpos)) continue;
#else
		if (p_ptr->dun_depth != q_ptr->dun_depth) continue;
#endif

		/* Skip the dungeon master */
		if (p_ptr->admin_dm) continue;

		/* Detect all invisible players but not the dungeon master */
		if (panel_contains(py, px) && q_ptr->ghost) 
		{
			/* Mega-Hack -- Show the player */
			p_ptr->play_vis[i] = TRUE;
			lite_spot(Ind, py, px);
			flag = TRUE;
		}
	}

	/* Describe result, and clean up */
	if (flag)
	{
		/* Describe, and wait for acknowledgement */
		msg_print(Ind, "You sense the presence of invisible creatures!");
		msg_print(Ind, NULL);

		/* Wait */
		Send_pause(Ind);

		/* Mega-Hack -- Fix the monsters and players */
		update_monsters(FALSE);
		update_players();
	}
	else
	{
        msg_print(Ind, "You sense the absence of invisible creatures.");
        msg_print(Ind, NULL);
	}

	/* Result */
	return (flag);
}



/*
 * Display evil creatures on current panel		-RAK-
 */
bool detect_evil(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int		i;
	bool	flag = FALSE;


	/* Display all the evil monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
                monster_race *r_ptr = race_inf(m_ptr);

		int fy = m_ptr->fy;
		int fx = m_ptr->fx;

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip visible monsters */
		if (p_ptr->mon_vis[i]) continue;

		/* Skip monsters not on this depth */
#ifdef NEW_DUNGEON
		if (!inarea(&m_ptr->wpos, &p_ptr->wpos)) continue;
#else
		if (m_ptr->dun_depth != p_ptr->dun_depth) continue;
#endif

		/* Detect evil monsters */
		if (panel_contains(fy, fx) && (r_ptr->flags3 & RF3_EVIL))
		{
			/* Mega-Hack -- Show the monster */
			p_ptr->mon_vis[i] = TRUE;
			lite_spot(Ind, fy, fx);
			flag = TRUE;
		}
	}

	/* Note effects and clean up */
	if (flag)
	{
		/* Describe, and wait for acknowledgement */
		msg_print(Ind, "You sense the presence of evil!");
		msg_print(Ind, NULL);

		/* Wait */
		Send_pause(Ind);

		/* Mega-Hack -- Fix the monsters */
		update_monsters(FALSE);
	}
    else
    {
        msg_print(Ind, "You sense the absence of evil.");
        msg_print(Ind, NULL);
    }

	/* Result */
	return (flag);
}



/*
 * Display all non-invisible monsters/players on the current panel
 */
bool detect_creatures(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int		i;
	bool	flag = FALSE;


	/* Detect non-invisible monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
                monster_race *r_ptr = race_inf(m_ptr);

		int fy = m_ptr->fy;
		int fx = m_ptr->fx;

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip visible monsters */
		if (p_ptr->mon_vis[i]) continue;

		/* Skip monsters not on this depth */
#ifdef NEW_DUNGEON
		if (!inarea(&m_ptr->wpos, &p_ptr->wpos)) continue;
#else
		if (m_ptr->dun_depth != p_ptr->dun_depth) continue;
#endif

		/* Detect all non-invisible monsters */
		if (panel_contains(fy, fx) && (!(r_ptr->flags2 & RF2_INVISIBLE)))
		{
			/* Mega-Hack -- Show the monster */
			p_ptr->mon_vis[i] = TRUE;
			lite_spot(Ind, fy, fx);
			flag = TRUE;
		}
	}

	/* Detect non-invisible players */
	for (i = 1; i <= NumPlayers; i++)
	{
		player_type *q_ptr = Players[i];

		int py = q_ptr->py;
		int px = q_ptr->px;

		/* Skip disconnected players */
		if (q_ptr->conn == NOT_CONNECTED) continue;

		/* Skip visible players */
		if (p_ptr->play_vis[i]) continue;

		/* Skip players not on this depth */
#ifdef NEW_DUNGEON
		if (!inarea(&p_ptr->wpos, &q_ptr->wpos)) continue;
#else
		if (p_ptr->dun_depth != q_ptr->dun_depth) continue;
#endif

		/* Skip ourself */
		if (i == Ind) continue;

		/* Detect all non-invisible players */
		if (panel_contains(py, px) && !q_ptr->ghost)
		{
			/* Mega-Hack -- Show the player */
			p_ptr->play_vis[i] = TRUE;
			lite_spot(Ind, py, px);
			flag = TRUE;
		}
	}

	/* Describe and clean up */
	if (flag)
	{
		/* Describe, and wait for acknowledgement */
		msg_print(Ind, "You sense the presence of creatures!");
		msg_print(Ind, NULL);

		/* Wait */
		Send_pause(Ind);

		/* Mega-Hack -- Fix the monsters and players */
		update_monsters(FALSE);
		update_players();
	}
    else
    {
        msg_print(Ind, "You sense the absence of creatures.");
        msg_print(Ind, NULL);
    }

	/* Result */
	return (flag);
}


/*
 * Detect everything
 */
bool detection(int Ind)
{
	bool	detect = FALSE;

	/* Detect the easy things */
	if (detect_treasure(Ind)) detect = TRUE;
	if (detect_object(Ind)) detect = TRUE;
	if (detect_trap(Ind)) detect = TRUE;
	if (detect_sdoor(Ind)) detect = TRUE;
	if (detect_creatures(Ind)) detect = TRUE;

	/* Result */
	return (detect);
}


/*
 * Detect all objects on the current panel		-RAK-
 */
bool detect_object(int Ind)
{
	player_type *p_ptr = Players[Ind];

#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
#else
	int Depth = p_ptr->dun_depth;
#endif

	int		i, j;
	bool	detect = FALSE;

	cave_type	*c_ptr;

	object_type	*o_ptr;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);
#endif


	/* Scan the current panel */
	for (i = p_ptr->panel_row_min; i <= p_ptr->panel_row_max; i++)
	{
		for (j = p_ptr->panel_col_min; j <= p_ptr->panel_col_max; j++)
		{
#ifdef NEW_DUNGEON
			c_ptr = &zcave[i][j];
#else
			c_ptr = &cave[Depth][i][j];
#endif

			o_ptr = &o_list[c_ptr->o_idx];

			/* Nothing here */
			if (!(c_ptr->o_idx)) continue;

			/* Do not detect "gold" */
			if (o_ptr->tval == TV_GOLD) continue;

			/* Note new objects */
			if (!(p_ptr->obj_vis[c_ptr->o_idx]))
			{
				/* Detect */
				detect = TRUE;

				/* Hack -- memorize it */
				p_ptr->obj_vis[c_ptr->o_idx] = TRUE;

				/* Redraw */
				lite_spot(Ind, i, j);
			}
		}
	}

	return (detect);
}


/*
 * Locates and displays traps on current panel
 */
bool detect_trap(int Ind)
{
	player_type *p_ptr = Players[Ind];

#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
#else
	int Depth = p_ptr->dun_depth;
#endif

	int		i, j, chance;

	bool	detect = FALSE;

	cave_type  *c_ptr;
	byte *w_ptr;
	trap_type	*t_ptr;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);
#endif

	chance = (p_ptr->pclass == CLASS_ROGUE ? 75 : 50) + p_ptr->lev / 4;

	/* Scan the current panel */
	for (i = p_ptr->panel_row_min; i <= p_ptr->panel_row_max; i++)
	{
		for (j = p_ptr->panel_col_min; j <= p_ptr->panel_col_max; j++)
		{
			/* Access the grid */
#ifdef NEW_DUNGEON
			c_ptr = &zcave[i][j];
#else
			c_ptr = &cave[Depth][i][j];
#endif
			w_ptr = &p_ptr->cave_flag[i][j];

			/* Detect invisible traps */
			//			if (c_ptr->feat == FEAT_INVIS)
			if (c_ptr->special.type == CS_TRAPS)
			{
				t_ptr = c_ptr->special.ptr;

				if (!t_ptr->found)
				{
					/* Hack -- check for failure */
					if (rand_int(100) > chance) continue;

					/* Pick a trap */
#ifdef NEW_DUNGEON
					pick_trap(wpos, i, j);
#else
					pick_trap(Depth, i, j);
#endif
				}

					/* Hack -- memorize it */
					*w_ptr |= CAVE_MARK;

					/* Redraw */
					lite_spot(Ind, i, j);

					/* Obvious */
					detect = TRUE;
#if 0
				/* Already seen traps */
				else if (c_ptr->feat >= FEAT_TRAP_HEAD && c_ptr->feat <= FEAT_TRAP_TAIL)
				{
					/* Memorize it */
					*w_ptr |= CAVE_MARK;

					/* Redraw */
					lite_spot(Ind, i, j);

					/* Obvious */
					detect = TRUE;
				}
#endif	// 0
			}
		}
	}

	return (detect);
}



/*
 * Locates and displays all stairs and secret doors on current panel -RAK-
 */
bool detect_sdoor(int Ind)
{
	player_type *p_ptr = Players[Ind];

#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
#else
	int Depth = p_ptr->dun_depth;
#endif

	int		i, j;
	bool	detect = FALSE;

	cave_type *c_ptr;
	byte *w_ptr;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);
#endif


	/* Scan the panel */
	for (i = p_ptr->panel_row_min; i <= p_ptr->panel_row_max; i++)
	{
		for (j = p_ptr->panel_col_min; j <= p_ptr->panel_col_max; j++)
		{
			/* Access the grid and object */
#ifdef NEW_DUNGEON
			c_ptr = &zcave[i][j];
#else
			c_ptr = &cave[Depth][i][j];
#endif
			w_ptr = &p_ptr->cave_flag[i][j];

			/* Hack -- detect secret doors */
			if (c_ptr->feat == FEAT_SECRET)
			{
				/* Find the door XXX XXX XXX */
				c_ptr->feat = FEAT_DOOR_HEAD + 0x00;

				/* Memorize the door */
				*w_ptr |= CAVE_MARK;

				/* Redraw */
				lite_spot(Ind, i, j);

				/* Obvious */
				detect = TRUE;
			}

			/* Ignore known grids */
			if (*w_ptr & CAVE_MARK) continue;

			/* Hack -- detect stairs */
			if ((c_ptr->feat == FEAT_LESS) ||
			    (c_ptr->feat == FEAT_MORE))
			{
				/* Memorize the stairs */
				*w_ptr |= CAVE_MARK;

				/* Redraw */
				lite_spot(Ind, i, j);

				/* Obvious */
				detect = TRUE;
			}
		}
	}

	return (detect);
}


/*
 * Create stairs at the player location
 */
void stair_creation(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* Access the grid */
	cave_type *c_ptr;

#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#else
	int Depth = p_ptr->dun_depth;
#endif

	/* Access the player grid */
#ifdef NEW_DUNGEON
	c_ptr = &zcave[p_ptr->py][p_ptr->px];
#else
	c_ptr = &cave[Depth][p_ptr->py][p_ptr->px];
#endif

	/* XXX XXX XXX */
#ifdef NEW_DUNGEON
	if (!cave_valid_bold(zcave, p_ptr->py, p_ptr->px))
#else
	if (!cave_valid_bold(Depth, p_ptr->py, p_ptr->px))
#endif
	{
		msg_print(Ind, "The object resists the spell.");
		return;
	}

	/* Hack -- Delete old contents */
#ifdef NEW_DUNGEON
	delete_object(wpos, p_ptr->py, p_ptr->px);
#else
	delete_object(Depth, p_ptr->py, p_ptr->px);
#endif

	/* Create a staircase */
#ifdef NEW_DUNGEON
	if (can_go_down(wpos) && !can_go_up(wpos))
#else
	if (!Depth)
#endif
	{
		c_ptr->feat = FEAT_MORE;
	}
#ifdef NEW_DUNGEON
	else if(can_go_up(wpos) && !can_go_down(wpos))
#else
	else if (is_quest(Depth) || (Depth >= MAX_DEPTH-1))
#endif
	{
		c_ptr->feat = FEAT_LESS;
	}
	else if (rand_int(100) < 50)
	{
		c_ptr->feat = FEAT_MORE;
	}
	else
	{
		c_ptr->feat = FEAT_LESS;
	}

	/* Notice */
	note_spot(Ind, p_ptr->py, p_ptr->px);

#ifdef NEW_DUNGEON
	/* Redraw */
	everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);
#else
	/* Redraw */
	everyone_lite_spot(Depth, p_ptr->py, p_ptr->px);
#endif
}




/*
 * Hook to specify "weapon"
 */
static bool item_tester_hook_weapon(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_SWORD:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_DIGGING:
		case TV_BOW:
		case TV_BOLT:
		case TV_ARROW:
		case TV_SHOT:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Hook to specify "armour"
 */
static bool item_tester_hook_armour(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_DRAG_ARMOR:
		case TV_HARD_ARMOR:
		case TV_SOFT_ARMOR:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_CROWN:
		case TV_HELM:
		case TV_BOOTS:
		case TV_GLOVES:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}



/*
 * Enchants a plus onto an item.                        -RAK-
 *
 * Revamped!  Now takes item pointer, number of times to try enchanting,
 * and a flag of what to try enchanting.  Artifacts resist enchantment
 * some of the time, and successful enchantment to at least +0 might
 * break a curse on the item.  -CFT
 *
 * Note that an item can technically be enchanted all the way to +15 if
 * you wait a very, very, long time.  Going from +9 to +10 only works
 * about 5% of the time, and from +10 to +11 only about 1% of the time.
 *
 * Note that this function can now be used on "piles" of items, and
 * the larger the pile, the lower the chance of success.
 */
bool enchant(int Ind, object_type *o_ptr, int n, int eflag)
{
	player_type *p_ptr = Players[Ind];

	int i, chance, prob;

	bool res = FALSE;

	bool a = artifact_p(o_ptr);

	    u32b f1, f2, f3, f4, f5, esp;

			  /* Extract the flags */
			  object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	/* Large piles resist enchantment */
	prob = o_ptr->number * 100;

	/* Missiles are easy to enchant */
	if ((o_ptr->tval == TV_BOLT) ||
	    (o_ptr->tval == TV_ARROW) ||
	    (o_ptr->tval == TV_SHOT))
	{
		prob = prob / 20;
	}

	/* Try "n" times */
	for (i=0; i<n; i++)
	{
		/* Hack -- Roll for pile resistance */
		if (rand_int(prob) >= 100) continue;

		/* Enchant to hit */
		if (eflag & ENCH_TOHIT)
		{
			if (o_ptr->to_h < 0) chance = 0;
			else if (o_ptr->to_h > 15) chance = 1000;
			else chance = enchant_table[o_ptr->to_h];

			if ((randint(1000) > chance) && (!a || (rand_int(100) < 50)))
			{
				o_ptr->to_h++;
				res = TRUE;

				/* only when you get it above -1 -CFT */
				if (cursed_p(o_ptr) &&
				    (!(f3 & TR3_PERMA_CURSE)) &&
				    (o_ptr->to_h >= 0) && (rand_int(100) < 25))
				{
					msg_print(Ind, "The curse is broken!");
					o_ptr->ident &= ~ID_CURSED;
					o_ptr->ident |= ID_SENSE;
					o_ptr->note = quark_add("uncursed");
				}
			}
		}

		/* Enchant to damage */
		if (eflag & ENCH_TODAM)
		{
			if (o_ptr->to_d < 0) chance = 0;
			else if (o_ptr->to_d > 15) chance = 1000;
			else chance = enchant_table[o_ptr->to_d];

			if ((randint(1000) > chance) && (!a || (rand_int(100) < 50)))
			{
				o_ptr->to_d++;
				res = TRUE;

				/* only when you get it above -1 -CFT */
				if (cursed_p(o_ptr) &&
				    (!(f3 & TR3_PERMA_CURSE)) &&
				    (o_ptr->to_d >= 0) && (rand_int(100) < 25))
				{
					msg_print(Ind, "The curse is broken!");
					o_ptr->ident &= ~ID_CURSED;
					o_ptr->ident |= ID_SENSE;
					o_ptr->note = quark_add("uncursed");
				}
			}
		}

		/* Enchant to armor class */
		if (eflag & ENCH_TOAC)
		{
			if (o_ptr->to_a < 0) chance = 0;
			else if (o_ptr->to_a > 15) chance = 1000;
			else chance = enchant_table[o_ptr->to_a];

			if ((randint(1000) > chance) && (!a || (rand_int(100) < 50)))
			{
				o_ptr->to_a++;
				res = TRUE;

				/* only when you get it above -1 -CFT */
				if (cursed_p(o_ptr) &&
				    (!(f3 & TR3_PERMA_CURSE)) &&
				    (o_ptr->to_a >= 0) && (rand_int(100) < 25))
				{
					msg_print(Ind, "The curse is broken!");
					o_ptr->ident &= ~ID_CURSED;
					o_ptr->ident |= ID_SENSE;
					o_ptr->note = quark_add("uncursed");
				}
			}
		}
	}

	/* Failure */
	if (!res) return (FALSE);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Resend plusses */
	p_ptr->redraw |= (PR_PLUSSES);

	/* Success */
	return (TRUE);
}

bool create_artifact(int Ind)
{
  player_type *p_ptr = Players[Ind];

  p_ptr->current_artifact = TRUE;
  get_item(Ind);

  return TRUE;
}

bool create_artifact_aux(int Ind, int item)
{
  player_type *p_ptr = Players[Ind];

  object_type *o_ptr;
  char o_name[160];

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}


	if (o_ptr->number > 1) return FALSE;
	if (o_ptr->name1) return FALSE;
	
	/* Description */
	object_desc(Ind, o_name, o_ptr, FALSE, 0);

	/* Describe */
	msg_format(Ind, "%s %s glow%s brightly!",
	           ((item >= 0) ? "Your" : "The"), o_name,
	           ((o_ptr->number > 1) ? "" : "s"));

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

			return FALSE;
		}

#ifdef NEW_DUNGEON
		apply_magic(&p_ptr->wpos, o_ptr, p_ptr->lev, FALSE, FALSE, FALSE);
#else
		apply_magic(p_ptr->dun_depth, o_ptr, p_ptr->lev, FALSE, FALSE, FALSE);
#endif

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);


		p_ptr->current_artifact = FALSE;
		return TRUE;
}

bool curse_spell(int Ind){	// could be void
	player_type *p_ptr=Players[Ind];
	get_item(Ind);
	p_ptr->current_curse=TRUE;	/* This is awful. I intend to change it */
	return(TRUE);
}

bool curse_spell_aux(int Ind, int item){
	player_type *p_ptr=Players[Ind];
	object_type *o_ptr=&p_ptr->inventory[item];
	char		o_name[160];

	p_ptr->current_curse=FALSE;
	object_desc(Ind, o_name, o_ptr, FALSE, 0);


	if(artifact_p(o_ptr) && (randint(10)<8)){
		msg_format(Ind,"The artifact resists your attempts.");
		return(FALSE);
	}
	if(item_tester_hook_weapon(o_ptr)){
		o_ptr->to_h=0-randint(10);
		o_ptr->to_d=0-randint(10);
	}
	else if(item_tester_hook_armour(o_ptr)){
		o_ptr->to_a=0-randint(10);
	}
	else{
		msg_format(Ind,"You cannot curse that item!");
		return(FALSE);
	}

	msg_format(Ind,"A terrible black aura surrounds your %s",o_name,o_ptr->number>1 ? "" : "s");
	/* except it doesnt actually get cursed properly yet. */
	o_ptr->name1=0;
	o_ptr->name3=0;
	o_ptr->ident|=ID_CURSED;
	o_ptr->ident&=~ID_KNOWN|ID_SENSE;	/* without this, the spell is pointless */

	if(o_ptr->name2)
		o_ptr->pval=0-randint(10);	/* nasty */

	/* Recalculate the bonuses - if stupid enough to curse worn item ;) */
	p_ptr->update |= (PU_BONUS);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);

	return(TRUE);
}

bool enchant_spell(int Ind, int num_hit, int num_dam, int num_ac)
{
	player_type *p_ptr = Players[Ind];

	get_item(Ind);

	p_ptr->current_enchant_h = num_hit;
	p_ptr->current_enchant_d = num_dam;
	p_ptr->current_enchant_a = num_ac;

	return (TRUE);
}
	
/*
 * Enchant an item (in the inventory or on the floor)
 * Note that "num_ac" requires armour, else weapon
 * Returns TRUE if attempted, FALSE if cancelled
 */
bool enchant_spell_aux(int Ind, int item, int num_hit, int num_dam, int num_ac)
{
	player_type *p_ptr = Players[Ind];

	bool		okay = FALSE;

	object_type		*o_ptr;

	char		o_name[160];

	/* Assume enchant weapon */
	item_tester_hook = item_tester_hook_weapon;

	/* Enchant armor if requested */
 	if (num_ac) item_tester_hook = item_tester_hook_armour;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}


	if (!item_tester_hook(o_ptr))
	{
		msg_print(Ind, "Sorry, you cannot enchant that item.");
		get_item(Ind);
		return (FALSE);
	}

	/* Description */
	object_desc(Ind, o_name, o_ptr, FALSE, 0);

	/* Describe */
	msg_format(Ind, "%s %s glow%s brightly!",
	           ((item >= 0) ? "Your" : "The"), o_name,
	           ((o_ptr->number > 1) ? "" : "s"));

	/* Enchant */
	if (enchant(Ind, o_ptr, num_hit, ENCH_TOHIT)) okay = TRUE;
	if (enchant(Ind, o_ptr, num_dam, ENCH_TODAM)) okay = TRUE;
	if (enchant(Ind, o_ptr, num_ac, ENCH_TOAC)) okay = TRUE;

	/* Failure */
	if (!okay)
	{
		/* Flush */
		/*if (flush_failure) flush();*/

		/* Message */
		msg_print(Ind, "The enchantment failed.");
	}

	p_ptr->current_enchant_h = -1;
	p_ptr->current_enchant_d = -1;
	p_ptr->current_enchant_a = -1;

	/* Something happened */
	return (TRUE);
}


bool ident_spell(int Ind)
{
	player_type *p_ptr = Players[Ind];

	get_item(Ind);

	p_ptr->current_identify = 1;

	return TRUE;
}

/*
 * Identify an object in the inventory (or on the floor)
 * This routine does *not* automatically combine objects.
 * Returns TRUE if something was identified, else FALSE.
 */
bool ident_spell_aux(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];

	object_type		*o_ptr;

	char		o_name[160];


	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}


	/* Identify it fully */
	object_aware(Ind, o_ptr);
	object_known(o_ptr);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Description */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);

	/* Describe */
	if (item >= INVEN_WIELD)
	{
		msg_format(Ind, "%^s: %s (%c).",
		           describe_use(Ind, item), o_name, index_to_label(item));
	}
	else if (item >= 0)
	{
		msg_format(Ind, "In your pack: %s (%c).",
		           o_name, index_to_label(item));
	}
	else
	{
		msg_format(Ind, "On the ground: %s.",
		           o_name);
	}

	p_ptr->current_identify = 0;

	/* Something happened */
	return (TRUE);
}


bool identify_fully(int Ind)
{
	player_type *p_ptr = Players[Ind];

	get_item(Ind);

	p_ptr->current_star_identify = 1;

	return TRUE;
}


/*
 * Fully "identify" an object in the inventory	-BEN-
 * This routine returns TRUE if an item was identified.
 */
bool identify_fully_item(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];

	object_type		*o_ptr;

	char		o_name[160];


	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}


	/* Identify it fully */
	object_aware(Ind, o_ptr);
	object_known(o_ptr);

	/* Mark the item as fully known */
	o_ptr->ident |= (ID_MENTAL);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Description */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);

	/* Describe */
	if (item >= INVEN_WIELD)
	{
		msg_format(Ind, "%^s: %s (%c).",
		           describe_use(Ind, item), o_name, index_to_label(item));
	}
	else if (item >= 0)
	{
		msg_format(Ind, "In your pack: %s (%c).",
		           o_name, index_to_label(item));
	}
	else
	{
		msg_format(Ind, "On the ground: %s.",
		           o_name);
	}

	/* Describe it fully */
	identify_fully_aux(Ind, o_ptr);

	/* We no longer have a *identify* in progress */
	p_ptr->current_star_identify = 0;

	/* Success */
	return (TRUE);
}




/*
 * Hook for "get_item()".  Determine if something is rechargable.
 */
static bool item_tester_hook_recharge(object_type *o_ptr)
{
	u32b f1, f2, f3, f4, f5, esp;

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	/* Some objects cannot be recharged */
	if (f4 & TR4_NO_RECHARGE) return (FALSE);

	/* Recharge staffs */
	if (o_ptr->tval == TV_STAFF) return (TRUE);

	/* Recharge wands */
	if (o_ptr->tval == TV_WAND) return (TRUE);

	/* Hack -- Recharge rods */
	if (o_ptr->tval == TV_ROD) return (TRUE);

	/* Nope */
	return (FALSE);
}


bool recharge(int Ind, int num)
{
	player_type *p_ptr = Players[Ind];

	get_item(Ind);

	p_ptr->current_recharge = num;

	return TRUE;
}


/*
 * Recharge a wand/staff/rod from the pack or on the floor.
 *
 * Mage -- Recharge I --> recharge(5)
 * Mage -- Recharge II --> recharge(40)
 * Mage -- Recharge III --> recharge(100)
 *
 * Priest -- Recharge --> recharge(15)
 *
 * Scroll of recharging --> recharge(60)
 *
 * recharge(20) = 1/6 failure for empty 10th level wand
 * recharge(60) = 1/10 failure for empty 10th level wand
 *
 * It is harder to recharge high level, and highly charged wands.
 *
 * XXX XXX XXX Beware of "sliding index errors".
 *
 * Should probably not "destroy" over-charged items, unless we
 * "replace" them by, say, a broken stick or some such.  The only
 * reason this is okay is because "scrolls of recharging" appear
 * BEFORE all staffs/wands/rods in the inventory.  Note that the
 * new "auto_sort_pack" option would correctly handle replacing
 * the "broken" wand with any other item (i.e. a broken stick).
 *
 * XXX XXX XXX Perhaps we should auto-unstack recharging stacks.
 */
bool recharge_aux(int Ind, int item, int num)
{
	player_type *p_ptr = Players[Ind];

	int                 i, t, lev;

	object_type		*o_ptr;


	/* Only accept legal items */
	item_tester_hook = item_tester_hook_recharge;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	if (!item_tester_hook(o_ptr))
	{
		msg_print(Ind, "You cannot recharge that item.");
		get_item(Ind);
		return (FALSE);
	}

	/* Extract the object "level" */
	lev = k_info[o_ptr->k_idx].level;

	/* Recharge a rod */
	if (o_ptr->tval == TV_ROD)
	{
		/* Extract a recharge power */
		i = (100 - lev + num) / 5;

		/* Paranoia -- prevent crashes */
		if (i < 1) i = 1;

		/* Back-fire */
		if (rand_int(i) == 0)
		{
			/* Hack -- backfire */
			msg_print(Ind, "The recharge backfires, draining the rod further!");

			/* Hack -- decharge the rod */
			if (o_ptr->pval < 10000) o_ptr->pval = (o_ptr->pval + 100) * 2;
		}

		/* Recharge */
		else
		{
			/* Rechange amount */
			t = (num * damroll(2, 4));

			/* Recharge by that amount */
			if (o_ptr->pval > t)
			{
				o_ptr->pval -= t;
			}

			/* Fully recharged */
			else
			{
				o_ptr->pval = 0;
			}
		}
	}

	/* Recharge wand/staff */
	else
	{
		/* Recharge power */
		i = (num + 100 - lev - (10 * o_ptr->pval)) / 15;

		/* Paranoia -- prevent crashes */
		if (i < 1) i = 1;

		/* Back-fire XXX XXX XXX */
		if (rand_int(i) == 0)
		{
			/* Dangerous Hack -- Destroy the item */
			msg_print(Ind, "There is a bright flash of light.");

			/* Reduce and describe inventory */
			if (item >= 0)
			{
				inven_item_increase(Ind, item, -999);
				inven_item_describe(Ind, item);
				inven_item_optimize(Ind, item);
			}

			/* Reduce and describe floor item */
			else
			{
				floor_item_increase(0 - item, -999);
				floor_item_describe(0 - item);
				floor_item_optimize(0 - item);
			}
		}

		/* Recharge */
		else
		{
			/* Extract a "power" */
			t = (num / (lev + 2)) + 1;

			/* Recharge based on the power */
			if (t > 0) o_ptr->pval += 2 + randint(t);

			/* Hack -- we no longer "know" the item */
			o_ptr->ident &= ~ID_KNOWN;

			/* Hack -- we no longer think the item is empty */
			o_ptr->ident &= ~ID_EMPTY;
		}
	}

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);

	/* We no longer have a recharge in progress */
	p_ptr->current_recharge = 0;

	/* Something was done */
	return (TRUE);
}








/*
 * Apply a "project()" directly to all viewable monsters
 */
bool project_hack(int Ind, int typ, int dam)
{
	player_type *p_ptr = Players[Ind];

#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
#else
	int Depth = p_ptr->dun_depth;
#endif

	int		i, x, y;

	int		flg = PROJECT_JUMP | PROJECT_KILL | PROJECT_HIDE;

	bool	obvious = FALSE;


	/* Affect all (nearby) monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Location */
		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Skip monsters not on this depth */
#ifdef NEW_DUNGEON
		if(!inarea(wpos, &m_ptr->wpos)) continue;
#else
		if (Depth != m_ptr->dun_depth) continue;
#endif

		/* Require line of sight */
		if (!player_has_los_bold(Ind, y, x)) continue;

		/* Jump directly to the target monster */
#ifdef NEW_DUNGEON
		if (project(0 - Ind, 0, wpos, y, x, dam, typ, flg)) obvious = TRUE;
#else
		if (project(0 - Ind, 0, Depth, y, x, dam, typ, flg)) obvious = TRUE;
#endif
	}

	/* Result */
	return (obvious);
}


/*
 * Speed monsters
 */
bool speed_monsters(int Ind)
{
	player_type *p_ptr = Players[Ind];

	return (project_hack(Ind, GF_OLD_SPEED, p_ptr->lev));
}

/*
 * Slow monsters
 */
bool slow_monsters(int Ind)
{
	player_type *p_ptr = Players[Ind];

	return (project_hack(Ind, GF_OLD_SLOW, p_ptr->lev));
}

/*
 * Sleep monsters
 */
bool sleep_monsters(int Ind)
{
	player_type *p_ptr = Players[Ind];

	return (project_hack(Ind, GF_OLD_SLEEP, p_ptr->lev));
}

/*
 * Fear monsters
 */
bool fear_monsters(int Ind)
{
	player_type *p_ptr = Players[Ind];

	return (project_hack(Ind, GF_TURN_ALL, p_ptr->lev));
}

/*
 * Stun monsters
 */
bool stun_monsters(int Ind)
{
	player_type *p_ptr = Players[Ind];

	return (project_hack(Ind, GF_STUN, p_ptr->lev));
}


/*
 * Banish evil monsters
 */
bool banish_evil(int Ind, int dist)
{
	return (project_hack(Ind, GF_AWAY_EVIL, dist));
}


/*
 * Turn undead
 */
bool turn_undead(int Ind)
{
	player_type *p_ptr = Players[Ind];

	return (project_hack(Ind, GF_TURN_UNDEAD, p_ptr->lev));
}

/*
 * Turn everyone
 */
bool turn_monsters(int Ind, int dam)
{
	player_type *p_ptr = Players[Ind];
	return (project_hack(Ind, GF_TURN_ALL, dam));
}


/*
 * Dispel undead monsters
 */
bool dispel_undead(int Ind, int dam)
{
	return (project_hack(Ind, GF_DISP_UNDEAD, dam));
}

/*
 * Dispel evil monsters
 */
bool dispel_evil(int Ind, int dam)
{
	return (project_hack(Ind, GF_DISP_EVIL, dam));
}

/*
 * Dispel all monsters
 */
bool dispel_monsters(int Ind, int dam)
{
	return (project_hack(Ind, GF_DISP_ALL, dam));
}




/*
 * Wake up all monsters, and speed up "los" monsters.
 */
void aggravate_monsters(int Ind, int who)
{
	player_type *p_ptr = Players[Ind];

	int i;

	bool sleep = FALSE;
	bool speed = FALSE;

	/* Aggravate everyone nearby */
	for (i = 1; i < m_max; i++)
	{
		monster_type	*m_ptr = &m_list[i];
                monster_race    *r_ptr = race_inf(m_ptr);

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip monsters not on this depth */
#ifdef NEW_DUNGEON
		if(!inarea(&p_ptr->wpos, &m_ptr->wpos)) continue;
#else
		if (p_ptr->dun_depth != m_ptr->dun_depth) continue;
#endif

		/* Skip aggravating monster (or player) */
		if (i == who) continue;

		/* Wake up nearby sleeping monsters */
		if (m_ptr->cdis < MAX_SIGHT * 2)
		{
			/* Wake up */
			if (m_ptr->csleep)
			{
				/* Wake up */
				m_ptr->csleep = 0;
				sleep = TRUE;
			}
		}

		/* Speed up monsters in line of sight */
		if (player_has_los_bold(Ind, m_ptr->fy, m_ptr->fx))
		{
			/* Speed up (instantly) to racial base + 10 */
                        if (m_ptr->mspeed < m_ptr->speed + 10)
			{
				/* Speed up */
                                m_ptr->mspeed = m_ptr->speed + 10;
				speed = TRUE;
			}
		}
	}

	/* Messages */
	if (speed) msg_print(Ind, "You feel a sudden stirring nearby!");
	else if (sleep) msg_print(Ind, "You hear a sudden stirring in the distance!");
}



/*
 * Delete all non-unique monsters of a given "type" from the level
 *
 * This is different from normal Angband now -- the closest non-unique
 * monster is chosen as the designed character to genocide.
 */
bool genocide(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int		i;

	char	typ;

	bool	result = FALSE;

	/* int		msec = delay_factor * delay_factor * delay_factor; */

	int d = 999, tmp;

	/* Search all monsters and find the closest */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
                monster_race *r_ptr = race_inf(m_ptr);

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Hack -- Skip Unique Monsters */
		if (r_ptr->flags1 & RF1_UNIQUE) continue;

		/* Skip monsters not on this depth */
#ifdef NEW_DUNGEON
		if(!inarea(&p_ptr->wpos, &m_ptr->wpos)) continue;
#else
		if (p_ptr->dun_depth != m_ptr->dun_depth) continue;
#endif

		/* Check distance */
		if ((tmp = distance(p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx)) < d)
		{
			/* Set closest distance */
			d = tmp;

			/* Set char */
			typ = r_ptr->d_char;
		}
	}

	/* Check to make sure we found a monster */
	if (d == 999)
	{
		return FALSE;
	}

	/* Delete the monsters of that "type" */
	for (i = 1; i < m_max; i++)
	{
		monster_type	*m_ptr = &m_list[i];
                monster_race    *r_ptr = race_inf(m_ptr);

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Hack -- Skip Unique Monsters */
		if (r_ptr->flags1 & RF1_UNIQUE) continue;

		/* Skip "wrong" monsters */
		if (r_ptr->d_char != typ) continue;

		/* Skip monsters not on this depth */
#ifdef NEW_DUNGEON
		if(!inarea(&p_ptr->wpos, &m_ptr->wpos)) continue;
#else
		if (p_ptr->dun_depth != m_ptr->dun_depth) continue;
#endif

		/* Delete the monster */
                if (!(r_ptr->flags9 & RF9_IM_TELE)) delete_monster_idx(i);
                else continue;

		/* Take damage */
		take_hit(Ind, randint(4), "the strain of casting Genocide");

		/* Redraw */
		p_ptr->redraw |= (PR_HP);

		/* Window stuff */
		/* p_ptr->window |= (PW_PLAYER); */

		/* Handle */
		handle_stuff(Ind);

		/* Fresh */
		/* Term_fresh(); */

		/* Delay */
		Send_flush(Ind);

		/* Take note */
		result = TRUE;
	}

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	/* Handle */
	handle_stuff(Ind);

	return (result);
}


/*
 * Delete all nearby (non-unique) monsters
 */
bool mass_genocide(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int		i;

	bool	result = FALSE;

	/*int		msec = delay_factor * delay_factor * delay_factor;*/


	/* Delete the (nearby) monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type	*m_ptr = &m_list[i];
                monster_race    *r_ptr = race_inf(m_ptr);

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip monsters not on this depth */
#ifdef NEW_DUNGEON
		if(!inarea(&p_ptr->wpos, &m_ptr->wpos)) continue;
#else
		if (p_ptr->dun_depth != m_ptr->dun_depth) continue;
#endif

		/* Hack -- Skip unique monsters */
		if (r_ptr->flags1 & RF1_UNIQUE) continue;

		/* Skip distant monsters */
		if (m_ptr->cdis > MAX_SIGHT) continue;

		/* Delete the monster */
                if (!(r_ptr->flags9 & RF9_IM_TELE)) delete_monster_idx(i);
                else continue;

		/* Hack -- visual feedback */
		/* does not effect the dungeon master, because it disturbs his movement
		 */
		if (!p_ptr->admin_dm)
			take_hit(Ind, randint(3), "the strain of casting Mass Genocide");

		/* Redraw */
		p_ptr->redraw |= (PR_HP);

		/* Window stuff */
		/* p_ptr->window |= (PW_PLAYER); */

		/* Handle */
		handle_stuff(Ind);

		/* Fresh */
		/*Term_fresh();*/

		/* Delay */
		Send_flush(Ind);

		/* Note effect */
		result = TRUE;
	}

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	/* Handle */
	handle_stuff(Ind);

	return (result);
}



/*
 * Probe nearby monsters
 */
bool probing(int Ind)
{
	player_type *p_ptr = Players[Ind];

#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
#else
	int Depth = p_ptr->dun_depth;
#endif

	int            i;

	bool	probe = FALSE;


	/* Probe all (nearby) monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip monsters not on this depth */
#ifdef NEW_DUNGEON
		if (!inarea(&m_ptr->wpos, wpos)) continue;
#else
		if (Depth != m_ptr->dun_depth) continue;
#endif

		/* Require line of sight */
		if (!player_has_los_bold(Ind, m_ptr->fy, m_ptr->fx)) continue;

		/* Probe visible monsters */
		if (p_ptr->mon_vis[i])
		{
			char m_name[80];
                        char buf[80];
                        int j;

			/* Start the message */
			if (!probe) msg_print(Ind, "Probing...");

			/* Get "the monster" or "something" */
			monster_desc(Ind, m_name, i, 0x04);
                        sprintf(buf, "blows");

                        for (j = 0; j < 4; j++)
                        {
                                if (m_ptr->blow[j].d_dice) strcat(buf, format(" %dd%d", m_ptr->blow[j].d_dice, m_ptr->blow[j].d_side));
                        }

			/* Describe the monster */
                        msg_format(Ind, "%^s (%d) has %d hp, %d ac, %d speed.", m_name, m_ptr->level, m_ptr->hp, m_ptr->ac, m_ptr->speed);
                        msg_format(Ind, "%^s (%d) %s.", m_name, m_ptr->level, buf);

			/* Learn all of the non-spell, non-treasure flags */
			lore_do_probe(i);

			/* Probe worked */
			probe = TRUE;
		}
	}

	/* Done */
	if (probe)
	{
		msg_print(Ind, "That's all.");
	}

	/* Result */
	return (probe);
}



/*
 * The spell of destruction
 *
 * This spell "deletes" monsters (instead of "killing" them).
 *
 * Later we may use one function for both "destruction" and
 * "earthquake" by using the "full" to select "destruction".
 */
#ifdef NEW_DUNGEON
void destroy_area(struct worldpos *wpos, int y1, int x1, int r, bool full)
#else
void destroy_area(int Depth, int y1, int x1, int r, bool full)
#endif
{
	int y, x, k, t, Ind;

	player_type *p_ptr;

	cave_type *c_ptr;

	/*bool flag = FALSE;*/
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif


	/* XXX XXX */
	full = full ? full : 0;

	/* Don't hurt the main town or surrounding areas */
#ifdef NEW_DUNGEON
	if(istown(wpos))
#else
	if (Depth <= 0 ? (wild_info[Depth].radius <= 2) : 0)
#endif
		return;

	/* Big area of affect */
	for (y = (y1 - r); y <= (y1 + r); y++)
	{
		for (x = (x1 - r); x <= (x1 + r); x++)
		{
			/* Skip illegal grids */
#ifdef NEW_DUNGEON
			if (!in_bounds(y, x)) continue;
#else
			if (!in_bounds(Depth, y, x)) continue;
#endif

			/* Extract the distance */
			k = distance(y1, x1, y, x);

			/* Stay in the circle of death */
			if (k > r) continue;

			/* Access the grid */
#ifdef NEW_DUNGEON
			c_ptr = &zcave[y][x];
#else
			c_ptr = &cave[Depth][y][x];
#endif

			/* Lose room and vault */
			/* Hack -- don't do this to houses/rooms outside the dungeon,
			 * this will protect hosues outside town.
			 */
#ifdef NEW_DUNGEON
			if(wpos->wz)
#else
			if (Depth > 0)
#endif
			{
				c_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);
			}

			/* Lose light and knowledge */
			c_ptr->info &= ~(CAVE_GLOW);
#ifdef NEW_DUNGEON
			everyone_forget_spot(zcave, y, x);
#else
			everyone_forget_spot(Depth, y, x);
#endif

			/* Hack -- Notice player affect */
			if (c_ptr->m_idx < 0)
			{
				Ind = 0 - c_ptr->m_idx;
				p_ptr = Players[Ind];

				/* Message */
				msg_print(Ind, "There is a searing blast of light!");
	
				/* Blind the player */
				if (!p_ptr->resist_blind && !p_ptr->resist_lite)
				{
					/* Become blind */
					(void)set_blind(Ind, p_ptr->blind + 10 + randint(10));
				}

				/* Mega-Hack -- Forget the view and lite */
				p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);

				/* Update stuff */
				p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

				/* Update the monsters */
				p_ptr->update |= (PU_MONSTERS);
		
				/* Redraw map */
				p_ptr->redraw |= (PR_MAP);

				/* Window stuff */
				p_ptr->window |= (PW_OVERHEAD);

				/* Do not hurt this grid */
				continue;
			}

			/* Hack -- Skip the epicenter */
			if ((y == y1) && (x == x1)) continue;

			/* Delete the monster (if any) */
                        if (c_ptr->m_idx > 0)
                        {
                                monster_race *r_ptr = race_inf(&m_list[c_ptr->m_idx]);
#ifdef NEW_DUNGEON
                                if (!(r_ptr->flags9 & RF9_IM_TELE)) delete_monster(wpos, y, x);
#else
                                if (!(r_ptr->flags9 & RF9_IM_TELE)) delete_monster(Depth, y, x);
#endif
                                else continue;
                        }

			/* Destroy "valid" grids */
#ifdef NEW_DUNGEON
			if ((cave_valid_bold(zcave, y, x)) && !(c_ptr->info&CAVE_ICKY))
#else
			if ((cave_valid_bold(Depth, y, x)) && !(c_ptr->info&CAVE_ICKY))
#endif
			{
				/* Delete the object (if any) */
#ifdef NEW_DUNGEON
				delete_object(wpos, y, x);
#else
				delete_object(Depth, y, x);
#endif

				/* Wall (or floor) type */
				t = rand_int(200);

				/* Granite */
				if (t < 20)
				{
					/* Create granite wall */
					c_ptr->feat = FEAT_WALL_EXTRA;
				}

				/* Quartz */
				else if (t < 70)
				{
					/* Create quartz vein */
					c_ptr->feat = FEAT_QUARTZ;
				}

				/* Magma */
				else if (t < 100)
				{
					/* Create magma vein */
					c_ptr->feat = FEAT_MAGMA;
				}

				/* Floor */
				else
				{
					/* Create floor */
					c_ptr->feat = FEAT_FLOOR;
				}
			}
		}
	}
}


/*
 * Induce an "earthquake" of the given radius at the given location.
 *
 * This will turn some walls into floors and some floors into walls.
 *
 * The player will take damage and "jump" into a safe grid if possible,
 * otherwise, he will "tunnel" through the rubble instantaneously.
 *
 * Monsters will take damage, and "jump" into a safe grid if possible,
 * otherwise they will be "buried" in the rubble, disappearing from
 * the level in the same way that they do when genocided.
 *
 * Note that thus the player and monsters (except eaters of walls and
 * passers through walls) will never occupy the same grid as a wall.
 * Note that as of now (2.7.8) no monster may occupy a "wall" grid, even
 * for a single turn, unless that monster can pass_walls or kill_walls.
 * This has allowed massive simplification of the "monster" code.
 */
#ifdef NEW_DUNGEON
void earthquake(struct worldpos *wpos, int cy, int cx, int r)
#else
void earthquake(int Depth, int cy, int cx, int r)
#endif
{
	int		i, t, y, x, yy, xx, dy, dx, oy, ox;

	int		damage = 0;

	int		sn = 0, sy = 0, sx = 0;

	int Ind;

	player_type *p_ptr;

	/*bool	hurt = FALSE;*/

	cave_type	*c_ptr;

	bool	map[32][32];
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#endif


	/* Don't hurt town or surrounding areas */
#ifdef NEW_DUNGEON
	if(istown(wpos))
#else
	if (Depth <= 0 ? wild_info[Depth].radius <= 2 : 0)
#endif
		return;

	/* Paranoia -- Dnforce maximum range */
	if (r > 12) r = 12;

	/* Clear the "maximal blast" area */
	for (y = 0; y < 32; y++)
	{
		for (x = 0; x < 32; x++)
		{
			map[y][x] = FALSE;
		}
	}

	/* Check around the epicenter */
	for (dy = -r; dy <= r; dy++)
	{
		for (dx = -r; dx <= r; dx++)
		{
			/* Extract the location */
			yy = cy + dy;
			xx = cx + dx;

			/* Skip illegal grids */
#ifdef NEW_DUNGEON
			if (!in_bounds(yy, xx)) continue;
#else
			if (!in_bounds(Depth, yy, xx)) continue;
#endif

			/* Skip distant grids */
			if (distance(cy, cx, yy, xx) > r) continue;

			/* Access the grid */
#ifdef NEW_DUNGEON
			c_ptr = &zcave[yy][xx];
#else
			c_ptr = &cave[Depth][yy][xx];
#endif

			/* Hack -- ICKY spaces are protected outside of the dungeon */
#ifdef NEW_DUNGEON
			if((!wpos->wz) && (c_ptr->info & CAVE_ICKY)) continue;
#else
			if ((Depth < 0) && (c_ptr->info & CAVE_ICKY)) continue;
#endif

			/* Lose room and vault */
			c_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

			/* Lose light and knowledge */
			c_ptr->info &= ~(CAVE_GLOW);
#ifdef NEW_DUNGEON
			everyone_forget_spot(wpos, y, x);
#else
			everyone_forget_spot(Depth, y, x);
#endif

			/* Skip the epicenter */
			if (!dx && !dy) continue;

			/* Skip most grids */
			if (rand_int(100) < 85) continue;

			/* Damage this grid */
			map[16+yy-cy][16+xx-cx] = TRUE;

			/* Hack -- Take note of player damage */
			if (c_ptr->m_idx < 0)
			{
				Ind = 0 - c_ptr->m_idx;
				p_ptr = Players[Ind];

				/* Check around the player */
				for (i = 0; i < 8; i++)
				{
					/* Access the location */
					y = p_ptr->py + ddy[i];
					x = p_ptr->px + ddx[i];

					/* Skip non-empty grids */
#ifdef NEW_DUNGEON
					if (!cave_empty_bold(zcave, y, x)) continue;
#else
					if (!cave_empty_bold(Depth, y, x)) continue;
#endif

					/* Important -- Skip "quake" grids */
					if (map[16+y-cy][16+x-cx]) continue;

					/* Count "safe" grids */
					sn++;

					/* Randomize choice */
					if (rand_int(sn) > 0) continue;

					/* Save the safe location */
					sy = y; sx = x;
				}

				/* Random message */
				switch (randint(3))
				{
					case 1:
					{
						msg_print(Ind, "The cave ceiling collapses!");
						break;
					}
					case 2:
					{
						msg_print(Ind, "The cave floor twists in an unnatural way!");
						break;
					}
					default:
					{
						msg_print(Ind, "The cave quakes!  You are pummeled with debris!");
						break;
					}
				}

				/* Hurt the player a lot */
				if (!sn)
				{
					/* Message and damage */
					msg_print(Ind, "You are severely crushed!");
					damage = 300;
				}

				/* Destroy the grid, and push the player to safety */
				else
				{
					/* Calculate results */
					switch (randint(3))
					{
						case 1:
						{
							msg_print(Ind, "You nimbly dodge the blast!");
							damage = 0;
							break;
						}
						case 2:
						{
							msg_print(Ind, "You are bashed by rubble!");
							damage = damroll(10, 4);
							(void)set_stun(Ind, p_ptr->stun + randint(50));
							break;
						}
						case 3:
						{
							msg_print(Ind, "You are crushed between the floor and ceiling!");
							damage = damroll(10, 4);
							(void)set_stun(Ind, p_ptr->stun + randint(50));
							break;
						}
					}
		
					/* Save the old location */
					oy = p_ptr->py;
					ox = p_ptr->px;

					/* Move the player to the safe location */
					p_ptr->py = sy;
					p_ptr->px = sx;

					/* Update the cave player indices */
#ifdef NEW_DUNGEON
					zcave[oy][ox].m_idx = 0;
					zcave[sy][sx].m_idx = 0 - Ind;
#else
					cave[Depth][oy][ox].m_idx = 0;
					cave[Depth][sy][sx].m_idx = 0 - Ind;
#endif

#ifdef NEW_DUNGEON
					/* Redraw the old spot */
					everyone_lite_spot(wpos, oy, ox);

					/* Redraw the new spot */
					everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);
#else
					/* Redraw the old spot */
					everyone_lite_spot(Depth, oy, ox);

					/* Redraw the new spot */
					everyone_lite_spot(Depth, p_ptr->py, p_ptr->px);
#endif

					/* Check for new panel */
					verify_panel(Ind);
				}

				/* Important -- no wall on player */
				map[16+p_ptr->py-cy][16+p_ptr->px-cx] = FALSE;

				/* Take some damage */
				if (damage) take_hit(Ind, damage, "an earthquake");

				/* Mega-Hack -- Forget the view and lite */
				p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);

				/* Update stuff */
				p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

				/* Update the monsters */
				p_ptr->update |= (PU_DISTANCE);

				/* Update the health bar */
				p_ptr->redraw |= (PR_HEALTH);

				/* Redraw map */
				p_ptr->redraw |= (PR_MAP);

				/* Window stuff */
				p_ptr->window |= (PW_OVERHEAD);
			}
		}
	}


	/* Examine the quaked region */
	for (dy = -r; dy <= r; dy++)
	{
		for (dx = -r; dx <= r; dx++)
		{
			/* Extract the location */
			yy = cy + dy;
			xx = cx + dx;

			/* Skip unaffected grids */
			if (!map[16+yy-cy][16+xx-cx]) continue;

			/* Access the grid */
#ifdef NEW_DUNGEON
			c_ptr = &zcave[yy][xx];
#else
			c_ptr = &cave[Depth][yy][xx];
#endif

			/* Process monsters */
			if (c_ptr->m_idx > 0)
			{
				monster_type *m_ptr = &m_list[c_ptr->m_idx];
                                monster_race *r_ptr = race_inf(m_ptr);

				/* Most monsters cannot co-exist with rock */
				if (!(r_ptr->flags2 & RF2_KILL_WALL) &&
				    !(r_ptr->flags2 & RF2_PASS_WALL))
				{
					/* Assume not safe */
					sn = 0;

					/* Monster can move to escape the wall */
					if (!(r_ptr->flags1 & RF1_NEVER_MOVE))
					{
						/* Look for safety */
						for (i = 0; i < 8; i++)
						{
							/* Access the grid */
							y = yy + ddy[i];
							x = xx + ddx[i];

							/* Skip non-empty grids */
#ifdef NEW_DUNGEON
							if (!cave_empty_bold(zcave, y, x)) continue;

							/* Hack -- no safety on glyph of warding */
							if (zcave[y][x].feat == FEAT_GLYPH) continue;
#else
							if (!cave_empty_bold(Depth, y, x)) continue;

							/* Hack -- no safety on glyph of warding */
							if (cave[Depth][y][x].feat == FEAT_GLYPH) continue;
#endif

							/* Important -- Skip "quake" grids */
							if (map[16+y-cy][16+x-cx]) continue;

							/* Count "safe" grids */
							sn++;

							/* Randomize choice */
							if (rand_int(sn) > 0) continue;

							/* Save the safe grid */
							sy = y; sx = x;
						}
					}

					/* Describe the monster */
					/*monster_desc(Ind, m_name, c_ptr->m_idx, 0);*/

					/* Scream in pain */
					/*msg_format("%^s wails out in pain!", m_name);*/

					/* Take damage from the quake */
					damage = (sn ? damroll(4, 8) : 200);

					/* Monster is certainly awake */
					m_ptr->csleep = 0;

					/* Apply damage directly */
					m_ptr->hp -= damage;

					/* Delete (not kill) "dead" monsters */
					if (m_ptr->hp < 0)
					{
						/* Message */
						/*msg_format("%^s is embedded in the rock!", m_name);*/

						/* Delete the monster */
#ifdef NEW_DUNGEON
						delete_monster(wpos, yy, xx);
#else
						delete_monster(Depth, yy, xx);
#endif

						/* No longer safe */
						sn = 0;
					}

					/* Hack -- Escape from the rock */
					if (sn)
					{
#ifdef NEW_DUNGEON
						int m_idx = zcave[yy][xx].m_idx;

						/* Update the new location */
						zcave[sy][sx].m_idx = m_idx;

						/* Update the old location */
						zcave[yy][xx].m_idx = 0;
#else
						int m_idx = cave[Depth][yy][xx].m_idx;

						/* Update the new location */
						cave[Depth][sy][sx].m_idx = m_idx;

						/* Update the old location */
						cave[Depth][yy][xx].m_idx = 0;
#endif

						/* Move the monster */
						m_ptr->fy = sy;
						m_ptr->fx = sx;

						/* Update the monster (new location) */
						update_mon(m_idx, TRUE);

#ifdef NEW_DUNGEON
						/* Redraw the new grid */
						everyone_lite_spot(wpos, yy, xx);

						/* Redraw the new grid */
						everyone_lite_spot(wpos, sy, sx);
#else
						/* Redraw the old grid */
						everyone_lite_spot(Depth, yy, xx);

						/* Redraw the new grid */
						everyone_lite_spot(Depth, sy, sx);
#endif
					}
				}
			}
		}
	}


	/* Examine the quaked region */
	for (dy = -r; dy <= r; dy++)
	{
		for (dx = -r; dx <= r; dx++)
		{
			/* Extract the location */
			yy = cy + dy;
			xx = cx + dx;

			/* Skip unaffected grids */
			if (!map[16+yy-cy][16+xx-cx]) continue;

			/* Access the cave grid */
#ifdef NEW_DUNGEON
			c_ptr = &zcave[yy][xx];
#else
			c_ptr = &cave[Depth][yy][xx];
#endif

			/* Paranoia -- never affect player */
			if (c_ptr->m_idx < 0) continue;

			/* Destroy location (if valid) */
#ifdef NEW_DUNGEON
			if (cave_valid_bold(zcave, yy, xx))
			{
				bool floor = cave_floor_bold(zcave, yy, xx);

				/* Delete any object that is still there */
				delete_object(wpos, yy, xx);
#else
			if (cave_valid_bold(Depth, yy, xx))
			{
				bool floor = cave_floor_bold(Depth, yy, xx);

				/* Delete any object that is still there */
				delete_object(Depth, yy, xx);
#endif

				/* Wall (or floor) type */
				t = (floor ? rand_int(100) : 200);

				/* Granite */
				if (t < 20)
				{
					/* Create granite wall */
					c_ptr->feat = FEAT_WALL_EXTRA;
				}

				/* Quartz */
				else if (t < 70)
				{
					/* Create quartz vein */
					c_ptr->feat = FEAT_QUARTZ;
				}

				/* Magma */
				else if (t < 100)
				{
					/* Create magma vein */
					c_ptr->feat = FEAT_MAGMA;
				}

				/* Floor */
				else
				{
					/* Create floor */
					c_ptr->feat = FEAT_FLOOR;
				}
			}
		}
	}
}

/* Wipe everything */
#ifdef NEW_DUNGEON
void wipe_spell(struct worldpos *wpos, int cy, int cx, int r)
#else
void wipe_spell(int Depth, int cy, int cx, int r)
#endif
{
	int		i, t, y, x, yy, xx, dy, dx, oy, ox;

	cave_type	*c_ptr;



#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
	/* Don't hurt town or surrounding areas */
	if(istown(wpos))
#else
	if (Depth <= 0 ? wild_info[Depth].radius <= 2 : 0)
#endif
		return;

	/* Paranoia -- Dnforce maximum range */
	if (r > 12) r = 12;

	/* Check around the epicenter */
	for (dy = -r; dy <= r; dy++)
		for (dx = -r; dx <= r; dx++)
		{
			/* Extract the location */
			yy = cy + dy;
			xx = cx + dx;

			/* Skip illegal grids */
#ifdef NEW_DUNGEON
			if (!in_bounds(yy, xx)) continue;
#else
			if (!in_bounds(Depth, yy, xx)) continue;
#endif

			/* Skip distant grids */
			if (distance(cy, cx, yy, xx) > r) continue;

			/* Access the grid */
#ifdef NEW_DUNGEON
			c_ptr = &zcave[yy][xx];
#else
			c_ptr = &cave[Depth][yy][xx];
#endif

			/* Hack -- ICKY spaces are protected outside of the dungeon */
			if (c_ptr->info & CAVE_ICKY) continue;

			/* Lose room and vault */
			c_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);
			
			/* Turn into basic floor */
			c_ptr->feat = FEAT_FLOOR;
			
			/* Delete monsters */
                        if (c_ptr->m_idx > 0)
                        {
                                monster_race *r_ptr = race_inf(&m_list[c_ptr->m_idx]);
#ifdef NEW_DUNGEON
                                if (!(r_ptr->flags9 & RF9_IM_TELE)) delete_monster(wpos, y, x);
#else
                                if (!(r_ptr->flags9 & RF9_IM_TELE)) delete_monster(Depth, y, x);
#endif
                                else continue;
                        }
			
#ifdef NEW_DUNGEON
			/* Delete objects */
			delete_object(wpos, yy, xx);

			everyone_lite_spot(wpos, yy, xx);
#else
			/* Delete objects */
			delete_object(Depth, yy, xx);

			everyone_lite_spot(Depth, yy, xx);
#endif
		}
}


/*
 * This routine clears the entire "temp" set.
 *
 * This routine will Perma-Lite all "temp" grids.
 *
 * This routine is used (only) by "lite_room()"
 *
 * Dark grids are illuminated.
 *
 * Also, process all affected monsters.
 *
 * SMART monsters always wake up when illuminated
 * NORMAL monsters wake up 1/4 the time when illuminated
 * STUPID monsters wake up 1/10 the time when illuminated
 */
static void cave_temp_room_lite(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int i;
#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#else
	int Depth = p_ptr->dun_depth;
#endif

	/* Clear them all */
	for (i = 0; i < p_ptr->temp_n; i++)
	{
		int y = p_ptr->temp_y[i];
		int x = p_ptr->temp_x[i];

#ifdef NEW_DUNGEON
		cave_type *c_ptr = &zcave[y][x];
#else
		cave_type *c_ptr = &cave[Depth][y][x];
#endif

		/* No longer in the array */
		c_ptr->info &= ~CAVE_TEMP;

		/* Update only non-CAVE_GLOW grids */
		/* if (c_ptr->info & CAVE_GLOW) continue; */

		/* Perma-Lite */
		c_ptr->info |= CAVE_GLOW;

		/* Process affected monsters */
		if (c_ptr->m_idx > 0)
		{
			int chance = 25;

			monster_type	*m_ptr = &m_list[c_ptr->m_idx];

                        monster_race    *r_ptr = race_inf(m_ptr);

			/* Update the monster */
			update_mon(c_ptr->m_idx, FALSE);

			/* Stupid monsters rarely wake up */
			if (r_ptr->flags2 & RF2_STUPID) chance = 10;

			/* Smart monsters always wake up */
			if (r_ptr->flags2 & RF2_SMART) chance = 100;

			/* Sometimes monsters wake up */
			if (m_ptr->csleep && (rand_int(100) < chance))
			{
				/* Wake up! */
				m_ptr->csleep = 0;

				/* Notice the "waking up" */
				if (p_ptr->mon_vis[c_ptr->m_idx])
				{
					char m_name[80];

					/* Acquire the monster name */
					monster_desc(Ind, m_name, c_ptr->m_idx, 0);

					/* Dump a message */
					msg_format(Ind, "%^s wakes up.", m_name);
				}
			}
		}

#ifdef NEW_DUNGEON
		/* Note */
		note_spot_depth(wpos, y, x);

		/* Redraw */
		everyone_lite_spot(wpos, y, x);
#else
		/* Note */
		note_spot_depth(Depth, y, x);

		/* Redraw */
		everyone_lite_spot(Depth, y, x);
#endif
	}

	/* None left */
	p_ptr->temp_n = 0;
}



/*
 * This routine clears the entire "temp" set.
 *
 * This routine will "darken" all "temp" grids.
 *
 * In addition, some of these grids will be "unmarked".
 *
 * This routine is used (only) by "unlite_room()"
 *
 * Also, process all affected monsters
 */
static void cave_temp_room_unlite(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int i;
#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#else
	int Depth = p_ptr->dun_depth;
#endif

	/* Clear them all */
	for (i = 0; i < p_ptr->temp_n; i++)
	{
		int y = p_ptr->temp_y[i];
		int x = p_ptr->temp_x[i];

#ifdef NEW_DUNGEON
		cave_type *c_ptr = &zcave[y][x];
#else
		cave_type *c_ptr = &cave[Depth][y][x];
#endif

		/* No longer in the array */
		c_ptr->info &= ~CAVE_TEMP;

		/* Darken the grid */
		c_ptr->info &= ~CAVE_GLOW;

		/* Hack -- Forget "boring" grids */
		if (c_ptr->feat <= FEAT_INVIS)
		{
			/* Forget the grid */
			p_ptr->cave_flag[y][x] &= ~CAVE_MARK;

			/* Notice */
#ifdef NEW_DUNGEON
			note_spot_depth(wpos, y, x);
#else
			note_spot_depth(Depth, y, x);
#endif
		}

		/* Process affected monsters */
		if (c_ptr->m_idx > 0)
		{
			/* Update the monster */
			update_mon(c_ptr->m_idx, FALSE);
		}

#ifdef NEW_DUNGEON
		/* Redraw */
		everyone_lite_spot(wpos, y, x);
#else
		/* Redraw */
		everyone_lite_spot(Depth, y, x);
#endif
	}

	/* None left */
	p_ptr->temp_n = 0;
}




/*
 * Aux function -- see below
 */
#ifdef NEW_DUNGEON
static void cave_temp_room_aux(int Ind, struct worldpos *wpos, int y, int x)
#else
static void cave_temp_room_aux(int Ind, int Depth, int y, int x)
#endif
{
	player_type *p_ptr = Players[Ind];

#ifdef NEW_DUNGEON
	cave_type *c_ptr;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
	c_ptr = &zcave[y][x];
#else
	cave_type *c_ptr = &cave[Depth][y][x];
#endif

	/* Avoid infinite recursion */
	if (c_ptr->info & CAVE_TEMP) return;

	/* Do not "leave" the current room */
	if (!(c_ptr->info & CAVE_ROOM)) return;

	/* Paranoia -- verify space */
	if (p_ptr->temp_n == TEMP_MAX) return;

	/* Mark the grid as "seen" */
	c_ptr->info |= CAVE_TEMP;

	/* Add it to the "seen" set */
	p_ptr->temp_y[p_ptr->temp_n] = y;
	p_ptr->temp_x[p_ptr->temp_n] = x;
	p_ptr->temp_n++;
}




/*
 * Illuminate any room containing the given location.
 */
#ifdef NEW_DUNGEON
void lite_room(int Ind, struct worldpos *wpos, int y1, int x1)
#else
void lite_room(int Ind, int Depth, int y1, int x1)
#endif
{
	player_type *p_ptr = Players[Ind];

	int i, x, y;

#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;

	/* Add the initial grid */
	cave_temp_room_aux(Ind, wpos, y1, x1);
#else
	/* Add the initial grid */
	cave_temp_room_aux(Ind, Depth, y1, x1);
#endif

	/* While grids are in the queue, add their neighbors */
	for (i = 0; i < p_ptr->temp_n; i++)
	{
		x = p_ptr->temp_x[i], y = p_ptr->temp_y[i];

		/* Walls get lit, but stop light */
#ifdef NEW_DUNGEON
		if (!cave_floor_bold(zcave, y, x)) continue;

		/* Spread adjacent */
		cave_temp_room_aux(Ind, wpos, y + 1, x);
		cave_temp_room_aux(Ind, wpos, y - 1, x);
		cave_temp_room_aux(Ind, wpos, y, x + 1);
		cave_temp_room_aux(Ind, wpos, y, x - 1);

		/* Spread diagonal */
		cave_temp_room_aux(Ind, wpos, y + 1, x + 1);
		cave_temp_room_aux(Ind, wpos, y - 1, x - 1);
		cave_temp_room_aux(Ind, wpos, y - 1, x + 1);
		cave_temp_room_aux(Ind, wpos, y + 1, x - 1);
#else
		if (!cave_floor_bold(Depth, y, x)) continue;

		/* Spread adjacent */
		cave_temp_room_aux(Ind, Depth, y + 1, x);
		cave_temp_room_aux(Ind, Depth, y - 1, x);
		cave_temp_room_aux(Ind, Depth, y, x + 1);
		cave_temp_room_aux(Ind, Depth, y, x - 1);

		/* Spread diagonal */
		cave_temp_room_aux(Ind, Depth, y + 1, x + 1);
		cave_temp_room_aux(Ind, Depth, y - 1, x - 1);
		cave_temp_room_aux(Ind, Depth, y - 1, x + 1);
		cave_temp_room_aux(Ind, Depth, y + 1, x - 1);
#endif
	}

	/* Now, lite them all up at once */
	cave_temp_room_lite(Ind);
}


/*
 * Darken all rooms containing the given location
 */
#ifdef NEW_DUNGEON
void unlite_room(int Ind, struct worldpos *wpos, int y1, int x1)
#else
void unlite_room(int Ind, int Depth, int y1, int x1)
#endif
{
	player_type *p_ptr = Players[Ind];

	int i, x, y;

#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;

	/* Add the initial grid */
	cave_temp_room_aux(Ind, wpos, y1, x1);
#else
	/* Add the initial grid */
	cave_temp_room_aux(Ind, Depth, y1, x1);
#endif

	/* Spread, breadth first */
	for (i = 0; i < p_ptr->temp_n; i++)
	{
		x = p_ptr->temp_x[i], y = p_ptr->temp_y[i];

		/* Walls get dark, but stop darkness */
#ifdef NEW_DUNGEON
		if (!cave_floor_bold(zcave, y, x)) continue;

		/* Spread adjacent */
		cave_temp_room_aux(Ind, wpos, y + 1, x);
		cave_temp_room_aux(Ind, wpos, y - 1, x);
		cave_temp_room_aux(Ind, wpos, y, x + 1);
		cave_temp_room_aux(Ind, wpos, y, x - 1);

		/* Spread diagonal */
		cave_temp_room_aux(Ind, wpos, y + 1, x + 1);
		cave_temp_room_aux(Ind, wpos, y - 1, x - 1);
		cave_temp_room_aux(Ind, wpos, y - 1, x + 1);
		cave_temp_room_aux(Ind, wpos, y + 1, x - 1);
#else
		if (!cave_floor_bold(Depth, y, x)) continue;

		/* Spread adjacent */
		cave_temp_room_aux(Ind, Depth, y + 1, x);
		cave_temp_room_aux(Ind, Depth, y - 1, x);
		cave_temp_room_aux(Ind, Depth, y, x + 1);
		cave_temp_room_aux(Ind, Depth, y, x - 1);

		/* Spread diagonal */
		cave_temp_room_aux(Ind, Depth, y + 1, x + 1);
		cave_temp_room_aux(Ind, Depth, y - 1, x - 1);
		cave_temp_room_aux(Ind, Depth, y - 1, x + 1);
		cave_temp_room_aux(Ind, Depth, y + 1, x - 1);
#endif
	}

	/* Now, darken them all at once */
	cave_temp_room_unlite(Ind);
}



/*
 * Hack -- call light around the player
 * Affect all monsters in the projection radius
 */
bool lite_area(int Ind, int dam, int rad)
{
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_GRID | PROJECT_KILL;

	/* Hack -- Message */
	if (!p_ptr->blind)
	{
		msg_print(Ind, "You are surrounded by a white light.");
	}

#ifdef NEW_DUNGEON
	/* Hook into the "project()" function */
	(void)project(0 - Ind, rad, &p_ptr->wpos, p_ptr->py, p_ptr->px, dam, GF_LITE_WEAK, flg);

	/* Lite up the room */
	lite_room(Ind, &p_ptr->wpos, p_ptr->py, p_ptr->px);
#else
	/* Hook into the "project()" function */
	(void)project(0 - Ind, rad, p_ptr->dun_depth, p_ptr->py, p_ptr->px, dam, GF_LITE_WEAK, flg);

	/* Lite up the room */
	lite_room(Ind, p_ptr->dun_depth, p_ptr->py, p_ptr->px);
#endif

	/* Assume seen */
	return (TRUE);
}


/*
 * Hack -- call darkness around the player
 * Affect all monsters in the projection radius
 */
bool unlite_area(int Ind, int dam, int rad)
{
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_GRID | PROJECT_KILL;

	/* Hack -- Message */
	if (!p_ptr->blind)
	{
		msg_print(Ind, "Darkness surrounds you.");
	}

#ifdef NEW_DUNGEON
	/* Hook into the "project()" function */
	(void)project(0 - Ind, rad, &p_ptr->wpos, p_ptr->py, p_ptr->px, dam, GF_DARK_WEAK, flg);

	/* Lite up the room */
	unlite_room(Ind, &p_ptr->wpos, p_ptr->py, p_ptr->px);
#else
	/* Hook into the "project()" function */
	(void)project(0 - Ind, rad, p_ptr->dun_depth, p_ptr->py, p_ptr->px, dam, GF_DARK_WEAK, flg);

	/* Lite up the room */
	unlite_room(Ind, p_ptr->dun_depth, p_ptr->py, p_ptr->px);
#endif

	/* Assume seen */
	return (TRUE);
}



/*
 * Cast a ball spell
 * Stop if we hit a monster, act as a "ball"
 * Allow "target" mode to pass over monsters
 * Affect grids, objects, and monsters
 */
bool fire_ball(int Ind, int typ, int dir, int dam, int rad)
{
	player_type *p_ptr = Players[Ind];

	int tx, ty;

	int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

	/* Use the given direction */
	tx = p_ptr->px + 99 * ddx[dir];
	ty = p_ptr->py + 99 * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay(Ind))
	{
		flg &= ~PROJECT_STOP;
		tx = p_ptr->target_col;
		ty = p_ptr->target_row;
	}

	/* Analyze the "dir" and the "target".  Hurt items on floor. */
#ifdef NEW_DUNGEON
	return (project(0 - Ind, rad, &p_ptr->wpos, ty, tx, dam, typ, flg));
#else
	return (project(0 - Ind, rad, p_ptr->dun_depth, ty, tx, dam, typ, flg));
#endif
}


/*
 * Hack -- apply a "projection()" in a direction (or at the target)
 */
bool project_hook(int Ind, int typ, int dir, int dam, int flg)
{
	player_type *p_ptr = Players[Ind];

	int tx, ty;

	/* Pass through the target if needed */
	flg |= (PROJECT_THRU);

	/* Use the given direction */
	tx = p_ptr->px + ddx[dir];
	ty = p_ptr->py + ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay(Ind))
	{
		tx = p_ptr->target_col;
		ty = p_ptr->target_row;
	}

	/* Analyze the "dir" and the "target", do NOT explode */
#ifdef NEW_DUNGEON
	return (project(0 - Ind, 0, &p_ptr->wpos, ty, tx, dam, typ, flg));
#else
	return (project(0 - Ind, 0, p_ptr->dun_depth, ty, tx, dam, typ, flg));
#endif
}


/*
 * Cast a bolt spell
 * Stop if we hit a monster, as a "bolt"
 * Affect monsters (not grids or objects)
 */
bool fire_bolt(int Ind, int typ, int dir, int dam)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(Ind, typ, dir, dam, flg));
}

/*
 * Cast a beam spell
 * Pass through monsters, as a "beam"
 * Affect monsters (not grids or objects)
 */
bool fire_beam(int Ind, int typ, int dir, int dam)
{
	int flg = PROJECT_BEAM | PROJECT_KILL;
	return (project_hook(Ind, typ, dir, dam, flg));
}

/*
 * Cast a bolt spell, or rarely, a beam spell
 */
bool fire_bolt_or_beam(int Ind, int prob, int typ, int dir, int dam)
{
	if (rand_int(100) < prob)
	{
		return (fire_beam(Ind, typ, dir, dam));
	}
	else
	{
		return (fire_bolt(Ind, typ, dir, dam));
	}
}


/*
 * Some of the old functions
 */

bool lite_line(int Ind, int dir)
{
	int flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_KILL;
	return (project_hook(Ind, GF_LITE_WEAK, dir, damroll(6, 8), flg));
}

bool drain_life(int Ind, int dir, int dam)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(Ind, GF_OLD_DRAIN, dir, dam, flg));
}

bool wall_to_mud(int Ind, int dir)
{
	int flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
	return (project_hook(Ind, GF_KILL_WALL, dir, 20 + randint(30), flg));
}

bool destroy_door(int Ind, int dir)
{
	int flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM;
	return (project_hook(Ind, GF_KILL_DOOR, dir, 0, flg));
}

bool disarm_trap(int Ind, int dir)
{
	int flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM;
	return (project_hook(Ind, GF_KILL_TRAP, dir, 0, flg));
}

bool heal_monster(int Ind, int dir)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(Ind, GF_OLD_HEAL, dir, damroll(4, 6), flg));
}

bool speed_monster(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(Ind, GF_OLD_SPEED, dir, p_ptr->lev, flg));
}

bool slow_monster(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(Ind, GF_OLD_SLOW, dir, p_ptr->lev, flg));
}

bool sleep_monster(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(Ind, GF_OLD_SLEEP, dir, p_ptr->lev, flg));
}

bool confuse_monster(int Ind, int dir, int plev)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(Ind, GF_OLD_CONF, dir, plev, flg));
}

bool poly_monster(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(Ind, GF_OLD_POLY, dir, p_ptr->lev, flg));
}

bool clone_monster(int Ind, int dir)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(Ind, GF_OLD_CLONE, dir, 0, flg));
}

bool fear_monster(int Ind, int dir, int plev)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(Ind, GF_TURN_ALL, dir, plev, flg));
}

bool teleport_monster(int Ind, int dir)
{
	int flg = PROJECT_BEAM | PROJECT_KILL;
	return (project_hook(Ind, GF_AWAY_ALL, dir, MAX_SIGHT * 5, flg));
}

bool cure_light_wounds_proj(int Ind, int dir)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(Ind, GF_HEAL_PLAYER, dir, damroll(2, 10), flg));
}

bool cure_serious_wounds_proj(int Ind, int dir)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(Ind, GF_HEAL_PLAYER, dir, damroll(4, 10), flg));
}

bool cure_critical_wounds_proj(int Ind, int dir)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(Ind, GF_HEAL_PLAYER, dir, damroll(6, 10), flg));
}

bool heal_other_proj(int Ind, int dir)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(Ind, GF_HEAL_PLAYER, dir, 2000, flg));
}



/*
 * Hooks -- affect adjacent grids (radius 1 ball attack)
 */

bool door_creation(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
#ifdef NEW_DUNGEON
	return (project(0 - Ind, 1, &p_ptr->wpos, p_ptr->py, p_ptr->px, 0, GF_MAKE_DOOR, flg));
#else
	return (project(0 - Ind, 1, p_ptr->dun_depth, p_ptr->py, p_ptr->px, 0, GF_MAKE_DOOR, flg));
#endif
}

bool trap_creation(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
#ifdef NEW_DUNGEON
	return (project(0 - Ind, 1, &p_ptr->wpos, p_ptr->py, p_ptr->px, 0, GF_MAKE_TRAP, flg));
#else
	return (project(0 - Ind, 1, p_ptr->dun_depth, p_ptr->py, p_ptr->px, 0, GF_MAKE_TRAP, flg));
#endif
}

bool destroy_doors_touch(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
#ifdef NEW_DUNGEON
	return (project(0 - Ind, 1, &p_ptr->wpos, p_ptr->py, p_ptr->px, 0, GF_KILL_DOOR, flg));
#else
	return (project(0 - Ind, 1, p_ptr->dun_depth, p_ptr->py, p_ptr->px, 0, GF_KILL_DOOR, flg));
#endif
}

bool sleep_monsters_touch(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_KILL | PROJECT_HIDE;
#ifdef NEW_DUNGEON
	return (project(0 - Ind, 1, &p_ptr->wpos, p_ptr->py, p_ptr->px, p_ptr->lev, GF_OLD_SLEEP, flg));
#else
	return (project(0 - Ind, 1, p_ptr->dun_depth, p_ptr->py, p_ptr->px, p_ptr->lev, GF_OLD_SLEEP, flg));
#endif
}

/* Scan magical powers for the golem */
void scan_golem_flags(object_type *o_ptr, monster_race *r_ptr)
{
	    u32b f1, f2, f3, f4, f5, esp;

			  /* Extract the flags */
			  object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

        if (f1 & TR1_LIFE) r_ptr->hdice += o_ptr->pval;
        if (f1 & TR1_SPEED) r_ptr->speed += o_ptr->pval * 2 / 3;
        if (f1 & TR1_TUNNEL) r_ptr->flags2 |= RF2_KILL_WALL;
        if (f2 & TR2_RES_FIRE) r_ptr->flags3 |= RF3_IM_FIRE;
        if (f2 & TR2_RES_ACID) r_ptr->flags3 |= RF3_IM_ACID;
        if (f2 & TR2_RES_NETHER) r_ptr->flags3 |= RF3_RES_NETH;
        if (f2 & TR2_RES_NEXUS) r_ptr->flags3 |= RF3_RES_NEXU;
        if (f2 & TR2_RES_DISEN) r_ptr->flags3 |= RF3_RES_DISE;
}

/* multi builder stuff - move when complete */
struct builder{
	int player;
	int lx,ly,dx,dy,minx,miny,maxx,maxy;
	int sx,sy;
	int odir;
	int moves;
	int cvert;
#ifdef NEW_DUNGEON
	struct worldpos wpos;
#else
	int depth;
#endif
	bool nofloor;
	struct dna_type *dna;
	char *vert;
	struct builder *next;
};

#define MAX_BUILDERS 4	/* Just so the builders can go on strike */
/* end of move stuff */

bool poly_build(int Ind, char *args){
	static struct builder *builders=NULL;
	static int num_build=0;

	player_type *p_ptr=Players[Ind];
	struct builder *curr=builders;
	int x,y;
	int dir=0;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return(FALSE);
#endif
	while(curr){
		struct builder *prev=NULL;
		bool kill=FALSE;
		if(curr->player==p_ptr->id) break;
		if(!lookup_player_name(curr->player)){	/* disconnect or free builders */
			if(prev)
				prev->next=curr->next;
			else
				builders=curr->next;
			kill=TRUE;
		}
		prev=curr;
		curr=curr->next;
		if(kill){
			KILL(prev, struct builder);
		}
	}

	if(!curr){			/* new builder */
#ifdef MAX_BUILDERS
		if(num_build==MAX_BUILDERS){
			msg_print(Ind,"The builders are on strike!");
			return FALSE;
		}
#endif
		MAKE(curr, struct builder);
		curr->next=builders;	/* insert is fastest */
		curr->player=p_ptr->id;	/* set him up */
		C_MAKE(curr->vert, MAXCOORD, byte);
		MAKE(curr->dna, struct dna_type);
		curr->dna->creator=p_ptr->dna;
		curr->dna->owner=p_ptr->id;
		curr->dna->owner_type=OT_PLAYER;
		curr->dna->a_flags=ACF_NONE;
		curr->dna->min_level=ACF_NONE;
		curr->dna->price=5;	/* so work out */
		curr->odir=0;
		curr->cvert=0;
		curr->nofloor=(*args=='N');
		curr->sx=p_ptr->px;
		curr->sy=p_ptr->py;
		curr->minx=curr->maxx=curr->sx;
		curr->miny=curr->maxy=curr->sy;
		curr->dx=curr->lx=curr->sx;
		curr->dy=curr->ly=curr->sy;
		curr->moves=25;	/* always new */
#ifdef NEW_DUNGEON
		wpcopy(&curr->wpos, &p_ptr->wpos);
		if(zcave[curr->sy][curr->sx].feat==FEAT_PERM_EXTRA){
			zcave[curr->sy][curr->sx].special.ptr=NULL;
#else
		curr->depth=p_ptr->dun_depth;
		if(cave[p_ptr->dun_depth][curr->sy][curr->sx].feat==FEAT_PERM_EXTRA){
			cave[p_ptr->dun_depth][curr->sy][curr->sx].special.ptr=NULL;
#endif
			msg_print(Ind, "Your foundations were laid insecurely");
			KILL(curr->dna, struct dna_type);
			C_KILL(curr->vert, MAXCOORD, byte);
			p_ptr->master_move_hook=NULL;
			KILL(curr, struct builder);	/* Sack the builders! */
			return FALSE;
		}
#ifdef NEW_DUNGEON
		zcave[curr->sy][curr->sx].feat=FEAT_HOME_OPEN;
		zcave[curr->sy][curr->sx].special.type=DNA_DOOR;
		zcave[curr->sy][curr->sx].special.ptr=curr->dna;
#else
		cave[p_ptr->dun_depth][curr->sy][curr->sx].feat=FEAT_HOME_OPEN;
		cave[p_ptr->dun_depth][curr->sy][curr->sx].special.type=DNA_DOOR;
		cave[p_ptr->dun_depth][curr->sy][curr->sx].special.ptr=curr->dna;
#endif
		builders=curr;
		return TRUE;

	}

	if(args){
		curr->moves+=25;
		return TRUE;
	}
	x=p_ptr->px;
	y=p_ptr->py;
	curr->minx=MIN(x,curr->minx);
	curr->maxx=MAX(x,curr->maxx);
	curr->miny=MIN(y,curr->miny);
	curr->maxy=MAX(y,curr->maxy);
	if(x!=curr->dx){
		if(x>curr->dx) dir=1;
		else dir=2;
	}
	if(y!=curr->dy){
		if(dir){
			curr->moves=0;
			/* diagonal! house building failed */
		}
		if(y>curr->dy) dir|=4;
		else dir|=8;
	}
	if(curr->odir!=dir){
		if(curr->odir){		/* first move not new vertex */
			curr->vert[curr->cvert++]=curr->dx-curr->lx;
			curr->vert[curr->cvert++]=curr->dy-curr->ly;
		}
		curr->lx=curr->dx;
		curr->ly=curr->dy;
		curr->odir=dir;		/* change direction, add vertex */
	}
	curr->dx=x;
	curr->dy=y;

	if(p_ptr->px==curr->sx && p_ptr->py==curr->sy && curr->moves){	/* check for close */
		curr->vert[curr->cvert++]=curr->dx-curr->lx;			/* last vertex */
		curr->vert[curr->cvert++]=curr->dy-curr->ly;
		if(curr->cvert==10 || curr->cvert==8){
			/* rectangle! */
			houses[num_houses].flags=HF_RECT;
			houses[num_houses].x=curr->minx;
			houses[num_houses].y=curr->miny;
			houses[num_houses].coords.rect.width=curr->maxx+1-curr->minx;
			houses[num_houses].coords.rect.height=curr->maxy+1-curr->miny;
			houses[num_houses].dx=curr->sx-curr->minx;
			houses[num_houses].dy=curr->sy-curr->miny;
			C_KILL(curr->vert, MAXCOORD, byte);
		}
		else{
			houses[num_houses].flags=HF_NONE;	/* polygonal */
			houses[num_houses].x=curr->sx;
			houses[num_houses].y=curr->sy;
			houses[num_houses].coords.poly=curr->vert;
		}
		if(curr->nofloor) houses[num_houses].flags|=HF_NOFLOOR;
/* Moat testing */
#if 0
		houses[num_houses].flags|=HF_MOAT;
#endif
/* Do not commit! */
#ifdef NEW_DUNGEON
		wpcopy(&houses[num_houses].wpos, &p_ptr->wpos);
#else
		houses[num_houses].depth=p_ptr->dun_depth;
#endif
		houses[num_houses].dna=curr->dna;
		if(curr->cvert>=8 && fill_house(&houses[num_houses], 2, NULL)){
			int area=(curr->maxx-curr->minx)*(curr->maxy-curr->miny);
			wild_add_uhouse(&houses[num_houses]);
			curr->dna->price=area*area*400;
			msg_print(Ind,"You have completed your house");
			num_houses++;
		}
		else{
			msg_print(Ind,"Your house was built unsoundly");
			if(curr->vert) C_KILL(curr->vert, MAXCOORD, byte);
			KILL(curr->dna, struct dna_type);
		}
		curr->player=0;		/* send the builders home */
		p_ptr->master_move_hook=NULL;
		p_ptr->update|=PU_VIEW;
		return TRUE;
	}
	/* no going off depth, and no spoiling moats */
#ifdef NEW_DUNGEON
	if(inarea(&curr->wpos, &p_ptr->wpos) && !(zcave[curr->dy][curr->dx].info&CAVE_ICKY && zcave[curr->dy][curr->dx].feat==FEAT_WATER)){
		zcave[curr->dy][curr->dx].feat=FEAT_WALL_EXTRA;
		if(curr->cvert<MAXCOORD && (--curr->moves)>0) return TRUE;
		p_ptr->update|=PU_VIEW;
	}
	msg_print(Ind,"Your house building attempt has failed");
	zcave[curr->sy][curr->sx].special.ptr=NULL;
#else
	if(curr->depth==p_ptr->dun_depth && !(cave[curr->depth][curr->dy][curr->dx].info&CAVE_ICKY && cave[curr->depth][curr->dy][curr->dx].feat==FEAT_WATER)){
		cave[p_ptr->dun_depth][curr->dy][curr->dx].feat=FEAT_WALL_EXTRA;
		if(curr->cvert<MAXCOORD && (--curr->moves)>0) return TRUE;
		p_ptr->update|=PU_VIEW;
	}
	msg_print(Ind,"Your house building attempt has failed");
	cave[p_ptr->dun_depth][curr->sy][curr->sx].special.ptr=NULL;
#endif
	KILL(curr->dna, struct dna_type);
	C_KILL(curr->vert, MAXCOORD, byte);
	curr->player=0;		/* send the builders home */
	p_ptr->master_move_hook=NULL;
	return FALSE;
}

void house_creation(int Ind, bool floor){
	player_type *p_ptr=Players[Ind];
	/* set master_move_hook : a bit like a setuid really ;) */

#ifdef NEW_DUNGEON
	if(p_ptr->wpos.wz || istown(&p_ptr->wpos)) return;		/* Building in town??? no */
#else
	if(p_ptr->dun_depth>=0) return;		/* Building in town??? no */
#endif
	if((house_alloc-num_houses)<32){
		GROW(houses, house_alloc, house_alloc+512, house_type);
		house_alloc+=512;
	}
	p_ptr->master_move_hook=poly_build;
	poly_build(Ind, floor ? "Y" : "N");	/* Its a (char*) ;( */
}


/* Create a mindless servant ! */
void golem_creation(int Ind, int max)
{
	player_type *p_ptr = Players[Ind];
        monster_race *r_ptr;
        monster_type *m_ptr;
        int i;
        int golem_type = -1;
        int golem_arms[4], golem_m_arms = 0;
        int golem_legs[30], golem_m_legs = 0;
        s16b golem_flags = 0;
        cave_type *c_ptr;
        int x, y, k, g_cnt = 0;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return;
#endif

        /* Process the monsters */
        for (k = m_top - 1; k >= 0; k--)
        {
                /* Access the index */
                i = m_fast[k];

                /* Access the monster */
                m_ptr = &m_list[i];

                /* Excise "dead" monsters */
                if (!m_ptr->r_idx) continue;

                if (m_ptr->owner != p_ptr->id) continue;

                if (!i) continue;

                g_cnt++;
        }

        if (g_cnt >= max)
        {
                msg_print(Ind, "You cannot create more golems.");
        }

        for (x = p_ptr->px - 1; x <= p_ptr->px; x++)        
        for (y = p_ptr->py - 1; y <= p_ptr->py; y++)
        {
                /* Verify location */
#ifdef NEW_DUNGEON
                if (!in_bounds(y, x)) continue;
                /* Require empty space */
                if (!cave_empty_bold(zcave, y, x)) continue;

                /* Hack -- no creation on glyph of warding */
                if (zcave[y][x].feat == FEAT_GLYPH) continue;
#else
                if (!in_bounds(p_ptr->dun_depth, y, x)) continue;
                /* Require empty space */
                if (!cave_empty_bold(p_ptr->dun_depth, y, x)) continue;

                /* Hack -- no creation on glyph of warding */
                if (cave[p_ptr->dun_depth][y][x].feat == FEAT_GLYPH) continue;
#endif


                if ((p_ptr->px == x) || (p_ptr->py == y)) continue;

                break;
        }

	/* Access the location */
#ifdef NEW_DUNGEON
        c_ptr = &zcave[y][x];
#else
        c_ptr = &cave[p_ptr->dun_depth][y][x];
#endif

	/* Make a new monster */
	c_ptr->m_idx = m_pop();

	/* Mega-Hack -- catch "failure" */
        if (!c_ptr->m_idx) return;

        /* Grab and allocate */
        m_ptr = &m_list[c_ptr->m_idx];
        MAKE(m_ptr->r_ptr, monster_race);
        m_ptr->special = TRUE;
        m_ptr->fx = x;
        m_ptr->fy = y;

        r_ptr = m_ptr->r_ptr;

        r_ptr->flags1 = 0;
        r_ptr->flags2 = 0;
        r_ptr->flags3 = 0;
        r_ptr->flags4 = 0;
        r_ptr->flags5 = 0;
        r_ptr->flags6 = 0;

        msg_print(Ind, "Some of your items begins to consume in roaring flames.");

        /* Find items used for "golemification" */
        for (i = 0; i < INVEN_WIELD; i++)
        {
                object_type *o_ptr = &p_ptr->inventory[i];

                if (o_ptr->tval == TV_GOLEM)
                {
                        if (o_ptr->sval <= SV_GOLEM_ADAM)
                        {
                                golem_type = o_ptr->sval;
				inven_item_increase(Ind,i,-o_ptr->number);
				inven_item_optimize(Ind,i);
				i--;
				continue;
                        }
                        if (o_ptr->sval == SV_GOLEM_ARM)
                        {
                                int k;

                                for (k = 0; k < o_ptr->number; k++){
					if(golem_m_arms==4) break;
                                        golem_arms[golem_m_arms++] = o_ptr->pval;
				}
				inven_item_increase(Ind,i,-o_ptr->number);
				inven_item_optimize(Ind,i);
				i--;
				continue;
                        }
                        if (o_ptr->sval == SV_GOLEM_LEG)
                        {
                                int k;

                                for (k = 0; k < o_ptr->number; k++){
					if(golem_m_legs==30) break;
                                        golem_legs[golem_m_legs++] = o_ptr->pval;
				}
				inven_item_increase(Ind,i,-o_ptr->number);
				inven_item_optimize(Ind,i);
				i--;
				continue;
                        }
                        else
                        {
                                golem_flags |= 1 << (o_ptr->sval - 200);
                        }
                }
		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);
        }

        /* Ahah FAIL !!! */
        if ((golem_type == -1) || (golem_m_legs < 2))
        {
                msg_print(Ind, "The spell fails! You lose all your material.");
		delete_monster_idx(c_ptr->m_idx);
                return;
        }

        r_ptr->text = 0;
        r_ptr->name = 0;
        r_ptr->sleep = 0;
        r_ptr->aaf = 20;
        r_ptr->speed = 100;
        for (i = 0; i < golem_m_legs; i++)
        {
                r_ptr->speed += golem_legs[i];
        }
        r_ptr->mexp = 1;

        r_ptr->d_attr = TERM_YELLOW;
        r_ptr->d_char = 'g';
        r_ptr->x_attr = TERM_YELLOW;
        r_ptr->x_char = 'g';

        r_ptr->freq_inate = 0;
        r_ptr->freq_spell = 0;
        r_ptr->flags1 |= RF1_FORCE_MAXHP;
        r_ptr->flags2 |= RF2_STUPID | RF2_EMPTY_MIND | RF2_REGENERATE | RF2_POWERFUL | RF2_BASH_DOOR | RF2_MOVE_BODY;
        r_ptr->flags3 |= RF3_HURT_ROCK | RF3_IM_COLD | RF3_IM_ELEC | RF3_IM_POIS | RF3_NO_FEAR | RF3_NO_CONF | RF3_NO_SLEEP | RF9_IM_TELE;

        switch (golem_type)
        {
                case SV_GOLEM_WOOD:
                        r_ptr->hdice = 10;
                        r_ptr->hside = 10;
                        r_ptr->ac = 20;
                        break;
                case SV_GOLEM_COPPER:
                        r_ptr->hdice = 10;
                        r_ptr->hside = 20;
                        r_ptr->ac = 40;
                        break;
                case SV_GOLEM_IRON:
                        r_ptr->hdice = 10;
                        r_ptr->hside = 40;
                        r_ptr->ac = 70;
                        break;
                case SV_GOLEM_ALUM:
                        r_ptr->hdice = 10;
                        r_ptr->hside = 60;
                        r_ptr->ac = 90;
                        break;
                case SV_GOLEM_SILVER:
                        r_ptr->hdice = 10;
                        r_ptr->hside = 70;
                        r_ptr->ac = 100;
                        break;
                case SV_GOLEM_GOLD:
                        r_ptr->hdice = 10;
                        r_ptr->hside = 80;
                        r_ptr->ac = 130;
                        break;
                case SV_GOLEM_MITHRIL:
                        r_ptr->hdice = 10;
                        r_ptr->hside = 100;
                        r_ptr->ac = 160;
                        break;
                case SV_GOLEM_ADAM:
                        r_ptr->hdice = 10;
                        r_ptr->hside = 150;
                        r_ptr->ac = 210;
                        break;
//		default:
        }
        r_ptr->extra = golem_flags;
//#if 0
        /* Find items used for "golemification" */
        for (i = 0; i < INVEN_WIELD; i++)
        {
                object_type *o_ptr = &p_ptr->inventory[i];
                unsigned char *inscription = (unsigned char *) quark_str(o_ptr->note);

                /* Additionnal items ? */
                if (inscription != NULL)
                {
                        /* scan the inscription for @P */
                        while ((*inscription != '\0'))
                        {
		
                                if (*inscription == '@')
                                {
                                        inscription++;
			
                                        /* a valid @G has been located */
                                        if (*inscription == 'G')
                                        {                
                                                inscription++;

                                                scan_golem_flags(o_ptr, r_ptr);
						inven_item_increase(Ind,i,-o_ptr->number);
						inven_item_optimize(Ind,i);
						i--;
						continue;
                                        }
                                }
                                inscription++;
                        }
                }
        }
//#endif
        m_ptr->speed = r_ptr->speed;
        m_ptr->mspeed = m_ptr->speed;
        m_ptr->ac = r_ptr->ac;
        m_ptr->maxhp = maxroll(r_ptr->hdice, r_ptr->hside);
        m_ptr->hp = maxroll(r_ptr->hdice, r_ptr->hside);
        m_ptr->clone = TRUE;

        for (i = 0; i < golem_m_arms; i++)
        {
                m_ptr->blow[i].method = r_ptr->blow[i].method = RBM_HIT;
                m_ptr->blow[i].effect = r_ptr->blow[i].effect = RBE_HURT;
                m_ptr->blow[i].d_dice = r_ptr->blow[i].d_dice = (golem_type + 1) * 3;
                m_ptr->blow[i].d_side = r_ptr->blow[i].d_side = 3 + golem_arms[i];
        }

        m_ptr->owner = p_ptr->id;
        m_ptr->r_idx = 1 + golem_type;

        m_ptr->level = p_ptr->lev;
        m_ptr->exp = MONSTER_EXP(m_ptr->level);

	/* Assume no sleeping */
	m_ptr->csleep = 0;
#ifdef NEW_DUNGEON
	wpcopy(&m_ptr->wpos, &p_ptr->wpos);
#else
        m_ptr->dun_depth = p_ptr->dun_depth;
#endif

	/* No "damage" yet */
	m_ptr->stunned = 0;
	m_ptr->confused = 0;
	m_ptr->monfear = 0;

	/* No knowledge */
	m_ptr->cdis = 0;
        m_ptr->mind = GOLEM_NONE;

	/* Update the monster */
	update_mon(c_ptr->m_idx, TRUE);
}

/* pernAngband Additions	- Jir - */
void call_chaos(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];
	int Chaos_type, dummy;
	int plev = p_ptr->lev;
	bool line_chaos = FALSE;

	int hurt_types[26] =	// 30
	{
		GF_ELEC,      GF_POIS,    GF_ACID,    GF_COLD,
		GF_FIRE,      GF_MISSILE, GF_ARROW,   GF_PLASMA,
//		GF_HOLY_FIRE, GF_WATER,   GF_LITE,    GF_DARK,
		GF_WATER,   GF_LITE,    GF_DARK,
		GF_FORCE,     GF_INERTIA, GF_MANA,    GF_METEOR,
		GF_ICE,       GF_CHAOS,   GF_NETHER,  GF_DISENCHANT,
		GF_SHARDS,    GF_SOUND,   GF_NEXUS,   GF_CONFUSION,
//		GF_TIME,      GF_GRAVITY, GF_ROCKET,  GF_NUKE,
		GF_TIME,      GF_GRAVITY, GF_ROCKET,
//		GF_HELL_FIRE, GF_DISINTEGRATE
	};

	Chaos_type = hurt_types[randint(26) - 1];
	if (randint(4) == 1) line_chaos = TRUE;

#if 0
	/* Probably a meaningless line, a remnant from earlier code */
	while (Chaos_type > GF_GRAVITY && Chaos_type < GF_ROCKET);
#endif

	if (randint(6) == 1)
	{
		for (dummy = 1; dummy < 10; dummy++)
		{
			if (dummy-5)
			{
				if (line_chaos)
					fire_beam(Ind, Chaos_type, dummy, 75);
				else
					fire_ball(Ind, Chaos_type, dummy, 75, 2);
			}
		}
	}
	else if (randint(3)==1)
	{
		fire_ball(Ind, Chaos_type, 0, 300, 8);
	}
	else
	{
//		if (!get_aim_dir(&dir)) return;
		if (line_chaos)
			fire_beam(Ind, Chaos_type, dir, 150);
		else
			fire_ball(Ind, Chaos_type, dir, 150, 3 + (plev/35));
	}
}

void summon_cyber(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int i;
	int max_cyber = (getlevel(&p_ptr->wpos)/ 50) + randint(6);

	for (i = 0; i < max_cyber; i++)
	{
		(void)summon_specific(&p_ptr->wpos, p_ptr->py, p_ptr->px, 100, SUMMON_HI_DEMON);
	}
}

/* Heal insanity. */
bool heal_insanity(int Ind, int val)
{
	player_type *p_ptr = Players[Ind];

	if (p_ptr->csane < p_ptr->msane) {
		p_ptr->csane += val;

		if (p_ptr->csane >= p_ptr->msane) {
			p_ptr->csane = p_ptr->msane;
			p_ptr->csane_frac = 0;
		}

		p_ptr->redraw |= PR_SANITY;
		p_ptr->window |= (PW_SPELL | PW_PLAYER);

		if (val < 5) {
			msg_print(Ind, "You feel a little saner.");
		} else if (val < 15) {
			msg_print(Ind, "You feel saner.");
		} else if (val < 35) {
			msg_print(Ind, "You feel much saner.");
		} else {
			msg_print(Ind, "You feel very sane.");
		}

		return TRUE;
	}

	return FALSE;
}

