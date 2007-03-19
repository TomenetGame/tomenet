-- C. Blue admin functions. --
------------------------------

-- Grants a player awareness of all items.
function knowall(name)
    local p, i
    p = ind(name)
    for i = 1, MAX_K_IDX do
	players(p).obj_aware[i] = TRUE;
    end
end

-- Cures all maladies of a player.
function cureall(name)
    local p, t
    p = ind(name)
    set_afraid(p, 0)
    set_image(p, 0)
    set_poisoned(p, 0)
    set_cut(p, 0)
    set_stun(p, 0)
    set_blind(p, 0)
    set_confused(p, 0)
    set_paralyzed(p, 0)
    set_slow(p, 0)
    do_res_stat(p, A_STR)
    do_res_stat(p, A_CON)
    do_res_stat(p, A_DEX)
    do_res_stat(p, A_WIS)
    do_res_stat(p, A_INT)
    do_res_stat(p, A_CHR)
    restore_level(p)
    set_food(p, PY_FOOD_FULL)
    players(p).black_breath = FALSE
    t = players(p).msane
    players(p).csane = t
    t = players(p).mhp
    players(p).chp = t
    players(p).chp_frac = 0
end

-- Prepares a character for testing purposes.
function mktest(name)
    local p
    p = ind(name)
    players(p).lev = 50
    players(p).skill_points = 9999
    players(p).score = 1
    setexp(name, 0)
end

-- Recall all players from the dungeons for urgent server restarts.
-- (Players in towers aren't affected.)
function allrec()
    local p = 0
    for p = 1, NumPlayers do
	if players(p).wpos.wz < 0 then
	    recall(p)
	end
    end
end

-- Set a character's experience according to its level.
-- Also allow a modifier to let it end up just before a certain level.
function setexp(name, modif)
    local p
    p = ind(name)
--    if players(p).lev == 1 then
--	players(p).exp = 0
--	players(p).max_exp = 0
--    else
--        players(p).exp = player_exp[players(p).lev - 1] * players(p).expfact / 100 - modif
--	players(p).exp = lua_player_exp(players(p).lev, players(p).expfact)
	players(p).exp = lua_player_exp(players(p).lev, 100)
        players(p).max_exp = players(p).exp
--    end
end

-- Set a character's level, and experience according to it.
function setlev(name, l)
    local p
    p = ind(name)
    players(p).lev = l
    players(p).max_plv = l
    if l == 1 then
	players(p).exp = 0
	players(p).max_exp = 0
    else
--        players(p).exp = player_exp[l - 1] * players(p).expfact / 100
	players(p).exp = lua_player_exp(l, players(p).expfact)
        players(p).max_exp = players(p).exp
    end
end

-- Maximize all skills of a character.
function maxskills(name)
    local p, i
    p = ind(name)
    for i = 0, (MAX_SKILLS - 1) do
	players(p).s_info[i].value = 50000
    end
end

-- Set all uniques/monsters to '1 times killed' for a player.
--function allunis()
function allunis(name)
    local p, i
    p = ind(name)
--    p = Ind
    for i = 1, MAX_R_IDX do
	players(p).r_killed[i] = 1
    end
end

-- Set all uniques/monsters to '0 times killed' for a player.
--function nomons()
function nomons(name)
    local p, i
    p = ind(name)
--    p = Ind
    for i = 1, MAX_R_IDX do
	players(p).r_killed[i] = 0
    end
end

-- Set all uniques/monsters to '666 times killed' for a player.
--function allmons()
function allmons(name)
    local p, i
    p = ind(name)
--    p = Ind
    for i = 1, MAX_R_IDX do
	players(p).r_killed[i] = 666
    end
end

-- Copy unique mask of another player to calling player (idea from evileye).
function unicpy(name)
    local p, i, c
    p = ind(name)
    for i = 1, MAX_R_IDX do
        c = players(p).r_killed[i]
	players(Ind).r_killed[i] = c
    end
end

-- Displays status information about a player, for example to
-- calculate the possible difficulty level of admin quests :).
function status(name)
    local p, r, k
    p = ind(name)
    r = players(p).body_monster
    msg_print(Ind, "\255UStatus for "..name.." (Index "..p..", id "..players(p).id..")")
    msg_print(Ind, "HP :  "..players(p).chp.." / "..players(p).mhp.."    SP :  "..players(p).csp.." / "..players(p).msp.."    San:  "..players(p).csane.." / "..players(p).msane)
    msg_print(Ind, "AC :  "..players(p).ac.."   +AC: "..players(p).to_a.."   Total AC: "..(players(p).ac+players(p).to_a))
    if players(p).r_killed[863] > 0 then
	msg_print(Ind, "Spd:  "..players(p).pspeed.."   MDLev: "..players(p).max_dlv.."   Killed Morgoth")
    elseif players(p).r_killed[861] > 0 then
	msg_print(Ind, "Spd:  "..players(p).pspeed.."   MDLev: "..players(p).max_dlv.."   Killed Sauron")
    else
	msg_print(Ind, "Spd:  "..players(p).pspeed.."   MDLev: "..players(p).max_dlv)
    end
    msg_print(Ind, "ToH:  "..players(p).to_h.."   THM:  "..players(p).to_h_melee.."   THR:  "..players(p).to_h_ranged.."   *TMH*:  "..players(p).dis_to_h+players(p).to_h_melee)
    msg_print(Ind, "ToD:  "..players(p).to_d.."   TDM:  "..players(p).to_d_melee.."   TDR:  "..players(p).to_d_ranged.."   *TMD*:  "..players(p).dis_to_d+players(p).to_d_melee)
    msg_print(Ind, "BpR:  "..players(p).num_blow.."   SpR:  "..players(p).num_fire.."   CpR:  "..players(p).num_spell)
    msg_print(Ind, "Au :  "..players(p).au.."   Bank:  "..players(p).balance)
    if players(p).lev < 100 then
