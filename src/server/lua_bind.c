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

#if 0
/*
 * Get a new magic type
 */
magic_power *new_magic_power(int num)
{
	magic_power *m_ptr;
	C_MAKE(m_ptr, num, magic_power);
	return (m_ptr);
}
magic_power *grab_magic_power(magic_power *m_ptr, int num)
{
	return (&m_ptr[num]);
}
static char *magic_power_info_lua_fct;
static void magic_power_info_lua(char *p, int power)
{
	int oldtop = lua_gettop(L);

	lua_getglobal(L, magic_power_info_lua_fct);
	tolua_pushnumber(L, power);
	lua_call(L, 1, 1);
	strcpy(p, lua_tostring(L, -1));
	lua_settop(L, oldtop);
}
int get_magic_power_lua(int *sn, magic_power *powers, int max_powers, char *info_fct, int plev, int cast_stat)
{
	magic_power_info_lua_fct = info_fct;
	return (get_magic_power(sn, powers, max_powers, magic_power_info_lua, plev, cast_stat));
}

bool lua_spell_success(magic_power *spell, int stat, char *oups_fct)
{
	int             chance;
	int             minfail = 0;

	/* Spell failure chance */
	chance = spell->fail;

	/* Reduce failure rate by "effective" level adjustment */
	chance -= 3 * (p_ptr->lev - spell->min_lev);

	/* Reduce failure rate by INT/WIS adjustment */
        chance -= 3 * (adj_mag_stat[p_ptr->stat_ind[stat]] - 1);

	/* Not enough mana to cast */
	if (spell->mana_cost > p_ptr->csp)
	{
		chance += 5 * (spell->mana_cost - p_ptr->csp);
	}

	/* Extract the minimum failure rate */
        minfail = adj_mag_fail[p_ptr->stat_ind[stat]];

	/* Minimum failure rate */
	if (chance < minfail) chance = minfail;

	/* Stunning makes spells harder */
	if (p_ptr->stun > 50) chance += 25;
	else if (p_ptr->stun) chance += 15;

	/* Always a 5 percent chance of working */
	if (chance > 95) chance = 95;

	/* Failed spell */
	if (rand_int(100) < chance)
	{
		if (flush_failure) flush();
		msg_format("You failed to concentrate hard enough!");
		sound(SOUND_FAIL);

		if (oups_fct != NULL)
			exec_lua(format("%s(%d)", oups_fct, chance));
		return (FALSE);
	}
	return (TRUE);
}

/*
 * Create objects
 */
object_type *new_object()
{
	object_type *o_ptr;
	MAKE(o_ptr, object_type);
	return (o_ptr);
}

void end_object(object_type *o_ptr)
{
	FREE(o_ptr, object_type);
}

/*
 * Powers
 */
s16b    add_new_power(cptr name, cptr desc, cptr gain, cptr lose, byte level, byte cost, byte stat, byte diff)
{
	/* Increase the size */
	reinit_powers_type(power_max + 1);

	/* Copy the strings */
	C_MAKE(powers_type[power_max - 1].name, strlen(name) + 1, char);
	strcpy(powers_type[power_max - 1].name, name);
	C_MAKE(powers_type[power_max - 1].desc_text, strlen(desc) + 1, char);
	strcpy(powers_type[power_max - 1].desc_text, desc);
	C_MAKE(powers_type[power_max - 1].gain_text, strlen(gain) + 1, char);
	strcpy(powers_type[power_max - 1].gain_text, gain);
	C_MAKE(powers_type[power_max - 1].lose_text, strlen(lose) + 1, char);
	strcpy(powers_type[power_max - 1].lose_text, lose);

	/* Copy the other stuff */
	powers_type[power_max - 1].level = level;
	powers_type[power_max - 1].cost = cost;
	powers_type[power_max - 1].stat = stat;
	powers_type[power_max - 1].diff = diff;

	return (power_max - 1);
}

