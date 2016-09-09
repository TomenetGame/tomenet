-- $Id: init.lua,v 1.20 2007/12/28 17:40:19 mikaelh Exp $
-- Server side LUA initialization of TomeNET

-- Get a normal alert to stdout so we can see bugs at server init
__old_ALERT = _ALERT
_ALERT = function(s) print(s) end

-- Beware of the scary undefined globals
function safe_getglobal(x)
	local v = rawget(globals(), x)
	if v then
		return v
	else
		error("undefined global variable'"..x.."'")
	end
end

settagmethod(tag(nil), "getglobal", safe_getglobal)

-- A function for checking if a global is either not defined or not set to 0
function def(x)
        local v = rawget(globals(), x)
        return (not v or v ~= 0)
end

-- Same as def() except that the default value for undefined global can be set
function def_hack(x, default)
	local v = rawget(globals(), x)
	if v then
		return v~= 0
	else
		return default
	end
end

-- Automaticaly update clients, neat heh ? :;=)
pern_dofile(Ind, "update.lua")

-- Init !
pern_dofile(Ind, "player.lua")
pern_dofile(Ind, "s_aux.lua")
pern_dofile(Ind, "powers.lua")

-- Add the schools of magic
pern_dofile(Ind, "spells.lua")

-- Initialize audio
pern_dofile(Ind, "audio.lua")
-- *** Set max values to compare on player login to determine if her/his audio packs are outdated. ***
-- Note that not all fx/songs are neccesarily uncommented, even if they're supported, even in the official packs.
-- Custom packs could be faster and actually implement those already. :)
__audio_sfx_max = getn(audio_sfx)
__audio_mus_max = getn(audio_bgm)

-- Initialize tables
pern_dofile(Ind, "races.lua")
pern_dofile(Ind, "classes.lua")
pern_dofile(Ind, "traits.lua")

-- Put whatever is needed here

pern_dofile(Ind, "cblue.lua")
--pern_dofile(Ind, "dg.lua")
pern_dofile(Ind, "evil.lua")
-- pern_dofile(Ind, "zz.lua") -- file isn't there? -the_sandman
--pern_dofile(Ind, "jir.lua")
pern_dofile(Ind, "it.lua")
pern_dofile(Ind, "mikaelh.lua")


-- Custom scripts that are run automatically - C. Blue
-- 1) on server starting up: server_startup(string time)
-- 2) on 1st player joining since server_startup, additionally execute:
--    first_player_has_joined(int num, int id, string name, string time)
-- 3) on player joining an empty server, additionally execute:
--    player_has_joined_empty_server(int num, int id, string name, string time)
-- 4) on player joining: player_has_joined(int num, int id, string name, string time)
-- 5) on player leaving: player_leaves(int num, int id, string name, string time)
-- 6) on player leaving but still waiting for time out (exited in dungeon):
--    player_leaves_timeout(int num, int id, string name, string time)
-- 7) on player having left completely: player_has_left(int num, int id, string name, string time)
-- 8) on last player having left: last_player_has_left(int num, int id, string name, string time)
-- 9) every hour: cron_1h(string time, int h, int m, int s)
-- 10) every 24 hours: cron_24h(string time, int h, int m, int s)
pern_dofile(Ind, "custom.lua")


-- Restore a good neat handler
_ALERT = __old_ALERT

-- Seed random number generator - mikaelh
randomseed(date("%s"))
