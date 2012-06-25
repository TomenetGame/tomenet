/* $Id$ */
/* File: runecraft.c */

/* Purpose: New Runecraft Spell System */

/* by Relsiet/Andy Dubbeld (andy@foisdin.com) */
/* maintained by Kurzel (kurzel.tomenet@gmail.com) */

#include "angband.h"

/*
Spells are created on the fly with an mkey interface as a combination of elements, an imperative and a type.
*/
#ifdef ENABLE_RCRAFT
//#define LIMIT_NON_RUNEMASTERS //2-rune maximum for adventurers. Disabled for now, as earlier extravagant utility access has been removed.

#define ENABLE_GROUP_SPELLS //Allow new runemasters to accept 'rune charges' for combination spells.
//#define ENABLE_RUNE_GOLEMS //Allow the runespells pertaining to rune golems!
//#define ENABLE_RUNE_CURSES //Allow the runespells pertaining to rune curses!

//#define ENABLE_AUGMENTS //Modify runespell parameters based on included elements.
//#define CONSUME_RUNES //Allow self-spells to 'cost' additional runes, ie. for 'socketing'.

#define ENABLE_BLIND_CASTING //Blinding increases fail chance instead of preventing the cast. (Assume runes 'glow' here, no 'no_lite(Ind)' check as with books.)
//#define ENABLE_CONFUSED_CASTING //Consusion increases fail chance instead of preventing the cast.
#define ENABLE_SHELL_PROJECTIONS //AM shell stops only self-spells with this enabled. (Assume runes 'emit' the magic, not the caster.)

#define ENABLE_LIFE_CASTING //Allows casting with HP if not enough MP.
#define SAFE_RETALIATION //Disable retaliation/FTK when casting with HP.

#define FEEDBACK_MESSAGE //Gives the caster a feedback message if penalized.

//#define RCRAFT_DEBUG //Enables debugging messages for the server.

/* Decode Runespell */
byte flags_to_elements(byte element[], u16b e_flags);
byte flags_to_imperative(u16b m_flags);
byte flags_to_type(u16b m_flags);
/* Validation Parameters */
byte rspell_skill(u32b Ind, byte element[], byte elements);
byte rspell_level(byte element[], byte elements, byte imperative, byte type);
s16b rspell_diff(byte skill, byte level);
s16b rspell_energy(u32b Ind, byte element[], byte elements, byte imperative, byte type);
#ifdef ENABLE_GROUP_SPELLS
bool is_group_spell(s16b rune[], u16b e_flags1, u16b e_flags2, byte elements);
#endif
byte rspell_cost(u32b Ind, byte element[], byte elements, byte imperative, byte type, byte skill, byte level);
s16b rspell_inventory(u32b Ind, byte element[], byte elements, u16b *mali);
byte rspell_fail(u32b Ind, byte element[], byte elements, byte imperative, byte type, s16b diff, u16b mali);
/* Decode Projection */
byte flags_to_projection(u16b flags);
/* Remaining Parameters */
u32b rspell_damage(u32b Ind, byte element[], byte elements, byte imperative, byte type, byte skill, byte projection);
void rspell_penalty(s16b margin, byte *p_flags);
byte rspell_radius(u32b Ind, byte element[], byte elements, byte imperative, byte type, byte skill, byte level);
byte rspell_duration(u32b Ind, byte element[], byte elements, byte imperative, byte type, byte skill, byte level);
/* Penalty Function */
void rspell_do_penalty(u32b Ind, u32b gf_type, u32b damage, byte cost, byte p_flags, s16b link);
/* Additional Functions */
#ifdef CONSUME_RUNES
bool rspell_socket(u32b Ind, byte rune);
#endif
/* Main Function */
byte execute_rspell(u32b Ind, byte dir, u16b e_flags1, u16b e_flags2, u16b m_flags, bool retaliate);

byte flags_to_elements(byte element[], u16b e_flags) {
	byte elements = 0;
	byte i;
	for (i = 0; i < RCRAFT_MAX_ELEMENTS; i++) {
		if ((e_flags & r_elements[i].flag) == r_elements[i].flag) {
			element[elements] = i;
			elements++;
		}
	}
	return elements;
}

byte flags_to_imperative(u16b m_flags) {
	byte i;
	for (i = 0; i < RCRAFT_MAX_IMPERATIVES; i++) {
		if ((m_flags & r_imperatives[i].flag) == r_imperatives[i].flag) return i;
	}
	return -1;
}

byte flags_to_type(u16b m_flags) {
	byte i;
	for (i = 0; i < RCRAFT_MAX_TYPES; i++) {
		if ((m_flags & r_types[i].flag) == r_types[i].flag) return i;
	}
	return -1;
}

byte rspell_skill(u32b Ind, byte element[], byte elements) {
	player_type *p_ptr = Players[Ind];
	u16b skill = 0;
	byte i;
	for (i = 0; i < elements; i++) {
		skill += get_skill(p_ptr, r_elements[element[i]].skill);
	}
	skill /= elements;
	return (byte)skill;
}

byte rspell_level(byte element[], byte elements, byte imperative, byte type) {
	u16b level = 0;
	byte i;
	for (i = 0; i < elements; i++) {
		level += r_elements[element[i]].level;
	}
	level += r_imperatives[imperative].level;
	level += r_types[type].level;
	/* Normalize */
	switch (elements) {
		case 1:
		level = level * (50 - RSPELL_MIN_LVL_1) / RSPELL_MAX_LVL_1 + RSPELL_MIN_LVL_1;
		break;
		case 2:
		level = level * (50 - RSPELL_MIN_LVL_2) / RSPELL_MAX_LVL_2 + RSPELL_MIN_LVL_2;
		break;
		case 3:
		level = level * (50 - RSPELL_MIN_LVL_3) / RSPELL_MAX_LVL_3 + RSPELL_MIN_LVL_3;
	}
	return (byte)level;
}

s16b rspell_diff(byte skill, byte level) {
	s16b diff = skill - level;
	if (diff > S_DIFF_MAX) return S_DIFF_MAX;
	else return diff;
}

s16b rspell_energy(u32b Ind, byte element[], byte elements, byte imperative, byte type) {
	player_type * p_ptr = Players[Ind];
	s16b energy = level_speed(&p_ptr->wpos) / (1 + (S_ENERGY_CPR * get_skill(p_ptr, SKILL_RUNEMASTERY) / 50)); //steps at 50 / S_ENERGY_CPR
	//s16b energy = level_speed(&p_ptr->wpos) * 50 / (50 + S_ENERGY_CPR * get_skill(p_ptr, SKILL_RUNEMASTERY));
	
#ifdef ENABLE_AUGMENTS
	if (p_ptr->rcraft_augment == -1) {
		byte i;
		for (i = 0; i < elements; i++) {
			if (r_elements[element[i]].energy > 10) energy = energy * (10 + (r_elements[element[i]].energy - 10) / elements) / 10;
			else energy = energy * (10 - (10 - r_elements[element[i]].energy) / elements) / 10;
		}
	}
	else energy = energy * r_elements[p_ptr->rcraft_augment].energy / 10;
#endif
	
	energy = energy * r_imperatives[imperative].energy / 10;
	energy = energy * r_types[type].energy / 10;
	return energy;
}

#ifdef ENABLE_GROUP_SPELLS
bool is_group_spell(s16b rune[], u16b e_flags1, u16b e_flags2, byte elements) {	
	byte temp_array[1];
	u16b temp_flag;
	/* Clear the flag array for up to 2 runes */
	byte i;
	for (i = 0; i < RSPELL_MAX_ELEMENTS - 1; i++) {
		rune[i] = -1;
	}
	/* Hack -- Search for the rune(s) to combine - Kurzel */
	switch (elements) {
		case 2:
		if ((e_flags1 & R_TIME) == R_TIME) {
			temp_flag = e_flags1 - R_TIME;
			if (temp_flag == R_POIS || temp_flag == R_NEXU || temp_flag == R_FORC) return FALSE;
			flags_to_elements(temp_array, temp_flag);
			rune[0] = temp_array[0];
			return TRUE;
		}
		else return FALSE;
		case 3:
		//Is R_TIME in the leading pair?
		if ((e_flags1 & R_TIME) == R_TIME) {
			temp_flag = e_flags1 - R_TIME;
			if (temp_flag == R_POIS || temp_flag == R_NEXU || temp_flag == R_FORC) return FALSE;
			flags_to_elements(temp_array, temp_flag);
			rune[0] = temp_array[0];
		}
		else return FALSE;
		//Is R_TIME *also* in the trailing pair?
		if ((e_flags2 & R_TIME) == R_TIME) {
			temp_flag = e_flags2 - R_TIME;
			if (temp_flag == R_POIS || temp_flag == R_NEXU || temp_flag == R_FORC) return FALSE;
			flags_to_elements(temp_array, temp_flag);
			rune[1] = temp_array[0];
		}
		else { //otherwise, time was the first rune
			temp_flag = e_flags2 - r_elements[rune[0]].flag;
			if (temp_flag == R_POIS || temp_flag == R_NEXU || temp_flag == R_FORC) return FALSE;
			flags_to_elements(temp_array, temp_flag);
			rune[1] = temp_array[0];
		}
		return TRUE;
		default:
		return FALSE;
	}
}	
#endif

byte rspell_cost(u32b Ind, byte element[], byte elements, byte imperative, byte type, byte skill, byte level) {
//unused?	player_type * p_ptr = Players[Ind];
	u16b cost = ((level + skill) / (2 * S_COST_DIV));
	//cost = cost * 50 / (50 + S_ENERGY_CPR * get_skill(p_ptr, SKILL_RUNEMASTERY));
	
#ifdef ENABLE_AUGMENTS
	if (p_ptr->rcraft_augment == -1) {
		byte i;
		for (i = 0; i < elements; i++) {
			if (r_elements[element[i]].cost > 10) cost = cost * (10 + (r_elements[element[i]].cost - 10) / elements) / 10;
			else cost = cost * (10 - (10 - r_elements[element[i]].cost) / elements) / 10;
		}
	}
	else cost = cost * r_elements[p_ptr->rcraft_augment].cost / 10;
#endif
	
	cost = cost * r_imperatives[imperative].cost / 10;
	cost = cost * r_types[type].cost / 10;
	
	/* Hard limit */
	if (cost < S_COST_MIN) cost = S_COST_MIN;
	if (cost > S_COST_MAX) cost = S_COST_MAX;
	
	return (byte)cost;
}

s16b rspell_inventory(u32b Ind, byte element[], byte elements, u16b *mali) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	
	/* Predetermine the rune to break; remain negative if not found - Kurzel*/
	s16b link = -randint(elements);
	
	/* Reduce the mali for each rune found */
	byte i,j;
	for (i = 0; i < INVEN_TOTAL; i++) {
		if (i >= INVEN_PACK) continue;		
		o_ptr = &p_ptr->inventory[i];
		if ((o_ptr->tval == TV_RUNE2) && (o_ptr->level || o_ptr->owner == p_ptr->id)) {
			for (j = 0; j < elements; j++) { 
				if (o_ptr->sval == element[j]) {
					*mali -= S_FAIL_RUNE;
					/* Store the inventory location of a predetermined rune */
					if (-link == j + 1) link = i;
				}
			}
		}
	}
	
	return link;
}

byte rspell_fail(u32b Ind, byte element[], byte elements, byte imperative, byte type, s16b diff, u16b mali) {
	player_type *p_ptr = Players[Ind];

	/* Set the base failure rate; currently 50% at equal skill to level such that the range is [5,95] over 30 levels - Kurzel */
	s16b fail = 3 * (S_DIFF_MAX - diff) + 5;

#ifdef ENABLE_AUGMENTS
	if (p_ptr->rcraft_augment == -1) {
		byte i;
		for (i = 0; i < elements; i++) {
			fail += r_elements[element[i]].fail / elements;
		}
	}
	else fail += r_elements[p_ptr->rcraft_augment].fail;
#endif
	
	fail += r_imperatives[imperative].fail;
	fail += r_types[type].fail;

	fail += mali; //Place before stat modifier; casters of great ability can reduce the penalty for casting while hindered. - Kurzel
	
	/* Reduce failure rate by STAT adjustment */
	fail -= 3 * ((adj_mag_stat[p_ptr->stat_ind[A_INT]] * 65 + adj_mag_stat[p_ptr->stat_ind[A_DEX]] * 35) / 100 - 1);
	
	/* Extract the minimum failure rate */
        s16b minfail = (adj_mag_fail[p_ptr->stat_ind[A_INT]] * 65 + adj_mag_fail[p_ptr->stat_ind[A_DEX]] * 35) / 100;

	/* Minimum failure rate */
	if (fail < minfail) fail = minfail;
	
	/* Always a 5 percent chance of working */
	if (fail > 95) fail = 95;
	
	return (byte)fail;
}

byte flags_to_projection(u16b flags) {
	byte i;
	for (i = 0; i < RCRAFT_MAX_PROJECTIONS; i++) {
		if (flags == r_projections[i].flags) {
			if (r_projections[i].gf_type == GF_WONDER) //Wonder hack - Kurzel
				while (r_projections[i].gf_type == GF_WONDER) { i = randint(RCRAFT_MAX_PROJECTIONS) - 1; }
			return i;
		}
	}
	return -1;
}

u32b rspell_damage(u32b Ind, byte element[], byte elements, byte imperative, byte type, byte skill, byte projection) {
	player_type * p_ptr = Players[Ind];
	u32b damage = 0, damage_dice, weight_hi, weight_lo, influence;
	switch (r_projections[projection].gf_class) {
		case DT_DIRECT: {
		weight_hi = W_MAX_DIR;
		weight_lo = W_MIN_DIR;
		influence = W_INF_DIR;
		
		damage = weight_hi * ((10 - influence) + (influence * (r_projections[projection].weight - weight_lo) / (weight_hi - weight_lo))) / 10 + weight_lo;
		byte skill_rune = get_skill(p_ptr, SKILL_RUNEMASTERY);
		damage = damage * (10 + 2 * (S_ENERGY_CPR * skill_rune / 50)) / (1 + (S_ENERGY_CPR * skill_rune / 50)) / 10; //steps at 50 / S_ENERGY_CPR
		damage_dice = damage;
		
#ifdef ENABLE_AUGMENTS
		if (p_ptr->rcraft_augment == -1) {
			byte i;
			for (i = 0; i < elements; i++) {
				if (r_elements[element[i]].damage > 10) damage = damage * (10 + (r_elements[element[i]].damage - 10) / elements) / 10;
				else damage = damage * (10 - (10 - r_elements[element[i]].damage) / elements) / 10;
			}
		}
		else damage = damage * r_elements[p_ptr->rcraft_augment].damage / 10;
#endif
		
		damage = damage * r_imperatives[imperative].damage / 10;
		damage = damage * r_types[type].damage / 10;
		
		switch (r_types[type].flag) {
			case T_MELE:
				damage = damroll(2 + rget_level(damage_dice * 28 / weight_hi), 2 + rget_level(damage * 28 / weight_hi)) ;
				break;
			case T_SELF:
				damage = damage * rget_level(25) * rget_level(25) / weight_hi;
				break;
			case T_BOLT:
				damage = damroll(3 + rget_level(damage_dice * 37 / weight_hi), 1 + rget_level(damage * 19 / weight_hi));
				break;
			case T_BEAM:
				damage = damroll(3 + rget_level(damage_dice * 37 / weight_hi), 1 + rget_level(damage * 19 / weight_hi));
				break;
			case T_BALL:
				damage = damage * rget_level(25) * rget_level(25) / weight_hi;
				if (damage < 5) damage = 5;
				break;
			case T_WAVE:
				//damage = damage * (10 + 2 * (S_ENERGY_CPR * skill_rune / 50)) / 10; //Shall we fix dot damage decrease?
				damage = damage * rget_level(25) * rget_level(25) / 3 / weight_hi;
				if (damage < 3) damage = 3;
				break;
			case T_CLOU:
				damage = damage * (10 + 2 * (S_ENERGY_CPR * skill_rune / 50)) / 10; //Shall we fix dot damage decrease?
				damage = damage * rget_level(25) * rget_level(25) / 5 / weight_hi;
				break;
			case T_STOR:
				damage = damage * (10 + 2 * (S_ENERGY_CPR * skill_rune / 50)) / 10; //Shall we fix dot damage decrease?
				damage = damage * rget_level(25) * rget_level(25) / 5 / weight_hi;
				break;
		}
		break; }
		
		case DT_INDIRECT: {
		weight_hi = W_MAX_IND;
		weight_lo = W_MIN_IND;
		influence = W_INF_IND;
		
		damage = weight_hi * ((10 - influence) + (influence * (r_projections[projection].weight - weight_lo) / (weight_hi - weight_lo))) / 10 + weight_lo;
		byte skill_rune = get_skill(p_ptr, SKILL_RUNEMASTERY);
		damage = damage * (10 + 2 * (S_ENERGY_CPR * skill_rune / 50)) / (1 + (S_ENERGY_CPR * skill_rune / 50)) / 10; //steps at 50 / S_ENERGY_CPR
		damage_dice = damage;
		
#ifdef ENABLE_AUGMENTS
		if (p_ptr->rcraft_augment == -1) {
			byte i;
			for (i = 0; i < elements; i++) {
				if (r_elements[element[i]].damage > 10) damage = damage * (10 + (r_elements[element[i]].damage - 10) / elements) / 10;
				else damage = damage * (10 - (10 - r_elements[element[i]].damage) / elements) / 10;
			}
		}
		else damage = damage * r_elements[p_ptr->rcraft_augment].damage / 10;
#endif
		
		damage = damage * r_imperatives[imperative].damage / 10;
		damage = damage * r_types[type].damage / 10;
		
		switch (r_types[type].flag) {
			case T_MELE:
				damage = damroll(2 + rget_level(damage_dice * 3 / weight_hi), 2 + rget_level(damage * 3 / weight_hi)) ;
				break;
			case T_SELF:
				damage = damage * rget_level(5) * rget_level(5) / weight_hi;
				break;
			case T_BOLT:
				damage = damroll(3 + rget_level(damage_dice * 2 / weight_hi), 1 + rget_level(damage * 4 / weight_hi));
				break;
			case T_BEAM:
				damage = damroll(3 + rget_level(damage_dice * 2 / weight_hi), 1 + rget_level(damage * 4 / weight_hi));
				break;
			case T_BALL:
				damage = damage * rget_level(5) * rget_level(5) / weight_hi;
				if (damage < 5) damage = 5;
				break;
			case T_WAVE:
				//damage = damage * (10 + 2 * (S_ENERGY_CPR * skill_rune / 50)) / 10; //Shall we fix dot damage decrease?
				damage = damage * rget_level(5) * rget_level(5) / 3 / weight_hi;
				if (damage < 3) damage = 3;
				break;
			case T_CLOU:
				damage = damage * (10 + 2 * (S_ENERGY_CPR * skill_rune / 50)) / 10; //Shall we fix dot damage decrease?
				damage = damage * rget_level(5) * rget_level(5) / 5 / weight_hi;
				break;
			case T_STOR:
				damage = damage * (10 + 2 * (S_ENERGY_CPR * skill_rune / 50)) / 10; //Shall we fix dot damage decrease?
				damage = damage * rget_level(5) * rget_level(5) / 5 / weight_hi;
				break;
		}
		break; }
		
		case DT_EFFECT: {
		weight_hi = W_MAX_DIR;
		weight_lo = W_MIN_DIR;
		influence = W_INF_DIR;
		
		damage = weight_hi * ((10 - influence) + (influence * (r_projections[projection].weight - weight_lo) / (weight_hi - weight_lo))) / 10 + weight_lo;
		byte skill_rune = get_skill(p_ptr, SKILL_RUNEMASTERY);
		damage = damage * (10 + 2 * (S_ENERGY_CPR * skill_rune / 50)) / (1 + (S_ENERGY_CPR * skill_rune / 50)) / 10; //steps at 50 / S_ENERGY_CPR
		damage_dice = damage;
		
#ifdef ENABLE_AUGMENTS
		if (p_ptr->rcraft_augment == -1) {
			byte i;
			for (i = 0; i < elements; i++) {
				if (r_elements[element[i]].damage > 10) damage = damage * (10 + (r_elements[element[i]].damage - 10) / elements) / 10;
				else damage = damage * (10 - (10 - r_elements[element[i]].damage) / elements) / 10;
			}
		}
		else damage = damage * r_elements[p_ptr->rcraft_augment].damage / 10;
#endif
		
		damage = damage * r_imperatives[imperative].damage / 10;
		damage = damage * r_types[type].damage / 10;
		
		switch (r_types[type].flag) {
			case T_MELE:
				damage = damroll(2 + rget_level(damage_dice * 28 / weight_hi), 2 + rget_level(damage * 28 / weight_hi)) ;
				break;
			case T_SELF:
				damage = damage * rget_level(25) * rget_level(25) / weight_hi;
				break;
			case T_BOLT:
				damage = damroll(3 + rget_level(damage_dice * 37 / weight_hi), 1 + rget_level(damage * 19 / weight_hi));
				break;
			case T_BEAM:
				damage = damroll(3 + rget_level(damage_dice * 37 / weight_hi), 1 + rget_level(damage * 19 / weight_hi));
				break;
			case T_BALL:
				damage = damage * rget_level(25) * rget_level(25) / weight_hi;
				if (damage < 5) damage = 5;
				break;
			case T_WAVE:
				//damage = damage * (10 + 2 * (S_ENERGY_CPR * skill_rune / 50)) / 10; //Shall we fix dot damage decrease?
				damage = damage * rget_level(25) * rget_level(25) / 3 / weight_hi;
				if (damage < 3) damage = 3;
				break;
			case T_CLOU:
				damage = damage * (10 + 2 * (S_ENERGY_CPR * skill_rune / 50)) / 10; //Shall we fix dot damage decrease?
				damage = damage * rget_level(25) * rget_level(25) / 5 / weight_hi;
				break;
			case T_STOR:
				damage = damage * (10 + 2 * (S_ENERGY_CPR * skill_rune / 50)) / 10; //Shall we fix dot damage decrease?
				damage = damage * rget_level(25) * rget_level(25) / 5 / weight_hi;
				break;
		}
		break; }
		
		case DT_HACK: {
		damage = r_projections[projection].weight;
		break; }
	}
	
	/* Hard limit */
	if (damage < 1) damage = 1;
	
	return damage;
}

