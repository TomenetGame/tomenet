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

struct header
{
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

struct worldpos
{
	s16b wx;	/* west to east */
	s16b wy;	/* south to north */
	s16b wz;	/* deep to sky */
};

/* worldspot adds exact x and y coordinates */
typedef struct worldspot worldspot;

struct worldspot
{
	struct worldpos wpos;
	s16b x;
	s16b y;
};

/* cavespot consists of coordinates within a cave */
typedef struct cavespot cavespot;

struct cavespot
{
	s16b x;
	s16b y;
};

#if 0
/*
 * Following are rough sketches for versions(maybe v5) to come.	- Jir -
 */
/* TODO: tweak it in a way so that one wilderness field can contain
 * more than one dungeons/towers/rooms (eg.quest, arena..)
 *
 * probably, wild_info will contain array of pointers instead of
 * 'cave', 'tower' and 'dungeon', and then functions like getcave will
 * look up for a proper 'dungeon'.
 *
 * wpos={32,32,-20,2} => wild_info[32][32].dungeons[2].level[19]
 *
 * dungeons[] contains the info whether it's tower/dungeon or whatever,
 * and maybe dungeons[0] always is the surface.
 *
 * To tell which dungeon is which, we should use 'id' maybe.
 */
struct worldpos
{
	s16b wx;	/* west to east */
	s16b wy;	/* south to north */
	s16b wz;	/* deep to sky */
	s16b ww;	/* 4th dimention! actually it's index to the dungeons */
};

struct wilderness_type
{
	u16b radius; /* the distance from the town */
	u16b type;   /* what kind of terrain we are in */

	u32b flags; /* various */
	struct dungeon_type *dungeons;
	s32b own;	/* King owning the wild */

	/* for sector-by-sector (instead of global) client-side weather,
	   we have more distinct possibilities easily now: */
	int weather_type; /* stop(-1), none(0), rain(1), snow(2) */
	int weather_wind; /* none(0), west(odd), east(even) - the higher the stronger */
	int weather_intensity; /* [0] modify amount of elements cast in parallel */
	int weather_speed; /* 'vertical wind', makes sense for snowflakes only: how fast they drop */
};

typedef struct dungeon_type dungeon_type;
struct dungeon_type
{
	u16b id;		/* dungeon id */
	u16b baselevel;		/* base level (1 - 50ft etc). */

	/* maybe contains info about if tower/dungeon/building etc */
	u32b flags1;		/* dungeon flags */
	u32b flags2;		/* DF2 flags */

	byte maxdepth;		/* max height/depth */
	char r_char[10];	/* races allowed */
	char nr_char[10];	/* races prevented */
	struct dun_level *level;	/* array of dungeon levels */
	byte ud_y, ud_x		/* location of entrance */
};


/*
 * No work is done yet for this.
 * Probably, we should use not 'getlevel' but actual 'wz'
 * to determine the max depth a player can recall to.
 *
 * XXX some dungeons/towers can be added afterwards;
 * we should either allocate bigger array in advance, or make it
 * reallocatable.
 */
typedef struct recall_depth recall_depth;

struct recall_depth
{
	u16b id;	/* dungeon id (see dungeon_type) */
	s16b depth;	/* max recall-depth */
}
#endif	/* 0 */


/*
 * "Themed" objects.
 * Probability in percent for each class of objects to be dropped.
 * This could perhaps be an array - but that wouldn't be as clear.
 */
/* Borrowed from ToME	- Jir - */
typedef struct obj_theme obj_theme;
struct obj_theme
{
	byte treasure;
	byte combat;
	byte magic;
	byte tools;
};


/*
 * Information about terrain "features"
 */

typedef struct feature_type feature_type;

struct feature_type
{
	u16b name;			/* Name (offset) */
	u16b text;			/* Text (offset) */
#if 1
	u32b tunnel;            /* Text for tunneling */
	u32b block;             /* Text for blocking */

	u32b flags1;            /* First set of flags */
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

struct object_kind
{
	u16b name;			/* Name (offset) */
	u16b text;			/* Text (offset) */

	byte tval;			/* Object type */
	byte sval;			/* Object sub type */

	s16b pval;			/* Object extra info */

	s16b to_h;			/* Bonus to hit */
	s16b to_d;			/* Bonus to damage */
	s16b to_a;			/* Bonus to armor */

	s16b ac;			/* Base armor */

	byte dd, ds;		/* Damage dice/sides */

	s16b weight;		/* Weight */

	s32b cost;			/* Object "base cost" */

	u32b flags1;		/* Flags, set 1 */
	u32b flags2;		/* Flags, set 2 */
	u32b flags3;		/* Flags, set 3 */
        u32b flags4;            /* Flags, set 4 */
        u32b flags5;            /* Flags, set 5 */


	byte locale[4];		/* Allocation level(s) */
	byte chance[4];		/* Allocation chance(s) */

	byte level;			/* Level */
	byte extra;			/* Something */


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

        u32b esp;                       /* ESP flags */
#if 0
        byte btval;                     /* Become Object type */
        byte bsval;                     /* Become Object sub type */

        s16b power;                     /* Power granted(if any) */
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

struct artifact_type
{
	u16b name;			/* Name (offset) */
	u16b text;			/* Text (offset) */

	byte tval;			/* Artifact type */
	byte sval;			/* Artifact sub type */

	s16b pval;			/* Artifact extra info */

	s16b to_h;			/* Bonus to hit */
	s16b to_d;			/* Bonus to damage */
	s16b to_a;			/* Bonus to armor */

	s16b ac;			/* Base armor */

	byte dd, ds;		/* Damage when hits */

	s16b weight;		/* Weight */

	s32b cost;			/* Artifact "cost" */

	u32b flags1;		/* Artifact Flags, set 1 */
	u32b flags2;		/* Artifact Flags, set 2 */
	u32b flags3;		/* Artifact Flags, set 3 */
        u32b flags4;            /* Artifact Flags, set 4 */
        u32b flags5;            /* Artifact Flags, set 5 */

	byte level;			/* Artifact level */
	byte rarity;		/* Artifact rarity */

	byte cur_num;		/* Number created (0 or 1) */
	byte max_num;		/* Unused (should be "1") */
        u32b esp;                       /* ESP flags */
#if 0

        s16b power;                     /* Power granted(if any) */

        s16b set;               /* Does it belongs to a set ?*/
#endif	/* 0 */

	bool known;			/* Is this artifact already IDed? */
};


/*
 * Information about "ego-items".
 */

typedef struct ego_item_type ego_item_type;

struct ego_item_type
{
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
        u32b esp[5];                       /* ESP flags */
        u32b fego[5];                       /* ego flags */

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

struct monster_blow
{
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

struct monster_race
{
	u16b name;				/* Name (offset) */
	u16b text;				/* Text (offset) */

	u16b hdice;				/* Creatures hit dice count */
	u16b hside;				/* Creatures hit dice sides */

	s16b ac;				/* Armour Class */

	s16b sleep;				/* Inactive counter (base) */
	byte aaf;				/* Area affect radius (1-100) */
	byte speed;				/* Speed (normally 110) */

	s32b mexp;				/* Exp value for kill */

	s32b weight;		/* Weight of the monster */
	s16b extra;				/* Unused (for now) */

	byte freq_inate;		/* Inate spell frequency */
	byte freq_spell;		/* Other spell frequency */

	u32b flags1;			/* Flags 1 (general) */
	u32b flags2;			/* Flags 2 (abilities) */
	u32b flags3;			/* Flags 3 (race/resist) */
	u32b flags4;			/* Flags 4 (inate/breath) */
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

	s16b level;				/* Level of creature */
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

	byte r_cast_inate;		/* Max number of inate spells seen */
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
};



/*
 * Information about "vault generation"
 */

typedef struct vault_type vault_type;

struct vault_type
{
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

struct swear{
	char word[30];
	int level;
};

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

#if 1 /* Evileye - work in progress */

/* Cave special types */
/* TODO: move those defines to defines.h */
#define CS_NONE		0
#define CS_DNADOOR	1
#define CS_KEYDOOR	2
#define CS_TRAPS	3 	/* CS stands for Cave Special */
#define CS_INSCRIP	4	/* ok ;) i'll follow that from now */

#define CS_FOUNTAIN	5
#define CS_BETWEEN	6	/* petit jump type */
#define CS_BETWEEN2	7	/* world traveller type */
#define CS_MON_TRAP	8	/* monster traps */
#define CS_SHOP		9	/* shop/building */
#define CS_MIMIC	10	/* mimic-ing feature (eg. secret door) */

/* heheh it's kludge.. */
#define sc_is_pointer(type)	(type < 3 || type == 4 || type > 10)

typedef struct c_special c_special;

struct c_special{
	unsigned char type;
	union	/* 32bits */
	{
		void *ptr;		/* lazy - refer to other arrays or sth */
		s32b omni;		/* needless of other arrays? k, add here! */
		struct { byte t_idx; bool found; } trap;
		struct { byte fy, fx; } between; /* or simply 'dpos'? */
		struct { byte wx, wy; s16b wz; } wpos;	/* XXX */
		struct { byte type, rest; bool known; } fountain;
		struct { u16b trap_kit; byte difficulty, feat; } montrap;
	} sc;
	struct c_special *next;
};

typedef struct cave_type cave_type;

/* hooks structure containing calls for specials */
struct sfunc{
	void (*load)(c_special *cs_ptr);		/* load function */
	void (*save)(c_special *cs_ptr);		/* save function */
	void (*see)(c_special *cs_ptr, char *c, byte *a, int Ind);	/* sets player view */
	int (*activate)(c_special *cs_ptr, int y, int x, int Ind);	/* walk on/bump */
};
#endif

struct cave_type
{
	u16b info;		/* Hack -- cave flags */
	byte feat;		/* Hack -- feature type */
	byte feat_org;		/* Feature type backup */
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

#if 0 /* since monsters might track different players, with paths leading over same grids though, I'm adding this to astar_list instead - C. Blue */
#ifdef MONSTER_ASTAR
	int astarF, astarG, astarH; /* grid score (F=G+H), starting point distance cost, estimated goal distance cost */
#endif
#endif

