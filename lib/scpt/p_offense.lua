-- handle the holy offense school

HCURSE = add_spell
{
        ["name"] =      "Curse",
        ["school"] =    {SCHOOL_HOFFENSE},
        ["am"] =	75,
        ["level"] =     1,
        ["mana"] =      2,
        ["mana_max"] =  30,
        ["fail"] =      10,
	["stat"] =	A_WIS,
        ["direction"] = function () if get_level(Ind, HCURSE, 50) >= 25 then return FALSE else return TRUE end end,
        ["spell"] =     function(args)
                        if get_level(Ind, HCURSE, 50) >= 25 then
				project_los(Ind, GF_CURSE, 10 + get_level(Ind, HCURSE, 150), "points and curses for")
                        elseif get_level(Ind, HCURSE, 50) >= 15 then
				fire_grid_beam(Ind, GF_CURSE, args.dir, 10 + get_level(Ind, HCURSE, 150), "points and curses for")
			else
				fire_grid_bolt(Ind, GF_CURSE, args.dir, 10 + get_level(Ind, HCURSE, 150), "points and curses for")
                        end
        end,
        ["info"] =      function()
                        return "power "..(10 + get_level(Ind, HCURSE, 150))
        end,
        ["desc"] =      {
                        "Randomly causes confusion damage, slowness or blindness.",
                        "At level 15 it passes through monsters, affecting those behind as well",
                        "At level 25 it affects all monsters in sight",
        }
}

HGLOBELIGHT = add_spell
{
	["name"] = 	"Holy Light",
        ["school"] = 	{SCHOOL_HOFFENSE, SCHOOL_HSUPPORT},
        ["am"] =	75,
        ["level"] = 	1,
        ["mana"] = 	2,
        ["mana_max"] = 	30,
        ["fail"] = 	10,
	["stat"] =      A_WIS,
        ["spell"] = 	function()
		local ret, dir

                if get_level(Ind, HGLOBELIGHT, 50) >= 3 then lite_area(Ind, 10, 4)
                else lite_room(Ind, player.wpos, player.py, player.px) end
                if get_level(Ind, HGLOBELIGHT, 50) >= 8 then
		        fire_ball(Ind, GF_LITE, 0, (10 + get_level(Ind, HGLOBELIGHT, 100)) * 2, 5 + get_level(Ind, HGLOBELIGHT, 6), " calls a globe of light of")
		end
		msg_print(Ind, "You are surrounded by a globe of light")
	end,
	["info"] = 	function()
        	if get_level(Ind, HGLOBELIGHT, 50) >= 8 then
			return "dam "..(10 + get_level(Ind, HGLOBELIGHT, 100)).." rad "..(5 + get_level(Ind, HGLOBELIGHT, 6))
                else
                	return ""
                end
	end,
        ["desc"] =	{
        		"Creates a globe of pure light",
        		"At level 3 it starts damaging monsters",
        		"At level 8 it starts creating a more powerful kind of light"
        }
}

if (def_hack("TEST_SERVER", nil)) then
HCURSEDD = add_spell
{
        ["name"] =      "Cause wounds",
        ["school"] =    {SCHOOL_HOFFENSE},
        ["am"] =	75,
        ["level"] =     5,
        ["mana"] =      1,
        ["mana_max"] =  20,
        ["fail"] =      15,
	["stat"] =	A_WIS,
	["ftk"] = 1,
        ["spell"] =     function(args)
			fire_grid_bolt(Ind, GF_MISSILE, args.dir, 10 + get_level(Ind, HCURSEDD, 300), "points and curses for")
        end,
        ["info"] =      function()
                        return "power "..(10 + get_level(Ind, HCURSEDD, 300))
        end,
        ["desc"] =      {
                        "Curse an enemy, causing wounds.",
        }
}
end

HORBDRAIN = add_spell
{
	["name"] = 	"Orb Of Draining",
        ["school"] = 	{SCHOOL_HOFFENSE},
        ["am"] =	75,
        ["level"] = 	20,
        ["mana"] = 	5,
        ["mana_max"] = 	25,
        ["fail"] = 	30,
	["stat"] =      A_WIS,
        ["direction"] = TRUE,
        ["ftk"] = 2,
        ["spell"] = 	function(args)
                local type
        	type = GF_HOLY_ORB
    		fire_ball(Ind, type, args.dir, 20 + get_level(Ind, HORBDRAIN, 475), 2 + get_level(Ind, HORBDRAIN, 5), " casts a holy orb for")
	end,
	["info"] = 	function()
		return "dam "..(20 + get_level(Ind, HORBDRAIN, 475)).." rad "..(2 + get_level(Ind, HORBDRAIN, 5))
	end,
        ["desc"] =	{
        		"Calls an holy orb to devour the evil",
        }
}

