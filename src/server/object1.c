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


/* Ones borrowed from PernAngband.	- Jir - */
/*
 * Max sizes of the following arrays
 */
#define MAX_ROCKS      62       /* Used with rings (min 58) */
#define MAX_AMULETS    38       /* Used with amulets (min 30) */
#define MAX_WOODS      36       /* Used with staffs (min 32) */
#define MAX_METALS     39       /* Used with wands/rods (min 32/30) */
#define MAX_COLORS     68       /* Used with potions (min 62) */
#define MAX_SHROOM     20       /* Used with mushrooms (min 20) */
#define MAX_TITLES     65       /* Used with scrolls (min 55) */
#define MAX_SYLLABLES 164       /* Used with scrolls (see below) */

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

static byte ring_col[MAX_ROCKS] =
{
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


/*
 * Amulets (adjectives and colors)
 */

static cptr amulet_adj[MAX_AMULETS] =
{
	"Amber", "Driftwood", "Coral", "Agate", "Ivory",
	"Obsidian", "Bone", "Brass", "Bronze", "Pewter",
	"Tortoise Shell", "Golden", "Azure", "Crystal", "Silver",
        "Copper", "Amethyst", "Mithril", "Sapphire", "Dragon Tooth",
        "Carved Oak", "Sea Shell", "Flint Stone", "Ruby", "Scarab",
        "Origami Paper", "Meteoric Iron", "Platinum", "Glass", "Beryl",
        "Malachite", "Adamantite", "Mother-of-pearl", "Runed",
	"Sandalwood", "Emerald", "Aquamarine", "Sparkling",
};

static byte amulet_col[MAX_AMULETS] =
{
	TERM_YELLOW, TERM_L_UMBER, TERM_WHITE, TERM_L_WHITE, TERM_WHITE,
	TERM_L_DARK, TERM_WHITE, TERM_ORANGE, TERM_L_UMBER, TERM_SLATE,
	TERM_GREEN, TERM_YELLOW, TERM_L_BLUE, TERM_L_BLUE, TERM_L_WHITE,
	TERM_L_UMBER, TERM_VIOLET, TERM_L_BLUE, TERM_BLUE, TERM_L_WHITE,
	TERM_UMBER, TERM_L_BLUE, TERM_SLATE, TERM_RED, TERM_L_GREEN, 
	TERM_WHITE, TERM_L_DARK, TERM_L_WHITE, TERM_WHITE, TERM_L_GREEN, 
	TERM_GREEN, TERM_VIOLET, TERM_L_WHITE, TERM_UMBER,
	TERM_L_WHITE, TERM_GREEN, TERM_L_BLUE, TERM_ELEC,
};


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
	"cryptomeria"
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
        "Clear", "Light Brown", "Icky Green", "Strangely Phosphorescent",
	"Azure", "Blue", "Black", "Brown",
	"Chartreuse", "Crimson", "Cyan",
	"Dark Blue", "Dark Green", "Dark Red", "Green",
	"Grey", "Hazy", "Indigo",
	"Light Blue", "Light Green", "Magenta",
	"Orange",
	"Pink", "Puce", "Purple",
	"Red", "Tangerine",
	"Violet", "Vermilion", "White", "Yellow",
	"Gloopy Green",
	"Gold",
	"Ichor", "Ivory White", "Sky Blue",
	"Beige",
};

static byte potion_col[MAX_COLORS] =
{
        TERM_WHITE, TERM_L_UMBER, TERM_GREEN, TERM_MULTI,
	TERM_L_BLUE, TERM_BLUE, TERM_L_DARK, TERM_UMBER,
	TERM_L_GREEN, TERM_RED, TERM_L_BLUE,
	TERM_BLUE, TERM_GREEN, TERM_RED, TERM_GREEN,
	TERM_SLATE,TERM_L_WHITE, TERM_VIOLET,
	TERM_L_BLUE, TERM_L_GREEN, TERM_RED,
	TERM_ORANGE,
	TERM_L_RED, TERM_VIOLET, TERM_VIOLET,
	TERM_RED, TERM_ORANGE,
	TERM_VIOLET, TERM_RED, TERM_WHITE, TERM_YELLOW,
	TERM_GREEN,
	TERM_YELLOW,
	TERM_RED, TERM_WHITE, TERM_L_BLUE,
	TERM_L_UMBER,
};

static char potion_adj[MAX_COLORS][24];

#else
static cptr potion_adj[MAX_COLORS] =
{
        "Clear", "Light Brown", "Icky Green", "Strangely Phosphorescent",
	"Azure", "Blue", "Blue Speckled", "Black", "Brown", "Brown Speckled",
	"Bubbling", "Chartreuse", "Cloudy", "Copper Speckled", "Crimson", "Cyan",
	"Dark Blue", "Dark Green", "Dark Red", "Gold Speckled", "Green",
	"Green Speckled", "Grey", "Grey Speckled", "Hazy", "Indigo",
	"Light Blue", "Light Green", "Magenta", "Metallic Blue", "Metallic Red",
	"Metallic Green", "Metallic Purple", "Misty", "Orange", "Orange Speckled",
	"Pink", "Pink Speckled", "Puce", "Purple", "Purple Speckled",
	"Red", "Red Speckled", "Silver Speckled", "Smoky", "Tangerine",
	"Violet", "Vermilion", "White", "Yellow", "Violet Speckled",
	"Pungent", "Clotted Red", "Viscous Pink", "Oily Yellow", "Gloopy Green",
	"Shimmering", "Coagulated Crimson", "Yellow Speckled", "Gold",
	"Manly", "Stinking", "Oily Black", "Ichor", "Ivory White", "Sky Blue",
	"Beige", "Whirling",
};

static byte potion_col[MAX_COLORS] =
{
        TERM_WHITE, TERM_L_UMBER, TERM_GREEN, TERM_MULTI,
	TERM_L_BLUE, TERM_BLUE, TERM_BLUE, TERM_L_DARK, TERM_UMBER, TERM_UMBER,
	TERM_L_WHITE, TERM_L_GREEN, TERM_WHITE, TERM_L_UMBER, TERM_RED, TERM_L_BLUE,
	TERM_BLUE, TERM_GREEN, TERM_RED, TERM_YELLOW, TERM_GREEN,
	TERM_GREEN, TERM_SLATE, TERM_SLATE, TERM_L_WHITE, TERM_VIOLET,
	TERM_L_BLUE, TERM_L_GREEN, TERM_RED, TERM_BLUE, TERM_RED,
	TERM_GREEN, TERM_VIOLET, TERM_L_WHITE, TERM_ORANGE, TERM_ORANGE,
	TERM_L_RED, TERM_L_RED, TERM_VIOLET, TERM_VIOLET, TERM_VIOLET,
	TERM_RED, TERM_RED, TERM_L_WHITE, TERM_L_DARK, TERM_ORANGE,
	TERM_VIOLET, TERM_RED, TERM_WHITE, TERM_YELLOW, TERM_VIOLET,
	TERM_L_RED, TERM_RED, TERM_L_RED, TERM_YELLOW, TERM_GREEN,
	TERM_MULTI, TERM_RED, TERM_YELLOW, TERM_YELLOW,
	TERM_L_UMBER, TERM_UMBER, TERM_L_DARK, TERM_RED, TERM_WHITE, TERM_L_BLUE,
	TERM_L_UMBER, TERM_L_WHITE,
};
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
		{
			return (TRUE);
		}

		/* All Food, Potions, Scrolls, Rods */
		case TV_FOOD:
		case TV_POTION:
		case TV_SCROLL:
		case TV_PARCHEMENT:
		case TV_ROD:
				case TV_POTION2:
                case TV_ROD_MAIN:
                case TV_BATERIE:
		{
			return (TRUE);
		}

		/* Some Rings, Amulets, Lites */
		case TV_RING:
		case TV_AMULET:
		case TV_LITE:
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

		case TV_HAFTED:
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
		case TV_PARCHEMENT:
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

	/* Potions */
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
			while (1)
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
			case TV_FOOD:   return (food_col[indexx]);
			case TV_POTION: return (potion_col[indexx]);
			case TV_SCROLL: return (scroll_col[indexx]);
			case TV_AMULET: return (amulet_col[indexx]);
			case TV_RING:   return (ring_col[indexx]);
			case TV_STAFF:  return (staff_col[indexx]);
			case TV_WAND:   return (wand_col[indexx]);
			case TV_ROD:    return (rod_col[indexx]);

			/* hack -- borrow those of potions */
			case TV_POTION2: return (potion_col[indexx + 4]);
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
	if (o_ptr->name1)
	{
		artifact_type *a_ptr;
	
		/* Hack -- Randarts! */
		if (o_ptr->name1 == ART_RANDART)
		{
			if (!(a_ptr = randart_make(o_ptr))) return;
			(*f1) = a_ptr->flags1;
			(*f2) = a_ptr->flags2;
			(*f3) = a_ptr->flags3;
			(*f4) = a_ptr->flags4;
			(*f5) = a_ptr->flags5;
			(*esp) = a_ptr->esp;
		}
		else
		{
			if (!(a_ptr = &a_info[o_ptr->name1])) return;
				(*f1) |= a_ptr->flags1;
				(*f2) |= a_ptr->flags2;
				(*f3) |= a_ptr->flags3;
				(*f4) |= a_ptr->flags4;
				(*f5) |= a_ptr->flags5;
				(*esp) |= a_ptr->esp;
		}
//		if (a_ptr == NULL) return;

#if 0
		(*f1) = a_ptr->flags1;
		(*f2) = a_ptr->flags2;
		(*f3) = a_ptr->flags3;
                (*f4) = a_ptr->flags4;
                (*f5) = a_ptr->flags5;
                (*esp) = a_ptr->esp;
                if ((!object_flags_no_set) && (a_ptr->set != -1))
                        apply_flags_set(o_ptr->name1, a_ptr->set, f1, f2, f3, f4, f5, esp);
#endif	// 0
	}

	/* Ego-item */
	if (o_ptr->name2)
	{
//		ego_item_type *e_ptr = &e_info[o_ptr->name2];
	 	artifact_type *a_ptr;
	 	
			a_ptr =	ego_make(o_ptr);

		(*f1) |= a_ptr->flags1;
		(*f2) |= a_ptr->flags2;
		(*f3) |= a_ptr->flags3;
                (*f4) |= a_ptr->flags4;
                (*f5) |= a_ptr->flags5;
                (*esp) |= a_ptr->esp;
	}
#if 0
	/* Extra powers */
	switch (o_ptr->xtra1)
	{
		case EGO_XTRA_SUSTAIN:
		{
			/* Choose a sustain */
			switch (o_ptr->xtra2 % 6)
			{
				case 0: (*f2) |= TR2_SUST_STR; break;
				case 1: (*f2) |= TR2_SUST_INT; break;
				case 2: (*f2) |= TR2_SUST_WIS; break;
				case 3: (*f2) |= TR2_SUST_DEX; break;
				case 4: (*f2) |= TR2_SUST_CON; break;
				case 5: (*f2) |= TR2_SUST_CHR; break;
			}

			break;
		}

		case EGO_XTRA_POWER:
		{
			/* Choose a power */
			switch (o_ptr->xtra2 % 9)
			{
				case 0: (*f2) |= TR2_RES_BLIND; break;
				case 1: (*f2) |= TR2_RES_CONF; break;
				case 2: (*f2) |= TR2_RES_SOUND; break;
				case 3: (*f2) |= TR2_RES_SHARDS; break;
				case 4: (*f2) |= TR2_RES_NETHER; break;
				case 5: (*f2) |= TR2_RES_NEXUS; break;
				case 6: (*f2) |= TR2_RES_CHAOS; break;
				case 7: (*f2) |= TR2_RES_DISEN; break;
				case 8: (*f2) |= TR2_RES_POIS; break;
			}

			break;
		}

		case EGO_XTRA_ABILITY:
		{
			/* Choose an ability */
			switch (o_ptr->xtra2 % 8)
			{
				case 0: (*f3) |= TR3_FEATHER; break;
				case 1: (*f3) |= TR3_LITE; break;
				case 2: (*f3) |= TR3_SEE_INVIS; break;
				case 3: (*f3) |= TR3_TELEPATHY; break;
				case 4: (*f3) |= TR3_SLOW_DIGEST; break;
				case 5: (*f3) |= TR3_REGEN; break;
				case 6: (*f2) |= TR2_FREE_ACT; break;
				case 7: (*f2) |= TR2_HOLD_LIFE; break;
			}

			break;
		}
	}
#endif	// 0
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
 */   
/*   
 *  +8 -- Cloak Death [1,+3](+2stl){nifty}
 *
 * If the strings created with mode 0-3 are too long, this function is called
 * again with 8 added to 'mode' and attempt to 'abbreviate' the strings. -Jir-
 */
