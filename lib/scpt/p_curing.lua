-- handle the holy curing school

function get_healing_percents()
	return 25 + get_level(Ind, HHEALING, 31)
end

function get_healing_cap()
	local pow
	pow = get_level(Ind, HHEALING, 417)
	if pow > 400 then
		pow = 400
	end
	return pow
end

function get_healing_power2()
        local pow, cap
        pow = player.mhp * get_healing_percents() / 100
	cap = get_healing_cap()
	if pow > cap then
		pow = cap
	end
        return pow
end

function get_curewounds_dice()
	local hd
        hd = get_level(Ind, HCUREWOUNDS, 27) + 1
        if (hd > 14) then hd = 14 end
        return hd
end
function get_curewounds_power()
	local pow
--[[
        pow = get_level(Ind, HCUREWOUNDS, 200)
	if pow > 112 then
		pow = 112
	end
]]
        pow = damroll(get_curewounds_dice(), 8)
        return pow
end

-- Keep consistent with GHOST_XP_LOST
function get_exp_loss()
	local pow
	--ENABLE_INSTANT_RES?
	if (def_hack("TEMP0", nil) == 1) then
		pow = (36 * (735 - (5 * get_level(Ind, HRESURRECT)))) / 735
		if pow < 30 then
			pow = 30
		end
	else
		pow = (41 * (120 - get_level(Ind, HRESURRECT))) / 120
		if pow < 33 then
			pow = 33
		end
	end
	return pow
end

HCUREWOUNDS = add_spell
{
	["name"] = 	"Cure Wounds",
        ["school"] = 	{SCHOOL_HCURING},
        ["am"] =	75,
	["level"] =     3,
	["mana"] =      3,
	["mana_max"] =  30,
	["fail"] =      15,
	["stat"] =      A_WIS,
	["direction"] = TRUE,
	["spell"] =     function(args)
			local status_ailments
			--hacks to cure effects same as potions would
			if get_level(Ind, HCUREWOUNDS, 50) >= 24 then
				status_ailments = 4000
			elseif get_level(Ind, HCUREWOUNDS, 50) >= 10 then
				status_ailments = 3000
			elseif get_level(Ind, HCUREWOUNDS, 50) >= 4 then
				status_ailments = 2000
			else
				status_ailments = 0
			end
			fire_grid_bolt(Ind, GF_HEAL_PLAYER, args.dir, status_ailments + get_curewounds_power(), " points at your wounds.")
	end,
	["info"] =      function()
--			return "heal "..get_curewounds_power()
			return "heal "..get_curewounds_dice().."d8"
	end,
	["desc"] =      {
		"Heals a certain amount of hitpoints of a friendly target",
		"Caps at 14d8 (same as potion of cure critical wounds)",
		"Also cures blindness and cuts at level 4",
		"Also cures confusion at level 10",
		"Also cures stun at level 24",
	}
}

HHEALING = add_spell
{
	["name"] = 	"Heal",
        ["school"] = 	{SCHOOL_HCURING},
        ["am"] =	75,
	["level"] =     3,
	["mana"] =      3,
	["mana_max"] =  70,
	["fail"] =      25,
	["stat"] =      A_WIS,
	["spell"] =     function()
			fire_ball(Ind, GF_HEAL_PLAYER, 0, 1000 + get_healing_power2(), 1, " points at your wounds.")
	end,
	["info"] =      function()
			return "heal "..get_healing_percents().."% (max "..get_healing_cap()..") = "..get_healing_power2()
	end,
	["desc"] =      {
		"Heals a percentage of your hitpoints up to a spell level-dependent cap",
		"The final cap is 400",
		"Projecting it will heal up to 3/4 of that amount on nearby players",
		"***Automatically projecting***",
	}
}
__lua_HHEALING = HHEALING

HDELCURSES = add_spell
{
        ["name"] =      "Break Curses",
        ["school"] =    SCHOOL_HCURING,
        ["am"] =	75,
        ["level"] =     10,
        ["mana"] =      20,
        ["mana_max"] =  40,
        ["fail"] =      40,
        ["stat"] =      A_WIS,
        ["spell"] =     function()
	                local done
			if get_level(Ind, HDELCURSES, 50) >= 30 then done = remove_all_curse(Ind)
            		else done = remove_curse(Ind) end
	                if done == TRUE then msg_print(Ind, "The curse is broken!") end
		        end,
        ["info"] =      function()
	                return ""
		        end,
        ["desc"] =      {
                "Remove curses of worn objects",
                "At level 30 switches to *remove curses*"
        }
}

