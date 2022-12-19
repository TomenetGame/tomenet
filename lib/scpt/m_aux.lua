-- Server side functions to help with the finally luarized mimic powers - C. Blue

__tmp_powers = {}
__tmp_powers_num = 0


-- define get_check2() for older clients
if rawget(globals(), "power_get_check2") == nil then
	function power_get_check2(prompt, defaultyes)
		return power_get_check(prompt)
	end
end


-- TomeNET additional
-- Get the amount of mana(or power) needed
function power_need_direction(s)
	return power(s).direction
end

function power_need_item(s)
	return power(s).item
end

function add_power(s)
	__tmp_powers[__tmp_powers_num] = s

	__tmp_powers_num = __tmp_powers_num + 1
	return (__tmp_powers_num - 1)
end

function finish_power(must_i)
	local i, s

	s = __tmp_powers[must_i]

	assert(s.name, "No power name!")
	assert(s.name2, "No short name!")
	if not s.level then s.level = 0 end
	assert(s.mana, "No power mana!")
	if not s.mana_max then s.mana_max = s.mana end
	assert(s.fail, "No power failure rate!")
	assert(s.spell, "No power function!")
	assert(s.info, "No power info!")
	assert(s.desc, "No power desc!")
	if not s.direction then s.direction = FALSE end
	if not s.item then s.item = -1 end

	i = new_power(must_i, s.name)
	assert(i == must_i, "ACK ! i != must_i ! please contact the maintainer")
	power(i).mana = s.mana
	power(i).mana_max = s.mana_max
	power(i).fail = s.fail
	power(i).skill_level = s.level
	if type(s.spell_power) == "number" then
		power(i).spell_power = s.spell_power
	else
		power(i).spell_power = 0
	end
	__power_spell[i] = s.spell
	__power_name2[i] = s.name2
	__power_info[i] = s.info
	__power_desc[i] = s.desc
	return i
end

-- Creates the power arrays
__power_spell = {}
__power_name2 = {}
__power_info = {}
__power_desc = {}

-- Find a spell by name
function find_power(name)
	local i

	i = 0
	while (i < __tmp_powers_num) do
		if __tmp_powers[i].name == name then return i end
		i = i + 1
	end
	return -1
end

-- Can we use the power? (Note: Usually all powers have level 0 so we can always use them)
function power_get_level(i, s)
	return players(i).s_info[SKILL_MIMICRY + 1].value / 1000 - spells(s).level + 1
end
function is_ok_power(i, s)
	if power_get_level(i, s) >= 1 then return 1 end
	return nil
end
function is_available_power(i, s)
	--if (!(players(i).innate_spells[power[s].flag_block] & (1U << power[s].flag_index))) return FALSE;
	if power_get_level(i, s) >= 1 then return 1 end
	return nil
end

-- Get the amount of mana(or power) needed
function power_get_mana(i, s)
	return power(s).mana
end

-- Return the amount of power(mana, piety, whatever) for the spell
function power_get_power(i, s)
	-- client-side (0) or server-side (>=1) ?
	if i ~= 0 then
		player = players(i)
	end
	return player.cmp
end

-- Changes the amount of power(mana, piety, whatever) for the spell
function power_adjust_power(i, s, x)
	power_increase_mana(i, x)
end

-- Print the power list
-- XXX client only
function print_powers2(i)
	local x, y, index, size, s

	x = 0
	y = 2
	size = 0

	-- Parse all powers
	for index, s in __tmp_powers do
		local color = TERM_L_DARK
		local lvl = power_get_level(i, s)

		if is_ok_power(i, s) then
			if power_get_mana(i, s) > power_get_power(i, s) then color = TERM_ORANGE
			else color = TERM_L_GREEN end
		end

		c_prt(color, format("%c) %-22s %4s %3d%s %s", size + strbyte("a"), power(s).name, power_get_mana(i, s), power_chance(i, s), "%", __power_info[s]()), y, x)
		y = y + 1
		size = size + 1
	end

	prt(format("   %-22s Cost Fail Info", "Name"), 1, x)
	return y
end

