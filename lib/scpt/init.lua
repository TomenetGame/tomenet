-- $Id$
-- Server side LUA initialization of TomeNET

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

-- Add the schools of magic
pern_dofile(Ind, "spells.lua")


-- Put whatever is needed here

pern_dofile(Ind, "dg.lua")
pern_dofile(Ind, "evil.lua")
pern_dofile(Ind, "zz.lua")
pern_dofile(Ind, "jir.lua")
