/* $Id$ */
/* File: cmd4.c */

/* Purpose: Interface commands */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define SERVER

#include "angband.h"



/*
 * Check the status of "artifacts"
 *
 * Should every artifact that is held by anyone be listed?  If so, should this
 * list the holder?  Doing so might induce wars to take hold of relatively
 * worthless artifacts (like the Phial), simply because there are very few
 * (three) artifact lites.  Right now, the holder isn't listed.
 *
 * Also, (for simplicity), artifacts lying in the dungeon, or artifacts that
 * are in a player's inventory but not identified are put in the list.
 *
 * Why does Ben save the list to a file and then display it?  Seems like a
 * strange way of doing things to me.  --KLJ--
 */
void do_cmd_check_artifacts(int Ind, int line)
{
	int i, j, k, z;

	FILE *fff;

	char file_name[MAX_PATH_LENGTH];

	char base_name[80];

	bool okay[MAX_A_IDX];

	player_type *q_ptr = Players[Ind];
	bool admin = is_admin(q_ptr);
	bool shown = FALSE;

	/* Temporary file */
	if (path_temp(file_name, MAX_PATH_LENGTH)) return;

	/* Open a new file */
	fff = my_fopen(file_name, "w");

	/* Scan the artifacts */
	for (k = 0; k < MAX_A_IDX; k++)
	{
		artifact_type *a_ptr = &a_info[k];

		/* Default */
		okay[k] = FALSE;

		/* Skip "empty" artifacts */
		if (!a_ptr->name) continue;

		/* Skip "uncreated" artifacts */
		if (!a_ptr->cur_num) continue;

		/* Skip "unknown" artifacts */
		if (!a_ptr->known && !admin) continue;

		/* Assume okay */
		okay[k] = TRUE;
	}

	/* Check the inventories */
	for (i = 1; i <= NumPlayers; i++)
	{
		player_type *p_ptr = Players[i];
		
		/* Check this guy's */
		for (j = 0; j < INVEN_PACK; j++)
		{
			object_type *o_ptr = &p_ptr->inventory[j];

			/* Ignore non-objects */
			if (!o_ptr->k_idx) continue;

			/* Ignore non-artifacts */
			if (!true_artifact_p(o_ptr)) continue;

			/* Ignore known items */
//			if (object_known_p(Ind, o_ptr) && !admin) continue;
			if (object_known_p(Ind, o_ptr)) continue;

			/* Note the artifact */
			okay[o_ptr->name1] = FALSE;
		}
	}

	/* Scan the artifacts */
	for (k = 0; k < MAX_A_IDX; k++)
	{
		artifact_type *a_ptr = &a_info[k];

		/* List "dead" ones */
		if (!okay[k]) continue;

		/* Paranoia */
		strcpy(base_name, "Unknown Artifact");

		/* Obtain the base object type */
		z = lookup_kind(a_ptr->tval, a_ptr->sval);

		/* Real object */
		if (z)
		{
			object_type forge;

			/* Create the object */
			invcopy(&forge, z);

			/* Create the artifact */
			forge.name1 = k;

			/* Describe the artifact */
			object_desc_store(Ind, base_name, &forge, FALSE, 0);

			/* Hack -- remove {0} */
			j = strlen(base_name);
			base_name[j-4] = '\0';
		}

		/* Hack -- Build the artifact name */
		if (admin) fprintf(fff, "(%3d)", k);
		fprintf(fff, "     The %s%s\n", base_name, admin && !a_ptr->known ?
				" (unknown)" : "");

		shown = TRUE;
	}

	if (!shown)
	{
		fprintf(fff, "No artifacts are witnessed so far.\n");
	}

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(Ind, file_name, "Artifacts Seen", line, 0);

	/* Remove the file */
	fd_kill(file_name);
}


/*
 * Check the status of "uniques"
 *
 * Note that the player ghosts are ignored.  XXX XXX XXX
 *
 * Any unique seen by any player will be shown.  Also, I plan to add the name
 * of the slayer (if any) to the list, so the others will know just how
 * powerful any certain player is.  --KLJ--
 */
/* Pfft, we should rewrite show_file so that we can change
 * the colour for each letter!	- Jir - */
void do_cmd_check_uniques(int Ind, int line)
{
	//player_type *p_ptr = Players[Ind];
	monster_race *r_ptr;

	int k, l, total = 0;

	FILE *fff;

	char file_name[MAX_PATH_LENGTH];

	player_type *q_ptr = Players[Ind];
	bool admin = is_admin(q_ptr);
	s16b idx[MAX_R_IDX];

	/* Temporary file */
	if (path_temp(file_name, MAX_PATH_LENGTH)) return;

	/* Open a new file */
	fff = my_fopen(file_name, "w");

#if 0
	/* Scan the monster races */
	for (k = 1; k < MAX_R_IDX-1; k++)
	{
		monster_race *r_ptr = &r_info[k];

		/* Only print Uniques */
		if (r_ptr->flags1 & RF1_UNIQUE)
		{
			/* Only display known uniques */
//			if (r_ptr->r_sights && mon_allowed(r_ptr))
			if (r_ptr->r_sights)
			{
				int i, j = 0;
				byte ok = FALSE;
				bool full = FALSE;

				/* Output color byte */
				fprintf(fff, "\377%c", 'w');

				/* Hack -- Show the ID for admin */
				if (admin) fprintf(fff, "(%4d) ", k);
				/* don't display dungeon master to players */
				else if (q_ptr->admin_dm) continue;

				/* Format message */
//				fprintf(fff, "%s has been killed by:\n", r_name + r_ptr->name);
				fprintf(fff, "%s has been killed by", r_name + r_ptr->name);

				for (i = 1; i <= NumPlayers; i++)
				{
					player_type *q_ptr = Players[i];
					
					/* skip dungeon master */
					if (q_ptr->admin_dm) continue;

					if (q_ptr->r_killed[k])
					{
//						byte attr = 'U';
//						fprintf(fff, "        %s\n", q_ptr->name);
						if (!ok)
						{
							fprintf(fff, ":\n");
							fprintf(fff, "\377%c", 'B');
							ok = TRUE;
						}
#if 0
						/* Print self in green */
						if (Ind == k) attr = 'G';

						/* Print party members in blue */
						else if (p_ptr->party && p_ptr->party == q_ptr->party) attr = 'B';

						/* Print hostile players in red */
						else if (check_hostile(Ind, i)) attr = 'r';

						/* Output color byte */
						fprintf(fff, "\377%c", attr);
#endif

						fprintf(fff, "  %-16.16s", q_ptr->name);
						j++;
						full = FALSE;
						if (j == 4)
						{
							fprintf(fff, "\n");
							fprintf(fff, "\377%c", 'B');
							j = 0;
							full = TRUE;
						}
					}
				}
//				if (!ok) fprintf(fff, "       Nobody\n");
				if (!ok)
				{
					/* Output color byte */
//					fprintf(fff, "%c", 'y');

					if (r_ptr->r_tkills) fprintf(fff, " Somebody.");
					else fprintf(fff, " Nobody.");
				}
				/* Terminate line */
				/*                              fprintf(fff, "\n");*/
				if (!full) fprintf(fff, "\n");
			}

		}
	}
#endif	// 0

	fprintf(fff, "\377U======== Unique Monster List ========\n");

	/* Scan the monster races */
	for (k = 1; k < MAX_R_IDX-1; k++)
	{
		r_ptr = &r_info[k];

		/* Only print Uniques */
		if (r_ptr->flags1 & RF1_UNIQUE)
		{
			/* Only display known uniques */
//			if (r_ptr->r_sights && mon_allowed(r_ptr))
			if (r_ptr->r_sights && mon_allowed_view(r_ptr))
			{
				idx[total++] = k;
			}
		}
	}

	if (total)
	{
		/* Setup the sorter */
		ang_sort_comp = ang_sort_comp_mon_lev;
		ang_sort_swap = ang_sort_swap_s16b;

		/* Sort the monsters according to value */
		ang_sort(Ind, &idx, NULL, total);

		/* for each unique */
		for (l = total - 1; l >= 0; l--)
		{
			int i, j = 0;
			byte ok = FALSE;
			bool full = FALSE;

			k = idx[l];
			r_ptr = &r_info[k];

			/* Output color byte */
			fprintf(fff, "\377w");

			/* Hack -- Show the ID for admin */
			if (admin) fprintf(fff, "(%4d) ", k);

			/* Format message */
			//				fprintf(fff, "%s has been killed by:\n", r_name + r_ptr->name);
			fprintf(fff, "%s has been killed by", r_name + r_ptr->name);

			for (i = 1; i <= NumPlayers; i++)
			{
				player_type *q_ptr = Players[i];

				/* don't display dungeon master to players */
				if (q_ptr->admin_dm && !Players[Ind]->admin_dm) continue;

				if (q_ptr->r_killed[k])
				{
					byte attr = 'B';

					/* Print self in green */
					if (Ind == i) attr = 'G';

					/* Print party members in blue */
//					else if (p_ptr->party && p_ptr->party == q_ptr->party) attr = 'B';

					/* Print hostile players in red */
//					else if (check_hostile(Ind, i)) attr = 'r';

					if (!ok)
					{

						fprintf(fff, ":\n");
						ok = TRUE;
					}

					fprintf(fff, "\377%c", attr);
					fprintf(fff, "  %-16.16s", q_ptr->name);
					j++;
					full = FALSE;
					if (j == 4)
					{
						fprintf(fff, "\n");
						j = 0;
						full = TRUE;
					}
				}
			}
			//				if (!ok) fprintf(fff, "       Nobody\n");
			if (!ok)
			{
				/* Output color byte */
				//					fprintf(fff, "%c", 'y');

				if (r_ptr->r_tkills) fprintf(fff, " Somebody.");
				else fprintf(fff, " Nobody.");
			}
			/* Terminate line */
			/*                              fprintf(fff, "\n");*/
			if (!full) fprintf(fff, "\n");
		}
	}
	else
	{
		fprintf(fff, "\377w");
		fprintf(fff, "No uniques are witnessed so far.\n");
	}


	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(Ind, file_name, "Known Uniques", line, 0);

	/* Remove the file */
	fd_kill(file_name);
}

