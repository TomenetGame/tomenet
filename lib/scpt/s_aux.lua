-- $Id$
-- Server side functions to help with spells, do not touch
--

__schools = {}
__schools_num = 0

__tmp_spells = {}
__tmp_spells_num = 0

-- TomeNET additional
-- Get the amount of mana(or power) needed
function need_direction(s)
        return spell(s).direction
end

function need_item(s)
        return spell(s).item
end



function add_school(s)
	__schools[__schools_num] = s
        
        __schools_num = __schools_num + 1
        return (__schools_num - 1)
end

function finish_school(i)
	local s
        
        s = __schools[i]
        assert(s.name, "No school name!")
        assert(s.skill, "No school skill!")
        new_school(i, s.name, s.skill)
end

function add_spell(s)
	__tmp_spells[__tmp_spells_num] = s
        
        __tmp_spells_num = __tmp_spells_num + 1
        return (__tmp_spells_num - 1)
end

function finish_spell(must_i)
	local i, s
        
        s = __tmp_spells[must_i]

        assert(s.name, "No spell name!")
        assert(s.school, "No spell school!")
        assert(s.level, "No spell level!")
        assert(s.mana, "No spell mana!")
        if not s.mana_max then s.mana_max = s.mana end
        assert(s.fail, "No spell failure rate!")
        assert(s.spell, "No spell function!")
        assert(s.info, "No spell info!")
        assert(s.desc, "No spell desc!")
       	if not s.direction then s.direction = FALSE end
       	if not s.item then s.item = -1 end

        i = new_spell(must_i, s.name)
        assert(i == must_i, "ACK ! i != must_i ! please contact the maintainer")
        if type(s.school) == "number" then __spell_school[i] = {s.school}
	else __spell_school[i] = s.school end
        spell(i).mana = s.mana
        spell(i).mana_max = s.mana_max
        spell(i).fail = s.fail
        spell(i).skill_level = s.level
	if type(s.spell_power) == "number" then
		spell(i).spell_power = s.spell_power
	else
		spell(i).spell_power = 1
	end
        __spell_spell[i] = s.spell
        __spell_info[i] = s.info
        __spell_desc[i] = s.desc
        return i
end

-- Creates the school books array
__spell_spell = {}
__spell_info = {}
__spell_desc = {}
__spell_school = {}
school_book = {}

-- Find a spell by name
function find_spell(name)
        local i

        i = 0
        while (i < __tmp_spells_num) do
                if __tmp_spells[i].name == name then return i end
                i = i + 1
        end
        return -1
end

-- Find if the school is under the influence of a god, returns nil or the level
--
-- pgod? 100% sure it won't work
--[[
function get_god_level(i, sch)
--	local player = players(i)
        if __schools[sch].gods[player.pgod] then
                return (s_info[__schools[sch].gods[player.pgod].skill + 1].value * __schools[sch].gods[player.pgod].mul) / __schools[sch].gods[player.pgod].div
        else
                return nil
        end
end
]]

-- Change this fct if I want to switch to learnable spells
function get_level_school(i, s, max, min)
	local lvl, sch, index, num, bonus
--	local player = players(i)

	lvl = 0
        num = 0
        bonus = 0

        -- No max specified ? assume 50
        if not max then
                max = 50
        end

        for index, sch in __spell_school[s] do
                local r, p, ok = 0, 0, 0, 0

                -- Take the basic skill value
                r = player.s_info[(school(sch).skill) + 1].value

        	-- Are we under sorcery effect ?
		p = 0
                if __schools[sch].sorcery then
                        p = player.s_info[SKILL_SORCERY + 1].value
                end
                
                -- Find the higher
                ok = r
                if r < p then ok = p end

                -- Do we need to add a special bonus ?
                if __schools[sch].bonus_level then
                	bonus = bonus + (__schools[sch].bonus_level() * SKILL_STEP)
                end

                -- Apply it
                lvl = lvl + ok
                num = num + 1
	end
        -- / 10 because otherwise we can overflow a s32b and we can use a u32b because the value can be negative
        -- The loss of information should be negligible since 1 skill = 1000 internaly
        lvl = (lvl / num) / 10
	if not min then
	        lvl = lua_get_level(i, s, lvl, max, 1, bonus)
	else
	        lvl = lua_get_level(i, s, lvl, max, min, bonus)
        end

