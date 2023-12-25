-- C. Blue's automatic stuff --
-------------------------------

-- Special features:
	--reset true artifacts every n months (must be one of 1,2,3,4,6)
	art_reset_freq = 6
	censor_swearing = 0

-- Helper variables
	cur_month = -1
	cur_day = -1
	cur_weekday = -1
	cur_year = -1

-- Run automatically on server starting up:
function server_startup(timestamp, h, m, s, dwd, dd, dm, dy)
	lua_s_print(timestamp.."_SERVERSTARTUP_"..h..":"..m..":"..s.."-"..dy.."/"..dm.."/"..dd.."("..dwd..")\n")

	cur_month = dm
	cur_day = dd
	cur_weekday = dwd
	cur_year = dy

	-- Admin parameters */
	watch_cp = 0
	watch_nr = 1
	watch_df = 1
	watch_morgoth = 1

	-- Server changes like rearranging arrays. Should NOT be changed here!
--	updated_server = 0

	-- Modify 'updated_savegame' value for newly created chars [0]
	updated_savegame_birth = 0
end

function server_startup_post(timestamp, h, m, s, dwd, dd, dm, dy)
	lua_s_print(timestamp.."_SERVERSTARTUPPOST_"..h..":"..m..":"..s.."-"..dy.."/"..dm.."/"..dd.."("..dwd..")\n")
end

-- Run automatically on play_game()/sched() starting up:
function playloop_startup(timestamp, h, m, s, dwd, dd, dm, dy)
	lua_s_print(timestamp.."_PLAYLOOPSTARTUP_"..h..":"..m..":"..s.."-"..dy.."/"..dm.."/"..dd.."("..dwd..")\n")

	--[[automatically fix season if required, for example if server
	    was down for 24+ hours while a season change date occured.
	    see cron_24h() for details and to keep values synchronized.]]
	if false then
		-- seasons follow northern hemisphere seasons in real-time

		if (season ~= 0) then -- spring
			-- march + april + may
			if (dm >= 3 and dm <= 5) then
				lua_season_change(0, 0)
			end
		end
		if (season ~= 1) then -- summer
			-- june + july + aug + 22 days
			if ((dm >= 6 and dm <= 8) or (dm == 9 and dd < 23)) then
				lua_season_change(1, 0)
			end
		end
		if (season ~= 2) then -- autumn
			-- 8 days + oct + nov + 21 days
			if ((dm == 9 and dd >= 23) or (dm >= 10 and dm <= 11) or (dm == 12 and dd < 22)) then
				lua_season_change(2, 0)
			end
		end
		if (season ~= 3) then -- winter
			-- 10 days + jan + feb
			if ((dm == 12 and dd >= 22) or (dm < 3)) then
				lua_season_change(3, 0)
			end
		end
	else
		-- seasons run at 2x real-time, and specifically aligning northern-hemisphere-style with christmas during winter
		-- additionally aligned it so that summer solstice / winter solstice are the final days of each cycle ^^

		if (season ~= 0) then -- spring
			-- 7 days + feb + 8 days (march)
			if ((dm == 1 and dd >= 25) or (dm == 2) or (dm == 3 and dd <= 8)) then
				lua_season_change(0, 0)
			end
			-- 7 days + aug + 8 days (sept)
			if ((dm == 7 and dd >= 25) or (dm == 8) or (dm == 9 and dd <= 8)) then
				lua_season_change(0, 0)
			end
		end
		if (season ~= 1) then -- summer
			-- 22 days + april + 4 days (may)
			if ((dm == 3 and dd >= 9) or (dm == 4) or (dm == 5 and dd <= 4)) then
				lua_season_change(1, 0)
			end
			-- 21 days + oct + 4 days (nov)
			if ((dm == 9 and dd >= 9) or (dm == 10) or (dm == 11 and dd <= 4)) then
				lua_season_change(1, 0)
			end
		end
		if (season ~= 2) then -- autumn
			-- 26 days + 21 days (june)
			if ((dm == 5 and dd >= 5) or (dm == 6 and dd <= 21)) then
				lua_season_change(2, 0)
			end
			-- 25 days + 21 days (dec)
			if ((dm == 11 and dd >= 5) or (dm == 12 and dd <= 21)) then
				lua_season_change(2, 0)
			end
		end
		if (season ~= 3) then -- winter
			-- 10 days + 24 days (july)
			if ((dm == 6 and dd >= 22) or (dm == 7 and dd <= 24)) then
				lua_season_change(3, 0)
			end
			-- 10 days + 24 days (january)
			if ((dm == 12 and dd >= 22) or (dm == 1 and dd <= 24)) then
				lua_season_change(3, 0)
			end
		end
	end

