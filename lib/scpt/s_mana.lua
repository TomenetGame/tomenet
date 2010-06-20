-- The mana school

function get_manathrust_dam()
	return 3 + get_level(Ind, MANATHRUST, 50), 1 + get_level(Ind, MANATHRUST, 20)
end

MANATHRUST = add_spell
{
	["name"] = 	"Manathrust",
        ["school"] = 	SCHOOL_MANA,
        ["level"] = 	1,
        ["mana"] = 	1,
        ["mana_max"] =  25,
        ["fail"] = 	10,
        ["direction"] = TRUE,
        ["ftk"] = 1,
        ["spell"] = 	function(args)
	        	fire_bolt(Ind, GF_MANA, args.dir, damroll(get_manathrust_dam()), " casts a mana bolt for")
	end,
	["info"] = 	function()
	        	local x, y

        	        x, y = get_manathrust_dam()
			return "dam "..x.."d"..y
	end,
        ["desc"] =	{
        		"Conjures up mana into a powerful bolt",
                        "The damage is nearly irresistible and will increase with level"
		}
}

DELCURSES = add_spell
{
	["name"] = 	"Remove Curses",
        ["school"] = 	SCHOOL_MANA,
        ["level"] = 	20,
        ["mana"] = 	20,
        ["mana_max"] = 	40,
        ["fail"] = 	10,
        ["spell"] = 	function()
        		local done

        		if get_level(Ind, DELCURSES, 50) >= 20 then done = remove_all_curse(Ind)
                        else done = remove_curse(Ind) end
                        if done == TRUE then msg_print(Ind, "The curse is broken!") end
	end,
	["info"] = 	function()
			return ""
	end,
        ["desc"] =	{
        		"Remove curses of worn objects",
        		"At level 20 switches to *remove curses*"
		}
}

RESISTS = add_spell
{
	["name"] = 	"Elemental Shield",
        ["school"] = 	SCHOOL_MANA,
        ["level"] = 	20,
        ["mana"] = 	17,
        ["mana_max"] = 	20,
        ["fail"] = 	40,
        ["spell"] = 	function()
--                       	if player.oppose_fire == 0 then set_oppose_fire(Ind, randint(10) + 15 + get_level(Ind, RESISTS, 50)) end
--                       	if player.oppose_cold == 0 then set_oppose_cold(Ind, randint(10) + 15 + get_level(Ind, RESISTS, 50)) end
--                       	if player.oppose_elec == 0 then set_oppose_elec(Ind, randint(10) + 15 + get_level(Ind, RESISTS, 50)) end
--                       	if player.oppose_acid == 0 then set_oppose_acid(Ind, randint(10) + 15 + get_level(Ind, RESISTS, 50)) end
                           	set_oppose_fire(Ind, randint(10) + 15 + get_level(Ind, RESISTS, 50))
                           	set_oppose_cold(Ind, randint(10) + 15 + get_level(Ind, RESISTS, 50))
                           	set_oppose_elec(Ind, randint(10) + 15 + get_level(Ind, RESISTS, 50))
                           	set_oppose_acid(Ind, randint(10) + 15 + get_level(Ind, RESISTS, 50))
                        if player.spell_project > 0 then
                                fire_ball(Ind, GF_RESFIRE_PLAYER, 0, randint(20) + get_level(Ind, RESISTS, 50), player.spell_project, "")
                                fire_ball(Ind, GF_RESCOLD_PLAYER, 0, randint(20) + get_level(Ind, RESISTS, 50), player.spell_project, "")
                                fire_ball(Ind, GF_RESELEC_PLAYER, 0, randint(20) + get_level(Ind, RESISTS, 50), player.spell_project, "")
                                fire_ball(Ind, GF_RESACID_PLAYER, 0, randint(20) + get_level(Ind, RESISTS, 50), player.spell_project, "")
                        end
	end,
	["info"] = 	function()
			return "dur "..(15 + get_level(Ind, RESISTS, 50)).."+d10"
	end,
        ["desc"] =	{
        		"Provide resistances to the four basic elements",
                        "***Affected by the Meta spell: Project Spell***",
		}
}

MANASHIELD = add_spell
{
	["name"] = 	"Disruption Shield",
        ["school"] = 	SCHOOL_MANA,
--        ["level"] = 	45,
        ["level"] = 	35,
        ["mana"] = 	50,
        ["mana_max"] = 	50,
--        ["fail"] = 	-200,
        ["fail"] = 	10,
        ["spell"] = 	function()
--        		if get_level(Ind, MANASHIELD, 50) >= 5 then
--	                       	if (player.invuln == 0) then
--                                	set_invuln(Ind, randint(5) + 15 + get_level(Ind, MANASHIELD, 15))
--				end
--			else
--	                       	if (player.tim_manashield == 0) then
                                	set_tim_manashield(Ind, randint(10) + 20 + get_level(Ind, MANASHIELD, 75))
--                                end
--			end
        end,
	["info"] = 	function()
			return "dur "..(20 + get_level(Ind, MANASHIELD, 75)).."+d10"
	end,
        ["desc"] =	{
        		"Uses mana instead of hp to take damage"
--        		"At level 5 switches to globe of invulnerability",
--                        "The spell breaks as soon as a melee, shooting,",
--                        "throwing or magical skill action is attempted"
		}
}
