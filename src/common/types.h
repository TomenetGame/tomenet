/* $Id$ */
/* File: types.h */

/* Purpose: global type declarations */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */


/*
 * This file should ONLY be included by "angband.h"
 */


/*
 * Note that "char" may or may not be signed, and that "signed char"
 * may or may not work on all machines.  So always use "s16b" or "s32b"
 * for signed values.  Also, note that unsigned values cause math problems
 * in many cases, so try to only use "u16b" and "u32b" for "bit flags",
 * unless you really need the extra bit of information, or you really
 * need to restrict yourself to a single byte for storage reasons.
 *
 * Also, if possible, attempt to restrict yourself to sub-fields of
 * known size (use "s16b" or "s32b" instead of "int", and "byte" instead
 * of "bool"), and attempt to align all fields along four-byte words, to
 * optimize storage issues on 32-bit machines.  Also, avoid "bit flags"
 * since these increase the code size and slow down execution.  When
 * you need to store bit flags, use one byte per flag, or, where space
 * is an issue, use a "byte" or "u16b" or "u32b", and add special code
 * to access the various bit flags.
 *
 * Many of these structures were developed to reduce the number of global
 * variables, facilitate structured program design, allow the use of ascii
 * template files, simplify access to indexed data, or facilitate efficient
 * clearing of many variables at once.
 *
 * Certain data is saved in multiple places for efficient access, currently,
 * this includes the tval/sval/weight fields in "object_type", various fields
 * in "header_type", and the "m_idx" and "o_idx" fields in "cave_type".  All
 * of these could be removed, but this would, in general, slow down the game
 * and increase the complexity of the code.
 */





/*
 * Template file header information (see "init.c").  16 bytes.
 *
 * Note that the sizes of many of the "arrays" are between 32768 and
 * 65535, and so we must use "unsigned" values to hold the "sizes" of
 * these arrays below.  Normally, I try to avoid using unsigned values,
 * since they can cause all sorts of bizarre problems, but I have no
 * choice here, at least, until the "race" array is split into "normal"
 * and "unique" monsters, which may or may not actually help.
 *
 * Note that, on some machines, for example, the Macintosh, the standard
 * "read()" and "write()" functions cannot handle more than 32767 bytes
 * at one time, so we need replacement functions, see "util.c" for details.
 *
 * Note that, on some machines, for example, the Macintosh, the standard
 * "malloc()" function cannot handle more than 32767 bytes at one time,
 * but we may assume that the "ralloc()" function can handle up to 65535
 * butes at one time.  We should not, however, assume that the "ralloc()"
 * function can handle more than 65536 bytes at a time, since this might
 * result in segmentation problems on certain older machines, and in fact,
 * we should not assume that it can handle exactly 65536 bytes at a time,
 * since the internal functions may use an unsigned short to specify size.
 *
 * In general, these problems occur only on machines (such as most personal
 * computers) which use 2 byte "int" values, and which use "int" for the
 * arguments to the relevent functions.
 */

typedef struct header header;
struct header {
	byte	v_major;		/* Version -- major */
	byte	v_minor;		/* Version -- minor */
	byte	v_patch;		/* Version -- patch */
	byte	v_extra;		/* Version -- extra */


	u32b	info_num;		/* Number of "info" records */

	u32b	info_len;		/* Size of each "info" record */


	u32b	head_size;		/* Size of the "header" in bytes */

	u32b	info_size;		/* Size of the "info" array in bytes */

	u32b	name_size;		/* Size of the "name" array in bytes */

	u32b	text_size;		/* Size of the "text" array in bytes */
};


/* worldpos - replaces depth/dun_depth ulong with x,y,z
 * coordinates of world positioning.
 * it may seem cumbersome, but better than having
 * extra variables in each struct. (its standard).
 */
typedef struct worldpos worldpos;
struct worldpos {
	s16b wx;	/* west to east */
	s16b wy;	/* south to north */
	s16b wz;	/* deep to sky */
};

/* worldspot adds exact x and y coordinates */
typedef struct worldspot worldspot;
struct worldspot {
	struct worldpos wpos;
	s16b x;
	s16b y;
};

/* cavespot consists of coordinates within a cave */
typedef struct cavespot cavespot;
struct cavespot {
	s16b x;
	s16b y;
};


/*
 * "Themed" objects.
 * Probability in percent for each class of objects to be dropped.
 * This could perhaps be an array - but that wouldn't be as clear.
 */
/* Borrowed from ToME	- Jir - */
typedef struct obj_theme obj_theme;
struct obj_theme {
	byte treasure;
	byte combat;
	byte magic;
	byte tools;
};


/*
 * Information about terrain "features"
 */

typedef struct feature_type feature_type;
struct feature_type {
	u16b name;			/* Name (offset) */
	u16b text;			/* Text (offset) */
#if 1
	u32b tunnel;            /* Text for tunneling */
	u32b block;             /* Text for blocking */

	u32b flags1;            /* First set of flags */
	u32b flags2;
#endif

	byte mimic;			/* Feature to mimic */

	byte extra;			/* Extra byte (unused) */

	s16b unused;		/* Extra bytes (unused) */

	/* NOTE: it's d_ and x_ in ToME */
	byte f_attr;		/* Object "attribute" */
	char f_char;		/* Object "symbol" */

	byte z_attr;		/* The desired attr for this feature */
	char z_char;		/* The desired char for this feature */

#if 1
	byte shimmer[7];        /* Shimmer colors */

	int d_dice[4];                  /* Number of dices */
	int d_side[4];                  /* Number of sides */
	int d_frequency[4];             /* Frequency of damage (1 is the minimum) */
	int d_type[4];                  /* Type of damage */
#endif
};


/*
 * Information about object "kinds", including player knowledge.
 *
 * Only "aware" and "tried" are saved in the savefile
 */

typedef struct object_kind object_kind;
struct object_kind {
	u16b name;		/* Name (offset) */
	u16b text;		/* Text (offset) */

	byte tval;		/* Object type */
	byte sval;		/* Object sub type */

	s16b pval;		/* Object extra info */

	s16b to_h;		/* Bonus to hit */
	s16b to_d;		/* Bonus to damage */
	s16b to_a;		/* Bonus to armor */

	s16b ac;		/* Base armor */

	byte dd, ds;		/* Damage dice/sides */

	s16b weight;		/* Weight */

	s32b cost;		/* Object "base cost" */

	u32b flags1;		/* Flags, set 1 */
	u32b flags2;		/* Flags, set 2 */
	u32b flags3;		/* Flags, set 3 */
	u32b flags4;		/* Flags, set 4 */
	u32b flags5;		/* Flags, set 5 */
	u32b flags6;		/* Flags, set 6 */


	byte locale[4];		/* Allocation level(s) */
	u16b chance[4];		/* Allocation chance(s) */

	byte level;		/* Level */
	byte extra;		/* Something */


	byte k_attr;		/* Standard object attribute */
	char k_char;		/* Standard object character */


	byte d_attr;		/* Default object attribute */
	char d_char;		/* Default object character */


	byte x_attr;		/* Desired object attribute */
	char x_char;		/* Desired object character */


	bool has_flavor;	/* This object has a flavor */

	bool easy_know;		/* This object is always known (if aware) */


/*	bool aware;	*/		/* The player is "aware" of the item's effects */

/*	bool tried;	*/		/* The player has "tried" one of the items */

	u32b esp;		/* ESP flags */
#if 0
	byte btval;		/* Become Object type */
	byte bsval;		/* Become Object sub type */

	s16b power;		/* Power granted(if any) */
#endif
};



/*
 * Information about "artifacts".
 *
 * Note that the save-file only writes "cur_num" to the savefile.
 *
 * Note that "max_num" is always "1" (if that artifact "exists")
 */

typedef struct artifact_type artifact_type;
struct artifact_type {
	u16b name;		/* Name (offset) */
	u16b text;		/* Text (offset) */

	byte tval;		/* Artifact type */
	byte sval;		/* Artifact sub type */

	s16b pval;		/* Artifact extra info */

	s16b to_h;		/* Bonus to hit */
	s16b to_d;		/* Bonus to damage */
	s16b to_a;		/* Bonus to armor */

	s16b ac;		/* Base armor */

	byte dd, ds;		/* Damage when hits */

	s16b weight;		/* Weight */

	s32b cost;		/* Artifact "cost" */

	u32b flags1;		/* Artifact Flags, set 1 */
	u32b flags2;		/* Artifact Flags, set 2 */
	u32b flags3;		/* Artifact Flags, set 3 */
        u32b flags4;            /* Artifact Flags, set 4 */
        u32b flags5;            /* Artifact Flags, set 5 */
        u32b flags6;            /* Artifact Flags, set 6 */

	byte level;		/* Artifact level */
	byte rarity;		/* Artifact rarity */

	byte cur_num;		/* Number created (0 or 1) */
	byte max_num;		/* Unused (should be "1") */
        u32b esp;               /* ESP flags */
#if 0

        s16b power;             /* Power granted(if any) */

        s16b set;               /* Does it belongs to a set ?*/
#endif	/* 0 */

	bool known;		/* Is this artifact already IDed? */

	s32b carrier;		/* Current holder (not necessarily same as o_ptr->owner), just to keep track */
	s32b timeout;		/* anti-hoarding artifact reset timer (-1 = permanent) */
	bool iddc;		/* for IDDC_ARTIFACT_FAST_TIMEOUT */
	bool winner;		/* for WINNER_ARTIFACT_FAST_TIMEOUT */
};


/*
 * Information about "ego-items".
 */

typedef struct ego_item_type ego_item_type;
struct ego_item_type {
	u16b name;			/* Name (offset) */
	u16b text;			/* Text (offset) */

        bool before;                    /* Before or after the object name ? */

        byte tval[MAX_EGO_BASETYPES];
        byte min_sval[MAX_EGO_BASETYPES];
        byte max_sval[MAX_EGO_BASETYPES];

	byte rating;		/* Rating boost */

	byte level;			/* Minimum level */
	byte rarity;		/* Object rarity */
        byte mrarity;           /* Object rarity */

	char max_to_h;		/* Maximum to-hit bonus */
	char max_to_d;		/* Maximum to-dam bonus */
	char max_to_a;		/* Maximum to-ac bonus */

	char max_pval;		/* Maximum pval */

	s32b cost;			/* Ego-item "cost" */

        byte rar[5];
        u32b flags1[5];            /* Ego-Item Flags, set 1 */
        u32b flags2[5];            /* Ego-Item Flags, set 2 */
        u32b flags3[5];            /* Ego-Item Flags, set 3 */
        u32b flags4[5];            /* Ego-Item Flags, set 4 */
        u32b flags5[5];            /* Ego-Item Flags, set 5 */
        u32b flags6[5];            /* Ego-Item Flags, set 6 */
        u32b esp[5];                       /* ESP flags */
        u32b fego1[5];                       /* ego flags */
        u32b fego2[5];                       /* ego flags */

#if 0
        s16b power;                     /* Power granted(if any) */
#endif
};




/*
 * Monster blow structure
 *
 *	- Method (RBM_*)
 *	- Effect (RBE_*)
 *	- Damage Dice
 *	- Damage Sides
 */

typedef struct monster_blow monster_blow;
struct monster_blow {
	byte method;
	byte effect;
	byte d_dice;
	byte d_side;
	byte org_d_dice;
	byte org_d_side;
};



/*
 * Monster "race" information, including racial memories
 *
 * Note that "d_attr" and "d_char" are used for MORE than "visual" stuff.
 *
 * Note that "x_attr" and "x_char" are used ONLY for "visual" stuff.
 *
 * Note that "cur_num" (and "max_num") represent the number of monsters
 * of the given race currently on (and allowed on) the current level.
 * This information yields the "dead" flag for Unique monsters.
 *
 * Note that "max_num" is reset when a new player is created.
 * Note that "cur_num" is reset when a new level is created.
 *
 * Note that several of these fields, related to "recall", can be
 * scrapped if space becomes an issue, resulting in less "complete"
 * monster recall (no knowledge of spells, etc).  All of the "recall"
 * fields have a special prefix to aid in searching for them.
 */


typedef struct monster_race monster_race;
struct monster_race {
	u16b name;				/* Name (offset) */
	u16b text;				/* Text (offset) */
	u16b dup_idx;				/* For mimicry: Race idx of duplicate that differs only in FRIENDS flag */

	u16b hdice;				/* Creatures hit dice count */
	u16b hside;				/* Creatures hit dice sides */

	s16b ac;				/* Armour Class */

	s16b sleep;				/* Inactive counter (base) */
	byte aaf;				/* Area affect radius (1-100) */
	byte speed;				/* Speed (normally 110) */

	s32b mexp;				/* Exp value for kill */

	s32b weight;		/* Weight of the monster */
	s16b extra;				/* Unused (for now) */

	byte freq_innate;		/* Innate spell frequency */
	byte freq_spell;		/* Other spell frequency */

	u32b flags1;			/* Flags 1 (general) */
	u32b flags2;			/* Flags 2 (abilities) */
	u32b flags3;			/* Flags 3 (race/resist) */
	u32b flags4;			/* Flags 4 (innate/breath) */
	u32b flags5;			/* Flags 5 (normal spells) */
	u32b flags6;			/* Flags 6 (special spells) */
#if 1
	u32b flags7;			/* Flags 7 (movement related abilities) */
	u32b flags8;			/* Flags 8 (wilderness info) */
	u32b flags9;			/* Flags 9 (drops info) */

	u32b flags0;			/* Flags 10 (extra spells) */
#endif

	monster_blow blow[4];	/* Up to four blows per round */

	byte body_parts[BODY_MAX];	/* To help to decide what to use when body changing */

	s16b level;			/* Level of creature */
	byte rarity;			/* Rarity of creature */


	byte d_attr;			/* Default monster attribute */
	char d_char;			/* Default monster character */


	byte x_attr;			/* Desired monster attribute */
	char x_char;			/* Desired monster character */


	s32b max_num;			/* Maximum population allowed per level */

	s32b cur_num;			/* Monster population on current level */

	s32b r_sights;			/* Count sightings of this monster */
	s32b r_deaths;			/* Count deaths from this monster */
	s32b r_tkills;			/* Count monsters killed by all players */

#ifdef OLD_MONSTER_LORE
	s16b r_pkills;			/* Count monsters killed in this life */

	byte r_wake;			/* Number of times woken up (?) */
	byte r_ignore;			/* Number of times ignored (?) */

	/*byte r_xtra1;			changed to time for japanese patch APD Something (unused)
	  byte r_xtra2;                    Something (unused) */

	byte r_drop_gold;		/* Max number of gold dropped at once */
	byte r_drop_item;		/* Max number of item dropped at once */

	byte r_cast_innate;		/* Max number of innate spells seen */
	byte r_cast_spell;		/* Max number of other spells seen */

	byte r_blows[4];		/* Number of times each blow type was seen */

	u32b r_flags1;			/* Observed racial flags */
	u32b r_flags2;			/* Observed racial flags */
	u32b r_flags3;			/* Observed racial flags */
	u32b r_flags4;			/* Observed racial flags */
	u32b r_flags5;			/* Observed racial flags */
	u32b r_flags6;			/* Observed racial flags */
#if 0
	u32b r_flags7;			/* Observed racial flags */
	u32b r_flags8;			/* Observed racial flags */
	u32b r_flags9;			/* Observed racial flags */

	u32b r_flags0;			/* Observed racial flags */
#endif
#endif

	obj_theme drops;		/* The drops type */

	int u_idx;			/* Counter for sorted unique positioning */

	int restrict_dun;		/* restrict to specific dungeon (used for non-FINAL_GUARDIAN monsters) */
};



/*
 * Information about "vault generation"
 */

typedef struct vault_type vault_type;
struct vault_type {
	u16b name;			/* Name (offset) */
	u32b text;			/* Text (offset) */

	byte typ;			/* Vault type */

	byte rat;			/* Vault rating */

	byte hgt;			/* Vault height */
	byte wid;			/* Vault width */

	u32b flags1;			/* VF1 flags */

#if 0
	s16b lvl;                       /* level of special (if any) */
	byte dun_type;                  /* Dungeon type where the level will show up */

	s16b mon[10];                   /* special monster */
	int item[3];                   /* number of item (usually artifact) */
#endif	/* 0 */
};

typedef struct swear_info {
	char word[NAME_LEN];
	int level;
} swear_info;

/* jk */
/* name and description are in some other arrays */
typedef struct trap_kind trap_kind;
struct trap_kind{
  s16b probability; /* probability of existence */
  s16b another;     /* does this trap easily combine */
  s16b p1valinc;     /* how much does this trap attribute to p1val */
  byte difficulty;  /* how difficult to disarm */
  byte minlevel;    /* what is the minimum level on which the traps should be */
  byte color;       /* what is the color on screen */
  byte vanish;       /* probability of disappearence */
  u32b flags;       /* where can these traps go - and perhaps other flags */
#if 0	/* Handled in player_type */
  bool ident;       /* do we know the name */
  s16b known;       /* how well is this trap known */
#endif
  s16b name;        /* normal name like weakness */
  s16b dd, ds;      /* base damage */
  s16b text;        /* longer description once you've met this trap */
};



