--[[ 'audio.lua'
 * Purpose: Bind numerical indices for sound fx and background music, used for
 * efficient network transmissions, to actual strings, for human-readability
 * and easy user-made modification of sound.cfg and music.cfg; this file is for
 * the new SDL-based sound system (snd-sdl.c, USE_SOUND_2010) - C. Blue
]]

-- Sound FX
audio_sfx = {
    --[player/alert]
    "bell",
    "page",
    "greeting",
    "warning",
    "gong",
    "success",
    "failure",

    --[weather]
    "rain_soft",
    "rain_storm",
    "snow_soft", --nothing really
    "snow_storm",

    "thunder",

    --[player]
    --commands
    "activate",
    "aim_wand",
    "bash",
    "bash_door_hold",
    "bash_door_break",
    "browse",
    "browse_book",
    "cast",
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
    "staircase_fly",
    "staircase_pad",
    "staircase_slither",
    "staircase_scuttle",
    "staircase_rider",
    "use_staff",
    "throw",
    "tunnel_rock",
    "tunnel_rubble",
    "tunnel_tree",
    "zap_rod",

    "drop_gold",
    "pickup_gold",

    "cough",
    "cough_male",
    "cough_female",
    "shout",
    "shout_male",
    "shout_female",
    "scream",
    "scream_male",
    "scream_female",

    "taunt_male",
    "taunt_female",
    "flash_bomb",
    "spin",
    "berserk_male",
    "berserk_female",
    "shadow_run",

    "flare_missile",
    "barrage_shot",
    "barrage_arrow",
    "barrage_bolt",

    "slap",
    "snowball",
    "applaud",

    "knock",
    "knock_castle",

    --game
    "playing_cards",
    "playing_cards_shuffle",
    "playing_cards_dealer",
    "dice_roll",
    "coin_flip",
    "go_stone",
    "ball_pass",
    "game_piece",

    --misc
    "grow_trees",
    "insanity",
    "levelup",
    "am_field",

    "death", --everyone's favourite DOOM scream....
    "death_male",
    "death_female",

    --[combat]
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
    "disarm_weapon",

    "fire_shot",
    "fire_arrow",
    "fire_bolt",
    "fire_boomerang",
    "fire_missile",
    "throw_boulder",

    --[magic]
    "breath",
    "cast_ball",
    "cast_bolt",
    "cast_beam",
    "cast_cloud",
    "cast_wall",
    "cast_wave",
    "blink",
    "phase_door",
    "puff",
    "teleport",
    "curse",
    "heal",
    "summon",
    "rocket",

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

    --[misc]
    "shriek",
    "earthquake",
    "destruction",
    "detonation",
    "trap_setoff",
    "hollow_noise",
    "stirring",
    "stone_wall",
    "shatter_potion",
    "store_doorbell_enter",
    "store_doorbell_leave",
    "jailed",
    "fireworks_big",
    "fireworks_norm",
    "fireworks_small",
    "fireworks_launch",

    --[item]
    "item_food",
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
    "item_armour_light",
    "item_armour_heavy",
    "item_tool",
    "item_tool_digger",

    "item_book",
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

    --[ambient]
    "ambient_fireplace",
    "ambient_shore",
    "ambient_lake",
    "ambient_fire",

    "animal_bird",
    "animal_owl",
    "animal_seagull",
    "animal_toad",
    "animal_wolf",
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
    "generic",--0

    "town_generic",
    "town_dungeon",
    "Bree",
    "Gondolin",
    "MinasAnor",
    "Lothlorien",
    "Khazaddum",

    "Valinor",

    "wilderness_generic_day",
    "wilderness_generic_night",--10

    "dungeon_generic",
    "dungeon_generic_nodeath",
    "dungeon_generic_ironman",
    "dungeon_generic_forcedownhellish",

    "TheTrainingTower",
    "BarrowDowns",
    "Mordor",
    "ThePathsoftheDead",
    "Angband",

    "TheOrcCave",--20
    "MountDoom",
    "NetherRealm",
    "TheHelcaraxe",
    "TheLandofRhun",
    "TheSandwormLair",
    "TheHallsofMandos",
    "TheOldForest",
    "TheHeartoftheEarth",
    "TheMinesofMoria",
    "CirithUngol",--30
    "TheSmallWaterCave",
    "Mirkwood",
    "DeathFate",
    "DolGuldur",
    "SubmergedRuins",
    "Erebor",
    "TheIllusoryCastle",
    "TheSacredLandofMountains",
    "TheMaze",

    "boss_specialunique",--40
    "boss_dungeon",
    "boss_Nazgul",
    "boss_Sauron",
    "boss_Morgoth",
    "boss_ZuAon",

    "feeling_Terrifying",

    "event_Highlander_deathmatch",
    "event_ArenaMonsterChallenge",

    "town_generic_night",
    "Bree_night",--50
    "Gondolin_night",
    "MinasAnor_night",
    "Lothlorien_night",
    "Khazaddum_night",

    "event_Halloween",
    "TheCloudPlanes",

    "Menegroth",
    "Nargothrond",

    "title",
    "account",--60
    "tomb",

    "event_Highlander_dungeon",
    "event_Highlander_dungeon_final",
    "event_DungeonKeeper",
    "event_DungeonKeeper_final",
    "event_PvP_arena",
    "event_Xmas",

    "tavern_town_generic",
    "tavern_town_generic_night",
    "tavern_Bree",--70
    "tavern_Bree_night",
    "tavern_Gondolin",
    "tavern_Gondolin_night",
    "tavern_MinasAnor",
    "tavern_MinasAnor_night",
    "tavern_Lothlorien",
    "tavern_Lothlorien_night",
    "tavern_Khazaddum",
    "tavern_Khazaddum_night",
    "tavern_town_dungeon",--80
    "tavern_Menegroth",
    "tavern_Nargothrond",

    "season_halloween",
    "season_xmas",
    "season_newyearseve",

    "sickbay",
    "jail",
    "winner",
}
function get_music_name(idx)
    if audio_bgm[idx + 1] == nil then return "" end
--    if getn(audio_bgm) < idx then return "" end
    return (audio_bgm[idx + 1])
end
--just for do_cmd_options_mus_sdl():
function get_music_index(name)
    for i = 1, getn(audio_bgm) do
        if audio_bgm[i] == name then
            return(i - 1)
        end
    end
    return (-1)
end
