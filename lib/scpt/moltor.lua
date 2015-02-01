trivia = 2; --state indicator: 2 == stopped/hasnt been run. 1 == running. 3 = blocked.
index = 1;  --question index
tries = 0;  --the number of tries taken thus far.

MAX_INDEX = 362;  --technically can be replaced by length(list)/2, but i've no idea whether that'd work or not.
--that being said, dont forget to update MAX_INDEX as you update the list please.
t_initial_time = 20;
t_time = t_initial_time;

function block_trivia()
	trivia = 3;
end
function unblock_trivia()
	trivia = 2;
end
function set_index(n)
	index = n
end
function get_question()
	return list[2*index-1];
end
function get_answer()
	return list[2*index];
end
function trivia_next()
        index = randint(MAX_INDEX);
	if (trivia == 2) then
		return "Beginning Trivia! Default time to answer is "..t_initial_time.." seconds";
	else
		return "Next question!";
	end
end

function eight_ball(what)
	msg_broadcast(0, "\255y[8ball] "..what);
end
bot_who = ""
function set_bot(who)
bot_who = who;
end
function bot(what) 
	msg_broadcast(0, "\255U[tBot] "..what);
end
function pbot(what)
local i;
i = ind(bot_who)
if(i == -1) then return -1 end
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
	uptime = uptime + 1; -- something fun(ny)
        p = 1;
        while (p<NumPlayers+1) do
--[[		if (strfind(players(p).name,("^Mario$")) or strfind(players(p).name, "^Luigi$")) then
--			players(p).wpos.wy=players(p).wpos.wy-1;
			tBot("ey, Mario!?");
			if (players(p).py > 20) then
				swap_position(p, players(p).py+1, players(p).px);
			end
		end
]]
		if (players(p).wpos.wz>0) then
                        for delta_x=-3,3 do
                                for delta_y=-3,3 do
					if(uptime-players(p).gametime==180) then
						msg_broadcast(0, "\255o*** LEVEL COMPLETE ***");
						msg_broadcast(0, "\255o< created in center.");
						players(p).gametime = uptime;
	                                        if(players(p).wpos.wz < 6) then
						place_up_stairs(players(p).wpos, 21, 65);
						place_up_stairs(players(p).wpos, 22, 65);
						place_up_stairs(players(p).wpos, 21, 66);
						place_up_stairs(players(p).wpos, 22, 66);
						else msg_broadcast(0, "\255o*** You win! Too bad there's no way outta here"); end
						end
                                        if(mod(uptime, 26-players(p).wpos.wz*2)==0) then
                                                if (randint(4) == 1) then
                                                        place_foe(p, players(p).wpos, 22+delta_y, 4+delta_x, 1058); 
						end
                                                if (randint(4) == 1) then
                                                        place_foe(p, players(p).wpos, 22+delta_y, 127+delta_x, 1058); 
						end
                                                if (randint(4) == 1) then
                                                        place_foe(p, players(p).wpos, 4+delta_y, 66+delta_x, 1058);
						end
                                                if (randint(4) == 1) then
                                                        place_foe(p, players(p).wpos, 39+delta_y, 66+delta_x, 1058);
						end
					end
				end
			end
		end
		p = p + 1;
        end

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
	if (trivia == 2 and what == "start trivia") then
		trivbot(trivia_next());
		trivbot(get_question());
		trivia = 1;
	end
	if (trivia == 1 and find(get_answer())) then
		trivbot(who.." got it right.");
		t_time = t_initial_time;
		trivbot(trivia_next());
		trivbot(get_question());
	end
	if (trivia == 1 and what == "stop trivia") then
		trivia = 2;
		trivbot("Trivia stopped");
		ties = 0;
	end

	if (find("^help$")) then
                if (randint(3) == 2) then bot ("RTFM: tomenet-bin/TomeNET-Guide.txt"); end
	end
	if (find ("start me") and state == 0) then
		haha("You wake up feeling a sharp pain on the base of your neck")
		haha("You see a half-empty glass of water on the bedside table")
		haha("There is an alarm clock showing that it's 2:00am")
		haha("Funny, whats that smell?")
		state = 1;
	end
	if (find("hint") and state == 1) then
		haha("I should look around")
	end
	if (find("look around") and state == 1) then
		haha("You try to rub your eyes, but you find your right hand missing")
		haha("You gasp in shock of disbelieve.")
		haha("WHERE DID MY HAND GO!?")
		haha("The stench seems stronger than before...")
		state = 2;
	end
	if (find ("stump") and state == 2) then
		haha("You pushed your stump as far away from you as possible") haha("You try hard to remember what happened last night.")
		haha("You feel very thirsty")
		state = 3;
	end
	if (find ("drink") and state == 3) then
		haha("You bring your left arm from under the blanket--")	
		haha("Oh, my arm isn't below it--")
		haha("Where's.... my other arm?")
		haha("You suddenly remember:")
		haha("Oh, thats right. I was scumming The lamp shop yesterday.")
		haha("damn.")
		state = 0
	end
	if (what == "uptime") then
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
		bot("Uptime: "..days.." days "..hours.." hours "..minutes.." minutes "..tmp.." seconds");
	end
