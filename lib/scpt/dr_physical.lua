function get_speed()
	local spd

	spd = (get_level(Ind, QUICKFEET, 100) + 4) / 3
	if spd > 20 then
		spd = 20
	end
	return(spd / 2)
end

-- The original 'healing cloud' for priests (which never happened)
-- Basically a nox that heals you. Not targettable; casts the cloud around the caster

HEALINGCLOUD_I = add_spell {
	["name"] = 	"Forest's Embrace I",
	["name2"] = 	"Embrace I",
	["school"] = 	{SCHOOL_DRUID_PHYSICAL},
	["spell_power"] = 0,
	["level"] = 	18,
	["mana"] = 	18,
	["mana_max"] = 	18,
	["fail"] = 	-30,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			fire_cloud(Ind, GF_HEALINGCLOUD, 0, (4 + get_level(Ind, HEALINGCLOUD_I, 20)), (1 + get_level(Ind, HEALINGCLOUD_I, 5)), (7 + get_level(Ind, HEALINGCLOUD_I, 9)), 10, " calls the spirits")
			end,
	["info"] = 	function()
			return "heals " .. (4 + get_level(Ind, HEALINGCLOUD_I, 20)) .. " rad " .. (1 + get_level(Ind, HEALINGCLOUD_I, 5)) .. " dur " .. (7 + get_level(Ind, HEALINGCLOUD_I, 9))
			end,
	["desc"] = 	{ "Continuously heals you and those around you.",
			  "Damages undead creatures.", }
}
HEALINGCLOUD_II = add_spell {
	["name"] = 	"Forest's Embrace II",
	["name2"] = 	"Embrace II",
	["school"] = 	{SCHOOL_DRUID_PHYSICAL},
	["spell_power"] = 0,
	["level"] = 	29,
	["mana"] = 	32,
	["mana_max"] = 	32,
	["fail"] = 	-60,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			fire_cloud(Ind, GF_HEALINGCLOUD, 0, (4 + get_level(Ind, HEALINGCLOUD_I, 35)), (1 + get_level(Ind, HEALINGCLOUD_I, 5)), (7 + get_level(Ind, HEALINGCLOUD_I, 9)), 10, " calls the spirits")
			end,
	["info"] = 	function()
			return "heals " .. (4 + get_level(Ind, HEALINGCLOUD_I, 35)) .. " rad " .. (1 + get_level(Ind, HEALINGCLOUD_I, 5)) .. " dur " .. (7 + get_level(Ind, HEALINGCLOUD_I, 9))
			end,
	["desc"] = 	{ "Continuously heals you and those around you.",
			  "Damages undead creatures.", }
}
HEALINGCLOUD_III = add_spell {
	["name"] = 	"Forest's Embrace III",
	["name2"] = 	"Embrace III",
	["school"] = 	{SCHOOL_DRUID_PHYSICAL},
	["spell_power"] = 0,
	["level"] = 	40,
	["mana"] = 	60,
	["mana_max"] = 	60,
	["fail"] = 	-85,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			fire_cloud(Ind, GF_HEALINGCLOUD, 0, (4 + get_level(Ind, HEALINGCLOUD_I, 50)), (1 + get_level(Ind, HEALINGCLOUD_I, 5)), (7 + get_level(Ind, HEALINGCLOUD_I, 9)), 10, " calls the spirits")
			end,
	["info"] = 	function()
			return "heals " .. (4 + get_level(Ind, HEALINGCLOUD_I, 50)) .. " rad " .. (1 + get_level(Ind, HEALINGCLOUD_I, 5)) .. " dur " .. (7 + get_level(Ind, HEALINGCLOUD_I, 9))
			end,
	["desc"] = 	{ "Continuously heals you and those around you.",
			  "Damages undead creatures.", }
}

