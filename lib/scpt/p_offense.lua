-- handle the holy offense school

HGLOBELIGHT = add_spell
{
	["name"] = 	"Holy Light",
        ["school"] = 	{SCHOOL_HOFFENSE, SCHOOL_HSUPPORT},
        ["level"] = 	1,
        ["mana"] = 	2,
        ["mana_max"] = 	15,
        ["fail"] = 	10,
	["stat"] =      A_WIS,
        ["spell"] = 	function()
		local ret, dir

                if get_level(Ind, HGLOBELIGHT, 50) > 3 then lite_area(Ind, 10, 4)
                else lite_room(Ind, player.wpos, player.py, player.px) end
                if get_level(Ind, HGLOBELIGHT, 50) > 15 then
		        fire_ball(Ind, GF_LITE, 0, 10 + get_level(Ind, HGLOBELIGHT, 100), 5 + get_level(Ind, HGLOBELIGHT, 6), " calls a globe of light of")
		end
		msg_print(Ind, "You are surrounded by a globe of light")
	end,
	["info"] = 	function()
        	if get_level(Ind, HGLOBELIGHT, 50) > 15 then
			return "dam "..(10 + get_level(Ind, HGLOBELIGHT, 100)).." rad "..(5 + get_level(Ind, HGLOBELIGHT, 6))
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

HORBDRAIN = add_spell
{
	["name"] = 	"Orb Of Draining",
        ["school"] = 	{SCHOOL_HOFFENSE},
        ["level"] = 	20,
        ["mana"] = 	5,
        ["mana_max"] = 	30,
        ["fail"] = 	30,
	["stat"] =      A_WIS,
        ["direction"] = TRUE,
        ["spell"] = 	function(args)
                local type
        	type = GF_HOLY_ORB
    		fire_ball(Ind, type, args.dir, 20 + get_level(Ind, HORBDRAIN, 500), 2 + get_level(Ind, HORBDRAIN, 5), " casts a holy orb for")
	end,
	["info"] = 	function()
		return "dam "..(20 + get_level(Ind, HORBDRAIN, 300)).." rad "..(2 + get_level(Ind, HORBDRAIN, 5))
	end,
        ["desc"] =	{
        		"Calls an holy orb to devour the evil",
        }
}

HEXORCISM = add_spell
{
	["name"] = 	"Exorcism",
        ["school"] = 	{SCHOOL_HOFFENSE},
        ["level"] = 	10,
        ["mana"] = 	5,
        ["mana_max"] = 	100,
        ["fail"] = 	30,
	["stat"] =      A_WIS,
        ["spell"] = 	function(args)
                local type
		if get_level(Ind, HEXORCISM, 50) < 20 then
		    dispel_undead(Ind, 10 + get_level(Ind, HEXORCISM, 200))
		elseif get_level(Ind, HEXORCISM, 50) < 35 then
		    dispel_evil(Ind, 10 + get_level(Ind, HEXORCISM, 200))
		else
		    dispel_monsters(Ind, 10 + get_level(Ind, HEXORCISM, 200))
		end
	end,
	["info"] = 	function()
		return "dam "..(10 + get_level(Ind, HEXORCISM, 200))
	end,
        ["desc"] =	{
        		"Dispels nearby undead",
			"At level 20 it dispels all evil",
			"At level 35 it dispels all monsters",
        }
}

HDRAINLIFE = add_spell
{
	["name"] = 	"Drain Life",
        ["school"] = 	{SCHOOL_HOFFENSE},
        ["level"] = 	20,
        ["mana"] = 	5,
        ["mana_max"] = 	80,
        ["fail"] = 	30,
	["stat"] =      A_WIS,
        ["direction"] = TRUE,
        ["spell"] = 	function(args)
                local type
    		drain_life(Ind, args.dir, 10 + get_level(Ind, HDRAINLIFE, 10))
		hp_player(Ind, Players[Ind].ret_dam / 2)
	end,
	["info"] = 	function()
		return "drains "..(10 + get_level(Ind, HDRAINLIFE, 10)).."% life"
	end,
        ["desc"] =	{
        		"Drains life from a target, which must not be non-living or undead.",
        }
}

HRELSOULS = add_spell
{
	["name"] = 	"Release Souls",
	["school"] =	{SCHOOL_HOFFENSE},
	["level"] =	10,
	["mana"] = 	5,
	["max_mana"] =	100,
	["fail"] =	25,
	["stat"] =      A_WIS,
	["spell"] =	function(args)
			dispel_undead(Ind, 10 + get_level(Ind, HRELSOULS, 1000))
			end,
	["info"] = 	function()
		return "dam "..(10 + get_level(Ind, HRELSOULS, 1000))
	end,
        ["desc"] =	{
        		"Banishes nearby undead.",
			}
}

--[[
HHOLYWORD = add_spell
{
	["name"] = 	"Holy Word",
	["school"] =	{SCHOOL_HOFFENSE, SCHOOL_HCURING},
	["level"] =	45,
	["mana"] = 	500,
	["fail"] =	30,
	["stat"] =      A_WIS,
	["spell"] =	function(args)
			hp_player(Ind, 1000)
			set_afraid(Ind, 0)
			set_poisoned(Ind, 0)
			set_stun(Ind, 0)
			set_cut(Ind, 0)
			end,
	["info"] =	function()
			return "Dispels & heals."
			end,
	["desc"] =	{
			"Dispels evil, heals and cures you."
			}
}
]]