
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
	int i, j, k, z, y, x;

	FILE *fff;

	char file_name[1024];

	char base_name[80];

	bool okay[MAX_A_IDX];


	/* Temporary file */
	if (path_temp(file_name, 1024)) return;

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
		if (!a_ptr->known) continue;

		/* Assume okay */
		okay[k] = TRUE;
	}

#ifdef NEW_DUNGEON
#if 0
	/* Check the dungeon */
	for (Depth = 0; Depth < MAX_DEPTH; Depth++)
	{
		/* Skip uncreated levels */
		if (!cave[Depth]) continue;

		/* Scan this level */
		for (y = 0; y < MAX_HGT; y++)
		{
			for (x = 0; x < MAX_WID; x++)
			{
				cave_type *c_ptr = &cave[Depth][y][x];

				/* Process objects */
				if (c_ptr->o_idx)
				{
					object_type *o_ptr = &o_list[c_ptr->o_idx];

					/* Ignore non-artifacts */
					if (!artifact_p(o_ptr)) continue;

					/* Ignore known items */
					if (object_known_p(Ind, o_ptr)) continue;

					/* Note the artifact */
					okay[o_ptr->name1] = FALSE;
				}
			}
		}
	}
#endif /* evil -temp */
#else
	/* Check the dungeon */
	for (Depth = 0; Depth < MAX_DEPTH; Depth++)
	{
		/* Skip uncreated levels */
		if (!cave[Depth]) continue;

		/* Scan this level */
		for (y = 0; y < MAX_HGT; y++)
		{
			for (x = 0; x < MAX_WID; x++)
			{
				cave_type *c_ptr = &cave[Depth][y][x];

				/* Process objects */
				if (c_ptr->o_idx)
				{
					object_type *o_ptr = &o_list[c_ptr->o_idx];

					/* Ignore non-artifacts */
					if (!artifact_p(o_ptr)) continue;

					/* Ignore known items */
					if (object_known_p(Ind, o_ptr)) continue;

					/* Note the artifact */
					okay[o_ptr->name1] = FALSE;
				}
			}
		}
	}
#endif

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
			if (!artifact_p(o_ptr)) continue;

			/* Ignore known items */
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
		fprintf(fff, "     The %s\n", base_name);
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
	player_type *p_ptr = Players[Ind];
	int k;

	FILE *fff;

	char file_name[1024];

	/* Temporary file */
	if (path_temp(file_name, 1024)) return;

	/* Open a new file */
	fff = my_fopen(file_name, "w");

	/* Scan the monster races */
	for (k = 1; k < MAX_R_IDX-1; k++)
	{
		monster_race *r_ptr = &r_info[k];

		/* Only print Uniques */
		if (r_ptr->flags1 & RF1_UNIQUE)
		{
			/* Only display known uniques */
			if (r_ptr->r_sights && mon_allowed(r_ptr))
			{
				int i, j = 0;
				byte ok = FALSE;
				bool full = FALSE;

				/* Output color byte */
				fprintf(fff, "%c", 'w');

				/* Format message */
//				fprintf(fff, "%s has been killed by:\n", r_name + r_ptr->name);
				fprintf(fff, "%s has been killed by", r_name + r_ptr->name);

				for (i = 1; i <= NumPlayers; i++)
				{
					player_type *q_ptr = Players[i];

					if (q_ptr->r_killed[k])
					{
//						byte attr = 'U';
//						fprintf(fff, "        %s\n", q_ptr->name);
						if (!ok)
						{
							fprintf(fff, ":\n");
							fprintf(fff, "%c", 'B');
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
						fprintf(fff, "%c", attr);
#endif

						fprintf(fff, "  %-16.16s", q_ptr->name);
						j++;
						full = FALSE;
						if (j == 4)
						{
							fprintf(fff, "\n");
							fprintf(fff, "%c", 'B');
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

					fprintf(fff, " Nobody.");
				}
				/* Terminate line */
				/*                              fprintf(fff, "\n");*/
				if (!full) fprintf(fff, "\n");
			}

		}
	}

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(Ind, file_name, "Known Uniques", line, 1);

	/* Remove the file */
	fd_kill(file_name);
}

/*
 * Check the status of "players"
 *
 * The player's name, race, class, and experience level are shown.
 */
void do_cmd_check_players(int Ind, int line)
{
	int k;

	FILE *fff;

	char file_name[1024];

	player_type *p_ptr = Players[Ind];

	/* Temporary file */
	if (path_temp(file_name, 1024)) return;

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
		   (cfg.secret_dungeon_master)) continue;

		/*** Determine color ***/

		/* Print self in green */
		if (Ind == k) attr = 'G';

		/* Print party members in blue */
		else if (p_ptr->party && p_ptr->party == q_ptr->party) attr = 'B';

		/* Print hostile players in red */
		else if (check_hostile(Ind, k)) attr = 'r';

		/* Output color byte */
		fprintf(fff, "%c", attr);

		/* Print a message */
		if(q_ptr->fruit_bat)
		{
			fprintf(fff, "  %s the %s%s %s (%sFruit bat, Lv %d, %s)",
				q_ptr->name, (q_ptr->mode == MODE_HELL)?"hellish ":"", race_info[q_ptr->prace].title, 
				class_info[q_ptr->pclass].title, (q_ptr->total_winner)?((q_ptr->male)?"King, ":"Queen, "):"", q_ptr->lev,
				parties[q_ptr->party].name);
		}
		else
		{
			fprintf(fff, "  %s the %s%s %s (%sLv %d, %s)",
				q_ptr->name, (q_ptr->mode == MODE_HELL)?"hellish ":"", race_info[q_ptr->prace].title, 
				class_info[q_ptr->pclass].title, (q_ptr->total_winner)?((q_ptr->male)?"King, ":"Queen, "):"", q_ptr->lev,
				parties[q_ptr->party].name);
	}

		/* AFK */
		if(q_ptr->afk)
		{
			fprintf(fff, " AFK");
		}
		if(q_ptr->pkill & PKILL_SET)
		{
			fprintf(fff, " PK");
		}
		else if(!(q_ptr->pkill & PKILL_KILLABLE)){
			fprintf(fff, " SAFE");
		}
		else fprintf(fff, "Newbie");
				
		/* Newline */
		/* -AD- will this work? */
		fprintf(fff, "\n");
		if (p_ptr->admin_dm || p_ptr->admin_wiz)
			fprintf(fff, "   %c(%d)", (q_ptr->quest_id?'Q':' '), k);
		fprintf(fff, "     %s@%s", q_ptr->realname, q_ptr->hostname);

		/* Print extra info if these people are in the same party */
		/* Hack -- always show extra info to dungeon master */
		if ((p_ptr->party == q_ptr->party && p_ptr->party) || Ind == k || p_ptr->admin_dm || p_ptr->admin_wiz)
		{
			/* maybe too kind? */
			fprintf(fff, "   {[%d,%d] of %dft(%d,%d)}", q_ptr->panel_row, q_ptr->panel_col, q_ptr->wpos.wz*50, q_ptr->wpos.wx, q_ptr->wpos.wy);

		}
		fprintf(fff, "\n");

	}

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(Ind, file_name, "Player list", line, 1);

	/* Remove the file */
	fd_kill(file_name);
}
 /*
 * Check the equipments of other player.
 */