-- Similar to Temporal's 'Essence of Speed'
-- This one adds {1 .. 10} to player's speed, and affects all in radius 2
QUICKFEET = add_spell {
	["name"] = 	"Quickfeet",
	["name2"] = 	"Quick",
	["school"] = 	{SCHOOL_DRUID_PHYSICAL},
	["spell_power"] = 0,
	["level"] = 	16,
	["mana"] = 	25,
	["mana_max"] = 	25,
	["fail"] = 	-10,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			set_fast(Ind, 30 + randint(10) + get_level(Ind, QUICKFEET, 30), get_speed())
			fire_ball(Ind, GF_SPEED_PLAYER, 0, get_speed(), 2, "")
			end,
	["info"] = 	function()
			return("dur " .. (30 + get_level(Ind, QUICKFEET, 30)) .. "+d10 speed +" .. get_speed())
			end,
	["desc"] = 	{ "Quicken your steps and those around you. (Auto-projecting)", }
}

-- A curing spell
HERBALTEA = add_spell {
	["name"] = 	"Herbal Tea",
	["name2"] = 	"Tea",
	["school"] = 	{SCHOOL_DRUID_PHYSICAL},
	["spell_power"] = 0,
	["level"] = 	3,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	20,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			local lvl

			lvl = get_level(Ind, HERBALTEA, 50)

			msg_print(Ind, "That tasted bitter sweet.")
			set_food(Ind, PY_FOOD_MAX - 1)

			if lvl >= 35 then
				set_poisoned(Ind, 0, 0)
				restore_level(Ind)
				do_res_stat(Ind, A_STR)
				do_res_stat(Ind, A_CON)
				do_res_stat(Ind, A_DEX)
				do_res_stat(Ind, A_WIS)
				do_res_stat(Ind, A_INT)
				do_res_stat(Ind, A_CHR)
				if (player.black_breath == TRUE) then
					msg_print(Ind, "The hold of the Black Breath on you is broken!");
					player.black_breath = FALSE
				end
				fire_ball(Ind, GF_RESTORE_PLAYER, 0, 1 + 2 + 4 + 8 + 16, 1, " gives you something bitter to drink.")
			elseif lvl >= 25 then
				set_poisoned(Ind, 0, 0)
				restore_level(Ind)
				do_res_stat(Ind, A_STR)
				do_res_stat(Ind, A_CON)
				do_res_stat(Ind, A_DEX)
				do_res_stat(Ind, A_WIS)
				do_res_stat(Ind, A_INT)
				do_res_stat(Ind, A_CHR)
				fire_ball(Ind, GF_RESTORE_PLAYER, 0, 1 + 2 + 4 + 16, 1, " gives you something bitter to drink.")
			elseif lvl >= 20 then
				set_poisoned(Ind, 0, 0)
				restore_level(Ind)
				fire_ball(Ind, GF_RESTORE_PLAYER, 0, 1 + 2 + 16, 1, " gives you something bitter to drink.")
			elseif lvl >= 8 then
				set_poisoned(Ind, 0, 0)
				fire_ball(Ind, GF_RESTORE_PLAYER, 0, 1 + 16, 1, " gives you something bitter to drink.")
			else
				fire_ball(Ind, GF_RESTORE_PLAYER, 0, 1, 1, " gives you something bitter to drink.")
			end
			end,
	["info"] = 	function()
			return ""
			end,
	["desc"] = 	{ "It sustains you and those around you. (Auto-projecting)",
			  "At level 8 the tea will also cure poisons circulating your body.",
			  "At level 20 it brews a drink that restores your life level.",
			  "At level 25 it brews tea that restores your body's attributes.",
			  "At level 35 it brews the strongest tea to cure even the Black Breath.", }
}

