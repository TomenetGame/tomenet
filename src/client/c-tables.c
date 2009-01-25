/* $Id$ */
/* File: tables.c */

/* Purpose: Angband Tables */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"



/*
 * Global array for looping through the "keypad directions"
 */
s16b ddd[9] =
{ 2, 8, 6, 4, 3, 1, 9, 7, 5 };

/*
 * Global arrays for converting "keypad direction" into offsets
 */
s16b ddx[10] =
{ 0, -1, 0, 1, -1, 0, 1, -1, 0, 1 };

s16b ddy[10] =
{ 0, 1, 1, 1, 0, 0, 0, -1, -1, -1 };

/*
 * Global arrays for optimizing "ddx[ddd[i]]" and "ddy[ddd[i]]"
 */
s16b ddx_ddd[9] =
{ 0, 0, 1, -1, 1, -1, 1, -1, 0 };

s16b ddy_ddd[9] =
{ 1, -1, 0, 0, 1, 1, -1, -1, 0 };


/*
 * Global array for converting numbers to uppercase hecidecimal digit
 * This array can also be used to convert a number to an octal digit
 */
char hexsym[16] =
{
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

/*
 * Stat Table (INT/WIS) -- Minimum failure rate (percentage)
 */
byte adj_mag_fail[] =
{
	99	/* 3 */,
	99	/* 4 */,
	99	/* 5 */,
	99	/* 6 */,
	99	/* 7 */,
	50	/* 8 */,
	30	/* 9 */,
	20	/* 10 */,
	15	/* 11 */,
	12	/* 12 */,
	11	/* 13 */,
	10	/* 14 */,
	9	/* 15 */,
	8	/* 16 */,
	7	/* 17 */,
	6	/* 18/00-18/09 */,
	6	/* 18/10-18/19 */,
	5	/* 18/20-18/29 */,
	5	/* 18/30-18/39 */,
	5	/* 18/40-18/49 */,
	4	/* 18/50-18/59 */,
	4	/* 18/60-18/69 */,
	4	/* 18/70-18/79 */,
	4	/* 18/80-18/89 */,
	3	/* 18/90-18/99 */,
	3	/* 18/100-18/109 */,
	2	/* 18/110-18/119 */,
	2	/* 18/120-18/129 */,
	2	/* 18/130-18/139 */,
	2	/* 18/140-18/149 */,
	1	/* 18/150-18/159 */,
	1	/* 18/160-18/169 */,
	1	/* 18/170-18/179 */,
	1	/* 18/180-18/189 */,
	1	/* 18/190-18/199 */,
	0	/* 18/200-18/209 */,
	0	/* 18/210-18/219 */,
	0	/* 18/220+ */
};


/*
 * Stat Table (INT/WIS) -- Various things
 */
byte adj_mag_stat[] =
{
	0	/* 3 */,
	0	/* 4 */,
	0	/* 5 */,
	0	/* 6 */,
	0	/* 7 */,
	1	/* 8 */,
	1	/* 9 */,
	1	/* 10 */,
	1	/* 11 */,
	1	/* 12 */,
	1	/* 13 */,
	1	/* 14 */,
	2	/* 15 */,
	2	/* 16 */,
	2	/* 17 */,
	3	/* 18/00-18/09 */,
	3	/* 18/10-18/19 */,
	3	/* 18/20-18/29 */,
	3	/* 18/30-18/39 */,
	3	/* 18/40-18/49 */,
	4	/* 18/50-18/59 */,
	4	/* 18/60-18/69 */,
	5	/* 18/70-18/79 */,
	6	/* 18/80-18/89 */,
	7	/* 18/90-18/99 */,
	8	/* 18/100-18/109 */,
	9	/* 18/110-18/119 */,
	10	/* 18/120-18/129 */,
	11	/* 18/130-18/139 */,
	12	/* 18/140-18/149 */,
	13	/* 18/150-18/159 */,
	14	/* 18/160-18/169 */,
	15	/* 18/170-18/179 */,
	16	/* 18/180-18/189 */,
	17	/* 18/190-18/199 */,
	18	/* 18/200-18/209 */,
	19	/* 18/210-18/219 */,
	20	/* 18/220+ */
};

/*
 * Each chest has a certain set of traps, determined by pval
 * Each chest has a "pval" from 1 to the chest level (max 55)
 * If the "pval" is negative then the trap has been disarmed
 * The "pval" of a chest determines the quality of its treasure
 * Note that disarming a trap on a chest also removes the lock.
 */
byte chest_traps[64] =
{
	0,					/* 0 == empty */
	(CHEST_POISON),
	(CHEST_LOSE_STR),
	(CHEST_LOSE_CON),
	(CHEST_LOSE_STR),
	(CHEST_LOSE_CON),			/* 5 == best small wooden */
	0,
	(CHEST_POISON),
	(CHEST_POISON),
	(CHEST_LOSE_STR),
	(CHEST_LOSE_CON),
	(CHEST_POISON),
	(CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_SUMMON),			/* 15 == best large wooden */
	0,
	(CHEST_LOSE_STR),
	(CHEST_LOSE_CON),
	(CHEST_PARALYZE),
	(CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_SUMMON),
	(CHEST_PARALYZE),
	(CHEST_LOSE_STR),
	(CHEST_LOSE_CON),
	(CHEST_EXPLODE),			/* 25 == best small iron */
	0,
	(CHEST_POISON | CHEST_LOSE_STR),
	(CHEST_POISON | CHEST_LOSE_CON),
	(CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_PARALYZE),
	(CHEST_POISON | CHEST_SUMMON),
	(CHEST_SUMMON),
	(CHEST_EXPLODE),
	(CHEST_EXPLODE | CHEST_SUMMON),	/* 35 == best large iron */
	0,
	(CHEST_SUMMON),
	(CHEST_EXPLODE),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_POISON | CHEST_PARALYZE),
	(CHEST_EXPLODE),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_POISON | CHEST_PARALYZE),	/* 45 == best small steel */
	0,
	(CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_POISON | CHEST_PARALYZE | CHEST_LOSE_STR),
	(CHEST_POISON | CHEST_PARALYZE | CHEST_LOSE_CON),
	(CHEST_POISON | CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_POISON | CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_POISON | CHEST_PARALYZE | CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_POISON | CHEST_PARALYZE),
	(CHEST_POISON | CHEST_PARALYZE),	/* 55 == best large steel */
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_EXPLODE | CHEST_SUMMON),
};








