-- C. Blue admin functions

-- Grants a player awareness of all items.
function knowall(name)
    local p, i
    p = ind(name)
    for i = 1, MAX_K_IDX do
	players(p).obj_aware[i] = TRUE;
    end
end

-- Cures all maladies of a player
function cureall(name)
    local p
    p = ind(name)
    set_afraid(Ind, 0)
    set_image(Ind, 0)
    set_poisoned(Ind, 0)
    set_cut(Ind, 0)
    set_stun(Ind, 0)
    set_blind(Ind, 0)
    set_confused(Ind, 0)
    set_paralyzed(Ind, 0)
    set_slow(Ind, 0)
    do_res_stat(Ind, A_STR)
    do_res_stat(Ind, A_CON)
    do_res_stat(Ind, A_DEX)
    do_res_stat(Ind, A_WIS)
    do_res_stat(Ind, A_INT)
    do_res_stat(Ind, A_CHR)
    restore_level(Ind)
    set_food(Ind, PY_FOOD_FULL)
    player.black_breath = FALSE
    player.csane = player.msane
    player.chp = player.mhp
    player.chp_frac = 0
end
