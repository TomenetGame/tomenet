-- $Id: spells.lua,v 1.36 2003/11/22 23:47:44 cblue Exp $
-- This file takes care of the schools of magic
-- (Edit this file and funny funny things will happen :)

-- Create the schools --

-- Istari primarily

SCHOOL_CONVEYANCE = add_school {
	["name"] = "Conveyance",
	["skill"] = SKILL_CONVEYANCE,
	["sorcery"] = TRUE,
}
SCHOOL_MANA = add_school {
	["name"] = "Mana",
	["skill"] = SKILL_MANA,
	["sorcery"] = TRUE,
}
SCHOOL_FIRE = add_school {
	["name"] = "Fire",
	["skill"] = SKILL_FIRE,
	["sorcery"] = TRUE,
}
SCHOOL_AIR = add_school {
	["name"] = "Air",
	["skill"] = SKILL_AIR,
	["sorcery"] = TRUE,
}
SCHOOL_WATER = add_school {
	["name"] = "Water",
	["skill"] = SKILL_WATER,
	["sorcery"] = TRUE,
}
SCHOOL_EARTH = add_school {
	["name"] = "Earth",
	["skill"] = SKILL_EARTH,
	["sorcery"] = TRUE,
}
SCHOOL_TEMPORAL = add_school {
	["name"] = "Temporal",
	["skill"] = SKILL_TEMPORAL,
	["sorcery"] = TRUE,
}
SCHOOL_NATURE = add_school {
	["name"] = "Nature",
	["skill"] = SKILL_NATURE,
	["sorcery"] = TRUE,
}
--[[SCHOOL_META = add_school {
	["name"] = "Meta",
	["skill"] = SKILL_META,
	["sorcery"] = TRUE,
}]]--
SCHOOL_MIND = add_school {
	["name"] = "Mind",
	["skill"] = SKILL_MIND,
	["sorcery"] = TRUE,
}
SCHOOL_DIVINATION = add_school {
	["name"] = "Divination",
	["skill"] = SKILL_DIVINATION,
	["sorcery"] = TRUE,
}
SCHOOL_UDUN = add_school {
	["name"] = "Udun",
	["skill"] = SKILL_UDUN,
	["sorcery"] = TRUE,
}

-- Priests / Paladins

SCHOOL_HOFFENSE = add_school {
	["name"] = "Holy Offense",
	["skill"] = SKILL_HOFFENSE,
}
SCHOOL_HDEFENSE = add_school {
	["name"] = "Holy Defense",
	["skill"] = SKILL_HDEFENSE,
}
SCHOOL_HCURING = add_school {
	["name"] = "Holy Curing",
	["skill"] = SKILL_HCURING,
}
SCHOOL_HSUPPORT = add_school {
	["name"] = "Holy Support",
	["skill"] = SKILL_HSUPPORT,
}

-- Druids

SCHOOL_DRUID_ARCANE = add_school {
	["name"] = "Arcane Lore",
	["skill"] = SKILL_DRUID_ARCANE,
}
SCHOOL_DRUID_PHYSICAL = add_school {
	["name"] = "Physical Lore",
	["skill"] = SKILL_DRUID_PHYSICAL,
}

-- Maiar

SCHOOL_ASTRAL = add_school {
	["name"] = "Astral Knowledge",
	["skill"] = SKILL_ASTRAL,
}

-- Mindcrafters

SCHOOL_PPOWER = add_school {
	["name"] = "Psycho-power",
	["skill"] = SKILL_PPOWER,
}
SCHOOL_TCONTACT = add_school {
	["name"] = "Attunement",
	["skill"] = SKILL_TCONTACT,
}
SCHOOL_MINTRUSION = add_school {
	["name"] = "Mental intrusion",
	["skill"] = SKILL_MINTRUSION,
}

-- Occult
if (def_hack("TEMP2", nil)) then
	SCHOOL_OSHADOW = add_school {
		["name"] = "Shadow",
		["skill"] = SKILL_OSHADOW,
	}
	SCHOOL_OSPIRIT = add_school {
		["name"] = "Spirit",
		["skill"] = SKILL_OSPIRIT,
	}
	-- If Occult and Death Knight are enabled, this is a hack for 'Nether Bolt' spell
	SCHOOL_NECROMANCY = add_school {
		["name"] = "Necromancy",
		["skill"] = SKILL_NECROMANCY,
	}
	-- Occult: Hereticism (for Hell Knights)
	if (def_hack("TEMP3", nil)) then
		SCHOOL_OHERETICISM = add_school {
			["name"] = "Hereticism",
			["skill"] = SKILL_OHERETICISM,
		}
	end
