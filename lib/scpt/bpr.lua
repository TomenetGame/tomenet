-- Tables with startup-BpR numbers, depending on STR/DEX/Class.
-- To be displayed during character creation process to alleviate
-- the need of looking up stats in the guide to hit breakpoints. - C. Blue

--(A) just return fixed values from the table?
--Using string for class just in case there's a server-client incompatibility after another class gets added.
function get_class_bpr(cn, n)
	local c
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
	return "S" .. __class_bpr[c + 1][n + 1][1] .. "D" .. __class_bpr[c + 1][n + 1][2] .. "B" .. __class_bpr[c + 1][n + 1][3]
end
function get_class_bpr_tablesize()
	return 4
end

--(B) alternatively, return the actual dps formula's result
function get_class_bpr2(cn, wgt, str, dex)
	return "X" --means "disabled"

--[[
	--some classes don't have significant weapon-BpR usage
	if cn == "Istar" then return "N" end
	if cn == "Archer" then return "N" end
	if cn == "Druid" then return "N" end

	--todo: add formula here..
	--return "F" .. --return a bpr value
]]
end

-- for each class: 3x {str,dex,bpr} (stat 0 = any)
__class_bpr = {
{
--warrior
    { 18, 10, 2, },
    { 19,  0, 2, },
    { 21,  0, 3, },
    { 22, 19, 4, },
}, {
--istar
    { 0,  0, 0, },
    { 0,  0, 0, },
    { 0,  0, 0, },
    { 0,  0, 0, },
}, {
--priest
    { 19, 10, 2, },
    { 23, 10, 3, },
    {  0,  0, 0, },
    { 0,  0, 0, },
}, {
--rogue
    { 15, 19, 2, },
    { 19, 23, 3, },
    { 20, 19, 3, },
    { 0,  0, 0, },
}, {
--mimic
    { 19, 10, 2, },
    { 23, 10, 3, },
    { 21, 19, 3, },--+
    { 0,  0, 0, },
}, {
--archer
    { 0,  0, 0, },
    { 0,  0, 0, },
    { 0,  0, 0, },
    { 0,  0, 0, },
}, {
--paladin
    { 19, 10, 2, },
    { 21, 10, 3, },
    { 20, 19, 3, },
    { 0,  0, 0, },
}, {
--ranger
    { 19, 10, 2, },
    { 23, 10, 3, },
    { 21, 19, 3, },
    { 0,  0, 0, },
}, {
--adventurer
    { 19, 10, 2, },
    { 21, 10, 3, },
    { 20, 19, 3, },
    { 0,  0, 0, },
}, {
--druid
    { 0,  0, 0, },
    { 0,  0, 0, },
    { 0,  0, 0, },
    { 0,  0, 0, },
}, {
--shaman
    { 19, 10, 2, },
    { 23, 10, 3, },
    { 21,  0, 3, },
    { 0,  0, 0, },
}, {
--runemaster
    { 15, 19, 2, },
    { 19, 23, 3, },
    { 20, 19, 3, },
    { 0,  0, 0, },
}, {
--mindcrafter
    { 19, 10, 2, },
    { 23, 10, 3, },
    { 21, 19, 3, },
    { 0,  0, 0, },
}, {
--death knight
    { 19, 10, 2, },
    { 21, 10, 3, },
    { 20, 19, 3, },
    { 0,  0, 0, },
}, {
--hell knight
    { 19, 10, 2, },
    { 21, 10, 3, },
    { 20, 19, 3, },
    { 0,  0, 0, },
}, {
--corrupted priest
    { 19, 10, 2, },
    { 23, 10, 3, },
    {  0,  0, 0, },
    { 0,  0, 0, },
},
}
