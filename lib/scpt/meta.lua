function meta_display(xml_feed)
        local x = xml:collect(xml_feed)
        local i, nb

        nb = 0
        for i = 1, getn(x) do
                if type(x[i]) == 'table' then
        	        prt(x[i].args.url, i, 0)
                        nb = nb + 1
                end
        end

        return nb
end

function meta_get(xml_feed, pos)
        local x = xml:collect(xml_feed)
        return x[pos + 1].args.url, x[pos + 1].args.port
end
