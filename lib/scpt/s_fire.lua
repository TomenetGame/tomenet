-- handle the fire school

GLOBELIGHT = add_spell
{
	["name"] = 	"Globe of Light",
        ["school"] = 	{SCHOOL_FIRE},
        ["level"] = 	1,
        ["mana"] = 	2,
        ["mana_max"] = 	15,
        ["fail"] = 	10,
        ["spell"] = 	function()
		local ret, dir

                if get_level(Ind, GLOBELIGHT, 50) > 3 then lite_area(Ind, 10, 4)
                else lite_room(Ind, player.wpos, player.py, player.px) end
                if get_level(Ind, GLOBELIGHT, 50) > 15 then
		        fire_ball(Ind, GF_LITE, 0, 10 + get_level(Ind, GLOBELIGHT, 100), 5 + get_level(Ind, GLOBELIGHT, 6))
		end
		msg_print(Ind, "You are surrounded by a globe of light")
	end,
	["info"] = 	function()
        	if get_level(Ind, GLOBELIGHT, 50) > 15 then
			return "dam "..(10 + get_level(Ind, GLOBELIGHT, 100)).." rad "..(5 + get_level(Ind, GLOBELIGHT, 6))
                else
                	return ""
                end
	end,
        ["desc"] =	{
        		"Creates a globe of pure light",
        		"At level 3 it starts damaging monsters",
        		"At level 15 it starts creating a more powerful kind of light",
        }
}

FIREFLASH = add_spell
{
	["name"] = 	"Fireflash",
        ["school"] = 	{SCHOOL_FIRE},
        ["level"] = 	10,
        ["mana"] = 	5,
        ["mana_max"] = 	30,
        ["fail"] = 	30,
        ["direction"] = TRUE,
        ["spell"] = 	function(args)
                local type
--        	if (get_level(Ind, FIREFLASH, 50) >= 20) then
--	        	type = GF_HOLY_FIRE
--        	else
        		type = GF_FIRE
--	        end
	        fire_ball(Ind, type, args.dir, 20 + get_level(Ind, FIREFLASH, 500), 2 + get_level(Ind, FIREFLASH, 5))
	end,
	["info"] = 	function()
		return "dam "..(20 + get_level(Ind, FIREFLASH, 500)).." rad "..(2 + get_level(Ind, FIREFLASH, 5))
	end,
        ["desc"] =	{
        		"Conjures a ball of fire to burn your foes to ashes"
--                      ,"At level 20 it turns into a ball of holy fire"
        }
}

FIERYAURA = add_spell
{
	["name"] = 	"Fiery Shield",
        ["school"] = 	{SCHOOL_FIRE},
        ["level"] = 	16,
        ["mana"] = 	20,
        ["mana_max"] = 	60,
        ["fail"] = 	50,
        ["spell"] = 	function()
		local type
--        	if (get_level(Ind, FIERYAURA, 50) > 7) then
--	        	type = SHIELD_GREAT_FIRE
--        	else
        		type = SHIELD_FIRE
--	        end
                set_shield(Ind, randint(20) + 10 + get_level(Ind, FIERYAURA, 70), 10, type, 5 + get_level(Ind, FIERYAURA, 10), 5 + get_level(Ind, FIERYAURA, 7))
	end,
	["info"] = 	function()
		return "dam "..(5 + get_level(Ind, FIERYAURA, 15)).."d"..(5 + get_level(Ind, FIERYAURA, 7)).." dur "..(10 + get_level(Ind, FIERYAURA, 70)).."+d20"
	end,
        ["desc"] =	{
        		"Creates a shield of fierce flames around you"
--                      ,"At level 8 it turns into a greater kind of flame that can not be resisted"
        }
}

FIREWALL = add_spell
{
	["name"] = 	"Firewall",
        ["school"] = 	{SCHOOL_FIRE},
        ["level"] = 	20,
        ["mana"] = 	25,
        ["mana_max"] = 	100,
        ["fail"] = 	40,
        ["direction"] = TRUE,
        ["spell"] = 	function(args)
                local type
--        	if (get_level(Ind, FIREWALL, 50) > 5) then
--	        	type = GF_HOLY_FIRE
--        	else
        		type = GF_FIRE
--	        end
	        fire_wall(Ind, type, args.dir, 20 + get_level(Ind, FIREWALL, 150), 6 + get_level(Ind, FIREWALL, 14))
	end,
	["info"] = 	function()
		return "dam "..(20 + get_level(Ind, FIREWALL, 150)).." dur "..(6 + get_level(Ind, FIREWALL, 14))
	end,
        ["desc"] =	{
        		"Creates a fiery wall to incinerate monsters stupid enough to attack you"
--                      ,"At level 6 it turns into a ball of holy fire"
        }
}
