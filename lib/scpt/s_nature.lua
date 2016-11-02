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

	--pow = get_level(Ind, HEALING, 417)
	--if limit_lev ~= 0 then
	--	if pow > limit_lev * 8 then
	--		pow = limit_lev * 8 + (pow - limit_lev * 8) / 3
	--	end
	--end

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
--	["school"] = 	{SCHOOL_NATURE, SCHOOL_TEMPORAL},
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
	["school"] = 	{SCHOOL_NATURE},
	["level"] = 	1,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	10,
	["spell"] = 	function()
			local status_ailments = 1024
			--hacks to cure effects same as potions would
			--if get_level(Ind, HEALING, 50) >= 24 then
			--	status_ailments = status_ailments + 8192 + 4096 + 2048
			--elseif get_level(Ind, HEALING, 50) >= 10 then
			--	status_ailments = status_ailments + 4096 + 2048
			--elseif get_level(Ind, HEALING, 50) >= 4 then
			--	status_ailments = status_ailments + 2048
			--end

			fire_ball(Ind, GF_HEAL_PLAYER, 0, status_ailments + get_healing_power(1), 1, "")
--			hp_player(Ind, get_healing_power(0)) <- doesn't give a neat msg with numbers
	end,
	["info"] = 	function()
			return "heal "..get_healing_percents(1).."% (max "..get_healing_cap(1)..") = "..get_healing_power(1)
	end,
	["desc"] = 	{
			"Heals a percent of hitpoints up to a maximum of 400 points healed.",
			"Projecting it will heal up to half that amount on nearby players.",
	}
}
HEALING_II = add_spell {
	["name"] = 	"Healing II",
	["school"] = 	{SCHOOL_NATURE},
	["level"] = 	20,
	["mana"] = 	40,
	["mana_max"] = 	40,
	["fail"] = 	-30,
	["spell"] = 	function()
			local status_ailments = 1024
			--hacks to cure effects same as potions would
			--if get_level(Ind, HEALING, 50) >= 24 then
			--	status_ailments = status_ailments + 8192 + 4096 + 2048
			--elseif get_level(Ind, HEALING, 50) >= 10 then
			--	status_ailments = status_ailments + 4096 + 2048
			--elseif get_level(Ind, HEALING, 50) >= 4 then
			--	status_ailments = status_ailments + 2048
			--end

			fire_ball(Ind, GF_HEAL_PLAYER, 0, status_ailments + get_healing_power(15), 1, "")
--			hp_player(Ind, get_healing_power(0)) <- doesn't give a neat msg with numbers
	end,
	["info"] = 	function()
			return "heal "..get_healing_percents(15).."% (max "..get_healing_cap(15)..") = "..get_healing_power(15)
	end,
	["desc"] = 	{
			"Heals a percent of hitpoints up to a maximum of 400 points healed.",
			"Projecting it will heal up to half that amount on nearby players.",
	}
}
HEALING_III = add_spell {
	["name"] = 	"Healing III",
	["school"] = 	{SCHOOL_NATURE},
	["level"] = 	40,
	["mana"] = 	120,
	["mana_max"] = 	120,
	["fail"] = 	-70,
	["spell"] = 	function()
			local status_ailments = 1024
			--hacks to cure effects same as potions would
			--if get_level(Ind, HEALING, 50) >= 24 then
			--	status_ailments = status_ailments + 8192 + 4096 + 2048
			--elseif get_level(Ind, HEALING, 50) >= 10 then
			--	status_ailments = status_ailments + 4096 + 2048
			--elseif get_level(Ind, HEALING, 50) >= 4 then
			--	status_ailments = status_ailments + 2048
			--end

			fire_ball(Ind, GF_HEAL_PLAYER, 0, status_ailments + get_healing_power(0), 1, "")
--			hp_player(Ind, get_healing_power(0)) <- doesn't give a neat msg with numbers
	end,
	["info"] = 	function()
			return "heal "..get_healing_percents(0).."% (max "..get_healing_cap(0)..") = "..get_healing_power(0)
	end,
	["desc"] = 	{
			"Heals a percent of hitpoints up to a maximum of 400 points healed.",
			"Projecting it will heal up to half that amount on nearby players.",
	}
}

RECOVERY_I = add_spell	{
	["name"] = 	"Recovery I",
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
	["school"] = 	{SCHOOL_NATURE},
	["level"] = 	35,
	["mana"] = 	40,
	["mana_max"] = 	40,
	["fail"] = 	-30,
	["spell"] = 	function()
			set_poisoned(Ind, 0, 0)
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
			"Neutralizes poison, heals cuts, cures confusion, blindness and stun.",
			"Restores drained stats and lost experience.",
			"***Automatically projecting***",
	}
}

REGENERATION = add_spell {
	["name"] = 	"Regeneration",
	["school"] = 	{SCHOOL_NATURE},
	["level"] = 	20,
	["mana"] = 	40,
	["mana_max"] = 	40,
	["fail"] = 	20,
	["spell"] = 	function()
			set_tim_regen(Ind, randint(10) + 5 + get_level(Ind, REGENERATION, 50), 300 + get_level(Ind, REGENERATION, 700))
	end,
	["info"] = 	function()
			return "dur "..(5 + get_level(Ind, REGENERATION, 50)).."+d10 power "..(300 + get_level(Ind, REGENERATION, 700))
	end,
	["desc"] = 	{ "Increases your body's regeneration rate.", }
}

VERMINCONTROL = add_spell {
	["name"] = 	"Vermin Control",
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
--	["school"] = 	SCHOOL_NATURE,
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
--	["school"] = 	SCHOOL_NATURE,
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
			"heat, cold, lightning and acid.",
			"***Automatically projecting***",
		}
}

--[[
SUMMONANIMAL = add_spell {
	["name"] = 	"Summon Animal",
	["school"] = 	{SCHOOL_NATURE},
	["level"] = 	25,
	["mana"] = 	25,
	["mana_max"] = 	50,
	["fail"] = 	20,
	["spell"] = 	function()
			summon_specific_level = 25 + get_level(SUMMONANIMAL, 50)
			summon_monster(py, px, dun_level, TRUE, SUMMON_ANIMAL)
	end,
	["info"] = 	function()
			return "level "..(25 + get_level(SUMMONANIMAL, 50))
	end,
	["desc"] = 	{
			"Summons a leveled animal to your aid",
	}
}
]]