	struct c_special *special;	/* Special pointer to various struct */

	/* I don't really love to enlarge cave_type ... but it'd suck if
	 * trapped floor will be immune to Noxious Cloud */
	/* Adding 1byte in this struct costs 520Kb memory, in average */
	/* This should be replaced by 'stackable c_special' code -
	 * let's wait for evileye to to this :)		- Jir - */
	int effect;            /* The lasting effects */

#ifdef HOUSE_PAINTING
	int colour;	/* colour that overrides the usual colour of a feature */
#endif
};

/* ToME parts, arranged */
/* This struct can be enlarged to handle more generic timed events maybe? */
/* Lasting spell effects(clouds, ..) */
typedef struct effect_type effect_type;
struct effect_type
{
	s16b	interval;	/* How quickly does it tick (10 = normal, once per 10 frames at 0 ft depth) */
	s16b    time;           /* For how long */
	s16b    dam;            /* How much damage */
	s16b    type;           /* Of which type */ /* GF_XXX, for now */
	s16b    cy;             /* Center of the cast*/
	s16b    cx;             /* Center of the cast*/
	s16b    rad;            /* Radius -- if needed *//* Not used? */
	u32b    flags;          /* Flags */

	s32b	who;			/* Who caused this effect (0-id if player) */
	worldpos wpos;			/* Where in the world */
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
 */

typedef struct object_type object_type;

struct object_type
{
        s32b owner;                     /* Player that found it */
        byte mode;                	/* Mode of player who found it */
        s16b level;                     /* Level req */
			/* Hack -- ego-items use 'level' in a special way, and
			 * altering this value will result in change of ego-item
			 * powers themselves!	- Jir -
			 */

	s16b k_idx;			/* Kind index (zero if "dead") */

	byte iy;			/* Y-position on map, or zero */
	byte ix;			/* X-position on map, or zero */

	struct worldpos wpos;

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
#if 1 /* don't exist in the code at all, except load/save */
	s32b pval4;			/* Item extra-parameter for some special items */
	s32b pval5;			/* Item extra-parameter for some special items */
#endif

	byte discount;		/* Discount (if any) */

	byte number;		/* Number of items */

	s16b weight;		/* Item weight */

	u16b name1;			/* Artifact type, if any */
	u16b name2;			/* Ego-Item type, if any */
	u16b name2b;			/* 2e Ego-Item type, if any */
	s32b name3;			/* Randart seed, if any (now it's common with ego-items -Jir-) */
	u16b name4;			/* Index of randart name in file 'randarts.txt', solely for fun set bonus - C. Blue */
	byte attr;			/* colour in inventory (for client) */

	byte xtra1;			/* Extra info type, for various purpose */
	byte xtra2;			/* Extra info index */
	/* more info added for self-made spellbook feature Adam suggested - C. Blue */
	byte xtra3;			/* Extra info */
	byte xtra4;			/* Extra info */
	byte xtra5;			/* Extra info */
	byte xtra6;			/* Extra info */
	byte xtra7;			/* Extra info */
	byte xtra8;			/* Extra info */
	byte xtra9;			/* Extra info */

#ifdef PLAYER_STORES
	byte ps_idx_x;			/* Index or x-coordinate of player store item in the original house */
	byte ps_idx_y;			/* y-coordinate of player store item in the original house */
#endif

	s16b to_h;			/* Plusses to hit */
	s16b to_d;			/* Plusses to damage */
	s16b to_a;			/* Plusses to AC */

	s16b ac;			/* Normal AC */

	byte dd, ds;			/* Damage dice/sides */

	s32b timeout;			/* Timeout Counter */

	u16b ident;			/* Special flags  */

	s32b marked;			/* Object is marked (for deletion after a certain time) */
	byte marked2;			/* additional parameters */

	u16b note;			/* Inscription index */
	char note_utag;			/* Added for making pseudo-id overwrite unique loot tags */

	char uses_dir;			/* Client-side: Uses a direction or not? (for rods) */

#if 0	/* from pernA.. consumes memory, but quick. shall we? */
        u16b art_name;			/* Artifact name (random artifacts) */

        u32b art_flags1;        	/* Flags, set 1  Alas, these were necessary */
        u32b art_flags2;        	/* Flags, set 2  for the random artifacts of*/
        u32b art_flags3;        	/* Flags, set 3  Zangband */
        u32b art_flags4;        	/* Flags, set 4  PernAngband */
        u32b art_flags5;        	/* Flags, set 5  PernAngband */
        u32b art_esp;           	/* Flags, set esp  PernAngband */
#endif	/* 0 */

	byte inven_order;		/* Inventory position if held by a player,
					   only use is in xtra2.c when pack is ang_sort'ed */

	u16b next_o_idx;		/* Next object in stack (if any) */
	u16b held_m_idx;		/* Monster holding us (if any) */
	char stack_pos;			/* Position in stack: Use to limit stack size */

	s16b cheeze_dlv, cheeze_plv, cheeze_plv_carry;	/* anti-cheeze */

	bool changed;		/* dummy flag to refresh item if o_name changed, but memory copy didn't */
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
 * Note: fy, fx constrain dungeon size to 256x256
 */

typedef struct monster_type monster_type;

struct monster_type
{
   byte pet;			/* Special pet value (not an ID). 0 = not a pet. 1 = is a pet. */
   bool special;                   /* Does it use a special r_info ? */
   monster_race *r_ptr;            /* The aforementionned r_info */

   s32b owner;                     /* id of Player owning it */

   s16b r_idx;			/* Monster race index */

   byte fy;			/* Y location on map */
   byte fx;			/* X location on map */

   struct worldpos wpos;

   s32b exp;                       /* Experience of the monster */
   s16b level;                     /* Level of the monster */

   monster_blow blow[4];        /* Up to four blows per round */
   byte speed;			/* ORIGINAL Monster "speed" (gets copied from r_ptr->speed on monster placement) */
   byte mspeed;			/* CURRENT Monster "speed" (is set to 'speed' initially on monster placement) */
   s16b ac;                     /* Armour Class */
   s16b org_ac;               	/* Armour Class */

   s32b hp;                        /* Current Hit points */
   s32b maxhp;                     /* Max Hit points */
   s32b org_maxhp;                 /* Max Hit points */

   s16b csleep;		/* Inactive counter */

   s16b energy;		/* Monster "energy" */

   byte monfear;		/* Monster is afraid */
   byte confused;		/* Monster is confused */
   byte stunned;		/* Monster is stunned */
   byte paralyzed;		/* Monster is paralyzed (unused) */
   byte bleeding;		/* Monster is bleeding (unused) */
   byte poisoned;		/* Monster is poisoned (unused) */
   byte blinded;		/* monster appears confused (unused: wrapped as confusion currently) */
   byte silenced;		/* monster can't cast spells for a short time (for new mindcrafters) */
   int charmedignore;		/* monster is charmed in a way that it ignores players */

   u16b hold_o_idx;	/* Object being held (if any) */

   s16b cdis;			/* Current dis from player */

   /*	bool los;*/			/* Monster is "in sight" */
   /*	bool ml;*/			/* Monster is "visible" */

   s16b closest_player;	/* The player closest to this monster */

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

   s16b mind;                      /* Current action -- golems */

   #ifdef RANDUNIS
   u16b ego;                       /* Ego monster type */
   s32b name3;			/* Randuni seed, if any */
   #endif
   #if 0
   s16b status;		/* Status(friendly, pet, companion, ..) */
   s16b target;		/* Monster target */
   s16b possessor;		/* Is it under the control of a possessor ? */
   #endif

   u16b ai_state;		/* What special behaviour this monster takes now? */
   s16b last_target;		/* For C. Blue's anti-cheeze C_BLUE_AI in melee2.c */
   s16b last_target_melee;	/* For C. Blue's C_BLUE_AI_MELEE in melee2.c */
   s16b last_target_melee_temp;	/* For C. Blue's C_BLUE_AI_MELEE in melee2.c */
   s16b switch_target;		/* For distract_monsters(), implemented within C_BLUE_AI_MELEE in melee2.c */

   s16b cdis_on_damage;		/* New Ball spell / explosion anti-cheeze */
//   byte turns_tolerance;	/* Optional: How many turns pass until we react the new way */
   s16b damage_tx, damage_ty;	/* new temporary target position: where we received damage from */
   signed char previous_direction;	/* Optional: Don't move right back where we came from (at least during this turn -_-) after reaching the damage epicentrum. */
   s16b damage_dis;		/* Remember distance to epicenter */
   s16b p_tx, p_ty;		/* Coordinates from where the player cast the damaging projection */

   s16b highest_encounter;	/* My final anti-cheeze strike I hope ;) - C. Blue
      This keeps track of the highest player which the monster
      has 'encountered' (might offer various definitions of this
      by different #defines) and adjusts its own experience value
      towards that player, so low players who get powerful help
      will get less exp out of it. */
//   s16b highest_encounter_mlvl;	/* lol.. */
   byte backstabbed;	/* has this monster been backstabbed from cloaking mode already? prevent exploit */
   byte taunted;	/* has this monster been taunted (melee technique)? */

   bool no_esp_phase;	/* for WEIRD_MIND esp flickering */
   int extra;	/* extra flag for debugging/testing purpose; also used for target dummy's "snowiness" now */

#ifdef MONSTER_ASTAR
    int astar_idx;		/* index in available A* arrays. A* is expensive, so we only provide a couple of instances for a few monsters to use */
#endif
};

typedef struct monster_ego monster_ego;

struct monster_ego
{
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

