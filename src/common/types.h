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
#endif	// 0


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
//		s32b pval2;                     /* Object extra info */

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
#if 1
        u32b flags4;            /* Artifact Flags, set 4 */
        u32b flags5;            /* Artifact Flags, set 5 */
#endif	// 0

	byte level;			/* Artifact level */
	byte rarity;		/* Artifact rarity */

	byte cur_num;		/* Number created (0 or 1) */
	byte max_num;		/* Unused (should be "1") */
        u32b esp;                       /* ESP flags */
#if 0

        s16b power;                     /* Power granted(if any) */

        s16b set;               /* Does it belongs to a set ?*/
#endif	// 0

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

        byte tval[6];
        byte min_sval[6];
        byte max_sval[6];

//	byte slot;			/* Standard slot value */
	byte rating;		/* Rating boost */

	byte level;			/* Minimum level */
	byte rarity;		/* Object rarity */
        byte mrarity;           /* Object rarity */

	char max_to_h;		/* Maximum to-hit bonus */
	char max_to_d;		/* Maximum to-dam bonus */
	char max_to_a;		/* Maximum to-ac bonus */

	char max_pval;		/* Maximum pval */

	s32b cost;			/* Ego-item "cost" */

#if 0
	u32b flags1;		/* Ego-Item Flags, set 1 */
	u32b flags2;		/* Ego-Item Flags, set 2 */
	u32b flags3;		/* Ego-Item Flags, set 3 */
#endif	// 0

        byte rar[5];
        u32b flags1[5];            /* Ego-Item Flags, set 1 */
        u32b flags2[5];            /* Ego-Item Flags, set 2 */
        u32b flags3[5];            /* Ego-Item Flags, set 3 */
#if 1
        u32b flags4[5];            /* Ego-Item Flags, set 4 */
        u32b flags5[5];            /* Ego-Item Flags, set 5 */
        u32b esp[5];                       /* ESP flags */
        u32b fego[5];                       /* ego flags */

//        s16b power;                     /* Power granted(if any) */
#endif	// 0
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

	byte hdice;				/* Creatures hit dice count */
	byte hside;				/* Creatures hit dice sides */

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
#endif

	monster_blow blow[4];	/* Up to four blows per round */

	byte body_parts[BODY_MAX];	/* To help to decide what to use when body changing */

	s16b level;				/* Level of creature */
	byte rarity;			/* Rarity of creature */


	byte d_attr;			/* Default monster attribute */
	char d_char;			/* Default monster character */


	byte x_attr;			/* Desired monster attribute */
	char x_char;			/* Desired monster character */


	byte max_num;			/* Maximum population allowed per level */

	byte cur_num;			/* Monster population on current level */


	s32b killer;			/* ID of the player who killed him */

	s16b r_sights;			/* Count sightings of this monster */
	s16b r_deaths;			/* Count deaths from this monster */

	s16b r_pkills;			/* Count monsters killed in this life */
	s16b r_tkills;			/* Count monsters killed in all lives */

	byte r_wake;			/* Number of times woken up (?) */
	byte r_ignore;			/* Number of times ignored (?) */

	/*byte r_xtra1;			changed to time for japanese patch APD Something (unused)
	  byte r_xtra2;                    Something (unused) */
	  
	s32b respawn_timer;			/* The amount of time until the unique respawns */

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

#endif
	obj_theme drops;		/* The drops type */
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
#endif	// 0
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
#if 0	// Handled in player_type
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
#define CS_NONE 0
#define CS_DNADOOR 1
#define CS_KEYDOOR 2
#define CS_TRAPS 3 	/* CS stands for Cave Special */
#define CS_INSCRIP 4	/* ok ;) i'll follow that from now */

#define CS_FOUNTAIN 5
#define CS_BETWEEN	6	/* petit jump type */
#define CS_BETWEEN2	7	/* world traveller type */
#define CS_MON_TRAP	8	/* monster traps */
#define CS_SHOP		9	/* shop/building */
#define CS_MIMIC	10	/* mimic-ing feature (eg. secret door) */

