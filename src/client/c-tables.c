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
char hexsym[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

/*
 * Hack -- the "basic" sound names (see "SOUND_xxx")
 */
//#if defined(USE_SOUND) && !defined(USE_SOUND_2010) /* It's instead defined in snd-sdl.c, different sound system */
cptr sound_names[SOUND_MAX] = {
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

/*
 * Abbreviations of healthy stats
 */
cptr stat_names[6] = { "STR: ", "INT: ", "WIS: ", "DEX: ", "CON: ", "CHR: " };

/*
 * Abbreviations of damaged stats
 */
cptr stat_names_reduced[6] = { "Str: ", "Int: ", "Wis: ", "Dex: ", "Con: ", "Chr: " };

/*
 * Standard window names
 */
char ang_term_name[ANGBAND_TERM_MAX][40] = {
	"TomeNET",
	"Msg/Chat",
	"Inventory",
	"Character",
	"Chat",
	"Equipment",
	"Bags",
	"Term-7",
	"Term-8",
	"Term-9"
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
cptr window_flag_desc[32] = {
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
cptr window_flag_desc[NR_OPTIONS_SHOWN] = {
	"Display inven/equip",
	"Display equip/inven",
	"Display character",
	"Display non-chat messages",
	"Display all messages",
	"Display chat messages",
	"Display clone-map",
	"Display lag-o-meter",
	"Display player list",
	"Display character boni",
	"Display subinventory",
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

   byte	o_set;		//unused/deprecated
   byte	o_bit;		//deprecated
   byte o_enabled;	//is the option even available aka listed in the options menu?

   cptr	o_text;
   cptr	o_desc;
*/
//page layout up to 4.9.2: #(ui:)1,4,6,7,(audio:)5,9,(gameplay):2,3,8
option_type option_info[OPT_MAX] = { // there is room for 22 options per page on the (non-bigmap) screen
	{ &c_cfg.rogue_like_commands,	FALSE,	1,	0, 0, TRUE,
	    "rogue_like_commands",	"Rogue-like keyset (for covering lack of a numpad)" },
	{ &c_cfg.newbie_hints,		TRUE,	1,	0, 1, TRUE,
	    "newbie_hints",		"Display tips/warnings for new players" },
	{ &c_cfg.censor_swearing,	TRUE,	1,	0, 2, TRUE,
	    "censor_swearing",		"Censor certain swear words in public messages" },

	{ &c_cfg.hilite_chat,		TRUE,	7,	0, 3, TRUE,
	    "highlight_chat",		"Highlight chat messages containing your name" },
	{ &c_cfg.hibeep_chat,		TRUE,	7,	0, 4, TRUE,
	    "highbeep_chat",		"Beep on chat messages containing your name" },
	{ &c_cfg.page_on_privmsg,	FALSE,	7,	0, 5, TRUE,
	    "page_on_privmsg",		"Beep when receiving a private message" },
	{ &c_cfg.page_on_afk_privmsg,	TRUE,	7,	0, 6, TRUE,
	    "page_on_afk_privmsg",	"Beep when receiving a private message while AFK" },

	//inactive
#ifndef GLOBAL_BIG_MAP
	{ &c_cfg.big_map,		FALSE,	1,	0, 7, TRUE,
	    "big_map",			"Double height of the map shown in the main window" },
#else
	{ &c_cfg.big_map,		FALSE,	1,	0, 7, FALSE,
	    "big_map",			"Double height of the map shown in the main window" },
#endif

	{ &c_cfg.font_map_solid_walls,	TRUE,	4,	0, 8, TRUE,
	    "font_map_solid_walls",	"Certain fonts only: Walls look like solid blocks" },
	{ &c_cfg.view_animated_lite,	TRUE,	4,	0, 9, TRUE,
	    "view_animated_light",	"Animate lantern light, flickering in colour" },
	{ &c_cfg.wall_lighting,		TRUE,	4,	0, 10, TRUE,
	    "wall_lighting",		"Generally enable lighting/shading for wall grids" },
	{ &c_cfg.view_lamp_walls,	TRUE,	4,	0, 11, TRUE,
	    "view_lamp_walls",		"Use special colors for lamp-lit wall grids" },
	{ &c_cfg.view_shade_walls,	TRUE,	4,	0, 12, TRUE,
	    "view_shade_walls",		"Use special colors to shade wall grids" },
	{ &c_cfg.floor_lighting,	TRUE,	4,	0, 13, TRUE,
	    "floor_lighting",		"Generally enable lighting/shading for floor grids" },
	{ &c_cfg.view_lamp_floor,	TRUE,	4,	0, 14, TRUE,
	    "view_lamp_floor",		"Use special colors for lamp-lit floor grids" },
	{ &c_cfg.view_shade_floor,	TRUE,	4,	0, 15, TRUE,
	    "view_shade_floor",		"Use special colors to shade floor grids" },
	{ &c_cfg.view_lite_extra,	TRUE,	4,	9, 16, TRUE,
	    "view_light_extra",		"Lamp light affects more floor/wall types" },

	{ &c_cfg.alert_hitpoint,	FALSE,	7,	0, 17, TRUE,
	    "alert_hitpoint",		"Beep/message about critical hitpoints/sanity" },
	{ &c_cfg.alert_mana,		FALSE,	7,	0, 18, TRUE,
	    "alert_mana",		"Beep/message about critically low mana pool" },
	{ &c_cfg.alert_afk_dam,		FALSE,	7,	0, 19, TRUE,
	    "alert_afk_dam",		"Beep when taking damage while AFK" },
	{ &c_cfg.alert_offpanel_dam,	TRUE,	7,	0, 20, TRUE,
	    "alert_offpanel_dam",	"Beep when taking damage while looking elsewhere" },

	{ &c_cfg.exp_bar,		TRUE,	1,	0, 21, TRUE, //moved to page 3 in 4.7.2 to make room for alert_starvation
	    "exp_bar",			"Show experience bar instead of a number" },

	{ &c_cfg.uniques_alive,		FALSE,	6,	0, 22, TRUE,
	    "uniques_alive",		"List only unslain uniques for your local party" },
	{ &c_cfg.warn_unique_credit,	FALSE,	7,	0, 23, TRUE,
	    "warn_unique_credit",	"Beep on attacking a unique you already killed" },
	{ &c_cfg.limit_chat,		FALSE,	0,	0, 24, TRUE,
	    "limit_chat",		"Chat only with players on the same floor" },
	{ &c_cfg.no_afk_msg,		FALSE,	6,	0, 25, TRUE,
	    "no_afk_msg",		"Don't show AFK toggle messages of other players" },
	{ &c_cfg.overview_startup,	FALSE,	6,	0, 26, TRUE,
	    "overview_startup",		"Display overview resistance/boni page at startup" },

	{ &c_cfg.allow_paging,		TRUE,	7,	0, 27, TRUE,
	    "allow_paging",		"Allow users to page you (recommended!)" },
	{ &c_cfg.ring_bell,		TRUE,	7,	0, 28, TRUE,
	    "ring_bell",		"Beep on misc warnings and errors" },


	{ &c_cfg.linear_stats,		FALSE,	6,	0, 29, TRUE,
	    "linear_stats",			"Stats are represented in a linear way" },
	{ &c_cfg.exp_need,		FALSE,	6,	0, 30, TRUE,
	    "exp_need",			"Show the experience needed for next level" },
	{ &c_cfg.depth_in_feet,		TRUE,	6,	0, 31, TRUE,
	    "depth_in_feet",		"Show dungeon level in feet" },
	{ &c_cfg.newb_suicide,		TRUE,	7,	0, 32, TRUE,
	    "newb_suicide",		"Display newbie suicides" },
	{ &c_cfg.show_weights,		TRUE,	6,	0, 33, TRUE,
	    "show_weights",		"Show weights in object listings" },
    /* currently problematic: best might be to move line-splitting to client side, from util.c
       For now, let's just insert hourly chat marker lines instead. - C. Blue */
	{ &c_cfg.time_stamp_chat,	FALSE,	0,	0, 34, TRUE,
	    "time_stamp_chat",		"Add half-hourly time stamps to chat window" },
	{ &c_cfg.hide_unusable_skills,	TRUE,	6,	0, 35, TRUE,
	    "hide_unusable_skills",	"Hide unusable skills" },
	{ &c_cfg.short_item_names,	FALSE,	6,	0, 36, TRUE,
	    "short_item_names", 	"Don't display known 'flavours' in item names" },
	{ &c_cfg.keep_topline,		FALSE,	0,	0, 37, TRUE,
	    "keep_topline",		"Don't clear messages in the top line if avoidable" },
	{ &c_cfg.target_history,	FALSE,	0,	0, 38, TRUE,
	    "target_history",		"Add target informations to the message history" },
	{ &c_cfg.taciturn_messages,	FALSE,	0,	0, 39, TRUE,
	    "taciturn_messages",	"Suppress server messages as far as possible" },
	{ &c_cfg.always_show_lists,	TRUE,	6,	0, 40, TRUE,
	    "always_show_lists",	"Always show lists in item/skill selection" },

	{ &c_cfg.no_weather,		FALSE,	4,	0, 41, TRUE,
	    "no_weather",		"Disable weather visuals and sounds completely" },

	{ &c_cfg.player_list,		FALSE,	6,	0, 42, TRUE,
	    "player_list",		"Show a more compact player list in @ screen" },
	{ &c_cfg.player_list2,		FALSE,	6,	0, 43, TRUE,
	    "player_list2",		"Compacts the player list in @ screen even more" },

	{ &c_cfg.flash_player,		TRUE,	7,	0, 44, TRUE,
	    "flash_player",		"Flash own character icon after far relocation" },
    //todo: fix/implement good cursor on *nix/osx
	{ &c_cfg.hilite_player,		FALSE,	7,	0, 45, TRUE,
	    "highlight_player",		"Highlight own character icon with the cursor" },
	{ &c_cfg.basic_players_symb,	FALSE,	1,	0, 46, TRUE,
	    "basic_players_symb",	"Use very basic static symbols for players" },

	{ &c_cfg.recall_flicker,	TRUE,	4,	0, 47, TRUE,
	    "recall_flicker",		"Show animated text colours in sub-windows" },
	{ &c_cfg.no_verify_sell,	FALSE,	8,	0, 48, TRUE,
	    "no_verify_sell",		"Skip safety question when selling items" },
	{ &c_cfg.no_verify_destroy,	FALSE,	8,	0, 49, TRUE,
	    "no_verify_destroy",	"Skip safety question when destroying items" },

	{ &c_cfg.auto_afk,		TRUE,	2,	0, 50, TRUE,
	    "auto_afk",			"Set 'AFK mode' automatically" },
	{ &c_cfg.idle_starve_kick,	TRUE,	2,	0, 51, TRUE,
	    "idle_starve_kick",		"Disconnect when idle for 30s while starving" },
	{ &c_cfg.safe_float,		FALSE,	2,	0, 52, TRUE,
	    "safe_float",		"Prevent floating for a short while after death" },
	{ &c_cfg.safe_macros,		TRUE,	2,	0, 53, TRUE,
	    "safe_macros",		"Abort macro if item is missing or an action fails" },

	{ &c_cfg.auto_untag,		FALSE,	8,	0, 54, TRUE,
	    "auto_untag",		"Remove unique monster inscription on pick-up" },
	{ &c_cfg.clear_inscr,		FALSE,	8,	9, 55, TRUE,
	    "clear_inscr",		"Clear @-inscriptions on taking item ownership" },
	{ &c_cfg.auto_inscr_server,	FALSE,	8,	9, 56, TRUE,
	    "auto_inscr_server",	"Also use predefined server-side auto-inscriptions" },
	{ &c_cfg.stack_force_notes,	TRUE,	8,	0, 57, TRUE,
	    "stack_force_notes",	"Merge inscriptions when stacking" },
	{ &c_cfg.stack_force_costs,	TRUE,	8,	0, 58, TRUE,
	    "stack_force_costs",	"Merge discounts when stacking" },
	{ &c_cfg.stack_allow_items,	TRUE,	8,	0, 59, TRUE,
	    "stack_allow_items",	"Allow weapons and armor to stack" },
	{ &c_cfg.stack_allow_devices,	TRUE,	8,	0, 60, TRUE,
	    "stack_allow_devices",	"Allow wands/staffs/rods to stack" },
	{ &c_cfg.whole_ammo_stack,	FALSE,	8,	0, 61, TRUE,
	    "whole_ammo_stack",		"For ammo/misc items always operate on whole stack" },
	{ &c_cfg.always_repeat,		TRUE,	2,	0, 62, TRUE,
	    "always_repeat",		"Repeat obvious commands (eg search/tunnel)" },
	{ &c_cfg.always_pickup,		FALSE,	8,	0, 63, TRUE,
	    "always_pickup",		"Pick things up by default" },
	{ &c_cfg.use_old_target,	TRUE,	2,	0, 64, TRUE,
	    "use_old_target",		"Use old target by default" },
	{ &c_cfg.autooff_retaliator,	FALSE,	2,	0, 65, TRUE,
	    "autooff_retaliator",	"Stop the retaliator when protected by GoI etc" },
	{ &c_cfg.fail_no_melee,		FALSE,	2,	0, 66, TRUE,
	    "fail_no_melee",		"Don't melee if other auto-retaliation ways fail" },
	{ &c_cfg.basic_players_col,	FALSE,	1,	0, 67, TRUE,
	    "basic_players_col",	"Use very basic static colours for players" },
	{ &c_cfg.auto_target,		FALSE,	2,	0, 68, TRUE,
	    "auto_target",		"Automatically set target to the nearest enemy" },
	{ &c_cfg.thin_down_flush,	TRUE,	4,	0, 69, TRUE,
	    "thin_down_flush",		"Thin down screen flush signals to avoid freezing" },
	{ &c_cfg.disable_flush,		FALSE,	4,	0, 70, TRUE,
	    "disable_flush",		"Disable delays from flush signals" },

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
	{ &c_cfg.view_perma_grids,	TRUE,	3,	0, 81, TRUE,
	    "view_perma_grids",		"Map remembers all perma-lit grids" },
	{ &c_cfg.view_torch_grids,	FALSE,	3,	0, 82, TRUE,
	    "view_torch_grids",		"Map remembers all torch-lit grids" },

	//inactive
	{ &c_cfg.view_reduce_lite,	FALSE,	3,	0, 83, FALSE, /* Doesn't make sense */
	    "view_reduce_light",	"Reduce light radius when running" },
	{ &c_cfg.view_reduce_view,	FALSE,	3,	0, 84, FALSE, /* Doesn't make sense */
	    "view_reduce_view",		"Reduce view radius in town" },

	{ &c_cfg.easy_open,		TRUE,	3,	0, 85, TRUE,
	    "easy_open",		"Automatically open doors" },
	{ &c_cfg.easy_disarm,		FALSE,	3,	0, 86, TRUE,
	    "easy_disarm",		"Automatically disarm traps (except under items)" },
	{ &c_cfg.easy_tunnel,		FALSE,	3,	0, 87, TRUE,
	    "easy_tunnel",		"Automatically tunnel walls" },

	{ &c_cfg.audio_paging,		TRUE,	9,	0, 88, TRUE,
	    "audio_paging",		"Use audio system for page/alert, if available" },
	{ &c_cfg.paging_master_volume,	FALSE,	9,	0, 89, TRUE,
	    "paging_master_vol",	"Play page/alert sounds at master volume" },
	{ &c_cfg.paging_max_volume,	FALSE,	9,	0, 90, TRUE,
	    "paging_max_vol",		"Play page/alert sounds at maximum volume" },
	{ &c_cfg.no_ovl_close_sfx,	TRUE,	5,	0, 91, TRUE,
	    "no_ovl_close_sfx",		"Prevent re-playing sfx received after <100ms gap" },
	{ &c_cfg.ovl_sfx_attack,	TRUE,	5,	0, 92, TRUE,
	    "ovl_sfx_attack",		"Allow overlapping combat sounds of same type" },
	{ &c_cfg.no_combat_sfx,		FALSE,	5,	0, 93, TRUE,
	    "no_combat_sfx",		"Don't play melee/launcher attack/miss sound fx" },
	{ &c_cfg.no_magicattack_sfx,	FALSE,	5,	0, 94, TRUE,
	    "no_magicattack_sfx",	"Don't play basic spell/device attack sound fx" },
	{ &c_cfg.no_defense_sfx,	FALSE,	5,	0, 95, TRUE,
	    "no_defense_sfx",		"Don't play attack-avoiding/neutralizing sound fx" },
	{ &c_cfg.half_sfx_attack,	FALSE,	5,	0, 96, TRUE,
	    "half_sfx_attack",		"Skip every second attack sound" },
	{ &c_cfg.cut_sfx_attack,	TRUE,	5,	0, 97, TRUE,
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
	{ &c_cfg.no_monsterattack_sfx,	FALSE,	5,	0, 103, TRUE,
	    "no_monsterattack_sfx",	"Don't play basic monster attack sound fx" },
	{ &c_cfg.positional_audio,	TRUE,	5,	0, 104, TRUE,
	    "positional_audio",		"Play '3d' positional sound fx, via normal stereo" },
	{ &c_cfg.no_house_sfx,		FALSE,	5,	0, 105, TRUE,
	    "no_house_sfx",		"Don't play ambient/weather sound in buildings" },
	{ &c_cfg.quiet_house_sfx,	TRUE,	5,	0, 106, TRUE,
	    "quiet_house_sfx",		"Play quieter ambient/weather sound in buildings" },
	{ &c_cfg.mute_when_idle,	FALSE,	5,	0, 107, TRUE,
	    "mute_when_idle",		"Mute music+ambient sfx while AFK/idle in town" },
	{ &c_cfg.alert_starvation,	TRUE,	7,	0, 108, TRUE, //moved exp_bar to page 2 to make room for this
	    "alert_starvation",		"Beep when taking damage from starvation" },

	//inactive
    /* unmutable options, pfft -- these are never shown in any options menu (-> FALSE) */
	{ &c_cfg.use_color,		TRUE,	1,	0, 109, FALSE,//works, but pretty useless - disabled to make room (we always use colours nowadays)		//HOLE if we really want to completely remove it
	    "use_color",		"(deprecated) Use color if possible" },
	{ &c_cfg.other_query_flag,	FALSE,	2,	0, 110, FALSE, /* deprecated/not enabled: Verifies on mimic form change and warns about overflow/loss on various magic device/item handling */		//HOLE if we really want to completely remove it
	    "other_query_flag",		"Prompt for various information (mimic polymorph)" },

	//inactive
    /* deprecated/broken/todo options */
#if 0
	{ &c_cfg.quick_messages,	FALSE,	6,	0, 1, TRUE,
	    "quick_messages",		"Activate quick messages (skill etc)" },
	{ &c_cfg.carry_query_flag,	FALSE,	3,	0, 3, FALSE,
	    "carry_query_flag",		"(broken) Prompt before picking things up" },
	{ &c_cfg.show_labels,		TRUE,	6,	0, 10, FALSE, //not implemented at all
	    "show_labels",		"(broken) Show labels in object listings" },
	{ &c_cfg.show_choices,		FALSE,	6,	0, 12, FALSE, //not implemented at all
	    "show_choices",		"(broken) Show choices in certain sub-windows" },
	{ &c_cfg.show_details,		TRUE,	6,	0, 13, FALSE, //not implemented at all
	    "show_details",		"(broken) Show details in certain sub-windows" },
	{ &c_cfg.expand_look,		FALSE,	6,	0, 4, FALSE, //not implemented at all
	    "expand_look",		"(broken) Expand the power of the look command" },
	{ &c_cfg.expand_list,		FALSE,	6,	0, 5, FALSE, //not implemented at all
	    "expand_list",		"(broken) Expand the power of the list commands" },
	{ &c_cfg.avoid_other,		FALSE,	6,	0, 19, FALSE,
	    "avoid_other",		"(broken) Avoid processing special colors" },
	{ &c_cfg.flush_failure,		TRUE,	6,	0, 20, FALSE, /* (resurrect me?) */
	    "flush_failure",		"(broken) Flush input on various failures" },
	{ &c_cfg.flush_disturb,		FALSE,	6,	0, 21, FALSE,
	    "flush_disturb",		"(broken) Flush input whenever disturbed" },
	{ &c_cfg.fresh_after,           FALSE,  6,      0, 24, FALSE,
	    "fresh_after",		"(obsolete) Flush output after every command" },
    //todo: check shopkeeper speakage
	{ &c_cfg.speak_unique,		TRUE,	6,	0, xx, TRUE,
	    "speak_unique",		"Allow shopkeepers and uniques to speak" },
#endif

	{ &c_cfg.shuffle_music,		FALSE,	5,	0, 111, TRUE,
	    "shuffle_music",		"Don't loop song files but shuffle through them" },
	{ &c_cfg.permawalls_shade,	FALSE,	4,	0, 112, TRUE,
	    "permawalls_shade",		"Display permanent vault walls in a special colour" },
	{ &c_cfg.topline_no_msg,	FALSE,	0,	0, 113, TRUE, //page 3 (UI 3)
	    "topline_no_msg",		"Don't display messages in main window top line" },
	{ &c_cfg.targetinfo_msg,	FALSE,	0,	0, 114, TRUE, //page 3 (UI 3)
	    "targetinfo_msg",		"Display look/target info in message window too" },
	{ &c_cfg.live_timeouts,		TRUE,	7,	0, 115, TRUE, //page 3 (UI 3)
	    "live_timeouts",		"Always update item timeout numbers on every tick" },
	{ &c_cfg.flash_insane,		FALSE,	7,	0, 116, TRUE, //page 3 (UI 3)
	    "flash_insane",		"Flash own character icon when going badly insane" },

	{ &c_cfg.last_words,		TRUE,	0,	0, 117, TRUE,
	    "last_words",		"Get last words when the character dies" },
	{ &c_cfg.disturb_see,		FALSE,	3,	0, 118, TRUE,
	    "disturb_see",		"Disturb whenever seeing any monster" },

	{ &c_cfg.diz_unique,		TRUE,	0,	0, 119, TRUE,
	    "diz_unique",		"Displays lore when killing a unique monster" },
	{ &c_cfg.diz_death,		TRUE,	0,	0, 120, TRUE,
	    "diz_death",		"Displays lore on monster that killed you" },
	{ &c_cfg.diz_death_any,		TRUE,	0,	0, 121, TRUE,
	    "diz_death_any",		"Displays lore on monster that kills anyone" },
	{ &c_cfg.diz_first,		TRUE,	0,	0, 122, TRUE,
	    "diz_first",		"Displays lore on first-time monster kill" },
	{ &c_cfg.screenshot_format,	TRUE,	6,	0, 123, TRUE,
	    "screenshot_format",	"Screenshots are timestamped instead of numbered" },
	{ &c_cfg.palette_animation,	TRUE,	4,	0, 124, TRUE,
	    "palette_animation",	"Shade world surface colours depending on daytime" },
	{ &c_cfg.play_all,		TRUE,	5,	0, 125, TRUE,
	    "play_all",			"Loop over all available songs instead of just one" },
	{ &c_cfg.id_selection,		TRUE,	6,	0, 126, TRUE,
	    "id_selection",		"Show/accept only eligible items for ID/*ID*" },
	{ &c_cfg.hp_bar,		FALSE,	1,	0, 127, TRUE,
	    "hp_bar",			"Display hit points as bar instead of numbers" },
	{ &c_cfg.mp_bar,		FALSE,	1,	0, 128, TRUE,
	    "mp_bar",			"Display mana pool as bar instead of numbers" },
	{ &c_cfg.st_bar,		FALSE,	1,	0, 129, TRUE,
	    "st_bar",			"Display stamina as bar instead of numbers" },

	{ &c_cfg.find_ignore_montraps,	TRUE,	3,	0, 130, TRUE,
	    "find_ignore_montraps",	"Run through monster traps" },

	{ &c_cfg.quiet_os,		FALSE,	9,	0, 131, TRUE,
	    "quiet_os",			"Don't play beep/alert/page beeps through OS" },
	{ &c_cfg.disable_lightning,	FALSE,	4,	0, 132, TRUE,
	    "disable_lightning",	"Disable visual screen flash effect for lightning" },
	{ &c_cfg.macros_in_stores,	FALSE,	2,	0, 133, TRUE,
	    "macros_in_stores",		"Eable macros while inside a store" },
	{ &c_cfg.item_error_beep,	TRUE,	7,	0, 134, TRUE,
	    "item_error_beep",		"Beep when an item selection fails" },
	{ &c_cfg.keep_bottle,		FALSE,	8,	0, 135, TRUE,
	    "keep_bottle",		"Keep the empty bottle when you quaff a potion" },

	{ &c_cfg.easy_disarm_montraps,	FALSE,	3,	0, 136, TRUE,
	    "easy_disarm_montraps",	"Automatically disarm monster traps ('/edmt')" },
	{ &c_cfg.no_house_magic,	FALSE,	2,	0, 137, TRUE,
	    "no_house_magic",		"Prevent using magic inside houses" },
	{ &c_cfg.no_lite_fainting,	FALSE,	4,	0, 138, TRUE,
	    "no_light_fainting",	"Disable shading effect for fainting light source" },

	{ &c_cfg.auto_pickup,		FALSE,	8,	0, 139, TRUE,
	    "auto_pickup",		"Automatically pickup items (see '/apickup')" },
	{ &c_cfg.auto_destroy,		FALSE,	8,	0, 140, TRUE,
	    "auto_destroy",		"Automatically destroy items (see '/adestroy')" },
	{ &c_cfg.destroy_all_unmatched,	FALSE,	8,	0, 141, TRUE,	// this option does't make sense, can just use '#' in & menu instead, much cheaper
	    "destroy_all_unmatched",	"Destroys ALL unmatched items. (Like A'#' in &.)" },

	{ &c_cfg.mp_huge_bar,		TRUE,	1,	0, 142, TRUE,
	    "mp_huge_bar",		"Also show mana pool as huge bar (big_map only)" },
	{ &c_cfg.sn_huge_bar,		TRUE,	1,	0, 143, TRUE,
	    "sn_huge_bar",		"Also show sanity as huge bar (big_map only)" },
	{ &c_cfg.hp_huge_bar,		TRUE,	1,	0, 144, TRUE,
	    "hp_huge_bar",		"Also show HP pool as huge bar (big_map only)" },

	{ &c_cfg.clone_to_stdout,	FALSE,	1,	0, 145, TRUE,
	    "clone_to_stdout",		"Clone client chat and messages to stdout" },
	{ &c_cfg.clone_to_file,		FALSE,	1,	0, 146, TRUE,
	    "clone_to_file",		"Clone client chat and messages to 'stdout.txt'" },
	{ &c_cfg.first_song,		FALSE,	5,	0, 147, TRUE,
	    "first_song",		"Start with first eligible song of a music event" },
	{ &c_cfg.equip_text_colour,	FALSE,	6,	0, 148, TRUE,
	    "equip_text_colour",	"Display equipment indices/weight in yellow" },
	{ &c_cfg.equip_set_colour,	TRUE,	6,	0, 149, TRUE,
	    "equip_set_colour",		"Colourize indices of items giving set bonus" },
	{ &c_cfg.colourize_bignum,	FALSE,	6,	0, 150, TRUE,
	    "colourize_bignum",		"Colourize prices, AU and XP in 3-digit columns" },

	{ &c_cfg.flash_player2,		FALSE,	7,	0, 151, TRUE,
	    "flash_player2",		"Flash own character icon after close relocation" },
	{ &c_cfg.load_form_macros,	TRUE,	2,	0, 152, TRUE,
	    "load_form_macros",		"Automatically load form-specific macros on change" },
	{ &c_cfg.auto_inscr_off,	FALSE,	8,	0, 153, TRUE,
	    "auto_inscr_off",		"Disable all client-side auto-inscriptions" },

	{ &c_cfg.ascii_feats,		FALSE,	4,	0, 154, TRUE,
	    "ascii_feats",		"Disable font-specific mapping for all floor feats" },
	{ &c_cfg.ascii_items,		FALSE,	4,	0, 155, TRUE,
	    "ascii_items",		"Disable font-specific mapping for all items" },
	{ &c_cfg.ascii_monsters,	FALSE,	4,	0, 156, TRUE,
	    "ascii_monsters",		"Disable font-specific mapping for all monsters" },
	{ &c_cfg.ascii_uniques,		FALSE,	4,	0, 157, TRUE,
	    "ascii_uniques",		"Disable font-specific mapping for unique monsters" },

	{ &c_cfg.no_flicker,		FALSE,	4,	0, 158, TRUE,
	    "no_flicker",		"Disable all fast colour animations" },
	{ &c_cfg.stun_huge_bar,		TRUE,	1,	0, 159, TRUE,
	    "stun_huge_bar",		"Show stun status as huge bar (big_map only)" },

	//inactive
	{ &c_cfg.gfx_autooff_fmsw,	FALSE,	1,	0, 160, FALSE,	/* superceded by S_GFX_AUTOOFF_FMSW which is superior as it allows solid status bar visuals etc still! */
	    "gfx_autooff_fmsw",		"Auto-disable font_map_solid_walls for graphics" },

	{ &c_cfg.solid_bars,		TRUE,	1,	0, 161, TRUE,
	    "solid_bars",		"Display status bars in solid style, not as '#'" },
	{ &c_cfg.autopickup_chemicals,	TRUE,	8,	9, 162, TRUE,
	    "autopickup_chemicals",	"Automatically pick up freshly dropped chemicals" },
	{ &c_cfg.add_kind_diz,		TRUE,	0,	0, 163, TRUE,
	    "add_kind_diz",		"Inv/eq window only: Item-to-chat-paste extra info" },
	{ &c_cfg.hide_lore_paste,	FALSE,	0,	0, 164, TRUE,
	    "hide_lore_paste",		"Hide artifact/monster lore pastes in public chat" },

	{ &c_cfg.instant_retaliator,	TRUE,	2,	0, 165, TRUE,
	    "instant_retaliator",	"Autoretaliate instantly but forego reserve energy" }, //note: includes FTK
	{ &c_cfg.sunburn_msg,		TRUE,	0,	0, 166, TRUE,
	    "sunburn_msg",		"Show message if you start/stop burning in the sun" },
	{ &c_cfg.wide_scroll_margin,	TRUE,	1,	0, 167, TRUE,
	    "wide_scroll_margin",	"Scroll the screen further away from the edges" },
	{ &c_cfg.gfx_palanim_repaint,	TRUE,	1,	0, 168, FALSE, //just for debugging repaint glitches
	    "gfx_palanim_repaint",	"Flicker-free repaint, glitches on some systems" },
	{ &c_cfg.gfx_hack_repaint,	TRUE,	1,	0, 169, FALSE, //just for debugging repaint glitches
	    "gfx_hack_repaint",		"May reduce flicker if gfx_palanim_repaint is off" },
	{ &c_cfg.topline_first,		TRUE,	0,	0, 170, TRUE,
	    "topline_first",		"Top line shows first line of multi-line messages" },
	{ &c_cfg.ascii_weather,		FALSE,	1,	0, 171, TRUE, //just for debugging weather glitches
	    "ascii_weather",		"Always use plain ASCII symbols for weather" },
	{ &c_cfg.no2mask_weather,	FALSE,	1,	0, 172, FALSE, //just for debugging weather glitches
	    "no2mask_weather",		"Never use dual-mask grapics for weather" },
	{ &c_cfg.auto_inscr_server_ch,	TRUE,	8,	9, 173, TRUE,
	    "auto_inscr_server_ch",	"Server-defined auto-inscriptions for chemicals" },

	{ &c_cfg.screenshot_keys,	FALSE,	6,	0, 174, TRUE,
	    "screenshot_keys",		"Swap CTRL+T and SHIFT+CTRL+T key (XHTML vs PNG)" },
	{ &c_cfg.wild_resume_from_any,	TRUE,	5,	0, 175, TRUE,
	    "wild_resume_from_any",	"Resume wilderness music even after non-wild music" },
	{ &c_cfg.tavern_town_resume,	TRUE,	5,	0, 176, TRUE,
	    "tavern_town_resume",	"If wild_resume_from_any, also resume tavern/town" },
	{ &c_cfg.st_huge_bar,		FALSE,	1,	0, 177, TRUE,
	    "st_huge_bar",		"Also show stamina as huge bar (big_map only)" },
	{ &c_cfg.huge_bars_gfx,		TRUE,	1,	0, 178, TRUE,
	    "huge_bars_gfx",		"Display huge bars graphically if graphics are on" },

	{ &c_cfg.autoinsc_debug,	FALSE,	8,	0, 179, TRUE,
	    "autoinsc_debug",		"Displays extra info on auto-inscriptions matching" },
	{ &c_cfg.autoloot_dunonly,	FALSE,	8,	0, 180, TRUE,
	    "autoloot_dunonly",		"Suppress auto-pickup/destroy while not in dungeon" },
	{ &c_cfg.autoloot_dununown,	FALSE,	8,	0, 181, TRUE,
	    "autoloot_dununown",	"Suppress auto-pck/des if owned and not in dungeon" },
	{ &c_cfg.flash_starvation,	TRUE,	7,	0, 182, TRUE, //moved exp_bar to page 2 to make room for this
	    "flash_starvation",		"Flash 'Starved' food status by colour animation." },
	{ &c_cfg.autoswitch_inven,	TRUE,	6,	0, 183, TRUE,
	    "autoswitch_inven",		"Empty inven prompts auto-switch to equip or bag." },
	{ &c_cfg.show_newest,		FALSE,	8,	0, 184, TRUE, //'items' page 8; could also be on page 3 (formatting) or 4 (notifications) perhaps
	    "show_newest",		"Marks the inventory slot of the 'newest' item." },
	{ &c_cfg.log_music,		FALSE,	9,	0, 185, TRUE,
	    "log_music",		"Log game-initiated music to 'Messages' window" },
};

cptr melee_techniques[16] = {
  "Sprint",
  "Taunt",
  "Throw Dirt",
  "Bash",

#if 0
  "Stab", //swords
  "Slice", //axes
  "Quake", //blunts
  "Sweep", //polearms
#endif

  "Distract",
  "Apply Poison", //"Knockback",
  "Track Animals",
  "Perceive Noise",

  "Flash Bomb",
  "Steam Blast",
  "Spin",
  "Assassinate", /*"Charge",*/ //ENABLE_ASSASSINATE (TEST_SERVER only)

  "Berserk",
  "Jump", //UNUSED
  "Shadow Jump", //UNUSED
  "Shadow Run",
};

cptr ranged_techniques[16] = {
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
