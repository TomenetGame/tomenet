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


#if EXTRA_FLAVORS

 #define MAX_MODIFIERS	6       /* Used with rings (see below) */

static cptr ring_adj2[MAX_ROCKS2] =
{
	"", "Brilliant", "Dark", "Enchanting", "Murky", "Bright"
};

#endif	// EXTRA_FLAVORS


/*
 * Rings (adjectives and colors)
 */

static cptr ring_adj[MAX_ROCKS] =
{
	"Alexandrite", "Amethyst", "Aquamarine", "Azurite", "Beryl",
	"Bloodstone", "Calcite", "Carnelian", "Corundum", "Diamond",
	"Emerald", "Fluorite", "Garnet", "Granite", "Jade",
	"Jasper", "Lapis Lazuli", "Malachite", "Marble", "Moonstone",
	"Onyx", "Opal", "Pearl", "Quartz", "Quartzite",
	"Rhodonite", "Ruby", "Sapphire", "Tiger Eye", "Topaz",
	"Turquoise", "Zircon", "Platinum", "Bronze", "Gold",
	"Obsidian", "Silver", "Tortoise Shell", "Mithril", "Jet",
	"Engagement", "Adamantite",
	"Wire", "Dilithium", "Bone", "Wooden",
	"Spikard", "Serpent", "Wedding", "Double",
	"Plain", "Brass", "Scarab", "Shining",
	"Rusty", "Transparent", "Copper", "Black Opal", "Nickel",
        "Glass", "Fluorspar", "Agate",
};
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
	TERM_YELLOW, TERM_VIOLET,
	TERM_UMBER, TERM_L_WHITE, TERM_WHITE, TERM_UMBER,
	TERM_BLUE, TERM_GREEN, TERM_YELLOW, TERM_ORANGE,
	TERM_YELLOW, TERM_ORANGE, TERM_L_GREEN, TERM_LITE,
	TERM_UMBER, TERM_L_WHITE, TERM_UMBER, TERM_L_DARK, TERM_L_WHITE,
        TERM_L_WHITE, TERM_BLUE, TERM_L_WHITE
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
	TERM_YELLOW, TERM_VIOLET,
	TERM_UMBER, TERM_L_WHITE, TERM_WHITE, TERM_UMBER,
	TERM_BLUE, TERM_GREEN, TERM_YELLOW, TERM_ORANGE,
	TERM_YELLOW, TERM_ORANGE, TERM_L_GREEN, TERM_YELLOW,
	TERM_RED, TERM_WHITE, TERM_UMBER, TERM_L_DARK, TERM_L_WHITE,
        TERM_WHITE, TERM_BLUE, TERM_L_WHITE
};
#endif

/*
 * Amulets (adjectives and colors)
 */

static cptr amulet_adj[MAX_AMULETS] = {
	"Amber", "Driftwood", "Coral", "Agate", "Ivory",
	"Obsidian", "Bone", "Brass", "Bronze", "Pewter",
	"Tortoise Shell", "Golden", "Azure", "Crystal", "Silver",
        "Copper", "Amethyst", "Mithril", "Sapphire", "Dragon Tooth",
        "Carved Oak", "Sea Shell", "Flint Stone", "Ruby", "Scarab",
        "Origami Paper", "Meteoric Iron", "Platinum", "Glass", "Beryl",
        "Malachite", "Adamantite", "Mother-of-pearl", "Runed", "Sandalwood",
	"Emerald", "Aquamarine", "Sapphire", "Glimmer-Stone", "Ebony",
	"Meerschaum", "Jade", "Red Opal",
};
#if 1 /*more animated TERMs*/
static byte amulet_col[MAX_AMULETS] = {
	TERM_YELLOW, TERM_L_UMBER, TERM_WHITE, TERM_L_WHITE, TERM_WHITE,
	TERM_L_DARK, TERM_WHITE, TERM_ORANGE, TERM_L_UMBER, TERM_SLATE,
	TERM_GREEN, TERM_YELLOW, TERM_L_BLUE, TERM_L_BLUE, TERM_L_WHITE,
	TERM_L_UMBER, TERM_VIOLET, TERM_L_BLUE, TERM_BLUE, TERM_L_WHITE,
	TERM_UMBER, TERM_L_BLUE, TERM_SLATE, TERM_RED, TERM_L_GREEN, 
	TERM_WHITE, TERM_L_DARK, TERM_L_WHITE, TERM_L_WHITE, TERM_L_GREEN, 
	TERM_GREEN, TERM_VIOLET, TERM_L_WHITE, TERM_UMBER, TERM_L_WHITE,
	TERM_GREEN, TERM_L_BLUE, TERM_ELEC, TERM_LITE, TERM_L_DARK,
	TERM_L_WHITE, TERM_L_GREEN, TERM_RED
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
	TERM_L_WHITE, TERM_L_GREEN, TERM_RED
};
#endif


/*
 * Staffs (adjectives and colors)
 */

static cptr staff_adj[MAX_WOODS] =
{
	"Aspen", "Balsa", "Banyan", "Birch", "Cedar",
	"Cottonwood", "Cypress", "Dogwood", "Elm", "Eucalyptus",
	"Hemlock", "Hickory", "Ironwood", "Locust", "Mahogany",
	"Maple", "Mulberry", "Oak", "Pine", "Redwood",
	"Rosewood", "Spruce", "Sycamore", "Teak", "Walnut",
	"Mistletoe", "Hawthorn", "Bamboo", "Silver", "Runed",
	"Golden", "Ashen", "Gnarled", "Ivory", "Willow",
	"Cryptomeria"
};