/* heheh it's kludge.. */
#define sc_is_pointer(type)	(type < 3 || type == 4 || 10 < type)

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
//		struct { u16b trap_kit, trap_load; } montrap;
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
	//void (*kill)(void *ptr);		/* removal */
};
#endif

struct cave_type
{
	u16b info;		/* Hack -- cave flags */

	byte feat;		/* Hack -- feature type */

	s16b o_idx;		/* Item index (in o_list) or zero */

	s16b m_idx;		/* Monster index (in m_list) or zero */
				/* or negative if a player */

#ifdef MONSTER_FLOW

	byte cost;		/* Hack -- cost of flowing */
	byte when;		/* Hack -- when cost was computed */

#endif
	struct c_special *special;	/* Special pointer to various struct */

	/* I don't really love to enlarge cave_type ... but it'd suck if
	 * trapped floor will be immune to Noxious Cloud */
	/* Adding 1byte in this struct costs 520Kb memory, in average */
	/* This should be replaced by 'stackable c_special' code -
	 * let's wait for evileye to to this :)		- Jir - */
	byte effect;            /* The lasting effects */
};

/* ToME parts, arranged */
/* This struct can be enlarged to handle more generic timed events maybe? */
/* Lasting spell effects(clouds, ..) */
typedef struct effect_type effect_type;
struct effect_type
{
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

	s32b bpval;			/* Base item extra-parameter */
	s32b pval;			/* Extra enchantment item extra-parameter */
#if 0
	s16b pval2;			/* Item extra-parameter for some special items*/
	s32b pval3;			/* Item extra-parameter for some special items*/
#endif

	byte discount;		/* Discount (if any) */

	byte number;		/* Number of items */

	s16b weight;		/* Item weight */

	byte name1;			/* Artifact type, if any */
	byte name2;			/* Ego-Item type, if any */
	byte name2b;		/* 2e Ego-Item type, if any */
	s32b name3;			/* Randart seed, if any */
						/* now it's common with ego-items -Jir-*/

	byte xtra1;			/* Extra info type (ego only? -Jir-) */
	byte xtra2;			/* Extra info index */

	s16b to_h;			/* Plusses to hit */
	s16b to_d;			/* Plusses to damage */
	s16b to_a;			/* Plusses to AC */

	s16b ac;			/* Normal AC */

	byte dd, ds;			/* Damage dice/sides */

	s16b timeout;			/* Timeout Counter */

	byte ident;			/* Special flags  */

	byte marked;			/* Object is marked */

	u16b note;			/* Inscription index */

#if 0	// from pernA.. consumes memory, but quick. shall we?
        u16b art_name;      /* Artifact name (random artifacts) */

        u32b art_flags1;        /* Flags, set 1  Alas, these were necessary */
        u32b art_flags2;        /* Flags, set 2  for the random artifacts of*/
        u32b art_flags3;        /* Flags, set 3  Zangband */
        u32b art_flags4;        /* Flags, set 4  PernAngband */
        u32b art_flags5;        /* Flags, set 5  PernAngband */
        u32b art_esp;           /* Flags, set esp  PernAngband */
#endif	// 0
	
	u16b next_o_idx;	/* Next object in stack (if any) */
	u16b held_m_idx;	/* Monster holding us (if any) */
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
        bool special;                   /* Does it use a special r_info ? */
        monster_race *r_ptr;            /* The aforementionned r_info */

        s32b owner;                     /* Player owning it */

	s16b r_idx;			/* Monster race index */

	byte fy;			/* Y location on map */
	byte fx;			/* X location on map */

	struct worldpos wpos;

        s32b exp;                       /* Experience of the monster */
        s16b level;                     /* Level of the monster */

        monster_blow blow[4];           /* Up to four blows per round */
        byte speed;                     /* Monster "speed" - better s16b? */
        s16b ac;                        /* Armour Class */

