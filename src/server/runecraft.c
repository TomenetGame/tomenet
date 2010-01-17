/* Purpose: New runemaster class functionality */

/* by Relsiet/Andy Dubbeld (andy@foisdin.com) */

/*
Proposal for new runemastery class/spell system

Spells are cast via a parsed incantation comprised of any combination of the runes that the player knows.

It uses an m-key interface.

All combinations of runes should do something.

There are three parts to a runic incantation:
	1. The effect, which is up to five different runes which combine to form an effect type
	2. The imperative, which is one of eight magic words, which govern the tone of the spell, and influence power, cost, fail-rates and dangerousness of a spell.
	3. The method is the spell-type (self,bolt,beam,ball,wave,cloud or LOS)
	
The spell characteristics (damage, radius and duration, fail-rate, cost, etc) of a spell are decided by the average of the caster's skill level in each rune.

Penalties are applied for casting spells when unable to look at the runes. (Missing a rune, blind, confused, stunned, etc.)

If a spell fails by a little bit, the spell might still be cast. Either way, in the case of failure the caster might be: (depending on severity)
	hit by some of the spell-element,
	blinded, confused, slowed, cut or poisoned,
	charged extra mp,
	driven partly insane,
	drained of stats,
	given the black breath,
	killed.
	
So the versatility of this runemaster is balanced by its riskiness.

The spells are similar to Istari spells, but focused on flashiness rather than practicality. There's lots of spells, but not all of them are equally useful, and they're all dangerous.

His primary stats are Int and Dex.

He should have a skill for each pair of opposing runes. The skill should grant basic resistance to the elements it governs by lv 50 (but never immunity).

A high level rune-master (lv 50) should only have a few runes skilled > 40.

Skill tree:

- Magic						1.000 [1.000]
	- Rune Mastery
		. Heat & Cold			1.000 [0.900]
		. Water & Acid	 		0.000 [0.900]
		. Lightning	& Earth		0.000 [0.800]
		. Wind & Poison			1.000 [0.800]
		. Mana & Chaos			0.000 [0.700]
		. Force	& Gravity		0.000 [0.700]
		. Nether & Time			0.000 [0.600]
		. Mind & Nexus			0.000 [0.600]

*/

#include "angband.h"

#ifdef ENABLE_RCRAFT

byte execute_rspell (u32b Ind, byte dir, u32b spell, byte imperative);
u16b cast_runespell(u32b Ind, byte dir, u16b damage, u16b radius, u16b duration, s16b cost, u32b type, s16b diff, byte imper, u32b type_flags, u16b s_av, s16b mali);
u32b rspell_type (u32b flags);
u32b rspell_flag_type (r_element *list, u16b listc);
u32b rspell_flag_method (r_element *list, u16b listc);
s16b rspell_diff(u32b Ind, byte imperative, s16b s_cost, u16b s_av, u32b s_type, u32b s_flags, s16b * mali);
u16b rspell_skill(u32b Ind, u32b s_flags);
u16b rspell_do_penalty(u32b Ind, byte type, u16b damage, u16b duration, s16b cost, u32b s_type, char * attacker, byte imperative);
byte rspell_penalty(u32b Ind, u16b pow);
u16b rspell_dam (u32b Ind, u16b *radius, u16b *duration, u16b s_type, u32b s_flags, u16b s_av, byte imperative);
s16b rspell_cost (u32b Ind, u16b s_type, u32b s_flags, u16b s_av, byte imperative);
byte rspell_check(u32b Ind, s16b * mali, u32b s_flags);
byte meth_to_id(u32b s_meth);
byte runes_in_flag(byte runes[], u32b flags);
byte rune_value(byte runes[]);

byte runes_in_flag(byte runes[], u32b flags)
{
	if((flags & R_FIRE)==R_FIRE) { runes[0]=1; }  else { runes[0] = 0; }
	if((flags & R_COLD)==R_COLD) { runes[1]=1; }  else { runes[1] = 0; }
	if((flags & R_ACID)==R_ACID) { runes[2]=1; }  else { runes[2] = 0; }
	if((flags & R_WATE)==R_WATE) { runes[3]=1; }  else { runes[3] = 0; }
	if((flags & R_ELEC)==R_ELEC) { runes[4]=1; }  else { runes[4] = 0; }
	if((flags & R_EART)==R_EART) { runes[5]=1; }  else { runes[5] = 0; }
	if((flags & R_POIS)==R_POIS) { runes[6]=1; }  else { runes[6] = 0; }
	if((flags & R_WIND)==R_WIND) { runes[7]=1; }  else { runes[7] = 0; }
	if((flags & R_MANA)==R_MANA) { runes[8]=1; }  else { runes[8] = 0; }
	if((flags & R_CHAO)==R_CHAO) { runes[9]=1; }  else { runes[9] = 0; }
	if((flags & R_FORC)==R_FORC) { runes[10]=1; } else { runes[10] = 0; }
	if((flags & R_GRAV)==R_GRAV) { runes[11]=1; } else { runes[11] = 0; }
	if((flags & R_NETH)==R_NETH) { runes[12]=1; } else { runes[12] = 0; }
	if((flags & R_TIME)==R_TIME) { runes[13]=1; } else { runes[13] = 0; }
	if((flags & R_MIND)==R_MIND) { runes[14]=1; } else { runes[14] = 0; }
	if((flags & R_NEXU)==R_NEXU) { runes[15]=1; } else { runes[15] = 0; }
	
	return 0;
}

byte rune_value(byte runes[])
{
	byte i,count;
	count=0;
	for(i=0;i<16;i++)
		if(runes[i])
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
	s16b p_runes[16];
	s16b p_rc = 0;
	byte runes[16];
	
	for (j = 0; j < INVEN_TOTAL; j++)
	{
		if (j >= INVEN_PACK)
			continue;
		
		o_ptr = &p_ptr->inventory[j];

		if(o_ptr->tval == TV_RUNE2)
		{
			p_runes[p_rc] = o_ptr->sval;
			p_rc++;
		}
	}
	
	runes_in_flag(runes,s_flags);
	/* Type check */
	for(i=0;i<16;i++)
	{
		if(runes[i]==1)
		{
			k=0;
			for (j = 0; j < p_rc; j++)
				if(p_runes[j] == r_elements[i].id)
					k = 1;
			if(!k)
				*mali += 10;
		}
	}

	if(!*mali)
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
	if(s_meth & R_MELE) m = 0;
	else if(s_meth & R_SELF) m = 1;
	else if(s_meth & R_BOLT) m = 2;
	else if(s_meth & R_BEAM) m = 3;

	else if(s_meth & R_BALL) m = 4;
	else if(s_meth & R_WAVE) m = 5;
	else if(s_meth & R_CLOU) m = 6;
	else if(s_meth & R_LOS) m = 7;
		
	return m;
}