	byte freq_inate;                /* Inate spell frequency */
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
	u32b mflags4;                    /* Flags 4 (inate/breath) */
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
	u32b nflags4;                    /* Flags 4 (inate/breath) */
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

struct alloc_entry
{
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

struct setup_t
{
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

struct client_setup_t
{
	bool options[OPT_MAX];

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

struct option_type
{
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

struct store_type
{
	u16b st_idx;

	u16b owner;                     /* Owner index */

#ifdef PLAYER_STORES
	u32b player_owner;		/* Temporary value for player's id */
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

typedef struct quest quest;

struct quest
{
	int level;		/* Dungeon level */
	int r_idx;		/* Monster race */

	int cur_num;	/* Number killed (unused) */
	int max_num;	/* Number required (unused) */
};

/* Quests, random or preset by the dungeon master */

#if 0
/* Quest prize types */
#define QPRIZE_CASH 1
#define QPRIZE_GOOD 2
#define QPRIZE_EXC 3
#define QPRIZE_ART 4	/* Must be rare, hard, and only ever by DM */

struct quest_type{
	s16b id;		/* Quest ID. Value of 0 means inactive */
	byte type;		/* Quest type */
	byte prize;		/* Prize type. */

};

#endif

/* Quest types */
#define QUEST_RANDOM 1	/* Random quest, not set by the DM */
#define QUEST_MONSTER 2 /* Kill some normal monsters. */
#define QUEST_UNIQUE 4  /* Kill unique monster (unkilled by players) */
#define QUEST_OBJECT 8	/* Find some object. Must not be owned, or found
			   in the town. */
#define QUEST_RACE 16	/* Race quest - ie, not personal */
#define QUEST_GUILD 32	/* Guildmaster's quest (no prize) */

/* evileye - same as above, but multiplayerized. */
struct quest_type{
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
struct dun_level
{
	int ondepth;
	time_t lastused;
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
};

/* dungeon_type structure
 *
 * Filter for races is not strict. It shall alter the probability.
 * (consider using rule_type	- Jir -)
 */
typedef struct dungeon_type dungeon_type;
struct dungeon_type
{
	u16b id;		/* dungeon id */
	u16b type;		/* dungeon type (of d_info) */
	u16b baselevel;		/* base level (1 - 50ft etc). */
	u32b flags1;		/* dungeon flags */
	u32b flags2;		/* DF2 flags */
	byte maxdepth;		/* max height/depth */
#if 0
	rule_type rules[5];	/* Monster generation rules */
	char r_char[10];	/* races allowed */
	char nr_char[10];	/* races prevented */
#endif	/* 0 */
	struct dun_level *level;	/* array of dungeon levels */
};

/*
 * TODO:
 * - allow towns in the dungeons/towers
 * - allow towns to have dungeon flags(DFn_*)
 */
struct town_type
{
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
};

typedef struct wilderness_type wilderness_type;

struct wilderness_type
{
	u16b radius;	/* the distance from the town */
	u16b type;	/* what kind of terrain we are in */
	byte town_idx;	/* Which town resides exactly in this sector? */

	u32b flags;	/* various */
	struct dungeon_type *tower;
	struct dungeon_type *dungeon;
	s16b ondepth;
	time_t lastused;
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
};


/*
 * A store owner
 */

typedef struct owner_type owner_type;

struct owner_type
{
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

struct store_info_type
{
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

struct store_action_type
{
	u32b name;                      /* Name (offset) */

	s16b costs[3];                  /* Costs for liked people */
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

struct magic_type
{
	byte slevel;		/* Required level (to learn) */
	byte smana;			/* Required mana (to cast) */
	byte sfail;			/* Minimum chance of failure */
	byte sexp;			/* Encoded experience bonus */
};


/*
 * Information about the player's "magic"
 *
 * Note that a player with a "spell_book" of "zero" is illiterate.
 */

typedef struct player_magic player_magic;

struct player_magic
{
	s16b spell_book;		/* Tval of spell books (if any) */

	s16b spell_stat;		/* Stat for spells (if any)  */

        magic_type info[64];	/* The available spells */
};



/*
 * Player racial info
 */

typedef struct player_race player_race;

struct player_race
{
	cptr title;			/* Type of race */

	s16b r_adj[6];		/* Racial stat boni */

	s16b r_dis;			/* disarming */
	s16b r_dev;			/* magic devices */
	s16b r_sav;			/* saving throw */
	s16b r_stl;			/* stealth */
	s16b r_srh;			/* search ability */
	s16b r_fos;			/* search frequency */
	s16b r_thn;			/* combat (normal) */
	s16b r_thb;			/* combat (shooting) */

	byte r_mhp;			/* Race hit-dice modifier */
	s16b r_exp;			/* Race experience factor */

	byte b_age;			/* base age */
	byte m_age;			/* mod age */

	byte m_b_ht;		/* base height (males) */
	byte m_m_ht;		/* mod height (males) */
	byte m_b_wt;		/* base weight (males) */
	byte m_m_wt;		/* mod weight (males) */

	byte f_b_ht;		/* base height (females) */
	byte f_m_ht;		/* mod height (females)	  */
	byte f_b_wt;		/* base weight (females) */
	byte f_m_wt;		/* mod weight (females) */

	byte infra;			/* Infra-vision	range */

        s32b choice;            /* Legal class choices depending on race */

        s16b mana;              /* % mana */


        struct
        {
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

struct player_class
{
	cptr title;			/* Type of class */

        byte color;                     /* @ color */

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

        struct
        {
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

struct player_trait
{
	cptr title; /* Name of trait */
	s32b choice; /* Legal trait choices, depending on race */
};


/* The information needed to show a single "grid" */
typedef struct cave_view_type cave_view_type;

struct cave_view_type
{
	byte a;		/* Color attribute */
	char c;		/* ASCII character */
};

/*
 * Information about a "party"
 */
typedef struct party_type party_type;

struct party_type
{
	char name[80];		/* Name of the party */
	char owner[20];		/* Owner's name */
	s32b members;		/* Number of people in the party */
	s32b created;		/* Creation (or disband-tion) time */
	byte mode;		/* 'Iron Team' or normal party? (C. Blue) */
	s32b experience;	/* For 'Iron Teams': Max experienc of members. */
};

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
#define GF_RACE 1		/* race rstricted */
#define GF_CLASS 2		/* class restricted */
#define GF_PKILL 4		/* pkill within guild? */

struct guild_type{
	char name[80];
	s32b master;		/* Guildmaster unique player ID */
	s32b members;		/* Number of guild members */
	u32b flags;		/* Guild rules flags */
	s16b minlev;		/* minimum level to join */
};

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

#define HF_NONE 0x00
#define HF_MOAT 0x01
#define HF_RECT 0x02		/* Rectangular house; used for non-trad-houses (currently ALL houses are HF_RECT) */
#define HF_STOCK 0x04		/* house generated by town */
#define HF_NOFLOOR 0x08		/* courtyard generated without floor */
#define HF_JAIL 0x10		/* generate with CAVE_STCK */
#define HF_APART 0x20		/* build apartment (obsolete - DELETEME) */
#define HF_TRAD	0x40		/* Vanilla style house */
#define HF_DELETED 0x80		/* Ruined house - do not save/reload */

#define MAXCOORD 200		/* Maximum vertices on non-rect house */

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

#define OT_PLAYER 1
#define OT_PARTY 2
#define OT_CLASS 3
#define OT_RACE 4
#define OT_GUILD 5


/* house flags */
#define ACF_NONE 0x00

#define ACF_PARTY 0x01
#define ACF_CLASS 0x02
#define ACF_RACE 0x04
#define ACF_LEVEL 0x08

#define ACF_WINNER 0x10
#define ACF_FALLENWINNER 0x20
#define ACF_NOGHOST 0x40
#define ACF_STORE 0x80		/* older idea, I implemented it differently now (see PLAYER_STORES) - C. Blue */


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
	char text[80];		/* that should be enough */
	u16b found;		/* we may want hidden inscription? */
};


#if 0
/* Traditional, store-like house */
struct trad_house_type
{
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

struct hostile_type
{
	s32b id;		/* ID of player we are hostile to */
	hostile_type *next;	/* Next in list */
};
#else
/*
 * More general linked list for player id numbers
 */
#define hostile_type player_list_type
typedef struct player_list_type player_list_type;

struct player_list_type
{
	s32b id;		/* ID of player */
	player_list_type *next;
};
#endif

/* remotely ignore players */
struct remote_ignore
{
	unsigned int id;		/* player unique id */
	short serverid;
	struct remote_ignore *next;	/* Next in list */
};

#if 0 /* not finished - mikaelh */
/*
 * ESP link list
 */
typedef struct esp_link_type esp_link_type;
struct esp_link_type
{
	s32b id;	/* player ID */
	byte type;
	u16b flags;
	u16b end;
	esp_link_type *next;
};
#endif

/* The Troll Pit */
/* Temporary banning of certain addresses */
struct ip_ban{
	struct ip_ban *next;	/* next ip in the list */
	char ip[20];	/* so it shouldn't be really */
	int time;	/* Time in minutes, or zero is permanent */
};

/*
 * Skills !
 */
typedef struct skill_type skill_type;
struct skill_type
{
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
struct skill_player
{
	s32b base_value;                         /* Base value */
	s32b value;                             /* Actual value */
	u16b mod;                               /* Modifier(1 skill point = modifier skill) */
	bool dev;                               /* Is the branch developped ? */
	bool touched;				/* need refresh? */
	char flags1;                            /* Skill flags */
};

/* account flags */
#define ACC_TRIAL	0x00000001	/* Account is awaiting validation */
#define ACC_ADMIN	0x00000002	/* Account members are admins */
#define ACC_MULTI	0x00000004	/* Simultaneous play */
#define ACC_NOSCORE	0x00000008	/* No scoring allowed */
#define ACC_RESTRICTED	0x00000010	/* is restricted (ie after cheezing) */
#define ACC_VRESTRICTED	0x00000020	/* is restricted (ie after cheating) */
#define ACC_PRIVILEGED	0x00000040	/* has privileged powers (ie for running quests) */
#define ACC_VPRIVILEGED	0x00000080	/* has privileged powers (ie for running quests) */
#define ACC_PVP		0x00000100		/* may kill other players */
#define ACC_NOPVP	0x00000200	/* is not able to kill other players */
#define ACC_ANOPVP	0x00000400	/* cannot kill other players; gets punished on trying */
#define ACC_GREETED	0x00000800	/* This player has received a one-time greeting. */
#define ACC_QUIET	0x00001000	/* may not chat or emote in public */
#define ACC_VQUIET	0x00002000	/* may not chat or emote, be it public or private */
#define ACC_BANNED	0x00004000	/* account is temporarily suspended */
#define ACC_DELD	0x00008000	/* Delete account/members */
#define ACC_WARN_REST	0x80000000	/* Received a one-time warning about resting */
/*
 * new account struct - pass in player_type will be removed
 * this will provide a better account management system
 */
struct account{
	u32b id;	/* account id */
	u32b flags;	/* account flags */
//todo, instead of ACC_GREETED, ACC_WARN_.. etc, maybe:	a dedicated 'u32b warnings;	/* account flags for received (one-time) hints/warnings */'
	char name[30];	/* login */
	char pass[20];	/* some crypts are not 13 */
	time_t acc_laston;	/* last time this account logged on (for expiry check) */
	s32b cheeze;	/* value in gold of cheezed goods or money */
	s32b cheeze_self; /* value in gold of cheezed goods or money to own characters */
};
/* Used for updating tomenet.acc structure: */
#if 1
struct account_old{
	u32b id;	/* account id */
	u32b flags;	/* account flags */
	char name[30];	/* login */
	char pass[20];	/* some crypts are not 13 */
	time_t acc_laston;	/* last time this account logged on (for expiry check) */
	s32b cheeze;	/* value in gold of cheezed goods or money */
	s32b cheeze_self; /* value in gold of cheezed goods or money to own characters */
};
#endif
#if 0
struct account_old{
	u32b id;	/* account id */
	u16b flags;	/* account flags */
	char name[30];	/* login */
	char pass[20];	/* some crypts are not 13 */
//#ifdef TEST_SERVER /*enable when converter function is implemented */
	u32b expiry;	/* last time this account logged on (for expiry check) */
	s32b cheeze;	/* value in gold of cheezed goods or money */
	s32b cheeze_self; /* value in gold of cheezed goods or money to own characters */
//#endif
};
#endif
#if 0
struct account_old{
	u32b id;	/* account id */
	u16b flags;	/* account flags */
	char name[30];	/* login */
	char pass[20];	/* some crypts are not 13 */
};
#endif

typedef struct version_type version_type;

struct version_type {		/* Extended version structure */
	int major;
	int minor;
	int patch;
	int extra;
	int branch;
	int build;
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

struct player_type
{
	int conn;			/* Connection number */
	char name[MAX_CHARS];		/* Nickname */
	char basename[MAX_CHARS];	/* == Charactername (Nickname)? */
	char realname[MAX_CHARS];	/* Userid (local machine's user name, default is 'PLAYER') */
	char accountname[MAX_CHARS];
	char hostname[MAX_CHARS];	/* His hostname */
	char addr[MAX_CHARS];		/* His IP address */
//	unsigned int version;		/* His version */
	version_type version;

