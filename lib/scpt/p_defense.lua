-- handle the holy defense school

HBLESSING = add_spell
{
	["name"] = 	"Blessing",
        ["school"] = 	{SCHOOL_HDEFENSE},
        ["level"] = 	1,
        ["mana"] = 	4,
        ["mana_max"] = 	25,
        ["fail"] = 	10,
	["stat"] =      A_WIS,
        ["spell"] = 	function()
                if get_level(Ind, HBLESSING, 50) < 15 then
			if player.blessed_power == 0 then
				player.blessed_power = 10
				set_blessed(Ind, 6 + randint(get_level(Ind, HBLESSING, 50)))
			end
                elseif get_level(Ind, HBLESSING, 50) < 30 then
			if player.blessed_power < 20 then
				player.blessed_power = 20
				set_blessed(Ind, 12 + randint(get_level(Ind, HBLESSING, 50)))
			end
		else
			if player.blessed_power < 30 then
				player.blessed_power = 30
				set_blessed(Ind, 24 + randint(get_level(Ind, HBLESSING, 50)))
			end
		end
	end,
	["info"] = 	function()
        	if get_level(Ind, HBLESSING, 50) < 15 then
			return "AC+10  dur 7.."..get_level(Ind, HBLESSING, 50)+6
                elseif get_level(Ind, HBLESSING, 50) < 30 then
			return "AC+20  dur 13.."..get_level(Ind, HBLESSING, 50)+12
		else
			return "AC+30  dur 25.."..get_level(Ind, HBLESSING, 50)+25
                end
	end,
        ["desc"] =	{
        		"Protects you with a shield of righterousness",
        		"At level 15 it turns into a holy chant",
        		"At level 30 becomes a holy prayer",
        }
}

HRESISTS = add_spell
{
	["name"] = 	"Holy Resistance",
        ["school"] = 	{SCHOOL_HDEFENSE},
        ["level"] = 	10,
        ["mana"] = 	4,
        ["mana_max"] = 	45,
        ["fail"] = 	15,
	["stat"] =      A_WIS,
        ["spell"] = 	function()
		set_oppose_fire(Ind, randint(10) + 15 + get_level(Ind, HRESISTS, 50))
        	if get_level(Ind, HRESISTS, 50) > 9 then
			set_oppose_cold(Ind, randint(10) + 15 + get_level(Ind, HRESISTS, 50))
		end
                if get_level(Ind, HRESISTS, 50) > 19 then
			set_oppose_elec(Ind, randint(10) + 15 + get_level(Ind, HRESISTS, 50))
		end
	end,
	["info"] = 	function()
        	if get_level(Ind, HRESISTS, 50) < 10 then
			return "Res heat, dur "..(get_level(Ind, HRESISTS, 50)+15)..".."..(get_level(Ind, HRESISTS, 50)+25)
                elseif get_level(Ind, HRESISTS, 50) < 20 then
			return "Res heat & cold, dur "..(get_level(Ind, HRESISTS, 50)+15)..".."..(get_level(Ind, HRESISTS, 50)+25)
		else
			return "Res heat/cold/lightning, dur "..(get_level(Ind, HRESISTS, 50)+15)..".."..(get_level(Ind, HRESISTS, 50)+25)
                end
	end,
        ["desc"] =	{
        		"Lets you resist heat.",
        		"At level 10 you also resist cold.",
        		"At level 20 you resist lightning too.",
        }
}

HPROTEVIL = add_spell
{
	["name"] = 	"Protection From Evil",
        ["school"] = 	{SCHOOL_HDEFENSE},
        ["level"] = 	15,
        ["mana"] = 	20,
        ["mana_max"] = 	60,
        ["fail"] = 	20,
	["stat"] =      A_WIS,
        ["spell"] = 	function()
            		set_protevil(Ind, randint(25) + 3*get_level(Ind, HPROTEVIL, 50))
	end,
	["info"] = 	function()
			return "dur "..1+(get_level(Ind, HPROTEVIL, 50)*3)..".."..25+(3*get_level(Ind, HPROTEVIL, 50))
	end,
        ["desc"] =	{
        		"Repels evil that tries to lay hand at you.",
        }
}

HRUNEPROT = add_spell
{
	["name"] =	"Glyph Of Warding",
	["school"] = 	{SCHOOL_HDEFENSE},
	["level"] =	45,
	["mana"]=	50,
	["mana_max"] =	50,
	["fail"] = 	10,
	["stat"] =      A_WIS,
	["spell"] = 	function()
                        local x, y
                        y = player.py
                        x = player.px
                        cave_set_feat(player.wpos, y, x, 3)
			end,
	["info"] = 	function()
			return ""
			end,
	["desc"] = 	{
			"Creates a rune of protection on the ground.",
	}
}
