-- C. Blue admin functions. --
------------------------------

-- Grants a player awareness of all items.
function knowall(name)
    local p, i
    p = ind(name)
    if (p == -1) then return -1 end
    for i = 1, max_k_idx do
	players(p).obj_aware[i] = TRUE;
    end
end

-- Cures all maladies of a player.
function cure(name)
    local p, t
    p = ind(name)
    if (p == -1) then return -1 end
    set_afraid(p, 0)
    set_image(p, 0)
    set_poisoned(p, 0, 0)
    set_cut(p, 0, 0)
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
    t = players(p).mst
    players(p).cst = t
end

-- Prepares a character for testing purposes.
function mktest(name)
    local p
    p = ind(name)
    if (p == -1) then return -1 end
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
	if players(p).wpos.wz ~= 0 then
	    players(p).recall_pos.wz = 0;
	    recall_player(p, "")
	end
    end
end

-- Set a character's experience according to its level.
-- Also allow a modifier to let it end up just before a certain level.
function setexp(name, modif)
    local p
    p = ind(name)
    if (p == -1) then return -1 end
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
    if (p == -1) then return -1 end
    players(p).lev = l
    players(p).max_lev = l
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
    if (p == -1) then return -1 end
--    for i = 0, (MAX_SKILLS - 1) do
    for i = 1, (MAX_SKILLS - 0) do
	players(p).s_info[i].value = 50000
    end
end

-- Set all uniques/monsters to '1 times killed' for a player.
--function allunis()
function allunis(name)
    local p, i
    p = ind(name)
    if (p == -1) then return -1 end
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
    if (p == -1) then return -1 end
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
    if (p == -1) then return -1 end
--    p = Ind
    for i = 1, MAX_R_IDX do
	players(p).r_killed[i] = 666
    end
end

-- Copy unique mask of another player to calling player (idea from evileye).
function unicpy(name)
    local p, i, c
    p = ind(name)
    if (p == -1) then return -1 end
    for i = 1, MAX_R_IDX do
        c = players(p).r_killed[i]
	player.r_killed[i] = c
    end
end

-- Displays status information about a player, for example to
-- calculate the possible difficulty level of admin quests :).
function status(name)
    local p, r, k, bspeed, rs, rstim, ks
    p = ind_loose(name)
    if (p == -1) then return -1 end
    r = players(p).body_monster
    local line1, linetab

    if r > 0 then
	k = players(p).r_killed[r + 1]
	rstim = ""
	if players(p).tim_mimic_what == r then
	    rstim = " (\255y"..players(p).tim_mimic.."\255w)"
	end
	rs = r.." - "..lua_get_mon_name(r)..rstim..", lev "..lua_get_mon_lev(r)..", killed: "..k
    else
	rs = "Normal form"
    end
    if players(p).r_killed[863] > 0 then
	ks = "   \255vK.Morgoth"
    elseif players(p).r_killed[861] > 0 then
	ks = "   \255BK.Sauron"
    else
	ks = ""
    end
    
    if players(p).fast > 0 then
	bspeed = players(p).fast_mod
    else
	bspeed = 0
    end
    bspeed = players(p).pspeed - bspeed

    if players(p).tim_blacklist > 0 then
	blklst = "\255oBlacklist: "..players(p).tim_blacklist
    else
	blklst = ""
    end
    if players(p).tim_watchlist > 0 then
	wchlst = "\255yWatchlist: "..players(p).tim_watchlist
    else
	wchlst = ""
    end

    if players(p).max_lev < players(p).max_plv then
	mlvc = "\255g"
    else
	mlvc = "\255G"
    end
    if players(p).lev < players(p).max_lev then
	clvc = "\255y"
    else
	clvc = mlvc
    end

    if band(players(p).mode, 16) ~= 0 then
	cmode = "ypvp"
    elseif band(players(p).mode, 8) ~= 0 then
	cmode = "Beverlasting"
    elseif band(players(p).mode, 2) ~= 0 then
	if band(players(p).mode, 4) ~= 0 then
	    cmode = "rhellish"
	else
	    cmode = "Whard"
	end
    else
	if players(p).mode == 0 then
	    cmode = "wnormal"
	else
	    cmode = "Dnoghost"
	end
    end

    msg_print(Ind, "\255UStatus for "..players(p).name.." (Ind "..p..", id "..players(p).id..", party "..players(p).party..", \255"..cmode.."\255U)")
    msg_print(Ind, "Race: "..race_info[players(p).prace + 1].title.." ("..players(p).prace..")  Class: "..class_info[players(p).pclass + 1].title.." ("..players(p).pclass..")  Trait: "..trait_info[players(p).ptrait + 1].title.." ("..players(p).ptrait..")  Go: "..players(p).go_level)
    msg_print(Ind, "HP: "..players(p).chp.."/"..players(p).mhp.."  MP: "..players(p).csp.."/"..players(p).msp.."  SN: "..players(p).csane.."/"..players(p).msane.."  St: "..players(p).cst.."/"..players(p).mst.."  Crt: "..players(p).xtra_crit.."  Lu: "..players(p).luck..ks)

    line1 = "Base Spd: "..bspeed.."   Spd: "..players(p).pspeed.."  MDLev: "..players(p).max_dlv
    linetab = ""
    for i = 1, 47 - strlen(line1) do
	linetab = linetab.." "
    end
    msg_print(Ind, line1..linetab..blklst)

    line1 = "Lev: "..clvc..players(p).lev.."\255w   Max Lev: "..mlvc..players(p).max_lev.."\255w   Top Lev: \255G"..players(p).max_plv
    linetab = ""
    for i = 1, 57 - strlen(line1) do
	linetab = linetab.." "
    end
    msg_print(Ind, line1..linetab..wchlst)

    msg_print(Ind, "Body: "..rs)
    line1 = "AC : "..players(p).ac.."   +AC: "..players(p).to_a.."   Total AC: "..(players(p).ac+players(p).to_a)
    linetab = ""
    for i = 1, 45 - strlen(line1) do
	linetab = linetab.." "
    end
    msg_print(Ind, line1..linetab.."  \255sOnline: "..players(p).turns_online)

    line1 = "ToH: "..players(p).to_h.."   THM: "..players(p).to_h_melee.."   THR: "..players(p).to_h_ranged.."   *TMH*: "..players(p).dis_to_h+players(p).to_h_melee
    linetab = ""
    for i = 1, 45 - strlen(line1) do
	linetab = linetab.." "
    end
    msg_print(Ind, line1..linetab.."  \255sAFK:    "..players(p).turns_afk)

    line1 = "ToD: "..players(p).to_d.."   TDM: "..players(p).to_d_melee.."   TDR: "..players(p).to_d_ranged.."   *TMD*: "..players(p).dis_to_d+players(p).to_d_melee
    linetab = ""
    for i = 1, 45 - strlen(line1) do
	linetab = linetab.." "
    end
    msg_print(Ind, line1..linetab.."  \255sIdle:   "..players(p).turns_idle)

    msg_print(Ind, "BpR: "..players(p).num_blow.."   SpR: "..players(p).num_fire.."   CpR: "..players(p).num_spell.."   Au : "..players(p).au.."   Bank:  "..players(p).balance)

    if players(p).lev < 100 then
	if players(p).exp == players(p).max_exp then
		estr = "\255w"
	else
		estr = "\255y"
	end
