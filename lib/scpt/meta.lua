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
	return ""
end

-- check if a server is an official server aka in the static-servers list
function get_server_static(x)
	local static = ""

	for i = 1, getn(x) do
		if x[i].label == "static" then
			static = x[i][1]
			return static
		end
	end
	--default to non-official if no info received
	return "0"
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
	local categories_official = {}
	local categories_unofficial = {}
	local meta_name = "Unknown metaserver"
	local nb_servers = 0

	-- META_PINGS
	local tmp_line

	for i = 1, getn(x) do
		if type(x[i]) == 'table' then
			if x[i].label == "server" then
				local game, version = get_game_version(x[i])
				local cat_name = game .. " " .. version
				local extra = ""
				local server_notes = get_server_notes(x[i])
				local server_static = get_server_static(x[i])
				local maxlen = 48 - strlen(x[i].args.url)

				if strlen(server_notes) >= maxlen then
					server_notes = strsub(server_notes, 1, maxlen)
				end
				if server_notes ~= "" then
					server_notes = " \255s(" .. server_notes .. ") "
				end

				if get_players_count(x[i]) == 1 then
					extra = "\255b" .. get_players_count(x[i]) .. " player"
				else
					extra = "\255b" .. get_players_count(x[i]) .. " players"
				end

				-- sort server into existing category or create a new category if 1st server of a kind

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
				tinsert(categories[cat_slot].servers, {
					name = x[i].args.url,
					port = x[i].args.port,
					protocol = x[i].args.protocol,
					extra = extra,
					players = get_players(x[i]),
					static = server_static,
					notes = server_notes,
				})

				if server_static == "1" then
					cat_slot = 0
					for k, e in categories_official do
						if e.name == cat_name then
							cat_slot = k
							break
						end
					end
					if cat_slot == 0 then
						cat_slot = getn(categories_official) + 1
						categories_official[cat_slot] = { name = cat_name, servers = {} }
					end
					tinsert(categories_official[cat_slot].servers, {
						name = x[i].args.url,
						port = x[i].args.port,
						protocol = x[i].args.protocol,
						extra = extra,
						players = get_players(x[i]),
						static = server_static,
						notes = server_notes,
					})
				else
					cat_slot = 0
					for k, e in categories_unofficial do
						if e.name == cat_name then
							cat_slot = k
							break
						end
					end
					if cat_slot == 0 then
						cat_slot = getn(categories_unofficial) + 1
						categories_unofficial[cat_slot] = { name = cat_name, servers = {} }
					end
					tinsert(categories_unofficial[cat_slot].servers, {
						name = x[i].args.url,
						port = x[i].args.port,
						protocol = x[i].args.protocol,
						extra = extra,
						players = get_players(x[i]),
						static = server_static,
						notes = server_notes,
					})
				end

				nb_servers = nb_servers + 1
			elseif x[i].label == "meta" then
				meta_name = x[i][1]
			end
		end
	end

	line = 0
	nb = 0

	--color_print(line, 0, "\255B" .. meta_name .. "\255B, " .. nb_servers .. " live servers available.")
	--if nb_servers == 0 then line = line + 6
	--else line = line + 4; end

	line = line + 3

	if nb_servers ~= 0 then
		color_print(line, 0, "\255B--- Official servers ---"); line = line + 1
		for k, e in categories_official do
			color_print(line, 0, "\255o " .. e.name .. " :"); line = line + 1
			e = e.servers

			for i = 1, getn(e) do
				tmp_line = line
				color_print(line, 2, "\255G" .. strchar(nb + strbyte('a')) .. ") \255w" .. e[i].name .. e[i].notes)
				color_print(line, 57, e[i].extra); line = line + 1
				color_print(line, 4, "\255b" .. e[i].players); line = line + 1

				-- Store the info for retrieval -- add <line position> and <'0'> for ping in ms (META_PINGS)
				meta_list[nb] = { e[i].name, e[i].port, e[i].protocol, tmp_line, 0 }
				nb = nb + 1
			end
		end

		line = line + 1
		color_print(line, 0, "\255B--- Unofficial servers ---"); line = line + 1
		for k, e in categories_unofficial do
			color_print(line, 0, "\255o " .. e.name .. " :"); line = line + 1
			e = e.servers

			for i = 1, getn(e) do
				tmp_line = line
				color_print(line, 2, "\255G" .. strchar(nb + strbyte('a')) .. ") \255w" .. e[i].name .. e[i].notes)
				color_print(line, 57, e[i].extra); line = line + 1
				color_print(line, 4, "\255b" .. e[i].players); line = line + 1

				-- Store the info for retrieval -- add <line position> and <'0'> for ping in ms (META_PINGS)
				meta_list[nb] = { e[i].name, e[i].port, e[i].protocol, tmp_line, 0 }
					nb = nb + 1
			end
		end
	end

--	line = 1

	line = 0

	if nb_servers == 0 then
		color_print(line, 0, "\255B" .. meta_name .. ". Press \255RQ\255B to enter server IP or hostname manually or to quit."); line = line + 2
		color_print(line, 0, "\255BSince the meta server seems unreachable or reports no servers, you can also try"); line = line + 1
		color_print(line, 0, "\255Blooking in your TomeNET folder for the script files named '\255RTomeNET-direct-xxxx\255B'"); line = line + 1
		color_print(line, 0, "\255Bwhere xxxx denote the region of each script, ie \255oEU\255B for europe, \255oNA\255B for north"); line = line + 1
		color_print(line, 0, "\255Bamerica, \255oAPAC\255B for asia-pacific, and double-click one of them to connect directly"); line = line + 1
		color_print(line, 0, "\255Bto a TomeNET server of the region of your choice, bypassing DNS and meta server."); line = line + 1
		--color_print(line, 0, "\255Bbypassing meta and DNS servers!"); line = line + 1
	elseif nb_servers == 1 then
		color_print(line, 0, "\255B" .. meta_name .. ". Select server \255Ra\255B or press \255RQ\255B for manual input or to quit."); line = line + 1
		color_print(line, 0, "\255B(To connect directly, run a \255RTomeNET-direct-xxxx\255B script instead of TomeNET.exe.)"); line = line + 1
	else
		--local c = string.char(96 + nb_servers)
		local c = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

		--color_print(line, 0, "\255BSelect a server with \255Ra\255B-\255R" .. c[nb_servers]); line = line + 1
		--color_print(line, 0, "\255Bor press \255RQ\255B to enter an IP or hostname manually or to quit."); line = line + 1

		color_print(line, 0, "\255B" .. meta_name .. ". Select server with \255Ra\255B-\255R" .. c[nb_servers]  .. "\255B or press \255RQ\255B for manual input/to quit."); line = line + 1
		color_print(line, 0, "\255B(To connect directly, run a \255RTomeNET-direct-xxxx\255B script instead of TomeNET.exe.)"); line = line + 1
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
		ping = "pinging.."
		unit = ""
		spacer = ""
	elseif ping == -1 then
		attr = "s"
		ping = "unknown"
		unit = ""
		spacer = ""
	elseif ping >= 400 then attr = "r"
	elseif ping >= 300 then attr = "o"
	elseif ping >= 200 then attr = "y"
	elseif ping >= 100 then attr = "g"
	else attr = "G"
	end

	color_print(meta_list[pos][4], 50 + 20, "\255" .. attr .. spacer .. ping .. unit .. "       ")

	return 0
end

-- Restore a good neat handler
_ALERT = __old_ALERT
