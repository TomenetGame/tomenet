-- handle the udun school

function get_disebolt_dam()
        return 40, 15 + get_level(Ind, DISEBOLT, 50)
end

--[[
DRAIN = add_spell
{
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
        ["desc"] =	{
                        "Drains the mana contained in wands, staves and rods to increase yours",
        }
}
]]
GENOCIDE = add_spell
{
	["name"] = 	"Genocide",
        ["school"] = 	{SCHOOL_UDUN},
        ["level"] = 	25,
        ["mana"] = 	50,
        ["mana_max"] = 	50,
        ["fail"] = 	20,
        ["stat"] =      A_WIS,
        ["extra"] =     function () if get_check("Genocide all monsters near you (y=all in radius, n=race on whole map)? ") == TRUE then return TRUE else return FALSE end end,
        ["spell"] = 	function(args)
                        local type

                        type = 0
                        if get_level(Ind, GENOCIDE) >= 15 then type = 1 end
                        if type == 0 then
                                genocide(Ind)
                        else
                                if args.aux == TRUE then
                                        mass_genocide(Ind)
                                else
                                	genocide(Ind)
                                end
                        end
	end,
	["info"] = 	function()
	                return ""
	end,
        ["desc"] =	{
                        "Genocides all monsters of a race on the level",
                        "At level 15 it can genocide all monsters near you"
        }
}

WRAITHFORM = add_spell
{
	["name"] = 	"Wraithform",
        ["school"] = 	{SCHOOL_UDUN},
        ["level"] = 	35,
        ["mana"] = 	20,
        ["mana_max"] = 	40,
        ["fail"] = 	20,
        ["spell"] = 	function()
                        local dur = randint(30) + 20 + get_level(Ind, WRAITHFORM, 40)
                       	set_tim_wraith(Ind, dur)
                        if player.spell_project > 0 then
                                fire_ball(Ind, GF_WRAITH_PLAYER, 0, dur, player.spell_project, "")
                        end
	end,
	["info"] = 	function()
	                return "dur "..(20 + get_level(Ind, WRAITHFORM, 40)).."+d30"
	end,
        ["desc"] =	{
                        "Turns you into an immaterial being",
                        "***Affected by the Meta spell: Project Spell***",
        }
}
--[[
FLAMEOFUDUN = add_spell
{
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
        ["desc"] =	{
                        "Turns you into a powerful balrog",
        }
}
]]

DISEBOLT = add_spell
{
	["name"] =      "Disenchantment Bolt",
	["school"] =    {SCHOOL_UDUN},
	["level"] =     40,
	["mana"] =      30,
	["mana_max"] =  140,
        ["fail"] =      -40,
	["direction"] = TRUE,
	["spell"] =     function(args)
		fire_bolt(Ind, GF_DISENCHANT, args.dir, damroll(get_disebolt_dam()), " casts a disenchantment bolt for")
	end,
	["info"] =      function()
		local x, y

		x, y = get_disebolt_dam()
		return "dam "..x.."d"..y
	end,
	["desc"] =      {
		"Conjures a powerful disenchantment bolt",
		"The damage is nearly irresistible and will increase with level"
	}
}

HELLFIRE = add_spell
{
        ["name"] =      "Hellfire",
        ["school"] =    {SCHOOL_UDUN},
        ["level"] =     20,
        ["mana"] =      15,
        ["mana_max"] =  40,
        ["fail"] =      30,
        ["direction"] = TRUE,
	["spell"] =     function(args)
			local type
	                type = GF_HELL_FIRE
	                fire_ball(Ind, type, args.dir, 20 + get_level(Ind, HELLFIRE, 500), 2 + get_level(Ind, HELLFIRE, 4), " casts a ball of hellfire for")
	        end,
        ["info"] =      function()
	                return "dam "..(20 + get_level(Ind, HELLFIRE, 500)).." rad "..(2 + get_level(Ind, HELLFIRE, 4))
	        end,
        ["desc"] =      {
	                "Conjures a ball of hellfire to burn your foes to ashes",
		        }
}
