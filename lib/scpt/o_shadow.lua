-- handle the occultism school ('shadow magic')

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
		"At level 15 it passes through monsters, affecting those behind as well.",
		"At level 25 it affects all monsters in sight.",
	}
}

if (def_hack("TEST_SERVER", nil)) then
OCURSEDD = add_spell {
	["name"] = 	"Cause wounds",
	["school"] = 	{SCHOOL_},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	5,
	["mana"] = 	1,
	["mana_max"] = 	20,
	["fail"] = 	15,
	["stat"] = 	A_WIS,
	["ftk"] = 1,
	["spell"] = 	function(args)
		fire_grid_bolt(Ind, GF_MISSILE, args.dir, 10 + get_level(Ind, OCURSEDD, 300), "points and curses for")
	end,
	["info"] = 	function()
		return "power "..(10 + get_level(Ind, OCURSEDD, 300))
	end,
	["desc"] = 	{
		"Curse an enemy, causing wounds.",
	}
}
end
]]

OFEAR_I = add_spell {
	["name"] = 	"Cause Fear I",
	["school"] = 	{SCHOOL_OSHADOW},
	["spell_power"] = 0,
	["level"] = 	1,
	["mana"] = 	2,
	["mana_max"] = 	2,
	["fail"] = 	0,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		fire_bolt(Ind, GF_TURN_ALL, args.dir, 5 + get_level(Ind, OFEAR_I, 65), "hisses")
	end,
	["info"] = 	function()
		return "power "..(5 + get_level(Ind, OFEAR_I, 65))
	end,
	["desc"] = { "Temporarily scares a target.", }
}
__lua_OFEAR = OFEAR_I
OFEAR_II = add_spell {
	["name"] = 	"Cause Fear II",
	["school"] = 	{SCHOOL_OSHADOW},
	["spell_power"] = 0,
	["level"] = 	18,
	["mana"] = 	16,
	["mana_max"] = 	16,
	["fail"] = 	-25,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
		project_los(Ind, GF_TURN_ALL, 5 + get_level(Ind, OFEAR_I, 65), "hisses")
	end,
	["info"] = 	function()
		return "power "..(5 + get_level(Ind, OFEAR_I, 65))
	end,
	["desc"] = { "Temporarily scares all nearby foes.", }
}

OBLIND_I = add_spell {
	["name"] = 	"Blindness",
	["school"] = 	{SCHOOL_OSHADOW},
	["spell_power"] = 0,
	["level"] = 	3,
	["mana"] = 	2,
	["mana_max"] = 	2,
	["fail"] = 	0,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		fire_bolt(Ind, GF_BLIND, args.dir, 5 + get_level(Ind, OBLIND_I, 80), "hisses")
	end,
	["info"] = 	function()
		return "power "..(5 + get_level(Ind, OBLIND_I, 80))
	end,
	["desc"] = { "Temporarily blinds a target.", }
}
OBLIND_II = add_spell {
	["name"] = 	"Darkness",
	["school"] = 	{SCHOOL_OSHADOW},
	["spell_power"] = 0,
	["level"] = 	20,
	["mana"] = 	16,
	["mana_max"] = 	16,
	["fail"] = 	-25,
	["direction"] = FALSE,
	["spell"] = 	function()
		--project_los(Ind, GF_BLIND, 5 + get_level(Ind, OBLIND_I, 80), "hisses")
		--1..gl(7) starting at rad 2, or just gl(9) starting at rad 1
		msg_print(Ind, "You are surrounded by darkness")
		fire_ball(Ind, GF_DARK_WEAK, 0, 8192 + 10 + get_level(Ind, OBLIND_I, 80), 1 + get_level(Ind, OBLIND_II, 7), " calls darkness for")
	end,
	["info"] = 	function()
		return "power "..(10 + get_level(Ind, OBLIND_I, 80)).." rad "..(1 + get_level(Ind, OBLIND_II, 7))
	end,
	--["desc"] = { "Temporarily blinds all nearby foes.", }
	["desc"] = { "Causes a burst of darkness around you, blinding nearby foes.", }
}

