--state indicator: 2 == stopped/hasnt been run. 1 == running. 3 = blocked.
trivia = 3;
trivia_index = 1;  --question index
tries = 0;  --the number of tries taken thus far.
teleport_all = 0;
last_teleport = 0;

list = {
"What major river must you cross when travelling from Rivendell to Mirkwood? ______", "Anduin",
"What was the name of the sword that Gandalf bore? _________", "Glamdring",
"What was the name of Frodo's father? _____", "Drogo",
"Who did Smeagol murder and take the One Ring from? ______", "Deagol",
"On what date did Bilbo and Frodo share a birthday? _________ ____", "September 22nd",
"On what date were the companions attacked on Weathertop? _______ ___", "October 6th",
"What gift did Gimli request request from Galadriel in Lothlorien? _ ______ ______ __ ___ ____", "A single strand of her hair",
"What Ring of Power did Gandalf wield? _____", "Narya",
"What did Gandalf's Ring of Power give to Men? _______ ___ ________", "courage and resolute",
"What were the last two of the Fellowship to depart from Middle-earth? _____ ___ _______", "Gimli and Legolas",
"What did Gandalf shout as he fell into the abyss in Khazad-dum? ___ ___ _____!", "Fly you fools",
"How many times did Frodo see Black Riders in The Shire? _____", "twice",
"Which chapter is the longest in the Lord of the Rings trilogy? ___ _______ __ ______", "The Council of Elrond",
"How old was Bilbo in the beginning of The Lord of the Rings? ___", "110",
"What relative of Ungoliant the Great hindered Sam and Frodo in Mordor? ______", "Shelob",
"What does Dunedan mean? __________", "Numenorean",
"The main gate of the Lonely Mountain faces in which direction? _____", "South",
"The name of Thorin's father was: ______", "Thrain",
"What did Gandalf call Pippen after he dropped a stone down a well? ____ __ _ ____", "Fool of a Took",
"How many children did Sam Gamgee have? __", "13",
"Who were the three Eagles that rescued Frodo and Sam from Mordor? _______, _________ ___ ________", "Gwaihir, Landroval and Meneldor",
"What do the emblems on the doors of Moria represent? _____, ______, ______", "Durin, Noldor, Feanor",
"Who was the father of Beren? _______", "Barahir",
"What were the names of Farmer Maggot's dogs? ____, ____, ____", "Fang, Wolf, Grip",
"What were the names of the fathers of Merry and Pippen? _______ ___ _______", "Saradoc and Paladin",
"What damage did Frodo do to the Lord of the Nazgul on Weathertop? __ _______ ___ _____ __ ____", "He slashed his cloak in half",
"Who were the two Orc leaders that Hobbit-napped Merry and Pippen? _____ ___ ________", "Ugluk and Grishnak",
"What are the names of Arwen's mother and grandmother? _________, _________, ______", "Celebrian, Galadriel, Elwing",
"Which famous Dwarf was killed by the Watcher in the Water? ___", "Oin",
"What was the name given to Aragorn by Eomer in the fields of Rohan? ________", "Wingfoot",
"What Ring of Power did Galadriel wield? _____", "Nenya",
"What are the two titles of the Lord of the Nazgul? ___ _____ _______ ___ ___ ________ ____ __ ______", "The Black Captain and the Sorcerer King of Angmar",
"What was the name of the coat that was worth more than The Shire? ___ _______ ____", "The Mithril coat",
"What was the name of Merry's pony? ______", "Stybba",
"The father of Anarion and Isildur was Elendil, but who was Elendil's father? _______", "Amandil",
"In The Silmarillion, what hill was Mim's home located on? ____ ____", "Amon Rudh",
"Which Elf battled with Morgoth before the gates of Angband? _________", "Fingolfin",
"Who created the Silmarils and of what race was he? ______ ___ ___", "Feanor the Elf",
"What was Gandalf's favorite type of pipe weed? ___ ____", "Old Toby",
"What are the two names of Aragorn's sword? _______ ___ ______", "Anduril and Narsil",
"How old was Aragorn when he became King of Gondor? __", "88",
"What was the name of Eomer's sword? ________", "Guthwine",
"Who was the last King of Gondor before the War of the Rings? ______", "Earnur",
"How far above the hilt was Aragorn's sword, Anduril, broken? _ ____", "1 foot",
"How many fingers did Gollum have before his demise? __", "11",
"Who was Thorin Oakensheild's sister? ___, ______ __ ____ ___ ____", "Dis, mother of Fili and Kili",
"What animals did Queen Beruthiel keep as pets? ____, ____ _____ ___ ___ _____", "Cats, nine black and one white",
"When was Grond forged? __ ___ _____ ___", "In the First Age",
"Who is the oldest Elf in Lord of the Rings? ______, ____ __ ___ ______", "Cirdan, Lord of the Havens",
"How old is Cirdan, Lord of the Havens? ____ _____ ___", "7000 years old",
"Which Valar overpowered Melkor and brought him to justice before the Council of the Valar? ______", "Tulkas",
"Where did Gil-galad dwell? ______", "Lindon",
"What was the name of Aragorn's son? ________", "Eldarion",
"Who was the father of Gorlim the Unhappy? ______", "Angrim",
"What did Tuor name his ship that he used to sail into the West? _______", "Earrame",
"What are the names of the Istari that entered Middle-earth? ______, ______, ________, ________, ______", "Curumo, Olorin, Aiwendil, Pallando, Alatar",
"Where did the Queen Arwen Evenstar die? _____ ______ __ __________", "Cerin Amroth in Lothlorien",
"What was the Watcher in the Water near the Gates of Moria called? _______", "Kracken",
"Who was Elrond's paternal grandmother? _____", "Idril",
"What was the name of Aragon during his infancy? _____", "Estel",
"What name did Aragorn adopt as his surname? _________", "Telcontar",
"Did Denethor II and Aragorn ever meet? ___", "Yes",
"Who actually saw an Entwife? ___", "Hal",
"How many years had passed since Edoras had been built by the time Gandalf, Aragorn, Legolas, and Gimli arrived together? ___", "500",
"What was the name of the power that gave life to Eru? ___ _____ ____________", "The Flame Imperishable",
"What was the name of Eru, the One, in the Elvish tongue? ________", "Iluvatar",
"What was the surname of Manwe? ______", "Sulimo",
"Who was given the Elvish name Iarwain Ben-Adar? ___ ________", "Tom Bombadil",
"When Tuor goes to Gondolin with Voronwe, who is the Captain of the Guard of the Seven Gates of Gondolin? _________", "Elemmakil",
"What was the name of the sword that Turin used to kill Glaurung? _________", "Anglachel",
"What was the name of the son of Mim who was killed by an arrow fired by one of the company of Turin Turambar? ____", "Khim",
"What shape did Luthien take when she went with Beren to Angband? _____________ ___ _______ ___, _________ __ ______", "Thuringwethil the vampire bat, messenger of Sauron",
"How many witness signatures are needed to certify a legal document in The Shire? _____ ", "seven",
}
MAX_INDEX=72;
trivia_index = 0;
trivia_participants = {};
t_initial_time = 20;
t_time = t_initial_time;