static byte staff_col[MAX_WOODS] =
{
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

static cptr wand_adj[MAX_METALS] =
{
	"Aluminium", "Cast Iron", "Chromium", "Copper", "Gold",
	"Iron", "Magnesium", "Molybdenum", "Nickel", "Rusty",
	"Silver", "Steel", "Tin", "Titanium", "Tungsten",
	"Zirconium", "Zinc", "Aluminium-Plated", "Copper-Plated", "Gold-Plated",
	"Nickel-Plated", "Silver-Plated", "Steel-Plated", "Tin-Plated", "Zinc-Plated",
	"Mithril-Plated", "Mithril", "Runed", "Bronze", "Brass",
	"Platinum", "Lead","Lead-Plated", "Ivory" , "Adamantite",
	"Uridium", "Long", "Short", "Hexagonal"
};

static byte wand_col[MAX_METALS] =
{
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

static cptr rod_adj[MAX_METALS];

static byte rod_col[MAX_METALS];


/*
 * Mushrooms (adjectives and colors)
 */

static cptr food_adj[MAX_SHROOM] =
{
	"Blue", "Black", "Black Spotted", "Brown", "Dark Blue",
	"Dark Green", "Dark Red", "Yellow", "Furry", "Green",
	"Grey", "Light Blue", "Light Green", "Violet", "Red",
	"Slimy", "Tan", "White", "White Spotted", "Wrinkled",
};

static byte food_col[MAX_SHROOM] =
{
	TERM_BLUE, TERM_L_DARK, TERM_L_DARK, TERM_UMBER, TERM_BLUE,
	TERM_GREEN, TERM_RED, TERM_YELLOW, TERM_L_WHITE, TERM_GREEN,
	TERM_SLATE, TERM_L_BLUE, TERM_L_GREEN, TERM_VIOLET, TERM_RED,
	TERM_SLATE, TERM_L_UMBER, TERM_WHITE, TERM_WHITE, TERM_UMBER
};


/*
 * Color adjectives and colors, for potions.
 * Hack -- The first four entries are hard-coded.
 * (water, apple juice, slime mold juice, something)
 */

#ifdef EXTRA_FLAVORS
/* 15 */
static cptr potion_mod[MAX_MOD_COLORS] =
{
	"Speckled", "Bubbling", "Cloudy", "Metallic", "Misty", "Smoky",
	"Pungent", "Clotted", "Viscous", "Oily", "Shimmering", "Coagulated",
	"Manly", "Stinking", "Whirling",
};

/* 34 */
static cptr potion_base[MAX_BASE_COLORS] =
{
        "Clear", "Light Brown", "Icky Green", "Phosphorescent",
	"Azure", "Blue", "Black", "Brown",
	"Chartreuse", "Crimson", "Cyan", "Dark Blue",
	"Dark Green", "Dark Red", "Green", "Grey", 
	"Hazy", "Indigo", "Light Blue", "Light Green", 
	"Magenta", "Orange", "Pink", "Puce",
	"Purple", "Red", "Tangerine", "Violet", 
	"Vermilion", "White", "Yellow", "Gloopy Green", 
	"Gold", "Ichor", "Ivory White", "Sky Blue", 
	"Beige",
};

#ifdef HOUSE_PAINTING
byte potion_col[MAX_COLORS] =
#else
static byte potion_col[MAX_COLORS] =
#endif
{
        TERM_WHITE, TERM_L_UMBER, TERM_GREEN, TERM_MULTI,
	TERM_L_BLUE, TERM_BLUE, TERM_L_DARK, TERM_UMBER,
	TERM_L_GREEN, TERM_RED, TERM_L_BLUE, TERM_BLUE, 
	TERM_GREEN, TERM_RED, TERM_GREEN, TERM_SLATE,
	TERM_L_WHITE, TERM_VIOLET, TERM_L_BLUE, TERM_L_GREEN, 
	TERM_RED, TERM_ORANGE, TERM_L_RED, TERM_VIOLET, 
	TERM_VIOLET, TERM_RED, TERM_ORANGE, TERM_VIOLET, 
	TERM_RED, TERM_WHITE, TERM_YELLOW, TERM_GREEN,
	TERM_YELLOW, TERM_RED, TERM_WHITE, TERM_L_BLUE,
	TERM_L_UMBER,
};

static char potion_adj[MAX_COLORS][24];

#else
static cptr potion_adj[MAX_COLORS] = {
        "Clear", "Light Brown", "Icky Green", "Phosphorescent",
	"Azure", "Blue", "Blue Speckled", "Black",
	"Brown", "Brown Speckled", "Bubbling", "Chartreuse", 
	"Cloudy", "Copper Speckled", "Crimson", "Cyan",
	"Dark Blue", "Dark Green", "Dark Red", "Gold Speckled",
	"Green", "Green Speckled", "Grey", "Grey Speckled", 
	"Hazy", "Indigo", "Light Blue", "Light Green", 
	"Magenta", "Metallic Blue", "Metallic Red", "Metallic Green", 
	"Metallic Purple", "Misty", "Orange", "Orange Speckled",
	"Pink", "Pink Speckled", "Puce", "Purple",
	"Purple Speckled", "Red", "Red Speckled", "Silver Speckled", 
	"Smoky", "Tangerine", "Violet", "Vermilion", 
	"White", "Yellow", "Violet Speckled", "Pungent", 
	"Clotted Red", "Viscous Pink", "Oily Yellow", "Gloopy Green",
	"Shimmering", "Coagulated Crimson", "Yellow Speckled", "Gold",
	"Manly", "Stinking", "Oily Black", "Ichor",
	"Ivory White", "Sky Blue", "Beige", "Whirling",
//	"Glowing Green", "Glowing Blue", "Glowing Red", "Glittering",
};
# if 1
#ifdef HOUSE_PAINTING
byte potion_col[MAX_COLORS] = {
#else
static byte potion_col[MAX_COLORS] = {
#endif
	TERM_ELEC, TERM_L_UMBER, TERM_GREEN, TERM_LITE,
	TERM_L_BLUE, TERM_BLUE, TERM_ELEC, TERM_L_DARK, 
	TERM_UMBER, TERM_SHAR, TERM_COLD, TERM_L_GREEN, 
	TERM_ACID, TERM_CONF, TERM_RED, TERM_L_BLUE,
	TERM_BLUE, TERM_GREEN, TERM_RED, TERM_LITE, 
	TERM_GREEN, TERM_POIS, TERM_SLATE, TERM_ACID, 
	TERM_L_WHITE, TERM_VIOLET, TERM_L_BLUE, TERM_L_GREEN, 
	TERM_L_RED, TERM_BLUE, TERM_RED, TERM_GREEN, 
	TERM_VIOLET, TERM_L_WHITE, TERM_ORANGE, TERM_SOUN,
	TERM_L_RED, TERM_L_RED, TERM_VIOLET, TERM_VIOLET, 
	TERM_VIOLET, TERM_RED, TERM_RED, TERM_COLD, 
	TERM_DARKNESS, TERM_ORANGE, TERM_VIOLET, TERM_RED, 
	TERM_WHITE, TERM_YELLOW, TERM_VIOLET, TERM_L_RED, 
	TERM_RED, TERM_L_RED, TERM_YELLOW, TERM_GREEN,
	TERM_MULTI, TERM_RED, TERM_YELLOW, TERM_LITE,
	TERM_L_UMBER, TERM_UMBER, TERM_L_DARK, TERM_RED, 
	TERM_WHITE, TERM_L_BLUE, TERM_L_WHITE, TERM_MULTI,
//	TERM_POIS, TERM_ELEC, TERM_FIRE, TERM_COLD,
};
# else
#ifdef HOUSE_PAINTING
byte potion_col[MAX_COLORS] = {
#else
static byte potion_col[MAX_COLORS] = {
#endif
        TERM_WHITE, TERM_L_UMBER, TERM_GREEN, TERM_LITE,
	TERM_L_BLUE, TERM_BLUE, TERM_BLUE, TERM_L_DARK, 
	TERM_UMBER, TERM_UMBER, TERM_L_WHITE, TERM_L_GREEN, 
	TERM_WHITE, TERM_L_UMBER, TERM_RED, TERM_L_BLUE,
	TERM_BLUE, TERM_GREEN, TERM_RED, TERM_YELLOW, 
	TERM_GREEN, TERM_GREEN, TERM_SLATE, TERM_SLATE, 
	TERM_L_WHITE, TERM_VIOLET, TERM_L_BLUE, TERM_L_GREEN, 
	TERM_RED, TERM_BLUE, TERM_RED, TERM_GREEN, 
	TERM_VIOLET, TERM_L_WHITE, TERM_ORANGE, TERM_ORANGE,
	TERM_L_RED, TERM_L_RED, TERM_VIOLET, TERM_VIOLET, 
	TERM_VIOLET, TERM_RED, TERM_RED, TERM_L_WHITE, 
	TERM_L_DARK, TERM_ORANGE, TERM_VIOLET, TERM_RED, 
	TERM_WHITE, TERM_YELLOW, TERM_VIOLET, TERM_L_RED, 
	TERM_RED, TERM_L_RED, TERM_YELLOW, TERM_GREEN,
	TERM_MULTI, TERM_RED, TERM_YELLOW, TERM_YELLOW,
	TERM_L_UMBER, TERM_UMBER, TERM_L_DARK, TERM_RED, 
	TERM_WHITE, TERM_L_BLUE, TERM_L_UMBER, TERM_L_WHITE,
//	TERM_POIS, TERM_ELEC, TERM_FIRE, TERM_COLD,
};
# endif
#endif	// 0


/*
 * Syllables for scrolls (must be 1-4 letters each)
 */

static cptr syllables[MAX_SYLLABLES] =
{
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
static bool object_has_flavor(int i)
{
	object_kind *k_ptr = &k_info[i];

	/* Check for flavor */
	switch (k_ptr->tval)
	{
		/* The standard "flavored" items */
		case TV_AMULET:
		case TV_RING:
		case TV_STAFF:
		case TV_WAND:
		case TV_SCROLL:
		case TV_POTION:
		case TV_POTION2:	// Hack inside!
		case TV_ROD:
		{
			return (TRUE);
		}

		/* Hack -- food SOMETIMES has a flavor */
		case TV_FOOD:
		{
			if ((k_ptr->sval < SV_FOOD_MIN_FOOD) || (k_ptr->sval > SV_FOOD_MAX_FOOD)) return (TRUE);
			return (FALSE);
		}
	}

	/* Assume no flavor */
	return (FALSE);
}


/*
 * Certain items, if aware, are known instantly
 * This function is used only by "flavor_init()"
 *
 * XXX XXX XXX Add "EASY_KNOW" flag to "k_info.txt" file
 */
static bool object_easy_know(int i)
{
	object_kind *k_ptr = &k_info[i];

	/* Analyze the "tval" */
	switch (k_ptr->tval)
	{
		/* Simple items */
		case TV_FLASK:
		case TV_JUNK:
		case TV_BOTTLE:
		case TV_SKELETON:
		case TV_SPIKE:
                case TV_EGG:
                case TV_FIRESTONE:
                case TV_CORPSE:
                case TV_HYPNOS:
		case TV_RUNE1:
		case TV_RUNE2:
		{
			return (TRUE);
		}

		/* All Food, Potions, Scrolls, Rods */
		case TV_FOOD:
		case TV_POTION:
		case TV_POTION2:
		case TV_SCROLL:
		case TV_PARCHMENT:
		case TV_ROD:
                case TV_ROD_MAIN:
                case TV_BATERIE:
		{
			return (TRUE);
		}

		/* Some Rings, Amulets, Lites */
		case TV_RING:
		case TV_AMULET:
		case TV_LITE:
		/* added default (for tools which got EASY_KNOW added,
		   such as flint/climbing set) */
		default:
		{
			if (k_ptr->flags3 & TR3_EASY_KNOW) return (TRUE);
			return (FALSE);
		}
	}

	/* Nope */
	return (FALSE);
}


/*
 * Hack -- prepare the default object attr codes by tval
 *
 * XXX XXX XXX Off-load to "pref.prf" file
 */
static byte default_tval_to_attr(int tval)
{
	switch (tval)
	{
		case TV_SKELETON:
		case TV_BOTTLE:
		case TV_JUNK:
		{
			return (TERM_WHITE);
		}

		case TV_CHEST:
		{
			return (TERM_SLATE);
		}

		case TV_SHOT:
		case TV_BOLT:
		case TV_ARROW:
		{
			return (TERM_L_UMBER);
		}

		case TV_LITE:
		{
			return (TERM_YELLOW);
		}

		case TV_SPIKE:
		{
			return (TERM_SLATE);
		}

		case TV_BOW:
		{
			return (TERM_UMBER);
		}

		case TV_DIGGING:
		{
			return (TERM_SLATE);
		}

		case TV_BLUNT:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_AXE:
		{
			return (TERM_L_WHITE);
		}

		case TV_BOOTS:
		case TV_GLOVES:
		case TV_CROWN:
		case TV_HELM:
		case TV_SHIELD:
		case TV_CLOAK:
		{
			return (TERM_L_UMBER);
		}

		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		{
			return (TERM_SLATE);
		}

                case TV_GOLEM:
		{
                        return (TERM_VIOLET);
		}

		case TV_AMULET:
		{
			return (TERM_ORANGE);
		}

		case TV_RING:
		{
			return (TERM_RED);
		}

		case TV_STAFF:
		{
			return (TERM_L_UMBER);
		}

		case TV_WAND:
		{
			return (TERM_L_GREEN);
		}

		case TV_ROD:
		{
			return (TERM_L_WHITE);
		}

		case TV_SCROLL:
		case TV_PARCHMENT:
		{
			return (TERM_WHITE);
		}

		case TV_POTION:
		case TV_POTION2:
		case TV_BOOK:	/* Right? */
		{
			return (TERM_L_BLUE);
		}

		case TV_FLASK:
		{
			return (TERM_YELLOW);
		}

		case TV_FOOD:
		{
			return (TERM_L_UMBER);
		}
	}

	return (TERM_WHITE);
}


/*
 * Hack -- prepare the default object char codes by tval
 *
 * XXX XXX XXX Off-load to "pref.prf" file (?)
 */
static byte default_tval_to_char(int tval)
{
	int i;

	/* Hack -- Guess at "correct" values for tval_to_char[] */
	for (i = 1; i < max_k_idx; i++)
	{
		object_kind *k_ptr = &k_info[i];

		/* Use the first value we find */
		if (k_ptr->tval == tval) return (k_ptr->k_char);
	}

	/* Default to space */
	return (' ');
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
void flavor_init(void)
{
	int		i, j;

	byte	temp_col;

	cptr	temp_adj;


	/* Hack -- Use the "simple" RNG */
	Rand_quick = TRUE;

	/* Hack -- Induce consistant flavors */
	Rand_value = seed_flavor;


	/* Efficiency -- Rods/Wands share initial array */
	for (i = 0; i < MAX_METALS; i++)
	{
		rod_adj[i] = wand_adj[i];
		rod_col[i] = wand_col[i];
	}


	/* Rings have "ring colors" */
	for (i = 0; i < MAX_ROCKS; i++)
	{
		j = rand_int(MAX_ROCKS);
		temp_adj = ring_adj[i];
		ring_adj[i] = ring_adj[j];
		ring_adj[j] = temp_adj;
		temp_col = ring_col[i];
		ring_col[i] = ring_col[j];
		ring_col[j] = temp_col;
	}

	/* Amulets have "amulet colors" */
	for (i = 0; i < MAX_AMULETS; i++)
	{
		j = rand_int(MAX_AMULETS);
		temp_adj = amulet_adj[i];
		amulet_adj[i] = amulet_adj[j];
		amulet_adj[j] = temp_adj;
		temp_col = amulet_col[i];
		amulet_col[i] = amulet_col[j];
		amulet_col[j] = temp_col;
	}

	/* Staffs */
	for (i = 0; i < MAX_WOODS; i++)
	{
		j = rand_int(MAX_WOODS);
		temp_adj = staff_adj[i];
		staff_adj[i] = staff_adj[j];
		staff_adj[j] = temp_adj;
		temp_col = staff_col[i];
		staff_col[i] = staff_col[j];
		staff_col[j] = temp_col;
	}

	/* Wands */
	for (i = 0; i < MAX_METALS; i++)
	{
		j = rand_int(MAX_METALS);
		temp_adj = wand_adj[i];
		wand_adj[i] = wand_adj[j];
		wand_adj[j] = temp_adj;
		temp_col = wand_col[i];
		wand_col[i] = wand_col[j];
		wand_col[j] = temp_col;
	}

	/* Rods */
	for (i = 0; i < MAX_METALS; i++)
	{
		j = rand_int(MAX_METALS);
		temp_adj = rod_adj[i];
		rod_adj[i] = rod_adj[j];
		rod_adj[j] = temp_adj;
		temp_col = rod_col[i];
		rod_col[i] = rod_col[j];
		rod_col[j] = temp_col;
	}

	/* Foods (Mushrooms) */
	for (i = 0; i < MAX_SHROOM; i++)
	{
		j = rand_int(MAX_SHROOM);
		temp_adj = food_adj[i];
		food_adj[i] = food_adj[j];
		food_adj[j] = temp_adj;
		temp_col = food_col[i];
		food_col[i] = food_col[j];
		food_col[j] = temp_col;
	}

	/* Potions (the first 4 potions are fixed) */
	for (i = 4; i < MAX_COLORS; i++)
	{
		j = rand_int(MAX_COLORS - 4) + 4;
		temp_adj = potion_adj[i];
		potion_adj[i] = potion_adj[j];
		potion_adj[j] = temp_adj;
		temp_col = potion_col[i];
		potion_col[i] = potion_col[j];
		potion_col[j] = temp_col;
	}

	/* Scrolls (random titles, always white) */
	for (i = 0; i < MAX_TITLES; i++)
	{
		/* Get a new title */
		while (TRUE)
		{
			char buf[80];

			bool okay;

			/* Start a new title */
			buf[0] = '\0';

			/* Collect words until done */
			while (TRUE)
			{
				int q, s;

				char tmp[80];

				/* Start a new word */
				tmp[0] = '\0';

				/* Choose one or two syllables */
				s = ((rand_int(100) < 30) ? 1 : 2);

				/* Add a one or two syllable word */
				for (q = 0; q < s; q++)
				{
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
			strcpy(scroll_adj[i], buf+1);

			/* Assume okay */
			okay = TRUE;

			/* Check for "duplicate" scroll titles */
			for (j = 0; j < i; j++)
			{
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
		scroll_col[i] = TERM_WHITE;
	}


	/* Hack -- Use the "complex" RNG */
	Rand_quick = FALSE;

	/* Analyze every object */
	for (i = 1; i < max_k_idx; i++)
	{
		object_kind *k_ptr = &k_info[i];

		/* Skip "empty" objects */
		if (!k_ptr->name) continue;

		/* Check for a "flavor" */
		k_ptr->has_flavor = object_has_flavor(i);

		/* No flavor yields aware */
		/*if (!k_ptr->has_flavor) k_ptr->aware = TRUE;*/
//                if ((!k_ptr->flavor) && (k_ptr->tval != TV_ROD_MAIN)) k_ptr->aware = TRUE;


		/* Check for "easily known" */
		k_ptr->easy_know = object_easy_know(i);
	}
}




/*
 * Extract the "default" attr for each object
 * This function is used only by "flavor_init()"
 */
static byte object_d_attr(int i)
{
	object_kind *k_ptr = &k_info[i];

	/* Flavored items */
	if (k_ptr->has_flavor)
	{
		/* Extract the indexx */
		int indexx = k_ptr->sval;

		/* Analyze the item */
		switch (k_ptr->tval)
		{
			case TV_FOOD:
				/* Hack for Mushroom of Unmagic */
				if (indexx == SV_FOOD_UNMAGIC) return (TERM_MULTI);
				else return (food_col[indexx]);
			case TV_POTION: return (potion_col[indexx]);
			case TV_SCROLL: return (scroll_col[indexx]);
			case TV_AMULET: return (amulet_col[indexx]);
			case TV_RING:   return (ring_col[indexx]);
			case TV_STAFF:  return (staff_col[indexx]);
			case TV_WAND:   return (wand_col[indexx]);
			case TV_ROD:    return (rod_col[indexx]);

			/* hack -- borrow those of potions */
			case TV_POTION2: return (potion_col[indexx + 4]); /* the first 4 potions are unique */
		}
	}

	/* Default attr if legal */
	if (k_ptr->k_attr) return (k_ptr->k_attr);

	/* Default to white */
	return (TERM_WHITE);
}


/*
 * Extract the "default" char for each object
 * This function is used only by "flavor_init()"
 */
static byte object_d_char(int i)
{
	object_kind *k_ptr = &k_info[i];

	return (k_ptr->k_char);
}


/*
 * Reset the "visual" lists
 *
 * This is useful for switching on/off the "use_graphics" flag.
 */
void reset_visuals(void)
{
	int i;

	/* Extract some info about terrain features */
	for (i = 0; i < MAX_F_IDX; i++)
	{
		feature_type *f_ptr = &f_info[i];

		/* Assume we will use the underlying values */
		f_ptr->z_attr = f_ptr->f_attr;
		f_ptr->z_char = f_ptr->f_char;
	}

	/* Extract some info about objects */
	for (i = 0; i < max_k_idx; i++)
	{
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
	for (i = 0; i < MAX_R_IDX; i++)
	{
		/* Extract the "underlying" attr */
		r_info[i].x_attr = r_info[i].d_attr;

		/* Extract the "underlying" char */
		r_info[i].x_char = r_info[i].d_char;
	}

	/* Extract attr/chars for equippy items (by tval) */
	for (i = 0; i < 128; i++)
	{
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
void object_flags(object_type *o_ptr, u32b *f1, u32b *f2, u32b *f3, u32b *f4, u32b *f5, u32b *esp)
{
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Base object */
	(*f1) = k_ptr->flags1;
	(*f2) = k_ptr->flags2;
	(*f3) = k_ptr->flags3;
	(*f4) = k_ptr->flags4;
	(*f5) = k_ptr->flags5;
	(*esp) = k_ptr->esp;


	/* Artifact */
	if (o_ptr->name1) {
		artifact_type *a_ptr;

		/* Hack -- Randarts! */
		if (o_ptr->name1 == ART_RANDART) {
			if (!(a_ptr = randart_make(o_ptr))) return;
			(*f1) = a_ptr->flags1;
			(*f2) = a_ptr->flags2;
			(*f3) = a_ptr->flags3;
			(*f4) = a_ptr->flags4;
			(*f5) = a_ptr->flags5;
			(*esp) = a_ptr->esp;
		} else {
			if (!(a_ptr = &a_info[o_ptr->name1])) return;
			(*f1) |= a_ptr->flags1;
			(*f2) |= a_ptr->flags2;
			(*f3) |= a_ptr->flags3;
			(*f4) |= a_ptr->flags4;
			(*f5) |= a_ptr->flags5;
			(*esp) |= a_ptr->esp;
		}
#if 0
		if ((!object_flags_no_set) && (a_ptr->set != -1))
			apply_flags_set(o_ptr->name1, a_ptr->set, f1, f2, f3, f4, f5, esp);
#endif	// 0
	}

	/* Ego-item */
	if (o_ptr->name2) {
//		ego_item_type *e_ptr = &e_info[o_ptr->name2];
		artifact_type *a_ptr;
		a_ptr = ego_make(o_ptr);

		(*f1) |= a_ptr->flags1;
		(*f2) |= a_ptr->flags2;
		(*f3) |= a_ptr->flags3;
		(*f4) |= a_ptr->flags4;
		(*f5) |= a_ptr->flags5;
		(*esp) |= a_ptr->esp;
	}
}




/*
 * Print a char "c" into a string "t", as if by sprintf(t, "%c", c),
 * and return a pointer to the terminator (t + 1).
 */
static char *object_desc_chr(char *t, char c)
{
	/* Copy the char */
	*t++ = c;

	/* Terminate */
	*t = '\0';

	/* Result */
	return (t);
}


/*
 * Print a string "s" into a string "t", as if by strcpy(t, s),
 * and return a pointer to the terminator.
 */
static char *object_desc_str(char *t, cptr s)
{
	/* Copy the string */
	while (*s) *t++ = *s++;

	/* Terminate */
	*t = '\0';

	/* Result */
	return (t);
}



/*
 * Print an unsigned number "n" into a string "t", as if by
 * sprintf(t, "%u", n), and return a pointer to the terminator.
 */
static char *object_desc_num(char *t, uint n)
{
	uint p;

        if (n > 99999) n = 99999;

	/* Find "size" of "n" */
	for (p = 1; n >= p * 10; p = p * 10) /* loop */;

	/* Dump each digit */
	while (p >= 1)
	{
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
	return (t);
}




/*
 * Print an signed number "v" into a string "t", as if by
 * sprintf(t, "%+d", n), and return a pointer to the terminator.
 * Note that we always print a sign, either "+" or "-".
 */
static char *object_desc_int(char *t, sint v)
{
	uint p, n;

	/* Negative */
	if (v < 0)
	{
		/* Take the absolute value */
		n = 0 - v;

		/* Use a "minus" sign */
		*t++ = '-';
	}

	/* Positive (or zero) */
	else
	{
		/* Use the actual number */
		n = v;

		/* Use a "plus" sign */
		*t++ = '+';
	}

	/* Find "size" of "n" */
	for (p = 1; n >= p * 10; p = p * 10) /* loop */;

	/* Dump each digit */
	while (p >= 1)
	{
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
	return (t);
}



/*
 * Print an unsigned number "n" into a string "t", as if by
 * sprintf(t, "%u%%", n), and return a pointer to the terminator.
 */
static char *object_desc_per(char *t, sint v)
{
	uint p, n;

	/* Negative */
	if (v < 0)
	{
		/* Take the absolute value */
		n = 0 - v;

		/* Use a "minus" sign */
		*t++ = '-';
	}

	/* Positive (or zero) */
	else
	{
		/* Use the actual number */
		n = v;
	}

	/* Find "size" of "n" */
	for (p = 1; n >= p * 10; p = p * 10) /* loop */;

	/* Dump each digit */
	while (p >= 1)
	{
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
	return (t);
}



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
 *
 * If the strings created with mode 0-3 are too long, this function is called
 * again with 8 added to 'mode' and attempt to 'abbreviate' the strings. -Jir-
 */
void object_desc(int Ind, char *buf, object_type *o_ptr, int pref, int mode)
{
        player_type     *p_ptr = NULL;
	cptr		basenm, modstr;
	int		power, indexx;

	bool		aware = FALSE;
	bool		known = FALSE;

	bool		append_name = FALSE;

	bool		show_weapon = FALSE;
	bool		show_armour = FALSE;
	bool		show_shield = FALSE;

	cptr		s, u;
	char		*t;

	char		p1 = '(', p2 = ')';
	char		b1 = '[', b2 = ']';
	char		c1 = '{', c2 = '}';

	char		tmp_val[ONAME_LEN];
	
	bool 		short_item_names = FALSE;

	u32b f1, f2, f3, f4, f5, esp;

	object_kind	*k_ptr = &k_info[o_ptr->k_idx];

	bool skip_base_article = FALSE;

	/* Extract some flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	/* hack - don't show special abilities on shirts easily (by adding them to their item name) - C. Blue */
	if (o_ptr->tval == TV_SOFT_ARMOR && o_ptr->sval == SV_SHIRT) mode |= 32;
	/* same for seals - C. Blue */
	if (o_ptr->tval == TV_SPECIAL) mode |= 32;

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
	}

	/* Hack -- Extract the sub-type "indexx" */
	indexx = o_ptr->sval;

	/* Extract default "base" string */
	basenm = (k_name + k_ptr->name);

	/* Assume no "modifier" string */
	modstr = "";


	/* Analyze the object */
	switch (o_ptr->tval) {
		/* Some objects are easy to describe */
		case TV_SPECIAL:
			if (o_ptr->sval == SV_CUSTOM_OBJECT)
				basenm = o_ptr->note ? quark_str(o_ptr->note) : "";
		case TV_SKELETON:
		case TV_BOTTLE:
		case TV_JUNK:
		case TV_SPIKE:
		case TV_KEY:
		case TV_FLASK:
		case TV_CHEST:
                case TV_FIRESTONE:
                case TV_INSTRUMENT:
                case TV_TOOL:
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
			basenm = "& # Trap Set~";
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
			/* Known artifacts */
//			if (artifact_p(o_ptr) && aware) break;
			if (artifact_p(o_ptr) && known) break;
			
			/* "Amulets of Luck" are just called "Talismans" -C. Blue */
			if ((o_ptr->sval == SV_AMULET_LUCK) && aware) break;

			/* Color the object */
			modstr = amulet_adj[indexx];
			if (aware && (!artifact_p(o_ptr) || (!known && !(f3 & TR3_INSTA_ART)))) append_name = TRUE;
			if (short_item_names)
			basenm = aware ? "& Amulet~" : "& # Amulet~";
			else
			basenm = "& # Amulet~";
			break;


			/* Rings (including a few "Specials") */
		case TV_RING:
			/* Known artifacts */
//			if (artifact_p(o_ptr) && aware) break;
			//if (artifact_p(o_ptr) && known && o_ptr->sval!=SV_RING_SPECIAL) break;
			if(o_ptr->sval == SV_RING_SPECIAL) basenm = "Ring of Power";
			if (artifact_p(o_ptr) && known) break;

			/* Color the object */
			modstr = ring_adj[indexx];
			if (aware && (!artifact_p(o_ptr) || (!known && !(f3 & TR3_INSTA_ART)))) append_name = TRUE;
//			if (aware) append_name = TRUE;
			if (short_item_names)
			basenm = aware ? "& Ring~" : "& # Ring~";
			else
			basenm = "& # Ring~";

			/* Hack -- The One Ring */
			if (!aware && (o_ptr->sval == SV_RING_POWER)) modstr = "Plain Gold";

			break;


		case TV_STAFF:
			/* Color the object */
			modstr = staff_adj[indexx];
			if (aware) append_name = TRUE;
			if (short_item_names)
			basenm = aware ? "& Staff~" : "& # Staff~";
			else
			basenm = "& # Staff~";
			break;

		case TV_WAND:
			/* Color the object */
			modstr = wand_adj[indexx];
			if (aware) append_name = TRUE;
			if (short_item_names)
			basenm = aware ? "& Wand~" : "& # Wand~";
			else
			basenm = "& # Wand~";
			break;

		case TV_ROD:
			/* Color the object */
			modstr = rod_adj[indexx];
			if (aware) append_name = TRUE;
			if (short_item_names)
			basenm = aware ? "& Rod~" : "& # Rod~";
			else
			basenm = "& # Rod~";
			break;

                case TV_ROD_MAIN:
		{
                        object_kind *tip_ptr = &k_info[lookup_kind(TV_ROD, o_ptr->pval)];
                        modstr = k_name + tip_ptr->name;
			break;
		}

		case TV_SCROLL:
			if (artifact_p(o_ptr) && known) break;
			if (o_ptr->sval == SV_SCROLL_CHEQUE) {
				basenm = "& Cheque~";
				break;
			}
			/* Color the object */
			modstr = scroll_adj[indexx];
			if (aware) append_name = TRUE;
			if (short_item_names)
			basenm = aware ? "& Scroll~" : "& Scroll~ titled \"#\"";
			else
			basenm = aware ? "& Scroll~ \"#\"" : "& Scroll~ titled \"#\"";
//			basenm = "& Scroll~ titled \"#\"";
			break;

		case TV_POTION:
		case TV_POTION2:
//			if (artifact_p(o_ptr) && known) break; // <-- might be "&& aware" instead!
			if (artifact_p(o_ptr) && aware) break;
			/* Color the object */
			modstr = potion_adj[indexx + (o_ptr->tval == TV_POTION2 ? 4 : 0)]; /* the first 4 potions are unique */
			if (aware) append_name = TRUE;
			if (short_item_names) basenm = aware ? "& Potion~" : "& # Potion~";
			else basenm = "& # Potion~";
			break;

		case TV_FOOD:
			/* Ordinary food is "boring" */
			if ((o_ptr->sval >= SV_FOOD_MIN_FOOD) && (o_ptr->sval <= SV_FOOD_MAX_FOOD)) break;

			if (indexx == SV_FOOD_UNMAGIC) {
				/* Hack for Mushroom of Unmagic */
				modstr = "Shimmering";
			} else {
				/* Color the object */
				modstr = food_adj[indexx];
			}
			if (aware) append_name = TRUE;
			if (short_item_names)
			basenm = aware ? "& Mushroom~" : "& # Mushroom~";
			else
			basenm = "& # Mushroom~";
			break;

		case TV_PARCHMENT:
			modstr = basenm;
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
//			basenm = k_name + k_ptr->name;
			if (o_ptr->sval == SV_SPELLBOOK) {
				if (school_spells[o_ptr->pval].name)
					modstr = school_spells[o_ptr->pval].name;
				else modstr = "Unknown spell";
			}
			break;
		case TV_RUNE1:
		    append_name = TRUE;
		    basenm = "& Basic Rune~#";
		    break;

#ifndef ENABLE_RCRAFT
		case TV_RUNE2:
		    append_name = TRUE;
		    basenm = "& Modifiying Rune~#";
		    break;
#else
		case TV_RUNE2:
			append_name = TRUE;
			if(o_ptr->sval >=0 && o_ptr->sval <=15)
				modstr = r_elements[o_ptr->sval].e_syl;
			basenm = "& '#' Rune~";
			break;
#endif

		/* Used in the "inventory" routine */
		default:
			/* the_sandman: debug line */
			if (o_ptr->tval != 0) s_printf("NOTHING_NOTICED: tval %d, sval %d\n", o_ptr->tval, o_ptr->sval);
			strcpy(buf, "(nothing)");
			return;
	}


	/* Start dumping the result */
	t = buf;

	/* The object "expects" a "number" */
	if (basenm[0] == '&') {
		cptr ego = NULL;

		/* Grab any ego-item name */
//		if ((o_ptr->name2 || o_ptr->name2b) && (o_ptr->tval != TV_ROD_MAIN))
		if (known && (o_ptr->name2 || o_ptr->name2b) && (o_ptr->tval != TV_ROD_MAIN) &&
		    !(o_ptr->tval == TV_SOFT_ARMOR && o_ptr->sval == SV_SHIRT))
		{
			ego_item_type *e_ptr = &e_info[o_ptr->name2];
			ego_item_type *e2_ptr = &e_info[o_ptr->name2b];

			if (e_ptr->before)
				ego = e_ptr->name + e_name;
#if 1
			else if (e2_ptr->before)
				ego = e2_ptr->name + e_name;
#endif	// 0

		}

		/* Skip the ampersand (and space) */
		s = basenm + 2;

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
		else if (known && artifact_p(o_ptr)) {
			/* hack: some base item types in k_info start
			   on 'the', so prevent a second 'The ' for those */
			t = object_desc_str(t, "The ");
			if (strstr(k_name + k_ptr->name, "the ") == k_name + k_ptr->name) {
				s += 4;
				skip_base_article = TRUE;
			}
		}

		/* A single one, with a vowel in the modifier */
		else if ((*s == '#') && (is_a_vowel(modstr[0])))
			t = object_desc_str(t, "an ");

		else if (ego != NULL) {
			if (is_a_vowel(ego[0])) t = object_desc_str(t, "an ");
			else t = object_desc_str(t, "a ");
		}

		/* A single one, with a vowel */
		else if (is_a_vowel(*s)) t = object_desc_str(t, "an ");

		/* A single one, without a vowel */
		else t = object_desc_str(t, "a ");
	}

	/* Hack -- objects that "never" take an article */
	else {
		/* No ampersand */
		s = basenm;

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
		else if (known && artifact_p(o_ptr) && !(mode & 8)) {
			/* hack: some base item types in k_info start
			   on 'the', so prevent a second 'The ' for those */
			t = object_desc_str(t, "The ");
			if (strstr(k_name + k_ptr->name, "the ") == k_name + k_ptr->name) {
				s += 4;
				skip_base_article = TRUE;
			}
		}

		/* Hack -- single items get no prefix */
		else {
			/* Nothing */
		}
	}

	/* Grab any ego-item name */
//	if ((o_ptr->name2 || o_ptr->name2b) && (o_ptr->tval != TV_ROD_MAIN))
	if (known && (o_ptr->name2 || o_ptr->name2b) && (o_ptr->tval != TV_ROD_MAIN) &&
	    !(o_ptr->tval == TV_SOFT_ARMOR && o_ptr->sval == SV_SHIRT)
//	    && o_ptr->tval != TV_SPECIAL
	    )
	{
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
	/* while (*s == '~') s++; */

	/* Copy the string */
	for (; *s; s++) {
		/* Pluralizer */
		if (*s == '~') {
			/* Add a plural if needed */
			if (o_ptr->number != 1) {
				char k = t[-1], k2 = t[-2];

				/* XXX XXX XXX Mega-Hack */

				/* Hack -- "Cutlass-es" and "Torch-es" and theoretically "Topaz-es" */
				if ((k == 's') || (k == 'h') || (k == 'z')) *t++ = 'e';

				/* Hack -- Finally, staffs become staves.
				   Note: Might require fix for future word additions ;) */
				if (k == 'f') {
					if (k2 == 'f') t -= 2;
					else t--;
					*t++ = 'v';
					*t++ = 'e';
				}

				/* Hack -- ""Cod/ex -> Cod/ices" */
				if (k == 'x') {
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

				/* Add an 's' */
				*t++ = 's';
			}
		}

		/* Modifier */
		else if (*s == '#') {
                        /* Grab any ego-item name */
                        if (o_ptr->tval == TV_ROD_MAIN) {
                                t = object_desc_chr(t, ' ');

                                if (known && o_ptr->name2) {
                                        ego_item_type *e_ptr = &e_info[o_ptr->name2];

                                        t = object_desc_str(t, (e_name + e_ptr->name));
                                }
                        }

			/* Insert the modifier */
			for (u = modstr; *u; u++) *t++ = *u;
		}

		/* Normal */
		else {
			/* Copy */
			*t++ = *s;
		}
	}

	/* Terminate */
	*t = '\0';


	/* Append the "kind name" to the "base name" */
	if (append_name) {
		t = object_desc_str(t, !(mode & 8) ? " of " : " ");
		if (!skip_base_article)
			t = object_desc_str(t, (k_name + k_ptr->name));
		else
			t = object_desc_str(t, (k_name + k_ptr->name + 4));
	}


	/* Hack -- Append "Artifact" or "Special" names */
	if (known) {
		/* Grab any ego-item name */
//                if ((o_ptr->name2 || o_ptr->name2b) && (o_ptr->tval != TV_ROD_MAIN))
		if ((o_ptr->name2 || o_ptr->name2b) && (o_ptr->tval != TV_ROD_MAIN) &&
		    !(o_ptr->tval == TV_SOFT_ARMOR && o_ptr->sval == SV_SHIRT)
//		    && o_ptr->tval != TV_SPECIAL
		    )
		{
			ego_item_type *e_ptr = &e_info[o_ptr->name2];
			ego_item_type *e2_ptr = &e_info[o_ptr->name2b];

                        if (!e_ptr->before && o_ptr->name2) {
				if (strlen(e_name + e_ptr->name)) t = object_desc_chr(t, ' ');
                                t = object_desc_str(t, (e_name + e_ptr->name));

                        }
#if 1
                        else if (!e2_ptr->before && o_ptr->name2b)
                        {
				if (strlen(e_name + e2_ptr->name)) t = object_desc_chr(t, ' ');
                                t = object_desc_str(t, (e_name + e2_ptr->name));
//                                ego = e2_ptr->name + e_name;
						}
#endif	// 0

		}

		/* Grab any randart name */
                if (o_ptr->name1 == ART_RANDART) {
			t = object_desc_chr(t, ' ');

			if(o_ptr->tval == TV_RING && o_ptr->sval == SV_RING_SPECIAL){
				monster_race *r_ptr = &r_info[o_ptr->bpval];
				t = object_desc_str(t, "of ");
				if (!(r_ptr->flags7 & RF7_NAZGUL)) {
					t = object_desc_str(t, "bug");
				} else {
					t = object_desc_str(t, r_name + r_ptr->name);
				}
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

		/* -TM- Hack -- Add false-artifact names */
		/* Dagger inscribed {@w0#of Smell} will be named
		 * Dagger of Smell {@w0} */
		if (o_ptr->note && !(o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT)) {
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

	/* Print level and owning */
	t = object_desc_chr(t, ' ');
	t = object_desc_chr(t, '{');
	if (o_ptr->owner)
	{
		cptr name = lookup_player_name(o_ptr->owner);

		if (name != NULL)
		{
			if (Ind && !strcmp(name, p_ptr->name))
			{
				t = object_desc_chr(t, '+');
			}
			else
			{
				if (mode & 16) t = object_desc_chr(t, '*');
				else t = object_desc_str(t, name);
			}
		}
		else
		{
			t = object_desc_chr(t, '-');
		}
		t = object_desc_chr(t, ',');
	}

	/* you cannot tell the level if not id-ed. */
	/* XXX it's still abusable */
	if (!known) t = object_desc_chr(t, '?');
	else t = object_desc_num(t, o_ptr->level);
	t = object_desc_chr(t, '}');


	/* No more details wanted */
	if ((mode & 7) < 1)
	{
		if (t - buf <= 65 || mode & 8) {
			return;
		} else {
			object_desc(Ind, buf, o_ptr, pref, mode | 8);
			return;
		}
	}


	/* Hack -- Chests must be described in detail */
	if (o_ptr->tval == TV_CHEST) {
		/* Not searched yet */
		if (!known) {
			/* Nothing */
		}

		/* May be "empty" */
		else if (!o_ptr->pval)
			t = object_desc_str(t, " (empty)");
		/* May be "disarmed" */
		else if (o_ptr->pval < 0)
			t = object_desc_str(t, " (disarmed)");

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
	}


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


	/* Dump base weapon info */
	switch (o_ptr->tval)
	{
		/* Missiles and Weapons */
		case TV_SHOT:
		case TV_BOLT:
		case TV_ARROW:
			/* Exploding arrow? */
			if (o_ptr->pval != 0 && known)
				t = object_desc_str(t, " (exploding)");
			/* No break, we want to continue the description */

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
		power = (o_ptr->sval % 10);

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


	/* Add the weapon bonuses */
	if (known)
	{
		/* Show the tohit/todam on request */
		if (show_weapon)
		{
			if (!(mode & 8)) t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, p1);
			t = object_desc_int(t, o_ptr->to_h);
			t = object_desc_chr(t, ',');
			t = object_desc_int(t, o_ptr->to_d);
			t = object_desc_chr(t, p2);
		}

		/* Show the tohit if needed */
		else if (o_ptr->to_h)
		{
			if (!(mode & 8)) t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, p1);
			t = object_desc_int(t, o_ptr->to_h);
			t = object_desc_chr(t, p2);
		}

		/* Show the todam if needed */
		else if (o_ptr->to_d)
		{
			if (!(mode & 8)) t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, p1);
			t = object_desc_int(t, o_ptr->to_d);
			t = object_desc_chr(t, p2);
		}
	}


	/* Add the armor bonuses */
	if (known)
	{
		/* Show the armor class info */
		if (show_armour)
		{
			if (!(mode & 8)) t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, b1);
			t = object_desc_num(t, o_ptr->ac);
			t = object_desc_chr(t, ',');
			t = object_desc_int(t, o_ptr->to_a);
			t = object_desc_chr(t, b2);
		}

		else if (show_shield)
		{
			if (!(mode & 8)) t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, b1);
			t = object_desc_per(t, o_ptr->ac);
			t = object_desc_chr(t, ',');
			t = object_desc_int(t, o_ptr->to_a);
			t = object_desc_chr(t, b2);
		}

		/* No base armor, but does increase armor */
		else if (o_ptr->to_a)
		{
			if (!(mode & 8)) t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, b1);
			t = object_desc_int(t, o_ptr->to_a);
			t = object_desc_chr(t, b2);
		}
	}

	/* Hack -- always show base armor */
	else if (show_armour)
	{
		if (!(mode & 8)) t = object_desc_chr(t, ' ');
		t = object_desc_chr(t, b1);
		t = object_desc_num(t, o_ptr->ac);
		t = object_desc_chr(t, b2);
	}
	else if (show_shield)
	{
		if (!(mode & 8)) t = object_desc_chr(t, ' ');
		t = object_desc_chr(t, b1);
		t = object_desc_per(t, o_ptr->ac);
		t = object_desc_chr(t, b2);
	}


	/* No more details wanted */
	if ((mode & 7) < 2)
	{
		if (t - buf <= 65 || mode & 8)
		{
			return;
		}
		else
		{
			object_desc(Ind, buf, o_ptr, pref, mode | 8);
			return;
		}
	}


	/* Hack -- Wands and Staffs have charges */
	if (known &&
	    ((o_ptr->tval == TV_STAFF) ||
	     (o_ptr->tval == TV_WAND)))
	{
		/* Dump " (N charges)" */
		if (!(mode & 8)) t = object_desc_chr(t, ' ');
		t = object_desc_chr(t, p1);
		t = object_desc_num(t, o_ptr->pval);
		if (!(mode & 8))
		{
			t = object_desc_str(t, " charge");
			if (o_ptr->pval != 1) t = object_desc_chr(t, 's');
		}
		t = object_desc_chr(t, p2);
	}

	/* Hack -- Rods have a "charging" indicator */
	else if (known && (o_ptr->tval == TV_ROD))
	{
		/* Hack -- Dump " (charging)" if relevant */
		if (o_ptr->pval) t = object_desc_str(t, !(mode & 8) ? " (charging)" : "(#)");
	}

	/* Hack -- Process Lanterns/Torches */
//	else if ((o_ptr->tval == TV_LITE) && (o_ptr->sval < SV_LITE_DWARVEN) && (!o_ptr->name3))
        else if ((o_ptr->tval == TV_LITE) && (f4 & TR4_FUEL_LITE))
	{
		/* Hack -- Turns of light for normal lites */
		t = object_desc_str(t, !(mode & 8) ? " (with " : "(");
		t = object_desc_num(t, o_ptr->timeout);
		t = object_desc_str(t, !(mode & 8) ? " turns of light)" : "t)");
	}

if (!(mode & 32)) {
	/* Dump "pval" flags for wearable items */
        if (known && (((f1 & (TR1_PVAL_MASK)) || (f5 & (TR5_PVAL_MASK))) || (o_ptr->tval == TV_GOLEM))) {
		/* Hack -- first display any base pval bonuses.  
		 * The "bpval" flags are never displayed.  */
		if (o_ptr->bpval && !(o_ptr->tval == TV_RING && o_ptr->sval == SV_RING_SPECIAL)) {
			if (!(mode & 8)) t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, p1);
			/* Dump the "pval" itself */
			t = object_desc_int(t, o_ptr->bpval);
#if 1
			/* hack (originally just for plain talismans, since it doesn't say "amulet of luck"
			   to give a hint to newbies what they do) to display a basic bpval property: */
			if (!o_ptr->pval && !(f3 & TR3_HIDE_TYPE)) {
				if (f1 & TR1_SPEED) t = object_desc_str(t, !(mode & 8) ? " to speed" : "spd");
				else if (f1 & TR1_BLOWS) {
					t = object_desc_str(t, !(mode & 8) ? " attack" : "at");
					if (ABS(o_ptr->pval) != 1 && !(mode & 8)) t = object_desc_chr(t, 's');
				} else if (f5 & (TR5_CRIT)) t = object_desc_str(t, !(mode & 8) ? " critical hits" : "crt");
				else if (f1 & TR1_STEALTH) t = object_desc_str(t, !(mode & 8) ? " to stealth" : "stl");
				else if (f1 & TR1_SEARCH) t = object_desc_str(t, !(mode & 8) ? " to searching" : "srch");
				else if (f1 & TR1_INFRA) t = object_desc_str(t, !(mode & 8) ? " to infravision" : "infr");
				else if (f5 & TR5_LUCK) t = object_desc_str(t, !(mode & 8) ? " to luck" : "luck");
				else if (f1 & TR1_TUNNEL) ;
			}
#endif
			/* finish with closing bracket ')' */
			t = object_desc_chr(t, p2);
		}
		/* Next, display any pval bonuses. */
		if (o_ptr->pval) {
			/* Start the display */
			if (!(mode & 8)) t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, p1);

			/* Dump the "pval" itself */
			t = object_desc_int(t, o_ptr->pval);

			/* Do not display the "pval" flags */
			if (f3 & TR3_HIDE_TYPE)
			{
				/* Nothing */
			}

			/* Speed */
			else if (f1 & TR1_SPEED)
			{
				/* Dump " to speed" */
				t = object_desc_str(t, !(mode & 8) ? " to speed" : "spd");
			}

			/* Attack speed */
			else if (f1 & TR1_BLOWS)
			{
				/* Add " attack" */
				t = object_desc_str(t, !(mode & 8) ? " attack" : "at");

				/* Add "attacks" */
				if (ABS(o_ptr->pval) != 1 && !(mode & 8)) t = object_desc_chr(t, 's');
			}

			/* Critical chance */
			else if (f5 & (TR5_CRIT))
			{
				/* Add " attack" */
				t = object_desc_str(t, !(mode & 8) ? " critical hits" : "crt");
			}

			/* Stealth */
			else if (f1 & TR1_STEALTH)
			{
				/* Dump " to stealth" */
				t = object_desc_str(t, !(mode & 8) ? " to stealth" : "stl");
			}

			/* Search */
			else if (f1 & TR1_SEARCH)
			{
				/* Dump " to searching" */
				t = object_desc_str(t, !(mode & 8) ? " to searching" : "srch");
			}

			/* Infravision */
			else if (f1 & TR1_INFRA)
			{
				/* Dump " to infravision" */
				t = object_desc_str(t, !(mode & 8) ? " to infravision" : "infr");
			}

			else if (f5 & TR5_LUCK) {
				/* Dump " to infravision" */
				t = object_desc_str(t, !(mode & 8) ? " to luck" : "luck");
			}

			/* Tunneling */
			else if (f1 & TR1_TUNNEL)
			{
				/* Nothing */
			}

			/* Finish the display */
			t = object_desc_chr(t, p2);
		}
	}
    } /* +32 mode */

	/* Special case, ugly, but needed */
	if (known && aware && (o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH))
	{
		t = object_desc_str(t, !(mode & 8) ? " of " : "-");
		t = object_desc_str(t, r_info[o_ptr->pval].name + r_name);

		/* Polymorph rings that run out.. */
		t = object_desc_str(t, "(");
		t = object_desc_num(t, o_ptr->timeout);
		t = object_desc_str(t, !(mode & 8) ? " turns of energy)" : "t)");
	}

	/* Costumes */
	if (known && aware && (o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_COSTUME))
	{
		t = object_desc_str(t, !(mode & 8) ? " of " : "-");
		t = object_desc_str(t, r_info[o_ptr->bpval].name + r_name);
	}


	/* Indicate "charging" artifacts XXX XXX XXX */
	if (known && o_ptr->timeout && !((o_ptr->tval == TV_LITE) && (f4 & TR4_FUEL_LITE))
	    && !((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH)))
	{
		/* Hack -- Dump " (charging)" if relevant */
		t = object_desc_str(t, !(mode & 8) ? " (charging)" : "(#)");
	}


	/* No more details wanted */
	if ((mode & 7) < 3) {
		if (t - buf <= 65 || mode & 8) {
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
                char *u = tmp_val;

		strcpy(tmp_val, quark_str(o_ptr->note));

                for (; *u && (*u != '#'); u++);
	
		*u = '\0';
	}

	/* Note "cursed" if the item is known to be cursed */
	else if (cursed_p(o_ptr) && (known || (o_ptr->ident & ID_SENSE)))
	{
		strcpy(tmp_val, "cursed");
	}

	/* Mega-Hack -- note empty wands/staffs */
	else if (!known && (o_ptr->ident & ID_EMPTY))
	{
		strcpy(tmp_val, "empty");
	}

	/* Note "tried" if the object has been tested unsuccessfully */
	else if (!aware && object_tried_p(Ind, o_ptr))
	{
		strcpy(tmp_val, "tried");
	}

	/* Note the discount, if any */
	else if (o_ptr->discount)
	{
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
		if (t - buf <= 65 || mode >= 8) {
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
void object_desc_store(int Ind, char *buf, object_type *o_ptr, int pref, int mode)
{
	player_type *p_ptr = Players[Ind];

	bool hack_aware = FALSE;
	bool hack_known = FALSE;

	/* Only save flags if we have a valid player */
	if (Ind) {
		/* Save the "aware" flag */
		hack_aware = p_ptr->obj_aware[o_ptr->k_idx];

		/* Save the "known" flag */
		hack_known = (o_ptr->ident & ID_KNOWN) ? TRUE : FALSE;
	}

	/* Set the "known" flag */
	o_ptr->ident |= ID_KNOWN;

	/* Valid players only */
	if (Ind) {
		/* Force "aware" for description */
		p_ptr->obj_aware[o_ptr->k_idx] = TRUE;
	}


	/* Describe the object */
	object_desc(Ind, buf, o_ptr, pref, mode);

	/* Only restore flags if we have a valid player */
	if (Ind) {
		/* Restore "aware" flag */
		p_ptr->obj_aware[o_ptr->k_idx] = hack_aware;

		/* Clear the known flag */
		if (!hack_known) o_ptr->ident &= ~ID_KNOWN;
	}
}




/*
 * Determine the "Activation" (if any) for an artifact
 * Return a string, or NULL for "no activation"
 */
cptr item_activation(object_type *o_ptr)
{
	    u32b f1, f2, f3, f4, f5, esp;

        /* Needed hacks */
        //static char rspell[2][80];

			  /* Extract the flags */
			  object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	/* Require activation ability */
	if (!(f3 & TR3_ACTIVATE)) return (NULL);

	/* Require activation ability */
	if (o_ptr->tval == TV_DRAG_ARMOR)
	  {
	    /* Branch on the sub-type */
	    switch (o_ptr->sval)
	      {
	      case SV_DRAGON_BLUE:
		{
		    return "Breathe lightning every 200+d100 turns";
//		  return "polymorph into an Ancient Blue Dragon every 200+d100 turns";
		}
	      case SV_DRAGON_WHITE:
		{
		    return "Breathe frost every 200+d100 turns";
//		  return "polymorph into an Ancient White Dragon every 200+d100 turns";
		}
	      case SV_DRAGON_BLACK:
		{
		    return "Breathe acid every 200+d100 turns";
//		  return "polymorph into an Ancient Black Dragon every 200+d100 turns";
		}
	      case SV_DRAGON_GREEN:
		{
		    return "Breathe poison every 200+d100 turns";
//		  return "polymorph into an Ancient Green Dragon every 200+d100 turns";
		}
	      case SV_DRAGON_RED:
		{
		    return "Breathe fire every 200+d100 turns";
//		  return "polymorph into an Ancient Red Dragon every 200+d100 turns";
		}
	      case SV_DRAGON_MULTIHUED:
		{
		    return "Breathe the elements every 200+d100 turns";
//		  return "polymorph into an Ancient MultiHued Dragon every 200+d100 turns";
		}
	      case SV_DRAGON_PSEUDO:
		{
		    return "Breathe light/dark every 200+d100 turns";
//		  return "polymorph into an Ethereal Drake every 200+d100 turns";
		  //return "polymorph into a Pseudo Dragon every 200+d100 turns";
		}
	      case SV_DRAGON_BRONZE:
		{
		    return "Breathe confusion every 200+d100 turns";
//		  return "polymorph into an Ancient Bronze Dragon every 200+d100 turns";
		}
	      case SV_DRAGON_GOLD:
		{
		    return "Breathe sound every 200+d100 turns";
//		  return "polymorph into an Ancient Gold Dragon every 200+d100 turns";
		}
	      case SV_DRAGON_CHAOS:
		{
		    return "Breathe chaos every 200+d100 turns";
//		  return "polymorph into a Great Wyrm of Chaos every 200+d100 turns";
		}
	      case SV_DRAGON_LAW:
		{
		    return "Breathe shards/sound every 200+d100 turns";
//		  return "polymorph into a Great Wyrm of Law every 200+d100 turns";
		}
	      case SV_DRAGON_BALANCE:
		{
		    return "Breathe disenchantment every 200+d100 turns";
//		  return "polymorph into a Great Wyrm of Balance every 200+d100 turns";
		}
	      case SV_DRAGON_SHINING:
		{
		    return "Breathe light/dark every 200+d100 turns";
//		  return "polymorph into an Ethereal Dragon every 200+d100 turns";
		}
	      case SV_DRAGON_POWER:
		{
		    return "Breathe havoc every 200+d100 turns";
//		  return "polymorph into a Great Wyrm of Power every 200+d100 turns";
		}
	      case SV_DRAGON_DEATH:
		{
		    return "Breathe nether every 200+d100 turns";
//		  return "polymorph into a death drake every 200+d100 turns";
		}
	      case SV_DRAGON_CRYSTAL:
		{
		    return "Breathe shards every 200+d100 turns";
//		  return "polymorph into a great crystal drake every 200+d100 turns";
		}
	      case SV_DRAGON_DRACOLICH:
		{
		    return "Breathe nether/cold every 200+d100 turns";
//		  return "polymorph into a dracolich every 200+d100 turns";
		}
	      case SV_DRAGON_DRACOLISK:
		{
		    return "Breathe fire/nexus every 200+d100 turns";
//		  return "polymorph into a dracolisk every 200+d100 turns";
		}
	      case SV_DRAGON_SKY:
		{
		    return "Breathe electricity/light/gravity every 200+d100 turns";
//		  return "polymorph into a sky drake every 200+d100 turns";
		}
	      case SV_DRAGON_SILVER:
		{
		    return "Breathe inertia/cold every 200+d100 turns";
//		  return "polymorph into an Ancient Gold Dragon every 200+d100 turns";
		}
	      }
	  }
	
	/* Some artifacts can be activated */
	switch (o_ptr->name1)
	{
		case ART_EOL:
		{
			return "mana bolt (9d8) 7+d7 turns";
		}
		case ART_SKULLCLEAVER:
		{
			return "destruction every 200+d200 turns";
		}
		case ART_HARADRIM:
		{
			return "berserk strength every 50+d50 turns";
		}
		case ART_FUNDIN:
		{
			return "dispel evil (x4) every 100+d100 turns";
		}
		case ART_NAIN:
		{
			return "stone to mud every 7+d5 turns";
		}
		case ART_CELEBRIMBOR:
		{
			return "temporary ESP (dur 20+d20) every 50+d20 turns";
		}
		case ART_UMBAR:
		{
			return "magic arrow (10d10) every 20+d20 turns";
		}
		case ART_GILGALAD:
		{
			return "starlight (75) every 75+d75 turns";
		}
		case ART_HIMRING:
		{
			return "protect evil (dur level*3 + d25) every 225+d225 turns";
		}
		case ART_NUMENOR:
		{
			return "analyze monster every 500+d100 turns";
		}
		case ART_NARTHANC:
		{
			return "fire bolt (9d8) every 8+d8 turns";
		}
		case ART_NIMTHANC:
		{
			return "frost bolt (6d8) every 7+d7 turns";
		}
		case ART_DETHANC:
		{
			return "lightning bolt (4d8) every 6+d6 turns";
		}
		case ART_RILIA:
		{
			return "stinking cloud (16) every 4+d4 turns";
		}
		case ART_BELANGIL:
		{
			return "frost ball (48) every 5+d5 turns";
		}
		case ART_DAL:
		{
			return "remove fear and cure poison every 5 turns";
		}
		case ART_RINGIL:
		{
			return "frost ball (100) every 300 turns";
		}
		case ART_ERU:
		{
			return "healing(7000), curing every 500 turns";
		}
		case ART_DAWN:
		{
			return "summon the Legion of the Dawn every 500+d500 turns";
		}
		case ART_ANDURIL:
		{
			return "fire ball (72) every 400 turns";
		}
		case ART_FIRESTAR:
		{
			return "large fire ball (72) every 100 turns";
		}
		case ART_FEANOR:
		{
			return "haste self (20+d20 turns) every 200 turns";
		}
		case ART_THEODEN:
		{
			return "drain life (120) every 400 turns";
		}
		case ART_TURMIL:
		{
			return "drain life (90) every 70 turns";
		}
		case ART_CASPANION:
		{
			return "door and trap destruction every 10 turns";
		}
		case ART_AVAVIR:
		{
			return "word of recall every 200 turns";
		}
		case ART_TARATOL:
		{
			return "haste self (20+d20 turns) every 100+d100 turns";
		}
		case ART_ERIRIL:
		{
			return "identify every 10 turns";
		}
		case ART_OLORIN:
		{
			return "probing, detection and full id every 1000 turns";
		}
		case ART_EONWE:
		{
			return "mass genocide every 1000 turns";
		}
		case ART_LOTHARANG:
		{
			return "cure wounds (4d7) every 3+d3 turns";
		}
		case ART_CUBRAGOL:
		{
			return "fire branding of bolts every 999 turns";
		}
		case ART_ANGUIREL:
		{
			return "a getaway every 35 turns";
		}
		case ART_AEGLOS:
		{
			return "lightning ball (100) every 500 turns";
		}
		case ART_OROME:
		{
			return "stone to mud every 5 turns";
		}
		case ART_SOULKEEPER:
		{
			return "heal (1000) every 888 turns";
		}
		case ART_BELEGENNON:
		{
			return ("heal (777), curing and heroism every 300 turns");
		}
		case ART_CELEBORN:
		{
			return "genocide every 500 turns";
		}
		case ART_LUTHIEN:
		{
			return "restore life levels every 450 turns";
		}
		case ART_ULMO:
		{
			return "teleport away every 150 turns";
		}
		case ART_COLLUIN:
		{
			return "resistance (20+d20 turns) every 111 turns";
		}
		case ART_HOLCOLLETH:
		{
			return "Sleep II every 55 turns";
		}
		case ART_THINGOL:
		{
			return "recharge item I every 70 turns";
		}
		case ART_COLANNON:
		{
			return "teleport every 45 turns";
		}
		case ART_TOTILA:
		{
			return "confuse monster every 15 turns";
		}
		case ART_CAMMITHRIM:
		{
			return "magic missile (2d6) every 2 turns";
		}
		case ART_PAURHACH:
		{
			return "fire bolt (9d8) every 8+d8 turns";
		}
		case ART_PAURNIMMEN:
		{
			return "frost bolt (6d8) every 7+d7 turns";
		}
		case ART_PAURAEGEN:
		{
			return "lightning bolt (4d8) every 6+d6 turns";
		}
		case ART_PAURNEN:
		{
			return "acid bolt (5d8) every 5+d5 turns";
		}
		case ART_FINGOLFIN:
		{
			return "a magical arrow (150) every 90+d90 turns";
		}
		case ART_HOLHENNETH:
		{
			return "detection every 55+d55 turns";
		}
		case ART_GONDOR:
		{
			return "heal (700) every 250 turns";
		}
		case ART_RAZORBACK:
		{
			return "star ball (150) every 1000 turns";
		}
		case ART_BLADETURNER:
		{
			return "invulnerability (4+d8) every 800 turns";
		}
		case ART_MEDIATOR:
		{
			return "breathe elements (300), berserk rage, bless, and resistance";
		}
		case ART_KNOWLEDGE:
		{
			return "whispers from beyond 100+d200 turns";
		}
		case ART_GALADRIEL:
		{
			return "illumination every 10+d10 turns";
		}
		case ART_UNDEATH:
		{
			return "ruination every 10+d10 turns";
		}
		case ART_ELENDIL:
		{
			return "magic mapping and light every 50+d50 turns";
		}
		case ART_THRAIN:
		{
			return "detection every 30+d30 turns";
		}
		case ART_INGWE:
		{
			return "dispel evil (x5) every 300+d300 turns";
		}
		case ART_CARLAMMAS:
		{
			return "protection from evil every 225+d225 turns";
		}
		case ART_FLAR:
		{
			return "dimension door every 100 turns";
		}
		case ART_BARAHIR:
		{
			return "dispel small life every 55+d55 turns";
		}
		case ART_TULKAS:
		{
			return "haste self (75+d75 turns) every 150+d150 turns";
		}
		case ART_NARYA:
		{
			return "large fire ball (120) every 225+d225 turns";
		}
		case ART_NENYA:
		{
			return "large frost ball (200) every 325+d325 turns";
		}
		case ART_VILYA:
		{
			return "large lightning ball (250) every 425+d425 turns";
		}
#if 0	// implement me!
		case ART_NARYA:
		{
			return "healing (500) every 200+d100 turns";
		}
		case ART_NENYA:
		{
			return "healing (800) every 100+d200 turns";
		}
		case ART_VILYA:
		{
			return "greater healing (900) every 200+d200 turns";
		}
#endif	// 0
		case ART_POWER:
		{
			return "powerful things";
		}
		case ART_STONE_LORE:
		{
			return "perilous identify every turn";
		}
		case ART_DOR: case ART_GORLIM:
		{
			return "rays of fear in every direction";
		}
		case ART_GANDALF:
		{
			return "restore mana every 666 turns";
		}
		case ART_EVENSTAR:
		{
			return "restore every 150 turns";
		}
		case ART_ELESSAR:
		{
			return "heal and cure black breath every 200 turns";
		}
		case ART_MARDA:
		{
			return "summon a dragonrider every 1000 turns";
		}
		case ART_PALANTIR_ITHIL:
		case ART_PALANTIR:
		{
			return "clairvoyance every 100+d100 turns";
		}
#if 0
		case ART_ROBINTON:
		{
			return music_info[3].desc;
		}
		case ART_PIEMUR:
		{
			return music_info[9].desc;
		}
		case ART_MENOLLY:
		{
			return music_info[10].desc;
		}
#endif	// 0
		case ART_EREBOR:
		{
			return "open a secret passage every 75 turns";
		}
		case ART_DRUEDAIN:
		{
			return "detection every 150 turns";
		}
		case ART_ROHAN:
		{
			return "heroism, berserker, haste every 250 turns";
		}
		case ART_HELM:
		{
			return "sound ball (300) every 400 turns";
		}
		case ART_BOROMIR:
		{
			return "mass human summoning every 1000 turns";
		}
		case ART_HURIN:
		{
			return "berserker and +10 to speed (50) every 100+d200 turns";
		}
		case ART_AXE_GOTHMOG:
		{
			return "fire ball (300) every 200+d200 turns";
		}
		case ART_MELKOR:
		{
			return "darkness ball (150) every 100 turns";
		}
		case ART_GROND:
		{
			return "alter reality every 100 turns";
		}
		case ART_ORCHAST:
		{
			return "detect orcs every 10 turns";
		}
		case ART_NIGHT:
		{
			return "drain life (3*100) every 250 turns";
		}
		case ART_NATUREBANE:
		{
			return "dispel monsters (300) every 200+d200 turns";
		}
		case ART_BILBO:
		{
			return "destroy doors and traps every 30+d30 turns";
		}
		case ART_SOULCURE:
		{
			return "holy prayer (+30 AC) every 150+d200 turns";
		}
		case ART_AMUGROM:
		{
			return "temporary resistance boost every 250+d200 turns";
		}
		case ART_HELLFIRE:
		{
			return "invoke raw chaos every 250+d200 turns";
		}
		case ART_SPIRITSHARD:
		{
			return "turn into a wraith every 300+d100 turns";
		}
		case ART_PHASING:
		{
			return "open the final gate to the shores of Valinor";
		}
	}

	// requires some substitution..
	/* Some ego items can be activated */
	switch (o_ptr->name2)
	{
		case EGO_CLOAK_LORDLY_RES:
		{
			return "resistance every 150+d50 turns";
		}
	}

	/* divers */
	if (is_ego_p(o_ptr, EGO_DRAGON))
	{
		return "teleport every 50+d50 turns";
	}
	if (is_ego_p(o_ptr, EGO_JUMP))
	{
		return "phasing every 10+d10 turns";
	}
	if (is_ego_p(o_ptr, EGO_SPINNING))
	{
		return "spinning around every 50+d25 turns";
	}
	if (is_ego_p(o_ptr, EGO_FURY))
	{
		return "grow a fury every 100+d50 turns";
	}
	if (is_ego_p(o_ptr, EGO_NOLDOR))
	{
		return "detect treasure every 10+d20 turns";
	}
	if (is_ego_p(o_ptr, EGO_SPECTRAL))
	{
		return "wraith-form every 50+d50 turns";
	}
	if (o_ptr->tval == TV_RING)
	{
		switch(o_ptr->sval)
		{
			case SV_RING_ELEC:
				return "ball of lightning and resist lightning";
			case SV_RING_FLAMES:
				return "ball of fire and resist fire";
			case SV_RING_ICE:
				return "ball of cold and resist cold";
			case SV_RING_ACID:
				return "ball of acid and resist acid";
			case SV_RING_TELEPORTATION:
				return "teleportation and destruction of the ring";
			case SV_RING_POLYMORPH:
				if (o_ptr->pval)
					return format("polymorph into %s", r_info[o_ptr->pval].name + r_name);
				else
					return "memorize the form you are mimicing";
			default:
				return NULL;
		}
	}

	if (o_ptr->tval == TV_AMULET)
	{
		switch(o_ptr->sval)
		{
			/* The amulet of the moon can be activated for sleep */
			case SV_AMULET_THE_MOON:
				return "sleep monsters every 100+d100 turns";
			case SV_AMULET_SERPENT:
				return "venom breathing every 40+d60 turns";
			case SV_AMULET_RAGE:
				return "grow a fury every 150+d100 turns";
			default:
				return NULL;
		}
	}

	/* For the moment ignore (non-ego) randarts */
	if (o_ptr->name1 == ART_RANDART) return "a crash ;-)";

	/* Oops */
	return NULL;
}

/*
 * Display the damage done with a multiplier
 */
//void output_dam(object_type *o_ptr, int mult, int mult2, cptr against, cptr against2, bool *first)
static void output_dam(int Ind, FILE *fff, object_type *o_ptr, int mult, int mult2, cptr against, cptr against2)
{
	player_type *p_ptr = Players[Ind];
	int dam;

	dam = ((o_ptr->dd + (o_ptr->dd * o_ptr->ds)) * 5L * mult) / FACTOR_MULT;
	dam += (o_ptr->to_d + p_ptr->to_d + p_ptr->to_d_melee) * 10;
	dam *= p_ptr->num_blow;
	if (dam > 0)
	{
		if (dam % 10)
			fprintf(fff, "    %d.%d", dam / 10, dam % 10);
		else
			fprintf(fff, "    %d", dam / 10);
	}
	else
		fprintf(fff, "    0");
	fprintf(fff, " against %s", against);

	if (mult2)
	{
		fprintf(fff, "\n");
		dam = ((o_ptr->dd + (o_ptr->dd * o_ptr->ds)) * 5L * mult2) / FACTOR_MULT;
		dam += (o_ptr->to_d + p_ptr->to_d + p_ptr->to_d_melee) * 10;
		dam *= p_ptr->num_blow;
		if (dam > 0)
		{
			if (dam % 10)
				fprintf(fff, "    %d.%d", dam / 10, dam % 10);
			else
				fprintf(fff, "    %d", dam / 10);
		}
		else
			fprintf(fff, "    0");
		fprintf(fff, " against %s", against2);
	}
	fprintf(fff, "\n");
}


/* XXX this ignores the chance of extra dmg via 'critical hit' */
/* the following stuff is a really bad hack, because the player's real in-game values
   will be modified. TODO: definitely change this to an independant calculation - C. Blue */
static void display_weapon_damage(int Ind, object_type *o_ptr, FILE *fff)
{
	player_type *p_ptr = Players[Ind];
	object_type forge, forge2, *old_ptr = &forge, *old_ptr2 = &forge2;
	u32b f1, f2, f3, f4, f5, esp;
	bool first = TRUE;

	/* save timed effects that might be changed on weapon switching - C. Blue */
	long tim_wraith = p_ptr->tim_wraith;

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

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

	/* handle secondary weapon or shield */
	object_copy(old_ptr2, &p_ptr->inventory[INVEN_ARM]);
	/* take care of MUST2H if player still has a shield or secondary weapon equipped */
	if (f4 & TR4_MUST2H) {
		p_ptr->inventory[INVEN_ARM].k_idx = 0; /* temporarily delete */
	}
	/* take care of SHOULD2H if player is dual-wielding */
	else if ((f4 & TR4_SHOULD2H) && (is_weapon(p_ptr->inventory[INVEN_ARM].tval))) {
		p_ptr->inventory[INVEN_ARM].k_idx = 0; /* temporarily delete */
	}

	/* Hack -- hush the messages up */
	suppress_message = TRUE;
	calc_boni(Ind);

	fprintf(fff, "\n");
//	/* give weight warning, so player won't buy something he can't use. (todo: for shields and bows too) */
//	if (p_ptr->heavy_wield) fprintf(fff, "\377rThis weapon is currently too heavy for you to use effectively:\377w\n");
	fprintf(fff, "\377sUsing it you would have \377W%d\377s blow%s and do an average damage per round of:\n", p_ptr->num_blow, (p_ptr->num_blow > 1) ? "s" : "");

	if (f1 & TR1_SLAY_ANIMAL) output_dam(Ind, fff, o_ptr, FACTOR_HURT, 0, "animals", NULL);
	if (f1 & TR1_SLAY_EVIL) output_dam(Ind, fff, o_ptr, FACTOR_HURT, 0, "evil creatures", NULL);
	if (f1 & TR1_SLAY_ORC) output_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, "orcs", NULL);
	if (f1 & TR1_SLAY_TROLL) output_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, "trolls", NULL);
	if (f1 & TR1_SLAY_GIANT) output_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, "giants", NULL);
	if (f1 & TR1_KILL_DRAGON) output_dam(Ind, fff, o_ptr, FACTOR_KILL, 0, "dragons", NULL);
	else if (f1 & TR1_SLAY_DRAGON) output_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, "dragons", NULL);
	if (f1 & TR1_KILL_UNDEAD) output_dam(Ind, fff, o_ptr, FACTOR_KILL, 0, "undeads", NULL);
	else if (f1 & TR1_SLAY_UNDEAD) output_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, "undeads", NULL);
	if (f1 & TR1_KILL_DEMON) output_dam(Ind, fff, o_ptr, FACTOR_KILL, 0, "demons", NULL);
	else if (f1 & TR1_SLAY_DEMON) output_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, "demons", NULL);

	if (f1 & TR1_BRAND_FIRE) output_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, "non fire resistant creatures", "fire susceptible creatures");
	if (f1 & TR1_BRAND_COLD) output_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, "non cold resistant creatures", "cold susceptible creatures");
	if (f1 & TR1_BRAND_ELEC) output_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, "non lightning resistant creatures", "lightning susceptible creatures");
	if (f1 & TR1_BRAND_ACID) output_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, "non acid resistant creatures", "acid susceptible creatures");
	if (f1 & TR1_BRAND_POIS) output_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, "non poison resistant creatures", "poison susceptible creatures");

	output_dam(Ind, fff, o_ptr, FACTOR_MULT, 0, (first) ? "all monsters" : "other monsters", NULL);

	fprintf(fff, "\n");

	/* restore secondary weapon or shield */
	object_copy(&p_ptr->inventory[INVEN_ARM], old_ptr2);
	/* get our weapon back */
	object_copy(&p_ptr->inventory[INVEN_WIELD], old_ptr);
	calc_boni(Ind);

	/* restore timed effects that might have been changed from the weapon switching - C. Blue */
	p_ptr->tim_wraith = tim_wraith;

	suppress_message = FALSE;
}

/*
 * Display the ammo damage done with a multiplier
 */
//void output_ammo_dam(object_type *o_ptr, int mult, int mult2, cptr against, cptr against2, bool *first)
static void output_ammo_dam(int Ind, FILE *fff, object_type *o_ptr, int mult, int mult2, cptr against, cptr against2)
{
	player_type *p_ptr = Players[Ind];
	long dam;
	object_type *b_ptr = &p_ptr->inventory[INVEN_BOW];
	int tmul = get_shooter_mult(o_ptr);
	tmul += p_ptr->xtra_might;

	dam = (o_ptr->dd + (o_ptr->dd * o_ptr->ds)) * 5;
	dam += (o_ptr->to_d + b_ptr->to_d) * 10;
	dam *= tmul;
	dam += (p_ptr->to_d_ranged) * 10;
	dam *= FACTOR_MULT + ((mult - FACTOR_MULT) * 2) / 5;
	dam /= FACTOR_MULT;
	if (dam > 0) {
		if (dam % 10)
			fprintf(fff, "    %ld.%ld", dam / 10, dam % 10);
		else
			fprintf(fff, "    %ld", dam / 10);
	}
	else
		fprintf(fff, "    0");
	fprintf(fff, " against %s", against);

	if (mult2)
	{
		fprintf(fff, "\n");
		dam = (o_ptr->dd + (o_ptr->dd * o_ptr->ds)) * 5;
		dam += (o_ptr->to_d + b_ptr->to_d) * 10;
		dam *= tmul;
		dam += (p_ptr->to_d_ranged) * 10;
		dam *= FACTOR_MULT + ((mult2 - FACTOR_MULT) * 2) / 5;
		dam /= FACTOR_MULT;
		if (dam > 0) {
			if (dam % 10)
				fprintf(fff, "    %ld.%ld", dam / 10, dam % 10);
			else
				fprintf(fff, "    %ld", dam / 10);
		}
		else
			fprintf(fff, "    0");
		fprintf(fff, " against %s", against2);
	}
	fprintf(fff, "\n");
}

/*
 * Outputs the damage we do/would do with the current bow and this ammo
 */
/* TODO: tell something about boomerangs */
static void display_ammo_damage(int Ind, object_type *o_ptr, FILE *fff)
{
	player_type *p_ptr = Players[Ind];
	u32b f1, f2, f3, f4, f5, esp;
	object_type forge, *old_ptr = &forge;
	bool first = TRUE;
	// int i;

	/* save timed effects that might be changed on ammo switching - C. Blue */
	long tim_wraith = p_ptr->tim_wraith;

	/* swap hack */
	object_copy(old_ptr, &p_ptr->inventory[INVEN_AMMO]);
	object_copy(&p_ptr->inventory[INVEN_AMMO], o_ptr);

	/* Hack -- hush the messages up */
	suppress_message = TRUE;
	calc_boni(Ind);

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	fprintf(fff, "\nUsing it with your current shooter you would do an avarage damage per shot of:\n");
	if (f1 & TR1_SLAY_ANIMAL) output_ammo_dam(Ind, fff, o_ptr, FACTOR_HURT, 0, "animals", NULL);
	if (f1 & TR1_SLAY_EVIL) output_ammo_dam(Ind, fff, o_ptr, FACTOR_HURT, 0, "evil creatures", NULL);
	if (f1 & TR1_SLAY_ORC) output_ammo_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, "orcs", NULL);
	if (f1 & TR1_SLAY_TROLL) output_ammo_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, "trolls", NULL);
	if (f1 & TR1_SLAY_GIANT) output_ammo_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, "giants", NULL);
	if (f1 & TR1_KILL_DRAGON) output_ammo_dam(Ind, fff, o_ptr, FACTOR_KILL, 0, "dragons", NULL);
	else if (f1 & TR1_SLAY_DRAGON) output_ammo_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, "dragons", NULL);
	if (f1 & TR1_KILL_UNDEAD) output_ammo_dam(Ind, fff, o_ptr, FACTOR_KILL, 0, "undeads", NULL);
	else if (f1 & TR1_SLAY_UNDEAD) output_ammo_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, "undeads", NULL);
	if (f1 & TR1_KILL_DEMON) output_ammo_dam(Ind, fff, o_ptr, FACTOR_KILL, 0, "demons", NULL);
	else if (f1 & TR1_SLAY_DEMON) output_ammo_dam(Ind, fff, o_ptr, FACTOR_SLAY, 0, "demons", NULL);

	if (f1 & TR1_BRAND_FIRE) output_ammo_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, "non fire resistant creatures", "fire susceptible creatures");
	if (f1 & TR1_BRAND_COLD) output_ammo_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, "non cold resistant creatures", "cold susceptible creatures");
	if (f1 & TR1_BRAND_ELEC) output_ammo_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, "non lightning resistant creatures", "lightning susceptible creatures");
	if (f1 & TR1_BRAND_ACID) output_ammo_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, "non acid resistant creatures", "acid susceptible creatures");
	if (f1 & TR1_BRAND_POIS) output_ammo_dam(Ind, fff, o_ptr, FACTOR_BRAND, FACTOR_BRAND_SUSC, "non poison resistant creatures", "poison susceptible creatures");

	output_ammo_dam(Ind, fff, o_ptr, FACTOR_MULT, 0, (first) ? "all monsters" : "other monsters", NULL);
	fprintf(fff, "\n");

