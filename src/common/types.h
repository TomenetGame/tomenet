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



/*
 * Information about terrain "features"
 */

typedef struct feature_type feature_type;

struct feature_type
{
	u16b name;			/* Name (offset) */
	u16b text;			/* Text (offset) */

	byte mimic;			/* Feature to mimic */

	byte extra;			/* Extra byte (unused) */

	s16b unused;		/* Extra bytes (unused) */

	byte f_attr;		/* Object "attribute" */
	char f_char;		/* Object "symbol" */

	byte z_attr;		/* The desired attr for this feature */
	char z_char;		/* The desired char for this feature */
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

	byte level;			/* Artifact level */
	byte rarity;		/* Artifact rarity */

	byte cur_num;		/* Number created (0 or 1) */
	byte max_num;		/* Unused (should be "1") */
};


/*
 * Information about "ego-items".
 */

typedef struct ego_item_type ego_item_type;

struct ego_item_type
{
	u16b name;			/* Name (offset) */
	u16b text;			/* Text (offset) */

	byte slot;			/* Standard slot value */
	byte rating;		/* Rating boost */

	byte level;			/* Minimum level */
	byte rarity;		/* Object rarity */

	char max_to_h;		/* Maximum to-hit bonus */
	char max_to_d;		/* Maximum to-dam bonus */
	char max_to_a;		/* Maximum to-ac bonus */

	char max_pval;		/* Maximum pval */

	s32b cost;			/* Ego-item "cost" */

	u32b flags1;		/* Ego-Item Flags, set 1 */
	u32b flags2;		/* Ego-Item Flags, set 2 */
	u32b flags3;		/* Ego-Item Flags, set 3 */
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

	s16b extra;				/* Unused (for now) */

	byte freq_inate;		/* Inate spell frequency */
	byte freq_spell;		/* Other spell frequency */

	u32b flags1;			/* Flags 1 (general) */
	u32b flags2;			/* Flags 2 (abilities) */
	u32b flags3;			/* Flags 3 (race/resist) */
	u32b flags4;			/* Flags 4 (inate/breath) */
	u32b flags5;			/* Flags 5 (normal spells) */
	u32b flags6;			/* Flags 6 (special spells) */

	monster_blow blow[4];	/* Up to four blows per round */


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

typedef struct cave_type cave_type;

struct cave_type
{
	byte info;		/* Hack -- cave flags */

	byte feat;		/* Hack -- feature type */

	s16b o_idx;		/* Item index (in o_list) or zero */

	s16b m_idx;		/* Monster index (in m_list) or zero */
				/* or negative if a player */

#ifdef MONSTER_FLOW

	byte cost;		/* Hack -- cost of flowing */
	byte when;		/* Hack -- when cost was computed */

#endif
	void *special;		/* Special pointer to various struct */

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
	s16b k_idx;			/* Kind index (zero if "dead") */

	byte iy;			/* Y-position on map, or zero */
	byte ix;			/* X-position on map, or zero */

	s32b dun_depth;			/* Depth into the dungeon */

	byte tval;			/* Item type (from kind) */
	byte sval;			/* Item sub-type (from kind) */

	s32b bpval;			/* Base item extra-parameter */
	s32b pval;			/* Extra enchantment item extra-parameter */

	byte discount;		/* Discount (if any) */

	byte number;		/* Number of items */

	s16b weight;		/* Item weight */

	byte name1;			/* Artifact type, if any */
	byte name2;			/* Ego-Item type, if any */
	s32b name3;			/* Randart seed, if any */

	byte xtra1;			/* Extra info type */
	byte xtra2;			/* Extra info index */

	s16b to_h;			/* Plusses to hit */
	s16b to_d;			/* Plusses to damage */
	s16b to_a;			/* Plusses to AC */

	s16b ac;			/* Normal AC */

	byte dd, ds;		/* Damage dice/sides */

	s16b timeout;		/* Timeout Counter */

	byte ident;			/* Special flags  */

	/*byte marked;	*/	/* Object is marked */

	u16b note;			/* Inscription index */
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

	s16b dun_depth;			/* Level of the dungeon */

        s32b exp;                       /* Experience of the monster */
        s16b level;                     /* Level of the monster */

        monster_blow blow[4];           /* Up to four blows per round */
        byte speed;                     /* Monster "speed" */
        s16b ac;                        /* Armour Class */

        s32b hp;                        /* Current Hit points */
        s32b maxhp;                     /* Max Hit points */

	s16b csleep;		/* Inactive counter */

	byte mspeed;		/* Monster "speed" */
	s16b energy;		/* Monster "energy" */

	byte stunned;		/* Monster is stunned */
	byte confused;		/* Monster is confused */
	byte monfear;		/* Monster is afraid */

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
	bool clone;