--        msg_print(Ind, "Exp:  "..players(p).exp.."   MEx:  "..players(p).max_exp.."   E2A:  "..(player_exp[players(p).lev] / 100 * players(p).expfact))
        msg_print(Ind, "Exp:  "..players(p).exp.."   MEx:  "..players(p).max_exp.."   E2A:  "..(player_exp[players(p).lev]))
    end
    if r > 0 then
	k = players(p).r_killed[r + 1]
	msg_print(Ind, "Body: "..r.." ("..lua_get_mon_name(r)..", level "..lua_get_mon_lev(r)..", killed: "..k..")")
    else
	msg_print(Ind, "Body: Normal form")
    end
    msg_print(Ind, "Lifes: "..players(p).lives.."  -  Houses: "..players(p).houses_owned)
end

-- Displays resistance/suspectibilities/immunities of a player
function resist(name)
    local p
    local susstr, susint, suswis, susdex, suscon, suschr
    p = ind(name)
    msg_print(Ind, "\255UResistances for "..name.." (Index "..p..")")
    msg_print(Ind, "  \255bElec: "..players(p).resist_elec.."  \255wFrost: "..players(p).resist_cold.."  \255sAcid: "..players(p).resist_acid.."  \255rFire: "..players(p).resist_fire.."  \255gPoison: "..players(p).resist_pois.."  \255BWater: "..players(p).resist_water)
    msg_print(Ind, "  \255bELEC: "..players(p).immune_elec.."  \255wFROST: "..players(p).immune_cold.."  \255sACID: "..players(p).immune_acid.."  \255rFIRE: "..players(p).immune_fire.."  \255gPOISON: "..players(p).immune_poison.."  \255BWATER: "..players(p).immune_water)
