--
-- display some information about a specified player.
-- 
function p_in(i)
-- +1 offset
	p = Players[i+1]
        
-- Ind is global.
	msg_print(Ind, "location: ("..p.px..","..p.py..") of "..(p.dun_depth*50).."ft  AU:"..p.au)
	msg_print(Ind, "HP:"..p.chp.."/"..p.mhp.."  SP:"..p.csp.."/"..p.msp.."  XP:"..p.exp.."/"..p.max_exp)
	msg_print(Ind, "St:"..p.stat_cur[1].."/"..p.stat_max[1].." In:"..p.stat_cur[2].."/"..p.stat_max[2].." Wi:"..p.stat_cur[3].."/"..p.stat_max[3].." Dx:"..p.stat_cur[4].."/"..p.stat_max[4].." Co:"..p.stat_cur[5].."/"..p.stat_max[5].." Ch:"..p.stat_cur[6].."/"..p.stat_max[6])

end
--
-- give knowledge about traps
-- 
function trap_k()
p = Players[Ind]
for(i=0;i<255;i++)
	{
		p.trap_ident[i]=TRUE
	}
        
end

