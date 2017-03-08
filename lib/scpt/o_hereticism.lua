-- handle the occultism school ('hereticism')
-- Note that this lua file requires a client update and the new s_aux.lua function 'find_spell_from_item()' to go with it,
-- to allow spells of identical names (Fire Bolt, Chaos Bolt and Hellfire).

TERROR_I = add_spell {
	["name"] = 	"Terror I",
	["school"] = 	{SCHOOL_OHERETICISM},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	3,
	["mana"] = 	5,
	["mana_max"] = 	5,
	["fail"] = 	10,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["spell"] = 	function(args)
				fire_grid_bolt(Ind, GF_TERROR, args.dir, 5 + get_level(Ind, TERROR_I, 100), " gazes at you")
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
	["mana"] = 	16,
	["mana_max"] = 	16,
	["fail"] = 	-20,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
				fire_ball(Ind, GF_TERROR, 0, 5 + get_level(Ind, TERROR_I, 100), 1, " screams disturbingly")
			end,
	["info"] = 	function()
				return "power "..(5 + get_level(Ind, TERROR_I, 100))
			end,
	["desc"] = 	{ "Invades the minds of all adjacent enemies, confusing and scaring them.", }
}

ODELFEAR2 = add_spell {
	["name"] = 	"Ignore Fear",
	["school"] = 	{SCHOOL_OHERETICISM},
	["spell_power"] = 0,
	["am"] = 	0,
	["level"] = 	7,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	10,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			set_afraid(Ind, 0)
			set_res_fear(Ind, get_level(Ind, ODELFEAR2, 30))
			end,
	["info"] = 	function()
			return "dur "..get_level(Ind, ODELFEAR2, 30)
			end,
	["desc"] = 	{ "Removes the ignorant weakness that is fear, for a while.", }
}

