/* $Id$ */
/*
 * Support for the "party" system.
 */

#include "angband.h"
#include "party.h"

#include "../world/world.h"

/*
 * Give some exp-bonus to encourage partying (aka "C.Blue party bonus") [2]
 * formula: (PARTY_XP_BOOST+1)/(PARTY_XP_BOOST + (# of applicable players))
 */
#define PARTY_XP_BOOST	(cfg.party_xp_boost)

#ifdef HAVE_CRYPT
#include <unistd.h>
#endif	// HAVE_CRYPT

static char *t_crypt(char *inbuf, cptr salt);
static void del_party(int id);
static void party_msg(int party_id, cptr msg);
static void del_guild(int id);
static void guild_msg(int guild_id, cptr msg);
static u32b new_accid(void);

/* The hash table itself */
static hash_entry *hash_table[NUM_HASH_ENTRIES];

/* admin only - account edit function */
bool WriteAccount(struct account *r_acc, bool new){
	int fd;
	FILE *fp;
	short found=0;
	struct account c_acc;
	long delpos=-1L;

#ifdef NETBSD
	fd=open("tomenet.acc", O_RDWR|O_EXLOCK|O_NONBLOCK);
#else
	fd=open("tomenet.acc", O_RDWR|O_NONBLOCK);
#endif
	if(fd<0) return(FALSE);
#if (!defined(NETBSD)) && (!defined(WIN32))
	if((flock(fd, LOCK_EX))!=0) return(FALSE);
#endif
	fp=fdopen(fd, "r+");
	if(fp!=(FILE*)NULL){
		while(!feof(fp) && !found){
			fread(&c_acc, sizeof(struct account), 1, fp);
			if(c_acc.flags & ACC_DELD){
				if(delpos==-1L) delpos=(ftell(fp)-sizeof(struct account));
				if(new) break;
				continue;
			}
			if(!strcmp(c_acc.name, r_acc->name)) found=1;
		}
		if(found){
			fseek(fp, -sizeof(struct account), SEEK_CUR);
			fwrite(r_acc, sizeof(struct account), 1, fp);
		}
		if(new){
			if(delpos!=-1L)
				fseek(fp, delpos, SEEK_SET);
			else fseek(fp, 0L, SEEK_END);
			fwrite(r_acc, sizeof(struct account), 1, fp);
			found=1;
		}
		fclose(fp);
	}
#if (!defined(NETBSD)) && (!defined(WIN32))
	flock(fd, LOCK_UN);
#endif
	close(fd);
	return(found);
}

/*
 Get an existing account and set default valid flags on it
 That will be SCORE on only (hack it for MULTI)
 */
void validate(char *name){
	struct account *c_acc;
	c_acc=GetAccount(name, NULL, 1);
	if(!c_acc) return;
	c_acc->flags&=~(ACC_TRIAL | ACC_NOSCORE);
	WriteAccount(c_acc, FALSE);
	memset((char *)c_acc->pass, 0, 20);
	KILL(c_acc, struct account);
}

/*
 return player account information (by name)
 */
void accinfo(char *name){
}

/* most account type stuff was already in here.
   a separate file should probably be made in
   order to split party/guild from account
   and database handling */
/* Note. Accounts will be deleted when empty
   They will not be subject to their own 90
   days timeout, but will be removed upon
   the removal of the last character. */
struct account *GetAccount(cptr name, char *pass, bool leavepass){
	FILE *fp;
	struct account *c_acc;

	MAKE(c_acc, struct account);
	if(c_acc==(struct account*)NULL) return(NULL);
	fp=fopen("tomenet.acc", "r");
	if(fp==(FILE*)NULL){
		if(errno==ENOENT){	/* ONLY if non-existent */
			fp=fopen("tomenet.acc", "w+");
			if(fp==(FILE*)NULL) return(NULL);
			s_printf("Generated new account file\n");
		}
		else return(NULL);	/* failed */
	}
	while(!feof(fp)){
		fread(c_acc, sizeof(struct account), 1, fp);
		if(c_acc->flags & ACC_DELD) continue;
		if(!strcmp(c_acc->name, name)){
			int val;
			if(pass==NULL)		/* direct name lookup */
				val=0;
			else
				val=strcmp(c_acc->pass, t_crypt(pass, name));
			if(!leavepass || pass!=NULL) memset((char *)c_acc->pass, 0, 20);
			if(val){
				fclose(fp);
				KILL(c_acc, struct account);
				return(NULL);
			}
			fclose(fp);
			return(c_acc);
		}
	}
	/* New accounts always have pass */
	if(pass==(char*)NULL){
		KILL(c_acc, struct account);
		fclose(fp);
		return(NULL);
	}

	/* No account found. Create trial account */ 
	c_acc->id=new_accid();
	if(c_acc->id!=0L){
		c_acc->flags=(ACC_TRIAL|ACC_NOSCORE);
		strcpy(c_acc->name, name);
		strcpy(c_acc->pass, t_crypt(pass, name));
		if(!(WriteAccount(c_acc, TRUE))){
			KILL(c_acc, struct account);
			fclose(fp);
			return(NULL);
		}
	}
	memset((char *)c_acc->pass, 0, 20);
	fclose(fp);
	if(c_acc->id) return(c_acc);
	KILL(c_acc, struct account);
	return(NULL);
}

/* our password encryptor */
static char *t_crypt(char *inbuf, cptr salt){
#ifdef HAVE_CRYPT
	static char out[64];
	char setting[9];
	setting[0]='_';
	strncpy(&setting[1], salt, 8);
	strcpy(out, (char*)crypt(inbuf, salt));
	return(out);
#else
	return(inbuf);
#endif
}

bool check_account(char *accname, char *c_name){
	struct account *l_acc;
	u32b id, a_id;
	u16b flags;
	hash_entry *ptr;
	int i;

	if((l_acc=GetAccount(accname, NULL, FALSE))){
		a_id=l_acc->id;
		flags=l_acc->flags;
		KILL(l_acc, struct account);
		id=lookup_player_id(c_name);
		ptr=lookup_player(id);
		if(!ptr || ptr->account==a_id){
			for(i=1; i<=NumPlayers; i++){
				if(Players[i]->account==a_id && !(flags&ACC_MULTI) && strcmp(c_name, Players[i]->name))
					return(FALSE);
			}
			return(TRUE);
		}
	}
	return(FALSE);
}

