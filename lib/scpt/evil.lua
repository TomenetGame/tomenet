
function ball(name, rad, type, harm, dam, flg)
	local i;
	local w;
	i=ind(name);
	if(i==-1) then return end;
--	if(harm==1) then w=-999 else w=-i end;
	if(harm==1) then w=-1000 else w=-i end;
	if(rad>8) then rad=8 end;
	project(w, rad, players(i).wpos, players(i).py, players(i).px, dam, type, flg);
end

function wallin(name)
	ball(name, 2, 71, 0, 0, 176);
	msg_print(Ind, "Walling in "..name);
end

function trap(name)
	ball(name, 2, 47, 0, 0, 176);
	msg_print(Ind, "Trapping "..name);
end

function manaball(name)
	ball(name, 2, 26, 1, 1000, 73);
	msg_print(Ind, "Blasted "..name);
end

-- 40 stm 81 disin
function openarea(name)
	ball(name, 8, 81, 0, 0, 176);
	msg_print(Ind, "Opening up area for "..name);
end

function curse(i)
	local j = 1;
	players(i).black_breath=1;
	players(i).au=0;
	while(j<7)
	do
		players(i).stat_cur[j]=3;
		players(i).stat_max[j]=3;
		j=j+1;
	end
end

function res(i)
	players(i).exp=players(i).exp/2;
	players(i).max_exp=players(i).max_exp/2;
	players(i).ghost=0;
end

function recall(i, dep)
	players(i).word_recall=3;
end
