/* $Id$ */
/* File: script.c */

/* Purpose: scripting in lua */

/*
 * Copyright (c) 2001 Dark God
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

/* added this for consistency in some (unrelated) header-inclusion,
   it IS a server file, isn't it? */
#define SERVER

#include "angband.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "tolua.h"

extern int tolua_z_pack_open (lua_State* tolua_S);

int  tolua_util_open (lua_State *L);
int  tolua_player_open (lua_State *L);
int  tolua_spells_open (lua_State *L);

void set_server_features();

/*
 * Lua state
 */
lua_State* L = NULL;

/* PernAngband Lua error message handler */
static int pern_errormessage(lua_State *L) {
	char buf[200];
	cptr str = luaL_check_string(L, 1);
	int i = 0, j = 0;

	while (str[i]) {
		if (str[i] != '\n') buf[j++] = str[i];
		else {
			buf[j] = '\0';
			//msg_broadcast_format(0, "\377vLUA: %s", buf);
			msg_admin("\377vLUA: %s", buf);
			j = 0;
		}
		i++;
	}
	buf[j] = '\0';
	//msg_broadcast_format(0, "\377vLUA: %s", buf);
	msg_admin("\377vLUA: %s", buf);
	return (0);
}

static struct luaL_reg pern_iolib[] = {
	{"_ALERT", pern_errormessage},
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
static int int_not(lua_State* L) {
	lua_pushnumber(L, ~luaL_check_bit(L, 1));
	return 1;
}


/*
 * Dyadic integer modulus operation
 * DYADIC(mod,      %)
 */
static int int_mod(lua_State* L) {
	lua_pushnumber(L, luaL_check_bit(L, 1) % luaL_check_bit(L, 2));
	return 1;
}


/*
 * Variable length bitwise AND operation
 * VARIADIC(and,    &)
 */
static int int_and(lua_State *L) {
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
static int int_or(lua_State *L) {
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
static int int_xor(lua_State *L) {
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
static int int_lshift(lua_State* L) {
	lua_pushnumber(L, luaL_check_bit(L, 1) << luaL_check_ubit(L, 2));
	return 1;
}

/*
 * Binary logical right shift operation
 * TDYADIC(rshift,  >>, u, u)
 */
static int int_rshift(lua_State* L) {
	lua_pushnumber(L, luaL_check_ubit(L, 1) >> luaL_check_ubit(L, 2));
	return 1;
}

/*
 * Binary arithmetic right shift operation
 * TDYADIC(arshift, >>, , u)
 */
static int int_arshift(lua_State* L) {
	lua_pushnumber(L, luaL_check_bit(L, 1) >> luaL_check_ubit(L, 2));
	return 1;
}


static const struct luaL_reg bitlib[] = {
	{"bnot",    int_not},
	{"imod",    int_mod},  /* "mod" already in Lua math library */
	{"band",    int_and},
	{"bor",     int_or},
	{"bxor",    int_xor},
	{"lshift",  int_lshift},
	{"rshift",  int_rshift},
	{"arshift", int_arshift},
};


/* Set global lua variables that define the server type */
void set_server_features() {
	int oldtop = lua_gettop(L);

	/* Server flags */

#ifdef RPG_SERVER
	lua_dostring(L, "RPG_SERVER = 1");
#else
	lua_dostring(L, "RPG_SERVER = 0");
#endif
	lua_settop(L, oldtop);

#ifdef ARCADE_SERVER
	lua_dostring(L, "ARCADE_SERVER = 1");
#else
	lua_dostring(L, "ARCADE_SERVER = 0");
#endif
	lua_settop(L, oldtop);

#ifdef FUN_SERVER
	lua_dostring(L, "FUN_SERVER = 1");
#else
	lua_dostring(L, "FUN_SERVER = 0");
#endif
	lua_settop(L, oldtop);

#ifdef PARTY_SERVER
	lua_dostring(L, "PARTY_SERVER = 1");
#else
	lua_dostring(L, "PARTY_SERVER = 0");
#endif
	lua_settop(L, oldtop);

#ifdef TEST_SERVER
	lua_dostring(L, "TEST_SERVER = 1");
#else
	lua_dostring(L, "TEST_SERVER = 0");
#endif
	lua_settop(L, oldtop);

	/* Misc flags */

#ifdef ENABLE_INSTANT_RES
	sflags_TEMP |= 0x00000001;
	lua_dostring(L, "TEMP0 = 1");
#else
	lua_dostring(L, "TEMP0 = 0");
#endif
	lua_settop(L, oldtop);

#ifdef TELEKINESIS_GETITEM_SERVERSIDE
	sflags_TEMP |= 0x00000002;
	lua_dostring(L, "TEMP1 = 1");
#else
	lua_dostring(L, "TEMP1 = 0");
#endif
	lua_settop(L, oldtop);

#ifdef ENABLE_OCCULT
	sflags_TEMP |= 0x00000004;
	lua_dostring(L, "TEMP2 = 1");
	lua_settop(L, oldtop);
#else
	lua_dostring(L, "TEMP2 = 0");
	lua_settop(L, oldtop);
#endif

#ifdef ENABLE_OHERETICISM
	sflags_TEMP |= 0x00000008;
	lua_dostring(L, "TEMP3 = 1");
	lua_settop(L, oldtop);
#else
	lua_dostring(L, "TEMP3 = 0");
	lua_settop(L, oldtop);
#endif

//	sflags_TEMP |= 0x00000010;
	lua_dostring(L, "TEMP4 = 0");
	lua_settop(L, oldtop);

//	sflags_TEMP |= 0x00000020;
	lua_dostring(L, "TEMP5 = 0");
	lua_settop(L, oldtop);

//	sflags_TEMP |= 0x00000040;
	lua_dostring(L, "TEMP6 = 0");
	lua_settop(L, oldtop);

//	sflags_TEMP |= 0x00000080;
	lua_dostring(L, "TEMP7 = 0");
	lua_settop(L, oldtop);
}


/* Initialize lua scripting */
void init_lua() {
	int i, max;

	/* Start the interpreter with default stack size */
	L = lua_open(0);

	/* Register the Lua base libraries */
	lua_baselibopen(L);
	lua_mathlibopen(L);
	lua_strlibopen(L);
	lua_iolibopen(L);
	lua_dblibopen(L);

	/* Register pern lua debug library */
	luaL_openl(L, pern_iolib);

	/* Register the bitlib */
	luaL_openl(L, bitlib);

	/* Register the PernAngband main APIs */
	tolua_util_open(L);
	tolua_player_open(L);
	tolua_spells_open(L);
	tolua_z_pack_open(L);

	/* Set server features before loading any lua files */
	set_server_features();

	/* Load the first lua file */
	pern_dofile(0, "init.lua");

	/* Finish up schools */
	max = exec_lua(0, "return __schools_num");
	init_schools(max);
	for (i = 0; i < max; i++)
		exec_lua(0, format("finish_school(%d)", i));
	/* Copy certain schools which mark the beginning and end of each magic resort
	   to local C array for get_spellbook_name_colour() */
	SCHOOL_HOFFENSE = exec_lua(0, "return SCHOOL_HOFFENSE");
	SCHOOL_HSUPPORT = exec_lua(0, "return SCHOOL_HSUPPORT");
	SCHOOL_DRUID_ARCANE = exec_lua(0, "return SCHOOL_DRUID_ARCANE");
	SCHOOL_DRUID_PHYSICAL = exec_lua(0, "return SCHOOL_DRUID_PHYSICAL");
	SCHOOL_ASTRAL = exec_lua(0, "return SCHOOL_ASTRAL");
	SCHOOL_PPOWER = exec_lua(0, "return SCHOOL_PPOWER");
	SCHOOL_MINTRUSION = exec_lua(0, "return SCHOOL_MINTRUSION");
#ifdef ENABLE_OCCULT /* Occult */
	SCHOOL_OSHADOW = exec_lua(0, "return SCHOOL_OSHADOW");
	SCHOOL_OSPIRIT = exec_lua(0, "return SCHOOL_OSPIRIT");
 #ifdef ENABLE_OHERETICISM
	SCHOOL_OHERETICISM = exec_lua(0, "return SCHOOL_OHERETICISM");
 #endif
#endif

	/* Finish up the spells */
	max = exec_lua(0, "return __tmp_spells_num");
	init_spells(max);
	for (i = 0; i < max; i++) {
		exec_lua(0, format("finish_spell(%d)", i));

		/* Copy spells to local C array for get_spellbook_name_colour() */
		spell_school[i] = exec_lua(0, format("return __spell_school[%d][1]", i));
	}

#ifdef USE_SOUND_2010
	/* copy over the audio_sfx[] array for efficiency */
	for (i = 0; i < SOUND_MAX_2010; i++) {
		strcpy(audio_sfx[i],
		    string_exec_lua(0, format("return get_sound_name(%d)", i)));

		if (!strcmp(audio_sfx[i], "am_field")) __sfx_am = i;
	}
#endif

	ID_spell1 = exec_lua(0, "return IDENTIFY_I"); ID_spell1a = exec_lua(0, "return IDENTIFY_II"); ID_spell1b = exec_lua(0, "return IDENTIFY_III");
	ID_spell2 = exec_lua(0, "return MIDENTIFY");
	ID_spell3 = exec_lua(0, "return STARIDENTIFY");
	ID_spell4 = exec_lua(0, "return BAGIDENTIFY");
}

void reinit_lua() {
	int i;

	/* Close the old Lua state */
	lua_close(L);

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

	init_lua();
}

bool pern_dofile(int Ind, char *file) {
	char buf[MAX_PATH_LENGTH];
	int error;
	int oldtop = lua_gettop(L);
	char ind[80];

	/* Build the filename */
	path_build(buf, MAX_PATH_LENGTH, ANGBAND_DIR_SCPT, file);

	sprintf(ind, "Ind = %d; player = Players_real[Ind + 1]", Ind);
	lua_dostring(L, ind);
	lua_settop(L, oldtop);

	error = lua_dofile(L, buf);
	lua_settop(L, oldtop);

	return (error?TRUE:FALSE);
}

int exec_lua(int Ind, char *file) {
	int oldtop = lua_gettop(L);
	int res;
	char ind[80];

	sprintf(ind, "Ind = %d; player = Players_real[Ind + 1]", Ind);
	lua_dostring(L, ind);
	lua_settop(L, oldtop);

	if (!lua_dostring(L, file)) {
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

cptr string_exec_lua(int Ind, char *file) {
	int oldtop = lua_gettop(L);
	cptr res;
	char ind[80];

	sprintf(ind, "Ind = %d; player = Players_real[Ind + 1]", Ind);
	lua_dostring(L, ind);
	lua_settop(L, oldtop);

	if (!lua_dostring(L, file)) {
		int size = lua_gettop(L) - oldtop;
		if (size != 0)
			res = tolua_getstring(L, -size, "");
		else
			res = 0;
	} else res = "";

	lua_settop(L, oldtop);
	return (res);
}

static FILE *lua_file;
void master_script_begin(char *name, char mode) {
	char buf[MAX_PATH_LENGTH];

	/* Build the filename */
	path_build(buf, MAX_PATH_LENGTH, ANGBAND_DIR_SCPT, name);

	switch (mode) {
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

void master_script_end() {
	my_fclose(lua_file);
}

void master_script_line(char *buf) {
	fprintf(lua_file, "%s\n", buf);
}

void master_script_exec(int Ind, char *buf) {
	exec_lua(Ind, buf);
}

void cat_script(int Ind, char *name) {
	char buf[1025];
	FILE *fff;

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_SCPT, name);
	fff = my_fopen(buf, "rb");
	if (fff == NULL) return;

	/* Process the file */
	while (0 == my_fgets(fff, buf, 1024, FALSE))
		msg_print(Ind, buf);
}
