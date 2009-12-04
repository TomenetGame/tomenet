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
#if 0
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
#else
char ang_term_name[10][40] =
{
	"TomeNET",
	"Msg/Chat",
	"Inventory",
	"Chat",
	"Messages",
	"Character",
	"Term-6",
	"Term-7"
};
#endif


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

	{ &c_cfg.rogue_like_commands,	FALSE,	1,	0, 0, TRUE,
	"rogue_like_commands",		"Rogue-like commands" },

	{ &c_cfg.quick_messages,	FALSE,	1,	0, 1, TRUE,
	"quick_messages",		"Activate quick messages (skill etc)" },

	{ &c_cfg.other_query_flag,	FALSE,	1,	0, 2, TRUE,
	"other_query_flag",		"Prompt for various information (mimic polymorph)" },

	{ &c_cfg.carry_query_flag,	FALSE,	1,	0, 3, FALSE,
	"carry_query_flag",		"(broken) Prompt before picking things up" },

	{ &c_cfg.use_old_target,	FALSE,	1,	0, 4, TRUE,
	"use_old_target",		"Use old target by default" },

	{ &c_cfg.always_pickup,		TRUE,	1,	0, 5, TRUE,
	"always_pickup",		"Pick things up by default" },

	{ &c_cfg.always_repeat,		TRUE,	1,	0, 6, TRUE,
	"always_repeat",		"Repeat obvious commands" },

	{ &c_cfg.depth_in_feet,		TRUE,	1,	0, 7, TRUE,
	"depth_in_feet",		"Show dungeon level in feet" },

	{ &c_cfg.stack_force_notes,	FALSE,	1,	0, 8, TRUE,
	"stack_force_notes",		"Merge inscriptions when stacking" },

	{ &c_cfg.stack_force_costs,	FALSE,	1,	0, 9, TRUE,
	"stack_force_costs",		"Merge discounts when stacking" },

	{ &c_cfg.show_labels,		TRUE,	1,	0, 10, FALSE,
	"show_labels",			"(broken) Show labels in object listings" },

	{ &c_cfg.show_weights,		FALSE,	1,	0, 11, TRUE,
	"show_weights",			"Show weights in object listings" },

	{ &c_cfg.show_choices,		FALSE,	1,	0, 12, FALSE,
	"show_choices",			"(broken) Show choices in certain sub-windows" },

	{ &c_cfg.show_details,		FALSE,	1,	0, 13, FALSE,
	"show_details",			"(broken) Show details in certain sub-windows" },

	{ &c_cfg.ring_bell,		TRUE,	1,	0, 14, TRUE,
	"ring_bell",			"Audible bell (on errors, etc)" },

	{ &c_cfg.use_color,		TRUE,	1,	0, 15, TRUE,
	"use_color",			"Use color if possible (slow)" },

	/*** Disturbance ***/

	{ &c_cfg.find_ignore_stairs,	TRUE,	2,	0, 16, TRUE,
	"find_ignore_stairs",		"Run past stairs" },

	{ &c_cfg.find_ignore_doors,	TRUE,	2,	0, 17, TRUE,
	"find_ignore_doors",		"Run through open doors" },

	{ &c_cfg.find_cut,		TRUE,	2,	0, 18, TRUE,
	"find_cut",			"Run past known corners" },

	{ &c_cfg.find_examine,		TRUE,	2,	0, 19, TRUE,
	"find_examine",			"Run into potential corners" },

	{ &c_cfg.disturb_move,		TRUE,	2,	0, 20, TRUE,
	"disturb_move",			"Disturb whenever any monster moves" },

	{ &c_cfg.disturb_near,		TRUE,	2,	0, 21, TRUE,
	"disturb_near",			"Disturb whenever viewable monster moves" },

	{ &c_cfg.disturb_panel,		TRUE,	2,	0, 22, TRUE,
	"disturb_panel",		"Disturb whenever map panel changes" },

	{ &c_cfg.disturb_state,		TRUE,	2,	0, 23, TRUE,
	"disturb_state",		"Disturb whenever player state changes" },

	{ &c_cfg.disturb_minor,		TRUE,	2,	0, 24, TRUE,
	"disturb_minor",		"Disturb whenever boring things happen" },

	{ &c_cfg.disturb_other,		TRUE,	2,	0, 25, TRUE,
	"disturb_other",		"Disturb whenever various things happen" },

	{ &c_cfg.alert_hitpoint,	FALSE,	2,	0, 26, TRUE,
	"alert_hitpoint",		"Alert user to critical hitpoints/sanity" },

	{ &c_cfg.alert_failure,		FALSE,	2,	0, 27, FALSE,
	"alert_failure",		"(broken) Alert user to various failures" },


	/*** Game-Play ***/

	{ &c_cfg.auto_afk,		FALSE,	3,	1, 0, TRUE,	/* former auto_haggle */
	"auto_afk",			"Set 'AFK mode' automatically" },

	{ &c_cfg.newb_suicide,		FALSE,	3,	1, 1, TRUE,	/* former auto_scum */
	"newb_suicide",			"Display newbie suicides" },

	{ &c_cfg.stack_allow_items,	TRUE,	3,	1, 2, TRUE,
	"stack_allow_items",		"Allow weapons and armor to stack" },

	{ &c_cfg.stack_allow_wands,	TRUE,	3,	1, 3, TRUE,
	"stack_allow_wands",		"Allow wands/staffs/rods to stack" },

	{ &c_cfg.expand_look,		FALSE,	3,	1, 4, FALSE,
	"expand_look",			"(broken) Expand the power of the look command" },

	{ &c_cfg.expand_list,		FALSE,	3,	1, 5, FALSE,
	"expand_list",			"(broken) Expand the power of the list commands" },

	{ &c_cfg.view_perma_grids,	TRUE,	3,	1, 6, TRUE,
	"view_perma_grids",		"Map remembers all perma-lit grids" },

	{ &c_cfg.view_torch_grids,	FALSE,	3,	1, 7, TRUE,
	"view_torch_grids",		"Map remembers all torch-lit grids" },

	{ &c_cfg.dungeon_align,		TRUE,	3,	1, 8, FALSE,
	"dungeon_align",		"(obsolete) Generate dungeons with aligned rooms" },

	{ &c_cfg.dungeon_stair,		TRUE,	3,	1, 9, FALSE,
	"dungeon_stair",		"(obsolete) Generate dungeons with connected stairs" },

	{ &c_cfg.recall_flicker,	FALSE,	3,	1, 10, TRUE,
	"recall_flicker",		"Flicker messages in recall" },

	{ &c_cfg.flow_by_smell,		FALSE,	3,	1, 11, FALSE,
	"flow_by_smell",		"(obsolete) Monsters chase recent locations (v.slow)" },

	{ &c_cfg.track_follow,		FALSE,	3,	1, 12, FALSE,
	"track_follow",			"(obsolete) Monsters follow the player (broken)" },

	{ &c_cfg.track_target,		FALSE,	3,	1, 13, FALSE,
	"track_target",			"(obsolete) Monsters target the player (broken)" },

	{ &c_cfg.smart_learn,		FALSE,	3,	1, 14, FALSE,
	"smart_learn",			"(obsolete) Monsters learn from their mistakes" },

	{ &c_cfg.smart_cheat,		FALSE,	3,	1, 15, FALSE,
	"smart_cheat",			"(obsolete) Monsters exploit players weaknesses" },


	/*** Efficiency ***/

	{ &c_cfg.view_reduce_lite,	FALSE,	4,	1, 16, TRUE,	/* (44) */
	"view_reduce_lite",		"Reduce lite-radius when running" },

	{ &c_cfg.view_reduce_view,	FALSE,	4,	1, 17, TRUE,
	"view_reduce_view",		"Reduce view-radius in town" },

	{ &c_cfg.avoid_abort,		FALSE,	4,	1, 18, FALSE,
	"avoid_abort",			"(obsolete) Avoid checking for user abort" },

	{ &c_cfg.avoid_other,		FALSE,	4,	1, 19, FALSE,
	"avoid_other",			"(broken) Avoid processing special colors" },

	{ &c_cfg.flush_failure,		TRUE,	4,	1, 20, FALSE, /* (resurrect me?) */
	"flush_failure",		"(broken) Flush input on various failures" },

	{ &c_cfg.flush_disturb,		FALSE,	4,	1, 21, FALSE,
	"flush_disturb",		"(broken) Flush input whenever disturbed" },

	{ &c_cfg.flush_command,		FALSE,	4,	1, 22, FALSE,
	"flush_command",		"(obsolete) Flush input before every command" },

	{ &c_cfg.fresh_before,		TRUE,	4,	1, 23, FALSE,
	"fresh_before",			"(obsolete) Flush output before every command" },

	{ &c_cfg.fresh_after,		FALSE,	4,	1, 24, FALSE,
	"fresh_after",			"(obsolete) Flush output after every command" },

	{ &c_cfg.fresh_message,		FALSE,	4,	1, 25, FALSE,
	"fresh_message",		"(obsolete) Flush output after every message" },

	{ &c_cfg.compress_savefile,	TRUE,	4,	1, 26, FALSE,
	"compress_savefile",		"(broken) Compress messages in savefiles" },

	{ &c_cfg.hilite_player,		FALSE,	4,	1, 27, FALSE, /* (resurrect me) */
	"hilite_player",		"(broken) Hilite the player with the cursor" },

	{ &c_cfg.view_yellow_lite,	FALSE,	4,	1, 28, TRUE,
	"view_yellow_lite",		"Use special colors for torch-lit grids" },

	{ &c_cfg.view_bright_lite,	FALSE,	4,	1, 29, TRUE,
	"view_bright_lite",		"Use special colors for 'viewable' grids" },

	{ &c_cfg.view_granite_lite,	FALSE,	4,	1, 30, TRUE,
	"view_granite_lite",		"Use special colors for wall grids (slow)" },

	{ &c_cfg.view_special_lite,	FALSE,	4,	1, 31, TRUE,	/* (59) */
	"view_special_lite",		"Use special colors for floor grids (slow)" },


	/*** TomeNET additions ***/

	{ &c_cfg.easy_open,		TRUE,	5,	9, 60, TRUE,
	"easy_open",			"Automatically open doors" },

	{ &c_cfg.easy_disarm,		FALSE,	5,	9, 61, TRUE,
	"easy_disarm",			"Automatically disarm traps" },

	{ &c_cfg.easy_tunnel,		FALSE,	5,	9, 62, TRUE,
	"easy_tunnel",			"Automatically tunnel walls" },

	{ &c_cfg.auto_destroy,		FALSE,	5,	9, 63, FALSE,
	"auto_destroy",			"(broken) No query to destroy known junks" },

	{ &c_cfg.auto_inscribe,		FALSE,	5,	9, 64, TRUE,
	"auto_inscribe",		"Automatically inscribe books and so on" },

	{ &c_cfg.taciturn_messages,	FALSE,	5,	9, 65, TRUE,
	"taciturn_messages",		"Suppress server messages as far as possible" },

	{ &c_cfg.last_words,		TRUE,	5,	9, 66, TRUE,
	"last_words",			"Get last words when the character dies" },

	{ &c_cfg.limit_chat,		FALSE,	5,	9, 67, TRUE,
	"limit_chat",			"Chat only with players on the same floor" },

	{ &c_cfg.thin_down_flush,	TRUE,	5,	9, 68, TRUE,
	"thin_down_flush",		"Thin down screen flush signals to avoid freezing" },

	{ &c_cfg.auto_target,		FALSE,	5,	9, 69, TRUE,
	"auto_target",			"Automatically set target to the nearest enemy" },

	{ &c_cfg.autooff_retaliator,	FALSE,	5,	9, 70, TRUE,
	"autooff_retaliator",		"Stop the retaliator when protected by GoI etc" },

	{ &c_cfg.wide_scroll_margin,	FALSE,	5,	9, 71, TRUE,
	"wide_scroll_margin",		"Scroll the screen more frequently" },

	{ &c_cfg.fail_no_melee,		FALSE,	5,	9, 72, TRUE,
	"fail_no_melee",		"Stay still when item-retaliation fails" },

	{ &c_cfg.always_show_lists,	FALSE,	5,	9, 73, TRUE,
	"always_show_lists",		"Always show lists in item/skill selection" },

	{ &c_cfg.target_history,	FALSE,	5,	9, 74, TRUE,
	"target_history",		"Add target informations to the message history" },

	{ &c_cfg.linear_stats,		FALSE,	5,	9, 75, TRUE,
	"linear_stats",			"Stats are represented in a linear way" },

	{ &c_cfg.exp_need,		FALSE,  5,  9, 76, TRUE,
	"exp_need",			"Show the experience needed for next level" },

	{ &c_cfg.short_item_names,      FALSE, 1, 	0, 77, TRUE,
        "short_item_names", 		"Don't display 'flavours' in item names" },

	{ &c_cfg.disable_flush,		FALSE,	5,	9, 78, TRUE,
	"disable_flush",		"Disable delays from flush signals" },

	/*** End of Table ***/