        s32b hp;                        /* Current Hit points */
        s32b maxhp;                     /* Max Hit points */

	s16b csleep;		/* Inactive counter */

	byte mspeed;		/* Monster "speed" */
	s16b energy;		/* Monster "energy" */

	byte stunned;		/* Monster is stunned */
	byte confused;		/* Monster is confused */
	byte monfear;		/* Monster is afraid */

#if 0
	s16b bleeding;          /* Monster is bleeding */
	s16b poisoned;          /* Monster is poisoned */
#endif

	u16b hold_o_idx;	/* Object being held (if any) */

	byte cdis;			/* Current dis from player */

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
	u32b hflags1;                    /* Flags 1 */
	u32b hflags2;                    /* Flags 1 */
	u32b hflags3;                    /* Flags 1 */
	u32b hflags7;                    /* Flags 1 */
	u32b hflags8;                    /* Flags 1 */
	u32b hflags9;                    /* Flags 1 */

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
	byte prob1;		/* Probability, pass 1 */
	byte prob2;		/* Probability, pass 2 */
	byte prob3;		/* Probability, pass 3 */

	u16b total;		/* Unused for now */
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

	s16b insult_cur;		/* Insult counter */

	s16b good_buy;			/* Number of "good" buys */
	s16b bad_buy;			/* Number of "bad" buys */

	s32b store_open;		/* Closed until this turn */

	s32b last_visit;		/* Last visited on this turn */

	byte stock_num;			/* Stock -- Number of entries */
	s16b stock_size;		/* Stock -- Total Size of Array */
	object_type *stock;		/* Stock -- Actual stock items */
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
//	u32b flags2;		/* LF2 flags */
	byte hgt;			/* Vault height */
	byte wid;			/* Vault width */
//	char feeling[80]	/* feeling description */

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
	rule_type rules[5];             /* Monster generation rules */
#else	// 0
	char r_char[10];	/* races allowed */
	char nr_char[10];	/* races prevented */
#endif	// 0
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
};

typedef struct wilderness_type wilderness_type;

struct wilderness_type
{
	u16b radius; /* the distance from the town */
	u16b type;   /* what kind of terrain we are in */

	u32b flags; /* various */
	struct dungeon_type *tower;
	struct dungeon_type *dungeon;
	s16b ondepth;
	time_t lastused;
	cave_type **cave;
	byte up_x, up_y;
	byte dn_x, dn_y;
	byte rn_x, rn_y;
	s32b own;	/* King owning the wild */
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

	u16b owners[4];                 /* List of owners(refers to ow_info) */

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

	s16b r_adj[6];		/* Racial stat bonuses */

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

        s32b choice;            /* Legal class choices */

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

	s16b c_adj[6];		/* Class stat modifier */

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
	s32b num;		/* Number of people in the party */
	s32b created;		/* Creation (or disband-tion) time */
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
	s32b num;		/* Number of guild members */
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
#define HF_RECT 0x02
#define HF_STOCK 0x04		/* house generated by town */
#define HF_NOFLOOR 0x08		/* courtyard generated without floor */
#define HF_JAIL 0x10		/* generate with CAVE_STCK */
#define HF_APART 0x20		/* build apartment (obsolete - DELETEME) */
#define HF_TRAD	0x40		/* Vanilla style house */
#define HF_DELETED 0x80		/* Ruined house - do not save/reload */

#define MAXCOORD 200		/* Maximum vertices on non-rect house */

struct house_type{
	byte x,y;		/* Absolute starting coordinates */
	byte dx,dy;		/* door coords */
	struct dna_type *dna;	/* house dna door information */
	u16b flags;		/* house flags - HF_xxxx */
	struct worldpos wpos;
	union{
		struct{ byte width, height; }rect;
		char *poly;	/* coordinate array for non rect houses */
	}coords;
#ifndef USE_MANG_HOUSE_ONLY
	s16b stock_num;			/* Stock -- Number of entries */
	s16b stock_size;		/* Stock -- Total Size of Array */
	object_type *stock;		/* Stock -- Actual stock items */
#endif	// USE_MANG_HOUSE
};

#define OT_PLAYER 1
#define OT_PARTY 2
#define OT_CLASS 3
#define OT_RACE 4
#define OT_GUILD 5

#define ACF_NONE 0x00
#define ACF_PARTY 0x01
#define ACF_CLASS 0x02
#define ACF_RACE 0x04
#define ACF_LEVEL 0x08

struct dna_type{
	u32b creator;		/* Unique ID of creator/house admin */
	u32b owner;		/* Player/Party/Class/Race ID */
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
#endif	// 0


/*
 * Information about a "hostility"
 */
typedef struct hostile_type hostile_type;

struct hostile_type
{
	s32b id;		/* ID of player we are hostile to */
	hostile_type *next;	/* Next in list */
};

/* remotely ignore players */
struct remote_ignore
{
	unsigned long id;		/* player unique id */
	short serverid;
	struct remote_ignore *next;	/* Next in list */
};

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
	u32b name;                              /* Name */
	u32b desc;                              /* Description */
	u32b action_desc;                       /* Action Description */

