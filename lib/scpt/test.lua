function test(i, i2)
	p = Players[i+1]
	p2 = Players[i2+1]
        
        msg_broadcast(0, "teleporting to "..(p.py-1).." "..p.px)
        teleport_player_to(i2, p.py-1, p.px)
end
