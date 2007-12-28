/*
 * Slash commands..
 * It was getting too ugly, and 
 * util.c was getting big
 *
 * old (working) code is at bottom of file
 *
 * design subject to change (suggestions welcome)
 *
 * - evileye
 */

/* #define NOTYET 	*//* only for testing and working atm */

#include "angband.h"

static void do_slash_brief_help(int Ind);
char pet_creation(int Ind);
//static int lInd = 0;
#ifdef NOTYET	/* new idea */

struct scmd{
	char *cmd;
	unsigned short admin;		/* bool require admin or not */
	short minargs, maxargs;		/* MIN/MAX number of args 
					 * both 0 for a line content, -1 for any */
	void (*func)(int, void*);	/* point either to char * or args (NULL terminated char**) */
	char *errorhlp;			/* if its bad, tell them this */
};

/* the function declarations */
void sc_shutdown(int Ind, void *value);
void sc_report(int Ind, void *string);
void sc_wish(int Ind, void *argp);
void sc_shout(int Ind, void *string);

/* the commands structure */
struct scmd scmd[]={
	{ "shutdown",	1,	0,	1,	sc_shutdown	, NULL		},

	{ "bug",	0,	0,	0,	sc_report,	"\377oUsage: /rfe (message)"	},
	{ "rfe",	0,	0,	0,	sc_report,	"\377oUsage: /rfe (message)"	},
	{ "cookie",	0,	0,	0,	sc_report,	"\377oUsage: /rfe (message)"	},
	{ "ignore",	0,	1,	1,	add_ignore,	"\377oUsage: /ignore (user)"	},
	{ "shout",	0,	0,	1,	sc_shout,	NULL		},

	{ "wish",	1,	3,	5,	sc_wish,	"\377oUsage: /wish (tval) (sval) (pval) [discount] [name]"	},
	{ NULL,		0,	0,	0,	NULL,		NULL		}
};

/*
 * updated slash command parser - evileye
 *
 */
void do_slash_cmd(int Ind, char *message){
	int i, j;
	player_type *p_ptr;

	p_ptr=Players[Ind];
	if(!p_ptr) return;

	/* check for idiots */
	if(!message[1]) return;

	for(i=0; scmd[i].cmd; i++){
		if(!strncmp(scmd[i].cmd, &message[1], strlen(scmd[i].cmd))){
			if(scmd[i].admin && !is_admin(p_ptr)) break;

			if(scmd[i].minargs==-1){
				/* no string required */
				scmd[i].func(Ind, NULL);
				return;
			}

			for(j=strlen(scmd[i].cmd)+1; message[j]; j++){
				if(message[j]!=' '){
					break;
				}
			}
			if(!message[j] && scmd[i].minargs){
				if(scmd[i].errorhlp) msg_print(Ind, scmd[i].errorhlp);
				return;
			}

			if(!scmd[i].minargs && !scmd[i].maxargs){
				/* a line arg */
				scmd[i].func(Ind, &message[strlen(scmd[i].cmd)+1]);
			}
			else{
				char **args;
				char *cp=&message[strlen(scmd[i].cmd)+1];

				/* allocate args array */
				args=(char**)malloc(sizeof(char*) * (scmd[i].maxargs+1));
				if(args==(char**)NULL){
					/* unlikely - best to check */
					msg_print(Ind, "\377uError. Please try later.");
					return;
				}

				/* simple tokenize into args */
				j=0;
				while(j <= scmd[i].maxargs){
					while(*cp==' '){
						*cp='\0';
						cp++;
					}
					if(*cp=='\0') break;
					args[j++]=cp;
	
					while(*cp!=' ') cp++;
					if(*cp=='\0') break;
				}
	
				if(j < scmd[i].minargs || j > scmd[i].maxargs){
					if(scmd[i].errorhlp) msg_print(Ind, scmd[i].errorhlp);
					return;
				}
	
				args[j]=NULL;
				if(scmd[i].maxargs==1){
					scmd[i].func(Ind, args[0]);
				}
				else scmd[i].func(Ind, args);
			}
			return;
		}
	}
	do_slash_brief_help(Ind);
}

void sc_shout(int Ind, void *string){
	player_type *p_ptr=Players[Ind];

	aggravate_monsters(Ind, 1);
	if(string)
		msg_format_near(Ind, "\377B%^s shouts:%s", p_ptr->name, (char*)string);
	else
		msg_format_near(Ind, "\377BYou hear %s shout!", p_ptr->name);
	msg_format(Ind, "\377BYou shout %s", (char*)string);
}

void sc_shutdown(int Ind, void *value){
	bool kick = (cfg.runlevel == 1024);

	int val;

	if((char*)value)
		val=atoi((char*)value);
	else
		val=(cfg.runlevel < 6 || kick) ? 6 : 5;

	set_runlevel(val);

	msg_format(Ind, "Runlevel set to %d", cfg.runlevel);

	/* Hack -- character edit mode */
	if (val == 1024 || kick)
	{
		int i;
		if (val == 1024) msg_print(Ind, "\377rEntering edit mode!");
		else msg_print(Ind, "\377rLeaving edit mode!");

		for (i = NumPlayers; i > 0; i--)
		{
			/* Check connection first */
			if (Players[i]->conn == NOT_CONNECTED)
				continue;

			/* Check for death */
			if (!is_admin(Players[i]))
				Destroy_connection(Players[i]->conn, "kicked out");
		}
	}
	time(&cfg.closetime);
}

void sc_wish(int Ind, void *argp){
	char **args=(args);
#if 0
	object_type	forge;
	object_type	*o_ptr = &forge;

	if (tk < 1 || !k)
	{
		msg_print(Ind, "\377oUsage: /wish (tval) (sval) (pval) [discount] [name] or /wish (o_idx)");
		return;
	}

	invcopy(o_ptr, tk > 1 ? lookup_kind(k, atoi(token[2])) : k);

	/* Wish arts out! */
	if (tk > 4)
	{
		int nom = atoi(token[5]);
		o_ptr->number = 1;

		if (nom > 0) o_ptr->name1 = nom;
		else
		{
			/* It's ego or randarts */
			if (nom)
			{
				o_ptr->name2 = 0 - nom;
				if (tk > 4) o_ptr->name2b = 0 - atoi(token[5]);
			}
			else o_ptr->name1 = ART_RANDART;

			/* Piece together a 32-bit random seed */
			o_ptr->name3 = rand_int(0xFFFF) << 16;
			o_ptr->name3 += rand_int(0xFFFF);
		}
	}
	else
	{
		o_ptr->number = o_ptr->weight > 100 ? 2 : 99;
	}

	apply_magic(&p_ptr->wpos, o_ptr, -1, TRUE, TRUE, TRUE, FALSE, make_resf(p_ptr));
	if (tk > 3){
		o_ptr->discount = atoi(token[4]);
	}
	else{
		o_ptr->discount = 100;
	}
	object_known(o_ptr);
	o_ptr->owner = 0;
	if(tk>2)
		o_ptr->pval=atoi(token[3]);
	//o_ptr->owner = p_ptr->id;
	o_ptr->level = 1;
	(void)inven_carry(Ind, o_ptr);
#endif
}


void sc_report(int Ind, void *string){
	player_type *p_ptr=Players[Ind];
	rfe_printf("[%s]%s\n", p_ptr->name, (char*)string);
	msg_print(Ind, "\377GThank you for sending us a message!");
}

#else

/*
 * Litterally.	- Jir -
 */

static char *find_inscription(s16b quark, char *what)
{
    const char  *ax = quark_str(quark);
    if( ax == NULL || !what) { return FALSE; };
	
	return (strstr(ax, what));
}  

static void do_cmd_refresh(int Ind)
{
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	int k;

	/* Hack -- fix the inventory count */
	p_ptr->inven_cnt = 0;
	for (k = 0; k < INVEN_PACK; k++)
	{
		o_ptr = &p_ptr->inventory[k];

		/* Skip empty items */
		if (!o_ptr->k_idx || !o_ptr->tval) {
			/* hack - make sure the item is really erased - had some bugs there
			   since some code parts use k_idx, and some tval, to kill/test items - C. Blue */
			invwipe(o_ptr);
			continue;
		}

		p_ptr->inven_cnt++;
	}

	/* Clear the target */
	p_ptr->target_who = 0;

	/* Update his view, light, bonuses, and torch radius */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_BONUS | PU_TORCH |
			PU_DISTANCE | PU_SKILL_MOD);

	/* Recalculate mana */
	p_ptr->update |= (PU_MANA | PU_HP | PU_SANITY);

	/* Redraw */
	p_ptr->redraw |= PR_MAP | PR_EXTRA | PR_HISTORY | PR_VARIOUS;
	p_ptr->redraw |= (PR_HP | PR_GOLD | PR_BASIC | PR_PLUSSES);

	/* Notice */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	return;
}

/*
 * Slash commands - huge hack function for command expansion.
 *
 * TODO: introduce sl_info.txt, like:
 * N:/lua:1
 * F:ADMIN
 * D:/lua (LUA script command)
 */