/*
 * Class titles for the player.
 *
 * The player gets a new title every five levels, so each class
 * needs only ten titles total.
 */
cptr player_title[MAX_CLASS][PY_MAX_LEVEL/5] =
{
	/* Adventurer */
	{
		"Adventurer",
		"Adventurer",
		"Adventurer",
		"Adventurer",
		"Adventurer",
		"Adventurer",
		"Adventurer",
		"Adventurer",
		"Adventurer",
		"Adventurer",
	},

	/* Warrior */
	{
		"Rookie",
		"Soldier",
		"Mercenary",
		"Veteran",
		"Swordsman",
		"Champion",
		"Hero",
		"Baron",
		"Duke",
		"Lord",
	},

	/* Warlock */
	{
		"Novice",
		"Apprentice",
		"Trickster",
		"Illusionist",
		"Spellbinder",
		"Evoker",
		"Conjurer",
		"Warlock",
		"Sorcerer",
		"Mage Lord",
	},

	/* Priest */
	{
		"Believer",
		"Acolyte",
		"Adept",
		"Curate",
		"Canon",
		"Lama",
		"Patriarch",
		"Priest",
		"High Priest",
		"Priest Lord",
	},

	/* Rogues */
	{
		"Vagabond",
		"Cutpurse",
		"Robber",
		"Burglar",
		"Filcher",
		"Sharper",
		"Low Thief",
		"High Thief",
		"Master Thief",
		"Assassin",
	},
	/* Mimic */
	{
                "Copier",
                "Copier",
                "Modifier",
                "Multiple",
                "Multiple",
                "Changer",
                "Metamorph",
                "Metamorph",
                "Shapeshifter",
                "Shapeshifter",
        },

        /* Archer */
	{
                "Rock Thrower",
                "Slinger",
                "Great Slinger",
                "Bowsen",
                "Bowsen",
                "Great Bowmen",
                "Great Bowmen",
                "Archer",
                "Archer",
                "Great Archer",
	},
};



/*
 * Hack -- the "basic" color names (see "TERM_xxx")
 */
cptr color_names[16] =
{
	"Dark",
	"White",
	"Slate",
	"Orange",
	"Red",
	"Green",
	"Blue",
	"Umber",
	"Light Dark",
	"Light Slate",
	"Violet",
	"Yellow",
	"Light Red",
	"Light Green",
	"Light Blue",
	"Light Umber",
};


/*
 * Hack -- the "basic" sound names (see "SOUND_xxx")
 */
