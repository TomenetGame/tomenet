-- C. Blue's automatic stuff --
-------------------------------

-- Run automatically on server starting up:
function server_startup(timestamp, h, m, s, dwd, dd, dm, dy)
	lua_s_print(timestamp.."_SERVERSTARTUP_"..h..":"..m..":"..s.."-"..dy.."/"..dm.."/"..dd.."("..dwd..")\n")

        --[[automatically fix season if required, for example if server 
            was down for 24+ hours while a season change date occured. 
            see cron_24h() for details and to keep values synchronized.]]
        if (season ~= 0) then
                if ((dm == 3 and dd >= 20) or (dm > 3 and dm < 6) or (dm == 6 and dd < 21)) then
                        lua_season_change(0, 0)
                end
        end
        if (season ~= 1) then
                if ((dm == 6 and dd >= 21) or (dm > 6 and dm < 9) or (dm == 9 and dd < 23)) then
                        lua_season_change(1, 0)
                end
        end
        if (season ~= 2) then
                if ((dm == 9 and dd >= 23) or (dm > 9 and dm < 12) or (dm == 12 and dd < 21)) then
                        lua_season_change(2, 0)
                end
        end
        if (season ~= 3) then
                if ((dm == 12 and dd >= 21) or (dm < 3) or (dm == 3 and dd < 20)) then
                        lua_season_change(3, 0)
                end
        end

