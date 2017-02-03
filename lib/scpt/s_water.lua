-- handle the water school

function get_frostbolt_dam(Ind, limit_lev)
	--return 4 + get_level(Ind, FROSTBOLT, 25), 6 + get_level(Ind, FROSTBOLT, 25) + 0
	local lev

	lev = get_level(Ind, FROSTBOLT_I, 50)
	if limit_lev ~= 0 and lev > limit_lev then lev = limit_lev + (lev - limit_lev) / 3 end

	return 4 + ((lev * 3) / 5), 6 + (lev / 2) + 0
end
FROSTBOLT_I = add_spell {
	["name"] = 	"Frost Bolt I",
	["school"] = 	SCHOOL_WATER,
	["level"] = 	8,
	["mana"] = 	2,
	["mana_max"] = 	2,
	["fail"] = 	-10,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			fire_bolt(Ind, GF_COLD, args.dir, damroll(get_frostbolt_dam(Ind, 1)), " casts a frost bolt for")
		end,
	["info"] = 	function()
			local x, y

			x, y = get_frostbolt_dam(Ind, 1)
			return "dam "..x.."d"..y
		end,
	["desc"] = 	{ "Conjures up icy moisture into a powerful frost bolt.", }
}
FROSTBOLT_II = add_spell {
	["name"] = 	"Frost Bolt II",
	["school"] = 	SCHOOL_WATER,
	["level"] = 	22,
	["mana"] = 	5,
	["mana_max"] = 	5,
	["fail"] = 	-30,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			fire_bolt(Ind, GF_COLD, args.dir, damroll(get_frostbolt_dam(Ind, 14)), " casts a frost bolt for")
		end,
	["info"] = 	function()
			local x, y

			x, y = get_frostbolt_dam(Ind, 14)
			return "dam "..x.."d"..y
		end,
	["desc"] = 	{ "Conjures up icy moisture into a powerful frost bolt.", }
}
FROSTBOLT_III = add_spell {
	["name"] = 	"Frost Bolt III",
	["school"] = 	SCHOOL_WATER,
	["level"] = 	40,
	["mana"] = 	10,
	["mana_max"] = 	10,
	["fail"] = 	-75,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			fire_bolt(Ind, GF_COLD, args.dir, damroll(get_frostbolt_dam(Ind, 0)), " casts a frost bolt for")
		end,
	["info"] = 	function()
			local x, y

			x, y = get_frostbolt_dam(Ind, 0)
			return "dam "..x.."d"..y
		end,
	["desc"] = 	{ "Conjures up icy moisture into a powerful frost bolt.", }
}

function get_waterbolt_dam(Ind, limit_lev)
	--return 4 + get_level(Ind, WATERBOLT, 25), 6 + get_level(Ind, WATERBOLT, 25) + 0
	local lev

	lev = get_level(Ind, WATERBOLT_I, 50)
	if limit_lev ~= 0 and lev > limit_lev then lev = limit_lev + (lev - limit_lev) / 3 end

	return 4 + ((lev * 3) / 5), 6 + (lev / 2) + 0
end
WATERBOLT_I = add_spell {
	["name"] = 	"Water Bolt I",
	["school"] = 	SCHOOL_WATER,
	["level"] = 	14,
	["mana"] = 	4,
	["mana_max"] = 	4,
	["fail"] = 	-10,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			fire_bolt(Ind, GF_WATER, args.dir, damroll(get_waterbolt_dam(Ind, 1)), " casts a water bolt for")
		end,
	["info"] = 	function()
			local x, y

			x, y = get_waterbolt_dam(Ind, 1)
			return "dam "..x.."d"..y
		end,
	["desc"] = 	{ "Conjures up water into a powerful bolt.", }
}
WATERBOLT_II = add_spell {
	["name"] = 	"Water Bolt II",
	["school"] = 	SCHOOL_WATER,
	["level"] = 	24,
	["mana"] = 	8,
	["mana_max"] = 	8,
	["fail"] = 	-34,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			fire_bolt(Ind, GF_WATER, args.dir, damroll(get_waterbolt_dam(Ind, 10)), " casts a water bolt for")
		end,
	["info"] = 	function()
			local x, y

			x, y = get_waterbolt_dam(Ind, 10)
			return "dam "..x.."d"..y
		end,
	["desc"] = 	{ "Conjures up water into a powerful bolt.", }
}
WATERBOLT_III = add_spell {
	["name"] = 	"Water Bolt III",
	["school"] = 	SCHOOL_WATER,
	["level"] = 	40,
	["mana"] = 	17,
	["mana_max"] = 	17,
	["fail"] = 	-75,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			fire_bolt(Ind, GF_WATER, args.dir, damroll(get_waterbolt_dam(Ind, 0)), " casts a water bolt for")
		end,
	["info"] = 	function()
			local x, y

			x, y = get_waterbolt_dam(Ind, 0)
			return "dam "..x.."d"..y
		end,
	["desc"] = 	{ "Conjures up water into a powerful bolt.", }
}

