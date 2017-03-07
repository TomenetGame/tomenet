-- handle the psycho-power school
--((static: +saving throw))

function get_pyro_dam(Ind, limit_lev)
	local lev, llev, lev2
	lev = get_level(Ind, MPYROKINESIS_I, 700)
	llev = limit_lev * 14
	if limit_lev ~= 0 and lev > llev then lev = llev + (lev - llev) / 2 end
	lev2 = 200
	--if limit_lev ~= 0 and lev2 - 100 > limit_lev * 2 then lev2 = 100 + limit_lev * 2 + (lev2 - 100 - limit_lev * 2) / 2 end
	if limit_lev ~= 0 and lev2 > limit_lev * 4 then lev2 = limit_lev * 4 + (lev2 - limit_lev * 4) / 2 end
	return lev, lev2
end
function get_cryo_dam(Ind, limit_lev)
	local lev, llev, lev2
	lev = get_level(Ind, MCRYOKINESIS_I, 750)
	llev = limit_lev * 15
	if limit_lev ~= 0 and lev > llev then lev = llev + (lev - llev) / 2 end
	lev2 = 150
	--if limit_lev ~= 0 and lev2 - 100 > limit_lev * 2 then lev2 = 100 + limit_lev * 2 + (lev2 - 100 - limit_lev * 2) / 2 end
	if limit_lev ~= 0 and lev2 > limit_lev * 4 then lev2 = limit_lev * 4 + (lev2 - limit_lev * 4) / 2 end
	return lev, lev2
end

MBASH = add_spell {
	["name"] = 	"Psychic Hammer",
	["school"] = 	{SCHOOL_PPOWER},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	1,
	["mana"] = 	8,
	["mana_max"] = 	8,
	["fail"] = 	5,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			fire_grid_bolt(Ind, GF_FORCE, args.dir, get_level(Ind, MBASH, 50), " releases a psychic hammer for")
			end,
	["info"] = 	function()
			return "dam "..(get_level(Ind, MBASH, 50))
			end,
	["desc"] = 	{ "Telekinetic power focussed to bash your opponent.", }
}

MBLINK = add_spell {
	["name"] = 	"Autokinesis I",
	["school"] = 	{SCHOOL_PPOWER},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	5,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	10,
	["spell"] = 	function()
			local dist = 6 + get_level(Ind, MBLINK, 6)
			teleport_player(Ind, dist, TRUE)
			end,
	["info"] = 	function()
			return "distance "..(6 + get_level(Ind, MBLINK, 6))
			end,
	["desc"] = 	{ "Teleports you on a small scale range.", }
}

MTELEPORT = add_spell {
	["name"] = 	"Autokinesis II",
	["school"] = 	{SCHOOL_PPOWER},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	16,
	["mana"] = 	12,
	["mana_max"] = 	12,
	["fail"] = 	20,
	["spell"] = 	function()
			local dist = 100 + get_level(Ind, MTELEPORT, 100)
			teleport_player(Ind, dist, FALSE)
			end,
	["info"] = 	function()
			return ""
			end,
	["desc"] = 	{ "Teleports you around the level.", }
}

MTELETOWARDS = add_spell {
	["name"] = 	"Autokinesis III",
	["school"] = 	{SCHOOL_PPOWER},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	24,
	["mana"] = 	26,
	["mana_max"] = 	26,
	["fail"] = 	10,
	["spell"] = 	function()
			do_autokinesis_to(Ind, 20 + get_level(Ind, MTELETOWARDS, 150))
			end,
	["info"] = 	function()
			return "range "..(20 + get_level(Ind, MTELETOWARDS, 150))
			end,
	["desc"] = 	{ "Teleports you to the nearest friendly opened mind.", }
}

