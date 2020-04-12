-- Tables with startup-BpR numbers, depending on STR/DEX/Class.
-- To be displayed during character creation process to alleviate
-- the need of looking up stats in the guide to hit breakpoints. - C. Blue

function get_class_bpr(c, n)
        return "S" .. __class_bpr[c + 1][n + 1][1] .. "D" .. __class_bpr[c + 1][n + 1][2] .. "B" .. __class_bpr[c + 1][n + 1][3]
end

-- for each class: 3x {str,dex,bpr} (stat 0 = any)
__class_bpr = {
{
--warrior
    { 18, 10, 2, },
    { 19,  0, 2, },
    { 21,  0, 3, },
}, {
--mage
    { 0,  0, 0, },
    { 0,  0, 0, },
    { 0,  0, 0, },
}, {
--priest
    { 19, 10, 2, },
    { 23, 10, 3, },
    {  0,  0, 0, },
}, {
--rogue
    { 15, 19, 2, },
    { 19, 23, 3, },
    { 20, 19, 3, },
}, {
--mimic
    { 19, 10, 2, },
    { 23, 10, 3, },
    { 21, 19, 3, },--+
}, {
--archer
    { 0,  0, 0, },
    { 0,  0, 0, },
    { 0,  0, 0, },
}, {
--paladin
    { 19, 10, 2, },
    { 21, 10, 3, },
    { 20, 19, 3, },
}, {
--ranger
    { 19, 10, 2, },
    { 23, 10, 3, },
    { 21, 19, 3, },
}, {
--adventurer
    { 19, 10, 2, },
    { 21, 10, 3, },
    { 20, 19, 3, },
}, {
--druid
    { 0,  0, 0, },
    { 0,  0, 0, },
    { 0,  0, 0, },
}, {
--shaman
    { 19, 10, 2, },
    { 23, 10, 3, },
    { 21,  0, 3, },
}, {
--runemaster
    { 15, 19, 2, },
    { 19, 23, 3, },
    { 20, 19, 3, },
}, {
--mindcrafter
    { 19, 10, 2, },
    { 23, 10, 3, },
    { 21, 19, 3, },
}, {
--death knight
    { 19, 10, 2, },
    { 21, 10, 3, },
    { 20, 19, 3, },
}, {
--hell knight
    { 19, 10, 2, },
    { 21, 10, 3, },
    { 20, 19, 3, },
}, {
--corrupted priest
    { 19, 10, 2, },
    { 23, 10, 3, },
    {  0,  0, 0, },
},
}