--	--Hack: Disruption Shield only for Istari. Not for Adventurer/Ranger.
--	if spell(s).name == "Disruption Shield" then
--		if player.pclass == 0 then
--			lvl = 0
--		end
--		if player.pclass == 8 then
--			lvl = 0
--		end
--	end

        return lvl
end

-- The real get_level, works for schooled magic and for innate powers
function get_level(i, s, max, min)
        if type(s) == "number" then
                return get_level_school(i, s, max, min)
        else
-- this shouldn't happen for now
                return get_level_power(i, s, max, min)
        end
end

-- Can we cast the spell ?
function is_ok_spell(i, s)
        if get_level(i, s, 50, 0) == 0 then return nil end
	return 1
end

-- Get the amount of mana(or power) needed
function get_mana(i, s)
        return spell(s).mana + get_level(i, s, spell(s).mana_max - spell(s).mana, 0)
end

-- Return the amount of power(mana, piety, whatever) for the spell
function get_power(i, s)
--	local player = players(i)
        if check_affect(s, "piety", FALSE) then
                return player.grace
        else
                return player.csp
        end
end

-- Return the amount of power(mana, piety, whatever) for the spell
function get_power_name(s)
        if check_affect(s, "piety", FALSE) then
                return "piety"
        else
                return "mana"
        end
end

-- Changes the amount of power(mana, piety, whatever) for the spell
function adjust_power(i, s, x)
        if check_affect(s, "piety", FALSE) then
                inc_piety(i, GOD_ALL, x)
        else
		increase_mana(i, x)
        end
end

-- Print the book and the spells
-- XXX client only
function print_book(i, book, spl)
	local x, y, index, sch, size, s

	x = 0
	y = 2
	size = 0

	-- Hack if the book is 255 it is a random book
	if book == 255 then
		school_book[book] = {spl}
	end

	-- Parse all spells
	for index, s in school_book[book] do
		local color = TERM_L_DARK
		local lvl = get_level(i, s, 50, -50)
		local xx, sch_str

		if is_ok_spell(i, s) then
			if get_mana(i, s) > get_power(i, s) then color = TERM_ORANGE
			else color = TERM_L_GREEN end
		end

		xx = nil
		sch_str = ""

		for index, sch in __spell_school[s] do
			if xx then
				sch_str = sch_str.."/"..school(sch).name
			else
				xx = 1
				sch_str = sch_str..school(sch).name
			end
		end

		c_prt(color, format("%c) %-20s%-16s   %3d %4s %3d%s %s", size + strbyte("a"), spell(s).name, sch_str, lvl, get_mana(i, s), spell_chance(i, s), "%", __spell_info[s]()), y, x)
		y = y + 1
		size = size + 1
	end

	prt(format("   %-20s%-16s Level Cost Fail Info", "Name", "School"), 1, x)
	return y
end


-- Print the book and the spells
-- XXX client only
function print_book2(i, inven_slot, sval, spl)
	local x, y, index, sch, size, s, book

	x = 0
	y = 2
	size = 0

--[[STOREBROWSE
	if inven_slot ~= -1 then
	    --from inventory
	    book = get_inven_sval(Ind, inven_slot)
	else
	    --browsing within a store!
	    book = sval
	end
]]
	if inven_slot >= 0 then
	    --from inventory
	    book = get_inven_sval(Ind, inven_slot)
	else
	    --browsing within a store!
	    book = sval
	end

	-- Hack for custom tomes
	if book == 100 or book == 101 or book == 102 then
		return print_custom_tome(i, inven_slot)
	end

	-- Hack if the book is 255 it is a random book
	if book == 255 then
		school_book[book] = {spl}
	end

	-- Parse all spells
	for index, s in school_book[book] do
        	local color = TERM_L_DARK
                local lvl = get_level(i, s, 50, -50)
        	local xx, sch_str

                if is_ok_spell(i, s) then
                	if get_mana(i, s) > get_power(i, s) then color = TERM_ORANGE
                        else color = TERM_L_GREEN end
                end

                xx = nil
                sch_str = ""

		for index, sch in __spell_school[s] do
                	if xx then
		                sch_str = sch_str.."/"..school(sch).name
                	else
		                xx = 1
		                sch_str = sch_str..school(sch).name
	                end
                end

                c_prt(color, format("%c) %-20s%-16s   %3d %4s %3d%s %s", size + strbyte("a"), spell(s).name, sch_str, lvl, get_mana(i, s), spell_chance(i, s), "%", __spell_info[s]()), y, x)
		y = y + 1
                size = size + 1
        end

        prt(format("   %-20s%-16s Level Cost Fail Info", "Name", "School"), 1, x)
        return y