/*
	rspell_cost

	Determines mp cost of a given spell
	
	Cost should be determined by a few criteria:
	
	1. Number of runes in a spell (complexity)
	2. Level of the spell effect--
	3. Average skill level of player
	4. Cost multipliers (effect, method & imperative)

	However, it's tricky to get all that into a sane cost scale, so this function, and the damage function are still undergoing significant tweaks.

	The first three should be approximately the average cost of an istari spell. The fourth should adjust for necessary variations from that.
*/
s16b rspell_cost (u32b Ind, u16b s_type, u32b s_flags, u16b s_av, byte imperative)
{
	player_type *p_ptr = Players[Ind];
	byte m = meth_to_id(s_flags);
	byte runes[16];
	byte value = 0;	
	byte penalty = 0;
	s16b cost = 0;
	
	runes_in_flag(runes,s_flags);
	value = rune_value(runes);
	
	byte t_pen = 0; byte s_pen = 0; byte d_pen = 0;
	
	if(!s_av) s_av = 1;
	
	cost = rget_level(30+value);
	
	t_pen = runespell_types[m].pen;
	s_pen = runespell_list[s_type].pen;
	d_pen = r_imperatives[imperative].cost ? r_imperatives[imperative].cost : randint(25)+1;

	if(cost > S_COST_MAX)
		cost = S_COST_MAX;
	
	cost = (cost*t_pen)/10;
	cost = (cost*s_pen)/10;
	cost = (cost*d_pen)/10;
	
	/* Alternative to these just increasing fail rates. Now the fail rates are increased less, and spell costs are increased by a percentage. */
	if (no_lite(Ind) || p_ptr->blind)
	{
		penalty+=20;
	}
	if (p_ptr->confused)
	{
		penalty+=20;
	}
	if (p_ptr->stun > 50)
	{
		penalty+= 20;
	}
	else if (p_ptr->stun)
	{
		penalty+= 10;
	}
	
	cost = cost+((cost*penalty)/10); //Costs can increase up to 50% if everything is going wrong.
	
	if(cost < S_COST_MIN)
		cost = S_COST_MIN;
	
	return cost;
}

