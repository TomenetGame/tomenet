/* To handle most of the runecrafting and what not */
#include "angband.h"

/* The basic bulk of the rune handling bizzzzzzz... The design doc was
 * written by Mark (Mark@Dave)
 */

#ifndef ENABLE_RCRAFT

/* cast_rune_spell moved to externs.h - mikaelh */
int do_use_mp(int, int, float);
void cast_rune_spell(int, int);
void transform_level(int, byte, int);

void transform_level(int Ind, byte feat, int chance) { 
	player_type *p_ptr = Players[Ind];
	int x, y;
	struct c_special *cs_ptr;
	struct worldpos *wpos;
	cave_type *c_ptr;
	cave_type **zcave;

	wpos  = &(p_ptr->wpos);
	if (!(zcave = getcave(wpos))) return; 

	/* Don't hurt the main town or surrounding areas */
	if (!allow_terraforming(wpos, feat) || istownarea(wpos, MAX_TOWNAREA)) {
		msg_print(Ind, "\377rYou can't do that here!"); 
		return;
	}
	for (y = 0; y < MAX_HGT; y++) { for (x = 0; x < MAX_WID; x++) {
		if (!in_bounds(y, x)) continue;

		c_ptr = &zcave[y][x];
		
		if (c_ptr->info & CAVE_ICKY) continue; 
		if ((cs_ptr = GetCS(c_ptr, CS_KEYDOOR))) continue; 
		if (cave_valid_bold(zcave, y, x))
		{
			if (magik(chance)) {
				/* Delete the object (if any) */
				delete_object(wpos, y, x, TRUE);
				cave_set_feat_live(&p_ptr->wpos, y, x, feat);
			}
		}
	} /* width */ } /* height */
}

//Returns 0 if successful
//Else, return SP needed
int do_use_mp(int Ind, int mod, float mul) {
	player_type *p_ptr = Players[Ind];
	int cost = RUNE_DMG;

#ifdef ALTERNATE_DMG
	if (get_skill(p_ptr, SKILL_RUNEMASTERY) < RBARRIER) {
		cost = cost*get_skill(p_ptr, SKILL_RUNEMASTERY)/RBARRIER + 1; 
		if (cost > RBARRIER) cost = RBARRIER;
	}
#endif
	switch (mod) {
		/* Beginner's :-) */
		case SV_RUNE2_FIRE:
		case SV_RUNE2_COLD:
		case SV_RUNE2_ACID:
		case SV_RUNE2_ELEC:
		case SV_RUNE2_POIS:
			cost *= RBASIC_COST;
			cost *= mul;  //Further multiply!
			break;
		/* INtermediate */
		case SV_RUNE2_WATER:
		case SV_RUNE2_GRAV:
		case SV_RUNE2_DARK:
		case SV_RUNE2_LITE:
			cost *= RMEDIUM_COST;
			cost *= mul;  //Further multiply!
			break;
		/* Advanced */
		case SV_RUNE2_STONE:
		case SV_RUNE2_ARMAGEDDON:
			cost *= RADVANCE_COST; //I pity the f00l
			cost *= mul;  //Further multiply!
			break;
		default:
			cost = 0;
			break;
	} 

	if (p_ptr->rune_num_of_buffs)
		cost += (1<<(p_ptr->rune_num_of_buffs + 3));

	if (p_ptr->csp < cost) {
		return cost;
	}

	else {
		p_ptr->csp -= cost;
	}

	p_ptr->redraw |= PR_MANA; 
	return 0; 
} 

void cast_rune_spell_header (int Ind, int a, int b) {
	player_type *p_ptr = Players[Ind];

	p_ptr->current_rune1 = a;
	object_type *base = &p_ptr->inventory[p_ptr->current_rune1]; //TV_RUNE1

	p_ptr->current_rune2 = b; 

	if (base->sval == SV_RUNE1_SELF) cast_rune_spell(Ind, 0);
	else get_aim_dir(Ind);
}