	s32b id;		/* Unique ID to each player */
	u32b account;		/* account group id */
	u32b dna;		/* DNA - psuedo unique to each player life */
	s32b turn;		/* Player's birthday */
	s32b turns_online;	/* How many turns this char has spent online */
	s32b turns_afk;		/* How many turns this char has spent online while being /afk */
	s32b turns_idle;	/* How many turns this char has spent online while being counted as 'idle' */
	s32b turns_active;	/* How many turns this char has spent online while being neither 'idle' nor 'afk' at once */
	time_t msg;		/* anti spamming protection */
	byte msgcnt;
	byte spam;
	byte talk;		/* talk too much (moltors idea) */

	player_list_type *hostile;	/* List of players we wish to attack */

	char savefile[MAX_PATH_LENGTH];	/* Name of the savefile */

	byte restricted;	/* account is restricted (ie after cheating) */
	byte privileged;	/* account is privileged (ie for quest running) */
	byte pvpexception;	/* account uses different pvp rules than server settings */
	byte mutedchat;		/* account has chat restrictions */
	bool inval;		/* Non validated account */
	bool newly_created;	/* Just newly created char by player_birth()? */

	bool alive;		/* Are we alive */
	bool death;		/* Have we died */
	bool safe_sane;		/* Save players from insanity-death on resurrection (atomic flag) - C. Blue */
	int deathblow;          /* How much damage the final blow afflicted */
	u16b deaths, soft_deaths;	/* Times this character died so far / safely-died (no real death) so far */
	s16b ghost;		/* Are we a ghost */
	s16b fruit_bat;		/* Are we a fruit bat */
	byte lives;         /* number of times we have ressurected */
	byte houses_owned;	/* number of simultaneously owned houses */

	byte prace;			/* Race index */
	byte pclass;		/* Class index */
	byte ptrait;
	byte male;			/* Sex of character */
        byte oops;			/* Unused */

        skill_player s_info[MAX_SKILLS]; /* Player skills */
        s16b skill_points;      /* number of skills assignable */

        /* Copies for /undoskills - mikaelh */
        skill_player s_info_old[MAX_SKILLS]; /* Player skills */
        s16b skill_points_old;  /* number of skills assignable */
	bool reskill_possible;
	
	s16b class_extra;	/* Class extra info */

	byte hitdie;		/* Hit dice (sides) */
	s16b expfact;		/* Experience factor */

	byte maximize;		/* Maximize stats */
	byte preserve;		/* Preserve artifacts */

	s16b age;			/* Characters age */
	s16b ht;			/* Height */
	s16b wt;			/* Weight */
	s16b sc;			/* Social Class */

	u16b align_law;			/* alignment */
	u16b align_good;

	player_race *rp_ptr;		/* Pointers to player tables */
	player_class *cp_ptr;
	player_trait *tp_ptr;

	s32b au;			/* Current Gold */

	s32b max_exp;		/* Max experience */
	s32b exp;			/* Cur experience */
	u16b exp_frac;		/* Cur exp frac (times 2^16) */

	s16b lev;			/* Level */
	s16b max_lev;			/* Usual level after 'restoring life levels' */

	s16b mhp;			/* Max hit pts */
	s16b chp;			/* Cur hit pts */
	u16b chp_frac;		/* Cur hit frac (times 2^16) */
	s16b player_hp[PY_MAX_LEVEL];
	s16b form_hp_ratio;		/* mimic form HP+ percentage */

	s16b msp;			/* Max mana pts */
	s16b csp;			/* Cur mana pts */
	u16b csp_frac;		/* Cur mana frac (times 2^16) */

	s16b mst;			/* Max stamina pts */
	s16b cst;			/* Cur stamina pts */
	s16b cst_frac;			/* 1/10000 */

	object_type *inventory;	/* Player's inventory */
	object_type *inventory_copy; /* Copy of the last inventory sent to the client */

	/* Inventory revisions */
	inventory_change_type *inventory_changes; /* List of recent inventory changes */
	int inventory_revision;	/* Current inventory ID */
	char inventory_changed;	/* Inventory has changed since last update to the client */

	s32b total_weight;	/* Total weight being carried */

	s16b inven_cnt;		/* Number of items in inventory */
	s16b equip_cnt;		/* Number of items in equipment */

	s16b max_plv;		/* Max Player Level */
	s16b max_dlv;		/* Max level explored - extension needed! */
	worldpos recall_pos;	/* what position to recall to */
	u16b town_x, town_y;

	s16b stat_max[6];	/* Current "maximal" stat values */
	s16b stat_cur[6];	/* Current "natural" stat values */

	char history[4][60];	/* The player's "history" */

	unsigned char wild_map[MAX_WILD_8]; /* the wilderness we have explored */

	s16b py;		/* Player location in dungeon */
	s16b px;

	struct worldpos wpos;

	struct worldspot memory; /* Runemaster's remembered teleportation spot */
	u16b tim_deflect;	/* Timed -- Deflection */
	u16b tim_trauma;	/* Timed -- Traumaturgy */
	u16b tim_trauma_pow;	/* Timed -- Traumaturgy */

	s16b cur_hgt;		/* Height and width of their dungeon level */
	s16b cur_wid;

	bool new_level_flag;	/* Has this player changed depth? */
	byte new_level_method;	/* Climb up stairs, down, or teleport level? */

	/* changed from byte to u16b - mikaelh */
	u16b party;		/* The party he belongs to (or 0 if neutral) */
	byte guild;		/* The guild he belongs to (0 if neutral)*/

	s16b target_who;
	s16b target_col;	/* What position is targetted */
	s16b target_row;

	s16b health_who;	/* Who's shown on the health bar */

	s16b view_n;		/* Array of grids viewable to player */
	byte view_y[VIEW_MAX];
	byte view_x[VIEW_MAX];

	s16b lite_n;		/* Array of grids lit by player lite */
	byte lite_y[LITE_MAX];
	byte lite_x[LITE_MAX];

	s16b temp_n;		/* Array of grids used for various things */
	byte temp_y[TEMP_MAX];
	byte temp_x[TEMP_MAX];

	s16b target_n;		/* Array of grids used for targetting/looking */
	byte target_y[TEMP_MAX];
	byte target_x[TEMP_MAX];
	s16b target_idx[TEMP_MAX];

	char infofile[MAX_PATH_LENGTH];		/* Temp storage of *ID* and Self Knowledge info */
	char cur_file[MAX_PATH_LENGTH];		/* Filename this player's viewing */
	byte special_file_type;	/* Is he using *ID* or Self Knowledge? */

