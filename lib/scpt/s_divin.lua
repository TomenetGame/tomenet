-- Handles thhe divination school


STARIDENTIFY = add_spell {
	["name"] = 	"Greater Identify",
	["school"] = 	{SCHOOL_DIVINATION},
	["level"] = 	35,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	10,
--	["stat"] = 	A_WIS,
	["extra"] = 	function () return get_check2("Cast on yourself?", FALSE) end,
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
	["desc"] = 	{
			"Fully identifies an object, providing the complete list of its powers.",
			"Cast at yourself it will reveal your powers."
	}
}

IDENTIFY_I = add_spell {
	["name"] = 	"Identify I",
	["school"] = 	{SCHOOL_DIVINATION},
	["level"] = 	8,
	["mana"] = 	10,
	["mana_max"] = 	10,
	["fail"] = 	20,
--	["stat"] = 	A_WIS,
	["spell"] = 	function()
			ident_spell(Ind)
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{ "Asks for an object and identifies it.", }
}
IDENTIFY_II = add_spell {
	["name"] = 	"Identify II",
	["school"] = 	{SCHOOL_DIVINATION},
	["level"] = 	25,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	-20,
--	["stat"] = 	A_WIS,
	["spell"] = 	function()
			identify_pack(Ind)
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{ "Identifies all objects in your inventory.", }
}
IDENTIFY_III = add_spell {
	["name"] = 	"Identify III",
	["school"] = 	{SCHOOL_DIVINATION},
	["level"] = 	35,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	-40,
--	["stat"] = 	A_WIS,
	["spell"] = 	function()
			identify_pack(Ind)
			fire_ball(Ind, GF_IDENTIFY_I, 0, 1, get_level(Ind, IDENTIFY_I, 3), "")
	end,
	["info"] = 	function()
			return "rad "..(get_level(Ind, IDENTIFY_I, 3))
	end,
	["desc"] = 	{
			"Identifies all objects in your inventory and in a radius",
			"on the floor, as well as probing monsters in that radius."
	}
}

VISION_I = add_spell {
	["name"] = 	"Vision I",
	["school"] = 	{SCHOOL_DIVINATION},
	["level"] = 	15,
	["mana"] = 	7,
	["mana_max"] = 	7,
--	["stat"] = 	A_WIS,
	["fail"] = 	10,
	["spell"] = 	function()
			map_area(Ind)
			if player.spell_project > 0 then
				fire_ball(Ind, GF_SEEMAP_PLAYER, 0, 1, player.spell_project, "")
			end
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{
			"Detects the layout of the surrounding area.",
			"***Affected by the Meta spell: Project Spell***",
	}
}
VISION_II = add_spell {
	["name"] = 	"Vision II",
	["school"] = 	{SCHOOL_DIVINATION},
	["level"] = 	40,
	["mana"] = 	55,
	["mana_max"] = 	55,
--	["stat"] = 	A_WIS,
	["fail"] = 	-40,
	["spell"] = 	function()
			wiz_lite_extra(Ind)
			if player.spell_project > 0 then
				fire_ball(Ind, GF_SEEMAP_PLAYER, 0, 1, player.spell_project, "")
			end
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{
			"Gives clairvoyance, mapping and lighting up the whole dungeon.",
			"***Affected by the Meta spell: Project Spell***",
	}
}

SENSEHIDDEN_I = add_spell {
	["name"] = 	"Sense Hidden I",
	["school"] = 	{SCHOOL_DIVINATION},
	["level"] = 	5,
	["mana"] = 	2,
	["mana_max"] = 	2,
	["fail"] = 	10,
--	["stat"] = 	A_WIS,
	["spell"] = 	function()
			detect_trap(Ind, 10 + get_level(Ind, SENSEHIDDEN_I, 40, 0))
			if player.spell_project > 0 then
				fire_ball(Ind, GF_DETECTTRAP_PLAYER, 0, 1, player.spell_project, "")
			end
	end,
	["info"] = 	function()
			return "rad "..(10 + get_level(Ind, SENSEHIDDEN_I, 40))
	end,
	["desc"] = 	{
			"Detects the traps in a certain radius around you.",
			"***Affected by the Meta spell: Project Spell***",
	}
}
SENSEHIDDEN_II = add_spell {
	["name"] = 	"Sense Hidden II",
	["school"] = 	{SCHOOL_DIVINATION},
	["level"] = 	20,
	["mana"] = 	10,
	["mana_max"] = 	10,
	["fail"] = 	-10,
--	["stat"] = 	A_WIS,
	["spell"] = 	function()
			detect_trap(Ind, 10 + get_level(Ind, SENSEHIDDEN_I, 40, 0))
			if player.spell_project > 0 then
				fire_ball(Ind, GF_DETECTTRAP_PLAYER, 0, 1, player.spell_project, "")
			end
			set_tim_invis(Ind, 10 + randint(20) + get_level(Ind, SENSEHIDDEN_I, 40))
			if player.spell_project > 0 then
				fire_ball(Ind, GF_SEEINVIS_PLAYER, 0, 10 + randint(20) + get_level(Ind, SENSEHIDDEN_I, 40), player.spell_project, "")
			end
	end,
	["info"] = 	function()
			return "rad "..(10 + get_level(Ind, SENSEHIDDEN_I, 40)).." dur "..(10 + get_level(Ind, SENSEHIDDEN_I, 40)).."+d20"
	end,
	["desc"] = 	{
			"Detects the traps in a certain radius around you",
			"and allows you to see invisible for a while.",
			"***Affected by the Meta spell: Project Spell***",
	}
}

REVEALWAYS = add_spell {
	["name"] = 	"Reveal Ways",
	["school"] = 	{SCHOOL_DIVINATION},
	["level"] = 	9,
	["mana"] = 	3,
	["mana_max"] = 	15,
	["fail"] = 	10,
--	["stat"] = 	A_WIS,
	["spell"] = 	function()
			detect_sdoor(Ind, 10 + get_level(Ind, REVEALWAYS, 40, 0))
			if player.spell_project > 0 then
				fire_ball(Ind, GF_DETECTDOOR_PLAYER, 0, 1, player.spell_project, "")
			end
	end,
	["info"] = 	function()
			return "rad "..(10 + get_level(Ind, REVEALWAYS, 40))
	end,
	["desc"] = 	{
			"Detects the doors/stairs/ways in a certain radius around you.",
			"***Affected by the Meta spell: Project Spell***",
	}
}

DETECTMONSTERS = add_spell {
	["name"] = 	"Detect Monsters",
	["school"] = 	{SCHOOL_DIVINATION},
	["level"] = 	3,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	10,
	["spell"] = 	function()
			detect_creatures(Ind)
			if player.spell_project > 0 then
				fire_ball(Ind, GF_DETECTCREATURE_PLAYER, 0, 1, player.spell_project, "")
			end
	end,
	["info"] = 	function()
--			return "rad "..(10 + get_level(Ind, DETECTMONSTERS, 40))
			return ""
	end,
	["desc"] = 	{
			"Detects all nearby non-invisible creatures.",
			"***Affected by the Meta spell: Project Spell***",
	}
}