u16b rspell_dam (u32b Ind, u16b *radius, u16b *duration, u16b s_type, u32b s_flags, u16b s_av, byte imperative)
/*
	Determines the damage, radius and duration of a given spell

	Should follow a similar curve to mage-spells.
	
	Melee:
		(Is supposed to be a range 1 bolt, but a wave is simpler for the moment)
		fire_wave(Ind, type, 0, 80 + get_level(Ind, ICESTORM, 200),
					1, 1, 1, EFF_STORM, " invokes an ice storm for")
	Bolt:
		Manathrust: (standard for bolt spells)
		damage = damroll(3 + s_av, 1 + (s_av*20)/50)

	Self:
		EL Shield: (self spell duration?)
		duration = randint(10) + 15 + (s_av - e_level)

		Mana Shield:
		duration = randint(10) + 20 + ((s_av - e_level)*75)/50

		Averaged:
		duration = randint(10) + randint(30) + ((s_av - e_level)*75)/50

	Ball:
		Fireflash:
		damage = 20 + ((s_av - e_level)*500)/50;
		radius = 2 + ((s_av - e_level)*5)/50;
*/
{
	u16b e_level = runespell_list[s_type].level;
	byte runes[16];
	byte value = 0;
	u16b damage = 1;
	byte m = meth_to_id(s_flags);
	e_level += runespell_types[m].cost;
	e_level += (r_imperatives[imperative].danger/10);
	*radius = 1;
	*duration = 1;
	if(s_av<e_level+1)
		s_av = e_level+1; //Give a minimum of damage; if it's out of the caster's depth, he'll be penalised for trying, so a reasonable amount of damage is fair.
	
	runes_in_flag(runes,s_flags);
	value = rune_value(runes); //Approximate expense of runes, taking into account skill costs and perceived worth.
	
	if ((s_flags & R_BOLT) == R_BOLT) 
	{
		damage = damroll(3 + rget_level(50), 1 + rget_level(20));
	}
	else if ((s_flags & R_BEAM) == R_BEAM)
	{
		damage = damroll(value + rget_level(45), 1 + rget_level(20));
	}
	else if ((s_flags & R_SELF) == R_SELF)
	{
		damage = randint(value*10) + rget_level(136);
		*duration = randint(value*10) + randint(30) + rget_level(75);
		
		if(r_imperatives[imperative].dam == 0)
			*radius = (*radius * (1+randint(40)))/10;
		else
			*radius = (*radius * (1+r_imperatives[imperative].dam))/10;
	}
	else if ((s_flags & R_BALL) == R_BALL)
	{
		damage = randint(value*20) + rget_level(500);
		*radius = (1+randint(4)) + rget_level(5);
		if(r_imperatives[imperative].dam == 0)
			*radius = (*radius * (1+randint(40)))/10;
		else
			*radius = (*radius * (1+r_imperatives[imperative].dam))/10;
	}
	else if (s_flags & R_WAVE)
	{
		*radius = 2+randint(value*3) + rget_level(6);
		
		if(r_imperatives[imperative].dam == 0)
			*radius = (*radius * (1+randint(40)))/10;
		else
			*radius = (*radius * (1+r_imperatives[imperative].dam))/10;
		
		damage = randint(60+(value*10)) + rget_level(200);
	}
	else if (s_flags & R_CLOU)
	{
		*radius = 1 + randint(4) - value + rget_level(2); //Value works backwards here to balance cloud effects
		*duration = 1 + randint(6) - value + rget_level(5);
		if(r_imperatives[imperative].dam == 0)
		{
			*radius = (*radius * (1+randint(40)))/10;
			*duration = (*duration * (1+randint(40)))/10;
		}
		else
		{
			*radius = (*radius * (1+r_imperatives[imperative].dam))/10;
			*duration = (*duration * (1+r_imperatives[imperative].dam))/10;
		}
		if(*radius > *duration)
		{
			*radius-=1;
			*duration+=1;
		}
		/* Damage should be proportional to radius and duration. */
		damage = randint(value*1.2) + rget_level(75-(*radius+(*duration/2)));
	}
	else if ((s_flags & R_LOS) == R_LOS)
	{
		damage = e_level + value + rget_level(100);
	}
	else //R_MELE
	{
		damage = damroll(3 + rget_level(50), 1 + rget_level(20));
	}
	
	if(damage > S_DAM_MAX)
	{
		damage = S_DAM_MAX;
	}
	if(damage < 1)
	{
		damage = 1;
	}
	if (*radius < 1)
	{
		*radius = 1;
	}
	if (*duration < 5)
	{
		*duration = 5;
	}
	
	if(r_imperatives[imperative].dam == 0)
	{
		damage = (damage * (1+randint(40)))/10;
	}
	else
	{
		if(r_imperatives[imperative].dam != 1)
			damage = (damage * (1+r_imperatives[imperative].dam))/10;
		else
			damage = (damage * (1+r_imperatives[imperative].dam))/5;
	}
	
	damage = (damage*runespell_list[s_type].dam)/10; //So that things like meteor and nuke do more damage than fire and acid
	
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
*/
byte rspell_penalty(u32b Ind, u16b pow)
{
	player_type *p_ptr = Players[Ind];
	u16b x = 0; u16b r = 1; u16b num = 40; u16b i = 0; u16b y = 0;
	byte pen = 0;
	
	while(1)
		if(pow>num)
		{
			r+=1;
			num+=20;
		}
		else
			break;
		
	
	for(i=0;i<r;i++)
	{
		x=randint(pow);
		
		y = randint(50);
		if(y>(10+p_ptr->luck_cur))
		{
			if (x>300)
			{
				pen |= RPEN_MAJ_DT;
				break;
			}
			else if (x>250)
			{
				pen |= RPEN_MAJ_BB;
				break;
			}
			else if (x>200)
			{
				pen |= RPEN_MAJ_BK;
				break;
			}
			else if (x>150)
			{
				pen |= RPEN_MAJ_SN;
				break;
			}
			else if (x>100)
			{
				pen |= RPEN_MIN_ST;
				break;
			}
			else if (x>75)
			{
				pen |= RPEN_MIN_HP;
				break;
			}
			else if (x>50)
			{
				pen |= RPEN_MIN_SP;
				break;
			}
			else if (x>25)
			{
				pen |= RPEN_MIN_RN;
				break;
			}
		}
	}
	return pen;
}


/* rspell_do_penalty
Executes the penalties worked out in rspell_penalty()

Destroys runes, inflicts damage and other negative effects on player Ind.

*/
u16b rspell_do_penalty(u32b Ind, byte type, u16b damage, u16b duration, s16b cost, u32b s_type, char * attacker, byte imperative)
{
	player_type *p_ptr = Players[Ind];
	int mod_luck = p_ptr->luck_cur+10; //Player's current luck as a positive value between 0 and 50
	u16b d = 0;
	
	if(r_imperatives[imperative].danger == 0)
		damage *= (1+randint(40))/10;
	else
		damage *= 1+(r_imperatives[imperative].danger/10);
	
	if (duration<5)
		duration = 5;
	
	if (damage<10)
		damage = 10;
	
	if(type & RPEN_MIN_RN)
	{
		int i,amt;
		amt = 0;
		object_type	*o_ptr;
		char o_name[160];

		for (i = 0; i < INVEN_TOTAL; i++)	/* Modified version of inven_damage from spells1.c */
		{
			if (i >= INVEN_PACK)
				continue;

			/* Get the item in that slot */
			o_ptr = &p_ptr->inventory[i];

			/* Give this item slot a shot at death */
			if (o_ptr)
			{
				
				if(o_ptr->tval == TV_RUNE2)
				{
					if (rand_int(100) < 10)
					{
						/* Select up to 50% of them, minimum of 1 */
						amt = rand_int(o_ptr->number*50/100);
						
						if(amt == 0 && o_ptr->number >= 1)
							amt = 1;
					}
				}
				
				/* Break */
				if (amt)
				{
					/* Get a description */
					object_desc(Ind, o_name, o_ptr, FALSE, 3);

					/* Message */
					msg_format(Ind, "\377o%sour %s (%c) %s destroyed!", ((o_ptr->number > 1) ? ((amt == o_ptr->number) ? "All of y" : (amt > 1 ? "Some of y" : "One of y")) : "Y"), o_name, index_to_label(i), ((amt > 1) ? "were" : "was"));
					
					inven_item_increase(Ind, i, -amt); inven_item_optimize(Ind, i);
					
					i = INVEN_TOTAL;
					break;
				}
			}
		}
		/* If we didn't break any runes, take some HP */
		if(amt == 0)
		{
			type |= RPEN_MIN_HP;
		}
	}
	if(type & RPEN_MIN_SP)
	{
		d = damroll(1,2)+1;
		
		if (d == 2)
		{
			if(d==1)
				set_paralyzed(Ind, cost);
		}
		if(p_ptr->csp>(cost*2))
		{
			p_ptr->csp -= (cost*2);
		}
		else
		{
			p_ptr->csp = 0;
			type |= RPEN_MIN_HP;
		}
	}
		
	if(type & RPEN_MIN_HP)
	{
		d = damroll(1,7);
		switch(d)
		{
			case 0:
			case 1:
				set_cut(Ind, damage, Ind);
				break;
			case 2:
				set_blind(Ind, damage);
				break;
			case 3:
				set_stun(Ind, duration);
				break;
			case 4:
				set_poisoned(Ind, duration, Ind);
			case 5:
				set_slow(Ind, duration);
				break;
			default:
				break;
		}
		p_ptr->redraw |= PR_HP;
	}
		
	if(type & RPEN_MIN_ST)
	{
		d = randint(4);
		u16b mode = 0; u16b stat = 0;
		
		if(d>1)
			mode = STAT_DEC_TEMPORARY;
		else
			mode = STAT_DEC_PERMANENT;
		while(d)
		{
			d = randint(12+mod_luck);
			switch(d)
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
		}
		if(d)
		{
			d = damroll(1,30);
			dec_stat(Ind, stat, d, mode);
		}
	}
	/* Hurt sanity. With luck it may only confuse, scare or cause hallucinations. Should be less dangerous at low levels. */
	if(type & RPEN_MAJ_SN)
	{
		d = damroll(1,(p_ptr->lev*(55/mod_luck)));
		switch(d)
		{
			case 9:
			case 19:
			case 29:
				set_image(Ind, duration);
				break;
			case 10:
			case 20:
			case 30:
				set_afraid(Ind, duration);
				break;
			case 11:
			case 21:
			case 31:
				set_confused(Ind, duration);
				break;
			default:
				take_sanity_hit(Ind, damroll(1,d), "a malformed invocation");
				break;
		}
	}
		
	if(type & RPEN_MAJ_BK)
	{
		take_hit(Ind, damage, "a malformed invocation", 0);
		p_ptr->redraw |= (PR_HP);
	}
	
	if(type & RPEN_MAJ_BB)
	{
		if(p_ptr->black_breath != 1)
		{
			p_ptr->black_breath = 1;
			msg_print(Ind, "\377rYou have contracted the black breath.");
		}
		else
			take_hit(Ind, damage, "a malformed invocation", 0);
	}
	
	if(type & RPEN_MAJ_DT)
	{
		take_hit(Ind, damage*10, "a malformed invocation", 0);
	}
	
	if(type & RPEN_MAJ_SN)
		msg_print(Ind, "\377rYou feel a little less sane!");
	if(type & RPEN_MIN_ST)
		msg_print(Ind, "\377rYou feel a little less potent.");
	if(type & RPEN_MIN_SP)
		msg_print(Ind, "\377rYou feel a little drained.");
	
	p_ptr->redraw |= PR_HP;
	p_ptr->redraw |= PR_MANA;
	p_ptr->redraw |= PR_SPEED;
	p_ptr->redraw |= PR_SANITY;
	
	return 0;
}

/* rspell_skill

Takes both of the spell arrays, and their sizes.

Returns s_av, or the average (mean) of all the skills in a spell

This should maybe take spell-power into account*/
u16b rspell_skill(u32b Ind, u32b s_type)
{
	player_type *p_ptr = Players[Ind];
	u16b s = 0; u16b i; byte s_size = 0;
	
	byte s_runes[16];
	runes_in_flag(s_runes, s_type);
	
	for(i=0;i<16;i++)
	{
		if(s_runes[i]==1)
		{
			s+=get_skill(p_ptr, r_elements[i].skill);
			s_size++;
		}
	}
	if(s_size && s)
		s /= s_size;
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
	s16b fail = 0; u16b e_level = 0; u16b minfail = 0;
	u16b statbonus = 0;
	int adj_level = 0;
	
	int m = meth_to_id(s_flags);
	
	e_level = runespell_list[s_type].level;
	e_level += runespell_types[m].cost;
	e_level += (r_imperatives[imperative].danger/10);
	
	if(e_level == 3)
		e_level = 1; //Bonus for level 1 spells, so that level 1 casters can cast.
	
	u16b t;
	
	t = *mali;
	/* Runes are tactile as well as visual, so no need to test for darkness/blindness. Confusion and stunning are consequently worse. */
	if (p_ptr->confused)
	{
		*mali+=10;
	}
	if (p_ptr->stun > 50)
	{
		*mali += 10;
	}
	else if (p_ptr->stun)
	{
		*mali += 5;
	}
	
	/* Spells are harder to cast while holding on to other spells. */
	if (p_ptr->rune_num_of_buffs)
	{
		*mali += (p_ptr->rune_num_of_buffs * 5) + 3;
	}
	
	if(p_ptr->confused || p_ptr->stun || p_ptr->stun)
		if(t)
			msg_print(Ind, "\377yYou struggle to remember the rune-forms.");
		else
			msg_print(Ind, "\377yYou struggle to recite the rune-forms.");
	else if (t)
		msg_print(Ind, "\377yYou recite the rune-forms from memory.");
	
	statbonus += adj_mag_stat[p_ptr->stat_ind[A_INT]]*65/100;
	statbonus += adj_mag_stat[p_ptr->stat_ind[A_DEX]]*35/100;
	
	fail -= statbonus;
	
	if(fail <= 0) fail = 1;
	
	if(runespell_list[s_type].fail != 0) //Spell effect difficulty
		fail = (1+fail * runespell_list[s_type].fail)/10;
	
	if(fail <= 0) fail = 1;
	
	if (p_ptr->csp < s_cost)
	{
		*mali = *mali + (2*(s_cost - p_ptr->csp));
		if(p_ptr->csp == 0)
			*mali += 5;
	}
	
	/* Adjust chances with respect to spell level */
	adj_level = s_av - (e_level+5); //Skill needs to be spell_level + 5 for a perfect cast
	
	if(adj_level < 0)
		adj_level = adj_level*4; //4% fail per level below
	
	*mali -= adj_level;
		
	fail += *mali;
	
	minfail += adj_mag_fail[p_ptr->stat_ind[A_INT]]*65/100;
	minfail += adj_mag_fail[p_ptr->stat_ind[A_DEX]]*35/100;
	
	if (fail < minfail) fail = minfail;
	
	if(r_imperatives[imperative].fail == 0) //Imperative (*)
		fail = ((fail+1) * randint(40))/10;
	else
		fail = ((fail+1) * r_imperatives[imperative].fail)/10;
	
	if (fail > 95) fail = 95;
	
	return fail;
}
/* rspell_flag method
Element list to method flags (as long).
Takes the method array and it's size, returns the flags.
*/
u32b rspell_flag_method (r_element *method, u16b m_size)
{
	int i, n, match;
	int flags[8];
	n=0;
	for(i=0;i<8;i++)
		flags[i] = 0;
	for(i=0;i<m_size;i++)
	{
		for(n=0;n<8;n++)
			if (method[i].flags & runespell_types[n].type)
				flags[n] += 1;
	}

	i=8;
	match=0;
	while(i)
	{
		i--;
		if(flags[i] >= runespell_types[i].cost)
			if(i==1 || !(flags[i-1] >= runespell_types[i-1].cost))
				return runespell_types[i].type;
	}
	
	return R_MELE;
}


/* rspell_flag_type
Element list to element flags, takes the list array and its size, returns the flags as a long. */
u32b rspell_flag_type (r_element *spell, u16b s_size)
{
	u16b i;
	unsigned long flags=0;

	for(i=0;i<s_size;i++)
	{
		//Regex for the win
		if((spell[i].flags & R_FIRE)) { flags |= R_FIRE; }
		else if(spell[i].flags & R_COLD) { flags |= R_COLD; } 
		else if(spell[i].flags & R_ACID) { flags |= R_ACID; }
		else if(spell[i].flags & R_ELEC) { flags |= R_ELEC; }
		else if(spell[i].flags & R_POIS) { flags |= R_POIS; }
		else if(spell[i].flags & R_WATE) { flags |= R_WATE; }
		else if(spell[i].flags & R_WIND) { flags |= R_WIND; }
		else if(spell[i].flags & R_EART) { flags |= R_EART; } 
		else if(spell[i].flags & R_MANA) { flags |= R_MANA; }
		else if(spell[i].flags & R_CHAO) { flags |= R_CHAO; }
		else if(spell[i].flags & R_NETH) { flags |= R_NETH; }
		else if(spell[i].flags & R_NEXU) { flags |= R_NEXU; }
		else if(spell[i].flags & R_MIND) { flags |= R_MIND; }
		else if(spell[i].flags & R_TIME) { flags |= R_TIME; }
		else if(spell[i].flags & R_GRAV) { flags |= R_GRAV; }
		else if(spell[i].flags & R_FORC) { flags |= R_FORC; }
	}
	
	return flags;
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
			if ((rspell_selector[i].flags & flags) == rspell_selector[i].flags)
				return rspell_selector[i].type;
	}
	return RT_NONE;
}

