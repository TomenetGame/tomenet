
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
 * Hack -- redraw the screen
 *
 * This command performs various low level updates, clears all the "extra"
 * windows, does a total redraw of the main window, and requests all of the
 * interesting updates and redraws that I can think of.
 *
 * This doesn't need to be in the server --KLJ--
 */
void do_cmd_redraw(void)
{
}


/*
 * Hack -- change name
 *
 * This isn't allowed anymore --KLJ--
 */
void do_cmd_change_name(void)
{
}


/*
 * Recall the most recent message
 *
 * This should be handled by the client --KLJ--
 */
void do_cmd_message_one(void)
{
}


/*
 * Show previous messages to the user	-BEN-
 *
 * The screen format uses line 0 and 23 for headers and prompts,
 * skips line 1 and 22, and uses line 2 thru 21 for old messages.
 *
 * This command shows you which commands you are viewing, and allows
 * you to "search" for strings in the recall.
 *
 * Note that messages may be longer than 80 characters, but they are
 * displayed using "infinite" length, with a special sub-command to
 * "slide" the virtual display to the left or right.
 *
 * Attempt to only hilite the matching portions of the string.
 *
 * This is in the client code --KLJ--
 */
void do_cmd_messages(void)
{
}


/*
 * Set or unset various options.
 *
 * The user must use the "Ctrl-R" command to "adapt" to changes
 * in any options which control "visual" aspects of the game.
 *
 * Any options that can be changed will be client-only --KLJ--
 */
void do_cmd_options(void)
{
}



/*
 * Ask for a "user pref line" and process it
 *
 * XXX XXX XXX Allow absolute file names?
 *
 * This is client-side --KLJ--
 */
void do_cmd_pref(void)
{
}


/*
 * Interact with "macros"
 *
 * Note that the macro "action" must be defined before the trigger.
 *
 * XXX XXX XXX Need messages for success, plus "helpful" info
 *
 * Macros are handled by the client --KLJ--
 */
void do_cmd_macros(void)
{
}



/*
 * Interact with "visuals"
 *
 * This is (of course) a client-side thing --KLJ--
 */
void do_cmd_visuals(void)
{
}


/*
 * Interact with "colors"
 *
 * This is kind of client-side, but maybe we should allow players to select
 * the color and character of objects, like is done in single-player Angband.
 * Right now, the colors and characters of objects are fixed, but the user
 * will be able to change the overall colors (if his visual modules supports
 * that.)    --KLJ--
 */
void do_cmd_colors(void)
{
}


/*
 * Note something in the message recall
 *
 * This is a (I think) useless command.  It will later be used to "talk" to
 * other players on the same level as the "talker".  --KLJ--
 */
void do_cmd_note(void)
{
}


/*
 * Mention the current version
 *
 * The client handles this, and it also knows the server version, and prints
 * that out as well.  --KLJ--
 */
void do_cmd_version(void)
{
}



/*
 * Array of feeling strings
 */
#if 0
static cptr do_cmd_feeling_text[11] =
{
	"Looks like any other level.",
	"You feel there is something special about this level.",
	"You have a superb feeling about this level.",
	"You have an excellent feeling...",
	"You have a very good feeling...",
	"You have a good feeling...",
	"You feel strangely lucky...",
	"You feel your luck is turning...",
	"You like the look of this place...",
	"This level can't be all bad...",
	"What a boring place..."
};
#endif


/*
 * Note that "feeling" is set to zero unless some time has passed.
 * Note that this is done when the level is GENERATED, not entered.
 *
 * Level feelings are tricky.  Say level 1 (50') has been around for a long
 * time, because there's always been at least one player on it.  Say that
 * it was "special" when it was generated, because the Phial was on it.  But
 * it has long since been picked up, but any players who come later onto this
 * level will still get a "special" feeling.  So, should the level feeling
 * be recomputed whenever it is asked for?  Right now, level feelings are
 * disabled.  --KLJ--
 */
void do_cmd_feeling(void)
{
}





/*
 * Encode the screen colors
 */
/*static char hack[17] = "dwsorgbuDWvyRGBU";*/


/*
 * Hack -- load a screen dump from a file
 *
 * This can be client side --KLJ--
 */
void do_cmd_load_screen(void)
{
}


/*
 * Hack -- save a screen dump to a file
 *
 * This can also be client side --KLJ--
 */