void do_slash_cmd(int Ind, char *message)
{
	int i = 0;
	int k = 0, tk = 0;
	player_type *p_ptr = Players[Ind];
 	char *colon, *token[9], message2[80], message3[80];
	worldpos wp;
	bool admin = is_admin(p_ptr);

	strcpy(message2, message);
	wpcopy(&wp, &p_ptr->wpos);

	/* message3 contains all tokens but not the command: */
	strcpy(message3, "");
	for (i = 0; i < strlen(message); i++)
		if (message[i] == ' ') {
			strcpy(message3, message + i + 1);
			break;
		}

	/* Look for a player's name followed by a colon */
	colon = strchr(message, ' ');

	/* hack -- non-token ones first */
	if ((prefix(message, "/script") ||
			prefix(message, "/scr") ||
			prefix(message, "/ ") ||	// use with care!
			prefix(message, "//") ||	// use with care!
			prefix(message, "/lua")) && admin)
	{
		if (colon)
		{
			master_script_exec(Ind, colon);
		}
		else
		{
			msg_print(Ind, "\377oUsage: /lua (LUA script command)");
		}
		return;
	}
	else if ((prefix(message, "/rfe")) ||
			prefix(message, "/bug") ||
			prefix(message, "/cookie"))
	{
		if (colon)
		{
			rfe_printf("[%s]%s\n", p_ptr->name, colon);
			msg_print(Ind, "\377GThank you for sending us a message!");
		}
		else
		{
			msg_print(Ind, "\377oUsage: /rfe (message)");
		}
		return;
	}
	/* Oops conflict; took 'never duplicate' principal */
	else if (prefix(message, "/shout") ||
			prefix(message, "/sho"))
	{
		aggravate_monsters(Ind, 1);
		if (colon)
		{
			msg_format_near(Ind, "\377B%^s shouts:%s", p_ptr->name, colon);
			msg_format(Ind, "\377BYou shout:%s", colon);
		}
		else
		{
			msg_format_near(Ind, "\377BYou hear %s shout!", p_ptr->name);
			msg_format(Ind, "\377BYou shout!", colon);
		}
		break_cloaking(Ind);
		return;
	}
	/* RPG-style talking to people who are nearby, instead of global chat. - C. Blue */
	else if (prefix(message, "/say"))
	{
		if (colon)
		{
			msg_format_near(Ind, "\377B%^s says:%s", p_ptr->name, colon);
			msg_format(Ind, "\377BYou say:%s", colon);
		}
		else
		{
			msg_format_near(Ind, "\377B%s clears %s throat.", p_ptr->name, p_ptr->male ? "his" : "her");
			msg_format(Ind, "\377BYou clear your throat.", colon);
		}
		return;
// :)		break_cloaking(Ind);
	}

	else
	{
		/* cut tokens off (thx Asclep(DEG)) */
		if ((token[0]=strtok(message," ")))
		{
//			s_printf("%d : %s", tk, token[0]);
			for (i=1;i<9;i++)
			{
				token[i]=strtok(NULL," ");
				if (token[i]==NULL)
					break;
				tk = i;
			}
		}

		/* Default to no search string */
		//		strcpy(search, "");

		/* Form a search string if we found a colon */
		if (tk)
		{
			k = atoi(token[1]);
		}

		/* User commands */
		if (prefix(message, "/ignore") ||
				prefix(message, "/ig"))
		{
			add_ignore(Ind, token[1]);
			return;
		}
		if (prefix(message, "/ignchat") ||
				prefix(message, "/ic"))
		{
			if (p_ptr->ignoring_chat) {
				p_ptr->ignoring_chat = FALSE;
				msg_print(Ind, "\377yYou're no longer ignoring normal chat messages.");
			} else {
				p_ptr->ignoring_chat = TRUE;
				msg_print(Ind, "\377yYou're now ignoring normal chat messages.");
				msg_print(Ind, "\377yYou will only receive private and party messages.");
			}
			return;
		}
		else if (prefix(message, "/afk"))
		{
			if (strlen(message2 + 4) > 0)
			    toggle_afk(Ind, message2 + 5);
			else
			    toggle_afk(Ind, "");
			return;
		}

		else if (prefix(message, "/page"))
		{
			int p;
			if(!tk) {
				msg_print(Ind, "\377oUsage: /page <playername>");
				msg_print(Ind, "\377oAllows you to send a 'beep' sound to someone who is currently afk.");
				return;
			}
			p = name_lookup_loose(Ind, message3, FALSE);
			if (!p || (Players[Ind]->admin_dm && cfg.secret_dungeon_master && !is_admin(Players[Ind]))) {
				msg_format(Ind, "\377yPlayer %s not online.", message3);
				return;
			}
#if 0 /* no real need to restrict this */
			if (!Players[p]->afk) {
				msg_format(Ind, "\377yPlayer %s is not afk.", message3);
				return;
			}
#endif
#if 0 /* no need to tell him, same as for chat messages.. */
			if (check_ignore(p, Ind)) {
				msg_format(Ind, "\377yThat player is currently ignoring you.");
				return;
			}
#endif
			if (Players[p]->paging) {
				/* already paging, I hope this can prevent floods too - mikaelh */
				return;
			}
			if (!check_ignore(p, Ind)) Players[p]->paging = 3; /* Play 3 beeps quickly */
			msg_format(Ind, "\377yPaged %s.", message3);
			msg_format(p, "\377y%s is paging you.", Players[Ind]->name);
			return;
		}

		/* Semi-auto item destroyer */
		else if ((prefix(message, "/dispose")) ||
				prefix(message, "/dis"))
		{
			object_type		*o_ptr;
			u32b f1, f2, f3, f4, f5, esp;
			bool nontag = FALSE;

			disturb(Ind, 1, 0);

			/* only tagged ones? */
			if (tk > 0 && prefix(token[1], "a")) nontag = TRUE;

			for(i = 0; i < INVEN_PACK; i++)
			{
				bool resist = FALSE;
				o_ptr = &(p_ptr->inventory[i]);
				if (!o_ptr->tval) break;

				object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

				/* skip inscribed items */
				/* skip non-matching tags */
				if (o_ptr->note && 
					strcmp(quark_str(o_ptr->note), "terrible") &&
					strcmp(quark_str(o_ptr->note), "cursed") &&
					strcmp(quark_str(o_ptr->note), "uncursed") &&
					strcmp(quark_str(o_ptr->note), "broken") &&
					strcmp(quark_str(o_ptr->note), "average") &&
					strcmp(quark_str(o_ptr->note), "good") &&
//					strcmp(quark_str(o_ptr->note), "excellent"))
					strcmp(quark_str(o_ptr->note), "worthless"))
					continue;

				if (!nontag && !o_ptr->note && !k &&
					!(cursed_p(o_ptr) &&	/* Handle {cursed} */
						(object_known_p(Ind, o_ptr) ||
						 (o_ptr->ident & ID_SENSE))))
					continue;

				/* Player might wish to identify it first */
				if (k_info[o_ptr->k_idx].has_flavor &&
						!p_ptr->obj_aware[o_ptr->k_idx])
					continue;

				/* Hrm, this cannot be destroyed */
				if (((f4 & TR4_CURSE_NO_DROP) && cursed_p(o_ptr)) ||
						artifact_p(o_ptr))
					resist = TRUE;

				/* Hack -- filter by value */
				if (k && (!object_known_p(Ind, o_ptr) ||
							object_value_real(Ind, o_ptr) > k))
					continue;

				do_cmd_destroy(Ind, i, o_ptr->number);
				if (!resist) i--;

				/* Hack - Don't take a turn here */
				p_ptr->energy += level_speed(&p_ptr->wpos);
			}
			/* Take total of one turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);
			return;
		}

		/* add inscription to everything */
		else if (prefix(message, "/tag"))
		{
			object_type		*o_ptr;
			for(i = 0; i < INVEN_PACK; i++)
			{
				o_ptr = &(p_ptr->inventory[i]);
				if (!o_ptr->tval) break;

				/* skip inscribed items */
				if (o_ptr->note) continue;

				o_ptr->note = quark_add(token[1] ? token[1] : "!k");
			}
			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP);

			return;
		}
		/* remove specific inscription */
		else if (prefix(message, "/untag"))
		{
			object_type		*o_ptr;
//			cptr	*ax = token[1] ? token[1] : "!k";
			cptr	ax = token[1] ? token[1] : "!k";

			for(i = 0; i < INVEN_PACK; i++)
			{
				o_ptr = &(p_ptr->inventory[i]);
				if (!o_ptr->tval) break;

				/* skip inscribed items */
				if (!o_ptr->note) continue;

				/* skip non-matching tags */
				if (strcmp(quark_str(o_ptr->note), ax)) continue;

				o_ptr->note = 0;
			}

			/* Combine the pack */
			p_ptr->notice |= (PN_COMBINE);

			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP);

			return;
		}
		/* '/cast' code is written by Asclep(DEG). thx! */
		else if (prefix(message, "/cast"))
		{
			msg_print(Ind, "\377oSorry, /cast is not available for the time being.");
#if 0 // TODO: make that work without dependance on CLASS_
                        int book, whichplayer, whichspell;
			bool ami = FALSE;
#if 0
			token[0]=strtok(message," ");
			if (token[0]==NULL)
			{
				msg_print(Ind, "\377oUsage: /cast (Book) (Spell) [Playername]");
				return;
			}

			for (i=1;i<50;i++)
			{
				token[i]=strtok(NULL," ");
				if (token[i]==NULL)
					break;
			}
#endif

			/* at least 2 arguments required */
			if (tk < 2)
			{
				msg_print(Ind, "\377oUsage: /cast (Book) (Spell) [Player name]");
				return;
			}

			if(*token[1]>='1' && *token[1]<='9')
			{	
				object_type *o_ptr;
				char c[4] = "@";
				bool found = FALSE;

				c[1] = ((p_ptr->pclass == CLASS_PRIEST) ||
						(p_ptr->pclass == CLASS_PALADIN)? 'p':'m');
				if (p_ptr->pclass == CLASS_WARRIOR) c[1] = 'n';
				c[2] = *token[1];
				c[3] = '\0';

				for(i = 0; i < INVEN_PACK; i++)
				{
					o_ptr = &(p_ptr->inventory[i]);
					if (!o_ptr->tval) break;

					if (find_inscription(o_ptr->note, c))
					{
						book = i;
						found = TRUE;
						break;
					}
				}

				if (!found)
				{
					msg_format(Ind, "\377oInscription {%s} not found.", c);
					return;
				}
				//					book = atoi(token[1])-1;
			}	
			else
			{	
				*token[1] &= ~(0x20);
				if(*token[1]>='A' && *token[1]<='W')
				{	
					book = (int)(*token[1]-'A');
				}		
				else 
				{
					msg_print(Ind,"\377oBook variable was out of range (a-i) or (1-9)");
					return;
				}	
			}

			if(*token[2]>='1' && *token[2]<='9')
			{	
				//					whichspell = atoi(token[2]+'A'-1);
				whichspell = atoi(token[2]) - 1;
			}	
			else if(*token[2]>='a' && *token[2]<='i')
			{	
				whichspell = (int)(*token[2]-'a');
			}		
			/* if Capital letter, it's for friends */
			else if(*token[2]>='A' && *token[2]<='I')
			{
				whichspell = (int)(*token[2]-'A');
				//					*token[2] &= 0xdf;
				//					whichspell = *token[2]-1;
				ami = TRUE;
			}
			else 
			{
				msg_print(Ind,"\377oSpell out of range [A-I].");
				return;
			}	

			if (token[3])
			{
				if (!(whichplayer = name_lookup_loose(Ind, token[3], TRUE)))
					return;

				if (whichplayer == Ind)
				{
					msg_print(Ind,"You feel lonely.");
				}
				/* Ignore "unreasonable" players */
				else if (!target_able(Ind, 0 - whichplayer))
				{
					msg_print(Ind,"\377oThat player is out of your sight.");
					return;
				}
				else
				{
//					msg_format(Ind,"Book = %ld, Spell = %ld, PlayerName = %s, PlayerID = %ld",book,whichspell,token[3],whichplayer); 
					target_set_friendly(Ind,5,whichplayer);
					whichspell += 64;
				}
			}
			else if (ami)
			{
				target_set_friendly(Ind, 5);
				whichspell += 64;
			}
			else
			{
				target_set(Ind, 5);
			}

			switch (p_ptr->pclass)
			{
			}

//			msg_format(Ind,"Book = %ld, Spell = %ld, PlayerName = %s, PlayerID = %ld",book,whichspell,token[3],whichplayer); 
#endif
                        return;
		}
		/* Take everything off */
		else if ((prefix(message, "/bed")) ||
				prefix(message, "/naked"))
		{
			byte start = INVEN_WIELD, end = INVEN_TOTAL;
			object_type		*o_ptr;

			if (!tk)
			{
				start = INVEN_BODY;
				end = INVEN_FEET + 1;
			}

			disturb(Ind, 1, 0);

//			for (i=INVEN_WIELD;i<INVEN_TOTAL;i++)
			for (i = start; i < end; i++)
			{
				o_ptr = &(p_ptr->inventory[i]);
				if (!o_ptr->tval) continue;

				/* skip inscribed items */
				/* skip non-matching tags */
				if ((check_guard_inscription(o_ptr->note, 't')) ||
					(check_guard_inscription(o_ptr->note, 'T')) ||
					(cursed_p(o_ptr))) continue;

				inven_takeoff(Ind, i, 255);
				p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;
			}
			return;
		}

		/* Try to wield everything */
		else if ((prefix(message, "/dress")) ||
				prefix(message, "/dr"))
		{
			object_type		*o_ptr;
			int j;
			bool gauche = FALSE;

			/* Paralyzed? */
			if (p_ptr->energy < level_speed(&p_ptr->wpos)) return;

			disturb(Ind, 1, 0);

			for (i=INVEN_WIELD;i<INVEN_TOTAL;i++)
			{
				if (!item_tester_hook_wear(Ind, i)) continue;

				o_ptr = &(p_ptr->inventory[i]);
				if (o_ptr->tval) continue;

				for(j = 0; j < INVEN_PACK; j++)
				{
					o_ptr = &(p_ptr->inventory[j]);
					if (!o_ptr->k_idx) break;

					/* skip unsuitable inscriptions */
					if (o_ptr->note &&
							(!strcmp(quark_str(o_ptr->note), "cursed") ||
							 !strcmp(quark_str(o_ptr->note), "terrible") ||
							 !strcmp(quark_str(o_ptr->note), "worthless") ||
							 check_guard_inscription(o_ptr->note, 'w')) )continue;

					if (!object_known_p(Ind, o_ptr)) continue;
					if (cursed_p(o_ptr)) continue;

					/* Already used? */
					if (!tk && wield_slot(Ind, o_ptr) != i) continue;

					/* Limit to items with specified strings, if any */
					if (tk && (!o_ptr->note ||
								!strstr(quark_str(o_ptr->note), token[1])))
						continue;

					do_cmd_wield(Ind, j, 0x0);

					/* MEGAHACK -- tweak to handle rings right */
					if (o_ptr->tval == TV_RING && !gauche)
					{
						i -= 2;
						gauche = TRUE;
					}

					break;
				}
			}
			return;
		}
		/* Display extra information */
		else if (prefix(message, "/extra") ||
				prefix(message, "/examine") ||
				prefix(message, "/ex"))
		{
			if (admin) msg_format(Ind, "The game turn: %d", turn);
			do_cmd_time(Ind);

#ifdef ENABLE_STANCES
			if (get_skill(p_ptr, SKILL_STANCE)) {
				switch (p_ptr->combat_stance) {
				case 0: msg_print(Ind, "You are currently in balanced combat stance."); break;
				case 1: switch (p_ptr->combat_stance_power) {
					case 0: msg_print(Ind, "You are currently in defensive combat stance rank I."); break;
					case 1: msg_print(Ind, "You are currently in defensive combat stance rank II."); break;
					case 2: msg_print(Ind, "You are currently in defensive combat stance rank III."); break;
					case 3: msg_print(Ind, "You are currently in Royal Rank defensive combat stance."); break;
					} break;
				case 2: switch (p_ptr->combat_stance_power) {
					case 0: msg_print(Ind, "You are currently in offensive combat stance rank I."); break;
					case 1: msg_print(Ind, "You are currently in offensive combat stance rank II."); break;
					case 2: msg_print(Ind, "You are currently in offensive combat stance rank III."); break;
					case 3: msg_print(Ind, "You are currently in Royal Rank offensive combat stance."); break;
					} break;
				}
			}
#endif

//			do_cmd_knowledge_dungeons(Ind);
			if (p_ptr->depth_in_feet) msg_format(Ind, "The deepest point you've reached: \377G-%d\377wft", p_ptr->max_dlv * 50);
			else msg_format(Ind, "The deepest point you've reached: Lev \377G-%d", p_ptr->max_dlv);

			msg_format(Ind, "You can move %d.%d times each turn.",
					extract_energy[p_ptr->pspeed] / 10,
					extract_energy[p_ptr->pspeed]
					- (extract_energy[p_ptr->pspeed] / 10) * 10);

			if (get_skill(p_ptr, SKILL_DODGE)) use_ability_blade(Ind);

			/* Insanity warning (better message needed!) */
			if (p_ptr->csane < p_ptr->msane / 8)
				msg_print(Ind, "\377rYou can hardly resist the temptation to cry out!");
			else if (p_ptr->csane < p_ptr->msane / 4)
				msg_print(Ind, "\377yYou feel insanity about to grasp your mind..");
			else if (p_ptr->csane < p_ptr->msane / 2)
				msg_print(Ind, "\377yYou feel insanity creep into your mind..");
			else
				msg_print(Ind, "\377wYou are sane.");

			if (p_ptr->body_monster)
			{
				monster_race *r_ptr = &r_info[p_ptr->body_monster];
				msg_format(Ind, "You %shave a head.", r_ptr->body_parts[BODY_HEAD] ? "" : "don't ");
				msg_format(Ind, "You %shave arms.", r_ptr->body_parts[BODY_ARMS] ? "" : "don't ");
				msg_format(Ind, "You can %s use weapons.", r_ptr->body_parts[BODY_WEAPON] ? "" : "not");
				msg_format(Ind, "You can %s wear %s.", r_ptr->body_parts[BODY_FINGER] ? "" : "not", r_ptr->body_parts[BODY_FINGER] == 1 ? "a ring" : "rings");
				msg_format(Ind, "You %shave a torso.", r_ptr->body_parts[BODY_TORSO] ? "" : "don't ");
				msg_format(Ind, "You %shave legs suitable for shoes.", r_ptr->body_parts[BODY_LEGS] ? "" : "don't ");
			} else if (p_ptr->fruit_bat) {
				msg_format(Ind, "You have a head.");
				msg_format(Ind, "You can wear rings.");
				msg_format(Ind, "You don't have a torso, but you can wear cloaks.");
			}

			if (admin)
			{
				cave_type **zcave;
				cave_type *c_ptr;

				msg_format(Ind, "your sanity: %d/%d", p_ptr->csane, p_ptr->msane);
				msg_format(Ind, "server status: m_max(%d) o_max(%d)",
						m_max, o_max);

				msg_print(Ind, "Colour test - \377ddark \377wwhite \377sslate \377oorange \377rred \377ggreen \377bblue \377uumber");
				msg_print(Ind, "\377Dl_dark \377Wl_white \377vviolet \377yyellow \377Rl_red \377Gl_green \377Bl_blue \377Ul_umber");
				if(!(zcave=getcave(&wp)))
				{
					msg_print(Ind, "\377rOops, the cave's not allocated!!");
					return;
				}
				c_ptr=&zcave[p_ptr->py][p_ptr->px];

				msg_format(Ind, "(x:%d y:%d) info:%d feat:%d o_idx:%d m_idx:%d effect:%d",
						p_ptr->px, p_ptr->py,
						c_ptr->info, c_ptr->feat, c_ptr->o_idx, c_ptr->m_idx, c_ptr->effect);
			}

			int lev = p_ptr->lev;

			if (p_ptr->pclass == CLASS_DRUID) { /* compare mimic_druid in defines.h */
				if (lev >= 5) msg_print(Ind, "\377GYou know how to change into a Cave Bear (#160) and Panther (#198)");
				if (lev >= 10) msg_print(Ind, "\377GYou know how to change into a Grizzly Bear (#191) and Yeti (#154)");
				if (lev >= 15) msg_print(Ind, "\377GYou know how to change into a Griffon (#279) and Sasquatch (#343)");
				if (lev >= 20) msg_print(Ind, "\377GYou know how to change into a Werebear (#414), Great Eagle (#335), Aranea (#963) and White Shark (#901)");
				if (lev >= 25) msg_print(Ind, "\377GYou know how to change into a Wyvern (#334) and Multi-hued Hound (#513)");
				if (lev >= 30) msg_print(Ind, "\377GYou know how to change into a 5-h-Hydra (#440), Minotaur (#641) and Giant Squid (#482)");
				if (lev >= 35) msg_print(Ind, "\377GYou know how to change into a 7-h-Hydra (#614), Elder Aranea (#964) and Plasma Hound (#726)");
				if (lev >= 40) msg_print(Ind, "\377GYou know how to change into an 11-h-Hydra (#688), Giant Roc (#640) and Lesser Kraken (#740)");
				if (lev >= 45) msg_print(Ind, "\377GYou know how to change into a Maulotaur (#723), Winged Horror (#704) and Behemoth (#716)");
				if (lev >= 50) msg_print(Ind, "\377GYou know how to change into a Spectral tyrannosaur (#705), Jabberwock (#778) and Leviathan (#782)");
			}
			
			if (p_ptr->prace == RACE_VAMPIRE) {
				if (lev >= 20) msg_print(Ind, "\377GYou are able to turn into a vampire bat (#391).");
			}
#ifdef CLASS_RUNEMASTER
			if (p_ptr->pclass == CLASS_RUNEMASTER) {
				msg_format(Ind, "\377BYour rune mastery rating is %d.", RUNE_DMG);
#ifdef ALTERNATE_DMG
				if (get_skill_scale(p_ptr, SKILL_RUNEMASTERY, 50) < 25) {
					msg_format(Ind, "\377yYou do not use the full SP mentioned here.");
					msg_format(Ind, "\377yYou do not do the full damage mentioned here.");
				}
#endif
				msg_format(Ind, "\377B|  Type  |   Damage   | Cost (base, medium, adv)");
				msg_format(Ind, "\377B|  Bolt  | %2dd%2.0d + 3  | %4.0f, %4.0f, %4.0f", 
					(int)(1 + RUNE_DMG), (int)(1 + RUNE_DMG/2),
					(RUNE_DMG*RBASIC_COST*RBOLT_BASE),
					(RUNE_DMG*RMEDIUM_COST*RBOLT_BASE),
					(RUNE_DMG*RADVANCE_COST*RBOLT_BASE));
				msg_format(Ind, "\377B|  Beam  | %2dd%2.0d + 3  | %4.0f, %4.0f, %4.0f", 
					(int)(1 + RUNE_DMG), (int)(1 + RUNE_DMG/3),
					(RUNE_DMG*RBASIC_COST*RBEAM_BASE),
					(RUNE_DMG*RMEDIUM_COST*RBEAM_BASE),
					(RUNE_DMG*RADVANCE_COST*RBEAM_BASE));
				msg_format(Ind, "\377B|  Ball  | %2dd%2.0d + 3  | %4.0f, %4.0f, %4.0f", 
					(int)(1 + RUNE_DMG), (int)(1 + RUNE_DMG/3),
					(RUNE_DMG*RBASIC_COST*RBALL_BASE),
					(RUNE_DMG*RMEDIUM_COST*RBALL_BASE),
					(RUNE_DMG*RADVANCE_COST*RBALL_BASE)); 
				msg_format(Ind, "\377B|  Cloud | %2d dur:%2d  | %4.0f, %4.0f, %4.0f", 
					(int)(rcloud_dmg(RUNE_DMG)), (int)(RCLOUD_DURATION),
					(RUNE_DMG*RBASIC_COST*RCLOUD_BASE),
					(RUNE_DMG*RMEDIUM_COST*RCLOUD_BASE),
					(RUNE_DMG*RADVANCE_COST*RCLOUD_BASE));

				lev = get_skill(p_ptr, SKILL_RUNEMASTERY);

#ifdef ALTERNATE_DMG
				if (lev >= RBARRIER) msg_print(Ind, "\377BYou are able to cast with your full potential");
#endif

#if ALLOW_PERFECT_RUNE_CASTING
				if (lev >= 15) msg_print(Ind, "\377BYou have perfect bolt casting.");
				if (lev >= 25) msg_print(Ind, "\377BYou have perfect beam casting.");
				if (lev >= 35) msg_print(Ind, "\377BYou have perfect ball casting.");
				if (lev >= 50) msg_print(Ind, "\377BYou have perfect cloud casting."); //omg wtf woot ^_^"
#else
				msg_print(Ind, "\377BFailure rate: 0.1%%"); 
#endif
				lev = p_ptr->lev; //restore ^^"
			}
#endif

			return;
		}
		/* Please add here anything you think is needed.  */
		else if ((prefix(message, "/refresh")) ||
				prefix(message, "/ref"))
		{
			do_cmd_refresh(Ind);
			return;
		}
		else if ((prefix(message, "/target")) ||
				prefix(message, "/tar"))
		{
			int tx, ty;

			/* Clear the target */
			p_ptr->target_who = 0;

			/* at least 2 arguments required */
			if (tk < 2)
			{
				msg_print(Ind, "\377oUsage: /target (X) (Y) <from your position>");
				return;
			}

			tx = p_ptr->px + k;
			ty = p_ptr->py + atoi(token[2]);
			
			if (!in_bounds(ty,tx))
			{
				msg_print(Ind, "\377oIllegal position!");
				return;
			}

			/* Set the target */
			p_ptr->target_row = ty;
			p_ptr->target_col = tx;

			/* Set 'stationary' target */
			p_ptr->target_who = 0 - MAX_PLAYERS - 2;
			
			return;
		}
		/* Now this command is opened for everyone */
		else if (prefix(message, "/recall") ||
				prefix(message, "/rec"))
		{
			if (admin)
			{
				set_recall_timer(Ind, 1);
			}
			else
			{
				int item=-1;
				object_type *o_ptr;

				/* Paralyzed or just not enough energy left to perform a move? */
#if 0 /* this also prevents recalling while resting, too harsh maybe */
				if (p_ptr->energy < level_speed(&p_ptr->wpos)) return;
#else
				if (p_ptr->paralyzed) return;
#endif

				/* Turn off resting mode */
				disturb(Ind, 0, 0);

				for(i = 0; i < INVEN_PACK; i++)
				{
					o_ptr = &(p_ptr->inventory[i]);
					if (!o_ptr->tval) break;

					if (find_inscription(o_ptr->note, "@R"))
					{
						item = i;
						break;
					}
				}

				if (item==-1)
				{
					msg_print(Ind, "\377oInscription {@R} not found.");
					//return;
				}
				else
				{
					int spell;
					disturb(Ind, 1, 0);

					/* ALERT! Hard-coded! */
					switch (o_ptr->tval)
					{
						case TV_SCROLL:
							do_cmd_read_scroll(Ind, item);
							break;
						case TV_ROD:
							do_cmd_zap_rod(Ind, item);
							break;
						/* Cast Recall spell - mikaelh */
						case TV_BOOK:
							spell=exec_lua(Ind, "return find_spell(\"Recall\")");
							if (o_ptr->sval == SV_SPELLBOOK)
							{
								if (o_ptr->pval != spell)
								{
									msg_print(Ind, "\377oThis is not Spellbook of Recall.");
									return;
								}
							}
							else
							{
								if (exec_lua(Ind, format("return spell_in_book(%d, %d)", o_ptr->sval, spell)) == FALSE)
								{
									msg_print(Ind, "\377oRecall spell not found in this book.");
									return;
								}
							}
							cast_school_spell(Ind, item, spell, -1, -1, 0);
							break;
						default:
							do_cmd_activate(Ind, item);
							//msg_print(Ind, "\377oYou cannot recall with that.");
							break;
					}
				}
			}

			switch (tk)
			{
				case 1:
					/* depth in feet */
					p_ptr->recall_pos.wz = k / (p_ptr->depth_in_feet ? 50 : 1);
					break;

				case 2:
					p_ptr->recall_pos.wx = k % MAX_WILD_X;
					p_ptr->recall_pos.wy = atoi(token[2]) % MAX_WILD_Y;
					p_ptr->recall_pos.wz = 0;
					break;

//				default:	/* follow the inscription */
					/* TODO: support tower */
//					p_ptr->recall_pos.wz = 0 - p_ptr->max_dlv;
//					p_ptr->recall_pos.wz = 0;
			}

			return;
		}
		/* TODO: remove &7 viewer commands */
		/* view RFE file or any other files in lib/data. */
		else if (prefix(message, "/less")) 
		{
			char    path[MAX_PATH_LENGTH];
			if (tk && is_admin(p_ptr))
			{
				//					if (strstr(token[1], "log") && is_admin(p_ptr))
				{
					//						path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_TEXT, "mangband.log");
					path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_DATA, token[1]);
					do_cmd_check_other_prepare(Ind, path);
					return;
				}
				//					else if (strstr(token[1], "rfe") &&

			}
			/* default is "mangband.rfe" */
			else if ((is_admin(p_ptr) || cfg.public_rfe))
			{
				path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_DATA, "tomenet.rfe");
				do_cmd_check_other_prepare(Ind, path);
				return;
			}
			else msg_print(Ind, "\377o/less is not opened for use...");
			return;
		}
		else if (prefix(message, "/news")) 
		{
			char    path[MAX_PATH_LENGTH];
			path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_TEXT, "news.txt");
			do_cmd_check_other_prepare(Ind, path);
			return;
		}
		else if (prefix(message, "/version") ||
				prefix(message, "/ver"))
		{
			if (tk) do_cmd_check_server_settings(Ind);
			else msg_print(Ind, longVersion);

			return;
		}
		else if (prefix(message, "/help") ||
				prefix(message, "/he") ||
				prefix(message, "/?"))
		{
			char    path[MAX_PATH_LENGTH];

			/* Build the filename */
			if (admin && !tk) path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_TEXT, "slash_ad.hlp");
			else path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_TEXT, "slash.hlp");

			do_cmd_check_other_prepare(Ind, path);
			return;
		}
		else if(prefix(message, "/pkill") ||
				prefix(message, "/pk"))
		{
			set_pkill(Ind, admin? 10 : 200);
			return;
		}
		/* TODO: move it to the Mayor's house */
		else if(prefix(message, "/quest") ||
				prefix(message, "/que"))	/* /quIt */
		{
			int i, j=Ind; //k=0;
			u16b r, num;
			int lev;
			u16b flags=(QUEST_MONSTER|QUEST_RANDOM);
	
			if(tk && !strcmp(token[1], "reset")){
				int qn;
				if(!admin) return;
				for(qn=0;qn<20;qn++){
					quests[qn].active=0;
					quests[qn].type=0;
					quests[qn].id=0;
				}
				msg_broadcast(0, "\377yQuests are reset");
				for(i=1; i<=NumPlayers; i++){
					if(Players[i]->conn==NOT_CONNECTED) continue;
					Players[i]->quest_id=0;
				}
				return;
			}
			if(tk && !strcmp(token[1], "guild")){
				if(!p_ptr->guild || guilds[p_ptr->guild].master != p_ptr->id){
					msg_print(Ind, "\377rYou are not a guildmaster");
					return;
				}
				if(tk==2){
					if((j=name_lookup_loose(Ind, token[2], FALSE))){
						if(Players[j]->quest_id){
							msg_format(Ind, "\377y%s has a quest already.", Players[j]->name);
							return;
						}
					}
					else{
						msg_format(Ind, "Player %s is not here", token[2]);
						return;
					}
				}
				else{
					msg_print(Ind, "Usage: /quest guild name");
					return;
				}
				flags|=QUEST_GUILD;
				lev=Players[j]->lev+5;
			}
			else if(admin && tk){
				if((j=name_lookup_loose(Ind, token[1], FALSE))){
					if(Players[j]->quest_id){
						msg_format(Ind, "\377y%s has a quest already.", Players[j]->name);
						return;
					}
					lev=Players[j]->lev;	/* for now */
				}
				else return;
			}
			else{
				flags|=QUEST_RACE;
				lev=Players[j]->lev;
			}
			if (prepare_quest(Ind, j, flags, &lev, &r, &num))
#if 0 /* done in prepare_quest */
			if(Players[j]->quest_id){
				for(i=0; i<20; i++){
					if(quests[i].id==Players[j]->quest_id){
						if(j==Ind)
							msg_format(Ind, "\377oYour quest to kill \377y%d \377g%s \377ois not complete.%s", p_ptr->quest_num, r_name+r_info[quests[i].type].name, quests[i].flags&QUEST_GUILD?" (guild)":"");
						return;
					}
				}
			}
			
			/* don't start too early -C. Blue */
#ifndef RPG_SERVER
			if (Players[j]->lev < 5) {
				msg_print(Ind, "\377oYou need to be level 5 or higher to receive a quest!");
#else /* for ironman there's no harm in allowing early quests */
			if (Players[j]->lev < 3) {
				msg_print(Ind, "\377oYou need to be level 3 or higher to receive a quest!");
#endif
				return;
			}
			
			/* plev 1..50 -> mlev 1..100 (!) */
			if (lev <= 50) lev += (lev * lev) / 83;
			else lev = 80 + rand_int(20);

			get_mon_num_hook=dungeon_aux;
			get_mon_num_prep();
			i=2+randint(5);

			do{
				r=get_mon_num(lev, 0);
				k++;
                                if(k>1000) {
                                        lev--;
                                        k = lev * 5;
//                                      k = lev * 9;
//                                      k = 900;
                                }
			} while(	((lev-5) > r_info[r].level) || 
					(r_info[r].flags1 & RF1_UNIQUE) || 
					(r_info[r].flags7 & RF7_MULTIPLY) || 
					!(r_info[r].level > 2)); /* no Training Tower quests */
#ifndef RPG_SERVER
			if (r_info[r].flags1 & RF1_FRIENDS) i = i + 11 + randint(7);
#else /* very hard in the beginning in ironman dungeons */
			if (lev < 40) {
				if (r_info[r].flags1 & RF1_FRIENDS) i = i * 2;
				else i = (i + 1) / 2;
			} else {
				if (r_info[r].flags1 & RF1_FRIENDS) i = i + 11 + randint(7);
			}
#endif
#endif /* if 0 */
			add_quest(Ind, j, r, num, flags);
			return;
		}
		else if (prefix(message, "/feeling") ||
				prefix(message, "/fe"))
		{
			if (!show_floor_feeling(Ind))
				msg_print(Ind, "You feel nothing special.");
			return;
		}
		else if (prefix(message, "/monster") ||
				prefix(message, "/mo"))
		{
			int r_idx, num;
			monster_race *r_ptr;
			if (!tk)
			{
				do_cmd_show_monster_killed_letter(Ind, NULL);
				return;
			}

			/* Handle specification like 'D', 'k' */
			if (strlen(token[1]) == 1)
			{
				do_cmd_show_monster_killed_letter(Ind, token[1]);
				return;
			}

			/* Directly specify a name (tho no1 would use it..) */
			r_idx = race_index(token[1]);
			if (!r_idx)
			{
				msg_print(Ind, "No such monster.");
				return;
			}

			r_ptr = &r_info[r_idx];
			num = p_ptr->r_killed[r_idx];

			if (get_skill(p_ptr, SKILL_MIMIC) &&
			    !((p_ptr->pclass == CLASS_DRUID) && !mimic_druid(r_idx, p_ptr->lev)) &&
			    !((p_ptr->prace == RACE_VAMPIRE) && !mimic_vampire(r_idx, p_ptr->lev)) &&
			    !((p_ptr->pclass == CLASS_SHAMAN) && !mimic_shaman(r_idx)))
			{
				i = r_ptr->level - num;

	                        if ((i > 0)
				    && !((p_ptr->pclass == CLASS_DRUID) && mimic_druid(r_idx, p_ptr->lev))
				    && !((p_ptr->prace == RACE_VAMPIRE) && mimic_vampire(r_idx, p_ptr->lev)))
					msg_format(Ind, "%s : %d slain (%d more to go)",
							r_name + r_ptr->name, num, i);
				else
					msg_format(Ind, "%s : %d slain (learnt)", r_name + r_ptr->name, num);
			}
			else
			{
				msg_format(Ind, "%s : %d slain.", r_name + r_ptr->name, num);
			}

			/* TODO: show monster description */
			
			return;
		}
		/* add inscription to books */
		else if (prefix(message, "/autotag") ||
				prefix(message, "/at"))
		{
			object_type		*o_ptr;
			for(i = 0; i < INVEN_PACK; i++)
			{
				o_ptr = &(p_ptr->inventory[i]);
				auto_inscribe(Ind, o_ptr, tk);
			}
			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP);

			return;
		}
		else if (prefix(message, "/house") ||
				prefix(message, "/ho"))
		{
			do_cmd_show_houses(Ind);
			return;
		}
		else if (prefix(message, "/object") ||
				prefix(message, "/obj"))
		{
			if (!tk)
			{
				do_cmd_show_known_item_letter(Ind, NULL);
				return;
			}

			if (strlen(token[1]) == 1)
			{
				do_cmd_show_known_item_letter(Ind, token[1]);
				return;
			}

			return;
		}
		else if (prefix(message, "/sip"))
		{
			/* Paralyzed? */
			if (p_ptr->energy < level_speed(&p_ptr->wpos)) return;

			do_cmd_drink_fountain(Ind);
			return;
		}
		else if (prefix(message, "/fill"))
		{
			/* Paralyzed? */
			if (p_ptr->energy < level_speed(&p_ptr->wpos)) return;

			do_cmd_fill_bottle(Ind);
			return;
		}
		else if (prefix(message, "/empty") || prefix(message, "/emp"))
		{
			int slot;
			return;//disabled for anti-cheeze
			if (!tk)
			{
				msg_print(Ind, "\377oUsage: /empty (inventory slot letter)");
				return;
			}
			slot = (char)(token[1][0]);
			/* convert to upper case ascii code */
			if (slot >= 97 && slot <= 122) slot -= 32;
			/* check for valid inventory slot */
			if (slot < 65 || slot > (90 - 3))
			{
				msg_print(Ind, "\377oValid inventory slots are a-w (or A-W). Please try again.");
				return;
			}
			do_cmd_empty_potion(Ind, slot - 65);
			return;
		}
		else if (prefix(message, "/dice"))
		{
			int rn;
			if (tk < 1)
			{
				msg_print(Ind, "\377oUsage: /dice (number of dice)");
				return;
			}
			if ((k < 1) || (k > 100))
			{
				msg_print(Ind, "\377oNumber of dice must be between 1 and 100!");
				return;
			}
			if (p_ptr->energy < level_speed(&p_ptr->wpos)) return;
			p_ptr->energy -= level_speed(&p_ptr->wpos);
			rn = 0;
			for (i = 0; i < k; i++)
			{
			    rn += randint(6);
			}
			msg_format(Ind, "\377UYou throw %d dice and get a %d", k, rn);
			msg_format_near(Ind, "\377U%s throws %d dice and gets a %d", p_ptr->name, k, rn);
			return;
		}