	byte cave_flag[MAX_HGT][MAX_WID]; /* Can the player see this grid? */

	bool mon_vis[MAX_M_IDX];  /* Can this player see these monsters? */
	bool mon_los[MAX_M_IDX];

	bool obj_vis[MAX_O_IDX];  /* Can this player see these objcets? */

	bool play_vis[MAX_PLAYERS];	/* Can this player see these players? */
	bool play_los[MAX_PLAYERS];

	bool obj_aware[MAX_K_IDX]; /* Is the player aware of this obj type? */
	bool obj_tried[MAX_K_IDX]; /* Has the player tried this obj type? */
	bool obj_felt[MAX_K_IDX]; /* Has the player felt the value of this obj type via pseudo-id before? - C. Blue */
	bool obj_felt_heavy[MAX_K_IDX]; /* Has the player had strong pseudo-id on this item? */

	bool trap_ident[MAX_T_IDX];       /* do we know the name */

	byte d_attr[MAX_K_IDX];
	char d_char[MAX_K_IDX];
	byte f_attr[MAX_F_IDX];
	char f_char[MAX_F_IDX];
	byte k_attr[MAX_K_IDX];
	char k_char[MAX_K_IDX];
	byte r_attr[MAX_R_IDX];
	char r_char[MAX_R_IDX];

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
	bool disturb_panel;
	bool disturb_state;
	bool disturb_minor;
	bool disturb_other;

	bool auto_afk;
	bool newb_suicide;
	bool stack_allow_items;
	bool stack_allow_wands;
	bool view_perma_grids;
	bool view_torch_grids;

	bool view_reduce_lite;
	bool view_reduce_view;
	bool view_yellow_lite;
	bool view_bright_lite;
	bool view_granite_lite;
	bool view_special_lite;

	/* TomeNET additions -- consider using macro or bitfield */
	bool easy_open;
	bool easy_disarm;
	bool easy_tunnel;
	bool auto_destroy;
	bool auto_inscribe;
	bool taciturn_messages;
	bool last_words;
	bool limit_chat;
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

	s16b max_panel_rows;
	s16b max_panel_cols;
	s16b panel_row;
	s16b panel_col;
	s16b panel_row_min;
	s16b panel_row_max;
	s16b panel_col_min;
	s16b panel_col_max;
	s16b panel_col_prt;	/* What panel this guy's on */
	s16b panel_row_prt;
	s16b panel_row_old;
	s16b panel_col_old;

	/* What he should be seeing */
	cave_view_type scr_info[24][80];	/* Hard-coded 80x24 display */

	/* Overlay layer used for detection */
	cave_view_type ovl_info[24][80];	/* Hard-coded 80x24 display */
	
	s32b mimic_seed;	/* seed for random mimic immunities etc. */

	char died_from[80];	/* What off-ed him */
	char really_died_from[80];	/* What off-ed him */
	char died_from_list[80]; /* what goes on the high score list */
	s16b died_from_depth;	/* what depth we died on */

	u16b total_winner;	/* Is this guy the winner */
	u16b once_winner;	/* Has this guy ever been a winner */
	struct worldpos own1, own2;	/* IF we are a king what do we own ? */
	u16b retire_timer;	/* The number of minutes this guy can play until
				   he will be forcibly retired.
				 */

	u16b noscore;		/* Has he cheated in some way (hopefully not) */
	s16b command_rep;	/* Command repetition */

	byte last_dir;		/* Last direction moved (used for swapping places) */

	s16b running;		/* Are we running */
	byte find_current;	/* These are used for the running code */
	byte find_prevdir;
	bool find_openarea;
	bool find_breakright;
	bool find_breakleft;
	bool running_on_floor;	/* Are we running on normal floor, or over grids that we have special abilities to actually pass */

	bool resting;		/* Are we resting? */

	s16b energy_use;	/* How much energy has been used */

	int look_index;		/* Used for looking or targeting */

	s32b current_char;
	s16b current_spell;	/* Spell being cast */
	s16b current_realm;	/* Realm of spell being cast */
	s16b current_mind;	/* Power being use */
	/* XXX XXX consider using union or sth */
	s16b current_rod;	/* Rod being zapped */
	s16b current_activation;/* Artifact (or dragon mail) being activated */
	s16b current_enchant_h; /* Current enchantments */
	s16b current_enchant_d;
	s16b current_enchant_a;
	s16b current_enchant_flag;
	s16b current_identify;	/* Are we identifying something? */
	s16b current_star_identify;
	s16b current_recharge;
	s16b current_rune_dir; /* Current rune spell direction */
	s16b current_rune1; /* Current rune1 index in bag */
	s16b current_rune2; /* Current rune2 index in bag */
	s16b current_artifact;
	object_type *current_telekinesis;
	s16b current_curse;
	s16b current_tome_creation; /* adding a spell scroll to a custom tome - C. Blue */
	s16b current_force_stack; /* which level 0 item we're planning to stack */

	s16b current_selling;
	s16b current_sell_amt;
	int current_sell_price;

	int using_up_item;	/* Item being used up while enchanting, *ID*ing etc. */

	int store_num;		/* What store this guy is in */
#ifdef PLAYER_STORES
	int ps_house_x, ps_house_y; /* coordinates of the house linked to current player store */
	int ps_mcheque_x;		/* Index or x-coordinate of existing mass-cheque in the house */
	int ps_mcheque_y;		/* y-coordinate of existing mass-cheque in the house */
#endif

	s16b fast;			/* Timed -- Fast */
	s16b fast_mod;   		/* Timed -- Fast */
	s16b slow;			/* Timed -- Slow */
	s16b blind;			/* Timed -- Blindness */
	s16b paralyzed;		/* Timed -- Paralysis */
	s16b confused;		/* Timed -- Confusion */
	s16b afraid;		/* Timed -- Fear */
	s16b image;			/* Timed -- Hallucination */
	s16b poisoned;		/* Timed -- Poisoned */
	int poisoned_attacker;	/* Who poisoned the player - used for blood bond */
	s16b cut;			/* Timed -- Cut */
	int cut_attacker;		/* Who cut the player - used for blood bond */
	s16b stun;			/* Timed -- Stun */

	s16b xtrastat;		/* timed temp +stats */
	s16b statval;		/* which */
	byte xstr;
	byte xint;
	byte xdex;
	byte xcon;
	byte xchr;

	s16b focus_time;		/* focus shot */
	s16b focus_val;

	s16b protevil;		/* Timed -- Protection */
	s16b zeal;		/* timed EA bonus */
	s16b zeal_power;
	s16b martyr;
	s16b martyr_timeout;
	s16b res_fear_temp;
	s16b invuln, invuln_applied;	/* Timed -- Invulnerable; helper var */
	s16b hero;			/* Timed -- Heroism */
	s16b shero;			/* Timed -- Super Heroism */
	s16b berserk;			/* Timed -- Berserk #2 */
	s16b fury;			/* Timed -- Furry */
	s16b tim_thunder;   	/* Timed thunderstorm */
	s16b tim_thunder_p1;	/* Timed thunderstorm */
        s16b tim_thunder_p2;	/* Timed thunderstorm */
	s16b tim_ffall;     	/* Timed Levitation */
	s16b tim_fly;       	/* Timed Levitation */
	s16b shield;		/* Timed -- Shield Spell */
	s16b shield_power;      /* Timed -- Shield Spell Power */
	s16b shield_opt;        /* Timed -- Shield Spell options */
	s16b shield_power_opt;  /* Timed -- Shield Spell Power */
	s16b shield_power_opt2; /* Timed -- Shield Spell Power */
	s16b tim_regen;     /* Timed extra regen */
	s16b tim_regen_pow; /* Timed extra regen power */
	s16b blessed;		/* Timed -- Blessed */
	s16b blessed_power;		/* Timed -- Blessed */
	s16b tim_invis;		/* Timed -- See Invisible */
	s16b tim_infra;		/* Timed -- Infra Vision */
	s16b tim_wraith;	/* Timed -- Wraithform */
	u16b tim_jail;		/* Timed -- Jailed */
	u16b tim_susp;		/* Suspended sentence (dungeon) */
	u16b tim_pkill;		/* pkill changeover timer */
	u16b pkill;			/* pkill flags */
	u16b tim_store;		/* timed -- how long (s)he can stay in a store */
	bool wraith_in_wall;	/* currently no effect! */
	s16b tim_meditation;	/* Timed -- Meditation */
	s16b tim_invisibility;		/* Timed -- Invisibility */
	s16b tim_invis_power;	/* Timed -- Invisibility Power */
	s16b tim_traps; 	/* Timed -- Avoid traps */
	s16b tim_manashield;    /* Timed -- Mana Shield */
	s16b tim_mimic; 	/* Timed -- Mimicry */
	s16b tim_mimic_what; 	/* Timed -- Mimicry */
	s16b bow_brand; 	/* Timed -- Bow Branding */
	byte bow_brand_t; 	/* Timed -- Bow Branding */
	s16b bow_brand_d; 	/* Timed -- Bow Branding */
	s16b brand; 	/* Timed -- Weapon Branding */
	byte brand_t; 	/* Timed -- Weapon Branding */
	s16b brand_d; 	/* Timed -- Weapon Branding */
        s16b prob_travel;       /* Timed -- Probability travel */
        s16b st_anchor;         /* Timed -- Space/Time Anchor */
        s16b tim_esp;           /* Timed -- ESP */
	s16b adrenaline;
	s16b biofeedback;
	s16b mindboost;
	s16b mindboost_power;
	s16b kinetic_shield;

	s16b auto_tunnel;
	s16b body_monster;
	bool dual_wield;	/* Currently wielding 2 one-handers at once */

	s16b bless_temp_luck;		/* Timed blessing - luck */
	s16b bless_temp_luck_power;