DETECTINVIS = add_spell {
	["name"] = 	"Detect Invisible",
	["school"] = 	{SCHOOL_OSHADOW},
	["spell_power"] = 0,
	["level"] = 	3,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	10,
	["spell"] = 	function()
		detect_invisible(Ind)
	end,
	["info"] = 	function()
		--return "rad "..(10 + get_level(Ind, DETECTMONSTERS, 40))
		return ""
	end,
	["desc"] = 	{ "Detects all nearby invisible creatures.", }
}

--[[
POISONFOG_I = add_spell {
	["name"] = 	"Poisonous Fog I",
	["school"] = 	{SCHOOL_OSHADOW},
	["spell_power"] = 0,
	["level"] = 	8,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	15,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		fire_cloud(Ind, GF_POIS, args.dir, (1 + get_level(Ind, POISONFOG_I, 40)), 3, 5 + get_level(Ind, POISONFOG_I, 14), 9, " fires a noxious cloud of")
	end,
	["info"] = 	function()
		return "dam "..(1 + get_level(Ind, POISONFOG_I, 40)).." rad 3 dur "..(5 + get_level(Ind, POISONFOG_I, 14))
	end,
	["desc"] = {
		"Creates a cloud of poisonous fog.",
		"The cloud will persist for some turns, damaging all monsters passing by.",
	}
}
POISONFOG_II = add_spell {
	["name"] = 	"Poisonous Fog II",
	["school"] = 	{SCHOOL_OSHADOW},
	["spell_power"] = 0,
	["level"] = 	20,
	["mana"] = 	9,
	["mana_max"] = 	9,
	["fail"] = 	-30,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		fire_cloud(Ind, GF_POIS, args.dir, (1 + 38 + get_level(Ind, POISONFOG_II, 40)), 3, 5 + get_level(Ind, POISONFOG_I, 14), 9, " fires a noxious cloud of")
	end,
	["info"] = 	function()
		return "dam "..(1 + 38 + get_level(Ind, POISONFOG_II, 40)).." rad 3 dur "..(5 + get_level(Ind, POISONFOG_I, 14))
	end,
	["desc"] = {
		"Creates a cloud of poison.",
		"The cloud will persist for some turns, damaging all monsters passing by.",
	}
}
POISONFOG_III = add_spell {
	["name"] = 	"Poisonous Fog III",
	["school"] = 	{SCHOOL_OSHADOW},
	["spell_power"] = 0,
	["level"] = 	33,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	-60,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		fire_cloud(Ind, GF_UNBREATH, args.dir, (1 + 76 + get_level(Ind, POISONFOG_III, 40)), 3, 5 + get_level(Ind, POISONFOG_I, 14), 9, " fires a noxious cloud of")
	end,
	["info"] = 	function()
		return "dam "..(1 + 76 + get_level(Ind, POISONFOG_III, 40)).." rad 3 dur "..(5 + get_level(Ind, POISONFOG_I, 14))
	end,
	["desc"] = {
		"Creates a cloud of thick fog, not just poisoning but also preventing",
		"living beings from breathing. The cloud will persist for some turns.",
	}
}
]]--

OSLEEP_I = add_spell {
	["name"] = 	"Veil of Night I",
	["school"] = 	{SCHOOL_OSHADOW},
	["spell_power"] = 0,
	["level"] = 	5,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	0,
	["direction"] = TRUE,
	["stat"] = 	A_WIS,
	["spell"] = 	function(args)
		fire_bolt(Ind, GF_OLD_SLEEP, args.dir, 5 + get_level(Ind, OSLEEP_I, 80), "mumbles softly")
	end,
	["info"] = 	function()
		return "power "..(5 + get_level(Ind, OSLEEP_I, 80))
	end,
	["desc"] = { "Causes the target to fall asleep instantly.", }
}
OSLEEP_II = add_spell {
	["name"] = 	"Veil of Night II",
	["school"] = 	{SCHOOL_OSHADOW},
	["spell_power"] = 0,
	["level"] = 	20,--22
	["mana"] = 	19,
	["mana_max"] = 	19,
	["fail"] = 	-25,
	["direction"] = FALSE,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
		--project_los(Ind, GF_OLD_SLEEP, 5 + get_level(Ind, OSLEEP_I, 80), "mumbles softly")
		fire_wave(Ind, GF_OLD_SLEEP, 0, 5 + get_level(Ind, OSLEEP_I, 80), 1, 10, 3, EFF_WAVE, "mumbles softly")
	end,
	["info"] = 	function()
		return "power "..(5 + get_level(Ind, OSLEEP_I, 80)).." rad 10"
	end,
	--["desc"] = { "Lets all nearby monsters fall asleep.", }
	["desc"] = { "Expanding veil that lets monsters fall asleep.", }
}

