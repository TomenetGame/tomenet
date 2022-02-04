-- Purpose of this file is to provide special lua functions that are called by the
-- quests framework in q_info.txt. As convention to keep things clean, these should
-- all be named 'quest_' and end on the 'internal quest codename'.  - C. Blue

-- Purpose: The Town Elder gives you advice based on your actual current character's details!
function quest_towneltalk(Ind, msg, topic)
	local hinted, hintsub, i, w, x, y, z

	hinted = 0
	hintsub = 0 --subsequent hints that chain grammatically for nice flow of text
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
					if player.pclass == CLASS_ROGUE or admin == 1 then
						msg_print(Ind, "\252\255U As a rogue a third, easy way is to dual-wield two one-handed weapons at once!")
					end
					hinted = 1
				end
			end
			--Cursed non-artifact equipment on lowbies:
			if player.lev < 20 or admin == 1 then
				x = 0
				for i = INVEN_WIELD, INVEN_TOTAL - 1 do
					if band(player.inventory[i + 1].ident, 64) ~= 0 then --ID_CURSED
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
			if player.lev < 25 or admin == 1 then
				x = 0
				for i = 1, INVEN_PACK do
					if player.inventory[i + 1].tval == 45 and player.inventory[i + 1].sval == 5 and player.inventory[i + 1].owner ~= player.id then --TV_RING,SV_RING_SPECIAL
						x = 1
					end
				end
				if x == 1 then
					if hinted == 1 then
						msg_print(Ind, "\252\255UAlso, about that Ring of Power in your backpack, you cannot use it as it isn't yours! If you want to get rid of it you will need a scroll of *remove curse*! A normal scroll of remove curse will not suffice as it is heavily cursed!")
					else
						msg_print(Ind, "\252\255UOh "..msg.."! That Ring of Power in your backpack, you cannot use it as it isn't yours! If you want to get rid of it you will need a scroll of *remove curse*! A normal scroll of remove curse will not suffice as it is heavily cursed!")
					end
					hinted = 1
				end
			end
			--Critical encumberments:
		end

		--Give proper question about advice topics
		--msg_print(Ind, " ")
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
		if player.monk_heavyarmor == 1 then
			msg_print(Ind, "\252\255UIt seems your armour weight negatively impacts your martial arts performance, hindering your abilities!");
			hinted = 1
		end
		if player.rogue_heavyarmor == 1 then
			msg_print(Ind, "\252\255UIt seems your armour weight negatively impacts your flexibility and awareness, hindering your abilities!");
			if player.inventory[INVEN_WIELD + 2].k_idx ~= 0 and player.inventory[INVEN_WIELD + 2].tval ~= 34 then -- INVEN_ARM+1, TV_SHIELD
				msg_print(Ind, "\252\255UBe aware that your secondary weapon will count as NON-EXISTANT while you are encumbered this way! Meaning that you won't get any abilities or resistances from it either!");
			end
			hinted = 1
		end

		--suggest phase/heals etc?
		w = 0
		x = 0
		y = 0
		z = 0
		if player.lev < 10 then
			--nothing yet
		elseif player.lev < 20 then
			for i = 0, INVEN_PACK do
				--check for escapes
				if player.inventory[i + 1].tval == 55 then --TV_STAFF
					--SV_STAFF_TELEPORTATION
					if player.inventory[i + 1].sval == 4 then
						x = 1
					end
				end
				if player.inventory[i + 1].tval == 70 then --TV_SCROLL
					--SV_SCROLL_PHASE_DOOR, SV_SCROLL_TELEPORT
					if player.inventory[i + 1].sval == 8 or player.inventory[i + 1].sval == 9 then
						x = 1
					end
				end

				--check for heals
				if player.inventory[i + 1].tval == 80 then --TV_FOOD
					--SV_FOOD_CURE_SERIOUS
					if player.inventory[i + 1].sval == 16 then
						y = 1
					end
				end
				if player.inventory[i + 1].tval == 55 then --TV_STAFF
					--SV_STAFF_CURE_SERIOUS
					if player.inventory[i + 1].sval == 16 then
						y = 1
					end
				end
				if player.inventory[i + 1].tval == 71 then --TV_POTION
					 --SV_POTION_CURE_SERIOUS, SV_POTION_CURE_CRITICAL, SV_POTION_HEALING
					if player.inventory[i + 1].sval == 35 or player.inventory[i + 1].sval == 36 or player.inventory[i + 1].sval == 38 then
						y = 1
					end
				end
			end

			hintsub = 0
			if x == 0 then
				if hinted == 1 then
					msg_print(Ind, "\252\255UIf you don't have means of escape, you should buy scrolls of phase door from the alchemist (the blue '5') in town.")
				else
					msg_print(Ind, "\252\255UOh "..msg..", if you don't have means of escape, you should buy scrolls of phase door from the alchemist (the blue '5') in town.")
				end
				hinted = 1
				hintsub = 1
			end
			if y == 0 then
				if hinted == 1 then
					if hintsub == 1 then
						msg_print(Ind, "\252\255ULikewise if you don't have healing spells available, you should buy potions to cure at least serious wounds from the temple (the green '4') in town.")
					else
						msg_print(Ind, "\252\255UIf you don't have healing spells available, you should buy potions to cure at least serious wounds from the temple (the green '4') in town.")
					end
				else
					msg_print(Ind, "\252\255UOh "..msg..", if you don't have healing spells available, you should buy potions to cure at least serious wounds from the temple (the green '4') in town.")
				end
				hinted = 1
				hintsub = 1
			end
		elseif player.lev < 30 then
			for i = 0, INVEN_PACK do
				--check for escapes
				if player.inventory[i + 1].tval == 55 then --TV_STAFF
					--SV_STAFF_TELEPORTATION
					if player.inventory[i + 1].sval == 4 then
						x = 1
						z = 1
					end
				end
				if player.inventory[i + 1].tval == 70 then --TV_SCROLL
					--SV_SCROLL_PHASE_DOOR, SV_SCROLL_TELEPORT
					if player.inventory[i + 1].sval == 8 then
						x = 1
					end
					if player.inventory[i + 1].sval == 9 then
						x = 1
						z = 1
					end
				end

				--check for heals
				if player.inventory[i + 1].tval == 71 then --TV_POTION
					 --SV_POTION_CURE_CRITICAL, SV_POTION_HEALING
					if player.inventory[i + 1].sval == 36 or player.inventory[i + 1].sval == 38 then
						y = 1
					end
				end
			end

			hintsub = 0
			if x == 0 then
				if hinted == 1 then
					msg_print(Ind, "\252\255UIf you don't have means of escape, you should buy scrolls of phase door from the alchemist (the blue '5') in town.")
				else
					msg_print(Ind, "\252\255UOh "..msg..", if you don't have means of escape, you should buy scrolls of phase door from the alchemist (the blue '5') in town.")
				end
				hinted = 1
				hintsub = 1
			end
			if z == 0 then
				if hinted == 1 then
					if hintsub == 1 then
						msg_print(Ind, "\252\255UAnd if you can afford it you should buy a scroll or at least a staff of teleportation to get out of more serious trouble easily.")
					else
						msg_print(Ind, "\252\255UIf you can afford it you should buy a scroll or at least a staff of teleportation to get out of more serious trouble easily.")
					end
				else
					msg_print(Ind, "\252\255UOh "..msg..", if you can afford it you should buy a scroll or at least a staff of teleportation to get out of more serious trouble easily.")
				end
				hinted = 1
				hintsub = 1
			end
			if y == 0 then
				if hinted == 1 then
					if hintsub == 1 then
						msg_print(Ind, "\252\255ULikewise if you don't have healing spells available, you should buy potions of cure critical wounds from the temple (the green '4') in town.")
					else
						msg_print(Ind, "\252\255UIf you don't have healing spells available, you should buy potions of cure critical wounds from the temple (the green '4') in town.")
					end
				else
					msg_print(Ind, "\252\255UOh "..msg..", if you don't have healing spells available, you should buy potions of cure critical wounds from the temple (the green '4') in town.")
				end
				hinted = 1
				hintsub = 1
			end
		else -- lev 30+
			hintsub = 0
			for i = 0, INVEN_PACK do
				--check for escapes
				if player.inventory[i + 1].tval == 70 then --TV_SCROLL
					--SV_SCROLL_TELEPORT
					if player.inventory[i + 1].sval == 9 then
						x = 1
						if player.inventory[i + 1].sval == 9 and player.inventory[i + 1].number < 5 then
							if hinted == 1 then
								msg_print(Ind, "\252\255UYour stack of teleportation scrolls is running dangerously low, you should buy more!")
							else
								msg_print(Ind, "\252\255UOh "..msg..", your stack of teleportation scrolls is running dangerously low, you should buy more!")
							end
							hinted = 1
							hintsub = 1
						end
					end
				end
				--check for heals
				if player.inventory[i + 1].tval == 71 then --TV_POTION
					 --SV_POTION_HEALING
					if player.inventory[i + 1].sval == 38 then
						y = 1
						if player.inventory[i + 1].sval == 9 and player.inventory[i + 1].number < 10 then
							if hinted == 1 then
								if hintsub == 1 then
									msg_print(Ind, "\252\255UYour supply of healing potions is running dangerously low, too!")
								else
									msg_print(Ind, "\252\255UYour supply of healing potions is running dangerously low, you should buy more!")
								end
							else
								msg_print(Ind, "\252\255UOh "..msg..", your supply of healing potions is running dangerously low, you should buy more!")
							end
							hinted = 1
							hintsub = 1
						end
					end
				end
				--check for speed
				if player.inventory[i + 1].tval == 71 then --TV_POTION
					 --SV_POTION_SPEED
					if player.inventory[i + 1].sval == 29 then
						z = 1
						if player.inventory[i + 1].sval == 9 and player.inventory[i + 1].number < 6 then
							if hinted == 1 then
								if hintsub == 1 then
									msg_print(Ind, "\252\255USame goes for your supply of speed potins!")
								else
									msg_print(Ind, "\252\255UYour supply of speed potions is running dangerously low, you should buy more!")
								end
							else
								msg_print(Ind, "\252\255UOh "..msg..", your supply of speed potions is running dangerously low, you should buy more!")
							end
							hinted = 1
							hintsub = 1
						end
					end
				end
				--check for resist
				if player.inventory[i + 1].tval == 71 then --TV_POTION
					 --SV_POTION_RESISTANCE
					if player.inventory[i + 1].sval == 60 then
						w = 1
						if player.inventory[i + 1].sval == 9 and player.inventory[i + 1].number < 5 then
							if hinted == 1 then
								if hintsub == 1 then
									msg_print(Ind, "\252\255UAnd your supply of resistance potions needs restocking as well!")
								else
									msg_print(Ind, "\252\255UYour supply of resistance potions is running dangerously low, you should buy more!")
								end
							else
								msg_print(Ind, "\252\255UOh "..msg..", your supply of resistance potions is running dangerously low, you should buy more!")
							end
							hinted = 1
							hintsub = 1
						end
					end
				end
			end

			hintsub = 0
			if x == 0 then
				if hinted == 1 then
					msg_print(Ind, "\252\255UYou should buy scrolls of teleportation from the black market to be able to get out of bigger trouble fast.")
				else
					msg_print(Ind, "\252\255UOh "..msg..", you should buy scrolls of teleportation from the black market to be able to get out of bigger trouble fast.")
				end
				hinted = 1
				hintsub = 1
			end
			if y == 0 then
				if hinted == 1 then
					if hintsub == 1 then
						--msg_print(Ind, "\252\255ULikewise if you don't have healing spells available, you should buy potions of healing there too.")
						msg_print(Ind, "\252\255ULikewise, if you aren't a master healer, you should buy potions of healing there too.")
						if get_skill_value(Ind, SKILL_NECROMANCY) >= 10 then
							msg_print(Ind, "\252\255U I can sense that you are adept in Necromancy, but that alone is not enough!")
						end
					else
						--msg_print(Ind, "\252\255UIf you don't have healing spells available, you should buy potions of healing from the black market.")
						msg_print(Ind, "\252\255UIf you aren't a master healer, you should buy potions of healing from the black market.")
						if get_skill_value(Ind, SKILL_NECROMANCY) >= 10 then
							msg_print(Ind, "\252\255U I can sense that you are adept in Necromancy, but that alone is not enough!")
						end
					end
				else
					--msg_print(Ind, "\252\255UOh "..msg..", if you don't have healing spells available, you should buy potions of healing from the black market.")
					msg_print(Ind, "\252\255UOh "..msg..", if you aren't a master healer, you should buy potions of healing from the black market.")
					if get_skill_value(Ind, SKILL_NECROMANCY) >= 10 then
						msg_print(Ind, "\252\255U I can sense that you are adept in Necromancy, but that alone is not enough!")
					end
				end
				hinted = 1
				hintsub = 1
			end
			if z == 0 then
				if hinted == 1 then
					if hintsub == 1 then
						msg_print(Ind, "\252\255UAlso if you don't have speed spells available, you should buy potions of speed there too.")
					else
						msg_print(Ind, "\252\255UIf you don't have speed spells available, you should buy potions of speed from the black market.")
					end
				else
					msg_print(Ind, "\252\255UOh "..msg..", if you don't have speed spells available, you should buy potions of speed from the black market.")
				end
				hinted = 1
				hintsub = 1
			end
			if w == 0 then
				if hinted == 1 then
					if hintsub == 1 then
						msg_print(Ind, "\252\255UAnd if you don't have resistance spells available, you should buy potions of resistance there too.")
					else
						msg_print(Ind, "\252\255UIf you don't have resistance spells available, you should buy potions of resistance from the black market.")
					end
				else
					msg_print(Ind, "\252\255UOh "..msg..", if you don't have resistance spells available, you should buy potions of resistance from the black market.")
				end
				hinted = 1
				hintsub = 1
			end
		end
	end

	--*** equipment ***
	if topic == 1 then
		--resistances
		if player.lev >= 29 or admin == 1 then
			if player.free_act == 0 then
				msg_print(Ind, "\252\255UYou should really make sure you have Free Action, or you might get paralzyed by a monster and become unable to defend yourself or flee!")
				hinted = 1
			end
			if player.resist_nexus == 0 then
				msg_print(Ind, "\252\255UYou probably want nexus resistance soon, because nexus can swap two of your attributes randomly, which can mean very, very serious trouble.")
				hinted = 1
			end
			if player.resist_pois == 0 then
				msg_print(Ind, "\252\255UYou should look for poison resistance. I heard that Amulets of the Serpents are relatively easy to acquire for that need.")
				hinted = 1
			end
			if (player.resist_fire == 0 and player.immune_fire == 0) or (player.resist_cold == 0 and player.immune_cold == 0) or (player.resist_acid == 0 and player.immune_acid == 0) or (player.resist_elec == 0 and player.immune_elec == 0) then
				msg_print(Ind, "\252\255UYou definitely want resistance to the four basic elements, fire and cold, acid and lightning!")
				hinted = 1
			end
		elseif player.lev >= 23 or admin == 1 then
			if player.free_act == 0 then
				msg_print(Ind, "\252\255UYou might want to look out for Free Action, or you might get paralzyed by a monster and become unable to defend yourself or flee!")
				hinted = 1
			end
			if (player.resist_fire == 0 and player.immune_fire == 0) or (player.resist_cold == 0 and player.immune_cold == 0) or (player.resist_acid == 0 and player.immune_acid == 0) or (player.resist_elec == 0 and player.immune_elec == 0) then
				msg_print(Ind, "\252\255UYou probably want to complete your array of resistances to the four basic elements, fire and cold, acid and lightning.")
				hinted = 1
			end
		elseif player.lev >= 15 or admin == 1 then
			if player.resist_fire == 0 then
				msg_print(Ind, "\252\255UYou might want to look for something that provides fire resistance. Quite useful to have early on.")
				hinted = 1
			end
		end
		--encumberments
		if player.monk_heavyarmor == 1 then
			msg_print(Ind, "\252\255UIt seems your armour weight negatively impacts your martial arts performance, hindering your abilities!");
			hinted = 1
		end
		if player.rogue_heavyarmor == 1 then
			msg_print(Ind, "\252\255UIt seems your armour weight negatively impacts your flexibility and awareness, hindering your abilities!");
			if player.inventory[INVEN_WIELD + 2].k_idx ~= 0 and player.inventory[INVEN_WIELD + 2].tval ~= 34 then -- INVEN_ARM+1, TV_SHIELD
				msg_print(Ind, "\252\255UBe aware that your secondary weapon will count as NON-EXISTANT while you are encumbered this way! Meaning that you won't get any abilities or resistances from it either!");
			end
			hinted = 1
		end
		--redundant flags
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
		msg_print(Ind, "\252\255UGo to the town hall, the largest building in Bree! The right side entrance is the mathom house. There you can see a list of all known dungeons and also find out which ones haven't been explored recently, giving you bonus XP!")
		hinted = 1
	end

	--*** events ***
	if topic == 6 then
		msg_print(Ind, "\252\255UTo check for ongoing events, type '/evinfo' into chat. Events take place regularly every 1 or 2 hours.")
		hinted = 1
	end

	if hinted == 0 then
		msg_print(Ind, "\252\255UYou seem to be doing fine, "..msg..".")
	end
end
