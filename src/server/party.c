/*
 * Support for the "party" system.
 */

#include "angband.h"

/*
 * Lookup a party number by name.
 */
int party_lookup(cptr name)
{
	int i;

	/* Check each party */
	for (i = 0; i < MAX_PARTIES; i++)
	{
		/* Check name */
		if (streq(parties[i].name, name))
			return i;
	}

	/* No match */
	return -1;
}

/*
 * Check for the existance of a player in a party.
 */
bool player_in_party(int party_id, int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* Check */
	if (p_ptr->party == party_id)
		return TRUE;

	/* Not in the party */
	return FALSE;
}
	
/*
 * Create a new party, owned by "Ind", and called
 * "name".
 */
int party_create(int Ind, cptr name)
{
	player_type *p_ptr = Players[Ind];
	int index = 0, i, oldest = turn;

	/* Check for already existing party by that name */
	if (party_lookup(name) != -1)
	{
		msg_print(Ind, "A party by that name already exists.");
		return FALSE;
	}

	/* Make sure this guy isn't in some other party already */
	if (p_ptr->party != 0)
	{
		msg_print(Ind, "You already belong to a party!");
		return FALSE;
	}

	/* Find the "best" party index */
	for (i = 1; i < MAX_PARTIES; i++)
	{
		/* Check deletion time of disbanded parties */
		if (parties[i].num == 0 && parties[i].created < oldest)
		{
			/* Track oldest */
			oldest = parties[i].created;
			index = i;
		}
	}

	/* Make sure we found an empty slot */
	if (index == 0 || oldest == turn)
	{
		/* Error */
		msg_print(Ind, "There aren't enough party slots!");
		return FALSE;
	}

	/* Set party name */
	strcpy(parties[index].name, name);

	/* Set owner name */
	strcpy(parties[index].owner, p_ptr->name);

	/* Add the owner as a member */
	p_ptr->party = index;
	parties[index].num++;

	/* Set the "creation time" */
	parties[index].created = turn;

	/* Resend party info */
	Send_party(Ind);

	/* Success */
	return TRUE;
}

/*
 * Add a player to a party.
 */
int party_add(int adder, cptr name)
{
	player_type *p_ptr;
	player_type *q_ptr = Players[adder];
	int party_id = q_ptr->party, Ind = 0, i;

	/* Find name */
	for (i = 1; i <= NumPlayers; i++)
	{
		/* Check this one */
		if (streq(name, Players[i]->name))
		{
			/* Found him */
			Ind = i;
			break;
		}
	}

	/* Check for existance */
	if (Ind == 0)
	{
		/* Oops */
		msg_print(adder, "That player is not currently in the game.");

		return FALSE;
	}

	/* Set pointer */
	p_ptr = Players[Ind];

	/* Make sure this isn't an imposter */
	if (!streq(parties[party_id].owner, q_ptr->name))
	{
		/* Message */
		msg_print(adder, "You must be the owner to add someone.");

		/* Abort */
		return FALSE;
	}

	/* Make sure this added person is neutral */
	if (p_ptr->party != 0)
	{
		/* Message */
		msg_print(adder, "That player is already in a party.");

		/* Abort */
		return FALSE;
	}

	/* Tell the party about its new member */
	party_msg_format(party_id, "%s has been added to party %s.", p_ptr->name, parties[party_id].name);

	/* One more player in this party */
	parties[party_id].num++;

	/* Tell him about it */
	msg_format(Ind, "You've been added to party '%s'.", parties[party_id].name);

	/* Set his party number */
	p_ptr->party = party_id;

	/* Resend info */
	Send_party(Ind);

	/* Success */
	return TRUE;
}

/*
 * Remove a person from a party.
 *
 * Removing the party owner destroys the party.
 */
