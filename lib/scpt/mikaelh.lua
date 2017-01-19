function setskill(skill, v)
	players(Ind).s_info[skill + 1].value = v
end

function get_skill_formatted(p, skill)
	local m = mod(players(p).s_info[skill + 1].value, 1000)
--	return (players(p).s_info[skill + 1].value / 1000) .. "." .. mod(players(p).s_info[skill + 1].value, 1000)

	if m < 10 then
	    m = "00" .. m
	elseif m < 100 then
	    m = "0" .. m
	end

	return (players(p).s_info[skill + 1].value / 1000) .. "." .. m
end

function get_skill_value(p, skill)
	return players(p).s_info[skill + 1].value
end

-- temporary
SKILL_DUAL = 78
SKILL_STANCE = 79

-- Neat list of player skills
function showskills(name)
	local p, f
	p = ind_loose(name)
	if (p == -1) then return -1 end
	f = players(p).skill_points
	msg_print(Ind, "\255BSkills of player '" .. players(p).name .. "', Ind " .. p .." (".. f .." points available)")
	if get_skill_value(p, SKILL_COMBAT) > 0 then		msg_print(Ind, " - Combat                         " .. get_skill_formatted(p, SKILL_COMBAT)) end
	if get_skill_value(p, SKILL_MASTERY) > 0 then		msg_print(Ind, "     - Weaponmastery              " .. get_skill_formatted(p, SKILL_MASTERY)) end
	if get_skill_value(p, SKILL_SWORD) > 0 then		msg_print(Ind, "         - Sword-mastery          " .. get_skill_formatted(p, SKILL_SWORD)) end
	if get_skill_value(p, SKILL_CRITS) > 0 then		msg_print(Ind, "             . Critical-strike    " .. get_skill_formatted(p, SKILL_CRITS)) end
	if get_skill_value(p, SKILL_AXE) > 0 then		msg_print(Ind, "         . Axe-mastery            " .. get_skill_formatted(p, SKILL_AXE)) end
	if get_skill_value(p, SKILL_BLUNT) > 0 then		msg_print(Ind, "         . Blunt-mastery          " .. get_skill_formatted(p, SKILL_BLUNT)) end
	if get_skill_value(p, SKILL_POLEARM) > 0 then		msg_print(Ind, "         . Polearm-mastery        " .. get_skill_formatted(p, SKILL_POLEARM)) end
	if get_skill_value(p, SKILL_STANCE) > 0 then		msg_print(Ind, "         . Combat Stances         " .. get_skill_formatted(p, SKILL_STANCE)) end
	if get_skill_value(p, SKILL_DUAL) > 0 then		msg_print(Ind, "     . Dual-Wield                 " .. get_skill_formatted(p, SKILL_DUAL)) end
	if get_skill_value(p, SKILL_ARCHERY) > 0 then		msg_print(Ind, "     - Archery                    " .. get_skill_formatted(p, SKILL_ARCHERY)) end
	if get_skill_value(p, SKILL_SLING) > 0 then		msg_print(Ind, "         . Sling-mastery          " .. get_skill_formatted(p, SKILL_SLING)) end
	if get_skill_value(p, SKILL_BOW) > 0 then		msg_print(Ind, "         . Bow-mastery            " .. get_skill_formatted(p, SKILL_BOW)) end
	if get_skill_value(p, SKILL_XBOW) > 0 then		msg_print(Ind, "         . Crossbow-mastery       " .. get_skill_formatted(p, SKILL_XBOW)) end
	if get_skill_value(p, SKILL_BOOMERANG) > 0 then		msg_print(Ind, "         . Boomerang-mastery      " .. get_skill_formatted(p, SKILL_BOOMERANG)) end
	if get_skill_value(p, SKILL_MARTIAL_ARTS) > 0 then	msg_print(Ind, "     . Martial Arts               " .. get_skill_formatted(p, SKILL_MARTIAL_ARTS)) end
	if get_skill_value(p, SKILL_INTERCEPT) > 0 then		msg_print(Ind, "     . Interception               " .. get_skill_formatted(p, SKILL_INTERCEPT)) end
	if get_skill_value(p, SKILL_MAGIC) > 0 then		msg_print(Ind, " - Magic                          " .. get_skill_formatted(p, SKILL_MAGIC)) end
	if get_skill_value(p, SKILL_MIMIC) > 0 then		msg_print(Ind, "     . Mimicry                    " .. get_skill_formatted(p, SKILL_MIMIC)) end
	if get_skill_value(p, SKILL_DEVICE) > 0 then		msg_print(Ind, "     . Magic-device               " .. get_skill_formatted(p, SKILL_DEVICE)) end
	if get_skill_value(p, SKILL_SPELL) > 0 then		msg_print(Ind, "     . Spell-power                " .. get_skill_formatted(p, SKILL_SPELL)) end
	if get_skill_value(p, SKILL_SORCERY) > 0 then		msg_print(Ind, "     . Sorcery                    " .. get_skill_formatted(p, SKILL_SORCERY)) end
	if get_skill_value(p, SKILL_MANA) > 0 then		msg_print(Ind, "     . Mana                       " .. get_skill_formatted(p, SKILL_MANA)) end
	if get_skill_value(p, SKILL_FIRE) > 0 then		msg_print(Ind, "     . Fire                       " .. get_skill_formatted(p, SKILL_FIRE)) end
	if get_skill_value(p, SKILL_WATER) > 0 then		msg_print(Ind, "     . Water                      " .. get_skill_formatted(p, SKILL_WATER)) end
	if get_skill_value(p, SKILL_AIR) > 0 then		msg_print(Ind, "     . Air                        " .. get_skill_formatted(p, SKILL_AIR)) end
	if get_skill_value(p, SKILL_EARTH) > 0 then		msg_print(Ind, "     . Earth                      " .. get_skill_formatted(p, SKILL_EARTH)) end
	if get_skill_value(p, SKILL_META) > 0 then		msg_print(Ind, "     . Meta                       " .. get_skill_formatted(p, SKILL_META)) end
	if get_skill_value(p, SKILL_CONVEYANCE) > 0 then	msg_print(Ind, "     . Conveyance                 " .. get_skill_formatted(p, SKILL_CONVEYANCE)) end
	if get_skill_value(p, SKILL_DIVINATION) > 0 then	msg_print(Ind, "     . Divination                 " .. get_skill_formatted(p, SKILL_DIVINATION)) end
	if get_skill_value(p, SKILL_TEMPORAL) > 0 then		msg_print(Ind, "     . Temporal                   " .. get_skill_formatted(p, SKILL_TEMPORAL)) end
	if get_skill_value(p, SKILL_MIND) > 0 then		msg_print(Ind, "     . Mind                       " .. get_skill_formatted(p, SKILL_MIND)) end
	if get_skill_value(p, SKILL_NATURE) > 0 then		msg_print(Ind, "     . Nature                     " .. get_skill_formatted(p, SKILL_NATURE)) end
	if get_skill_value(p, SKILL_UDUN) > 0 then		msg_print(Ind, "     . Udun                       " .. get_skill_formatted(p, SKILL_UDUN)) end
	if get_skill_value(p, SKILL_HOFFENSE) > 0 then		msg_print(Ind, "     . Holy Offense               " .. get_skill_formatted(p, SKILL_HOFFENSE)) end
	if get_skill_value(p, SKILL_HDEFENSE) > 0 then		msg_print(Ind, "     . Holy Defense               " .. get_skill_formatted(p, SKILL_HDEFENSE)) end
	if get_skill_value(p, SKILL_HCURING) > 0 then		msg_print(Ind, "     . Holy Curing                " .. get_skill_formatted(p, SKILL_HCURING)) end
	if get_skill_value(p, SKILL_HSUPPORT) > 0 then		msg_print(Ind, "     . Holy Support               " .. get_skill_formatted(p, SKILL_HSUPPORT)) end
	if get_skill_value(p, SKILL_DRUID_ARCANE) > 0 then	msg_print(Ind, "     . Arcane Lore                " .. get_skill_formatted(p, SKILL_DRUID_ARCANE)) end
	if get_skill_value(p, SKILL_DRUID_PHYSICAL) > 0 then	msg_print(Ind, "     . Physical Lore              " .. get_skill_formatted(p, SKILL_DRUID_PHYSICAL)) end
	if get_skill_value(p, SKILL_ASTRAL) > 0 then		msg_print(Ind, "     . Astral Knowledge           " .. get_skill_formatted(p, SKILL_ASTRAL)) end
	if get_skill_value(p, SKILL_PPOWER) > 0 then		msg_print(Ind, "     . Psycho-Power               " .. get_skill_formatted(p, SKILL_PPOWER)) end
	if get_skill_value(p, SKILL_TCONTACT) > 0 then		msg_print(Ind, "     . Attunement                 " .. get_skill_formatted(p, SKILL_TCONTACT)) end
	if get_skill_value(p, SKILL_MINTRUSION) > 0 then	msg_print(Ind, "     . Mental Intrusion           " .. get_skill_formatted(p, SKILL_MINTRUSION)) end
