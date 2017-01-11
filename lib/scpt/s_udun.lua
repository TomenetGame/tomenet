-- handle the udun school

function get_disebolt_dam()
	return 20 + get_level(Ind, DISEBOLT, 47), 9 + get_level(Ind, DISEBOLT, 50)
end

--[[
DRAIN = add_spell {
	["name"] = 	"Drain",
	["school"] = 	{SCHOOL_UDUN},
	["level"] = 	1,
	["mana"] = 	0,
	["mana_max"] = 	0,
	["fail"] = 	20,
	["spell"] = 	function()
			local ret, item, obj, o_name, add

			-- Ask for an item
			ret, item = get_item("What item to drain?", "You have nothing you can drain", USE_INVEN,
				function (obj)
					if (obj.tval == TV_WAND) or (obj.tval == TV_ROD_MAIN) or (obj.tval == TV_STAFF) then
						return TRUE
					end
					return FALSE
				end
			)

			if ret == TRUE then
				-- get the item
				obj = get_object(item)

				add = 0
				if (obj.tval == TV_STAFF) or (obj.tval == TV_WAND) then
					local kind = get_kind(obj)

					add = kind.level * obj.pval * obj.number

					-- Destroy it!
					inven_item_increase(item, -99)
					inven_item_describe(item)
					inven_item_optimize(item)
				end
				if obj.tval == TV_ROD_MAIN then
					add = obj.timeout
					obj.timeout = 0;

					--Combine / Reorder the pack (later)
					player.notice = bor(player.notice, PN_COMBINE, PN_REORDER)
					player.window = bor(player.window, PW_INVEN, PW_EQUIP, PW_PLAYER)
				end
				increase_mana(add)
			end
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{ "Drains the mana contained in wands, staves and rods to increase yours", }
}
]]
GENOCIDE_I = add_spell {
	["name"] = 	"Genocide",
	["school"] = 	{SCHOOL_UDUN},
	["level"] = 	25,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	0,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			genocide(Ind)
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{ "Casts 'Genocide' on all monsters of particular race on the level.", }
}
GENOCIDE_II = add_spell {
	["name"] = 	"Obliteration",
	["school"] = 	{SCHOOL_UDUN},
	["level"] = 	40,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	-40,
	["stat"] = 	A_WIS,
	["spell"] = 	function()
			obliteration(Ind)
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{ "Casts 'Obliteration' on all monsters in your vicinity.", }
}

WRAITHFORM = add_spell {
	["name"] = 	"Wraithform",
	["school"] = 	{SCHOOL_UDUN},
	["level"] = 	35,
	["mana"] = 	40,
	["mana_max"] = 	40,
	["fail"] = 	20,
	["spell"] = 	function()
			local dur = randint(30) + 20 + get_level(Ind, WRAITHFORM, 40)
			set_tim_wraith(Ind, dur)
	end,
	["info"] = 	function()
			return "dur "..(20 + get_level(Ind, WRAITHFORM, 40)).."+d30"
	end,
	["desc"] = 	{
			"Turns you into an immaterial being.",
			"***Automatically projecting***",
	}
}
--[[
FLAMEOFUDUN = add_spell {
	["name"] = 	"Flame of Udun",
	["school"] = 	{SCHOOL_UDUN},
	["level"] = 	35,
	["mana"] = 	70,
	["mana_max"] = 	100,
	["fail"] = 	20,
	["spell"] = 	function()
			set_mimic(randint(15) + 5 + get_level(FLAMEOFUDUN, 30), MIMIC_BALROG)
	end,
	["info"] = 	function()
			return "dur "..(5 + get_level(FLAMEOFUDUN, 30)).."+d15"
	end,
	["desc"] = 	{ "Turns you into a powerful balrog", }
}
]]

DISEBOLT = add_spell {
	["name"] = 	"Disenchantment Ray",
	["school"] = 	{SCHOOL_UDUN},
	["level"] = 	40,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	-40,
	["direction"] = TRUE,
	["ftk"] = 	1,
	["spell"] = 	function(args)
		fire_beam(Ind, GF_DISENCHANT, args.dir, damroll(get_disebolt_dam()), " casts a disenchantment ray for")
	end,
	["info"] = 	function()
		local x, y

		x, y = get_disebolt_dam()
		return "dam "..x.."d"..y
	end,
	["desc"] = 	{
		"Conjures a powerful disenchantment beam.",
		"The damage is nearly irresistible and will increase with level."
	}
}

HELLFIRE_I = add_spell {
	["name"] = 	"Hellfire I",
	["school"] = 	{SCHOOL_UDUN},
	["level"] = 	20,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	-10,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			fire_ball(Ind, GF_HELL_FIRE, args.dir, 20 + get_level(Ind, HELLFIRE_I, 250), 2 + get_level(Ind, HELLFIRE_I, 2), " casts a ball of hellfire for")
		end,
	["info"] = 	function()
			return "dam "..(20 + get_level(Ind, HELLFIRE_I, 250)).." rad "..(2 + get_level(Ind, HELLFIRE_I, 2))
		end,
	["desc"] = 	{ "Conjures a ball of hellfire to burn your foes to ashes.", }
}
HELLFIRE_II = add_spell {
	["name"] = 	"Hellfire II",
	["school"] = 	{SCHOOL_UDUN},
	["level"] = 	40,
	["mana"] = 	40,
	["mana_max"] = 	40,
	["fail"] = 	-75,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			fire_ball(Ind, GF_HELL_FIRE, args.dir, 20 + 250 + get_level(Ind, HELLFIRE_II, 250), 2 + get_level(Ind, HELLFIRE_I, 2), " casts a ball of hellfire for")
		end,
	["info"] = 	function()
			return "dam "..(20 + 250 + get_level(Ind, HELLFIRE_II, 250)).." rad "..(2 + get_level(Ind, HELLFIRE_I, 2))
		end,
	["desc"] = 	{ "Conjures a ball of hellfire to burn your foes to ashes.", }
}

STOPWRAITH = add_spell {
	["name"] = 	"Stop Wraithform",
	["school"] = 	{SCHOOL_UDUN},
	["level"] = 	20,
	["mana"] = 	0,
	["mana_max"] = 	0,
	["fail"] = 	101,
	["spell"] = 	function()
			set_tim_wraith(Ind, 0)
	end,
	["info"] = 	function()
			return ""
	end,
	["desc"] = 	{
			"Immediately returns you to material form.",
			"***Automatically projecting***",
	}
}
