-- handle the occultism school ('hereticism')
-- Note that this lua file requires a client update and the new s_aux.lua function 'find_spell_from_item()' to go with it,
-- to allow spells of identical names (Fire Bolt, Chaos Bolt and Hellfire).

TERROR_I = add_spell {
	["name"] = 	"Terror I",
	["school"] = 	{SCHOOL_OHERETICISM},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	3,
	["mana"] = 	4,
	["mana_max"] = 	4,
	["fail"] = 	10,
	["direction"] = TRUE,
	["spell"] = 	function(args)
				fire_grid_bolt(Ind, GF_TERROR, args.dir, 5 + get_level(Ind, TERROR_I, 100), "focusses on your mind")
			end,
	["info"] = 	function()
				return "power "..(5 + get_level(Ind, TERROR_I, 100))
			end,
	["desc"] = 	{ "Invades the mind of your target, confusing and scaring it.", }
}
TERROR_II = add_spell {
	["name"] = 	"Terror II",
	["school"] = 	{SCHOOL_OHERETICISM},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	18,
	["mana"] = 	14,
	["mana_max"] = 	14,
	["fail"] = 	-20,
	["direction"] = TRUE,
	["spell"] = 	function(args)
				fire_ball(Ind, GF_TERROR, 0, 5 + get_level(Ind, TERROR_I, 100), 1, " screams disturbingly")
			end,
	["info"] = 	function()
				return "power "..(5 + get_level(Ind, TERROR_I, 100))
			end,
	["desc"] = 	{ "Invades the minds of all adjacent enemies, confusing and scaring them.", }
}

FIREBOLT_I = add_spell {
	["name"] = 	"Fire Bolt I",
	["school"] = 	SCHOOL_OHERETICISM,
	["level"] = 	10,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	-10,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			fire_bolt(Ind, GF_FIRE, args.dir, damroll(get_firebolt_dam(Ind, 1)), " casts a fire bolt for")
	end,
	["info"] = 	function()
			local x, y

			x, y = get_firebolt_dam(Ind, 1)
			return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Conjures up fire into a powerful bolt.", }
}
FIREBOLT_II = add_spell {
	["name"] = 	"Fire Bolt II",
	["school"] = 	SCHOOL_OHERETICISM,
	["level"] = 	25,
	["mana"] = 	6,
	["mana_max"] = 	6,
	["fail"] = 	-30,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			fire_bolt(Ind, GF_FIRE, args.dir, damroll(get_firebolt_dam(Ind, 15)), " casts a fire bolt for")
	end,
	["info"] = 	function()
			local x, y

			x, y = get_firebolt_dam(Ind, 15)
			return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Conjures up fire into a powerful bolt.", }
}
FIREBOLT_III = add_spell {
	["name"] = 	"Fire Bolt III",
	["school"] = 	SCHOOL_OHERETICISM,
	["level"] = 	40,
	["mana"] = 	12,
	["mana_max"] = 	12,
	["fail"] = 	-75,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			fire_bolt(Ind, GF_FIRE, args.dir, damroll(get_firebolt_dam(Ind, 0)), " casts a fire bolt for")
	end,
	["info"] = 	function()
			local x, y

			x, y = get_firebolt_dam(Ind, 0)
			return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Conjures up fire into a powerful bolt.", }
}

FIRERES = add_spell {
	["name"] = 	"Wrathflame",
	["school"] = 	{SCHOOL_OHERETICISM},
	["spell_power"] = 0,
	["level"] = 	12,
	["mana"] = 	15,
	["mana_max"] = >15,
	["fail"] = 	0,
	["spell"] = 	function()
		local dur
		dur = randint(15) + 20 + get_level(Ind, FIRERES, 25)
		set_brand(Ind, dur, BRAND_FIRE, 10)
		if get_level(Ind, FIRERES, 50) >= 7 then
			set_oppose_fire(Ind, dur)
		end
	end,
	["info"] = 	function()
		return "dur "..(20 + get_level(Ind, FIRERES, 25)).."+d15"
	end,
	["desc"] = 	{
		"It temporarily brands your weapons with unholy fire.",
		"At level 7 it grants temporary fire resistance.",
	}
}

HCHAOSBOLT = add_spell {
	["name"] = 	"Chaos Bolt",
	["school"] = 	{SCHOOL_OHERETICISM},
	["spell_power"] = 0,
	["level"] = 	30,
	["mana"] = 	18,
	["mana_max"] = 	18,
	["fail"] = 	-55,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
		local x, y
		x, y = get_chaosbolt_dam(Ind)
		x = x + 2
		y = y + 2
		fire_bolt(Ind, GF_CHAOS, args.dir, damroll(x, y), " casts a chaos bolt for")
	end,
	["info"] = 	function()
		local x, y
		x, y = get_chaosbolt_dam(Ind)
		x = x + 2
		y = y + 2
		return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Channels the powers of chaos into a bolt.", }
}

SAPLIFE = add_spell {
	["name"] = 	"Sap Life",
	["school"] = 	{SCHOOL_OHERETICISM},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	37,
	["mana"] = 	45,
	["mana_max"] = 	45,
	["fail"] = 	-60,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		drain_life(Ind, args.dir, 14 + get_level(Ind, ODRAINLIFE, 22))
		hp_player(Ind, player.ret_dam / 4)
	end,
	["info"] = 	function()
		--return "drain "..(14 + get_level(Ind, ODRAINLIFE, 22)).."%, heal for 25%"
		return (14 + get_level(Ind, ODRAINLIFE, 22)).."% (max 900), 25% heal"
	end,
	["desc"] = 	{ "Drains life from a target, which must not be non-living or undead.", }
}

HELLFIRE_II = add_spell {
	["name"] = 	"Hellfire II",
	["school"] = 	{SCHOOL_OHERETICISM},
	["level"] = 	40,
	["mana"] = 	40,
	["mana_max"] = 	40,
	["fail"] = 	-75,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			fire_ball(Ind, GF_HELL_FIRE, args.dir, 20 + 250 + get_level(Ind, HELLFIRE_II, 250), 2 + 1 + get_level(Ind, HELLFIRE_II, 2), " casts a ball of hellfire for")
		end,
	["info"] = 	function()
			return "dam "..(20 + 250 + get_level(Ind, HELLFIRE_II, 250)).." rad "..(2 + 1 + get_level(Ind, HELLFIRE_II, 2))
		end,
	["desc"] = 	{ "Conjures a ball of hellfire to burn your foes to ashes.", }
}

--similar to martyr, duration-wise
BLOODSACRIFICE = add_spell {
	["name"] = 	"Blood Sacrifice",
	["school"] = 	{SCHOOL_OHERETICISM},
	["am"] = 	75,
	["spell_power"] = 0,
	["level"] = 	45,
	["mana"] = 	70,
	["mana_max"] = 	70,
	["fail"] = 	-60,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			p_ptr->martyr_timeout = 1000 --abuse martyr for this =p
			msg_print(Ind, "You feel the warped powers of chaos possess your body and mind!")
			set_mimic(randint(15) + 50 + get_level(BLOODSACRIFICE, 30), 758)
	end,
	["info"] = 	function()
			return "dur "..(50 + get_level(BLOODSACRIFICE, 30)).."+d15"
	end,
	["desc"] = 	{ "Inflict a mortal wound on yourself, causing the warped powers",
			  "of chaos to temporarily change your form into a greater demon.", }
}