--	lua_add_anote("Make sure to regularly visit the forum on www.tomenet.net")
--	lua_add_anote(" and the bulletin board (press !).") --haha
--	lua_add_anote("-All characters are no-ghost.")
--	lua_add_anote("-Only 1 character per player.")
--	lua_add_anote("-NO_DEATH dungeons were laid to ruins.")
--	lua_add_anote("-All town-dungeons are IRONMAN.")
--	lua_add_anote("Stealing from shops puts you on a watch list which runs")
--	lua_add_anote("out after 0..3 day/night breaks (charlevel-dependant).")
--	lua_add_anote("Cash bonus for classes which can't sell startup equipm.")
--	lua_add_anote("Added dungeon shops for IRONMAN dungeons (dlvl 14+).")
--	lua_add_anote("Yeeks get 80% exp penalty (compared to 100% for humans).")
--	lua_add_anote("Speed/HP boni from forms were reduced (mimicry).")
--	lua_add_anote("Just seeing an item in shop 1 to 6 lets you remember it.")
--	lua_add_anote("Townie spawn rate increased.")
--	lua_add_anote("Archers get a brighter lantern (without resistances).")
--	lua_add_anote("Latest scripts at www.c-blue.de/rogue .")
--	lua_add_anote("All dungeons/towers are now ironman. Town-dungeons except")
--	lua_add_anote("for barrow-downs allow recall every 500 ft though. Non-")
--	lua_add_anote("town-dungeons may allow recall on every floor at 20% prb.")
--	lua_add_anote("KNOWN BUGS: Houses turn unowned. Chars vanish from list.")
--	lua_add_anote("Xbow: +1es+1em at 25/50. Bow: +1es at 12.5/25/37.5/50.")
--	lua_add_anote("Sling: +1es at 10/20/30/40/50. Boom:+1es at 16.6/33.3/50.")
--	lua_add_anote("!X on your identify scrolls will auto read everytime you")
--	lua_add_anote("pick up an unknown item ^^.")
--	lua_add_anote("Forum has the list of all the changes.")
--	lua_add_anote("All magic ammos now exist.")
--	lua_add_anote("Wild dungeons don't allow recall before dlvl 30/bottom.")
--	lua_add_anote("Player targetting fixed.")
--	lua_add_anote("Removed item level restriction when weilding an item.")
--	lua_add_anote("Ownership still does not change hand if clvl < itemlvl")
--	lua_add_anote("Townie spawn rate restored but they drop more in Bree.")
--	lua_add_anote("Traps fixed. Flasks allowed for fume traps(requires 4.3.0a).")
--	lua_add_anote("More info on trap(-related items): see forum.")
--	lua_add_anote("New priest spell under Holy Offense: curse.")
--	lua_add_anote("Backstab skill reworked.")
--	lua_add_anote("New effects when quaffed for The Ale of Khazad")
--	lua_add_anote("1 in 3 for each of Speed, Hero, Berserk and")
--	lua_add_anote("1 in 10 for confusion (resistable)")
--	lua_add_anote("New artifact pick. Find it!")
--	lua_add_anote("Ammo dice reduced. Ammo added. Silver ammo KILL_UNDEAD.")
--	lua_add_anote("Sorcery ratio reduced.")
--	lua_add_anote("New expratio system. It affects expgain, not level-req.")
--	lua_add_anote("{@R} now works on spellbooks/tomes with /recall.")
--	lua_add_anote("Specialist stores added")
--	lua_add_anote("New classes: Druid, Shaman (info in the forum now)")
--	lua_add_anote("New highly experimental skill: Traumaturgy.")
--	lua_add_anote("4.3.5 released. LUA auto-update now working correctly.")
--	lua_add_anote("4.3.5 UPDATED AGAIN! (no new version number)")
--	lua_add_anote("Experimental: Shamans can use E and G forms which resemble")
--	lua_add_anote(" spirits/elementals/ghosts, while staying fully equipped.")
--	lua_add_anote(" Shamans receive NO random-movement from any form now.")
--	lua_add_anote("Exp: Encumbering armor reduces to-hit & sneakiness bonus.")
--	lua_add_anote("lua updates, more druid changes. Priest has a new spell.");
--	lua_add_anote("Drinking from fountains might summon a guardian.")
--	lua_add_anote("Several 1h-weapons are now 'May2h'->more bpr if no shield.")
--	lua_add_anote("Spellbook auto-retaliation works now!")
--	lua_add_anote("Inscribe your spellbook @O[spell letter] for auto-retaliation.")
--	lua_add_anote("If there's no letter, the first spell in the book will be cast.")
--	lua_add_anote("(nothing)s should've been fixed. Please report any you see.")
--	lua_add_anote("Please report if you see them again.")
--	lua_add_anote("Use /geinfo and /gesign commands to participate in events.")
--	lua_add_anote("WARNING: Acidic attacks may once again damage equipment!")
--	lua_add_anote("2- and 1.5-handed randart weapons might have changed.")
--	lua_add_anote("  Each player has 2 free rerolls per account.")
--	lua_add_anote("Linux client: X11 files updated on CVS (see forum!).")
--	lua_add_anote("Sorry, due to unforeseen exploiting, ghost-diving was restricted.")
--	lua_add_anote("Mimics get +xbow shot too from all ARROW_123 forms (exp)")
--	lua_add_anote("Increased mimic bpr limit from 4 to 5. Reduced MA by 0.05.")
--	lua_add_anote("Added Diggler's idea of a bbs. Type /bbs or press '!' key!")
--	lua_add_anote("Healing spell of Holy Curing is now level 3 instead of 10.")
--	lua_add_anote("Projecting Holy prayers doesn't need 'project' any more!")
--	lua_add_anote("!R will inhibit shots from ricocheting.")
--	lua_add_anote("Havoc rods no longer stack.")
--	lua_add_anote("Toxic Moisture now only does poison effect at level <= 20")
--	lua_add_anote("Rogue class get auto-detection (in radius) \"esp\" while in ")
--	lua_add_anote(" searching mode. It has 5..15 radius and (25+level)% to ")
--	lua_add_anote(" reveal a notable terrain object (i.e., stairs, treasures, ")
--	lua_add_anote(" traps, hidden doors)")
--	lua_add_anote("!X now works on perception staves (NOT *perc*) and rods ")
--	lua_add_anote("Bloodbond fights should not consume the backpack anymore")
--	lua_add_anote("New class. Some documentation @ forum... ")
--	lua_add_anote("Added water rune (dmg, intermediate mana usage)")
--	lua_add_anote("Runes can break now. If it does, only the mod rune will.")
--	lua_add_anote("Gravity rune == a \"toggle\" buff. While active, it will")
--	lua_add_anote(" increase the amount of SP use for any spell you cast.")
--	lua_add_anote("Added the speed rune as well @ shop. For now")
--	lua_add_anote("Pardon the targetting dodginess. Will fix it soon.")
--	lua_add_anote("Buffer runes (e.g., gravity) will work with any base")
--	lua_add_anote("Health adds +10% hp at 30, 40 and 50. Unstacking with items")
--	lua_add_anote("Lowered havoc rod's dmg and recharge time. Improves with MD.")
--	lua_add_anote("Multi-hued dragon scale mail gives 2 random immunities.")
--	lua_add_anote("Health changed: Adds skill*2 HP. Stacks with +LIFE. If you")
--	lua_add_anote(" want to redistribute your 'Health' points now, just ask.")
--	lua_add_anote("Client 4.4.0a released. See the forum.")
--	lua_add_anote("Earthquake damage is now truly capped to 300.")
--	lua_add_anote("A single breath or ball can hit you only once now.")
--	lua_add_anote("You should avoid using the buggy 4.4.0a client.")
--	lua_add_anote("Ice Storm now fully covers its radius.")
--	lua_add_anote("Spell damage is now displayed in PvP.")
--	lua_add_anote("Currently testing new stuff on this server:")
--	lua_add_anote("Shields have block chance and can deflect balls too.")
--	lua_add_anote("Players may parry (weaponmastery-skill dependant) and block.")
--	lua_add_anote("Weight limit enforced more strictly; new light armour added.")
--	lua_add_anote("New super-heavy kings-only armour added for fighter-classes.")
--	lua_add_anote("Martial arts skill supports Dodging skill.")
--	lua_add_anote("New race class restriction enabled. Test them please.")
--	lua_add_anote("NEW: Monsters for Divine race quest aren't unique.")
--	lua_add_anote(" Kill ONE TYPE ONLY before level 20 or else you'd die.")
--	lua_add_anote("Divines may kill ONLY ONE TYPE before level 20, or they die.")
--	lua_add_anote("level <40 spells work now for divines")
--	lua_add_anote("New client 4.4.0c released. Makes '/page' work again.");
--	lua_add_anote("4.4.0c Zip was updated once more, sorry. No more console!");
--	lua_add_anote("Dual-Wield skill now available to rogues and warriors.");
--	lua_add_anote("{RWarning: *Destruction* now causes knockout on everyone.");
--	lua_add_anote("{GNew client 4.4.0d released. Required to play properly.");
--	lua_add_anote("Combat stance skill now available to warriors via 'm' key.");
--	lua_add_anote("Cloaking mode now available to rogues via 'V' key.");
--	lua_add_anote("Added dual-wield for rangers, following Theyli's suggestion.");
--	lua_add_anote("Made stances also available to mimics, paladins, rangers.");
--	lua_add_anote("New dodging formulas implemented!");
--	lua_add_anote("{GMake sure you use client version 4.4.0e to avoid crashing!");
	lua_add_anote("Recalling into town dungeons possible every 1000+50 ft!");
	lua_add_anote(" Eg: -50ft, -1050ft, -2050ft..");
