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
#include "lua.h"
#include "tolua.h"
extern lua_State *L;

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
int get_magic_power_lua(int *sn, magic_power *powers, int max_powers, char *info_fct)
{
        magic_power_info_lua_fct = info_fct;
        return (get_magic_power(sn, powers, max_powers, magic_power_info_lua));
}

bool lua_spell_success(magic_power *spell, char *oups_fct)
{
	int             chance;
        int             minfail = 0;

	/* Spell failure chance */
	chance = spell->fail;

	/* Reduce failure rate by "effective" level adjustment */
	chance -= 3 * (p_ptr->lev - spell->min_lev);

	/* Reduce failure rate by INT/WIS adjustment */
	chance -= 3 * (adj_mag_stat[p_ptr->stat_ind[cp_ptr->spell_stat]] - 1);

	/* Not enough mana to cast */
	if (spell->mana_cost > p_ptr->csp)
	{
		chance += 5 * (spell->mana_cost - p_ptr->csp);
	}

	/* Extract the minimum failure rate */
	minfail = adj_mag_fail[p_ptr->stat_ind[cp_ptr->spell_stat]];

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
                return summon_specific(y, x, lev, SUMMON_LUA);
        else
                return summon_specific_friendly(y, x, lev, TRUE, SUMMON_LUA);
}

#endif