-- Occult
--if (def_hack("TEST_SERVER", nil)) then
if (def_hack("TEMP3", nil)) then
	if get_skill_value(p, SKILL_OSHADOW) > 0 then		msg_print(Ind, "     . Shadow                     " .. get_skill_formatted(p, SKILL_OSHADOW)) end
	if get_skill_value(p, SKILL_OSPIRIT) > 0 then		msg_print(Ind, "     . Spirit                     " .. get_skill_formatted(p, SKILL_OSPIRIT)) end
	if get_skill_value(p, SKILL_OHERETICISM) > 0 then	msg_print(Ind, "     . Hereticism                 " .. get_skill_formatted(p, SKILL_OHERETICISM)) end
elseif (def_hack("TEMP2", nil)) then
	if get_skill_value(p, SKILL_OSHADOW) > 0 then		msg_print(Ind, "     . Shadow                     " .. get_skill_formatted(p, SKILL_OSHADOW)) end
	if get_skill_value(p, SKILL_OSPIRIT) > 0 then		msg_print(Ind, "     . Spirit                     " .. get_skill_formatted(p, SKILL_OSPIRIT)) end
end
	if get_skill_value(p, SKILL_RUNEMASTERY ) > 0 then	msg_print(Ind, " . Runemastery                    " .. get_skill_formatted(p, SKILL_RUNEMASTERY)) end
	if get_skill_value(p, SKILL_SCHOOL_RUNECRAFT) > 0 then	msg_print(Ind, " . Runecraft                      " .. get_skill_formatted(p, SKILL_SCHOOL_RUNECRAFT)) end
	if get_skill_value(p, SKILL_R_FIRECOLD) > 0 then	msg_print(Ind, "     . Fire/Cold                  " .. get_skill_formatted(p, SKILL_R_FIRECOLD)) end
	if get_skill_value(p, SKILL_R_WATEACID) > 0 then	msg_print(Ind, "     . Water/Acid                 " .. get_skill_formatted(p, SKILL_R_WATEACID)) end
	if get_skill_value(p, SKILL_R_ELECEART) > 0 then	msg_print(Ind, "     . Electricity/Earth          " .. get_skill_formatted(p, SKILL_R_ELECEART)) end
	if get_skill_value(p, SKILL_R_WINDPOIS) > 0 then	msg_print(Ind, "     . Wind/Poison                " .. get_skill_formatted(p, SKILL_R_WINDPOIS)) end
	if get_skill_value(p, SKILL_R_MANACHAO) > 0 then	msg_print(Ind, "     . Mana/Chaos                 " .. get_skill_formatted(p, SKILL_R_MANACHAO)) end
	if get_skill_value(p, SKILL_R_FORCGRAV) > 0 then	msg_print(Ind, "     . Force/Gravity              " .. get_skill_formatted(p, SKILL_R_FORCGRAV)) end
	if get_skill_value(p, SKILL_R_NETHTIME) > 0 then	msg_print(Ind, "     . Nether/Time                " .. get_skill_formatted(p, SKILL_R_NETHTIME)) end
	if get_skill_value(p, SKILL_R_MINDNEXU) > 0 then	msg_print(Ind, "     . Mind/Nexus                 " .. get_skill_formatted(p, SKILL_R_MINDNEXU)) end
	if get_skill_value(p, SKILL_SNEAKINESS) > 0 then	msg_print(Ind, " - Sneakiness                     " .. get_skill_formatted(p, SKILL_SNEAKINESS)) end
	if get_skill_value(p, SKILL_STEALTH) > 0 then		msg_print(Ind, "     . Stealth                    " .. get_skill_formatted(p, SKILL_STEALTH)) end
	if get_skill_value(p, SKILL_DISARM) > 0 then		msg_print(Ind, "     - Disarming                  " .. get_skill_formatted(p, SKILL_DISARM)) end
	if get_skill_value(p, SKILL_TRAPPING) > 0 then		msg_print(Ind, "         . Trapping               " .. get_skill_formatted(p, SKILL_TRAPPING)) end
	if get_skill_value(p, SKILL_BACKSTAB) > 0 then		msg_print(Ind, "     . Backstabbing               " .. get_skill_formatted(p, SKILL_BACKSTAB)) end
	if get_skill_value(p, SKILL_STEALING) > 0 then		msg_print(Ind, "     . Stealing                   " .. get_skill_formatted(p, SKILL_STEALING)) end
	if get_skill_value(p, SKILL_DODGE) > 0 then		msg_print(Ind, "     . Dodging                    " .. get_skill_formatted(p, SKILL_DODGE)) end
	if get_skill_value(p, SKILL_CALMNESS) > 0 then		msg_print(Ind, "     . Calmness                   " .. get_skill_formatted(p, SKILL_CALMNESS)) end
	if get_skill_value(p, SKILL_NECROMANCY) > 0 then	msg_print(Ind, " - Necromancy                     " .. get_skill_formatted(p, SKILL_NECROMANCY)) end
	if get_skill_value(p, SKILL_TRAUMATURGY) > 0 then	msg_print(Ind, "     . Traumaturgy                " .. get_skill_formatted(p, SKILL_TRAUMATURGY)) end
	if get_skill_value(p, SKILL_AURA_FEAR) > 0 then		msg_print(Ind, "     . Aura of Fear               " .. get_skill_formatted(p, SKILL_AURA_FEAR)) end
	if get_skill_value(p, SKILL_AURA_SHIVER) > 0 then	msg_print(Ind, "     . Shivering Aura             " .. get_skill_formatted(p, SKILL_AURA_SHIVER)) end
	if get_skill_value(p, SKILL_AURA_DEATH) > 0 then	msg_print(Ind, "     . Aura of Death              " .. get_skill_formatted(p, SKILL_AURA_DEATH)) end
	if get_skill_value(p, SKILL_ANTIMAGIC) > 0 then		msg_print(Ind, " . Anti-magic                     " .. get_skill_formatted(p, SKILL_ANTIMAGIC)) end
	if get_skill_value(p, SKILL_HEALTH) > 0 then		msg_print(Ind, " - Health                         " .. get_skill_formatted(p, SKILL_HEALTH)) end