--.."\255W-  \255gNETHER: "..players(p).immune_neth)
--    msg_print(Ind, "  \255belec: "..players(p).sensible_elec.."  \255wfrost: "..players(p).sensible_cold.."  \255sacid: "..players(p).sensible_acid.."  \255rfire:  "..players(p).sensible_fire.."  \255gpoison:  "..players(p).sensible_pois)
    msg_print(Ind, "  \255sNeth: "..players(p).resist_neth.."  \255vChaos: "..players(p).resist_chaos.."  \255vNexu: "..players(p).resist_nexus.."  \255oDise: "..players(p).resist_disen.."  \255uShards: "..players(p).resist_shard.."  \255ySound: "..players(p).resist_sound)
    msg_print(Ind, "  \255DDark: "..players(p).resist_dark.."  \255WLight: "..players(p).resist_lite.."  \255vMana: "..players(p).resist_mana.."  \255BTime: "..players(p).resist_time)
--.."  \255RPlasma: "..players(p).resist_plasma)
    msg_print(Ind, "  \255sFear: "..players(p).resist_fear.."  \255BConfusion: "..players(p).resist_conf.."  \255bFeather Falling: "..players(p).feather_fall.."  \255rFA:  "..players(p).free_act.."  \255gBlind: "..players(p).resist_blind)
--    msg_print(Ind, "  \255yReflect: "..players(p).reflect.."  \255uNo-cut: "..players(p).no_cut.."  \255oRes.Tele.: "..players(p).feather_fall.."  \255rFA:  "..players(p).free_act.."  \255gBlind: "..players(p).res_tele)
-- display sustenances:
    susstr = "---"
    susint = "---"
    suswis = "---"
    susdex = "---"
    suscon = "---"
    suschr = "---"
    if players(p).sustain_str == TRUE then susstr = "STR" end
    if players(p).sustain_int == TRUE then susint = "INT" end
    if players(p).sustain_wis == TRUE then suswis = "WIS" end
    if players(p).sustain_dex == TRUE then susdex = "DEX" end
    if players(p).sustain_con == TRUE then suscon = "CON" end
    if players(p).sustain_chr == TRUE then suschr = "CHR" end
    msg_print(Ind, "\255USustenances:\255u  "..susstr.." "..susint.." "..suswis.." "..susdex.." "..suscon.." "..suschr)
end

-- "Detail" - Displays status(name) and resist(name)
function det(name)
    msg_print(Ind, " ".." ")
    status(name)
    resist(name)
    msg_print(Ind, " ".." ")
end

-- Show the internal memory index of a player.
function sind(name)
    local p
    p = ind(name)
    msg_print(Ind, "Ind of "..name.." = "..p)
end

-- Toggle ghost flag of a player quickly.
function tgh()
    if players(Ind).ghost == 1 then
	players(Ind).ghost = 0
	msg_print(Ind, "Player "..Ind.." is no ghost anymore")
    else
	players(Ind).ghost = 1
	msg_print(Ind, "Player "..Ind.." is now a ghost")
    end
end

-- Toggle king flag of a player quickly.
function tki(i)
    if players(i).total_winner == 1 then
	players(i).total_winner = 0
	msg_print(Ind, "Player "..i.." is no king anymore")
    else
	players(i).total_winner = 1
	msg_print(Ind, "Player "..i.." is now a king")
    end
end

-- Display owner mode of an item
function gownmode(name, i)
    local p
    p = ind(name)
    msg_print(Ind, "owner_mode for item on inventory slot "..i.." of player "..name.." is "..players(p).inventory[i].owner_mode)
end

-- Examine an item (*ID*)
function xitem(name, i)
    if (i >= 1) and (i <= 38) then
        lua_examine_item(Ind, name, i - 1)
    end
end

-- Recalculate an item's level requirements
function det_lev_req(name, i)
    local p
    p = ind(name)
    lua_determine_level_req(p, i)
end