#if 0
	if (o_ptr->pval2) {
		roff("The explosion will be ");
		i = 0;
		while (gf_names[i].gf != -1)
		{
			if (gf_names[i].gf == o_ptr->pval2)
				break;
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
}

/* display how much AC an armour would add for us (for MA users!) and
   especially if it would encumber us, so we won't have to guess in shops - C. Blue */
static void display_armour_handling(int Ind, object_type *o_ptr, FILE *fff, bool star_identify)
{
	player_type *p_ptr = Players[Ind], p_backup;
	object_type forge, *old_ptr = &forge;
	int slot = wield_slot(Ind, o_ptr); /* slot the item goes into */

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

	/* since his mana or HP might get changed or even nulled, backup player too! */
	COPY(&p_backup, p_ptr, player_type);

	/* Ok now the hackish stuff, we replace the current armour with this one */
	object_copy(old_ptr, &p_ptr->inventory[slot]);
	object_copy(&p_ptr->inventory[slot], o_ptr);
	if (!star_identify) {
		p_ptr->inventory[slot].name1 = p_ptr->inventory[slot].name2 = p_ptr->inventory[slot].name2b = 0;
		p_ptr->inventory[slot].pval = 0;
	}

	/* Hack -- hush the messages up */
	suppress_message = TRUE;
	calc_boni(Ind);
	calc_mana(Ind);
	suppress_message = FALSE;

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
//			fprintf(fff, "\n");
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
//		if (fff) fprintf(fff, "\n");
	}

	if (fff) fprintf(fff, "\377sUsing it you would have \377W%d\377s total AC.\n", p_ptr->dis_ac + p_ptr->dis_to_a);
//	else msg_format(Ind, "\377s  Using it you would have %d total AC.", p_ptr->dis_ac + p_ptr->dis_to_a);

	/* get our armour back */
	object_copy(&p_ptr->inventory[slot], old_ptr);

	suppress_message = TRUE;
	calc_boni(Ind);
	calc_mana(Ind);
	suppress_message = FALSE;

	/* and get our mana (in most cases the problem) back, yay */
	COPY(p_ptr, &p_backup, player_type);

	/* restore timed effects that might have been changed from the weapon switching */
	p_ptr->tim_wraith = tim_wraith;
}