struct account *GetAccountID(u32b id){
	FILE *fp;
	struct account *c_acc;

	/* we may want to store a local index for fast
	   id/name/filepos lookups in the future */
	MAKE(c_acc, struct account);
	if(c_acc==(struct account*)NULL) return(NULL);
	fp=fopen("tomenet.acc", "r");
	if(fp==(FILE*)NULL) return(NULL);	/* failed */
	while(!feof(fp)){
		fread(c_acc, sizeof(struct account), 1, fp);
		if(id==c_acc->id && !(c_acc->flags & ACC_DELD)){
			memset((char *)c_acc->pass, 0, 20);
			fclose(fp);
			return(c_acc);
		}
	}
	fclose(fp);
	KILL(c_acc, struct account);
	return(NULL);
}

static u32b new_accid(){
	u32b id;
	FILE *fp;
	char *t_map;
	struct account t_acc;
	id=account_id;
	fp=fopen("tomenet.acc", "r");
	if(fp==(FILE*)NULL) return(0L);
	t_map=malloc(MAX_ACCOUNTS/8);
	while(!feof(fp)){
		if(fread(&t_acc, sizeof(struct account), 1, fp))
			t_map[t_acc.id/8]|=(1<<(t_acc.id%8));
	}
	fclose(fp);
	for(id=account_id; id<MAX_ACCOUNTS; id++){
		if(!(t_map[id/8]&(1<<(id%8)))) break;
	}
	if(id==MAX_ACCOUNTS){
		for(id=1; id<account_id; id++){
			if(!(t_map[id/8]&(1<<(id%8)))) break;
		}
		if(id==account_id) id=0;
	}
	free(t_map);
	account_id=id+1;

	return(id);	/* temporary */
}

/*
 * Lookup a guild number by name.
 */
int guild_lookup(cptr name)
{
	int i;

	/* Check each guild */
	for (i = 0; i < MAX_GUILDS; i++)
	{
		/* Check name */
		if (streq(guilds[i].name, name)){
			return i;
		}
	}

	/* No match */
	return -1;
}

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

	/* Check - Fail on non party */
	if (party_id && p_ptr->party == party_id)
		return TRUE;

	/* Not in the party */
	return FALSE;
}
	
/*
 * Create a new guild.
 */
int guild_create(int Ind, cptr name){
	player_type *p_ptr=Players[Ind];
	int index = 0, i;
	object_type forge, *o_ptr=&forge;
	char temp[160];

	if(p_ptr->lev<30){
		msg_print(Ind, "\377yYou are not high enough level to start a guild.");
		return FALSE;
	}
	/* This could probably be improved. */
	if(p_ptr->au<2000000){
		msg_print(Ind, "\377yYou need 2,000,000 gold pieces to start a guild.");
		return FALSE;
	}

	/* Prevent abuse */
	if (streq(name, "Neutral"))
	{
		msg_print(Ind, "\377yThat's not a legal guild name.");
		return FALSE;
	}

	/* Check for already existing guild by that name */
	if ((index=guild_lookup(name) != -1))
	{
		if(p_ptr->admin_dm){
			/* make the guild key */
			invcopy(o_ptr, lookup_kind(TV_KEY, 2));
			o_ptr->number=1;
			o_ptr->pval=index;
			o_ptr->level=1;
			o_ptr->owner=p_ptr->id;
			object_known(o_ptr);
			object_aware(Ind, o_ptr);
			(void)inven_carry(Ind, o_ptr);
			msg_print(Ind, "Spare key created.");
			return FALSE;
		}
		msg_print(Ind, "\377yA guild by that name already exists.");
		return FALSE;
	}
	/* Make sure this guy isn't in some other guild already */
	if (p_ptr->guild != 0)
	{
		msg_print(Ind, "\377yYou already belong to a guild!");
		return FALSE;
	}
	/* Find the "best" party index */
	for (i = 1; i < MAX_GUILDS; i++)
	{
		if (guilds[i].members == 0)
		{
			index = i;
			break;
		}
	}
	/* Make sure we found an empty slot */
	if (index == 0)
	{
		/* Error */
		msg_print(Ind, "\377yThere aren't enough guild slots!");
		return FALSE;
	}
	/* broadcast the news */
	sprintf(temp, "\377GA new guild '%s' has been created.", name);
	msg_broadcast(0, temp);

	p_ptr->au-=2000000;
	p_ptr->redraw|=PR_GOLD;

	/* make the guild key */
	invcopy(o_ptr, lookup_kind(TV_KEY, 2));
	o_ptr->number=1;
	o_ptr->pval=index;
	o_ptr->level=1;
	o_ptr->owner=p_ptr->id;
	object_known(o_ptr);
	object_aware(Ind, o_ptr);
	(void)inven_carry(Ind, o_ptr);

	/* Give the guildmaster some scrolls for a hall */
#if 0	//scrolls are broken
	invcopy(o_ptr, lookup_kind(TV_SCROLL, SV_SCROLL_HOUSE));
	o_ptr->number=6;
	o_ptr->level=p_ptr->lev;
	o_ptr->owner=p_ptr->id;
	o_ptr->discount=50;
	object_known(o_ptr);
	object_aware(Ind, o_ptr);
	(void)inven_carry(Ind, o_ptr);
#endif
	/* Set party name */
	strcpy(guilds[index].name, name);

	/* Set guildmaster */
	guilds[index].master=p_ptr->id;

	/* Add the owner as a member */
	p_ptr->guild = index;
	guilds[index].members=1;
	return(TRUE);
}

/* 
 * New party check function - to be timed 
 *
 */
void party_check(int Ind){
	int i, id;
	for (i = 1; i < MAX_PARTIES; i++)
	{
		if (parties[i].members != 0){
			if(!(id=lookup_player_id(parties[i].owner))){
				msg_format(Ind, "Lost party %s (%s)", parties[i].name, parties[i].owner);
				del_party(i);
			}
			else{
				if((lookup_player_party(id)!=i)){
					msg_format(Ind, "Disowned party %s (%s)", parties[i].name, parties[i].owner);
					del_party(i);
				}
			}
		}
	}
}

/*
 * as with party checker, scan ALL player entries
 * if they are not linked to an existing account,
 * delete them.
 */
