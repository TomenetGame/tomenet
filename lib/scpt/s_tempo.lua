-- Handles thhe temporal school

MAGELOCK = add_spell
{
	["name"] = 	"Magelock",
        ["school"] = 	{SCHOOL_TEMPORAL},
        ["level"] = 	1,
        ["mana"] = 	1,
        ["mana_max"] = 	35,
        ["fail"] = 	10,
        ["direction"] = function () if get_level(Ind, MAGELOCK, 50) >= 30 then return FALSE else return TRUE end end,
        ["spell"] = 	function(args)
        		if get_level(Ind, MAGELOCK, 50) >= 30 then
	        		local ret, x, y, c_ptr

                		if get_level(Ind, MAGELOCK, 50) >= 40 then
        	                	ret, x, y = tgt_pt()
		                        if ret == FALSE then return end
                                else
                                	y = player.py
                                        x = player.px
                                end
                        	cave_set_feat(player.wpos, y, x, 3)
                        else
                                wizard_lock(Ind, args.dir)
                        end
	end,
	["info"] = 	function()
                       	return ""
	end,
        ["desc"] =	{
        		"Magicaly locks a door",
                        "At level 30 it creates a glyph of warding",
                        "At level 40 the glyph can be placed anywhere in the field of vision"
        }
}

SLOWMONSTER = add_spell
{
	["name"] = 	"Slow Monster",
        ["school"] = 	{SCHOOL_TEMPORAL},
        ["level"] = 	10,
        ["mana"] = 	10,
        ["mana_max"] = 	15,
        ["fail"] = 	10,
        ["direction"] = TRUE,
        ["spell"] = 	function(args)
                        if get_level(Ind, SLOWMONSTER, 50) >= 20 then
                        	fire_ball(Ind, GF_OLD_SLOW, args.dir, 40 + get_level(Ind, SLOWMONSTER, 160), 1)
                        else
                        	fire_bolt(Ind, GF_OLD_SLOW, args.dir, 40 + get_level(Ind, SLOWMONSTER, 160))
                        end
	end,
	["info"] = 	function()
                        if get_level(Ind, SLOWMONSTER, 50) >= 20 then
	                       	return "power "..(40 + get_level(Ind, SLOWMONSTER, 160)).." rad 1"
                        else
	                       	return "power "..(40 + get_level(Ind, SLOWMONSTER, 160))
                        end
	end,
        ["desc"] =	{
                        "Magically slows down the passing of time around a monster",
                        "At level 20 it affects a zone"
        }
}

ESSENSESPEED = add_spell
{
	["name"] = 	"Essense of Speed",
        ["school"] = 	{SCHOOL_TEMPORAL},
        ["level"] = 	15,
        ["mana"] = 	20,
        ["mana_max"] = 	40,
        ["fail"] = 	10,
        ["spell"] = 	function()
        		set_fast(Ind, 10 + randint(10) + get_level(Ind, ESSENSESPEED, 50), 5 + get_level(Ind, ESSENSESPEED, 20))
                        if player.spell_project > 0 then
                                fire_ball(Ind, GF_SPEED_PLAYER, 0, 50 + get_level(Ind, ESSENSESPEED, 50), player.spell_project)
                        end
	end,
	["info"] = 	function()
                       	return "dur "..(10 + get_level(Ind, ESSENSESPEED, 50)).."+d10 speed "..(5 + get_level(Ind, ESSENSESPEED, 20))
	end,
        ["desc"] =	{
        		"Magicaly increases the passing of time around you",
                        "***Affected by the Meta spell: Project Spell***",
        }
}

BANISHMENT = add_spell
{
	["name"] = 	"Banishment",
        ["school"] = 	{SCHOOL_TEMPORAL, SCHOOL_CONVEYANCE},
        ["level"] = 	30,
        ["mana"] = 	30,
        ["mana_max"] = 	40,
        ["fail"] = 	10,
        ["spell"] = 	function()
                        project_los(Ind, GF_AWAY_ALL, 40 + get_level(Ind, BANISHMENT, 160))
                        if get_level(Ind, BANISHMENT, 50) >= 15 then
                                project_los(Ind, GF_STASIS, 20 + get_level(Ind, BANISHMENT, 120))
                        end
	end,
	["info"] = 	function()
                     	return "power "..(40 + get_level(Ind, BANISHMENT, 160))
	end,
        ["desc"] =	{
        		"Disrupt the space/time continuum in your area and teleports all monsters away",
                        "At level 15 it also may lock them in a time bubble for some turns"
        }
}
