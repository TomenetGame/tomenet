-- handle the conveyance school

BLINK = add_spell
{
	["name"] = 	"Phase Door",
        ["school"] = 	{SCHOOL_CONVEYANCE},
        ["level"] = 	1,
        ["mana"] = 	1,
        ["mana_max"] =  3,
        ["fail"] = 	10,
        ["spell"] = 	function()
                	local dist = 10 + get_level(Ind, BLINK, 8)
			teleport_player(Ind, dist)
                        if player.spell_project > 0 then
                                fire_ball(Ind, GF_TELEPORT_PLAYER, 0, dist, player.spell_project)
                        end
	end,
	["info"] = 	function()
                	return "distance "..(10 + get_level(Ind, BLINK, 8))
	end,
        ["desc"] =	{
        		"Teleports you on a small scale range",
                        "***Affected by the Meta spell: Project Spell***",
        },
}

DISARM = add_spell
{
	["name"] = 	"Disarm",
        ["school"] = 	{SCHOOL_CONVEYANCE},
        ["level"] = 	3,
        ["mana"] = 	2,
        ["mana_max"] = 	4,
        ["fail"] = 	10,
        ["spell"] = 	function()
			destroy_doors_touch(Ind, 1 + get_level(Ind, DISARM, 4, 0))
	end,
	["info"] = 	function()
                	return "rad "..(1 + get_level(Ind, DISARM, 4, 0))
	end,
        ["desc"] =	{
        		"Destroys doors and traps",
        }
}

TELEPORT = add_spell
{
	["name"] = 	"Teleportation",
        ["school"] = 	{SCHOOL_CONVEYANCE},
        ["level"] = 	10,
        ["mana"] = 	8,
        ["mana_max"] = 	14,
        ["fail"] = 	50,
        ["spell"] = 	function()
                        local dist = 100 + get_level(Ind, TELEPORT, 100)
			teleport_player(Ind, dist)
                        if player.spell_project > 0 then
                                fire_ball(Ind, GF_TELEPORT_PLAYER, 0, dist, player.spell_project)
                        end
	end,
	["info"] = 	function()
        		return ""
	end,
        ["desc"] =	{
        		"Teleports you around the level. The casting time decreases with level",
                        "***Affected by the Meta spell: Project Spell***",
        }
}

TELEAWAY = add_spell
{
	["name"] = 	"Teleport Away",
        ["school"] = 	{SCHOOL_CONVEYANCE},
        ["level"] = 	23,
        ["mana"] = 	15,
        ["mana_max"] = 	40,
        ["fail"] = 	70,
        ["direction"] = function () if get_level(Ind, TELEAWAY) >= 10 then return FALSE else return TRUE end end,
        ["spell"] = 	function(args)
        		if get_level(Ind, TELEAWAY, 50) >= 20 then
                                project_los(Ind, GF_AWAY_ALL, 100)
                        elseif get_level(Ind, TELEAWAY, 50) >= 10 then
                                fire_ball(Ind, GF_AWAY_ALL, args.dir, 100, 3 + get_level(Ind, TELEAWAY, 4))
                        else
                                teleport_monster(Ind, args.dir)
			end
	end,
	["info"] = 	function()
        		return ""
	end,
        ["desc"] =	{
                        "Teleports a line of monsters away",
                        "At level 10 it turns into a ball",
                        "At level 20 it teleports all monsters in sight"
        }
}

RECALL = add_spell
{
	["name"] = 	"Recall",
        ["school"] = 	{SCHOOL_CONVEYANCE},
        ["level"] = 	30,
        ["mana"] = 	25,
        ["mana_max"] = 	25,
        ["fail"] =      20,
        ["spell"] = 	function(args)
                        local dur = randint(21 - get_level(Ind, RECALL, 15)) + 15 - get_level(Ind, RECALL, 10)

                        if args.book < 0 then return end
        		set_recall(Ind, dur, player.inventory[1 + args.book])
                        if player.spell_project > 0 then
                                fire_ball(Ind, GF_RECALL_PLAYER, 0, dur, player.spell_project)
                        end
	end,
	["info"] = 	function()
			return "dur "..(15 - get_level(Ind, RECALL, 10)).."+d"..(21 - get_level(Ind, RECALL, 15))
	end,
        ["desc"] =	{
        		"Cast on yourself it will recall you to the surface/dungeon.",
                        "***Affected by the Meta spell: Project Spell***",
        }
}

PROBABILITY_TRAVEL = add_spell
{
	["name"] = 	"Probability Travel",
        ["school"] = 	{SCHOOL_CONVEYANCE},
        ["level"] = 	35,
        ["mana"] = 	30,
        ["mana_max"] = 	50,
        ["fail"] = 	50,
        ["spell"] = 	function()
                        set_proba_travel(Ind, randint(10) + get_level(Ind, PROBABILITY_TRAVEL, 60))
	end,
	["info"] = 	function()
        		return "dur "..get_level(Ind, PROBABILITY_TRAVEL, 60).."+d20"
	end,
        ["desc"] =	{
        		"Renders you instable, when you hit a wall you travel throught it and",
                        "instantly appear on the other side of it. You can also float up and down",
                        "at will"
        }
}