void account_check(int Ind){	/* Temporary Ind */
	hash_entry *ptr;
	int i,del;
	player_type *p_ptr=Players[Ind];

	/* Search in each array slot */
	for (i = 0; i < NUM_HASH_ENTRIES; i++)
	{
		/* Acquire pointer to this chain */
		ptr = hash_table[i];

		/* Check all entries in this chain */
		while (ptr)
		{
			/* Check this name */
			if(!GetAccountID(ptr->account)){
				s_printf("Lost player: %s\n", ptr->name);
				msg_format(Ind, "Lost player: %s", ptr->name);
				del=ptr->id;
			}

			/* Next entry in chain */
			ptr = ptr->next;
			delete_player_id(del);
		}
	}
}

/*
 * Create a new party, owned by "Ind", and called
 * "name".
 */
int party_create(int Ind, cptr name)
{
	player_type *p_ptr = Players[Ind];
	int index = 0, i, oldest = turn;

	/* Prevent abuse */
	if (streq(name, "Neutral"))
	{
		msg_print(Ind, "\377yThat's not a legal party name.");
		return FALSE;
	}

	/* Check for already existing party by that name */
	if (party_lookup(name) != -1)
	{
		msg_print(Ind, "\377yA party by that name already exists.");
		return FALSE;
	}

        /* If he's party owner, it's name change */
	if (streq(parties[p_ptr->party].owner, p_ptr->name))
	{
		if (parties[p_ptr->party].mode != PA_NORMAL)
		{
			msg_print(Ind, "\377yYour party is an Iron Team. Choose '2) Create an Iron Team' instead.");
			return FALSE;
		}

		strcpy(parties[p_ptr->party].name, name);

		/* Tell the party about its new name */
		party_msg_format(p_ptr->party, "\377GYour party is now called '%s'.", name);

		Send_party(Ind);
		return TRUE;
	}

	/* Make sure this guy isn't in some other party already */
	if (p_ptr->party != 0)
	{
		msg_print(Ind, "\377yYou already belong to a party!");
		return FALSE;
	}

	/* Find the "best" party index */
	for (i = 1; i < MAX_PARTIES; i++)
	{
		/* Check deletion time of disbanded parties */
		if (parties[i].members == 0 && parties[i].created < oldest)
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
		msg_print(Ind, "\377yThere aren't enough party slots!");
		return FALSE;
	}

	/* Set party name */
	strcpy(parties[index].name, name);

	/* Set owner name */
	strcpy(parties[index].owner, p_ptr->name);

	/* Set mode to normal */
	parties[index].mode = PA_NORMAL;

	/* Add the owner as a member */
	p_ptr->party = index;
	parties[index].members = 1;
	clockin(Ind, 2);

	/* Set the "creation time" */
	parties[index].created = turn;

	/* Resend party info */
	Send_party(Ind);

	/* Success */
	return TRUE;
}

int party_create_ironteam(int Ind, cptr name)
{
	player_type *p_ptr = Players[Ind];
	int index = 0, i, oldest = turn;

	/* Only newly created characters can create an iron team */
	if (p_ptr->max_exp > 0)
	{
		msg_print(Ind, "\377yOnly newly created characters without experience can create an iron team.");
		return FALSE;
	}

	/* Prevent abuse */
	if (streq(name, "Neutral"))
	{
		msg_print(Ind, "\377yThat's not a legal party name.");
		return FALSE;
	}

	/* Check for already existing party by that name */
	if (party_lookup(name) != -1)
	{
		msg_print(Ind, "\377yA party by that name already exists.");
		return FALSE;
	}

        /* If he's party owner, it's name change */
	if (streq(parties[p_ptr->party].owner, p_ptr->name))
	{
		if (parties[p_ptr->party].mode != PA_IRONTEAM)
		{
			msg_print(Ind, "\377yYour party isn't an Iron Team. Choose '1) Create a party' instead.");
			return FALSE;
		}

		strcpy(parties[p_ptr->party].name, name);

		/* Tell the party about its new name */
		party_msg_format(p_ptr->party, "\377GYour iron team is now called '%s'.", name);

		Send_party(Ind);
		return TRUE;
	}

	/* Make sure this guy isn't in some other party already */
	if (p_ptr->party != 0)
	{
		msg_print(Ind, "\377yYou already belong to a party!");
		return FALSE;
	}

	/* Find the "best" party index */
	for (i = 1; i < MAX_PARTIES; i++)
	{
		/* Check deletion time of disbanded parties */
		if (parties[i].members == 0 && parties[i].created < oldest)
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
		msg_print(Ind, "\377yThere aren't enough party slots!");
		return FALSE;
	}

	/* Set party name */
	strcpy(parties[index].name, name);

	/* Set owner name */
	strcpy(parties[index].owner, p_ptr->name);
	
	/* Set mode to iron team */
	parties[index].mode = PA_IRONTEAM;

	/* Add the owner as a member */
	p_ptr->party = index;
	parties[index].members = 1;
	clockin(Ind, 2);

	/* Set the "creation time" */
	parties[index].created = turn;

	/* Resend party info */
	Send_party(Ind);

	/* Success */
	return TRUE;
}

/*
 * Add player to a guild
 */