	s16b action_mkey;                       /* Action do to */

	s16b rate;                              /* Modifier decreasing rate */

//	s16b action[MAX_SKILLS][2];             /* List of actions against other skills in th form: action[x] = {SKILL_FOO, 10} */
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
	s32b value;                             /* Actual value */
	u16b mod;                               /* Modifier(1 skill point = modifier skill) */
	bool dev;                               /* Is the branch developped ? */
        bool hidden;                            /* Innactive */
	bool touched;				/* need refresh? */

};

/* account flags */
#define ACC_TRIAL 0x0001	/* Account is awaiting validation */
#define ACC_ADMIN 0x0002	/* Account members are admins */
#define ACC_MULTI 0x0004	/* Simultaneous play */
#define ACC_NOSCORE 0x0008	/* No scoring allowed */
#define ACC_DELD  0x8000	/* Delete account/members */

/*
 * new account struct - pass in player_type will be removed
 * this will provide a better account management system
 */
struct account{
	u32b id;	/* account id */
	u16b flags;	/* account flags */
	char name[30];	/* login */
	char pass[20];	/* some crypts are not 13 */
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
	char basename[MAX_CHARS];
	char realname[MAX_CHARS];	/* Userid */
	char hostname[MAX_CHARS];	/* His hostname */
	char addr[MAX_CHARS];		/* His IP address */
	unsigned int version;		/* His version */

	s32b id;		/* Unique ID to each player */
	u32b account;		/* account group id */
	u32b dna;		/* DNA - psuedo unique to each player life */
	s32b turn;		/* Player's birthday */
	time_t msg;		/* anti spamming protection */
	byte msgcnt;
	byte spam;
	byte talk;		/* talk too much (moltors idea) */

	hostile_type *hostile;	/* List of players we wish to attack */

	char savefile[MAX_PATH_LENGTH];	/* Name of the savefile */

	bool inval;		/* Non validated account */
	bool alive;		/* Are we alive */
	bool death;		/* Have we died */
	int deathblow;          /* How much damage the final blow afflicted */
	s16b ghost;		/* Are we a ghost */
	s16b fruit_bat;		/* Are we a fruit bat */
	byte lives;         /* number of times we have ressurected */

	byte prace;			/* Race index */
	byte pclass;		/* Class index */
	byte male;			/* Sex of character */
        byte oops;			/* Unused */

        skill_player s_info[MAX_SKILLS]; /* Player skills */
        s16b skill_points;      /* number of skills assignable */
//        s16b skill_last_level;  /* last level we gained a skill point */
	
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

	s32b au;			/* Current Gold */

	s32b max_exp;		/* Max experience */
	s32b exp;			/* Cur experience */
	u16b exp_frac;		/* Cur exp frac (times 2^16) */

