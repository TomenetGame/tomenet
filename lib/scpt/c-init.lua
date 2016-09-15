-- $Id: c-init.lua,v 1.8 2002/09/03 14:01:38 darkgod Exp $
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

-- A function for checking if a global is either not defined or not set to 0
-- The default value in case the global isn't set is true
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


---- INIT client-side scripts ----

--CONFLICT: Must already be done in c-script.c, see there.
 -- Sound system
 --pern_dofile(0, "audio.lua");

 -- Load the xml module
 --pern_dofile(0, "xml.lua");
 --pern_dofile(0, "meta.lua");

pern_dofile(Ind, "player.lua")

--pern_dofile("c-s_aux.lua")
pern_dofile(Ind, "s_aux.lua")

-- Add the schools of magic
pern_dofile(Ind, "spells.lua")

-- Data tables
pern_dofile(Ind, "races.lua")
pern_dofile(Ind, "classes.lua")
pern_dofile(Ind, "traits.lua")
--done in init_lua() already, for early access
--pern_dofile(Ind, "guide.lua")
