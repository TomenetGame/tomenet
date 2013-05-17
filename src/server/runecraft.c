/* $Id$ */
/* File: runecraft.c */

/* Purpose: Runecraft Spell System */
/*
 * Spells are created on the fly with an mkey interface.
 * Currently as a combination of 1-2 elements, an imperative and a type.
 * More elements, imperatives, and types could be supported in theory.
 * Eventually to be luaized for smoother maintainance once final.
 * 
 * By Relsiet/Andy Dubbeld (andy@foisdin.com)
 * Maintained by Kurzel (kurzel.tomenet@gmail.com)
 */
 
 /* Runecraft Credits: Mark, Adam, Relsiet, C.Blue, Kurzel */

#include "angband.h"

/** Options **/

/* Class Options */
//#define ENABLE_SKILLFUL_CASTING //Increases or decreases damage for individual spells slightly, based on fail rate margin. (Disabled currently, would mismatch INFO mkey/wizard displays.)
//#define ENABLE_SUSCEPT //Extra backlash if the caster is susceptable to the projection element. (Disabled for now, until it can be completely implemented across classes; ie. Globe of Light hurts Dark-Elf casters.)
//#define ENABLE_LIFE_CASTING //Allows casting with HP if not enough MP. (Disabled due to simplified, damage only, backlash penalties. We don't want OP necro/blood (infinite mana) runies!)
#ifdef ENABLE_LIFE_CASTING
 #define SAFE_RETALIATION //Disable retaliation/FTK when casting with HP.
#endif

/* Spell Options */
//#define ENABLE_AVERAGE_SKILL //Take the average skill level, rather than the lowest. (Disabled to prevent one skill granting half-access to all others.)
#define ENABLE_BLIND_CASTING //Blinding increases fail chance instead of preventing the cast. (No 'reading' aka 'no_lite(Ind)' check as with books.)
//#define ENABLE_CONFUSED_CASTING //Confusion increases fail chance instead of preventing the cast. (Runecraft tends toward magic rather than something like archery, so this remains disabled.)
#define ENABLE_SHELL_ENCHANT //Allow the T_ENCH (enchanting) spell type to be cast under an antimagic shell. (Allows equipment to be targetted, all other 'projection' types still fail.)

/* Spell Constants */
#define RCRAFT_PJ_RADIUS 2 //What radius do friendly projecting spells project outward to? (Default: 2; This matches auto-magically projecting spells of other classes.)

/* Interface */
//#define FEEDBACK_MESSAGE //Gives the caster a feedback message if penalized. (Disabled under the pretext that we want to reduce messages. The guide is now more explicit about failure of runespells.)
//#define RCRAFT_DEBUG //Enables debugging messages for the server.

/** Internal Headers **/

/* Decode Runespell */
byte flags_to_elements(byte element[], u16b e_flags);
byte flags_to_imperative(u16b m_flags);
byte flags_to_type(u16b m_flags);
byte flags_to_projection(u16b e_flags);

/* Validate Runespell */
byte rspell_skill(int Ind, byte element[], byte elements);
byte rspell_level(byte imperative, byte type);
s16b rspell_diff(byte skill, byte level);
s16b rspell_energy(int Ind, byte imperative, byte type);
byte rspell_cost(byte imperative, byte type, byte skill);

/* Runespell Parameters */
byte rspell_fail(int Ind, byte imperative, byte type, s16b diff, u16b penalty);
u16b rspell_damage(u32b *dx, u32b *dy, byte imperative, byte type, byte skill, byte projection);
byte rspell_radius(byte imperative, byte type, byte skill, byte projection);
byte rspell_duration(byte imperative, byte type, byte skill, byte projection, u16b dice);

/* Item Handling */
object_type * rspell_trap(int Ind, byte projection);
bool rspell_socket(int Ind, byte projection, bool sigil);
bool rspell_sigil(int Ind, byte projection, byte imperative, u16b item);

/** Internal Functions **/

byte flags_to_elements(byte element[], u16b e_flags) {
	byte elements = 0;
	byte i;
	for (i = 0; i < RCRAFT_MAX_ELEMENTS; i++)
		if ((e_flags & r_elements[i].flag) == r_elements[i].flag) {
			element[elements] = i;
			elements++;
		}
	return elements;
}

byte flags_to_imperative(u16b m_flags) {
	byte i;
	for (i = 0; i < RCRAFT_MAX_IMPERATIVES; i++)
		if ((m_flags & r_imperatives[i].flag) == r_imperatives[i].flag) 
			return i;
	return -1;
}

byte flags_to_type(u16b m_flags) {
	byte i;
	for (i = 0; i < RCRAFT_MAX_TYPES; i++)
		if ((m_flags & r_types[i].flag) == r_types[i].flag) 
			return i;
	return -1;
}

byte flags_to_projection(u16b e_flags) {
	byte i;
	for (i = 0; i < RCRAFT_MAX_PROJECTIONS; i++) {
		if (e_flags == r_projections[i].flags)
			return i;
	}
	return -1;
}

byte rspell_skill(int Ind, byte element[], byte elements) {
	player_type *p_ptr = Players[Ind];
	u16b skill = 0;
	byte i;
#ifdef ENABLE_AVERAGE_SKILL //calculate an average
	for (i = 0; i < elements; i++) {
		skill += get_skill(p_ptr, r_elements[element[i]].skill);
	}
	skill /= elements;
#else //take the lowest value
    u16b skill_compare;
    skill = skill - 1; //overflow to largest u16b value
    for (i = 0; i < elements; i++) {
        skill_compare = get_skill(p_ptr, r_elements[element[i]].skill);
        if (skill_compare < skill) skill = skill_compare;
    }
#endif
	return (byte)skill;
}

byte rspell_level(byte imperative, byte type) {
	byte level = 0;
	level += r_imperatives[imperative].level;
	level += r_types[type].level;
	return level;
}

s16b rspell_diff(byte skill, byte level) {
	s16b diff = skill - level + 1;
	if (diff > S_DIFF_MAX) return S_DIFF_MAX;
	else return diff;
}