--        msg_print(Ind, "Exp: "..players(p).exp.."   MEx: "..players(p).max_exp.."   E2A: "..(player_exp[players(p).lev] / 100 * players(p).expfact))
	if players(p).kills == 0 then
	    kills = "Kills: 0"
	else
	    if players(p).mode ~= 16 then
		kills = "Kills: \255r"..players(p).kills
	    else
		kills = "Kills: \255y"..players(p).kills
	    end
	end
        msg_print(Ind, "Deaths: \255D"..players(p).deaths.."\255g("..players(p).soft_deaths..")  "..estr.."Exp:\255w "..players(p).exp.."  MEx: "..players(p).max_exp.."  E2A: "..(player_exp[players(p).lev]).."  "..kills)
    end
--    msg_print(Ind, "Lifes: "..players(p).lives.."  -  Houses: "..players(p).houses_owned.."  -  Combat Stance: "..players(p).combat_stance.."  - Dodge level: "..players(p).dodge_level.."/"..apply_dodge_chance(p, players(p).lev))
--    msg_print(Ind, "Lifes: "..players(p).lives.."  Houses: "..players(p).houses_owned.."  Stance: "..players(p).combat_stance.."  Dual: "..players(p).dual_mode.."  Dodge: "..players(p).dodge_level.."/"..apply_dodge_chance(p, players(p).lev).."  Parry/Block: "..players(p).weapon_parry.."/"..players(p).shield_deflect)
    msg_print(Ind, "Liv/Hous: "..players(p).lives.."/"..players(p).houses_owned.."  Stance/DW: "..players(p).combat_stance.."/"..players(p).dual_mode.."  Dodge: "..players(p).dodge_level.."/"..apply_dodge_chance(p, players(p).lev).."  Parry/Blk: "..players(p).weapon_parry.."/"..players(p).shield_deflect)
end

-- Displays resistance/suspectibilities/immunities of a player
function resist(name)
    local p
    local resf, resc, resa, rese, resp, resw, resl
    local susstr, susint, suswis, susdex, suscon, suschr
    p = ind_loose(name)
    if (p == -1) then return -1 end
--    msg_print(Ind, "\255UResistances for "..players(p).name.." (Index "..p..")")
--    msg_print(Ind, "  \255bElec: "..players(p).resist_elec.."  \255wFrost: "..players(p).resist_cold.."  \255sAcid: "..players(p).resist_acid.."  \255rFire: "..players(p).resist_fire.."  \255gPoison: "..players(p).resist_pois.."  \255BWater: "..players(p).resist_water)
--    msg_print(Ind, "  \255bELEC: "..players(p).immune_elec.."  \255wFROST: "..players(p).immune_cold.."  \255sACID: "..players(p).immune_acid.."  \255rFIRE: "..players(p).immune_fire.."  \255gPOISON: "..players(p).immune_poison.."  \255BWATER: "..players(p).immune_water)
    rese = players(p).resist_elec
    if players(p).suscep_elec == 1 then rese = "-" end
    if players(p).immune_elec == 1 then rese = "*" end
    resc = players(p).resist_cold
    if players(p).suscep_cold == 1 then resc = "-" end
    if players(p).immune_cold == 1 then resc = "*" end
    resa = players(p).resist_acid
    if players(p).suscep_acid == 1 then resa = "-" end
    if players(p).immune_acid == 1 then resa = "*" end
    resf = players(p).resist_fire
    if players(p).suscep_fire == 1 then resf = "-" end
    if players(p).immune_fire == 1 then resf = "*" end
    resp = players(p).resist_pois
    if players(p).suscep_pois == 1 then resp = "-" end
    if players(p).immune_poison == 1 then resp = "*" end
    resw = players(p).resist_water
    if players(p).immune_water == 1 then resw = "*" end
--    msg_print(Ind, "  \255bELEC: "..rese.."  \255wFROST: "..resc.."  \255sACID: "..resa.."  \255rFIRE: "..resf.."  \255gPOISON: "..resp.."  \255BWATER: "..players(p).resune_water)
    msg_print(Ind, "  \255rFire: "..resf.."  \255wFrost: "..resc.."  \255sAcid: "..resa.."  \255bElec: "..rese.."  \255gPoison: "..resp.."  \255BWater: "..resw)
--.."\255W-  \255gNETHER: "..players(p).immune_neth)
--    msg_print(Ind, "  \255belec: "..players(p).sensible_elec.."  \255wfrost: "..players(p).sensible_cold.."  \255sacid: "..players(p).sensible_acid.."  \255rfire:  "..players(p).sensible_fire.."  \255gpoison:  "..players(p).sensible_pois)
    msg_print(Ind, "  \255DNeth: "..players(p).resist_neth.."  \255vChaos: "..players(p).resist_chaos.."  \255oDise: "..players(p).resist_disen.."  \255vNexu: "..players(p).resist_nexus.."  \255uShards: "..players(p).resist_shard.."  \255ySound: "..players(p).resist_sound)
