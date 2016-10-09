/* $Id$ */
/* File: lua_bind.c */

/* Purpose: various lua bindings */

/*
 * Copyright (c) 2001 DarkGod
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "tolua.h"

extern lua_State* L;

/* items */
static char *lua_item_tester_fct;
static bool lua_item_tester(object_type* o_ptr) {
	int oldtop = lua_gettop(L);
	bool ret;

	lua_getglobal(L, lua_item_tester_fct);
	tolua_pushusertype(L, o_ptr, tolua_tag(L, "object_type"));
	lua_call(L, 1, 1);
	ret = lua_tonumber(L, -1);
	lua_settop(L, oldtop);
	return (ret);
}

void lua_set_item_tester(int tval, char *fct) {
	if (tval) item_tester_tval = tval;
	else {
		lua_item_tester_fct = fct;
		item_tester_hook = lua_item_tester;
	}
}

/* Spell schools */
s16b new_school(int i, cptr name, s16b skill) {
	schools[i].name = string_make(name);
	schools[i].skill = skill;
	return (i);
}

s16b new_spell(int i, cptr name) {
	school_spells[i].name = string_make(name);
	school_spells[i].level = 0;
	school_spells[i].level = 0;
	return (i);
}

spell_type *grab_spell_type(s16b num) {
	return (&school_spells[num]);
}

school_type *grab_school_type(s16b num) {
	return (&schools[num]);
}

/* Change this fct if I want to switch to learnable spells */
/* NOTE: KEEP CONSISTENT WITH SERVER-SIDE lua_bind.c:lua_get_level()! */
s32b lua_get_level(int Ind, s32b s, s32b lvl, s32b max, s32b min, s32b bonus) {
	(void) Ind; /* suppress compiler warning */
	s32b tmp;

#ifdef FIX_LUA_GET_LEVEL
	/* Improved version with the following changes:
	 * - Fixed spell power calculations so that they have an effect with small values of 'max'.
	 * - Higher resolution for spell-power calculations.
	 * - Fixed LIMIT_SPELLS
	 */
	tmp = lvl - ((school_spells[s].skill_level - 1) * (SKILL_STEP / 10));

	/* Require at least one spell level before applying bonus */
	if (tmp >= (SKILL_STEP / 10)) {
		/* Applying spell power bonus here makes it work for small values of 'max' */
		if (school_spells[s].spell_power) {
			tmp += tmp * get_skill_scale(p_ptr, SKILL_SPELL, 4000) / 10000;
		}

		/* Currently not used... */
		tmp += bonus;
	}

 #ifdef LIMIT_SPELLS
	if (hack_force_spell_level > 0) {
		s32b tmp_limit = hack_force_spell_level * (SKILL_STEP / 10);
		if (tmp > tmp_limit) tmp = tmp_limit;
	}
 #endif

	tmp = (tmp * (max * (SKILL_STEP / 10)) / (SKILL_MAX / 10));

	if (tmp < 0) /* Shift all negative values, so they map to appropriate integer */
		tmp -= SKILL_STEP / 10 - 1;

	/* Now, we can safely divide */
	lvl = tmp / (SKILL_STEP / 10);

	if (lvl < min)
		lvl = min;
#else
	tmp = lvl - ((school_spells[s].skill_level - 1) * (SKILL_STEP / 10));
	lvl = (tmp * (max * (SKILL_STEP / 10)) / (SKILL_MAX / 10)) / (SKILL_STEP / 10);

	//hack to fix rounding-down, for correctly displaying spell levels <= 0:
	if (tmp < 0 && tmp % 100) lvl--;

	if (lvl < min) lvl = min;
	else if (lvl > 0) {
		tmp += bonus;
		lvl = (tmp * (max * (SKILL_STEP / 10)) / (SKILL_MAX / 10)) / (SKILL_STEP / 10);
		if (school_spells[s].spell_power) {
			lvl *= (100 + get_skill_scale(p_ptr, SKILL_SPELL, 40));
			lvl /= 100;
		}
	}

 #ifdef LIMIT_SPELLS
	if (hack_force_spell_level > 0 && lvl > hack_force_spell_level) lvl = hack_force_spell_level;
 #endif
#endif

	return lvl;
}