int party_remove(int remover, cptr name)
{
	player_type *p_ptr;
	player_type *q_ptr = Players[remover];
	int party_id = q_ptr->party, Ind = 0, i;

	/* Find name */
	for (i = 1; i <= NumPlayers; i++)
	{
		/* Check this one */
		if (streq(name, Players[i]->name))
		{
			/* Found him */
			Ind = i;
			p_ptr = Players[Ind];
			break;
		}
	}

	/* Check for existance */
	if (Ind == 0)
	{
		/* Oops */
		msg_print(remover, "That player is not currently in the game.");

		return FALSE;
	}

	/* Make sure this is the owner */
	if (!streq(parties[party_id].owner, q_ptr->name))
	{
		/* Message */
		msg_print(remover, "You must be the owner to delete someone.");

		/* Abort */
		return FALSE;
	}

	/* Make sure they were in the party to begin with */
	if (!player_in_party(party_id, Ind))
	{
		/* Message */
		msg_print(remover, "You can only delete party members.");

		/* Abort */
		return FALSE;
	}

	/* See if this is the owner we're deleting */
	if (remover == Ind)
	{
		/* Remove the party altogether */
		kill_houses(party_id, OT_PARTY);

		/* Set the number of people in this party to zero */
		parties[party_id].num = 0;

		/* Remove everyone else */
		for (i = 1; i <= NumPlayers; i++)
		{
			/* Check if they are in here */
			if (player_in_party(party_id, i))
			{
				Players[i]->party = 0;
				msg_print(i, "Your party has been disbanded.");
				Send_party(i);
			}
		}

		/* Set the creation time to "disbanded time" */
		parties[party_id].created = turn;
		
		/* Empty the name */
		strcpy(parties[party_id].name, "");
	}

	/* Keep the party, just lose a member */
	else
	{
		/* Lose a member */
		parties[party_id].num--;

		/* Set his party number back to "neutral" */
		p_ptr->party = 0;

		/* Messages */
		msg_print(Ind, "You have been removed from your party.");
		party_msg_format(party_id, "%s has been removed from the party.", p_ptr->name);

		/* Resend info */
		Send_party(Ind);
	}

	return TRUE;
}


/*
 * A player wants to leave a party.
 */
void party_leave(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int party_id = p_ptr->party;

	/* Make sure he belongs to a party */
	if (!party_id)
	{
		msg_print(Ind, "You don't belong to a party.");
		return;
	}

	/* If he's the owner, use the other function */
	if (streq(p_ptr->name, parties[party_id].owner))
	{
		/* Call party_remove */
		party_remove(Ind, p_ptr->name);
		return;
	}

	/* Lose a member */
	parties[party_id].num--;

	/* Set him back to "neutral" */
	p_ptr->party = 0;

	/* Inform people */
	msg_print(Ind, "You have been removed from your party.");
	party_msg_format(party_id, "%s has left the party.", p_ptr->name);

	/* Resend info */
	Send_party(Ind);
}


/*
 * Send a message to everyone in a party.
 */
void party_msg(int party_id, cptr msg)
{
	int i;

	/* Check for this guy */
	for (i = 1; i <= NumPlayers; i++)
	{
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* Check this guy */
		if (player_in_party(party_id, i))
			msg_print(i, msg);
	}
}

/*
 * Send a formatted message to a party.
 */
void party_msg_format(int party_id, cptr fmt, ...)
{
	va_list vp;
	char buf[1024];

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);

	/* Format the args, save the length */
	(void)vstrnfmt(buf, 1024, fmt, vp);

	/* End the Varargs Stuff */
	va_end(vp);

	/* Display */
	party_msg(party_id, buf);
}

/*
 * Split some experience among party members.
 *
 * This should ONLY be used while killing monsters.  The amount
 * should be the monster base experience times the monster level.
 *
 * This algorithm may need some work....  However, it does have some nifty
 * features, such as:
 *
 * 1) A party with just one member functions identically as before.
 *
 * 2) A party with two equally-levelled members functions such that each
 * member gets half as much experience as he would have by killing the monster
 * by himself.
 *
 * 3) Higher-leveled members of a party get higher percentages of the
 * experience.
 */
 
 /* The XP distribution was too unfair for low level characters,
    it made partying a real pain. I am changing it so that if the players
    have a difference in level of less than 5 than there is no difference
    in XP distribution. 
    
    I am also changing it so it divides by each players level, AFTER
    it has been given to them.
    
    UPDATE: it appears that it may be giving too much XP to the low lvl chars,
    but I have been too lazy to change it... however, this doesnt appear to be being
    abused much, and the new system is regardless much nicer than the old one.
    
    -APD-
    */

