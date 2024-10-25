--[[ 'guide.lua'
 * Purpose: Provide meta information for in-client guide searching for key terms. - C. Blue
]]

guide_races = 18
guide_race = {
    "Human",
    "Half-Elf",
    "Elf",
    "Hobbit",
    "Gnome",
    "Dwarf",
    "Half-Orc",
    "Half-Troll",
    "Dunadan",
    "High-Elf",
    "Yeek",
    "Goblin",
    "Ent",
    "Draconian",
    "Kobold",
    "Dark-Elf",
    "Vampire",
    "Maia",
}

guide_classes = 15
guide_class = {
    "Warrior",
    "Istar",
    "Priest",
    "Rogue",
    "Mimic",
    "Archer",
    "Paladin",
    "Death Knight",
    "Hell Knight",
    "Ranger",
    "Adventurer",
    "Druid",
    "Shaman",
    "Runemaster",
    "Mindcrafter",
}

--don't count magic schools as skills for searching purpose
--guide_skills = 71
guide_skills = 74 - 1 - 11 - 15 - 6 - 1
guide_skill = {
    "+Combat",
    "+Weaponmastery",
    "+Sword-mastery",
    ".Critical-strike",
    ".Axe-mastery",
    ".Blunt-mastery",
    ".Polearm-mastery",
    ".Combat Stances",
    ".Dual-wield",
    ".Martial Arts",
    ".Interception",
    "+Archery",
    ".Sling-mastery",
    ".Bow-mastery",
    ".Crossbow-mastery",
    ".Boomerang-mastery",
    "+Magic",
--[[
    "+Wizardry",
]]--
    ".Sorcery",
--[[
    ".Mana",
    ".Fire",
    ".Water",
    ".Air",
    ".Earth",
    ".Nature",
    ".Conveyance",
    ".Divination",
    ".Temporal",
    ".Udun",
    ".Mind",
]]--
    ".Spell-power",
--[[
    "+Prayers",
    ".Holy Offense",
    ".Holy Defense",
    ".Holy Curing",
    ".Holy Support",
    "+Occultism",
    ".Shadow",
    ".Spirit",
    ".Hereticism",
    "+Druidism",
    ".Arcane Lore",
    ".Physical Lore",
    "+Mindcraft",
    ".Psycho-Power",
    ".Attunement",
    ".Mental Intrusion",
]]--
    "+Runecraft",
--[[
    ".Light",
    ".Dark",
    ".Nexus",
    ".Nether",
    ".Chaos",
    ".Mana ",

    ".Astral Knowledge",
]]--
    "+Blood Magic",
    ".Necromancy",
    ".Traumaturgy",
    ".Aura of Fear",
    ".Shivering Aura",
    ".Aura of Death",
    ".Mimicry",
    ".Magic Device",
    ".Anti-magic",
    "+Sneakiness",
    ".Stealth",
    ".Stealing",
    ".Backstabbing",
    ".Dodging",
    ".Calmness",
    ".Trapping",
    "+Health",
    ".Swimming",
    ".Climbing",
    ".Digging",
}