	s16b lev;			/* Level */

	s16b mhp;			/* Max hit pts */
	s16b chp;			/* Cur hit pts */
	u16b chp_frac;		/* Cur hit frac (times 2^16) */

	s16b player_hp[PY_MAX_LEVEL];

	s16b msp;			/* Max mana pts */
	s16b csp;			/* Cur mana pts */
	u16b csp_frac;		/* Cur mana frac (times 2^16) */

	object_type *inventory;	/* Player's inventory */

	s16b total_weight;	/* Total weight being carried */

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

	s16b cur_hgt;		/* Height and width of their dungeon level */
	s16b cur_wid;

	bool new_level_flag;	/* Has this player changed depth? */
	byte new_level_method;	/* Climb up stairs, down, or teleport level? */

	byte party;		/* The party he belongs to (or 0 if neutral) */
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

//	cptr info[128];		/* Temp storage of *ID* and Self Knowledge info */
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

	bool trap_ident[MAX_T_IDX];       /* do we know the name */
//	s16b trap_known[MAX_T_IDX];       /* how well is this trap known */

//	bool options[64];	/* Player's options */
//	bool options[OPT_MAX];	/* obsolete */
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
	// bool speak_unique;

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
	cave_view_type scr_info[SCREEN_HGT + 20][SCREEN_WID + 24];
	
	u32b mimic_seed;	/* seed for random mimic immunities etc. */
	
	char died_from[80];	/* What off-ed him */
	char died_from_list[80]; /* what goes on the high score list */
	s16b died_from_depth;	/* what depth we died on */

	u16b total_winner;	/* Is this guy the winner */
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
  s16b current_artifact;
  object_type *current_telekinesis;
	s16b current_curse;

	s16b current_selling;
	s16b current_sell_amt;
	int current_sell_price;

	int store_num;		/* What store this guy is in */

	s16b fast;			/* Timed -- Fast */
	s16b fast_mod;   		/* Timed -- Fast */
	s16b slow;			/* Timed -- Slow */
	s16b blind;			/* Timed -- Blindness */
	s16b paralyzed;		/* Timed -- Paralysis */
	s16b confused;		/* Timed -- Confusion */
	s16b afraid;		/* Timed -- Fear */
	s16b image;			/* Timed -- Hallucination */
	s16b poisoned;		/* Timed -- Poisoned */
	s16b cut;			/* Timed -- Cut */
	s16b stun;			/* Timed -- Stun */

	s16b protevil;		/* Timed -- Protection */
	s16b invuln;		/* Timed -- Invulnerable */
	s16b hero;			/* Timed -- Heroism */
	s16b shero;			/* Timed -- Super Heroism */
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
	bool wraith_in_wall;
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

  s16b auto_tunnel;
  s16b body_monster;

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
	bool old_cumber_glove;
	bool old_heavy_wield;
	bool old_heavy_shoot;
	bool old_icky_wield;
	bool old_awkward_wield;
	bool old_cumber_weight;

	s16b old_lite;		/* Old radius of lite (if any) */
	s16b old_view;		/* Old radius of view (if any) */

	s16b old_food_aux;	/* Old value of food */


	bool cumber_armor;	/* Mana draining armor */
	bool cumber_glove;	/* Mana draining gloves */
	bool heavy_wield;	/* Heavy weapon */
	bool heavy_shoot;	/* Heavy shooter */
	bool icky_wield;	/* Icky weapon */
	bool awkward_wield;	/* shield and COULD_2H weapon */
	bool cumber_weight;	/* Full weight. FA from MA will be lost if overloaded */

	s16b cur_lite;		/* Radius of lite (if any) */


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

	bool exp_drain;		/* Experience draining */

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
//	bool telepathy;		/* Telepathy */
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
	s16b dis_to_a;		/* Known bonus to ac */

	s16b dis_ac;		/* Known base ac */

