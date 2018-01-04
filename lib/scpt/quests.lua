-- Purpose of this file is to provide special lua functions that are called by the
-- quests framework in q_info.txt. As convention to keep things clean, these should
-- all be named 'quest_' and end on the 'internal quest codename'.  - C. Blue

-- Purpose: The Town Elder gives you advice based on your actual current character's details!
function quest_towneltalk(Ind, msg)
	if player.pclass == CLASS_WARRIOR or player.pclass == CLASS_PALADIN or player.pclass == CLASS_MIMIC or player.pclass == CLASS_ROGUE or player.admin_dm ~= 0 then
		if player.num_blow < 2 then
			msg_print(Ind, "\255UYour number of blows per round (BpR) is too low, it should at least be 2!")
			msg_print(Ind, "\255UYou should try getting the lightest weapon possible, to remedy this! For example a dagger, whip, or maybe a spear or cleaver if you specialized in those.")
			msg_print(Ind, "\255UIf this doesn't give you at least 2 BpR, you should really increase your strength, or maybe even your dexterity, as these might just be too low.")
		else
			msg_print(Ind, "\255UYou seem to be doing fine.")
		end
	else
		msg_print(Ind, "\255UYou seem to be doing fine.")
	end
end