s16b rspell_energy(int Ind, byte imperative, byte type) {
	player_type * p_ptr = Players[Ind];
	s16b energy = level_speed(&p_ptr->wpos);
	energy = energy * r_imperatives[imperative].energy / 10;
	return energy;
}

byte rspell_cost(byte imperative, byte type, byte skill) {
	u16b cost = r_types[type].c_min + rget_level(r_types[type].c_max - r_types[type].c_min);
	cost = cost * r_imperatives[imperative].cost / 10;
	if (cost < S_COST_MIN) cost = S_COST_MIN;
	if (cost > S_COST_MAX) cost = S_COST_MAX;
	return (byte)cost;
}

byte rspell_fail(int Ind, byte imperative, byte type, s16b diff, u16b penalty) {
	player_type *p_ptr = Players[Ind];

	/* Set the base failure rate; currently 50% at equal skill to level such that the range is [5,95] over 30 levels. */
	s16b fail = 3 * (S_DIFF_MAX - diff) + 5;

	/* Modifier */
	fail += r_imperatives[imperative].fail;

	/* Status Penalty */
	fail += penalty; //Place before stat modifier; casters of great ability can reduce the penalty for casting while hindered?

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

u16b rspell_damage(u32b *dx, u32b *dy, byte imperative, byte type, byte skill, byte projection) {
	u32b damage = rget_weight(r_projections[projection].weight);
	u32b d1, d2;

	/* Modifier */
	damage = damage * r_imperatives[imperative].damage / 10;

	/* Calculation */
	d1 = r_types[type].d1min + rget_level(r_types[type].d1max - r_types[type].d1min) * damage / S_WEIGHT_HI; 
	d2 = r_types[type].d2min + rget_level(r_types[type].d2max - r_types[type].d2min) * damage / S_WEIGHT_HI;
	damage = r_types[type].dbmin + rget_level(r_types[type].dbmax - r_types[type].dbmin) * damage / S_WEIGHT_HI;

	/* Return */
	*dx = (byte)d1;
	*dy = (byte)d2;
	
	/* Further modify for individual spells */
	if (r_imperatives[imperative].flag == I_ENHA) {
		if (r_types[type].flag == T_SIGN) {
			switch (projection) {
				case SV_R_TIME: { //stasis
					damage = 50 + rget_level(50) * r_imperatives[imperative].damage / 10;
					if (damage > 100) damage = 100;
					if (damage < 50) damage = 50;
				break; }
				
				case SV_R_CONF: { //recharging
					damage = (50 + rget_level(50)) * r_imperatives[imperative].damage / 10;
					if (damage > 100) damage = 100;
					if (damage < 50) damage = 50;
				break; }
			}
		}
	} else {
		if (r_types[type].flag == T_SIGN) {
			switch (projection) {
				
				case SV_R_WATE: { //regen
					damage = rget_level(700) * r_imperatives[imperative].damage / 10;
					if (damage > 700) damage = 700;
					if (damage < 70) damage = 70;
				break; }

				case SV_R_TIME: { //speed
					damage = rget_level(15) * r_imperatives[imperative].damage / 10;
					if (damage > 10) damage = 10;
					if (damage < 1) damage = 1;
				break; }
				
				case SV_R_FORC: { //armor
					damage = rget_level(20) * r_imperatives[imperative].damage / 10;
					if (damage > 20) damage = 20;
					if (damage < 1) damage = 1;
				}

				default: {
				break; }
			}
		}
	}
	
	return (u16b)damage;
}

byte rspell_radius(byte imperative, byte type, byte skill, byte projection) {
	s16b radius = r_types[type].r_min + rget_level(r_types[type].r_max - r_types[type].r_min) * rget_weight(r_projections[projection].weight) / S_WEIGHT_HI;
	radius += r_imperatives[imperative].radius;
	if (radius < S_RADIUS_MIN) radius = S_RADIUS_MIN;
	if (radius > S_RADIUS_MAX) radius = S_RADIUS_MAX;
	
	/* Further modify for individual spells (not limited) */
	if (r_types[type].flag == T_SIGN) {
		switch (projection) {
			case SV_R_NEXU: { //blink
				//radius = rget_level(150) * radius / S_RADIUS_MAX;
				radius = rget_level(25) * radius / S_RADIUS_MAX;
				if (radius > 25) radius = 25;
				if (radius < 5) radius = 5;
			break; }

			case SV_R_GRAV: { //tele_to
				radius = rget_level(10) * radius / S_RADIUS_MAX;
			break; }

			case SV_R_SHAR: { //earthquake
				radius = rget_level(10) * radius / S_RADIUS_MAX;
			break; }
			
			default: {
			break; }
		}
	}
	
	return (byte)radius;
}

byte rspell_duration(byte imperative, byte type, byte skill, byte projection, u16b dice) {
	s16b duration = r_types[type].d_min + rget_level(r_types[type].d_max - r_types[type].d_min);
	duration = duration * r_imperatives[imperative].duration / 10;
	
	/* Further modify for individual spells (limited) */
	if (r_imperatives[imperative].flag == I_ENHA) {
		if (r_types[type].flag == T_SIGN) {
			switch (projection) {
				
				case SV_R_DARK: //invis
				case SV_R_FORC: //reflect
					duration = dice;
				break;
				
				case SV_R_CHAO: //brand
				case SV_R_ELEC:
				case SV_R_FIRE:
				case SV_R_WATE: //regen
				case SV_R_COLD:
				case SV_R_ACID:	
				case SV_R_POIS:
				case SV_R_PLAS:
					duration = duration + dice; //actually dice uses damage %s (future fix? - maximized > lengthened this way) - Kurzel
				break;
				
				default: {
				break; }
			}
		}
	} else {
		if (r_types[type].flag == T_SIGN) {
			switch (projection) {

				case SV_R_INER: //anchor
				case SV_R_FORC: //armor
					duration = dice;
				break;
				
				case SV_R_ELEC: //resist
				case SV_R_FIRE:
				case SV_R_COLD:
				case SV_R_ACID:	
				case SV_R_POIS:
				case SV_R_TIME: //haste
				case SV_R_PLAS:
					duration = duration + dice; //actually dice uses damage %s (future fix? - maximized > lengthened this way) - Kurzel
				break;
				
				default: {
				break; }
			}
		}
	}
		
	if (duration < S_DURATION_MIN) duration = S_DURATION_MIN;
	if (duration > S_DURATION_MAX) duration = S_DURATION_MAX;
	
	return (byte)duration;
}

object_type * rspell_trap(int Ind, byte projection) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	
	byte i;
	for (i = 0; i < INVEN_TOTAL; i++) {
		if (i >= INVEN_PACK) continue;		
		o_ptr = &p_ptr->inventory[i];
		if ((o_ptr->tval == TV_RUNE) && (o_ptr->level || o_ptr->owner == p_ptr->id)) { //Do we own it..?
			if (o_ptr->sval == projection) { //Element match
				return o_ptr;
			}
		}
	}
	
	return NULL;
}

