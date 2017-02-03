-- The astral school ///update-dummy-bytes

function get_astral_lev(Ind)
	return ((players(Ind).s_info[SKILL_ASTRAL + 1].value + 1) / 2000 + players(Ind).lev / 2)
end

function get_astral_dam(Ind, limit_lev)
	local lev
	lev = get_astral_lev(Ind)
	if limit_lev ~= 0 and lev > limit_lev then lev = limit_lev + (lev - limit_lev) / 2 end
	return (3 + ((lev * 3) / 5)), (1 + lev / 2)
end

function get_astral_ball_dam(Ind, limit_lev)
	local lev
	lev = get_astral_lev(Ind)
	if limit_lev ~= 0 and lev > limit_lev then lev = limit_lev + (lev - limit_lev) / 2 end
	return lev * 9
end

function get_veng_power(Ind)
	local l = get_astral_lev(Ind)
	if (l > 50) then
		return (500)
	else
		return ((l * l) / 5)
	end
end

function get_astral_bonus_hp(Ind)
	if (get_astral_lev(Ind) >= 53) then
		return 2
	elseif (get_astral_lev(Ind) >= 40) then
		return 1
	end
	return 0
end

POWERBOLT_I = add_spell {
	["name"] = 	"Power Bolt I",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	1,
	["mana"] = 	3,
	["mana_max"] = 	3,
	["fail"] = 	5,
	["stat"] = 	A_INT,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
				fire_bolt(Ind, GF_MANA, args.dir, damroll(get_astral_dam(Ind, 1)), " casts a mana bolt for")
			elseif (players(Ind).ptrait == TRAIT_CORRUPTED) then
				fire_bolt(Ind, GF_DISP_ALL, args.dir, damroll(get_astral_dam(Ind, 1)), " casts a dispelling bolt for")
			else
				fire_bolt(Ind, GF_ELEC, args.dir, damroll(get_astral_dam(Ind, 1)), " casts a lightning bolt for")
			end
	end,
	["info"] = 	function()
			local xx, yy
			xx, yy = get_astral_dam(Ind, 1)
			return "dam "..xx.."d"..yy
	end,
	["desc"] = 	{
			"Enlightened: conjures up a powerful bolt of mana.",
			"Corrupted: conjures up a powerful dispelling bolt.",
			"Neutral: conjures up a bolt of lightning."
		}
}
POWERBOLT_II = add_spell {
	["name"] = 	"Power Bolt II",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	20,
	["mana"] = 	7,
	["mana_max"] = 	7,
	["fail"] = 	-35,
	["stat"] = 	A_INT,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
				fire_bolt(Ind, GF_MANA, args.dir, damroll(get_astral_dam(Ind, 20)), " casts a mana bolt for")
			elseif (players(Ind).ptrait == TRAIT_CORRUPTED) then
				fire_bolt(Ind, GF_DISP_ALL, args.dir, damroll(get_astral_dam(Ind, 20)), " casts a dispelling bolt for")
			else
				fire_bolt(Ind, GF_ELEC, args.dir, damroll(get_astral_dam(Ind, 20)), " casts a lightning bolt for")
			end
	end,
	["info"] = 	function()
			local xx, yy
			xx, yy = get_astral_dam(Ind, 20)
			return "dam "..xx.."d"..yy
	end,
	["desc"] = 	{
			"Enlightened: conjures up a powerful bolt of mana.",
			"Corrupted: conjures up a powerful dispelling bolt.",
			"Neutral: conjures up a bolt of lightning."
		}
}
POWERBOLT_III = add_spell {
	["name"] = 	"Power Bolt III",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	40,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	-100,
	["stat"] = 	A_INT,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
				fire_bolt(Ind, GF_MANA, args.dir, damroll(get_astral_dam(Ind, 0)), " casts a mana bolt for")
			elseif (players(Ind).ptrait == TRAIT_CORRUPTED) then
				fire_bolt(Ind, GF_DISP_ALL, args.dir, damroll(get_astral_dam(Ind, 0)), " casts a dispelling bolt for")
			end
	end,
	["info"] = 	function()
			local xx, yy
			xx, yy = get_astral_dam(Ind, 0)
			return "dam "..xx.."d"..yy
	end,
	["desc"] = 	{
			"Enlightened: conjures up a powerful bolt of mana.",
			"Corrupted: conjures up a powerful dispelling bolt.",
		}
}

