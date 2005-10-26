-- Sample custom.lua for server admins :)

-- Run automatically on server starting up:
function server_startup(timestamp)
	lua_add_anote("Some note...")
end

-- Run additionally when 1st player joins since starting up:
function first_player_has_joined(num, id, name, timestamp)
end

-- Run additionally if server was currently empty:
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