--    msg_print(Ind, "  \255DDark: "..players(p).resist_dark.."  \255WLight: "..players(p).resist_lite.."  \255vMana: "..players(p).resist_mana.."  \255BTime: "..players(p).resist_time.."  \255rRH \255vRM \255rDH \255vDM \255oDX \255w: \255r"..players(p).regenerate.." \255v"..players(p).regen_mana.." \255r"..players(p).drain_life.." \255v"..players(p).drain_mana.." \255o"..players(p).drain_exp)
    resl = players(p).resist_lite
    if players(p).suscep_lite == 1 then resl = "-" end
    msg_print(Ind, "  \255DDark: "..players(p).resist_dark.."  \255WLight: "..resl.."  \255vMana: "..players(p).resist_mana.."  \255BTime: "..players(p).resist_time.."  \255rRH\255D-\255rDH \255vRM\255D-\255vDM \255oDX \255w: \255r"..players(p).regenerate.."\255D-\255r"..players(p).drain_life.." \255v"..players(p).regen_mana.."\255D-\255v"..players(p).drain_mana.." \255o"..players(p).drain_exp)
--.."  \255RPlasma: "..players(p).resist_plasma)
    if players(p).aggravate == 1 then
	aggr = "  \255D(AGGR)"
    else
	aggr = ""
    end
    msg_print(Ind, "  \255DHL: "..players(p).hold_life.."  \255GFeather: "..players(p).feather_fall.."  \255yFear: "..players(p).resist_fear.."  \255oConf: "..players(p).resist_conf.."  \255rBlind: "..players(p).resist_blind.."  \255RFA: "..players(p).free_act.."  \255r+H\255s/\255v+M\255s: \255r"..(players(p).to_l * 10).."\255s/\255v"..players(p).to_m..aggr)
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
--    msg_print(Ind, "\255USustenances:\255u  "..susstr.." "..susint.." "..suswis.." "..susdex.." "..suscon.." "..suschr)
--    msg_print(Ind, "\255USustenances:\255u  "..susstr.." "..susint.." "..suswis.." "..susdex.." "..suscon.." "..suschr.."  "..encum(name))
    msg_print(Ind, "\255USust:\255u  "..susstr.." "..susint.." "..suswis.." "..susdex.." "..suscon.." "..suschr.."  "..encum(name).." "..esplist(name))
end

-- Displays attribute values of a player
function attr(name)
    local p, astr, aint, awis, adex, acon, achr
    p = ind_loose(name)
    if (p == -1) then return -1 end
    if players(p).stat_use[1] == players(p).stat_top[1] then
	if players(p).stat_cur[1] == 18 + 100 then
	    astr = "\255U"
	else
	    astr = "\255G"
	end
    else
	astr = "\255y"
    end
    if players(p).stat_use[2] == players(p).stat_top[2] then
	if players(p).stat_cur[2] == 18 + 100 then
	    aint = "\255U"
	else
	    aint = "\255G"
	end
    else
	aint = "\255y"
    end
    if players(p).stat_use[3] == players(p).stat_top[3] then
	if players(p).stat_cur[3] == 18 + 100 then
	    awis = "\255U"
	else
	    awis = "\255G"
	end
    else
	awis = "\255y"
    end
    if players(p).stat_use[4] == players(p).stat_top[4] then
	if players(p).stat_cur[4] == 18 + 100 then
	    adex = "\255U"
	else
	    adex = "\255G"
	end
    else
	adex = "\255y"
    end
    if players(p).stat_use[5] == players(p).stat_top[5] then
	if players(p).stat_cur[5] == 18 + 100 then
	    acon = "\255U"
	else
	    acon = "\255G"
	end
    else
	acon = "\255y"
    end
    if players(p).stat_use[6] == players(p).stat_top[6] then
	if players(p).stat_cur[6] == 18 + 100 then
	    achr = "\255U"
	else
	    achr = "\255G"
	end
    else
	achr = "\255y"
    end
    astr = astr.."S:\255s"..players(p).stat_use[1].."/"..players(p).stat_top[1]
    aint = aint.." I:\255s"..players(p).stat_use[2].."/"..players(p).stat_top[2]
    awis = awis.." W:\255s"..players(p).stat_use[3].."/"..players(p).stat_top[3]
    adex = adex.." D:\255s"..players(p).stat_use[4].."/"..players(p).stat_top[4]
    acon = acon.." C:\255s"..players(p).stat_use[5].."/"..players(p).stat_top[5]
    achr = achr.." H:\255s"..players(p).stat_use[6].."/"..players(p).stat_top[6]
    msg_print(Ind, astr..aint..awis..adex..acon..achr)
end

-- "Encumberments" displays encumberment details of a player
function encum(name)
    local p = ind_loose(name)
    if (p == -1) then return -1 end
    combo = " "
    combo2 = " "

    if players(p).cumber_armor == TRUE then
	ca = "\255u("
    else
	ca = " "
    end
    if players(p).heavy_wield == TRUE then
	hw = "\255r/"
    else
	hw = " "
    end
    if players(p).icky_wield == TRUE then
	iw = "\255o\\"
    else
	iw = " "
    end
    if players(p).awkward_wield == TRUE then
	aw = "\255y/"
	combo2 = "\255y/"
    else
	aw = " "
    end
    if players(p).easy_wield == TRUE then
	ew = "\255g|"
	combo2 = "\255g|"
    else
	ew = " "
    end
    if players(p).heavy_shield == TRUE then
	hs = "\255r["
    else
	hs = " "
    end
    if players(p).heavy_shoot == TRUE then
	hst = "\255r}"
    else
	hst = " "
    end
    if players(p).awkward_shoot == TRUE then
	as = "\255y}"
    else
	as = " "
    end
    if players(p).cumber_weight == TRUE then
	cw = "\255RF"
    else
	cw = " "
    end
    if players(p).monk_heavyarmor == TRUE then
	mh = "\255y("
	combo = "\255y("
    else
	mh = " "
    end
    if players(p).rogue_heavyarmor == TRUE then
	rh = "\255b("
	combo = "\255b("
    else
	rh = " "
    end
    if players(p).awkward_armor == TRUE then
	aa = "\255v("
    else
	aa = " "
    end
    if players(p).cumber_glove == TRUE then
	cg = "\255v]"
    else
	cg = " "
    end

