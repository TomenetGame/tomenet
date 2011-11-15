-- handle the mind school
--blind/paralysis? wouldn't probably make noticable diff from conf/sleep!
--((static: res insanity/effects))

function get_psiblast_dam()
--	return 4 + get_level(Ind, MMINDBLAST, 4), 3 + get_level(Ind, MMINDBLAST, 45) <- just 50% of targetted value
--	return 2 + get_level(Ind, MMINDBLAST, 6), 3 + get_level(Ind, MMINDBLAST, 45), get_level(Ind, MMINDBLAST, 200)
	return 2 + get_level(Ind, MMINDBLAST, 7), 3 + get_level(Ind, MMINDBLAST, 45), get_level(Ind, MMINDBLAST, 250)
end

MSCARE = add_spell
{
	["name"] = 	"Scare",
        ["school"] = 	{SCHOOL_MINTRUSION},
        ["am"] = 	50,
        ["spell_power"] = 0,
        ["level"] = 	1,
        ["mana"] = 	1,
        ["mana_max"] = 	8,
        ["fail"] = 	10,
        ["direction"] = function() if get_level(Ind, MSCARE, 50) >= 20 then return FALSE else return TRUE end end,
        ["spell"] = 	function(args)
                        if get_level(Ind, MSCARE, 50) >= 20 then
				project_los(Ind, GF_TURN_ALL, 10 + get_level(Ind, MSCARE, 50), "stares deep into your eyes")
                        elseif get_level(Ind, MSCARE, 50) >= 10 then
                                fire_ball(Ind, GF_TURN_ALL, args.dir, 10 + get_level(Ind, MSCARE, 50), 3, "stares deep into your eyes")
                        else
                                fire_grid_bolt(Ind, GF_TURN_ALL, args.dir, 10 + get_level(Ind, MSCARE, 50), "stares deep into your eyes")
                        end
			end,
	["info"] = 	function()
                	return "power "..(10 + get_level(Ind, MSCARE, 50))
			end,
        ["desc"] =	{
                        "Tries to manipulate the mind of a monster to scare it",
                        "At level 10 it turns into a ball",
                        "At level 20 it affects all monsters in sight",
        }
}
__lua_MSCARE = MSCARE

MCONFUSE = add_spell
{
	["name"] = 	"Confuse",
        ["school"] = 	{SCHOOL_MINTRUSION},
        ["am"] = 	50,
        ["spell_power"] = 0,
        ["level"] = 	3,
        ["mana"] = 	3,
        ["mana_max"] = 	20,
        ["fail"] = 	10,
        ["direction"] = function() if get_level(Ind, MCONFUSE, 50) >= 30 then return FALSE else return TRUE end end,
        ["spell"] = 	function(args)
                        if get_level(Ind, MCONFUSE, 50) >= 30 then
                                project_los(Ind, GF_OLD_CONF, 10 + get_level(Ind, MCONFUSE, 150), "focusses on your mind")
                        elseif get_level(Ind, MCONFUSE, 50) >= 15 then
                                fire_ball(Ind, GF_OLD_CONF, args.dir, 10 + get_level(Ind, MCONFUSE, 150), 3, "focusses on your mind")
                        else
                                fire_grid_bolt(Ind, GF_OLD_CONF, args.dir, 10 + get_level(Ind, MCONFUSE, 150), "focusses on your mind")
                        end
			end,
	["info"] = 	function()
                	return "power "..(10 + get_level(Ind, MCONFUSE, 150))
			end,
        ["desc"] =	{
        		"Tries to manipulate the mind of a monster to confuse it",
                        "At level 15 it turns into a ball",
                        "At level 30 it affects all monsters in sight",
        }
}

MSLEEP = add_spell
{
	["name"] =	"Hypnosis",
	["school"] =	{SCHOOL_MINTRUSION},
        ["am"] = 	33,
        ["spell_power"] = 0,
	["level"] =	5,
	["mana"] =	5,
	["mana_max"] =	30,
	["fail"] =	10,
        ["direction"] = function() if get_level(Ind, MSLEEP, 50) >= 20 then return FALSE else return TRUE end end,
	["spell"] =	function(args)
			if get_level(Ind, MSLEEP, 50) < 20 then
--				project(0 - Ind, get_level(Ind, MSLEEP, 10), player.wpos, player.py, player.px, (3 + get_level(Ind, MSLEEP, 30)) * 2, GF_OLD_SLEEP, 64+16+8, "mumbles softly")
--				fire_grid_bolt(Ind, GF_OLD_SLEEP, args.dir, 3 + get_level(Ind, MSLEEP, 25), "mumbles softly")
				fire_grid_bolt(Ind, GF_OLD_SLEEP, args.dir, 10 + get_level(Ind, MSLEEP, 100), "mumbles softly")
			else
--				project_los(Ind, GF_OLD_SLEEP, 3 + get_level(Ind, MSLEEP, 25), "mumbles softly")
				project_los(Ind, GF_OLD_SLEEP, 10 + get_level(Ind, MSLEEP, 100), "mumbles softly")
			end
			end,
	["info"] =	function()
--			if get_level(Ind, MSLEEP, 50) < 20 then
--				return "dur "..(3 + get_level(Ind, MSLEEP, 30)).." rad "..get_level(Ind, MSLEEP, 10)
--			else
				return "dur "..(3 + get_level(Ind, MSLEEP, 25))
--			end
			end,
	["desc"] = {
			"Causes the target to fall asleep instantly",
--			"Lets monsters next to you fall asleep",
			"At level 20 it lets all nearby monsters fall asleep",
	}
}

