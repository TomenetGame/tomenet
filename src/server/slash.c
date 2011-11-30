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

/* how many chars someone may enter (used for /bbs) */
#define MAX_SLASH_LINE_LEN	140

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
		msg_format_near(Ind, "\377%c%^s shouts:%s", COLOUR_CHAT, p_ptr->name, (char*)string);
	else
		msg_format_near(Ind, "\377%cYou hear %s shout!", COLOUR_CHAT, p_ptr->name);
	msg_format(Ind, "\377%cYou shout %s", COLOUR_CHAT, (char*)string);
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
			if (nom) {
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
	if(tk > 2)
		o_ptr->pval = atoi(token[3]);
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
	p_ptr->redraw |= PR_MAP | PR_EXTRA | PR_HISTORY | PR_VARIOUS | PR_STATE;
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
	int i = 0, j = 0, h = 0;
	int k = 0, tk = 0;
	player_type *p_ptr = Players[Ind];
 	char *colon, *token[9], message2[MAX_SLASH_LINE_LEN], message3[MAX_SLASH_LINE_LEN];/* was [80], [80] */
	char message4[MAX_SLASH_LINE_LEN];

	worldpos wp;
#ifndef TEST_SERVER
	bool admin = is_admin(p_ptr);
#else
	bool admin = TRUE;
#endif

	/* prevent overflow - bad hack for now (needed as you can now enter lines MUCH longer than 140 chars) */
	message[MAX_SLASH_LINE_LEN - 1] = 0;

	strcpy(message2, message);
	wpcopy(&wp, &p_ptr->wpos);

	/* message3 contains all tokens but not the command: */
	strcpy(message3, "");
	for (i = 0; i < (int)strlen(message); i++)
		if (message[i] == ' ') {
			strcpy(message3, message + i + 1);
			break;
		}

	/* Look for a player's name followed by a colon */
	colon = strchr(message, ' ');

	/* hack -- non-token ones first */
	if ((prefix(message, "/script") ||
//			prefix(message, "/scr") || used for /scream now
			prefix(message, "/ ") ||	// use with care!
			prefix(message, "//") ||	// use with care!
			prefix(message, "/lua")) && admin)
	{
		if (colon) {
			master_script_exec(Ind, colon);
		} else {
			msg_print(Ind, "\377oUsage: /lua (LUA script command)");
		}
		return;
	}
	else if ((prefix(message, "/rfe")) ||
		prefix(message, "/cookie"))
	{
		if (colon) {
			rfe_printf("RFE [%s]%s\n", p_ptr->name, colon);
			msg_print(Ind, "\377GThank you for sending us a message!");
		} else {
			msg_print(Ind, "\377oUsage: /rfe <message>");
		}
		return;
	}
	else if (prefix(message, "/bug")) {
		if (colon) {
			rfe_printf("BUG [%s]%s\n", p_ptr->name, colon);
			msg_print(Ind, "\377GThank you for sending us a message!");
		} else {
			msg_print(Ind, "\377oUsage: /bug <message>");
		}
		return;
	}
	/* Oops conflict; took 'never duplicate' principal */
	else if (prefix(message, "/cough") ||
			prefix(message, "/cou"))
	{
		break_cloaking(Ind, 4);
		msg_format_near(Ind, "\377%c%^s coughs noisily.", COLOUR_CHAT, p_ptr->name);
		msg_format(Ind, "\377%cYou cough noisily..", COLOUR_CHAT);
		wakeup_monsters_somewhat(Ind, 1);
		return;
	}
	else if (prefix(message, "/shout") ||
			prefix(message, "/sho"))
	{
		break_cloaking(Ind, 4);
		if (colon) {
			msg_format_near(Ind, "\377%c%^s shouts:%s", COLOUR_CHAT, p_ptr->name, colon);
			msg_format(Ind, "\377%cYou shout:%s", COLOUR_CHAT, colon);
		} else {
			msg_format_near(Ind, "\377%cYou hear %s shout!", COLOUR_CHAT, p_ptr->name);
			msg_format(Ind, "\377%cYou shout!", COLOUR_CHAT);
		}
		wakeup_monsters(Ind, 1);
		return;
	}
	else if (prefix(message, "/scream") ||
			prefix(message, "/scr"))
	{
		break_cloaking(Ind, 6);
		if (colon) {
			msg_format_near(Ind, "\377%c%^s screams:%s", COLOUR_CHAT, p_ptr->name, colon);
			msg_format(Ind, "\377%cYou scream:%s", COLOUR_CHAT, colon);
		} else {
			msg_format_near(Ind, "\377%cYou hear %s scream!", COLOUR_CHAT, p_ptr->name);
			msg_format(Ind, "\377%cYou scream!", COLOUR_CHAT);
		}
		aggravate_monsters(Ind, 1);
		return;
	}
	/* RPG-style talking to people who are nearby, instead of global chat. - C. Blue */
	else if (prefix(message, "/say"))
	{
		if (colon) {
			msg_format_near(Ind, "\377%c%^s says:%s", COLOUR_CHAT, p_ptr->name, colon);
			msg_format(Ind, "\377%cYou say:%s", COLOUR_CHAT, colon);
		} else {
			msg_format_near(Ind, "\377%c%s clears %s throat.", COLOUR_CHAT, p_ptr->name, p_ptr->male ? "his" : "her");
			msg_format(Ind, "\377%cYou clear your throat.", COLOUR_CHAT);
		}
		return;
// :)		break_cloaking(Ind, 3);
	}
	else if (prefix(message, "/whisper"))
	{
		if (colon) {
			msg_format_verynear(Ind, "\377%c%^s whispers:%s", COLOUR_CHAT, p_ptr->name, colon);
			msg_format(Ind, "\377%cYou whisper:%s", COLOUR_CHAT, colon);
		} else {
			msg_print(Ind, "What do you want to whisper?");
		}
		return;
// :)		break_cloaking(Ind, 3);
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
//spam?			s_printf("(%s) SLASH_PAGE: %s:%s.\n", showtime(), p_ptr->name, message3);
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
				msg_print(Ind, "\377yThat player is currently ignoring you.");
				return;
			}
#endif
			if (Players[p]->paging) {
				/* already paging, I hope this can prevent floods too - mikaelh */
				return;
			}
			if (!check_ignore(p, Ind)) Players[p]->paging = 3; /* Play 3 beeps quickly */
			msg_format(Ind, "\376\377yPaged %s.", Players[p]->name);
			msg_format(p, "\376\377y%s is paging you.", Players[Ind]->name);
			return;
		}

		/* Semi-auto item destroyer */
		else if ((prefix(message, "/dispose")) ||
				prefix(message, "/dis"))
		{
			object_type		*o_ptr;
			u32b f1, f2, f3, f4, f5, esp;
			bool nontag = FALSE, baseonly = FALSE;

			disturb(Ind, 1, 0);

			/* only tagged ones? */
			if (tk > 0) {
				if (prefix(token[1], "a")) {
					nontag = TRUE;
				} else if (prefix(token[1], "b")) {
					nontag = baseonly = TRUE;
				} else {
					msg_print(Ind, "\377oUsage: /dis [a|b]");
					return;
				}
			}

			for(i = 0; i < INVEN_PACK; i++) {
				bool resist = FALSE;
				o_ptr = &(p_ptr->inventory[i]);
				if (!o_ptr->tval) break;

				object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

#if 1 /* check for: tag _equals_ pseudo id tag */
				/* skip items inscribed with more than a single non-greatpseudo-ID tag */
				if (o_ptr->note &&
				    strcmp(quark_str(o_ptr->note), "terrible") &&
				    strcmp(quark_str(o_ptr->note), "cursed") &&
				    strcmp(quark_str(o_ptr->note), "uncursed") &&
				    strcmp(quark_str(o_ptr->note), "broken") &&
				    strcmp(quark_str(o_ptr->note), "average") &&
				    strcmp(quark_str(o_ptr->note), "good") &&
//				    strcmp(quark_str(o_ptr->note), "excellent"))
				    strcmp(quark_str(o_ptr->note), "worthless"))
					continue;
#else /* check for: tag _contains_  pseudo id tag */
				if (o_ptr->note &&
				    !strstr(quark_str(o_ptr->note), "terrible") &&
				    !strstr(quark_str(o_ptr->note), "cursed") &&
				    !strstr(quark_str(o_ptr->note), "uncursed") &&
				    !strstr(quark_str(o_ptr->note), "broken") &&
				    !strstr(quark_str(o_ptr->note), "average") &&
				    !strstr(quark_str(o_ptr->note), "good") &&
//				    !strstr(quark_str(o_ptr->note), "excellent"))
				    !strstr(quark_str(o_ptr->note), "worthless"))
					continue;

				/* skip items that are tagged as unkillable */
				if (check_guard_inscription(o_ptr->note, 'k')) continue;
				/* skip items that seem to be tagged as being in use */
				if (strchr(quark_str(o_ptr->note), '@')) continue;
#endif

				/* destroy base items (non-egos)? */
				if (baseonly && object_known_p(Ind, o_ptr) &&
				    (o_ptr->name1 || o_ptr->name2 || o_ptr->name2b ||
				    /* let exploding ammo count as ego.. pft */
				    (is_ammo(o_ptr->tval) && o_ptr->pval)))
					continue;

				/* destroy non-inscribed items too? */
				if (!nontag && !o_ptr->note &&
				    /* Handle {cursed} */
				    !(cursed_p(o_ptr) &&
				    (object_known_p(Ind, o_ptr) ||
				    (o_ptr->ident & ID_SENSE))))
					continue;

				/* Player might wish to identify it first */
				if (k_info[o_ptr->k_idx].has_flavor &&
				    !p_ptr->obj_aware[o_ptr->k_idx])
					continue;

				/* Hrm, this cannot be destroyed */
				if (((f4 & TR4_CURSE_NO_DROP) && cursed_p(o_ptr)) ||
				    like_artifact_p(o_ptr))
					resist = TRUE;
#if 0 /* too easy! */		
				/* Hack -- filter by value */
				if (k && (!object_known_p(Ind, o_ptr) ||
				    object_value_real(Ind, o_ptr) > k))
					continue;
#endif

				/* Avoid being somewhat spammy, since arts can't be destroyed */
				if (artifact_p(o_ptr)) continue;

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
			object_type *o_ptr;
			
			if (tk && (token[1][0] != '*')) {
				h = (token[1][0]) - 'a';
				j = h;
				if (h < 0 || h > 22 || token[1][1]) {
					msg_print(Ind, "\377oUsage: /tag [a..w|* [<inscription>]]");
					return;
				}
			} else {
				h = 0;
				j = INVEN_PACK - 1;
			}

			for(i = h; i <= j; i++) {
				o_ptr = &(p_ptr->inventory[i]);
				if (!o_ptr->tval) break;

				/* skip inscribed items */
				if (o_ptr->note) continue;

				o_ptr->note = quark_add(tk < 2 ? "!k" : token[2]);
			}
			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP);

			return;
		}
		/* remove specific inscription.
		   If '*' is given, all pseudo-id tags are removed,
		   if no parameter is given, '!k' is the default. */
		else if (prefix(message, "/untag"))
		{
			object_type *o_ptr;
//			cptr ax = token[1] ? token[1] : "!k";
			cptr ax = tk ? message3 : "!k";
			char note2[80], noteid[10];
			bool remove_all = !strcmp(ax, "*");
			bool remove_pseudo = !strcmp(ax, "p");
			bool remove_unique = !strcmp(ax, "u");

			for (i = 0; i < (remove_pseudo || remove_unique ? INVEN_TOTAL : INVEN_PACK); i++) {
				o_ptr = &(p_ptr->inventory[i]);

				/* Skip empty slots */
				if (!o_ptr->tval) continue;

				/* skip uninscribed items */
				if (!o_ptr->note) continue;

				/* remove all inscriptions? */
				if (remove_all) {
					o_ptr->note = 0;
					o_ptr->note_utag = 0;
					continue;
				}

				if (remove_unique && o_ptr->note_utag) {
					j = strlen(quark_str(o_ptr->note)) - o_ptr->note_utag;
					if (j >= 0) { /* bugfix hack */
//s_printf("j: %d, strlen: %d, note_utag: %d, i: %d.\n", j, strlen(quark_str(o_ptr->note)), o_ptr->note_utag, i);
						strncpy(note2, quark_str(o_ptr->note), j);
						if (j > 0 && note2[j - 1] == '-') j--; /* absorb '-' orphaned spacers */
						note2[j] = 0; /* terminate string */
						o_ptr->note_utag = 0;
						if (note2[0]) o_ptr->note = quark_add(note2);
						else o_ptr->note = 0;
					} else o_ptr->note_utag = 0; //paranoia?
					continue;
				}

				/* just remove pseudo-id tags? */
				if (remove_pseudo) {
					/* prevent 'empty' inscriptions from being erased by this */
					if ((quark_str(o_ptr->note))[0] == '\0') continue;

					note_crop_pseudoid(note2, noteid, quark_str(o_ptr->note));
					if (!note2[0]) {
						o_ptr->note = 0;
						o_ptr->note_utag = 0; //paranoia
					} else {
						o_ptr->note = quark_add(note2);
					}
					continue;
				}

				/* ignore pseudo-id inscriptions */
				note_crop_pseudoid(note2, noteid, quark_str(o_ptr->note));

				/* skip non-matching tags */
				if (strcmp(note2, ax)) continue;

				if (!noteid[0]) {
					/* tag removed, no more inscription */
					o_ptr->note = 0;
					o_ptr->note_utag = 0; //in case tag == unique name
				} else {
					/* tag removed, keeping pseudo-id inscription */
					o_ptr->note = quark_add(noteid);
					o_ptr->note_utag = 0; //in case tag == unique name
				}
			}

			/* Combine the pack */
			p_ptr->notice |= (PN_COMBINE);

			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP);

			return;
		}
#if 0 /* new '/cast' version below this one - C. Blue */
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

				if (whichplayer == Ind) {
					msg_print(Ind,"You feel lonely.");
				/* Ignore "unreasonable" players */
				} else if (!target_able(Ind, 0 - whichplayer)) {
					msg_print(Ind,"\377oThat player is out of your sight.");
					return;
				} else {
//					msg_format(Ind,"Book = %ld, Spell = %ld, PlayerName = %s, PlayerID = %ld",book,whichspell,token[3],whichplayer); 
					target_set_friendly(Ind,5,whichplayer);
					whichspell += 64;
				}
			} else if (ami) {
				target_set_friendly(Ind, 5);
				whichspell += 64;
			} else {
				target_set(Ind, 5);
			}

			switch (p_ptr->pclass)
			{
			}

//			msg_format(Ind,"Book = %ld, Spell = %ld, PlayerName = %s, PlayerID = %ld",book,whichspell,token[3],whichplayer); 
 #endif
                        return;
		}
#endif
#if 0
		/* cast a spell by name, instead of book/position */
		else if (prefix(message, "/cast"))
		{
			for (i = 0; i < 100; i++) {
				if (!strncmp(p_ptr->spell_name[i], message3, strlen(message3))) {
					cast_school_spell(Ind, p_ptr->spell_book[i], p_ptr->spell_pos[i], dir, item, aux);
					break;
				}
			}
                        return;
		}
#endif
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
			if (!(p_ptr->mode & (MODE_EVERLASTING | MODE_PVP | MODE_NO_GHOST)))
				msg_format(Ind, "You have %d %s left.", p_ptr->lives-1-1, p_ptr->lives-1-1 > 1 ? "resurrections" : "resurrection");

#if 0 /* already displayed to the left */
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
#endif
			if (get_skill(p_ptr, SKILL_AURA_FEAR)) check_aura(Ind, 0); /* MAX_AURAS */
			if (get_skill(p_ptr, SKILL_AURA_SHIVER)) check_aura(Ind, 1);
			if (get_skill(p_ptr, SKILL_AURA_DEATH)) check_aura(Ind, 2);

//			do_cmd_knowledge_dungeons(Ind);
			if (p_ptr->depth_in_feet) msg_format(Ind, "The deepest point you've reached: \377G-%d\377wft", p_ptr->max_dlv * 50);
			else msg_format(Ind, "The deepest point you've reached: Lev \377G-%d", p_ptr->max_dlv);
			

			msg_format(Ind, "You can move %d.%d times each turn.",
					extract_energy[p_ptr->pspeed] / 10,
					extract_energy[p_ptr->pspeed]
					- (extract_energy[p_ptr->pspeed] / 10) * 10);

			if (get_skill(p_ptr, SKILL_DODGE)) use_ability_blade(Ind);

#if 0 /* this is already displayed to the left */
			/* Insanity warning (better message needed!) */
			if (p_ptr->csane < p_ptr->msane / 8)
				msg_print(Ind, "\377rYou can hardly resist the temptation to cry out!");
			else if (p_ptr->csane < p_ptr->msane / 4)
				msg_print(Ind, "\377yYou feel insanity about to grasp your mind..");
			else if (p_ptr->csane < p_ptr->msane / 2)
				msg_print(Ind, "\377yYou feel insanity creep into your mind..");
			else
				msg_print(Ind, "\377wYou are sane.");
#endif

#if 0 /* deprecated, new one below.. */
			if (p_ptr->body_monster)
			{
				monster_race *r_ptr = &r_info[p_ptr->body_monster];
				msg_format(Ind, "You %shave a head.", r_ptr->body_parts[BODY_HEAD] ? "" : "don't ");
				msg_format(Ind, "You %shave arms.", r_ptr->body_parts[BODY_ARMS] ? "" : "don't ");
				msg_format(Ind, "You can %s use weapons.", r_ptr->body_parts[BODY_WEAPON] ? "" : "not");
				msg_format(Ind, "You can %s wear %s.", r_ptr->body_parts[BODY_FINGER] ? "" : "not", r_ptr->body_parts[BODY_FINGER] == 1 ? "a ring" : "rings");
				msg_format(Ind, "You %shave a torso.", r_ptr->body_parts[BODY_TORSO] ? "" : "don't ");
				msg_format(Ind, "You %shave legs/suitable feet for shoes.", r_ptr->body_parts[BODY_LEGS] ? "" : "don't ");
			} else if (p_ptr->fruit_bat) {
				msg_print(Ind, "You have a head.");
				msg_print(Ind, "You can wear rings.");
				msg_print(Ind, "You don't have a torso, but you can wear cloaks.");
			}
#endif
#if 0 /* another one.. */
			bool i_ringr = TRUE, i_ringl = TRUE, i_neck = TRUE, i_head = TRUE, i_outer = TRUE;
			bool i_light = TRUE, i_arms = TRUE, i_tool = TRUE, i_wield = TRUE, i_bow = TRUE;
			bool i_ammo = TRUE, i_hands = TRUE, i_feet = TRUE, i_body = TRUE;

			if (p_ptr->fruit_bat) {
				i_wield = i_bow = i_ammo = i_hands i_feet = i_body = FALSE;
			}
			if (p_ptr->body_monster) {
				if (!r_ptr->body_parts[BODY_WEAPON]) i_wield = i_bow = FALSE;
				if (r_ptr->body_parts[BODY_FINGER] <= 1) i_ringl = FALSE;
				if (!r_ptr->body_parts[BODY_FINGER]) i_ringr = FALSE;
				if (!r_ptr->body_parts[BODY_HEAD]) i_neck = i_head = FALSE;
				if (!r_ptr->body_parts[BODY_WEAPON] &&
				    !r_ptr->body_parts[BODY_FINGER] &&
				    !r_ptr->body_parts[BODY_HEAD] &&
				    !r_ptr->body_parts[BODY_ARMS])
					i_light = FALSE;
				if (!r_ptr->body_parts[BODY_TORSO]) i_body = i_outer = i_ammo = FALSE;
				if (!r_ptr->body_parts[BODY_ARMS]) i_arms = FALSE;
				if (!r_ptr->body_parts[BODY_WEAPON] &&
				    !r_ptr->body_parts[BODY_ARMS])
					i_tool = FALSE;
				if (!r_ptr->body_parts[BODY_FINGER] &&
				    !r_ptr->body_parts[BODY_ARMS])
					i_hands = FALSE;
				if (!r_ptr->body_parts[BODY_LEGS]) i_feet = FALSE;
			}