end

function print_custom_tome(i, inven_slot)
	local x, y, index, sch, size, s, custom_book

	x = 0
        y = 2
        size = 0

--[[STOREBROWSE
--HACK/problem: can't browse custom tomes in shops!:
	if inven_slot == -1 then return 0 end
]]

	custom_book = {}

	if get_inven_xtra(Ind, inven_slot, 1) ~= 0 then
		custom_book[1] = get_inven_xtra(Ind, inven_slot, 1) - 1
	end
	if get_inven_xtra(Ind, inven_slot, 2) ~= 0 then
		custom_book[2] = get_inven_xtra(Ind, inven_slot, 2) - 1
	end
	if get_inven_xtra(Ind, inven_slot, 3) ~= 0 then
		custom_book[3] = get_inven_xtra(Ind, inven_slot, 3) - 1
	end
	if get_inven_xtra(Ind, inven_slot, 4) ~= 0 then
		custom_book[4] = get_inven_xtra(Ind, inven_slot, 4) - 1
	end
	if get_inven_xtra(Ind, inven_slot, 5) ~= 0 then
		custom_book[5] = get_inven_xtra(Ind, inven_slot, 5) - 1
	end
	if get_inven_xtra(Ind, inven_slot, 6) ~= 0 then
		custom_book[6] = get_inven_xtra(Ind, inven_slot, 6) - 1
	end
	if get_inven_xtra(Ind, inven_slot, 7) ~= 0 then
		custom_book[7] = get_inven_xtra(Ind, inven_slot, 7) - 1
	end
	if get_inven_xtra(Ind, inven_slot, 8) ~= 0 then
		custom_book[8] = get_inven_xtra(Ind, inven_slot, 8) - 1
	end
	if get_inven_xtra(Ind, inven_slot, 9) ~= 0 then
		custom_book[9] = get_inven_xtra(Ind, inven_slot, 9) - 1
	end

        -- Parse all spells
	for index, s in custom_book do
        	local color = TERM_L_DARK
                local lvl = get_level(i, s, 50, -50)
        	local xx, sch_str

                if is_ok_spell(i, s) then
                	if get_mana(i, s) > get_power(i, s) then color = TERM_ORANGE
                        else color = TERM_L_GREEN end
                end

                xx = nil
                sch_str = ""

		for index, sch in __spell_school[s] do
                	if xx then
		                sch_str = sch_str.."/"..school(sch).name
                	else
		                xx = 1
		                sch_str = sch_str..school(sch).name
	                end
                end

                c_prt(color, format("%c) %-20s%-16s   %3d %4s %3d%s %s", size + strbyte("a"), spell(s).name, sch_str, lvl, get_mana(i, s), spell_chance(i, s), "%", __spell_info[s]()), y, x)
		y = y + 1
                size = size + 1
        end

        prt(format("   %-20s%-16s Level Cost Fail Info", "Name", "School"), 1, x)
        return y
end

-- XXX client only
function print_spell_desc(s, y)
	local index, desc, x
        
        x = 0

	if type(__spell_desc[s]) == "string" then c_prt(TERM_L_BLUE, __spell_desc[s], y, x)
        else
        	for index, desc in __spell_desc[s] do
		        c_prt(TERM_L_BLUE, desc, y, x)
		        y = y + 1
	        end
        end
        if check_affect(s, "piety", FALSE) then
		c_prt(TERM_L_WHITE, "It uses piety to cast.", y, x)
		y = y + 1
        end
        if not check_affect(s, "blind") then
		c_prt(TERM_ORANGE, "It is castable even while blinded.", y, x)
		y = y + 1
        end
        if not check_affect(s, "confusion") then
		c_prt(TERM_ORANGE, "It is castable even while confused.", y, x)
		y = y + 1
        end
end

