-- File: lib/scpt/runecraft.lua - Kurzel

-- MASKS

R1 = bshl(255,0)
R2 = bshl(255,8)
PROJ = bor(R1,R2)
MODE = bshl(255,16)
TYPE = bshl(255,24)
WARN = 0

-- FLAGS

LITE = bshl(1,0)
DARK = bshl(1,1)
NEXU = bshl(1,2)
NETH = bshl(1,3)
CHAO = bshl(1,4)
MANA = bshl(1,5)
WARN = bor(WARN,bshl(1,6))
WARN = bor(WARN,bshl(1,7))

MINI = bshl(1,16)
LENG = bshl(1,17)
COMP = bshl(1,18)
MDRT = bshl(1,19)
ENHA = bshl(1,20)
EXPA = bshl(1,21)
BRIE = bshl(1,22)
MAXI = bshl(1,23)

BOLT = bshl(1,24)
CLOU = bshl(1,25)
BALL = bshl(1,26)
STRM = bshl(1,27)
CONE = bshl(1,28)
SURG = bshl(1,29)
FLAR = bshl(1,30)
WARN = bor(WARN,bshl(1,31))

-- TABLES

R = { -- Rune Skill
[LITE] = { "Light",    SKILL_R_LITE },
[DARK] = { "Darkness", SKILL_R_DARK },
[NEXU] = { "Nexus",    SKILL_R_NEXU },
[NETH] = { "Nether",   SKILL_R_NETH },
[CHAO] = { "Chaos",    SKILL_R_CHAO },
[MANA] = { "Mana",     SKILL_R_MANA }}

P = { -- Projection GF_TYPE Weight Colour
[LITE]           = { "light",           GF_LITE,       400, "L" },
[DARK]           = { "darkness",        GF_DARK,       550, "A" },
[NEXU]           = { "nexus",           GF_NEXUS,      250, "x" }, -- ADD tele,ruination
[NETH]           = { "nether",          GF_NETHER,     550, "n" }, -- ADD drain (invert GF_EXP)
[CHAO]           = { "chaos",           GF_CHAOS,      600, "m" }, -- ADD drain
[MANA]           = { "mana",            GF_MANA,       600, "N" },
[bor(LITE,DARK)] = { "confusion",       GF_CONFUSION,  400, "C" },
[bor(LITE,NEXU)] = { "inertia",         GF_INERTIA,    200, "q" },
[bor(LITE,NETH)] = { "electricity",     GF_ELEC,      1200, "e" },
[bor(LITE,CHAO)] = { "fire",            GF_FIRE,      1200, "f" },
[bor(LITE,MANA)] = { "water",           GF_WATER,      300, "Y" }, -- FIX confusion,stun
[bor(DARK,NEXU)] = { "gravity",         GF_GRAVITY,    150, "V" }, -- FIX slow,stun,phase
[bor(DARK,NETH)] = { "cold",            GF_COLD,      1200, "c" },
[bor(DARK,CHAO)] = { "acid",            GF_ACID,      1200, "a" },
[bor(DARK,MANA)] = { "poison",          GF_POIS,       800, "p" }, -- ADD m_ptr->poisoned
[bor(NEXU,NETH)] = { "time",            GF_TIME,       150, "t" }, -- ADD drain,ruination
[bor(NEXU,CHAO)] = { "sound",           GF_SOUND,      400, "S" },
[bor(NEXU,MANA)] = { "shards",          GF_SHARDS,     400, "H" }, -- ADD m_ptr->bleeding
[bor(NETH,CHAO)] = { "hellfire",        GF_HELLFIRE,   400, "X" },
[bor(NETH,MANA)] = { "force",           GF_FORCE,      250, "F" },
[bor(CHAO,MANA)] = { "disenchantment",  GF_DISENCHANT, 500, "T" }} -- ADD unpower? ^^ Players too!

