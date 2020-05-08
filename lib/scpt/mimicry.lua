-- Povide and handle mimic spells (monster form spells) - C. Blue


--Note: So far mimic powers used INT for determining fail rate.
--	["stat"] = 	A_INT,
--Note: So far mimic powers always had a level of 0 and gave 0 exp on first usage.
--	["level"] = 	0,
--Note: So far all mimic powers were set to 100% AM-affected, compare check_antimagic() in do_cmd_mimic()
--	["am"] = 	100,

--misc notes..
--fire_grid_bolt(Ind, GF_, args.dir, 5 + get_level(Ind, MIMICXXX, 100), " ")
--return "power "..(5 + get_level(Ind, MIMICXXX, 100))

--[[
MIMIC_SHRIEK = add_power {
	["name"] = 	"Shriek",
	["mana"] = 	2,
	["fail"] = 	0,
	["stat"] = 	A_INT,
	["direction"] = FALSE,
	["ftk"] =	0,
	["spell"] = 	function(args)
				shriek(Ind);
			end,
	["info"] = 	function()
				return ""
			end,
	["desc"] = 	{ "You emit a shrill noise.", }
}
]]

--[[
Shriek                  Aggravates nearby creatures and annoys nearby players.
Arrow                   Fires an arrow, does not require bow or ammunition.
Shot                    Fires a shot, does not require bow or ammunition.
Bolt                    Fires a bolt, does not require bow or ammunition.
Missile                 Fires a (physical) missile.
Throw Boulder           Hurls a boulder at a target (giants do this usually).
Mana Storm              .. (casts a mana ball)
Darkness Storm          .. (casts a darkness ball)
Mind Blast              Psi attack, similar to Psionic Blast mindcrafter spell.
Brain Smash             Powerful psi attack, similar to Psionic Blast.
Cause Wounds            The curse spell, eg used by priests and archpriests.
Raw Chaos               .. (casts a chaos ball)
Paralyze                Tries to put a monster into stasis (similar to sleep).
Haste Self              Temporarily increases your speed by 10.
Heal                    Heals yourself, amount depends on spell level.
Blink                   Short range teleport, like Phase Door.
Teleport To             Tries to teleport a monster towards you.
Teleport Away           Tries to teleport monsters away (it's a beam!).
Darkness                Unlights the area around you.
Cause Amnesia           Confuses a target.
]]


--monster_spell_type [32]
monster_spells4_lua = {
  {"Shriek", 0},
  {"Negate magic", 0},
  {"XXX", 1},
  {"Fire Rocket", 1},

  {"Arrow", 1},
  {"Shot", 1},
  {"Bolt", 1},
  {"Missile", 1},

  {"Breathe Acid", 1},
  {"Breathe Lightning", 1},
  {"Breathe Fire", 1},
  {"Breathe Cold", 1},

  {"Breathe Poison", 1},
  {"Breathe Nether", 1},
  {"Breathe Lite", 1},
  {"Breathe Darkness", 1},

  {"Breathe Confusion", 1},
  {"Breathe Sound", 1},
  {"Breathe Chaos", 1},
  {"Breathe Disenchantment", 1},

  {"Breathe Nexus", 1},
  {"Breathe Time", 1},
  {"Breathe Inertia", 1},
  {"Breathe Gravity", 1},

  {"Breathe Shards", 1},
  {"Breathe Plasma", 1},
  {"Breathe Force", 1},
  {"Breathe Mana", 1},

  {"Breathe Disintegration", 1},
  {"Breathe Toxic Waste", 1},
  {"Ghastly Moan", 0},
  {"Throw Boulder", 1},
}

--monster_spell_type [32]
monster_spells5_lua = {
  {"Acid Ball", 1},
  {"Lightning Ball", 1},
  {"Fire Ball", 1},
  {"Cold Ball", 1},

  {"Poison Ball", 1},
  {"Nether Ball", 1},
  {"Water Ball", 1},
  {"Mana Storm", 1},

  {"Darkness Storm", 1},
  {"Drain Mana", 1},
  {"Mind Blast", 1},
  {"Brain Smash", 1},

  {"Cause Wounds", 1},
  {"XXX", 1},
  {"Toxic Waste Ball", 1},
  {"Raw Chaos", 1},

  {"Acid Bolt", 1},
  {"Lightning Bolt", 1},
  {"Fire Bolt", 1},
  {"Cold Bolt", 1},

  {"Poison Bolt", 1},
  {"Nether Bolt", 1},
  {"Water Bolt", 1},
  {"Mana Bolt", 1},

  {"Plasma Bolt", 1},
  {"Ice Bolt", 1},
  {"Magic Missile", 1},
  {"Scare", 1},

  {"Blind", 1},
  {"Confusion", 1},
  {"Slow", 1},
  {"Paralyze", 1},
}

--monster_spell_type [32]
monster_spells6_lua = {
  {"Haste Self", 0},
  {"Hand of Doom", 1},
  {"Heal", 0},
  {"XXX", 1},

  {"Blink", 0},
  {"Teleport", 0},
  {"XXX", 1},
  {"XXX", 1},

  {"Teleport To", 1},
  {"Teleport Away", 1},
  {"Teleport Level", 0},
  {"XXX", 1},

  {"Darkness", 0},
  {"Trap Creation", 0},
  {"Cause Amnesia", 1},
  {"XXX", 1},

  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},

  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},

  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},

  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},
}