/* display weapon encumberment changes, so we won't have to guess in shops - C. Blue */
static void display_weapon_handling(int Ind, object_type *o_ptr, FILE *fff, bool star_identify)
{
	player_type *p_ptr = Players[Ind], p_backup;
	object_type forge, *old_ptr = &forge, forge2, *old_ptr2 = &forge2;

	bool old_heavy_wield = p_ptr->heavy_wield;
	bool old_icky_wield = p_ptr->icky_wield;
	bool old_awkward_wield = p_ptr->awkward_wield;
//	bool old_easy_wield = p_ptr->easy_wield;

	/* save timed effects that might be changed on weapon switching */
	long tim_wraith = p_ptr->tim_wraith;

	/* since his mana or HP might get changed or even nulled, backup player too! */
	COPY(&p_backup, p_ptr, player_type);

	/* Ok now the hackish stuff, we replace the current weapon/shield with this one */
	object_copy(old_ptr, &p_ptr->inventory[INVEN_WIELD]);
	object_copy(old_ptr2, &p_ptr->inventory[INVEN_ARM]);
	object_copy(&p_ptr->inventory[INVEN_WIELD], o_ptr);
	if (k_info[o_ptr->k_idx].flags4 & TR4_MUST2H) p_ptr->inventory[INVEN_ARM].k_idx = 0;
	/* If we don't really know the item, rely on k_info flags only! */
	if (!star_identify) {
		p_ptr->inventory[INVEN_WIELD].name1 = p_ptr->inventory[INVEN_WIELD].name2 = p_ptr->inventory[INVEN_WIELD].name2b = 0;
		p_ptr->inventory[INVEN_WIELD].pval = 0;
	}

	/* Hack -- hush the messages up */
	suppress_message = TRUE;
	calc_boni(Ind);
	calc_mana(Ind);
	suppress_message = FALSE;

	/* Gained encumberment? */
	if ((p_ptr->awkward_wield && !old_awkward_wield) ||
	    (p_ptr->heavy_wield && !old_heavy_wield) ||
	    (p_ptr->icky_wield && !old_icky_wield)) {

		if (!old_awkward_wield && !old_heavy_wield && !old_icky_wield) {
			if (fff) fprintf(fff, "\377rWielding this weapon would give you one or more mali:\n");
//			else msg_print(Ind, "\377s  This weapon seems too heavy or in another way unfit for you!");
//see below instead
		} else {
			if (fff) fprintf(fff, "\377rWielding this weapon would give you one or more additional mali:\n");
//			else msg_print(Ind, "\377s  This weapon would seem to give you additional trouble!");
//see below instead
		}

		if (fff) {
			if (p_ptr->awkward_wield && !old_awkward_wield) fprintf(fff, "\377r    One-handed use reduces its efficiency.\n");
			if (k_info[o_ptr->k_idx].flags2 & TR4_COULD2H) fprintf(fff, "\377r    Wielding it two-handedly might make it even more effective.\n");
			if (p_ptr->heavy_wield && !old_heavy_wield) fprintf(fff, "\377r    Your strength is insufficient to hold it properly.\n");
			if (p_ptr->icky_wield && !old_icky_wield) fprintf(fff, "\377r    It is edged and not blessed.\n");
//			fprintf(fff, "\n");
		}
//we get this info although we didn't *id*!
		else {
			if (p_ptr->awkward_wield && !old_awkward_wield) msg_print(Ind, "\377s  One-handed use reduces its efficiency.");
			if (k_info[o_ptr->k_idx].flags2 & TR4_COULD2H) msg_print(Ind, "\377s  Wielding it two-handedly might make it even more effective.");
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
				if (!p_ptr->icky_wield && old_icky_wield) fprintf(fff, "\377g    It is not edged, or at least blessed.\n");
			}
		}
//		if (fff) fprintf(fff, "\n");
	}

	/* get our weapon back */
	object_copy(&p_ptr->inventory[INVEN_WIELD], old_ptr);
	object_copy(&p_ptr->inventory[INVEN_ARM], old_ptr2);

	suppress_message = TRUE;
	calc_boni(Ind);
	calc_mana(Ind);
	suppress_message = FALSE;

	/* and get our mana (in most cases the problem) back, yay */
	COPY(p_ptr, &p_backup, player_type);

	/* restore timed effects that might have been changed from the weapon switching */
	p_ptr->tim_wraith = tim_wraith;
}