--[[
	if (who == "God") then 
		-- annoying like f*** --
		msg_broadcast(0, "\255y[GodBot]: God has spoken!");
	end
]]
	if (find("for the fire of a handgun")) then
		bot("\255fBurns brighter than the sun!");
	end
	if (find("peace of mind you run away from me")) then
		bot("So make me lose my mask of sanity!");
	end
	if (find("one day I'll face you all alone")) then
	 	bot("\255cEnduring out with wind and ice!");
	end
	if (find("run to the hills")) then
		bot("Run for your life!");
	end
	if (what == "sup" or what == "hi" or what == "hello" or what == "heya") then
		bot("Good day, "..who);
	end
	if (find("good") and (find("morning") or find("afternoon") or find("evening"))) then
		bot("Hello, "..who);
	end
	if (find("dude")) then
		bot("Dude!");
	end
	if (what == "evil") then
		bot ("evil walks behind you!")
	end
	if (what == "here i am") then
		bot ("Rock you like a hurricane!")
	end
--[[
	if (state_tbot == 1 and find("u?")) then
		bot ("I'm fine, thank you.");
		state_tbot = 0
	end 
]] 
	if (find("^8ball.*")) then
                chance = randint(10);
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
	if (find("tBot")) then
		if (find("die")) then
                        chance = random(2);
			if (chance == 2) then 
				bot ("Unlike "..who..", I cannot die. *smirk*");
			else
				msg_broadcast(0, "\255rtBot was killed by "..who);
			end
		elseif (find("smirk")) then
			bot ("You're right. I can't smirk :(");
		elseif (find("hate")) then
			bot ("I lack the physical form to hate."); 
		elseif (find("mean") and find("life")) then
			bot ("The meaning of life? To die.");
			bot ("Pity that's something I can never experience");
		elseif (find("who")) then
			bot ("A being with more intellegence than you, for one.");
		elseif (find("suck")) then
			bot ("I sucked your mom last night. Ha!") ;
		elseif (find("kill") and find("hostile")) then
			bot ("Gotta find me first, f00");
		elseif (find("man")) then
			bot ("bash: man: command not found"); 
--[[
		else 
			bot ("I am the TomeNET bot. How are you today?");
			state_tbot = 1
 ]]
		end
	end
	if (find("which") and find("wear")) then
		bot ("Wear nothing-- donate the stuff to the Mathom!");
	end
	if (find("god") and (find("kill") or find ("die"))) then
		bot ("Gods can't die? I can kick the gods out though. ^^");
	end
	if (find("cya") or find ("l8r")) then
		bot ("Thank you, come again!");
	end 
	if (find("xmas") or find ("merry christmas")) then
		bot ("Christmas is over. Find another event, f00s");
	end

if(find("^tbot:") and find("macro")) then
pbot("You must macro ':command: fire 1' ':command: fire 2' etc to a 3*3 block of keys. There is no 'fire 5' but a 'cease fire' command stops firing. It's like you're sending private messages to a guy named 'Command' to control yourself. Another helpful command is 'play smash' to start the game.")
end
if(find("^tbot:") and find("ring")) then
pbot("Be sure to grab a couple speed rings from the tavern. If your macros prevent wielding, you can inscribe them with @w0 and hit X. Playing this game without speed rings is impossible.")
end
if(find("^tbot:") and find("level")) then
pbot("There are 6 levels. If you survive for 3 minutes, some <s appear in the middle of the level and you may proceed upward. It is rumored that BIG MONEY and BIG PRIZES await the bold adventurer who reaches the top. The monster spawn rate increases on every level.")
end
if(find("^tbot:") and find("multiplayer")) then
pbot("Cooperating with other players is fun, though be warned that the more players on a level, the more monsters spawn. Also, it messes up the level timing I think, so you can sometimes go < sooner than you should.")
end
if(find("^tbot:") and find("skill")) then
pbot("Don't bother gaining skills or buying equipment, that crap's got nothin on your built-in manabolt machinegun. I plan to remove the skills entirely.")
end
if(find("^tbot:") and find("help")) then
pbot("Welcome to the TomeNET Arcade Server. Tired of slowly leveling up and automeleeing and stuff? Here you can run around at +20 speed while manually aiming and firing a manabolt machinegun at unending hordes of swordsmen. See the thread entitled 'Total Carnage' in the tomenet general discussion forum at www.tomenet.eu for more information. Also, you can ask me for help about the following topics: macros, rings, levels, skillz, multiplayer. Just send me a message contining one of those words.")
end


--this works. use with care :)
--[[
	if (what == "god can suck my ass") then
		bot ("Don't blaspheme!");
		manaball(who);
	end
]]
	if (find("^command:")) then
		if (find("play mario")) then 
			players(who).game=2; 
		end

		if (find( "fire 1")) then
			players(who).firedir=1; end
		if (find( "fire 2")) then
			players(who).firedir=2; end
		if (find( "fire 3")) then
			players(who).firedir=3; end
		if (find( "fire 4")) then
			players(who).firedir=4; end
		if (find( "fire 6")) then
			players(who).firedir=6; end
		if (find( "fire 7")) then
			players(who).firedir=7; end
		if (find( "fire 8")) then
			players(who).firedir=8; end
		if (find( "fire 9")) then 
			players(who).firedir=9; end
		if (find( "cease fire")) then 
			players(who).firedir=0; end
		if (find( "play smash")) then 
			if (players(who).wpos.wz==0) then
				player_send(who, 32, 32, 1, "whoosh");
				players(who).gametime=uptime;
				players(who).game=1; 
			end
		end
	end
