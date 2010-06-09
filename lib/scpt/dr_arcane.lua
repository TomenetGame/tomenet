-- Similar to Nature's 'Grow Trees'
-- radius is {1 .. 10}, cost {5 .. 45}
NATURESCALL = add_spell
{
	["name"] = 	"Nature's Call",
	["school"] = 	{SCHOOL_DRUID_ARCANE},
	["spell_power"] = 0,
	["level"] = 	10,
	["mana"] = 	5,
	["mana_max"] = 	40,
	["fail"] = 	20,
        ["stat"] =      A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			grow_trees(Ind, 1 + get_level(Ind, NATURESCALL, 8))
			end,
	["info"] = 	function()
			return "rad " .. (get_level(Ind, NATURESCALL, 8) + 1)
			end,
	["desc"] = 	{ "Boosts the growth of the saplings around you.",
			  "Surrounding you with green-goodness!",}
}

-- Finally, a second nox? :-)
-- This one is water/poison (_not_ unbreath) which will eventually turn into ice(water/shards)/poison (not unbreath)
WATERPOISON = add_spell
{
	["name"] = 	"Toxic Moisture",
	["school"] = 	{SCHOOL_DRUID_ARCANE},
	["spell_power"] = 0,
	["level"] = 	3,
	["mana"] = 	1,
	["mana_max"] = 	40,
	["fail"] = 	20,
        ["stat"] =      A_WIS,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			local type
			type = GF_POIS
			if get_level(Ind, WATERPOISON, 50) >= 30 then
				type = GF_ICEPOISON
			elseif get_level(Ind, WATERPOISON, 50) >= 20 then
				type = GF_WATERPOISON
			end
--			fire_cloud(Ind, type, args.dir, (2 + get_level(Ind, WATERPOISON, 100)), 3, (5 + get_level(Ind, WATERPOISON, 40)), 10, " fires a toxic moisture of")
--1.5			fire_cloud(Ind, type, args.dir, (2 + get_level(Ind, WATERPOISON, 136)), 3, (5 + get_level(Ind, WATERPOISON, 14)), 9, " fires a toxic moisture of")
			fire_cloud(Ind, type, args.dir, (2 + get_level(Ind, WATERPOISON, 182)), 3, (5 + get_level(Ind, WATERPOISON, 14)), 9, " fires a toxic moisture of")
			end,
	["info"] = 	function()
--			return "dam " .. (2 + get_level(Ind, WATERPOISON, 136)) .. " rad 3 dur " .. (5 + get_level(Ind, WATERPOISON, 14))
			return "dam " .. (2 + get_level(Ind, WATERPOISON, 182)) .. " rad 3 dur " .. (5 + get_level(Ind, WATERPOISON, 14))
			end,
	["desc"] = 	{ "Creates a cloud of toxic moisture.",
			  "At level 20 it combines the surrounding air into the attack.",
			  "At level 30 it condenses the water into ice.",}
}

-- Do we want druids to have an id spell? I do =) he he. remove if otherwise
BAGIDENTIFY = add_spell
{
	["name"] = 	"Ancient Lore",
	["school"] = 	{SCHOOL_DRUID_ARCANE},
	["spell_power"] = 0,
	["level"] = 	20,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	20,
        ["stat"] =      A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			identify_pack(Ind);
			end,
	["info"] = 	function()
			return ""
			end,
	["desc"] = 	{ "Identifies your backpack.",}
}

-- The best spell yet! 
-- For each wall on level: (spell_lvl)% chance for it to get turned to a tree!
REPLACEWALL = add_spell
{
	["name"] = 	"Garden of the Gods",
	["school"] = 	{SCHOOL_DRUID_ARCANE},
	["spell_power"] = 0,
	["level"] = 	1,
	["mana"] = 	150,
	["mana_max"] = 	250,
	["fail"] = 	40,
        ["stat"] =      A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			create_garden(Ind, get_level(Ind, REPLACEWALL, 50))
			end,
	["info"] = 	function()
			return "chance per wall: "..get_level(Ind, REPLACEWALL, 50).."%"
			end,
	["desc"] = 	{ "Transform walls into trees.",}
}

-- _The_Master_ Druid Spell!
-- Let it be known that no animals shall harm a druid is he wishes it so!
-- 'banishes' animals on level. (only their level can save them!)
-- ((druid_lvl*2 - monster_lvl) > randint(100)) ? banished! : aggravated!
-- We're talking about aggravating a pack of lvl 80+ Aether Zs here. :)
-- Muahaha
BANISHANIMALS = add_spell
{
	["name"] = 	"Call of the Forest",
	["school"] = 	{SCHOOL_DRUID_ARCANE},
	["spell_power"] = 0,
	["level"] = 	1,
	["mana"] = 	250,
	["mana_max"] = 	300,
	["fail"] = 	50,
        ["stat"] =      A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			do_banish_animals(Ind, get_level(Ind, BANISHANIMALS, 100))
			end,
	["info"] = 	function()
			return "chance: (" .. get_level(Ind, BANISHANIMALS, 100).."-mlvl)%"
			end,
	["desc"] = 	{ "Bashish animals to where they belong!",}
}
