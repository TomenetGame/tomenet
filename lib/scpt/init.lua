-- $Id$
-- Server side LUA initialization of TomeNET

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
