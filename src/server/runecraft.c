/* Purpose: New runemaster class functionality */

/* by Relsiet/Andy Dubbeld (andy@foisdin.com) */

/*
New runemastery class/spell system

Spells are created on the fly with an mkey interface as a combination of up to three rune elements, a modifier and a spell type.

The spell characteristics (damage, radius and duration, fail-rate, cost, etc) of a spell are decided by the average of the caster's skill level in their rune skills, and are further modified by the 'successfulness' of a given cast (or how near/far you were to failing).

Spells can always be attempted, but penalties are applied for failing cast an attempted spell, and difficulty increases when the caster is impaired (Missing a rune, blind, confused, stunned, not enough mp, etc).

This makes runemastery quite versatile, but also quite risky.

Primary stats are Int and Dex.

A high level rune-master (lv 50) should only have a few runes skilled > 40.
*/

#include "angband.h"

#ifdef ENABLE_RCRAFT

#define R_CAP 60 // / 100

/* Limit adventurers so they cannot utilize 3-runes-spells?
   Reason is that one rune school might give them ~ 15 utility spells,
   which is about 3x as much as they'd get from any other spell school. */
#define LIMIT_NON_RUNEMASTERS


byte execute_rspell (u32b Ind, byte dir, u32b spell, byte imperative);
s16b rspell_time(u32b Ind, byte imperative);
bool is_attack(u32b s_flags);
u16b cast_runespell(u32b Ind, byte dir, u16b damage, u16b radius, u16b duration, s16b cost, u32b type, s16b diff, byte imper, u32b type_flags, u16b s_av, s16b mali);
u32b rspell_type(u32b flags);
s16b rspell_diff(u32b Ind, byte imperative, s16b s_cost, u16b s_av, u32b s_type, u32b s_flags, s16b * mali);
u16b rspell_skill(u32b Ind, u32b s_flags);
u16b rspell_do_penalty(u32b Ind, byte type, u16b damage, u16b duration, s16b cost, u32b s_type, char * attacker, byte imperative, u32b s_flags);
byte rspell_penalty(u32b Ind, u16b pow);
u16b rspell_dam(u32b Ind, u16b *radius, u16b *duration, u16b s_type, u32b s_flags, u16b s_av, byte imperative);
s16b rspell_cost(u32b Ind, u16b s_type, u32b s_flags, u16b s_av, byte imperative);
byte rspell_check(u32b Ind, s16b * mali, u32b s_flags);
byte meth_to_id(u32b s_meth);
byte runes_in_flag(byte runes[], u32b flags);
byte rune_value(byte runes[]);
int b_compare (const void * a, const void * b); //Byte comparison for qsort

s16b rspell_time(u32b Ind, byte imperative)
{
	player_type * p_ptr = Players[Ind];
	
	int cast_time = r_imperatives[imperative].time;

	return (level_speed(&p_ptr->wpos) * cast_time) / 10;
}

byte runes_in_flag(byte runes[], u32b flags)
{
	if ((flags & R_ACID) == R_ACID) { runes[0] = 1; } else { runes[0] = 0; }
	if ((flags & R_ELEC) == R_ELEC) { runes[1] = 1; } else { runes[1] = 0; }
	if ((flags & R_FIRE) == R_FIRE) { runes[2] = 1; } else { runes[2] = 0; }
	if ((flags & R_COLD) == R_COLD) { runes[3] = 1; } else { runes[3] = 0; }
	if ((flags & R_POIS) == R_POIS) { runes[4] = 1; } else { runes[4] = 0; }
	if ((flags & R_FORC) == R_FORC) { runes[5] = 1; } else { runes[5] = 0; }
	if ((flags & R_WATE) == R_WATE) { runes[6] = 1; } else { runes[6] = 0; }
	if ((flags & R_EART) == R_EART) { runes[7] = 1; } else { runes[7] = 0; }
	if ((flags & R_CHAO) == R_CHAO) { runes[8] = 1; } else { runes[8] = 0; }
	if ((flags & R_NETH) == R_NETH) { runes[9] = 1; } else { runes[9] = 0; }
	if ((flags & R_NEXU) == R_NEXU) { runes[10] = 1; } else { runes[10] = 0; }
	if ((flags & R_TIME) == R_TIME) { runes[11] = 1; } else { runes[11] = 0; }

	/*
	if ((flags & R_FIRE) == R_FIRE) { runes[0] = 1; } else { runes[0] = 0; }
	if ((flags & R_COLD) == R_COLD) { runes[1] = 1; } else { runes[1] = 0; }
	if ((flags & R_ACID) == R_ACID) { runes[2] = 1; } else { runes[2] = 0; }
	if ((flags & R_WATE) == R_WATE) { runes[3] = 1; } else { runes[3] = 0; }
	if ((flags & R_ELEC) == R_ELEC) { runes[4] = 1; } else { runes[4] = 0; }
	if ((flags & R_EART) == R_EART) { runes[5] = 1; } else { runes[5] = 0; }
	if ((flags & R_POIS) == R_POIS) { runes[6] = 1; } else { runes[6] = 0; }
	if ((flags & R_WIND) == R_WIND) { runes[7] = 1; } else { runes[7] = 0; }
	if ((flags & R_MANA) == R_MANA) { runes[8] = 1; } else { runes[8] = 0; }
	if ((flags & R_CHAO) == R_CHAO) { runes[9] = 1; } else { runes[9] = 0; }
	if ((flags & R_FORC) == R_FORC) { runes[10] = 1; } else { runes[10] = 0; }
	if ((flags & R_GRAV) == R_GRAV) { runes[11] = 1; } else { runes[11] = 0; }
	if ((flags & R_NETH) == R_NETH) { runes[12] = 1; } else { runes[12] = 0; }
	if ((flags & R_TIME) == R_TIME) { runes[13] = 1; } else { runes[13] = 0; }
	if ((flags & R_MIND) == R_MIND) { runes[14] = 1; } else { runes[14] = 0; }
	if ((flags & R_NEXU) == R_NEXU) { runes[15] = 1; } else { runes[15] = 0; }
 */
	return 0;
}

byte rune_value(byte runes[])
{
	byte i,count;
	count=0;
	for(i=0;i<RCRAFT_MAX_ELEMENTS;i++)
		if (runes[i])
			count+=r_elements[i].cost;
	return count;
}

/* rspell_check

Checks if the runespells are in the player's possession

Each missing rune is worth 20% extra to the fail counter, plus an additional 10 if there are any.

It should be possible to cast a runespell without runes in situations of dire need, but too risky to attempt otherwise.
*/
byte rspell_check(u32b Ind, s16b * mali, u32b s_flags)
{
	player_type * p_ptr = Players[Ind];
	s16b i, k, j;
	object_type	*o_ptr;
	s16b p_runes[RCRAFT_MAX_ELEMENTS];
	s16b p_rc = 0;
	byte runes[RCRAFT_MAX_ELEMENTS];
	
	for (j = 0; j < INVEN_TOTAL; j++)
	{
		if (j >= INVEN_PACK)
			continue;
		
		o_ptr = &p_ptr->inventory[j];

		if (o_ptr->tval == TV_RUNE2)
		{
			p_runes[p_rc] = o_ptr->sval;
			p_rc++;
		}
	}
	
	runes_in_flag(runes,s_flags);
	/* Type check */
	for(i=0;i<RCRAFT_MAX_ELEMENTS;i++)
	{
		if (runes[i]==1)
		{
			k=0;
			for (j = 0; j < p_rc; j++)
				if (p_runes[j] == r_elements[i].id)
					k = 1;
			if (!k)
				*mali += 10;
		}
	}

	if (!*mali)
		return 1;
	return 0;
}


/* meth_to_id()
Converts from the method flags to a method index.
To rectify a design mistake.
*/
byte meth_to_id(u32b s_meth)
{
	int m = 0;
	if (s_meth & R_MELE) m = 0;
	else if (s_meth & R_SELF) m = 1;
	else if (s_meth & R_BOLT) m = 2;
	else if (s_meth & R_BEAM) m = 3;

	else if (s_meth & R_BALL) m = 4;
	else if (s_meth & R_WAVE) m = 5;
	else if (s_meth & R_CLOU) m = 6;
	else if (s_meth & R_STOR) m = 7;
		
	return m;
}