end


-- Put some spells

pern_dofile(Ind, "s_mana.lua")
pern_dofile(Ind, "s_fire.lua")
pern_dofile(Ind, "s_air.lua")
pern_dofile(Ind, "s_water.lua")
pern_dofile(Ind, "s_earth.lua")
pern_dofile(Ind, "s_convey.lua")
pern_dofile(Ind, "s_divin.lua")
pern_dofile(Ind, "s_tempo.lua")
--pern_dofile(Ind, "s_meta.lua")
pern_dofile(Ind, "s_nature.lua")
pern_dofile(Ind, "s_mind.lua")
pern_dofile(Ind, "s_udun.lua")

pern_dofile(Ind, "p_offense.lua")
pern_dofile(Ind, "p_defense.lua")
pern_dofile(Ind, "p_curing.lua")
pern_dofile(Ind, "p_support.lua")

pern_dofile(Ind, "dr_arcane.lua")
pern_dofile(Ind, "dr_physical.lua")

__lua_M_FIRST = __tmp_spells_num
pern_dofile(Ind, "m_ppower.lua")
pern_dofile(Ind, "m_tcontact.lua")
pern_dofile(Ind, "m_mintrusion.lua")
__lua_M_LAST = __tmp_spells_num - 1

pern_dofile(Ind, "d_astral.lua")

-- Occult
if (def_hack("TEMP2", nil)) then
	pern_dofile(Ind, "o_shadow.lua")
	pern_dofile(Ind, "o_spirit.lua")
	if (def_hack("TEMP3", nil)) then
		pern_dofile(Ind, "o_hereticism.lua")
	end
end


-- Tomes / Greater crystals

-- Create the crystal of mana (1-4)
school_book[0] = { DISPERSEMAGIC, MANASHIELD, MANATHRUST_III, RECHARGE_III, DELCURSES_II, }
-- The book of the eternal flame (5-8)
school_book[1] = { RESISTS_II, GLOBELIGHT_II, FIERYAURA_II, FIREBOLT_III, FIREWALL_II, FIREBALL_II, FIREFLASH_II, }
-- The book of the blowing winds (9-13)
--school_book[2] = { AIRWINGS, RESISTS_II, THUNDERSTORM, POISONBLOOD, NOXIOUSCLOUD_III, INVISIBILITY, LIGHTNINGBOLT_III, }
school_book[2] = { AIRWINGS, RESISTS_II, THUNDERSTORM, NOXIOUSCLOUD_III, INVISIBILITY, LIGHTNINGBOLT_III, }
-- The book of the impenetrable earth (14-17)
school_book[3] = {
--	STONESKIN, DIG, STONEPRISON, SHAKE, STRIKE_II,
	DIG, RESISTS_II, STONEPRISON, SHAKE, STRIKE_II, ACIDBOLT_III,
}
-- The book of the everrunning wave (18-21)
school_book[4] = { ENTPOTION, RESISTS_II, TIDALWAVE_II, ICESTORM_II, VAPOR_III, FROSTBOLT_III, WATERBOLT_III, FROSTBALL_II, }
-- Create the book of translocation (22-27)
school_book[5] = { BLINK, DISARM, TELEPORT, RECALL, PROBABILITY_TRAVEL, TELEAWAY_II, }
-- Create the book of the tree * SUMMONANIMAL requires pets first (28-32)
--school_book[6] = { VERMINCONTROL, REGENERATION, GROWTREE, RECOVERY_II, HEALING_III, }
school_book[6] = { VERMINCONTROL, REGENERATION, GROWTREE, POISONBLOOD, RECOVERY_II, HEALING_III, }
-- Create the book of Knowledge (33-38)
school_book[7] = { DETECTMONSTERS, REVEALWAYS, SENSEHIDDEN_II, IDENTIFY_III,  STARIDENTIFY, VISION_II,}
-- Create the book of the Time (39-42)
school_book[8] = { ESSENSESPEED, SLOWMONSTER_II, MAGELOCK_II, MASSWARP, }
-- Create the book of meta spells (43-45)
--school_book[9] = { PROJECT_SPELLS, DISPERSEMAGIC, RECHARGE_III, }
-- Create the book of the mind * CHARM requires pets first (46-48)
school_book[10] = { CONFUSE_II, TELEKINESIS, SENSEMONSTERS, STUN_II, }
-- Create the book of hellflame * DRAIN, FLAMEOFUDUN missing (49-53)
school_book[11] = { GENOCIDE_I, GENOCIDE_II, WRAITHFORM, STOPWRAITH, DISEBOLT, HELLFIRE_II, }

