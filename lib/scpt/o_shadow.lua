-- handle the occultism school ('shadow magic')

--[[
OCURSE = add_spell {
	["name"] = 	"Curse",
	["school"] = 	{SCHOOL_},
	["am"] =	75,
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
			"At level 15 it passes through monsters, affecting those behind as well",
			"At level 25 it affects all monsters in sight",
	}
}

if (def_hack("TEST_SERVER", nil)) then
OCURSEDD = add_spell {
	["name"] = 	"Cause wounds",
	["school"] = 	{SCHOOL_},
	["am"] =	75,
	["level"] = 	5,
	["mana"] = 	1,
	["mana_max"] = 	20,
	["fail"] = 	15,
	["stat"] =	A_WIS,
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

ODRAINLIFE = add_spell {
	["name"] = 	"Sap Life",
	["school"] = 	{SCHOOL_SHADOW},
	["am"] =	75,
	["level"] = 	20,
	["mana"] = 	10,
	["mana_max"] = 	80,
	["fail"] = 	30,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		local type
		drain_life(Ind, args.dir, 10 + get_level(Ind, HDRAINLIFE, 10))
		hp_player(Ind, player.ret_dam / 4)
	end,
	["info"] = 	function()
		return "drains "..(10 + get_level(Ind, HDRAINLIFE, 10)).."% life"
	end,
	["desc"] =	{
			"Drains life from a target, which must not be non-living or undead.",
	}
}

POISONFOG = add_spell {
	["name"] = 	"Poisonous Fog",
	["school"] = 	{SCHOOL_SHADOW},
	["level"] = 	3,
	["mana"] = 	3,
	["mana_max"] = >30,
	["fail"] = 	20,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			local type

			if get_level(Ind, NOXIOUSCLOUD, 50) >= 30 then type = GF_UNBREATH
			else type = GF_POIS end
			type = GF_POIS
			fire_cloud(Ind, type, args.dir, ((1 + get_level(Ind, NOXIOUSCLOUD, 75)) + (get_level(Ind,MANATHRUST,50) * 2)), 3, 5 + get_level(Ind, NOXIOUSCLOUD, 40), " fires a noxious cloud of")
			fire_cloud(Ind, type, args.dir, (1 + get_level(Ind, NOXIOUSCLOUD, 75)), 3, 5 + get_level(Ind, NOXIOUSCLOUD, 40), 10, " fires a noxious cloud of")
			fire_cloud(Ind, type, args.dir, (1 + get_level(Ind, NOXIOUSCLOUD, 113)), 3, 5 + get_level(Ind, NOXIOUSCLOUD, 10), 10, " fires a noxious cloud of")
--1.5			fire_cloud(Ind, type, args.dir, (1 + get_level(Ind, NOXIOUSCLOUD, 113)), 3, 5 + get_level(Ind, NOXIOUSCLOUD, 14), 9, " fires a noxious cloud of")
			fire_cloud(Ind, type, args.dir, (1 + get_level(Ind, NOXIOUSCLOUD, 156)), 3, 5 + get_level(Ind, NOXIOUSCLOUD, 14), 9, " fires a noxious cloud of")
	end,
	["info"] = 	function()
			return "dam "..(1 + (get_level(Ind, NOXIOUSCLOUD, 156))).." rad 3 dur "..(5 + get_level(Ind, NOXIOUSCLOUD, 14))
	end,
	["desc"] 	{
			"Creates a cloud of poison",
			"The cloud will persist for some turns, damaging all monsters passing by",
			"At level 30 it turns into a thick gas preventing living beings from breathing"
	}
}

SHADOWJUMP = add_spell {
	["name"] = 	"Shadow Jump",
	["school"] = 	{SCHOOL_SHADOW},
	["am"] =	50,
	["spell_power"] = 0,
	["blind"] = 	0,
	["level"] = 	24,
	["mana"] = 	20,
	["mana_max"] = 	30,
	["fail"] = 	10,
	["spell"] = 	function()
			do_shadow_jump(Ind, 5 + get_level(Ind, SHADOWJUMP, 5))
			end,
	["info"] = 	function()
			return "range "..(5 + get_level(Ind, SHADOWJUMP, 5))
			end,
	["desc"] = {
			"Teleports you to the nearest opponent.",
	}
}

function get_darkbolt_dam()
	return 5 + get_level(Ind, DARKBOLT, 25), 7 + get_level(Ind, DARKBOLT, 25) + 1
end
DARKBOLT = add_spell {
	["name"] = 	"Darkness Bolt",
	["school"] = 	SCHOOL_SHADOW,
	["level"] = 	10,
	["mana"] = 	3,
	["mana_max"] = 	12,
	["fail"] = 	-10,
	["direction"] = TRUE,
	["ftk"] = 1,
	["spell"] = 	function(args)
			fire_bolt(Ind, GF_DARK, args.dir, damroll(get_darkbolt_dam()), " casts a darkness bolt for")
	end,
	["info"] = 	function()
			local x, y

			x, y = get_darkbolt_dam()
			return "dam "..x.."d"..y
	end,
	["desc"] = 	{
			"Conjures up shadows into a powerful bolt",
		}
}

OINVIS = add_spell {
	["name"] = 	"Shadow Shroud",
	["school"] = 	{SCHOOL_SHADOW},
	["level"] = 	30,
	["mana"] = 	20,
	["mana_max"] = 	50,
	["fail"] = 	70,
	["spell"] = 	function()
--		if player.tim_invisibility == 0 then set_invis(Ind, randint(20) + 15 + get_level(Ind, OINVIS, 50), 20 + get_level(Ind, OINVIS, 50)) end
		set_invis(Ind, randint(20) + 15 + get_level(Ind, OINVIS, 50), 20 + get_level(Ind, OINVIS, 50))
	end,
	["info"] = 	function()
			return "dur "..(15 + get_level(Ind, OINVIS, 50)).."+d20 power "..(20 + get_level(Ind, OINVIS, 50))
	end,
	["desc"] = 	{
			"Grants invisibility"
	}
}

DETECTINVIS = add_spell {
	["name"] = 	"Detect Invisible",
	["school"] = 	{SCHOOL_SHADOW},
	["level"] = 	3,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	10,
	["spell"] = 	function()
			detect_invisible(Ind)
--			if player.spell_project > 0 then
--				fire_ball(Ind, GF_DETECTINVIS_PLAYER, 0, 1, player.spell_project, "")
--			end
	end,
	["info"] = 	function()
--			return "rad "..(10 + get_level(Ind, DETECTMONSTERS, 40))
			return ""
	end,
	["desc"] = 	{
			"Detects all nearby invisible creatures.",
--			"***Affected by the Meta spell: Project Spell***",
	}
}

POISONRES = add_spell {
	["name"] = 	"Poison Resistance",
	["school"] = 	{SCHOOL_SHADOW},
	["level"] = 	22,
	["mana"] = 	10,
	["mana_max"] = 	20,
	["fail"] = 	70,
	["spell"] = 	function()
			set_oppose_pois(Ind, randint(30) + 25 + get_level(Ind, POISONRES, 25))
--			if get_level(Ind, POISONBLOOD, 50) >= 10 then set_brand(Ind, randint(30) + 25 + get_level(Ind, POISONBLOOD, 25), BRAND_POIS, 10) end
	end,
	["info"] = 	function()
			return "dur "..(25 + get_level(Ind, POISONRES, 25)).."+d30"
	end,
	["desc"] = 	{
			"Grants poison resistance",
--			"At level 10 it provides poison branding to wielded weapon"
	}
}

OSLEEP = add_spell {
	["name"] = 	"Veil of Night",
	["school"] = 	{SCHOOL_SHADOW},
	["am"] = 	33,
	["spell_power"] = 0,
	["level"] = 	5,
	["mana"] = 	2,
	["mana_max"] = 	16,
	["fail"] = 	10,
	["direction"] = function() if get_level(Ind, OSLEEP, 50) >= 20 then return FALSE else return TRUE end end,
	["spell"] = 	function(args)
			if get_level(Ind, OSLEEP, 50) < 20 then
				fire_grid_bolt(Ind, GF_OLD_SLEEP, args.dir, 5 + get_level(Ind, OSLEEP, 80), "mumbles softly")
			else
				project_los(Ind, GF_OLD_SLEEP, 5 + get_level(Ind, OSLEEP, 80), "mumbles softly")
			end
			end,
	["info"] = 	function()
				return "power "..(5 + get_level(Ind, OSLEEP, 80))
			end,
	["desc"] = {
			"Causes the target to fall asleep instantly",
--			"Lets monsters next to you fall asleep",
			"At level 20 it lets all nearby monsters fall asleep",
	}
}
