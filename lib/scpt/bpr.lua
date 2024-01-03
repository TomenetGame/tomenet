-- Tables/formula with startup-BpR numbers, depending on STR/DEX/weapon weight/Class.
-- To be displayed during character creation process to alleviate
-- the need of looking up stats in the guide to hit breakpoints. - C. Blue

--Using class name string instead of numerical index just in case there's a server-client incompatibility after another class gets added.
function classname2index(cn)
	local c

	c = 0
	if cn == "Warrior" then c = 0 end
	if cn == "Istar" then c = 1 end
	if cn == "Priest" then c = 2 end
	if cn == "Rogue" then c = 3 end
	if cn == "Mimic" then c = 4 end
	if cn == "Archer" then c = 5 end
	if cn == "Paladin" then c = 6 end
	if cn == "Ranger" then c = 7 end
	if cn == "Adventurer" then c = 8 end
	if cn == "Druid" then c = 9 end
	if cn == "Shaman" then c = 10 end
	if cn == "Runemaster" then c = 11 end
	if cn == "Mindcrafter" then c = 12 end
	if cn == "Death Knight" then c = 13 end
	if cn == "Hell Knight" then c = 14 end
	if cn == "Corrupted Priest" then c = 15 end
	return c
end

--(A) just return fixed values from the table?
function get_class_bpr(cn, n)
	local c

	c = classname2index(cn)
	return "S" .. __class_bpr[c + 1][n + 1][1] .. "D" .. __class_bpr[c + 1][n + 1][2] .. "B" .. __class_bpr[c + 1][n + 1][3]
end
function get_class_bpr_tablesize()
	return 4
end

--(B) alternatively, return the actual dps formula's result
function get_class_bpr2(cn, wwgt, st, dx)
--	return "X" --means "disabled"
	local div, wgt, mul, str_adj, str_index, dex_index, numblow
	local c, bpr

	c = classname2index(cn)
	--some classes don't have significant weapon-BpR usage
	if wwgt == 0 or st < 3 or dx < 3 then
		--it's just test call ("classname",0,0,0) to verify presence
		if c == 1 or c == 5 or c == 9 then
			--confirm ok + insignificant bpr
			bpr = "N"
		else
			--confirm ok + signficant bpr
			bpr = "F"
		end
	elseif c == 1 or c == 5 or c == 9 then
		bpr = "N"
	else
		--calculate BpR..
		if c == 0 then
			wgt = 30
			mul = 4
		elseif c == 2 then
			wgt = 35
			mul = 4
		elseif c == 3 then
			wgt = 30
			mul = 4
		elseif c == 4 then
			wgt = 35
			mul = 5
		elseif c == 6 then
			wgt = 35
			mul = 5
		elseif c == 7 then
			wgt = 35
			mul = 4
		elseif c == 8 then
			wgt = 35
			mul = 4
		elseif c == 10 then
			wgt = 35
			mul = 4
		elseif c == 11 then
			wgt = 30
			mul = 4
		elseif c == 12 then
			wgt = 35
			mul = 4
		elseif c == 13 then
			wgt = 35
			mul = 5
		elseif c == 14 then
			wgt = 35
			mul = 5
		elseif c == 15 then
			wgt = 35
			mul = 4
		end

		div = wwgt
		if div < wgt then div = wgt end
		str_adj = adj_str_blow[st - 3 + 1]
		str_index = (str_adj * mul) / div
		if str_index > 11 then str_index = 11 end
		dex_index = adj_dex_blow[dx - 3 + 1]
		if dex_index > 11 then dex_index = 11 end
		numblow = blows_table[str_index + 1][dex_index + 1]
		--bpr = "F" .. numblow
		bpr = numblow
	end
	return bpr
end

--(C) use formula to calc bpr for all 4 weapon classes
function get_class_bpr3(cn, st, dx)
	local sword, blunt, axe, polearm
	local ssword, sblunt, saxe, spolearm

	sword = get_class_bpr2(cn, 30, st, dx) --dagger/main gauche
	blunt = get_class_bpr2(cn, 30, st, dx) --whip
	axe = get_class_bpr2(cn, 50, st, dx) --cleaver
	polearm = get_class_bpr2(cn, 60, st, dx) --spear

	if sword == 1 then ssword = "\255o1"
	elseif sword == 2 then ssword = "\255y2"
	else ssword = "\255G" .. sword
	end

	if blunt == 1 then sblunt = "\255o1"
	elseif blunt == 2 then sblunt = "\255y2"
	else sblunt = "\255G" .. blunt
	end

	if axe == 1 then saxe = "\255o1"
	elseif axe == 2 then saxe = "\255y2"
	else saxe = "\255G" .. axe
	end

	if polearm == 1 then spolearm = "\255o1"
	elseif polearm == 2 then spolearm = "\255y2"
	else spolearm = "\255G" .. polearm
	end

	return "" .. ssword .. " \255W/ " .. sblunt .. " \255W/ " .. saxe .. " \255W/ " .. spolearm
