function ind(i)
	local id

        id = get_playerind(i)
        if id == -1 then msg_print(Ind, "Unknown player "..i) return end
        return id
end

function players(i)
        if type(i) == "number" then
                return Players_real[i + 1]
        else
                local id

                id = get_playerind(i)
                if id == -1 then msg_print(Ind, "Unknown player "..i) return end
                return Players_real[id + 1]
        end
end