/* display ranged weapon encumberment changes, so we won't have to guess in shops - C. Blue */
static void display_shooter_handling(int Ind, object_type *o_ptr, FILE *fff, bool star_identify)
{
	player_type *p_ptr = Players[Ind], p_backup;
	object_type forge, *old_ptr = &forge;

	bool old_heavy_shoot = p_ptr->heavy_shoot;
//	bool old_awkward_shoot = p_ptr->awkward_shoot;

	/* save timed effects that might be changed on weapon switching */
	long tim_wraith = p_ptr->tim_wraith;

	/* since his mana or HP might get changed or even nulled, backup player too! */
	COPY(&p_backup, p_ptr, player_type);

	/* Ok now the hackish stuff, we replace the current weapon/shield with this one */
	object_copy(old_ptr, &p_ptr->inventory[INVEN_BOW]);
	object_copy(&p_ptr->inventory[INVEN_BOW], o_ptr);
	/* If we don't really know the item, rely on k_info flags only! */
	if (!star_identify) {
		p_ptr->inventory[INVEN_BOW].name1 = p_ptr->inventory[INVEN_BOW].name2 = p_ptr->inventory[INVEN_BOW].name2b = 0;
		p_ptr->inventory[INVEN_BOW].pval = 0;
	}

	/* Hack -- hush the messages up */
	suppress_message = TRUE;
	calc_boni(Ind);
	calc_mana(Ind);
	suppress_message = FALSE;

	/* Gained encumberment? */
	if (p_ptr->heavy_shoot && !old_heavy_shoot) {

		if (!old_heavy_shoot) {
			if (fff) fprintf(fff, "\377rThis weapon currently seems too heavy for you to use!\n");
			else msg_print(Ind, "\377s  This weapon currently seems too heavy for you!");
		}
//			fprintf(fff, "\n");
	}

	/* Lost encumberment? */
	if (!p_ptr->heavy_shoot && old_heavy_shoot) {

		if (!p_ptr->heavy_shoot) {
			if (fff) fprintf(fff, "\377gThis weapon seems light enough for you to use properly.\n");
			else msg_print(Ind, "\377s  This weapon seems light enough for you to use properly.");
		}
//		if (fff) fprintf(fff, "\n");
	}

	/* get our weapon back */
	object_copy(&p_ptr->inventory[INVEN_BOW], old_ptr);

	suppress_message = TRUE;
	calc_boni(Ind);
	calc_mana(Ind);
	suppress_message = FALSE;

	/* and get our mana (in most cases the problem) back, yay */
	COPY(p_ptr, &p_backup, player_type);

	/* restore timed effects that might have been changed from the weapon switching */
	p_ptr->tim_wraith = tim_wraith;
}


