-- Handles thhe divination school


STARIDENTIFY = add_spell
{
	["name"] = 	"Greater Identify",
        ["school"] = 	{SCHOOL_DIVINATION},
        ["level"] = 	35,
        ["mana"] = 	30,
        ["mana_max"] = 	30,
        ["fail"] = 	10,
        ["stat"] =      A_WIS,
        ["extra"] =     function () return get_check("Cast on yourself?") end,
        ["spell"] = 	function (args)
        		if args.aux == TRUE then
                        	self_knowledge(Ind)
                        else
                        	identify_fully(Ind)
                        end
	end,
	["info"] = 	function()
                       	return ""
	end,
        ["desc"] =	{
        		"Asks for an object and fully identify it, providing the full list of powers",
                        "Cast at yourself it will reveal your powers"
        }
}

IDENTIFY = add_spell
{
	["name"] = 	"Identify",
        ["school"] = 	{SCHOOL_DIVINATION},
        ["level"] = 	8,
        ["mana"] = 	10,
        ["mana_max"] = 	50,
        ["fail"] = 	10,
        ["stat"] =      A_WIS,
        ["spell"] = 	function()
        		if get_level(Ind, IDENTIFY, 50) >= 27 then
                        	identify_pack(Ind)
                                fire_ball(Ind, GF_IDENTIFY, 0, 1, get_level(Ind, IDENTIFY, 3), "")
        		elseif get_level(Ind, IDENTIFY, 50) >= 17 then
                        	identify_pack(Ind)
                                fire_ball(Ind, GF_IDENTIFY, 0, 1, 0, "")
                        else
                        	ident_spell(Ind)
                        end
	end,
	["info"] = 	function()
        		if get_level(Ind, IDENTIFY, 50) >= 27 then
				return "rad "..(get_level(Ind, IDENTIFY, 3))
                        else
                        	return ""
                        end
	end,
        ["desc"] =	{
        		"Asks for an object and identifies it",
                        "At level 17 it identifies all objects in the inventory",
                        "At level 27 it identifies all objects in the inventory and in a",
                        "radius on the floor, as well as probing monsters in that radius"
        }
}

VISION = add_spell
{
	["name"] = 	"Vision",
        ["school"] = 	{SCHOOL_DIVINATION},
        ["level"] = 	15,
        ["mana"] = 	7,
        ["mana_max"] = 	55,
        ["stat"] =      A_WIS,
        ["fail"] = 	10,
        ["spell"] = 	function()
        		if get_level(Ind, VISION, 50) >= 25 then
                        	wiz_lite_extra(Ind)
                        else
                        	map_area(Ind)
                        end
                        if player.spell_project > 0 then
                                fire_ball(Ind, GF_SEEMAP_PLAYER, 0, 1, player.spell_project, "")
                        end
	end,
	["info"] = 	function()
			return ""
	end,
        ["desc"] =	{
                        "Detects the layout of the surrounding area",
                        "At level 25 it maps and lights the whole level",
                        "***Affected by the Meta spell: Project Spell***",
        }
}

SENSEHIDDEN = add_spell
{
	["name"] = 	"Sense Hidden",
        ["school"] = 	{SCHOOL_DIVINATION},
        ["level"] = 	5,
        ["mana"] = 	2,
        ["mana_max"] = 	10,
        ["fail"] = 	10,
        ["stat"] =      A_WIS,
        ["spell"] = 	function()
        		detect_trap(Ind, 10 + get_level(Ind, SENSEHIDDEN, 40, 0))
                        if player.spell_project > 0 then
                                fire_ball(Ind, GF_DETECTTRAP_PLAYER, 0, 1, player.spell_project, "")
                        end
        		if get_level(Ind, SENSEHIDDEN, 50) >= 10 then
                        	set_tim_invis(Ind, 10 + randint(20) + get_level(Ind, SENSEHIDDEN, 40))
	                        if player.spell_project > 0 then
        	                        fire_ball(Ind, GF_SEEINVIS_PLAYER, 0, 10 + randint(20) + get_level(Ind, SENSEHIDDEN, 40), player.spell_project, "")
                	        end
                        end
	end,
	["info"] = 	function()
        		if get_level(Ind, SENSEHIDDEN, 50) >= 10 then
				return "rad "..(10 + get_level(Ind, SENSEHIDDEN, 40)).." dur "..(10 + get_level(Ind, SENSEHIDDEN, 40)).."+d20"
			else
                                return "rad "..(10 + get_level(Ind, SENSEHIDDEN, 40))
                        end
	end,
        ["desc"] =	{
        		"Detects the traps in a certain radius around you",
                        "At level 15 it allows you to sense invisible for a while",
                        "***Affected by the Meta spell: Project Spell***",
        }
}

REVEALWAYS = add_spell
{
	["name"] = 	"Reveal Ways",
        ["school"] = 	{SCHOOL_DIVINATION},
        ["level"] = 	9,
        ["mana"] = 	3,
        ["mana_max"] = 	15,
        ["fail"] = 	10,
        ["stat"] =      A_WIS,
        ["spell"] = 	function()
        		detect_sdoor(Ind, 10 + get_level(Ind, REVEALWAYS, 40, 0))
                        if player.spell_project > 0 then
                                fire_ball(Ind, GF_DETECTDOOR_PLAYER, 0, 1, player.spell_project, "")
                        end
	end,
	["info"] = 	function()
                        return "rad "..(10 + get_level(Ind, REVEALWAYS, 40))
	end,
        ["desc"] =	{
        		"Detects the doors/stairs/ways in a certain radius around you",
                        "***Affected by the Meta spell: Project Spell***",
        }
}

SENSEMONSTERS = add_spell
{
	["name"] = 	"Sense Monsters",
        ["school"] = 	{SCHOOL_DIVINATION},
        ["level"] = 	1,
        ["mana"] =      1,
        ["mana_max"] =  20,
        ["fail"] = 	10,
        ["stat"] =      A_WIS,
        ["spell"] = 	function()
                        detect_creatures(Ind)
                        if player.spell_project > 0 then
                                fire_ball(Ind, GF_DETECTCREATURE_PLAYER, 0, 1, player.spell_project, "")
                        end
        		if get_level(Ind, SENSEMONSTERS, 50) >= 30 then
                        	set_tim_esp(Ind, 10 + randint(10) + get_level(Ind, SENSEMONSTERS, 20))
                        end
	end,
	["info"] = 	function()
        		if get_level(Ind, SENSEMONSTERS, 50) >= 30 then
                                return "rad "..(10 + get_level(Ind, SENSEMONSTERS, 40)).." dur "..(10 + get_level(Ind, SENSEMONSTERS, 20)).."+d10"
			else
                                return "rad "..(10 + get_level(Ind, SENSEMONSTERS, 40))
                        end
	end,
        ["desc"] =	{
        		"Detects all monsters near you",
                        "At level 30 it allows you to sense monster minds for a while",
                        "***Affected by the Meta spell: Project Spell***",
        }
}
