-- handle the occultism school ('unlife')
-- More spell ideas: "Torment" (res fear+conf), "Rune of Styx" (glyph), "Desecration" (aoe cloud)

--[[ there is currently no paralyzation, it's just sleep so monster wakes up on taking damage..
OHOLD_I = add_spell {
	["name"] = 	"Paralyze",
	["name2"] = 	"Para",
	["school"] = 	{SCHOOL_OUNLIFE},
	["spell_power"] = 0,
	["level"] = 	5,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	0,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		fire_bolt(Ind, GF_HOLD, args.dir, 5 + get_level(Ind, OHOLD_I, 65), "hisses")
	end,
	["info"] = 	function()
		return "power "..(5 + get_level(Ind, OHOLD_I, 65))
	end,
	["desc"] = { "Temporarily paralyzes a target.", }
} ]]

OSLOWMONSTER_I = add_spell {
	["name"] = 	"Fatigue I",
	["name2"] = 	"Fatig I",
	["school"] = 	{SCHOOL_OUNLIFE},
	["spell_power"] = 0,
	["level"] = 	5,
	["mana"] = 	5,
	["mana_max"] = 	5,
	["fail"] = 	10,
	["direction"] = TRUE,
	["spell"] = 	function(args)
				fire_grid_bolt(Ind, GF_OLD_SLOW, args.dir, 5 + get_level(Ind, OSLOWMONSTER_I, 100), "drains power from your muscles")
			end,
	["info"] = 	function()
				return "power "..(5 + get_level(Ind, OSLOWMONSTER_I, 100))
			end,
	["desc"] = 	{ "Drains power from the muscles of your opponent, slowing it down.", }
}
OSLOWMONSTER_II = add_spell {
	["name"] = 	"Fatigue II",
	["name2"] = 	"Fatig II",
	["school"] = 	{SCHOOL_OUNLIFE},
	["spell_power"] = 0,
	["level"] = 	20,
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	-20,
	["direction"] = FALSE,
	["spell"] = 	function()
				project_los(Ind, GF_OLD_SLOW, 5 + get_level(Ind, OSLOWMONSTER_I, 100), "drains power from your muscles")
			end,
	["info"] = 	function()
				return "power "..(5 + get_level(Ind, OSLOWMONSTER_I, 100))
			end,
	["desc"] = 	{ "Drains power from the muscles of all opponents in sight, slowing them down.", }
}

OSENSELIFE = add_spell {
	["name"] = 	"Detect Lifeforce",
	["name2"] = 	"DetLF",
	["school"] = 	{SCHOOL_OUNLIFE},
	["spell_power"] = 0,
	--["level"] = 	32,
	["level"] = 	12,
	["mana"] = 	3,
	["mana_max"] = 	3,
	--["fail"] = 	-20,
	["fail"] = 	15,
	["spell"] = 	function()
			--set_tim_espInd, 20 + randint(10) + get_level(Ind, OSENSELIFE, 50))
			detect_living(Ind)
			end,
	["info"] = 	function()
			--return "dur 20+d10+d"..get_level(Ind, OSENSELIFE, 50)
			return ""
			end,
	["desc"] = 	{
			--"Sense the presence of creatures for a while.",
			"Detects the presence of living creatures.",
	}
}

OVERMINCONTROL = add_spell {
	["name"] = 	"Tainted Grounds",
	["name2"] = 	"TGrounds",
	["school"] = 	{SCHOOL_OUNLIFE},
	["spell_power"] = 0,
	["level"] = 	17,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	20,
	["spell"] = 	function()
			do_vermin_control(Ind)
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{ "Prevents any vermin from breeding.", }
}

