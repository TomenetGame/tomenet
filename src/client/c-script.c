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
/* TODO: rename pern_* to tomenet_* :)
 * TODO: make msg_print/msg_admin dummy function and integrate this file
 * with server/script.c
 */

#include "angband.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "tolua.h"

int  tolua_util_open (lua_State *L);
int  tolua_player_open (lua_State *L);
int  tolua_spells_open (lua_State *L);
void dump_lua_stack_stdout(int min, int max);
void dump_lua_stack(int min, int max);

/* Don't include the iolib because of security concerns. */
#define NO_CLIENT_IOLIB

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
			c_msg_format("\377vLUA: %s", buf);
			j = 0;
		}
		i++;
	}
	buf[j] = '\0';
	c_msg_format("\377vLUA: %s", buf);
	return (0);
}

#ifdef NO_CLIENT_IOLIB

#include "luadebug.h"

/* Code from liolib.c to get pern_errormessage working. */

#define LEVELS1	12	/* size of the first part of the stack */
#define LEVELS2	10	/* size of the second part of the stack */

static int errorfb (lua_State *L) {
  int level = 1;  /* skip level 0 (it's this function) */
  int firstpart = 1;  /* still before eventual `...' */
  lua_Debug ar;
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  luaL_addstring(&b, "error: ");
  luaL_addstring(&b, luaL_check_string(L, 1));
  luaL_addstring(&b, "\n");
  while (lua_getstack(L, level++, &ar)) {
    char buff[120];  /* enough to fit following `sprintf's */
    if (level == 2)
      luaL_addstring(&b, "stack traceback:\n");
    else if (level > LEVELS1 && firstpart) {
      /* no more than `LEVELS2' more levels? */
      if (!lua_getstack(L, level+LEVELS2, &ar))
        level--;  /* keep going */
      else {
        luaL_addstring(&b, "       ...\n");  /* too many levels */
        while (lua_getstack(L, level+LEVELS2, &ar))  /* find last levels */
          level++;
      }
      firstpart = 0;
      continue;
    }
    sprintf(buff, "%4d:  ", level-1);
    luaL_addstring(&b, buff);
    lua_getinfo(L, "Snl", &ar);
    switch (*ar.namewhat) {
      case 'g':  case 'l':  /* global, local */
        sprintf(buff, "function `%.50s'", ar.name);
        break;
      case 'f':  /* field */
        sprintf(buff, "method `%.50s'", ar.name);
        break;
      case 't':  /* tag method */
        sprintf(buff, "`%.50s' tag method", ar.name);
        break;
      default: {
        if (*ar.what == 'm')  /* main? */
          sprintf(buff, "main of %.70s", ar.short_src);
        else if (*ar.what == 'C')  /* C function? */
          sprintf(buff, "%.70s", ar.short_src);
        else
          sprintf(buff, "function <%d:%.70s>", ar.linedefined, ar.short_src);
        ar.source = NULL;  /* do not print source again */
      }
    }
    luaL_addstring(&b, buff);
    if (ar.currentline > 0) {
      sprintf(buff, " at line %d", ar.currentline);
      luaL_addstring(&b, buff);
    }
    if (ar.source) {
      sprintf(buff, " [%.70s]", ar.short_src);
      luaL_addstring(&b, buff);
    }
    luaL_addstring(&b, "\n");
  }
  luaL_pushresult(&b);
  lua_getglobal(L, LUA_ALERT);
  if (lua_isfunction(L, -1)) {  /* avoid loop if _ALERT is not defined */
    lua_pushvalue(L, -2);  /* error message */
    lua_rawcall(L, 1, 0);
  }
  return 0;
}

#endif

static struct luaL_reg pern_iolib[] =
{
	{LUA_ALERT, pern_errormessage},
#ifdef NO_CLIENT_IOLIB
	{LUA_ERRORMESSAGE, errorfb},
#endif
};

#define luaL_check_bit(L, n)  ((long)luaL_check_number(L, n))
#define luaL_check_ubit(L, n) ((unsigned long)luaL_check_bit(L, n))


/* DYADIC/MONADIC is commented out in ToME;
 * better avoid to use them maybe */

#define DYADIC(name, op) \
	s32b name(s32b a, s32b b); \
	s32b name(s32b a, s32b b) { \
		return (a op b); \
	}

#define MONADIC(name, op) \
	s32b name(s32b b); \
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


bool init_lua_done = FALSE;
bool open_lua_done = FALSE;

