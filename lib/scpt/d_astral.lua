-- The astral school

function get_astral_lev(Ind)
	return (players(Ind).s_info[SKILL_ASTRAL + 1].value + 1) / 2000 + players(Ind).lev / 2
end

function get_astral_dam(Ind)
	return	(3 + get_astral_lev(Ind) / 3), (3 + get_astral_lev(Ind) / 2)
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
	return ""..xx..","..yy

end
POWERBOLT = add_spell
{
	["name"] = 	"Power Bolt",
        ["school"] = 	SCHOOL_ASTRAL,
        ["spell_power"] = 0,
        ["level"] = 	1,
        ["mana"] = 	1,
        ["mana_max"] =  15,
        ["fail"] = 	5,
        ["stat"] =      A_INT,
        ["direction"] = TRUE,
        ["ftk"] = 1,
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
        ["desc"] =	{
        		"Enlightened: conjures up a powerful bolt of mana",
        		"Corrupted: conjures up a powerful bolt of nether",
        		"Neutral: conjures up a bolt of lightning"
		}
}
POWERBEAM = add_spell
{
	["name"] = 	"Power Blast",
        ["school"] = 	SCHOOL_ASTRAL,
        ["spell_power"] = 0,
        ["level"] = 	10,
        ["mana"] = 	8,
        ["mana_max"] =  28,
        ["fail"] = 	5,
        ["stat"] =      A_INT,
        ["direction"] = TRUE,
        ["ftk"] = 1,
        ["spell"] = 	function(args)
	        	local xx, yy
			xx, yy = get_astral_dam(Ind)
			if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
				fire_beam(Ind, GF_LITE, args.dir, damroll(xx,yy), " casts a beam of light for")
			elseif (players(Ind).ptrait == TRAIT_CORRUPTED) then
				fire_beam(Ind, GF_DARK, args.dir, damroll(xx,yy), " casts a beam of unlight for")
			else
				fire_beam(Ind, GF_ELEC, args.dir, damroll(xx,yy), " casts a lightning beam for")
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
	["name"] = 	"Power Ball",
        ["school"] = 	SCHOOL_ASTRAL,
        ["spell_power"] = 0,
        ["level"] = 	15,
        ["mana"] = 	10,
        ["mana_max"] =  30,
        ["fail"] = 	5,
        ["stat"] =      A_INT,
        ["direction"] = TRUE,
        ["ftk"] = 2,
        ["spell"] = 	function(args)
			if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
	        		fire_ball(Ind, GF_MANA, args.dir, damroll(get_astral_dam(Ind)), 2 + get_level(Ind,POWERBALL,2), " casts a ball of mana for")
			elseif (players(Ind).ptrait == TRAIT_CORRUPTED) then
	        		fire_ball(Ind, GF_NETHER, args.dir, damroll(get_astral_dam(Ind)), 2 + get_level(Ind,POWERBALL,2), " casts a nether ball for")
			else	
	        		fire_ball(Ind, GF_ELEC, args.dir, damroll(get_astral_dam(Ind)), 2 + get_level(Ind,POWERBALL,2), " casts a ball of lightning for")
			end
	end,
	["info"] = 	function()
	        	local xx, yy
			xx, yy = get_astral_dam(Ind)
			return "dam "..xx.."d"..yy
	end,
        ["desc"] =	{
        		"Enlightened: conjures up a powerful ball of mana",
        		"Corrupted: conjures up a powerful ball of nether",
			"Neutral: conjures up a ball of lightning"
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
        ["stat"] =      A_WIS,
        ["direction"] = FALSE,
        ["spell"] = 	function()
			divine_vengeance(Ind, (30+get_astral_lev(Ind)*10))
	end,
	["info"] = 	function()
			return "power: "..(30+get_astral_lev(Ind)*10)
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
        ["mana_max"] =  50,
        ["fail"] = 	-98,
        ["stat"] =      A_WIS,
        ["direction"] = FALSE,
        ["spell"] = 	function(args)
				if (get_astral_lev(Ind)>=40) then
					divine_empowerment(Ind, get_astral_lev(Ind)); 
				end
	end,
	["info"] = 	function()
				return "dur: "..(20+get_astral_lev(Ind)/10)
	end,
        ["desc"] =	{
			"Requires astral level of 40",
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
        ["mana_max"] =  50,
        ["fail"] = 	-98,
        ["stat"] =      A_WIS,
        ["direction"] = FALSE,
        ["spell"] = 	function(args)
				if (get_astral_lev(Ind) >= 45) then
					divine_intensify(Ind, get_astral_lev(Ind));
				end
	end,
	["info"] = 	function()
				return "dur: "..(20+get_astral_lev(Ind)/10)
	end,
        ["desc"] =	{
			"Requires astral level of 45",
        		"Enlightened: slows down monsters in sight",
--			"         grants temporary mana and time resistance",
        		"Corrupted: increases your critical chance (+2 base",
			"         +2 per 5 astral levels thereafter)"
		}
}
POWERCLOUD = add_spell
{
	["name"] = 	"Power Cloud",
        ["school"] = 	SCHOOL_ASTRAL,
        ["spell_power"] = 0,
        ["level"] = 	50,
        ["mana"] = 	48,
        ["mana_max"] =  48,
        ["fail"] = 	-98,
        ["stat"] =      A_INT,
        ["direction"] = TRUE,
        ["spell"] = 	function(args)
			local lev = get_astral_lev(Ind)
			if (lev >= 50) then
				if (players(Ind).ptrait == TRAIT_ENLIGHTENED) then
					fire_cloud(Ind, GF_MANA, args.dir, (1 + lev*2), 3, (5 + lev/5), 9, " fires a cloud of mana of")
				else
					fire_cloud(Ind, GF_ROCKET, args.dir, (1 + lev*2), 3, (5 + lev/5), 9, " fires a rocket cloud of")
				end
			end
	end,
	["info"] = 	function()
			local lev = get_astral_lev(Ind)
			return "dam "..(1 + (lev*2)).." rad 3 dur "..(5 + (lev/5))
	end,
        ["desc"] =	{
			"Requires astral level of 50",
        		"Enlightened: summons a cloud of mana",
        		"Corrupted: summons a cloud of blazing heat"
		}
}
GATEWAY = add_spell
{
	["name"] = 	"Gateway",
        ["school"] = 	SCHOOL_ASTRAL,
        ["spell_power"] = 0,
        ["level"] = 	50,
        ["mana"] = 	999,
        ["mana_max"] =  999,
        ["fail"] = 	1,
        ["stat"] =      A_WIS,
        ["direction"] = TRUE,
        ["spell"] = 	function(args)
	end,
	["info"] = 	function()
			return ""
	end,
        ["desc"] =	{
			"DOES NOT WORK (yet)",
			"Requires level 50 Astral Knowledge and at least character level 62",
        		"Enlightened: instantaneous wor for every party member on the level",
        		"Corrupted: creates a void jump gate",
			"         (cast once to set the first location and the second for the target)",
			"         (Maximum of one pair of jump gate is allowed)."
		}
}
