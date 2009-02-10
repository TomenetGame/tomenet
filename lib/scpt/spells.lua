-- $Id$
-- This file takes care of the schools of magic
-- (Edit this file and funny funny things will happen :)

-- <- dummy line to change checksum - C. Blue
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
}

SCHOOL_MIND = add_school
{
	["name"] = "Mind",
        ["skill"] = SKILL_MIND,
        ["sorcery"] = TRUE,
}

SCHOOL_DIVINATION = add_school
{
	["name"] = "Divination",
        ["skill"] = SKILL_DIVINATION,
        ["sorcery"] = TRUE,
}

SCHOOL_UDUN = add_school
{
	["name"] = "Udun",
        ["skill"] = SKILL_UDUN,
        ["sorcery"] = TRUE,
}

-- Priests / Paladins

SCHOOL_HOFFENSE = add_school
{
	["name"] = "Holy Offense",
        ["skill"] = SKILL_HOFFENSE,
}
SCHOOL_HDEFENSE = add_school
{
	["name"] = "Holy Defense",
        ["skill"] = SKILL_HDEFENSE,
}
SCHOOL_HCURING = add_school
{
	["name"] = "Holy Curing",
        ["skill"] = SKILL_HCURING,
}
SCHOOL_HSUPPORT = add_school
{
	["name"] = "Holy Support",
        ["skill"] = SKILL_HSUPPORT,
}

-- Druids
SCHOOL_DRUID_ARCANE = add_school
{
	["name"] = "Arcane Lore",
	["skill"] = SKILL_DRUID_ARCANE,
}
SCHOOL_DRUID_PHYSICAL = add_school
{
	["name"] = "Physical Lore",
	["skill"] = SKILL_DRUID_PHYSICAL,
}

-- Put some spells
pern_dofile(Ind, "s_mana.lua")
pern_dofile(Ind, "s_fire.lua")
pern_dofile(Ind, "s_air.lua")
pern_dofile(Ind, "s_water.lua")
pern_dofile(Ind, "s_earth.lua")
pern_dofile(Ind, "s_convey.lua")
pern_dofile(Ind, "s_divin.lua")
pern_dofile(Ind, "s_tempo.lua")
pern_dofile(Ind, "s_meta.lua")
pern_dofile(Ind, "s_nature.lua")
pern_dofile(Ind, "s_mind.lua")
pern_dofile(Ind, "s_udun.lua")

pern_dofile(Ind, "p_offense.lua")
pern_dofile(Ind, "p_defense.lua")
pern_dofile(Ind, "p_curing.lua")
pern_dofile(Ind, "p_support.lua")

pern_dofile(Ind, "dr_arcane.lua") 
pern_dofile(Ind, "dr_physical.lua")

-- Create the crystal of mana (1-4)
school_book[0] = {
	MANATHRUST, DELCURSES, RESISTS, MANASHIELD,
}

-- The book of the eternal flame (5-8)
school_book[1] = {
	GLOBELIGHT, FIREFLASH, FIERYAURA, FIREWALL,
}

-- The book of the blowing winds (9-13)
school_book[2] = {
        NOXIOUSCLOUD, POISONBLOOD, INVISIBILITY, AIRWINGS, THUNDERSTORM,
}

-- The book of the impenetrable earth (14-17)
school_book[3] = {
--        STONESKIN, DIG, STONEPRISON, SHAKE, STRIKE,
        DIG, STONEPRISON, SHAKE, STRIKE,
}

-- The book of the everrunning wave (18-21)
school_book[4] = {
        VAPOR, ENTPOTION, TIDALWAVE, ICESTORM,
}

-- Create the book of translocation (22-27)
school_book[5] = {
        DISARM, BLINK, TELEPORT, TELEAWAY, RECALL, PROBABILITY_TRAVEL,
}

-- Create the book of the tree * SUMMONANIMAL requires pets first (28-32)
school_book[6] = {
        GROWTREE, HEALING, RECOVERY, REGENERATION, VERMINCONTROL,
}

-- Create the book of Knowledge (33-38)
school_book[7] = {
        SENSEMONSTERS, SENSEHIDDEN, REVEALWAYS, IDENTIFY, VISION, STARIDENTIFY,
}

-- Create the book of the Time (39-42)
school_book[8] = {
        MAGELOCK, SLOWMONSTER, ESSENSESPEED, BANISHMENT,
}

