-- $Id$
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

-- Init !
pern_dofile(Ind, "player.lua")
pern_dofile(Ind, "s_aux.lua")
pern_dofile(Ind, "powers.lua")

-- Add the schools of magic
pern_dofile(Ind, "spells.lua")

-- Automaticaly update clients, neat heh ? :;=)
pern_dofile(Ind, "update.lua")


-- Put whatever is needed here

pern_dofile(Ind, "dg.lua")
pern_dofile(Ind, "evil.lua")
pern_dofile(Ind, "zz.lua")
pern_dofile(Ind, "jir.lua")

-- Restore a good neat handler
_ALERT = __old_ALERT