--[[
--also add fly,climb,swim,invis?
--no need i think (also not implemented in player.pkg)
    if players(p).levitate == TRUE then
	fly = "LEV "
    else
	fly = "    "
    end
    if players(p).climb == TRUE then
	fly = "CLM "
    else
	fly = "    "
    end
    if players(p).swim == TRUE then
	fly = "SWM "
    else
	fly = "    "
    end
    if players(p).invis == TRUE then
	fly = "INV "
    else
	fly = "    "
    end

--    msg_print(Ind, "\255UEncumberments: "..ca..hw..iw..aw..ew..hs..hst..as..cw..combo..aa..cg.."  \255UExtra: "..fly..climb..swim..inv)
    msg_print(Ind, "\255UEncumberments: "..ca..hw..iw..combo2..hs..hst..as..cw..mh..rh..aa..cg.."  \255UExtra: "..fly..climb..swim..inv)
]]

    if players(p).black_breath == TRUE then
--	bb = " \255DBlack Breath"
	bb = "  \255DBB"
    else
--	bb = "             "
	bb = ""
    end

--    msg_print(Ind, "\255UEncumberments: "..ca..hw..iw..aw..ew..hs..hst..as..cw..combo..aa..cg..bb)
--    msg_print(Ind, "\255UEncumberments: "..ca..hw..iw..combo2..hs..hst..as..cw..mh..rh..aa..cg..bb)
    return ("\255UEncumb: "..ca..hw..iw..combo2..hs..hst..as..cw..mh..rh..aa..cg..bb)
end

-- ESPs
function esplist(name)
    local p = ind_loose(name)
    espall = " "
    espuni = " "
    espevil = " "
    espgood = " "
    espZ = " "
    espN = " "
    espD = " "
    espDR = " "
    espU = " "
    espG = " "
    espP = " "
    espO = " "
    espT = " "
    espS = " "

    if (p == -1) then return -1 end

    if band(players(p).telepathy, 1) ~= 0 then
	espO = "O"
    end
    if band(players(p).telepathy, 2) ~= 0 then
	espT = "T"
    end
    if band(players(p).telepathy, 4) ~= 0 then
	espD = "D"
    end
    if band(players(p).telepathy, 8) ~= 0 then
	espP = "P"
    end
    if band(players(p).telepathy, 16) ~= 0 then
	espU = "U"
    end
    if band(players(p).telepathy, 32) ~= 0 then
	espG = "G"
    end
    if band(players(p).telepathy, 64) ~= 0 then
	espevil = "e"
    end
    if band(players(p).telepathy, 128) ~= 0 then
	espZ = "a"
    end
    if band(players(p).telepathy, 256) ~= 0 then
	espDR = "d"
    end
    if band(players(p).telepathy, 512) ~= 0 then
	espgood = "g"
    end
    if band(players(p).telepathy, 1024) ~= 0 then
	espN = "n"
    end
    if band(players(p).telepathy, 2048) ~= 0 then
	espuni = "u"
    end
    if band(players(p).telepathy, 4096) ~= 0 then
	espS = "S"
    end
    if band(players(p).telepathy, -2147483648) ~= 0 then
	espall = "*"
    end

    return ("\255UESP: "..espS..espO..espT..espP..espN..espG..espDR..espD..espU..espZ..espuni..espgood..espevil..espall)
end

-- Helper function for miscellaneous stats, taken from c-xtra1.c
function likert(x, y, max)
    local n
    if y < 1 then y = 1 end
    if x < 0 then return "r" end
    if x >= max and max > 0 then return "U" end
    n = (x * 10) / y
    if n == 0 or n == 1 or n == 2 then return "r" end
    if n == 3 or n == 4 or n == 5 or n == 6 then return "y" end
    if n >= 7 and n <= 17 then return "G" end
    if max > 0 then return "g" else return "B" end
end

-- Show miscellaneous character abilities of a player
function miscab(name)
    local p = ind_loose(name)
    if (p == -1) then return -1 end
    bth_plus_adj = 3

--    n = players(p).skill_thn + ((players(p).dis_to_h + players(p).to_h_melee) * bth_plus_adj)
    n = players(p).skill_thn
    cfig = likert(n, 120, 0)
--    n = players(p).skill_thb + ((players(p).to_h + players(p).to_h_ranged) * bth_plus_adj)
    n = players(p).skill_thb
    cthb = likert(n, 120, 0)
    csav = likert(players(p).skill_sav, 52, 95)
    csrh = likert(players(p).skill_srh, 60, 100)
    cfos = likert(players(p).skill_fos, 40, 75)
    cstl = likert(players(p).skill_stl, 10, 30)
    cdis = likert(players(p).skill_dis, 80, 100)
    cdev = likert(players(p).skill_dev, 60, 0)

--    msg_print(Ind, "\255wFig \255B"..players(p).skill_thn.."  \255wB/T \255B"..players(p).skill_thb.."  \255wPer \255B"..players(p).skill_fos.."  \255wSrc \255B"..players(p).skill_srh.."  \255wSav \255B"..players(p).skill_sav.."  \255wStl \255B"..players(p).skill_stl.."  \255wDis \255B"..players(p).skill_dis.."  \255wMDv \255B"..players(p).skill_dev)
    msg_print(Ind, "\255wFig \255"..cfig..players(p).skill_thn.."  \255wB/T \255"..cthb..players(p).skill_thb.."  \255wSav \255"..csav..players(p).skill_sav.."  \255wStl \255"..cstl..players(p).skill_stl.."  \255wPer \255"..cfos..players(p).skill_fos.."  \255wSrc \255"..csrh..players(p).skill_srh.."  \255wDis \255"..cdis..players(p).skill_dis.."  \255wMDv \255"..cdev..players(p).skill_dev)
