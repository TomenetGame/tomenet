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
		local dur
                if get_level(Ind, HBLESSING, 50) < 15 then
			if player.blessed_power == 0 then
				player.blessed_power = 10
				dur = 9 + randint(get_level(Ind, HBLESSING, 25))
				set_blessed(Ind, dur)
				if player.spell_project > 0 then
		                        fire_ball(Ind, GF_BLESS_PLAYER, 0, dur, player.spell_project, " recites a blessing.")
			        end
			end
                elseif get_level(Ind, HBLESSING, 50) < 30 then
			if player.blessed_power < 20 then
				player.blessed_power = 20
				dur = 17 + randint(get_level(Ind, HBLESSING, 25))
				set_blessed(Ind, dur)
				if player.spell_project > 0 then
		                        fire_ball(Ind, GF_BLESS_PLAYER, 0, dur, player.spell_project, " chants.")
			        end
			end
		else
			if player.blessed_power < 30 then
				player.blessed_power = 30
				dur = 32 + randint(get_level(Ind, HBLESSING, 25))
				set_blessed(Ind, dur)
				if player.spell_project > 0 then
		                        fire_ball(Ind, GF_BLESS_PLAYER, 0, dur, player.spell_project, " speaks a holy prayer.")
			        end
			end
		end
	end,
	["info"] = 	function()
        	if get_level(Ind, HBLESSING, 50) < 15 then
			return "AC+10  dur 9.."..get_level(Ind, HBLESSING, 25)+9
                elseif get_level(Ind, HBLESSING, 50) < 30 then
			return "AC+20  dur 17.."..get_level(Ind, HBLESSING, 25)+17
		else
			return "AC+30  dur 32.."..get_level(Ind, HBLESSING, 25)+32
                end
	end,
        ["desc"] =	{
        		"Protects you with a shield of righterousness",
        		"At level 15 it turns into a holy chant",
        		"At level 30 becomes a holy prayer",
			"***Affected by the Meta spell: Project Spell***",
        }
}

HRESISTS = add_spell
{
	["name"] = 	"Holy Resistance",
        ["school"] = 	{SCHOOL_HDEFENSE},
        ["level"] = 	11,
        ["mana"] = 	4,
        ["mana_max"] = 	45,
        ["fail"] = 	15,
	["stat"] =      A_WIS,
        ["spell"] = 	function()
		local dur
		dur = randint(10) + 15 + get_level(Ind, HRESISTS, 50)

		set_oppose_fire(Ind, dur)
		if player.spell_project > 0 then
			fire_ball(Ind, GF_RESFIRE_PLAYER, 0, dur, player.spell_project, " calls to the heavens for protection from the elements.")
		end
        	if get_level(Ind, HRESISTS, 50) > 4 then
			set_oppose_cold(Ind, dur)
			if player.spell_project > 0 then
				fire_ball(Ind, GF_RESCOLD_PLAYER, 0, dur, player.spell_project, "")
			end
		end
                if get_level(Ind, HRESISTS, 50) > 9 then
			set_oppose_elec(Ind, dur)
			if player.spell_project > 0 then
				fire_ball(Ind, GF_RESELEC_PLAYER, 0, dur, player.spell_project, "")
			end
		end
                if get_level(Ind, HRESISTS, 50) > 14 then
			set_oppose_acid(Ind, dur)
			if player.spell_project > 0 then
				fire_ball(Ind, GF_RESACID_PLAYER, 0, dur, player.spell_project, "")
			end
		end
	end,
	["info"] = 	function()
        	if get_level(Ind, HRESISTS, 50) < 5 then
			return "Res heat dur "..(get_level(Ind, HRESISTS, 50)+15)..".."..(get_level(Ind, HRESISTS, 50)+25)
                elseif get_level(Ind, HRESISTS, 50) < 10 then
			return "Res heat/cold, dur "..(get_level(Ind, HRESISTS, 50)+15)..".."..(get_level(Ind, HRESISTS, 50)+25)
		elseif get_level(Ind, HRESISTS, 50) < 15 then
			return "Res heat/cold/elec, dur "..(get_level(Ind, HRESISTS, 50)+15)..".."..(get_level(Ind, HRESISTS, 50)+25)
		else
			return "Base resistance, dur "..(get_level(Ind, HRESISTS, 50)+15)..".."..(get_level(Ind, HRESISTS, 50)+25)
                end
	end,
        ["desc"] =	{
        		"Lets you resist heat.",
        		"At level 5 you also resist cold.",
        		"At level 10 you resist lightning too.",
        		"At level 15 it gives acid resistance as well.",
			"***Affected by the Meta spell: Project Spell***",
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
	["level"] =	40,
	["mana"]=	20,
	["mana_max"] =	20,
	["fail"] = 	0,
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

HMARTYR = add_spell
{
	["name"] =	"Eternal Martyr",
	["school"] = 	{SCHOOL_HDEFENSE},
	["level"] =	40,
	["mana"]=	50,
	["mana_max"] =	50,
	["fail"] = 	0,
	["stat"] =      A_WIS,
	["spell"] = 	function()
			if player.martyr_timeout > 0 then
				msg_print("The heavens are not ready yet to accept your martyrium.")
			else
				set_martyr(Ind, 15)
			end
			end,
	["info"] = 	function()
			return "dur 15  timeout 1000"
			end,
	["desc"] = 	{
			"Turns you into an holy martyr, blessed with immortality",
			"to fulfil his work. When the holy fire ceases, you will",
			"be left with 1 HP. It will take a while until the heavens",
			"are ready to accept another martyrium (1000 turns timeout).",
	}
}