bool rspell_socket(int Ind, byte projection, bool sigil) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	
	byte i;
	for (i = 0; i < INVEN_TOTAL; i++) {
		if (i >= INVEN_PACK) continue;		
		o_ptr = &p_ptr->inventory[i];
		if ((o_ptr->tval == TV_RUNE) && (o_ptr->level || o_ptr->owner == p_ptr->id)) { //Do we own it..?
			if (o_ptr->sval == projection && (sigil || !check_guard_inscription(o_ptr->note, 'R'))) { //Element match, not protected?
				byte amt = 1; //Always consume exactly one!
				char o_name[ONAME_LEN];
				object_desc(Ind, o_name, o_ptr, FALSE, 3);
				msg_format(Ind, "\377o%sour %s (%c) %s consumed!",
					((o_ptr->number > 1) ? ((amt == o_ptr->number) ? "All of y" :
					(amt > 1 ? "Some of y" : "One of y")) : "Y"),
					o_name, index_to_label(i), ((amt > 1) ? "were" : "was"));

				/* Erase a rune from inventory */
				inven_item_increase(Ind, i, -1);
				
				/* Play a sound! */
				sound(Ind, "item_rune", NULL, SFX_TYPE_COMMAND, FALSE);

				/* Clean up inventory */
				inven_item_optimize(Ind, i);

				return TRUE;
			}
		}
	}
	
	return FALSE;
}

bool rspell_sigil(int Ind, byte projection, byte imperative, u16b item) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr = &p_ptr->inventory[item];
	
	/* Restrict new sigils to one per category */
	if (!o_ptr->sigil) {
		/* Store tval for comparison speed */
		byte tval = o_ptr->tval;

		/* Weapon */
		if ((tval == TV_MSTAFF)
		 || (tval == TV_BLUNT)
		 || (tval == TV_POLEARM)
		 || (tval == TV_SWORD)
		 || (tval == TV_AXE)
		 || (tval == TV_BOOMERANG)) {
			if ((p_ptr->inventory[INVEN_WIELD].sigil)
			 || (p_ptr->inventory[INVEN_BOW].sigil)
			 || ((p_ptr->inventory[INVEN_ARM].sigil) && (p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD))) {

				/* Give a message */
				msg_format(Ind, "\377yYou can only maintain a single weapon sigil.");

				return FALSE;
			}
		}
		
		/* Armor */
		if ((tval == TV_BOOTS)
		 || (tval == TV_GLOVES)
		 || (tval == TV_HELM)
		 || (tval == TV_CROWN)
		 || (tval == TV_SHIELD)
		 || (tval == TV_CLOAK)
		 || (tval == TV_SOFT_ARMOR)
		 || (tval == TV_HARD_ARMOR)
		 || (tval == TV_DRAG_ARMOR)) {
			if ((p_ptr->inventory[INVEN_BODY].sigil)
			 || (p_ptr->inventory[INVEN_OUTER].sigil)
			 || ((p_ptr->inventory[INVEN_ARM].sigil) && (p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD))
			 || (p_ptr->inventory[INVEN_HEAD].sigil)
			 || (p_ptr->inventory[INVEN_HANDS].sigil)
			 || (p_ptr->inventory[INVEN_FEET].sigil)) {
			 
				/* Give a message */
				msg_format(Ind, "\377yYou can only maintain a single armor sigil.");
			
				return FALSE;
			}
		}
	}
	
	/* Do we have a rune? */
	if (!rspell_socket(Ind, projection, TRUE)) {
		msg_format(Ind, "\377yYou do not have the required rune. (%s)", r_projections[projection].name);
		return FALSE;
	}
	
	/* Artifact? */
	if (o_ptr->name1) {
		msg_print(Ind, "The artifact resists your attempts!");
		return FALSE;
	}
	
	/* Store the info */
	o_ptr = &p_ptr->inventory[item];
	o_ptr->sigil = projection + 1; //Hack -- 0 is an item WITHOUT a sigil. Subtract 1 later...
	if (r_imperatives[imperative].flag == I_ENHA) o_ptr->sseed = Rand_value; /* Save RNG */

	/* Give a message */
	char o_name[ONAME_LEN];
	object_desc(Ind, o_name, o_ptr, FALSE, 3);	
	msg_format(Ind, "\377sYour %s is emblazoned with %s!", o_name, r_projections[projection].name);

	return TRUE;
}

/** External Functions **/

/*
 * Cast a runespell if able. The main function!
 */