static void do_write_others_attributes(FILE *fff, player_type *q_ptr, bool modify)
{
	int modify_number = 0;
	cptr p = "";
	bool text_pk = FALSE, text_silent = FALSE, text_afk = FALSE;

	/* Prepare title already */
        if (q_ptr->lev < 60)
                p = player_title[q_ptr->pclass][((q_ptr->lev/5) < 10)? (q_ptr->lev/5) : 10][1 - q_ptr->male];
        else
                p = player_title_special[q_ptr->pclass][(q_ptr->lev < 99)? (q_ptr->lev - 60)/10 : 4][1 - q_ptr->male];

	/* Check for special character */
	if(modify){
		/* Uncomment these as you feel it's needed ;) */
		//if(!strcmp(q_ptr->name,"")) modify_number=1; //wussy Cheezer
		//if(!strcmp(q_ptr->name,"")) modify_number=2; //silyl Slacker
		//if(!strcmp(q_ptr->name,"Duncan McLeod")) modify_number=3; //Highlander games Judge ;) Bread and games to them!!
		//if(!strcmp(q_ptr->name,"Tomenet")) modify_number=4;//Server-specific Dungeon Masters
		//if(!strcmp(q_ptr->name,"C. Blue")) modify_number=4;//Server-specific Dungeon Masters
		//if(!strcmp(q_ptr->name,"C.Blue")) modify_number=4;//Server-specific Dungeon Masters
		if(q_ptr->admin_dm) modify_number=4;
		if(q_ptr->admin_wiz) modify_number=5;
	}
	/* Print a message */
#if 0
	fprintf(fff, "  %s the %s%s %s (%s%sLv %d, %s)",
			q_ptr->name, (q_ptr->mode == MODE_HELL)?"hellish ":"",
			race_info[q_ptr->prace].title, class_info[q_ptr->pclass].title,
			(q_ptr->total_winner)?
			    ((p_ptr->mode & (MODE_HELL | MODE_NO_GHOST))?
				((q_ptr->male)?"Emperor":"Empress"):
				((q_ptr->male)?"King, ":"Queen, ")):
			    ((q_ptr->male)?"Male, ":"Female, "),
			q_ptr->fruit_bat ? "Fruit bat, " : "",
			q_ptr->lev, parties[q_ptr->party].name);
#else	// 0
	fprintf(fff, "  %s the ", q_ptr->name);

	switch(modify_number){
	case 1:	fprintf(fff, "wussy "); break;
	case 2: fprintf(fff, "silyl "); break;
	default:
		switch (q_ptr->mode)	// TODO: give better modifiers
		{
    			case MODE_NORMAL:
				break;
			case MODE_HELL:
				fprintf(fff, "purgatorial ");
				break;
			case MODE_NO_GHOST:
				fprintf(fff, "unworldly ");
				break;
			case MODE_IMMORTAL:
				fprintf(fff, "everlasting ");
				break;
	    		case (MODE_HELL + MODE_NO_GHOST):
				fprintf(fff, "hellish ");
				break;
		}
		break;
	}

	switch(modify_number){
	case 3: fprintf(fff, "Highlander "); break; //Judge for Highlander games
	default: fprintf(fff, "%s ", race_info[q_ptr->prace].title); break;
	}
	
	switch(modify_number){
	case 1: fprintf(fff, "Cheezer "); break;
	case 2: fprintf(fff, "Slacker "); break;
	case 3: if (q_ptr->male) fprintf(fff, "Swordsman ");
		else fprintf(fff, "Swordswoman ");
		break; //Judge for Highlander games
	default:
		fprintf(fff, "%s", class_info[q_ptr->pclass].title); break;
	}

	/* PK */
	if (cfg.use_pk_rules == PK_RULES_DECLARE)
	{
		text_pk = TRUE;
		if(q_ptr->pkill & (PKILL_SET | PKILL_KILLER))
			fprintf(fff, "   (PK");
		else if(!(q_ptr->pkill & PKILL_KILLABLE))
			fprintf(fff, "   (SAFE");
		else if(!(q_ptr->tim_pkill))
			fprintf(fff, q_ptr->lev < 5 ? "   (Newbie" : "   (Killable");
		else
			text_pk = FALSE;
	}
	if(q_ptr->limit_chat)
	{
		text_silent = TRUE;
		if (text_pk)
			fprintf(fff, ", Silent");
		else
			fprintf(fff, "   (Silent");
	}
	/* AFK */
	if(q_ptr->afk)
	{
		text_afk = TRUE;
		if (text_pk || text_silent) {
//			if (strlen(q_ptr->afk_msg) == 0)
				fprintf(fff, ", AFK");
//			else
//				fprintf(fff, ", AFK: %s", q_ptr->afk_msg);
		} else {
//			if (strlen(q_ptr->afk_msg) == 0)
				fprintf(fff, "   (AFK");
//			else
//				fprintf(fff, "   (AFK: %s", q_ptr->afk_msg);
		}
	}
	if (text_pk || text_silent || text_afk) fprintf(fff, ")");

	/* Line break here, it's getting too long with all that mods -C. Blue */
	fprintf(fff, "\n\377U     ");

	switch(modify_number){
	case 3: fprintf(fff, "\377rJudge\377U "); break; //Judge for Highlander games
	case 4: if (q_ptr->male) fprintf(fff,"\377bDungeon Master\377U ");
		else fprintf(fff,"\377bDungeon Mistress\377U ");
		break; //Server Admin
	case 5: if (q_ptr->male) fprintf(fff,"\377bDungeon Wizard\377U ");
		else fprintf(fff,"\377bDungeon Wizard\377U ");
		break; //Server Admin
	default: fprintf(fff, "%s",
		(q_ptr->total_winner)?
		    ((q_ptr->mode & (MODE_HELL | MODE_NO_GHOST))?
			((q_ptr->male)?"\377vEmperor\377U ":"\377vEmpress\377U "):
		        ((q_ptr->male)?"\377vKing\377U ":"\377vQueen\377U ")):
		((q_ptr->male)?"Male ":"Female "));
		break;
	}
	
	if (q_ptr->party)
	fprintf(fff, "%sLevel %d, Party: '%s%s\377U'",
		q_ptr->fruit_bat ? "Batty " : "",
		q_ptr->lev,
		(parties[q_ptr->party].mode == PA_IRONTEAM) ? "\377s" : "",
		parties[q_ptr->party].name);
	else
	fprintf(fff, "%sLevel %d",
		q_ptr->fruit_bat ? "Batty " : "",
		q_ptr->lev);
#endif	// 0
}

