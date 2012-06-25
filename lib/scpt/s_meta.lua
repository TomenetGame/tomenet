-- handle the meta school

RECHARGE = add_spell
{
	["name"] = 	"Recharge",
        ["school"] = 	{SCHOOL_META},
        ["level"] = 	5,
        ["mana"] = 	10,
        ["mana_max"] = 	100,
        ["fail"] = 	10,
        ["stat"] =      A_INT,
        ["spell"] = 	function()
        		recharge(Ind, 10 + get_level(Ind, RECHARGE, 140))
	end,
	["info"] = 	function()
                	return "power "..(10 + get_level(Ind, RECHARGE, 140))
	end,
        ["desc"] =	{
        		"Taps on the ambient mana to recharge an object's power (charges or mana)",
        }
}

PROJECT_SPELLS = add_spell
{
	["name"] = 	"Project Spells",
        ["school"] = 	{SCHOOL_META},
        ["level"] = 	10,
        ["mana"] = 	0,
        ["mana_max"] = 	0,
        ["fail"] = 	-99,
        ["stat"] =      A_INT,
        ["spell"] = 	function()
			if player.rcraft_project ~= 0 then
				player.rcraft_project = 0
				msg_print(Ind, "You stop synchronizing your spells.")
                        end
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
        ["desc"] =	{
        		"Affects some of your spells(mostly utility ones) to make them",
                        "have an effect on your nearby party members",
        }
}

DISPERSEMAGIC = add_spell
{
	["name"] = 	"Disperse Magic",
        ["school"] = 	{SCHOOL_META},
        ["level"] = 	15,
        ["mana"] = 	30,
        ["mana_max"] = 	60,
        ["fail"] = 	10,
        ["stat"] =      A_INT,
        -- Unnafected by blindness
        ["blind"] =     FALSE,
        -- Unnafected by confusion
        ["confusion"] = FALSE,
        ["spell"] = 	function()
                        set_blind(Ind, 0)
                        if get_level(Ind, DISPERSEMAGIC, 50) >= 5 then
	                        set_confused(Ind, 0)
	                        set_image(Ind, 0)
                        end
                        if get_level(Ind, DISPERSEMAGIC, 50) >= 10 then
	                        set_slow(Ind, 0)
	                        set_fast(Ind, 0, 0)
                        end
                        if get_level(Ind, DISPERSEMAGIC, 50) >= 15 then
	                        set_stun(Ind, 0)
	                        -- set_meditation(Ind, 0)
	                        set_cut(Ind, 0, 0)
                        end
                        if get_level(Ind, DISPERSEMAGIC, 50) >= 20 then
	                        set_hero(Ind, 0)
	                        set_shero(Ind, 0)
                                set_blessed(Ind, 0)
                                set_shield(Ind, 0, 0, 0, 0, 0)
                                set_afraid(Ind, 0)
                        end
	end,
	["info"] = 	function()
                	return ""
	end,
        ["desc"] =	{
        		"Dispels a lot of magic that can affect you, be it good or bad",
                        "Level 1: blindness",
                        "Level 5: confusion and hallucination",
                        "Level 10: speed (both bad or good)",
                        -- "Level 15: stunning, meditation, cuts",
			"Level 15: stunning, cuts",
                        "Level 20: hero, super hero, bless, shields, afraid",
        }
}