OREGEN = add_spell {
	["name"] = 	"Nether Sap",
	["name2"] = 	"NSap",
	["school"] = 	{SCHOOL_OUNLIFE},
	["spell_power"] = 0,
	["level"] = 	22,
	["mana"] = 	40,
	["mana_max"] = 	40,
	["fail"] = 	0,
	["spell"] = 	function()
			set_tim_mp2hp(Ind, randint(10) + 5 + get_level(Ind, OREGEN, 50), 8 + get_level(Ind, OREGEN, 50) / 4)
	end,
	["info"] = 	function()
			return "dur "..(5 + get_level(Ind, OREGEN, 50)).."+d10 power "..((8 + get_level(Ind, OREGEN, 50) / 4))
	end,
	["desc"] = 	{
			"Draws from nether undercurrents to replenish your health.",
			"--- This spell is only usable by true vampires. ---",
	}
}

OSUBJUGATION = add_spell {
	["name"] = 	"Subjugation",
	["name2"] = 	"Subj",
	["school"] = 	{SCHOOL_OUNLIFE, SCHOOL_NECROMANCY},
	["spell_power"] = 0,
	["level"] = 	26,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	-10,
	["spell"] = 	function()
			project_los(Ind, GF_STASIS, 1000 + 5 + get_level(Ind, OSUBJUGATION, 50), "casts a spell")
	end,
	["info"] = 	function()
			return "dur "..(5 + get_level(Ind, OSUBJUGATION, 50))
	end,
	["desc"] = 	{ "Attempts to subjugate all undead lesser than you.", }
}

--ENABLE_DEATHKNIGHT:
function get_netherbolt_dam(Ind)
	local lev
	--same damage as shadow bolt iii at 50:
	--lev = get_level(Ind, NETHERBOLT, 50) + 20
	--slightly more damage:
	lev = get_level(Ind, NETHERBOLT, 50) + 21
	return 0 + (lev * 3) / 5, 1 + lev
end
NETHERBOLT = add_spell {
	["name"] = 	"Nether Bolt",
	["name2"] = 	"NBolt",
	["school"] = 	{SCHOOL_OUNLIFE},
	["spell_power"] = 0,
	["level"] = 	30,
	["mana"] = 	16,
	["mana_max"] = 	18,
	["fail"] = 	-55,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
		fire_bolt(Ind, GF_NETHER, args.dir, damroll(get_netherbolt_dam(Ind)), " casts a nether bolt for")
	end,
	["info"] = 	function()
		local x, y
		x, y = get_netherbolt_dam(Ind)
		return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Channels lingering nether into a bolt.", }
}

OUNLIFERES = add_spell {
	["name"] = 	"Permeation",
	["name2"] = 	"Perm",
	["school"] = 	{SCHOOL_OUNLIFE},
	["spell_power"] = 0,
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
	["desc"] = 	{
			"Restores drained stats and lost experience.",
			"--- This spell is only usable by true vampires. ---",
	}
}

OIMBUE = add_spell {
	["name"] = 	"Touch of Hunger",
	["name2"] = 	"Hunger",
	["school"] = 	{SCHOOL_OUNLIFE},
	["spell_power"] = 0,
	["level"] = 	42,
	["mana"] = 	45,
	["mana_max"] = 	45,
	["fail"] = 	-70,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			set_melee_brand(Ind, randint(10) + 10 + get_level(Ind, OIMBUE, 25), TBRAND_VAMP, 10)
			end,
	["info"] = 	function()
			return ""
			end,
	["desc"] = 	{ "Temporarily imbue your melee weapon with vampiric power.", }
}

OWRAITHSTEP = add_spell {
	["name"] = 	"Wraithstep",
	["name2"] = 	"Wraith",
	["school"] = 	{SCHOOL_OUNLIFE, SCHOOL_OSHADOW},
	["level"] = 	46,
	["mana"] = 	60,
	["mana_max"] = 	60,
	["fail"] = 	-80,
	["spell"] = 	function()
			set_tim_wraithstep(Ind, randint(30) + 20 + get_level(Ind, OWRAITHSTEP, 40))
	end,
	["info"] = 	function()
			return "dur "..(20 + get_level(Ind, OWRAITHSTEP, 40)).."+d30"
	end,
	["desc"] = 	{
			"Renders you temporarily immaterial, but the effect will cease",
			"immediately when you step from walls into open space again.",
	}
}
