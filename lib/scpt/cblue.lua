-- C. Blue admin functions. --
------------------------------

-- Grants a player awareness of all items.
function knowall(name)
    local p, i
    p = ind(name)
    for i = 1, MAX_K_IDX do
	players(p).obj_aware[i] = TRUE;
    end
end

-- Cures all maladies of a player.
function cureall(name)
    local p
    p = ind(name)
    set_afraid(p, 0)
    set_image(p, 0)
    set_poisoned(p, 0)
    set_cut(p, 0)
    set_stun(p, 0)
    set_blind(p, 0)
    set_confused(p, 0)
    set_paralyzed(p, 0)
    set_slow(p, 0)
    do_res_stat(p, A_STR)
    do_res_stat(p, A_CON)
    do_res_stat(p, A_DEX)
    do_res_stat(p, A_WIS)
    do_res_stat(p, A_INT)
    do_res_stat(p, A_CHR)
    restore_level(p)
    set_food(p, PY_FOOD_FULL)
    players(p).black_breath = FALSE
    players(p).csane = player.msane
    players(p).chp = player.mhp
    players(p).chp_frac = 0
end

-- Prepares a character for testing purposes.
function mktest(name)
    local p
    p = ind(name)
    players(p).lev = 50
    players(p).skill_points = 9999
    players(p).score = 1
end

-- Recall all players from the dungeons for urgent server restarts.
-- (Players in towers aren't affected.)
function allrec()
    local p = 0
    for p = 1, NumPlayers do
	if players(p).wpos.wz < 0 then
	    recall(p)
	end
    end
end

-- Set a character's experience according to its level.
-- Also allow a modifier to let it end up just before a certain level.
function setexp(name, modif)
    local p
    p = ind(name)
    players(p).exp = player_exp(players(p).lev - 2) * players(p).exp_fact - modif
end

-- Maximize all skills of a character.
function maxskills(name)
    local p, i
    p = ind(name)
    for i = 0, (MAX_SKILLS - 1) do
	players(p).s_info[i].value = 50000
    end
end

-- Set all uniques/monsters to '1 times killed' for a player.
function allunis(name)
    local p, i
    p = ind(name)
    for i = 1, MAX_R_IDX do
	players(p).r_killed[i] = 1
    end
end

-- Set all uniques/monsters to '0 times killed' for a player.
function nounis(name)
    local p, i
    p = ind(name)
    for i = 1, MAX_R_IDX do
	players(p).r_killed[i] = 0
    end
end

-- Set all uniques/monsters to '100 times killed' for a player.
function allmons(name)
    local p, i
    p = ind(name)
    for i = 1, MAX_R_IDX do
	players(p).r_killed[i] = 1
    end
end
