-- Print errors to stdout
__old_ALERT = _ALERT
_ALERT = print

function color_print(y, x, str)
	Term_putstr(x, y, strlen(str), TERM_WHITE, str)
end

MAX_LEN = 70

function get_server_notes(x)
	local notes = ""

	for i = 1, getn(x) do
		if x[i].label == "notes" then
			notes = x[i][1]
			return notes
		end
	end
	return game, version
end

function get_game_version(x)
	local game, version = "unknown", "unknown"

	for i = 1, getn(x) do
		if x[i].label == "game" then game = x[i][1] end
		if x[i].label == "version" then version = x[i][1] end
	end
	return game, version
end

function get_players(x)
	local players = ""

	for i = 1, getn(x) do
		if x[i].label == "player" then
			if strlen(players..", "..x[i][1]) < MAX_LEN then
				if players ~= "" then
					players = players..", "..x[i][1]
				else
					players = x[i][1]
				end
			else
				return players..", ..."
			end
		end
	end
	return players
end

function get_players_count(x)
	local nb = 0

	for i = 1, getn(x) do
		if x[i].label == "player" then
			nb = nb + 1
		end
	end
	return nb
end

meta_list = {}
function meta_display(xml_feed)
	local x = xml:collect(xml_feed)
	local i, nb, e, k, line, cat_slot
	local categories = {}
	local meta_name = "Unknown metaserver"
	local nb_servers = 0

	for i = 1, getn(x) do
		if type(x[i]) == 'table' then
			if x[i].label == "server" then
				local game, version = get_game_version(x[i])
				local cat_name = game .. " " .. version

				cat_slot = 0
				for k, e in categories do
					if e.name == cat_name then
						cat_slot = k
						break
					end
				end

				if cat_slot == 0 then
					cat_slot = getn(categories) + 1
					categories[cat_slot] = { name = cat_name, servers = {} }
				end

--				local server_notes
--				server_notes = get_server_notes(x[i])

				local extra
				if get_players_count(x[i]) == 1 then
					extra = "\255b" .. get_players_count(x[i]) .. " player"
				else
					extra = "\255b" .. get_players_count(x[i]) .. " players"
				end

				tinsert(categories[cat_slot].servers, {
					name = x[i].args.url,
					port = x[i].args.port,
					protocol = x[i].args.protocol,
					extra = extra,
					players = get_players(x[i]),
				})

				nb_servers = nb_servers + 1
			elseif x[i].label == "meta" then
				meta_name = x[i][1]
			end
		end
	end

	line = 0
	nb = 0
	color_print(line, 0, "\255y" .. meta_name .. " \255Wwith " .. nb_servers .. " connected servers"); line = line + 1; line = line + 1
	for k, e in categories do
		color_print(line, 0, "\255o" .. e.name .. " :"); line = line + 1

		e = e.servers
		for i = 1, getn(e) do
			color_print(line, 2, "\255G" .. strchar(nb + strbyte('a')) .. ") \255w" .. e[i].name)
			color_print(line, 50, e[i].extra); line = line + 1
			color_print(line, 4, e[i].players); line = line + 1

			-- Store the info for retrieval
			meta_list[nb] = { e[i].name, e[i].port, e[i].protocol }
			nb = nb + 1
		end
	end
	line = line + 1
	color_print(line, 0, "\255BSelect a server or press \255RQ\255B to enter an IP or hostname manually."); line = line + 1

	return nb
end

function meta_get(xml_feed, pos)
	-- protocol to be used when connecting the selected server - mikaelh
	server_protocol = meta_list[pos][3]

	return meta_list[pos][1], meta_list[pos][2]
end

-- Restore a good neat handler
_ALERT = __old_ALERT