u16b cast_runespell(u32b Ind, byte dir, u16b damage, u16b radius, u16b duration, s16b cost, u32b type, s16b difficulty, byte imper, u32b type_flags, u16b s_av, s16b mali)
/* Now that we have some numbers and figures to work with, cast the spell. MP is deducted here, and negative spell effects/failure stuff happens here. */
{
	u16b m, y, x, t;
	u16b fail_chance = 0;
	s16b margin = 0;
	s16b modifier = 100;
	s16b cost_m = 0;
	int success = 0;
	char * description = NULL;
	char * begin = NULL;
	fail_chance = randint(100);
	player_type * p_ptr = Players[Ind];
	u16b gf_type = runespell_list[type].gf_type;
	u16b e_level = runespell_list[type].level;
	m = meth_to_id(type_flags);
	e_level += runespell_types[m].cost;
	e_level += (r_imperatives[imper].danger/10);
	s16b speed = 0;
	int level = s_av - e_level;
	cave_type **zcave; //For glyph removal function of "disperse"
	if(!(zcave=getcave(&p_ptr->wpos))) printf("Not in a cave?");
	
	margin = fail_chance - difficulty;
	if(margin>-20) //For small failure values cast the spell
	{
		success = 1;
	}
	
	if(p_ptr->csp < cost)
	{
		begin = "\377sDrawing on your reserves, you";
	}
	else
	{
		begin = "\377sYou";
	}
	
	if (margin < -80)
	{
		description = " dangerously fail to";
		modifier = 100;
	}
	else if (margin < -20)
	{
		description = " fail to";
		modifier = 100;
	}
	else if (margin < 0)
	{
		description = " barely manage to";
		modifier = 70;
	}
	else if (margin < 20)
	{
		description = " clumsily";
		modifier = 90;
	}
	else if (margin < 60)
	{
		description = "";
		modifier = 100;
	}
	else if (margin < 80)
	{
		modifier = 110;
		description = " effectively";
	}
	else
	{
		modifier = 130;
		description = " elegantly";
	}
	
	if ((type_flags & R_SELF) && 
		(((type == RT_MAGIC_CIRCLE) && !allow_terraforming(&p_ptr->wpos, FEAT_GLYPH)) ||
		((type == RT_MAGIC_WARD) && !allow_terraforming(&p_ptr->wpos, FEAT_GLYPH)) ||
		((type == RT_WALLS || type == RT_EARTHQUAKE) && !allow_terraforming(&p_ptr->wpos, FEAT_WALL_EXTRA))))
	{
		description = " decide not to";
		success = 0;
	}
	
	if(modifier > 1)
	{
		damage = (damage*modifier)/100;
		duration = (duration*modifier)/100;
		radius = (radius*modifier)/100;
		
		cost_m = 100-modifier;
		cost += (cost*cost_m)/100;
		if(cost < 1)
			cost = 1;
	}
	
	if(type_flags & R_SELF)
	{
		if(runespell_list[type].gf_type == 0)
		{
			gf_type = GF_ARROW; //For fail penalties
		}
		/* Handle self spells here */
		switch (type)
		{
			case RT_PSI_ESP:
				msg_format(Ind, "%s%s cast a rune of extra-sensory perception. (%i turns)", begin, description, duration);
				if(success) set_tim_esp(Ind, duration);
				break;
			
			case RT_MAGIC_WARD:
				msg_format(Ind, "%s%s draw a sigil of protection.", begin, description);
				if(success) warding_glyph(Ind);
				break;
			
			case RT_MAGIC_CIRCLE:
				msg_format(Ind, "%s%s%s surround yourself with protective sigils.", begin, description);
			
				if(success) 
				{
					for(x = 0; x<3;x++)
						for(y = 0; y<3;y++)
							cave_set_feat_live(&p_ptr->wpos, (p_ptr->py+y-1), (p_ptr->px+x-1), FEAT_GLYPH);
					p_ptr->redraw |= PR_MAP;
				}
				else if (!istown(&p_ptr->wpos))
				{
					warding_glyph(Ind); //Failing this spell will probably kill the player: help them out with one glyph
				}
				break;
				
			case RT_WALLS:
				msg_format(Ind, "%s%s summon walls.", begin, description);
				if(success) fire_ball(Ind, GF_STONE_WALL, 0, 1, 1, "");
				break;
			
			case RT_VISION:
				msg_format(Ind, "%s%s summon magical vision.", begin, description);
				if(success)
				{
					if(level > 26) //Skill level at 40
					{
						wiz_lite_extra(Ind);
					}
					else
					{
						map_area(Ind);
					}
				}
				break;
				
			case RT_MYSTIC_SHIELD:
				msg_format(Ind, "%s%s summon mystic protection. (%i turns)", begin, description, duration);
				if(success) (void)set_shield(Ind, damage, duration, SHIELD_NONE, 0, 0);
				break;
			
			case RT_SHARDS:
			case RT_DETECT_TRAP:
				msg_format(Ind, "%s%s sense hidden traps.", begin, description);
				if(success) detect_trap(Ind, 36);
				break;
			
			case RT_STASIS_DISARM:
				msg_format(Ind, "%s%s cast a rune of trap destruction.", begin, description);
				if(success) destroy_doors_touch(Ind, 1 + ((s_av - runespell_list[type].level)*4)/50);
				break;
			
			case RT_FLY:
				msg_format(Ind, "%s%s summon ethereal wings. (%i turns)", begin, description, duration);
				if(success) set_tim_fly(Ind, duration);
				break;
			
			case RT_TRAUMATURGY:
				if(damage > 20)
					damage = 20;
				msg_format(Ind, "%s%s summon an otherworldly bloodlust. (%i turns)", begin, description, duration);
				set_tim_trauma(Ind, duration, damage);
				break;
				
			case RT_THUNDER:
				if(success) set_tim_thunder(Ind, 10+randint(10)+rget_level(25), 5+rget_level(10), 10+rget_level(25));
				break;
			
			case RT_TIME_INVISIBILITY:
				msg_format(Ind, "%s%s cast a rune of invisibility. (%i turns)", begin, description, duration);
				if(success) set_invis(Ind, duration, damage);
				printf("Made %s invisible (power: %i)", p_ptr->name,damage);
				break;
			
			case RT_BLESSING:
				msg_format(Ind, "%s%s summon a blessing. (%i turns)", begin, description, duration);
				if(success) set_blessed(Ind,duration);
				if(success) set_protevil(Ind, duration);
				break;
			
			case RT_SATIATION:
				msg_format(Ind, "%s%s cast a rune of satiation.", begin, description);
				if(success) set_food(Ind, PY_FOOD_MAX - 1);
				break;
			
			case RT_RESISTANCE:
				msg_format(Ind, "%s%s summon elemental protection. (%i turns)", begin, description, duration);
				if(success)
				{
					set_oppose_acid(Ind, duration);
					set_oppose_elec(Ind, duration);
					set_oppose_fire(Ind, duration);
					set_oppose_cold(Ind, duration);
				}
				break;
			
			case RT_FIRE:
				msg_format(Ind, "%s%s cast a rune of fire resistance. (%i turns)", begin, description, duration);
				if(success) set_oppose_fire(Ind, duration);
				break;
			
			case RT_COLD:
				msg_format(Ind, "%s%s cast a rune of cold resistance. (%i turns)", begin, description, duration);
				if(success) set_oppose_cold(Ind, duration);
				break;
			
			case RT_ACID:
				msg_format(Ind, "%s%s cast a rune of acid resistance. (%i turns)", begin, description, duration);
				if(success) set_oppose_acid(Ind, duration);
				break;
			
			case RT_ELEC:
				msg_format(Ind, "%s%s cast a rune of electrical resistance. (%i turns)", begin, description, duration);
				if(success) set_oppose_elec(Ind, duration);
				break;
			
			case RT_POISON:
				msg_format(Ind, "%s%s cast a rune of poison resistance.", begin, description);
				if(success) set_oppose_pois(Ind, duration);
				break;
			
			case RT_ICEPOISON:
			case RT_DISPERSE:
				msg_format(Ind, "%s%s banish the magical energies.", begin, description);
				if(success)
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
			case RT_QUICKEN:
				msg_format(Ind, "%s%s cast a rune of quickening. (%i turns)", begin, description, duration);
				speed = randint(damage);
				speed += 2;
				if(speed > 12)
					speed = 12;
				if(success) set_fast(Ind, duration, speed);
				break;
			
			case RT_DIG:
			case RT_DISINTEGRATE:
			case RT_TELEPORT:
				msg_format(Ind, "%s%s teleport.", begin, description);
				if(success)
				{
					if(inarea(&p_ptr->memory.wpos, &p_ptr->wpos))
					{
						teleport_player_to(Ind, p_ptr->memory.y, p_ptr->memory.x);
					}
					else 
					{
						teleport_player(Ind, (100+((s_av -runespell_list[type].level)*100)/50), FALSE);
					}
				}
				break;
			
			case RT_INERTIA:
			case RT_TELEPORT_TO:
				msg_format(Ind, "%s%s teleport forward.", begin, description);
				if(success) teleport_player_to(Ind, p_ptr->target_col, p_ptr->target_row);
				break;
			
			case RT_CHAOS:
			case RT_TELEPORT_LEVEL:
				msg_format(Ind, "%s%s teleport away.", begin, description);
				if(success)
				{
					teleport_player_level(Ind);
					
					if(randint(100)<=1) //A small amount of unreliability
						teleport_player_level(Ind);
					
					if(modifier != 130)
					{
						take_hit(Ind, (p_ptr->mhp*(0-(modifier-130))/100), "level teleportation", 0); //Hit for up to 60% of health, depending on inverse sucessfullness
					}
					
				}
				break;
			
			case RT_GRAVITY:
			case RT_SUMMON:
				msg_format(Ind, "%s%s summon monsters.", begin, description);
				if(success) project_hack(Ind, GF_TELE_TO, 0, " summons"); 
				break;
			
			case RT_ICE:
			case RT_MEMORY:
				msg_format(Ind, "%s%s memorise your position.", begin, description);
				
				if(success)
				{
					wpcopy(&p_ptr->memory.wpos, &p_ptr->wpos);
					p_ptr->memory.x = p_ptr->px;
					p_ptr->memory.y = p_ptr->py;
				}
				break;
				
			case RT_RECALL:
				msg_format(Ind, "%s%s recall yourself.", begin, description);
				if(success) set_recall_timer(Ind, damroll(1,100)); 
				break;

			case RT_EARTHQUAKE:
				msg_format(Ind, "%s%s summon an earthquake.", begin, description);
				if(success)
				{
					if(level>35)
						destroy_area(&p_ptr->wpos, p_ptr->py, p_ptr->px, 15, TRUE, FEAT_FLOOR);
					else
						earthquake(&p_ptr->wpos, p_ptr->py, p_ptr->px, 15);
				}
				break;
			
			case RT_WATERPOISON:
			case RT_DETECTION_BLIND:
				msg_format(Ind, "%s%s cast a rune of detection.", begin, description);
				if(success) 
				{
					if(level>40)
						detection(Ind, DEFAULT_RADIUS * 2);
					else if(level>20)
						detect_monsters_forced(Ind);
					else
						detect_creatures(Ind);
				}
				break;
			
			case RT_WATER:
			case RT_DETECT_STAIR:
				msg_format(Ind, "%s%s cast a rune of exit detection.", begin, description);
				if(success) detect_sdoor(Ind, 36);
				break;
			
			case RT_SEE_INVISIBLE:
				msg_format(Ind, "%s%s cast a rune of see invisible.", begin, description);
				if(success) set_tim_invis(Ind, duration);
				break;
			
			case RT_NEXUS:
				msg_format(Ind, "%s%s summon nexus.", begin, description);
				
				if(success)
				{
					switch(randint(10))
					{
						case 1: teleport_player(Ind, (10+((s_av -runespell_list[type].level)*10)/50), FALSE); break;
						case 2: set_recall_timer(Ind, damroll(1,100)); break;
						case 3: project_hack(Ind, GF_TELE_TO, 0, " summons"); break;
						case 4: teleport_player_level(Ind); break;
						case 5: teleport_player_to(Ind, p_ptr->target_col, p_ptr->target_row); break;
						default: teleport_player(Ind, (100+((s_av -runespell_list[type].level)*100)/50), FALSE); break;
					}
				}
				break;
			
			case RT_CLONE_BLINK:
			case RT_WIND_BLINK:
				msg_format(Ind, "%s%s blink.", begin, description);
				if(success) teleport_player(Ind, (10+((s_av -runespell_list[type].level)*10)/50), FALSE);
				break;
			
			case RT_LIGHT:
				msg_format(Ind, "%s%s summon light.", begin, description);
			
				if(success)
				{
					if (level > 9)
						lite_area(Ind, 10, 4);
					lite_room(Ind, &p_ptr->wpos, p_ptr->py, p_ptr->px);
				}
				
				break;
			
			case RT_PLASMA:
			case RT_BRILLIANCE:
				msg_format(Ind, "%s%s summon bright light.", begin, description);
			
				if(success)
				{
					lite_area(Ind, 10, 8);
					lite_room(Ind, &p_ptr->wpos, p_ptr->py, p_ptr->px);
					fire_ball(Ind, GF_LITE, 0, damage, 4, "");
				}
				
				break;
			
			case RT_DARK_SLOW:
			case RT_SHADOW:
				msg_format(Ind, "%s%s summon shadows.", begin, description);
			
				if(success)
				{
					if (level > 9)
						unlite_area(Ind, 10, 4);
					unlite_room(Ind, &p_ptr->wpos, p_ptr->py, p_ptr->px);
				}
				
				break;
			
			case RT_HELL_FIRE:
			case RT_OBSCURITY:
				msg_format(Ind, "%s%s summon obscurity.", begin, description);
			
				if(success)
				{
					unlite_area(Ind, 10, 8);
					unlite_room(Ind, &p_ptr->wpos, p_ptr->py, p_ptr->px);
					fire_ball(Ind, GF_DARK, 0, damage, 4, "");
				}
				
				break;
				
			case RT_STEALTH:
				msg_format(Ind, "%s%s cast a rune of stealth.", begin, description);
				/* Unset */
				if(success)
				{
					if (p_ptr->rune_stealth)
					{
						printf("Set %s's stealth buff to %i\n",p_ptr->name,0);
						p_ptr->rune_stealth = 0;
						p_ptr->rune_num_of_buffs--;
						msg_print(Ind, "You feel less stealthy.");
					}
					/* Set */
					else
					{
						damage /= 2;
						if(damage > 7)
							damage = 7;
						printf("Set %s's stealth buff to %i\n",p_ptr->name,(damage));
						p_ptr->rune_stealth = (damage);
						p_ptr->rune_num_of_buffs++;
						msg_print(Ind, "You feel more stealthy.");
					}
					p_ptr->redraw |= PR_EXTRA;
				}
				break;
			
			case RT_ROCKET:
			case RT_MISSILE:
				duration = duration/10;
				if(duration < 5) duration = 5;
				else if(duration > 20) duration = 20;
				
				msg_format(Ind, "%s%s cast a rune of deflection. (%i turns)", begin, description, duration);
				
				if(success)
				{
					set_tim_deflect(Ind,duration);
				}
				break;
			
			case RT_NUKE:
			case RT_ANCHOR:
				msg_format(Ind, "%s%s constrict the space-time continuum.", begin, description);
				if(success) set_st_anchor(Ind, duration);
				break;
			
			case RT_FURY:
				msg_format(Ind, "%s%s summon fury. (%i turns)", begin, description, duration);
				if(success) set_fury(Ind, duration);
				break;
			
			case RT_BESERK:
				if(duration > 40) duration = 40;
				msg_format(Ind, "%s%s cast a rune of beserking. (%i turns)", begin, description, duration);
				if(success) set_adrenaline(Ind, duration);
				break;
			
			case RT_POLYMORPH:
			case RT_HEALING:
				msg_format(Ind, "%s%s summon healing.", begin, description);
				if(success) hp_player(Ind, damage);
				break;
			
			case RT_NETHER:
			case RT_AURA:
				msg_format(Ind, "%s%s summon a fiery aura. (%i turns)", begin, description, duration);
				if(success)
				{
					if(level > 35) //i.e. Player lv 42+
						set_shield(Ind, duration, damage, SHIELD_GREAT_FIRE, SHIELD_GREAT_FIRE, 0);
					else
						set_shield(Ind, duration, damage, SHIELD_FIRE, SHIELD_FIRE, 0);
				}
				break;
			
			default:
				break;
		}
	}
	else
	{
		if(type_flags & R_LOS)
		{
			sprintf(p_ptr->attacker, " fills the air with %s for", runespell_list[type].title);
			msg_format(Ind, "%s%s fill the air with %s.", begin, description, runespell_list[type].title);
			if(success) project_hack(Ind, gf_type, damage, p_ptr->attacker);
		}
		else if(type_flags & R_WAVE)
		{
			sprintf(p_ptr->attacker, " summons a wave of %s for", runespell_list[type].title);
			msg_format(Ind, "%s%s summon a %s wave of %s.", begin, description, r_imperatives[imper].name, runespell_list[type].title);
			if(success) fire_wave(Ind, gf_type, 0, damage, radius, duration, 10, EFF_LAST, p_ptr->attacker);
		}
		if(type_flags & R_MELE)
		{
			int px = p_ptr->px;
			int py = p_ptr->py;
			int tx = p_ptr->target_col;
			int ty = p_ptr->target_row;
			
			int dx = px - tx;
			int dy = py - ty;
			
			if(dx > 1 || dx < -1 || dy > 1 || dy < -1)
			{
				p_ptr->target_col = px;
				p_ptr->target_row = py;
			}
			
			if((dx > 1 || dx < -1 || dy > 1 || dy < -1) && tx != 0 && ty != 0)
			{
				sprintf(p_ptr->attacker, " is summons %s for", runespell_list[type].title);
				msg_format(Ind, "%s%s summon a %s burst of undirected %s.", begin, description, r_imperatives[imper].name, runespell_list[type].title);
				if(success) fire_wave(Ind, gf_type, 0, (damage - damage/4), 1, 1, 1, EFF_STORM, p_ptr->attacker);
			}
			else
			{
				sprintf(p_ptr->attacker, " is summons %s for", runespell_list[type].title);
				msg_format(Ind, "%s%s summon a %s burst of %s.", begin, description, r_imperatives[imper].name, runespell_list[type].title);
				if(success) fire_bolt(Ind, gf_type, dir, damage, p_ptr->attacker);
			}
		}
		else if((type_flags & R_BALL))
		{
			sprintf(p_ptr->attacker, " summons a ball of %s for", runespell_list[type].title);
			msg_format(Ind, "%s%s summon a %s ball of %s.", begin, description, r_imperatives[imper].name, runespell_list[type].title);
			if(success) fire_ball(Ind, gf_type, dir, damage, radius, p_ptr->attacker);
		}
		else if(type_flags & R_CLOU)
		{
			sprintf(p_ptr->attacker, " summons a cloud of %s for", runespell_list[type].title);
			msg_format(Ind, "%s%s summon a %s cloud of %s.", begin, description, r_imperatives[imper].name, runespell_list[type].title);
			if(success) fire_cloud(Ind, gf_type, dir, damage, radius, duration, 9, p_ptr->attacker);
		}
		else if(type_flags & R_BOLT || type_flags & R_BEAM)
		{
			t = 0;
			if(type_flags & R_BEAM)
			{
				t = 1;
			}
				
			if(type==RT_DIG) //Regular bolt and beam will stop before the wall
			{
				int flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
				if(success) project_hook(Ind, GF_KILL_WALL, dir, 20 + randint(30), flg, "");
				msg_format(Ind, "%s%s cast a rune of wall destruction.", begin, description);
			}
			else
			{
				if(t==0)
				{
					sprintf(p_ptr->attacker, " summons a bolt of %s for", runespell_list[type].title);
					msg_format(Ind, "%s%s summon a %s bolt of %s.", begin, description, r_imperatives[imper].name, runespell_list[type].title);
					if(success) fire_bolt(Ind, gf_type, dir, damage, p_ptr->attacker);
				}
				else
				{
					sprintf(p_ptr->attacker, " summons a beam of %s for", runespell_list[type].title);
					msg_format(Ind, "%s%s summon a %s beam of %s.", begin, description, r_imperatives[imper].name, runespell_list[type].title);
					if(success) fire_beam(Ind, gf_type, dir, damage, p_ptr->attacker);
				}
			}
		}
	}
	
	if(p_ptr->csp > cost)
	{
		p_ptr->csp -= cost;
	}
	else
	{
		/* Any MP the caster does not have comes out of their HP. */
		take_hit(Ind, (cost-p_ptr->csp), "magical exhaustion", 0);
		p_ptr->csp = 0;
	}
	
	if(margin<0)
	{
		difficulty = (difficulty+mali>0 ? difficulty+mali : 0);
		difficulty = (difficulty-margin>0 ? difficulty-margin : 0);
		rspell_do_penalty(Ind, rspell_penalty(Ind, difficulty), damage, duration, cost, gf_type, "",  imper); //Then do the self-harm
	}
	
	p_ptr->energy -= (level_speed(&p_ptr->wpos)*(101-s_av))/100;
	
	p_ptr->redraw |= PR_MANA;
	
	return 0;
}