MTELEAWAY = add_spell {
	["name"] = 	"Psychic Warp",
	["school"] = 	{SCHOOL_PPOWER},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	30,
	["mana"] = 	45,
	["mana_max"] = 	45,
	["fail"] = 	10,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			fire_grid_bolt(Ind, GF_AWAY_ALL, args.dir, 40 + get_level(Ind, MTELEAWAY, 160), " teleports you away")
			end,
	["info"] = 	function()
			return ""
			end,
	["desc"] = 	{ "Attempts to teleport your opponent away.", }
}

MDISARM = add_spell {
	["name"] = 	"Psychokinesis",
	["school"] = 	{SCHOOL_PPOWER},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	3,
	["mana"] = 	5,
	["mana_max"] = 	5,
	["fail"] = 	10,
	["direction"] = TRUE,
	["spell"] = 	function(args)
--			destroy_traps_touch(Ind, 1)
			fire_grid_beam(Ind, GF_KILL_TRAP, args.dir, 0, "")
			end,
	["info"] = 	function()
			return ""
			end,
	["desc"] = 	{
			"Destroys traps and locks by psychokinetic manipulation,",
			"also revealing secret doors in the process.",
	}
}

--[[ --moved to mintrusion, to make room for ff/lev spells here..
MPSISTORM = add_spell {
	["name"] = 	"Psi Storm",
	["school"] = 	{SCHOOL_PPOWER},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	18,
	["mana"] = 	15,
	["mana_max"] = 	40,
	["fail"] = 	25,
	["direction"] = TRUE,
	["spell"] = 	function(args)
--	["spell"] = 	function()
			fire_cloud(Ind, GF_PSI, args.dir, (1 + get_level(Ind, MPSISTORM, 76)), 2 + get_level(Ind, MPSISTORM, 2), 5 + get_level(Ind, MPSISTORM, 5), 14, " releases a psi storm for")
--			fire_cloud(Ind, GF_PSI, 0, (1 + get_level(Ind, MPSISTORM, 76)), 2 + get_level(Ind, MPSISTORM, 4), 5 + get_level(Ind, MPSISTORM, 5), 14, " releases a psi storm for")
--			fire_wave(Ind, GF_PSI, 0, (1 + get_level(Ind, MPSISTORM, 76)), 2 + get_level(Ind, MPSISTORM, 4), 5 + get_level(Ind, MPSISTORM, 5), 14, EFF_STORM, " releases a psi storm for")
			end,
	["info"] = 	function()
			return "dam "..(1 + get_level(Ind, MPSISTORM, 76)).." rad "..(2 + get_level(Ind, MPSISTORM, 2)).." dur "..(5 + get_level(Ind, MPSISTORM, 5))
			end,
	["desc"] = 	{ "A psionic storm that damages and disturbs all minds within an area", }
}
]]

MFEEDBACK = add_spell {
	["name"] = 	"Feedback",
	["school"] = 	{SCHOOL_PPOWER},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	18,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	15,
	["direction"] = FALSE,
	["spell"] = function()
		if get_level(Ind, MFEEDBACK, 50) >= 15 then set_tim_lev(Ind, randint(5) + 5 + get_level(Ind, MFEEDBACK, 15))
			else set_tim_ffall(Ind, randint(5) + 5 + get_level(Ind, MFEEDBACK, 15))
		end
	end,
	["info"] = function()
		return "dur "..(5 + get_level(Ind, MFEEDBACK, 15)).."+d5"
	end,
	["desc"] = {
		"Uses psycho-kinetic force for propulsion, making you fall",
		"gently like a feather.",
		"At level 15 it becomes sufficient for levitation."
	}
}