-- Priests / Paladins:

-- Create the book of Holy Offense (54-60)
if (def_hack("TEST_SERVER", nil)) then
	school_book[12] = { HGLOBELIGHT_II, HCURSE_III, HORBDRAIN_II, HEXORCISM_II, HRELSOULS_III, HDRAINCLOUD, EARTHQUAKE, HCURSEDD, }
else
	school_book[12] = { HGLOBELIGHT_II, HCURSE_III, HORBDRAIN_II, HEXORCISM_II, HRELSOULS_III, HDRAINCLOUD, EARTHQUAKE, }
end
-- Create the book of Holy Defense (61-65)
school_book[13] = { HPROTEVIL, HBLESSING_III, DISPELMAGIC, HRUNEPROT, HRESISTS_III, HMARTYR, }
-- Create the book of Holy Curing (66-72)
school_book[14] = { HCURING_III, HSANITY, HCUREWOUNDS_II, HDELBB, HRESURRECT, HRESTORING, HDELCURSES_II, HHEALING_III, HHEALING2_III, }
-- Create the book of Holy Support (73-79)
school_book[15] = { HDELFEAR, HDETECTEVIL, HSATISFYHUNGER, HGLOBELIGHT_II, HSANCTUARY_II, HSENSEMON, HSENSE_II, HZEAL_III, }

-- Druids:

-- Create the book of druidism: Arcane Lore (80-84)
school_book[16] = { NATURESCALL, BAGIDENTIFY, WATERPOISON_III, REPLACEWALL, BANISHANIMALS, }
-- Create the book of druidism: Physical Lore (85-89)
school_book[17] = { FOCUS, HERBALTEA, QUICKFEET, HEALINGCLOUD_III, EXTRASTATS_II, }

-- Maiar:

-- Divine Race Tome
school_book[18] = { POWERBOLT_III, POWERBEAM_III, POWERBALL_III, RELOCATION, VENGEANCE, EMPOWERMENT, GATEWAY, INTENSIFY, POWERCLOUD, }

-- Mindcraft:

if (def_hack("TEST_SERVER", nil)) then
	school_book[19] = {MBASH, MDISARM, MBLINK, MTELEPORT, MTELETOWARDS, MFEEDBACK, MPYROKINESIS_II, MCRYOKINESIS_II, MTELEAWAY, MTELEKINESIS, MSHIELD, MFUSION,}
	school_book[20] = {MCURE, MBOOST, MSELFKNOW, MHASTE, MIDENTIFY, MSENSEMON, MSANITY, MTELEKINESIS, MFUSION,}
else
	-- Create the book of mindcrafting: Psycho-power (-)
	school_book[19] = { MBASH, MDISARM, MBLINK, MTELEPORT, MTELETOWARDS, MFEEDBACK, MPYROKINESIS_II, MCRYOKINESIS_II, MTELEAWAY, MTELEKINESIS, MSHIELD, }
	-- Create the book of mindcrafting: Thought contact (aka Attunement)
	school_book[20] = { MCURE, MBOOST, MSELFKNOW, MHASTE, MIDENTIFY, MSENSEMON, MSANITY, MTELEKINESIS, }
end
-- Create the book of mindcrafting: Mental intrusion (-)
school_book[21] = { MSCARE_II, MCONFUSE_II, MSLEEP_II, MSLOWMONSTER_II, MPSISTORM_II, MMINDBLAST_III, MSILENCE, MIDENTIFY, MMAP, MCHARM, MSTOPCHARM, }