function print_custom_tome(i, inven_slot)
	local x, y, index, sch, size, s, custom_book

	x = 0
	y = 2
	size = 0

	-- Parse all powers
	for index, s in custom_book do
		local color = TERM_L_DARK
		local lvl = power_get_level(i, s, 50, -50)

		if is_ok_power(i, s) then
			if power_get_mana(i, s) > power_get_power(i, s) then color = TERM_ORANGE
			else color = TERM_L_GREEN end
		end

		c_prt(color, format("%c) %-22s %4s %3d%s %s", size + strbyte("a"), power(s).name, power_get_mana(i, s), power_chance(i, s), "%", __power_info[s]()), y, x)
		y = y + 1
		size = size + 1
	end

	prt(format("   %-22s%-14s Level Cost Fail Info", "Name", "School"), 1, x)
	return y
end

-- XXX client only
function print_power_desc(s, y)
	local index, desc, x

	x = 0

	if type(__power_desc[s]) == "string" then c_prt(TERM_L_BLUE, __power_desc[s], y, x)
	else
		for index, desc in __power_desc[s] do
			c_prt(TERM_L_BLUE, desc, y, x)
			y = y + 1
		end
	end
	if power_check_affect(s, "piety", FALSE) then
		c_prt(TERM_L_WHITE, "It uses piety to cast.", y, x)
		y = y + 1
	end
	if not power_check_affect(s, "blind") then
		c_prt(TERM_ORANGE, "It is castable even while blinded.", y, x)
		y = y + 1
	end
	if not power_check_affect(s, "confusion") then
		c_prt(TERM_ORANGE, "It is castable even while confused.", y, x)
		y = y + 1
	end

	return y
end

