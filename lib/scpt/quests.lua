-- Purpose of this file is to provide special lua functions that are called by the
-- quests framework in q_info.txt. As convention to keep things clean, these should
-- all be named 'quest_' and end on the 'internal quest codename'.  - C. Blue

-- Purpose: The Town Elder gives you advice based on your actual current character's details!
function quest_towneltalk(Ind, msg)
	lua_s_print("q-towneltalk: "..msg.."\n")
	msg_print(Ind, "q-towneltalk: "..msg.. " - "..players(Ind).num_blow)
end