end

-- "Detail" - Displays status(name) and resist(name)
function det(name)
    if ind_loose(name) == -1 then return end
    msg_print(Ind, " ".." ")
    status(name)
    resist(name)
    attr(name)
--    encum(name)
    miscab(name)
    msg_print(Ind, " ".." ")
end

-- Show the internal memory index of a player.
function sind(name)
    local p
    p = ind_loose(name)
    if (p == -1) then return -1 end
    msg_print(Ind, "Ind of "..players(p).name.." = "..p)
end

-- Toggle ghost flag of a player quickly.
function tg()
    if player.ghost == 1 then
	player.ghost = 0
	msg_print(Ind, "Player "..Ind.." is no ghost anymore")
    else
	player.ghost = 1
	msg_print(Ind, "Player "..Ind.." is now a ghost")
    end
end

-- Toggle king flag of a player quickly.
function tk(i)
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
    p = ind_loose(name)
    if (p == -1) then return -1 end
    msg_print(Ind, "owner_mode for item on inventory slot "..i.." of player "..players(p).name.." is "..players(p).inventory[i].owner_mode)
end

-- Examine an item (*ID*)
function xitem(name, i)
    local it
    it = ind_loose(name)
    if (it == -1) then return -1 end
    if (i >= 1) and (i <= 38) then
        lua_examine_item(Ind, it, i - 1)
    end
end
function xnitem(name, iname)
    local it, iti
    it = ind_loose(name)
    if (it == -1) then return -1 end
    if (iname == nil) then return -1 end
    iti = slot_loose(it, iname)
    if (iti == -1) then return -1 end
    lua_examine_item(Ind, it, iti)
end

-- Recalculate an item's level requirements
function det_lev_req(name, i)
    local p
    p = ind(name)
    if (p == -1) then return -1 end
    lua_determine_level_req(p, i)
end

-- Establish VNC mind link
function vnc(name)
    local p, id, i
    p = ind_loose(name)
    if (p == -1) then return -1 end
    id = player.id

    --test target for already-mind-linked. yes -> exit.
    if players(p).esp_link ~= 0 then
	msg_print(Ind, "Target is already mind-linked.")
	return
    end

    --test self for already-mind-linked. yes -> break it.
    if bor(player.esp_link_flags, 256) ~= 0 then
	--break old link cleanly
	for i = 1, NumPlayers do
	    if players(i).esp_link == id then
		vncoff(players(i).name)
	    end
	end
    end

    --clear own (client-side) weather
    Send_weather(Ind, -1, 0, 0, 0, 0, FALSE, FALSE)

    players(p).esp_link = id
    players(p).esp_link_type = 1
    players(p).esp_link_end = 0
    players(p).esp_link_flags = bor(players(p).esp_link_flags, 1 + 16 + 128)
    player.esp_link_flags = bor(player.esp_link_flags, 256)
    players(p).redraw2 = bor(players(p).redraw2, 1)

    --redraw target's (client-side) weather to refresh own weather too
    players(p).panel_changed = 1
    msg_print(Ind, "Mind link established with '"..players(p).name.."'.")
end

-- Break VNC mind link
function vncoff(name)
    local p
    p = ind_loose(name)
    if (p == -1) then return -1 end

    players(p).esp_link = 0
    players(p).esp_link_type = 0
    players(p).esp_link_end = 0
    players(p).esp_link_flags = band(players(p).esp_link_flags, bnot(1 + 16 + 128 + 512))

    player.esp_link_flags = band(player.esp_link_flags, bnot(256))
    player.redraw = bor(player.redraw, 67108864) -- PR_MAP
    msg_print(Ind, "Mind link broken with '"..players(p).name.."'.")
end

-- Reset own mind link state (use for clean up if someone steals it from you)
function vncrs()
    local i, id

    --clean up own mind-link state
    player.esp_link_flags = band(player.esp_link_flags, bnot(256))
    player.redraw = bor(player.redraw, 67108864) -- PR_MAP

    --clean up any linked player's mind-link too
    id = player.id
    for i = 1, NumPlayers do
	if players(i).esp_link == id then
	    vncoff(players(i).name)
	end
    end
    msg_print(Ind, "Mind link reset.")
end

-- Show a player's kill count (bodycount) for his current form
function bcnt(name)
    local p, r
    p = ind_loose(name)
    if (p == -1) then return -1 end
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
    player.body_monster = r_idx
end

--quickly do msg_print to oneself
function prn(tolua_S)
    msg_print(Ind, tolua_S.."#")
end

--quickly get own item
function sginv(i, tolua_S)
    exec_lua(Ind, "msg_print(Ind, player.inventory["..i.."]."..tolua_S.."..\"#\")")
end
--quickly manipulate own item
function sinv(i, tolua_S)
    exec_lua(Ind, "player.inventory["..i.."]."..tolua_S)
end
--quickly get item
function ginv(name, i, tolua_S)
    p = ind_loose(name)
    if (p == -1) then return -1 end
    exec_lua(Ind, "msg_print(Ind, players("..p..").inventory["..i.."]."..tolua_S.."..\"#\")")
end
--quickly manipulate item
function inv(name, i, tolua_S)
    p = ind(name)
    if (p == -1) then return -1 end
    exec_lua(Ind, "players("..p..").inventory["..i.."]."..tolua_S)
end
--quickly display own property
function sgprop(tolua_S)
    exec_lua(Ind, "msg_print(Ind, player."..tolua_S.."..\"#\")")
end
--quickly display property
function gprop(name, tolua_S)
    p = ind_loose(name)
    if (p == -1) then return -1 end
    exec_lua(Ind, "msg_print(Ind, players("..p..")."..tolua_S.."..\"#\")")
end

--refills own spell points
function fsp()
    player.csp = player.msp
end

--display number of kills of a certain monster
function nk(name, r)
    local p
    p = ind_loose(name)
    if (p == -1) then return -1 end
    msg_print(Ind, "Killed "..players(p).r_killed[r + 1].." "..lua_get_mon_name(r)..".")