#endif
#if 1 /* just use item_tester_hook_wear() to prevent duplicate stuff.. */
			if (p_ptr->body_monster &&
			    p_ptr->pclass != CLASS_DRUID && p_ptr->prace != RACE_VAMPIRE &&
			    (p_ptr->pclass != CLASS_SHAMAN || !strchr("EG", r_info[p_ptr->body_monster].d_char))) {
				msg_print(Ind, "In your current form...");
				if (item_tester_hook_wear(Ind, INVEN_WIELD)) msg_print(Ind, "  you are able to wield a weapon.");
				else msg_print(Ind, "  you cannot wield weapons.");
				if (item_tester_hook_wear(Ind, INVEN_ARM)) msg_print(Ind, "  you are able to wield a shield.");
				else msg_print(Ind, "  you cannot wield shields.");
				if (item_tester_hook_wear(Ind, INVEN_BOW)) msg_print(Ind, "  you are able to wield a ranged weapon.");
				else msg_print(Ind, "  you cannot wield ranged weapons.");
				if (item_tester_hook_wear(Ind, INVEN_LEFT)) msg_print(Ind, "  you are able to wear rings.");
				else if (item_tester_hook_wear(Ind, INVEN_RIGHT)) msg_print(Ind, "  you are able to wear a ring.");
				else msg_print(Ind, "  you cannot wear rings.");
				if (item_tester_hook_wear(Ind, INVEN_NECK)) msg_print(Ind, "  you are able to wear an amulet.");
				else msg_print(Ind, "  you cannot wear amulets.");
				if (item_tester_hook_wear(Ind, INVEN_LITE)) msg_print(Ind, "  you are able to wield a light source.");
				else msg_print(Ind, "  you cannot wield light sources.");
				if (item_tester_hook_wear(Ind, INVEN_BODY)) msg_print(Ind, "  you are able to wear body armour.");
				else msg_print(Ind, "  you cannot wear body armour.");
				if (item_tester_hook_wear(Ind, INVEN_OUTER)) msg_print(Ind, "  you are able to wear a cloak.");
				else msg_print(Ind, "  you cannot wear cloaks.");
				if (item_tester_hook_wear(Ind, INVEN_HEAD)) msg_print(Ind, "  you are able to wear head gear.");
				else msg_print(Ind, "  you cannot wear head gear.");
				if (item_tester_hook_wear(Ind, INVEN_HANDS)) msg_print(Ind, "  you are able to wear gloves.");
				else msg_print(Ind, "  you cannot wear gloves.");
				if (item_tester_hook_wear(Ind, INVEN_FEET)) msg_print(Ind, "  you are able to wear boots.");
				else msg_print(Ind, "  you cannot wear boots.");
				if (item_tester_hook_wear(Ind, INVEN_AMMO)) msg_print(Ind, "  you are able to carry ammunition.");
				else msg_print(Ind, "  you cannot carry ammunition.");
				if (item_tester_hook_wear(Ind, INVEN_TOOL)) msg_print(Ind, "  you are able to use tools.");
				else msg_print(Ind, "  you cannot use tools.");
			} else if (p_ptr->fruit_bat) {
				msg_print(Ind, "As a fruit bat..");
				if (!item_tester_hook_wear(Ind, INVEN_WIELD)) msg_print(Ind, "  you cannot wield weapons.");
				if (!item_tester_hook_wear(Ind, INVEN_ARM)) msg_print(Ind, "  you cannot wield shields.");
				if (!item_tester_hook_wear(Ind, INVEN_BOW)) msg_print(Ind, "  you cannot wield ranged weapons.");
				if (!item_tester_hook_wear(Ind, INVEN_RIGHT)) msg_print(Ind, "  you cannot wear rings.");
				if (!item_tester_hook_wear(Ind, INVEN_NECK)) msg_print(Ind, "  you cannot wear amulets.");
				if (!item_tester_hook_wear(Ind, INVEN_LITE)) msg_print(Ind, "  you cannot wield light sources.");
				if (!item_tester_hook_wear(Ind, INVEN_BODY)) msg_print(Ind, "  you cannot wear body armour.");
				if (!item_tester_hook_wear(Ind, INVEN_OUTER)) msg_print(Ind, "  you cannot wear cloaks.");
				if (!item_tester_hook_wear(Ind, INVEN_HEAD)) msg_print(Ind, "  you cannot wear head gear.");
				if (!item_tester_hook_wear(Ind, INVEN_HANDS)) msg_print(Ind, "  you cannot wear gloves.");
				if (!item_tester_hook_wear(Ind, INVEN_FEET)) msg_print(Ind, "  you cannot wear boots.");
				if (!item_tester_hook_wear(Ind, INVEN_AMMO)) msg_print(Ind, "  you cannot carry ammunition.");
				if (!item_tester_hook_wear(Ind, INVEN_TOOL)) msg_print(Ind, "  you cannot use tools.");
			}
#endif

			if (admin) {
				cave_type **zcave;
				cave_type *c_ptr;

				msg_format(Ind, "your sanity: %d/%d", p_ptr->csane, p_ptr->msane);
				msg_format(Ind, "server status: m_max(%d) o_max(%d)",
						m_max, o_max);

				msg_print(Ind, "Colour test - \377ddark \377wwhite \377sslate \377oorange \377rred \377ggreen \377bblue \377uumber");
				msg_print(Ind, "\377Dl_dark \377Wl_white \377vviolet \377yyellow \377Rl_red \377Gl_green \377Bl_blue \377Ul_umber");
				if (!(zcave = getcave(&wp))) {
					msg_print(Ind, "\377rOops, the cave's not allocated!!");
					return;
				}
				c_ptr = &zcave[p_ptr->py][p_ptr->px];
				msg_format(Ind, "(x:%d y:%d) info:%d feat:%d o_idx:%d m_idx:%d effect:%d",
						p_ptr->px, p_ptr->py,
						c_ptr->info, c_ptr->feat, c_ptr->o_idx, c_ptr->m_idx, c_ptr->effect);

				switch (cfg.runlevel) {
				case 2048: msg_print(Ind, "\377y* Empty-server-shutdown command pending *"); break;
				case 2047: msg_print(Ind, "\377y* Low-server-shutdown command pending *"); break;
				case 2046: msg_print(Ind, "\377y* VeryLow-server-shutdown command pending *");; break;
				case 2045: msg_print(Ind, "\377y* None-server-shutdown command pending *"); break;
				case 2044: msg_print(Ind, "\377y* ActiveVeryLow-server-shutdown command pending *"); break;
				case 2043:
//					msg_print(NumPlayers, "\377y* Recall-server-shutdown command pending *");
					if (shutdown_recall_timer >= 120)
						msg_format(Ind, "\374\377I*** \377RServer-shutdown in max %d minutes (auto-recall). \377I***", shutdown_recall_timer / 60);
					else
						msg_format(Ind, "\374\377I*** \377RServer-shutdown in max %d seconds (auto-recall). \377I***", shutdown_recall_timer);
					break;
				}
			}

			int lev = p_ptr->lev;

			if (p_ptr->pclass == CLASS_DRUID) { /* compare mimic_druid in defines.h */
				if (lev >= 5) msg_print(Ind, "\377GYou know how to change into a Cave Bear (#160) and Panther (#198)");
				if (lev >= 10) msg_print(Ind, "\377GYou know how to change into a Grizzly Bear (#191) and Yeti (#154)");
				if (lev >= 15) msg_print(Ind, "\377GYou know how to change into a Griffon (#279) and Sasquatch (#343)");
				if (lev >= 20) msg_print(Ind, "\377GYou know how to change into a Werebear (#414), Great Eagle (#335), Aranea (#963) and Great White Shark (#898)");
				if (lev >= 25) msg_print(Ind, "\377GYou know how to change into a Wyvern (#334) and Multi-hued Hound (#513)");
				if (lev >= 30) msg_print(Ind, "\377GYou know how to change into a 5-h-Hydra (#440), Minotaur (#641) and Giant Squid (#482)");
				if (lev >= 35) msg_print(Ind, "\377GYou know how to change into a 7-h-Hydra (#614), Elder Aranea (#964) and Plasma Hound (#726)");
				if (lev >= 40) msg_print(Ind, "\377GYou know how to change into an 11-h-Hydra (#688), Giant Roc (#640) and Lesser Kraken (740)");
				if (lev >= 45) msg_print(Ind, "\377GYou know how to change into a Maulotaur (#723) and Winged Horror (#704)");// and Behemoth (#716)");
				if (lev >= 50) msg_print(Ind, "\377GYou know how to change into a Spectral tyrannosaur (#705), Jabberwock (#778) and Greater Kraken (#775)");// and Leviathan (#782)");
				if (lev >= 55) msg_print(Ind, "\377GYou know how to change into a Horned Serpent (#1131)");
				if (lev >= 60) msg_print(Ind, "\377GYou know how to change into a Firebird (#1127)");
			}
			
			if (p_ptr->prace == RACE_VAMPIRE) {
				if (lev >= 20) msg_print(Ind, "\377GYou are able to turn into a vampire bat (#391).");
			}
#ifndef ENABLE_RCRAFT
			if (p_ptr->pclass == CLASS_RUNEMASTER) {
				msg_format(Ind, "\377BYour rune mastery rating is %d.", RUNE_DMG);
#ifdef ALTERNATE_DMG
				if (get_skill_scale(p_ptr, SKILL_RUNEMASTERY, 50) < 25) {
					msg_print(Ind, "\377yYou do not use the full SP mentioned here.");
					msg_print(Ind, "\377yYou do not do the full damage mentioned here.");
				}
#endif
				msg_print(Ind,  "\377B|  Type  |   Damage   | Cost (base, medium, adv)");
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

#ifdef ALLOW_PERFECT_RUNE_CASTING
				if (lev >= 15) msg_print(Ind, "\377BYou have perfect bolt casting.");
				if (lev >= 25) msg_print(Ind, "\377BYou have perfect beam casting.");
				if (lev >= 35) msg_print(Ind, "\377BYou have perfect ball casting.");
				if (lev >= 50) msg_print(Ind, "\377BYou have perfect cloud casting."); //omg wtf woot ^_^"
#else
				msg_print(Ind, "\377BFailure rate: 0.1%");
#endif
				lev = p_ptr->lev; //restore ^^"
			}
#endif //ENABLE_RCRAFT
			/* display PvP kills */
			if (p_ptr->kills) msg_format(Ind, "\377rYou have defeated %d opponents.", p_ptr->kills);

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
			if (admin) set_recall_timer(Ind, 1);
			else {
				int item = -1, spell = -1, spell_rec = -1, spell_rel = -1;
				bool spell_rec_found = FALSE, spell_rel_found = FALSE;
				object_type *o_ptr;

				/* Paralyzed or just not enough energy left to perform a move? */

				/* this also prevents recalling while resting, too harsh maybe */
//				if (p_ptr->energy < level_speed(&p_ptr->wpos)) return;
				if (p_ptr->paralyzed) return;

				/* Don't drain energy far below zero - mikaelh */
				if (p_ptr->energy < 0) return;
				/* hm, how about this? - C. Blue */
				if (p_ptr->energy < level_speed(&p_ptr->wpos)) return;

				/* Test for 'Recall' istar spell and for 'Relocation' astral spell */
#if 0 /* hm, which version might be easier/better?.. */
				spell_rec = exec_lua(Ind, "return find_spell(\"Recall\")"); 
 #ifdef ENABLE_DIVINE
				spell_rel = exec_lua(Ind, "return find_spell(\"Relocation\")");
 #endif
#else
				spell_rec = exec_lua(Ind, "return RECALL");
 #ifdef ENABLE_DIVINE
				spell_rel = exec_lua(Ind, "return RELOCATION");
 #endif
#endif

				/* Turn off resting mode */
				disturb(Ind, 0, 0);

//				for (i = 0; i < INVEN_PACK; i++)
				for (i = 0; i < INVEN_TOTAL; i++) { /* allow to activate equipped items for recall (some art(s)!) */
					o_ptr = &(p_ptr->inventory[i]);
					if (!o_ptr->tval) continue;
					if (!find_inscription(o_ptr->note, "@R")) continue;

					/* For spell books: Test if we can actually use this item at all,
					   ie have learned the spell yet, otherwise skip it completely!
					   This is because we might've picked up books from someone else. */
					if (o_ptr->tval == TV_BOOK) {
						if (o_ptr->sval == SV_SPELLBOOK) {
							if (o_ptr->pval == spell_rec || o_ptr->pval == spell_rel) {
								/* Have we learned this spell yet at all? */
								if (!exec_lua(Ind, format("return is_ok_spell(%d, %d)", Ind, o_ptr->pval)))
									/* Just continue&ignore instead of return, since we
									   might just have picked up someone else's book! */
									continue;
								/* If so then use it */
								spell = o_ptr->pval;
							} else {
								/* "No recall spell found in this book!" */
								//continue;
								/* Be severe and point out the wrong inscription: */
								msg_print(Ind, "\377oThe inscribed spell scroll isn't a recall spell.");
								return;
							}
						} else {
							if (MY_VERSION < (4 << 12 | 4 << 8 | 1 << 4 | 8)) {
								spell_rec_found = exec_lua(Ind, format("return spell_in_book(%d, %d)", o_ptr->sval, spell_rec));
#ifdef ENABLE_DIVINE
								spell_rel_found = exec_lua(Ind, format("return spell_in_book(%d, %d)", o_ptr->sval, spell_rel));
#endif
								if (!spell_rec_found && !spell_rel_found) {
									/* Be severe and point out the wrong inscription: */
									msg_print(Ind, "\377oThe inscribed book doesn't contain a recall spell.");
									return;
								}
							} else {
								spell_rec_found = exec_lua(Ind, format("return spell_in_book2(%d, %d, %d)", i, o_ptr->sval, spell_rec));
#ifdef ENABLE_DIVINE
								spell_rel_found = exec_lua(Ind, format("return spell_in_book2(%d, %d, %d)", i, o_ptr->sval, spell_rel));
#endif
								if (!spell_rec_found && !spell_rel_found) {
									/* Be severe and point out the wrong inscription: */
									msg_print(Ind, "\377oThe inscribed book doesn't contain a recall spell.");
									return;
								}
							}
							/* Have we learned this spell yet at all? */
							if (spell_rec_found && exec_lua(Ind, format("return is_ok_spell(%d, %d)", Ind, spell_rec)))
								spell = spell_rec;
							if (spell_rel_found && exec_lua(Ind, format("return is_ok_spell(%d, %d)", Ind, spell_rel)))
								spell = spell_rel;
							/* Just continue&ignore instead of return, since we
							   might just have picked up someone else's book! */
							if (spell == -1) continue;
						}
					}

					item = i;
					break;
				}

				if (item == -1) {
					msg_print(Ind, "\377oNo usable item with '@R' inscription found.");
					return;
				}

				disturb(Ind, 1, 0);

				/* ALERT! Hard-coded! */
				switch (o_ptr->tval) {
				case TV_SCROLL:
					do_cmd_read_scroll(Ind, item);
					break;
				case TV_ROD:
					do_cmd_zap_rod(Ind, item, 0);
					break;
				/* Cast Recall spell - mikaelh */
				case TV_BOOK:
#if 0
					if (o_ptr->sval == SV_SPELLBOOK) {
						/* Test for 'Recall' istar spell: */
						if (o_ptr->pval != spell_rec) {
							msg_print(Ind, "\377oThis is not a Spell Scroll of Recall.");
							return;
						}
						/* Test for 'Relocation' astral spell: */
					} else {
						if (MY_VERSION < (4 << 12 | 4 << 8 | 1 << 4 | 8)) {
							if (exec_lua(Ind, format("return spell_in_book(%d, %d)", o_ptr->sval, spell)) == FALSE) {
								msg_print(Ind, "\377oRecall spell not found in this book.");
								return;
							}
						} else {
							if (exec_lua(Ind, format("return spell_in_book2(%d, %d, %d)", item, o_ptr->sval, spell)) == FALSE) {
								msg_print(Ind, "\377oRecall spell not found in this book.");
								return;
							}
						}
					}
#endif
					cast_school_spell(Ind, item, spell, -1, -1, 0);
					break;
				default:
					do_cmd_activate(Ind, item, 0);
					//msg_print(Ind, "\377oYou cannot recall with that.");
					break;
				}
			}

			switch (tk) {
			case 1:
				/* depth in feet */
				p_ptr->recall_pos.wz = k / (p_ptr->depth_in_feet ? 50 : 1);
				break;
			case 2:
				p_ptr->recall_pos.wx = k % MAX_WILD_X;
				p_ptr->recall_pos.wy = atoi(token[2]) % MAX_WILD_Y;
				p_ptr->recall_pos.wz = 0;
				break;
//			default:	/* follow the inscription */
				/* TODO: support tower */
//				p_ptr->recall_pos.wz = 0 - p_ptr->max_dlv;
//				p_ptr->recall_pos.wz = 0;
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
					do_cmd_check_other_prepare(Ind, path, "");
					return;
				}
				//					else if (strstr(token[1], "rfe") &&

			}
			/* default is "mangband.rfe" */
			else if ((is_admin(p_ptr) || cfg.public_rfe))
			{
				path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_DATA, "tomenet.rfe");
				do_cmd_check_other_prepare(Ind, path, "RFE/Bug file");
				return;
			}
			else msg_print(Ind, "\377o/less is not opened for use...");
			return;
		}
		else if (prefix(message, "/news")) 
		{
			char    path[MAX_PATH_LENGTH];
			path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_TEXT, "news.txt");
			do_cmd_check_other_prepare(Ind, path, "News");
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

			do_cmd_check_other_prepare(Ind, path, "Help");
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
			j = Ind; //k=0;
			u16b r, num;
			int lev;
			u16b flags=(QUEST_MONSTER|QUEST_RANDOM);
	
			if (tk && !strcmp(token[1], "reset")) {
				int qn;
				if (!admin) return;
				for (qn = 0; qn < 20; qn++) {
					quests[qn].active=0;
					quests[qn].type=0;
					quests[qn].id=0;
				}
				msg_broadcast(0, "\377yQuests are reset");
				for (i = 1; i <= NumPlayers; i++) {
					if (Players[i]->conn == NOT_CONNECTED) continue;
					Players[i]->quest_id = 0;
				}
				return;
			}
			if (tk && !strcmp(token[1], "guild")) {
				if (!p_ptr->guild || guilds[p_ptr->guild].master != p_ptr->id) {
					msg_print(Ind, "\377rYou are not a guildmaster");
					return;
				}
				if (tk == 2) {
					if ((j = name_lookup_loose(Ind, token[2], FALSE))) {
						if(Players[j]->quest_id){
							msg_format(Ind, "\377y%s has a quest already.", Players[j]->name);
							return;
						}
					} else {
						msg_format(Ind, "Player %s is not here", token[2]);
						return;
					}
				} else {
					msg_print(Ind, "Usage: /quest guild name");
					return;
				}
				flags |= QUEST_GUILD;
				lev = Players[j]->lev + 5;
			}
			else if (admin && tk) {
				if ((j = name_lookup_loose(Ind, token[1], FALSE))) {
					if (Players[j]->quest_id) {
						msg_format(Ind, "\377y%s has a quest already.", Players[j]->name);
						return;
					}
					lev = Players[j]->lev;	/* for now */
				}
				else return;
			} else {
				flags |= QUEST_RACE;
				lev = Players[j]->lev;
			}
			if (prepare_quest(Ind, j, flags, &lev, &r, &num))
