-- Purpose of this file is to provide special lua functions that are called by the
-- quests framework in q_info.txt. As convention to keep things clean, these should
-- all be named 'quest_' and end on the 'internal quest codename'.  - C. Blue

-- Purpose: The Town Elder gives you advice based on your actual current character's details!
function quest_towneltalk(Ind, msg, topic)
	local hinted, i, x

	hinted = 0
	if player.admin_dm ~= 0 then admin = 1 else admin = 0 end

	--tips regarding equipping [prepare|preparing|preparationS],skilling [skillS],
	-- dungeoneering [explore|exploring|exploration],events [eventS],partying [partyING|parties]
	--warn about redundant resses [equipMENT]/ encumberments/hunger/light [status|state]

	if admin == 1 then msg_print(Ind, "Topic #"..topic) end

	--*** generic advice that is so important that it's always given ***
	if topic == -1 then
		--We're dead :p prioritize and discard all other info..
		if player.ghost ~= 0 and admin == 0 then
			msg_print(Ind, "\252\255UOh "..msg..", I have bad news - for you seem to have died and are now a ghost!")
			msg_print(Ind, "\252\255U Seek out the temple (4) or another powerful holy eminence to have you resurrected again!")
			hinted = 1
			--maybe add dual-wield cheap hint for warriors/rangers/HK
		else
			--Black Breath:
			if player.black_breath ~= 0 then
				msg_print(Ind, "\252\255UOh my, "..msg..", you seem to be suffering from a dreadful affliction!")
				msg_print(Ind, "\252\255U You should seek cure immediately. To my knowledge, a sprig of the healing herb Athelas can cure it.")
				msg_print(Ind, "\252\255U Or maybe you happen to know a proficient healer? And I heard that in the city of Gondolin there is herbal healing service available for a fee.")
				hinted = 1
			end

			--Hunger:
			if player.food < 3000 then --PY_FOOD_ALERT
				if hinted == 1 then
					msg_print(Ind, "\252\255UYou also seem to be in dire need of food. I don't have any with me, but if you visit the temple (4), I'm sure they will hand you some.")
				else
					msg_print(Ind, "\252\255U"..msg..", you seem to be in dire need of food. I don't have any with me, but if you visit the temple (4), I'm sure they will hand you some.")
				end
				hinted = 1
			end
			--BpR:
			if player.pclass == CLASS_WARRIOR or player.pclass == CLASS_PALADIN or player.pclass == CLASS_MIMIC or player.pclass == CLASS_ROGUE then
				if player.num_blow < 2 then
					msg_print(Ind, "\252\255UFor a "..msg.." you attack too slowly. Your number of blows per round (BpR) should at least be 2!")
					msg_print(Ind, "\252\255U You should try getting the lightest weapon possible, to remedy this! For example a dagger, whip, or maybe a spear or cleaver if you specialized in those.")
					msg_print(Ind, "\252\255U If this doesn't give you at least 2 BpR, you should really increase your strength, or maybe even your dexterity, as these might just be too low.")
					if player.pclass == CLASS_ROGUE or admin then
						msg_print(Ind, "\252\255U As a rogue a third, easy way is to dual-wield two one-handed weapons at once!")
					end
					hinted = 1
				end
			end
			--Cursed non-artifact equipment on lowbies:
			if player.lev < 20 or admin then
				x = 0
				for i = INVEN_WIELD, INVEN_TOTAL do
					if band(player.inventory[i].ident, 64) ~= 0 then --ID_CURSED
						x = 1
					end
				end
				if x == 1 then
					if hinted == 1 then
						msg_print(Ind, "\252\255UAnd I notice that you apparently have equipped a cursed item. If that was not intentional, you should purchase a scroll of remove curse from the temple (4). As long as it's not a heavy curse it should be broken from that.")
					else
						msg_print(Ind, "\252\255U"..msg..", I notice that you apparently have equipped a cursed item. If that was not intentional, you should purchase a scroll of remove curse from the temple (4). As long as it's not a heavy curse it should be broken from that.")
					end
					hinted = 1
				end
			end
			--Stranger-owned Rings of Power in lowbie inven:
			if player.lev < 25 or admin then
				x = 0
				for i = 1, INVEN_PACK do
					if player.inventory[i].tval == 45 and player.inventory[i].sval == 5 and player.inventory[i].owner ~= player.id then --TV_RING,SV_RING_SPECIAL
						x = 1
					end
				end
				if x == 1 then
					if hinted == 1 then
						msg_print(Ind, "\252\255UThat ring of power in your backpack, you cannot use it as it isn't yours! If you want to get rid of it you will need a scroll of *remove curse*! A normal scroll of remove curse will not suffice as it is heavily cursed!")
					else
						msg_print(Ind, "\252\255UOh "..msg.."! That ring of power in your backpack, you cannot use it as it isn't yours! If you want to get rid of it you will need a scroll of *remove curse*! A normal scroll of remove curse will not suffice as it is heavily cursed!")
					end
					hinted = 1
				end
			end
			--Critical encumberments:
			
		end

		--Give proper question about advice topics
		msg_print(Ind, " ")
		if hinted == 1 then
			msg_print(Ind, "\252\255UIs there anything else you need advice on?")
		else
			--Print a message that looks as if it came straight from q_info.txt, sort of ^^
			msg_print(Ind, "\252\255UHm hm, what do you need advice on?")
		end

		--Don't visit any of the other topics after this, as this '-1' topic is always called automatically when branching to 'advice' keyword
		return
	end

	--*** preparation/inventory ***
	if topic == 0 then
		--encumberment too for MA (and Dodging?)
	end

	--*** equipment ***
	if topic == 1 then
		--resistances, encumberments, redundant flags
		--if we dual-wield a wrong weapon type, tell about one-hand mode
		--maybe add dual-wield cheap hint for warriors/rangers/HK that are naked
	end

	--*** skills ***
	if topic == 2 then
		--if someone has trained spell skills, tell him to actually get the important spell scrolls for those
		--tell him how to create macros
		
	end

	--*** status ***
	if topic == 3 then
		--check for rather low attributes, eg CON, with advice how to improve it. Also stealth. And drained stats.
		
	end

	--*** partying ***
	if topic == 4 then
	end

	--*** dungeon exploration ***
	if topic == 5 then
	end

	--*** events ***
	if topic == 6 then
	end

	if hinted == 0 then
		msg_print(Ind, "\252\255UYou seem to be doing fine, "..msg..".")
	end
end
