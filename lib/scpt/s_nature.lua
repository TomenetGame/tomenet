-- handle the nature school

function get_healing_percents(limit_lev)
	local perc
	perc = get_level(Ind, HEALING_I, 31)
	if limit_lev ~= 0 then
		if perc > (limit_lev * 3) / 5 then
			perc = (limit_lev * 3) / 5 + (perc - (limit_lev * 3) / 5) / 3
		end
	end
	return 25 + perc
end
function get_healing_cap(limit_lev)
	local pow

	pow = ((10 + get_level(Ind, HEALING_I, 417)) * (get_level(Ind, HEALING_I, 417) + 209)) / 1562 + 1
	if limit_lev ~= 0 then
		if pow > limit_lev * 3 then
			pow = limit_lev * 3 + (pow - limit_lev * 3) / 3
		end
	end
	pow = (pow * 5) / 2

	if pow > 400 then
		pow = 400
	end
	return pow
end
function get_healing_power(limit_lev)
	local pow, cap
	pow = player.mhp * get_healing_percents(limit_lev) / 100
	cap = get_healing_cap(limit_lev)
	if pow > cap then
		pow = cap
	end
	return pow
end

GROWTREE = add_spell {
	["name"] = 	"Grow Trees",
	["name2"] = 	"GTrees",
	["school"] = 	{SCHOOL_NATURE},
	["level"] = 	30,
	["mana"] = 	25,
	["mana_max"] = 	25,
	["fail"] = 	-30,
	["spell"] = 	function()
			grow_trees(Ind, 1 + get_level(Ind, GROWTREE, 6))
	end,
	["info"] = 	function()
			return "rad "..(1 + get_level(Ind, GROWTREE, 6))
	end,
	["desc"] = 	{ "Makes trees grow extremely quickly around you.", }
}

HEALING_I = add_spell {
	["name"] = 	"Healing I",
	["name2"] = 	"Heal I",
	["school"] = 	{SCHOOL_NATURE},
	["level"] = 	1,
	["mana"] = 	13,
	["mana_max"] = 	13,
	["fail"] = 	10,
	["spell"] = 	function()
			fire_ball(Ind, GF_HEAL_PLAYER, 0, get_healing_power(1), 1, "")
	end,
	["info"] = 	function()
			return "heal "..get_healing_percents(1).."% (max "..get_healing_cap(1)..") = "..get_healing_power(1)
	end,
	["desc"] = 	{
			"Heals a percentage of your max hitpoints up to a spell level-dependent cap.",
			"Projecting heals nearby players for 1/2 of the amount.",
			"***Automatically projecting***",
	}
}
HEALING_II = add_spell {
	["name"] = 	"Healing II",
	["name2"] = 	"Heal II",
	["school"] = 	{SCHOOL_NATURE},
	["level"] = 	20,
	["mana"] = 	28,
	["mana_max"] = 	28,
	["fail"] = 	-30,
	["spell"] = 	function()
			fire_ball(Ind, GF_HEAL_PLAYER, 0, get_healing_power(15), 1, "")
	end,
	["info"] = 	function()
			return "heal "..get_healing_percents(15).."% (max "..get_healing_cap(15)..") = "..get_healing_power(15)
	end,
	["desc"] = 	{
			"Heals a percentage of your max hitpoints up to a spell level-dependent cap.",
			"Projecting heals nearby players for 1/2 of the amount.",
			"***Automatically projecting***",
	}
}
HEALING_III = add_spell {
	["name"] = 	"Healing III",
	["name2"] = 	"Heal III",
	["school"] = 	{SCHOOL_NATURE},
	["level"] = 	40,
	["mana"] = 	80,
	["mana_max"] = 	80,
	["fail"] = 	-70,
	["spell"] = 	function()
			fire_ball(Ind, GF_HEAL_PLAYER, 0, get_healing_power(0), 1, "")
	end,
	["info"] = 	function()
			return "heal "..get_healing_percents(0).."% (max "..get_healing_cap(0)..") = "..get_healing_power(0)
	end,
	["desc"] = 	{
			"Heals a percentage of your max hitpoints up to a spell level-dependent cap.",
			"Final cap is 400. Projecting heals nearby players for 1/2 of the amount.",
			"***Automatically projecting***",
	}
}

RECOVERY_I = add_spell	{
	["name"] = 	"Recovery I",
	["name2"] = 	"Recov I",
	["school"] = 	{SCHOOL_NATURE},
	["level"] = 	15,
	["mana"] = 	10,
	["mana_max"] = 	10,
	["fail"] = 	10,
	["spell"] = 	function()
			set_poisoned(Ind, 0, 0)
			set_cut(Ind, 0, 0)
			set_confused(Ind, 0)
			set_blind(Ind, 0)
			set_stun(Ind, 0)
			fire_ball(Ind, GF_CURE_PLAYER, 0, 4 + 8 + 16 + 256, 2, "")
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{
			"Neutralizes poison, heals cuts, cures confusion, blindness and stun.",
			"***Automatically projecting***",
	}
}
RECOVERY_II = add_spell	{
	["name"] = 	"Recovery II",
	["name2"] = 	"Recov II",
	["school"] = 	{SCHOOL_NATURE},
	["level"] = 	35,
	["mana"] = 	40,
	["mana_max"] = 	40,
	["fail"] = 	-30,
	["spell"] = 	function()
			set_poisoned(Ind, 0, 0)
			set_diseased(Ind, 0, 0)
			set_cut(Ind, 0, 0)
			set_confused(Ind, 0)
			set_blind(Ind, 0)
			set_stun(Ind, 0)
			do_res_stat(Ind, A_STR)
			do_res_stat(Ind, A_CON)
			do_res_stat(Ind, A_DEX)
			do_res_stat(Ind, A_WIS)
			do_res_stat(Ind, A_INT)
			do_res_stat(Ind, A_CHR)
			restore_level(Ind)
			fire_ball(Ind, GF_CURE_PLAYER, 0, 4 + 8 + 16 + 256 + 64 + 128, 2, "")
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{
			"Neutralizes poison, cures diseases, heals cuts, cures confusion,",
			"blindness and stun. Restores drained stats and lost experience.",
			"***Automatically projecting***",
	}
}