/*
 * Check the status of "players"
 *
 * The player's name, race, class, and experience level are shown.
 */
void do_cmd_check_players(int Ind, int line)
{
	int k;
	bool i;

	FILE *fff;

	char file_name[MAX_PATH_LENGTH];

	player_type *p_ptr = Players[Ind];

	bool admin = is_admin(p_ptr);

	/* Temporary file */
	if (path_temp(file_name, MAX_PATH_LENGTH)) return;

	/* Open a new file */
	fff = my_fopen(file_name, "w");
	if(fff==(FILE*)NULL) return;

	/* Scan the player races */
	for (k = 1; k < NumPlayers + 1; k++)
	{
		player_type *q_ptr = Players[k];
		byte attr = 'w';

		/* Only print connected players */
		if (q_ptr->conn == NOT_CONNECTED)
			continue;

		/* don't display the dungeon master if the secret_dungeon_master
		 * option is set 
		 */
		if (q_ptr->admin_dm &&
		   (cfg.secret_dungeon_master) && !admin) continue;

		/*** Determine color ***/

		/* Print self in green */
		if (Ind == k) attr = 'G';

		/* Print party members in blue */
		else if (p_ptr->party && p_ptr->party == q_ptr->party) attr = 'B';

		/* Print hostile players in red */
		else if (check_hostile(Ind, k)) attr = 'r';

		/* Output color byte */
		fprintf(fff, "\377%c", attr);

		/* Print a message */
//		if(Ind!=k) i=TRUE; else i=FALSE;
		i=TRUE;
		do_write_others_attributes(fff, q_ptr, i);
		/* Colour might have changed due to Iron Team party name,
		   so print the closing ')' in the original colour again: */
		/* not needed anymore since we have linebreak now
		fprintf(fff, "\377%c)", attr);*/

		/* Newline */
		/* -AD- will this work? - Sure -C. Blue- */
		fprintf(fff, "\n\377U");

		if (is_admin(p_ptr)) fprintf(fff, "  (%d)", k);

		fprintf(fff, "     %s@%s", q_ptr->realname, q_ptr->hostname);

		/* Print extra info if these people are in the same party */
		/* Hack -- always show extra info to dungeon master */
		if ((p_ptr->party == q_ptr->party && p_ptr->party) || Ind == k || admin)
		{
			/* maybe too kind? */
//			fprintf(fff, "   {[%d,%d] of %dft(%d,%d)}", q_ptr->panel_row, q_ptr->panel_col, q_ptr->wpos.wz*50, q_ptr->wpos.wx, q_ptr->wpos.wy);
			fprintf(fff, "  {[%d,%d] %s}", q_ptr->panel_row, q_ptr->panel_col, wpos_format(Ind, &q_ptr->wpos));

		}
		if((p_ptr->guild == q_ptr->guild && q_ptr->guild) || Ind == k || admin){
			if(q_ptr->guild)
				fprintf(fff, " [%s]", guilds[q_ptr->guild].name);
			fprintf(fff, " \377%c", (q_ptr->quest_id?'Q':' '));
		}
		if ((!q_ptr->afk) || !strlen(q_ptr->afk_msg))
			fprintf(fff, "\n\n");
		else
			fprintf(fff, "\n     \377U(%s)\n", q_ptr->afk_msg);
	}
#ifdef TOMENET_WORLDS
	world_remote_players(fff);
#endif	// TOMENET_WORLDS

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(Ind, file_name, "Player list", line, 0);

	/* Remove the file */
	fd_kill(file_name);
}
 /*
 * Check the equipments of other player.
 */