function get_darkbolt_dam(Ind, limit_lev)
	--return 5 + get_level(Ind, DARKBOLT, 25), 7 + get_level(Ind, DARKBOLT, 25) + 1
	local lev

	lev = get_level(Ind, DARKBOLT_I, 50)
	if limit_lev ~= 0 and lev > limit_lev then lev = limit_lev + (lev - limit_lev) / 3 end

	--return 5 + (lev / 2), 7 + ((lev * 2) / 3) + 1
	--..since spirit was buffed:
	return 5 + ((lev * 3) / 5), 7 + ((lev * 2) / 3)
end
DARKBOLT_I = add_spell {
	["name"] = 	"Shadow Bolt I",
	["school"] = 	{SCHOOL_OSHADOW},
	["spell_power"] = 0,
	["level"] = 	10,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	0,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
		fire_bolt(Ind, GF_DARK, args.dir, damroll(get_darkbolt_dam(Ind, 1)), " casts a shadow bolt for")
	end,
	["info"] = 	function()
		local x, y

		x, y = get_darkbolt_dam(Ind, 1)
		return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Conjures up darkness into a powerful bolt.", }
}
DARKBOLT_II = add_spell {
	["name"] = 	"Shadow Bolt II",
	["school"] = 	{SCHOOL_OSHADOW},
	["spell_power"] = 0,
	["level"] = 	25,
	["mana"] = 	6,
	["mana_max"] = 	6,
	["fail"] = 	-30,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
		fire_bolt(Ind, GF_DARK, args.dir, damroll(get_darkbolt_dam(Ind, 15)), " casts a shadow bolt for")
	end,
	["info"] = 	function()
		local x, y

		x, y = get_darkbolt_dam(Ind, 15)
		return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Conjures up darkness into a powerful bolt.", }
}
DARKBOLT_III = add_spell {
	["name"] = 	"Shadow Bolt III",
	["school"] = 	{SCHOOL_OSHADOW},
	["spell_power"] = 0,
	["level"] = 	40,
	["mana"] = 	13,
	["mana_max"] = 	13,
	["fail"] = 	-70,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
		fire_bolt(Ind, GF_DARK, args.dir, damroll(get_darkbolt_dam(Ind, 0)), " casts a shadow bolt for")
	end,
	["info"] = 	function()
		local x, y

		x, y = get_darkbolt_dam(Ind, 0)
		return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Conjures up darkness into a powerful bolt.", }
}

POISONRES = add_spell {
	["name"] = 	"Aspect of Peril",
	["school"] = 	{SCHOOL_OSHADOW},
	["spell_power"] = 0,
	["level"] = 	14,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	0,
	["spell"] = 	function()
		local dur
		dur = randint(15) + 20 + get_level(Ind, POISONRES, 25)
		set_brand(Ind, dur, TBRAND_POIS, 10)
		if get_level(Ind, POISONRES, 50) >= 10 then
			set_oppose_pois(Ind, dur)
		end
	end,
	["info"] = 	function()
		return "dur "..(20 + get_level(Ind, POISONRES, 25)).."+d15"
	end,
	["desc"] = 	{
		"It temporarily bestows the touch of poison on your weapons.",
		"At level 10 it grants temporary poison resistance.",
	}
}

SHADOWGATE = add_spell {
	["name"] = 	"Shadow Gate",
	["school"] = 	{SCHOOL_OSHADOW, SCHOOL_CONVEYANCE},
	["am"] = 	75,
	["spell_power"] = 0,
	["level"] = 	26,
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	-30,
	["spell"] = 	function()
		--begin at ANNOY_DISTANCE as a minimum, to overcome
		do_shadow_gate(Ind, 4 + get_level(Ind, SHADOWGATE, 12))
		end,
	["info"] = 	function()
		return "range "..(4 + get_level(Ind, SHADOWGATE, 12))
		end,
	["desc"] = 	{ "Teleports you to the nearest opponent in line of sight.", }
}

