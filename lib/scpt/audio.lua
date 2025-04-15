--[[ 'audio.lua'
 * Purpose: Bind numerical indices for sound fx and background music, used for
 * efficient network transmissions, to actual strings, for human-readability
 * and easy user-made modification of sound.cfg and music.cfg; this file is for
 * the new SDL-based sound system (snd-sdl.c, USE_SOUND_2010) - C. Blue
]]

-- Sound FX
audio_sfx = {
    --[player/alert]
    "bell", --0
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
    "snow_storm", --10

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
    "cloak", --20
    "close_door",
    "disarm",
    "eat",
    "fuel",
    "open_chest",
    "open_door",
    "open_door_stuck",
    "open_pick",
    "quaff_potion",
    "read_scroll", --30
    "staircase",
    "staircase_fly",
    "staircase_pad",
    "staircase_slither",
    "staircase_scuttle",
    "staircase_rider",
    "use_staff",
    "throw",
    "tunnel_rock",
    "tunnel_rubble", --40
    "tunnel_tree",
    "hit_floor",
    "zap_rod",

    "drop_gold",
    "pickup_gold",

    "cough",
    "cough_male",
    "cough_female",
    "shout",
    "shout_male", --50
    "shout_female",
    "scream",
    "scream_male",
    "scream_female",

    "taunt_male",
    "taunt_female",
    "flash_bomb",
    "spin",
    "berserk_male",
    "berserk_female", --60
    "shadow_run",

    "flare_missile",
    "barrage_shot",
    "barrage_arrow",
    "barrage_bolt",

    "slap",
    "snowball",
    "applaud",

    "knock",
    "knock_castle", --70
    "knock_window",

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
    "grow_trees", --80
    "insanity",
    "inventory",
    "levelup",
    "jailed",
    "receive_xo",

    "death", --everyone's favourite DOOM scream....
    "death_male",
    "death_female",

    --[combat]
    "hit",
    "hit_weapon", --90
    "hit_sword",
    "hit_blunt",
    "hit_whip",
    "hit_axe",
    "hit_polearm",

    "miss",
    "miss_weapon",
    "miss_projectile",
    "parry",
    "parry_weapon", --100
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
    "breath", --110
    "cast_ball",
    "cast_bolt",
    "cast_beam",
    "cast_cloud",
    "cast_wall",
    "cast_wave",
    "blink",
    "phase_door",
    "puff",
    "teleport", --120
    "recall",
    "curse",
    "heal",
    "summon",
    "rocket",

    --[monsters]
    "death_monster", --smack/smash sound (floating eye, insects, molds, jellies, all other stuff..)
    "death_monster_animal", --hiss
    "death_monster_animal_small", --hiss
    "death_monster_animal_large", --roar
    "death_monster_animal_aquatic", --bubbling --130
    "death_monster_CZ", --howl
    "death_monster_humanoid", --scream/ugh/silence/whatever
    "death_monster_oOTP", --roar/scream
    "death_monster_A", --probably nothing, really
    "death_monster_dragon_low", --hiss
    "death_monster_dragon_high", --roar
    "death_monster_demon_low", --squeak
    "death_monster_demon_high", --scream
    "death_monster_undead_low", --pulverize
    "death_monster_undead_high", --ghostly dissolve --140
    "death_monster_nonliving", --breakdown (construct)
    "death_monster_Ev", --swushsh (elementals and vortices dissolve)

    "monster_beg",
    "monster_bite",
    "monster_butt",
    "monster_charge",
    "monster_claw",
    "monster_crawl",
    "monster_crush",
    "monster_drool", --150
    "monster_engulf",
    "monster_explode",
    "monster_gaze",
    "monster_insult",
    "monster_kick",
    "monster_moan",
    "monster_punch",
    "monster_show",
    "monster_spit",
    "monster_spore", --160
    "monster_sting",
    "monster_touch",
    "monster_wail",
    "monster_whisper",

    "monster_stirring",
    "monster_roar",
    -- 'notice' sfx by priority, from lowest to highest:
    "monster_notice_animal_small", --hiss
    "monster_notice_animal_large", --growl
    "monster_notice_humanoid", --speech-like
    "monster_notice_grunt", --orcs, trolls, ogres, giants, hybrids maybe --170
    "monster_notice_undead",
    "monster_notice_demon",
    "monster_notice_dragon",

    --[misc]
    "shriek",
    "earthquake",
    "destruction",
    "detonation",
    "trap_setoff",
    "hollow_noise",
    "stirring", --180
    "stone_wall",
    "shatter_potion",
    "am_field",
    "fireworks_big",
    "fireworks_norm",
    "fireworks_small",
    "fireworks_launch",

    --[store]
    "casino_inbetween",
    "casino_craps",
    "casino_wheel", --190
    "casino_slots_init",
    "casino_slots",
    "casino_rules",
    "casino_win",
    "casino_lose",
    "home_extend",
    "store_doorbell_enter",
    "store_doorbell_leave",
    "store_paperwork",
    "store_cancel", --200
    "store_repair",
    "store_food_and_drink",
    "store_redeem",
    "store_rest",
    "store_listen",
    "store_prayer",
    "store_enchant",
    "store_recharge",
    "store_id",
    "store_curing", --210
    "store_recall",

    --[item]
    "item_food",
    "item_weapon",
    "item_sword",
    "item_blunt",
    "item_whip",
    "item_axe",
    "item_polearm",
    "item_magestaff",
    "item_boomerang", --220
    "item_bow",
    "item_shot",
    "item_arrow",
    "item_bolt",
    "item_ring",
    "item_amulet",
    "item_lightsource",
    "item_armour_light",
    "item_armour_heavy",
    "item_tool", --230
    "item_tool_digger",

    "item_book",
    "item_scroll",
    "item_bottle",
    "item_potion",
    "item_rune",
    "item_skeleton",
    "item_firestone",
    "item_spike",
    "item_chest", --240
    "item_junk",
    "item_trapkit",
    "item_staff",
    "item_wand",
    "item_rod",
    "item_key",
    "item_golem_wood",
    "item_golem_metal",
    "item_golem_misc",
    "item_seal", --250

    --[ambient]
    "ambient_fireplace",
    "ambient_shore",
    "ambient_lake",
    "ambient_fire",

    "ambient_store_general",
    "ambient_store_armour",
    "ambient_store_weapon",
    "ambient_store_temple",
    "ambient_store_alchemy",
    "ambient_store_magic", --260
    "ambient_store_black",
    "ambient_store_book",
    "ambient_store_rune",
    "ambient_store_merchants",
    "ambient_store_official",
    "ambient_store_casino",
    "ambient_store_misc",

    "animal_bird",
    "animal_owl",
    "animal_seagull", --270
    "animal_toad",
    "animal_wolf",
    "animal_birdofprey",
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
    "ghost",
    "dungeonboss_slain",--90
    "Sauron_slain",
    "ZuAon_slain",

    "store_town",
    "store_blackmarket",
    "store_dungeon",
    "store_casino",
    "store_service",
    "extra",
    "misc",

    "specialunique_slain",--100
    "Nazgul_slain",
    "all_Nazgul_slain",
    "tomb_insanity",
    "wilderness_spring_day",
    "wilderness_spring_night",
    "wilderness_summer_day",
    "wilderness_summer_night",
    "wilderness_autumn_day",
    "wilderness_autumn_night",
    "wilderness_winter_day",--110
    "wilderness_winter_night",

    "wilderness_grass_day",
    "wilderness_grass_night",
    "wilderness_forest_day",
    "wilderness_forest_night",
    "wilderness_mountain_day",
    "wilderness_mountain_night",
    "wilderness_ocean_day",
    "wilderness_ocean_night",
    "wilderness_lake_day",--120
    "wilderness_lake_night",
    "wilderness_swamp_day",
    "wilderness_swamp_night",
    "wilderness_waste_day",
    "wilderness_waste_night",
    "wilderness_desert_day",
    "wilderness_desert_night",
    "wilderness_icywaste_day",
    "wilderness_icywaste_night",

    "town_generic_spring_day",--130
    "town_generic_spring_night",
    "Bree_spring_day",
    "Bree_spring_night",
    "Gondolin_spring_day",
    "Gondolin_spring_night",
    "MinasAnor_spring_day",
    "MinasAnor_spring_night",
    "Lothlorien_spring_day",
    "Lothlorien_spring_night",
    "Khazaddum_spring_day",--140
    "Khazaddum_spring_night",

    "town_generic_summer_day",
    "town_generic_summer_night",
    "Bree_summer_day",
    "Bree_summer_night",
    "Gondolin_summer_day",
    "Gondolin_summer_night",
    "MinasAnor_summer_day",
    "MinasAnor_summer_night",
    "Lothlorien_summer_day",--150
    "Lothlorien_summer_night",
    "Khazaddum_summer_day",
    "Khazaddum_summer_night",

    "town_generic_autumn_day",
    "town_generic_autumn_night",
    "Bree_autumn_day",
    "Bree_autumn_night",
    "Gondolin_autumn_day",
    "Gondolin_autumn_night",
    "MinasAnor_autumn_day",--160
    "MinasAnor_autumn_night",
    "Lothlorien_autumn_day",
    "Lothlorien_autumn_night",
    "Khazaddum_autumn_day",
    "Khazaddum_autumn_night",

    "town_generic_winter_day",
    "town_generic_winter_night",
    "Bree_winter_day",
    "Bree_winter_night",
    "Gondolin_winter_day",--170
    "Gondolin_winter_night",
    "MinasAnor_winter_day",
    "MinasAnor_winter_night",
    "Lothlorien_winter_day",
    "Lothlorien_winter_night",
    "Khazaddum_winter_day",
    "Khazaddum_winter_night",

    "jail_dungeon",
    "meta",
    "season_xmas_day",
    "season_xmas_night",

    "boss_BarrowDowns",--182
    "boss_ThePathsoftheDead",
    "boss_TheOrcCave",
    "boss_TheHelcaraxe",
    "boss_TheLandofRhun",
    "boss_TheSandwormLair",
    "boss_TheOldForest",
    "boss_TheHeartoftheEarth",
    "boss_TheMinesofMoria",--190
    "boss_CirithUngol",
    "boss_TheSmallWaterCave",
    "boss_Mirkwood",
    "boss_DolGuldur",
    "boss_SubmergedRuins",
    "boss_Erebor",
    "boss_TheIllusoryCastle",
    "boss_TheSacredLandofMountains",
    "boss_TheMaze",
    "boss_TheCloudPlanes",--200

    "specialunique_Michael",
    "specialunique_TikSrvzllat",
    "specialunique_Bahamut",
    "specialunique_Hellraiser",
    "specialunique_Dor",--205

    "boss_BarrowDowns_slain",
    "boss_ThePathsoftheDead_slain",
    "boss_TheOrcCave_slain",
    "boss_TheHelcaraxe_slain",
    "boss_TheLandofRhun_slain",--210
    "boss_TheSandwormLair_slain",
    "boss_TheOldForest_slain",
    "boss_TheHeartoftheEarth_slain",
    "boss_TheMinesofMoria_slain",
    "boss_CirithUngol_slain",
    "boss_TheSmallWaterCave_slain",
    "boss_Mirkwood_slain",
    "boss_DolGuldur_slain",
    "boss_SubmergedRuins_slain",
    "boss_Erebor_slain",--220
    "boss_TheIllusoryCastle_slain",
    "boss_TheSacredLandofMountains_slain",
    "boss_TheMaze_slain",
    "boss_TheCloudPlanes_slain",

    "specialunique_Michael_slain",
    "specialunique_TikSrvzllat_slain",
    "specialunique_Bahamut_slain",
    "specialunique_Hellraiser_slain",
    "specialunique_Dor_slain",

    "Nazgul_Uvatha",--230
    "Nazgul_Adunaphel",
    "Nazgul_Akhorahil",
    "Nazgul_Ren",
    "Nazgul_Ji",
    "Nazgul_Dwar",
    "Nazgul_Hoarmurath",
    "Nazgul_Khamul",
    "Nazgul_Witchking",

    "Nazgul_Uvatha_slain",
    "Nazgul_Adunaphel_slain",--240
    "Nazgul_Akhorahil_slain",
    "Nazgul_Ren_slain",
    "Nazgul_Ji_slain",
    "Nazgul_Dwar_slain",
    "Nazgul_Hoarmurath_slain",
    "Nazgul_Khamul_slain",
    "Nazgul_Witchking_slain",

    "event_Halloween_done",--248
}
function get_music_name(idx)
    if audio_bgm[idx + 1] == nil then return "" end
--    if getn(audio_bgm) < idx then return "" end
    return (audio_bgm[idx + 1])
end
--just for do_cmd_options_mus_sdl():
function get_music_index(name)
    local i

    for i = 1, getn(audio_bgm) do
        if audio_bgm[i] == name then
            return(i - 1)
        end
    end
    return (-1)
end