void object_desc(int Ind, char *buf, object_type *o_ptr, int pref, int mode)
{
        player_type     *p_ptr = Players[Ind];
	cptr		basenm, modstr;
	int			power, indexx;

	bool		aware = FALSE;
	bool		known = FALSE;

	bool		append_name = FALSE;

	bool		show_weapon = FALSE;
	bool		show_armour = FALSE;

	cptr		s, u;
	char		*t;

	char		p1 = '(', p2 = ')';
	char		b1 = '[', b2 = ']';
	char		c1 = '{', c2 = '}';

	char		tmp_val[160];

	    u32b f1, f2, f3, f4, f5, esp;

	object_kind		*k_ptr = &k_info[o_ptr->k_idx];


	/* Extract some flags */
			  object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);


	/* Assume aware and known if not a valid player */
	if (Ind)
	{
		/* See if the object is "aware" */
		if (object_aware_p(Ind, o_ptr)) aware = TRUE;

		/* See if the object is "known" */
		if (object_known_p(Ind, o_ptr)) known = TRUE;
	}
	else
	{
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
	switch (o_ptr->tval)
	{
			/* Some objects are easy to describe */
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
		{
			break;
		}


			/* Missiles/ Bows/ Weapons */
		case TV_SHOT:
		case TV_BOLT:
		case TV_ARROW:
		case TV_BOW:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_DIGGING:
                case TV_BOOMERANG:
                case TV_AXE:
                case TV_MSTAFF:
		{
			show_weapon = TRUE;
			break;
		}

		/* Trapping Kits (not implemented yet) */
		case TV_TRAPKIT:
		{
			modstr = basenm;
			basenm = "& # Trap Set~";
			break;
		}

			/* Armour */
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_CLOAK:
		case TV_CROWN:
		case TV_HELM:
		case TV_SHIELD:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		{
			show_armour = TRUE;
			break;
		}


                case TV_GOLEM:
		{
			break;
		}

			/* Lites (including a few "Specials") */
		case TV_LITE:
		{
			break;
		}


			/* Amulets (including a few "Specials") */
		case TV_AMULET:
		{
			/* Known artifacts */
//			if (artifact_p(o_ptr) && aware) break;
			if (artifact_p(o_ptr) && known) break;

			/* Color the object */
			modstr = amulet_adj[indexx];
			if (aware) append_name = TRUE;
			basenm = aware ? "& Amulet~" : "& # Amulet~";
			break;
		}


			/* Rings (including a few "Specials") */
		case TV_RING:
		{
			/* Known artifacts */
//			if (artifact_p(o_ptr) && aware) break;
			//if (artifact_p(o_ptr) && known && o_ptr->sval!=SV_RING_SPECIAL) break;
			if(o_ptr->sval==SV_RING_SPECIAL){
				basenm="The Ring of Power";
			}
			if (artifact_p(o_ptr) && known) break;

			/* Color the object */
			modstr = ring_adj[indexx];
			if (aware) append_name = TRUE;
			basenm = aware ? "& Ring~" : "& # Ring~";

			/* Hack -- The One Ring */
			if (!aware && (o_ptr->sval == SV_RING_POWER)) modstr = "Plain Gold";

			break;
		}


		case TV_STAFF:
		{
			/* Color the object */
			modstr = staff_adj[indexx];
			if (aware) append_name = TRUE;
			basenm = aware ? "& Staff~" : "& # Staff~";
			break;
		}

		case TV_WAND:
		{
			/* Color the object */
			modstr = wand_adj[indexx];
			if (aware) append_name = TRUE;
			basenm = aware ? "& Wand~" : "& # Wand~";
			break;
		}

		case TV_ROD:
		{
			/* Color the object */
			modstr = rod_adj[indexx];
			if (aware) append_name = TRUE;
			basenm = aware ? "& Rod~" : "& # Rod~";
			break;
		}

                case TV_ROD_MAIN:
		{
                        object_kind *tip_ptr = &k_info[lookup_kind(TV_ROD, o_ptr->pval)];

                        modstr = k_name + tip_ptr->name;
			break;
		}


		case TV_SCROLL:
		{
			/* Color the object */
			modstr = scroll_adj[indexx];
			if (aware) append_name = TRUE;
			basenm = aware ? "& Scroll~" : "& Scroll~ titled \"#\"";
			break;
		}

		case TV_POTION:
		case TV_POTION2:
		{
			/* Color the object */
			modstr = potion_adj[indexx];
			if (aware) append_name = TRUE;
			basenm = aware ? "& Potion~" : "& # Potion~";
			break;
		}

		case TV_FOOD:
		{
			/* Ordinary food is "boring" */
			if ((o_ptr->sval >= SV_FOOD_MIN_FOOD) && (o_ptr->sval <= SV_FOOD_MAX_FOOD)) break;

			/* Color the object */
			modstr = food_adj[indexx];
			if (aware) append_name = TRUE;
			basenm = aware ? "& Mushroom~" : "& # Mushroom~";
			break;
		}

		case TV_PARCHEMENT:
		{
			modstr = basenm;
			basenm = "& Parchment~ - #";
			break;
		}

			/* Hack -- Gold/Gems */
		case TV_GOLD:
		{
			strcpy(buf, basenm);
			return;
		}

		case TV_BOOK:
		{                        
//			basenm = k_name + k_ptr->name;
			if (o_ptr->sval == 255) modstr = school_spells[o_ptr->pval].name;
			break;
		}

			/* Used in the "inventory" routine */
		default:
		{
			strcpy(buf, "(nothing)");
			return;
		}
	}


	/* Start dumping the result */
	t = buf;

	/* The object "expects" a "number" */
	if (basenm[0] == '&')
	{
		cptr ego = NULL;

		/* Grab any ego-item name */
		//                if ((o_ptr->name2 || o_ptr->name2b) && (o_ptr->tval != TV_ROD_MAIN))
		if (known && (o_ptr->name2 || o_ptr->name2b) && (o_ptr->tval != TV_ROD_MAIN))
		{
			ego_item_type *e_ptr = &e_info[o_ptr->name2];
			ego_item_type *e2_ptr = &e_info[o_ptr->name2b];

			if (e_ptr->before)
			{
				ego = e_ptr->name + e_name;
			}
#if 1
			else if (e2_ptr->before)
			{
				ego = e2_ptr->name + e_name;
			}
#endif	// 0

		}

		/* Skip the ampersand (and space) */
		s = basenm + 2;

		/* No prefix */
		if (!pref)
		{
			/* Nothing */
		}

		/* Hack -- None left */
		else if (o_ptr->number <= 0)
		{
			t = object_desc_str(t, "no more ");
		}

		/* Extract the number */
		else if (o_ptr->number > 1)
		{
			t = object_desc_num(t, o_ptr->number);
			t = object_desc_chr(t, ' ');
		}

		/* Hack -- Omit an article */
		else if (mode >= 8)
		{
			/* Naught */
		}

		/* Hack -- The only one of its kind */
		else if (known && artifact_p(o_ptr))
		{
			t = object_desc_str(t, "The ");
		}

		/* A single one, with a vowel in the modifier */
		else if ((*s == '#') && (is_a_vowel(modstr[0])))
		{
			t = object_desc_str(t, "an ");
		}

		else if (ego != NULL)
		{
			if (is_a_vowel(ego[0]))
			{
				t = object_desc_str(t, "an ");
			}
			else
			{
				t = object_desc_str(t, "a ");
			}
		}

		/* A single one, with a vowel */
		else if (is_a_vowel(*s))
		{
			t = object_desc_str(t, "an ");
		}

		/* A single one, without a vowel */
		else
		{
			t = object_desc_str(t, "a ");
		}
	}

	/* Hack -- objects that "never" take an article */
	else
	{
		/* No ampersand */
		s = basenm;

		/* No pref */
		if (!pref)
		{
			/* Nothing */
		}

		/* Hack -- all gone */
		else if (o_ptr->number <= 0)
		{
			t = object_desc_str(t, "no more ");
		}

		/* Prefix a number if required */
		else if (o_ptr->number > 1)
		{
			t = object_desc_num(t, o_ptr->number);
			t = object_desc_chr(t, ' ');
		}

		/* Hack -- The only one of its kind */
		else if (known && artifact_p(o_ptr) && mode < 8)
		{
			t = object_desc_str(t, "The ");
		}

		/* Hack -- single items get no prefix */
		else
		{
			/* Nothing */
		}
	}

	/* Grab any ego-item name */
	//                if ((o_ptr->name2 || o_ptr->name2b) && (o_ptr->tval != TV_ROD_MAIN))
	if (known && (o_ptr->name2 || o_ptr->name2b) && (o_ptr->tval != TV_ROD_MAIN))
	{
		ego_item_type *e_ptr = &e_info[o_ptr->name2];
		ego_item_type *e2_ptr = &e_info[o_ptr->name2b];

		if (e_ptr->before)
		{
			t = object_desc_str(t, (e_name + e_ptr->name));
			t = object_desc_chr(t, ' ');
		}
#if 1
		if (e2_ptr->before)
		{
			t = object_desc_str(t, (e_name + e2_ptr->name));
			t = object_desc_chr(t, ' ');
		}
#endif	// 0
	}

	/* Paranoia -- skip illegal tildes */
	/* while (*s == '~') s++; */

	/* Copy the string */
	for (; *s; s++)
	{
		/* Pluralizer */
		if (*s == '~')
		{
			/* Add a plural if needed */
			if (o_ptr->number != 1)
			{
				char k = t[-1];

				/* XXX XXX XXX Mega-Hack */

				/* Hack -- "Cutlass-es" and "Torch-es" */
				if ((k == 's') || (k == 'h')) *t++ = 'e';

				/* Add an 's' */
				*t++ = 's';
			}
		}

		/* Modifier */
		else if (*s == '#')
		{
                        /* Grab any ego-item name */
                        if (o_ptr->tval == TV_ROD_MAIN)
                        {
                                t = object_desc_chr(t, ' ');

                                if (known && o_ptr->name2)
                                {
                                        ego_item_type *e_ptr = &e_info[o_ptr->name2];

                                        t = object_desc_str(t, (e_name + e_ptr->name));
                                }
                        }

			/* Insert the modifier */
			for (u = modstr; *u; u++) *t++ = *u;
		}

		/* Normal */
		else
		{
			/* Copy */
			*t++ = *s;
		}
	}

	/* Terminate */
	*t = '\0';


	/* Append the "kind name" to the "base name" */
	if (append_name)
	{
		t = object_desc_str(t,mode < 8 ? " of " : " ");
		t = object_desc_str(t, (k_name + k_ptr->name));
	}


	/* Hack -- Append "Artifact" or "Special" names */
	if (known)
	{
		/* Grab any ego-item name */
//                if ((o_ptr->name2 || o_ptr->name2b) && (o_ptr->tval != TV_ROD_MAIN))
		if ((o_ptr->name2 || o_ptr->name2b) && (o_ptr->tval != TV_ROD_MAIN))
		{
			ego_item_type *e_ptr = &e_info[o_ptr->name2];
			ego_item_type *e2_ptr = &e_info[o_ptr->name2b];

                        if (!e_ptr->before && o_ptr->name2)
                        {
                                t = object_desc_chr(t, ' ');
                                t = object_desc_str(t, (e_name + e_ptr->name));
                        }
#if 1
                        else if (!e2_ptr->before && o_ptr->name2b)
                        {
                                t = object_desc_chr(t, ' ');
                                t = object_desc_str(t, (e_name + e2_ptr->name));
//                                ego = e2_ptr->name + e_name;
						}
#endif	// 0
		}

		/* Grab any randart name */
                if (o_ptr->name1 == ART_RANDART)
		{
			t = object_desc_chr(t, ' ');

			if(o_ptr->tval==TV_RING && o_ptr->sval==SV_RING_SPECIAL){
				monster_race *r_ptr=&r_info[o_ptr->bpval];
				t=object_desc_str(t, "of ");
				if(!(r_ptr->flags7 & RF7_NAZGUL)){
					t=object_desc_str(t, "bug");
				}
				else{
					t=object_desc_str(t, r_name+r_ptr->name);
				}
				
			}
			else{
				/* Create the name */
				randart_name(o_ptr, tmp_val);
				t = object_desc_str(t, tmp_val);
			}
		}
			
		/* Grab any artifact name */
		else if (o_ptr->name1)
		{
			artifact_type *a_ptr = &a_info[o_ptr->name1];

			t = object_desc_chr(t, ' ');
			t = object_desc_str(t, (a_name + a_ptr->name));
		}

		/* -TM- Hack -- Add false-artifact names */
		/* Dagger inscribed {@w0#of Smell} will be named
		 * Dagger of Smell {@w0} */
		if (o_ptr->note)
		{
			cptr str = strchr(quark_str(o_ptr->note), '#');

			/* Add the false name */
			if (str)
			{
				t = object_desc_chr(t, ' ');
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
			if (!strcmp(name, p_ptr->name))
			{
				t = object_desc_chr(t, '+');
			}
			else
			{
				t = object_desc_str(t, name);
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
	if (!known)
	{
		t = object_desc_chr(t, '?');
	}
	else
	{
		t = object_desc_num(t, o_ptr->level);
	}
	t = object_desc_chr(t, '}');


	/* No more details wanted */
	if ((mode & 7) < 1)
	{
		if (t - buf <= 65 || mode & 8)
		{
			return;
		}
		else
		{
			object_desc(Ind, buf, o_ptr, pref, mode + 8);
			return;
		}
	}


	/* Hack -- Chests must be described in detail */
	if (o_ptr->tval == TV_CHEST)
	{
		/* Not searched yet */
		if (!known)
		{
			/* Nothing */
		}

		/* May be "empty" */
		else if (!o_ptr->pval)
		{
			t = object_desc_str(t, " (empty)");
		}

		/* May be "disarmed" */
		else if (o_ptr->pval < 0)
		{
			t = object_desc_str(t, " (disarmed)");
#if 0
			if (chest_traps[o_ptr->pval])
			{
				t = object_desc_str(t, " (disarmed)");
			}
			else
			{
				t = object_desc_str(t, " (unlocked)");
			}
#endif	// 0
		}

		/* Describe the traps, if any */
		else
		{
			/* Describe the traps */
			t = object_desc_str(t, " (");
			if (p_ptr->trap_ident[o_ptr->pval])
				t = object_desc_str(t, t_name + t_info[o_ptr->pval].name);
			else
				t = object_desc_str(t, "trapped");
			t = object_desc_str(t, ")");
#if 0
			/* Describe the traps */
			switch (chest_traps[o_ptr->pval])
			{
				case 0:
				{
					t = object_desc_str(t, " (Locked)");
					break;
				}
				case CHEST_LOSE_STR:
				{
					t = object_desc_str(t, " (Poison Needle)");
					break;
				}
				case CHEST_LOSE_CON:
				{
					t = object_desc_str(t, " (Poison Needle)");
					break;
				}
				case CHEST_POISON:
				{
					t = object_desc_str(t, " (Gas Trap)");
					break;
				}
				case CHEST_PARALYZE:
				{
					t = object_desc_str(t, " (Gas Trap)");
					break;
				}
				case CHEST_EXPLODE:
				{
					t = object_desc_str(t, " (Explosion Device)");
					break;
				}
				case CHEST_SUMMON:
				{
					t = object_desc_str(t, " (Summoning Runes)");
					break;
				}
				default:
				{
					t = object_desc_str(t, " (Multiple Traps)");
					break;
				}
			}
#endif	// 0
		}
	}


	/* Display the item like a weapon */
	if (f3 & TR3_SHOW_MODS) show_weapon = TRUE;

	/* Display the item like a weapon */
	if (o_ptr->to_h && o_ptr->to_d) show_weapon = TRUE;

	/* Display the item like armour */
	if (o_ptr->ac) show_armour = TRUE;


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

		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_DIGGING:
                case TV_BOOMERANG:
                case TV_AXE:
                case TV_MSTAFF:

		/* Append a "damage" string */
		if (mode < 8) t = object_desc_chr(t, ' ');
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
		if (mode < 8) t = object_desc_chr(t, ' ');
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
			if (mode < 8) t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, p1);
			t = object_desc_int(t, o_ptr->to_h);
			t = object_desc_chr(t, ',');
			t = object_desc_int(t, o_ptr->to_d);
			t = object_desc_chr(t, p2);
		}

		/* Show the tohit if needed */
		else if (o_ptr->to_h)
		{
			if (mode < 8) t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, p1);
			t = object_desc_int(t, o_ptr->to_h);
			t = object_desc_chr(t, p2);
		}

		/* Show the todam if needed */
		else if (o_ptr->to_d)
		{
			if (mode < 8) t = object_desc_chr(t, ' ');
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
			if (mode < 8) t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, b1);
			t = object_desc_num(t, o_ptr->ac);
			t = object_desc_chr(t, ',');
			t = object_desc_int(t, o_ptr->to_a);
			t = object_desc_chr(t, b2);
		}

		/* No base armor, but does increase armor */
		else if (o_ptr->to_a)
		{
			if (mode < 8) t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, b1);
			t = object_desc_int(t, o_ptr->to_a);
			t = object_desc_chr(t, b2);
		}
	}

	/* Hack -- always show base armor */
	else if (show_armour)
	{
		if (mode < 8) t = object_desc_chr(t, ' ');
		t = object_desc_chr(t, b1);
		t = object_desc_num(t, o_ptr->ac);
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
			object_desc(Ind, buf, o_ptr, pref, mode + 8);
			return;
		}
	}


	/* Hack -- Wands and Staffs have charges */
	if (known &&
	    ((o_ptr->tval == TV_STAFF) ||
	     (o_ptr->tval == TV_WAND)))
	{
		/* Dump " (N charges)" */
		if (mode < 8) t = object_desc_chr(t, ' ');
		t = object_desc_chr(t, p1);
		t = object_desc_num(t, o_ptr->pval);
		if (mode < 8)
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
		if (o_ptr->pval) t = object_desc_str(t, mode < 8 ? " (charging)" : "(#)");
	}