/* Display examine (x/I) text of an item we haven't *identified* yet */
void observe_aux(int Ind, object_type *o_ptr) {
	player_type *p_ptr = Players[Ind];
	char o_name[ONAME_LEN];
	u32b f1, f2, f3, f4, f5, esp;
//	int hold;

	/* Description */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);

#ifdef PLAYER_STORES
	if (p_ptr->store_num <= -2 && o_ptr->note) player_stores_cut_inscription(o_name);
	if (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_CHEQUE) {
		msg_format(Ind, "\377sIt's a cheque worth \377y%d\377s gold pieces.", ps_get_cheque_value(o_ptr));
		return;
	}
#endif

	/* Extract some flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	/* Describe */
	msg_format(Ind, "\377s%s:", o_name);
	// ?<-->if (strlen(o_name) > 77) msg_format(Ind, "\377s%s:", o_name + 77);

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

	if (f4 & TR4_SHOULD2H) msg_print(Ind, "\377s  It should be wielded two-handed.");
	else if (f4 & TR4_MUST2H) msg_print(Ind, "\377s  It must be wielded two-handed.");
	else if (f4 & TR4_COULD2H) {
	if (o_ptr->weight <= 999)
		msg_print(Ind, "\377s  It may be wielded two-handed or dual.");
	else
		msg_print(Ind, "\377s  It may be wielded two-handed.");
	}
	else if (is_weapon(o_ptr->tval) && o_ptr->weight <= 80) msg_print(Ind, "\377s  It may be dual-wielded.");

	if (o_ptr->tval == TV_BOW) display_shooter_handling(Ind, o_ptr, NULL, FALSE);
	else if (is_weapon(o_ptr->tval)) display_weapon_handling(Ind, o_ptr, NULL, FALSE);
	else if (is_armour(o_ptr->tval)) display_armour_handling(Ind, o_ptr, NULL, FALSE);

	if (wield_slot(Ind, o_ptr) == INVEN_WIELD) {
		/* copied from object1.c.. */
		object_type forge, forge2, *old_ptr = &forge, *old_ptr2 = &forge2;
		long tim_wraith = p_ptr->tim_wraith;
		object_copy(old_ptr, &p_ptr->inventory[INVEN_WIELD]);
		object_copy(&p_ptr->inventory[INVEN_WIELD], o_ptr);
		object_copy(old_ptr2, &p_ptr->inventory[INVEN_ARM]);
		if (k_info[o_ptr->k_idx].flags4 & TR4_MUST2H)
			p_ptr->inventory[INVEN_ARM].k_idx = 0;
		else if ((k_info[o_ptr->k_idx].flags4 & TR4_SHOULD2H) &&
		    (is_weapon(p_ptr->inventory[INVEN_ARM].tval)))
			p_ptr->inventory[INVEN_ARM].k_idx = 0;
		suppress_message = TRUE;
		calc_boni(Ind);
		msg_format(Ind, "\377s  With it, you can usually attack %d time%s/turn.",
		    p_ptr->num_blow, p_ptr->num_blow > 1 ? "s" : "");
		object_copy(&p_ptr->inventory[INVEN_ARM], old_ptr2);
		object_copy(&p_ptr->inventory[INVEN_WIELD], old_ptr);
		calc_boni(Ind);
		suppress_message = FALSE;
		p_ptr->tim_wraith = tim_wraith;
	}

	if (wield_slot(Ind, o_ptr) != INVEN_WIELD
	    //&& !is_armour(o_ptr->tval)
	    )
		msg_print(Ind, "\377s  You have no special knowledge about that item.");
}

/*
 * Describe a "fully identified" item
 */