	s16b to_h_ranged;			/* Bonus to hit */
	s16b to_d_ranged;			/* Bonus to dam */
	s16b to_h_melee;			/* Bonus to hit */
	s16b to_d_melee;			/* Bonus to dam */
	s16b to_h;			/* Bonus to hit */
	s16b to_d;			/* Bonus to dam */
	s16b to_a;			/* Bonus to ac */

	s16b ac;			/* Base ac */

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

	s32b innate_spells[3]; /* Monster spells */
	bool body_changed;
	
	bool anti_magic;	/* Can the player resist magic */

	s32b blood_bond; /* Norc is now happy :) */

	byte mode; /* Difficulty MODE */

	s32b esp_link; /* Mental link */
	byte esp_link_type;
	u16b esp_link_flags;
	u16b esp_link_end; /* Time before actual end */
	bool (*master_move_hook)(int Ind, char * args);

	/* some new borrowed flags (saved) */
	bool black_breath;      /* The Tolkien's Black Breath */
	bool black_breath_tmp;	/* (NOT saved) BB induced by an item */
	//        u32b malady;      /* TODO: Flags for malady */

	s16b msane;                   /* Max sanity */
	s16b csane;                   /* Cur sanity */
	u16b csane_frac;              /* Cur sanity frac */

	/* elements under this line won't be saved...for now. - Jir - */
	hostile_type	*ignore;  /* List of players whose chat we wish to ignore */
	struct remote_ignore	*w_ignore;  /* List of players whose chat we wish to ignore */
	bool	afk;		/* player is afk */
	bool	use_r_gfx;	/* hack - client uses gfx? */

	byte drain_mana;        /* mana draining */
	byte drain_life;        /* hp draining */

	bool sensible_fire;     /* Fire does more damage on the player */
	bool sensible_cold;     /* Cold does more damage on the player */
	bool sensible_acid;     /* Acid does more damage on the player */
	bool sensible_elec;     /* Electricity does more damage on the player */
	bool sensible_pois;     /* Poison does more damage on the player */

	bool reflect;       /* Reflect 'bolt' attacks */
	bool no_cut;	    /* For mimic forms */
	bool sh_fire;       /* Fiery 'immolation' effect */
	bool sh_elec;       /* Electric 'immolation' effect */
	bool wraith_form;   /* wraithform */
	bool immune_neth;       /* Immunity to nether */
	bool climb;             /* Can climb mountains */
	bool fly;               /* Can fly over some features */
	bool can_swim;		/* Can swim like a fish (or Lizard..whatever) */
	bool pass_trees;	/* Can pass thick forest */
	
	int luck_cur;	/* Extra luck of this player */

	//        byte anti_magic_spell;    /* Anti-magic(newer one..) */
	byte antimagic;    		/* Anti-magic(in percent) */
	byte antimagic_dis;     /* Radius of the anti magic field */
	bool anti_tele;     /* Prevent any teleportation + phasing + recall */
	bool res_tele;		/* Prevents being teleported from someone else */
	/* in PernM, it's same as st_anchor */
	bool resist_continuum;	/* non-timed -- Space/Time Anchor */
	bool admin_wiz;		/* Is this char Wizard? */
	bool admin_dm;		/* or Dungeon Master? */
	bool stormbringer;	/* Attack friends? */

	u16b quest_id;		/* Quest number */
	s16b quest_num;		/* Number of kills needed */

	s16b xtra_crit;         /* % of increased crits */
	s16b extra_blows;		/* Number of extra blows */

	s16b to_l;                      /* Bonus to life */
	s16b to_m;                      /* Bonus to mana */
	//        s16b to_s;                      /* Bonus to spell(num_spell) */
	s16b dodge_chance;		/* Chance of dodging blows/missiles */

	s32b balance;		/* Deposit/debt */
	s32b tim_blacklist;		/* Player is on the 'Black List' */
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
#endif	// 0

        byte spell_project;             /* Do the spells(some) affect nearby party members ? */