        byte mind;                      /* Current action -- golems */
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
typedef struct setup_t setup_t;

struct setup_t
{
	s16b frames_per_second;
	int motd_len;
	int setup_size;
	char motd[80 * 23];
};

/*
 * The setup data that the client transmits to the
 * server.
 */
typedef struct client_setup_t client_setup_t;

struct client_setup_t
{
	bool options[64];

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


/* Adding this structure so we can have different creatures generated
   in different types of wilderness... this will probably be completly
   redone when I do a proper landscape generator.
   -APD-
*/

typedef struct wilderness_type wilderness_type;

struct wilderness_type
{
	int world_x; /* the world coordinates */
	int world_y;
	int radius; /* the distance from the town */
	int type;   /* what kind of terrain we are in */
	u16b flags; /* various */

	s32b own;	/* King owning the wild */
};


/*
 * A store owner
 */

typedef struct owner_type owner_type;

struct owner_type
{
	cptr owner_name;	/* Name */

	s32b max_cost;		/* Purse limit */

	byte max_inflate;	/* Inflation (max) */
	byte min_inflate;	/* Inflation (min) */

	byte haggle_per;	/* Haggle unit */

	byte insult_max;	/* Insult limit */

	byte owner_race;	/* Owner race */

	byte unused;		/* Unused */
};




/*
 * A store, with an owner, various state flags, a current stock
 * of items, and a table of items that are often purchased.
 */

typedef struct store_type store_type;

struct store_type
{
	byte owner;				/* Owner index */
	byte extra;				/* Unused for now */

	s16b insult_cur;		/* Insult counter */

	s16b good_buy;			/* Number of "good" buys */
	s16b bad_buy;			/* Number of "bad" buys */

	s32b store_open;		/* Closed until this turn */

	s32b store_wrap;		/* Unused for now */

	s16b table_num;			/* Table -- Number of entries */
	s16b table_size;		/* Table -- Total Size of Array */
	s16b *table;			/* Table -- Legal item kinds */

	s16b stock_num;			/* Stock -- Number of entries */
	s16b stock_size;		/* Stock -- Total Size of Array */
	object_type *stock;		/* Stock -- Actual stock items */
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
	s16b spell_xtra;		/* Something for later */

	s16b spell_stat;		/* Stat for spells (if any)  */
	s16b spell_type;		/* Spell type (mage/priest) */

	s16b spell_first;		/* Level of first spell */
	s16b spell_weight;		/* Weight that hurts spells */

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

	byte choice;		/* Legal class choices */
};


/*
 * Player class info
 */

typedef struct player_class player_class;

struct player_class
{
	cptr title;			/* Type of class */

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
 * Information about a "house"
 */
typedef struct house_type house_type;

/*
In order to delete the contents of a house after its key is lost,
added x_1, y_1, x_2, y_2, which are the locations of the opposite 
corners of the house.
-APD-
*/

struct house_type
{
	byte x_1;
	byte y_1; 
	byte x_2;
	byte y_2;
	
	byte door_y;		/* Location of door */
	byte door_x;
	byte strength;		/* Strength of door (unused) */
	byte owned;		/* Currently owned? */

	int depth;

	s32b price;		/* Cost of buying */
};

#define OT_PLAYER 1
#define OT_PARTY 2
#define OT_CLASS 3
#define OT_RACE 4

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
};


/*
 * Information about a "hostility"
 */
typedef struct hostile_type hostile_type;

struct hostile_type
{
	s32b id;		/* ID of player we are hostile to */
	hostile_type *next;	/* Next in list */
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


typedef struct player_type player_type;

struct player_type
{
	int conn;			/* Connection number */
	char name[MAX_CHARS];		/* Nickname */
	char pass[MAX_CHARS];		/* Password */
	char basename[MAX_CHARS];
	char realname[MAX_CHARS];	/* Userid */
	char hostname[MAX_CHARS];	/* His hostname */
	char addr[MAX_CHARS];		/* His IP address */
	unsigned int version;		/* His version */

	s32b id;		/* Unique ID to each player */
#ifdef NEWHOUSES
	u32b dna;		/* DNA - psuedo unique to each player life */
#endif

	hostile_type *hostile;	/* List of players we wish to attack */

	char savefile[1024];	/* Name of the savefile */

	bool alive;		/* Are we alive */
	bool death;		/* Have we died */
	s16b ghost;		/* Are we a ghost */
	s16b fruit_bat;		/* Are we a fruit bat */
	byte lives;         /* number of times we have ressurected */

	byte prace;			/* Race index */
	byte pclass;		/* Class index */
	byte male;			/* Sex of character */
	byte oops;			/* Unused */
	
	s16b class_extra;	/* Class extra info */

	byte hitdie;		/* Hit dice (sides) */
	s16b expfact;		/* Experience factor */

	byte maximize;		/* Maximize stats */
	byte preserve;		/* Preserve artifacts */

	s16b age;			/* Characters age */
	s16b ht;			/* Height */
	s16b wt;			/* Weight */
	s16b sc;			/* Social Class */


	player_race *rp_ptr;		/* Pointers to player tables */
	player_class *cp_ptr;
	player_magic *mp_ptr;

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
	s16b max_dlv;		/* Max level explored */
	s16b recall_depth;	/* which depth to recall to */

	s16b stat_max[6];	/* Current "maximal" stat values */
	s16b stat_cur[6];	/* Current "natural" stat values */