/*
 * A single "grid" in a Cave
 *
 * Note that several aspects of the code restrict the actual cave
 * to a max size of 256 by 256.  In partcular, locations are often
 * saved as bytes, limiting each coordinate to the 0-255 range.
 *
 * The "o_idx" and "m_idx" fields are very interesting.  There are
 * many places in the code where we need quick access to the actual
 * monster or object(s) in a given cave grid.  The easiest way to
 * do this is to simply keep the index of the monster and object
 * (if any) with the grid, but takes a lot of memory.  Several other
 * methods come to mind, but they all seem rather complicated.
 *
 * Note the special fields for the simple "monster flow" code,
 * and for the "tracking" code.
 */
typedef struct c_special c_special;
struct c_special{
	unsigned char type;
	union	/* 32bits -> 64bits (rune) */
	{
		void *ptr;		/* lazy - refer to other arrays or sth */
		s32b omni;		/* needless of other arrays? k, add here! */
		struct { byte t_idx; bool found; } trap;
		struct { byte fy, fx; } between; /* or simply 'dpos'? */
		struct { byte wx, wy; s16b wz; } wpos;	/* XXX */
		struct { byte type, rest; bool known; } fountain;
		struct { u16b trap_kit; byte difficulty, feat; } montrap;
		struct { byte typ, mod, lev, feat; s32b id; s16b note; byte discount; s16b level; } rune; /* CS_RUNE */
	} sc;
	struct c_special *next;
};

typedef struct cave_type cave_type;

/* hooks structure containing calls for specials */
struct sfunc {
	void (*load)(c_special *cs_ptr);		/* load function */
	void (*save)(c_special *cs_ptr);		/* save function */
	void (*see)(c_special *cs_ptr, char *c, byte *a, int Ind);	/* sets player view */
	int (*activate)(c_special *cs_ptr, int y, int x, int Ind);	/* walk on/bump */
};

struct cave_type {
	u32b info;		/* Hack -- cave flags */
	byte feat;		/* Hack -- feature type */
	byte feat_org;		/* Feature type backup (todo: for wall-created grids to revert to original feat when tunneled!) */
	s16b o_idx;		/* Item index (in o_list) or zero */
	s16b m_idx;		/* Monster index (in m_list) or zero */
				/* or negative if a player */

#ifdef MONSTER_FLOW		/* Note: Currently only flow_by_sound is implemented, not flow_by_smell - C. Blue */
	byte cost;		/* Hack -- cost of flowing */
	byte when;		/* Hack -- when cost was computed */
#endif
#ifdef MONSTER_FLOW_BY_SMELL	/* Added this for reduced stamp radius around the player, representing his "scent" surrounding him - C. Blue */
	byte cost_smell;	/* Hack -- cost of flowing */
	byte when_smell;	/* Hack -- when cost was computed */
#endif

	struct c_special *special;	/* Special pointer to various struct */

	/* I don't really love to enlarge cave_type ... but it'd suck if
	 * trapped floor will be immune to Noxious Cloud */
	/* Adding 1byte in this struct costs 520Kb memory, in average */
	/* This should be replaced by 'stackable c_special' code -
	 * let's wait for evileye to to this :)		- Jir - */
	int effect, effect_xtra;	/* The lasting effects */

#ifdef HOUSE_PAINTING
	byte colour;	/* colour that overrides the usual colour of a feature */
#endif
};

/* ToME parts, arranged */
/* This struct can be enlarged to handle more generic timed events maybe? */
/* Lasting spell effects(clouds, ..) */
typedef struct effect_type effect_type;
struct effect_type {
	s16b	interval;	/* How quickly does it tick (10 = normal, once per 10 frames at 0 ft depth) */
	s16b    time;           /* For how long */
	s16b    dam;            /* How much damage */
	u32b    type;           /* Of which type */ /* GF_XXX, for now */
	s16b	sy;		/* Start of the cast (beam shapes) */
	s16b	sx;		/* Start of the cast (beam shapes) */
	s16b    cy;             /* Center of the cast */
	s16b    cx;             /* Center of the cast */
	s16b    rad;            /* Radius */
	u32b    flags;          /* Flags */

	s32b	who;		/* Who caused this effect (0-id if player) */
	worldpos wpos;		/* Where in the world */
};

/*
 * Structure for an object. (32 bytes)
 *
 * Note that a "discount" on an item is permanent and never goes away.
 *
 * Note that inscriptions are now handled via the "quark_str()" function
 * applied to the "note" field, which will return NULL if "note" is zero.
 *
 * Note that "object" records are "copied" on a fairly regular basis.
 *
 * Note that "object flags" must now be derived from the object kind,
 * the artifact and ego-item indexes, and the two "xtra" fields.
 *
 * NOTE: Keep this structure packed tightly since 32768 of these are allocated
 * Try to keep fields sorted by size of the data type
 */

typedef struct object_type object_type;
struct object_type {
	s32b owner;			/* Player that found it */
	s16b level;			/* Level req */

	s16b k_idx;			/* Kind index (zero if "dead") */
	s16b h_idx;			/* inside house? (-1 if not) */

	struct worldpos wpos;		/* worldmap position (6 x s16b) */
	byte iy;			/* Y-position on map, or zero */
	byte ix;			/* X-position on map, or zero */

	byte tval;			/* Item type (from kind) */
	byte sval;			/* Item sub-type (from kind) */
	byte tval2;			/* normally unused (except for item-invalid-seal) */
	byte sval2;			/* normally unused (except for item-invalid-seal) */

	s32b bpval;			/* Base item extra-parameter */
	s32b pval;			/* Extra enchantment item extra-parameter (name1 or name2) */
#if 1 /* existing but currently not in use */
	s32b pval2;			/* Item extra-parameter for some special items */
	s32b pval3;			/* Item extra-parameter for some special items */
#endif

	/* VAMPIRES_INV_CURSED */
	s32b pval_org, bpval_org;
	s16b to_h_org, to_d_org, to_a_org;

	/* Used for temporarily augmented equipment. (Runecraft) */
	s32b sigil;			/* Element index (+1) for r_projection (common/tables.c) boni lookup. Zero if no sigil. */
	s32b sseed;			/* RNG Seed used to determine the boni (if random). Zero if not randomized. */

	byte discount;			/* Discount (if any) */
	byte number;			/* Number of items */
	s16b weight;			/* Item weight */

	u16b name1;			/* Artifact type, if any */
	u16b name2;			/* Ego-Item type, if any */
	u16b name2b;			/* 2e Ego-Item type, if any */
	s32b name3;			/* Randart seed, if any (now it's common with ego-items -Jir-) */
	u16b name4;			/* Index of randart name in file 'randarts.txt', solely for fun set bonus - C. Blue */
	byte attr;			/* colour in inventory (for client) */

	byte mode;			/* Mode of player who found it */

	s16b xtra1;			/* Extra info type, for various purpose */
	s16b xtra2;			/* Extra info index */
	/* more info added for self-made spellbook feature Adam suggested - C. Blue */
	s16b xtra3;			/* Extra info */
	s16b xtra4;			/* Extra info */
	s16b xtra5;			/* Extra info */
	s16b xtra6;			/* Extra info */
	s16b xtra7;			/* Extra info */
	s16b xtra8;			/* Extra info */
	s16b xtra9;			/* Extra info */

	char uses_dir;			/* Client-side: Uses a direction or not? (for rods) */

#ifdef PLAYER_STORES
	byte ps_idx_x;			/* Index or x-coordinate of player store item in the original house */
	byte ps_idx_y;			/* y-coordinate of player store item in the original house */
	s64b appraised_value;		/* HOME_APPRAISAL: object_value(Ind_seller, o_ptr); */
#endif

	s16b to_h;			/* Plusses to hit */
	s16b to_d;			/* Plusses to damage */
	s16b to_a;			/* Plusses to AC */

	s16b ac;			/* Normal AC */
	byte dd, ds;			/* Damage dice/sides */

	u16b ident;			/* Special flags  */
	s32b timeout;			/* Timeout Counter: amount of fuel left until it is depleted. */
	s32b timeout_magic;		/* Timeout Counter: amount of power left until it is depleted, can be discharged. */
	s32b recharging;		/* Auto-recharge-state of auto-recharging items (rods and activatable items). */

	s32b marked;			/* Object is marked (for deletion after a certain time) */
	byte marked2;			/* additional parameters */
	/* for new quest_info: */
	bool questor;			/* further quest_info flags are referred to when required, no need to copy all of them here */
	s16b quest, quest_stage, questor_idx;	/* It's an item for a quest (either the questor item or an item that needs to be retrieved for a quest goal).
		//IMPORTAAAAAAANT:	   Hack: 0 = no quest; n = quest + 1. So we don't have to initialise all items to -1 here :-p */
	byte questor_invincible;	/* invincible to players/monsters? */
	bool quest_credited;		/* ugly hack for inven_carry() usage within carry(), to avoid double-crediting */

	u16b note;			/* Inscription index */
	char note_utag;			/* Added for making pseudo-id overwrite unique loot tags */

#if 0	/* from pernA.. consumes memory, but quick. shall we? */
	u16b art_name;			/* Artifact name (random artifacts) */

	u32b art_flags1;		/* Flags, set 1  Alas, these were necessary */
	u32b art_flags2;		/* Flags, set 2  for the random artifacts of*/
	u32b art_flags3;		/* Flags, set 3  Zangband */
	u32b art_flags4;		/* Flags, set 4  PernAngband */
	u32b art_flags5;		/* Flags, set 5  PernAngband */
	u32b art_esp;			/* Flags, set esp  PernAngband */
#endif	/* 0 */

	byte inven_order;		/* Inventory position if held by a player,
					   only use is in xtra2.c when pack is ang_sort'ed */

	u16b next_o_idx;		/* Next object in stack (if any) */
	u16b held_m_idx;		/* Monster holding us (if any) */
	bool auto_insc;			/* Request client-side auto-inscription after item has changed? */
	char stack_pos;			/* Position in stack: Use to limit stack size */

	s16b cheeze_dlv, cheeze_plv, cheeze_plv_carry;	/* anti-cheeze */

	u16b housed;			/* <house index + 1> or 0 for not currently inside a house */
	bool changed;			/* dummy flag to refresh item if o_name changed, but memory copy didn't */
	bool NR_tradable;		/* for ALLOW_NR_CROSS_ITEMS */
	byte temp;			/* any local hacks */
	/* For IDDC_IRON_COOP || IRON_IRON_TEAM : */
	s32b iron_trade;		/* Needed for the last survivor after a party was erased: Former party of the last player who picked it up */
};

/*
 * NPC type information - LUA programmable
 * Basic structure for experimental use only
 * More data will need to be added for the
 * real thing.
 */

struct npc_type{
	byte active;			/* ignore this? */
	char name[20];			/* NPC name */
	s16b fy, fx;			/* Position */
	struct worldpos wpos;

	s32b exp;                       /* Experience of the monster */
	s16b level;                     /* Level of the monster */

	s16b energy;		/* Monster "energy" */

	byte stunned;		/* Monster is stunned */
	byte confused;		/* Monster is confused */
	byte monfear;		/* Monster is afraid */
};

/*
 * Monster information, for a specific monster.
 *
 * NOTE: fy, fx constrain dungeon size to 256x256
 *
 * NOTE: Keep this structure packed tightly since 32768 of these are allocated
 * Try to keep fields sorted by size of the data type
 */

typedef struct monster_type monster_type;
struct monster_type {
	monster_race *r_ptr;		/* Used for special monsters and questors */
	bool special;			/* Does it use a special r_info ? */
	byte pet;			/* Special pet value (not an ID). 0 = not a pet. 1 = is a pet. */

	s16b r_idx;			/* Monster race index */

	s32b owner;			/* ID of the player owning it (if it is a pet) */

	byte fy;			/* Y location on map */
	byte fx;			/* X location on map */

	struct worldpos wpos;		/* (6 x s16b) */

	s32b exp;			/* Experience of the monster */
	s16b level;			/* Level of the monster */

	monster_blow blow[4];		/* Up to four blows per round (6 x byte) */
	byte speed;			/* ORIGINAL Monster "speed" (gets copied from r_ptr->speed on monster placement) */
	byte mspeed;			/* CURRENT Monster "speed" (is set to 'speed' initially on monster placement) */
	s16b ac;			/* Armour Class */
	s16b org_ac;			/* Armour Class */

	s32b hp;			/* Current Hit points */
	s32b maxhp;			/* Max Hit points */
	s32b org_maxhp;			/* Max Hit points */

	s16b csleep;			/* Inactive counter */

	u16b hold_o_idx;		/* Object being held (if any) */

	s16b energy;			/* Monster "energy" */
	byte no_move;			/* special effect GF_STOP */

	byte monfear;			/* Monster is afraid */
	byte monfear_gone;		/* Monster is no longer afraid because it has no other options or is temporarily immune */
	byte confused;			/* Monster is confused */
	byte stunned;			/* Monster is stunned */
	byte paralyzed;			/* Monster is paralyzed (unused) */
	byte bleeding;			/* Monster is bleeding (unused) */
	byte poisoned;			/* Monster is poisoned (unused) */
	byte blinded;			/* monster appears confused (unused: wrapped as confusion currently) */
	byte silenced;			/* monster can't cast spells for a short time (for new mindcrafters) */
	s32b charmedignore;		/* monster is charmed in a way that it ignores players */

	s16b cdis;			/* Current dis from player */

	// bool los;			/* Monster is "in sight" */
	// bool ml;			/* Monster is "visible" */

	s16b closest_player;		/* The player closest to this monster */

#ifdef WDT_TRACK_OPTIONS

	byte ty;			/* Y location of target */
	byte tx;			/* X location of target */
	byte t_dur;			/* How long are we tracking */
	byte t_bit;			/* Up to eight bit flags */

#endif

#ifdef DRS_SMART_OPTIONS

	u32b smart;			/* Field for "smart_learn" */

#endif

	u16b clone;			/* clone value */
	u16b clone_summoning;		/* counter to keep track of summoning */

	s16b mind;			/* Current action -- golems */

#ifdef RANDUNIS
	u16b ego;			/* Ego monster type */
	s32b name3;			/* Randuni seed, if any */
#endif

	s16b status;			/* Status(friendly, pet, companion, ..) */
	s16b target;			/* Monster target */
	s16b possessor;			/* Is it under the control of a possessor ? */
	s16b destx, desty;		/* Monster target grid to walk to. Added for questors (quest_info). */
	s16b determination;		/* unused, maybe useful in the future for determining what it takes to stop the monster from doing something */
	s16b limit_hp;			/* for questors - revert hostility when <= this (makes lookup easier than referring through lots of pointers..) */

	u16b ai_state;			/* What special behaviour this monster takes now? */
	s16b last_target;		/* For C. Blue's anti-cheeze C_BLUE_AI in melee2.c */
	s16b last_target_melee;		/* For C. Blue's C_BLUE_AI_MELEE in melee2.c */
	s16b last_target_melee_temp;	/* For C. Blue's C_BLUE_AI_MELEE in melee2.c */
	s16b switch_target;		/* For distract_monsters(), implemented within C_BLUE_AI_MELEE in melee2.c */

	s16b cdis_on_damage;		/* New Ball spell / explosion anti-cheeze */
	// byte turns_tolerance;	/* Optional: How many turns pass until we react the new way */
	s16b damage_tx, damage_ty;	/* new temporary target position: where we received damage from */
	s16b damage_dis;		/* Remember distance to epicenter */
	s16b p_tx, p_ty;		/* Coordinates from where the player cast the damaging projection */
	signed char previous_direction;	/* Optional: Don't move right back where we came from (at least during this turn -_-) after reaching the damage epicentrum. */

	byte backstabbed;		/* has this monster been backstabbed from cloaking mode already? prevent exploit */

	s16b henc, henc_top;		/* 'highest_encounter' - my final anti-cheeze strike I hope ;) - C. Blue
		This keeps track of the highest player which the monster
		has 'encountered' (might offer various definitions of this
		by different #defines) and adjusts its own experience value
		towards that player, so low players who get powerful help
		will get less exp out of it. */
	byte taunted;			/* has this monster been taunted (melee technique)? */
#ifdef COMBO_AM_IC_CAP
	byte intercepted;		/* remember best interception cap of adjacent players to determine reduction of subsequent antimagic field chances to avoid excessive suppression */
#endif

	s32b extra;			/* extra flag for debugging/testing purpose; also used for target dummy's "snowiness" now; new: also for Sauron boosting */

#ifdef MONSTER_ASTAR
	s32b astar_idx;			/* index in available A* arrays. A* is expensive, so we only provide a couple of instances for a few monsters to use */
#endif

	/* Prevent a monster getting hit by cumulative projections caused recursively in project()
		(except for intended effects such as runecraft sub-explosions). */
	s32b hit_proj_id;

	u16b do_dist;			/* execute all monster teleportation at the end of turn */

#if 0 /* currently solved by bidirectional LoS testing via DOUBLE_LOS_SAFETY instead! */
	byte xlos_x[5], xlos_y[5];	/* Prevent system immanent LoS-exploit when monster gets targetted diagonally */
	/* note: affects near_hit, process_monsters, make_attack_spell, summon_possible, clean_shot..., projectable..., los... */
#endif