#ifdef RPG_SERVER /* too dangerous on the pm server right now - mikaelh */
/* Oops, meant to be on RPG only for now. forgot to add it. thanks - the_sandman */
#if 0 //moved the old code here. 
		else if (prefix(message, "/pet")){
			if (tk && prefix(token[1], "force"))
			{
				summon_pet(Ind, 1);
				msg_print(Ind, "You summon a pet");
			}
			else
			{
				msg_print(Ind, "Pet code is under working; summoning is bad for your server's health.");
				msg_print(Ind, "If you still want to summon one, type \377o/pet force\377w.");
			}
			return;
		}
#endif 
                else if (prefix(message, "/pet")) {
			if (strcmp(Players[Ind]->accountname, "The_sandman")) {
				msg_format(Ind, "\377rPet system is disabled.");
				return;
			}
			if (Players[Ind]->has_pet == 2) {
				msg_format(Ind, "\377rYou cannot have anymore pets!");
				return;
			}
                        if (pet_creation(Ind))
				msg_format(Ind, "\377USummoning a pet.");
			else 
				msg_format(Ind, "\377rYou already have a pet!");
			return;
                }
#endif
                else if (prefix(message, "/unpet")) {
			#ifdef RPG_SERVER
			msg_format(Ind, "\377RYou abandon your pet! You cannot have anymore pets!");
			if (strcmp(Players[Ind]->accountname, "The_sandman")) return;
//			if (Players[Ind]->wpos.wz != 0) {
				for (i = m_top-1; i >= 0; i--) {
					monster_type *m_ptr = &m_list[i];
					if (!m_ptr->pet) continue;
					if (find_player(m_ptr->owner) == Ind) {
						m_ptr->pet = 0; //default behaviour!
						m_ptr->owner = 0;
						Players[Ind]->has_pet = 0; //spec value
						i = -1; //quit early
					}
				} 
//			} else {
//				msg_format(Ind, "\377yYou cannot abandon your pet while the whole town is looking!");
//			}
			#endif
			return;
                } 
                /* An alternative to dicing :) -the_sandman */
                else if (prefix(message, "/deal"))
                {
                        int value, flower;
                        char* temp;
                        temp = (char*)malloc(10*sizeof(char));
                        value = randint(13); flower = randint(4);
                        switch (value) {
                                case 1: strcpy(temp, "Ace"); break;
                                case 2: strcpy(temp, "Two"); break;
                                case 3: strcpy(temp, "Three"); break;
                                case 4: strcpy(temp, "Four"); break;
                                case 5: strcpy(temp, "Five"); break;
                                case 6: strcpy(temp, "Six"); break;
                                case 7: strcpy(temp, "Seven"); break;
                                case 8: strcpy(temp, "Eight"); break;
                                case 9: strcpy(temp, "Nine"); break;
                                case 10: strcpy(temp, "Ten"); break;
                                case 11: strcpy(temp, "Jack"); break;
                                case 12: strcpy(temp, "Queen"); break;
                                case 13: strcpy(temp, "King"); break;
                                default: strcpy(temp, "joker ^^"); break;
                        }
                        switch (flower) {
                                case 1:
                                        msg_format(Ind, "\377U%s was dealt the %s of Diamond", p_ptr->name, temp);
                                        msg_format_near(Ind, "\377U%s was dealt the %s of Diamond", p_ptr->name, temp);
                                        break;
                                case 2:
                                        msg_format(Ind, "\377U%s was dealt the %s of Club", p_ptr->name, temp);
                                        msg_format_near(Ind, "\377U%s was dealt the %s of Club", p_ptr->name, temp);
                                        break;
                                case 3:
                                        msg_format(Ind, "\377U%s was dealt the %s of Heart", p_ptr->name, temp);
                                        msg_format_near(Ind, "\377U%s was dealt the %s of Heart", p_ptr->name, temp);
                                        break;
                                case 4:
                                        msg_format(Ind, "\377U%s was dealt the %s of Spade", p_ptr->name, temp);
                                        msg_format_near(Ind, "\377U%s was dealt the %s of Spade", p_ptr->name, temp);
                                        break;
                                default:
                                        break;
                        }
                        free(temp); return;
                }
		else if (prefix(message, "/martyr") || prefix(message, "/mar"))
		{
			if (Players[Ind]->martyr_timeout)
				msg_print(Ind, "The heavens are not yet ready to accept your martyrium.");
			else
				msg_print(Ind, "The heavens are ready to accept your martyrium.");
			return;
		}
		else if (prefix(message, "/notep"))
		{
			int p = -1, i, j = 0;
			for (i = 1; i < MAX_PARTIES; i++) { /* was i = 0 but real parties start from i = 1 - mikaelh */
				if(!strcmp(parties[i].owner, p_ptr->name)) p = i;
			}
			if (p < 0) {
				msg_print(Ind, "\377oYou aren't a party owner.");
				return;
			}
			if (tk == 0)	/* Erase party note */
			{
				for (i = 0; i < MAX_PARTYNOTES; i++) {
					/* search for pending party note */
					if (!strcmp(party_note_target[i], parties[p_ptr->party].name)) {
						/* found a matching note */
						/* delete the note */
						strcpy(party_note_target[i], "");
						strcpy(party_note[i], "");
						break;
					}
				}
				msg_print(Ind, "\377oParty note has been erased.");
				return;
			}
			if (tk > 0)	/* Set new party note */
			{
				/* Search for begin of parms ( == text of the note) */
				for (i = 6; i < strlen(message2); i++)
					if (message2[i] == ' ')
						for (j = i; j < strlen(message2); j++)
							if (message2[j] != ' ')	{
								/* save start pos in j for latter use */
								i = strlen(message2);
								break;
							}

				/* search for pending party note to change it */
				for (i = 0; i < MAX_PARTYNOTES; i++) {
					if (!strcmp(party_note_target[i], parties[p_ptr->party].name)) {
						/* found a matching note */
						break;
					}
				}
				if (i < MAX_PARTYNOTES) {
					/* change existing party note to new text */
					strcpy(party_note[i], &message2[j]);
					msg_print(Ind, "\377yNote has been stored.");
					return;
				}
				
				/* seach for free spot to create a new party note */
				for (i = 0; i < MAX_PARTYNOTES; i++) {
	    				if (!strcmp(party_note[i], "")) {
						/* found a free memory spot */
						break;
					}
				}
				if (i == MAX_PARTYNOTES) {
					msg_format(Ind, "\377oSorry, the server reached the maximum of %d pending party notes.", MAX_PARTYNOTES);
					return;
				}
				strcpy(party_note_target[i], parties[p_ptr->party].name);
				strcpy(party_note[i], &message2[j]);
				msg_print(Ind, "\377yNote has been stored.");
				return;
			}
		}
		else if (prefix(message, "/noteg"))
		{
			int p = -1, i, j = 0;
			for (i = 0; i < MAX_GUILDS; i++) {
				if(guilds[i].master == p_ptr->id) p = i;
			}
			if (p < 0) {
				msg_print(Ind, "\377oYou aren't a guild master.");
				return;
			}
			if (tk == 0)	/* Erase guild note */
			{
				for (i = 0; i < MAX_GUILDNOTES; i++) {
					/* search for pending guild note */
					if (!strcmp(guild_note_target[i], guilds[p_ptr->guild].name)) {
						/* found a matching note */
						/* delete the note */
						strcpy(guild_note_target[i], "");
						strcpy(guild_note[i], "");
						break;
					}
				}
				msg_print(Ind, "\377oGuild note has been erased.");
				return;
			}
			if (tk > 0)	/* Set new Guild note */
			{
				/* Search for begin of parms ( == text of the note) */
				for (i = 6; i < strlen(message2); i++)
					if (message2[i] == ' ')
						for (j = i; j < strlen(message2); j++)
							if (message2[j] != ' ')	{
								/* save start pos in j for latter use */
								i = strlen(message2);
								break;
							}

				/* search for pending guild note to change it */
				for (i = 0; i < MAX_GUILDNOTES; i++) {
					if (!strcmp(guild_note_target[i], guilds[p_ptr->guild].name)) {
						/* found a matching note */
						break;
					}
				}
				if (i < MAX_GUILDNOTES) {
					/* change existing guild note to new text */
					strcpy(guild_note[i], &message2[j]);
					msg_format(Ind, "\377yNote has been stored.");
					return;
				}
				
				/* seach for free spot to create a new guild note */
				for (i = 0; i < MAX_GUILDNOTES; i++) {
	    				if (!strcmp(guild_note[i], "")) {
						/* found a free memory spot */
						break;
					}
				}
				if (!i == MAX_GUILDNOTES) {
					msg_format(Ind, "\377oSorry, the server reached the maximum of %d pending guild notes.", MAX_GUILDNOTES);
					return;
				}
				strcpy(guild_note_target[i], guilds[p_ptr->guild].name);
				strcpy(guild_note[i], &message2[j]);
				msg_format(Ind, "\377yNote has been stored.");
				return;
			}
		}
		else if (prefix(message, "/notes"))
		{
			int i, notes = 0;
			for (i = 0; i < MAX_NOTES; i++) {
				/* search for pending notes of this player */
				if (!strcmp(priv_note_sender[i], p_ptr->name)) {
					/* found a matching note */
					notes++;
				}
			}
			if (notes > 0) msg_format(Ind, "\377oYou wrote %d currently pending notes:", notes);
			else msg_format(Ind, "\377oYou didn't write any pending notes.");
			for (i = 0; i < MAX_NOTES; i++) {
				/* search for pending notes of this player */
				if (!strcmp(priv_note_sender[i], p_ptr->name)) {
					/* found a matching note */
					msg_format(Ind, "\377oTo %s: %s", priv_note_target[i], priv_note[i]);
				}
			}
			return;
		}
		else if (prefix(message, "/note"))
		{
			int i, notes = 0, found_note=MAX_NOTES, j = 0;
			bool colon = FALSE;
			char tname[80]; /* target's account name */
			struct account *c_acc;
			if (tk < 1)	/* Explain command usage */
			{
				msg_print(Ind, "\377oUsage: /note <player-account>[:<text>]");
				msg_print(Ind, "\377oExample: /note El Hazard:Hiho!");
				msg_print(Ind, "\377oNot specifiying any text will remove pending notes to that account.");
				msg_print(Ind, "\377oTo clear all pending notes of yours, type: /note *");
				return;
			}
			/* Does a colon appear? */
			for (i = 0; i < strlen(message2); i++)
				if (message2[i] == ':') colon = TRUE;
			/* Depending on colon existance, extract the target name */
			for (i = 5; i < strlen(message2); i++)
				if (message2[i] == ' ') {
					for (j = i; j < strlen(message2); j++)
						/* find where target name starts, save pos in j */
						if (message2[j] != ' ')	{
							for (i = j; i < strlen(message2); i++) {
								/* find ':' which terminates target name, save pos in i */
								if (message2[i] == ':') {
									/* extract target name up to the ':' */
									strcpy(tname, message2 + j);
									tname[i - j] = '\0';
									/* save i in j for latter use */
									j = i;
									break;
								}
							}
							if (i == strlen(message2)) {
								/* extract name till end of line (it's the only parm) */
								strcpy(tname, message2 + j);
							}
							break;
						}
					break;
				}
			/* does target account exist? */
			c_acc = GetAccount(tname, NULL, FALSE);
			if (!c_acc) {
				msg_print(Ind, "\377oThat account does not exist.");
				return;
			}
			/* No colon found -> only a name parm give */
			if (!colon)	/* Delete all pending notes to a specified player */
			{
				for (i = 0; i < MAX_NOTES; i++) {
					/* search for pending notes of this player to the specified player */
					if (!strcmp(priv_note_sender[i], p_ptr->name) &&
					    (!strcmp(priv_note_target[i], tname) || !strcmp(tname, "*"))) {
						/* found a matching note */
						notes++;
						strcpy(priv_note_sender[i], "");
						strcpy(priv_note_target[i], "");
						strcpy(priv_note[i], "");
					}
				}
				msg_format(Ind, "\377oDeleted %d notes.", notes);
				return;
			}

			/* Colon found, store a note to someone */
			/* Store a new note from this player to the specified player */

			/* Check whether player has his notes quota exceeded */
			for (i = 0; i < MAX_NOTES; i++) {
				if (!strcmp(priv_note_sender[i], p_ptr->name)) notes++;
			}
			if ((notes >= 3) && !is_admin(p_ptr)) {
				msg_print(Ind, "\377oYou have already reached the maximum of 3 pending notes per player.");
				return;
			}

			/* Look for free space to store the new note */
			for (i = 0; i < MAX_NOTES; i++) {
				if (!strcmp(priv_note[i], "")) {
					/* found a free memory spot */
					found_note = i;
					break;
				}
			}
			if (found_note == MAX_NOTES) {
				msg_format(Ind, "\377oSorry, the server reached the maximum of %d pending notes.", MAX_NOTES);
				return;
			}

			strcpy(priv_note_sender[found_note], p_ptr->name);
			strcpy(priv_note_target[found_note], tname);
			strcpy(priv_note[found_note], message2 + j + 1);
			msg_format(Ind, "\377yNote for account '%s' has been stored.", priv_note_target[found_note]);
			return;
		}
		else if (prefix(message, "/play")) /* for joining games - mikaelh */
		{
	
			if (p_ptr->team != 0 && gametype == EEGAME_RUGBY)
			{
				teams[p_ptr->team - 1]--;
				p_ptr->team = 0;
				msg_print(Ind, "\377gYou are no more playing!");
				return;
			}
			if (gametype == EEGAME_RUGBY)
			{
				msg_print(Ind, "\377gThere's a rugby game going on!");
				if (teams[0] > teams[1])
				{
					p_ptr->team = 2;
					teams[1]++;
					msg_print(Ind, "\377gYou have been added to the light blue team!");
				}
				else
				{
					p_ptr->team = 1;
					teams[0]++;
					msg_print(Ind, "\377gYou have been added to the light red team!");
				}
			}
			else
			{
				msg_print(Ind, "\377gThere's no game going on!");
			}
			return;
		}