function print_trivia_score() 
	local i, j
	for i,j in trivia_participants do 
		trivbot("\255y"..i.. "\255u has \255y"..trivia_participants[i].."\255u points.") 
	end 
end
function teleport_on() 
	teleport_all = 1; 
end
function teleport_off()
	teleport_all = 0; 
end
function block_trivia()
	trivia = 3;
end
function unblock_trivia()
	trivia = 2;
end
function set_index(n)
	trivia_index = n
end
function get_question()
	return list[2*trivia_index-1];
end
function get_answer()
	return list[2*trivia_index];
end
function trivia_next()
	trivia_index = trivia_index + 1; --random(1,MAX_INDEX);
	if (trivia_index == MAX_INDEX + 1)  then trivia_index = 1 end
	if (trivia == 2) then
		return "Beginning Trivia! Default time to answer is \255y"..t_initial_time.." seconds";
	else
		return "Next question!";
	end
end

function eight_ball(what)
	msg_broadcast(0, "\255y[8ball] "..what);
end 
function eb(what)
	msg_broadcast(0, "\255y[8ball] "..what);
end 
bot_who = ""
function set_bot(who)
	bot_who = who;
end
function bot(what)
--function bot(what) 
--	msg_broadcast(0, "\255U[tBot] "..what);
	local i;
	i = ind(bot_who)
	if (i == -1) then return -1 end
	msg_print(ind(bot_who), "\255g["..who..":tBot] "..what);
end 
function trivbot(what) 
	msg_broadcast(0, "\255u[>] "..what);
end 

function find(what)
	unh = lua_get_last_chat_line();
	return strfind(strlower(unh), strlower(what))
end
function haha(what)
	msg_broadcast(0, "\255u>>\255y"..what);
end	
uptime = 0;
-- gets called every second -- 
function second_handler()
--check_votemute();
	uptime = uptime + 1; -- something fun(ny)
-- start trivia event handler
	if (trivia == 1) then
		t_time = t_time - 1;
		if (t_time <= 0)  then
			t_time = t_initial_time; 
			trivbot("\255U"..get_answer().."\255u was the answer.");
			trivbot(trivia_next());
			trivbot(get_question());
		end
	end 
-- end of trivia event handler
	if (teleport_all == 1)  then
		last_teleport = last_teleport+1;
		if (last_teleport > 30 and ind("God") > -1) then
			last_teleport = 0;
			teleport_ball("God");
		end
	end
end