TIDALWAVE_I = add_spell {
	["name"] = 	"Tidal Wave I",
	["school"] = 	{SCHOOL_WATER},
	["level"] = 	16,
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	-10,
	["spell"] = 	function()
			fire_wave(Ind, GF_WAVE, 0, 30 + get_level(Ind, TIDALWAVE_I, 80), 1, 6 + get_level(Ind, TIDALWAVE_I, 6), 5, EFF_WAVE, " casts a tidal wave for")
	end,
	["info"] = 	function()
			return "dam "..(30 + get_level(Ind, TIDALWAVE_I,  80)).." rad "..(6 + get_level(Ind, TIDALWAVE_I, 6))
	end,
	["desc"] = 	{
			"Summons a monstruous tidal wave that will expand and crush the",
			"monsters under it's mighty waves."
	}
}
TIDALWAVE_II = add_spell {
	["name"] = 	"Tidal Wave II",
	["school"] = 	{SCHOOL_WATER},
	["level"] = 	36,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	-60,
	["spell"] = 	function()
			fire_wave(Ind, GF_WAVE, 0, 40 + get_level(Ind, TIDALWAVE_I, 200), 1, 6 + get_level(Ind, TIDALWAVE_I, 6), 5, EFF_WAVE, " casts a tidal wave for")
	end,
	["info"] = 	function()
			return "dam "..(40 + get_level(Ind, TIDALWAVE_I,  200)).." rad "..(6 + get_level(Ind, TIDALWAVE_I, 6))
	end,
	["desc"] = 	{
			"Summons a monstruous tidal wave that will expand and crush the",
			"monsters under it's mighty waves."
	}
}

ICESTORM_I = add_spell {
	["name"] = 	"Frost Barrier I",
	["school"] = 	{SCHOOL_WATER},
	["level"] = 	22,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	20,
	["spell"] = 	function()
			fire_wave(Ind, GF_COLD, 0, 80 + get_level(Ind, ICESTORM_I, 200), 1, 20 + get_level(Ind, ICESTORM_I, 47), 9, EFF_STORM, " summons an ice storm for")
	end,
	["info"] = 	function()
			return "dam "..(80 + get_level(Ind, ICESTORM_I, 200)).." rad 1 dur "..(20 + get_level(Ind, ICESTORM_I, 47))
	end,
	["desc"] = 	{ "Engulfs you in a whirl of roaring cold that strikes all foes at close range.", }
}
ICESTORM_II = add_spell {
	["name"] = 	"Frost Barrier II",
	["school"] = 	{SCHOOL_WATER},
	["level"] = 	37,
	["mana"] = 	60,
	["mana_max"] = 	60,
	["fail"] = 	-50,
	["spell"] = 	function()
			fire_wave(Ind, GF_ICE, 0, 80 + get_level(Ind, ICESTORM_I, 200), 1, 20 + get_level(Ind, ICESTORM_I, 47), 9, EFF_STORM, " summons an ice storm for")
	end,
	["info"] = 	function()
			return "dam "..(80 + get_level(Ind, ICESTORM_I, 200)).." rad 1 dur "..(20 + get_level(Ind, ICESTORM_I, 47))
	end,
	["desc"] = 	{ "Engulfs you in a whirl of sparkling ice that strikes all foes at close range.", }
}

ENTPOTION = add_spell {
	["name"] = 	"Ent's Potion",
	["school"] = 	{SCHOOL_WATER},
	["level"] = 	6,
	["mana"] = 	10,
	["mana_max"] = 	10,
	["fail"] = 	20,
	["spell"] = 	function()
				fire_ball(Ind, GF_SATHUNGER_PLAYER, 0, 1, 2, "")
				--if player.suscep_life == false then
				if player.prace ~= RACE_VAMPIRE then
					set_food(Ind, PY_FOOD_MAX - 1)
					msg_print(Ind, "The Ent's Potion fills your stomach.")
				end
			if get_level(Ind, ENTPOTION, 50) >= 5 then
				fire_ball(Ind, GF_REMFEAR_PLAYER, 0, 1, 2, "")
				set_afraid(Ind, 0)
			end
			if get_level(Ind, ENTPOTION, 50) >= 12 then
				fire_ball(Ind, GF_HERO_PLAYER, 0, randint(25) + 25 + get_level(Ind, ENTPOTION, 40), 2, "")
				set_hero(Ind, randint(25) + 25 + get_level(Ind, ENTPOTION, 40))
			end
	end,
	["info"] = 	function()
			if get_level(Ind, ENTPOTION, 50) >= 12 then
				return "dur "..(25 + get_level(Ind, ENTPOTION, 40)).."+d25"
			else
				return ""
			end
	end,
	["desc"] = 	{
			"Fills up your stomach.",
			"At level 5 it boldens your heart.",
			"At level 12 it make you heroic.",
			"***Automatically projecting***",
	}
}

