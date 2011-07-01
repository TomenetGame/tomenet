-- The astral school ///update-dummy-bytes

function get_astral_lev(Ind)
	return ((players(Ind).s_info[SKILL_ASTRAL + 1].value + 1) / 2000 + players(Ind).lev / 2)
end

--[[ 
manathrust does 3...53 d 1 .. 21 damage base
powerbolt/beam doing 3 .. 53 d 1 .. 26 might be OK because:
1. mana school on its own is severely lacking (manathrust as main nuke only really viable with spell power)
2. reflection 

I'll let you know about #1 when Pikachu is higher level :) 
]]
function get_astral_dam(Ind)
	return (3 + ((get_astral_lev(Ind) * 3) / 5)), (1 + get_astral_lev(Ind) / 2)
end


function get_veng_power(Ind)
	return (get_astral_lev(Ind)*get_astral_lev(Ind) / 5);
end
--orb of draining (main nuke for priests) go from 20 + (0 .. 475)
--fireflash (main nuke for mages) go from 20 + (0 .. 500)
--PROPOSED: due to lack of SUSECP_MANA/NETHER, we should be able to at least average out to ~750 per nuke (similar to resisted FF on SUSCEPT and FIRE_IMM monsters (754, to be exact))
-- proposed: 40 + astral_lev*15  => 40 + (0 .. 750) at lvl 50
function get_astral_dam_ball(Ind)
--	return 40 + get_astral_lev(Ind) * 12;
	return (3 + ((get_astral_lev(Ind) * 2) / 5)), (3 + get_astral_lev(Ind) / 1)
end

function get_astral_bonus_hp(Ind)
	if (players(Ind).ptrait == TRAIT_ENLIGHTNED) then 
		return 0
	end

	if (get_astral_lev(Ind) >= 55) then
		return 3
	elseif (get_astral_lev(Ind) > 52) then
		return 2
	elseif (get_astral_lev(Ind) > 40) then
		return 1
	end
	return 0
end

function pfft()
	local xx, yy
	xx, yy = get_astral_dam(Ind)
	return ""..xx..", "..yy

end
POWERBOLT = add_spell
{
	["name"] = 	"Power Bolt",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	1,
	["mana"] = 	1,
	["mana_max"] = 	15,
	["fail"] = 	5,
	["stat"] = 	A_INT,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
				fire_bolt(Ind, GF_MANA, args.dir, damroll(get_astral_dam(Ind)), " casts a mana bolt for")
			elseif (players(Ind).ptrait == TRAIT_CORRUPTED) then
				fire_bolt(Ind, GF_NETHER, args.dir, damroll(get_astral_dam(Ind)), " casts a nether bolt for")
			else
				fire_bolt(Ind, GF_ELEC, args.dir, damroll(get_astral_dam(Ind)), " casts a lightning bolt for")
			end
	end,
	["info"] = 	function()
			local xx, yy 
			xx, yy = get_astral_dam(Ind)
			return "dam "..xx.."d"..yy
	end,
	["desc"] = 	{
			"Enlightened: conjures up a powerful bolt of mana",
			"Corrupted: conjures up a powerful bolt of nether",
			"Neutral: conjures up a bolt of lightning"
		}
}
POWERBEAM = add_spell
{
	["name"] = 	"Power Ray",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	5,
	["mana"] = 	2,
	["mana_max"] = 	25,
	["fail"] = 	10,
	["stat"] = 	A_INT,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
			local xx, yy
			xx, yy = get_astral_dam(Ind)
			if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
				fire_beam(Ind, GF_LITE, args.dir, damroll(xx, yy), " casts a beam of light for")
			elseif (players(Ind).ptrait == TRAIT_CORRUPTED) then
				fire_beam(Ind, GF_DARK, args.dir, damroll(xx, yy), " casts a beam of unlight for")
			else
				fire_beam(Ind, GF_ELEC, args.dir, damroll(xx, yy), " casts a lightning beam for")
			end
	end,
	["info"] = 	function()
			local xx, yy
			xx, yy = get_astral_dam(Ind)
			return "dam "..xx.."d"..yy
	end,
	["desc"] =	{
			"Enlightened: conjures up a powerful beam of light",
			"Corrupted: conjures up a powerful beam of unlight",
			"Neutral: conjures up a beam of lightning"
		}
}
POWERBALL = add_spell
{
	["name"] = 	"Power Blast",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	10,
	["mana"] = 	5,
	["mana_max"] = 	30,
	["fail"] = 	15,
	["stat"] = 	A_INT,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
				fire_ball(Ind, GF_MANA, args.dir, damroll(get_astral_dam_ball(Ind)), 2 + get_level(Ind, POWERBALL, 2), " casts a mana ball for")
				--fire_ball(Ind, GF_MANA, args.dir, get_astral_dam_ball(Ind), 2 + get_level(Ind, POWERBALL, 2), " casts a ball of mana for")
			elseif (players(Ind).ptrait == TRAIT_CORRUPTED) then
				fire_ball(Ind, GF_NETHER, args.dir, damroll(get_astral_dam_ball(Ind)), 2 + get_level(Ind, POWERBALL, 2), " casts a nether ball for")
				--fire_ball(Ind, GF_NETHER, args.dir, get_astral_dam_ball(Ind), 2 + get_level(Ind, POWERBALL, 2), " casts a ball of nether for")
			else	
				fire_ball(Ind, GF_ELEC, args.dir, damroll(get_astral_dam_ball(Ind)), 2 + get_level(Ind, POWERBALL, 2), " casts a lightning ball for")
				--fire_ball(Ind, GF_ELEC, args.dir, get_astral_dam_ball(Ind), 2 + get_level(Ind, POWERBALL, 2), " casts a ball of lightning for")
			end
	end,
	["info"] = 	function()
			local xx, yy
			--xx = get_astral_dam_ball(Ind)
			xx, yy = get_astral_dam_ball(Ind)
			return "dam "..xx.."d"..yy.." rad "..2 + get_level(Ind, POWERBALL, 2)
			--return "dam "..xx.." rad "..2 + get_level(Ind, POWERBALL, 2);
	end,
	["desc"] =	{
			"Enlightened: conjures up a powerful ball of mana",
			"Corrupted: conjures up a powerful ball of nether",
			"Neutral: conjures up a ball of lightning"
		}
}