-- Here are the states required for some of the triggers:
state = 0	--short story state
state_tbot = 0  --tBot's state.
--[[
chat handler. called from dungeon.c when anything interesting was said.
Mind you, "interesting" is defined as whatever a player said in chat in the last turn. X_X ]] 
function chat_handler()
	who = lua_get_last_chat_owner(); 
	what = lua_get_last_chat_line();
	set_bot(who);

	-- Make sure we don't intercept actual pvt msgs!
	local colon = strfind(what,":",1);
	if (colon) then -- Avoid nil arithmetic
		colon = colon -1; 
		colon_target = strsub(what,1,colon);
		if (get_playerind(colon_target) > -1) then
			return;
		end
	end

	if (trivia == 2 and find("^start trivia$")) then
		trivbot(trivia_next());
		trivbot(get_question());
		trivia = 1;
	end
	if (trivia == 3 and find("^start trivia$")) then
		msg_broadcast(0, "\255sTrivia is blocked"); 
	end
	if (trivia == 1 and find(get_answer())) then
		trivbot(who.." got it right.");
		if (trivia_participants[who])  then
			trivia_participants[who] = trivia_participants[who] + (t_initial_time - t_time);
		else 
			trivia_participants[who] = t_initial_time - t_time;
		end
		print_trivia_score();
		t_time = t_initial_time;

		trivbot(trivia_next());
		trivbot(get_question());
	end
	if (trivia == 1 and find("^stop trivia$")) then
		trivia = 2;
		trivbot("Trivia stopped");
		trivia_participants = {};
		ties = 0;
	end

	if (find("^help$")) then
		if (random (3) == 2) then bot ("RTM: tomenet-bin/TomeNET-Guide.txt"); end
	end

	if (find("^uptime$")) then
		days = 0; hours = 0; minutes = 0; tmp = uptime;
	 	while (tmp >= 0) do
			days = days + 1;
			tmp = tmp - 86400;
		end
		tmp = tmp + 86400; days = days - 1;
		while (tmp >= 0) do
			hours = hours + 1;
			tmp = tmp - 3600;
		end
		tmp = tmp + 3600; hours = hours - 1;
		while (tmp >= 0) do
			minutes = minutes + 1;
			tmp = tmp - 60
		end
		tmp = tmp + 60; minutes = minutes -1;
		msg_broadcast(0, "\255sUptime: "..days.." days "..hours.." hours "..minutes.." minutes "..tmp.." seconds");
	end

	if (find("^8ball.*")) then
		chance = random(1,10);
		if (chance == 1) then
			eight_ball ("Yes.");
		elseif (chance == 2) then
		       eight_ball ("No."); 
		elseif (chance == 3) then
		       eight_ball ("Outlook good."); 
		elseif (chance == 4) then
		       eight_ball ("Outlook not so good."); 
		elseif (chance == 5) then
		       eight_ball ("I cannot answer that."); 
		elseif (chance == 6) then
		       eight_ball ("^ is stoned"); 
		elseif (chance == 7) then
		       eight_ball ("Ha ha ha ha."); 
		elseif (chance == 8) then
		       eight_ball ("Y..No."); 
		elseif (chance == 9) then
			eight_ball ("N..Yes.");
		elseif (chance == 10) then
			eight_ball ("Hell if I know.");
		end
	end 
end

-- teleports --name-- to 0, 0
function hh(name)
	player_send(name,0,0,0,"Welcome to the Highlander Tournament")
end

function r(blah) -- short hand for / rumour(blah)
        msg_broadcast(0, "Suddenly a thought comes to your mind:");
        msg_broadcast(0, blah);
end


-- broadcast in the chatting window
function live(what)
	msg_broadcast(0,"\255d[\255v*"..what.."\255v*")
end

-- Speak for someone else (fixed! -- e=everlasting n=normal u=unworldly k=king)
function say_as_e(who, what)
	msg_broadcast(0, "\255B["..who.."] \255B"..what)
end
function say_as_n(who, what)
	msg_broadcast(0, "\255W["..who.."] \255B"..what)
end
function say_as_u(who, what)
	msg_broadcast(0, "\255D["..who.."] \255B"..what)
end
function say_as_k(who, what)
        msg_broadcast(0, "\255v["..who.."] \255B"..what)
end
function say_as(who, what, colour)
	msg_broadcast(0, "\255"..colour.."["..who.."] \255B"..what)
end

function lev0(name)
    p = ind(name);
    players(p).inventory[1].level=0;
    players(p).inventory[2].level=0;
end

-- dump (the updated) skill tree
function dst(name) 
  p = ind(name);
  for i=0, 128 do
    mod = lua_get_skill_mod(p, i);
    val = lua_get_skill_value(p, i);
    if mod > 0 then
      msg_print(Ind, "SKILL "..i.." has mod = "..mod.." and val = "..val);
    end
  end 
end

function tf(name) 
  ball(name, 3, 136, 0, 0, 65536+32767, 500, "");
end
function teleport_ball(name) 
  ball(name, 3, 136, 0, 0, 65536+600, 500, "");
end