POWERBEAM_I = add_spell {
	["name"] = 	"Power Ray I",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	5,
	["mana"] = 	5,
	["mana_max"] = 	5,
	["fail"] = 	10,
	["stat"] = 	A_INT,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
				fire_beam(Ind, GF_LITE, args.dir, damroll(get_astral_dam(Ind, 1)), " casts a beam of light for")
			elseif (players(Ind).ptrait == TRAIT_CORRUPTED) then
				fire_beam(Ind, GF_DARK, args.dir, damroll(get_astral_dam(Ind, 1)), " casts a beam of unlight for")
			else
				fire_beam(Ind, GF_ELEC, args.dir, damroll(get_astral_dam(Ind, 1)), " casts a lightning beam for")
			end
	end,
	["info"] = 	function()
			local xx, yy
			xx, yy = get_astral_dam(Ind, 1)
			return "dam "..xx.."d"..yy
	end,
	["desc"] = 	{
			"Enlightened: conjures up a powerful beam of light.",
			"Corrupted: conjures up a powerful darkness beam.",
			"Neutral: conjures up a beam of lightning."
		}
}
POWERBEAM_II = add_spell {
	["name"] = 	"Power Ray II",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	20,
	["mana"] = 	10,
	["mana_max"] = 	10,
	["fail"] = 	-30,
	["stat"] = 	A_INT,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
				fire_beam(Ind, GF_LITE, args.dir, damroll(get_astral_dam(Ind, 15)), " casts a beam of light for")
			elseif (players(Ind).ptrait == TRAIT_CORRUPTED) then
				fire_beam(Ind, GF_DARK, args.dir, damroll(get_astral_dam(Ind, 15)), " casts a beam of unlight for")
			else
				fire_beam(Ind, GF_ELEC, args.dir, damroll(get_astral_dam(Ind, 15)), " casts a lightning beam for")
			end
	end,
	["info"] = 	function()
			local xx, yy
			xx, yy = get_astral_dam(Ind, 15)
			return "dam "..xx.."d"..yy
	end,
	["desc"] = 	{
			"Enlightened: conjures up a powerful beam of light.",
			"Corrupted: conjures up a powerful darkness beam.",
			"Neutral: conjures up a beam of lightning."
		}
}
POWERBEAM_III = add_spell {
	["name"] = 	"Power Ray III",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	40,
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	-100,
	["stat"] = 	A_INT,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
				fire_beam(Ind, GF_LITE, args.dir, damroll(get_astral_dam(Ind, 0)), " casts a beam of light for")
			elseif (players(Ind).ptrait == TRAIT_CORRUPTED) then
				fire_beam(Ind, GF_DARK, args.dir, damroll(get_astral_dam(Ind, 0)), " casts a beam of unlight for")
			end
	end,
	["info"] = 	function()
			local xx, yy
			xx, yy = get_astral_dam(Ind, 0)
			return "dam "..xx.."d"..yy
	end,
	["desc"] = 	{
			"Enlightened: conjures up a powerful beam of light.",
			"Corrupted: conjures up a powerful darkness beam.",
		}
}