-- Establish VNC mind link
function vnc(name)
    local p, id
    p = ind(name)
    id = players(Ind).id
    players(p).esp_link = id
    players(p).esp_link_type = 1
    players(p).esp_link_end = 0
    players(p).esp_link_flags = 1 + 128
    msg_print(Ind, "Mind link established.")
end

-- Break VNC mind link
function vncoff(name)
    local p
    p = ind(name)
    players(p).esp_link = 0
    players(p).esp_link_type = 0
    players(p).esp_link_end = 0
    players(p).esp_link_flags = 0
    msg_print(Ind, "Mind link broken.")
end

-- Show a player's kill count (bodycount) for his current form
function bcnt(name)
    local p, r
    p = ind(name)
    r = players(p).body_monster
    if r > 0 then
	k = players(p).r_killed[r + 1]
	msg_print(Ind, "Currently in form "..r.." ("..lua_get_mon_name(r)..", level: "..lua_get_mon_lev(r)..", killed: "..k..").")
    else
	msg_print(Ind, "Currently in @ form.")
    end
end

--quickly polymorph self into..
function poly(r_idx)
    players(Ind).body_monster = r_idx
end

--quickly do msg_print to oneself
function prn(tolua_S)
    msg_print(Ind, tolua_S.."#")
end

--quickly manipulate items
function inv(tolua_S)
    exec_lua(Ind, "players(Ind).inventory"..tolua_S)
end

--refills own spell points
function fsp()
    players(Ind).csp = players(Ind).msp
end

--display number of kills of a certain monster
function nk(name, r)
    local p
    p = ind(name)
    msg_print(Ind, "Killed "..players(p).r_killed[r + 1].." "..lua_get_mon_name(r)..".")
end

--display all learnt forms for mimics
function mf(name, minlev)
    local i, k, n, p
    p = ind(name)
    msg_print(Ind, "scanning "..MAX_R_IDX.." forms.")
    for i = 1, MAX_R_IDX-1 do
	n = lua_get_mon_lev(i)
	k = players(p).r_killed[i + 1]
        if n >= minlev then
	    if k >= n then
		if n >= 80 then
		    msg_print(Ind, "\255R"..n.."\255W "..lua_get_mon_name(i)..": "..k.."/"..n)
		elseif n >= 65 then
		    msg_print(Ind, "\255r"..n.."\255W "..lua_get_mon_name(i)..": "..k.."/"..n)
		elseif n >= 50 then
		    msg_print(Ind, "\255o"..n.."\255W "..lua_get_mon_name(i)..": "..k.."/"..n)
		elseif n >= 30 then
		    msg_print(Ind, "\255y"..n.."\255W "..lua_get_mon_name(i)..": "..k.."/"..n)
		else
		    msg_print(Ind, "\255s"..n.."\255W "..lua_get_mon_name(i)..": "..k.."/"..n)
		end
	    end
	end
    end
end

--display amount of points spent into a skill
function ssp(name, skill0)
    local skill, p, v, m, s
    skill = skill0 + 1
    p = ind(name)
    v = players(p).s_info[skill].value
    m = players(p).s_info[skill].mod
    if m == 0 then
	s = 0
    else
        s = v / m
    end
    msg_print(Ind, "Skill "..skill0.." of mod "..m.." is at "..v.." and used "..s.." points");
end

--erase a skill and give player the points back
function rsp(name, skill0)
    p = ind(name)
    local skill, p, v, m, s
    skill = skill0 + 1
    p = ind(name)
    v = players(p).s_info[skill].value
    m = players(p).s_info[skill].mod
    if m == 0 then
	s = 0
    else
        s = v / m
    end
    msg_print(Ind, "Skill "..skill0.." of mod "..m.." was at "..v.." and had used "..s.." points");
    players(p).s_info[skill].value = 0
    players(p).skill_points = players(p).skill_points + s
end

--mhh
function lsd(name)
    p = ind(name)
    players(p).image = players(p).image + 20
end
