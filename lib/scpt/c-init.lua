-- $Id$
-- Client side LUA initialization of TomeNET

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

pern_dofile(Ind, "player.lua")

function testtest(i)
-- silly
--	assert(i, "KABOOM!")
end

--pern_dofile("c-s_aux.lua")
pern_dofile(Ind, "s_aux.lua")


-- Add the schools of magic

pern_dofile(Ind, "spells.lua")

