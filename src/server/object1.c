/* $Id$ */
/* File: obj-desc.c */

/* Purpose: handle object descriptions, mostly string handling code */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define SERVER

#include "angband.h"

/*
 * Name flavors by combination of adj and extra modifier, so that
 * we don't have to devise new flavor every time adding a new item.
 * eg. 'whirling' + 'red' = 'whirling red potion'
 *
 * Not implemented yet.		- Jir -
 */
//#define EXTRA_FLAVORS

/*
 * XXX XXX Hack -- note that "TERM_MULTI" is now just "TERM_VIOLET"
 * We will have to find a cleaner method for "MULTI_HUED" later.
 * There were only two multi-hued "flavors" (one potion, one food).
 * Plus five multi-hued "base-objects" (3 dragon scales, one blade
 * of chaos, and one something else).  See the SHIMMER_OBJECTS code
 * in "dungeon.c" and the object color extractor in "cave.c".
 */
/*#define TERM_MULTI	TERM_VIOLET */

/* Special: Further shorten the very long item names of royal armour [optionally only if we already omit its article for length constraints (mode & 8) ]: */
#define ROYAL_ARMOUR_SHORTEN_NAME
/* Don't shorten the item name of the artifact 'Antiriad': */
#define ROYAL_ARMOUR_SHORTEN_ANTIRIAD_ONLY
/* Don't shorten the item name of the artifact 'Antiriad' in its depleted version: */
#define ROYAL_ARMOUR_SHORTEN_ANTIRIAD_DEPLETED




#if EXTRA_FLAVORS
 #define MAX_MODIFIERS	6       /* Used with rings (see below) */
static cptr ring_adj2[MAX_ROCKS2] = {
	"", "Brilliant", "Dark", "Enchanting", "Murky", "Bright"
};
#endif	// EXTRA_FLAVORS

/*
 * Rings (adjectives and colors)
 */

static cptr ring_adj[MAX_ROCKS] = {
	//f'lar: ?
	//barahir: serpents with emerald eyes
	//tulkas: ?
	//narya: ring of fire (use R instead of fmaybe, since it doesn't give anything fire-related really)
	//nenya: adamant with pure white centerpiece
	//vilya: sapphire (using b, not e, so we don't have to add anything.)
	//the one ring: gold
	//phasing: ?
	//durin: gold with sapphire, if it's same Thror got from Durin III. (again, using b)
	//-> fortunately we have enough TERM_BLUE there..
	"Alexandrite", "Amethyst", "Aquamarine", "Azurite", "Beryl",
	"Bloodstone", "Calcite", "Carnelian", "Corundum", "Diamond",
	"Emerald", "Fluorite", "Garnet", "Granite", "Jade",
	"Jasper", "Lapis Lazuli", "Malachite", "Marble", "Moonstone",

	"Onyx", "Opal", "Pearl", "Quartz", "Quartzite",
	"Rhodonite", "Ruby", "Sapphire", "Tiger Eye", "Topaz",
	"Turquoise", "Zircon", "Platinum", "Bronze", "Gold",
	"Obsidian", "Silver", "Tortoise Shell", "Mithril", "Jet",

	"Engagement", "Adamantite", "Wire", "Dilithium", "Bone",
	"Wooden", "Spikard", "Serpent", "Wedding", "Double",
	"Plain Gold", "Brass", "Scarab", "Shining", "Rusty",
	"Transparent", "Copper", "Black Opal", "Nickel", "Glass",

	"Fluorspar", "Agate",
};
/* Specialty for flavor_hacks(): Don't use unfitting materials for artifacts */
static bool ring_cheap[MAX_ROCKS] = {
	0, 0, 0, 0, 0,   0, 1, 0, 0, 0,   0, 1, 0, 1, 0,   0, 0, 1, 0, 0,
	0, 0, 0, 0, 0,   0, 0, 0, 1, 0,   0, 0, 0, 0, 0,   0, 0, 1, 0, 1,
	1, 0, 1, 0, 0,   1, 1, 0, 0, 1,   0, 0, 0, 0, 1,   0, 0, 0, 0, 0,
	0, 1,
};
//todo: make money pile colours (gems) consistent with these
#if 1 /*more animated TERMs*/
static byte ring_col[MAX_ROCKS] = {
	TERM_GREEN, TERM_VIOLET, TERM_L_BLUE, TERM_L_BLUE, TERM_L_GREEN,
	TERM_RED, TERM_WHITE, TERM_RED, TERM_SLATE, TERM_COLD,
	TERM_GREEN, TERM_L_GREEN, TERM_RED, TERM_L_DARK, TERM_L_GREEN,
	TERM_UMBER, TERM_BLUE, TERM_GREEN, TERM_WHITE, TERM_L_WHITE,

	TERM_L_RED, TERM_L_WHITE, TERM_WHITE, TERM_L_WHITE, TERM_L_WHITE,
	TERM_L_RED, TERM_RED, TERM_BLUE, TERM_YELLOW, TERM_YELLOW,
	TERM_L_BLUE, TERM_L_UMBER, TERM_WHITE, TERM_L_UMBER, TERM_YELLOW,
	TERM_L_DARK, TERM_L_WHITE, TERM_GREEN, TERM_L_BLUE, TERM_L_DARK,

	TERM_YELLOW, TERM_VIOLET, TERM_UMBER, TERM_L_WHITE, TERM_WHITE,
	TERM_UMBER, TERM_BLUE, TERM_GREEN, TERM_YELLOW, TERM_ORANGE,
	TERM_YELLOW, TERM_ORANGE, TERM_L_GREEN, TERM_LITE, TERM_UMBER,
	TERM_L_WHITE, TERM_UMBER, TERM_L_DARK, TERM_L_WHITE, TERM_L_WHITE,

	TERM_BLUE, TERM_L_WHITE
};
#else /*classic*/
static byte ring_col[MAX_ROCKS] = {
	TERM_GREEN, TERM_VIOLET, TERM_L_BLUE, TERM_L_BLUE, TERM_L_GREEN,
	TERM_RED, TERM_WHITE, TERM_RED, TERM_SLATE, TERM_WHITE,
	TERM_GREEN, TERM_L_GREEN, TERM_RED, TERM_L_DARK, TERM_L_GREEN,
	TERM_UMBER, TERM_BLUE, TERM_GREEN, TERM_WHITE, TERM_L_WHITE,

	TERM_L_RED, TERM_L_WHITE, TERM_WHITE, TERM_L_WHITE, TERM_L_WHITE,
	TERM_L_RED, TERM_RED, TERM_BLUE, TERM_YELLOW, TERM_YELLOW,
	TERM_L_BLUE, TERM_L_UMBER, TERM_WHITE, TERM_L_UMBER, TERM_YELLOW,
	TERM_L_DARK, TERM_L_WHITE, TERM_GREEN, TERM_L_BLUE, TERM_L_DARK,

	TERM_YELLOW, TERM_VIOLET, TERM_UMBER, TERM_L_WHITE, TERM_WHITE,
	TERM_UMBER, TERM_BLUE, TERM_GREEN, TERM_YELLOW, TERM_ORANGE,
	TERM_YELLOW, TERM_ORANGE, TERM_L_GREEN, TERM_YELLOW, TERM_RED,
	TERM_WHITE, TERM_UMBER, TERM_L_DARK, TERM_L_WHITE, TERM_WHITE,

	TERM_BLUE, TERM_L_WHITE
};
#endif

/*
 * Amulets (adjectives and colors)
 */

static cptr amulet_adj[MAX_AMULETS] = {
	//carlammas: fiery circle of bronze (Z)
	//toris: blue stone of orichalcum (e)
	//ingwe: ?
	//nauglamir: carencet of gold w/ multitude of shining gems (m)
	//elessar: green gem
	//evenstar: pure white jewel
	//grom: well, make it half-hued? or just dark! (h/D/s)
	//spirit shard: white+silver (use q or c, glass or silver)
	//-> replace by Z,e,m,(h),c/q

	"Amber", "Driftwood", "Coral", "Agate", "Ivory",
	"Obsidian", "Bone", "Brass", "Bronze", "Pewter",
	"Tortoise Shell", "Golden", "Azure", "Crystal", "Silver",
	"Copper", "Amethyst", "Mithril", "Sapphire", "Dragon Tooth",

	"Carved Oak", "Sea Shell", "Flint Stone", "Ruby", "Scarab",
	"Origami Paper", "Meteoric Iron", "Platinum", "Glass", "Beryl",
	"Malachite", "Adamantite", "Mother-of-pearl", "Runed", "Sandalwood",
	"Emerald", "Aquamarine", "Orichalcum", "Shining", "Ebony",

	"Meerschaum", "Jade", "Red Opal", "Glimmer-Stone",
};
/* Specialty for flavor_hacks(): Don't use unfitting materials for artifacts */
static bool amulet_cheap[MAX_AMULETS] = {
	0, 1, 1, 0, 0,   0, 1, 0, 0, 1,   1, 0, 0, 0, 0,   0, 0, 0, 0, 1,
	1, 1, 1, 0, 1,   1, 0, 0, 0, 0,   1, 0, 0, 0, 1,   0, 0, 0, 0, 1,
	0, 0, 0, 1,
};
//todo: make money pile colours (gems) consistent with these
#if 1 /*more animated TERMs*/
static byte amulet_col[MAX_AMULETS] = {
	TERM_YELLOW, TERM_L_UMBER, TERM_WHITE, TERM_L_WHITE, TERM_WHITE,
	TERM_L_DARK, TERM_WHITE, TERM_ORANGE, TERM_L_UMBER, TERM_SLATE,
	TERM_GREEN, TERM_YELLOW, TERM_L_BLUE, TERM_L_BLUE, TERM_COLD,
	TERM_L_UMBER, TERM_VIOLET, TERM_L_BLUE, TERM_BLUE, TERM_L_WHITE,

	TERM_UMBER, TERM_L_BLUE, TERM_SLATE, TERM_RED, TERM_L_GREEN,
	TERM_WHITE, TERM_L_DARK, TERM_L_WHITE, TERM_INER, TERM_L_GREEN,
	TERM_GREEN, TERM_VIOLET, TERM_L_WHITE, TERM_UMBER, TERM_L_WHITE,
	TERM_GREEN, TERM_L_BLUE, TERM_ELEC, TERM_MULTI, TERM_L_DARK,

	TERM_L_WHITE, TERM_L_GREEN, TERM_EMBER, TERM_LITE
};
#else
static byte amulet_col[MAX_AMULETS] = {
	TERM_YELLOW, TERM_L_UMBER, TERM_WHITE, TERM_L_WHITE, TERM_WHITE,
	TERM_L_DARK, TERM_WHITE, TERM_ORANGE, TERM_L_UMBER, TERM_SLATE,
	TERM_GREEN, TERM_YELLOW, TERM_L_BLUE, TERM_L_BLUE, TERM_L_WHITE,
	TERM_L_UMBER, TERM_VIOLET, TERM_L_BLUE, TERM_BLUE, TERM_L_WHITE,

	TERM_UMBER, TERM_L_BLUE, TERM_SLATE, TERM_RED, TERM_L_GREEN,
	TERM_WHITE, TERM_L_DARK, TERM_L_WHITE, TERM_WHITE, TERM_L_GREEN,
	TERM_GREEN, TERM_VIOLET, TERM_L_WHITE, TERM_UMBER, TERM_L_WHITE,
	TERM_GREEN, TERM_L_BLUE, TERM_ELEC, TERM_LITE, TERM_L_DARK,

	TERM_L_WHITE, TERM_L_GREEN, TERM_RED, TERM_YELLOW
};
#endif

/*
 * Staffs (adjectives and colors)
 */

static cptr staff_adj[MAX_WOODS] = {
	"Aspen", "Balsa", "Banyan", "Birch", "Cedar",
	"Cottonwood", "Cypress", "Dogwood", "Elm", "Eucalyptus",
	"Hemlock", "Hickory", "Ironwood", "Locust", "Mahogany",
	"Maple", "Mulberry", "Oak", "Pine", "Redwood",
	"Rosewood", "Spruce", "Sycamore", "Teak", "Walnut",
	"Mistletoe", "Hawthorn", "Bamboo", "Silver", "Runed",
	"Golden", "Ashen", "Gnarled", "Ivory", "Willow",
	"Cryptomeria"
};
static byte staff_col[MAX_WOODS] = {
	TERM_L_UMBER, TERM_L_UMBER, TERM_L_UMBER, TERM_L_UMBER, TERM_L_UMBER,
	TERM_L_UMBER, TERM_L_UMBER, TERM_L_UMBER, TERM_L_UMBER, TERM_L_UMBER,
	TERM_L_UMBER, TERM_L_UMBER, TERM_UMBER, TERM_L_UMBER, TERM_UMBER,
	TERM_L_UMBER, TERM_L_UMBER, TERM_L_UMBER, TERM_L_UMBER, TERM_RED,
	TERM_RED, TERM_L_UMBER, TERM_L_UMBER, TERM_L_UMBER, TERM_UMBER,
	TERM_GREEN, TERM_L_UMBER, TERM_L_UMBER, TERM_L_WHITE, TERM_UMBER,
	TERM_YELLOW, TERM_SLATE, TERM_UMBER, TERM_L_WHITE, TERM_L_UMBER,
	TERM_L_UMBER
};


/*
 * Wands (adjectives and colors)
 */

cptr wand_adj[MAX_METALS] = {
	"Aluminium", "Cast Iron", "Chromium", "Copper", "Gold",
	"Iron", "Magnesium", "Molybdenum", "Nickel", "Rusty",
	"Silver", "Steel", "Tin", "Titanium", "Tungsten",
	"Zirconium", "Zinc", "Aluminium-Plated", "Copper-Plated", "Gold-Plated",
	"Nickel-Plated", "Silver-Plated", "Steel-Plated", "Tin-Plated", "Zinc-Plated",
	"Mithril-Plated", "Mithril", "Runed", "Bronze", "Brass",
	"Platinum", "Lead", "Lead-Plated", "Ivory" , "Adamantite",
	"Uridium", "Long", "Short", "Hexagonal"
};
static byte wand_col[MAX_METALS] = {
	TERM_L_BLUE, TERM_L_DARK, TERM_WHITE, TERM_UMBER, TERM_YELLOW,
	TERM_SLATE, TERM_L_WHITE, TERM_L_WHITE, TERM_L_WHITE, TERM_RED,
	TERM_L_WHITE, TERM_L_WHITE, TERM_L_WHITE, TERM_WHITE, TERM_WHITE,
	TERM_L_WHITE, TERM_L_WHITE, TERM_L_BLUE, TERM_L_UMBER, TERM_YELLOW,
	TERM_L_UMBER, TERM_L_WHITE, TERM_L_WHITE, TERM_L_WHITE, TERM_L_WHITE,
	TERM_L_BLUE, TERM_L_BLUE, TERM_UMBER, TERM_L_UMBER, TERM_L_UMBER,
	TERM_WHITE, TERM_SLATE, TERM_SLATE, TERM_WHITE, TERM_VIOLET,
	TERM_L_RED, TERM_L_BLUE, TERM_BLUE, TERM_RED
};


/*
 * Rods (adjectives and colors).
 * Efficiency -- copied from wand arrays
 */

cptr rod_adj[MAX_METALS];

static byte rod_col[MAX_METALS];


/*
 * Mushrooms (adjectives and colors)
 */

static cptr food_adj[MAX_SHROOM] = {
	"Shimmering", "Blue", "Black", "Black Spotted", "Brown",
	"Dark Blue", "Dark Green", "Dark Red", "Yellow", "Furry",
	"Green", "Grey", "Light Blue", "Light Green", "Violet",
	"Red", "Slimy", "Tan", "White", "White Spotted",
	"Wrinkled", "Orange"
};
/* note: 'reserve' TERM_L_RED for shrieker shroom (monster) ;).
         TERM_L_UMBER is also for non-shroom food. */
static byte food_col[MAX_SHROOM] = {
	TERM_MULTI, TERM_BLUE, TERM_L_DARK, TERM_L_DARK, TERM_UMBER,
	TERM_BLUE, TERM_GREEN, TERM_RED, TERM_YELLOW, TERM_L_WHITE,
	TERM_GREEN, TERM_SLATE, TERM_L_BLUE, TERM_L_GREEN, TERM_VIOLET,
	TERM_RED, TERM_SLATE, TERM_L_UMBER, TERM_WHITE, TERM_WHITE,
	TERM_UMBER, TERM_ORANGE
};


/*
 * Color adjectives and colors, for potions.
 * Hack -- The first four entries are hard-coded.
 * (water, apple juice, slime mold juice, something)
 */

#ifdef EXTRA_FLAVORS//not implemented!

/* 15 */
static cptr potion_mod[MAX_MOD_COLORS] = {
	"Speckled", "Bubbling", "Cloudy", "Metallic", "Misty", "Smoky",
	"Pungent", "Clotted", "Viscous", "Oily", "Shimmering", "Coagulated",
	"Manly", "Stinking", "Whirling",
};

/* 34 */
static cptr potion_base[MAX_BASE_COLORS] = {
	"Clear", "Light Brown", "Icky Green", "Scarlet",
	"Azure", "Blue", "Black", "Brown",
	"Chartreuse", "Crimson", "Cyan", "Dark Blue",
	"Dark Green", "Dark Red", "Green", "Grey",
	"Hazy", "Indigo", "Light Blue", "Light Green",
	"Magenta", "Orange", "Pink", "Puce",
	"Purple", "Red", "Tangerine", "Violet",
	"Vermilion", "White", "Yellow", "Gloopy Green",
	"Gold", "Ichor", "Ivory White", "Sky Blue",
	"Beige",// "Phosphorescent",
};

 #ifdef HOUSE_PAINTING
    byte potion_col[MAX_COLORS] =
 #else
    static byte potion_col[MAX_COLORS] =
 #endif
    {
	TERM_WHITE, TERM_L_UMBER, TERM_GREEN, TERM_RED,
	TERM_L_BLUE, TERM_BLUE, TERM_L_DARK, TERM_UMBER,
	TERM_L_GREEN, TERM_RED, TERM_L_BLUE, TERM_BLUE,
	TERM_GREEN, TERM_RED, TERM_GREEN, TERM_SLATE,
	TERM_L_WHITE, TERM_VIOLET, TERM_L_BLUE, TERM_L_GREEN,
	TERM_RED, TERM_ORANGE, TERM_L_RED, TERM_VIOLET,
	TERM_VIOLET, TERM_RED, TERM_ORANGE, TERM_VIOLET,
	TERM_RED, TERM_WHITE, TERM_YELLOW, TERM_GREEN,
	TERM_YELLOW, TERM_RED, TERM_WHITE, TERM_L_BLUE,
	TERM_L_UMBER, //TERM_MULTI,
};

static char potion_adj[MAX_COLORS][24];

#else /* no EXTRA_FLAVOURS -- (extra flavours aren't implemented yet) */

static cptr potion_adj[MAX_COLORS] = {
	"Clear", "Light Brown", "Icky Green", "Scarlet", /* instead of "Blood Red", */
	"Orange", "Cloudy", "Azure", "Blue",
	"Blue Speckled", "Black", "Brown", "Brown Speckled",
	"Bubbling", "Chartreuse", "Copper Speckled", "Crimson",
	"Cyan", "Dark Blue", "Dark Green", "Dark Red",
	"Gold Speckled", "Green", "Green Speckled", "Grey",
	"Grey Speckled", "Hazy", "Indigo", "Light Blue",
	"Light Green", "Magenta", "Metallic Blue", "Metallic Red",
	"Metallic Green", "Metallic Purple", "Misty", "Orange Speckled",
	"Pink", "Pink Speckled", "Puce", "Purple",
	"Purple Speckled", "Red", "Red Speckled", "Silver Speckled",
	"Smoky", "Tangerine", "Violet", "Vermilion",
	"White", "Yellow", "Violet Speckled", "Pungent",
	"Clotted Red", "Viscous Pink", "Oily Yellow", "Gloopy Green",
	"Shimmering", "Coagulated Crimson", "Yellow Speckled", "Gold",
	"Manly", "Stinking", "Oily Black", "Ichor",
	"Whirling", /* 65 (basic TV_POTION) */
#ifdef EXPAND_TV_POTION
	"Ivory White",
#endif

	/* add more colours to make TV_POTION2 obsolete */
	//"Turquoise", "Beige",
	//"Amber",
	//"Radiant", "Lilac", <- note, we already have TOO MANY TERM_VIOLET colours..
	//"Glowing Green", "Glowing Blue", "Glowing Red", "Glittering", "Phosphorescent",
	//notes: light red is covered by vermillon, ochre is covered by light brown, sky blue by azure, sepia by beige maybe
	//viridian==turquoise, citron covered by chartreuse,
	//viridian, sienna, sepia, amber, umber, ivory
};

 # if 1 /* more sparkly term colours for more visual diversity */

  #ifdef HOUSE_PAINTING
    byte potion_col[MAX_COLORS] = {
  #else
    static byte potion_col[MAX_COLORS] = {
  #endif
	TERM_ELEC, TERM_L_UMBER, TERM_GREEN, TERM_RED,
	TERM_ORANGE, TERM_COLD, TERM_L_BLUE, TERM_BLUE,
	TERM_ELEC, TERM_L_DARK, TERM_UMBER, TERM_SHAR,
	TERM_ACID, TERM_L_GREEN, TERM_CONF, TERM_RED,
	TERM_L_BLUE, TERM_BLUE, TERM_GREEN, TERM_RED,
	TERM_LITE, TERM_GREEN, TERM_POIS, TERM_SLATE,
	TERM_ACID, TERM_L_WHITE, TERM_VIOLET, TERM_L_BLUE,
	TERM_L_GREEN, TERM_L_RED, TERM_BLUE, TERM_RED,
	TERM_GREEN, TERM_VIOLET, TERM_L_WHITE, TERM_SOUN,
	TERM_L_RED, TERM_L_RED, TERM_VIOLET, TERM_VIOLET,
	TERM_VIOLET, TERM_RED, TERM_RED, TERM_COLD,
	TERM_DARKNESS, TERM_ORANGE, TERM_VIOLET, TERM_RED,
	TERM_WHITE, TERM_YELLOW, TERM_VIOLET, TERM_L_RED,
	TERM_RED, TERM_L_RED, TERM_YELLOW, TERM_GREEN,
	TERM_MULTI, TERM_RED, TERM_YELLOW, TERM_LITE,
	TERM_L_UMBER, TERM_UMBER, TERM_L_DARK, TERM_RED,
	TERM_MULTI, /* 64 (basic TV_POTION) */
#ifdef EXPAND_TV_POTION
	TERM_WHITE,
#endif

	//TERM_L_BLUE, TERM_L_WHITE,
	//TERM_LITE, (amber)
	//lilac:TERM_VIOLET, radiant TERM_COLD,
	//TERM_POIS, TERM_ELEC, TERM_FIRE, TERM_COLD,
    };
 #else /* traditional conservative colour usage, less sparkly */

  #ifdef HOUSE_PAINTING
    byte potion_col[MAX_COLORS] = {
  #else
    static byte potion_col[MAX_COLORS] = {
  #endif
	TERM_WHITE, TERM_L_UMBER, TERM_GREEN, TERM_LITE,
	TERM_ORANGE, TERM_WHITE, TERM_L_BLUE, TERM_BLUE,
	TERM_BLUE, TERM_L_DARK, TERM_UMBER, TERM_UMBER,
	TERM_L_WHITE, TERM_L_GREEN, TERM_L_UMBER, TERM_RED,
	TERM_L_BLUE, TERM_BLUE, TERM_GREEN, TERM_RED,
	TERM_YELLOW, TERM_GREEN, TERM_GREEN, TERM_SLATE,
	TERM_SLATE, TERM_L_WHITE, TERM_VIOLET, TERM_L_BLUE,
	TERM_L_GREEN, TERM_RED, TERM_BLUE, TERM_RED,
	TERM_GREEN, TERM_VIOLET, TERM_L_WHITE, TERM_ORANGE,
	TERM_L_RED, TERM_L_RED, TERM_VIOLET, TERM_VIOLET,
	TERM_VIOLET, TERM_RED, TERM_RED, TERM_L_WHITE,
	TERM_L_DARK, TERM_ORANGE, TERM_VIOLET, TERM_RED,
	TERM_WHITE, TERM_YELLOW, TERM_VIOLET, TERM_L_RED,
	TERM_RED, TERM_L_RED, TERM_YELLOW, TERM_GREEN,
	TERM_MULTI, TERM_RED, TERM_YELLOW, TERM_YELLOW,
	TERM_L_UMBER, TERM_UMBER, TERM_L_DARK, TERM_RED,
	TERM_L_WHITE, /* 64 (basic TV_POTION) */
#ifdef EXPAND_TV_POTION
	TERM_WHITE,
#endif

	//TERM_L_BLUE, TERM_L_UMBER,
	//TERM_POIS, TERM_ELEC, TERM_FIRE, TERM_COLD,
    };
 #endif

#endif /* EXTRA_FLAVOURS */


/*
 * Syllables for scrolls (must be 1-4 letters each)
 */
static cptr syllables[MAX_SYLLABLES] = {
	"a", "ab", "ag", "aks", "ala", "an", "ankh", "app",
	"arg", "arze", "ash", "aus", "ban", "bar", "bat", "bek",
	"bie", "bin", "bit", "bjor", "blu", "bot", "bu",
	"byt", "comp", "con", "cos", "cre", "dalf", "dan",
	"den", "der", "doe", "dok", "eep", "el", "eng", "er", "ere", "erk",
	"esh", "evs", "fa", "fid", "flit", "for", "fri", "fu", "gan",
	"gar", "glen", "gop", "gre", "ha", "he", "hyd", "i",
	"ing", "ion", "ip", "ish", "it", "ite", "iv", "jo",
	"kho", "kli", "klis", "la", "lech", "man", "mar",
	"me", "mi", "mic", "mik", "mon", "mung", "mur", "nag", "nej",
	"nelg", "nep", "ner", "nes", "nis", "nih", "nin", "o",
	"od", "ood", "org", "orn", "ox", "oxy", "pay", "pet",
	"ple", "plu", "po", "pot", "prok", "re", "rea", "rhov",
	"ri", "ro", "rog", "rok", "rol", "sa", "san", "sat",
	"see", "sef", "seh", "shu", "ski", "sna", "sne", "snik",
	"sno", "so", "sol", "sri", "sta", "sun", "ta", "tab",
	"tem", "ther", "ti", "tox", "trol", "tue", "turs", "u",
	"ulk", "um", "un", "uni", "ur", "val", "viv", "vly",
	"vom", "wah", "wed", "werg", "wex", "whon", "wun", "x",
	"yerg", "yp", "zun", "tri", "blaa", "jah", "bul", "on",
	"foo", "ju", "xuxu"
};

/*
 * Hold the titles of scrolls, 6 to 14 characters each
 * Also keep an array of scroll colors (always WHITE for now)
 */

static char scroll_adj[MAX_TITLES][16];
static byte scroll_col[MAX_TITLES];


/*
 * Certain items have a flavor
 * This function is used only by "flavor_init()"
 */
static bool object_has_flavor(int i) {
	object_kind *k_ptr = &k_info[i];

	/* Check for flavor */
	switch (k_ptr->tval) {
	/* The standard "flavored" items */
	case TV_AMULET:
	case TV_RING:
	case TV_STAFF:
	case TV_WAND:
	case TV_SCROLL:
	case TV_POTION:
	case TV_POTION2:	// Hack inside!
	case TV_ROD:
		return(TRUE);

	/* Hack -- food SOMETIMES has a flavor */
	case TV_FOOD:
		if (k_ptr->sval <= SV_FOOD_MUSHROOMS_MAX) return(TRUE);
		return(FALSE);
	}

	/* Assume no flavor */
	return(FALSE);
}


/*
 * Certain items, if aware, are known instantly
 * This function is used only by "flavor_init()"
 *
 * XXX XXX XXX Add "EASY_KNOW" flag to "k_info.txt" file
 */
static bool object_easy_know(int i) {
	object_kind *k_ptr = &k_info[i];

	/* Analyze the "tval" */
	switch (k_ptr->tval) {
	/* Simple items */
	case TV_FLASK:
	case TV_JUNK:
	case TV_GAME:
	case TV_BOTTLE:
	case TV_SKELETON:
	case TV_SPIKE:
	case TV_EGG:
	case TV_FIRESTONE:
	case TV_CORPSE:
	case TV_HYPNOS:
	case TV_RUNE:
		return(TRUE);

	/* All Food, Potions, Scrolls, Rods */
	case TV_FOOD:
	case TV_POTION:
	case TV_POTION2:
	case TV_SCROLL:
	case TV_PARCHMENT:
	case TV_ROD:
	case TV_ROD_MAIN:
	case TV_BATERIE:
		return(TRUE);

	/* Some Rings, Amulets, Lites */
	case TV_RING:
	case TV_AMULET:
	case TV_LITE:
	/* added default (for tools which got EASY_KNOW added,
	   such as flint/climbing set) */
	default:
		if (k_ptr->flags3 & TR3_EASY_KNOW) return(TRUE);
		return(FALSE);
	}

	/* Nope */
	return(FALSE);
}


/*
 * Hack -- prepare the default object attr codes by tval
 *
 * XXX XXX XXX Off-load to "pref.prf" file
 */
static byte default_tval_to_attr(int tval) {
	switch (tval) {
	case TV_SKELETON:
	case TV_BOTTLE:
	case TV_JUNK:
	case TV_GAME:
		return(TERM_WHITE);

	case TV_CHEST:
		return(TERM_SLATE);

	case TV_SHOT:
	case TV_BOLT:
	case TV_ARROW:
		return(TERM_L_UMBER);

	case TV_LITE:
		return(TERM_YELLOW);

	case TV_SPIKE:
		return(TERM_SLATE);

	case TV_BOOMERANG: /* maybe L_UMBER? */
	case TV_BOW:
		return(TERM_UMBER);

	case TV_DIGGING:
		return(TERM_SLATE);

	case TV_BLUNT:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_AXE:
		return(TERM_L_WHITE);

	case TV_BOOTS:
	case TV_GLOVES:
	case TV_CROWN:
	case TV_HELM:
	case TV_SHIELD:
	case TV_CLOAK:
		return(TERM_L_UMBER);

	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_DRAG_ARMOR:
		return(TERM_SLATE);

	case TV_GOLEM:
		return(TERM_VIOLET);

	case TV_AMULET:
		return(TERM_ORANGE);

	case TV_RING:
		return(TERM_RED);

	case TV_STAFF:
		return(TERM_L_UMBER);

	case TV_WAND:
		return(TERM_L_GREEN);

	case TV_ROD:
		return(TERM_L_WHITE);

	case TV_SCROLL:
	case TV_PARCHMENT:
		return(TERM_WHITE);

	case TV_POTION:
	case TV_POTION2:
	case TV_BOOK:	/* Right? */
		return(TERM_L_BLUE);

	case TV_FLASK:
		return(TERM_YELLOW);

	case TV_FOOD:
		return(TERM_L_UMBER);

	case TV_TRAPKIT:
		return(TERM_SLATE);
	case TV_RUNE:
		return(TERM_L_RED);

#ifdef ENABLE_DEMOLITIONIST
	case TV_CHARGE:
		return(TERM_ORANGE);
	case TV_CHEMICAL:
		//return(TERM_UMBER); //too close to ranged weapons maybe
		return(TERM_YELLOW);
#endif
#ifdef ENABLE_SUBINVEN
	case TV_SUBINVEN:
 #ifdef SUBINVEN_UNIFIED_COLOUR
		return(TERM_SLATE); /* Use same as chests */
 #else
		return(TERM_WHITE); /* Really depends on subtype later */
 #endif
#endif
	}

	return(TERM_WHITE);
}


/*
 * Hack -- prepare the default object char codes by tval
 *
 * XXX XXX XXX Off-load to "pref.prf" file (?)
 */
static byte default_tval_to_char(int tval) {
	int i;

	/* Hack -- Guess at "correct" values for tval_to_char[] */
	for (i = 1; i < max_k_idx; i++) {
		object_kind *k_ptr = &k_info[i];

		/* Use the first value we find */
		if (k_ptr->tval == tval) return(k_ptr->k_char);
	}

	/* Default to space */
	return(' ');
}



/*
 * Prepare the "variable" part of the "k_info" array.
 *
 * The "color"/"metal"/"type" of an item is its "flavor".
 * For the most part, flavors are assigned randomly each game.
 *
 * Initialize descriptions for the "colored" objects, including:
 * Rings, Amulets, Staffs, Wands, Rods, Food, Potions, Scrolls.
 *
 * The first 4 entries for potions are fixed (Water, Apple Juice,
 * Slime Mold Juice, Unused Potion).
 *
 * Scroll titles are always between 6 and 14 letters long.  This is
 * ensured because every title is composed of whole words, where every
 * word is from 1 to 8 letters long (one or two syllables of 1 to 4
 * letters each), and that no scroll is finished until it attempts to
 * grow beyond 15 letters.  The first time this can happen is when the
 * current title has 6 letters and the new word has 8 letters, which
 * would result in a 6 letter scroll title.
 *
 * Duplicate titles are avoided by requiring that no two scrolls share
 * the same first four letters (not the most efficient method, and not
 * the least efficient method, but it will always work).
 *
 * Hack -- make sure everything stays the same for each saved game
 * This is accomplished by the use of a saved "random seed", as in
 * "town_gen()".  Since no other functions are called while the special
 * seed is in effect, so this function is pretty "safe".
 *
 * Note that the "hacked seed" may provide an RNG with alternating parity!
 */
void flavor_init(void) {
	int	i, j;
	byte	temp_col;
	cptr	temp_adj;
	bool	temp_cheap;


	/* Hack -- Use the "simple" RNG */
	Rand_quick = TRUE;

	/* Hack -- Induce consistant flavors */
	Rand_value = seed_flavor;


	/* Efficiency -- Rods/Wands share initial array */
	for (i = 0; i < MAX_METALS; i++) {
		rod_adj[i] = wand_adj[i];
		rod_col[i] = wand_col[i];
	}


	/* Rings have "ring colors" */
	for (i = 0; i < MAX_ROCKS; i++) {
		j = rand_int(MAX_ROCKS);
		temp_adj = ring_adj[i];
		ring_adj[i] = ring_adj[j];
		ring_adj[j] = temp_adj;
		temp_col = ring_col[i];
		ring_col[i] = ring_col[j];
		ring_col[j] = temp_col;
		temp_cheap = ring_cheap[i];
		ring_cheap[i] = ring_cheap[j];
		ring_cheap[j] = temp_cheap;
	}

	/* Amulets have "amulet colors" */
	for (i = 0; i < MAX_AMULETS; i++) {
		j = rand_int(MAX_AMULETS);
		temp_adj = amulet_adj[i];
		amulet_adj[i] = amulet_adj[j];
		amulet_adj[j] = temp_adj;
		temp_col = amulet_col[i];
		amulet_col[i] = amulet_col[j];
		amulet_col[j] = temp_col;
		temp_cheap = amulet_cheap[i];
		amulet_cheap[i] = amulet_cheap[j];
		amulet_cheap[j] = temp_cheap;
	}

	/* Staffs */
	for (i = 0; i < MAX_WOODS; i++) {
		j = rand_int(MAX_WOODS);
		temp_adj = staff_adj[i];
		staff_adj[i] = staff_adj[j];
		staff_adj[j] = temp_adj;
		temp_col = staff_col[i];
		staff_col[i] = staff_col[j];
		staff_col[j] = temp_col;
	}

	/* Wands */
	for (i = 0; i < MAX_METALS; i++) {
		j = rand_int(MAX_METALS);
		temp_adj = wand_adj[i];
		wand_adj[i] = wand_adj[j];
		wand_adj[j] = temp_adj;
		temp_col = wand_col[i];
		wand_col[i] = wand_col[j];
		wand_col[j] = temp_col;
	}

	/* Rods */
	for (i = 0; i < MAX_METALS; i++) {
		j = rand_int(MAX_METALS);
		temp_adj = rod_adj[i];
		rod_adj[i] = rod_adj[j];
		rod_adj[j] = temp_adj;
		temp_col = rod_col[i];
		rod_col[i] = rod_col[j];
		rod_col[j] = temp_col;
	}

	/* Foods (Mushrooms) */
	for (i = STATIC_SHROOMS; i < MAX_SHROOM; i++) {
		j = rand_int(MAX_SHROOM - STATIC_SHROOMS) + STATIC_SHROOMS;
		temp_adj = food_adj[i];
		food_adj[i] = food_adj[j];
		food_adj[j] = temp_adj;
		temp_col = food_col[i];
		food_col[i] = food_col[j];
		food_col[j] = temp_col;
	}

	/* Potions (the first 4 potions are fixed) */
	for (i = STATIC_COLORS; i < MAX_COLORS; i++) {
		j = rand_int(MAX_COLORS - STATIC_COLORS) + STATIC_COLORS;
		temp_adj = potion_adj[i];
		potion_adj[i] = potion_adj[j];
		potion_adj[j] = temp_adj;
		temp_col = potion_col[i];
		potion_col[i] = potion_col[j];
		potion_col[j] = temp_col;
	}

	/* Scrolls (random titles, always white) */
	for (i = 0; i < MAX_TITLES; i++) {
		/* Get a new title */
		while (TRUE) {
			char buf[80];
			bool okay;

			/* Start a new title */
			buf[0] = '\0';

			/* Collect words until done */
			while (TRUE) {
				int q, s;
				char tmp[80];

				/* Start a new word */
				tmp[0] = '\0';

				/* Choose one or two syllables */
				s = ((rand_int(100) < 30) ? 1 : 2);

				/* Add a one or two syllable word */
				for (q = 0; q < s; q++) {
					/* Add the syllable */
					strcat(tmp, syllables[rand_int(MAX_SYLLABLES)]);
				}

				/* Stop before getting too long */
				if (strlen(buf) + 1 + strlen(tmp) > 15) break;

				/* Add a space */
				strcat(buf, " ");

				/* Add the word */
				strcat(buf, tmp);
			}

			/* Save the title */
			strcpy(scroll_adj[i], buf + 1);

			/* Assume okay */
			okay = TRUE;

			/* Check for "duplicate" scroll titles */
			for (j = 0; j < i; j++) {
				cptr hack1 = scroll_adj[j];
				cptr hack2 = scroll_adj[i];

				/* Compare first four characters */
				if (*hack1++ != *hack2++) continue;
				if (*hack1++ != *hack2++) continue;
				if (*hack1++ != *hack2++) continue;
				if (*hack1++ != *hack2++) continue;

				/* Not okay */
				okay = FALSE;

				/* Stop looking */
				break;
			}

			/* Break when done */
			if (okay) break;
		}

		/* All scrolls are white */
#ifdef PLAYER_STORES
		if (i == SV_SCROLL_CHEQUE) scroll_col[i] = TERM_L_UMBER;
		else
#endif
		if (i == SV_SCROLL_FIREWORK) scroll_col[i] = TERM_L_RED; else
		scroll_col[i] = TERM_WHITE;
	}


	/* Hack -- Use the "complex" RNG */
	Rand_quick = FALSE;

	/* Analyze every object */
	for (i = 1; i < max_k_idx; i++) {
		object_kind *k_ptr = &k_info[i];

		/* Skip "empty" objects */
		if (!k_ptr->name) continue;

		/* Check for a "flavor" */
		k_ptr->has_flavor = object_has_flavor(i);

		/* No flavor yields aware */
		//if (!k_ptr->has_flavor) k_ptr->aware = TRUE;
		//if ((!k_ptr->flavor) && (k_ptr->tval != TV_ROD_MAIN)) k_ptr->aware = TRUE;


		/* Check for "easily known" */
		k_ptr->easy_know = object_easy_know(i);
	}
}
/* Hack certain flavours for objects that have immutable colour. - C. Blue
   This means INSTA_ARTs in the ring and amulet department,
   which have colours determined by lore that must therefore be accurate. */
void flavor_hacks(void) {
	int i, j, k;
	byte temp_col;
	cptr temp_adj;
	bool temp_cheap;

	/* Check all INSTA_ART rings and amulets in k_info */
	for (i = 0; i < MAX_K_IDX; i++) {
		if (!(k_info[i].flags3 & TR3_INSTA_ART)) continue;
		switch (k_info[i].tval) {
		case TV_AMULET:
			/* Hack: TERM_DARK colour means: Don't reassign (keep it random) */
			if (k_info[i].k_attr == TERM_DARK) {
				/* At least make sure it doesn't use unfitting material in general */
				if (!amulet_cheap[k_info[i].sval]) continue;
				/* Find a fitting colour and switch the current object's colour with it */
				for (j = 0; j < MAX_AMULETS; j++) {
					/* Skip unfitting colours */
					if (amulet_cheap[j]) continue;
					/* Don't steal colour of another INSTA_ART that has already found its fitting colour */
					k = lookup_kind(TV_AMULET, j);
					if ((k_info[k].flags3 & TR3_INSTA_ART) && amulet_col[j] == k_info[k].k_attr) continue;
					/* Ok! Switch them */
					temp_col = amulet_col[k_info[i].sval];
					temp_adj = amulet_adj[k_info[i].sval];
					temp_cheap = amulet_cheap[k_info[i].sval];
					amulet_col[k_info[i].sval] = amulet_col[j];
					amulet_adj[k_info[i].sval] = amulet_adj[j];
					amulet_cheap[k_info[i].sval] = amulet_cheap[j];
					amulet_col[j] = temp_col;
					amulet_adj[j] = temp_adj;
					amulet_cheap[j] = temp_cheap;
					break;
				}
				if (j == MAX_AMULETS) s_printf(" Flavour couldn't be switched for item %d,%d.\n", k_info[i].tval, k_info[i].sval);
				continue;
			}
			/* Already comes with correct colour assigned? */
			if (k_info[i].k_attr == amulet_col[k_info[i].sval]) continue;
			/* Find a fitting colour and switch the current object's colour with it */
			for (j = 0; j < MAX_AMULETS; j++) {
				/* Skip unfitting colours */
				if (amulet_cheap[j]) continue;
				if (amulet_col[j] != k_info[i].k_attr) continue;
				/* Don't steal colour of another INSTA_ART that has already found its fitting colour  */
				k = lookup_kind(TV_AMULET, j);
				if ((k_info[k].flags3 & TR3_INSTA_ART) && amulet_col[j] == k_info[k].k_attr) continue;
				/* Ok! Switch them */
				temp_col = amulet_col[k_info[i].sval];
				temp_adj = amulet_adj[k_info[i].sval];
				temp_cheap = amulet_cheap[k_info[i].sval];
				amulet_col[k_info[i].sval] = amulet_col[j];
				amulet_adj[k_info[i].sval] = amulet_adj[j];
				amulet_cheap[k_info[i].sval] = amulet_cheap[j];
				amulet_col[j] = temp_col;
				amulet_adj[j] = temp_adj;
				amulet_cheap[j] = temp_cheap;
				break;
			}
			if (j == MAX_AMULETS) s_printf(" Flavour couldn't be switched for item %d,%d.\n", k_info[i].tval, k_info[i].sval);
			break;
		case TV_RING:
			/* Hack: TERM_DARK colour means: Don't reassign (keep it random) */
			if (k_info[i].k_attr == TERM_DARK) {
				/* At least make sure it doesn't use unfitting material in general */
				if (!ring_cheap[k_info[i].sval]) continue;
				/* Find a fitting colour and switch the current object's colour with it */
				for (j = 0; j < MAX_ROCKS; j++) {
					/* Skip unfitting colours */
					if (ring_cheap[j]) continue;
					/* Don't steal colour of another INSTA_ART that has already found its fitting colour */
					k = lookup_kind(TV_RING, j);
					if ((k_info[k].flags3 & TR3_INSTA_ART) && ring_col[j] == k_info[k].k_attr) continue;
					/* Ok! Switch them */
					temp_col = ring_col[k_info[i].sval];
					temp_adj = ring_adj[k_info[i].sval];
					temp_cheap = ring_cheap[k_info[i].sval];
					ring_col[k_info[i].sval] = ring_col[j];
					ring_adj[k_info[i].sval] = ring_adj[j];
					ring_cheap[k_info[i].sval] = ring_cheap[j];
					ring_col[j] = temp_col;
					ring_adj[j] = temp_adj;
					ring_cheap[j] = temp_cheap;
					break;
				}
				if (j == MAX_ROCKS) s_printf(" Flavour couldn't be switched for item %d,%d.\n", k_info[i].tval, k_info[i].sval);
				continue;
			}
			/* Already comes with correct colour assigned? */
			if (k_info[i].k_attr == ring_col[k_info[i].sval]
			    //special hack for The One Ring: (part 3/3)
			    && !(k_info[i].sval == SV_RING_POWER && strcmp(ring_adj[k_info[i].sval], "Plain Gold"))) //hopefully correct
				continue;
			/* Find a fitting colour and switch the current object's colour with it */
			for (j = 0; j < MAX_ROCKS; j++) {
				/* Skip unfitting colours */
				if (ring_cheap[j]) continue;
				if (ring_col[j] != k_info[i].k_attr) continue;
				//special hack for The One Ring: (part 1/3)
				if (k_info[i].sval == SV_RING_POWER && strcmp(ring_adj[j], "Plain Gold")) continue;
				/* Don't steal colour of another INSTA_ART that has already found its fitting colour */
				k = lookup_kind(TV_RING, j);
				if ((k_info[k].flags3 & TR3_INSTA_ART) && ring_col[j] == k_info[k].k_attr
				    && k_info[i].sval != SV_RING_POWER) //special hack for The One Ring: (part 2/3)
					continue;
				/* Ok! Switch them */
				temp_col = ring_col[k_info[i].sval];
				temp_adj = ring_adj[k_info[i].sval];
				temp_cheap = ring_cheap[k_info[i].sval];
				ring_col[k_info[i].sval] = ring_col[j];
				ring_adj[k_info[i].sval] = ring_adj[j];
				ring_cheap[k_info[i].sval] = ring_cheap[j];
				ring_col[j] = temp_col;
				ring_adj[j] = temp_adj;
				ring_cheap[j] = temp_cheap;
				break;
			}
			if (j == MAX_ROCKS) s_printf(" Flavour couldn't be switched for item %d,%d.\n", k_info[i].tval, k_info[i].sval);
			break;
		default: continue;
		}
	}
}


/*
 * Extract the "default" attr for each object
 * This function is used only by "flavor_init()"
 */
static byte object_d_attr(int i) {
	object_kind *k_ptr = &k_info[i];

	/* Flavored items */
	if (k_ptr->has_flavor) {
		/* Extract the indexx */
		int indexx = k_ptr->sval;

		/* Analyze the item */
		switch (k_ptr->tval) {
		case TV_FOOD:
			/* Hack for Mushroom of Unmagic */
			if (indexx == SV_FOOD_UNMAGIC) return(TERM_MULTI);
			else return(food_col[indexx]);
		case TV_POTION: return(potion_col[indexx]);
		case TV_SCROLL: return(scroll_col[indexx]);
		case TV_AMULET: return(amulet_col[indexx]);
		case TV_RING:   return(ring_col[indexx]);
		case TV_STAFF:  return(staff_col[indexx]);
		case TV_WAND:   return(wand_col[indexx]);
		case TV_ROD:    return(rod_col[indexx]);

		/* hack -- borrow those of potions */
		case TV_POTION2: return(potion_col[indexx + 4]); /* the first 4 potions are unique */
		}
	}

	/* Default attr if legal */
	if (k_ptr->k_attr) return(k_ptr->k_attr);

	/* Default to white */
	return(TERM_WHITE);
}


/*
 * Extract the "default" char for each object
 * This function is used only by "flavor_init()"
 */
static byte object_d_char(int i) {
	object_kind *k_ptr = &k_info[i];

	return(k_ptr->k_char);
}


/*
 * Reset the "visual" lists
 *
 * This is useful for switching on/off the "use_graphics" flag.
 */
void reset_visuals(void) {
	int i;

	/* Extract some info about terrain features */
	for (i = 0; i < MAX_F_IDX; i++) {
		feature_type *f_ptr = &f_info[i];

		/* Assume we will use the underlying values */
		f_ptr->z_attr = f_ptr->f_attr;
		f_ptr->z_char = f_ptr->f_char;
	}

	/* Extract some info about objects */
	for (i = 0; i < max_k_idx; i++) {
		object_kind *k_ptr = &k_info[i];

		/* Extract the "underlying" attr */
		k_ptr->d_attr = object_d_attr(i);

		/* Extract the "underlying" char */
		k_ptr->d_char = object_d_char(i);

		/* Assume we will use the underlying values */
		k_ptr->x_attr = k_ptr->d_attr;
		k_ptr->x_char = k_ptr->d_char;
	}

	/* Extract some info about monsters */
	for (i = 0; i < MAX_R_IDX; i++) {
		/* Extract the "underlying" attr */
		r_info[i].x_attr = r_info[i].d_attr;

		/* Extract the "underlying" char */
		r_info[i].x_char = r_info[i].d_char;
	}

	/* Extract attr/chars for equippy items (by tval) */
	for (i = 0; i < 128; i++) {
		/* Extract a default attr */
		tval_to_attr[i] = default_tval_to_attr(i);

		/* Extract a default char */
		tval_to_char[i] = default_tval_to_char(i);
	}
}



/*
 * Obtain the "flags" for an item
 */
//void object_flags(object_type *o_ptr, u32b *f1, u32b *f2, u32b *f3)
void object_flags(object_type *o_ptr, u32b *f1, u32b *f2, u32b *f3, u32b *f4, u32b *f5, u32b *f6, u32b *esp) {
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Base object */
	(*f1) = k_ptr->flags1;
	(*f2) = k_ptr->flags2;
	(*f3) = k_ptr->flags3;
	(*f4) = k_ptr->flags4;
	(*f5) = k_ptr->flags5;
	(*f6) = k_ptr->flags6;
	(*esp) = k_ptr->esp;

	artifact_type *a_ptr;

	/* Artifact */
	if (o_ptr->name1) {
		/* Hack -- Randarts! */
		if (o_ptr->name1 == ART_RANDART) {
			if (!(a_ptr = randart_make(o_ptr))) return;
			(*f1) = a_ptr->flags1;
			(*f2) = a_ptr->flags2;
			(*f3) = a_ptr->flags3;
			(*f4) = a_ptr->flags4;
			(*f5) = a_ptr->flags5;
			(*f6) = a_ptr->flags6;
			(*esp) = a_ptr->esp;
		} else {
			if (!(a_ptr = &a_info[o_ptr->name1])) return;
			(*f1) |= a_ptr->flags1;
			(*f2) |= a_ptr->flags2;
			(*f3) |= a_ptr->flags3;
			(*f4) |= a_ptr->flags4;
			(*f5) |= a_ptr->flags5;
			(*f6) |= a_ptr->flags6;
			(*esp) |= a_ptr->esp;
		}
#if 0
		if ((!object_flags_no_set) && (a_ptr->set != -1))
			apply_flags_set(o_ptr->name1, a_ptr->set, f1, f2, f3, f4, f5, f6, esp);
#endif	// 0
	}

	/* Ego-item */
	if (o_ptr->name2) {
		a_ptr = ego_make(o_ptr);

		(*f1) |= a_ptr->flags1;
		(*f2) |= a_ptr->flags2;
		(*f3) |= a_ptr->flags3;
		(*f4) |= a_ptr->flags4;
		(*f5) |= a_ptr->flags5;
		(*f6) |= a_ptr->flags6;
		(*esp) |= a_ptr->esp;
	}

	/* Sigil */
	if (o_ptr->sigil) {
		bool failed = 0;
		if (o_ptr->sseed) {
			/* Build the flag pool */
			u32b flag_pool[192]; byte flag_category[192]; byte flag_count = 0; //192 is 32*6, aka max # of flags - Kurzel
			s16b pval = o_ptr->pval; //PVAL for discrimination of flags
			byte sigil = o_ptr->sigil;

			/* Save RNG */
			bool old_rand = Rand_quick;
			u32b tmp_seed = Rand_value;

			/* Use the stored/quick RNG */
			Rand_quick = TRUE;
			Rand_value = o_ptr->sseed;

			if (sigil == SV_R_LITE) {
				if (!((*f2) & TR2_RES_LITE)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_LITE; flag_count++; }
				if (!((*f2) & TR2_RES_BLIND)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_BLIND; flag_count++; }
				if (!((*f3) & TR3_SEE_INVIS)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_SEE_INVIS; flag_count++; }
				if (!((*f3) & TR3_LITE1)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_LITE1; flag_count++; }
				switch (o_ptr->tval) {
					case TV_MSTAFF:
					case TV_SWORD:
					case TV_AXE:
					case TV_BLUNT:
					case TV_POLEARM:
						if (!((*f3) & TR3_SEE_INVIS)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_SEE_INVIS; flag_count++; }
					break;
					case TV_HELM:
					case TV_CROWN:
						if (!((*f2) & TR2_RES_BLIND)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_BLIND; flag_count++; }
						if (!((*f1) & TR1_INFRA) && pval) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_INFRA; flag_count++; }
					break;
					default:
					break;
				}
			}

			else if (sigil == SV_R_DARK) {
				if (!((*f2) & TR2_RES_DARK)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_DARK; flag_count++; }
				if (!((*f2) & TR2_RES_BLIND)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_BLIND; flag_count++; }
				if (!((*f3) & TR3_SEE_INVIS)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_SEE_INVIS; flag_count++; }
				if (!((*f1) & TR1_STEALTH) && pval && (pval < 6)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_STEALTH; flag_count++; }
				switch (o_ptr->tval) {
					case TV_MSTAFF:
					case TV_SWORD:
					case TV_AXE:
					case TV_BLUNT:
					case TV_POLEARM:
						if (!((*f3) & TR3_SEE_INVIS)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_SEE_INVIS; flag_count++; }
					break;
					case TV_CLOAK:
						if (!((*f5) & TR5_INVIS)) { flag_category[flag_count] = 5; flag_pool[flag_count] = TR5_INVIS; flag_count++; }
					case TV_SOFT_ARMOR:
					case TV_HARD_ARMOR:
					case TV_DRAG_ARMOR:
					case TV_BOOTS:
						if (!((*f1) & TR1_STEALTH) && pval && (pval < 6)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_STEALTH; flag_count++; }
					break;
					case TV_HELM:
					case TV_CROWN:
						if (!((*f2) & TR2_RES_BLIND)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_BLIND; flag_count++; }
						if (!((*f1) & TR1_INFRA) && pval) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_INFRA; flag_count++; }
					break;
					default:
					break;
				}
			}

			else if (sigil == SV_R_NEXU) {
				if (!((*f2) & TR2_RES_NEXUS)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_NEXUS; flag_count++; }
				if (!((*f2) & TR2_SUST_STR)
				 || !((*f2) & TR2_SUST_INT)
				 || !((*f2) & TR2_SUST_WIS)
				 || !((*f2) & TR2_SUST_DEX)
				 || !((*f2) & TR2_SUST_CON)
				 || !((*f2) & TR2_SUST_CHR)) {
					flag_category[flag_count] = 2;
					flag_pool[flag_count] = (TR2_SUST_STR | TR2_SUST_INT | TR2_SUST_WIS | TR2_SUST_DEX | TR2_SUST_CON | TR2_SUST_CHR);
					flag_count++;
				}
				switch (o_ptr->tval) {
					case TV_SWORD:
					case TV_AXE:
					case TV_BLUNT:
					case TV_POLEARM:
						if (!((*f1) & TR1_SLAY_ANIMAL)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_SLAY_ANIMAL; flag_count++; }
						if (!((*f1) & TR1_SLAY_ORC)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_SLAY_ORC; flag_count++; }
						if (!((*f1) & TR1_SLAY_TROLL)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_SLAY_TROLL; flag_count++; }
						if (!((*f1) & TR1_SLAY_GIANT)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_SLAY_GIANT; flag_count++; }
						if (!((*f1) & TR1_SLAY_DRAGON) && !((*f1) & TR1_KILL_DRAGON)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_SLAY_DRAGON; flag_count++; }
						if (!((*f1) & TR1_SLAY_DEMON) && !((*f1) & TR1_KILL_DEMON)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_SLAY_DEMON; flag_count++; }
						if (!((*f1) & TR1_SLAY_UNDEAD) && !((*f1) & TR1_KILL_UNDEAD)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_SLAY_UNDEAD; flag_count++; }
						if (!((*f1) & TR1_KILL_DRAGON)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_KILL_DRAGON; flag_count++; }
						if (!((*f1) & TR1_KILL_DEMON)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_KILL_DEMON; flag_count++; }
						if (!((*f1) & TR1_KILL_UNDEAD)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_KILL_UNDEAD; flag_count++; }
						if (!((*f1) & TR1_SLAY_EVIL)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_SLAY_EVIL; flag_count++; }
					break;
					default:
					break;
				}
			}

			else if (sigil == SV_R_NETH) {
				if (!((*f2) & TR2_RES_NETHER)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_NETHER; flag_count++; }
				if (!((*f2) & TR2_HOLD_LIFE)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_HOLD_LIFE; flag_count++; }
				switch (o_ptr->tval) {
					case TV_SWORD:
					case TV_AXE:
					case TV_BLUNT:
					case TV_POLEARM:
					case TV_GLOVES:
						if (!((*f1) & TR1_VAMPIRIC)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_VAMPIRIC; flag_count++; }
					break;
					case TV_SOFT_ARMOR:
					case TV_HARD_ARMOR:
					case TV_DRAG_ARMOR:
					case TV_CLOAK:
						if (!((*f2) & TR2_HOLD_LIFE)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_HOLD_LIFE; flag_count++; }
					break;
					default:
					break;
				}
			}

			else if (sigil == SV_R_CHAO) {
				if (!((*f2) & TR2_RES_CHAOS)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_CHAOS; flag_count++; }
				if (!((*f2) & TR2_RES_CONF)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_CONF; flag_count++; }
				switch (o_ptr->tval) {
					case TV_SWORD:
					case TV_AXE:
					case TV_BLUNT:
					case TV_POLEARM:
						if (!((*f5) & TR5_CHAOTIC)) { flag_category[flag_count] = 5; flag_pool[flag_count] = TR5_CHAOTIC; flag_count++; }
					break;
					default:
					break;
				}
			}

			else if (sigil == SV_R_MANA) {
				if (!((*f5) & TR5_RES_MANA)) { flag_category[flag_count] = 5; flag_pool[flag_count] = TR5_RES_MANA; flag_count++; }
				if (!((*f3) & TR3_REGEN_MANA)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_REGEN_MANA; flag_count++; }
				switch (o_ptr->tval) {
					case TV_CROWN:
						if (!((*f1) & TR1_MANA) && pval
					&& !(pval > 3))
							{ flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_MANA; flag_count++; }
						if (!((*f3) & TR3_REGEN_MANA)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_REGEN_MANA; flag_count++; }
					break;
					case TV_GLOVES:
						if (!((*f1) & TR1_MANA) && pval
						&& !((((*f5) & TR5_CRIT) || ((*f1) & TR1_SPEED)) && (pval > 7))
						&& !(((*f5) & TR5_CRIT) && ((*f1) & TR1_SPEED) && (pval > 5)))
							{ flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_MANA; flag_count++; }
					break;
					default:
					break;
				}
			}

			else if (sigil == SV_R_CONF) {
				if (!((*f2) & TR2_RES_CONF)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_CONF; flag_count++; }
				if (!((*f1) & TR1_INT) && pval && (pval < 7)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_INT; flag_count++; }
				if (!((*f1) & TR1_WIS) && pval && (pval < 7)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_WIS; flag_count++; }
				switch (o_ptr->tval) {
					case TV_CROWN:
						if (!((*f1) & TR1_INT) && pval && (pval < 7)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_INT; flag_count++; }
						if (!((*f1) & TR1_WIS) && pval && (pval < 7)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_WIS; flag_count++; }
					break;
					case TV_SHIELD:
					case TV_HARD_ARMOR:
					case TV_DRAG_ARMOR:
						if (!((*f5) & TR5_REFLECT)) { flag_category[flag_count] = 5; flag_pool[flag_count] = TR5_REFLECT; flag_count++; }
					break;
					default:
					break;
				}
			}

			else if (sigil == SV_R_INER) {
				if (!((*f2) & TR2_FREE_ACT)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_FREE_ACT; flag_count++; }
				if (!((*f3) & TR3_SLOW_DIGEST)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_SLOW_DIGEST; flag_count++; }
				switch (o_ptr->tval) {
					case TV_GLOVES:
					case TV_BOOTS:
						if (!((*f2) & TR2_FREE_ACT)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_FREE_ACT; flag_count++; }
					break;
					default:
					break;
				}
			}

			else if (sigil == SV_R_ELEC) {
				if (!((*f2) & TR2_RES_ELEC) && !((*f2) & TR2_IM_ELEC)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_ELEC; flag_count++; }
				if (!((*f2) & TR2_IM_ELEC) && !(o_ptr->sval == SV_DRAGON_MULTIHUED)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_IM_ELEC; flag_count++; }
				if (!((*f3) & TR3_LITE1)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_LITE1; flag_count++; }
				if (!((*f1) & TR1_DEX) && pval && (pval < 7)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_DEX; flag_count++; }
				switch (o_ptr->tval) {
					case TV_SWORD:
					case TV_AXE:
					case TV_BLUNT:
					case TV_POLEARM:
					case TV_GLOVES:
						if (!((*f1) & TR1_BRAND_ELEC)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_BRAND_ELEC; flag_count++; }
					break;
					case TV_CLOAK:
						if (!((*f3) & TR3_SH_ELEC)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_SH_ELEC; flag_count++; }
					break;
					default:
					break;
				}
			}

			else if (sigil == SV_R_FIRE) {
				if (!((*f2) & TR2_RES_FIRE) && !((*f2) & TR2_IM_FIRE)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_FIRE; flag_count++; }
				if (!((*f2) & TR2_IM_FIRE) && !(o_ptr->sval == SV_DRAGON_MULTIHUED)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_IM_FIRE; flag_count++; }
				if (!((*f3) & TR3_LITE1)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_LITE1; flag_count++; }
				if (!((*f1) & TR1_STR) && pval && (pval < 7)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_STR; flag_count++; }
				switch (o_ptr->tval) {
					case TV_SWORD:
					case TV_AXE:
					case TV_BLUNT:
					case TV_POLEARM:
					case TV_GLOVES:
						if (!((*f1) & TR1_BRAND_FIRE)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_BRAND_FIRE; flag_count++; }
					break;
					case TV_CLOAK:
						if (!((*f3) & TR3_SH_FIRE)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_SH_FIRE; flag_count++; }
					break;
					default:
					break;
				}
			}

			else if (sigil == SV_R_WATE) {
				if (!((*f2) & TR2_RES_WATER) && !((*f2) & TR2_RES_WATER)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_WATER; flag_count++; }
				if (!((*f2) & TR2_IM_WATER) && !(o_ptr->sval == SV_DRAGON_MULTIHUED)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_IM_WATER; flag_count++; }
				if (!((*f3) & TR3_SLOW_DIGEST)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_SLOW_DIGEST; flag_count++; }
			}

			else if (sigil == SV_R_GRAV) {
				if (!((*f3) & TR3_FEATHER) && !((*f4) & TR4_LEVITATE)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_FEATHER; flag_count++; }
				switch (o_ptr->tval) {
					case TV_CLOAK:
					case TV_BOOTS:
						if (!((*f3) & TR3_FEATHER) && !((*f4) & TR4_LEVITATE)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_FEATHER; flag_count++; }
						if (!((*f4) & TR4_LEVITATE)) { flag_category[flag_count] = 4; flag_pool[flag_count] = TR4_LEVITATE; flag_count++; }
					break;
					default:
					break;
				}
			}

			else if (sigil == SV_R_COLD) {
				if (!((*f2) & TR2_RES_COLD) && !((*f2) & TR2_IM_COLD)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_COLD; flag_count++; }
				if (!((*f2) & TR2_IM_COLD) && !(o_ptr->sval == SV_DRAGON_MULTIHUED)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_IM_COLD; flag_count++; }
				if (!((*f3) & TR3_SLOW_DIGEST)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_SLOW_DIGEST; flag_count++; }
				if (!((*f1) & TR1_STR) && pval && (pval < 7)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_STR; flag_count++; }
				switch (o_ptr->tval) {
					case TV_SWORD:
					case TV_AXE:
					case TV_BLUNT:
					case TV_POLEARM:
					case TV_GLOVES:
						if (!((*f1) & TR1_BRAND_COLD)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_BRAND_COLD; flag_count++; }
					break;
					case TV_CLOAK:
						if (!((*f3) & TR3_SH_COLD)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_SH_COLD; flag_count++; }
					break;
					default:
					break;
				}
			}

			else if (sigil == SV_R_ACID) {
				if (!((*f2) & TR2_RES_ACID) && !((*f2) & TR2_IM_ACID)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_ACID; flag_count++; }
				if (!((*f2) & TR2_IM_ACID) && !(o_ptr->sval == SV_DRAGON_MULTIHUED)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_IM_ACID; flag_count++; }
				if (!((*f3) & TR3_SLOW_DIGEST)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_SLOW_DIGEST; flag_count++; }
				if (!((*f1) & TR1_CHR) && pval && (pval < 7)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_CHR; flag_count++; }
				switch (o_ptr->tval) {
					case TV_SWORD:
					case TV_AXE:
					case TV_BLUNT:
					case TV_POLEARM:
					case TV_GLOVES:
						if (!((*f1) & TR1_BRAND_ACID)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_BRAND_ACID; flag_count++; }
					break;
					default:
					break;
				}
			}

			else if (sigil == SV_R_POIS) {
				if (!((*f2) & TR2_RES_POIS) && !((*f2) & TR2_IM_POISON)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_POIS; flag_count++; }
				if (!((*f2) & TR2_IM_POISON) && !(o_ptr->sval == SV_DRAGON_MULTIHUED)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_IM_POISON; flag_count++; }
				if (!((*f1) & TR1_CON) && pval && (pval < 7)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_CON; flag_count++; }
				switch (o_ptr->tval) {
					case TV_SWORD:
					case TV_AXE:
					case TV_BLUNT:
					case TV_POLEARM:
					case TV_GLOVES:
						if (!((*f1) & TR1_BRAND_POIS)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_BRAND_POIS; flag_count++; }
					break;
					case TV_SOFT_ARMOR:
					case TV_HARD_ARMOR:
					case TV_DRAG_ARMOR:
						if (!((*f1) & TR1_CON) && pval && (pval < 7)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_CON; flag_count++; }
					break;
					default:
					break;
				}
			}

			else if (sigil == SV_R_TIME) {
				if (!((*f5) & TR5_RES_TIME)) { flag_category[flag_count] = 5; flag_pool[flag_count] = TR5_RES_TIME; flag_count++; }
				if (!((*f1) & TR1_SPEED) && pval && !(o_ptr->tval == TV_SHIELD)
				&& !(is_melee_weapon(o_ptr->tval) && !(((*f4) & TR4_SHOULD2H) || ((*f4) & TR4_MUST2H)) && (pval > 3))
				&& !((((*f4) & TR4_SHOULD2H) || ((*f4) & TR4_MUST2H)) && (pval > 5))
				&& !((((*f5) & TR5_CRIT) || ((*f1) & TR1_MANA)) && (pval > 7))
				&& !(((*f5) & TR5_CRIT) && ((*f1) & TR1_MANA) && (pval > 5)))
					{ flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_SPEED; flag_count++; }
				if (!((*f2) & TR2_SUST_STR)
				 || !((*f2) & TR2_SUST_INT)
				 || !((*f2) & TR2_SUST_WIS)
				 || !((*f2) & TR2_SUST_DEX)
				 || !((*f2) & TR2_SUST_CON)
				 || !((*f2) & TR2_SUST_CHR)) {
					flag_category[flag_count] = 2;
					flag_pool[flag_count] = (TR2_SUST_STR | TR2_SUST_INT | TR2_SUST_WIS | TR2_SUST_DEX | TR2_SUST_CON | TR2_SUST_CHR);
					flag_count++;
				}
				switch (o_ptr->tval) {
					case TV_SWORD:
					case TV_AXE:
					case TV_BLUNT:
					case TV_POLEARM:
						if (!((*f1) & TR1_BLOWS) && pval && (pval < 4)
						&& !(((*f1) & TR1_LIFE) && (pval > 1)))
							{ flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_BLOWS; flag_count++; }
					break;
					case TV_GLOVES:
						if (!((*f1) & TR1_BLOWS) && pval && (pval < 3)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_BLOWS; flag_count++; }
					break;
					case TV_BOOTS:
						if (!((*f1) & TR1_SPEED) && pval
						&& !((((*f5) & TR5_CRIT) || ((*f1) & TR1_MANA)) && (pval > 7))
						&& !(((*f5) & TR5_CRIT) && ((*f1) & TR1_MANA) && (pval > 5)))
							{ flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_SPEED; flag_count++; }
					break;
					default:
					break;
				}
			}

			else if (sigil == SV_R_SOUN) {
				if (!((*f2) & TR2_RES_SOUND)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_SOUND; flag_count++; }
				if (!((*f1) & TR1_STEALTH) && pval && (pval < 6)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_STEALTH; flag_count++; }
				switch (o_ptr->tval) {
					case TV_SOFT_ARMOR:
					case TV_HARD_ARMOR:
					case TV_DRAG_ARMOR:
					case TV_CLOAK:
					case TV_BOOTS:
						if (!((*f1) & TR1_STEALTH) && pval && (pval < 6)) { flag_category[flag_count] = 1; flag_pool[flag_count] = TR1_STEALTH; flag_count++; }
					break;
					default:
					break;
				}
			}

			else if (sigil == SV_R_SHAR) {
				if (!((*f2) & TR2_RES_SHARDS)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_SHARDS; flag_count++; }
				if (!((*f3) & TR3_REGEN)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_REGEN; flag_count++; }
				switch (o_ptr->tval) {
					case TV_BLUNT:
						if (!((*f5) & TR5_IMPACT)) { flag_category[flag_count] = 5; flag_pool[flag_count] = TR5_IMPACT; flag_count++; }
					break;
					case TV_SWORD:
					case TV_AXE:
					case TV_POLEARM:
						if (!((*f5) & TR5_VORPAL)) { flag_category[flag_count] = 5; flag_pool[flag_count] = TR5_VORPAL; flag_count++; }
					case TV_GLOVES:
						if (!((*f5) & TR5_CRIT) && pval
						&& !((((*f1) & TR1_SPEED) || ((*f1) & TR1_MANA)) && (pval > 7))
						&& !(((*f1) & TR1_SPEED) && ((*f1) & TR1_MANA) && (pval > 5)))
							{ flag_category[flag_count] = 5; flag_pool[flag_count] = TR5_CRIT; flag_count++; }
					break;
					default:
					break;
				}
			}

			else if (sigil == SV_R_HELL) {
				if (!((*f2) & TR2_IM_FIRE) && !(o_ptr->sval == SV_DRAGON_MULTIHUED)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_IM_FIRE; flag_count++; }
				switch (o_ptr->tval) {
					case TV_HELM:
					case TV_CROWN:
						if (!((*f2) & TR2_RES_FEAR)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_FEAR; flag_count++; }
					break;
					default:
					break;
				}
			}

			else if (sigil == SV_R_FORC) {
				if (!((*f2) & TR2_SUST_STR)
				 || !((*f2) & TR2_SUST_INT)
				 || !((*f2) & TR2_SUST_WIS)
				 || !((*f2) & TR2_SUST_DEX)
				 || !((*f2) & TR2_SUST_CON)
				 || !((*f2) & TR2_SUST_CHR)) {
					flag_category[flag_count] = 2;
					flag_pool[flag_count] = (TR2_SUST_STR | TR2_SUST_INT | TR2_SUST_WIS | TR2_SUST_DEX | TR2_SUST_CON | TR2_SUST_CHR);
					flag_count++;
				}
				switch (o_ptr->tval) {
					case TV_SHIELD:
					case TV_HARD_ARMOR:
					case TV_DRAG_ARMOR:
						if (!((*f5) & TR5_REFLECT)) { flag_category[flag_count] = 5; flag_pool[flag_count] = TR5_REFLECT; flag_count++; }
					break;
					default:
					break;
				}
			}

			else if (sigil == SV_R_DISE) {
				if (!((*f2) & TR2_RES_DISEN)) { flag_category[flag_count] = 2; flag_pool[flag_count] = TR2_RES_DISEN; flag_count++; }
				if (!((*f3) & TR3_NO_MAGIC)) { flag_category[flag_count] = 3; flag_pool[flag_count] = TR3_NO_MAGIC; flag_count++; }
			}

			else failed = 1;

			/* Assign a flag from the pool, sustain stats, remove conflicts */
			if (flag_count) {
				byte flag_pick = randint(flag_count) - 1;
				byte category = flag_category[flag_pick];

				if (category == 1) {
					switch (flag_pool[flag_pick]) {
						case TR1_STR: { if (!((*f2) & TR2_SUST_STR) && randint(2) == 1) (*f2) |= TR2_SUST_STR; break; }
						case TR1_DEX: { if (!((*f2) & TR2_SUST_DEX) && randint(2) == 1) (*f2) |= TR2_SUST_DEX; break; }
						case TR1_CON: { if (!((*f2) & TR2_SUST_CON) && randint(2) == 1) (*f2) |= TR2_SUST_CON; break; }
						case TR1_INT: { if (!((*f2) & TR2_SUST_INT) && randint(2) == 1) (*f2) |= TR2_SUST_INT; break; }
						case TR1_WIS: { if (!((*f2) & TR2_SUST_WIS) && randint(2) == 1) (*f2) |= TR2_SUST_WIS; break; }
						case TR1_CHR: { if (!((*f2) & TR2_SUST_CHR) && randint(2) == 1) (*f2) |= TR2_SUST_CHR; break; }
						case TR1_KILL_DRAGON: { if ((*f1) & TR1_SLAY_DRAGON) (*f1) &= ~(TR1_SLAY_DRAGON); (*f1) |= flag_pool[flag_pick]; break; }
						case TR1_KILL_DEMON: { if ((*f1) & TR1_SLAY_DEMON) (*f1) &= ~(TR1_SLAY_DEMON); (*f1) |= flag_pool[flag_pick]; break; }
						case TR1_KILL_UNDEAD: { if ((*f1) & TR1_SLAY_UNDEAD) (*f1) &= ~(TR1_SLAY_UNDEAD); (*f1) |= flag_pool[flag_pick]; break; }
						default:
						break;
					}
					(*f1) |= flag_pool[flag_pick];
				} else if (category == 2) {
					switch (flag_pool[flag_pick]) {
						case TR2_IM_ACID: { if ((*f2) & TR2_RES_ACID) (*f2) &= ~(TR2_RES_ACID); (*f2) |= flag_pool[flag_pick]; break; }
						case TR2_IM_ELEC: { if ((*f2) & TR2_RES_ELEC) (*f2) &= ~(TR2_RES_ELEC); (*f2) |= flag_pool[flag_pick]; break; }
						case TR2_IM_FIRE: { if ((*f2) & TR2_RES_FIRE) (*f2) &= ~(TR2_RES_FIRE); (*f2) |= flag_pool[flag_pick]; break; }
						case TR2_IM_COLD: { if ((*f2) & TR2_RES_COLD) (*f2) &= ~(TR2_RES_COLD); (*f2) |= flag_pool[flag_pick]; break; }
						case TR2_IM_POISON: { if ((*f2) & TR2_RES_POIS) (*f2) &= ~(TR2_RES_POIS); (*f2) |= flag_pool[flag_pick]; break; }
						case TR2_IM_WATER: { if ((*f2) & TR2_RES_WATER) (*f2) &= ~(TR2_RES_WATER); (*f2) |= flag_pool[flag_pick]; break; }
						default:
							(*f2) |= flag_pool[flag_pick];
						break;
					}
				} else if (category == 3) (*f3) |= flag_pool[flag_pick];
				else if (category == 4) {
					switch (flag_pool[flag_pick]) {
						case TR4_LEVITATE: { if ((*f3) & TR3_FEATHER) (*f3) &= ~(TR3_FEATHER); (*f4) |= flag_pool[flag_pick]; break; }
						default:
							(*f4) |= flag_pool[flag_pick];
						break;
					}
				} else if (category == 5) (*f5) |= flag_pool[flag_pick];
				else if (category == 6) (*esp) |= flag_pool[flag_pick];
				else failed = 1;
			} else failed = 1;

			/* Restore RNG */
			Rand_quick = old_rand;
			Rand_value = tmp_seed;
		}

		/* Sigil (reset it) */
		if (failed) {
			o_ptr->sigil = 0;
			o_ptr->sseed = 0;
		}
	}

	/* Hack for mindcrafter spell scrolls:
	   Since they're called 'crystals', add water+fire immunity.
	   Acid immunity is only for the greater crystals.
	   NOTE: When this occurrance of get_spellbook_name_colour() was still
	   the LUA version (exec_lua(0,..)) instead of this C version it was
	   probably responsible for (occasional) crash/error on spellcasting. */
	if (o_ptr->tval == TV_BOOK && o_ptr->sval == SV_SPELLBOOK &&
	    get_spellbook_name_colour(o_ptr->pval) == TERM_YELLOW) {
		(*f3) |= TR3_IGNORE_FIRE;
		(*f5) |= TR5_IGNORE_WATER;
	}

	if (o_ptr->questor && o_ptr->questor_invincible) {
		(*f3) |= TR3_IGNORE_FIRE;
		(*f3) |= TR3_IGNORE_COLD;
		(*f3) |= TR3_IGNORE_ACID;
		(*f3) |= TR3_IGNORE_ELEC;
		(*f5) |= TR5_IGNORE_WATER;
		(*f5) |= TR5_IGNORE_MANA;
		(*f5) |= TR5_IGNORE_DISEN;
	}
}



/*
 * Print a char "c" into a string "t", as if by sprintf(t, "%c", c),
 * and return a pointer to the terminator (t + 1).
 */
static char *object_desc_chr(char *t, char c) {
	/* Copy the char */
	*t++ = c;

	/* Terminate */
	*t = '\0';

	/* Result */
	return(t);
}

/*
 * Print a string "s" into a string "t", as if by strcpy(t, s),
 * and return a pointer to the terminator.
 */
static char *object_desc_str(char *t, cptr s) {
	/* Copy the string */
	while (*s) {
#if defined(POWINS_DYNAMIC) && !defined(POWINS_DYNAMIC_CLIENTSIDE)
		/* Hide the internal server-side markers */
		if (*s == '@' && (*(s + 1) == '&' || *(s + 1) == '^')) {
			s += 2;
			continue;
		}
#endif
		*t++ = *s++;
	}

	/* Terminate */
	*t = '\0';

	/* Result */
	return(t);
}

/*
 * Print an unsigned number "n" into a string "t", as if by
 * sprintf(t, "%u", n), and return a pointer to the terminator.
 */
static char *object_desc_num(char *t, uint n) {
	uint p;

	if (n > 99999) n = 99999;

	/* Find "size" of "n" */
	for (p = 1; n >= p * 10; p = p * 10) /* loop */;

	/* Dump each digit */
	while (p >= 1) {
		/* Dump the digit */
		*t++ = '0' + n / p;

		/* Remove the digit */
		n = n % p;

		/* Process next digit */
		p = p / 10;
	}

	/* Terminate */
	*t = '\0';

	/* Result */
	return(t);
}

/*
 * Print an signed number "v" into a string "t", as if by
 * sprintf(t, "%+d", n), and return a pointer to the terminator.
 * Note that we always print a sign, either "+" or "-".
 */
static char *object_desc_int(char *t, sint v) {
	uint p, n;

	/* Negative */
	if (v < 0) {
		/* Take the absolute value */
		n = 0 - v;

		/* Use a "minus" sign */
		*t++ = '-';
	}
	/* Positive (or zero) */
	else {
		/* Use the actual number */
		n = v;

		/* Use a "plus" sign */
		*t++ = '+';
	}

	/* Find "size" of "n" */
	for (p = 1; n >= p * 10; p = p * 10) /* loop */;

	/* Dump each digit */
	while (p >= 1) {
		/* Dump the digit */
		*t++ = '0' + n / p;

		/* Remove the digit */
		n = n % p;

		/* Process next digit */
		p = p / 10;
	}

	/* Terminate */
	*t = '\0';

	/* Result */
	return(t);
}

/*
 * Print a signed number "n" into a string "t", as if by
 * sprintf(t, "%+u%%", n), and return a pointer to the terminator.
 */
static char *object_desc_per(char *t, sint v) {
	uint p, n;

	/* Negative */
	if (v < 0) {
		/* Take the absolute value */
		n = 0 - v;

		/* Use a "minus" sign */
		*t++ = '-';
	}
	/* Positive (or zero) */
	else {
		/* Use the actual number */
		n = v;
	}

	/* Find "size" of "n" */
	for (p = 1; n >= p * 10; p = p * 10) /* loop */;

	/* Dump each digit */
	while (p >= 1) {
		/* Dump the digit */
		*t++ = '0' + n / p;

		/* Remove the digit */
		n = n % p;

		/* Process next digit */
		p = p / 10;
	}

	/* Use a "percent" sign */
	*t++ = '%';

	/* Terminate */
	*t = '\0';

	/* Result */
	return(t);
}

#ifdef WEAPONS_NO_AC
/*
 * Print an unsigned number "n" into a string "t", as if by
 * sprintf(t, "%u%%", n), and return a pointer to the terminator.
 * Note that we always print a sign, either "+" or "-".
 */
static char *object_desc_intper(char *t, sint v) {
	uint p, n;

	/* Negative */
	if (v < 0) {
		/* Take the absolute value */
		n = 0 - v;

		/* Use a "minus" sign */
		*t++ = '-';
	}
	/* Positive (or zero) */
	else {
		/* Use the actual number */
		n = v;

		/* Use a "plus" sign */
		*t++ = '+';
	}

	/* Find "size" of "n" */
	for (p = 1; n >= p * 10; p = p * 10) /* loop */;

	/* Dump each digit */
	while (p >= 1) {
		/* Dump the digit */
		*t++ = '0' + n / p;

		/* Remove the digit */
		n = n % p;

		/* Process next digit */
		p = p / 10;
	}

	/* Use a "percent" sign */
	*t++ = '%';

	/* Terminate */
	*t = '\0';

	/* Result */
	return(t);
}
#endif

#if 0 /* could be used for really long timeouts > 99999 */
/*
 * Print an unsigned large number "n" into a string "t", as if by
 * sprintf(t, "%u", n), and return a pointer to the terminator.
 */
static char *object_desc_lnum(char *t, uint n) {
	uint p;

	/* Find "size" of "n" */
	for (p = 1; n >= p * 10; p = p * 10) /* loop */;

	/* Dump each digit */
	while (p >= 1) {
		/* Dump the digit */
		*t++ = '0' + n / p;

		/* Remove the digit */
		n = n % p;

		/* Process next digit */
		p = p / 10;
	}

	/* Terminate */
	*t = '\0';

	/* Result */
	return(t);
}
#endif


/*
 * Creates a description of the item "o_ptr", and stores it in "out_val".
 *
 * One can choose the "verbosity" of the description, including whether
 * or not the "number" of items should be described, and how much detail
 * should be used when describing the item.
 *
 * The given "buf" must be 80 chars long to hold the longest possible
 * description, which can get pretty long, including incriptions, such as:
 * "no more Maces of Disruption (Defender) (+10,+10) [+5] (+3 to stealth)".
 * Note that the inscription will be clipped to keep the total description
 * under 79 chars (plus a terminator).
 *
 * Note the use of "object_desc_num()" and "object_desc_int()" as hyper-efficient,
 * portable, versions of some common "sprintf()" commands.
 *
 * Note that all ego-items (when known) append an "Ego-Item Name", unless
 * the item is also an artifact, which should NEVER happen.
 *
 * Note that all artifacts (when known) append an "Artifact Name", so we
 * have special processing for "Specials" (artifact Lites, Rings, Amulets).
 * The "Specials" never use "modifiers" if they are "known", since they
 * have special "descriptions", such as "The Necklace of the Dwarves".
 *
 * Special Lite's use the "k_info" base-name (Phial, Star, or Arkenstone),
 * plus the artifact name, just like any other artifact, if known.
 *
 * Special Ring's and Amulet's, if not "aware", use the same code as normal
 * rings and amulets, and if "aware", use the "k_info" base-name (Ring or
 * Amulet or Necklace).  They will NEVER "append" the "k_info" name.  But,
 * they will append the artifact name, just like any artifact, if known.
 *
 * None of the Special Rings/Amulets are "EASY_KNOW", though they could be,
 * at least, those which have no "pluses", such as the three artifact lites.
 *
 * Hack -- Display "The One Ring" as "a Plain Gold Ring" until aware.
 *
 * If "pref" then a "numeric" prefix will be pre-pended.
 *
 * Mode:
 *   0 -- The Cloak of Death
 *   1 -- The Cloak of Death [1,+3]
 *   2 -- The Cloak of Death [1,+3] (+2 to Stealth)
 *   3 -- The Cloak of Death [1,+3] (+2 to Stealth) {nifty}
 *   4 -- The Cloak of Death\373\5nifty [1,+3] (+2 to Stealth)
 *        same as 3, just preceed #-inscription by a char \373 and a char
 *        specifying its length, to make auto-inscriptions work.
 *
 *  +8 -- Cloak Death [1,+3](+2stl){nifty}
 *  +16 - Replace full owner name by a symbol to shorten the string - C. Blue
 *  +32 - Suppress the "(+2 to Stealth)" part (used for Fancy Shirts and seals only) - C. Blue
 *  +64 - Don't add 'total' behind amount of wand charges;
 *        used in store inventory in case STORE_SHOWS_SINGLE_WAND_CHARGES isn't defined;
 *        (not used in player/home inventory, but used in player stores again.) - C. Blue
 *  +128 - Don't prefix "The"
 *  +256 - Short name: Only the purely textual name, no stats/level/owner/status.
 *  +512 - Don't suppress flavour for flavoured true arts or hacked-base-name items (insta-arts, ie rings and amulets) (for ~4 list)
 * +1024 - Assume that no flavours are known by the player (added for exporting player store item list)
 *         ONLY works with Ind == 0.
 * +2048 - Do not display anything referring to the base item (for seals and wrapped gifts). Add +32 too when using this.
 * +4096 - Assume item is not "known", even if Ind is 0.
 *
 * If the strings created with mode 0-3 are too long, this function is called
 * again with 8 added to 'mode' and attempt to 'abbreviate' the strings. -Jir-
 */
void object_desc(int Ind, char *buf, object_type *o_ptr, int pref, int mode) {
	player_type     *p_ptr = NULL;
	cptr		basenm, modstr;
	int		power, indexx;

	bool		aware = FALSE;
	bool		known = FALSE;

	bool		append_name = FALSE, hacked_base_name = FALSE;
	bool		switched_ego_prefix_and_modstr = FALSE;

	bool		show_weapon = FALSE;
	bool		show_armour = FALSE;
	bool		show_shield = FALSE;

	cptr		s_ptr;
	char		*t;

	char		p1 = '(', p2 = ')';
	char		b1 = '[', b2 = ']';
	char		c1 = '{', c2 = '}';

	char		tmp_val[ONAME_LEN];
	static char	basenm2[ONAME_LEN];
	bool		short_item_names = FALSE;
	u32b		f1, f2, f3, f4, f5, f6, esp;
	object_kind	*k_ptr = &k_info[o_ptr->k_idx];
	bool		skip_base_article = FALSE;
	bool		special_rop = (o_ptr->tval == TV_RING && o_ptr->sval == SV_RING_SPECIAL);
#ifdef ENABLE_DEMOLITIONIST
	char		tmp_modstr[ONAME_LEN];
#endif

	/* For custom objects mimicking other objects: */
	int		tval;
	object_type	forge, *ox_ptr = &forge;


	/* Extract some flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	/* hack - don't show special abilities on shirts easily (by adding them to their item name) - C. Blue */
	if (o_ptr->tval == TV_SOFT_ARMOR && o_ptr->sval == SV_SHIRT) mode |= 32;
	/* same for seals and gift wrappings - C. Blue */
	if (o_ptr->tval == TV_SPECIAL) mode |= 32 | 2048;

	/* Assume aware and known if not a valid player */
	if (Ind) {
		p_ptr = Players[Ind];
		short_item_names = p_ptr->short_item_names;

		/* See if the object is "aware" */
		if (object_aware_p(Ind, o_ptr)) aware = TRUE;

		/* See if the object is "known" */
		if (object_known_p(Ind, o_ptr)) known = TRUE;
	} else {
		/* Assume aware and known */
		aware = known = TRUE;
		if (mode & 1024) aware = FALSE; //don't spoil flavours in player shop export list!
		if (mode & 4096) known = FALSE;
	}
	/* Never use short item names in flavour knowledge list */
	if ((mode & 512)) short_item_names = FALSE;
#ifdef ENABLE_ITEM_ORDER
	/* Usually this is only used for arts, but now also for item orders,
	   so we need to strip the flavour really (way too annoying) */
	else if ((mode & 256)) short_item_names = TRUE;
#endif

	/* Hack -- Extract the sub-type "indexx" */
	indexx = o_ptr->sval;

	/* Extract default "base" string */
	basenm = (k_name + k_ptr->name);

	/* Assume no "modifier" string */
	modstr = "";


	/* Hack: Fix silly names on artifact flavoured items (rings/amulets) */
	if (artifact_p(o_ptr) && (k_info[o_ptr->k_idx].flags3 & TR3_INSTA_ART) && aware && !known && !special_rop && !(mode & 2048)) aware = FALSE;

	/* Analyze the object */
	switch (o_ptr->tval) {
	/* Some objects are easy to describe */
	case TV_SPECIAL:
		if (o_ptr->sval == SV_CUSTOM_OBJECT)
			basenm = o_ptr->note ? quark_str(o_ptr->note) : "";
	case TV_SKELETON:
	case TV_BOTTLE:
	case TV_JUNK:
	case TV_GAME:
	case TV_SPIKE:
	case TV_KEY:
	case TV_FLASK:
	case TV_CHEST:
	case TV_FIRESTONE:
	case TV_INSTRUMENT:
	case TV_TOOL:
#ifdef ENABLE_SUBINVEN
	case TV_SUBINVEN:
#endif
		break;

	/* Missiles/ Bows/ Weapons */
	case TV_SHOT:
	case TV_BOLT:
	case TV_ARROW:
	case TV_BOW:
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_DIGGING:
	case TV_BOOMERANG:
	case TV_AXE:
	case TV_MSTAFF:
		show_weapon = TRUE;
		break;

	/* Trapping Kits */
	case TV_TRAPKIT:
		modstr = basenm;
		switched_ego_prefix_and_modstr = TRUE;
		basenm = "& # Trap Kit~";
		break;

		/* Armour */
	case TV_SHIELD:
#ifdef USE_NEW_SHIELDS
		show_shield = TRUE;
		break;
#endif
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_CLOAK:
	case TV_CROWN:
	case TV_HELM:
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_DRAG_ARMOR:
		show_armour = TRUE;
		break;

	case TV_GOLEM:
		break;

	/* Lites (including a few "Specials") */
	case TV_LITE:
		break;

	/* Amulets (including a few "Specials") */
	case TV_AMULET:
		/* "Amulets of Luck" are just called "Talismans" -C. Blue */
		if ((o_ptr->sval == SV_AMULET_LUCK) && aware) {
			if (mode & 512) { /* Specialty: In ~ menu list, show the colour actually, so player knows which flavour is a talisman */
				if (!seed_flavor) modstr = ""; else //hack: Allow object_desc() calls early on in load2.c before flavours are actually initialised
				modstr = amulet_adj[indexx];
				basenm = "& # Talisman~";
				break;
			}
			basenm = "& Talisman~";
			hacked_base_name = TRUE;
			break;
		}
		/* Optionally flavoury */
		if (o_ptr->sval == SV_AMULET_INVINCIBILITY) {
			basenm = "& Administrative Decree~";
			hacked_base_name = TRUE;
			break;
		}

		/* Color the object */
		if (!seed_flavor) modstr = ""; else //hack: Allow object_desc() calls early on in load2.c before flavours are actually initialised
		modstr = amulet_adj[indexx];
		if (aware) append_name = TRUE;
		if (short_item_names) basenm = aware ? "& Amulet~" : "& # Amulet~";
		else basenm = "& # Amulet~";
		break;

	/* Rings (including a few "Specials") */
	case TV_RING:
		/* Color the object */
		if (!seed_flavor) modstr = ""; else //hack: Allow object_desc() calls early on in load2.c before flavours are actually initialised
		modstr = ring_adj[indexx];
		if (aware) append_name = TRUE;
		if (short_item_names) basenm = aware ? "& Ring~" : "& # Ring~";
		else basenm = "& # Ring~";
		break;

	case TV_STAFF:
		/* Color the object */
		if (!seed_flavor) modstr = ""; else //hack: Allow object_desc() calls early on in load2.c before flavours are actually initialised
		modstr = staff_adj[indexx];
		if (aware) append_name = TRUE;
		if (short_item_names) basenm = aware ? "& Staff~" : "& # Staff~";
		else basenm = "& # Staff~";
		break;

	case TV_WAND:
		/* Color the object */
		if (!seed_flavor) modstr = ""; else //hack: Allow object_desc() calls early on in load2.c before flavours are actually initialised
		modstr = wand_adj[indexx];
		if (aware) append_name = TRUE;
		if (short_item_names) basenm = aware ? "& Wand~" : "& # Wand~";
		else basenm = "& # Wand~";
		break;

	case TV_ROD:
		/* Color the object */
		if (!seed_flavor) modstr = ""; else //hack: Allow object_desc() calls early on in load2.c before flavours are actually initialised
		modstr = rod_adj[indexx];
		if (aware) append_name = TRUE;
		if (short_item_names) basenm = aware ? "& Rod~" : "& # Rod~";
		else basenm = "& # Rod~";
		break;

	case TV_ROD_MAIN: {
		object_kind *tip_ptr = &k_info[lookup_kind(TV_ROD, o_ptr->pval)];

		modstr = k_name + tip_ptr->name;
		break;
	}

	case TV_SCROLL:
#ifdef NEW_WILDERNESS_MAP_SCROLLS
		/* For new wilderness mapping code, where it's actually a puzzle piece of the map */
		if (o_ptr->sval == SV_SCROLL_WILDERNESS_MAP) {
			if (mode & 512) { /* Specialty: In ~ menu list, show the colour actually, so player knows which flavour is a talisman */
				if (!seed_flavor) modstr = ""; else //hack: Allow object_desc() calls early on in load2.c before flavours are actually initialised
				modstr = scroll_adj[indexx];
				basenm = "& Wilderness Map Piece~ titled \"#\"";
				break;
			}
			basenm = "& Wilderness Map Piece~";
			hacked_base_name = TRUE;
			break;
		}
#endif

		if (o_ptr->sval == SV_SCROLL_CHEQUE) {
			if (mode & 256)
				basenm = "& Cheque~";
			else
				basenm = "& Cheque~ worth $ Au";
			hacked_base_name = TRUE;
			break;
		}

		/* Color the object */
		if (!seed_flavor) modstr = ""; else //hack: Allow object_desc() calls early on in load2.c before flavours are actually initialised
		modstr = scroll_adj[indexx];
		if (aware) append_name = TRUE;
		if (short_item_names) basenm = aware ? "& Scroll~" : "& Scroll~ titled \"#\"";
		else basenm = aware ? "& Scroll~ \"#\"" : "& Scroll~ titled \"#\"";
		break;

	case TV_POTION:
	case TV_POTION2:
		/* Color the object */
		if (!seed_flavor) modstr = ""; else //hack: Allow object_desc() calls early on in load2.c before flavours are actually initialised
		modstr = potion_adj[indexx + (o_ptr->tval == TV_POTION2 ? STATIC_COLORS : 0)]; /* the first n potions have static flavours */
		if (aware) append_name = TRUE;
		if (short_item_names) basenm = aware ? "& Potion~" : "& # Potion~";
		else basenm = "& # Potion~";
		break;

	case TV_FOOD:
		/* Ordinary food is "boring" */
		if (o_ptr->sval > SV_FOOD_MUSHROOMS_MAX) break;

		/* Color the object */
		if (!seed_flavor) modstr = ""; else //hack: Allow object_desc() calls early on in load2.c before flavours are actually initialised
		modstr = food_adj[indexx];

		if (aware) append_name = TRUE;
		if (short_item_names) basenm = aware ? "& Mushroom~" : "& # Mushroom~";
		else basenm = "& # Mushroom~";
		break;

	case TV_PARCHMENT:
		modstr = basenm;
		switched_ego_prefix_and_modstr = TRUE;
		basenm = "& Parchment~ - #";
		break;

		/* Hack -- Gold/Gems */
	case TV_GOLD:
		strcpy(buf, basenm);
		return;

	case TV_BOOK:
		/* hack for mindcrafter spell scrolls -> spell crystals - C. Blue */
		if (o_ptr->pval >= __lua_M_FIRST && o_ptr->pval <= __lua_M_LAST)
			basenm = "& Spell Crystal~ of #";
		/* hack for priest spell scrolls -> prayer scrolls - C. Blue */
		if (o_ptr->pval >= __lua_P_FIRST && o_ptr->pval <= __lua_P_LAST)
			basenm = "& Prayer Scroll~ of #";
		//basenm = k_name + k_ptr->name;
		if (o_ptr->sval == SV_SPELLBOOK) {
			if (school_spells[o_ptr->pval].name)
				modstr = school_spells[o_ptr->pval].name;
			else modstr = "Unknown spell";
		}
		break;

	case TV_RUNE:
		append_name = TRUE;
		basenm = "& Rune~";
		break;

	case TV_MONSTER:
		break;

#ifdef ENABLE_DEMOLITIONIST
	case TV_CHEMICAL:
		if (o_ptr->sval == SV_MIXTURE) {
			mixture_flavour(o_ptr, tmp_modstr);
			modstr = tmp_modstr;
		}
		break;
	case TV_CHARGE:
		break;
#endif

	/* Used in the "inventory" routine */
	default:
		/* the_sandman: debug line */
		if (o_ptr->tval != 0) s_printf("NOTHING_NOTICED: tval %d, sval %d\n", o_ptr->tval, o_ptr->sval);
		strcpy(buf, "(nothing)");
		return;
	}

	/* Emergency hack for startup logging stuff: Prevent crashes if we're called from load2.c or anywhere else before flavor_init() was actually called:
	    It crashes, as flavor_init() isn't called yet (and cannot be called yet because seed_flavor is loaded _after_ items),
	    and object_desc() will refer to modstr which is NULL pointer for flavoured items w/o flavours.
	   This shouldn't be necessary though as we've already set modstr to "" in flavour-related tvals above: */
	if (modstr == NULL) modstr = "<nf>";

	/* Handle flavoured randart names: Keep our special randart naming ;) 'the slow digestion of..' */
	if (o_ptr->name1 == ART_RANDART && known && !special_rop) {
		//no flavour
		modstr = "";
		append_name = FALSE;
		//no object-kind name prefix
		if (!hacked_base_name) basenm = (k_name + k_ptr->name);
	}
	/* Handle flavoured insta-art names */
	/* Redundant 'if' condition: modstr+unswitched can only be flavoured arts, and flavoured arts are always insta-arts (paranoia) */
	else if (modstr[0] && !switched_ego_prefix_and_modstr && (f3 & TR3_INSTA_ART)) {
		append_name = FALSE;

		/* Hack for insta-arts: Use alternative name from k_info?
		   If the k-name starts on '&' it's actually an alternative base item name,
		   otherwise it's the usual subtype name (eg 'Slow Digestion'). */
		if (*(k_ptr->name + k_name) == '&') {
#if 0 /* Stop displaying the flavour colour once the insta-art is identified? */
			if ((short_item_names || aware) && !(mode & 512)) basenm = k_name + k_ptr->name;
#else
			if ((short_item_names) && !(mode & 512)) basenm = k_name + k_ptr->name;
#endif
			else {
				strcpy(basenm2, "& #");
				strcat(basenm2, k_name + k_ptr->name + 1);
				basenm = basenm2;
			}
		}
	}

	/* Start dumping the result */
	t = buf;

	/* hack: questors have an arbitrary name */
	if (o_ptr->questor) basenm = q_info[o_ptr->quest - 1].questor[o_ptr->questor_idx].name;
	/* hack: quest items have an arbitrary name */
	if (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_QUEST
	    && o_ptr->xtra4) /* <- extra check, in case a silyl admin tries to wish for one ;) */
		basenm = q_info[o_ptr->quest - 1].stage[o_ptr->quest_stage].qitem[o_ptr->xtra3].name; // o_O

	/* The object "expects" a "number" */
	if (basenm[0] == '&') {
		cptr ego = NULL;

		/* Grab any ego-item name */
		//if ((o_ptr->name2 || o_ptr->name2b) && (o_ptr->tval != TV_ROD_MAIN))
		if (known && (o_ptr->name2 || o_ptr->name2b) && (o_ptr->tval != TV_ROD_MAIN) &&
		    !(mode & 32)) {
			ego_item_type *e_ptr = &e_info[o_ptr->name2];
			ego_item_type *e2_ptr = &e_info[o_ptr->name2b];

			if (e_ptr->before) ego = e_ptr->name + e_name;
#if 1
			else if (e2_ptr->before) ego = e2_ptr->name + e_name;
#endif
		}

		/* Skip the ampersand (and space) */
		s_ptr = basenm + 2;

		/* No prefix */
		if (!pref) {
			/* Nothing */
		}

		/* Hack -- None left */
		else if (o_ptr->number <= 0)
			t = object_desc_str(t, "no more ");

		/* Extract the number */
		else if (o_ptr->number > 1) {
			t = object_desc_num(t, o_ptr->number);
			t = object_desc_chr(t, ' ');
		}

		/* Hack -- Omit an article */
		else if (mode & 8) {
			/* Naught */
		}

		/* Hack -- The only one of its kind */
		else if (known && artifact_p(o_ptr) && !(mode & 2048)) {
			/* hack: some base item types in k_info start
			   on 'the', so prevent a second 'The ' for those */
			if (!(mode & 128)) t = object_desc_str(t, "The ");
			if (strstr(k_name + k_ptr->name, "the ") == k_name + k_ptr->name) {
				s_ptr += 4;
				skip_base_article = TRUE;
			}
		}

		/* Trap kits hackily exchange the order usage of 'modstr' vs prefix ego */
		else if (switched_ego_prefix_and_modstr && ego != NULL) {
			if (is_a_vowel(ego[0])) t = object_desc_str(t, "an ");
			else t = object_desc_str(t, "a ");
		}

		/* A single one, with a vowel in the modifier */
		else if ((*s_ptr == '#') && is_a_vowel(modstr[0])) {
			t = object_desc_str(t, "an ");
		}

		else if (ego != NULL) {
			if (is_a_vowel(ego[0])) t = object_desc_str(t, "an ");
			else t = object_desc_str(t, "a ");
		}

		/* A single one, with a vowel */
		else if (is_a_vowel(*s_ptr)) t = object_desc_str(t, "an ");

		/* A single one, without a vowel */
		else t = object_desc_str(t, "a ");
	}

	/* Hack -- objects that "never" take an article */
	else {
		/* No ampersand */
		s_ptr = basenm;

		/* No pref */
		if (!pref) {
			/* Nothing */
		}

		/* Hack -- all gone */
		else if (o_ptr->number <= 0)
			t = object_desc_str(t, "no more ");

		/* Prefix a number if required */
		else if (o_ptr->number > 1) {
			t = object_desc_num(t, o_ptr->number);
			t = object_desc_chr(t, ' ');
		}

		/* Hack -- The only one of its kind */
		else if (known && artifact_p(o_ptr) && !(mode & 8) && !(mode & 2048)) {
			/* hack: some base item types in k_info start
			   on 'the', so prevent a second 'The ' for those */
			if (!(mode & 128)) t = object_desc_str(t, "The ");
			if (strstr(k_name + k_ptr->name, "the ") == k_name + k_ptr->name) {
				s_ptr += 4;
				skip_base_article = TRUE;
			}
		}

		/* Hack -- single items get no prefix */
		else {
			/* Nothing */
		}
	}

#ifdef ROYAL_ARMOUR_SHORTEN_NAME
	/* Special: Shorten 'heavy' for heavy armour, if we already omit 'the' or anything */
	if (known
 #if 0
	    && (mode & 8) /* Only if we're already shortening the name anyway, leaving out the item's article, due to length constraints? */
 #endif
 #ifdef ROYAL_ARMOUR_SHORTEN_ANTIRIAD_ONLY
	    && (o_ptr->name1 == ART_ANTIRIAD || o_ptr->name1 == ART_ANTIRIAD_DEPLETED) //for now only for Antiriad?
 #endif
 #ifndef ROYAL_ARMOUR_SHORTEN_ANTIRIAD_DEPLETED
	    && o_ptr->name1 != ART_ANTIRIAD_DEPLETED //not for the depleted version actually?
 #endif
	    ) {
		if (strstr(k_name + k_ptr->name, "Heavy Adam") == k_name + k_ptr->name) {
			s_ptr += 6 + 11; //'heavy ' + 'adamantite '
			t = object_desc_str(t, "H.Adamant.");
		}
		else if (strstr(k_name + k_ptr->name, "Heavy Ribbed Adam") == k_name + k_ptr->name) {
			s_ptr += 6 + 7 + 11; //'heavy ' + 'ribbed ' + 'adamantite '
			t = object_desc_str(t, "H.Ribbed Adamant.");
		}
		else if (strstr(k_name + k_ptr->name, "Heavy Mith") == k_name + k_ptr->name) {
			s_ptr += 6; // 'heavy '
			t = object_desc_str(t, "H.");
		}
		else if (strstr(k_name + k_ptr->name, "Heavy Ribbed Mith") == k_name + k_ptr->name) {
			s_ptr += 6; // 'heavy '
			t = object_desc_str(t, "H.");
		}
	}
#endif

	/* Grab any ego-item name */
	//if ((o_ptr->name2 || o_ptr->name2b) && (o_ptr->tval != TV_ROD_MAIN))
	if (known && (o_ptr->name2 || o_ptr->name2b) && (o_ptr->tval != TV_ROD_MAIN) &&
	    !(mode & 32)
	    //&& o_ptr->tval != TV_SPECIAL
	    ) {
		ego_item_type *e_ptr = &e_info[o_ptr->name2];
		ego_item_type *e2_ptr = &e_info[o_ptr->name2b];

		if (e_ptr->before) {
			t = object_desc_str(t, (e_name + e_ptr->name));
			t = object_desc_chr(t, ' ');
		}
#if 1
		if (e2_ptr->before) {
			t = object_desc_str(t, (e_name + e2_ptr->name));
			t = object_desc_chr(t, ' ');
		}
#endif	// 0
	}

	/* Paranoia -- skip illegal tildes */
	/* while (*s_ptr == '~') s_ptr++; */

	/* Copy the string */
	for (; *s_ptr; s_ptr++) {
		/* Pluralizer */
		if (*s_ptr == '~') {
			/* Hack -- The only one of its kind - 'No more Tunic of the Wind' instead of 'Tunics' */
			if (known && artifact_p(o_ptr) && !(mode & 2048)) continue;

			/* Add a plural if needed */
			if (o_ptr->number != 1 && t >= buf + 2) { //guard vs buffer underflow
				char k = t[-1], k2 = t[-2];

				/* XXX XXX XXX Mega-Hack */

				/* Hack -- "Torch-es" and theoretically "Topaz-es" */
				if ((k == 'h') || (k == 'z')) *t++ = 'e';

				/* Hack -- Finally, staffs become staves.
				   Note: Might require fix for future word additions ;) */
				else if (k == 'f') {
					if (k2 == 'f') t -= 2;
					else t--;
					*t++ = 'v';
					*t++ = 'e';
				}

				/* Hack -- "Cod/ex -> Cod/ices" */
				else if (k == 'x') {
					if (k2 == 'e') {
						t -= 2;
						*t++ = 'i';
						*t++ = 'c';
						*t++ = 'e';
					} else {
					    /* and "Suffix" -> "Suffixes", theoretically.. >_> */
					    *t++ = 'e';
					}
				}

				/* Hack -- "Aeg/is -> Aeg/ides" */
				else if (k == 's') {
					if (k2 == 'i') {
						t -= 2;
						*t++ = 'i';
						*t++ = 'd';
						*t++ = 'e';
					}
					/* Cutlass-es */
					else *t++ = 'e';
				}

				/* Add an 's' */
				*t++ = 's';
			}
		}
		/* Modifier */
		else if (*s_ptr == '#') {
			cptr tmp_cptr;

			/* Grab any ego-item name */
			if (o_ptr->tval == TV_ROD_MAIN) {
				t = object_desc_chr(t, ' ');

				if (known && o_ptr->name2) {
					ego_item_type *e_ptr = &e_info[o_ptr->name2];

					t = object_desc_str(t, (e_name + e_ptr->name));
				}
			}

			/* Insert the modifier */
			for (tmp_cptr = modstr; *tmp_cptr; tmp_cptr++) *t++ = *tmp_cptr;
		}
		/* Cheque value */
		else if (*s_ptr == '$') {
			sprintf(tmp_val, "%d", ps_get_cheque_value(o_ptr));
			t = object_desc_str(t, tmp_val);
		}
		/* Normal */
		else {
			/* Copy */
			*t++ = *s_ptr;
		}
	}

	/* Terminate */
	*t = '\0';

	/* Append the "kind name" to the "base name" */
	if (append_name) {
		t = object_desc_str(t, !(mode & 8) ? " of " : " ");

		if (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_FIREWORK) {
			if (!(mode & 256)) {
				switch (o_ptr->xtra1) {
				case 0: t = object_desc_str(t, "Small "); break;
				case 1: t = object_desc_str(t, "Medium "); break;
				case 2: t = object_desc_str(t, "Big "); break;
				}
				switch (o_ptr->xtra2) { //0..FIREWORK_COLOURS-1
				case 0: t = object_desc_str(t, "Red"); break;
				case 1: t = object_desc_str(t, "Blue"); break;
				case 2: t = object_desc_str(t, "Green"); break;
				case 3: t = object_desc_str(t, "Golden"); break;
				case 4: t = object_desc_str(t, "Shining"); break;
				case 5: t = object_desc_str(t, "Purple"); break;
				case 6: t = object_desc_str(t, "Mixed"); break;
				}
				t = object_desc_str(t, " Firework");
			} else t = object_desc_str(t, "Firework");
		} else {
			if (!skip_base_article)
				t = object_desc_str(t, (k_name + k_ptr->name));
			else
				t = object_desc_str(t, (k_name + k_ptr->name + 4));
		}
	}

	if (o_ptr->tval == TV_KEY && o_ptr->sval == SV_GUILD_KEY &&
	    guilds[o_ptr->pval].members && !(mode & 256)) {
		t = object_desc_str(t, " of ");
		t = object_desc_chr(t, '\'');
		t = object_desc_str(t, guilds[o_ptr->pval].name);
		t = object_desc_chr(t, '\'');
	}


	/* Hack -- Append "Artifact" or "Special" (#-inscription) names; also handle raw-name-only */
	if (known) {
		if (!(mode & 2048)) {
			/* Grab any ego-item name */
			//if ((o_ptr->name2 || o_ptr->name2b) && (o_ptr->tval != TV_ROD_MAIN))
			if ((o_ptr->name2 || o_ptr->name2b) && (o_ptr->tval != TV_ROD_MAIN) &&
			    !(mode & 32)
			    //&& o_ptr->tval != TV_SPECIAL
			    ) {
				ego_item_type *e_ptr = &e_info[o_ptr->name2];
				ego_item_type *e2_ptr = &e_info[o_ptr->name2b];

				if (!e_ptr->before && o_ptr->name2) {
					if (strlen(e_name + e_ptr->name)) t = object_desc_chr(t, ' ');
					t = object_desc_str(t, (e_name + e_ptr->name));

				}
#if 1
				else if (!e2_ptr->before && o_ptr->name2b) {
					if (strlen(e_name + e2_ptr->name)) t = object_desc_chr(t, ' ');
					t = object_desc_str(t, (e_name + e2_ptr->name));
					//ego = e2_ptr->name + e_name;
				}
#endif	// 0

			}

			/* Grab any randart name */
			if (o_ptr->name1 == ART_RANDART) {
				t = object_desc_chr(t, ' ');

				if (special_rop) {
					monster_race *r_ptr = &r_info[o_ptr->bpval];
					t = object_desc_str(t, "of ");
					if (!(r_ptr->flags7 & RF7_NAZGUL))
						t = object_desc_str(t, "bug");
					else
						t = object_desc_str(t, r_name + r_ptr->name);
				} else {
					/* Create the name */
					randart_name(o_ptr, tmp_val, NULL);
					t = object_desc_str(t, tmp_val);
				}
			}

			/* Grab any artifact name */
			else if (o_ptr->name1) {
				artifact_type *a_ptr = &a_info[o_ptr->name1];

				t = object_desc_chr(t, ' ');
				t = object_desc_str(t, (a_name + a_ptr->name));
			}
		}

		/* raw name only? */
		if ((mode & 256)) return;

		/* -TM- Hack -- Add false-artifact names */
		/* Dagger inscribed {@w0#of Smell} will be named
		 * Dagger of Smell {@w0} */
		if (o_ptr->note &&
		    /* Not for custom objects! As their inscription is actually their item name! */
		    !(o_ptr->tval == TV_SPECIAL &&
		     o_ptr->sval == SV_CUSTOM_OBJECT)) {
			cptr str = strchr(quark_str(o_ptr->note), '#');

			/* Add the false name */
			if (str) {
				/* the_sandman: lets omit the space so we
				   can make cool names like
				   'Cloak'->'Cloaking Device' =)

				t = object_desc_chr(t, ' ');
				*/

				/* auto-inscription hack */
				if ((mode & 7) == 4) {
					t = object_desc_chr(t, '\373');
					t = object_desc_chr(t, strlen(str + 1) + 1); /* in case len is 0, avoid char \0 */
				}

				t = object_desc_str(t, &str[1]);
			}
		}
	}

	/* raw name only? */
	if ((mode & 256)) return;

	/* Quest item (retrieval item, not questor) */
	if (o_ptr->quest && !o_ptr->questor) t = object_desc_str(t, " (Q)");

	/* Sigil */
	if (o_ptr->sigil) {
		/* Older clients cannot unhack the colour code from character dumps, making the equipment look bad */
		if (!Ind || is_newer_than(&p_ptr->version, 4, 6, 1, 2, 0, 0))
			t = object_desc_str(t, format(" <\377%s&\377.>",
				string_exec_lua(0, format("return rcraft_code(%d)", o_ptr->sigil))
			));
		else
			t = object_desc_str(t, " <&>");
	}

	/* Print level and owning */
	t = object_desc_chr(t, ' ');
	t = object_desc_chr(t, '{');
	if (o_ptr->owner) {
		cptr name = lookup_player_name(o_ptr->owner);

		if (name != NULL) {
			if (Ind && !strcmp(name, p_ptr->name))
				t = object_desc_chr(t, '+');
			else {
				if (mode & 16) t = object_desc_chr(t, '*');
				else t = object_desc_str(t, name);
			}
		} else t = object_desc_chr(t, '-');
		t = object_desc_chr(t, ',');
	}

	/* you cannot tell the level if not id-ed. */
	/* XXX it's still abusable */
	if (!known) t = object_desc_chr(t, '?');
	else t = object_desc_num(t, o_ptr->level);
	t = object_desc_chr(t, '}');


	/* No more details wanted */
	if ((mode & 7) < 1) {
		if (t - buf <= 65 || (mode & 8)) {
			return;
		} else {
			object_desc(Ind, buf, o_ptr, pref, mode | 8);
			return;
		}
	}


	/* Hack -- Chests must be described in detail */
	if (o_ptr->tval == TV_CHEST) {
		/* May be "empty" - cannot happen with SUBINVEN_CHESTS as chests will be immediately converted to TV_SUBINVEN on emptying, except for Ruined Chests. */
		if (!o_ptr->pval) t = object_desc_str(t, " (empty)");
		/* Not searched yet - even though we perhaps didn't know the chest yet, we could still at least see whether it was empty or not (above). Important for Ruined Chests too. */
		else if (!known) {
			/* Nothing */
#ifdef SUBINVEN_CHESTS
			t = object_desc_str(t, " (shut tight)"); /* To visually distinguish this lootable chest from subinven-chests in our inventory */
#endif
		}
		/* May be "disarmed" */
		else if (o_ptr->pval < 0) t = object_desc_str(t, " (disarmed)");
		/* Describe the traps, if any */
		else if (o_ptr->pval) {
			/* Describe the traps */
			t = object_desc_str(t, " (");
			if (Ind) {
				if (p_ptr->trap_ident[o_ptr->pval])
					t = object_desc_str(t, t_name + t_info[o_ptr->pval].name);
				else
					t = object_desc_str(t, "trapped");
			} else {
					t = object_desc_str(t, t_name + t_info[o_ptr->pval].name);
			}
			t = object_desc_str(t, ")");
		}
#ifdef SUBINVEN_CHESTS
		else t = object_desc_str(t, " (locked)");/* To visually distinguish this lootable chest from subinven-chests in our inventory */
	}
#endif
#if 0 /* doesn't work atm, because we cannot know if we're inside the player inventory or on the floor/inside a house! So for now we just inscribe a chest 'empty' on opening it (empty-chest-hack). */
#ifdef SUBINVEN_CHESTS
	else if (o_ptr->tval == TV_SUBINVEN && o_ptr->sval >= SV_SI_GROUP_CHEST_MIN && o_ptr->sval <= SV_SI_GROUP_CHEST_MAX) {
		/* Attach "empty" to bag-chests, unless player stored an object or more in them */
		if (!p_ptr->subinventory[slot][0].k_idx) {
			t = object_desc_str(t, " (empty)");
		}
	}
#endif
#endif

	/* Display the item like a weapon */
	if (f3 & TR3_SHOW_MODS) show_weapon = TRUE;

	/* Display the item like a weapon */
	if (o_ptr->to_h && o_ptr->to_d) show_weapon = TRUE;

	/* Display the item like armour */
	if (o_ptr->ac) show_armour = TRUE;

#ifdef USE_NEW_SHIELDS
	if (o_ptr->tval == TV_SHIELD) {
		show_armour = FALSE;
		show_shield = TRUE;
	}
#endif

	if (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT && o_ptr->xtra3 & 0x0200) {
		tval = o_ptr->tval2;
		invcopy(ox_ptr, lookup_kind(o_ptr->tval2, o_ptr->sval2));

		/* Hack: Actually permanently *(re)transfer* the dice and base AC onto the custom item! */
		o_ptr->dd = ox_ptr->dd;
		o_ptr->ds = ox_ptr->ds;
		o_ptr->ac = ox_ptr->ac;
	} else tval = o_ptr->tval;

	/* Dump base weapon info */
	switch (tval) {
	/* Missiles and Weapons */
	case TV_SHOT:
	case TV_BOLT:
	case TV_ARROW:
		/* Exploding arrow? */
		if (o_ptr->pval != 0 && known) t = object_desc_str(t, " (exploding)");
		/* No break, we want to continue the description */
		__attribute__ ((fallthrough));

	case TV_BLUNT:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_DIGGING:
	case TV_BOOMERANG:
	case TV_AXE:
	case TV_MSTAFF:

		/* Append a "damage" string */
		if (!(mode & 8)) t = object_desc_chr(t, ' ');
		t = object_desc_chr(t, p1);
		t = object_desc_num(t, o_ptr->dd);
		t = object_desc_chr(t, 'd');
		t = object_desc_num(t, o_ptr->ds);
		t = object_desc_chr(t, p2);

		/* All done */
		break;


	/* Bows get a special "damage string" */
	case TV_BOW:

		/* Mega-Hack -- Extract the "base power" */
		//power = (sval % 10);
		power = get_shooter_mult(o_ptr);

		/* Apply the "Extra Might" flag */
		if (f3 & TR3_XTRA_MIGHT) power++;

		/* Append a special "damage" string */
		if (!(mode & 8)) t = object_desc_chr(t, ' ');
		t = object_desc_chr(t, p1);
		t = object_desc_chr(t, 'x');
		t = object_desc_num(t, power);
		t = object_desc_chr(t, p2);

		/* All done */
		break;
	}

	/* Add base item +hit,+dam,ac,+ac */
	if (!(mode & 2048)) {
		/* Add the weapon boni */
		if (known) {
			/* Show the tohit/todam on request */
			if (show_weapon) {
				if (!(mode & 8)) t = object_desc_chr(t, ' ');
				t = object_desc_chr(t, p1);
				t = object_desc_int(t, o_ptr->to_h);
				t = object_desc_chr(t, ',');
				t = object_desc_int(t, o_ptr->to_d);
				t = object_desc_chr(t, p2);
			}
			/* Show the tohit if needed */
			else if (o_ptr->to_h) {
				if (!(mode & 8)) t = object_desc_chr(t, ' ');
				t = object_desc_chr(t, p1);
				t = object_desc_int(t, o_ptr->to_h);
				t = object_desc_chr(t, p2);
			}
			/* Show the todam if needed */
			else if (o_ptr->to_d) {
				if (!(mode & 8)) t = object_desc_chr(t, ' ');
				t = object_desc_chr(t, p1);
				t = object_desc_int(t, o_ptr->to_d);
				t = object_desc_chr(t, p2);
			}
		}

		/* Add the armor boni */
		if (known) {
			/* Show the armor class info */
			if (show_armour) {
				if (!(mode & 8)) t = object_desc_chr(t, ' ');
				t = object_desc_chr(t, b1);
				t = object_desc_num(t, o_ptr->ac);
				t = object_desc_chr(t, ',');
				t = object_desc_int(t, o_ptr->to_a);
				t = object_desc_chr(t, b2);
			} else if (show_shield) {
				if (!(mode & 8)) t = object_desc_chr(t, ' ');
#ifndef NEW_SHIELDS_NO_AC
				t = object_desc_chr(t, b1);
				t = object_desc_per(t, o_ptr->ac);
				t = object_desc_chr(t, ',');
				t = object_desc_int(t, o_ptr->to_a);
				t = object_desc_chr(t, b2);
#else
				t = object_desc_chr(t, b1);
				t = object_desc_per(t, o_ptr->ac);
				t = object_desc_chr(t, b2);
#endif
			/* No base armor, but does increase armor */
			} else if (o_ptr->to_a) {
#ifdef WEAPONS_NO_AC
				if (is_melee_weapon(o_ptr->tval)) {
					/* Weapons give to parry chance instead? */
					if (!(mode & 8)) t = object_desc_chr(t, ' ');
					t = object_desc_chr(t, b1);
					t = object_desc_intper(t, (o_ptr->to_a * 10 + WEAPONS_NO_AC - 10) / WEAPONS_NO_AC);
					t = object_desc_chr(t, b2);
				} else {
#endif
					if (!(mode & 8)) t = object_desc_chr(t, ' ');
					t = object_desc_chr(t, b1);
					t = object_desc_int(t, o_ptr->to_a);
					t = object_desc_chr(t, b2);
#ifdef WEAPONS_NO_AC
				}
#endif
			}
		}
		/* Hack -- always show base armor */
		else if (show_armour) {
			if (!(mode & 8)) t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, b1);
			t = object_desc_num(t, o_ptr->ac);
			t = object_desc_chr(t, b2);
		} else if (show_shield) {
			if (!(mode & 8)) t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, b1);
			t = object_desc_per(t, o_ptr->ac);
			t = object_desc_chr(t, b2);
		}
	}


	/* No more details wanted */
	if ((mode & 7) < 2) {
		if (t - buf <= 65 || (mode & 8)) return;
		else {
			object_desc(Ind, buf, o_ptr, pref, mode | 8);
			return;
		}
	}

	if (!(mode & 2048)) {
		/* Hack -- Wands and Staffs have charges */
		if (known &&
		    ((o_ptr->tval == TV_STAFF) ||
		     (o_ptr->tval == TV_WAND))) {
			/* Dump " (N charges)" */
			if (!(mode & 8)) t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, p1);
			t = object_desc_num(t, o_ptr->pval);
			if (!(mode & 8)) {
				t = object_desc_str(t, " charge");
				if (o_ptr->pval != 1) t = object_desc_chr(t, 's');
				if (o_ptr->tval == TV_WAND &&
				    o_ptr->number > 1 &&
				    !(mode & 64))
					t = object_desc_str(t, " total");
			}
			t = object_desc_chr(t, p2);
		}

		/* Hack -- Rods have a "charging" indicator */
		else if (known && (o_ptr->tval == TV_ROD)) {
			/* Hack -- Dump " (charging)" if relevant */
#ifndef NEW_MDEV_STACKING
			if (o_ptr->pval) t = object_desc_str(t, !(mode & 8) ? " (charging)" : "(#)");
#else
			if (o_ptr->bpval == o_ptr->number
			    && o_ptr->number) /* <- special case: 'You have no more rods..' */
				t = object_desc_str(t, !(mode & 8) ? " (charging)" : "(#)");
 #if 0
			else if (o_ptr->pval) t = object_desc_str(t, !(mode & 8) ? " (partially charging)" : "(~)");
 #else
			else if (o_ptr->pval) {
				if (mode & 8) {
					t = object_desc_str(t, "(");
					t = object_desc_num(t, o_ptr->bpval);
					t = object_desc_str(t, "~)");
				} else {
					t = object_desc_str(t, " (");
					t = object_desc_num(t, o_ptr->bpval);
					t = object_desc_str(t, " charging)");
				}
			}
 #endif
#endif
		}

		/* Hack -- Process Lanterns/Torches */
		//else if ((o_ptr->tval == TV_LITE) && (o_ptr->sval < SV_LITE_DWARVEN) && (!o_ptr->name3))
		else if ((o_ptr->tval == TV_LITE) && (f4 & TR4_FUEL_LITE)) {
			/* Hack -- Turns of light for normal lites */
			t = object_desc_str(t, !(mode & 8) ? " (with " : "(");
			t = object_desc_num(t, o_ptr->timeout);
			t = object_desc_str(t, !(mode & 8) ? " turns of light)" : "t)");
		}
	}

	if (!(mode & 32)) {
		/* Dump "pval" flags for wearable items */
		if (known && (((f1 & (TR1_PVAL_MASK)) || (f5 & (TR5_PVAL_MASK)))
		    || o_ptr->tval == TV_GOLEM || o_ptr->tval == TV_TRAPKIT)) {
			/* Hack -- first display any base pval bonuses. */
			if (o_ptr->bpval && !special_rop) {
				if (!(mode & 8)) t = object_desc_chr(t, ' ');
				t = object_desc_chr(t, p1);
				/* Dump the "pval" itself */
				t = object_desc_int(t, o_ptr->bpval);
#if 1
				/* hack (originally just for plain talismans, since it doesn't say "amulet of luck"
				   to give a hint to newbies what they do) to display a basic bpval property: */
				if (!o_ptr->pval && !(f3 & TR3_HIDE_TYPE)) {
					if (k_ptr->flags1 & TR1_LIFE) t = object_desc_str(t, !(mode & 8) ? " to life" : "life");
					else if (k_ptr->flags1 & TR1_SPEED) t = object_desc_str(t, !(mode & 8) ? " to speed" : "spd");
					else if (k_ptr->flags1 & TR1_BLOWS) {
						t = object_desc_str(t, !(mode & 8) ? " attack" : "at");
						if (ABS(o_ptr->bpval) != 1 && !(mode & 8)) t = object_desc_chr(t, 's');
					} else if (k_ptr->flags5 & (TR5_CRIT)) t = object_desc_str(t, !(mode & 8) ? " critical hits" : "crt");
					else if (k_ptr->flags1 & TR1_STEALTH) t = object_desc_str(t, !(mode & 8) ? " to stealth" : "stl");
					else if (k_ptr->flags1 & TR1_SEARCH) t = object_desc_str(t, !(mode & 8) ? " to searching" : "srch");
					else if (k_ptr->flags1 & TR1_INFRA) t = object_desc_str(t, !(mode & 8) ? " to infra-vision" : "infr");
					else if (k_ptr->flags5 & TR5_LUCK) t = object_desc_str(t, !(mode & 8) ? " to luck" : "luck");
					else if (k_ptr->flags1 & TR1_TUNNEL) {}
				}
#endif
				/* finish with closing bracket ')' */
				t = object_desc_chr(t, p2);
			}
#ifdef ART_WITAN_STEALTH
			else if (o_ptr->name1 && o_ptr->tval == TV_BOOTS && o_ptr->sval == SV_PAIR_OF_WITAN_BOOTS) {
				if (!(mode & 8)) t = object_desc_chr(t, ' ');
				t = object_desc_str(t, "(-2)");
			}
#endif
			/* Next, display any pval bonuses. */
			if (o_ptr->pval) {
				artifact_type *a_ptr;

				/* Start the display */
				if (!(mode & 8)) t = object_desc_chr(t, ' ');
				t = object_desc_chr(t, p1);

				/* Dump the "pval" itself */
				t = object_desc_int(t, o_ptr->pval);

				/* Don't confuse ego flags with kind flags.. */
				if (o_ptr->name2) {
					a_ptr = ego_make(o_ptr);
					f1 &= ~(k_ptr->flags1 & TR1_PVAL_MASK & ~a_ptr->flags1);
					f5 &= ~(k_ptr->flags5 & TR5_PVAL_MASK & ~a_ptr->flags5);
				}

				/* Do not display the "pval" flags */
				if ((f3 & TR3_HIDE_TYPE) || o_ptr->name1) {
					/* Nothing */
				}
				/* Life */
				else if (f1 & TR1_LIFE) {
					t = object_desc_str(t, !(mode & 8) ? " to life" : "life");
				}
				/* Speed */
				else if (f1 & TR1_SPEED) {
					t = object_desc_str(t, !(mode & 8) ? " to speed" : "spd");
				}
				/* Attack speed */
				else if (f1 & TR1_BLOWS) {
					t = object_desc_str(t, !(mode & 8) ? " attack" : "at");
					if (ABS(o_ptr->pval) != 1 && !(mode & 8)) t = object_desc_chr(t, 's');
				}
				/* Critical chance */
				else if (f5 & (TR5_CRIT)) {
					t = object_desc_str(t, !(mode & 8) ? " critical hits" : "crt");
				}
				/* Stealth */
				else if (f1 & TR1_STEALTH) {
					t = object_desc_str(t, !(mode & 8) ? " to stealth" : "stl");
				}
				/* Search */
				else if (f1 & TR1_SEARCH) {
					t = object_desc_str(t, !(mode & 8) ? " to searching" : "srch");
				}
				/* Infravision */
				else if (f1 & TR1_INFRA) {
					t = object_desc_str(t, !(mode & 8) ? " to infra-vision" : "infr");
				}
				else if (f5 & TR5_LUCK) {
					t = object_desc_str(t, !(mode & 8) ? " to luck" : "luck");
				}
				/* Tunneling */
				else if (f1 & TR1_TUNNEL) {
					/* Nothing */
				}

				/* Finish the display */
				t = object_desc_chr(t, p2);
			}
		}
	} /* +32 mode */

	/* Special case, ugly, but needed */
	if (known && aware && (o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH)) {
		t = object_desc_str(t, !(mode & 8) ? " of " : "-");
		t = object_desc_str(t, r_info[o_ptr->pval].name + r_name);

		/* Polymorph rings that run out.. */
		t = object_desc_str(t, !(mode & 8) ? " (" : "(");
		t = object_desc_num(t, o_ptr->timeout_magic);
		t = object_desc_str(t, !(mode & 8) ? " turns of energy)" : "t)");
	}

	/* Costumes */
	if (known && aware && (o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_COSTUME)) {
		t = object_desc_str(t, !(mode & 8) ? " of " : "-");
		t = object_desc_str(t, r_info[o_ptr->bpval].name + r_name);
	}

	if (known && o_ptr->timeout) {
		/* Items going bad */
		if (o_ptr->tval == TV_POTION || o_ptr->tval == TV_FOOD) {
			t = object_desc_str(t, !(mode & 8) ? " (" : "(");
			t = object_desc_num(t, o_ptr->timeout);
			t = object_desc_str(t, !(mode & 8) ? " turns of shelf life)" : "t)");
		}
	}
	if (known && o_ptr->recharging) {
		/* Hack -- Dump " (charging)" if relevant */
		t = object_desc_str(t, !(mode & 8) ? " (charging)" : "(#)");
	}

	/* Special case: Sacred Armour of Antiriad */
	if (known && (o_ptr->name1 == ART_ANTIRIAD || o_ptr->name1 == ART_ANTIRIAD_DEPLETED)) {
		t = object_desc_str(t, !(mode & 8) ? " (" : "(");
#if 0
		t = object_desc_lnum(t, o_ptr->timeout);
		t = object_desc_str(t, !(mode & 8) ? " turns of energy)" : "t)"); /* 1 turn is ~ 1/2 s real-time */
#else
		t = object_desc_num(t, o_ptr->timeout);
		t = object_desc_str(t, !(mode & 8) ? " hours of energy)" : "h)"); /* 1 real-time hour is ~ 2 turns, but in-game time runs at 20x speed, so 2 * 3600 / 20 = 360 turns of energy are 1 (in-game) hour of energy */
#endif
	}

	/* No more details wanted */
	if ((mode & 7) < 3) {
		if (t - buf <= 65 || (mode & 8)) {
			return;
		} else {
			object_desc(Ind, buf, o_ptr, pref, mode + 8);
			return;
		}
	}


	/* No inscription yet */
	tmp_val[0] = '\0';

	/* Use the standard inscription if available */
	if (o_ptr->note && !(o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT)) {
		char *tmp_ptr = tmp_val;

		strcpy(tmp_val, quark_str(o_ptr->note));
		for (; *tmp_ptr && (*tmp_ptr != '#'); tmp_ptr++);
		*tmp_ptr = '\0';
	}

	/* Note "cursed" if the item is known to be cursed */
	else if (cursed_p(o_ptr) && (known || (o_ptr->ident & ID_SENSE)))
		strcpy(tmp_val, "cursed");
	/* Mega-Hack -- note empty wands/staffs */
	else if (!known && (o_ptr->ident & ID_EMPTY))
		strcpy(tmp_val, "empty");
	/* Note "tried" if the object has been tested unsuccessfully */
	else if (!aware && (Ind && object_tried_p(Ind, o_ptr)))
		strcpy(tmp_val, "tried");
	/* Note 100% discount and write it a bit special */
	else if (o_ptr->discount == 100 && !(mode & 2048)) {
		strcat(tmp_val, "unsalable");
	}
	/* Note the discount, if any */
	else if (o_ptr->discount && !(mode & 2048)) {
		object_desc_num(tmp_val, o_ptr->discount);
		strcat(tmp_val, "% off");
	}

	/* Append the inscription, if any */
	if (tmp_val[0]) {
		int n;

		/* Hack -- How much so far */
		n = (t - buf);

		/* Hack -- shrink too long inscriptions; 3 additional chars used for ' ', '{', '}' -> -1-3 */
		if (n >= ONAME_LEN - 4) n = ONAME_LEN - 4;
		tmp_val[(ONAME_LEN - 4) - n] = '\0';

		/* Append the inscription */
		if (!(mode & 8)) t = object_desc_chr(t, ' ');
		t = object_desc_chr(t, c1);
		t = object_desc_str(t, tmp_val);
		t = object_desc_chr(t, c2);
	}

	/* This should always be true, but still.. */
	if ((mode & 7) < 4) {
		if (t - buf <= 65 || (mode >= 8)) {
			return;
		} else {
			object_desc(Ind, buf, o_ptr, pref, mode + 8);
			return;
		}
	}
}


/*
 * Hack -- describe an item currently in a store's inventory
 * This allows an item to *look* like the player is "aware" of it
 */
void object_desc_store(int Ind, char *buf, object_type *o_ptr, int pref, int mode) {
	player_type *p_ptr = Players[Ind];

	bool hack_aware = FALSE;
	bool hack_known = FALSE;

	/* Only save flags if we have a valid player */
	if (Ind) {
		/* Save the "aware" flag */
		hack_aware = p_ptr->obj_aware[o_ptr->k_idx];

		/* Force "aware" for description */
		p_ptr->obj_aware[o_ptr->k_idx] = TRUE;
	}

	/* Save the "known" flag */
	hack_known = (o_ptr->ident & ID_KNOWN) ? TRUE : FALSE;

	/* Force "known" flag for description */
	o_ptr->ident |= ID_KNOWN;

	/* Describe the object */
	object_desc(Ind, buf, o_ptr, pref, mode);

	/* Only restore flags if we have a valid player */
	if (Ind) {
		/* Restore "aware" flag */
		p_ptr->obj_aware[o_ptr->k_idx] = hack_aware;
	}
	/* Restore the known flag */
	if (!hack_known) o_ptr->ident &= ~ID_KNOWN;
}




/*
 * Determine the "Activation" (if any) for an artifact
 * Return a string, or NULL for "no activation"
 */
cptr item_activation(object_type *o_ptr) {
	u32b f1, f2, f3, f4, f5, f6, esp;

	/* Needed hacks */
	//static char rspell[2][80];

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	/* Require activation ability */
	if (!(f3 & TR3_ACTIVATE)) return(NULL);

	// -------------------- artifacts -------------------- //

	/* Some artifacts can be activated */
	switch (o_ptr->name1) {
	case ART_EOL:
		return("a mana bolt (9..29d8) 2..7+d3 turns");
	case ART_SKULLCLEAVER:
		return("destruction every 100..200+d200 turns");
	case ART_HARADRIM:
		return("berserk strength every 15..50+d40 turns");
	case ART_FUNDIN:
		return("dispelling evil (lev x8 +0..400) every 20..100+d50 turns");
	case ART_NAIN:
		return("stone-to-mud every 4..7+d5 turns");
	case ART_CELEBRIMBOR:
		return("temporary ESP (dur 20+d20) every 25..50+d25 turns");
	case ART_UMBAR:
		return("a magic arrow (10..30d10) every 5..20+d10 turns");
	case ART_GILGALAD:
		return("starlight (75..225) every 15..75+d50 turns");
	case ART_HIMRING:
		return("protection from evil (dur 30+d15) every 75..225+d125 turns");
	case ART_NUMENOR:
		return("analyzing monsters every 50..300+d200 turns");
	case ART_NARTHANC:
		return("a fire bolt (9..29d8) every 2..8+d8 turns");
	case ART_NIMTHANC:
		return("a frost bolt (6..26d8) every 2..7+d7 turns");
	case ART_DETHANC:
		return("a lightning bolt (4..24d8) every 2..6+d6 turns");
	case ART_RILIA:
		return("a stinking cloud (4..11) every 2..4+d4 turns");
	case ART_BELANGIL:
		return("a frost ball (48..108) every 2..5+d5 turns");
	case ART_DAL:
		return("removing fear and curing poison every 5 turns");
	case ART_RINGIL:
		return("a frost ball (100..400) every 75..300 turns");
	case ART_ERU:
		return("healing (1000), curing every 250..500 turns");
	case ART_DAWN:
		//return("summon the Legion of the Dawn every 500+d500 turns");
		return("banishing undead (100..130) every 100..500+d100 turns");
	case ART_ANDURIL:
		return("a fire ball (72..222) every 100..400 turns");
	case ART_FIRESTAR:
		return("a large fire ball (72..222) every 25..100 turns");
	case ART_FEANOR:
		return("haste self (20+d20 turns) every 50..200 turns");
	case ART_THEODEN:
		return("draining life (25%) every 100..400 turns");
	case ART_TURMIL:
		return("draining life (15%) every 20..70 turns");
	case ART_ARUNRUTH:
		return("a frost bolt (12..27d8) every 100..500 turns");
	case ART_CASPANION:
		return("door and trap destruction every 4..10 turns");
	case ART_AVAVIR:
		return("word of recall every 100..200 turns");
	case ART_TARATOL:
		return("haste self (20+d20 turns) every 30..100+d50 turns");
	case ART_ERIRIL:
		return("identifying every 2..10 turns");
	case ART_RUYIWANG:
#if 0 /* ART_OLORIN */
		//return("probing, detection and full id every 1000 turns");
		return("probing and detection every 5..20 turns");
#else /* ART_RUYIWANG */
		return("spinning around every 10..20+d5 turns");
#endif
	case ART_EONWE:
		return("obliteration every 500..1000 turns");
	case ART_LOTHARANG:
		return("curing wounds (4d8..4d28) every 1..3+d3 turns");
	case ART_CUBRAGOL:
		return("fire branding of bolts every 222..999 turns");
	case ART_ANGUIREL:
		return("a getaway every 15..35 turns");
	case ART_AEGLOS:
		return("a lightning ball (100..250) every 100..500 turns");
	case ART_OROME:
		return("stone-to-mud every 2..5 turns");
	case ART_SOULKEEPER:
		return("healing (1000) every 222..888 turns");
	case ART_BELEGENNON:
		//return("healing (777), curing and heroism every 300 turns");
		return("teleport every 2..10 turns");
	case ART_CELEBORN:
		return("genocide every 100..500 turns");
	case ART_LUTHIEN:
		return("restoring life levels every 100..450 turns");
	case ART_ULMO:
		return("teleporting away every 50..150 turns");
	case ART_COLLUIN:
		return("resistance (20+d20 turns) every 55..111 turns");
	case ART_HOLCOLLETH:
		return("sleep every 15..55 turns");
	case ART_THINGOL:
		return("recharging (60..100) an item every 20..70 turns");
	case ART_COLANNON:
		return("teleportation every 10..45 turns");
	case ART_TOTILA:
		return("confusing a monster (lev +10..60) every 10..50 turns");
	case ART_CAMMITHRIM:
		return("a magic missile (2..10d6) every 1..2 turns");
	case ART_PAURHACH:
		return("a fire bolt (9..29d8) every 2..8+d4 turns");
	case ART_PAURNIMMEN:
		return("a frost bolt (6..26d8) every 2..7+d3 turns");
	case ART_PAURAEGEN:
		return("a lightning bolt (4..24d8) every 2..6+d3 turns");
	case ART_PAURNEN:
		return("an acid bolt (5..25d8) every 2..5+d2 turns");
	case ART_FINGOLFIN:
		return("a magical arrow (150) every 15..90+d30 turns");
	case ART_HOLHENNETH:
		return("detection every 15..55+d25 turns");
	case ART_GONDOR:
		return("healing (500) every 100..500 turns");
	case ART_RAZORBACK:
		return("lightning balls (8x150..600) every 300..1000 turns");
	case ART_BLADETURNER:
		//return("invulnerability (4+d8) every 800 turns");
		//return("Berserk rage, prayer and resistance every 200..400 turns");
		return("Berserk rage and resistance every 200..400 turns");
	case ART_MEDIATOR:
		//return("breathing elements (300..600), berserk, prayer, resistance every 100..400 turns");
		return("breathing elements (300..600), berserk, resistance every 100..400 turns");
	case ART_KNOWLEDGE:
		return("whispers from beyond 100..200+d100 turns");
	case ART_GALADRIEL:
		return("illumination every 5..10+d10 turns");//damage details omitted
	case ART_UNDEATH:
		return("darkness and ruination every 5..10+d10 turns");
	case ART_ELENDIL:
		return("magic mapping and light every 10..50+d25 turns");
	case ART_THRAIN:
		//return("detection every 30+d30 turns");
		return("clairvoyance every 200..1000+d150 turns");
	case ART_INGWE:
		return("dispelling evil (lev x10 +0..500) every 50..300+d150 turns");
	case ART_CARLAMMAS:
		return("protection from evil every 25..225+d125 turns");
	case ART_FLAR:
		//return("dimension door every 100 turns");
		return("creating a gateway every 500..1000 turns");
	case ART_BARAHIR:
		return("dispelling small life (100..400) every 15..55+d55 turns");
	case ART_TULKAS:
		return("haste self (75+d50 turns) every 50..150+d100 turns");
	case ART_NARYA:
		return("a large fire ball (120..370) every 75..225+d75 turns");
	case ART_NENYA:
		return("a large frost ball (200..450) every 100..325+d125 turns");
	case ART_VILYA:
		return("a large lightning ball (250..500) every 100..425+d175 turns");
#if 0	// implement me!
	case ART_NARYA:
		return("healing (500) every 200+d100 turns");
	case ART_NENYA:
		return("healing (800) every 100+d200 turns");
	case ART_VILYA:
		return("greater healing (900) every 200+d200 turns");
#endif
	case ART_POWER:
		//return("powerful things every 225..450+d450 turns");
		return("powerful things every 450+d450 turns");
	case ART_STONE_LORE:
		return("perilous identify every 4..10 turns (drains 20 mp)");
	case ART_ANCHOR:
		return("a Space-time barrier every 100..300+d20 turns");
	case ART_DOR: case ART_GORLIM:
		return("rays of fear in every direction every 3x(lev+10) turns");
	case ART_GANDALF:
		return("restoring mana every 222..666 turns");
	case ART_EVENSTAR:
		return("restoration every 50..150 turns");
	case ART_ELESSAR:
		return("healing and curing black breath every 100..200 turns");
	case ART_MARDRA:
		//return("summon a dragonrider every 1000 turns");
		return("banishing dragons (100..130) every 500..1000 turns");
	case ART_PALANTIR_ITHIL:
	case ART_PALANTIR_ORTHANC:
		return("clairvoyance every 200..1000+d150 turns");
#if 0
	case ART_ROBINTON:
		return(music_info[3].desc);
	case ART_PIEMUR:
		return(music_info[9].desc);
	case ART_MENOLLY:
		return(music_info[10].desc);
#endif	// 0
	case ART_EREBOR:
		return("opening a secret passage every 50..200 turns");
	case ART_DRUEDAIN:
		return("detection every 49..99 turns");
	case ART_ROHAN:
		return("heroism, berserk rage, haste every 100..250+d50 turns");
	case ART_HELM:
		return("a sound ball (300..600) every 300 turns");
	case ART_BOROMIR:
		return("mass human summoning every 500..1000 turns");

	case ART_HURIN:
		return("berserk rage and +10 to speed (50) every 75..175+d75 turns");
	case ART_AXE_GOTHMOG:
		return("a fire ball (300..600) every 50..200+d200 turns");
	case ART_MELKOR:
		return("a darkness storm (150..300) every 20..100 turns");
	case ART_GROND:
		return("altering reality every 20000 turns");
	case ART_ORCHAST:
		return("detecting orcs every 10 turns");
	case ART_NIGHT:
		return("draining life (3*7%) every 50..250 turns");
	case ART_NATUREBANE:
		return("dispelling monsters (300) every 100..200+d200 turns");
	case ART_BILBO:
		return("destroying locks and traps every 5..30+d10 turns");
	case ART_SOULGRIP:
		return("holy prayer (+20 AC) every 50..150+d100 turns");
	case ART_GOGGLES_DM:
		return("identifying possessions");
	case ART_AMUGROM:
		return("temporary resistance boost every 50..150+d50 turns");
	case ART_HELLFIRE:
		return("invoking hellfire (400..600) every 30+d10 turns");
	case ART_SPIRITSHARD:
		return("turning into a wraith every 50..100+d50 turns");
	case ART_PHASING:
		return("opening the final gate to the shores of Valinor every 1000 turns");
	case ART_LEBOHAUM:
		return("singing a cheerful song every 30 turns");
	case ART_HAVOC:
		return("invoking a force bolt (8..24d8) every d2 turns");
	case ART_SMASHER:
		return("destroying doors every 10..15+d3 turns");
	case ART_COBALTFOCUS:
		return("mirage mirror for 30+d10 turns every 30..80+d10 turns");
	case ART_FIST:
		return("hellfire brand every 150..350+d50 turns");
	case ART_WARPSPEAR:
		return("teleport-to every 15..40+d5 turns");
	case ART_SEVENLEAGUE:
		return("teleportation every 5..15 turns");
	case ART_ANTIRIAD:
		return("fire a plasma bolt (50..65d20) every 2 turns");

#if 0 /* no, eg randart serpent amulet should retain basic activation! */
	/* For the moment ignore (non-ego) randarts */
	case ART_RANDART: return("a crash ;-)");
#endif
	}

	// -------------------- ego items -------------------- //

	/* Some ego items can be activated */
	if (is_ego_p(o_ptr, EGO_DRAGON))
		return("teleportation every 25..50+d50 turns");
	if (is_ego_p(o_ptr, EGO_JUMP))
		return("phase jump every 4..13+d3 turns");
	if (is_ego_p(o_ptr, EGO_SPINNING))
		return("spinning around every 15..50+d25 turns");
	if (is_ego_p(o_ptr, EGO_FURY))
		return("growing a fury every 50..100+d50 turns");
	if (is_ego_p(o_ptr, EGO_NOLDOR))
		return("detecting treasure every 5..20+d10 turns");
	if (is_ego_p(o_ptr, EGO_SPECTRAL))
		return("wraithform every 25..50+d50 turns");

	if (is_ego_p(o_ptr, EGO_CLOAK_LORDLY_RES))
		return("temporary resistance every 50..150+d20 turns");
	if (is_ego_p(o_ptr, EGO_AURA_FIRE2))
		return("temporary fire resistance every 50..150+d20 turns");
	if (is_ego_p(o_ptr, EGO_AURA_ELEC2))
		return("temporary lighting resistance every 50..150+d20 turns");
	if (is_ego_p(o_ptr, EGO_AURA_COLD2))
		return("temporary frost resistance every 50..150+d20 turns");

	//EGO_MUSIC_ELDAR, EGO_MUSIC_POWER

	// -------------------- base items -------------------- //

	/* Require activation ability */
	if (o_ptr->tval == TV_DRAG_ARMOR) {
		/* Branch on the sub-type */
		switch (o_ptr->sval) {
		case SV_DRAGON_BLUE:
			return("breathing lightning (600..1200) every 200+d100 turns");
			//return("polymorph into an Ancient Blue Dragon every 200+d100 turns");
		case SV_DRAGON_WHITE:
			return("breathing frost (600..1200) every 200+d100 turns");
			//return("polymorph into an Ancient White Dragon every 200+d100 turns");
		case SV_DRAGON_BLACK:
			return("breathing acid (600..1200) every 200+d100 turns");
			//return("polymorph into an Ancient Black Dragon every 200+d100 turns");
		case SV_DRAGON_GREEN:
			return("breathing poison (600..1200) every 200+d100 turns");
			//return("polymorph into an Ancient Green Dragon every 200+d100 turns");
		case SV_DRAGON_RED:
			return("breathing fire (600..1200) every 200+d100 turns");
			//return("polymorph into an Ancient Red Dragon every 200+d100 turns");
		case SV_DRAGON_MULTIHUED:
			return("breathing the elements (600..1200) every 200+d100 turns");
			//return("polymorph into an Ancient MultiHued Dragon every 200+d100 turns");
		case SV_DRAGON_PSEUDO:
			return("breathing light/dark (200..400) every 200+d100 turns");
			//return("polymorph into an Ethereal Drake every 200+d100 turns");
			//return("polymorph into a Pseudo Dragon every 200+d100 turns");
		case SV_DRAGON_BRONZE:
			return("breathing confusion (300..600) every 200+d100 turns");
			//return("polymorph into an Ancient Bronze Dragon every 200+d100 turns");
		case SV_DRAGON_GOLD:
			return("breathing sound (400..800) every 200+d100 turns");
			//return("polymorph into an Ancient Gold Dragon every 200+d100 turns");
		case SV_DRAGON_CHAOS:
			return("breathing chaos (600..1200) every 200+d100 turns");
			//return("polymorph into a Great Wyrm of Chaos every 200+d100 turns");
		case SV_DRAGON_LAW:
			return("breathing shards/sound (400..800) every 200+d100 turns");
			//return("polymorph into a Great Wyrm of Law every 200+d100 turns");
		case SV_DRAGON_BALANCE:
			return("breathing disenchantment (500..1000) every 200+d100 turns");
			//return("polymorph into a Great Wyrm of Balance every 200+d100 turns");
		case SV_DRAGON_SHINING:
			return("breathing light/dark (400..800) every 200+d100 turns");
			//return("polymorph into an Ethereal Dragon every 200+d100 turns");
		case SV_DRAGON_POWER:
			return("breathing havoc (675..1675) every 200+d100 turns");
			//return("polymorph into a Great Wyrm of Power every 200+d100 turns");
		case SV_DRAGON_DEATH:
			return("breathing nether (550..1050) every 200+d100 turns");
			//return("polymorph into a death drake every 200+d100 turns");
		case SV_DRAGON_CRYSTAL:
			return("breathing shards (400..800) every 200+d100 turns");
			//return("polymorph into a great crystal drake every 200+d100 turns");
		case SV_DRAGON_DRACOLICH:
			return("breathing nether/cold (400..1200) every 200+d100 turns");
			//return("polymorph into a dracolich every 200+d100 turns");
		case SV_DRAGON_DRACOLISK:
			return("breathing fire/nexus (250..1200) every 200+d100 turns");
			//return("polymorph into a dracolisk every 200+d100 turns");
		case SV_DRAGON_SKY:
			return("breathing electricity/light/gravity (300..1200) every 200+d100 turns");
			//return("polymorph into a sky drake every 200+d100 turns");
		case SV_DRAGON_SILVER:
			return("breathing inertia/cold (250..1200) every 200+d100 turns");
			//return("polymorph into an Ancient Gold Dragon every 200+d100 turns");
		}
	}

	if (o_ptr->tval == TV_RING) {
		switch (o_ptr->sval) {
		case SV_RING_ELEC:
			return("a ball of lightning (50..200) and resist lightning every 25..50+d25 turns");
		case SV_RING_FLAMES:
			return("a ball of fire (50..200) and resist fire every 25..50+d25 turns");
		case SV_RING_ICE:
			return("a ball of cold (50..200) and resist cold every 25..50+d25 turns");
		case SV_RING_ACID:
			return("a ball of acid (50..200) and resist acid every 25..50+d25 turns");
		case SV_RING_TELEPORTATION:
			return("teleportation and destruction of the ring");
		case SV_RING_POLYMORPH:
			if (o_ptr->pval) {
				char m_name[MNAME_LEN];

				m_name[0] = 0;
				if (!(r_info[o_ptr->pval].flags8 & RF8_PLURAL)) {
					if (is_a_vowel(*(r_info[o_ptr->pval].name + r_name)))
						strcpy(m_name, "an ");
					else strcpy(m_name, "a ");
				}
				strcat(m_name, r_info[o_ptr->pval].name + r_name);
				return(format("polymorphing into %s", m_name));
			} else
				return("memorizing the form you are mimicing");
		}
	}

	if (o_ptr->tval == TV_AMULET) {
		switch (o_ptr->sval) {
			/* The amulet of the moon can be activated for sleep */
		case SV_AMULET_THE_MOON:
			return("sleeping monsters (lev +20..100) every 100+d100 turns");
		case SV_AMULET_SERPENT:
			return("breathing venom (100..300) every 40+d60 turns");
		case SV_AMULET_RAGE:
			return("growing a fury every 150+d100 turns");
		}
	}

	if (o_ptr->tval == TV_GOLEM) {
		switch (o_ptr->sval) {
		case SV_GOLEM_ATTACK:
			return("commanding your golem to attack your target or stop doing so");
		case SV_GOLEM_GUARD:
			return("commanding your golem to stay and guard its position or stop doing so");
		case SV_GOLEM_FOLLOW:
			return("commanding your golem to follow you or stop doing so");
		}
	}

	if (o_ptr->tval == TV_JUNK && o_ptr->sval == SV_GLASS_SHARD)
		return("altering a death fate");

	if (o_ptr->tval == TV_JUNK && o_ptr->sval == SV_ENERGY_CELL)
		return("delivering a full energy recharge");

	if (o_ptr->tval == TV_JUNK && o_ptr->sval >= SV_GIFT_WRAPPING_START && o_ptr->sval <= SV_GIFT_WRAPPING_END)
		return("wrapping a present");
	if (o_ptr->tval == TV_SPECIAL && o_ptr->sval >= SV_GIFT_WRAPPING_START && o_ptr->sval <= SV_GIFT_WRAPPING_END)
		return("Opening the present");

#if 0
	if (o_ptr->tval == TV_PARCHMENT && o_ptr->sval == SV_PARCHMENT_DEATH)
		return("Spiritual recall");
#endif

	if (o_ptr->tval == TV_BOOK && is_custom_tome(o_ptr->sval))
		//return("transcribing a spell scroll or spell crystal into it");
		return(format("transcribing up to %d spell/prayer scrolls or spell crystals into it", o_ptr->bpval));

	if (o_ptr->tval == TV_RUNE) return("tracing a sigil onto equipment");

#ifdef ENABLE_DEMOLITIONIST
	if (o_ptr->tval == TV_CHEMICAL) {
		if (o_ptr->sval == SV_WOOD_CHIPS) return("heating up to get processed into charcoal");
		else if (o_ptr->sval == SV_MIXTURE) return("combining with other chemicals or mixtures. Or with itself to form a product");
		else return("combining with other chemical ingredients or mixtures");
	}
	if (o_ptr->tval == TV_CHARGE) {
		char *c;
		int fuse;

		if (o_ptr->note && (c = strchr(quark_str(o_ptr->note), '!')) && (c[1] == 'F')) {
			fuse = atoi(c + 2);

			/* Limits: Fuse duration must be between 1s and 15s */
			if (fuse > 15) fuse = 15;
			if (fuse < 1) fuse = 1;
		}
		/* Otherwise use default fuse length */
		else fuse = o_ptr->pval;

		return(format("ignition after %d seconds", fuse));
	}
	if (o_ptr->tval == TV_TOOL && o_ptr->sval == SV_TOOL_GRINDER) return("grinding things consisting of wood or metal to bits");
#endif

	/* Oops */
	return(NULL);
}

/*
 * Compute the probability (as a fraction of denominator die)
 * that a die roll of d(die) + bonus will have a result
 * less than target. This is trivial, but exists in analogy
 * with the much more involved two_dice_cdf below.
 */
static long one_die_cdf(int die, int bonus, int target) {
	if (target <= bonus + 1) return(0);
	if (target <= die + bonus) return((target - bonus - 1));
	return(die);
}

/*
 * Compute the probability (as a fraction of denominator die1 * die2)
 * that a die roll of d(die1) + d(die2) + bonus will have a result
 * less than target.
 */
static long two_dice_cdf(int die1, int die2, int bonus, int target) {
	if (die1 < die2) { // Swap to guarantee that die1 > die2 without requiring it of the user of the function.
		int temp = die2;

		die2 = die1;
		die1 = temp;
	}
	if (target <= bonus + 2) return(0);
	if (target <= die2 + bonus + 1) return((long)(target - bonus - 2) * (target - bonus - 1) / 2);
	if (target <= die1 + bonus + 1)
		return ((long)(target - die2 - bonus - 1) * die2 + (long)(die2 - 1) * die2 / 2);
	if (target <= die1 + die2 + bonus)
		return((long) die1 * die2 - (long) (die1 + die2 + bonus - target + 1) * (die1 + die2 + bonus - target + 2) / 2);
	return((long) die1 * die2);
}

/*
 * Compute the expected damage value of a melee critical hit,
 * given base damage and some other relevant parameters.
 * Bonus is equal to weapon weight + a quantity depending on +crits.
 * Crit die size is 0 if the player cannot use their weapon in a way
 * that utilizes the critical hits skill, and scales with that skill
 * up to 900 otherwise.
 * See also cmd1's critical_melee and py_attack_mon for more information.
 */
static int melee_crit_dam(int dam, int bonus, int crit_die) {
	long denominator = crit_die ? (700L * crit_die) : 700L; // This can in principle exceed 2^15, so it should be long.
	long great_chance = denominator - (crit_die ? two_dice_cdf(700, crit_die, bonus, 400) : one_die_cdf(700, bonus, 400));
	long superb_chance = denominator - (crit_die ? two_dice_cdf(700, crit_die, bonus, 700) : one_die_cdf(700, bonus, 700));
	long greater_chance = denominator - (crit_die ? two_dice_cdf(700, crit_die, bonus, 900) : one_die_cdf(700, bonus, 900));
	long superber_chance = denominator - (crit_die ? two_dice_cdf(700, crit_die, bonus, 1300) : one_die_cdf(700, bonus, 1300));

	// Checked the math for the largest possible values of damage and crit die,
	// and we get uncomfortably close to the 32-bit max value, so I'm playing it safe.
	int64_t crit_dam = (int64_t) denominator * (4 * dam + 15);


	crit_dam += (int64_t) great_chance * (dam + 15);
	crit_dam += (int64_t) superb_chance * (dam + 15);
	crit_dam += (int64_t) greater_chance * (dam + 15);
	crit_dam += (int64_t) superber_chance * (dam + 15);
	crit_dam /= (3 * denominator);

	return((int) crit_dam);
}

/*
 * Display the melee damage done with a multiplier
 */
//void output_dam(object_type *o_ptr, int mult, int mult2, cptr against, cptr against2, bool *first)
static void output_dam(int Ind, FILE *fff, object_type *o_ptr, int mult, int mult2, int bonus, int bonus2, cptr against, cptr against2) {
	player_type *p_ptr = Players[Ind];
	int dam;

	bool allow_skill_crit = rogue_armed_melee(o_ptr, p_ptr);
	int plus_crit = p_ptr->xtra_crit + calc_crit_obj(o_ptr);
	int w = (o_ptr->weight > 100) ? 10 : ((o_ptr->weight < 10) ? 10 : (110 - o_ptr->weight));
	int critical_chance = 2 * w + // Chance out of 5000.
		5 * (p_ptr->to_h + p_ptr->to_h_melee + o_ptr->to_h) +
		get_skill_scale(p_ptr, SKILL_MASTERY, 150) +
		50 * BOOST_CRIT(plus_crit) +
		(allow_skill_crit ? get_skill_scale(p_ptr, SKILL_CRITS, 2000) : 0);
	int crit_flat_bonus = o_ptr->weight +
		500 - (10000 / (BOOST_CRIT(p_ptr->xtra_crit) + 20));
	int scaled_crit_skill = get_skill_scale(p_ptr, SKILL_CRITS, 900);
	int crit_die_size = scaled_crit_skill ? scaled_crit_skill : 1; //randint(0)=1
	long critical_damage;


	dam = ((o_ptr->dd + (o_ptr->dd * o_ptr->ds)) * 5L * mult) / FACTOR_MULT;
	dam += (o_ptr->to_d + p_ptr->to_d + p_ptr->to_d_melee + bonus) * 10;
	dam *= p_ptr->num_blow;

#ifdef DEFENSIVE_STANCE_TOTAL_MELEE_REDUCTION
	if (p_ptr->combat_stance == 1) {
		if (p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD)
			switch (p_ptr->combat_stance_power) {
			case 0: dam = (dam * 7 + 9) / 10; break;
			case 1: dam = (dam * 7 + 9) / 10; break;
			case 2: dam = (dam * 7 + 9) / 10; break;
			case 3: dam = (dam * 7 + 9) / 10; break;
			}
		else
			switch (p_ptr->combat_stance_power) {
			case 0: dam = (dam * 6 + 9) / 10; break;
			case 1: dam = (dam * 7 + 9) / 10; break;
			case 2: dam = (dam * 7 + 9) / 10; break;
			case 3: dam = (dam * 7 + 9) / 10; break;
			}
	}
#endif

	// expected damage IF it crits
	critical_damage = melee_crit_dam(dam, crit_flat_bonus, allow_skill_crit ? crit_die_size : 0);
	// expected damage factoring in crits
	dam = (((long) critical_chance) * critical_damage + (5000L - critical_chance) * dam) / 5000;

	if (dam > 0) {
		if (dam % 10) fprintf(fff, "    %d.%d", dam / 10, dam % 10);
		else fprintf(fff, "    %d", dam / 10);
	} else fprintf(fff, "    0");
	fprintf(fff, " against %s", against);

	if (mult2) {
		fprintf(fff, "\n");
		dam = ((o_ptr->dd + (o_ptr->dd * o_ptr->ds)) * 5L * mult2) / FACTOR_MULT;
		dam += (o_ptr->to_d + p_ptr->to_d + p_ptr->to_d_melee + bonus2) * 10;
		dam *= p_ptr->num_blow;

		// expected damage IF it crits
		critical_damage = melee_crit_dam(dam, crit_flat_bonus, allow_skill_crit ? crit_die_size : 0);
		// expected damage factoring in crits
		dam = (((long) critical_chance) * critical_damage + (5000L - critical_chance) * dam) / 5000;

		if (dam > 0) {
			if (dam % 10) fprintf(fff, "    %d.%d", dam / 10, dam % 10);
			else fprintf(fff, "    %d", dam / 10);
		} else fprintf(fff, "    0");
		fprintf(fff, " against %s", against2);
	}
	fprintf(fff, "\n");
}

/* For displaying expected weapon damage prognosis on weapon inspection:
   Add player's own current slay flags to the weapon's, so we really get the damage we "would have"?
   Currently still missing monster brands/aura brands.		- C. Blue*/
#define DISPLAY_DAMAGE_INTRINSIC_SLAYS

/* XXX this ignores the chance of extra dmg via 'critical hit' */
static void display_weapon_damage(int Ind, object_type *o_ptr, FILE *fff, u32b f1) {
	player_type *p_ptr = Players[Ind];
	object_type forge, forge2, *old_ptr = &forge, *old_ptr2 = &forge2;
	bool first = TRUE;

	/* save timed effects that might be changed on weapon switching - C. Blue */
	long tim_wraith = p_ptr->tim_wraith;

	/* this stuff doesn't take into account dual-wield or shield(lessness) directly,
	   but the player actually has to take care of unequipping/reequipping his
	   secondary weapon slot before calling this accordingly to the results he wants
	   to get! We can just take care of forced cases which are..
	   - unequipping a shield or secondary weapon if weapon is MUST2H here.
	   - unequipping a secondary weapon if weapon is weapon is SHOULD2H - C. Blue */


	/* Ok now the hackish stuff, we replace the current weapon with this one */
	/* XXX this hack can be even worse under TomeNET, dunno :p */
	object_copy(old_ptr, &p_ptr->inventory[INVEN_WIELD]);
	object_copy(&p_ptr->inventory[INVEN_WIELD], o_ptr);
	p_ptr->inventory[INVEN_WIELD].number = 1; /* fix weight */

	/* handle secondary weapon or shield */
	object_copy(old_ptr2, &p_ptr->inventory[INVEN_ARM]);
	/* take care of MUST2H if player still has a shield or secondary weapon equipped */
	if (k_info[o_ptr->k_idx].flags4 & TR4_MUST2H) {
		p_ptr->inventory[INVEN_ARM].k_idx = 0; /* temporarily delete */
	}
	/* take care of SHOULD2H if player is dual-wielding */
	else if ((k_info[o_ptr->k_idx].flags4 & TR4_SHOULD2H) && (is_melee_weapon(p_ptr->inventory[INVEN_ARM].tval))) {
		p_ptr->inventory[INVEN_ARM].k_idx = 0; /* temporarily delete */
	}

	/* Hack -- hush the messages up */
	suppress_message = TRUE;
	suppress_boni = TRUE;
	calc_boni(Ind);

	fprintf(fff, "\n");
//	/* give weight warning, so player won't buy something he can't use. (todo: for shields and bows too) */
//	if (p_ptr->heavy_wield) fprintf(fff, "\377rThis weapon is currently too heavy for you to use effectively:\377w\n");
#if 0 /* don't colour the # of bpr */
	fprintf(fff, "\377sUsing it you would have \377W%d\377s blow%s and do an average damage per round of:\n", p_ptr->num_blow, (p_ptr->num_blow > 1) ? "s" : "");
#else /* colour the # of bpr -- compare with prt_bpr_wraith() */
	{
		byte attr = TERM_L_GREEN;//TERM_L_WHITE;

		switch (p_ptr->pclass) {
		case CLASS_WARRIOR:
		case CLASS_MIMIC:
		case CLASS_PALADIN:
 #ifdef ENABLE_DEATHKNIGHT
		case CLASS_DEATHKNIGHT:
 #endif
 #ifdef ENABLE_HELLKNIGHT
		case CLASS_HELLKNIGHT:
 #endif
		case CLASS_RANGER:
		case CLASS_ROGUE:
		case CLASS_MINDCRAFTER:
			if (p_ptr->num_blow == 1) attr = TERM_ORANGE;
			else if (p_ptr->num_blow == 2) attr = TERM_YELLOW;
			break;
		case CLASS_SHAMAN:
		case CLASS_ADVENTURER:
		case CLASS_RUNEMASTER:
		case CLASS_PRIEST:
 #ifdef ENABLE_CPRIEST
		case CLASS_CPRIEST:
 #endif
		case CLASS_DRUID:
			if (p_ptr->num_blow == 1) attr = TERM_YELLOW;
			break;
		case CLASS_MAGE:
		case CLASS_ARCHER:
			break;
		}

		fprintf(fff, "\377sUsing it you would have \377%c%d\377s blow%s and do an average damage per round of:\n", color_attr_to_char(attr), p_ptr->num_blow, (p_ptr->num_blow > 1) ? "s" : "");
	}
#endif

	if (p_ptr->instakills) {
		/* Admin */
		fprintf(fff, "    \377%coo\377w against all monsters\n", p_ptr->instakills == 2 ? '6' : 'U');
	} else {
		/* Normal */

#ifdef DISPLAY_DAMAGE_INTRINSIC_SLAYS
		/* add player's own current slay flags to the weapon's, so we really get the damage we "would have"... */
		f1 |= p_ptr->slay | p_ptr->slay_equip | p_ptr->slay_melee; // TODO: Add monster brands and aura brands! And maybe temp-melee-weapon brands from enchantment spells.
#endif

		if (f1 & TR1_SLAY_ANIMAL) output_dam(Ind, fff, o_ptr, FACTOR_HURT, 0, FLAT_HURT_BONUS, 0, "animals", NULL);
		if (f1 & TR1_SLAY_EVIL) output_dam(Ind, fff, o_ptr, FACTOR_HURT, 0, FLAT_HURT_BONUS, 0, "evil creatures", NULL);
		if (f1 & TR1_SLAY_ORC) output_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "orcs", NULL);
		if (f1 & TR1_SLAY_TROLL) output_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "trolls", NULL);
		if (f1 & TR1_SLAY_GIANT) output_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "giants", NULL);
		if (f1 & TR1_KILL_DRAGON) output_dam(Ind, fff, o_ptr, FACTOR_KILL, 0, FLAT_KILL_BONUS, 0, "dragons", NULL);
		else if (f1 & TR1_SLAY_DRAGON) output_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "dragons", NULL);
		if (f1 & TR1_KILL_UNDEAD) output_dam(Ind, fff, o_ptr, FACTOR_KILL, 0, FLAT_KILL_BONUS, 0, "undead", NULL);
		else if (f1 & TR1_SLAY_UNDEAD) output_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "undead", NULL);
		if (f1 & TR1_KILL_DEMON) output_dam(Ind, fff, o_ptr, FACTOR_KILL, 0, FLAT_KILL_BONUS, 0, "demons", NULL);
		else if (f1 & TR1_SLAY_DEMON) output_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "demons", NULL);

		if (f1 & TR1_BRAND_FIRE) output_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non fire resistant creatures", "fire susceptible creatures");
		if (f1 & TR1_BRAND_COLD) output_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non cold resistant creatures", "cold susceptible creatures");
		if (f1 & TR1_BRAND_ELEC) output_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non lightning resistant creatures", "lightning susceptible creatures");
		if (f1 & TR1_BRAND_ACID) output_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non acid resistant creatures", "acid susceptible creatures");
		if (f1 & TR1_BRAND_POIS) output_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non poison resistant creatures", "poison susceptible creatures");

		output_dam(Ind, fff, o_ptr, FACTOR_MULT, 0, 0, 0, (first) ? "all monsters" : "other monsters", NULL);
	}

	fprintf(fff, "\n");

	/* restore secondary weapon or shield */
	object_copy(&p_ptr->inventory[INVEN_ARM], old_ptr2);
	/* get our weapon back */
	object_copy(&p_ptr->inventory[INVEN_WIELD], old_ptr);
	calc_boni(Ind);

	/* restore timed effects that might have been changed from the weapon switching - C. Blue */
	p_ptr->tim_wraith = tim_wraith;

	suppress_message = FALSE;
	suppress_boni = FALSE;
}

/*
 * Compute the expected damage value of a ranged critical hit,
 * given base damage and some other relevant parameters.
 * Bonus is equal to ammo weight + 2 * archery skill.
 * Crit die size follows a formula that has doubly diminishing returns with xtra_crit:
 * 600 - (12000 / (BOOST_CRIT(p_ptr->xtra_crit) + 20))
 * See also cmd1's critical_shot and cmd2's do_cmd_fire for more information.
 */
static int ranged_crit_dam(int dam, int bonus, int crit_die) {
	long denominator = 700L * crit_die; // This can in principle exceed 2^15, so it should be long.
	long great_chance = denominator - two_dice_cdf(700, crit_die, bonus, 350);
	long superb_chance = denominator - two_dice_cdf(700, crit_die, bonus, 650);
	long greater_chance = denominator - two_dice_cdf(700, crit_die, bonus, 900);
	long superber_chance = denominator - two_dice_cdf(700, crit_die, bonus, 1100);

	// Checked the math for the largest possible values of damage and crit die,
	// and we get uncomfortably close to the 32-bit max value, so I'm playing it safe.
	int64_t crit_dam = (int64_t) denominator * (4 * dam + 15);


	crit_dam += (int64_t) great_chance * (dam + 15);
	crit_dam += (int64_t) superb_chance * dam;
	crit_dam += (int64_t) greater_chance * dam;
	crit_dam += (int64_t) superber_chance * (dam + 15);
	crit_dam /= (3 * denominator);

	return((int) crit_dam);
}

static void output_boomerang_dam(int Ind, FILE *fff, object_type *o_ptr, int mult, int mult2, int bonus, int bonus2, cptr against, cptr against2) {
	player_type *p_ptr = Players[Ind];
	int dam;

	int critical_chance = o_ptr->weight + // Chance out of 3500.
		5 * (p_ptr->to_h + p_ptr->to_h_ranged + o_ptr->to_h) +
		50 * BOOST_CRIT(p_ptr->xtra_crit);
	int crit_flat_bonus = o_ptr->weight;
	int crit_die_size = 600 - (12000 / (BOOST_CRIT(p_ptr->xtra_crit) + 20));
	long critical_damage;

	dam = ((o_ptr->dd + (o_ptr->dd * o_ptr->ds)) * 5L * mult) / FACTOR_MULT;
	dam += (o_ptr->to_d + p_ptr->to_d_ranged + bonus) * 10;
	dam = dam * (10 + p_ptr->xtra_might) / 10;

#ifdef DEFENSIVE_STANCE_FIXED_RANGED_REDUCTION
	if (p_ptr->combat_stance == 1) dam /= 2;
#endif

	// expected damage IF it crits
	critical_damage = ranged_crit_dam(dam, crit_flat_bonus, crit_die_size);
	// expected damage factoring in crits
	dam = (((long) critical_chance) * critical_damage + (3500L - critical_chance) * dam) / 3500;

	if (dam > 0) {
		if (dam % 10) fprintf(fff, "    %d.%d", dam / 10, dam % 10);
		else fprintf(fff, "    %d", dam / 10);
	} else fprintf(fff, "    0");
	fprintf(fff, " against %s", against);

	if (mult2) {
		fprintf(fff, "\n");
		dam = ((o_ptr->dd + (o_ptr->dd * o_ptr->ds)) * 5L * mult2) / FACTOR_MULT;
		dam += (o_ptr->to_d + p_ptr->to_d_ranged + bonus2) * 10;
		dam = dam * (10 + p_ptr->xtra_might) / 10;

#ifdef DEFENSIVE_STANCE_FIXED_RANGED_REDUCTION
		if (p_ptr->combat_stance == 1) dam /= 2;
#endif

		// expected damage IF it crits
		critical_damage = ranged_crit_dam(dam, crit_flat_bonus, crit_die_size);
		// expected damage factoring in crits
		dam = (((long) critical_chance) * critical_damage + (3500L - critical_chance) * dam) / 3500;

		if (dam > 0) {
			if (dam % 10) fprintf(fff, "    %d.%d", dam / 10, dam % 10);
			else fprintf(fff, "    %d", dam / 10);
		} else fprintf(fff, "    0");
		fprintf(fff, " against %s", against2);
	}
	fprintf(fff, "\n");
}

static void display_boomerang_damage(int Ind, object_type *o_ptr, FILE *fff, u32b f1) {
	player_type *p_ptr = Players[Ind];
	object_type forge, *old_ptr = &forge;
	bool first = TRUE;

	/* save timed effects that might be changed on weapon switching - C. Blue */
	long tim_wraith = p_ptr->tim_wraith;


	/* this stuff doesn't take into account dual-wield or shield(lessness) directly,
	   but the player actually has to take care of unequipping/reequipping his
	   secondary weapon slot before calling this accordingly to the results he wants
	   to get! We can just take care of forced cases which are..
	   - unequipping a shield or secondary weapon if weapon is MUST2H here.
	   - unequipping a secondary weapon if weapon is weapon is SHOULD2H - C. Blue */

	/* Ok now the hackish stuff, we replace the current weapon with this one */
	/* XXX this hack can be even worse under TomeNET, dunno :p */
	object_copy(old_ptr, &p_ptr->inventory[INVEN_BOW]);
	object_copy(&p_ptr->inventory[INVEN_BOW], o_ptr);

	/* Hack -- hush the messages up */
	suppress_message = TRUE;
	suppress_boni = TRUE;
	calc_boni(Ind);

	fprintf(fff, "\n");
	///* give weight warning, so player won't buy something he can't use. (todo: for shields and bows too) */
	//if (p_ptr->heavy_wield) fprintf(fff, "\377rThis weapon is currently too heavy for you to use effectively:\377w\n");
	fprintf(fff, "\377sUsing it you would have %d throw%s and do an average damage per throw of:\n", p_ptr->num_fire, (p_ptr->num_fire > 1) ? "s" : "");

#ifdef DISPLAY_DAMAGE_INTRINSIC_SLAYS
	/* add player's own current slay flags to the weapon's, so we really get the damage we "would have"... */
	f1 |= p_ptr->slay | p_ptr->slay_equip; // TODO maybe: Add monster brands and aura brands, if applicable to ranged weapons
#endif

	if (f1 & TR1_SLAY_ANIMAL) output_boomerang_dam(Ind, fff, o_ptr, FACTOR_HURT, 0, FLAT_HURT_BONUS, 0, "animals", NULL);
	if (f1 & TR1_SLAY_EVIL) output_boomerang_dam(Ind, fff, o_ptr, FACTOR_HURT, 0, FLAT_HURT_BONUS, 0, "evil creatures", NULL);
	if (f1 & TR1_SLAY_ORC) output_boomerang_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "orcs", NULL);
	if (f1 & TR1_SLAY_TROLL) output_boomerang_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "trolls", NULL);
	if (f1 & TR1_SLAY_GIANT) output_boomerang_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "giants", NULL);
	if (f1 & TR1_KILL_DRAGON) output_boomerang_dam(Ind, fff, o_ptr, FACTOR_KILL, 0, FLAT_KILL_BONUS, 0, "dragons", NULL);
	else if (f1 & TR1_SLAY_DRAGON) output_boomerang_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "dragons", NULL);
	if (f1 & TR1_KILL_UNDEAD) output_boomerang_dam(Ind, fff, o_ptr, FACTOR_KILL, 0, FLAT_KILL_BONUS, 0, "undead", NULL);
	else if (f1 & TR1_SLAY_UNDEAD) output_boomerang_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "undead", NULL);
	if (f1 & TR1_KILL_DEMON) output_boomerang_dam(Ind, fff, o_ptr, FACTOR_KILL, 0, FLAT_KILL_BONUS, 0, "demons", NULL);
	else if (f1 & TR1_SLAY_DEMON) output_boomerang_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "demons", NULL);

	if (f1 & TR1_BRAND_FIRE) output_boomerang_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non fire resistant creatures", "fire susceptible creatures");
	if (f1 & TR1_BRAND_COLD) output_boomerang_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non cold resistant creatures", "cold susceptible creatures");
	if (f1 & TR1_BRAND_ELEC) output_boomerang_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non lightning resistant creatures", "lightning susceptible creatures");
	if (f1 & TR1_BRAND_ACID) output_boomerang_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non acid resistant creatures", "acid susceptible creatures");
	if (f1 & TR1_BRAND_POIS) output_boomerang_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non poison resistant creatures", "poison susceptible creatures");

	output_boomerang_dam(Ind, fff, o_ptr, FACTOR_MULT, 0, 0, 0, (first) ? "all monsters" : "other monsters", NULL);

	fprintf(fff, "\n");

	/* get our weapon back */
	object_copy(&p_ptr->inventory[INVEN_BOW], old_ptr);
	calc_boni(Ind);

	/* restore timed effects that might have been changed from the weapon switching - C. Blue */
	p_ptr->tim_wraith = tim_wraith;

	suppress_message = FALSE;
	suppress_boni = FALSE;
}

/*
 * Display the ammo damage done with a multiplier
 */
//void output_ammo_dam(object_type *o_ptr, int mult, int mult2, cptr against, cptr against2, bool *first)
static void output_ammo_dam(int Ind, FILE *fff, object_type *o_ptr, int mult, int mult2, int bonus, int bonus2, cptr against, cptr against2) {
	player_type *p_ptr = Players[Ind];
	int dam;
	object_type *b_ptr = &p_ptr->inventory[INVEN_BOW];
	int tmul = get_shooter_mult(b_ptr) + p_ptr->xtra_might;

	int critical_chance = o_ptr->weight + // Chance out of 3500.
		5 * (p_ptr->to_h + p_ptr->to_h_ranged + o_ptr->to_h) +
		get_skill_scale(p_ptr, SKILL_ARCHERY, 150) +
		50 * BOOST_CRIT(p_ptr->xtra_crit);
	int crit_flat_bonus = o_ptr->weight + get_skill_scale(p_ptr, SKILL_ARCHERY, 100);
	int crit_die_size = 600 - (12000 / (BOOST_CRIT(p_ptr->xtra_crit) + 20));
	long critical_damage;

	dam = (o_ptr->dd + (o_ptr->dd * o_ptr->ds)) * 5;
	dam *= FACTOR_MULT + ((mult - FACTOR_MULT) * 2) / 5;
	dam /= FACTOR_MULT;
	dam += (bonus / 2) * 10;
	dam += (o_ptr->to_d + b_ptr->to_d) * 10;
	dam += (p_ptr->to_d_ranged) * 10;
	dam *= tmul;

#ifdef DEFENSIVE_STANCE_FIXED_RANGED_REDUCTION
	if (p_ptr->combat_stance == 1) dam /= 2;
#endif

	// expected damage IF it crits
	critical_damage = ranged_crit_dam(dam, crit_flat_bonus, crit_die_size);
	// expected damage factoring in crits
	dam = (((long) critical_chance) * critical_damage + (3500L - critical_chance) * dam) / 3500;

	if (dam > 0) {
		if (dam % 10) fprintf(fff, "    %d.%d", dam / 10, dam % 10);
		else fprintf(fff, "    %d", dam / 10);
	} else fprintf(fff, "    0");
	fprintf(fff, " against %s", against);

	if (mult2) {
		fprintf(fff, "\n");
		dam = (o_ptr->dd + (o_ptr->dd * o_ptr->ds)) * 5;
		dam *= FACTOR_MULT + ((mult2 - FACTOR_MULT) * 2) / 5;
		dam /= FACTOR_MULT;
		dam += (bonus2 / 2) * 10;
		dam += (o_ptr->to_d + b_ptr->to_d) * 10;
		dam += (p_ptr->to_d_ranged) * 10;
		dam *= tmul;

#ifdef DEFENSIVE_STANCE_FIXED_RANGED_REDUCTION
		if (p_ptr->combat_stance == 1) dam /= 2;
#endif

		// expected damage IF it crits
		critical_damage = ranged_crit_dam(dam, crit_flat_bonus, crit_die_size);
		// expected damage factoring in crits
		dam = (((long) critical_chance) * critical_damage + (3500L - critical_chance) * dam) / 3500;

		if (dam > 0) {
			if (dam % 10) fprintf(fff, "    %d.%d", dam / 10, dam % 10);
			else fprintf(fff, "    %d", dam / 10);
		} else fprintf(fff, "    0");
		fprintf(fff, " against %s", against2);
	}
	fprintf(fff, "\n");
}

/*
 * Outputs the damage we do/would do with the current bow and this ammo
 */
/* TODO: tell something about boomerangs */
static void display_ammo_damage(int Ind, object_type *o_ptr, FILE *fff, u32b f1, u32b bow_f1) {
	player_type *p_ptr = Players[Ind];
	object_type forge, *old_ptr = &forge, *ob_ptr = &p_ptr->inventory[INVEN_BOW];
	bool first = TRUE;
	// int i;

	/* save timed effects that might be changed on ammo switching - C. Blue */
	long tim_wraith = p_ptr->tim_wraith;


	switch (o_ptr->tval) {
	case TV_SHOT:
		if (ob_ptr->tval != TV_BOW || ob_ptr->sval != SV_SLING) {
			fprintf(fff, "\n\377s(You need to have a sling equipped to calculate ammunition damage.)\n");
			return;
		}
		break;
	case TV_ARROW:
		if (ob_ptr->tval != TV_BOW || (ob_ptr->sval != SV_SHORT_BOW && ob_ptr->sval != SV_LONG_BOW)) {
			fprintf(fff, "\n\377s(You need to have a bow equipped to calculate ammunition damage.)\n");
			return;
		}
		break;
	case TV_BOLT:
		if (ob_ptr->tval != TV_BOW || (ob_ptr->sval != SV_LIGHT_XBOW && ob_ptr->sval != SV_HEAVY_XBOW)) {
			fprintf(fff, "\n\377s(You need to have a crossbow equipped to calculate ammunition damage.)\n");
			return;
		}
		break;
	}

	/* swap hack */
	object_copy(old_ptr, &p_ptr->inventory[INVEN_AMMO]);
	object_copy(&p_ptr->inventory[INVEN_AMMO], o_ptr);

	/* Hack -- hush the messages up */
	suppress_message = TRUE;
	suppress_boni = TRUE;
	calc_boni(Ind);

#ifdef DISPLAY_DAMAGE_INTRINSIC_SLAYS
	/* add player's own current slay flags to the weapon's, so we really get the damage we "would have"... */
	f1 |= p_ptr->slay | p_ptr->slay_equip; // TODO maybe: Add monster brands and aura brands, if applicable to ranged weapons
#endif

	/* combine slay flags of ammo and bow */
	f1 |= bow_f1;

	fprintf(fff, "\n\377sUsing it with your current shooter you would do an average damage per shot of:\n");
	if (f1 & TR1_SLAY_ANIMAL) output_ammo_dam(Ind, fff, o_ptr, FACTOR_HURT, 0, FLAT_HURT_BONUS, 0, "animals", NULL);
	if (f1 & TR1_SLAY_EVIL) output_ammo_dam(Ind, fff, o_ptr, FACTOR_HURT, 0, FLAT_HURT_BONUS, 0, "evil creatures", NULL);
	if (f1 & TR1_SLAY_ORC) output_ammo_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "orcs", NULL);
	if (f1 & TR1_SLAY_TROLL) output_ammo_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "trolls", NULL);
	if (f1 & TR1_SLAY_GIANT) output_ammo_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "giants", NULL);
	if (f1 & TR1_KILL_DRAGON) output_ammo_dam(Ind, fff, o_ptr, FACTOR_KILL, 0, FLAT_KILL_BONUS, 0, "dragons", NULL);
	else if (f1 & TR1_SLAY_DRAGON) output_ammo_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "dragons", NULL);
	if (f1 & TR1_KILL_UNDEAD) output_ammo_dam(Ind, fff, o_ptr, FACTOR_KILL, 0, FLAT_KILL_BONUS, 0, "undead", NULL);
	else if (f1 & TR1_SLAY_UNDEAD) output_ammo_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "undead", NULL);
	if (f1 & TR1_KILL_DEMON) output_ammo_dam(Ind, fff, o_ptr, FACTOR_KILL, 0, FLAT_KILL_BONUS, 0, "demons", NULL);
	else if (f1 & TR1_SLAY_DEMON) output_ammo_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "demons", NULL);

	if (f1 & TR1_BRAND_FIRE) output_ammo_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non fire resistant creatures", "fire susceptible creatures");
	if (f1 & TR1_BRAND_COLD) output_ammo_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non cold resistant creatures", "cold susceptible creatures");
	if (f1 & TR1_BRAND_ELEC) output_ammo_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non lightning resistant creatures", "lightning susceptible creatures");
	if (f1 & TR1_BRAND_ACID) output_ammo_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non acid resistant creatures", "acid susceptible creatures");
	if (f1 & TR1_BRAND_POIS) output_ammo_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non poison resistant creatures", "poison susceptible creatures");

	output_ammo_dam(Ind, fff, o_ptr, FACTOR_MULT, 0, 0, 0, (first) ? "all monsters" : "other monsters", NULL);
	fprintf(fff, "\n");

#if 0
	if (o_ptr->pval2) {
		roff("The explosion will be ");
		i = 0;
		while (gf_names[i].gf != -1) {
			if (gf_names[i].gf == o_ptr->pval2) break;
			i++;
		}
		c_roff(TERM_L_GREEN, (gf_names[i].gf != -1) ? gf_names[i].name : "something weird");
		roff(".");
	}
#endif	// 0

	/* get our ammo back */
	object_copy(&p_ptr->inventory[INVEN_AMMO], old_ptr);
	calc_boni(Ind);

	/* restore timed effects that might have been changed from the weapon switching - C. Blue */
	p_ptr->tim_wraith = tim_wraith;

	suppress_message = FALSE;
	suppress_boni = FALSE;
}

/* Note: We assume that we're only called if o_ptr->tval is TV_BOW */
static void display_shooter_damage(int Ind, object_type *o_ptr, FILE *fff, u32b f1, u32b ammo_f1) {
	player_type *p_ptr = Players[Ind];
	object_type forge, *old_ptr = &forge, *oa_ptr = &p_ptr->inventory[INVEN_AMMO];
	bool first = TRUE;
	// int i;

	/* save timed effects that might be changed on ammo switching - C. Blue */
	long tim_wraith = p_ptr->tim_wraith;


	switch (o_ptr->sval) {
	case SV_SLING:
		if (oa_ptr->tval != TV_SHOT) {
			fprintf(fff, "\n\377s(You need to have pebbles or shots in your quiver to calculate sling damage.)\n");
			return;
		}
		break;
	case SV_SHORT_BOW:
	case SV_LONG_BOW:
		if (oa_ptr->tval != TV_ARROW) {
			fprintf(fff, "\n\377s(You need to have arrows equipped to calculate bow damage.)\n");
			return;
		}
		break;
	case SV_LIGHT_XBOW:
	case SV_HEAVY_XBOW:
		if (oa_ptr->tval != TV_BOLT) {
			fprintf(fff, "\n\377s(You need to have bolts equipped to calculate crossbow damage.)\n");
			return;
		}
		break;
	}

	/* swap hack */
	object_copy(old_ptr, &p_ptr->inventory[INVEN_BOW]);
	object_copy(&p_ptr->inventory[INVEN_BOW], o_ptr);

	/* Hack -- hush the messages up */
	suppress_message = TRUE;
	suppress_boni = TRUE;
	calc_boni(Ind);

#ifdef DISPLAY_DAMAGE_INTRINSIC_SLAYS
	/* add player's own current slay flags to the weapon's, so we really get the damage we "would have"... */
	f1 |= p_ptr->slay | p_ptr->slay_equip; // TODO maybe: Add monster brands and aura brands, if applicable to ranged weapons
#endif

	/* combine slay flags of ammo and bow */
	f1 |= ammo_f1;

	fprintf(fff, "\n\377sUsing it with your ammo you would have %d shot%s and do an average damage of:\n", p_ptr->num_fire, (p_ptr->num_fire > 1) ? "s" : "");
	if (f1 & TR1_SLAY_ANIMAL) output_ammo_dam(Ind, fff, oa_ptr, FACTOR_HURT, 0, FLAT_HURT_BONUS, 0, "animals", NULL);
	if (f1 & TR1_SLAY_EVIL) output_ammo_dam(Ind, fff, oa_ptr, FACTOR_HURT, 0, FLAT_HURT_BONUS, 0, "evil creatures", NULL);
	if (f1 & TR1_SLAY_ORC) output_ammo_dam(Ind, fff, oa_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "orcs", NULL);
	if (f1 & TR1_SLAY_TROLL) output_ammo_dam(Ind, fff, oa_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "trolls", NULL);
	if (f1 & TR1_SLAY_GIANT) output_ammo_dam(Ind, fff, oa_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "giants", NULL);
	if (f1 & TR1_KILL_DRAGON) output_ammo_dam(Ind, fff, oa_ptr, FACTOR_KILL, 0, FLAT_KILL_BONUS, 0, "dragons", NULL);
	else if (f1 & TR1_SLAY_DRAGON) output_ammo_dam(Ind, fff, oa_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "dragons", NULL);
	if (f1 & TR1_KILL_UNDEAD) output_ammo_dam(Ind, fff, oa_ptr, FACTOR_KILL, 0, FLAT_KILL_BONUS, 0, "undead", NULL);
	else if (f1 & TR1_SLAY_UNDEAD) output_ammo_dam(Ind, fff, oa_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "undead", NULL);
	if (f1 & TR1_KILL_DEMON) output_ammo_dam(Ind, fff, oa_ptr, FACTOR_KILL, 0, FLAT_KILL_BONUS, 0, "demons", NULL);
	else if (f1 & TR1_SLAY_DEMON) output_ammo_dam(Ind, fff, oa_ptr, FACTOR_SLAY, 0, FLAT_SLAY_BONUS, 0, "demons", NULL);

	if (f1 & TR1_BRAND_FIRE) output_ammo_dam(Ind, fff, oa_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non fire resistant creatures", "fire susceptible creatures");
	if (f1 & TR1_BRAND_COLD) output_ammo_dam(Ind, fff, oa_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non cold resistant creatures", "cold susceptible creatures");
	if (f1 & TR1_BRAND_ELEC) output_ammo_dam(Ind, fff, oa_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non lightning resistant creatures", "lightning susceptible creatures");
	if (f1 & TR1_BRAND_ACID) output_ammo_dam(Ind, fff, oa_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non acid resistant creatures", "acid susceptible creatures");
	if (f1 & TR1_BRAND_POIS) output_ammo_dam(Ind, fff, oa_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, FLAT_BRAND_BONUS, FLAT_BRAND_BONUS, "non poison resistant creatures", "poison susceptible creatures");

	output_ammo_dam(Ind, fff, oa_ptr, FACTOR_MULT, 0, 0, 0, (first) ? "all monsters" : "other monsters", NULL);
	fprintf(fff, "\n");

#if 0
	if (oa_ptr->pval2) {
		roff("The explosion will be ");
		i = 0;
		while (gf_names[i].gf != -1)
		{
			if (gf_names[i].gf == oa_ptr->pval2)
				break;
			i++;
		}
		c_roff(TERM_L_GREEN, (gf_names[i].gf != -1) ? gf_names[i].name : "something weird");
		roff(".");
	}
#endif	// 0

	/* get our ammo back */
	object_copy(&p_ptr->inventory[INVEN_BOW], old_ptr);
	calc_boni(Ind);

	/* restore timed effects that might have been changed from the weapon switching - C. Blue */
	p_ptr->tim_wraith = tim_wraith;

	suppress_message = FALSE;
	suppress_boni = FALSE;
}

/* display how much AC an armour would add for us (for MA users!) and
   especially if it would encumber us, so we won't have to guess in shops - C. Blue */
static void display_armour_handling(int Ind, object_type *o_ptr, FILE *fff, int Ind_target) {
	player_type *p_ptr = Ind_target ? Players[Ind_target] : Players[Ind], p_backup;
	object_type forge, *old_ptr = &forge;
	int slot;

	bool old_cumber_armor = p_ptr->cumber_armor;
	bool old_awkward_armor = p_ptr->awkward_armor;
	bool old_cumber_glove = p_ptr->cumber_glove;
	bool old_cumber_helm = p_ptr->cumber_helm;
	bool old_monk_heavyarmor = p_ptr->monk_heavyarmor;
	bool old_rogue_heavyarmor = p_ptr->rogue_heavyarmor;
	bool old_heavy_shield = p_ptr->heavy_shield;
	bool old_awkward_shoot = p_ptr->awkward_shoot;

	/* save timed effects that might be changed on weapon switching */
	long tim_wraith = p_ptr->tim_wraith;


	if (!Ind_target) Ind_target = Ind;
	slot = wield_slot(Ind, o_ptr); /* slot the item goes into */

	/* since his mana or HP might get changed or even nulled, backup player too! */
	COPY(&p_backup, p_ptr, player_type);

	/* Ok now the hackish stuff, we replace the current armour with this one */
	object_copy(old_ptr, &p_ptr->inventory[slot]);
	object_copy(&p_ptr->inventory[slot], o_ptr);
	p_ptr->inventory[slot].number = 1; /* fix weight, for martial arts especially */

#if 0 /* shouldn't be required just for measuring encumberment */
	if (!star_identify) {
		p_ptr->inventory[slot].name1 = p_ptr->inventory[slot].name2 = p_ptr->inventory[slot].name2b = 0;
		p_ptr->inventory[slot].pval = 0;
	}
#endif

	/* Hack -- hush the messages up */
	suppress_message = TRUE;
	suppress_boni = TRUE;
	calc_boni(Ind_target);
	calc_mana(Ind_target);

	/* Gained encumberment? */
	if ((p_ptr->cumber_armor && !old_cumber_armor) ||
	    (p_ptr->awkward_armor && !old_awkward_armor) ||
	    (p_ptr->cumber_glove && !old_cumber_glove) ||
	    (p_ptr->cumber_helm && !old_cumber_helm) ||
	    (p_ptr->heavy_shield && !old_heavy_shield) ||
	    (p_ptr->awkward_shoot && !old_awkward_shoot) ||
	    (p_ptr->monk_heavyarmor && !old_monk_heavyarmor) ||
	    (p_ptr->rogue_heavyarmor && !old_rogue_heavyarmor)) {

		if (!old_cumber_armor && !old_awkward_armor && !old_cumber_glove &&
		    !old_cumber_helm && !old_monk_heavyarmor && !old_rogue_heavyarmor &&
		    !old_heavy_shield && !old_awkward_shoot) {
			if (fff) fprintf(fff, "\377rWearing this armour would encumber you in the following way(s):\n");
			else msg_print(Ind, "\377s  This piece of armour seems so heavy that it would encumber you!");
		} else {
			if (fff) fprintf(fff, "\377rWearing this armour would additionally encumber you in the following way(s):\n");
			else msg_print(Ind, "\377s  This piece of armour seems so heavy that it would additionally encumber you!");
		}

		if (fff) {
			if (p_ptr->cumber_armor && !old_cumber_armor) fprintf(fff, "\377r    Your movement.\n");
			if (p_ptr->awkward_armor && !old_awkward_armor) fprintf(fff, "\377r    Your spellcasting abilities.\n");
			if (p_ptr->cumber_glove && !old_cumber_glove) fprintf(fff, "\377r    Your spellcasting abilities.\n");
			if (p_ptr->cumber_helm && !old_cumber_helm) fprintf(fff, "\377r    Your spellcasting abilities.\n");
			if (p_ptr->heavy_shield && !old_heavy_shield) {
				if (o_ptr->tval == TV_SHIELD) fprintf(fff, "\377r    Your strength will be too low for wielding this shield.\n");
				else fprintf(fff, "\377r    Your strength will be too low for wielding your shield.\n");
			}
			if (p_ptr->monk_heavyarmor && !old_monk_heavyarmor) fprintf(fff, "\377r    Your martial arts performance.\n");
			if (p_ptr->rogue_heavyarmor && !old_rogue_heavyarmor) fprintf(fff, "\377r    Your flexibility and awareness.\n");
			if (p_ptr->awkward_shoot && !old_awkward_shoot) fprintf(fff, "\377r    Your shooting ability (all shields do that).\n");
			//fprintf(fff, "\n");
		}
	}

	/* Lost encumberment? */
	if ((!p_ptr->cumber_armor && old_cumber_armor) ||
	    (!p_ptr->awkward_armor && old_awkward_armor) ||
	    (!p_ptr->cumber_glove && old_cumber_glove) ||
	    (!p_ptr->cumber_helm && old_cumber_helm) ||
	    (!p_ptr->heavy_shield && old_heavy_shield) ||
	    (!p_ptr->awkward_shoot && old_awkward_shoot) ||
	    (!p_ptr->monk_heavyarmor && old_monk_heavyarmor) ||
	    (!p_ptr->rogue_heavyarmor && old_rogue_heavyarmor)) {

		if (!p_ptr->cumber_armor && !p_ptr->awkward_armor && !p_ptr->cumber_glove &&
		    !p_ptr->cumber_helm && !p_ptr->monk_heavyarmor && !p_ptr->rogue_heavyarmor &&
		    !p_ptr->heavy_shield && !p_ptr->awkward_shoot) {
			if (fff) fprintf(fff, "\377gWearing this armour would remove your encumberment.\n");
			else msg_print(Ind, "\377s  This piece of armour seems light enough to not encumber you.");
		} else {
			if (fff) fprintf(fff, "\377gWearing this armour would remove encumberment of the following:\n");
			else msg_print(Ind, "\377s  This piece of armour seems light enough to reduce some encumberment.");

			if (fff) {
				if (!p_ptr->cumber_armor && old_cumber_armor) fprintf(fff, "\377g    Your movement.\n");
				if (!p_ptr->awkward_armor && old_awkward_armor) fprintf(fff, "\377g    Your spellcasting abilities.\n");
				if (!p_ptr->cumber_glove && old_cumber_glove) fprintf(fff, "\377g    Your spellcasting abilities.\n");
				if (!p_ptr->cumber_helm && old_cumber_helm) fprintf(fff, "\377g    Your spellcasting abilities.\n");
				if (!p_ptr->heavy_shield && old_heavy_shield) {
					if (o_ptr->tval == TV_SHIELD) fprintf(fff, "\377r    Your strength will be sufficient for wielding this shield.\n");
					else fprintf(fff, "\377r    Your strength will be sufficient for wielding your shield.\n");
				}
				if (!p_ptr->monk_heavyarmor && old_monk_heavyarmor) fprintf(fff, "\377g    Your martial arts performance.\n");
				if (!p_ptr->rogue_heavyarmor && old_rogue_heavyarmor) fprintf(fff, "\377g    Your flexibility and awareness.\n");
				if (!p_ptr->awkward_shoot && old_awkward_shoot) fprintf(fff, "\377g    Your shooting ability (all shields encumber this).\n");
			}
		}
		//if (fff) fprintf(fff, "\n");
	}

#ifdef NEW_SHIELDS_NO_AC
	if (o_ptr->tval != TV_SHIELD)
#endif
	if (fff) fprintf(fff, "\377sUsing it you would have \377W%d\377s total AC.\n", p_ptr->dis_ac + p_ptr->dis_to_a);
	//else msg_format(Ind, "\377s  Using it you would have %d total AC.", p_ptr->dis_ac + p_ptr->dis_to_a);

	/* get our armour back */
	object_copy(&p_ptr->inventory[slot], old_ptr);

	calc_boni(Ind_target);
	calc_mana(Ind_target);
	suppress_boni = FALSE;
	suppress_message = FALSE;

	/* and get our mana (in most cases the problem) back, yay */
	COPY(p_ptr, &p_backup, player_type);

	/* restore timed effects that might have been changed from the weapon switching */
	p_ptr->tim_wraith = tim_wraith;
}

/* display weapon encumberment changes, so we won't have to guess in shops - C. Blue */
static void display_weapon_handling(int Ind, object_type *o_ptr, FILE *fff, int Ind_target) {
	player_type *p_ptr = Ind_target ? Players[Ind_target] : Players[Ind], p_backup;
	object_type forge, *old_ptr = &forge, forge2, *old_ptr2 = &forge2;

	bool old_heavy_wield = p_ptr->heavy_wield;
	bool old_icky_wield = p_ptr->icky_wield;
	bool old_awkward_wield = p_ptr->awkward_wield;
	//bool old_easy_wield = p_ptr->easy_wield;

	/* save timed effects that might be changed on weapon switching */
	long tim_wraith = p_ptr->tim_wraith;


	if (!Ind_target) Ind_target = Ind;

	/* since his mana or HP might get changed or even nulled, backup player too! */
	COPY(&p_backup, p_ptr, player_type);

	/* Ok now the hackish stuff, we replace the current weapon/shield with this one */
	object_copy(old_ptr, &p_ptr->inventory[INVEN_WIELD]);
	object_copy(old_ptr2, &p_ptr->inventory[INVEN_ARM]);
	object_copy(&p_ptr->inventory[INVEN_WIELD], o_ptr);
	p_ptr->inventory[INVEN_WIELD].number = 1; /* fix weight */
	if (k_info[o_ptr->k_idx].flags4 & TR4_MUST2H) p_ptr->inventory[INVEN_ARM].k_idx = 0;
	else if ((k_info[o_ptr->k_idx].flags4 & TR4_SHOULD2H) &&
	    (is_melee_weapon(p_ptr->inventory[INVEN_ARM].tval)))
		p_ptr->inventory[INVEN_ARM].k_idx = 0;

#if 0 /* shouldn't be required just for measuring encumberment */
	/* If we don't really know the item, rely on k_info flags only! */
	if (!star_identify) {
		p_ptr->inventory[INVEN_WIELD].name1 = p_ptr->inventory[INVEN_WIELD].name2 = p_ptr->inventory[INVEN_WIELD].name2b = 0;
		p_ptr->inventory[INVEN_WIELD].pval = 0;
	}
#endif

	/* Hack -- hush the messages up */
	suppress_message = TRUE;
	suppress_boni = TRUE;
	calc_boni(Ind_target);
	calc_mana(Ind_target);

	/* Gained encumberment? */
	if ((p_ptr->awkward_wield && !old_awkward_wield) ||
	    (p_ptr->heavy_wield && !old_heavy_wield) ||
	    (p_ptr->icky_wield && !old_icky_wield)) {

		if (!old_awkward_wield && !old_heavy_wield && !old_icky_wield) {
			if (fff) fprintf(fff, "\377rWielding this weapon would give you one or more mali:\n");
			//else msg_print(Ind, "\377s  This weapon seems too heavy or in another way unfit for you!");
			//see below instead
		} else {
			if (fff) fprintf(fff, "\377rWielding this weapon would give you one or more additional mali:\n");
			//else msg_print(Ind, "\377s  This weapon would seem to give you additional trouble!");
			//see below instead
		}

		if (fff) {
			if (p_ptr->awkward_wield && !old_awkward_wield) fprintf(fff, "\377r    One-handed use reduces its efficiency.\n");
			if (k_info[o_ptr->k_idx].flags4 & TR4_COULD2H) fprintf(fff, "\377r    Wielding it two-handedly might make it even more effective.\n");
			if (p_ptr->heavy_wield && !old_heavy_wield) fprintf(fff, "\377r    Your strength is insufficient to hold it properly.\n");
			if (p_ptr->icky_wield && !old_icky_wield) {
				if (p_ptr->blessed_weapon && (
				    p_ptr->prace == RACE_VAMPIRE
				    || p_ptr->ptrait == TRAIT_CORRUPTED
#ifdef ENABLE_CPRIEST
				    || p_ptr->pclass == CLASS_CPRIEST
#endif
#ifdef ENABLE_HELLKNIGHT
				    || p_ptr->pclass == CLASS_HELLKNIGHT
#endif
				    ))
					fprintf(fff, "\377r    It is blessed.\n");
				else
					fprintf(fff, "\377r    It is edged and not blessed.\n");
			}
			//fprintf(fff, "\n");
		}
		//we get this info although we didn't *id*!
		else {
			if (p_ptr->awkward_wield && !old_awkward_wield) msg_print(Ind, "\377s  One-handed use reduces its efficiency.");
			if (k_info[o_ptr->k_idx].flags4 & TR4_COULD2H) msg_print(Ind, "\377s  Wielding it two-handedly might make it even more effective.");
			if (p_ptr->heavy_wield && !old_heavy_wield) msg_print(Ind, "\377s  It seems to be too heavy for you to hold it properly.");
			if (p_ptr->pclass == CLASS_PRIEST && o_ptr->tval != TV_BLUNT) msg_print(Ind, "\377s  It is an edged weapon and might not be blessed.");
		}
	}

	/* Lost encumberment? */
	if ((!p_ptr->awkward_wield && old_awkward_wield) ||
	    (!p_ptr->heavy_wield && old_heavy_wield) ||
	    (!p_ptr->icky_wield && old_icky_wield)) {

		if (!p_ptr->awkward_wield && !p_ptr->heavy_wield && !p_ptr->icky_wield) {
			if (fff) fprintf(fff, "\377gWielding this weapon would remove all mali.\n");
			else msg_print(Ind, "\377s  It looks like you could use it just fine.");
		} else {
			if (fff) fprintf(fff, "\377gWielding this weapon would remove one or more mali:\n");
			else msg_print(Ind, "\377s  This weapon seems to fit better in some ways that your current one.");

			if (fff) {
				if (!p_ptr->awkward_wield && old_awkward_wield) fprintf(fff, "\377g    One-handed use will not reduce its efficiency.\n");
				if (!p_ptr->heavy_wield && old_heavy_wield) fprintf(fff, "\377g    Your strength is sufficient to hold it properly.\n");
				if (!p_ptr->icky_wield && old_icky_wield) {
					if (p_ptr->prace == RACE_VAMPIRE
					|| p_ptr->ptrait == TRAIT_CORRUPTED
#ifdef ENABLE_CPRIEST
					    || p_ptr->pclass == CLASS_CPRIEST
#endif
#ifdef ENABLE_HELLKNIGHT
					    || p_ptr->pclass == CLASS_HELLKNIGHT
#endif
					    )
						fprintf(fff, "\377g    It is not blessed.\n");
					else {
						if (o_ptr->tval == TV_BLUNT) fprintf(fff, "\377g    It is not an edged weapon.\n");
						else fprintf(fff, "\377g    It is an edged weapon, but it is blessed.\n");
					}
				}
			}
		}
		//if (fff) fprintf(fff, "\n");
	}

	/* get our weapon back */
	object_copy(&p_ptr->inventory[INVEN_WIELD], old_ptr);
	object_copy(&p_ptr->inventory[INVEN_ARM], old_ptr2);

	calc_boni(Ind_target);
	calc_mana(Ind_target);
	suppress_message = FALSE;
	suppress_boni = FALSE;

	/* and get our mana (in most cases the problem) back, yay */
	COPY(p_ptr, &p_backup, player_type);

	/* restore timed effects that might have been changed from the weapon switching */
	p_ptr->tim_wraith = tim_wraith;
}

/* display ranged weapon encumberment changes, so we won't have to guess in shops - C. Blue */
static void display_shooter_handling(int Ind, object_type *o_ptr, FILE *fff, int Ind_target) {
	player_type *p_ptr = Ind_target ? Players[Ind_target] : Players[Ind], p_backup;
	object_type forge, *old_ptr = &forge;

	bool old_heavy_shoot = p_ptr->heavy_shoot;
	//bool old_awkward_shoot = p_ptr->awkward_shoot;

	/* save timed effects that might be changed on weapon switching */
	long tim_wraith = p_ptr->tim_wraith;


	if (!Ind_target) Ind_target = Ind;

	/* since his mana or HP might get changed or even nulled, backup player too! */
	COPY(&p_backup, p_ptr, player_type);

	/* Ok now the hackish stuff, we replace the current weapon/shield with this one */
	object_copy(old_ptr, &p_ptr->inventory[INVEN_BOW]);
	object_copy(&p_ptr->inventory[INVEN_BOW], o_ptr);
	p_ptr->inventory[INVEN_BOW].number = 1; /* fix weight */

#if 0 /* shouldn't be required just for measuring encumberment */
	/* If we don't really know the item, rely on k_info flags only! */
	if (!star_identify) {
		p_ptr->inventory[INVEN_BOW].name1 = p_ptr->inventory[INVEN_BOW].name2 = p_ptr->inventory[INVEN_BOW].name2b = 0;
		p_ptr->inventory[INVEN_BOW].pval = 0;
	}
#endif

	/* Hack -- hush the messages up */
	suppress_message = TRUE;
	suppress_boni = TRUE;
	calc_boni(Ind_target);
	calc_mana(Ind_target);

	/* Gained encumberment? */
	if (p_ptr->heavy_shoot && !old_heavy_shoot) {

		if (!old_heavy_shoot) {
			if (fff) fprintf(fff, "\377rThis weapon currently seems too heavy for you to use!\n");
			else msg_print(Ind, "\377s  This weapon currently seems too heavy for you!");
		}
		//fprintf(fff, "\n");
	}

	/* Lost encumberment? */
	if (!p_ptr->heavy_shoot && old_heavy_shoot) {

		if (!p_ptr->heavy_shoot) {
			if (fff) fprintf(fff, "\377gThis weapon seems light enough for you to use properly.\n");
			else msg_print(Ind, "\377s  This weapon seems light enough for you to use properly.");
		}
		//if (fff) fprintf(fff, "\n");
	}

	/* get our weapon back */
	object_copy(&p_ptr->inventory[INVEN_BOW], old_ptr);

	calc_boni(Ind_target);
	calc_mana(Ind_target);
	suppress_message = FALSE;
	suppress_boni = FALSE;

	/* and get our mana (in most cases the problem) back, yay */
	COPY(p_ptr, &p_backup, player_type);

	/* restore timed effects that might have been changed from the weapon switching */
	p_ptr->tim_wraith = tim_wraith;
}

/* display tool encumberment changes, so we won't have to guess in shops - C. Blue */
static void display_tool_handling(int Ind, object_type *o_ptr, FILE *fff, int Ind_target) {
	player_type *p_ptr = Ind_target ? Players[Ind_target] : Players[Ind], p_backup;
	object_type forge, *old_ptr = &forge;

	bool old_heavy_tool = p_ptr->heavy_tool;

	/* save timed effects that might be changed on weapon switching */
	long tim_wraith = p_ptr->tim_wraith;


	if (!Ind_target) Ind_target = Ind;

	/* since his mana or HP might get changed or even nulled, backup player too! */
	COPY(&p_backup, p_ptr, player_type);

	/* Ok now the hackish stuff, we replace the current weapon/shield with this one */
	object_copy(old_ptr, &p_ptr->inventory[INVEN_TOOL]);
	object_copy(&p_ptr->inventory[INVEN_TOOL], o_ptr);
	p_ptr->inventory[INVEN_TOOL].number = 1; /* fix weight */

#if 0 /* shouldn't be required just for measuring encumberment */
	/* If we don't really know the item, rely on k_info flags only! */
	if (!star_identify) {
		p_ptr->inventory[INVEN_TOOL].name1 = p_ptr->inventory[INVEN_TOOL].name2 = p_ptr->inventory[INVEN_TOOL].name2b = 0;
		p_ptr->inventory[INVEN_TOOL].pval = 0;
	}
#endif

	/* Hack -- hush the messages up */
	suppress_message = TRUE;
	suppress_boni = TRUE;
	calc_boni(Ind_target);
	calc_mana(Ind_target);

	/* Gained encumberment? */
	if (p_ptr->heavy_tool && !old_heavy_tool) {
		if (fff) fprintf(fff, "\377r    Your strength is insufficient to hold it properly.\n");
		else msg_print(Ind, "\377s  It seems to be too heavy for you to hold it properly.");
	}

	/* Lost encumberment? */
	if (!p_ptr->heavy_tool && old_heavy_tool) {
		if (fff) fprintf(fff, "\377gThis tool seems light enough for you to use properly.\n");
		else msg_print(Ind, "\377s  It looks like you could use it just fine.");
	}

	/* get our weapon back */
	object_copy(&p_ptr->inventory[INVEN_TOOL], old_ptr);

	calc_boni(Ind_target);
	calc_mana(Ind_target);
	suppress_message = FALSE;
	suppress_boni = FALSE;

	/* and get our mana (in most cases the problem) back, yay */
	COPY(p_ptr, &p_backup, player_type);

	/* restore timed effects that might have been changed from the weapon switching */
	p_ptr->tim_wraith = tim_wraith;
}

/* New helper function for @@/@@@ inscriptions: Check if item might have hidden powers (in which case the inscription is declined).
  ignore_id: If TRUE, the function ignores the item's ID_MENTAL state (aka *identified*) and hence can still return(TRUE) in that case. Added for shop-pasting.
  Ind is allowed to be 0 for the edge case of power-reinscribing when curse-flipping. And now also for use in object_known() to check for ID_NO_HIDDEN application.
*/
bool maybe_hidden_powers(int Ind, object_type *o_ptr, bool ignore_id) {
	bool aware = !Ind || object_aware_p(Ind, o_ptr);
	ego_item_type *e_ptr;
	int j;

	/* item not already *id*ed or well known (flavoured item)? */
	if ((!ignore_id && (o_ptr->ident & ID_MENTAL)) ||  // todo maybe: check ID_NO_HIDDEN?
	    (k_info[o_ptr->k_idx].easy_know && aware && !o_ptr->name1 && !o_ptr->name2 && !o_ptr->name2b)) return(FALSE);

	/* Un-*id*-ed artifacts can (and will) always have hidden powers.. */
	if (o_ptr->name1) return(TRUE);

	/* Unidentified items always qualify (if they're not easily known anyway and don't need ID, eg flavoured items) */
	if (Ind && !object_known_p(Ind, o_ptr)) return(TRUE);

	/* be strict: unknown AM field counts too.
	   Arts can have deviant AM fields. For non-arts however they're obvious by just normal ID, to reveal +hit,+dam values. */
	if (o_ptr->tval == TV_SWORD && o_ptr->sval == SV_DARK_SWORD && o_ptr->name1) return(TRUE);

	/* Assume we must *id* (just once) to learn sigil powers - Kurzel */
	if (o_ptr->sigil) return(TRUE);

	/* Multi-Hued DSMs need *id* to reveal their immunities */
	if (o_ptr->tval == TV_DRAG_ARMOR && o_ptr->sval == SV_DRAGON_MULTIHUED) return(TRUE);

	/* Just add the fixed ego flags that we know to be on the item */
	if (o_ptr->name2) {
		e_ptr = &e_info[o_ptr->name2];
		for (j = 0; j < 5; j++) {
			if (e_ptr->rar[j] == 0) continue;
			/* hack: can *identifying* actually make a difference at all? */

			/* any non-trivial (on the object name itself visible) abilities? */
			if ((e_ptr->fego1[j] & ETR1_EASYKNOW_MASK) ||
			    (e_ptr->fego2[j] & ETR2_EASYKNOW_MASK) ||
			    /* random ego mods (R_xxx)? */
			    (e_ptr->esp[j] & ESP_R_MASK)) return(TRUE);

			/* random base mods? */
			if (e_ptr->rar[j] != 100) {
				if (e_ptr->flags1[j] | e_ptr->flags2[j] | e_ptr->flags3[j] |
				    e_ptr->flags4[j] | e_ptr->flags5[j] | e_ptr->flags6[j] |
				    e_ptr->esp[j]) return(TRUE);
			}
		}
	}
	if (o_ptr->name2b) {
		e_ptr = &e_info[o_ptr->name2b];
		for (j = 0; j < 5; j++) {
			if (e_ptr->rar[j] == 0) continue;
			/* hack: can *identifying* actually make a difference at all? */

			/* any non-trivial (on the object name itself visible) abilities? */
			if ((e_ptr->fego1[j] & ETR1_EASYKNOW_MASK) ||
			    (e_ptr->fego2[j] & ETR2_EASYKNOW_MASK) ||
			    /* random ego mods (R_xxx)? */
			    (e_ptr->esp[j] & ESP_R_MASK)) return(TRUE);

			/* random base mods? */
			if (e_ptr->rar[j] != 100) {
				if (e_ptr->flags1[j] | e_ptr->flags2[j] | e_ptr->flags3[j] |
				    e_ptr->flags4[j] | e_ptr->flags5[j] | e_ptr->flags6[j] |
				    e_ptr->esp[j]) return(TRUE);
			}
		}
	}
	return(FALSE);
}

/* Display examine (x/I) text of an item we haven't *identified* yet. */
#ifndef NEW_ID_SCREEN /* old way: paste some info directly into chat. */
void observe_aux(int Ind, object_type *o_ptr) {
	player_type *p_ptr = Players[Ind];
	char o_name[ONAME_LEN];
	u32b f1, f2, f3, f4, f5, f6, esp;
	object_type forge;

	/* Description */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);

 #ifdef PLAYER_STORES
	if (p_ptr->store_num <= -2 && o_ptr->note) player_stores_cut_inscription(o_name);
	if (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_CHEQUE) {
		msg_format(Ind, "\377sIt's a cheque worth \377y%d\377s gold pieces.", ps_get_cheque_value(o_ptr));
		return;
	}
 #endif

	/* Not *identified* */
	object_copy(&forge, o_ptr);
	forge.name1 = forge.name2 = forge.name2b = 0;
	forge.pval = 0;

	/* Extract some flags */
	object_flags(&forge, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	/* Describe */
	msg_format(Ind, "\377s%s:", o_name);
	// ?	if (strlen(o_name) > 77) msg_format(Ind, "\377s%s:", o_name + 77);

	if (o_ptr->tval == TV_SPECIAL && (o_ptr->sval == SV_SEAL || o_ptr->sval >= SV_GIFT_WRAPPING_START && o_ptr->sval <= SV_GIFT_WRAPPING_END)) {
		msg_print(Ind, "\377s  You have no special knowledge about that item.");
		return;
	}

	/* Sigil */
	if (o_ptr->sigil) msg_format(Ind, "\377B  It is emblazoned with a sigil of %s.", string_exec_lua(0, format("return rcraft_name(%d)", o_ptr->sigil)));

 #ifdef ENABLE_DEMOLITIONIST
	if (o_ptr->tval == TV_CHARGE) msg_format(Ind, "\377s  Its default fuse length will burn down in %d seconds.", o_ptr->pval);
 #endif
 #ifdef SUBINVEN_LIMIT_GROUP
	if (o_ptr->tval == TV_SUBINVEN) msg_format(Ind, "\377s  You cannot use more than one of this type of container.");
 #endif
	switch (o_ptr->tval) {
	case TV_BLUNT:
		msg_print(Ind, "\377s  It's a blunt weapon."); break;
	case TV_POLEARM:
		msg_print(Ind, "\377s  It's a polearm."); break;
	case TV_SWORD:
		msg_print(Ind, "\377s  It's a sword-type weapon."); break;
	case TV_AXE:
		msg_print(Ind, "\377s  It's an axe-type weapon."); break;
	}

	if (f4 & TR4_SHOULD2H) msg_print(Ind, "\377s  It can be wielded one-handed, but should be wielded two-handed.");
	else if (f4 & TR4_MUST2H) msg_print(Ind, "\377s  It must be wielded two-handed.");
	else if (f4 & TR4_COULD2H) {
		if (o_ptr->weight <= DUAL_MAX_WEIGHT)
			msg_print(Ind, "\377s  It can be wielded one-handed, two-handed or dual.");
		else
			msg_print(Ind, "\377s  It can be wielded one-handed or two-handed.");
	}
	else if (is_melee_weapon(o_ptr->tval) && o_ptr->weight <= DUAL_MAX_WEIGHT) msg_print(Ind, "\377s  It can be wielded one-handed or dual.");
	else if (is_melee_weapon(o_ptr->tval)) msg_print(Ind, "\377s  It is wielded one-handed.");

	if (is_throwing_weapon(o_ptr)) msg_print(Ind, "\377s  It can be used as an effective throwing weapon.");

	if (o_ptr->tval == TV_BOW) display_shooter_handling(Ind, &forge, NULL, Ind_target);
	else if (is_melee_weapon(o_ptr->tval)) display_weapon_handling(Ind, &forge, NULL, Ind_target);
	else if (is_armour(o_ptr->tval)) display_armour_handling(Ind, &forge, NULL, Ind_target);
	else if (o_ptr->tval == TV_DIGGING) display_tool_handling(Ind, &forge, NULL, Ind_target);

	if (wield_slot(Ind, o_ptr) == INVEN_WIELD
	    //|| is_armour(o_ptr->tval)
 #ifdef EQUIPPABLE_DIGGERS
	    || o_ptr->tval == TV_DIGGING
 #endif
	    ) {
		/* copied from object1.c.. */
		object_type forge, forge2, *old_ptr = &forge, *old_ptr2 = &forge2;
		long tim_wraith = p_ptr->tim_wraith;

		object_copy(old_ptr, &p_ptr->inventory[INVEN_WIELD]);
		object_copy(&p_ptr->inventory[INVEN_WIELD], o_ptr);
		p_ptr->inventory[INVEN_WIELD].number = 1; /* fix weight */
		object_copy(old_ptr2, &p_ptr->inventory[INVEN_ARM]);
		if (k_info[o_ptr->k_idx].flags4 & TR4_MUST2H)
			p_ptr->inventory[INVEN_ARM].k_idx = 0;
		else if ((k_info[o_ptr->k_idx].flags4 & TR4_SHOULD2H) &&
		    (is_melee_weapon(p_ptr->inventory[INVEN_ARM].tval)))
			p_ptr->inventory[INVEN_ARM].k_idx = 0;
		suppress_message = TRUE;
		suppress_boni = TRUE;
		calc_boni(Ind);
		msg_format(Ind, "\377s  With it, you can usually attack %d time%s/turn.",
		    p_ptr->num_blow, p_ptr->num_blow > 1 ? "s" : "");
		object_copy(&p_ptr->inventory[INVEN_ARM], old_ptr2);
		object_copy(&p_ptr->inventory[INVEN_WIELD], old_ptr);
		calc_boni(Ind);
		suppress_boni = FALSE;
		suppress_message = FALSE;
		p_ptr->tim_wraith = tim_wraith;
	} else msg_print(Ind, "\377s  You have no special knowledge about that item.");
}
#else /* new way: display an info screen as for identify_fully_aux(), for additional k_info information - C. Blue */
/* If inventory 'slot' isn't specified it must be -1. */
bool identify_combo_aux(int Ind, object_type *o_ptr, bool full, int slot, int Ind_target);
void observe_aux(int Ind, object_type *o_ptr, int slot) {
	(void)identify_combo_aux(Ind, o_ptr, FALSE, slot, 0);
}
bool identify_fully_aux(int Ind, object_type *o_ptr, bool assume_aware, int slot, int Ind_target) {
	/* special hack (added for *id*ed items in player stores):
	   we cannot fully inspect a flavoured item if we don't know the flavour yet */
	if (!assume_aware && !object_aware_p(Ind, o_ptr))
		return(identify_combo_aux(Ind, o_ptr, FALSE, slot, Ind_target));

	return(identify_combo_aux(Ind, o_ptr, TRUE, slot, Ind_target));
}
#endif

/*
 * Describe a "fully identified" item
 * New albeit unused: You can call this even if item isn't *ID*ed and it'll
 * just display some basic information, a note that it isn't *ID*ed, and exit
 * (for player stores maybe.).
 */
#ifndef NEW_ID_SCREEN
bool identify_fully_aux(int Ind, object_type *o_ptr) {
	player_type *p_ptr = Players[Ind];
#else
/* combined handling of non-id, id, *id* info screens:
   If inventory 'slot' isn't used it must be -1. */
bool identify_combo_aux(int Ind, object_type *o_ptr, bool full, int slot, int Ind_target) {
	player_type *p_ptr = Players[Ind];
	player_type *pt_ptr = (Ind_target ? Players[Ind_target] : p_ptr);
	bool id = full || object_known_p(Ind, o_ptr); /* item has undergone basic ID (or is easy-know and basic)? */
	bool can_have_hidden_powers = FALSE, eff_full = full;
	ego_item_type *e_ptr;
	bool aware = object_aware_p(Ind, o_ptr) || full;
	bool aware_cursed = id || (o_ptr->ident & ID_SENSE);
	object_type forge;
	bool am_unknown = FALSE;
#endif
	int j, k, am;
	u32b f1 = 0, f2 = 0, f3 = 0, f4 = 0, f5 = 0, f6 = 0, esp = 0;
	FILE *fff;
	char buf[1024], o_name[ONAME_LEN];
	char *ca_ptr = "", a = (id && artifact_p(o_ptr)) ? 'U' : 'w';
	char buf_tmp[90];
	int buf_tmp_i, buf_tmp_n;
	char timeleft[51] = { 0 };//[26]


	/* Open a new file */
	fff = my_fopen(p_ptr->infofile, "wb");

	/* Current file viewing */
	strcpy(p_ptr->cur_file, p_ptr->infofile);

	/* Let the player scroll through this info */
	p_ptr->special_file_type = TRUE;

	/* Admins can peek inside gifts */
	if (is_admin(p_ptr) && o_ptr->tval == TV_SPECIAL && o_ptr->sval >= SV_GIFT_WRAPPING_START && o_ptr->sval <= SV_GIFT_WRAPPING_END) {
		object_desc(0, o_name, o_ptr, TRUE, 3);
		fprintf(fff, "%s\n", o_name);
#ifdef KIND_DIZ
		fprintf(fff, "%s", k_text + k_info[o_ptr->k_idx].text);
#endif

		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
		if (f3 & TR3_ACTIVATE) {
			/* TODO maybe: Some of the strings in item_activation() are rendered via format() and hence not constant! Might need free'ing! */
			cptr activation = item_activation(o_ptr);

			if (!activation) fprintf(fff, "It can be activated.\n");
			else fprintf(fff, "It can be activated for...\n %s.\n", activation);
		}
		f1 = f2 = f3 = f4 = f5 = f6 = esp = 0x0;

		fprintf(fff, "\nIt contains:\n\n");
		/* Let o_ptr point to a new, static object that is a clone of the gift contents */
		peek_gift(o_ptr, &o_ptr);
		slot = -1; /* Not directly located inside our inventory/equipment */
	}

#ifdef NEW_ID_SCREEN
	/* ---------------------------- determine degree of knowledge ------------------------------- */

	/* hack: can *identifying* actually make a difference at all? */
	if (!id || o_ptr->name1) can_have_hidden_powers = TRUE;

	/* Item is *identified*? */
	if (full) {
		bool aware_tmp = object_aware_p(Ind, o_ptr);

		/* Extract the flags */
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

		/* hack for inspecting items in stores, which are supposed to
		   be fully identified even though we aren't aware of them yet */
		p_ptr->obj_aware[o_ptr->k_idx] = TRUE;

		/* Describe the result */
		/* in case we just *ID* it because an admin inspected it */
		if (!(o_ptr->ident & ID_MENTAL)) object_desc(0, o_name, o_ptr, TRUE, 3);
		/* normal players: */
		else object_desc(Ind, o_name, o_ptr, TRUE, 3);

		/* unhack */
		p_ptr->obj_aware[o_ptr->k_idx] = aware_tmp;

		/* create virtual object here too, just because we're lazy */
		object_copy(&forge, o_ptr);
	} else {
		/* Just assume basic fixed flags */
		if (aware) {
			f1 = k_info[o_ptr->k_idx].flags1;
			f2 = k_info[o_ptr->k_idx].flags2;
			f3 = k_info[o_ptr->k_idx].flags3;
			f4 = k_info[o_ptr->k_idx].flags4;
			f5 = k_info[o_ptr->k_idx].flags5;
			f6 = k_info[o_ptr->k_idx].flags6;
			esp = k_info[o_ptr->k_idx].esp;

			/* hack: granted pval-abilities */
			if (o_ptr->tval == TV_MSTAFF && o_ptr->pval) f1 |= TR1_MANA;
			if (o_ptr->tval == TV_SWORD && o_ptr->sval == SV_DARK_SWORD) am_unknown = TRUE;
		}

		/* Assume we must *id* (just once) to learn sigil powers - Kurzel */
		if (o_ptr->sigil) can_have_hidden_powers = TRUE;

		/* Multi-Hued DSMs need *id* to reveal their immunities */
		if (o_ptr->tval == TV_DRAG_ARMOR && o_ptr->sval == SV_DRAGON_MULTIHUED) can_have_hidden_powers = TRUE;

		/* Just add the fixed ego flags that we know to be on the item */
		if (id) {
			if (o_ptr->name2) {
				e_ptr = &e_info[o_ptr->name2];
				for (j = 0; j < 5; j++) {
					if (e_ptr->rar[j] == 0) continue;
					/* hack: can *identifying* actually make a difference at all? */

					/* any non-trivial (on the object name itself visible) abilities? */
					if ((e_ptr->fego1[j] & ETR1_EASYKNOW_MASK) ||
					    (e_ptr->fego2[j] & ETR2_EASYKNOW_MASK) ||
					    /* random ego mods (R_xxx)? */
					    (e_ptr->esp[j] & ESP_R_MASK)) {
						can_have_hidden_powers = TRUE;
					}

					/* random base mods? */
					if (e_ptr->rar[j] != 100) {
						if (e_ptr->flags1[j] | e_ptr->flags2[j] | e_ptr->flags3[j] |
						    e_ptr->flags4[j] | e_ptr->flags5[j] | e_ptr->flags6[j] |
						    e_ptr->esp[j]) {
							can_have_hidden_powers = TRUE;
							continue;
						}
					}

					/* fixed base mods, ie which we absolutely know will be on the item even without *id*ing */
					f1 |= e_ptr->flags1[j];
					f2 |= e_ptr->flags2[j];
					f3 |= e_ptr->flags3[j];
					f4 |= e_ptr->flags4[j];
					f5 |= e_ptr->flags5[j];
					f6 |= e_ptr->flags6[j];
					esp |= e_ptr->esp[j]; /* & ~ESP_R_MASK -- not required */
				}
			}
			if (o_ptr->name2b) {
				e_ptr = &e_info[o_ptr->name2b];
				for (j = 0; j < 5; j++) {
					if (e_ptr->rar[j] == 0) continue;
					/* hack: can *identifying* actually make a difference at all? */

					/* any non-trivial (on the object name itself visible) abilities? */
					if ((e_ptr->fego1[j] & ETR1_EASYKNOW_MASK) ||
					    (e_ptr->fego2[j] & ETR2_EASYKNOW_MASK) ||
					    /* random ego mods (R_xxx)? */
					    (e_ptr->esp[j] & ESP_R_MASK)) {
						can_have_hidden_powers = TRUE;
					}

					/* random base mods? */
					if (e_ptr->rar[j] != 100) {
						if (e_ptr->flags1[j] | e_ptr->flags2[j] | e_ptr->flags3[j] |
						    e_ptr->flags4[j] | e_ptr->flags5[j] | e_ptr->flags6[j] |
						    e_ptr->esp[j]) {
							can_have_hidden_powers = TRUE;
							continue;
						}
					}

					/* fixed base mods, ie which we absolutely know will be on the item even without *id*ing */
					f1 |= e_ptr->flags1[j];
					f2 |= e_ptr->flags2[j];
					f3 |= e_ptr->flags3[j];
					f4 |= e_ptr->flags4[j];
					f5 |= e_ptr->flags5[j];
					f6 |= e_ptr->flags6[j];
					esp |= e_ptr->esp[j]; /* & ~ESP_R_MASK -- not required */
				}
			}
		}

		/* Get the item name we know */
		object_desc(Ind, o_name, o_ptr, TRUE, 3);

		/* create virtual object with those flags that are certain */
		object_copy(&forge, o_ptr);
		forge.name1 = forge.name2 = forge.name2b = 0;
		/* not for artifacts */
		if (!o_ptr->name1) {
			e_ptr = &e_info[EGO_TEMPORARY];
			forge.name2 = EGO_TEMPORARY;
			e_ptr->rar[0] = 100;
			e_ptr->flags1[0] = f1; /* need f1 for determining encumberment (+stats) */
			e_ptr->flags2[0] = f2;
			e_ptr->flags3[0] = f3; /* need f3 for determining encumberment (BLESSED) */
			e_ptr->flags4[0] = f4;
			e_ptr->flags5[0] = f5;
			e_ptr->flags6[0] = f6;
			e_ptr->esp[0] = esp;
		}
	}

	/* conclude hack: can *identifying* actually make a difference at all? */
	if (!can_have_hidden_powers) eff_full = TRUE;

	/* ------------------------------------------------------------------------------------------ */
#endif


#ifdef USE_SOUND_2010
	sound_item(Ind, o_ptr->tval, o_ptr->sval, "pickup_");
#endif

#ifdef PLAYER_STORES
	if (pt_ptr->store_num <= -2 && o_ptr->note) player_stores_cut_inscription(o_name);

	/* Hack for cheques - display special name and info instead */
	if (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_CHEQUE) {
		fprintf(fff, "\377sIt's a cheque worth \377y%d\377s gold pieces.\n", ps_get_cheque_value(o_ptr));
 #ifdef KIND_DIZ
		fprintf(fff, "%s", k_text + k_info[o_ptr->k_idx].text);
 #endif

		/* Ooook, in the rare case that someone... Questor object! */
		if (o_ptr->questor) quest_interact(Ind, o_ptr->quest - 1, o_ptr->questor_idx, fff);

		my_fclose(fff);

		/* Let the client know it's about to get some info */
		strcpy(p_ptr->cur_file_title, "Cheque Details");
		Send_special_other(Ind);

		/* Gave knowledge */
		return(TRUE);
	}
#endif

	/* Hack for seals and wrapped gifts */
	if (o_ptr->tval == TV_SPECIAL && (o_ptr->sval == SV_SEAL || (o_ptr->sval >= SV_GIFT_WRAPPING_START && o_ptr->sval <= SV_GIFT_WRAPPING_END))) {
		fprintf(fff, "%s\n", o_name);
 #ifdef KIND_DIZ
		fprintf(fff, "%s", k_text + k_info[o_ptr->k_idx].text);
 #endif

		if (f3 & TR3_ACTIVATE) {
			/* TODO maybe: Some of the strings in item_activation() are rendered via format() and hence not constant! Might need free'ing! */
			cptr activation = item_activation(o_ptr);

			if (!activation) {
				/* Mysterious message for items missing description (eg. golem command scrolls) - mikaelh */
				if (wearable_p(o_ptr)) fprintf(fff, "When equipped, it can be activated.\n");
				else fprintf(fff, "It can be activated.\n");
			} else {
				if (wearable_p(o_ptr)) fprintf(fff, "When equipped, it can be activated for...\n");
				else fprintf(fff, "It can be activated for...\n");
				fprintf(fff, " %s.\n", activation);
			}
		}
		my_fclose(fff);
		Send_special_other(Ind);
		return(TRUE);
	}

	/* Start with the object's name in the first line of the item description */
	if (strlen(o_name) > 79) {
		strncpy(buf_tmp, o_name, 79);
		buf_tmp[79] = 0;
		fprintf(fff, "\377%c%s\n", a, buf_tmp);
		fprintf(fff, "\377%c%s\377w\n", a, o_name + 79);
	} else {
		fprintf(fff, "\377%c%s\377w\n", a, o_name);
	}

#ifdef ART_DIZ
	if (true_artifact_p(o_ptr)
 #ifdef NEW_ID_SCREEN
	    && full
 #endif
	    )
		if (a_info[o_ptr->name1].text)
			fprintf(fff, "%s", a_text + a_info[o_ptr->name1].text);
#endif

#ifdef KIND_DIZ
 #ifdef NEW_ID_SCREEN
	if (aware)
 #endif
	{
		/* Custom objects can emulate a k-diz via LUA function */
		if (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT && o_ptr->xtra5)
			fprintf(fff, "%s", string_exec_lua(0, format("return custom_object_diz(%d)", o_ptr->xtra5)));
		else

		if (k_info[o_ptr->k_idx].text)
			fprintf(fff, "%s", k_text + k_info[o_ptr->k_idx].text);
 #ifdef EGO_DIZ
		if (o_ptr->name2 && e_info[o_ptr->name2].text)
			fprintf(fff, "%s", e_text + e_info[o_ptr->name2].text);
		if (o_ptr->name2b && e_info[o_ptr->name2b].text)
			fprintf(fff, "%s", e_text + e_info[o_ptr->name2b].text);
 #endif
	}
#endif

#if STARTEQ_TREATMENT >= 2
	if (o_ptr->mode & MODE_STARTER_ITEM) {
 #if STARTEQ_TREATMENT == 3
		/* Cannot sell any starter items, including own ones */
		fprintf(fff, "\377sThis is a starter item and therefore cannot be sold to shops.\n");
 #else
		/* Prevent suicide-item-accumulation-cheeze, but allow selling own level 0 rewards (that weren't 100% discounted) */
		if (o_ptr->owner != p_ptr->id) fprintf(fff, "\377sThis is a starter item. It can therefore only be sold by its owner.\n");
 #endif
	}
#endif

#ifdef SUBINVEN_LIMIT_GROUP
	if (o_ptr->tval == TV_SUBINVEN) fprintf(fff, "\377WYou cannot use more than one of this type of container at a time.\n");
#endif

	/* Questor object! */
	if (o_ptr->questor) quest_interact(Ind, o_ptr->quest - 1, o_ptr->questor_idx, fff);

	/* Sigil */
	if (o_ptr->sigil) fprintf(fff, "\377BIt is emblazoned with a sigil of %s.\n", string_exec_lua(0, format("return rcraft_name(%d)", o_ptr->sigil)));

#ifdef ENABLE_DEMOLITIONIST
	if (o_ptr->tval == TV_CHARGE) fprintf(fff, "\377s  Its default fuse length will burn down in %d seconds.\n", o_ptr->pval);
#endif

#ifdef NEW_ID_SCREEN
 #ifdef ENABLE_SUBINVEN
	if (slot < SUBINVEN_INVEN_MUL)
 #endif
	/* Temporary brands -- kinda hacky that they use p_ptr instead of o_ptr.. */
	if (pt_ptr->melee_brand && !pt_ptr->melee_brand_ma && is_melee_weapon(o_ptr->tval) && (slot == INVEN_WIELD || slot == INVEN_ARM))
		switch (pt_ptr->melee_brand_t) {
		case TBRAND_ELEC:
			fprintf(fff, "\377BLightning charge has been applied to it temporarily.\n");
			break;
		case TBRAND_COLD:
			fprintf(fff, "\377BFrost brand has been applied to it temporarily.\n");
			break;
		case TBRAND_ACID:
			fprintf(fff, "\377BAcid cover has been applied to it temporarily.\n");
			break;
		case TBRAND_FIRE:
			fprintf(fff, "\377BFire brand has been applied to it temporarily.\n");
			break;
		case TBRAND_POIS:
			fprintf(fff, "\377BVenom has been applied to it temporarily.\n");
			break;
		case TBRAND_HELLFIRE:
			fprintf(fff, "\377BHellfire brand has been applied to it temporarily.\n");
			break;
		case TBRAND_VAMPIRIC:
			fprintf(fff, "\377BVampirism brand has been applied to it temporarily.\n");
			break;
		//other brands are unused atm (possibly not fully implemented even)
		}
	//Note: ammo_brand_t is unused atm (possibly not fully implemented even)
	/* Note: Static brands (p_ptr->brand_..) aren't displayed here, since they come completely independant of the weapon usage,
		 while temporary brands at least (usually) stop when you take off the weapon. */
#endif

	/* in case we just *ID* it because an admin inspected it */
	if (is_admin(p_ptr) && maybe_hidden_powers(0, o_ptr, FALSE)
#ifdef NEW_ID_SCREEN
	    && full
#endif
	    )
		fprintf(fff, "\377y(This item has not been *identified* yet.)\n");

	if (artifact_p(o_ptr)
#ifdef NEW_ID_SCREEN
	    && id
#endif
	    ) {
#ifdef NEW_ID_SCREEN
		if (full || is_admin(p_ptr)) {
#endif
			if (true_artifact_p(o_ptr)) {
#ifdef FLUENT_ARTIFACT_RESETS
				int timeout = ((FALSE
 #ifdef IDDC_ARTIFACT_FAST_TIMEOUT
				    || a_info[o_ptr->name1].iddc
 #endif
 #ifdef WINNER_ARTIFACT_FAST_TIMEOUT
				    || a_info[o_ptr->name1].winner
 #endif
				     ) && a_info[o_ptr->name1].timeout > 0) ? a_info[o_ptr->name1].timeout / 2 : a_info[o_ptr->name1].timeout;

 #ifdef RING_OF_PHASING_NO_TIMEOUT
				if (o_ptr->name1 == ART_PHASING) strcpy(timeleft, " (It will reset when Zu-Aon is defeated once more)");
				else
 #endif
				if (timeout <= 0 || cfg.persistent_artifacts) ;
				else if (timeout < 60 * 2) sprintf(timeleft, " (\377r%d minutes\377%c till reset)", timeout, a);
				else if (timeout < 60 * 24 * 2) sprintf(timeleft, " (\377y%d hours\377%c till reset)", timeout / 60, a);
				else sprintf(timeleft, " (%d days till reset)", timeout / 60 / 24);
#endif
				ca_ptr = " true artifact";
			} else {
				if (!is_admin(p_ptr)) ca_ptr = " random artifact";
				else ca_ptr = format(" random artifact (ap = %d, %d Au)", artifact_power(randart_make(o_ptr)), object_value_real(0, o_ptr));
			}
#ifdef NEW_ID_SCREEN
		} else ca_ptr = "n artifact";
#endif
	}

	switch (o_ptr->tval) {
	case TV_BLUNT:
		fprintf(fff, "\377%cIt's a%s blunt weapon%s.\377w\n", a, ca_ptr, timeleft); break;
	case TV_POLEARM:
		fprintf(fff, "\377%cIt's a%s polearm%s.\377w\n", a, ca_ptr, timeleft); break;
	case TV_SWORD:
		fprintf(fff, "\377%cIt's a%s sword-type weapon%s.\377w\n", a, ca_ptr, timeleft); break;
	case TV_AXE:
		fprintf(fff, "\377%cIt's a%s axe-type weapon%s.\377w\n", a, ca_ptr, timeleft); break;
	default:
		if (artifact_p(o_ptr)
#ifdef NEW_ID_SCREEN
		    && id
#endif
		    )
			fprintf(fff, "\377%cIt's a%s%s.\377w\n", a, ca_ptr, timeleft);
		break;
	}

	if (f4 & TR4_SHOULD2H) fprintf(fff, "It can be wielded one-handed, but should be wielded two-handed.\n");
	else if (f4 & TR4_MUST2H) fprintf(fff, "It must be wielded two-handed.\n");
	else if (f4 & TR4_COULD2H) {
		if (o_ptr->weight <= DUAL_MAX_WEIGHT)
	fprintf(fff, "It can be wielded one-handed, two-handed or dual.\n");
		else
	fprintf(fff, "It can be wielded one-handed or two-handed.\n");
	}
	else if (is_melee_weapon(o_ptr->tval) && o_ptr->weight <= DUAL_MAX_WEIGHT) fprintf(fff, "It can be wielded one-handed or dual.\n");
	else if (is_melee_weapon(o_ptr->tval)) fprintf(fff, "It is wielded one-handed.\n");

	if (is_throwing_weapon(o_ptr)) fprintf(fff, "It can be used as an effective throwing weapon.\n");

#ifdef WEAPONS_NO_AC
	/* Optional. Shields don't say block chance, weapons don't say to-hit/to-dam either. These are assumed to be understood. */
	//if (id && is_melee_weapon(o_ptr->tval) && o_ptr->to_a) fprintf(fff, "It increases your chance to parry by %+d%%.\n", o_ptr->to_a);
#endif

	/* Kings/Queens only warning */
	if (f5 & TR5_WINNERS_ONLY) fprintf(fff, "\377vIt is to be used by royalties exclusively.\377w\n");
	/* Morgoth crown hardcoded note to give a warning!- C. Blue */
	else if (o_ptr->name1 == ART_MORGOTH && full) fprintf(fff, "\377vIt may only be worn by kings and queens!\377w\n");

	if (o_ptr->tval == TV_BOW) display_shooter_handling(Ind, &forge, fff, Ind_target);
	else if (is_melee_weapon(o_ptr->tval)) display_weapon_handling(Ind, &forge, fff, Ind_target);
	else if (is_armour(o_ptr->tval)) display_armour_handling(Ind, &forge, fff, Ind_target);
	else if (o_ptr->tval == TV_DIGGING) display_tool_handling(Ind, &forge, fff, Ind_target);

	/* specialty: recognize custom spell books and display their contents! - C. Blue */
	if (o_ptr->tval == TV_BOOK) {
		if (is_custom_tome(o_ptr->sval)) {
			if (!o_ptr->xtra1) fprintf(fff, "It contains no spells yet.\n");
			else fprintf(fff, "It contains spells:\n");
			if (o_ptr->xtra1) fprintf(fff, "- %s\n", string_exec_lua(0, format("return(__tmp_spells[%d].name)", o_ptr->xtra1 - 1)));
			if (o_ptr->xtra2) fprintf(fff, "- %s\n", string_exec_lua(0, format("return(__tmp_spells[%d].name)", o_ptr->xtra2 - 1)));
			if (o_ptr->xtra3) fprintf(fff, "- %s\n", string_exec_lua(0, format("return(__tmp_spells[%d].name)", o_ptr->xtra3 - 1)));
			if (o_ptr->xtra4) fprintf(fff, "- %s\n", string_exec_lua(0, format("return(__tmp_spells[%d].name)", o_ptr->xtra4 - 1)));
			if (o_ptr->xtra5) fprintf(fff, "- %s\n", string_exec_lua(0, format("return(__tmp_spells[%d].name)", o_ptr->xtra5 - 1)));
			if (o_ptr->xtra6) fprintf(fff, "- %s\n", string_exec_lua(0, format("return(__tmp_spells[%d].name)", o_ptr->xtra6 - 1)));
			if (o_ptr->xtra7) fprintf(fff, "- %s\n", string_exec_lua(0, format("return(__tmp_spells[%d].name)", o_ptr->xtra7 - 1)));
			if (o_ptr->xtra8) fprintf(fff, "- %s\n", string_exec_lua(0, format("return(__tmp_spells[%d].name)", o_ptr->xtra8 - 1)));
			if (o_ptr->xtra9) fprintf(fff, "- %s\n", string_exec_lua(0, format("return(__tmp_spells[%d].name)", o_ptr->xtra9 - 1)));
		} else if (o_ptr->sval != SV_SPELLBOOK && is_admin(p_ptr)) { /* Predefined book (handbook or tome) */
			char powers[MSG_LEN], tmp[MAX_CHARS], *pp;

			powers[0] = 0;
			power_inscribe(o_ptr, TRUE, powers); //TRUE:redux
			pp = powers;
			/* split long line */
			do {
				strncpy(tmp, pp, MAX_CHARS - 1);
				if (strlen(pp) < 80) {
					fprintf(fff, " %s\n", tmp);
					break;
				}

				/* keep reducing string char by char until we hit a ',' separator, for a clean cut */
				while (tmp[strlen(tmp) - 1] != ',' && tmp[strlen(tmp) - 1] != 0 && strlen(tmp) >= 60) tmp[strlen(tmp) - 1] = 0;
				/* if we didn't find a separator, just reset to taking the whole line */
				if (strlen(tmp) < 60) strncpy(tmp, pp, MAX_CHARS - 1);
				tmp[MAX_CHARS - 1] = 0;

				fprintf(fff, " %s\n", tmp);
				pp += strlen(tmp);
			} while (TRUE);
		}
	}

#ifdef ENABLE_SUBINVEN
	/* Display special bag contents */
	if (o_ptr->tval == TV_SUBINVEN && slot != -1) {
		object_type *o2_ptr = &pt_ptr->subinventory[slot][0];

		if (!o2_ptr->k_idx) fprintf(fff, "\377WIt can hold up to %d items or stacks and is currently empty.\n", o_ptr->bpval);
		else {
			char o2_name[ONAME_LEN];

			fprintf(fff, "\377WIt can hold up to %d items or stacks and currently contains:\n", o_ptr->bpval);
			for (j = 0; j < o_ptr->bpval; j++) {
				o2_ptr = &pt_ptr->subinventory[slot][j];
				if (!o2_ptr->k_idx) break;
				object_desc(Ind, o2_name, o2_ptr, TRUE, 3);
				fprintf(fff, "\377W %c) %s.\n", index_to_label(j), o2_name);
			}
		}
	}
#endif

	/* Hack: Stop here if the item wasn't *ID*ed and we still were allowed to read so far (player stores maybe?) */
	if (!(o_ptr->ident & ID_MENTAL) && !is_admin(p_ptr)
#ifdef NEW_ID_SCREEN
	    && full
#endif
	    ) {
#if 1 /* display breakage for ammo and trigger chance for magic devices even if not *id*ed? */
		if (eff_full) {
			if (wield_slot(0, o_ptr) == INVEN_AMMO) {
				byte chance, permille, vowel;

				chance = breakage_chance_with_skill(pt_ptr->Ind, o_ptr, &permille);
				vowel = (chance == 8) || (chance == 11) || (chance == 18); /* 0 <= chance <= 20 */
				if (permille == 0) fprintf(fff, "\377WIt has a%s %d%% chance to break upon hit///.\n", vowel ? "n" : "", chance);
				else fprintf(fff, "\377WIt has a%s %d.%d%% chance to break upon hit///.\n", vowel ? "n" : "", chance, permille);
			}

 #if 1 /* display trigger chance for magic devices? */
			if ((is_magic_device(o_ptr->tval) || (f3 & TR3_ACTIVATE))
  #ifdef ENABLE_DEMOLITIONIST
			    && o_ptr->tval != TV_CHEMICAL && o_ptr->tval != TV_CHARGE
  #endif
			    && o_ptr->tval != TV_BOOK) {
				if (!get_skill(pt_ptr, SKILL_ANTIMAGIC)) {
					byte chance, permille;

					chance = activate_magic_device_chance(pt_ptr->Ind, o_ptr, &permille, is_magic_device(o_ptr->tval) && slot == INVEN_WIELD);
					if (chance == 99) fprintf(fff, "\377WYou have a 99.%d%% chance to successfully activate this magic device.\n", permille);
					else fprintf(fff, "\377WYou have a %d%% chance to successfully activate this magic device.\n", chance);
				} else
					fprintf(fff, "\377DAs an unbeliever you cannot activate this magic device.\n");
			}
 #endif
		}
#endif

		if (o_ptr->discount) fprintf(fff, "\377WThe value of this item is %d%% less than usual.\n", o_ptr->discount);

#ifdef PLAYER_STORES
 #ifdef HOME_APPRAISAL
		if ((inside_house(&p_ptr->wpos, p_ptr->px, p_ptr->py) || p_ptr->store_num == STORE_HOME)
		    && !(true_artifact_p(o_ptr) && (cfg.anti_arts_hoard || cfg.anti_arts_house)))
			//if (istownarea(&p_ptr->wpos, MAX_TOWNAREA))
			fprintf(fff, "\377WThis item would be appraised at \377y%ld Au\377W if put up for sale in your store.\n", (long int)price_item_player_store(Ind, o_ptr));
 #endif
#endif

		fprintf(fff, "\n\377y(This item has not been *identified* yet.)\n");
		my_fclose(fff);
		fff = my_fopen(p_ptr->infofile, "rb");
		if (my_fgets(fff, buf, 1024, FALSE)) {
			my_fclose(fff);
			return(FALSE);
		}
		my_fclose(fff);
		strcpy(p_ptr->cur_file_title, "Basic Item Information");
		Send_special_other(Ind);
		return(TRUE);
	}


	/* Mega Hack^3 -- describe the amulet of life saving */
	if (o_ptr->tval == TV_AMULET && o_ptr->sval == SV_AMULET_LIFE_SAVING)
		fprintf(fff, "It will save your life from perilous scene once.\n");

#if 0 /* instead using the 'D:' diz lines in k_info */
	/* Magic device information */
	switch (o_ptr->tval) {
	case TV_WAND:
		break;
	case TV_STAFF:
		break;
	case TV_ROD:
		break;
	}
#endif

	/* Mega-Hack -- describe activation */
	if (f3 & TR3_ACTIVATE) {
		/* TODO maybe: Some of the strings in item_activation() are rendered via format() and hence not constant! Might need free'ing! */
		cptr activation = item_activation(o_ptr);

		if (!activation) {
			/* Mysterious message for items missing description (eg. golem command scrolls) - mikaelh */
			if (wearable_p(o_ptr)) fprintf(fff, "When equipped, it can be activated.\n");
			else fprintf(fff, "It can be activated.\n");
		} else {
			if (wearable_p(o_ptr)) fprintf(fff, "When equipped, it can be activated for...\n");
			else fprintf(fff, "It can be activated for...\n");
			fprintf(fff, " %s.\n", activation);
			//fprintf(fff, "...if it is being worn.\n");
		}
	}

	/* Hack -- describe lite's */
	if (o_ptr->tval == TV_LITE &&
	    //(full || (!o_ptr->name1 && !o_ptr->name2 && !o_ptr->name2b))) {
	    (full || !artifact_p(o_ptr))) {
		int radius = 0;

		if (f3 & TR3_LITE1) radius++;
		if (f4 & TR4_LITE2) radius += 2;
		if (f4 & TR4_LITE3) radius += 3;

		if (radius > LITE_CAP) radius = LITE_CAP; /* LITE_MAX ? */

		//maybe todo: distinguish TR5_WHITE_LIGHT?
		if (f4 & TR4_FUEL_LITE) fprintf(fff, "It provides %slight (radius %d) when fueled.\n", (f5 & TR5_WHITE_LIGHT) ? "white " : "", radius);
		else if (radius) fprintf(fff, "It provides %slight (radius %d) forever.\n", (f5 & TR5_WHITE_LIGHT) ? "white " : "", radius);
		else fprintf(fff, "It never provides light.\n");
	}

	/* Mega Hack^3 -- describe the Anchor of Space-time */
	if (o_ptr->name1 == ART_ANCHOR && full) fprintf(fff, "When activated prevents the space-time continuum from being disrupted.\n");

	am = ((f4 & (TR4_ANTIMAGIC_50)) ? 50 : 0)
	    + ((f4 & (TR4_ANTIMAGIC_30)) ? 30 : 0)
	    + ((f4 & (TR4_ANTIMAGIC_20)) ? 20 : 0)
	    + ((f4 & (TR4_ANTIMAGIC_10)) ? 10 : 0);
	if (am) {
		j = o_ptr->to_h + o_ptr->to_d;// + o_ptr->pval + o_ptr->to_a;
		if (j > 0) am -= j;
		if (am > 50) am = 50;
	}
#ifdef NEW_ANTIMAGIC_RATIO
	am = (am * 3) / 5;
#endif

#ifndef NEW_ID_SCREEN
	if (am > 0) fprintf(fff, "It generates an anti-magic field that has %d%% chance of supressing magic.\n", am);
	else if (am < 0) fprintf(fff, "It generates a suppressed anti-magic field.\n");
#else
	if (full || (!o_ptr->name1 && id)) {
		if (am > 0) fprintf(fff, "It generates an anti-magic field that has %d%% chance of supressing magic.\n", am);
		else if (am < 0) fprintf(fff, "It generates a suppressed anti-magic field.\n");
	}
	else if (am_unknown) fprintf(fff, "It generates an unknown anti-magic field.\n");
#endif

	/* And then describe it fully */

	if (f1 & (TR1_LIFE)) fprintf(fff, "It affects your hit points%s.\n", o_ptr->name1 == ART_RANDART ? " \377v(royalties only)\377w" : "");
	if (f1 & (TR1_STR)) fprintf(fff, "It affects your strength.\n");
	if (f1 & (TR1_INT)) fprintf(fff, "It affects your intelligence.\n");
	if (f1 & (TR1_WIS)) fprintf(fff, "It affects your wisdom.\n");
	if (f1 & (TR1_DEX)) fprintf(fff, "It affects your dexterity.\n");
	if (f1 & (TR1_CON)) fprintf(fff, "It affects your constitution.\n");
	if (f1 & (TR1_CHR)) fprintf(fff, "It affects your charisma.\n");

	if (f1 & (TR1_STEALTH)) {
		if (o_ptr->tval != TV_TRAPKIT) fprintf(fff, "It affects your stealth.\n");
		else fprintf(fff, "It is well-hidden.\n");
	}
#ifdef ART_WITAN_STEALTH
	else if (o_ptr->name1 && o_ptr->tval == TV_BOOTS && o_ptr->sval == SV_PAIR_OF_WITAN_BOOTS) fprintf(fff, "It affects your stealth.\n");
#endif
	if (f1 & (TR1_SEARCH)) fprintf(fff, "It affects your searching.\n");
	if (f5 & (TR5_DISARM)) fprintf(fff, "It affects your disarming.\n");
	if (f1 & (TR1_INFRA)) fprintf(fff, "It affects your infra-vision.\n");
	if (f1 & (TR1_TUNNEL)) fprintf(fff, "It affects your ability to tunnel.\n");
	if (f1 & (TR1_SPEED)) fprintf(fff, "It affects your speed.\n");
	if (f1 & (TR1_BLOWS)) fprintf(fff, "It affects your melee attack speed.\n");
	if (f5 & (TR5_CRIT)) fprintf(fff, "It affects your ability to score critical hits.\n");
	if (f5 & (TR5_LUCK)) fprintf(fff, "It affects your luck.\n");

	if (f1 & (TR1_BRAND_ACID)) fprintf(fff, "It does extra damage from acid.\n");
	if (f1 & (TR1_BRAND_ELEC)) fprintf(fff, "It does extra damage from electricity.\n");
	if (f1 & (TR1_BRAND_FIRE)) fprintf(fff, "It does extra damage from fire.\n");
	if (f1 & (TR1_BRAND_COLD)) fprintf(fff, "It does extra damage from frost.\n");
	if (f1 & (TR1_BRAND_POIS)) fprintf(fff, "It poisons your foes.\n");
	if (f5 & (TR5_CHAOTIC)) fprintf(fff, "It produces chaotic effects.\n");
	if (f1 & (TR1_VAMPIRIC)) fprintf(fff, "It drains life from your foes.\n");
	if (f5 & (TR5_IMPACT)) fprintf(fff, "It can cause earthquakes.\n");
	if (f5 & (TR5_VORPAL)) fprintf(fff, "It is very sharp and can cut your foes.\n");
	/*if (f5 & (TR5_WOUNDING)) fprintf(fff, "It is very sharp and makes your foes bleed.\n");*/
	if (f1 & (TR1_SLAY_ORC)) fprintf(fff, "It is especially deadly against orcs.\n");
	if (f1 & (TR1_SLAY_TROLL)) fprintf(fff, "It is especially deadly against trolls.\n");
	if (f1 & (TR1_SLAY_GIANT)) fprintf(fff, "It is especially deadly against giants.\n");
	if (f1 & (TR1_SLAY_ANIMAL)) fprintf(fff, "It is especially deadly against natural creatures.\n");
	if (f1 & (TR1_KILL_UNDEAD)) fprintf(fff, "It is a great bane of undead.\n");
	else if (f1 & (TR1_SLAY_UNDEAD)) fprintf(fff, "It strikes at undead with holy wrath.\n");
	if (f1 & (TR1_KILL_DEMON)) fprintf(fff, "It is a great bane of demons.\n");
	else if (f1 & (TR1_SLAY_DEMON)) fprintf(fff, "It strikes at demons with holy wrath.\n");
	if (f1 & (TR1_KILL_DRAGON)) fprintf(fff, "It is a great bane of dragons.\n");
	else if (f1 & (TR1_SLAY_DRAGON)) fprintf(fff, "It is especially deadly against dragons.\n");
	if (f1 & (TR1_SLAY_EVIL)) fprintf(fff, "It fights against evil with holy fury.\n");
	if (f1 & (TR1_MANA)) fprintf(fff, "It affects your mana capacity.\n");
	if (f1 & (TR1_SPELL)) fprintf(fff, "It affects your spell power.\n");
	if (f5 & (TR5_INVIS)) fprintf(fff, "It makes you invisible.\n");
	if (o_ptr->tval != TV_TRAPKIT) {
		if (f2 & (TR2_SUST_STR)) fprintf(fff, "It sustains your strength.\n");
		if (f2 & (TR2_SUST_INT)) fprintf(fff, "It sustains your intelligence.\n");
		if (f2 & (TR2_SUST_WIS)) fprintf(fff, "It sustains your wisdom.\n");
		if (f2 & (TR2_SUST_DEX)) fprintf(fff, "It sustains your dexterity.\n");
		if (f2 & (TR2_SUST_CON)) fprintf(fff, "It sustains your constitution.\n");
		if (f2 & (TR2_SUST_CHR)) fprintf(fff, "It sustains your charisma.\n");
		if (f2 & (TR2_IM_FIRE)) fprintf(fff, "It provides \377Uimmunity\377- to fire.\n");
		if (f2 & (TR2_IM_COLD)) fprintf(fff, "It provides \377Uimmunity\377- to cold.\n");
		if (f2 & (TR2_IM_ELEC)) fprintf(fff, "It provides \377Uimmunity\377- to electricity.\n");
		if (f2 & (TR2_IM_ACID)) fprintf(fff, "It provides \377Uimmunity\377- to acid.\n");
	}
#if 1
	else {
		if (f2 & (TRAP2_AUTOMATIC_5)) fprintf(fff, "It can rearm itself.\n");
		if (f2 & (TRAP2_AUTOMATIC_99)) fprintf(fff, "It rearms itself.\n");
		if (f2 & (TRAP2_KILL_GHOST)) fprintf(fff, "It can affect wraithed creatures too.\n");
		if (f2 & (TRAP2_TELEPORT_TO)) fprintf(fff, "It can teleport monsters to you.\n");
		if (f2 & (TRAP2_ONLY_DRAGON)) fprintf(fff, "It can only be set off by dragons.\n");
		if (f2 & (TRAP2_ONLY_DEMON)) fprintf(fff, "It can only be set off by demons.\n");
		if (f2 & (TRAP2_ONLY_UNDEAD)) fprintf(fff, "It can only be set off by undead.\n");
		if (f2 & (TRAP2_ONLY_ANIMAL)) fprintf(fff, "It can only be set off by animals.\n");
		if (f2 & (TRAP2_ONLY_EVIL)) fprintf(fff, "It can only be set off by evil creatures.\n");
	}
#endif
	if (f2 & (TR2_IM_POISON)) fprintf(fff, "It provides \377Uimmunity\377- to poison.\n");

	if (o_ptr->tval == TV_DRAG_ARMOR && o_ptr->sval == SV_DRAGON_MULTIHUED) {
		if (!(f2 & (TR2_IM_FIRE))) {
			if (eff_full && (o_ptr->xtra2 & 0x01)) fprintf(fff, "It provides \377Uimmunity\377- to fire.\n");
			else fprintf(fff, "It provides resistance to fire.\n");
		}
		if (!(f2 & (TR2_IM_COLD))) {
			if (eff_full && (o_ptr->xtra2 & 0x02)) fprintf(fff, "It provides \377Uimmunity\377- to cold.\n");
			else fprintf(fff, "It provides resistance to cold.\n");
		}
		if (!(f2 & (TR2_IM_ELEC))) {
			if (eff_full && (o_ptr->xtra2 & 0x04)) fprintf(fff, "It provides \377Uimmunity\377- to electricity.\n");
			else fprintf(fff, "It provides resistance to electricity.\n");
		}
		if (!(f2 & (TR2_IM_ACID))) {
			if (eff_full && (o_ptr->xtra2 & 0x08)) fprintf(fff, "It provides \377Uimmunity\377- to acid.\n");
			else fprintf(fff, "It provides resistance to acid.\n");
		}
		if (!(f2 & (TR2_IM_POISON))) {
			if (eff_full && (o_ptr->xtra2 & 0x10)) fprintf(fff, "It provides \377Uimmunity\377- to poison.\n");
			else fprintf(fff, "It provides resistance to poison.\n");
		}
	} else {
		/* Note: The immunity checks are for items that get both,
		   resistance AND immunity to the same element. This can
		   happen if art/ego gives IM, while base item type
		   already has RES. */
		if ((f2 & (TR2_RES_FIRE)) && !(f2 & (TR2_IM_FIRE))) fprintf(fff, "It provides resistance to fire.\n");
		if ((f2 & (TR2_RES_COLD)) && !(f2 & (TR2_IM_COLD))) fprintf(fff, "It provides resistance to cold.\n");
		if ((f2 & (TR2_RES_ELEC)) && !(f2 & (TR2_IM_ELEC))) fprintf(fff, "It provides resistance to electricity.\n");
		if ((f2 & (TR2_RES_ACID)) && !(f2 & (TR2_IM_ACID))) fprintf(fff, "It provides resistance to acid.\n");
		if ((f2 & (TR2_RES_POIS)) && !(f2 & (TR2_IM_POISON))) fprintf(fff, "It provides resistance to poison.\n");
	}

	if (f2 & (TR2_IM_WATER)) fprintf(fff, "It provides \377Ucomplete protection\377- from unleashed water.\n");
	else if (f2 & (TR2_RES_WATER)) fprintf(fff, "It provides resistance to unleashed water.\n");
	if (f2 & (TR2_IM_NETHER)) fprintf(fff, "It provides \377Uimmunity\377- to nether.\n");
	else if (f2 & (TR2_RES_NETHER)) fprintf(fff, "It provides resistance to nether.\n");

	if (f2 & (TR2_RES_NEXUS)) fprintf(fff, "It provides resistance to nexus.\n");
	if (f2 & (TR2_RES_CHAOS)) fprintf(fff, "It provides resistance to chaos.\n");
	if (f2 & (TR2_RES_DISEN)) fprintf(fff, "It provides resistance to disenchantment.\n");
	if (f2 & (TR2_RES_SOUND)) fprintf(fff, "It provides resistance to sound.\n");
	if (f2 & (TR2_RES_SHARDS)) fprintf(fff, "It provides resistance to shards.\n");

	if (f5 & (TR5_RES_TIME)) fprintf(fff, "It provides resistance to time.\n");
	if (f5 & (TR5_RES_MANA)) fprintf(fff, "It provides resistance to magical energy.\n");
	if (f5 & TR5_RES_TELE) fprintf(fff, "It provides resistance to teleportation attacks.\n");

	if (f2 & (TR2_RES_LITE)) fprintf(fff, "It provides resistance to light.\n");
	if (f2 & (TR2_RES_DARK)) fprintf(fff, "It provides resistance to dark.\n");
	if (f2 & (TR2_RES_BLIND)) fprintf(fff, "It provides resistance to blindness.\n");

	if (f2 & (TR2_RES_FEAR)) fprintf(fff, "It makes you completely fearless.\n");
	if (f2 & (TR2_FREE_ACT)) fprintf(fff, "It provides immunity to paralysis.\n");
	if (f2 & (TR2_RES_CONF)) fprintf(fff, "It provides resistance to confusion.\n");
	if (f2 & (TR2_HOLD_LIFE)) fprintf(fff, "It provides resistance to life draining attacks.\n");
	if (f3 & (TR3_SEE_INVIS)) fprintf(fff, "It allows you to see invisible monsters.\n");

	if (f3 & (TR3_FEATHER)) fprintf(fff, "It slows down your fall gently.\n");
	if (f4 & (TR4_LEVITATE)) fprintf(fff, "It allows you to levitate.\n");
	if (f5 & (TR5_PASS_WATER)) fprintf(fff, "It allows you to swim easily.\n");
	if (f4 & (TR4_CLIMB)) fprintf(fff, "It allows you to climb high mountains.\n");
	if (f3 & (TR3_WRAITH)) fprintf(fff, "It renders you incorporeal.\n");
	if ((o_ptr->tval != TV_LITE) && ((f3 & (TR3_LITE1)) || (f4 & (TR4_LITE2)) || (f4 & (TR4_LITE3))))
		fprintf(fff, "It provides %slight.\n", (f5 & TR5_WHITE_LIGHT) ? "white " : "");
	if (esp) {
		if (esp & ESP_ALL) fprintf(fff, "It gives telepathic powers.\n");
		else {
			if (esp & ESP_ORC) fprintf(fff, "It allows you to sense the presence of orcs.\n");
			if (esp & ESP_TROLL) fprintf(fff, "It allows you to sense the presence of trolls.\n");
			if (esp & ESP_DRAGON) fprintf(fff, "It allows you to sense the presence of dragons.\n");
			if (esp & ESP_SPIDER) fprintf(fff, "It allows you to sense the presence of spiders.\n");
			if (esp & ESP_GIANT) fprintf(fff, "It allows you to sense the presence of giants.\n");
			if (esp & ESP_DEMON) fprintf(fff, "It allows you to sense the presence of demons.\n");
			if (esp & ESP_UNDEAD) fprintf(fff, "It allows you to sense the presence of undead.\n");
			if (esp & ESP_EVIL) fprintf(fff, "It allows you to sense the presence of evil beings.\n");
			if (esp & ESP_ANIMAL) fprintf(fff, "It allows you to sense the presence of animals.\n");
			if (esp & ESP_DRAGONRIDER) fprintf(fff, "It allows you to sense the presence of dragonriders.\n");
			if (esp & ESP_GOOD) fprintf(fff, "It allows you to sense the presence of good beings.\n");
			if (esp & ESP_NONLIVING) fprintf(fff, "It allows you to sense the presence of non-living things.\n");
			if (esp & ESP_UNIQUE) fprintf(fff, "It allows you to sense the presence of unique beings.\n");
		}
	}
	if (f3 & (TR3_SLOW_DIGEST)) fprintf(fff, "It slows your metabolism.\n");
	if (f3 & (TR3_REGEN)) fprintf(fff, "It speeds your hit point regeneration.\n");
	if (f3 & (TR3_REGEN_MANA)) fprintf(fff, "It speeds your mana recharging.\n");
	if (f5 & (TR5_REFLECT)) fprintf(fff, "It reflects bolts and arrows.\n");
	if (f3 & (TR3_SH_FIRE)) fprintf(fff, "It produces a fiery aura.\n");
	if (f3 & (TR3_SH_COLD)) fprintf(fff, "It produces an icy aura.\n");
	if (f3 & (TR3_SH_ELEC)) fprintf(fff, "It produces an electric aura.\n");
	if (f3 & (TR3_NO_MAGIC)) fprintf(fff, "It produces an anti-magic shell.\n");
	if (f3 & (TR3_BLESSED)) fprintf(fff, "It has been blessed by the gods.\n");
	if (f4 & (TR4_AUTO_ID)) fprintf(fff, "It identifies all items for you.\n");

	if (f3 & (TR3_XTRA_MIGHT)) fprintf(fff, "It fires missiles with extra might.\n");
	if (f3 & (TR3_XTRA_SHOTS)) {
		if (o_ptr->tval == TV_BOOMERANG) fprintf(fff, "It flies excessively fast.\n");
		else fprintf(fff, "It fires missiles excessively fast.\n");
	}

	if (f6 & TR6_RETURNING) fprintf(fff, "It returns to you when thrown while equipped.\n");

	if (f4 & (TR4_EASY_USE)) fprintf(fff, "It is especially easy to activate.\n");
	if (f4 & (TR4_CAPACITY)) fprintf(fff, "It can hold more mana.\n");
	if (f4 & (TR4_CHEAPNESS)) fprintf(fff, "It can cast spells for a lesser mana cost.\n");
	if (f4 & (TR4_FAST_CAST)) fprintf(fff, "It can cast spells faster.\n");
	if (f4 & (TR4_CHARGING)) fprintf(fff, "It regenerates its mana faster.\n");

	if (f3 & (TR3_TELEPORT)) fprintf(fff, "It induces random teleportation.\n");

	/* exploding ammo */
	if (is_ammo(o_ptr->tval) && (o_ptr->pval != 0))
		switch (o_ptr->pval) {
		case GF_ELEC: fprintf(fff, "It explodes with lightning.\n"); break;
		case GF_POIS: fprintf(fff, "It explodes with poison.\n"); break;
		case GF_ACID: fprintf(fff, "It explodes with acid.\n"); break;
		case GF_COLD: fprintf(fff, "It explodes with frost.\n"); break;
		case GF_FIRE: fprintf(fff, "It explodes with fire.\n"); break;
		case GF_PLASMA: fprintf(fff, "It explodes with plasma.\n"); break;
		case GF_LITE: fprintf(fff, "It explodes with bright light.\n"); break;
		case GF_DARK: fprintf(fff, "It explodes with darkness.\n"); break;
		case GF_SHARDS: fprintf(fff, "It explodes with shards.\n"); break;
		case GF_SOUND: fprintf(fff, "It explodes with sound.\n"); break;
		case GF_CONFUSION: fprintf(fff, "It explodes with confusion.\n"); break;
		case GF_FORCE: fprintf(fff, "It explodes with force.\n"); break;
		case GF_INERTIA: fprintf(fff, "It explodes with inertia.\n"); break;
		case GF_MANA: fprintf(fff, "It explodes with mana.\n"); break;
		case GF_METEOR: fprintf(fff, "It explodes with mini-meteors.\n"); break;
		case GF_ICE: fprintf(fff, "It explodes with ice.\n"); break;
		case GF_CHAOS: fprintf(fff, "It explodes with chaos.\n"); break;
		case GF_NETHER: fprintf(fff, "It explodes with nether.\n"); break;
		case GF_NEXUS: fprintf(fff, "It explodes with nexus.\n"); break;
		case GF_TIME: fprintf(fff, "It explodes with time.\n"); break;
		case GF_GRAVITY: fprintf(fff, "It explodes with gravity.\n"); break;
		case GF_KILL_WALL: fprintf(fff, "It explodes with stone-to-mud.\n"); break;
		case GF_AWAY_ALL: fprintf(fff, "It explodes with teleportation.\n"); break;
		case GF_TURN_ALL: fprintf(fff, "It explodes with fear.\n"); break;
		case GF_NUKE: fprintf(fff, "It explodes with radiation.\n"); break;
		case GF_STUN: fprintf(fff, "It explodes with stun.\n"); break;
		case GF_DISINTEGRATE: fprintf(fff, "It explodes with disintegration.\n"); break;
		case GF_HELLFIRE: fprintf(fff, "It explodes with hellfire.\n"); break;
	}

	/* special artifacts hardcoded - C. Blue */
	if (o_ptr->tval == TV_POTION2 && o_ptr->sval == SV_POTION2_AMBER
#ifdef NEW_ID_SCREEN
	    && full
#endif
	    )
		fprintf(fff, "It turns your skin into amber, increasing your powers.\n");
	if (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_SLEEPING
#ifdef NEW_ID_SCREEN
	    && full
#endif
	    )
		fprintf(fff, "It drops a veil of sleep over all your surroundings.\n");

	if (f3 & (TR3_NO_TELE)) fprintf(fff, "\377DIt prevents teleportation.\n");
	if (f5 & (TR5_DRAIN_MANA)) fprintf(fff, "\377DIt drains your magic.\n");

	if (f5 & (TR5_DRAIN_HP)) {
		/* Note: This assumes that there is no possible double-ego power that also has DRAIN_HP besides 'Spectral', otherwise this message might turn out incorrect.. */
		if (pt_ptr->prace == RACE_VAMPIRE && (o_ptr->name2 == EGO_SPECTRAL || o_ptr->name2b == EGO_SPECTRAL))
			fprintf(fff, "\377DIt drains health, but as a true vampire you are unaffected.\n");
		else fprintf(fff, "\377DIt drains your health.\n");
	}

	if (f3 & (TR3_DRAIN_EXP)) fprintf(fff, "\377DIt drains your life force.\n");
	if (f3 & (TR3_AGGRAVATE)) fprintf(fff, "\377DIt aggravates nearby creatures.\n");
	if (f4 & (TR4_NEVER_BLOW)) fprintf(fff, "\377DIt can't attack.\n");
	if (f4 & (TR4_BLACK_BREATH)) fprintf(fff, "\377DIt fills you with the Black Breath.\n");
	if (f4 & (TR4_CLONE)) fprintf(fff, "\377DIt can clone monsters.\n");

	if (cursed_p(o_ptr) && aware_cursed) {
		if (f3 & (TR3_PERMA_CURSE)) fprintf(fff, "\377DIt is permanently cursed.\n");
		else if (f3 & (TR3_HEAVY_CURSE)) fprintf(fff, "\377DIt is heavily cursed.\n");
		else fprintf(fff, "\377DIt is cursed.\n");

		if (f3 & (TR3_TY_CURSE)) fprintf(fff, "\377DIt carries an ancient foul curse.\n");
		if (f4 & (TR4_DG_CURSE)) fprintf(fff, "\377DIt carries an ancient morgothian curse.\n");
		if (f4 & (TR4_CURSE_NO_DROP)) fprintf(fff, "\377DIt cannot be dropped while cursed.\n");
	}
	if (f3 & (TR3_AUTO_CURSE)) fprintf(fff, "\377DIt can re-curse itself.\n");

	/* Stormbringer hardcoded note to give a warning!- C. Blue */
	if (o_ptr->name2 == EGO_STORMBRINGER) fprintf(fff, "\377DIt's possessed by mad wrath!\n");

	/* also show anti-undead/demon life drain */
	j = anti_undead(o_ptr, pt_ptr);
	k = anti_demon(o_ptr, pt_ptr);
	am = wield_slot(Ind, o_ptr); //abuse am
	am = (am == INVEN_HEAD || am == INVEN_HANDS);
	if (j >= k) switch (j) {
		case 1: fprintf(fff, "\377oIts power is adverse to undead, draining your %s.\n", am ? "mana" : "health"); break;
		case 2: //fprintf(fff, "\377oIts power is adverse to undead, preventing your %s from regenerating.\n", am ? "mana" : "health"); break;
			fprintf(fff, "\377oIts power is adverse to undead, hampering your %s regeneration.\n", am ? "mana" : "health"); break;
		}
	else switch (k) {
		case 1: fprintf(fff, "\377oIts power is adverse to demons, draining your %s.\n", am ? "mana" : "health"); break;
		case 2: //fprintf(fff, "\377oIts power is adverse to demons, preventing your %s from regenerating.\n", am ? "mana" : "health"); break;
			fprintf(fff, "\377oIts power is adverse to demons, hampering your %s regeneration.\n", am ? "mana" : "health"); break;
	}

	/* magically returning ranged weapon? */
	if (o_ptr->tval == TV_BOOMERANG && o_ptr->name1) fprintf(fff, "\377WIt always returns to your quiver.\n");
	else if (is_ammo(o_ptr->tval)) {
		if (o_ptr->name2 == EGO_ETHEREAL || o_ptr->name2b == EGO_ETHEREAL) fprintf(fff, "\377WIt magically returns to your quiver most of the time.\n");
		else if (o_ptr->sval == SV_AMMO_MAGIC || o_ptr->name1) fprintf(fff, "\377WIt always magically returns to your quiver.\n");
	}

	if (((f5 & TR5_NO_ENCHANT) || o_ptr->name1) && is_enchantable(o_ptr))
		fprintf(fff, "\377WIt cannot be enchanted by any means.\n");

	strcpy(buf_tmp, "\377WUnaffected by ");
	buf_tmp_i = buf_tmp_n = 0;
	if (f3 & (TR3_IGNORE_FIRE)) buf_tmp_n++;
	if (f3 & (TR3_IGNORE_COLD)) buf_tmp_n++;
	if (f3 & (TR3_IGNORE_ELEC)) buf_tmp_n++;
	if (f3 & (TR3_IGNORE_ACID)) buf_tmp_n++;
	if (f5 & (TR5_IGNORE_WATER)) buf_tmp_n++;
	if (f5 & (TR5_IGNORE_MANA)) buf_tmp_n++;
	if (f5 & (TR5_IGNORE_DISEN)) buf_tmp_n++;
	if (f3 & (TR3_IGNORE_FIRE)) {
		strcat(buf_tmp, "fire");
		buf_tmp_i++;
	}
	if (f3 & (TR3_IGNORE_COLD)) {
		if (buf_tmp_i) strcat(buf_tmp, buf_tmp_i == buf_tmp_n - 1 ? " and " : ", ");
		strcat(buf_tmp, "cold");
		buf_tmp_i++;
	}
	if (f3 & (TR3_IGNORE_ELEC)) {
		if (buf_tmp_i) strcat(buf_tmp, buf_tmp_i == buf_tmp_n - 1 ? " and " : ", ");
		strcat(buf_tmp, "lightning");
		buf_tmp_i++;
	}
	if (f3 & (TR3_IGNORE_ACID)) {
		if (buf_tmp_i) strcat(buf_tmp, buf_tmp_i == buf_tmp_n - 1 ? " and " : ", ");
		strcat(buf_tmp, "acid");
		buf_tmp_i++;
	}
	if (f5 & (TR5_IGNORE_WATER)) {
		if (buf_tmp_i) strcat(buf_tmp, buf_tmp_i == buf_tmp_n - 1 ? " and " : ", ");
		strcat(buf_tmp, "water");
		buf_tmp_i++;
	}
	if (f5 & (TR5_IGNORE_MANA)) {
		if (buf_tmp_i) strcat(buf_tmp, buf_tmp_i == buf_tmp_n - 1 ? " and " : ", ");
		strcat(buf_tmp, "mana");
		buf_tmp_i++;
	}
	if (f5 & (TR5_IGNORE_DISEN)) {
		if (buf_tmp_i) strcat(buf_tmp, buf_tmp_i == buf_tmp_n - 1 ? " and " : ", ");
		strcat(buf_tmp, "disenchantment");
		buf_tmp_i++;
	}
	if (buf_tmp_n) fprintf(fff, "%s.\n", buf_tmp);

	/* Damage display for weapons */
	if (wield_slot(0, o_ptr) == INVEN_WIELD
#ifdef EQUIPPABLE_DIGGERS
	    || o_ptr->tval == TV_DIGGING
#endif
	    )
		/* TODO: 3rd party observe via Ind_target */
		display_weapon_damage(Ind_target ? Ind_target : Ind, &forge, fff, f1);

	/* Damage display for ranged weapons */
	if (wield_slot(0, o_ptr) == INVEN_BOW) {
		if (o_ptr->tval == TV_BOW) {
			u32b ammo_f1 = 0, dummy;
			object_type *x_ptr = &pt_ptr->inventory[INVEN_AMMO];

			if (x_ptr->k_idx) {
				if ((x_ptr->ident & ID_MENTAL)) {
					object_flags(x_ptr, &ammo_f1, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy);
				} else {
					/* Just assume basic fixed flags */
					ammo_f1 = k_info[o_ptr->k_idx].flags1;
					/* item has undergone basic ID (or is easy-know and basic)? */
					if (object_known_p(Ind, x_ptr)) {
						/* Just add the fixed ego flags that we know to be on the item */
						if (x_ptr->name2) {
							e_ptr = &e_info[x_ptr->name2];
							for (j = 0; j < 5; j++) {
								if (e_ptr->rar[j] != 100) continue;
								ammo_f1 |= e_ptr->flags1[j];
							}
						}
						if (x_ptr->name2b) {
							e_ptr = &e_info[x_ptr->name2b];
							for (j = 0; j < 5; j++) {
								if (e_ptr->rar[j] != 100) continue;
								ammo_f1 |= e_ptr->flags1[j];
							}
						}
					}
				}
			}
			/* TODO: 3rd party observe via Ind_target */
			display_shooter_damage(Ind_target ? Ind_target : Ind, &forge, fff, f1, ammo_f1);
		} else
			/* TODO: 3rd party observe via Ind_target */
			display_boomerang_damage(Ind_target ? Ind_target : Ind, &forge, fff, f1);
	}

	/* Breakage/Damage display for ammo */
	if (eff_full && wield_slot(0, o_ptr) == INVEN_AMMO) {
		u32b shooter_f1 = 0, dummy;
		object_type *x_ptr = &pt_ptr->inventory[INVEN_BOW];
		byte chance, permille, vowel;

		if (x_ptr->k_idx && x_ptr->tval == TV_BOW &&
		    (((x_ptr->sval == SV_SHORT_BOW || x_ptr->sval == SV_LONG_BOW) && o_ptr->tval == TV_ARROW) ||
		     ((x_ptr->sval == SV_LIGHT_XBOW || x_ptr->sval == SV_HEAVY_XBOW) && o_ptr->tval == TV_BOLT) ||
		     (x_ptr->sval == SV_SLING && o_ptr->tval == TV_SHOT))) {
			if ((x_ptr->ident & ID_MENTAL))
				object_flags(x_ptr, &shooter_f1, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy);
			else {
				/* Just assume basic fixed flags */
				shooter_f1 = k_info[o_ptr->k_idx].flags1;
				/* item has undergone basic ID (or is easy-know and basic)? */
				if (object_known_p(Ind, x_ptr)) {
					/* Just add the fixed ego flags that we know to be on the item */
					if (x_ptr->name2) {
						e_ptr = &e_info[x_ptr->name2];
						for (j = 0; j < 5; j++) {
							if (e_ptr->rar[j] != 100) continue;
							shooter_f1 |= e_ptr->flags1[j];
						}
					}
					if (x_ptr->name2b) {
						e_ptr = &e_info[x_ptr->name2b];
						for (j = 0; j < 5; j++) {
							if (e_ptr->rar[j] != 100) continue;
							shooter_f1 |= e_ptr->flags1[j];
						}
					}
				}
			}
		}

		chance = breakage_chance_with_skill(pt_ptr->Ind, o_ptr, &permille);
		vowel = (chance == 8) || (chance == 11) || (chance == 18);
		if (permille == 0) fprintf(fff, "\377WIt has a%s %d%% chance to break upon hit.\n", vowel ? "n" : "", chance);
		else fprintf(fff, "\377WIt has a%s %d.%d%% chance to break upon hit.\n", vowel ? "n" : "", chance, permille);

		/* TODO: 3rd party observe via Ind_target */
		display_ammo_damage(Ind_target ? Ind_target : Ind, &forge, fff, f1, shooter_f1);
	}

#if 1 /* display trigger chance for magic devices? */
	if ((eff_full && (is_magic_device(o_ptr->tval) || (f3 & TR3_ACTIVATE)))
 #ifdef ENABLE_DEMOLITIONIST
	    && o_ptr->tval != TV_CHEMICAL && o_ptr->tval != TV_CHARGE
 #endif
	    && o_ptr->tval != TV_BOOK) {
		if (!get_skill(pt_ptr, SKILL_ANTIMAGIC)) {
			byte chance, permille;

			chance = activate_magic_device_chance(p_ptr->Ind, o_ptr, &permille, is_magic_device(o_ptr->tval) && slot == INVEN_WIELD);
			if (chance == 99) fprintf(fff, "\377WYou have a 99.%d%% chance to successfully activate this magic device.\n", permille);
			else fprintf(fff, "\377WYou have a %d%% chance to successfully activate this magic device.\n", chance);
		} else
			fprintf(fff, "\377DAs an unbeliever you cannot activate this magic device.\n");
	}
#endif

	if (o_ptr->discount) fprintf(fff, "\377WThe value of this item is %d%% less than usual.\n", o_ptr->discount);

#ifdef PLAYER_STORES
 #ifdef HOME_APPRAISAL
	if ((inside_house(&p_ptr->wpos, p_ptr->px, p_ptr->py) || p_ptr->store_num == STORE_HOME)
	    && !(true_artifact_p(o_ptr) && (cfg.anti_arts_hoard || cfg.anti_arts_house)))
		//if (istownarea(&p_ptr->wpos, MAX_TOWNAREA))
		fprintf(fff, "\377WThis item would be appraised at \377y%ld Au\377W if put up for sale in your store.\n", (long int)price_item_player_store(Ind, o_ptr));
 #endif
#endif

#ifdef NEW_ID_SCREEN
	if (!eff_full) {
		if (!id) {
 #if 0 /* can be incorrect etc! disabled for now */
			/* make message depend on pseudo-id status! */
			if ((o_ptr->ident & ID_SENSE)) {
				int quality;

				if ((o_ptr->ident & ID_SENSE_HEAVY)) quality = pseudo_id_result(o_ptr, TRUE);
				else quality = pseudo_id_result(o_ptr, FALSE);

				if (quality == -1) {
					//fprintf(fff, "\377yIt seems to be cursed.\n"); //cursed/terrible/broken/worthless
				} else if (quality == 0) {
					//no message //(average)
				}


				/* note: some high-cost base items can be excellent without ego power */
				fprintf(fff, "\377yIt seems to be enchanted or have hidden powers.\n"); //good/excellent

				fprintf(fff, "\377yIt seems to have hidden powers.\n"); //special
			} else fprintf(fff, "\377yIt might be enchanted or have hidden powers.\n"); //no pseudo-id
 #else
			/* no message maybe */
			//fprintf(fff, "\377yIt has not been identified yet.\n");
 #endif

 #if 1
			/* maybe at least tell the person that this item needs ID */
			if (!(f3 & TR3_EASY_KNOW)) fprintf(fff, "\377yThis item has not been identified yet.\n");
 #endif
		}
 #if 0 /* looks better if the text is the same for artifacts as it's for ego items, visually =P */
		else if (o_ptr->name1) fprintf(fff, "\377yIt seems to have hidden powers.\n"); //art
 #endif
		else fprintf(fff, "\377yIt may have hidden powers.\n"); //ego
	}
 #if 1
	/* maybe at least tell the person that this item needs ID */
	else if (!id) {
		if (!(f3 & TR3_EASY_KNOW)) fprintf(fff, "\377yThis item has not been identified yet.\n");
	}
 #endif
#endif

	/* Close the file */
	my_fclose(fff);

	/* Hack -- anything written? (rewrite me) */
	fff = my_fopen(p_ptr->infofile, "rb");
	if (my_fgets(fff, buf, 1024, FALSE)) {
		my_fclose(fff);
		return(FALSE);
	}
	my_fclose(fff);

#if 0 /* moved to client-side, clean! */
	/* hack: apply client-side auto-inscriptions */
	if (p_ptr == pt_ptr && !p_ptr->inventory_changes) Send_inventory_revision(Ind);
#endif

	/* Let the client know it's about to get some info */
	strcpy(p_ptr->cur_file_title, "Item Details");
	Send_special_other(Ind);

	/* Gave knowledge */
	return(TRUE);
}



/*
 * Convert an inventory index into a one character label
 * Note that the label does NOT distinguish inven/equip.
 */
s16b index_to_label(int i) {
#ifdef ENABLE_SUBINVEN
	if (i >= SUBINVEN_INVEN_MUL) return(I2A(i % SUBINVEN_INVEN_MUL));
#endif

	/* Indexes for "inven" are easy */
	if (i < INVEN_WIELD) return(I2A(i));

	/* Indexes for "equip" are offset */
	return(I2A(i - INVEN_WIELD));
}


/*
 * Convert a label into the index of an item in the "inven"
 * Return "-1" if the label does not indicate a real item
 */
s16b label_to_inven(int Ind, int c) {
	player_type *p_ptr = Players[Ind];
	int i;

	/* Convert */
	i = (islower(c) ? A2I(c) : -1);

	/* Verify the index */
	if ((i < 0) || (i > INVEN_PACK)) return(-1);

	/* Empty slots can never be chosen */
	if (!p_ptr->inventory[i].k_idx) return(-1);

	/* Return the index */
	return(i);
}


/*
 * Convert a label into the index of a item in the "equip"
 * Return "-1" if the label does not indicate a real item
 */
s16b label_to_equip(int Ind, int c) {
	player_type *p_ptr = Players[Ind];
	int i;

	/* Convert */
	i = (islower(c) ? A2I(c) : -1) + INVEN_WIELD;

	/* Verify the index */
	if ((i < INVEN_WIELD) || (i >= INVEN_TOTAL)) return(-1);

	/* Empty slots can never be chosen */
	if (!p_ptr->inventory[i].k_idx) return(-1);

	/* Return the index */
	return(i);
}



/*
 * Determine which equipment slot (if any) an item likes
 */
s16b wield_slot(int Ind, object_type *o_ptr) {
	player_type *p_ptr = NULL;
	if (Ind) p_ptr = Players[Ind];
	int tval = (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT && (o_ptr->xtra3 & 0x0200)) ? o_ptr->tval2 : o_ptr->tval;

	/* Slot for equipment */
	switch (tval) {
	case TV_DIGGING:
	case TV_TOOL:
		return(INVEN_TOOL);

	case TV_BLUNT:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_AXE:
	case TV_MSTAFF:
		return(INVEN_WIELD);

	case TV_BOW:
	case TV_BOOMERANG:
	case TV_INSTRUMENT:
		return(INVEN_BOW);

	case TV_RING:
		/* Use the right hand first */
		if (Ind && !p_ptr->inventory[INVEN_RIGHT].k_idx) return(INVEN_RIGHT);

		/* Use the left hand for swapping (by default) */
		return(INVEN_LEFT);

	case TV_AMULET:
		return(INVEN_NECK);

	case TV_LITE:
		return(INVEN_LITE);

	case TV_DRAG_ARMOR:
	case TV_HARD_ARMOR:
	case TV_SOFT_ARMOR:
		return(INVEN_BODY);

	case TV_CLOAK:
		return(INVEN_OUTER);

	case TV_SHIELD:
		return(INVEN_ARM);

	case TV_CROWN:
	case TV_HELM:
		return(INVEN_HEAD);

	case TV_GLOVES:
		return(INVEN_HANDS);

	case TV_BOOTS:
		return(INVEN_FEET);

	case TV_SHOT:
		return(INVEN_AMMO);
	case TV_ARROW:
		return(INVEN_AMMO);
	case TV_BOLT:
		return(INVEN_AMMO);

	/* Special hack for custom objects that have wear/wield flag set */
	case TV_SPECIAL:
		if (o_ptr->sval != SV_CUSTOM_OBJECT || !(o_ptr->xtra3 & 0x0100)) return(-1);
		/* Paranoia - check for valid equipment slot */
		if (o_ptr->tval2 < INVEN_WIELD || o_ptr->tval2 > INVEN_TOOL) return(-1);
		/* Equippable special object */
		return(o_ptr->tval2);

#ifdef WIELD_BOOKS
	case TV_BOOK:
		return(INVEN_WIELD);
#endif
#ifdef WIELD_DEVICES
	case TV_WAND:
	case TV_STAFF:
	case TV_ROD:
		return(INVEN_WIELD);
#endif
	}

	/* No slot available */
	return(-1);
}


/*
 * Return a string mentioning how a given item is carried
 */
cptr mention_use(int Ind, int i) {
	player_type *p_ptr = Players[Ind];
	cptr p;

	/* Examine the location */
	switch (i) {
	case INVEN_WIELD: p = "Wielding"; break;
	case INVEN_BOW:   p = "Shooting"; break;
	case INVEN_LEFT:  p = "On left hand"; break;
	case INVEN_RIGHT: p = "On right hand"; break;
	case INVEN_NECK:  p = "Around neck"; break;
	case INVEN_LITE:  p = "Light source"; break;
	case INVEN_BODY:  p = "On body"; break;
	case INVEN_OUTER: p = "About body"; break;
	case INVEN_ARM:   p = "On off-hand"; break; //was 'On arm'
	case INVEN_HEAD:  p = "On head"; break;
	case INVEN_HANDS: p = "On hands"; break;
	case INVEN_FEET:  p = "On feet"; break;
	default:	  p = "In pack"; break;
	}

	/* Hack -- Heavy weapon */
	if (i == INVEN_WIELD || (i == INVEN_ARM && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)) /* dual-wield, not that needed here though */ {
		object_type *o_ptr;
		o_ptr = &p_ptr->inventory[i];
		if (adj_str_hold[p_ptr->stat_ind[A_STR]] < o_ptr->weight / 10) p = "Just lifting";
	}

	/* Hack -- Heavy bow */
	if (i == INVEN_BOW) {
		object_type *o_ptr;
		o_ptr = &p_ptr->inventory[i];
		if (adj_str_hold[p_ptr->stat_ind[A_STR]] < o_ptr->weight / 10) p = "Just holding";
	}

	/* Return the result */
	return(p);
}


/*
 * Return a string describing how a given item is being worn.
 * Currently, only used for items in the equipment, not inventory.
 */
cptr describe_use(int Ind, int i) {
	player_type *p_ptr = Players[Ind];
	cptr p;

	switch (i) {
	/* you attack players too, none? */
	//case INVEN_WIELD: p = "attacking monsters with"; break;
	case INVEN_WIELD: p = "attacking enemies with"; break;
	case INVEN_BOW:   p = "shooting missiles with"; break;
	case INVEN_LEFT:  p = "wearing on your left hand"; break;
	case INVEN_RIGHT: p = "wearing on your right hand"; break;
	case INVEN_NECK:  p = "wearing around your neck"; break;
	case INVEN_LITE:  p = "using to light the way"; break;
	case INVEN_BODY:  p = "wearing on your body"; break;
	case INVEN_OUTER: p = "wearing on your back"; break;
	case INVEN_ARM:   p = "wielding in off-hand"; break;//was "wearing on your arm"
	case INVEN_HEAD:  p = "wearing on your head"; break;
	case INVEN_HANDS: p = "wearing on your hands"; break;
	case INVEN_FEET:  p = "wearing on your feet"; break;
	default:	  p = "carrying in your pack"; break;
	}

	/* Hack -- Heavy weapon */
	if (i == INVEN_WIELD || (i == INVEN_ARM && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)) /* dual-wield, not that needed here though */ {
		object_type *o_ptr;
		o_ptr = &p_ptr->inventory[i];
		if (adj_str_hold[p_ptr->stat_ind[A_STR]] < o_ptr->weight / 10) p = "just lifting";
	}

	/* Hack -- Heavy bow */
	if (i == INVEN_BOW) {
		object_type *o_ptr;
		o_ptr = &p_ptr->inventory[i];
		if (adj_str_hold[p_ptr->stat_ind[A_STR]] < o_ptr->weight / 10) p = "just holding";
	}

	/* Return the result */
	return(p);
}





/*
 * Check an item against the item tester info
 */
bool item_tester_okay(object_type *o_ptr) {
	/* Hack -- allow listing empty slots */
	if (item_tester_full) return(TRUE);

	/* Require an item */
	if (!o_ptr->k_idx) return(FALSE);

	/* Hack -- ignore "gold" */
	if (o_ptr->tval == TV_GOLD) return(FALSE);

	/* Check the tval */
	if (item_tester_tval) {
		if (!(item_tester_tval == o_ptr->tval)) return(FALSE);
	}

	/* Check the hook */
	if (item_tester_hook) {
		if (!(*item_tester_hook)(o_ptr)) return(FALSE);
	}

	/* Assume okay */
	return(TRUE);
}



/*
 * Choice window "shadow" of the "show_inven()" function
 *
 * Note that this function simply sends a text string describing the
 * inventory slot, along with the tval, weight, and position in the inventory
 * to the client --KLJ--
 */
void display_inven(int Ind) {
	player_type	*p_ptr = Players[Ind];
	register	int i, z = 0;//n
	object_type	*o_ptr;
	byte		attr = TERM_WHITE;
	char		tmp_val[80];
	char		o_name[ONAME_LEN];
	int		wgt;


	/* Have the final slot be the FINAL slot */
	z = INVEN_WIELD;

	/* Display the pack */
	for (i = 0; i < z; i++) {
		/* Examine the item */
		o_ptr = &p_ptr->inventory[i];

		/* Hack -- Only send changed slots - mikaelh */
		if (memcmp(o_ptr, &p_ptr->inventory_copy[i], sizeof(object_type)) == 0) {
			/* Exactly the same item still in this slot */
			continue;
		}

		/* Update the copy */
		memcpy(&p_ptr->inventory_copy[i], o_ptr, sizeof(object_type));

		/* Start with an empty "index" */
		tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';

		/* Prepare an "index" */
		tmp_val[0] = index_to_label(i);

		/* Obtain an item description */
		if (is_newer_than(&p_ptr->version, 4, 4, 4, 1, 0, 0))
			object_desc(Ind, o_name, o_ptr, TRUE, 4);
		else
			object_desc(Ind, o_name, o_ptr, TRUE, 3);

		/* Obtain the length of the description */
		//n = strlen(o_name);

		/* Get a color */
		if (can_use_admin(Ind, o_ptr)) {
			/* Get a color for a book */
			if (o_ptr->tval == TV_BOOK) attr = get_book_name_color(o_ptr);
			/* all other items */
			else attr = get_attr_from_tval(o_ptr);
		} else attr = TERM_L_DARK; /* unusable item */

		/* You can inscribe with !U to force TERM_L_DARK colouring (more visibility tuning!) */
		if (check_guard_inscription(o_ptr->note, 'U')) attr = TERM_L_DARK;

#ifdef SUBINVEN_LIMIT_GROUP
		if (o_ptr->tval == TV_SUBINVEN && subinven_group_player(Ind, get_subinven_group(o_ptr->sval), i)) attr = TERM_L_DARK;
#endif

		/* Hack -- fake monochrome */
		if (!use_color) attr = TERM_WHITE;

		/* Display the weight if needed */
		wgt = o_ptr->weight; //* o_ptr->number;

		/* Send the info to the client */
		if (is_newer_than(&p_ptr->version, 4, 4, 1, 7, 0, 0)) {
			if ((o_ptr->tval != TV_BOOK || !is_custom_tome(o_ptr->sval)) && (o_ptr->tval != TV_SPECIAL || o_ptr->sval != SV_CUSTOM_OBJECT)) {
				Send_inven(Ind, tmp_val[0], attr, wgt, o_ptr, o_name);
			} else {
				Send_inven_wide(Ind, tmp_val[0], attr, wgt, o_ptr, o_name);
			}
		} else {
			Send_inven(Ind, tmp_val[0], attr, wgt, o_ptr, o_name);
		}
	}

	/* Send the new inventory revision if the inventory has changed - mikaelh */
	if (p_ptr->inventory_changed) {
		Send_inventory_revision(Ind);
		p_ptr->inventory_changed = FALSE;
	}

	for (i = 0; i < z; i++) {
		if (p_ptr->inventory[i].auto_insc) {
			p_ptr->inventory[i].auto_insc = FALSE;
			Send_apply_auto_insc(Ind, i);
		}
	}

#if 0 /* moved to client-side, clean! */
	/* hack: apply client-side auto-inscriptions */
	else if (!p_ptr->inventory_changes) Send_inventory_revision(Ind);
#endif
}

#ifdef ENABLE_SUBINVEN
/* For now very simple */
void display_subinven_aux(int Ind, int islot, int slot) {
	player_type	*p_ptr = Players[Ind];
	object_type	*o_ptr;
	byte		attr = TERM_WHITE;
	char		tmp_val[80];
	char		o_name[ONAME_LEN];
	int		wgt;

	o_ptr = &p_ptr->subinventory[islot][slot];

	/* Start with an empty "index" */
	tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';

	/* Prepare an "index" */
	tmp_val[0] = index_to_label(slot);

	object_desc(Ind, o_name, o_ptr, TRUE, 4);

	/* Get a color */
	if (can_use_admin(Ind, o_ptr)) {
		/* Get a color for a book */
		if (o_ptr->tval == TV_BOOK) attr = get_book_name_color(o_ptr);
		/* all other items */
		else attr = get_attr_from_tval(o_ptr);
	} else attr = TERM_L_DARK; /* unusable item */

	/* You can inscribe with !U to force TERM_L_DARK colouring (more visibility tuning!) */
	if (check_guard_inscription(o_ptr->note, 'U')) attr = TERM_L_DARK;

	/* Hack -- fake monochrome */
	if (!use_color) attr = TERM_WHITE;

	/* Display the weight if needed */
	wgt = o_ptr->weight; //* o_ptr->number;

	/* Send the info to the client */
	Send_subinven(Ind, islot, tmp_val[0], attr, wgt, o_ptr, o_name);
}
/* Send whole subinventory contents of one container */
void display_subinven(int Ind, int islot) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr = &p_ptr->inventory[islot];
	int i;

	/* paranoia */
	if (o_ptr->tval != TV_SUBINVEN) return;

	for (i = 0; i < o_ptr->bpval; i++)
		display_subinven_aux(Ind, islot, i);
}
#endif

/*
 * Choice window "shadow" of the "show_equip()" function
 */
void display_equip(int Ind) {
	player_type	*p_ptr = Players[Ind];
	register	int i;//, n;
	object_type	*o_ptr;
	byte		attr = TERM_WHITE;
	char		tmp_val[80];
	char		o_name[ONAME_LEN + 4]; //+4 is sigil hack, see further below
	int		wgt;

	/* Display the equipment */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
		/* Examine the item */
		o_ptr = &p_ptr->inventory[i];

		/* Hack -- Only send changed slots - mikaelh */
		if (memcmp(o_ptr, &p_ptr->inventory_copy[i], sizeof(object_type)) == 0) {
			/* Exactly the same item still in this slot */
			continue;
		}

		/* Unhack hack to show equip_set[] boni, aka force-update slot */
		o_ptr->temp &= ~0x01;

		/* Update the copy */
		memcpy(&p_ptr->inventory_copy[i], o_ptr, sizeof(object_type));

		/* Start with an empty "index" */
		tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';

		/* Prepare an "index" */
		tmp_val[0] = index_to_label(i);

		/* Obtain an item description */
		object_desc(Ind, o_name, o_ptr, TRUE, 3);

		/* Obtain the length of the description */
		//n = strlen(o_name);

		/* Hack for sigiled items: Extend o_name by 4 spaces to make the client
		   overwrite the old item name properly instead of discounting 4 characters
		   which were taken up by the colour code of the sigil,
		   resulting in the final 4 characters not getting erased.
		   Eg: Uninscribe a sigiled item to cause this problem. */
		if (o_ptr->sigil) strcat(o_name, "    ");
		o_name[ONAME_LEN - 1] = 0; //fix in case our 4 spaces caused the string to exceed ONAME_LEN

		/* Get the color */
		if (o_ptr->tval == TV_BOOK) attr = get_book_name_color(o_ptr); /* WIELD_BOOKS */
		else attr = get_attr_from_tval(o_ptr);

		/* You can inscribe with !U to force TERM_L_DARK colouring (more visibility tuning!) */
		if (check_guard_inscription(o_ptr->note, 'U')) attr = TERM_L_DARK;

		/* Hack -- fake monochrome */
		if (!use_color) attr = TERM_WHITE;

		/* Display the weight (if needed) */
		wgt = o_ptr->weight;// * o_ptr->number;
		//wgt = o_ptr->weight; <- shows wrongly for ammunition!

		/* Send the info off */
		if (((o_ptr->tval != TV_BOOK || !is_custom_tome(o_ptr->sval)) && (o_ptr->tval != TV_SPECIAL || o_ptr->sval != SV_CUSTOM_OBJECT)) || !is_newer_than(&p_ptr->version, 4, 9, 0, 5, 0, 1))
			//Send_equip(Ind, tmp_val[0], attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->pval, o_name);
			Send_equip(Ind, tmp_val[0], attr, wgt, o_ptr, o_name);
		else
			Send_equip_wide(Ind, tmp_val[0], attr, wgt, o_ptr, o_name);
	}

	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
		if (p_ptr->inventory[i].auto_insc) {
			p_ptr->inventory[i].auto_insc = FALSE;
			Send_apply_auto_insc(Ind, i);
		}
	}
}

/* Redisplays all inventory and equipment even if not changed (for mindlinking) */
void display_invenequip(int Ind) {
	player_type	*p_ptr = Players[Ind];
	register	int i;
	object_type	*o_ptr;
	byte		attr = TERM_WHITE;
	char		tmp_val[80];
	char		o_name[ONAME_LEN];
	int		wgt;

	/* Display the pack */
	for (i = 0; i < INVEN_WIELD; i++) {
		/* Examine the item */
		o_ptr = &p_ptr->inventory[i];

		/* Start with an empty "index" */
		tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';

		/* Prepare an "index" */
		tmp_val[0] = index_to_label(i);

		/* Obtain an item description */
		if (is_newer_than(&p_ptr->version, 4, 4, 4, 1, 0, 0))
			object_desc(Ind, o_name, o_ptr, TRUE, 4);
		else
			object_desc(Ind, o_name, o_ptr, TRUE, 3);

		/* Obtain the length of the description */
		//n = strlen(o_name);

		/* Get a color */
		if (can_use_admin(Ind, o_ptr)) {
			/* Get a color for a book */
			if (o_ptr->tval == TV_BOOK) attr = get_book_name_color(o_ptr);
			/* all other items */
			else attr = get_attr_from_tval(o_ptr);
		}
		else attr = TERM_L_DARK;

		/* You can inscribe with !U to force TERM_L_DARK colouring (more visibility tuning!) */
		if (check_guard_inscription(o_ptr->note, 'U')) attr = TERM_L_DARK;

#ifdef SUBINVEN_LIMIT_GROUP
		if (o_ptr->tval == TV_SUBINVEN && subinven_group_player(Ind, get_subinven_group(o_ptr->sval), i)) attr = TERM_L_DARK;
#endif

		/* Hack -- fake monochrome */
		if (!use_color) attr = TERM_WHITE;

		/* Display the weight if needed */
		wgt = o_ptr->weight; //* o_ptr->number;

		/* Send the info to the client */
		if (is_newer_than(&p_ptr->version, 4, 4, 1, 7, 0, 0)) {
			if ((o_ptr->tval != TV_BOOK || !is_custom_tome(o_ptr->sval)) && (o_ptr->tval != TV_SPECIAL || o_ptr->sval != SV_CUSTOM_OBJECT)) {
				Send_inven(Ind, tmp_val[0], attr, wgt, o_ptr, o_name);
			} else {
				Send_inven_wide(Ind, tmp_val[0], attr, wgt, o_ptr, o_name);
			}
		} else {
			Send_inven(Ind, tmp_val[0], attr, wgt, o_ptr, o_name);
		}
	}

	/* Display the equipment */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
		/* Examine the item */
		o_ptr = &p_ptr->inventory[i];

		/* Start with an empty "index" */
		tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';

		/* Prepare an "index" */
		tmp_val[0] = index_to_label(i);

		/* Obtain an item description */
		object_desc(Ind, o_name, o_ptr, TRUE, 3);

		/* Obtain the length of the description */
		//n = strlen(o_name);

		/* Get the color */
		if (o_ptr->tval == TV_BOOK) attr = get_book_name_color(o_ptr); /* WIELD_BOOKS */
		else attr = get_attr_from_tval(o_ptr);

		/* You can inscribe with !U to force TERM_L_DARK colouring (more visibility tuning!) */
		if (check_guard_inscription(o_ptr->note, 'U')) attr = TERM_L_DARK;

		/* Hack -- fake monochrome */
		if (!use_color) attr = TERM_WHITE;

		/* Display the weight (if needed) */
		wgt = o_ptr->weight;// * o_ptr->number;
		//wgt = o_ptr->weight; <- shows wrongly for ammunition!

		/* Send the info off */
		if (((o_ptr->tval != TV_BOOK || !is_custom_tome(o_ptr->sval)) && (o_ptr->tval != TV_SPECIAL || o_ptr->sval != SV_CUSTOM_OBJECT)) || !is_newer_than(&p_ptr->version, 4, 9, 0, 5, 0, 1))
			//Send_equip(Ind, tmp_val[0], attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->pval, o_name);
			Send_equip(Ind, tmp_val[0], attr, wgt, o_ptr, o_name);
		else
			Send_equip_wide(Ind, tmp_val[0], attr, wgt, o_ptr, o_name);
	}
}

/* Computes ammo breakage chance after skill has been factored in. */
int breakage_chance_with_skill(int Ind, object_type *o_ptr, byte *permille) {
	player_type *p_ptr = Players[Ind];
	int base_percentage = breakage_chance(o_ptr);
	int archery = get_archery_skill_from_ammo(o_ptr);
	int j = base_percentage * 100;

	if (wield_slot(0, o_ptr) != INVEN_AMMO) return(-1);

	j = (j * (100 - get_skill_scale(p_ptr, archery, 90))) / 100;
	*permille = (j % 100) / 10;
	return(j / 100);
}

/* Takes an ammo object and returns the relevant skill, without relying
   on what's equipped, since breakage chance doesn't depend on shooter. */
int get_archery_skill_from_ammo(object_type *o_ptr) {
	if (o_ptr->tval == TV_ARROW) return(SKILL_BOW);
	if (o_ptr->tval == TV_BOLT) return(SKILL_XBOW);
	if (o_ptr->tval == TV_SHOT) return(SKILL_SLING);
	else return(-1);
}

/* Attempts to convert owner of item to player, if allowed.
   Returns TRUE if the player can actually use the item
   ie if ownership taking was successful, else FALSE.
   NOTE: DID NOT OWN UNOWNED ITEMS and hence fails for those (in compat_pomode())!
         That was changed now. */
bool can_use(int Ind, object_type *o_ptr) {
	player_type *p_ptr = Players[Ind];

	/* exception for Highlander Tournament amulets */
	if ((o_ptr->tval == TV_AMULET) && (o_ptr->sval == SV_AMULET_HIGHLANDS || o_ptr->sval == SV_AMULET_HIGHLANDS2)) {
		o_ptr->owner = p_ptr->id;
		o_ptr->mode = p_ptr->mode;
		return(TRUE);
	}

	/* Owner always can use */
	if (p_ptr->id == o_ptr->owner || p_ptr->admin_dm) {
		if (true_artifact_p(o_ptr)) a_info[o_ptr->name1].carrier = p_ptr->id;
		return(TRUE);
	}

	if (o_ptr->level < 1 && o_ptr->owner && p_ptr->id != o_ptr->owner && !p_ptr->admin_dm) return(FALSE);

	/* Own unowned items (for stores).
	   Note that CTRL+R -> display_inven() -> if (can_use()) checks will
	   automatically own unowned items this way, that's why the admin_dm
	   is exempt, for when he /wished for items. - C. Blue */
	if (!o_ptr->owner && !p_ptr->admin_dm) {
		o_ptr->owner = p_ptr->id;
		o_ptr->mode = p_ptr->mode;
		if (true_artifact_p(o_ptr)) determine_artifact_timeout(o_ptr->name1, &o_ptr->wpos); /* paranoia? */
	}

	if (compat_pomode(Ind, o_ptr)) return(FALSE);

#ifndef RPG_SERVER
	/* Hack -- convert if available */
	if ((p_ptr->lev >= o_ptr->level || in_irondeepdive(&p_ptr->wpos))
	    && !p_ptr->admin_dm) {
		o_ptr->owner = p_ptr->id;
		if (true_artifact_p(o_ptr)) a_info[o_ptr->name1].carrier = p_ptr->id;
		return(TRUE);
	}
	else return(FALSE);
#else
	if (true_artifact_p(o_ptr)) a_info[o_ptr->name1].carrier = p_ptr->id;
	return(TRUE);
#endif
}

/* Same as can_use() but avoids auto-owning items (to be used for admin characters)
   and doesn't allow admin_dm to return(TRUE) in cases where normal characters would get FALSE. */
bool can_use_admin(int Ind, object_type *o_ptr) {
	player_type *p_ptr = Players[Ind];

	/* exception for Highlander Tournament amulets */
	if ((o_ptr->tval == TV_AMULET) && (o_ptr->sval == SV_AMULET_HIGHLANDS || o_ptr->sval == SV_AMULET_HIGHLANDS2))
		return(TRUE);

	/* Owner always can use */
	if (p_ptr->id == o_ptr->owner) return(TRUE);

	if (o_ptr->level < 1 && o_ptr->owner && p_ptr->id != o_ptr->owner) return(FALSE);

	if (compat_pomode(Ind, o_ptr)) return(FALSE);

#ifndef RPG_SERVER
	if (p_ptr->lev >= o_ptr->level || in_irondeepdive(&p_ptr->wpos)) return(TRUE);
	else return(FALSE);
#else
	return(TRUE);
#endif
}

/* Same as can_use() but also displays status messages. */
bool can_use_verbose(int Ind, object_type *o_ptr) {
	player_type *p_ptr = Players[Ind];

	/* exception for Highlander Tournament amulets */
	if ((o_ptr->tval == TV_AMULET) && (o_ptr->sval == SV_AMULET_HIGHLANDS || o_ptr->sval == SV_AMULET_HIGHLANDS)) {
		o_ptr->owner = p_ptr->id;
		o_ptr->mode = p_ptr->mode;
		return(TRUE);
	}

	/* Owner always can use */
	if (p_ptr->id == o_ptr->owner || p_ptr->admin_dm) return(TRUE);

	if (o_ptr->level < 1 && o_ptr->owner) {
		msg_print(Ind, "You must be the owner in order to use it.");
		return(FALSE);
	}

	if (compat_pomode(Ind, o_ptr)) {
		msg_format(Ind, "You cannot use things that belong to %s players.", compat_pomode(Ind, o_ptr));
		return(FALSE);
	}

#ifndef RPG_SERVER /* hm not sure about this.. */
	/* Hack -- convert if available */
	if ((p_ptr->lev >= o_ptr->level || in_irondeepdive(&p_ptr->wpos))
	    && !p_ptr->admin_dm) {
		if (!o_ptr->owner && true_artifact_p(o_ptr)) determine_artifact_timeout(o_ptr->name1, &o_ptr->wpos); /* paranoia? */
		o_ptr->owner = p_ptr->id;
		return(TRUE);
	} else {
		msg_print(Ind, "Your level is not high enough yet to use this.");
		return(FALSE);
	}
#else
	/* Let's still have this restriction - mikaelh */
	if (o_ptr->level < 1 && o_ptr->owner && p_ptr->id != o_ptr->owner && !p_ptr->admin_dm) {
		msg_print(Ind, "You must be the owner in order to use it.");
		return(FALSE);
	}

	/* we are the new owner */
	if (!o_ptr->owner && true_artifact_p(o_ptr)) determine_artifact_timeout(o_ptr->name1, &o_ptr->wpos); /* paranoia? */
	o_ptr->owner = p_ptr->id;
	o_ptr->mode = p_ptr->mode;

	/* the_sandman: let's turn this off? Party trading is horrible with this one. Plus we
	 * already only allow 1 char each account. */
	return(TRUE);
#endif
}

byte get_book_name_color(object_type *o_ptr) {
	if (o_ptr->sval == SV_SPELLBOOK) { /* Simple spell scrolls */
		return(get_spellbook_name_colour(o_ptr->pval));
	} else if (is_custom_tome(o_ptr->sval)) { /* Custom books */
		/* first annotated spell decides */
		if (o_ptr->xtra1)
			return(get_spellbook_name_colour(o_ptr->xtra1 - 1));
		else
			return(TERM_WHITE); /* unused custom book */
	} else { /* School Tomes -- hard-coding partyyyeh ._. */
		/* priests */
		if ((o_ptr->sval >= 12 && o_ptr->sval <= 15) ||
		    o_ptr->sval == 53 || o_ptr->sval == 56)
			return(TERM_GREEN);
		/* druids */
		else if (o_ptr->sval >= 16 && o_ptr->sval <= 17) return(TERM_L_GREEN);
		/* astral tome */
		else if (o_ptr->sval == 18) return(TERM_ORANGE);
		/* mindcrafters */
		else if (o_ptr->sval >= 19 && o_ptr->sval <= 21) return(TERM_YELLOW);
#ifdef ENABLE_OCCULT
		/* Occult */
 #ifdef ENABLE_OUNLIFE
		else if (o_ptr->sval >= 22 && o_ptr->sval <= 25) return(TERM_BLUE);
 #else
  #ifdef ENABLE_OHERETICISM
		else if (o_ptr->sval >= 22 && o_ptr->sval <= 24) return(TERM_BLUE);
  #else
		else if (o_ptr->sval >= 22 && o_ptr->sval <= 23) return(TERM_BLUE);
  #endif
 #endif
#endif
		/* mages (default) */
		else return(TERM_L_BLUE);
	}
}

byte get_attr_from_tval(object_type *o_ptr) {
	int attr = tval_to_attr[o_ptr->tval];

	/* Gift wrappings: Get the attr from sval */
	if ((o_ptr->tval == TV_SPECIAL || o_ptr->tval == TV_JUNK) && o_ptr->sval >= SV_GIFT_WRAPPING_START && o_ptr->sval <= SV_GIFT_WRAPPING_END) return(k_info[o_ptr->k_idx].d_attr);
#ifdef ENABLE_SUBINVEN
 #ifndef SUBINVEN_UNIFIED_COLOUR
	if (o_ptr->tval == TV_SUBINVEN) switch (get_subinven_group(o_ptr->sval)) {
		case SV_SI_GROUP_CHEST_MIN: return(tval_to_attr[TV_CHEST]);
		case SV_SI_SATCHEL: return(tval_to_attr[TV_CHEMICAL]);
		case SV_SI_TRAPKIT_BAG: return(TERM_BLUE); //rogueish
		case SV_SI_MDEVP_WRAPPING: return(TERM_L_WHITE);
		case SV_SI_POTION_BELT: return(tval_to_attr[TV_POTION]);
	}
 #endif
#endif
#ifdef ENABLE_DEMOLITIONIST
	if (o_ptr->tval == TV_CHEMICAL && o_ptr->sval == SV_MIXTURE)
		return(TERM_L_UMBER);
#endif
#ifdef PLAYER_STORES
	if (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_CHEQUE)
		return(TERM_L_UMBER);
#endif
	if ((o_ptr->tval == TV_SOFT_ARMOR && o_ptr->sval == SV_SHIRT) ||
	    (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT)) {
		if (!o_ptr->xtra1) o_ptr->xtra1 = (s16b)attr;
		return((byte)o_ptr->xtra1);
	}
	if (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_QUEST)
		return((byte)o_ptr->xtra2);
#if 0
	if (o_ptr->tval == TV_JUNK && o_ptr->sval == SV_WOOD_PIECE)
		return(TERM_UMBER);
#endif

	return(attr);
}

/* Returns the colour of a spell book in inventory/store.
   This is the C version to prevent exec_lua(0,..) trouble with 'player' LUA var.
   There is also an unsafe LUA version of this in s_aux.lua. */
byte get_spellbook_name_colour(int pval) {
	/* green for priests */
	if (spell_school[pval] >= SCHOOL_HOFFENSE && spell_school[pval] <= SCHOOL_HSUPPORT) return(TERM_GREEN);
	/* light green for druids */
	if (spell_school[pval] == SCHOOL_DRUID_ARCANE || spell_school[pval] == SCHOOL_DRUID_PHYSICAL) return(TERM_L_GREEN);
	/* orange for astral tome */
	if (spell_school[pval] == SCHOOL_ASTRAL) return(TERM_ORANGE);
	/* yellow for mindcrafters */
	if (spell_school[pval] >= SCHOOL_PPOWER && spell_school[pval] <= SCHOOL_MINTRUSION) return(TERM_YELLOW);
#ifdef ENABLE_OCCULT
	/* blue for Occult */
 #ifdef ENABLE_OUNLIFE
	if (spell_school[pval] >= SCHOOL_OSHADOW && spell_school[pval] <= SCHOOL_OUNLIFE) return(TERM_BLUE);
 #else
  #ifdef ENABLE_OHERETICISM
	if (spell_school[pval] >= SCHOOL_OSHADOW && spell_school[pval] <= SCHOOL_OHERETICISM) return(TERM_BLUE);
  #else
	if (spell_school[pval] >= SCHOOL_OSHADOW && spell_school[pval] <= SCHOOL_OSPIRIT) return(TERM_BLUE);
  #endif
 #endif
#endif
	/* light blue for the rest (istari schools) */
	return(TERM_L_BLUE);
}

/* Search inventory for items inscribed !X to use them to identify a newly picked up object. */
#ifdef ENABLE_XID_SPELL
/* Allow BAGIDENTIFY spell to be used with !X inscription? */
 #define ALLOW_X_BAGID
#endif
void apply_XID(int Ind, object_type *o_ptr, int slot) {
	player_type *p_ptr = Players[Ind];
	object_type *i_ptr;
	int index, tc;
	bool ID_spell1_found = FALSE, ID_spell1a_found = FALSE, ID_spell1b_found = FALSE, ID_spell2_found = FALSE, ID_spell3_found = FALSE, ID_spell4_found = FALSE;
	byte failure = 0x0;
#ifdef ENABLE_SUBINVEN
	int k = -1;
	int j = -1; /* init for silyl compiler warning */
#endif

	/* Look for ID / *ID* (the decadence) scrolls 1st */
	for (index = 0; index < INVEN_PACK; index++) {
		i_ptr = &(p_ptr->inventory[index]);
		if (!i_ptr->k_idx) continue;

		/* Not an ID / *ID* scroll? */
		if (i_ptr->tval != TV_SCROLL || !i_ptr->number ||
		    (i_ptr->sval != SV_SCROLL_IDENTIFY && i_ptr->sval != SV_SCROLL_STAR_IDENTIFY))
			continue;

		if (!can_use(Ind, i_ptr)) continue;

		/* Check if the player does want this feature (!X - for now :) ) */
		if (!(tc = check_guard_inscription(i_ptr->note, 'X'))) continue;
		/* Allow 'treasure class': No flavoured items (2), only flavoured items (1), all items (-1 aka not specified). */
		if (tc == 1 && !object_has_flavor(o_ptr->k_idx)) continue;
		else if (tc == 2 && object_has_flavor(o_ptr->k_idx)) continue;

		/* Can't use them? skip */
		if (p_ptr->blind || no_lite(Ind) || p_ptr->confused) {
			failure |= 0x1;
			break;
		}

		/* Read it later, at a point where we can use p_ptr->command_rep.
		   Even though scrolls always succeed, this is better because of energy management!
		   This way, each carry() will be preceeded by the ID scroll read from the previous
		   carry(), avoiding stacking negative energy. */
		p_ptr->delayed_index = index;
		p_ptr->delayed_spell = -4;
		p_ptr->current_item = slot;

#ifdef XID_REPEAT
		p_ptr->command_rep_active = FALSE;//paranoia
		p_ptr->delayed_index_temp = -1;
#endif
		return;
	}

	/* Check activatable items we have equipped */
	for (index = INVEN_WIELD; index < INVEN_TOTAL; index++) {
		i_ptr = &(p_ptr->inventory[index]);
		if (!i_ptr->k_idx) continue;

		/* hard-coded -- the only artifacts that have ID activation */
		if (i_ptr->name1 != ART_ERIRIL &&
		    i_ptr->name1 != ART_STONE_LORE) continue;

		/* Check if the player does want this feature (!X - for now :) ) */
		if (!(tc = check_guard_inscription(i_ptr->note, 'X'))) continue;
		/* Allow 'treasure class': No flavoured items (2), only flavoured items (1), all items (-1 aka not specified). */
		if (tc == 1 && !object_has_flavor(o_ptr->k_idx)) continue;
		else if (tc == 2 && object_has_flavor(o_ptr->k_idx)) continue;

		/* Item is still charging up? */
		if (i_ptr->recharging) continue;

		/* Can't use them? skip */
		if (p_ptr->antimagic || get_skill(p_ptr, SKILL_ANTIMAGIC)) {
			msg_format(Ind, "\377%cYour anti-magic prevents you from using your magic device.", COLOUR_AM_OWN);
			failure |= 0x2;
			break; /* can't activate any other items either */
		}

		/* activate it later, at a point where we can use p_ptr->command_rep */
		p_ptr->delayed_index = index;
		p_ptr->delayed_spell = -1;
		p_ptr->current_item = slot;

#ifdef XID_REPEAT
		p_ptr->command_rep_active = FALSE;//paranoia
		p_ptr->delayed_index_temp = -1;
#endif
		return;
	}

#ifdef ENABLE_XID_MDEV
	/* Check for ID rods/staves */
	for (index = 0; index < INVEN_PACK; index++) {
 #ifdef ENABLE_SUBINVEN
		/* Currently operating on subinven instead of backpack? */
		if (k != -1) {
			/* Test first/subsequent item from the bag */
			j++;

			/* Reached end of bag? */
			if (j == k) {
				/* Proceed with actual backpack */
				k = -1;
				continue;
			}

			i_ptr = &p_ptr->subinventory[index][j];

			/* Reached last available item in the bag? */
			if (!i_ptr->tval) {
				/* Proceed with actual backpack */
				k = -1;
				continue;
			}

			/* Don't advance backpack - ensure loop works once more */
			index--;
		} else {
 #endif
			i_ptr = &(p_ptr->inventory[index]);
			if (!i_ptr->k_idx) continue;

 #ifdef ENABLE_SUBINVEN
			/* Antistatic Wrapping: Check for staves of perception / *perception*, rods of perception */
			if (i_ptr->tval == TV_SUBINVEN) {
				j = -1;
				/* Do loop over the subinven now */
				k = i_ptr->bpval;
				/* Don't advance backpack - ensure loop works once more */
				index--;
				continue;
			}
		}
 #endif

		/* ID rod && ID staff (no *perc*) */
		if (!(i_ptr->tval == TV_ROD && i_ptr->sval == SV_ROD_IDENTIFY &&
 #ifndef NEW_MDEV_STACKING
		    !i_ptr->pval
 #else
		    i_ptr->bpval < i_ptr->number
 #endif
		    ) && !(i_ptr->tval == TV_STAFF && i_ptr->sval == SV_STAFF_IDENTIFY &&
		     (i_ptr->pval > 0 || (!object_known_p(Ind, i_ptr) && !(i_ptr->ident & ID_EMPTY)))))
			continue;

		if (!can_use(Ind, i_ptr)) continue;

		/* Check if the player does want this feature (!X - for now :) ) */
		if (!(tc = check_guard_inscription(i_ptr->note, 'X'))) continue;
		/* Allow 'treasure class': No flavoured items (2), only flavoured items (1), all items (-1 aka not specified). */
		if (tc == 1 && !object_has_flavor(o_ptr->k_idx)) continue;
		else if (tc == 2 && object_has_flavor(o_ptr->k_idx)) continue;

		if (p_ptr->antimagic || get_skill(p_ptr, SKILL_ANTIMAGIC)) {
			msg_format(Ind, "\377%cYour anti-magic prevents you from using your magic device.", COLOUR_AM_OWN);
			failure |= 0x2;
			break; /* can't activate any other items either */
		}

		/* activate it later, at a point where we can use p_ptr->command_rep */
 #ifdef ENABLE_SUBINVEN
		if (k != -1) p_ptr->delayed_index = (index + 2) * 100 + j;
		else
 #endif
		p_ptr->delayed_index = index;
		p_ptr->delayed_spell = (i_ptr->tval == TV_ROD) ? -3 : -2;
		p_ptr->current_item = slot;

 #ifdef XID_REPEAT
		p_ptr->command_rep_active = FALSE;//paranoia
		p_ptr->delayed_index_temp = -1;
 #endif
		return;
	}
#endif

#ifdef ENABLE_XID_SPELL
	/* Lastly, check for ID spells -- IDENTIFY or MIDENTIFY or even STARIDENTIFY or BAGIDENTIFY. */
	for (index = 0; index < INVEN_PACK; index++) {
		i_ptr = &(p_ptr->inventory[index]);
		if (!i_ptr->k_idx) continue;

		if (i_ptr->tval != TV_BOOK) continue;

		if (!can_use(Ind, i_ptr)) continue;

		/* Check if the player does want this feature (!X - for now :) ) */
		if (!(tc = check_guard_inscription(i_ptr->note, 'X'))) continue;
		/* Allow 'treasure class': No flavoured items (2), only flavoured items (1), all items (-1 aka not specified). */
		if (tc == 1 && !object_has_flavor(o_ptr->k_idx)) continue;
		else if (tc == 2 && object_has_flavor(o_ptr->k_idx)) continue;

		/* Can't use them? skip. (Note: Even 'Revelation' requires these, luckily) */
		if (p_ptr->blind || no_lite(Ind) || p_ptr->confused) {
			failure |= 0x4;
			break;
		}

		if (p_ptr->antimagic || get_skill(p_ptr, SKILL_ANTIMAGIC)) {
			msg_format(Ind, "\377%cYour anti-magic prevents you from casting your spell.", COLOUR_AM_OWN);
			failure |= 0x8;
			break; /* can't activate any other items either */
		}

		if (i_ptr->sval == SV_SPELLBOOK) {
			if (i_ptr->pval == ID_spell1 || i_ptr->pval == ID_spell1a || i_ptr->pval == ID_spell1b || i_ptr->pval == ID_spell2 ||
			    i_ptr->pval == ID_spell3
 #ifdef ALLOW_X_BAGID
			    || i_ptr->pval == ID_spell4
 #endif
			    ) {
				/* Have we learned this spell yet at all? */
				if (!exec_lua(Ind, format("return is_ok_spell(%d, %d)", Ind, i_ptr->pval))) {
					/* Just continue&ignore instead of return, since we
					   might just have picked up someone else's book! */
					msg_print(Ind, "\377oYou cannot cast the identify spell of your inscribed spell scroll.");
					continue;
				}
				/* If so then use it */
				spell = i_ptr->pval;
				//wow-- use ground item! ;) (except for BAGIDENTIFY)
				if (spell != ID_spell4 && spell != ID_spell1a && spell != ID_spell1b)
					p_ptr->current_item = slot;
			} else {
				/* Be severe and point out the wrong inscription: */
				msg_print(Ind, "\377oThe inscribed spell scroll isn't an eligible identify spell.");
				continue;
			}
		} else {
			if (MY_VERSION < (4 << 12 | 4 << 8 | 1U << 4 | 8)) {
			/* now <4.4.1.8 is no longer supported! to make s_aux.lua slimmer */
				ID_spell1_found = exec_lua(Ind, format("return spell_in_book(%d, %d)", i_ptr->sval, ID_spell1));//NO LONGER SUPPORTED
				//ID_spell1a, ID_spell1b are not supported
				ID_spell2_found = exec_lua(Ind, format("return spell_in_book(%d, %d)", i_ptr->sval, ID_spell2));//NO LONGER SUPPORTED
				ID_spell3_found = exec_lua(Ind, format("return spell_in_book(%d, %d)", i_ptr->sval, ID_spell3));//NO LONGER SUPPORTED
 #ifdef ALLOW_X_BAGID
				ID_spell4_found = exec_lua(Ind, format("return spell_in_book(%d, %d)", i_ptr->sval, ID_spell4));//NO LONGER SUPPORTED
 #endif
				if (!ID_spell1_found && !ID_spell2_found && //ID_spell1a, ID_spell1b are not supported
				    !ID_spell3_found && !ID_spell4_found) {
					/* Be severe and point out the wrong inscription: */
					msg_print(Ind, "\377oThe inscribed book doesn't contain an eligible identify spell.");
					continue;
				}
			} else {
				ID_spell1_found = exec_lua(Ind, format("return spell_in_book2(%d, %d, %d)", index, i_ptr->sval, ID_spell1));
				ID_spell1a_found = exec_lua(Ind, format("return spell_in_book2(%d, %d, %d)", index, i_ptr->sval, ID_spell1a));
				ID_spell1b_found = exec_lua(Ind, format("return spell_in_book2(%d, %d, %d)", index, i_ptr->sval, ID_spell1b));
				ID_spell2_found = exec_lua(Ind, format("return spell_in_book2(%d, %d, %d)", index, i_ptr->sval, ID_spell2));
				ID_spell3_found = exec_lua(Ind, format("return spell_in_book2(%d, %d, %d)", index, i_ptr->sval, ID_spell3));
 #ifdef ALLOW_X_BAGID
				ID_spell4_found = exec_lua(Ind, format("return spell_in_book2(%d, %d, %d)", index, i_ptr->sval, ID_spell4));
 #endif
				if (!ID_spell1_found && !ID_spell1a_found && !ID_spell1b_found && !ID_spell2_found &&
				    !ID_spell3_found && !ID_spell4_found) {
					/* Be severe and point out the wrong inscription: */
					msg_print(Ind, "\377oThe inscribed book doesn't contain an eligible identify spell.");
					continue;
				}
			}
		}

		/* Have we learned this spell yet at all? */
		//wow, first time use of '-item' ground access nowadays? :-p
		if (ID_spell1_found && exec_lua(Ind, format("return is_ok_spell(%d, %d)", Ind, ID_spell1))) {
			spell = ID_spell1;
			p_ptr->current_item = slot;
		}
		else if (ID_spell1a_found && exec_lua(Ind, format("return is_ok_spell(%d, %d)", Ind, ID_spell1a)))
			spell = ID_spell1a; //bag-id effect
		else if (ID_spell1b_found && exec_lua(Ind, format("return is_ok_spell(%d, %d)", Ind, ID_spell1b)))
			spell = ID_spell1b; //bag-id effect
		else if (ID_spell2_found && exec_lua(Ind, format("return is_ok_spell(%d, %d)", Ind, ID_spell2))) {
			spell = ID_spell2;
			p_ptr->current_item = slot;
		} else if (ID_spell3_found && exec_lua(Ind, format("return is_ok_spell(%d, %d)", Ind, ID_spell3))) {
			spell = ID_spell3;
			p_ptr->current_item = slot;
		}
 #ifdef ALLOW_X_BAGID
		else if (ID_spell4_found && exec_lua(Ind, format("return is_ok_spell(%d, %d)", Ind, ID_spell4)))
			spell = ID_spell4;
 #endif

		/* Just continue&ignore instead of return, since we
		   might just have picked up someone else's book! */
		if (spell == -1) {
			msg_print(Ind, "\377oYou cannot cast an identify spell in your inscribed book.");
			continue;
		}

		/* cast it later, at a point where we can use p_ptr->command_rep */
		p_ptr->delayed_index = index;
		p_ptr->delayed_spell = spell + 1;
		p_ptr->current_item = slot;

 #ifdef XID_REPEAT
		p_ptr->command_rep_active = FALSE;//paranoia
		p_ptr->delayed_index_temp = -1;
 #endif
		return;
	}
#endif

	/* 'failure' is set at this point; clear actions (spell and item) */
	p_ptr->delayed_spell = 0;
	p_ptr->current_item = -1;

	/* Display single failure */
	switch (failure) {
	case 0x1:
		msg_print(Ind, "You cannot read identify scrolls while blinded, confused or without light.");
		return;
	case 0x2:
		msg_print(Ind, "Your anti-magic prevents using your magic device to identify the item.");
		return;
	case 0x4:
		msg_print(Ind, "You cannot cast identify spells while blinded, confused or without light.");
		return;
	case 0x8:
		msg_print(Ind, "Your anti-magic prevents you from casting your identify spell.");
		return;
	}
	/* Multiple failures occurred */
	if (failure) msg_print(Ind, "You are unable to use any of your identify items.");
}
