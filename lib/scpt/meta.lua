_ALERT=print

MAX_LEN = 70
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
        local i, nb, e, k, line
        local sorted = {}

        for i = 1, getn(x) do
                if type(x[i]) == 'table' then
                        local game, version = get_game_version(x[i])
                        if not sorted[game.." "..version] then
	                        sorted[game.." "..version] = {}
                        end
                        local extra
                        if get_players_count(x[i]) == 1 then
	                	extra = get_players_count(x[i]).." player"
                        else
	                	extra = get_players_count(x[i]).." players"
                        end
                        tinsert(sorted[game.." "..version], {
                        	name = x[i].args.url,
                        	port = x[i].args.port,
                                extra = extra,
                                players = get_players(x[i]),
                        })
                end
        end

        line = 0
        nb = 0
        for k, e in sorted do
                prt(k.." :", line, 0); line = line + 1
                for i = 1, getn(e) do
        	        prt(strchar(nb + strbyte('a'))..") "..e[i].name, line, 2)
        	        prt(e[i].extra, line, 50); line = line + 1
        	        prt(e[i].players, line, 4); line = line + 1

                        -- Store the info for retrieval
                        meta_list[nb] = { e[i].name, e[i].port }
                        nb = nb + 1
                end
        end

        return nb
end

function meta_get(xml_feed, pos)
        return meta_list[pos][1], meta_list[pos][2]
end