--monster_spell_type [32]
monster_spells0_lua = {
  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},

  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},
  {"Disenchantment Bolt", 1},

  {"Disenchantment Ball", 1},
  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},

  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},

  {"XXX", 1},
  {"XXX", 1},
  {"Breathe Ice", 1},
  {"Breathe Water", 1},

  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},

  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},

  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},
  {"XXX", 1},
}


--/* 0(lev), mana, fail, 0(exp), ftk */
--magic_type [128]
innate_powers_lua = {
--// RF4_SHRIEK			0x00000001	/* Shriek for help */
	{0, 2, 0, 0, 0},
--// RF4_UNMAGIC		0x00000002	/* (?) */
	{0, 0, 0, 0, 0},
--// (S_ANIMAL) RF4_XXX3	0x00000004	/* (?) */
	{0, 0, 0, 0, 0},
--// RF4_ROCKET			0x00000008	/* (?) */
	{0, 60, 70, 0, 2},
--// RF4_ARROW_1		0x00000010	/* Fire an arrow (light) */
	{0, 2, 5, 0, 1},
--// RF4_ARROW_2		0x00000020	/* Fire an shot (heavy) */
	{0, 2, 6, 0, 1},
--// XXX (RF4_ARROW_3)		0x00000040	/* Fire bolt (heavy) */
	{0, 2, 7, 0, 1},
--// XXX (RF4_ARROW_4)		0x00000080	/* Fire missiles (heavy) */
	{0, 3, 9, 0, 1},
--// RF4_BR_ACID		0x00000100	/* Breathe Acid */
	{0, 10, 20, 0, 2},
--// RF4_BR_ELEC		0x00000200	/* Breathe Elec */
	{0, 10, 20, 0, 2},
--// RF4_BR_FIRE		0x00000400	/* Breathe Fire */
	{0, 10, 20, 0, 2},
--// RF4_BR_COLD		0x00000800	/* Breathe Cold */
	{0, 10, 20, 0, 2},
--// RF4_BR_POIS		0x00001000	/* Breathe Poison */
	{0, 13, 25, 0, 2},
--// RF4_BR_NETH		0x00002000	/* Breathe Nether */
	{0, 22, 35, 0, 2},
--// RF4_BR_LITE		0x00004000	/* Breathe Lite */
	{0, 10, 20, 0, 2},
--// RF4_BR_DARK		0x00008000	/* Breathe Dark */
	{0, 10, 20, 0, 2},
--// RF4_BR_CONF		0x00010000	/* Breathe Confusion */
	{0, 10, 25, 0, 2},
--// RF4_BR_SOUN		0x00020000	/* Breathe Sound */
	{0, 14, 30, 0, 2},
--// RF4_BR_CHAO		0x00040000	/* Breathe Chaos */
	{0, 25, 40, 0, 2},
--// RF4_BR_DISE		0x00080000	/* Breathe Disenchant */
	{0, 28, 45, 0, 2},
--// RF4_BR_NEXU		0x00100000	/* Breathe Nexus */
	{0, 20, 45, 0, 2},
--// RF4_BR_TIME		0x00200000	/* Breathe Time */
	{0, 30, 50, 0, 2},
--// RF4_BR_INER		0x00400000	/* Breathe Inertia */
	{0, 25, 35, 0, 2},
--// RF4_BR_GRAV		0x00800000	/* Breathe Gravity */
	{0, 25, 35, 0, 2},
--// RF4_BR_SHAR		0x01000000	/* Breathe Shards */
	{0, 15, 25, 0, 2},
--// RF4_BR_PLAS		0x02000000	/* Breathe Plasma */
	{0, 25, 30, 0, 2},
--// RF4_BR_WALL		0x04000000	/* Breathe Force */
	{0, 30, 40, 0, 2},
--// RF4_BR_MANA		0x08000000	/* Breathe Mana */
	{0, 35, 45, 0, 2},
--// RF4_BR_DISI		0x10000000
	{0, 50, 70, 0, 2},
--// RF4_BR_NUKE		0x20000000
	{0, 27, 40, 0, 2},
--// 0x40000000
	{0, 0, 0, 0, 0},
--// RF4_BOULDER
	{0, 2, 15, 0, 1},


--// RF5_BA_ACID		0x00000001	/* Acid Ball */
	{0, 8, 10, 0, 2},
--// RF5_BA_ELEC		0x00000002	/* Elec Ball */
	{0, 8, 10, 0, 2},
--// RF5_BA_FIRE		0x00000004	/* Fire Ball */
	{0, 8, 10, 0, 2},
--// RF5_BA_COLD		0x00000008	/* Cold Ball */
	{0, 8, 10, 0, 2},
--// RF5_BA_POIS		0x00000010	/* Poison Ball */
	{0, 11, 20, 0, 2},
--// RF5_BA_NETH		0x00000020	/* Nether Ball */
	{0, 25, 40, 0, 2},
--// RF5_BA_WATE		0x00000040	/* Water Ball */
	{0, 17, 30, 0, 2},
--// RF5_BA_MANA		0x00000080	/* Mana Storm */
	{0, 45, 50, 0, 2},
--// RF5_BA_DARK		0x00000100	/* Darkness Storm */
	{0, 20, 0, 0, 2},
--// RF5_DRAIN_MANA		0x00000200	/* Drain Mana */
	{0, 0, 0, 0, 0},
--// RF5_MIND_BLAST		0x00000400	/* Blast Mind */
	{0, 15, 13, 0, 2},
--// RF5_BRAIN_SMASH		0x00000800	/* Smash Brain */
	{0, 25, 15, 0, 2},
--// RF5_CAUSE_1		0x00001000	/* Cause Light Wound */
	{0, 3, 15, 0, 1},
--// RF5_CAUSE_2		0x00002000	/* Cause Serious Wound */
	{0, 6, 20, 0, 1},
--// RF5_BA_NUKE		0x00004000	/* Toxic Ball */
	{0, 30, 30, 0, 2},
--// RF5_BA_CHAO		0x00008000	/* Chaos Ball */
	{0, 45, 40, 0, 2},
--// RF5_BO_ACID		0x00010000	/* Acid Bolt */
	{0, 4, 13, 0, 1},
--// RF5_BO_ELEC		0x00020000	/* Elec Bolt (unused) */
	{0, 3, 13, 0, 1},
--// RF5_BO_FIRE		0x00040000	/* Fire Bolt */
	{0, 4, 13, 0, 1},
--// RF5_BO_COLD		0x00080000	/* Cold Bolt */
	{0, 3, 13, 0, 1},
--// RF5_BO_POIS		0x00100000	/* Poison Bolt (unused) */
	{0, 5, 16, 0, 1},
--// RF5_BO_NETH		0x00200000	/* Nether Bolt */
	{0, 15, 20, 0, 1},
--// RF5_BO_WATE		0x00400000	/* Water Bolt */
	{0, 13, 18, 0, 1},
--// RF5_BO_MANA		0x00800000	/* Mana Bolt */
	{0, 20, 25, 0, 1},
--// RF5_BO_PLAS		0x01000000	/* Plasma Bolt */
	{0, 15, 18, 0, 1},
--// RF5_BO_ICEE		0x02000000	/* Ice Bolt */
	{0, 10, 15, 0, 1},
--// RF5_MISSILE		0x04000000	/* Magic Missile */
	{0, 2, 5, 0, 1},
--// RF5_SCARE			0x08000000	/* Frighten Player */
	{0, 3, 8, 0, 0},
--// RF5_BLIND			0x10000000	/* Blind Player */
	{0, 5, 8, 0, 0},
--// RF5_CONF			0x20000000	/* Confuse Player */
	{0, 5, 8, 0, 0},
--// RF5_SLOW			0x40000000	/* Slow Player */
	{0, 7, 10, 0, 0},
--// RF5_HOLD			0x80000000	/* Paralyze Player (translates into forced monsleep) */
	{0, 5, 10, 0, 0},


--// RF6_HASTE			0x00000001	/* Speed self */
	{0, 10, 20, 0, 0},
--// RF6_HAND_DOOM		0x00000002	/* Speed a lot (?) */
	{0, 100, 80, 0, 0},
--// RF6_HEAL			0x00000004	/* Heal self */
	{0, 10, 20, 0, 0},
--// RF6_XXX2			0x00000008	/* Heal a lot (?) */
	{0, 0, 0, 0, 0},
--// RF6_BLINK			0x00000010	/* Teleport Short */
	{0, 5, 15, 0, 0},
--// RF6_TPORT			0x00000020	/* Teleport Long */
	{0, 20, 30, 0, 0},
--// RF6_XXX3			0x00000040	/* Move to Player (?) */
	{0, 0, 0, 0, 0},
--// RF6_XXX4			0x00000080	/* Move to Monster (?) */
	{0, 0, 0, 0, 0},
--// RF6_TELE_TO		0x00000100	/* Move player to monster */
	{0, 20, 30, 0, 0},
--// RF6_TELE_AWAY		0x00000200	/* Move player far away */
	{0, 25, 30, 0, 0},
--// RF6_TELE_LEVEL		0x00000400	/* Move player vertically */
	{0, 30, 60, 0, 0},
--// RF6_XXX5			0x00000800	/* Move player (?) */
	{0, 0, 0, 0, 0},
--// RF6_DARKNESS	 	0x00001000	/* Create Darkness */
	{0, 6, 8, 0, 0},
--// RF6_TRAPS			0x00002000	/* Create Traps */
	{0, 15, 25, 0, 0},
--// RF6_FORGET			0x00004000	/* Cause amnesia */
	{0, 25, 35, 0, 0},
--// RF6_XXX5			0x00008000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF6_XXX5			0x00010000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF6_XXX5			0x00020000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF6_XXX5			0x00040000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF6_XXX5			0x00080000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF6_XXX5			0x00100000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF6_XXX5			0x00200000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF6_XXX5			0x00400000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF6_XXX5			0x00800000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF6_XXX5			0x01000000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF6_XXX5			0x02000000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF6_XXX5			0x04000000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF6_XXX5			0x08000000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF6_XXX5			0x10000000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF6_XXX5			0x20000000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF6_XXX5			0x40000000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF6_XXX5			0x80000000	/* (unavailable) */
	{0, 0, 0, 0, 0},


--// RF0_XXX3			0x00000001	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x00000002	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x00000004	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x00000008	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x00000010	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x00000020	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x00000040	/* (unavailable) */
	{0, 0, 0, 0, 0},

--// RF0_BO_DISE		0x00000080
	{0, 15, 20, 0, 1},
--// RF0_BA_DISE		0x00000100
	{0, 30, 40, 0, 1},

--// RF0_XXX5			0x00000200	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x00000400	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x00000800	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x00001000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x00002000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x00004000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x00008000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x00010000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x00020000	/* (unavailable) */
	{0, 0, 0, 0, 0},

--// RF0_BR_ICE			0x00040000
	{0, 20, 27, 0, 2},
--// RF0_BR_WATER>		0x00080000
	{0, 25, 30, 0, 2},

--// RF0_XXX3			0x00100000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x00200000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x00400000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x00800000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x01000000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x02000000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x04000000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_BO_DISE		0x08000000
	{0, 15, 20, 0, 1},
--// RF0_BA_DISE		0x10000000
	{0, 30, 40, 0, 1},
--// RF0_XXX5			0x20000000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x40000000	/* (unavailable) */
	{0, 0, 0, 0, 0},
--// RF0_XXX5			0x80000000	/* (unavailable) */
	{0, 0, 0, 0, 0},
}