M = { -- Mode Level Cost Fail Damage Radius Duration Energy
[MINI] = { "minimized",   0,  6, -20,  6, -1,  8, 10 },
[LENG] = { "lengthened",  2,  8, -10,  8,  0, 14, 10 },
[COMP] = { "compressed",  3,  7,  -5,  9, -2, 12, 10 },
[MDRT] = { "moderate",    5, 10,   0, 10,  0, 10, 10 },
[ENHA] = { "enhanced",    5, 10,   5, 10,  0, 10, 10 },
[EXPA] = { "expanded",    7, 14,  10,  8,  2,  8, 10 },
[BRIE] = { "brief",       8,  7,  20,  6,  0,  6,  5 },
[MAXI] = { "maximized",  10, 18,  40, 14,  1, 12, 10 }}

T = { -- Type Level Cost Max Dice Max Damage Max Radius Max Duration Max
[BOLT] = { "bolt",    5,  2, 15, 4, 46,  2,  26, 0,  0,  0,  0},
[CLOU] = { "cloud",  10,  4, 20, 0,  0,  3,  75, 2,  2,  3,  7},
[BALL] = { "ball",   15,  8, 25, 0,  0, 90, 450, 3,  3,  0,  0},
[STRM] = { "storm",  20, 16, 30, 0,  0, 20, 135, 1,  1,  7, 27},
[CONE] = { "cone",   25, 16, 40, 4, 46,  2,  26, 3,  3,  0,  0},
[SURG] = { "surge",  30, 24, 50, 0,  0, 30, 240, 7, 13,  0,  0},
[FLAR] = { "flare",  35, 25, 25, 4, 46,  2,  26, 0,  0,  2,  2}}

E = { -- Enhanced Level Cost Max Dice Max Damage Max Radius Max Duration Max
[BOLT] = { "beam",   10,  4, 20, 4, 46,  2,  26, 0,  0,  0,  0},
[CLOU] = { "wall",   15,  6, 30, 0,  0, 20, 135, 0,  0,  8, 20},
[BALL] = { "burst",  20, 16, 40, 0,  0, 90, 450, 2,  2,  0,  0},
[STRM] = { "nimbus", 25, 25, 25, 0,  0, 16,  40, 1,  1, 30, 75},
[CONE] = { "shot",   30,  6, 42, 4, 46,  2,  15, 9,  9,  0,  0},
[SURG] = { "glyph",  35, 40, 40, 4, 25,  2,  20, 1,  1,  0,  0},
[FLAR] = { "nova",   40, 99, 99, 4, 46,  2,  26, 0,  0,  7,  7}}

-- HACKS

adj_mag_fail = {99,99,99,99,99,50,30,20,15,12,11,10,9,8,7,6,6,5,5,5,4,4,4,4,3,3,2,2,2,2,1,1,1,1,1,0,0,0}
adj_mag_stat = {0,0,0,1,1,1,2,2,3,3,4,4,5,6,7,8,9,10,11,12,13,14,16,18,21,24,27,30,33,36,39,42,45,48,51,54,57,60}

function hamming(u)
  local x = 0
  for i = 0,31 do
    x = x + bshr(band(u,bshl(1,i)),i)
  end
  return x
end

-- COMMON

function rspell_skill(I,u)
  local p = I~=0 and players(I) or player
  local a = p.s_info[1 + R[band(u,R1)][2]].value / 1000
  local b = p.s_info[1 + R[bshr(band(u,R2),8)][2]].value / 1000
  if a < b then
    return a
  else
    return b
  end
end

function rspell_level(u)
  return M[band(u,MODE)][2] + T[band(u,TYPE)][2]
end

function rspell_ability(s,l)
  return s - l + 1
end

function rspell_scale(s,l,h)
  return l + ((h - l) * s / 50)
end

function rspell_cost(u,s)
  local XX = band(u,ENHA)~=0 and E[band(u,TYPE)] or T[band(u,TYPE)]
  local l = XX[3]
  local x = rspell_scale(s, l, XX[4]) * M[band(u,MODE)][3] / 10
  return x < l and l or x