byte execute_rspell(int Ind, byte dir, u16b e_flags, u16b m_flags, u16b item, bool retaliate) {
	player_type *p_ptr = Players[Ind];
	
	/** Activity Update **/
	
	/* Break AFK */
	un_afk_idle(Ind);
	
	/* Break FTK */
	p_ptr->shooting_till_kill = FALSE;
	
	/* Paranoia; toggle states that require inactivity */
	/* Move checks like this into their generating functions please? >_>" */
	/* Or actually leave it in, in case this never happens and adventurers/runies get both features somehow... oO */
	break_cloaking(Ind, 5);
	break_shadow_running(Ind);
	stop_precision(Ind);
	
	/** Decode Runespell **/
	
	/* Elements */
	byte element[RCRAFT_MAX_ELEMENTS];
	byte elements = flags_to_elements(element, e_flags);
	
	/* Projection Element */
	byte projection = flags_to_projection(e_flags);

	/* Imperative and Type */
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
	
	/** Skill Validation **/
	
	/* Level Requirement */
	byte skill = rspell_skill(Ind, element, elements);
	byte level = rspell_level(imperative, type);
	
	/* Check it! */
	s16b diff = rspell_diff(skill, level);
	if (diff < 1) {
		msg_format(Ind, "\377sYou are not skilled enough. (%s %s %s; level: %d)", r_imperatives[imperative].name, r_projections[projection].name, r_types[type].name, diff);
		p_ptr->energy -= level_speed(&p_ptr->wpos);
		return 0;
	}
	
	/** Target Validation **/
	
	/* Energy Requirement */
	s16b energy = rspell_energy(Ind, imperative, type);
	
	/* Handle '+' targetting mode */
	if (dir == 11) {
		get_aim_dir(Ind);
		p_ptr->current_rcraft = 1;
		p_ptr->current_rcraft_e_flags = e_flags;
		p_ptr->current_rcraft_m_flags = m_flags;
		return 1;
	}
	p_ptr->current_rcraft = -1;
	
	/* Handle FTK targetting mode */
	if (p_ptr->shoot_till_kill) {
	
		/* Assume failure */
		p_ptr->shoot_till_kill_rcraft = FALSE;
		
		/* If the spell targets and is instant */
		if ((r_types[type].flag == T_BOLT) || (r_types[type].flag == T_BEAM) || (r_types[type].flag == T_BALL)) {
			
			/* If a new spell, update FTK */
			if (p_ptr->FTK_e_flags != e_flags || p_ptr->FTK_m_flags != m_flags) {
				p_ptr->FTK_e_flags = e_flags;
				p_ptr->FTK_m_flags = m_flags;
				p_ptr->FTK_energy = energy;
			}
			
			/* Cancel if we've lost our target */
			if (!target_okay(Ind)) return 0;
			
			/* Cancel if we're going to automatically wake new monsters */
#ifndef PY_PROJ_ON_WALL
			if (!projectable_real(Ind, p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col, MAX_RANGE))
#else
			if (!projectable_wall_real(Ind, p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col, MAX_RANGE))
#endif
				return 0;
			
			/* Don't FTK if not targetting a monster in FTK mode; but continue and fire the spell! */
			if (dir == 5) {
				p_ptr->shooting_till_kill = TRUE;
				p_ptr->shoot_till_kill_rcraft = TRUE;
			}
		}
	}
	
	/** Caster Validation **/
	
	/* Examine Status */
	byte penalty = 0;
	
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
#ifdef ENABLE_SHELL_ENCHANT
		if (r_types[type].flag != T_ENCH) {
			msg_format(Ind, "\377%cYour anti-magic shell absorbs the spell. (%s %s %s)", COLOUR_AM_OWN, r_imperatives[imperative].name, r_projections[projection].name, r_types[type].name);
			p_ptr->energy -= energy;
			return 0;
		}
#else
		msg_format(Ind, "\377%cYour anti-magic shell absorbs the spell. (%s %s %s)", COLOUR_AM_OWN, r_imperatives[imperative].name, r_projections[projection].name, r_types[type].name);
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
	if ((r_types[type].flag != T_ENCH) && (interfere(Ind, cfg.spell_interfere))) { //Ignore intercept for now (don't waste a rune?)
		p_ptr->energy -= energy;
		return 0;
	}
	
	
	/* Calculate the Cost */
	byte cost = rspell_cost(imperative, type, skill);
	
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
		msg_format(Ind, "\377oYou do not have enough mana. (%s %s %s; cost: %d)", r_imperatives[imperative].name, r_projections[projection].name, r_types[type].name, cost);
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
	msg_3 = " trace the rune-forms.";
#endif
	
#ifdef FEEDBACK_MESSAGE
	/* Resolve the Feedback Message */
	if (penalty) msg_format(Ind, "%s%s%s", msg_1, msg_2, msg_3);
#endif	
	
	/* Calculate Failure Chance */
	byte fail = rspell_fail(Ind, imperative, type, diff, penalty);

	/* Calculate the Damage */
	u32b dx, dy;
	u16b damage = rspell_damage(&dx, &dy, imperative, type, skill, projection);
	u16b dice = damroll(dx, dy);

	/* Calculate the Remaining Parameters */
	byte radius = rspell_radius(imperative, type, skill, projection);
	byte duration = rspell_duration(imperative, type, skill, projection, dice);
	
	/** Cast the Runespell **/
	
	/* Success? Store an indicator of the spell fail margin. (Additionally boost or reduce damage, if the option is enabled). */
	char * msg_q = NULL;
	bool failure = 0;
	s16b margin = randint(100) - fail;
	if (margin < 1) {
		msg_q = "\377rincompetently\377w";
#ifdef ENABLE_SKILLFUL_CASTING
		damage = damage * 8 / 10 + 1;
#endif
		failure = 1;
	}
	else if (margin < 10) { 
		msg_q = "clumsily";
#ifdef ENABLE_SKILLFUL_CASTING
		damage = damage * 9 / 10 + 1;
#endif
	}
	else if (margin < 30) {
		msg_q = "casually";
	}
	else if (margin < 50) {
		msg_q = "effectively";
#ifdef ENABLE_SKILLFUL_CASTING
		damage = damage * 11 / 10 + 1;
#endif
	}
	else {
		msg_q = "elegantly";
#ifdef ENABLE_SKILLFUL_CASTING
		damage = damage * 12 / 10 + 1;
#endif
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
s_printf("dx:   %d\n", dx);
s_printf("dy:   %d\n", dy);
s_printf("Radius:   %d\n", radius);
s_printf("Duration: %d\n", duration);
#endif

	/** Resolve the Runespell **/
	
	/* Set the projection type */
	byte gf_type = r_projections[projection].gf_type;
	
	/* Generic spell message */
	msg_format(Ind, "You %s trace %s %s %s of %s.", msg_q, ((r_imperatives[imperative].flag == I_EXPA) || (r_imperatives[imperative].flag == I_ENHA)) ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, r_projections[projection].name);
	
	switch (r_types[type].flag) {
		
		case T_BOLT: {
			sprintf(p_ptr->attacker, " %s traces %s %s %s of %s for", msg_q, ((r_imperatives[imperative].flag == I_EXPA) || (r_imperatives[imperative].flag == I_ENHA)) ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, r_projections[projection].name);	
			if (r_imperatives[imperative].flag != I_ENHA) fire_bolt(Ind, gf_type, dir, dice, p_ptr->attacker);
			else fire_grid_bolt(Ind, gf_type, dir, dice, p_ptr->attacker);
		break; }
		
		case T_BEAM: {
			sprintf(p_ptr->attacker, " %s traces %s %s %s of %s for", msg_q, ((r_imperatives[imperative].flag == I_EXPA) || (r_imperatives[imperative].flag == I_ENHA)) ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, r_projections[projection].name);	
			if (r_imperatives[imperative].flag != I_ENHA) fire_beam(Ind, gf_type, dir, dice, p_ptr->attacker);
			else fire_wall(Ind, gf_type, dir, damage, duration, 9, p_ptr->attacker);
		break; }
		
		case T_CLOU: {
			sprintf(p_ptr->attacker, " %s traces %s %s %s of %s for", msg_q, ((r_imperatives[imperative].flag == I_EXPA) || (r_imperatives[imperative].flag == I_ENHA)) ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, r_projections[projection].name);	
			if (r_imperatives[imperative].flag != I_ENHA) fire_cloud(Ind, gf_type, dir, damage, radius, duration, 9, p_ptr->attacker);
			else fire_wave(Ind, gf_type, 0, damage*3/2, radius, duration*2, 10, EFF_STORM, p_ptr->attacker);
		break; }
		
		case T_BALL: {
			sprintf(p_ptr->attacker, " %s traces %s %s %s of %s for", msg_q, ((r_imperatives[imperative].flag == I_EXPA) || (r_imperatives[imperative].flag == I_ENHA)) ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, r_projections[projection].name);	
			if (r_imperatives[imperative].flag != I_ENHA) fire_ball(Ind, gf_type, dir, damage, radius, p_ptr->attacker);
			else fire_full_ball(Ind, gf_type, dir, damage, radius, p_ptr->attacker);
		break; }
		
		case T_SIGN: {
			switch (projection) {
			
				case SV_R_LITE: {
					sprintf(p_ptr->attacker, " %s traces %s %s %s of %s for", msg_q, ((r_imperatives[imperative].flag == I_EXPA) || (r_imperatives[imperative].flag == I_ENHA)) ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, r_projections[projection].name);	
					if (r_imperatives[imperative].flag != I_ENHA) lite_area(Ind, damage, radius); 
					else { byte k; for (k = 0; k < 8; k++) lite_line(Ind, ddd[k], dice); } //actually GF_LITE_WEAK
				break; }
				
				case SV_R_DARK: {
					sprintf(p_ptr->attacker, " %s traces %s %s %s of %s for", msg_q, ((r_imperatives[imperative].flag == I_EXPA) || (r_imperatives[imperative].flag == I_ENHA)) ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, r_projections[projection].name);	
					if (r_imperatives[imperative].flag != I_ENHA) unlite_area(Ind, damage, radius); 
					else set_invis(Ind, duration, damage);
				break; }
				
				case SV_R_NEXU: {
					if (r_imperatives[imperative].flag != I_ENHA) teleport_player(Ind, radius, FALSE); //FALSE -> reduced effect in pvp
					else teleport_player(Ind, 200, FALSE); //A full-range, standard-issue teleport.
				break; }
				
				case SV_R_NETH: {
					if (r_imperatives[imperative].flag != I_ENHA) (void)mass_genocide(Ind); //Use a radius in the future?
					else (void)genocide(Ind); //Maybe resist/power in the future as well?
				break; }
				
				case SV_R_CHAO: {
					if (r_imperatives[imperative].flag != I_ENHA) fire_ball(Ind, GF_OLD_POLY, 0, 0, 0, ""); //poly self
					else set_brand(Ind, duration, BRAND_CONF, 0); //chaos brand
				break; }
				
				case SV_R_MANA: {
					if (r_imperatives[imperative].flag != I_ENHA) remove_curse(Ind);
					else remove_all_curse(Ind);
				break; }
				
				case SV_R_CONF: {
					if (r_imperatives[imperative].flag != I_ENHA) { //monster confusion
						msg_print(Ind, "Your hands begin to glow.");
						p_ptr->confusing = TRUE;
					} else recharge(Ind, damage);
				break; }
				
				case SV_R_INER: {
					if (r_imperatives[imperative].flag != I_ENHA) set_st_anchor(Ind, duration); //Future -- expanded, a small area anchor?
					else {
						int ty, tx;//, py = p_ptr->py, px = p_ptr->px;
						if ((dir == 5) && target_okay(Ind)) { /* Use a target -- Fix this in wizard/mkey!! - Kurzel */
							tx = p_ptr->target_col;
							ty = p_ptr->target_row;
							if (player_has_los_bold(Ind, ty, tx)) teleport_player_to(Ind, ty, tx); //Require LoS. (No jumping through walls!)
						}
					}
				break; }
				
				case SV_R_ELEC: {
					if (r_imperatives[imperative].flag != I_ENHA) { 
						set_oppose_elec(Ind, duration);
						if (r_imperatives[imperative].flag == I_EXPA) fire_ball(Ind, GF_RESELEC_PLAYER, 0, duration, RCRAFT_PJ_RADIUS, "");
					} else set_brand(Ind, duration, BRAND_ELEC, 0);
				break; }
				
				case SV_R_FIRE: {
					if (r_imperatives[imperative].flag != I_ENHA) {
						set_oppose_fire(Ind, duration);
						if (r_imperatives[imperative].flag == I_EXPA) fire_ball(Ind, GF_RESFIRE_PLAYER, 0, duration, RCRAFT_PJ_RADIUS, "");
					} else set_brand(Ind, duration, BRAND_FIRE, 0);
				break; }
				
				case SV_R_WATE: {
					if (r_imperatives[imperative].flag != I_ENHA) set_poisoned(Ind, 0, 0);
					else set_tim_regen(Ind, duration, damage);
				break; }
				
				case SV_R_GRAV: {
					if (r_imperatives[imperative].flag != I_ENHA) fire_ball(Ind, GF_TELE_TO, 0, damage, radius, "");
					else {
						int ty, tx;//, py = p_ptr->py, px = p_ptr->px;
						if ((dir == 5) && target_okay(Ind)) { /* Use a target -- Fix this in wizard/mkey!! - Kurzel */
							tx = p_ptr->target_col;
							ty = p_ptr->target_row;
							if (player_has_los_bold(Ind, ty, tx)) teleport_player_to(Ind, ty, tx); //Require LoS. (No jumping through walls!)
					/*	} else { //Use a a direction...(Teleport in a direction until collision or max range. No thru-wall powers!)
							tx = px;
							ty = py;
							switch (dir) { //Kurzel -- Test to ensure spam doesn't overload!
								case 1:
								while (player_has_los_bold(Ind, ty, tx) && distance(py, px, ty, tx) < MAX_RANGE) { ty++; tx--; }
								break;
								case 2:
								while (player_has_los_bold(Ind, ty, tx) && distance(py, px, ty, tx) < MAX_RANGE) ty++;
								break;
								case 3:
								while (player_has_los_bold(Ind, ty, tx) && distance(py, px, ty, tx) < MAX_RANGE) { ty++; tx++; }
								break;
								case 4:
								while (player_has_los_bold(Ind, ty, tx) && distance(py, px, ty, tx) < MAX_RANGE) tx--;
								break;
								case 6:
								while (player_has_los_bold(Ind, ty, tx) && distance(py, px, ty, tx) < MAX_RANGE) tx++;
								break;
								case 7:
								while (player_has_los_bold(Ind, ty, tx) && distance(py, px, ty, tx) < MAX_RANGE) { ty--; tx--; }
								break;
								case 8:
								while (player_has_los_bold(Ind, ty, tx) && distance(py, px, ty, tx) < MAX_RANGE) ty--;
								break;
								case 9:
								while (player_has_los_bold(Ind, ty, tx) && distance(py, px, ty, tx) < MAX_RANGE) { ty--; tx++; }
								break;
								default:
									msg_print(Ind, "Gravity warps around you.");
								break;
							}
							if (ty == p_ptr->py && tx == p_ptr->px) teleport_player(Ind, 5, FALSE);
							else teleport_player_to(Ind, ty, tx);
					*/	}
					}
				break; }
				
				case SV_R_COLD: {
					if (r_imperatives[imperative].flag != I_ENHA) {
						set_oppose_cold(Ind, duration);
						if (r_imperatives[imperative].flag == I_EXPA) fire_ball(Ind, GF_RESCOLD_PLAYER, 0, duration, RCRAFT_PJ_RADIUS, "");
					} else set_brand(Ind, duration, BRAND_COLD, 0);
				break; }
				
				case SV_R_ACID: {
					if (r_imperatives[imperative].flag != I_ENHA) {
						set_oppose_acid(Ind, duration);
						if (r_imperatives[imperative].flag == I_EXPA) fire_ball(Ind, GF_RESACID_PLAYER, 0, duration, RCRAFT_PJ_RADIUS, "");
					} else set_brand(Ind, duration, BRAND_ACID, 0);
				break; }
				
				case SV_R_POIS: {
					if (r_imperatives[imperative].flag != I_ENHA) {
						set_oppose_pois(Ind, duration);
						if (r_imperatives[imperative].flag == I_EXPA) fire_ball(Ind, GF_RESPOIS_PLAYER, 0, duration, RCRAFT_PJ_RADIUS, "");
					} else set_brand(Ind, duration, BRAND_POIS, 0);
					break; }
				
				case SV_R_TIME: {
					if (r_imperatives[imperative].flag != I_ENHA) {
						set_fast(Ind, duration, damage);
						if (r_imperatives[imperative].flag == I_EXPA) fire_ball(Ind, GF_SPEED_PLAYER, 0, damage*2, RCRAFT_PJ_RADIUS, ""); //Hacked -- duration =P same for set_shield, full value when rad 1 (must not hit self!)
					} else project_los(Ind, GF_STASIS, damage, p_ptr->attacker);
				break; }
				
				case SV_R_SOUN: {
					sprintf(p_ptr->attacker, " %s traces %s %s %s of %s for", msg_q, ((r_imperatives[imperative].flag == I_EXPA) || (r_imperatives[imperative].flag == I_ENHA)) ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, r_projections[projection].name);
					if (r_imperatives[imperative].flag != I_ENHA) fire_ball(Ind, GF_KILL_TRAP, 0, damage, radius, p_ptr->attacker);
					else fire_beam(Ind, GF_KILL_TRAP, dir, dice, p_ptr->attacker);
				break; }
				
				case SV_R_SHAR: {
					sprintf(p_ptr->attacker, " %s traces %s %s %s of %s for", msg_q, ((r_imperatives[imperative].flag == I_EXPA) || (r_imperatives[imperative].flag == I_ENHA)) ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, r_projections[projection].name);
					if (r_imperatives[imperative].flag != I_ENHA) earthquake(&p_ptr->wpos, p_ptr->py, p_ptr->px, radius);
					else fire_bolt(Ind, GF_KILL_WALL, dir, dice, p_ptr->attacker);
				break; }
				
				case SV_R_DISE: {
					if (r_imperatives[imperative].flag != I_ENHA) unmagic(Ind);
					else do_cancellation(Ind, 0);
				break; }
				
				case SV_R_FORC: {
					if (r_imperatives[imperative].flag != I_ENHA) {
						set_shield(Ind, duration, damage, SHIELD_NONE, 0, 0);
						if (r_imperatives[imperative].flag == I_EXPA) fire_ball(Ind, GF_SHIELD_PLAYER, 0, damage, RCRAFT_PJ_RADIUS, "");
					} else set_tim_deflect(Ind, duration);
				break; }
				
				case SV_R_PLAS: {
					if (r_imperatives[imperative].flag != I_ENHA) {
						set_oppose_acid(Ind, duration);
						set_oppose_elec(Ind, duration);
						set_oppose_fire(Ind, duration);
						set_oppose_cold(Ind, duration);
						//set_oppose_pois(Ind, duration); //For Holy Resistance ONLY
						if (r_imperatives[imperative].flag == I_EXPA) {
							fire_ball(Ind, GF_RESACID_PLAYER, 0, duration, RCRAFT_PJ_RADIUS, "");
							fire_ball(Ind, GF_RESELEC_PLAYER, 0, duration, RCRAFT_PJ_RADIUS, "");
							fire_ball(Ind, GF_RESFIRE_PLAYER, 0, duration, RCRAFT_PJ_RADIUS, "");
							fire_ball(Ind, GF_RESCOLD_PLAYER, 0, duration, RCRAFT_PJ_RADIUS, "");
							//fire_ball(Ind, GF_RESPOIS_PLAYER, 0, duration, RCRAFT_PJ_RADIUS, "");
						}
					} else set_brand(Ind, duration, BRAND_BASE, 0); //multibrand 'base' - Kurzel
				break; }
				
				default: {
				break; }
			}
		break; }
		
		case T_RUNE: {
			if (r_imperatives[imperative].flag != I_ENHA) warding_rune(Ind, projection, imperative, skill);
			else warding_glyph(Ind);
		break; }
		
		case T_ENCH: {
			rspell_sigil(Ind, projection, imperative, item);
		break; }
		
		case T_WAVE: {
			sprintf(p_ptr->attacker, " %s traces %s %s %s of %s for", msg_q, ((r_imperatives[imperative].flag == I_EXPA) || (r_imperatives[imperative].flag == I_ENHA)) ? "an" : "a", r_imperatives[imperative].name, r_types[type].name, r_projections[projection].name);
			if (r_imperatives[imperative].flag != I_ENHA) fire_wave(Ind, gf_type, 0, damage, 0, duration, 2, EFF_WAVE, p_ptr->attacker);
			else project_los(Ind, gf_type, damage, p_ptr->attacker);
		break; }
		
	}
	
	/** Expend Resources **/
	
	/* Energy */
	p_ptr->energy -= energy;
	
	/* Mana */
	calc_mana(Ind);
	if (p_ptr->csp > cost) p_ptr->csp -= cost;
	else { /* The damage is implied by the "Exhausted, ..." message, so not explicitly stated. */
		take_hit(Ind, (cost-p_ptr->csp), "magical exhaustion", 0);
		p_ptr->csp = 0;
		p_ptr->shooting_till_kill = FALSE;
	}
	p_ptr->redraw |= PR_MANA;

	/** Backlash **/

	/* Failure */
	if (failure)
		if (!rspell_socket(Ind, projection, FALSE)) {
			if (dice > damage) damage = dice; //Get the highest damage part of the spell, could be reworked to match actual spell type (as modified by enhanced). TODO
			project(PROJECTOR_RUNE, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px, damage / 5 + 1, gf_type, (PROJECT_KILL | PROJECT_NORF | PROJECT_JUMP), "");
		}

#ifdef ENABLE_SUSCEPT
	/* Lite Susceptability Check -- Add other suscept checks? ie. Fire for Ents? Disabled for consistancy until this is done. */
	if (!failure && (gf_type == GF_LITE)) {
		if (p_ptr->suscep_lite && !p_ptr->resist_lite && !p_ptr->resist_blind) (void)set_blind(Ind, p_ptr->blind + 4 + randint(10));
		if (p_ptr->suscep_lite && !p_ptr->resist_lite) project(PROJECTOR_RUNE, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px, damage / 5 + 1, GF_LITE, (PROJECT_KILL | PROJECT_NORF), "");
		p_ptr->shooting_till_kill = FALSE;
	}
#endif

	return 1;
}

/*
 * Leave a "rune of warding" which explodes when broken.
 * This otherwise behaves like a normal "glyph of warding".
 * Minimal runecraft parameters are stored.
 */
void warding_rune(int Ind, byte projection, byte imperative, byte skill)
{
	player_type *p_ptr = Players[Ind];
	int y = p_ptr->py, x = p_ptr->px;
	struct worldpos *wpos = &p_ptr->wpos;
	object_type forge;
	object_type *o_ptr = &forge;
	object_type *r_ptr;

	/* Allowed? */
	if ((!allow_terraforming(wpos, FEAT_RUNE) || istown(wpos))
	    && !is_admin(p_ptr))
		return;
	
	/* Emulate cave_set_feat_live but handle cs_ptr too! */
	cave_type **zcave = getcave(wpos);
	cave_type *c_ptr;
	struct c_special *cs_ptr;
	
	/* Boundary check */
	if (!(zcave = getcave(wpos))) return;
	if (!in_bounds(y, x)) return;
	c_ptr = &zcave[y][x];
	
	/* No runes of protection / glyphs of warding on non-empty grids - C. Blue */
	if (!(cave_clean_bold(zcave, y, x)) && /* cave_clean_bold also checks for object absence */
	    ((c_ptr->feat == FEAT_NONE) ||
	    (c_ptr->feat == FEAT_FLOOR) ||
	    (c_ptr->feat == FEAT_DIRT) ||
	    (c_ptr->feat == FEAT_LOOSE_DIRT) || /* used for gardens (fields) in wild.c */
	    (c_ptr->feat == FEAT_CROP) || /* used for gardens (fields) in wild.c */
	    (c_ptr->feat == FEAT_GRASS) ||
	    (c_ptr->feat == FEAT_ICE) ||
	    (c_ptr->feat == FEAT_SAND) ||
	    (c_ptr->feat == FEAT_ASH) ||
	    (c_ptr->feat == FEAT_MUD) ||
	    (c_ptr->feat == FEAT_FLOWER) ||
//	    (c_ptr->feat == FEAT_SHAL_LAVA) || maybe required, waiting for player input (2009/1/14) - C. Blue
	    (c_ptr->feat == FEAT_NETHER_MIST)))
		return;
	
	/* Don't mess with inns please! */
	if (f_info[c_ptr->feat].flags1 & FF1_PROTECTED) return;

	/* 'Disarm' old runes */
	if ((cs_ptr = GetCS(c_ptr, CS_RUNE)))
	{
		/* Restore the original cave feature */
		cave_set_feat_live(wpos, y, x, cs_ptr->sc.rune.feat);
		
		/* Drop rune */
		invcopy(o_ptr, lookup_kind(TV_RUNE, cs_ptr->sc.rune.typ));
		o_ptr->discount = cs_ptr->sc.rune.discount;
		o_ptr->level = cs_ptr->sc.rune.level;
		o_ptr->owner = cs_ptr->sc.rune.id;
		o_ptr->note = cs_ptr->sc.rune.note;
		//drop_near(o_ptr, -1, wpos, y, x);
		inven_carry(Ind, o_ptr); //let's automatically throw it in the pack
		
		/* Combine and update the pack */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);
		
		/* Window stuff */
		p_ptr->window |= (PW_INVEN);
		
		/* Cleanup */
		cs_erase(c_ptr, cs_ptr);
		return;
	}
	
	/* Do we have a rune? */
	r_ptr = rspell_trap(Ind, projection);
	if (!r_ptr) {
		msg_format(Ind, "\377yYou do not have the required rune. (%s)", r_projections[projection].name);
		return;
	}
	
	/* Store the rune */
	byte discount = r_ptr->discount;
	s16b item_level = r_ptr->level;
	s16b note = r_ptr->note;
	
	/* Consume one rune */
	(void)rspell_socket(Ind, projection, TRUE);
	
	/* Allocate memory for a new rune */
	if (!(cs_ptr = AddCS(c_ptr, CS_RUNE))) return;
	
	/* Store the rune */
	cs_ptr->sc.rune.discount = discount;
	cs_ptr->sc.rune.level = item_level;
	cs_ptr->sc.rune.note = note;
	//cs_ptr->sc.rune.id = r_ptr->owner; //use trap setter, not rune owner
	
	/* Preserve Terrain Feature */
	cs_ptr->sc.rune.feat = c_ptr->feat;
	
	/* Store minimal runecraft info */
	cs_ptr->sc.rune.typ = projection;
	cs_ptr->sc.rune.mod = imperative;
	cs_ptr->sc.rune.lev = skill;
	cs_ptr->sc.rune.id = p_ptr->id; 
	
	/* Change the feature */
	if (c_ptr->feat != FEAT_RUNE) c_ptr->info &= ~CAVE_NEST_PIT; /* clear teleport protection for nest grid if it gets changed */
	c_ptr->feat = FEAT_RUNE;
	
	int i;
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];

		/* Only works for players on the level */
		if (!inarea(wpos, &p_ptr->wpos)) continue;

		/* Notice */
		note_spot(i, y, x);

		/* Redraw */
		lite_spot(i, y, x);

		/* Update some things */
		p_ptr->update |= (PU_VIEW | PU_DISTANCE);
	}
}