cptr sound_names[SOUND_MAX] =
{
	"",
	"hit",
	"miss",
	"flee",
	"drop",
	"kill",
	"level",
	"death",
};



/*
 * Abbreviations of healthy stats
 */
cptr stat_names[6] =
{
	"STR: ", "INT: ", "WIS: ", "DEX: ", "CON: ", "CHR: "
};

/*
 * Abbreviations of damaged stats
 */
cptr stat_names_reduced[6] =
{
	"Str: ", "Int: ", "Wis: ", "Dex: ", "Con: ", "Chr: "
};


/*
 * Standard window names
 */
char ang_term_name[8][40] =
{
	"Angband",
	"Mirror",
	"Recall",
	"Choice",
	"Term-4",
	"Term-5",
	"Term-6",
	"Term-7"
};


/*
 * Certain "screens" always use the main screen, including News, Birth,
 * Dungeon, Tomb-stone, High-scores, Macros, Colors, Visuals, Options.
 *
 * Later, special flags may allow sub-windows to "steal" stuff from the
 * main window, including File dump (help), File dump (artifacts, uniques),
 * Character screen, Small scale map, Previous Messages, Store screen, etc.
 *
 * The "ctrl-i" (tab) command flips the "Display inven/equip" and "Display
 * equip/inven" flags for all windows.
 *
 * The "ctrl-g" command (or pseudo-command) should perhaps grab a snapshot
 * of the main screen into any interested windows.
 */
cptr window_flag_desc[32] =
{
	"Display inven/equip",
	"Display equip/inven",
	"Display spell list",
	"Display character",
	"Display lag-o-meter",
	NULL,
	"Display messages",
	"Display overhead view",
	"Display monster recall",
	"Display object recall",
	NULL,
	"Display snap-shot",
	NULL,
	NULL,
	"Display borg messages",
	"Display borg status",
	NULL,
	NULL,
	"Display chat",
	"Display msgs except chat",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};


/*
 * Available Options
 *
 * Option Screen Sets:
 *
 *	Set 1: User Interface
 *	Set 2: Disturbance
 *	Set 3: Inventory
 *	Set 4: Game Play
 *
 * Note that bits 28-31 of set 0 are currently unused.
 */
/*
 * XXX XXX hard-coded in sync_options!
 */