	/* for new quest_info */
	s16b quest, questor_idx;
	bool questor;
	byte questor_invincible;	/* further quest_info flags are referred to when required, no need to copy all of them here */
	byte questor_hostile;		/* hostility flags (0x1 = vs py, 0x2 = vs mon) */
	byte questor_target;		/* can get targetted by monsters and stuff..? */

	bool no_esp_phase;		/* for WEIRD_MIND esp flickering */
};

typedef struct monster_ego monster_ego;
struct monster_ego {
	u32b name;				/* Name (offset) */
	bool before;                            /* Display ego before or after */

	monster_blow blow[4];                   /* Up to four blows per round */
	byte blowm[4][2];

	s16b hdice;                             /* Creatures hit dice count */
	s16b hside;                             /* Creatures hit dice sides */

	s16b ac;				/* Armour Class */

	s16b sleep;				/* Inactive counter (base) */
	s16b aaf;                               /* Area affect radius (1-100) */
	s16b speed;                             /* Speed (normally 110) */

	s32b mexp;				/* Exp value for kill */

	s32b weight;                            /* Weight of the monster */

	byte freq_innate;               /* Innate spell frequency */
	byte freq_spell;		/* Other spell frequency */

	/* Ego flags */
	u32b flags1;                    /* Flags 1 */
	u32b flags2;                    /* Flags 1 */
	u32b flags3;                    /* Flags 1 */
	u32b flags7;                    /* Flags 1 */
	u32b flags8;                    /* Flags 1 */
	u32b flags9;                    /* Flags 1 */
	u32b flags0;                    /* Flags 1 */
	u32b hflags1;                    /* Flags 1 */
	u32b hflags2;                    /* Flags 1 */
	u32b hflags3;                    /* Flags 1 */
	u32b hflags7;                    /* Flags 1 */
	u32b hflags8;                    /* Flags 1 */
	u32b hflags9;                    /* Flags 1 */
	u32b hflags0;                    /* Flags 1 */

	/* Monster flags */
	u32b mflags1;                    /* Flags 1 (general) */
	u32b mflags2;                    /* Flags 2 (abilities) */
	u32b mflags3;                    /* Flags 3 (race/resist) */
	u32b mflags4;                    /* Flags 4 (innate/breath) */
	u32b mflags5;                    /* Flags 5 (normal spells) */
	u32b mflags6;                    /* Flags 6 (special spells) */
	u32b mflags7;                    /* Flags 7 (movement related abilities) */
	u32b mflags8;                    /* Flags 8 (wilderness info) */
	u32b mflags9;                    /* Flags 9 (drops info) */
	u32b mflags0;                    /* Flags 10 (extra spells) */

	/* Negative Flags, to be removed from the monster flags */
	u32b nflags1;                    /* Flags 1 (general) */
	u32b nflags2;                    /* Flags 2 (abilities) */
	u32b nflags3;                    /* Flags 3 (race/resist) */
	u32b nflags4;                    /* Flags 4 (innate/breath) */
	u32b nflags5;                    /* Flags 5 (normal spells) */
	u32b nflags6;                    /* Flags 6 (special spells) */
	u32b nflags7;                    /* Flags 7 (movement related abilities) */
	u32b nflags8;                    /* Flags 8 (wilderness info) */
	u32b nflags9;                    /* Flags 9 (drops info) */
	u32b nflags0;                    /* Flags 10 (extra spells) */

	s16b level;                     /* Level of creature */
	s16b rarity;                    /* Rarity of creature */


	byte d_attr;			/* Default monster attribute */
	char d_char;			/* Default monster character */

	char r_char[10];                 /* Monster race allowed */
	char nr_char[10];                /* Monster race not allowed */
};




/*
 * An entry for the object/monster allocation functions
 *
 * Pass 1 is determined from allocation information
 * Pass 2 is determined from allocation restriction
 * Pass 3 is determined from allocation calculation
 */

typedef struct alloc_entry alloc_entry;
struct alloc_entry {
	s16b index;		/* The actual index */

	s16b level;		/* Base dungeon level */
	s16b prob1;		/* Probability, pass 1 */
	s16b prob2;		/* Probability, pass 2 */
	s16b prob3;		/* Probability, pass 3 */
};


/*
 * The setup data that the server transmits to the
 * client.
 */
/*
 * Very sorry, this struct doesn't contain all the data sent during setup.
 * Please see Init_setup for details.		- Jir -
 */
typedef struct setup_t setup_t;
struct setup_t {
	s16b frames_per_second;
	byte max_race;
	byte max_class;
	byte max_trait;
	int motd_len;
	int setup_size;
	/* char motd[80 * 23]; */
	char motd[120 * 23];
};

/*
 * The setup data that the client transmits to the
 * server.
 */
typedef struct client_setup_t client_setup_t;
struct client_setup_t {
	bool options[OPT_MAX];

	s16b screen_wid;
	s16b screen_hgt;

	byte u_attr[TV_MAX];
	char u_char[TV_MAX];

	byte f_attr[MAX_F_IDX];
	char f_char[MAX_F_IDX];

	byte k_attr[MAX_K_IDX];
	char k_char[MAX_K_IDX];

	byte r_attr[MAX_R_IDX];
	char r_char[MAX_R_IDX];
};


/*
 * Available "options"
 *
 *	- Address of actual option variable (or NULL)
 *
 *	- Normal Value (TRUE or FALSE)
 *
 *	- Option Page Number (or zero)
 *
 *	- Savefile Set (or zero)
 *	- Savefile Bit in that set
 *
 *	- Textual name (or NULL)
 *	- Textual description
 */

typedef struct option_type option_type;
struct option_type {
	bool	*o_var;

	byte	o_norm;

	byte	o_page;

	byte	o_set;
	byte	o_bit;
	byte	o_enabled;

	cptr	o_text;
	cptr	o_desc;
};

/*
 * A store, with an owner, various state flags, a current stock
 * of items, and a table of items that are often purchased.
 */
typedef struct store_type store_type;
struct store_type {
	u16b st_idx;

	u16b owner;                     /* Owner index */

#ifdef PLAYER_STORES
	u32b player_owner;		/* Temporary value for player's id */
	byte player_owner_type;		/* Is it really a player or maybe a guild? */
#endif

	s16b insult_cur;		/* Insult counter */

	s16b good_buy;			/* Number of "good" buys */
	s16b bad_buy;			/* Number of "bad" buys */

	s32b store_open;		/* Closed until this turn */

	s32b last_visit;		/* Last visited on this turn */

	byte stock_num;			/* Stock -- Number of entries */
	s16b stock_size;		/* Stock -- Total Size of Array */
	object_type *stock;		/* Stock -- Actual stock items */
	
	s16b town;			/* residence town of this store. Just added for debugging purposes - C. Blue */
	
	s16b tim_watch;			/* store owner watching out for thieves? */
	s32b last_theft;		/* Turn of the last occurred theft that was noticed by the owner */
};

/*
 * Structure for the "quests"
 *
 * Hack -- currently, only the "level" parameter is set, with the
 * semantics that "one (QUEST) monster of that level" must be killed,
 * and then the "level" is reset to zero, meaning "all done".  Later,
 * we should allow quests like "kill 100 fire hounds", and note that
 * the "quest level" is then the level past which progress is forbidden
 * until the quest is complete.  Note that the "QUESTOR" flag then could
 * become a more general "never out of depth" flag for monsters.
 *
 * Actually, in Angband 2.8.0 it will probably prove easier to restrict
 * the concept of quest monsters to specific unique monsters, and to
 * actually scan the dead unique list to see what quests are left.
 */

typedef struct xorder xorder; /* UNUSED. The new 'xorder_type' is used instead. */
struct xorder {
	int level;		/* Dungeon level */
	int r_idx;		/* Monster race */

	int cur_num;		/* Number killed (unused) */
	int max_num;		/* Number required (unused) */
};

/* Quests, random or preset by the dungeon master */
/* evileye - same as old quest type, but multiplayerized. */
struct xorder_type {
	u16b active;		/* quest is active? (num players) */
	u16b id;		/* quest id */
	s16b type;		/* Monster race or object type */
	u16b flags;		/* Quest flags */
	s32b creator;		/* Player ID or 0L (DM, guildmaster only) */
	s32b turn;		/* quest started */
};

/* Adding this structure so we can have different creatures generated
   in different types of wilderness... this will probably be completly
   redone when I do a proper landscape generator.
   -APD-
*/

/*
 * struct for individual levels.
 */
typedef struct dun_level dun_level;
struct dun_level {
	int ondepth;
	time_t lastused;
	time_t creationtime;
	time_t total_static_time;
	u32b id;		/* Unique ID to check if players logged out on the same
				   floor or not, when they log in again- C. Blue */
	byte up_x,up_y;
	byte dn_x,dn_y;
	byte rn_x,rn_y;

	u32b flags1;		/* LF1 flags */
	u32b flags2;		/* LF2 flags */
	byte hgt;		/* Vault height */
	byte wid;		/* Vault width */
/*	char feeling[80] */	/* feeling description */
	char *uniques_killed;

	cave_type **cave;	/* Leave this the last entry (for aesthetic reason) */

	int fake_town_num;	/* for dungeon stores: which town we abuse the stores from */

	/* for obtaining statistical IDDC information: */
	int monsters_generated, monsters_spawned, monsters_killed;
};

/* dungeon_type structure
 *
 * Filter for races is not strict. It shall alter the probability.
 * (consider using rule_type	- Jir -)
 */
typedef struct dungeon_type dungeon_type;
struct dungeon_type {
	u16b id;		/* dungeon id */
	u16b type;		/* dungeon type (of d_info) */
	u16b baselevel;		/* base level (1 - 50ft etc). */
	u32b flags1;		/* dungeon flags */
	u32b flags2;		/* DF2 flags */
	u32b flags3;		/* DF3 flags */
	byte maxdepth;		/* max height/depth */
#if 0
	rule_type rules[5];	/* Monster generation rules */
	char r_char[10];	/* races allowed */
	char nr_char[10];	/* races prevented */
#endif	/* 0 */
	int store_timer;	/* control frequency of dungeon store generation (for misc iron stores mostly) */
	byte theme;		/* inspired by IDDC themes - for 'wilderness' dungeons */
	s16b quest, quest_stage;/* this dungeon was spawned by a quest? (for quest_info) quest==0 = no quest (it's q_idx + 1!) */
#ifdef GLOBAL_DUNGEON_KNOWLEDGE
	byte known;		/* optional: Bits: 0x1 seen, 0x2 mindepth, 0x4 maxdepth, 0x8 boss seen */
#endif

	struct dun_level *level;	/* array of dungeon levels */

};

/*
 * TODO:
 * - allow towns to have dungeon flags(DFn_*)
 */
struct town_type {
	u16b x,y;		/* town wilderness location */
	u16b baselevel;		/* Normally 0 for the basic town */
	u16b flags;		/* town flags */
	u16b num_stores;	/* always 8 or unused atm. */
	store_type *townstore;  /* pointer to the stores */
	u16b type;		/* town type (0=vanilla, 1=bree etc) */

	u16b terraformed_trees; /* keep track of and limit players modifying town layout */
	u16b terraformed_walls; /* keep track of and limit players modifying town layout */
	u16b terraformed_water; /* keep track of and limit players modifying town layout */
	u16b terraformed_glyphs; /* keep track of and limit players modifying town layout */

	u32b dlev_id; /* for dungeon towns, abusing fake stores from real towns */
	u16b dlev_depth; /* know the depth of this dungeon town, for determining store items */
};

typedef struct wilderness_type wilderness_type;
struct wilderness_type {
	u16b radius;	/* the distance from the town */
	u16b type;	/* what kind of terrain we are in */
	u16b town_lev;	/* difficulty level of the town that 'radius' refers to */
	signed char town_idx;	/* Which town resides exactly in this sector? */

	u32b flags;	/* various */
	struct dungeon_type *tower;
	struct dungeon_type *dungeon;
	s16b ondepth;
	time_t lastused;
	time_t total_static_time;
	cave_type **cave;
	byte up_x, up_y;
	byte dn_x, dn_y;
	byte rn_x, rn_y;
	s32b own;	/* King owning the wild */

	/* client-side worldmap-sector-specific weather:
	   (possible ideas for future: transmit x,y,wid,hgt weather frame
	   for current level too instead of always using full size gen.) */
	int weather_type, weather_wind, weather_wind_vertical, weather_intensity, weather_speed;
	bool weather_updated; /* notice any change in local weather (like a PR_ flag would do) */
	int clouds_to_update; /* number of clouds that were changed since last update (for efficiency) */
	bool cloud_updated[10]; /* 'has cloud been changed?' */
	int cloud_x1[10], cloud_y1[10], cloud_x2[10], cloud_y2[10], cloud_dsum[10], cloud_xm100[10], cloud_ym100[10], cloud_idx[10];

	u16b bled;	/* type that was bled into this sector (USE_SOUND_2010: ambient sfx) */
	bool ambient_sfx, ambient_sfx_counteddown, ambient_sfx_dummy; /* for synchronizing ambient sfx (USE_SOUND_2010) */
	int ambient_sfx_timer;
};


/*
 * A store owner
 */

typedef struct owner_type owner_type;
struct owner_type {
	u32b name;                      /* Name (offset) */

	s32b max_cost;                  /* Purse limit */

	byte max_inflate;               /* Inflation (max) */
	byte min_inflate;               /* Inflation (min) */

	byte haggle_per;                /* Haggle unit */

	byte insult_max;                /* Insult limit */

	s32b races[2][2];                  /* Liked/hated races */
	s32b classes[2][2];                /* Liked/hated classes */
	s32b realms[2][2];	/* Liked/hated realms */ /* unused */

	s16b costs[3];                  /* Costs for liked people */
};

/*
 * A store/building type
 */
/* I'd prefer 'store_kind'.. but just let's not change it */
typedef struct store_info_type store_info_type;
struct store_info_type {
	u32b name;                      /* Name (offset) */

	s16b table[STORE_CHOICES][2];   /* Table -- Legal item kinds */
	byte table_num;                 /* Number of items */
	s16b max_obj;                   /* Number of items this store can hold */

	u16b owners[6];                 /* List of owners(refers to ow_info) */

	u16b actions[6];                /* Actions(refers to ba_info) */

	byte d_attr;			/* Default building attribute */
	char d_char;			/* Default building character */

	byte x_attr;			/* Desired building attribute */
	char x_char;			/* Desired building character */

	u32b flags1;                    /* Flags */
};

/*
 * Stores/buildings actions
 */
typedef struct store_action_type store_action_type;
struct store_action_type {
	u32b name;                      /* Name (offset) */

	int costs[3];			/* Costs for hated/neutral/liked people */
	char letter;                    /* Action letter */
	s16b action;                    /* Action code */
	s16b action_restr;              /* Action restriction */
	byte flags;		/* Client flags */
};


/*
 * The "name" of spell 'N' is stored as spell_names[X][N],
 * where X is 0 for mage-spells and 1 for priest-spells.
 */

typedef struct magic_type magic_type;
struct magic_type {
	byte slevel;		/* Required level (to learn) */
	byte smana;		/* Required mana (to cast) */
	byte sfail;		/* Minimum chance of failure */
	byte sexp;		/* Encoded experience bonus */
	byte ftk;		/* Fire-till-kill class (0 = not possible, 1 = needs LOS, 2 = does't need LOS) */
};


/*
 * Information about the player's "magic"
 *
 * Note that a player with a "spell_book" of "zero" is illiterate.
 */

typedef struct player_magic player_magic;
struct player_magic {
	s16b spell_book;		/* Tval of spell books (if any) */
	s16b spell_stat;		/* Stat for spells (if any)  */
        magic_type info[64];	/* The available spells */
};



/*
 * Player racial info
 */

typedef struct player_race player_race;
struct player_race {
	cptr title;		/* Type of race */

	s16b r_adj[6];		/* Racial stat boni */

	s16b r_dis;		/* disarming */
	s16b r_dev;		/* magic devices */
	s16b r_sav;		/* saving throw */
	s16b r_stl;		/* stealth */
	s16b r_srh;		/* search ability */
	s16b r_fos;		/* search frequency */
	s16b r_thn;		/* combat (normal) */
	s16b r_thb;		/* combat (shooting) */

	byte r_mhp;		/* Race hit-dice modifier */
	s16b r_exp;		/* Race experience factor */

	byte b_age;		/* base age */
	byte m_age;		/* mod age */

	byte m_b_ht;		/* base height (males) */
	byte m_m_ht;		/* mod height (males) */
	byte m_b_wt;		/* base weight (males) */
	byte m_m_wt;		/* mod weight (males) */

	byte f_b_ht;		/* base height (females) */
	byte f_m_ht;		/* mod height (females)	  */
	byte f_b_wt;		/* base weight (females) */
	byte f_m_wt;		/* mod weight (females) */

	byte infra;		/* Infra-vision	range */

        s32b choice;            /* Legal class choices depending on race */

        s16b mana;              /* % mana */

