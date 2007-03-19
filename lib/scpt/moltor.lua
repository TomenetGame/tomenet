function pfft()
	fire_bolt(Ind, GF_MANA, 2, 10, "")
	players("Motok").chp = players("Motok").chp - 10;
end

function blast()
	if(players("Motok").chp > 11)
	then
	pfft();
	end
end