HHEALING2 = add_spell
{
        ["name"] =      "Cleansing Light",
        ["school"] =    {SCHOOL_HCURING},
        ["am"] =	75,
        ["level"] =     18,
        ["mana"] =      1,
        ["mana_max"] =  100,
        ["fail"] =      30,
        ["stat"] =      A_WIS,
        ["direction"] = FALSE,
        ["spell"] =     function()
			fire_cloud(Ind, GF_HEALINGCLOUD, 0, (1 + get_level(Ind, HHEALING2, 42)), (1 + get_level(Ind, HHEALING2, 10)), (5 + get_level(Ind, HHEALING2, 16)), 10, " calls the spirits")
                        end,
        ["info"] =      function()
                        return "heals " .. (get_level(Ind, HHEALING2, 42) + 1) .. " rad " .. (1 + get_level(Ind,HHEALING2,10)) .. " dur " .. (5 + get_level(Ind, HHEALING2, 16))
                        end,
        ["desc"] =      { "Continuously heals you and those around you.",
			  }
}

HCURING = add_spell
{
	["name"] =      "Curing",
	["school"] =    {SCHOOL_HCURING},
        ["am"] =	75,
	["level"] =     3,
	["mana"] =      2,
	["mana_max"] =  10,
	["fail"] =      20,
	["stat"] =      A_WIS,
	["spell"] =     function()
	                if get_level(Ind, HCURING, 50) >= 15 then
				set_confused(Ind, 0)
				set_blind(Ind, 0)
				set_stun(Ind, 0)
		                set_poisoned(Ind, 0, 0)
	                        fire_ball(Ind, GF_CURE_PLAYER, 0, 1, 1, " concentrates on your maladies.")
			elseif get_level(Ind, HCURING, 50) >= 10 then
		                set_poisoned(Ind, 0, 0)
	                        fire_ball(Ind, GF_CUREPOISON_PLAYER, 0, 1, 1, " concentrates on your maladies.")
		        else
		    		if (player.poisoned ~= 0 and player.slow_poison == 0) then
					player.slow_poison = 1
				end
	                        fire_ball(Ind, GF_SLOWPOISON_PLAYER, 0, 1, 1, " concentrates on your maladies.")
			end
		        end,
	["info"] =      function()
		        return ""
	    		end,
        ["desc"] =      {
                        "Slows down the effect of poison",
                        "At level 10 it neutralizes poison",
                        "At level 15 it cures confusion, blindness and stun",
                	"***Automatically projecting***",
	}
}

HRESTORING = add_spell
{
	["name"] =      "Restoration",
	["school"] =    {SCHOOL_HCURING},
        ["am"] =	75,
	["level"] =     26,
	["mana"] =      20,
	["mana_max"] =  25,
	["fail"] =      20,
	["stat"] =      A_WIS,
	["spell"] =     function()
		        do_res_stat(Ind, A_STR)
		        do_res_stat(Ind, A_CON)
		        do_res_stat(Ind, A_DEX)
		        do_res_stat(Ind, A_WIS)
		        do_res_stat(Ind, A_INT)
			do_res_stat(Ind, A_CHR)
                        fire_ball(Ind, GF_RESTORESTATS_PLAYER, 0, 1, 1, "")
                        if get_level(Ind, HRESTORING, 50) >= 5 then
	                        restore_level(Ind)
	                        fire_ball(Ind, GF_RESTORELIFE_PLAYER, 0, 1, 1, "")
	                end
		        end,
	["info"] =      function()
		        return ""
	    		end,
        ["desc"] =      {
                        "At level 1 restores drained stats",
	                "At level 5 it restores lost experience",
                	"***Automatically projecting***",
	}
}

