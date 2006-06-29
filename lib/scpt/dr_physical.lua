function get_speed()
        local spd
--  22->35skill+10, 19->39skill+10, 17->42skill+10
	spd = get_level(Ind, QUICKFEET, 17)
	if spd > 10 then
		spd = 10
	end
        return spd
end

-- The original 'healing cloud' for priests (which never happened)
-- Basically a nox that heals you. Not targettable; casts the cloud around the caster

HEALINGCLOUD = add_spell
{
        ["name"] =      "Forest's Embrace",
        ["school"] =    {SCHOOL_DRUID_PHYSICAL},
        ["level"] =     18,
        ["mana"] =      1,
        ["mana_max"] =  100,
        ["fail"] =      30,
        ["stat"] =      A_WIS,
        ["direction"] = FALSE,
        ["spell"] =     function()
			fire_cloud(Ind, GF_HEALINGCLOUD, 0, (1 + get_level(Ind, HEALINGCLOUD, 25)), (1 + get_level(Ind, HEALINGCLOUD, 2)), (5 + get_level(Ind, HEALINGCLOUD, 20)), " calls the spirits")
                        end,
        ["info"] =      function()
                        return "heals " .. (get_level(Ind, HEALINGCLOUD, 25) + 1) .. " rad " .. (1 + get_level(Ind,HEALINGCLOUD,2)) .. " dur " .. (5 + get_level(Ind, HEALINGCLOUD, 20))
                        end,
        ["desc"] =      { "Continuously heals you and those around you. (Auto-projecting)",
			  }
}

-- Similar to Temporal's 'Essence of Speed'
-- This one adds {1 .. 10} to player's speed, and affects all in radius 2
QUICKFEET = add_spell
{
	["name"] = 	"Quickfeet",
	["school"] = 	{SCHOOL_DRUID_PHYSICAL},
	["level"] = 	13,
	["mana"] = 	10,
	["mana_max"] = 	50,
	["fail"] = 	40,
        ["stat"] =      A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			set_fast(Ind, 30 + randint(10) + get_level(Ind, QUICKFEET, 30), get_speed())
			fire_ball(Ind, GF_SPEED_PLAYER, 0, get_speed() * 2, 2, "")
			end,
	["info"] = 	function()
			return "dur " .. (30 + get_level(Ind, QUICKFEET, 30)) .. "+d10 speed +" .. get_speed()
			end,
	["desc"] = 	{ "Quicken your steps and those around you. (Auto-projecting)",}
}

-- A curing spell
HERBALTEA = add_spell
{
	["name"] = 	"Herbal Tea",
	["school"] = 	{SCHOOL_DRUID_PHYSICAL},
	["level"] = 	3,
	["mana"] = 	50,
	["mana_max"] = 	100,
	["fail"] = 	20,
        ["stat"] =      A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			local lvl
			lvl = get_level(Ind, HERBALTEA, 50)
				set_food(Ind, PY_FOOD_MAX - 1)

			if lvl >= 20 then
				restore_level(Ind)
				fire_ball(Ind, GF_RESTORELIFE_PLAYER, 0, 1, 1, "")
			end
                        if lvl >= 25 then
                                do_res_stat(Ind, A_STR)
                                do_res_stat(Ind, A_CON)
                                do_res_stat(Ind, A_DEX)
                                do_res_stat(Ind, A_WIS)
                                do_res_stat(Ind, A_INT)
                                do_res_stat(Ind, A_CHR)
				fire_ball(Ind, GF_RESTORESTATS_PLAYER, 0, 1, 1, "")
			end
			if lvl >= 35 then
				msg_print(Ind, "That tasted bitter sweet.")
				player.black_breath = FALSE
				fire_ball(Ind, GF_SOULCURE_PLAYER, 0, 1, 1, " gives you something bitter to drink.")
			end
			end,
	["info"] = 	function()
			return ""
			end,
	["desc"] = 	{ "It sustains you and those around you. (Auto-projecting)",
			  "At level 20 it brews a drink that restores your life level.",
			  "At level 25 it brews tea that restores your body status.",
			  "At level 35 it brews the strongest tea to cure even the Black Breath.",}
}

-- +stats booster!
EXTRASTATS = add_spell
{
	["name"] = 	"Extra Growth",
	["school"] = 	{SCHOOL_DRUID_PHYSICAL},
	["level"] = 	10,
	["mana"] = 	10,
	["mana_max"] = 	50,
	["fail"] = 	40,
        ["stat"] =      A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			do_xtra_stats(Ind, get_level(Ind, EXTRASTATS, 100), get_level(Ind, EXTRASTATS, 50))
			end,
	["info"] = 	function()
			return "dur " .. (20 + get_level(Ind, EXTRASTATS, 50))
			end,
	["desc"] = 	{ "At level 5 increases your charisma.",
			  "At level 10 also increases your dexterity.",
			  "At level 15 also increases your strength.",
			  "At level 20 also increases your constitution.",
			  "At level 25 also increases your intelligence.",}
}

-- A shot that increases a players SPR (if wearing a shooter) 
-- but also decreases his/her speed!
FOCUSSHOT = add_spell
{
	["name"] = 	"Focus",
	["school"] = 	{SCHOOL_DRUID_PHYSICAL},
	["level"] = 	1,
	["mana"] = 	20,
	["mana_max"] = 	50,
	["fail"] = 	30,
        ["stat"] =      A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			do_focus_shot(Ind, get_level(Ind, FOCUSSHOT, 100), get_level(Ind, FOCUSSHOT, 50))
			end,
	["info"] = 	function()
			return "increases toXHit by " .. get_level(Ind, FOCUSSHOT, 25)
			end,
	["desc"] = 	{ "Increases your accuracy.", }
}
