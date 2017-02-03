-- Handles thhe temporal school

MAGELOCK_I = add_spell {
	["name"] = 	"Magelock I",
	["school"] = 	{SCHOOL_TEMPORAL},
	["level"] = 	1,
	["mana"] = 	1,
	["mana_max"] = 	1,
	["fail"] = 	10,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			wizard_lock(Ind, args.dir)
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{ "Magically locks a door.", }
}
MAGELOCK_II = add_spell {
	["name"] = 	"Magelock II",
	["school"] = 	{SCHOOL_TEMPORAL},
	["level"] = 	41,
	["mana"] = 	60,
	["mana_max"] = 	60,
	["fail"] = 	-60,
	["direction"] = FALSE,
	["spell"] = 	function()
			local ret, x, y, c_ptr
			y = player.py
			x = player.px
			warding_glyph(Ind)
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{ "Creates a glyph of warding." }
}

SLOWMONSTER_I = add_spell {
	["name"] = 	"Slow Monster I",
	["school"] = 	{SCHOOL_TEMPORAL},
	["level"] = 	10,
	["mana"] = 	7,
	["mana_max"] = 	7,
	["fail"] = 	10,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			fire_bolt(Ind, GF_OLD_SLOW, args.dir, 5 + get_level(Ind, SLOWMONSTER_I, 100), "")
	end,
	["info"] = 	function()
			return "power "..(5 + get_level(Ind, SLOWMONSTER_I, 100))
	end,
	["desc"] = 	{ "Magically slows down the passing of time around a monster.", }
}
SLOWMONSTER_II = add_spell {
	["name"] = 	"Slow Monster II",
	["school"] = 	{SCHOOL_TEMPORAL},
	["level"] = 	30,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	-30,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			fire_ball(Ind, GF_OLD_SLOW, args.dir, 5 + get_level(Ind, SLOWMONSTER_I, 100), 1, "")
	end,
	["info"] = 	function()
			return "power "..(5 + get_level(Ind, SLOWMONSTER_I, 100)).." rad 1"
	end,
	["desc"] = 	{ "Magically slows down the passing of time in a small zone.", }
}

ESSENSESPEED = add_spell {
	["name"] = 	"Essence of Speed",
	["school"] = 	{SCHOOL_TEMPORAL},
	["level"] = 	15,
	["mana"] = 	35,
	["mana_max"] = 	35,
	["fail"] = 	10,
	["spell"] = 	function()
			local s
			s = 5 + get_level(Ind, ESSENSESPEED, 20)
			if s > 24 then
				s = 24
			end
			if player.pclass ~= CLASS_MAGE then
				if s > 10 then s = 10 end
			end
			fire_ball(Ind, GF_SPEED_PLAYER, 0, s, 2, "")
			set_fast(Ind, 10 + randint(10) + get_level(Ind, ESSENSESPEED, 50), s)
	end,
	["info"] = 	function()
			local s
			s = 5 + get_level(Ind, ESSENSESPEED, 20)
			if s > 24 then
				s = 24
			end
			if player.pclass ~= CLASS_MAGE then
				if s > 10 then s = 10 end
			end
			return "dur "..(10 + get_level(Ind, ESSENSESPEED, 50)).."+d10 speed "..s
	end,
	["desc"] = 	{
			"Magically increases the passing of time around you.",
--			"Istari will see twice the effect others will see.",
			"Non-Istari cannot gain more than +10 speed from this spell.",
			"***Automatically projecting***",
	}
}

MASSWARP = add_spell {
	["name"] = 	"Mass Stasis",
	["school"] = 	{SCHOOL_TEMPORAL, SCHOOL_CONVEYANCE},
	["level"] = 	45,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	-70,
	["spell"] = 	function()
			project_los(Ind, GF_STASIS, 80 + get_level(Ind, MASSWARP, 200), "casts a spell")
	end,
	["info"] = 	function()
			return "power "..(80 + get_level(Ind, MASSWARP, 200))
	end,
	["desc"] = 	{ "Attempts to lock all monsters in your area in a time bubble.", }
}
