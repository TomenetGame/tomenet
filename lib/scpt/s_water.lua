-- handle the water school

TIDALWAVE = add_spell
{
	["name"] = 	"Tidal Wave",
        ["school"] = 	{SCHOOL_WATER},
        ["level"] = 	16,
        ["mana"] = 	16,
        ["mana_max"] = 	40,
        ["fail"] = 	20,
        ["spell"] = 	function()
		        fire_wave(Ind, GF_WAVE, 0, 40 + get_level(Ind, TIDALWAVE, 200), 0, 6 + get_level(Ind, TIDALWAVE, 10), EFF_WAVE)
	end,
	["info"] = 	function()
			return "dam "..(40 + get_level(Ind, TIDALWAVE,  200)).." rad "..(6 + get_level(Ind, TIDALWAVE,  10))
	end,
        ["desc"] =	{
        		"Summons a monstruous tidal wave that will expand and crush the",
                        "monsters under it's mighty waves"
        }
}

ICESTORM = add_spell
{
	["name"] = 	"Ice Storm",
        ["school"] = 	{SCHOOL_WATER},
        ["level"] = 	22,
        ["mana"] = 	30,
        ["mana_max"] = 	60,
        ["fail"] = 	20,
        ["spell"] = 	function()
        		local type
        
        		if get_level(Ind, ICESTORM, 50) >= 10 then type = GF_ICE
                        else type = GF_COLD end
		        fire_wave(Ind, type, 0, 80 + get_level(Ind, ICESTORM, 200), 1 + get_level(Ind, ICESTORM, 3, 0), 20 + get_level(Ind, ICESTORM, 70), EFF_STORM)
	end,
	["info"] = 	function()
			return "dam "..(80 + get_level(Ind, ICESTORM, 200)).." rad "..(1 + get_level(Ind, ICESTORM, 3, 0)).." dur "..(20 + get_level(Ind, ICESTORM, 70))
	end,
        ["desc"] =	{
        		"Engulfs you in a storm of roaring cold that strikes your foes",
                        "At level 10 it turns into shards of ice"
        }
}

ENTPOTION = add_spell
{
	["name"] = 	"Ent's Potion",
        ["school"] = 	{SCHOOL_WATER},
        ["level"] = 	6,
        ["mana"] = 	7,
        ["mana_max"] = 	15,
        ["fail"] = 	20,
        ["spell"] = 	function()
        		set_food(Ind, PY_FOOD_MAX - 1)
                        msg_print(Ind, "The Ent's Potion fills your stomach.")
        		if get_level(Ind, ENTPOTION, 50) >= 5 then
                        	set_afraid(Ind, 0)
                        end
        		if get_level(Ind, ENTPOTION, 50) >= 12 then
                        	set_hero(Ind, player.hero + randint(25) + 25 + get_level(Ind, ENTPOTION, 40))
                        end
	end,
	["info"] = 	function()
        		if get_level(Ind, ENTPOTION, 50) >= 12 then
                                return "dur "..(25 + get_level(Ind, ENTPOTION, 40)).."+d25"
                        else
				return ""
                        end
	end,
        ["desc"] =	{
                        "Fills up your stomach",
                        "At level 5 it boldens your heart",
                        "At level 12 it make you heroic"
        }
}

VAPOR = add_spell
{
	["name"] = 	"Vapor",
        ["school"] = 	{SCHOOL_WATER},
        ["level"] = 	2,
        ["mana"] = 	2,
        ["mana_max"] = 	12,
        ["fail"] = 	20,
        ["spell"] = 	function()
		        fire_cloud(Ind, GF_WATER, 0, 3 + get_level(Ind, VAPOR, 20), 3 + get_level(Ind, VAPOR, 9, 0), 5)
	end,
	["info"] = 	function()
       			return "dam "..(3 + get_level(Ind, VAPOR, 20)).." rad "..(3 + get_level(Ind, VAPOR, 9, 0)).." dur 5"
	end,
        ["desc"] =	{
                        "Fills the air with toxic moisture to eradicate annoying critters"
        }
}
