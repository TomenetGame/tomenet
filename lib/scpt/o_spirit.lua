-- handle the occultism school ('spirit magic')

--[[
OCURSE = add_spell {
	["name"] = 	"Curse",
	["school"] = 	{SCHOOL_},
	["am"] =	75,
	["level"] = 	1,
	["mana"] = 	2,
	["mana_max"] = 	30,
	["fail"] = 	10,
	["stat"] = 	A_WIS,
	["direction"] = function () if get_level(Ind, OCURSE, 50) >= 25 then return FALSE else return TRUE end end,
	["spell"] = 	function(args)
			if get_level(Ind, OCURSE, 50) >= 25 then
				project_los(Ind, GF_CURSE, 10 + get_level(Ind, OCURSE, 150), "points and curses for")
			elseif get_level(Ind, OCURSE, 50) >= 15 then
				fire_beam(Ind, GF_CURSE, args.dir, 10 + get_level(Ind, OCURSE, 150), "points and curses for")
			else
				fire_grid_bolt(Ind, GF_CURSE, args.dir, 10 + get_level(Ind, OCURSE, 150), "points and curses for")
			end
	end,
	["info"] = 	function()
			return "power "..(10 + get_level(Ind, OCURSE, 150))
	end,
	["desc"] = {
			"Randomly causes confusion damage, slowness or blindness.",
			"At level 15 it passes through monsters, affecting those behind as well",
			"At level 25 it affects all monsters in sight",
	}
}
]]

OCURSEDD = add_spell {
	["name"] = 	"Cause wounds",
	["school"] = 	{SCHOOL_SPIRIT},
	["am"] =	75,
	["level"] = 	5,
	["mana"] = 	1,
	["mana_max"] = 	20,
	["fail"] = 	15,
	["stat"] =	A_WIS,
	["ftk"] = 1,
	["spell"] = 	function(args)
			fire_grid_bolt(Ind, GF_MISSILE, args.dir, 10 + get_level(Ind, OCURSEDD, 300), "points and curses for")
	end,
	["info"] = 	function()
			return "power "..(10 + get_level(Ind, OCURSEDD, 300))
	end,
	["desc"] = 	{
			"Curse an enemy, causing wounds.",
	}
}

function get_litebeam_dam()
	return 5 + get_level(Ind, LITEBEAM, 25), 7 + get_level(Ind, LITEBEAM, 25) + 1
end
LITEBEAM = add_spell {
	["name"] = 	"Spear of Light",
	["school"] = 	SCHOOL_SPIRIT,
	["level"] = 	10,
--	["mana"] = 	3,
--	["mana_max"] = 	12,
	["mana"] = 	5,
	["mana_max"] = 	20,
	["fail"] = 	-10,
	["direction"] = TRUE,
	["ftk"] = 1,
	["spell"] = 	function(args)
			fire_beam(Ind, GF_LITE, args.dir, damroll(get_litebeam_dam()), " casts a spear of light for")
	end,
	["info"] = 	function()
			local x, y

			x, y = get_litebeam_dam()
			return "dam "..x.."d"..y
	end,
	["desc"] = 	{
			"Conjures up spiritual light into a powerful beam",
		}
}

OLIGHTNINGBOLT = add_spell {
	["name"] = 	"Lightning",
	["school"] = 	SCHOOL_SPIRIT,
	["level"] = 	10,
	["mana"] = 	3,
	["mana_max"] = 	12,
	["fail"] = 	-10,
	["direction"] = TRUE,
	["ftk"] = 1,
	["spell"] = 	function(args)
			fire_bolt(Ind, GF_LITE, args.dir, damroll(get_lightningbolt_dam()), " casts a light bolt for")
	end,
	["info"] = 	function()
			local x, y

			x, y = get_lightningbolt_dam()
			return "dam "..x.."d"..y
	end,
	["desc"] = 	{
			"Conjures up spiritual power into a lightning bolt",
		}
}