POWERBALL_I = add_spell {
	["name"] = 	"Power Blast I",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	10,
	["mana"] = 	8,
	["mana_max"] = 	8,
	["fail"] = 	0,
	["stat"] = 	A_INT,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
				fire_ball(Ind, GF_MANA, args.dir, get_astral_ball_dam(Ind, 1), 2 + get_level(Ind, POWERBALL_I, 2), " casts a mana ball for")
			elseif (players(Ind).ptrait == TRAIT_CORRUPTED) then
				fire_ball(Ind, GF_DISP_ALL, args.dir, get_astral_ball_dam(Ind, 1), 2 + get_level(Ind, POWERBALL_I, 2), " casts a dispelling ball for")
			else
				fire_ball(Ind, GF_ELEC, args.dir, get_astral_ball_dam(Ind, 1), 2 + get_level(Ind, POWERBALL_I, 2), " casts a lightning ball for")
			end
	end,
	["info"] = 	function()
			local dam
			dam = get_astral_ball_dam(Ind, 1)
			return "dam "..dam.." rad "..2 + get_level(Ind, POWERBALL_I, 2)
	end,
	["desc"] = 	{
			"Enlightened: conjures up a powerful ball of mana.",
			"Corrupted: conjures up a powerful dispelling ball.",
			"Neutral: conjures up a ball of lightning."
		}
}
POWERBALL_II = add_spell {
	["name"] = 	"Power Blast II",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	25,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	-50,
	["stat"] = 	A_INT,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
				fire_ball(Ind, GF_MANA, args.dir, get_astral_ball_dam(Ind, 15), 2 + get_level(Ind, POWERBALL_I, 2), " casts a mana ball for")
			elseif (players(Ind).ptrait == TRAIT_CORRUPTED) then
				fire_ball(Ind, GF_DISP_ALL, args.dir, get_astral_ball_dam(Ind, 15), 2 + get_level(Ind, POWERBALL_I, 2), " casts a dispelling ball for")
			end
	end,
	["info"] = 	function()
			local dam
			dam = get_astral_ball_dam(Ind, 15)
			return "dam "..dam.." rad "..2 + get_level(Ind, POWERBALL_I, 2)
	end,
	["desc"] = 	{
			"Enlightened: conjures up a powerful ball of mana.",
			"Corrupted: conjures up a powerful dispelling ball.",
		}
}
POWERBALL_III = add_spell {
	["name"] = 	"Power Blast III",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	45,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	-115,
	["stat"] = 	A_INT,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
				fire_ball(Ind, GF_MANA, args.dir, get_astral_ball_dam(Ind, 0), 2 + get_level(Ind, POWERBALL_I, 2), " casts a mana ball for")
			elseif (players(Ind).ptrait == TRAIT_CORRUPTED) then
				fire_ball(Ind, GF_DISP_ALL, args.dir, get_astral_ball_dam(Ind, 0), 2 + get_level(Ind, POWERBALL_I, 2), " casts a dispelling ball for")
			end
	end,
	["info"] = 	function()
			local dam
			dam = get_astral_ball_dam(Ind, 0)
			return "dam "..dam.." rad "..2 + get_level(Ind, POWERBALL_I, 2)
	end,
	["desc"] = 	{
			"Enlightened: conjures up a powerful ball of mana.",
			"Corrupted: conjures up a powerful dispelling ball.",
		}
}

RELOCATION = add_spell {
	["name"] = 	"Relocation",
	["school"] = 	SCHOOL_ASTRAL,
	["level"] = 	22, --the same level that one gets initiated (!) (ie 20 + 2)
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	10,
	["spell_power"] = 0,
	["am"] = 	67,
	["spell"] = 	function(args)
			local dur = randint(21 - get_level(Ind, RECALL, 15)) + 15 - get_level(Ind, RECALL, 10)
			if args.book < 0 then return end
			set_recall(Ind, dur, player.inventory[1 + args.book])
	end,
	["info"] = 	function()
			return "dur "..(15 - get_level(Ind, RECALL, 10)).."+d"..(21 - get_level(Ind, RECALL, 15))
	end,
	["desc"] = 	{
			"Recalls into the dungeon, back to the surface or across the world.",
	}
}