#if 0 /* done in prepare_quest */
			if (Players[j]->quest_id) {
				for (i = 0; i < 20; i++) {
					if (quests[i].id == Players[j]->quest_id) {
						if (j == Ind)
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

			get_mon_num_hook = quest_aux;
			get_mon_num_prep(0, NULL);
			i = 2 + randint(5);

			do {
				r = get_mon_num(lev, lev);

				/* Hack: If (non-)FRIENDS variant exists, use the first one (usually the non-FRIENDS one) */
				if (r_info[r].dup_idx) r = r_info[r].dup_idx;

				k++;
				if (k > 1000) {
					lev--;
					k = lev * 5;
//					k = lev * 9;
//					k = 900;
                                }
			} while (((lev-5) > r_info[r].level) || 
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
			cave_type **zcave = getcave(&p_ptr->wpos);
			bool no_tele = FALSE;
			if (zcave)
				no_tele = (zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) != 0;

			if (!show_floor_feeling(Ind) && !no_tele)
				msg_print(Ind, "You feel nothing special.");

			if (no_tele)
				msg_print(Ind, "\377DThe air in here feels very still.");
			return;
		}
		else if (prefix(message, "/monster") ||
				prefix(message, "/mon"))
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
			if (!r_idx) {
				msg_print(Ind, "No such monster.");
				return;
			}

			r_ptr = &r_info[r_idx];
			num = p_ptr->r_killed[r_idx];

			if (r_ptr->flags1 & RF1_UNIQUE) {
				if (!num) msg_format(Ind, "%s : not slain.", r_name + r_ptr->name);
				else if (num == 1) msg_format(Ind, "%s : slain.", r_name + r_ptr->name);
				else msg_format(Ind, "%s : assisted in slaying.", r_name + r_ptr->name);
			} else if (get_skill(p_ptr, SKILL_MIMIC) &&
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
			} else {
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
			p_ptr->window |= (PW_INVEN);// | PW_EQUIP);

			return;
		}
		else if (prefix(message, "/house") ||
				prefix(message, "/hou"))
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
		else if (prefix(message, "/sip")) {
			/* Paralyzed? */
			if (p_ptr->energy < level_speed(&p_ptr->wpos)) return;

			do_cmd_drink_fountain(Ind);
			return;
		}
		else if (prefix(message, "/fill")) {
			/* Paralyzed? */
			if (p_ptr->energy < level_speed(&p_ptr->wpos)) return;

			do_cmd_fill_bottle(Ind);
			return;
		}
		else if (prefix(message, "/empty") || prefix(message, "/emp")) {
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
		else if (prefix(message, "/dice")) {
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
			msg_format(Ind, "\374\377%cYou throw %d dice and get a %d", COLOUR_GAMBLE, k, rn);
			msg_format_near(Ind, "\374\377%c%s throws %d dice and gets a %d", COLOUR_GAMBLE, p_ptr->name, k, rn);
			return;
		}
		else if (prefix(message, "/coin")) {
			bool coin = (rand_int(2) == 0);
			if (p_ptr->energy < level_speed(&p_ptr->wpos)) return;
			p_ptr->energy -= level_speed(&p_ptr->wpos);

			msg_format(Ind, "\374\377%cYou flip a coin and get %s.", COLOUR_GAMBLE, coin ? "heads" : "tails");
			msg_format_near(Ind, "\374\377%c%s flips a coin and gets %d", COLOUR_GAMBLE, p_ptr->name, coin ? "heads" : "tails");
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
				msg_print(Ind, "\377rPet system is disabled.");
				return;
			}
			if (Players[Ind]->has_pet == 2) {
				msg_print(Ind, "\377rYou cannot have anymore pets!");
				return;
			}
                        if (pet_creation(Ind))
				msg_print(Ind, "\377USummoning a pet.");
			else 
				msg_print(Ind, "\377rYou already have a pet!");
			return;
                }
#endif
                else if (prefix(message, "/unpet")) {
#ifdef RPG_SERVER
			msg_print(Ind, "\377RYou abandon your pet! You cannot have anymore pets!");
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
//				msg_print(Ind, "\377yYou cannot abandon your pet while the whole town is looking!");
//			}
#endif
			return;
                }
                /* An alternative to dicing :) -the_sandman */
                else if (prefix(message, "/deal"))
                {
                        int value, flower;
                        char* temp;
			if (p_ptr->energy < level_speed(&p_ptr->wpos)) return;
			p_ptr->energy -= level_speed(&p_ptr->wpos);

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
                                        msg_format(Ind, "\377%c%s was dealt the %s of Diamond", COLOUR_GAMBLE, p_ptr->name, temp);
                                        msg_format_near(Ind, "\377%c%s was dealt the %s of Diamond", COLOUR_GAMBLE, p_ptr->name, temp);
                                        break;
                                case 2:
                                        msg_format(Ind, "\377%c%s was dealt the %s of Club", COLOUR_GAMBLE, p_ptr->name, temp);
                                        msg_format_near(Ind, "\377%c%s was dealt the %s of Club", COLOUR_GAMBLE, p_ptr->name, temp);
                                        break;
                                case 3:
                                        msg_format(Ind, "\377%c%s was dealt the %s of Heart", COLOUR_GAMBLE, p_ptr->name, temp);
                                        msg_format_near(Ind, "\377%c%s was dealt the %s of Heart", COLOUR_GAMBLE, p_ptr->name, temp);
                                        break;
                                case 4:
                                        msg_format(Ind, "\377%c%s was dealt the %s of Spade", COLOUR_GAMBLE, p_ptr->name, temp);
                                        msg_format_near(Ind, "\377%c%s was dealt the %s of Spade", COLOUR_GAMBLE, p_ptr->name, temp);
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
			j = 0;
#if 0 /* allow only a party owner to write the note */
			int p = -1;
			for (i = 1; i < MAX_PARTIES; i++) { /* was i = 0 but real parties start from i = 1 - mikaelh */
				if(!strcmp(parties[i].owner, p_ptr->name)) p = i;
			}
			if (p < 0) {
				msg_print(Ind, "\377oYou aren't a party owner.");
				return;
			}
#else /* allow every party member to change the note */
			if (!p_ptr->party) {
				msg_print(Ind, "\377oYou are not in a party.");
				return;
			}
#endif
			if (tk == 1 && !strcmp(token[1], "*")) {
				/* Erase party note */
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
//				msg_print(Ind, "\377oParty note has been erased.");
				msg_party_format(Ind, "\377b%s erased the party note.", p_ptr->name);
				return;
			}
			if (tk == 0) {
				/* Display party note */
				bool found_note = FALSE;
				for (i = 0; i < MAX_PARTYNOTES; i++) {
					if (!strcmp(party_note_target[i], parties[p_ptr->party].name)) {
						if (strcmp(party_note[i], "")) {
							msg_format(Ind, "\377bParty Note: %s", party_note[i]);
							found_note = TRUE;
						}
						break;
					}
				}
				if (!found_note) msg_print(Ind, "\377bNo party note stored.");
				return;
			}
			if (tk > 0)	/* Set new party note */
			{
				/* Search for begin of parms ( == text of the note) */
				for (i = 6; i < (int)strlen(message2); i++)
					if (message2[i] == ' ')
						for (j = i; j < (int)strlen(message2); j++)
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
//					msg_print(Ind, "\377yNote has been stored.");
					msg_party_format(Ind, "\377b%s changed party note to: %s", p_ptr->name, party_note[i]);
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
					msg_format(Ind, "\377oSorry, the server reached the maximum of %d party notes.", MAX_PARTYNOTES);
					return;
				}
				strcpy(party_note_target[i], parties[p_ptr->party].name);
				message2[j + 79] = '\0'; /* Limit to 80 chars */
				strcpy(party_note[i], &message2[j]);
//				msg_print(Ind, "\377yNote has been stored.");
				msg_party_format(Ind, "\377b%s set party note to: %s", p_ptr->name, party_note[i]);
				return;
			}
		}
		else if (prefix(message, "/noteg"))
		{
			j = 0;
#if 0 /* only allow the guild master to write the note */
			int p = -1;
			for (i = 0; i < MAX_GUILDS; i++) {
				if(guilds[i].master == p_ptr->id) p = i;
			}
			if (p < 0) {
				msg_print(Ind, "\377oYou aren't a guild master.");
				return;
			}
#else /* allow all guild members to write the note */
			if (!p_ptr->guild) {
				msg_print(Ind, "\377oYou are not in a guild.");
				return;
			}
#endif

			if (tk == 1 && !strcmp(token[1], "*")) {
				/* Erase guild note */
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
//				msg_print(Ind, "\377oGuild note has been erased.");
				msg_guild_format(Ind, "\377b%s erased the guild note.", p_ptr->name);
				return;
			}
			if (tk == 0) {
				/* Display guild note */
				bool found_note = FALSE;
				for (i = 0; i < MAX_GUILDNOTES; i++) {
					if (!strcmp(guild_note_target[i], guilds[p_ptr->guild].name)) {
						if (strcmp(guild_note[i], "")) {
							msg_format(Ind, "\377bGuild Note: %s", guild_note[i]);
							found_note = TRUE;
						}
						break;
					}
				}
				if (!found_note) msg_print(Ind, "\377bNo guild note stored.");
				return;
			}
			if (tk > 0) { /* Set new Guild note */
				/* Search for begin of parms ( == text of the note) */
				for (i = 6; i < (int)strlen(message2); i++)
					if (message2[i] == ' ')
						for (j = i; j < (int)strlen(message2); j++)
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
					message2[j + 79] = '\0'; /* Limit to 80 chars */
					strcpy(guild_note[i], &message2[j]);
//					msg_print(Ind, "\377yNote has been stored.");
					msg_guild_format(Ind, "\377b%s changed the guild note to: %s", p_ptr->name, guild_note[i]);
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
					msg_format(Ind, "\377oSorry, the server reached the maximum of %d guild notes.", MAX_GUILDNOTES);
					return;
				}
				strcpy(guild_note_target[i], guilds[p_ptr->guild].name);
				strcpy(guild_note[i], &message2[j]);
//				msg_print(Ind, "\377yNote has been stored.");
				msg_guild_format(Ind, "\377b%s set the guild note to: %s", p_ptr->name, guild_note[i]);
				return;
			}
		}
		else if (prefix(message, "/snotes") ||
			prefix(message, "/motd")) { /* same as /anotes for admins basically */
			for (i = 0; i < MAX_ADMINNOTES; i++) {
				if (strcmp(admin_note[i], "")) {
					msg_format(Ind, "\377sMotD: %s", admin_note[i]);
				}
			}
			if (server_warning[0]) msg_format(Ind, "\377R*** Note: %s ***", server_warning);
			return;
		}
		else if (prefix(message, "/notes"))
		{
			int notes = 0;
			for (i = 0; i < MAX_NOTES; i++) {
				/* search for pending notes of this player */
				if (!strcmp(priv_note_sender[i], p_ptr->name)) {
					/* found a matching note */
					notes++;
				}
			}
			if (notes > 0) msg_format(Ind, "\377oYou wrote %d currently pending notes:", notes);
			else msg_print(Ind, "\377oYou didn't write any pending notes.");
			for (i = 0; i < MAX_NOTES; i++) {
				/* search for pending notes of this player */
				if (!strcmp(priv_note_sender[i], p_ptr->name)) {
					/* found a matching note */
					msg_format(Ind, "\377oTo %s: %s", priv_note_target[i], priv_note[i]);
				}
			}
			return;
		}
		else if (prefix(message, "/notec") || prefix(message, "/note"))
		{
			int notes = 0, found_note = MAX_NOTES;
			j = 0;
			bool colon = FALSE;
			char tname[80], *tpname; /* target's account name */
			struct account *c_acc;

			if (prefix(message, "/notec")) {
				if (tk < 1) { /* Explain command usage */
					msg_print(Ind, "\377oUsage: /notec <character name>[:<text>]");
					msg_print(Ind, "\377oExample: /notec Pally:Hiho!");
					msg_print(Ind, "\377oNot specifiying any text will remove pending notes to the player's account.");
					msg_print(Ind, "\377oTo clear all pending notes of yours, type:  /note *  or  /notec *");
					return;
				}

				/* translate character name to account name */
				if (!(tpname = strchr(message2 + 7, ':'))) {
					/* no text given */
					if (!lookup_player_id(message2 + 7)) {
						msg_print(Ind, "\377oPlayer not found.");
						return;
					}
					strcpy(tname, "/note ");
					strcat(tname, lookup_accountname(lookup_player_id(message2 + 7)));
					strcpy(message2, tname);
					strcpy(tname, "");
				} else {
					/* note text given */
					tpname[0] = 0;
					if (!lookup_player_id(message2 + 7)) {
						msg_print(Ind, "\377oPlayer not found.");
						return;
					}
					strcpy(tname, "/note ");
					strcat(tname, lookup_accountname(lookup_player_id(message2 + 7)));
					strcat(tname, ":");
					strcat(tname, tpname + 1);
					strcpy(message2, tname);
					strcpy(tname, "");
				}
			}

			if (tk < 1) { /* Explain command usage */
				msg_print(Ind, "\377oUsage: /note <player account>[:<text>]");
				msg_print(Ind, "\377oExample: /note Mithrandir:Hiho!");
				msg_print(Ind, "\377oNot specifiying any text will remove pending notes to that account.");
				msg_print(Ind, "\377oTo clear all pending notes of yours, type:  /note *");
				msg_print(Ind, "\377oHint: Use /notec to specify a character name instead of account.");
				return;
			}
			/* Does a colon appear? */
			for (i = 0; i < (int)strlen(message2); i++)
				if (message2[i] == ':') colon = TRUE;
			/* Depending on colon existance, extract the target name */
			for (i = 5; i < (int)strlen(message2); i++)
				if (message2[i] == ' ') {
					for (j = i; j < (int)strlen(message2); j++)
						/* find where target name starts, save pos in j */
						if (message2[j] != ' ') {
							for (i = j; i < (int)strlen(message2); i++) {
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
							if (i == (int)strlen(message2)) {
								/* extract name till end of line (it's the only parm) */
								strcpy(tname, message2 + j);
							}
							break;
						}
					break;
				}

			/* was it just a '/note *' ? */
			if (!colon && !strcmp(tname, "*")) { /* Delete all pending notes to all players */
				for (i = 0; i < MAX_NOTES; i++) {
					/* search for pending notes of this player */
					if (!strcmp(priv_note_sender[i], p_ptr->name)) {
						notes++;
						strcpy(priv_note_sender[i], "");
						strcpy(priv_note_target[i], "");
						strcpy(priv_note[i], "");
					}
				}
				msg_format(Ind, "\377oDeleted %d notes.", notes);
				return;
			}

			/* No colon found -> only a name parm give */
			if (!colon) { /* Delete all pending notes to a specified player */
				for (i = 0; i < MAX_NOTES; i++) {
					/* search for pending notes of this player to the specified player */
					if (!strcmp(priv_note_sender[i], p_ptr->name) &&
					    !strcmp(priv_note_target[i], tname)) {
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

			/* does target account exist? */
			c_acc = GetAccount(tname, NULL, FALSE);
			if (!c_acc) {
				msg_print(Ind, "\377oThat account does not exist.");
				return;
			}

			/* Check whether player has his notes quota exceeded */
			for (i = 0; i < MAX_NOTES; i++) {
				if (!strcmp(priv_note_sender[i], p_ptr->name)) notes++;
			}
			if ((notes >= 4) && !is_admin(p_ptr)) {
				msg_print(Ind, "\377oYou have already reached the maximum of 4 pending notes per player.");
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

			/* Check whether target is actually online by now :) */
			if ((i = name_lookup_loose_quiet(Ind, tname, FALSE))) {
				msg_format(i, "\377bNote from %s: %s", p_ptr->name, message2 + j + 1);
//				return; //so double-msg him just to be safe he sees it
			}

			strcpy(priv_note_sender[found_note], p_ptr->name);
			strcpy(priv_note_target[found_note], tname);
			message2[j + 1 + 79] = '\0'; /* Limit to 80 chars */
			strcpy(priv_note[found_note], message2 + j + 1);
			msg_format(Ind, "\377yNote for account '%s' has been stored.", priv_note_target[found_note]);
			return;
		}
		else if (prefix(message, "/play") /* for joining games - mikaelh */
		    && !prefix(message, "/playgo"))
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
			msg_format(Ind, "%d", kidx);
			invcopy(o_ptr, kidx);

			if(tk > 2) o_ptr->pval = (atoi(token[3]) < 15) ? atoi(token[3]) : 15;
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
					if (nom) {
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
//			if(tk > 2) o_ptr->pval = (atoi(token[3]) < 15) ? atoi(token[3]) : 15;
			//o_ptr->owner = p_ptr->id;
			o_ptr->level = 1;
			(void)inven_carry(Ind, o_ptr);

			return;
		}
#endif
		else if (prefix(message, "/evinfo")) /* get info on a global event */
		{
			int n = 0;
			char ppl[75];
			bool found = FALSE;
			if (tk < 1) {
				msg_print(Ind, "\377d ");
				for (i = 0; i<MAX_GLOBAL_EVENTS; i++) if ((global_event[i].getype != GE_NONE) && (global_event[i].hidden==FALSE || admin)) {
					n++;
					if (n == 1) {
						msg_print(Ind, "\377sCurrently ongoing events (use /evsign to participate):");
						msg_print(Ind, "\377sUse command '/evinfo <number>' to get information on a specific event.");
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
				msg_format(Ind, "Usage: /evinfo    or    /evinfo 1..%d", MAX_GLOBAL_EVENTS);
			} else if ((global_event[k-1].getype == GE_NONE) && (global_event[k-1].hidden==FALSE || admin)) {
				msg_print(Ind, "\377yThere is currently no running event of that number.");
			} else {
				msg_format(Ind, "\377sInfo on event #%d '\377s%s\377s':", k, global_event[k-1].title);
				for (i = 0; i < 10; i++) if (strcmp(global_event[k-1].description[i], ""))
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
		else if (prefix(message, "/evsign")) /* sign up for a global event */
		{
			if ((tk < 1) || (k < 1) || (k > MAX_GLOBAL_EVENTS))
				msg_format(Ind, "Usage: /evsign 1..%d [options..]    -- Also try: /evinfo", MAX_GLOBAL_EVENTS);
			else if ((global_event[k-1].getype == GE_NONE) && (global_event[k-1].hidden==FALSE || admin))
				msg_print(Ind, "\377yThere is currently no running event of that number.");
			else if (global_event[k-1].signup_time == -1)
				msg_print(Ind, "\377yThat event doesn't offer to sign up.");
			else if (!global_event[k-1].signup_time &&
				    (!global_event[k-1].announcement_time ||
				    (global_event[k-1].announcement_time - (turn - global_event[k-1].start_turn) / cfg.fps <= 0)))
				msg_print(Ind, "\377yThat event has already started.");
			else if (global_event[k-1].signup_time &&
				    (global_event[k-1].signup_time - (turn - global_event[k-1].start_turn) / cfg.fps <= 0))
				msg_print(Ind, "\377yThat event does not allow signing up anymore now.");
			else {
				if (tk < 2) global_event_signup(Ind, k-1, NULL);
				else global_event_signup(Ind, k-1, message3 + 1 + strlen(format("%d", k)));
			}
			return;
		}
		else if (prefix(message, "/bbs")) /* write a short text line to the server's in-game message board,
						    readable by all players via '!' key. For example to warn about
						    static (deadly) levels - C. Blue */
		{
			if (!strcmp(message3, "")) {
#if 0
				msg_print(Ind, "Usage: /bbs <line of text for others to read>");
#else
				bool bbs_empty = TRUE;
				/* Look at in-game bbs, instead of "usage" msg above */
				msg_print(Ind, "\377sBulletin board (type '/bbs <text>' in chat to write something):");
				for (i = 0; i < BBS_LINES; i++)
					if (strcmp(bbs_line[i], "")) {
						msg_format(Ind, "\377s %s", bbs_line[i]);
						bbs_empty = FALSE;
					}
				if (bbs_empty) msg_print(Ind, "\377s <nothing has been written on the board so far>");
#endif
				return;
			}

			if (p_ptr->inval) {
				msg_print(Ind, "\377oYou must be validated to write to the in-game bbs.");
				return;
			}

			msg_broadcast_format(0, "\374\377s[%s->BBS] \377W%s", p_ptr->name, message3);
//			bbs_add_line(format("%s %s: %s",showtime() + 7, p_ptr->name, message3));
			bbs_add_line(format("\377s%s %s: \377W%s",showdate(), p_ptr->name, message3));
			return;
		}
		else if (prefix(message, "/time")) { /* show time / date */
			msg_format(Ind, "Current server time is %s", showtime());
			return;
		}
		else if (prefix(message, "/pvp")) { /* enter pvp-arena (for MODE_PVP) */
			struct worldpos apos;
			int ystart = 0, xstart = 0;
			bool fresh_arena = FALSE;

			/* can't get in if not PvP mode */
			if (!(p_ptr->mode & MODE_PVP)) {
				msg_print(Ind, "\377yYour character is not PvP mode.");
				if (!is_admin(p_ptr)) return;
			}

			/* transport out of arena? */
			if (!p_ptr->wpos.wx && !p_ptr->wpos.wy &&
			    p_ptr->wpos.wz == 1) {
				if (p_ptr->pvp_prevent_tele) {
					msg_print(Ind, "\377oThere is no easy way out of this fight!");
					if (!is_admin(p_ptr)) return;
				}
				p_ptr->recall_pos.wx = cfg.town_x;
				p_ptr->recall_pos.wy = cfg.town_y;
				p_ptr->recall_pos.wz = 0;
				p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
				recall_player(Ind, "");
				msg_format(Ind, "\377%cYou leave the arena again.", COLOUR_DUNGEON);
				return;
			}

			/* prevent exploit */
			if (!istown(&p_ptr->wpos)) {
				if (isdungeontown(&p_ptr->wpos)) {
					msg_print(Ind, "\377yYou cannot enter the arena from within a dungeon!");
					if (!is_admin(p_ptr)) return;
				}
				msg_print(Ind, "\377yYou need to be in town to enter the arena!");
				if (!is_admin(p_ptr)) return;
			}

			msg_print(Ind, "\377fYou enter the arena to fight as Gladiator!");
			if (!admin_p(Ind)) {
				msg_broadcast(Ind, "\377cSomeone entered the gladiators' arena!");
				/* prevent instant-leavers if they see roaming monsters or whatever */
				if (p_ptr->pvp_prevent_tele < PVP_COOLDOWN_TELEPORT) p_ptr->pvp_prevent_tele = PVP_COOLDOWN_TELEPORT;
			}

			/* actually create temporary Arena tower at reserved wilderness sector 0,0! */
			apos.wx = 0; apos.wy = 0; apos.wz = 0;
			if (!wild_info[apos.wy][apos.wx].tower) {
				add_dungeon(&apos, 1, 1, DF1_NO_RECALL | DF1_SMALLEST,
				    DF2_NO_ENTRY_MASK | DF2_NO_EXIT_MASK, TRUE, 0);
				fresh_arena = TRUE;
			}
			apos.wz = 1;
			if (!getcave(&apos)) {
				alloc_dungeon_level(&apos);
				fresh_arena = TRUE;
			}
			if (fresh_arena) generate_cave(&apos, p_ptr); /* <- required or panic save: py,px will be far negative (haven't checked why) */

                	p_ptr->recall_pos = apos;
                	p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
			recall_player(Ind, "");

			if (fresh_arena) {
				wipe_m_list(&apos);
				wipe_o_list_safely(&apos);
				process_dungeon_file("t_arena_pvp.txt", &apos, &ystart, &xstart, MAX_HGT, MAX_WID, TRUE);

				timer_pvparena1 = 1; /* (hack: generate 1st cycle) // seconds countdown */
				timer_pvparena2 = 1; /* start with releasing 1st monster */
				timer_pvparena3 = 0; /* 'basic monsters' cycle active */
			}
			return;
		}
#ifdef AUCTION_SYSTEM
		else if (prefix(message, "/auc")) {
			int n;

			if (p_ptr->inval) {
				msg_print(Ind, "\377oYou must be validated to use the auction system.");
				return;
			}

			if (p_ptr->wpos.wz && !is_admin(p_ptr))
			{
				if (p_ptr->wpos.wz < 0) msg_print(Ind, "\377B[@] \377rYou can't use the auction system while in a dungeon!");
				else msg_print(Ind, "\377B[@] \377rYou can't use the auction system while in a tower!");
				return;
			}

			if ((tk < 1) || ((tk < 2) && (!strcmp("help", token[1]))))
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
					msg_format(Ind, "\377B[@] \377wShortest duration allowed is %s.", time_string);
					C_KILL(time_string, strlen(time_string), char);
#endif
#ifdef AUCTION_MAXIMUM_DURATION
					time_string = auction_format_time(AUCTION_MAXIMUM_DURATION);
					msg_format(Ind, "\377B[@] \377wLongest duration allowed is %s.", time_string);
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
			else if (!strncmp("bid", token[1], 3))
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
			else if (!strncmp("buyout", token[1], 3))
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
			else if (!strncmp("cancel", token[1], 3))
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
			else if (!strncmp("examine", token[1], 3))
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
			else if (!strncmp("list", token[1], 3))
			{
				auction_list(Ind);
			}
			else if (!strncmp("retrieve", token[1], 3))
			{
				int retrieved, unretrieved;
				auction_retrieve_items(Ind, &retrieved, &unretrieved);
				if (!unretrieved) msg_format(Ind, "\377B[@] \377wRetrieved %d item(s).", retrieved);
				else msg_format(Ind, "\377B[@] \377wRetrieved %d items, you didn't have room for %d more item(s).", retrieved, unretrieved);
			}
			else if (!strncmp("search", token[1], 3))
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
			else if (!strncmp("show", token[1], 3))
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
			else if (!strncmp("set", token[1], 3))
			{
				if (tk < 5)
				{
					msg_print(Ind, "\377B[@] \377oUsage: /auction set <inventory slot> <starting price> <buyout price> <duration>");
				}
				else
				{
					n = auction_set(Ind, token[2][0] - 'a', token[3], token[4], token[5]);
					if (n)
					{
						auction_print_error(Ind, n);
					}
				}
			}
			else if (!strncmp("start", token[1], 3))
			{
				if (!p_ptr->current_auction)
				{
					msg_print(Ind, "\377B[@] \377rNo item!");
				}
				else
				{
					n = auction_start(Ind);
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

		/* Allow players to undo some of their skills - mikaelh */
		else if (prefix(message, "/undoskills") ||
				prefix(message, "/undos"))
		{
			/* Skill points gained */
			int gain = p_ptr->skill_points_old - p_ptr->skill_points;

			if (gain && p_ptr->reskill_possible)
			{
				memcpy(p_ptr->s_info, p_ptr->s_info_old, MAX_SKILLS * sizeof(skill_player));
				p_ptr->skill_points = p_ptr->skill_points_old;

				msg_format(Ind, "\377GYou have regained %d skill points.", gain);
				
				/* Update all skills */
				calc_techniques(Ind);
				for (i = 0; i < MAX_SKILLS; i++)
					Send_skill_info(Ind, i, FALSE);

				p_ptr->update |= (PU_SKILL_MOD | PU_BONUS | PU_MANA | PU_HP);
				p_ptr->redraw |= (PR_SKILLS | PR_PLUSSES);

				/* No more reskills */
				p_ptr->reskill_possible = FALSE;
			}
			else
			{
				msg_print(Ind, "\377yNo skills could be undone.");
			}
			return;
		}
#if 0
		else if (prefix(message, "/info")) { /* set a personal info message - C. Blue */
			char to_strip[80];
			if (strlen(message2) > 6) {
				strncpy(to_strip, message2 + 6, MAX_CHARS);
				if (strlen(message2 + 6) >= MAX_CHARS) to_strip[MAX_CHARS - 1] = '\0';
				else to_strip[strlen(message2 + 6)] = '\0';
				strip_control_codes(p_ptr->info_msg, to_strip);
				msg_print(Ind, "Personal info message has been changed.");
			} else {
				strcpy(p_ptr->info_msg, "");
				msg_print(Ind, "Personal info message has been cleared.");
			}
			return;
		}
#endif
#if 0
		else if (prefix(message, "/pray")) { /* hidden broadcast to all admins :) */
			msg_admin("\377b[\377U%s\377b]\377D%s", p_ptr->name, 'w', message3);
			return;
		}
#endif
		else if (prefix(message, "/pbbs")) {
			/* Look at or write to in-game party bbs, as suggested by Caine/Goober - C. Blue */
			bool bbs_empty = TRUE;
			int n;

			if (!p_ptr->party) {
				msg_print(Ind, "You have to be in a party to interact with a party BBS.");
				return;
			}

			if (tk) {
				/* write something */
				msg_party_format(Ind, "\374\377B[%s->PBBS] \377W%s", p_ptr->name, message3);
				pbbs_add_line(p_ptr->party, format("\377B%s %s: \377W%s",showdate(), p_ptr->name, message3));
				return;
			}
			msg_print(Ind, "\377BParty bulletin board (type '/pbbs <text>' in chat to write something):");
			for (n = 0; n < BBS_LINES; n++)
				if (strcmp(pbbs_line[p_ptr->party][n], "")) {
					msg_format(Ind, "\377B %s", pbbs_line[p_ptr->party][n]);
					bbs_empty = FALSE;
				}
			if (bbs_empty) msg_print(Ind, "\377B <nothing has been written on the party board so far>");
			return;
		}
		else if (prefix(message, "/gbbs")) {
			/* Look at or write to in-game guild bbs - C. Blue */
			bool bbs_empty = TRUE;
			int n;

			if (!p_ptr->guild) {
				msg_print(Ind, "You have to be in a guild to interact with a guild BBS.");
				return;
			}

			if (tk) {
				/* write something */
				msg_guild_format(Ind, "\374\377G[%s->GBBS] \377W%s", p_ptr->name, message3);
				gbbs_add_line(p_ptr->guild, format("\377G%s %s: \377W%s",showdate(), p_ptr->name, message3));
				return;
			}
			msg_print(Ind, "\377GGuild bulletin board (type '/gbbs <text>' in chat to write something):");
			for (n = 0; n < BBS_LINES; n++)
				if (strcmp(gbbs_line[p_ptr->guild][n], "")) {
					msg_format(Ind, "\377G %s", gbbs_line[p_ptr->guild][n]);
					bbs_empty = FALSE;
				}
			if (bbs_empty) msg_print(Ind, "\377G <nothing has been written on the guild board so far>");
			return;
		}
		else if (prefix(message, "/ftkon")) {
			msg_print(Ind, "\377wFire-till-kill mode now on.");
			p_ptr->shooting_till_kill = TRUE;
			s_printf("SHOOT_TILL_KILL: Player %s sets true.\n", p_ptr->name);
			p_ptr->redraw |= PR_STATE;
			return;
		} else if (prefix(message, "/ftkoff")) {
			msg_print(Ind, "\377wFire-till-kill mode now off.");
			p_ptr->shooting_till_kill = FALSE;
			s_printf("SHOOT_TILL_KILL: Player %s sets false.\n", p_ptr->name);
			p_ptr->redraw |= PR_STATE;
			return;
		}
#ifdef PLAYER_STORES
		else if (prefix(message, "/pstore")) {
			int x, y;
			cave_type **zcave = getcave(&p_ptr->wpos);
			/* Enter a player store next to us */
			for (x = p_ptr->px - 1; x <= p_ptr->px + 1; x++)
			for (y = p_ptr->py - 1; y <= p_ptr->py + 1; y++) {
				if (!in_bounds(y, x)) continue;
				if (zcave[y][x].feat != FEAT_HOME &&
				    zcave[y][x].feat != FEAT_HOME_OPEN)
					continue;
				disturb(Ind, 1, 0);
				if (do_cmd_player_store(Ind, x, y)) return;
			}
			msg_print(Ind, "There is no player store next to you.");
			return;
		}
#endif
#ifdef HOUSE_PAINTING /* goes hand in hand with player stores.. */
		else if (prefix(message, "/paint")) { /* paint a house that we own */
			int x, y;
			bool found = FALSE;
			cave_type **zcave = getcave(&p_ptr->wpos);

			/* need to specify one parm: the potion used for colouring */
			if (tk == 1) k = message3[0] - 97;
			if (!tk || tk > 1 || k < 0 || k >= INVEN_PACK) {
				msg_print(Ind, "\377oUsage:     /paint <inventory slot>");
				msg_print(Ind, "\377oExample:   /paint f");
				msg_print(Ind, "\377oWhere the slot must be a potion which determines the colour.");
				return;
			}

			/* Check for a house door next to us */
			for (x = p_ptr->px - 1; x <= p_ptr->px + 1; x++) {
				for (y = p_ptr->py - 1; y <= p_ptr->py + 1; y++) {
					if (!in_bounds(y, x)) continue;
					if (zcave[y][x].feat != FEAT_HOME &&
					    zcave[y][x].feat != FEAT_HOME_OPEN)
					        continue;

					/* found a home door, assume it is a house */
					found = TRUE;
					break;
				}
				if (found) break;
			}
			if (!found) {
				msg_print(Ind, "There is no house next to you.");
				return;
			}

			/* possibly paint it */
			paint_house(Ind, x, y, k);
			return;
		}
#endif
		else if (prefix(message, "/slap")) { /* Slap someone around, as threat :-o */
			cave_type **zcave = getcave(&p_ptr->wpos);
			if (!tk) {
				msg_print(Ind, "Usage: /slap <player name>");
				return;
			}
			if (p_ptr->energy < level_speed(&p_ptr->wpos)) return;
			p_ptr->energy -= level_speed(&p_ptr->wpos);

			j = name_lookup_loose(Ind, token[1], FALSE);
			if (!j || !p_ptr->play_vis[j]) return;
			for (i = 1; i <= 9; i++) {
				if (i == 5) continue;
				if (zcave[p_ptr->py + ddy[i]][p_ptr->px + ddx[i]].m_idx == -j) break;
			}
			if (i == 10) {
				msg_print(Ind, "Player is not standing next to you.");
				return;
			}

#ifdef USE_SOUND_2010
			sound_near_site(p_ptr->py, p_ptr->px, &p_ptr->wpos, 0, "hit_whip", "hit", SFX_TYPE_COMMAND, TRUE);
#endif
			msg_format(j, "\377o%s slaps you!", p_ptr->name);
			msg_format_near(j, "\377y%s slaps %s!", p_ptr->name, Players[j]->name);
			return;
		}
		else if (prefix(message, "/pat")) { /* Counterpart to /slap :-p */
			cave_type **zcave = getcave(&p_ptr->wpos);
			if (!tk) {
				msg_print(Ind, "Usage: /pat <player name>");
				return;
			}
			if (p_ptr->energy < level_speed(&p_ptr->wpos)) return;
			p_ptr->energy -= level_speed(&p_ptr->wpos);

			j = name_lookup_loose(Ind, token[1], FALSE);
			if (!j || !p_ptr->play_vis[j]) return;

			for (i = 1; i <= 9; i++) {
				if (i == 5) continue;
				if (zcave[p_ptr->py + ddy[i]][p_ptr->px + ddx[i]].m_idx == -j) break;
			}
			if (i == 10) {
				msg_print(Ind, "Player is not standing next to you.");
				return;
			}

			msg_format(j, "\377o%s pats you.", p_ptr->name);
			msg_format_near(j, "\377y%s pats %s.", p_ptr->name, Players[j]->name);
			return;
		}
		else if (prefix(message, "/hug")) { /* Counterpart to /slap :-p */
			cave_type **zcave = getcave(&p_ptr->wpos);
			if (!tk) {
				msg_print(Ind, "Usage: /hug <player name>");
				return;
			}
			if (p_ptr->energy < level_speed(&p_ptr->wpos)) return;
			p_ptr->energy -= level_speed(&p_ptr->wpos);

			j = name_lookup_loose(Ind, token[1], FALSE);
			if (!j || !p_ptr->play_vis[j]) return;

			for (i = 1; i <= 9; i++) {
				if (i == 5) continue;
				if (zcave[p_ptr->py + ddy[i]][p_ptr->px + ddx[i]].m_idx == -j) break;
			}
			if (i == 10) {
				msg_print(Ind, "Player is not standing next to you.");
				return;
			}

			msg_format(j, "\377o%s hugs you.", p_ptr->name);
			msg_format_near(j, "\377y%s hugs %s.", p_ptr->name, Players[j]->name);
			return;
		}
		else if (prefix(message, "/poke")) {
			cave_type **zcave = getcave(&p_ptr->wpos);
			if (!tk) {
				msg_print(Ind, "Usage: /poke <player name>");
				return;
			}
			if (p_ptr->energy < level_speed(&p_ptr->wpos)) return;
			p_ptr->energy -= level_speed(&p_ptr->wpos);

			j = name_lookup_loose(Ind, token[1], FALSE);
			if (!j || !p_ptr->play_vis[j]) return;

			for (i = 1; i <= 9; i++) {
				if (i == 5) continue;
				if (zcave[p_ptr->py + ddy[i]][p_ptr->px + ddx[i]].m_idx == -j) break;
			}
			if (i == 10) {
				msg_print(Ind, "Player is not standing next to you.");
				return;
			}

			msg_format(j, "\377o%s pokes you.", p_ptr->name);
			msg_format_near(j, "\377y%s pokes %s.", p_ptr->name, Players[j]->name);
			return;
		}

		/*
		 * Admin commands
		 *
		 * These commands should be replaced by LUA scripts in the future.
		 */


		else if (admin) {
			/* presume worldpos */
			switch (tk) {
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

//no effect				if (tk && k == 0) msg_broadcast(0, "\377o** Server is being restarted and will be back immediately! **");

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
				msg_admins(0, "\377y* Shutting down as soon as dungeons are empty *");
				cfg.runlevel = 2048;
				return;
			}
			else if (prefix(message, "/shutlow")) {
				msg_admins(0, "\377y* Shutting down as soon as dungeons are empty and few players are on *");
				cfg.runlevel = 2047;
				return;
			}
			else if (prefix(message, "/shutvlow")) {
				msg_admins(0, "\377y* Shutting down as soon as dungeons are empty and very few players are on *");
				cfg.runlevel = 2046;
				return;
			}
			else if (prefix(message, "/shutnone")) {
				msg_admins(0, "\377y* Shutting down as soon as no players are on anymore *");
				cfg.runlevel = 2045;
				return;
			}
			else if (prefix(message, "/shutactivevlow")) {
				msg_admins(0, "\377y* Shutting down as soon as dungeons are empty and very few players are active *");
				cfg.runlevel = 2044;
				return;
			}
#if 0	/* not implemented yet - /shutempty is currently working this way */
			else if (prefix(message, "/shutsurface")) {
				msg_admins(0, "\377y* Shutting down as soon as noone is inside a dungeon/tower *");
				cfg.runlevel = 2050;
				return;
			}
#endif
			else if (prefix(message, "/shutrec")) {
				if (!k) k = 5;
				timed_shutdown(k);
				return;
			}
			else if (prefix(message, "/shutcancel")) {
				msg_admins(0, "\377w* Shut down cancelled *");
				if (cfg.runlevel == 2043)
					msg_broadcast_format(0, "\377I*** \377yServer-shutdown cancelled. \377I***");
				cfg.runlevel = 6;
				return;
			}
			else if (prefix(message, "/val")){
				if(!tk) return;
				/* added checking for account existance - mikaelh */
				switch(validate(message3)) {
				case -1: msg_format(Ind, "\377GValidating %s", message3);
					break;
				case 0: msg_format(Ind, "\377rAccount %s not found", message3);
					break;
				case 1: msg_format(Ind, "\377rAccount %s already completely valid", message3);
				}
				return;
			}
			else if (prefix(message, "/inval")){
				if(!tk) return;
				/* added checking for account existance - mikaelh */
				switch(invalidate(message3)) {
				case -1: msg_format(Ind, "\377GInvalidating %s", message3);
					break;
				case 0: msg_format(Ind, "\377rAccount %s not found", message3);
					break;
				case 1: msg_format(Ind, "\377rAccount %s already completely invalid", message3);
				}
				return;
			}
			else if (prefix(message, "/makeadmin")){
				if(!tk) return;
				/* added checking for account existance - mikaelh */
				if (makeadmin(message3)) {
					msg_format(Ind, "\377GMaking %s an admin", message3);
				}
				else {
					msg_format(Ind, "\377rAccount %s not found", message3);
				}
				return;
			}
			else if (prefix(message, "/ban"))
			{
				if (tk >= 2)
				{
					j = name_lookup_loose(Ind, token[1], FALSE);
					if (j)
					{
						char kickmsg[MAX_SLASH_LINE_LEN];
						/* Success maybe :) */
						add_banlist(j, (tk>1 ? atoi(token[2]) : 5));
						if (tk < 3)
						{
							msg_format(Ind, "Banning %s for %d minutes...", Players[j]->name, atoi(token[2]));
							snprintf(kickmsg, MAX_SLASH_LINE_LEN, "banned for %d minutes", atoi(token[2]));
							Destroy_connection(Players[j]->conn, kickmsg);
						} else {
							msg_format(Ind, "Banning %s for %d minutes (%s)...", Players[j]->name, atoi(token[2]), token[3]);
							snprintf(kickmsg, MAX_SLASH_LINE_LEN, "Banned for %d minutes (%s)", atoi(token[2]), token[3]);
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
					j = name_lookup_loose(Ind, token[1], FALSE);
					if (j)
					{
						char kickmsg[MAX_SLASH_LINE_LEN];
						/* Success maybe :) */
						if (tk < 2)
						{
							msg_format(Ind, "Kicking %s out...", Players[j]->name);
							Destroy_connection(Players[j]->conn, "kicked out");
						} else {
							msg_format(Ind, "Kicking %s out (%s)...", Players[j]->name, token[2]);
							snprintf(kickmsg, MAX_SLASH_LINE_LEN, "kicked out (%s)", token[2]);
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
                                        j = name_lookup_loose(Ind, token[1], FALSE);
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
                                        j = name_lookup_loose(Ind, token[1], FALSE);
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
				for (i = 1; i <= NumPlayers; i++) {
					if (!inarea(&wp, &Players[i]->wpos)) continue;
					Players[i]->redraw |= PR_MAP;
				}
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
			else if (prefix(message, "/game")){
				if (!tk) {
					msg_print(Ind, "Usage: /game stop   or   /game rugby");
					return;
				} else if (!strcmp(token[1], "rugby")) {
					object_type	forge;
					object_type	*o_ptr = &forge;

					invcopy(o_ptr, lookup_kind(1, 9));
					o_ptr->number = 1;
					o_ptr->name1=0;
					o_ptr->name2=0;
					o_ptr->name3=0;
					o_ptr->pval = 0;
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
				} else if (!strcmp(token[1], "stop")) { /* stop all games - mikaelh */
					char sstr[80];
					msg_broadcast(0, "\377pThe game has been stopped!");
					snprintf(sstr, MAX_CHARS, "Score: \377RReds: %d  \377BBlues: %d", teamscore[0], teamscore[1]);
					msg_broadcast(0, sstr);
					teamscore[0] = 0;
					teamscore[1] = 0;
					teams[0] = 0;
					teams[1] = 0;
					gametype = 0;
					for (k = 1; k <= NumPlayers; k++)
						Players[k]->team = 0;
				}
			}
			else if (prefix(message, "/unstatic-level") ||
			    prefix(message, "/unst")) {
				/* no sanity check, so be warned! */
				master_level_specific(Ind, &wp, "u");
				//				msg_format(Ind, "\377rItems and monsters on %dft is cleared.", k * 50);
				return;
			}
			else if (prefix(message, "/treset")) {
				struct worldpos wpos;
				wpcopy(&wpos, &p_ptr->wpos);
				master_level_specific(Ind, &wp, "u");
				dealloc_dungeon_level(&wpos);
				return;
			}
			else if (prefix(message, "/static-level") ||
			    prefix(message, "/stat")) {
				/* no sanity check, so be warned! */
				master_level_specific(Ind, &wp, "s");
				//				msg_format(Ind, "\377rItems and monsters on %dft is cleared.", k * 50);
				return;
			}
			/* TODO: make this player command (using spells, scrolls etc) */
			else if (prefix(message, "/identify") ||
			    prefix(message, "/id")) {
				identify_pack(Ind);

				/* Combine the pack */
				p_ptr->notice |= (PN_COMBINE);

				/* Window stuff */
				p_ptr->window |= (PW_INVEN | PW_EQUIP);

				return;
			}
			else if (prefix(message, "/artifact") ||
			    prefix(message, "/art")) {
				if (k) {
					if (a_info[k].cur_num) {
						a_info[k].cur_num = 0;
						a_info[k].known = FALSE;
						msg_format(Ind, "Artifact %d is now \377Gfindable\377w.", k);
					} else {
						a_info[k].cur_num = 1;
						a_info[k].known = TRUE;
						msg_format(Ind, "Artifact %d is now \377runfindable\377w.", k);
					}
				}
				else if (tk > 0 && prefix(token[1], "show")) {
					int count = 0;
					for (i = 0; i < MAX_A_IDX ; i++) {
						if (!a_info[i].cur_num || a_info[i].known) continue;

						a_info[i].known = TRUE;
						count++;
					}
					msg_format(Ind, "%d 'found but unknown' artifacts are set as '\377Gknown\377w'.", count);
				} else if (tk > 0 && prefix(token[1], "fix")) {
					int count = 0;
					for (i = 0; i < MAX_A_IDX ; i++) {
						if (!a_info[i].cur_num || a_info[i].known) continue;

						a_info[i].cur_num = 0;
						count++;
					}
					msg_format(Ind, "%d 'found but unknown' artifacts are set as '\377rfindable\377w'.", count);
				} else if (tk > 0 && prefix(token[1], "hack")) {
					int count = 0;
					for (i = 0; i < MAX_A_IDX ; i++) {
						if (!a_info[i].cur_num || !a_info[i].known) continue;

						a_info[i].known = TRUE;
						count++;
					}
					msg_format(Ind, "%d 'found but unknown' artifacts are set as '\377yknown\377w'.", count);
				}
//				else if (tk > 0 && strchr(token[1],'*'))
				else if (tk > 0 && prefix(token[1], "reset!")) {
					for (i = 0; i < MAX_A_IDX ; i++) {
						a_info[i].cur_num = 0;
						a_info[i].known = FALSE;
					}
					msg_print(Ind, "All the artifacts are \377Gfindable\377w!");
				} else if (tk > 0 && prefix(token[1], "ban!")) {
					for (i = 0; i < MAX_A_IDX ; i++) {
						a_info[i].cur_num = 1;
						a_info[i].known = TRUE;
					}
					msg_print(Ind, "All the artifacts are \377runfindable\377w!");
				} else {
					msg_print(Ind, "Usage: /artifact (No. | (show | fix | reset! | ban!)");
				}
				return;
			}
			else if (prefix(message, "/uniques"))
			{
				monster_race *r_ptr;
				if (!tk) {
					msg_print(Ind, "Usage: /uniques (unseen | nonkill)");
					return;
				}

				if (prefix(token[k], "unseen")) {
					for (i = 0; i < MAX_R_IDX - 1 ; i++) {
						r_ptr = &r_info[i];
						if (!(r_ptr->flags1 & RF1_UNIQUE)) continue;
						r_ptr->r_sights = 0;
					}
					msg_print(Ind, "All the uniques are set as '\377onot seen\377'.");
				} else if (prefix(token[k], "nonkill")) {
					monster_race *r_ptr;
					for (i = 0; i < MAX_R_IDX - 1 ; i++) {
						r_ptr = &r_info[i];
						if (!(r_ptr->flags1 & RF1_UNIQUE)) continue;
						r_ptr->r_tkills = 0;
					}
					msg_print(Ind, "All the uniques are set as '\377onever killed\377'.");
				}
				return;
			}
			else if (prefix(message, "/unidisable")) {
				if (k) {
					if (!(r_info[k].flags1 & RF1_UNIQUE)) return;
					if (r_info[k].max_num) {
						r_info[k].max_num = 0;
						msg_format(Ind, "(%d) %s is now \377runfindable\377w.", k, r_name + r_info[k].name);
					} else {
						r_info[k].max_num = 1;
						msg_format(Ind, "(%d) %s is now \377Gfindable\377w.", k, r_name + r_info[k].name);
					}
				} else {
					msg_print(Ind, "Usage: /unidisable <monster_index>");
				}
				return;
			}
			else if (prefix(message, "/uniunkill")) {
				if (k) {
					if (!(r_info[k].flags1 & RF1_UNIQUE)) {
						msg_print(Ind, "That's not a unique monster.");
						return;
					}
					if (!r_info[k].r_tkills) {
						msg_print(Ind, "That unique is already at zero kill count.");
						return;
					}
					r_info[k].r_tkills = 0;
					msg_format(Ind, "(%d) %s kill count reset to \377G0\377w.", k, r_name + r_info[k].name);
				} else {
					msg_print(Ind, "Usage: /uniunkill <monster_index>");
				}
				return;
			}
			else if (prefix(message, "/unicheck")) {
				if (!k || !(r_info[k].flags1 & RF1_UNIQUE)) {
					msg_print(Ind, "Usage: /uninum <unique_monster_index>");
					return;
				}
				msg_format(Ind, "(%d) %s has cur_num/max_num of %d/%d.",
				    k, r_name + r_info[k].name, r_info[k].cur_num, r_info[k].max_num);
				return;
			}
			else if (prefix(message, "/unifix")) {
				if (!k || !(r_info[k].flags1 & RF1_UNIQUE)) {
					msg_print(Ind, "Usage: /unifix <unique_monster_index>");
					return;
				}
				if (!r_info[k].cur_num) {
					msg_print(Ind, "That monster is already at zero cur_num count.");
					return;
				}
				for (i = 1; i < m_max; i++)
					if (m_list[i].r_idx == k) {
						msg_print(Ind, "That monster is currently spawned!");
						return;
					}
				r_info[k].cur_num = 0;
				msg_format(Ind, "(%d) %s has been set to zero cur_num.", k, r_name + r_info[k].name);
				return;
			}
			else if (prefix(message, "/curnumcheck")) {
				int i;
				bool uniques_only = (tk);

				/* First set all to zero */
				for (i = 0; i < MAX_R_IDX; i++) {
					if (uniques_only && !(r_info[i].flags1 & RF1_UNIQUE)) continue;
					j = 0;
					/* Now count how many monsters there are of each race */
					for (k = 1; k < m_max; k++)
						if (m_list[k].r_idx == i) j++;
					if (r_info[i].cur_num != j)
						msg_format(Ind, "(%d) %s mismatch: cur_num %d, real %d.",
						    i, r_name + r_info[i].name, r_info[i].cur_num, j);
				}

				msg_print(Ind, "done.");
				return;
			}
			else if (prefix(message, "/curnumfix")) {
				/* Fix r_info[i].cur_num counts */
				int i;
				bool uniques_only = (tk);

				/* First set all to zero */
				for (i = 0; i < MAX_R_IDX; i++) {
					if (uniques_only && !(r_info[i].flags1 & RF1_UNIQUE)) continue;
					r_info[i].cur_num = 0;
				}

				/* Now count how many monsters there are of each race */
				for (i = 1; i < m_max; i++) {
					if (m_list[i].r_idx && (!uniques_only || (r_info[i].flags1 & RF1_UNIQUE))) {
						r_info[m_list[i].r_idx].cur_num++;
					}
				}

				msg_format(Ind, "cur_num fields fixed for all %s.", uniques_only ? "uniques" : "monsters");
				return;
			}
			else if (prefix(message, "/reload-config") ||
			    prefix(message, "/cfg")) {
				if (tk) {
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

						/* Piece together a 32-bit random seed */
						o_ptr->name3 = rand_int(0xFFFF) << 16;
						o_ptr->name3 += rand_int(0xFFFF);
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
					o_ptr->pval = atoi(token[3]);
				//o_ptr->owner = p_ptr->id;
				o_ptr->level = 1;
				(void)inven_carry(Ind, o_ptr);

				return;
			}
#endif
			else if (prefix(message, "/trap") ||
					prefix(message, "/tr"))
			{
				if (k) {
					wiz_place_trap(Ind, k);
				} else {
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
			else if (prefix(message, "/wizlitex")) {
				wiz_lite_extra(Ind);
				return;
			}
			else if (prefix(message, "/wizlite")) {
				wiz_lite(Ind);
				return;
			}
			else if (prefix(message, "/equip") ||
					prefix(message, "/eq"))
			{
				if (tk) admin_outfit(Ind, k);
//				else admin_outfit(Ind, -1);
				else {
					msg_print(Ind, "usage: /eq (realm no.)");
					msg_print(Ind, "    Mage(0) Pray(1) sorc(2) fight(3) shad(4) hunt(5) psi(6) None(else)");
				}
				p_ptr->au = 50000000;
				p_ptr->skill_points = 9999;
				return;
			}
			else if (prefix(message, "/uncurse") ||
			    prefix(message, "/unc")) {
				remove_all_curse(Ind);
				return;
			}
			/* do a wilderness cleanup */
			else if (prefix(message, "/purgewild")) {
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
				if (tk && prefix(token[1], "owner")) {
					for (i = 0; i < numtowns; i++) {
//						for (k = 0; k < MAX_BASE_STORES - 2; k++)
						for (k = 0; k < max_st_idx; k++) {
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
			else if (prefix(message, "/cheeze")) {
				char path[MAX_PATH_LENGTH];
				object_type *o_ptr;
				for (i = 0; i < o_max; i++) {
					o_ptr = &o_list[i];
					cheeze(o_ptr);
				}

				cheeze_trad_house();

				path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_DATA, "tomenet.log");
				do_cmd_check_other_prepare(Ind, path, "Log File");
				return;
			}
			/* Respawn monsters on the floor
			 * TODO: specify worldpos to respawn */
			else if (prefix(message, "/respawn")) {
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
#if 0 /* until it's needed again */
			else if (prefix(message, "/swap-towns")){
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
#endif
			else if (prefix(message, "/debug-towns")){
				msg_format(Ind, "numtowns = %d", numtowns);
				for (i = 0; i < numtowns; i++) {
					msg_format(Ind, "%d: type = %d, x = %d, y = %d",
					    i, town[i].type, town[i].x, town[i].y);
				}
				return;
			}
			else if (prefix(message, "/debug-stair")){
				/* C. Blue's mad debug code to fix
				   stairs [for entering Mount Doom].. */
				int scx, scy;
				worldpos *tpos = &p_ptr->wpos;
				cave_type **zcave = getcave(tpos);
				if (!(zcave = getcave(tpos))) return;
				for (scx = 0; scx < MAX_WID; scx++) {
					for (scy = 0;scy < MAX_HGT; scy++) {
						if (zcave[scy][scx].feat == FEAT_MORE) {
							zcave[scy][scx].feat = FEAT_FLOOR;
							zcave[p_ptr->py][p_ptr->px].feat = FEAT_MORE;
							new_level_up_y(tpos, p_ptr->py);
							new_level_up_x(tpos, p_ptr->px);
							return;
						}
					}
				}
				msg_print(Ind, "No staircase downwards found.");
				return;
			}
			else if (prefix(message, "/update-dun")){
				/* Reloads dungeon flags from d_info.txt, updating existing
				   dungeons. Note that you have to call this after you made changes
				   to d_info.txt, since dungeons will NOT update automatically.
				   Syntax: /debug-dun    updates dungeon on current worldmap sector.
				           /debug-dun *  updates ALL dungeons. - C. Blue */
				int type, x, y;
#ifdef RPG_SERVER
				bool found_town = FALSE;
#endif
				struct dungeon_type *d_ptr;
				struct worldpos *tpos = &p_ptr->wpos;
				wilderness_type *wild = &wild_info[tpos->wy][tpos->wx];

				/* more mad code to change RPG_SERVER dungeon flags.. */
				for(x = 0; x < (tk ? 64 : 1); x++)
				for(y = 0; y < (tk ? 64 : 1); y++) {
					if (!tk) {tpos = &p_ptr->wpos;}
					else {tpos->wx = x; tpos->wy = y; tpos->wz = 0;}
					wild = &wild_info[tpos->wy][tpos->wx];

					if ((d_ptr = wild->tower)) {
						type = d_ptr->type;

						if (type) {
							d_ptr->flags1 = d_info[type].flags1;
							d_ptr->flags2 = d_info[type].flags2 | DF2_RANDOM;
							d_ptr->baselevel = d_info[type].mindepth;
							d_ptr->maxdepth = d_info[type].maxdepth - d_ptr->baselevel + 1;
						} else {
#if 0 /* don't touch custom dungeons, that might have flags such as ironman or no-entry etc! their flags would get zero'ed here! */
							d_ptr->flags1 = d_info[type].flags1;
							d_ptr->flags2 = d_info[type].flags2;
#else
							continue;
#endif
						}
						//d_ptr->r_char = d_info[type].r_char;
						//d_ptr->nr_char = d_info[type].nr_char;

#ifdef RPG_SERVER /* Make towers harder */
//						d_ptr->flags2 &= ~(DF2_IRON || DF2_IRONFIX1 || DF2_IRONFIX2 || DF2_IRONFIX3 || DF2_IRONFIX4 || 
//							    DF2_IRONRND1 || DF2_IRONRND2 || DF2_IRONRND3 || DF2_IRONRND4) ; /* Reset flags first */
//						if (!(d_info[type].flags1 & DF1_NO_UP))	d_ptr->flags1 &= ~DF1_NO_UP;
						if (!(d_ptr->flags2 & DF2_NO_DEATH)) {
							found_town = FALSE;
						        for(i = 0; i < numtowns; i++) {
	    	        					if(town[i].x == tpos->wx && town[i].y == tpos->wy) {
									found_town = TRUE;
									if (tpos->wx == cfg.town_x && tpos->wy == cfg.town_y) {
										/* exempt training tower since it might be needed for global events */
										continue;
							
										d_ptr->flags2 |= DF2_IRON; /* Barrow-downs only */
									} else {
										d_ptr->flags2 |= DF2_IRON | DF2_IRONFIX2; /* Other towns */
									}
								}
							}
							if (!found_town) {
								/* hack - exempt the Shores of Valinor! */
								if (d_ptr->baselevel != 200)
									d_ptr->flags2 |= DF2_IRON | DF2_IRONRND1; /* Wilderness dungeons */
//									d_ptr->flags1 |= DF1_NO_UP; /* Wilderness dungeons */
							}
						}
#endif
						if(tk) msg_print(Ind, "Tower flags updated.");
					}

					if ((d_ptr = wild->dungeon)) {
						type = d_ptr->type;

						if (type) {
							d_ptr->flags1 = d_info[type].flags1;
							d_ptr->flags2 = d_info[type].flags2 | DF2_RANDOM;
							d_ptr->baselevel = d_info[type].mindepth;
							d_ptr->maxdepth = d_info[type].maxdepth - d_ptr->baselevel + 1;
						} else {
#if 0 /* don't touch custom dungeons, that might have flags such as ironman or no-entry etc! their flags would get zero'ed here! */
							d_ptr->flags1 = d_info[type].flags1;
							d_ptr->flags2 = d_info[type].flags2;
#else
							continue;
#endif
						}

#ifdef RPG_SERVER /* Make dungeons harder */
//						d_ptr->flags2 &= ~(DF2_IRON || DF2_IRONFIX1 || DF2_IRONFIX2 || DF2_IRONFIX3 || DF2_IRONFIX4 || 
//							    DF2_IRONRND1 || DF2_IRONRND2 || DF2_IRONRND3 || DF2_IRONRND4) ; /* Reset flags first */
//						if (!(d_info[type].flags1 & DF1_NO_UP))	d_ptr->flags1 &= ~DF1_NO_UP;
						if (!(d_ptr->flags2 & DF2_NO_DEATH)) {
							found_town = FALSE;
						        for(i = 0; i < numtowns; i++) {
	    	        					if(town[i].x == tpos->wx && town[i].y == tpos->wy) {
									found_town = TRUE;
									if (tpos->wx == cfg.town_x && tpos->wy == cfg.town_y) {
										d_ptr->flags2 |= DF2_IRON; /* Barrow-downs only */
									} else {
										d_ptr->flags2 |= DF2_IRON | DF2_IRONFIX2; /* Other towns */
									}
								}
							}
							if (!found_town) {
								/* hack - exempt the Shores of Valinor! */
								if (d_ptr->baselevel != 200)
									d_ptr->flags2 |= DF2_IRON | DF2_IRONRND1; /* Wilderness dungeons */
//									d_ptr->flags1 |= DF1_NO_UP; /* Wilderness dungeons */
							}
						}
#endif
						if (tk) msg_print(Ind, "Dungeon flags updated.");
					}
				}
				if(!tk) msg_print(Ind, "Dungeon/tower flags updated.");
				return;
			}
			else if (prefix(message, "/debug-pos")){
				/* C. Blue's mad debug code to change player @
				   startup positions in Bree (px, py) */
				new_level_rand_x(&p_ptr->wpos, atoi(token[1]));
				new_level_rand_y(&p_ptr->wpos, atoi(token[2]));
				msg_format(Ind, "Set x=%d, y=%d for this wpos.",atoi(token[1]),atoi(token[2]));
				return;
			}
			else if (prefix(message, "/anotes"))
			{
				int notes = 0;
				for (i = 0; i < MAX_ADMINNOTES; i++) {
					/* search for pending notes of this player */
					if (strcmp(admin_note[i], "")) {
						/* found a matching note */
						notes++;
					}
				}
				if (notes > 0) msg_format(Ind, "\377oAdmins wrote %d currently pending notes:", notes);
				else msg_print(Ind, "\377oNo admin wrote any pending note.");
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
				int notes = 0;
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
						msg_format(Ind, "\377oDeleted note %d.", notes);
					}
				}
				return;
			}
			else if (prefix(message, "/anote")) /* Send a global admin note to everyone */
			{
				j = 0;
				if (tk < 1)	/* Explain command usage */
				{
					msg_print(Ind, "\377oUsage: /anote <text>");
					msg_print(Ind, "\377oUse /danote <message index> to delete a message.");
					msg_print(Ind, "\377oTo clear all pending notes of yours, type: /danote *");
					return;
				}
				/* Search for begin of parms ( == text of the note) */
				for (i = 6; i < (int)strlen(message2); i++)
					if (message2[i] == ' ')
						for (j = i; j < (int)strlen(message2); j++)
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
				for (i = 0; i < MAX_ADMINNOTES; i++)
					if (strcmp(admin_note[i], ""))
						msg_broadcast_format(0, "\377sGlobal Admin Note: %s", admin_note[i]);
				return;
			}
			else if (prefix(message, "/swarn")) /* Send a global server warning everyone */
			{
				j = 0;
				if (tk < 1) {
					strcpy(server_warning, "");
					msg_print(Ind, "Server warning has been cleared.");
					return;
				}
				/* Search for begin of parms ( == text of the note) */
				for (i = 6; i < (int)strlen(message2); i++)
					if (message2[i] == ' ')
						for (j = i; j < (int)strlen(message2); j++)
							if (message2[j] != ' ') {
								/* save start pos in j for latter use */
								i = strlen(message2);
								break;
							}

				strcpy(server_warning, &message2[j]);
				if (server_warning[0]) msg_broadcast_format(0, "\374\377R*** Note: %s ***", server_warning);
				return;
			}
			else if (prefix(message, "/reart")) /* re-roll a random artifact */
			{
				object_type *o_ptr;
				int min_pval = -999, min_ap = -999, tries = 1000;
				int th ,td ,ta; //for retaining jewelry properties in case they get inverted by cursing
				if (tk < 1) {
					msg_print(Ind, "\377oUsage: /reart <inventory-slot> [<+min pval>|<min artifact power>]");
					return;
				}

				if (atoi(token[1]) < 1 || atoi(token[1]) >= INVEN_TOTAL) {
					msg_print(Ind, "\377oInvalid inventory slot.");
					return;
				}

				o_ptr = &p_ptr->inventory[atoi(token[1]) - 1];
				if (o_ptr->name1 != ART_RANDART) {
					if (o_ptr->name1) {
						msg_print(Ind, "\377oIt's a static art. Aborting.");
						return;
					} else {
						msg_print(Ind, "\377oNot a randart - turning it into one.");
						o_ptr->name1 = ART_RANDART;
					}
				}

				if (tk > 1) {
					if (token[2][0] == '+') min_pval = atoi(token[2]);
					else min_ap = atoi(token[2]);
				}

				if (min_ap > -999 && min_pval > -999)
					msg_format(Ind, "\377wrerolling for at least +%d pval and %d ap.", min_pval, min_ap);
				else if (min_pval > -999)
					msg_format(Ind, "\377wrerolling for at least +%d pval.", min_pval);
				else if (min_ap > -999)
					msg_format(Ind, "\377wrerolling for at least %d ap.", min_ap);


				th = o_ptr->to_h; td = o_ptr->to_d; ta = o_ptr->to_a; //for jewelry
#if 0/*no good because on cursing, the stats aren't just inverted but also modified!*/
				//hacking around old curses
				if (th < 0) th = -th;
				if (td < 0) td = -td;
				if (ta < 0) ta = -ta;
#endif

				while (tries) {
					/* Piece together a 32-bit random seed */
					o_ptr->name3 = rand_int(0xFFFF) << 16;
					o_ptr->name3 += rand_int(0xFFFF);
					/* Check the tval is allowed */
					if (randart_make(o_ptr) == NULL) {
						/* If not, wipe seed. No randart today */
						o_ptr->name1 = 0;
						o_ptr->name3 = 0L;
						msg_print(Ind, "Randart creation failed..");
						return;
					}
					o_ptr->timeout = 0;
					apply_magic(&p_ptr->wpos, o_ptr, p_ptr->lev, FALSE, FALSE, FALSE, FALSE, FALSE);

					if (o_ptr->pval >= min_pval && artifact_power(randart_make(o_ptr)) >= min_ap) break;
					tries--;
				}
				if (!tries) msg_format(Ind, "Re-rolling failed (out of tries (1000))!");
				else msg_format(Ind, "Re-rolled randart in inventory slot %d (Tries: %d).", atoi(token[1]), 1000 + 1 - tries);
				if (o_ptr->tval == TV_RING || o_ptr->tval == TV_AMULET) {
					o_ptr->to_a = ta; o_ptr->to_d = td; o_ptr->to_h = th;
				}

				o_ptr->ident |= ID_MENTAL; /* *id*ed */
				p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
				return;
			}
			else if (prefix(message, "/reego")) /* re-roll an ego item */
			{
				object_type *o_ptr;
				if (tk < 1) {
					msg_print(Ind, "\377oUsage: /reego <inventory-slot>");
					return;
				}
				if (atoi(token[1]) < 1 || atoi(token[1]) >= INVEN_TOTAL) {
					msg_print(Ind, "\377oInvalid inventory slot.");
					return;
				}
				o_ptr = &p_ptr->inventory[atoi(token[1]) - 1];
				if (!o_ptr->name2) {
					msg_print(Ind, "\377oNot an ego item.");
					return;
				}

				o_ptr->timeout=0;
				return;/* see create_reward for proper loop */
				apply_magic(&p_ptr->wpos, o_ptr, p_ptr->lev, TRUE, TRUE, TRUE, FALSE, FALSE);

				msg_format(Ind, "Re-rolled ego in inventory slot %d!", atoi(token[1]));
				/* Window stuff */
				p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
				return;
			}
			/* very dangerous if player is poisoned, very weak, or has hp draining */
			else if (prefix(message, "/threaten") || prefix(message, "/thr")) { /* Nearly kill someone, as threat >:) */
				j = name_lookup_loose(Ind, token[1], FALSE);
				if (!tk) {
					msg_print(Ind, "Usage: /threaten <player name>");
					return;
				}
				if (!j) return;
				msg_format(Ind, "\377yThreatening %s.", Players[j]->name);
				msg_format_near(j, "\377y%s is hit by a bolt from the blue!", Players[j]->name);
				msg_print(j, "\377rYou are hit by a bolt from the blue!");
				bypass_invuln = TRUE;
				take_hit(j, Players[j]->chp - 1, "", 0);
				bypass_invuln = FALSE;
				msg_print(j, "\377rThat was close huh?!");
				return;
			}
			else if (prefix(message, "/aslap")) { /* Slap someone around, as threat :-o */
				if (!tk) {
					msg_print(Ind, "Usage: /slap <player name>");
					return;
				}
				j = name_lookup_loose(Ind, token[1], FALSE);
				if (!j) return;
#ifdef USE_SOUND_2010
				sound_near_site(Players[j]->py, Players[j]->px, &p_ptr->wpos, 0, "hit_whip", "hit", SFX_TYPE_COMMAND, TRUE);
#endif
				msg_format(Ind, "\377ySlapping %s.", Players[j]->name);
				msg_print(j, "\377rYou are slapped by something invisible!");
				msg_format_near(j, "\377y%s is slapped by something invisible!", Players[j]->name);
				bypass_invuln = TRUE;
				take_hit(j, Players[j]->chp / 2, "", 0);
				bypass_invuln = FALSE;
				return;
			}
			else if (prefix(message, "/apat")) { /* Counterpart to /slap :-p */
				if (!tk) {
					msg_print(Ind, "Usage: /pat <player name>");
					return;
				}
				j = name_lookup_loose(Ind, token[1], FALSE);
				if (!j) return;
				msg_format(Ind, "\377yPatting %s.", Players[j]->name);
				msg_print(j, "\377yYou are patted by something invisible.");
				msg_format_near(j, "\377y%s is patted by something invisible.", Players[j]->name);
				return;
			}
			else if (prefix(message, "/ahug")) { /* Counterpart to /slap :-p */
				if (!tk) {
					msg_print(Ind, "Usage: /hug <player name>");
					return;
				}
				j = name_lookup_loose(Ind, token[1], FALSE);
				if (!j) return;
				msg_format(Ind, "\377yHugging %s.", Players[j]->name);
				msg_print(j, "\377yYou are hugged by something invisible.");
				msg_format_near(j, "\377y%s is hugged by something invisible.", Players[j]->name);
				return;
			}
			else if (prefix(message, "/apoke")) {
				if (!tk) {
					msg_print(Ind, "Usage: /poke <player name>");
					return;
				}
				j = name_lookup_loose(Ind, token[1], FALSE);
				if (!j) return;
				msg_format(Ind, "\377yPoking %s.", Players[j]->name);
				msg_print(j, "\377yYou are poked by something invisible.");
				msg_format_near(j, "\377y%s is being poked by something invisible.", Players[j]->name);
				return;
			}
			else if (prefix(message, "/strangle")) {/* oO */
				if (!tk) {
					msg_print(Ind, "Usage: /strangle <player name>");
					return;
				}
				j = name_lookup_loose(Ind, token[1], FALSE);
				if (!j) return;
				msg_format(Ind, "\377yPoking %s.", Players[j]->name);
				msg_print(j, "\377yYou are being strangled by something invisible!");
				msg_format_near(j, "\377y%s is being strangled by something invisible!", Players[j]->name);
				bypass_invuln = TRUE;
				take_hit(j, Players[j]->chp / 4, "", 0);
				set_stun(j, Players[j]->stun + 5);
				bypass_invuln = FALSE;
				return;
			}
			else if (prefix(message, "/cheer")) {
				if (!tk) {
					msg_print(Ind, "Usage: /cheer <player name>");
					return;
				}
				j = name_lookup_loose(Ind, token[1], FALSE);
				if (!j) return;
				msg_print(j, "\377ySomething invisible is cheering for you!");
				msg_format_near(j, "\377yYou hear something invisible cheering for %s!", Players[j]->name);
				Players[j]->blessed_power = 10;
				set_blessed(j, randint(5) + 15);
				return;
			}
			else if (prefix(message, "/applaud")) {
				if (!tk) {
					msg_print(Ind, "Usage: /applaud <player name>");
					return;
				}
				j = name_lookup_loose(Ind, token[1], FALSE);
				if (!j) return;
				msg_format(Ind, "\377yApplauding %s.", Players[j]->name);
				msg_print(j, "\377ySomeone invisible is applauding for you!");
				msg_format_near(j, "\377yYou hear someone invisible applauding for %s!", Players[j]->name);
				set_hero(j, randint(5) + 15);
				return;
			}
			else if (prefix(message, "/presence")) {
				if (!tk) {
					msg_print(Ind, "Usage: /presence <player name>");
					return;
				}
				j = name_lookup_loose(Ind, token[1], FALSE);
				if (!j) return;
				msg_print(j, "\377yYou feel an invisible presence watching you!");
				msg_format_near(j, "\377yYou feel an invisible presence near %s!", Players[j]->name);
				return;
			}
			else if (prefix(message, "/snicker")) {
				if (!tk) {
					msg_print(Ind, "Usage: /snicker <player name>");
					return;
				}
				j = name_lookup_loose(Ind, token[1], FALSE);
				if (!j) return;
				msg_format(Ind, "\377ySnickering at %s.", Players[j]->name);
				msg_print(j, "\377yYou hear someone invisible snickering evilly!");
				msg_format_near(j, "\377yYou hear someone invisible snickering evilly near %s!", Players[j]->name);
				set_afraid(j, Players[j]->afraid + 6);
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
				int c = 0;
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
				int c = 0;
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
				int cp = 0, cy = 0, cg = 0;
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
				msg_format(Ind, "Rerolled HP for %s.", token[1]);
				return;
			}
			/* Reroll a player's HP a lot and measure */
			else if (prefix(message, "/roll!char")) {
				int p, min = 9999, max = 0;
				long avg = 0;
				if (tk < 1) {
					msg_print(Ind, "\377oUsage: /roll!char <player name>");
					return;
				}
				p = name_lookup_loose(Ind, token[1], FALSE);
				if (!p) return;
				for (i = 0; i < 10000; i++) {
					lua_recalc_char(p);
					if (Players[p]->mhp > max) max = Players[p]->mhp;
					if (Players[p]->mhp < min) min = Players[p]->mhp;
					avg += Players[p]->mhp;
				}
				avg /= 10000;
				msg_format(Ind, "Rerolled HP for %s 10000 times:", token[1]);
				msg_format(Ind, "  Min: %d, Max: %d, Avg: %d.", min, max, avg);
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
				teleport_player_force(p, 10);
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
				teleport_player_force(p, 100);
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
				char o_name[ONAME_LEN];
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
						if (tmpm & MODE_EVERLASTING) strcpy(colour_sequence, "\377B");
						else if (tmpm & MODE_PVP) strcpy(colour_sequence, format("\377%c", COLOUR_MODE_PVP));
						else if (tmpm & MODE_NO_GHOST) strcpy(colour_sequence, "\377D");
						else if (tmpm & MODE_HARD) strcpy(colour_sequence, "\377s");
						else strcpy(colour_sequence, "\377W");
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
				if (tk < 1) {
					msg_print(Ind, "Usage: /loadmap t_<mapname>.txt");
					return;
				}
				msg_print(Ind, "Trying to load map..");
//				process_dungeon_file(format("t_%s.txt", message3), &p_ptr->wpos, &ystart, &xstart, 20+1, 32+34, TRUE);
				process_dungeon_file(format("t_%s.txt", message3), &p_ptr->wpos, &ystart, &xstart, MAX_HGT, MAX_WID, TRUE);
				wpos_apply_season_daytime(&p_ptr->wpos, getcave(&p_ptr->wpos));
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
							msg_format(Ind, "Monster #%d is holding an invalid item (o_idx=%d) (k_idx=%d)!", i, m_ptr->hold_o_idx, o_ptr->k_idx);
						}

						if (o_ptr->held_m_idx != i)
						{
							msg_format(Ind, "Item (o_idx=%d) has wrong held_m_idx! (is %d, should be %d)", this_o_idx, i);
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
			/* rename a certain player character file */
			else if (prefix(message, "/renamechar"))
			{
				if (tk < 1) {
					msg_print(Ind, "Usage: /renamechar <character name>:<new name>");
					return;
				}
				msg_format(Ind, "Renaming character %s.", message3);
				rename_player_name(message3);
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
					msg_print(Ind, "No such event.");
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
					msg_print(Ind, "No such event.");
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
				/* (or optionally specfiy new remaining announce time in seconds) */
			{
				int t = 60;
				if (tk < 1 || k < 1 || k > MAX_GLOBAL_EVENTS) {
					msg_format(Ind, "Usage: /geretime 1..%d [<new T-x>]", MAX_GLOBAL_EVENTS);
					return;
				}
				if (global_event[k-1].getype == GE_NONE) {
					msg_print(Ind, "No such event.");
					return;
				}
				if (tk == 2) t = atoi(token[2]);
				/* only if announcement phase isn't over yet, we don't want to mess up a running event */
				if ((turn - global_event[k-1].start_turn) / cfg.fps < global_event[k-1].announcement_time) {
					global_event[k-1].announcement_time = (turn - global_event[k-1].start_turn) / cfg.fps + t;
					announce_global_event(k-1);
				}
				return;
			}
			else if (prefix(message, "/gefforward")) /* skip some running time - C. Blue */
			/* (use negative parameter to go back in time) (in seconds) */
			{
				int t = 60;
				if (tk < 1 || k < 1 || k > MAX_GLOBAL_EVENTS) {
					msg_format(Ind, "Usage: /gefforward 1..%d [<new T-x>]", MAX_GLOBAL_EVENTS);
					return;
				}
				if (global_event[k-1].getype == GE_NONE) {
					msg_print(Ind, "No such event.");
					return;
				}
				if (tk == 2) t = atoi(token[2]);

				/* fix time overflow if set beyond actual end time */
				if (global_event[k-1].end_turn &&
				    (turn + t * cfg.fps >= global_event[k-1].end_turn)) { /* end at 1 turn before actual end for safety */
					t = global_event[k-1].end_turn - turn - 1;
				}

				/* dance the timewarp */
				global_event[k-1].start_turn = global_event[k-1].start_turn - cfg.fps * t;
				if (global_event[k-1].end_turn)
					global_event[k-1].end_turn = global_event[k-1].end_turn - cfg.fps * t;
				return;
			}
			else if (prefix(message, "/partydebug"))
			{
				FILE *fp;
				fp = fopen("tomenet_parties", "wb");
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
			/* delete current highscore completely */
			else if (prefix(message, "/highscorereset")) {
				(void)highscore_reset(Ind);
				return;
			}
			/*
			 * remove an entry from the high score file
			 * required for restored chars that were lost to bugs - C. Blue :/
			*/
			else if (prefix(message, "/highscorerm")) {
				if (tk < 1 || k < 1 || k > MAX_HISCORES) {
					msg_format(Ind, "Usage: /hiscorerm 1..%d", MAX_HISCORES);
					return;
				}
				(void)highscore_remove(Ind, k - 1);
				return;
			}
			/* convert current highscore file to new format */
			else if (prefix(message, "/highscorecv")) {
				(void)highscore_file_convert(Ind);
				return;
			}
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
				if (k < 1 || k > INVEN_TOTAL) {
					msg_format(Ind, "Inventory slot must be between 1 and %d", INVEN_TOTAL);
					return; /* invalid inventory slot index */
				}
				k--; /* start at index 1, easier for user */
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
					j_ptr->mode = 0;
					j_ptr->held_m_idx = m_idx;
					j_ptr->next_o_idx = m_ptr->hold_o_idx;
					m_ptr->hold_o_idx = o_idx;
#if 1 /* only transfer 1 item instead of a whole stack? */
					j_ptr->number = 1;
					inven_item_increase(Ind, k, -1);
#else
					inven_item_increase(Ind, k, -j_ptr->number);
#endif
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
			else if (prefix(message, "/unown")) { /* clear owner of an item - C. Blue */
				object_type *o_ptr;
				if (!tk) {
					msg_print(Ind, "No inventory slot specified.");
					return; /* no inventory slot specified */
				}
				if (k < 1 || k > INVEN_TOTAL) {
					msg_format(Ind, "Inventory slot must be between 1 and %d", INVEN_TOTAL);
					return; /* invalid inventory slot index */
				}
				k--; /* start at index 1, easier for user */
				if (!p_ptr->inventory[k].tval) {
					msg_print(Ind, "Specified inventory slot is empty.");
					return; /* inventory slot empty */
				}
				o_ptr = &p_ptr->inventory[k];
				o_ptr->owner = 0;
				o_ptr->mode = 0;
				p_ptr->window |= PW_INVEN;
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
			else if (prefix(message, "/olistcheck")) {
				object_type *o_ptr;
				msg_print(Ind, "Check o_list for invalid items...");
				for (i = 0; i < o_max; i++) {
					o_ptr = &o_list[i];

					if (!o_ptr->k_idx || o_ptr->k_idx == 1) {
						if (o_ptr->held_m_idx) {
							msg_format(Ind, "Invalid item (o_idx=%d) (k_idx=%d) held by monster %d", i, o_ptr->k_idx, o_ptr->held_m_idx);
						}
						else {
							msg_format(Ind, "Invalid item (o_idx=%d) (k_idx=%d) found at (%d,%d,%d) (x=%d,y=%d)", i, o_ptr->k_idx, o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz, o_ptr->ix, o_ptr->iy);
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
								msg_format(Ind, "Non-existent object (o_idx > o_max) (o_idx=%d, o_max=%d) found at (x=%d,y=%d)", c_ptr->o_idx, o_max, x, y);
								continue;
							}
							o_ptr = &o_list[o_idx];
							if (memcmp(&o_ptr->wpos, &wpos, sizeof(struct worldpos)) != 0 || x != o_ptr->ix || y != o_ptr->iy) {
								msg_format(Ind, "Invalid reference found! Item (o_idx=%d) that should be at (%d, %d, %d) (x=%d, y=%d)", o_idx, o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz, o_ptr->ix, o_ptr->iy);
								msg_format(Ind, "Invalid reference is located at (%d, %d, %d) (x=%d, y=%d)", wpos.wx, wpos.wy, wpos.wz, x, y);
							}
							/* k_idx = 1 is something weird... */
							if (!o_ptr->k_idx || o_ptr->k_idx == 1) {
								msg_format(Ind, "Invalid item (o_idx=%d) (k_idx=%d) found at (x=%d,y=%d)", o_idx, o_ptr->k_idx, x, y);
							}
							/* more objects on this grid? */
							while (o_ptr->next_o_idx) {
								o_idx = o_ptr->next_o_idx;
								if (o_idx > o_max) {
									msg_format(Ind, "Non-existent object (o_idx > o_max) (o_idx=%d) found under a pile at (x=%d,y=%d)", c_ptr->o_idx, x, y);
									break;
								}
								o_ptr = &o_list[o_idx];
								if (memcmp(&o_ptr->wpos, &wpos, sizeof(struct worldpos)) != 0 || x != o_ptr->ix || y != o_ptr->iy) {
									msg_format(Ind, "Invalid reference found! Item (o_idx=%d) that should be at (%d, %d, %d) (x=%d, y=%d)", o_idx, o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz, o_ptr->ix, o_ptr->iy);
									msg_format(Ind, "Invalid reference is located under a pile at (%d, %d, %d) (x=%d, y=%d)", wpos.wx, wpos.wy, wpos.wz, x, y);
								}
								if (!o_ptr->k_idx || o_ptr->k_idx == 1) {
									msg_format(Ind, "Invalid item (o_idx=%d) (k_idx=%d) found under a pile at (x=%d,y=%d)", o_idx, o_ptr->k_idx, x, y);
								}
							}
						}
					}
				}
				msg_print(Ind, "Check complete.");
				return;
			}
			/* attempt to remove problematic items - mikaelh */
			else if (prefix(message, "/floorfix")) {
				struct worldpos wpos;
				cave_type **zcave, *c_ptr;
				object_type *o_ptr, *prev_o_ptr;
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
				msg_format(Ind, "Fixing (%d,%d,%d)...", wpos.wx, wpos.wy, wpos.wz);
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
								msg_format(Ind, "Erased reference to a non-existent (o_idx > o_max) object (o_idx=%d, o_max=%d) found at (x=%d,y=%d)", c_ptr->o_idx, o_max, x, y);
								c_ptr->o_idx = 0;
								continue;
							}
							o_ptr = &o_list[o_idx];
							if (memcmp(&o_ptr->wpos, &wpos, sizeof(struct worldpos)) != 0 || x != o_ptr->ix || y != o_ptr->iy) {
								msg_format(Ind, "Invalid reference erased! Found item (o_idx=%d) that should be at (%d, %d, %d) (x=%d, y=%d)", o_idx, o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz, o_ptr->ix, o_ptr->iy);
								msg_format(Ind, "Invalid reference was located at (%d, %d, %d) (x=%d, y=%d)", wpos.wx, wpos.wy, wpos.wz, x, y);
								c_ptr->o_idx = 0;
							}
							/* k_idx = 1 is something weird... */
							else if (!o_ptr->k_idx || o_ptr->k_idx == 1) {
								msg_format(Ind, "Removed an invalid item (o_idx=%d) (k_idx=%d) found at (x=%d,y=%d)", o_idx, o_ptr->k_idx, x, y);
								delete_object_idx(o_idx, FALSE);
							}
							prev_o_ptr = NULL;
							/* more objects on this grid? */
							while (o_ptr->next_o_idx) {
								o_idx = o_ptr->next_o_idx;
								if (o_idx > o_max) {
									msg_format(Ind, "Erased an invalid reference (o_idx > o_max) (o_idx=%d) from a pile at (x=%d,y=%d)", c_ptr->o_idx, x, y);
									o_ptr->next_o_idx = 0;
									break;
								}
								prev_o_ptr = o_ptr;
								o_ptr = &o_list[o_idx];
								if (memcmp(&o_ptr->wpos, &wpos, sizeof(struct worldpos)) != 0 || x != o_ptr->ix || y != o_ptr->iy) {
									msg_format(Ind, "Invalid reference erased! Found item (o_idx=%d) that should be at (%d, %d, %d) (x=%d, y=%d)", o_idx, o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz, o_ptr->ix, o_ptr->iy);
									msg_format(Ind, "Invalid reference was located under a pile at (%d, %d, %d) (x=%d, y=%d)", wpos.wx, wpos.wy, wpos.wz, x, y);
									if (prev_o_ptr) prev_o_ptr->next_o_idx = o_ptr->next_o_idx;
								}
								else if (!o_ptr->k_idx || o_ptr->k_idx == 1) {
									msg_format(Ind, "Removed an invalid item (o_idx=%d) (k_idx=%d) from a pile at (x=%d,y=%d)", o_idx, o_ptr->k_idx, x, y);
									delete_object_idx(o_idx, FALSE);
								}
							}
						}
					}
				}
				msg_print(Ind, "Everything fixed.");
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
				if (!tk) {
					msg_print(Ind, "Usage: /reward <player name>");
					return;
				}
				j = name_lookup_loose(Ind, token[1], FALSE);
				if (!j) return;
			        msg_print(j, "\377GYou have been rewarded by the gods!");

//				create_reward(Ind, o_ptr, 1, 100, TRUE, TRUE, make_resf(Players[j]) | RESF_NOHIDSM, 5000);
				give_reward(Ind, RESF_LOW2, NULL, 0, 100);
				return;
			}
			else if (prefix(message, "/debug1")) { /* debug an issue at hand */
				for (j = INVEN_TOTAL; j >= 0; j--)
	                                if (p_ptr->inventory[j].tval == TV_AMULET && p_ptr->inventory[j].sval == SV_AMULET_HIGHLANDS)
    		                                invcopy(&p_ptr->inventory[j], lookup_kind(TV_AMULET, SV_AMULET_HIGHLANDS2));
				p_ptr->update |= (PU_BONUS | PU_VIEW);
				handle_stuff(Ind);
				msg_print(Ind, "debug1");
				return;
			}
			else if (prefix(message, "/debug2")) { /* debug an issue at hand */
				for (j = INVEN_TOTAL - 1; j >= 0; j--)
	                                if (p_ptr->inventory[j].tval == TV_AMULET && p_ptr->inventory[j].sval == SV_AMULET_HIGHLANDS) {
    		                                invcopy(&p_ptr->inventory[j], lookup_kind(TV_AMULET, SV_AMULET_HIGHLANDS2));
    		                                p_ptr->inventory[j].number = 1;
    		                                p_ptr->inventory[j].level = 0;
    		                                p_ptr->inventory[j].discount = 0;
    		                                p_ptr->inventory[j].ident |= ID_MENTAL;
    		                                p_ptr->inventory[j].owner = p_ptr->id;
    		                                p_ptr->inventory[j].mode = p_ptr->mode;
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
				int h = (turn % DAY) / HOUR;
				turn -= (turn % DAY) % HOUR;
				if (h < SUNRISE) turn += (SUNRISE - h) * HOUR - 1;
				else if (h < NIGHTFALL) turn += (NIGHTFALL - h) * HOUR - 1;
				else turn += (24 + SUNRISE - h) * HOUR - 1;
				return;
			}
			else if (prefix(message, "/season")) { /* switch through 4 seasons */
				if (tk >= 1) {
					if (k < 0 || k > 3) {
						msg_print(Ind, "Usage: /season [0..3]");
						return;
					}
					season_change(k, FALSE);
				}
				else season_change((season + 1) % 4, FALSE);
				return;
			}
			else if (prefix(message, "/weather")) { /* toggle snowfall during WINTER_SEASON */
#ifdef CLIENT_SIDE_WEATHER
				if (tk) {
					if (k) weather = 0;
					else if (season == SEASON_WINTER) weather = 2;
					else weather = 1;
				}
				weather_duration = 0;
#else
				if (tk >= 1) weather = k;
				else if (weather == 1) weather = 0;
				else {
					weather = 1;
					weather_duration = 60 * 4; /* 4 minutes */
				}
#endif
				return;
			}
			/* toggle own client-side weather for testing purpose:
			   This overrides regular weather, to reenable use
			   /cweather -1 x x x.
			   Syntax: Type, Wind, WEATHER_GEN_TICKS, Intensity, Speed.
			   To turn on rain: 1 0 3 8 3 */
			else if (prefix(message, "/cweather") || prefix(message, "/cw")) {
				if (k == -1) p_ptr->custom_weather = FALSE;
				else p_ptr->custom_weather = TRUE;
				/* Note: 'cloud'-shaped restrictions don't apply
				   if there are no clouds in the current sector.
				   In that case, the weather will simply fill the
				   whole worldmap sector.
				   revoke_clouds is TRUE here, so it'll be all-sector. */
				Send_weather(Ind,
				    atoi(token[1]), atoi(token[2]), atoi(token[3]), atoi(token[4]), atoi(token[5]),
				    FALSE, TRUE);
				return;
			}
			else if (prefix(message, "/jokeweather")) {//unfinished
				if (!k || k > NumPlayers) return;
				if (Players[k]->joke_weather == 0) {
					/* check clouds from first to last (so it has a good chance of
					   getting into the 10 that will actually be transmitted) */
					for (i = 0; i < MAX_CLOUDS; i++) {
						/* assume that 'permanent' clouds are set only by us (might change)
						   and don't abuse those, since they are already reserved for a player ;) */
						if (cloud_dur[i] != -1) {
							Players[k]->joke_weather = i + 1; /* 0 is reserved for 'disabled' */
							cloud_dur[i] = -1;

							/* set cloud parameters */
							cloud_x1[i] = Players[k]->px - 1;
							cloud_y1[i] = Players[k]->py;
							cloud_x2[i] = Players[k]->px + 1;
							cloud_y2[i] = Players[k]->py;
							cloud_state[i] = 1;
							cloud_dsum[i] = 7;
							cloud_xm100[i] = 0;
							cloud_ym100[i] = 0;
							cloud_mdur[i] = 200;

							/* send new situation to everyone */
#if 0
							wild_info[Players[k]->wpos.wy][Players[k]->wpos.wx].weather_type = (season == SEASON_WINTER ? 2 : 1);
							wild_info[Players[k]->wpos.wy][Players[k]->wpos.wx].weather_updated = TRUE;
#else
							Send_weather(Ind,
							    1, 0, 3, 8, 3,
							    TRUE, FALSE);
#endif
							break;
						}
					}
				} else {
					/* run out (and thereby get freed) next turn */
					cloud_dur[Players[k]->joke_weather - 1] = 1;
					Players[k]->joke_weather = 0;
				}
				return;
			}
			else if (prefix(message, "/fireworks")) { /* toggle fireworks during NEW_YEARS_EVE */
				if (tk >= 1) {
					fireworks = k;
				}
				else if (fireworks) fireworks = 0;
				else fireworks = 1;
				return;
			}
			else if (prefix(message, "/hostilities")) {
				player_list_type *ptr;

				for (i = 1; i <= NumPlayers; i++) {
					ptr = Players[i]->hostile;

					while (ptr) {
						if (player_list_find(Players[i]->blood_bond, ptr->id)) {
							msg_format(Ind, "%s is hostile towards %s. (blood bond)", p_ptr->name, lookup_player_name(ptr->id));
						}
						else {
							msg_format(Ind, "%s is hostile towards %s.", p_ptr->name, lookup_player_name(ptr->id));
						}
					}

					ptr = ptr->next;
				}

				msg_print(Ind, "\377sEnd of hostility list.");
				return;
			}
			else if (prefix(message, "/debugstore")) { /* parameter is # of maintenance runs to perform at once (1..10) */
				if (tk > 0) {
					if (!store_debug_mode) store_debug_startturn = turn;

					/* reset maintenance-timer for cleanness */
    					for (i = 0; i < numtowns; i++)
	    				for (j = 0; j < max_st_idx; j++)
				                town[i].townstore[j].last_visit = turn;

 					store_debug_mode = k;
					
					if (tk > 1) store_debug_quickmotion = atoi(token[2]);
					else store_debug_quickmotion = 10;

					s_printf("STORE_DEBUG_MODE: freq %d, time x%d at %s (%d).\n", store_debug_mode, store_debug_quickmotion, showtime(), turn - store_debug_startturn);
				}
				msg_format(Ind, "store_debug_mode: freq %d, time x%d.", store_debug_mode, store_debug_quickmotion);
				return;
			}
			else if (prefix(message, "/costs")) { /* shows monetary details about an object */
				object_type *o_ptr;
				char o_name[ONAME_LEN];
				if (tk < 1)
				{
					msg_print(Ind, "\377oUsage: /costs <inventory-slot>");
					return;
				}
				if (atoi(token[1]) < 1 || atoi(token[1]) >= INVEN_TOTAL) {
					msg_print(Ind, "\377oInvalid inventory slot.");
					return;
				}
				o_ptr = &p_ptr->inventory[atoi(token[1]) - 1];
				object_desc(Ind, o_name, o_ptr, FALSE, 0);
				msg_format(Ind, "Overview for item %s in slot %d:",
				    o_name, atoi(token[1]));
				msg_format(Ind, "Flag cost: %d; for artifact: %d.",
				    flag_cost(o_ptr, o_ptr->pval), artifact_flag_cost(o_ptr, o_ptr->pval));
				msg_format(Ind, "Your value: %d.", object_value(Ind, o_ptr));
				msg_format(Ind, "Your real value: %d; for artifact: %d.",
				    object_value_real(Ind, o_ptr), artifact_value_real(Ind, o_ptr));
				msg_format(Ind, "Full real value: %d; for artifact: %d.",
				    object_value_real(0, o_ptr), artifact_value_real(0, o_ptr));
				msg_format(Ind, "Discount %d; aware? %d; known? %d; broken? %d.",
				    o_ptr->discount, object_aware_p(Ind, o_ptr), object_known_p(Ind, o_ptr), broken_p(o_ptr));
				return;
			}
			/* just calls cron_24h as if it was time to do so */
			else if (prefix(message, "/debugdate")) {
				int dwd, dd, dm, dy;
				get_date(&dwd, &dd, &dm, &dy);
				exec_lua(0, format("cron_24h(\"%s\", %d, %d, %d, %d, %d, %d, %d)", showtime(), 0, 0, 0, dwd, dd, dm, dy));
				return;
			}
			/* copy an object from someone's inventory into own inventory */
			else if (prefix(message, "/ocopy")) {
				object_type forge, *o_ptr = &forge;
				if (tk < 2) return;
#if 0
				/* syntax: /ocopy <name> <0..37> - bad idea */
				j = name_lookup_loose(Ind, token[1], FALSE);
				if (!j) return;
				object_copy(o_ptr, &Players[j]->inventory[atoi(token[2])]);
#else
				/* syntax: /ocopy <1..38> <name> */
				j = name_lookup_loose(Ind, strstr(message3, " ") + 1, FALSE);
				if (!j) return;
				object_copy(o_ptr, &Players[j]->inventory[atoi(token[1]) - 1]);
#endif
				/* skip true arts to prevent duplicates */
				if (true_artifact_p(o_ptr)) {
					if (!multiple_artifact_p(o_ptr)) return;
					handle_art_inum(o_ptr->name1);
				}
				inven_carry(Ind, o_ptr);
				return;
			}
			/* re-initialize the skill chart */
			else if (prefix(message, "/fixskills")) {
				if (tk < 1) return;
				j = name_lookup_loose(Ind, token[1], FALSE);
				if (j < 1) return;
				lua_fix_skill_chart(j);
				return;
			}
			/* debug-hack: set all items within houses to ITEM_REMOVAL_HOUSE - C. Blue
			   warning: can cause a pause of serious duration >:) */
			else if (prefix(message, "/debugitemremovalhouse")) {
				cave_type **zcave;
				object_type *o_ptr;
				j = 0;
				bool sj;
				/* go through all items (well except for player inventories
				   or tradehouses, but that's not needed anyway) */
				for(i = 0; i < o_max; i++){
					o_ptr = &o_list[i];
					/* check unprotected items on the world's surface in CAVE_ICKY locations, ie houses */
					if(o_ptr->k_idx && o_ptr->marked2 == ITEM_REMOVAL_NORMAL && !o_ptr->wpos.wz) {
						/* make sure object's floor is currently allocated so we can test for CAVE_ICKY flag */
						h = 0;
						if (!getcave(&o_ptr->wpos)) {
							alloc_dungeon_level(&o_ptr->wpos);
							h = 1;
							/* relink c_ptr->o_idx with objects */
							wilderness_gen(&o_ptr->wpos);
						}
						if ((zcave = getcave(&o_ptr->wpos))) { /* paranoia? */
							/* monster traps hack */
							sj = FALSE;
							if (!in_bounds_array(o_ptr->iy, o_ptr->ix) &&
							    in_bounds_array(255 - o_ptr->iy, o_ptr->ix)){
								sj = TRUE;
								o_ptr->iy = 255 - o_ptr->iy;
							}
							/* in a house (or vault, theoretically) */
							if (zcave[o_ptr->iy][o_ptr->ix].info & CAVE_ICKY) {
								/* mark item as 'inside house' */
								o_ptr->marked2 = ITEM_REMOVAL_HOUSE;
								/* count for fun */
								j++;
							}
							/* restore monster traps hack */
							if(sj) o_ptr->iy = 255 - o_ptr->iy;
							/* remove our exclusively allocated level again */
							if (h) dealloc_dungeon_level(&o_ptr->wpos);
						} else {
							/* display warning! */
							msg_format(Ind, "WARNING: Couldn't allocate %d,%d,%d.",
							    o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz);
							s_printf("Debugging ITEM_REMOVAL_HOUSE couldn't allocate %d,%d,%d.\n",
							    o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz);
						}
					}
				}
				/* give msg and quit */
				msg_format(Ind, "Set %d items to ITEM_REMOVAL_HOUSE.", j);
				s_printf("Set %d items to ITEM_REMOVAL_HOUSE.\n", j);
				return;
			}
			/* debug-hack: un-perma all death loot or other ITEM_REMOVAL_NEVER
			   items in wilderness, to clean 'lost items' off the object list - C. Blue
			   warning: can cause a pause of serious duration >:)
			   note: also un-permas 'game pieces' and generated items such as
			         cabbage etc. :/ */
			else if (prefix(message, "/purgeitemremovalnever")) {
				cave_type **zcave;
				object_type *o_ptr;
				j = 0;
				bool sj;
				/* go through all items (well except for player inventories
				   or tradehouses, but that's not needed anyway) */
				for(i = 0; i < o_max; i++){
					o_ptr = &o_list[i];
					/* check ITEM_REMOVAL_NEVER items on the world's surface that aren't inside houses */
					if(o_ptr->k_idx && o_ptr->marked2 == ITEM_REMOVAL_NEVER && !o_ptr->wpos.wz) {
						/* make sure object's floor is currently allocated so we can test for CAVE_ICKY flag;
						   this is neccessary for server-generated public house contents for example */
						h = 0;
						if (!getcave(&o_ptr->wpos)) {
							alloc_dungeon_level(&o_ptr->wpos);
							h = 1;
							/* relink c_ptr->o_idx with objects */
							wilderness_gen(&o_ptr->wpos);
						}
						if ((zcave = getcave(&o_ptr->wpos))) { /* paranoia? */
							/* monster traps hack */
							sj = FALSE;
							if (!in_bounds_array(o_ptr->iy, o_ptr->ix) &&
							    in_bounds_array(255 - o_ptr->iy, o_ptr->ix)){
								sj = TRUE;
								o_ptr->iy = 255 - o_ptr->iy;
							}
							/* not in a house (or vault, theoretically) */
							if (!(zcave[o_ptr->iy][o_ptr->ix].info & CAVE_ICKY)) {
								/* remove permanence */
								o_ptr->marked2 = ITEM_REMOVAL_NORMAL;
								/* count for fun */
								j++;
							}
							/* restore monster traps hack */
							if(sj) o_ptr->iy = 255 - o_ptr->iy;
							/* remove our exclusively allocated level again */
							if (h) dealloc_dungeon_level(&o_ptr->wpos);
						} else {
							/* display warning! */
							msg_format(Ind, "WARNING: Couldn't allocate %d,%d,%d.",
							    o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz);
							s_printf("Purging ITEM_REMOVAL_NEVER couldn't allocate %d,%d%d.\n",
							    o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz);
						}
					}
				}
				/* give msg and quit */
				msg_format(Ind, "Set %d items to ITEM_REMOVAL_NORMAL.", j);
				s_printf("Purged ITEM_REMOVAL_NEVER off %d items.\n", j);
				return;
			}
			/* fire up all available status tags in the display to see
			   which space is actually occupied and which is free */
			else if (prefix(message, "/testdisplay")) {
				struct worldpos wpos;
				wpos.wx = 0; wpos.wy = 0; wpos.wz = 200;
				Send_extra_status(Ind, "ABCDEFGHIJKL");
//				Send_depth(Ind, &wpos);
				Send_depth_hack(Ind, &wpos, TRUE, "TOONTOWNoO");
				Send_food(Ind, PY_FOOD_MAX);
				Send_blind(Ind, TRUE);
				Send_confused(Ind, TRUE);
				Send_fear(Ind, TRUE);
				Send_poison(Ind, TRUE);
				Send_state(Ind, TRUE, TRUE, TRUE);
				Send_speed(Ind, 210);
				Send_study(Ind, TRUE);
				Send_cut(Ind, 1001);
				Send_stun(Ind, 101);
				Send_AFK(Ind, 1);
				Send_encumberment(Ind, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1);
				Send_monster_health(Ind, 10, TERM_VIOLET);
				return;
			}
			/* test new \376, \375, \374 chat line prefixes */
			else if (prefix(message, "/testchat")) {
				msg_print(Ind, "No code.");
				msg_print(Ind, "\376376 code.");
				msg_print(Ind, "No code.");
				msg_print(Ind, "\375375 code.");
				msg_print(Ind, "No code.");
				msg_print(Ind, "\374374 code.");
				msg_print(Ind, "No code.");
				msg_print(Ind, "[Chat-style].");
				msg_print(Ind, "No code.");
				msg_print(Ind, "[Chat-style].");
				msg_print(Ind, "~'~' follow-up.");
				msg_print(Ind, "No code.");
				return;
			}
			else if (prefix(message, "/initlua")) {
				msg_print(Ind, "Reinitializing Lua");
				reinit_lua();
				return;
			}
			/* hazardous/incomplete */
			else if (prefix(message, "/reinitarrays")) {
				msg_print(Ind, "Reinitializing some arrays");
				reinit_some_arrays();
				return;
			}
			else if (prefix(message, "/bench")) {
				if (tk < 1) {
					msg_print(Ind, "Usage: /bench <something>");
					msg_print(Ind, "Use on an empty server!");
					return;
				}
				do_benchmark(Ind);
				return;
			}
			else if (prefix(message, "/pings")) {
				struct timeval now;
				player_type *q_ptr;

				if (tk < 1) {
					msg_print(Ind, "Usage: /pings <player>");
					return;
				}

				j = name_lookup_loose(Ind, token[1], FALSE);
				if (!j) {
					msg_format(Ind, "Couldn't find player %s.", token[1]);
					return;
				}
				q_ptr = Players[j];

				gettimeofday(&now, NULL);

				msg_format(Ind, "Last pings for player %s:", q_ptr->name);

				/* Starting from latest */
				for (i = 0; i < MAX_PING_RECVS_LOGGED; i++) {
					j = (q_ptr->pings_received_head - i + MAX_PING_RECVS_LOGGED) % MAX_PING_RECVS_LOGGED;
					if (q_ptr->pings_received[j].tv_sec) {
						msg_format(Ind, " - %s", timediff(&q_ptr->pings_received[j], &now));
					}
				}

				return;
			}
			else if (prefix(message, "/dmpriv")) {
				if (!p_ptr->admin_dm) { // || !cfg.secret_dungeon_master) {
					msg_print(Ind, "Command only available to hidden dungeon masters.");
					return;
				}
				if (p_ptr->admin_dm_chat)
					msg_print(Ind, "You can no longer receive direct private chat from players.");
				else
					msg_print(Ind, "You can now receive direct private chat from players.");
				p_ptr->admin_dm_chat = ~p_ptr->admin_dm_chat;
				return;
			}
			/* unidentifies an item */
			else if (prefix(message, "/unid")) {
				object_type *o_ptr;
				char note2[80], noteid[10];

				if (!tk) {
					msg_print(Ind, "No inventory slot specified.");
					return; /* no inventory slot specified */
				}
				if (k < 1 || k > INVEN_TOTAL) {
					msg_format(Ind, "Inventory slot must be between 1 and %d", INVEN_TOTAL);
					return; /* invalid inventory slot index */
				}
				k--; /* start at index 1, easier for user */
				if (!p_ptr->inventory[k].tval) {
					msg_print(Ind, "Specified inventory slot is empty.");
					return; /* inventory slot empty */
				}
				o_ptr = &p_ptr->inventory[k];
				/* Hack -- Clear the "empty" flag */
				o_ptr->ident &= ~ID_EMPTY;
				/* Hack -- Clear the "known" flag */
				o_ptr->ident &= ~ID_KNOWN;
				/* Hack -- Clear the "felt" flag */
				o_ptr->ident &= ~(ID_SENSE | ID_SENSED_ONCE | ID_MENTAL);
				p_ptr->window |= PW_INVEN;

				/* remove pseudo-id tags too */
				if (o_ptr->note) {
					note_crop_pseudoid(note2, noteid, quark_str(o_ptr->note));
					if (!note2[0]) o_ptr->note = 0;
					else o_ptr->note = quark_add(note2);
				}
				return;
			}
			else if (prefix(message, "/ai")) { /* returns/resets all AI flags and states of monster currently looked at (NOT the one targetted) - C. Blue */
				monster_type *m_ptr;
				int m_idx;
				if (p_ptr->health_who <= 0) {//target_who
					msg_print(Ind, "No monster looked at.");
					return; /* no monster targetted */
				}
				m_idx = p_ptr->health_who;
				m_ptr = &m_list[m_idx];
				if (!tk) msg_print(Ind, "Current Monster AI state:");
				else msg_print(Ind, "Former monster AI state (RESETTING NOW):");
#ifdef ANTI_SEVEN_EXPLOIT
				msg_format(Ind, "  ANTI_SEVEN_EXPLOIT (PD %d, X %d, Y %d, CD %d, DD %d, PX %d, PY %d).",
				    m_ptr->previous_direction, m_ptr->damage_tx, m_ptr->damage_ty, m_ptr->cdis_on_damage, m_ptr->damage_dis, m_ptr->p_tx, m_ptr->p_ty);
				if (tk) {
					m_ptr->previous_direction = 0;
					m_ptr->damage_tx = 0;
					m_ptr->damage_ty = 0;
					m_ptr->cdis_on_damage = 0;
					m_ptr->damage_dis = 0;
					m_ptr->p_tx = 0;
					m_ptr->p_ty = 0;
				}
#endif
				return;
			}
#ifdef USE_SOUND_2010
			else if (prefix(message, "/hmus")) { /* set own music according to the location of someone else */
				u32b f;
				if (tk < 1) {
					msg_print(Ind, "Usage: /hmus <player>");
					return;
				}
				j = name_lookup_loose(Ind, message3, FALSE);
				if (!j) {
					msg_format(Ind, "Couldn't find player %s.", message3);
					return;
				}

				msg_format(Ind, "Using music of player %s.", Players[j]->name);
				f = Players[Ind]->esp_link_flags;
				Players[Ind]->esp_link_flags &= ~LINKF_VIEW_DEDICATED;
				Send_music(Ind, Players[j]->music_current);
				Players[Ind]->esp_link_flags = f;
				return;
			}
#endif
			else if (prefix(message, "/towea")) { /* teleport player to a sector with weather going */
				int x, y;
				for (x = 0; x < 64; x++)
					for (y = 0; y < 64; y++)
						if (wild_info[y][x].weather_type > 0 &&
						    (wild_info[y][x].weather_type == 1 || //rain
						    wild_info[y][x].weather_type == 2) && //snow
						    wild_info[y][x].cloud_idx[0] > 0) {
							p_ptr->recall_pos.wx = x;
							p_ptr->recall_pos.wy = y;
							p_ptr->recall_pos.wz = 0;
							p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
							recall_player(Ind, "");
							return;
						}
				return;
			}
			else if (prefix(message, "/madart_")) { /* try to create a very specific randart - C. Blue */
				/* added this to create a new matching uber bow for Andur, after his old one fell victim to randart code changes. */
				object_type *o_ptr;
				artifact_type *a_ptr;
				int tries = 0; //keep track xD

				o_ptr = &p_ptr->inventory[10 - 1]; //slot 'j'
				if (o_ptr->name1 != ART_RANDART) {
					msg_print(Ind, "\377oNot a randart. Aborting.");
					return;
				}

				msg_print(Ind, "\377yMadness in progress..searching 4E9 combinations :-p");
				handle_stuff(Ind);
				do {
					tries++;
			                /* Piece together a 32-bit random seed */
			                o_ptr->name3 = rand_int(0xFFFF) << 16;
			                o_ptr->name3 += rand_int(0xFFFF);
			                /* Check the tval is allowed */
			                if ((a_ptr = randart_make(o_ptr)) == NULL) {
			                        /* If not, wipe seed. No randart today */
			                        o_ptr->name1 = 0;
				                o_ptr->name3 = 0L;
						msg_print(Ind, "Randart creation failed.");
						return;
					}
					o_ptr->timeout = 0;
					apply_magic(&p_ptr->wpos, o_ptr, p_ptr->lev, FALSE, FALSE, FALSE, FALSE, FALSE);
				} while	(
				    o_ptr->to_h < 30 ||
				    o_ptr->to_d < 29 ||
				    o_ptr->pval < 2 ||
				    !(a_ptr->flags1 & TR1_STEALTH) ||
				    !(a_ptr->flags2 & TR2_FREE_ACT) ||
				    !(a_ptr->flags2 & TR2_RES_COLD) ||
				    !(a_ptr->flags2 & TR2_RES_SHARDS) ||
				    !(a_ptr->flags3 & TR3_XTRA_MIGHT) ||
				    !(a_ptr->flags3 & TR3_XTRA_SHOTS) ||
				    (a_ptr->flags3 & TR3_AGGRAVATE)
				    );
				msg_format(Ind, "Re-rolled randart %d times.", tries);

				o_ptr->ident |= ID_MENTAL; /* *id*ed */
				p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
				return;
			}
#ifdef TEST_SERVER
			else if (prefix(message, "/testdam")) {
				if (tk) {
					p_ptr->test_count = p_ptr->test_dam = p_ptr->test_heal = 0;
					p_ptr->test_turn = turn;
					msg_print(Ind, "test count/dam/heal have been reset.");
				} else {
					if (p_ptr->test_count == 0)
						msg_print(Ind, "test count 0. no result.");
					else
						msg_format(Ind, "test count %d - tot dam %d, avg %d - tot heal %d, avg %d",
						    p_ptr->test_count,
						    p_ptr->test_dam, p_ptr->test_dam / p_ptr->test_count,
						    p_ptr->test_heal, p_ptr->test_heal / p_ptr->test_count);
						msg_format(Ind, "test turns %d - tot dam %d, avg %d - tot heal %d, avg %d",
						    turn - p_ptr->test_turn,
						    p_ptr->test_dam, p_ptr->test_dam / (turn - p_ptr->test_turn),
						    p_ptr->test_heal, p_ptr->test_heal / (turn - p_ptr->test_count));
				}
				return;
			}
#endif
			/* Display all information about the grid an admin-char is currently standing on - C. Blue */
			else if (prefix(message, "/debug-grid")){
				worldpos *tpos = &p_ptr->wpos;
				cave_type **zcave, *c_ptr;
				struct c_special *cs_ptr;
				if (!(zcave = getcave(tpos))) {
					msg_print(Ind, "Fatal: Couldn't acquire zcave!");
					return;
				}
				c_ptr = &zcave[p_ptr->py][p_ptr->px];
				cs_ptr = c_ptr->special;
				msg_format(Ind, "Feature / org    : %d / %d", c_ptr->feat, c_ptr->feat_org);
				msg_format(Ind, "Info flags       : %d", c_ptr->info);
				if (cs_ptr) msg_format(Ind, "1st Special->Type: %d", cs_ptr->type);
				else msg_print(Ind, "1st Special->Type: NONE");
				return;
			}
			/* Display various gameplay-related or rather global server status
			   like seasons and (seasonal) events (for debugging) - C. Blue */
			else if (prefix(message, "/vars")) {
				msg_format(Ind, "season (0..3): %d.", season);
				msg_format(Ind, "season_halloween: %d, season_xmas: %d, season_newyearseve: %d.", season_halloween, season_xmas, season_newyearseve);
				msg_format(Ind, "sector weather: %d, sector_wind: %d.", wild_info[p_ptr->py][p_ptr->px].weather_type, wild_info[p_ptr->py][p_ptr->px].weather_wind);
				msg_format(Ind, "server time: %s", showtime());
				msg_format(Ind, "lua-seen current date: %d-%d-%d", exec_lua(0, "return cur_year"), exec_lua(0, "return cur_month"), exec_lua(0, "return cur_day"));
				return;
			}
			else if (prefix(message, "/debug-wild")) {
//				cptr wf = flags_str(wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags);
				msg_format(Ind, "wild_info[%d][%d]:", p_ptr->wpos.wy, p_ptr->wpos.wx);
				msg_format(Ind, "  terrain: %d", wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].type);
//				msg_format(Ind, "  flags:   %s (%d)", wf, wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags);
//				free(wf);
				msg_format(Ind, "  flags:   %d", wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags);
				msg_format(Ind, "  tower:   %d", wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].tower != NULL ? 1 : 0);
				msg_format(Ind, "  dungeon: %d", wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].dungeon != NULL ? 1 : 0);
				msg_format(Ind, "  town:    %d", wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].town_idx);
				msg_format(Ind, "  townrad: %d", wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].radius);
				return;
			}
			else if (prefix(message, "/fix-wildflock")) {
				/* erase WILD_F_..LOCK flags from when the server crashed during dungeon removal (??) */
				wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags &= ~(WILD_F_LOCKDOWN | WILD_F_LOCKUP);
				return;
			}
			else if (prefix(message, "/fix-house-modes")) {
				/* if house doesn't have its mode set yet, search
				   hash for player who owns it and set mode to his. */
				k = 0;
				for (i = 0; i < num_houses; i++) {
					if (houses[i].dna->owner &&
					    (houses[i].dna->owner_type == OT_PLAYER)) {
						j = lookup_player_mode(houses[i].dna->owner);
						if (j != houses[i].dna->mode) {
							houses[i].dna->mode = j;
							k++;
						}
					}
				}
				msg_format(Ind, "Houses that had their ownership changed: %d.", k);
				return;
			}
#ifdef TEST_SERVER
			else if (prefix(message, "/testreqs")) {
				msg_print(Ind, "Sending request(s)...");
				switch (k) {
				case 0: Send_request_key(Ind, 0, "Key: "); break;
				case 1: Send_request_num(Ind, 0, "Num: ", 9); break;
				case 2: Send_request_str(Ind, 0, "Str: ", "mh"); break;
				case 3: Send_request_cfr(Ind, 0, "Y/N: "); break;
				}
//				Send_request_abort(Ind);
				msg_print(Ind, "...done.");
				return;
			}
#endif
			/* Update 'noteworthy occurances' aka legends.log display, for debugging purposes */
			else if (prefix(message,"/update-leg")) {
				char path[MAX_PATH_LENGTH];
				char path_rev[MAX_PATH_LENGTH];
				path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_DATA, "legends.log");
				path_build(path_rev, MAX_PATH_LENGTH, ANGBAND_DIR_DATA, "legends-rev.log");
				/* create reverse version of the log for viewing,
				   so the latest entries are displayed top first! */
				reverse_lines(path, path_rev);
				msg_print(Ind, "updated legends-rev.log");
				return;
			}
			/* Test values of deep_dive..[] deep dive record saving array */
			else if (prefix(message, "/deepdivestats")) {
				for (i = 0; i < 20; i++)
					msg_format(Ind, "#%2d.  %20s  %3d", i + 1, deep_dive_name[i], deep_dive_level[i]);
				return;
			}
			/* Reset Ironman Deep Dive Challenge records */
			else if (prefix(message, "/deepdivereset")) {
				char path[MAX_PATH_LENGTH];
				char path_rev[MAX_PATH_LENGTH];
				path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_DATA, "legends.log");
				path_build(path_rev, MAX_PATH_LENGTH, ANGBAND_DIR_DATA, "legends-rev.log");
				/* Reset stats */
				for (i = 0; i < 20; i++) {
					strcpy(deep_dive_name[i], "");
					deep_dive_level[i] = 0;
				}
				/* Rebuild legends log file */
				reverse_lines(path, path_rev);
				msg_print(Ind, "Ironman Deep Dive Challenge records have been reset!");
				return;
			}
			/* list statics */
			else if (prefix(message, "/lsl") || prefix(message, "/lsls") || prefix(message, "/lslsu")) {
				int x, y;
				struct dungeon_type *d_ptr;
				worldpos tpos;
				bool stale = FALSE, used = FALSE;
				if (prefix(message, "/lsls")) stale = TRUE;
				if (prefix(message, "/lslsu")) used = TRUE;
				for (x = 0; x < 64; x++) for (y = 0; y < 64; y++) {
					/* check surface */
					k = 0; tpos.wx = x; tpos.wy = y; tpos.wz = 0;
					for (j = 1; j < NumPlayers + 1; j++) if (inarea(&Players[j]->wpos, &tpos)) k++;
					if (used && k) msg_format(Ind, "\377g  %2d,%2d", x, y);
					else if (wild_info[y][x].ondepth > k) msg_format(Ind, "  %2d,%2d", x, y);
					else if (stale && getcave(&tpos) && stale_level(&tpos, cfg.anti_scum)) msg_format(Ind, "\377D  %2d,%2d", x, y);
					/* check tower */
					if ((d_ptr = wild_info[y][x].tower)) {
					    //msg_format(Ind, "T max,base = %d,%d", d_ptr->maxdepth, d_ptr->baselevel);
					    for (i = 0; i < d_ptr->maxdepth; i++) {
						k = 0; tpos.wx = x; tpos.wy = y; tpos.wz = i + 1;
						for (j = 1; j < NumPlayers + 1; j++) if (inarea(&Players[j]->wpos, &tpos)) k++;
						if (used && k) msg_format(Ind, "\377gT %2d,%2d,%2d", x, y, i + 1);
						else if (d_ptr->level[i].ondepth > k) msg_format(Ind, "T %2d,%2d,%2d", x, y, i + 1);
						else if (stale && getcave(&tpos) && stale_level(&tpos, cfg.anti_scum)) msg_format(Ind, "\377DT %2d,%2d,%2d", x, y, i + 1);
					    }
					}
					/* check dungeon */
					if ((d_ptr = wild_info[y][x].dungeon)) {
					    //msg_format(Ind, "D max,base = %d,%d", d_ptr->maxdepth, d_ptr->baselevel);
					    for (i = 0; i < d_ptr->maxdepth; i++) {
						k = 0; tpos.wx = x; tpos.wy = y; tpos.wz = -(i + 1);
						for (j = 1; j < NumPlayers + 1; j++) if (inarea(&Players[j]->wpos, &tpos)) k++;
						if (used && k) msg_format(Ind, "\377gD %2d,%2d,%2d", x, y, -(i + 1));
						else if (d_ptr->level[i].ondepth > k) msg_format(Ind, "d %2d,%2d,%2d", x, y, -(i + 1));
						else if (stale && getcave(&tpos) && stale_level(&tpos, cfg.anti_scum)) msg_format(Ind, "\377DD %2d,%2d,%2d", x, y, -(i + 1));
					    }
					}
				}
				msg_print(Ind, "done.");
				return;
			}
		}
	}

	do_slash_brief_help(Ind);
	return;
}
#endif

static void do_slash_brief_help(int Ind){
#if 0 /* pretty outdated */
	player_type *p_ptr = Players[Ind];

	msg_print(Ind, "Commands: afk at bed broadcast bug cast dis dress ex feel fill help ignore me");	// pet ?
	msg_print(Ind, "          pk quest rec ref rfe shout sip tag target untag;");

	if (is_admin(p_ptr)) {
		msg_print(Ind, "  art cfg cheeze clv cp en eq geno id kick lua purge respawn shutdown");
		msg_print(Ind, "  sta store trap unc unst wish");
	} else {
		msg_print(Ind, "  /dis \377rdestroys \377wall the uninscribed items in your inventory!");
	}
#else
#endif
	msg_print(Ind, "Common commands: afk page ex feel rec tag untag dis me fill ignore bug rfe");
	msg_print(Ind, "  please type '/help' for detailed help.");
}