        struct {
                s16b skill;

                char vmod;
                s32b value;

                char mmod;
                s16b mod;
        } skills[MAX_SKILLS];
};


/*
 * Player class info
 */

typedef struct player_class player_class;
struct player_class {
	cptr title;			/* Type of class */
	byte color;                     /* @ color */
	bool hidden;			/* Class isn't displayed in the 'Choose class' screen? */
	byte base_class;		/* Used if 'hidden': From which base class does this class result? */

	s16b c_adj[6];			/* Class stat modifier */
	s16b min_recommend[6];		/* Recommended minimum stat just for informing the user */

	s16b c_dis;			/* class disarming */
	s16b c_dev;			/* class magic devices */
	s16b c_sav;			/* class saving throws */
	s16b c_stl;			/* class stealth */
	s16b c_srh;			/* class searching ability */
	s16b c_fos;			/* class searching frequency */
	s16b c_thn;			/* class to hit (normal) */
	s16b c_thb;			/* class to hit (bows) */

	s16b x_dis;			/* extra disarming */
	s16b x_dev;			/* extra magic devices */
	s16b x_sav;			/* extra saving throws */
	s16b x_stl;			/* extra stealth */
	s16b x_srh;			/* extra searching ability */
	s16b x_fos;			/* extra searching frequency */
	s16b x_thn;			/* extra to hit (normal) */
	s16b x_thb;			/* extra to hit (bows) */

	s16b c_mhp;			/* Class hit-dice adjustment */
	s16b c_exp;			/* Class experience factor */

	struct {
		s16b skill;

		char vmod;
		s32b value;

		char mmod;
		s16b mod;
	} skills[MAX_SKILLS];
};


/*
 * Player trait info, originally added for Draconians - C. Blue
 */
typedef struct player_trait player_trait;
struct player_trait {
	cptr title;	/* Name of trait */
	s32b choice;	/* Legal trait choices, depending on race */
};


/* The information needed to show a single "grid" */
typedef struct cave_view_type cave_view_type;
struct cave_view_type {
	byte a;		/* Color attribute */
	char c;		/* ASCII character */
};

/*
 * Information about a "party"
 */
typedef struct party_type {
	char name[MAX_CHARS];	/* Name of the party */
	char owner[NAME_LEN];	/* Owner's name */
	s32b members;		/* Number of people in the party */
	s32b created;		/* Creation (or disband-tion) time */
	byte cmode;		/* Party creator's character mode */
	byte mode;		/* 'Iron Team' or normal party? (C. Blue) */
	s32b experience;	/* For 'Iron Teams': Max experienc of members. */
	u32b flags;		/* Party rules flags */
	/* For IDDC_IRON_COOP || IRON_IRON_TEAM : */
	s32b iron_trade;
} party_type;

/*
 * Information about a guild.
 */

/*
 * Guilds are semi permanent parties which allow party membership
 * at the same time as being a guild member. Experience is never
 * shared by guild members (unless in a party too). The guildmaster
 * has building privileges within the guild hall, and may alter
 * the layout of the hall at his/her discretion. Should the guild
 * master die, the guild is not disbanded, but he may drop a guild
 * key (if pkill is set) which will pass on ownership. A non member
 * picking this up, or the loss of the key (unstat etc.) will result
 * in a disputed guild where there is no master. In this case, the
 * position will be decided by some form of contest set by the
 * dungeon master. Should *all* guild members die, or commit suicide,
 * the guild will be disbanded, and the hall will be cleared and sold
 * to the bank in the same way houses are. (evileye)
 */
typedef struct guild_type {
	char name[MAX_CHARS];
	s32b master;		/* Guildmaster unique player ID */
	s32b members;		/* Number of guild members */
	byte cmode;		/* Guild creator's character mode */
	u32b flags;		/* Guild rules flags */
	s16b minlev;		/* minimum level to join */
	char adder[5][NAME_LEN];	/* Guild may have up to 5 people who can add besides the guild master */
	s16b h_idx;		/* Guild Hall - house index */
	u32b dna;		/* Remember the guild's identity - in case it times out and a new guild gets created of the same index */
	int timeout;		/* Timer for removal of a guild that has been leaderless for too long */
} guild_type;

/* Save data work information for guild halls */

struct guildsave{
	FILE *fp;	/* the passed file pointer */
	bool mode;	/* load=0 save=1 */
};


/*
 * Information about a "house"
 */
typedef struct house_type house_type;

/*
In order to delete the contents of a house after its key is lost,
added x_1, y_1, x_2, y_2, which are the locations of the opposite 
corners of the house.
-APD-
*/
struct house_type {
	byte x, y;		/* Absolute starting coordinates */
	byte dx, dy;		/* door coords */
	struct dna_type *dna;	/* house dna door information */
	u16b flags;		/* house flags - HF_xxxx */
	struct worldpos wpos;
	union {
		struct { byte width, height; } rect;
		char *poly;	/* coordinate array for non rect houses */
	} coords;

#ifndef USE_MANG_HOUSE_ONLY
	s16b stock_num;		/* Stock -- Number of entries */
	s16b stock_size;	/* Stock -- Total Size of Array */
	object_type *stock;	/* Stock -- Actual stock items */
#endif	/* USE_MANG_HOUSE_ONLY */

	byte colour;		/* house colour for custom house painting (HOUSE_PAINTING) */
	byte xtra;		/* unused; maybe for player stores if required */
};

struct dna_type{
	u32b creator;		/* Unique ID of creator/house admin */
	byte mode;		/* Creator's p_ptr->mode (normal, everlasting, pvp..) */
	s32b owner;		/* Player/Party/Class/Race ID */
	byte owner_type;	/* OT_xxxx */
	byte a_flags;		/* Combination of ACF_xxxx */
	u16b min_level;		/* minimum level - no higher than admin level */
	u32b price;		/* Speed before memory */
};

/* evileye - work in progress */
struct key_type{
	u16b id;		/* key pval */	
};

struct floor_insc{
	char text[MAX_CHARS];	/* that should be enough */
	u16b found;		/* we may want hidden inscription? */
};


#if 0
/* Traditional, store-like house */
struct trad_house_type {
	struct dna_type *dna;	/* house dna door information */
	s16b stock_num;			/* Stock -- Number of entries */
	s16b stock_size;		/* Stock -- Total Size of Array */
	object_type *stock;		/* Stock -- Actual stock items */
};
#endif	/* 0 */


#if 0
/*
 * Information about a "hostility"
 */
typedef struct hostile_type hostile_type;

struct hostile_type {
	s32b id;		/* ID of player we are hostile to */
	hostile_type *next;	/* Next in list */
};
#else
/*
 * More general linked list for player id numbers
 */
#define hostile_type player_list_type
typedef struct player_list_type player_list_type;

struct player_list_type {
	s32b id;		/* ID of player */
	player_list_type *next;
};
#endif

/* remotely ignore players */
struct remote_ignore {
	unsigned int id;		/* player unique id */
	short serverid;
	struct remote_ignore *next;	/* Next in list */
};

#if 0 /* not finished - mikaelh */
/*
 * ESP link list
 */
typedef struct esp_link_type esp_link_type;
struct esp_link_type {
	s32b id;	/* player ID */
	byte type;
	u16b flags;
	u16b end;
	esp_link_type *next;
};
#endif

/* The Troll Pit */
/* Temporary banning of certain addresses */
#if 0
struct ip_ban {
	struct ip_ban *next;	/* next ip in the list */
	char ip[20];	/* so it shouldn't be really */
	int time;	/* Time in minutes, or zero is permanent */
};
#else
struct combo_ban {
	struct combo_ban *next;	/* next ip in the list */
	char ip[20];
	char acc[NAME_LEN];
	char hostname[MAX_CHARS];
	char reason[MAX_CHARS];
	int time;	/* Time in minutes, or zero is permanent */
};
#endif

/*
 * Skills !
 */
typedef struct skill_type skill_type;
struct skill_type {
	uintptr name;                              /* Name */
	uintptr desc;                              /* Description */
	uintptr action_desc;                       /* Action Description */

	s16b action_mkey;                       /* Action do to */

	s16b rate;                              /* Modifier decreasing rate */

	s16b action[MAX_SKILLS];             /* List of actions against other skills in th form: action[x] = {SKILL_FOO, 10} */

	s16b father;                            /* Father in the skill tree */
	s16b order;                             /* Order in the tree */

	u32b flags1;                            /* Skill flags */
	byte tval;	/* tval associated */
};

/*
 * Skills of each player
 */
typedef struct skill_player skill_player;
struct skill_player {
	s32b base_value;                         /* Base value */
	s32b value;                             /* Actual value */
	u16b mod;                               /* Modifier(1 skill point = modifier skill) */
	bool dev;                               /* Is the branch developped ? */
	bool touched;				/* need refresh? */
	u32b flags1;                            /* Skill flags */
};


//todo, instead of ACC_GREETED, ACC_WARN_.. etc, maybe:	a dedicated 'u32b warnings;	/* account flags for received (one-time) hints/warnings */'
struct account {
	u32b id;	/* account id */
	u32b flags;	/* account flags */
	char name[ACCFILE_NAME_LEN];	/* login */
	char name_normalised[ACCFILE_NAME_LEN];	/* login name, but in a simplified form, used for preventing creation of too similar account names */
	char pass[ACCFILE_PASSWD_LEN];	/* some crypts are not 13 */
#ifdef ACC32
	int acc_laston, acc_laston_real;
#else
	time_t acc_laston, acc_laston_real;	/* last time this account logged on (for expiry check) */
#endif
	s32b cheeze;	/* value in gold of cheezed goods or money */
	s32b cheeze_self; /* value in gold of cheezed goods or money to own characters */
	char deed_event;	/* receive a deed for a global event participation? */
	char deed_achievement;	/* receive a deed for a (currently PvP) achievement? */
	s32b guild_id;	/* auto-rejoin its guild after a char perma-died */
	u32b guild_dna;	/* auto-rejoin its guild after a char perma-died */

	char houses; /* for account-wide house limit (installed after increasing the # of generic character slots above 8) */
};
/* Used for updating tomenet.acc structure: */
struct account_old {
	u32b id;	/* account id */
	u32b flags;	/* account flags */
	char name[ACCFILE_NAME_LEN];	/* login */
	char name_normalised[ACCFILE_NAME_LEN];	/* login name, but in a simplified form, used for preventing creation of too similar account names */
	char pass[ACCFILE_PASSWD_LEN];	/* some crypts are not 13 */
#ifdef ACC32
	int acc_laston, acc_laston_real;
#else
	time_t acc_laston, acc_laston_real;	/* last time this account logged on (for expiry check) */
#endif
	s32b cheeze;	/* value in gold of cheezed goods or money */
	s32b cheeze_self; /* value in gold of cheezed goods or money to own characters */
	char deed_event;	/* receive a deed for a global event participation? */
	char deed_achievement;	/* receive a deed for a (currently PvP) achievement? */
	s32b guild_id;	/* auto-rejoin its guild after a char perma-died */
	u32b guild_dna;	/* auto-rejoin its guild after a char perma-died */
};

typedef struct version_type version_type;
struct version_type {		/* Extended version structure */
	int major;
	int minor;
	int patch;
	int extra;
	int branch;
	int build;

	int os; /* after 4.4.8.1.0.0 */
};

typedef struct inventory_change_type inventory_change_type;

/*
 * Structure for keeping track of inventory changes
 */
struct inventory_change_type {
	char type;
	int revision;
	s16b begin;
	s16b end;
	s16b mod;
	inventory_change_type *next;
};

/*
 * Most of the "player" information goes here.
 *
 * This stucture gives us a large collection of player variables.
 *
 * This structure contains several "blocks" of information.
 *   (1) the "permanent" info
 *   (2) the "variable" info
 *   (3) the "transient" info
 *
 * All of the "permanent" info, and most of the "variable" info,
 * is saved in the savefile.  The "transient" info is recomputed
 * whenever anything important changes.
 */

/*
 * high time to economize memory by bandling bool arrays into
 * char one or something, like in wild_map[MAX_WILD_8] ?	- Jir -
 */

typedef struct player_type player_type;
struct player_type {
	int conn;			/* Connection number */
	int Ind;			/* Self-reference */
	char name[MAX_CHARS];		/* Nickname */
	char basename[MAX_CHARS];	/* == Charactername (Nickname)? */
	char realname[MAX_CHARS];	/* Userid (local machine's user name, default is 'PLAYER') */
	char accountname[MAX_CHARS];
	char hostname[MAX_CHARS];	/* His hostname */
	char addr[MAX_CHARS];		/* His IP address */
	//unsigned int version;		/* His version */
	version_type version;
	bool v_outdated, v_latest, v_test, v_test_latest, v_unknown;
	bool rogue_like_commands;

	s32b id;			/* Unique ID to each player */
	u32b account;			/* account group id */
	u32b dna;			/* DNA - psuedo unique to each player life */
	s32b turn;			/* Player's birthday */
	s32b turns_online;		/* How many turns this char has spent online */
	s32b turns_afk;			/* How many turns this char has spent online while being /afk */
	s32b turns_idle;		/* How many turns this char has spent online while being counted as 'idle' */
	s32b turns_active;		/* How many turns this char has spent online while being neither 'idle' nor 'afk' at once */
	time_t msg;			/* anti spamming protection */
	byte msgcnt;
	byte spam;
	byte talk;			/* talk too much (moltors idea) */

	player_list_type *hostile;	/* List of players we wish to attack */

	char savefile[MAX_PATH_LENGTH];	/* Name of the savefile */

	byte restricted;		/* account is restricted (ie after cheating) */
	byte privileged;		/* account is privileged (ie for quest running) */
	byte pvpexception;		/* account uses different pvp rules than server settings */
	byte mutedchat;			/* account has chat restrictions */
	bool inval;			/* Non validated account */
	bool newly_created;		/* Just newly created char by player_birth()? */

	bool alive;			/* Are we alive */
	bool death;			/* Have we died */
	bool safe_float;		/* for safe_float option */
	int safe_float_turns;
	bool safe_sane;			/* Save players from insanity-death on resurrection (atomic flag) - C. Blue */
	int deathblow;			/* How much damage the final blow afflicted */
	u16b deaths, soft_deaths;	/* Times this character died so far / safely-died (no real death) so far */
	s16b ghost;			/* Are we a ghost */
	s16b fruit_bat;			/* Are we a fruit bat */
	byte lives;			/* number of times we have ressurected */
	byte houses_owned;		/* number of simultaneously owned houses */
	byte castles_owned;		/* number of owned castles */

	byte prace;			/* Race index */
	byte pclass;			/* Class index */
	byte ptrait;
	byte male;			/* Sex of character */
//FREE
	byte oops;			/* Unused */

	skill_player s_info[MAX_SKILLS]; /* Player skills */
	s16b skill_points;		/* number of skills assignable */

	/* Copies for /undoskills - mikaelh */
	skill_player s_info_old[MAX_SKILLS]; /* Player skills */
	s16b skill_points_old;		/* number of skills assignable */
	bool reskill_possible;

	s16b class_extra;		/* Class extra info */

	byte hitdie;			/* Hit dice (sides) */
	s16b expfact;			/* Experience factor */

//DEPRECATED
	byte maximize;			/* Maximize stats */
	byte preserve;			/* Preserve artifacts */

	s16b age;			/* Characters age */
	s16b ht;			/* Height */
	s16b wt;			/* Weight */
	s16b sc;			/* Social Class */

//UNUSED but set in do_cmd_steal and do_life_scroll
	u16b align_law;			/* alignment */
	u16b align_good;

	player_race *rp_ptr;		/* Pointers to player tables */
	player_class *cp_ptr;
	player_trait *tp_ptr;

	s32b au;			/* Current Gold */

	s32b max_exp;			/* Max experience */
	s32b exp;			/* Cur experience */
	u16b exp_frac;			/* Cur exp frac (times 2^16) */

	s16b lev;			/* Level */
	s16b max_lev;			/* Usual level after 'restoring life levels' */

	s16b mhp;			/* Max hit pts */
	s16b chp;			/* Cur hit pts */
	u16b chp_frac;			/* Cur hit frac (times 2^16) */
	s16b player_hp[PY_MAX_LEVEL];
	s16b form_hp_ratio;		/* mimic form HP+ percentage */
	bool hp_drained;		/* hack for client-size recognition of "harmless" lifedrain damage */

	s16b msp;			/* Max mana pts */
	s16b csp;			/* Cur mana pts */
	u16b csp_frac;			/* Cur mana frac (times 2^16) */

	s16b mst;			/* Max stamina pts */
	s16b cst;			/* Cur stamina pts */
	s16b cst_frac;			/* 1/10000 */

	object_type *inventory;		/* Player's inventory */
	object_type *inventory_copy;	/* Copy of the last inventory sent to the client */

	/* Inventory revisions */
	inventory_change_type *inventory_changes; /* List of recent inventory changes */
	int inventory_revision;		/* Current inventory ID */
	char inventory_changed;		/* Inventory has changed since last update to the client */

	s32b total_weight;		/* Total weight being carried */

	s16b inven_cnt;			/* Number of items in inventory */
	s16b equip_cnt;			/* Number of items in equipment */