int guild_add(int adder, cptr name){
	player_type *p_ptr;
	player_type *q_ptr = Players[adder];
	int guild_id = q_ptr->guild, Ind = 0;

	if(!guild_id){
		msg_print(adder, "\377yYou are not in a guild");
		return(FALSE);
	}

	Ind = name_lookup_loose(adder, name, FALSE);

	if (Ind <= 0)
	{
		return FALSE;
	}

	/* Set pointer */
	p_ptr = Players[Ind];

	/* Make sure this isn't an impostor */
	if (guilds[guild_id].master!=q_ptr->id && !q_ptr->admin_dm && !q_ptr->admin_wiz)
	{
		/* Message */
		msg_print(adder, "\377yOnly the guildmaster or a dungeon wizard may add new members.");

		/* Abort */
		return FALSE;
	}

	/* Make sure this added person is neutral */
	if (p_ptr->guild != 0)
	{
		/* Message */
		msg_print(adder, "\377yThat player is already in a guild.");

		/* Abort */
		return FALSE;
	}

	/* Tell the guild about its new member */
	guild_msg_format(guild_id, "\377y%s has been added to %s.", p_ptr->name, guilds[guild_id].name);

	/* One more player in this guild */
	guilds[guild_id].members++;

	/* Tell him about it */
	msg_format(Ind, "\377yYou've been added to '%s'.", guilds[guild_id].name);

	/* Set his guild number */
	p_ptr->guild = guild_id;
	clockin(Ind, 3);

#if 0
	/* Resend info */
	Send_guild(Ind);
#endif

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
	int party_id = q_ptr->party, Ind = 0;
	
	Ind = name_lookup_loose(adder, name, FALSE);

	if (Ind <= 0)
	{
		return FALSE;
	}

	/* Set pointer */
	p_ptr = Players[Ind];
#if 0 // It's really a prob that the owner can't add his own chars..so if0
	/* Make sure this isn't an impostor */
	if (!streq(parties[party_id].owner, q_ptr->name))
	{
		/* Message */
		msg_print(adder, "\377yYou must be the owner to add someone.");

		/* Abort */
		return FALSE;
	}
#endif
	/* Make sure this added person is neutral */
	if (p_ptr->party != 0)
	{
		/* Message */
		msg_print(adder, "\377yThat player is already in a party.");

		/* Abort */
		return FALSE;
	}

	/* Everlasting and other chars cannot be in the same party */
	if ((p_ptr->mode & MODE_IMMORTAL) != (q_ptr->mode & MODE_IMMORTAL))
	{
		msg_print(adder, "\377yEverlasting characters and other characters can't be in the same party.");
		return FALSE;
	}

	/* Only newly created characters can create an iron team */
        if (p_ptr->max_exp > 0)
        {
	        msg_print(Ind, "\377yOnly newly created characters without experience can join an iron team.");
		return FALSE;
	}

	/* Tell the party about its new member */
	party_msg_format(party_id, "\377y%s has been added to party %s.", p_ptr->name, parties[party_id].name);

	/* One more player in this party */
	parties[party_id].members++;

	/* Tell him about it */
	if (parties[party_id].mode == PA_IRONTEAM)
		msg_format(Ind, "\377yYou've been added to iron team '%s'.", parties[party_id].name);
	else
		msg_format(Ind, "\377yYou've been added to party '%s'.", parties[party_id].name);

	/* Set his party number */
	p_ptr->party = party_id;
	clockin(Ind, 2);

	/* Resend info */
	Send_party(Ind);

	/* Success */
	return TRUE;
}

/*
 * Remove a guild. What a sad day.
 *
 * In style of del_party.
 */
static void del_guild(int id){
	char temp[160];

	/* Clear the guild hall */
	kill_houses(id, OT_GUILD);

	/* Tell everyone */
	sprintf(temp, "\377gThe guild \377r'\377y%s\377r'\377g no longer exists.", guilds[id].name);
	msg_broadcast(0, temp);
	/* Clear the basic info */
	guilds[id].members=0;	/* it should be zero anyway */
	strcpy(guilds[id].name,"");
}

/*
 * Delete a party. Was in party remove.
 *
 * Design improvement
 */
static void del_party(int id){
	int i;
	/* Remove the party altogether */
	kill_houses(id, OT_PARTY);

	/* Set the number of people in this party to zero */
	parties[id].members = 0;

	/* Remove everyone else */
	for (i = 1; i <= NumPlayers; i++)
	{
		/* Check if they are in here */
		if (player_in_party(id, i))
		{
			Players[i]->party = 0;
			clockin(i, 2);
			if (parties[id].mode == PA_IRONTEAM)
				msg_print(i, "\377yYour iron team has been disbanded.");
			else
				msg_print(i, "\377yYour party has been disbanded.");
			Send_party(i);
		}
	}

	/* Set the creation time to "disbanded time" */
	parties[id].created = turn;

	/* Empty the name */
	strcpy(parties[id].name, "");
}

/* 
 * Remove player from a guild
 */
int guild_remove(int remover, cptr name){
	player_type *p_ptr;
	player_type *q_ptr = Players[remover];
	int guild_id = q_ptr->guild, Ind = 0;

	if(!guild_id){
		if(!q_ptr->admin_dm && !q_ptr->admin_wiz)
		{
			msg_print(remover, "\377yYou are not in a guild");
			return FALSE;
		}
	}

	/* Make sure this is the owner */
	if (guilds[guild_id].master!=q_ptr->id && !q_ptr->admin_dm && !q_ptr->admin_wiz)
	{
		/* Message */
		msg_print(remover, "\377yYou must be the owner to delete someone.");

		/* Abort */
		return FALSE;
	}

	Ind = name_lookup_loose(remover, name, FALSE);

	if (Ind <= 0)
	{
		return FALSE;
	}

	if(Ind==remover){	/* remove oneself from guild - leave */
		guild_leave(remover);
		return TRUE;
	}

	p_ptr = Players[Ind];

	if(!guild_id && (q_ptr->admin_dm || q_ptr->admin_wiz))
	{
		guild_id = p_ptr->guild;
	}

	/* Make sure they were in the guild to begin with */
	if (guild_id!=p_ptr->guild)
	{
		/* Message */
		msg_print(remover, "\377yYou can only delete guild members.");

		/* Abort */
		return FALSE;
	}

	/* Keep the guild, just lose a member */
	else
	{
		/* Lose a member */
		guilds[guild_id].members--;

		/* Set his party number back to "neutral" */
		p_ptr->guild = 0;

		/* Messages */
		msg_print(Ind, "\377yYou have been removed from the guild.");
		guild_msg_format(guild_id, "\377y%s has been removed from the guild.", p_ptr->name);

#if 0
		/* Resend info */
		Send_guild(Ind);
#endif
		/* Last member deleted? */
		if(guilds[guild_id].members==0)
			del_guild(guild_id);
	}

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
	int party_id = q_ptr->party, Ind = 0;

	/* Make sure this is the owner */
	if (!streq(parties[party_id].owner, q_ptr->name) && !q_ptr->admin_dm && !q_ptr->admin_wiz)
	{
		/* Message */
		msg_print(remover, "\377yYou must be the owner to delete someone.");

		/* Abort */
		return FALSE;
	}

	Ind = name_lookup_loose(remover, name, FALSE);

	if (Ind <= 0)
	{
		return FALSE;
	}

	p_ptr = Players[Ind];

	if((!party_id || !streq(parties[party_id].owner, q_ptr->name)) && (q_ptr->admin_dm || q_ptr->admin_wiz))
	{
		party_id = p_ptr->party;
	}

	/* Make sure they were in the party to begin with */
	if (!player_in_party(party_id, Ind))
	{
		/* Message */
		msg_print(remover, "\377yYou can only delete party members.");

		/* Abort */
		return FALSE;
	}

	/* See if this is the owner we're deleting */
	if (remover == Ind)
	{
		del_party(party_id);
	}

	/* Keep the party, just lose a member */
	else
	{
		/* Lose a member */
		parties[party_id].members--;

		/* Set his party number back to "neutral" */
		p_ptr->party = 0;
		clockin(Ind, 2);

		/* Messages */
		if (parties[party_id].mode == PA_IRONTEAM)
		{
			msg_print(Ind, "\377yYou have been removed from your iron team.");
			party_msg_format(party_id, "\377y%s has been removed from the iron team.", p_ptr->name);
		} else {
			msg_print(Ind, "\377yYou have been removed from your party.");
			party_msg_format(party_id, "\377y%s has been removed from the party.", p_ptr->name);
		}

		/* Resend info */
		Send_party(Ind);
	}

	return TRUE;
}