HEXORCISM = add_spell
{
	["name"] = 	"Exorcism",
        ["school"] = 	{SCHOOL_HOFFENSE},
        ["am"] =	75,
        ["level"] = 	11,
        ["mana"] = 	15,
        ["mana_max"] = 	100,
        ["fail"] = 	30,
	["stat"] =      A_WIS,
        ["spell"] = 	function(args)
                local type
		if get_level(Ind, HEXORCISM, 50) < 20 then
		    dispel_undead(Ind, 10 + get_level(Ind, HEXORCISM, 200))
		elseif get_level(Ind, HEXORCISM, 50) < 30 then
		    dispel_undead(Ind, 10 + get_level(Ind, HEXORCISM, 200))
		    dispel_demons(Ind, 10 + get_level(Ind, HEXORCISM, 200))
		else
		    dispel_evil(Ind, 10 + get_level(Ind, HEXORCISM, 200))
		end
	end,
	["info"] = 	function()
		return "dam "..(10 + get_level(Ind, HEXORCISM, 200))
	end,
        ["desc"] =	{
        		"Dispels nearby undead",
			"At level 20 it dispels all demons",
			"At level 30 it dispels all evil",
        }
}

HDRAINLIFE = add_spell
{
	["name"] = 	"Drain Life",
        ["school"] = 	{SCHOOL_HOFFENSE},
        ["am"] =	75,
        ["level"] = 	20,
        ["mana"] = 	5,
        ["mana_max"] = 	80,
        ["fail"] = 	30,
	["stat"] =      A_WIS,
        ["direction"] = TRUE,
        ["spell"] = 	function(args)
                local type
    		drain_life(Ind, args.dir, 10 + get_level(Ind, HDRAINLIFE, 10))
		hp_player(Ind, player.ret_dam / 2)
	end,
	["info"] = 	function()
		return "drains "..(10 + get_level(Ind, HDRAINLIFE, 10)).."% life"
	end,
        ["desc"] =	{
        		"Drains life from a target, which must not be non-living or undead.",
        }
}

HRELSOULS = add_spell
{
	["name"] = 	"Release Souls",
	["school"] =	{SCHOOL_HOFFENSE},
        ["am"] =	75,
	["level"] =	10,
	["mana"] = 	10,
	["mana_max"] =	150,
	["fail"] =	25,
	["stat"] =      A_WIS,
	["spell"] =	function(args)
			dispel_undead(Ind, 10 + get_level(Ind, HRELSOULS, 1000))
			end,
	["info"] = 	function()
		return "dam "..(10 + get_level(Ind, HRELSOULS, 1000))
	end,
        ["desc"] =	{
        		"Banishes nearby undead.",
			}
}

HDRAINCLOUD = add_spell
{
        ["name"] =      "Doomed Grounds",
        ["school"] =    {SCHOOL_HOFFENSE},
        ["am"] =	75,
        ["level"] =     40,     -- pointless for crap with low lvl anyway
        ["mana"] =      50,
        ["mana_max"] =  100,
        ["fail"] =      -30,
	["stat"] = 	A_WIS,
        ["direction"] = TRUE,
        ["spell"] =     function(args)
--			fire_cloud(Ind, GF_OLD_DRAIN, args.dir, 9999, 3, 8 + get_level(Ind, HDRAINCLOUD, 10), 10, " drains for")
			fire_cloud(Ind, GF_OLD_DRAIN, args.dir, 9999, 3, 4 + get_level(Ind, HDRAINCLOUD, 4), 10, " drains for")
                        -- dmgs a Power D for 2050 (307 goes to hp), Balance D for 1286 (192 goes to hp) from full hp
			-- (with, of course, maxed spell power and h_offense schools)
                        -- The amount of what goes to player is 15% of the damage the monster taken. 
                        -- 9999 is a hack handled in spells1.c. Sorry for this. For the moment this
                        -- the only possible way (i think) to do this spell without affecting the
                        -- current ability of wands and artifacts (that can be activated) that uses
                        -- GF_OLD_DRAIN as well. Once I get to know my way around better, I'll fix this.
                        -- By the way, the real value there is 2.
                        -- HUGE note about this spell: it can NOT kill a monster!
                        --                                                      -the_sandman
        end,
        ["info"] =      function()
                        return "dam ".."var".." rad 3 dur "..(8 + get_level(Ind, HDRAINCLOUD, 10))
        end,
        ["desc"] =      {
                        "Curses an area temporarily, sucking life force of those walking it.",
                        "15% of the damage is fed back to the caster's hit points."
        }
}

--[[
HHOLYWORD = add_spell
{
	["name"] = 	"Holy Word",
	["school"] =	{SCHOOL_HOFFENSE, SCHOOL_HCURING},
        ["am"] =	75,
	["level"] =	45,
	["mana"] = 	500,
	["fail"] =	30,
	["stat"] =      A_WIS,
	["spell"] =	function(args)
			hp_player(Ind, 1000)
			set_afraid(Ind, 0)
			set_poisoned(Ind, 0, 0)
			set_stun(Ind, 0, 0)
			set_cut(Ind, 0, 0)
			end,
	["info"] =	function()
			return "Dispels & heals."
			end,
	["desc"] =	{
			"Dispels evil, heals and cures you."
			}
}
]]