end

function rspell_failure(p,u,x,c)
  if p.cmp == nil then xxx = p.csp else xxx = p.cmp end
  if x < 1 or xxx < c then
    return 100
  else
    x = 15 - (x > 15 and 15 or x)
    x = x * 3 - 13 + M[band(u,MODE)][4]
  end
  x = x - (((adj_mag_stat[p.stat_ind[1+A_INT]] * 65 + adj_mag_stat[p.stat_ind[1+A_DEX]] * 35) / 100) - 3)
  local minfail = (adj_mag_fail[p.stat_ind[1+A_INT]] * 65 + adj_mag_fail[p.stat_ind[1+A_DEX]] * 35) / 100
  if x < minfail then x = minfail end
  if p.blind~=0 then x = x + 10 end
  if p.stun > 50 then
    x = x + 25
  elseif p.stun~=0 then
    x = x + 15
  end
  return x > 95 and 95 or x
end

function rspell_sval(u)
  return bor(band(u,R1),bshr(band(u,R2),8))
end

function rspell_damage(u,s)
  local XX = band(u,ENHA)~=0 and E[band(u,TYPE)] or T[band(u,TYPE)]
  -- old elemental damage spread, scale down with weight/4 from 1200
  -- local w = (P[rspell_sval(u)][3] * 25 + 1200 * (100 - 25)) / 100
  -- local m = M[band(u,MODE)][5]
  -- local x = rspell_scale(s, XX[5], XX[6] * w / 1200)
  -- local y = rspell_scale(s, XX[7], XX[8] * m / 10)
  -- local d = rspell_scale(s, XX[7], ((XX[8] * w) / 1200) * m / 10)
  -- new elemental damage spread, scale down or up with weight/3 from 600
  local w = (P[rspell_sval(u)][3] * 33 + 600 * (100 - 33)) / 100
  -- rare element nerf -- if scaling down, weight linearly
  if P[rspell_sval(u)][3] < 600 then
    w = P[rspell_sval(u)][3]
  end
  local m = M[band(u,MODE)][5]
  local x = rspell_scale(s, XX[5], XX[6] * w / 600)
  local y = rspell_scale(s, XX[7], XX[8] * m / 10)
  local d = rspell_scale(s, XX[7], ((XX[8] * w) / 600) * m / 10)
  return x,y,d
end

function rspell_radius(u,s)
  local XX = band(u,ENHA)~=0 and E[band(u,TYPE)] or T[band(u,TYPE)]
  local x = rspell_scale(s, XX[9], XX[10]) + M[band(u,MODE)][6]
  return x < 1 and 1 or x
end

function rspell_duration(u,s)
  local XX = band(u,ENHA)~=0 and E[band(u,TYPE)] or T[band(u,TYPE)]
  local x = rspell_scale(s, XX[11], XX[12]) * M[band(u,MODE)][7] / 10
  return x < 3 and 3 or x
end

function rspell_energy(I,u)
  local p = I~=0 and players(I) or player
  return level_speed(p.wpos) * M[band(u,MODE)][8] / 10
end

-- CLIENT

function rcraft_end(u)
  return band(u,TYPE)
end

function rcraft_com(u)
  if band(u,MODE)~=0 then
    return "(Types a-g, *=List, ESC=exit) Which type? "
  elseif band(u,R2)~=0 then
    return "(Modes a-h, *=List, ESC=exit) Which mode? "
  else
    return "(Runes a-f, *=List, ESC=exit) Which rune? "
  end
end

function rcraft_max(u)
  if band(u,MODE)~=0 then
    return 6
  elseif band(u,R2)~=0 then
    return 7
  else
    return 5
  end
end