VENGEANCE = add_spell {
	["name"] = 	"Vengeance",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	30,
	["mana"] = 	80,
	["mana_max"] =  80,
	["fail"] = 	102,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			divine_vengeance(Ind, get_veng_power(Ind));
	end,
	["info"] = 	function()
			return "power "..get_veng_power(Ind);
	end,
	["desc"] = 	{
			"Enlightened: summons party member on the same area to you.",
			"             (Also will teleport monsters in sight to you,",
			"             as well as summoning additional monsters per ",
			"             player you pull towards you.)",
			"Corrupted: damages all monsters in sight."
		}
}
EMPOWERMENT = add_spell {
	["name"] = 	"Empowerment",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	40,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	102,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["am"] = 	33,
	["blind"] = 	0,
	["spell"] = 	function(args)
			local alev = get_astral_lev(Ind)

			-- A: fury
			-- D: +hp (stacks with +LIFE, up to +3 total cap)

			if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
				set_fury(Ind, 15 + rand_int(alev / 10))
			elseif (players(Ind).ptrait == TRAIT_CORRUPTED) then
				do_divine_hp(Ind, get_astral_bonus_hp(Ind), randint(5) + (alev * 2) / 3)
			end
	end,
	["info"] = 	function()
			if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
				return "dur 15+d"..(get_astral_lev(Ind) / 10)
			elseif (players(Ind).ptrait == TRAIT_CORRUPTED) then
				return "pow +"..(get_astral_bonus_hp(Ind) * 10).."% dur d5+"..((get_astral_lev(Ind) * 2) / 3)
			else
				return ""
			end
	end,
	["desc"] = 	{
			"Enlightened: incite self fury.",
			"Corrupted: increases your hit points."
		}
}
INTENSIFY = add_spell {
	["name"] = 	"The Silent Force",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	45,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	102,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["am"] = 	67,
	["spell"] = 	function(args)
			local alev = get_astral_lev(Ind)

			-- A: aoe slow, time/mana res
			-- D: +crit

			if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
				project_los(Ind, GF_OLD_SLOW, alev * 3, "")
				do_divine_xtra_res_time(Ind, randint(10) + alev)
			elseif (players(Ind).ptrait == TRAIT_CORRUPTED) then
				do_divine_crit(Ind, 2 + ((alev - 45) / 5), randint(5) + (alev * 2) / 3)
			end
	end,
	["info"] = 	function()
			local alev = get_astral_lev(Ind)

			if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
				return "pow "..(alev * 3).." dur d10+"..alev
			elseif (players(Ind).ptrait == TRAIT_CORRUPTED) then
				return "pow +"..(2 + ((alev - 45) / 5)).." dur d5+"..((alev * 2) / 3)
			else
				return ""
			end
	end,
	["desc"] = 	{
			"Enlightened: slows down monsters in sight and",
			"             grants temporary time resistance.",
			"Corrupted: increases your critical chance.",
		}
}

POWERCLOUD = add_spell {
	["name"] = 	"Sphere of Destruction",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	50,
	["mana"] = 	48,
	["mana_max"] = 	48,
	["fail"] = 	102,
	["stat"] = 	A_INT,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			local lev = get_astral_lev(Ind)
				if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
					fire_cloud(Ind, GF_MANA, args.dir, (1 + lev * 2), 3, (5 + lev / 5), 9, " conjures up a mana storm of")
				else
					fire_cloud(Ind, GF_INFERNO, args.dir, (1 + lev * 2), 3, (5 + lev / 5), 9, " conjures up inferno of")
				end
	end,
	["info"] = 	function()
			local lev = get_astral_lev(Ind)
			return "dam "..(1 + lev * 2).." rad 3 dur "..(5 + (lev / 5))
	end,
	["desc"] = 	{
			"Enlightened: conjures up a storm of mana.",
			"Corrupted: conjures up a raging inferno."
		}
}

GATEWAY = add_spell {
	["name"] = 	"Gateway",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	40,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	102,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function(args)
				if (players(Ind).lev >= 62 and get_astral_lev(Ind) >= 50) then
					divine_gateway(Ind);
				else
					msg_print(Ind, "\255yYou need Astral Knowledge level of 50 and character level of 62 or higher.");
				end
	end,
	["info"] = 	function()
			return "";
	end,
	["desc"] = 	{
			"Requires level 50 Astral Knowledge and at least character level 62.",
			"Enlightened: nigh-instantaneous wor for every party member on the level.",
			"Corrupted: creates a void jump gate",
			"           (cast once to set first location and then second for the target)."
		}
}