OFIREBOLT_I = add_spell {
	["name"] = 	"Fire Bolt I",
	["school"] = 	SCHOOL_OHERETICISM,
	["spell_power"] = 0,
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
OFIREBOLT_II = add_spell {
	["name"] = 	"Fire Bolt II",
	["school"] = 	SCHOOL_OHERETICISM,
	["spell_power"] = 0,
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
OFIREBOLT_III = add_spell {
	["name"] = 	"Fire Bolt III",
	["school"] = 	SCHOOL_OHERETICISM,
	["spell_power"] = 0,
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
	["mana_max"] = 	15,
	["fail"] = 	0,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
		local dur
		dur = randint(15) + 20 + get_level(Ind, FIRERES, 25)
		set_brand(Ind, dur, TBRAND_FIRE, 10)
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

OEXTRASTATS = add_spell {
	["name"] = 	"Demonic Strength",
	["school"] = 	{SCHOOL_OHERETICISM},
	["spell_power"] = 0,
	["level"] = 	30,
	["mana"] = 	40,
	["mana_max"] = 	40,
	["fail"] = 	-30,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			do_xtra_stats(Ind, 4, 1 + get_level(Ind, OEXTRASTATS, 50) / 7, rand_int(7) + 22 + get_level(Ind, OEXTRASTATS, 30))
			end,
	["info"] = 	function()
			return "+" .. (1 + get_level(Ind, OEXTRASTATS, 50) / 7) .. " dur d7+" .. (22 + get_level(Ind, OEXTRASTATS, 30))
			end,
	["desc"] = 	{ "Temporarily increases and sustains strength and constitution.", }
}

function get_chaosbolt2_dam(Ind)
	local lev
	--must at same level have at least same damage as identically named CHAOSBOLT to make 'priority' work
	lev = get_level(Ind, CHAOSBOLT2, 50) + 21
	return 0 + (lev * 3) / 5 + 1, 1 + lev + 1
end
CHAOSBOLT2 = add_spell {
	["name"] = 	"Chaos Bolt",
	["school"] = 	{SCHOOL_OHERETICISM},
	["spell_power"] = 0,
	["level"] = 	29,
	["priority"] = 	1, --vs CHAOSBOLT: actually same damage at same level, but lower mana cost
	["mana"] = 	16,
	["mana_max"] = 	16,
	["fail"] = 	-55,
	["direction"] = TRUE,
	["stat"] = 	A_WIS,
	["ftk"] = 	1,
	["spell"] = 	function(args)
		fire_bolt(Ind, GF_CHAOS, args.dir, damroll(get_chaosbolt2_dam(Ind)), " casts a chaos bolt for")
	end,
	["info"] = 	function()
		local x, y

		x, y = get_chaosbolt2_dam(Ind)
		return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Channels the powers of chaos into a bolt.", }
}

ORESTORING = add_spell {
	["name"] = 	"Wicked Oath",
	["school"] = 	{SCHOOL_OHERETICISM},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	35,
	["mana"] = 	25,
	["mana_max"] = 	25,
	["fail"] = 	-35,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			do_res_stat(Ind, A_STR)
			do_res_stat(Ind, A_CON)
			do_res_stat(Ind, A_DEX)
			do_res_stat(Ind, A_WIS)
			do_res_stat(Ind, A_INT)
			do_res_stat(Ind, A_CHR)
			restore_level(Ind)
			end,
	["info"] = 	function()
			return ""
			end,
	["desc"] = 	{ "Restores drained stats and lost experience.", }
}

--[[SAPLIFE = add_spell {
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
}]]--

LEVITATION = add_spell {
	["name"] = 	"Levitation",
	["school"] = 	{SCHOOL_OHERETICISM},
	["spell_power"] = 0,
	["level"] = 	39,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	70,
	["spell"] = 	function()
			set_tim_lev(Ind, randint(10) + 5 + get_level(Ind, LEVITATION, 25))
	end,
	["info"] = 	function()
			return "dur "..(5 + get_level(Ind, LEVITATION, 25)).."+d10"
	end,
	["desc"] = 	{
			"Grants the power of levitation."
	}
}

FIRESTORM = add_spell {
	["name"] = 	"Robes of Havoc",
	["school"] = 	{SCHOOL_OHERETICISM},
	["spell_power"] = 0,
	["level"] = 	42,
	["mana"] = 	25,
	["mana_max"] = 	25,
	["fail"] = 	-75,
--	["stat"] = 	A_WIS,
	["spell"] = 	function(args)
			fire_wave(Ind, GF_HELL_FIRE, 0, 80 + get_level(Ind, FIRESTORM, 200), 1, 25 + get_level(Ind, FIRESTORM, 47), 9, EFF_STORM, " conjures hellfire for")
		end,
	["info"] = 	function()
			return "dam "..(80 + get_level(Ind, FIRESTORM, 200)).." rad 1 dur "..(25 + get_level(Ind, FIRESTORM, 47))
		end,
	["desc"] = 	{ "Envelops you in hellfire, burning your opponents to ashes.", }
}

--[[
HELLFIRE_III = add_spell {
	["name"] = 	"Hellfire III",
	["school"] = 	{SCHOOL_OHERETICISM},
	["spell_power"] = 0,
	["level"] = 	42,
	["mana"] = 	25,
	["mana_max"] = 	25,
	["fail"] = 	-75,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			fire_ball(Ind, GF_HELL_FIRE, args.dir, 300 + get_level(Ind, HELLFIRE_III, 400), 2 + 1 + get_level(Ind, HELLFIRE_II, 2), " casts a ball of hellfire for")
		end,
	["info"] = 	function()
			return "dam "..(300 + get_level(Ind, HELLFIRE_II, 400)).." rad "..(2 + 1 + get_level(Ind, HELLFIRE_II, 2))
		end,
	["desc"] = 	{ "Conjures a ball of hellfire to burn your foes to ashes.", }
}
]]--

--martyr's twin
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
			--abuse martyr vars for this =p (no race/class can learn both spells)
			if player.martyr_timeout > 0 then
				msg_print(Ind, "\255yThe maelstrom of chaos doesn't favour your blood sacrifice yet.")
			else
				player.martyr_timeout = 1000
				msg_print(Ind, "You feel the warped powers of chaos possess your body and mind..")
				do_mimic_change(Ind, 758, TRUE); --RI_BLOODTHIRSTER
				player.tim_mimic_what = 758; --RI_BLOODTHIRSTER
				player.tim_mimic = randint(15) + 50 + get_level(Ind, BLOODSACRIFICE, 30);
			end
	end,
	["info"] = 	function()
			return "dur "..(50 + get_level(Ind, BLOODSACRIFICE, 30)).."+d15"
	end,
	["desc"] = 	{ "Inflict a mortal wound on yourself, causing the warped powers of chaos",
			  "to temporarily change your form into a terrifying Bloodthirster.",
			  "--- This spell is only usable by Maia Priests and Hell Knights. ---", }
}