function rcraft_prt(u,w)
  local U,C,row,col
  if w~=0 then
    C = TERM_GREEN
    row,col = 9,16
  else
    C = TERM_WHITE
    row,col = 1,13
  end
  if band(u,MODE)~=0 then
    if w~=0 then
      c_prt(C,"Please choose the component type of the spell.",row,col)
      row = row + 1
      c_prt(C,"ie. Select the spell form:",row,col)
      row = row + 2
    end
    c_prt(C,"   Type     Level Cost Fail Info",row,col)
    local XX,l,s,a,c,f,x,y,d,r,t
    local X = (band(u,ENHA)==0)
    local p = player
    for i = 0,rcraft_max(u) do
      U = bshl(1,i+24)
      XX = X and T[U] or E[U]
      U = bor(u,U)
      l = rspell_level(U)
      s = rspell_skill(0,U)
      a = rspell_ability(s,l)
      c = rspell_cost(U,s)
      f = rspell_failure(p,U,a,(w~=0) and 0 or c)
      x,y,d = rspell_damage(U,s)
      r = rspell_radius(U,s)
      t = rspell_duration(U,s)
      if a > 0 then
        C = TERM_L_GREEN
        if w==0 then
          if p.stun~=0 or p.blind~=0 then C = TERM_YELLOW end
          if p.cmp == nil then xxx = p.csp else xxx = p.cmp end
          if xxx < c then C = TERM_ORANGE end
          if p.antimagic~=0 and p.admin_dm==0 then C = TERM_RED end -- UNHACK
          if p.anti_magic~=0 then C = TERM_RED end
        end
      else
        C = TERM_L_DARK
      end
      if band(U,BOLT)~=0 then
        c_prt(C, format("%c) %-8s %5d %4d %3d%% dam %dd%d",
          strbyte('a')+i, XX[1], a, c, f,
          x, y),
        row+i+1, col)
      elseif band(U,CLOU)~=0 then
        if X then
          c_prt(C, format("%c) %-8s %5d %4d %3d%% dam %d rad %d dur %d",
            strbyte('a')+i, XX[1], a, c, f,
            d, r, t),
          row+i+1, col)
        else
          c_prt(C, format("%c) %-8s %5d %4d %3d%% dam %d dur %d",
            strbyte('a')+i, XX[1], a, c, f,
            d, t),
          row+i+1, col)
        end
      elseif band(U,BALL)~=0 then
        c_prt(C, format("%c) %-8s %5d %4d %3d%% dam %d rad %d",
          strbyte('a')+i, XX[1], a, c, f,
          d, r),
        row+i+1, col)
      elseif band(U,STRM)~=0 then
        c_prt(C, format("%c) %-8s %5d %4d %3d%% dam %d rad %d dur %d",
          strbyte('a')+i, XX[1], a, c, f,
          d, r, t),
        row+i+1, col)
      elseif band(U,CONE)~=0 then
        c_prt(C, format("%c) %-8s %5d %4d %3d%% dam %dd%d%s rad %d",
          strbyte('a')+i, XX[1], a, c, f,
          x, y, X and "" or " (x4)", r),
        row+i+1, col)
      elseif band(U,SURG)~=0 then
        if X then
          c_prt(C, format("%c) %-8s %5d %4d %3d%% dam %d (x3) rad %d",
            strbyte('a')+i, XX[1], a, c, f,
            d, r),
          row+i+1, col)
        else
          c_prt(C, format("%c) %-8s %5d %4d %3d%% dam %dd%d rad %d",
            strbyte('a')+i, XX[1], a, c, f,
            x, y, r),
          row+i+1, col)
        end
      elseif band(U,FLAR)~=0 then
        if X then
          c_prt(C, format("%c) %-8s %5d %4d %3d%% dam %dd%d (x%d) backlash 10%%",
            strbyte('a')+i, XX[1], a, c, f,
            -- x, y, t),
            x, y, 2),
          row+i+1, col)
        else
          if p.cmp == nil then xxx = p.csp else xxx = p.cmp end
          if xxx > c then
            c = xxx
            d = xxx
          end
          c_prt(C, format("%c) %-8s %5d %4d %3d%% dam %d dur %d backlash 20%%",
            strbyte('a')+i, XX[1], a, c, f,
            d, t),
          row+i+1, col)
        end
      end
    end
  elseif band(u,R2)~=0 then
    if w~=0 then
      c_prt(C,"Please choose the component mode of the spell.",row,col)
      row = row + 1
      c_prt(C,"ie. Select the spell modifier:",row,col)
      row = row + 2
      col = col - 6
    end
    c_prt(C,"   Mode       Level Cost Fail Damage Radius Duration Energy",row,col)
    local MM,a
    for i = 0,rcraft_max(u) do
      U = bshl(1,i+16)
      MM = M[U]
      U = bor(bor(u,U),BOLT)
      l = rspell_level(U)
      s = rspell_skill(0,U)
      a = rspell_ability(s,l)
      c_prt(a < 1 and TERM_L_DARK or TERM_L_GREEN,
      format("%c) %-10s %+5d %3d%% %+3d%% %5d%% %+6d %7d%% %5d%%",
        strbyte('a')+i,
        MM[1],
        MM[2],
        MM[3] * 10,
        MM[4],
        MM[5] * 10,
        MM[6],
        MM[7] * 10,
        MM[8] * 10),
      row+i+1, col)
    end
  else
    if w~=0 then
      c_prt(C,"Please choose the component runes of the spell.",row,col)
      row = row + 1
      c_prt(C,"ie. Select the spell runes to combine:",row,col)
      row = row + 2
      c_prt(C,"(Select the same rune twice to cast a single rune spell!)",row,col-5)
      row = row + 2
    end
    c_prt(C, "   Rune", row, col)
    for i = 0,rcraft_max(u) do
      U = bshl(1,i)
      if band(u,R1)~=0 then
        U = bor(bor(bor(bshl(u,8),U),MINI),BOLT)
      else
        U = bor(bor(bor(bshl(U,8),U),MINI),BOLT)
      end
      l = rspell_level(U)
      s = rspell_skill(0,U)
      a = rspell_ability(s,l)
      C = a < 1 and TERM_L_DARK or TERM_L_GREEN
      U = bshl(1,i)
      c_prt(band(u,R1)~=0 and band(u,U)~=0 and TERM_L_UMBER or C,
        format("%c) %-11s",
        strbyte('a')+i,
        R[U][1]),
      row+i+1, col)
    end
  end
