-- C. Blue's automatic stuff --
-------------------------------

-- Run automatically on server starting up:
function server_startup(timestamp, h, m, s, dwd, dd, dm, dy)
	lua_s_print(timestamp.."_SERVERSTARTUP_"..h..":"..m..":"..s.."-"..dy.."/"..dm.."/"..dd.."("..dwd..")\n")

        --[[automatically fix season if required, for example if server 
            was down for 24+ hours while a season change date occured. 
            see cron_24h() for details and to keep values synchronized.]]
        if (season ~= 0) then
                if ((dm == 3 and dd >= 20) or (dm > 3 and dm < 6) or (dm == 6 and dd < 21)) then
                        lua_season_change(0, 0)
                end
        end
        if (season ~= 1) then
                if ((dm == 6 and dd >= 21) or (dm > 6 and dm < 9) or (dm == 9 and dd < 23)) then
                        lua_season_change(1, 0)
                end
        end
        if (season ~= 2) then
                if ((dm == 9 and dd >= 23) or (dm > 9 and dm < 12) or (dm == 12 and dd < 21)) then
                        lua_season_change(2, 0)
                end
        end
        if (season ~= 3) then
                if ((dm == 12 and dd >= 21) or (dm < 3) or (dm == 3 and dd < 20)) then
                        lua_season_change(3, 0)
                end
        end

	lua_add_anote("{v---- Client 4.4.2 released (see forum)! ----");

-- Admin parameters */
	watch_nr = 1
	watch_morgoth = 1

-- Modify 'updated_savegame' value for newly created chars [0]
	updated_savegame_birth = 0

-- Update spellbooks if one was added/removed (part 1 of 2)
	if updated_server == -1 then
		lua_fix_spellbooks(MCRYOKINESIS + 1, -1)
		updated_server = 1
	end
end

-- Run additionally when 1st player joins since starting up: (character fully loaded at this point)
function first_player_has_joined(num, id, name, timestamp)
end

-- Run additionally if server was currently empty: (character fully loaded at this point)
function player_has_joined_empty_server(num, id, name, timestamp)
end

-- Run automatically if a player joins: (character fully loaded at this point)
function player_has_joined(num, id, name, timestamp)
-- Reset true artifacts for this player (after '/art reset!').
-- Note: No additional changes anywhere are required for artifact reset.
	if players(num).updated_savegame == -1 then
		players(num).updated_savegame = 0
		lua_strip_true_arts_from_present_player(num, 1)
	end

-- Update spellbooks if one was added/removed (part 2 of 2)
	if players(num).updated_savegame == -1 then
		fix_spellbooks(name, MCRYOKINESIS + 1, -1)
		players(num).updated_savegame = 1
	end

-- Update skill chart layout (leaves skill values and points untouched)
	lua_fix_skill_chart(num)
end

-- Run automatically when a player leaves the server:
function player_leaves(num, id, name, timestamp)
end

-- Run when a player leaves, but char stays in dungeon, waiting for timeout:
function player_leaves_timeout(num, id, name, timestamp)
end

-- Run additonally when a player has left the server (char saved and gone):
function player_has_left(num, id, name, timestamp)
end

-- Run additionally when the last player has left and server becomes empty:
function last_player_has_left(num, id, name, timestamp)
--	clean up all towns to get rid of (nothing)s
--	lua_s_print("Cleaning towns.\n")
--	lua_towns_treset()
end

-- Run once every hour:
function cron_1h(timestamp, h, m, s)
	lua_s_print(timestamp.."_CRON1H_"..h..":"..m..":"..s.."\n")

	lua_start_global_event(0, 1, "")

	if mod(h,3)==0 then
		lua_start_global_event(0, 3, "")
	end
end

-- Run once every 24 hours:
function cron_24h(timestamp, h, m, s, dwd, dd, dm, dy)
	lua_s_print(timestamp.."_CRON24H_"..h..":"..m..":"..s.."-"..dy.."/"..dm.."/"..dd.."("..dwd..")\n")
	
	--[[season changes (averaged, may in reality vary +/- 1 day:
	    spring 20.march, summer 21.june,
	    autumn 23. september, winter 21. december ]]
	if (dd == 20 and dm == 3) then lua_season_change(0, 0) end
	if (dd == 21 and dm == 6) then lua_season_change(1, 0) end
	if (dd == 23 and dm == 9) then lua_season_change(2, 0) end
	if (dd == 21 and dm == 12) then lua_season_change(3, 0) end
end