OINVIS = add_spell {
	["name"] = 	"Shadow Shroud",
	["school"] = 	{SCHOOL_OSHADOW},
	["spell_power"] = 0,
	["level"] = 	30,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	-40,
	["spell"] = 	function()
		--if player.tim_invisibility == 0 then set_invis(Ind, randint(20) + 15 + get_level(Ind, OINVIS, 50), 20 + get_level(Ind, OINVIS, 50)) end
		set_invis(Ind, randint(20) + 15 + get_level(Ind, OINVIS, 50), 20 + get_level(Ind, OINVIS, 50))
	end,
	["info"] = 	function()
		return "dur "..(15 + get_level(Ind, OINVIS, 50)).."+d20 power "..(20 + get_level(Ind, OINVIS, 50))
	end,
	["desc"] = 	{ "Grants invisibility.", }
}

function get_chaosbolt_dam(Ind)
	local lev
	--same damage as shadow bolt iii at 50:
	--lev = get_level(Ind, CHAOSBOLT, 50) + 20
	--slightly more damage:
	lev = get_level(Ind, CHAOSBOLT, 50) + 21
	return 0 + (lev * 3) / 5, 1 + lev
end
CHAOSBOLT = add_spell {
	["name"] = 	"Chaos Bolt",
	["school"] = 	{SCHOOL_OSHADOW, SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["level"] = 	30,
	["mana"] = 	18,
	["mana_max"] = 	20,
	["fail"] = 	-55,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
		fire_bolt(Ind, GF_CHAOS, args.dir, damroll(get_chaosbolt_dam(Ind)), " casts a chaos bolt for")
	end,
	["info"] = 	function()
		local x, y
		x, y = get_chaosbolt_dam(Ind)
		return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Channels the powers of chaos into a bolt.", }
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
	["school"] = 	{SCHOOL_OSHADOW, SCHOOL_NECROMANCY},
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

--[[
ODRAINLIFE_I = add_spell {
	["name"] = 	"Drain Life I",
	["school"] = 	{SCHOOL_OSHADOW, SCHOOL_NECROMANCY},
	["am"] = 	75,
	["spell_power"] = 0,
	["level"] = 	20,
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	0,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		drain_life(Ind, args.dir, 9 + get_level(Ind, ODRAINLIFE_I, 5))
		hp_player(Ind, player.ret_dam / 6)
	end,
	["info"] = 	function()
		return "drain "..(9 + get_level(Ind, ODRAINLIFE_I, 5)).."%, heal for 17%"
	end,
	["desc"] = 	{ "Drains life from a target, which must not be non-living or undead.", }
}
ODRAINLIFE = add_spell {
	["name"] = 	"Drain Life",
	["school"] = 	{SCHOOL_OSHADOW, SCHOOL_NECROMANCY},
	["am"] = 	75,
	["spell_power"] = 0,
	["level"] = 	40,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	-65,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		drain_life(Ind, args.dir, 10 + get_level(Ind, ODRAINLIFE_I, 9))
		hp_player(Ind, player.ret_dam / 4)
	end,
	["info"] = 	function()
		return "drain "..(10 + get_level(Ind, ODRAINLIFE_I, 9)).."%, heal for 25%"
	end,
	["desc"] = 	{ "Drains life from a target, which must not be non-living or undead.", }
}]]--
ODRAINLIFE = add_spell {
	["name"] = 	"Drain Life",
	["school"] = 	{SCHOOL_OSHADOW, SCHOOL_NECROMANCY},
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

DARKBALL = add_spell {
	["name"] = 	"Darkness Storm",
	["school"] = 	{SCHOOL_OSHADOW},
	["spell_power"] = 0,
	["level"] = 	42,
	["mana"] = 	35,
	["mana_max"] = 	35,
	["fail"] = 	-70,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			fire_ball(Ind, GF_DARK, args.dir, rand_int(100) + 340 + get_level(Ind, DARKBALL, 1400), 3, " conjures up a darkness storm for")
	end,
	["info"] = 	function()
		return "dam d100+"..(340 + get_level(Ind, DARKBALL, 1400)).." rad 3"
	end,
	["desc"] = 	{ "Conjures up a storm of darkness.", }
}