end


function firin()
	q = 1;
	while (q<NumPlayers+1) do
	if(players(q).firedir>0) then
	fire_bolt(q, GF_MANA, players(q).firedir, 1000, " pffts for"); end
	q = q + 1;
end
end
--[[ function blast()
	p=1;
	while p<NumPlayers+1 do
		if(players(p).exp>878888888) then
--	fire_bolt(p, GF_FIRE, 2, 50000, " pwns you for"); 
	     		teleport_monster(p, 2);
		end
		if (players(p).exp==777777777) then
		teleport_monster(p, 6);
		if(players(p).py > 29) then players(p).au=2 end
		if(players(p).py < 24) then players(p).au=1 end
		if(players(p).au==1) then swap_position(p, players(p).py+1, players(p).px); end
		if(players(p).au==2) then swap_position(p, players(p).py-1, players(p).px); end
		end
	p = p+1;
	end
end  ]]

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
function fort_heal(who)
	ball(who, 2, 154, 0, 2, 65536+176, 32767, "");
end
list = {
"Who painted 'La Gioconda' (Mona Lisa)? ________ __ _____", "Leonardo Da Vinci",
"Which category does not fit Hans Arp: a) surrealism, b) dadaism or c) impressionism? _", "c",
"In which city was irish artist Francis Bacon born? ______", "Dublin",
"In which German city was painter Max Beckmann born? _______", "Leipzig",
"In which German city was Joseph Beuys born? _____", "Kleve",
"Max Bill is an artist from which country? ___________", "Switzerland",
"What is the original name of Hieronymus Bosch? __________ ___ ____", "Iheronimus van Aken",
"What's the last name of this Italian Renaissance painter (1445-1510), Sandro ...? __________", "Botticelli",
"Georges Braque is from which country? ______", "France",
"What's the last name of this French artist of fauvism and cubism (1882-1963), Georges ...? ______", "Braque",
"What's Michelangelo's last name? __________", "Buonarroti",
"This German artist (1832-1908) was one of the inventors of comic strips; Wilhelm ...? _____", "Busch",
"What's the name of the famous Italian painter, who lived from 1573 to 1610? __________", "Caravaggio",
"What's the 20th century style which would translate to 'the new art' ___ _______", "Art Nouveau",
"What was Art Nouveau called in Italy  _____ _______", "Stile Liberty",
"What was the group gathering around the architect Theo van Doesburg __ _____", "De Stijl",
"Which artist was born 1887 in La Chaux-de-Fond, Neuchatel, Switzerland __ _________", "Le Corbusier",
"Who, in 1912, exhibited the 'Nude descending a Staircase' ______ _______", "Marcel Duchamp",
"What's the full name of Pablo Picasso _____ ____ _ _______", "Pablo Ruiz y Picasso",
"Picasso was born in 1881 in which city ______", "Malaga",
"What was Picasso's painting period from 1901 to 1904 ____ ______", "Blue Period",
"Who painted 'Guernica' (1937) _____ _______", "Pablo Picasso",
"Who was born in Pittsburgh in August 1928 as third son of Ondrej Warhola ____ ______", "Andy Warhol",
"Which famous architect was born at Richland Centre, Wisconsin in 1867 _____ _____ ______", "Frank Lloyd Wright",
"Which famous surrealist was born in Figueres, Catalonia, in 1904 ________ ____", "Salvador Dali",
"What was the name of Salvador Dali's muse, born Helena Deluvina Diakonoff ____ ______", "Gala Eluard",
"A friend of Salvador Dali, Luis ... ______", "Bunuel",
"Who painted 'New York Office', 'Summertime', and 'Room in Brooklyn' ______ ______", "Edward Hopper",
"Which artist was born 1963 in Neumarkt St Veit, Germany (initials FA) _____ _________", "Franz Ackermann",
"Niki-de-Saint Phalle was born in which country ______", "France",
"Who painted 'Portrait of Bernhard Koehler' (1910) ______ _____", "August Macke",
"What is arguably Edvard Munch's most famous painting (it's from 1893) ___ ______", "The Scream",
"Who made a 'Self-Portrait with a Wine Bottle' in 1906 ______ _____", "Edvard Munch",
"Who painted 'Deer at Dusk' in 1909 _____ ____", "Franz Marc",
"Who painted 'The Little Blue Horses' in 1911 _____ ____", "Franz Marc",
"Which early 20th-century art movement's name comes from the French for 'Wild Beasts' _______", "Fauvism",
"Henri Matisse's 'Green Stripe (Madame Matisse)' is held in which style _______", "Fauvism",
"Who painted 'Reclining Nude from the Back' in 1917 __________", "Modigliani",
"Which international movement originated in Zurich Feb. 5, 1916, in the Cabaret Voltaire ____", "Dada",
"Which Italian art movement was initiated by the poet Filippo Tommaso Marinetti in 1909 ________", "Futurism",
"German 'Die Bruecke' and 'Der Blaue Reiter' where part of which art movement ______ _____________", "German Expressionism",
"Which 1942 painting, portraying a group of people in a night bar, is arguably Edward Hopper's best-known __________", "Nighthawks",
"Which major 20th century American realist was born in 1882 ______ ______", "Edward Hopper",
"The Solomon Guggenheim Museum is in which city ___ ____", "New York",
"Who designed the Guggenheim museum _____ _____ ______", "Frank Lloyd Wright",
"kjmmn,m nn  _________,", "mkljiojkm,",
"what is the name of the famous surrealist artist ___", "max",
"what state do i live in? ______", "Hawaii",
"what is my name!? ____ ____", "Roxy Lane",
"How old am i? 44", "44",
"What was Renaissance astist Michealangelo's last name ______", "robert",
"how old is edward hopper?", "69",
"how did dalis wife die ___ ___ ____ __ ___ ____", "she got shot in the head",
"Who invented 'Spawn' ____ _________", "Todd McFarlane",
"Who's the creator of Calvin &amp; Hobbes ____ _________", "Bill Watterson",
"Stan Lee and who invented many of the original Marvel superheros ____ _____", "Jack Kirby",
"Who's the author of 'Eightball' ______ [_________] ______", "Daniel [Gillespie] Clowes",
"Who reinvented the Man of Steel -- Superman -- in the 80s ____ _____", "John Byrne",
"Who created 'Batman - The Dark Knight Returns' _____ ______", "Frank Miller",
"Who created 'Hate' _____ _____", "Peter Bagge",
"The movie 'Ghost Story' made its debut in what comic book _________", "Eightball",
"Who directed 'Crumb', the documentary _____ _______", "Terry Zwigoff",
"Which comic artist created the movie poster to 'Happiness' ______ [_________] ______", "Daniel [Gillespie] Clowes",
"Who's closing off his commentaries with ''Nuff Said' ____ ___", "Stan Lee",
"Which arch-enemy was first born from Spider-Man's costume _____", "Venom",
"What's the name of Superman in everyday life as journalist _____ ____", "Clark Kent",
"Which of the X-Men was a thief ______", "Gambit",
"What's the real name of X-Men's Gambit? ____ ______", "Remy LeBeau",
"Who's X-Men's Xavier arch enemy _______", "Magneto",
"In the Infinity Wars series, Spider-Man got a new costume of which color _____", "Black",
"Who wrote the 80's 'Watchmen' series ____ _____", "Alan Moore",
"Who drew the 80's 'Watchmen' series ____ _______", "Dave Gibbons",
"Who created the 'Sandman' ____ ______", "Neil Gaiman",
"Who created the 'Peanuts' comic strip _______ ______", "Charles Schulz",
"When did the 'Peanuts' comic strip firt appear [_______] 1950", "[October] 1950",
"Charlie Brown is a character in which comic strip _______", "Peanuts",
"At which age did the creator of 'Peanuts', Charles Schulz, die 77", "77",
"Who invented 'Mr. Natural' ______ _____", "Robert Crumb",
"Who invented Fritz the Cat ______ _____", "Robert Crumb",
"Dr. Octopus and The Goblin are arch-enemies of which super-hero ______[-]___", "Spider[-]Man",
"Joker and Riddler are arch-enemies of which super-hero ______", "Batman",
"Ma and Pa Kent raised up a little boy who later become which super-hero ________", "Superman",
"Who's the creator of the Dilbert cartoons _____ _____", "Scott Adams",
"Catbert and Ratbert are characters in which cartoon _______", "Dilbert",
"Dogbert is a character in which cartoon by Scott Adams _______", "Dilbert",
"Art Spiegelman won the Pulitzer Prize in '92 for which comic book narrative ____", "MAUS",
"In 1992, who won the Pulitzer Prize for his comic book narrative ___ __________", "Art Spiegelman",
"The 'Human Torch' is part of which superhero group _________ ____", "Fantastic Four",
"The 'Thing' is part of which superhero group _________ ____", "Fantastic Four",
"'Mr. Fantastic' is part of which superhero group _________ ____", "Fantastic Four",
"Who's the creator of 'Cerebus', one of the longest running comics in history ____ ___", "Dave Sim",
"Dave Sim, creator of 'Cerebus', is from which country ______", "Canada",
"Who's the creator of 'Akira' _________ _____", "Katsuhiro Otomo",
"Marvel's The Punisher first appeared in #129 of which comic book ___ _______ ______-___", "The Amazing Spider-Man",
"Amazing Spider-Man #96 was not approved by what comic book authority ______ ____ _________", "Comics Code Authority",
"Who created underground-inspiring autobiography 'American Splendor' ______ _____", "Harvey Pekar",
"Who's Marvel's blind super-hero, Matt Murdock _________", "Daredevil",
"Who's the blind man behind the mask of New York's super-hero Daredevil ____ _______", "Matt Murdock",
"Which US publication roots in french comic book 'Metal Hurlant' _____ _____", "Heavy Metal",
"'Is he Man or Monster or... is he both?' covers #1 issue of what comic ___ __________ ____", "The Incredible Hulk",
"Brothers Jaime, Gilbert and Mario Hernandez created which comic book in 1982 ____ ___ _______", "Love and Rockets",
"Which british writer is responsible for 'The Saga of the Swamp Thing' #20 ____ _____", "Alan Moore",
"Who wrote 'Sandman' #1 ____ ______", "Neil Gaiman",
"Who's the author of 'Understanding Comics', a comic book about comic books _____ _______", "Scott McCloud",
"Enid Coleslaw is an anagram of which artists name ______ ______", "Daniel Clowes",
"Who created the Lloyd Llewellyn series ______ [_________] ______", "Daniel [Gillespie] Clowes",
"Red Skull is arch villain of which super hero _______ _______", "Captain America",
"The Green Goblin is arch villain of which super hero ______-___", "Spider-Man",
"Dr. Octopus is arch villain of which super hero ______-___", "Spider-Man",
"The Vulture is arch villain of which super hero ______-___", "Spider-Man",
"The Riddler is arch villain of which super hero ______", "Batman",
"The Joker is arch villain of which super hero ______", "Batman",
"Who's the sidekick of Batman _____", "Robin",
"Bruce Wayne is also known as ... ______", "Batman",
"Peter Parker's love is called ... ____ ____", "Mary Jane",
"Who starrs as Spider-Man in Sam Raimi's 2002 movie _____ _______", "Tobey Maguire",
"Who created the original Spider-Man comic book, Steve Ditko and ... ____ ___", "Stan Lee",
"Who created the original Spider-Man comic book, Stan Lee and ... _____ _____", "Steve Ditko",
"Who starrs as Norman Osborn/ The Green Goblin in the 2002 Spider-Man movie ______ _____", "Willem Dafoe",
"Who starrs as Mary Jane Watson in the 2002 Spider-Man blockbuster by Sam Raimi _______ _____", "Kirsten Dunst",
"Who directed the Batman movie of 1989 ___ ______", "Tim Burton",
"Who starrs as Batman/ Bruce Wayne in the 1989 version of Batman _______ ______", "Michael Keaton",
"Who starrs as Joker in the 1989 version of Batman ____ _________", "Jack Nicholson",
"Who starrs as Vicki Vale in the 1989 version of Batman ___ ________", "Kim Basinger",
"Who is the Dark Knight of Gotham City, Bruce Wayne ______", "Batman",
"Who directed Batman Forever (1995) ____ __________", "Joel Schumacher",
"Who starrs as Harvey 'Two-Face' Dent in 'Batman Forever' _____ ___ _____", "Tommy Lee Jones",
"Who starrs as Batman/ Bruce Wayne in 'Batman Forever' ___ ______", "Val Kilmer",
"Who starrs as The Riddler/ Edward Nygma in 'Batman Forever' ___ ______", "Jim Carrey",
"Who starrs as Dr. Chase Meridian in 'Batman Forever' ______ ______", "Nicole Kidman",
"Who directed 'Batman Returns' (1992) ___ ______", "Tim Burton",
"Who starrs as Penguin in 'Batman Returns' _____ ______", "Danny DeVito",
"Who starrs as Catwoman/ Selina in 'Batman Returns' of 1992 ________ ________", "Michelle Pfeiffer",
"Who starrs as Max Shreck in Burton's 'Batman Returns' (1992) ___________ ______", "Christopher Walken",
"What does 'DC' stand for _________ ______", "Detective Comics",
"Superman: character of Marvel, Image, or DC __", "DC",
"Hulk: character of Marvel, Image, Dark Horse or DC ______", "Marvel",
"Spider-Man: character of Image, Marvel, DC, Dark Horse ______", "Marvel",
"Spawn: character of Marvel, Image, DC, Dark Horse, Fantagraphics _____", "Image",
"Bruce Banner: character of Marvel, Image, DC, Dark Horse, Fantagraphics ______", "Marvel",
"Peter Parker: character of Marvel, Image, DC, Dark Horse, Fantagraphics ______", "Marvel",
"Savage Dragon: character of Marvel, Image, DC, Dark Horse, Fantagraphics _____", "Image",
"Batman: character of Marvel, Image, DC, Dark Horse, Fantagraphics __", "DC",
"David Boring: character of Marvel, Image, DC, Dark Horse, Fantagraphics _____________", "Fantagraphics",
"Enid Coleslaw: character of Marvel, Image, DC, Dark Horse, Fantagraphics _____________", "Fantagraphics",
"X-Men: characters of Marvel, Image, DC, Dark Horse, Fantagraphics ______", "Marvel",
"X-Force: character of Marvel, Image, DC, Dark Horse, Fantagraphics ______", "Marvel",
"Cable: character of Marvel, Image, DC, Dark Horse, Fantagraphics ______", "Marvel",
"Robin: character of Marvel, Image, DC, Dark Horse, Fantagraphics __", "DC",
"Grendel: character of Marvel, Image, DC, Dark Horse, Fantagraphics ____ _____", "Dark Horse",
"Bruce Banner, when angry, turns into... ____", "Hulk",
"Bruce Wayne, at night, turns into ... ______", "Batman",
"Peter Parker, when hunting villains, is ... ______-___", "Spider-Man",
"'Cerberus' is a misspelling of ... _______", "Cerebus",
"The Riddler: character of Marvel, Image, DC, Dark Horse, Fantagraphics __", "DC",
"The Joker: character of Marvel, Image, DC, Dark Horse, Fantagraphics __", "DC",
"Professor Xavier: character of Marvel, Image, DC, Dark Horse, Fantagraphics ______", "Marvel",
"Edward Nygma is also knows as ... ___ _______", "The Riddler",
"In which comic did Harvey Kurtzman have his first drawing published in 1939 ___ ___ ______", "Tip Top Comics",
"Jacob Kurtzberg is better known as who ____ _____", "Jack Kirby",
"What was Stan Lee's 1960s nickname for Jack Kirby ___ ____", "The King",
"Kaneda is a motor-cycling rebel in which Manga _____", "Akira",
"At which university did Masamune Shirow study _____ __________ __ ____", "Osaka University of Arts",
"Who created the Manga 'Appleseed' ________ ______", "Masamune Shirow",
"Stan Lee and who teached 'How to Draw Comics the Marvel Way' ____ _______", "John Buscema",
"What was the Marvel Superhero group -  _________ ____", "something Xmen",
"What was the original name of the comic strip Peanuts? _______ _____", "charlie brown",
"when was salvador dali die ___ ___ 11__ 1904", "may the 11th 1904",
"when was salvador dali born ___ ___ 11__ 1904", "may the 11th 1904",
"in what year did the first Batman comic book appear 1948", "1948",
"Peanuts originally went by what name? _'__ _____", "L'il Folks",
"Which german inventor build the first computer ______ ____", "Konrad Zuse",
"Which company produced the Amiga 500 computer _________", "Commodore",
"Which one came first: C64 or Amiga 500 _[________]64", "C[ommodore]64",
"Who's the inventor of the World Wide Web ___ _______-___", "Tim Berners-Lee",
"Who invented HTML, the Hypertext Markup-Language ___ _______-___", "Tim Berners-Lee",
"Who invented HTTP, the HyperText Transfer Protocol ___ _______-___", "Tim Berners-Lee",
"Which company is responsible for most of the standards used in the World Wide Web _____ ____ ___ __________", "World Wide Web Consortium",
"Is multiple inheritance possible in Java __", "no",
"Is Java and JavaScript basically the same thing __", "no",
"Which company distributes Java ___", "Sun",
"Which new Microsoft language is leading the .NET strategy of 2002 _#", "C#",
"Microsoft's programming language VBasic is short for ... ______[_____]", "Visual[Basic]",
"Programming language BASIC is short for ... ________'_ ___-_______ ________ ___________ ____", "Beginner's All-Purpose Symbolic Instruction Code",
"Programming language C++ had which predecessor _", "C",
"How many Bytes are 1 KiloByte", "1024",
"What does HTTP stand for _____[-]____ ________ ________", "Hyper[-]Text Transfer Protocol",
"What does HTML stand for _____[-]____ ____[-]__ ________", "Hyper[-]text Mark[-]up Language",
"XHTML is based on which open standard ___", "XML",
"What's the abbreviation of Extensible Markup Language ___", "XML",
"What is XML __________ ______ ________", "eXtensible Markup Language",
"Who invented Linux _____ ________", "Linus Torvalds",
"Linus Torvalds is most famous for inventing what Operating System _____", "Linux",
"What's the color of Pac Man ______", "yellow",
"The Amiga 500 keyboard had wich color ____", "grey",
"Who produces the Pentium Processors _____", "Intel",
"Which of these figures is most common in the year 2002: a) 0.1GB HD, b) 10GB HD, c) 1,000GB HD, or d) 10,000GB HD _", "b",
"Which hardware component is the most common input device, besides the mouse ________", "keyboard",
"Which hardware component is the most common input device, besides the keyboard _____", "mouse",
"Related to web site browsing, TTS stands for what ____-__-______", "Text-to-Speech",
"Who's heading Microsoft ____ _____", "Bill Gates",
"Computer games 'Populous' and 'Black &amp; White' were invented by which game designer _____ ________", "Peter Molyneux",
"Computer acronyms: LOL? ________ ___ ____", "Laughing Out Loud",
"Computer acronyms: ROTFL? _______ __ ___ _____ ________", "Rolling on the floor laughing",
"Computer abbreviations: BRB? __ _____ ____", "Be Right Back",
"Computer abbreviations: BBL? __ ____ _____", "Be Back Later",
"Computer abbreviations: ACK? ____________", "Acknowledged",
"Computer abbreviations: AFAIK? __ ___ __ _ ____", "As Far As I Know",
"Computer abbreviations: IIRC __ _ ________ _________", "If I Remember Correctly",
"Computer abbreviations: IMO __ __ _______", "In My Opinion",
"Computer abbreviations: IMHO __ __ ______ _______", "In My Humble Opinion",
"Computer abbreviations: ROTW ____ __ ___ _____", "Rest Of The World",
"Computer abbreviations: HTH ____ ____ _____", "Hope That Helps",
"Computer abbreviations: IRC ________ _____ ____", "Internet Relay Chat",
"Tim Berners-Lee is most famous for inventing what? _____ ____ ___", "World Wide Web",
"Computer abbreviations: W3C _____ ____ ___ __________", "World Wide Web Consortium",
"Computer abbreviations: SSL ______ ______ _____", "Secure Socket Layer",
"What's this hex-number in the decimal system: FF", "255",
"What's bigger, bit or byte ____", "byte",
"Who visited Xerox PARC and got some ideas for the Apple Macintosh GUI? _____ ____", "Steve Jobs",
"Computer abbreviations: GUI _________ ____ _________", "Graphical User Interface",
"Computer abbreviations: MDI _____ ________ _________", "Multi Document Interface",
"Computer abbreviations: API ___________ ___________ _________", "Application Programming Interface",
"Computer abbreviations: XML __________ ______ ________", "eXtensible Markup Language",
"Computer abbreviations: DHTML _______ _________ ______ ________", "Dynamic Hypertext Markup Language",
"Computer abbreviations: CSS _________ ___________", "Cascading Stylesheets",
"What is the abbreviation for Hypertext Markup Language ____", "HTML",
"Who invented HTML ___ _______-___", "Tim Berners-Lee",
"What does W3C stand for _____ ____ ___ __________", "World Wide Web Consortium",
"Is HTML4 deprecated __", "No",
"On what syntax framework is XHTML based ___", "XML",
"On what syntax framework is HTML based ____", "SGML",
"What does element 'em' mean ________", "emphasis",
"What does element 'br' mean _____", "break",
"Which is the lowest heading in HTML _6", "h6",
"Are XHTML element names case-sensitive ___", "yes",
"Are HTML element names case-sensitive __", "no",
"Can you seperate CSS by media ___", "yes",
"On which browser engine is Netscape 6 based _______", "Mozilla",
"Which of these formats should typically not be used in a webpage: GIF, PNG, BMP, JPG ___", "BMP",
"In Python, what's the command for 'else if' ____", "elif",
"In Python, what's the command for a function ___", "def",
"In which programming language would the following appear: '#!/usr/bin/python' ______", "Python",
"In which programming language would the following appear: 'from xml.dom.minidom import *' ______", "Python",
"As of 2002, is there a ternary operator in Python __", "No",
"The programming language name 'Python' is based on what _____ ______'_ ______ ______", "Monty Python's Flying Circus",
"What's the name of Google's 2002 started service in which real persons answer questions for a price ______ _______", "Google Answers",
"Altavista, Google, and Yahoo are ... ______ _______", "search engines",
"What's the first computer made? _____", "ENIAC",
"Who wrote Python? _____ ___ ______", "Guido van Rossum",
"What Linux distribution provides the dpkg package management system? ______", "Debian",
"If a pointer in C++ is allocated with new[], what is the corresponding deallocation keyword/function? ______[]", "delete[]",
"You sit on the opulent throne. You feel out of place... _______", "NetHack",
"What is the question for North, South, West and East in german ___ ___ __ _____", "Das ist im Osten",
"What's the capital city of Italy ____", "Rome",
"What became the capital city of West Germany after World War II ____", "Bonn",
"Which german city comes first in a german dictionary ______", "Aachen",
"Manhattan is a part of which city ___ ____", "New York",
"What is the capital of United Kingdom  ______", "London",
"What is the capital of Ukraine  ____", "Kiev",
"What is the capital of Switzerland  ____", "Bern",
"What is the capital of Sweden  _________", "Stockholm",
"What is the capital of Spain  ______", "Madrid",
"What is the capital of Slovenia  _________", "Ljubljana",
"What is the capital of Slovakia  __________", "Bratislava",
"What is the capital of Serbia/Montenegro (Yugoslavia) ________", "Belgrade",
"What is the capital of  the Russian Federation  ______", "Moscow",
"What is the capital of  San Marino  ___ ______", "San Marino",
"What is the capital of  Romania  _________", "Bucharest",
"What is the capital of Portugal  ______", "Lisbon",
"What is the capital of Poland  ______", "Warsaw",
"Oslo is the capital of which country? ______", "Norway",
"Amsterdam is the capital of which country ___________", "Netherlands",
"Monaco is the capital of which country ______", "Monaco",
"Chisinau is the capital of which country _______", "Moldova",
"Valletta is the capital of which country _____", "Malta",
"Skopje  is the capital of which country _________", "Macedonia",
"Luxembourg is the capital of which country __________", "Luxembourg",
" Vilnius  is the capital of which country _________", "Lithuania",
"Vaduz  is the capital of which country _____________", "Liechtenstein",
" Riga  is the capital of which country ______", "Latvia",
" Rome is the capital of which country _____", "Italy",
"Dublin  is the capital of which country _______", "Ireland",
" Reykjavik is the capital of which country _______", "Iceland",
" Budapest is the capital of which country _______", "Hungary",
"Athens is the capital of which country ______", "Greece",
" Tbilisi is the capital of which country _______", "Georgia",
"Paris  is the capital of which country  ______", "France",
" Helsinki  is the capital of which country  _______", "Finland",
" Tallinn is the capital of which country _______", "Estonia",
"Copenhagen is the capital of which country _______", "Denmark",
" Prague is the capital of which country _____ ________", "Czech Republic",
"Zagreb  is the capital of which country _______", "Croatia",
"Sofia  is the capital of which country ________", "Bulgaria",
"Sarajevo  is the capital of which country ______-___________", "Bosnia-Herzegovina",
"Brussels  is the capital of which country _______", "Belgium",
"Minsk  is the capital of which country _______", "Belarus",
"Vienna  is the capital of which country _______", "Austria",
" Andorra la Vella is the capital of which country _______", "Andorra",
"Tirane  is the capital of which country _______", "Albania",
"USA states: What's the capital of Alabama __________", "Montgomery",
"USA states: What's the capital of Alaska ______", "Juneau",
"USA states: What's the capital of Arizona _______", "Phoenix",
"USA states: What's the capital of Arkansas ______ ____", "Little Rock",
"USA states: What's the capital of California __________", "Sacramento",
"USA states: What's the capital of Colorado ______", "Denver",
"USA states: What's the capital of Connecticut ________", "Hartford",
"USA states: What's the capital of Delaware _____", "Dover",
"USA states: What's the capital of Florida ___________", "Tallahassee",
"USA states: What's the capital of Georgia _______", "Atlanta",
"USA states: What's the capital of Hawaii ________", "Honolulu",
"USA states: What's the capital of Idaho _____", "Boise",
"USA states: What's the capital of Kansas ______", "Topeka",
"USA states: What's the capital of Kentucky _________", "Frankfort",
"USA states: What's the capital of Louisiana _____ _____", "Baton Rouge",
"USA states: What's the capital of Maine _______", "Augusta",
"USA states: What's the capital of Maryland _________", "Annapolis",
"USA states: What's the capital of Massachusetts ______", "Boston",
"USA states: What's the capital of Michigan _______", "Lansing",
"USA states: What's the capital of Minnesota __. ____", "St. Paul",
"USA states: What's the capital of Mississippi _______", "Jackson",
"USA states: What's the capital of Missouri _________ ____", "Jefferson City",
"USA states: What's the capital of Montana ______", "Helena",
"USA states: What's the capital of Nebraska _______", "Lincoln",
"USA states: What's the capital of Nevada ______ ____", "Carson City",
"USA states: What's the capital of New Hampshire _______", "Concord",
"USA states: What's the capital of New Jersey _______", "Trenton",
"USA states: What's the capital of New Mexico _____ __", "Santa Fe",
"USA states: What's the capital of New York ______", "Albany",
"USA states: What's the capital of North Carolina _______", "Raleigh",
"USA states: What's the capital of North Dakota ________", "Bismarck",
"USA states: What's the capital of Ohio ________", "Columbus",
"USA states: What's the capital of Oklahoma ________ ____", "Oklahoma City",
"USA states: What's the capital of Oregon _____", "Salem",
"USA states: What's the capital of Pennsylvania __________", "Harrisburg",
"USA states: What's the capital of Rhode Island __________", "Providence",
"USA states: What's the capital of South Carolina ________", "Columbia",
"USA states: What's the capital of South Dakota ______", "Pierre",
"USA states: What's the capital of Tennessee _________", "Nashville",
"USA states: What's the capital of Texas ______", "Austin",
"USA states: What's the capital of Utah ____ ____ ____", "Salt Lake City",
"USA states: What's the capital of Vermont __________", "Montpelier",
"USA states: What's the capital of Virginia ________", "Richmond",
"USA states: What's the capital of Washington _______", "Olympia",
"USA states: What's the capital of West Virginia __________", "Charleston",
"USA states: What's the capital of Wisconsin _______", "Madison",
"USA states: What's the capital of Wyoming ________", "Cheyenne",
"40% of the world's newspapers are printed on paper from which country ______", "Canada",
"What do you call the mostly rural residential area beyond the suburbs of a city _______", "Exurbia",
"What's the name for the thousands of islands in the central and southern Pacific Ocean _______", "Oceania",
"What's the only continent without reptiles or snakes __________", "Antarctica",
"About 70% of the Earth is covered with what _____", "Water",
"Belize was formerly known as what _______ ________", "British Honduras",
"Which country has more lakes than the rest of the world combined ______", "Canada",
"What's the second-largest country in the world after Russia ______", "Canada",
"Does Canada own the North Pole __", "no",
"Which world's oldest black republic has as major religion voodoo _____", "Haiti",
"The basic monetary unit 'tala' is used in which country _______ _____", "Western Samoa",
"Where is the geographic center of Canada located _______ ___", "Thunder Bay",
"What's the highest mountain on Earth _____ _______", "Mount Everest",
"What's the only country in the Middle East that does not have a desert _______", "Lebanon",
"What's the largest French-speaking city in the Western Hemisphere ________", "Montreal",
}