void do_cmd_check_player_equip(int Ind, int line)
{
	int i, k;
	bool m;

	FILE *fff;

	char file_name[MAX_PATH_LENGTH];

	player_type *p_ptr = Players[Ind];

	bool admin = is_admin(p_ptr);

	/* Temporary file */
	if (path_temp(file_name, MAX_PATH_LENGTH)) return;

	/* Open a new file */
	fff = my_fopen(file_name, "w");

	/* Scan the player races */
	for (k = 1; k < NumPlayers + 1; k++)
	{
		player_type *q_ptr = Players[k];
		byte attr = 'w';
		bool hidden = FALSE;

		/* Only print connected players */
		if (q_ptr->conn == NOT_CONNECTED)
			continue;

		/* don't display the dungeon master if the secret_dungeon_master
		 * option is set
		 */
		if (q_ptr->admin_dm &&
				(cfg.secret_dungeon_master)) continue;

		/*** Determine color ***/

		attr = 'G';

		/* Skip myself */
		if (Ind == k) continue;

		/* Print party members in blue */
		else if (p_ptr->party && p_ptr->party == q_ptr->party) attr = 'B';

		/* Print hostile players in red */
		else if (check_hostile(Ind, k)) attr = 'r';

		/* Print newbies/lowbies in white */
		else if (q_ptr->lev < 10) attr = 'w';

		/* Party member & hostile players only */
		/* else continue; */

		/* Only party member or those on the same dungeon level */
		//                              if ((attr != 'B') && (p_ptr->dun_depth != q_ptr->dun_depth)) continue;
		if ((attr != 'B') && (attr != 'w') && !admin)
		{
			/* Make sure this player is at this depth */
			if(!inarea(&p_ptr->wpos, &q_ptr->wpos)) continue;

			/* Can he see this player? */
			if (!(p_ptr->cave_flag[q_ptr->py][q_ptr->px] & CAVE_VIEW)) continue;
		}

		/* Skip invisible players */
#if 0
		if ((!p_ptr->see_inv || ((q_ptr->inventory[INVEN_OUTER].k_idx) && (q_ptr->inventory[INVEN_OUTER].tval == TV_CLOAK) && (q_ptr->inventory[INVEN_OUTER].sval == SV_SHADOW_CLOAK))) && q_ptr->invis)
		{
			if ((q_ptr->lev > p_ptr->lev) || (randint(p_ptr->lev) > (q_ptr->lev / 2)))
				continue;
		}
#endif

		if (q_ptr->invis && !admin &&
				(!p_ptr->see_inv ||
				 ((q_ptr->inventory[INVEN_OUTER].k_idx) && (q_ptr->inventory[INVEN_OUTER].tval == TV_CLOAK) && (q_ptr->inventory[INVEN_OUTER].sval == SV_SHADOW_CLOAK))) &&
				((q_ptr->lev > p_ptr->lev) || (randint(p_ptr->lev) > (q_ptr->lev / 2))))
			continue;

		/* Output color byte */
		fprintf(fff, "\377%c", attr);

		/* Print a message */
//		if(Ind!=k) m=TRUE; else m=FALSE;
		m=TRUE;
		do_write_others_attributes(fff, q_ptr, m);
		/* Colour might have changed due to Iron Team party name,
		   so print the closing ')' in the original colour again: */
		/* not needed anymore since we have a linebreak now
		fprintf(fff, "\377%c)", attr);*/

		fprintf(fff, "\n");

		/* Covered by a mummy wrapping? */
		if ((TOOL_EQUIPPED(q_ptr) == SV_TOOL_WRAPPING) && !admin) hidden = TRUE;

		/* Print equipments */
		for (i = (admin ? 0 : INVEN_WIELD);
				i < (hidden ? INVEN_LEFT : INVEN_TOTAL); i++)
		{
			object_type *o_ptr = &q_ptr->inventory[i];
			char o_name[160];
			if (o_ptr->tval) {
				object_desc(Ind, o_name, o_ptr, TRUE, 3);
				fprintf(fff, "\377%c %s\n", i < INVEN_WIELD? 'o' : 'w', o_name);
			}
		}

		/* Covered by a mummy wrapping? */
		if (hidden) fprintf(fff, "\377%c (Covered by a grubby wrapping)\n", 'D');

		/* Add blank line */
		fprintf(fff, "\377%c\n", 'w');

	}

       /* Close the file */
       my_fclose(fff);

       /* Display the file contents */
       show_file(Ind, file_name, "Someone's equipments", line, 0);

       /* Remove the file */
       fd_kill(file_name);
}


/*
 * List recall depths
 */
void do_cmd_knowledge_dungeons(int Ind)
{
	player_type *p_ptr = Players[Ind];

//	msg_format(Ind, "The deepest point you've reached: \377G-%d\377wft", p_ptr->max_dlv * 50);

	int		i, x, y;	// num, total = 0;
	//bool	shown = FALSE;
	bool	admin = is_admin(p_ptr);
	dungeon_type *d_ptr;

	FILE *fff;

	/* Paranoia */
	// if (!letter) return;

	/* Open a new file */
	fff = my_fopen(p_ptr->infofile, "w");

	/* Current file viewing */
	strcpy(p_ptr->cur_file, p_ptr->infofile);

	/* Let the player scroll through the info */
	p_ptr->special_file_type = TRUE;

	fprintf(fff, "======== Dungeon(s) ========\n\n");

#if 0
	if (p_ptr->depth_in_feet) fprintf(fff, "The deepest point you've reached: -%d ft\n", p_ptr->max_dlv * 50);
	else fprintf(fff, "The deepest point you've reached: Lev -%d\n", p_ptr->max_dlv);
#else	// 0
	fprintf(fff, "The deepest/highest point you've ever reached: %d ft (Lv %d)\n", p_ptr->max_dlv * 50, p_ptr->max_dlv);
#endif	// 0

#if 0
	/* Scan all dungeons */
	for (y = 1; y < max_d_idx; y++)
	{
		/* The dungeon has a valid recall depth set */
		if (max_dlv[y])
		{
			/* Describe the recall depth */
			fprintf(fff, "       %c%s: Level %d (%d')\n",
				(p_ptr->recall_dungeon == y) ? '*' : ' ',
				d_name + d_info[y].name,
				max_dlv[y], 50 * (max_dlv[y]));
		}
	}
#endif	// 0

	fprintf(fff,"\n");

	for (y = 0; y < MAX_WILD_Y; y++)
	{
		for (x = 0; x < MAX_WILD_X; x++)
		{
			if (!((p_ptr->wild_map[(x + y * MAX_WILD_X) / 8] &
						(1 << ((x + y * MAX_WILD_X) % 8))) || admin))
				continue;

			if ((d_ptr = wild_info[y][x].tower))
			{
				i = d_ptr->type;
				fprintf(fff, " (%3d, %3d) %-30s", x, y,
						d_info[i].name + d_name);
				if (admin)
#if 0
					fprintf(fff, "  Lev: %3d~%3d  Req: %3d  type: %3d",
							d_info[i].mindepth, d_info[i].maxdepth,
							d_info[i].min_plev, i);
#else	// 0
					fprintf(fff, "  Lev: %3d~%3d  Req: %3d  type: %3d",
							d_ptr->baselevel, d_ptr->baselevel + d_ptr->maxdepth - 1,
//							d_info[i].mindepth, d_info[i].mindepth + d_info[i].maxdepth - 1,
							d_info[i].min_plev, i);
#endif	// 0

				fprintf(fff,"\n");
			}
			if ((d_ptr = wild_info[y][x].dungeon))
			{
				i = d_ptr->type;
				fprintf(fff, " (%3d, %3d) %-30s", x, y,
						d_info[i].name + d_name);
				if (admin)
					fprintf(fff, "  Lev: %3d~%3d  Req: %3d  type: %3d",
							d_ptr->baselevel, d_ptr->baselevel + d_ptr->maxdepth - 1,
//							d_info[i].mindepth, d_info[i].mindepth + d_info[i].maxdepth - 1,
							d_info[i].min_plev, i);

				fprintf(fff,"\n");
			}
		}
	}

	fprintf(fff,"\n");



	fprintf(fff, "\n\n======== Town(s) ========\n\n");

	/* Scan all towns */
	for (i = 0; i < numtowns; i++)
	{
		y = town[i].y;
		x = town[i].x;

		/* The dungeon has a valid recall depth set */
		if ((p_ptr->wild_map[(x + y * MAX_WILD_X) / 8] &
					(1 << ((x + y * MAX_WILD_X) % 8))) || admin)
		{
			/* Describe the town locations */
			fprintf(fff, " (%3d, %3d) : %-15s", x, y,
					town_profile[town[i].type].name);
			if (admin)
				fprintf(fff, "  Lev: %d", town[i].baselevel);

			if (p_ptr->town_x == x && p_ptr->town_y == y)
				fprintf(fff, "  (default recall point)");

			fprintf(fff,"\n");
		}
	}

	/* Close the file */
	my_fclose(fff);

	/* Let the client know to expect some info */
	Send_special_other(Ind);
}

/*
 * Tell players of server settings, using temporary file. - Jir -
 */
