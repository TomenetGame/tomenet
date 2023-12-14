-- File: lib/scpt/module.lua - Kurzel
-- https://www.lua.org/manual/4.0/manual.html#6.4

-- Server-side use only.
-- DM menu "]" helper functions to save/load/handle module components.
-- To reload this file as a DM (thus applying changes w/o a restart):
-- / pern_dofile(0, "module.lua")

MAX_WID = 198
MAX_HGT = 66

SCREEN_WID = 66
SCREEN_HGT = 22

-- Build a path, hardcoded to /lib/save w/ .module extension for now...
function lua_path_build(name) -- LUA (Kurzel?) has trouble passing by reference! UwU
  -- return *path_build(char *buf?, 1024, ANGBAND_DIR_SAVE, name)
  return "lib/modules/"..name -- less clutter in ANGBAND_DIR_SAVE
end

-- save a floor
function module_save(Ind,name)
  -- msg_print(Ind,"LUA: ".._VERSION)
  path = lua_path_build(name)

  msg_print(Ind,"Saving Module File: "..path)
  
  msg_print(Ind,"DM @ wpos: "..players(Ind).name.." @ ("..players(Ind).wpos.wx..","..players(Ind).wpos.wy..","..players(Ind).wpos.wz..")")
  
  file = writeto(""..path, "w");

  -- dimensions
  w = dun_get_wid(players(Ind).wpos)
  h = dun_get_hgt(players(Ind).wpos)
  -- msg_print(Ind,"WRITE (w,h): ("..w..","..h..")")
  write(w.."\n")
  write(h.."\n")

  -- features
  for x = 1, MAX_WID do
		for y = 1, MAX_HGT do
      f = check_feat(players(Ind).wpos,y-1,x-1)
      write(f.."\n")
    end
  end

  -- entry positions
  ux = level_up_x(players(Ind).wpos)
  uy = level_up_y(players(Ind).wpos)
  -- msg_print(Ind,"WRITE (ux,uy): ("..ux..","..uy..")")
  write(ux.."\n")
  write(uy.."\n")
  dx = level_down_x(players(Ind).wpos)
  dy = level_down_y(players(Ind).wpos)
  -- msg_print(Ind,"WRITE (dx,dy): ("..dx..","..dy..")")
  write(dx.."\n")
  write(dy.."\n")
  rx = level_rand_x(players(Ind).wpos)
  ry = level_rand_y(players(Ind).wpos)
  -- msg_print(Ind,"WRITE (rx,ry): ("..rx..","..ry..")")
  write(rx.."\n")
  write(ry.."\n")
  
  -- monster races
  for x = 1, MAX_WID do
		for y = 1, MAX_HGT do
      r = check_monster(players(Ind).wpos,y-1,x-1)
      write(r.."\n")
      e = check_monster_ego(players(Ind).wpos,y-1,x-1)
      write(e.."\n")
    end
  end

  closefile(file)

  return 0
end

-- load a floor
function module_load(Ind,name)
  path = lua_path_build(name)

  msg_print(Ind,"Loading Module File: "..path)
  msg_print(Ind,"DM @ wpos: "..players(Ind).name.." @ ("..players(Ind).wpos.wx..","..players(Ind).wpos.wy..","..players(Ind).wpos.wz..")")

  file = readfrom(""..path, "r");

  w = read("*n")
  h = read("*n")
  -- msg_print(Ind,"READ (w,h): ("..w..","..h..")")

  W = (MAX_WID - w) * 2 / SCREEN_WID
  H = (MAX_HGT - h) * 2 / SCREEN_HGT
  -- msg_print(Ind,"READ (W,H): ("..W..","..H..")")

  -- no better way to pass wpos? LUA doesn't pass by reference / C structs
  wx = players(Ind).wpos.wx
  wy = players(Ind).wpos.wy
  wz = players(Ind).wpos.wz
  generate_cave_blank(wx,wy,wz,W,H) -- summon_override_checks = SO_ALL;
  -- no longer on floor
  twx = players(Ind).wpos.wx
  twy = players(Ind).wpos.wy
  twz = players(Ind).wpos.wz
  -- hijack the DM wpos to target the floor wpos for cave_set_feat()
  tmp = players(Ind).wpos
  tmp.wx = wx
  tmp.wy = wy
  tmp.wz = wz
  for x = 1, MAX_WID do
		for y = 1, MAX_HGT do
      f = read("*n")
      -- msg_print(Ind,"READ feat: "..f) -- crashes client
      cave_set_feat(tmp,y-1,x-1,f)
    end
  end

  -- entry positions
  ux = read("*n")
  uy = read("*n")
  -- msg_print(Ind,"READ (ux,uy): ("..ux..","..uy..")")
  new_level_up_x(players(Ind).wpos,ux)
  new_level_up_y(players(Ind).wpos,uy)
  dx = read("*n")
  dy = read("*n")
  -- msg_print(Ind,"READ (dx,dy): ("..dx..","..dy..")")
  new_level_down_x(players(Ind).wpos,dx)
  new_level_down_y(players(Ind).wpos,dy)
  rx = read("*n")
  ry = read("*n")
  -- msg_print(Ind,"READ (rx,ry): ("..rx..","..ry..")")
  new_level_rand_x(players(Ind).wpos,rx)
  new_level_rand_y(players(Ind).wpos,ry)

  -- monster races, summon = target empty grid, always awake
  for x = 1, MAX_WID do
		for y = 1, MAX_HGT do
      r,e = 0,0
      r = read("*n")
      e = read("*n")
      if r ~= 0 then
        place_monster_one(tmp,y-1,x-1,r,e,0,1,0,0) -- sleep 1
      end
    end
  end

  -- restore the DM
  tmp.wx = twx
  tmp.wy = twy
  tmp.wz = twz

  closefile(file)

  return 0
end