--	lua_add_anote("This is how you add server notes.");
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
--[[
	if players(num).updated_savegame == -1 then
		players(num).updated_savegame = 0
		lua_strip_true_arts_from_present_player(num, 1)
	end
]]

	-- Update skill chart layout (leaves skill values and points untouched)
	lua_fix_skill_chart(num)

	-- No costumes after halloween
	if (season_halloween == 0 and season_xmas == 0) then
		lua_takeoff_costumes(num)
	end
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

	if mod(h,2) == 0 then
		lua_start_global_event(0, 1, ">")
	end
	if mod(h,2) == 1 then
		lua_start_global_event(0, 5, ">")
	end

	if mod(h,3) == 0 then
		lua_start_global_event(0, 3, "")
	end
end

-- Run once every 24 hours:
function cron_24h(timestamp, h, m, s, dwd, dd, dm, dy)
	lua_s_print(timestamp.."_CRON24H_"..h..":"..m..":"..s.."-"..dy.."/"..dm.."/"..dd.."("..dwd..")\n")

	--[[season changes (averaged, may in reality vary +/- 1 day:
	    spring 1. march, summer 1. june,
	    autumn 23. september, winter 22. december ]]
	if false then
		-- seasons follow northern hemisphere seasons in real-time

		if (dd == 1 and dm == 3) then lua_season_change(0, 0) end -- spring
		if (dd == 1 and dm == 6) then lua_season_change(1, 0) end -- summer
		if (dd == 23 and dm == 9) then lua_season_change(2, 0) end -- autumn
		if (dd == 22 and dm == 12) then lua_season_change(3, 0) end -- winter
	else
		-- seasons run at 2x real-time, and specifically aligning northern-hemisphere-style with christmas during winter
		-- additionally aligned it so that summer solstice / winter solstice are the final days of each cycle ^^

		if (dd == 25 and dm == 1) then lua_season_change(0, 0) end -- spring A
		if (dd == 25 and dm == 7) then lua_season_change(0, 0) end -- spring B
		if (dd == 9 and dm == 3) then lua_season_change(1, 0) end -- summer A
		if (dd == 9 and dm == 9) then lua_season_change(1, 0) end -- summer B
		if (dd == 5 and dm == 5) then lua_season_change(2, 0) end -- autumn A
		if (dd == 5 and dm == 11) then lua_season_change(2, 0) end -- autumn B
		if (dd == 22 and dm == 6) then lua_season_change(3, 0) end -- winter B
		if (dd == 22 and dm == 12) then lua_season_change(3, 0) end -- winter A
	end

	--regrow fields (gardens) sometimes :/
	if mod(dd, 4) == rand_int(4) then
		lively_wild(8192)
	end

	--Acknowledge +1 day progress of time
	cur_month = dm
	cur_day = dd
	cur_weekday = dwd
	cur_year = dy
end

-- Experimental beta-test features:
function beta(Ind, code)
	--msg_print(Ind, "beta="..code)
end

--For usable custom objects:
function custom_object(Ind, item, dir)
end
--For usable custom objects, emulate k_info diz. Index is 1..n.
function custom_object_diz(index)
end

--[[
 For chests:
  xtra1 = call on acivate trap, xtra2 = call on opening, xtra3: 0x1 = destroy on trap trigger, 0x2 = destroy on ruination by trap trigger, 0x4 = destroy on opening, 0x8 = skip normal trap routines
  xtra4 = number of objects to drop + 1 (ie 1 = no drops!), xtra5 = cash to drop per pile (usual +/-20% random modifier included).
  <index> is xtra1 (for trap) or xtra2 (for open) respectively.
]]
function custom_chest_trap(Ind, index)
end
function custom_chest_open(Ind, index, small, number)
end
