/* $Id$ */
/* party.h - evileye*/
/* The struct to hold a data entry */

typedef struct hash_entry hash_entry;

struct hash_entry
{
	int id;				/* The character ID */
	u32b account;			/* account id */
	cptr name;			/* Player name */
	byte race,class;		/* Race/class */
	
	/* new in savegame version 4.2.2 (4.2.0c server) - C. Blue */
	byte mode;			/* Character mode (for account overview screen) */

	/* new in 3.4.2 */
	byte level;			/* Player maximum level */
	/* changed from byte to u16b - mikaelh */
	u16b party;			/* Player party */
	/* 3.5.0 */
	byte guild;			/* Player guild */
	s16b quest;			/* Player quest */

	time_t laston;			/* Last on time */

#ifdef AUCTION_SYSTEM
	s32b au;
	s32b balance;
#endif

	struct hash_entry *next;	/* Next entry in the chain */
};

/* lookup function */
hash_entry *lookup_player(int id);