VAPOR_I = add_spell {
	["name"] = 	"Vapor I",
	["school"] = 	{SCHOOL_WATER},
	["level"] = 	2,
	["mana"] = 	2,
	["mana_max"] = 	12,
	["fail"] = 	20,
	["spell"] = 	function()
			fire_cloud(Ind, GF_VAPOUR, 0, 3 + get_level(Ind, VAPOR_I, 20), 3 + get_level(Ind, VAPOR_I, 4, 0), 5, 8, " fires a cloud of vapor for")
	end,
	["info"] = 	function()
			return "dam "..(3 + get_level(Ind, VAPOR_I, 20)).." rad "..(3 + get_level(Ind, VAPOR_I, 4, 0)).." dur 5"
	end,
	["desc"] = 	{ "Fills the air with toxic moisture to eradicate annoying critters." }
}
VAPOR_II = add_spell {
	["name"] = 	"Vapor II",
	["school"] = 	{SCHOOL_WATER},
	["level"] = 	20,
	["mana"] = 	5,
	["mana_max"] = 	5,
	["fail"] = 	-20,
	["spell"] = 	function()
			fire_cloud(Ind, GF_VAPOUR, 0, 3 + 20 + get_level(Ind, VAPOR_II, 20), 3 + get_level(Ind, VAPOR_I, 4, 0), 5, 8, " fires a cloud of vapor for")
	end,
	["info"] = 	function()
			return "dam "..(3 + 20 + get_level(Ind, VAPOR_II, 20)).." rad "..(3 + get_level(Ind, VAPOR_I, 4, 0)).." dur 5"
	end,
	["desc"] = 	{ "Fills the air with toxic moisture to eradicate annoying critters." }
}
VAPOR_III = add_spell {
	["name"] = 	"Vapor III",
	["school"] = 	{SCHOOL_WATER},
	["level"] = 	40,
	["mana"] = 	12,
	["mana_max"] = 	12,
	["fail"] = 	-75,
	["spell"] = 	function()
			fire_cloud(Ind, GF_VAPOUR, 0, 3 + 40 + get_level(Ind, VAPOR_III, 20), 3 + get_level(Ind, VAPOR_I, 4, 0), 5, 8, " fires a cloud of vapor for")
	end,
	["info"] = 	function()
			return "dam "..(3 + 40 + get_level(Ind, VAPOR_III, 20)).." rad "..(3 + get_level(Ind, VAPOR_I, 4, 0)).." dur 5"
	end,
	["desc"] = 	{ "Fills the air with toxic moisture to eradicate annoying critters." }
}

FROSTBALL_I = add_spell {
	["name"] = 	"Frost Ball I",
	["school"] = 	{SCHOOL_WATER},
	["level"] = 	22,
	["mana"] = 	9,
	["mana_max"] = 	9,
	["fail"] = 	-25,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			fire_ball(Ind, GF_COLD, args.dir, 90 + get_level(Ind, FROSTBALL_I, 490), 2 + get_level(Ind, FROSTBALL_I, 3), " casts a frost ball for")
	end,
	["info"] = 	function()
			return "dam "..(90 + get_level(Ind, FROSTBALL_I, 490)).." rad "..(2 + get_level(Ind, FROSTBALL_I, 3))
	end,
	["desc"] = 	{ "Conjures a ball of frost to shatter your foes to pieces." }
}
FROSTBALL_II = add_spell {
	["name"] = 	"Frost Ball II",
	["school"] = 	{SCHOOL_WATER},
	["level"] = 	40,
	["mana"] = 	23,
	["mana_max"] = 	23,
	["fail"] = 	-90,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			fire_ball(Ind, GF_COLD, args.dir, 160 + get_level(Ind, FROSTBALL_I, 780), 2 + get_level(Ind, FROSTBALL_I, 3), " casts a frost ball for")
	end,
	["info"] = 	function()
			return "dam "..(160 + get_level(Ind, FROSTBALL_I, 780)).." rad "..(2 + get_level(Ind, FROSTBALL_I, 3))
	end,
	["desc"] = 	{ "Conjures a ball of frost to shatter your foes to pieces." }
}