/*
   bool *o_var;

   byte	o_norm;

   byte	o_page;

   byte	o_set;
   byte	o_bit;

   cptr	o_text;
   cptr	o_desc;
*/
option_type option_info[OPT_MAX] =
{
	/*** User-Interface ***/

	{ &c_cfg.rogue_like_commands,	FALSE,	1,	0, 0,
	"rogue_like_commands",	"Rogue-like commands" },

	{ &c_cfg.quick_messages,	 	FALSE,	1,	0, 1,
	"quick_messages",		"Activate quick messages (skill etc)" },

	{ &c_cfg.other_query_flag,	FALSE,	1,	0, 2,
	"other_query_flag",		"Prompt for various information (mimic polymorph)" },

	{ &c_cfg.carry_query_flag,	FALSE,	1,	0, 3,
	"carry_query_flag",		"(broken) Prompt before picking things up" },

	{ &c_cfg.use_old_target,		FALSE,	1,	0, 4,
	"use_old_target",		"Use old target by default" },

	{ &c_cfg.always_pickup,		TRUE,	1,	0, 5,
	"always_pickup",		"Pick things up by default" },

	{ &c_cfg.always_repeat,		TRUE,	1,	0, 6,
	"always_repeat",		"Repeat obvious commands" },

	{ &c_cfg.depth_in_feet,		TRUE,	1,	0, 7,
	"depth_in_feet",		"Show dungeon level in feet" },

	{ &c_cfg.stack_force_notes,	FALSE,	1,	0, 8,
	"stack_force_notes",	"Merge inscriptions when stacking" },

	{ &c_cfg.stack_force_costs,	FALSE,	1,	0, 9,
	"stack_force_costs",	"Merge discounts when stacking" },

	{ &c_cfg.show_labels,			TRUE,	1,	0, 10,
	"show_labels",			"(broken) Show labels in object listings" },

	{ &c_cfg.show_weights,		FALSE,	1,	0, 11,
	"show_weights",			"Show weights in object listings" },

	{ &c_cfg.show_choices,		FALSE,	1,	0, 12,
	"show_choices",			"(broken) Show choices in certain sub-windows" },

	{ &c_cfg.show_details,		FALSE,	1,	0, 13,
	"show_details",			"(broken) Show details in certain sub-windows" },

	{ &c_cfg.ring_bell,			TRUE,	1,	0, 14,
	"ring_bell",			"Audible bell (on errors, etc)" },

	{ &c_cfg.use_color,			TRUE,	1,	0, 15,
	"use_color",			"Use color if possible (slow)" },

	/*** Disturbance ***/

	{ &c_cfg.find_ignore_stairs,	TRUE,	2,	0, 16,
	"find_ignore_stairs",	"Run past stairs" },

	{ &c_cfg.find_ignore_doors,	TRUE,	2,	0, 17,
	"find_ignore_doors",	"Run through open doors" },

	{ &c_cfg.find_cut,			TRUE,	2,	0, 18,
	"find_cut",				"Run past known corners" },

	{ &c_cfg.find_examine,		TRUE,	2,	0, 19,
	"find_examine",			"Run into potential corners" },

	{ &c_cfg.disturb_move,		TRUE,	2,	0, 20,
	"disturb_move",			"Disturb whenever any monster moves" },

	{ &c_cfg.disturb_near,		TRUE,	2,	0, 21,
	"disturb_near",			"Disturb whenever viewable monster moves" },

	{ &c_cfg.disturb_panel,		TRUE,	2,	0, 22,
	"disturb_panel",		"Disturb whenever map panel changes" },

	{ &c_cfg.disturb_state,		TRUE,	2,	0, 23,
	"disturb_state",		"Disturb whenever player state changes" },

	{ &c_cfg.disturb_minor,		TRUE,	2,	0, 24,
	"disturb_minor",		"Disturb whenever boring things happen" },

	{ &c_cfg.disturb_other,		TRUE,	2,	0, 25,
	"disturb_other",		"Disturb whenever various things happen" },

	{ &c_cfg.alert_hitpoint,		FALSE,	2,	0, 26,
	"alert_hitpoint",		"Alert user to critical hitpoints/sanity" },

	{ &c_cfg.alert_failure,		FALSE,	2,	0, 27,
	"alert_failure",		"(broken) Alert user to various failures" },


	/*** Game-Play ***/

	{ &c_cfg.auto_afk,			FALSE,	3,	1, 0,	/* former auto_haggle */
	"auto_afk",				"Set 'AFK mode' automatically" },

	{ &c_cfg.newb_suicide,			FALSE,	3,	1, 1,	/* former auto_scum */
	"newb_suicide",			"Display newbie suicides" },

	{ &c_cfg.stack_allow_items,	TRUE,	3,	1, 2,
	"stack_allow_items",	"Allow weapons and armor to stack" },

	{ &c_cfg.stack_allow_wands,	TRUE,	3,	1, 3,
	"stack_allow_wands",	"Allow wands/staffs/rods to stack" },

	{ &c_cfg.expand_look,			FALSE,	3,	1, 4,
	"expand_look",			"(broken) Expand the power of the look command" },

	{ &c_cfg.expand_list,			FALSE,	3,	1, 5,
	"expand_list",			"(broken) Expand the power of the list commands" },

	{ &c_cfg.view_perma_grids,	TRUE,	3,	1, 6,
	"view_perma_grids",		"Map remembers all perma-lit grids" },

	{ &c_cfg.view_torch_grids,	FALSE,	3,	1, 7,
	"view_torch_grids",		"Map remembers all torch-lit grids" },

	{ &c_cfg.dungeon_align,		TRUE,	3,	1, 8,
	"dungeon_align",		"(obsolete) Generate dungeons with aligned rooms" },

	{ &c_cfg.dungeon_stair,		TRUE,	3,	1, 9,
	"dungeon_stair",		"(obsolete) Generate dungeons with connected stairs" },

	{ &c_cfg.recall_flicker,		FALSE,	3,	1, 10,
	"recall_flicker",		"Flicker messages in recall" },

	{ &c_cfg.flow_by_smell,		FALSE,	3,	1, 11,
	"flow_by_smell",		"(obsolete) Monsters chase recent locations (v.slow)" },

	{ &c_cfg.track_follow,		FALSE,	3,	1, 12,
	"track_follow",			"(obsolete) Monsters follow the player (broken)" },

	{ &c_cfg.track_target,		FALSE,	3,	1, 13,
	"track_target",			"(obsolete) Monsters target the player (broken)" },

	{ &c_cfg.smart_learn,			FALSE,	3,	1, 14,
	"smart_learn",			"(obsolete) Monsters learn from their mistakes" },

	{ &c_cfg.smart_cheat,			FALSE,	3,	1, 15,
	"smart_cheat",			"(obsolete) Monsters exploit players weaknesses" },


	/*** Efficiency ***/

	{ &c_cfg.view_reduce_lite,	FALSE,	4,	1, 16,	/* (44) */
	"view_reduce_lite",		"Reduce lite-radius when running" },

	{ &c_cfg.view_reduce_view,	FALSE,	4,	1, 17,
	"view_reduce_view",		"Reduce view-radius in town" },

	{ &c_cfg.avoid_abort,			FALSE,	4,	1, 18,
	"avoid_abort",			"(obsolete) Avoid checking for user abort" },

	{ &c_cfg.avoid_other,			FALSE,	4,	1, 19,
	"avoid_other",			"(broken) Avoid processing special colors" },

	{ &c_cfg.flush_failure,		TRUE,	4,	1, 20, /* (resurrect me?) */
	"flush_failure",		"(broken) Flush input on various failures" },

	{ &c_cfg.flush_disturb,		FALSE,	4,	1, 21,
	"flush_disturb",		"(broken) Flush input whenever disturbed" },

	{ &c_cfg.flush_command,		FALSE,	4,	1, 22,
	"flush_command",		"(obsolete) Flush input before every command" },

	{ &c_cfg.fresh_before,		TRUE,	4,	1, 23,
	"fresh_before",			"(obsolete) Flush output before every command" },

	{ &c_cfg.fresh_after,			FALSE,	4,	1, 24,
	"fresh_after",			"(obsolete) Flush output after every command" },

	{ &c_cfg.fresh_message,		FALSE,	4,	1, 25,
	"fresh_message",		"(obsolete) Flush output after every message" },

	{ &c_cfg.compress_savefile,	TRUE,	4,	1, 26,
	"compress_savefile",	"(broken) Compress messages in savefiles" },

	{ &c_cfg.hilite_player,		FALSE,	4,	1, 27, /* (resurrect me) */
	"hilite_player",		"(broken) Hilite the player with the cursor" },

	{ &c_cfg.view_yellow_lite,	FALSE,	4,	1, 28,
	"view_yellow_lite",		"Use special colors for torch-lit grids" },

	{ &c_cfg.view_bright_lite,	FALSE,	4,	1, 29,
	"view_bright_lite",		"Use special colors for 'viewable' grids" },

	{ &c_cfg.view_granite_lite,	FALSE,	4,	1, 30,
	"view_granite_lite",	"Use special colors for wall grids (slow)" },

	{ &c_cfg.view_special_lite,	FALSE,	4,	1, 31,	/* (59) */
	"view_special_lite",	"Use special colors for floor grids (slow)" },


	/*** TomeNET additions ***/

	{ &c_cfg.easy_open,			TRUE,	5,	9, 60,
	"easy_open",			"Automatically open doors" },

	{ &c_cfg.easy_disarm,			FALSE,	5,	9, 61,
	"easy_disarm",			"Automatically disarm traps" },

	{ &c_cfg.easy_tunnel,			FALSE,	5,	9, 62,
	"easy_tunnel",			"Automatically tunnel walls" },

	{ &c_cfg.auto_destroy,		FALSE,	5,	9, 63,
	"auto_destroy",			"(broken) No query to destroy known junks" },

	{ &c_cfg.auto_inscribe,		FALSE,	5,	9, 64,
	"auto_inscribe",		"Automatically inscribe books and so on" },

	{ &c_cfg.taciturn_messages,	FALSE,	5,	9, 65,
	"taciturn_messages",	"Suppress server messages as far as possible" },

	{ &c_cfg.last_words,			TRUE,	5,	9, 66,
	"last_words",			"Get last words when the character dies" },

	{ &c_cfg.limit_chat,			FALSE,	5,	9, 67,
	"limit_chat",			"Chat only with players on the same floor" },

	{ &c_cfg.thin_down_flush,		TRUE,	5,	9, 68,
	"thin_down_flush",		"Thin down screen flush signals to avoid freezing" },

	{ &c_cfg.auto_target,			FALSE,	5,	9, 69,
	"auto_target",			"Automatically set target to the nearest enemy" },

	{ &c_cfg.autooff_retaliator,	FALSE,	5,	9, 70,
	"autooff_retaliator",	"Stop the retaliator when protected by GoI etc" },

	{ &c_cfg.wide_scroll_margin,	FALSE,	5,	9, 71,
	"wide_scroll_margin",	"Scroll the screen more frequently" },

	{ &c_cfg.fail_no_melee,		FALSE,	5,	9, 72,
	"fail_no_melee",		"Stay still when item-retaliation fails" },

	{ &c_cfg.always_show_lists,		FALSE,	5,	9, 73,
	"always_show_lists",	"Always show lists in item/skill selection" },

	{ &c_cfg.target_history,		FALSE,	5,	9, 74,
	"target_history",		"Add target informations to the message history" },

	{ &c_cfg.linear_stats,			FALSE,	5,	9, 75,
	"linear_stats",			"Stats are represented in a linear way" },

	{ &c_cfg.exp_need,				FALSE,  5,  9, 76,
	"exp_need",				"Show the experience needed for next level" },

#if 0
	{ &c_cfg.speak_unique,                TRUE,   2,      13,
	"speak_unique",                 "Allow shopkeepers and uniques to speak" },
#endif	/* 0 */

	{ &c_cfg.short_item_names,      FALSE, 1, 	0, 77,
        "short_item_names", 		"Don't display 'flavours' in item names" },

	/*** End of Table ***/

	{ NULL,			0, 0, 0, 0,
	NULL,			NULL }
};