MPYROKINESIS_I = add_spell {
	["name"] = 	"Pyrokinesis I",
	["school"] = 	{SCHOOL_PPOWER},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	20,
	["mana"] = 	9,
	["mana_max"] = 	9,
	["fail"] = 	-25,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			local n, p
			n, p = get_pyro_dam(Ind, 1)
			fire_grid_bolt(Ind, GF_FIRE, args.dir, n + p, " causes an inflammation for")
			end,
	["info"] = 	function()
			n, p = get_pyro_dam(Ind, 1)
			return "dam "..(n + p)
			end,
	["desc"] = 	{ "Causes a severe inflammation to burn your opponent.", }
}
MPYROKINESIS_II = add_spell {
	["name"] = 	"Pyrokinesis II",
	["school"] = 	{SCHOOL_PPOWER},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	37,
	["mana"] = 	16,
	["mana_max"] = 	16,
	["fail"] = 	-85,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			local n, p
			n, p = get_pyro_dam(Ind, 0)
			fire_grid_bolt(Ind, GF_FIRE, args.dir, n + p, " causes an inflammation for")
			end,
	["info"] = 	function()
			n, p = get_pyro_dam(Ind, 0)
			return "dam "..(n + p)
			end,
	["desc"] = 	{ "Causes a severe inflammation to burn your opponent.", }
}

MCRYOKINESIS_I = add_spell {
	["name"] = 	"Cryokinesis I",
	["school"] = 	{SCHOOL_PPOWER},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	24,
	["mana"] = 	10,
	["mana_max"] = 	10,
	["fail"] = 	-25,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			local n, p
			n, p = get_cryo_dam(Ind, 1)
			fire_grid_bolt(Ind, GF_COLD, args.dir, n + p, " causes freezing for")
			end,
	["info"] = 	function()
			local n, p
			n, p = get_cryo_dam(Ind, 1)
			return "dam "..(n + p)
			end,
	["desc"] = 	{ "Causes a dramatic temperature drop on your opponent.", }
}
MCRYOKINESIS_II = add_spell {
	["name"] = 	"Cryokinesis II",
	["school"] = 	{SCHOOL_PPOWER},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	39,
	["mana"] = 	17,
	["mana_max"] = 	17,
	["fail"] = 	-90,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			local n, p
			n, p = get_cryo_dam(Ind, 0)
			fire_grid_bolt(Ind, GF_COLD, args.dir, n + p, " causes freezing for")
			end,
	["info"] = 	function()
			local n, p
			n, p = get_cryo_dam(Ind, 0)
			return "dam "..(n + p)
			end,
	["desc"] = 	{ "Causes a dramatic temperature drop on your opponent.", }
}

MSHIELD = add_spell {
	["name"] = 	"Kinetic Shield",
	["school"] = 	{SCHOOL_PPOWER},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	40,
	["mana"] = 	30,
	["mana_max"] = 	30,
--	["fail"] = 	158,
	["fail"] = 	-50,
	["direction"] = FALSE,
	["spell"] = 	function()
			set_kinetic_shield(Ind, 10 + 2 * get_level(Ind, MSHIELD, 50) + randint(10))
			end,
	["info"] = 	function()
			return "dur "..10 + (2 * get_level(Ind, MSHIELD, 50)).."+d10"
			end,
	["desc"] = 	{
			"Uses telekinetic power as a shield that repels up to about",
			"half of all incoming melee attacks and physical projectiles.",
	}
}

if (def_hack("TEST_SERVER", nil)) then
MFUSION = add_spell {
	["name"] = 	"Mental Fusion",
--	["name"] = 	"Corporeal Fusion",
--	["school"] = 	{SCHOOL_TCONTACT, SCHOOL_PPOWER, SCHOOL_MINTRUSION},
	["school"] = 	{SCHOOL_TCONTACT, SCHOOL_PPOWER},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	40,
	["mana"] = 	200,
	["mana_max"] = 	200,
	["fail"] = 	20,
	["spell"] = 	function()
			do_cmd_fusion(Ind)
			end,
	["info"] = 	function()
			return ""
			end,
	["desc"] = 	{
			"Fuses your mind with a friendly target with open mind nearby,",
			"allowing you spell-casting but giving up control over your body.",
--			"Fuses your mind and body with a friendly target with open mind nearby,",
--			"allowing you spell-casting but giving up control over the body.",
	}
}
end
