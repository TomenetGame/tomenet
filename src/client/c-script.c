/* $Id$ */
/* File: c-script.c */

/* Purpose: scripting in lua */

/*
 * Copyright (c) 2001 Dark God
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */
/* TODO: rename pern_* to tomenet_* :) */

#include "angband.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "tolua.h"

int  tolua_util_open (lua_State *L);
int  tolua_player_open (lua_State *L);

/*
 * Lua state
 */
lua_State* L = NULL;

/* PernAngband Lua error message handler */
static int pern_errormessage(lua_State *L)
{
	char buf[200];
	cptr str = luaL_check_string(L, 1);
	int i = 0, j = 0;

	while (str[i])
	{
		if (str[i] != '\n')
		{
			buf[j++] = str[i];
		}
		else
		{
			buf[j] = '\0';
			//msg_broadcast(0, format("\377vLUA: %s", buf));
			//msg_admin("\377vLUA: %s", buf);
			c_msg_format("\377vLUA: %s", buf);
			j = 0;
		}
		i++;
	}
	buf[j] = '\0';
	//msg_broadcast(0, format("\377vLUA: %s", buf));
	//msg_admin("\377vLUA: %s", buf);
	c_msg_format("\377vLUA: %s", buf);
	return (0);
}

static struct luaL_reg pern_iolib[] =
{
        {"_ALERT", pern_errormessage},
};

#define luaL_check_bit(L, n)  ((long)luaL_check_number(L, n))
#define luaL_check_ubit(L, n) ((unsigned long)luaL_check_bit(L, n))


/* DYADIC/MONADIC is commented out in ToME;
 * better avoid to use them maybe */

#define DYADIC(name, op) \
    s32b name(s32b a, s32b b) { \
		return (a op b); \
    }

#define MONADIC(name, op) \
    s32b name(s32b b) { \
		return (op b); \
    }


DYADIC(intMod,      % )
DYADIC(intAnd,      & )
DYADIC(intOr,       | )
DYADIC(intXor,      ^ )
DYADIC(intShiftl,   <<)
DYADIC(intShiftr,   >>)
MONADIC(intBitNot,  ~ )


/*
 * Monadic bit nagation operation
 * MONADIC(not,     ~)
 */
static int int_not(lua_State* L)
{
	lua_pushnumber(L, ~luaL_check_bit(L, 1));
	return 1;
}


/*
 * Dyadic integer modulus operation
 * DYADIC(mod,      %)
 */
static int int_mod(lua_State* L)
{
	lua_pushnumber(L, luaL_check_bit(L, 1) % luaL_check_bit(L, 2));
    return 1;
}


/*
 * Variable length bitwise AND operation
 * VARIADIC(and,    &)
 */
static int int_and(lua_State *L)
{
	int n = lua_gettop(L), i;
	long w = luaL_check_bit(L, 1);

	for (i = 2; i <= n; i++) w &= luaL_check_bit(L, i);
	lua_pushnumber(L, w);

	return 1;
}


/*
 * Variable length bitwise OR operation
 * VARIADIC(or,     |)
 */
static int int_or(lua_State *L)
{
	int n = lua_gettop(L), i;
	long w = luaL_check_bit(L, 1);

	for (i = 2; i <= n; i++) w |= luaL_check_bit(L, i);
    lua_pushnumber(L, w);

    return 1;
}


/*
 * Variable length bitwise XOR operation
 * VARIADIC(xor,    ^)
 */
static int int_xor(lua_State *L)
{
	int n = lua_gettop(L), i;
	long w = luaL_check_bit(L, 1);

	for (i = 2; i <= n; i++) w ^= luaL_check_bit(L, i);
    lua_pushnumber(L, w);

    return 1;
}


/*
 * Binary left shift operation
 * TDYADIC(lshift,  <<, , u)
 */
static int int_lshift(lua_State* L)
{
	lua_pushnumber(L, luaL_check_bit(L, 1) << luaL_check_ubit(L, 2));
    return 1;
}

/*
 * Binary logical right shift operation
 * TDYADIC(rshift,  >>, u, u)
 */
static int int_rshift(lua_State* L)
{
	lua_pushnumber(L, luaL_check_ubit(L, 1) >> luaL_check_ubit(L, 2));
	return 1;
}

/*
 * Binary arithmetic right shift operation
 * TDYADIC(arshift, >>, , u)
 */
static int int_arshift(lua_State* L)
{
	lua_pushnumber(L, luaL_check_bit(L, 1) >> luaL_check_ubit(L, 2));
	return 1;
}


static const struct luaL_reg bitlib[] =
{
        {"bnot",    int_not},
        {"imod",    int_mod},  /* "mod" already in Lua math library */
        {"band",    int_and},
        {"bor",     int_or},
        {"bxor",    int_xor},
        {"lshift",  int_lshift},
        {"rshift",  int_rshift},
        {"arshift", int_arshift},
};


/* Initialize lua scripting */
void init_lua()
{
	int i, max;

	/* Start the interpreter with default stack size */
	L = lua_open(0);

	/* Register the Lua base libraries */
	lua_baselibopen(L);
	lua_strlibopen(L);
	lua_iolibopen(L);
	lua_dblibopen(L);

	/* Register pern lua debug library */
	luaL_openl(L, pern_iolib);

	/* Register the bitlib */
	luaL_openl(L, bitlib);

	/* Register the TomeNET main APIs */
	tolua_util_open(L);
	tolua_player_open(L);

	/* Load the first lua file */
	pern_dofile("c-init.lua");

	/* Finish up schools */
	max = exec_lua("return __schools_num");
	init_schools(max);
	for (i = 0; i < max; i++)
	{
		exec_lua(format("finish_school(%d)", i));
	}
}

bool pern_dofile(char *file)
{
	char buf[MAX_PATH_LENGTH];
        int oldtop = lua_gettop(L);

	/* Build the filename */
        path_build(buf, MAX_PATH_LENGTH, ANGBAND_DIR_SCPT, file);

//        lua_dostring(L, format("Ind = %d", Ind));
        lua_dofile(L, buf);
        lua_settop(L, oldtop);

        return (FALSE);
}

bool exec_lua(char *file)
{
        int oldtop = lua_gettop(L);
//        lua_dostring(L, format("Ind = %d", Ind));
        lua_dostring(L, file);
        lua_settop(L, oldtop);
        return (FALSE);
}

static FILE *lua_file;
void master_script_begin(char *name, char mode)
{
	char buf[MAX_PATH_LENGTH];

	/* Build the filename */
        path_build(buf, MAX_PATH_LENGTH, ANGBAND_DIR_SCPT, name);

        switch (mode)
        {
        case MASTER_SCRIPTB_W:
                lua_file = my_fopen(buf, "w");
                break;
        case MASTER_SCRIPTB_A:
                lua_file = my_fopen(buf, "a");
                break;
        }
        if (!lua_file)
                plog(format("ERROR: creating lua file %s in mode %c", buf, mode));
        else
                plog(format("Creating lua file %s in mode %c", buf, mode));

}

void master_script_end()
{
        my_fclose(lua_file);
}

void master_script_line(char *buf)
{
        fprintf(lua_file, "%s\n", buf);
}

void master_script_exec(char *buf)
{
        exec_lua(buf);
}

void cat_script(char *name)
{
        char buf[1025];
        FILE *fff;

        /* Build the filename */
        path_build(buf, 1024, ANGBAND_DIR_SCPT, name);
        fff = my_fopen(buf, "r");
        if (fff == NULL) return;

        /* Process the file */
        while (0 == my_fgets(fff, buf, 1024))
        {
                c_msg_print(buf);
        }
}