#if 0
	{ &c_cfg.speak_unique,		TRUE,   2,      13, TRUE, TRUE,
	"speak_unique",                 "Allow shopkeepers and uniques to speak" },
#endif
#if 0
	{ NULL,			0, 0, 0, 0, 0,
	NULL,			NULL }
#endif	/* 0 */
	{ &c_cfg.hide_unusable_skills,	FALSE,	1,	0, 79, TRUE,
	"hide_unusable_skills",		"Hide unusable skills" },
};

/* XXX XXX they should be sent from server like other spells! */
cptr monster_spells4[32] =
{
  "Shriek",
  "Negate magic",
  "XXX",
  "Fire Rocket",

  "Arrow",
  "Shot",
  "Bolt",
  "Missile",

  "Breathe Acid",
  "Breathe Lightning",
  "Breathe Fire",
  "Breathe Cold",

  "Breathe Poison",
  "Breathe Nether",
  "Breathe Lite",
  "Breathe Darkness",

  "Breathe Confusion",
  "Breathe Sound",
  "Breathe Chaos",
  "Breathe Disenchantment",

  "Breathe Nexus",
  "Breathe Time",
  "Breathe Inertia",
  "Breathe Gravity",

  "Breathe Shards",
  "Breathe Plasma",
  "Breathe Force",
  "Breathe Mana",

  "Breathe Disintegration",
  "Breathe Toxic Waste",
  "Ghastly Moan",
  "Throw Boulder",	/* "XXX", */
};

