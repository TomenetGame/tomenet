-- handle the psycho-power school
--((static: +saving throw))

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
			teleport_player(Ind, dist, TRUE);
			end,
	["info"] = 	function()
                	return "distance "..(10 + get_level(Ind, MBLINK, 8))
			end,
        ["desc"] =	{
        		"Teleports you on a small scale range",
        },
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

--[[
MFUSION = add_spell
{
	["name"] =      "Mental Fusion",
--	["name"] =      "Corporeal Fusion",
	["school"] =    {SCHOOL_TCONTACT, SCHOOL_PPOWER, SCHOOL_MINTRUSION},
	["level"] =     40,
	["mana"] =      200,
	["mana_max"] =  200,
	["fail"] =      20,
	["spell"] =     function()
			do_fusion(Ind)
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
]]
