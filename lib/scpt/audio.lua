--[[ 'audio.lua'
 * Purpose: Bind numerical indices for sound fx and background music, used for
 * efficient network transmissions, to actual strings, for human-readability
 * and easy user-made modification of sound.cfg and music.cfg; this file is for
 * the new SDL-based sound system (snd-sdl.c, USE_SOUND_2010) - C. Blue
]]

-- Sound FX
audio_sfx = {
    "Rain_soft",
    "Rain_storm",
    "Snow_soft", --nothing really
    "Snow_storm",

    "hit",
    "hit_weapon",
    "miss",
    "miss_weapon",
    "parry",
    "parry_weapon",
    "block_shield",

    "death_player", --your favourite scream (I like the DOOM space marine)
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

    "staircase", --steps
    "door_open",
    "door_close",
    "door_stuck",
    "door_smash",
    "trap_setoff", --click? ^^
}
function get_sound_name(idx)
    if audio_sfx[idx] == nil then return "" end
    return (audio_sfx[idx])
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

    "Wilderness_generic_day",
    "Wilderness_generic_night",

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

    "SpecialUnique",
    "DungeonBoss",
    "Boss_Nazgul",
    "Boss_Sauron",
    "Boss_Morgoth",
    "Boss_ZuAon",

    "Terrifying",

    "Highlander_deathmatch",
    "AreanaMonsterChallenge",
}
function get_music_name(idx)
    if audio_bgm[idx] == nil then return "" end
    return (audio_bgm[idx])
end
