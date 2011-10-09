--[[ 'audio.lua'
 * Purpose: Bind numerical indices for sound fx and background music, used for
 * efficient network transmissions, to actual strings, for human-readability
 * and easy user-made modification of sound.cfg and music.cfg; this file is for
 * the new SDL-based sound system (snd-sdl.c, USE_SOUND_2010) - C. Blue
]]

-- Sound FX
audio_sfx = {
    --[misc]
    "page",
    "warning",
    "error",

    --[weather]
    "rain_soft",
    "rain_storm",
    "snow_soft", --nothing really
    "snow_storm",

    --[player]
    --combat
    "hit",
    "hit_weapon",
    "hit_sword",
    "hit_blunt",
    "hit_whip",
    "hit_axe",
    "hit_polearm",
    "miss",
    "miss_weapon",
    "miss_projectile",
    "parry",
    "parry_weapon",
    "block_shield",
    "block_shield_projectile",

    "fire_shot",
    "fire_arrow",
    "fire_bolt",
    "fire_missile",
    "fire_boomerang",

    "death", --everyone's favourite DOOM scream....
    "death_male",
    "death_female",

    "insanity",

    "levelup",

    --commands
    "activate",
    "aim_wand",
    "bash",
    "bash_door_hold",
    "bash_door_break",
    "browse",
    "browse_book",
    "breath",
    "cast",
    "cast_ball",
    "cast_bolt",
    "cast_beam",
    "cast_cloud",
    "cast_wall",
    "cast_wave",
    "grow_trees",
    "stone_wall",
    "phase_door",
    "teleport",
    "cloak",
    "close_door",
    "disarm",
    "eat",
    "fuel",
    "open_chest",
    "open_door",
    "open_door_stuck",
    "open_pick",
    "quaff_potion",
    "read_scroll",
    "staircase",
    "use_staff",
    "throw",
    "tunnel_rock",
    "tunnel_rubble",
    "tunnel_tree",
    "zap_rod",

    "drop_gold",
    "pickup_gold",

    "item_weapon",
    "item_sword",
    "item_blunt",
    "item_whip",
    "item_axe",
    "item_polearm",
    "item_magestaff",
    "item_boomerang",
    "item_bow",
    "item_shot",
    "item_arrow",
    "item_bolt",
    "item_ring",
    "item_amulet",
    "item_lightsource",
    "item_armor_light",
    "item_armor_heavy",
    "item_tool",
    "item_tool_digger",

    "item_scroll",
    "item_bottle",
    "item_potion",
    "item_rune",
    "item_skeleton",
    "item_firestone",
    "item_spike",
    "item_chest",
    "item_junk",
    "item_trapkit",
    "item_staff",
    "item_wand",
    "item_rod",
    "item_key",
    "item_golem_wood",
    "item_golem_metal",
    "item_golem_misc",
    "item_seal",

    "cough",
    "cough_male",
    "cough_female",
    "shout",
    "shout_male",
    "shout_female",
    "flash_bomb",

    --[monsters]
    "death_monster", --smack/smash sound (floating eye, insects, molds, jellies, all other stuff..)
    "death_monster_animal", --hiss
    "death_monster_animal_small", --hiss
    "death_monster_animal_large", --roar
    "death_monster_animal_aquatic", --bubbling
    "death_monster_CZ", --howl
    "death_monster_humanoid", --scream/ugh/silence/whatever
    "death_monster_oOTP", --roar/scream
    "death_monster_A", --probably nothing, really
    "death_monster_dragon_low", --hiss
    "death_monster_dragon_high", --roar
    "death_monster_demon_low", --squeak
    "death_monster_demon_high", --scream
    "death_monster_undead_low", --pulverize
    "death_monster_undead_high", --ghostly dissolve
    "death_monster_nonliving", --breakdown (construct)
    "death_monster_Ev", --swushsh (elementals and vortices dissolve)

    "monster_hit",
    "monster_hit_weapon",
    "monster_hit_sword",
    "monster_hit_blunt",
    "monster_hit_whip",
    "monster_hit_axe",
    "monster_hit_polearm",

    "monster_beg",
    "monster_bite",
    "monster_butt",
    "monster_charge",
    "monster_claw",
    "monster_crawl",
    "monster_crush",
    "monster_drool",
    "monster_engulf",
    "monster_explode",
    "monster_gaze",
    "monster_insult",
    "monster_kick",
    "monster_moan",
    "monster_punch",
    "monster_show",
    "monster_spit",
    "monster_spore",
    "monster_sting",
    "monster_touch",
    "monster_wail",
    "monster_whisper",

    "monster_miss",
    "monster_miss_weapon",
    "monster_miss_projectile",
    "monster_parry",
    "monster_parry_weapon",
    "monster_block_shield",
    "monster_block_shield_projectile",

    "monster_fire_shot",
    "monster_fire_arrow",
    "monster_fire_bolt",
    "monster_fire_missile",

    "monster_breath",
    "monster_cast_bolt",
    "monster_cast_ball",
    "monster_cast_cloud",
    "monster_curse",
    "monster_heal",
    "monster_shriek",
    "monster_summon",
    "monster_blink",
    "monster_teleport",
    "monster_puff",

    --[grid]
    "destruction",
    "detonation",
    "earthquake",
    "rocket",
    "trap_setoff",
    "hollow_noise",
    "stirring",
    "shatter_potion",
}
function get_sound_name(idx)
    if audio_sfx[idx + 1] == nil then return "" end
--    if getn(audio_sfx) < idx then return "" end
    return (audio_sfx[idx + 1])
end
function get_sound_index(name)
    for i = 1, getn(audio_sfx) do
	if audio_sfx[i] == name then
	    return(i - 1)
	end
    end
    return (-1)
end

-- Background Music
audio_bgm = {
    "generic",

    "town_generic",
    "town_dungeon",

    "Bree",
    "Gondolin",
    "MinasAnor",
    "Lothlorien",
    "Khazaddum",
    "Valinor",

    "wilderness_generic_day",
    "wilderness_generic_night",

    "dungeon_generic",
    "dungeon_generic_nodeath",
    "dungeon_generic_ironman",
    "dungeon_generic_forcedownhellish",

    "TheTrainingTower",
    "BarrowDowns",
    "Mordor",
    "ThePathsoftheDead",
    "Angband",

    "TheOrcCave",
    "MountDoom",
    "NetherRealm",
    "TheHelcaraxe",
    "TheLandofRhun",
    "TheSandwormLair",
    "TheHallsofMandos",
    "TheOldForest",
    "TheHeartoftheEarth",
    "TheMinesofMoria",
    "CirithUngol",
    "TheSmallWaterCave",
    "Mirkwood",
    "DeathFate",
    "DolGuldur",
    "SubmergedRuins",
    "Erebor",
    "TheIllusoryCastle",
    "TheSacredLandofMountains",
    "TheMaze",

    "boss_specialunique",
    "boss_dungeon",
    "boss_Nazgul",
    "boss_Sauron",
    "boss_Morgoth",
    "boss_ZuAon",

    "feeling_Terrifying",

    "event_Highlander_deathmatch",
    "event_AreanaMonsterChallenge",
}
function get_music_name(idx)
    if audio_bgm[idx + 1] == nil then return "" end
--    if getn(audio_bgm) < idx then return "" end
    return (audio_bgm[idx + 1])
end
