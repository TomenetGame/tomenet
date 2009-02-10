-- C. Blue's automatic stuff --
-------------------------------

-- Run automatically on server starting up:
function server_startup(timestamp)
--	lua_add_anote("A server note.")
end

-- Run additionally when 1st player joins since starting up: (character fully loaded at this point)
function first_player_has_joined(num, id, name, timestamp)
end

-- Run additionally if server was currently empty: (character fully loaded at this point)
function player_has_joined_empty_server(num, id, name, timestamp)
end

-- Run automatically if a player joins: (character fully loaded at this point)
function player_has_joined(num, id, name, timestamp)
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
end

-- Run once every hour:
function cron_1h(timestamp, h, m, s)
--	lua_s_print(timestamp.."_CRON1H_"..h..":"..m..":"..s.."\n")

-- Highlander tournament
--	lua_start_global_event(0, 1, "")

-- Arena monster challenge
--	if mod(h,3)==0 then
--		lua_start_global_event(0, 3, "")
--	end
end

-- Run once every 24 hours:
function cron_24h(timestamp, h, m, s)
--	lua_s_print(timestamp.."_CRON24H_"..h..":"..m..":"..s.."\n")
end
