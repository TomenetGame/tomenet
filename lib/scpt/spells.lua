-- $Id$
-- This file takes care of the schools of magic
-- (Edit this file and funny funny things will happen :)

-- Create the schools
SCHOOL_CONVEYANCE = add_school
{
	["name"] = "Conveyance", 
        ["skill"] = SKILL_CONVEYANCE,
        ["sorcery"] = TRUE,
}

SCHOOL_MANA = add_school
{
	["name"] = "Mana", 
        ["skill"] = SKILL_MANA,
        ["sorcery"] = TRUE,
}

-- Put some spells
pern_dofile("s_convey.lua")

-- Create the book of beginner's cantrip
-- Great book, eh? ;)
school_book[50] = {
--        MANATHRUST, GLOBELIGHT, ENTPOTION, BLINK, SENSEMONSTER, SENSEHIDDEN,
        BLINK,
}




--[[
-- Create the schools
SCHOOL_MANA = add_school
{
	["name"] = "Mana", 
        ["skill"] = SKILL_MANA,
        ["sorcery"] = TRUE,
        ["gods"] =
        {
                -- Eru Iluvatar provides the Mana school at half the prayer skill
                [GOD_ERU] =
                {
                        ["skill"] = SKILL_PRAY,
                        ["mul"] = 1,
                        ["div"] = 2,
                },
        },
}
SCHOOL_FIRE = add_school
{
	["name"] = "Fire", 
        ["skill"] = SKILL_FIRE,
        ["sorcery"] = TRUE,
}
SCHOOL_AIR = add_school
{
	["name"] = "Air", 
        ["skill"] = SKILL_AIR,
        ["sorcery"] = TRUE,
        ["gods"] =
        {
                -- Manwe Sulimo provides the Air school at 2/3 the prayer skill
                [GOD_MANWE] =
                {
                        ["skill"] = SKILL_PRAY,
                        ["mul"] = 2,
                        ["div"] = 3,
                },
        },
}
SCHOOL_WATER = add_school
{
	["name"] = "Water", 
        ["skill"] = SKILL_WATER,
        ["sorcery"] = TRUE,
}
SCHOOL_EARTH = add_school
{
	["name"] = "Earth", 
        ["skill"] = SKILL_EARTH,
        ["sorcery"] = TRUE,
        ["gods"] =
        {
                -- Tulkas provides the Earth school at 4/5 the prayer skill
                [GOD_TULKAS] =
                {
                        ["skill"] = SKILL_PRAY,
                        ["mul"] = 4,
                        ["div"] = 5,
                },
        },
}
SCHOOL_CONVEYANCE = add_school
{
	["name"] = "Conveyance", 
        ["skill"] = SKILL_CONVEYANCE,
        ["sorcery"] = TRUE,
        ["gods"] =
        {
                -- Manwe Sulimo provides the Conveyance school at 1/2 the prayer skill
                [GOD_MANWE] =
                {
                        ["skill"] = SKILL_PRAY,
                        ["mul"] = 1,
                        ["div"] = 2,
                },
        },
}
SCHOOL_DIVINATION = add_school
{
	["name"] = "Divination", 
        ["skill"] = SKILL_DIVINATION,
        ["sorcery"] = TRUE,
        ["gods"] =
        {
                -- Eru Iluvatar provides the Divination school at 2/3 the prayer skill
                [GOD_ERU] =
                {
                        ["skill"] = SKILL_PRAY,
                        ["mul"] = 2,
                        ["div"] = 3,
                },
        },
}
SCHOOL_TEMPORAL = add_school
{
	["name"] = "Temporal", 
        ["skill"] = SKILL_TEMPORAL,
        ["sorcery"] = TRUE,
}
SCHOOL_NATURE = add_school
{
	["name"] = "Nature", 
        ["skill"] = SKILL_NATURE,
        ["sorcery"] = TRUE,
}
SCHOOL_META = add_school
{
	["name"] = "Meta", 
        ["skill"] = SKILL_META,
        ["sorcery"] = TRUE,
        ["gods"] =
        {
                -- Manwe Sulimo provides the Meta school at 1/3 the prayer skill
                [GOD_MANWE] =
                {
                        ["skill"] = SKILL_PRAY,
                        ["mul"] = 1,
                        ["div"] = 3,
                },
        },
}
SCHOOL_MIND = add_school
{
	["name"] = "Mind",
        ["skill"] = SKILL_MIND,
        ["sorcery"] = TRUE,
        ["gods"] =
        {
                -- Eru Iluvatar provides the Mind school at 1/3 the prayer skill
                [GOD_ERU] =
                {
                        ["skill"] = SKILL_PRAY,
                        ["mul"] = 1,
                        ["div"] = 3,
                },
                -- Melkor Bauglir provides the Mind school at full prayer skill
                [GOD_MELKOR] =
                {
                        ["skill"] = SKILL_PRAY,
                        ["mul"] = 1,
                        ["div"] = 3,
                },
        },
}
SCHOOL_UDUN = add_school
{
	["name"] = 		"Udun",
        ["skill"] = 		SKILL_UDUN,
        ["bonus_level"] = 	function()
        				return ((player.lev * 2) / 3)
        			end,
}

-- The God specific schools, all tied to the prayer skill
SCHOOL_ERU = add_school
{
	["name"] = "Eru Iluvatar",
        ["skill"] = SKILL_PRAY,
        ["god"] = GOD_ERU,
}
SCHOOL_MANWE = add_school
{
        ["name"] = "Manwe Sulimo",
        ["skill"] = SKILL_PRAY,
        ["god"] = GOD_MANWE,
}
SCHOOL_TULKAS = add_school
{
        ["name"] = "Tulkas",
        ["skill"] = SKILL_PRAY,
        ["god"] = GOD_TULKAS,
}
SCHOOL_MELKOR = add_school
{
        ["name"] = "Melkor Bauglir",
        ["skill"] = SKILL_PRAY,
        ["god"] = GOD_MELKOR,
}

-- Put some spells
tome_dofile("s_fire.lua")
tome_dofile("s_mana.lua")
tome_dofile("s_water.lua")
tome_dofile("s_air.lua")
tome_dofile("s_earth.lua")
tome_dofile("s_convey.lua")
tome_dofile("s_nature.lua")
tome_dofile("s_divin.lua")
tome_dofile("s_tempo.lua")
tome_dofile("s_meta.lua")
tome_dofile("s_mind.lua")
tome_dofile("s_udun.lua")

-- God's specific spells
tome_dofile("s_eru.lua")
tome_dofile("s_manwe.lua")
tome_dofile("s_tulkas.lua")
tome_dofile("s_melkor.lua")

-- Create the crystal of mana
school_book[0] = {
	MANATHRUST, DELCURSES, RESISTS, MANASHIELD,
}

-- The book of the eternal flame
school_book[1] = {
	GLOBELIGHT, FIREGOLEM, FIREFLASH, FIREWALL, FIERYAURA,
}

-- The book of the blowing winds
school_book[2] = {
        NOXIOUSCLOUD, POISONBLOOD, INVISIBILITY, AIRWINGS, THUNDERSTORM,
}

-- The book of the impenetrable earth
school_book[3] = {
        STONESKIN, DIG, STONEPRISON, SHAKE, STRIKE,
}

-- The book of the unstopable wave
school_book[4] = {
        ENTPOTION, TIDALWAVE, ICESTORM,
}

-- Create the book of translocation
school_book[5] = {
        DISARM, BLINK, TELEPORT, TELEAWAY, RECALL,
}

-- Create the book of the tree
school_book[6] = {
        GROWTREE, HEALING, RECOVERY, REGENERATION, SUMMONANNIMAL,
}

-- Create the book of Knowledge
school_book[7] = {
        SENSEMONSTERS, SENSEHIDDEN, REVEALWAYS, IDENTIFY, VISION, STARIDENTIFY,
}

-- Create the book of the Time
school_book[8] = {
        MAGELOCK, SLOWMONSTER, ESSENSESPEED, BANISHMENT,
}

-- Create the book of meta spells
school_book[9] = {
        RECHARGE, DISPERSEMAGIC, SPELLBINDER,
}

-- Create the book of the mind
school_book[10] = {
        CHARM, CONFUSE, ARMOROFFEAR, STUN,
}

-- Create the book of hellflame
school_book[11] = {
        DRAIN, GENOCIDE, WRAITHFORM, FLAMEOFUDUN,
}

-- Create the book of eru
school_book[20] = {
        ERU_SEE, ERU_LISTEN, ERU_UNDERSTAND, ERU_PROT,
}

-- Create the book of manwe
school_book[21] = {
        MANWE_BLESS, MANWE_SHIELD, MANWE_CALL, MANWE_AVATAR,
}

-- Create the book of tulkas
school_book[22] = {
        TULKAS_AIM, TULKAS_SPIN, TULKAS_WAVE,
}

-- Create the book of melkor
school_book[23] = {
        MELKOR_CURSE, MELKOR_CORPSE_EXPLOSION, MELKOR_MIND_STEAL,
}

-- Create the book of beginner's cantrip
school_book[50] = {
        MANATHRUST, GLOBELIGHT, ENTPOTION, BLINK, SENSEMONSTER, SENSEHIDDEN,
}

-- Create the book of teleporatation
school_book[51] = {
        TELEPORT, TELEAWAY
}

-- Create the book of recall
school_book[52] = {
        RECALL
}

-- Create the book of summoning
school_book[53] = {
        FIREGOLEM, SUMMONANNIMAL
}

-- Create the book of fireflash
school_book[54] = {
        FIREFLASH
}
]]