void rspell_penalty(s16b margin, byte *p_flags) {
	if (margin > 0) {
		*p_flags |= RPEN_MIN_RN;
	}
	else {
		byte roll = randint(-margin);
		if (roll > 90) {
			*p_flags |= RPEN_MIN_RN;
			*p_flags |= RPEN_MAJ_DT;
		}
		else if (roll > 80) {
			*p_flags |= RPEN_MIN_RN;
			*p_flags |= RPEN_MAJ_HP;
		}
		else if (roll > 60) {
			*p_flags |= RPEN_MIN_ST;
			*p_flags |= RPEN_MAJ_ST;
		}
		else if (roll > 30) {
			*p_flags |= RPEN_MIN_RN;
			*p_flags |= RPEN_MAJ_SN;
		}
		else if (roll > 25) {
			*p_flags |= RPEN_MIN_RN;
			*p_flags |= RPEN_MIN_ST;
		}
		else if (roll > 20) {
			*p_flags |= RPEN_MIN_HP;
		}
		else if (roll > 15) {
			*p_flags |= RPEN_MIN_SP;
		}
		else if (roll > 5) {
			*p_flags |= RPEN_MIN_RN;
		}
	}
	return;
}

byte rspell_radius(u32b Ind, byte element[], byte elements, byte imperative, byte type, byte skill, byte level) {
//unused?	player_type * p_ptr = Players[Ind];
	s16b radius = 0;
	
#ifdef ENABLE_AUGMENTS
	if (!(p_ptr->rcraft_augment == -1)) radius += r_elements[p_ptr->rcraft_augment].radius; //Toggle - Kurzel
#endif
	
	radius += r_imperatives[imperative].radius;
	radius += r_types[type].radius;
	switch (r_types[type].flag) {
		case T_SELF:
		radius += (skill - level) / (50 / 5);
		if (radius > S_RADIUS_MAX) radius = S_RADIUS_MAX;
		if (radius < S_RADIUS_MIN) radius = S_RADIUS_MIN;
		break;
		case T_BALL:
		radius += (skill - level) / (50 / 5);
		if (radius > S_RADIUS_MAX) radius = S_RADIUS_MAX;
		if (radius < S_RADIUS_MIN) radius = S_RADIUS_MIN;
		break;
		case T_CLOU:
		radius += (skill - level) / (50 / 4);
		if (radius > S_RADIUS_MAX - 1) radius = S_RADIUS_MAX - 1;
		if (radius < S_RADIUS_MIN) radius = S_RADIUS_MIN;
		break;
		case T_STOR:
		radius += (skill - level) / (50 / 3);
		if (radius > S_RADIUS_MAX - 2) radius = S_RADIUS_MAX - 2;
		if (radius < S_RADIUS_MIN) radius = S_RADIUS_MIN;
		break;
		default:
		radius = 0;
	}
	return (byte)radius;
}

byte rspell_duration(u32b Ind, byte element[], byte elements, byte imperative, byte type, byte skill, byte level) {
//unused?	player_type *p_ptr = Players[Ind];
	s16b duration = 0;
	switch (r_types[type].flag) {
		case T_SELF:
		duration = 4 + randint(10) + rget_level(40);
		break;
		case T_CLOU:
		duration = 6 + randint(4) + rget_level(10);
		break;
		case T_WAVE:
		duration = 4 + rget_level(10) / 3;
		break;
		case T_STOR:
		duration = 6 + randint(4) + rget_level(15);
		break;
	}
	
#ifdef ENABLE_AUGMENTS
	if (p_ptr->rcraft_augment == -1) {
		byte i;
		for (i = 0; i < elements; i++) {
			if (r_elements[element[i]].duration > 10) duration = duration * (10 + (r_elements[element[i]].duration - 10) / elements) / 10;
			else duration = duration * (10 - (10 - r_elements[element[i]].duration) / elements) / 10;
		}
	}
	else duration = duration * r_elements[p_ptr->rcraft_augment].duration / 10;
#endif	

	duration = duration * r_imperatives[imperative].duration / 10;
	duration = duration * r_types[type].duration / 10;
	
	/* Hard limit */
	if (duration > S_DURATION_MAX) duration = S_DURATION_MAX;
	if (duration < S_DURATION_MIN) duration = S_DURATION_MIN;
	
	return (byte)duration;
}

void rspell_do_penalty(u32b Ind, u32b gf_type, u32b damage, byte cost, byte p_flags, s16b link) {
	player_type *p_ptr = Players[Ind];
	/* Roll for effects */
	byte chance = randint(20);
	
	/* Remain 0 if 'unprotected' */
	int amt = 0;

	if (p_flags & RPEN_MIN_RN) {
		/* Did we have a rune to destroy? */
		if (link >= 0) {
			/* Destroy the linked rune */
			amt = 1;
			char o_name[ONAME_LEN];
			object_type *o_ptr;
			o_ptr = &p_ptr->inventory[link];
			object_desc(Ind, o_name, o_ptr, FALSE, 3);
			msg_format(Ind, "\377o%sour %s (%c) %s destroyed!",
				((o_ptr->number > 1) ? ((amt == o_ptr->number) ? "All of y" :
				(amt > 1 ? "Some of y" : "One of y")) : "Y"),
				o_name, index_to_label(link), ((amt > 1) ? "were" : "was"));

			/* Erase a rune from inventory */
			inven_item_increase(Ind, link, -1);

			/* Clean up inventory */
			inven_item_optimize(Ind, link);
		}
		else p_flags |= RPEN_MIN_HP;
	}

	if (p_flags & RPEN_MIN_SP) {
		msg_print(Ind, "\377rYou feel drained.");
		
		/* Status Effect */
		if (chance == 20 && !p_ptr->free_act) set_paralyzed(Ind, 2);
		else if (chance >= 15) set_stun(Ind, 10);
		else if (chance >= 10) set_slow(Ind, 10);
		
		/* SP Hit */
		if (p_ptr->csp > cost) p_ptr->csp -= (randint(cost));
		else {
			take_hit(Ind, cost - p_ptr->csp, "magical exhaustion", 0);
			p_ptr->csp = 0;
			p_flags |= RPEN_MIN_HP;
		}	
	}
		
	if (p_flags & RPEN_MIN_HP) {
		msg_print(Ind, "\377rThe runespell lashes out at you!");
		
		/* Status Effect */
		if (chance == 20 && !p_ptr->resist_blind && !p_ptr->resist_lite) set_blind(Ind, 10);
		else if (chance > 15 && !p_ptr->resist_pois) set_poisoned(Ind, 10, Ind);
		else if (chance > 10) set_cut(Ind, 10, Ind);
		
		/* HP Hit */
		/* Hack -- Decode gf_type */
		if (gf_type > HACK_GF_FACTOR) {
			u32b gf_main = gf_type % HACK_GF_FACTOR;
			u32b gf_off = gf_type / HACK_GF_FACTOR;
			u32b damage_main = damage % HACK_DAM_FACTOR;
			u32b damage_off = damage / HACK_DAM_FACTOR;
			if (gf_main != GF_AWAY_ALL) project(PROJECTOR_RUNE, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px, damage_main / 5 + 1, gf_main, PROJECT_KILL | PROJECT_NORF | PROJECT_JUMP, "");
			if (gf_off != GF_AWAY_ALL) project(PROJECTOR_RUNE, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px, damage_off / 5 + 1, gf_off, PROJECT_KILL | PROJECT_NORF | PROJECT_JUMP, "");
		}
		else if (gf_type != GF_AWAY_ALL) project(PROJECTOR_RUNE, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px, damage / 5 + 1, gf_type, PROJECT_KILL | PROJECT_NORF | PROJECT_JUMP, "");
	}
		
	if (p_flags & RPEN_MIN_ST) {
		msg_print(Ind, "\377rYou feel less potent.");

		/* ST Hit */
		byte mode = 0;
		if ((chance == 20) && (amt == 0)) mode = STAT_DEC_NORMAL;
		else mode = STAT_DEC_TEMPORARY;
		do_dec_stat(Ind, randint(6) - 1, mode);
	}
	
	if ((p_flags & RPEN_MAJ_SN) && (amt == 0)) {
		msg_print(Ind, "\377rYou feel less sane!");

		/* Status Effect(s) */
		if (chance == 20) set_image(Ind, 10);
		if (chance > 15 && !p_ptr->resist_fear) set_afraid(Ind, 10);
		if (chance > 10 && !p_ptr->resist_conf) set_confused(Ind, 10);
		
		/* SN Hit */
		take_sanity_hit(Ind, damroll(1, p_ptr->msane / 10 / chance + 1), "a malformed invocation");
	}
	
	if ((p_flags & RPEN_MAJ_ST) && (amt == 0)) {
		msg_format(Ind, "\377rThe invocation ruins you!");
		
		/* Ruination */
		byte i;
		for (i = 0; i < 6; i++) {
			(void)dec_stat(Ind, i, 25, STAT_DEC_NORMAL);
		}
	}
	
	if ((p_flags & RPEN_MAJ_HP) && (amt == 0)) {
		msg_format(Ind, "\377rYour grasp on the invocation slips!");
		
		/* HP Hit */
		/* Hack -- Decode gf_type */
		if (gf_type > HACK_GF_FACTOR) {
			u32b gf_main = gf_type % HACK_GF_FACTOR;
			u32b gf_off = gf_type / HACK_GF_FACTOR;
			u32b damage_main = damage % HACK_DAM_FACTOR;
			u32b damage_off = damage / HACK_DAM_FACTOR;
			if (gf_main != GF_AWAY_ALL) project(PROJECTOR_RUNE, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px, damage_main / 2 + 1, gf_main, PROJECT_KILL | PROJECT_NORF | PROJECT_JUMP, "");
			if (gf_off != GF_AWAY_ALL) project(PROJECTOR_RUNE, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px, damage_off / 2 + 1, gf_off, PROJECT_KILL | PROJECT_NORF | PROJECT_JUMP, "");
		}
		else if (gf_type != GF_AWAY_ALL) project(PROJECTOR_RUNE, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px, damage / 2 + 1, gf_type, PROJECT_KILL | PROJECT_NORF | PROJECT_JUMP, "");
	}
	
	if ((p_flags & RPEN_MAJ_DT) && (amt == 0)) {
		msg_format(Ind, "\377rYou lose control of the invocation!");
		
		/* Effect */
		set_paralyzed(Ind, 1);
		
		/* HP Hit */
		/* Hack -- Decode gf_type */
		if (gf_type > HACK_GF_FACTOR) {
			u32b gf_main = gf_type % HACK_GF_FACTOR;
			u32b gf_off = gf_type / HACK_GF_FACTOR;
			u32b damage_main = damage % HACK_DAM_FACTOR;
			u32b damage_off = damage / HACK_DAM_FACTOR;
			if (gf_main != GF_AWAY_ALL) project(PROJECTOR_RUNE, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px, damage_main, gf_main, PROJECT_KILL | PROJECT_NORF | PROJECT_JUMP, "");
			if (gf_off != GF_AWAY_ALL) project(PROJECTOR_RUNE, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px, damage_off, gf_off, PROJECT_KILL | PROJECT_NORF | PROJECT_JUMP, "");
		}
		else if (gf_type != GF_AWAY_ALL) project(PROJECTOR_RUNE, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px, damage, gf_type, PROJECT_KILL | PROJECT_NORF | PROJECT_JUMP, "");
	}

	/* Update the Player */
	p_ptr->redraw |= PR_HP;
	p_ptr->redraw |= PR_MANA;
	p_ptr->redraw |= PR_SPEED;
	p_ptr->redraw |= PR_SANITY;
	handle_stuff(Ind);
	
	return;
}

#ifdef CONSUME_RUNES
bool rspell_socket(u32b Ind, byte rune) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	byte i;
	for (i = 0; i < INVEN_TOTAL; i++) {
		if (i >= INVEN_PACK) continue;		
		o_ptr = &p_ptr->inventory[i];
		if ((o_ptr->tval == TV_RUNE2)) { // && (o_ptr->level || o_ptr->owner == p_ptr->id)
			if (o_ptr->sval == rune) {
				byte amt = 1; //Always socket one?
				char o_name[ONAME_LEN];
				object_type *o_ptr;
				o_ptr = &p_ptr->inventory[i];
				object_desc(Ind, o_name, o_ptr, FALSE, 3);
				msg_format(Ind, "\377y%sour %s (%c) %s consumed!",
					((o_ptr->number > 1) ? ((amt == o_ptr->number) ? "All of y" :
					(amt > 1 ? "Some of y" : "One of y")) : "Y"),
					o_name, index_to_label(i), ((amt > 1) ? "were" : "was"));

				/* Erase a rune from inventory */
				inven_item_increase(Ind, i, -1);

				/* Clean up inventory */
				inven_item_optimize(Ind, i);
				
				return TRUE;
			}
		}
	}
	return FALSE;
}
#endif