adj_int_pow = { 250, 210, 190, 160, 140, 120, 110, 100, 100, 95, 90, 85, 80, 75, 70, 65, 61, 57, 53, 49, 45, 41, 38, 35, 32, 29, 27, 25, 23, 21, 19, 17, 15, 14, 13, 12, 11, 10, }

--Syntax 1: block is 0..3 (RF4_,RF5_,RF6_,RF0_) and spell is 0..31.
--Syntax 2: block is -1 and spell is 0..127.
--Set pint to 0 to obtain the raw fail rate instead of player-adjusted one,
-- in which case pwis and pskill may be set arbitrarily.
function get_monster_spell_info(block, spell, pint, pdex, pskill)
	local name, dir
	local lev, mana, fail, exp, ftk

	--Syntax 2 reverse calculation
	if block == -1 then
		if spell >= 96 then
			block = 3
			spell = spell - 96
		elseif spell >= 64 then
			block = 2
			spell = spell - 64
		elseif spell >= 32 then
			block = 1
			spell = spell - 32
		else
			block = 0
		end
	end

	--Silly lua index fix
	spell = spell + 1

	--Assume Syntax 1 from here on
	if block == 0 then
	    name = monster_spells4_lua[spell][1]
	    dir = monster_spells4_lua[spell][2]
	elseif block == 1 then
	    name = monster_spells5_lua[spell][1]
	    dir = monster_spells5_lua[spell][2]
	    spell = 32 + spell
	elseif block == 2 then
	    name = monster_spells6_lua[spell][1]
	    dir = monster_spells6_lua[spell][2]
	    spell = 64 + spell
	elseif block == 3 then
	    name = monster_spells0_lua[spell][1]
	    dir = monster_spells0_lua[spell][2]
	    spell = 96 + spell
	end

	lev = innate_powers_lua[spell][1]
	mana = innate_powers_lua[spell][2]
	fail = innate_powers_lua[spell][3]
	exp = innate_powers_lua[spell][4]
	ftk = innate_powers_lua[spell][5]

	--Adjust raw fail rate according to the player's abilities?
	if pint ~= 0 then
		--Reverse stat from 'visual' back to to normal calculation scale
		if pint > 18 + 220 then pint = 18 + 220 end
		if pint > 18 then
			pint = 18 + ((pint - 18) / 10)
		end
		--Also, 3 is minimum possible stat, so any stat-array starts at stat 3 with its index 0.
		pint = pint - 3
		--Calculate our actual fail chance
		fail = (fail * adj_int_pow[pint + 1]) / 100;
		if fail < 1 then fail = 1 end
		if fail > 99 then fail = 99 end
	end

	--Return complete spell info
	return name.."_d"..dir.."_l"..lev.."_m"..mana.."_f"..fail.."_x"..exp.."_k"..ftk
end
