--
-- simple scripts to examine/modify players' status.
-- 

-- display some information about a specified player.
function p_in(i)
-- +1 offset
local p = Players[i+1]
        
-- Ind is global.
	msg_print(Ind, "location: ("..p.px..","..p.py..") of ["..(p.wpos.wx)..","..(p.wpos.wy).."], "..(p.wpos.wz*50).."ft  AU:"..p.au)
	msg_print(Ind, "HP:"..p.chp.."/"..p.mhp.."  SP:"..p.csp.."/"..p.msp.."  SN:"..p.csane.."/"..p.msane.."  XP:"..p.exp.."/"..p.max_exp)
	msg_print(Ind, "St:"..p.stat_cur[1].."/"..p.stat_max[1].." In:"..p.stat_cur[2].."/"..p.stat_max[2].." Wi:"..p.stat_cur[3].."/"..p.stat_max[3].." Dx:"..p.stat_cur[4].."/"..p.stat_max[4].." Co:"..p.stat_cur[5].."/"..p.stat_max[5].." Ch:"..p.stat_cur[6].."/"..p.stat_max[6])

end

-- give knowledge about traps
function trap_k()
local i = 0
local p = Players[Ind+1]
-- for(i=0;i<255;i++)
	while (i < 256)
	do
		i = i + 1
		p.trap_ident[i]=TRUE
	end
end

-- namely.
function adj_xp(i, amt)
local p = Players[i+1]

	p.max_exp = amt
	p.exp = amt
end

-- resurrect/exterminate all the uniques
function resurrect_uni(state)
local i = 0
local p = Players[Ind+1]
-- for(i=0;i<255;i++)
	while (i < 1152)
	do
		i = i + 1
		p.r_killed[i]=state
	end
end

-- make every item 'known'
function id_all(state)
local i = 0
local p = Players[Ind+1]
-- for(i=0;i<255;i++)
	while (i < 640)
	do
		i = i + 1
		p.obj_aware[i]=state
	end
end

-- lazy command for an admin :)
function id_all2()
	id_all(1)
	trap_k()
end
	

