-- handle the thought contact school (Attunement)
--((static: sense/esp))

--[[
MTAUNT = add_spell {
	["name"] = 	"Cause Agression",
	["school"] = 	{SCHOOL_TCONTACT},
	["am"] = 	50,
	["spell_power"] = 0,
	["blind"] = 	0,
	["level"] = 	5,
	["mana"] = 	10,
	["mana_max"] = 	20,
	["fail"] = 	20,
	["spell"] = 	function()
			end,
	["info"] = 	function()
			return ""
			end,
	["desc"] = 	{ "Causes a monster to charge and attack you in melee", }
}

MDISTRACT = add_spell {
	["name"] = 	"Divert Attention",
	["school"] = 	{SCHOOL_TCONTACT},
	["am"] = 	50,
	["spell_power"] = 0,
	["blind"] = 	0,
	["level"] = 	10,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	20,
	["spell"] = 	function()
			end,
	["info"] = 	function()
			return ""
			end,
	["desc"] = 	{
			"Diverts attention of a monster next to you",
			"so it attacks another player who is standing",
			"right next to it (if any) instead of you.",
	}
}
]]

MSELFKNOW = add_spell {
	["name"] = 	"Self-Reflection",
	["school"] = 	{SCHOOL_TCONTACT},
	["am"] = 	33,
	["spell_power"] = 0,
	["blind"] = 	0,
	["level"] = 	15,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	20,
	["spell"] = 	function()
			self_knowledge(Ind)
			end,
	["info"] = 	function()
			return ""
			end,
	["desc"] = 	{ "Find out more about yourself.", }
}

MBOOST = add_spell {
	["name"] = 	"Willpower",
	["school"] = 	{SCHOOL_TCONTACT},
	["am"] = 	33,
	["spell_power"] = 0,
	["blind"] = 	0,
	["confusion"] = 0,
	["level"] = 	3,
	["mana"] = 	7,
	["mana_max"] = 	7,
	["fail"] = 	5,
	["spell"] = 	function()
			fire_ball(Ind, GF_MINDBOOST_PLAYER, 0, 40 + get_level(Ind, MBOOST, 180), 2, " focusses on your mind, unlocking something!")
			set_mindboost(Ind, 20 + get_level(Ind, MBOOST, 90), 15 + randint(6))
			end,
	["info"] = 	function()
			return "dur 15+d6"
			end,
	["desc"] = 	{
			"Boosts your willpower to unleash hidden potential,",
--			"improving both your performance and resilience",
			"improving your performance and resilience to various effects.",
			"At level 25 it will also allow you to attack slightly faster.",
			"***Automatically projecting***",
-- what it infact does: tohit, res.fear, %res.conf, %res.para, %res.slow, +1 ea (later)
	}
}

MHASTE = add_spell {
	["name"] = 	"Accelerate Nerves",
	["school"] = 	{SCHOOL_TCONTACT},
	["am"] = 	50,
	["spell_power"] = 0,
	["blind"] = 	0,
	["level"] = 	20,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	1,
	["spell"] = 	function()
			local spd
			spd = get_level(Ind, MHASTE, 25)
			if spd > 10 then
				spd = 10
			end
			set_fast(Ind, 30 + randint(10) + get_level(Ind, MHASTE, 17), spd)
			end,
	["info"] = 	function()
			local spd
			spd = get_level(Ind, MHASTE, 25)
			if spd > 10 then
				spd = 10
			end
			return "dur " .. (30 + get_level(Ind, MHASTE, 17)) .. "+d10 speed +" .. spd
			end,
	["desc"] = 	{
			"Accelerates your nerve functions and metabolism",
			"by direct mental infusion, speeding you up.",
	}
}

MCURE = add_spell {
	["name"] = 	"Clear Mind",
	["school"] = 	{SCHOOL_TCONTACT},
	["am"] = 	50,
	["spell_power"] = 0,
	["blind"] = 	0,
	["confusion"] = 0,
	["level"] = 	3,
	["mana"] = 	8,
	["mana_max"] = 	8,
	["fail"] = 	5,
	["spell"] = 	function()
			if get_level(Ind, MCURE, 50) >= 10 then
				set_afraid(Ind, 0)
				set_res_fear(Ind, get_level(Ind, MCURE, 50))
				set_confused(Ind, 0)
				set_image(Ind, 0)

				fire_ball(Ind, GF_REMIMAGE_PLAYER, 0, 1, 4, " waves over your eyes.")
			else
				if get_level(Ind, MCURE, 50) >= 5 then
					set_afraid(Ind, 0)
					set_res_fear(Ind, get_level(Ind, MCURE, 50))
					set_confused(Ind, 0)

					fire_ball(Ind, GF_REMCONF_PLAYER, 0, 1, 4, " waves over your eyes.")
				else
					set_afraid(Ind, 0)
					set_res_fear(Ind, get_level(Ind, MCURE, 50))

					fire_ball(Ind, GF_REMFEAR_PLAYER, 0, 1, 4, " waves over your eyes.")
				end
			end
			end,
	["info"] = 	function()
			return ""
			end,
	["desc"] = 	{
			"Frees your mind from fear, confusion (level 5)",
			"and hallucinations (level 10).",
			"***Automatically projecting***",
	}
}