--[[ old mind focus spell
HSANITY = add_spell
{
	["name"] =      "Mind Focus",
	["school"] =    {SCHOOL_HCURING},
        ["am"] =	75,
	["level"] =     21,
	["mana"] =      50,
	["mana_max"] =  100,
	["fail"] =      50,
	["stat"] =      A_WIS,
	["spell"] =     function()
			set_image(Ind, 0)
			if get_level(Ind, HSANITY, 50) >= 20 then
				if player.csane < (player.msane * 6 / 12) then
					player.csane = (player.msane * 6 / 12)
				end
				fire_ball(Ind, GF_SANITY_PLAYER, 0, 6 * 2, 1, " waves over your eyes, murmuring some words.")
			elseif get_level(Ind, HSANITY, 50) >= 10 then
				if player.csane < (player.msane * 3 / 12) then
					player.csane = (player.msane * 3 / 12)
				end
				fire_ball(Ind, GF_SANITY_PLAYER, 0, 3 * 2, 1, " waves over your eyes, murmuring some words.")
			else
				fire_ball(Ind, GF_SANITY_PLAYER, 0, 0, 1, " waves over your eyes.")
			end
			end,
	["info"] =      function()
			return ""
			end,
	["desc"] =      {
			"Frees your mind from hallucinations",
			"At level 10 it slightly cures very bad insanity",
			"At level 20 it fairly cures very bad insanity",
			"***Automatically projecting***",
		}
}
]]

-- the new mind focus spell - mikaelh
-- effect ranges from a light SN potion at level 21 to a serious SN potion at level 50
-- increased max_mana from 100 to 150
HSANITY = add_spell
{
	["name"] =      "Faithful Focus",
	["school"] =    {SCHOOL_HCURING},
        ["am"] =	75,
	["level"] =     21,
	["mana"] =      50,
	["mana_max"] =  150,
	["fail"] =      50,
	["stat"] =      A_WIS,
	["spell"] =     function()
			set_afraid(Ind, 0)
			set_res_fear(Ind, get_level(Ind, HSANITY, 50))
			set_confused(Ind, 0)
			set_image(Ind, 0)

			heal_insanity(Ind, 15 + get_level(Ind, HSANITY, 55))
			if player.csane == player.msane then
				msg_print(Ind, "You are in full command of your mental faculties.")
			end

			fire_ball(Ind, GF_SANITY_PLAYER, 0, 30 + get_level(Ind, HSANITY, 110), 1, " waves over your eyes, murmuring some words..")
			end,
	["info"] =      function()
			return "cures "..(15 + get_level(Ind, HSANITY, 55)).." SN"
			end,
	["desc"] =      {
			"Frees your mind from fear, confusion, hallucinations",
			"and also cures some insanity.",
			"***Automatically projecting***",
		}
}

HRESURRECT = add_spell
{
	["name"] =      "Resurrection",
	["school"] =    {SCHOOL_HCURING},
        ["am"] =	100,
	["level"] =     30,
	["mana"] =      200,
	["mana_max"] =  500,
	["fail"] =      70,
	["stat"] =      A_WIS,
	["spell"] =     function()
			fire_ball(Ind, GF_RESURRECT_PLAYER, 0, get_level(Ind, HRESURRECT, 46*2), 1, " resurrects you!")
		        end,
	["info"] =      function()
		        return "exp -"..get_exp_loss().."%"
	    		end,
        ["desc"] =      {
                        "Resurrects another player's ghost back to life.",
			"The higher the skill, the less experience he will lose.",
		}
}

HDELBB = add_spell
{
	["name"] =      "Soul Curing",
	["school"] =    {SCHOOL_HCURING},
        ["am"] =	75,
	["level"] =     25,	-- 45 the_sandman: too high lvl and this spell doesn't seem to be useful then. Asked around,
				-- and ppl say their first encounter with RW is about pvp 25-32ish.
	["mana"] =      150,	-- was 200. Only chat/hope has priests with >200 mana at lvl ~25+ =)
	["mana_max"] =  200,
	["fail"] =      0,
	["stat"] =      A_WIS,
	["spell"] =     function()
			msg_print(Ind, "You feel a calming warmth touching your soul.");
			if (player.black_breath) then
	            		msg_print(Ind, "The hold of the Black Breath on you is broken!");
				player.black_breath = FALSE
			end
                        fire_ball(Ind, GF_SOULCURE_PLAYER, 0, 1, 1, " chants loudly, praising the light!")
		        end,
	["info"] =      function()
		        return ""
	    		end,
        ["desc"] =      {
                        "Cures the Black Breath.",
                	"***Automatically projecting***",
		}
}