void do_cmd_check_server_settings(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int		 k;

	FILE *fff;

#if 0
	char file_name[MAX_PATH_LENGTH];

	/* Temporary file */
	if (path_temp(file_name, MAX_PATH_LENGTH)) return;

	strcpy(p_ptr->infofile, file_name);
#endif

	/* Open a new file */
	fff = my_fopen(p_ptr->infofile, "w");

	/* Current file viewing */
	strcpy(p_ptr->cur_file, p_ptr->infofile);

	/* Let the player scroll through the info */
	p_ptr->special_file_type = TRUE;


	/* Output color byte */
//	fprintf(fff, "%c", 'G');

	fprintf(fff, "%s\n", longVersion);
	fprintf(fff, "======== Server Settings ========\n\n");

	/* Output color byte */
//	fprintf(fff, "%c", 'w');

	/* General information */
	fprintf(fff, "Server notes: %s\n", cfg.server_notes);
	fprintf(fff, "Game speed(FPS): %d (%+d%%)\n", cfg.fps, (cfg.fps-60)*100/60);
	fprintf(fff, "Players' running speed is boosted (x%d, ie. %+d%%).\n", cfg.running_speed, (cfg.running_speed - 5) * 100 / 5);
	fprintf(fff, "While 'resting', HP/SP recovers %d times quicker (%+d%%)\n", cfg.resting_rate, (cfg.resting_rate-3)*100/3);

	if ((k=cfg.party_xp_boost))
		fprintf(fff, "Party members get boosted exp(factor %d).\n", k);

	if (cfg.replace_hiscore == 0)
		fprintf(fff, "High-score entries are added to the high-score table.\n");
	if (cfg.replace_hiscore == 1)
		fprintf(fff, "Newer high-score entries replace old entries.\n");
	if (cfg.replace_hiscore == 2)
		fprintf(fff, "Only higher & newer high-score entries replace old entries.\n(Otherwise the new entry is ignored).\n");

	/* Several restrictions */
	if (!cfg.maximize)
		fprintf(fff, "This server is *NOT* maximized!\n");

	fprintf(fff,"\n");

	if ((k=cfg.newbies_cannot_drop))
		fprintf(fff, "Players under exp.level %d are not allowed to drop items/gold.\n", k);

	if ((k=cfg.spell_interfere))
		fprintf(fff, "Monsters adjacent to you have %d%% chance of interfering your spellcasting.\n", k);

	if ((k=cfg.spell_stack_limit))
		fprintf(fff, "Duration of assistance spells is limited to %d turns.\n", k);

	if (cfg.clone_summoning != 999)
		fprintf(fff, "Monsters may summon up to %d times until the summons start to become clones.\n", cfg.clone_summoning);

	k=cfg.use_pk_rules;
	switch (k)
	{
		case PK_RULES_DECLARE:
			fprintf(fff, "You should use /pk first to attack other players.\n", k);
			break;

		case PK_RULES_NEVER:
			fprintf(fff, "You are not allowed to attack/rob other players.\n", k);
			break;

		case PK_RULES_TRAD:
		default:
			fprintf(fff, "You can attack/rob other players(but not recommended).\n", k);
			break;
	}

	fprintf(fff,"\n");

	/* level preservation */
	if (cfg.no_ghost)
		fprintf(fff, "You disappear the moment you die, without becoming a ghost.\n");
	if (cfg.lifes)
		fprintf(fff, "Players with ghosts can be resurrected up to %d times until their soul\n will escape and their bodies will be permanently destroyed.\n", cfg.lifes);
	if (cfg.houses_per_player)
		fprintf(fff, "Players may own up to level/%d houses at once.\n", cfg.houses_per_player);
	else
		fprintf(fff, "Players may own as many houses at once as they like.\n");	    

	fprintf(fff, "The floor will be erased about %d~%d seconds after you left.\n", cfg.anti_scum, cfg.anti_scum + 10);
	if ((k=cfg.level_unstatic_chance))
		fprintf(fff, "When saving in dungeon, the floor is kept for %dx(level) minutes.\n", k);

	if ((k=cfg.min_unstatic_level) > 0) 
		fprintf(fff, "Shallow dungeon(till %d) will never be saved. Save in town!\n", k);

	if ((k=cfg.preserve_death_level) < 201)
		fprintf(fff, "Site of death under level %d will be static, allowing others to loot it.\n", k);


	fprintf(fff,"\n");
		
        /* Items */
	if (cfg.anti_cheeze_pickup)
		fprintf(fff, "Items cannot be transferred to a character of too low a level.\n");
	if (cfg.surface_item_removal)
		fprintf(fff, "Items on the world surface will be removed after %d minutes.\n", cfg.surface_item_removal);
	if (cfg.dungeon_item_removal)
		fprintf(fff, "Items on a dungeon/tower floor will be removed after %d minutes.\n", cfg.surface_item_removal);

	fprintf(fff,"\n");

	/* arts & winners */
	if (cfg.anti_arts_hoard)
		fprintf(fff, "True-Artifacts will disappear if you drop/leave them.\n");
	else if (cfg.anti_arts_house)
		fprintf(fff, "True-Artifacts will disappear if you drop/leave them inside a house.\n");
	if (cfg.anti_arts_pickup)
		fprintf(fff, "True-Artifacts cannot be transferred to a character of too low a level.\n");
	if (cfg.anti_arts_send)
		fprintf(fff, "True-Artifacts cannot be sent via telekinesis.\n");

	if ((k=cfg.retire_timer) > 0)
		fprintf(fff, "The winner will automatically retire after %d minutes.\n", k);
	else if (k == 0)
		fprintf(fff, "The game ends the moment you beat the final foe, Morgoth.\n", k);

	if (k !=0)
	{
		if (cfg.kings_etiquette)
			fprintf(fff, "The winner is not allowed to carry/use artifacts(save Grond/Crown).\n");

		if ((k=cfg.unique_respawn_time))
			fprintf(fff, "After winning the game, unique monsters will resurrect randomly.(%d)\n", k);
	}

	fprintf(fff,"\n");

	/* monster-sets */
	fprintf(fff, "Monsters:\n");
	if (is_admin(p_ptr))
	{
		if (cfg.vanilla_monsters)
			fprintf(fff, "  Vanilla-angband(default) monsters (%d%%)\n", cfg.vanilla_monsters);
		if (cfg.zang_monsters)
			fprintf(fff, "  Zelasny Angband additions (%d%%)\n", cfg.zang_monsters);
		if (cfg.pern_monsters)
			/* XXX Let's be safe */
			//fprintf(fff, "  DragonRiders of Pern additions (%d%%)\n", cfg.pern_monsters);
			fprintf(fff, "  Thunderlord additions (%d%%)\n", cfg.pern_monsters);
		if (cfg.cth_monsters)
			fprintf(fff, "  Lovecraft additions (%d%%)\n", cfg.cth_monsters);
		if (cfg.cblue_monsters)
			fprintf(fff, "  C. Blue-monsters (%d%%)\n", cfg.cblue_monsters);
		if (cfg.joke_monsters)
			fprintf(fff, "  Joke-monsters (%d%%)\n", cfg.joke_monsters);
		if (cfg.pet_monsters)
			fprintf(fff, "  Pet/neutral monsters (%d%%)\n", cfg.pet_monsters);
	}
	else
	{
		if (cfg.vanilla_monsters > TELL_MONSTER_ABOVE)
			fprintf(fff, "  Vanilla-angband(default) monsters\n");
		if (cfg.zang_monsters > TELL_MONSTER_ABOVE)
			fprintf(fff, "  Zelasny Angband additions\n");
		if (cfg.pern_monsters > TELL_MONSTER_ABOVE)
			//fprintf(fff, "  DragonRiders of Pern additions\n");
			fprintf(fff, "  Thunderlord additions\n");
		if (cfg.cth_monsters > TELL_MONSTER_ABOVE)
			fprintf(fff, "  Lovecraft additions\n");
		if (cfg.cblue_monsters > TELL_MONSTER_ABOVE)
			fprintf(fff, "  C. Blue-monsters\n");
		if (cfg.joke_monsters > TELL_MONSTER_ABOVE)
			fprintf(fff, "  Joke-monsters\n");
	}

	fprintf(fff,"\n");

	/* trivial */
	if (cfg.public_rfe)
//		fprintf(fff, "You can see RFE files via '&62' command.\n");
		fprintf(fff, "You can see RFE files via '~e' command.\n");

	/* TODO: reflect client options too */
	if (cfg.door_bump_open & BUMP_OPEN_DOOR)
//		fprintf(fff, "You'll try to open a door by bumping onto it.\n");
		fprintf(fff, "easy_open is allowed.\n");
	else
		fprintf(fff, "You should use 'o' command explicitly to open a door.\n");

	if (cfg.door_bump_open & BUMP_OPEN_TRAP)
//		fprintf(fff, "You'll try to disarm a visible trap by stepping onto it.\n");
		fprintf(fff, "easy_disarm is allowed.\n");

	if (cfg.door_bump_open & BUMP_OPEN_HOUSE)
		fprintf(fff, "You can 'walk through' your house door.\n");


	/* Administrative */
	if (is_admin(p_ptr))
	{
		/* Output color byte */
//		fprintf(fff, "%c\n", 'o');

		fprintf(fff,"\n");
		

		fprintf(fff, "==== Administrative or hidden settings ====\n");

		/* Output color byte */
//		fprintf(fff, "%c\n", 'w');

		fprintf(fff, "dun_unusual: %d (default = 200)\n", cfg.dun_unusual);
		fprintf(fff, "Stores change their inventory every %d seconds(store_turns=%d).\n", cfg.store_turns * 10 / cfg.fps, cfg.store_turns);

		fprintf(fff, "starting town: location [%d, %d], baselevel(%d)\n", cfg.town_x, cfg.town_y, cfg.town_base);
		fprintf(fff, "Bree dungeon: baselevel(%d) depth(%d)\n", cfg.dun_base, cfg.dun_max);

		if (cfg.auto_purge)
			fprintf(fff, "Non-used monsters/objects are purged every 24H.\n");

#if 0
		if (cfg.mage_hp_bonus)
			fprintf(fff, "mage_hp_bonus is applied.\n");
#endif	// 0
		if (cfg.report_to_meta)
			fprintf(fff, "Reporting to the meta-server.\n");
		if (cfg.secret_dungeon_master)
			fprintf(fff, "Dungeon Master is hidden.\n");
		else
			fprintf(fff, "Dungeon Master is *SHOWN*!!\n");
		//	cfg.unique_max_respawn_time
		//	cfg.game_port
		//	cfg.console_port
	}

	/* Close the file */
	my_fclose(fff);

	/* Let the client know to expect some info */
	Send_special_other(Ind);
}