bool players_in_level(int Ind, int Ind2)
{
        if ((Players[Ind]->lev - Players[Ind2]->lev) > 7) return FALSE;
        if ((Players[Ind2]->lev - Players[Ind]->lev) > 7) return FALSE;
        return TRUE;
}

void party_gain_exp(int Ind, int party_id, s32b amount)
{
	player_type *p_ptr;
	int i, Depth = Players[Ind]->dun_depth;
	s32b new_exp, new_exp_frac, average_lev = 0, num_members = 0;
	s32b modified_level;

	/* Calculate the average level */
	for (i = 1; i <= NumPlayers; i++)
	{
		p_ptr = Players[i];

		if (p_ptr->conn == NOT_CONNECTED)
			continue;

		/* Check for his existance in the party */
                if (player_in_party(party_id, i) && (p_ptr->dun_depth == Depth) && players_in_level(Ind, i))
		{
			/* Increase the "divisor" */
			average_lev += p_ptr->lev;
			num_members++;
		}
	}

	/* Now, distribute the experience */
	for (i = 1; i <= NumPlayers; i++)
	{
		p_ptr = Players[i];

		if (p_ptr->conn == NOT_CONNECTED)
			continue;

		/* Check for existance in the party */
                if (player_in_party(party_id, i) && (p_ptr->dun_depth == Depth) && players_in_level(Ind, i))
		{
			/* Calculate this guy's experience */
			
			if (p_ptr->lev * num_members < average_lev) // below average
			{
				if ((average_lev - p_ptr->lev * num_members) > 2 * num_members )
				{
					modified_level = p_ptr->lev * num_members + 2 * num_members;
				}				
				else modified_level = average_lev;
			}
			else
			{
				if ((p_ptr->lev * num_members - average_lev) > 2 * num_members )
				{
					modified_level = p_ptr->lev * num_members - 2 * num_members;
				}				
				else modified_level = average_lev;
						
			}
			
			
			new_exp = (amount * modified_level) / (average_lev * num_members * p_ptr->lev);
			new_exp_frac = ((((amount * modified_level) % (average_lev * num_members * p_ptr->lev) )
			                * 0x10000L ) / (average_lev * num_members * p_ptr->lev)) + p_ptr->exp_frac;

			/* Keep track of experience */
			if (new_exp_frac >= 0x10000L)
			{
				new_exp++;
				p_ptr->exp_frac = new_exp_frac - 0x10000L;
			}
			else
			{
				p_ptr->exp_frac = new_exp_frac;
			}

			/* Gain experience */
			gain_exp(i, new_exp);
		}
	}
}

/*
 * Add a player to another player's list of hostilities.
 */
bool add_hostility(int Ind, cptr name)
{
	player_type *p_ptr = Players[Ind], *q_ptr;
	hostile_type *h_ptr;
	int i;

	/* Check for sillyness */
	if (streq(name, p_ptr->name))
	{
		/* Message */
		msg_print(Ind, "You cannot be hostile toward yourself.");

		return FALSE;
	}

	/* Search for player to add */
	for (i = 1; i <= NumPlayers; i++)
	{
		q_ptr = Players[i];

		/* Check name */
		if (!streq(q_ptr->name, name)) continue;

		/* Make sure players aren't in the same party */
		if (p_ptr->party && player_in_party(p_ptr->party, i))
		{
			/* Message */
			msg_format(Ind, "%^s is in your party!", q_ptr->name);

			return FALSE;
		}

		/* Ensure we don't add the same player twice */
		for (h_ptr = p_ptr->hostile; h_ptr; h_ptr = h_ptr->next)
		{
			/* Check this ID */
			if (h_ptr->id == q_ptr->id)
			{
				/* Message */
				msg_format(Ind, "You are already hostile toward %s.", q_ptr->name);

				return FALSE;
			}
		}

		/* Create a new hostility node */
		MAKE(h_ptr, hostile_type);

		/* Set ID in node */
		h_ptr->id = q_ptr->id;

		/* Put this node at the beginning of the list */
		h_ptr->next = p_ptr->hostile;
		p_ptr->hostile = h_ptr;

		/* Message */
		msg_format(Ind, "You are now hostile toward %s.", q_ptr->name);

		/* Success */
		return TRUE;
	}

	/* Search for party to add */
	if ((i = party_lookup(name)) != -1)
	{
		/* Ensure we don't add the same party twice */
		for (h_ptr = p_ptr->hostile; h_ptr; h_ptr = h_ptr->next)
		{
			/* Check this ID */
			if (h_ptr->id == 0 - i)
			{
				/* Message */
				msg_format(Ind, "You are already hostile toward party '%s'.", parties[i].name);

				return FALSE;
			}
		}

		/* Create a new hostility node */
		MAKE(h_ptr, hostile_type);

		/* Set ID in node */
		h_ptr->id = 0 - i;

		/* Put this node at the beginning of the list */
		h_ptr->next = p_ptr->hostile;
		p_ptr->hostile = h_ptr;

		/* Message */
		msg_format(Ind, "You are now hostile toward party '%s'.", parties[i].name);

		/* Success */
		return TRUE;
	}

	/* Couldn't find player */
	msg_format(Ind, "%^s is not currently in the game.", name);

	return FALSE;
}