//bool identify_fully_aux(object_type *o_ptr, FILE *fff)
bool identify_fully_aux(int Ind, object_type *o_ptr)
{
	player_type *p_ptr = Players[Ind];
//	cptr		*info = p_ptr->info;

	int j, am;//, hold;

	u32b f1, f2, f3, f4, f5, esp;

	//        cptr            info[400];
	//        byte            color[400];

	FILE *fff;
	char buf[1024], o_name[ONAME_LEN];
	char *ca_ptr = "";
	char buf_tmp[90];
	int buf_tmp_i, buf_tmp_n;

#if 0
	char file_name[MAX_PATH_LENGTH];

	/* Temporary file */
	if (path_temp(file_name, MAX_PATH_LENGTH)) return;

	strcpy(p_ptr->infofile, file_name);
#endif	// 0

	/* Open a new file */
	fff = my_fopen(p_ptr->infofile, "wb");

	/* Current file viewing */
	strcpy(p_ptr->cur_file, p_ptr->infofile);

	/* Let the player scroll through this info */
	p_ptr->special_file_type = TRUE;

	/* Output color byte */
//	fprintf(fff, "%c", 'w');


	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

        /* Describe the result */
	/* in case we just *ID* it because an admin inspected it */
	if (!(o_ptr->ident & ID_MENTAL)) object_desc(0, o_name, o_ptr, TRUE, 3);
	/* normal players: */
	else object_desc(Ind, o_name, o_ptr, TRUE, 3);

#ifdef USE_SOUND_2010
	sound_item(Ind, o_ptr->tval, o_ptr->sval, "pickup_");
#endif


#ifdef PLAYER_STORES
	if (p_ptr->store_num <= -2 && o_ptr->note) player_stores_cut_inscription(o_name);
	if (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_CHEQUE) {
		fprintf(fff, "\377sIt's a cheque worth \377y%d\377s gold pieces.\n", ps_get_cheque_value(o_ptr));
		my_fclose(fff);

		/* Hack -- anything written? (rewrite me) */
		/* Open a new file */
		fff = my_fopen(p_ptr->infofile, "rb");
		if (my_fgets(fff, buf, 1024, FALSE)) {
			/* Close the file */
			my_fclose(fff);
			return (FALSE);
		}

		/* Close the file */
		my_fclose(fff);

		/* No special effects */
//		if (!i) return (FALSE);

		/* Let the client know it's about to get some info */
		Send_special_other(Ind);

		/* Gave knowledge */
		return (TRUE);
	}
#endif

	if (strlen(o_name) > 79) {
		strncpy(buf_tmp, o_name, 79);
		buf_tmp[79] = 0;
		fprintf(fff, "%s\n", buf_tmp);
		fprintf(fff, "%s\n", o_name + 79);
	} else {
		fprintf(fff, "%s\n", o_name);
	}

	/* in case we just *ID* it because an admin inspected it */
	if (!(o_ptr->ident & ID_MENTAL)) fprintf(fff, "\377y(This item has not been *identified* yet.)\n");

	if (artifact_p(o_ptr)) {
		if (true_artifact_p(o_ptr)) ca_ptr = " true artifact";
		else {
			if (!is_admin(p_ptr)) ca_ptr = " random artifact";
			else ca_ptr = format(" random artifact (ap = %d, %d Au)", artifact_power(randart_make(o_ptr)), object_value_real(0, o_ptr));
		}
	}

	switch(o_ptr->tval){
	case TV_BLUNT:
		fprintf(fff, "It's a%s blunt weapon.\n", ca_ptr); break;
	case TV_POLEARM:
		fprintf(fff, "It's a%s polearm.\n", ca_ptr); break;
	case TV_SWORD:
		fprintf(fff, "It's a%s sword-type weapon.\n", ca_ptr); break;
	case TV_AXE:
		fprintf(fff, "It's a%s axe-type weapon.\n", ca_ptr); break;
	default:
		if (artifact_p(o_ptr))
			fprintf(fff, "It's a%s.\n", ca_ptr);
		break;
	}

	/* todo: Display artifact description text
	   (currently, reading it is disabled in init1.c) */
	if (true_artifact_p(o_ptr)) {
		//see #if 0 stuff below
	}

#if 0
	/* All white */
	for (j = 0; j < 400; j++)
		color[j] = TERM_WHITE;

	/* No need to dump that */
	if (fff == NULL) {
		i = grab_tval_desc(o_ptr->tval, info, 0);
		if ((fff == NULL) && i) info[i++] = "";
		for (j = 0; j < i; j++)
			color[j] = TERM_L_BLUE;
	}

	if (o_ptr->k_idx) {
		char buff2[400], *s, *t;
		int n, oi = i;
		object_kind *k_ptr = &k_info[o_ptr->k_idx];

		strcpy (buff2, k_text + k_ptr->text);

		s = buff2;

		/* Collect the history */
		while (TRUE) {

			/* Extract remaining length */
			n = strlen(s);

			/* All done */
			if (n < 60) {
				/* Save one line of history */
				color[i] = TERM_ORANGE;
				info[i++] = s;

				/* All done */
				break;
			}

			/* Find a reasonable break-point */
			for (n = 60; ((n > 0) && (s[n-1] != ' ')); n--) /* loop */;

			/* Save next location */
			t = s + n;

			/* Wipe trailing spaces */
			while ((n > 0) && (s[n-1] == ' ')) s[--n] = '\0';

			/* Save one line of history */
			color[i] = TERM_ORANGE;
			info[i++] = s;

			s = t;
		}

		/* Add a blank line */
		if ((fff == NULL) && (oi < i)) info[i++] = "";
	}

	if (o_ptr->name1) {

		char buff2[400], *s, *t;
		int n, oi = i;
		artifact_type *a_ptr = &a_info[o_ptr->name1];

		strcpy (buff2, a_text + a_ptr->text);

		s = buff2;

		/* Collect the history */
		while (TRUE) {
			/* Extract remaining length */
			n = strlen(s);

			/* All done */
			if (n < 60) {
				/* Save one line of history */
				color[i] = TERM_YELLOW;
				info[i++] = s;

				/* All done */
				break;
			}

			/* Find a reasonable break-point */
			for (n = 60; ((n > 0) && (s[n-1] != ' ')); n--) /* loop */;

			/* Save next location */
			t = s + n;

			/* Wipe trailing spaces */
			while ((n > 0) && (s[n-1] == ' ')) s[--n] = '\0';

			/* Save one line of history */
			color[i] = TERM_YELLOW;
			info[i++] = s;

			s = t;
		}

		/* Add a blank line */
		if ((fff == NULL) && (oi < i)) info[i++] = "";

		if (a_ptr->set != -1) {
			char buff2[400], *s, *t;
			int n;
			set_type *set_ptr = &set_info[a_ptr->set];

			strcpy (buff2, set_text + set_ptr->desc);

			s = buff2;

			/* Collect the history */
			while (TRUE) {
				/* Extract remaining length */
				n = strlen(s);

				/* All done */
				if (n < 60) {
					/* Save one line of history */
					color[i] = TERM_GREEN;
					info[i++] = s;

					/* All done */
					break;
				}

				/* Find a reasonable break-point */
				for (n = 60; ((n > 0) && (s[n-1] != ' ')); n--) /* loop */;

				/* Save next location */
				t = s + n;

				/* Wipe trailing spaces */
				while ((n > 0) && (s[n-1] == ' ')) s[--n] = '\0';

				/* Save one line of history */
				color[i] = TERM_GREEN;
				info[i++] = s;

				s = t;
			}

			/* Add a blank line */
			info[i++] = "";
		}
	}

	if (f5 & TR5_LEVELS) {
		int j = 0;

		color[i] = TERM_VIOLET;
		if (count_bits(o_ptr->pval3) == 0) info[i++] = "It is sentient.";
		else if (count_bits(o_ptr->pval3) > 1) info[i++] = "It is sentient and can have access to the realms of:";
		else  info[i++] = "It is sentient and can have access to the realm of:";

		for (j = 0; j < MAX_FLAG_GROUP; j++) {
			if (BIT(j) & o_ptr->pval3) {
				color[i] = flags_groups[j].color;
				info[i++] = flags_groups[j].name;
			}
		}

		/* Add a blank line */
		info[i++] = "";
	}

#endif
	if (f4 & TR4_SHOULD2H) fprintf(fff, "It should be wielded two-handed.\n");
	else if (f4 & TR4_MUST2H) fprintf(fff, "It must be wielded two-handed.\n");
	else if (f4 & TR4_COULD2H) {
		if (o_ptr->weight <= 999)
			fprintf(fff, "It may be wielded two-handed or dual.\n");
		else
			fprintf(fff, "It may be wielded two-handed.\n");
	}
	else if (is_weapon(o_ptr->tval) && o_ptr->weight <= 80) fprintf(fff, "It may be dual-wielded.\n");

	/* Kings/Queens only warning */
	if (f5 & TR5_WINNERS_ONLY) fprintf(fff, "\377vIt is to be used by royalties exclusively.\377w\n");
	/* Morgoth crown hardcoded note to give a warning!- C. Blue */
	else if (o_ptr->name1 == ART_MORGOTH) {
		fprintf(fff, "\377vIt may only be worn by kings and queens!\377w\n");
	}

	if (o_ptr->tval == TV_BOW) display_shooter_handling(Ind, o_ptr, fff, TRUE);
	else if (is_weapon(o_ptr->tval)) display_weapon_handling(Ind, o_ptr, fff, TRUE);
	else if (is_armour(o_ptr->tval)) display_armour_handling(Ind, o_ptr, fff, TRUE);

	/* Mega Hack^3 -- describe the amulet of life saving */
	if (o_ptr->tval == TV_AMULET && o_ptr->sval == SV_AMULET_LIFE_SAVING)
		fprintf(fff, "It will save your life from perilous scene once.\n");

	/* Mega-Hack -- describe activation */
	if (f3 & TR3_ACTIVATE) {
		cptr activation;
		if (!(activation = item_activation(o_ptr))) {
			/* Mysterious message for items missing description (eg. golem command scrolls) - mikaelh */
			fprintf(fff, "It can be activated.\n");
		} else {
			fprintf(fff, "It can be activated for...\n");
			fprintf(fff, "%s\n", item_activation(o_ptr));
			fprintf(fff, "...if it is being worn.\n");
		}
	}

#if 0
	/* Mega-Hack -- describe activation */
	if (f3 & (TR3_ACTIVATE)) {
		info[i++] = "It can be activated for...";
		if (is_ego_p(o_ptr, EGO_MSTAFF_SPELL))
		{
			info[i++] = item_activation(o_ptr, 1);
			info[i++] = "And...";
		}
		info[i++] = item_activation(o_ptr, 0);

		/* Mega-hack -- get rid of useless line for randarts */
		if (o_ptr->tval != TV_RANDART) {
			info[i++] = "...if it is being worn.";
		}
	}

	/* Granted power */
	if (object_power(o_ptr) != -1) {
		info[i++] = "It grants you the power of...";
		info[i++] = powers_type[object_power(o_ptr)].name;
		info[i++] = "...if it is being worn.";
	}
#endif	// 0


	/* Hack -- describe lite's */
	if (o_ptr->tval == TV_LITE) {
		int radius = 0;

		if (f3 & TR3_LITE1) radius++;
		if (f4 & TR4_LITE2) radius += 2;
		if (f4 & TR4_LITE3) radius += 3;

		if (radius > LITE_CAP) radius = LITE_CAP; /* LITE_MAX ? */

		if (f4 & TR4_FUEL_LITE)
			fprintf(fff, "It provides light (radius %d) when fueled.\n", radius);
		else if (radius)
			fprintf(fff, "It provides light (radius %d) forever.\n", radius);
		else
			fprintf(fff, "It never provides light.\n");
	}

	/* Mega Hack^3 -- describe the Anchor of Space-time */
	if (o_ptr->name1 == ART_ANCHOR)
		fprintf(fff, "It prevents the space-time continuum from being disrupted.\n");

	am = ((f4 & (TR4_ANTIMAGIC_50)) ? 50 : 0)
		+ ((f4 & (TR4_ANTIMAGIC_30)) ? 30 : 0)
		+ ((f4 & (TR4_ANTIMAGIC_20)) ? 20 : 0)
		+ ((f4 & (TR4_ANTIMAGIC_10)) ? 10 : 0);
	if (am)
	{
		j = o_ptr->to_h + o_ptr->to_d;// + o_ptr->pval + o_ptr->to_a;
		if (j > 0) am -= j;
		if (am > 50) am = 50;
	}
#ifdef NEW_ANTIMAGIC_RATIO
	am = (am * 3) / 5;
#endif
/*	if (am >= 100)
		fprintf(fff, "It generates a perfect antimagic field.\n");
	else if (am >= 80)
		fprintf(fff, "It generates a mighty antimagic field.\n");
	else if (am >= 60)
		fprintf(fff, "It generates a strong antimagic field.\n");
	else if (am >= 40)
		fprintf(fff, "It generates an antimagic field.\n");
	else if (am >= 20)
		fprintf(fff, "It generates a mellow antimagic field.\n");
	else if (am > 0) fprintf(fff, "It generates a feeble antimagic field.\n");
*/
	if (am > 0) fprintf(fff, "It generates an antimagic field that has %d%% chance of supressing magic.\n", am);
	if (am < 0) fprintf(fff, "It generates a suppressed antimagic field.\n");

	/* And then describe it fully */

	if (f1 & (TR1_LIFE))
		fprintf(fff, "It affects your hit points.\n");
	if (f1 & (TR1_STR))
		fprintf(fff, "It affects your strength.\n");
	if (f1 & (TR1_INT))
		fprintf(fff, "It affects your intelligence.\n");
	if (f1 & (TR1_WIS))
		fprintf(fff, "It affects your wisdom.\n");
	if (f1 & (TR1_DEX))
		fprintf(fff, "It affects your dexterity.\n");
	if (f1 & (TR1_CON))
		fprintf(fff, "It affects your constitution.\n");
	if (f1 & (TR1_CHR))
		fprintf(fff, "It affects your charisma.\n");

	if (f1 & (TR1_STEALTH)) {
		if (o_ptr->tval != TV_TRAPKIT)
			fprintf(fff, "It affects your stealth.\n");
		else
			fprintf(fff, "It is well-hidden.\n");
	}
	if (f1 & (TR1_SEARCH))
		fprintf(fff, "It affects your searching.\n");
	if (f5 & (TR5_DISARM))
		fprintf(fff, "It affects your disarming.\n");
	if (f1 & (TR1_INFRA))
		fprintf(fff, "It affects your infravision.\n");
	if (f1 & (TR1_TUNNEL))
		fprintf(fff, "It affects your ability to tunnel.\n");
	if (f1 & (TR1_SPEED))
		fprintf(fff, "It affects your speed.\n");
	if (f1 & (TR1_BLOWS))
		fprintf(fff, "It affects your attack speed.\n");
	if (f5 & (TR5_CRIT))
		fprintf(fff, "It affects your ability to score critical hits.\n");
	if (f5 & (TR5_LUCK))
		fprintf(fff, "It affects your luck.\n");

	if (f1 & (TR1_BRAND_ACID))
		fprintf(fff, "It does extra damage from acid.\n");
	if (f1 & (TR1_BRAND_ELEC))
		fprintf(fff, "It does extra damage from electricity.\n");
	if (f1 & (TR1_BRAND_FIRE))
		fprintf(fff, "It does extra damage from fire.\n");
	if (f1 & (TR1_BRAND_COLD))
		fprintf(fff, "It does extra damage from frost.\n");
	if (f1 & (TR1_BRAND_POIS))
		fprintf(fff, "It poisons your foes.\n");
	if (f5 & (TR5_CHAOTIC))
		fprintf(fff, "It produces chaotic effects.\n");
	if (f1 & (TR1_VAMPIRIC))
		fprintf(fff, "It drains life from your foes.\n");
	if (f5 & (TR5_IMPACT))
		fprintf(fff, "It can cause earthquakes.\n");
	if (f5 & (TR5_VORPAL))
		fprintf(fff, "It is very sharp and can cut your foes.\n");
	if (f5 & (TR5_WOUNDING))
		fprintf(fff, "It is very sharp and makes your foes bleed.\n");
	if (f1 & (TR1_SLAY_ORC))
		fprintf(fff, "It is especially deadly against orcs.\n");
	if (f1 & (TR1_SLAY_TROLL))
		fprintf(fff, "It is especially deadly against trolls.\n");
	if (f1 & (TR1_SLAY_GIANT))
		fprintf(fff, "It is especially deadly against giants.\n");
	if (f1 & (TR1_SLAY_ANIMAL))
		fprintf(fff, "It is especially deadly against natural creatures.\n");
	if (f1 & (TR1_KILL_UNDEAD))
		fprintf(fff, "It is a great bane of undead.\n");
	else if (f1 & (TR1_SLAY_UNDEAD))
		fprintf(fff, "It strikes at undead with holy wrath.\n");
	if (f1 & (TR1_KILL_DEMON))
		fprintf(fff, "It is a great bane of demons.\n");
	else if (f1 & (TR1_SLAY_DEMON))
		fprintf(fff, "It strikes at demons with holy wrath.\n");
	if (f1 & (TR1_KILL_DRAGON))
		fprintf(fff, "It is a great bane of dragons.\n");
	else if (f1 & (TR1_SLAY_DRAGON))
		fprintf(fff, "It is especially deadly against dragons.\n");
	if (f1 & (TR1_SLAY_EVIL))
		fprintf(fff, "It fights against evil with holy fury.\n");
	if (f1 & (TR1_MANA))
		fprintf(fff, "It affects your mana capacity.\n");
	if (f1 & (TR1_SPELL))
		fprintf(fff, "It affects your spell power.\n");
	if (f5 & (TR5_INVIS))
		fprintf(fff, "It makes you invisible.\n");
	if (o_ptr->tval != TV_TRAPKIT) {
		if (f2 & (TR2_SUST_STR))
			fprintf(fff, "It sustains your strength.\n");
		if (f2 & (TR2_SUST_INT))
			fprintf(fff, "It sustains your intelligence.\n");
		if (f2 & (TR2_SUST_WIS))
			fprintf(fff, "It sustains your wisdom.\n");
		if (f2 & (TR2_SUST_DEX))
			fprintf(fff, "It sustains your dexterity.\n");
		if (f2 & (TR2_SUST_CON))
			fprintf(fff, "It sustains your constitution.\n");
		if (f2 & (TR2_SUST_CHR))
			fprintf(fff, "It sustains your charisma.\n");
		if (f2 & (TR2_IM_FIRE))
			fprintf(fff, "It provides immunity to fire.\n");
		if (f2 & (TR2_IM_COLD))
			fprintf(fff, "It provides immunity to cold.\n");
		if (f2 & (TR2_IM_ELEC))
			fprintf(fff, "It provides immunity to electricity.\n");
		if (f2 & (TR2_IM_ACID))
			fprintf(fff, "It provides immunity to acid.\n");
	}
#if 1
	else {
		if (f2 & (TRAP2_AUTOMATIC_5))
			fprintf(fff, "It can rearm itself.\n");
		if (f2 & (TRAP2_AUTOMATIC_99))
			fprintf(fff, "It rearms itself.\n");
		if (f2 & (TRAP2_KILL_GHOST))
			fprintf(fff, "It is effective against Ghosts.\n");
		if (f2 & (TRAP2_TELEPORT_TO))
			fprintf(fff, "It can teleport monsters to you.\n");
		if (f2 & (TRAP2_ONLY_DRAGON))
			fprintf(fff, "It can only be set off by dragons.\n");
		if (f2 & (TRAP2_ONLY_DEMON))
			fprintf(fff, "It can only be set off by demons.\n");
		if (f2 & (TRAP2_ONLY_UNDEAD))
			fprintf(fff, "It can only be set off by undead.\n");
		if (f2 & (TRAP2_ONLY_ANIMAL))
			fprintf(fff, "It can only be set off by animals.\n");
		if (f2 & (TRAP2_ONLY_EVIL))
			fprintf(fff, "It can only be set off by evil creatures.\n");
	}
#endif
	if (f5 & (TR5_IM_POISON))
		fprintf(fff, "It provides immunity to poison.\n");

        if (o_ptr->tval == TV_DRAG_ARMOR && o_ptr->sval == SV_DRAGON_MULTIHUED) {
		if (!(f2 & (TR2_IM_FIRE))) {
		        if (o_ptr->xtra2 & 0x01) fprintf(fff, "It provides immunity to fire.\n");
			else fprintf(fff, "It provides resistance to fire.\n");
		}
		if (!(f2 & (TR2_IM_COLD))) {
		        if (o_ptr->xtra2 & 0x02) fprintf(fff, "It provides immunity to cold.\n");
			else fprintf(fff, "It provides resistance to cold.\n");
		}
		if (!(f2 & (TR2_IM_ELEC))) {
		        if (o_ptr->xtra2 & 0x04) fprintf(fff, "It provides immunity to electricity.\n");
		        else fprintf(fff, "It provides resistance to electricity.\n");
		}
		if (!(f2 & (TR2_IM_ACID))) {
		        if (o_ptr->xtra2 & 0x08) fprintf(fff, "It provides immunity to acid.\n");
			else fprintf(fff, "It provides resistance to acid.\n");
		}
		if (!(f5 & (TR5_IM_POISON))) {
			if (o_ptr->xtra2 & 0x10) fprintf(fff, "It provides immunity to poison.\n");
			else fprintf(fff, "It provides resistance to poison.\n");
		}
	} else {
		/* Note: The immunity checks are for items that get both,
		   resistance AND immunity to the same element. This can
		   happen if art/ego gives IM, while base item type
		   already has RES. */
		if ((f2 & (TR2_RES_FIRE)) && !(f2 & (TR2_IM_FIRE)))
			fprintf(fff, "It provides resistance to fire.\n");
		if ((f2 & (TR2_RES_COLD)) && !(f2 & (TR2_IM_COLD)))
			fprintf(fff, "It provides resistance to cold.\n");
		if ((f2 & (TR2_RES_ELEC)) && !(f2 & (TR2_IM_ELEC)))
			fprintf(fff, "It provides resistance to electricity.\n");
		if ((f2 & (TR2_RES_ACID)) && !(f2 & (TR2_IM_ACID)))
			fprintf(fff, "It provides resistance to acid.\n");
		if ((f2 & (TR2_RES_POIS)) && !(f2 & (TR5_IM_POISON)))
			fprintf(fff, "It provides resistance to poison.\n");
	}

	if (f2 & (TR2_RES_LITE))
		fprintf(fff, "It provides resistance to light.\n");
	if (f2 & (TR2_RES_DARK))
		fprintf(fff, "It provides resistance to dark.\n");

	if (f2 & (TR2_RES_BLIND))
		fprintf(fff, "It provides resistance to blindness.\n");
	if (f2 & (TR2_RES_CONF))
		fprintf(fff, "It provides resistance to confusion.\n");
	if (f2 & (TR2_RES_SOUND))
		fprintf(fff, "It provides resistance to sound.\n");
	if (f2 & (TR2_RES_SHARDS))
		fprintf(fff, "It provides resistance to shards.\n");

	if (f4 & (TR4_IM_NETHER))
		fprintf(fff, "It provides immunity to nether.\n");
	else if (f2 & (TR2_RES_NETHER))
		fprintf(fff, "It provides resistance to nether.\n");
	if (f2 & (TR2_RES_NEXUS))
		fprintf(fff, "It provides resistance to nexus.\n");
	if (f2 & (TR2_RES_CHAOS))
		fprintf(fff, "It provides resistance to chaos.\n");
	if (f2 & (TR2_RES_DISEN))
		fprintf(fff, "It provides resistance to disenchantment.\n");
	if (f5 & (TR5_IM_WATER))
		fprintf(fff, "It provides complete protection from unleashed water.\n");
	else if (f5 & (TR5_RES_WATER))
		fprintf(fff, "It provides resistance to unleashed water.\n");
	if (f5 & (TR5_RES_TIME))
		fprintf(fff, "It provides resistance to time.\n");
	if (f5 & (TR5_RES_MANA))
		fprintf(fff, "It provides resistance to magical energy.\n");
	if (f5 & TR5_RES_TELE) fprintf(fff, "It provides resistance to teleportation attacks.\n");

	if (f2 & (TR2_FREE_ACT))
		fprintf(fff, "It provides immunity to paralysis.\n");
	if (f2 & (TR2_HOLD_LIFE))
		fprintf(fff, "It provides resistance to life draining attacks.\n");
	if (f2 & (TR2_RES_FEAR))
		fprintf(fff, "It makes you completely fearless.\n");
	if (f3 & (TR3_SEE_INVIS))
		fprintf(fff, "It allows you to see invisible monsters.\n");

	if (f3 & (TR3_FEATHER))
		fprintf(fff, "It allows you to levitate.\n");
	if (f4 & (TR4_FLY))
		fprintf(fff, "It allows you to fly.\n");
	if (f5 & (TR5_PASS_WATER))
		fprintf(fff, "It allows you to swim easily.\n");
	if (f4 & (TR4_CLIMB))
		fprintf(fff, "It allows you to climb high mountains.\n");
	if (f3 & (TR3_WRAITH))
		fprintf(fff, "It renders you incorporeal.\n");
	if ((o_ptr->tval != TV_LITE) && ((f3 & (TR3_LITE1)) || (f4 & (TR4_LITE2)) || (f4 & (TR4_LITE3))))
		fprintf(fff, "It provides light.\n");
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
	if (f3 & (TR3_SLOW_DIGEST))
		fprintf(fff, "It slows your metabolism.\n");
	if (f3 & (TR3_REGEN))
		fprintf(fff, "It speeds your regenerative powers.\n");
	if (f5 & (TR5_REGEN_MANA))
		fprintf(fff, "It speeds your mana recharging.\n");
	if (f5 & (TR5_REFLECT))
		fprintf(fff, "It reflects bolts and arrows.\n");
	if (f3 & (TR3_SH_FIRE))
		fprintf(fff, "It produces a fiery sheath.\n");
	if (f5 & (TR5_SH_COLD))
		fprintf(fff, "It produces an icy sheath.\n");
	if (f3 & (TR3_SH_ELEC))
		fprintf(fff, "It produces an electric sheath.\n");
	if (f3 & (TR3_NO_MAGIC))
		fprintf(fff, "It produces an anti-magic shell.\n");
	/* Mega Hack^3 -- describe the Anchor of Space-time */
	if (o_ptr->name1 == ART_ANCHOR)
		fprintf(fff, "It prevents the space-time continuum from being disrupted.\n");

	if (f3 & (TR3_BLESSED))
		fprintf(fff, "It has been blessed by the gods.\n");

	if (f4 & (TR4_AUTO_ID))
		fprintf(fff, "It identifies all items for you.\n");

	if (f3 & (TR3_XTRA_MIGHT))
		fprintf(fff, "It fires missiles with extra might.\n");
	if (f3 & (TR3_XTRA_SHOTS))
		fprintf(fff, "It fires missiles excessively fast.\n");

	if (f4 & (TR4_CAPACITY))
		fprintf(fff, "It can hold more mana.\n");
	if (f4 & (TR4_CHEAPNESS))
		fprintf(fff, "It can cast spells for a lesser mana cost.\n");
	if (f4 & (TR4_FAST_CAST))
		fprintf(fff, "It can cast spells faster.\n");
	if (f4 & (TR4_CHARGING))
		fprintf(fff, "It regenerates its mana faster.\n");

	if (f3 & (TR3_TELEPORT))
		fprintf(fff, "It induces random teleportation.\n");
	if (f3 & (TR3_NO_TELE))
		fprintf(fff, "\377DIt prevents teleportation.\n");
	if (f5 & (TR5_DRAIN_MANA))
		fprintf(fff, "\377DIt drains your magic.\n");

	if (f5 & (TR5_DRAIN_HP))
		fprintf(fff, "\377DIt drains your health.\n");

	if (f3 & (TR3_DRAIN_EXP))
		fprintf(fff, "\377DIt drains your life force.\n");
	if (f3 & (TR3_AGGRAVATE))
		fprintf(fff, "\377DIt aggravates nearby creatures.\n");


	if (f4 & (TR4_NEVER_BLOW))
		fprintf(fff, "\377DIt can't attack.\n");

	if (f4 & (TR4_BLACK_BREATH))
		fprintf(fff, "\377DIt fills you with the Black Breath.\n");
	if (f4 & (TR4_CLONE))
		fprintf(fff, "\377DIt can clone monsters.\n");

	if (cursed_p(o_ptr)) {
		if (f3 & (TR3_PERMA_CURSE))
			fprintf(fff, "\377DIt is permanently cursed.\n");
		else if (f3 & (TR3_HEAVY_CURSE))
			fprintf(fff, "\377DIt is heavily cursed.\n");
		else
			fprintf(fff, "\377DIt is cursed.\n");
	}

	if (f3 & (TR3_TY_CURSE))
		fprintf(fff, "\377DIt carries an ancient foul curse.\n");

	if (f4 & (TR4_DG_CURSE))
		fprintf(fff, "\377DIt carries an ancient morgothian curse.\n");
	if (f4 & (TR4_CURSE_NO_DROP))
		fprintf(fff, "\377DIt cannot be dropped while cursed.\n");
	if (f3 & (TR3_AUTO_CURSE))
		fprintf(fff, "\377DIt can re-curse itself.\n");

	/* Stormbringer hardcoded note to give a warning!- C. Blue */
	if (o_ptr->name2 == EGO_STORMBRINGER)
		fprintf(fff, "\377DIt's possessed by mad wrath!\n");


	if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DARK_SWORD))
		fprintf(fff, "\377WIt cannot be enchanted nor disenchanted by any means.\n");
	else if (f5 & (TR5_NO_ENCHANT))
		fprintf(fff, "\377WIt cannot be enchanted by any means.\n");

#if 1
//	strcpy(buf_tmp, "\377WIt cannot be harmed by ");
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
		if (buf_tmp_i) strcat(buf_tmp, buf_tmp_i == buf_tmp_n - 1 ? " or " : ", ");
		strcat(buf_tmp, "cold");
		buf_tmp_i++;
	}
	if (f3 & (TR3_IGNORE_ELEC)) {
		if (buf_tmp_i) strcat(buf_tmp, buf_tmp_i == buf_tmp_n - 1 ? " or " : ", ");
		strcat(buf_tmp, "lightning");
		buf_tmp_i++;
	}
	if (f3 & (TR3_IGNORE_ACID)) {
		if (buf_tmp_i) strcat(buf_tmp, buf_tmp_i == buf_tmp_n - 1 ? " or " : ", ");
		strcat(buf_tmp, "acid");
		buf_tmp_i++;
	}
	if (f5 & (TR5_IGNORE_WATER)) {
		if (buf_tmp_i) strcat(buf_tmp, buf_tmp_i == buf_tmp_n - 1 ? " or " : ", ");
		strcat(buf_tmp, "water");
		buf_tmp_i++;
	}
	if (f5 & (TR5_IGNORE_MANA)) {
		if (buf_tmp_i) strcat(buf_tmp, buf_tmp_i == buf_tmp_n - 1 ? " or " : ", ");
		strcat(buf_tmp, "mana");
		buf_tmp_i++;
	}
	if (f5 & (TR5_IGNORE_DISEN)) {
		if (buf_tmp_i) strcat(buf_tmp, buf_tmp_i == buf_tmp_n - 1 ? " or " : ", ");
		strcat(buf_tmp, "disenchantment");
		buf_tmp_i++;
	}
	if (buf_tmp_n) fprintf(fff, "%s.\n", buf_tmp);
#else
	if ((f5 & (TR5_IGNORE_WATER)) && (f5 & (TR5_IGNORE_MANA)) && (f3 & (TR3_IGNORE_ACID)) && (f3 & (TR3_IGNORE_FIRE)) && (f3 & (TR3_IGNORE_COLD)) && (f3 & (TR3_IGNORE_ELEC))) {
		fprintf(fff, "\377WIt cannot be harmed by water, lightning, cold, acid, fire or pure energy.\n");
	} else {
		if (f5 & (TR5_IGNORE_WATER))
			fprintf(fff, "\377WIt cannot be harmed by water.\n");
		if (f3 & (TR3_IGNORE_ELEC))
			fprintf(fff, "\377WIt cannot be harmed by electricity.\n");
		if (f3 & (TR3_IGNORE_COLD))
			fprintf(fff, "\377WIt cannot be harmed by cold.\n");
		if (f3 & (TR3_IGNORE_ACID))
			fprintf(fff, "\377WIt cannot be harmed by acid.\n");
		if (f3 & (TR3_IGNORE_FIRE))
			fprintf(fff, "\377WIt cannot be harmed by fire.\n");
		if (f5 & (TR5_IGNORE_MANA))
			fprintf(fff, "\377WIt cannot be harmed by pure energy.\n");
	}