	s16b oppose_acid;	/* Timed -- oppose acid */
	s16b oppose_elec;	/* Timed -- oppose lightning */
	s16b oppose_fire;	/* Timed -- oppose heat */
	s16b oppose_cold;	/* Timed -- oppose cold */
	s16b oppose_pois;	/* Timed -- oppose poison */

	s16b word_recall;	/* Word of recall counter */

	s16b energy;		/* Current energy */

	s16b food;			/* Current nutrition */

	byte confusing;		/* Glowing hands */
	byte stunning;		/* Heavy hands */
	byte searching;		/* Currently searching */

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

	s16b old_lite;		/* Old radius of lite (if any) */
	s16b old_vlite;		/* Old radius of virtual lite (if any) */
	s16b old_view;		/* Old radius of view (if any) */

	s16b old_food_aux;	/* Old value of food */


	bool cumber_armor;	/* Encumbering armor (tohit/sneakiness) */
	bool awkward_armor;	/* Mana draining armor */
	bool cumber_glove;	/* Mana draining gloves */
	bool cumber_helm;	/* Mana draining headgear */
	bool heavy_wield;	/* Heavy weapon */
	bool heavy_shield;	/* Heavy shield */
	bool heavy_shoot;	/* Heavy shooter */
	bool icky_wield;	/* Icky weapon */
	bool awkward_wield;	/* shield and COULD_2H weapon */
	bool easy_wield;	/* Using a 1-h weapon which is MAY2H with both hands */
	bool cumber_weight;	/* Full weight. FA from MA will be lost if overloaded */
	bool monk_heavyarmor;	/* Reduced MA power? */
	bool awkward_shoot;	/* using ranged weapon while having a shield on the arm */
	bool rogue_heavyarmor;	/* No AoE-searching? Encumbered dual-wield? */

	s16b cur_lite;		/* Radius of lite (if any) */
	s16b cur_vlite;		/* radius of virtual light (not visible to others) */


	u32b notice;		/* Special Updates (bit flags) */
	u32b update;		/* Pending Updates (bit flags) */
	u32b redraw;		/* Normal Redraws (bit flags) */
	u32b window;		/* Window Redraws (bit flags) */

	s16b stat_use[6];	/* Current modified stats */
	s16b stat_top[6];	/* Maximal modified stats */

	s16b stat_add[6];	/* Modifiers to stat values */
	s16b stat_ind[6];	/* Indexes into stat tables */

	s16b stat_cnt[6];	/* Counter for temporary drains */
	s16b stat_los[6];	/* Amount of temporary drains */

	bool immune_acid;	/* Immunity to acid */
	bool immune_elec;	/* Immunity to lightning */
	bool immune_fire;	/* Immunity to fire */
	bool immune_cold;	/* Immunity to cold */

        s16b reduc_fire;        /* Fire damage reduction */
        s16b reduc_elec;        /* elec damage reduction */
        s16b reduc_acid;        /* acid damage reduction */
        s16b reduc_cold;        /* cold damage reduction */

	bool resist_acid;	/* Resist acid */
	bool resist_elec;	/* Resist lightning */
	bool resist_fire;	/* Resist fire */
	bool resist_cold;	/* Resist cold */
	bool resist_pois;	/* Resist poison */

	bool resist_conf;	/* Resist confusion */
	bool resist_sound;	/* Resist sound */
	bool resist_lite;	/* Resist light */
	bool resist_dark;	/* Resist darkness */
	bool resist_chaos;	/* Resist chaos */
	bool resist_disen;	/* Resist disenchant */
	bool resist_shard;	/* Resist shards */
	bool resist_nexus;	/* Resist nexus */
	bool resist_blind;	/* Resist blindness */
	bool resist_neth;	/* Resist nether */
	bool resist_fear;	/* Resist fear */

	bool sustain_str;	/* Keep strength */
	bool sustain_int;	/* Keep intelligence */
	bool sustain_wis;	/* Keep wisdom */
	bool sustain_dex;	/* Keep dexterity */
	bool sustain_con;	/* Keep constitution */
	bool sustain_chr;	/* Keep charisma */

	bool aggravate;		/* Aggravate monsters */
	bool teleport;		/* Random teleporting */

	bool feather_fall;	/* No damage falling */
	bool lite;		/* Permanent light */
	bool free_act;		/* Never paralyzed */
	bool see_inv;		/* Can see invisible */
	bool regenerate;	/* Regenerate hit pts */
	bool resist_time;	/* Resist time */
	bool resist_mana;	/* Resist mana */
	bool immune_poison;	/* Poison immunity */
	bool immune_water;	/* Makes immune to water */
	bool resist_water;	/* Resist Water */
	bool resist_plasma;	/* Resist Plasma */
	bool regen_mana;	/* Regenerate mana */
	bool hold_life;		/* Resist life draining */
	u32b telepathy;		/* Telepathy */
	bool slow_digest;	/* Slower digestion */
	bool bless_blade;	/* Blessed blade */
	byte xtra_might;	/* Extra might bow */
	bool impact;		/* Earthquake blows */
        bool auto_id; /* Pickup = Id */
	bool reduce_insanity;	/* For mimic forms with weird/empty mind */
	
	s16b invis;		/* Invisibility */

	s16b dis_to_h;		/* Known bonus to hit */
	s16b dis_to_d;		/* Known bonus to dam */
	s16b dis_to_h_ranged;	/* Known bonus to hit */
	s16b dis_to_d_ranged;	/* Known bonus to dam */
	s16b dis_to_a;		/* Known bonus to ac */

	s16b dis_ac;		/* Known base ac */

	s16b to_h_ranged;	/* Bonus to hit */
	s16b to_d_ranged;	/* Bonus to dam */
	s16b to_h_melee;	/* Bonus to hit */
	s16b to_d_melee;	/* Bonus to dam */
	s16b to_h;		/* Bonus to hit */
	s16b to_d;		/* Bonus to dam */
	s16b to_a;		/* Bonus to ac */

	s16b ac;		/* Base ac */

	s16b see_infra;		/* Infravision range */

	s16b skill_dis;		/* Skill: Disarming */
	s16b skill_dev;		/* Skill: Magic Devices */
	s16b skill_sav;		/* Skill: Saving throw */
	s16b skill_stl;		/* Skill: Stealth factor */
	s16b skill_srh;		/* Skill: Searching ability */
	s16b skill_fos;		/* Skill: Searching frequency */
	s16b skill_thn;		/* Skill: To hit (normal) */
	s16b skill_thb;		/* Skill: To hit (shooting) */
	s16b skill_tht;		/* Skill: To hit (throwing) */
	s16b skill_dig;		/* Skill: Digging */

	s16b num_blow;		/* Number of blows */
	s16b num_fire;		/* Number of shots */
	s16b num_spell;		/* Number of spells */

	byte tval_xtra;		/* Correct xtra tval */

	byte tval_ammo;		/* Correct ammo tval */

	s16b pspeed;		/* Current speed */
	
 	s16b r_killed[MAX_R_IDX];	/* Monsters killed */

	s32b melee_techniques_old; /* melee techniques before last skill point update */
	s32b melee_techniques; /* melee techniques */
	s32b ranged_techniques_old; /* ranged techniques before last skill point update */
	s32b ranged_techniques; /* ranged techniques */
	s32b innate_spells[3]; /* Monster spells */
	bool body_changed;
	
	bool anti_magic;	/* Can the player resist magic */

	player_list_type	*blood_bond;	/* Norc is now happy :) */

	byte mode; /* Difficulty MODE */

#if 1
	s32b esp_link; /* Mental link */
	byte esp_link_type;
	u16b esp_link_flags;
	u16b esp_link_end; /* Time before actual end */
#else
	/* new esp link stuff - mikaelh */
	esp_link_type *esp_link; /* Mental link */
	u16b esp_link_flags; /* Some flags */
#endif
	bool (*master_move_hook)(int Ind, char *args);

	/* some new borrowed flags (saved) */
	bool black_breath;      /* The Tolkien's Black Breath */
	bool black_breath_tmp;	/* (NOT saved) BB induced by an item */
	/*        u32b malady; */     /* TODO: Flags for malady */

	s16b msane;                   /* Max sanity */
	s16b csane;                   /* Cur sanity */
	u16b csane_frac;              /* Cur sanity frac */

	/* elements under this line won't be saved...for now. - Jir - */
	player_list_type	*ignore;  /* List of players whose chat we wish to ignore */
	struct remote_ignore	*w_ignore;  /* List of players whose chat we wish to ignore */
	long int idle;   	/* player is idling for <idle> seconds.. */
	long int idle_char;   	/* character is idling for <idle_char> seconds (player still might be chatting etc) */
	bool	afk;		/* player is afk */
	char	afk_msg[80];	/* afk reason */
	char	info_msg[80];	/* public info message (display gets overridden by an afk reason, if specified) */
	bool	use_r_gfx;	/* hack - client uses gfx? */
	player_list_type	*afk_noticed; /* Only display AFK messages once in private conversations */

	byte drain_exp;		/* Experience draining */
	byte drain_life;        /* hp draining */
	byte drain_mana;        /* mana draining */

	bool suscep_fire;     /* Fire does more damage on the player */
	bool suscep_cold;     /* Cold does more damage on the player */
	bool suscep_acid;     /* Acid does more damage on the player */
	bool suscep_elec;     /* Electricity does more damage on the player */
	bool suscep_pois;     /* Poison does more damage on the player */
	bool suscep_lite;	/* Light does more damage on the player */
	bool suscep_good;	/* Anti-evil effects do more damage on the player */
	bool suscep_life;	/* Anti-undead effects do more damage on the player */