--	lua_add_anote("Starting phase scrolls replaced with au. Buy your own.");
--	lua_add_anote("/undoskills will restore skill points you spent after lvup.");
--	lua_add_anote("Sorry guys, there was a major flaw in randart routine.");
--	lua_add_anote("ALL randarts will probably have changed!");
--	lua_add_anote("Cloaking should no longer break from terrain damage (lava).");
--      lua_add_anote("Added new rogue skills (untested). Older clients display");
--	lua_add_anote(" them wrongly in 'm' menu, so get 4.4.1c.");
--	lua_add_anote("{vClient 4.4.1c released, update mostly required.");
--	lua_add_anote("Fully implemented 'fire-till-kill' mode (see 'm')!");                         
--	lua_add_anote(" (It'll stop firing if sleeping monsters get in the way.)");                   
--	lua_add_anote("Some randarts changed. We reroll your stuff as usual.");
--      lua_add_anote("{vClient 4.4.1c r2 released, re-dowload from c-blue.de/rogue");              
--      lua_add_anote(" (No more crashing when you try to dump macros to a file.)");                
--	lua_add_anote("{oRumours have it the Great Pumpkin appeared!");
--      lua_add_anote("{HClient 4.4.1d is out! Get it to be ready for upcoming");
--      lua_add_anote("{H server move! Also contains couple of important fixes!");
--	lua_add_anote("{vClient 4.4.1d is out. Update very important.");
--	lua_add_anote("{v- - - - Client 4.4.1e released! - - -");
	lua_add_anote("Reduced min floor level for allowing recall from 30 to 20.");
	lua_add_anote("Mindcrafter class stats: INT primary, CHR 2nd, WIS 3rd.");
	lua_add_anote("Mindcrafter spells can be viewed in Bree's Mathom-House.");
	lua_add_anote("Divine's skill reset at lvl 20 should work now. You will");
	lua_add_anote(" get teleported once.");
	lua_add_anote("{v---- Client 4.4.2 released (see forum)! ----");

