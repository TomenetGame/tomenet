--[[ 'audio.lua'
 * Purpose: Bind numerical indices for sound fx and background music, used for
 * efficient network transmissions, to actual strings, for human-readability
 * and easy user-made modification of sound.cfg and music.cfg; this file is for
 * the new SDL-based sound system (snd-sdl.c, USE_SOUND_2010) - C. Blue
]]

-- Sound FX
audio_sfx = {
    --[weather] (0)
    "Rain_soft",
    "Rain_storm",
    "Snow_soft", --nothing really
    "Snow_storm",

    --[player] (4)
    --combat (4)
    "hit",
    "hit_weapon",
    "hit_sword",
    "hit_blunt",
    "hit_whip",
    "hit_axe",
    "hit_polearm",
    "miss", --11
    "miss_projectile",
    "miss_weapon",
    "parry", --14
    "parry_weapon",
    "block_shield", --16
    "block_shield_projectile",
    "death", --your favourite scream (I like the DOOM space marine) (18)
    "death_male", --your favourite scream (I like the DOOM space marine)
    "death_female", --your favourite scream (I like the DOOM space marine)
    "levelup",
    --commands
    "eat", --22
    "quaff_potion",
    "read_scroll",
    "aim_wand",
    "zap_rod",
    "use_staff",
    "cast_bolt",--spells (28)
    "cast_ball",--spells
    "cast_cloud",--spells
    "cast_wave",--spells
    "phase_door",--spells
    "teleport",--spells
    "activate", --34
    "browse",
    "browse_book",
    "bash_door_hold",
    "bash_door_break",
    "open_door_stuck",
    "open_door",
    "open_chest",
    "open_pick", --42
    "close_door",
    "disarm",
    "drop_gold",
    "pickup_gold",
    "staircase", --steps
    "tunnel_rock",
    "tunnel_tree",
    "wearwield_armor_light", --50
    "wearwield_armor_heavy",
    "wearwield_lightsource",
    "wearwield_jewelry",
    "wearwield_tool",
    "wearwield_tool_digger",
    "wearwield_weapon",
    "wearwield_sword",
    "wearwield_blunt",
    "wearwield_whip",
    "wearwield_axe",
    "wearwield_polearm", --61

    --[monsters]
    "death_monster", --smack/smash sound (floating eye, insects, molds, jellies, all other stuff..) (62)
    "death_monster_animal", --hiss
    "death_monster_animal_small", --hiss
    "death_monster_animal_large", --roar
    "death_monster_animal_aquatic", --bubbling
    "death_monster_CZ", --howl
    "death_monster_humanoid", --scream/ugh/silence/whatever
    "death_monster_oOTP", --roar/scream
    "death_monster_A", --probably nothing, really
    "death_monster_dragon_low", --hiss (71)
    "death_monster_dragon_high", --roar
    "death_monster_demon_low", --squeak
    "death_monster_demon_high", --scream
    "death_monster_undead_low", --pulverize
    "death_monster_undead_high", --ghostly dissolve
    "death_monster_nonliving", --breakdown (construct)
    "death_monster_Ev", --swushsh (elementals and vortices dissolve)
    "monster_hits", --79
    "monster_hits_claws",
    "monster_hits_weapon",
    "monster_misses",
    "monster_casts_bolt",
    "monster_casts_ball",
    "monster_breaths",
    "monster_summons",
    "monster_curses",
    "monster_blinks",
    "monster_teleports",
    "monster_heals", --90

    --[grid]
    "trap_setoff", --click? ^^ (91)
    "earthquake",
    "destruction",
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
    "TheIllusionaryCastle",
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