--	if get_skill_value(p, SKILL_TRAINING) > 0 then		msg_print(Ind, "     . Training                   " .. get_skill_formatted(p, SKILL_TRAINING)) end
	if get_skill_value(p, SKILL_SWIM) > 0 then		msg_print(Ind, "     . Swimming                   " .. get_skill_formatted(p, SKILL_SWIM)) end
	if get_skill_value(p, SKILL_DIG) > 0 then		msg_print(Ind, "     . Digging                    " .. get_skill_formatted(p, SKILL_DIG)) end
	if get_skill_value(p, SKILL_CLIMB) > 0 then		msg_print(Ind, "     . Climbing                   " .. get_skill_formatted(p, SKILL_CLIMB)) end
--	if get_skill_value(p, SKILL_LEVITATE) > 0 then		msg_print(Ind, "     . Levitation                 " .. get_skill_formatted(p, SKILL_LEVITATE)) end
--	if get_skill_value(p, SKILL_FREEACT) > 0 then		msg_print(Ind, "     . Free Action                " .. get_skill_formatted(p, SKILL_FREEACT)) end
--	if get_skill_value(p, SKILL_RESCONF) > 0 then		msg_print(Ind, "     . Confusion Resistance       " .. get_skill_formatted(p, SKILL_RESCONF)) end
end