MSLOWMONSTER = add_spell
{
        ["name"] =	"Drain Strength",
        ["school"] =	{SCHOOL_MINTRUSION},
        ["am"] = 	50,
        ["spell_power"] = 0,
        ["level"] =	7,
        ["mana"] =	10,
        ["mana_max"] =	30,
        ["fail"] =	10,
        ["direction"] = function() if get_level(Ind, MSLOWMONSTER, 50) >= 20 then return FALSE else return TRUE end end,
        ["spell"] =	function(args)
                        if get_level(Ind, MSLOWMONSTER, 50) >= 20 then
				project_los(Ind, GF_OLD_SLOW, 40 + get_level(Ind, MSLOWMONSTER, 160), "drains power from your muscles")
                        elseif get_level(Ind, MSLOWMONSTER, 50) >= 10 then
				fire_ball(Ind, GF_OLD_SLOW, args.dir, 40 + get_level(Ind, MSLOWMONSTER, 160), 2, "drains power from your muscles")
			else
                                fire_grid_bolt(Ind, GF_OLD_SLOW, args.dir, 40 + get_level(Ind, MSLOWMONSTER, 160), "drains power from your muscles")
                        end
                        end,
        ["info"] =	function()
			if get_level(Ind, MSLOWMONSTER, 50) >= 20 then
				return "power "..(40 + get_level(Ind, MSLOWMONSTER, 160))
			elseif get_level(Ind, MSLOWMONSTER, 50) >= 10 then
				return "power "..(40 + get_level(Ind, MSLOWMONSTER, 160)).." rad 2"
			else
				return "power "..(40 + get_level(Ind, MSLOWMONSTER, 160))
			end
                        end,
        ["desc"] =	{
                        "Drains power from the muscles of your opponent, slowing it down",
                        "At level 10 it turns into a ball",
                        "At level 20 it affects all monsters in sight",
        }
}

MMINDBLAST = add_spell
{
	["name"] = 	"Psionic Blast",
        ["school"] = 	{SCHOOL_MINTRUSION},
        ["am"] = 	50,
        ["spell_power"] = 0,
        ["level"] = 	1,
        ["mana"] = 	1,
        ["mana_max"] = 	15,
        ["fail"] = 	10,
        ["direction"] = TRUE,
        ["ftk"] = 2,
        ["spell"] = 	function(args)
    			local d, s, p
    			d, s, p = get_psiblast_dam()
                        fire_grid_bolt(Ind, GF_PSI, args.dir, damroll(d, s) + p, "")
			end,
	["info"] = 	function()
    			local d, s, p
    			d, s, p = get_psiblast_dam()
                	return "power "..d.."d"..s.."+"..p
			end,
        ["desc"] =	{
                        "Blasts the target's mind with psionic energy",
        }
}

