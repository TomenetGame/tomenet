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
function module_save(wx,wy,wz,name)
  local wpos = make_wpos(wx,wy,wz)
  local path = lua_path_build(name)

  -- msg_print(Ind,"Saving Module File: "..path)
  -- msg_print(Ind,"wpos: ("..wx..","..wy..","..wz..")")
  
  local file = writeto(""..path, "w");

  -- dimensions
  local w = dun_get_wid(wpos)
  local h = dun_get_hgt(wpos)

  -- msg_print(Ind,"WRITE (w,h): ("..w..","..h..")")
  write(w.."\n")
  write(h.."\n")

  -- features, monsters, items
  for x = 0, MAX_WID do
    for y = 0, MAX_HGT do
      local f,r,e,t,s = 0,0,0,0,0

      f = check_feat(wpos,y,x)
      write(f.."\n")
      r = check_monster(wpos,y,x)
      write(r.."\n")
      e = check_monster_ego(wpos,y,x)
      write(e.."\n")
      t = check_item_tval(wpos,y,x)
      write(t.."\n")
      s = check_item_sval(wpos,y,x)
      write(s.."\n")
    end
  end

  -- entry positions
  local ux = level_up_x(wpos)
  local ux = level_up_x(wpos)
  local uy = level_up_y(wpos)
  -- msg_print(Ind,"WRITE (ux,uy): ("..ux..","..uy..")")
  write(ux.."\n")
  write(uy.."\n")
 
  local dx = level_down_x(wpos)
  local dy = level_down_y(wpos)
  -- msg_print(Ind,"WRITE (dx,dy): ("..dx..","..dy..")")
  write(dx.."\n")
  write(dy.."\n")

  local rx = level_rand_x(wpos)
  local ry = level_rand_y(wpos)
  -- msg_print(Ind,"WRITE (rx,ry): ("..rx..","..ry..")")
  write(rx.."\n")
  write(ry.."\n")
  
  closefile(file)

  return 0
end

-- load a floor
function module_load(wx,wy,wz,name,light)
  if wz == 0 then return 1 end -- paranoia - no surface modules, yet

  local wpos = make_wpos(wx,wy,wz)
  local path = lua_path_build(name)

  -- msg_print(Ind,"Loading Module File: "..path)
  -- msg_print(Ind,"wpos: ("..wx..","..wy..","..wz..")")

  local file = readfrom(""..path, "r");

  local w = read("*n")
  local h = read("*n")
  -- msg_print(Ind,"READ (w,h): ("..w..","..h..")")

  local W = (MAX_WID - w) * 2 / SCREEN_WID
  local H = (MAX_HGT - h) * 2 / SCREEN_HGT
  -- msg_print(Ind,"READ (W,H): ("..W..","..H..")")

  local x, y, f, r, e, t, s

  generate_cave_blank(wpos,W,H,light)
  summon_override(1) -- SO_ALL
  for x = 0, MAX_WID do
    for y = 0, MAX_HGT do
      f = read("*n")
      cave_set_feat(wpos,y,x,f)
      r,e = 0,0
      r = read("*n")
      e = read("*n")
      if r ~= 0 then
        place_monster_ego(wpos,y,x,r,e,1,0,0,0) -- sleep 1
      end
      t,s = 0,0
      t = read("*n")
      s = read("*n")
      if t ~= 0 then
        place_item_module(wpos,y,x,t,s)
      end
    end
  end
  summon_override(0) -- SO_NONE

  -- entry positions
  local ux = read("*n")
  local uy = read("*n")
  -- msg_print(Ind,"READ (ux,uy): ("..ux..","..uy..")")
  new_level_up_x(wpos,ux)
  new_level_up_y(wpos,uy)

  local dx = read("*n")
  local dy = read("*n")
  -- msg_print(Ind,"READ (dx,dy): ("..dx..","..dy..")")
  new_level_down_x(wpos,dx)
  new_level_down_y(wpos,dy)

  local rx = read("*n")
  local ry = read("*n")
  -- msg_print(Ind,"READ (rx,ry): ("..rx..","..ry..")")
  new_level_rand_x(wpos,rx)
  new_level_rand_y(wpos,ry)

  closefile(file)

  return 0
end