static char *lua_item_tester_fct;
static bool lua_item_tester(object_type* o_ptr)
{
	int oldtop = lua_gettop(L);
	bool ret;

	lua_getglobal(L, lua_item_tester_fct);
	tolua_pushusertype(L, o_ptr, tolua_tag(L, "object_type"));
	lua_call(L, 1, 1);
	ret = lua_tonumber(L, -1);
	lua_settop(L, oldtop);
	return (ret);
}

void    lua_set_item_tester(int tval, char *fct)
{
	if (tval)
	{
		item_tester_tval = tval;
	}
	else
	{
		lua_item_tester_fct = fct;
		item_tester_hook = lua_item_tester;
	}
}

char *lua_object_desc(object_type *o_ptr, int pref, int mode)
{
	static char buf[150];

	object_desc(buf, o_ptr, pref, mode);
	return (buf);
}

/*
 * Monsters
 */

void find_position(int y, int x, int *yy, int *xx)
{
	int attempts = 500;

	do
	{
		scatter(yy, xx, y, x, 6, 0);
	}
	while (!(in_bounds(*yy, *xx) && cave_floor_bold(*yy, *xx)) && --attempts);
}

static char *summon_lua_okay_fct;
bool summon_lua_okay(int r_idx)
{
	int oldtop = lua_gettop(L);
	bool ret;

	lua_getglobal(L, lua_item_tester_fct);
	tolua_pushnumber(L, r_idx);
	lua_call(L, 1, 1);
	ret = lua_tonumber(L, -1);
	lua_settop(L, oldtop);
	return (ret);
}

bool lua_summon_monster(int y, int x, int lev, bool friend, char *fct)
{
	summon_lua_okay_fct = fct;

	if (!friend)
		return summon_specific(y, x, lev, SUMMON_LUA, 1, 0);
	else
		return summon_specific_friendly(y, x, lev, SUMMON_LUA, TRUE);
}

/*
 * Quests
 */
s16b    add_new_quest(char *name)
{
	int i;

	/* Increase the size */
	reinit_quests(max_q_idx + 1);
	quest[max_q_idx - 1].type = HOOK_TYPE_LUA;
	strncpy(quest[max_q_idx - 1].name, name, 39);

	for (i = 0; i < 10; i++)
		strncpy(quest[max_q_idx - 1].desc[i], "", 39);

	return (max_q_idx - 1);
}

void    desc_quest(int q_idx, int d, char *desc)
{
	if (d >= 0 && d < 10)
		strncpy(quest[q_idx].desc[d], desc, 79);
}

/*
 * Misc
 */
bool    get_com_lua(cptr prompt, int *com)
{
        char c;

        if (!get_com(prompt, &c)) return (FALSE);
        *com = c;
        return (TRUE);
}
#endif	// 0

/* Spell schools */
s16b new_school(int i, cptr name, s16b skill)
{
        schools[i].name = string_make(name);
        schools[i].skill = skill;
        return (i);
}

s16b new_spell(int i, cptr name)
{
        school_spells[i].name = string_make(name);
        school_spells[i].level = 0;
        school_spells[i].level = 0;
        return (i);
}

spell_type *grab_spell_type(s16b num)
{
	return (&school_spells[num]);
}

school_type *grab_school_type(s16b num)
{
	return (&schools[num]);
}

/* Change this fct if I want to switch to learnable spells */
s32b lua_get_level(int Ind, s32b s, s32b lvl, s32b max, s32b min, s32b bonus)
{
	player_type *p_ptr = Players[Ind];
        s32b tmp;

        tmp = lvl - ((school_spells[s].skill_level - 1) * (SKILL_STEP / 10));
        lvl = (tmp * (max * (SKILL_STEP / 10)) / (SKILL_MAX / 10)) / (SKILL_STEP / 10);
        if (lvl < min) lvl = min;
        else if (lvl > 0)
        {
//                tmp += p_ptr->to_s * (SKILL_STEP / 10);
                tmp += bonus;
//                tmp += (get_skill_scale(p_ptr, SKILL_SPELL, 20) * (SKILL_STEP / 10));
//                tmp /= 100; tmp *= (100 + (get_skill_scale(p_ptr, SKILL_SPELL, 40) * (SKILL_STEP / 10)));
                lvl = (tmp * (max * (SKILL_STEP / 10)) / (SKILL_MAX / 10)) / (SKILL_STEP / 10);
		lvl *= (100 + get_skill_scale(p_ptr, SKILL_SPELL, 40));
		lvl /= 100;
        }
        return lvl;
}