#ifdef FUN_SERVER /* make wishing available to players for fun, until rollback happens - C. Blue */
		else if (prefix(message, "/wish"))
		{
			object_type	forge;
			object_type	*o_ptr = &forge;
			WIPE(o_ptr, object_type);
			int tval, sval, kidx;

			if (tk < 1 || !k)
			{
				msg_print(Ind, "\377oUsage: /wish (tval) (sval) (pval) [discount] [name] or /wish (o_idx)");
				return;
			}

			if (tk > 1) {
			    tval = k; sval = atoi(token[2]);
			    kidx = lookup_kind(tval, sval);
			}
			else kidx = k;
//			if (kidx == 238 || kidx == 909 || kidx == 888 || kidx == 920) return; /* Invuln pots, Learning pots, Invinc + Highland ammys */
			if (kidx == 239 || kidx == 616 || kidx == 626 || kidx == 595 || kidx == 179) return; /* ..and rumour scrolls (spam) */
			msg_print(Ind, format("%d",kidx));
			invcopy(o_ptr, kidx);

			if(tk>2) o_ptr->pval=(atoi(token[3]) < 15) ? atoi(token[3]) : 15;
			/* the next check is not needed (bpval is used, not pval) */
			if (kidx == 428 && o_ptr->pval > 3) o_ptr->pval = 3; //436  (limit EA ring +BLOWS)

			/* Wish arts out! */
			if (tk > 4)
			{
				int nom = atoi(token[5]);
				if (nom == 203 || nom == 218) return; /* Phasing ring, Wizard cloak */
				o_ptr->number = 1;

				if (nom > 0) o_ptr->name1 = nom;
				else
				{
					/* It's ego or randarts */
					if (nom)
					{
						o_ptr->name2 = 0 - nom;
						if (tk > 4) o_ptr->name2b = 0 - atoi(token[5]);
						/* the next check might not be needed (bpval is used, not pval?) */
						if ((o_ptr->name2 == 65 || o_ptr->name2b == 65 ||
						    o_ptr->name2 == 70 || o_ptr->name2b == 70 ||
						    o_ptr->name2 == 173 || o_ptr->name2b == 173 ||
						    o_ptr->name2 == 176 || o_ptr->name2b == 176 ||
						    o_ptr->name2 == 187 || o_ptr->name2b == 187)
						    && (o_ptr->pval > 3)) /* all +BLOWS items */
							o_ptr->pval = 3;
					}
					else o_ptr->name1 = ART_RANDART;

					/* Piece together a 32-bit random seed */
					o_ptr->name3 = rand_int(0xFFFF) << 16;
					o_ptr->name3 += rand_int(0xFFFF);
				}
			}
			else
			{
				o_ptr->number = o_ptr->weight > 100 ? 2 : 99;
			}

			apply_magic(&p_ptr->wpos, o_ptr, -1, !o_ptr->name2, TRUE, TRUE, FALSE, TRUE);
			if (tk > 3){
				o_ptr->discount = atoi(token[4]);
			}
			else{
				o_ptr->discount = 100;
			}
			object_known(o_ptr);
			o_ptr->owner = 0;
//			if(tk>2) o_ptr->pval=(atoi(token[3]) < 15) ? atoi(token[3]) : 15;
			//o_ptr->owner = p_ptr->id;
			o_ptr->level = 1;
			(void)inven_carry(Ind, o_ptr);

			return;
		}
#endif
		else if (prefix(message, "/geinfo")) /* get info on a global event */
		{
			int i, n=0, j;
			char ppl[75];
			bool found = FALSE;
			if (tk < 1) {
				msg_print(Ind, "\377d ");
				for (i = 0; i<MAX_GLOBAL_EVENTS; i++) if ((global_event[i].getype != GE_NONE) && (global_event[i].hidden==FALSE || admin)) {
					n++;
					if (n == 1) {
						msg_print(Ind, "\377sCurrently ongoing events:");
						msg_print(Ind, "\377sUse command '/geinfo <number>' to get information on a specific event.");
					}
					/* Event still in announcement phase? */
					if (global_event[i].announcement_time - ((turn - global_event[i].start_turn) / cfg.fps) > 0)
						msg_format(Ind, " # %d '%s' \377grecruits\377w for %d mins.", i+1, global_event[i].title, (global_event[i].announcement_time - ((turn - global_event[i].start_turn) / cfg.fps)) / 60);
					/* or has already begun? */
					else	msg_format(Ind, " # %d '%s' began %d mins ago.", i+1, global_event[i].title, -(global_event[i].announcement_time - ((turn - global_event[i].start_turn) / cfg.fps)) / 60);
				}
				if (!n) msg_print(Ind, "\377sNo events are currently running.");
				msg_print(Ind, "\377d ");
			} else if ((k < 1) || (k > MAX_GLOBAL_EVENTS)) {
				msg_format(Ind, "Usage: /geinfo    or    /geinfo 1..%d", MAX_GLOBAL_EVENTS);
			} else if ((global_event[k-1].getype == GE_NONE) && (global_event[k-1].hidden==FALSE || admin)) {
				msg_print(Ind, "\377yThere is currently no running event of that number.");
			} else {
				msg_format(Ind, "\377sInfo on event #%d '\377s%s\377s':", k, global_event[k-1].title);
				for (i = 0; i<6; i++) if (strcmp(global_event[k-1].description[i], ""))
					msg_print(Ind, global_event[k-1].description[i]);
//				msg_print(Ind, "\377d ");
				if ((global_event[k-1].announcement_time - (turn - global_event[k-1].start_turn) / cfg.fps) >= 120) {
					msg_format(Ind, "\377WThis event will start in %d minutes.", (global_event[k-1].announcement_time - ((turn - global_event[k-1].start_turn) / cfg.fps)) / 60);
				} else if ((global_event[k-1].announcement_time - (turn - global_event[k-1].start_turn) / cfg.fps) > 0) {
					msg_format(Ind, "\377WThis event will start in %d seconds.", global_event[k-1].announcement_time - ((turn - global_event[k-1].start_turn) / cfg.fps));
				} else if ((global_event[k-1].announcement_time - (turn - global_event[k-1].start_turn) / cfg.fps) > -120) {
					msg_format(Ind, "\377WThis event has been running for %d seconds.", -global_event[k-1].announcement_time + ((turn - global_event[k-1].start_turn) / cfg.fps));
				} else {
					msg_format(Ind, "\377WThis event has been running for %d minutes.", -(global_event[k-1].announcement_time - ((turn - global_event[k-1].start_turn) / cfg.fps)) / 60);
				}
				strcpy(ppl, "\377WSubscribers: ");
				for (j = 0; j < MAX_GE_PARTICIPANTS; j++) {
					if (!global_event[k-1].participant[j]) continue;
					for (i = 1; i <= NumPlayers; i++) {
						if (global_event[k-1].participant[j] == Players[i]->id) {
							if (found) strcat(ppl, ", ");
							strcat(ppl, Players[i]->name);
							found = TRUE;
						}
					}
				}
				if (found) msg_format(Ind, "%s", ppl);
				msg_print(Ind, "\377d ");
			}
			return;
		}
		else if (prefix(message, "/gesign")) /* sign up for a global event */
		{
//			int i, notes = 0; /* not needed */
			if ((tk < 1) || (k < 1) || (k > MAX_GLOBAL_EVENTS)) {
				msg_format(Ind, "Usage: /gesign 1..%d", MAX_GLOBAL_EVENTS);
			} else if ((global_event[k-1].getype == GE_NONE) && (global_event[k-1].hidden==FALSE || admin)) {
				msg_print(Ind, "\377yThere is currently no running event of that number.");
			} else if (!global_event[k-1].announcement_time ||
				    (global_event[k-1].announcement_time - (turn - global_event[k-1].start_turn) / cfg.fps <= 0)) {
				msg_print(Ind, "\377yThat event has already started.");
			} else {
				global_event_signup(Ind, k-1);
			}
			return;
		}
		else if (prefix(message, "/bbs")) /* write a short text line to the server's in-game message board,
						    readable by all players via '!' key. For example to warn about
						    static (deadly) levels - C. Blue */
		{
			if (p_ptr->inval) {
				msg_print(Ind, "\377oYou must be validated to write to the in-game bbs.");
				return;
			}
			if (!strcmp(message3, "")) {
				msg_print(Ind, "Usage: /bbs <line of text for others to read>");
				return;
			}
			bbs_add_line(format("%s %s: %s",showtime() + 7, p_ptr->name, message3));
			return;
		}
		else if (prefix(message, "/time")) { /* show time / date */
			msg_format(Ind, "Current server time is %s", showtime());
			return;
		}
