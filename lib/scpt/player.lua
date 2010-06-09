function ind(i)
	local id

        id = get_playerind(i)
        if id == -1 then msg_print(Ind, "Unknown player "..i) end
        return id
end

--note: multiple matches will just result in picking one instead of giving an error message - C. Blue
function ind_loose(i)
	local id

        id = get_playerind_loose(i)
        if id == -1 then msg_print(Ind, "Unknown player "..i) end
        return id
end

function slot_loose(pind, iname)
	local s

        s = get_playerslot_loose(pind, iname)
        if s == -1 then msg_print(Ind, "Unknown item "..iname) end
        return s
end

function players(i)
        if type(i) == "number" then
			return Players_real[i + 1]
--[[
			if i < 0 then return Player_real
			else return Players_real[i + 1]
			end
]]
        else
                local id

                id = get_playerind(i)
                if id == -1 then msg_print(Ind, "Unknown player "..i) return end
                return Players_real[id + 1]
        end
end

-- modify mana
-- returns TRUE if there is a pb
function increase_mana(i, amt)
	local player = players(i)
        player.csp = player.csp + amt
        player.redraw = bor(player.redraw, PR_MANA)
        if (player.csp < 0) then
                player.csp = 0
                return TRUE
        end
        if (player.csp > player.msp) then
                player.csp = player.msp
        end
        return FALSE
end