void guild_leave(int Ind){
	player_type *p_ptr = Players[Ind];
	int guild_id = p_ptr->guild;

	/* Make sure he belongs to a guild */
	if (!guild_id)
	{
		msg_print(Ind, "\377yYou don't belong to a guild.");
		return;
	}

	/* Lose a member */
	guilds[guild_id].members--;

	/* Set him back to "neutral" */
	p_ptr->guild = 0;

	/* Inform people */
	msg_print(Ind, "\377yYou have been removed from your guild.");
	guild_msg_format(guild_id, "\377y%s has left the guild.", p_ptr->name);
	
	/* If he's the guildmaster, set master to zero */
	if (p_ptr->id==guilds[guild_id].master)
	{
		guild_msg_format(guild_id, "\377yThe guild is currently leaderless");
		guilds[guild_id].master=0;
	}

#if 0
	/* Resend info */
	Send_guild(Ind);
#endif

	/* Last member deleted? */
	if(guilds[guild_id].members==0)
		del_guild(guild_id);
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
		msg_print(Ind, "\377yYou don't belong to a party.");
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
	parties[party_id].members--;

	/* Set him back to "neutral" */
	p_ptr->party = 0;
	clockin(Ind, 2);

	/* Inform people */
	if (parties[party_id].mode == PA_IRONTEAM)
	{
		msg_print(Ind, "\377yYou have been removed from your iron team.");
		party_msg_format(party_id, "\377y%s has left the iron team.", p_ptr->name);
	} else {
		msg_print(Ind, "\377yYou have been removed from your party.");
		party_msg_format(party_id, "\377y%s has left the party.", p_ptr->name);
	}

	/* Resend info */
	Send_party(Ind);
}

/*
 * Send a message to everyone in a party.
 */
static void guild_msg(int guild_id, cptr msg)
{
	int i;

	/* Check for this guy */
	for (i = 1; i <= NumPlayers; i++)
	{
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* Check this guy */
		if (guild_id==Players[i]->guild)
			msg_print(i, msg);
	}
}


/*
 * Send a message to everyone in a party.
 */
static void party_msg(int party_id, cptr msg)
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
 * Send a formatted message to a guild.
 */
void guild_msg_format(int guild_id, cptr fmt, ...)
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
	guild_msg(guild_id, buf);
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
 * Send a message to everyone in a party, considering ignorance.
 */
static void party_msg_ignoring(int sender, int party_id, cptr msg)
{
	int i;

	/* Check for this guy */
	for (i = 1; i <= NumPlayers; i++)
	{
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		if (check_ignore(i, sender))
			continue;

		/* Check this guy */
		if (player_in_party(party_id, i))
			msg_print(i, msg);
	}
}

/*
 * Send a formatted message to a party.
 */
void party_msg_format_ignoring(int sender, int party_id, cptr fmt, ...)
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
	party_msg_ignoring(sender, party_id, buf);
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

static bool players_in_level(int Ind, int Ind2)
{
        if ((Players[Ind]->lev - Players[Ind2]->lev) > 7) return FALSE;
        if ((Players[Ind2]->lev - Players[Ind]->lev) > 7) return FALSE;
        return TRUE;
}