/*
	rspell_cost

	Determines mp cost of a given spell
	
	Cost should be determined by a few criteria:
	
	1. Number of runes in a spell (complexity) + (spell effect level + type level) / 2 + modifier level
	2. Level of the spell effect--
	3. Average skill level of player
	4. Cost multipliers (effect, method & imperative)

	Currently cost = (#1 + 30) * skill_level / 50 * #4
*/
s16b rspell_cost (u32b Ind, u16b s_type, u32b s_flags, u16b s_av, byte imperative)
{
	player_type *p_ptr = Players[Ind];
	byte m = meth_to_id(s_flags);
	byte runes[RCRAFT_MAX_ELEMENTS];
	byte value = 0;	
	byte penalty = 0;
	s16b cost = 0;
	
	int type_level = runespell_types[m].cost;
	int mod_level = r_imperatives[imperative].level;
	int effect_level = runespell_list[s_type].level;
	int total_level = type_level + effect_level + mod_level;
	
	runes_in_flag(runes,s_flags);
	value = rune_value(runes);
	
	byte t_pen = 0; byte s_pen = 0; byte d_pen = 0;
	
	if (!s_av) s_av = 1;
	
	cost = rget_level(30 + (type_level)/2 + mod_level);
	
	t_pen = runespell_types[m].pen;
	s_pen = runespell_list[s_type].pen;
	d_pen = r_imperatives[imperative].cost ? r_imperatives[imperative].cost : randint(15)+5;
	
	if (cost > S_COST_MAX)
		cost = S_COST_MAX;
	
	cost = (cost * t_pen) / 10;
	cost = (cost * s_pen) / 10;
	cost = (cost * d_pen) / 10;
	
	/* Alternative to these just increasing fail rates: */
	if (no_lite(Ind) || p_ptr->blind)
	{
		penalty += 20;
	}
	if (p_ptr->confused)
	{
		penalty += 20;
	}
	if (p_ptr->cut)
	{
		penalty += p_ptr->cut / 5;
	}
	if (p_ptr->stun > 50)
	{
		penalty += 20;
	}
	else if (p_ptr->stun)
	{
		penalty += 10;
	}
	
	cost = cost+((cost*penalty)/100); //Costs can increase up to 60% if everything is going wrong.
	
	if (cost < total_level)
		cost = total_level;
	
	return cost;
}

u16b rspell_dam (u32b Ind, u16b *radius, u16b *duration, u16b s_type, u32b s_flags, u16b s_av, byte imperative)
/*
	Determines the damage, radius and duration of a given spell

	Should follow a similar curve to mage-spells.
*/
{
	u16b e_level = runespell_list[s_type].level;
	byte runes[RCRAFT_MAX_ELEMENTS];
	runes_in_flag(runes, s_flags);
	int damage = 1;
	int base_radius = runespell_list[s_type].radius;
	
	int r = *radius;
	int dur = *duration;
	
	u16b d_multiplier = runespell_list[s_type].dam;
	u16b t_multiplier = r_imperatives[imperative].duration;
	s16b r_mod = r_imperatives[imperative].radius;
	
	if (r_mod == 5)
		r_mod = randint(3) - 2;
	
	if (s_av < e_level + 1)
		s_av = e_level + 1; //Give a minimum of damage
	
	if ((s_flags & R_BOLT) == R_BOLT) 
	{
		damage = damroll(3 + rget_level(45), 1 + rget_level(20));
	}
	else if ((s_flags & R_BEAM) == R_BEAM)
	{
		//damage = damroll(3 + rget_level(40), 1 + rget_level(20));
		damage = damroll(3 + rget_level(40), 1 + rget_level(30));
	}
	else if ((s_flags & R_SELF) == R_SELF)
	{
		damage = randint(20) + rget_level(136);
		//dur = randint(50) + rget_level(75);
		dur = randint(25) + rget_level(75);
	}
	else if ((s_flags & R_BALL) == R_BALL)
	{
		damage = rget_level(400 + randint(50));
		r = rget_level(base_radius);
	}
	else if ((s_flags & R_CLOU) == R_CLOU)
	{
		r = rget_level(base_radius);
		dur = randint(10) + rget_level(10);
		
		/* Damage should be proportional to radius and duration. */
		damage = randint(4) + rget_level(80 - (*radius + *duration) / 2);
	}
 	else if ((s_flags & R_WAVE) == R_WAVE)
	{
		/* Wave uses duration, instead of radius. */
		dur = rget_level(15) / 3;
		
		/* Damage should be proportional to radius and duration. */
		damage = randint(4) + rget_level(90 - (*radius + *duration) / 2);
	}
	else if ((s_flags & R_STOR) == R_STOR)
	{
		//damage = e_level + 3 + rget_level(100);
		r = rget_level(base_radius);
		dur = (randint(10) + rget_level(10));
		
		/* Damage should be proportional to radius and duration. (and interval) */
		damage = randint(4) + rget_level(40 - (*radius + *duration) / 2);
	}
	else //R_MELE
	{
		damage = damroll(3 + rget_level(50), 5 + rget_level(20));
	}
	
	if (r < 1 || r > 5)
	{
		r = 1;
	}
	
	if ((s_flags & R_WAVE) == R_WAVE) //Waves are cheaper standing clouds, with a slight boost to radius.
	{
		//duration IS radius only for the new wave, don't boost! - Kurzel
		//r += 1;
		//r = (r < 2 ? 2 : r);
		dur += 1;
	}
	else if ((s_flags & R_BALL) == R_BALL)
	{
		r += 1;
	}
	
	if (damage > S_DAM_MAX)
	{
		damage = S_DAM_MAX;
	}
	
	if (damage < 1)
	{
		damage = 1;
	}
	
	damage = (damage * (r_imperatives[imperative].dam != 0 ? r_imperatives[imperative].dam : randint(15)+5)) / 10;
	damage = (damage * d_multiplier) / 10;
	dur = (dur * t_multiplier) / 10;
	
	if (r_mod >= 0 || (r_mod < 0 && r > (0 - r_mod)))
	{
		r = r + r_mod;
	}
	else
	{
		r = 1;
	}
	
	if (r > 5)
	{
		r = 5;
	}
	
	if (dur > 260 || dur < 5)
	{
		dur = 5;
	}
	
	*duration = dur;
	*radius = r;
	
	return damage;
}


/* rspell_penalty

Generates a penalty, or a number of them based on the severity of the offense.

Penalties generated by low fail rates are less severe; trying a spell with a fail rate above 30 can be fatal, occasionally.

The chances of something bad happening increase for every 20% of failure. (i.e. it rolls more than once, depending on severity)

Returns a byte of penalty flags.

#define RPEN_MIN_RN 0x01 //Rune destruction
#define RPEN_MIN_SP 0x02 //Spell points
#define RPEN_MIN_HP 0x04 //Health points
#define RPEN_MIN_ST 0x08 //Stat drain

#define RPEN_MAJ_SN 0x10 //Sanity drain
#define RPEN_MAJ_BK 0x20 //Spell backlash
#define RPEN_MAJ_BB 0x40 //Black breath
#define RPEN_MAJ_DT 0x80 //Death?

No longer deal serious penalties when a rune can be broken instead. - Kurzel
*/
byte rspell_penalty(u32b Ind, u16b pow)
{
	player_type *p_ptr = Players[Ind];
	u16b roll = 0;
	u16b penalties = 1;
	u16b threshold = 60;
	u16b i = 0;
	u16b check = 0;
	byte penalty_flag = 0;
	
	if (pow > threshold)
	{
		penalties += ((pow - threshold) / 20);
	}
	
	for(i=0; i<penalties; i++)
	{
		roll = randint(pow);
		
		check = randint(100);
		if (check > (10 + p_ptr->luck))
		{
			if (roll > 320)
			{
				penalty_flag |= RPEN_MAJ_DT;
				penalty_flag |= RPEN_MIN_RN;
				break;
			}
			else if (roll > 270)
			{
				penalty_flag |= RPEN_MAJ_BB;
				penalty_flag |= RPEN_MIN_RN;
				break;
			}
			else if (roll > 220)
			{
				penalty_flag |= RPEN_MAJ_BK;
				penalty_flag |= RPEN_MIN_RN;
				break;
			}
			else if (roll > 160)
			{
				penalty_flag |= RPEN_MAJ_SN;
				penalty_flag |= RPEN_MIN_RN;
				break;
			}
			else if (roll > 110)
			{
				penalty_flag |= RPEN_MIN_ST;
				penalty_flag |= RPEN_MIN_RN;
				break;
			}
			else if (roll > 80)
			{
				penalty_flag |= RPEN_MIN_HP;
				break;
			}
			else if (roll > 50)
			{
				penalty_flag |= RPEN_MIN_SP;
				break;
			}
			else if (roll > 25)
			{
				penalty_flag |= RPEN_MIN_RN;
				break;
			}
		}
	}
	return penalty_flag;
}