/* XXX XXX they should be sent from server like other spells! */
cptr monster_spells4[32] =
{
  "Shriek",
  "Negate magic",
  "XXX3",
  "Rocket",
  "Arrow",
  "Bolt",
  "XXX7",
  "XXX8",
  "Breath Acid",
  "Breath Lightning",
  "Breath Fire",
  "Breath Cold",
  "Breath Poison",
  "Breath Nether",
  "Breath Lite",
  "Breath Darkness",
  "Breath Confusion",
  "Breath Sound",
  "Breath Chaos",
  "Breath Disenchantment",
  "Breath Nexus",
  "Breath Time",
  "Breath Inertia",
  "Breath Gravity",
  "Breath Shards",
  "Breath Plasma",
  "Breath Force",
  "Breath Mana",
  "Breath Disintegration",
  "Breath Toxic Waste",
  "XXX",
  "Boulder",	/* "XXX", */
};

/*
 * New monster race bit flags
 */
cptr monster_spells5[32] =
{
  "Ball of Acid",
  "Ball of Lightning",
  "Ball of Fire",
  "Ball of Cold",
  "Ball of Poison",
  "Ball of Nether",
  "Ball of Water",
  "Ball of Mana",
  "Ball of Darkness",
  "Drain Mana",
  "Mind Blast",
  "Brain Smash",
  "Cause Wounds",
  "XXX",
  "Ball of Toxic Waste",
  "Ball of Chaos",
  "Bolt of Acid",
  "Bolt of Lightning",
  "Bolt of Fire",
  "Bolt of Cold",
  "Bolt of Poison",
  "Bolt of Nether",
  "Bolt of Water",
  "Bolt of Mana",
  "Bolt of Plasma",
  "Bolt of Ice",
  "Magic Missile",
  "Scare",
  "Blind",
  "Confusion",
  "Slow",
  "Paralyze",
};