function book_spells_num(book)
	local size, index, sch

        size = 0

        -- Hack if the book is 255 it is a random book
	if book == 255 then
                return 1
        end

        -- Parse all spells
        for index, s in school_book[book] do
		size = size + 1
        end

        return size
end

function book_spells_num2(inven_slot, sval)
	local size, index, sch, book

        size = 0

--[[STOREBROWSE
        if inven_slot ~= -1 then
	    book = get_inven_sval(Ind, inven_slot)
        else
    	    book = sval
        end
]]
        if inven_slot >= 0 then
	    book = get_inven_sval(Ind, inven_slot)
        else
    	    book = sval
        end

        -- Hack if the book is 255 it is a random book
        if book == 255 then
                return 1
        end

        -- Hacks for custom tomes
        if book == 100 or book == 101 or book == 102 then
--[[STOREBROWSE
--HACK/problem: can't browse custom tomes in shops!:
		if inven_slot == -1 then return 0 end
]]
    		if get_inven_xtra(Ind, inven_slot, 9) ~= 0 then
    			return 9
    		end
    		if get_inven_xtra(Ind, inven_slot, 8) ~= 0 then
    			return 8
    		end
    		if get_inven_xtra(Ind, inven_slot, 7) ~= 0 then
    			return 7
    		end
    		if get_inven_xtra(Ind, inven_slot, 6) ~= 0 then
    			return 6
    		end
    		if get_inven_xtra(Ind, inven_slot, 5) ~= 0 then
    			return 5
    		end
    		if get_inven_xtra(Ind, inven_slot, 4) ~= 0 then
    			return 4
    		end
    		if get_inven_xtra(Ind, inven_slot, 3) ~= 0 then
    			return 3
    		end
    		if get_inven_xtra(Ind, inven_slot, 2) ~= 0 then
    			return 2
    		end
    		if get_inven_xtra(Ind, inven_slot, 1) ~= 0 then
    			return 1
    		end
		--no spells in this custom tome yet
    		return 0
    	end

        -- Parse all spells
	for index, s in school_book[book] do
                size = size + 1
        end

        return size
end

function spell_x(book, spl, s)
        if book == 255 then
                return spl
        else
                local i, x, val

                i, val = next(school_book[book], nil)
                x = 0
                while x < s do
                        i, val = next(school_book[book], i)
                        x = x + 1
                end
                return val
        end
end

function spell_x2(inven_slot, sval, spl, s)
	local book

--[[STOREBROWSE
	if inven_slot ~= -1 then
	    book = get_inven_sval(Ind, inven_slot)
	else
	    book = sval
	end
]]
	if inven_slot >= 0 then
	    book = get_inven_sval(Ind, inven_slot)
	else
	    book = sval
	end

        if book == 255 then
                return spl
        elseif book == 100 or book == 101 or book == 102 then
--[[STOREBROWSE
--HACK/problem: can't browse custom tomes in shops!:
		if inven_slot == -1 then return 0 end
]]
    		if s == 0 then return get_inven_xtra(Ind, inven_slot, 1) - 1 end
    		if s == 1 then return get_inven_xtra(Ind, inven_slot, 2) - 1 end
    		if s == 2 then return get_inven_xtra(Ind, inven_slot, 3) - 1 end
    		if s == 3 then return get_inven_xtra(Ind, inven_slot, 4) - 1 end
    		if s == 4 then return get_inven_xtra(Ind, inven_slot, 5) - 1 end
    		if s == 5 then return get_inven_xtra(Ind, inven_slot, 6) - 1 end
    		if s == 6 then return get_inven_xtra(Ind, inven_slot, 7) - 1 end
    		if s == 7 then return get_inven_xtra(Ind, inven_slot, 8) - 1 end
    		if s == 8 then return get_inven_xtra(Ind, inven_slot, 9) - 1 end
    		--failure?
    		return 0
        else
                local i, x, val

                i, val = next(school_book[book], nil)
                x = 0
                while x < s do
                        i, val = next(school_book[book], i)
                        x = x + 1
                end
                return val
        end
end

function spell_in_book(book, spell)
        local i, s
	for i, s in school_book[book] do
                if s == spell then return TRUE end
        end
        return FALSE
end