	bool reflect;       /* Reflect 'bolt' attacks */
	int shield_deflect;       /* Deflect various attacks (ranged), needs USE_BLOCKING */
	int weapon_parry;       /* Parry various attacks (melee), needs USE_PARRYING */
	bool no_cut;	    /* For mimic forms */
	bool sh_fire;       /* Fiery 'immolation' effect */
	bool sh_elec;       /* Electric 'immolation' effect */
	bool sh_cold;       /* Cold 'immolation' effect */
	bool wraith_form;   /* wraithform */
	bool immune_neth;       /* Immunity to nether */
	bool climb;             /* Can climb mountains */
	bool fly;               /* Can fly over some features */
	bool can_swim;		/* Can swim like a fish (or Lizard..whatever) */
	bool pass_trees;	/* Can pass thick forest */
	bool town_pass_trees;	/* Can pass forest in towns, as an exception to make movement easier */
	
	int luck;	/* Extra luck of this player */

	/*        byte anti_magic_spell;    *//* Anti-magic(newer one..) */
	byte antimagic;    		/* Anti-magic(in percent) */
	byte antimagic_dis;     /* Radius of the anti magic field */
	bool anti_tele;     /* Prevent any teleportation + phasing + recall */
	bool res_tele;		/* Prevents being teleported from someone else */
	/* in PernM, it's same as st_anchor */
	bool resist_continuum;	/* non-timed -- Space/Time Anchor */
	bool admin_wiz;		/* Is this char Wizard? */
	bool admin_dm;		/* or Dungeon Master? */
	bool admin_dm_chat;	/* allow players to send private chat to an invisible DM */
	bool stormbringer;	/* Attack friends? */
	int vampiric_melee;		/* vampiric in close combat? */
	int vampiric_ranged;		/* shots have vampiric effects? */

	bool ty_curse;		/* :-o */
	bool dg_curse;

	u16b quest_id;		/* Quest number */
	s16b quest_num;		/* Number of kills needed */

	s16b xtra_crit;         /* critical strike bonus from item */
	s16b extra_blows;		/* Number of extra blows */

	s16b to_l;                      /* Bonus to life */
	s32b to_hp;                      /* Bonus to Hit Points */
	s16b to_m;                      /* Bonus to mana */
	/*        s16b to_s; */                     /* Bonus to spell(num_spell) */
	s16b dodge_level;		/* Chance of dodging blows/missiles */

	s32b balance;		/* Deposit/debt */
	s32b tim_blacklist;		/* Player is on the 'Black List' (he gets penalties in shops) */
	s32b tim_watchlist;		/* Player is on the 'Watch List' (he may not steal) */
	s32b pstealing;			/* Player has just tried to steal from another player. Cooldown timer. */
	int ret_dam;                    /* Drained life from a monster */
	char attacker[80];		/* Monster doing a ranged attack on the player */
#if 0
	s16b mtp;                       /* Max tank pts */
	s16b ctp;                       /* Cur tank pts */
	s16b tp_aux1;                   /* aux1 tank pts */
	s16b tp_aux2;                   /* aux2 tank pts */

	s32b grace;                     /* Your God's appreciation factor. */
	byte pgod;                      /* Your God. */
	bool praying;                   /* Praying to your god. */
	s16b melkor_sacrifice;          /* How much hp has been sacrified for damage */
#endif	/* 0 */

        byte spell_project;             /* Do the spells(some) affect nearby party members ? */

        /* Special powers */
        s16b powers[MAX_POWERS];        /* What powers do we possess? */
        s16b power_num;                 /* How many */

	/* evileye games */
	s16b team;			/* what team */

	/* Moltor's arcade crap */
        int arc_a, arc_b, arc_c, arc_d, arc_e, arc_f, arc_g, arc_h, arc_i, arc_j, arc_k, arc_l;
	char firedir; 
	char game;
	int gametime;
	char pushed;
	char pushdir;

	/* C. Blue - was the last shutdown a panic save? */
	bool panic;
	
	/* Anti-cheeze */
	s16b supported_by;		/* level of the highest supporter */
	s16b support_timer;		/* safe maximum possible duration of the support spells */

	/* any automatic savegame update to perform? (toggle) */
	byte updated_savegame;
	/* for automatic artifact reset (similar to updated_savegame) */
	byte artifact_reset;
	/* C. Blue - Fun stuff :) Make player vomit if he turns around ***a lot*** (can't happen in 'normal' gameplay) */
	s16b corner_turn;
	int joke_weather;	/* personal rain^^ */
	/* automatic (scripted) transport sequences */
	byte auto_transport;
	/* Player being paged by others? (Beep counter) */
	byte paging;
	/* Ignoring normal chat? (Will only see private & party messages then) */
	bool ignoring_chat;
	/* Being an ass? - the_sandman */
	bool muted;
	/* Pet limiter */
	byte has_pet;
	/* Is the player auto-retaliating? (required for hack that fixes a lock bug) */
	bool auto_retaliating;
	bool auto_retaliaty; /* TRUE for code-wise duration of autorataliation
				actions, to prevent going un-AFK from them! */
	
	/* Global events participant? */
	int global_event_type[MAX_GLOBAL_EVENTS]; /* 0 means 'not participating' */
	time_t global_event_signup[MAX_GLOBAL_EVENTS];
	time_t global_event_started[MAX_GLOBAL_EVENTS];
	u32b global_event_progress[MAX_GLOBAL_EVENTS][4];
	u32b global_event_temp; /* not saved. see defines.h for details */

	/* Add more here... These are "toggle" buffs. They will add to the num_of_buffs
	 * and that number will affect mana usage */
	int rune_num_of_buffs;
#ifdef ENABLE_RUNEMASTER
	int rune_speed;
	int rune_stealth;
	int rune_IV;
	int rune_vamp;
#endif //runemaster

#ifdef ENABLE_DIVINE
	int divine_crit;
	int divine_hp;
	int divine_xtra_res_time_mana;

	int divine_crit_mod;
	int divine_hp_mod;
	int divine_xtra_res_time_mana_mod;
#endif
	/* Prevent players from taking it multiple times from a single effect - mikaelh */
	bool got_hit;
	/* No insane amounts of damage either */
	s32b total_damage;

#ifdef AUCTION_SYSTEM
	/* The current auction - mikaelh */
	int current_auction;
#endif

/* ENABLE_STANCES - this code must always be compiled, otherwise savegames would screw up! so no #ifdef here. */
	/* combat stances */
	int combat_stance; /* 0 = normal, 1 = def, 2 = off */
	int combat_stance_power; /* 1,2,3, and 4 = royal (for NR balanced) */

	/* more techniques */	
	byte cloaked, cloak_neutralized; /* Cloaking mode enabled; suspicious action was spotted */
	s16b melee_sprint, ranged_double_used;
	bool ranged_flare, ranged_precision, ranged_double, ranged_barrage;
	bool shoot_till_kill, shooty_till_kill, shooting_till_kill; /* Shoot a target until it's dead, like a ranged 'auto-retaliator' - C. Blue */
	int shoot_till_kill_book, shoot_till_kill_spell;
	bool shadow_running;
	bool dual_mode; /* for dual-wield: TRUE = dual-mode, FALSE = main-hand-mode */
	
#ifdef ENABLE_RCRAFT
	/* Last attack spell cast for ftk mode */
	u32b shoot_till_kill_rune_spell;
	byte shoot_till_kill_rune_modifier;
#endif

#if 0 /* deprecated */
	/* NOT IMPLEMENTED YET: add spell array for quick access via new method of macroing spells
	   by specifying the spell name instead of a book and position - C. Blue */
	char spell_name[100][20];
	int spell_book[100], spell_pos[100];
#endif

	bool aura[MAX_AURAS]; /* allow toggling auras for possibly more tactical utilization - C. Blue */
	
	/* for C_BLUE_AI, new thingy: Monsters that are able to ignore a "tank" player */
	int heal_turn[20 + 1]; /* records the amount of healing the player received for each of 20 consecutive turns */
	u32b heal_turn_20, heal_turn_10, heal_turn_5;
	int dam_turn[20 + 1]; /* records the amount of damage the player dealt for each of 20 consecutive turns */
	u32b dam_turn_20, dam_turn_10, dam_turn_5;
	
	/* for PvP mode: keep track of kills/progress for adding a reward or something - C. Blue */
	int kills, kills_lower, kills_higher, kills_equal, kills_own;
	int free_mimic, pvp_prevent_tele, pvp_prevent_phase;
	long heal_effect;
	
	/* for client-side weather */
	bool panel_changed;
	int custom_weather; /* used /cw command */

	/* buffer for anti-cheeze system, just to reduce file access to tomenet.acc */
	s32b cheeze_value, cheeze_self_value;

	int mcharming;	/* for mindcrafters' charming */
	u32b turns_on_floor;	/* number of turns spent on the current floor */
	bool distinct_floor_feeling;	/* set depending on turns_on_floor */
	bool sun_burn;		/* Player is vampire, currently burning in the sun? */

	/* server-side animation timing flags */
	bool invis_phase; /* for invisible players who flicker towards others */
//not needed!	int colour_phase; /* for mimics mimicking multi-coloured stuff */

	/* for hunting down bots generating exp by opening and magelocking doors */
	u32b silly_door_exp;
	
#if (MAX_PING_RECVS_LOGGED > 0)
	/* list of ping reception times */
	struct timeval pings_received[MAX_PING_RECVS_LOGGED];
	char pings_received_head;
#endif

	/* allow admins to put a character into 'administrative stasis' */
	int admin_stasis;
	/* more admin fooling around (give a 1-hit-kill attack to the player, or let him die in 1 hit) */
	int admin_godly_strike, admin_set_defeat;

#ifdef TEST_SERVER
	u32b test_count, test_dam;
#endif