--(note: Sorcery isn't a school)
guide_schools = 29
guide_school = {
    "Wizardry -",
    "Mana",
    "Fire",
    "Water",
    "Air",
    "Earth",
    "Nature",
    "Conveyance",
    "Divination",
    "Temporal",
    "Udun",
    "Prayers -",
    "Holy Offense",
    "Holy Defense",
    "Holy Curing",
    "Holy Support",
    "Occultism -",
    "Shadow",
    "Spirit",
    "Hereticism",
    "Druidism -" ,
    "Arcane Lore",
    "Physical Lore",
    "Mindcraft -",
    "Psycho-power",
    "Attunement",
    "Mental Intrusion",
    "Astral Knowledge -",
    "Unlife",
}

-- note: indices start at 0, so number is final index + 1
guide_spells = 194
guide_spell = {
    "Manathrust",
    "Recharge",
    "Disperse Magic",
    "Remove Curses",
    "Disruption Shield",

    "Globe of Light",
    "Fire Bolt",
    "Elemental Shield",
    "Fiery Shield",
    "Firewall",
    "Fire Ball",
    "Fireflash", --12

    "Vapor",
    "Ent's Potion",
    "Frost Bolt",
    "Water Bolt",
    "Tidal Wave",
    "Frost Barrier",
    "Frost Ball",

    "Lightning Bolt",
    "Noxious Cloud",
    "Thunderstorm",
    "Wings of Winds",
    "Invisibility", --24
    "Mirage Mirror",

    "Dig",
    "Acid Bolt",
    "Stone Prison",
    "Strike",
    "Shake",

    "Healing",
    "Grasping Vines",
    "Vermin Control",
    "Recovery",
    --"Thunderstorm", --duplicate (Air)
    "Regeneration",
    "Grow Trees",
    "Poison Blood", --37

    "Phase Door",
    "Disarm",
    "Teleport",
    "Teleport Away",
    "Recall",
    "Probability Travel",
    "Mass Warp",
    "Telekinesis I",

    "Detect Monsters",
    "Identify",
    "Greater Identify",
    "Vision",
    "Sense Hidden",
    "Reveal Ways",
    "Sense Minds", --52

    "Magelock",
    "Slow Monster",
    "Essence of Speed",
    "Mass Warp",

    "Confusion",
    "Stun",
    "Genocide",
    "Obliteration",
    "Wraithform",
    "Stop Wraithform",
    "Disenchantment Beam",
    "Hellfire", --64

    "Curse",
    "Call Light",
    "Ray of Light",
    "Exorcism",
    "Redemption",
    "Orb of Draining",
    "Doomed Grounds",
    "Earthquake", --71

    "Blessing",
    "Holy Resistance",
    "Protection from evil",
    "Dispel Magic",
    "Glyph of Warding",
    "Martyrdom",

    "Cure Wounds",
    "Heal",
    "Break Curses",
    "Cleansing Light",
    "Curing",
    "Restoration",
    "Faithful Focus",
    "Resurrection",
    "Soul Curing", --86

    "Remove Fear",
    "Call Light",
    "Detect Evil",
    "Sense Monsters",
    "Sanctuary",
    "Satisfy Hunger",
    "Sense Surroundings",
    "Zeal", --94

    "Cause Fear",
    "Blindness",
    "Detect Invisible",
    "Veil of Night",
    "Shadow Bolt",
    "Aspect of Peril",
    "Darkness",
    "Shadow Gate",
    "Shadow Shroud",
    "Chaos Bolt",
    "Drain Life",
    "Darkness Storm", --106

    "Cause Wounds",
    "Tame Fear",
    "Starlight",
    "Meditation",
    "Trance",
    "Lightning",
    "Spear of Light",
    "Lift Curses",
    "Ethereal Eye",
    "Possess",
    "Stop Possess",
    "Guardian Spirit", --118
    "Purification Rites",

    "Terror",
    "Ignore Fear",
    "Fire Bolt",
    "Wrathflame",
    "Flame Wave",
    "Demonic Strength",
    --"Chaos Bolt", --duplicate (Shadow)
    "Boundless Rage",
    "Wicked Oath",
    "Levitation",
    "Robes of Havoc",
    "Blood Sacrifice", --130

    "Nature's Call",
    "Toxic Moisture",
    "Ancient Lore",
    "Garden of the Gods",
    "Call of the Forest",

    "Forest's Embrace",
    "Quickfeet",
    "Herbal Tea",
    "Extra Growth",
    "Focus", --140

    "Psychic Hammer",
    "Psychokinesis",
    "Autokinesis I",
    "Autokinesis II",
    "Autokinesis III",
    "Feedback",
    "Pyrokinesis",
    "Cryokinesis",
    "Psychic Warp",
    "Telekinesis II",
    "Kinetic Shield",

    "Clear Mind",
    "Willpower",
    "Self-Reflection",
    "Accelerate Nerves",
    "Telepathy",
    "Recognition",
    "Stabilize Thoughts", --158

    "Psionic Blast",
    "Psi Storm",
    "Scare",
    "Confuse",
    "Hypnosis",
    "Apathy",
    "Psychic Suppression",
    "Remote Vision",
    "Recognition",
    "Charm",
    "Stop Charm",

    "Power Bolt",
    "Power Ray",
    "Power Blast",
    "Relocation",
    "Vengeance",
    "Empowerment",
    "The Silent Force",
    "Sphere of Destruction",
    "Gateway", --178

    "Fatigue",
    "Detect Lifeforce",
    "Tainted Grounds",
    "Nether Sap",
    "Subjugation",
    "Nether Bolt",
    "Permeation",
    "Siphon Life",
    "Touch of Hunger",
    "Wraithstep", --188
    "Mists of Decay",

    --additional Shadow spells
    "Retreat",
    "Shadow Stream",
    "Dispersion",
    "Stop Dispersion", --193
}
