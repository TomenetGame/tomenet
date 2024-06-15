-- Tables with descriptive text about the attributes, to be displayed
-- during character creation process. - C. Blue

function get_attribute_diz(r_title, l)
    local i = 0

    while __attributes_diz[i * 2 + 1] ~= nil do
        if __attributes_diz[i * 2 + 1] == r_title then
            if __attributes_diz[i * 2 + 2][l + 1] == nil then
                return ""
            else
                return __attributes_diz[i * 2 + 2][l + 1]
            end
        end
        i = i + 1
    end
    return ""
end

__attributes_diz = {

--  "12345678901234567890123456789012345678901234567890",

"Strength", {
    "   How quickly you can strike with weapons.     ",
    "   How much you can carry and wear/wield.       ",
    "   How much damage your strikes inflict.        ",
    "   How easily you can bash, throw and dig.      ",
    "   Slightly improves your swimming.             ",
    "                                                ",
    "                                                ",
    "                                                ",
    },
"Intelligence", {
    "   How well you can use magic                   ",
    "    (depending on your class and spells).       ",
    "   How well you can use magic devices.          ",
    "   Helps your disarming ability.                ",
    "   Helps noticing attempts to steal from you.   ",
    "                                                ",
    "                                                ",
    "                                                ",
    },
"Wisdom", {
    "   How well you can use prayers and magic       ",
    "    (depending on your class and spells).       ",
    "   How well can you resist malicious effects    ",
    "    and influences on both body and mind        ",
    "    (saving throw).                             ",
    "   How much insanity your mind can take.        ",
    "                                                ",
    "                                                ",
    },
"Dexterity", {
    "   How quickly you can strike with weapons.     ",
    "   Reduces your chance to miss.                 ",
    "   Opponents will miss very slightly more often.",
    "   Helps your stealing skills (if any).         ",
    "   Helps to prevent foes stealing from you.     ",
    "   Helps keeping your balance after bashing.    ",
    "   Helps your disarming ability.                ",
    "   Slightly improves your swimming.             ",
    },
"Constitution", {
    "   Determines your amount of HP                 ",
    "    (hit points, ie how much damage you can     ",
    "    take without dying. High constitution might ",
    "    not show much effect until your character   ",
    "    also reaches an appropriate level.)         ",
    "   Reduces duration of poison and stun effects. ",
    "   Increases stamina regeneration rate.         ",
    "   Helps your character not to drown too easily.",
    },
"Charisma", {
    "   Shops will buy and sell at better prices.    ",
    "    (Note that shop keepers are also influenced ",
    "    by your character's race.)                  ",
    "   Also affects house prices.                   ",
    "   Helps you to resist seducing attacks.        ",
    "   Affects mindcrafters' mana pool somewhat.    ",
    "                                                ",
    "                                                ",
    },
}
