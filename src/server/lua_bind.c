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
	if (spell->mana_cost > p_ptr->csp) {
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
	if (rand_int(100) < chance) {
		if (flush_failure) flush();
		msg_print("You failed to concentrate hard enough!");
#ifdef USE_SOUND_2010
#else
		sound(SOUND_FAIL);
#endif

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
	if (tval) {
		item_tester_tval = tval;
	} else {
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

	do {
		scatter(yy, xx, y, x, 6, 0);
	} while (!(in_bounds(*yy, *xx) && cave_floor_bold(*yy, *xx)) && --attempts);
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
	else if (lvl > 0) {
//		tmp += p_ptr->to_s * (SKILL_STEP / 10);
		tmp += bonus;
//		  tmp += (get_skill_scale(p_ptr, SKILL_SPELL, 20) * (SKILL_STEP / 10));
//		  tmp /= 100; tmp *= (100 + (get_skill_scale(p_ptr, SKILL_SPELL, 40) * (SKILL_STEP / 10)));
		lvl = (tmp * (max * (SKILL_STEP / 10)) / (SKILL_MAX / 10)) / (SKILL_STEP / 10);
		if (school_spells[s].spell_power) {
			lvl *= (100 + get_skill_scale(p_ptr, SKILL_SPELL, 40));
			lvl /= 100;
		}
	}
	return lvl;
}

/* adj_mag_stat? stat_ind??  pfft */
//s32b lua_spell_chance(s32b chance, int level, int skill_level, int mana, int cur_mana, int stat)
/* NOTE: KEEP CONSISTENT WITH CLIENT-SIDE lua_bind.c:lua_spell_chance()! */
s32b lua_spell_chance(int i, s32b chance, int level, int skill_level, int mana, int cur_mana, int stat)
{
	player_type *p_ptr = Players[i];
	int             minfail;

//DEBUG:s_printf("chance %d - level %d - skill_level %d", chance, level, skill_level);
	/* correct LUA overflow bug ('fail' is type char, ie unsigned byte) */
	if (chance >= 156) chance -= 256;

#if 0 /* requires client-side modification too, so I'm moving it into s_aux.lua instead - C. Blue */
	/* hack - -99 means 'never fails'. This is used for 'stop' spells that
	   turn off durational spells that have been cast, like Wraithform,
	   and also don't cost any mana to "cast". - C. Blue */
	if (chance == -99) return(0);
#endif

	/* Reduce failure rate by "effective" level adjustment */
	chance -= 3 * (level - skill_level);

	/* Reduce failure rate by INT/WIS adjustment */
	chance -= 3 * (adj_mag_stat[p_ptr->stat_ind[stat]] - 1);

	 /* Not enough mana to cast */
	if (chance < 0) chance = 0;
#if 0 /* you cannot cast the spell anyway, so this just confuses */
	if (mana > cur_mana) {
		chance += 15 * (mana - cur_mana);
	}
#endif

	/* Extract the minimum failure rate */
	minfail = adj_mag_fail[p_ptr->stat_ind[stat]];

#if 0	// disabled for the time being
	/*
	 * Non mage characters never get too good
	 */
	if (!(PRACE_FLAG(PR1_ZERO_FAIL))) {
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
	get_mon_num_prep(0, NULL);

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
	for (y = *y1 - 1; y <= *y2 + 1; y++) {
		for (x = *x1 - 1; x <= *x2 + 1; x++) {
			cave_type *c_ptr = &cave[y][x];
			cave_set_feat(y, x, floor_type[rand_int(1000)]);
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

	/* Reject 'non-spawning' monsters */
	if (r_ptr->rarity == 255) return (FALSE);

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

	/* Reject neutral creatures */
	if (r_ptr->flags7 & RF7_NEUTRAL) return (FALSE);

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
	get_mon_num_prep(0, NULL);

	/* Set up the quest monster. */
	r_idx = get_mon_num(lev, lev);

	/* Undo the filters */
	get_mon_num_hook = dungeon_aux;

	return r_idx;
}

#endif

/* To do some connection magik ! */
void remote_update_lua(int Ind, cptr file)
{
	player_type *p_ptr = Players[Ind];

	/* Count # of LUA files to check for updates (for 4.4.8.1.0.0 crash bug) */
	p_ptr->warning_lua_count++;

	remote_update(p_ptr->conn, file);
	return;
}

/* Write a string to the log file */
void lua_s_print(cptr logstr) {
	s_printf(logstr);
	return;
}

void lua_add_anote(char *anote) {
	int i;
	bracer_ff(anote); /* allow colouring */
	for (i = 0; i < MAX_ADMINNOTES; i++)
		if (!strcmp(admin_note[i], "")) break;
	if (i < MAX_ADMINNOTES) {
		strcpy(admin_note[i], anote);
		msg_broadcast_format(0, "\377s->MotD: %s", anote);
	} else {
		s_printf("lua_add_anote() failed: out of notes.\n");
	}
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

/* keep whole function in sync with birth.c! */
void lua_recalc_char(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i, j, min_value, max_value, min_value_king = 0, max_value_king = 9999;
	int tries = 300;

	/* Hitdice */
	p_ptr->hitdie = p_ptr->rp_ptr->r_mhp + p_ptr->cp_ptr->c_mhp;

	/* keep in sync with birth.c! */
#if 0
	/* Minimum hitpoints at highest level */
        min_value = (PY_MAX_LEVEL * (p_ptr->hitdie - 1) * 3) / 8;
	min_value += PY_MAX_LEVEL;

	/* Maximum hitpoints at highest level */
	max_value = (PY_MAX_LEVEL * (p_ptr->hitdie - 1) * 5) / 8;
	max_value += PY_MAX_LEVEL;
#else
	/* Minimum hitpoints at kinging level */
        min_value_king = (50 * (p_ptr->hitdie - 1) * 15) / 32;
	min_value_king += 50;
	/* Maximum hitpoints at kinging level */
	max_value_king = (50 * (p_ptr->hitdie - 1) * 17) / 32;
	max_value_king += 50;

	/* Minimum hitpoints at highest level */
        min_value = (PY_MAX_LEVEL * (p_ptr->hitdie - 1) * 15) / 32;
	min_value += PY_MAX_LEVEL;
	/* Maximum hitpoints at highest level */
	max_value = (PY_MAX_LEVEL * (p_ptr->hitdie - 1) * 17) / 32;
	max_value += PY_MAX_LEVEL;
#endif

	/* Pre-calculate level 1 hitdice */
	p_ptr->player_hp[0] = p_ptr->hitdie;

	/* Roll out the hitpoints */
	while (--tries) {
		/* Roll the hitpoint values */
		for (i = 1; i < PY_MAX_LEVEL; i++) {
//			j = randint(p_ptr->hitdie);
			j = 2 + randint(p_ptr->hitdie - 4);
			p_ptr->player_hp[i] = p_ptr->player_hp[i-1] + j;
		}
		/* XXX Could also require acceptable "mid-level" hitpoints */

		/* Require "valid" hitpoints at kinging level */
		if (p_ptr->player_hp[50 - 1] < min_value_king) continue;
		if (p_ptr->player_hp[50 - 1] > max_value_king) continue;

		/* Require "valid" hitpoints at highest level */
		if (p_ptr->player_hp[PY_MAX_LEVEL - 1] < min_value) continue;
		if (p_ptr->player_hp[PY_MAX_LEVEL - 1] > max_value) continue;

		/* Acceptable */
		break;
	}

	if (!tries) s_printf("CHAR_REROLLING: %s exceeded 300 tries for HP rolling.\n", p_ptr->name);

	p_ptr->update |= PU_HP;
	update_stuff(Ind);
}

void lua_examine_item(int Ind, int Ind_target, int item) {
	identify_fully_aux(Ind, &Players[Ind_target]->inventory[item]);
	return;
}

void lua_determine_level_req(int Ind, int item) {
	/* -9999 is a fixed hack value, see object2.c, determine_level_req.
	   It means: assume that finding-depth is same as object-base-level,
	   for smooth and easy calculation. */
	determine_level_req(-9999, &Players[Ind]->inventory[item]);
	return;
}

/* the essential function for art reset */
void lua_strip_true_arts_from_present_player(int Ind, int mode) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	int i, total_number = 0;
	bool reimbursed = FALSE;
	s32b cost;

	for (i = 0; i < INVEN_TOTAL; i++) {
		o_ptr = &p_ptr->inventory[i];
		if (resettable_artifact_p(o_ptr))
			total_number++;
	}

	/* Tell the player what's going on.. */
	if (total_number == 1) msg_print(Ind, "\377yYour true artifact vanishes into the air!");
	else if (total_number == 2) msg_print(Ind, "\377yBoth of your true artifacts vanish into the air!");
	else if (total_number) msg_print(Ind, "\377yYour true artifacts all vanish into the air!");

	for (i = 0; i < INVEN_TOTAL; i++) {
		o_ptr = &p_ptr->inventory[i];
		if (resettable_artifact_p(o_ptr)) {
			//char  o_name[ONAME_LEN];
			//object_desc(Ind, o_name, o_ptr, TRUE, 0);
			//msg_format(Ind, "%s fades into the air!", o_name);

			/* reimburse player monetarily */
			cost = a_info[o_ptr->name1].cost;
			if (cost > 0) {
//				if (cost > 500000) cost = 500000; //not required, it's still fair
				/* hack: prevent s32b overflow */
				if (2000000000 - cost < p_ptr->au) {
					msg_format(Ind, "\377yYou cannot carry more than 2 billion worth of gold!");
				} else {
					p_ptr->au += cost;
					p_ptr->redraw |= (PR_GOLD);
					reimbursed = TRUE;
				}
			}

			if (mode == 0) handle_art_d(o_ptr->name1);

			/* Decrease the item, optimize. */
			inven_item_increase(Ind, i, -o_ptr->number);
			inven_item_describe(Ind, i);
			inven_item_optimize(Ind, i);
			i--;
		}
	}
	if (total_number) s_printf("True-art strip: %s loses %d artifact(s).\n", p_ptr->name, total_number);
	if (reimbursed) msg_print(Ind, "\377yYour purse magically seems heavier.");
}

void lua_check_player_for_true_arts(int Ind) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	int i, total_number = 0;

	for (i = 0; i < INVEN_TOTAL; i++) {
		o_ptr = &p_ptr->inventory[i];
		if (true_artifact_p(o_ptr)) total_number++;
	}
	if (total_number) s_printf("True-art scan: %s carries %d.\n", p_ptr->name, total_number);
}

void lua_strip_true_arts_from_absent_players(void) {
	strip_true_arts_from_hashed_players();
}

/* Remove all true artifacts lying on the (mang-house) floor
   anywhere in the world. Not processing traditional houses atm. */
void lua_strip_true_arts_from_floors(void) {
	int i, cnt=0, dcnt=0;
	object_type *o_ptr;
#if 0
	cave_type **zcave;
#endif

	for(i = 0; i < o_max; i++){
		o_ptr = &o_list[i];
		if(o_ptr->k_idx){
			cnt++;
			/* check items on the world's floors */
#if 0
			if((zcave = getcave(&o_ptr->wpos)) &&
			    resettable_artifact_p(o_ptr))
			{
				delete_object_idx(zcave[o_ptr->iy][o_ptr->ix].o_idx, TRUE);
                                dcnt++;
			}
#else
			if(resettable_artifact_p(o_ptr))
			{
				delete_object_idx(i, TRUE);
                                dcnt++;
			}
#endif
		}
	}
	s_printf("Scanned %d objects. Removed %d artefacts.\n", cnt, dcnt);
}

/* Return a monster level */
int lua_get_mon_lev(int r_idx) {
	return(r_info[r_idx].level);
}
/* Return a monster name */
char *lua_get_mon_name(int r_idx) {
	return(r_name + r_info[r_idx].name);
}

/* Return the last chat line */
inline char *lua_get_last_chat_line() {
	return last_chat_line;
}

/* Return the last person who said the last chat line */
char *lua_get_last_chat_owner() {
	return last_chat_owner;
}

/* Reset all towns */
void lua_towns_treset(void) {
	int x, y;
	struct worldpos wpos;

	wpos.wz = 0;

	for (x = 0; x < MAX_WILD_X; x++)
	for (y = 0; y < MAX_WILD_Y; y++) {
		wpos.wx = x;
		wpos.wy = y;
		if (istown(&wpos)) {
			unstatic_level(&wpos);
			if(getcave(&wpos)) dealloc_dungeon_level(&wpos);
		}
	}
}

/* To do some connection magik ! */
long lua_player_exp(int level, int expfact)
{
	s32b adv_exp;
	s64b adv;
	if ((level > 1) && (level < 100))
#ifndef ALT_EXPRATIO
	        adv = ((s64b)player_exp[level - 2] * (s64b)expfact / 100L);
#else
	        adv = (s64b)player_exp[level - 2];
#endif
	else
		adv = 0;
	adv_exp = (s32b)(adv);		
	return adv;
}

/* Fix all spellbooks not in players inventories (houses...)
 * Adds mod to spellbook pval if it's greater than or equal to spell
 * spell in most cases is the number of the new spell and mod is 1
 * - mikaelh */
void lua_fix_spellbooks(int spell, int mod)
{
	int i, j;
	object_type *o_ptr;
	house_type *h_ptr;

#ifndef USE_MANG_HOUSE_ONLY
	/* scan world (includes MAngband-style houses) */
	for (i = 0; i < o_max; i++) {
		o_ptr = &o_list[i];
		if (o_ptr->tval == TV_BOOK && o_ptr->sval == SV_SPELLBOOK && o_ptr->pval >= spell) {
			o_ptr->pval += mod;
		}
	}
#endif

	/* scan traditional (vanilla) houses */
	for (j = 0; j < num_houses; j++) {
		h_ptr = &houses[j];
		for (i = 0; i < h_ptr->stock_num; i++) {
			o_ptr = &h_ptr->stock[i];
			if (!o_ptr->k_idx) continue;
			if (o_ptr->tval == TV_BOOK && o_ptr->sval == SV_SPELLBOOK
			    && o_ptr->pval >= spell) {
				o_ptr->pval += mod;
			}
		}
	}
}

/* This function is called after load_player, so his artifacts would already be set to 1;
   for that reason we disable the mega-hack in load2.c instead for players whose updated_savegame
   flag shows that they might still carry unfixed arts, and do a general incrementing here  - C. Blue */
void lua_arts_fix(int Ind) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	int i; //, total_number = 0;

	for (i = 0; i < INVEN_TOTAL; i++) {
		o_ptr = &p_ptr->inventory[i];
		if (resettable_artifact_p(o_ptr))
			handle_art_i(o_ptr->name1);
	}
}

/* Display a player's global_event status */
void lua_get_pgestat(int Ind, int n) {
	player_type *p_ptr = Players[Ind];
	msg_format(Ind, "%s: #%d, type %d, signup %ld, started %ld,",
		    p_ptr->name, n, p_ptr->global_event_type[n], p_ptr->global_event_signup[n], p_ptr->global_event_started[n]);
	msg_format(Ind, "  progress: %ld,%ld,%ld,%ld", p_ptr->global_event_progress[n][0], p_ptr->global_event_progress[n][1],
		    p_ptr->global_event_progress[n][2], p_ptr->global_event_progress[n][3]);
}

void lua_start_global_event(int Ind, int evtype, char *parm) {
	start_global_event(Ind, evtype, parm);
}

/* Fix items that were changed on updating the server. If Ind == 0 -> scan world/houses - C. Blue */
void lua_apply_item_changes(int Ind, int changes) {
	int i, j;
	object_type *o_ptr;
	house_type *h_ptr;

	if (!Ind) {
	/* scan world/houses */
#ifndef USE_MANG_HOUSE_ONLY
		/* scan traditional (vanilla) houses */
		for(j = 0; j < num_houses; j++){
			h_ptr = &houses[j];
			for (i = 0; i < h_ptr->stock_num; i++){
				o_ptr = &h_ptr->stock[i];
		                if(!o_ptr->k_idx) continue;
				lua_apply_item_changes_aux(o_ptr, changes);
			}
		}
#endif
		/* scan world (includes MAngband-style houses) */
		for(i = 0; i < o_max; i++) {
			o_ptr = &o_list[i];
			if(!o_ptr->k_idx) continue;
			lua_apply_item_changes_aux(o_ptr, changes);
		}
	} else {
		/* scan a player's inventory/equipment */
		for (i = 0; i < INVEN_TOTAL; i++){
			o_ptr = &Players[Ind]->inventory[i];
			if (!o_ptr->k_idx) continue;
			lua_apply_item_changes_aux(o_ptr, changes);
		}
	}
}
void lua_apply_item_changes_aux(object_type *o_ptr, int changes) {
}

/* sets LFx_ flags on the dungeon master's floor to specific values */
void lua_set_floor_flags(int Ind, u32b flags) {
	getfloor(&Players[Ind]->wpos)->flags1 = flags;
}

s32b lua_get_skill_mod(int Ind, int i) {
	s32b value = 0, mod = 0;
	player_type *p_ptr = Players[Ind];
	compute_skills(p_ptr, &value, &mod, i);
	return mod;
}

s32b lua_get_skill_value(int Ind, int i) {
	s32b value = 0, mod = 0;
	player_type *p_ptr = Players[Ind];
	compute_skills(p_ptr, &value, &mod, i);
	return value;
}

/* fix dual-wield slot position change */
void lua_fix_equip_slots(int Ind) {
	int i;
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;

	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
		if (p_ptr->inventory[i].k_idx &&
		    wield_slot(Ind, &p_ptr->inventory[i]) != i &&
		    !(i == INVEN_ARM && wield_slot(Ind, &p_ptr->inventory[i]) == INVEN_WIELD)) {

			/* Hack - wield_slot always returns INVEN_LEFT for rings because they're already being worn
			 * This prevents the ring in the right hand from being taken off
			 * - mikaelh
			 */
			if ((i == INVEN_RIGHT || i == INVEN_LEFT) &&
			    p_ptr->inventory[i].tval == TV_RING)
				continue;

			bypass_inscrption = TRUE;
			inven_takeoff(Ind, i, 255);
	}
	bypass_inscrption = FALSE;

	/* excise bugged zero-items */
	/* (same as in do_cmd_refresh) */
	/* Hack -- fix the inventory count */
	p_ptr->inven_cnt = 0;
	for (i = 0; i < INVEN_PACK; i++) {
		o_ptr = &p_ptr->inventory[i];
		/* Skip empty items */
		if (!o_ptr->k_idx || !o_ptr->tval) {
			/* hack - make sure the item is really erased - had some bugs there
			   since some code parts use k_idx, and some tval, to kill/test items - C. Blue */
			invwipe(o_ptr);
			continue;
		}
		p_ptr->inven_cnt++;
	}
}

void lua_season_change(int s, int force) {
	season_change(s, (force != 0) ? TRUE : FALSE);
}

#if 0 /* no pointer support / dangerous ? */
/* added for changing seasons via lua cron_24h() - C. Blue */
void lua_get_date(int *weekday, int *day, int *month, int *year)
{
	time_t		now;
	struct tm	*tmp;
	time(&now);
	tmp = localtime(&now);
	*weekday = tmp->tm_wday;
	*day = tmp->tm_mday;
	*month = tmp->tm_mon + 1;
	*year = tmp->tm_mday;
}
#endif

/* for Player-Customizable Tomes feature - C. Blue */
int get_inven_sval(int Ind, int inven_slot) {
	return (Players[Ind]->inventory[inven_slot].sval);
}
int get_inven_xtra(int Ind, int inven_slot, int n) {
	switch (n) {
	case 1: return (Players[Ind]->inventory[inven_slot].xtra1);
	case 2: return (Players[Ind]->inventory[inven_slot].xtra2);
	case 3: return (Players[Ind]->inventory[inven_slot].xtra3);
	case 4: return (Players[Ind]->inventory[inven_slot].xtra4);
	case 5: return (Players[Ind]->inventory[inven_slot].xtra5);
	case 6: return (Players[Ind]->inventory[inven_slot].xtra6);
	case 7: return (Players[Ind]->inventory[inven_slot].xtra7);
	case 8: return (Players[Ind]->inventory[inven_slot].xtra8);
	case 9: return (Players[Ind]->inventory[inven_slot].xtra9);
	default: return (0); //failure
	}
}

/* re-initialize the skill chart, keeping all values though */
//#define MODIFY_DEV_STATE /* mess with expand/collapse status? might be annoying if happens on every login */
void lua_fix_skill_chart(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i, j;

#ifdef MODIFY_DEV_STATE
	for (i = 0; i < MAX_SKILLS; i++)
		p_ptr->s_info[i].dev = FALSE;
#endif
	for (i = 0; i < MAX_SKILLS; i++) {
//		s32b value = 0, mod = 0;
		/* Make sure all are touched */
		p_ptr->s_info[i].touched = TRUE;
//		compute_skills(p_ptr, &value, &mod, i);
//		init_skill(p_ptr, value, mod, i);
		/* pseudo-init-skill */
#if 0 //SMOOTH_SKILLS
		if (s_info[i].flags1 & SKF1_HIDDEN) {
			p_ptr->s_info[i].hidden = TRUE;
		} else {
			p_ptr->s_info[i].hidden = FALSE;
		}
		if (s_info[i].flags1 & SKF1_DUMMY) {
			p_ptr->s_info[i].dummy = TRUE;
		} else {
			p_ptr->s_info[i].dummy = FALSE;
		}
#else
		p_ptr->s_info[i].flags1 = (char)(s_info[i].flags1 & 0xFF);

		/* hack: Rangers can train limited Archery skill */
		if (p_ptr->pclass == CLASS_RANGER && i == SKILL_ARCHERY)
			p_ptr->s_info[i].flags1 |= SKF1_MAX_10;
		/* hack: Druids/Vampires can't train Mimicry skill */
		if ((p_ptr->pclass == CLASS_DRUID || p_ptr->prace == RACE_VAMPIRE)
		    && i == SKILL_MIMIC)
			p_ptr->s_info[i].flags1 |= SKF1_MAX_1;
#endif
		/* Develop only revelant branches */
		if (p_ptr->s_info[i].value || p_ptr->s_info[i].mod) {
			j = s_info[i].father;
			while (j != -1) {
#ifdef MODIFY_DEV_STATE
				p_ptr->s_info[j].dev = TRUE;
#endif
				j = s_info[j].father;
				if (j == 0) break;
			}
		}
	}

	/* hack - fix SKILL_STANCE skill */
	if (get_skill(p_ptr, SKILL_STANCE) && p_ptr->lev <= 50) {
		p_ptr->s_info[SKILL_STANCE].value = p_ptr->lev * 1000;
		/* Update the client */
		Send_skill_info(Ind, SKILL_STANCE, TRUE);
		/* Also update the client's 'm' menu for fighting techniques */
		calc_techniques(Ind);
		Send_skill_info(Ind, SKILL_TECHNIQUE, TRUE);
	}

	p_ptr->update |= PU_SKILL_INFO | PU_SKILL_MOD;
	update_stuff(Ind);
}

void lua_takeoff_costumes(int Ind) {
	player_type *p_ptr = Players[Ind];

	if ((p_ptr->inventory[INVEN_BODY].tval == TV_SOFT_ARMOR) && (p_ptr->inventory[INVEN_BODY].sval == SV_COSTUME)) {
		bypass_inscrption = TRUE;
		inven_takeoff(Ind, INVEN_BODY, 255);
		bypass_inscrption = FALSE;
		msg_print(Ind, "It's not that time of the year anymore.");
	}
}

bool lua_is_unique(int r_idx) {
	if (r_info[r_idx].flags1 & RF1_UNIQUE) return TRUE; else return FALSE;
}

/* Return if a certain race/class combo could in theory learn a monster form if mimicry was high enough */
bool lua_mimic_eligible(int Ind, int r_idx) {
	if (Players[Ind]->prace == RACE_VAMPIRE) {
		return (mimic_vampire(r_idx, Players[Ind]->lev));
	}

	if (Players[Ind]->pclass == CLASS_SHAMAN) {
		return (mimic_shaman(r_idx));
	} else if (Players[Ind]->pclass == CLASS_DRUID) {
		return (mimic_druid(r_idx, Players[Ind]->lev));
	}

	/* also filter out unattainable (in theory) forms */
	if (r_info[r_idx].rarity == 255) return FALSE;
	if ((r_info[r_idx].flags1 & RF1_UNIQUE)) return FALSE;
	if (!mon_allowed_chance(&r_info[r_idx])) return FALSE;

	return TRUE;
}

void swear_set(int i, char *word, int level) {
	strcpy(swear[i].word, word);
	swear[i].level = level;
}
char *swear_get_word(int i) { return swear[i].word; }
int swear_get_level(int i) { return swear[i].level; }

void nonswear_set(int i, char *word) { strcpy(nonswear[i], word); }
char *nonswear_get(int i) { return nonswear[i]; }
