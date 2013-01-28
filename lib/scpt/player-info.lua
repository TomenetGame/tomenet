-- $Id: player-info.lua,v 1.18 2007/12/28 16:44:42 cblue Exp $

--
-- simple scripts to examine/modify players' status.
--

-- display some information about a specified player.
function p_in(i)
-- +1 offset
local p = players(i)

-- Ind is global.
	msg_print(Ind, "location: ("..p.px..","..p.py..") of ["..(p.wpos.wx)..","..(p.wpos.wy).."], "..(p.wpos.wz*50).."ft  AU:"..p.au)
	msg_print(Ind, "HP:"..p.chp.."/"..p.mhp.."  SP:"..p.csp.."/"..p.msp.."  SN:"..p.csane.."/"..p.msane.."  XP:"..p.exp.."/"..p.max_exp)
	msg_print(Ind, "St:"..p.stat_cur[1].."/"..p.stat_max[1].." In:"..p.stat_cur[2].."/"..p.stat_max[2].." Wi:"..p.stat_cur[3].."/"..p.stat_max[3].." Dx:"..p.stat_cur[4].."/"..p.stat_max[4].." Co:"..p.stat_cur[5].."/"..p.stat_max[5].." Ch:"..p.stat_cur[6].."/"..p.stat_max[6])

end

-- display some information about a specified object in inventory.
-- (prolly easier to use gdb..)
function o_in(i, j)
-- +1 offset
local o = players(i).inventory[j+1]

-- Ind is global.
	msg_print(Ind, "location: ("..o.ix..","..o.iy..") of ["..(o.wpos.wx)..","..(o.wpos.wy).."], "..(o.wpos.wz*50).."ft")
	msg_print(Ind, "k_idx:"..o.k_idx.." tval:"..o.tval.." sval:"..o.sval.." bpval:"..o.bpval.." pval:"..o.pval)
	msg_print(Ind, "name1:"..o.name1.." name2:"..o.name2.." name3:"..o.name3.." xtra1:"..o.xtra1.." xtra2:"..o.xtra2)
	msg_print(Ind, "timeout:"..o.timeout.." ident:"..o.ident.." note:"..o.note)

end

-- give knowledge about traps
function trap_k()
local i = 0
local p = players(Ind)
-- for(i=0;i<255;i++)
	while (i < 256)
	do
		i = i + 1
		p.trap_ident[i]=TRUE
	end
end

-- namely.
function adj_xp(i, amt)
local p = players(i)

	p.max_exp = amt
	p.exp = amt
end

-- resurrect/exterminate all the uniques
-- also nice to test mimics.
function res_uni(state)
local i = 0
local p = players(Ind)
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
local p = players(Ind)
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

-- namely.
function healup(i)
local p = players(i)
local j = 0

	p.exp = p.max_exp
	p.chp = p.mhp
	p.csp = p.msp
	p.csane = p.msane
	p.black_breath = 0
	p.slow = 0
	p.blind = 0
	p.paralyzed = 0
	p.confused = 0
	p.afraid = 0
	p.image = 0
	p.poisoned = 0
	p.cut = 0
	p.stun = 0
	p.food = 10000

	while (j < 6)
	do
		j = j + 1
		p.stat_cur[j] = p.stat_max[j]
	end
end

-- reload lua files.
--DEPRECATED AND BAD. use /initlua slash.c command instead!
function init()
	pern_dofile(Ind, "cblue.lua")
--	pern_dofile(Ind, "dg.lua")
--	pern_dofile(Ind, "evil.lua")
--	pern_dofile(Ind, "zz.lua")
--	pern_dofile(Ind, "jir.lua")
	pern_dofile(Ind, "it.lua")
	pern_dofile(Ind, "mikaelh.lua")

	pern_dofile(Ind, "custom.lua")
	pern_dofile(Ind, "player-info.lua")

--	how about adding all the updatable luas here? we'd just need to include an erase-all function for schools etc.
end

-- get all the skills (cept antimagic)
function learn()
local i = 0
local p = players(Ind)
-- 0xffffffff
	while (i < MAX_SKILLS)
	do
		i = i + 1
--		if p.s_info[i].mod then
			p.s_info[i].value = 50000
--		end
	end

	antimagic(0)
end

-- set specified skill
function skill(skill, val)
local p = players(Ind)
	p.s_info[skill + 1].value = val
end

-- set antimagic skill (you'll need it :)
function antimagic(val)
local p = players(Ind)
	p.s_info[SKILL_ANTIMAGIC + 1].value = val
end

