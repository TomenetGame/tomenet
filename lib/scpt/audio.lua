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

    "death", --everyone's favourite DOOM scream....
    "death_male",
    "death_female",

    "levelup",
    --commands
    "activate",
    "aim_wand",
    "bash",
    "bash_door_hold",
    "bash_door_break",
    "browse",
    "browse_book",
    "cast",
    "cast_ball",
    "cast_bolt",
    "cast_cloud",
    "cast_wave",
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
    "drop_weapon",
    "drop_sword",
    "drop_blunt",
    "drop_whip",
    "drop_axe",
    "drop_polearm",
    "drop_ring",
    "drop_amulet",
    "drop_lightsource",
    "drop_armor_light",
    "drop_armor_heavy",
    "drop_tool",
    "drop_tool_digger",

    "pickup_gold",
    "pickup_weapon",
    "pickup_sword",
    "pickup_blunt",
    "pickup_whip",
    "pickup_axe",
    "pickup_polearm",
    "pickup_ring",
    "pickup_amulet",
    "pickup_lightsource",
    "pickup_armor_light",
    "pickup_armor_heavy",
    "pickup_tool",
    "pickup_tool_digger",

    "takeoff_weapon",
    "takeoff_sword",
    "takeoff_blunt",
    "takeoff_whip",
    "takeoff_axe",
    "takeoff_polearm",
    "takeoff_ring",
    "takeoff_amulet",
    "takeoff_lightsource",
    "takeoff_armor_light",
    "takeoff_armor_heavy",
    "takeoff_tool",
    "takeoff_tool_digger",

    "wearwield_weapon",
    "wearwield_sword",
    "wearwield_blunt",
    "wearwield_whip",
    "wearwield_axe",
    "wearwield_polearm",
    "wearwield_ring",
    "wearwield_amulet",
    "wearwield_lightsource",
    "wearwield_armor_light",
    "wearwield_armor_heavy",
    "wearwield_tool",
    "wearwield_tool_digger",

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
    "monster_curse",
    "monster_heal",
    "monster_summon",
    "monster_blink",
    "monster_puff",
    "monster_teleport",

    --[grid]
    "destruction",
    "detonation",
    "earthquake",
    "rocket",
    "trap_setoff",
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