MSANITY = add_spell {
	["name"] = 	"Stabilize Thoughts",
	["school"] = 	{SCHOOL_TCONTACT},
	["am"] = 	50,
	["spell_power"] = 0,
	["blind"] = 	0,
	["confusion"] = 0,
	["level"] = 	25,
	["mana"] = 	65,
	["mana_max"] = 	65,
	["fail"] = 	40,
	["spell"] = 	function()
			set_afraid(Ind, 0)
			set_res_fear(Ind, get_level(Ind, MSANITY, 50))
			set_confused(Ind, 0)
			set_image(Ind, 0)

			heal_insanity(Ind, 15 + get_level(Ind, MSANITY, 55))
			if player.csane == player.msane then
				msg_print(Ind, "You are in full command of your mental faculties.")
			end

			fire_ball(Ind, GF_SANITY_PLAYER, 0, 30 + get_level(Ind, MSANITY, 110), 1, " focusses on your mind.")
			end,
	["info"] = 	function()
			return "cures "..(15 + get_level(Ind, MSANITY, 55)).." SN"
			end,
	["desc"] = 	{
			"Cures some insanity and removes malicious effects.",
			"***Automatically projecting***",
	}
}

MSENSEMON = add_spell {
	["name"] = 	"Telepathy",
	["school"] = 	{SCHOOL_TCONTACT},
	["am"] = 	50,
	["spell_power"] = 0,
	["blind"] = 	0,
--	["level"] = 	1,
--	["mana"] = 	3,
	["level"] = 	20,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	5,
	["spell"] = 	function()
--			if get_level(Ind, MSENSEMON, 50) >= 20 then
				set_tim_esp(Ind, 20 + randint(10) + get_level(Ind, MSENSEMON, 50))
--			end
--			set_tim_invis(Ind, 10 + get_level(Ind, HSENSEMON, 50))
--			detect_creatures(Ind) <- detecting empty-minded monsters isn't mindcrafter ability
			end,
	["info"] = 	function()
			return "dur 20+d10+d"..get_level(Ind, MSENSEMON, 50)
			end,
	["desc"] = 	{
--			"Detects all nearby non-invisible creatures once and also lets",
--			"you see invisible creatures for a while.",
--			"At level 20 it lets you sense the presence creatures for a while.",
			"Lets you sense the presence creatures for a while.",
	}
}

MIDENTIFY = add_spell {
	["name"] = 	"Recognition",
	["school"] = 	{SCHOOL_TCONTACT, SCHOOL_MINTRUSION},
	["am"] = 	33,
	["spell_power"] = 0,
	["level"] = 	20,
	["mana"] = 	7,
	["mana_max"] = 	7,
	["fail"] = 	-5,
	["spell"] = 	function()
			ident_spell(Ind)
			end,
	["info"] = 	function()
			return ""
			end,
	["desc"] = 	{ "Recognizes magic, allowing you to identify an item.", }
}

if (def_hack("TEMP1", nil) == 0) then
MTELEKINESIS = add_spell {
	["name"] = 	"Telekinesis II",
	["school"] = 	{SCHOOL_TCONTACT, SCHOOL_PPOWER},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	35,
	["mana"] = 	25,
	["mana_max"] = 	25,
	["fail"] = 	-5,
	["get_item"] = 	{
			["prompt"] = 	"Teleport which object? ",
			["inven"] = 	TRUE,
			["get"] = 	function (obj)
					if obj.weight * obj.number <= 4 + get_level(Ind, MTELEKINESIS, 350, 0) then
						return TRUE
					end
					return FALSE
			end,
	},
	["spell"] = 	function(args)
			if args.item == -1 then
			    msg_print(Ind, "Telekinesis has been cancelled.")
			    return
			end
			if player.inventory[1 + args.item].weight * player.inventory[1 + args.item].number <= 4 + get_level(Ind, MTELEKINESIS, 350, 0) then
				player.current_telekinesis = player.inventory[1 + args.book]
				telekinesis_aux(Ind, args.item)
			else
				msg_print(Ind, "Pfft trying to hack your client ? pretty lame ...")
			end
			end,
	["info"] = 	function()
			return "max wgt "..((4 + get_level(Ind, MTELEKINESIS, 350, 0)) / 10).."."..(imod(4 + get_level(Ind, MTELEKINESIS, 350, 0), 10))
			end,
	["desc"] = 	{
			"Inscribe your book with @Pplayername, cast it, select an item",
			"and the item will be teleported to that player whereever he/she might",
			"be in the Universe.",
	}
}
else
MTELEKINESIS = add_spell {
	["name"] = 	"Telekinesis II",
	["school"] = 	{SCHOOL_TCONTACT, SCHOOL_PPOWER},
	["am"] = 	50,
	["spell_power"] = 0,
	["level"] = 	35,
	["mana"] = 	25,
	["mana_max"] = 	25,
	["fail"] = 	-5,
	["spell"] = 	function(args)
			telekinesis(Ind, player.inventory[1 + args.book], 4 + get_level(Ind, MTELEKINESIS, 400, 0))
			end,
	["info"] = 	function()
			return "max wgt "..((4 + get_level(Ind, MTELEKINESIS, 400, 0)) / 10).."."..(imod(4 + get_level(Ind, MTELEKINESIS, 400, 0), 10))
			end,
	["desc"] = 	{
			"Inscribe your book with @Pplayername, cast it, select an item",
			"and the item will be teleported to that player whereever he/she might",
			"be in the Universe.",
	}
}
end
