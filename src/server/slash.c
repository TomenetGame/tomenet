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

	apply_magic(&p_ptr->wpos, o_ptr, -1, TRUE, TRUE, TRUE, FALSE, p_ptr->total_winner?FALSE:TRUE);
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
		if (!o_ptr->k_idx) continue;

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
 	char *colon, *token[9], message2[80];
	worldpos wp;
	bool admin = is_admin(p_ptr);

	strcpy(message2, message);
	wpcopy(&wp, &p_ptr->wpos);

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
		}
		else
		{
			msg_format_near(Ind, "\377BYou hear %s shout!", p_ptr->name);
		}
		msg_format(Ind, "\377BYou shout:%s", colon);
		return;
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
		else if (prefix(message, "/afk"))
		{
			if (strlen(message2 + 4) > 0)
			    toggle_afk(Ind, message2 + 5);
			else
			    toggle_afk(Ind, "");
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

					do_cmd_wield(Ind, j);

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
				msg_format(Ind, "You %shave head.", r_ptr->body_parts[BODY_HEAD] ? "" : "don't ");
				msg_format(Ind, "You %shave arms.", r_ptr->body_parts[BODY_ARMS] ? "" : "don't ");
				msg_format(Ind, "You can%s use weapons.", r_ptr->body_parts[BODY_WEAPON] ? "" : "not");
				msg_format(Ind, "You can%s wear %s.", r_ptr->body_parts[BODY_FINGER] ? "" : "not", r_ptr->body_parts[BODY_FINGER] == 1 ? "a ring" : "rings");
				msg_format(Ind, "You %shave torso.", r_ptr->body_parts[BODY_TORSO] ? "" : "don't ");
				msg_format(Ind, "You %shave legs suitable for shoes.", r_ptr->body_parts[BODY_LEGS] ? "" : "don't ");
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

				if (i==-1)
				{
					msg_print(Ind, "\377oInscription {@R} not found.");
					return;
				}

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
					default:
						do_cmd_activate(Ind, item);
						//							msg_print(Ind, "\377oYou cannot recall with that.");
						break;
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
			int i, j=Ind, k=0;
			s16b r;
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
			if (Players[j]->lev < 5) {
				msg_print(Ind, "\377oYou need to be level 5 or higher to receive a quest!");
				return;
			}
			
			get_mon_num_hook=dungeon_aux;
			get_mon_num_prep();
			i=2+randint(5);

			/* plev 1..50 -> mlev 1..100 (!) */
			if (lev <= 50) lev += (lev * lev) / 83;
			else lev = 80 + rand_int(20);

			do{
				r=get_mon_num(lev, 0);
				k++;
				if(k>100) lev--;
			} while(((lev-5) > r_info[r].level) || r_info[r].flags1 & RF1_UNIQUE);
			if (r_info[r].flags1 & RF1_FRIENDS) i = i + 11 + randint(7);
			add_quest(Ind, j, r, i, flags);
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

			if (get_skill(p_ptr, SKILL_MIMIC))
			{
				i = r_ptr->level - num;

				if (i > 0)
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
			rn = 0;
			for (i = 0; i < k; i++)
			{
			    rn += randint(6);
			}
			msg_format(Ind, "\377UYou throw %d dice and get a %d", k, rn);
			msg_format_near(Ind, "\377U%s throws %d dice and gets a %d", p_ptr->name, k, rn);
			return;
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
			for (i = 0; i < MAX_PARTIES; i++) {
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
			char tname[80];
			if (tk < 1)	/* Explain command usage */
			{
				msg_print(Ind, "\377oUsage: /note <player>[:<text>]  --  Example: /note El Hazard:Hiho!");
				msg_print(Ind, "\377oNot specifiying any text will remove pending notes to that player.");
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
			/* Look for free space to store the new note */
			for (i = 0; i < MAX_NOTES; i++) {
				if (!strcmp(priv_note[i], "")) {
					/* found a free memory spot */
					found_note = i;
				}
				if (!strcmp(priv_note_sender[i], p_ptr->name)) notes++;
			}
			if (found_note == MAX_NOTES) {
				msg_format(Ind, "\377oSorry, the server reached the maximum of %d pending notes.", MAX_NOTES);
				return;
			}
			if ((notes >= 3) && !is_admin(p_ptr)) {
				msg_print(Ind, "\377oYou have already reached the maximumm of 3 pending notes per player.");
				return;
			}
			strcpy(priv_note_sender[found_note], p_ptr->name);
			strcpy(priv_note_target[found_note], tname);
			strcpy(priv_note[found_note], message2 + j + 1);
			msg_format(Ind, "\377yNote %s to %s has been stored.", priv_note[found_note], priv_note_target[found_note]);
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
				msg_print(Ind, "\377y* Shutting down as soon as server is empty *");
				cfg.runlevel = 2048;
				return;
			}
#if 0	/* not implemented yet */
			else if (prefix(message, "/shutsurface")) {
				msg_print(Ind, "\377y* Shutting down as soon as noone is inside a dungeon/tower *");
				cfg.runlevel = 2049;
				return;
			}
#endif
			else if (prefix(message, "/pet")){
#if 1
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
#endif
				return;
			}
			else if (prefix(message, "/val")){
				if(!tk) return;
				do{
					msg_format(Ind, "\377GValidating %s", token[tk]);
					validate(token[tk]);
				}while(--tk);
				return;
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
							sprintf(kickmsg, "banned for %d minutes", atoi(token[2]));
							Destroy_connection(Players[j]->conn, kickmsg);
						} else {
							msg_format(Ind, "Banning %s for %d minutes (%s)...", Players[j]->name, atoi(token[2]), token[3]);
							sprintf(kickmsg, "Banned for %d minutes (%s)", atoi(token[2]), token[3]);
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
							sprintf(kickmsg, "kicked out (%s)", token[2]);
							Destroy_connection(Players[j]->conn, kickmsg);
						}
						/* Kick him */
					}
					return;
				}

				msg_print(Ind, "\377oUsage: /kick (Player name) [reason]");
				return;
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
					gametype=EEGAME_RUGBY;
					msg_broadcast(0, "A new game of rugby has started!");
					for(k=1; k<=NumPlayers; k++){
						Players[k]->team=0;
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
					prefix(message, "/sta"))
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
			else if (prefix(message, "/wish"))
			{
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

				apply_magic(&p_ptr->wpos, o_ptr, -1, TRUE, TRUE, TRUE, FALSE, TRUE);
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
			else if (prefix(message, "/store") ||
					prefix(message, "/sto"))
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
					if(!strcmp(string_make(token[1]),"on"))
					{
						msg_print(Ind, "log_u is now on");
						cfg.log_u = TRUE;
						return;
					}
					else if(!strcmp(string_make(token[1]),"off"))
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
					if(!strcmp(string_make(token[1]),"on"))
					{
						msg_print(Ind, "artifact generation is now supressed");
						cfg.arts_disabled = TRUE;
						return;
					}
					else if(!strcmp(string_make(token[1]),"off"))
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
			else if (prefix(message, "/debug-t")){
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
			else if (prefix(message, "/debug-s")){
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
			else if (prefix(message, "/debug-d")){
				/* C. Blue's mad debug code to change
				   dungeon flags [of Nether Realm] */
				int type;
				struct dungeon_type *d_ptr;
				bool tower = FALSE;
				worldpos *tpos = &p_ptr->wpos;
				wilderness_type *wild=&wild_info[tpos->wy][tpos->wx];

				if (d_ptr = wild->tower) tower = TRUE;
				else d_ptr = wild->dungeon;
				type = d_ptr->type;

				d_ptr->flags1 = d_info[type].flags1;
				d_ptr->flags2 = d_info[type].flags2 | DF2_RANDOM;
				msg_print(Ind, "Dungeon flags updated.");
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
				if (message2[8] = '*') {
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
			else if (prefix(message, "/threaten") || prefix(message, "/thr")) { /* Nearly kill someone, as threat >:) */
				int j;
				if (!tk) {
					msg_print(Ind, "Usage: /threaten <player-Index>");
					return;
				}
				/*j = name_lookup_loose(Ind, token[1], FALSE);
				if (!j) return;*/
				j = atoi(token[1]);
			        take_hit(j, Players[j]->chp - 1, "");
			        msg_print(j, "\377rYou are hit by a bolt from the blue!");
			        msg_print(j, "\377rThat was close huh?!");
				return;
			}
			else if (prefix(message, "/slap")) { /* Slap someone around, as threat :-o */
				int j;
				if (!tk) {
					msg_print(Ind, "Usage: /slap <player-Index>");
					return;
				}
				/*j = name_lookup_loose(Ind, token[1], FALSE);
				if (!j) return;*/
				j = atoi(token[1]);
			        take_hit(j, Players[j]->chp / 4, "");
			        msg_print(j, "\377rYou are slapped by something invisible!");
				return;
			}
			else if (prefix(message, "/deltown")){
				deltown(Ind);
				return;
			}
			else if (prefix(message, "/counthouses")) {
				int i;
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
				teleport_player(Ind, 10);
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
				teleport_player(Ind, 100);
				msg_print(Ind, "Teleported that player.");
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