#ifdef AUCTION_SYSTEM
		else if (prefix(message, "/auc")) {
			int n;
			if (!p_ptr->wpos.wz && !is_admin(p_ptr))
			{
				if (p_ptr->wpos.wz < 0) msg_print(Ind, "\377B[@] \377rYou can't use the auction system while in a dungeon!");
				else msg_print(Ind, "\377B[@] \377rYou can't use the auction system while in a tower!");
			}
			else if ((tk < 1) || ((tk < 2) && (!strcmp("help", token[1]))))
			{
				msg_print(Ind, "\377B[@] \377wTomeNET Auction system");
				msg_print(Ind, "\377B[@] \377oUsage: /auction <subcommand> ...");
				msg_print(Ind, "\377B[@] \377GAvailable subcommands:");
				msg_print(Ind, "\377B[@] \377w  bid  buyout  cancel  examine  list");
				msg_print(Ind, "\377B[@] \377w  retrieve  search  show  set  start");
				msg_print(Ind, "\377B[@] \377GFor help about a specific subcommand:");
				msg_print(Ind, "\377B[@] \377o  /auction help <subcommand>");
			}
			else if (!strcmp("help", token[1]))
			{
				if (!strcmp("bid", token[2]))
				{
					msg_print(Ind, "\377B[@] \377oUsage: /auction bid <auction id> <bid>");
					msg_print(Ind, "\377B[@] \377wPlaces a bid on an item.");
					msg_print(Ind, "\377B[@] \377wYou must be able to pay your bid in order to win.");
					msg_print(Ind, "\377B[@] \377wYou also have to satisfy the level requirement when placing your bid.");
				}
				else if (!strcmp("buyout", token[2]))
				{
					msg_print(Ind, "\377B[@] \377oUsage: /auction buyout <auction id>");
					msg_print(Ind, "\377B[@] \377wInstantly buys an item.");
					msg_print(Ind, "\377B[@] \377wUse the \377Gretrieve \377wcommand to get the item.");
				}
				else if (!strcmp("cancel", token[2]))
				{
					msg_print(Ind, "\377B[@] \377oUsage: /auction cancel [auction id]");
					msg_print(Ind, "\377B[@] \377wCancels your bid on an item or aborts the auction on your item.");
				}
				else if (!strcmp("examine", token[2]))
				{
					msg_print(Ind, "\377B[@] \377oUsage: /auction examine <auction id>");
					msg_print(Ind, "\377B[@] \377wTells you everything about an item.");
				}
				else if (!strcmp("list", token[2]))
				{
					msg_print(Ind, "\377B[@] \377oUsage: /auction list");
					msg_print(Ind, "\377B[@] \377wShows a list of all items available.");
				}
				else if (!strcmp("retrieve", token[2]))
				{
					msg_print(Ind, "\377B[@] \377oUsage: /auction retrieve");
					msg_print(Ind, "\377B[@] \377wRetrieve won and cancelled items.");
				}
				else if (!strcmp("search", token[2]))
				{
					msg_print(Ind, "\377B[@] \377oUsage: /auction search <description>");
					msg_print(Ind, "\377B[@] \377wSearches for items with matching descriptions.");
				}
				else if (!strcmp("show", token[2]))
				{
					msg_print(Ind, "\377B[@] \377oUsage: /auction show <auction id>");
					msg_print(Ind, "\377B[@] \377wShows auction-related information about an item.");
				}
				else if (!strcmp("set", token[2]))
				{
					char *time_string;
					msg_print(Ind, "\377B[@] \377oUsage: /auction set <inventory slot> <starting price> <buyout price> <duration>");
					msg_print(Ind, "\377B[@] \377wSets up an auction.");
					msg_print(Ind, "\377B[@] \377wInventory slot is the item's letter in your inventory.");
#ifdef AUCTION_MINIMUM_STARTING_PRICE
					msg_format(Ind, "\377B[@] \377wMinimum starting price is %d%% of the item's real value.", AUCTION_MINIMUM_STARTING_PRICE);
#endif
#ifdef AUCTION_MAXIMUM_STARTING_PRICE
					msg_format(Ind, "\377B[@] \377wMaximum starting price is %d%% of the item's real value.", AUCTION_MAXIMUM_STARTING_PRICE);
#endif
#ifdef AUCTION_MINIMUM_BUYOUT_PRICE
					msg_format(Ind, "\377B[@] \377wMinimum buyout price is %d%% of the item's real value.", AUCTION_MINIMUM_BUYOUT_PRICE);
#endif
#ifdef AUCTION_MAXIMUM_BUYOUT_PRICE
					msg_format(Ind, "\377B[@] \377wMaximum buyout price is %d%% of the item's real value.", AUCTION_MAXIMUM_BUYOUT_PRICE);
#endif
#ifdef AUCTION_MINIMUM_DURATION
					time_string = auction_format_time(AUCTION_MINIMUM_DURATION);
					msg_format(Ind, "\377B[@] \377wShortest duration allowed is %d seconds.", time_string);
					C_KILL(time_string, strlen(time_string), char);
#endif
#ifdef AUCTION_MAXIMUM_DURATION
					time_string = auction_format_time(AUCTION_MINIMUM_DURATION);
					msg_format(Ind, "\377B[@] \377wLongest duration allowed is %d seconds.", AUCTION_MAXIMUM_DURATION);
					C_KILL(time_string, strlen(time_string), char);
#endif
				}
				else if (!strcmp("start", token[2]))
				{
					msg_print(Ind, "\377B[@] \377oUsage: /auction start");
					msg_print(Ind, "\377B[@] \377wConfirms that you want to start start an auction.");
				}
				else
				{
					msg_print(Ind, "\377B[@] \377oUsage: /auction help <subcommand>");
					msg_print(Ind, "\377B[@] \377ySee \"\377G/auction help\377y\" for list of valid subcommands.");
				}
			}
			else if (!strcmp("bid", token[1]))
			{
				if (tk < 3)
				{
					msg_print(Ind, "\377B[@] \377oUsage: /auction bid <auction id> <bid>");
				}
				else
				{
					k = atoi(token[2]);
					n = auction_place_bid(Ind, k, token[3]);
					if (n)
					{
						auction_print_error(Ind, n);
					}
				}
			}
			else if (!strcmp("buyout", token[1]))
			{
				if (tk < 2)
				{
					msg_print(Ind, "\377B[@] \377oUsage: /auction buyout <auction id>");
				}
				else
				{
					k = atoi(token[2]);
					n = auction_buyout(Ind, k);
					if (n)
					{
						auction_print_error(Ind, n);
					}
				}
			}
			else if (!strcmp("cancel", token[1]))
			{
				if (tk < 2)
				{
					if (p_ptr->current_auction)
					{
						auction_cancel(Ind, p_ptr->current_auction);
					}
					else
					{
						msg_print(Ind, "\377B[@] \377rNo item to cancel!");
						msg_print(Ind, "\377B[@] \377oUsage: /auction cancel [auction id]");
					}
				}
				else
				{
					k = atoi(token[2]);
					n = auction_cancel(Ind, k);
					if (n)
					{
						auction_print_error(Ind, n);
					}
				}
			}
			else if (!strcmp("examine", token[1]))
			{
				if (tk < 2)
				{
					msg_print(Ind, "\377B[@] \377oUsage: /auction examine <auction id>");
				}
				else
				{
					k = atoi(token[2]);
					n = auction_examine(Ind, k);
					if (n)
					{
						auction_print_error(Ind, n);
					}
				}
			}
			else if (!strcmp("list", token[1]))
			{
				auction_list(Ind);
			}
			else if (!strcmp("retrieve", token[1]))
			{
				int retrieved, unretrieved;
				auction_retrieve_items(Ind, &retrieved, &unretrieved);
				if (!unretrieved) msg_format(Ind, "\377B[@] \377wRetrieved %d item(s).", retrieved);
				else msg_format(Ind, "\377B[@] \377wRetrieved %d items, you didn't have room for %d more item(s).", retrieved, unretrieved);
			}
			else if (!strcmp("search", token[1]))
			{
				if (tk < 2)
				{
					msg_print(Ind, "\377B[@] \377oUsage: /auction search <name>");
				}
				else
				{
					auction_search(Ind, message3 + 7);
				}
			}
			else if (!strcmp("show", token[1]))
			{
				if (tk < 2)
				{
					msg_print(Ind, "\377B[@] \377oUsage: /auction show <auction id>");
				}
				else
				{
					k = atoi(token[2]);
					n = auction_show(Ind, k);
					if (n)
					{
						auction_print_error(Ind, n);
					}
				}
			}
			else if (!strcmp("set", token[1]))
			{
				if (tk < 5)
				{
					msg_print(Ind, "\377B[@] \377oUsage: /auction set <inventory slot> <starting price> <buyout price> <duration>");
				}
				else
				{
					n = auction_set(Ind, token[2][0], token[3], token[4], token[5]);
					if (n)
					{
						auction_print_error(Ind, n);
					}
				}
			}
			else if (!strcmp("start", token[1]))
			{
				if (!p_ptr->current_auction)
				{
					msg_print(Ind, "\377B[@] \377rNo item!");
				}
				else
				{
					n = auction_start(p_ptr->current_auction);
					if (n)
					{
						auction_print_error(Ind, n);
					}
				}
			}
			else
			{
				msg_print(Ind, "\377B[@] \377rUnknown subcommand!");
				msg_print(Ind, "\377B[@] \377ySee \"/auction help\" for list of valid subcommands.");
			}
			return;
		}