/* adj_mag_stat? stat_ind??  pfft */
/* NOTE: KEEP CONSISTENT WITH SERVER-SIDE lua_bind.c:lua_spell_chance()! */
s32b lua_spell_chance(int i, s32b chance, int level, int skill_level, int mana, int cur_mana, int stat) {
	(void) i; /* suppress compiler warning */
	(void) mana; /* suppress compiler warning */
	(void) cur_mana; /* suppress compiler warning */
	int minfail;

	/* correct LUA overflow bug */
	if (chance > 100) chance -= 256;

	/* Reduce failure rate by "effective" level adjustment */
	chance -= 3 * (level - skill_level);

	/* Reduce failure rate by INT/WIS adjustment */
	chance -= adj_mag_stat[p_ptr->stat_ind[stat]] - 3;

	 /* Not enough mana to cast */
	if (chance < 0) chance = 0;

#if 0 /* just confuses and casting without mana is disabled anyway */
	if (mana > cur_mana) chance += 15 * (mana - cur_mana);
#endif

	/* Extract the minimum failure rate */
	minfail = adj_mag_fail[p_ptr->stat_ind[stat]];

#if 0	/* disabled for the time being */
	/*
	 * Non mage characters never get too good
	 */
	if (!(PRACE_FLAG(PR1_ZERO_FAIL))) {
		if (minfail < 5) minfail = 5;
	}

	/* Hack -- Priest prayer penalty for "edged" weapons  -DGK */
	if ((forbid_non_blessed()) && (p_ptr->icky_wield)) chance += 25;
#endif

	/* Minimum failure rate */
	if (chance < minfail) chance = minfail;

	/* Stunning makes spells harder */
	if (p_ptr->stun > 50) chance += 25;
	else if (p_ptr->stun) chance += 15;

	/* Always a 5 percent chance of working */
	if (chance > 95) chance = 95;

	/* Return the chance */
	return (chance);
}

#if 0
/* Cave */
cave_type *lua_get_cave(int y, int x) {
	return (&(cave[y][x]));
}

void set_target(int y, int x) {
	target_who = -1;
	target_col = x;
	target_row = y;
}

/* Level gen */
void get_map_size(char *name, int *ysize, int *xsize) {
	*xsize = 0;
	*ysize = 0;
	init_flags = INIT_GET_SIZE;
	process_dungeon_file_full = TRUE;
	process_dungeon_file(name, ysize, xsize, cur_hgt, cur_wid, TRUE);
	process_dungeon_file_full = FALSE;

}

void load_map(char *name, int *y, int *x) {
	/* Set the correct monster hook */
	set_mon_num_hook();

	/* Prepare allocation table */
	get_mon_num_prep();

	init_flags = INIT_CREATE_DUNGEON;
	process_dungeon_file_full = TRUE;
	process_dungeon_file(name, y, x, cur_hgt, cur_wid, TRUE);
	process_dungeon_file_full = FALSE;
}

bool alloc_room(int by0, int bx0, int ysize, int xsize, int *y1, int *x1, int *y2, int *x2) {
	int xval, yval, x, y;

	/* Try to allocate space for room.  If fails, exit */
	if (!room_alloc(xsize + 2, ysize + 2, FALSE, by0, bx0, &xval, &yval)) return FALSE;

	/* Get corner values */
	*y1 = yval - ysize / 2;
	*x1 = xval - xsize / 2;
	*y2 = yval + (ysize) / 2;
	*x2 = xval + (xsize) / 2;

	/* Place a full floor under the room */
	for (y = *y1 - 1; y <= *y2 + 1; y++) {
		for (x = *x1 - 1; x <= *x2 + 1; x++) {
			cave_type *c_ptr = &cave[y][x];
			cave_set_feat(y, x, floor_type[rand_int(100)]);
			c_ptr->info |= (CAVE_ROOM);
			c_ptr->info |= (CAVE_GLOW);
		}
	}
	return TRUE;
}


/* Files */
void lua_print_hook(cptr str) {
	fprintf(hook_file, str);
}


