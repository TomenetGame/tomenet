-- Tables with descriptive text about the classes, to be displayed
-- during character creation process. - C. Blue

function get_class_diz(r_title, l)
    local i = 0

    while __classes_diz[(i * 2) + 1] ~= nil do
        if __classes_diz[(i * 2) + 1] == r_title then
            if __classes_diz[(i * 2) + 2][l + 1] == nil then
                return ""
            else
                return __classes_diz[(i * 2) + 2][l + 1]
            end
        end
        i = i + 1
    end
    return ""
end

function get_class_spellnt(c)
    return __classes_macrowiz[c + 1][1]
end
function get_class_spellt(c)
    return __classes_macrowiz[c + 1][2]
end
function get_class_mimicnt(c)
    return __classes_macrowiz[5][1]
end
function get_class_mimict(c)
    return __classes_macrowiz[5][2]
end

__classes_diz = {

--  "12345678901234567890123456789012345678901234567890",

"Warrior",
    {"\255uWarriors\255U solve most problems by cutting them to",
    "pieces. They are good at fighting and bows/throws",
    "but bad at most other things, although they can",
    "train to use magic devices. They are able to dual-",
    "wield one-handers, which requires light armour.",
    "They can also choose the paths of an unbeliever,",
    "who disbelieves in magic so strongly, negating it,",
    "or the path of a martial artist, foregoing weapons",
    "for fighting bare-handed instead.",
    "In most cases warrior are seen using two-handers,",
    "or one-hander and shield, and heavy armour to go",
    "with either."},

"Istar",
    {"\255uIstari\255U, as mages are called in elvish, are the",
    "all-around best magicians having both devastating",
    "and very useful support spells. On the downside",
    "they are the worst in close combat, and usually",
    "stick with a mage staff instead of weapons.",
    "Once they learn the Disruption Shield spell and",
    "are able to use it reliably, they become very hard",
    "to kill, but until then it will be a dangerous",
    "journey full of peril for every novice istar.",
    "Istari primarily use intelligence to determine",
    "their amount of mana and spell failure rate.",
    "They are the best class at using magic devices.",},

"Priest",
    {"\255uPriests\255U use divine magic know as praying.",
    "They aren't bad at close quarters if they train it",
    "but are restricted to blunt (or blessed) weaponry.",
    "They can learn spells to heal and support their",
    "party members. They sense curses on itemseasily.",
    "Priests primarily use wisdom to determine their",
    "amount of mana and spell failure rate.",
    "Similar to istari, priests are somewhat weak in",
    "the beginning, since they cannot deal much damage",
    "and not take much either. Call Light and Curse are",
    "those spells often seen to be used by novices.",
    "Their saving throw is extraordinarily good.",},

"Rogue",
    {"\255uRogues\255U are living by cunning, yet capable of",
    "fighting dual-handed, making use of light weapons.",
    "As masters of traps and locks they detect those in",
    "a radius and can overcome almost any device. They",
    "are even able to cast certain spells and may train",
    "themselves to steal items from stores.",
    "At level 15 they learn how to cloak themselves,",
    "allowing them to pass by unseen by almost any",
    "creature.",
    "Their light weapons become very deadly when they",
    "execute surprising backstab attacks or aim at the",
    "vital points, dealing critical strikes.",},

"Mimic",
    {"\255uMimics\255U are close combattants who learned how",
    "to shapechange into the forms of opponents they",
    "have beaten, inheriting their abilities but also",
    "ther weaknesses. They are even able to use their",
    "magical spells or breath attacks if their mana",
    "pool allows for it. They are able to attain the",
    "highest hit points of any class, and can forge and",
    "use rings of polymorphing.",
    "Using martial arts, forgoing armed combat, is not",
    "unpopular with mimics, since many forms such as",
    "animals or dragons do not allow them to wear",
    "weapons while shapeshifted.",},

"Archer",
    {"\255uArchers\255U are to ranged combats what warriors are",
    "to melee brawls. No class deals more damage in",
    "ranged fights than an archer who is equipped in",
    "ultimate gear. Having said that, few classes are",
    "as dependant on their weapon to deal damage as the",
    "archer is. They are bad at close quarters, and may",
    "get hindered when trying to shoot, but can train",
    "calmness to overcome this. One of the techniques",
    "they learn is to craft ammunition from rubble",
    "(slings) and bones (bows/crossbows). Dexterity is",
    "their primary stat, and the lighter armour they",
    "wear, the better is their aim.",},

"Paladin",
    {"\255uPaladins\255U are holy warriors, who can utilize divine",
    "magic much like priests. They primarily need",
    "wisdom for that, and strength for fighting.",
    "Paladins excel at using shields, being on par with",
    "warriors and surpassing any other class. This does",
    "not prevent them from wielding mighty two-handers",
    "at times. They have an outstanding saving throw",
    "but are rather poor at magic devices, stealth and",
    "perception.",
    "There are rumours of perished paladins who have",
    "risen again as undead warriors, and also those who",
    "became corrupted by chaos, utilizing unholy magic.",},

"Ranger",
    {"\255uRangers\255U are very versatile, able to fight in",
    "close combat, cast spells, or use ranged weapons.",
    "Their intelligence is prime for spell-casting.",
    "They are able to train Archery up to 10, allowing",
    "them to acquire all ranged techniques. They are",
    "even able to dual-wield two one-handed weapons,",
    "same as warriors and rogues may do. Devoted to",
    "nature they avoid foul magic such as necromancy",
    "but are skilled at trapping, magic devices, saving",
    "throw, stealth, searching and perception. At level",
    "15 rangers pass through thick woods unhindered and",
    "at 25 they are already very able swimmers as well.",},

"Adventurer",
    {"\255uAdventurers\255U are the most versatile of all classes.",
    "However, while they can train almost any skill and",
    "do not possess any specific strengths or weakness",
    "they also do not excel at anything in particular.",
    "They are hybrids, combining features of other",
    "more distinct classes in one character, creating",
    "unusual mixtures of abilities and synergies.",
    "The size of their mana pool depends on wisdom and",
    "intelligence in equal parts.",
    "They are difficult to play for beginners, because",
    "it can be hard to decide and find out what to do",
    "when faced with an overwhelming amount of skills.",},

"Druid",
    {"\255uDruids\255U are able to call upon the mystic powers",
    "of nature. Their wisdom is their primary stat",
    "for spell-casting and they train Martial Arts to",
    "complement this in close quarters, where they",
    "shapeshift into various animals and hybrids to",
    "make use of their perks as befits the situation.",
    "Unlike mimics, druids learn their forms every 5",
    "levels, without having to kill those monsters.",
    "A special ability of druids is that they can wear",
    "their full equipment no matter what form they are",
    "using. The spell Toxic Moisture should be acquired",
    "as early as possible in any druidic career.",},

"Shaman",
    {"\255uShamans\255U are able to call on both, magical and",
    "divine forces as they see fit, allowing them to",
    "train the spells used by istari and those used by",
    "priests. Their mana pool depends on either wisdom",
    "or intelligence, depending on what is higher. They",
    "are able to shapeshift much like mimics, except",
    "that some forms are not available, and they in",
    "turn receive the benefit of wearing full equipment",
    "in spirit, ghost and elemental forms. They excel",
    "at magic while they are still good at fighting, if",
    "they have acquired a strong monster form to change",
    "into, and trained either a weapon or martial arts.",},

"Runemaster",
    {"\255uRunemasters\255U use arcane symbols known as runes to",
    "summon pure elemental magic. They use dexterity",
    "and intelligence to manipulate runes and protect",
    "themselves from the fickle and dangerous forces of",
    "their art. They are able to cast spells even with-",
    "out possessing the actual runes, just from memory,",
    "but side effects may occur. They are not the most",
    "systematic of adventurers, and sometimes mistaken",
    "for rogues, at least until they pulverize every-",
    "thing nearby in an elemental blast gone wrong.",
    "",
    "",},

"Mindcrafter",
    {"\255uMindcrafters\255U are calm warriors who have trained",
    "their mind to utilize psychic powers. They have a",
    "variety of spells of most different nature, be it",
    "a psionic blast that fries an opponent's brain or",
    "a telepathic overtake that lets him see what all",
    "the creatures nearby can see. Mindcrafters can",
    "also affect their own mind and body, to bolster",
    "own combat abilities or those of team mates.",
    "Their primary stats are strength and intelligence",
    "and some of their spells can actually be cast",
    "while they are blinded or even confused.",
    "",},
}

__classes_macrowiz = {
{ "Phase Door", "Manathrust", }, --warrior
{ "Phase Door", "Manathrust", }, --istar
{ "Blessing", "Curse", }, --priest
{ "Phase Door", "Blindness", }, --rogue
{ "Blink", "Magic Missile", }, --mimic
{ "Phase Door", "Manathrust", }, --archer
{ "Remove Fear", "Curse", }, --paladin
{ "Healing", "Frost Bolt", }, --ranger
{ "Phase Door", "Manathrust", }, --adventurer
{ "Focus", "Toxic Moisture", }, --druid
{ "Starlight", "Cause Wounds", }, --shaman
{ "Phase Door", "Manathrust", }, --runemaster
{ "Scare", "Psionic Blast", }, --mindcrafter
{ "Cause Fear", "Fatigue", }, --death knight
{ "Ignore Fear", "Terror", }, --hell knight (not applied until initiation anyway)
{ "Ignore Fear", "Cause Fear", }, --c.priest (not applied until initiation anyway)
}
