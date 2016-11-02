-- Similar to Nature's 'Grow Trees'
-- radius is {1 .. 10}, cost {5 .. 40}
NATURESCALL = add_spell {
	["name"] = 	"Nature's Call",
	["school"] = 	{SCHOOL_DRUID_ARCANE},
	["spell_power"] = 0,
	["level"] = 	15,
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	0,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			grow_trees(Ind, 1 + get_level(Ind, NATURESCALL, 8))
			end,
	["info"] = 	function()
			return "rad " .. (get_level(Ind, NATURESCALL, 8) + 1)
			end,
	["desc"] = 	{ "Boosts the growth of the saplings around you,",
			  "surrounding you with green-goodness!", }
}

-- Finally, a second nox? :-)
-- This one is water/poison (_not_ unbreath) which will eventually turn into ice(water/shards)/poison (not unbreath)
WATERPOISON_I = add_spell {
	["name"] = 	"Toxic Moisture I",
	["school"] = 	{SCHOOL_DRUID_ARCANE},
	["spell_power"] = 0,
	["level"] = 	3,
	["mana"] = 	5,
	["mana_max"] = 	5,
	["fail"] = 	20,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			fire_cloud(Ind, GF_POIS, args.dir, (2 + get_level(Ind, WATERPOISON_I, 60)), 3, (5 + get_level(Ind, WATERPOISON_I, 14)), 9, " fires a toxic moisture of")
			end,
	["info"] = 	function()
			return "dam " .. (2 + get_level(Ind, WATERPOISON_I, 60)) .. " rad 3 dur " .. (5 + get_level(Ind, WATERPOISON_I, 14))
			end,
	["desc"] = 	{ "Creates a cloud of toxic moisture.", }
}
WATERPOISON_II = add_spell {
	["name"] = 	"Toxic Moisture II",
	["school"] = 	{SCHOOL_DRUID_ARCANE},
	["spell_power"] = 0,
	["level"] = 	20,
	["mana"] = 	20,
	["mana_max"] = 	20,
	["fail"] = 	-40,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			fire_cloud(Ind, GF_WATERPOISON, args.dir, (2 + 60 + get_level(Ind, WATERPOISON_II, 60)), 3, (5 + get_level(Ind, WATERPOISON_I, 14)), 9, " fires toxic vapour of")
			end,
	["info"] = 	function()
			return "dam " .. (2 + 60 + get_level(Ind, WATERPOISON_II, 60)) .. " rad 3 dur " .. (5 + get_level(Ind, WATERPOISON_I, 14))
			end,
	["desc"] = 	{ "Creates a mixed cloud of toxic moisture and hot vapour.", }
}
WATERPOISON_III = add_spell {
	["name"] = 	"Toxic Moisture III",
	["school"] = 	{SCHOOL_DRUID_ARCANE},
	["spell_power"] = 0,
	["level"] = 	33,
	["mana"] = 	40,
	["mana_max"] = 	40,
	["fail"] = 	-70,
	["stat"] = 	A_WIS,
	["direction"] = TRUE,
	["spell"] = 	function(args)
			fire_cloud(Ind, GF_ICEPOISON, args.dir, (2 + 106 + get_level(Ind, WATERPOISON_III, 70)), 3, (5 + get_level(Ind, WATERPOISON_I, 14)), 9, " fires icy toxic moisture of")
			end,
	["info"] = 	function()
			return "dam " .. (2 + 106 + get_level(Ind, WATERPOISON_III, 70)) .. " rad 3 dur " .. (5 + get_level(Ind, WATERPOISON_I, 14))
			end,
	["desc"] = 	{ "Creates a mixed cloud of toxic moisture and ice shards.", }
}

-- Do we want druids to have an id spell? I do =) he he. remove if otherwise
BAGIDENTIFY = add_spell {
	["name"] = 	"Ancient Lore",
	["school"] = 	{SCHOOL_DRUID_ARCANE},
	["spell_power"] = 0,
	["level"] = 	20,
	["mana"] = 	30,
	["mana_max"] = 	30,
	["fail"] = 	10,
	["stat"] = 	A_WIS,
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
REPLACEWALL = add_spell {
	["name"] = 	"Garden of the Gods",
	["school"] = 	{SCHOOL_DRUID_ARCANE},
	["spell_power"] = 0,
	["level"] = 	35,
	["mana"] = 	150,
	["mana_max"] = 	150,
	["fail"] = 	102,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			create_garden(Ind, get_level(Ind, REPLACEWALL, 50))
			end,
	["info"] = 	function()
			return "chance per wall: "..get_level(Ind, REPLACEWALL, 50).."%"
			end,
	["desc"] = 	{ "Transforms walls into trees.",}
}

-- _The_Master_ Druid Spell!
-- Let it be known that no animals shall harm a druid is he wishes it so!
-- 'banishes' animals on level. (only their level can save them!)
-- ((druid_lvl*2 - monster_lvl) > randint(100)) ? banished! : aggravated!
-- We're talking about aggravating a pack of lvl 80+ Aether Zs here. :)
-- Muahaha
BANISHANIMALS = add_spell {
	["name"] = 	"Call of the Forest",
	["school"] = 	{SCHOOL_DRUID_ARCANE},
	["spell_power"] = 0,
	["level"] = 	40,
	["mana"] = 	250,
	["mana_max"] = 	250,
	["fail"] = 	102,
	["stat"] = 	A_WIS,
	["direction"] = FALSE,
	["spell"] = 	function()
			do_banish_animals(Ind, get_level(Ind, BANISHANIMALS, 100))
			end,
	["info"] = 	function()
			return "chance: (" .. get_level(Ind, BANISHANIMALS, 100).."-mlvl)%"
			end,
	["desc"] = 	{ "Banishes animals to where they belong!",}
}