/*
 * Tell players of the # of monsters killed, using temporary file. - Jir -
 */
void do_cmd_show_monster_killed_letter(int Ind, char *letter)
{
	player_type *p_ptr = Players[Ind];

	int		i, j, num, total = 0;
	monster_race	*r_ptr;
	bool	shown = FALSE, all = FALSE;
	byte	mimic = (get_skill_scale(p_ptr, SKILL_MIMIC, 100));
	bool	admin = is_admin(p_ptr);

	FILE *fff;

	/* Paranoia */
	// if (!letter) return;

	/* Open a new file */
	fff = my_fopen(p_ptr->infofile, "w");

	/* Current file viewing */
	strcpy(p_ptr->cur_file, p_ptr->infofile);

	/* Let the player scroll through the info */
	p_ptr->special_file_type = TRUE;


	/* Output color byte */
	fprintf(fff, "\377G");

	if (letter && *letter) fprintf(fff, "======== Killed List for Monster Group '%c' ========\n", *letter);
	else
	{
		all = TRUE;
		fprintf(fff, "======== Killed List ========\n");
	}

	/* for each monster race */
	/* XXX I'm not sure if this list should be sorted.. */
	for (i = 1; i <= MAX_R_IDX; i++)
	{
		r_ptr = &r_info[i];
//		if (letter && *letter != r_ptr->d_char) continue;
		if (!all && !strchr(letter, r_ptr->d_char)) continue;
		num = p_ptr->r_killed[i];
		/* Hack -- always show townie */
		// if (num < 1 && r_ptr->level) continue;
		if (num < 1) continue;
		if (!r_ptr->name) continue;

		/* Let's not show uniques here */
		if (r_ptr->flags1 & RF1_UNIQUE) continue;

		/*if (admin)*/ fprintf(fff, "(%4d) ", i); /* mimics need that number for Polymorph Self Into.. */

		if (mimic && mimic >= r_ptr->level)
		{
			j = r_ptr->level - num;

			if (j > 0)
				fprintf(fff, "%-30s : %d (%d more to go)\n",
						r_name + r_ptr->name, num, j);
			else
			{
				if (p_ptr->body_monster == i)
					fprintf(fff, "\377G%-30s : %d  ** Your current form **\n",
							r_name + r_ptr->name, num);
				else fprintf(fff, "\377U%-30s : %d (learnt)\n",
						r_name + r_ptr->name, num);
			}
		}
		else
		{
			fprintf(fff, "%-30s : %d\n", r_name + r_ptr->name, num);
		}
		total += num;
		shown = TRUE;
	}

	if (!shown) fprintf(fff, "Nothing so far.\n");
	else fprintf(fff, "\nTotal : %d\n", total);

	/* Close the file */
	my_fclose(fff);

	/* Let the client know to expect some info */
	Send_special_other(Ind);
}


