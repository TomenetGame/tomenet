-- handle the meta school

RECHARGE_I = add_spell {
	["name"] = 	"Recharge I",
	["school"] = 	{SCHOOL_META},
	["level"] = 	5,
	["mana"] = 	10,
	["mana_max"] = 	10,
	["fail"] = 	20,
	["stat"] = 	A_INT,
	["spell"] = 	function()
			recharge(Ind, 10 + get_level(Ind, RECHARGE_I, 45))
	end,
	["info"] = 	function()
			return "power "..(10 + get_level(Ind, RECHARGE_I, 45))
	end,
	["desc"] = 	{ "Taps on the ambient mana to recharge a wand or staff.", }
}
RECHARGE_II = add_spell {
	["name"] = 	"Recharge II",
	["school"] = 	{SCHOOL_META},
	["level"] = 	25,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	-10,
	["stat"] = 	A_INT,
	["spell"] = 	function()
			recharge(Ind, 10 + 50 + get_level(Ind, RECHARGE_II, 50))
	end,
	["info"] = 	function()
			return "power "..(10 + 50 + get_level(Ind, RECHARGE_II, 50))
	end,
	["desc"] = 	{ "Taps on the ambient mana to recharge a magic device.", }
}
RECHARGE_III = add_spell {
	["name"] = 	"Recharge III",
	["school"] = 	{SCHOOL_META},
	["level"] = 	40,
	["mana"] = 	100,
	["mana_max"] = 	100,
	["fail"] = 	-60,
	["stat"] = 	A_INT,
	["spell"] = 	function()
			recharge(Ind, 10 + get_level(Ind, RECHARGE_III, 140))
	end,
	["info"] = 	function()
			return "power "..(10 + get_level(Ind, RECHARGE_III, 140))
	end,
	["desc"] = 	{ "Taps on the ambient mana to recharge a magic device.", }
}

PROJECT_SPELLS = add_spell {
	["name"] = 	"Project Spells",
	["school"] = 	{SCHOOL_META},
	["level"] = 	10,
	["mana"] = 	0,
	["mana_max"] = 	0,
	["fail"] = 	101,
	["stat"] = 	A_INT,
	["spell"] = 	function()
			if player.spell_project == 0 then
				player.spell_project = 1 + get_level(Ind, PROJECT_SPELLS, 6, 0)
				msg_print(Ind, "Your utility spells will now affect players in a radius of "..(player.spell_project)..".")
			else
				player.spell_project = 0
				msg_print(Ind, "Your utility spells will now only affect yourself.")
			end
			player.redraw = bor(player.redraw, 1048576)
	end,
	["info"] = 	function()
			return "base rad "..(1 + get_level(Ind, PROJECT_SPELLS, 6, 0))
	end,
	["desc"] = 	{
			"Affects some of your spells(mostly utility ones) to make them",
			"have an effect on your nearby party members.",
	}
}

DISPERSEMAGIC = add_spell {
	["name"] = 	"Disperse Magic",
	["school"] = 	{SCHOOL_META},
	["level"] = 	15,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	10,
	["stat"] = 	A_INT,
	-- Unaffected by blindness
	["blind"] = 	FALSE,
	-- Unaffected by confusion
	["confusion"] = FALSE,
	["spell"] = 	function()
			set_blind(Ind, 0)
			set_confused(Ind, 0)
			if get_level(Ind, DISPERSEMAGIC, 50) >= 10 then
				set_image(Ind, 0)
			end
			if get_level(Ind, DISPERSEMAGIC, 50) >= 15 then
				set_slow(Ind, 0)
				set_fast(Ind, 0, 0)
				set_stun(Ind, 0)
			end
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{
			"Dispels a lot of magic that can affect you, be it good or bad.",
			"Level 1: blindness and confusion.",
			"Level 10: hallucination.",
			"Level 15: speed (both bad or good) and stun.",
	}
}

DELCURSES_I = add_spell {
	["name"] = 	"Remove Curses I",
	["school"] = 	SCHOOL_META,
	["level"] = 	15,
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	20,
	["spell"] = 	function()
			done = remove_curse(Ind)
			if done == TRUE then msg_print(Ind, "The curse is broken!") end
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{ "Removes light curses from your items.", }
}
DELCURSES_II = add_spell {
	["name"] = 	"Remove Curses II",
	["school"] = 	SCHOOL_META,
	["level"] = 	40,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	-20,
	["spell"] = 	function()
			remove_all_curse(Ind)
			if done == TRUE then msg_print(Ind, "The curse is broken!") end
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{ "Removes all light and heavy curses from your items.", }
}
