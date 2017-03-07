-- handle the conveyance school

BLINK = add_spell {
	["name"] = 	"Phase Door",
	["school"] = 	{SCHOOL_CONVEYANCE},
	["level"] = 	2,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	10,
	["spell"] = 	function()
			local dist = 6 + get_level(Ind, BLINK, 6)
			teleport_player(Ind, dist, TRUE);
	end,
	["info"] = 	function()
			return "distance "..(6 + get_level(Ind, BLINK, 6))
	end,
	["desc"] = 	{ "Teleports you on a small scale range.", },
}

DISARM = add_spell {
	["name"] = 	"Disarm",
	["school"] = 	{SCHOOL_CONVEYANCE},
	["level"] = 	5,
	["mana"] = 	4,
	["mana_max"] = 	4,
	["fail"] = 	10,
	["spell"] = 	function()
			destroy_traps_touch(Ind, 1 + get_level(Ind, DISARM, 4, 0))
	end,
	["info"] = 	function()
			return "rad "..(1 + get_level(Ind, DISARM, 4, 0))
	end,
	["desc"] = 	{ "Destroys traps and reveals and unlocks doors.", }
}

TELEPORT = add_spell {
	["name"] = 	"Teleportation",
	["school"] = 	{SCHOOL_CONVEYANCE},
	["level"] = 	10,
	["mana"] = 	12,
	["mana_max"] = 	12,
	["fail"] = 	50,
	["spell"] = 	function()
			local dist = 100 + get_level(Ind, TELEPORT, 100)
			teleport_player(Ind, dist, FALSE)
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{ "Teleports you around the level.", }
}

TELEAWAY_I = add_spell {
	["name"] = 	"Teleport Away I",
	["school"] = 	{SCHOOL_CONVEYANCE},
	["level"] = 	23,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	0,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			teleport_monster(Ind, args.dir)
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{ "Teleports a line of monsters away.", }
}
TELEAWAY_II = add_spell {
	["name"] = 	"Teleport Away II",
	["school"] = 	{SCHOOL_CONVEYANCE},
	["level"] = 	43,
	["mana"] = 	40,
	["mana_max"] = 	40,
	["fail"] = 	-60,
	["direction"] = FALSE,
	["spell"] = 	function()
			project_los(Ind, GF_AWAY_ALL, 100, "points and shouts")
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{ "Teleports all monsters in sight away." }
}

RECALL = add_spell {
	["name"] = 	"Recall",
	["school"] = 	{SCHOOL_CONVEYANCE},
	["level"] = 	30,
	["mana"] = 	25,
	["mana_max"] = 	25,
	["fail"] = 	20,
	["spell"] = 	function(args)
			local dur = randint(21 - get_level(Ind, RECALL, 15)) + 15 - get_level(Ind, RECALL, 10)

			if args.book < 0 then return end
			set_recall(Ind, dur, player.inventory[1 + args.book])
			--fire_ball(Ind, GF_RECALL_PLAYER, 0, dur, 1, "")
	end,
	["info"] = 	function()
			return "dur "..(15 - get_level(Ind, RECALL, 10)).."+d"..(21 - get_level(Ind, RECALL, 15))
	end,
	["desc"] = 	{
			"Cast on yourself it will recall you to the surface/dungeon.",
			--"***Automatically projecting***",
	}
}

PROBABILITY_TRAVEL = add_spell {
	["name"] = 	"Probability Travel",
	["school"] = 	{SCHOOL_CONVEYANCE},
	["level"] = 	35,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	20,
	["spell"] = 	function()
			set_prob_travel(Ind, randint(10) + get_level(Ind, PROBABILITY_TRAVEL, 60))
	end,
	["info"] = 	function()
			return "dur "..get_level(Ind, PROBABILITY_TRAVEL, 60).."+d20"
	end,
	["desc"] = 	{
			"Renders you instable, when you hit a wall you travel throught it and",
			"instantly appear on the other side of it. You can also float up and down",
			"at will."
	}
}