/* Set global lua variables that define the server type */
static void set_server_features() {
	char buf[80];
	int oldtop;

	oldtop = lua_gettop(L);

	/* Server type flags */

	sprintf(buf, "RPG_SERVER = %d", s_RPG);
	lua_dostring(L, buf);
	lua_settop(L, oldtop);

	sprintf(buf, "ARCADE_SERVER = %d", s_ARCADE);
	lua_dostring(L, buf);
	lua_settop(L, oldtop);

	sprintf(buf, "FUN_SERVER = %d", s_FUN);
	lua_dostring(L, buf);
	lua_settop(L, oldtop);

	sprintf(buf, "PARTY_SERVER = %d", s_PARTY);
	lua_dostring(L, buf);
	lua_settop(L, oldtop);

	sprintf(buf, "TEST_SERVER = %d", s_TEST);
	lua_dostring(L, buf);
	lua_settop(L, oldtop);

	/* Temporary features flags */

	sprintf(buf, "TEMP0 = %d", sflags_TEMP & 0x00000001);
	lua_dostring(L, buf);
	lua_settop(L, oldtop);

	sprintf(buf, "TEMP1 = %d", sflags_TEMP & 0x00000002);
	lua_dostring(L, buf);
	lua_settop(L, oldtop);

	sprintf(buf, "TEMP2 = %d", sflags_TEMP & 0x00000004);
	lua_dostring(L, buf);
	lua_settop(L, oldtop);

	sprintf(buf, "TEMP3 = %d", sflags_TEMP & 0x00000008);
	lua_dostring(L, buf);
	lua_settop(L, oldtop);

	sprintf(buf, "TEMP4 = %d", sflags_TEMP & 0x00000010);
	lua_dostring(L, buf);
	lua_settop(L, oldtop);

	sprintf(buf, "TEMP5 = %d", sflags_TEMP & 0x00000020);
	lua_dostring(L, buf);
	lua_settop(L, oldtop);

	sprintf(buf, "TEMP6 = %d", sflags_TEMP & 0x00000040);
	lua_dostring(L, buf);
	lua_settop(L, oldtop);

	sprintf(buf, "TEMP7 = %d", sflags_TEMP & 0x00000080);
	lua_dostring(L, buf);
	lua_settop(L, oldtop);
}


/* Initialize lua scripting */
void init_lua()
{
	char ind[80];
	int oldtop;

	if (init_lua_done) return;

	/* Start the interpreter with default stack size */
	L = lua_open(0);

	/* Register the Lua base libraries */
	lua_baselibopen(L);
	lua_strlibopen(L);
	/* I/O and system functions are a potential security risk in the client - mikaelh */
#ifndef NO_CLIENT_IOLIB
	lua_iolibopen(L);
#endif
	lua_dblibopen(L);

	/* Register pern lua debug library */
	luaL_openl(L, pern_iolib);

	/* Register the bitlib */
	luaL_openl(L, bitlib);

	/* Register the TomeNET main APIs */
	tolua_util_open(L);
	tolua_player_open(L);
	tolua_spells_open(L);

	/* Set the Ind and player variables */
	oldtop = lua_gettop(L);
	sprintf(ind, "Ind = %d; player = Players_real[2]", 1);
	lua_dostring(L, ind);
	lua_settop(L, oldtop);

#if 1
/* CONFLICT: We want to fetch server flags before opening lua, so we can't read audio.lua
in time, nor xml.lua/meta.lua which are needed earlier too, so we need to hard-code it here -_-. */

	/* Load the xml module */
	pern_dofile(0, "xml.lua");
	pern_dofile(0, "meta.lua");

	/* Sound system */
	pern_dofile(0, "audio.lua");
#endif

	/* We want to init guide info asap, no need to wait for later */
	pern_dofile(0, "guide.lua");

	init_lua_done = TRUE;
}

/* Reinitialize Lua */
void reinit_lua()
{
	if (init_lua_done) {
		/* Close the old Lua state */
		lua_close(L);

		init_lua_done = FALSE;
	}

	init_lua();
}

void open_lua()
{
	int i, max;
	char out_val[160];

	if (open_lua_done) return;

	if (!init_lua_done) init_lua();

	/* Set server feature flags */
	set_server_features();

	/* Load the first lua file */
	pern_dofile(0, "c-init.lua");

	/* Finish up schools */
	max = exec_lua(0, "return __schools_num");
	init_schools(max);
	for (i = 0; i < max; i++) {
		sprintf(out_val, "finish_school(%d)", i);
		exec_lua(0, out_val);
	}

	/* Finish up the spells */
	max = exec_lua(0, "return __tmp_spells_num");
	init_spells(max);
	for (i = 0; i < max; i++) {
		sprintf(out_val, "finish_spell(%d)", i);
		exec_lua(0, out_val);
	}

	open_lua_done = TRUE;
}

void reopen_lua()
{
	int i;

	/* Free up school names */
	for (i = 0; i < max_schools; i++) {
		if (schools[i].name) string_free(schools[i].name);
	}

	/* Free up spell names */
	for (i = 0; i < max_spells; i++) {
		if (school_spells[i].name) string_free(school_spells[i].name);
	}

	/* Free up schools and spells */
	C_KILL(schools, max_schools, school_type);
	C_KILL(school_spells, max_spells, spell_type);

	/* Reinitialize Lua */
	reinit_lua();

	open_lua_done = FALSE;
	open_lua();
}

