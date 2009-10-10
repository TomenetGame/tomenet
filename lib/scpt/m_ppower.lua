-- handle the psycho-power school
--((static: +saving throw))

MBASH = add_spell
{
	["name"] =	"Psychic Hammer",
        ["school"] =	{SCHOOL_PPOWER},
        ["level"] =	1,
        ["mana"] =	1,
        ["mana_max"] =	15,
        ["fail"] =	5,
        ["direction"] = TRUE,
        ["spell"] =	function(args)
                        fire_grid_bolt(Ind, GF_FORCE, args.dir, get_level(Ind, MBASH, 100), " releases a psychic hammer for")
                        end,
        ["info"] = 	function()
	                return "dam "..(get_level(Ind, MBASH, 100))
                        end,
        ["desc"] =	{
			"Telekinetic power focussed to bash your opponent",
        }
}

MBLINK = add_spell
{
	["name"] = 	"Autokinesis I",
        ["school"] = 	{SCHOOL_PPOWER},
        ["level"] = 	5,
        ["mana"] = 	3,
        ["mana_max"] =  3,
        ["fail"] = 	10,
        ["spell"] = 	function()
                	local dist = 10 + get_level(Ind, MBLINK, 8)
			teleport_player(Ind, dist, TRUE)
			end,
	["info"] = 	function()
                	return "distance "..(10 + get_level(Ind, MBLINK, 8))
			end,
        ["desc"] =	{
        		"Teleports you on a small scale range",
        }
}

MTELEPORT = add_spell
{
	["name"] = 	"Autokinesis II",
        ["school"] = 	{SCHOOL_PPOWER},
        ["level"] = 	16,
        ["mana"] = 	8,
        ["mana_max"] = 	14,
        ["fail"] = 	20,
        ["spell"] = 	function()
                        local dist = 100 + get_level(Ind, MTELEPORT, 100)
			teleport_player(Ind, dist, FALSE)
			end,
	["info"] = 	function()
        		return ""
			end,
        ["desc"] =	{
        		"Teleports you around the level.",
        }
}

MTELETOWARDS = add_spell
{
	["name"] = 	"Autokinesis III",
        ["school"] = 	{SCHOOL_PPOWER},
        ["blind"] = 	0,
        ["level"] = 	24,
        ["mana"] = 	20,
        ["mana_max"] = 	30,
        ["fail"] = 	25,
        ["spell"] = 	function()
			do_autokinesis_to(Ind, 20 + get_level(Ind, MTELETOWARDS, 150))
			end,
	["info"] = 	function()
        		return "range "..(20 + get_level(Ind, MTELETOWARDS, 150))
			end,
        ["desc"] =	{
        		"Teleports you to the nearest friendly opened mind.",
        }
}

MTELEAWAY = add_spell
{
	["name"] = 	"Psychic Warp",
        ["school"] = 	{SCHOOL_PPOWER},
        ["level"] = 	30,
        ["mana"] = 	40,
        ["mana_max"] = 	50,
        ["fail"] = 	30,
        ["direction"] = TRUE,
        ["spell"] = 	function(args)
                        fire_grid_bolt(Ind, GF_AWAY_ALL, args.dir, 40 + get_level(Ind, MTELEAWAY, 160), " teleports you away")
			end,
	["info"] = 	function()
        		return ""
			end,
        ["desc"] =	{
        		"Attempts to teleport your opponent away.",
        }
}

MDISARM = add_spell
{
	["name"] =	"Psychokinesis",
	["school"] =	{SCHOOL_PPOWER},
	["level"] =	3,
	["mana"] =	5,
	["mana_max"] =	5,
	["fail"] =	10,
        ["direction"] = TRUE,
	["spell"] =	function(args)
--			destroy_doors_touch(Ind, 1)
                        fire_grid_beam(Ind, GF_KILL_TRAP, args.dir, 0, "")
			end,
	["info"] =	function()
			return ""
			end,
	["desc"] =	{
			"Destroys traps by psychokinetic manipulation",
	}
}

