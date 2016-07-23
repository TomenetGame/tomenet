-- handle the mind school
--blind/paralysis? wouldn't probably make noticable diff from conf/sleep!
--((static: res insanity/effects))

function get_psiblast_dam(Ind, limit_lev)
	local lev, llev, lev2

	lev = get_level(Ind, MMINDBLAST_I, 45)
	--10% cut, since we scaled level from 1..45, not from 1..50:
	llev = (limit_lev * 9 + 1) / 10
	if limit_lev ~= 0 and lev > llev then lev = llev + (lev - llev) / 3 end

	lev2 = get_level(Ind, MMINDBLAST_I, 250)
	if limit_lev ~= 0 and lev2 > limit_lev * 5 then lev2 = limit_lev * 5 + (lev2 - limit_lev * 5) / 3 end

--	return 4 + get_level(Ind, MMINDBLAST, 4), 3 + get_level(Ind, MMINDBLAST, 45) <- just 50% of targetted value
--	return 2 + get_level(Ind, MMINDBLAST, 6), 3 + get_level(Ind, MMINDBLAST, 45), get_level(Ind, MMINDBLAST, 200)
	return 2 + get_level(Ind, MMINDBLAST_I, 7), 3 + lev, lev2
end

function get_psistorm_dam(Ind, limit_lev)
	local lev
	lev = get_level(Ind, MPSISTORM_I, 200)
	if limit_lev ~= 0 and lev > limit_lev * 4 then lev = limit_lev * 4 + (lev - limit_lev * 4) / 2 end
	return 33 + lev
end

MSCARE_I = add_spell {
	["name"] = 	"Scare I",
	["school"] = 	{SCHOOL_MINTRUSION},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	1,
	["mana"] = 	2,
	["mana_max"] = 	2,
	["fail"] = 	10,
	["direction"] = TRUE,
	["spell"] = 	function(args)
				fire_grid_bolt(Ind, GF_TURN_ALL, args.dir, 5 + get_level(Ind, MSCARE_I, 80), "stares deep into your eyes")
			end,
	["info"] = 	function()
				return "power "..(5 + get_level(Ind, MSCARE_I, 80))
			end,
	["desc"] = 	{ "Tries to manipulate the mind of a monster to scare it.", }
}
__lua_MSCARE = MSCARE_I
MSCARE_II = add_spell {
	["name"] = 	"Scare II",
	["school"] = 	{SCHOOL_MINTRUSION},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	15,
	["mana"] = 	10,
	["mana_max"] = 	10,
	["fail"] = 	-20,
	["direction"] = FALSE,
	["spell"] = 	function()
			project_los(Ind, GF_TURN_ALL, 5 + get_level(Ind, MSCARE_I, 80), "stares deep into your eyes")
			end,
	["info"] = 	function()
				return "power "..(5 + get_level(Ind, MSCARE_I, 80))
			end,
	["desc"] = 	{ "Tries to manipulate the minds of all monster in sight to scare them.", }
}

MCONFUSE_I = add_spell {
	["name"] = 	"Confuse I",
	["school"] = 	{SCHOOL_MINTRUSION},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	3,
	["mana"] = 	4,
	["mana_max"] = 	4,
	["fail"] = 	10,
	["direction"] = TRUE,
	["spell"] = 	function(args)
				fire_grid_bolt(Ind, GF_OLD_CONF, args.dir, 5 + get_level(Ind, MCONFUSE_I, 100), "focusses on your mind")
			end,
	["info"] = 	function()
				return "power "..(5 + get_level(Ind, MCONFUSE_I, 100))
			end,
	["desc"] = 	{ "Tries to manipulate the mind of a monster to confuse it.", }
}
MCONFUSE_II = add_spell {
	["name"] = 	"Confuse II",
	["school"] = 	{SCHOOL_MINTRUSION},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	18,
	["mana"] = 	14,
	["mana_max"] = 	14,
	["fail"] = 	-20,
	["direction"] = FALSE,
	["spell"] = 	function()
				project_los(Ind, GF_OLD_CONF, 5 + get_level(Ind, MCONFUSE_I, 100), "focusses on your mind")
			end,
	["info"] = 	function()
				return "power "..(5 + get_level(Ind, MCONFUSE_I, 100))
			end,
	["desc"] = 	{ "Tries to manipulate the minds of all monsters in sight to confuse them.", }
}