/* Tell the player of her/his houses.	- Jir - */
/* TODO: handle HF_DELETED */
void do_cmd_show_houses(int Ind)
{
	player_type *p_ptr = Players[Ind];
	house_type *h_ptr;
	struct dna_type *dna;
	cptr name;

	int		i, total = 0;	//j, num,
	bool	shown = FALSE;
	bool	admin = is_admin(p_ptr);

	FILE *fff;

	/* Paranoia */
	// if (!letter) return;

	/* Open a new file */
	fff = my_fopen(p_ptr->infofile, "w");

	/* Current file viewing */
	strcpy(p_ptr->cur_file, p_ptr->infofile);

	/* Let the player scroll through the info */
	p_ptr->special_file_type = TRUE;


	/* Output color byte */
//	fprintf(fff, "%c", 'G');

	fprintf(fff, "======== House List ========\n");

	for(i=0;i<num_houses;i++)
	{
		//				if(!houses[i].dna->owner) continue;
		//				if(!admin && houses[i].dna->owner != p_ptr->id) continue;
		h_ptr = &houses[i];
		dna = h_ptr->dna;

		if (!access_door(Ind, h_ptr->dna) && !admin_p(Ind)) continue;

		shown = TRUE;
		total++;

		fprintf(fff, "%3d)   [%d,%d] in %s", total,
				h_ptr->dy * 5 / MAX_HGT, h_ptr->dx * 5 / MAX_WID,
				wpos_format(Ind, &h_ptr->wpos));
//				h_ptr->wpos.wz*50, h_ptr->wpos.wx, h_ptr->wpos.wy);

		if (dna->creator == p_ptr->dna)
		{
			/* Take player's CHR into account */
			int factor = adj_chr_gold[p_ptr->stat_ind[A_CHR]];
			int price = dna->price / 100 * factor;

			if (price < 100) price = 100;
			fprintf(fff, "  %dau", price / 2);
		}

		if (admin)
		{
#if 0
			name = lookup_player_name(houses[i].dna->creator);
			if (name) fprintf(fff, "  Creator:%s", name);
			else fprintf(fff, "  Dead's. ID: %d", dna->creator);
#endif	// 0
			name = lookup_player_name(houses[i].dna->owner);
			if (name) fprintf(fff, "  Owner:%s", name);
			else fprintf(fff, "  ID: %ld", dna->owner);
		}

#if 1
		switch(dna->owner_type)
		{
			case OT_PLAYER:
#if 0
				if (dna->owner == dna->creator) break;
				name = lookup_player_name(dna->owner);
				if (name)
				{
					fprintf(fff, "  Legal owner:%s", name);
				}
#endif	// 0
#if 0	// nothig so far.
				else
				{
					s_printf("Found old player houses. ID: %d\n", houses[i].dna->owner);
					kill_houses(houses[i].dna->owner, OT_PLAYER);
				}
#endif	// 0
				break;

			case OT_PARTY:
				name = parties[dna->owner].name;
				if(strlen(name))
				{
					fprintf(fff, "  as %s", name);
				}
#if 0	// nothig so far.
				else
				{
					s_printf("Found old party houses. ID: %d\n", houses[i].dna->owner);
					kill_houses(houses[i].dna->owner, OT_PARTY);
				}
#endif	// 0
				break;
			case OT_CLASS:
				name = class_info[dna->owner].title;
				if(strlen(name))
				{
					fprintf(fff, "  as %s", name);
				}
				break;
			case OT_RACE:
				name = race_info[dna->owner].title;
				if(strlen(name))
				{
					fprintf(fff, "  as %s", name);
				}
				break;
			case OT_GUILD:
				name = guilds[dna->owner].name;
				if(strlen(name))
				{
					fprintf(fff, "  as %s", name);
				}
				break;
		}
#endif	// 0

		fprintf(fff, "\n");
	}

	if (!shown) fprintf(fff, "You're homeless for now.\n");
//	else fprintf(fff, "\nTotal : %d\n", total);

	/* Close the file */
	my_fclose(fff);

	/* Let the client know to expect some info */
	Send_special_other(Ind);
}

/*
 * Tell players of the known items, using temporary file. - Jir -
 */
/*
 * NOTE: we don't show the flavor of already-identified objects
 * since flavors are the same for all the player.
 */
void do_cmd_show_known_item_letter(int Ind, char *letter)
{
	player_type *p_ptr = Players[Ind];

	int		i, j, total = 0;
	object_kind	*k_ptr;
	object_type forge;
	char o_name[80];
	bool all = FALSE;
	bool admin = is_admin(p_ptr);
	s16b idx[max_k_idx];

	FILE *fff;

	/* Paranoia */
	// if (!letter) return;

	/* Open a new file */
	fff = my_fopen(p_ptr->infofile, "w");

	/* Current file viewing */
	strcpy(p_ptr->cur_file, p_ptr->infofile);

	/* Let the player scroll through the info */
	p_ptr->special_file_type = TRUE;


	/* Output color byte */
//	fprintf(fff, "%c", 'G');

	if (letter && *letter) fprintf(fff, "======== Objects known (%c) ========\n", *letter);
	else
	{
		all = TRUE;
		fprintf(fff, "======== Objects known ========\n");
	}

#if 0
	/* for each object kind */
	for (i = 1; i <= MAX_K_IDX; i++)
	{
		k_ptr = &k_info[i];
		if (!k_ptr->name) continue;
		if (!all && *letter != k_ptr->d_char) continue;	// k_char ?
//		if (!object_easy_know(i)) continue;
//		if (!k_ptr->easy_know) continue;
		if (!k_ptr->has_flavor) continue;
		if (!p_ptr->obj_aware[i]) continue;

		/* Create the object */
		invcopy(&forge, i);

		/* Describe the artifact */
		object_desc_store(Ind, o_name, &forge, FALSE, 0);

		/* Hack -- remove {0} */
		j = strlen(o_name);
		o_name[j-4] = '\0';

		if (admin) fprintf(fff, "%3d, %3d) ", k_ptr->tval, k_ptr->sval);

		fprintf(fff, "%s\n", o_name);

		total++;
		shown = TRUE;
	}

	/* for each object kind */
	for (i = 1; i <= MAX_K_IDX; i++)
	{
		k_ptr = &k_info[i];
		if (!k_ptr->name) continue;
		if (letter && *letter != k_ptr->d_char) continue;	// k_char ?
//		if (!object_easy_know(i)) continue;
//		if (!k_ptr->easy_know) continue;
		if (!k_ptr->has_flavor) continue;
		if (p_ptr->obj_aware[i]) continue;
		if (!p_ptr->obj_tried[i]) continue;

		/* Create the object */
		invcopy(&forge, i);

		/* Describe the artifact */
		object_desc(Ind, o_name, &forge, FALSE, 0);

		/* Hack -- remove {0} */
		j = strlen(o_name);
		o_name[j-4] = '\0';

		if (admin) fprintf(fff, "%3d, %3d) ", k_ptr->tval, k_ptr->sval);

		fprintf(fff, "%s\n", o_name);

		total++;
		shown = TRUE;
	}
#endif	// 0

	/* for each object kind */
	for (i = 1; i <= max_k_idx; i++)
	{
		k_ptr = &k_info[i];
		if (!k_ptr->name) continue;
		if (!all && *letter != k_ptr->d_char) continue;	// k_char ?
//		if (!object_easy_know(i)) continue;
//		if (!k_ptr->easy_know) continue;
		if (!k_ptr->has_flavor) continue;
		if (!p_ptr->obj_aware[i]) continue;

		idx[total++] = i;
	}

	if (total)
	{
		/* Setup the sorter */
		ang_sort_comp = ang_sort_comp_tval;
		ang_sort_swap = ang_sort_swap_s16b;

		/* Sort the item list according to value */
		ang_sort(Ind, &idx, NULL, total);

		/* for each object kind */
		for (i = total - 1; i >= 0; i--)
		{
			k_ptr = &k_info[idx[i]];

			/* Create the object */
			invcopy(&forge, idx[i]);

			/* Describe the artifact */
			object_desc_store(Ind, o_name, &forge, FALSE, 0);

			/* Hack -- remove {0} */
			j = strlen(o_name);
			o_name[j-4] = '\0';

			if (admin) fprintf(fff, "(%3d, %3d) ", k_ptr->tval, k_ptr->sval);

			fprintf(fff, "%s\n", o_name);
		}
	}


	if (!total) fprintf(fff, "Nothing so far.\n");
	else fprintf(fff, "\nTotal : %d\n", total);

	fprintf(fff, "\n");

	if (!all) fprintf(fff, "======== Objects tried (%c) ========\n", *letter);
	else fprintf(fff, "======== Objects tried ========\n");

	total = 0;

	/* for each object kind */
	for (i = 1; i <= max_k_idx; i++)
	{
		k_ptr = &k_info[i];
		if (!k_ptr->name) continue;
		if (!all && *letter != k_ptr->d_char) continue;	// k_char ?
//		if (!object_easy_know(i)) continue;
//		if (!k_ptr->easy_know) continue;
		if (!k_ptr->has_flavor) continue;
		if (p_ptr->obj_aware[i]) continue;
		if (!p_ptr->obj_tried[i]) continue;

		idx[total++] = i;
	}

	if (total)
	{
		/* Setup the sorter */
		ang_sort_comp = ang_sort_comp_tval;
		ang_sort_swap = ang_sort_swap_s16b;

		/* Sort the item list according to value */
		ang_sort(Ind, &idx, NULL, total);

		/* for each object kind */
		for (i = total - 1; i >= 0; i--)
		{
			k_ptr = &k_info[idx[i]];

			/* Create the object */
			invcopy(&forge, idx[i]);

			/* Describe the artifact */
			object_desc(Ind, o_name, &forge, FALSE, 0);

			/* Hack -- remove {0} */
			j = strlen(o_name);
			o_name[j-4] = '\0';

			if (admin) fprintf(fff, "(%3d, %3d) ", k_ptr->tval, k_ptr->sval);

			fprintf(fff, "%s\n", o_name);
		}
	}


//	fprintf(fff, "\n");

	if (!total) fprintf(fff, "Nothing so far.\n");
	else fprintf(fff, "\nTotal : %d\n", total);

	/* Close the file */
	my_fclose(fff);

	/* Let the client know to expect some info */
	Send_special_other(Ind);
}

