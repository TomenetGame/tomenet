-- C. Blue admin functions

-- Grants a player awareness of all items.
function knowall(name)
    local p, i
    p = ind(name)
    for i = 1, MAX_K_IDX do
	players(p).obj_aware[i] = TRUE;
    end
end