MSLEEP_I = add_spell {
	["name"] = 	"Hypnosis I",
	["school"] = 	{SCHOOL_MINTRUSION},
	["am"] = 	33,
	["spell_power"] = 0,
	["level"] = 	5,
	["mana"] = 	4,
	["mana_max"] = 	4,
	["fail"] = 	10,
	["direction"] = TRUE,
	["spell"] = 	function(args)
				fire_grid_bolt(Ind, GF_OLD_SLEEP, args.dir, 5 + get_level(Ind, MSLEEP_I, 80), "mumbles softly")
			end,
	["info"] = 	function()
				return "power "..(5 + get_level(Ind, MSLEEP_I, 80))
			end,
	["desc"] = 	{ "Causes the target to fall asleep instantly.", }
}
MSLEEP_II = add_spell {
	["name"] = 	"Hypnosis II",
	["school"] = 	{SCHOOL_MINTRUSION},
	["am"] = 	33,
	["spell_power"] = 0,
	["level"] = 	20,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	-20,
	["direction"] = FALSE,
	["spell"] = 	function()
				project_los(Ind, GF_OLD_SLEEP, 5 + get_level(Ind, MSLEEP_I, 80), "mumbles softly")
			end,
	["info"] = 	function()
				return "power "..(5 + get_level(Ind, MSLEEP_I, 80))
			end,
	["desc"] = 	{ "Causes all monsters in sight to fall asleep instantly.", }
}

MSLOWMONSTER_I = add_spell {
	["name"] = 	"Drain Strength I",
	["school"] = 	{SCHOOL_MINTRUSION},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	7,
	["mana"] = 	5,
	["mana_max"] = 	5,
	["fail"] = 	10,
	["direction"] = TRUE,
	["spell"] = 	function(args)
				fire_grid_bolt(Ind, GF_OLD_SLOW, args.dir, 5 + get_level(Ind, MSLOWMONSTER_I, 100), "drains power from your muscles")
			end,
	["info"] = 	function()
				return "power "..(5 + get_level(Ind, MSLOWMONSTER_I, 100))
			end,
	["desc"] = 	{ "Drains power from the muscles of your opponent, slowing it down.", }
}
MSLOWMONSTER_II = add_spell {
	["name"] = 	"Drain Strength II",
	["school"] = 	{SCHOOL_MINTRUSION},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	22,
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	-20,
	["direction"] = FALSE,
	["spell"] = 	function()
				project_los(Ind, GF_OLD_SLOW, 5 + get_level(Ind, MSLOWMONSTER_I, 100), "drains power from your muscles")
			end,
	["info"] = 	function()
				return "power "..(5 + get_level(Ind, MSLOWMONSTER_I, 100))
			end,
	["desc"] = 	{ "Drains power from the muscles of all opponents in sight, slowing them down.", }
}

MMINDBLAST_I = add_spell {
	["name"] = 	"Psionic Blast I",
	["school"] = 	{SCHOOL_MINTRUSION},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	1,
	["mana"] = 	2,
	["mana_max"] = 	2,
	["fail"] = 	10,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			local d, s, p
			d, s, p = get_psiblast_dam(Ind, 1)
			fire_grid_bolt(Ind, GF_PSI, args.dir, damroll(d, s) + p, "")
			end,
	["info"] = 	function()
			local d, s, p
			d, s, p = get_psiblast_dam(Ind, 1)
			return "power "..d.."d"..s.."+"..p
			end,
	["desc"] = 	{ "Blasts the target's mind with psionic energy.", }
}
MMINDBLAST_II = add_spell {
	["name"] = 	"Psionic Blast II",
	["school"] = 	{SCHOOL_MINTRUSION},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	20,
	["mana"] = 	6,
	["mana_max"] = 	6,
	["fail"] = 	-30,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			local d, s, p
			d, s, p = get_psiblast_dam(Ind, 20)
			fire_grid_bolt(Ind, GF_PSI, args.dir, damroll(d, s) + p, "")
			end,
	["info"] = 	function()
			local d, s, p
			d, s, p = get_psiblast_dam(Ind, 20)
			return "power "..d.."d"..s.."+"..p
			end,
	["desc"] = 	{ "Blasts the target's mind with psionic energy.", }
}
MMINDBLAST_III = add_spell {
	["name"] = 	"Psionic Blast III",
	["school"] = 	{SCHOOL_MINTRUSION},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	40,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	-95,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			local d, s, p
			d, s, p = get_psiblast_dam(Ind, 0)
			fire_grid_bolt(Ind, GF_PSI, args.dir, damroll(d, s) + p, "")
			end,
	["info"] = 	function()
			local d, s, p
			d, s, p = get_psiblast_dam(Ind, 0)
			return "power "..d.."d"..s.."+"..p
			end,
	["desc"] = 	{ "Blasts the target's mind with psionic energy.", }
}

