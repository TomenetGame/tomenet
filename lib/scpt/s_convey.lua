-- $Id$
-- handle the conveyance school

BLINK = add_spell
{
	["name"] = 	"Phase Door",
        ["school"] = 	{SCHOOL_CONVEYANCE},
        ["level"] = 	1,
        ["mana"] = 	1,
        ["mana_max"] =  3,
        ["fail"] = 	10,
        ["direction"] = FALSE,
        ["item"] = FALSE,
        ["spell"] = 	function()
        	        if get_level(BLINK, 50) >= 30 then
                                local oy, ox = py, px

        	        	teleport_player(10 + get_level(BLINK, 8))
--                                create_between_gate(0, oy, ox)
                	else
        	        	teleport_player(10 + get_level(BLINK, 8))
	                end
	end,
	["info"] = 	function()
	                if get_level(BLINK, 50) >= 30 then
        	        	return "distance "..(5 + get_level(BLINK, 8))
                	else
                		return "distance "..(10 + get_level(BLINK, 8))
	                end
	end,
        ["desc"] =	{
        		"Teleports you on a small scale range",
                        "At level 30 it creates void jumpgates",
        }
}

--[[
BLINK = add_spell
{
	["name"] = 	"Phase Door",
        ["school"] = 	{SCHOOL_CONVEYANCE},
        ["level"] = 	1,
        ["mana"] = 	1,
        ["mana_max"] =  3,
        ["fail"] = 	10,
        ["spell"] = 	function()
        	        if get_level(BLINK, 50) >= 30 then
                                local oy, ox = py, px

        	        	teleport_player(10 + get_level(BLINK, 8))
                                create_between_gate(0, oy, ox)
                	else
        	        	teleport_player(10 + get_level(BLINK, 8))
	                end
	end,
	["info"] = 	function()
	                if get_level(BLINK, 50) >= 30 then
        	        	return "distance "..(5 + get_level(BLINK, 8))
                	else
                		return "distance "..(10 + get_level(BLINK, 8))
	                end
	end,
        ["desc"] =	{
        		"Teleports you on a small scale range",
                        "At level 30 it creates void jumpgates",
        }
}

DISARM = add_spell
{
	["name"] = 	"Disarm",
        ["school"] = 	{SCHOOL_CONVEYANCE},
        ["level"] = 	3,
        ["mana"] = 	2,
        ["mana_max"] = 	4,
        ["fail"] = 	10,
        ["spell"] = 	function()
			destroy_doors_touch()
        	        if get_level(DISARM, 50) >= 10 then destroy_traps_touch() end
	end,
	["info"] = 	function()
                	return ""
	end,
        ["desc"] =	{
        		"Destroys doors",
        		"At level 10 it disarms traps",
        }
}

TELEPORT = add_spell
{
	["name"] = 	"Teleportation",
        ["school"] = 	{SCHOOL_CONVEYANCE},
        ["level"] = 	10,
        ["mana"] = 	8,
        ["mana_max"] = 	14,
        ["fail"] = 	50,
        ["spell"] = 	function()
        		player.energy = player.energy - (25 - get_level(TELEPORT, 50))
			teleport_player(100 + get_level(TELEPORT, 100))
	end,
	["info"] = 	function()
        		return ""
	end,
        ["desc"] =	{
        		"Teleports you around the level. The casting time decreases with level",
        }
}

TELEAWAY = add_spell
{
	["name"] = 	"Teleport Away",
        ["school"] = 	{SCHOOL_CONVEYANCE},
        ["level"] = 	23,
        ["mana"] = 	15,
        ["mana_max"] = 	40,
        ["fail"] = 	70,
        ["spell"] = 	function()
               		local ret, dir

        		if get_level(TELEAWAY, 50) >= 20 then
                                project_los(GF_AWAY_ALL, 100)
                        elseif get_level(TELEAWAY, 50) >= 10 then
        	                ret, dir = get_aim_dir()
                                if ret == FALSE then return FALSE end
                                fire_ball(GF_AWAY_ALL, dir, 100, 3 + get_level(TELEAWAY, 4))
                        else
        	                ret, dir = get_aim_dir()
                                if ret == FALSE then return FALSE end
                                teleport_monster(dir)
			end
	end,
	["info"] = 	function()
        		return ""
	end,
        ["desc"] =	{
                        "Teleports a line of monsters away",
                        "At level 10 it turns into a ball",
                        "At level 20 it teleports all monsters in sight"
        }
}

RECALL = add_spell
{
	["name"] = 	"Recall",
        ["school"] = 	{SCHOOL_CONVEYANCE},
        ["level"] = 	30,
        ["mana"] = 	25,
        ["mana_max"] = 	25,
        ["fail"] =      20,
        ["spell"] = 	function()
        		local ret, x, y, c_ptr
                        ret, x, y = tgt_pt()
                        if ret == FALSE then return end
                        c_ptr = cave(y, x)
                        if (y == py) and (x == px) then
                        	recall_player(21 - get_level(RECALL, 15), 15 - get_level(RECALL, 10))
                        elseif c_ptr.m_idx > 0 then
                        	swap_position(y, x)
                        elseif c_ptr.o_idx > 0 then
                        	set_target(y, x)
                                if get_level(RECALL, 50) >= 15 then
	                        	fetch(5, 10 + get_level(RECALL, 150), FALSE)
                                else
	                        	fetch(5, 10 + get_level(RECALL, 150), TRUE)
                                end
                        end
	end,
	["info"] = 	function()
			return "dur "..(15 - get_level(RECALL, 10)).."+d"..(21 - get_level(RECALL, 15)).." weight "..(1 + get_level(RECALL, 15)).."lb"
	end,
        ["desc"] =	{
        		"Cast on yourself it will recall you to the surface/dungeon.",
        		"Cast at a monster you will swap positions with the monster.",
                        "Cast at an object it will fetch the object to you."
        }
}
]]
