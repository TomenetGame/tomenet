/* party.h - evileye*/
/* The struct to hold a data entry */

typedef struct hash_entry hash_entry;

struct hash_entry
{
	int id;				/* The ID */
	u32b account;		/* account id */	/* right? (Jir) */
	cptr name;			/* Player name */

	/* new in 3.4.2 */
	byte level;			/* Player maximum level */
	byte party;			/* Player party */
	/* 3.5.0 */
	byte guild;			/* Player guild */
	s16b quest;			/* Player quest */

	time_t laston;			/* Last on time */
	struct hash_entry *next;	/* Next entry in the chain */
};

/* lookup function */
hash_entry *lookup_player(int id);