#endif
		/* workaround - refill ligth source (outdated clients cannot use 'F' due to INVEN_ order change */
		else if (prefix(message, "/lite"))
		{
			if (tk != 1) {
				msg_print(Ind, "Usage: /lite a...w");
				return;
			}
			k = message3[0] - 97;
			if (k < 0 || k >= INVEN_PACK) {
				msg_print(Ind, "Usage: /lite a...w");
				return;
			}
			do_cmd_refill(Ind, k);
			return;
		}


		/*
		 * Admin commands
		 *
		 * These commands should be replaced by LUA scripts in the future.
		 */
		else if (admin)
		{
			/* presume worldpos */
			switch (tk)
			{
				case 1:
					/* depth in feet */
					wp.wz = (p_ptr->depth_in_feet ? k / 50 : k);
					break;
				case 3:
					/* depth in feet */
					wp.wx = k % MAX_WILD_X;
					wp.wy = atoi(token[2]) % MAX_WILD_Y;
					wp.wz = atoi(token[3]) / (p_ptr->depth_in_feet ? 50 : 1);
					break;
				case 2:
					wp.wx = k % MAX_WILD_X;
					wp.wy = atoi(token[2]) % MAX_WILD_Y;
					wp.wz = 0;
					break;
				default:
					break;
			}

#ifdef TOMENET_WORLDS
			if (prefix(message, "/world")){
				world_connect(Ind);
				return;
			}
#endif

			if (prefix(message, "/shutdown") ||
					prefix(message, "/quit"))
			{
				bool kick = (cfg.runlevel == 1024);
				set_runlevel(tk ? k :
						((cfg.runlevel < 6 || kick)? 6 : 5));
				msg_format(Ind, "Runlevel set to %d", cfg.runlevel);

				/* Hack -- character edit mode */
				if (k == 1024 || kick)
				{
					if (k == 1024) msg_print(Ind, "\377rEntering edit mode!");
					else msg_print(Ind, "\377rLeaving edit mode!");

					for (i = NumPlayers; i > 0; i--)
					{
						/* Check connection first */
						if (Players[i]->conn == NOT_CONNECTED)
							continue;

						/* Check for death */
						if (!is_admin(Players[i]))
							Destroy_connection(Players[i]->conn, "kicked out");
					}
				}
				time(&cfg.closetime);
				return;
			}
			else if (prefix(message, "/shutempty")) {
				msg_print(Ind, "\377y* Shutting down as soon as dungeons are empty *");
				cfg.runlevel = 2048;
				return;
			}
			else if (prefix(message, "/shutlow")) {
				msg_print(Ind, "\377y* Shutting down as soon as dungeons are empty and few players are on *");
				cfg.runlevel = 2047;
				return;
			}
			else if (prefix(message, "/shutvlow")) {
				msg_print(Ind, "\377y* Shutting down as soon as dungeons are empty and very few players are on *");
				cfg.runlevel = 2046;
				return;
			}
			else if (prefix(message, "/shutnone")) {
				msg_print(Ind, "\377y* Shutting down as soon as no players are on anymore *");
				cfg.runlevel = 2045;
				return;
			}
#if 0	/* not implemented yet - /shutempty is currently working this way */
			else if (prefix(message, "/shutsurface")) {
				msg_print(Ind, "\377y* Shutting down as soon as noone is inside a dungeon/tower *");
				cfg.runlevel = 2050;
				return;
			}
#endif
			else if (prefix(message, "/val")){
				if(!tk) return;
				/* added checking for account existance - mikaelh */
				if (validate(message3)) {
					msg_format(Ind, "\377GValidating %s", message3);
				}
				else {
					msg_format(Ind, "\377rAccount %s not found", message3);
				}
					
/*				do{
					msg_format(Ind, "\377GValidating %s", token[tk]);
					validate(token[tk]);
				}while(--tk);
*/				return;
			}
			else if (prefix(message, "/ban"))
			{
				if (tk)
				{
					int j = name_lookup_loose(Ind, token[1], FALSE);
					if (j)
					{
						char kickmsg[80];
						/* Success maybe :) */
						add_banlist(j, (tk>1 ? atoi(token[2]) : 5));
						if (tk < 3)
						{
							msg_format(Ind, "Banning %s for %d minutes...", Players[j]->name, atoi(token[2]));
							snprintf(kickmsg, 80, "banned for %d minutes", atoi(token[2]));
							Destroy_connection(Players[j]->conn, kickmsg);
						} else {
							msg_format(Ind, "Banning %s for %d minutes (%s)...", Players[j]->name, atoi(token[2]), token[3]);
							snprintf(kickmsg, 80, "Banned for %d minutes (%s)", atoi(token[2]), token[3]);
							Destroy_connection(Players[j]->conn, kickmsg);
						}
						/* Kick him */
					}
					return;
				}

				msg_print(Ind, "\377oUsage: /ban (Player name) (time) [reason]");
				return;
			}
			else if (prefix(message, "/ipban"))
			{
				if (tk)
				{
					add_banlist_ip(token[1], (tk>1 ? atoi(token[2]) : 5));
					if (tk < 3)
					{
						msg_format(Ind, "Banning %s for %d minutes...", token[1], atoi(token[2]));
					} else {
						msg_format(Ind, "Banning %s for %d minutes (%s)...", token[1], atoi(token[2]), token[3]);
					}
					/* Kick him */
					return;
				}

				msg_print(Ind, "\377oUsage: /ipban (IP address) (time) [reason]");
				return;
			}
			else if (prefix(message, "/kick"))
			{
				if (tk)
				{
					int j = name_lookup_loose(Ind, token[1], FALSE);
					if (j)
					{
						char kickmsg[80];
						/* Success maybe :) */
						if (tk < 2)
						{
							msg_format(Ind, "Kicking %s out...", Players[j]->name);
							Destroy_connection(Players[j]->conn, "kicked out");
						} else {
							msg_format(Ind, "Kicking %s out (%s)...", Players[j]->name, token[2]);
							snprintf(kickmsg, 80, "kicked out (%s)", token[2]);
							Destroy_connection(Players[j]->conn, kickmsg);
						}
						/* Kick him */
					}
					return;
				}

				msg_print(Ind, "\377oUsage: /kick (Player name) [reason]");
				return;
			}
                        /* The idea is to reduce the age of the target player because s/he was being
                         * immature (and deny his/her chatting privilege). - the_sandman
			 */
                        else if (prefix(message, "/mute"))
                        {
                                if (tk)
                                {
                                        int j = name_lookup_loose(Ind, token[1], FALSE);
                                        if (j)
                                        {
                                                Players[j]->muted = TRUE;
						msg_print(j, "\377fYou have been muted.");
                                        }
                                        return;
                                }
                                msg_print(Ind, "\377oUsage: /mute (player name)");
                        }
                        else if (prefix(message, "/unmute"))    //oh no!
                        {
                                if (tk)
                                {
                                        int j = name_lookup_loose(Ind, token[1], FALSE);
                                        if (j)
                                        {
                                                Players[j]->muted = FALSE;
                                        }
                                        return;
                                }
                                msg_print(Ind, "\377oUsage: /unmute (player name)");
                        }
			/* erase items and monsters */
			else if (prefix(message, "/clear-level") ||
					prefix(message, "/clv"))
			{
				/* Wipe even if town/wilderness */
				wipe_o_list_safely(&wp);
				wipe_m_list(&wp);
				/* XXX trap clearance support dropped - reimplement! */
//				wipe_t_list(&wp);

				msg_format(Ind, "\377rItems/monsters on %s are cleared.", wpos_format(Ind, &wp));
				return;
			}
			/* erase items (prevent loot-mass-freeze) */
			else if (prefix(message, "/clear-items") ||
					prefix(message, "/cli"))
			{
				/* Wipe even if town/wilderness */
				wipe_o_list_safely(&wp);

				msg_format(Ind, "\377rItems on %s are cleared.", wpos_format(Ind, &wp));
				return;
			}
			/* erase ALL items (never use this when houses are on the map sector) */
			else if (prefix(message, "/clear-extra") ||
					prefix(message, "/xcli"))
			{
				/* Wipe even if town/wilderness */
				wipe_o_list(&wp);

				msg_format(Ind, "\377rItems on %s are cleared.", wpos_format(Ind, &wp));
				return;
			}
			else if(prefix(message, "/cp")){
				party_check(Ind);
				account_check(Ind);
				return;
			}
			else if (prefix(message, "/geno-level") ||
					prefix(message, "/geno"))
			{
				/* Wipe even if town/wilderness */
				wipe_m_list(&wp);

				msg_format(Ind, "\377rMonsters on %s are cleared.", wpos_format(Ind, &wp));
				return;
			}
			else if (prefix(message, "/mkill-level") ||
					prefix(message, "/mkill"))
			{
			        /* Kill all the monsters */
			        for (i = m_max - 1; i >= 1; i--)
			        {
			                monster_type *m_ptr = &m_list[i];
			                if (inarea(&m_ptr->wpos,&p_ptr->wpos))
			                        monster_death(Ind, i);
			        }
				wipe_m_list(&wp);
				msg_format(Ind, "\377rMonsters on %s were killed.", wpos_format(Ind, &wp));
				return;
			}
			else if (prefix(message, "/game") && tk>0){
				if(!strcmp(token[1], "rugby")){
					object_type	forge;
					object_type	*o_ptr = &forge;

					invcopy(o_ptr, lookup_kind(1, 9));
					o_ptr->number = 1;
					o_ptr->name1=0;
					o_ptr->name2=0;
					o_ptr->name3=0;
					o_ptr->pval=0;
					o_ptr->level = 1;
					(void)inven_carry(Ind, o_ptr);

					teamscore[0]=0;
					teamscore[1]=0;
					teams[0]=0;
					teams[1]=0;
					gametype=EEGAME_RUGBY;
					msg_broadcast(0, "\377pA new game of rugby has started!");
					for(k=1; k<=NumPlayers; k++){
						Players[k]->team=0;
					}
				}
				else if (!strcmp(token[1], "stop")) /* stop all games - mikaelh */
				{
					char sstr[80];
					msg_broadcast(0, "\377pThe game has been stopped!");
					snprintf(sstr, 80, "Score: \377RReds: %d  \377BBlues: %d", teamscore[0], teamscore[1]);
					msg_broadcast(0, sstr);
					teamscore[0] = 0;
					teamscore[1] = 0;
					teams[0] = 0;
					teams[1] = 0;
					gametype = 0;
					for (k = 1; k <= NumPlayers; k++)
					{
						Players[k]->team = 0;
					}
				}
				return;
			}
			else if (prefix(message, "/unstatic-level") ||
					prefix(message, "/unst"))
			{
				/* no sanity check, so be warned! */
				master_level_specific(Ind, &wp, "u");
				//				msg_format(Ind, "\377rItems and monsters on %dft is cleared.", k * 50);
				return;
			}
			else if (prefix(message, "/treset")){
				struct worldpos wpos;
				wpcopy(&wpos, &p_ptr->wpos);
				master_level_specific(Ind, &wp, "u");
				dealloc_dungeon_level(&wpos);
				return;
			}
			else if (prefix(message, "/static-level") ||
					prefix(message, "/stat"))
			{
				/* no sanity check, so be warned! */
				master_level_specific(Ind, &wp, "s");
				//				msg_format(Ind, "\377rItems and monsters on %dft is cleared.", k * 50);
				return;
			}
			/* TODO: make this player command (using spells, scrolls etc) */
			else if (prefix(message, "/identify") ||
					prefix(message, "/id"))
			{
				identify_pack(Ind);

				/* Combine the pack */
				p_ptr->notice |= (PN_COMBINE);

				/* Window stuff */
				p_ptr->window |= (PW_INVEN | PW_EQUIP);

				return;
			}
			else if (prefix(message, "/artifact") ||
					prefix(message, "/art"))
			{
				if (k)
				{
					if (a_info[k].cur_num)
					{
						a_info[k].cur_num = 0;
						a_info[k].known = FALSE;
						msg_format(Ind, "Artifact %d is now \377Gfindable\377w.", k);
					}
					else
					{
						a_info[k].cur_num = 1;
						a_info[k].known = TRUE;
						msg_format(Ind, "Artifact %d is now \377runfindable\377w.", k);
					}
				}
				else if (tk > 0 && prefix(token[1], "show"))
				{
					int count = 0;
					for (i = 0; i < MAX_A_IDX ; i++)
					{
						if (!a_info[i].cur_num || a_info[i].known) continue;

						a_info[i].known = TRUE;
						count++;
					}
					msg_format(Ind, "%d 'found but unknown' artifacts are set as '\377Gknown\377w'.", count);
				}
				else if (tk > 0 && prefix(token[1], "fix"))
				{
					int count = 0;
					for (i = 0; i < MAX_A_IDX ; i++)
					{
						if (!a_info[i].cur_num || a_info[i].known) continue;

						a_info[i].cur_num = 0;
						count++;
					}
					msg_format(Ind, "%d 'found but unknown' artifacts are set as '\377rfindable\377w'.", count);
				}
				else if (tk > 0 && prefix(token[1], "hack"))
				{
					int count = 0;
					for (i = 0; i < MAX_A_IDX ; i++)
					{
						if (!a_info[i].cur_num || !a_info[i].known) continue;

						a_info[i].known = TRUE;
						count++;
					}
					msg_format(Ind, "%d 'found but unknown' artifacts are set as '\377yknown\377w'.", count);
				}
//				else if (tk > 0 && strchr(token[1],'*'))
				else if (tk > 0 && prefix(token[1], "reset!"))
				{
					for (i = 0; i < MAX_A_IDX ; i++)
					{
						a_info[i].cur_num = 0;
						a_info[i].known = FALSE;
					}
					msg_format(Ind, "All the artifacts are \377rfindable\377w!", k);
				}
				else if (tk > 0 && prefix(token[1], "ban!"))
				{
					for (i = 0; i < MAX_A_IDX ; i++)
					{
						a_info[i].cur_num = 1;
						a_info[i].known = TRUE;
					}
					msg_format(Ind, "All the artifacts are \377runfindable\377w!", k);
				}
				else
				{
					msg_print(Ind, "Usage: /artifact (No. | (show | fix | reset! | ban!)");
				}
				return;
			}
			else if (prefix(message, "/unique"))
			{
				bool done = FALSE;
				monster_race *r_ptr;
				for (k = 1; k <= tk; k++)
				{
					if (prefix(token[k], "unseen"))
					{
						for (i = 0; i < MAX_R_IDX - 1 ; i++)
						{
							r_ptr = &r_info[i];
							if (!(r_ptr->flags1 & RF1_UNIQUE)) continue;

							r_ptr->r_sights = 0;
						}
						msg_print(Ind, "All the uniques are set as '\377onot seen\377'.");
						done = TRUE;
					}
					else if (prefix(token[k], "nonkill"))
					{
						monster_race *r_ptr;
						for (i = 0; i < MAX_R_IDX - 1 ; i++)
						{
							r_ptr = &r_info[i];
						if (!(r_ptr->flags1 & RF1_UNIQUE)) continue;

							r_ptr->r_tkills = r_ptr->r_pkills = 0;
						}
						msg_print(Ind, "All the uniques are set as '\377onever killed\377'.");
						done = TRUE;
					}
				}
				if (!done) msg_print(Ind, "Usage: /unique (unseen | nonkill)");
				return;
			}
			else if (prefix(message, "/uninum"))
			{
				if (k)
				{
					if (!(r_info[k].flags1 & RF1_UNIQUE)) return;
					if (r_info[k].max_num)
					{
						r_info[k].max_num = 0;
						msg_format(Ind, "Monster %d is now \377Gunfindable\377w.", k);
					}
					else
					{
						r_info[k].max_num = 1;
						msg_format(Ind, "Monster %d is now \377rfindable\377w.", k);
					}
				}
				else
				{
					msg_print(Ind, "Usage: /uninum <monster_index>");
				}
				return;
			}
			else if (prefix(message, "/unizero"))
			{
				if (k)
				{
					if (!(r_info[k].flags1 & RF1_UNIQUE)) return;
					r_info[k].r_tkills = r_info[k].r_pkills = 0;
					msg_format(Ind, "Monster %d kill count reset to \377G0\377w.", k);
				}
				else
				{
					msg_print(Ind, "Usage: /unizero <monster_index>");
				}
				return;
			}
			else if (prefix(message, "/reload-config") ||
					prefix(message, "/cfg"))
			{
				if (tk)
				{
					if (MANGBAND_CFG != NULL) string_free(MANGBAND_CFG);
					MANGBAND_CFG = string_make(token[1]);
				}

				//				msg_print(Ind, "Reloading server option(tomenet.cfg).");
				msg_format(Ind, "Reloading server option(%s).", MANGBAND_CFG);

				/* Reload the server preferences */
				load_server_cfg();
				return;
			}
			/* Admin wishing :)
			 * TODO: better parser like
			 * /wish 21 8 a117 d40
			 * for Aule {40% off}
			 */
#ifndef FUN_SERVER /* disabled while server is being exploited */
			else if (prefix(message, "/wish"))
			{
				object_type	forge;
				object_type	*o_ptr = &forge;
				WIPE(o_ptr, object_type);

				if (tk < 1 || !k)
				{
					msg_print(Ind, "\377oUsage: /wish (tval) (sval) (pval) [discount] [name] [name2b]  --or /wish (o_idx)");
					return;
				}

				invcopy(o_ptr, tk > 1 ? lookup_kind(k, atoi(token[2])) : k);

				/* Wish arts out! */
				if (tk > 4)
				{
					int nom = atoi(token[5]);
					o_ptr->number = 1;

					if (nom == ART_RANDART) { /* see defines.h */
						/* Piece together a 32-bit random seed */
						o_ptr->name1 = ART_RANDART;
						o_ptr->name3 = rand_int(0xFFFF) << 16;
						o_ptr->name3 += rand_int(0xFFFF);
					} else if (nom > 0) {
						o_ptr->name1 = nom;
						handle_art_inum(o_ptr->name1);
					} else if (nom < 0) {
						/* It's ego or randarts */
						o_ptr->name2 = 0 - nom;
						if (tk > 5) o_ptr->name2b = 0 - atoi(token[6]);
					}
				}
				else
				{
					o_ptr->number = o_ptr->weight > 100 ? 2 : 99;
				}

//				apply_magic(&p_ptr->wpos, o_ptr, -1, !o_ptr->name2, TRUE, TRUE, FALSE, TRUE);
				apply_magic(&p_ptr->wpos, o_ptr, -1, !o_ptr->name2, o_ptr->name1 || o_ptr->name2, o_ptr->name1 || o_ptr->name2, FALSE, TRUE);
				if (tk > 3){
					o_ptr->discount = atoi(token[4]);
				}
				else{
					o_ptr->discount = 100;
				}
				object_known(o_ptr);
				o_ptr->owner = 0;
				if(tk>2)
					o_ptr->pval=atoi(token[3]);
				//o_ptr->owner = p_ptr->id;
				o_ptr->level = 1;
				(void)inven_carry(Ind, o_ptr);

				return;
			}
#endif
			else if (prefix(message, "/trap") ||
					prefix(message, "/tr"))
			{
				if (k)
				{
					wiz_place_trap(Ind, k);
				}
				else
				{
					wiz_place_trap(Ind, TRAP_OF_FILLING);
				}
				return;
			}
			else if (prefix(message, "/enlight") ||
					prefix(message, "/en"))
			{
				wiz_lite(Ind);
				(void)detect_treasure(Ind, DEFAULT_RADIUS * 2);
				(void)detect_object(Ind, DEFAULT_RADIUS * 2);
				(void)detect_sdoor(Ind, DEFAULT_RADIUS * 2);
				(void)detect_trap(Ind, DEFAULT_RADIUS * 2);
				if (k)
				{
//					(void)detect_trap(Ind);
					identify_pack(Ind);
					self_knowledge(Ind);

					/* Combine the pack */
					p_ptr->notice |= (PN_COMBINE);

					/* Window stuff */
					p_ptr->window |= (PW_INVEN | PW_EQUIP);
				}

				return;
			}
			else if (prefix(message, "/equip") ||
					prefix(message, "/eq"))
			{
				if (tk) admin_outfit(Ind, k);
//				else admin_outfit(Ind, -1);
				else
				{
					msg_print(Ind, "usage: /eq (realm no.)");
					msg_print(Ind, "    Mage(0) Pray(1) sorc(2) fight(3) shad(4) hunt(5) psi(6) None(else)");
				}
				p_ptr->au = 50000000;
				p_ptr->skill_points = 9999;
				return;
			}
			else if (prefix(message, "/uncurse") ||
					prefix(message, "/unc"))
			{
				remove_all_curse(Ind);
				return;
			}
			/* do a wilderness cleanup */
			else if (prefix(message, "/purge")) 
			{
				msg_format(Ind, "previous server status: m_max(%d) o_max(%d)",
						m_max, o_max);
				compact_monsters(0, TRUE);
				compact_objects(0, TRUE);
//				compact_traps(0, TRUE);
				msg_format(Ind, "current server status: m_max(%d) o_max(%d)",
						m_max, o_max);

				return;
			}
			/* Refresh stores
			 * XXX very slow */
			else if (prefix(message, "/store"))
			{
				store_turnover();
				if (tk && prefix(token[1], "owner"))
				{
					for(i=0;i<numtowns;i++)
					{
//						for (k = 0; k < MAX_STORES - 2; k++)
						for (k = 0; k < max_st_idx; k++)
						{
							/* Shuffle a random shop (except home and auction house) */
							store_shuffle(&town[i].townstore[k]);
						}
					}
					msg_print(Ind, "\377oStore owners had been changed!");
				}
				else msg_print(Ind, "\377GStore inventory had been changed.");

				return;
			}
			/* Empty a store */
			else if (prefix(message, "/stnew"))
			{
				if (!k) {
					msg_print(Ind, "\377oUsage: /stnew <store#>");
					return;
				}
				for(i=0;i<numtowns;i++)
				{
					int what, num;
					object_type *o_ptr;
					store_type *st_ptr;

					st_ptr = &town[i].townstore[k];
					/* Pick a random slot */
					what = rand_int(st_ptr->stock_num);
					/* Determine how many items are here */
					o_ptr = &st_ptr->stock[what];
					num = o_ptr->number;

//					store_item_increase(st_ptr, what, -num);
//					store_item_optimize(st_ptr, what);
//					st_ptr->stock[what].num=0;
				}
				msg_print(Ind, "\377oStores were emptied!");
				return;
			}
			/* take 'cheezelog'
			 * result is output to the logfile */
			else if (prefix(message, "/cheeze")) 
			{
				char    path[MAX_PATH_LENGTH];
				object_type *o_ptr;
				for(i=0;i<o_max;i++)
				{
					o_ptr=&o_list[i];
					cheeze(o_ptr);
				}

				cheeze_trad_house();

				path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_DATA, "tomenet.log");
				do_cmd_check_other_prepare(Ind, path);
				return;
			}
			/* Respawn monsters on the floor
			 * TODO: specify worldpos to respawn */
			else if (prefix(message, "/respawn")) 
			{
				/* Set the monster generation depth */
				monster_level = getlevel(&p_ptr->wpos);
				if (p_ptr->wpos.wz)
					alloc_monster(&p_ptr->wpos, MAX_SIGHT + 5, FALSE);
				else wild_add_monster(&p_ptr->wpos);

				return;
			}
			else if (prefix(message, "/log_u"))
			{
				if (tk)
				{
					if(!strcmp(token[1], "on"))
					{
						msg_print(Ind, "log_u is now on");
						cfg.log_u = TRUE;
						return;
					}
					else if(!strcmp(token[1], "off"))
					{
						msg_print(Ind, "log_u is now off");
						cfg.log_u = FALSE;
						return;
					}
					else
					{
						msg_print(Ind, "valid parameters are 'on' or 'off'");
						return;
					}
				}
				if (cfg.log_u) msg_print(Ind, "log_u is on");
				else msg_print(Ind, "log_u is off");
				return;
			}

			else if (prefix(message, "/noarts"))
			{
				if (tk)
				{
					if(!strcmp(token[1], "on"))
					{
						msg_print(Ind, "artifact generation is now supressed");
						cfg.arts_disabled = TRUE;
						return;
					}
					else if(!strcmp(token[1], "off"))
					{
						msg_print(Ind, "artifact generation is now back to normal");
						cfg.arts_disabled = FALSE;
						return;
					}
					else
					{
						msg_print(Ind, "valid parameters are 'on' or 'off'");
						return;
					}
				}
				if (cfg.arts_disabled) msg_print(Ind, "artifact generation is currently supressed");
				else msg_print(Ind, "artifact generation is currently allowed");
				return;
			}
			else if (prefix(message, "/debug-town")){
				/* C. Blue's mad debug code to swap Minas Anor
				   and Khazad-Dum on the worldmap :) */
				struct town_type tmptown;
				tmptown.x = town[2].x;
				tmptown.y = town[2].y;
				town[2].x = town[4].x;
				town[2].y = town[4].y;
				town[4].x = tmptown.x;
				town[4].y = tmptown.y;
				
				return;
			}
			else if (prefix(message, "/debug-stair")){
				/* C. Blue's mad debug code to fix
				   stairs [for entering Mount Doom].. */
				int scx,scy;
				worldpos *tpos = &p_ptr->wpos;
				cave_type **zcave = getcave(tpos);
				if(!(zcave=getcave(tpos))) return;
				for(scx = 0; scx < 133; scx++) {
					for(scy = 0;scy < 66; scy++) {
						if(zcave[scy][scx].feat==FEAT_MORE) {
							zcave[scy][scx].feat=FEAT_FLOOR;
							zcave[p_ptr->py][p_ptr->px].feat=FEAT_MORE;
							new_level_up_y(tpos, p_ptr->py);
							new_level_up_x(tpos, p_ptr->px);
							return;
						}
					}
				}
				msg_print(Ind, "No staircase downwards found.");
				return;
			}
			else if (prefix(message, "/debug-dun")){
				/* C. Blue's mad debug code to change
				   dungeon flags [of Nether Realm] */
				int type, x, y;
#ifdef RPG_SERVER
				bool found_town = FALSE;
				int i;
#endif
				struct dungeon_type *d_ptr;
				struct worldpos *tpos = &p_ptr->wpos;
				wilderness_type *wild = &wild_info[tpos->wy][tpos->wx];

/* more mad code to change RPG_SERVER dungeon flags.. */
for(x=0;x<(tk?64:1);x++)
for(y=0;y<(tk?64:1);y++) {
if (!tk) {tpos=&p_ptr->wpos;} else {tpos->wx=x;tpos->wy=y;tpos->wz=0;}
wild=&wild_info[tpos->wy][tpos->wx];

				if ((d_ptr = wild->tower)) {
					type = d_ptr->type;

					d_ptr->flags1 = d_info[type].flags1;
					d_ptr->flags2 = d_info[type].flags2 | DF2_RANDOM;

#ifdef RPG_SERVER /* Make towers harder */
//					d_ptr->flags2 &= ~(DF2_IRON || DF2_IRONFIX1 || DF2_IRONFIX2 || DF2_IRONFIX3 || DF2_IRONFIX4 || 
//							    DF2_IRONRND1 || DF2_IRONRND2 || DF2_IRONRND3 || DF2_IRONRND4) ; /* Reset flags first */
//					if (!(d_info[type].flags1 & DF1_NO_UP))	d_ptr->flags1 &= ~DF1_NO_UP;
if (!(d_ptr->flags2 & DF2_NO_DEATH)) {
					found_town = FALSE;
				        for(i=0;i<numtowns;i++) {
	    	        			if(town[i].x==tpos->wx && town[i].y==tpos->wy) {
							found_town = TRUE;
							if (tpos->wx == 32 && tpos->wy == 32)
								d_ptr->flags2 |= DF2_IRON; /* Barrow-downs only */
							else
								d_ptr->flags2 |= DF2_IRON | DF2_IRONFIX2; /* Other towns */
						}
					}
					if (!found_town) {
						d_ptr->flags2 |= DF2_IRON | DF2_IRONRND1; /* Wilderness dungeons */
//						d_ptr->flags1 |= DF1_NO_UP; /* Wilderness dungeons */
					}
}
#endif
if(tk)					msg_print(Ind, "Tower flags updated.");
				}
				if ((d_ptr = wild->dungeon)) {
					type = d_ptr->type;

					d_ptr->flags1 = d_info[type].flags1;
					d_ptr->flags2 = d_info[type].flags2 | DF2_RANDOM;

#ifdef RPG_SERVER /* Make dungeons harder */
//					d_ptr->flags2 &= ~(DF2_IRON || DF2_IRONFIX1 || DF2_IRONFIX2 || DF2_IRONFIX3 || DF2_IRONFIX4 || 
//							    DF2_IRONRND1 || DF2_IRONRND2 || DF2_IRONRND3 || DF2_IRONRND4) ; /* Reset flags first */
//					if (!(d_info[type].flags1 & DF1_NO_UP))	d_ptr->flags1 &= ~DF1_NO_UP;
if (!(d_ptr->flags2 & DF2_NO_DEATH)) {
					found_town = FALSE;
				        for(i=0;i<numtowns;i++) {
	    	        			if(town[i].x==tpos->wx && town[i].y==tpos->wy) {
							found_town = TRUE;
							if (tpos->wx == 32 && tpos->wy == 32)
								d_ptr->flags2 |= DF2_IRON; /* Barrow-downs only */
							else
								d_ptr->flags2 |= DF2_IRON | DF2_IRONFIX2; /* Other towns */
						}
					}
					if (!found_town) {
						d_ptr->flags2 |= DF2_IRON | DF2_IRONRND1; /* Wilderness dungeons */
//						d_ptr->flags1 |= DF1_NO_UP; /* Wilderness dungeons */
					}
}
#endif
if(tk)					msg_print(Ind, "Dungeon flags updated.");
				}
}
if(!tk)					msg_print(Ind, "Dungeon/tower flags updated.");
				return;
			}
			else if (prefix(message, "/debug-pos")){
				/* C. Blue's mad debug code to change player @
				   startup positions in Bree (px, py) */
				new_level_rand_x(&p_ptr->wpos, atoi(token[1]));
				new_level_rand_y(&p_ptr->wpos, atoi(token[2]));
				msg_print(Ind, format("Set x=%d, y=%d for this wpos.",atoi(token[1]),atoi(token[2])));
				return;
			}
			else if (prefix(message, "/anotes"))
			{
				int i, notes = 0;
				for (i = 0; i < MAX_ADMINNOTES; i++) {
					/* search for pending notes of this player */
					if (strcmp(admin_note[i], "")) {
						/* found a matching note */
						notes++;
					}
				}
				if (notes > 0) msg_format(Ind, "\377oAdmins wrote %d currently pending notes:", notes);
				else msg_format(Ind, "\377oNo admin wrote any pending note.");
				for (i = 0; i < MAX_ADMINNOTES; i++) {
					/* search for pending admin notes */
					if (strcmp(admin_note[i], "")) {
						/* found a matching note */
						msg_format(Ind, "\377o(#%d)- %s", i, admin_note[i]);
					}
				}
				return;
			}
			else if (prefix(message, "/danote")) /* Delete a global admin note to everyone */
			{
				int i, notes = 0;
				if ((tk < 1) || (strlen(message2) < 8)) /* Explain command usage */
				{
					msg_print(Ind, "\377oUse /danote <message index> to delete a message.");
					msg_print(Ind, "\377oTo clear all pending notes of yours, type: /danote *");
					return;
				}
				if (message2[8] == '*') {
					for (i = 0; i < MAX_ADMINNOTES; i++)
						if (strcmp(admin_note[i], "")) {
							notes++;
							strcpy(admin_note[i], "");
						}
					msg_format(Ind, "\377oDeleted %d notes.", notes);
				} else {
					notes = atoi(message2 + 8);
					if ((notes > 0) && (notes < MAX_ADMINNOTES)) {
						strcpy(admin_note[notes], "");
						msg_format(Ind, "\377oDeleted note´%d.", notes);
					}
				}
				return;
			}
			else if (prefix(message, "/anote")) /* Send a global admin note to everyone */
			{
				int i, j = 0;
				if (tk < 1)	/* Explain command usage */
				{
					msg_print(Ind, "\377oUsage: /anote <text>");
					msg_print(Ind, "\377oUse /danote <message index> to delete a message.");
					msg_print(Ind, "\377oTo clear all pending notes of yours, type: /danote *");
					return;
				}
				/* Search for begin of parms ( == text of the note) */
				for (i = 6; i < strlen(message2); i++)
					if (message2[i] == ' ')
						for (j = i; j < strlen(message2); j++)
							if (message2[j] != ' ')	{
								/* save start pos in j for latter use */
								i = strlen(message2);
								break;
							}

				/* search for free admin note */
				for (i = 0; i < MAX_ADMINNOTES; i++) {
	    				if (!strcmp(admin_note[i], "")) {
						/* found a free memory spot */
						break;
					}
				}
				if (i < MAX_ADMINNOTES) {
					/* Add admin note */
					strcpy(admin_note[i], &message2[j]);
					msg_print(Ind, "\377yNote has been stored.");
				} else {
					msg_format(Ind, "\377oSorry, the server reached the maximum of %d pending admin notes.", MAX_ADMINNOTES);
				}
				return;
			}
			else if (prefix(message, "/broadcast-anotes")) /* Display all admin notes to all players NOW! :) */
			{
				int i;
				for (i = 0; i < MAX_ADMINNOTES; i++)
					if (strcmp(admin_note[i], ""))
						msg_broadcast(0, format("\377sGlobal Admin Note: %s", admin_note[i]));
				return;
			}
			else if (prefix(message, "/reart")) /* re-roll a random artifact */
			{
				object_type *o_ptr;
				if (tk < 1)
				{
					msg_print(Ind, "\377oUsage: /reart <inventory-slot>");
					return;
				}
				if (atoi(token[1]) < 1 || atoi(token[1]) > INVEN_TOTAL) {
					msg_print(Ind, "\377oInvalid inventory slot.");
					return;
				}
				o_ptr = &p_ptr->inventory[atoi(token[1])];
				if (o_ptr->name1 != ART_RANDART) {
					msg_print(Ind, "\377oNot a randart.");
					return;
				}

		                /* Piece together a 32-bit random seed */
		                o_ptr->name3 = rand_int(0xFFFF) << 16;
		                o_ptr->name3 += rand_int(0xFFFF);

		                /* Check the tval is allowed */
		                if (randart_make(o_ptr) == NULL)
		                {
		                        /* If not, wipe seed. No randart today */
		                        o_ptr->name1 = 0;
			                o_ptr->name3 = 0L;
					msg_print(Ind, "Randart creation failed..");
			                return;
			        }

			        o_ptr->timeout=0;
		    	        apply_magic(&p_ptr->wpos, o_ptr, p_ptr->lev, FALSE, FALSE, FALSE, FALSE, FALSE);

				msg_format(Ind, "Re-rolled randart in inventory slot %d!", atoi(token[1]));
				/* Window stuff */
				p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
				return;
			}
			/* very dangerous if player is poisoned, very weak, or has hp draining */
			else if (prefix(message, "/threaten") || prefix(message, "/thr")) { /* Nearly kill someone, as threat >:) */
				int j = name_lookup_loose(Ind, token[1], FALSE);
				if (!tk) {
					msg_print(Ind, "Usage: /threaten <player name>");
					return;
				}
				if (!j) return;
				bypass_invuln = TRUE;
			        take_hit(j, Players[j]->chp - 1, "", 0);
				bypass_invuln = FALSE;
			        msg_format_near(j, "\377y%s is hit by a bolt from the blue!", Players[j]->name);
			        msg_print(j, "\377rYou are hit by a bolt from the blue!");
			        msg_print(j, "\377rThat was close huh?!");
				return;
			}
			else if (prefix(message, "/slap")) { /* Slap someone around, as threat :-o */
				int j;
				if (!tk) {
					msg_print(Ind, "Usage: /slap <player name>");
					return;
				}
				j = name_lookup_loose(Ind, token[1], FALSE);
				if (!j) return;
				bypass_invuln = TRUE;
			        take_hit(j, Players[j]->chp / 2, "", 0);
				bypass_invuln = FALSE;
			        msg_print(j, "\377rYou are slapped by something invisible!");
			        msg_format_near(j, "\377y%s is slapped by something invisible!", Players[j]->name);
				return;
			}
			else if (prefix(message, "/pat")) { /* Counterpart to /slap :-p */
				int j;
				if (!tk) {
					msg_print(Ind, "Usage: /pat <player name>");
					return;
				}
				j = name_lookup_loose(Ind, token[1], FALSE);
				if (!j) return;
			        msg_print(j, "\377yYou are patted by something invisible!");
			        msg_format_near(j, "\377y%s is patted by something invisible!", Players[j]->name);
				return;
			}
			else if (prefix(message, "/hug")) { /* Counterpart to /slap :-p */
				int j;
				if (!tk) {
					msg_print(Ind, "Usage: /hug <player name>");
					return;
				}
				j = name_lookup_loose(Ind, token[1], FALSE);
				if (!j) return;
			        msg_print(j, "\377yYou are hugged by something invisible!");
			        msg_format_near(j, "\377y%s is hugged by something invisible!", Players[j]->name);
				return;
			}
			else if (prefix(message, "/deltown")){
				deltown(Ind);
				return;
			}
			else if (prefix(message, "/counthouses")) {
				if (!tk || (atoi(token[1]) < 1) || (atoi(token[1]) > NumPlayers)) {
					msg_print(Ind, "Usage: /counthouses <player-Index>");
					return;
				}
				lua_count_houses(atoi(token[1]));
				return;
			}
			/* fix insane hit dice of a golem manually - gotta solve the bug really */
			else if (prefix(message, "/mblowdice") || prefix(message, "/mbd")) {
				cave_type *c_ptr, **zcave = getcave(&p_ptr->wpos);
				monster_type *m_ptr;
				int x, y, i;
				if (tk < 2) {
					msg_print(Ind, "\377oUsage: /mblowdice <dice> <sides>");
					return;
				}
				y = p_ptr->py + ddy[p_ptr->last_dir];
				x = p_ptr->px + ddx[p_ptr->last_dir];
				c_ptr = &zcave[y][x];
				if (c_ptr->m_idx) {
					m_ptr = &m_list[c_ptr->m_idx];
					for (i = 0; i < 4; i++) {
						m_ptr->blow[i].d_dice = atoi(token[1]);
						m_ptr->blow[i].d_side = atoi(token[2]);
					}
				}
				return;
			}
			/* Umm, well I added this for testing purpose =) - C. Blue */
			else if (prefix(message, "/crash")) {
				msg_print(Ind, "\377RCRASHING");
				s_printf("$CRASHING$\n");
				s_printf("%d %s", "game over man", 666);
				return; /* ^^ */
			}
			/* Assign all houses of a <party> or <guild> to a <player> instead (chown) - C. BLue */
			else if (prefix(message, "/citychown")) {
				int i, c = 0;
#if 0
				int p; - after 'return': p = name_lookup_loose(Ind, token[2], FALSE);
				if (tk < 2) {
					msg_print(Ind, "\377oUsage: /citychown <party|guild> <player>");
					msg_print(Ind, "\377oExample: /citychown Housekeepers Janitor");
#else /* improved version */
				if (tk < 3) {
					msg_print(Ind, "\377oUsage: /citychown p|g <party-id|guild-id> <player-Index>");
					msg_print(Ind, "\377oExample: /citychown g 127 5");
#endif
					return;
				}
				msg_format(Ind, "Changing house owner of all %s to %s...", token[1], token[2]);
				for (i = 0; i < num_houses; i++) {
					struct dna_type *dna=houses[i].dna;
#if 0
                                        if (((dna->owner_type == OT_PARTY) && (!strcmp(parties[dna->owner].name, token[1]))) ||
					     ((dna->owner_type == OT_GUILD) && (!strcmp(guilds[dna->owner].name, token[1]))))
                                        if (((dna->owner_type == OT_PARTY) || (dna->owner_type == OT_GUILD)) &&
					    (dna->owner == atoi(token[1])))
					{
						dna->creator = Players[p]->dna;
						dna->owner = lookup_player_id_messy(token[2]);
						dna->owner_type = OT_PLAYER; /* Single player (code 1) is new owner */
						c++; /* :) */
					}
#else /* improved version: */
					if ((((token[1][0] == 'p') && (dna->owner_type == OT_PARTY)) ||
					    ((token[1][0] == 'g') && (dna->owner_type == OT_GUILD))) &&
					    (dna->owner == atoi(token[2])))
					{
						dna->creator = Players[atoi(token[3])]->dna;
						dna->owner = Players[atoi(token[3])]->id;
						dna->owner_type = OT_PLAYER; /* Single player (code 1) is new owner */
						c++; /* :) */
					}
#endif
				}
				msg_format(Ind, "%d houses have been changed.", c);
				lua_count_houses(atoi(token[3]));
				return;
			}
			/* This one is to fix houses which were changed by an outdated version of /citychown =p */
			else if (prefix(message, "/fixchown")) {
				int i, c = 0;
				int p;
				if (tk < 1) {
					msg_print(Ind, "\377oUsage: /fixchown <player>");
					return;
				}
				p = name_lookup_loose(Ind, token[1], FALSE);
				msg_format(Ind, "Fixing house owner %s...", token[1]);
				for (i = 0; i < num_houses; i++) {
					struct dna_type *dna=houses[i].dna;
//                                        if ((dna->owner_type == OT_PLAYER) && (!strcmp(lookup_player_name(dna->owner), token[1])))
                                        if ((dna->owner_type == OT_PLAYER) && (dna->owner == lookup_player_id_messy(token[1])))
					{
						dna->creator = Players[p]->dna;
						c++; /* :) */
					}
				}
				msg_format(Ind, "%d houses have been changed.", c);
				lua_count_houses(p);
				return;
			}
			/* Check house number */
			else if (prefix(message, "/listhouses")) {
				int i, cp = 0, cy = 0, cg = 0;
				if (tk < 1) {
					msg_print(Ind, "\377oUsage: /listhouses <owner-name>");
					return;
				}
				for (i = 0; i < num_houses; i++) {
					struct dna_type *dna=houses[i].dna;
					if (!dna->owner) ;
						/* not owned */
                                        else if ((dna->owner_type == OT_PLAYER) && (dna->owner == lookup_player_id(message2 + 12)))
						cp++;
                                        else if ((dna->owner_type == OT_PARTY) && (!strcmp(parties[dna->owner].name, message2 + 12)))
						cy++;
					else if ((dna->owner_type == OT_GUILD) && (!strcmp(guilds[dna->owner].name, message2 + 12)))
						cg++;
				}
				msg_format(Ind, "%s has houses: Player %d, Party %d, Guild %d.", message2 + 12, cp, cy, cg);
				return;
			}
			/* Reroll a player's birth hitdice to test major changes - C. Blue */
			else if (prefix(message, "/rollchar")) {
				int p;
				if (tk < 1) {
					msg_print(Ind, "\377oUsage: /rollchar <player name>");
					return;
				}
				p = name_lookup_loose(Ind, token[1], FALSE);
				if (!p) return;
				lua_recalc_char(p);
				msg_format(Ind, "Rerolled HP and recalculated exp ratio for %s.", token[1]);
				return;
			}
			/* Turn all non-everlasting items inside a house to everlasting items if the owner is everlasting */
			else if (prefix(message, "/everhouse")) {
				/* house_contents_chmod .. (scan_obj style) */
			}
			/* Blink a player */
			else if (prefix(message, "/blink")) {
				int p;
				if (tk < 1) {
					msg_print(Ind, "\377oUsage: /blink <player name>");
					return;
				}
				p = name_lookup_loose(Ind, token[1], FALSE);
				if (!p) return;
				teleport_player(p, 10);
				msg_print(Ind, "Phased that player.");
				return;
			}
			/* Blink a player */
			else if (prefix(message, "/tport")) {
				int p;
				if (tk < 1) {
					msg_print(Ind, "\377oUsage: /tport <player name>");
					return;
				}
				p = name_lookup_loose(Ind, token[1], FALSE);
				if (!p) return;
				teleport_player(p, 100);
				msg_print(Ind, "Teleported that player.");
				return;
			}
			/* STRIP ALL TRUE ARTIFACTS FROM ALL PLAYERS (!) */
			else if (prefix(message, "/strathash")) {
				msg_print(Ind, "Stripping all players.");
				lua_strip_true_arts_from_absent_players();
				return;
			}
			/* STRIP ALL TRUE ARTIFACTS FROM ALL FLOORS */
			else if (prefix(message, "/stratmap")) {
				msg_print(Ind, "Stripping all floors.");
				lua_strip_true_arts_from_floors();
				return;
			}
			/* STRIP ALL TRUE ARTIFACTS FROM A PLAYER */
			else if (prefix(message, "/strat")) {
				int p = name_lookup_loose(Ind, token[1], FALSE);
				if (!p) return;
				lua_strip_true_arts_from_present_player(Ind, 0);
				msg_format(Ind, "Stripped arts from player %s.", Players[Ind]->name);
				return;
			}
			/* wipe wilderness map of tournament players - mikaelh */
			else if (prefix(message, "/wipewild")) {
				if (!tk) {
					msg_print(Ind, "\377oUsage: /wipewild <player name>");
					return;
				}
				int p = name_lookup_loose(Ind, message3, FALSE);
				if (!p) return;
				for (i = 0; i < MAX_WILD_8; i++)
					Players[p]->wild_map[i] = 0;
				msg_format(Ind, "Wiped wilderness map of player %s.", Players[p]->name);
				return;
			}
			/* Find all true arts in o_list an tell where they are - mikaelh */
			else if (prefix(message, "/findarts")) {
				msg_print(Ind, "finding arts..");
				object_type *o_ptr;
				char o_name[160];
				for(i = 0; i < o_max; i++){
					o_ptr = &o_list[i];
					if (o_ptr->k_idx) {
						if (true_artifact_p(o_ptr))
						{
							object_desc(Ind, o_name, o_ptr, FALSE, 0);
							msg_format(Ind, "%s is at (%d, %d, %d) (x=%d,y=%d)", o_name, o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz, o_ptr->ix, o_ptr->iy);
						}
					}
				}
				msg_print(Ind, "done.");
				return;
			}
			else if (prefix(message, "/debug-store")){
				/* Debug store size - C. Blue */
				store_type *st_ptr;
		                store_info_type *sti_ptr;
				int i;
				if (tk < 1) {
					msg_print(Ind, "Usage: /debug-store <store#>");
					return;
				}
				i = atoi(token[1]);
				st_ptr = &town[0].townstore[i];
		                sti_ptr = &st_info[i];
				msg_format(Ind, "Store %d '%s' has size %d (expected %d).",
				    i, st_name + sti_ptr->name, st_ptr->stock_size, sti_ptr->max_obj);
				return;
			}
			else if (prefix(message, "/acclist")){ /* list all living characters of a specified account name - C. Blue */
				int *id_list, i, n;
				struct account *l_acc;
				byte tmpm;
				char colour_sequence[3];
				if (tk < 1) {
					msg_print(Ind, "Usage: /acclist <account name>");
					return;
				}
				msg_format(Ind, "Looking up account %s.", message3);
				l_acc = Admin_GetAccount(message3);
				if (l_acc) {
					n = player_id_list(&id_list, l_acc->id);
					/* Display all account characters here */
					for(i = 0; i < n; i++) {
//unused huh					u16b ptype = lookup_player_type(id_list[i]);
						/* do not change protocol here */
						tmpm = lookup_player_mode(id_list[i]);
						if (tmpm & MODE_IMMORTAL) strcpy(colour_sequence, "\377g");
						else if (tmpm & MODE_NO_GHOST) strcpy(colour_sequence, "\377D");
						else if (tmpm & MODE_HELL) strcpy(colour_sequence, "\377W");
						else strcpy(colour_sequence, "\377w");
						msg_format(Ind, "Character #%d: %s%s (%d) (ID: %d)", i+1, colour_sequence, lookup_player_name(id_list[i]), lookup_player_level(id_list[i]), id_list[i]);
					}
					if (n) C_KILL(id_list, n, int);
					KILL(l_acc, struct account);
				} else {
					msg_print(Ind, "Account not found.");
				}
				return;
			}
			else if (prefix(message, "/addnewdun")) {
				msg_print(Ind, "Trying to add new dungeons..");
				wild_add_new_dungeons();
				msg_print(Ind, "done.");
				return;
			}
			/* for now only loads Valinor */
			else if (prefix(message, "/loadmap")) {
				int xstart = 0, ystart = 0;
				if (!message2) {
					msg_print(Ind, "Usage: /loadmap t_<mapname>.txt");
					return;
				}
				msg_print(Ind, "Trying to load map..");
//                                process_dungeon_file(format("t_%s.txt", message3), &p_ptr->wpos, &ystart, &xstart, 20+1, 32+34, TRUE);
                                process_dungeon_file(format("t_%s.txt", message3), &p_ptr->wpos, &ystart, &xstart, MAX_HGT, MAX_WID, TRUE);

				msg_print(Ind, "done.");
				return;
			}
			/* check monster inventories for (nothing)s - mikaelh */
			else if (prefix(message, "/minvcheck"))
			{
				monster_type *m_ptr;
				object_type *o_ptr;
				int this_o_idx, next_o_idx;
				msg_print(Ind, "Checking monster inventories...");
				for (i = 1; i < m_max; i++)
				{
					m_ptr = &m_list[i];
					for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx) {
						o_ptr = &o_list[this_o_idx];

						if (!o_ptr->k_idx || o_ptr->k_idx == 1)
						{
							msg_format(Ind, "Monster #%d is holding an invalid item (#%d) (k_idx=%d)!", i, m_ptr->hold_o_idx, o_ptr->k_idx);
						}

						if (o_ptr->held_m_idx != i)
						{
							msg_format(Ind, "Item #%d has wrong held_m_idx! (is %d, should be %d)", this_o_idx, i);
						}

						next_o_idx = o_ptr->next_o_idx;
					}
				}
				msg_print(Ind, "Check complete.");
				return;
			}
			/* erase a certain player character file */
			else if (prefix(message, "/erasechar"))
			{
				if (tk < 1) {
					msg_print(Ind, "Usage: /erasechar <character name>");
					return;
				}
				msg_format(Ind, "Erasing character %s.", message3);
				erase_player_name(message3);
				return;
			}
			/* list of players about to expire - mikaelh */
			else if (prefix(message, "/checkexpir"))
			{
				int days;
				if (tk < 1) {
					msg_print(Ind, "Usage: /checkexpiry <number of days>");
					return;
				}
				days = atoi(token[1]);
				checkexpiry(Ind, days);
				return;
			}
			/* start a predefined global_event (quest, highlander tournament..),
			   see process_events() in xtra1.c for details - C. Blue */
			else if (prefix(message, "/gestart"))
			{
				char message4[80];
				int err, msgpos = 0;
				if (tk < 1) {
					msg_print(Ind, "Usage: /gestart <predefined type> [parameters...]");
					return;
				}
				while(message3[msgpos] && message3[msgpos] != 32) msgpos++;
				if (message3[msgpos] && message3[++msgpos]) strcpy(message4, message3 + msgpos);
				else strcpy(message4, "");
				err = start_global_event(Ind, atoi(token[1]), message4);
				if (err) msg_print(Ind, "Error: no more global events.");
				return;
			}
			else if (prefix(message, "/gestop"))
			{
				if (tk < 1 || k < 1 || k > MAX_GLOBAL_EVENTS) {
					msg_format(Ind, "Usage: /gestop 1..%d", MAX_GLOBAL_EVENTS);
					return;
				}
				if (global_event[k-1].getype == GE_NONE) {
					msg_format(Ind, "no such event.");
					return;
				}
				stop_global_event(Ind, k-1);
				return;
			}
			else if (prefix(message, "/gepause"))
			{
				if (tk < 1 || k < 1 || k > MAX_GLOBAL_EVENTS) {
					msg_format(Ind, "Usage: /gepause 1..%d", MAX_GLOBAL_EVENTS);
					return;
				}
				if (global_event[k-1].getype == GE_NONE) {
					msg_format(Ind, "no such event.");
					return;
				}
				if (global_event[k-1].paused == FALSE) {
					global_event[k-1].paused = TRUE;
					msg_format(Ind, "Global event #%d of type %d is now paused.", k, global_event[k-1].getype);
				} else {
					global_event[k-1].paused = FALSE;
					msg_format(Ind, "Global event #%d of type %d has been resumed.", k, global_event[k-1].getype);
				}
				return;
			}
			else if (prefix(message, "/geretime")) /* skip the announcements, start NOW */
			{
				int t = 60;
				if (tk < 1 || k < 1 || k > MAX_GLOBAL_EVENTS) {
					msg_format(Ind, "Usage: /geretime 1..%d [<new T-x>]", MAX_GLOBAL_EVENTS);
					return;
				}
				if (global_event[k-1].getype == GE_NONE) {
					msg_format(Ind, "no such event.");
					return;
				}
				if (tk == 2) t = atoi(token[2]);
				if ((turn - global_event[k-1].start_turn) / cfg.fps < global_event[k-1].announcement_time) {
					global_event[k-1].announcement_time = (turn - global_event[k-1].start_turn) / cfg.fps + t;
					announce_global_event(k-1);
				}
				return;
			}
			else if (prefix(message, "/partydebug"))
			{
				FILE *fp;
				fp = fopen("tomenet_parties", "w");
				if (!fp) {
					msg_print(Ind, "\377rError! Couldn't open tomenet_parties");
					return;
				}
				for (i = 1; i < MAX_PARTIES; i++) {
					fprintf(fp, "Party: %s Owner: %s Members: %d Created: %d\n", parties[i].name, parties[i].owner, (int)parties[i].members, (int)parties[i].created);
				}
				fclose(fp);
				msg_print(Ind, "Party data dumped to tomenet_parties");
				return;
			}
			else if (prefix(message, "/partyclean")) /* reset the creation times of empty parties - THIS MUST BE RUN WHEN THE TURN COUNTER IS RESET - mikaelh */
			{
				for (i = 1; i < MAX_PARTIES; i++) {
					if (parties[i].members == 0) parties[i].created = 0;
				}
				msg_print(Ind, "Creation times of empty parties reseted!");
				return;
			}
			else if (prefix(message, "/meta"))
			{
				if (!strcmp(message3, "update")) {
					msg_print(Ind, "Sending updated info to the metaserver");
					Report_to_meta(META_UPDATE);
				}
				else if (!strcmp(message3, "die")) {
					msg_print(Ind, "Reporting to the metaserver that we are dead");
					Report_to_meta(META_DIE);
				}
				else if (!strcmp(message3, "start")) {
					msg_print(Ind, "Starting to report to the metaserver");
					Report_to_meta(META_START);
				}
				else {
					msg_print(Ind, "Usage: /meta <update|die|start>");
				}
				return;
			}
			/*
			 * remove an entry from the high score file
			 * required for restored chars that were lost to bugs - C. Blue :/
			*/