#endif

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
		case GF_HELL_FIRE: fprintf(fff, "It explodes with hell fire.\n"); break;
	}

	/* special artifacts hardcoded - C. Blue */
	if (o_ptr->tval == TV_POTION2 && o_ptr->sval == SV_POTION2_AMBER)
		fprintf(fff, "It turns your skin into amber, increasing your powers.\n");
	if (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_SLEEPING)
		fprintf(fff, "It drops a veil of sleep over all your surroundings.\n");

	/* Damage display for weapons */
	if (wield_slot(Ind, o_ptr) == INVEN_WIELD)
		display_weapon_damage(Ind, o_ptr, fff);

	/* Breakage/Damage display for ammo */
	if (wield_slot(Ind, o_ptr) == INVEN_AMMO) {
		fprintf(fff, "\377WIt has %d%% chances to break upon hit.\n"
				, breakage_chance(o_ptr));
		display_ammo_damage(Ind, o_ptr, fff);
	}

	/* specialty: recognize custom spell books and display their contents! - C. Blue */
	if (o_ptr->tval == TV_BOOK &&
	    (o_ptr->sval == SV_CUSTOM_TOME_1 ||
	    o_ptr->sval == SV_CUSTOM_TOME_2 ||
	    o_ptr->sval == SV_CUSTOM_TOME_3)) {
		if (!o_ptr->xtra1) fprintf(fff, "\n- no spells -\n");
		else fprintf(fff,"\nSpells:\n");
		if (o_ptr->xtra1) fprintf(fff, "- %s\n", string_exec_lua(Ind, format("return(__tmp_spells[%d].name)", o_ptr->xtra1 - 1)));
		if (o_ptr->xtra2) fprintf(fff, "- %s\n", string_exec_lua(Ind, format("return(__tmp_spells[%d].name)", o_ptr->xtra2 - 1)));
		if (o_ptr->xtra3) fprintf(fff, "- %s\n", string_exec_lua(Ind, format("return(__tmp_spells[%d].name)", o_ptr->xtra3 - 1)));
		if (o_ptr->xtra4) fprintf(fff, "- %s\n", string_exec_lua(Ind, format("return(__tmp_spells[%d].name)", o_ptr->xtra4 - 1)));
		if (o_ptr->xtra5) fprintf(fff, "- %s\n", string_exec_lua(Ind, format("return(__tmp_spells[%d].name)", o_ptr->xtra5 - 1)));
		if (o_ptr->xtra6) fprintf(fff, "- %s\n", string_exec_lua(Ind, format("return(__tmp_spells[%d].name)", o_ptr->xtra6 - 1)));
		if (o_ptr->xtra7) fprintf(fff, "- %s\n", string_exec_lua(Ind, format("return(__tmp_spells[%d].name)", o_ptr->xtra7 - 1)));
		if (o_ptr->xtra8) fprintf(fff, "- %s\n", string_exec_lua(Ind, format("return(__tmp_spells[%d].name)", o_ptr->xtra8 - 1)));
		if (o_ptr->xtra9) fprintf(fff, "- %s\n", string_exec_lua(Ind, format("return(__tmp_spells[%d].name)", o_ptr->xtra9 - 1)));
	}

	//	info[i]=NULL;
	/* Close the file */
	my_fclose(fff);

	/* Hack -- anything written? (rewrite me) */

	/* Open a new file */
	fff = my_fopen(p_ptr->infofile, "rb");

	if (my_fgets(fff, buf, 1024, FALSE))
	{
		/* Close the file */
		my_fclose(fff);

		return (FALSE);
	}

	/* Close the file */
	my_fclose(fff);

	/* No special effects */
//	if (!i) return (FALSE);

	/* Let the client know it's about to get some info */
	Send_special_other(Ind);

	/* Gave knowledge */
	return (TRUE);
}



/*
 * Convert an inventory index into a one character label
 * Note that the label does NOT distinguish inven/equip.
 */
s16b index_to_label(int i)
{
	/* Indexes for "inven" are easy */
	if (i < INVEN_WIELD) return (I2A(i));

	/* Indexes for "equip" are offset */
	return (I2A(i - INVEN_WIELD));
}


/*
 * Convert a label into the index of an item in the "inven"
 * Return "-1" if the label does not indicate a real item
 */
s16b label_to_inven(int Ind, int c)
{
	player_type *p_ptr = Players[Ind];

	int i;

	/* Convert */
	i = (islower(c) ? A2I(c) : -1);

	/* Verify the index */
	if ((i < 0) || (i > INVEN_PACK)) return (-1);

	/* Empty slots can never be chosen */
	if (!p_ptr->inventory[i].k_idx) return (-1);

	/* Return the index */
	return (i);
}


/*
 * Convert a label into the index of a item in the "equip"
 * Return "-1" if the label does not indicate a real item
 */
s16b label_to_equip(int Ind, int c)
{
	player_type *p_ptr = Players[Ind];

	int i;

	/* Convert */
	i = (islower(c) ? A2I(c) : -1) + INVEN_WIELD;

	/* Verify the index */
	if ((i < INVEN_WIELD) || (i >= INVEN_TOTAL)) return (-1);

	/* Empty slots can never be chosen */
	if (!p_ptr->inventory[i].k_idx) return (-1);

	/* Return the index */
	return (i);
}



/*
 * Determine which equipment slot (if any) an item likes
 */
s16b wield_slot(int Ind, object_type *o_ptr)
{
	player_type *p_ptr = Players[Ind];

	/* Slot for equipment */
	switch (o_ptr->tval)
	{
		case TV_DIGGING:
		case TV_TOOL:
			return (INVEN_TOOL);
#ifndef ENABLE_RCRAFT
		case TV_RUNE1:
		{
			if (o_ptr->sval == SV_RUNE1_SELF && o_ptr->name2) 
				return (INVEN_TOOL);
			return (-1);
		}
#endif
		case TV_BLUNT:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_AXE:
		case TV_MSTAFF:
		{
			return (INVEN_WIELD);
		}

		case TV_BOW:
		case TV_BOOMERANG:
		case TV_INSTRUMENT:
		{
			return (INVEN_BOW);
		}

		case TV_RING:
		{
			/* Use the right hand first */
			if (!p_ptr->inventory[INVEN_RIGHT].k_idx) return (INVEN_RIGHT);

			/* Use the left hand for swapping (by default) */
			return (INVEN_LEFT);
		}

		case TV_AMULET:
		{
			return (INVEN_NECK);
		}

		case TV_LITE:
		{
			return (INVEN_LITE);
		}

		case TV_DRAG_ARMOR:
		case TV_HARD_ARMOR:
		case TV_SOFT_ARMOR:
		{
			return (INVEN_BODY);
		}

		case TV_CLOAK:
		{
			return (INVEN_OUTER);
		}

		case TV_SHIELD:
		{
			return (INVEN_ARM);
		}

		case TV_CROWN:
		case TV_HELM:
		{
			return (INVEN_HEAD);
		}

		case TV_GLOVES:
		{
			return (INVEN_HANDS);
		}

		case TV_BOOTS:
		{
			return (INVEN_FEET);
		}

		case TV_SHOT:
		{
/*			if(p_ptr->inventory[INVEN_BOW].k_idx)
			{
				if(p_ptr->inventory[INVEN_BOW].sval < 10)
//					return get_slot(INVEN_AMMO);
*/					return (INVEN_AMMO);
/*			}
			return -1;
*/		}

		case TV_ARROW:
		{
/*			if(p_ptr->inventory[INVEN_BOW].k_idx)
			{
				if((p_ptr->inventory[INVEN_BOW].sval >= 10)&&(p_ptr->inventory[INVEN_BOW].sval < 20))
//					return get_slot(INVEN_AMMO);
*/					return (INVEN_AMMO);
/*			}
			return -1;
*/		}
		case TV_BOLT:
		{                        
/*			if(p_ptr->inventory[INVEN_BOW].k_idx)
			{
				if(p_ptr->inventory[INVEN_BOW].sval >= 20)
//					return get_slot(INVEN_AMMO);
*/				return (INVEN_AMMO);
/*			}
			return -1;
*/		}
	}

	/* No slot available */
	return (-1);
}


/*
 * Return a string mentioning how a given item is carried
 */
cptr mention_use(int Ind, int i)
{
	player_type *p_ptr = Players[Ind];

	cptr p;

	/* Examine the location */
	switch (i)
	{
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
		default:          p = "In pack"; break;
	}

	/* Hack -- Heavy weapon */
	if (i == INVEN_WIELD || (i == INVEN_ARM && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)) /* dual-wield, not that needed here though */
	{
		object_type *o_ptr;
		o_ptr = &p_ptr->inventory[i];
		if (adj_str_hold[p_ptr->stat_ind[A_STR]] < o_ptr->weight / 10)
		{
			p = "Just lifting";
		}
	}

	/* Hack -- Heavy bow */
	if (i == INVEN_BOW)
	{
		object_type *o_ptr;
		o_ptr = &p_ptr->inventory[i];
		if (adj_str_hold[p_ptr->stat_ind[A_STR]] < o_ptr->weight / 10)
		{
			p = "Just holding";
		}
	}

	/* Return the result */
	return (p);
}


/*
 * Return a string describing how a given item is being worn.
 * Currently, only used for items in the equipment, not inventory.
 */
cptr describe_use(int Ind, int i)
{
	player_type *p_ptr = Players[Ind];

	cptr p;

	switch (i)
	{
		/* you attack players too, none? */
//		case INVEN_WIELD: p = "attacking monsters with"; break;
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
		default:          p = "carrying in your pack"; break;
	}

	/* Hack -- Heavy weapon */
	if (i == INVEN_WIELD || (i == INVEN_ARM && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)) /* dual-wield, not that needed here though */
	{
		object_type *o_ptr;
		o_ptr = &p_ptr->inventory[i];
		if (adj_str_hold[p_ptr->stat_ind[A_STR]] < o_ptr->weight / 10)
		{
			p = "just lifting";
		}
	}

	/* Hack -- Heavy bow */
	if (i == INVEN_BOW)
	{
		object_type *o_ptr;
		o_ptr = &p_ptr->inventory[i];
		if (adj_str_hold[p_ptr->stat_ind[A_STR]] < o_ptr->weight / 10)
		{
			p = "just holding";
		}
	}

	/* Return the result */
	return p;
}





/*
 * Check an item against the item tester info
 */
bool item_tester_okay(object_type *o_ptr)
{
	/* Hack -- allow listing empty slots */
	if (item_tester_full) return (TRUE);

	/* Require an item */
	if (!o_ptr->k_idx) return (FALSE);

	/* Hack -- ignore "gold" */
	if (o_ptr->tval == TV_GOLD) return (FALSE);

	/* Check the tval */
	if (item_tester_tval)
	{
		if (!(item_tester_tval == o_ptr->tval)) return (FALSE);
	}

	/* Check the hook */
	if (item_tester_hook)
	{
		if (!(*item_tester_hook)(o_ptr)) return (FALSE);
	}

	/* Assume okay */
	return (TRUE);
}




/*
 * Choice window "shadow" of the "show_inven()" function
 *
 * Note that this function simply sends a text string describing the
 * inventory slot, along with the tval, weight, and position in the inventory
 * to the client --KLJ--
 */
void display_inven(int Ind)
{
	player_type	*p_ptr = Players[Ind];
	register	int i, n, z = 0;
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
		n = strlen(o_name);

		/* Get a color */
		if (p_ptr->admin_dm) {
			/* hack- show correct mode-fail colour to admins */
			if (can_use_admin(Ind, o_ptr)) attr = get_attr_from_tval(o_ptr);
			else attr = TERM_L_DARK;
		} else {
			if (can_use(Ind, o_ptr)) attr = get_attr_from_tval(o_ptr);
			else attr = TERM_L_DARK;
		}

		/* Get a color for a book */
		if (o_ptr->tval == TV_BOOK) attr = get_book_name_color(Ind, o_ptr);

		/* Hack -- fake monochrome */
		if (!use_color) attr = TERM_WHITE;

		/* Display the weight if needed */
		wgt = o_ptr->weight; //* o_ptr->number;

		/* Send the info to the client */
		if (is_newer_than(&p_ptr->version, 4, 4, 1, 7, 0, 0)) {
			if (o_ptr->tval != TV_BOOK || o_ptr->sval < SV_CUSTOM_TOME_1 || o_ptr->sval == SV_SPELLBOOK) {
				Send_inven(Ind, tmp_val[0], attr, wgt, o_ptr, o_name);
			} else {
				Send_inven_wide(Ind, tmp_val[0], attr, wgt, o_ptr, o_name);
			}
		} else {
			Send_inven(Ind, tmp_val[0], attr, wgt, o_ptr, o_name);
		}
	}

	/* Send the new inventory revision if the inventory has changed - mikaelh */
	if (p_ptr->inventory_changed)
	{
		Send_inventory_revision(Ind);
		p_ptr->inventory_changed = FALSE;
	}
}



/*
 * Choice window "shadow" of the "show_equip()" function
 */
void display_equip(int Ind)
{
	player_type	*p_ptr = Players[Ind];
	register	int i, n;
	object_type	*o_ptr;
	byte		attr = TERM_WHITE;
	char		tmp_val[80];
	char		o_name[ONAME_LEN];
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

		/* Update the copy */
		memcpy(&p_ptr->inventory_copy[i], o_ptr, sizeof(object_type));

		/* Start with an empty "index" */
		tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';

		/* Prepare an "index" */
		tmp_val[0] = index_to_label(i);

		/* Obtain an item description */
		object_desc(Ind, o_name, o_ptr, TRUE, 3);

		/* Obtain the length of the description */
		n = strlen(o_name);

		/* Get the color */
		attr = get_attr_from_tval(o_ptr);

		/* Hack -- fake monochrome */
		if (!use_color) attr = TERM_WHITE;

		/* Display the weight (if needed) */
		wgt = o_ptr->weight;// * o_ptr->number;
//		wgt = o_ptr->weight; <- shows wrongly for ammunition!

		/* Send the info off */
		//Send_equip(Ind, tmp_val[0], attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->pval, o_name);
		Send_equip(Ind, tmp_val[0], attr, wgt, o_ptr, o_name);
	}
}

/* Attempts to convert owner of item to player, if allowed.
   Returns TRUE if the player can actually use the item
   ie if ownership taking was successful, else FALSE.
   NOTE: DID NOT OWN UNOWNED ITEMS and hence fails for those (in compat_pomode())!
         That was changed now. */
bool can_use(int Ind, object_type *o_ptr)
{
	player_type *p_ptr = Players[Ind];

	/* exception for Highlander Tournament amulets */
	if ((o_ptr->tval == TV_AMULET) && (o_ptr->sval == SV_AMULET_HIGHLANDS)) {
		o_ptr->owner = p_ptr->id;
		o_ptr->mode = p_ptr->mode;
		return TRUE;
	}

	/* Owner always can use */
	if (p_ptr->id == o_ptr->owner || p_ptr->admin_dm) return (TRUE);

	if (o_ptr->level < 1 && o_ptr->owner) return (FALSE);

	/* Own unowned items (for stores).
	   Note that CTRL+R -> display_inven() -> if (can_use()) checks will
	   automatically own unowned items this way, that's why the admin_dm
	   is exempt, for when he /wished for items. - C. Blue */
	if (!o_ptr->owner && !p_ptr->admin_dm) {
		o_ptr->owner = p_ptr->id;
		o_ptr->mode = p_ptr->mode;
	}

	if (compat_pomode(Ind, o_ptr)) return FALSE;

	/* Hack -- convert if available */
	if (p_ptr->lev >= o_ptr->level && !p_ptr->admin_dm) {
		o_ptr->owner = p_ptr->id;
		return (TRUE);
	}
	else return (FALSE);
}

/* Same as can_use(), but avoids auto-owning items (to be used for admin characters) */
bool can_use_admin(int Ind, object_type *o_ptr)
{
	player_type *p_ptr = Players[Ind];

	/* exception for Highlander Tournament amulets */
	if ((o_ptr->tval == TV_AMULET) && (o_ptr->sval == SV_AMULET_HIGHLANDS))
		return TRUE;

	/* Owner always can use */
	if (p_ptr->id == o_ptr->owner) return (TRUE);

	if (o_ptr->level < 1 && o_ptr->owner) return (FALSE);

	if (compat_pomode(Ind, o_ptr)) return FALSE;

	/* Hack -- convert if available */
	if (p_ptr->lev >= o_ptr->level) return (TRUE);
	else return (FALSE);
}

bool can_use_verbose(int Ind, object_type *o_ptr)
{
	player_type *p_ptr = Players[Ind];

	/* exception for Highlander Tournament amulets */
	if ((o_ptr->tval == TV_AMULET) && (o_ptr->sval == SV_AMULET_HIGHLANDS)) {
		o_ptr->owner = p_ptr->id;
		o_ptr->mode = p_ptr->mode;
		return TRUE;
	}

#ifndef RPG_SERVER /* hm not sure about this.. */
	/* Owner always can use */
	if (p_ptr->id == o_ptr->owner || p_ptr->admin_dm) return (TRUE);

	if (o_ptr->level < 1 && o_ptr->owner) {
		msg_print(Ind, "You must be the owner in order to use it.");
		return (FALSE);
	}

	if (compat_pomode(Ind, o_ptr)) {
		msg_format(Ind, "You cannot use things that belong to %s players.", compat_pomode(Ind, o_ptr));
		return FALSE;
	}

	/* Hack -- convert if available */
	if (p_ptr->lev >= o_ptr->level && !p_ptr->admin_dm) {
		o_ptr->owner = p_ptr->id;
		return (TRUE);
	} else {
		msg_print(Ind, "Your level is not high enough yet to use this.");
		return (FALSE);
	}
#else
	/* Let's still have this restriction - mikaelh */
	if (o_ptr->level < 1 && o_ptr->owner && p_ptr->id != o_ptr->owner && !p_ptr->admin_dm) {
		msg_print(Ind, "You must be the owner in order to use it.");
		return(FALSE);
	}

	/* the_sandman: let's turn this off? Party trading is horrible with this one. Plus we
	 * already only allow 1 char each account. */
	return (TRUE);
#endif
}

byte get_book_name_color(int Ind, object_type *o_ptr)
{
	//player_type *p_ptr = Players[Ind];
	if (o_ptr->sval == SV_SPELLBOOK) {
		return (byte)exec_lua(Ind, format("return get_spellbook_name_colour(%d)", o_ptr->pval));
	} else if (o_ptr->sval >= SV_CUSTOM_TOME_1) {
		/* first annotated spell decides */
		if (o_ptr->xtra1)
			return (byte)exec_lua(Ind, format("return get_spellbook_name_colour(%d)", o_ptr->xtra1 - 1));
		else
			return TERM_WHITE; /* unused custom book */
	} else {
		/* priests */
		if ((o_ptr->sval >= 12 && o_ptr->sval <= 15) ||
		    o_ptr->sval == 53)
			return TERM_GREEN;
		/* druids */
		else if (o_ptr->sval >= 16 && o_ptr->sval <= 17) return TERM_L_GREEN;
		/* astral tome */
		else if (o_ptr->sval == 18) return TERM_ORANGE;
		/* mindcrafters */
		else if (o_ptr->sval >= 19 && o_ptr->sval <= 21) return TERM_YELLOW;
		/* mages (default) */
		else return TERM_L_BLUE;
	}
}

byte get_attr_from_tval(object_type *o_ptr) {
	int attr = tval_to_attr[o_ptr->tval];

#ifdef PLAYER_STORES
	if (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_CHEQUE)
		attr = TERM_L_UMBER;
#endif

	if (o_ptr->tval == TV_SOFT_ARMOR && o_ptr->sval == SV_SHIRT) {
		if (!o_ptr->xtra1) o_ptr->xtra1 = attr;
		attr = o_ptr->xtra1;
	}

	return(attr);
}
