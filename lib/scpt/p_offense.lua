-- handle the holy offense school

HCURSE_I = add_spell {
	["name"] = 	"Curse I",
	["name2"] = 	"Curse I",
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	1,
	["mana"] = 	2,
	["mana_max"] = 	2,
	["fail"] = 	10,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			fire_grid_bolt(Ind, GF_CURSE, args.dir, 10 + get_level(Ind, HCURSE_I, 150), "points and curses for")
	end,
	["info"] = 	function()
			return "power "..(10 + get_level(Ind, HCURSE_I, 150))
	end,
	["desc"] = 	{ "Randomly causes confusion damage, slowness or blindness.", }
}
HCURSE_II = add_spell {
	["name"] = 	"Curse II",
	["name2"] = 	"Curse II",
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	16,
	["mana"] = 	5,
	["mana_max"] = 	5,
	["fail"] = 	-15,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			fire_beam(Ind, GF_CURSE, args.dir, 10 + get_level(Ind, HCURSE_I, 150), "points and curses for")
	end,
	["info"] = 	function()
			return "power "..(10 + get_level(Ind, HCURSE_I, 150))
	end,
	["desc"] = 	{
			"Randomly causes confusion damage, slowness or blindness",
			"in a line, passing through monsters.",
	}
}
HCURSE_III = add_spell {
	["name"] = 	"Curse III",
	["name2"] = 	"Curse III",
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	26,
	["mana"] = 	12,
	["mana_max"] = 	12,
	["fail"] = 	-45,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function(args)
			project_los(Ind, GF_CURSE, 10 + get_level(Ind, HCURSE_I, 150), "points and curses for")
	end,
	["info"] = 	function()
			return "power "..(10 + get_level(Ind, HCURSE_I, 150))
	end,
	["desc"] = 	{
			"Randomly causes confusion damage, slowness or blindness",
			"on all monsters within your field of view.",
	}
}

HGLOBELIGHT_I = add_spell {
	["name"] = 	"Call Light I",
	["name2"] = 	"CLight I",
	["school"] = 	{SCHOOL_HOFFENSE, SCHOOL_HSUPPORT},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	2,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	10,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
		if get_level(Ind, HGLOBELIGHT_I, 50) >= 3 then
			lite_area(Ind, 19 + get_level(Ind, HGLOBELIGHT_I, 50), 3)
		else
			msg_print(Ind, "You are surrounded by a white light.")
			lite_room(Ind, player.wpos, player.py, player.px)
		end
	end,
	["info"] = 	function()
		if get_level(Ind, HGLOBELIGHT_I, 50) >= 3 then
			return "dam "..((19 + get_level(Ind, HGLOBELIGHT_I, 50)) / 2).." rad 3"
		else
			return ""
		end
	end,
	["desc"] = 	{
			"Creates a globe of pure light.",
			"At level 3 it hurts monsters that are susceptible to light.",
	}
}
HGLOBELIGHT_II = add_spell {
	["name"] = 	"Call Light II",
	["name2"] = 	"CLight II",
	["school"] = 	{SCHOOL_HOFFENSE, SCHOOL_HSUPPORT},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	20,
	["mana"] = 	8,
	["mana_max"] = 	8,
	["fail"] = 	-25,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			msg_print(Ind, "You are surrounded by a white light.")
			lite_room(Ind, player.wpos, player.py, player.px)
			fire_ball(Ind, GF_LITE, 0, (10 + get_level(Ind, HGLOBELIGHT_I, 100)) * 2, 1 + get_level(Ind, HGLOBELIGHT_I, 6), " calls light for")
	end,
	["info"] = 	function()
			return "dam "..(10 + get_level(Ind, HGLOBELIGHT_I, 100)).." rad "..(1 + get_level(Ind, HGLOBELIGHT_I, 6))
	end,
	["desc"] = 	{ "Creates a powerful globe of pure light that hurts all foes.", }
}

function get_literay_dam(Ind, limit_lev)
	local lev

	lev = get_level(Ind, HLITERAY, 50)
	if limit_lev ~= 0 and lev > limit_lev then lev = limit_lev + (lev - limit_lev) / 3 end

	return 5 + ((lev * 2) / 5), 7 + ((lev * 3) / 3) + 1
end
HLITERAY = add_spell {
	["name"] = 	"Ray of Light",
	["name2"] = 	"Ray",
	["school"] = 	SCHOOL_HOFFENSE,
	["spell_power"] = 0,
	["level"] = 	18,
	["mana"] = 	10,
	["mana_max"] = 	10,
	["fail"] = 	-30,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["ftk"] = 1,
	["spell"] = 	function(args)
			fire_beam(Ind, GF_LITE, args.dir, damroll(get_literay_dam(Ind, 15)), " casts a ray of light for")
	end,
	["info"] = 	function()
			local x, y

			x, y = get_literay_dam(Ind, 15)
			return "dam "..x.."d"..y
	end,
	["desc"] = 	{ "Conjures up holy light into a powerful ray.", }
}