/* adj_mag_stat? stat_ind??  pfft */
//s32b lua_spell_chance(s32b chance, int level, int skill_level, int mana, int cur_mana, int stat)
s32b lua_spell_chance(int i, s32b chance, int level, int skill_level, int mana, int cur_mana, int stat)
{
	player_type *p_ptr = Players[i];
        int             minfail;
//DEBUG:s_printf("chance %d - level %d - skill_level %d", chance, level, skill_level);
	/* correct LUA overflow bug */
	if (chance >= 156) chance -= 256;

	/* Reduce failure rate by "effective" level adjustment */
        chance -= 3 * (level - skill_level);

	/* Reduce failure rate by INT/WIS adjustment */
        chance -= 3 * (adj_mag_stat[p_ptr->stat_ind[stat]] - 1);

	 /* Not enough mana to cast */
        if (chance < 0) chance = 0;
        if (mana > cur_mana)
	{
                chance += 15 * (mana - cur_mana);
	}

	/* Extract the minimum failure rate */
        minfail = adj_mag_fail[p_ptr->stat_ind[stat]];

#if 0	// disabled for the time being
	/*
         * Non mage characters never get too good
	 */
	if (!(PRACE_FLAG(PR1_ZERO_FAIL)))
	{
		if (minfail < 5) minfail = 5;
	}

	/* Hack -- Priest prayer penalty for "edged" weapons  -DGK */
	if ((forbid_non_blessed()) && (p_ptr->icky_wield)) chance += 25;
#endif	// 0

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
cave_type *lua_get_cave(int y, int x)
{
        return (&(cave[y][x]));
}

void set_target(int y, int x)
{
        target_who = -1;
        target_col = x;
        target_row = y;
}

/* Level gen */
void get_map_size(char *name, int *ysize, int *xsize)
{
        *xsize = 0;
	*ysize = 0;
	init_flags = INIT_GET_SIZE;
	process_dungeon_file_full = TRUE;
	process_dungeon_file(name, ysize, xsize, cur_hgt, cur_wid, TRUE);
	process_dungeon_file_full = FALSE;

}

void load_map(char *name, int *y, int *x)
{
	/* Set the correct monster hook */
	set_mon_num_hook();

	/* Prepare allocation table */
	get_mon_num_prep();

	init_flags = INIT_CREATE_DUNGEON;
	process_dungeon_file_full = TRUE;
	process_dungeon_file(name, y, x, cur_hgt, cur_wid, TRUE);
	process_dungeon_file_full = FALSE;
}

bool alloc_room(int by0, int bx0, int ysize, int xsize, int *y1, int *x1, int *y2, int *x2)
{
        int xval, yval, x, y;

	/* Try to allocate space for room.  If fails, exit */
	if (!room_alloc(xsize + 2, ysize + 2, FALSE, by0, bx0, &xval, &yval)) return FALSE;

	/* Get corner values */
	*y1 = yval - ysize / 2;
	*x1 = xval - xsize / 2;
	*y2 = yval + (ysize) / 2;
	*x2 = xval + (xsize) / 2;

	/* Place a full floor under the room */
	for (y = *y1 - 1; y <= *y2 + 1; y++)
	{
		for (x = *x1 - 1; x <= *x2 + 1; x++)
		{
			cave_type *c_ptr = &cave[y][x];
			cave_set_feat(y, x, floor_type[rand_int(100)]);
			c_ptr->info |= (CAVE_ROOM);
			c_ptr->info |= (CAVE_GLOW);
		}
        }
        return TRUE;
}


/* Files */
void lua_print_hook(cptr str)
{
        fprintf(hook_file, str);
}


/*
 * Finds a good random bounty monster
 * Im too lazy to write it in lua since the lua API for monsters is not very well yet
 */

/*
 * Hook for bounty monster selection.
 */
static bool lua_mon_hook_bounty(int r_idx)
{
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

        /* Forbid C. Blue's monsters */
        if (r_ptr->flags8 & RF8_BLUEBAND) return (FALSE);

        /* Accept only monsters that are not good */
        if (r_ptr->flags3 & RF3_GOOD) return (FALSE);

	/* The rest are acceptable */
	return (TRUE);
}

int lua_get_new_bounty_monster(int lev)
{
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

/* To do some connection magik ! */
void remote_update_lua(int Ind, cptr file)
{
        player_type *p_ptr = Players[Ind];

        remote_update(p_ptr->conn, file);
	return;
}

void lua_s_print(cptr logstr) {
	s_printf(logstr);
	return;
}

void lua_add_anote(cptr anote) {
	int i;
	for (i = 0; i < MAX_ADMINNOTES; i++)
	        if (!strcmp(admin_note[i], "")) break;
	if (i < MAX_ADMINNOTES) strcpy(admin_note[i], anote);
	return;
}

void lua_count_houses(int Ind) {
	int i;
	Players[Ind]->houses_owned = 0;
	for (i = 0; i < num_houses; i++)
		if ((houses[i].dna->owner_type==OT_PLAYER) &&
    		    (houses[i].dna->owner == Players[Ind]->id))
	    	    Players[Ind]->houses_owned++;
	return;
}

void lua_recalc_char(int Ind) {
	player_type *p_ptr = Players[Ind];
	int             i, j, min_value, max_value;
        int tries = 300;

	/* Experience factor */
/* This one is too harsh for TLs and too easy on yeeks
	p_ptr->expfact = p_ptr->rp_ptr->r_exp * (100 + p_ptr->cp_ptr->c_exp) / 100;
Commented out this totally, since ppl won't get skill points for these 'fake level ups'! :/
      p_ptr->expfact = p_ptr->rp_ptr->r_exp + p_ptr->cp_ptr->c_exp; */

	/* Hitdice */
	p_ptr->hitdie = p_ptr->rp_ptr->r_mhp + p_ptr->cp_ptr->c_mhp;

	/* Minimum hitpoints at highest level */
        min_value = (PY_MAX_LEVEL * (p_ptr->hitdie - 1) * 3) / 8;
	min_value += PY_MAX_LEVEL;

	/* Maximum hitpoints at highest level */
	max_value = (PY_MAX_LEVEL * (p_ptr->hitdie - 1) * 5) / 8;
	max_value += PY_MAX_LEVEL;

	/* Pre-calculate level 1 hitdice */
	p_ptr->player_hp[0] = p_ptr->hitdie;

	/* Roll out the hitpoints */
	while (--tries)
	{
		/* Roll the hitpoint values */
		for (i = 1; i < PY_MAX_LEVEL; i++)
		{
		        j = randint(p_ptr->hitdie);
		        p_ptr->player_hp[i] = p_ptr->player_hp[i-1] + j;
		}
		/* XXX Could also require acceptable "mid-level" hitpoints */
		
		/* Require "valid" hitpoints at highest level */
		if (p_ptr->player_hp[PY_MAX_LEVEL-1] < min_value) continue;
		if (p_ptr->player_hp[PY_MAX_LEVEL-1] > max_value) continue;
		
		/* Acceptable */
		break;
	}
}

void lua_examine_item(int Ind, cptr name, int item) {
	int i = name_lookup_loose(Ind, name, FALSE);
	if (!i) return;
	identify_fully_aux(Ind, &Players[i]->inventory[item]);
	return;
}

void lua_determine_level_req(int Ind, int item) {
	/* -9999 is a fixed hack value, see object2.c, determine_level_req.
	   It means: assume that finding-depth is same as object-base-level,
	   for smooth and easy calculation. */
	determine_level_req(-9999, &Players[Ind]->inventory[item]);
	return;
}