/*
 * Check the status of traps
 */
void do_cmd_knowledge_traps(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int k;

	FILE *fff;

	trap_kind *t_ptr;

	int	total = 0;
	bool shown = FALSE;
	bool admin = is_admin(p_ptr);

	/* Open a new file */
	fff = my_fopen(p_ptr->infofile, "w");

	/* Current file viewing */
	strcpy(p_ptr->cur_file, p_ptr->infofile);

	/* Let the player scroll through the info */
	p_ptr->special_file_type = TRUE;

	fprintf(fff, "======== known traps ========\n");

	/* Scan the traps */
	for (k = 0; k < MAX_T_IDX; k++)
	{
		/* Get the trap */
		t_ptr = &t_info[k];

		/* Skip "empty" traps */
		if (!t_ptr->name) continue;

		/* Skip unidentified traps */
		if(!p_ptr->trap_ident[k]) continue;

		if (admin) fprintf(fff, "%3d)", k);

		/* Hack -- Build the trap name */
		fprintf(fff, "     %s\n", t_name + t_ptr->name);

		total++;
		shown = TRUE;
	}

	fprintf(fff, "\n");

	if (!shown) fprintf(fff, "Nothing so far.\n");
	else fprintf(fff, "\nTotal : %d\n", total);

	/* Close the file */
	my_fclose(fff);

	/* Let the client know to expect some info */
	Send_special_other(Ind);
}

/*
 * Display the time and date
 */
void do_cmd_time(Ind)
{
	player_type *p_ptr = Players[Ind];

	int day = bst(DAY, turn);

	int hour = bst(HOUR, turn);

	int min = bst(MINUTE, turn);

	int full = hour * 100 + min;

	char buf2[20];

	int start = 9999;

	int end = -9999;

	int num = 0;

	char desc[1024];

	char buf[1024];

	FILE *fff;


	/* Note */
	strcpy(desc, "It is a strange time.");

	/* Format time of the day */
	strnfmt(buf2, 20, get_day(bst(YEAR, turn) + START_YEAR));

	/* Display current date in the Elvish calendar */
	msg_format(Ind, "This is %s of the %s year of the third age.",
	           get_month_name(day, is_admin(p_ptr), FALSE), buf2);

	/* Message */
	msg_format(Ind, "The time is %d:%02d %s.",
				 (hour % 12 == 0) ? 12 : (hour % 12),
				 min, (hour < 12) ? "AM" : "PM");

#if CHATTERBOX_LEVEL > 2
	/* Find the path */
	if (!rand_int(10) || p_ptr->image)
	{
		path_build(buf, 1024, ANGBAND_DIR_TEXT, "timefun.txt");
	}
	else
	{
		path_build(buf, 1024, ANGBAND_DIR_TEXT, "timenorm.txt");
	}

	/* Open this file */
	fff = my_fopen(buf, "rt");

	/* Oops */
	if (!fff) return;

	/* Find this time */
	while (!my_fgets(fff, buf, 1024, FALSE))
	{
		/* Ignore comments */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Ignore invalid lines */
		if (buf[1] != ':') continue;

		/* Process 'Start' */
		if (buf[0] == 'S')
		{
			/* Extract the starting time */
			start = atoi(buf + 2);

			/* Assume valid for an hour */
			end = start + 59;

			/* Next... */
			continue;
		}

		/* Process 'End' */
		if (buf[0] == 'E')
		{
			/* Extract the ending time */
			end = atoi(buf + 2);

			/* Next... */
			continue;
		}

		/* Ignore incorrect range */
		if ((start > full) || (full > end)) continue;

		/* Process 'Description' */
		if (buf[0] == 'D')
		{
			num++;

			/* Apply the randomizer */
			if (!rand_int(num)) strcpy(desc, buf + 2);

			/* Next... */
			continue;
		}
	}

	/* Message */
	msg_print(Ind, desc);

	/* Close the file */
	my_fclose(fff);
#endif	// 0
}

/*
 * Prepare to view already-existing text file. full path is needed.
 * do_cmd_check_other is called after the client is ready.	- Jir -
 *
 * Unlike show_file and do_cmd_help_aux, this can display the file
 * w/o request from client, ie. no new packet definition etc. is needed.
 */
void do_cmd_check_other_prepare(int Ind, char *path)
{
	player_type *p_ptr = Players[Ind];

	/* Current file viewing */
	strcpy(p_ptr->cur_file, path);

	/* Let the player scroll through the info */
	p_ptr->special_file_type = TRUE;

	/* Let the client know to expect some info */
	Send_special_other(Ind);
}


/*
 * Scroll through *ID* or Self Knowledge information.
 */
//void do_cmd_check_other(int Ind, int line, int color)
void do_cmd_check_other(int Ind, int line)
{
	player_type *p_ptr = Players[Ind];


	/* Make sure the player is allowed to */
	if (!p_ptr->special_file_type) return;

	/* Display the file contents */
	show_file(Ind, p_ptr->cur_file, "Extra Info", line, 0);
//	show_file(Ind, p_ptr->cur_file, "Extra Info", line, color);

#if 0
	/* Remove the file */
	fd_kill(p_ptr->infofile);

	strcpy(p_ptr->infofile, "");
#endif	// 0
}

#if 0
void do_cmd_check_other(int Ind, int line)
{
	player_type *p_ptr = Players[Ind];

	int n = 0;

	FILE *fff;

	char file_name[MAX_PATH_LENGTH];


	/* Make sure the player is allowed to */
	if (!p_ptr->special_file_type) return;

	/* Temporary file */
	if (path_temp(file_name, MAX_PATH_LENGTH)) return;

	/* Open a new file */
	fff = my_fopen(file_name, "w");

	/* Scan "info" */
	while (n < 128 && p_ptr->info[n] && strlen(p_ptr->info[n]))
	{
		/* Dump a line of info */
		fprintf(fff, p_ptr->info[n]);

		/* Newline */
		fprintf(fff, "\n");

		/* Next line */
		n++;
	}

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(Ind, file_name, "Extra Info", line, 0);

	/* Remove the file */
	fd_kill(file_name);
}
#endif	// 0
