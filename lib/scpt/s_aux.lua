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
	local player = players(i)
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
                local r, s, p, ok = 0, 0, 0, 0

                -- Take the basic skill value
                r = player.s_info[(school(sch).skill) + 1].value

	c_msg_print("r:"..r)
        	-- Are we under sorcery effect ?
                if __schools[sch].sorcery then
                        s = player.s_info[SKILL_SORCERY + 1].value
                end
	c_msg_print("s:"..s)
                
                -- Find the higher
                ok = r
                if ok < s then ok = s end
                if ok < p then ok = p end

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
	local player = players(i)
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

function spell_in_book(book, spell)
        local i, s

	for i, s in school_book[book] do
                if s == spell then return TRUE end
        end
        return FALSE
end

-- Returns spell chance of failure for spell
function spell_chance(i, s)
        local chance, s_ptr

	s_ptr = spell(s)

	-- Extract the base spell failure rate
        chance = lua_spell_chance(i, s_ptr.fail, get_level(i, s, 50), s_ptr.skill_level, get_mana(i, s), get_power(i, s), get_spell_stat(s))

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
	if check_antimagic(Ind) == TRUE then
		msg_print(i, "Your anti-magic field disrupts any magic attempts.")
		return
        end

	-- TODO: check the ownership
        -- uh?

	-- if it costs something then some condition must be met
	if not no_cost then
	 	-- Require lite
		if (check_affect(s, "blind")) and ((player.blind > 0) or (no_lite(Ind) == TRUE)) then
			msg_print(i, "You cannot see!")
			return
		end

		-- Not when confused
                if (check_affect(s, "confusion")) and (player.confused > 0) then
			msg_print(i, "You are too confused!")
			return
		end

		-- Enough mana
		if (get_mana(i, s) > get_power(s)) then
--                        if (get_check("You do not have enough "..get_power_name(s)..", do you want to try anyway?") == FALSE) then return end
				return
	        end
        
		-- Invoke the spell effect
	        if (magik(spell_chance(i, s)) == FALSE) then
			if (__spell_spell[s](other) == nil) then
	        		use  = TRUE
			end
		else
                        local index, sch

--[[
			-- added because this is *extremely* important --pelpel
			if (flush_failure) then flush() end
]]

                        msg_print(i, "You failed to get the spell off!")
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
		adjust_power(s, -get_mana(i, s))

--[[ DGDGDGDG - Later
	        -- Take a turn
        	if is_magestaff() == TRUE then energy_use = 80
	        else energy_use = 100 end
]]
	end

	player.redraw = bor(player.redraw, PR_MANA)
	player.window = bor(player.window, PW_PLAYER)
	player.window = bor(player.window, PW_SPELL)
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
        if __tmp_spells[s].direction then return __tmp_spells[s].direction end
end