-- Returns spell chance of failure for power
function power_chance(i, s)
	local chance, s_ptr
	local player, ls

	s_ptr = power(s)

	--hack: LIMIT_SPELLS uses top-level failrate here instead of worse one we had at lower level!
	-- client-side (0) or server-side (>=1) ?
	if i ~= 0 then
		player = players(i)
		ls = player.limit_spells
		player.limit_spells = 0
	--client version recent enough to even know 'hack_force_spell_level'? (otherwise we'd get a lua error)
	elseif (def_hack("hack_force_spell_level", nil)) then
		ls = hack_force_spell_level
		hack_force_spell_level = 0
	end

	-- Hack: "101" means 100% chance to succeed ('fail' is unsigned byte, so it'll be 157) - C. Blue
	if (s_ptr.fail == 101) then
		chance = 0
	-- A new hack: "102" means greatly reduced fail chance (from 0 base fail chance) - C. Blue
	elseif (s_ptr.fail == 102) then
		chance = (lua_power_chance(i, 0, power_get_level(i, s, 50), s_ptr.skill_level, power_get_mana(i, s), power_get_power(i, s), get_power_stat(s)) + 5) / 6
	else
		-- Extract the base spell failure rate
		chance = lua_power_chance(i, s_ptr.fail, power_get_level(i, s, 50), s_ptr.skill_level, power_get_mana(i, s), power_get_power(i, s), get_power_stat(s))
	end

	--unhack: LIMIT_SPELLS
	-- client-side (0) or server-side (>=1) ?
	if i ~= 0 then
		player = players(i)
		player.limit_spells = ls
	--client version recent enough to even know 'hack_force_spell_level'? (otherwise we'd get a lua error)
	elseif (def_hack("hack_force_spell_level", nil)) then
		hack_force_spell_level = ls
	end

	-- Return the chance
	return chance
end

function power_check_affect(s, name, default)
	local s_ptr = __tmp_powers[s]
	local a

	if type(s_ptr[name]) == "number" then
		a = s_ptr[name]
	else
		a = default
	end

	if a == FALSE then
		return nil
	else
		return TRUE
	end
end

-- Returns the stat to use for the power, INT by default
function get_power_stat(s)
	if not __tmp_powers[s].stat then return A_INT
	else return __tmp_powers[s].stat end
end

-- XXX server only
-- one question.. why this should be LUA anyway?
-- because accessing lua table is so badly easier in lua
function use_power(i, s, s_ptr, no_cost, other)
	-- client-side (0) or server-side (>=1) ?
	if i ~= 0 then
		player = players(i)
	end

	local use = FALSE


	-- No magic
	if check_antimagic(Ind, get_power_am(s)) == TRUE then
--Next line is already in the server sources.
--		msg_print(i, "Your anti-magic field disrupts any magic attempts.")
		local energy = level_speed(player.wpos);
		player.energy = player.energy - energy
		return 1 --continue ftk
	end

	-- TODO: check the ownership
	-- uh?

	-- if it costs something then some condition must be met
	if not no_cost then
		-- Require lite
		if (power_check_affect(s, "blind")) and ((player.blind > 0) or (no_lite(Ind) == TRUE)) then
			local energy = level_speed(player.wpos);
			player.energy = player.energy - energy
			msg_print(i, "You cannot see!")
			return 0
		end

		-- Not when confused
		if (power_check_affect(s, "confusion")) and (player.confused > 0) then
			local energy = level_speed(player.wpos);
			player.energy = player.energy - energy
			msg_print(i, "You are too confused!")
			return 0
		end

		-- Level requirements met?
		if (power_get_level(i, s) < 1) then
			msg_print(i, "Your skill is not high enough!")
			lua_intrusion(i, "bad power level")
			return 0
		end

		-- Enough mana
		if (power_get_mana(i, s) > power_get_power(i, s)) then
			local energy = level_speed(player.wpos);
			player.energy = player.energy - energy
--			if (get_check2("You do not have enough "..get_power_name(s)..", do you want to try anyway?", FALSE) == FALSE) then return end
			msg_print(i, "You do not have enough mana to use "..power(s).name..".")
				return 0
		end

--[[		-- Sanity check for direction
		if (need_direction(s) && other.direction == -1) then
			msg_print(i, "Spell needs a direction.")
			return
		end
]]

		-- Invoke the spell effect
		if (magik(power_chance(i, s)) == FALSE) then
			local mp_cost = power_get_mana(i, s)

			--unlike for spells, stick with not giving an extra confirmation message for now..
			--msg_print(i, "You successfully use "..spell(s).name..".")

			-- Reduce mana BEFORE casting the spell, for Necromancy to work effectively:
			-- If the monster dies, MP should not get refunded before the spell cost was actually deducted.
			adjust_power(i, s, -mp_cost)
			use  = TRUE

			if (__power_spell[s](other) ~= nil) then
				--correct the situation - we have to do it this way round,
				--so we were able to deduct MP before actually casting the spell above
				use = FALSE
				--and refund the mana
				adjust_power(i, s, mp_cost)
			end
		else
			msg_print(i, "\255yYou failed to get the power "..power(s).name.." off!")
			-- Reduce mana
			adjust_power(i, s, -get_mana(i, s))
			use  = TRUE
		end
	else
		__power_spell[s](other)
	end

	if use == TRUE then
		-- Take a turn
		local energy = level_speed(player.wpos);
		player.energy = player.energy - energy
	end

	player.redraw = bor(player.redraw, PR_MANA)
	--player.window = bor(player.window, PW_PLAYER)
	--player.window = bor(player.window, PW_SPELL)

	return 1
end

function pre_exec_power_dir(s)
	if __tmp_powers[s].direction then
		if type(__tmp_powers[s].direction) == "function" then
			return __tmp_powers[s].direction()
		else
			return __tmp_powers[s].direction
		end
	end
end

-- Returns the percentage of AM (Antimagic field) efficiency vs this power
function get_power_am(s)
	if not __tmp_powers[s].am then return 100
	else return __tmp_powers[s].am end
end

-- Returns if a power is an attack power and therefore to be repeated in FTK mode - C. Blue
-- note: 0 means no FTK, 1 means 'do FTK', 2 means 'do FTK && no need for monster-free LOS'
function get_power_ftk(s)
	if not __tmp_powers[s].ftk then return 0
	else return __tmp_powers[s].ftk end
end

-- Returns if a power is used on players instead of monsters (friendly power)
function get_power_friendly(s)
	if not __tmp_powers[s].friendly then return 0
	else return __tmp_powers[s].friendly end
end