end

--display all learnt forms for mimics
function mf(name, minlev, maxlev)
    local i, k, n, p
    if maxlev == nil then maxlev = 127 end
    p = ind_loose(name)
    if (p == -1) then return -1 end
    msg_print(Ind, "scanning "..MAX_R_IDX.." forms.")
    for i = 1, MAX_R_IDX - 1 do
	n = lua_get_mon_lev(i)
	k = players(p).r_killed[i + 1]
        if n >= minlev and n <= maxlev and lua_is_unique(i) == 0 then
	    if k >= n and k >= 1 then
		if n >= 80 then
		    msg_print(Ind, "\255v"..n.."\255W ("..i..") "..lua_get_mon_name(i)..": "..k.."/"..n)
		elseif n >= 70 then
		    msg_print(Ind, "\255R"..n.."\255W ("..i..") "..lua_get_mon_name(i)..": "..k.."/"..n)
		elseif n >= 60 then
		    msg_print(Ind, "\255r"..n.."\255W ("..i..") "..lua_get_mon_name(i)..": "..k.."/"..n)
		elseif n >= 50 then
		    msg_print(Ind, "\255o"..n.."\255W ("..i..") "..lua_get_mon_name(i)..": "..k.."/"..n)
		elseif n >= 40 then
		    msg_print(Ind, "\255y"..n.."\255W ("..i..") "..lua_get_mon_name(i)..": "..k.."/"..n)
		elseif n >= 30 then
		    msg_print(Ind, "\255w"..n.."\255W ("..i..") "..lua_get_mon_name(i)..": "..k.."/"..n)
		elseif n > 0 then
		    msg_print(Ind, "\255s"..n.."\255W ("..i..") "..lua_get_mon_name(i)..": "..k.."/"..n)
		else
		    msg_print(Ind, "\255D"..n.."\255W ("..i..") "..lua_get_mon_name(i)..": "..k.."/"..n)
		end
	    end
	end
    end
end
--displays forms affected by specified form-learning progress */
function mfp(name, minlev, progress)
    local i, k, n, p, str
--    if maxlev == nil then maxlev = 127 end
    p = ind_loose(name)
    if (p == -1) then return -1 end
    msg_print(Ind, "scanning "..MAX_R_IDX.." forms.")
    for i = 1, MAX_R_IDX - 1 do
	if lua_mimic_egligible(p, i) == TRUE then
	    n = lua_get_mon_lev(i)
	    k = players(p).r_killed[i + 1]
--	if n >= minlev and n <= maxlev and lua_is_unique(i) == 0 then
--	if k >= progress and k < n and lua_is_unique(i) == 0 then
--	if n >= minlev and k >= progress and k < n and lua_is_unique(i) == 0 then
	    if n >= minlev and k >= progress and lua_is_unique(i) == 0 then
		if n >= 80 then
		    str = "\255v"
		elseif n >= 70 then
		    str = "\255R"
		elseif n >= 60 then
		    str = "\255r"
		elseif n >= 50 then
		    str = "\255o"
		elseif n >= 40 then
		    str = "\255y"
		elseif n >= 30 then
		    str = "\255w"
		elseif n > 0 then
		    str = "\255s"
		else
		    str = "\255D"
		end
		str = str..n
		if (k < n) then
		    str = str.."\255W"
		else
		    str = str.."\255D"
		end
		msg_print(Ind, str.." ("..i..") "..lua_get_mon_name(i)..": "..k.."/"..n)
	    end
	end
    end
end

--set skill points to 9999
function s9()
    player.skill_points = 9999
end

--[[
--display amount of points spent into a skill
--careful, doesn't take into account synergies from sub-skills!!!
function ssp(name, skill0)
    local skill, p, v, m, s
    skill = skill0 + 1
    p = ind_loose(name)
    if (p == -1) then return -1 end
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
--careful, doesn't take into account synergies from sub-skills!!!
function rsp(name, skill0)
    p = ind(name)
    if (p == -1) then return -1 end
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

--erase ALL skills - for admins
function admin_es(name)
    local skill, p, s
    p = ind(name)
    if (p == -1) then return -1 end
    for s = 1, MAX_SKILLS do
        players(p).s_info[s].value = 0
    end
end
]]

--return number of invested skill points for a skill
function gsk(name, skill)
    local skill0, p, v, m, s
    skill0 = skill + 1
    p = ind(name)
    if (p == -1) then return -1 end
    v = players(p).s_info[skill0].value
    m = players(p).s_info[skill0].mod
    s = invested_skill_points(p, skill)
    msg_print(Ind, "Skill "..skill.." of mod "..m.." is at "..v.." and used "..s.." points");
end

--reset one skill, making the invested points available again
function rsk(name, skill, poly)
    local p
    p = ind(name)
    if (p == -1) then return -1 end
    respec_skill(p, skill, 0, poly)
end

--reset one skill, making the invested points available again, and update it to latest version
function fsk(name, skill, poly)
    local p
    p = ind(name)
    if (p == -1) then return -1 end
    respec_skill(p, skill, 1, poly)
end

--reset whole skill chart, making all invested skill points available again
function rskc(name)
    local p
    p = ind(name)
    if (p == -1) then return -1 end
    respec_skills(p, 0)
end

--reset whole skill chart, making all invested skill points available again, and update it to latest version
function fskc(name)
    local p
    p = ind(name)
    if (p == -1) then return -1 end
    respec_skills(p, 1)
end

--mhh
function lsd(name)
    p = ind_loose(name)
    if (p == -1) then return -1 end
    players(p).image = players(p).image + 20
end

--show a skill
function shows(name, skill)
    p = ind_loose(name)
    if (p == -1) then return -1 end
    msg_print(Ind, "val "..players(p).s_info[skill + 1].value.." mod "..players(p).s_info[skill + 1].mod)
end
--fix old chars for DUAL_WIELD / SKILL_DUAL
function fixs(name)
    p = ind(name)
    if (p == -1) then return -1 end
    players(p).s_info[78 + 1].value = 1000