void cast_rune_spell (int Ind, int dir) { 
	player_type *p_ptr = Players[Ind];
	int basic_rune = p_ptr->current_rune1;
	int mod_rune = p_ptr->current_rune2;
	p_ptr->current_rune1 = -1;
	p_ptr->current_rune2 = -1;
	object_type *base, *mod;

	/* Consume a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);

	/* Check some conditions */
	if (p_ptr->blind)
	{
		msg_print(Ind, "\377rYou can't see anything.");
		return;
	}
	if (no_lite(Ind))
	{
		msg_print(Ind, "\377rYou find it hard to see your hands without a light");
		return;
	}
	if (p_ptr->confused)
	{
		msg_print(Ind, "\377rYou are too confused!");
		return;
	} 

	base = &p_ptr->inventory[basic_rune]; //TV_RUNE1
	mod  = &p_ptr->inventory[mod_rune];   //TV_RUNE2

	if (!can_use_verbose(Ind, base)) return;
	if (!can_use_verbose(Ind, mod))  return;

	//Sanity checks are done. beep beep.


	/* Always break after use (a small consolation ^^) */
	int chance_to_break = 0; //Awww
	int ability_lvl = get_skill_scale(p_ptr, SKILL_RUNEMASTERY, 50);

	/* elem != 0 iff it's an attacking spell that is already defined
	 * somewhere (e.g., fire_ball, fire_beam, fire_cloud, fire_bolt)
	 * Set it as 0 and handle it on the end, if it has other non con-
	 * ventional effects.
	 */
	int elem 	= 0; 	//What kind?
	char *elem_n	= NULL;
	int rad 	= 3;  	//Radius, if applicable. 
	int notice	= 1;	//Did we cast it?
	char *what = NULL; //What DID we cast? :-o

	//Spam! It's so good it's gone!
//	msg_print(Ind, "\377gYou draw your runes out...");

	switch (mod->sval) { 
		case SV_RUNE2_FIRE: 
			elem = GF_FIRE;
			elem_n = "fire";
			break;
		case SV_RUNE2_COLD:
			elem = GF_COLD;
			elem_n = "cold";
			break; 
		case SV_RUNE2_ACID:
			elem = GF_ACID;
			elem_n = "acid";
			break;
		case SV_RUNE2_ELEC:
			elem = GF_ELEC;
			elem_n = "electric";
			break;
		case SV_RUNE2_POIS:
			elem = GF_POIS;
			elem_n = "poison";
			break; 
		case SV_RUNE2_WATER: 
			elem = GF_VAPOUR;
			elem_n = "water";
			break;
		case SV_RUNE2_GRAV:
			elem = GF_GRAVITY;
			elem_n = "gravity";
			break;
		case SV_RUNE2_DARK:
			elem = GF_DARK;
			elem_n = "darkness";
			break;
		case SV_RUNE2_LITE:
			elem = GF_LITE;
			elem_n = "light"; 
			break;
		case SV_RUNE2_STONE:
			elem = GF_KILL_WALL;
			elem_n = "stone to mud";
			break;
		case SV_RUNE2_ARMAGEDDON:
#if 0
			if (magik(70)) elem = GF_FIRE;
			else
#endif
			elem = GF_METEOR;
			elem_n = "meteor";
			
			break;
		default:
			msg_print(Ind, "\377rTrying to use a non runic item.");
			msg_print(Ind, "\377rOr not yet implemented.");
			return;
			break;
	}

	int rune_dmg = RUNE_DMG;

#ifdef ALTERNATE_DMG
	if (get_skill(p_ptr, SKILL_RUNEMASTERY) < RBARRIER) {
		rune_dmg = rune_dmg*get_skill(p_ptr, SKILL_RUNEMASTERY)/RBARRIER + 1; 
		if (rune_dmg > RBARRIER) rune_dmg = RBARRIER;
	}
#endif

	switch(base->sval) {
		case SV_RUNE1_BOLT: 
			notice = do_use_mp(Ind, mod->sval, RBOLT_BASE);
			if (ability_lvl < RSAFE_BOLT) 
				chance_to_break = RSAFE_BOLT - ability_lvl; 
			if (!notice) { 
				switch(mod->sval) {
					case SV_RUNE2_STONE:
						wall_to_mud(Ind, dir); 
						break; 
					default:
						sprintf(p_ptr->attacker, " fires %s %s bolt for", 
						  (elem_n[0] == 'a' || elem_n[0] == 'e' || elem_n[0] == 'i' || elem_n[0] == 'o' || elem_n[0] == 'u') ? "an" : "a",
						  elem_n
						);
						fire_bolt(Ind, elem, dir, rbolt_dmg(rune_dmg), p_ptr->attacker);
						break;
				}
			} 
			what = "bolt spell";
			break;
		case SV_RUNE1_BEAM:
			if (ability_lvl < RSAFE_BEAM) 
				chance_to_break = RSAFE_BEAM - ability_lvl; 
			notice = do_use_mp(Ind, mod->sval, RBEAM_BASE);
			if (!notice) {
				switch(mod->sval) {
					case SV_RUNE2_STONE:
						wall_to_mud(Ind, dir); 
						wall_to_mud(Ind, dir); 
						wall_to_mud(Ind, dir); 
						wall_to_mud(Ind, dir); 
						wall_to_mud(Ind, dir); 
						wall_to_mud(Ind, dir); 
						break; 
					default: 
						sprintf(p_ptr->attacker, " fires %s %s beam for", 
						  (elem_n[0] == 'a' || elem_n[0] == 'e' || elem_n[0] == 'i' || elem_n[0] == 'o' || elem_n[0] == 'u') ? "an" : "a",
						  elem_n
						);
						fire_beam(Ind, elem, dir, rbeam_dmg(rune_dmg),p_ptr->attacker);
						break;
				}
			}
			what = "beam spell";
			break;
		case SV_RUNE1_BALL:
			if (ability_lvl < RSAFE_BALL) 
				chance_to_break = RSAFE_BALL - ability_lvl; 
			notice = do_use_mp(Ind, mod->sval, RBALL_BASE);
			if (!notice) {
				sprintf(p_ptr->attacker, " fires %s %s ball for", 
				  (elem_n[0] == 'a' || elem_n[0] == 'e' || elem_n[0] == 'i' || elem_n[0] == 'o' || elem_n[0] == 'u') ? "an" : "a",
				  elem_n
				); 
				fire_ball(Ind, elem, dir, rball_dmg(rune_dmg), rad, p_ptr->attacker);
			}
			what = "ball spell";
			break;
		case SV_RUNE1_CLOUD:
			if (ability_lvl < RSAFE_CLOUD) 
				chance_to_break = (RSAFE_CLOUD - ability_lvl) * 2;
			notice = do_use_mp(Ind, mod->sval, RCLOUD_BASE);
			if (!notice) {
				sprintf(p_ptr->attacker, " invokes %s %s cloud for", 
				  (elem_n[0] == 'a' || elem_n[0] == 'e' || elem_n[0] == 'i' || elem_n[0] == 'o' || elem_n[0] == 'u') ? "an" : "a",
				  elem_n
				);
				fire_cloud(Ind, elem, dir, rcloud_dmg(rune_dmg), rad, RCLOUD_DURATION, 10, p_ptr->attacker);
			}
			what = "cloud spell";
			break;
		case SV_RUNE1_SELF:
		{
			//No breakage when drawing on yourself? :-o
			notice =  do_use_mp(Ind, mod->sval, RSELF_BASE);
			if (!notice && p_ptr->pclass == CLASS_RUNEMASTER) {
				switch (mod->sval) {
					/* Basic elemental rune buffs are costless */
					case SV_RUNE2_FIRE:
						set_oppose_fire(Ind, randint(rune_dmg) + 10);
						break;
					case SV_RUNE2_COLD:
						set_oppose_cold(Ind, randint(rune_dmg) + 10);
						break;
					case SV_RUNE2_ACID:
						set_oppose_acid(Ind, randint(rune_dmg) + 10);
						break;
					case SV_RUNE2_ELEC:
						set_oppose_elec(Ind, randint(rune_dmg) + 10);
						break;
					case SV_RUNE2_POIS:
						set_oppose_pois(Ind, randint(rune_dmg) + 10); 
						break;
					case SV_RUNE2_WATER:
						if (!p_ptr->suscep_life)
							set_food(Ind, PY_FOOD_MAX - 1);
						break; 
					/* Speed BUff */
					case SV_RUNE2_GRAV:
						/* Unset */
						if (p_ptr->rune_speed) {
							p_ptr->rune_speed = 0;
							p_ptr->rune_num_of_buffs--; 
							what = "Anti Speed"; 
						/* Set */
						} else {
							p_ptr->rune_speed = 7;
							p_ptr->rune_num_of_buffs++;
							what = "Speed Spell";
						}
						p_ptr->redraw |= PR_SPEED;
						break; 
					/* Stealth Buff */
					case SV_RUNE2_DARK:
						/* Unset */
						if (p_ptr->rune_stealth) {
							p_ptr->rune_num_of_buffs--; 
							what = "Anti Stealth";
						/* Set */
						} else {
							p_ptr->rune_stealth = 7;
							p_ptr->rune_num_of_buffs++;
							what = "Stealth Spell";
						}
						p_ptr->redraw |= PR_EXTRA;
						break; 
					/* IV Buff */
					case SV_RUNE2_LITE:
						/* Unset */
						if (p_ptr->rune_IV) {
							p_ptr->rune_IV = 0;
							p_ptr->rune_num_of_buffs--; 
							what = "Half Infravision spell";
						/* Set */
						} else {
							p_ptr->rune_IV = 7;
							p_ptr->rune_num_of_buffs++;
							what = "Boost Infravision spell";
						}
						p_ptr->redraw |= PR_EXTRA;
						break; 
					case SV_RUNE2_STONE: 
						what = "Mad Earthquake"; 
						transform_level(Ind, FEAT_FLOOR, rune_dmg);
						p_ptr->redraw |= PR_MAP;
						break;
					case SV_RUNE2_ARMAGEDDON: 
						what = "Highway to Hell";
						if (magik(50)) transform_level(Ind, FEAT_QUARTZ, rune_dmg);
						else transform_level(Ind, FEAT_MAGMA, rune_dmg);
						p_ptr->redraw |= PR_MAP;
						break;
					default: 
						break; 
				} //switch(mod->pval) 
			} 
			break;
		}//CASE SV_RUNE1_SELF:
		default:
			msg_print(Ind, "\377rTrying to use a non runic item.");
			return;
			break;
	} 

	//Failed to cast-- out of SP!
	if (notice) {
		msg_format(Ind, "\377rYou don't have the energy: %d/%d. Needed: %d", 
				p_ptr->csp, p_ptr->msp, notice);
		return;
#ifdef ALLOW_PERFECT_RUNE_CASTING
	} else if (magik(chance_to_break)) {
#else
	} else if (magik(1) && magik(10)) {
#endif
		msg_print(Ind, "\377rThe rune cracks and becomes unusable.");
		/* Break me! */ 
		inven_item_increase(Ind, mod_rune, -1); //Only the mod... 
		inven_item_describe(Ind, mod_rune);
		inven_item_optimize(Ind, mod_rune);
		return;
	} else {
		if (what) msg_format(Ind, "\377gYou cast \377U%s\377g gracefully.", what);
		return;
	} 
	return;
}

#endif /* ENABLE_RCRAFT */
