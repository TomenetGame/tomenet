-- handle the holy offense school

HCURSE_I = add_spell {
	["name"] = 	"Curse I",
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
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	16,
	["mana"] = 	8,
	["mana_max"] = 	8,
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
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	26,
	["mana"] = 	30,
	["mana_max"] = 	30,
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
	["name"] = 	"Holy Light I",
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
			lite_area(Ind, 10, 4)
		else
			msg_print(Ind, "You are surrounded by a white light")
			lite_room(Ind, player.wpos, player.py, player.px)
		end
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{
			"Creates a globe of pure light.",
			"At level 3 it hurts monsters that are susceptible to light.",
	}
}
HGLOBELIGHT_II = add_spell {
	["name"] = 	"Holy Light II",
	["school"] = 	{SCHOOL_HOFFENSE, SCHOOL_HSUPPORT},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	20,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	-25,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			msg_print(Ind, "You are surrounded by a white light")
			lite_room(Ind, player.wpos, player.py, player.px)
			fire_ball(Ind, GF_LITE, 0, (10 + get_level(Ind, HGLOBELIGHT_I, 100)) * 2, 5 + get_level(Ind, HGLOBELIGHT_I, 6), " calls light for")
	end,
	["info"] = 	function()
			return "dam "..(10 + get_level(Ind, HGLOBELIGHT_I, 100)).." rad "..(5 + get_level(Ind, HGLOBELIGHT_I, 6))
	end,
	["desc"] = 	{ "Creates a powerful globe of pure light that hurts all foes.", }
}

if (def_hack("TEST_SERVER", nil)) then
HCURSEDD = add_spell {
	["name"] = 	"Cause wounds",
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	5,
	["mana"] = 	1,
	["mana_max"] = 	20,
	["fail"] = 	15,
	["stat"] = 	A_WIS,
	["ftk"] = 1,
	["spell"] = 	function(args)
			fire_grid_bolt(Ind, GF_MISSILE, args.dir, 10 + get_level(Ind, HCURSEDD, 300), "points and curses for")
	end,
	["info"] = 	function()
			return "power "..(10 + get_level(Ind, HCURSEDD, 300))
	end,
	["desc"] = 	{ "Curse an enemy, causing wounds.", }
}
end

HORBDRAIN_I = add_spell {
	["name"] = 	"Orb of Draining I",
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	20,
	["mana"] = 	7,
	["mana_max"] = 	7,
	["fail"] = 	20,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["ftk"] = 2,
	["spell"] = 	function(args)
		local typ
		typ = GF_HOLY_ORB
		fire_ball(Ind, typ, args.dir, 20 + get_level(Ind, HORBDRAIN_I, 350), 2 + get_level(Ind, HORBDRAIN_I, 3), " casts a holy orb for")
	end,
	["info"] = 	function()
		return "dam "..(20 + get_level(Ind, HORBDRAIN_I, 350)).." rad "..(2 + get_level(Ind, HORBDRAIN_I, 3))
	end,
	["desc"] = 	{ "Calls an holy orb to devour the evil.", }
}
HORBDRAIN_II = add_spell {
	["name"] = 	"Orb of Draining II",
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
		fire_ball(Ind, typ, args.dir, 20 + get_level(Ind, HORBDRAIN_I, 660), 2 + get_level(Ind, HORBDRAIN_I, 3), " casts a holy orb for")
	end,
	["info"] = 	function()
		return "dam "..(20 + get_level(Ind, HORBDRAIN_I, 660)).." rad "..(2 + get_level(Ind, HORBDRAIN_I, 3))
	end,
	["desc"] = 	{ "Calls an holy orb to devour the evil.", }
}

HEXORCISM_I = add_spell {
	["name"] = 	"Exorcism I",
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	20,
	["mana"] = 	25,
	["mana_max"] = 	25,
	["fail"] = 	15,
	["stat"] = 	A_WIS,
	["spell"] = 	function(args)
			dispel_demons(Ind, 50 + get_level(Ind, HEXORCISM_I, 400))
	end,
	["info"] = 	function()
		return "dam "..(50 + get_level(Ind, HEXORCISM_I, 400))
	end,
	["desc"] = 	{ "Dispels nearby demons.", }
}
HEXORCISM_II = add_spell {
	["name"] = 	"Exorcism II",
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	40,
	["mana"] = 	70,
	["mana_max"] = 	70,
	["fail"] = 	-90,
	["stat"] = 	A_WIS,
	["spell"] = 	function(args)
			dispel_demons(Ind, 50 + get_level(Ind, HEXORCISM_I, 800))
	end,
	["info"] = 	function()
		return "dam "..(50 + get_level(Ind, HEXORCISM_I, 800))
	end,
	["desc"] = 	{ "Dispels nearby demons.", }
}