-- Alias for showskills()
function skills(name)
	showskills(name)
end

-- Manual forced update
function update_p(name)
	players(name).update = bor(players(name).update, 3145919)

-- 3145919 is a combination of the following:
-- PU_BONUS		0x00000001L	Calculate bonuses
-- PU_TORCH		0x00000002L	Calculate torch radius
-- PU_SKILL_INFO	0x00000004L	Update client skill info
-- PU_SANITY		0x00000008L	Calculate csane and msane
-- PU_HP		0x00000010L	Calculate chp and mhp
-- PU_MANA		0x00000020L	Calculate csp and msp
-- PU_SKILL_MOD		0x00000080L	Update client skill values/...
-- PU_VIEW		0x00100000L	Update view
-- PU_LITE		0x00200000L	Update lite

end

-- Manual forced redraw
function redraw_p(name)
	players(name).redraw = bor(players(name).redraw, 134217727)

-- and the magic number here comes from:
-- 2^27 - 1 = 134217727
end

-- Manual forced update & redraw
function ur_p(name)
	update_p(name)
	redraw_p(name)
end

-- Fix spellbooks after adding new spells
-- Usage: fix_spellbooks(<player name>, <new spell number>, 1)
-- Example, after adding the Stop Wraithform spell:
-- fix_spellbooks("Pfft", STOPWRAITH, 1)

