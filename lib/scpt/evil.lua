-- evileye functions for evil admins

function ball(name, rad, type, harm, dam, flg, time, attacker)
	local i;
	local w;
	i=ind(name);
	if(i==-1) then return end;
	if(harm==1) then w=-1000 else w=-i end;
	if(rad>8) then rad=8 end;
	project_interval=10;
	project_time=time;
	project(w, rad, players(i).wpos, players(i).py, players(i).px, dam, type, flg, attacker);
end

function wallin(name)
	ball(name, 2, 71, 0, 0, 176, 0, "");
	msg_print(Ind, "Walling in "..name);
end

function dom(name)
	ball(name, 2, 113, 0, 0, 192, 0, ""); 
	msg_print(Ind, "Dominate - "..name);
end

function evilrumour(text)
	msg_broadcast(0, "Suddenly a thought comes to your mind:");
	msg_broadcast(0, text);
end

function trap(name)
	ball(name, 2, 47, 0, 0, 176, 0, "");
	msg_print(Ind, "Trapping "..name);
end

function manaball(name)
	ball(name, 2, 26, 1, 1000, 73, 0, "");
	msg_print(Ind, "Blasted "..name);
end

function firelit(name)
	ball(name, 2, 5, 0, 0, 65536+256+176, 500, "");
	msg_print(Ind, "firelit "..name);
end

function lightness(name)
	ball(name, 2, 15, 0, 0, 65536+176, 500, "");
	msg_print(Ind, "lit "..name);
end

function feed(name)
	ball(name, 0, 116, 1, 1000, 73, 0, "");
	msg_print(Ind, "Fed "..name);
end

function darkness(name)
	ball(name, 4, 16, 0, 0, 65536+176, 500, "");
	msg_print(Ind, "darkened "..name);
end

-- 40 stm 81 disin
function openarea(name)
	ball(name, 8, 81, 0, 0, 176, 0, "");
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

function punish(i)
	local j=1;
	while(j<7)
	do
		players(i).stat_cur[j]=players(i).stat_cur[j]-1;
		players(i).stat_max[j]=players(i).stat_max[j]-1;
		j=j+1;
	end
end

-- Use this when you want to hassle a player
-- in the dungeon. (Sets same uniques as victim)
function unimask(name)
	local j = 1;
	local vict, evil;
	local i;
	i=ind(name);
	if(i==-1) then return end;
	--vict=players(i);
	--evil=players(Ind);
	while(j<max_r_idx)
	do
		players(Ind).r_killed[j]=players(i).r_killed[j];
		j=j+1;
	end
	msg_print(Ind, "Copied uniques of "..name);
end

function res(i)
	players(i).exp=players(i).exp/2;
	players(i).max_exp=players(i).max_exp/2;
	players(i).ghost=0;
end

function recall(i, dep)
	players(i).word_recall=3;
end

function player_send(name, x, y, z, msg)
	i=ind(name);
	players(i).recall_pos.wx=x;
	players(i).recall_pos.wy=y;
	players(i).recall_pos.wz=z;
-- let's try LEVEL_OUTSIDE_RAND (5) instead of LEVEL_OUTSIDE (4) - C. Blue :)
	players(i).new_level_method=5;
	recall_player(i, msg);
end

function highlander(name, x, y)
	player_send(name, x, y, 0, "You feel drawn to the battleground!");
end

function jail(name, time, reason)
	i=ind(name);
	imprison(i, time, reason);
end

function team(name, tnum)
	i=ind(name);
	players(i).team=tnum;
end