        /* Special powers */
        s16b powers[MAX_POWERS];        /* What powers do we possess? */
        s16b power_num;                 /* How many */
};

/* For Monk martial arts */

typedef struct martial_arts martial_arts;

struct martial_arts
{
    cptr    desc;    /* A verbose attack description */
    int     min_level;  /* Minimum level to use */
    int     chance;     /* Chance of 'success' */
    int     dd;        /* Damage dice */
    int     ds;        /* Damage sides */
    int     effect;     /* Special effects */
};

/* Define monster generation rules */
typedef struct rule_type rule_type;
struct rule_type
{
	byte mode;                      /* Mode of combinaison of the monster flags */
	byte percent;                   /* Percent of monsters affected by the rule */

	u32b mflags1;                   /* The monster flags that are allowed */
	u32b mflags2;
	u32b mflags3;
	u32b mflags4;
	u32b mflags5;
	u32b mflags6;
	u32b mflags7;
	u32b mflags8;
	u32b mflags9;

	char r_char[5];                 /* Monster race allowed */
};

/* A structure for the != dungeon types */
typedef struct dungeon_info_type dungeon_info_type;
struct dungeon_info_type
{
	u32b name;                      /* Name */
	u32b text;                      /* Description */
	char short_name[3];             /* Short name */

	s16b floor1;                    /* Floor tile 1 */
	byte floor_percent1[2];         /* Chance of type 1 */
	s16b floor2;                    /* Floor tile 2 */
	byte floor_percent2[2];         /* Chance of type 2 */
	s16b floor3;                    /* Floor tile 3 */
	byte floor_percent3[2];         /* Chance of type 3 */
	s16b outer_wall;                /* Outer wall tile */
	s16b inner_wall;                /* Inner wall tile */
	s16b fill_type1;                /* Cave tile 1 */
	byte fill_percent1[2];          /* Chance of type 1 */
	s16b fill_type2;                /* Cave tile 2 */
	byte fill_percent2[2];          /* Chance of type 2 */
	s16b fill_type3;                /* Cave tile 3 */
	byte fill_percent3[2];          /* Chance of type 3 */
	byte fill_method;				/* Smoothing parameter for the above */

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
	char * meta_address;
	s16b meta_port;
	
	char * bind_name;
	char * console_password;
	char * admin_wizard;
	char * dungeon_master;
	char * wserver;
	
	char * pass;
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
	char vanilla_monsters;
	char pet_monsters;
	bool report_to_meta;
	bool secret_dungeon_master;

	bool anti_arts_hoard;
	bool anti_arts_house;
	bool anti_arts_shop;
	bool anti_arts_pickup;
	bool anti_arts_send;

	bool anti_cheeze_pickup;
	s16b surface_item_removal; /* minutes before items are cleared */
	s16b dungeon_shop_chance;
	s16b dungeon_shop_type;
	s16b dungeon_shop_timeout;

	bool mage_hp_bonus;	/* DELETEME (replace it, that is) */
	char door_bump_open;
	bool no_ghost;
	int lifes;		/* number of times a ghost player can be resurrected */
	bool maximize;
	bool kings_etiquette;

	bool public_rfe;
	bool auto_purge;
	bool log_u;
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
	bool dungeon_align;
	bool dungeon_stair;
	bool recall_flicker;
/*	bool flow_by_sound;	*/
	bool flow_by_smell;
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

	//bool dummy_option;	/* for options not used in client */
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
//	s16b store_num;			/* XXX makeshift - will be removed soon */

	/* list of command */
//	store_action_type actions[6];
//	byte num_actions;
	u16b actions[6];                /* Actions(refers to ba_info) */
	u16b bact[6];                /* ba_ptr->action */
	char action_name[6][40];
	char action_attr[6];
	u16b action_restr[6];
	char letter[6];
	s16b cost[6];
	byte flags[6];
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
};

typedef struct school_type school_type;
struct school_type
{
        cptr name;                      /* Name */
        s16b skill;                     /* Skill used for that school */
};