	s16b max_plv;			/* Max Player Level */
	s16b max_dlv;			/* Max dungeon level explored. */
	worldpos recall_pos;		/* what position to recall to */
	u16b town_x, town_y;

	int avoid_loc;			/* array size of locations to avoid when changing wpos (recalling) */
	int *avoid_loc_x, *avoid_loc_y;

	s16b stat_max[6];		/* Current "maximal" stat values */
	s16b stat_cur[6];		/* Current "natural" stat values */

	char history[4][60];		/* The player's "history" */

	unsigned char wild_map[MAX_WILD_8]; /* the wilderness we have explored */

	s16b py;			/* Player location in dungeon */
	s16b px;

	struct worldpos wpos;

	s16b cur_hgt;			/* Height and width of their dungeon level */
	s16b cur_wid;

	bool new_level_flag;		/* Has this player changed depth? */
	byte new_level_method;		/* Climb up stairs, down, or teleport level? */

	/* changed from byte to u16b - mikaelh */
	u16b party;			/* The party he belongs to (or 0 if neutral) */
	byte guild;			/* The guild he belongs to (0 if neutral)*/
	u32b guild_dna;			/* Remember the guild, to avoid confusion it was disbanded while we were offline */

	s32b target_who;
	s16b target_col;		/* What position is targetted */
	s16b target_row;

	s16b health_who;		/* Who's shown on the health bar */

	s16b view_n;			/* Array of grids viewable to player */
	byte view_y[VIEW_MAX];
	byte view_x[VIEW_MAX];

	s16b lite_n;			/* Array of grids lit by player lite */
	byte lite_y[LITE_MAX];
	byte lite_x[LITE_MAX];

	s16b temp_n;			/* Array of grids used for various things */
	byte temp_y[TEMP_MAX];
	byte temp_x[TEMP_MAX];

	s16b target_n;			/* Array of grids used for targetting/looking */
	byte target_y[TEMP_MAX];
	byte target_x[TEMP_MAX];
	byte target_state[TEMP_MAX];
	s16b target_idx[TEMP_MAX];

	char infofile[MAX_PATH_LENGTH];	/* Temp storage of *ID* and Self Knowledge info */
	char cur_file[MAX_PATH_LENGTH];	/* Filename this player's viewing */
	char cur_file_title[MAX_CHARS];	/* Filename this player's viewing */
	byte special_file_type;		/* Is he using *ID* or Self Knowledge? */

	u32b dlev_id;			/* ID of the dungeon floor the player logged out on
					   or 0 for surface, to decide about cave_flag reset. - C. Blue */
	byte cave_flag[MAX_HGT][MAX_WID]; /* Can the player see this grid? */

	bool mon_vis[MAX_M_IDX];	/* Can this player see these monsters? */
	bool mon_los[MAX_M_IDX];

	bool obj_vis[MAX_O_IDX];	/* Can this player see these objcets? */

	bool play_vis[MAX_PLAYERS];	/* Can this player see these players? */
	bool play_los[MAX_PLAYERS];

	bool obj_aware[MAX_K_IDX];	/* Is the player aware of this obj type? */
	bool obj_tried[MAX_K_IDX];	/* Has the player tried this obj type? */
	//obj_felt and obj_felt_heavy have currently no function
	bool obj_felt[MAX_K_IDX];	/* Has the player felt the value of this obj type via pseudo-id before? - C. Blue */
	bool obj_felt_heavy[MAX_K_IDX];	/* Has the player had strong pseudo-id on this item? */

	bool trap_ident[MAX_T_IDX];	/* do we know the name */

	byte d_attr[MAX_K_IDX];
	char d_char[MAX_K_IDX];
	byte f_attr[MAX_F_IDX];
	byte f_attr_solid[MAX_F_IDX];
	char f_char[MAX_F_IDX];
	char f_char_solid[MAX_F_IDX];
	char f_char_mod[256];
	byte k_attr[MAX_K_IDX];
	char k_char[MAX_K_IDX];
	byte r_attr[MAX_R_IDX];
	char r_char[MAX_R_IDX];
	char r_char_mod[256];

	bool carry_query_flag;
	bool use_old_target;
	bool always_pickup;
	bool stack_force_notes;
	bool stack_force_costs;
	bool short_item_names;

	bool find_ignore_stairs;
	bool find_ignore_doors;
	bool find_cut;
	bool find_examine;
	bool disturb_move;
	bool disturb_near;
	bool disturb_see;
	bool disturb_panel;
	bool disturb_state;
	bool disturb_minor;
	bool disturb_other;

	bool alert_hitpoints;
	bool alert_mana;
	bool alert_afk_dam;
	bool alert_offpanel_dam;
	bool no_alert;
	bool auto_afk;
	bool newb_suicide;
	bool stack_allow_items;
	bool stack_allow_wands;
	bool view_perma_grids;
	bool view_torch_grids;

	bool view_reduce_lite;
	bool view_reduce_view;
	bool view_lamp_floor;
	bool view_lamp_walls;
	bool view_shade_floor;
	bool view_shade_walls;
	bool wall_lighting;
	bool floor_lighting;
	bool view_animated_lite;
	bool view_lite_extra;
	bool permawalls_shade;
	bool live_timeouts;

	/* TomeNET additions -- consider using macro or bitfield */
	bool easy_open;
	bool easy_disarm;
	bool easy_tunnel;
	//bool auto_destroy;
	bool clear_inscr;
	bool auto_inscribe;
	bool taciturn_messages;
	bool last_words;
	bool limit_chat;
	bool no_afk_msg;
	/* bool speak_unique; */

	/* 'make clean; make' consumes time :) */
	bool depth_in_feet;
	bool auto_target;
	bool autooff_retaliator;
	bool wide_scroll_margin;
	bool always_repeat;
	bool fail_no_melee;
	bool dummy_option_7;
	bool dummy_option_8;

	bool page_on_privmsg;
	bool page_on_afk_privmsg;
	bool player_list;
	bool player_list2;
	bool auto_untag;
	bool idle_starve_kick;
	bool newbie_hints;
	bool censor_swearing;
	bool warn_unique_credit;
	bool uniques_alive;
	bool overview_startup;

	s16b max_panel_rows, max_panel_cols;
	s16b panel_row, panel_col;
	s16b panel_row_min, panel_col_max;
	s16b panel_col_min, panel_row_max;
	s16b panel_row_prt, panel_col_prt; /* What panel this guy's on */
	s16b panel_row_old, panel_col_old;
#if 1	/* used for functions that still need to use the 'traditional' panel size of 66x22, eg magic mapping */
	/* panel values assumed we'd use SCREEN_WID x SCREEN_HGT panels (and maybe for [x,y] location display) */
	s16b max_tradpanel_rows, max_tradpanel_cols;
	s16b tradpanel_row, tradpanel_col;
	s16b tradpanel_row_min, tradpanel_col_min;
	s16b tradpanel_row_max, tradpanel_col_max;
#endif

	s16b screen_wid;
	s16b screen_hgt;

	/* What he should be seeing */
	cave_view_type scr_info_guard_before[MAX_WINDOW_WID+1]; /* overflow protection */
	cave_view_type scr_info[MAX_WINDOW_HGT][MAX_WINDOW_WID]; /* Hard-coded Y*X display */
	cave_view_type scr_info_guard_after[MAX_WINDOW_WID+1]; /* overflow protection */

	/* Overlay layer used for detection */
	cave_view_type ovl_info_guard_before[MAX_WINDOW_WID+1]; /* overflow protection */
	cave_view_type ovl_info[MAX_WINDOW_HGT][MAX_WINDOW_WID]; /* Hard-coded Y*X display */
	cave_view_type ovl_info_guard_after[MAX_WINDOW_WID+1]; /* overflow protection */

	s32b mimic_seed;		/* seed for random mimic immunities etc. */
	char mimic_immunity;		/* preferred immunity when mimicking (overrides mimic_seed) */

	char died_from[MAX_CHARS];	/* What off-ed him */
	char really_died_from[MAX_CHARS]; /* What off-ed him */
	char died_from_list[MAX_CHARS]; /* what goes on the high score list */
	s16b died_from_depth;		/* what depth we died on */

	u16b total_winner;		/* Is this guy the winner */
	u16b once_winner;		/* Has this guy ever been a winner */
	bool iron_winner, iron_winner_ded; /* for those who beat the Ironman Deep Dive Challenge */
	struct worldpos own1, own2;	/* IF we are a king what do we own ? */
	u16b retire_timer;		/* The number of minutes this guy can play until
					   he will be forcibly retired. */

	u16b noscore;			/* Has he cheated in some way (hopefully not) */
	s16b command_rep;		/* Command repetition */
	s16b command_rep_discard;	/* Command repetition assist: Don't discard the first of the new repeated action packets.
					   Needed addition for when command_rep is now used in vital functions such as zapping rods, because of '!X':
					   Healing rod zaps must not be discarded! So far it wasn't important or even noticable for minor stuff like bash/disarm/open/tunnel.. */
#ifdef XID_REPEAT
	s16b command_rep_temp;		/* Command repetition */
	bool command_rep_active;	/* Semaphore to avoid packet spam when re-injecting packets after command_rep was temporarily killed by Receive_inventory_revision() */
	int delayed_index_temp;
	int delayed_spell_temp;
	s16b current_item_temp;
#endif

	byte last_dir;			/* Last direction moved (used for swapping places) */

	s16b running;			/* Are we running */
	byte find_current;		/* These are used for the running code */
	byte find_prevdir;
	bool find_openarea;
	bool find_breakright;
	bool find_breakleft;
	bool running_on_floor;		/* Are we running on normal floor, or over grids that we have special abilities to actually pass */

	bool resting;			/* Are we resting? */
	s16b energy_use;		/* How much energy has been used */

	int look_index;			/* Used for looking or targeting */

	s32b current_char;
	s16b current_spell;		/* Spell being cast */
	s16b current_realm;		/* Realm of spell being cast */
	s16b current_mind;		/* Power being use */
	/* XXX XXX consider using union or sth */
	s16b current_rod;		/* Rod being zapped */
	s16b current_activation;	/* Artifact (or dragon mail) being activated */
	s16b current_enchant_h;		/* Current enchantments */
	s16b current_enchant_d;
	s16b current_enchant_a;
	s16b current_enchant_flag;
	s16b current_identify;		/* Are we identifying something? */
	s16b current_star_identify;
	s16b current_recharge;
	s16b current_artifact;
	bool current_artifact_nolife;
	object_type *current_telekinesis;
#ifdef TELEKINESIS_GETITEM_SERVERSIDE
	s16b current_telekinesis_mw;
#endif
	s16b current_curse;
	s16b current_tome_creation;	/* adding a spell scroll to a custom tome - C. Blue */
	s16b current_rune;
	s16b current_force_stack;	/* which level 0 item we're planning to stack */
	s16b current_wand;
	s16b current_item;
	s16b current_aux;
	s16b current_fire;
	s16b current_throw;
	s16b current_book;
	s16b current_rcraft;
	u16b current_rcraft_e_flags;
	u16b current_rcraft_m_flags;
	s16b current_breath;
	s16b current_selling;
	s16b current_sell_amt;
	int current_sell_price;
	bool current_create_sling_ammo;

	int using_up_item;		/* Item being used up while enchanting, *ID*ing etc. */

	int store_num;			/* What store this guy is in */
#ifdef PLAYER_STORES
	int ps_house_x, ps_house_y;	/* coordinates of the house linked to current player store */
	int ps_mcheque_x;		/* Index or x-coordinate of existing mass-cheque in the house */
	int ps_mcheque_y;		/* y-coordinate of existing mass-cheque in the house */
#endif

	s16b fast;			/* Timed -- Fast */
	s16b fast_mod;   		/* Timed -- Fast */
	s16b slow;			/* Timed -- Slow */
	s16b blind;			/* Timed -- Blindness */
	s16b paralyzed;			/* Timed -- Paralysis */
	s16b confused;			/* Timed -- Confusion */
	s16b afraid;			/* Timed -- Fear */
	s16b image;			/* Timed -- Hallucination */
	s16b poisoned;			/* Timed -- Poisoned */
	s16b slow_poison;
	int poisoned_attacker;		/* Who poisoned the player - used for blood bond */
	s16b cut;			/* Timed -- Cut */
	int cut_attacker;		/* Who cut the player - used for blood bond */
	s16b stun;			/* Timed -- Stun */

	byte xtrastat_tim;		/* timed temp +stats */
	byte xtrastat_pow;		/* power */
	s16b xtrastat_which;		/* which */

	s16b focus_time;		/* focus */
	s16b focus_val;

	s16b protevil;			/* Timed -- Protection */
	s16b zeal;			/* timed EA bonus */
	s16b zeal_power;
	s16b martyr;
	s16b martyr_timeout;
	s16b martyr_dur;
	s16b res_fear_temp;
	s16b invuln, invuln_applied;	/* Timed -- Invulnerable; helper var */
	s16b invuln_dur;		/* How long this invuln was when it started */
	s16b hero;			/* Timed -- Heroism */
	s16b shero;			/* Timed -- Super Heroism */
	s16b berserk;			/* Timed -- Berserk #2 */
	s16b fury;			/* Timed -- Furry */
	s16b tim_thunder;		/* Timed thunderstorm */
	s16b tim_thunder_p1;		/* Timed thunderstorm */
	s16b tim_thunder_p2;		/* Timed thunderstorm */
	s16b tim_ffall;			/* Timed Feather Falling */
	s16b tim_lev;			/* Timed Levitation */
	s16b shield;			/* Timed -- Shield Spell */
	s16b shield_power;		/* Timed -- Shield Spell Power */
	s16b shield_opt;		/* Timed -- Shield Spell options */
	s16b shield_power_opt;		/* Timed -- Shield Spell Power */
	s16b shield_power_opt2;		/* Timed -- Shield Spell Power */
	s16b tim_regen;			/* Timed extra regen */
	s16b tim_regen_pow;		/* Timed extra regen power */
	s16b blessed;			/* Timed -- Blessed */
	s16b blessed_power;		/* Timed -- Blessed */
	s16b tim_invis;			/* Timed -- See Invisible */
	s16b tim_infra;			/* Timed -- Infra Vision */
	s16b tim_wraith;		/* Timed -- Wraithform */
	u16b tim_jail;			/* Timed -- Jailed */
	u16b tim_susp;			/* Suspended sentence (dungeon) */
	u16b house_num;			/* Added for easier jail-leaving handling: House index of jail we're in */
	u16b tim_pkill;			/* pkill changeover timer */
	u16b pkill;			/* pkill flags */
	u16b tim_store;			/* timed -- how long (s)he can stay in a store */
	bool wraith_in_wall;		/* currently no effect! */
	s16b tim_meditation;		/* Timed -- Meditation */
	s16b tim_invisibility;		/* Timed -- Invisibility */
	s16b tim_invis_power;		/* Timed -- Invisibility Power (perm) */
	s16b tim_invis_power2;		/* Timed -- Invisibility Power (temp) */
	s16b tim_traps;			/* Timed -- Avoid traps */
	s16b tim_manashield;		/* Timed -- Mana Shield */
	s16b tim_mimic;			/* Timed -- Mimicry */
	s16b tim_mimic_what;		/* Timed -- Mimicry */
//UNUSED just queried
	s16b bow_brand;			/* Timed -- Bow Branding */
	byte bow_brand_t;		/* Timed -- Bow Branding */
	s16b bow_brand_d;		/* Timed -- Bow Branding */
	s16b brand;			/* Timed -- Weapon Branding (used by runecraft) */
	byte brand_t;			/* Timed -- Weapon Branding */
	s16b brand_d;			/* Timed -- Weapon Branding */
	bool brand_fire;		/* Added for Draconians, but could clean up a lot of tot_dam_aux.. code too */
	bool brand_cold;
	bool brand_elec;
	bool brand_acid;
	bool brand_pois;
	s16b prob_travel;		/* Timed -- Probability travel */
	s16b st_anchor;			/* Timed -- Space/Time Anchor */
	s16b tim_esp;			/* Timed -- ESP */
	s16b adrenaline;
	s16b biofeedback;
	s16b mindboost;
	s16b mindboost_power;
	s16b kinetic_shield;

#ifdef ENABLE_OCCULT
	s16b temp_savingthrow;
	s16b spirit_shield;
	s16b spirit_shield_pow;
#endif

	s16b auto_tunnel;
	s16b body_monster, body_monster_prev;
	bool dual_wield;		/* Currently wielding 2 one-handers at once */

	s16b bless_temp_luck;		/* Timed blessing - luck */
	s16b bless_temp_luck_power;

	s16b oppose_acid;		/* Timed -- oppose acid */
	s16b oppose_elec;		/* Timed -- oppose lightning */
	s16b oppose_fire;		/* Timed -- oppose heat */
	s16b oppose_cold;		/* Timed -- oppose cold */
	s16b oppose_pois;		/* Timed -- oppose poison */

	s16b word_recall;		/* Word of recall counter */

	s16b energy;			/* Current energy */
	bool requires_energy;		/* Player requires energy to perform a normal action instead of shooting-till-kill (and auto-retaliating?) */

