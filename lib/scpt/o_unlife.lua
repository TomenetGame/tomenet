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
	["fail"] = 	0,
	["stat"] = 	A_WIS,
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
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	-30,
	["stat"] = 	A_WIS,
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
	["level"] = 	11,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	5,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			detect_living(Ind)
			end,
	["info"] = 	function()
			return ""
			end,
	["desc"] = 	{
			"Detects the presence of nearby living creatures.",
	}
}

OVERMINCONTROL = add_spell {
	["name"] = 	"Tainted Grounds",
	["name2"] = 	"TGrounds",
	["school"] = 	{SCHOOL_OUNLIFE},
	["spell_power"] = 0,
	["level"] = 	13,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	10,
	["stat"] = 	A_WIS,
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
	["mana"] = 	10,
	["mana_max"] = 	10,
	["fail"] = 	-10,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			if player.prace == RACE_VAMPIRE then
				set_tim_mp2hp(Ind, randint(5) + 10 + get_level(Ind, OREGEN, 30), 200 + get_level(Ind, OREGEN, 500), 10)
			else
				msg_print(Ind, "You shudder, as nether streams envelope you and quickly dissipate again..");
			end
	end,
	["info"] = 	function()
			local p = 200 + get_level(Ind, OREGEN, 500)
			p = p / 10
			return "dur "..(10 + get_level(Ind, OREGEN, 30)).."+d5 "..p.."HP/10MP tick"
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
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	-10,
	["stat"] = 	A_WIS,
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
	["fail"] = 	-70,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			if player.prace == RACE_VAMPIRE then
				do_res_stat(Ind, A_STR)
				do_res_stat(Ind, A_CON)
				do_res_stat(Ind, A_DEX)
				do_res_stat(Ind, A_WIS)
				do_res_stat(Ind, A_INT)
				do_res_stat(Ind, A_CHR)
				restore_level(Ind)
			else
				msg_print(Ind, "You shudder, as ghastly powers pass by your body..")
			end
		end,
	["info"] = 	function()
			return ""
			end,
	["desc"] = 	{
			"Restores drained stats and lost experience.",
			"--- This spell is only usable by true vampires. ---",
	}
}

ODRAINLIFE2 = add_spell {
	["name"] = 	"Siphon Life",
	["name2"] = 	"Siphon",
	["school"] = 	{SCHOOL_OUNLIFE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	37,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	-70,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		drain_life(Ind, args.dir, 14 + get_level(Ind, ODRAINLIFE2, 22))
		hp_player(Ind, player.ret_dam / 4, FALSE, FALSE)
	end,
	["info"] = 	function()
		return (14 + get_level(Ind, ODRAINLIFE2, 22)).."% (max 900), 25% heal"
	end,
	["desc"] = 	{ "Drains life from a target, which must not be non-living or undead.", }
}

OIMBUE = add_spell {
	["name"] = 	"Touch of Hunger",
	["name2"] = 	"Hunger",
	["school"] = 	{SCHOOL_OUNLIFE},
	["spell_power"] = 0,
	["level"] = 	42,
	["mana"] = 	40,
	["mana_max"] = 	40,
	["fail"] = 	-70,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			set_melee_brand(Ind, randint(10) + 10 + get_level(Ind, OIMBUE, 25), TBRAND_VAMPIRIC, 10, TRUE, FALSE)
			end,
	["info"] = 	function()
			return "dur "..(10 + get_level(Ind, OIMBUE, 25)).."+d10"
			end,
	["desc"] = 	{ "Temporarily imbue your melee attacks with vampiric power.", }
}

OWRAITHSTEP = add_spell {
	["name"] = 	"Wraithstep",
	["name2"] = 	"Wraith",
	["school"] = 	{SCHOOL_OUNLIFE, SCHOOL_OSHADOW},
	["level"] = 	46,
	["mana"] = 	45,
	["mana_max"] = 	45,
	["fail"] = 	-80,
	["stat"] = 	A_WIS,
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
