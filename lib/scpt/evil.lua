
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