HORBDRAIN_I = add_spell {
	["name"] = 	"Orb of Draining I",
	["name2"] = 	"OoD I",
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	20,
	["mana"] = 	10,
	["mana_max"] = 	10,
	["fail"] = 	10,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["ftk"] = 2,
	["spell"] = 	function(args)
		local typ

		typ = GF_HOLY_ORB
		fire_ball(Ind, typ, args.dir, 20 + get_level(Ind, HORBDRAIN_I, 300), 2 + get_level(Ind, HORBDRAIN_I, 3), " casts a holy orb for")
	end,
	["info"] = 	function()
		return "dam "..(20 + get_level(Ind, HORBDRAIN_I, 300)).." rad "..(2 + get_level(Ind, HORBDRAIN_I, 3))
	end,
	["desc"] = 	{ "Calls an holy orb to devour the evil.", }
}
HORBDRAIN_II = add_spell {
	["name"] = 	"Orb of Draining II",
	["name2"] = 	"OoD II",
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	40,
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	-90,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["ftk"] = 2,
	["spell"] = 	function(args)
		local typ

		typ = GF_HOLY_ORB
		fire_ball(Ind, typ, args.dir, 20 + get_level(Ind, HORBDRAIN_I, 560), 2 + get_level(Ind, HORBDRAIN_I, 3), " casts a holy orb for")
	end,
	["info"] = 	function()
		return "dam "..(20 + get_level(Ind, HORBDRAIN_I, 560)).." rad "..(2 + get_level(Ind, HORBDRAIN_I, 3))
	end,
	["desc"] = 	{ "Calls an holy orb to devour the evil.", }
}

HEXORCISM_I = add_spell {
	["name"] = 	"Exorcism I",
	["name2"] = 	"Exo I",
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	20,
	["mana"] = 	25,
	["mana_max"] = 	25,
	["fail"] = 	-15,
	["stat"] = 	A_WIS,
	["spell"] = 	function(args)
			dispel_demons(Ind, 50 + get_level(Ind, HEXORCISM_I, 600))
	end,
	["info"] = 	function()
		return "dam "..(50 + get_level(Ind, HEXORCISM_I, 600))
	end,
	["desc"] = 	{ "Dispels nearby demons.", }
}
HEXORCISM_II = add_spell {
	["name"] = 	"Exorcism II",
	["name2"] = 	"Exo II",
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	40,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	-90,
	["stat"] = 	A_WIS,
	["spell"] = 	function(args)
			dispel_demons(Ind, 50 + get_level(Ind, HEXORCISM_I, 1423))
	end,
	["info"] = 	function()
		return "dam "..(50 + get_level(Ind, HEXORCISM_I, 1423))
	end,
	["desc"] = 	{ "Dispels nearby demons.", }
}

HRELSOULS_I = add_spell {
	["name"] = 	"Redemption I",
	["name2"] = 	"Red I",
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	10,
	["mana"] = 	10,
	["mana_max"] = 	10,
	["fail"] = 	25,
	["stat"] = 	A_WIS,
	["spell"] = 	function(args)
			dispel_undead(Ind, 10 + get_level(Ind, HRELSOULS_I, 275))
			end,
	["info"] = 	function()
		return "dam "..(10 + get_level(Ind, HRELSOULS_I, 275))
	end,
	["desc"] = 	{ "Banishes nearby undead.", }
}
HRELSOULS_II = add_spell {
	["name"] = 	"Redemption II",
	["name2"] = 	"Red II",
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	25,
	["mana"] = 	25,
	["mana_max"] = 	25,
	["fail"] = 	-40,
	["stat"] = 	A_WIS,
	["spell"] = 	function(args)
			dispel_undead(Ind, 10 + get_level(Ind, HRELSOULS_I, 550))
			end,
	["info"] = 	function()
		return "dam "..(10 + get_level(Ind, HRELSOULS_I, 550))
	end,
	["desc"] = 	{ "Banishes nearby undead.", }
}
HRELSOULS_III = add_spell {
	["name"] = 	"Redemption III",
	["name2"] = 	"Red III",
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	40,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	-85,
	["stat"] = 	A_WIS,
	["spell"] = 	function(args)
			dispel_undead(Ind, 10 + get_level(Ind, HRELSOULS_I, 1125))
			end,
	["info"] = 	function()
		return "dam "..(10 + get_level(Ind, HRELSOULS_I, 1125))
	end,
	["desc"] = 	{ "Banishes nearby undead.", }
}

HDRAINCLOUD = add_spell {
	["name"] = 	"Doomed Grounds",
	["name2"] = 	"Doom",
	["school"] = 	{SCHOOL_HOFFENSE, SCHOOL_OSHADOW},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	40,     -- pointless for crap with low lvl anyway
	["mana"] = 	65,
	["mana_max"] = 	65,
	["fail"] = 	-35,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			fire_cloud(Ind, GF_ANNIHILATION, args.dir, 3, 3, 5 + get_level(Ind, HDRAINCLOUD, 39) / 4, 10, " damages for")
			-- HUGE note about this spell: it can NOT kill a monster!
			--                                                      -the_sandman
	end,
	["info"] = 	function()
			return "dam 3% rad 3 dur "..(5 + get_level(Ind, HDRAINCLOUD, 39) / 4)
	end,
	["desc"] = 	{
			"Curses an area temporarily, damaging those walking across.",
			--"15% of the damage is fed back to the caster's hit points."
	}
}

EARTHQUAKE = add_spell {
	["name"] = 	"Earthquake",
	["name2"] = 	"Quake",
	["school"] = 	{SCHOOL_HOFFENSE},
	["level"] = 	42,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	-50,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			earthquake(player.wpos, player.py, player.px, 4 + get_level(Ind, EARTHQUAKE, 20));
	end,
	["info"] = 	function()
			return "rad "..(4 + get_level(Ind, SHAKE, 20))
	end,
	["desc"] = 	{ "Creates a localized earthquake." }
}

--HHOLYWORD