	s16b food;			/* Current nutrition */

	byte confusing;			/* Glowing hands */
	byte stunning;			/* Heavy hands */
	byte searching;			/* Currently searching */

	bool old_cumber_armor;
	bool old_awkward_armor;
	bool old_cumber_glove;
	bool old_cumber_helm;
	bool old_heavy_wield;
	bool old_heavy_shield;
	bool old_heavy_shoot;
	bool old_icky_wield;
	bool old_awkward_wield;
	bool old_easy_wield;
	bool old_cumber_weight;
	bool old_monk_heavyarmor;
	bool old_awkward_shoot;
	bool old_rogue_heavyarmor;
	bool old_heavy_swim;

	s16b old_lite;			/* Old radius of lite (if any) */
	s16b old_vlite;			/* Old radius of virtual lite (if any) */
	s16b old_view;			/* Old radius of view (if any) */

	s16b old_food_aux;		/* Old value of food */

	bool cumber_armor;		/* Encumbering armor (tohit/sneakiness) */
	bool awkward_armor;		/* Mana draining armor */
	bool cumber_glove;		/* Mana draining gloves */
	bool cumber_helm;		/* Mana draining headgear */
	bool heavy_wield;		/* Heavy weapon */
	bool heavy_shield;		/* Heavy shield */
	bool heavy_shoot;		/* Heavy shooter */
	bool icky_wield;		/* Icky weapon */
	bool awkward_wield;		/* shield and COULD_2H weapon */
	bool easy_wield;		/* Using a 1-h weapon which is MAY2H with both hands */
	bool cumber_weight;		/* Full weight. FA from MA will be lost if overloaded */
	bool monk_heavyarmor;		/* Reduced MA power? */
	bool awkward_shoot;		/* using ranged weapon while having a shield on the arm */
	bool rogue_heavyarmor;		/* No AoE-searching? Encumbered dual-wield? */
	bool heavy_swim;		/* Too heavy to swim without drowning chance? */

	s16b cur_lite;			/* Radius of lite (if any) */
	s16b cur_vlite;			/* radius of virtual light (not visible to others) */
	byte lite_type;


	u32b notice;			/* Special Updates (bit flags) */
	u32b update;			/* Pending Updates (bit flags) */
	u32b redraw;			/* Normal Redraws (bit flags) */
	u32b redraw2;			/* more Normal Redraws (bit flags) */
	u32b window;			/* Window Redraws (bit flags) */

	s16b stat_use[6];		/* Current modified stats */
	s16b stat_top[6];		/* Maximal modified stats */

	s16b stat_add[6];		/* Modifiers to stat values */
	s16b stat_ind[6];		/* Indexes into stat tables */

	s16b stat_cnt[6];		/* Counter for temporary drains */
	s16b stat_los[6];		/* Amount of temporary drains */

	bool immune_acid;		/* Immunity to acid */
	bool immune_elec;		/* Immunity to lightning */
	bool immune_fire;		/* Immunity to fire */
	bool immune_cold;		/* Immunity to cold */

//UNUSED just queried
	s16b reduc_fire;		/* Fire damage reduction */
	s16b reduc_elec;		/* elec damage reduction */
	s16b reduc_acid;		/* acid damage reduction */
	s16b reduc_cold;		/* cold damage reduction */

	bool resist_acid;		/* Resist acid */
	bool resist_elec;		/* Resist lightning */
	bool resist_fire;		/* Resist fire */
	bool resist_cold;		/* Resist cold */
	bool resist_pois;		/* Resist poison */

	bool resist_conf;		/* Resist confusion */
	bool resist_sound;		/* Resist sound */
	bool resist_lite;		/* Resist light */
	bool resist_dark;		/* Resist darkness */
	bool resist_chaos;		/* Resist chaos */
	bool resist_disen;		/* Resist disenchant */
	bool resist_discharge;		/* Resist UN_POWER discharging */
	bool resist_shard;		/* Resist shards */
	bool resist_nexus;		/* Resist nexus */
	bool resist_blind;		/* Resist blindness */
	bool resist_neth;		/* Resist nether */
	bool resist_fear;		/* Resist fear */

	bool sustain_str;		/* Keep strength */
	bool sustain_int;		/* Keep intelligence */
	bool sustain_wis;		/* Keep wisdom */
	bool sustain_dex;		/* Keep dexterity */
	bool sustain_con;		/* Keep constitution */
	bool sustain_chr;		/* Keep charisma */

	bool aggravate;			/* Aggravate monsters */
	bool teleport;			/* Random teleporting */

	bool feather_fall;		/* No damage falling */
	bool lite;			/* Permanent light */
	bool free_act;			/* Never paralyzed */
	bool see_inv;			/* Can see invisible */
	bool regenerate;		/* Regenerate hit pts */
	bool resist_time;		/* Resist time */
	bool resist_mana;		/* Resist mana */
	bool immune_poison;		/* Poison immunity */
	bool immune_water;		/* Makes immune to water */
	bool resist_water;		/* Resist Water */
	bool regen_mana;		/* Regenerate mana */
	bool keep_life;			/* Immune to life draining */
	bool hold_life;			/* Resist life draining */
	u32b telepathy;			/* Telepathy */
	bool slow_digest;		/* Slower digestion */
	bool bless_blade;		/* Blessed blade */
	byte xtra_might;		/* Extra might bow */
	bool impact;			/* Earthquake blows */
        bool auto_id;			/* Pickup = Id */
	bool reduce_insanity;		/* For mimic forms with weird/empty mind */

	s16b invis;			/* Invisibility */

	s16b dis_to_h;			/* Known bonus to hit */
	s16b dis_to_d;			/* Known bonus to dam */
	s16b dis_to_h_ranged;		/* Known bonus to hit */
	s16b dis_to_d_ranged;		/* Known bonus to dam */
	s16b dis_to_a;			/* Known bonus to ac */
	s16b dis_ac;			/* Known base ac */

	s16b to_h_ranged;		/* Bonus to hit */
	s16b to_d_ranged;		/* Bonus to dam */
	s16b to_h_melee;		/* Bonus to hit */
	s16b to_d_melee;		/* Bonus to dam */
	s16b to_h;			/* Bonus to hit */
	s16b to_d;			/* Bonus to dam */
	s16b to_a;			/* Bonus to ac */

	s16b ac;			/* Base ac */

	/* just for easy LUA handling; not game-play relevant: */
	s16b overall_tohit_r, overall_todam_r, overall_tohit_m, overall_todam_m;

	s16b see_infra;			/* Infravision range */

	s16b skill_dis;			/* Skill: Disarming */
	s16b skill_dev;			/* Skill: Magic Devices */
	s16b skill_sav;			/* Skill: Saving throw */
	s16b skill_stl;			/* Skill: Stealth factor */
	s16b skill_srh;			/* Skill: Searching ability */
	s16b skill_fos;			/* Skill: Searching frequency */
	s16b skill_thn;			/* Skill: To hit (normal) */
	s16b skill_thb;			/* Skill: To hit (shooting) */
	s16b skill_tht;			/* Skill: To hit (throwing) */
	s16b skill_dig;			/* Skill: Digging */

	s16b num_blow;			/* Number of blows */
	s16b num_fire;			/* Number of shots */
	s16b num_spell;			/* Number of spells */

	byte tval_xtra;			/* Correct xtra tval */
	byte tval_ammo;			/* Correct ammo tval */
	s16b pspeed;			/* Current speed */

 	s16b r_killed[MAX_R_IDX];	/* Monsters killed */
 	s16b r_mimicry[MAX_R_IDX];	/* Monster kill count or mimicry */

	s32b melee_techniques_old;	/* melee techniques before last skill point update */
	s32b melee_techniques;		/* melee techniques */
	s32b ranged_techniques_old;	/* ranged techniques before last skill point update */
	s32b ranged_techniques;		/* ranged techniques */
	s32b innate_spells[3];		/* Monster spells */
	bool body_changed;

	bool anti_magic;		/* Can the player resist magic */

	player_list_type *blood_bond;	/* Norc is now happy :) */

	byte mode;			/* Difficulty MODE */

#if 1
	s32b esp_link;			/* Mental link */
	byte esp_link_type;
	u16b esp_link_flags;
	u16b esp_link_end;		/* Time before actual end */
#else
	/* new esp link stuff - mikaelh */
	esp_link_type *esp_link;	/* Mental link */
	u16b esp_link_flags;		/* Some flags */
#endif
	bool (*master_move_hook)(int Ind, char *args);

	/* some new borrowed flags (saved) */
	bool black_breath;		/* The Tolkien's Black Breath */
	bool black_breath_tmp;		/* (NOT saved) BB induced by an item */
	/*u32b malady;*/		/* TODO: Flags for malady */

	s16b msane;			/* Max sanity */
	s16b csane;			/* Cur sanity */
	u16b csane_frac;		/* Cur sanity frac */
	byte sanity_bar;		/* preferred type of SN: bar, if player has sufficient Health skill */
	u16b csane_prev;		/* Previous value of 'Cur sanity' (for alert_offpanel_dam) */

	/* elements under this line won't be saved...for now. - Jir - */
	player_list_type *ignore;	/* List of players whose chat we wish to ignore */
	struct remote_ignore *w_ignore;	/* List of players whose chat we wish to ignore */
	long int idle;			/* player is idling for <idle> seconds.. */
	long int idle_char;		/* character is idling for <idle_char> seconds (player still might be chatting etc) */
	bool afk;			/* player is afk */
	char afk_msg[MAX_CHARS];	/* afk reason */
	char info_msg[MAX_CHARS];	/* public info message (display gets overridden by an afk reason, if specified) */
//CHECK
	bool use_r_gfx;			/* hack - client uses gfx? */
	bool custom_font;		/* Did player client upload custom attr/char mappings? */
	player_list_type *afk_noticed;	/* Only display AFK messages once in private conversations */

	byte drain_exp;			/* Experience draining */
	byte drain_life;		/* hp draining */
	byte drain_mana;		/* mana draining */

	bool suscep_fire;		/* Fire does more damage on the player */
	bool suscep_cold;		/* Cold does more damage on the player */
	bool suscep_acid;		/* Acid does more damage on the player */
	bool suscep_elec;		/* Electricity does more damage on the player */
	bool suscep_pois;		/* Poison does more damage on the player */
	bool suscep_lite;		/* Light does more damage on the player */
	bool suscep_good;		/* Anti-evil effects do more damage on the player */
	bool suscep_evil;		/* Anti-good effects do more damage on the player */
	bool suscep_life;		/* Anti-undead effects do more damage on the player */

	bool reflect;			/* Reflect 'bolt' attacks */
	int shield_deflect;		/* Deflect various attacks (ranged), needs USE_BLOCKING */
	int weapon_parry;		/* Parry various attacks (melee), needs USE_PARRYING */
	bool no_cut;			/* For mimic forms */
	bool sh_fire;			/* Fiery 'immolation' effect */
	bool sh_fire_tim, sh_fire_fix;
	bool sh_elec;			/* Electric 'immolation' effect */
	bool sh_elec_tim, sh_elec_fix;
	bool sh_cold;			/* Cold 'immolation' effect */
	bool sh_cold_tim, sh_cold_fix;
	bool wraith_form;		/* wraithform */
	bool immune_neth;		/* Immunity to nether */
	bool climb;			/* Can climb mountains */
	bool levitate;			/* Can levitate over some features */
	bool can_swim;			/* Can swim like a fish (or Lizard..whatever) */
	bool pass_trees;		/* Can pass thick forest */
	bool town_pass_trees;		/* Can pass forest in towns, as an exception to make movement easier */

	int luck;			/* Extra luck of this player */

	/*byte anti_magic_spell;*/	/* Anti-magic(newer one..) */
	byte antimagic;    		/* Anti-magic(in percent) */
	byte antimagic_dis;		/* Radius of the anti magic field */
	bool anti_tele;			/* Prevent any teleportation + phasing + recall */
	bool res_tele;			/* Prevents being teleported from someone else */
	bool resist_continuum;		/* non-timed -- Space/Time Anchor - in PernM, it's same as st_anchor */
	bool admin_wiz;			/* Is this char Wizard? */
	bool admin_dm;			/* or Dungeon Master? */
	bool admin_dm_chat;		/* allow players to send private chat to an invisible DM */
	bool stormbringer;		/* Attack friends? */
	int vampiric_melee;		/* vampiric in close combat? */
	int vampiric_ranged;		/* shots have vampiric effects? */
	int vamp_fed_midx;		/* monster we fed from */

	bool ty_curse;			/* revived these two, in different forms */
	bool dg_curse;

	u16b xorder_id;			/* Extermination order number */
	s16b xorder_num;		/* Number of kills needed */

	s16b xtra_crit;			/* critical strike bonus from item */
	s16b extra_blows;		/* Number of extra blows */

	s16b to_l;			/* Bonus to life */
	s32b to_hp;			/* Bonus to Hit Points */
	s16b to_m;			/* Bonus to mana */
	//s16b to_s;			/* Bonus to spell(num_spell) */
	s16b dodge_level;		/* Chance of dodging blows/missiles */

	s32b balance;			/* Deposit/debt */
	s32b tim_blacklist;		/* Player is on the 'Black List' (he gets penalties in shops) */
	s32b tim_watchlist;		/* Player is on the 'Watch List' (he may not steal) */
	s32b pstealing;			/* Player has just tried to steal from another player. Cooldown timer. */
	int ret_dam;                    /* Drained life from a monster */
	char attacker[MAX_CHARS];	/* Monster doing a ranged attack on the player */
#if 0
	s16b mtp;			/* Max tank pts */
	s16b ctp;			/* Cur tank pts */
	s16b tp_aux1;			/* aux1 tank pts */
	s16b tp_aux2;			/* aux2 tank pts */

	s32b grace;			/* Your God's appreciation factor. */
	byte pgod;			/* Your God. */
	bool praying;			/* Praying to your god. */
	s16b melkor_sacrifice;		/* How much hp has been sacrified for damage */
#endif	/* 0 */

	byte spell_project;		/* Do the spells(some) affect nearby party members ? */

	/* Special powers */
//UNUSED
	s16b powers[MAX_POWERS];	/* What powers do we possess? */
	s16b power_num;			/* How many */

	/* evileye games */
	s16b team;			/* what team */

#ifdef ARCADE_SERVER
	/* Moltor's arcade crap */
	int arc_a, arc_b, arc_c, arc_d, arc_e, arc_f, arc_g, arc_h, arc_i, arc_j, arc_k, arc_l;
	char firedir;
	char game;
	int gametime;
	char pushed;
	char pushdir;
#endif

	bool panic;			/* C. Blue - was the last shutdown a panic save? */

	/* Anti-cheeze */
	s16b supp, supp_top;		/* level of the highest supporter (who casted buffs/heals on us) */
	s16b support_timer;		/* safe maximum possible duration of the support spells */

	byte updated_savegame;		/* any automatic savegame update to perform? (toggle) */
	byte artifact_reset;		/* for automatic artifact reset (similar to updated_savegame) */
	bool fluent_artifact_reset;
	s16b corner_turn;		/* C. Blue - Fun stuff :) Make player vomit if he turns around ***a lot*** (can't happen in 'normal' gameplay) */
	byte auto_transport;		/* automatic (scripted) transport sequences */
	byte paging;			/* Player being paged by others? (Beep counter) */
	byte ignoring_chat;		/* Ignoring normal chat? (Will only see private & party messages then) */
	bool muted;			/* Being an ass? - the_sandman */
	byte has_pet;			/* Pet limiter */
	/* Is the player auto-retaliating? (required for hack that fixes a lock bug) */
	bool auto_retaliating;
	bool auto_retaliaty;		/* TRUE for code-wise duration of autorataliation
					   actions, to prevent going un-AFK from them! */
	bool ar_test_fail;
	monster_type *ar_m_target_ptr, *ar_prev_m_target_ptr;
	player_type *ar_p_target_ptr, *ar_prev_p_target_ptr;
	int ar_target, ar_item;
	cptr ar_at_O_inscription;
	bool ar_fallback, ar_no_melee;

	/* Global events participant? */
	int global_event_type[MAX_GLOBAL_EVENTS]; /* 0 means 'not participating' */
	time_t global_event_signup[MAX_GLOBAL_EVENTS];
	time_t global_event_started[MAX_GLOBAL_EVENTS];
	u32b global_event_progress[MAX_GLOBAL_EVENTS][4];
	u32b global_event_temp; /* not saved. see defines.h for details */
	int global_event_participated[MAX_GLOBAL_EVENT_TYPES];