void do_cmd_save_screen(void)
{
}




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
	int i, j, k, z, Depth, y, x;

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

		/* Assume okay */
		okay[k] = TRUE;
	}

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
void do_cmd_check_uniques(int Ind, int line)
{
	int k;

	FILE *fff;

	char file_name[1024];
	cptr killer;


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
			/* Only process uniques */
			if (r_ptr->flags1 & RF1_UNIQUE)
			{
				/* Only display known uniques */
				if (r_ptr->r_sights)
				{
					int i;
					byte ok = FALSE;
			
					/* Format message */
					fprintf(fff, "%s has been killed by:\n", r_name + r_ptr->name);
				
					for (i = 1; i <= NumPlayers; i++)
					{
						player_type *q_ptr = Players[i];
						
						if (q_ptr->r_killed[k])
						{
							fprintf(fff, "        %s\n", q_ptr->name);
							ok = TRUE;
						}
					}
					if (!ok) fprintf(fff, "       Nobody\n");
				}

				/* Terminate line */
				/*				fprintf(fff, "\n");*/
			}
		}
	}

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(Ind, file_name, "Known Uniques", line, 0);

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
		if ((!strcmp(q_ptr->name,cfg_dungeon_master)) &&
		   (cfg_secret_dungeon_master)) continue;

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
			fprintf(fff, "     %s the %s%s %s (%sFruit bat, Level %d, %s)",
				q_ptr->name, (q_ptr->mode == MODE_HELL)?"hellish ":"", race_info[q_ptr->prace].title, 
				class_info[q_ptr->pclass].title, (q_ptr->total_winner)?((q_ptr->male)?"King, ":"Queen, "):"", q_ptr->lev,
				parties[q_ptr->party].name);
		}
		else
		{
			fprintf(fff, "     %s the %s%s %s (%sLevel %d, %s)",
				q_ptr->name, (q_ptr->mode == MODE_HELL)?"hellish ":"", race_info[q_ptr->prace].title, 
				class_info[q_ptr->pclass].title, (q_ptr->total_winner)?((q_ptr->male)?"King, ":"Queen, "):"", q_ptr->lev,
				parties[q_ptr->party].name);
    	}

		/* AFK */
		if(q_ptr->afk)
		{
			fprintf(fff, " AFK");
		}
				
		/* Print extra info if these people are in the same party */
		/* Hack -- always show extra info to dungeon master */
		if ((p_ptr->party == q_ptr->party && p_ptr->party) || (!strcmp(p_ptr->name,cfg_dungeon_master)))
		{
			fprintf(fff, " at %ld ft", q_ptr->dun_depth * 50);
		}

		/* Newline */
		/* -AD- will this work? */
		fprintf(fff, "\n");
		fprintf(fff, "         %s@%s\n", q_ptr->realname, q_ptr->hostname);

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

		bool admin = (!strcmp(p_ptr->name,cfg_admin_wizard)
					|| !strcmp(p_ptr->name,cfg_dungeon_master))?TRUE:FALSE;

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
		        if ((!strcmp(q_ptr->name,cfg_dungeon_master)) &&
		           (cfg_secret_dungeon_master)) continue;

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
//				if ((attr != 'B') && (p_ptr->dun_depth != q_ptr->dun_depth)) continue;
				if ((attr != 'B') && (attr != 'w') && !admin)
				{
                	/* Make sure this player is at this depth */
                	if (p_ptr->dun_depth != q_ptr->dun_depth) continue;

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
					fprintf(fff, "     %s the %s%s %s (%sFruit bat, Level %d, %s)",
					q_ptr->name, (q_ptr->mode == MODE_HELL)?"hellish ":"", race_info[ q_ptr->prace].title,
					class_info[q_ptr->pclass].title, (q_ptr->total_winner)?((q_ptr->male)?"King, ":"Queen, "):"", q_ptr->lev,
					parties[q_ptr->party].name);
				}
				else
				{
					fprintf(fff, "     %s the %s%s %s (%sLevel %d, %s)",
					q_ptr->name, (q_ptr->mode == MODE_HELL)?"hellish ":"", race_info[ q_ptr->prace].title,
					class_info[q_ptr->pclass].title, (q_ptr->total_winner)?((q_ptr->male)?"King, ":"Queen, "):"", q_ptr->lev,
					parties[q_ptr->party].name);
				}

				/*
		        fprintf(fff, "%s the %s %s (Level %d, %s)\n",
		                q_ptr->name, race_info[q_ptr->prace].title,
		                class_info[q_ptr->pclass].title, q_ptr->lev,
		                parties[q_ptr->party].name);
				*/
				fprintf(fff, "\n");
              
				/* Print equipments */
				for (i=INVEN_WIELD; i<INVEN_TOTAL; i++)
				{
					object_type *o_ptr = &q_ptr->inventory[i];
					char o_name[80];
					if (o_ptr->tval) {
						object_desc(Ind, o_name, o_ptr, TRUE, 3);
						fprintf(fff, "%c %s\n", 'w', o_name);
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