void party_gain_exp(int Ind, int party_id, s32b amount)
{
	player_type *p_ptr;
	int i;
	struct worldpos *wpos=&Players[Ind]->wpos;
	s32b new_exp, new_exp_frac, average_lev = 0, num_members = 0;
	s32b modified_level;

	/* Iron Teams only get exp if the whole team is on the same floor! */
	if (parties[party_id].mode == PA_IRONTEAM)
	{
		int membershere = 0;
		for (i = 1; i <= NumPlayers; i++)
		{
			p_ptr = Players[i];
			if (p_ptr->conn == NOT_CONNECTED) continue;
			if (player_in_party(party_id, i)) membershere++;
		}
		if (membershere != parties[party_id].members) return;
	}

	/* Calculate the average level */
	for (i = 1; i <= NumPlayers; i++)
	{
		p_ptr = Players[i];

		if (p_ptr->conn == NOT_CONNECTED)
			continue;

		/* Check for his existance in the party */
                if (player_in_party(party_id, i) && (inarea(&p_ptr->wpos, wpos)) && players_in_level(Ind, i))
		{
			/* Increase the "divisor" */
			average_lev += p_ptr->lev;
			num_members++;
		}
	}
	average_lev /= num_members;

	/* Now, distribute the experience */
	for (i = 1; i <= NumPlayers; i++)
	{
		p_ptr = Players[i];

		if (p_ptr->conn == NOT_CONNECTED)
			continue;
		if (p_ptr->ghost)	/* no exp, but take share */
			continue;

		/* Check for existance in the party */
                if (player_in_party(party_id, i) && (inarea(&p_ptr->wpos, wpos)) && players_in_level(Ind, i))
		{
			/* Calculate this guy's experience */
			
			if (p_ptr->lev < average_lev) // below average
			{
				if ((average_lev - p_ptr->lev) > 2)
				{
					modified_level = p_ptr->lev + 2;
				}				
				else modified_level = average_lev;
			}
			else
			{
				if ((p_ptr->lev - average_lev) > 2)
				{
					modified_level = p_ptr->lev - 2;
				}				
				else modified_level = average_lev;
						
			}
			
			/*			
			new_exp = (amount * modified_level) / (average_lev * num_members * p_ptr->lev);
			new_exp_frac = ((((amount * modified_level) % (average_lev * num_members * p_ptr->lev) )
			                * 0x10000L ) / (average_lev * num_members * p_ptr->lev)) + p_ptr->exp_frac;
			*/

			/* Some bonus is applied to encourage partying	- Jir - */
			new_exp = (amount * modified_level * (PARTY_XP_BOOST + 1)) /
				(average_lev * p_ptr->lev * (num_members + PARTY_XP_BOOST));
			new_exp_frac = (  (((amount * modified_level * (PARTY_XP_BOOST + 1)) %
				(average_lev * p_ptr->lev * (num_members + PARTY_XP_BOOST))) * 0x10000L) /
			        (average_lev * p_ptr->lev * (num_members + PARTY_XP_BOOST))) + p_ptr->exp_frac;

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

	i = name_lookup_loose(Ind, name, TRUE);

	if (!i)
	{
		return FALSE;
	}

	/* Check for sillyness */
	if (i == Ind)
	{
		/* Message */
		msg_print(Ind, "\377yYou cannot be hostile toward yourself.");
		return FALSE;
	}

	if (cfg.use_pk_rules == PK_RULES_DECLARE)
	{
		if(!(p_ptr->pkill & PKILL_KILLER)){
			msg_print(Ind, "\377yYou may not be hostile to other players.");
			return FALSE;
		}
	}

	if (cfg.use_pk_rules == PK_RULES_NEVER)
	{
		msg_print(Ind, "\377yYou may not be hostile to other players.");
		return FALSE;
	}

	if (i > 0)
	{
		q_ptr = Players[i];

		/* Make sure players aren't in the same party */
		if (p_ptr->party && player_in_party(p_ptr->party, i))
		{
			/* Message */
			msg_format(Ind, "\377y%^s is in your party!", q_ptr->name);

			return FALSE;
		}

		/* Ensure we don't add the same player twice */
		for (h_ptr = p_ptr->hostile; h_ptr; h_ptr = h_ptr->next)
		{
			/* Check this ID */
			if (h_ptr->id == q_ptr->id)
			{
				/* Message */
				msg_format(Ind, "\377yYou are already hostile toward %s.", q_ptr->name);

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
		msg_format(Ind, "\377yYou are now hostile toward %s.", q_ptr->name);

		/* Success */
		return TRUE;
	}
	else
	{
		/* Tweak - inverse i once more */
		i = 0 - i;

		/* Ensure we don't add the same party twice */
		for (h_ptr = p_ptr->hostile; h_ptr; h_ptr = h_ptr->next)
		{
			/* Check this ID */
			if (h_ptr->id == 0 - i)
			{
				/* Message */
				msg_format(Ind, "\377yYou are already hostile toward party '%s'.", parties[i].name);

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
		msg_format(Ind, "\377oYou are now hostile toward party '%s'.", parties[i].name);

		/* Success */
		return TRUE;
	}

#if 0
	/* Check for sillyness */
	if (streq(name, p_ptr->name))
	{
		/* Message */
		msg_print(Ind, "\377yYou cannot be hostile toward yourself.");

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
			msg_format(Ind, "\377y%^s is in your party!", q_ptr->name);

			return FALSE;
		}

		/* Ensure we don't add the same player twice */
		for (h_ptr = p_ptr->hostile; h_ptr; h_ptr = h_ptr->next)
		{
			/* Check this ID */
			if (h_ptr->id == q_ptr->id)
			{
				/* Message */
				msg_format(Ind, "\377yYou are already hostile toward %s.", q_ptr->name);

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
		msg_format(Ind, "\377oYou are now hostile toward %s.", q_ptr->name);

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
				msg_format(Ind, "\377yYou are already hostile toward party '%s'.", parties[i].name);

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
		msg_format(Ind, "\377oYou are now hostile toward party '%s'.", parties[i].name);

		/* Success */
		return TRUE;
	}

	/* Couldn't find player */
	msg_format(Ind, "\377y%^s is currently not in the game.", name);

	return FALSE;
#endif
}

/*
 * Remove an entry from a player's list of hostilities
 */
bool remove_hostility(int Ind, cptr name)
{
	player_type *p_ptr = Players[Ind];
	hostile_type *h_ptr, *i_ptr;
	cptr p, q = NULL;
	int i = name_lookup_loose(Ind, name, TRUE);

	if (!i)
	{
		return FALSE;
	}

	/* Check for another silliness */
	if (i == Ind)
	{
		/* Message */
		msg_print(Ind, "\377GYou are not hostile toward yourself.");

		return FALSE;
	}

	/* Forge name */
	if (i > 0)
	{
		q = Players[i]->name;
	}

	/* Initialize lock-step */
	i_ptr = NULL;

	/* Search entries */
	for (h_ptr = p_ptr->hostile; h_ptr; i_ptr = h_ptr, h_ptr = h_ptr->next)
	{
		/* Lookup name of this entry */
		if (h_ptr->id > 0)
		{
			/* Efficiency */
			if (i < 0) continue;

			/* Look up name */
			p = lookup_player_name(h_ptr->id);

			/* Check player name */
//			if (p && (streq(p, q) || streq(p, name)))
			if (p && streq(p, q))
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
				msg_format(Ind, "\377GNo longer hostile toward %s.", p);

				/* Delete node */
				KILL(h_ptr, hostile_type);

				/* Success */
				return TRUE;
			}
		}
		else
		{
			/* Efficiency */
			if (i >= 0) continue;

			/* Assume this is a party */
//			if (streq(parties[0 - h_ptr->id].name, q))
			if (i == h_ptr->id)
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
				msg_format(Ind, "\377GNo longer hostile toward party '%s'.", parties[0 - i].name);

				/* Delete node */
				KILL(h_ptr, hostile_type);

				/* Success */
				return TRUE;
			}
		}
	}
	return(FALSE);
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
 * Add/remove a player to/from another player's list of ignorance.
 * These functions should be common with hostilityes in the future. -Jir-
 */
bool add_ignore(int Ind, cptr name)
{
	player_type *p_ptr = Players[Ind], *q_ptr;
	hostile_type *h_ptr, *i_ptr;
	int i;
	int snum=0;
	cptr p, q = NULL;
	char search[80], *pname;

	/* Check for silliness */
	if (!name)
	{
		msg_print(Ind, "Usage: /ignore foobar");

		return FALSE;
	}

	if((pname=strchr(name, '@'))){
		struct remote_ignore *curr, *prev=NULL;
		struct rplist *w_player;
		strncpy(search, name, pname-name);
		search[pname-name]='\0';
		snum=atoi(pname+1);
		w_player=world_find_player(search, snum);
		if(!w_player){
			msg_format(Ind, "Could not find %s in the world", search);
			return(FALSE);
		}
		curr=p_ptr->w_ignore;
		while(curr){
			if(curr->serverid==w_player->server && curr->id==w_player->id) break;
			prev=curr;
			curr=curr->next;
		}
		if(!curr){
			msg_format(Ind, "Ignoring %s across the world", w_player->name);
			curr=malloc(sizeof(struct remote_ignore));
			curr->serverid=w_player->server;
			curr->id=w_player->id;
			curr->next=NULL;
			if(prev)
				prev->next=curr;
			else
				p_ptr->w_ignore=curr;
		}
		else{
			msg_format(Ind, "Hearing %s from across the world", search);
			if(!prev)
				p_ptr->w_ignore=curr->next;
			else
				prev->next=curr->next;
			free(curr);
		}
		return(TRUE);
	}

	i = name_lookup_loose(Ind, name, TRUE);
	if (!i)
	{
		return FALSE;
	}

	/* Check for another silliness */
	if (i == Ind)
	{
		/* Message */
		msg_print(Ind, "You cannot ignore yourself.");

		return FALSE;
	}

	/* Forge name */
	if (i > 0)
	{
		q = Players[i]->name;
	}

	/* Initialize lock-step */
	i_ptr = NULL;

	/* Toggle ignorance if already on the list */
	for (h_ptr = p_ptr->ignore; h_ptr; i_ptr = h_ptr, h_ptr = h_ptr->next)
	{
		/* Lookup name of this entry */
		if (h_ptr->id > 0)
		{
			/* Efficiency */
			if (i < 0) continue;

			/* Look up name */
			p = lookup_player_name(h_ptr->id);

			/* Check player name */
			if (p && streq(p, q))
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
					p_ptr->ignore = h_ptr->next;
				}

				/* Message */
				msg_format(Ind, "Now listening to %s again.", p);

				/* Delete node */
				KILL(h_ptr, hostile_type);

				/* Success */
				return TRUE;
			}
		}
		else
		{
			/* Efficiency */
			if (i > 0) continue;

			/* Assume this is a party */
//			if (streq(parties[0 - h_ptr->id].name, q))
			if (i == h_ptr->id)
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
					p_ptr->ignore = h_ptr->next;
				}

				/* Message */
				msg_format(Ind, "Now listening to party '%s' again.", parties[0 - i].name);

				/* Delete node */
				KILL(h_ptr, hostile_type);

				/* Success */
				return TRUE;
			}
		}
	}

	if (i > 0)
	{
		q_ptr = Players[i];

		/* Create a new hostility node */
		MAKE(h_ptr, hostile_type);

		/* Set ID in node */
		h_ptr->id = q_ptr->id;

		/* Put this node at the beginning of the list */
		h_ptr->next = p_ptr->ignore;
		p_ptr->ignore = h_ptr;

		/* Message */
		msg_format(Ind, "You aren't hearing %s any more.", q_ptr->name);

		/* Success */
		return TRUE;
	}
	else
	{
		/* Tweak - inverse i once more */
		i = 0 - i;

		/* Create a new hostility node */
		MAKE(h_ptr, hostile_type);

		/* Set ID in node */
		h_ptr->id = 0 - i;

		/* Put this node at the beginning of the list */
		h_ptr->next = p_ptr->ignore;
		p_ptr->ignore = h_ptr;

		/* Message */
		msg_format(Ind, "You aren't hearing party '%s' any more.", parties[i].name);

		/* Success */
		return TRUE;
	}
}

/*
 * Check if one player is ignoring the other
 */
bool check_ignore(int attacker, int target)
{
	player_type *p_ptr = Players[attacker];
	hostile_type *h_ptr;

	/* Scan list */
	for (h_ptr = p_ptr->ignore; h_ptr; h_ptr = h_ptr->next)
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


/*
 * Return the slot in which an ID should be stored.
 */
static int hash_slot(int id)
{
	/* Be very efficient */
	return (id & (NUM_HASH_ENTRIES - 1));
}


/*
 * Lookup a player record ID.  Will return NULL on failure.
 */
hash_entry *lookup_player(int id)
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
			return ptr;

		/* Next entry in chain */
		ptr = ptr->next;
	}

	/* Not found */
	return NULL;
}


/*
 * Get the player's highest level.
 */
byte lookup_player_level(int id)
{
	hash_entry *ptr;
	if((ptr=lookup_player(id)))
		return ptr->level;

	/* Not found */
	return -1L;
}

u16b lookup_player_type(int id) {
	hash_entry *ptr;
	if((ptr=lookup_player(id)))
		return(ptr->race|(ptr->class<<8));

	/* Not found */
	return -1L;
}

/*
 * Get the player's current party.
 */
byte lookup_player_party(int id)
{
	hash_entry *ptr;
	if((ptr=lookup_player(id)))
		return ptr->party;

	/* Not found */
	return -1L;
}

/*
 * Get the timestamp for the last time player was on.
 */
time_t lookup_player_laston(int id)
{
	hash_entry *ptr;

	if((ptr=lookup_player(id)))
		return ptr->laston;

	/* Not found */
	return -1L;
}

/*
 * Lookup a player name by ID.  Will return NULL if the name doesn't exist.
 */
cptr lookup_player_name(int id)
{
	hash_entry *ptr;
	if((ptr=lookup_player(id)))
		return ptr->name;

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
			if (!strcmp(ptr->name, name)){
				return ptr->id;
			}

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

	id=lookup_player_id(name);
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
void clockin(int Ind, int type){
	int slot;
	hash_entry *ptr;
	player_type *p_ptr=Players[Ind];
	slot = hash_slot(p_ptr->id);
	ptr = hash_table[slot];
	while (ptr){
		if (ptr->id == p_ptr->id){
			switch(type){
				case 0:
					if(ptr->laston) ptr->laston=time(&ptr->laston);
					break;
				case 1:
					if(p_ptr->lev>ptr->level)
						ptr->level=p_ptr->lev;
					break;
				case 2:
					ptr->party=p_ptr->party;
					break;
				case 3:
					ptr->guild=p_ptr->guild;
					break;
				case 4:
					ptr->quest=p_ptr->quest_id;
					break;
			}
			break;
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

void sf_delete(const char *name){
	int i,k=0;
	char temp[128],fname[MAX_PATH_LENGTH];
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
	path_build(fname, MAX_PATH_LENGTH, ANGBAND_DIR_SAVE, temp);
	unlink(fname);
}

/*
 *  Called once every 24 hours. Deletes unused IDs.
 */
void scan_players(){
	int slot;
	hash_entry *ptr, *pptr=NULL;
	time_t now;
	now=time(&now);
	s_printf("Starting player inactivity check\n");
	for(slot=0; slot<NUM_HASH_ENTRIES;slot++){
		pptr=NULL;
		ptr=hash_table[slot];
		while(ptr){
			if(ptr->laston && (now - ptr->laston > 7776000)){
				int i;
				hash_entry *dptr;

				s_printf("Removing player: %s\n", ptr->name);

				for(i=0; i<MAX_PARTIES; i++){
					if(streq(parties[i].owner, ptr->name)){
						s_printf("Disbanding party: %s\n",parties[i].name);
						del_party(i);
						break;
					}
				}
				kill_houses(ptr->id, OT_PLAYER);
				rem_quest(ptr->quest);

				sf_delete(ptr->name);	/* a sad day ;( */
				if(!pptr)
					hash_table[slot]=ptr->next;
				else
					pptr->next=ptr->next;
				/* Free the memory in the player name */
				free((char *)(ptr->name));

				dptr=ptr;	/* safe storage */
				ptr=ptr->next;	/* advance */

				/* Free the memory for this struct */
				KILL(dptr, hash_entry);

				continue;
			}
			pptr=ptr;
			ptr=ptr->next;
		}
	}
	s_printf("Finished player inactivity check\n");
}

/*
 * Add a name to the hash table.
 */
void add_player_name(cptr name, int id, u32b account, byte race, byte class, byte level, byte party, byte guild, u16b quest, time_t laston)
{
	int slot;
	hash_entry *ptr;

	/* Set the entry's id */

	/* Get the destination slot */
	slot = hash_slot(id);

	/* Create a new hash entry struct */
	MAKE(ptr, hash_entry);

	/* Make a copy of the player name in the entry */
	ptr->name = strdup(name);
	ptr->laston = laston;
	ptr->id = id;
	ptr->account = account;
	ptr->level = level;
	ptr->party = party;
	ptr->guild = guild;
	ptr->quest = quest;
	ptr->race = race;
	ptr->class = class;

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
int player_id_list(int **list, u32b account)
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
			if(!account || ptr->account==account)
				len++;

			/* Next entry in chain */
			ptr = ptr->next;
		}
	}
	if(!len) return(0);

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
			if(!account || ptr->account==account)
				(*list)[k++] = ptr->id;

			/* Next entry in chain */
			ptr = ptr->next;
		}
	}

	/* Return length */
	return len;
}

/*
 * Set/reset 'pk' mode, which allows a player to kill the others
 *
 * These functions should be common with hostilityes in the future. -Jir-
 */
void set_pkill(int Ind, int delay)
{
	player_type *p_ptr = Players[Ind];
	//bool admin = is_admin(p_ptr);

	if (cfg.use_pk_rules != PK_RULES_DECLARE)
	{
		msg_print(Ind, "\377o/pkill is not available on this server. Be pacifist.");
		p_ptr->tim_pkill= 0;
		p_ptr->pkill= 0;
		return;
	}

//	p_ptr->tim_pkill= admin ? 10 : 200;	/* so many turns */
	p_ptr->tim_pkill= delay;
	p_ptr->pkill^=PKILL_SET; /* Toggle value */
	if(p_ptr->pkill&PKILL_SET){
		msg_print(Ind, "\377rYou wish to kill other players");
		p_ptr->pkill|=PKILL_KILLABLE;
	}
	else{
		hostile_type *t_host;
		msg_print(Ind, "\377gYou do not wish to kill other players");
		p_ptr->pkill&=~PKILL_KILLER;
		/* Remove all hostilities */
		while(p_ptr->hostile){
			t_host=p_ptr->hostile;
			p_ptr->hostile=t_host->next;
			KILL(t_host, hostile_type);
		}
	}
}

#if 0	// under construction
/*
 * Set/reset 'pilot' mode, which allows a player to follow another player
 * for comfort in party diving.
 *
 * These functions should be common with hostilityes in the future. -Jir-
 */
bool pilot_set(int Ind, cptr name)
{
	player_type *p_ptr = Players[Ind], *q_ptr;
	hostile_type *h_ptr, *i_ptr;
	int i;
	cptr p,q;

	/* Check for silliness */
	if (!name)
	{
		msg_print(Ind, "Usage: /pilot foobar");

		return FALSE;
	}

	i = name_lookup_loose(Ind, name, TRUE);

	if (!i)
	{
		return FALSE;
	}

	/* Check for another silliness */
	if (i == Ind)
	{
		/* Message */
		msg_print(Ind, "You cannot follow yourself.");

		return FALSE;
	}

	/* Forge name */
	if (i > 0)
	{
		q = Players[i]->name;
	}


	if (i > 0)
	{
		q_ptr = Players[i];

		/* Create a new hostility node */
		MAKE(h_ptr, hostile_type);

		/* Set ID in node */
		h_ptr->id = q_ptr->id;

		/* Put this node at the beginning of the list */
		h_ptr->next = p_ptr->ignore;
		p_ptr->ignore = h_ptr;

		/* Message */
		msg_format(Ind, "You aren't hearing %s any more.", q_ptr->name);

		/* Success */
		return TRUE;
	}
	else
	{
		/* Tweak - inverse i once more */
		i = 0 - i;

		/* Create a new hostility node */
		MAKE(h_ptr, hostile_type);

		/* Set ID in node */
		h_ptr->id = 0 - i;

		/* Put this node at the beginning of the list */
		h_ptr->next = p_ptr->ignore;
		p_ptr->ignore = h_ptr;

		/* Message */
		msg_format(Ind, "You aren't hearing party '%s' any more.", parties[i].name);

		/* Success */
		return TRUE;
	}
}
#endif	// 0

