-- handle the holy support school

HSANCTUARY = add_spell
{
	["name"] = 	"Sanctuary",
        ["school"] = 	{SCHOOL_HSUPPORT},
        ["level"] = 	1,
        ["mana"] = 	5,
        ["mana_max"] = 	15,
        ["fail"] = 	10,
	["stat"] =      A_WIS,
        ["spell"] = 	function()
                if get_level(Ind, HSANCTUARY, 50) < 20 then
			project(0 - Ind, get_level(Ind, HSANCTUARY, 20), player.wpos, player.py, player.px, get_level(Ind, HSANCTUARY, 50), GF_OLD_SLEEP, flg, "")
		else
			project_hack(Ind, GF_OLD_SLEEP, get_level(Ind, HSANCTUARY, 30))
		end
	end,
	["info"] = 	function()
                if get_level(Ind, HSANCTUARY, 50) < 20 then
			return "dur "..(get_level(Ind, HSANCTUARY, 50)).." rad "..get_level(Ind, HSANCTUARY, 20)
                else
			return "dur "..(get_level(Ind, HSANCTUARY, 30))
                end
	end,
        ["desc"] =	{
        		"Lets monsters next to you fall asleep",
        		"At level 20 it lets all nearby monsters fall asleep",
        }
}

HSATISFYHUNGER = add_spell
{
	["name"] = 	"Satisfy Hunger",
        ["school"] = 	{SCHOOL_HSUPPORT},
        ["level"] = 	15,
        ["mana"] = 	30,
        ["mana_max"] = 	30,
        ["fail"] = 	40,
	["stat"] =      A_WIS,
        ["spell"] = 	function()
			set_food(Ind, PY_FOOD_MAX - 1)
			if player.spell_project > 0 then
				fire_ball(Ind, GF_SATHUNGER_PLAYER, 0, 1, player.spell_project, "")
			end
			end,
	["info"] =	function()
			return ""
			end,
	["desc"] =	{
			"Satisfies your hunger.",
			"***Affected by the Meta spell: Project Spell***",
			}
}

HDELCURSES = add_spell
{
	["name"] =      "Remove Curses",
	["school"] =    SCHOOL_HSUPPORT,
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

HSENSE = add_spell
{
	["name"] =      "Sense Surroundings",
	["school"] =    SCHOOL_HSUPPORT,
	["level"] =     10,
	["mana"] =      20,
	["mana_max"] =  50,
	["fail"] =      25,
	["stat"] =      A_WIS,
	["spell"] =     function()
		if get_level(Ind, HSENSE, 50) >= 30 then
			set_tim_esp(Ind, 10 + randint(10) + get_level(Ind, HSENSE, 20))
  			wiz_lite_extra(Ind)
			if player.spell_project > 0 then
				fire_ball(Ind, GF_SEEMAP_PLAYER, 0, 1, player.spell_project, "")
			end
		elseif get_level(Ind, HSENSE, 50) >= 15 then
			map_area(Ind)
			if player.spell_project > 0 then
				fire_ball(Ind, GF_SEEMAP_PLAYER, 0, 1, player.spell_project, "")
			end
		end
		set_tim_invis(Ind, 10 + get_level(Ind, HSENSE, 50))
		detect_creatures(Ind)
		if player.spell_project > 0 then
			fire_ball(Ind, GF_DETECTCREATURE_PLAYER, 0, 1, player.spell_project, "")
		end
	end,
	["info"] =      function()
		return ""
	end,
	["desc"] =      {
		"Lets you see nearby creatures, as well as invisible ones.",
		"At level 15 it also maps the dungeon around you.",
		"At level 30 it grants you clairvoyance and lets you",
		"detect creatures for a while.",
		"***Affected by the Meta spell: Project Spell***",
	}
}

HSENSEMON = add_spell
{
	["name"] =      "Sense Monsters",
	["school"] =    SCHOOL_HSUPPORT,
	["level"] =     5,
	["mana"] =      3,
	["mana_max"] =  40,
	["fail"] =      15,
	["stat"] =      A_WIS,
	["spell"] =     function()
		detect_creatures(Ind)
		if player.spell_project > 0 then
			fire_ball(Ind, GF_DETECTCREATURE_PLAYER, 0, 1, player.spell_project, "")
		end
		if get_level(Ind, HSENSEMON, 50) >= 30 then
			set_tim_esp(Ind, 10 + randint(10) + get_level(Ind, HSENSEMON, 20))
		end
	end,
	["info"] =      function()
		return ""
	end,
	["desc"] =      {
		"Lets you see nearby creatures.",
		"At level 30 it lets you detect creatures for a while.",
		"***Affected by the Meta spell: Project Spell***",
	}
}