end

function rcraft_bit(u,i)
  local U = bshl(1,i)
  if band(u,MODE)~=0 then
    return bshl(U,24)
  elseif band(u,R2)~=0 then
    return bshl(U,16)
  elseif band(u,R1)~=0 then
    return bshl(U,8)
  else
    return bshl(U,0)
  end
end

function rcraft_dir(u)
  return band(u,bor(STRM,SURG))==0
end

function rcraft_ftk(u)
  if band(u,ENHA)~=0 and band(u,FLAR)~=0 then return 0 end
  return band(u,bor(bor(CLOU,STRM),SURG))==0
end

-- SERVER

function rcraft_arr_set(u)
  if band(u,ENHA)~=0 and band(u,bor(STRM,SURG))~=0 then return 0 end
  return 1
end

function rcraft_arr_test(I,u)
  if band(u,ENHA)~=0 and band(u,bor(STRM,SURG))~=0 then return 0 end
  -- Also silently fall-through to melee auto-ret in these cases...
  local p = players(I)
  if p.confused~=0 then return 2 end
  if p.antimagic~=0 and p.admin_dm==0 then return 2 end
  if p.anti_magic~=0 then return 2 end
  local l = rspell_level(u)
  local s = rspell_skill(I,u)
  local a = rspell_ability(s,l)
  if a < 1 then return 1 end
  local c = rspell_cost(u,s)
  if p.cmp == nil then xxx = p.csp else xxx = p.cmp end
  if xxx < c then return 3 end
  return 0
end