byte execute_rspell(u32b Ind, byte dir, u16b e_flags1, u16b e_flags2, u16b m_flags, bool retaliate) {
	player_type * p_ptr = Players[Ind];
	
	/* Toggle AFK */
	un_afk_idle(Ind);
	
	/* Paranoia; toggle states that require inactivity */
	break_cloaking(Ind, 5);
	break_shadow_running(Ind);
	stop_precision(Ind);
	
	/* Assume we fail and exit FTK */
	p_ptr->shooting_till_kill = FALSE;
	
	/* Decode the Elements */
	u16b e_flags = 0; e_flags |= e_flags1; e_flags |= e_flags2;
	byte element[RSPELL_MAX_ELEMENTS];
	byte elements = flags_to_elements(element, e_flags);
	
	/** Validate Spell **/
	
#ifdef LIMIT_NON_RUNEMASTERS
	if (elements > 2 && p_ptr->pclass != CLASS_RUNEMASTER) {
		msg_print(Ind, "\377rYou are not adept enough to combine more than two elements.");
		p_ptr->energy -= level_speed(&p_ptr->wpos);
		return 0;
	}
#endif

	byte skill = rspell_skill(Ind, element, elements);

	/* Decode the Imperative and Type */
	byte imperative = flags_to_imperative(m_flags);
	byte type = flags_to_type(m_flags);
	
#ifdef RCRAFT_DEBUG
s_printf("RCRAFT_DEBUG: Runespell attempted... \n");
byte ii;
for (ii = 0; ii < elements; ii++) {
s_printf("Rune %d: %s\n", ii, r_elements[element[ii]].name);
}
s_printf("Imperative: %s\n", r_imperatives[imperative].name);
s_printf("Type: %s\n", r_types[type].name);
#endif

	byte level = rspell_level(element, elements, imperative, type);
	
	/* Force a sane level range for spell access */
	s16b diff = rspell_diff(skill, level);
	if (diff < -S_DIFF_MAX) {
		msg_print(Ind, "\377rYou are not skillful enough to combine these elements.");
		p_ptr->energy -= level_speed(&p_ptr->wpos);
		return 0;
	}
	
	/* Handle '+' targetting mode */
	if (dir == 11) {
		get_aim_dir(Ind);
		p_ptr->current_rcraft = 1;
		p_ptr->current_rcraft_e_flags1 = e_flags1;
		p_ptr->current_rcraft_e_flags2 = e_flags2;
		p_ptr->current_rcraft_m_flags = m_flags;
		return 1;
	}
	p_ptr->current_rcraft = -1;
	
	s16b energy = rspell_energy(Ind, element, elements, imperative, type);
	
	/* Handle FTK targetting mode */
	if (p_ptr->shoot_till_kill) {
		p_ptr->shoot_till_kill_rcraft = FALSE;
		/* If the spell targets and is instant */
		if ((r_types[type].flag == T_BOLT) || (r_types[type].flag == T_BEAM) || (r_types[type].flag == T_BALL)) {
			/* If a new spell, update FTK */
			if (p_ptr->FTK_e_flags1 != e_flags1 || p_ptr->FTK_e_flags2 != e_flags2 || p_ptr->FTK_m_flags != m_flags) {
				p_ptr->FTK_e_flags1 = e_flags1;
				p_ptr->FTK_e_flags2 = e_flags2;
				p_ptr->FTK_m_flags = m_flags;
				p_ptr->FTK_energy = energy;
			}
			
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

			/* Continue casting if everything up to this point was a success */
			p_ptr->shooting_till_kill = TRUE;
			p_ptr->shoot_till_kill_rcraft = TRUE;
		}
	}
	
	/** Validate Caster **/
	
	/* Examine Status */
	u16b penalty = 0;
	
#ifdef FEEDBACK_MESSAGE
	/* Prepare a feedback message */
	char * msg_1 = NULL;
	char * msg_2 = NULL;
	char * msg_3 = NULL;
#endif

	/* Blind */
	if (p_ptr->blind) {
#ifdef ENABLE_BLIND_CASTING
		penalty += 10;
 #ifdef FEEDBACK_MESSAGE
		msg_2 = " struggle to";
 #endif
#else
		msg_print(Ind, "You cannot see!");
		p_ptr->energy -= energy;
		return 0;
#endif
	}
	
	/* Confused */
	if (p_ptr->confused) {
#ifdef	ENABLE_CONFUSED_CASTING
		penalty += 10;
 #ifdef FEEDBACK_MESSAGE
		msg_2 = " struggle to";
 #endif
#else
		msg_print(Ind, "You are too confused!");
		p_ptr->energy -= energy;
		return 0;
#endif
	}
	
	/* AM-Shell */
	if (p_ptr->anti_magic) {
#ifdef ENABLE_SHELL_PROJECTIONS
		if (r_types[type].flag != T_SELF) {
			msg_format(Ind, "\377%cYour anti-magic shell absorbs the spell.", COLOUR_AM_OWN);
			p_ptr->energy -= energy;
			return 0;
		}
#else
		msg_format(Ind, "\377%cYour anti-magic shell absorbs the spell.", COLOUR_AM_OWN);
		p_ptr->energy -= energy;
		return 0;
#endif
	}
	
	/* AM-Field */
	if (check_antimagic(Ind, 100)) {
		p_ptr->energy -= energy;
		return 0;
	}
	
	/* Interception */
	if ((r_types[type].flag != T_MELE) && (interfere(Ind, cfg.spell_interfere))) {
		p_ptr->energy -= energy;
		return 0;
	}
	
	/* Examine Inventory */
	u16b mali = S_FAIL_RUNE * elements;
	s16b link = rspell_inventory(Ind, element, elements, &mali);
	
	/* Is it a R_NEXU or R_TIME shifted spell? */
	if (elements > 2 && (e_flags1 & R_NEXU)) {
		if (((e_flags - R_NEXU) != (R_ACID | R_ELEC)) && ((e_flags - R_NEXU) != (R_FIRE | R_COLD))) {
			switch (e_flags1 - R_NEXU) {
				case R_ACID: {
					e_flags1 = e_flags - (R_ACID | R_NEXU) + R_ELEC;
					e_flags2 = R_ELEC;
					e_flags = e_flags1;
					elements = flags_to_elements(element, e_flags);
				break; }
				case R_ELEC: {
					e_flags1 = e_flags - (R_ELEC | R_NEXU) + R_ACID;
					e_flags2 = R_ACID;
					e_flags = e_flags1;
					elements = flags_to_elements(element, e_flags);
				break; }
				case R_FIRE: {
					e_flags1 = e_flags - (R_FIRE | R_NEXU) + R_COLD;
					e_flags2 = R_COLD;
					e_flags = e_flags1;
					elements = flags_to_elements(element, e_flags);
				break; }
				case R_COLD: {
					e_flags1 = e_flags - (R_COLD | R_NEXU) + R_FIRE;
					e_flags2 = R_FIRE;
					e_flags = e_flags1;
					elements = flags_to_elements(element, e_flags);
				break; }
			}
		}
	}
	
#ifdef ENABLE_GROUP_SPELLS
	s16b rune[RSPELL_MAX_ELEMENTS - 1];
	if (is_group_spell(rune, e_flags1, e_flags2, elements)) {
#ifdef RCRAFT_DEBUG
s_printf("RCRAFT_DEBUG: -is group spell- rune[0]: %d rune[1]: %d.\n", rune[0], rune[1]);
#endif
		byte runecharges = elements - 1;
		
		/* Reset the elements */
		e_flags = 0; e_flags1 = 0; e_flags2 = 0;
		
		if (p_ptr->tim_rcraft_xtra) {
			if (p_ptr->rcraft_xtra_a >= 0 && p_ptr->rcraft_xtra_a < RCRAFT_MAX_ELEMENTS) {
				/* Encode the first rune */
				e_flags1 |= r_elements[p_ptr->rcraft_xtra_a].flag;
				runecharges++;
				if (p_ptr->rcraft_xtra_b >= 0 && p_ptr->rcraft_xtra_b < RCRAFT_MAX_ELEMENTS) {
					/* Encode the second rune */
					e_flags1 |= r_elements[p_ptr->rcraft_xtra_b].flag;
					e_flags2 |= r_elements[p_ptr->rcraft_xtra_b].flag;
					runecharges++;
					/* Encode the third rune; check for overlap */
					if (rune[0] >= 0 && rune[0] < RCRAFT_MAX_ELEMENTS) {
						if ((r_elements[rune[0]].flag & e_flags1) == r_elements[rune[0]].flag) {
							runecharges--;
							if (rune[1] >= 0 && rune[1] < RCRAFT_MAX_ELEMENTS) {
								if ((r_elements[rune[1]].flag & e_flags1) == r_elements[rune[1]].flag) runecharges--;
								else e_flags2 |= r_elements[rune[1]].flag;
							}
						}
						else e_flags2 |= r_elements[rune[0]].flag;
					}
				}
				else {
					/* Encode the second rune; check for overlap */
					if (rune[0] >= 0 && rune[0] < RCRAFT_MAX_ELEMENTS) {
						if ((r_elements[rune[0]].flag & e_flags1) == r_elements[rune[0]].flag) {
							runecharges--;
							if (rune[1] >= 0 && rune[1] < RCRAFT_MAX_ELEMENTS) {
								if ((r_elements[rune[1]].flag & e_flags1) == r_elements[rune[1]].flag) runecharges--;
								else {
									e_flags1 |= r_elements[rune[1]].flag;
									e_flags2 |= r_elements[rune[1]].flag;
								}
							}
						}
						else {
							e_flags1 |= r_elements[rune[0]].flag;
							e_flags2 |= r_elements[rune[0]].flag;
							/* Encode the third rune; check for overlap */
							if (rune[1] >= 0 && rune[1] < RCRAFT_MAX_ELEMENTS) {
								if ((r_elements[rune[1]].flag & e_flags1) == r_elements[rune[1]].flag) runecharges--;
								else e_flags2 |= r_elements[rune[1]].flag;
							}
						}
					}
				}
			}
			/* Dispel the charges */
			set_tim_rcraft_xtra(Ind, 0);
		}
		else {
			if (rune[0] >= 0 && rune[0] < RCRAFT_MAX_ELEMENTS) e_flags1 |= r_elements[rune[0]].flag;
			if (rune[1] >= 0 && rune[1] < RCRAFT_MAX_ELEMENTS) {
				e_flags1 |= r_elements[rune[1]].flag;
				e_flags2 |= r_elements[rune[1]].flag;
			}
		}
		
		/* Sanity check */
		if (runecharges > RSPELL_MAX_ELEMENTS) {
			msg_print(Ind, "\377rYou cannot combine so many elements.");
			p_ptr->energy -= energy;
			return 0;
		}
		
		/* Decode the Elements */
		e_flags |= e_flags1; e_flags |= e_flags2;
		elements = flags_to_elements(element, e_flags);

#ifdef RCRAFT_DEBUG
s_printf("RCRAFT_DEBUG: Group Runespell attempted... \n");
byte ii;
for (ii = 0; ii < elements; ii++) {
s_printf("Rune %d: %s\n", ii, r_elements[element[ii]].name);
}
#endif

	}
#endif
	
	byte cost = 0;
	if (r_imperatives[imperative].flag != I_CHAO) cost = rspell_cost(Ind, element, elements, imperative, type, skill, level);
	else cost = rspell_cost(Ind, element, elements, randint(RCRAFT_MAX_IMPERATIVES) - 1, type, skill, level);
	
	/* Not enough MP */
	if (p_ptr->csp < cost) {
#ifdef ENABLE_LIFE_CASTING
 #ifdef SAFE_RETALIATION
		if (retaliate) return 2;
 #endif
		penalty += (cost - p_ptr->csp);
 #ifdef FEEDBACK_MESSAGE
		msg_1 = "\377rExhausted\377s, you";
 #endif
#else
		msg_print(Ind, "You do not have enough mana.");
		p_ptr->energy -= energy;
		return 0;
#endif
	}
	else {
#ifdef FEEDBACK_MESSAGE
		msg_1 = "\377sYou";
#endif
	}
	
	/** Attempt the Runespell **/
	
	/* Stunned */
	if (p_ptr->stun > 50) {
		penalty += 25;
#ifdef FEEDBACK_MESSAGE
		msg_2 = " struggle to";
#endif
	}
	else if (p_ptr->stun) {
		penalty += 15;
#ifdef FEEDBACK_MESSAGE
		msg_2 = " struggle to";
#endif
	}

#ifdef FEEDBACK_MESSAGE
	if (mali > 0) msg_3 = " draw the rune-forms from memory.";
	else msg_3 = " trace the rune-forms.";
#endif

	mali += penalty;
	
#ifdef FEEDBACK_MESSAGE
	/* Resolve the Feedback Message */
	if (mali) msg_format(Ind, "%s%s%s", msg_1, msg_2, msg_3);
#endif	
	
	byte fail = 0;
	if (r_imperatives[imperative].flag != I_CHAO) fail = rspell_fail(Ind, element, elements, imperative, type, diff, mali);
	else fail = rspell_fail(Ind, element, elements, randint(RCRAFT_MAX_IMPERATIVES) - 1, type, diff, mali);
	
	/* Choose the Projection Combinations */
	byte projection = flags_to_projection(e_flags1);
	byte explosion = -1;
	if (elements > 2) explosion = flags_to_projection(e_flags2);
	
	//Hack - Flux -> Plasma
	if (p_ptr->rcraft_empower && projection == 34) projection = RCRAFT_MAX_PROJECTIONS;
	if (p_ptr->rcraft_empower && explosion == 34) explosion = RCRAFT_MAX_PROJECTIONS;

	u32b damage = 0;
	u32b damage_off = 0;
	if (r_imperatives[imperative].flag != I_CHAO) {
		damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection);
		if (elements > 2) damage_off = rspell_damage(Ind, element, elements, imperative, 5, skill, explosion); //use T_BALL aka '5' - Kurzel
	}
	else {
		damage = rspell_damage(Ind, element, elements, randint(RCRAFT_MAX_IMPERATIVES) - 1, type, skill, projection);
		if (elements > 2) damage_off = rspell_damage(Ind, element, elements, randint(RCRAFT_MAX_IMPERATIVES) - 1, 5, skill, explosion); //use T_BALL aka '5' - Kurzel
	}
	
	/* Calculate the remaining parameters */
	byte radius = 0;
	byte duration = 0;
	if (r_imperatives[imperative].flag != I_CHAO) {
		radius = rspell_radius(Ind, element, elements, imperative, type, skill, level);
		duration = rspell_duration(Ind, element, elements, imperative, type, skill, level);
	}
	else { //If 'Chaotic' imperative, choose randomly from each imperative modifier, except energy, which would mess with FTK.
		radius = rspell_radius(Ind, element, elements, randint(RCRAFT_MAX_IMPERATIVES) - 1, type, skill, level);
		duration = rspell_duration(Ind, element, elements, randint(RCRAFT_MAX_IMPERATIVES) - 1, type, skill, level);
	}
	
	/** Cast the Runespell **/
	
	/* Determine quality of the cast and penalty; preserve hard fail at 0 - Kurzel */
	char * msg_q = NULL;
	byte p_flags = 0;
	s16b margin = randint(100) - fail;
	if (margin < -50) {
		msg_q = "\377rfail \377sto";
		rspell_penalty(margin, &p_flags);
	}
	else if (margin < 1) { //fail if margin is 0 or less
		msg_q = "fail to";
		rspell_penalty(margin, &p_flags);
	}
	else if (margin < 10) { 
		msg_q = "clumsily";
		//rspell_penalty(margin, &p_flags);
		damage = damage * 9 / 10 + 1;
		if (elements > 2) damage_off = damage_off * 9 / 10;
	}
	else if (margin < 30) {
		msg_q = "casually";
	}
	else if (margin < 50) {
		msg_q = "effectively";
		damage = damage * 11 / 10;
		if (elements > 2) damage_off = damage_off * 11 / 10;
	}
	else {
		msg_q = "elegantly";
		damage = damage * 12 / 10;
		if (elements > 2) damage_off = damage_off * 12 / 10;
	}
	
