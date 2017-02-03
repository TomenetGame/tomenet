-- handle the holy defense school

HBLESSING_I = add_spell {
	["name"] = 	"Blessing I",
	["school"] = 	{SCHOOL_HDEFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	1,
	["mana"] = 	4,
	["mana_max"] = 	4,
	["fail"] = 	10,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			if player.blessed_power <= 8 then
				local dur
				player.blessed_power = 8
				dur = 9 + randint(get_level(Ind, HBLESSING_I, 25))
				set_blessed(Ind, dur)
				fire_ball(Ind, GF_BLESS_PLAYER, 0, dur, 2, " recites a blessing.")
			end
	end,
	["info"] = 	function()
			return "AC+8  dur 9.."..get_level(Ind, HBLESSING_I, 25) + 9
	end,
	["desc"] = 	{
			"Protects you with a shield of righteousness.",
			"***Automatically projecting***",
	}
}
__lua_HBLESSING = HBLESSING_I
HBLESSING_II = add_spell {
	["name"] = 	"Blessing II",
	["school"] = 	{SCHOOL_HDEFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	15,
	["mana"] = 	11,
	["mana_max"] = 	11,
	["fail"] = 	-7,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			if player.blessed_power <= 14 then
				local dur
				player.blessed_power = 14
				dur = 17 + randint(get_level(Ind, HBLESSING_I, 25))
				set_blessed(Ind, dur)
				fire_ball(Ind, GF_BLESS_PLAYER, 0, dur, 2, " chants.")
			end
	end,
	["info"] = 	function()
			return "AC+14  dur 17.."..get_level(Ind, HBLESSING_I, 25) + 17
	end,
	["desc"] = 	{
			"Protects you with a shield of righteousness.",
			"***Automatically projecting***",
	}
}
HBLESSING_III = add_spell {
	["name"] = 	"Blessing III",
	["school"] = 	{SCHOOL_HDEFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	30,
	["mana"] = 	25,
	["mana_max"] = 	25,
	["fail"] = 	-50,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			if player.blessed_power <= 20 then
				local dur
				player.blessed_power = 20
				dur = 32 + randint(get_level(Ind, HBLESSING_I, 25))
				set_blessed(Ind, dur)
				fire_ball(Ind, GF_BLESS_PLAYER, 0, dur, 2, " speaks a holy prayer.")
			end
	end,
	["info"] = 	function()
			return "AC+20  dur 32.."..get_level(Ind, HBLESSING_I, 25) + 32
	end,
	["desc"] = 	{
			"Protects you with a shield of righteousness.",
			"***Automatically projecting***",
	}
}

HRESISTS_I = add_spell {
	["name"] = 	"Holy Resistance I",
	["school"] = 	{SCHOOL_HDEFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	20,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	-25,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
		local dur
		dur = randint(10) + 15 + get_level(Ind, HRESISTS_I, 50)

		set_oppose_fire(Ind, dur)
		fire_ball(Ind, GF_RESFIRE_PLAYER, 0, dur, 1, " calls to the heavens for protection from the elements.")
		set_oppose_cold(Ind, dur)
		fire_ball(Ind, GF_RESCOLD_PLAYER, 0, dur, 1, "")
	end,
	["info"] = 	function()
			return "Res heat/cold dur "..(get_level(Ind, HRESISTS_I, 50) + 15)..".."..(get_level(Ind, HRESISTS_I, 50) + 25)
	end,
	["desc"] = 	{
			"Lets you resist heat and cold.",
			"***Automatically projecting***",
	}
}
HRESISTS_II = add_spell {
	["name"] = 	"Holy Resistance II",
	["school"] = 	{SCHOOL_HDEFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	30,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	-50,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
		local dur
		dur = randint(10) + 15 + get_level(Ind, HRESISTS_I, 50)

		set_oppose_fire(Ind, dur)
		fire_ball(Ind, GF_RESFIRE_PLAYER, 0, dur, 1, " calls to the heavens for protection from the elements.")
		set_oppose_cold(Ind, dur)
		fire_ball(Ind, GF_RESCOLD_PLAYER, 0, dur, 1, "")
		set_oppose_elec(Ind, dur)
		fire_ball(Ind, GF_RESELEC_PLAYER, 0, dur, 1, "")
		set_oppose_acid(Ind, dur)
		fire_ball(Ind, GF_RESACID_PLAYER, 0, dur, 1, "")
	end,
	["info"] = 	function()
			return "Res heat/cold/elec/acid, dur "..(get_level(Ind, HRESISTS_I, 50) + 15)..".."..(get_level(Ind, HRESISTS_I, 50) + 25)
	end,
	["desc"] = 	{
			"Lets you resist heat, cold, lightning and acid.",
			"***Automatically projecting***",
	}
}
HRESISTS_III = add_spell {
	["name"] = 	"Holy Resistance III",
	["school"] = 	{SCHOOL_HDEFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	40,
	["mana"] = 	35,
	["mana_max"] = 	35,
	["fail"] = 	-80,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
		local dur
		dur = randint(10) + 15 + get_level(Ind, HRESISTS_I, 50)

		set_oppose_fire(Ind, dur)
		fire_ball(Ind, GF_RESFIRE_PLAYER, 0, dur, 1, " calls to the heavens for protection from the elements.")
		set_oppose_cold(Ind, dur)
		fire_ball(Ind, GF_RESCOLD_PLAYER, 0, dur, 1, "")
		set_oppose_elec(Ind, dur)
		fire_ball(Ind, GF_RESELEC_PLAYER, 0, dur, 1, "")
		set_oppose_acid(Ind, dur)
		fire_ball(Ind, GF_RESACID_PLAYER, 0, dur, 1, "")
		set_oppose_pois(Ind, dur)
		fire_ball(Ind, GF_RESPOIS_PLAYER, 0, dur, 1, "")
	end,
	["info"] = 	function()
			return "Base+poison res., dur "..(get_level(Ind, HRESISTS_I, 50) + 15)..".."..(get_level(Ind, HRESISTS_I, 50) + 25)
	end,
	["desc"] = 	{
			"Lets you resist heat, cold, lightning, acid and poison.",
			"***Automatically projecting***",
	}
}

HPROTEVIL = add_spell {
	["name"] = 	"Protection from Evil",
	["school"] = 	{SCHOOL_HDEFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	12,
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	20,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			set_protevil(Ind, 20 + randint(10) + get_level(Ind, HPROTEVIL, 50))
	end,
	["info"] = 	function()
			return "dur "..20 + get_level(Ind, HPROTEVIL, 50)..".."..30 + get_level(Ind, HPROTEVIL, 50)
	end,
	["desc"] = 	{ "Repels evil that tries to lay hand on you.", }
}

DISPELMAGIC = add_spell {
	["name"] = 	"Dispel Magic",
	["school"] = 	{SCHOOL_HDEFENSE},
	["level"] = 	18,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	10,
	["stat"] = 	A_WIS,
	-- Unaffected by blindness
	["blind"] = 	FALSE,
	-- Unaffected by confusion
	["confusion"] = FALSE,
	["spell"] = 	function()
			set_blind(Ind, 0)
			set_confused(Ind, 0)
			if get_level(Ind, DISPELMAGIC, 50) >= 8 then
				set_image(Ind, 0)
			end
			if get_level(Ind, DISPELMAGIC, 50) >= 13 then
				set_slow(Ind, 0)
				set_fast(Ind, 0, 0)
				set_stun(Ind, 0)
			end
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{
			"Dispels a lot of magic that can affect you, be it good or bad.",
			"Level 1: blindness and confusion.",
			"Level 8: hallucination.",
			"Level 13: speed (both bad or good) and stun.",
	}
}

HRUNEPROT = add_spell {
	["name"] = 	"Glyph of Warding",
	["school"] = 	{SCHOOL_HDEFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	35,
	["mana"]= 	20,
	["mana_max"] = 	20,
	["fail"] = 	0,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			local x, y
			y = player.py
			x = player.px
			warding_glyph(Ind)
			end,
	["info"] = 	function()
			return ""
			end,
	["desc"] = 	{ "Creates a rune of protection on the ground.", }
}

HMARTYR = add_spell {
	["name"] = 	"Martyrdom",
	["school"] = 	{SCHOOL_HDEFENSE},
	["spell_power"] = 0,
	["am"] = 	75,
	["level"] = 	45,
	["mana"]= 	50,
	["mana_max"] = 	50,
	["fail"] = 	-60,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			if player.martyr_timeout > 0 then
				msg_print(Ind, "\255yThe heavens are not yet willing to accept your martyrdom.")
			else
				if (player.pclass == CLASS_PRIEST) then set_martyr(Ind, -15)
				else set_martyr(Ind, -8) end
			end
			end,
	["info"] = 	function()
			if (player.pclass == CLASS_PRIEST) then return "dur 15s  timeout 1000"
			else return "dur 8s  timeout 1000" end
			end,
	["desc"] = 	{
			"Turns you into an holy martyr, blessed with immortality to fulfil",
			"his work. When the holy fire ceases, you will be very close to",
			"death (at least 30 HP left). It will take some time until the",
			"heavens are willing to accept another martyrdom (type '/martyr').",
	}
}