/*
 * Finds a good random bounty monster
 * Im too lazy to write it in lua since the lua API for monsters is not very well yet
 */

/*
 * Hook for bounty monster selection.
 */
static bool lua_mon_hook_bounty(int r_idx) {
	monster_race* r_ptr = &r_info[r_idx];

	/* Reject uniques */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* Reject those who cannot leave anything */
	if (!(r_ptr->flags9 & RF9_DROP_CORPSE)) return (FALSE);

	/* Accept only monsters that can be generated */
	if (r_ptr->flags9 & RF9_SPECIAL_GENE) return (FALSE);
	if (r_ptr->flags9 & RF9_NEVER_GENE) return (FALSE);

	/* Reject pets */
	if (r_ptr->flags7 & RF7_PET) return (FALSE);

	/* Reject friendly creatures */
	if (r_ptr->flags7 & RF7_FRIENDLY) return (FALSE);

	/* Accept only monsters that are not breeders */
	if (r_ptr->flags4 & RF4_MULTIPLY) return (FALSE);

	/* Forbid joke monsters */
	if (r_ptr->flags8 & RF8_JOKEANGBAND) return (FALSE);

	/* Accept only monsters that are not good */
	if (r_ptr->flags3 & RF3_GOOD) return (FALSE);

	/* The rest are acceptable */
	return (TRUE);
}

int lua_get_new_bounty_monster(int lev) {
	int r_idx;

	/*
	 * Set up the hooks -- no bounties on uniques or monsters
	 * with no corpses
	 */
	get_mon_num_hook = lua_mon_hook_bounty;
	get_mon_num_prep();

	/* Set up the quest monster. */
	r_idx = get_mon_num(lev);

	/* Undo the filters */
	get_mon_num_hook = NULL;
	get_mon_num_prep();

	return r_idx;
}

#endif

int get_inven_sval(int Ind, int inven_slot) {
	(void) Ind; /* suppress compiler warning */
	return (inventory[inven_slot].sval);
}
int get_inven_pval(int Ind, int inven_slot) {
	(void) Ind; /* suppress compiler warning */
	return (inventory[inven_slot].pval);
}
s16b get_inven_xtra(int Ind, int inven_slot, int n) {
	(void) Ind; /* suppress compiler warning */
	/* browsing item in a store? */
	if (inven_slot < 0) {
		switch (n) {
		case 1: return (store.stock[(-inven_slot) - 1].xtra1);
		case 2: return (store.stock[(-inven_slot) - 1].xtra2);
		case 3: return (store.stock[(-inven_slot) - 1].xtra3);
		case 4: return (store.stock[(-inven_slot) - 1].xtra4);
		case 5: return (store.stock[(-inven_slot) - 1].xtra5);
		case 6: return (store.stock[(-inven_slot) - 1].xtra6);
		case 7: return (store.stock[(-inven_slot) - 1].xtra7);
		case 8: return (store.stock[(-inven_slot) - 1].xtra8);
		case 9: return (store.stock[(-inven_slot) - 1].xtra9);
		default: return (0); //failure
		}
	} else  { /* browsing item in inventory */
		switch (n) {
		case 1: return (inventory[inven_slot].xtra1);
		case 2: return (inventory[inven_slot].xtra2);
		case 3: return (inventory[inven_slot].xtra3);
		case 4: return (inventory[inven_slot].xtra4);
		case 5: return (inventory[inven_slot].xtra5);
		case 6: return (inventory[inven_slot].xtra6);
		case 7: return (inventory[inven_slot].xtra7);
		case 8: return (inventory[inven_slot].xtra8);
		case 9: return (inventory[inven_slot].xtra9);
		default: return (0); //failure
		}
	}
}

/* lua wrapper for c_get_item that feeds the arguments to c_get_item correctly */
bool get_item_aux(int *cp, cptr pmt, bool equip, bool inven, bool floor) {
	int mode = 0;

	if (equip) mode |= USE_EQUIP;
	if (inven) mode |= USE_INVEN;
	if (floor) mode |= USE_FLOOR;

	return c_get_item(cp, pmt, mode);
}