MPSISTORM_I = add_spell {
	["name"] = 	"Psi Storm I",
	["school"] = 	{SCHOOL_MINTRUSION},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	18,
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	5,
	["direction"] = TRUE,
	["spell"] = function(args)
--	["spell"] = function()
		local d
		d = get_psistorm_dam(Ind, 1)
		fire_cloud(Ind, GF_PSI, args.dir, d, 3 + get_level(Ind, MPSISTORM_I, 4), 6 + get_level(Ind, MPSISTORM_I, 4), 14, " releases a psi storm for")
--		fire_cloud(Ind, GF_PSI, 0, (1 + get_level(Ind, MPSISTORM, 76)), 2 + get_level(Ind, MPSISTORM, 4), 5 + get_level(Ind, MPSISTORM, 5), 14, " releases a psi storm for")
--		fire_wave(Ind, GF_PSI, 0, (1 + get_level(Ind, MPSISTORM, 76)), 2 + get_level(Ind, MPSISTORM, 4), 5 + get_level(Ind, MPSISTORM, 5), 14, EFF_STORM, " releases a psi storm for
	end,
	["info"] = function()
		local d
		d = get_psistorm_dam(Ind, 1)
		return "dam "..d.." rad "..(3 + get_level(Ind, MPSISTORM_I, 4)).." dur "..(6 + get_level(Ind, MPSISTORM_I, 4))
	end,
	["desc"] = { "A psionic storm that damages and disturbs all minds within an area.", }
}
MPSISTORM_II = add_spell {
	["name"] = 	"Psi Storm II",
	["school"] = 	{SCHOOL_MINTRUSION},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	38,
	["mana"] = 	32,
	["mana_max"] = 	32,
	["fail"] = 	-90,
	["direction"] = TRUE,
	["spell"] = function(args)
--	["spell"] = function()
		local d
		d = get_psistorm_dam(Ind, 0)
		fire_cloud(Ind, GF_PSI, args.dir, d, 3 + get_level(Ind, MPSISTORM_I, 4), 6 + get_level(Ind, MPSISTORM_I, 4), 14, " releases a psi storm for")
--		fire_cloud(Ind, GF_PSI, 0, (1 + get_level(Ind, MPSISTORM, 76)), 2 + get_level(Ind, MPSISTORM, 4), 5 + get_level(Ind, MPSISTORM, 5), 14, " releases a psi storm for")
--		fire_wave(Ind, GF_PSI, 0, (1 + get_level(Ind, MPSISTORM, 76)), 2 + get_level(Ind, MPSISTORM, 4), 5 + get_level(Ind, MPSISTORM, 5), 14, EFF_STORM, " releases a psi storm for
	end,
	["info"] = function()
		local d
		d = get_psistorm_dam(Ind, 0)
		return "dam "..d.." rad "..(3 + get_level(Ind, MPSISTORM_I, 4)).." dur "..(6 + get_level(Ind, MPSISTORM_I, 4))
	end,
	["desc"] = { "A psionic storm that damages and disturbs all minds within an area.", }
}

MSILENCE = add_spell {
	["name"] = 	"Psychic Suppression",
	["school"] = 	{SCHOOL_MINTRUSION},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	10,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	10,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			--using this hack to transport 2 parameters at once,
			--ok since we use a single-target spell and not a ball
			fire_grid_bolt(Ind, GF_SILENCE, args.dir, get_level(Ind, MSILENCE, 63) + ((4 + get_level(Ind, MSILENCE, 4)) * 100), "")
			end,
	["info"] = 	function()
			return "power "..(get_level(Ind, MSILENCE, 63)).." dur "..(4 + get_level(Ind, MSILENCE, 4))
			end,
	["desc"] = 	{ "Drains the target's psychic energy, impacting its ability to cast spells.", }
}

MMAP = add_spell {
	["name"] = 	"Remote Vision",
	["school"] = 	{SCHOOL_MINTRUSION},
--	["school"] = 	{SCHOOL_MINTRUSION, SCHOOL_TCONTACT}
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	20,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	0,
	["direction"] = FALSE,
	["spell"] = 	function()
			local pow = get_level(Ind, MMAP)
			if pow > 15 then pow = 15 end
			mind_map_level(Ind, pow)
			end,
	["info"] = 	function()
			local pow = get_level(Ind, MMAP)
			if pow > 15 then pow = 15 end
			return "power "..pow
			end,
	["desc"] = 	{
			"Forcefully uses the vision of sentient life forms around.",
			"*** Will be transferred to allied open",
			"    minds on the same floor if your",
			"    Attunement skill is at least 20. ***",
	}
}

--[[ Old version, requiring pets. Not cool though. See new variant below!
MCHARM = add_spell {
	["name"] = 	"Charm",
	["school"] = 	{SCHOOL_MINTRUSION},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	1,
	["mana"] = 	1,
	["mana_max"] = 	20,
	["fail"] = 	10,
	["direction"] = function () if get_level(Ind, CHARM, 50) >= 35 then return FALSE else return TRUE end end,
	["spell"] = 	function(args)
			if get_level(Ind, CHARM, 50) >= 35 then
				project_los(Ind, GF_CHARM, 10 + get_level(Ind, CHARM, 150), "mumbles softly")
			elseif get_level(Ind, CHARM, 50) >= 15 then
				fire_ball(Ind, GF_CHARM, args.dir, 10 + get_level(Ind, CHARM, 150), 3, "mumbles softly")
			else
				fire_grid_bolt(Ind, GF_CHARM, args.dir, 10 + get_level(Ind, CHARM, 150), "mumbles softly")
			end

	end,
	["info"] = 	function()
			return "power "..(10 + get_level(Ind, CHARM, 150))
	end,
	["desc"] = 	{
			"Tries to manipulate the mind of a monster to make it friendly",
			"At level 15 it turns into a ball",
			"At level 35 it affects all monsters in sight",
	}
}
]]

-- New idea: works like *invincibility*: monsters will ignore you (and often your party members too ;)
MCHARM = add_spell {
	["name"] = 	"Charm",
	["school"] = 	{SCHOOL_MINTRUSION},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	33,
	["mana"] = 	10,
	["mana_max"] = 	10,
	["fail"] = 	10,
	["stat"] = 	A_CHR,
	["direction"] = function () if get_level(Ind, MCHARM, 50) >= 13 then return FALSE else return TRUE end end,
	["spell"] = 	function(args)
			--reset previous charm spell first:
			do_mstopcharm(Ind)
			--cast charm!
			if get_level(Ind, MCHARM, 50) >= 13 then
				project_los(Ind, GF_CHARMIGNORE, 10 + get_level(Ind, MCHARM, 150), "focusses")
			elseif get_level(Ind, MCHARM, 50) >= 7 then
				fire_ball(Ind, GF_CHARMIGNORE, args.dir, 10 + get_level(Ind, MCHARM, 150), 3, "focusses")
			else
				fire_grid_bolt(Ind, GF_CHARMIGNORE, args.dir, 10 + get_level(Ind, MCHARM, 150), "focusses")
			end
	end,
	["info"] = 	function()
--			return "power "..(10 + get_level(Ind, MCHARM, 150))
			return ""
	end,
	["desc"] = 	{
			"Tries to manipulate the mind of a monster to make it ignore you.",
			"At level 7 it turns into a ball.",
			"At level 13 it affects all monsters in sight.",
	}
}

MSTOPCHARM = add_spell {
	["name"] = 	"Stop Charm",
	["school"] = 	{SCHOOL_MINTRUSION},
	["am"] = 	0,
	["spell_power"] = 0,
	["level"] = 	33,
	["mana"] = 	0,
	["mana_max"] = 	0,
	["fail"] = 	101,
	["direction"] = FALSE,
	["spell"] = 	function()
			do_mstopcharm(Ind)
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{ "Cancel charming of any monsters.", }
}
