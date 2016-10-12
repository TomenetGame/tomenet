-- The earth school

--[[
STONESKIN = add_spell {
	["name"] = 	"Stone Skin",
	["school"] = 	SCHOOL_EARTH,
	["level"] = 	1,
	["mana"] = 	1,
	["mana_max"] = 	100,
	["fail"] = 	10,
	["spell"] = 	function()
			local type
			if get_level(Ind, STONESKIN, 50) >= 25 then
				type = SHIELD_COUNTER
			else
				type = 0
			end
			set_shield(Ind, randint(10) + 10 + get_level(Ind, STONESKIN, 100), 5 + get_level(Ind, STONESKIN, 30), type, 2 + get_level(Ind, STONESKIN, 5), 3 + get_level(Ind, STONESKIN, 5))
	end,
	["info"] = 	function()
			if get_level(Ind, STONESKIN, 50) >= 25 then
				return "dam "..(2 + get_level(Ind, STONESKIN, 5)).."d"..(3 + get_level(Ind, STONESKIN, 5)).." dur "..(10 + get_level(Ind, STONESKIN, 100)).."+d10 AC "..(5 + get_level(Ind, STONESKIN, 30))
			else
				return "dur "..(10 + get_level(Ind, STONESKIN, 100)).."+d10 AC "..(5 + get_level(Ind, STONESKIN, 30))
			end
	end,
	["desc"] = 	{
			"Creates a shield of earth around you to protect you",
			"At level 25 it starts dealing damage to attackers"
		}
}
]]
DIG = add_spell {
	["name"] = 	"Dig",
	["school"] = 	SCHOOL_EARTH,
	["level"] = 	12,
	["mana"] = 	14,
	["mana_max"] = 	14,
	["fail"] = 	10,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			wall_to_mud(Ind, args.dir)
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{ "Digs a hole in a wall much faster than any shovel.", }
}

function get_acidbolt_dam(Ind, limit_lev)
	--return 6 + get_level(Ind, ACIDBOLT, 25), 8 + get_level(Ind, ACIDBOLT, 25) + 1
	local lev

	lev = get_level(Ind, ACIDBOLT_I, 50)
	if limit_lev ~= 0 and lev > limit_lev then lev = limit_lev + (lev - limit_lev) / 3 end

	return 6 + ((lev * 3) / 5), 8 + (lev / 2) + 1
end
ACIDBOLT_I = add_spell {
	["name"] = 	"Acid Bolt I",
	["school"] = 	SCHOOL_EARTH,
	["level"] = 	12,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	-10,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			fire_bolt(Ind, GF_ACID, args.dir, damroll(get_acidbolt_dam(Ind, 1)), " casts a acid bolt for")
	end,
	["info"] = 	function()
			local x, y

			x, y = get_acidbolt_dam(Ind, 1)
			return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Conjures up corroding acid into a powerful bolt.", }
}
ACIDBOLT_II = add_spell {
	["name"] = 	"Acid Bolt II",
	["school"] = 	SCHOOL_EARTH,
	["level"] = 	24,
	["mana"] = 	7,
	["mana_max"] = 	7,
	["fail"] = 	-30,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			fire_bolt(Ind, GF_ACID, args.dir, damroll(get_acidbolt_dam(Ind, 12)), " casts a acid bolt for")
	end,
	["info"] = 	function()
			local x, y

			x, y = get_acidbolt_dam(Ind, 12)
			return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Conjures up corroding acid into a powerful bolt.", }
}
ACIDBOLT_III = add_spell {
	["name"] = 	"Acid Bolt III",
	["school"] = 	SCHOOL_EARTH,
	["level"] = 	40,
	["mana"] = 	13,
	["mana_max"] = 	13,
	["fail"] = 	-75,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			fire_bolt(Ind, GF_ACID, args.dir, damroll(get_acidbolt_dam(Ind, 0)), " casts a acid bolt for")
	end,
	["info"] = 	function()
			local x, y

			x, y = get_acidbolt_dam(Ind, 0)
			return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Conjures up corroding acid into a powerful bolt.", }
}

STONEPRISON = add_spell {
	["name"] = 	"Stone Prison",
	["school"] = 	SCHOOL_EARTH,
	["level"] = 	25,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	10,
	["spell"] = 	function()
			local ret, x, y
			fire_ball(Ind, GF_STONE_WALL, 0, 1, 1, "")
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{ "Creates a prison of walls around you." }
}

STRIKE_I = add_spell {
	["name"] = 	"Strike I",
	["school"] = 	{SCHOOL_EARTH},
	["level"] = 	25,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	10,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			fire_ball(Ind, GF_FORCE, args.dir, 50 + get_level(Ind, STRIKE_I, 50), 0, " casts a force bolt of")
	end,
	["info"] = 	function()
			return "dam "..(50 + get_level(Ind, STRIKE_I, 50))
	end,
	["desc"] = 	{ "Creates a force bolt that may stun enemies.", }
}
STRIKE_II = add_spell {
	["name"] = 	"Strike II",
	["school"] = 	{SCHOOL_EARTH},
	["level"] = 	37,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	-50,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			fire_ball(Ind, GF_FORCE, args.dir, 50 + get_level(Ind, STRIKE_I, 50), 1, " casts a force ball of")
	end,
	["info"] = 	function()
			return "dam "..(50 + get_level(Ind, STRIKE_I, 50)).." rad 1"
	end,
	["desc"] = 	{ "Creates a small force ball that may stun enemies.", }
}

SHAKE = add_spell {
	["name"] = 	"Shake",
	["school"] = 	{SCHOOL_EARTH},
	["level"] = 	35,
	["mana"] = 	60,
	["mana_max"] = 	60,
	["fail"] = 	15,
	["spell"] = 	function()
			earthquake(player.wpos, player.py, player.px, 2 + get_level(Ind, SHAKE, 17));
	end,
	["info"] = 	function()
			return "rad "..(2 + get_level(Ind, SHAKE, 17))
	end,
	["desc"] = 	{ "Creates a localized earthquake." }
}