/* rspell_do_penalty
Executes the penalties worked out in rspell_penalty()

Destroys runes, inflicts damage and other negative effects on player Ind.
No longer deal serious penalties when a rune can be broken instead. - Kurzel

*/
u16b rspell_do_penalty(u32b Ind, byte type, u16b damage, u16b duration, s16b cost, u32b s_type, char * attacker, byte imperative, u32b s_flags)
{
	player_type *p_ptr = Players[Ind];
	int mod_luck = p_ptr->luck+10; //Player's current luck as a positive value between 0 and 50
	u16b d = 0;
	
	byte runes[RCRAFT_MAX_ELEMENTS];
	runes_in_flag(runes,s_flags);
	
	if (duration<5)
		duration = 5;
	
	if (damage<10)
		damage = 10;
	
	int amt = 0;
	
	if (type & RPEN_MIN_RN)
	{
		int i;
		object_type	*o_ptr;
		char o_name[160];

		for (i = 0; i < INVEN_TOTAL; i++)	/* Modified version of inven_damage from spells1.c */
		{
			if (i >= INVEN_PACK)
				continue;

			/* Get the item in that slot */
			o_ptr = &p_ptr->inventory[i];

			/* Give this item slot a shot at death */
			if (o_ptr->k_idx)
			{
				
				if (o_ptr->tval == TV_RUNE2)
				{
					if (o_ptr->sval <=15)
					{
						if (runes[o_ptr->sval]==1)
						{
							//if (rand_int(100) < 70) //Always break a rune if able. - Kurzel
							//{
								/* Select up to a third */
								amt = 0; //Only break 1 rune, but more often - Kurzel */
								//amt = rand_int(o_ptr->number/3);
								
								if (amt == 0 && o_ptr->number >= 1)
									amt = 1;
							//}
						}
					}
				}
				
				/* Break */
				if (amt)
				{
					/* Get a description */
					object_desc(Ind, o_name, o_ptr, FALSE, 3);

					/* Message */
					msg_format(Ind, "\376\377o%sour %s (%c) %s destroyed!", ((o_ptr->number > 1) ? ((amt == o_ptr->number) ? "All of y" : (amt > 1 ? "Some of y" : "One of y")) : "Y"), o_name, index_to_label(i), ((amt > 1) ? "were" : "was"));
					
					inven_item_increase(Ind, i, -amt); inven_item_optimize(Ind, i);
					
					i = INVEN_TOTAL;
					break;
				}
			}
		}
		/* If we didn't break any runes, take some HP */
		if (amt == 0)
		{
			type |= RPEN_MIN_HP;
		}
	}
	if (type & RPEN_MIN_SP)
	{
		msg_print(Ind, "\377rYou feel a little drained.");
		d = randint(20)+1;
		if (d == 20)
		{
			//set_paralyzed(Ind, 2);
			set_paralyzed(Ind, 1); //A single turn may still be too long!
		}
		else if (d >= 17)
		{
			set_stun(Ind, duration/5);
		}
		else if (d >= 15)
		{
			set_slow(Ind, duration/4);
		}
		
		if (p_ptr->csp>(cost*2))
		{
			p_ptr->csp -= (cost*2);
		}
		else
		{
			p_ptr->csp = 0;
			type |= RPEN_MIN_HP;
		}
	}
		
	if (type & RPEN_MIN_HP)
	{
		
		d = randint(100);
		if (d > 93 && !p_ptr->resist_pois)
		{
			msg_print(Ind, "\377rThe runespell lashes out at you!");
			if(duration/3 < 10)
				duration = 30;
			set_poisoned(Ind, duration/3, Ind);
		}
		else if (d > 83 && !p_ptr->resist_blind && !p_ptr->resist_lite)
		{
			msg_print(Ind, "\377rThe runespell lashes out at you!");
			set_blind(Ind, damage/3);
		}
		else if (d > 73)
		{
			msg_print(Ind, "\377rThe runespell lashes out at you!");
			set_cut(Ind, damage/3, Ind);
		}
		else
		{
			//int hit = (p_ptr->mhp*(randint(15)+1)/100);
			//msg_format(Ind, "\377rThe runespell hits you for \377u%i \377rdamage!", hit);
			//take_hit(Ind, hit, "a malformed invocation", 0); //match GF_type for damage here? - Kurzel
			project(PROJECTOR_RUNE, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px, damage/5, s_type, PROJECT_KILL, "");
			//static bool project_p(int Ind, int who, int r, struct worldpos *wpos, int y, int x, int dam, int typ, int rad, int flg, char *attacker)
		}
		
		p_ptr->redraw |= PR_HP;
	}
		
	if (type & RPEN_MIN_ST)
	{
		d = 1;
		u16b mode = 0; u16b stat = 0;
		
		if (randint(40) > 2)
			mode = STAT_DEC_TEMPORARY;
		//else
		else if (amt == 0)
			mode = STAT_DEC_PERMANENT;
		
		switch(randint(12+mod_luck))
		{
			case 1:
			case 2:
				stat = A_INT;
				break;
			case 3:
			case 4:
				stat = A_CON;
				break;
			case 5:
			case 6:
				stat = A_WIS;
				break;
			case 7:
			case 8:
				stat = A_CHR;
				break;
			case 9:
			case 10:
				stat = A_STR;
				break;
			case 11:
			case 12:
				stat = A_DEX;
				break;
			default:
				d = 0;
				break;
		}
		
		if (d)
		{
			msg_print(Ind, "\377rYou feel a little less potent.");
			do_dec_stat(Ind, stat, mode);
		}
	}
	/* Hurt sanity. With luck it may only confuse, scare or cause hallucinations. Should be less dangerous at low levels. */
	//if (type & RPEN_MAJ_SN)
	if ((type & RPEN_MAJ_SN) && (amt == 0))
	{
		msg_print(Ind, "\377rYou feel a little less sane!");
		//d = damroll(1,(p_ptr->lev*(55/mod_luck)));
		d = damroll(1,(p_ptr->lev*(25/mod_luck))); //Reduced to avoid 1 shot vegetation at high levels! ;) - Kurzel
		if (d<= 3)
		{
			set_image(Ind, duration/5);
		}
		else if (d <= 6)
		{
			set_afraid(Ind, duration/5);
		}
		else if (d <= 9)
		{
			set_confused(Ind, duration/5);
		}
		else
		{
			take_sanity_hit(Ind, damroll(1,d), "a malformed invocation");
		}
	}
		
	if ((type & RPEN_MAJ_BK || type & RPEN_MAJ_BB) && (amt == 0))
	{
		//msg_format(Ind, "\377rYour grasp on the invocation slips! It hits you for \377u%i \377rdamage!", damage);
		//take_hit(Ind, damage, "a malformed invocation", 0); //match GF_type for damage here? - Kurzel
		project(PROJECTOR_RUNE, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px, damage, s_type, PROJECT_KILL, "");
	}
	/*
	if ((type & RPEN_MAJ_DT) && (amt == 0)) //Disabled for now. - Kurzel
	{
		msg_format(Ind, "\377rYou lose control of the invocation! It hits you for \377u%i \377rdamage!", damage);
		take_hit(Ind, damage*10, "a malformed invocation", 0);
	}
	*/
	p_ptr->redraw |= PR_HP;
	p_ptr->redraw |= PR_MANA;
	p_ptr->redraw |= PR_SPEED;
	p_ptr->redraw |= PR_SANITY;
	
	handle_stuff(Ind);
	
	return 0;
}

int b_compare (const void * a, const void * b)
{
	return ( *(byte*)b - *(byte*)a );
}