end
--automatically fix old chars for DUAL_WIELD / SKILL_DUAL
function fix_dual_wield(p)
    if players(p).pclass == CLASS_MAGE then
        players(p).s_info[78 + 1].value = 0
    end
    if players(p).pclass == CLASS_MIMIC then
        players(p).s_info[78 + 1].value = 0
    end
    if players(p).pclass == CLASS_ADVENTURER then
        players(p).s_info[78 + 1].value = 0
    end
    if players(p).pclass == CLASS_WARRIOR then
        players(p).s_info[78 + 1].value = 1000
    end
    if players(p).pclass == CLASS_ROGUE then
        players(p).s_info[78 + 1].value = 1000
    end
    if players(p).pclass == CLASS_RANGER then
        players(p).s_info[78 + 1].value = 1000
    end
end
--automatically fix old chars for ENABLE_STANCES / SKILL_STANCE
function fix_stance(p)
    local l
    l = players(p).max_lev
    l = l * 1000
    if l > 50000 then
	l = 50000
    end
    if players(p).pclass == CLASS_MAGE then
	players(p).s_info[79 + 1].value = 0
    end
    if players(p).pclass == CLASS_MIMIC then
	players(p).s_info[79 + 1].value = 0
    end
    if players(p).pclass == CLASS_ADVENTURER then
	players(p).s_info[79 + 1].value = 0
    end
    if players(p).pclass == CLASS_WARRIOR then
	if players(p).s_info[79 + 1].value == 0 then
            players(p).s_info[79 + 1].value = l
	end
    end
    if players(p).pclass == CLASS_MIMIC then
	if players(p).s_info[79 + 1].value == 0 then
            players(p).s_info[79 + 1].value = l
	end
    end
    if players(p).pclass == CLASS_PALADIN then
	if players(p).s_info[79 + 1].value == 0 then
            players(p).s_info[79 + 1].value = l
	end
    end
    if players(p).pclass == CLASS_RANGER then
	if players(p).s_info[79 + 1].value == 0 then
            players(p).s_info[79 + 1].value = l
	end
    end
end
--check whether player has a fraction of dodge skill
function hasdodge(p)
    --has dodge?
    if players(p).s_info[42+1].value > 0 then
	--but has no MA?
        if players(p).s_info[37+1].value == 0 then
	    --got it from interception then?
            if players(p).s_info[41+1].value > 0 then
		--not a new char?
		if players(p).lev > 1 then
		    lua_s_print("DODGE_FRACTION: "..p.."\n")
		end
	    end
	end
    end
end
--get rid of rogue_heavy_armor warnings if player didn't intend to skill dodging
--by just zeroing the remaining dodging skill value
function ddodge(p)
    players(p).s_info[42+1].value = 0
end

--turn a player into a king with all implications
function make_king(p, nazgul, michael, tik, hell, dor, zuaon)
    players(p).total_winner = 1
    --killed sauron
    players(p).r_killed[860 + 1] = 1
    --killed morgoth
    players(p).r_killed[862 + 1] = 1

    --optional:
    if nazgul == 1 then
        players(p).r_killed[946 + 1] = 1
        players(p).r_killed[947 + 1] = 1
        players(p).r_killed[948 + 1] = 1
        players(p).r_killed[949 + 1] = 1
        players(p).r_killed[950 + 1] = 1
        players(p).r_killed[951 + 1] = 1
        players(p).r_killed[952 + 1] = 1
        players(p).r_killed[953 + 1] = 1
        players(p).r_killed[954 + 1] = 1
    end
    if michael == 1 then
        players(p).r_killed[1074 + 1] = 1
    end
    if tik == 1 then
        players(p).r_killed[1032 + 1] = 1
    end
    if hell == 1 then
        players(p).r_killed[1067 + 1] = 1
    end
    if dor == 1 then
        players(p).r_killed[1085 + 1] = 1
    end
    if zuaon == 1 then
        players(p).r_killed[1097 + 1] = 1
    end
end

function def_blue(x)
	local v = rawget(globals(), x)
	if (v == nil) then return 0 end
	if (v == 0) then return 0 end
	return 1
end

function pvp(name)
	p = ind_loose(name)
	if (p == -1) then return -1 end
	msg_print(Ind, "  Kills (own): \255r"..players(p).kills.." ("..players(p).kills_own..")\255w   Kills(lo): "..players(p).kills_lower.."  Kills(hi): "..players(p).kills_higher.."  Kills(=): "..players(p).kills_equal)
	msg_print(Ind, "  Free mimicry: \255B"..players(p).free_mimic.."\255w   P-Tele: \255y"..players(p).pvp_prevent_tele.."\255w   Heal: \255g"..players(p).heal_effect)
end

--give a player a godly strike (instakill)
function gs(name)
	p = ind_loose(name)
	if (p == -1) then return -1 end
	players(p).admin_godly_strike = players(p).admin_godly_strike + 1;
	msg_print(Ind, "Player "..players(p).name.." has been blessed ("..players(p).admin_godly_strike..").")
end
function ggs(name)
	p = ind_loose(name)
	if (p == -1) then return -1 end
	msg_print(Ind, "Player "..players(p).name.."'s blessings remaining: "..players(p).admin_godly_strike..".")
end

--list players (like '@', basically)
function plist()
    local i
    for i = 1, NumPlayers do
	msg_print(Ind, "Player #"..i..": ("..players(i).accountname..") "..players(i).name..", R:"..players(i).prace.." C:"..players(i).pclass..", Lv "..players(i).lev.."/"..players(i).max_lev.." ("..players(i).max_plv..").")
    end
end

--like evileye's player_send()
function wrec(name, x, y)
    i = ind(name);
    players(i).recall_pos.wx = x;
    players(i).recall_pos.wy = y;
    players(i).recall_pos.wz = 0;
-- let's try LEVEL_OUTSIDE_RAND (5) instead of LEVEL_OUTSIDE (4) - C. Blue :)
    players(i).new_level_method = 5;
    msg_print(i, "\255yA magical gust of wind lifts you up and carries you away!");
    recall_player(i, "");