	char history[4][60];	/* The player's "history" */

	s16b world_x;	/* The wilderness x coordinate */
	s16b world_y;	/* The wilderness y coordinate */

	unsigned char wild_map[(MAX_WILD/8)]; /* the wilderness we have explored */

	s16b py;		/* Player location in dungeon */
	s16b px;
	s32b dun_depth;		/* Player depth -- wilderness level offset */

	s16b cur_hgt;		/* Height and width of their dungeon level */
	s16b cur_wid;

	bool new_level_flag;	/* Has this player changed depth? */
	byte new_level_method;	/* Climb up stairs, down, or teleport level? */

	byte party;		/* The party he belongs to (or 0 if neutral) */

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

	cptr info[128];		/* Temp storage of *ID* and Self Knowledge info */
	byte special_file_type;	/* Is he using *ID* or Self Knowledge? */

	byte cave_flag[MAX_HGT][MAX_WID]; /* Can the player see this grid? */

	bool mon_vis[MAX_M_IDX];  /* Can this player see these monsters? */
	bool mon_los[MAX_M_IDX];

	bool obj_vis[MAX_O_IDX];  /* Can this player see these objcets? */

	bool play_vis[MAX_PLAYERS];	/* Can this player see these players? */
	bool play_los[MAX_PLAYERS];

	bool obj_aware[MAX_K_IDX]; /* Is the player aware of this obj type? */
	bool obj_tried[MAX_K_IDX]; /* Has the player tried this obj type? */

	bool options[64];	/* Player's options */
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
	
	char died_from[80];	/* What off-ed him */
	char died_from_list[80]; /* what goes on the high score list */
	s16b died_from_depth;	/* what depth we died on */

	u16b total_winner;	/* Is this guy the winner */
	s16b own1, own2;	/* IF we are a king what do we own ? */
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
  s16b current_mind;	/* Power being use */
	s16b current_rod;	/* Rod being zapped */
	s16b current_activation;/* Artifact (or dragon mail) being activated */
	s16b current_enchant_h; /* Current enchantments */
	s16b current_enchant_d;
	s16b current_enchant_a;
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
	s16b furry;			/* Timed -- Furry */
	s16b shield;		/* Timed -- Shield Spell */
	s16b blessed;		/* Timed -- Blessed */
	s16b tim_invis;		/* Timed -- See Invisible */
	s16b tim_infra;		/* Timed -- Infra Vision */
	s16b tim_wraith;	/* Timed -- Wraithform */
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

	s16b new_spells;	/* Number of spells available */

	s16b old_spells;

	u32b spell_learned1;	/* bit mask of spells learned */
	u32b spell_learned2;	/* bit mask of spells learned */
	u32b spell_worked1;	/* bit mask of spells tried and worked */
	u32b spell_worked2;	/* bit mask of spells tried and worked */
	u32b spell_forgotten1;	/* bit mask of spells learned but forgotten */
	u32b spell_forgotten2;	/* bit mask of spells learned but forgotten */
	byte spell_order[64];	/* order spells learned/remembered/fogotten */

	bool old_cumber_armor;
	bool old_cumber_glove;
	bool old_heavy_wield;
	bool old_heavy_shoot;
	bool old_icky_wield;

	s16b old_lite;		/* Old radius of lite (if any) */
	s16b old_view;		/* Old radius of view (if any) */

	s16b old_food_aux;	/* Old value of food */


	bool cumber_armor;	/* Mana draining armor */
	bool cumber_glove;	/* Mana draining gloves */
	bool heavy_wield;	/* Heavy weapon */
	bool heavy_shoot;	/* Heavy shooter */
	bool icky_wield;	/* Icky weapon */

	s16b cur_lite;		/* Radius of lite (if any) */


	u32b notice;		/* Special Updates (bit flags) */
	u32b update;		/* Pending Updates (bit flags) */
	u32b redraw;		/* Normal Redraws (bit flags) */
	u32b window;		/* Window Redraws (bit flags) */

	s16b stat_use[6];	/* Current modified stats */
	s16b stat_top[6];	/* Maximal modified stats */

	s16b stat_add[6];	/* Modifiers to stat values */
	s16b stat_ind[6];	/* Indexes into stat tables */

	bool immune_acid;	/* Immunity to acid */
	bool immune_elec;	/* Immunity to lightning */
	bool immune_fire;	/* Immunity to fire */
	bool immune_cold;	/* Immunity to cold */

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
	bool hold_life;		/* Resist life draining */
	bool telepathy;		/* Telepathy */
	bool slow_digest;	/* Slower digestion */
	bool bless_blade;	/* Blessed blade */
	bool xtra_might;	/* Extra might bow */
	bool impact;		/* Earthquake blows */
  bool auto_id; /* Pickup = Id */
	
	s16b invis;		/* Invisibility */

	s16b dis_to_h;		/* Known bonus to hit */
	s16b dis_to_d;		/* Known bonus to dam */
	s16b dis_to_a;		/* Known bonus to ac */

	s16b dis_ac;		/* Known base ac */

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
