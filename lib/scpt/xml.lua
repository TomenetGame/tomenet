-- The xml module
xml = {}

xml.ent = {lt = "<", gt = ">", amp = "&", quot = "\""}

-- parse entities - mikaelh
function xml:parseentities (s)
  local t
  t = gsub(s, "&(%w+);", function (s)
    if (rawget(xml.ent, s) == nil) then return("&" .. s .. ";")
    else return xml.ent[s]
    end
  end)
  return t
end

function xml:parseargs (s)
  local arg = {}
  gsub(s, "(%w+)=([\"'])(.-)%2", function (w, _, a)
    %arg[w] = xml:parseentities(a)
  end)
  return arg
end

-- s is a xml stream, returns a table
function xml:collect (s)
  local stack = {n=0}
  local top = {n=0}
  tinsert(stack, top)
  local ni,c,label,args, empty
  local i, j = 1, 1
  while 1 do
    ni,j,c,label,args, empty = strfind(s, "<(%/?)(%w+)(.-)(%/?)>", j)
    if not ni then break end
    local text = strsub(s, i, ni-1)
    if not strfind(text, "^%s*$") then
      tinsert(top, xml:parseentities(text))
    end
    if empty == "/" then  -- empty element tag
      tinsert(top, {n=0, label=label, args=xml:parseargs(args), empty=1})
    elseif c == "" then   -- start tag
      top = {n=0, label=label, args=xml:parseargs(args)}
      tinsert(stack, top)   -- new level
    else  -- end tag
      local toclose = tremove(stack)  -- remove top
      top = stack[stack.n]
      if stack.n < 1 then
        error("nothing to close with "..label)
      end
      if toclose.label ~= label then
        error("trying to close "..toclose.label.." with "..label)
      end
      tinsert(top, toclose)
    end
    i = j+1
  end
  local text = strsub(s, i)
  if not strfind(text, "^%s*$") then
    tinsert(stack[stack.n], xml:parseentities(text))
  end
  if stack.n > 1 then
    error("unclosed "..stack[stack.n].label)
  end
  return stack[1]
end


xml.__str__ = ""
xml.write = function(str)
        xml.__str__ = xml.__str__ .. str
end

function xml:print_xml(t, tab)
        local i, k, e
        local inside = nil
        local bcol, ecol = TERM_L_GREEN, TERM_GREEN

        xml.write(tab.."<"..t.label)
        for k, e in t.args do
                xml.write(" "..k)
                xml.write("=\"")
                xml.write(e)
                xml.write("\"")
        end
        xml.write(">")

        for i = 1, getn(t) do
                if type(t[i]) == "string" then
                        xml.write(t[i])
                else
                        if not inside then xml.write("\n") end
                        inside = not nil
                        xml:print_xml(t[i], tab.."    ")
                end
        end

        if not inside then
                xml.write("</"..t.label..">\n")
        else
                xml.write(tab.."</"..t.label..">\n")
        end
end

-- t is a table representing xml, outputs the xml code via xml.write()
function xml:serialize(t)
        local e
        xml.__str__ = ''
        for _, e in t do
                if type(e) == "table" then
                        xml:print_xml(e, "")
                end
        end
        return xml.__str__
end