/* rspell_skill

Calculates a skill average value for the requested runes.

Uses a series of formulas created by Kurzel.

Assumes a max of 3 runes in a spell. Needs to be adjusted if this ever changes.
Also assumes that the rune skill flags are in order, starting with FIRECOLD.
*/
u16b rspell_skill(u32b Ind, u32b s_type)
{
	player_type *p_ptr = Players[Ind];
	u16b s = 0; u16b i; u16b skill = 0; u16b value = 0;
	s16b a=0,b=0,c=0;
	
	byte rune_c = 0;
	byte skill_c = 0;
	byte skills[RCRAFT_MAX_ELEMENTS/2] = {0,0,0,0,0,0};
	byte runes[3] = {0,0,0};
	
	byte s_runes[RCRAFT_MAX_ELEMENTS];
	runes_in_flag(s_runes, s_type);
	
	for (i=0; i<RCRAFT_MAX_ELEMENTS; i++)
	{
		if (s_runes[i]==1)
		{
			skill = r_elements[i].skill;
			value = get_skill(p_ptr, skill);
			//Ugh. We need an index from 0-7. FIRECOLD through MINDNEXU is 96-103
			//skills[skill - SKILL_R_FIRECOLD] = 1;
			skills[skill - SKILL_R_ACIDWATE] = 1;
			//ACIDWATE through FORCTIME is 96-101.
			if (rune_c > 2)
			{
				break;
			}
			
			runes[rune_c++] = value;
		}
	}
	
	for (i=0; i<RCRAFT_MAX_ELEMENTS/2; i++)
	{
		if (skills[i]==1)
		{
			skill_c++;
		}
	}
	
	if (rune_c == 1 || (rune_c == 2 && runes[0] == runes[1]) || (rune_c == 3 && (runes[0] == runes[1] && runes[1] == runes[2])))
	{
		return runes[0];
	}
	
	qsort(runes, 3, sizeof(byte), b_compare);
	
	switch (skill_c)
	{
		case 2:
			if (rune_c == 2)
			{
				a = 50 * R_CAP / 100;
				
				s = ((a*runes[0])+((100-a)*runes[1])) / 100;
			}
			else if (rune_c == 3)
			{
				if (runes[1] == runes[2])
				{
					a = 33 * R_CAP / 100;
				}
				else // if (runes[0] == runes[1])
				{
					a = 66 * R_CAP / 100;
				}
				
				s = ((a*runes[0])+((100-a)*runes[1])) / 100;
			}
			break;
		
		case 3:
			a = 33 * R_CAP / 100;
			b = 33;
			c = 100 - a - b;
			
			s = (a*runes[0] + b*runes[1] + c*runes[2]) / 100;
			break;
		
		default:
			s = runes[0];
			break;
	}
	
	return s;
}

/* rspell_diff
Calculates the difficulty of a runespell

takes the player index, imperative index, skill average, spell-effect index, and a pointer to the mali count

returns an int between 0 and 95
*/
s16b rspell_diff(u32b Ind, byte imperative, s16b s_cost, u16b s_av, u32b s_type, u32b s_flags, s16b * mali)
{
	player_type *p_ptr = Players[Ind];
	
	s16b fail = 0;
	s16b penalty = *mali;
	u16b minfail = 0;
	u16b statbonus = 0;
	bool chaotic = (r_imperatives[imperative].id == 7 ? 1 : 0);
	int m = meth_to_id(s_flags);
	int e_level = runespell_list[s_type].level + runespell_types[m].cost + r_imperatives[imperative].level + 10;
	int adj_level = e_level - s_av;
	
	if (p_ptr->confused || p_ptr->stun || p_ptr->cut)
	{
		if (*mali > 0)
		{
			msg_print(Ind, "\377yYou struggle to remember the rune-forms.");
		}
		else
		{
			msg_print(Ind, "\377yYou struggle to recite the rune-forms.");
		}
	}
	else if (*mali > 0)
	{
		msg_print(Ind, "\377yYou recite the rune-forms from memory.");
	}
	
	statbonus += adj_mag_stat[p_ptr->stat_ind[A_INT]]*65/100;
	statbonus += adj_mag_stat[p_ptr->stat_ind[A_DEX]]*35/100;
	
	minfail += adj_mag_fail[p_ptr->stat_ind[A_INT]]*65/100;
	minfail += adj_mag_fail[p_ptr->stat_ind[A_DEX]]*35/100;
	
	penalty += (p_ptr->csp < s_cost ? (2*(s_cost - p_ptr->csp)) : 0);
	penalty += (p_ptr->rune_num_of_buffs ? p_ptr->rune_num_of_buffs * 5 + 3 : 0);
	penalty += (p_ptr->csp == 0 ? 5 : 0);
	penalty += (p_ptr->confused ? 10 : 0);
	penalty += (p_ptr->stun > 50 ? 5 : 0);
	penalty += (p_ptr->stun ? 5 : 0);
	
	fail = (adj_level > 0 ? adj_level * 3 : adj_level);
	
	fail -= statbonus;
	
	fail += penalty;
	fail += (chaotic ? randint(20) : r_imperatives[imperative].fail);
	fail += runespell_list[s_type].fail;
	
	if (fail > 95) fail = 95;
	
	/* Bonus for >50 players. */
	fail -= (p_ptr->lev > 50 ? (p_ptr->lev - 50) / 2 : 0);
	
	if (fail < minfail) fail = minfail;
	if (fail > 95) fail = 95;
	
	*mali = penalty;
	return fail;
}

/* rspell_type

Selects a spell based on the rspell_selector[] array 
*/
u32b rspell_type (u32b flags)
{
	u16b i;
	
	for (i = 0; i<MAX_RSPELL_SEL; i++)
	{
		if (rspell_selector[i].flags)
		{
			if ((rspell_selector[i].flags & flags) == rspell_selector[i].flags)
			{
				return rspell_selector[i].type;
			}
		}
	}
	return RT_NONE;
}

