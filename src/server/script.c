/* File: script.c */

/* Purpose: scripting in lua */

/*
 * Copyright (c) 2001 Dark God
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
//                        msg_broadcast(0, format("\377vLUA: %s", buf));
                        msg_admin("\377vLUA: %s", buf);
                        j = 0;
                }
                i++;
        }
        buf[j] = '\0';
//        msg_broadcast(0, format("\377vLUA: %s", buf));
        msg_admin("\377vLUA: %s", buf);
        return (0);
}

static struct luaL_reg pern_iolib[] =
{
        {"_ALERT", pern_errormessage},
};

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


/* Initialize lua scripting */
void init_lua()
{
	/* Start the interpreter with default stack size */
	L = lua_open(0);

	/* Register the Lua base libraries */
	lua_baselibopen(L);
	lua_strlibopen(L);
	lua_iolibopen(L);
	lua_dblibopen(L);

	/* Register pern lua debug library */
	luaL_openl(L, pern_iolib);

        /* Register the PernAngband main APIs */
        tolua_util_open(L);
        tolua_player_open(L);

        /* Load the first lua file */
        pern_dofile(0, "init.lua");
}

bool pern_dofile(int Ind, char *file)
{
	char buf[1024];
        int oldtop = lua_gettop(L);

	/* Build the filename */
        path_build(buf, 1024, ANGBAND_DIR_SCPT, file);

        lua_dostring(L, format("Ind = %d", Ind));
        lua_dofile(L, buf);
        lua_settop(L, oldtop);

        return (FALSE);
}

bool exec_lua(int Ind, char *file)
{
        int oldtop = lua_gettop(L);
        lua_dostring(L, format("Ind = %d", Ind));
        lua_dostring(L, file);
        lua_settop(L, oldtop);
        return (FALSE);
}

static FILE *lua_file;
void master_script_begin(char *name, char mode)
{
	char buf[1024];

	/* Build the filename */
        path_build(buf, 1024, ANGBAND_DIR_SCPT, name);

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

void master_script_exec(int Ind, char *buf)
{
        exec_lua(Ind, buf);
}

void cat_script(int Ind, char *name)
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
                msg_print(Ind, buf);
        }
}