function rspell_name(u)
  local PP = P[rspell_sval(u)]
  local MM = M[band(u,MODE)]
  local X = (band(u,ENHA)==0)
  local XX = X and T[band(u,TYPE)] or E[band(u,TYPE)]
  -- return X and format("%s %s of %s",MM[1],XX[1],PP[1]) or format("%s of %s",XX[1],PP[1])
  return X and format("%s %s %s",MM[1],PP[1],XX[1]) or format("%s %s",PP[1],XX[1])
end

function cast_rune_spell(I,D,u)
  -- 4 bits, no invalid positioning
  if band(u,WARN)~=0 then return 0 end
  if hamming(u)~=4 then return 0 end
  if band(u,R1)==0 then return 0 end
  if band(u,R2)==0 then return 0 end
  if band(u,MODE)==0 then return 0 end
  if band(u,TYPE)==0 then return 0 end
  local p = players(I)
  local e = rspell_energy(I,u)
  if p.confused~=0 then
    msg_print(I,"You are too confused!")
    return 0
  end
  if p.antimagic~=0 and p.admin_dm==0 then
    msg_print(I,"\255wYour anti-magic field disrupts any magic attempts.")
    return 0
  end
  if p.anti_magic~=0 then
    msg_print(I,"\255wYour anti-magic shell disrupts any magic attempts.")
    return 0
  end
  local v = rspell_sval(u)
  local PP = P[v]
  local X = (band(u,ENHA)==0)
  local l = rspell_level(u)
  local s = rspell_skill(I,u)
  local a = rspell_ability(s,l)
  local S = rspell_name(u)
  if a < 1 then
    msg_print(I,format("\255sYour skill is not high enough! (%s; level: %d)",S,a))
    return 0
  end
  local c = rspell_cost(u,s)
  if p.cmp == nil then xxx = p.csp else xxx = p.cmp end
  if xxx < c then
    msg_print(I,format("\255oYou do not have enough mana. (%s; cost: %d)",S,c))
    return 0
  end
  if check_antimagic(I,100)~=0 then
    p.energy = p.energy - e
    return 1
  end
  local f = rspell_failure(p,u,a,c)
  local x,y,d = rspell_damage(u,s)
  local b = damroll(x,y)
  d = d > b and d or b
  if magik(f)~=0 then
    b = d / 5 + 1
  else
    b = 0
  end
  if band(u,FLAR)~=0 then
    if X then
      b = b + d / 10 + 1
    else
      if p.cmp == nil then xxx = p.csp else xxx = p.cmp end
      c = xxx
      d = xxx
      b = b + d / 10 + 1
    end
  end
  -- Prevent suicide due to backlash, including spell 'failure', for now.
  if b > p.chp then -- Assume no resistance/immunity for simplicity. -.-"
    msg_print(I,format("\255RThe strain is far too great! (%s; backlash: %d)",S,b))
    return 0
  end
  local SS = (((v==bor(LITE,NETH) or v==bor(DARK,CHAO)) and band(u,ENHA)~=0) or band(u,EXPA)~=0) and "an" or "a" -- electricity, acid, enhanced "vowels"
  msg_print(I,format("You %strace %s %s with \255B%d\255w mana.", b > 0 and "\255Rincompetently\255w " or "",SS,S,c)) -- display mana cost in LIGHT_BLUE
  -- msg_print(I,format("You %strace %s %s.", b > 0 and "\255Rincompetently\255w " or "",SS,S))
  p.attacker = format(" traces %s %s for", SS, S)
  p.energy = p.energy - e
  if p.cmp == nil then
    p.csp = p.csp - c
  else
    p.cmp = p.cmp - c
  end
  local r = rspell_radius(u,s)
  local t = rspell_duration(u,s)
  if band(u,BOLT)~=0 then
    if X then
      fire_bolt(I, PP[2], D, d, p.attacker)
    else
      fire_beam(I, PP[2], D, d, p.attacker)
    end
  elseif band(u,CLOU)~=0 then
    if X then
      fire_cloud(I, PP[2], D, d, r, t, 9, p.attacker)
    else
      -- fire_wall(I, PP[2], D, d, t, 10, p.attacker)
      fire_wall(I, PP[2], D, d, t, 8, p.attacker)
    end
  elseif band(u,BALL)~=0 then
    -- Easter Egg? Illuminate (or unlite) your position by firing a ball up!
    local tx = p.px
    local ty = p.py
    if D==5 and target_okay(I)~=0 then
      tx = p.target_col
      ty = p.target_row
    end
    if X then
      fire_ball(I, PP[2], D, d, r, p.attacker)
    else
      fire_burst(I, PP[2], D, d, r, p.attacker)
    end
    if tx==p.px and ty==p.py then
      if v==LITE then lite_room(I, p.wpos, p.py, p.px) end
      if v==DARK then unlite_room(I, p.wpos, p.py, p.px) end
    end
  elseif band(u,STRM)~=0 then
    if X then
      fire_wave(I, PP[2], 0, d, r, t, 10, EFF_STORM, p.attacker)
    else
      if (p.nimbus == 0) or (p.nimbus_t ~= PP[2]) then
        msg_print(I,format("\255WYou are wreathed in \255%s%s\255W!", PP[4], PP[1]));
      end
      set_nimbus(I, t, PP[2], d)
    end
  elseif band(u,CONE)~=0 then
    if X then
      fire_cone(I, PP[2], D, d, r, p.attacker)
    else
      fire_shot(I, PP[2], D, x, y, r, 4, p.attacker)
    end
  elseif band(u,SURG)~=0 then
    if X then
      fire_wave(I, PP[2], 0, d, 1, r, 1, EFF_WAVE, p.attacker)
    else
      warding_rune(I, PP[2], d, r)
    end
  elseif band(u,FLAR)~=0 then
    if X then
      -- fire_wave(I, PP[2], D, d, 0, t, 10, EFF_VORTEX, p.attacker)
      -- fire_wave(I, PP[2], D, d, 0, 2, 10, EFF_VORTEX, p.attacker)
      -- Simplification of the new two-hit version, essentially the same!
      fire_cloud(I, PP[2], D, d, 0, 2, 1, p.attacker)
    else
      fire_nova(I, PP[2], D, d, t, 10, p.attacker)
    end
  end
  if b > 0 then
    project(PROJECTOR_RUNE, 0, p.wpos, p.py, p.px, b, PP[2], bor(bor(bor(bor(bor(PROJECT_KILL,PROJECT_NORF),PROJECT_JUMP),PROJECT_RNAF),PROJECT_NODO),PROJECT_NODF), "")
  end
  p.redraw = bor(p.redraw,PR_MANA)
  return 1
end

function rcraft_name(v)
  return P[v][1]
end

function rcraft_type(v)
  return P[v][2]
end

function rcraft_code(v)
  return P[v][4]
end

function rcraft_rune(I,v)
  local p = I~=0 and players(I) or player
  local x = 51
  local s
  if band(v,LITE)~=0 then
    s = p.s_info[1 + R[LITE][2]].value / 1000
    x = (s < x) and s or x
  end
  if band(v,DARK)~=0 then
    s = p.s_info[1 + R[DARK][2]].value / 1000
    x = (s < x) and s or x
  end
  if band(v,NEXU)~=0 then
    s = p.s_info[1 + R[NEXU][2]].value / 1000
    x = (s < x) and s or x
  end
  if band(v,NETH)~=0 then
    s = p.s_info[1 + R[NETH][2]].value / 1000
    x = (s < x) and s or x
  end
  if band(v,CHAO)~=0 then
    s = p.s_info[1 + R[CHAO][2]].value / 1000
    x = (s < x) and s or x
  end
  if band(v,MANA)~=0 then
    s = p.s_info[1 + R[MANA][2]].value / 1000
    x = (s < x) and s or x
  end
  if x < 40 or x > 50 then
    return 0
  else
    return x * 2
  end
end