	/* Had a quest running when he logged out or something? ->respawn/reactivate quest? todo//unclear yet..
	   THIS IS NEW STUFF: quest_info. Don't confuse it with older quest_type/quest[]/plots[] code sketches in bldg.c. */
	int interact_questor_idx;	/* id in QI_QUESTORS, which questor we just interacted with (bumped into) */
	s16b quest_idx[MAX_PQUESTS];
	char quest_codename[MAX_PQUESTS][10 + 1]; /* track up to 5 quests by their codename and roughly the current stage and goals */
	s32b quest_acquired[MAX_PQUESTS]; /* the turn when it was acquired */
	s32b quest_timed_stage_change[MAX_PQUESTS]; /* turn tracker for automatically timed stage change */
	s16b quest_stage[MAX_PQUESTS]; /* in which stage is a quest? */
	s16b quest_stage_timer[MAX_PQUESTS]; /* stage automatics started a timer leading to stage completion */
	u16b quest_flags[MAX_PQUESTS]; /* our personal quest flags configuration */
	bool quest_goals[MAX_PQUESTS][QI_GOALS]; /* which goals have we completed so far? */
	bool quest_goals_nisi[MAX_PQUESTS][QI_GOALS]; /* which goals have we completed so far? */
	s16b quest_kill_number[MAX_PQUESTS][QI_GOALS]; /* which goals have we completed so far? */
	s16b quest_retrieve_number[MAX_PQUESTS][QI_GOALS]; /* which goals have we completed so far? */
	/* permanent quest info */
	s16b quest_done[MAX_Q_IDX];	/* player has completed a quest (n times) */
	s16b quest_cooldown[MAX_Q_IDX];	/* player has to wait n minutes till picking up the quest again */
	/* for 'individual' quests: */
	/* quest helper info */
	bool quest_any_k, quest_any_k_target, quest_any_k_within_target; /* just roughly remember in general whether ANY of our quests needs killing/retrieving (and maybe only in a particular location) */
	bool quest_any_r, quest_any_r_target, quest_any_r_within_target; /* just roughly remember in general whether ANY of our quests needs killing/retrieving (and maybe only in a particular location) */
	bool quest_any_deliver_xy, quest_any_deliver_xy_within_target;
	bool quest_kill[MAX_PQUESTS];
	bool quest_retrieve[MAX_PQUESTS];
	bool quest_deliver_pos[MAX_PQUESTS], quest_deliver_xy[MAX_PQUESTS];
	byte quest_eligible;		/* temporary, just for efficiency */
	u16b questor_dialogue_hack_xy; /* keep track of player's exact position */
	u32b questor_dialogue_hack_wpos;

#ifdef ENABLE_MAIA
	int voidx; int voidy;		//for the void jumpgate creation spell; reset on every recall/levelchange/relogins

	int divine_crit;
	int divine_hp;
	int divine_xtra_res_time;

	int divine_crit_mod;
	int divine_hp_mod;
	int divine_xtra_res_time_mod;
#endif
	bool got_hit;			/* Prevent players from taking it multiple times from a single effect - mikaelh */
	s32b total_damage;		/* No insane amounts of damage either */
	bool quaked;			/* Prevent players from causing more than one earthquake per round via melee attacks - C. Blue */

#ifdef AUCTION_SYSTEM
	int current_auction;		/* The current auction - mikaelh */
#endif

	/* ENABLE_STANCES - this code must always be compiled, otherwise savegames would screw up! so no #ifdef here. */
	/* combat stances */
	int combat_stance;		/* 0 = normal, 1 = def, 2 = off */
	int combat_stance_power;	/* 1,2,3, and 4 = royal (for NR balanced) */

	/* more techniques */
	byte cloaked, cloak_neutralized; /* Cloaking mode enabled; suspicious action was spotted */
	s16b melee_sprint, ranged_double_used;
	bool ranged_flare, ranged_precision, ranged_double, ranged_barrage;
	bool shadow_running;

#ifdef AUTO_RET_CMD
	int autoret;			/* set auto-retaliation via command instead of inscription */
#endif
	bool shoot_till_kill, shooty_till_kill, shooting_till_kill; /* Shoot a target until it's dead, like a ranged 'auto-retaliator' - C. Blue */
	int shoot_till_kill_book, shoot_till_kill_spell, shoot_till_kill_mimic; //and there's shoot_till_kill_rcraft too
	int shoot_till_kill_wand, shoot_till_kill_rod;
	bool dual_mode; /* for dual-wield: TRUE = dual-mode, FALSE = main-hand-mode */

	/* Runecraft Info */
	bool shoot_till_kill_rcraft;	/* FTK */
	u16b FTK_e_flags;
	u16b FTK_m_flags;
	u16b FTK_energy;

	u16b tim_deflect;

	struct worldpos wpos_old; /* used for dungeon-visit-boni, nether-realm cross-mode and ironman deep dive challenge stuff */

#if 0 /* deprecated */
	/* NOT IMPLEMENTED YET: add spell array for quick access via new method of macroing spells
	   by specifying the spell name instead of a book and position - C. Blue */
	char spell_name[100][20];
	int spell_book[100], spell_pos[100];
#endif

	bool aura[MAX_AURAS];		/* allow toggling auras for possibly more tactical utilization - C. Blue */

	/* for C_BLUE_AI, new thingy: Monsters that are able to ignore a "tank" player */
	int heal_turn[20 + 1];		/* records the amount of healing the player received for each of 20 consecutive turns */
	u32b heal_turn_20, heal_turn_10, heal_turn_5;
	int dam_turn[20 + 1];		/* records the amount of damage the player dealt for each of 20 consecutive turns */
	u32b dam_turn_20, dam_turn_10, dam_turn_5;

	/* for PvP mode: keep track of kills/progress for adding a reward or something - C. Blue */
	int kills, kills_lower, kills_higher, kills_equal, kills_own;
	int free_mimic, pvp_prevent_tele, pvp_prevent_phase;
	long heal_effect;
	bool no_heal;			/* for special events */

	/* for client-side weather */
	bool panel_changed;
	int custom_weather;		/* used /cw command */
	int joke_weather;		/* personal rain^^ */
	bool no_weather;

	/* buffer for anti-cheeze system, just to reduce file access to tomenet.acc */
	s32b cheeze_value, cheeze_self_value;

	int mcharming;			/* for mindcrafters' charming */
	u32b turns_on_floor;		/* number of turns spent on the current floor */
	bool distinct_floor_feeling;	/* set depending on turns_on_floor */
	bool sun_burn;			/* Player is vampire, currently burning in the sun? */

	/* server-side animation timing flags */
	int invis_phase;		/* for invisible players who flicker towards others */
//not needed!	int colour_phase; /* for mimics mimicking multi-coloured stuff */

	/* for hunting down bots generating exp by opening and magelocking doors */
	u32b silly_door_exp;

#if (MAX_PING_RECVS_LOGGED > 0)
	/* list of ping reception times */
	struct timeval pings_received[MAX_PING_RECVS_LOGGED];
	char pings_received_head;
#endif

	int admin_stasis;		/* allow admins to put a character into 'administrative stasis' */
	/* more admin fooling around (give a 1-hit-kill attack to the player, or let him die in 1 hit) */
	int admin_godly_strike, admin_set_defeat;
	bool admin_invuln, admin_invinc; /* Amulets of Invulnerability/Invincibility */
	char admin_parm[MAX_CHARS];	/* optional special admin command parameter (hacky o_O) */

	u32b test_count, test_dam, test_heal, test_turn;
#ifdef TEST_SERVER
	u32b test_attacks;
#endif

	/* give players certain warnings, meant to guide newbies along, and remember
	   if we already gave a specific warning, so we don't spam the player with it
	   again and again - although noone probably reads them anyway ;-p - C. Blue */
	char warning_bpr, warning_bpr2, warning_bpr3;
	char warning_run, warning_run_steps, warning_run_monlos, warning_run_lite;
	char warning_wield, warning_chat, warning_lite, warning_lite_refill;
	char warning_wield_combat; /* warn if engaging into combat (attacking/taking damage) without having equipped melee/ranged weapons! (except for druids) */
	char warning_rest, warning_rest_cooldown;/* if a char rests from <= 40% to 50% without R, or so..*/
	char warning_mimic, warning_dual, warning_dual_mode, warning_potions, warning_wor;
	char warning_ghost, warning_instares, warning_autoret, warning_autoret_ok;
	char warning_ma_weapon, warning_ma_shield;
	char warning_technique_melee, warning_technique_ranged;
	char warning_hungry, warning_autopickup, warning_ranged_autoret;
	/* note: a sort of "warning_skills" is already implemented, in a different manner */
	char warning_cloak, warning_macros, warning_numpadmove;
	char warning_ammotype, warning_ai_annoy;
	char warning_fountain, warning_voidjumpgate, warning_staircase, warning_worldmap, warning_dungeon;
	/* For the 4.4.8.1.0.0 lua update crash bug */
	char warning_lua_update, warning_lua_count;
	char warning_tunnel, warning_tunnel2, warning_tunnel3, warning_trap, warning_tele, warning_fracexp;
	char warning_death;
	char warning_drained, warning_boomerang, warning_bash, warning_inspect;

#ifdef USE_SOUND_2010
	int music_current, musicalt_current, music_monster; //background music currently playing for him/her; an overriding monster music
	int audio_sfx, audio_mus, music_start;
	int sound_ambient;
	/* added for ambient-sfx-handling, so it does not do smooth transition
	   on every wilderness wpos change even though we used WoR instead of walking: */
	bool is_day;
	int ambient_sfx_timer;		/* hack for running through wilderness too quickly for normal ambient sfx to get played */
#endif
	bool cut_sfx_attack, half_sfx_attack, half_sfx_attack_state;
	int count_cut_sfx_attack;
	bool sfx_combat, sfx_magicattack, sfx_defense, sfx_monsterattack, sfx_shriek, sfx_store, sfx_house_quiet, sfx_house, sfx_am;

	/* various flags/counters to check if a character is 'freshly made' and/or has already interacted in certain ways.
	   Mostly to test if he/she is eglibile to join events. */
	bool recv_gold, recv_item, bought_item, killed_mon;

	/* Stuff for new SPECIAL stores and PKT_REQUEST_...;
	   could also be used for quests and neutral monsters. - C. Blue */
	int store_action;		/* What the player is currently doing in a store */
	int request_id, request_type;	/* to keep track of PKT_REQUEST_... requests */
	//ENABLE_GO_GAME:
	unsigned char go_level, go_sublevel, go_level_top, go_hidden_stage;	/* For playing Go (latter two for HIDDEN_STAGE) */
	s32b go_turn; /* for HIDDEN_STAGE when playing Go */
	s16b go_mail_cooldown;

	/* Delayed requests are for quests, to prevent players from spamming password attempts */
	byte delay_str;
	int delay_str_id;
	char delay_str_prompt[MAX_CHARS];
	char delay_str_std[MAX_CHARS];
	byte delay_cfr;
	int delay_cfr_id;
	char delay_cfr_prompt[MAX_CHARS];
	bool delay_cfr_default_choice;

	char reply_name[MAX_CHARS];	/* last player who sent us a private message, for replying */

	int piercing;			/* Rogue skill 'assassinate' */
	bool piercing_charged;

	char last_chat_line[MSG_LEN];	/* last slash command (or chat msg) the player used, to prevent log file spam */
	int last_chat_line_cnt;
	int last_gold_drop, last_gold_drop_timer;

	u32b party_flags, guild_flags;	/* For things like 'Officer' status to add others etc */

	/* SEPARATE_RECALL_DEPTHS */
	byte max_depth[MAX_D_IDX * 2], max_depth_wx[MAX_D_IDX * 2], max_depth_wy[MAX_D_IDX * 2]; /* x2 to account for possible wilderness dungeons */
	bool max_depth_tower[MAX_D_IDX * 2];

	u32b gold_picked_up;		/* for EVENT_TOWNIE_GOLD_LIMIT */
	bool IDDC_found_rndtown;	/* prevent multiple random towns within one 'interval' */
	bool IDDC_logscum;		/* prevent log-scumming instead of proceeding downwards */
	byte IDDC_flags;		/* added for IDDC special hack: Make it easier to find up to two speed rings */
	/* For IDDC_IRON_COOP || IRON_IRON_TEAM : */
	s32b iron_trade;		/* Needed for the last survivor after a party was erased: Former party of the last player who picked it up */

	bool insta_res;			/* Instant resurrection */
	s16b tmp_x, tmp_y;		/* temporary xtra stuff, can be used by whatever */
	bool font_map_solid_walls;	/* Hack: Certain Windows bitmap fonts: Map walls to /127, solid block tile */
	s16b flash_self;
	bool flash_insane;
	bool hilite_player;		/* possible resurrection of long since broken c_cfg.hilite_player: Draw cursor around us at all times. */
	bool consistent_players;	/* Use consistent colouring for player and allies. Ignore all status/body_monster */
#ifdef TELEPORT_SURPRISES
	byte teleported;		/* optional/experimental: in the future, a cooldown for monsters who are 'surprised' from player teleporting next to them */
#endif

	char redraw_cooldown;		/* prevent people spamming CTRL+R (costs cpu+net) */
	bool auto_insc[INVEN_TOTAL];	/* client-side auto-inscribing helper var */
	bool grid_sunlit, grid_house;	/* vampire handling; ambient sfx handling */
	u16b cards_diamonds, cards_hearts, cards_spades, cards_clubs;	/* for /deal and /shuffle commands */

	bool exp_bar;			//just for tracking popularity of this feature..
	int delayed_index, delayed_spell; /* hack: write a spell to command queue, delayed */
	int limit_spells;		/* Limit next spell to a lower level than we could use at best */

#ifdef SOLO_REKING
	int solo_reking, solo_reking_au; /* 1min = 100xp = 250au, up to 5M au, and then another 5M au that cannot be paid off in xp or minutes. */
	time_t solo_reking_laston;	/* since Au is the finest unit, solo_reking vars are measured in Au (0..5M) */
#endif

#ifdef ENABLE_ITEM_ORDER
	int item_order_store, item_order_town, item_order_rarity;
	object_type item_order_forge;
	s64b item_order_cost;
	s32b item_order_turn;
#endif

#if defined(TARGET_SWITCHING_COST) || defined(TARGET_SWITCHING_COST_RANGED)
	s16b tsc_lasttarget, tsc_idle_energy;
#endif

#ifdef ENABLE_MERCHANT_MAIL
	int mail_item;
	s32b mail_gold;
	s32b mail_fee;
	s32b mail_xfee;
	bool mail_COD;
#endif

	int item_newest;
};

typedef struct boni_col boni_col;

struct boni_col {
	/* Index */
	byte i;
	/* Hack signed char/byte values */
	char spd, slth, srch, infr, lite, dig, blow, crit, shot, migh, mxhp, mxmp, luck, pstr, pint, pwis, pdex, pcon, pchr, amfi, sigl;
	/* Flags in char/byte chunks for PKT transfer */
	byte cb[16]; //16 so far, hardcode and check compatibility, ew - Kurzel
	/* Attr + Char */
	char color; char symbol;
};

/* For Monk martial arts */

typedef struct martial_arts martial_arts;
struct martial_arts {
	cptr desc;	/* A verbose attack description */
	int min_level;	/* Minimum level to use */
	int rchance;	/* Reverse chance, lower value means more often */
	int dd;		/* Damage dice */
	int ds;		/* Damage sides */
	int effect;	/* Special effects */
};

/* Define monster generation rules */
typedef struct rule_type rule_type;
struct rule_type {
	byte mode;			/* Mode of combination of the monster flags */
	byte percent;			/* Percentage of monsters added by this rule */

	u32b mflags1;			/* The monster flags that are allowed */
	u32b mflags2;
	u32b mflags3;
	u32b mflags4;
	u32b mflags5;
	u32b mflags6;
	u32b mflags7;
	u32b mflags8;
	u32b mflags9;
	u32b mflags0;

	char r_char[10];			/* Monster race allowed */
};

#ifdef IRONDEEPDIVE_MIXED_TYPES
typedef struct iddc_type iddc_type;
struct iddc_type {
	byte type; //d_info[] index
	byte step; //transition stage
	byte next; //next d_info[] index
};
#endif

/* A structure for the != dungeon types */
typedef struct dungeon_info_type dungeon_info_type;
struct dungeon_info_type {
	u32b name;                      /* Name */
	//int idx;			/* index in d_info.txt */
	u32b text;                      /* Description */
	char short_name[3];             /* Short name */

	s16b feat_boundary;		/* Boundary permanent wall visual */
	s16b floor[5];                    /* Floor tile n */
	s16b floor_percent[5][2];         /* Chance of type n [0]; End chance of type n [1] */
	s16b outer_wall;                /* Outer wall tile */
	s16b inner_wall;                /* Inner wall tile */
	s16b fill_type[5];                /* Cave tile n */
	s16b fill_percent[5][2];          /* Chance of type n [0]; End chance of type n [1] */
	byte fill_method;		/* Smoothing parameter for the above */

	s16b mindepth;                  /* Minimal depth */
	s16b maxdepth;                  /* Maximal depth */

	bool principal;                 /* If it's a part of the main dungeon */
	byte next;                      /* The next part of the main dungeon */
	byte min_plev;                  /* Minimal plev needed to enter -- it's an anti-cheating mesure */

	int min_m_alloc_level;          /* Minimal number of monsters per level */
	int max_m_alloc_chance;         /* There is a 1/max_m_alloc_chance chance per round of creating a new monster */

	u32b flags1;                    /* Flags 1 */
	u32b flags2;                    /* Flags 2 */
	u32b flags3;                    /* Flags 3 */

	byte rule_percents[100];        /* Flat rule percents */
	rule_type rules[10];            /* Monster generation rules */