void do_cmd_check_player_equip(int Ind, int line)
{
	int i, k;

	FILE *fff;

	char file_name[1024];

	player_type *p_ptr = Players[Ind];

	bool admin = p_ptr->admin_wiz || p_ptr->admin_dm;

	/* Temporary file */
	if (path_temp(file_name, 1024)) return;

	/* Open a new file */
	fff = my_fopen(file_name, "w");

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
					 ((q_ptr->lev > p_ptr->lev) || (randint(p_ptr->lev) < (q_ptr->lev / 2))))
					 continue;

			/* Output color byte */
			fprintf(fff, "%c", attr);

			/* Print a message */
				if(q_ptr->fruit_bat)
				{
					fprintf(fff, "  %s the %s%s %s (%sFruit bat, Lv %d, %s)",
					q_ptr->name, (q_ptr->mode == MODE_HELL)?"hellish ":"", race_info[ q_ptr->prace].title,
					class_info[q_ptr->pclass].title, (q_ptr->total_winner)?((q_ptr->male)?"King, ":"Queen, "):"", q_ptr->lev,
					parties[q_ptr->party].name);
				}
				else
				{
					fprintf(fff, "  %s the %s%s %s (%sLv %d, %s)",
					q_ptr->name, (q_ptr->mode == MODE_HELL)?"hellish ":"", race_info[ q_ptr->prace].title,
					class_info[q_ptr->pclass].title, (q_ptr->total_winner)?((q_ptr->male)?"King, ":"Queen, "):"", q_ptr->lev,
					parties[q_ptr->party].name);
				}

				fprintf(fff, "\n");
	      
				/* Print equipments */
				for (i=admin?0:INVEN_WIELD; i<INVEN_TOTAL; i++)
				{
					object_type *o_ptr = &q_ptr->inventory[i];
					char o_name[160];
					if (o_ptr->tval) {
						object_desc(Ind, o_name, o_ptr, TRUE, 3);
						fprintf(fff, "%c %s\n", i < INVEN_WIELD? 'o' : 'w', o_name);
					}
				}

			/* Add blank line */
			fprintf(fff, "%c\n", 'w');

		}

       /* Close the file */
       my_fclose(fff);

       /* Display the file contents */
       show_file(Ind, file_name, "Someone's equipments", line, 1);

       /* Remove the file */
       fd_kill(file_name);
}



/*
 * Scroll through *ID* or Self Knowledge information.
 */
void do_cmd_check_other(int Ind, int line)
{
	player_type *p_ptr = Players[Ind];

	int n = 0;

	FILE *fff;

	char file_name[1024];


	/* Make sure the player is allowed to */
	if (!p_ptr->special_file_type) return;

	/* Temporary file */
	if (path_temp(file_name, 1024)) return;

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