-- Create the Occult books, Shadow and Spirit (-)
if (def_hack("TEMP2", nil)) then
	--school_book[22] = { DETECTINVIS, OFEAR_II, OBLIND_II, OSLEEP_II, SHADOWGATE, OINVIS, POISONFOG_III, DARKBOLT_III, ODRAINLIFE, } --shadow
	--school_book[22] = { DETECTINVIS, OFEAR_II, OBLIND_II, OSLEEP_II, SHADOWGATE, OINVIS, CHAOSBOLT, DARKBOLT_III, ODRAINLIFE, } --shadow
	--school_book[22] = { DETECTINVIS, POISONRES, OFEAR_II, OBLIND_II, OSLEEP_II, SHADOWGATE, OINVIS, CHAOSBOLT, NETHERBOLT, POISONFOG_III, DARKBOLT_III, ODRAINLIFE, DARKBALL } --shadow/ENABLE_DEATHKNIGHT
	school_book[22] = { DETECTINVIS, POISONRES, OFEAR_II, OBLIND_II, OSLEEP_II, SHADOWGATE, OINVIS, CHAOSBOLT, NETHERBOLT, DARKBOLT_III, ODRAINLIFE, DARKBALL } --shadow/ENABLE_DEATHKNIGHT
	school_book[23] = { ODELFEAR, MEDITATION, TRANCE, POSSESS, STOPPOSSESS, STARLIGHT_II, DETECTCREATURES, OCURSEDD_III, OLIGHTNINGBOLT_III, LITEBEAM_III, ODELCURSES_II, GUARDIANSPIRIT_II, RITES_II } --spirit

	-- Create the Occult book Hereticism
	if (def_hack("TEMP3", nil)) then
		--school_book[24] = { TERROR_II, OFIREBOLT_III, FIRERES, CHAOSBOLTH, SAPLIFE, FIRESTORM, BLOODSACRIFICE } --ENABLE_HELLKNIGHT
		school_book[24] = { TERROR_II, ODELFEAR2, OFIREBOLT_III, FIRERES, OEXTRASTATS, CHAOSBOLT2, ORESTORING, LEVITATION, FIRESTORM, BLOODSACRIFICE } --ENABLE_HELLKNIGHT
	end
end

-- Handbooks:

-- Create the book of beginner's cantrip
school_book[50] = { MANATHRUST_I, GLOBELIGHT_I, BLINK, DETECTMONSTERS, SENSEHIDDEN_I, ENTPOTION }
-- Create the elementalist's handbook
school_book[51] = {
--	NOXIOUSCLOUD_III, THUNDERSTORM, AIRWINGS, FIREBALL_II, FIREWALL_II, TIDALWAVE_II, ICESTORM_II, SHAKE
	NOXIOUSCLOUD_III, THUNDERSTORM, AIRWINGS, RESISTS_II, FIERYAURA_II, VAPOR_III, ICESTORM_II, SHAKE, FROSTBALL_II, FIREBALL_II
}
-- Create the handbook for treasure hunting
school_book[52] = { SENSEHIDDEN_II, REVEALWAYS, DIG, DISARM, IDENTIFY_III, GLOBELIGHT_II, DELCURSES_II }
-- Create the handbook of piety
school_book[53] = { HBLESSING_III, HSANCTUARY_II, HSATISFYHUNGER, HDELCURSES_II, HPROTEVIL, HEXORCISM_II, HRELSOULS_III }
-- Create the naturalist's handbook
school_book[54] = {
--	NATURESCALL, REPLACEWALL, HERBALTEA, VAPOR_III, GROWTREE, RECOVERY_II, REGENERATION, POISONBLOOD
	NATURESCALL, REPLACEWALL, HERBALTEA, GROWTREE, RECOVERY_II, REGENERATION, POISONBLOOD
}
-- Create the destroyer's handbook
--occult available?
if (def_hack("TEMP3", nil)) then
school_book[55] = {
--	ICESTORM_II, HELLFIRE_II, FIREBALL_II, SHAKE, DISEBOLT, THUNDERSTORM, HORBDRAIN, HDRAINCLOUD
	TIDALWAVE_II, HELLFIRE_II, FIREBALL_II, STRIKE_II, DISEBOLT, THUNDERSTORM, HORBDRAIN_II, HDRAINCLOUD, CHAOSBOLT, CHAOSBOLT2, OFIREBOLT_III
}
elseif (def_hack("TEMP2", nil)) then
school_book[55] = {
--	ICESTORM_II, HELLFIRE_II, FIREBALL_II, SHAKE, DISEBOLT, THUNDERSTORM, HORBDRAIN, HDRAINCLOUD
	TIDALWAVE_II, HELLFIRE_II, FIREBALL_II, STRIKE_II, DISEBOLT, THUNDERSTORM, HORBDRAIN_II, HDRAINCLOUD, CHAOSBOLT
}
else
school_book[55] = {
--	ICESTORM_II, HELLFIRE_II, FIREBALL_II, SHAKE, DISEBOLT, THUNDERSTORM, HORBDRAIN, HDRAINCLOUD
	TIDALWAVE_II, HELLFIRE_II, FIREBALL_II, STRIKE_II, DISEBOLT, THUNDERSTORM, HORBDRAIN_II, HDRAINCLOUD
}
end
---- Create the handbook to the underworld
--school_book[55] = {
--	HELLFIRE_II, GENOCIDE_I, GENOCIDE_II, MASSWARP, CONFUSE_II, FIERYAURA_II
--}
-- Create the handbook of novice etiquette
school_book[56] = { HDELFEAR, HBLESSING_I, HCURING_I, HGLOBELIGHT_I, HDETECTEVIL, HHEALING_I}
---- Create the handbook for rogues (of deception)
if (def_hack("TEMP2", nil)) then
	--include Occult Shadow spells
	--school_book[57] = { BLINK, POISONFOG_III, OFEAR_II, OBLIND_II, DETECTINVIS, SENSEHIDDEN_II, REVEALWAYS, VISION_II, OINVIS, INVISIBILITY }
	school_book[57] = { BLINK, TELEPORT, OFEAR_II, OBLIND_II, OINVIS, SHADOWGATE }
