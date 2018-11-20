-- handle the occultism school ('hereticism')
-- Note that this lua file requires a client update and the new s_aux.lua function 'find_spell_from_item()' to go with it,
-- to allow spells of identical names (Fire Bolt, Chaos Bolt and Hellfire).

TERROR_I = add_spell {
	["name"] = 	"Terror I",
	["name2"] = 	"Terror I",
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
	["name2"] = 	"Terror II",
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
	["name2"] = 	"IgnFear",
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

function get_firebolt2_dam(Ind, limit_lev)
	--return 5 + get_level(Ind, FIREBOLT, 25), 7 + get_level(Ind, FIREBOLT, 25) + 1
	local lev

	lev = get_level(Ind, OFIREBOLT_I, 50)
	if limit_lev ~= 0 and lev > limit_lev then lev = limit_lev + (lev - limit_lev) / 3 end

	return 5 + ((lev * 3) / 5), 7 + (lev / 2) + 1
end
OFIREBOLT_I = add_spell {
	["name"] = 	"Fire Bolt I",
	["name2"] = 	"FBolt I",
	["school"] = 	SCHOOL_OHERETICISM,
	["spell_power"] = 0,
	["level"] = 	10,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	-10,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			fire_bolt(Ind, GF_FIRE, args.dir, damroll(get_firebolt2_dam(Ind, 1)), " casts a fire bolt for")
	end,
	["info"] = 	function()
			local x, y

			x, y = get_firebolt2_dam(Ind, 1)
			return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Conjures up fire into a powerful bolt.", }
}
OFIREBOLT_II = add_spell {
	["name"] = 	"Fire Bolt II",
	["name2"] = 	"FBolt II",
	["school"] = 	SCHOOL_OHERETICISM,
	["spell_power"] = 0,
	["level"] = 	25,
	["mana"] = 	6,
	["mana_max"] = 	6,
	["fail"] = 	-30,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			fire_bolt(Ind, GF_FIRE, args.dir, damroll(get_firebolt2_dam(Ind, 15)), " casts a fire bolt for")
	end,
	["info"] = 	function()
			local x, y

			x, y = get_firebolt2_dam(Ind, 15)
			return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Conjures up fire into a powerful bolt.", }
}
OFIREBOLT_III = add_spell {
	["name"] = 	"Fire Bolt III",
	["name2"] = 	"FBolt III",
	["school"] = 	SCHOOL_OHERETICISM,
	["spell_power"] = 0,
	["level"] = 	40,
	["mana"] = 	12,
	["mana_max"] = 	12,
	["fail"] = 	-75,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			fire_bolt(Ind, GF_FIRE, args.dir, damroll(get_firebolt2_dam(Ind, 0)), " casts a fire bolt for")
	end,
	["info"] = 	function()
			local x, y

			x, y = get_firebolt2_dam(Ind, 0)
			return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Conjures up fire into a powerful bolt.", }
}

FIRERES = add_spell {
	["name"] = 	"Wrathflame",
	["name2"] = 	"Wrath",
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
		if get_level(Ind, FIRERES, 50) >= 17 then
			set_melee_brand(Ind, dur, TBRAND_HELLFIRE, 10)
		else
			set_melee_brand(Ind, dur, TBRAND_FIRE, 10)
		end
		if get_level(Ind, FIRERES, 50) >= 7 then
			set_oppose_fire(Ind, dur)
		end
	end,
	["info"] = 	function()
		return "dur "..(20 + get_level(Ind, FIRERES, 25)).."+d15"
	end,
	["desc"] = 	{
		"It temporarily brands your melee weapons with fire.",
		"At level 7 it grants temporary fire resistance.",
		"At level 17 the flame turns into hellfire instead.",
	}
}

OEXTRASTATS = add_spell {
	["name"] = 	"Demonic Strength",
	["name2"] = 	"Strength",
	["school"] = 	{SCHOOL_OHERETICISM},
	["spell_power"] = 0,
	["level"] = 	23,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	-30,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			--(lv=30 -> +4 at 50)
			do_xtra_stats(Ind, 4, 2 + get_level(Ind, OEXTRASTATS, 50) / 7, rand_int(7) + 22 + get_level(Ind, OEXTRASTATS, 17))
			end,
	["info"] = 	function()
			return "+" .. (2 + get_level(Ind, OEXTRASTATS, 50) / 7) .. " dur d7+" .. (22 + get_level(Ind, OEXTRASTATS, 17))
			end,
	["desc"] = 	{
			"Temporarily increases and sustains strength and constitution.",
			"Also grants hitpoint regeneration power.",
	}
}

function get_chaosbolt2_dam(Ind)
	local lev
	--must at same level have at least same damage as identically named CHAOSBOLT to make 'priority' work
	lev = get_level(Ind, CHAOSBOLT2, 50) + 21
	return 0 + (lev * 3) / 5 + 1, 1 + lev + 1
end
CHAOSBOLT2 = add_spell {
	["name"] = 	"Chaos Bolt",
	["name2"] = 	"CBolt",
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
	["name2"] = 	"Oath",
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
	["name2"] = 	"Sap",
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
	["name2"] = 	"Lev",
	["school"] = 	{SCHOOL_OHERETICISM},
	["spell_power"] = 0,
	["level"] = 	39,
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	-70,
	["spell"] = 	function()
			set_tim_lev(Ind, randint(10) + 10 + get_level(Ind, LEVITATION, 30))
	end,
	["info"] = 	function()
			return "dur "..(10 + get_level(Ind, LEVITATION, 30)).."+d10"
	end,
	["desc"] = 	{
			"Grants the power of levitation."
	}
}

FIRESTORM = add_spell {
	["name"] = 	"Robes of Havoc",
	["name2"] = 	"RoH",
	["school"] = 	{SCHOOL_OHERETICISM},
	["spell_power"] = 0,
	["level"] = 	44,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	-75,
	--["stat"] = 	A_WIS,
	["spell"] = 	function(args)
			fire_wave(Ind, GF_HELLFIRE, 0, 114 + get_level(Ind, FIRESTORM, 258), 1, 25 + get_level(Ind, FIRESTORM, 50), 8, EFF_STORM, " conjures hellfire for")
		end,
	["info"] = 	function()
			return "dam "..(114 + get_level(Ind, FIRESTORM, 258)).." rad 1 dur "..(25 + get_level(Ind, FIRESTORM, 50))
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
			fire_ball(Ind, GF_HELLFIRE, args.dir, 300 + get_level(Ind, HELLFIRE_III, 400), 2 + 1 + get_level(Ind, HELLFIRE_II, 2), " casts a ball of hellfire for")
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
	["name2"] = 	"BSac",
	["school"] = 	{SCHOOL_OHERETICISM},
	["am"] = 	75,
	["spell_power"] = 0,
	["level"] = 	47,
	["mana"] = 	50,
	["mana_max"] = 	50,
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

FLAMEWAVE_I = add_spell {
	["name"] = 	"Flame Wave I",
	["name2"] = 	"FWave I",
	["school"] = 	{SCHOOL_OHERETICISM},
	["spell_power"] = 0,
	["level"] = 	20,
	["mana"] = 	7,
	["mana_max"] = 	7,
	["fail"] = 	-35,
	["spell"] = 	function()
			fire_wave(Ind, GF_FIRE, 0, 30 + get_level(Ind, FLAMEWAVE_I, 120), 1, 6 + get_level(Ind, FLAMEWAVE_I, 5), 3, EFF_THINWAVE, " emits a flamewave for")
	end,
	["info"] = 	function()
			return "dam "..(30 + get_level(Ind, FLAMEWAVE_I,  120)).." rad "..(7 + get_level(Ind, FLAMEWAVE_I, 9))
	end,
	["desc"] = 	{ "Eradicates critters beneath your notice that dare trifle with you." }
}
FLAMEWAVE_II = add_spell {
	["name"] = 	"Flame Wave II",
	["name2"] = 	"FWave II",
	["school"] = 	{SCHOOL_OHERETICISM},
	["spell_power"] = 0,
	["level"] = 	35,
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	-75,
	["spell"] = 	function()
			fire_wave(Ind, GF_FIRE, 0, 30 + get_level(Ind, FLAMEWAVE_I, 400), 1, 6 + get_level(Ind, FLAMEWAVE_I, 5), 3, EFF_THINWAVE, " casts a flamewave for")
	end,
	["info"] = 	function()
			return "dam "..(30 + get_level(Ind, FLAMEWAVE_I, 400)).." rad "..(7 + get_level(Ind, FLAMEWAVE_I, 9))
	end,
	["desc"] = 	{ "Eradicates critters beneath your notice that dare trifle with you." }
}

RAGE_I = add_spell {
	["name"] = 	"Boundless Rage I",
	["name2"] = 	"Rage I",
	["school"] = 	SCHOOL_OHERETICISM,
	["spell_power"] = 0,
	["am"] = 	50,
	["level"] = 	29,
	["mana"] = 	40,
	["mana_max"] = 	40,
	["fail"] = 	-40,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
		set_zeal(Ind, 10, 9 + randint(5))
	end,
	["info"] = 	function()
			return "dur 9+d5, +1 BpR"
	end,
	["desc"] = 	{
		"Granting you an extra melee attack per round for 9+d5 turns.",
		"The duration of this spell can be prolonged by Traumaturgy.",
	}
}

RAGE_II = add_spell {
	["name"] = 	"Boundless Rage II",
	["name2"] = 	"Rage II",
	["school"] = 	SCHOOL_OHERETICISM,
	["spell_power"] = 0,
	["am"] = 	50,
	["level"] = 	41,
	["mana"] = 	75,
	["mana_max"] = 	75,
	["fail"] = 	-85,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
		set_zeal(Ind, 20, 9 + randint(5))
	end,
	["info"] = 	function()
			return "dur 9+d5, +2 BpR"
	end,
	["desc"] = 	{
		"Granting you two extra melee attacks per round for 9+d5 turns.",
		"The duration of this spell can be prolonged by Traumaturgy.",
	}
}