HDRAINLIFE = add_spell {
	["name"] = 	"Drain Life",
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	37,
	["mana"] = 	45,
	["mana_max"] = 	45,
	["fail"] = 	-20,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		drain_life(Ind, args.dir, 15)
		hp_player(Ind, player.ret_dam / 4)
	end,
	["info"] = 	function()
		return "drain 15%, heal for 25%"
	end,
	["desc"] = 	{ "Drains life from a target, which must not be non-living or undead.", }
}

HRELSOULS_I = add_spell {
	["name"] = 	"Redemption I",
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	10,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	25,
	["stat"] = 	A_WIS,
	["spell"] = 	function(args)
			dispel_undead(Ind, 10 + get_level(Ind, HRELSOULS_I, 200))
			end,
	["info"] = 	function()
		return "dam "..(10 + get_level(Ind, HRELSOULS_I, 200))
	end,
	["desc"] = 	{ "Banishes nearby undead.", }
}
HRELSOULS_II = add_spell {
	["name"] = 	"Redemption II",
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	25,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	-40,
	["stat"] = 	A_WIS,
	["spell"] = 	function(args)
			dispel_undead(Ind, 10 + get_level(Ind, HRELSOULS_I, 400))
			end,
	["info"] = 	function()
		return "dam "..(10 + get_level(Ind, HRELSOULS_I, 400))
	end,
	["desc"] = 	{ "Banishes nearby undead.", }
}
HRELSOULS_III = add_spell {
	["name"] = 	"Redemption III",
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	40,
	["mana"] = 	70,
	["mana_max"] = 	70,
	["fail"] = 	-85,
	["stat"] = 	A_WIS,
	["spell"] = 	function(args)
			dispel_undead(Ind, 10 + get_level(Ind, HRELSOULS_I, 700))
			end,
	["info"] = 	function()
		return "dam "..(10 + get_level(Ind, HRELSOULS_I, 700))
	end,
	["desc"] = 	{ "Banishes nearby undead.", }
}

HDRAINCLOUD = add_spell {
	["name"] = 	"Doomed Grounds",
	["school"] = 	{SCHOOL_HOFFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	40,     -- pointless for crap with low lvl anyway
	["mana"] = 	65,
	["mana_max"] = 	65,
	["fail"] = 	-35,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["spell"] = 	function(args)
--			fire_cloud(Ind, GF_OLD_DRAIN, args.dir, 9999, 3, 8 + get_level(Ind, HDRAINCLOUD, 10), 10, " drains for")
			fire_cloud(Ind, GF_OLD_DRAIN, args.dir, 9999, 3, 4 + get_level(Ind, HDRAINCLOUD, 39) / 4, 10, " drains for")
			-- dmgs a Power D for 2050 (307 goes to hp), Balance D for 1286 (192 goes to hp) from full hp
			-- (with, of course, maxed spell power and h_offense schools)
			-- The amount of what goes to player is 15% of the damage the monster taken.
			-- 9999 is a hack handled in spells1.c. Sorry for this. For the moment this
			-- the only possible way (i think) to do this spell without affecting the
			-- current ability of wands and artifacts (that can be activated) that uses
			-- GF_OLD_DRAIN as well. Once I get to know my way around better, I'll fix this.
			-- By the way, the real value there is 2.
			-- HUGE note about this spell: it can NOT kill a monster!
			--                                                      -the_sandman
	end,
	["info"] = 	function()
			return "dam ".."var".." rad 3 dur "..(4 + get_level(Ind, HDRAINCLOUD, 39) / 4)
	end,
	["desc"] = 	{
			"Curses an area temporarily, sucking life force of those walking it.",
			"15% of the damage is fed back to the caster's hit points."
	}
}

--[[
HHOLYWORD = add_spell {
	["name"] = 	"Holy Word",
	["school"] = 	{SCHOOL_HOFFENSE, SCHOOL_HCURING},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	45,
	["mana"] = 	500,
	["fail"] = 	30,
	["stat"] = 	A_WIS,
	["spell"] = 	function(args)
			hp_player(Ind, 1000)
			set_afraid(Ind, 0)
			set_poisoned(Ind, 0, 0)
			set_stun(Ind, 0, 0)
			set_cut(Ind, 0, 0)
			end,
	["info"] = 	function()
			return "Dispels & heals."
			end,
	["desc"] = 	{ "Dispels evil, heals and cures you." }
}
]]