void dump_lua_stack(int min, int max)
{
	int i;

	c_msg_print("\377ylua_stack:");
	for (i = min; i <= max; i++)
	{
		if (lua_isnumber(L, i)) c_msg_format("\377y%d [n] = %ld", i, tolua_getnumber(L, i, 0));
		else if (lua_isstring(L, i)) c_msg_format("\377y%d [s] = '%s'", i, tolua_getstring(L, i, 0));
	}
	c_msg_print("\377yEND lua_stack");
}

void dump_lua_stack_stdout(int min, int max)
{
	int i;

	printf("ylua_stack:\n");
	for (i = min; i <= max; i++)
	{
		if (lua_isnumber(L, i)) printf("%d [n] = %ld\n", i, tolua_getnumber(L, i, 0));
		else if (lua_isstring(L, i)) printf("%d [s] = '%s'\n", i, tolua_getstring(L, i, 0));
	}
	printf("END lua_stack\n");
}

bool pern_dofile(int Ind, char *file)
{
	(void) Ind; /* suppress compiler warning */
	char buf[MAX_PATH_LENGTH];
	int error;
	int oldtop = lua_gettop(L);

	/* Build the filename */
	path_build(buf, MAX_PATH_LENGTH, ANGBAND_DIR_SCPT, file);

	error = lua_dofile(L, buf);
	lua_settop(L, oldtop);

	return (error?TRUE:FALSE);
}

int exec_lua(int Ind, char *file)
{
	(void) Ind; /* suppress compiler warning */
	int oldtop = lua_gettop(L);
	int res;

	if (!lua_dostring(L, file))
	{
		int size = lua_gettop(L) - oldtop;
		if (size != 0)
			res = tolua_getnumber(L, -size, 0);
		else
			res = 0;
	}
	else
		res = 0;

	lua_settop(L, oldtop);
	return (res);
}

cptr string_exec_lua(int Ind, char *file)
{
	(void) Ind; /* suppress compiler warning */
	int oldtop = lua_gettop(L);
	cptr res;

	if (!lua_dostring(L, file))
	{
		int size = lua_gettop(L) - oldtop;
		if (size != 0)
			res = tolua_getstring(L, -size, "");
		else
			res = 0;
	}
	else
		res = "";
	lua_settop(L, oldtop);
	return (res);
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

void master_script_exec(int Ind, char *buf)
{
	exec_lua(Ind, buf);
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

bool call_lua(int Ind, cptr function, cptr args, cptr ret, ...)
{
	(void) Ind; /* suppress compiler warning */
	int i = 0, nb = 0, nbr = 0;
	int oldtop = lua_gettop(L), size;
	va_list ap;

	va_start(ap, ret);

	/* Push the function */
	lua_getglobal(L, function);

	/* Push and count the arguments */
	while (args[i])
	{
		switch (args[i++])
		{
		case 'd':
		case 'l':
			tolua_pushnumber(L, va_arg(ap, s32b));
			nb++;
			break;
		case 's':
			tolua_pushstring(L, va_arg(ap, char*));
			nb++;
			break;
		case 'O':
			tolua_pushusertype(L, (void*)va_arg(ap, object_type*), tolua_tag(L, "object_type"));
			nb++;
			break;
		case '(':
		case ')':
		case ',':
			break;
		}
	}

	/* Count returns */
	nbr = strlen(ret);

	/* Call the function */
	if (lua_call(L, nb, nbr))
	{
		plog_fmt("ERROR in lua_call while calling '%s' from call_lua.\nThings should start breaking up from now on!", function);
		return FALSE;
	}

	/* Number of returned values, SHOULD be the same as nbr, but I'm paranoid */
	size = lua_gettop(L) - oldtop;

	/* Get the returns */
	for (i = 0; ret[i]; i++)
	{
		switch (ret[i])
		{
		case 'd':
		case 'l':
			{
				s32b *tmp = va_arg(ap, s32b*);

				if (lua_isnumber(L, (-size) + i)) *tmp = tolua_getnumber(L, (-size) + i, 0);
				else *tmp = 0;
				break;
			}

		case 's':
			{
				cptr *tmp = va_arg(ap, cptr*);

				if (lua_isstring(L, (-size) + i)) *tmp = tolua_getstring(L, (-size) + i, "");
				else *tmp = NULL;
				break;
			}

		case 'O':
			{
				object_type **tmp = va_arg(ap, object_type**);

				if (tolua_istype(L, (-size) + i, tolua_tag(L, "object_type"), 0))
					*tmp = (object_type*)tolua_getuserdata(L, (-size) + i, NULL);
				else
					*tmp = NULL;
				break;
			}

		default:
			plog_fmt("ERROR in lua_call while calling '%s' from call_lua:\n  Unkown return type '%c'", function, ret[i]);
			return FALSE;
		}
	}
	lua_settop(L, oldtop);

	va_end(ap);

	return TRUE;
}