-- Admin parameters */
	watch_nr = 1
	watch_morgoth = 1

-- Modify 'updated_savegame' value for newly created chars [0]
	updated_savegame_birth = 1

-- Update spellbooks if one was added/removed (part 1 of 2)
	if updated_server == 0 then
		lua_fix_spellbooks(MCRYOKINESIS + 1, -1)
		updated_server = 1
	end
end

-- Run additionally when 1st player joins since starting up: (character fully loaded at this point)
function first_player_has_joined(num, id, name, timestamp)
end

-- Run additionally if server was currently empty: (character fully loaded at this point)
function player_has_joined_empty_server(num, id, name, timestamp)
end

-- Run automatically if a player joins: (character fully loaded at this point)
function player_has_joined(num, id, name, timestamp)
--	if (players(num).admin_dm == 0) then
--		i = randint(5);
--		if (i == 1) then msg_broadcast(0, "\255U[tBot] Whazzup " .. name);
--		elseif (i == 2) then msg_broadcast(0, "\255U[tBot] Hey " .. name);
--		elseif (i == 3) then msg_broadcast(0, "\255U[tBot] Sup " .. name);
--		elseif (i == 4) then msg_broadcast(0, "\255U[tBot] Yo " .. name);
--		elseif (i == 5) then msg_broadcast(0, "\255U[tBot] Hello " .. name);
--	end
	if (name == "Bolt Lightning") then
		msg_broadcast(0, "ZOMG it's Bolt Lightning!")
	end

-- Reset true artifacts for this player (after '/art reset!').
-- Note: No additional changes anywhere are required for artifact reset.
	if players(num).updated_savegame == -1 then
		players(num).updated_savegame = 0
		lua_strip_true_arts_from_present_player(num, 1)
	end

-- Update spellbooks if one was added/removed (part 2 of 2)
	if players(num).updated_savegame == 0 then
		fix_spellbooks(name, MCRYOKINESIS + 1, -1)
		players(num).updated_savegame = 1
	end

-- Update skill chart layout (leaves skill values and points untouched)
	lua_fix_skill_chart(num)
end

-- Run automatically when a player leaves the server:
function player_leaves(num, id, name, timestamp)
end

-- Run when a player leaves, but char stays in dungeon, waiting for timeout:
function player_leaves_timeout(num, id, name, timestamp)
end

-- Run additonally when a player has left the server (char saved and gone):
function player_has_left(num, id, name, timestamp)
end

-- Run additionally when the last player has left and server becomes empty:
function last_player_has_left(num, id, name, timestamp)
--	clean up all towns to get rid of (nothing)s
--	lua_s_print("Cleaning towns.\n")
--	lua_towns_treset()
end

-- Run once every hour:
function cron_1h(timestamp, h, m, s)
	lua_s_print(timestamp.."_CRON1H_"..h..":"..m..":"..s.."\n")

	lua_start_global_event(0, 1, "")

	if mod(h,3)==0 then
		lua_start_global_event(0, 3, "")
	end
end

-- Run once every 24 hours:
function cron_24h(timestamp, h, m, s, dwd, dd, dm, dy)
	lua_s_print(timestamp.."_CRON24H_"..h..":"..m..":"..s.."-"..dy.."/"..dm.."/"..dd.."("..dwd..")\n")
	
	--[[season changes (averaged, may in reality vary +/- 1 day:
	    spring 20.march, summer 21.june,
	    autumn 23. september, winter 21. december ]]
	if (dd == 20 and dm == 3) then lua_season_change(0, 0) end
	if (dd == 21 and dm == 6) then lua_season_change(1, 0) end
	if (dd == 23 and dm == 9) then lua_season_change(2, 0) end
	if (dd == 21 and dm == 12) then lua_season_change(3, 0) end
end