u16b cast_runespell(u32b Ind, byte dir, u16b damage, u16b radius, u16b duration, s16b cost, u32b type, s16b difficulty, byte imper, u32b type_flags, u16b s_av, s16b mali)
/* Now that we have some numbers and figures to work with, cast the spell. MP is deducted here, and negative spell effects/failure stuff happens here. */
{
	u16b m;
//	u16b y, x;
	u16b fail_chance = 0;
	s16b margin = 0;
	s16b modifier = 100;
	s16b cost_m = 0;
	int success = 0;
//	int t = 0;
//	int brand_type = 0;
//	int d = 0;
	char * description = NULL;
	char * begin = NULL;
	fail_chance = randint(100);
	player_type * p_ptr = Players[Ind];
	//u16b gf_type = runespell_list[type].gf_type;
	u32b gf_type = runespell_list[type].gf_type; //Hack -- More info to pass! - Kurzel
	u32b gf_explode = runespell_list[type].gf_explode;
	u16b e_level = runespell_list[type].level;
	m = meth_to_id(type_flags);
	e_level += runespell_types[m].cost;
	e_level += r_imperatives[imper].level;
	
	int modifier_value = r_imperatives[imper].dam;
	modifier_value = (modifier_value == 0 ? randint(11) + 4 : modifier_value);
	
	s16b speed_max = modifier_value; //6 - 15
	s16b speed = 0;
	speed = damage / 13;
	speed = (speed > speed_max ? speed_max : speed);
	speed = (speed < 2 ? 2 : speed);	
	
	s16b shield_max = modifier_value * 3;//18 - 45
	s16b shield = 0;
	shield = damage / 20;
	shield = shield > shield_max ? shield_max : shield;
	shield = shield < 5 ? 5 : shield;
	
//	s16b spell_duration = 0;
	s16b spell_damage = 0;
	
	int level = s_av - e_level;
	int teleport_level = (level > 5 ? level : 5);
	
	cave_type **zcave; //For glyph removal function of "disperse"
	if (!(zcave = getcave(&p_ptr->wpos))) return 0;
	
	margin = fail_chance - difficulty;
	if (margin > -30) //For small failure values cast the spell
	{
		success = 1;
	}
	
	if (p_ptr->csp < cost)
	{
		begin = "\377rExhausted\377s, you";
	}
	else
	{
		begin = "\377sYou";
	}
	
	modifier = 100 + margin/6;
	
	if (margin < -80)
	{
		description = " \377rfail \377sto";
		modifier = 100;
	}
	else if (margin < -30)
	{
		description = " fail to";
		modifier = 100;
	}
	else if (margin < 0) //changed from -10 to 0 to avoid 'blank' (no effect/dmg) projections - Kurzel
	{
		description = " barely manage to";
	}
	else if (margin < 10)
	{
		description = " clumsily";
	}
	else if (margin < 60)
	{
		description = "";
	}
	else if (margin < 80)
	{
		description = " effectively";
	}
	else
	{
		description = " elegantly";
	}

	/*
	if ((type_flags & R_SELF) && 
		(((type == RT_MAGIC_CIRCLE) && !allow_terraforming(&p_ptr->wpos, FEAT_GLYPH)) ||
		((type == RT_MAGIC_WARD) && !allow_terraforming(&p_ptr->wpos, FEAT_GLYPH)) ||
		((type == RT_WALLS || type == RT_EARTHQUAKE) && !allow_terraforming(&p_ptr->wpos, FEAT_WALL_EXTRA))))
	{
		description = " decide not to";
		success = 0;
	}
	*/
	if (modifier > 1)
	{
		modifier = modifier + (margin / 10);
		
		damage = (damage * modifier) / 100;
		duration = (duration * modifier) / 100;
		speed = (speed * modifier) / 100;
		speed_max = (speed_max * modifier) / 100;
		shield = (shield * modifier) / 100;
		
		cost_m = 100 - modifier;
		cost += (cost * cost_m) / 100;
		if (cost < 1)
		{
			cost = 1;
		}
	}
	
	if (type_flags & R_SELF)
	{
		/* Handle self spells here */
		switch (type)
		{
      /*
			case RT_PSI_ESP:
				msg_format(Ind, "%s%s cast a %s rune of extra-sensory perception. (%i turns)", begin, description, r_imperatives[imper].name, duration);
				if (success) set_tim_esp(Ind, duration);
				break;
			
			case RT_MAGIC_WARD:
				msg_format(Ind, "%s%s draw a sigil of protection.", begin, description);
				if (success) warding_glyph(Ind);
				break;
			
			case RT_MAGIC_CIRCLE:
				msg_format(Ind, "%s%s surround yourself with protective sigils.", begin, description);
				
				if (success) //change this to circle like trap of walls (no center, perhaps compressed rad 0?) - Kurzel
				{
					for(x = 0; x<3;x++)
					{
						for(y = 0; y<3;y++)
						{
							cave_set_feat_live(&p_ptr->wpos, (p_ptr->py + y - 1), (p_ptr->px + x - 1), FEAT_GLYPH);
						}
					}
//done in cave_set_feat_live() now:	p_ptr->redraw |= PR_MAP;
				}
				else if (!istown(&p_ptr->wpos))
				{
					warding_glyph(Ind); //Failing this spell will probably kill the player: help them out with one glyph
				}
				break;
				
			case RT_WALLS:
				msg_format(Ind, "%s%s summon walls.", begin, description);
			
				if (success)
				{
					fire_ball(Ind, GF_STONE_WALL, 0, 1, 1, ""); //maybe copy this for GF_MAGIC_GLYPH (above) or time+base trap glyphs
				}
				break;
			
			case RT_VISION:
				msg_format(Ind, "%s%s summon %s magical vision.", begin, description, r_imperatives[imper].name);
			
				if (success)
				{
					map_area(Ind);
				}
				break;
				
			case RT_MYSTIC_SHIELD:
				msg_format(Ind, "%s%s summon %s mystic protection. (%i turns)", begin, description, r_imperatives[imper].name, duration);
				
				if (success)
				{
					(void)set_shield(Ind, duration, shield, SHIELD_NONE, 0, 0);
				}
				break;
			
			case RT_SHARDS:
			case RT_DETECT_TRAP:
				msg_format(Ind, "%s%s cast a %s rune of trap detection.", begin, description, r_imperatives[imper].name);
			
				if (success)
				{
					detect_trap(Ind, 36);
				}
				break;
			
			case RT_FORCE:
			case RT_STASIS_DISARM:
				msg_format(Ind, "%s%s cast a %s rune of trap destruction.", begin, description, r_imperatives[imper].name);
			
				if (success)
				{
					destroy_doors_touch(Ind, radius);
				}
				break;
			
			case RT_FLY:
				msg_format(Ind, "%s%s summon %s ethereal wings. (%i turns)", begin, description, r_imperatives[imper].name, duration);
				
				if (success)
				{
					set_tim_fly(Ind, duration);
				}
				break;
			*/
			case RT_ANNIHILATION:
				//Ranges from 2 to 22 bonus points
				speed = speed * 15 / 10;
				
				msg_format(Ind, "%s%s summon a %s otherworldly bloodlust. (%i turns)", begin, description, r_imperatives[imper].name, duration);
				
				if (success)
				{
					set_tim_trauma(Ind, duration, speed);
				}
				break;
		  /*
			case RT_THUNDER:
				if (success)
				{
					set_tim_thunder(Ind, 10+randint(10)+rget_level(25), 5+rget_level(10), 10+rget_level(25));
				}
				break;
			*/
			case RT_TIME:
				msg_format(Ind, "%s%s cast a %s rune of quickening. (%i turns)", begin, description, r_imperatives[imper].name, duration);
				
				if (success)
				{
					set_fast(Ind, duration, speed_max); //Ranges from 6 to 15
				}
				break;
			/*
			case RT_SATIATION:
				msg_format(Ind, "%s%s cast a %s rune of satiation.", begin, description, r_imperatives[imper].name);
				
				if (success)
				{
					//if (!p_ptr->suscep_life)
					if (p_ptr->prace != RACE_VAMPIRE)
						set_food(Ind, PY_FOOD_MAX - 1);
				}
				break;
			
			case RT_RESISTANCE:
				spell_duration = duration / 10;
				spell_duration = spell_duration > 35 ? 35 : spell_duration;
				spell_duration = spell_duration < 5 ? 5 : spell_duration;
				msg_format(Ind, "%s%s summon %s elemental protection. (%i turns)", begin, description, r_imperatives[imper].name, spell_duration);
				
				if (success)
				{
					set_oppose_acid(Ind, spell_duration);
					set_oppose_elec(Ind, spell_duration);
					set_oppose_fire(Ind, spell_duration);
					set_oppose_cold(Ind, spell_duration);
					set_oppose_pois(Ind, spell_duration);
				}
				break;
			*/
			case RT_FIRE:
				msg_format(Ind, "%s%s cast a %s rune of fire resistance. (%i turns)", begin, description, r_imperatives[imper].name, duration);
				//XOR immunity @ r-skill of 50? - Kurzel
				if (success)
				{
					set_oppose_fire(Ind, duration);
				}
				break;
			
			case RT_COLD:
				msg_format(Ind, "%s%s cast a %s rune of cold resistance. (%i turns)", begin, description, r_imperatives[imper].name, duration);
				
				if (success)
				{
					set_oppose_cold(Ind, duration);
				}
				break;
			
			case RT_ACID:
				msg_format(Ind, "%s%s cast a %s rune of acid resistance. (%i turns)", begin, description, r_imperatives[imper].name, duration);
				
				if (success)
				{
					set_oppose_acid(Ind, duration);
				}
				break;
			
			case RT_ELEC:
				msg_format(Ind, "%s%s cast a %s rune of electrical resistance. (%i turns)", begin, description, r_imperatives[imper].name, duration);
				
				if (success)
				{
					set_oppose_elec(Ind, duration);
				}
				break;
			
			case RT_POISON:
				msg_format(Ind, "%s%s cast a %s rune of poison resistance.", begin, description, r_imperatives[imper].name);
				
				if (success)
				{
					set_oppose_pois(Ind, duration);
				}
				break;
			/*
			case RT_ICEPOISON:
      */
			case RT_DISENCHANT:
				msg_format(Ind, "%s%s banish the magical energies.", begin, description);
				//re-check this for new planned temporary buffs (unmagic too?) - Kurzel
				if (success)
				{
					set_fast(Ind, 0, 0);
					set_oppose_acid(Ind, 0);
					set_oppose_elec(Ind, 0);
					set_oppose_fire(Ind, 0);
					set_oppose_cold(Ind, 0);
					set_oppose_pois(Ind, 0);
					set_tim_fly(Ind, 0);
					set_tim_ffall(Ind, 0);
					set_tim_esp(Ind, 0);
					set_blessed(Ind,0);
					set_st_anchor(Ind, 0);
					(void)set_shield(Ind, 0, 0, SHIELD_NONE, 0, 0);
					
					if (p_ptr->rune_stealth)
					{
						p_ptr->rune_stealth = 0;
						p_ptr->rune_num_of_buffs--;
						
						p_ptr->redraw |= PR_SKILLS;
					}
					
					/* Remove glyph */
					if (zcave[p_ptr->py][p_ptr->px].feat == FEAT_GLYPH)
						cave_set_feat_live(&p_ptr->wpos, p_ptr->py, p_ptr->px, FEAT_FLOOR);
					
					/*Clear teleport memory location */
					p_ptr->memory.wpos.wx = 0;
					p_ptr->memory.wpos.wy = 0;
					p_ptr->memory.wpos.wz = 0;
					p_ptr->memory.x = 0;
					p_ptr->memory.y = 0;
					set_tim_deflect(Ind,0);
				}
				break;
			/*
			case RT_QUICKEN:
				msg_format(Ind, "%s%s cast a %s rune of quickening. (%i turns)", begin, description, r_imperatives[imper].name, duration);
				
				if (success)
				{
					speed += 2;
					
					set_fast(Ind, duration, speed);
				}
				break;
			
			case RT_DISINTEGRATE:
      
			case RT_TELEPORT:
				msg_format(Ind, "%s%s teleport.", begin, description);
				
				if (success)
				{
					if (inarea(&p_ptr->memory.wpos, &p_ptr->wpos))
					{
						teleport_player_to(Ind, p_ptr->memory.y, p_ptr->memory.x); //use swap position for accuracy? - Kurzel
						p_ptr->memory.wpos.wx = 0;
						p_ptr->memory.wpos.wy = 0;
						p_ptr->memory.wpos.wz = 0;
						p_ptr->memory.x = 0;
						p_ptr->memory.y = 0;
					}
					else 
					{
						teleport_player(Ind, ((100 + teleport_level * 100) / 5), FALSE);
					}
				}
				break;
			
			case RT_CHAOS:
			case RT_INERTIA:
			case RT_TELEPORT_TO:
				msg_format(Ind, "%s%s teleport forward.", begin, description);
				
				if (success)
				{
					if (p_ptr->target_col == 0 && p_ptr->target_row == 0)
						teleport_player(Ind, ((100 + teleport_level * 100) / 5), FALSE);
					else
						teleport_player_to(Ind, p_ptr->target_row, p_ptr->target_col);
				}
				break;
			
			
			#if 0
			case RT_TELEPORT_LEVEL:
				msg_format(Ind, "%s%s teleport out.", begin, description);
				
				if (success)
				{
					teleport_player_level(Ind);
					
					if (randint(100)<=1) //A small amount of unreliability
						teleport_player_level(Ind);
					
					if (modifier != 130)
					{
						msg_format(Ind, "You are hit by debris for \377u%i \377wdamage", (p_ptr->mhp*(0-(modifier-115))/100));
						take_hit(Ind, (p_ptr->mhp*(0-(modifier-115))/100), "level teleportation", 0); //Hit for up to 60% of health, depending on inverse sucessfullness
					}
					
				}
				break;
			#endif
				
			
			case RT_TELEPORT_LEVEL:
			case RT_GRAVITY:
			case RT_SUMMON:
				msg_format(Ind, "%s%s summon monsters.", begin, description);
				
				if (success)
				{
					project_hack(Ind, GF_TELE_TO, 0, " summons"); 
				}
				break;
			
			case RT_ICE:
			case RT_MEMORY:
				msg_format(Ind, "%s%s memorise your position.", begin, description);
				
				if (success)
				{
					wpcopy(&p_ptr->memory.wpos, &p_ptr->wpos);
					p_ptr->memory.x = p_ptr->px;
					p_ptr->memory.y = p_ptr->py;
				}
				break;
				
			case RT_RECALL:
				msg_format(Ind, "%s%s recall yourself.", begin, description);
				
				if (success)
				{
					set_recall_timer(Ind, damroll(1,100)); 
				}
				break;

			case RT_EARTHQUAKE:
				msg_format(Ind, "%s%s summon an earthquake.", begin, description);
				
				if (success)
				{
					if (level>10)
					{
						destroy_area(&p_ptr->wpos, p_ptr->py, p_ptr->px, 15, TRUE, FEAT_FLOOR, 120);
					}
					else
					{
						earthquake(&p_ptr->wpos, p_ptr->py, p_ptr->px, 15);
					}
				}
				break;
			
			case RT_WATERPOISON:
			case RT_DETECTION_BLIND:
				msg_format(Ind, "%s%s cast a rune of detection.", begin, description);
				
				if (success) 
				{
					if (level>40)
					{
						detection(Ind, DEFAULT_RADIUS * 2);
					}
					else
					{
						detect_creatures(Ind);
					}
				}
				break;
			
			case RT_DETECT_STAIR:
				msg_format(Ind, "%s%s cast a rune of exit detection.", begin, description);
				
				if (success)
				{
					detect_sdoor(Ind, 36);
				}
				break;
			
			case RT_SEE_INVISIBLE:
				msg_format(Ind, "%s%s cast a %s rune of see invisible.", begin, description, r_imperatives[imper].name);
				
				if (success)
				{
					set_tim_invis(Ind, duration);
					
					if (level > 15)
					{
						detect_treasure(Ind, DEFAULT_RADIUS*2);
					}
				}
				break;
			*/
			case RT_NEXUS:
				msg_format(Ind, "%s%s summon nexus.", begin, description);
				
				if (success)
				{
					switch(randint(10))
					{
						case 1: teleport_player(Ind, (10 + (teleport_level * 10) / 50), FALSE); break;
						case 2: set_recall_timer(Ind, damroll(1,100)); break;
						case 3: project_hack(Ind, GF_TELE_TO, 0, " summons"); break;
						#if 0
						case 4: teleport_player_level(Ind); break;
						#endif
						case 5: teleport_player_to(Ind, p_ptr->target_col, p_ptr->target_row); break;
						default: teleport_player(Ind, (100 + (teleport_level * 100) / 5), FALSE); break;
					}
				}
				break;
			/*
			case RT_CLONE:
			case RT_WIND_BLINK:
      */
      case RT_WATER:
				msg_format(Ind, "%s%s blink.", begin, description);
				
				if (success)
				{
					teleport_player(Ind, (10 + (teleport_level * 10) / 50), FALSE);
				}
				break;
			
			case RT_LIGHT:
				msg_format(Ind, "%s%s summon light.", begin, description);
			
				if (success)
				{
					lite_area(Ind, 10, radius + 1);
					lite_room(Ind, &p_ptr->wpos, p_ptr->py, p_ptr->px);
				}
				
				break;
			/*
			case RT_PLASMA:
			case RT_BRILLIANCE:
				msg_format(Ind, "%s%s summon bright light.", begin, description);
			
				if (success)
				{
					lite_area(Ind, 10, 2);
					lite_room(Ind, &p_ptr->wpos, p_ptr->py, p_ptr->px);
				}
				
				break;
			
			case RT_DARK_SLOW:
      */
			case RT_SHADOW:
				msg_format(Ind, "%s%s summon shadows.", begin, description);
			
				if (success)
				{
					unlite_area(Ind, 10, radius + 1);
					
					unlite_room(Ind, &p_ptr->wpos, p_ptr->py, p_ptr->px);
				}
				
				break;
			/*
			case RT_HELL_FIRE:				
			case RT_OBSCURITY:
				msg_format(Ind, "%s%s summon obscurity.", begin, description);
			
				if (success)
				{
					unlite_area(Ind, 10, 2);
					unlite_room(Ind, &p_ptr->wpos, p_ptr->py, p_ptr->px);
					set_invis(Ind, duration, damage);
				}

				break;
       */
				
			case RT_UNBREATH:
				msg_format(Ind, "%s%s cast a %s rune of stealth.", begin, description, r_imperatives[imper].name);
				//This should be changed to a timed spell...
				/* Unset */
				if (success)
				{
					unlite_area(Ind, 10, 2);
					unlite_room(Ind, &p_ptr->wpos, p_ptr->py, p_ptr->px);
					
					if (p_ptr->rune_stealth)
					{
						printf("Set %s's stealth buff to %i\n",p_ptr->name,0);
						p_ptr->rune_stealth = 0;
						p_ptr->rune_num_of_buffs = p_ptr->rune_num_of_buffs - 1;
						msg_print(Ind, "You feel less stealthy.");
					}
					/* Set */
					else
					{
						speed /= 2; //Stealth ranges from 1-11
						
						printf("Set %s's stealth buff to %i\n", p_ptr->name, speed);
						p_ptr->rune_stealth = speed;
						p_ptr->rune_num_of_buffs = p_ptr->rune_num_of_buffs + 1;
						
						msg_print(Ind, "You feel more stealthy.");
					}
					
					p_ptr->redraw |= PR_SKILLS;
					handle_stuff(Ind);
				}
				break;
			/*
			case RT_MANA:
			case RT_ROCKET:
			case RT_MISSILE:
				spell_duration = duration / 10;
				spell_duration = (spell_duration < 5 ? 5 : spell_duration);
				spell_duration = (spell_duration > 20 && (type == RT_MISSILE || type == RT_MANA) ? 20 : spell_duration);
				spell_duration = (spell_duration > 60 ? 60 : spell_duration);
				
				msg_format(Ind, "%s%s cast a %s rune of deflection. (%i turns)", begin, description, r_imperatives[imper].name, spell_duration);
				
				if (success)
				{
					set_tim_deflect(Ind, spell_duration);
				}
				break;
			
			case RT_NUKE:
			case RT_ANCHOR:
				msg_format(Ind, "%s%s constrict the space-time continuum. (%i turns)", begin, description, duration);
				
				if (success)
				{
					set_st_anchor(Ind, duration);
				}
				break;
			*/
			/* RT_FURY and RT_BERSERK are out of theme and therefore disabled */
      //re-enable berserk though? - Kurzel
      /*
			case RT_FURY:
				msg_format(Ind, "%s%s summon fury. (%i turns)", begin, description, duration);
				
				if (success)
				{
					//set_fury(Ind, duration);
				}
				break;
			
			case RT_BESERK:
				msg_format(Ind, "%s%s cast a rune of beserking. (%i turns)", begin, description, duration);
				
				if (success)
				{
					if (duration > 40)
					{
						duration = 40;
					}
					
					//set_adrenaline(Ind, duration);
				}
				break;

			case RT_POLYMORPH:
			case RT_HEALING:
      */
			 case RT_DIG:
				spell_damage = damage;
				if (p_ptr->csp < cost)
					spell_damage = spell_damage - (spell_damage * ((cost - p_ptr->csp) * 20) / 100);
				
				if (spell_damage <= 0)
				{
					msg_format(Ind, "You fail to summon healing.");
					break;
				}
				
				msg_format(Ind, "%s%s summon %s healing.", begin, description, r_imperatives[imper].name);
				
				if (success)
				{
					hp_player(Ind, spell_damage);
					fire_ball(Ind, GF_HEAL_PLAYER, 0, spell_damage, radius, p_ptr->attacker);
				}
				break;
			/*
			case RT_NETHER:
			case RT_AURA:
				msg_format(Ind, "%s%s summon a %s fiery aura. (%i turns)", begin, description, r_imperatives[imper].name, duration);
				
				if (success)
				{
					shield = shield * 6 / 10; //60% of the +AC given by the equivalent Mystic Shield
					set_shield(Ind, duration, shield, SHIELD_FIRE, SHIELD_FIRE, 0);
				}
				break;
			
			default:
				break;
        */
		}
	}
	else
	{
		/* Hack -- Encode the exploding typ_explode, and imperative - Kurzel */
		if (gf_explode != 0) {
			gf_type = gf_type + 1000*gf_explode;
			gf_type = gf_type + 1000000*(r_imperatives[imper].radius+3); //hard-coded 3 (max-radius mod?)
			//Note that 'augments' don't effect explosions! (eg. radius)
		}
		
		if (type_flags & R_STOR)
		{
			sprintf(p_ptr->attacker, " fills the air with %s for", runespell_list[type].title);
			msg_format(Ind, "%s%s fill the air with %s %s.", begin, description, r_imperatives[imper].name, runespell_list[type].title);
			if (success) { //project_hack(Ind, gf_type, damage, p_ptr->attacker);
			
			/* Hack -- Encode the typ_effect for explosion */
			if (gf_explode != 0) gf_type = gf_type + 10000000*EFF_STORM;
			
			fire_wave(Ind, gf_type, 0, damage, radius, duration, 10, EFF_STORM, p_ptr->attacker);
			}
		}
		else if (type_flags & R_WAVE)
		{
			sprintf(p_ptr->attacker, " summons a wave of %s for", runespell_list[type].title);
			msg_format(Ind, "%s%s summon a %s wave of %s.", begin, description, r_imperatives[imper].name, runespell_list[type].title);
			if (success) { //fire_wave(Ind, gf_type, 0, damage, radius, duration, 10, EFF_LAST, p_ptr->attacker);
			
			/* Hack -- Encode the typ_effect for explosion */
			if (gf_explode != 0) gf_type = gf_type + 10000000*EFF_WAVE;
			
			fire_wave(Ind, gf_type, 0, damage, 0, duration, 2, EFF_WAVE, p_ptr->attacker);
			}
		}
		if (type_flags & R_MELE)
		{
			int px = p_ptr->px;
			int py = p_ptr->py;
			int tx = p_ptr->target_col;
			int ty = p_ptr->target_row;
			
			int dx = px - tx;
			int dy = py - ty;
			
			if (dx > 1 || dx < -1 || dy > 1 || dy < -1)
			{
				p_ptr->target_col = px;
				p_ptr->target_row = py;
			}
			
			if ((dx > 1 || dx < -1 || dy > 1 || dy < -1) && tx != 0 && ty != 0)
			{
				sprintf(p_ptr->attacker, " is summons %s for", runespell_list[type].title);
				msg_format(Ind, "%s%s summon a %s burst of undirected %s.", begin, description, r_imperatives[imper].name, runespell_list[type].title);
				
				if (success)
				{
					fire_wave(Ind, gf_type, 0, (damage - damage/4), 1, 1, 1, EFF_STORM, p_ptr->attacker);
				}
			}
			else
			{
				sprintf(p_ptr->attacker, " is summons %s for", runespell_list[type].title);
				
				msg_format(Ind, "%s%s summon a %s burst of %s.", begin, description, r_imperatives[imper].name, runespell_list[type].title);
				if (success)
				{
					fire_bolt(Ind, gf_type, dir, damage, p_ptr->attacker);
				}
			}
		}
		else if ((type_flags & R_BALL))
		{
			sprintf(p_ptr->attacker, " summons a ball of %s for", runespell_list[type].title);
			msg_format(Ind, "%s%s summon a %s ball of %s.", begin, description, r_imperatives[imper].name, runespell_list[type].title);
			
			if (success)
			{
				fire_ball(Ind, gf_type, dir, damage, radius, p_ptr->attacker);
			}
		}
		else if (type_flags & R_CLOU)
		{
			sprintf(p_ptr->attacker, " summons a cloud of %s for", runespell_list[type].title);
			msg_format(Ind, "%s%s summon a %s cloud of %s.", begin, description, r_imperatives[imper].name, runespell_list[type].title);
			
			if (success)
			{		
				/* Hack -- Encode the typ_effect for explosion */
				if (gf_explode != 0) gf_type = gf_type + 10000000*EFF_LAST; //Minor Hack -- This is 'unused' / not related to cloud type.
			
				fire_cloud(Ind, gf_type, dir, damage, radius, duration, 9, p_ptr->attacker);
			}
		}
		else if (type_flags & R_BOLT || type_flags & R_BEAM)
		{
			if (type==RT_DIG) //Regular bolt and beam will stop before the wall
			{
				int flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
				
				if (success)
				{
					project_hook(Ind, GF_KILL_WALL, dir, 20 + randint(30), flg, "");
				}
				
				msg_format(Ind, "%s%s cast a rune of wall destruction.", begin, description);
			}
			else
			{
				if (type_flags & R_BOLT)
				{
					sprintf(p_ptr->attacker, " summons a bolt of %s for", runespell_list[type].title);
					msg_format(Ind, "%s%s summon a %s bolt of %s.", begin, description, r_imperatives[imper].name, runespell_list[type].title);
					
					if (success)
					{
						fire_bolt(Ind, gf_type, dir, damage, p_ptr->attacker);
					}
				}
				else
				{
					sprintf(p_ptr->attacker, " summons a beam of %s for", runespell_list[type].title);
					msg_format(Ind, "%s%s summon a %s beam of %s.", begin, description, r_imperatives[imper].name, runespell_list[type].title);
					
					if (success)
					{
						fire_beam(Ind, gf_type, dir, damage, p_ptr->attacker);
					}
				}
			}
		}
	}
	
	if (p_ptr->csp > cost)
	{
		p_ptr->csp -= cost;
	}
	else
	{
		/* The damage is implied by the "Exhausted, ..." message, so not explicitly stated. */
		take_hit(Ind, (cost-p_ptr->csp), "magical exhaustion", 0);
		p_ptr->csp = 0;
	}
	
	p_ptr->redraw |= PR_MANA;
	
	if (margin < 0)
	{
		difficulty = (difficulty+mali>0 ? difficulty+mali : 0);
		difficulty = (difficulty-margin>0 ? difficulty-margin : 0);
		rspell_do_penalty(Ind, rspell_penalty(Ind, difficulty), damage, duration, cost, gf_type, "",  imper, type_flags);
	}
	
	p_ptr->energy -= rspell_time(Ind, imper);
	
	if (p_ptr->shooty_till_kill)
	{
		p_ptr->shooting_till_kill = TRUE;
		p_ptr->shooty_till_kill = FALSE;
	}
	
	return 0;
}