end

-- for using the real formula
adj_str_blow = { 3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170,180,190,200,210,220,230,240, }
adj_dex_blow = { 0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,2,2,2,2,3,3,4,4,5,6,7,8,9,10,11,12,14,16,18,20,20,20, }
blows_table = {
	{  1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   3, },
	{  1,   1,   1,   1,   2,   2,   3,   3,   3,   4,   4,   4, },
	{  1,   1,   2,   2,   3,   3,   4,   4,   4,   5,   5,   5, },
	{  1,   2,   2,   3,   3,   4,   4,   4,   5,   5,   5,   5, },
	{  1,   2,   2,   3,   3,   4,   4,   5,   5,   5,   5,   5, },
	{  2,   2,   3,   3,   4,   4,   5,   5,   5,   5,   5,   6, },
	{  2,   2,   3,   3,   4,   4,   5,   5,   5,   5,   5,   6, },
	{  2,   3,   3,   4,   4,   4,   5,   5,   5,   5,   5,   6, },
	{  3,   3,   3,   4,   4,   4,   5,   5,   5,   5,   6,   6, },
	{  3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6, },
	{  3,   3,   4,   4,   4,   4,   5,   5,   5,   6,   6,   6, },
	{  3,   3,   4,   4,   4,   4,   5,   5,   6,   6,   6,   7, },}

-- for each class: 3x {str,dex,bpr} (stat 0 = any)
__class_bpr = {
{ --warrior
    { 18, 10, 2, },
    { 19,  0, 2, },
    { 21,  0, 3, },
    { 22, 19, 4, },
}, { --istar
    { 0,  0, 0, },
    { 0,  0, 0, },
    { 0,  0, 0, },
    { 0,  0, 0, },
}, { --priest
    { 19, 10, 2, },
    { 23, 10, 3, },
    {  0,  0, 0, },
    { 0,  0, 0, },
}, { --rogue
    { 15, 19, 2, },
    { 19, 23, 3, },
    { 20, 19, 3, },
    { 0,  0, 0, },
}, { --mimic
    { 19, 10, 2, },
    { 23, 10, 3, },
    { 21, 19, 3, },
    { 0,  0, 0, },
}, { --archer
    { 0,  0, 0, },
    { 0,  0, 0, },
    { 0,  0, 0, },
    { 0,  0, 0, },
}, { --paladin
    { 19, 10, 2, },
    { 21, 10, 3, },
    { 20, 19, 3, },
    { 0,  0, 0, },
}, { --ranger
    { 19, 10, 2, },
    { 23, 10, 3, },
    { 21, 19, 3, },
    { 0,  0, 0, },
}, { --adventurer
    { 19, 10, 2, },
    { 21, 10, 3, },
    { 20, 19, 3, },
    { 0,  0, 0, },
}, { --druid
    { 0,  0, 0, },
    { 0,  0, 0, },
    { 0,  0, 0, },
    { 0,  0, 0, },
}, { --shaman
    { 19, 10, 2, },
    { 23, 10, 3, },
    { 21,  0, 3, },
    { 0,  0, 0, },
}, { --runemaster
    { 15, 19, 2, },
    { 19, 23, 3, },
    { 20, 19, 3, },
    { 0,  0, 0, },
}, { --mindcrafter
    { 19, 10, 2, },
    { 23, 10, 3, },
    { 21, 19, 3, },
    { 0,  0, 0, },
}, { --death knight
    { 19, 10, 2, },
    { 21, 10, 3, },
    { 20, 19, 3, },
    { 0,  0, 0, },
}, { --hell knight
    { 19, 10, 2, },
    { 21, 10, 3, },
    { 20, 19, 3, },
    { 0,  0, 0, },
}, { --corrupted priest
    { 19, 10, 2, },
    { 23, 10, 3, },
    {  0,  0, 0, },
    { 0,  0, 0, },
},}