byte execute_rspell (u32b Ind, byte dir, u32b s_flags, byte imperative)
{
	/*
	If s_flags is set, use that to create an effect based on the character's skill and whatnot.
	If it isn't, create it from expr.
	
	Call the effect.
	*/

	player_type * p_ptr = Players[Ind];
	
	u32b s_type = 0;
	s16b s_cost = 0; u16b s_dam = 0;  s16b s_diff = 0;
	u16b s_av = 0;

	u16b radius = 0; u16b duration = 0;
	s16b mali = 0;
	
	s_av = rspell_skill(Ind, s_flags);
	
	/* At max level, a spell takes about half the time to cast */
	if(p_ptr->energy < ((level_speed(&p_ptr->wpos)*(101-s_av))/100))
		return 0;
	
	if(s_av<=0)
	{
		msg_print(Ind, "\377rYou don't know these runes.");
		
		return 0;
	}
	
	s_type = rspell_type(s_flags);
	
	if(s_type == RT_NONE)
	{
		msg_print(Ind, "\377rThis is not a runespell.");
		return 0;
	}
	s_cost = rspell_cost(Ind, s_type, s_flags, s_av, imperative);
	s_dam = rspell_dam(Ind, &radius, &duration, s_type, s_flags, s_av, imperative);
	if(!rspell_check(Ind, &mali, s_flags))
		mali += 5; //+15% fail for first missing rune, +10% per additional rune
	s_diff = rspell_diff(Ind, imperative, s_cost, s_av, s_type, s_flags, &mali);

	cast_runespell(Ind, dir, s_dam, radius, duration, s_cost, s_type, s_diff, imperative, s_flags, s_av, mali);
	return 1;
}
#endif