TRANCE = add_spell {
	["name"] = 	"Trance",
	["school"] = 	{SCHOOL_SPIRIT},
	["am"] = 	33,
	["spell_power"] = 0,
	["level"] = 	5,
	["mana"] = 	2,
	["mana_max"] = 	16,
	["fail"] = 	10,
	["direction"] = function() if get_level(Ind, TRANCE, 50) >= 20 then return FALSE else return TRUE end end,
	["spell"] = 	function(args)
--			if get_level(Ind, TRANCE, 50) < 20 then
--				fire_grid_bolt(Ind, GF_OLD_SLEEP, args.dir, 5 + get_level(Ind, TRANCE, 80), "mumbles softly")
--			else
--				project_los(Ind, GF_OLD_SLEEP, 5 + get_level(Ind, TRANCE, 80), "mumbles softly")
--			end
			end,
	["info"] = 	function()
				return "power "..(5 + get_level(Ind, TRANCE, 80))
			end,
	["desc"] = {
			"Causes all ghosts, spirits and elementals that see you",
			"to fall into a deep, spiritual sleep instantly.",
--			"Lets monsters next to you fall sleep",
--			"At level 20 it lets all nearby monsters fall asleep",
	}
}

POSSESS = add_spell {
	["name"] = 	"Possess",
	["school"] = 	{SCHOOL_SPIRIT},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	33,
	["mana"] = 	10,
	["mana_max"] = 	10,
	["fail"] = 	10,
	["direction"] = function () if get_level(Ind, POSSESS, 50) >= 13 then return FALSE else return TRUE end end,
	["spell"] = 	function(args)
			--reset previous charm spell first:
			do_mstopcharm(Ind)
			--cast charm!
			fire_bolt(Ind, GF_CHARMIGNORE, args.dir, 10 + get_level(Ind, POSSESS, 150), "focusses")
	end,
	["info"] = 	function()
--			return "power "..(10 + get_level(Ind, POSSESS, 150))
			return ""
	end,
	["desc"] 	{
			--"Tries to manipulate the mind of an animal to become your pet.",
			"Tries to manipulate the mind of a monster to make it ignore you.",
--			"At level 7 it turns into a ball",
--			"At level 13 it affects all monsters in sight",
	}
}

STOPPOSSESS = add_spell {
	["name"] = 	"Stop Possess",
	["school"] = 	{SCHOOL_SPIRIT},
	["am"] = 	0,
	["spell_power"] = 0,
	["level"] = 	33,
	["mana"] = 	0,
	["mana_max"] = 	0,
	["fail"] = 	-99,
	["direction"] = FALSE,
	["spell"] = 	function()
			do_mstopcharm(Ind)
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] 	{
			"Cancel charming of any monsters",
	}
}

STARLIGHT = add_spell {
	["name"] = 	"Starlight",
	["school"] = 	{SCHOOL_SPIRIT},
	["level"] = 	2,
	["mana"] = 	2,
	["mana_max"] = 	15,
	["fail"] = 	10,
	["spell"] = 	function()
		if get_level(Ind, STARLIGHT, 50) >= 25 then
			msg_print(Ind, "You are surrounded by a globe of light")
			lite_room(Ind, player.wpos, player.py, player.px)
			fire_ball(Ind, GF_LITE, 0, 10 + get_level(Ind, STARLIGHT, 100), 5 + get_level(Ind, STARLIGHT, 6), " calls a globe of light for")
		elseif get_level(Ind, STARLIGHT, 50) >= 10 then
			lite_area(Ind, 10, 4)
		else
			msg_print(Ind, "You are surrounded by a globe of light")
			lite_room(Ind, player.wpos, player.py, player.px)
		end
	end,
	["info"] = 	function()
		if get_level(Ind, STARLIGHT, 50) >= 10 then
			return "dam "..(10 + get_level(Ind, STARLIGHT, 100)).." rad "..(5 + get_level(Ind, STARLIGHT, 6))
		else
			return ""
		end
	end,
	["desc"] = 	{
			"Creates a globe of starlight",
			"At level 10 it damages monsters that are susceptible to light",
			"At level 25 it becomes more powerful and hurts all monsters",
	}
}

DETECTCREATURES = add_spell {
	["name"] = 	"Detect Creatures",
	["school"] = 	{SCHOOL_SPIRIT},
	["level"] = 	3,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	10,
	["spell"] = 	function()
			detect_monsters_xxx(Ind, 0) --detect ALL monsters? (even invis+emptymind)
--			if player.spell_project > 0 then
--				fire_ball(Ind, GF_DETECTINVIS_PLAYER, 0, 1, player.spell_project, "")
--			end
	end,
	["info"] = 	function()
			return "rad "..(10 + get_level(Ind, DETECTCREATURES, 40))
			return ""
	end,
	["desc"] = 	{
			"Detects all nearby creatures.",
--			"***Affected by the Meta spell: Project Spell***",
	}
}