-- +stats booster!
EXTRASTATS_I = add_spell {
	["name"] = 	"Extra Growth I",
	["name2"] = 	"Growth I",
	["school"] = 	{SCHOOL_DRUID_PHYSICAL},
	["spell_power"] = 0,
	["level"] = 	15,
	["mana"] = 	10,
	["mana_max"] = 	10,
	["fail"] = 	-15,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			if (get_level(Ind, EXTRASTATS_I, 50) >= 5) then
				do_xtra_stats(Ind, 1, 1 + get_level(Ind, EXTRASTATS_I, 50) / 9, rand_int(5) + 24 + get_level(Ind, EXTRASTATS_I, 10), 0)
			else
				do_xtra_stats(Ind, 0, 1 + get_level(Ind, EXTRASTATS_I, 50) / 9, rand_int(5) + 24 + get_level(Ind, EXTRASTATS_I, 10), 0)
			end
			end,
	["info"] = 	function()
			return "+" .. (1 + get_level(Ind, EXTRASTATS_I, 50) / 9) .. " dur d5+" .. (24 + get_level(Ind, EXTRASTATS_I, 10))
			end,
	["desc"] = 	{ "At level 1 increases your strength.",
			  "At level 5 also increases your dexterity.",
			  "Also grants hitpoint regeneration power.",
			}
}
EXTRASTATS_II = add_spell {
	["name"] = 	"Extra Growth II",
	["name2"] = 	"Growth II",
	["school"] = 	{SCHOOL_DRUID_PHYSICAL},
	["spell_power"] = 0,
	["level"] = 	25,
	["mana"] = 	25,
	["mana_max"] = 	25,
	["fail"] = 	-45,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			if (get_level(Ind, EXTRASTATS_II, 50) >= 11) then
				do_xtra_stats(Ind, 3, 1 + get_level(Ind, EXTRASTATS_I, 50) / 9, rand_int(5) + 24 + get_level(Ind, EXTRASTATS_I, 10), 0)
			else
				do_xtra_stats(Ind, 2, 1 + get_level(Ind, EXTRASTATS_I, 50) / 9, rand_int(5) + 24 + get_level(Ind, EXTRASTATS_I, 10), 0)
			end
			end,
	["info"] = 	function()
			return "+" .. (1 + get_level(Ind, EXTRASTATS_I, 50) / 9) .. " dur d5+" .. (24 + get_level(Ind, EXTRASTATS_I, 10))
			end,
	["desc"] = 	{ "Increases strength, dexterity, constitution.",
			  "At level 11 also increases your intelligence.",
			  "Also grants hitpoint regeneration power.",
			}
}

-- A shot that increases a players SPR (if wearing a shooter)
-- but also decreases his/her speed!
FOCUS = add_spell {
	["name"] = 	"Focus I",
	["name2"] = 	"Focus I",
	["school"] = 	{SCHOOL_DRUID_PHYSICAL},
	["spell_power"] = 0,
	["level"] = 	1,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	0,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			local lev = get_level(Ind, FOCUS, 35)
			local dur = rand_int(5) + 15 + get_level(Ind, FOCUS, 10)

			if lev < 1 then lev = 1 end
			if lev > 10 then lev = 10 end
			fire_ball(Ind, GF_EXTRA_TOHIT, 0, lev + (dur * 100), 1, " calls on your inner focus.")
			do_focus(Ind, lev, dur)
			end,
	["info"] = 	function()
			local lev = get_level(Ind, FOCUS, 35)

			if lev < 1 then lev = 1 end
			if lev > 10 then lev = 10 end
			return "+" .. lev .. " dur d5+" .. (15 + get_level(Ind, FOCUS, 10))
			end,
	["desc"] = 	{ "Increases your accuracy, caps at +10. (Auto-projecting)", }
}
__lua_FOCUS = FOCUS

FOCUS_II = add_spell {
	["name"] = 	"Focus II",
	["name2"] = 	"Focus II",
	["school"] = 	{SCHOOL_DRUID_PHYSICAL},
	["spell_power"] = 0,
	["level"] = 	25,
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	-50,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			local lev = get_level(Ind, FOCUS, 30)
			local dur = rand_int(5) + 15 + get_level(Ind, FOCUS, 10)

			if lev < 15 then lev = 15 end
			if lev > 25 then lev = 25 end
			fire_ball(Ind, GF_EXTRA_TOHIT, 0, lev + (dur * 100), 1, " calls on your inner focus.")
			do_focus(Ind, lev, dur)
			end,
	["info"] = 	function()
			local lev = get_level(Ind, FOCUS, 30)

			if lev < 15 then lev = 15 end
			if lev > 25 then lev = 25 end
			return "+" .. lev .. " dur d5+" .. (15 + get_level(Ind, FOCUS, 10))
			end,
	["desc"] = 	{ "Increases your accuracy, caps at +25. (Auto-projecting)", }
}
