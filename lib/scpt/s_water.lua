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
		        fire_wave(Ind, GF_WAVE, 0, 40 + get_level(Ind, TIDALWAVE, 200), 0, 6 + get_level(Ind, TIDALWAVE, 10), EFF_WAVE, " casts a tidal wave for")
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
        
        		if get_level(Ind, ICESTORM, 50) >= 15 then type = GF_ICE
                        else type = GF_COLD end
		        fire_wave(Ind, type, 0, 80 + get_level(Ind, ICESTORM, 200), 1 + get_level(Ind, ICESTORM, 3, 0), 20 + get_level(Ind, ICESTORM, 70), EFF_STORM, " summons an ice storm for")
	end,
	["info"] = 	function()
			return "dam "..(80 + get_level(Ind, ICESTORM, 200)).." rad "..(1 + get_level(Ind, ICESTORM, 3, 0)).." dur "..(20 + get_level(Ind, ICESTORM, 70))
	end,
        ["desc"] =	{
        		"Engulfs you in a storm of roaring cold that strikes your foes",
                        "At level 15 it turns into shards of ice"
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
                        if player.spell_project > 0 then
                                fire_ball(Ind, GF_SATHUNGER_PLAYER, 0, 1, player.spell_project, "")
                        end
        		if get_level(Ind, ENTPOTION, 50) >= 5 then
                        	set_afraid(Ind, 0)
	                        if player.spell_project > 0 then
        	                        fire_ball(Ind, GF_REMFEAR_PLAYER, 0, 1, player.spell_project, "")
                	        end
                        end
        		if get_level(Ind, ENTPOTION, 50) >= 12 then
                        	set_hero(Ind, player.hero + randint(25) + 25 + get_level(Ind, ENTPOTION, 40))
	                        if player.spell_project > 0 then
        	                        fire_ball(Ind, GF_HERO_PLAYER, 0, randint(25) + 25 + get_level(Ind, ENTPOTION, 40), player.spell_project, "")
                	        end
                        end
--        		if get_level(Ind, ENTPOTION, 50) >= 28 then
--                        	set_shero(Ind, player.hero + randint(15) + 15 + get_level(Ind, ENTPOTION, 40))
--	                        if player.spell_project > 0 then
--        	                        fire_ball(Ind, GF_SHERO_PLAYER, 0, randint(15) + 15 + get_level(Ind, ENTPOTION, 40), player.spell_project, "")
--                	        end
--		        end
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
                        "At level 12 it make you heroic",
--			"At level 28 it gives you berserk strength",
                        "***Affected by the Meta spell: Project Spell***",
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
		        fire_cloud(Ind, GF_WATER, 0, 3 + get_level(Ind, VAPOR, 20), 3 + get_level(Ind, VAPOR, 9, 0), 5, " fires a cloud of vapor for")
	end,
	["info"] = 	function()
       			return "dam "..(3 + get_level(Ind, VAPOR, 20)).." rad "..(3 + get_level(Ind, VAPOR, 9, 0)).." dur 5"
	end,
        ["desc"] =	{
                        "Fills the air with toxic moisture to eradicate annoying critters"
        }
}