REGENERATION = add_spell {
	["name"] = 	"Regeneration",
	["name2"] = 	"Regen",
	["school"] = 	{SCHOOL_NATURE},
	["level"] = 	20,
	["mana"] = 	40,
	["mana_max"] = 	40,
	["fail"] = 	0,
	["spell"] = 	function()
			set_tim_regen(Ind, randint(10) + 5 + get_level(Ind, REGENERATION, 50), 10 + get_level(Ind, REGENERATION, 200))
	end,
	["info"] = 	function()
			local p = 10 + get_level(Ind, REGENERATION, 200)
			local p10 = p / 10
			p = p - p10 * 10
			return "dur "..(5 + get_level(Ind, REGENERATION, 50)).."+d10 heal "..p10.."."..p
	end,
	["desc"] = 	{ "Increases your body's regeneration rate.", }
}

VERMINCONTROL = add_spell {
	["name"] = 	"Vermin Control",
	["name2"] = 	"VermC",
	["school"] = 	{SCHOOL_NATURE},
	["level"] = 	10,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	20,
	["spell"] = 	function()
			do_vermin_control(Ind)
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{
			"Prevents any vermin from breeding.",
	}
}

RESISTS_I = add_spell {
	["name"] = 	"Elemental Shield I",
	["name2"] = 	"EleSh I",
	["school"] = 	{SCHOOL_FIRE,SCHOOL_WATER},
	["level"] = 	15,
	["mana"] = 	10,
	["mana_max"] = 	10,
	["fail"] = 	20,
	["spell"] = 	function()
			set_oppose_fire(Ind, randint(10) + 15 + get_level(Ind, RESISTS_I, 50))
			set_oppose_cold(Ind, randint(10) + 15 + get_level(Ind, RESISTS_I, 50))
			fire_ball(Ind, GF_RESFIRE_PLAYER, 0, randint(20) + get_level(Ind, RESISTS_I, 50), 2, "")
			fire_ball(Ind, GF_RESCOLD_PLAYER, 0, randint(20) + get_level(Ind, RESISTS_I, 50), 2, "")
	end,
	["info"] = 	function()
			return "dur "..(15 + get_level(Ind, RESISTS_I, 50)).."+d10"
	end,
	["desc"] = 	{
			"Provide resistances to heat and cold.",
			"***Automatically projecting***",
		}
}
RESISTS_II = add_spell {
	["name"] = 	"Elemental Shield II",
	["name2"] = 	"EleSh II",
	["school"] = 	{SCHOOL_FIRE,SCHOOL_WATER,SCHOOL_AIR,SCHOOL_EARTH},
	["level"] = 	20,
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	-5,
	["spell"] = 	function()
			set_oppose_fire(Ind, randint(10) + 15 + get_level(Ind, RESISTS_I, 50))
			set_oppose_cold(Ind, randint(10) + 15 + get_level(Ind, RESISTS_I, 50))
			set_oppose_elec(Ind, randint(10) + 15 + get_level(Ind, RESISTS_I, 50))
			set_oppose_acid(Ind, randint(10) + 15 + get_level(Ind, RESISTS_I, 50))
			fire_ball(Ind, GF_RESFIRE_PLAYER, 0, randint(20) + get_level(Ind, RESISTS_I, 50), 2, "")
			fire_ball(Ind, GF_RESCOLD_PLAYER, 0, randint(20) + get_level(Ind, RESISTS_I, 50), 2, "")
			fire_ball(Ind, GF_RESELEC_PLAYER, 0, randint(20) + get_level(Ind, RESISTS_I, 50), 2, "")
			fire_ball(Ind, GF_RESACID_PLAYER, 0, randint(20) + get_level(Ind, RESISTS_I, 50), 2, "")
	end,
	["info"] = 	function()
			return "dur "..(15 + get_level(Ind, RESISTS_I, 50)).."+d10"
	end,
	["desc"] = 	{
			"Provide resistances to the four basic elements,",
			"heat, cold, electricity and acid.",
			"***Automatically projecting***",
		}
}

DELCURSES_I = add_spell {
	["name"] = 	"Remove Curses I",
	["name2"] = 	"RCurs I",
	["school"] = 	SCHOOL_NATURE,
	["level"] = 	20,
	["mana"] = 	20,
	["mana_max"] =	20,
	["fail"] = 	20,
	["spell"] = 	function()
			local done
			done = remove_curse(Ind)
			if done == TRUE then msg_print(Ind, "The curse is broken!") end
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{ "Attempts to remove curses from your items.", }
}
DELCURSES_II = add_spell {
	["name"] = 	"Remove Curses II",
	["name2"] = 	"RCurs II",
	["school"] = 	SCHOOL_NATURE,
	["level"] = 	40,
	["mana"] = 	50,
	["mana_max"] =	50,
	["fail"] = 	-20,
	["spell"] = 	function()
			local done
			done = remove_all_curse(Ind)
			if done == TRUE then msg_print(Ind, "The curse is broken!") end
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{ "Removes all normal and heavy curses from your items.", }
}
