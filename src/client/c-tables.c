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
 * Hack -- the "basic" sound names (see "SOUND_xxx")
 */
//#ifdef USE_SOUND
//#ifndef USE_SOUND_2010
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
//#endif
//#endif


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
char ang_term_name[ANGBAND_TERM_MAX][40] =
{
	"TomeNET",
	"Msg/Chat",
	"Inventory",
	"Character",
	"Chat",
	"Equipment",
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
#if 0
cptr window_flag_desc[32] =
{
	"Display inven/equip",
	"Display equip/inven",
	NULL,
	"Display character",
	"Display lag-o-meter",
	NULL,
	"Display messages/chat",
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
#else
cptr window_flag_desc[8] =
{
	"Display inven/equip",
	"Display equip/inven",
	"Display character",
	"Display non-chat messages",
	"Display all messages",
	"Display chat messages",
	"Display mini-map",
	"Display lag-o-meter",
};
#endif

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
   byte	o_bit;		//deprecated
   byte o_enabled;	//deprecated

   cptr	o_text;
   cptr	o_desc;
*/
option_type option_info[OPT_MAX] = { // there is room for 22 options per page on the (non-bigmap) screen
    //page 1 - 0
	{ &c_cfg.rogue_like_commands,	FALSE,	1,	0, 0, TRUE,
	    "rogue_like_commands",	"Rogue-like keymap (for covering lack of a numpad)" },
	{ &c_cfg.newbie_hints,		TRUE,	1,	0, 1, TRUE,
	    "newbie_hints",		"Display hints/warnings for new players" },
	{ &c_cfg.censor_swearing,	TRUE,	1,	1, 2, TRUE,
	    "censor_swearing",		"Censor certain swear words in public messages" },

	{ &c_cfg.hilite_chat,		TRUE,	1,	0, 3, TRUE,
	    "hilite_chat",		"Highlight chat messages containing your name" },
	{ &c_cfg.hibeep_chat,		TRUE,	1,	0, 4, TRUE,
	    "hibeep_chat",		"Beep on chat messages containing your name" },
	{ &c_cfg.page_on_privmsg,	FALSE,	1,	1, 5, TRUE,
	    "page_on_privmsg",		"Beep when receiving a private message" },
	{ &c_cfg.page_on_afk_privmsg,	TRUE,	1,	1, 6, TRUE,
	    "page_on_afk_privmsg",	"Beep when receiving a private message while AFK" },

	{ &c_cfg.big_map,		FALSE,	1,	1, 7, TRUE,
	    "big_map",			"Double height of the map shown in the main window" },

	{ &c_cfg.font_map_solid_walls,	FALSE,	1,	0, 8, TRUE,
	    "font_map_solid_walls",	"Certain fonts only: Walls look like solid blocks" },
	{ &c_cfg.view_animated_lite,	TRUE,	1,	1, 9, TRUE,
	    "view_animated_lite",	"Animate lantern light, flickering in colour" },
	{ &c_cfg.wall_lighting,		TRUE,	1,	1, 10, TRUE,
	    "wall_lighting",		"Generally enable lighting/shading for wall grids" },
	{ &c_cfg.view_lamp_walls,	TRUE,	1,	1, 11, TRUE,
	    "view_lamp_walls",		"Use special colors for lamp-lit wall grids" },
	{ &c_cfg.view_shade_walls,	TRUE,	1,	1, 12, TRUE,
	    "view_shade_walls",		"Use special colors to shade wall grids" },
	{ &c_cfg.floor_lighting,	TRUE,	1,	1, 13, TRUE,
	    "floor_lighting",		"Generally enable lighting/shading for floor grids" },
	{ &c_cfg.view_lamp_floor,	TRUE,	1,	1, 14, TRUE,
	    "view_lamp_floor",		"Use special colors for lamp-lit floor grids" },
	{ &c_cfg.view_shade_floor,	TRUE,	1,	1, 15, TRUE,
	    "view_shade_floor",		"Use special colors to shade floor grids" },
	{ &c_cfg.view_lite_extra,	FALSE,	1,	9, 16, TRUE,
	    "view_lite_extra",		"Lamp light affects more floor/wall types" },

	{ &c_cfg.alert_hitpoint,	FALSE,	1,	0, 17, TRUE,
	    "alert_hitpoint",		"Beep/message about critical hitpoints/sanity" },
	{ &c_cfg.alert_mana,		FALSE,	1,	0, 18, TRUE,
	    "alert_mana",		"Beep/message about critically low mana pool" },
	{ &c_cfg.alert_afk_dam,		FALSE,	1,	0, 19, TRUE,
	    "alert_afk_dam",		"Beep when taking damage while AFK" },
	{ &c_cfg.alert_offpanel_dam,	FALSE,	1,	0, 20, TRUE,
	    "alert_offpanel_dam",	"Beep when taking damage while looking elsewhere" },

	{ &c_cfg.exp_bar,		TRUE,	1,	9, 21, TRUE,
	    "exp_bar",			"Show experience bar instead of a number" },

    //page 2 - 22
	{ &c_cfg.uniques_alive,		FALSE,	4,	1, 22, TRUE,
	    "uniques_alive",		"List only unslain uniques for your local party" },
	{ &c_cfg.warn_unique_credit,	FALSE,	4,	0, 23, TRUE,
	    "warn_unique_credit",	"Beep on attacking a unique you already killed" },
	{ &c_cfg.limit_chat,		FALSE,	4,	9, 24, TRUE,
	    "limit_chat",		"Chat only with players on the same floor" },
	{ &c_cfg.no_afk_msg,		FALSE,	4,	9, 25, TRUE,
	    "no_afk_msg",		"Don't show AFK toggle messages of other players" },
	{ &c_cfg.overview_startup,	FALSE,	4,	1, 26, TRUE,
	    "overview_startup",		"Display overview resistance/boni page at startup" },

	{ &c_cfg.allow_paging,		TRUE,	4,	0, 27, TRUE,
	    "allow_paging",		"Allow users to page you (recommended!)" },
	{ &c_cfg.ring_bell,		TRUE,	4,	0, 28, TRUE,
	    "ring_bell",		"Beep on misc warnings and errors" },

	{ &c_cfg.linear_stats,		FALSE,	4,	9, 29, TRUE,
	    "linear_stats",			"Stats are represented in a linear way" },
	{ &c_cfg.exp_need,		FALSE,	4,	9, 30, TRUE,
	    "exp_need",			"Show the experience needed for next level" },
	{ &c_cfg.depth_in_feet,		TRUE,	4,	0, 31, TRUE,
	    "depth_in_feet",		"Show dungeon level in feet" },
	{ &c_cfg.newb_suicide,		TRUE,	4,	1, 32, TRUE,
	    "newb_suicide",		"Display newbie suicides" },
	{ &c_cfg.show_weights,		TRUE,	4,	0, 33, TRUE,
	    "show_weights",		"Show weights in object listings" },
    /* currently problematic: best might be to move line-splitting to client side, from util.c
       For now, let's just insert hourly chat marker lines instead. - C. Blue */
	{ &c_cfg.time_stamp_chat,	FALSE,	4,	0, 34, TRUE,
	    "time_stamp_chat",		"Add hourly time stamps to chat window" },
	{ &c_cfg.hide_unusable_skills,	TRUE,	4,	0, 35, TRUE,
	    "hide_unusable_skills",	"Hide unusable skills" },
	{ &c_cfg.short_item_names,	FALSE,	4,	0, 36, TRUE,
	    "short_item_names", 	"Don't display known 'flavours' in item names" },
	{ &c_cfg.keep_topline,		FALSE,	4,	0, 37, TRUE,
	    "keep_topline",		"Don't clear messages in the top line if avoidable" },
	{ &c_cfg.target_history,	FALSE,	4,	9, 38, TRUE,
	    "target_history",		"Add target informations to the message history" },
	{ &c_cfg.taciturn_messages,	FALSE,	4,	9, 39, TRUE,
	    "taciturn_messages",	"Suppress server messages as far as possible" },
	{ &c_cfg.always_show_lists,	TRUE,	4,	9, 40, TRUE,
	    "always_show_lists",	"Always show lists in item/skill selection" },

	{ &c_cfg.no_weather,		FALSE,	4,	1, 41, TRUE,
	    "no_weather",		"Disable weather visuals and sounds completely" },

	{ &c_cfg.player_list,		FALSE,	4,	1, 42, TRUE,
	    "player_list",		"Show a more compact player list in @ screen" },
	{ &c_cfg.player_list2,		FALSE,	4,	1, 43, TRUE,
	    "player_list2",		"Compacts the player list in @ screen even more" },

    //page 3 - 44
	{ &c_cfg.flash_player,		FALSE,	6,	1, 44, TRUE,
	    "flash_player",		"Flash own character icon after far relocation" },
    //todo: fix/implement good cursor on *nix/osx
	{ &c_cfg.hilite_player,		FALSE,	6,	1, 45, FALSE,
	    "hilite_player",		"Hilite own character icon with the cursor" },
	{ &c_cfg.consistent_players,	FALSE,	6,	1, 46, TRUE,
	    "consistent_players",	"Use consistent symbols and colours for players" },

	{ &c_cfg.recall_flicker,	TRUE,	6,	1, 47, TRUE,
	    "recall_flicker",		"Show animated text colours in sub-windows" },
	{ &c_cfg.no_verify_sell,	FALSE,	6,	0, 48, TRUE,
	    "no_verify_sell",		"Skip safety question when selling items" },
	{ &c_cfg.no_verify_destroy,	FALSE,	6,	0, 49, TRUE,
	    "no_verify_destroy",	"Skip safety question when destroying items" },
	//HOLE:14

    //page 4 - 49
	{ &c_cfg.auto_afk,		TRUE,	2,	1, 50, TRUE,
	    "auto_afk",			"Set 'AFK mode' automatically" },
	{ &c_cfg.idle_starve_kick,	TRUE,	2,	1, 51, TRUE,
	    "idle_starve_kick",		"Disconnect when idle for 30s while starving" },
	{ &c_cfg.safe_float,		FALSE,	2,	1, 52, TRUE,
	    "safe_float",		"Prevent floating for a short while after death" },
	{ &c_cfg.safe_macros,		TRUE,	2,	1, 53, TRUE,
	    "safe_macros",		"Abort macro if item is missing or an action fails" },

	{ &c_cfg.auto_untag,		FALSE,	2,	1, 54, TRUE,
	    "auto_untag",		"Remove unique monster inscription on pick-up" },
	{ &c_cfg.clear_inscr,		FALSE,	2,	9, 55, TRUE,
	    "clear_inscr",		"Clear @-inscriptions on taking item ownership" },
	{ &c_cfg.auto_inscribe,		FALSE,	2,	9, 56, TRUE,
	    "auto_inscribe",		"Use additional predefined auto-inscriptions" },
	{ &c_cfg.stack_force_notes,	TRUE,	2,	0, 57, TRUE,
	    "stack_force_notes",	"Merge inscriptions when stacking" },
	{ &c_cfg.stack_force_costs,	TRUE,	2,	0, 58, TRUE,
	    "stack_force_costs",	"Merge discounts when stacking" },
	{ &c_cfg.stack_allow_items,	TRUE,	2,	1, 59, TRUE,
	    "stack_allow_items",	"Allow weapons and armor to stack" },
	{ &c_cfg.stack_allow_wands,	TRUE,	2,	1, 60, TRUE,
	    "stack_allow_wands",	"Allow wands/staffs/rods to stack" },
	{ &c_cfg.whole_ammo_stack,	FALSE,	2,	0, 61, TRUE,
	    "whole_ammo_stack",		"For ammo/misc items always operate on whole stack" },
	{ &c_cfg.always_repeat,		TRUE,	2,	0, 62, TRUE,
	    "always_repeat",		"Repeat obvious commands (eg search/tunnel)" },
	{ &c_cfg.always_pickup,		FALSE,	2,	0, 63, TRUE,
	    "always_pickup",		"Pick things up by default" },
	{ &c_cfg.use_old_target,	TRUE,	2,	0, 64, TRUE,
	    "use_old_target",		"Use old target by default" },
	{ &c_cfg.autooff_retaliator,	FALSE,	2,	9, 65, TRUE,
	    "autooff_retaliator",	"Stop the retaliator when protected by GoI etc" },
	{ &c_cfg.fail_no_melee,		FALSE,	2,	9, 66, TRUE,
	    "fail_no_melee",		"Don't melee if other auto-retaliation ways fail" },
	{ &c_cfg.wide_scroll_margin,	TRUE,	2,	9, 67, TRUE,
	    "wide_scroll_margin",	"Scroll the screen more frequently" },
	{ &c_cfg.auto_target,		FALSE,	2,	9, 68, TRUE,
	    "auto_target",		"Automatically set target to the nearest enemy" },
	{ &c_cfg.thin_down_flush,	TRUE,	2,	9, 69, TRUE,
	    "thin_down_flush",		"Thin down screen flush signals to avoid freezing" },
	{ &c_cfg.disable_flush,		FALSE,	2,	9, 70, TRUE,
	    "disable_flush",		"Disable delays from flush signals" },
	//HOLE:1

    //page 5 - 70
    /*** Disturbance ***/
	{ &c_cfg.find_ignore_stairs,	FALSE,	3,	0, 71, TRUE,
	    "find_ignore_stairs",	"Run past stairs" },
	{ &c_cfg.find_ignore_doors,	TRUE,	3,	0, 72, TRUE,
	    "find_ignore_doors",	"Run through open doors" },
	{ &c_cfg.find_cut,		TRUE,	3,	0, 73, TRUE,
	    "find_cut",			"Run past known corners" },
	{ &c_cfg.find_examine,		TRUE,	3,	0, 74, TRUE,
	    "find_examine",		"Run into potential corners" },
	{ &c_cfg.disturb_move,		FALSE,	3,	0, 75, TRUE,
	    "disturb_move",		"Disturb whenever any monster moves" },
	{ &c_cfg.disturb_near,		FALSE,	3,	0, 76, TRUE,
	    "disturb_near",		"Disturb whenever viewable monster moves" },
	{ &c_cfg.disturb_panel,		FALSE,	3,	0, 77, TRUE,
	    "disturb_panel",		"Disturb whenever map panel changes" },
	{ &c_cfg.disturb_state,		FALSE,	3,	0, 78, TRUE,
	    "disturb_state",		"Disturb whenever player state changes" },
	{ &c_cfg.disturb_minor,		FALSE,	3,	0, 79, TRUE,
	    "disturb_minor",		"Disturb whenever boring things happen" },
	{ &c_cfg.disturb_other,		FALSE,	3,	0, 80, TRUE,
	    "disturb_other",		"Disturb whenever various things happen" },//18
	{ &c_cfg.view_perma_grids,	TRUE,	3,	1, 81, TRUE,
	    "view_perma_grids",		"Map remembers all perma-lit grids" },
	{ &c_cfg.view_torch_grids,	FALSE,	3,	1, 82, TRUE,
	    "view_torch_grids",		"Map remembers all torch-lit grids" },
	{ &c_cfg.view_reduce_lite,	FALSE,	3,	1, 83, TRUE,
	    "view_reduce_lite",		"Reduce lite-radius when running" },
	{ &c_cfg.view_reduce_view,	FALSE,	3,	1, 84, TRUE,
	    "view_reduce_view",		"Reduce view-radius in town" },

	{ &c_cfg.easy_open,		TRUE,	3,	9, 85, TRUE,
	    "easy_open",		"Automatically open doors" },
	{ &c_cfg.easy_disarm,		FALSE,	3,	9, 86, TRUE,
	    "easy_disarm",		"Automatically disarm traps (except under items)" },
	{ &c_cfg.easy_tunnel,		FALSE,	3,	9, 87, TRUE,
	    "easy_tunnel",		"Automatically tunnel walls" },
	//HOLE: 4

    //page 6 - 87
	{ &c_cfg.audio_paging,		TRUE,	5,	0, 88, TRUE,
	    "audio_paging",		"Use audio system for page/alert, if available" },
	{ &c_cfg.paging_master_volume,	FALSE,	5,	0, 89, TRUE,
	    "paging_master_vol",	"Play page/alert sounds at master volume" },
	{ &c_cfg.paging_max_volume,	FALSE,	5,	0, 90, TRUE,
	    "paging_max_vol",		"Play page/alert sounds at maximum volume" },
	{ &c_cfg.no_ovl_close_sfx,	TRUE,	5,	0, 91, TRUE,
	    "no_ovl_close_sfx",		"Prevent re-playing sfx received after <100ms gap" },
	{ &c_cfg.ovl_sfx_attack,	TRUE,	5,	0, 92, TRUE,
	    "ovl_sfx_attack",		"Allow overlapping combat sounds of same type" },
	{ &c_cfg.no_combat_sfx,		FALSE,	5,	1, 93, TRUE,
	    "no_combat_sfx",		"Don't play melee/launcher attack/miss sound fx" },
	{ &c_cfg.no_magicattack_sfx,	FALSE,	5,	1, 94, TRUE,
	    "no_magicattack_sfx",	"Don't play basic spell/device attack sound fx" },
	{ &c_cfg.no_defense_sfx,	FALSE,	5,	1, 95, TRUE,
	    "no_defense_sfx",		"Don't play attack-avoiding/neutralizing sound fx" },
	{ &c_cfg.half_sfx_attack,	FALSE,	5,	0, 96, TRUE,
	    "half_sfx_attack",		"Skip every second attack sound" },
	{ &c_cfg.cut_sfx_attack,	FALSE,	5,	0, 97, TRUE,
	    "cut_sfx_attack",		"Skip attack sounds based on speed and bpr" },
	{ &c_cfg.ovl_sfx_command,	TRUE,	5,	0, 98, TRUE,
	    "ovl_sfx_command",		"Allow overlapping command sounds of same type" },
	{ &c_cfg.ovl_sfx_misc,		TRUE,	5,	0, 99, TRUE,
	    "ovl_sfx_misc",		"Allow overlapping misc sounds of same type" },
	{ &c_cfg.ovl_sfx_mon_attack,	TRUE,	5,	0, 100, TRUE,
	    "ovl_sfx_mon_attack",	"Allow overlapping monster attack sfx of same type" },
	{ &c_cfg.ovl_sfx_mon_spell,	TRUE,	5,	0, 101, TRUE, /* includes breaths, basically it's all S-flags */
	    "ovl_sfx_mon_spell",	"Allow ovl. monster spell/breath sfx of same type" },
	{ &c_cfg.ovl_sfx_mon_misc,	TRUE,	5,	0, 102, TRUE,
	    "ovl_sfx_mon_misc",		"Allow overlapping misc monster sfx of same type" },
	{ &c_cfg.no_monsterattack_sfx,	FALSE,	5,	1, 103, TRUE,
	    "no_monsterattack_sfx",	"Don't play basic monster attack sound fx" },
	{ &c_cfg.no_shriek_sfx,		FALSE,	5,	1, 104, TRUE,
	    "no_shriek_sfx",		"Don't play shriek (monster hasting) sound fx" },
	{ &c_cfg.no_store_bell,		FALSE,	5,	1, 105, TRUE,
	    "no_store_bell",		"Don't play sound fx when entering/leaving a store" },
	{ &c_cfg.quiet_house_sfx,	TRUE,	5,	1, 106, TRUE,
	    "quiet_house_sfx",		"Play quieter ambient/weather sound in buildings" },
	{ &c_cfg.no_house_sfx,		FALSE,	5,	1, 107, TRUE,
	    "no_house_sfx",		"Don't play ambient/weather sound in buildings" },
	{ &c_cfg.no_am_sfx,		FALSE,	5,	1, 108, TRUE,
	    "no_am_sfx",		"Don't play anti-magic disruption sound effect" },
	//HOLE: 1

    /* unmutable options, pfft -- these are never shown in any options menu (-> FALSE) */
	{ &c_cfg.use_color,		TRUE,	1,	0, 152, FALSE,//works, but pretty useless - disabled to make room (we always use colours nowadays)
	    "use_color",		"(deprecated) Use color if possible" },
	{ &c_cfg.other_query_flag,	FALSE,	2,	0, 153, FALSE,
	    "other_query_flag",		"Prompt for various information (mimic polymorph)" },

    /* deprecated/broken/todo options */
#if 0
	{ &c_cfg.quick_messages,	FALSE,	6,	0, 1, TRUE,
	    "quick_messages",		"Activate quick messages (skill etc)" },
	{ &c_cfg.carry_query_flag,	FALSE,	3,	0, 3, FALSE,
	    "carry_query_flag",		"(broken) Prompt before picking things up" },
	{ &c_cfg.show_labels,		TRUE,	6,	0, 10, FALSE,
	    "show_labels",		"(broken) Show labels in object listings" },
	{ &c_cfg.show_choices,		FALSE,	6,	0, 12, FALSE,
	    "show_choices",		"(broken) Show choices in certain sub-windows" },
	{ &c_cfg.show_details,		TRUE,	6,	0, 13, FALSE,
	    "show_details",		"(broken) Show details in certain sub-windows" },
	{ &c_cfg.expand_look,		FALSE,	6,	1, 4, FALSE,
	    "expand_look",		"(broken) Expand the power of the look command" },
	{ &c_cfg.expand_list,		FALSE,	6,	1, 5, FALSE,
	    "expand_list",		"(broken) Expand the power of the list commands" },
	{ &c_cfg.avoid_other,		FALSE,	6,	1, 19, FALSE,
	    "avoid_other",		"(broken) Avoid processing special colors" },
	{ &c_cfg.flush_failure,		TRUE,	6,	1, 20, FALSE, /* (resurrect me?) */
	    "flush_failure",		"(broken) Flush input on various failures" },
	{ &c_cfg.flush_disturb,		FALSE,	6,	1, 21, FALSE,
	    "flush_disturb",		"(broken) Flush input whenever disturbed" },
	{ &c_cfg.fresh_after,           FALSE,  6,      1, 24, FALSE,
	    "fresh_after",		"(obsolete) Flush output after every command" },
	{ &c_cfg.auto_destroy,		FALSE,	3,	9, 63, FALSE,
	    "auto_destroy",		"(broken) No query to destroy known junks" },
    //todo: check shopkeeper speakage
	{ &c_cfg.speak_unique,		TRUE,	6,	13, xx, TRUE,
	    "speak_unique",		"Allow shopkeepers and uniques to speak" },
#endif

    /* new additions for 4.6.2 */
	{ &c_cfg.shuffle_music,		FALSE,	5,	0, 109, TRUE,
	    "shuffle_music",		"Don't loop song files but shuffle through them" },
	{ &c_cfg.permawalls_shade,	FALSE,	6,	0, 110, TRUE, //page 3 (UI 3)
	    "permawalls_shade",		"Display permanent vault walls in a special colour" },
	{ &c_cfg.topline_no_msg,	FALSE,	6,	0, 111, TRUE, //page 3 (UI 3)
	    "topline_no_msg",		"Don't display messages in main window top line" },
	{ &c_cfg.targetinfo_msg,	FALSE,	6,	0, 112, TRUE, //page 3 (UI 3)
	    "targetinfo_msg",		"Display look/target info in message window too" },
	{ &c_cfg.live_timeouts,		TRUE,	6,	0, 113, TRUE, //page 3 (UI 3)
	    "live_timeouts",		"Always update item timeout numbers on every tick" },
	{ &c_cfg.flash_insane,		FALSE,	6,	0, 114, TRUE, //page 3 (UI 3)
	    "flash_insane",		"Flash own character icon when going badly insane" },
    /* 4.7.1: */
	{ &c_cfg.last_words,		TRUE,	6,	9, 115, TRUE,
	    "last_words",		"Get last words when the character dies" },
	{ &c_cfg.disturb_see,		FALSE,	3,	0, 116, TRUE,
	    "disturb_see",		"Disturb whenever seeing any monster" },
};


cptr melee_techniques[16] = {
  "Sprint",
  "Taunt",
  "TechA", /*"Jump",*/
  "Distract",

#if 0
  "Stab", //swords
  "Slice", //axes
  "Quake", //blunts
  "Sweep", //polearms
#endif

  "Apply Poison",
  "TechB", /*"Knock Back",*/
  "Track Animals", /*"Charge",*/
  "Perceive Noise",

  "Flash Bomb",
  "TechD", /*"Cloak",*/
  "Spin",
  "Assassinate",

  "Berserk",
  "TechE", /*"Shadow Jump",*/
  "Shadow Run",
  "TechF", /*"Instant Cloak",*/
};

cptr ranged_techniques[16] =
{

  "Flare Missile",
  "Precision Shot",
  "Craft Ammunition",
  "Double Shot",

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

