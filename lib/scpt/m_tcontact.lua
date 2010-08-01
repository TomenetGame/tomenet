-- handle the thought contact school
--((static: sense/esp))

--[[
MTAUNT = add_spell
{
	["name"] = 	"Cause Agression",
        ["school"] = 	{SCHOOL_TCONTACT},
        ["am"] =	50,
        ["spell_power"] = 0,
	["blind"] =	0,
        ["level"] = 	5,
        ["mana"] = 	10,
        ["mana_max"] = 	20,
        ["fail"] =      20,
        ["spell"] = 	function()
			end,
	["info"] = 	function()
			return ""
			end,
        ["desc"] =	{
        		"Causes a monster to charge and attack you in melee",
        }
}

MDISTRACT = add_spell
{
	["name"] = 	"Divert Attention",
        ["school"] = 	{SCHOOL_TCONTACT},
        ["am"] =	50,
        ["spell_power"] = 0,
	["blind"] =	0,
        ["level"] = 	10,
        ["mana"] = 	15,
        ["mana_max"] = 	15,
        ["fail"] =      20,
        ["spell"] = 	function()
			end,
	["info"] = 	function()
			return ""
			end,
        ["desc"] =	{
        		"Diverts attention of a monster next to you",
        		"so it attacks another player who is standing",
        		"right next to it (if any) instead of you.",
        }
}
]]

MSELFKNOW = add_spell
{
	["name"] = 	"Self-Reflection",
        ["school"] = 	{SCHOOL_TCONTACT},
        ["am"] =	33,
        ["spell_power"] = 0,
	["blind"] =	0,
        ["level"] = 	15,
        ["mana"] = 	15,
        ["mana_max"] = 	20,
        ["fail"] =      20,
        ["spell"] = 	function()
		        self_knowledge(Ind)
			end,
	["info"] = 	function()
			return ""
			end,
        ["desc"] =	{
        		"Find out more about yourself",
        }
}

MBOOST = add_spell
{
	["name"] =      "Willpower",
	["school"] =    {SCHOOL_TCONTACT},
        ["am"] =	33,
        ["spell_power"] = 0,
	["blind"] =	0,
	["level"] =     3,
	["mana"] =      3,
	["mana_max"] =  10,
	["fail"] =      5,
	["spell"] =     function()
			fire_ball(Ind, GF_MINDBOOST_PLAYER, 0, 40 + get_level(Ind, MBOOST, 180), 2, " focusses on your mind, unlocking something!")
                        set_mindboost(Ind, 20 + get_level(Ind, MBOOST, 90), 15 + randint(6))
			end,
	["info"] =      function()
			return ""
			end,
	["desc"] =      {
			"Boosts your willpower to unleash hidden potential,",
--			"improving both your performance and resilience",
			"improving your performance and resilience to various effects",
			"at level 23 it will also allow you to attack slightly faster.",
			"***Automatically projecting***",
-- what it infact does: tohit, res.fear, %res.conf, %res.para, %res.slow, +1 ea (later)
	}
}

MHASTE = add_spell
{
	["name"] =      "Accelerate Nerves",
	["school"] =    {SCHOOL_TCONTACT},
        ["am"] =	50,
        ["spell_power"] = 0,
	["blind"] =	0,
	["level"] =     20,
	["mana"] =      10,
	["mana_max"] =  30,
	["fail"] =      15,
	["spell"] =     function()
			local spd
			spd = get_level(Ind, MHASTE, 25)
			if spd > 10 then
				spd = 10
			end
			set_fast(Ind, 30 + randint(10) + get_level(Ind, MHASTE, 17), spd)
			end,
	["info"] =      function()
			local spd
			spd = get_level(Ind, MHASTE, 25)
			if spd > 10 then
				spd = 10
			end
			return "dur " .. (30 + get_level(Ind, MHASTE, 17)) .. "+d10 speed +" .. spd
			end,
	["desc"] =      {
			"Accelerates your nerve functions and metabolism",
			"by direct mental infusion, speeding you up",
	}
}