/*
 * Remove an entry from a player's list of hostilities
 */
bool remove_hostility(int Ind, cptr name)
{
	player_type *p_ptr = Players[Ind];
	hostile_type *h_ptr, *i_ptr;
	cptr p;

	/* Initialize lock-step */
	i_ptr = NULL;

	/* Search entries */
	for (h_ptr = p_ptr->hostile; h_ptr; i_ptr = h_ptr, h_ptr = h_ptr->next)
	{
		/* Lookup name of this entry */
		if (h_ptr->id > 0)
		{
			/* Look up name */
			p = lookup_player_name(h_ptr->id);

			/* Check player name */
			if (p && streq(p, name))
			{
				/* Delete this entry */
				if (i_ptr)
				{
					/* Skip over */
					i_ptr->next = h_ptr->next;
				}
				else
				{
					/* Adjust beginning of list */
					p_ptr->hostile = h_ptr->next;
				}

				/* Message */
				msg_format(Ind, "No longer hostile toward %s.", name);

				/* Delete node */
				KILL(h_ptr, hostile_type);

				/* Success */
				return TRUE;
			}
		}
		else
		{
			/* Assume this is a party */
			if (streq(parties[0 - h_ptr->id].name, name))
			{
				/* Delete this entry */
				if (i_ptr)
				{
					/* Skip over */
					i_ptr->next = h_ptr->next;
				}
				else
				{
					/* Adjust beginning of list */
					p_ptr->hostile = h_ptr->next;
				}

				/* Message */
				msg_format(Ind, "No longer hostile toward party '%s'.", name);

				/* Delete node */
				KILL(h_ptr, hostile_type);

				/* Success */
				return TRUE;
			}
		}
	}

	/* Message */
	msg_format(Ind, "You are not hostile toward %s.", name);

	/* Failure */
	return FALSE;
}

/*
 * Check if one player is hostile toward the other
 */
bool check_hostile(int attacker, int target)
{
	player_type *p_ptr = Players[attacker];
	hostile_type *h_ptr;

	/* Scan list */
	for (h_ptr = p_ptr->hostile; h_ptr; h_ptr = h_ptr->next)
	{
		/* Check ID */
		if (h_ptr->id > 0)
		{
			/* Identical ID's yield hostility */
			if (h_ptr->id == Players[target]->id)
				return TRUE;
		}
		else
		{
			/* Check if target belongs to hostile party */
			if (Players[target]->party == 0 - h_ptr->id)
				return TRUE;
		}
	}

	/* Not hostile */
	return FALSE;
}


