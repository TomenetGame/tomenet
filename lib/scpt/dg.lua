pern_dofile(0, "test.lua")

-- Speak for someone else :)
function say_as(who, what)
        msg_broadcast(0, "{B["..who.."] "..what)
end