MCURE = add_spell
{
	["name"] =      "Clear Mind",
	["school"] =    {SCHOOL_TCONTACT},
        ["am"] =	50,
        ["spell_power"] = 0,
	["blind"] =	0,
	["confusion"] =	0,
	["level"] =     3,
	["mana"] =      3,
	["mana_max"] =  10,
	["fail"] =      5,
	["spell"] =     function()
			fire_ball(Ind, GF_REMFEAR_PLAYER, 0, get_level(Ind, MCURE, 50 * 2), 4, " waves over your eyes.")
                        set_afraid(Ind, 0)
                        player.res_fear_temp = get_level(Ind, MSANITY, 50)
	                if get_level(Ind, MCURE, 50) >= 5 then
	                        set_confused(Ind, 0)
	                end
	                if get_level(Ind, MCURE, 50) >= 10 then
	                        set_image(Ind, 0)
	                end
			end,
	["info"] =      function()
			return ""
			end,
	["desc"] =      {
			"Frees your mind from fear, confusion (level 5)",
			"and hallucinations (level 10)",
			"***Automatically projecting***",
	}
}

MSANITY = add_spell
{
	["name"] =      "Stabilize Thoughts",
	["school"] =    {SCHOOL_TCONTACT},
        ["am"] =	50,
        ["spell_power"] = 0,
	["blind"] =	0,
	["confusion"] =	0,
	["level"] =     25,
	["mana"] =      80,
	["mana_max"] =  100,
	["fail"] =      40,
	["spell"] =     function()
			fire_ball(Ind, GF_REMFEAR_PLAYER, 0, get_level(Ind, MSANITY, 50 * 2), 4, " waves over your eyes.")
                        set_afraid(Ind, 0)
                        player.res_fear_temp = get_level(Ind, MSANITY, 50)
                        set_image(Ind, 0)
                        set_confused(Ind, 0)
			heal_insanity(Ind, 15 + get_level(Ind, MSANITY, 55))
			fire_ball(Ind, GF_SANITY_PLAYER, 0, 30 + get_level(Ind, MSANITY, 110), 1, " focusses on your mind.")
			end,
	["info"] =      function()
			return "cures "..(15 + get_level(Ind, MSANITY, 55)).." SN"
			end,
	["desc"] =      {
			"Cures some insanity and removes malicious effects",
			"***Automatically projecting***",
	}
}

MSENSEMON = add_spell
{
	["name"] =      "Telepathy",
	["school"] =    {SCHOOL_TCONTACT},
        ["am"] =	50,
        ["spell_power"] = 0,
	["blind"] =	0,
--	["level"] =     1,
--	["mana"] =      3,
	["level"] =     20,
	["mana"] =      15,
	["mana_max"] =  15,
	["fail"] =      5,
	["spell"] =     function()
--			set_tim_invis(Ind, 10 + get_level(Ind, HSENSEMON, 50))
--			detect_creatures(Ind) <- detecting empty-minded monsters isn't mindcrafter ability
--			if get_level(Ind, MSENSEMON, 50) >= 20 then
				set_tim_esp(Ind, 10 + randint(10) + get_level(Ind, MSENSEMON, 50))
--			end
			end,
	["info"] =      function()
			return "dur 10+d10+d"..get_level(Ind, MSENSEMON, 50)
			end,
	["desc"] =      {
--			"Lets you see nearby creatures and allows you to see invisible.",
--			"At level 20 it lets you sense the presence creatures for a while.",
			"Lets you sense the presence creatures for a while.",
	}
}

MTELEKINESIS = add_spell
{
	["name"] = 	"Telekinesis",
        ["school"] = 	{SCHOOL_TCONTACT, SCHOOL_PPOWER},
        ["am"] =	50,
        ["spell_power"] = 0,
        ["level"] = 	35,
        ["mana"] = 	25,
        ["mana_max"] = 	25,
        ["fail"] =      10,
        ["get_item"] =  {
                        ["prompt"] = 	"Teleport which object? ",
                        ["inven"] = 	TRUE,
                        ["get"] = 	function (obj)
	                                if obj.weight <= 4 + get_level(Ind, MTELEKINESIS, 250, 0) then
        	                        	return TRUE
                	                end
                        	        return FALSE
                        end,
        },
        ["spell"] = 	function(args)
                        if args.item == -1 then return end
			if player.inventory[1 + args.item].weight <= 4 + get_level(Ind, MTELEKINESIS, 250, 0) then
                        	player.current_telekinesis = player.inventory[1 + args.book]
                                telekinesis_aux(Ind, args.item)
                        else
                                msg_print(Ind, "Pfft trying to hack your client ? pretty lame ...")
                        end
			end,
	["info"] = 	function()
			return "max wgt "..((4 + get_level(Ind, MTELEKINESIS, 250, 0)) / 10).."."..(imod(4 + get_level(Ind, MTELEKINESIS, 250, 0), 10))
			end,
        ["desc"] =	{
        		"Inscribe your book with @Pplayername, cast it, select an item",
                        "and the item will be teleported to that player whereever he/she might",
                        "be in the Universe",
        }
}