bool is_attack(u32b s_flags)
{
	return (!(s_flags & R_MELE) && !(s_flags & R_SELF) && !(s_flags & R_WAVE) && !(s_flags & R_STOR));
}

byte execute_rspell (u32b Ind, byte dir, u32b s_flags, byte imperative)
{
	player_type * p_ptr = Players[Ind];
	
	u32b s_type = rspell_type(s_flags);
	s16b s_cost = 0; u16b s_dam = 0;  s16b s_diff = 0; s16b s_time = 0;
	u16b s_av = 0; u16b augment_level = 0;
	
	u16b radius = 0; u16b duration = 0;
	s16b mali = 0;
	s_av = rspell_skill(Ind, s_flags);
	
	s_cost = rspell_cost(Ind, s_type, s_flags, s_av, imperative);
	s_dam = rspell_dam(Ind, &radius, &duration, s_type, s_flags, s_av, imperative);
	s_diff = rspell_diff(Ind, imperative, s_cost, s_av, s_type, s_flags, &mali);
	
	s_time = rspell_time(Ind, imperative);
	
	if (!rspell_check(Ind, &mali, s_flags))
	{
		mali += 5; //+15% fail for first missing rune, +10% per additional rune
	}
	
	/* Augment the runespell if augment level requirement is met - Kurzel */
	u32b s_augment = runespell_list[s_type].self;
	int i;
	if (s_augment != 0) {
		augment_level = rspell_skill(Ind, s_augment);
		for (i=0;i<RCRAFT_MAX_ELEMENTS;i++) {
			if (s_augment == r_augments[i].rune) {
				//Check the level requirement for a single rune.
				if(augment_level >= r_augments[i].level) {
					s_cost = s_cost * (((r_augments[i].cost)-10 > 0) ? (r_augments[i].cost-10)*(100-augment_level)/50+10 : (r_augments[i].cost-10)*augment_level/50+10) / 10;
					mali = mali + (((r_augments[i].fail) > 0) ? r_augments[i].fail*(100-augment_level)/50 : r_augments[i].fail*augment_level/50);
					s_dam = s_dam * (((r_augments[i].dam)-10 < 0) ? (r_augments[i].dam-10)*(100-augment_level)/50+10 : (r_augments[i].dam-10)*augment_level/50+10) / 10;
					s_time = s_time * (((r_augments[i].time)-10 > 0) ? (r_augments[i].time-10)*(100-augment_level)/50+10 : (r_augments[i].time-10)*augment_level/50+10) / 10;
					radius = radius + (((r_augments[i].radius) > 0) ? r_augments[i].radius*(100-augment_level)/50 : r_augments[i].radius*augment_level/50);
					duration = duration * (((r_augments[i].duration)-10 < 0) ? (r_augments[i].duration-10)*(100-augment_level)/50+10 : (r_augments[i].duration-10)*augment_level/50+10) / 10;
				}
				break;
			}
		}
	}

	/* Time to cast can be varied by the active spell modifier. */
	if (p_ptr->energy < s_time)
	{
		return 0;
	}

	if (s_av<=0)
	{
		msg_print(Ind, "\377rYou don't know these runes.");
		p_ptr->energy -= s_time;
		return 0;
	}
	
	/* Forbid casting 15 levels above skill level - Kurzel */
	s16b type_level = 0;
	int j = 0;
	for (j=0;j<RCRAFT_MAX_TYPES;j++) {
		if((s_flags & runespell_types[j].type) == runespell_types[j].type) {
			type_level = runespell_types[j].cost;
			break;
		}
	}
	if (s_av<=runespell_list[s_type].level+r_imperatives[imperative].level+type_level-15)
	{
		msg_print(Ind, "\377rYou don't dare invoke such a dangerous spell.");
		p_ptr->energy -= s_time;
		return 0;
	}
	
	if (s_type == RT_NONE)
	{
		msg_print(Ind, "\377rThis is not a runespell.");
		p_ptr->energy -= s_time;
		return 0;
	}

#ifdef LIMIT_NON_RUNEMASTERS
	{
		int runes = 0, bit;
		for (bit = 8; bit < 32; bit++)
			if (s_flags & (1 << bit)) runes++;
		if (runes > 2 && p_ptr->pclass != CLASS_RUNEMASTER) {
			msg_print(Ind, "\377rYou are not adept enough to draw more than two runes.");
			p_ptr->energy -= s_time;
			return 0;
		}
	}
#endif

	/* Probably paranoia, since rune-able classes should have none of these (except ftk) */
	break_cloaking(Ind, 5);
	break_shadow_running(Ind);
	stop_precision(Ind);
//	stop_shooting_till_kill(Ind);

	/* (S)he is no longer afk */
	un_afk_idle(Ind);

	/* AM checks for runes assume that not the person is emitting the magic, but the runes are! */
	if (check_antimagic(Ind, 100))
	{
		p_ptr->energy -= s_time;
		return 0;
	}
	if (p_ptr->anti_magic && (s_flags & R_SELF)) {
		msg_format(Ind, "\377%cYour anti-magic shell absorbs the spell.", COLOUR_AM_OWN);
		p_ptr->energy -= s_time;
		return 0;
	}

	/* school spell casting interference chance used for this */
	if (interfere(Ind, cfg.spell_interfere)) {
		p_ptr->energy -= s_time;
		return 0;
	}

	if (p_ptr->shoot_till_kill)
	{
		if (is_attack(s_flags))
		{
			/* Set future FTK attack type, if we're trying something new */
			if (p_ptr->shoot_till_kill_rune_spell != s_flags || p_ptr->shoot_till_kill_rune_modifier != imperative)
			{
				p_ptr->shoot_till_kill_rune_spell = s_flags;
				p_ptr->shoot_till_kill_rune_modifier = imperative;
			}
			
			if (p_ptr->shooting_till_kill)
			{
				p_ptr->shooting_till_kill = FALSE;
				
				/* Cancel if we've lost our target */
				if (dir != 5 || !target_okay(Ind))
					return 0;
				
				/* Cancel if we're going to automatically wake new monsters */
#ifndef PY_PROJ_ON_WALL
				if (!projectable_real(Ind, p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col, MAX_RANGE))
#else
				if (!projectable_wall_real(Ind, p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col, MAX_RANGE))
#endif
					return 0;
				
				/* Cancel if we're going to exhaust ourselves */
				if (s_cost > p_ptr->csp)
					return 0;
				
				/* Cancel if we're being stupid */
				if (s_diff > 60)
					return 0;

				/* Continue casting after a successful cast */
				p_ptr->shooty_till_kill = TRUE;
			}
		}
	}
	
	cast_runespell(Ind, dir, s_dam, radius, duration, s_cost, s_type, s_diff, imperative, s_flags, s_av, mali);
	
	if (r_imperatives[imperative].id == 7) //Chaotic type
	{
		if (randint(100)==50)
		{
			msg_print(Ind, "\377sYour invocation is particularly flashy.");
			//project_hack(Ind, GF_TELE_TO, 0, " summons"); /*Use wonder instead? - Kurzel */
		}
	}
	
	return 1;
}
#endif
