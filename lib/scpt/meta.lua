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
	--return game, version
	return ""
end

function get_game_version(x)
	local game, version = "unknown", "unknown"
	local i

	for i = 1, getn(x) do
		if x[i].label == "game" then game = x[i][1] end
		if x[i].label == "version" then version = x[i][1] end
	end
	return game, version
end

function get_players(x)
	local players = ""
	local i

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
	local i

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

	-- META_PINGS
	local tmp_line

	for i = 1, getn(x) do
		if type(x[i]) == 'table' then
			if x[i].label == "server" then
				local game, version = get_game_version(x[i])
				local cat_name = game .. " " .. version
				local extra

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
	color_print(line, 0, "\255B" .. meta_name .. "\255B, " .. nb_servers .. " live servers available."); line = line + 3;
	for k, e in categories do
		color_print(line, 0, "\255o" .. e.name .. " :"); line = line + 1

		e = e.servers
		for i = 1, getn(e) do
			tmp_line = line
			color_print(line, 2, "\255G" .. strchar(nb + strbyte('a')) .. ") \255w" .. e[i].name)
			color_print(line, 50, e[i].extra); line = line + 1
			color_print(line, 4, "\255b" .. e[i].players); line = line + 1

			-- Store the info for retrieval -- add <line position> and <'0'> for ping in ms (META_PINGS)
			meta_list[nb] = { e[i].name, e[i].port, e[i].protocol, tmp_line, 0 }
			nb = nb + 1
		end
	end

	line = 1
	if nb_servers == 0 then
		color_print(line, 0, "\255BPress \255RQ\255B to enter a server (IP or hostname) manually or to quit."); line = line + 1
	elseif nb_servers == 1 then
		--color_print(line, 0, "\255BSelect the server with \255Ra"); line = line + 1
		--color_print(line, 0, "\255Bor press \255RQ\255B to enter an IP or hostname manually or to quit."); line = line + 1
		color_print(line, 0, "\255BSelect the server with \255Ra\255B or press \255RQ\255B to enter a server manually or to quit."); line = line + 1
	else
		--local c = string.char(96 + nb_servers)
		local c = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};
		--color_print(line, 0, "\255BSelect a server with \255Ra\255B-\255R" .. c[nb_servers]); line = line + 1
		--color_print(line, 0, "\255Bor press \255RQ\255B to enter an IP or hostname manually or to quit."); line = line + 1
		color_print(line, 0, "\255BSelect a server with \255Ra\255B-\255R" .. c[nb_servers]  .. "\255B or press \255RQ\255B to enter a server manually or to quit."); line = line + 1
	end

	return nb
end

function meta_get(xml_feed, pos)
	-- protocol to be used when connecting the selected server - mikaelh
	server_protocol = meta_list[pos][3]

	return meta_list[pos][1], meta_list[pos][2]
end

-- META_PINGS - set/update ping and display it
function meta_add_ping(pos, ping)
	local attr, unit, spacer

	meta_list[pos][5] = ping
	unit = " ms"

	if ping >= 1000 then spacer = "" -- not possible actually as 1000 ms is used as timeout for the ping commands
	elseif ping >= 100 then spacer = " "
	elseif ping >= 10 then spacer = "  "
	else spacer = "   "
	end

	-- Keep this colour-coding consistent with prt_lagometer()
	if ping == -2 then
		attr = "w"
		ping = "pinging..."
		unit = ""
		spacer = ""
	elseif ping == -1 then
		attr = "R"
		ping = "---"
		unit = ""
		spacer = " "
	elseif ping >= 400 then attr = "r"
	elseif ping >= 300 then attr = "o"
	elseif ping >= 200 then attr = "y"
	elseif ping >= 100 then attr = "g"
	else attr = "G"
	end

	color_print(meta_list[pos][4], 50 + 16, "\255" .. attr .. spacer .. ping .. unit.."       ")

	return 0
end

-- Restore a good neat handler
_ALERT = __old_ALERT