/*
 * New monster race bit flags
 */
cptr monster_spells6[32] =
{
  "Speed",
  "Hand of Doom",
  "Heal",
  "XXX",
  "Blink",
  "Teleport",
  "XXX",
  "XXX",
  "Teleport To",
  "Teleport Away",
  "Teleport Level",
  "XXX",
  "Darkness",
  "Traps",
  "Forget",
  "XXX",
  "XXX",
  "XXX",
  /* Summons follow, but players can't summon */
};



cptr melee_techniques[16] =
{
  "Sprint",
  "Taunt",
  "Spin",
  "Berserk",

#if 0
  "Stab",
  "Slice",
  "Quake",
  "Sweep",

  "Bash",
  "Knock-back",
  "Jump",
  "Charge",

  "Distract",
  "Flash bomb",
  "Shadow jump",
  "Instant cloak",

#else

  "Bash",
  "Knock-back",
  "Jump",
  "Charge",

  "Distract",
  "Flash bomb",
  "Cloak",
  "",

  "",
  "Shadow jump",
  "Shadow run",
  "Instant cloak",
#endif
};

cptr ranged_techniques[16] =
{

  "Flare missile",
  "Precision shot",
  "Craft some ammunition",
  "Double-shot",

  "Barrage",
  "XXX",
  "XXX",
  "XXX",

  "XXX",
  "XXX",
  "XXX",
  "XXX",

  "XXX",
  "XXX",
  "XXX",
  "XXX",
};
