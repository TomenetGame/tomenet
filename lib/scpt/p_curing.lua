-- handle the holy curing school

HHEALING = add_spell
{
	["name"] = 	"Healing",
        ["school"] = 	{SCHOOL_HCURING},
	["level"] =     10,
	["mana"] =      15,
	["mana_max"] =  50,
	["fail"] =      25,
	["stat"] =      A_WIS,
	["spell"] =     function()
		local hp = player.mhp * (15 + get_level(Ind, HHEALING, 35)) / 100
		hp_player(Ind, hp)
		if player.spell_project > 0 then
			fire_ball(Ind, GF_HEAL_PLAYER, 0, (hp * 3) / 2, player.spell_project, "")
		end
	end,
	["info"] =      function()
		return "heal "..(15 + get_level(Ind, HHEALING, 35)).."% = "..(player.mhp * (15 + get_level(Ind, HHEALING, 35)) / 100).."hp"
	end,
	["desc"] =      {
		"Heals a percent of hitpoints",
		"***Affected by the Meta spell: Project Spell***",
	}
}

HCURING = add_spell
{
	["name"] =      "Curing",
	["school"] =    {SCHOOL_HCURING},
	["level"] =     15,
	["mana"] =      10,
	["mana_max"] =  25,
	["fail"] =      20,
	["stat"] =      A_WIS,
	["spell"] =     function()
	        	set_poisoned(Ind, player.poisoned / 2)
	                if get_level(Ind, HCURING, 50) >= 10 then
		                set_poisoned(Ind, 0)
		                set_cut(Ind, 0)
				set_stun(Ind, 0)
		                if player.spell_project > 0 then
		                        fire_ball(Ind, GF_CURE_PLAYER, 0, 1, player.spell_project, "")
                    		end
			end
	                if get_level(Ind, HCURING, 50) >= 20 then
			        do_res_stat(Ind, A_STR)
			        do_res_stat(Ind, A_CON)
			        do_res_stat(Ind, A_DEX)
			        do_res_stat(Ind, A_WIS)
			        do_res_stat(Ind, A_INT)
				do_res_stat(Ind, A_CHR)
		                if player.spell_project > 0 then
		                        fire_ball(Ind, GF_RESTORESTATS_PLAYER, 0, 1, player.spell_project, "")
                    		end
                        end
                        if get_level(Ind, HCURING, 50) >= 25 then
	                        restore_level(Ind)
		                if player.spell_project > 0 then
		                        fire_ball(Ind, GF_RESTORELIFE_PLAYER, 0, 1, player.spell_project, "")
                    		end
	                end
		        end,
	["info"] =      function()
		        return ""
	    		end,
        ["desc"] =      {
                        "Reduces the length of time that you are poisoned",
                        "At level 10 it cures poison, cuts, stun, blindness and confusion",
                        "At level 20 it restores drained stats",
	                "At level 25 it restores lost experience",
                	"***Affected by the Meta spell: Project Spell***",
	}
}

HSANITY = add_spell
{
	["name"] =      "Mind Focus",
	["school"] =    {SCHOOL_HCURING},
	["level"] =     20,
	["mana"] =      50,
	["mana_max"] =  100,
	["fail"] =      50,
	["stat"] =      A_WIS,
	["spell"] =     function()
			set_image(Ind, 0)
	                if get_level(Ind, HSANITY, 50) >= 20 then
				if player.csane < (player.msane * 2 / 5) then
					player.csane = (player.msane * 2 / 5)
				end
		                if player.spell_project > 0 then
		                        fire_ball(Ind, GF_SANITY_PLAYER, 0, 2, player.spell_project, "")
	            		end
	                elseif get_level(Ind, HSANITY, 50) >= 10 then
				if player.csane < (player.msane / 5) then
					player.csane = (player.msane / 5)
				end
		                if player.spell_project > 0 then
		                        fire_ball(Ind, GF_SANITY_PLAYER, 0, 1, player.spell_project, "")
	            		end
			else
		                if player.spell_project > 0 then
		                        fire_ball(Ind, GF_SANITY_PLAYER, 0, 0, player.spell_project, "")
	            		end
	                end
		        end,
	["info"] =      function()
		        return ""
	    		end,
        ["desc"] =      {
                        "Frees your mind from hallucinations",
                        "At level 10 it slightly cures very bad insanity",
                        "At level 20 it fairly cures very bad insanity",
                	"***Affected by the Meta spell: Project Spell***",
		}
}

HRESURRECT = add_spell
{
	["name"] =      "Resurrection",
	["school"] =    {SCHOOL_HCURING},
	["level"] =     30,
	["mana"] =      200,
	["mana_max"] =  500,
	["fail"] =      50,
	["stat"] =      A_WIS,
	["spell"] =     function()
			fire_ball(Ind, GF_RESURRECT_PLAYER, 0, get_level(Ind, HRESURRECT, 20), 1, " resurrects you!")
		        end,
	["info"] =      function()
		        return ""
	    		end,
        ["desc"] =      {
                        "Resurrects another player's ghost back to life.",
			"The higher your skill is, the less experience he will lose.",
		}
}

HDELFEAR = add_spell
{
	["name"] =	"Remove Fear",
	["school"] =	{SCHOOL_HCURING},
	["level"] = 	1,
	["mana"] =	4,
	["mana_max"] =	4,
	["fail"] =	10,
	["stat"] =	A_WIS,
	["spell"] =	function()
			set_afraid(Ind, 0)
	                if player.spell_project > 0 then
	                        fire_ball(Ind, GF_REMFEAR_PLAYER, 0, 1, player.spell_project, "")
            		end
			end,
	["info"] =	function()
			return ""
			end,
	["desc"] =	{
			"Removes fear from your heart.",
                	"***Affected by the Meta spell: Project Spell***",
			}
}