else
	school_book[57] = { BLINK, NOXIOUSCLOUD_III, SENSEHIDDEN_II, REVEALWAYS, VISION_II, INVISIBILITY }
end
---- Create the handbook for dungeon masters & wizards (of dungeon keeping)
school_book[58] = { TELEKINESIS, DIG, STONEPRISON, GROWTREE, DISARM, VISION_II, STARIDENTIFY, MANATHRUST_III, DISEBOLT, FIREBALL_II, FIREFLASH_II, RECHARGE_III, MAGELOCK_II }
-- Create the handbook of revelation
if (def_hack("TEMP2", nil)) then
	--include Occult spells
	school_book[59] = { MEDITATION, IDENTIFY_III, STARIDENTIFY, SENSEHIDDEN_II, DETECTMONSTERS, SENSEMONSTERS, MSELFKNOW, MSANITY, HDETECTEVIL, MSENSEMON, MIDENTIFY, DETECTCREATURES }
else
	school_book[59] = { IDENTIFY_III, STARIDENTIFY, SENSEHIDDEN_II, DETECTMONSTERS, SENSEMONSTERS, MSELFKNOW, MSANITY, HDETECTEVIL, MSENSEMON, MIDENTIFY }
end
-- Create the handbook of transportation
if (def_hack("TEMP2", nil)) then
	--include Occult Shadow spells
	school_book[60] = {
	    --TELEAWAY_I, MTELEAWAY,
	    BLINK, TELEPORT, RECALL, TELEKINESIS, MBLINK, MTELEPORT, MTELETOWARDS, MTELEKINESIS, RELOCATION, GATEWAY, SHADOWGATE }
else
	school_book[60] = {
	    --TELEAWAY_I, MTELEAWAY,
	    BLINK, TELEPORT, RECALL, TELEKINESIS, MBLINK, MTELEPORT, MTELETOWARDS, MTELEKINESIS, RELOCATION, GATEWAY }
end
-- Create the handbook of first visions
if (def_hack("TEMP2", nil)) then
	--include Occult spells
	school_book[61] = { OCURSEDD_I, ODELFEAR, STARLIGHT_I, MEDITATION, OFEAR_I, DETECTINVIS, HDETECTEVIL, SENSEHIDDEN_I }
else
	school_book[61] = { HDELFEAR, HCURSE_I, MANATHRUST_I, GLOBELIGHT_I, DETECTMONSTERS, HDETECTEVIL, HSANCTUARY_I, SENSEHIDDEN_I }
end