function fix_spellbooks(name, start, mod)
	local i, p, x
	p = ind(name)
	if (p == -1) then return -1 end
	for i = 1, INVEN_PACK do
		if ((players(p).inventory[i].tval == 111) and (players(p).inventory[i].sval == 255) and (players(p).inventory[i].pval >= start)) then
			players(p).inventory[i].pval = players(p).inventory[i].pval + mod
		end
		if ((players(p).inventory[i].tval == 111) and (players(p).inventory[i].sval >= 100) and (players(p).inventory[i].sval <= 102)) then
			if players(p).inventory[i].xtra1 - 1 >= start then
				players(p).inventory[i].xtra1 = players(p).inventory[i].xtra1 + mod
			end
			if players(p).inventory[i].xtra2 - 1 >= start then
				players(p).inventory[i].xtra2 = players(p).inventory[i].xtra2 + mod
			end
			if players(p).inventory[i].xtra3 - 1 >= start then
				players(p).inventory[i].xtra3 = players(p).inventory[i].xtra3 + mod
			end
			if players(p).inventory[i].xtra4 - 1 >= start then
				players(p).inventory[i].xtra4 = players(p).inventory[i].xtra4 + mod
			end
			if players(p).inventory[i].xtra5 - 1 >= start then
				players(p).inventory[i].xtra5 = players(p).inventory[i].xtra5 + mod
			end
			if players(p).inventory[i].xtra6 - 1 >= start then
				players(p).inventory[i].xtra6 = players(p).inventory[i].xtra6 + mod
			end
			if players(p).inventory[i].xtra7 - 1 >= start then
				players(p).inventory[i].xtra7 = players(p).inventory[i].xtra7 + mod
			end
			if players(p).inventory[i].xtra8 - 1 >= start then
				players(p).inventory[i].xtra8 = players(p).inventory[i].xtra8 + mod
			end
			if players(p).inventory[i].xtra9 - 1 >= start then
				players(p).inventory[i].xtra9 = players(p).inventory[i].xtra9 + mod
			end
		end
	end
end


-- Fix for the stop wraithform spell
--[[
function fix1(name)
	fix_spellbooks(name, STOPWRAITH, 1)
end
]]

-- Establish VNC mind link
--I added bor() and band() like in cblue.lua, to avoid killing Mindcrafter links
function vnc_p2p(target, receiver)
    local p, p2, id
    p = ind(target)
    if (p == -1) then return -1 end
    p2 = ind(receiver)
    if (p2 == -1) then return -1 end
    id = players(p2).id
    players(p).esp_link = id
    players(p).esp_link_type = 1
    players(p).esp_link_end = 0
    players(p).esp_link_flags = bor(players(p).esp_link_flags, 1 + 128)
    msg_print(p2, "Mind link established.")
end

-- Break VNC mind link
-- needed?
--[[
function vncoff_p2p(name, receiver)
    local p, p2
    p = ind(name)
    if (p == -1) then return -1 end
    p2 = ind(receiver)
    if (p2 == -1) then return -1 end
    
    players(p).esp_link = 0
    players(p).esp_link_type = 0
    players(p).esp_link_end = 0
    players(p).esp_link_flags = band(players(p).esp_link_flags, bnot(1 + 128))
    msg_print(p2, "Mind link broken.")
end
]]

-- Effects
function fire_fx(name, rad, time)
	ball(name, rad, 5, 0, 0, 16+256+1024, time, "")
end

function cold_fx(name, rad, time)
	ball(name, rad, 4, 0, 0, 16+256+1024, time, "")
end

function acid_fx(name, rad, time)
	ball(name, rad, 3, 0, 0, 16+256+1024, time, "")
end

function pois_fx(name, rad, time)
	ball(name, rad, 2, 0, 0, 16+256+1024, time, "")
end

function elec_fx(name, rad, time)
	ball(name, rad, 1, 0, 0, 16+256+1024, time, "")
end

function lite_fx(name, rad, time)
	ball(name, rad, 15, 0, 0, 16+256+1024, time, "")
end

function any_fx(name, rad, typ, time)
	ball(name, rad, typ, 0, 0, 16+256+1024, time, "")
end