	/* give players certain warnings, meant to guide newbies along, and remember
	   if we already gave a specific warning, so we don't spam the player with it
	   again and again - although noone probably reads them anyway ;-p - C. Blue */
	char warning_bpr, warning_bpr2, warning_bpr3;
	char warning_run, warning_run_steps, warning_run_monlos;
	char warning_wield, warning_chat, warning_lite, warning_lite_refill;
	char warning_wield_combat; /* warn if engaging into combat (attacking/taking damage) without having equipped melee/ranged weapons! (except for druids) */
	char warning_rest;/* if a char rests from <= 40% to 50% without R, or so..*/
	char warning_mimic, warning_dual, warning_potions, warning_wor;
	char warning_ghost, warning_autoret, warning_autoret_ok;
	char warning_ma_weapon, warning_ma_shield;
	char warning_technique_melee, warning_technique_ranged;
	char warning_hungry;
	/* note: a sort of "warning_skills" is already implemented, in a different manner */

#ifdef USE_SOUND_2010
	int music_current, music_monster; //background music currently playing for him/her; an overriding monster music
#endif
	bool cut_sfx_attack;
	int count_cut_sfx_attack;

	/* various flags/counters to check if a character is 'freshly made' and/or has already interacted in certain ways.
	   Mostly to test if he/she is eglibile to join events. */
	bool recv_gold, recv_item, bought_item, killed_mon;
};

/* For Monk martial arts */

typedef struct martial_arts martial_arts;

struct martial_arts
{
    cptr    desc;    /* A verbose attack description */
    int     min_level;  /* Minimum level to use */
    int     rchance;     /* Reverse chance, lower value means more often */
    int     dd;        /* Damage dice */
    int     ds;        /* Damage sides */
    int     effect;     /* Special effects */
};

/* Define monster generation rules */
typedef struct rule_type rule_type;
struct rule_type
{
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

	char r_char[5];			/* Monster race allowed */
};

/* A structure for the != dungeon types */
typedef struct dungeon_info_type dungeon_info_type;
struct dungeon_info_type
{
	u32b name;                      /* Name */
//	int idx;			/* index in d_info.txt */
	u32b text;                      /* Description */
	char short_name[3];             /* Short name */

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
	u32b flags2;                    /* Flags 1 */

	byte rule_percents[100];        /* Flat rule percents */
	rule_type rules[5];             /* Monster generation rules */

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
struct town_extra
{
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

struct server_opts
{
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
	bool maximize;
	bool kings_etiquette;
	bool fallenkings_etiquette;

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
	bool worldd_pubchat, worldd_privchat, worldd_broadcast, worldd_lvlup, worldd_unideath, worldd_pwin, worldd_pdeath, worldd_pjoin, worldd_pleave, worldd_plist;
};

/* Client option struct */
/* Consider separate it into client/types.h and server/types.h */
typedef struct client_opts client_opts;

struct client_opts
{
	bool rogue_like_commands;
	bool quick_messages;
	bool other_query_flag;
	bool carry_query_flag;
	bool use_old_target;
	bool always_pickup;
	bool always_repeat;
	bool depth_in_feet;
	bool stack_force_notes;
	bool stack_force_costs;
	bool show_labels;
	bool show_weights;
	bool show_choices;
	bool show_details;
	bool ring_bell;
	bool use_color;
	bool short_item_names;
	bool hide_unusable_skills;

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
	bool alert_hitpoint;
	bool alert_failure;

	bool auto_afk;
	bool newb_suicide;
	bool stack_allow_items;
	bool stack_allow_wands;
	bool expand_look;
	bool expand_list;
	bool view_perma_grids;
	bool view_torch_grids;
	bool no_verify_destroy; //dungeon_align;
	bool whole_ammo_stack; //dungeon_stair;
	bool recall_flicker;
/*	bool flow_by_sound;	*/
	bool time_stamp_chat; //flow_by_smell;
	bool track_follow;
	bool track_target;
	bool smart_learn;
	bool smart_cheat;

	bool view_reduce_lite;
	bool view_reduce_view;
	bool avoid_abort;
	bool avoid_other;
	bool flush_failure;
	bool flush_disturb;
	bool flush_command;
	bool fresh_before;
	bool fresh_after;
	bool fresh_message;
	bool compress_savefile;
	bool hilite_player;
	bool view_yellow_lite;
	bool view_bright_lite;
	bool view_granite_lite;
	bool view_special_lite;

	bool easy_open;
	bool easy_disarm;
	bool easy_tunnel;
	bool auto_destroy;
	bool auto_inscribe;
	bool taciturn_messages;
	bool last_words;
	bool limit_chat;
	bool thin_down_flush;
	bool auto_target;
	bool autooff_retaliator;
	bool wide_scroll_margin;
	bool fail_no_melee;
	bool always_show_lists;
	bool target_history;
	bool linear_stats;
	bool exp_need;
	bool disable_flush;

	bool allow_paging;
	bool audio_paging;
	bool paging_max_volume;
	bool paging_master_volume;
	bool no_ovl_close_sfx;
	bool ovl_sfx_attack;
	bool half_sfx_attack;
	bool cut_sfx_attack;
	bool ovl_sfx_command;
	bool ovl_sfx_misc;
	bool ovl_sfx_mon_attack;
	bool ovl_sfx_mon_spell;
	bool ovl_sfx_mon_misc;
};

/*
 * Extra information on client-side that the server player_type
 * doesn't contain.		- Jir -
 *
 * Most variables in client/variable.c should be bandled here maybe.
 */
typedef struct c_player_extra c_player_extra;
struct c_player_extra
{
	char body_name[80];	/* Form of Player */
	char sanity[10];	/* Sanity strings */
	byte sanity_attr;	/* Colour to display sanity */
	char location_name[20];	/* Name of location (eg. 'Bree') */
};

typedef struct c_store_extra c_store_extra;
struct c_store_extra
{
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
struct hooks_chain
{
	char name[40];
	char script[40];
	hooks_chain *next;
};

typedef union hook_return hook_return;
union hook_return
{
	s32b num;
	char *str;
	object_type *o_ptr;
};

/*
 * The spell function must provide the desc
 */
typedef struct spell_type spell_type;
struct spell_type
{
        cptr name;                      /* Name */
        byte skill_level;               /* Required level (to learn) */
	byte mana;			/* Required mana at lvl 1 */
	byte mana_max;			/* Required mana at max lvl */
	byte fail;			/* Minimum chance of failure */
        s16b level;                     /* Spell level(0 = not learnt) */
        byte spell_power;		/* affected by spell-power skill? */
};

typedef struct school_type school_type;
struct school_type
{
        cptr name;                      /* Name */
        s16b skill;                     /* Skill used for that school */
};

/* C. Blue - don't confuse with quest_type, which is for the basic kill '/quest'.
   This is more of a global event, first use will be automated Highlander Tournament
   schedule. Timing is possible too. Might want to make use of AT_... sequences. */
typedef struct global_event_type global_event_type;
struct global_event_type
{
    int getype;			/* Type of the event (or quest) */
    bool paused;		/* Is the event currently paused? (special admin command) */
    s32b paused_turns;		/* Keeps track of turns the event was actually frozen */
    s32b state[64];		/* progress */
    s32b extra[64];		/* extra info */
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
};


#ifdef ENABLE_RCRAFT

typedef struct r_type r_type;
/* Method list: bolt/beam/etc */
struct r_type
{
	byte id;
	u32b type; /* Flag */
	char * title;
	byte cost; /* Extra levels required to cast */
	byte pen; /* MP multiplier */
};

typedef struct r_element r_element;
/* Element list */
struct r_element
{
	byte id;
	char * title; /* Description */
	char * e_syl; /* Magical description (to remove?) */
	byte cost; /* Base cost for use, used to calculate mp/damage/fail rates */
	u16b skill; /* The skill pair which governs it */
	u32b self; /* Its flag */
};

typedef struct r_spell r_spell;
/* Spell list */
struct r_spell
{
	int id;
	char * title;
	s16b dam; /* Damage multipler */
	s16b pen; /* MP multiplier */
	s16b level; /* Minimum level to cast */
	s16b fail; /* Fail rate multiplier */
	s16b radius; /* Radius at 50 before multipliers: linear scale */
	u16b gf_type; /* Projection type */
};

typedef struct r_imper r_imper;
/* Spell potencies */
struct r_imper
{
	int id;
	char * name;
	s16b level; //Level +/-
	s16b cost; //Cost multiplier
	s16b fail; //Fail multiplier
	s16b dam; //Damage multipler
	s16b time; //Time to cast
	s16b radius; //Radius +/-
	s16b duration; //Duration multiplier
};

typedef struct rspell_sel rspell_sel;
/* Spell selectors */
struct rspell_sel
{
	long flags;
	int type;
};

#endif /* ENABLE_RCRAFT */


/* Auction system - mikaelh */
typedef struct bid_type bid_type;
struct bid_type
{
	s32b		bid;
	s32b		bidder;
};

typedef struct auction_type auction_type;
struct auction_type
{
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
typedef struct astar_list_open astar_list_open;
struct astar_list_open {
	int m_idx; /* monster which currently uses this index in the available A* arrays, or -1 for 'unused' ie available */
	int nodes; /* current amount of nodes stored in this list */
	int node_x[ASTAR_MAX_NODES], node_y[ASTAR_MAX_NODES]; /* unsigned char would do, but maybe we want to stop using that one for floor grids (compiler warnings in other files too, etc..) */
	int astarF[ASTAR_MAX_NODES], astarG[ASTAR_MAX_NODES], astarH[ASTAR_MAX_NODES]; /* grid score (F=G+H), starting point distance cost, estimated goal distance cost */
	int closed_parent_idx[ASTAR_MAX_NODES]; /* the idx of the grid in the closed list, which is the parent of this grid */
};
typedef struct astar_list_closed astar_list_closed;
struct astar_list_closed {
	int nodes; /* current amount of nodes stored in this list */
	int node_x[ASTAR_MAX_NODES], node_y[ASTAR_MAX_NODES]; /* unsigned char would do, but maybe we want to stop using that one for floor grids (compiler warnings in other files too, etc..) */
	int closed_parent_idx[ASTAR_MAX_NODES]; /* the idx of the grid in the closed list, which is the parent of this grid */
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
