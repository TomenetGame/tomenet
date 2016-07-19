-- handle the occultism school ('spirit magic')

--[[
OCURSE = add_spell {
	["name"] = 	"Curse",
	["school"] = 	{SCHOOL_},
	["am"] = 	75,
	["spell_power"] = 0,
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

OCURSEDD_I = add_spell {
	["name"] = 	"Cause wounds I",
	["school"] = 	{SCHOOL_OSPIRIT},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	1,
	["mana"] = 	1,
	["mana_max"] = 	1,
	["fail"] = 	10,
	["stat"] = 	A_WIS,
	["ftk"] = 1,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		fire_grid_bolt(Ind, GF_MISSILE, args.dir, 10 + get_level(Ind, OCURSEDD_I, 100), "points and curses for")
	end,
	["info"] = 	function()
		return "power "..(10 + get_level(Ind, OCURSEDD_I, 100))
	end,
	["desc"] = 	{ "Curse an enemy, causing wounds.", }
}
OCURSEDD_II = add_spell {
	["name"] = 	"Cause wounds II",
	["school"] = 	{SCHOOL_OSPIRIT},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	20,
	["mana"] = 	5,
	["mana_max"] = 	5,
	["fail"] = 	-30,
	["stat"] = 	A_WIS,
	["ftk"] = 1,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		fire_grid_bolt(Ind, GF_MISSILE, args.dir, 10 + 100 + get_level(Ind, OCURSEDD_II, 100), "points and curses for")
	end,
	["info"] = 	function()
		return "power "..(10 + 100 + get_level(Ind, OCURSEDD_II, 100))
	end,
	["desc"] = 	{ "Curse an enemy, causing wounds.", }
}
OCURSEDD_III = add_spell {
	["name"] = 	"Cause wounds III",
	["school"] = 	{SCHOOL_OSPIRIT},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	40,
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	-80,
	["stat"] = 	A_WIS,
	["ftk"] = 1,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		fire_grid_bolt(Ind, GF_MISSILE, args.dir, 10 + 200 + get_level(Ind, OCURSEDD_III, 100), "points and curses for")
	end,
	["info"] = 	function()
		return "power "..(10 + 200 + get_level(Ind, OCURSEDD_III, 100))
	end,
	["desc"] = 	{ "Curse an enemy, causing wounds.", }
}

ODELFEAR = add_spell {
	["name"] = 	"Tame Fear",
	["school"] = 	{SCHOOL_OSPIRIT},
	["spell_power"] = 0,
	["am"] = 	50,
	["level"] = 	1,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	10,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			fire_ball(Ind, GF_REMFEAR_PLAYER, 0, get_level(Ind, ODELFEAR, 30 * 2), 4, " speaks some words of insight and you lose your fear.")
			set_afraid(Ind, 0)
			set_res_fear(Ind, get_level(Ind, ODELFEAR, 30))
			end,
	["info"] = 	function()
			return "dur "..get_level(Ind, ODELFEAR, 30)
			end,
	["desc"] = 	{
			"Removes fear from your heart for a while.",
			"***Automatically projecting***",
	}
}

STARLIGHT_I = add_spell {
	["name"] = 	"Starlight I",
	["school"] = 	{SCHOOL_OSPIRIT},
	["spell_power"] = 0,
	["level"] = 	2,
	["mana"] = 	4,
	["mana_max"] = 	4,
	["fail"] = 	10,
	["spell"] = 	function()
		if get_level(Ind, STARLIGHT_I, 50) >= 10 then
			lite_area(Ind, 10, 4)
		else
			msg_print(Ind, "You are surrounded by a globe of light")
			lite_room(Ind, player.wpos, player.py, player.px)
		end
	end,
	["info"] = 	function()
		if get_level(Ind, STARLIGHT_I, 50) >= 10 then
			return "dam "..(10 + get_level(Ind, STARLIGHT_I, 100)).." rad "..(5 + get_level(Ind, STARLIGHT_I, 6))
		else
			return ""
		end
	end,
	["desc"] = 	{
		"Creates a globe of starlight.",
		"At level 10 it damages monsters that are susceptible to light.",
	}
}
STARLIGHT_II = add_spell {
	["name"] = 	"Starlight II",
	["school"] = 	{SCHOOL_OSPIRIT},
	["spell_power"] = 0,
	["level"] = 	22,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	-20,
	["spell"] = 	function()
		msg_print(Ind, "You are surrounded by a globe of light")
		lite_room(Ind, player.wpos, player.py, player.px)
		fire_ball(Ind, GF_LITE, 0, 10 + get_level(Ind, STARLIGHT_I, 100), 5 + get_level(Ind, STARLIGHT_I, 6), " calls a globe of light for")
	end,
	["info"] = 	function()
		return "dam "..(10 + get_level(Ind, STARLIGHT_I, 100)).." rad "..(5 + get_level(Ind, STARLIGHT_I, 6))
	end,
	["desc"] = 	{ "Creates a globe of starlight, powerful enough to hurt monsters.", }
}

DETECTCREATURES = add_spell {
	["name"] = 	"Detect Creatures",
	["school"] = 	{SCHOOL_OSPIRIT},
	["spell_power"] = 0,
	["level"] = 	7,
	["mana"] = 	5,
	["mana_max"] = 	5,
	["fail"] = 	10,
	["spell"] = 	function()
		detect_creatures_xxx(Ind, 0) --detect ALL monsters? (even invis+emptymind)
--		if player.spell_project > 0 then
--			fire_ball(Ind, GF_DETECTINVIS_PLAYER, 0, 1, player.spell_project, "")
--		end
	end,
	["info"] = 	function()
--		return "rad "..(10 + get_level(Ind, DETECTCREATURES, 40))
		return ""
	end,
	["desc"] = 	{
		"Detects all nearby creatures.",
--		"***Affected by the Meta spell: Project Spell***",
	}
}

function get_litebeam_dam(Ind, limit_lev)
	--return 5 + get_level(Ind, LITEBEAM, 25), 7 + get_level(Ind, LITEBEAM, 25) + 1
	local lev

	lev = get_level(Ind, LITEBEAM_I, 50)
	if limit_lev ~= 0 and lev > limit_lev then lev = limit_lev + (lev - limit_lev) / 3 end

	return 5 + (lev / 2), 7 + (lev / 2) + 1
end
LITEBEAM_I = add_spell {
	["name"] = 	"Spear of Light I",
	["school"] = 	SCHOOL_OSPIRIT,
	["spell_power"] = 0,
	["level"] = 	10,
	["mana"] = 	5,
	["mana_max"] = 	20,
	["fail"] = 	-8,
	["direction"] = TRUE,
	["ftk"] = 1,
	["spell"] = 	function(args)
			fire_beam(Ind, GF_LITE, args.dir, damroll(get_litebeam_dam(Ind, 1)), " casts a spear of light for")
	end,
	["info"] = 	function()
			local x, y

			x, y = get_litebeam_dam(Ind, 1)
			return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Conjures up spiritual light into a powerful beam.", }
}
LITEBEAM_II = add_spell {
	["name"] = 	"Spear of Light II",
	["school"] = 	SCHOOL_OSPIRIT,
	["spell_power"] = 0,
	["level"] = 	25,
	["mana"] = 	10,
	["mana_max"] = 	10,
	["fail"] = 	-40,
	["direction"] = TRUE,
	["ftk"] = 1,
	["spell"] = 	function(args)
			fire_beam(Ind, GF_LITE, args.dir, damroll(get_litebeam_dam(Ind, 15)), " casts a spear of light for")
	end,
	["info"] = 	function()
			local x, y

			x, y = get_litebeam_dam(Ind, 15)
			return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Conjures up spiritual light into a powerful beam.", }
}
LITEBEAM_III = add_spell {
	["name"] = 	"Spear of Light III",
	["school"] = 	SCHOOL_OSPIRIT,
	["spell_power"] = 0,
	["level"] = 	40,
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	-75,
	["direction"] = TRUE,
	["ftk"] = 1,
	["spell"] = 	function(args)
			fire_beam(Ind, GF_LITE, args.dir, damroll(get_litebeam_dam(Ind, 0)), " casts a spear of light for")
	end,
	["info"] = 	function()
			local x, y

			x, y = get_litebeam_dam(Ind, 0)
			return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Conjures up spiritual light into a powerful beam.", }
}

function get_olightningbolt_dam(Ind, limit_lev)
	--return 3 + get_level(Ind, OLIGHTNINGBOLT, 25), 5 + get_level(Ind, OLIGHTNINGBOLT, 25) - 1
	local lev

	lev = get_level(Ind, OLIGHTNINGBOLT_I, 50)
	if limit_lev ~= 0 and lev > limit_lev then lev = limit_lev + (lev - limit_lev) / 3 end

	return 3 + (lev / 2), 5 + (lev / 2) - 1
end
OLIGHTNINGBOLT_I = add_spell {
	["name"] = 	"Lightning I",
	["school"] = 	SCHOOL_OSPIRIT,
	["spell_power"] = 0,
	["level"] = 	10,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	-10,
	["direction"] = TRUE,
	["ftk"] = 1,
	["spell"] = 	function(args)
		fire_bolt(Ind, GF_ELEC, args.dir, damroll(get_olightningbolt_dam(Ind, 1)), " casts a lighting bolt for")
	end,
	["info"] = 	function()
		local x, y

		x, y = get_olightningbolt_dam(Ind, 1)
		return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Conjures up spiritual power into a lightning bolt.", }
}
OLIGHTNINGBOLT_II = add_spell {
	["name"] = 	"Lightning II",
	["school"] = 	SCHOOL_OSPIRIT,
	["spell_power"] = 0,
	["level"] = 	25,
	["mana"] = 	6,
	["mana_max"] = 	6,
	["fail"] = 	-40,
	["direction"] = TRUE,
	["ftk"] = 1,
	["spell"] = 	function(args)
		fire_bolt(Ind, GF_ELEC, args.dir, damroll(get_olightningbolt_dam(Ind, 15)), " casts a lighting bolt for")
	end,
	["info"] = 	function()
		local x, y

		x, y = get_olightningbolt_dam(Ind, 15)
		return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Conjures up spiritual power into a lightning bolt.", }
}
OLIGHTNINGBOLT_III = add_spell {
	["name"] = 	"Lightning III",
	["school"] = 	SCHOOL_OSPIRIT,
	["spell_power"] = 0,
	["level"] = 	40,
	["mana"] = 	12,
	["mana_max"] = 	12,
	["fail"] = 	-80,
	["direction"] = TRUE,
	["ftk"] = 1,
	["spell"] = 	function(args)
		fire_bolt(Ind, GF_ELEC, args.dir, damroll(get_olightningbolt_dam(Ind, 0)), " casts a lighting bolt for")
	end,
	["info"] = 	function()
		local x, y

		x, y = get_olightningbolt_dam(Ind, 0)
		return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Conjures up spiritual power into a lightning bolt.", }
}

TRANCE = add_spell {
	["name"] = 	"Trance",
	["school"] = 	{SCHOOL_OSPIRIT},
	["am"] = 	33,
	["spell_power"] = 0,
	["level"] = 	5,
	["mana"] = 	13,
	["mana_max"] = 	13,
	["stat"] = 	A_WIS,
	["fail"] = 	10,
	["direction"] = FALSE,
	["spell"] = 	function(args)
		project_los(Ind, GF_OLD_SLEEP, 1024 + 5 + get_level(Ind, TRANCE, 80), "mumbles softly")
	end,
	["info"] = 	function()
		return "power "..(5 + get_level(Ind, TRANCE, 80))
	end,
	["desc"] = {
		"Causes all ghosts, spirits and elementals that see you",
		"to fall into a deep, spiritual sleep instantly.",
	}
}

--pet version
--[[POSSESS = add_spell {
	["name"] = 	"Possess",
	["school"] = 	{SCHOOL_OSPIRIT},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	23,
	["mana"] = 	10,
	["mana_max"] = 	10,
	["stat"] = 	A_WIS,
	["fail"] = 	-30,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		--reset previous charm spell first:
		do_ostoppossess(Ind)
		--cast charm!
		fire_bolt(Ind, GF_POSSESS, args.dir, 10 + get_level(Ind, POSSESS, 150), "focusses")
	end,
	["info"] = 	function()
--		return "power "..(10 + get_level(Ind, POSSESS, 150))
		return ""
	end,
	["desc"] =	{
		"Tries to manipulate the mind of an animal to become your pet.",
	}
}]]--
--placeholder version
POSSESS = add_spell {
	["name"] = 	"Possess",
	["school"] = 	{SCHOOL_OSPIRIT},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	23,
	["mana"] = 	10,
	["mana_max"] = 	10,
	["stat"] = 	A_WIS,
	["fail"] = 	-30,
	["direction"] = function () if get_level(Ind, POSSESS, 50) >= 17 then return FALSE else return TRUE end end,
	["spell"] = 	function(args)
		--reset previous charm spell first:
		do_mstopcharm(Ind)
		--cast charm!
		if get_level(Ind, POSSESS, 50) >= 17 then
			project_los(Ind, GF_CHARMIGNORE, 10 + get_level(Ind, POSSESS, 150), "focusses")
		elseif get_level(Ind, POSSESS, 50) >= 9 then
			fire_ball(Ind, GF_CHARMIGNORE, args.dir, 10 + get_level(Ind, POSSESS, 150), 3, "focusses")
		else
			fire_bolt(Ind, GF_CHARMIGNORE, args.dir, 10 + get_level(Ind, POSSESS, 150), "focusses")
		end
	end,
	["info"] = 	function()
--		return "power "..(10 + get_level(Ind, POSSESS, 150))
		return ""
	end,
	["desc"] =	{
		"Tries to manipulate the mind of a monster to make it ignore you.",
		"At level 9 it turns into a ball.",
		"At level 17 it affects all monsters in sight.",
	}
}

STOPPOSSESS = add_spell {
	["name"] = 	"Stop Possess",
	["school"] = 	{SCHOOL_OSPIRIT},
	["am"] = 	0,
	["spell_power"] = 0,
	["level"] = 	23,
	["mana"] = 	0,
	["mana_max"] = 	0,
	["stat"] = 	A_WIS,
	["fail"] = 	-99,
	["direction"] = FALSE,
	["spell"] = 	function()
		do_mstopcharm(Ind)
	end,
	["info"] = 	function()
		return ""
	end,
	["desc"] =	{ "Cancel charming of any monsters.", }
}