	int final_object;               /* The object you'll find at the bottom */
	int final_artifact;             /* The artifact you'll find at the bottom */
	int final_guardian;             /* The artifact's guardian. If an artifact is specified, then it's NEEDED */

	int ix, iy, ox, oy;             /* Wilderness coordinates of the entrance/output of the dungeon */

	obj_theme objs;                 /* The drops type */

	int d_dice[4];                  /* Number of dices */
	int d_side[4];                  /* Number of sides */
	int d_frequency[4];             /* Frequency of damage (1 is the minimum) */
	int d_type[4];                  /* Type of damage */

	s16b t_idx[TOWN_DUNGEON];       /* The towns */
	s16b t_level[TOWN_DUNGEON];     /* The towns levels */
	s16b t_num;                     /* Number of towns */
};

/*
 * Hack -- basic town data
 * (In great need of death!)
 */
typedef struct town_extra town_extra;
struct town_extra {
	cptr name;
	byte feat1;
	byte feat2;
	byte ratio;		/* percent of feat1 */
	byte wild_req;	/* On what kind of wilderness this town should be built */
	u16b dungeons[2];	/* Type of dungeon(s) the town contains */
	u16b dun_base;
	u16b dun_max;
	bool tower;		/* TODO: change it, so that a town can have both tower and dungeon */
	u32b flags1;
	u32b flags2;
};


/* Server option struct */

typedef struct server_opts server_opts;
struct server_opts {
	s16b runlevel;		/* Glorified shutdown mode */
	time_t runtime;		/* Server start time */
	time_t closetime;	/* Server closedown time */
	char *meta_address;
	s16b meta_port;
	
	char *bind_name;
	char *console_password;
	char *admin_wizard;
	char *dungeon_master;
	char *wserver;
	
	char *pass;
	s32b preserve_death_level;
	s32b unique_respawn_time;
	s32b unique_max_respawn_time;
	s32b level_unstatic_chance;

	s32b min_unstatic_level;
	s32b retire_timer;
	s32b game_port;
	s32b console_port;
	s32b gw_port;

	s32b spell_interfere;
	s32b spell_stack_limit;
	s16b fps;
	bool players_never_expire;
	s16b newbies_cannot_drop;
	s16b running_speed;

	s16b anti_scum;
	s16b dun_unusual;
	s16b town_x;
	s16b town_y;
	s16b town_base;

	s16b dun_base;
	s16b dun_max;
	s16b store_turns;
	s16b dun_store_turns;
	char resting_rate;
	char party_xp_boost;

	char use_pk_rules;
	char quit_ban_mode;
	char zang_monsters;
	char pern_monsters;
	char cth_monsters;

	char joke_monsters;
	char cblue_monsters;
	char vanilla_monsters;
	char pet_monsters;
	bool report_to_meta;
	bool secret_dungeon_master;

	bool anti_arts_hoard;
	bool anti_arts_house;
	bool anti_arts_wild;
	bool anti_arts_shop;
	bool anti_arts_pickup;
	bool anti_arts_send;
	bool persistent_artifacts;

	bool anti_cheeze_pickup;
	bool anti_cheeze_telekinesis;
	s16b surface_item_removal; /* minutes before items are erased */
	s16b dungeon_item_removal; /* minutes before items are erased */
	u16b death_wild_item_removal; /* minutes before items are erased */
	u16b long_wild_item_removal; /* minutes before items are erased */
	s16b dungeon_shop_chance;
	s16b dungeon_shop_type;
	s16b dungeon_shop_timeout;

	bool mage_hp_bonus;	/* DELETEME (replace it, that is) */
	char door_bump_open;
	bool no_ghost;
	int lifes;		/* number of times a ghost player can be resurrected */
	int houses_per_player;	/* number of houses a player is allowed to own at once;
				    it's: max_houses = (player_level / houses_per_player). */
	int castles_per_player; /* absolute # of castles a character may own (0 for infinite) */
	bool castles_for_kings;
	int acc_house_limit;
	bool maximize;
	bool kings_etiquette;
	bool fallenkings_etiquette;
	bool strict_etiquette;

	bool public_rfe;
	bool auto_purge;
	bool log_u;
	s16b replace_hiscore;
	s16b unikill_format;
	char *server_notes;
	bool arts_disabled;
	bool winners_find_randarts;
	s16b arts_level_req;
	bool surface_summoning;
	s16b clone_summoning;
	s16b henc_strictness;
	s16b bonus_calc_type;		/* The way hit points are calculated (0 = traditional, 1 = modern) */
	s16b charmode_trading_restrictions; /* how restricted is trading between everlating and non-everlasting players */
	s16b item_awareness;	/* How easily the player becomes aware of unknown items (id scroll/shop/..)-C. Blue */
	bool worldd_pubchat, worldd_privchat, worldd_broadcast, worldd_lvlup, worldd_unideath, worldd_pwin, worldd_pdeath, worldd_pjoin, worldd_pleave, worldd_plist, worldd_events;//worldd_ircchat;
};

/* Client option struct */
/* Consider separate it into client/types.h and server/types.h */
typedef struct client_opts client_opts;
struct client_opts {
    //page 1
	bool rogue_like_commands;
	bool newbie_hints;
	bool censor_swearing;
	bool hilite_chat;
	bool hibeep_chat;
	bool page_on_privmsg;
	bool page_on_afk_privmsg;
	bool big_map;
	bool font_map_solid_walls;
	bool view_animated_lite;
	bool wall_lighting;
	bool view_lamp_walls;
	bool view_shade_walls;
	bool floor_lighting;
	bool view_lamp_floor;
	bool view_shade_floor;
	bool view_lite_extra;
	bool alert_hitpoint;
	bool alert_mana;
	bool alert_afk_dam;
	bool alert_offpanel_dam;
	bool exp_bar;
    //page 2
	bool uniques_alive;
	bool warn_unique_credit;
	bool limit_chat;
	bool no_afk_msg;
	bool overview_startup;
	bool allow_paging;
	bool ring_bell;
	bool linear_stats;
	bool exp_need;
	bool depth_in_feet;
	bool newb_suicide;
	bool show_weights;
	bool time_stamp_chat;
	bool hide_unusable_skills;
	bool short_item_names;
	bool keep_topline;
	bool target_history;
	bool taciturn_messages;
	bool always_show_lists;
	bool no_weather;
	bool player_list;
	bool player_list2;

    //page 3
	bool flash_player;
	bool hilite_player;
	bool consistent_players;
	bool recall_flicker;
	bool no_verify_destroy;
	bool no_verify_sell;

    //page 5
	bool auto_afk;
	bool idle_starve_kick;
	bool safe_float;
	bool safe_macros;
	bool auto_untag;
	bool clear_inscr;
	bool auto_inscribe;
	bool stack_force_notes;
	bool stack_force_costs;
	bool stack_allow_items;
	bool stack_allow_wands;
	bool whole_ammo_stack;
	bool always_repeat;
	bool always_pickup;
	bool use_old_target;
	bool autooff_retaliator;
	bool fail_no_melee;
	bool wide_scroll_margin;
	bool auto_target;
	bool thin_down_flush;
	bool disable_flush;

    //page 6
	bool find_ignore_stairs;
	bool find_ignore_doors;
	bool find_cut;
	bool find_examine;
	bool disturb_move;
	bool disturb_near;
	bool disturb_panel;
	bool disturb_state;
	bool disturb_minor;
	bool disturb_other;
	bool view_perma_grids;
	bool view_torch_grids;
	bool view_reduce_lite;
	bool view_reduce_view;
	bool easy_open;
	bool easy_disarm;
	bool easy_tunnel;

    //page 4
	bool audio_paging;
	bool paging_master_volume;
	bool paging_max_volume;
	bool no_ovl_close_sfx;
	bool ovl_sfx_attack;
	bool no_combat_sfx;
	bool no_magicattack_sfx;
	bool no_defense_sfx;
	bool half_sfx_attack;
	bool cut_sfx_attack;
	bool ovl_sfx_command;
	bool ovl_sfx_misc;
	bool ovl_sfx_mon_attack;
	bool ovl_sfx_mon_spell;
	bool ovl_sfx_mon_misc;
	bool no_monsterattack_sfx;
	bool no_shriek_sfx;
	bool no_store_bell;
	bool quiet_house_sfx;
	bool no_house_sfx;
	bool no_am_sfx;

    //unmutable, pfft
	bool use_color;
	bool other_query_flag;

    //deprecated/broken/todo
#if 0
	bool quick_messages;
	bool carry_query_flag;
	bool show_labels;
	bool show_choices;
	bool show_details;
	bool expand_look;
	bool expand_list;
	bool avoid_other;
	bool flush_failure;
	bool flush_disturb;
	bool fresh_after;
	bool auto_destroy;
	bool last_words;
	bool speak_unique;

	//additional stuff
	bool auto_scum;
	bool flush_command;
	bool fresh_before;
	bool auto_haggle;
	bool flow_by_sound;
	bool flow_by_smell;
	bool dungeon_stair;
	bool smart_learn;
	bool smart_cheat;
	bool alert_failure;
	bool dungeon_align;
	bool avoid_abort;
	bool compress_savefile;
#endif

    //new additions
	bool shuffle_music;
	bool permawalls_shade;
	bool topline_no_msg;
	bool targetinfo_msg;
	bool live_timeouts;
	bool flash_insane;

	bool last_words;
	bool disturb_see;
};

/*
 * Extra information on client-side that the server player_type
 * doesn't contain.		- Jir -
 *
 * Most variables in client/variable.c should be bandled here maybe.
 */
typedef struct c_player_extra c_player_extra;
struct c_player_extra {
	char body_name[MAX_CHARS];	/* Form of Player */
	char sanity[10];	/* Sanity strings */
	byte sanity_attr;	/* Colour to display sanity */
	char location_name[20];	/* Name of location (eg. 'Bree') */
};

typedef struct c_store_extra c_store_extra;
struct c_store_extra {
	char owner_name[40];
	char store_name[40];
	s32b max_cost;			/* Purse limit */

	/* list of command */
	u16b actions[6];                /* Actions(refers to ba_info) */
	u16b bact[6];                /* ba_ptr->action */
	char action_name[6][40];
	char action_attr[6];
	u16b action_restr[6];
	char letter[6];
	s16b cost[6];
	byte flags[6];

	/* Store attr and char */
	byte store_attr;
	char store_char;
};

/* from spells1.c */
typedef int (*inven_func)(object_type *);

typedef struct hooks_chain hooks_chain;
struct hooks_chain {
	char name[40];
	char script[40];
	hooks_chain *next;
};

typedef union hook_return hook_return;
union hook_return {
	s32b num;
	char *str;
	object_type *o_ptr;
};

/*
 * The spell function must provide the desc
 */
typedef struct spell_type spell_type;
struct spell_type {
	cptr name;                      /* Name */
	byte skill_level;               /* Required level (to learn) */
	byte mana;			/* Required mana at lvl 1 */
	byte mana_max;			/* Required mana at max lvl */
	byte fail;			/* Minimum chance of failure */
	s16b level;                     /* Spell level(0 = not learnt) */
	byte spell_power;		/* affected by spell-power skill? */
};

typedef struct school_type school_type;
struct school_type {
	cptr name;                      /* Name */
	s16b skill;                     /* Skill used for that school */
};

/* C. Blue - don't confuse with xorder_type, which is for the basic kill '/xorder'.
   This is more of a global event, first use will be automated Highlander Tournament
   schedule. Timing is possible too. Might want to make use of AT_... sequences. */
typedef struct global_event_type global_event_type;
struct global_event_type {
	int getype;			/* Type of the event (or quest) */
	bool paused;		/* Is the event currently paused? (special admin command) */
	s32b paused_turns;		/* Keeps track of turns the event was actually frozen */
	s32b state[64];		/* progress (zero'ed on event start) */
	s32b extra[64];		/* extra info (zero'ed on event start) */
	s32b participant[MAX_GE_PARTICIPANTS];	/* player IDs */
	s32b creator;       	/* Player ID or 0L */
	long int announcement_time;	/* for how many seconds the event will be announced until it actually starts */
	long int signup_time;	/* for how many seconds the event will allow signing up:
				   -1 = this event doesn't allow signing up at all!
				   0 = same as announcement_time, ie during the announcement phase
				   >0 = designated time instead of announcement_time. */
	bool first_announcement;	/* just keep track of first advertisement, and add additional info that time */
	s32b start_turn;          	/* quest started */
	s32b end_turn;		/* quest will end */
	time_t started;		/* quest started */
	time_t ending;		/* quest will end */
	char title[64];		/* short title of this event (used for /gesign <n> player command) */
	char description[10][78];	/* longer event description */
	bool hidden;		/* hidden from the players? */
	int min_participants;	/* minimum amount of participants */
	int limited;		/* limited amount of participants? (smaller than MAX_GE_PARTICIPANTS) */
	int cleanup;		/* what kind of cleaning-up is required when event ends (state=255) ? */
	bool noghost;		/* event will erase character on failure */
};

/* Runecraft */
typedef struct r_element r_element;
struct r_element {
	u16b flag;
	char * name;
	u16b skill;
};
typedef struct r_imperative r_imperative;
struct r_imperative {
	u16b flag;
	char * name;
	byte level;
	byte cost;
	s16b fail;
	byte damage;
	s16b radius;
	byte duration;
	byte energy;
};
typedef struct r_type r_type;
struct r_type {
	u16b flag;
	char * name;
	byte level;
	byte c_min;
	byte c_max;
	byte d1min;
	byte d2min;
	byte d1max;
	byte d2max;
	byte dbmin;
	u16b dbmax;
	byte r_min;
	byte r_max;
	byte d_min;
	byte d_max;
};
typedef struct r_projection r_projection;
struct r_projection {
	u16b flags;
	int gf_type;
	int weight;
	char * name;
	u32b resist;
};

/* Auction system - mikaelh */
typedef struct bid_type bid_type;
struct bid_type {
	s32b		bid;
	s32b		bidder;
};

typedef struct auction_type auction_type;
struct auction_type {
	byte		status;			/* Status: setup, bidding, finished or cancelled */
	byte		flags;			/* Flags: payments */
	byte		mode;			/* Owner mode: Non-everlasting or everlasting */
	s32b		owner;			/* Owner */
	object_type	item;			/* Auctioned item */
	char		*desc;			/* Item description */
	s32b		starting_price;		/* Starting price */
	s32b		buyout_price;		/* Buy-out price */
	s32b		bids_cnt;		/* Number of bids */
	bid_type	*bids;
	s32b		winning_bid;		/* The winning bid (after bidding is over) */
	time_t		start;
	time_t		duration;
};

#ifdef MONSTER_ASTAR		/* A* path finding - C. Blue */
typedef struct astar_node astar_node;
struct astar_node {
	int x, y; /* floor grids. Unsigned char would do too */
	int F, G, H;
	int parent_idx;
	/* we don't want to rearrange nodes when we delete one, because other nodes might have
	   remembered our index as their parent, so we just use an inverse 'deleted' marker aka 'in_use' instead */
	bool in_use;
};
typedef struct astar_list_open astar_list_open;
struct astar_list_open {
	int m_idx; /* monster which currently uses this index in the available A* arrays, or -1 for 'unused' ie available */
	int nodes; /* current amount of nodes stored in this list */
	astar_node node[ASTAR_MAX_NODES];
 #ifdef ASTAR_DISTRIBUTE
	int result;
 #endif
};
typedef struct astar_list_closed astar_list_closed;
struct astar_list_closed {
	int nodes; /* current amount of nodes stored in this list */
	astar_node node[ASTAR_MAX_NODES];
};
#endif

#ifdef USE_SOUND_2010
//main.h: (from angband)
struct module {
	cptr name;
	cptr help;
	errr (*init)(int argc, char **argv);
};
#endif

/* for (currently hardcoded client-side) mimic spells, to enable proper targetting */
typedef struct monster_spell_type {
	cptr name;
	bool uses_dir; /* flag */
} monster_spell_type;


/* The struct to hold a data entry */
typedef struct hash_entry hash_entry;
struct hash_entry {
	int id;				/* The character ID */
	u32b account;			/* account id */
	cptr accountname;		/* NOTE: this value is NOT loaded/saved but fetched live on each server startup */
	cptr name;			/* Player name */
	byte race,class;		/* Race/class */
	byte admin;
	struct worldpos wpos;

	/* new in savegame version 4.2.2 (4.2.0c server) - C. Blue */
	byte mode;			/* Character mode (for account overview screen) */

	/* new in 3.4.2 */
	byte level;			/* Player maximum level */
	/* changed from byte to u16b - mikaelh */
	u16b party;			/* Player party */
	/* 3.5.0 */
	byte guild;			/* Player guild */
	u32b guild_flags;		/* 4.5.2.0.0.1 */
	s16b xorder;			/* Extermination order */

	time_t laston;			/* Last on time */

#ifdef AUCTION_SYSTEM
	s32b au;
	s32b balance;
#endif

	char houses; // ACC_HOUSE_LIMIT
	byte winner;

	struct hash_entry *next;	/* Next entry in the chain */
};