/*
 * New monster race bit flags
 */
cptr monster_spells5[32] =
{
  "Acid Ball",
  "Lightning Ball",
  "Fire Ball",
  "Cold Ball",

  "Poison Ball",
  "Nether Ball",
  "Water Ball",
  "Mana Storm",

  "Darkness Storm",
  "Drain Mana",
  "Mind Blast",
  "Brain Smash",

  "Cause Wounds",
  "XXX",
  "Ball Toxic Waste",
  "Raw Chaos",

  "Acid Bolt",
  "Lightning Bolt",
  "Fire Bolt",
  "Cold Bolt",

  "Poison Bolt",
  "Nether Bolt",
  "Water Bolt",
  "Mana Bolt",

  "Plasma Bolt",
  "Ice Bolt",
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
  "Haste Self",
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
  "Trap Creation",
  "Cause Amnesia",
  /* Summons follow, but players can't summon */
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
  "XXX",
  "XXX",

  "XXX",
  "XXX",
  "XXX",
  "XXX",
};



cptr melee_techniques[16] =
{
  "Sprint",
  "Taunt",
  "Jump",
  "Distract",

#if 0
  "Stab",
  "Slice",
  "Quake",
  "Sweep",

  "Bash",
  "Knock-back",
  "Charge",
  "Flash bomb",

  "Spin",
  "Berserk",
  "Shadow jump",
  "Instant cloak",

#else

  "Bash",
  "Knock-back",
  "Charge",
  "Flash bomb",

  "Cloak",
  "Spin",
  "",
  "Berserk",

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


#ifdef ENABLE_RCRAFT

/*
Runespell elements
*/
r_element r_elements[RCRAFT_MAX_ELEMENTS] =
{
	{ 0, "Heat", 		"aestus", 		1, R_FIRE | R_WAVE | R_BEAM, SKILL_R_FIRECOLD, R_FIRE,},
	{ 1, "Cold", 		"gelum",		1, R_COLD | R_WAVE | R_BEAM, SKILL_R_FIRECOLD, R_COLD,},
	{ 2, "Acid", 		"delibro",	 	1, R_ACID | R_WAVE | R_BOLT, SKILL_R_WATEACID, R_ACID,},
	{ 3, "Water",		"mio",	 		2, R_WATE | R_WAVE | R_CLOU, SKILL_R_WATEACID, R_WATE,},
	{ 4, "Lighting",	"fulmin", 		1, R_ELEC | R_BALL | R_BOLT, SKILL_R_ELECEART, R_ELEC,},
	{ 5, "Earth", 		"ostes", 		2, R_EART | R_SELF | R_BALL, SKILL_R_ELECEART, R_EART,},
	{ 6, "Poison", 		"lepis", 		2, R_POIS | R_SELF | R_CLOU, SKILL_R_WINDPOIS, R_POIS,},
	{ 7, "Wind", 		"ventus", 		1, R_WIND | R_BOLT | R_CLOU, SKILL_R_WINDPOIS, R_WIND,},
	{ 8, "Mana", 		"sacer",	 	3, R_MANA | R_BOLT | R_BEAM, SKILL_R_MANACHAO, R_MANA,},
	{ 9, "Chaos", 		"emuto", 		3, R_CHAO | R_LOS  | R_CLOU, SKILL_R_MANACHAO, R_CHAO,},
	{10, "Force", 		"fero",		 	3, R_FORC | R_BOLT | R_BEAM, SKILL_R_FORCGRAV, R_FORC,},
	{11, "Gravity",		"numen", 		3, R_GRAV | R_BALL | R_CLOU, SKILL_R_FORCGRAV, R_GRAV,},
	{12, "Nether", 		"elido", 		2, R_NETH | R_BOLT | R_BALL, SKILL_R_NETHTIME, R_NETH,},
	{13, "Time", 		"emero",	 	3, R_TIME | R_SELF | R_LOS , SKILL_R_NETHTIME, R_TIME,},
	{14, "Mind",		"cogito", 		3, R_MIND | R_SELF | R_BEAM, SKILL_R_MINDNEXU, R_MIND,},
	{15, "Nexus", 		"vicis", 		2, R_NEXU | R_SELF | R_BALL, SKILL_R_MINDNEXU, R_NEXU,},
};


/*
Runespell imperatives
*/
r_imper r_imperatives [RG_MAX] =
{
	{RG_HOPE, "qua",	 5, 10,  5, 10 },
	{RG_ASKS, "immo",	 8, 10,  8, 10 },
	{RG_REQU, "oratu",	10, 15, 10, 10 },
	{RG_VOLU, "multo",	12, 20, 12, 10 },
	{RG_WILL, "coactu",	12, 20, 10, 20 },
	{RG_MIGH, "armis",	15, 30, 12, 30 },
	{RG_DEMA, "iussu",	18, 40, 15, 40 },
	{RG_LUCK, "forte",	 0,  0,  0, 20 },
};

r_type runespell_types[8] =
/*
Runespell methods.
*/
{
	{ 0, R_MELE, "shield", 	0, 5 },
	{ 1, R_SELF, "self",  	1, 10 },
	{ 2, R_BOLT, "bolt",  	1, 10 },
	{ 3, R_BEAM, "beam",  	2, 11 },
	{ 4, R_BALL, "ball",  	2, 13 },
	{ 5, R_WAVE, "wave",  	3, 12 },
	{ 6, R_CLOU, "cloud", 	3, 15 },
	{ 7, R_LOS,  "sight", 	3, 40 },
};

#endif