RELOCATION = add_spell
{
	["name"] =	"Relocation",
	["school"] =	SCHOOL_ASTRAL,
	["level"] =	22, --the same level that one gets initiated (!) (ie 20 + 2)
	["mana"] =	20,
	["mana_max"] =	20,
	["fail"] =	10,
	["spell_power"] = 0,
	["am"] =	67,
	["blind"] =	0,
	["spell"] =	function(args)
		local dur = randint(21 - get_level(Ind, RECALL, 15)) + 15 - get_level(Ind, RECALL, 10)
		if args.book < 0 then return end
		set_recall(Ind, dur, player.inventory[1 + args.book])
	end,
	["info"] =	function()
		return "dur "..(15 - get_level(Ind, RECALL, 10)).."+d"..(21 - get_level(Ind, RECALL, 15))
	end,
	["desc"] =	{
		"Recalls into the dungeon, back to the surface or across the world.",
	}
}

VENGEANCE = add_spell
{
	["name"] = 	"Vengeance",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	30,
	["mana"] = 	50,
	["mana_max"] =  50,
	["fail"] = 	-98,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			divine_vengeance(Ind, get_veng_power(Ind));
	end,
	["info"] = 	function()
			return "power "..get_veng_power(Ind);
	end,
	["desc"] =	{
			"Enlightened: summons party member on the same area to you",
			"         (Also teleports all monsters in LOS to you!)",
			"Corrupted: damages all monsters in sight"
		}
}
EMPOWERMENT = add_spell
{
	["name"] = 	"Empowerment",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	40,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	-98,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["am"] =	33,
	["blind"] =	0,
	["spell"] = 	function(args)
--				if (get_astral_lev(Ind) >= 40) then
					divine_empowerment(Ind, get_astral_lev(Ind));
--				end
	end,
	["info"] = 	function()
				return "dur "..(20 + get_astral_lev(Ind) / 10)
	end,
	["desc"] =	{
--			"Requires astral level of 40",
			"Enlightened: incite self fury",
			"Corrupted: increases your hit points"
		}
}
INTENSIFY = add_spell
{
	["name"] = 	"The Silent Force",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	45,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	-98,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["am"] =	67,
	["spell"] = 	function(args)
--				if (get_astral_lev(Ind) >= 45) then
					divine_intensify(Ind, get_astral_lev(Ind));
--				end
	end,
	["info"] = 	function()
				return "dur "..(20 + get_astral_lev(Ind) / 10)
	end,
	["desc"] =	{
--			"Requires astral level of 45",
			"Enlightened: slows down monsters in sight",
			"         grants temporary mana and time resistance",
			"Corrupted: increases your critical chance (+2 base",
			"         +2 per 5 astral levels thereafter)"
		}
}
POWERCLOUD = add_spell
{
	["name"] = 	"Sphere of Destruction",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	50,
	["mana"] = 	48,
	["mana_max"] = 	48,
	["fail"] = 	-98,
	["stat"] = 	A_INT,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			local lev = get_astral_lev(Ind)
--			if (lev >= 50) then
				if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
					fire_cloud(Ind, GF_MANA, args.dir, (1 + lev * 2), 3, (5 + lev / 5), 9, " conjures up a mana storm of")
				else
					fire_cloud(Ind, GF_INFERNO, args.dir, (1 + lev * 2), 3, (5 + lev / 5), 9, " conjures up a raging inferno of")
				end
--			end
	end,
	["info"] = 	function()
			local lev = get_astral_lev(Ind)
			return "dam "..(1 + (lev * 2)).." rad 3 dur "..(5 + (lev / 5))
	end,
	["desc"] =	{
--			"Requires astral level of 50",
			"Enlightened: conjures up a storm of mana",
			"Corrupted: conjures up raging inferno"
		}
}
GATEWAY = add_spell
{
	["name"] = 	"Gateway",
	["school"] = 	SCHOOL_ASTRAL,
	["spell_power"] = 0,
	["level"] = 	40,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	-98,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function(args)
				if (players(Ind).lev >= 62 and get_astral_lev(Ind) >= 50) then
					divine_gateway(Ind);
				end
	end,
	["info"] = 	function()
			return "";
	end,
	["desc"] =	{
			"Requires level 50 Astral Knowledge and at least character level 62",
			"Enlightened: instantaneous wor for every party member on the level",
			"Corrupted: creates a void jump gate",
			"         (cast once to set the first location and the second for the target)",
			"         (Maximum of one pair of jump gate is allowed)."
		}
}