function spell_in_book2(inven_slot, book, spell)
        local i, s

	--treat exception
	if book == 255 then
		return FALSE
	end

	--hack of custom tomes
	if book == 100 or book == 101 or book == 102 then
		return spell_in_custom_tome(inven_slot, spell)
	end

	for i, s in school_book[book] do
                if s == spell then return TRUE end
        end
        return FALSE
end

function spell_in_custom_tome(inven_slot, spell)
	local i

	for i = 1, 9 do
		if get_inven_xtra(Ind, inven_slot, i) - 1 == spell then
			return TRUE
		end
	end

	return FALSE
end

-- Returns spell chance of failure for spell
function spell_chance(i, s)
	local chance, s_ptr

	s_ptr = spell(s)

	-- Hack: "-99" means 100% chance to succeed ('fail' is unsigned byte, so it'll be 157) - C. Blue
	if (s_ptr.fail == 157) then
		chance = 0
	-- A new hack: "-98" means halved fail chance (from 0 base fail chance) - C. Blue
	elseif (s_ptr.fail == 158) then
		chance = lua_spell_chance(i, 0, get_level(i, s, 50), s_ptr.skill_level, get_mana(i, s), get_power(i, s), get_spell_stat(s)) / 6 + 5
	else
		-- Extract the base spell failure rate
		chance = lua_spell_chance(i, s_ptr.fail, get_level(i, s, 50), s_ptr.skill_level, get_mana(i, s), get_power(i, s), get_spell_stat(s))
	end

	-- Return the chance
	return chance
end

function check_affect(s, name, default)
        local s_ptr = __tmp_spells[s]
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

-- Returns the stat to use for the spell, INT by default
function get_spell_stat(s)
        if not __tmp_spells[s].stat then return A_INT
        else return __tmp_spells[s].stat end
end

-- XXX server only
-- one question.. why this should be LUA anyway?
-- because accessing lua table is so badly easier in lua
function cast_school_spell(i, s, s_ptr, no_cost, other)
--	local player = players(i)
	local use = FALSE

	-- No magic
	if check_antimagic(Ind, get_spell_am(s)) == TRUE then
--Next line is already in the server sources.
--		msg_print(i, "Your anti-magic field disrupts any magic attempts.")
		return 1 --continue ftk
        end

	-- TODO: check the ownership
        -- uh?

	-- if it costs something then some condition must be met
	if not no_cost then
	 	-- Require lite
		if (check_affect(s, "blind")) and ((player.blind > 0) or (no_lite(Ind) == TRUE)) then
			msg_print(i, "You cannot see!")
			return 0
		end

		-- Not when confused
                if (check_affect(s, "confusion")) and (player.confused > 0) then
			msg_print(i, "You are too confused!")
			return 0
		end

		-- Level requirements met?
		if (get_level(i, s, 50, -50) < 1) then
			msg_print(i, "Your skill is not high enough!")
			lua_intrusion(i, "bad spell level")
			return 0
		end

		-- Enough mana
		if (get_mana(i, s) > get_power(i, s)) then
--                        if (get_check("You do not have enough "..get_power_name(s)..", do you want to try anyway?") == FALSE) then return end
			msg_print(i, "You do not have enough mana to cast "..spell(s).name..".")
				return 0
	        end

--[[		-- Sanity check for direction
		if (need_direction(s) && other.direction == -1) then
			msg_print(i, "Spell needs a direction.")
			return
		end
]]

		-- Invoke the spell effect
	        if (magik(spell_chance(i, s)) == FALSE) then
			msg_print(i, "You successfully cast the spell "..spell(s).name..".")
			if (__spell_spell[s](other) == nil) then
	        		use  = TRUE
			end
		else
                        local index, sch

--[[
			-- added because this is *extremely* important --pelpel
			if (flush_failure) then flush() end
]]

                        msg_print(i, "You failed to get the spell "..spell(s).name.." off!")
			for index, sch in __spell_school[s] do
	                        if __schools[sch].fail then
        	                        __schools[sch].fail(spell_chance(i, s))
                	        end
	                end
			use  = TRUE
		end
	else
        	__spell_spell[s](other)
        end

	if use == TRUE then
	        -- Reduce mana
		adjust_power(i, s, -get_mana(i, s))

	        -- Take a turn
                local energy = level_speed(player.wpos);
-- DGDGDG        	if is_magestaff() == TRUE then energy = (8 * energy) / 10 end
                player.energy = player.energy - energy
	end

	player.redraw = bor(player.redraw, PR_MANA)
	player.window = bor(player.window, PW_PLAYER)
	--player.window = bor(player.window, PW_SPELL)

	return 1
end

function get_spellbook_name_colour(i)
	local s
	s = __spell_school[i][1]
	-- green for priests
	if (s >= SCHOOL_HOFFENSE and s <= SCHOOL_HSUPPORT) then return TERM_GREEN end
	-- light green for druids
	if (s == SCHOOL_DRUID_ARCANE or s == SCHOOL_DRUID_PHYSICAL) then return TERM_L_GREEN end
	-- orange for astral tome
	if (s == SCHOOL_ASTRAL) then return TERM_ORANGE end
	-- yellow for mindcrafters
	if (s >= SCHOOL_PPOWER and s <= SCHOOL_MINTRUSION) then return TERM_YELLOW end
	-- light blue for the rest (istari schools)
	return TERM_L_BLUE
end


-- Helper functions
HAVE_ARTIFACT = 0
HAVE_OBJECT = 1
HAVE_EGO = 2
function have_object(mode, type, find, find2)
	local o, i, min, max
        
        max = 0
        min = 0
        if band(mode, USE_EQUIP) == USE_EQUIP then
        	min = INVEN_WIELD
                max = INVEN_TOTAL
        end
        if band(mode, USE_INVEN) == USE_INVEN then
        	min = 0
                if max == 0 then max = INVEN_WIELD end
        end

        i = min
        while i < max do
        	o = get_object(i)
                if o.k_idx ~= 0 then
			if type == HAVE_ARTIFACT then
                		if find == o.name1 then return i end
        		end
			if type == HAVE_OBJECT then
                        	if find2 == nil then
                			if find == o.k_idx then return i end
                                else
                			if (find == o.tval) and (find2 == o.sval) then return i end
                                end
        		end
			if type == HAVE_EGO then
                		if (find == o.name2) or (find == o.name2b) then return i end
        		end
                end
                i = i + 1
        end
        return -1
end

function pre_exec_spell_dir(s)
        if __tmp_spells[s].direction then
                if type(__tmp_spells[s].direction) == "function" then
                        return __tmp_spells[s].direction()
                else
        		return __tmp_spells[s].direction
                end
        end
end

function pre_exec_spell_extra(s)
        if __tmp_spells[s].extra then
                __pre_exec_extra = __tmp_spells[s].extra()
                return TRUE
        end
end

function pre_exec_spell_item(s)
        if __tmp_spells[s].get_item then
                if type(__tmp_spells[s].get_item.get) == "function" then
	        	__get_item_hook_default = __tmp_spells[s].get_item.get
			lua_set_item_tester(0, "__get_item_hook_default")
                else
			lua_set_item_tester(lua__tmp_spells[s].get_item.get, "")
                end
                if not __tmp_spells[s].get_item.inven then __tmp_spells[s].get_item.inven = FALSE end
                if not __tmp_spells[s].get_item.equip then __tmp_spells[s].get_item.equip = FALSE end
                if not __tmp_spells[s].get_item.floor then __tmp_spells[s].get_item.floor = FALSE end
                local ret
                ret, __pre_exec_item = get_item_aux(0, __tmp_spells[s].get_item.prompt, __tmp_spells[s].get_item.equip, __tmp_spells[s].get_item.inven, __tmp_spells[s].get_item.floor)
                return ret
        end
end

-- Returns the percentage of AM (Antimagic field) efficiency vs this spell
function get_spell_am(s)
        if not __tmp_spells[s].am then return 100
        else return __tmp_spells[s].am end
end

-- Returns if a spell is an attack spell and therefore to be repeated in FTK mode - C. Blue
-- note: 0 means no FTK, 1 means 'do FTK', 2 means 'do FTK && no need for monster-free LOS'
function get_spell_ftk(s)
        if not __tmp_spells[s].ftk then return 0
        else return __tmp_spells[s].ftk end
end

-- Returns if a spell is used on players instead of monsters (friendly spell)
function get_spell_friendly(s)
        if not __tmp_spells[s].friendly then return 0
        else return __tmp_spells[s].friendly end
end
