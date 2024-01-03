-- handle the udun school

function get_disebolt_dam()
	return 20 + get_level(Ind, DISEBOLT, 47), 15 + get_level(Ind, DISEBOLT, 50)
end

--[[
DRAIN = add_spell {
	["name"] = 	"Drain",
	["name2"] = 	"Drain",
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
	["name2"] = 	"Geno",
	["school"] = 	{SCHOOL_UDUN},
	["level"] = 	30,
	["mana"] = 	50,
	["mana_max"] = 	50,
	["fail"] = 	-10,
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
	["name2"] = 	"Oblit",
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
	["name2"] = 	"Wraith",
	["school"] = 	{SCHOOL_UDUN},
	["level"] = 	43,
	["mana"] = 	40,
	["mana_max"] = 	40,
	["fail"] = 	-40,
	["spell"] = 	function()
			local dur = randint(30) + 20 + get_level(Ind, WRAITHFORM, 40)

			set_tim_wraith(Ind, dur)
	end,
	["info"] = 	function()
			return "dur "..(20 + get_level(Ind, WRAITHFORM, 40)).."+d30"
	end,
	["desc"] = 	{ "Turns you into an immaterial being.", }
}

DISEBOLT = add_spell {
	["name"] = 	"Disenchantment Ray",
	["name2"] = 	"DisRay",
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
	["name2"] = 	"HFire I",
	["school"] = 	{SCHOOL_UDUN},
	["level"] = 	20,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	-10,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			fire_ball(Ind, GF_HELLFIRE, args.dir, 20 + get_level(Ind, HELLFIRE_I, 250), 2 + get_level(Ind, HELLFIRE_I, 2), " casts a ball of hellfire for")
		end,
	["info"] = 	function()
			return "dam "..(20 + get_level(Ind, HELLFIRE_I, 250)).." rad "..(2 + get_level(Ind, HELLFIRE_I, 2))
		end,
	["desc"] = 	{ "Conjures a ball of hellfire to burn your foes to ashes.", }
}
HELLFIRE_II = add_spell {
	["name"] = 	"Hellfire II",
	["name2"] = 	"HFire II",
	["school"] = 	{SCHOOL_UDUN},
	["level"] = 	40,
	["mana"] = 	40,
	["mana_max"] = 	40,
	["fail"] = 	-75,
	["direction"] = TRUE,
	["ftk"] = 	2,
	["spell"] = 	function(args)
			fire_ball(Ind, GF_HELLFIRE, args.dir, 20 + 250 + get_level(Ind, HELLFIRE_II, 250), 2 + get_level(Ind, HELLFIRE_I, 2), " casts a ball of hellfire for")
		end,
	["info"] = 	function()
			return "dam "..(20 + 250 + get_level(Ind, HELLFIRE_II, 250)).." rad "..(2 + get_level(Ind, HELLFIRE_I, 2))
		end,
	["desc"] = 	{ "Conjures a ball of hellfire to burn your foes to ashes.", }
}

STOPWRAITH = add_spell {
	["name"] = 	"Stop Wraithform",
	["name2"] = 	"SWraith",
	["school"] = 	{SCHOOL_UDUN},
	["level"] = 	43,
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

CONFUSE_I = add_spell {
	["name"] = 	"Confusion I",
	["name2"] = 	"Conf I",
	["school"] = 	{SCHOOL_UDUN},
	["level"] = 	5,
	["mana"] = 	5,
	["mana_max"] = 	5,
	["fail"] = 	10,
	["am"] = 	75,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		fire_bolt(Ind, GF_OLD_CONF, args.dir, 5 + get_level(Ind, CONFUSE_I, 100), "focusses on your mind")
	end,
	["info"] = 	function()
		return "power "..(5 + get_level(Ind, CONFUSE_I, 100))
	end,
	["desc"] = 	{ "Tries to manipulate the mind of a monster to confuse it.", }
}
CONFUSE_II = add_spell {
	["name"] = 	"Confusion II",
	["name2"] = 	"Conf II",
	["school"] = 	{SCHOOL_UDUN},
	["level"] = 	20,
	["mana"] = 	15,
	["mana_max"] = 	15,
	["fail"] = 	-15,
	["am"] = 	75,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		fire_ball(Ind, GF_OLD_CONF, args.dir, 5 + get_level(Ind, CONFUSE_I, 100), 2, "focusses on your mind")
	end,
	["info"] = 	function()
		return "power "..(5 + get_level(Ind, CONFUSE_I, 100)).." rad 2"
	end,
	["desc"] = 	{ "Tries to manipulate the mind of all monsters in a small area to confuse them.", }
}
STUN_I = add_spell {
	["name"] = 	"Stun I",
	["name2"] = 	"Stun I",
	["school"] = 	{SCHOOL_UDUN},
	["level"] = 	15,
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	0,
	["am"] = 	75,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		fire_bolt(Ind, GF_STUN, args.dir, 10 + get_level(Ind, STUN_I, 50), "")
	end,
	["info"] = 	function()
		return ""
	end,
	["desc"] = 	{ "Tries to manipulate the mind of a monster to stun it.", }
}
STUN_II = add_spell {
	["name"] = 	"Stun II",
	["name2"] = 	"Stun II",
	["school"] = 	{SCHOOL_UDUN},
	["level"] = 	40,
	["mana"] = 	60,
	["mana_max"] = 	60,
	["fail"] = 	-65,
	["am"] = 	75,
	["direction"] = TRUE,
	["spell"] = 	function(args)
		fire_ball(Ind, GF_STUN, args.dir, 10 + get_level(Ind, STUN_I, 50), 2, "")
	end,
	["info"] = 	function()
		return ""
	end,
	["desc"] = 	{ "Tries to manipulate the minds of all monsters in a small area to stun them.", }
}