/*
 * Break a "rune of warding" which explodes when broken.
 * This otherwise behaves like a normal "glyph of warding".
 * Stored parameters are used here.
 */
bool warding_rune_break(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	object_type forge;
	object_type *o_ptr = &forge;
	
	/* Location info */
	int mx = m_ptr->fx;
	int my = m_ptr->fy;

	/* Get the relevant level */
	cave_type **zcave;
	worldpos *wpos = &m_ptr->wpos;
	zcave = getcave(wpos);
	/* Paranoia */
	if (!zcave) return(FALSE);

	/* Get the stored rune info */
	struct c_special *cs_ptr;
	cave_type *c_ptr;
	c_ptr = &zcave[my][mx];

	cs_ptr = GetCS(c_ptr, CS_RUNE);
	/* Paranoia */
	if(!cs_ptr) return(FALSE);

	/* XXX Hack -- Owner online? */
	int i, who = PROJECTOR_MON_TRAP;
	player_type *p_ptr=(player_type*)NULL;
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];
		if (p_ptr->conn == NOT_CONNECTED) continue;
		if (!inarea(&p_ptr->wpos, wpos)) continue;

		/* Check if they are in here */
		if (cs_ptr->sc.rune.id == p_ptr->id) {
			who = i;
			break;
		}
	}

	/* Fire if ready */
	if (who > 0) {
	
		/* Load rune data */
		byte skill = cs_ptr->sc.rune.lev;
		byte imperative = cs_ptr->sc.rune.mod;
		byte projection = cs_ptr->sc.rune.typ;

		/* Calculate the Damage */
		u32b dx, dy;
		u16b damage = rspell_damage(&dx, &dy, imperative, 5, skill, projection); 
		
		/* WRAITHFORM reduces damage/effect! */
		//if (who > 0 && p_ptr->tim_wraith) damage /= 2;

		/* Calculate the Remaining Parameters */
		//byte radius = rspell_radius(imperative, 5, skill, projection);
		byte radius = 0; //Be consistent with player traps, for now.
		
		/* Resolve the Effect */
		byte gf_type = r_projections[projection].gf_type;

		/* Create the Effect */
		project(0 - who, radius, wpos, my, mx, damage, gf_type, (PROJECT_JUMP | PROJECT_NORF | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL), "");

#ifdef USE_SOUND_2010
		/* Sound */
		sound_near_site(my, mx, wpos, 0, "ball", NULL, SFX_TYPE_MISC, FALSE);
#endif
	}

	/* Restore the original cave feature */
	cave_set_feat_live(wpos, my, mx, cs_ptr->sc.rune.feat);
	
	/* Drop rune */
	invcopy(o_ptr, lookup_kind(TV_RUNE, cs_ptr->sc.rune.typ));
	o_ptr->discount = cs_ptr->sc.rune.discount;
	o_ptr->level = cs_ptr->sc.rune.level;
	o_ptr->owner = cs_ptr->sc.rune.id;
	o_ptr->note = cs_ptr->sc.rune.note;
	drop_near(o_ptr, -1, wpos, my, mx);
	
	/* Cleanup */
	cs_erase(c_ptr, cs_ptr);

	/* Return TRUE if m_idx still alive */
	return (zcave[my][mx].m_idx == 0 ? TRUE : FALSE);
}