#if 0
        /*
         * Hack -- Rods have a "charging" indicator.
	 */
        else if (known && (o_ptr->tval == TV_ROD_MAIN))
	{
                /* Display prettily. */
                t = object_desc_str(t, " (");
                t = object_desc_num(t, o_ptr->timeout);
                t = object_desc_chr(t, '/');
                t = object_desc_num(t, o_ptr->pval2);
                t = object_desc_chr(t, ')');
	}

        /*
         * Hack -- Rods have a "charging" indicator.
	 */
        else if (known && (o_ptr->tval == TV_ROD))
	{
                /* Display prettily. */
                t = object_desc_str(t, " (");
                t = object_desc_num(t, o_ptr->pval);
                t = object_desc_str(t, " Mana to cast");
                t = object_desc_chr(t, ')');
	}
#endif	// 0

	/* Hack -- Process Lanterns/Torches */
//	else if ((o_ptr->tval == TV_LITE) && (o_ptr->sval < SV_LITE_DWARVEN) && (!o_ptr->name3))
        else if ((o_ptr->tval == TV_LITE) && (f4 & TR4_FUEL_LITE))
	{
		/* Hack -- Turns of light for normal lites */
		t = object_desc_str(t, mode < 8 ? " (with " : "(");
		t = object_desc_num(t, o_ptr->timeout);
		t = object_desc_str(t, mode < 8 ? " turns of light)" : "t)");
	}


	/* Dump "pval" flags for wearable items */
        if (known && (((f1 & (TR1_PVAL_MASK)) || (f5 & (TR5_PVAL_MASK))) || (o_ptr->tval == TV_GOLEM)))
	{
		/* Hack -- first display any base pval bonuses.  
		 * The "bpval" flags are never displayed.  */
		if (o_ptr->bpval)
		{
			if (mode < 8) t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, p1);
			/* Dump the "pval" itself */
			t = object_desc_int(t, o_ptr->bpval);
			t = object_desc_chr(t, p2);
		}
		/* Next, display any pval bonuses. */
		if (o_ptr->pval)
		{
			/* Start the display */
			if (mode < 8) t = object_desc_chr(t, ' ');
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
				t = object_desc_str(t, mode < 8 ? " to speed" : "spd");
			}

			/* Attack speed */
			else if (f1 & TR1_BLOWS)
			{
				/* Add " attack" */
				t = object_desc_str(t, mode < 8 ? " attack" : "at");

				/* Add "attacks" */
				if (ABS(o_ptr->pval) != 1 && mode < 8) t = object_desc_chr(t, 's');
			}

			/* Critical chance */
			else if (f5 & (TR5_CRIT))
			{
				/* Add " attack" */
				t = object_desc_str(t, mode < 8 ? "% of critical hits" : "crt");
			}

			/* Stealth */
			else if (f1 & TR1_STEALTH)
			{
				/* Dump " to stealth" */
				t = object_desc_str(t, mode < 8 ? " to stealth" : "stl");
			}

			/* Search */
			else if (f1 & TR1_SEARCH)
			{
				/* Dump " to searching" */
				t = object_desc_str(t, mode < 8 ? " to searching" : "srch");
			}

			/* Infravision */
			else if (f1 & TR1_INFRA)
			{
				/* Dump " to infravision" */
				t = object_desc_str(t, mode < 8 ? " to infravision" : "infr");
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

	/* Special case, ugly, but needed */
	if (known && aware && (o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH))
	{
		t = object_desc_str(t, mode < 8 ? " of " : "-");
		t = object_desc_str(t, r_info[o_ptr->pval].name + r_name);
	}


	/* Indicate "charging" artifacts XXX XXX XXX */
	if (known && o_ptr->timeout && !((o_ptr->tval == TV_LITE) && (f4 & TR4_FUEL_LITE)))
	{
		/* Hack -- Dump " (charging)" if relevant */
		t = object_desc_str(t, mode < 8 ? " (charging)" : "(#)");
	}

#if 0	/* Now the stores does *ID* */
	if (o_ptr->ident & ID_MENTAL)
	{
		t = object_desc_chr(t, '*');
	}
#endif	// 0


	/* No more details wanted */
	if ((mode & 7) < 3)
	{
		if (t - buf <= 65 || mode & 8)
		{
			return;
		}
		else
		{
			object_desc(Ind, buf, o_ptr, pref, mode + 8);
			return;
		}
	}


	/* No inscription yet */
	tmp_val[0] = '\0';

	/* Use the standard inscription if available */
        if (o_ptr->note)
	{
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
	if (tmp_val[0])
	{
		int n;

		/* Hack -- How much so far */
		n = (t - buf);

		/* Paranoia -- do not be stupid */
		if (n > 75) n = 75;

		/* Hack -- shrink the inscription */
		tmp_val[75 - n] = '\0';

		/* Append the inscription */
		if (mode < 8) t = object_desc_chr(t, ' ');
		t = object_desc_chr(t, c1);
		t = object_desc_str(t, tmp_val);
		t = object_desc_chr(t, c2);
	}

	/* This should always be true, but still.. */
	if ((mode & 7) < 4)
	{
		if (t - buf <= 65 || mode >= 8)
		{
			return;
		}
		else
		{
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
	if (Ind)
	{
		/* Save the "aware" flag */
		hack_aware = p_ptr->obj_aware[o_ptr->k_idx];

		/* Save the "known" flag */
		hack_known = (o_ptr->ident & ID_KNOWN) ? TRUE : FALSE;
	}

	/* Set the "known" flag */
	o_ptr->ident |= ID_KNOWN;

	/* Valid players only */
	if (Ind)
	{
		/* Force "aware" for description */
		p_ptr->obj_aware[o_ptr->k_idx] = TRUE;
	}


	/* Describe the object */
	object_desc(Ind, buf, o_ptr, pref, mode);


	/* Only restore flags if we have a valid player */
	if (Ind)
	{
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
		  return "polymorph into an Ancient Blue Dragon every 200+d100 turns";
		}
	      case SV_DRAGON_WHITE:
		{
		  return "polymorph into an Ancient White Dragon every 200+d100 turns";
		}
	      case SV_DRAGON_BLACK:
		{
		  return "polymorph into an Ancient Black Dragon every 200+d100 turns";
		}
	      case SV_DRAGON_GREEN:
		{
		  return "polymorph into an Ancient Green Dragon every 200+d100 turns";
		}
	      case SV_DRAGON_RED:
		{
		  return "polymorph into an Ancient Red Dragon every 200+d100 turns";
		}
	      case SV_DRAGON_MULTIHUED:
		{
		  return "polymorph into an Ancient MultiHued Dragon every 200+d100 turns";
		}
	      case SV_DRAGON_PSEUDO:
		{
		  //return "polymorph into a Pseudo Dragon every 200+d100 turns";
		  return "polymorph into an Ethereal Drake every 200+d100 turns";
		}
	      case SV_DRAGON_BRONZE:
		{
		  return "polymorph into an Ancient Bronze Dragon every 200+d100 turns";
		}
	      case SV_DRAGON_GOLD:
		{
		  return "polymorph into an Ancient Gold Dragon every 200+d100 turns";
		}
	      case SV_DRAGON_CHAOS:
		{
		  return "polymorph into a Great Wyrm of Chaos every 200+d100 turns";
		}
	      case SV_DRAGON_LAW:
		{
		  return "polymorph into a Great Wyrm of Law every 200+d100 turns";
		}
	      case SV_DRAGON_BALANCE:
		{
		  return "polymorph into a Great Wyrm of Balance every 200+d100 turns";
		}
	      case SV_DRAGON_SHINING:
		{
		  return "polymorph into an Ethereal Dragon every 200+d100 turns";
		}
	      case SV_DRAGON_POWER:
		{
		  return "polymorph into a Great Wyrm of Power every 200+d100 turns";
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
			return "stinking cloud (12) every 4+d4 turns";
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
			return "vampiric drain (3*100) every 250 turns";
		}
		case ART_NATUREBANE:
		{
			return "dispel monsters (300) every 200+d200 turns";
		}
		case ART_BILBO:
		{
			return "destroy doors and traps every 30+d30 turns";
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
	if (is_ego_p(o_ptr, EGO_SPINING))
	{
		return "spining around every 50+d25 turns";
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
					return format("polymorph into %s",
							r_info[o_ptr->pval].name + r_name);
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
				return "entering berserk rage every 150+d100 turns";
			default:
				return NULL;
		}
	}

	/* For the moment ignore (non-ego) randarts */
	if (o_ptr->name1 == ART_RANDART) return "a crash ;-)";

	/* Oops */
	return NULL;
#if 0	// memo
	/* Some artifacts can be activated */
	switch (o_ptr->name1)
	{
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
			return "stinking cloud (12) every 4+d4 turns";
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
			return "probing every 20 turns";
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
		case ART_ARUNRUTH:
		{
			return "frost bolt (12d8) every 500 turns";
		}
		case ART_AEGLOS:
		{
			return "frost ball (100) every 500 turns";
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
			return "phase door every 2 turns";
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
			return "heal (500) every 500 turns";
		}
		case ART_RAZORBACK:
		{
			return "star ball (150) every 1000 turns";
		}
		case ART_BLADETURNER:
		{
			return "berserk rage, bless, and resistance every 400 turns";
		}
		case ART_GALADRIEL:
		{
			return "illumination every 10+d10 turns";
		}
		case ART_ELENDIL:
		{
			return "magic mapping every 50+d50 turns";
		}
		case ART_THRAIN:
		{
			return "clairvoyance every 100+d100 turns";
		}
		case ART_INGWE:
		{
			return "dispel evil (x5) every 300+d300 turns";
		}
		case ART_CARLAMMAS:
		{
			return "protection from evil every 225+d225 turns";
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
		case ART_POWER:
		{
			return "bizarre things every 450+d450 turns";
		}
	}


	/* From PernA.. some of them won't work as is described :p */
        if (is_ego_p(o_ptr, EGO_MSTAFF_SPELL))
        {
                int gf, mod, mana;

                if (!num)
                {
                        gf = o_ptr->pval & 0xFFFF;
                        mod = o_ptr->pval3 & 0xFFFF;
                        mana = o_ptr->pval2 & 0xFF;
                }
                else
                {
                        gf = o_ptr->pval >> 16;
                        mod = o_ptr->pval3 >> 16;
                        mana = o_ptr->pval2 >> 8;
                }
                sprintf(rspell[num], "runespell(%s, %s, %d) every %d turns",
                        k_info[lookup_kind(TV_RUNE1, gf)].name + k_name,
                        k_info[lookup_kind(TV_RUNE2, mod)].name + k_name,
                        mana, mana * 10);
                return rspell[num];
        }

        if ((!(o_ptr->name1) &&
	    !(o_ptr->name2) &&
	    !(o_ptr->xtra1) &&
             (o_ptr->xtra2)))
	{
		switch (o_ptr->xtra2)
		{
			case ACT_SUNLIGHT:
			{
				return "beam of sunlight every 10 turns";
			}
			case ACT_BO_MISS_1:
			{
				return "magic missile (2d6) every 2 turns";
			}
			case ACT_BA_POIS_1:
			{
				return "stinking cloud (12), rad. 3, every 4+d4 turns";
			}
			case ACT_BO_ELEC_1:
			{
				return "lightning bolt (4d8) every 6+d6 turns";
			}
			case ACT_BO_ACID_1:
			{
				return "acid bolt (5d8) every 5+d5 turns";
			}
			case ACT_BO_COLD_1:
			{
				return "frost bolt (6d8) every 7+d7 turns";
			}
			case ACT_BO_FIRE_1:
			{
				return "fire bolt (9d8) every 8+d8 turns";
			}
			case ACT_BA_COLD_1:
			{
				return "ball of cold (48) every 400 turns";
			}
			case ACT_BA_FIRE_1:
			{
				return "ball of fire (72) every 400 turns";
			}
			case ACT_DRAIN_1:
			{
				return "drain life (100) every 100+d100 turns";
			}
			case ACT_BA_COLD_2:
			{
				return "ball of cold (100) every 300 turns";
			}
			case ACT_BA_ELEC_2:
			{
				return "ball of lightning (100) every 500 turns";
			}
			case ACT_DRAIN_2:
			{
				return "drain life (120) every 400 turns";
			}
			case ACT_VAMPIRE_1:
			{
				return "vampiric drain (3*50) every 400 turns";
			}
			case ACT_BO_MISS_2:
			{
				return "arrows (150) every 90+d90 turns";
			}
			case ACT_BA_FIRE_2:
			{
				return "fire ball (120) every 225+d225 turns";
			}
			case ACT_BA_COLD_3:
			{
				return "ball of cold (200) every 325+d325 turns";
			}
			case ACT_WHIRLWIND:
			{
				return "whirlwind attack every 250 turns";
			}
			case ACT_VAMPIRE_2:
			{
				return "vampiric drain (3*100) every 400 turns";
			}
			case ACT_CALL_CHAOS:
			{
				return "call chaos every 350 turns";
			}
			case ACT_ROCKET:
			{
				return "launch rocket (120+level) every 400 turns";
			}
			case ACT_DISP_EVIL:
			{
				return "dispel evil (level*5) every 300+d300 turns";
			}
			case ACT_DISP_GOOD:
			{
				return "dispel good (level*5) every 300+d300 turns";
			}
			case ACT_BA_MISS_3:
			{
				return "elemental breath (300) every 500 turns";
			}
			case ACT_CONFUSE:
			{
				return "confuse monster every 15 turns";
			}
			case ACT_SLEEP:
			{
				return "sleep nearby monsters every 55 turns";
			}
			case ACT_QUAKE:
			{
				return "earthquake (rad 10) every 50 turns";
			}
			case ACT_TERROR:
			{
				return "terror every 3 * (level+10) turns";
			}
			case ACT_TELE_AWAY:
			{
				return "teleport away every 200 turns";
			}
			case ACT_BANISH_EVIL:
			{
				return "banish evil every 250+d250 turns";
			}
			case ACT_GENOCIDE:
			{
				return "genocide every 500 turns";
			}
			case ACT_MASS_GENO:
			{
				return "mass genocide every 1000 turns";
			}
			case ACT_CHARM_ANIMAL:
			{
				return "charm animal every 300 turns";
			}
			case ACT_CHARM_UNDEAD:
			{
				return "enslave undead every 333 turns";
			}
			case ACT_CHARM_OTHER:
			{
				return "charm monster every 400 turns";
			}
			case ACT_CHARM_ANIMALS:
			{
				return "animal friendship every 500 turns";
			}
			case ACT_CHARM_OTHERS:
			{
				return "mass charm every 750 turns";
			}
			case ACT_SUMMON_ANIMAL:
			{
				return "summon animal every 200+d300 turns";
			}
			case ACT_SUMMON_PHANTOM:
			{
				return "summon phantasmal servant every 200+d200 turns";
			}
			case ACT_SUMMON_ELEMENTAL:
			{
				return "summon elemental every 750 turns";
			}
			case ACT_SUMMON_DEMON:
			{
				return "summon demon every 666+d333 turns";
			}
			case ACT_SUMMON_UNDEAD:
			{
				return "summon undead every 666+d333 turns";
			}
			case ACT_CURE_LW:
			{
				return "remove fear & heal 30 hp every 10 turns";
			}
			case ACT_CURE_MW:
			{
				return "heal 4d8 & wounds every 3+d3 turns";
			}
			case ACT_CURE_POISON:
			{
				return "remove fear and cure poison every 5 turns";
			}
			case ACT_REST_LIFE:
			{
				return "restore life levels every 450 turns";
			}
			case ACT_REST_ALL:
			{
				return "restore stats and life levels every 750 turns";
			}
			case ACT_CURE_700:
			{
				return "heal 700 hit points every 250 turns";
			}
			case ACT_CURE_1000:
			{
				return "heal 1000 hit points every 888 turns";
			}
			case ACT_ESP:
			{
				return "temporary ESP (dur 25+d30) every 200 turns";
			}
			case ACT_BERSERK:
			{
				return "heroism and berserk (dur 50+d50) every 100+d100 turns";
			}
			case ACT_PROT_EVIL:
			{
				return "protect evil (dur level*3 + d25) every 225+d225 turns";
			}
			case ACT_RESIST_ALL:
			{
				return "resist elements (dur 40+d40) every 200 turns";
			}
			case ACT_SPEED:
			{
				return "speed (dur 20+d20) every 250 turns";
			}
			case ACT_XTRA_SPEED:
			{
				return "speed (dur 75+d75) every 200+d200 turns";
			}
			case ACT_WRAITH:
			{
				return "wraith form (level/2 + d(level/2)) every 1000 turns";
			}
			case ACT_INVULN:
			{
				return "invulnerability (dur 8+d8) every 1000 turns";
			}
			case ACT_LIGHT:
			{
				return "light area (dam 2d15) every 10+d10 turns";
			}
			case ACT_MAP_LIGHT:
			{
				return "light (dam 2d15) & map area every 50+d50 turns";
			}
			case ACT_DETECT_ALL:
			{
				return "detection every 55+d55 turns";
			}
			case ACT_DETECT_XTRA:
			{
				return "detection, probing and identify true every 1000 turns";
			}
			case ACT_ID_FULL:
			{
				return "identify true every 750 turns";
			}
			case ACT_ID_PLAIN:
			{
				return "identify spell every 10 turns";
			}
			case ACT_RUNE_EXPLO:
			{
				return "explosive rune every 200 turns";
			}
			case ACT_RUNE_PROT:
			{
				return "rune of protection every 400 turns";
			}
			case ACT_SATIATE:
			{
				return "satisfy hunger every 200 turns";
			}
			case ACT_DEST_DOOR:
			{
				return "destroy doors every 10 turns";
			}
			case ACT_STONE_MUD:
			{
				return "stone to mud every 5 turns";
			}
			case ACT_RECHARGE:
			{
				return "recharging every 70 turns";
			}
			case ACT_ALCHEMY:
			{
				return "alchemy every 500 turns";
			}
			case ACT_DIM_DOOR:
			{
				return "dimension door every 100 turns";
			}
			case ACT_TELEPORT:
			{
				return "teleport (range 100) every 45 turns";
			}
			case ACT_RECALL:
			{
				return "word of recall every 200 turns";
			}
			default:
			{
				return "something undefined";
			}
		}
	}
#endif	// 0
}

/*
 * Display the damage done with a multiplier
 */
//void output_dam(object_type *o_ptr, int mult, int mult2, cptr against, cptr against2, bool *first)
static void output_dam(int Ind, FILE *fff, object_type *o_ptr, int mult, int mult2, cptr against, cptr against2)
{
	player_type *p_ptr = Players[Ind];
	int dam;

	dam = (o_ptr->dd + (o_ptr->dd * o_ptr->ds)) * 5 * mult;
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
		dam = (o_ptr->dd + (o_ptr->dd * o_ptr->ds)) * 5 * mult2;
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
static void display_weapon_damage(int Ind, object_type *o_ptr, FILE *fff)
{
	player_type *p_ptr = Players[Ind];
	object_type forge, *old_ptr = &forge;
	u32b f1, f2, f3, f4, f5, esp;
	bool first = TRUE;

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	/* Ok now the hackish stuff, we replace the current weapon with this one */
	/* XXX this hack can be even worse under TomeNET, dunno :p */
	object_copy(old_ptr, &p_ptr->inventory[INVEN_WIELD]);
	object_copy(&p_ptr->inventory[INVEN_WIELD], o_ptr);

	/* Hack -- hush the messages up */
	suppress_message = TRUE;
	calc_bonuses(Ind);

	fprintf(fff, "\n");
#if 0 //double -> obsolete (already within the only calling function)
        switch(o_ptr->tval){
        case TV_HAFTED:
                fprintf(fff, "It's a hafted weapon.\n"); break;
        case TV_POLEARM:
                fprintf(fff, "It's a polearm.\n"); break;
        case TV_SWORD:
                fprintf(fff, "It's a sword-type weapon.\n"); break;
        case TV_AXE:
                fprintf(fff, "It's an axe-type weapon.\n"); break;
        default:
                break;
        }

        if (f4 & TR4_COULD2H) fprintf(fff, "It can be wielded two-handed.\n");
	if (f4 & TR4_MUST2H) fprintf(fff, "It must be wielded two-handed.\n");
#endif
	fprintf(fff, "Using it you would have %d blow%s and do an average damage per turn of:\n", p_ptr->num_blow, (p_ptr->num_blow) ? "s" : "");

	if (f1 & TR1_SLAY_ANIMAL) output_dam(Ind, fff, o_ptr, 2, 0, "animals", NULL);
	if (f1 & TR1_SLAY_EVIL) output_dam(Ind, fff, o_ptr, 2, 0, "evil creatures", NULL);
	if (f1 & TR1_SLAY_ORC) output_dam(Ind, fff, o_ptr, 3, 0, "orcs", NULL);
	if (f1 & TR1_SLAY_TROLL) output_dam(Ind, fff, o_ptr, 3, 0, "trolls", NULL);
	if (f1 & TR1_SLAY_GIANT) output_dam(Ind, fff, o_ptr, 3, 0, "giants", NULL);
	if (f1 & TR1_KILL_DRAGON) output_dam(Ind, fff, o_ptr, 5, 0, "dragons", NULL);
	else if (f1 & TR1_SLAY_DRAGON) output_dam(Ind, fff, o_ptr, 3, 0, "dragons", NULL);
	if (f5 & TR5_KILL_UNDEAD) output_dam(Ind, fff, o_ptr, 5, 0, "undeads", NULL);
	else if (f1 & TR1_SLAY_UNDEAD) output_dam(Ind, fff, o_ptr, 3, 0, "undeads", NULL);
	if (f5 & TR5_KILL_DEMON) output_dam(Ind, fff, o_ptr, 5, 0, "demons", NULL);
	else if (f1 & TR1_SLAY_DEMON) output_dam(Ind, fff, o_ptr, 3, 0, "demons", NULL);

	if (f1 & TR1_BRAND_FIRE) output_dam(Ind, fff, o_ptr, 3, 6, "non fire resistant creatures", "fire susceptible creatures");
	if (f1 & TR1_BRAND_COLD) output_dam(Ind, fff, o_ptr, 3, 6, "non cold resistant creatures", "cold susceptible creatures");
	if (f1 & TR1_BRAND_ELEC) output_dam(Ind, fff, o_ptr, 3, 6, "non lightning resistant creatures", "lightning susceptible creatures");
	if (f1 & TR1_BRAND_ACID) output_dam(Ind, fff, o_ptr, 3, 6, "non acid resistant creatures", "acid susceptible creatures");
	if (f1 & TR1_BRAND_POIS) output_dam(Ind, fff, o_ptr, 3, 6, "non poison resistant creatures", "poison susceptible creatures");

	output_dam(Ind, fff, o_ptr, 1, 0, (first) ? "all monsters" : "other monsters", NULL);

	fprintf(fff, "\n");

	/* get our weapon back */
	object_copy(&p_ptr->inventory[INVEN_WIELD], old_ptr);
	calc_bonuses(Ind);
	suppress_message = FALSE;
}
	
/*
 * Display the ammo damage done with a multiplier
 */
//void output_ammo_dam(object_type *o_ptr, int mult, int mult2, cptr against, cptr against2, bool *first)
static void output_ammo_dam(int Ind, FILE *fff, object_type *o_ptr, int mult, int mult2, cptr against, cptr against2)
{
	player_type *p_ptr = Players[Ind];
	int dam;
	object_type *b_ptr = &p_ptr->inventory[INVEN_BOW];
	int tmul = get_shooter_mult(o_ptr);
	tmul += p_ptr->xtra_might;

	dam = (o_ptr->dd + (o_ptr->dd * o_ptr->ds)) * 5;
	dam += (o_ptr->to_d + b_ptr->to_d) * 10;
	dam *= tmul;
	dam += (p_ptr->to_d_ranged) * 10;
	dam *= mult;
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
		dam = (o_ptr->dd + (o_ptr->dd * o_ptr->ds)) * 5;
		dam += (o_ptr->to_d + b_ptr->to_d) * 10;
		dam *= tmul;
		dam += (p_ptr->to_d_ranged) * 10;
		dam *= mult2;
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

/*
 * Outputs the damage we do/would do with the current bow and this ammo
 */
/* TODO: tell something about boomerangs */
static void display_ammo_damage(int Ind, object_type *o_ptr, FILE *fff)
{
	//player_type *p_ptr = Players[Ind];
	u32b f1, f2, f3, f4, f5, esp;
	bool first = TRUE;
	// int i;

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	fprintf(fff, "\nUsing it with your current shooter you would do an avarage damage per shot of:\n");
	if (f1 & TR1_SLAY_ANIMAL) output_ammo_dam(Ind, fff, o_ptr, 2, 0, "animals", NULL);
	if (f1 & TR1_SLAY_EVIL) output_ammo_dam(Ind, fff, o_ptr, 2, 0, "evil creatures", NULL);
	if (f1 & TR1_SLAY_ORC) output_ammo_dam(Ind, fff, o_ptr, 3, 0, "orcs", NULL);
	if (f1 & TR1_SLAY_TROLL) output_ammo_dam(Ind, fff, o_ptr, 3, 0, "trolls", NULL);
	if (f1 & TR1_SLAY_GIANT) output_ammo_dam(Ind, fff, o_ptr, 3, 0, "giants", NULL);
	if (f1 & TR1_KILL_DRAGON) output_ammo_dam(Ind, fff, o_ptr, 5, 0, "dragons", NULL);
	else if (f1 & TR1_SLAY_DRAGON) output_ammo_dam(Ind, fff, o_ptr, 3, 0, "dragons", NULL);
	if (f5 & TR5_KILL_UNDEAD) output_ammo_dam(Ind, fff, o_ptr, 5, 0, "undeads", NULL);
	else if (f1 & TR1_SLAY_UNDEAD) output_ammo_dam(Ind, fff, o_ptr, 3, 0, "undeads", NULL);
	if (f5 & TR5_KILL_DEMON) output_ammo_dam(Ind, fff, o_ptr, 5, 0, "demons", NULL);
	else if (f1 & TR1_SLAY_DEMON) output_ammo_dam(Ind, fff, o_ptr, 3, 0, "demons", NULL);

	if (f1 & TR1_BRAND_FIRE) output_ammo_dam(Ind, fff, o_ptr, 3, 6, "non fire resistant creatures", "fire susceptible creatures");
	if (f1 & TR1_BRAND_COLD) output_ammo_dam(Ind, fff, o_ptr, 3, 6, "non cold resistant creatures", "cold susceptible creatures");
	if (f1 & TR1_BRAND_ELEC) output_ammo_dam(Ind, fff, o_ptr, 3, 6, "non lightning resistant creatures", "lightning susceptible creatures");
	if (f1 & TR1_BRAND_ACID) output_ammo_dam(Ind, fff, o_ptr, 3, 6, "non acid resistant creatures", "acid susceptible creatures");
	if (f1 & TR1_BRAND_POIS) output_ammo_dam(Ind, fff, o_ptr, 3, 6, "non poison resistant creatures", "poison susceptible creatures");

	output_ammo_dam(Ind, fff, o_ptr, 1, 0, (first) ? "all monsters" : "other monsters", NULL);
	fprintf(fff, "\n");

#if 0
	if (o_ptr->pval2)
	{
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
}


/*
 * Describe a "fully identified" item
 */
//bool identify_fully_aux(object_type *o_ptr, FILE *fff)
bool identify_fully_aux(int Ind, object_type *o_ptr)
{
	player_type *p_ptr = Players[Ind];
//	cptr		*info = p_ptr->info;

	int j, am;

	u32b f1, f2, f3, f4, f5, esp;

	//        cptr            info[400];
	//        byte            color[400];

	FILE *fff;
	char    buf[1024], o_name[150];

#if 0
	char file_name[MAX_PATH_LENGTH];

	/* Temporary file */
	if (path_temp(file_name, MAX_PATH_LENGTH)) return;

	strcpy(p_ptr->infofile, file_name);
#endif	// 0

	/* Open a new file */
	fff = my_fopen(p_ptr->infofile, "w");

	/* Current file viewing */
	strcpy(p_ptr->cur_file, p_ptr->infofile);

	/* Let the player scroll through this info */
	p_ptr->special_file_type = TRUE;

	/* Output color byte */
//	fprintf(fff, "%c", 'w');


	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

        /* Describe the result */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);

	fprintf(fff, "%s\n", o_name);
	if (strlen(o_name) > 77) fprintf(fff, "%s\n", o_name + 77);

        switch(o_ptr->tval){
        case TV_HAFTED:
                fprintf(fff, "It's a hafted weapon.\n"); break;
        case TV_POLEARM:
                fprintf(fff, "It's a polearm.\n"); break;
        case TV_SWORD:
                fprintf(fff, "It's a sword-type weapon.\n"); break;
        case TV_AXE:
                fprintf(fff, "It's an axe-type weapon.\n"); break;
        default:
                break;
        }

#if 0
	/* All white */
	for (j = 0; j < 400; j++)
		color[j] = TERM_WHITE;

	/* No need to dump that */
	if (fff == NULL)
	{
		i = grab_tval_desc(o_ptr->tval, info, 0);
		if ((fff == NULL) && i) info[i++] = "";
		for (j = 0; j < i; j++)
			color[j] = TERM_L_BLUE;
	}

	if (o_ptr->k_idx)
	{

		char buff2[400], *s, *t;
		int n, oi = i;
		object_kind *k_ptr = &k_info[o_ptr->k_idx];

		strcpy (buff2, k_text + k_ptr->text);

		s = buff2;

		/* Collect the history */
		while (TRUE)
		{

			/* Extract remaining length */
			n = strlen(s);

			/* All done */
			if (n < 60)
			{
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

	if (o_ptr->name1)
	{

		char buff2[400], *s, *t;
		int n, oi = i;
		artifact_type *a_ptr = &a_info[o_ptr->name1];

		strcpy (buff2, a_text + a_ptr->text);

		s = buff2;

		/* Collect the history */
		while (TRUE)
		{

			/* Extract remaining length */
			n = strlen(s);

			/* All done */
			if (n < 60)
			{
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

		if (a_ptr->set != -1)
		{
			char buff2[400], *s, *t;
			int n;
			set_type *set_ptr = &set_info[a_ptr->set];

			strcpy (buff2, set_text + set_ptr->desc);

			s = buff2;

			/* Collect the history */
			while (TRUE)
			{

				/* Extract remaining length */
				n = strlen(s);

				/* All done */
				if (n < 60)
				{
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

	if (f4 & TR4_LEVELS)
	{
		int j = 0;

		color[i] = TERM_VIOLET;
		if (count_bits(o_ptr->pval3) == 0) info[i++] = "It is sentient.";
		else if (count_bits(o_ptr->pval3) > 1) info[i++] = "It is sentient and can have access to the realms of:";
		else  info[i++] = "It is sentient and can have access to the realm of:";

		for (j = 0; j < MAX_FLAG_GROUP; j++)
		{
			if (BIT(j) & o_ptr->pval3)
			{
				color[i] = flags_groups[j].color;
				info[i++] = flags_groups[j].name;
			}
		}

		/* Add a blank line */
		info[i++] = "";
	}

#endif
	if (f4 & TR4_COULD2H) fprintf(fff, "It can be wielded two-handed.\n");
	if (f4 & TR4_MUST2H) fprintf(fff, "It must be wielded two-handed.\n");

#if 0	// obsolete - DELETEME
	if (wield_slot(Ind, o_ptr) == INVEN_WIELD)
	{
		int blows = calc_blows(Ind, o_ptr);
		fprintf(fff, "With it, you can usually attack %d time%s/turn.\n",
				blows, blows > 1 ? "s" : "");
	}
#endif	// 0

	/* Mega Hack^3 -- describe the amulet of life saving */
	if (o_ptr->tval == TV_AMULET &&
			o_ptr->sval == SV_AMULET_LIFE_SAVING)
	{
		fprintf(fff, "It will save your life from perilous scene once.\n");
	}

	/* Mega-Hack -- describe activation */
	if (f3 & TR3_ACTIVATE)
	{
		fprintf(fff, "It can be activated for...\n");
		fprintf(fff, "%s\n", item_activation(o_ptr));
		fprintf(fff, "...if it is being worn.\n");
	}

#if 0
	/* Mega-Hack -- describe activation */
	if (f3 & (TR3_ACTIVATE))
	{
		info[i++] = "It can be activated for...";
		if (is_ego_p(o_ptr, EGO_MSTAFF_SPELL))
		{
			info[i++] = item_activation(o_ptr, 1);
			info[i++] = "And...";
		}
		info[i++] = item_activation(o_ptr, 0);

		/* Mega-hack -- get rid of useless line for randarts */
		if (o_ptr->tval != TV_RANDART){
			info[i++] = "...if it is being worn.";
		}
	}

	/* Granted power */
	if (object_power(o_ptr) != -1)
	{
		info[i++] = "It grants you the power of...";
		info[i++] = powers_type[object_power(o_ptr)].name;
		info[i++] = "...if it is being worn.";
	}
#endif	// 0


	/* Hack -- describe lite's */
	if (o_ptr->tval == TV_LITE)
	{
		int radius = 0;

		if (f3 & TR3_LITE1) radius++;
		if (f4 & TR4_LITE2) radius += 2;
		if (f4 & TR4_LITE3) radius += 3;

		if (radius > 5) radius = 5;

		if (f4 & TR4_FUEL_LITE)
		{
			fprintf(fff, "It provides light (radius %d) when fueled.\n", radius);
		}
		else if (radius)
		{
			fprintf(fff, "It provides light (radius %d) forever.\n", radius);
		}
		else
		{
			fprintf(fff, "It never provides light.\n");
		}
	}

	/* Mega Hack^3 -- describe the Anchor of Space-time */
	if (o_ptr->name1 == ART_ANCHOR)
	{
		fprintf(fff, "It prevents the space-time continuum from being disrupted.\n");
	}
#if 0
	if ((f4 & (TR4_ANTIMAGIC_50)) || (f4 & (TR4_ANTIMAGIC_30)) || (f4 & (TR4_ANTIMAGIC_20)) || (f4 & (TR4_ANTIMAGIC_10)))
	{
		info[i++] = "It generates an antimagic field.";
	}
#endif	// 0

	am = ((f4 & (TR4_ANTIMAGIC_50)) ? 50 : 0)
		+ ((f4 & (TR4_ANTIMAGIC_30)) ? 30 : 0)
		+ ((f4 & (TR4_ANTIMAGIC_20)) ? 20 : 0)
		+ ((f4 & (TR4_ANTIMAGIC_10)) ? 10 : 0);
	if (am)
	{
		j = o_ptr->to_h + o_ptr->to_d + o_ptr->pval + o_ptr->to_a;
		if (j > 0) am -= j;
	}

	if (am >= 100)
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

	/* And then describe it fully */

	if (f1 & (TR1_STR))
	{
		fprintf(fff, "It affects your strength.\n");
	}
	if (f1 & (TR1_INT))
	{
		fprintf(fff, "It affects your intelligence.\n");
	}
	if (f1 & (TR1_WIS))
	{
		fprintf(fff, "It affects your wisdom.\n");
	}
	if (f1 & (TR1_DEX))
	{
		fprintf(fff, "It affects your dexterity.\n");
	}
	if (f1 & (TR1_CON))
	{
		fprintf(fff, "It affects your constitution.\n");
	}
	if (f1 & (TR1_CHR))
	{
		fprintf(fff, "It affects your charisma.\n");
	}

	if (f1 & (TR1_STEALTH))
	{
#if 0
		fprintf(fff, "It affects your stealth.\n");
#else	// 0
		if (o_ptr->tval != TV_TRAPKIT)
			fprintf(fff, "It affects your stealth.\n");
		else
			fprintf(fff, "It is well-hidden.\n");
#endif	// 0
	}
	if (f1 & (TR1_SEARCH))
	{
		fprintf(fff, "It affects your searching.\n");
	}
	if (f5 & (TR5_DISARM))
	{
		fprintf(fff, "It affects your disarming.\n");
	}
	if (f1 & (TR1_INFRA))
	{
		fprintf(fff, "It affects your infravision.\n");
	}
	if (f1 & (TR1_TUNNEL))
	{
		fprintf(fff, "It affects your ability to tunnel.\n");
	}
	if (f1 & (TR1_SPEED))
	{
		fprintf(fff, "It affects your speed.\n");
	}
	if (f1 & (TR1_BLOWS))
	{
		fprintf(fff, "It affects your attack speed.\n");
	}
	if (f5 & (TR5_CRIT))
	{
		fprintf(fff, "It affects your ability to score critical hits.\n");
	}
	if (f5 & (TR5_LUCK))
	{
		fprintf(fff, "It affects your luck.\n");
	}

	if (f1 & (TR1_BRAND_ACID))
	{
		fprintf(fff, "It does extra damage from acid.\n");
	}
	if (f1 & (TR1_BRAND_ELEC))
	{
		fprintf(fff, "It does extra damage from electricity.\n");
	}
	if (f1 & (TR1_BRAND_FIRE))
	{
		fprintf(fff, "It does extra damage from fire.\n");
	}
	if (f1 & (TR1_BRAND_COLD))
	{
		fprintf(fff, "It does extra damage from frost.\n");
	}

	if (f1 & (TR1_BRAND_POIS))
	{
		fprintf(fff, "It poisons your foes.\n");
	}

	if (f5 & (TR5_CHAOTIC))
	{
		fprintf(fff, "It produces chaotic effects.\n");
	}

	if (f1 & (TR1_VAMPIRIC))
	{
		fprintf(fff, "It drains life from your foes.\n");
	}

	if (f1 & (TR1_IMPACT))
	{
		fprintf(fff, "It can cause earthquakes.\n");
	}

	if (f1 & (TR1_VORPAL))
	{
		fprintf(fff, "It is very sharp and can cut your foes.\n");
	}

	if (f5 & (TR5_WOUNDING))
	{
		fprintf(fff, "It is very sharp and makes your foes bleed.\n");
	}

	if (f1 & (TR1_KILL_DRAGON))
	{
		fprintf(fff, "It is a great bane of dragons.\n");
	}
	else if (f1 & (TR1_SLAY_DRAGON))
	{
		fprintf(fff, "It is especially deadly against dragons.\n");
	}
	if (f1 & (TR1_SLAY_ORC))
	{
		fprintf(fff, "It is especially deadly against orcs.\n");
	}
	if (f1 & (TR1_SLAY_TROLL))
	{
		fprintf(fff, "It is especially deadly against trolls.\n");
	}
	if (f1 & (TR1_SLAY_GIANT))
	{
		fprintf(fff, "It is especially deadly against giants.\n");
	}
	if (f5 & (TR5_KILL_DEMON))
	{
		fprintf(fff, "It is a great bane of demons.\n");
	}
	else if (f1 & (TR1_SLAY_DEMON))
	{
		fprintf(fff, "It strikes at demons with holy wrath.\n");
	}
	if (f5 & (TR5_KILL_UNDEAD))
	{
		fprintf(fff, "It is a great bane of undead.\n");
	}
	else if (f1 & (TR1_SLAY_UNDEAD))
	{
		fprintf(fff, "It strikes at undead with holy wrath.\n");
	}
	if (f1 & (TR1_SLAY_EVIL))
	{
		fprintf(fff, "It fights against evil with holy fury.\n");
	}
	if (f1 & (TR1_SLAY_ANIMAL))
	{
		fprintf(fff, "It is especially deadly against natural creatures.\n");
	}
	if (f1 & (TR1_MANA))
	{
		fprintf(fff, "It affects your mana capacity.\n");
	}
	if (f1 & (TR1_SPELL))
	{
		fprintf(fff, "It affects your spell power.\n");
	}

	if (f5 & (TR5_INVIS))
	{
		fprintf(fff, "It makes you invisible.\n");
	}
	if (f1 & (TR1_LIFE))
	{
		fprintf(fff, "It affects your hit points.\n");
	}
	if (o_ptr->tval != TV_TRAPKIT)
	{
		if (f2 & (TR2_SUST_STR))
		{
			fprintf(fff, "It sustains your strength.\n");
		}
		if (f2 & (TR2_SUST_INT))
		{
			fprintf(fff, "It sustains your intelligence.\n");
		}
		if (f2 & (TR2_SUST_WIS))
		{
			fprintf(fff, "It sustains your wisdom.\n");
		}
		if (f2 & (TR2_SUST_DEX))
		{
			fprintf(fff, "It sustains your dexterity.\n");
		}
		if (f2 & (TR2_SUST_CON))
		{
			fprintf(fff, "It sustains your constitution.\n");
		}
		if (f2 & (TR2_SUST_CHR))
		{
			fprintf(fff, "It sustains your charisma.\n");
		}
		if (f2 & (TR2_IM_ACID))
		{
			fprintf(fff, "It provides immunity to acid.\n");
		}
		if (f2 & (TR2_IM_ELEC))
		{
			fprintf(fff, "It provides immunity to electricity.\n");
		}
		if (f2 & (TR2_IM_FIRE))
		{
			fprintf(fff, "It provides immunity to fire.\n");
		}
	}
#if 1
	else
	{
		if (f2 & (TRAP2_AUTOMATIC_5))
		{
			fprintf(fff, "It can rearm itself.\n");
		}
		if (f2 & (TRAP2_AUTOMATIC_99))
		{
			fprintf(fff, "It rearms itself.\n");
		}               
		if (f2 & (TRAP2_KILL_GHOST))
		{
			fprintf(fff, "It is effective against Ghosts.\n");
		}
		if (f2 & (TRAP2_TELEPORT_TO))
		{
			fprintf(fff, "It can teleport monsters to you.\n");
		}
		if (f2 & (TRAP2_ONLY_DRAGON))
		{
			fprintf(fff, "It can only be set off by dragons.\n");
		}
		if (f2 & (TRAP2_ONLY_DEMON))
		{
			fprintf(fff, "It can only be set off by demons.\n");
		}
		if (f2 & (TRAP2_ONLY_UNDEAD))
		{
			fprintf(fff, "It can only be set off by undead.\n");
		}
		if (f2 & (TRAP2_ONLY_ANIMAL))
		{
			fprintf(fff, "It can only be set off by animals.\n");
		}
		if (f2 & (TRAP2_ONLY_EVIL))
		{
			fprintf(fff, "It can only be set off by evil creatures.\n");
		}
	}
#endif

	if (f2 & (TR2_IM_COLD))
	{
		fprintf(fff, "It provides immunity to cold.\n");
	}

	if (f2 & (TR2_FREE_ACT))
	{
		fprintf(fff, "It provides immunity to paralysis.\n");
	}
	if (f2 & (TR2_HOLD_LIFE))
	{
		fprintf(fff, "It provides resistance to life draining.\n");
	}
	if (f2 & (TR2_RES_FEAR))
	{
		fprintf(fff, "It makes you completely fearless.\n");
	}
	if (f2 & (TR2_RES_ACID))
	{
		fprintf(fff, "It provides resistance to acid.\n");
	}
	if (f2 & (TR2_RES_ELEC))
	{
		fprintf(fff, "It provides resistance to electricity.\n");
	}
	if (f2 & (TR2_RES_FIRE))
	{
		fprintf(fff, "It provides resistance to fire.\n");
	}
	if (f2 & (TR2_RES_COLD))
	{
		fprintf(fff, "It provides resistance to cold.\n");
	}
	if (f2 & (TR2_RES_POIS))
	{
		fprintf(fff, "It provides resistance to poison.\n");
	}

	if (f2 & (TR2_RES_LITE))
	{
		fprintf(fff, "It provides resistance to light.\n");
	}
	if (f2 & (TR2_RES_DARK))
	{
		fprintf(fff, "It provides resistance to dark.\n");
	}

	if (f2 & (TR2_RES_BLIND))
	{
		fprintf(fff, "It provides resistance to blindness.\n");
	}
	if (f2 & (TR2_RES_CONF))
	{
		fprintf(fff, "It provides resistance to confusion.\n");
	}
	if (f2 & (TR2_RES_SOUND))
	{
		fprintf(fff, "It provides resistance to sound.\n");
	}
	if (f2 & (TR2_RES_SHARDS))
	{
		fprintf(fff, "It provides resistance to shards.\n");
	}

	if (f4 & (TR4_IM_NETHER))
	{
		fprintf(fff, "It provides immunity to nether.\n");
	}
	if (f2 & (TR2_RES_NETHER))
	{
		fprintf(fff, "It provides resistance to nether.\n");
	}
	if (f2 & (TR2_RES_NEXUS))
	{
		fprintf(fff, "It provides resistance to nexus.\n");
	}
	if (f2 & (TR2_RES_CHAOS))
	{
		fprintf(fff, "It provides resistance to chaos.\n");
	}
	if (f2 & (TR2_RES_DISEN))
	{
		fprintf(fff, "It provides resistance to disenchantment.\n");
	}

	if (f3 & (TR3_WRAITH))
	{
		fprintf(fff, "It renders you incorporeal.\n");
	}
	if (f3 & (TR3_FEATHER))
	{
		fprintf(fff, "It allows you to levitate.\n");
	}
	if (f4 & (TR4_FLY))
	{
		fprintf(fff, "It allows you to fly.\n");
	}
	if (f5 & (TR5_PASS_WATER))
	{
		fprintf(fff, "It allows you to swim easily.\n");
	}
	if (f4 & (TR4_CLIMB))
	{
		fprintf(fff, "It allows you to climb high mountains.\n");
	}
	if ((o_ptr->tval != TV_LITE) && ((f3 & (TR3_LITE1)) || (f4 & (TR4_LITE2)) || (f4 & (TR4_LITE3))))
	{
		fprintf(fff, "It provides light.\n");
	}
	if (f3 & (TR3_SEE_INVIS))
	{
		fprintf(fff, "It allows you to see invisible monsters.\n");
	}
	if (esp)
	{
		if (esp & ESP_ALL) fprintf(fff, "It gives telepathic powers.\n");
		else
		{
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
	{
		fprintf(fff, "It slows your metabolism.\n");
	}
	if (f3 & (TR3_REGEN))
	{
		fprintf(fff, "It speeds your regenerative powers.\n");
	}
	if (f5 & (TR5_RES_TIME))
	{
		fprintf(fff, "It provides resistance to time.\n");
	}
	if (f5 & (TR5_RES_MANA))
	{
		fprintf(fff, "It provides resistance to magical energy.\n");
	}
	if (f5 & (TR5_IM_POISON))
	{
		fprintf(fff, "It provides immunity to poison.\n");
	}
	if (f5 & (TR5_IM_WATER))
	{
		fprintf(fff, "It provides complete protection from unleashed water.\n");
	}
	if (f5 & (TR5_RES_WATER))
	{
		fprintf(fff, "It provides resistance to unleashed water.\n");
	}
	if (f5 & (TR5_REGEN_MANA))
	{
		fprintf(fff, "It speeds your mana recharging.\n");
	}
	if (f5 & (TR5_REFLECT))
	{
		fprintf(fff, "It reflects bolts and arrows.\n");
	}
	if (f3 & (TR3_SH_FIRE))
	{
		fprintf(fff, "It produces a fiery sheath.\n");
	}
	if (f3 & (TR3_SH_ELEC))
	{
		fprintf(fff, "It produces an electric sheath.\n");
	}
	if (f3 & (TR3_NO_MAGIC))
	{
		fprintf(fff, "It produces an anti-magic shell.\n");
	}
	/* Mega Hack^3 -- describe the Anchor of Space-time */
	if (o_ptr->name1 == ART_ANCHOR)
	{
		fprintf(fff, "It prevents the space-time continuum from being disrupted.\n");
	}

	if (f3 & (TR3_NO_TELE))
	{
		fprintf(fff, "It prevents teleportation.\n");
	}
	if (f3 & (TR3_XTRA_MIGHT))
	{
		fprintf(fff, "It fires missiles with extra might.\n");
	}
	if (f3 & (TR3_XTRA_SHOTS))
	{
		fprintf(fff, "It fires missiles excessively fast.\n");
	}

	if (f5 & (TR5_DRAIN_MANA))
	{
		fprintf(fff, "It drains mana.\n");
	}

	if (f5 & (TR5_DRAIN_HP))
	{
		fprintf(fff, "It drains life.\n");
	}

	if (f3 & (TR3_DRAIN_EXP))
	{
		fprintf(fff, "It drains experience.\n");
	}
	if (f3 & (TR3_TELEPORT))
	{
		fprintf(fff, "It induces random teleportation.\n");
	}
	if (f3 & (TR3_AGGRAVATE))
	{
		fprintf(fff, "It aggravates nearby creatures.\n");
	}

	if (f3 & (TR3_BLESSED))
	{
		fprintf(fff, "It has been blessed by the gods.\n");
	}

	if (f4 & (TR4_AUTO_ID))
	{
		fprintf(fff, "It identifies all items for you.\n");
	}

	if (f4 & (TR4_NEVER_BLOW))
	{
		fprintf(fff, "It can't attack.\n");
	}

	if (f4 & (TR4_BLACK_BREATH))
	{
		fprintf(fff, "It fills you with the Black Breath.\n");
	}

	if (cursed_p(o_ptr))
	{
		if (f3 & (TR3_PERMA_CURSE))
		{
			fprintf(fff, "It is permanently cursed.\n");
		}
		else if (f3 & (TR3_HEAVY_CURSE))
		{
			fprintf(fff, "It is heavily cursed.\n");
		}
		else
		{
			fprintf(fff, "It is cursed.\n");
		}
	}

	if (f3 & (TR3_TY_CURSE))
	{
		fprintf(fff, "It carries an ancient foul curse.\n");
	}

	if (f4 & (TR4_DG_CURSE))
	{
		fprintf(fff, "It carries an ancient morgothian curse.\n");
	}
	if (f4 & (TR4_CLONE))
	{
		fprintf(fff, "It can clone monsters.\n");
	}
	if (f4 & (TR4_CURSE_NO_DROP))
	{
		fprintf(fff, "It cannot be dropped while cursed.\n");
	}
	if (f3 & (TR3_AUTO_CURSE))
	{
		fprintf(fff, "It can re-curse itself.\n");
	}
	if (f4 & (TR4_CAPACITY))
	{
		fprintf(fff, "It can hold more mana.\n");
	}
	if (f4 & (TR4_CHEAPNESS))
	{
		fprintf(fff, "It can cast spells for a lesser mana cost.\n");
	}
	if (f4 & (TR4_FAST_CAST))
	{
		fprintf(fff, "It can cast spells faster.\n");
	}
	if (f4 & (TR4_CHARGING))
	{
		fprintf(fff, "It regenerates its mana faster.\n");
	}

#if 1
	if (f5 & (TR5_NO_ENCHANT))
	{
		fprintf(fff, "It cannot be enchanted by any means.\n");
	}
#endif	// 0

	if ((f5 & (TR5_IGNORE_WATER)) && (f3 & (TR3_IGNORE_ACID)) && (f3 & (TR3_IGNORE_FIRE)) && (f3 & (TR3_IGNORE_COLD)) && (f3 & (TR3_IGNORE_ELEC)))
	{
		fprintf(fff, "It cannot be harmed by water, acid, cold, lightning or fire.\n");
	}
	else
	{
		if (f5 & (TR5_IGNORE_WATER))
		{
			fprintf(fff, "It cannot be harmed by water.\n");
		}
		if (f3 & (TR3_IGNORE_ACID))
		{
			fprintf(fff, "It cannot be harmed by acid.\n");
		}
		if (f3 & (TR3_IGNORE_ELEC))
		{
			fprintf(fff, "It cannot be harmed by electricity.\n");
		}
		if (f3 & (TR3_IGNORE_FIRE))
		{
			fprintf(fff, "It cannot be harmed by fire.\n");
		}
		if (f3 & (TR3_IGNORE_COLD))
		{
			fprintf(fff, "It cannot be harmed by cold.\n");
		}
	}

	/* Damage display for weapons */
	if (wield_slot(Ind, o_ptr) == INVEN_WIELD)
		display_weapon_damage(Ind, o_ptr, fff);

	/* Breakage/Damage display for ammo */
	if (wield_slot(Ind, o_ptr) == INVEN_AMMO)
	{
		fprintf(fff, "It has %d%% chances to break upon hit.\n"
				, breakage_chance(o_ptr));
		display_ammo_damage(Ind, o_ptr, fff);
	}


	//	info[i]=NULL;
	/* Close the file */
	my_fclose(fff);

	/* Hack -- anything written? (rewrite me) */

	/* Open a new file */
	fff = my_fopen(p_ptr->infofile, "r");

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
 * Describe a "fully identified" item
 */
#if 0
bool identify_fully_aux(int Ind, object_type *o_ptr)
{
	player_type *p_ptr = Players[Ind];
	int			i = 0;

	u32b f1, f2, f3;

	cptr		*info = p_ptr->info;


	/* Let the player scroll through this info */
	p_ptr->special_file_type = TRUE;

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3);


	/* Mega-Hack -- describe activation */
	if (f3 & TR3_ACTIVATE)
	{
		info[i++] = "It can be activated for...";
		info[i++] = item_activation(o_ptr);
		info[i++] = "...if it is being worn.";
	}


	/* Hack -- describe lite's */
	if (o_ptr->tval == TV_LITE)
	{
		if (artifact_p(o_ptr) || (o_ptr->sval == SV_LITE_FEANOR))
		{
			info[i++] = "It provides light (radius 3) forever.";
		}
		else if (o_ptr->sval == SV_LITE_DWARVEN)
		{
			info[i++] = "It provides light (radius 2) forever.";
		}
		else if (o_ptr->sval == SV_LITE_LANTERN)
		{
			info[i++] = "It provides light (radius 2) when fueled.";
		}
		else
		{
			info[i++] = "It provides light (radius 1) when fueled.";
		}
	}


	/* And then describe it fully */

	if (f1 & TR1_STR)
	{
		info[i++] = "It affects your strength.";
	}
	if (f1 & TR1_INT)
	{
		info[i++] = "It affects your intelligence.";
	}
	if (f1 & TR1_WIS)
	{
		info[i++] = "It affects your wisdom.";
	}
	if (f1 & TR1_DEX)
	{
		info[i++] = "It affects your dexterity.";
	}
	if (f1 & TR1_CON)
	{
		info[i++] = "It affects your constitution.";
	}
	if (f1 & TR1_CHR)
	{
		info[i++] = "It affects your charisma.";
	}
	
	if (f1 & TR1_MANA)
	{
		info[i++] = "It affects your mana capacity.";
	}
	
	if (f1 & TR1_SPELL_SPEED)
	{
		info[i++] = "It affects your spellcasting speed.";
	}

	if (f1 & TR1_STEALTH)
	{
		info[i++] = "It affects your stealth.";
	}
	if (f1 & TR1_SEARCH)
	{
		info[i++] = "It affects your searching.";
	}
	if (f1 & TR1_INFRA)
	{
		info[i++] = "It affects your infravision.";
	}
	if (f1 & TR1_TUNNEL)
	{
		info[i++] = "It affects your ability to tunnel.";
	}
	if (f1 & TR1_SPEED)
	{
		info[i++] = "It affects your speed.";
	}
	if (f1 & TR1_BLOWS)
	{
		info[i++] = "It affects your attack speed.";
	}

	if (f1 & TR1_BRAND_ACID)
	{
		info[i++] = "It does extra damage from acid.";
	}
	if (f1 & TR1_BRAND_ELEC)
	{
		info[i++] = "It does extra damage from electricity.";
	}
	if (f1 & TR1_BRAND_FIRE)
	{
		info[i++] = "It does extra damage from fire.";
	}
	if (f1 & TR1_BRAND_COLD)
	{
		info[i++] = "It does extra damage from frost.";
	}

	if (f1 & TR1_IMPACT)
	{
		info[i++] = "It can cause earthquakes.";
	}

	if (f1 & TR1_KILL_DRAGON)
	{
		info[i++] = "It is a great bane of dragons.";
	}
	else if (f1 & TR1_SLAY_DRAGON)
	{
		info[i++] = "It is especially deadly against dragons.";
	}
	if (f1 & TR1_SLAY_ORC)
	{
		info[i++] = "It is especially deadly against orcs.";
	}
	if (f1 & TR1_SLAY_TROLL)
	{
		info[i++] = "It is especially deadly against trolls.";
	}
	if (f1 & TR1_SLAY_GIANT)
	{
		info[i++] = "It is especially deadly against giants.";
	}
	if (f1 & TR1_SLAY_DEMON)
	{
		info[i++] = "It strikes at demons with holy wrath.";
	}
	if (f1 & TR1_SLAY_UNDEAD)
	{
		info[i++] = "It strikes at undead with holy wrath.";
	}
	if (f1 & TR1_SLAY_EVIL)
	{
		info[i++] = "It fights against evil with holy fury.";
	}
	if (f1 & TR1_SLAY_ANIMAL)
	{
		info[i++] = "It is especially deadly against natural creatures.";
	}

	if (f2 & TR2_ANTI_MAGIC)
	{
		info[i++] = "It creates an anti-magic shield.";
	}
	if (f2 & TR2_SUST_STR)
	{
		info[i++] = "It sustains your strength.";
	}
	if (f2 & TR2_SUST_INT)
	{
		info[i++] = "It sustains your intelligence.";
	}
	if (f2 & TR2_SUST_WIS)
	{
		info[i++] = "It sustains your wisdom.";
	}
	if (f2 & TR2_SUST_DEX)
	{
		info[i++] = "It sustains your dexterity.";
	}
	if (f2 & TR2_SUST_CON)
	{
		info[i++] = "It sustains your constitution.";
	}
	if (f2 & TR2_SUST_CHR)
	{
		info[i++] = "It sustains your charisma.";
	}

	if (f2 & TR2_IM_ACID)
	{
		info[i++] = "It provides immunity to acid.";
	}
	if (f2 & TR2_IM_ELEC)
	{
		info[i++] = "It provides immunity to electricity.";
	}
	if (f2 & TR2_IM_FIRE)
	{
		info[i++] = "It provides immunity to fire.";
	}
	if (f2 & TR2_IM_COLD)
	{
		info[i++] = "It provides immunity to cold.";
	}

	if (f2 & TR2_FREE_ACT)
	{
		info[i++] = "It provides immunity to paralysis.";
	}
	if (f2 & TR2_HOLD_LIFE)
	{
		info[i++] = "It provides resistance to life draining.";
	}

	if (f2 & TR2_RES_ACID)
	{
		info[i++] = "It provides resistance to acid.";
	}
	if (f2 & TR2_RES_ELEC)
	{
		info[i++] = "It provides resistance to electricity.";
	}
	if (f2 & TR2_RES_FIRE)
	{
		info[i++] = "It provides resistance to fire.";
	}
	if (f2 & TR2_RES_COLD)
	{
		info[i++] = "It provides resistance to cold.";
	}
	if (f2 & TR2_RES_POIS)
	{
		info[i++] = "It provides resistance to poison.";
	}

	if (f2 & TR2_RES_LITE)
	{
		info[i++] = "It provides resistance to light.";
	}
	if (f2 & TR2_RES_DARK)
	{
		info[i++] = "It provides resistance to dark.";
	}

	if (f2 & TR2_RES_BLIND)
	{
		info[i++] = "It provides resistance to blindness.";
	}
	if (f2 & TR2_RES_CONF)
	{
		info[i++] = "It provides resistance to confusion.";
	}
	if (f2 & TR2_RES_SOUND)
	{
		info[i++] = "It provides resistance to sound.";
	}
	if (f2 & TR2_RES_SHARDS)
	{
		info[i++] = "It provides resistance to shards.";
	}

	if (f2 & TR2_RES_NETHER)
	{
		info[i++] = "It provides resistance to nether.";
	}
	if (f2 & TR2_RES_NEXUS)
	{
		info[i++] = "It provides resistance to nexus.";
	}
	if (f2 & TR2_RES_CHAOS)
	{
		info[i++] = "It provides resistance to chaos.";
	}
	if (f2 & TR2_RES_DISEN)
	{
		info[i++] = "It provides resistance to disenchantment.";
	}

	if (f3 & TR3_FEATHER)
	{
		info[i++] = "It induces feather falling.";
	}
	if (f3 & TR3_LITE)
	{
		info[i++] = "It provides permanent light.";
	}
	if (f3 & TR3_SEE_INVIS)
	{
		info[i++] = "It allows you to see invisible monsters.";
	}
#if 0	// obsolete(DELETEME)
	if (f3 & TR3_TELEPATHY)
	{
		info[i++] = "It gives telepathic powers.";
	}
#endif	// 0
	if (f3 & TR3_SLOW_DIGEST)
	{
		info[i++] = "It slows your metabolism.";
	}
	if (f3 & TR3_REGEN)
	{
		info[i++] = "It speeds your regenerative powers.";
	}

	if (f3 & TR3_XTRA_MIGHT)
	{
		info[i++] = "It fires missiles with extra might.";
	}
	if (f3 & TR3_XTRA_SHOTS)
	{
		info[i++] = "It fires missiles excessively fast.";
	}

	if (f3 & TR3_DRAIN_EXP)
	{
		info[i++] = "It drains experience.";
	}
	if (f3 & TR3_TELEPORT)
	{
		info[i++] = "It induces random teleportation.";
	}
	if (f3 & TR3_AGGRAVATE)
	{
		info[i++] = "It aggravates nearby creatures.";
	}

	if (f3 & TR3_BLESSED)
	{
		info[i++] = "It has been blessed by the gods.";
	}

	if (f3 & TR3_KNOWLEDGE)
	{
		info[i++] = "It allows you to sense magic..";
	}

	if (cursed_p(o_ptr))
	{
		if (f3 & TR3_PERMA_CURSE)
		{
			info[i++] = "It is permanently cursed.";
		}
		else if (f3 & TR3_HEAVY_CURSE)
		{
			info[i++] = "It is heavily cursed.";
		}
		else
		{
			info[i++] = "It is cursed.";
		}
	}


	if (f5 & TR3_IGNORE_WATER)
	{
		info[i++] = "It cannot be harmed by water.";
	}
	if (f3 & TR3_IGNORE_ACID)
	{
		info[i++] = "It cannot be harmed by acid.";
	}
	if (f3 & TR3_IGNORE_ELEC)
	{
		info[i++] = "It cannot be harmed by electricity.";
	}
	if (f3 & TR3_IGNORE_FIRE)
	{
		info[i++] = "It cannot be harmed by fire.";
	}
	if (f3 & TR3_IGNORE_COLD)
	{
		info[i++] = "It cannot be harmed by cold.";
	}

	info[i]=NULL;

	/* No special effects */
	if (!i) return (FALSE);

	/* Let the client know it's about to get some info */
	Send_special_other(Ind);

	/* Gave knowledge */
	return (TRUE);
}
#endif	// 0

#if 0
/*
 * Describe a "fully identified" item
 */
//bool identify_fully_aux(object_type *o_ptr, FILE *fff)
bool identify_fully_aux(int Ind, object_type *o_ptr)
{
	player_type *p_ptr = Players[Ind];
	cptr		*info = p_ptr->info;

	int                     i = 0, j, k, am;

        u32b f1, f2, f3, f4, f5, esp;

//        cptr            info[400];
//        byte            color[400];

	/* Let the player scroll through this info */
	p_ptr->special_file_type = TRUE;


	/* Extract the flags */
        object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

#if 0
        /* All white */
        for (j = 0; j < 400; j++)
                color[j] = TERM_WHITE;

        /* No need to dump that */
        if (fff == NULL)
        {
                i = grab_tval_desc(o_ptr->tval, info, 0);
                if ((fff == NULL) && i) info[i++] = "";
                for (j = 0; j < i; j++)
                        color[j] = TERM_L_BLUE;
        }

        if (o_ptr->k_idx)
	{
	
                char buff2[400], *s, *t;
                int n, oi = i;
                object_kind *k_ptr = &k_info[o_ptr->k_idx];
			   
                strcpy (buff2, k_text + k_ptr->text);

		s = buff2;
		
                /* Collect the history */
                while (TRUE)
                {

                        /* Extract remaining length */
                        n = strlen(s);

                        /* All done */
                        if (n < 60)
                        {
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

	if (o_ptr->name1)
	{
	
                char buff2[400], *s, *t;
                int n, oi = i;
                artifact_type *a_ptr = &a_info[o_ptr->name1];
			   
		strcpy (buff2, a_text + a_ptr->text);

		s = buff2;
		
                /* Collect the history */
                while (TRUE)
                {

                        /* Extract remaining length */
                        n = strlen(s);

                        /* All done */
                        if (n < 60)
                        {
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

                if (a_ptr->set != -1)
                {
                        char buff2[400], *s, *t;
                        int n;
                        set_type *set_ptr = &set_info[a_ptr->set];

                        strcpy (buff2, set_text + set_ptr->desc);

                        s = buff2;

                        /* Collect the history */
                        while (TRUE)
                        {

                                /* Extract remaining length */
                                n = strlen(s);

                                /* All done */
                                if (n < 60)
                                {
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

        if (f4 & TR4_LEVELS)
        {
                int j = 0;

                color[i] = TERM_VIOLET;
                if (count_bits(o_ptr->pval3) == 0) info[i++] = "It is sentient.";
                else if (count_bits(o_ptr->pval3) > 1) info[i++] = "It is sentient and can have access to the realms of:";
                else  info[i++] = "It is sentient and can have access to the realm of:";

                for (j = 0; j < MAX_FLAG_GROUP; j++)
                {
                        if (BIT(j) & o_ptr->pval3)
                        {
                                color[i] = flags_groups[j].color;
                                info[i++] = flags_groups[j].name;
                        }
                }

                /* Add a blank line */
                info[i++] = "";
        }

        if (f4 & TR4_COULD2H) info[i++] = "It can be wielded two-handed.";
        if (f4 & TR4_MUST2H) info[i++] = "It must be wielded two-handed.";
#endif

	/* Mega Hack^3 -- describe the amulet of life saving */
	if (o_ptr->tval == TV_AMULET &&
		o_ptr->sval == SV_AMULET_LIFE_SAVING)
	{
		info[i++] = "It will save your life from perilous scene once.";
	}

	/* Mega-Hack -- describe activation */
	if (f3 & TR3_ACTIVATE)
	{
		info[i++] = "It can be activated for...";
		info[i++] = item_activation(o_ptr);
		info[i++] = "...if it is being worn.";
	}

#if 0
	/* Mega-Hack -- describe activation */
	if (f3 & (TR3_ACTIVATE))
	{
		info[i++] = "It can be activated for...";
                if (is_ego_p(o_ptr, EGO_MSTAFF_SPELL))
                {
                        info[i++] = item_activation(o_ptr, 1);
                        info[i++] = "And...";
                }
                info[i++] = item_activation(o_ptr, 0);

		/* Mega-hack -- get rid of useless line for randarts */
                if (o_ptr->tval != TV_RANDART){
                        info[i++] = "...if it is being worn.";
                }
	}

        /* Granted power */
        if (object_power(o_ptr) != -1)
        {
                info[i++] = "It grants you the power of...";
                info[i++] = powers_type[object_power(o_ptr)].name;
                info[i++] = "...if it is being worn.";
        }
#endif	// 0


	/* Hack -- describe lite's */
	if (o_ptr->tval == TV_LITE)
	{
                int radius = 0;

                if (f3 & TR3_LITE1) radius++;
                if (f4 & TR4_LITE2) radius += 2;
                if (f4 & TR4_LITE3) radius += 3;

                if (f4 & TR4_FUEL_LITE)
		{
                        info[i++] = format("It provides light (radius %d) when fueled.", radius);
		}
                else if (radius)
		{
                        info[i++] = format("It provides light (radius %d) forever.", radius);
		}
				else
		{
                        info[i++] = "It never provides light.";
		}
	}

        /* Mega Hack^3 -- describe the Anchor of Space-time */
        if (o_ptr->name1 == ART_ANCHOR)
        {
                info[i++] = "It prevents the space-time continuum from being disrupted.";
        }
#if 0
        if ((f4 & (TR4_ANTIMAGIC_50)) || (f4 & (TR4_ANTIMAGIC_30)) || (f4 & (TR4_ANTIMAGIC_20)) || (f4 & (TR4_ANTIMAGIC_10)))
        {
                info[i++] = "It generates an antimagic field.";
        }
#endif	// 0

		am = ((f4 & (TR4_ANTIMAGIC_50)) ? 50 : 0)
			+ ((f4 & (TR4_ANTIMAGIC_30)) ? 30 : 0)
			+ ((f4 & (TR4_ANTIMAGIC_20)) ? 20 : 0)
			+ ((f4 & (TR4_ANTIMAGIC_10)) ? 10 : 0);
		if (am) am += 0 - o_ptr->to_h - o_ptr->to_d - o_ptr->pval - o_ptr->to_a;

		if (am >= 100)
			info[i++] = "It generates a perfect antimagic field.";
		else if (am >= 80)
			info[i++] = "It generates a mighty antimagic field.";
		else if (am >= 60)
			info[i++] = "It generates a strong antimagic field.";
		else if (am >= 40)
			info[i++] = "It generates an antimagic field.";
		else if (am >= 20)
			info[i++] = "It generates a mellow antimagic field.";
		else if (am > 0) info[i++] = "It generates a feeble antimagic field.";

	/* And then describe it fully */

	if (f1 & (TR1_STR))
	{
		info[i++] = "It affects your strength.";
	}
	if (f1 & (TR1_INT))
	{
		info[i++] = "It affects your intelligence.";
	}
	if (f1 & (TR1_WIS))
	{
		info[i++] = "It affects your wisdom.";
	}
	if (f1 & (TR1_DEX))
	{
		info[i++] = "It affects your dexterity.";
	}
	if (f1 & (TR1_CON))
	{
		info[i++] = "It affects your constitution.";
	}
	if (f1 & (TR1_CHR))
	{
		info[i++] = "It affects your charisma.";
	}

	if (f1 & (TR1_STEALTH))
	{
			info[i++] = "It affects your stealth.";
#if 0
		if (o_ptr->tval != TV_TRAPKIT)
			info[i++] = "It affects your stealth.";
		else
			info[i++] = "It is well-hidden.";
#endif	// 0
	}
	if (f1 & (TR1_SEARCH))
	{
		info[i++] = "It affects your searching.";
	}
	if (f1 & (TR1_INFRA))
	{
		info[i++] = "It affects your infravision.";
	}
	if (f1 & (TR1_TUNNEL))
	{
		info[i++] = "It affects your ability to tunnel.";
	}
	if (f1 & (TR1_SPEED))
	{
		info[i++] = "It affects your speed.";
	}
	if (f1 & (TR1_BLOWS))
	{
		info[i++] = "It affects your attack speed.";
	}
        if (f5 & (TR5_CRIT))
	{
                info[i++] = "It affects your ability to score critical hits.";
	}
        if (f5 & (TR5_LUCK))
	{
                info[i++] = "It affects your luck.";
	}

	if (f1 & (TR1_BRAND_ACID))
	{
		info[i++] = "It does extra damage from acid.";
	}
	if (f1 & (TR1_BRAND_ELEC))
	{
		info[i++] = "It does extra damage from electricity.";
	}
	if (f1 & (TR1_BRAND_FIRE))
	{
		info[i++] = "It does extra damage from fire.";
	}
	if (f1 & (TR1_BRAND_COLD))
	{
		info[i++] = "It does extra damage from frost.";
	}

	if (f1 & (TR1_BRAND_POIS))
	{
		info[i++] = "It poisons your foes.";
	}

	if (f5 & (TR5_CHAOTIC))
	{
		info[i++] = "It produces chaotic effects.";
	}

	if (f1 & (TR1_VAMPIRIC))
	{
		info[i++] = "It drains life from your foes.";
	}

	if (f1 & (TR1_IMPACT))
	{
		info[i++] = "It can cause earthquakes.";
	}

	if (f1 & (TR1_VORPAL))
	{
		info[i++] = "It is very sharp and can cut your foes.";
	}

        if (f5 & (TR5_WOUNDING))
	{
                info[i++] = "It is very sharp and make your foes bleed.";
	}

	if (f1 & (TR1_KILL_DRAGON))
	{
		info[i++] = "It is a great bane of dragons.";
	}
	else if (f1 & (TR1_SLAY_DRAGON))
	{
		info[i++] = "It is especially deadly against dragons.";
	}
	if (f1 & (TR1_SLAY_ORC))
	{
		info[i++] = "It is especially deadly against orcs.";
	}
	if (f1 & (TR1_SLAY_TROLL))
	{
		info[i++] = "It is especially deadly against trolls.";
	}
	if (f1 & (TR1_SLAY_GIANT))
	{
		info[i++] = "It is especially deadly against giants.";
	}
        if (f5 & (TR5_KILL_DEMON))
	{
                info[i++] = "It is a great bane of demons.";
	}
        else if (f1 & (TR1_SLAY_DEMON))
	{
		info[i++] = "It strikes at demons with holy wrath.";
	}
        if (f5 & (TR5_KILL_UNDEAD))
	{
                info[i++] = "It is a great bane of undead.";
	}
        else if (f1 & (TR1_SLAY_UNDEAD))
	{
		info[i++] = "It strikes at undead with holy wrath.";
	}
	if (f1 & (TR1_SLAY_EVIL))
	{
		info[i++] = "It fights against evil with holy fury.";
	}
	if (f1 & (TR1_SLAY_ANIMAL))
	{
		info[i++] = "It is especially deadly against natural creatures.";
	}
        if (f1 & (TR1_MANA))
	{
                info[i++] = "It affects your mana capacity.";
	}
        if (f1 & (TR1_SPELL))
	{
                info[i++] = "It affects your spell power.";
	}

        if (f5 & (TR5_INVIS))
	{
                info[i++] = "It makes you invisible.";
	}
        if (f1 & (TR1_LIFE))
	{
                info[i++] = "It affects your hit points.";
	}
//        if (o_ptr->tval != TV_TRAPKIT)
//        {
                if (f2 & (TR2_SUST_STR))
                {
                        info[i++] = "It sustains your strength.";
                }
                if (f2 & (TR2_SUST_INT))
                {
                        info[i++] = "It sustains your intelligence.";
                }
                if (f2 & (TR2_SUST_WIS))
                {
                        info[i++] = "It sustains your wisdom.";
                }
                if (f2 & (TR2_SUST_DEX))
                {
                        info[i++] = "It sustains your dexterity.";
                }
                if (f2 & (TR2_SUST_CON))
                {
                        info[i++] = "It sustains your constitution.";
                }
                if (f2 & (TR2_SUST_CHR))
                {
                        info[i++] = "It sustains your charisma.";
                }
                if (f2 & (TR2_IM_ACID))
                {
                        info[i++] = "It provides immunity to acid.";
                }
                if (f2 & (TR2_IM_ELEC))
                {
                        info[i++] = "It provides immunity to electricity.";
                }
                if (f2 & (TR2_IM_FIRE))
                {
                        info[i++] = "It provides immunity to fire.";
                }
//        }
#if 0
		else
		{
			if (f2 & (TRAP2_AUTOMATIC_5))
			{
				info[i++] = "It can rearm itself.";
			}
			if (f2 & (TRAP2_AUTOMATIC_99))
			{
				info[i++] = "It rearms itself.";
			}               
			if (f2 & (TRAP2_KILL_GHOST))
			{
				info[i++] = "It is effective against Ghosts.";
			}
			if (f2 & (TRAP2_TELEPORT_TO))
			{
				info[i++] = "It can teleport monsters to you.";
			}
			if (f2 & (TRAP2_ONLY_DRAGON))
			{
				info[i++] = "It can only be set off by dragons.";
			}
			if (f2 & (TRAP2_ONLY_DEMON))
			{
				info[i++] = "It can only be set off by demons.";
			}
			if (f2 & (TRAP2_ONLY_UNDEAD))
			{
				info[i++] = "It can only be set off by undead.";
			}
			if (f2 & (TRAP2_ONLY_ANIMAL))
			{
				info[i++] = "It can only be set off by animals.";
			}
			if (f2 & (TRAP2_ONLY_EVIL))
			{
				info[i++] = "It can only be set off by evil creatures.";
			}
		}
#endif

	if (f2 & (TR2_IM_COLD))
	{
		info[i++] = "It provides immunity to cold.";
	}

	if (f2 & (TR2_FREE_ACT))
	{
		info[i++] = "It provides immunity to paralysis.";
	}
	if (f2 & (TR2_HOLD_LIFE))
	{
		info[i++] = "It provides resistance to life draining.";
	}
	if (f2 & (TR2_RES_FEAR))
	{
		info[i++] = "It makes you completely fearless.";
	}
	if (f2 & (TR2_RES_ACID))
	{
		info[i++] = "It provides resistance to acid.";
	}
	if (f2 & (TR2_RES_ELEC))
	{
		info[i++] = "It provides resistance to electricity.";
	}
	if (f2 & (TR2_RES_FIRE))
	{
		info[i++] = "It provides resistance to fire.";
	}
	if (f2 & (TR2_RES_COLD))
	{
		info[i++] = "It provides resistance to cold.";
	}
	if (f2 & (TR2_RES_POIS))
	{
		info[i++] = "It provides resistance to poison.";
	}

	if (f2 & (TR2_RES_LITE))
	{
		info[i++] = "It provides resistance to light.";
	}
	if (f2 & (TR2_RES_DARK))
	{
		info[i++] = "It provides resistance to dark.";
	}

	if (f2 & (TR2_RES_BLIND))
	{
		info[i++] = "It provides resistance to blindness.";
	}
	if (f2 & (TR2_RES_CONF))
	{
		info[i++] = "It provides resistance to confusion.";
	}
	if (f2 & (TR2_RES_SOUND))
	{
		info[i++] = "It provides resistance to sound.";
	}
	if (f2 & (TR2_RES_SHARDS))
	{
		info[i++] = "It provides resistance to shards.";
	}

        if (f4 & (TR4_IM_NETHER))
	{
                info[i++] = "It provides immunity to nether.";
	}
	if (f2 & (TR2_RES_NETHER))
	{
		info[i++] = "It provides resistance to nether.";
	}
	if (f2 & (TR2_RES_NEXUS))
	{
		info[i++] = "It provides resistance to nexus.";
	}
	if (f2 & (TR2_RES_CHAOS))
	{
		info[i++] = "It provides resistance to chaos.";
	}
	if (f2 & (TR2_RES_DISEN))
	{
		info[i++] = "It provides resistance to disenchantment.";
	}

	if (f3 & (TR3_WRAITH))
	{
		info[i++] = "It renders you incorporeal.";
	}
	if (f3 & (TR3_FEATHER))
	{
		info[i++] = "It allows you to levitate.";
	}
        if (f4 & (TR4_FLY))
	{
                info[i++] = "It allows you to fly.";
	}
	if (f5 & (TR5_PASS_WATER))
	{
		info[i++] = "It allows you to swim easily.";
	}
        if (f4 & (TR4_CLIMB))
	{
                info[i++] = "It allows you to climb high mountains.";
	}
        if ((o_ptr->tval != TV_LITE) && ((f3 & (TR3_LITE1)) || (f4 & (TR4_LITE2)) || (f4 & (TR4_LITE3))))
	{
                info[i++] = "It provides light.";
	}
	if (f3 & (TR3_SEE_INVIS))
	{
		info[i++] = "It allows you to see invisible monsters.";
	}
        if (esp)
	{
                if (esp & ESP_ALL) info[i++] = "It gives telepathic powers.";
                else
                {
                        if (esp & ESP_ORC) info[i++] = "It allows you to sense the presence of orcs.";
                        if (esp & ESP_TROLL) info[i++] = "It allows you to sense the presence of trolls.";
                        if (esp & ESP_DRAGON) info[i++] = "It allows you to sense the presence of dragons.";
                        if (esp & ESP_SPIDER) info[i++] = "It allows you to sense the presence of spiders.";
                        if (esp & ESP_GIANT) info[i++] = "It allows you to sense the presence of giants.";
                        if (esp & ESP_DEMON) info[i++] = "It allows you to sense the presence of demons.";
                        if (esp & ESP_UNDEAD) info[i++] = "It allows you to sense presence of undead.";
                        if (esp & ESP_EVIL) info[i++] = "It allows you to sense the presence of evil beings.";
                        if (esp & ESP_ANIMAL) info[i++] = "It allows you to sense the presence of animals.";
                        if (esp & ESP_DRAGONRIDER) info[i++] = "It allows you to sense the presence of dragonriders.";
                        if (esp & ESP_GOOD) info[i++] = "It allows you to sense the presence of good beings.";
                        if (esp & ESP_NONLIVING) info[i++] = "It allows you to sense the presence of non-living things.";
                        if (esp & ESP_UNIQUE) info[i++] = "It allows you to sense the presence of unique beings.";
                }
	}
	if (f3 & (TR3_SLOW_DIGEST))
	{
		info[i++] = "It slows your metabolism.";
	}
	if (f3 & (TR3_REGEN))
	{
		info[i++] = "It speeds your regenerative powers.";
	}
	if (f5 & (TR5_REFLECT))
	{
		info[i++] = "It reflects bolts and arrows.";
	}
	if (f3 & (TR3_SH_FIRE))
	{
		info[i++] = "It produces a fiery sheath.";
	}
	if (f3 & (TR3_SH_ELEC))
	{
		info[i++] = "It produces an electric sheath.";
	}
	if (f3 & (TR3_NO_MAGIC))
	{
		info[i++] = "It produces an anti-magic shell.";
	}
	if (f3 & (TR3_NO_TELE))
	{
		info[i++] = "It prevents teleportation.";
	}
	if (f3 & (TR3_XTRA_MIGHT))
	{
		info[i++] = "It fires missiles with extra might.";
	}
	if (f3 & (TR3_XTRA_SHOTS))
	{
		info[i++] = "It fires missiles excessively fast.";
	}

        if (f5 & (TR5_DRAIN_MANA))
	{
                info[i++] = "It drains mana.";
	}

        if (f5 & (TR5_DRAIN_HP))
	{
                info[i++] = "It drains life.";
	}

	if (f3 & (TR3_DRAIN_EXP))
	{
		info[i++] = "It drains experience.";
	}
	if (f3 & (TR3_TELEPORT))
	{
		info[i++] = "It induces random teleportation.";
	}
	if (f3 & (TR3_AGGRAVATE))
	{
		info[i++] = "It aggravates nearby creatures.";
	}

	if (f3 & (TR3_BLESSED))
	{
		info[i++] = "It has been blessed by the gods.";
	}

        if (f4 & (TR4_AUTO_ID))
	{
                info[i++] = "It identifies all items for you.";
	}

        if (f4 & (TR4_NEVER_BLOW))
	{
                info[i++] = "It can't attack.";
	}

        if (f4 & (TR4_BLACK_BREATH))
        {
                info[i++] = "It fills you with the Black Breath.";
        }

	if (cursed_p(o_ptr))
	{
		if (f3 & (TR3_PERMA_CURSE))
		{
			info[i++] = "It is permanently cursed.";
		}
		else if (f3 & (TR3_HEAVY_CURSE))
		{
			info[i++] = "It is heavily cursed.";
		}
		else
		{
			info[i++] = "It is cursed.";
		}
	}

	if (f3 & (TR3_TY_CURSE))
	{
		info[i++] = "It carries an ancient foul curse.";
	}

        if (f4 & (TR4_DG_CURSE))
	{
                info[i++] = "It carries an ancient morgothian curse.";
	}
        if (f4 & (TR4_CLONE))
	{
                info[i++] = "It can clone monsters.";
	}
        if (f4 & (TR4_CURSE_NO_DROP))
	{
                info[i++] = "It cannot be dropped while cursed.";
	}
        if (f3 & (TR3_AUTO_CURSE))
	{
                info[i++] = "It can re-curse itself.";
	}
        if (f4 & (TR4_CAPACITY))
	{
                info[i++] = "It can hold more mana.";
	}
        if (f4 & (TR4_CHEAPNESS))
	{
                info[i++] = "It can cast spells for a lesser mana cost.";
	}
        if (f4 & (TR4_FAST_CAST))
	{
                info[i++] = "It can cast spells faster.";
	}
        if (f4 & (TR4_CHARGING))
	{
                info[i++] = "It regenerates its mana faster.";
	}

        if ((f5 & (TR5_IGNORE_WATER)) && (f3 & (TR3_IGNORE_ACID)) && (f3 & (TR3_IGNORE_FIRE)) && (f3 & (TR3_IGNORE_COLD)) && (f3 & (TR3_IGNORE_ELEC)))
        {
                info[i++] = "It cannot be harmed by water, acid, cold, lightning or fire.";
        }
        else
        {
                if (f5 & (TR5_IGNORE_WATER))
                {
                        info[i++] = "It cannot be harmed by water.";
                }
                if (f3 & (TR3_IGNORE_ACID))
                {
                        info[i++] = "It cannot be harmed by acid.";
                }
                if (f3 & (TR3_IGNORE_ELEC))
                {
                        info[i++] = "It cannot be harmed by electricity.";
                }
                if (f3 & (TR3_IGNORE_FIRE))
                {
                        info[i++] = "It cannot be harmed by fire.";
                }
                if (f3 & (TR3_IGNORE_COLD))
                {
                        info[i++] = "It cannot be harmed by cold.";
                }
        }

	if(p_ptr->admin_dm){
		msg_format(Ind, "tval: %d sval: %d pval: %d bpval: %d\n", o_ptr->tval, o_ptr->sval, o_ptr->pval, o_ptr->bpval);
	}

	info[i]=NULL;

	/* No special effects */
	if (!i) return (FALSE);

	/* Let the client know it's about to get some info */
	Send_special_other(Ind);

	/* Gave knowledge */
	return (TRUE);
}
#endif	// 0




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

		case TV_HAFTED:
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
			if(p_ptr->inventory[INVEN_BOW].k_idx)
			{
				if(p_ptr->inventory[INVEN_BOW].sval < 10)
//					return get_slot(INVEN_AMMO);
					return (INVEN_AMMO);
			}
			return -1;
		}

		case TV_ARROW:
		{
			if(p_ptr->inventory[INVEN_BOW].k_idx)
			{
				if((p_ptr->inventory[INVEN_BOW].sval >= 10)&&(p_ptr->inventory[INVEN_BOW].sval < 20))
//					return get_slot(INVEN_AMMO);
					return (INVEN_AMMO);
			}
			return -1;
		}
		case TV_BOLT:
		{                        
			if(p_ptr->inventory[INVEN_BOW].k_idx)
			{
				if(p_ptr->inventory[INVEN_BOW].sval >= 20)
//					return get_slot(INVEN_AMMO);
				return (INVEN_AMMO);
			}
			return -1;
		}
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
		case INVEN_ARM:   p = "On arm"; break;
		case INVEN_HEAD:  p = "On head"; break;
		case INVEN_HANDS: p = "On hands"; break;
		case INVEN_FEET:  p = "On feet"; break;
		default:          p = "In pack"; break;
	}

	/* Hack -- Heavy weapon */
	if (i == INVEN_WIELD)
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
		case INVEN_ARM:   p = "wearing on your arm"; break;
		case INVEN_HEAD:  p = "wearing on your head"; break;
		case INVEN_HANDS: p = "wearing on your hands"; break;
		case INVEN_FEET:  p = "wearing on your feet"; break;
		default:          p = "carrying in your pack"; break;
	}

	/* Hack -- Heavy weapon */
	if (i == INVEN_WIELD)
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
	player_type *p_ptr = Players[Ind];

	register	int i, n, z = 0;

	object_type *o_ptr;

	byte	attr = TERM_WHITE;

	char	tmp_val[80];

	char	o_name[160];

	int wgt;


	/* Have the final slot be the FINAL slot */
	z = INVEN_WIELD;

	/* Display the pack */
	for (i = 0; i < z; i++)
	{
		/* Examine the item */
		o_ptr = &p_ptr->inventory[i];

		/* Start with an empty "index" */
		tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';

		/* Prepare an "index" */
		tmp_val[0] = index_to_label(i);

		/* Obtain an item description */
		object_desc(Ind, o_name, o_ptr, TRUE, 3);

		/* Obtain the length of the description */
		n = strlen(o_name);

		/* Get a color */
		if (can_use(Ind, o_ptr)) attr = tval_to_attr[o_ptr->tval % 128];
		else attr = TERM_L_DARK;

		/* Hack -- fake monochrome */
		if (!use_color) attr = TERM_WHITE;

		/* Display the weight if needed */
		wgt = o_ptr->weight * o_ptr->number;

		/* Send the info to the client */
		//Send_inven(Ind, tmp_val[0], attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->pval, o_name);
		Send_inven(Ind, tmp_val[0], attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, (o_ptr->tval == TV_BOOK) ? o_ptr->pval : 0, o_name);
	}
}



/*
 * Choice window "shadow" of the "show_equip()" function
 */
void display_equip(int Ind)
{
	player_type *p_ptr = Players[Ind];

	register	int i, n;
	object_type *o_ptr;
	byte	attr = TERM_WHITE;

	char	tmp_val[80];

	char	o_name[160];

	int wgt;

	/* Display the equipment */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	{
		/* Examine the item */
		o_ptr = &p_ptr->inventory[i];

		/* Start with an empty "index" */
		tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';

		/* Prepare an "index" */
		tmp_val[0] = index_to_label(i);

		/* Obtain an item description */
		object_desc(Ind, o_name, o_ptr, TRUE, 3);

		/* Obtain the length of the description */
		n = strlen(o_name);

		/* Get the color */
		attr = tval_to_attr[o_ptr->tval % 128];

		/* Hack -- fake monochrome */
		if (!use_color) attr = TERM_WHITE;

		/* Display the weight (if needed) */
//		wgt = o_ptr->weight * o_ptr->number;
		wgt = o_ptr->weight;

		/* Send the info off */
		//Send_equip(Ind, tmp_val[0], attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, o_ptr->pval, o_name);
		Send_equip(Ind, tmp_val[0], attr, wgt, o_ptr->number, o_ptr->tval, o_ptr->sval, (o_ptr->tval == TV_BOOK) ? o_ptr->pval : 0, o_name);
	}
}

bool can_use(int Ind, object_type *o_ptr)
{
	player_type *p_ptr = Players[Ind];

	/* Owner always can use */
	if (p_ptr->id == o_ptr->owner || p_ptr->admin_dm) return (TRUE);

	if (o_ptr->level < 1 && o_ptr->owner) return (FALSE);

	/* Hack -- convert if available */
	if (p_ptr->lev >= o_ptr->level && !p_ptr->admin_dm)
	{
		o_ptr->owner = p_ptr->id;
		return (TRUE);
	}
	else return (FALSE);
}