end

--recalls a player upwards
function rec(name)
    i = ind(name);
    players(i).recall_pos.wx = players(i).wpos.wx;
    players(i).recall_pos.wy = players(i).wpos.wy;
    players(i).recall_pos.wz = 0;
-- 7 = LEVEL_RECALL_UP, 8 = LEVEL_RECALL_DOWN
    players(i).new_level_method = 7;
    msg_print(i, "\255yA strong magical force carries you away!");
    recall_player(i, "");
end

--fix someone's Grond to new version oO
function _fg(name)
    local i, n
    i = ind(name)
    for n = 1, INVEN_TOTAL do
	if players(i).inventory[n].name1 == 111 then
	    players(i).inventory[n].to_h = 30
	    players(i).inventory[n].to_d = 30
	    players(i).inventory[n].to_a = 0
	    players(i).redraw = bor(player.redraw, 2147483648)
--	    msg_print(Ind, "Grond has been updated.")
	end
    end
--    else
--	msg_print(Ind, "No Grond found.")
--    end
end

--similar to fix_spellbooks() this function just replaces a single spell or swaps two spells
--swap == 0: just replace old by new; swap != 0: swap old and new; snew == -1: delete spell
function fix_spellbooks2(name, sold, snew, swap)
	local i, p, x
	p = ind(name)
	if (p == -1) then return -1 end

	--catch bad parameter choice
	if (snew == -1) then
		swap = 0
	end
	if (swap == 0) then
		for i = 1, INVEN_PACK do
			if ((players(p).inventory[i].tval == 111) and (players(p).inventory[i].sval == 255) and (players(p).inventory[i].pval == sold)) then
				if (snew == -1) then
					players(p).inventory[i].pval = 0
				else
					players(p).inventory[i].pval = snew
				end
			end
			if ((players(p).inventory[i].tval == 111) and (players(p).inventory[i].sval >= 100) and (players(p).inventory[i].sval <= 102)) then
				if players(p).inventory[i].xtra1 - 1 == sold then
					players(p).inventory[i].xtra1 = snew + 1
				end
				if players(p).inventory[i].xtra2 - 1 == sold then
					players(p).inventory[i].xtra2 = snew + 1
				end
				if players(p).inventory[i].xtra3 - 1 == sold then
					players(p).inventory[i].xtra3 = snew + 1
				end
				if players(p).inventory[i].xtra4 - 1 == sold then
					players(p).inventory[i].xtra4 = snew + 1
				end
				if players(p).inventory[i].xtra5 - 1 == sold then
					players(p).inventory[i].xtra5 = snew + 1
				end
				if players(p).inventory[i].xtra6 - 1 == sold then
					players(p).inventory[i].xtra6 = snew + 1
				end
				if players(p).inventory[i].xtra7 - 1 == sold then
					players(p).inventory[i].xtra7 = snew + 1
				end
				if players(p).inventory[i].xtra8 - 1 == sold then
					players(p).inventory[i].xtra8 = snew + 1
				end
				if players(p).inventory[i].xtra9 - 1 == sold then
					players(p).inventory[i].xtra9 = snew + 1
				end
			end
		end
	else
		for i = 1, INVEN_PACK do
			if ((players(p).inventory[i].tval == 111) and (players(p).inventory[i].sval == 255)) then
				if (players(p).inventory[i].pval == sold) then
					players(p).inventory[i].pval = snew
				elseif (players(p).inventory[i].pval == snew) then
					players(p).inventory[i].pval = sold
				end
			end
			if ((players(p).inventory[i].tval == 111) and (players(p).inventory[i].sval >= 100) and (players(p).inventory[i].sval <= 102)) then
				if players(p).inventory[i].xtra1 - 1 == sold then
					players(p).inventory[i].xtra1 = snew + 1
				elseif players(p).inventory[i].xtra1 - 1 == snew then
					players(p).inventory[i].xtra1 = sold + 1
				end
				if players(p).inventory[i].xtra2 - 1 == sold then
					players(p).inventory[i].xtra2 = snew + 1
				elseif players(p).inventory[i].xtra2 - 1 == snew then
					players(p).inventory[i].xtra2 = sold + 1
				end
				if players(p).inventory[i].xtra3 - 1 == sold then
					players(p).inventory[i].xtra3 = snew + 1
				elseif players(p).inventory[i].xtra3 - 1 == snew then
					players(p).inventory[i].xtra3 = sold + 1
				end
				if players(p).inventory[i].xtra4 - 1 == sold then
					players(p).inventory[i].xtra4 = snew + 1
				elseif players(p).inventory[i].xtra4 - 1 == snew then
					players(p).inventory[i].xtra4 = sold + 1
				end
				if players(p).inventory[i].xtra5 - 1 == sold then
					players(p).inventory[i].xtra5 = snew + 1
				elseif players(p).inventory[i].xtra5 - 1 == snew then
					players(p).inventory[i].xtra5 = sold + 1
				end
				if players(p).inventory[i].xtra6 - 1 == sold then
					players(p).inventory[i].xtra6 = snew + 1
				elseif players(p).inventory[i].xtra6 - 1 == snew then
					players(p).inventory[i].xtra6 = sold + 1
				end
				if players(p).inventory[i].xtra7 - 1 == sold then
					players(p).inventory[i].xtra7 = snew + 1
				elseif players(p).inventory[i].xtra7 - 1 == snew then
					players(p).inventory[i].xtra7 = sold + 1
				end
				if players(p).inventory[i].xtra8 - 1 == sold then
					players(p).inventory[i].xtra8 = snew + 1
				elseif players(p).inventory[i].xtra8 - 1 == snew then
					players(p).inventory[i].xtra8 = sold + 1
				end
				if players(p).inventory[i].xtra9 - 1 == sold then
					players(p).inventory[i].xtra9 = snew + 1
				elseif players(p).inventory[i].xtra9 - 1 == snew then
					players(p).inventory[i].xtra9 = sold + 1
				end
			end
		end
	end
end