/*
 * The following is a simple hash table, which is used for mapping a player's
 * ID number to his name.  Only players that are still alive are stored in
 * the table, thus the mapping from a 32-bit integer is very sparse.  Also,
 * duplicate ID numbers are prohibitied.
 *
 * The hash function is going to be h(x) = x % n, where n is the length of
 * the table.  For efficiency reasons, n will be a power of 2, thus the
 * hash function can be a bitwise "and" and get the relevant bits off the end.
 *
 * No "probing" is done; if any two ID's map to the same hash slot, they will
 * be chained in a linked list.  This will most likely be a very rare thing,
 * however.
 */

/* The struct to hold a data entry */
typedef struct hash_entry hash_entry;

struct hash_entry
{
	int id;				/* The ID */
	cptr name;			/* Player name */
	time_t laston;			/* Last on time */
	struct hash_entry *next;	/* Next entry in the chain */
};

/* The hash table itself */
static hash_entry *hash_table[NUM_HASH_ENTRIES];


/*
 * Return the slot in which an ID should be stored.
 */
static int hash_slot(int id)
{
	/* Be very efficient */
	return (id & (NUM_HASH_ENTRIES - 1));
}

/*
 * Lookup a player name by ID.  Will return NULL if the name doesn't exist.
 */
time_t lookup_player_laston(int id)
{
	int slot;
	hash_entry *ptr;

	/* Get the slot */
	slot = hash_slot(id);

	/* Acquire the pointer to the first element in the chain */
	ptr = hash_table[slot];

	/* Search the chain, looking for the correct ID */
	while (ptr)
	{
		/* Check this entry */
		if (ptr->id == id)
			return ptr->laston;

		/* Next entry in chain */
		ptr = ptr->next;
	}

	/* Not found */
	return -1L;
}

/*
 * Lookup a player name by ID.  Will return NULL if the name doesn't exist.
 */
cptr lookup_player_name(int id)
{
	int slot;
	hash_entry *ptr;

	/* Get the slot */
	slot = hash_slot(id);

	/* Acquire the pointer to the first element in the chain */
	ptr = hash_table[slot];

	/* Search the chain, looking for the correct ID */
	while (ptr)
	{
		/* Check this entry */
		if (ptr->id == id)
			return ptr->name;

		/* Next entry in chain */
		ptr = ptr->next;
	}

	/* Not found */
	return NULL;
}

/*
 * Lookup a player's ID by name.  Return 0 if not found.
 */
int lookup_player_id(cptr name)
{
	hash_entry *ptr;
	int i;

	/* Search in each array slot */
	for (i = 0; i < NUM_HASH_ENTRIES; i++)
	{
		/* Acquire pointer to this chain */
		ptr = hash_table[i];

		/* Check all entries in this chain */
		while (ptr)
		{
			/* Check this name */
			if (!strcmp(ptr->name, name))
				return ptr->id;

			/* Next entry in chain */
			ptr = ptr->next;
		}
	}

	/* Not found */
	return 0;
}

void stat_player(char *name, bool on){
	int id;
	int slot;
	hash_entry *ptr;

	id=lookup_player_name(name);
	if(id){
		slot = hash_slot(id);
		ptr = hash_table[slot];
		while (ptr){
			if (ptr->id == id){
				ptr->laston=on ? 0L : time(&ptr->laston);
			}
			ptr=ptr->next;
		}
	}
}

/* Timestamp an existing player */
void clockin(int Ind){
	int slot;
	hash_entry *ptr;
	player_type *p_ptr=Players[Ind];
	slot = hash_slot(p_ptr->id);
	ptr = hash_table[slot];
	while (ptr){
		if (ptr->id == p_ptr->id && (ptr->laston)){
			ptr->laston=time(&ptr->laston);
		}
		ptr=ptr->next;
	}
}

/* dish out a valid new player ID */
int newid(){
	int id;
	int slot;
	hash_entry *ptr;

/* there should be no need to do player_id > MAX_ID check
   as it should cycle just fine */

	for(id=player_id;id<=MAX_ID;id++){
		slot = hash_slot(id);
		ptr = hash_table[slot];

		while (ptr){
			if (ptr->id == id) break;
			ptr=ptr->next;
		}
		if(ptr) continue;	/* its on a valid one */
		player_id=id+1;	/* new cycle counter */
		return(id);
	}
	for(id=1;id<player_id;id++){
		slot = hash_slot(id);
		ptr = hash_table[slot];

		while (ptr){
			if (ptr->id == id) break;
			ptr=ptr->next;
		}
		if(ptr) continue;	/* its on a valid one */
		player_id=id+1;	/* new cycle counter */
		return(id);
	}
	return(0);	/* no user IDs available - not likely */
}

