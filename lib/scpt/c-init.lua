-- $Id$
-- Client side LUA initialization of TomeNET


pern_dofile("player.lua")

function testtest(i)
	assert(i, "KABOOM!")
end

--pern_dofile("c-s_aux.lua")
pern_dofile("s_aux.lua")


-- Add the schools of magic

pern_dofile("spells.lua")

