function ind(i)
	local id

        id = get_playerind(i)
        if id == -1 then msg_print(Ind, "Unknown player "..i) return id end
        return id
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