#ifdef RCRAFT_DEBUG
s_printf("RCRAFT_DEBUG: Runespell parameters: \n");
s_printf("Skill:    %d\n", skill);
s_printf("Level:    %d\n", level);
s_printf("Diff:     %d\n", diff);
s_printf("Energy:   %d\n", energy);
s_printf("Cost:     %d\n", cost);
s_printf("Fail:     %d\n", fail);
s_printf("Margin:   %d\n", margin);
s_printf("Damage:   %d\n", damage);
s_printf("DamageOf: %d\n", damage_off);
s_printf("Radius:   %d\n", radius);
s_printf("Duration: %d\n", duration);
#endif

	/* Resolve the Effect */
	u32b gf_type = r_projections[projection].gf_type;
	u32b gf_main = gf_type; //remember the main projection for quickness later
	
	/* Hack -- Encode an exploding projection */
	if (elements > 2) {
		gf_type += HACK_GF_FACTOR * r_projections[explosion].gf_type;
		gf_type += HACK_TYPE_FACTOR * r_types[type].flag;
		damage += HACK_DAM_FACTOR * damage_off;
	}

	switch (r_types[type].flag) {
		case T_MELE: {
		int px = p_ptr->px;
		int py = p_ptr->py;
		int tx, ty;
		
		/* Hack -- Limit range to 1 */
		if (dir == 5 && target_okay(Ind)) {
			tx = p_ptr->target_col;
			ty = p_ptr->target_row;
			
			/* .. for targetting use */
			if (tx - px > 1) tx = px + 1;
			if (px - tx > 1) tx = px - 1;
			if (ty - py > 1) ty = py + 1;
			if (py - ty > 1) ty = py - 1;

			msg_format(Ind, "You %s summon %s %s %s of %s%s.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, elements > 2 ? r_projections[explosion].adj : "", r_projections[projection].name);
		}
		else {
			/* .. for directional use */
			tx = px + ddx[dir];
			ty = py + ddy[dir];

			if (dir == 5) msg_format(Ind, "You %s summon %s %s %s of undirected %s%s.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, elements > 2 ? r_projections[explosion].adj : "", r_projections[projection].name);
			else msg_format(Ind, "You %s summon %s %s %s of directed %s%s.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, elements > 2 ? r_projections[explosion].adj : "", r_projections[projection].name);
		}
		
		if (margin > 0) {
			/* Emulate a fire_ball() call, so we can hack our (tx, ty) values in without modifying p_ptr->target_col/row - C. Blue */
#ifdef USE_SOUND_2010
			if (gf_main == GF_ROCKET) sound(Ind, "rocket", NULL, SFX_TYPE_COMMAND, FALSE);
			else if (gf_main == GF_DETONATION) sound(Ind, "detonation", NULL, SFX_TYPE_COMMAND, FALSE);
			else if (gf_main == GF_STONE_WALL) sound(Ind, "stone_wall", NULL, SFX_TYPE_COMMAND, FALSE);
			else sound(Ind, "cast_ball", NULL, SFX_TYPE_COMMAND, TRUE);
#endif
			sprintf(p_ptr->attacker, " %s summons %s %s %s of %s%s for", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, elements > 2 ? r_projections[explosion].adj : "", r_projections[projection].name);
			project(0 - Ind, 0, &p_ptr->wpos, ty, tx, damage, gf_type, PROJECT_NORF | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL, p_ptr->attacker);
		}
		break; }
		case T_BOLT: {
			msg_format(Ind, "You %s summon %s %s %s of %s%s.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, elements > 2 ? r_projections[explosion].adj : "", r_projections[projection].name);
			if ((gf_main == GF_KILL_WALL) || (gf_main == GF_EARTHQUAKE)) {
				int flg = PROJECT_NORF | PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
				if (margin > 0) {
					sprintf(p_ptr->attacker, " %s summons %s %s %s of %s%s for", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, elements > 2 ? r_projections[explosion].adj : "", r_projections[projection].name);
					project_hook(Ind, gf_type, dir, damage, flg, "");
				}
			}
			else {
				if (margin > 0) {
					sprintf(p_ptr->attacker, " %s summons %s %s %s of %s%s for", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, elements > 2 ? r_projections[explosion].adj : "", r_projections[projection].name);
					fire_bolt(Ind, gf_type, dir, damage, p_ptr->attacker);
				}
			}
		break; }
		case T_BEAM: {
			msg_format(Ind, "You %s summon %s %s %s of %s%s.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, elements > 2 ? r_projections[explosion].adj : "", r_projections[projection].name);
			if ((gf_main == GF_KILL_WALL) || (gf_main == GF_EARTHQUAKE)) {
				int flg = PROJECT_NORF | PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
				if (margin > 0) {
					sprintf(p_ptr->attacker, " %s summons %s %s %s of %s%s for", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, elements > 2 ? r_projections[explosion].adj : "", r_projections[projection].name);
					project_hook(Ind, gf_type, dir, damage, flg, "");
				}
			}
			else {
				if (margin > 0) {
					sprintf(p_ptr->attacker, " %s summons %s %s %s of %s%s for", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, elements > 2 ? r_projections[explosion].adj : "", r_projections[projection].name);
					if (gf_main == GF_LITE || gf_main == GF_DARK || gf_main == GF_ACID_DISARM || gf_main == GF_ELEC_DISARM || gf_main == GF_FIRE_DISARM || gf_main == GF_COLD_DISARM || gf_main == GF_EARTHQUAKE) {
						fire_grid_beam(Ind, gf_type, dir, damage, p_ptr->attacker);
					}
					else {
						fire_beam(Ind, gf_type, dir, damage, p_ptr->attacker);
					}
				}
			}
		break; }
		case T_BALL: {
			msg_format(Ind, "You %s summon %s %s %s of %s%s.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, elements > 2 ? r_projections[explosion].adj : "", r_projections[projection].name);
			if (margin > 0) {
				sprintf(p_ptr->attacker, " %s summons %s %s %s of %s%s for", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, elements > 2 ? r_projections[explosion].adj : "", r_projections[projection].name);
				fire_ball(Ind, gf_type, dir, damage, radius, p_ptr->attacker);
			}
		break; }
		case T_WAVE: {
			msg_format(Ind, "You %s summon %s %s %s of %s%s.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, elements > 2 ? r_projections[explosion].adj : "", r_projections[projection].name);
			if (margin > 0) {
				sprintf(p_ptr->attacker, " %s summons %s %s %s of %s%s for", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, elements > 2 ? r_projections[explosion].adj : "", r_projections[projection].name);
				fire_wave(Ind, gf_type, 0, damage, 0, duration, 2, EFF_WAVE, p_ptr->attacker);
			}
		break; }
		case T_CLOU: {
			msg_format(Ind, "You %s summon %s %s %s of %s%s.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, elements > 2 ? r_projections[explosion].adj : "", r_projections[projection].name);
			if (margin > 0) {
				sprintf(p_ptr->attacker, " %s summons %s %s %s of %s%s for", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, elements > 2 ? r_projections[explosion].adj : "", r_projections[projection].name);
				fire_cloud(Ind, gf_type, dir, damage, radius, duration, 9, p_ptr->attacker);
			}
		break; }
		case T_STOR: {
			msg_format(Ind, "You %s summon %s %s %s of %s%s.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, elements > 2 ? r_projections[explosion].adj : "", r_projections[projection].name);
			if (margin > 0) {
				sprintf(p_ptr->attacker, " %s summons %s %s %s of %s%s for", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, elements > 2 ? r_projections[explosion].adj : "", r_projections[projection].name);
				fire_wave(Ind, gf_type, 0, damage, radius, duration, 10, EFF_STORM, p_ptr->attacker);
			}
		break; }
		case T_SELF: {
		//Epic list of self spells here? Any undefined goes to a backlash penalty. - Kurzel
		switch (e_flags1) {
			/** 1-rune Self-Spells **/
			
			case (R_ACID): {
			msg_format(Ind, "You %s draw %s %s rune of acid resistance.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
			if (margin > 0) {
				set_oppose_acid(Ind, duration);
			}
			break; }
#ifdef ENABLE_AUGMENTS
			case (R_WATE): {
			if (p_ptr->rcraft_augment != 1) {
				msg_format(Ind, "You augment your spells with transmutation.");
				p_ptr->rcraft_augment = 1;
			}
			else {
				msg_format(Ind, "You stop augmenting your spells.");
				p_ptr->rcraft_augment = -1;
			}
			p_flags = 0;
			cost = 0;
			break; }
#endif	
			case (R_ELEC): {
			msg_format(Ind, "You %s draw %s %s rune of electricity resistance.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
			if (margin > 0) {
				set_oppose_elec(Ind, duration);
			}
			break; }
#ifdef ENABLE_AUGMENTS
			case (R_EART): {
			if (p_ptr->rcraft_augment != 3) {
				msg_format(Ind, "You augment your spells with conjuration.");
				p_ptr->rcraft_augment = 3;
			}
			else {
				msg_format(Ind, "You stop augmenting your spells.");
				p_ptr->rcraft_augment = -1;
			}
			p_flags = 0;
			cost = 0;
			break; }
#endif
			case (R_FIRE): {
			msg_format(Ind, "You %s draw %s %s rune of fire resistance.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
			if (margin > 0) {
				set_oppose_fire(Ind, duration);
			}
			break; }
#ifdef ENABLE_AUGMENTS
			case (R_CHAO): {
			if (p_ptr->rcraft_augment != 5) {
				msg_format(Ind, "You augment your spells with evocation.");
				p_ptr->rcraft_augment = 5;
			}
			else {
				msg_format(Ind, "You stop augmenting your spells.");
				p_ptr->rcraft_augment = -1;
			}
			p_flags = 0;
			cost = 0;
			break; }
#endif
			case (R_COLD): {
			msg_format(Ind, "You %s draw %s %s rune of cold resistance.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
			if (margin > 0) {
				set_oppose_cold(Ind, duration);
			}
			break; }
#ifdef ENABLE_AUGMENTS
			case (R_NETH): {
			if (p_ptr->rcraft_augment != 7) {
				msg_format(Ind, "You augment your spells with abjuration.");
				p_ptr->rcraft_augment = 7;
			}
			else {
				msg_format(Ind, "You stop augmenting your spells.");
				p_ptr->rcraft_augment = -1;
			}
			p_flags = 0;
			cost = 0;
			break; }
#endif
			case (R_POIS): {
			msg_format(Ind, "You %s draw %s %s rune of poison resistance.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
			if (margin > 0) {
				set_oppose_pois(Ind, duration);
			}
			break; }
#ifdef ENABLE_AUGMENTS	
			case (R_NEXU): {
			if (p_ptr->rcraft_augment != 9) {
				msg_format(Ind, "You augment your spells with illusion.");
				p_ptr->rcraft_augment = 9;
			}
			else {
				msg_format(Ind, "You stop augmenting your spells.");
				p_ptr->rcraft_augment = -1;
			}
			p_flags = 0;
			cost = 0;
			break; }
#endif
#ifdef ENABLE_AUGMENTS		
			case (R_FORC): {
			if (p_ptr->rcraft_augment != 10) {
				msg_format(Ind, "You augment your spells with compulsion.");
				p_ptr->rcraft_augment = 10;
			}
			else {
				msg_format(Ind, "You stop augmenting your spells.");
				p_ptr->rcraft_augment = -1;
			}
			p_flags = 0;
			cost = 0;	
			break; }
#endif
			case (R_TIME): { 
			if (p_ptr->spell_project) { // Don't allow both projection types. - Kurzel
				p_ptr->spell_project = 0;
				msg_format(Ind, "Your utility spells will now only affect yourself.");
			}
			if (p_ptr->rcraft_project) {
				msg_format(Ind, "You stop synchronizing your spells.");
				p_ptr->rcraft_project = 0;
				p_ptr->redraw |= PR_EXTRA;
			}
			else {
				msg_format(Ind, "You synchronize your spells with players in a radius of %d.", radius);
					p_ptr->rcraft_project = radius;
					p_ptr->redraw |= PR_EXTRA;
			}
			p_flags = 0;
			cost = 0;
			break; }
			
			/** R_ACID **/

			case (R_ACID | R_WATE): {
			msg_format(Ind, "You %s conjure %s %s matrix of spell energy.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
			type = 6; //T_CLOU
			duration = rspell_duration(Ind, element, elements, imperative, type, skill, level);
			switch (e_flags - (R_ACID | R_WATE)) {
				case 0: {
				if (margin > 0) {
					damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection);
					set_tim_rcraft_help(Ind, duration, type, projection, damage);
				}
				break; }
				
				default: {
				if (margin > 0) {
					projection = flags_to_projection(e_flags - R_WATE);
					damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection);
					set_tim_rcraft_help(Ind, duration, type, projection, damage);
				}
				break; }
			}
			break; }
			
			case (R_ACID | R_ELEC): {
			switch (e_flags - (R_ACID | R_ELEC)) {
				case R_FIRE: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_elec(Ind, duration);
					set_oppose_fire(Ind, duration);
				}
				break; }
				
				case R_COLD: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_elec(Ind, duration);
					set_oppose_cold(Ind, duration);
				}
				break; }
				
				case R_POIS: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_elec(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
				
				case R_FORC: {
				byte upkeep_rune[5];
				cost = 0;
				if (p_ptr->rcraft_attune == (R_ACID | R_ELEC)) {
					msg_format(Ind, "\377WYour armament is no longer attuned.");
					/* De-attune */
					p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					p_ptr->rcraft_attune = 0;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* De-attune */
						if (p_ptr->rcraft_attune) p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
						/* Can we sustain two more attunes? */
						if (p_ptr->rcraft_upkeep + (2 * UPKEEP_ATTUNE) > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Attune */
						msg_format(Ind, "You %s attune your armament.", msg_q);
						set_tim_brand_acid(Ind, 1);
						set_tim_brand_elec(Ind, 1);
						p_ptr->rcraft_attune = (R_ACID | R_ELEC);
						p_ptr->rcraft_upkeep += flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					}
					else msg_format(Ind, "You %s attune your armament.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
				
				case R_NEXU: {
				msg_format(Ind, "You %s project twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_elec(Ind, duration);
					fire_ball(Ind, GF_RESACID_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
					fire_ball(Ind, GF_RESELEC_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
				}
				break; }
				
				default: {
				msg_format(Ind, "You %s draw twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_elec(Ind, duration);
				}
				break; }
			}
			break; }
			
			case (R_ACID | R_EART): {
			switch (e_flags - (R_ACID | R_EART)) {
				case 0: {
				msg_format(Ind, "You %s corrode yourself.", msg_q);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
				
				case R_COLD: {
				msg_format(Ind, "You %s corrode and shatter yourself.", msg_q);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
				
				default: {
				msg_format(Ind, "You %s invoke %s %s malicious runespell.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
			}
			break; }

			case (R_ACID | R_FIRE): {
			switch (e_flags - (R_ACID | R_FIRE)) {
				case R_ELEC: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_elec(Ind, duration);
					set_oppose_fire(Ind, duration);
				}
				break; }
				
				case R_COLD: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_fire(Ind, duration);
					set_oppose_cold(Ind, duration);
				}
				break; }
				
				case R_POIS: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_fire(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
				
				case R_FORC: {
				byte upkeep_rune[5];
				cost = 0;
				if (p_ptr->rcraft_attune == (R_ACID | R_FIRE)) {
					msg_format(Ind, "\377WYour armament is no longer attuned.");
					/* De-attune */
					p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					p_ptr->rcraft_attune = 0;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* De-attune */
						if (p_ptr->rcraft_attune) p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
						/* Can we sustain two more attunes? */
						if (p_ptr->rcraft_upkeep + (2 * UPKEEP_ATTUNE) > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Attune */
						msg_format(Ind, "You %s attune your armament.", msg_q);
						set_tim_brand_acid(Ind, 1);
						set_tim_brand_fire(Ind, 1);
						p_ptr->rcraft_attune = (R_ACID | R_FIRE);
						p_ptr->rcraft_upkeep += flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					}
					else msg_format(Ind, "You %s attune your armament.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
				
				case R_NEXU: {
				msg_format(Ind, "You %s project twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_fire(Ind, duration);
					fire_ball(Ind, GF_RESACID_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
					fire_ball(Ind, GF_RESFIRE_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
				}
				break; }
				
				default: {
				msg_format(Ind, "You %s draw twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_fire(Ind, duration);
				}
				break; }
			}
			break; }

			case (R_ACID | R_CHAO): {
			switch (e_flags - (R_ACID | R_CHAO)) {
				case 0: {
				if (p_ptr->rcraft_brand == BRAND_CONF) {
					msg_format(Ind, "\377WYou are no longer branded.");
					p_ptr->rcraft_upkeep -= UPKEEP_BRAND;
					p_ptr->rcraft_brand = 0;
					p_flags = 0;
					cost = 0;
				}
				else {
					if (margin > 0) {
						/* De-attune */
						if (p_ptr->rcraft_brand) p_ptr->rcraft_upkeep -= UPKEEP_BRAND;
						/* Can we sustain a brand? */
						if (p_ptr->rcraft_upkeep + UPKEEP_BRAND > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Attune */
						msg_format(Ind, "You %s brand yourself.", msg_q);
						set_tim_brand_conf(Ind, 1);
						p_ptr->rcraft_brand = BRAND_CONF;
						p_ptr->rcraft_upkeep += UPKEEP_BRAND;
					}
					else msg_format(Ind, "You %s brand yourself.", msg_q);
				}
				calc_mana(Ind);
				break; }
				
				default: {
				projection = flags_to_projection(e_flags - R_CHAO);
				damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 10 + 1; //Severely reduced damage (per burst) - Kurzel
				msg_format(Ind, "You %s empower your defenses with %s %s.", msg_q, r_imperatives[imperative].name, r_projections[projection].name);
				if (margin > 0) set_tim_aura_ex(Ind, duration, projection, damage);
				break; }
			}
			break; }

			case (R_ACID | R_COLD): {
			switch (e_flags - (R_ACID | R_COLD)) {
				case R_ELEC: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_elec(Ind, duration);
					set_oppose_cold(Ind, duration);
				}
				break; }
				
				case R_FIRE: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_fire(Ind, duration);
					set_oppose_cold(Ind, duration);
				}
				break; }
				
				case R_POIS: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_cold(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
				
				case R_FORC: {
				byte upkeep_rune[5];
				cost = 0;
				if (p_ptr->rcraft_attune == (R_ACID | R_COLD)) {
					msg_format(Ind, "\377WYour armament is no longer attuned.");
					/* De-attune */
					p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					p_ptr->rcraft_attune = 0;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* De-attune */
						if (p_ptr->rcraft_attune) p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
						/* Can we sustain two more attunes? */
						if (p_ptr->rcraft_upkeep + (2 * UPKEEP_ATTUNE) > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Attune */
						msg_format(Ind, "You %s attune your armament.", msg_q);
						set_tim_brand_acid(Ind, 1);
						set_tim_brand_cold(Ind, 1);
						p_ptr->rcraft_attune = (R_ACID | R_COLD);
						p_ptr->rcraft_upkeep += flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					}
					else msg_format(Ind, "You %s attune your armament.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
				
				case R_NEXU: {
				msg_format(Ind, "You %s project twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_cold(Ind, duration);
					fire_ball(Ind, GF_RESACID_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
					fire_ball(Ind, GF_RESCOLD_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
				}
				break; }
				
				default: {
				msg_format(Ind, "You %s draw twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_cold(Ind, duration);
				}
				break; }
			}
			break; }
			
			case (R_ACID | R_NETH): {
			switch (e_flags - (R_ACID | R_NETH)) {
				case 0: {
				if (p_ptr->rcraft_brand == BRAND_VORP) {
					msg_format(Ind, "\377WYou are no longer branded.");
					p_ptr->rcraft_upkeep -= UPKEEP_BRAND;
					p_ptr->rcraft_brand = 0;
					p_flags = 0;
					cost = 0;
				}
				else {
					if (margin > 0) {
						/* De-attune */
						if (p_ptr->rcraft_brand) p_ptr->rcraft_upkeep -= UPKEEP_BRAND;
						/* Can we sustain a brand? */
						if (p_ptr->rcraft_upkeep + UPKEEP_BRAND > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Attune */
						msg_format(Ind, "You %s brand yourself.", msg_q);
						set_tim_brand_vorp(Ind, 1);
						p_ptr->rcraft_brand = BRAND_VORP;
						p_ptr->rcraft_upkeep += UPKEEP_BRAND;
					}
					else msg_format(Ind, "You %s brand yourself.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
				
				default: {
				msg_format(Ind, "You %s draw %s %s explosive rune.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) {
					projection = flags_to_projection(e_flags - R_NETH);
					if (set_rune_trap_okay(Ind)) set_rune_trap_aux(Ind, projection, imperative, skill);
				}
				cost = 0;
				break; }
			}
			break; }
			
			case (R_ACID | R_POIS): {
			switch (e_flags - (R_ACID | R_POIS)) {
				case R_ELEC: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_elec(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
				
				case R_FIRE: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_fire(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
				
				case R_COLD: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_cold(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
				
				case R_FORC: {
				byte upkeep_rune[5];
				cost = 0;
				if (p_ptr->rcraft_attune == (R_ACID | R_POIS)) {
					msg_format(Ind, "\377WYour armament is no longer attuned.");
					/* De-attune */
					p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					p_ptr->rcraft_attune = 0;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* De-attune */
						if (p_ptr->rcraft_attune) p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
						/* Can we sustain two more attunes? */
						if (p_ptr->rcraft_upkeep + (2 * UPKEEP_ATTUNE) > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Attune */
						msg_format(Ind, "You %s attune your armament.", msg_q);
						set_tim_brand_acid(Ind, 1);
						set_tim_brand_pois(Ind, 1);
						p_ptr->rcraft_attune = (R_ACID | R_POIS);
						p_ptr->rcraft_upkeep += flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					}
					else msg_format(Ind, "You %s attune your armament.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
				
				case R_NEXU: {
				msg_format(Ind, "You %s project twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_pois(Ind, duration);
					fire_ball(Ind, GF_RESACID_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
					fire_ball(Ind, GF_RESPOIS_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
				}
				break; }
				
				default: {
				msg_format(Ind, "You %s draw twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
			}
			break; }
			
			case (R_ACID | R_NEXU): {
			switch (e_flags - (R_ACID | R_NEXU)) {
				case 0: {
				msg_format(Ind, "You %s project %s %s rune of resistance.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_elec(Ind, duration);
					fire_ball(Ind, GF_RESELEC_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
				}
				break; }
				
				case R_ELEC: {
				cost = 0;
				if (p_ptr->rcraft_repel & R_ELEC) {
					/* De-repel */
					p_ptr->rcraft_upkeep -= UPKEEP_REPEL;
					p_ptr->rcraft_repel -= R_ELEC;
					if (p_ptr->rcraft_repel == 0) msg_format(Ind, "\377WYour raiment is no longer imbued.");
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* Can we sustain a repel? */
						if (p_ptr->rcraft_upkeep + UPKEEP_REPEL > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Repel */
						msg_format(Ind, "You %s imbue your raiment.", msg_q);
						set_oppose_elec(Ind, 1);
						set_tim_aura_acid(Ind, 1);
						p_ptr->rcraft_repel |= R_ELEC;
						p_ptr->rcraft_upkeep += UPKEEP_REPEL;
					}
					else msg_format(Ind, "You %s imbue your raiment.", msg_q);
				}
				calc_mana(Ind);
				break; }

				default: {
				//Do nothing, these are all reverting spells. - Kurzel
				break; }
			}
			break; }
			
			case (R_ACID | R_FORC): {
			switch (e_flags - (R_ACID | R_FORC)) {
				case 0: {
				byte upkeep_rune[5];
				cost = 0;
				if (p_ptr->rcraft_attune == R_ACID) {
					msg_format(Ind, "\377WYour armament is no longer attuned.");
					/* De-attune */
					p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					p_ptr->rcraft_attune = 0;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* De-attune */
						if (p_ptr->rcraft_attune) p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
						/* Can we sustain one more attune? */
						if (p_ptr->rcraft_upkeep + UPKEEP_ATTUNE > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Attune */
						msg_format(Ind, "You %s attune your armament.", msg_q);
						set_tim_brand_acid(Ind, 1);
						p_ptr->rcraft_attune = R_ACID;
						p_ptr->rcraft_upkeep += flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					}
					else msg_format(Ind, "You %s attune your armament.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
				
				default: {
				projection = flags_to_projection(e_flags - R_FORC);
				damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 10 + 1; //Severely reduced damage (per burst) - Kurzel
				msg_format(Ind, "You %s empower your attacks with %s %s.", msg_q, r_imperatives[imperative].name, r_projections[projection].name);
				if (margin > 0) set_tim_brand_ex(Ind, duration, projection, damage); 
				break; }
			}
			break; }
			
			case (R_ACID | R_TIME): {
			switch (e_flags - (R_ACID | R_TIME)) {
				default: {
				//Do nothing, these are all combining spells. - Kurzel
				break; }
			}
			break; }
			
			/** R_WATE **/
			
			case (R_WATE | R_ELEC): {
			switch (e_flags - (R_WATE | R_ELEC)) {
				case 0: {
				msg_format(Ind, "You %s discharge yourself.", msg_q);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
				
				case R_FIRE: {
				msg_format(Ind, "You %s discharge and evaporate yourself.", msg_q);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
				
				default: {
				msg_format(Ind, "You %s invoke %s %s malicious runespell.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
			}
			break; }
			
			case (R_WATE | R_EART): {
			switch (e_flags - (R_WATE | R_EART)) {
				case 0: {
				cost = 0;
				if (p_ptr->rcraft_dig) {
					msg_format(Ind, "\377WYour ability to tunnel returns to normal.");
					/* Remove upkeep */
					p_ptr->rcraft_upkeep -= UPKEEP_HI;
					p_ptr->rcraft_dig = 0;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* Remove upkeep */
						if (p_ptr->rcraft_dig) p_ptr->rcraft_upkeep -= UPKEEP_HI;
						/* Can we sustain? */
						if (p_ptr->rcraft_upkeep + UPKEEP_HI > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Add upkeep */
						msg_format(Ind, "You %s improve your ability to tunnel.", msg_q);
						p_ptr->rcraft_dig = rget_level(5) + 5;
						p_ptr->rcraft_upkeep += UPKEEP_HI;
					}
					else msg_format(Ind, "You %s improve your ability to tunnel.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
				
#ifdef ENABLE_RUNE_GOLEMS
//Golem stuff here! - Kurzel
#endif
				
				default: { //General backlash for mal effects; default for all unassigned effects. - Kurzel
				msg_format(Ind, "You %s invoke %s %s sealed runespell.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
			}
			break; }
			
			case (R_WATE | R_FIRE): {
			switch (e_flags - (R_WATE | R_FIRE)) {
				case 0: {
				msg_format(Ind, "You %s evaporate yourself.", msg_q);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
				
				case R_ELEC: {
				msg_format(Ind, "You %s discharge and evaporate yourself.", msg_q);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
				
				default: {
				msg_format(Ind, "You %s invoke %s %s malicious runespell.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
			}
			break; }
			
			case (R_WATE | R_CHAO): {
			switch (e_flags - (R_WATE | R_CHAO)) {
				case 0: {
				msg_format(Ind, "You %s polymorph yourself.", msg_q);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
			
				default: {
				msg_format(Ind, "You %s invoke %s %s malicious runespell.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
			}
			break; }
			
			case (R_WATE | R_COLD): {
			switch (e_flags - (R_WATE | R_COLD)) {
				case 0: {
				msg_format(Ind, "You %s draw %s %s explosive rune.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) if (set_rune_trap_okay(Ind)) set_rune_trap_aux(Ind, projection, imperative, skill);
				cost = 0;
				break; }

				default: {
				msg_format(Ind, "You %s draw %s %s explosive rune.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) {
					projection = flags_to_projection(e_flags - R_WATE);
					if (set_rune_trap_okay(Ind)) set_rune_trap_aux(Ind, projection, imperative, skill);
				}
				cost = 0;
				break; }
			}
			break; }
			
			case (R_WATE | R_NETH): {
			switch (e_flags - (R_WATE | R_NETH)) {
				case 0: {
				if (p_ptr->csp < cost || p_ptr->suscep_life) {
					msg_format(Ind, "You cannot heal yourself like this!");
					break;
				}
				else {
					msg_format(Ind, "You %s channel %s healing.", msg_q, r_imperatives[imperative].name);
					if (margin > 0) {
						damage = rget_level(400) * r_imperatives[imperative].damage;
						if (damage > 400) damage = 400;
						if (damage < 1) damage = 1;
						hp_player(Ind, damage);
					}
				}
				break; }
			
				default: {
				msg_format(Ind, "You %s invoke %s %s malicious runespell.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
			}
			break; }
			
			case (R_WATE | R_POIS): {
			switch (e_flags - (R_WATE | R_POIS)) {
				case 0: {
				cost = 0;
				if (p_ptr->rcraft_upkeep_flags & RUPK_MIN_FA) {
					msg_format(Ind, "\377WYou stop sealing the effects of paralysis.");
					/* Remove upkeep */
					p_ptr->rcraft_upkeep -= UPKEEP_MIN;
					p_ptr->rcraft_upkeep_flags -= RUPK_MIN_FA;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* Can we sustain? */
						if (p_ptr->rcraft_upkeep + UPKEEP_MIN > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Add upkeep */
						msg_format(Ind, "You %s seal the effects of paralysis.", msg_q);
						p_ptr->rcraft_upkeep_flags |= RUPK_MIN_FA;
						p_ptr->rcraft_upkeep += UPKEEP_MIN;
					}
					else msg_format(Ind, "You %s seal the effects of paralysis.", msg_q);
				}
				calc_mana(Ind);
				break; }
			
				default: {
				msg_format(Ind, "You %s draw %s %s explosive rune.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) {
					projection = flags_to_projection(e_flags - R_POIS);
					if (set_rune_trap_okay(Ind)) set_rune_trap_aux(Ind, projection, imperative, skill);
				}
				cost = 0;
				break; }
			}
			break; }
			
			case (R_WATE | R_NEXU): {
			switch (e_flags - (R_WATE | R_NEXU)) {
				case 0: {
				if (margin > 0)
				{
					if (target_okay(Ind) && !(distance(p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col) > MAX_RANGE)) {
						msg_format(Ind, "You %s teleport forward.", msg_q);
						teleport_player_to(Ind, p_ptr->target_row, p_ptr->target_col);
					}
					else msg_print(Ind, "\377oYou have no valid target!");	
				}
				else msg_format(Ind, "You %s teleport forward.", msg_q);
				break; }

				default: {
				projection = flags_to_projection(e_flags - R_NEXU);
				damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 10 + 1; //Severely reduced damage (per burst) - Kurzel
				msg_format(Ind, "You %s empower your defenses with %s %s.", msg_q, r_imperatives[imperative].name, r_projections[projection].name);
				if (margin > 0) set_tim_aura_ex(Ind, duration, projection, damage);
				break; }
			}
			break; }
			
			case (R_WATE | R_FORC): {
			switch (e_flags - (R_WATE | R_FORC)) {
				case 0: {
				cost = 0;
				if (p_ptr->rcraft_upkeep_flags & RUPK_MAJ_DO) {
					msg_format(Ind, "\377WYou release the pressure around you.");
					/* Remove upkeep */
					p_ptr->rcraft_upkeep -= UPKEEP_MIN;
					//p_ptr->rcraft_upkeep -= UPKEEP_MAJ;
					p_ptr->rcraft_upkeep_flags -= RUPK_MAJ_DO;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* Can we sustain? */
						if (p_ptr->rcraft_upkeep + UPKEEP_MAJ > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Add upkeep */
						msg_format(Ind, "You %s float in a bubble of pressure.", msg_q);
						//set_tim_dodge(Ind, 1, (rget_level(20) + 5) * 2); //High dodge bonus! - Kurzel!!
						p_ptr->rcraft_upkeep_flags |= RUPK_MAJ_DO;
						p_ptr->rcraft_upkeep += UPKEEP_MIN;
						//p_ptr->rcraft_upkeep += UPKEEP_MAJ;
					}
					else msg_format(Ind, "You %s focus on your reaction time.", msg_q);
				}
				calc_mana(Ind);
				break; }
				
				default: {
				projection = flags_to_projection(e_flags - R_FORC);
				damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 10 + 1; //Severely reduced damage (per burst) - Kurzel
				msg_format(Ind, "You %s empower your attacks with %s %s.", msg_q, r_imperatives[imperative].name, r_projections[projection].name);
				if (margin > 0) set_tim_brand_ex(Ind, duration, projection, damage); 
				break; }
			}
			break; }
			
			case (R_WATE | R_TIME): {
			switch (e_flags - (R_WATE | R_TIME)) {
				default: {
				//Do nothing, these are all combining spells. - Kurzel
				break; }
			}
			break; }
			
			/** R_ELEC **/
			
			case (R_ELEC | R_EART): {
			msg_format(Ind, "You %s conjure %s %s matrix of spell energy.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
			type = 2; //T_BOLT
			duration = rspell_duration(Ind, element, elements, imperative, 6, skill, level);//Duration as T_CLOU
			switch (e_flags - (R_ELEC | R_EART)) {
				case 0: {
				if (margin > 0) {
					damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 2 + 1;//T_CLOU damage / 2
					set_tim_rcraft_help(Ind, duration, type, projection, damage);
				}
				break; }
				
				default: {
				if (margin > 0) {
					projection = flags_to_projection(e_flags - R_EART);
					if (p_ptr->rcraft_empower && projection == 34) projection = RCRAFT_MAX_PROJECTIONS;//Hack - Flux -> Plasma
					damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 2 + 1;//T_CLOU damage / 2
					set_tim_rcraft_help(Ind, duration, type, projection, damage);
				}
				break; }
			}
			break; }
			
			case (R_ELEC | R_FIRE): {
			switch (e_flags - (R_ELEC | R_FIRE)) {
				case R_ACID: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_elec(Ind, duration);
					set_oppose_fire(Ind, duration);
				}
				break; }
				
				case R_WATE: {
				msg_format(Ind, "You %s discharge and evaporate yourself.", msg_q);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
				
				case R_EART: {
				if (margin > 0) {
					damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 2 + 1;//T_CLOU damage / 2
					set_tim_rcraft_help(Ind, duration, type, projection, damage);
				}
				break; }
				
				case R_CHAO: {
				msg_format(Ind, "You %s draw %s %s explosive rune.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) {
					if (set_rune_trap_okay(Ind)) set_rune_trap_aux(Ind, projection, imperative, skill);
				}
				cost = 0;
				break; }
				
				case R_COLD: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_elec(Ind, duration);
					set_oppose_fire(Ind, duration);
					set_oppose_cold(Ind, duration);
				}
				break; }
				
				
				
				case R_POIS: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_elec(Ind, duration);
					set_oppose_fire(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
				
				case R_FORC: {
				byte upkeep_rune[5];
				cost = 0;
				if (p_ptr->rcraft_attune == (R_ELEC | R_FIRE)) {
					msg_format(Ind, "\377WYour armament is no longer attuned.");
					/* De-attune */
					p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					p_ptr->rcraft_attune = 0;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* De-attune */
						if (p_ptr->rcraft_attune) p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
						/* Can we sustain two more attunes? */
						if (p_ptr->rcraft_upkeep + (2 * UPKEEP_ATTUNE) > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Attune */
						msg_format(Ind, "You %s attune your armament.", msg_q);
						set_tim_brand_elec(Ind, 1);
						set_tim_brand_fire(Ind, 1);
						p_ptr->rcraft_attune = (R_ELEC | R_FIRE);
						p_ptr->rcraft_upkeep += flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					}
					else msg_format(Ind, "You %s attune your armament.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
				
				case R_NEXU: {
				msg_format(Ind, "You %s project twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_elec(Ind, duration);
					set_oppose_fire(Ind, duration);
					fire_ball(Ind, GF_RESELEC_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
					fire_ball(Ind, GF_RESFIRE_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
				}
				break; }
				
				case R_TIME: {
				cost = 0;
				p_flags = 0;
				if (p_ptr->rcraft_empower) {
					msg_format(Ind, "\377WYour flux projections are no longer empowered.");
					p_ptr->rcraft_empower = 0;
				}
				else {
					msg_format(Ind, "\377WYour flux projections are empowered to \377Rplasma\377W.");
					p_ptr->rcraft_empower = 1;
				}
				break; }
				
				default: {
				msg_format(Ind, "You %s draw twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_elec(Ind, duration);
					set_oppose_fire(Ind, duration);
				}
				break; }
			}
			break; }
			
			case (R_ELEC | R_CHAO): {
			switch (e_flags - (R_ELEC | R_CHAO)) {
				case 0: {
				msg_format(Ind, "You %s call %s light.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					lite_area(Ind, damage, radius);
					lite_room(Ind, &p_ptr->wpos, p_ptr->py, p_ptr->px);
				}
				break; }
				
				default: {
				msg_format(Ind, "You %s draw %s %s explosive rune.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) {
					projection = flags_to_projection(e_flags - R_CHAO);
					if (p_ptr->rcraft_empower && projection == 34) projection = RCRAFT_MAX_PROJECTIONS;//Hack - Flux -> Plasma
					if (set_rune_trap_okay(Ind)) set_rune_trap_aux(Ind, projection, imperative, skill);
				}
				cost = 0;
				break; }
			}
			break; }
			
			case (R_ELEC | R_COLD): {
			switch (e_flags - (R_ELEC | R_COLD)) {
				case R_ACID: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_elec(Ind, duration);
					set_oppose_cold(Ind, duration);
				}
				break; }
				
				case R_FIRE: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_elec(Ind, duration);
					set_oppose_fire(Ind, duration);
					set_oppose_cold(Ind, duration);
				}
				break; }
				
				case R_POIS: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_elec(Ind, duration);
					set_oppose_cold(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
				
				case R_FORC: {
				byte upkeep_rune[5];
				cost = 0;
				if (p_ptr->rcraft_attune == (R_ELEC | R_COLD)) {
					msg_format(Ind, "\377WYour armament is no longer attuned.");
					/* De-attune */
					p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					p_ptr->rcraft_attune = 0;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* De-attune */
						if (p_ptr->rcraft_attune) p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
						/* Can we sustain two more attunes? */
						if (p_ptr->rcraft_upkeep + (2 * UPKEEP_ATTUNE) > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Attune */
						msg_format(Ind, "You %s attune your armament.", msg_q);
						set_tim_brand_elec(Ind, 1);
						set_tim_brand_cold(Ind, 1);
						p_ptr->rcraft_attune = (R_ELEC | R_COLD);
						p_ptr->rcraft_upkeep += flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					}
					else msg_format(Ind, "You %s attune your armament.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
				
				case R_NEXU: {
				msg_format(Ind, "You %s project twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_elec(Ind, duration);
					set_oppose_cold(Ind, duration);
					fire_ball(Ind, GF_RESELEC_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
					fire_ball(Ind, GF_RESCOLD_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
				}
				break; }
				
				default: {
				msg_format(Ind, "You %s draw twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_elec(Ind, duration);
					set_oppose_cold(Ind, duration);
				}
				break; }
			}
			break; }
			
			case (R_ELEC | R_NETH): {
			switch (e_flags - (R_ELEC | R_NETH)) {
				case 0: {
				msg_format(Ind, "You %s call %s darkness.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					unlite_area(Ind, damage, radius);
					unlite_room(Ind, &p_ptr->wpos, p_ptr->py, p_ptr->px);
				}
				break; }
				
				default: {
				projection = flags_to_projection(e_flags - R_NETH);
				if (p_ptr->rcraft_empower && projection == 34) projection = RCRAFT_MAX_PROJECTIONS;//Hack - Flux -> Plasma
				damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 10 + 1; //Severely reduced damage (per burst) - Kurzel
				msg_format(Ind, "You %s empower your defenses with %s %s.", msg_q, r_imperatives[imperative].name, r_projections[projection].name);
				if (margin > 0) set_tim_aura_ex(Ind, duration, projection, damage);
				break; }
			}
			break; }
			
			case (R_ELEC | R_POIS): {
			switch (e_flags - (R_ELEC | R_POIS)) {
				case R_ACID: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_elec(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
				
				case R_FIRE: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_elec(Ind, duration);
					set_oppose_fire(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
				
				case R_COLD: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_elec(Ind, duration);
					set_oppose_cold(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
				
				case R_FORC: {
				byte upkeep_rune[5];
				cost = 0;
				if (p_ptr->rcraft_attune == (R_ELEC | R_POIS)) {
					msg_format(Ind, "\377WYour armament is no longer attuned.");
					/* De-attune */
					p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					p_ptr->rcraft_attune = 0;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* De-attune */
						if (p_ptr->rcraft_attune) p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
						/* Can we sustain two more attunes? */
						if (p_ptr->rcraft_upkeep + (2 * UPKEEP_ATTUNE) > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Attune */
						msg_format(Ind, "You %s attune your armament.", msg_q);
						set_tim_brand_elec(Ind, 1);
						set_tim_brand_pois(Ind, 1);
						p_ptr->rcraft_attune = (R_ELEC | R_POIS);
						p_ptr->rcraft_upkeep += flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					}
					else msg_format(Ind, "You %s attune your armament.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
				
				case R_NEXU: {
				msg_format(Ind, "You %s project twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_elec(Ind, duration);
					set_oppose_pois(Ind, duration);
					fire_ball(Ind, GF_RESELEC_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
					fire_ball(Ind, GF_RESPOIS_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
				}
				break; }
				
				default: {
				msg_format(Ind, "You %s draw twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_elec(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
			}
			break; }
			
			case (R_ELEC | R_NEXU): {
			switch (e_flags - (R_ELEC | R_NEXU)) {
				case 0: {
				msg_format(Ind, "You %s project %s %s rune of resistance.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					fire_ball(Ind, GF_RESACID_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
				}
				break; }
				
				case R_ACID: {
				cost = 0;
				if (p_ptr->rcraft_repel & R_ACID) {
					/* De-repel */
					p_ptr->rcraft_upkeep -= UPKEEP_REPEL;
					p_ptr->rcraft_repel -= R_ACID;
					if (p_ptr->rcraft_repel == 0) msg_format(Ind, "\377WYour raiment is no longer imbued.");
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* Can we sustain a repel? */
						if (p_ptr->rcraft_upkeep + UPKEEP_REPEL > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Repel */
						msg_format(Ind, "You %s imbue your raiment.", msg_q);
						set_oppose_acid(Ind, 1);
						set_tim_aura_elec(Ind, 1);
						p_ptr->rcraft_repel |= R_ACID;
						p_ptr->rcraft_upkeep += UPKEEP_REPEL;
					}
					else msg_format(Ind, "You %s imbue your raiment.", msg_q);
				}
				calc_mana(Ind);
				break; }

				default: {
				//Do nothing, these are all reverting spells. - Kurzel
				break; }
			}
			break; }
			
			case (R_ELEC | R_FORC): {
			switch (e_flags - (R_ELEC | R_FORC)) {
				case 0: {
				byte upkeep_rune[5];
				cost = 0;
				if (p_ptr->rcraft_attune == R_ELEC) {
					msg_format(Ind, "\377WYour armament is no longer attuned.");
					/* De-attune */
					p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					p_ptr->rcraft_attune = 0;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* De-attune */
						if (p_ptr->rcraft_attune) p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
						/* Can we sustain one more attune? */
						if (p_ptr->rcraft_upkeep + UPKEEP_ATTUNE > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Attune */
						msg_format(Ind, "You %s attune your armament.", msg_q);
						set_tim_brand_elec(Ind, 1);
						p_ptr->rcraft_attune = R_ELEC;
						p_ptr->rcraft_upkeep += flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					}
					else msg_format(Ind, "You %s attune your armament.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
				
				default: {
				projection = flags_to_projection(e_flags - R_FORC);
				if (p_ptr->rcraft_empower && projection == 34) projection = RCRAFT_MAX_PROJECTIONS;//Hack - Flux -> Plasma
				damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 10 + 1; //Severely reduced damage (per burst) - Kurzel
				msg_format(Ind, "You %s empower your attacks with %s %s.", msg_q, r_imperatives[imperative].name, r_projections[projection].name);
				if (margin > 0) set_tim_brand_ex(Ind, duration, projection, damage); 
				break; }
			}
			break; }
			
			case (R_ELEC | R_TIME): {
			switch (e_flags - (R_ELEC | R_TIME)) {
				default: {
				//Do nothing, these are all combining spells. - Kurzel
				break; }
			}
			break; }
			
			/** R_EART **/
			
			case (R_EART | R_FIRE): {
			switch (e_flags - (R_EART | R_FIRE)) {
				case 0: {
				msg_format(Ind, "You %s draw %s %s explosive rune.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) if (set_rune_trap_okay(Ind)) set_rune_trap_aux(Ind, projection, imperative, skill);
				cost = 0;
				break; }
				
				default: {
				msg_format(Ind, "You %s draw %s %s explosive rune.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) {
					projection = flags_to_projection(e_flags - R_EART);
					if (set_rune_trap_okay(Ind)) set_rune_trap_aux(Ind, projection, imperative, skill);
				}
				cost = 0;
				break; }
			}
			break; }
			
			case (R_EART | R_CHAO): {
			switch (e_flags - (R_EART | R_CHAO)) {
				case 0: {
				msg_format(Ind, "You %s invoke %s %s earthquake.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) earthquake(&p_ptr->wpos, p_ptr->py, p_ptr->px, 5+radius);
				break; }
			
				default: {
				msg_format(Ind, "You %s invoke %s %s malicious runespell.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
			}
			break; }
			
			case (R_EART | R_COLD): {
			switch (e_flags - (R_EART | R_COLD)) {
				case 0: {
				msg_format(Ind, "You %s shatter yourself.", msg_q);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
				
				case R_ACID: {
				msg_format(Ind, "You %s corrode and shatter yourself.", msg_q);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
				
				default: {
				msg_format(Ind, "You %s invoke %s %s malicious runespell.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
			}
			break; }
			
			case (R_EART | R_NETH): {
			switch (e_flags - (R_EART | R_COLD)) {
				case 0: {
				msg_format(Ind, "You %s doom yourself.", msg_q);//doom instead of genocide, since we do nothing
				if (margin > 0) {
					msg_format(Ind, "You feel a doom upon you...");//Don't genocide the player? ^^' - Kurzel
				}
				break; }
				
				default: {
				msg_format(Ind, "You %s invoke %s %s malicious runespell.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
			}
			break; }
			
			case (R_EART | R_POIS): {
			switch (e_flags - (R_EART | R_POIS)) {
				case 0: {
				cost = 0;
				if (p_ptr->rcraft_upkeep_flags & RUPK_MIN_CU) {
					msg_format(Ind, "\377WYou stop sealing the effects of cuts.");
					/* Remove upkeep */
					p_ptr->rcraft_upkeep -= UPKEEP_MIN;
					p_ptr->rcraft_upkeep_flags -= RUPK_MIN_CU;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* Can we sustain? */
						if (p_ptr->rcraft_upkeep + UPKEEP_MIN > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Add upkeep */
						msg_format(Ind, "You %s seal the effects of cuts.", msg_q);
						p_ptr->rcraft_upkeep_flags |= RUPK_MIN_CU;
						p_ptr->rcraft_upkeep += UPKEEP_MIN;
					}
					else msg_format(Ind, "You %s seal the effects of cuts.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
			
				default: {
				msg_format(Ind, "You %s draw %s %s explosive rune.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) {
					projection = flags_to_projection(e_flags - R_POIS);
					if (set_rune_trap_okay(Ind)) set_rune_trap_aux(Ind, projection, imperative, skill);
				}
				cost = 0;
				break; }
			}
			break; }
			
			case (R_EART | R_NEXU): {
			switch (e_flags - (R_EART | R_NEXU)) {
				case 0: {
				msg_format(Ind, "You %s compel monsters to return.", msg_q);
				if (margin > 0) project_hack(Ind, GF_TELE_TO, 0, " summons"); 
				break; }
				
				default: {
				projection = flags_to_projection(e_flags - R_NEXU);
				damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 10 + 1; //Severely reduced damage (per burst) - Kurzel
				msg_format(Ind, "You %s empower your defenses with %s %s.", msg_q, r_imperatives[imperative].name, r_projections[projection].name);
				if (margin > 0) set_tim_aura_ex(Ind, duration, projection, damage);
				break; }
			}
			break; }
			
			case (R_EART | R_FORC): {
			switch (e_flags - (R_EART | R_FORC)) {
				case 0: {
				cost = 0;
				if (p_ptr->rcraft_upkeep_flags & RUPK_MAJ_ST) {
					msg_format(Ind, "\377WYou release your presence.");
					/* Remove upkeep */
					p_ptr->rcraft_upkeep -= UPKEEP_MAJ;
					p_ptr->rcraft_upkeep_flags -= RUPK_MAJ_ST;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* Can we sustain? */
						if (p_ptr->rcraft_upkeep + UPKEEP_MAJ > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Add upkeep */
						msg_format(Ind, "You %s restrain your presence.", msg_q);
						set_tim_stealth(Ind, 1, (rget_level(20) + 5) / 2); //Low stealth bonus! - Kurzel
						p_ptr->rcraft_upkeep_flags |= RUPK_MAJ_ST;
						p_ptr->rcraft_upkeep += UPKEEP_MAJ;
					}
					else msg_format(Ind, "You %s restrain your presence.", msg_q);
				}
				calc_mana(Ind);
				break; }
				
				default: {
				projection = flags_to_projection(e_flags - R_FORC);
				damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 10 + 1; //Severely reduced damage (per burst) - Kurzel
				msg_format(Ind, "You %s empower your attacks with %s %s.", msg_q, r_imperatives[imperative].name, r_projections[projection].name);
				if (margin > 0) set_tim_brand_ex(Ind, duration, projection, damage); 
				break; }
			}
			break; }
			
			case (R_EART | R_TIME): {
			switch (e_flags - (R_EART | R_TIME)) {
				default: {
				//Do nothing, these are all combining spells. - Kurzel
				break; }
			}
			break; }
			
			/** R_FIRE **/
			
			case (R_FIRE | R_CHAO): {
			switch (e_flags - (R_FIRE | R_CHAO)) {
				case 0: {
				cost = 0;
				if (p_ptr->rcraft_upkeep_flags & RUPK_MAJ_NE) {
					msg_format(Ind, "\377WYou no longer attract souls of the dying.");
					/* Remove upkeep */
					p_ptr->rcraft_upkeep -= UPKEEP_MAJ;
					p_ptr->rcraft_upkeep_flags -= RUPK_MAJ_NE;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* Can we sustain? */
						if (p_ptr->rcraft_upkeep + UPKEEP_MAJ > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Add upkeep */
						msg_format(Ind, "You %s attract souls of the dying.", msg_q);
						set_tim_necro(Ind, 1, (rget_level(20) + 5) * 1000);
						p_ptr->rcraft_upkeep_flags |= RUPK_MAJ_NE;
						p_ptr->rcraft_upkeep += UPKEEP_MAJ;
					}
					else msg_format(Ind, "You %s attract souls of the dying.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
				
				default: {
				/* Hack -- Increase Cost */
				cost *= 3; //Triple cost for LoS spells, enough? - Kurzel
				if (cost < S_COST_MIN) cost = S_COST_MIN;
				if (cost > S_COST_MAX) cost = S_COST_MAX;
				
				projection = flags_to_projection(e_flags - R_CHAO);
				if (p_ptr->rcraft_empower && projection == 34) projection = RCRAFT_MAX_PROJECTIONS;//Hack - Flux -> Plasma
				msg_format(Ind, "You %s fill the air with %s %s!", msg_q, r_imperatives[imperative].name, r_projections[projection].name);
				if (margin > 0) {
					damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 2 + 1;
					project_hack(Ind, r_projections[projection].gf_type, damage, p_ptr->attacker);
				}
				break; }
			}
			break; }
			
			case (R_FIRE | R_COLD): {
			switch (e_flags - (R_FIRE | R_COLD)) {
				case R_ACID: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_fire(Ind, duration);
					set_oppose_cold(Ind, duration);
				}
				break; }
				
				case R_ELEC: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_elec(Ind, duration);
					set_oppose_fire(Ind, duration);
					set_oppose_cold(Ind, duration);
				}
				break; }
				
				case R_POIS: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_fire(Ind, duration);
					set_oppose_cold(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
				
				case R_FORC: {
				byte upkeep_rune[5];
				cost = 0;
				if (p_ptr->rcraft_attune == (R_FIRE | R_COLD)) {
					msg_format(Ind, "\377WYour armament is no longer attuned.");
					/* De-attune */
					p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					p_ptr->rcraft_attune = 0;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* De-attune */
						if (p_ptr->rcraft_attune) p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
						/* Can we sustain two more attunes? */
						if (p_ptr->rcraft_upkeep + (2 * UPKEEP_ATTUNE) > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Attune */
						msg_format(Ind, "You %s attune your armament.", msg_q);
						set_tim_brand_fire(Ind, 1);
						set_tim_brand_cold(Ind, 1);
						p_ptr->rcraft_attune = (R_FIRE | R_COLD);
						p_ptr->rcraft_upkeep += flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					}
					else msg_format(Ind, "You %s attune your armament.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
				
				case R_NEXU: {
				msg_format(Ind, "You %s project twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_fire(Ind, duration);
					set_oppose_cold(Ind, duration);
					fire_ball(Ind, GF_RESFIRE_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
					fire_ball(Ind, GF_RESCOLD_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
				}
				break; }
				
				default: {
				msg_format(Ind, "You %s draw twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_fire(Ind, duration);
					set_oppose_cold(Ind, duration);
				}
				break; }
			}
			break; }
			
			case (R_FIRE | R_NETH): {
			switch (e_flags - (R_FIRE | R_NETH)) {
				case 0: {
				msg_format(Ind, "You %s fill yourself with lifefire.", msg_q);
				if (margin > 0) {
					if (!p_ptr->suscep_life) { //SV_POTION_LOSE_MEMORIES - Kurzel
						if ((!p_ptr->hold_life) && (p_ptr->exp > 0)) {
							msg_print(Ind, "\377GYou feel your memories fade.");
							lose_exp(Ind, p_ptr->exp / 4);
						}
						break;
					}
					else restore_level(Ind); //SV_POTION_RESTORE_EXP - Kurzel
				}
				break; }
				
				default: {
				projection = flags_to_projection(e_flags - R_NETH);
				if (p_ptr->rcraft_empower && projection == 34) projection = RCRAFT_MAX_PROJECTIONS;//Hack - Flux -> Plasma
				damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 10 + 1; //Severely reduced damage (per burst) - Kurzel
				msg_format(Ind, "You %s empower your defenses with %s %s.", msg_q, r_imperatives[imperative].name, r_projections[projection].name);
				if (margin > 0) set_tim_aura_ex(Ind, duration, projection, damage);
				break; }
			}
			break; }
			
			case (R_FIRE | R_POIS): {
			switch (e_flags - (R_FIRE | R_POIS)) {
				case R_ACID: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_fire(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
				
				case R_ELEC: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_elec(Ind, duration);
					set_oppose_fire(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
				
				case R_COLD: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_fire(Ind, duration);
					set_oppose_cold(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
				
				case R_FORC: {
				byte upkeep_rune[5];
				cost = 0;
				if (p_ptr->rcraft_attune == (R_FIRE | R_POIS)) {
					msg_format(Ind, "\377WYour armament is no longer attuned.");
					/* De-attune */
					p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					p_ptr->rcraft_attune = 0;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* De-attune */
						if (p_ptr->rcraft_attune) p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
						/* Can we sustain two more attunes? */
						if (p_ptr->rcraft_upkeep + (2 * UPKEEP_ATTUNE) > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Attune */
						msg_format(Ind, "You %s attune your armament.", msg_q);
						set_tim_brand_fire(Ind, 1);
						set_tim_brand_pois(Ind, 1);
						set_oppose_fire(Ind, 1);
						set_oppose_pois(Ind, 1);
						p_ptr->rcraft_attune = (R_FIRE | R_POIS);
						p_ptr->rcraft_upkeep += flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					}
					else msg_format(Ind, "You %s attune your armament.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
				
				case R_NEXU: {
				msg_format(Ind, "You %s project twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_fire(Ind, duration);
					set_oppose_pois(Ind, duration);
					fire_ball(Ind, GF_RESFIRE_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
					fire_ball(Ind, GF_RESPOIS_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
				}
				break; }
				
				default: {
				msg_format(Ind, "You %s draw twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_fire(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
			}
			break; }
			
			case (R_FIRE | R_NEXU): {
			switch (e_flags - (R_FIRE | R_NEXU)) {
				case 0: {
				msg_format(Ind, "You %s project %s %s rune of resistance.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_cold(Ind, duration);
					fire_ball(Ind, GF_RESCOLD_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
				}
				break; }
				
				case R_COLD: {
				cost = 0;
				if (p_ptr->rcraft_repel & R_COLD) {
					/* De-repel */
					p_ptr->rcraft_upkeep -= UPKEEP_REPEL;
					p_ptr->rcraft_repel -= R_COLD;
					if (p_ptr->rcraft_repel == 0) msg_format(Ind, "\377WYour raiment is no longer imbued.");
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* Can we sustain a repel? */
						if (p_ptr->rcraft_upkeep + UPKEEP_REPEL > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Repel */
						msg_format(Ind, "You %s imbue your raiment.", msg_q);
						set_oppose_cold(Ind, 1);
						set_tim_aura_fire(Ind, 1);
						p_ptr->rcraft_repel |= R_COLD;
						p_ptr->rcraft_upkeep += UPKEEP_REPEL;
					}
					else msg_format(Ind, "You %s imbue your raiment.", msg_q);
				}
				calc_mana(Ind);
				break; }

				default: {
				//Do nothing, these are all reverting spells. - Kurzel
				break; }
			}
			break; }
			
			case (R_FIRE | R_FORC): {
			switch (e_flags - (R_FIRE | R_FORC)) {
				case 0: {
				byte upkeep_rune[5];
				cost = 0;
				if (p_ptr->rcraft_attune == R_FIRE) {
					msg_format(Ind, "\377WYour armament is no longer attuned.");
					/* De-attune */
					p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					p_ptr->rcraft_attune = 0;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* De-attune */
						if (p_ptr->rcraft_attune) p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
						/* Can we sustain one more attune? */
						if (p_ptr->rcraft_upkeep + UPKEEP_ATTUNE > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Attune */
						msg_format(Ind, "You %s attune your armament.", msg_q);
						set_tim_brand_fire(Ind, 1);
						p_ptr->rcraft_attune = R_FIRE;
						p_ptr->rcraft_upkeep += flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					}
					else msg_format(Ind, "You %s attune your armament.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
				
				default: {
				projection = flags_to_projection(e_flags - R_FORC);
				if (p_ptr->rcraft_empower && projection == 34) projection = RCRAFT_MAX_PROJECTIONS;//Hack - Flux -> Plasma
				damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 10 + 1; //Severely reduced damage (per burst) - Kurzel
				msg_format(Ind, "You %s empower your attacks with %s %s.", msg_q, r_imperatives[imperative].name, r_projections[projection].name);
				if (margin > 0) set_tim_brand_ex(Ind, duration, projection, damage); 
				break; }
			}
			break; }
			
			case (R_FIRE | R_TIME): {
			switch (e_flags - (R_FIRE | R_TIME)) {
				default: {
				//Do nothing, these are all combining spells. - Kurzel
				break; }
			}
			break; }
			
			/** R_CHAO **/
			
			case (R_CHAO | R_COLD): {
			switch (e_flags - (R_CHAO | R_COLD)) {
				case 0: { //perhaps add beneficial effects too..? - Kurzel
				msg_format(Ind, "You %s fill yourself with wonder.", msg_q);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
				
				default: {
				projection = flags_to_projection(e_flags - R_CHAO);
				damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 10 + 1; //Severely reduced damage (per burst) - Kurzel
				msg_format(Ind, "You %s empower your defenses with %s %s.", msg_q, r_imperatives[imperative].name, r_projections[projection].name);
				if (margin > 0) set_tim_aura_ex(Ind, duration, projection, damage);
				break; }
			}
			break; }
			
			case (R_CHAO | R_NETH): {
			switch (e_flags - (R_CHAO | R_NETH)) {
				case 0: {
				msg_format(Ind, "You %s dispel lingering enchantments.", msg_q);
				if (margin > 0) remove_curse(Ind);
				break; }
			
#ifdef ENABLE_RUNE_CURSES
//Curse stuff here! - Kurzel
#endif
			
				default: { //General backlash for mal effects; default for all unassigned effects. - Kurzel
				msg_format(Ind, "You %s invoke %s %s sealed runespell.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) p_flags |= RPEN_MAJ_HP;
				break; }
			}
			break; }
			
			case (R_CHAO | R_POIS): {
			switch (e_flags - (R_CHAO | R_POIS)) {
				case 0: {
				cost = 0;
				if (p_ptr->rcraft_upkeep_flags & RUPK_MIN_CO) {
					msg_format(Ind, "\377WYou stop sealing the effects of confusion.");
					/* Remove upkeep */
					p_ptr->rcraft_upkeep -= UPKEEP_MIN;
					p_ptr->rcraft_upkeep_flags -= RUPK_MIN_CO;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* Can we sustain? */
						if (p_ptr->rcraft_upkeep + UPKEEP_MIN > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Add upkeep */
						msg_format(Ind, "You %s seal the effects of confusion.", msg_q);
						p_ptr->rcraft_upkeep_flags |= RUPK_MIN_CO;
						p_ptr->rcraft_upkeep += UPKEEP_MIN;
					}
					else msg_format(Ind, "You %s seal the effects of confusion.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
			
				default: {
				msg_format(Ind, "You %s draw %s %s explosive rune.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) {
					projection = flags_to_projection(e_flags - R_POIS);
					if (set_rune_trap_okay(Ind)) set_rune_trap_aux(Ind, projection, imperative, skill);
				}
				cost = 0;
				break; }
			}
			break; }
			
			case (R_CHAO | R_NEXU): {
			switch (e_flags - (R_CHAO | R_NEXU)) {
				case R_ACID: {
					msg_format(Ind, "You %s draw %s %s rune of protection.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
					if (margin > 0) {
						set_protacid(Ind, duration);
					}
				break; }
				
				case R_ELEC: {
					msg_format(Ind, "You %s draw %s %s rune of protection.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
					if (margin > 0) {
						set_protelec(Ind, duration);
					}
				break; }
				
				case R_FIRE: {
					msg_format(Ind, "You %s draw %s %s rune of protection.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
					if (margin > 0) {
						set_protfire(Ind, duration);
					}
				break; }
				
				case R_COLD: {
					msg_format(Ind, "You %s draw %s %s rune of protection.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
					if (margin > 0) {
						set_protcold(Ind, duration);
					}
				break; }
				
				case R_POIS: {
					msg_format(Ind, "You %s draw %s %s rune of protection.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
					if (margin > 0) {
						set_protpois(Ind, duration);
					}
				break; }
				
				default: {
				msg_format(Ind, "You %s restore yourself to order.", msg_q);
				if (margin > 0) { //Sync this list with self-knowledge / unmagic!? / reset_tim_flags() - Kurzel!!
					//Speed
					set_fast(Ind, 0, 0);
					//Resist
					set_oppose_acid(Ind, 0);
					set_oppose_elec(Ind, 0);
					set_oppose_fire(Ind, 0);
					set_oppose_cold(Ind, 0);
					set_oppose_pois(Ind, 0);
					//Brand
					set_tim_brand_acid(Ind, 0);
					set_tim_brand_elec(Ind, 0);
					set_tim_brand_fire(Ind, 0);
					set_tim_brand_cold(Ind, 0);
					set_tim_brand_pois(Ind, 0);
					set_tim_brand_conf(Ind, 0);
					set_tim_brand_vorp(Ind, 0);
					//Aura
					set_tim_aura_acid(Ind, 0);
					set_tim_aura_elec(Ind, 0);
					set_tim_aura_fire(Ind, 0);
					set_tim_aura_cold(Ind, 0);
					//Shield
					set_tim_deflect(Ind,0);
					set_shield(Ind, 0, 0, SHIELD_NONE, SHIELD_NONE, 0);
					set_tim_elemshield(Ind, 0, 0);
					//Anchor
					set_st_anchor(Ind, 0);
					
					//Misc
					set_tim_manashield(Ind, 0);
					
					/* Remove glyph */
					//if (zcave[p_ptr->py][p_ptr->px].feat == FEAT_GLYPH)
						//cave_set_feat_live(&p_ptr->wpos, p_ptr->py, p_ptr->px, FEAT_FLOOR);
				}
				break; }
			}
			break; }
			
			case (R_CHAO | R_FORC): {
			switch (e_flags - (R_CHAO | R_FORC)) {
				case 0: {
					msg_format(Ind, "You %s summon %s %s shield of entropy.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
					if (margin > 0) {
						set_tim_deflect(Ind, duration);
						set_shield(Ind, duration, rget_level(20) * r_imperatives[imperative].damage / 10, SHIELD_NONE, SHIELD_NONE, 0);
					}
				break; }
				
				default: {
				projection = flags_to_projection(e_flags - R_FORC);
				damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 10 + 1; //Severely reduced damage (per burst) - Kurzel
				msg_format(Ind, "You %s empower your attacks with %s %s.", msg_q, r_imperatives[imperative].name, r_projections[projection].name);
				if (margin > 0) set_tim_brand_ex(Ind, duration, projection, damage); 
				break; }
			}
			break; }
			
			case (R_CHAO | R_TIME): {
			switch (e_flags - (R_CHAO | R_TIME)) {
				default: {
				//Do nothing, these are all combining spells. - Kurzel
				break; }
			}
			break; }
			
			/** R_COLD **/
			
			case (R_COLD | R_NETH): {
			switch (e_flags - (R_COLD | R_NETH)) {
				case 0: {
				cost = 0;
				if (p_ptr->rcraft_upkeep_flags & RUPK_MAJ_TR) {
					msg_format(Ind, "\377WYou no longer revel in the anguish of your foes.");
					/* Remove upkeep */
					p_ptr->rcraft_upkeep -= UPKEEP_MAJ;
					p_ptr->rcraft_upkeep_flags -= RUPK_MAJ_TR;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* Can we sustain? */
						if (p_ptr->rcraft_upkeep + UPKEEP_MAJ > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Add upkeep */
						msg_format(Ind, "You %s revel in the anguish of your foes.", msg_q);
						set_tim_trauma(Ind, 1, (rget_level(20) + 5) * 1000);
						p_ptr->rcraft_upkeep_flags |= RUPK_MAJ_TR;
						p_ptr->rcraft_upkeep += UPKEEP_MAJ;
					}
					else msg_format(Ind, "You %s revel in the anguish of your foes.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
				
				default: {
				/* Hack -- Increase Cost */
				cost *= 3; //Triple cost for LoS spells, enough? - Kurzel
				if (cost < S_COST_MIN) cost = S_COST_MIN;
				if (cost > S_COST_MAX) cost = S_COST_MAX;
				
				projection = flags_to_projection(e_flags - R_NETH);
				msg_format(Ind, "You %s fill the air with %s %s!", msg_q, r_imperatives[imperative].name, r_projections[projection].name);
				if (margin > 0) {
					damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 2 + 1;
					project_hack(Ind, r_projections[projection].gf_type, damage, p_ptr->attacker);
					}
				break; }
			}
			break; }
						
			case (R_COLD | R_POIS): {
			switch (e_flags - (R_COLD | R_POIS)) {
				case R_ACID: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_acid(Ind, duration);
					set_oppose_cold(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
				
				case R_ELEC: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_elec(Ind, duration);
					set_oppose_cold(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
				
				case R_FIRE: {
				msg_format(Ind, "You %s draw triad %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_fire(Ind, duration);
					set_oppose_cold(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
				
				case R_FORC: {
				byte upkeep_rune[5];
				cost = 0;
				if (p_ptr->rcraft_attune == (R_COLD | R_POIS)) {
					msg_format(Ind, "\377WYour armament is no longer attuned.");
					/* De-attune */
					p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					p_ptr->rcraft_attune = 0;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* De-attune */
						if (p_ptr->rcraft_attune) p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
						/* Can we sustain two more attunes? */
						if (p_ptr->rcraft_upkeep + (2 * UPKEEP_ATTUNE) > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Attune */
						msg_format(Ind, "You %s attune your armament.", msg_q);
						set_tim_brand_cold(Ind, 1);
						set_tim_brand_pois(Ind, 1);
						p_ptr->rcraft_attune = (R_COLD | R_POIS);
						p_ptr->rcraft_upkeep += flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					}
					else msg_format(Ind, "You %s attune your armament.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
				
				case R_NEXU: {
				msg_format(Ind, "You %s project twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_cold(Ind, duration);
					set_oppose_pois(Ind, duration);
					fire_ball(Ind, GF_RESCOLD_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
					fire_ball(Ind, GF_RESPOIS_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
				}
				break; }
				
				default: {
				msg_format(Ind, "You %s draw twin %s runes of resistance.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_cold(Ind, duration);
					set_oppose_pois(Ind, duration);
				}
				break; }
			}
			break; }
			
			case (R_COLD | R_NEXU): {
			switch (e_flags - (R_COLD | R_NEXU)) {
				case 0: {
				msg_format(Ind, "You %s project %s %s rune of resistance.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_fire(Ind, duration);
					fire_ball(Ind, GF_RESFIRE_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
				}
				break; }
				
				case R_FIRE: {
				cost = 0;
				if (p_ptr->rcraft_repel & R_FIRE) {
					/* De-repel */
					p_ptr->rcraft_upkeep -= UPKEEP_REPEL;
					p_ptr->rcraft_repel -= R_FIRE;
					if (p_ptr->rcraft_repel == 0) msg_format(Ind, "\377WYour raiment is no longer imbued.");
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* Can we sustain a repel? */
						if (p_ptr->rcraft_upkeep + UPKEEP_REPEL > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Repel */
						msg_format(Ind, "You %s imbue your raiment.", msg_q);
						set_oppose_fire(Ind, 1);
						set_tim_aura_cold(Ind, 1);
						p_ptr->rcraft_repel |= R_FIRE;
						p_ptr->rcraft_upkeep += UPKEEP_REPEL;
					}
					else msg_format(Ind, "You %s imbue your raiment.", msg_q);
				}
				calc_mana(Ind);
				break; }

				default: {
				//Do nothing, these are all reverting spells. - Kurzel
				break; }
			}
			break; }
			
			case (R_COLD | R_FORC): {
			switch (e_flags - (R_COLD | R_FORC)) {
				case 0: {
				byte upkeep_rune[5];
				cost = 0;
				if (p_ptr->rcraft_attune == R_COLD) {
					msg_format(Ind, "\377WYour armament is no longer attuned.");
					/* De-attune */
					p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					p_ptr->rcraft_attune = 0;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* De-attune */
						if (p_ptr->rcraft_attune) p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
						/* Can we sustain one more attune? */
						if (p_ptr->rcraft_upkeep + UPKEEP_ATTUNE > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Attune */
						msg_format(Ind, "You %s attune your armament.", msg_q);
						set_tim_brand_cold(Ind, 1);
						p_ptr->rcraft_attune = R_COLD;
						p_ptr->rcraft_upkeep += flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					}
					else msg_format(Ind, "You %s attune your armament.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
				
				default: {
				projection = flags_to_projection(e_flags - R_FORC);
				damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 10 + 1; //Severely reduced damage (per burst) - Kurzel
				msg_format(Ind, "You %s empower your attacks with %s %s.", msg_q, r_imperatives[imperative].name, r_projections[projection].name);
				if (margin > 0) set_tim_brand_ex(Ind, duration, projection, damage); 
				break; }
			}
			break; }
			
			case (R_COLD | R_TIME): {
			switch (e_flags - (R_COLD | R_TIME)) {
				default: {
				//Do nothing, these are all combining spells. - Kurzel
				break; }
			}
			break; }
			
			/** R_NETH **/
			
			case (R_NETH | R_POIS): {
			switch (e_flags - (R_NETH | R_POIS)) {
				case 0: {
				cost = 0;
				if (p_ptr->rcraft_upkeep_flags & RUPK_MIN_HL) {
					msg_format(Ind, "\377WYou stop sealing the effects of life-draining attacks.");
					/* Remove upkeep */
					p_ptr->rcraft_upkeep -= UPKEEP_MIN;
					p_ptr->rcraft_upkeep_flags -= RUPK_MIN_HL;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* Can we sustain? */
						if (p_ptr->rcraft_upkeep + UPKEEP_MIN > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Add upkeep */
						msg_format(Ind, "You %s seal the effects of life-draining attacks.", msg_q);
						p_ptr->rcraft_upkeep_flags |= RUPK_MIN_HL;
						p_ptr->rcraft_upkeep += UPKEEP_MIN;
					}
					else msg_format(Ind, "You %s seal the effects of life-draining attacks.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
			
				default: {
				msg_format(Ind, "You %s draw %s %s explosive rune.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) {
					projection = flags_to_projection(e_flags - R_POIS);
					if (set_rune_trap_okay(Ind)) set_rune_trap_aux(Ind, projection, imperative, skill);
				}
				cost = 0;
				break; }
			}
			break; }
			
			case (R_NETH | R_NEXU): {
			switch (e_flags - (R_NETH | R_NEXU)) {
				case R_ACID: {
				msg_format(Ind, "You %s conjure %s %s shield of elemental negation.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) set_tim_elemshield(Ind, duration, 0);
				break; }
				
				case R_ELEC: {
				msg_format(Ind, "You %s conjure %s %s shield of elemental negation.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) set_tim_elemshield(Ind, duration, 1);
				break; }
				
				case R_FIRE: {
				msg_format(Ind, "You %s conjure %s %s shield of elemental negation.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) set_tim_elemshield(Ind, duration, 2);
				break; }
				
				case R_COLD: {
				msg_format(Ind, "You %s conjure %s %s shield of elemental negation.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) set_tim_elemshield(Ind, duration, 3);
				break; }
				
				case R_POIS: {
				msg_format(Ind, "You %s conjure %s %s shield of elemental negation.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) set_tim_elemshield(Ind, duration, 4);
				break; }
				
				default: {
					msg_format(Ind, "You %s invoke %s %s rune of recharging.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
					if (margin > 0) recharge(Ind, rget_level(50) * r_imperatives[imperative].damage / 10);
				break; }
			}
			break; }
			
			case (R_NETH | R_FORC): {
			switch (e_flags - (R_NETH | R_FORC)) {
				case 0: {
					msg_format(Ind, "You %s summon %s %s cloak of invisibility.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
					if (margin > 0) set_invis(Ind, duration, damage);
				break; }
				
				default: {
				projection = flags_to_projection(e_flags - R_FORC);
				damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 10 + 1; //Severely reduced damage (per burst) - Kurzel
				msg_format(Ind, "You %s empower your attacks with %s %s.", msg_q, r_imperatives[imperative].name, r_projections[projection].name);
				if (margin > 0) set_tim_brand_ex(Ind, duration, projection, damage); 
				break; }
			}
			break; }
			
			case (R_NETH | R_TIME): {
			switch (e_flags - (R_NETH | R_TIME)) {
				default: {
				//Do nothing, these are all combining spells. - Kurzel
				break; }
			}
			break; }
			
			/** R_POIS **/
			
			case (R_POIS | R_NEXU): {
			switch (e_flags - (R_POIS | R_NEXU)) {
				case 0: {
				msg_format(Ind, "You %s project %s %s rune of resistance.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) {
					set_oppose_pois(Ind, duration);
					fire_ball(Ind, GF_RESPOIS_PLAYER, 5, duration * 2, radius, p_ptr->attacker);
				}
				break; }
				
				default: {
				msg_format(Ind, "You %s conjure %s %s matrix of spell energy.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) {
					duration = rspell_duration(Ind, element, elements, imperative, 6, skill, level);//Duration as T_CLOU
					projection = flags_to_projection(e_flags - R_NEXU);
					damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 10 + 1;
					set_tim_rcraft_help(Ind, duration, RCRAFT_MAX_TYPES, projection, damage); //Hack -- T_LoS
				}
				break; }
			}
			break; }
			
			case (R_POIS | R_FORC): {
			switch (e_flags - (R_POIS | R_FORC)) {
				case 0: {
				byte upkeep_rune[5];
				cost = 0;
				if (p_ptr->rcraft_attune == R_POIS) {
					msg_format(Ind, "\377WYour armament is no longer attuned.");
					/* De-attune */
					p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					p_ptr->rcraft_attune = 0;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* De-attune */
						if (p_ptr->rcraft_attune) p_ptr->rcraft_upkeep -= flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
						/* Can we sustain one more attune? */
						if (p_ptr->rcraft_upkeep + UPKEEP_ATTUNE > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Attune */
						msg_format(Ind, "You %s attune your armament.", msg_q);
						set_tim_brand_pois(Ind, 1);
						set_oppose_pois(Ind, 1);
						p_ptr->rcraft_attune = R_POIS;
						p_ptr->rcraft_upkeep += flags_to_elements(upkeep_rune, p_ptr->rcraft_attune) * UPKEEP_ATTUNE;
					}
					else msg_format(Ind, "You %s attune your armament.", msg_q);
				}
				calc_mana(Ind);
				calc_boni(Ind);
				break; }
				
				default: {
				projection = flags_to_projection(e_flags - R_FORC);
				damage = rspell_damage(Ind, element, elements, imperative, type, skill, projection) / 10 + 1; //Severely reduced damage (per burst) - Kurzel
				msg_format(Ind, "You %s empower your attacks with %s %s.", msg_q, r_imperatives[imperative].name, r_projections[projection].name);
				if (margin > 0) set_tim_brand_ex(Ind, duration, projection, damage); 
				break; }
			}
			break; }
			
			case (R_POIS | R_TIME): {
			switch (e_flags - (R_POIS | R_TIME)) {
				case 0: {
				cost = 0;
				if (p_ptr->rcraft_regen) {
					msg_format(Ind, "\377WYour ability to regenerate returns to normal.");
					/* Remove upkeep */
					p_ptr->rcraft_upkeep -= UPKEEP_HI;
					p_ptr->rcraft_regen = 0;
					p_flags = 0;
				}
				else {
					if (margin > 0) {
						/* Remove upkeep */
						if (p_ptr->rcraft_regen) p_ptr->rcraft_upkeep -= UPKEEP_HI;
						/* Can we sustain? */
						if (p_ptr->rcraft_upkeep + UPKEEP_HI > 100) {
							msg_print(Ind, "\377oYou cannot sustain this runespell!");
							break;
						}
						/* Add upkeep */
						msg_format(Ind, "You %s improve your ability to regenerate.", msg_q);
						set_tim_regen(Ind, duration, rget_level(300 * r_imperatives[imperative].damage));
						p_ptr->rcraft_regen = 1; //toggle
						p_ptr->rcraft_upkeep += UPKEEP_HI;
					}
					else msg_format(Ind, "You %s improve your ability to regenerate.", msg_q);
				}
				calc_mana(Ind);
				break; }
				
				default: {
				msg_format(Ind, "You %s draw %s %s explosive rune.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) {
					projection = flags_to_projection(e_flags - R_TIME);
					if (set_rune_trap_okay(Ind)) set_rune_trap_aux(Ind, projection, imperative, skill);
				}
				cost = 0;
				break; }
			}
			break; }
			
			/** R_NEXU **/
			
			case (R_NEXU | R_FORC): {
			switch (e_flags - (R_NEXU | R_FORC)) {
				case R_ACID: {
				msg_format(Ind, "You %s invoke %s elemental teleportation.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					type = 0;
					duration = rspell_duration(Ind, element, elements, imperative, 5, skill, level);//duration T_WAVE
					//fire_wave(Ind, GF_ACID, 0, damage / 5, 0, duration, 2, EFF_WAVE, p_ptr->attacker);
					if (p_ptr->memory_feat[type]) {
						if (inarea(&p_ptr->memory_port[type].wpos, &p_ptr->wpos)) {
							swap_position(Ind, p_ptr->memory_port[type].y, p_ptr->memory_port[type].x); //paranoia
							/* Restore the feature of the previous existing portal */
							cave_set_feat_live(&p_ptr->wpos, p_ptr->memory_port[type].y, p_ptr->memory_port[type].x, p_ptr->memory_feat[type]);
							/* Upkeep */
							p_ptr->rcraft_upkeep -= UPKEEP_PORT;
							/* Reset Values */
							p_ptr->memory_feat[type] = 0;
							wpcopy(&p_ptr->memory_port[type].wpos, &p_ptr->wpos);
							p_ptr->memory_port[type].x = p_ptr->px;
							p_ptr->memory_port[type].y = p_ptr->py;
						}
						else teleport_player(Ind, 10 * (radius) + rget_level(10 * radius), FALSE);
					}
					else teleport_player(Ind, 10 * (radius) + rget_level(10 * radius), FALSE);
				}
				break; }
				
				case R_ELEC: {
				msg_format(Ind, "You %s invoke %s elemental teleportation.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					type = 1;
					duration = rspell_duration(Ind, element, elements, imperative, 5, skill, level);//duration T_WAVE
					//fire_wave(Ind, GF_ELEC, 0, damage / 5, 0, duration, 2, EFF_WAVE, p_ptr->attacker);
					if (p_ptr->memory_feat[type]) {
						if (inarea(&p_ptr->memory_port[type].wpos, &p_ptr->wpos)) {
							swap_position(Ind, p_ptr->memory_port[type].y, p_ptr->memory_port[type].x); //paranoia
							/* Restore the feature of the previous existing portal */
							cave_set_feat_live(&p_ptr->wpos, p_ptr->memory_port[type].y, p_ptr->memory_port[type].x, p_ptr->memory_feat[type]);
							/* Upkeep */
							p_ptr->rcraft_upkeep -= UPKEEP_PORT;
							/* Reset Values */
							p_ptr->memory_feat[type] = 0;
							wpcopy(&p_ptr->memory_port[type].wpos, &p_ptr->wpos);
							p_ptr->memory_port[type].x = p_ptr->px;
							p_ptr->memory_port[type].y = p_ptr->py;
						}
						else teleport_player(Ind, 10 * (radius) + rget_level(10 * radius), FALSE);
					}
					else teleport_player(Ind, 10 * (radius) + rget_level(10 * radius), FALSE);
				}
				break; }
				
				case R_FIRE: {
				msg_format(Ind, "You %s invoke %s elemental teleportation.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					type = 2;
					duration = rspell_duration(Ind, element, elements, imperative, 5, skill, level);//duration T_WAVE
					//fire_wave(Ind, GF_FIRE, 0, damage / 5, 0, duration, 2, EFF_WAVE, p_ptr->attacker);
					if (p_ptr->memory_feat[type]) {
						if (inarea(&p_ptr->memory_port[type].wpos, &p_ptr->wpos)) {
							swap_position(Ind, p_ptr->memory_port[type].y, p_ptr->memory_port[type].x); //paranoia
							/* Restore the feature of the previous existing portal */
							cave_set_feat_live(&p_ptr->wpos, p_ptr->memory_port[type].y, p_ptr->memory_port[type].x, p_ptr->memory_feat[type]);
							/* Upkeep */
							p_ptr->rcraft_upkeep -= UPKEEP_PORT;
							/* Reset Values */
							p_ptr->memory_feat[type] = 0;
							wpcopy(&p_ptr->memory_port[type].wpos, &p_ptr->wpos);
							p_ptr->memory_port[type].x = p_ptr->px;
							p_ptr->memory_port[type].y = p_ptr->py;
						}
						else teleport_player(Ind, 10 * (radius) + rget_level(10 * radius), FALSE);
					}
					else teleport_player(Ind, 10 * (radius) + rget_level(10 * radius), FALSE);
				}
				break; }
				
				case R_COLD: {
				msg_format(Ind, "You %s invoke %s elemental teleportation.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					type = 3;
					duration = rspell_duration(Ind, element, elements, imperative, 5, skill, level);//duration T_WAVE
					//fire_wave(Ind, GF_COLD, 0, damage / 5, 0, duration, 2, EFF_WAVE, p_ptr->attacker);
					if (p_ptr->memory_feat[type]) {
						if (inarea(&p_ptr->memory_port[type].wpos, &p_ptr->wpos)) {
							swap_position(Ind, p_ptr->memory_port[type].y, p_ptr->memory_port[type].x); //paranoia
							/* Restore the feature of the previous existing portal */
							cave_set_feat_live(&p_ptr->wpos, p_ptr->memory_port[type].y, p_ptr->memory_port[type].x, p_ptr->memory_feat[type]);
							/* Upkeep */
							p_ptr->rcraft_upkeep -= UPKEEP_PORT;
							/* Reset Values */
							p_ptr->memory_feat[type] = 0;
							wpcopy(&p_ptr->memory_port[type].wpos, &p_ptr->wpos);
							p_ptr->memory_port[type].x = p_ptr->px;
							p_ptr->memory_port[type].y = p_ptr->py;
						}
						else teleport_player(Ind, 10 * (radius) + rget_level(10 * radius), FALSE);
					}
					else teleport_player(Ind, 10 * (radius) + rget_level(10 * radius), FALSE);
				}
				break; }
				
				case R_POIS: {
				msg_format(Ind, "You %s invoke %s elemental teleportation.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					type = 4;
					duration = rspell_duration(Ind, element, elements, imperative, 5, skill, level);//duration T_WAVE
					//fire_wave(Ind, GF_POIS, 0, damage / 5, 0, duration, 2, EFF_WAVE, p_ptr->attacker);
					if (p_ptr->memory_feat[type]) {
						if (inarea(&p_ptr->memory_port[type].wpos, &p_ptr->wpos)) {
							swap_position(Ind, p_ptr->memory_port[type].y, p_ptr->memory_port[type].x); //paranoia
							/* Restore the feature of the previous existing portal */
							cave_set_feat_live(&p_ptr->wpos, p_ptr->memory_port[type].y, p_ptr->memory_port[type].x, p_ptr->memory_feat[type]);
							/* Upkeep */
							p_ptr->rcraft_upkeep -= UPKEEP_PORT;
							/* Reset Values */
							p_ptr->memory_feat[type] = 0;
							wpcopy(&p_ptr->memory_port[type].wpos, &p_ptr->wpos);
							p_ptr->memory_port[type].x = p_ptr->px;
							p_ptr->memory_port[type].y = p_ptr->py;
						}
						else teleport_player(Ind, 10 * (radius) + rget_level(10 * radius), FALSE);
					}
					else teleport_player(Ind, 10 * (radius) + rget_level(10 * radius), FALSE);
				}
				break; }
				
				case R_TIME: {
				msg_format(Ind, "You %s draw a glyph of warding.", msg_q);
				if (margin > 0) warding_glyph(Ind);
				break; }
				
				default: {
				msg_format(Ind, "You %s invoke %s teleportation.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) teleport_player(Ind, 10 * (radius) + rget_level(10 * radius), FALSE);
				break; }
			}
			break; }
			
			case (R_NEXU | R_TIME): {
			switch (e_flags - (R_NEXU | R_TIME)) {
				case 0: {
				msg_format(Ind, "You %s invoke %s %s space-time anchor.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) set_st_anchor(Ind, duration);
				break; }
				
				case R_ACID: {
				cost = 0;
				msg_format(Ind, "You %s memorize this location.", msg_q);
				if (margin > 0) if (set_rune_port_okay(Ind, 0)) set_rune_port_aux(Ind, 0);
				break; }
				
				case R_ELEC: {
				cost = 0;
				msg_format(Ind, "You %s memorize this location.", msg_q);
				if (margin > 0) if (set_rune_port_okay(Ind, 1)) set_rune_port_aux(Ind, 1);
				break; }
				
				case R_FIRE: {
				cost = 0;
				msg_format(Ind, "You %s memorize this location.", msg_q);
				if (margin > 0) if (set_rune_port_okay(Ind, 2)) set_rune_port_aux(Ind, 2);
				break; }
				
				case R_COLD: {
				cost = 0;
				msg_format(Ind, "You %s memorize this location.", msg_q);
				if (margin > 0) if (set_rune_port_okay(Ind, 3)) set_rune_port_aux(Ind, 3);
				break; }
				
				case R_POIS: {
				cost = 0;
				msg_format(Ind, "You %s memorize this location.", msg_q);
				if (margin > 0) if (set_rune_port_okay(Ind, 4)) set_rune_port_aux(Ind, 4);
				break; }
				
				case R_FORC: {
				msg_format(Ind, "You %s draw a glyph of warding.", msg_q);
				if (margin > 0) warding_glyph(Ind);
				break; }
				
				case R_WATE:
				case R_EART:
				case R_CHAO:
				case R_NETH: {
				msg_format(Ind, "You %s cast a circle of protection.", msg_q);
				if (margin > 0) fire_ball(Ind, GF_MAKE_GLYPH, 0, 1, 1, "");
				break; }
			}
			break; }
			
			/** R_FORC **/
			
			case (R_FORC | R_TIME): {
			switch (e_flags - (R_FORC | R_TIME)) {
				case 0: {
				msg_format(Ind, "You %s invoke %s haste.", msg_q, r_imperatives[imperative].name);
				if (margin > 0) {
					damage = rget_level(10) * r_imperatives[imperative].damage / 10;
					if (damage < 1) damage = 1;
					if (damage > 10) damage = 10;
					set_fast(Ind, duration, damage);
				}
				break; }
				
				case R_NEXU: {
				msg_format(Ind, "You %s draw a glyph of warding.", msg_q);
				if (margin > 0) warding_glyph(Ind);
				break; }
				
				default: {
				msg_format(Ind, "You %s draw %s %s explosive rune.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
				if (margin > 0) {
					projection = flags_to_projection(e_flags - R_TIME);
					if (set_rune_trap_okay(Ind)) set_rune_trap_aux(Ind, projection, imperative, skill);
				}
				cost = 0;
				break; }
			}
			break; }
			
			/** Default **/
			
			default: { //General backlash for mal effects; default for all unassigned effects. - Kurzel
			msg_format(Ind, "You %s invoke %s %s sealed runespell.", msg_q, imperative == 4 ? "an" : "a", r_imperatives[imperative].name);
			if (margin > 0) p_flags |= RPEN_MAJ_HP;
			break; }
		break; }
		}

		/* for GF_OLD_DRAIN */
                if (p_ptr->ret_dam) {
                        hp_player(Ind, p_ptr->ret_dam);
                        p_ptr->ret_dam = 0;
                }
	}
	
	/** Miscellaneous Effects **/

#ifdef RCRAFT_DEBUG
s_printf("RCRAFT_DEBUG: %d\n", p_ptr->rcraft_upkeep);
#endif
	
#ifdef ENABLE_GROUP_SPELLS
	/* Attempt to create new rune charges */
	int damage_gf_rcraft_player = -1;
	if (margin > 0 && p_ptr->rcraft_project) {
#ifdef RCRAFT_DEBUG
s_printf("RCRAFT_DEBUG: rune[0]: %d rune[1]: %d.\n", rune[0], rune[1]);
#endif
		if (rune[0] >= 0 && rune[0] < RCRAFT_MAX_ELEMENTS
		 && rune[1] >= 0 && rune[1] < RCRAFT_MAX_ELEMENTS) { 
			damage_gf_rcraft_player = (rune[1] + 1) * 100 + rune[0];
			fire_ball(Ind, GF_RCRAFT_PLAYER, 5, damage_gf_rcraft_player, p_ptr->rcraft_project, p_ptr->attacker);
		}
		else if (rune[0] >= 0 && rune[0] < RCRAFT_MAX_ELEMENTS) {
			damage_gf_rcraft_player = rune[0];
			fire_ball(Ind, GF_RCRAFT_PLAYER, 5, damage_gf_rcraft_player, p_ptr->rcraft_project, p_ptr->attacker);
		}
	}
#endif
	
	/* Lite Susceptability Check */
	if ((margin > 0) && (gf_main == GF_LITE)) {
		if (p_ptr->suscep_lite && !p_ptr->resist_lite && !p_ptr->resist_blind) (void)set_blind(Ind, p_ptr->blind + 4 + randint(10));
		if (p_ptr->suscep_lite && !p_ptr->resist_lite) project(PROJECTOR_RUNE, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px, (damage % HACK_DAM_FACTOR) / 5 + 1, GF_LITE, PROJECT_KILL | PROJECT_NORF, "");
		p_ptr->shooting_till_kill = FALSE;
	}
	
	/** Expend Resources **/
	
	/* Penalty */
	if (p_flags) {
		rspell_do_penalty(Ind, gf_type, damage, cost, p_flags, link);
		p_ptr->shooting_till_kill = FALSE;
	}
	
	/* Mana */
	calc_mana(Ind);
	if (p_ptr->csp > cost) p_ptr->csp -= cost;
	else {
		/* The damage is implied by the "Exhausted, ..." message, so not explicitly stated. */
		take_hit(Ind, (cost-p_ptr->csp), "magical exhaustion", 0);
		p_ptr->csp = 0;
		p_ptr->shooting_till_kill = FALSE;
	}
	p_ptr->redraw |= PR_MANA;
	
	/* Energy */
	p_ptr->energy -= energy;
	
	return 1;
}

/* Backlash when a player fails to disarm a rune trap */
void rune_trap_backlash(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int typ = GF_FIRE, dam = 0, rad = 1, i;
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;
	struct c_special *cs_ptr;
	cave_type *c_ptr;

	/* Disturb the player */
	disturb(Ind, 0, 0);

	/* Get the cave grid */
	if(!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[p_ptr->py][p_ptr->px];
	if(!(cs_ptr = GetCS(c_ptr, CS_RUNE_TRAP))) return;
	
	int skill = cs_ptr->sc.runetrap.lev; //Use 'skill' for the rget_level() macro. - Kurzel

	typ = r_projections[cs_ptr->sc.runetrap.typ].gf_type;
	rad = 2 + r_imperatives[cs_ptr->sc.runetrap.mod].radius;
	//Hacky quick 'ball' damage calc - Kurzel
	dam = r_projections[cs_ptr->sc.runetrap.typ].weight * rget_level(25) * rget_level(25) / W_MAX_DIR;
	dam = dam * r_imperatives[cs_ptr->sc.runetrap.mod].damage / 10;
	if (dam < 1) dam = 1;
	
	remove_rune_trap_upkeep(0, cs_ptr->sc.runetrap.id, p_ptr->px, p_ptr->py);

#ifdef USE_SOUND_2010
	if (typ != GF_DETONATION) {
		sound(Ind, "ball", NULL, SFX_TYPE_MISC, FALSE);
	} else {
		sound(Ind, "detonation", NULL, SFX_TYPE_MISC, FALSE);
	}
#endif

	/* trap is gone */
	i = cs_ptr->sc.runetrap.feat;
	cs_erase(c_ptr, cs_ptr);
	cave_set_feat_live(wpos, p_ptr->py, p_ptr->px, i);

	/* Trapping skill influences damage - C. Blue */
	//dam *= (5 + cs_ptr->sc.runetrap.mod); dam /= 10;
	//dam *= (50 + cs_ptr->sc.runetrap.lev); dam /= 100;
	
	//Use minor backlash calculation. - Kurzel
	dam = dam / 5;

	project(PROJECTOR_RUNE, rad, &p_ptr->wpos, p_ptr->py, p_ptr->px, dam, typ, PROJECT_KILL | PROJECT_NORF, "");
}

/* Remove a rune trap from the caster's upkeep,
   by either specifying a wpos or a player Index. - C. Blue
   Specify either Ind or id
   and add x,y or -1,y for specific trap/all traps. */
void remove_rune_trap_upkeep(int Ind, s32b id, int x, int y) {
	player_type *p_ptr = NULL;
	int i;

	if (!Ind) for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED) continue;
		if (Players[i]->id != id) continue;

		Ind = i;
		break;
	}
	if (!Ind) return; /* shouldn't happen */

	p_ptr = Players[Ind];

	/* erase all traps of a player? (when leaving/changing wpos) */
	if (x == -1) {
		p_ptr->rcraft_upkeep -= p_ptr->runetraps * (100 / UPKEEP_TRAP);
		p_ptr->runetraps = 0;
		calc_mana(Ind);
		return;
	}

	/* erase this trap */
	for (i = 0; i < p_ptr->runetraps; i++) {
		if (p_ptr->runetrap_x[i] == x && p_ptr->runetrap_y[i] == y) {
			for (; i < p_ptr->runetraps - 1; i++) {
				p_ptr->runetrap_x[i] = p_ptr->runetrap_x[i + 1];
				p_ptr->runetrap_y[i] = p_ptr->runetrap_y[i + 1];
			}
			p_ptr->runetrap_x[i] = p_ptr->runetrap_y[i] = 0;
			p_ptr->rcraft_upkeep -=  100 / UPKEEP_TRAP;
			p_ptr->runetraps--;
			break;
		}
	}
	calc_mana(Ind);
}

/* Check if setting a rune portal is possible - Kurzel */
bool set_rune_port_okay(int Ind, byte type) {
	player_type *p_ptr = Players[Ind];

	cave_type *c_ptr;
	cave_type **zcave;

	zcave = getcave(&p_ptr->wpos);
	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	if ((f_info[c_ptr->feat].flags1 & FF1_PROTECTED) ||
	    (c_ptr->info & CAVE_PROT)) {
		msg_print(Ind, "You cannot set rune portals on this special floor.");
		return FALSE;
	}

	/* Only set traps on clean floor grids */
	/* TODO: allow to set traps on poisoned floor */
	if (!cave_clean_bold(zcave, p_ptr->py, p_ptr->px) ||
	    c_ptr->special || c_ptr->feat == FEAT_RUNE_PORT || istown(&p_ptr->wpos)) { //don't override existing portals - Kurzel
		msg_print(Ind, "You cannot set a portal here.");
		return FALSE;
	}
	
	/* Can we sustain one more rune portal? */
	if (!p_ptr->memory_feat[type] && p_ptr->rcraft_upkeep + UPKEEP_PORT > 100) {
		msg_print(Ind, "\377oYou cannot sustain another rune portal!");
		return FALSE;
	}

	/* looks ok */
	return TRUE;
}

void set_rune_port_aux(int Ind, byte type) {
	player_type *p_ptr = Players[Ind];
	int py = p_ptr->py, px = p_ptr->px;

	cave_type *c_ptr;
	cave_type **zcave;

	zcave = getcave(&p_ptr->wpos);
	c_ptr = &zcave[py][px];

	/* Handle existing portals */
	byte j;
	for (j = 0; j < 5; j++) {
		if (p_ptr->memory_feat[j] && j == type) {
			/* Restore the feature of the previous existing portal */
			cave_set_feat_live(&p_ptr->wpos_old, p_ptr->memory_port[j].y, p_ptr->memory_port[j].x, p_ptr->memory_feat[j]);
			p_ptr->rcraft_upkeep -= UPKEEP_PORT;
		}
	}

	/* Store the portal info */
	p_ptr->memory_feat[type] = c_ptr->feat;
	wpcopy(&p_ptr->memory_port[type].wpos, &p_ptr->wpos);
	p_ptr->memory_port[type].x = px;
	p_ptr->memory_port[type].y = py;
	
	/* Upkeep */
	p_ptr->rcraft_upkeep += UPKEEP_PORT;
	calc_mana(Ind);
	
	/* Set the Portal */
	cave_set_feat_live(&p_ptr->wpos, py, px, FEAT_RUNE_PORT);
}

#endif
