-- File: lib/scpt/adventures.lua - Kurzel
-- https://www.lua.org/manual/4.0/manual.html

-- Server-side use only.
-- Supports GE_ADVENTURE events with ties to module.lua and quests.lua
-- To reload this file as a DM (thus applying changes w/o a restart):
-- / pern_dofile(0, "adventures.lua")

-- HACKS

LEVEL_RAND = 2

SFX_AMBIENT_NONE = -1
SFX_AMBIENT_FIREPLACE = 0
SFX_AMBIENT_SHORE = 1

WEATHER_GEN_TICKS = 3

WEATHER_NONE = 0
WEATHER_RAIN = 1
WEATHER_SNOW = 2
WEATHER_SAND = 3

WIND_STILL = 0
WIND_WINDY = 3

SEASON_NORMAL = 8
SEASON_WINTER = 5

INTENSE_SNOW = 9
INTENSE_WIND = 3
INTENSE_RAIN = 2

SPEED_SNOW = 9
SPEED_RAIN = 3

-- STORE_HIDDENLIBRARY = 65 -- TODO - place_store() where cs.omni = this int

-- TABLES

LOCALE_00 = { -- dlvl loc_pre desc music ambient
-- Under Elmoth
[2] = {50,"under","Nan Elmoth",32,SFX_AMBIENT_NONE}, -- Mirkwood
-- Stormy Isle
[3] = {150,"at the","Stormy Isle",35,SFX_AMBIENT_SHORE}, -- Submerged Ruins
-- Unoccupied
[4] = {-2,"","",0,SFX_AMBIENT_NONE},
[5] = {-2,"","",0,SFX_AMBIENT_NONE},
[6] = {-2,"","",0,SFX_AMBIENT_NONE},
[7] = {-2,"","",0,SFX_AMBIENT_NONE},
[8] = {-2,"","",0,SFX_AMBIENT_NONE},
[9] = {-2,"","",0,SFX_AMBIENT_NONE},
[10] = {-2,"","",0,SFX_AMBIENT_NONE}
} -- dlvl = depth level for apply_magic() / place_item_module() chests, etc.

WEATHER_00 = { -- weather wind season intensity speed
-- Under Elmoth
[2] = {0,0,0,0,0},
-- Stormy Isle
[3] = {WEATHER_RAIN,WIND_WINDY,SEASON_NORMAL,INTENSE_WIND,SPEED_RAIN}, -- Light Rain
-- Unoccupied
[4] = {0,0,0,0,0},
[5] = {0,0,0,0,0},
[6] = {0,0,0,0,0},
[7] = {0,0,0,0,0},
[8] = {0,0,0,0,0},
[9] = {0,0,0,0,0},
[10] = {0,0,0,0,0}
}

GE_TYPE = { -- announcement_time signup_time end_turn min_participants limited noghost challenge
["Under Elmoth"] = {5,0,35,0,0,1,1}, -- ge->state[1] = (challenge ? 1 : 0)
["Stormy Isle"] = {1,0,45,1,6,0,0}
} -- time in minutes; "sensible announcement_time" of 5m minimum avoids double announce

GE_EXTRA = { -- INDEX level req. (min,max) 00depth (min,max,entry,exit)
["Under Elmoth"] = {1,0,1,2,2,2,2}, -- min 0 = newly created level 1 only
["Stormy Isle"] = {2,1,21,3,3,3,3}
} -- reordering this ? find GE_EXTRA to update ge->extra[i-1] hardcode, xtra1.c

GE_DESCRIPTION = { -- description (lines [0,9])

["Under Elmoth"] = {
" Create a new character and challenge the forbidding dark-elven halls  ",
" under sunken Nan Elmoth. Once a place of beauty, now its cavernous    ",
" chambers contain only dread! Triumph over the corruption if you can!  ",
"                                                                       ",
" -- Prohibitions: ESP, Mapping, Detection, Genocide, *Destruction*.    ",
" -- Restrictions: Zero experience required. 30 minutes to explore.     ",
"                                                                       ",
" Hints: BEWARE! Some encounters will challenge different playstyles.   ",
"        Try various strategies or participate in a group to succeed!   ",
"                                                                       "},

["Stormy Isle"] = {
" Form an expedition team of 2-6 experienced characters and explore a   ",
" stormy islet off the southern coast of Middle-earth.                  ",
"                                                                       ",
" -- Prohibitions: Genocide, *Destruction*.                             ",
" -- Restrictions: Levels 14-21 only. 30 minutes to explore.            ",
"                                                                       ",
"",
"",
"",
""}
}

-- FUNCTIONS

-- z = sector00 tower depths [2,10] #define in_module(wpos) / above PVPARENA_Z

function adventure_locale(z,j)
 return LOCALE_00[z][j]
end

function adventure_weather(z,j)
 return WEATHER_00[z][j]
end

-- t = event title, defined here and executed with /gestart <title>

function adventure_type(t,j)
 if GE_TYPE[t] then return GE_TYPE[t][j] else return 0 end
end

function adventure_extra(t,j)
 if GE_EXTRA[t] then return GE_EXTRA[t][j] else return 0 end
end

function adventure_description(t,j)
  if GE_DESCRIPTION[t][j] then return GE_DESCRIPTION[t][j] else return "" end
end

function adventure_count(t)
  count = 0
  for i = GE_EXTRA[t][4],GE_EXTRA[t][5] do
    count = count + players_on_depth(make_wpos(0,0,i))
  end
  return count
end

function adventure_empty(t)
  return adventure_count(t) < (2 + GE_EXTRA[t][5] - GE_EXTRA[t][4])
end

function adventure_start(Ind,t)

  -- (Re)load module, if empty (DM presence is counted!)
  if adventure_empty(t) then
    if t=="Under Elmoth" then
      module_load(0,0,2,"elmoth",0)
    end
    if t=="Stormy Isle" then
      module_load(0,0,3,"isle",1) -- 1 = daytime, light out
    end
  end

  -- Relocate the adventurer
  players(Ind).recall_pos.wx = 0
  players(Ind).recall_pos.wy = 0
  players(Ind).recall_pos.wz = GE_EXTRA[t][6]
  players(Ind).new_level_method = LEVEL_RAND
  recall_player(Ind, "")

  -- msg_print(Ind,"Thanks for testing!")
end