MPSISTORM = add_spell
{
	["name"] = "Psi Storm",
	["school"] = {SCHOOL_MINTRUSION},
	["am"] = 50,
	["spell_power"] = 0,
	["level"] = 18,
	["mana"] = 15,
	["mana_max"] = 40,
	["fail"] = 25,
	["direction"] = TRUE,
	["spell"] = function(args)
--        ["spell"] = function()
		fire_cloud(Ind, GF_PSI, args.dir, (1 + get_level(Ind, MPSISTORM, 76)), 2 + get_level(Ind, MPSISTORM, 2), 5 + get_level(Ind, MPSISTORM, 5), 14, " releases a psi storm for")
--		fire_cloud(Ind, GF_PSI, 0, (1 + get_level(Ind, MPSISTORM, 76)), 2 + get_level(Ind, MPSISTORM, 4), 5 + get_level(Ind, MPSISTORM, 5), 14, " releases a psi storm for")
--                        fire_wave(Ind, GF_PSI, 0, (1 + get_level(Ind, MPSISTORM, 76)), 2 + get_level(Ind, MPSISTORM, 4), 5 + get_level(Ind, MPSISTORM, 5), 14, EFF_STORM, " releases a psi storm for
        end,
        ["info"] = function()
                return "dam "..(1 + get_level(Ind, MPSISTORM, 76)).." rad "..(2 + get_level(Ind, MPSISTORM, 2)).." dur "..(5 + get_level(Ind, MPSISTORM, 5))
        end,
        ["desc"] = {
                "A psionic storm that damages and disturbs all minds within an area",
        }
}

MSILENCE = add_spell
{
	["name"] = 	"Psychic Suppression",
        ["school"] = 	{SCHOOL_MINTRUSION},
        ["am"] = 	50,
        ["spell_power"] = 0,
        ["level"] = 	10,
        ["mana"] = 	50,
        ["mana_max"] = 	100,
        ["fail"] = 	10,
        ["direction"] = TRUE,
        ["spell"] = 	function(args)
    			--using this hack to transport 2 parameters at once,
    			--ok since we use a single-target spell and not a ball
                        fire_grid_bolt(Ind, GF_SILENCE, args.dir, get_level(Ind, MSILENCE, 63) + ((4 + get_level(Ind, MSILENCE, 4)) * 100), "")
			end,
	["info"] = 	function()
                	return "power "..(get_level(Ind, MSILENCE, 63)).." dur "..(4 + get_level(Ind, MSILENCE, 4))
			end,
        ["desc"] =	{
                        "Drains the target's psychic energy, impacting its ability to cast spells",
        }
}

MMAP = add_spell
{
        ["name"] =	"Remote Vision",
        ["school"] =	{SCHOOL_MINTRUSION},
--	["school"] =	{SCHOOL_MINTRUSION, SCHOOL_TCONTACT}
        ["am"] = 	50,
        ["spell_power"] = 0,
        ["level"] =	20,
        ["mana"] =	30,
        ["mana_max"] =	30,
        ["fail"] =	10,
        ["direction"] = FALSE,
        ["spell"] =	function()
			mind_map_level(Ind)
                        end,
        ["info"] =	function()
			return ""
                        end,
        ["desc"] =	{
                        "Forcefully uses the vision of sentient life forms around.",
                        "*** Will be transferred to allied open",
                        "    minds on the same floor if your",
                        "    Thought Contact skill is at least 20. ***",
        }
}

--[[ Old version, requiring pets. Not cool though. See new variant below!
MCHARM = add_spell
{
	["name"] = 	"Charm",
        ["school"] = 	{SCHOOL_MINTRUSION},
        ["am"] = 	50,
        ["spell_power"] = 0,
        ["level"] = 	1,
        ["mana"] = 	1,
        ["mana_max"] = 	20,
        ["fail"] = 	10,
        ["direction"] = function () if get_level(Ind, CHARM, 50) >= 35 then return FALSE else return TRUE end end,
        ["spell"] = 	function(args)
                        if get_level(Ind, CHARM, 50) >= 35 then
                                project_los(Ind, GF_CHARM, 10 + get_level(Ind, CHARM, 150), "mumbles softly")
                        elseif get_level(Ind, CHARM, 50) >= 15 then
                                fire_ball(Ind, GF_CHARM, args.dir, 10 + get_level(Ind, CHARM, 150), 3, "mumbles softly")
                        else
                                fire_bolt(Ind, GF_CHARM, args.dir, 10 + get_level(Ind, CHARM, 150), "mumbles softly")
                        end

	end,
	["info"] = 	function()
                	return "power "..(10 + get_level(Ind, CHARM, 150))
	end,
        ["desc"] =	{
        		"Tries to manipulate the mind of a monster to make it friendly",
                        "At level 15 it turns into a ball",
                        "At level 35 it affects all monsters in sight",
        }
}
]]

-- New idea: works like *invincibility*: monsters will ignore you (and often your party members too ;)
MCHARM = add_spell
{
	["name"] = 	"Charm",
        ["school"] = 	{SCHOOL_MINTRUSION},
        ["am"] = 	50,
        ["spell_power"] = 0,
        ["level"] = 	33,
        ["mana"] = 	10,
        ["mana_max"] = 	10,
        ["fail"] = 	10,
        ["direction"] = function () if get_level(Ind, MCHARM, 50) >= 13 then return FALSE else return TRUE end end,
        ["spell"] = 	function(args)
			--reset previous charm spell first:
			do_mstopcharm(Ind)
			--cast charm!
                        if get_level(Ind, MCHARM, 50) >= 13 then
                                project_los(Ind, GF_CHARMIGNORE, 10 + get_level(Ind, MCHARM, 150), "focusses")
                        elseif get_level(Ind, MCHARM, 50) >= 7 then
                                fire_ball(Ind, GF_CHARMIGNORE, args.dir, 10 + get_level(Ind, MCHARM, 150), 3, "focusses")
                        else
                                fire_bolt(Ind, GF_CHARMIGNORE, args.dir, 10 + get_level(Ind, MCHARM, 150), "focusses")
                        end
	end,
	["info"] = 	function()
--                	return "power "..(10 + get_level(Ind, MCHARM, 150))
			return ""
	end,
        ["desc"] =	{
        		"Tries to manipulate the mind of a monster to make it ignore you",
                        "At level 7 it turns into a ball",
                        "At level 13 it affects all monsters in sight",
        }
}
MSTOPCHARM = add_spell
{
	["name"] = 	"Stop Charm",
        ["school"] = 	{SCHOOL_MINTRUSION},
        ["am"] = 	0,
        ["spell_power"] = 0,
        ["level"] = 	33,
        ["mana"] = 	0,
        ["mana_max"] = 	0,
        ["fail"] = 	-99,
        ["direction"] = FALSE,
        ["spell"] = 	function()
			do_mstopcharm(Ind)
	end,
	["info"] = 	function()
			return ""
	end,
        ["desc"] =	{
        		"Cancel charming of any monsters",
        }
}
