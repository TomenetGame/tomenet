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
	["name"] = "Earh",
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

-- Create the crystal of mana
school_book[0] = {
	MANATHRUST, DELCURSES, RESISTS, MANASHIELD,
}

-- The book of the eternal flame
school_book[1] = {
	GLOBELIGHT, FIREFLASH, FIREWALL,
}

-- The book of the blowing winds
school_book[2] = {
        NOXIOUSCLOUD, POISONBLOOD, INVISIBILITY, AIRWINGS, THUNDERSTORM,
}

-- The book of the impenetrable earth
school_book[3] = {
        STONESKIN, DIG, STONEPRISON, SHAKE, STRIKE,
}

-- The book of the everrunning wave
school_book[4] = {
        ENTPOTION, TIDALWAVE, ICESTORM,
}

-- Create the book of translocation
school_book[5] = {
        DISARM, BLINK, TELEPORT, TELEAWAY, RECALL,
}

-- Create the book of the tree
school_book[6] = {
        GROWTREE, HEALING, RECOVERY, REGENERATION,
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
        RECHARGE, DISPERSEMAGIC,
}

-- Create the book of beginner's cantrip
school_book[50] = {
        MANATHRUST, GLOBELIGHT, ENTPOTION, BLINK, SENSEMONSTERS, SENSEHIDDEN,
}



--[[

-- Create the book of the mind
school_book[10] = {
        CHARM, CONFUSE, ARMOROFFEAR, STUN,
}

-- Create the book of hellflame
school_book[11] = {
        DRAIN, GENOCIDE, WRAITHFORM, FLAMEOFUDUN,
}

-- Create the book of beginner's cantrip
school_book[50] = {
        MANATHRUST, GLOBELIGHT, ENTPOTION, BLINK, SENSEMONSTER, SENSEHIDDEN,
}
]]