MPSISTORM = add_spell
{
	["name"] =	"Psi Storm",
        ["school"] =	{SCHOOL_PPOWER},
        ["level"] =	18,
        ["mana"] =	15,
        ["mana_max"] =	40,
        ["fail"] =	25,
        ["direction"] = TRUE,
        ["spell"] =	function(args)
--        ["spell"] =	function()
                        fire_cloud(Ind, GF_PSI, args.dir, (1 + get_level(Ind, MPSISTORM, 76)), 2 + get_level(Ind, MPSISTORM, 2), 5 + get_level(Ind, MPSISTORM, 5), 14, " releases a psi storm for")
--                        fire_cloud(Ind, GF_PSI, 0, (1 + get_level(Ind, MPSISTORM, 76)), 2 + get_level(Ind, MPSISTORM, 4), 5 + get_level(Ind, MPSISTORM, 5), 14, " releases a psi storm for")
--                        fire_wave(Ind, GF_PSI, 0, (1 + get_level(Ind, MPSISTORM, 76)), 2 + get_level(Ind, MPSISTORM, 4), 5 + get_level(Ind, MPSISTORM, 5), 14, EFF_STORM, " releases a psi storm for")
                        end,
        ["info"] = 	function()
	                return "dam "..(1 + get_level(Ind, MPSISTORM, 76)).." rad "..(2 + get_level(Ind, MPSISTORM, 2)).." dur "..(5 + get_level(Ind, MPSISTORM, 5))
                        end,
        ["desc"] =	{
			"A psionic storm that damages and disturbs all minds within an area",
        }
}

MPYROKINESIS = add_spell
{
	["name"] =	"Pyrokinesis",
        ["school"] =	{SCHOOL_PPOWER},
        ["level"] =	20,
        ["mana"] =	5,
        ["mana_max"] =	22,
        ["fail"] =	15,
        ["direction"] = TRUE,
        ["spell"] =	function(args)
                        fire_grid_bolt(Ind, GF_FIRE, args.dir, 130 + get_level(Ind, MPYROKINESIS, 350), " causes an inflammation for")
                        end,
        ["info"] = 	function()
	                return "dam "..(130 + get_level(Ind, MPYROKINESIS, 350))
                        end,
        ["desc"] =	{
			"Causes a severe inflammation to burn your opponent",
        }
}

MCRYOKINESIS = add_spell
{
	["name"] =	"Cryokinesis",
        ["school"] =	{SCHOOL_PPOWER},
        ["level"] =	24,
        ["mana"] =	6,
        ["mana_max"] =	23,
        ["fail"] =	15,
        ["direction"] = TRUE,
        ["spell"] =	function(args)
                        fire_grid_bolt(Ind, GF_COLD, args.dir, 115 + get_level(Ind, MCRYOKINESIS, 360), " causes freezing for")
                        end,
        ["info"] = 	function()
	                return "dam "..(115 + get_level(Ind, MCRYOKINESIS, 360))
                        end,
        ["desc"] =	{
			"Causes a dramatic temperature drop on your opponent",
        }
}

if (def_hack("TEST_SERVER", nil)) then
MFUSION = add_spell
{
	["name"] =      "Mental Fusion",
--	["name"] =      "Corporeal Fusion",
--	["school"] =    {SCHOOL_TCONTACT, SCHOOL_PPOWER, SCHOOL_MINTRUSION},
	["school"] =    {SCHOOL_TCONTACT, SCHOOL_PPOWER},
	["level"] =     40,
	["mana"] =      200,
	["mana_max"] =  200,
	["fail"] =      20,
	["spell"] =     function()
			do_cmd_fusion(Ind)
			end,
	["info"] =      function()
			return ""
			end,
	["desc"] =      {
			"Fuses your mind with a friendly target with open mind nearby,",
			"allowing you spell-casting but giving up control over your body.",
--			"Fuses your mind and body with a friendly target with open mind nearby,",
--			"allowing you spell-casting but giving up control over the body.",
	}
}
end