sf_delete(char *name){
	int i,k=0;
	char temp[128],fname[128];
	/* Extract "useful" letters */
	for (i = 0; name[i]; i++)
	{
		char c = name[i];

		/* Accept some letters */
		if (isalpha(c) || isdigit(c)) temp[k++] = c;

		/* Convert space, dot, and underscore to underscore */
		else if (strchr(". _", c)) temp[k++] = '_';
	}
	temp[k] = '\0';
	path_build(fname, 1024, ANGBAND_DIR_SAVE, temp);
	unlink(fname);
}

/*
 * Add a name to the hash table.
 */
void add_player_name(cptr name, int id, time_t laston)
{
	int slot;
	hash_entry *ptr;
	time_t now;
	
	now=time(&now);
	if(laston && now>laston){		/* it should be! */
		if(now-laston>7776000){		/* 90 days in seconds */
			sf_delete(name);	/* a sad day ;( */
			return;
		}
	}

	/* Set the entry's id */

	/* Get the destination slot */
	slot = hash_slot(id);

	/* Create a new hash entry struct */
	MAKE(ptr, hash_entry);

	/* Make a copy of the player name in the entry */
	ptr->name = strdup(name);
	ptr->laston = laston;
	ptr->id = id;

	/* Add the rest of the chain to this entry */
	ptr->next = hash_table[slot];

	/* Put this entry in the table */
	hash_table[slot] = ptr;
}

/*
 * Delete an entry from the table, by ID.
 */
void delete_player_id(int id)
{
	int slot;
	hash_entry *ptr, *old_ptr;

	/* Get the destination slot */
	slot = hash_slot(id);

	/* Acquire the pointer to the entry chain */
	ptr = hash_table[slot];

	/* Keep a pointer one step behind this one */
	old_ptr = NULL;

	/* Attempt to find the ID to delete */
	while (ptr)
	{
		/* Check this one */
		if (ptr->id == id)
		{
			/* Delete this one from the table */
			if (old_ptr == NULL)
				hash_table[slot] = ptr->next;
			else old_ptr->next = ptr->next;

			/* Free the memory in the player name */
			free((char *)(ptr->name));

			/* Free the memory for this struct */
			KILL(ptr, hash_entry);

			/* Done */
			return;
		}

		/* Remember this entry */
		old_ptr = ptr;

		/* Advance to next entry in the chain */
		ptr = ptr->next;
	}

	/* Not found */
	return;
}

/*
 * Delete a player by name.
 *
 * This is useful for fault tolerance, as it is possible to have
 * two entries for one player name, if the server crashes hideously
 * or the machine has a power outage or something.
 */
void delete_player_name(cptr name)
{
	int id;

	/* Delete every occurence of this name */
	while ((id = lookup_player_id(name)))
	{
		/* Delete this one */
		delete_player_id(id);
	}
}

/*
 * Return a list of the player ID's stored in the table.
 */
int player_id_list(int **list)
{
	int i, len = 0, k = 0;
	hash_entry *ptr;

	/* Count up the number of valid entries */
	for (i = 0; i < NUM_HASH_ENTRIES; i++)
	{
		/* Acquire this chain */
		ptr = hash_table[i];

		/* Check this chain */
		while (ptr)
		{
			/* One more entry */
			len++;

			/* Next entry in chain */
			ptr = ptr->next;
		}
	}

	/* Allocate memory for the list */
	C_MAKE((*list), len, int);

	/* Look again, this time storing ID's */
	for (i = 0; i < NUM_HASH_ENTRIES; i++)
	{
		/* Acquire this chain */
		ptr = hash_table[i];

		/* Check this chain */
		while (ptr)
		{
			/* Store this ID */
			(*list)[k++] = ptr->id;

			/* Next entry in chain */
			ptr = ptr->next;
		}
	}

	/* Return length */
	return len;
}