#if 0 //not working since actual erasing is missing
			else if (prefix(message, "/hiscorerm"))
			{
				int                     i, slot;
				bool            done = FALSE;
				high_score              the_score, tmpscore;

				if (tk < 1 || k < 1 || k > MAX_HISCORES) {
					msg_format(Ind, "Usage: /hiscorerm 1..%d", MAX_HISCORES);
					return;
				}
		    		/* Paranoia -- it may not have opened */
				if (highscore_fd < 0) return (-1);

				/* Hack -- prepare to dump the new score */
				the_score = (*score);

				for (i = k; !done && (i < MAX_HISCORES); i++) {
					/* Read the old guy, note errors */
					if (highscore_seek(i + 1)) return;
					if (highscore_read(&tmpscore)) done = TRUE;

					/* Back up and dump the score we were holding */
					if (highscore_seek(i)) return;
					if (highscore_write(&the_score)) return;
				}
				/* Erase the last slot */
				//.......
			}
#endif
		        else if (prefix(message, "/rem")) {     /* write a remark (comment) to log file, for bookmarking - C. Blue */
	            		char *rem = "-";
				if (tk) rem = message3;
				s_printf("%s ADMIN_REMARK by %s: %s\n", showtime(), p_ptr->name, rem);
				return;
			}
			else if (prefix(message, "/mcarry")) { /* give a designated item to the monster currently looked at (NOT the one targetted) - C. Blue */
#ifdef MONSTER_INVENTORY
				s16b o_idx, m_idx;
				object_type *o_ptr;
				monster_type *m_ptr;
				if (!tk) {
					msg_print(Ind, "No inventory slot specified.");
					return; /* no inventory slot specified */
				}
				if (!p_ptr->inventory[k].tval) {
					msg_print(Ind, "Specified inventory slot is empty.");
					return; /* inventory slot empty */
				}
				if (p_ptr->health_who <= 0) {//target_who
					msg_print(Ind, "No monster looked at.");
					return; /* no monster targetted */
				}
				m_idx = p_ptr->health_who;
				m_ptr = &m_list[m_idx];
				o_ptr = &p_ptr->inventory[k];
				o_idx = o_pop();
				if (o_idx) {
					object_type *j_ptr;
					j_ptr = &o_list[o_idx];
					object_copy(j_ptr, o_ptr);
					j_ptr->owner = 0;
					j_ptr->owner_mode = 0;
					j_ptr->held_m_idx = m_idx;
					j_ptr->next_o_idx = m_ptr->hold_o_idx;
					m_ptr->hold_o_idx = o_idx;
					inven_item_increase(Ind, k, -j_ptr->number);
					inven_item_optimize(Ind, k);
					msg_print(Ind, "Monster-carry successfully completed.");
				} else {
					msg_print(Ind, "No more objects available.");
				}
#else
				msg_print(Ind, "MONSTER_INVENTORY not defined.");
#endif  // MONSTER_INVENTORY
				return;
			}
			else if (prefix(message, "/erasehashtableid")) { /* erase a player id in case there's a duplicate entry in the hash table - mikaelh */
				int id;
				if (tk < 1) {
					msg_print(Ind, "Usage: /erasehashtableid <player id>");
					return;
				}
				id = atoi(message3);
				if (!id) {
					msg_print(Ind, "Invalid ID");
					return;
				}
                                msg_format(Ind, "Erasing player id %d from hash table.", id);
                                delete_player_id(id);
				return;
			}
			/* check o_list for invalid items - mikaelh */
			else if (prefix(message, "/nothingcheck")) {
				object_type *o_ptr;
				int i;
				msg_print(Ind, "Check o_list for invalid items...");
				for (i = 0; i < o_max; i++) {
					o_ptr = &o_list[i];

					if (!o_ptr->k_idx || o_ptr->k_idx == 1) {
						if (o_ptr->held_m_idx) {
							msg_format(Ind, "Invalid item #%d (k_idx=%d) held by monster %d", i, o_ptr->k_idx, o_ptr->held_m_idx);
						}
						else {
							msg_format(Ind, "Invalid item #%d (k_idx=%d) found at (%d,%d,%d) (x=%d,y=%d)", i, o_ptr->k_idx, o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz, o_ptr->ix, o_ptr->iy);
						}
					}
				}
				msg_print(Ind, "Check complete.");
				return;
			}
			/* check for anomalous items somewhere - mikaelh */
			else if (prefix(message, "/floorcheck")) {
				struct worldpos wpos;
				cave_type **zcave, *c_ptr;
				object_type *o_ptr;
				int y, x, o_idx;
				if (tk > 1) {
					wpos.wx = atoi(token[1]);
					wpos.wy = atoi(token[2]);
					if (tk > 2) wpos.wz = atoi(token[3]);
					else wpos.wz = 0;
				}
				else {
					wpcopy(&wpos, &p_ptr->wpos);
				}
				msg_format(Ind, "Checking (%d,%d,%d)...", wpos.wx, wpos.wy, wpos.wz);
				zcave = getcave(&wpos);
				if (!zcave) {
					msg_print(Ind, "Couldn't getcave().");
					return;
				}
				for (y = 1; y < MAX_HGT - 1; y++) {
					for (x = 1; x < MAX_WID - 1; x++) {
						c_ptr = &zcave[y][x];
						o_idx = c_ptr->o_idx;
						if (o_idx) {
							if (o_idx > o_max) {
								msg_format(Ind, "Non-existent object #%d found at (x=%d,y=%d)", c_ptr->o_idx, x, y);
								continue;
							}
							o_ptr = &o_list[o_idx];
							/* k_idx = 1 is something weird... */
							if (!o_ptr->k_idx || o_ptr->k_idx == 1) {
								msg_format(Ind, "Invalid item #%d (k_idx=%d) found at (x=%d,y=%d)", o_idx, o_ptr->k_idx, x, y);
							}
							if (o_ptr->wpos.wx != wpos.wx || o_ptr->wpos.wy != wpos.wy || o_ptr->wpos.wz != wpos.wz) {
								msg_format(Ind, "WORMHOLE! Found item #%d that should be at (%d, %d, %d) (x=%d, y=%d)", o_idx, o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz, o_ptr->ix, o_ptr->iy);
								msg_format(Ind, "Wormhole located at (%d, %d, %d) (x=%d, y=%d)", wpos.wx, wpos.wy, wpos.wz, x, y);
							}
							/* more objects on this grid? */
							while (o_ptr->next_o_idx) {
								o_idx = o_ptr->next_o_idx;
								if (o_idx > o_max) {
									msg_format(Ind, "Non-existent object #%d found under a pile at (x=%d,y=%d)", c_ptr->o_idx, x, y);
									break;
								}
								o_ptr = &o_list[o_idx];
								if (!o_ptr->k_idx || o_ptr->k_idx == 1) {
									msg_format(Ind, "Invalid item #%d (k_idx=%d) found under a pile at (x=%d,y=%d)", o_idx, o_ptr->k_idx, x, y);
								}
								if (o_ptr->wpos.wx != wpos.wx || o_ptr->wpos.wy != wpos.wy || o_ptr->wpos.wz != wpos.wz) {
									msg_format(Ind, "WORMHOLE! Found item #%d that should be at (%d, %d, %d) (x=%d, y=%d)", o_idx, o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz, o_ptr->ix, o_ptr->iy);
									msg_format(Ind, "Wormhole located at (%d, %d, %d) (x=%d, y=%d)", wpos.wx, wpos.wy, wpos.wz, x, y);
								}
							}
						}
					}
				}
				msg_print(Ind, "Check complete.");
				return;
			}
			/* delete a line from bbs */
			else if (prefix(message, "/dbbs")) {
				if (tk != 1) {
					msg_print(Ind, "Usage: /dbbs <line number>");
					return;
				}
				bbs_del_line(k);
				return;
			}
			/* erase all bbs lines */
			else if (prefix(message, "/ebbs")) {
				bbs_erase();
				return;
			}
			else if (prefix(message, "/reward")) { /* for testing purpose - C. Blue */
				int j;
				object_type reward_forge;
				object_type *o_ptr = &reward_forge;

				if (!tk) {
					msg_print(Ind, "Usage: /reward <player name>");
					return;
				}
				j = name_lookup_loose(Ind, token[1], FALSE);
				if (!j) return;
			        msg_print(j, "\377GYou have been rewarded by the gods!");

//				create_reward(Ind, o_ptr, 1, 100, TRUE, TRUE, make_resf(Players[j]) | RESF_NOHIDSM, 5000);
                                create_reward(Ind, o_ptr, 95, 95, TRUE, TRUE, RESF_LOW, 5000);
                                object_aware(Ind, o_ptr);
                                object_known(o_ptr);
                                o_ptr->discount = 100;
                                o_ptr->ident |= ID_MENTAL;
                                inven_carry(Ind, o_ptr);
				return;
			}
			else if (prefix(message, "/debug1")) { /* debug an issue at hand */
				int j;
				for (j = INVEN_TOTAL; j >= 0; j--)
	                                if (p_ptr->inventory[j].tval == TV_AMULET && p_ptr->inventory[j].sval == SV_AMULET_HIGHLANDS)
    		                                invcopy(&p_ptr->inventory[j], lookup_kind(TV_AMULET, SV_AMULET_HIGHLANDS2));
				p_ptr->update |= (PU_BONUS | PU_VIEW);
				handle_stuff(Ind);
				msg_print(Ind, "debug1");
				return;
			}
			else if (prefix(message, "/debug2")) { /* debug an issue at hand */
				int j;
				for (j = INVEN_TOTAL - 1; j >= 0; j--)
	                                if (p_ptr->inventory[j].tval == TV_AMULET && p_ptr->inventory[j].sval == SV_AMULET_HIGHLANDS) {
    		                                invcopy(&p_ptr->inventory[j], lookup_kind(TV_AMULET, SV_AMULET_HIGHLANDS2));
    		                                p_ptr->inventory[j].number = 1;
    		                                p_ptr->inventory[j].level = 0;
    		                                p_ptr->inventory[j].discount = 0;
    		                                p_ptr->inventory[j].ident |= ID_MENTAL;
    		                                p_ptr->inventory[j].owner = p_ptr->id;
    		                                p_ptr->inventory[j].owner_mode = p_ptr->mode;
    		                                object_aware(Ind, &p_ptr->inventory[j]);
    		                                object_known(&p_ptr->inventory[j]);
					}
				p_ptr->update |= (PU_BONUS | PU_VIEW);
				p_ptr->window |= (PW_INVEN | PW_EQUIP);
				handle_stuff(Ind);
				msg_print(Ind, "debug2");
				return;
			}
			else if (prefix(message, "/daynight")) { /* switch between day and night - use carefully! */
				int t = turn % ((10L * DAY) / 2);
				turn += ((10L * DAY) / 2) - t - 1;
				return;
			}
			else if (prefix(message, "/weather")) { /* toggle snowfall during WINTER_SEASON */
				if (weather == 1) weather = 0;
				else weather = 1;
				return;
			}
			else if (prefix(message, "/fireworks")) { /* toggle fireworks during NEW_YEARS_EVE */
				if (fireworks == 1) fireworks = 0;
				else fireworks = 1;
				return;
			}
		}
	}

	do_slash_brief_help(Ind);
	return;
}
#endif

static void do_slash_brief_help(int Ind){
	player_type *p_ptr = Players[Ind];

	msg_print(Ind, "Commands: afk at bed broadcast bug cast dis dress ex feel fill help ignore me");	// pet ?
	msg_print(Ind, "          pk quest rec ref rfe shout sip tag target untag;");

	if (is_admin(p_ptr))
	{
		msg_print(Ind, "  art cfg cheeze clv cp en eq geno id kick lua purge respawn shutdown");
		msg_print(Ind, "  sta store trap unc unst wish");
	}
	else
	{
		msg_print(Ind, "  /dis \377rdestroys \377wall the uninscribed items in your inventory!");
	}
	msg_print(Ind, "  please type '/help' for detailed help.");
}