-- Create the book of meta spells (43-45)
school_book[9] = {
        RECHARGE, PROJECT_SPELLS, DISPERSEMAGIC,
}

-- Create the book of the mind * CHARM requires pets first (46-48)
school_book[10] = {
        CONFUSE, STUN, TELEKINESIS,
}

-- Create the book of hellflame * DRAIN, FLAMEOFUDUN missing (49-53)
school_book[11] = {
        GENOCIDE, WRAITHFORM, DISEBOLT, HELLFIRE, STOPWRAITH,
}

-- Priests / Paladins:

-- Create the book of Holy Offense (54-60)
school_book[12] = {
        HCURSE, HGLOBELIGHT, HORBDRAIN, HDRAINLIFE, HEXORCISM, HRELSOULS, HDRAINCLOUD,
}

-- Create the book of Holy Defense (61-65)
school_book[13] = {
	HBLESSING, HRESISTS, HPROTEVIL, HRUNEPROT, HMARTYR,
}

-- Create the book of Holy Curing (66-72)
school_book[14] = {
	HHEALING, HDELCURSES, HHEALING2, HCURING, HSANITY, HRESURRECT, HDELBB,
}

-- Create the book of Holy Support (73-79)
school_book[15] = {
	HDELFEAR, HGLOBELIGHT, HSENSEMON, HSANCTUARY, HSATISFYHUNGER, HSENSE,  HZEAL,
}

-- Create the book of druidism: Arcane Lore (80-84)
school_book[16] = {
	NATURESCALL, WATERPOISON, BAGIDENTIFY, REPLACEWALL, BANISHANIMALS,
}

-- Create the book of druidism: Physical Lore (85-89)
school_book[17] = {
	HEALINGCLOUD, QUICKFEET, HERBALTEA, EXTRASTATS, FOCUSSHOT,
}

-- Create the book of beginner's cantrip
school_book[50] = {
        MANATHRUST, GLOBELIGHT, ENTPOTION, BLINK, SENSEMONSTERS, SENSEHIDDEN,
}

-- Create the elementalist's handbook
school_book[51] = {
--	NOXIOUSCLOUD, AIRWINGS, THUNDERSTORM, FIREFLASH, FIREWALL, TIDALWAVE, ICESTORM, SHAKE
	NOXIOUSCLOUD, AIRWINGS, THUNDERSTORM, FIREFLASH, FIERYAURA, VAPOR, ICESTORM, SHAKE
}

-- Create the handbook for treasure hunting
school_book[52] = {
	SENSEHIDDEN, REVEALWAYS, DIG, DISARM, IDENTIFY, GLOBELIGHT, DELCURSES
}

-- Create the handbook of piety
school_book[53] = {
	HDELFEAR, HBLESSING, HSATISFYHUNGER, HDELCURSES, HPROTEVIL, HEXORCISM, HRELSOULS
}

-- Create the naturalist's handbook
school_book[54] = {
--	NATURESCALL, REPLACEWALL, HERBALTEA, VAPOR, GROWTREE, RECOVERY, REGENERATION, POISONBLOOD
	NATURESCALL, REPLACEWALL, HERBALTEA, GROWTREE, RECOVERY, REGENERATION, POISONBLOOD
}

---- Create the handbook to the underworld
--school_book[55] = {
--	HELLFIRE, GENOCIDE, BANISHMENT, CONFUSE, FIERYAURA
--}

-- Create the destroyer's handbook
school_book[55] = {
--	ICESTORM, HELLFIRE, FIREFLASH, SHAKE, DISEBOLT, THUNDERSTORM, HORBDRAIN, HDRAINCLOUD
	TIDALWAVE, HELLFIRE, FIREFLASH, STRIKE, DISEBOLT, THUNDERSTORM, HORBDRAIN, HDRAINCLOUD
}

-- (56 is Astral Tome for Divine race)

---- Create the handbook for rogues (of shadows)
school_book[57] = {
	BLINK, DISARM, NOXIOUSCLOUD, SENSEHIDDEN, REVEALWAYS, VISION, INVISIBILITY
}

---- Create the handbook for dungeon masters & wizads (of dungeon keeping)
school_book[58] = {
        TELEKINESIS, DIG, STONEPRISON, GROWTREE, DISARM, VISION, STARIDENTIFY, MANATHRUST, DISEBOLT, FIREFLASH
}
