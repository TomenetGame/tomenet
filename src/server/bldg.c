/* $Id$ */
/* File: bldg.c */

/*
 * Purpose: Building commands
 * Created by Ken Wigle for Kangband - a variant of Angband 2.8.3
 * -KMW-
 *
 * Rewritten for Kangband 2.8.3i using Kamband's version of
 * bldg.c as written by Ivan Tkatchev
 *
 * Changed for ZAngband by Robert Ruehlmann
 *
 * Heavily modified for ToME by DarkGod
 *
 * Once more modified for TomeNET by Jir
 */

/* added this for consistency in some (unrelated) header-inclusion,
   it IS a server file, isn't it? */
#define SERVER
#include "angband.h"

#if 0
/* hack as in leave_store in store.c */
static bool leave_bldg = FALSE;

/* remember building location */
static int building_loc = 0;
#endif	// 0



/*
 * A helper function for is_state
 */
static bool is_state_aux(int Ind, store_type *s_ptr, int state)
{
	player_type *p_ptr = Players[Ind];
	owner_type *ot_ptr = &ow_info[s_ptr->owner];


	/* Check race */
	if (ot_ptr->races[state][p_ptr->prace / 32] & (1U << (p_ptr->prace % 32)))
		return (TRUE);

	/* Check class */
	if (ot_ptr->classes[state][p_ptr->pclass / 32] & (1U << (p_ptr->pclass % 32)))
		return (TRUE);

#if 0
	/* Check realms */
	if ((ot_ptr->realms[state][p_ptr->realm1 / 32] & (1U << (p_ptr->realm1 % 32))) ||
	    (ot_ptr->realms[state][p_ptr->realm2 / 32] & (1U << (p_ptr->realm2 % 32))))
		return (TRUE);
#endif	// 0

	/* All failed */
	return (FALSE);
}

/* Display a list of 'achievements' in chronological order - C. Blue */
static void race_legends(int Ind) {
	char path[MAX_PATH_LENGTH];
//	cptr name = "legends.log";
//	(void)do_cmd_help_aux(Ind, name, NULL, line, FALSE, 0);
	path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_DATA, "legends-rev.log");
	do_cmd_check_other_prepare(Ind, path, "Latest Occurances");
}

/*
 * Test if the state accords with the player
 */
bool is_state(int Ind, store_type *s_ptr, int state)
{
	if (state == STORE_NORMAL) {
		if (is_state_aux(Ind, s_ptr, STORE_LIKED)) return (FALSE);
		if (is_state_aux(Ind, s_ptr, STORE_HATED)) return (FALSE);
		return (TRUE);
	} else {
		return (is_state_aux(Ind, s_ptr, state));
	}
}




/*
 * Display a building.
 */
void show_building(int Ind, store_type *s_ptr) {
	char buff[20];
	int i, cost = 0;
	byte action_color;
	char tmp_str[80];
	store_info_type *st_ptr = &st_info[s_ptr->st_idx];
	store_action_type *ba_ptr;

	for (i = 0; i < 6; i++) {
		ba_ptr = &ba_info[st_ptr->actions[i]];

		/* hack: some actions are admin-only (for testing purpose) */
		if (!is_admin(Players[Ind]) && st_ptr->actions[i] == 70) { //BACT_STATIC
			Send_store_action(Ind, i, 0, 0, "", TERM_DARK, '.', 0, 0);
			continue;
		}
#ifndef GLOBAL_DUNGEON_KNOWLEDGE
		if (st_ptr->actions[i] == 73) { //BACT_DUNGEONS
			Send_store_action(Ind, i, 0, 0, "", TERM_DARK, '.', 0, 0);
			continue;
		}
#endif
#ifndef ENABLE_MERCHANT_MAIL
		if (st_ptr->actions[i] == 74 || st_ptr->actions[i] == 75 || st_ptr->actions[i] == 76) { //BACT_SEND_ITEM/BACT_SEND_GOLD/BACT_SEND_ITEM_PAY
			Send_store_action(Ind, i, 0, 0, "", TERM_DARK, '.', 0, 0);
			continue;
		}
#endif

		if (ba_ptr->letter != '.') {
			if (ba_ptr->action_restr == 0) {
				if ((is_state(Ind, s_ptr, STORE_LIKED) && (ba_ptr->costs[STORE_LIKED] == 0)) ||
				    (is_state(Ind, s_ptr, STORE_HATED) && (ba_ptr->costs[STORE_HATED] == 0)) ||
				    (is_state(Ind, s_ptr, STORE_NORMAL) && (ba_ptr->costs[STORE_NORMAL] == 0)))
				{
					action_color = TERM_WHITE;
					buff[0] = '\0';
				} else if (is_state(Ind, s_ptr, STORE_LIKED)) {
					action_color = TERM_L_GREEN;
					cost = ba_ptr->costs[STORE_LIKED];
					strnfmt(buff, 20, "(%dgp)", cost);
				} else if (is_state(Ind, s_ptr, STORE_HATED)) {
					action_color = TERM_RED;
					cost = ba_ptr->costs[STORE_HATED];
					strnfmt(buff, 20, "(%dgp)", cost);
				} else {
					action_color = TERM_YELLOW;
					cost = ba_ptr->costs[STORE_NORMAL];
					strnfmt(buff, 20, "(%dgp)", cost);
				}
			} else if (ba_ptr->action_restr == 1) {
				if ((is_state(Ind, s_ptr, STORE_LIKED) && (ba_ptr->costs[STORE_LIKED] == 0)) ||
				    (is_state(Ind, s_ptr, STORE_NORMAL) && (ba_ptr->costs[STORE_NORMAL] == 0)))
				{
					action_color = TERM_WHITE;
					buff[0] = '\0';
				} else if (is_state(Ind, s_ptr, STORE_LIKED)) {
					action_color = TERM_L_GREEN;
					cost = ba_ptr->costs[STORE_LIKED];
					strnfmt(buff, 20, "(%dgp)", cost);
				} else if (is_state(Ind, s_ptr, STORE_HATED)) {
					action_color = TERM_L_DARK;
					strnfmt(buff, 20, "(closed)");
				} else {
					action_color = TERM_YELLOW;
					cost = ba_ptr->costs[STORE_NORMAL];
					strnfmt(buff, 20, "(%dgp)", cost);
				}
			} else {
				if (is_state(Ind, s_ptr, STORE_LIKED) && (ba_ptr->costs[STORE_LIKED] == 0)) {
					action_color = TERM_WHITE;
					buff[0] = '\0';
				} else if (is_state(Ind, s_ptr, STORE_LIKED)) {
					action_color = TERM_L_GREEN;
					cost = ba_ptr->costs[STORE_LIKED];
					strnfmt(buff, 20, "(%dgp)", cost);
				} else {
					action_color = TERM_L_DARK;
					strnfmt(buff, 20, "(closed)");
				}
			}

			strnfmt(tmp_str, 80, " %c) %s %s", ba_ptr->letter, ba_ptr->name + ba_name, buff);
//			c_put_str(action_color, tmp_str, 21 + (i / 2), 17 + (30 * (i % 2)));
#if 0
			Send_store_action(Ind, j, ba_ptr->action, ba_name + ba_ptr->name,
					ba_ptr->letter, ba_ptr->costs[STORE_LIKED], ba_ptr->flags);
#else
			Send_store_action(Ind, i, ba_ptr->action, st_ptr->actions[i], tmp_str, action_color,
					ba_ptr->letter, cost, ba_ptr->flags);
#endif
		} else {
			Send_store_action(Ind, i, 0, 0, "", TERM_DARK, '.', 0, 0);
		}
	}
}

#if 0

static void reset_tim_flags()
{
	p_ptr->fast = 0;            /* Timed -- Fast */
	p_ptr->slow = 0;            /* Timed -- Slow */
	p_ptr->blind = 0;           /* Timed -- Blindness */
	p_ptr->paralyzed = 0;       /* Timed -- Paralysis */
	p_ptr->confused = 0;        /* Timed -- Confusion */
	p_ptr->afraid = 0;          /* Timed -- Fear */
	p_ptr->image = 0;           /* Timed -- Hallucination */
	p_ptr->poisoned = 0;        /* Timed -- Poisoned */
	p_ptr->cut = 0;             /* Timed -- Cut */
	p_ptr->stun = 0;            /* Timed -- Stun */

	p_ptr->protevil = 0;        /* Timed -- Protection */
	p_ptr->protgood = 0;        /* Timed -- Protection */
	p_ptr->invuln = 0;          /* Timed -- Invulnerable */
	p_ptr->hero = 0;            /* Timed -- Heroism */
	p_ptr->shero = 0;           /* Timed -- Berserk */
	p_ptr->fury = 0;            /* Timed -- Fury */
	p_ptr->shield = 0;          /* Timed -- Shield Spell */
	p_ptr->blessed = 0;         /* Timed -- Blessed */
	p_ptr->tim_invis = 0;       /* Timed -- Invisibility */
	p_ptr->tim_infra = 0;       /* Timed -- Infra Vision */

	p_ptr->oppose_acid = 0;     /* Timed -- oppose acid */
	p_ptr->oppose_elec = 0;     /* Timed -- oppose lightning */
	p_ptr->oppose_fire = 0;     /* Timed -- oppose heat */
	p_ptr->oppose_cold = 0;     /* Timed -- oppose cold */
	p_ptr->oppose_pois = 0;     /* Timed -- oppose poison */

	p_ptr->confusing = 0;       /* Touch of Confusion */
	p_ptr->zeal = 0;	    /* Holy +EA */
	p_ptr->martyr = 0;	    /* Holy martyr / invulnerability */
	p_ptr->martyr_timeout = 0;

	p_ptr->sh_fire_tim = 0;
	p_ptr->sh_cold_tim = 0;
	p_ptr->sh_elec_tim = 0;
}


/*
 * arena commands
 */
static void arena_comm(int cmd)
{
	char tmp_str[80];
	monster_race *r_ptr;
	cptr name;

	switch(cmd) {
		case BACT_ARENA:
		{
			if (p_ptr->arena_number == MAX_ARENA_MONS) {
				clear_bldg(5,19);
				prt("               Arena Victor!", 5, 0);
				prt("Congratulations!  You have defeated all before you.", 7, 0);
				prt("For that, receive the prize: 10,000 gold pieces", 8, 0);
				prt("",10,0);
				prt("", 11, 0);

				gain_au(Ind, 10000, FALSE, FALSE);

				msg_print("Press the space bar to continue");
				msg_print(NULL);
				p_ptr->arena_number++;
			} else if (p_ptr->arena_number > MAX_ARENA_MONS) {
				msg_print("You enter the arena briefly and bask in your glory.");
				msg_print(NULL);
			} else {
				p_ptr->inside_arena = TRUE;
				p_ptr->exit_bldg = FALSE;
				reset_tim_flags();
				p_ptr->leaving = TRUE;
				p_ptr->oldpx = px;
				p_ptr->oldpy = py;
				leave_bldg = TRUE;
			}

			break;
		}

		case BACT_POSTER:
		{
			if (p_ptr->arena_number == MAX_ARENA_MONS)
				msg_print("You are victorious. Enter the arena for the ceremony.");
			else if (p_ptr->arena_number > MAX_ARENA_MONS)
				msg_print("You have won against all foes.");
			else {
				r_ptr = &r_info[arena_monsters[p_ptr->arena_number]];
				name = (r_name + r_ptr->name);
				strnfmt(tmp_str, 80, "Do I hear any challenges against: %s", name);
				msg_print(tmp_str);
				msg_print(NULL);
			}

			break;
		}

		case BACT_ARENA_RULES:
		{
			/* Save screen */
			screen_save();

			/* Peruse the arena help file */
			(void)show_file("arena.txt", NULL, 0, 0, 0);

			/* Load screen */
			screen_load();

			break;
		}
	}
}


/*
 * display fruit for dice slots
 */
static void display_fruit(int row, int col, int fruit)
{
	switch(fruit)
	{
		case 0: /* lemon */
		{
			c_put_str(TERM_YELLOW,"   ####.",row,col);
			c_put_str(TERM_YELLOW,"  #    #",row+1,col);
			c_put_str(TERM_YELLOW," #     #",row+2,col);
			c_put_str(TERM_YELLOW,"#      #",row+3,col);
			c_put_str(TERM_YELLOW,"#      #",row+4,col);
			c_put_str(TERM_YELLOW,"#     # ",row+5,col);
			c_put_str(TERM_YELLOW,"#    #  ",row+6,col);
			c_put_str(TERM_YELLOW,".####   ",row+7,col);
			prt(" Lemon  ",row+8,col);

			break;
		}

		case 1: /* orange */
		{
			c_put_str(TERM_ORANGE,"   ##   ",row,col);
			c_put_str(TERM_ORANGE,"  #..#  ",row+1,col);
			c_put_str(TERM_ORANGE," #....# ",row+2,col);
			c_put_str(TERM_ORANGE,"#......#",row+3,col);
			c_put_str(TERM_ORANGE,"#......#",row+4,col);
			c_put_str(TERM_ORANGE," #....# ",row+5,col);
			c_put_str(TERM_ORANGE,"  #..#  ",row+6,col);
			c_put_str(TERM_ORANGE,"   ##   ",row+7,col);
			prt(" Orange ",row+8,col);

			break;
		}

		case 2: /* sword */
		{
			c_put_str(TERM_SLATE,"   /\\   ",row,col);
			c_put_str(TERM_SLATE,"   ##   ",row+1,col);
			c_put_str(TERM_SLATE,"   ##   ",row+2,col);
			c_put_str(TERM_SLATE,"   ##   ",row+3,col);
			c_put_str(TERM_SLATE,"   ##   ",row+4,col);
			c_put_str(TERM_SLATE,"   ##   ",row+5,col);
			c_put_str(TERM_UMBER," ###### ",row+6,col);
			c_put_str(TERM_UMBER,"   ##   ",row+7,col);
			prt(" Sword  ",row+8,col);

			break;
		}

		case 3: /* shield */
		{
			c_put_str(TERM_SLATE," ###### ",row,col);
			c_put_str(TERM_SLATE,"#      #",row+1,col);
			c_put_str(TERM_SLATE,"# ++++ #",row+2,col);
			c_put_str(TERM_SLATE,"# +==+ #",row+3,col);
			c_put_str(TERM_SLATE,"#  ++  #",row+4,col);
			c_put_str(TERM_SLATE," #    # ",row+5,col);
			c_put_str(TERM_SLATE,"  #  #  ",row+6,col);
			c_put_str(TERM_SLATE,"   ##   ",row+7,col);
			prt(" Shield ",row+8,col);

			break;
		}

		case 4: /* plum */
		{
			c_put_str(TERM_VIOLET,"   ##   ",row,col);
			c_put_str(TERM_VIOLET," ###### ",row+1,col);
			c_put_str(TERM_VIOLET,"########",row+2,col);
			c_put_str(TERM_VIOLET,"########",row+3,col);
			c_put_str(TERM_VIOLET,"########",row+4,col);
			c_put_str(TERM_VIOLET," ###### ",row+5,col);
			c_put_str(TERM_VIOLET,"  ####  ",row+6,col);
			c_put_str(TERM_VIOLET,"   ##   ",row+7,col);
			prt("  Plum  ",row+8,col);

			break;
		}

		case 5: /* cherry */
		{
			c_put_str(TERM_RED,"      ##",row,col);
			c_put_str(TERM_RED,"   ###  ",row+1,col);
			c_put_str(TERM_RED,"  #..#  ",row+2,col);
			c_put_str(TERM_RED,"  #..#  ",row+3,col);
			c_put_str(TERM_RED," ###### ",row+4,col);
			c_put_str(TERM_RED,"#..##..#",row+5,col);
			c_put_str(TERM_RED,"#..##..#",row+6,col);
			c_put_str(TERM_RED," ##  ## ",row+7,col);
			prt(" Cherry ",row+8,col);

			break;
		}
	}
}

#endif	// 0

/*
 * gamble_comm
 */
/*
 * Well it does work, but I want gambles playable 'between players'..
 * - Jir -
 */
static bool gamble_comm(int Ind, int cmd, int gold)
{
	player_type *p_ptr = Players[Ind];
	int roll1, roll2, roll3;
	int choice, odds, win;

	s32b wager;
	s32b maxbet;
	//s32b oldgold;

#if 0
	static const char *fruit[6] = {"Lemon", "Orange", "Sword", "Shield", "Plum", "Cherry"};
	char out_val[160], tmp_str[80], again;
	cptr p;
#endif	// 0

	/* Avoid mad casino spam */
	p_ptr->energy -= level_speed(&p_ptr->wpos);

	//screen_save();

	if (cmd == BACT_GAMBLE_RULES) {
		/* Peruse the gambling help file */
		//(void)show_file(Ind, "gambling.txt", NULL, 0, 0, 0);

		/* Get the filename */
		char    path[MAX_PATH_LENGTH];

		path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_TEXT, "gambling.txt");
		do_cmd_check_other_prepare(Ind, path, "Gambling Rules");
	} else {
		//clear_bldg(5, 23);

		/* Set maximum bet */
		if (p_ptr->lev < 10)
			maxbet = (p_ptr->lev * 100);
		else
			maxbet = (p_ptr->lev * 1000);

		/* Get the wager */
#if 0
		strcpy(out_val, "");
		strnfmt(tmp_str, 80, "Your wager (1-%d) ? ", maxbet);
		get_string(tmp_str, out_val,32);

		/* Strip spaces */
		for (p = out_val; *p == ' '; p++);

		wager = atol(p);
#endif	// 0

		wager = gold;

		if (wager > p_ptr->au) {
			msg_print(Ind, "Hey! You don't have the gold - get out of here!");
			store_kick(Ind, FALSE);
			//screen_load();
			return(FALSE);
		} else if (wager > maxbet) {
			msg_format(Ind, "I'll take $%d of that. Keep the rest.", maxbet);
			wager = maxbet;
		} else if (wager < 1) {
			msg_print(Ind, "Ok, we'll start with 1 Au.");
			wager = 1;
		}
		//msg_print(Ind, NULL);
		win = FALSE;
		odds = 0;
#if 0
		oldgold = p_ptr->au;
		strnfmt(tmp_str, 80, "Gold before game: %10d", oldgold);
		prt(tmp_str,20,2);

		strnfmt(tmp_str, 80, "Current Wager:    %10d", wager);
		prt(tmp_str,21,2);
#endif	// 0

		do {
			switch(cmd) {
			case BACT_IN_BETWEEN: /* Game of In-Between */
				//c_put_str(TERM_GREEN, "In Between",5,2);
				msg_print(Ind, "\377GIn Between");
				odds = 3;
				win = FALSE;
				roll1 = randint(10);
				roll2 = randint(10);
				choice = randint(10);
#ifdef USE_SOUND_2010
				sound(Ind, "dice_roll", NULL, SFX_TYPE_MISC, FALSE);//same for 'draw' and 'deal' actually
#endif
				msg_format(Ind, "Black die: \377s%d\377w     Black Die: \377s%d",
				    roll1, roll2);
				msg_format(Ind, "          Red die: \377r%d", choice);
				if (((choice > roll1) && (choice < roll2)) ||
				    ((choice < roll1) && (choice > roll2)))
					win = TRUE;

				if (win == TRUE) s_printf("CASINO: In Between - Player '%s' won %d Au.\n", p_ptr->name, odds * wager);
				else s_printf("CASINO: In Between - Player '%s' lost %d Au.\n", p_ptr->name, wager);
				break;

			case BACT_CRAPS:  /* Game of Craps - requires the good new RNG :) */
				msg_print(Ind, "\377GCraps");
				win = 3;
				odds = 1;
				roll1 = randint(6);
				roll2 = randint(6);
				roll3 = roll1 +  roll2;
				choice = roll3;
#ifdef USE_SOUND_2010
				sound(Ind, "dice_roll", NULL, SFX_TYPE_MISC, FALSE);//same for 'draw' and 'deal' actually
#endif
				msg_format(Ind, "First roll:   \377s%d %d\377w   Total: \377y%d", roll1,
				    roll2, roll3);
				if ((roll3 == 7) || (roll3 == 11))
					win = TRUE;
				else if ((roll3 == 2) || (roll3 == 3) || (roll3 == 12))
					win = FALSE;
				else {
					do {
						//msg_print(Ind, "Hit any key to roll again");
						//msg_print(Ind, NULL);
						msg_print(Ind, "Rerolling..");
						roll1 = randint(6);
						roll2 = randint(6);
						roll3 = roll1 +  roll2;

						msg_format(Ind, "Roll result:  \377s%d %d\377w   Total: \377s%d",
						    roll1, roll2, roll3);

						if (roll3 == choice) {
							win = TRUE;
						} else if (roll3 == 7) {
							win = FALSE;
						} else {
							/* reroll */
						}
					} while ((win != TRUE) && (win != FALSE));
				}

				if (win == TRUE) s_printf("CASINO: Craps - Player '%s' won %d Au.\n", p_ptr->name, odds * wager);
				else s_printf("CASINO: Craps - Player '%s' lost %d Au.\n", p_ptr->name, wager);
				break;

#if 0
			case BACT_SPIN_WHEEL:  /* Spin the Wheel Game */
				win = FALSE;
				odds = 10;
				c_put_str(TERM_GREEN,"Wheel", 5, 2);
				prt("0  1  2  3  4  5  6  7  8  9", 7, 5);
				prt("--------------------------------", 8, 3);
				strcpy(out_val, "");
				get_string ("Pick a number (1-9): ", out_val, 32);
				for (p = out_val; *p == ' '; p++);
				choice = atol(p);
				if (choice < 0) {
					msg_print(Ind, "I'll put you down for 0.");
					choice = 0;
				} else if (choice > 9) {
					msg_print(Ind, "Ok, I'll put you down for 9.");
					choice = 9;
				}
				msg_print(Ind, NULL);
				roll1 = randint(10) - 1;
				strnfmt(tmp_str, 80, "The wheel spins to a stop and the winner is %d",
				    roll1);
				prt(tmp_str,13,3);
				prt("", 9, 0);
				prt("*", 9, (3 * roll1 + 5));
				if (roll1 == choice)
					win = TRUE;

				if (win == TRUE) s_printf("CASINO: Spin the Wheel - Player '%s' won %d Au.\n", p_ptr->name, odds * wager);
				else s_printf("CASINO: Spin the Wheel - Player '%s' lost %d Au.\n", p_ptr->name, wager);
				break;

			case BACT_DICE_SLOTS: /* The Dice Slots */
				c_put_str(TERM_GREEN,"Dice Slots",5,2);
				win = FALSE;
				roll1 = randint(6);
				roll2 = randint(6);
				choice = randint(6);
				strnfmt(tmp_str, 80, "%s %s %s",
					    fruit[roll1-1], fruit[roll2-1],
					    fruit[choice-1]);
				prt(tmp_str,15,37);
				prt("/--------------------------\\",7,2);
				prt("\\--------------------------/",17,2);
				display_fruit(8,  3, roll1-1);
				display_fruit(8, 12, roll2-1);
				display_fruit(8, 21, choice-1);
				if ((roll1 == roll2) && (roll2 == choice)) {
					win = TRUE;
					if (roll1 == 1)
						odds = 4;
					else if (roll1 == 2)
						odds = 6;
					else
						odds = roll1 * roll1;
				} else if ((roll1 == 6) && (roll2 == 6)) {
					win = TRUE;
					odds = choice + 1;
				}

				if (win == TRUE) s_printf("CASINO: Dice Slots - Player '%s' won %d Au.\n", p_ptr->name, odds * wager);
				else s_printf("CASINO: Dice Slots - Player '%s' lost %d Au.\n", p_ptr->name, wager);
				break;
#endif /* 0 */
			}

			if (win) {
				msg_print(Ind, "YOU WON");
				/* hack: prevent s32b overflow */
				if (2000000000 - (odds * wager) < p_ptr->au) {
					msg_format(Ind, "\377yYou cannot carry more than 2 billion worth of gold!");
				} else {
					p_ptr->au = p_ptr->au + (odds * wager);
					/* Prevent a very far-fetched IDDC/Highlander exploit ^^ */
					if (!p_ptr->max_exp) {
						msg_print(Ind, "You gain a tiny bit of experience from gambling.");
						gain_exp(Ind, 1);
					}
					msg_format(Ind, "Payoff: %d", odds);
				}
			} else {
				msg_print(Ind, "You lost");
				p_ptr->au = p_ptr->au - wager;
			}
			Send_gold(Ind, p_ptr->au, p_ptr->balance);
#if 0
			strnfmt(tmp_str, 80, "Current Gold:     %10d", p_ptr->au);
			prt(tmp_str, 22, 2);
			prt("Again(Y/N)?", 18, 37);
			move_cursor(18, 49);
			again = inkey();
			if (wager > p_ptr->au) {
				msg_print(Ind, "Hey! You don't have the gold - get out of here!");
				msg_print(Ind, NULL);
				screen_load();
				return(FALSE);
/*				strnfmt(tmp_str, 80, "Current Wager:    %10d",wager);
				prt(tmp_str, 17, 2); */
			}
#endif /* 0 */
//		} while ((again == 'y') || (again == 'Y'));
		} while (FALSE);

#if 0
		prt("", 18, 37);
		if (p_ptr->au >= oldgold)
			msg_print(Ind, "You came out a winner! We'll win next time, I'm sure.");
		else
			msg_print(Ind, "You lost gold! Haha, better head home.");
		msg_print(Ind, NULL);
#endif /* 0 */
	}

	//screen_load();
	return(TRUE);
}


/*
 * inn commands
 * Note that resting for the night was a perfect way to avoid player
 * ghosts in the town *if* you could only make it to the inn in time (-:
 * Now that the ghosts are temporarily disabled in 2.8.X, this function
 * will not be that useful.  I will keep it in the hopes the player
 * ghost code does become a reality again. Does help to avoid filthy urchins.
 * Resting at night is also a quick way to restock stores -KMW-
 */
static bool inn_comm(int Ind, int cmd)
{
	player_type *p_ptr = Players[Ind];
	bool vampire = FALSE;


#if 0
	/* Extract race info */
	vampire = ((PRACE_FLAG(PR1_VAMPIRE)) || (p_ptr->mimic_form == MIMIC_VAMPIRE));
#endif	// 0
	vampire = p_ptr->suscep_life;

	switch(cmd)
	{
		case BACT_FOOD: /* Buy food & drink */
		{
			if (!vampire)
			{
				msg_print(Ind, "The barkeep gives you some gruel and a beer.");
				// msg_print(Ind, NULL);
				(void) set_food(Ind, PY_FOOD_MAX - 1);
			}
			else {
				msg_print(Ind, "You're a vampire and I don't have any food for you!");
				return(FALSE);
			}

			break;
		}

		/*
		 * I revamped this... Don't know why normal races didn't get
		 * mana regenerated. It is the grand tradition of p&p games -- pelpel
		 */
		/*
		 * XXX big problem.. we cannot modify 'turn'.. lol	- Jir -
		 */
		case BACT_REST: /* Rest for the night */
		{
#if 0
			bool nighttime;

			/* Extract the current time */
			nighttime = ((bst(HOUR, turn) < SUNRISE) || (bst(HOUR, turn) >= NIGHTFALL));

			/* Normal races rest at night */
			if (!vampire && !nighttime)
			{
				msg_print(Ind, "The rooms are available only at night.");
				msg_print(Ind, NULL);
				return(FALSE);
			}

			/* Vampires rest during daytime */
			if (vampire && nighttime)
			{
				msg_print(Ind, "The rooms are available only at daylight for the Undeads.");
				msg_print(Ind, NULL);
				return(FALSE);
			}
#endif	// 0

			/* Must cure HP draining status first */
			if ((p_ptr->poisoned > 0) || (p_ptr->cut > 0))
			{
				msg_print(Ind, "You need a healer, not a room.");
				// msg_print(Ind, NULL);
				msg_print(Ind, "Sorry, but don't want anyone dying in here.");
				return(FALSE);
			}

#if 0
			/* Let the time pass XXX XXX XXX */
			if (vampire)
			{
				/* Wait for sunset */
				while ((bst(HOUR, turn) >= SUNRISE) && (bst(HOUR, turn) < NIGHTFALL))
				{
					turn += (10L * MINUTE);
				}
			}
			else
			{
				/* Wait for sunrise */
				while ((bst(HOUR, turn) < SUNRISE) || (bst(HOUR, turn) >= NIGHTFALL))
				{
					turn += (10L * MINUTE);
				}
			}
#endif	// 0

			/* Regen */
			p_ptr->chp = p_ptr->mhp;
			p_ptr->csp = p_ptr->msp;

			/* Restore status */
			set_blind(Ind, 0);
			set_confused(Ind, 0);
			set_stun(Ind, 0);
			//p_ptr->stun = 0;

			/* Message */
			if (vampire) msg_print(Ind, "You awake refreshed for the new night.");
			else msg_print(Ind, "You awake refreshed for the new day.");

#if 0
			/* Dungeon stuff */
			p_ptr->leaving = TRUE;
			p_ptr->oldpx = px;
			p_ptr->oldpy = py;

			/* Select new bounties. */
			select_bounties();
#endif	// 0

			store_kick(Ind, FALSE);

			break;
		}

		case BACT_RUMORS: /* Listen for rumors */
		{
			char rumor[MAX_CHARS_WIDE];
#if 0 /* why this RNG stuff? also, it doesn't work (always same val) */
			/* Set the RNG seed. */
//			Rand_value = turn / (HOUR * 10);
			Rand_value = time(NULL) / 600;
			Rand_quick = TRUE;

			get_rnd_line("rumors.txt", 0, rumor, MAX_CHARS_WIDE);
			bracer_ff(rumor);	/* colour it */
			msg_format(Ind, "%s", rumor);
			// msg_print(Ind, NULL);

			/* Restore RNG */
			Rand_quick = FALSE;
#else
			get_rnd_line("rumors.txt", 0, rumor, MAX_CHARS_WIDE);
			bracer_ff(rumor);	/* colour it */
			msg_format(Ind, "%s", rumor);
#endif
			break;
		}
	}

	return(TRUE);
}


#if 0

/*
 * share gold for thieves
 */
static void share_gold(void)
{
	int i;

	i = (p_ptr->lev * 2) * 10;

	if (!gain_au(Ind, i, FALSE, FALSE)) return;

	msg_format("You collect %d gold pieces", i);
	msg_print(NULL);
}


/*
 * Display quest information
 */
static void get_questinfo(int questnum)
{
	int i;


	/* Print the quest info */
	prt(format("Quest Information (Danger level: %d)", quest[questnum].level), 5, 0);

	prt(quest[questnum].name, 7, 0);

	i = 0;
	while ((i < 10) && (quest[questnum].desc[i][0] != '\0')) {
		c_put_str(TERM_YELLOW, quest[questnum].desc[i++], i + 8, 0);
	}
}


/*
 * Request a quest from the Lord.
 */
static bool castle_quest(int y, int x)
{
	int             plot = 0;

	quest_type      *q_ptr;


	clear_bldg(7,18);

	/* Current plot of the building */
	plot = cave[y][x].special;

	/* Is there a quest available at the building? */
	if ((!plot) || (plots[plot] == QUEST_NULL)) {
		put_str("I don't have a quest for you at the moment.",8,0);
		return FALSE;
	}

	q_ptr = &quest[plots[plot]];

	/* Quest is completed */
	if (q_ptr->status == QUEST_STATUS_COMPLETED) {
		/* Rewarded quest */
		q_ptr->status = QUEST_STATUS_FINISHED;

		process_hooks(HOOK_QUEST_FINISH, "(d)", plots[plot]);

		return (TRUE);
	}

	/* Quest is still unfinished */
	else if (q_ptr->status == QUEST_STATUS_TAKEN)
	{
		put_str("You have not completed your current quest yet!",8,0);
		put_str("Use CTRL-Q to check the status of your quest.",9,0);
		put_str("Return when you have completed your quest.",12,0);

		return (FALSE);
	}
	/* Failed quest */
	else if (q_ptr->status == QUEST_STATUS_FAILED) {
		/* Mark quest as done (but failed) */
		q_ptr->status = QUEST_STATUS_FAILED_DONE;

		process_hooks(HOOK_QUEST_FAIL, "(d)", plots[plot]);

		return (FALSE);
	}
	/* No quest yet */
	else if (q_ptr->status == QUEST_STATUS_UNTAKEN) {
		if (process_hooks(HOOK_INIT_QUEST, "(d)", plots[plot])) return (FALSE);

		q_ptr->status = QUEST_STATUS_TAKEN;

		/* Assign a new quest */
		get_questinfo(plots[plot]);

		/* Add the hooks */
		if (quest[plots[plot]].type ==  HOOK_TYPE_C) quest[plots[plot]].init(plots[plot]);

		return (TRUE);
	}

	return FALSE;
}

/*
 * Displaying town history -KMW-
 */
static void town_history(void)
{
	/* Save screen */
	screen_save();

	/* Peruse the building help file */
	(void)show_file("bldg.txt", NULL, 0, 0, 0);

	/* Load screen */
	screen_load();
}


/*
 * compare_weapon_aux2 -KMW-
 */
static void compare_weapon_aux2(object_type *o_ptr, int numblows, int r, int c, int mult, int bonus, char attr[80], u32b f1, u32b f2, u32b f3, byte color)
{
	char tmp_str[80];

	c_put_str(color,attr,r,c);

	if (o_ptr->tval == TV_BOW || is_ammo(o_ptr->tval))
		strnfmt(tmp_str, 80, "Attack: %d-%d damage",
		    numblows * (((o_ptr->dd * mult) / FACTOR_MULT) + o_ptr->to_d),
	    	    numblows * (((o_ptr->ds * o_ptr->dd * mult) / FACTOR_MULT) + o_ptr->to_d));
	else
		strnfmt(tmp_str, 80, "Attack: %d-%d damage",
		    numblows * (((o_ptr->dd * mult) / FACTOR_MULT) + o_ptr->to_d + bonus),
	    	    numblows * (((o_ptr->ds * o_ptr->dd * mult) / FACTOR_MULT) + o_ptr->to_d + bonus));

	put_str(tmp_str,r,c+8);
	r++;
}


/*
 * compare_weapon_aux1 -KMW-
 */
static void compare_weapon_aux1(object_type *o_ptr, int col, int r)
{
	u32b f1, f2, f3, f4, f5, f6, esp;

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);


	if (f1 & (TR1_SLAY_ANIMAL)) {
		compare_weapon_aux2(o_ptr, p_ptr->num_blow, r++, col, FACTOR_HURT, FLAT_HURT_BONUS, "Animals:",
		                    f1, f2, f3, TERM_YELLOW);
	}
	if (f1 & (TR1_SLAY_ORC)) {
		compare_weapon_aux2(o_ptr, p_ptr->num_blow, r++, col, FACTOR_SLAY, FLAT_SLAY_BONUS, "Orcs:",
		                    f1, f2, f3, TERM_YELLOW);
	}
	if (f1 & (TR1_SLAY_TROLL)) {
		compare_weapon_aux2(o_ptr, p_ptr->num_blow, r++, col, FACTOR_SLAY, FLAT_SLAY_BONUS, "Trolls:",
		                    f1, f2, f3, TERM_YELLOW);
	}
	if (f1 & (TR1_SLAY_GIANT)) {
		compare_weapon_aux2(o_ptr, p_ptr->num_blow, r++, col, FACTOR_SLAY, FLAT_SLAY_BONUS, "Giants:",
		                    f1, f2, f3, TERM_YELLOW);
	}
	if (f1 & (TR1_SLAY_EVIL)) {
		compare_weapon_aux2(o_ptr, p_ptr->num_blow, r++, col, FACTOR_HURT, FLAT_HURT_BONUS, "Evil:",
		                    f1, f2, f3, TERM_YELLOW);
	}
	if (f1 & (TR1_KILL_UNDEAD)) {
		compare_weapon_aux2(o_ptr, p_ptr->num_blow, r++, col, FACTOR_KILL, FLAT_KILL_BONUS, "Undead:",
		                    f1, f2, f3, TERM_YELLOW);
	}
	else if (f1 & (TR1_SLAY_UNDEAD)) {
		compare_weapon_aux2(o_ptr, p_ptr->num_blow, r++, col, FACTOR_SLAY, FLAT_SLAY_BONUS, "Undead:",
		                    f1, f2, f3, TERM_YELLOW);
	}
	if (f1 & (TR1_KILL_DEMON)) {
		compare_weapon_aux2(o_ptr, p_ptr->num_blow, r++, col, FACTOR_KILL, FLAT_KILL_BONUS, "Demons:",
		                    f1, f2, f3, TERM_YELLOW);
	}
	else if (f1 & (TR1_SLAY_DEMON)) {
		compare_weapon_aux2(o_ptr, p_ptr->num_blow, r++, col, FACTOR_SLAY, FLAT_SLAY_BONUS, "Demons:",
		                    f1, f2, f3, TERM_YELLOW);
	}
	if (f1 & (TR1_KILL_DRAGON)) {
		compare_weapon_aux2(o_ptr, p_ptr->num_blow, r++, col, FACTOR_KILL, FLAT_KILL_BONUS, "Dragons:",
		                    f1, f2, f3, TERM_YELLOW);
	}
	else if (f1 & (TR1_SLAY_DRAGON)) {
		compare_weapon_aux2(o_ptr, p_ptr->num_blow, r++, col, FACTOR_SLAY, FLAT_SLAY_BONUS, "Dragons:",
		                    f1, f2, f3, TERM_YELLOW);
	}
	if (f1 & (TR1_BRAND_ACID)) {
		compare_weapon_aux2(o_ptr, p_ptr->num_blow, r++, col, FACTOR_BRAND, FLAT_BRAND_BONUS, "Acid:",
		                    f1, f2, f3, TERM_RED);
	}
	if (f1 & (TR1_BRAND_ELEC)) {
		compare_weapon_aux2(o_ptr, p_ptr->num_blow, r++, col, FACTOR_BRAND, FLAT_BRAND_BONUS, "Elec:",
		                    f1, f2, f3, TERM_RED);
	}
	if (f1 & (TR1_BRAND_FIRE)) {
		compare_weapon_aux2(o_ptr, p_ptr->num_blow, r++, col, FACTOR_BRAND, FLAT_BRAND_BONUS, "Fire:",
		                    f1, f2, f3, TERM_RED);
	}
	if (f1 & (TR1_BRAND_COLD)) {
		compare_weapon_aux2(o_ptr, p_ptr->num_blow, r++, col, FACTOR_BRAND, FLAT_BRAND_BONUS, "Cold:",
		                    f1, f2, f3, TERM_RED);
	}
	if (f1 & (TR1_BRAND_POIS)) {
		compare_weapon_aux2(o_ptr, p_ptr->num_blow, r++, col, FACTOR_BRAND, FLAT_BRAND_BONUS, "Poison:",
		                    f1, f2, f3, TERM_RED);
	}
}


/*
 * list_weapon -KMW-
 */
static void list_weapon(object_type *o_ptr, int row, int col)
{
	char o_name[ONAME_LEN];

	char tmp_str[80];


	object_desc(o_name, o_ptr, TRUE, 0);
	c_put_str(TERM_YELLOW,o_name,row,col);
	strnfmt(tmp_str, 80, "To Hit: %d   To Damage: %d",o_ptr->to_h, o_ptr->to_d);
	put_str(tmp_str,row+1,col);
	strnfmt(tmp_str, 80, "Dice: %d   Sides: %d",o_ptr->dd, o_ptr->ds);
	put_str(tmp_str,row+2,col);
	strnfmt(tmp_str, 80, "Number of Blows: %d", p_ptr->num_blow);
	put_str(tmp_str,row+3,col);
	c_put_str(TERM_YELLOW, "Possible Damage:",row+5,col);
	strnfmt(tmp_str, 80, "One Strike: %d-%d damage",o_ptr->dd + o_ptr->to_d,
	        (o_ptr->ds*o_ptr->dd) + o_ptr->to_d);
	put_str(tmp_str,row+6,col+1);
	strnfmt(tmp_str, 80, "One Attack: %d-%d damage",p_ptr->num_blow*(o_ptr->dd + o_ptr->to_d),
	        p_ptr->num_blow*(o_ptr->ds*o_ptr->dd + o_ptr->to_d));
	put_str(tmp_str,row+7,col+1);
}


/*
 * compare_weapons -KMW-
 */
static bool compare_weapons(void)
{
	int item, item2, i;

	object_type *o1_ptr, *o2_ptr, *orig_ptr;

	object_type *i_ptr;

	cptr q, s;


	clear_bldg(6,18);

	o1_ptr = NULL; o2_ptr = NULL; i_ptr = NULL;

	/* Store copy of original wielded weapon in pack slot */
	i_ptr = &inventory[INVEN_WIELD];
	orig_ptr = &inventory[INVEN_PACK];
	object_copy(orig_ptr, i_ptr);

	i = 6;
	/* Get an item */
	q = "What is your first weapon? ";
	s = "You have nothing to compare.";
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN)))
	{
		inven_item_increase(INVEN_PACK, -1);
		inven_item_optimize(INVEN_PACK);
		return(FALSE);
	}

	/* Get the item (in the pack) */
	if (item >= 0)
		o1_ptr = &inventory[item];

	/* To remove a warning */
	if (((o1_ptr->tval < TV_BOW) || (o1_ptr->tval > TV_AXE)) &&
	    (o1_ptr->tval != TV_MSTAFF))
	{
		msg_print("Not a weapon! Try again.");
		msg_print(NULL);
		inven_item_increase(INVEN_PACK, -1);
		inven_item_optimize(INVEN_PACK);
		return(FALSE);
	}

	/* Get an item */
	q = "What is your second weapon? ";
	s = "You have nothing to compare.";
	if (!get_item(&item2, q, s, (USE_EQUIP | USE_INVEN)))
	{
		inven_item_increase(INVEN_PACK, -1);
		inven_item_optimize(INVEN_PACK);
		return(FALSE);
	}

	/* Get the item (in the pack) */
	if (item2 >= 0) o2_ptr = &inventory[item2];

	/* To remove a warning */
	if (((o2_ptr->tval < TV_BOW) || (o2_ptr->tval > TV_AXE)) &&
	    (o2_ptr->tval != TV_MSTAFF))
	{
		msg_print("Not a weapon! Try again.");
		msg_print(NULL);
		inven_item_increase(INVEN_PACK, -1);
		inven_item_optimize(INVEN_PACK);
		return(FALSE);
	}

	put_str("Based on your current abilities, here is what your weapons will do", 4, 2);

	i_ptr = &inventory[INVEN_WIELD];
	object_copy(i_ptr, o1_ptr);
	calc_boni();

	list_weapon(o1_ptr,i,2);
	compare_weapon_aux1(o1_ptr, 2, i+8);

	i_ptr = &inventory[INVEN_WIELD];
	if (item2 == INVEN_WIELD)
		object_copy(i_ptr, orig_ptr);
	else
		object_copy(i_ptr, o2_ptr);
	calc_boni();

	list_weapon(o2_ptr,i,40);
	compare_weapon_aux1(o2_ptr, 40, i+8);

	i_ptr = &inventory[INVEN_WIELD];
	object_copy(i_ptr, orig_ptr);
	calc_boni();

	inven_item_increase(INVEN_PACK, -1);
	inven_item_optimize(INVEN_PACK);

	put_str("(Only highest damage applies per monster. Special damage not cumulative)",20,0);

	return(TRUE);
}
#endif	// 0


/*
 * general all-purpose fixing routine for items from building personnel
 * sharpen arrows, repair armor, repair weapon
 * -KMW-
 */
static bool fix_item(int Ind, int istart, int iend, int ispecific, bool iac, int ireward, bool set_reward) {
	player_type *p_ptr = Players[Ind];
	int i;

	/* Make it on par with scrolls at level 59 */
	int maxenchant = (p_ptr->lev / 5) + 5, maxenchant_eff;
	/* Avoid top level players getting pestered =P */
	if (maxenchant > 20) maxenchant = 20;

	object_type *o_ptr;
	char tmp_str[ONAME_LEN];
	u32b f1, f2, f3, f4, f5, f6, esp;

#if 0
	if (set_reward && p_ptr->rewards[ireward]) {
		msg_print("You already have been rewarded today.");
		msg_print(NULL);

		return (FALSE);
	}
#endif

#if 0
	if (maxenchant <= 15) msg_format(Ind, "  Based on your skill, we can improve up to +%d", maxenchant);
	else msg_format(Ind, "  Based on your skill, we can improve up to +%d (+15 for ammunition)", maxenchant);
#endif

	for (i = istart; i <= iend; i++) {
		o_ptr = &p_ptr->inventory[i];

		/* eligible item in this equipment slot? */
		if (!o_ptr->tval) continue;
		if (!is_enchantable(o_ptr)) continue;

		if (ispecific > 0 && o_ptr->tval != ispecific) continue;

		/* Artifacts cannot be enchanted. */
		if (artifact_p(o_ptr)) continue;

		/* Extract the flags */
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

		/* Unenchantable items always fail */
		if (f5 & TR5_NO_ENCHANT) continue;

		/* No easy fix for nothingness/morgul items */
		if (cursed_p(o_ptr)) continue;

		object_desc(Ind, tmp_str, o_ptr, FALSE, 1);

		maxenchant_eff = maxenchant;
		if ((is_ammo(o_ptr->tval)
#ifndef NEW_SHIELDS_NO_AC
		    || o_ptr->tval == TV_SHIELD
#endif
		    )
		    && (maxenchant_eff > 15))
			maxenchant_eff = 15; /* CAP_ITEM_BONI */

		if (iac) {
			if (!is_armour(o_ptr->tval)) continue;
			if (o_ptr->to_a >= maxenchant_eff) continue;
			o_ptr->to_a = maxenchant_eff;
			if (o_ptr->level < maxenchant_eff) o_ptr->level = maxenchant_eff;
			msg_format(Ind, "Your %s has been enchanted to [+%d].", tmp_str, maxenchant_eff);
			break;
		} else {
			if (!is_weapon(o_ptr->tval) && !is_ammo(o_ptr->tval)) continue;
			if (o_ptr->to_h >= maxenchant_eff && o_ptr->to_d >= maxenchant_eff) continue;
			if (o_ptr->to_h < maxenchant_eff) o_ptr->to_h = maxenchant_eff;
			if (o_ptr->to_d < maxenchant_eff) o_ptr->to_d = maxenchant_eff;
			if (o_ptr->level < maxenchant_eff) o_ptr->level = maxenchant_eff;
			msg_format(Ind, "Your %s %s been enchanted to (+%d,+%d).",
			    tmp_str, o_ptr->number == 1 ? "has" : "have", o_ptr->to_h, o_ptr->to_d);
			break;
		}
	}

	if (i > iend) {
		msg_print(Ind, "You don't have anything appropriate.");
		return (FALSE);
	}
s_printf("BACT_ENCHANT: %s enchanted %s\n", p_ptr->name, tmp_str);
#if 0
	if (set_reward) p_ptr->rewards[ireward] = TRUE;
#endif

	/* We might be wearing/wielding the item */
	p_ptr->update |= PU_BONUS;

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
	return (TRUE);
}


#if 0
/*
 * Research Item
 */
static bool research_item(void)
{
	clear_bldg(5,18);
	identify_fully();
	return (TRUE);
}


/*
 * Show the current quest monster.
 */
static void show_quest_monster(void)
{
	monster_race* r_ptr = &r_info[bounties[0][0]];


	msg_format("Quest monster: %s. "
	           "Need to turn in %d corpse%s to receive reward.",
	           r_name + r_ptr->name, bounties[0][1],
	           (bounties[0][1] > 1 ? "s" : ""));
	msg_print(NULL);
}


/*
 * Show the current bounties.
 */
static void show_bounties(void)
{
	int i, j = 6;
	monster_race* r_ptr;
	char buff[80];

	clear_bldg(7,18);
	c_prt(TERM_YELLOW, "Currently active bounties:", 4, 2);

	for (i = 1; i < MAX_BOUNTIES; i++, j++) {
		r_ptr = &r_info[bounties[i][0]];
		strnfmt(buff, 80, "%-30s (%d gp)", r_name + r_ptr->name, bounties[i][1]);
		prt(buff, j, 2);

		if (j >= 17) {
			msg_print("Press space for more.");
			msg_print(NULL);

			clear_bldg(7,18);
			j = 5;
		}
	}
}


/*
 * Filter for corpses that currently have a bounty on them.
 */
static bool item_tester_hook_bounty(object_type* o_ptr)
{
	int i;


	if (o_ptr->tval == TV_CORPSE) {
		for (i = 1; i < MAX_BOUNTIES; i++) {
			if (bounties[i][0] == o_ptr->pval2) return (TRUE);
		}
	}

	return (FALSE);
}

/* Filter to match the quest monster's corpse. */
static bool item_tester_hook_quest_monster(object_type* o_ptr)
{
	if ((o_ptr->tval == TV_CORPSE) &&
		(o_ptr->pval2 == bounties[0][0])) return (TRUE);
	return (FALSE);
}


/*
 * Return the boost in the corpse's value depending on how rare the body
 * part is.
 */
static int corpse_value_boost(int sval)
{
	switch (sval) {
		case SV_CORPSE_HEAD:
		case SV_CORPSE_SKULL:
			return (1);
		/* Default to no boost. */
		default:
			return (0);
	}
}

/*
 * Sell a corpse, if there's currently a bounty on it.
 */
static void sell_corpses(void)
{
	object_type* o_ptr;
	int i, boost = 0;
	s16b value;
	int item;

	/* Set the hook. */
	item_tester_hook = item_tester_hook_bounty;

	/* Select a corpse to sell. */
	if (!get_item(&item, "Sell which corpse",
	              "You have no corpses you can sell.", USE_INVEN)) return;

	o_ptr = &inventory[item];

	/* Exotic body parts are worth more. */
	boost = corpse_value_boost(o_ptr->sval);

	/* Try to find a match. */
	for (i = 1; i < MAX_BOUNTIES; i++) {
		if (o_ptr->pval2 == bounties[i][0]) {
			value = bounties[i][1] + boost*(r_info[o_ptr->pval2].level);

			if (!gain_au(Ind, value, FALSE, FALSE)) return;

			msg_format("Sold for %d gold pieces.", value);
			msg_print(NULL);

			/* Increase the number of collected bounties */
			total_bounties++;

			inven_item_increase(item, -1);
			inven_item_describe(item);
			inven_item_optimize(item);

			return;
		}
	}

	msg_print("Sorry, but that monster does not have a bounty on it.");
	msg_print(NULL);
}



/*
 * Hook for bounty monster selection.
 */
static bool mon_hook_bounty(int r_idx)
{
	return (lua_moon_hook_bounty(r_idx));
}


static void select_quest_monster(void)
{
	monster_race* r_ptr;
	int amt;

	/*
	 * Set up the hooks -- no bounties on uniques or monsters
	 * with no corpses
	 */
	get_mon_num_hook = mon_hook_bounty;
	get_mon_num_prep(0, NULL);

	/* Set up the quest monster. */
	bounties[0][0] = get_mon_num(p_ptr->lev, p_ptr->lev);

	r_ptr = &r_info[bounties[0][0]];

	/*
	 * Select the number of monsters needed to kill. Groups and
	 * breeders require more
	 */
	amt = randnor(5, 3);

	if (amt < 2) amt = 2;

	if (r_ptr->flags1 & RF1_FRIEND)  amt *= 3; amt /= 2;
	if (r_ptr->flags1 & RF1_FRIENDS) amt *= 2;
	if (r_ptr->flags4 & RF4_MULTIPLY) amt *= 3;

	if (r_ptr->flags7 & RF7_AQUATIC) amt /= 2;

	bounties[0][1] = amt;

	/* Undo the filters */
	get_mon_num_hook = dungeon_aux;
}



/*
 * Sell a corpse for a reward.
 */
static void sell_quest_monster(void)
{
	object_type* o_ptr;
	int item;

	/* Set the hook. */
	item_tester_hook = item_tester_hook_quest_monster;

	/* Select a corpse to sell. */
	if (!get_item(&item, "Sell which corpse",
	              "You have no corpses you can sell.", USE_INVEN)) return;

	o_ptr = &inventory[item];
	bounties[0][1] -= o_ptr->number;

	/* Completed the quest. */
	if (bounties[0][1] <= 0) {
		int m;
		monster_race *r_ptr;

		cmsg_print(TERM_YELLOW, "You have completed your quest!");
		msg_print(NULL);

		/* Give full knowledge */

		/* Hack -- Maximal info */
		r_ptr = &r_info[bounties[0][0]];

		msg_format("Well done! As a reward I'll teach you everything "
		                 "about the %s, (check your recall)",
		                 r_name + r_ptr->name);

#ifdef OLD_MONSTER_LORE
		r_ptr->r_wake = r_ptr->r_ignore = MAX_UCHAR;

		/* Observe "maximal" attacks */
		for (m = 0; m < 4; m++) {
			/* Examine "actual" blows */
			if (r_ptr->blow[m].effect || r_ptr->blow[m].method) {
				/* Hack -- maximal observations */
				r_ptr->r_blows[m] = MAX_UCHAR;
			}
		}

		/* Hack -- maximal drops // todo: correctly account for DROP_nn/DROP_n flags */
		r_ptr->r_drop_gold = r_ptr->r_drop_item =
			(((r_ptr->flags1 & (RF1_DROP_4D2)) ? 8 : 0) +
			 ((r_ptr->flags1 & (RF1_DROP_3D2)) ? 6 : 0) +
			 ((r_ptr->flags1 & (RF1_DROP_2D2)) ? 4 : 0) +
			 ((r_ptr->flags0 & (RF0_DROP_2))   ? 2 : 0) +
			 ((r_ptr->flags1 & (RF1_DROP_1D2)) ? 2 : 0) +
			 ((r_ptr->flags0 & (RF0_DROP_1))   ? 1 : 0) +
			 ((r_ptr->flags1 & (RF1_DROP_90))  ? 1 : 0) +
			 ((r_ptr->flags1 & (RF1_DROP_60))  ? 1 : 0));

		/* Hack -- but only "valid" drops */
		if (r_ptr->flags1 & (RF1_ONLY_GOLD)) r_ptr->r_drop_item = 0;
		if (r_ptr->flags1 & (RF1_ONLY_ITEM)) r_ptr->r_drop_gold = 0;

		/* Hack -- observe many spells */
		r_ptr->r_cast_innate = MAX_UCHAR;
		r_ptr->r_cast_spell = MAX_UCHAR;

		/* Hack -- know all the flags */
		r_ptr->r_flags1 = r_ptr->flags1;
		r_ptr->r_flags2 = r_ptr->flags2;
		r_ptr->r_flags3 = r_ptr->flags3;
		r_ptr->r_flags4 = r_ptr->flags4;
		r_ptr->r_flags5 = r_ptr->flags5;
		r_ptr->r_flags6 = r_ptr->flags6;
		r_ptr->r_flags4 = r_ptr->flags7;
		r_ptr->r_flags5 = r_ptr->flags8;
		r_ptr->r_flags6 = r_ptr->flags9;
#endif

		msg_print(NULL);
		select_quest_monster();

	} else {
		msg_format("Well done, only %d more to go.", bounties[0][1]);
		msg_print(NULL);
	}

	inven_item_increase(item, -1);
	inven_item_describe(item);
	inven_item_optimize(item);
}



/*
 * Fill the bounty list with monsters.
 */
void select_bounties(void)
{
	int i, j;


	select_quest_monster();

	/*
	 * Set up the hooks -- no bounties on uniques or monsters
	 * with no corpses
	 */
	get_mon_num_hook = mon_hook_bounty;
	get_mon_num_prep(0, NULL);

	for (i = 1; i < MAX_BOUNTIES; i++) {
		int lev = i * 5 + randnor(0, 2);
		monster_race* r_ptr;
		s16b r_idx;
		s16b val;

		if (lev < 1) lev = 1;
		if (lev >= MAX_DEPTH) lev = MAX_DEPTH-1;

		/* We don't want to duplicate entries in the list */
		while (TRUE) {
			r_idx = get_mon_num(lev, lev);
			for (j = 0; j < i; j++)
				if (bounties[j][0] == r_idx) continue;
			break;
		}

		bounties[i][0] = r_idx;
		r_ptr = &r_info[r_idx];
		val = r_ptr->mexp + r_ptr->level*20 + randnor(0, r_ptr->level*2);
		if (val < 1) val = 1;
		bounties[i][1] = val;
	}

	/* Undo the filters. */
	get_mon_num_hook = dungeon_aux;
}
#endif	// 0

/*
 * Execute a building command
 */
//bool bldg_process_command(store_type *st_ptr, int i)
bool bldg_process_command(int Ind, store_type *st_ptr, int action, int item, int item2, int amt, int gold) {
	player_type *p_ptr = Players[Ind];
	//object_type *q_ptr, forge;
	//store_action_type *ba_ptr = &ba_info[st_info[st_ptr->st_idx].actions[i]];
	store_action_type *ba_ptr = &ba_info[action];
	int bact = ba_ptr->action;
	int bcost;
	bool paid = FALSE;
	bool set_reward = FALSE;
	bool recreate = FALSE;
	//int amt;

	if (p_ptr->store_action) {
		msg_print(Ind, "You are currently busy.");
		return FALSE;
	}

	if (is_state(Ind, st_ptr, STORE_LIKED))
		bcost = ba_ptr->costs[STORE_LIKED];
	else if (is_state(Ind, st_ptr, STORE_HATED))
		bcost = ba_ptr->costs[STORE_HATED];
	else bcost = ba_ptr->costs[STORE_NORMAL];

	/* action restrictions */
	if (((ba_ptr->action_restr == 1) && (!is_state(Ind, st_ptr, STORE_LIKED) ||
	    !is_state(Ind, st_ptr, STORE_NORMAL))) ||
	    ((ba_ptr->action_restr == 2) && (!is_state(Ind, st_ptr, STORE_LIKED))))
	{
		msg_print(Ind, "You have no right to choose that!");
		//msg_print(NULL);
		return FALSE;
	}

	/* If player has loan and the time is out, few things work in stores */
#if 0
	if (p_ptr->loan && !p_ptr->loan_time) {
		if ((bact != BACT_SELL) && (bact != BACT_VIEW_BOUNTIES) &&
		    (bact != BACT_SELL_CORPSES) &&
		    (bact != BACT_VIEW_QUEST_MON) &&
		    (bact != BACT_SELL_QUEST_MON) &&
		    (bact != BACT_EXAMINE) && (bact != BACT_STEAL) &&
		    (bact != BACT_PAY_BACK_LOAN))
		{
			msg_print(Ind, "You are not allowed to do that until you have paid back your loan.");
			msg_print(Ind, NULL);
			return FALSE;
		}
	}
#endif	// 0
	/* Similar penalty for those on black-list */
	if (p_ptr->tim_blacklist && st_ptr->st_idx != STORE_CASINO) { /* allow them to still lose their money =P ..and play Go! */
		if ((bact != BACT_SELL) && (bact != BACT_VIEW_BOUNTIES) &&
		    (bact != BACT_SELL_CORPSES) &&
		    (bact != BACT_VIEW_QUEST_MON) &&
		    (bact != BACT_SELL_QUEST_MON) &&
		    (bact != BACT_EXAMINE) && (bact != BACT_STEAL) &&
		    (bact != BACT_PAY_BACK_LOAN) && (bact != BACT_BUY))
		{
			msg_print(Ind, "The owner refuses it.");
			return FALSE;
		}
	}

	/* check gold */
	if (bcost > p_ptr->au) {
		msg_print(Ind, "You do not have the gold!");
//		msg_print(NULL);
		return FALSE;
	}

	if (!bcost) set_reward = TRUE;

	switch (bact) {
		case BACT_RESEARCH_ITEM:
			/* since paid is always TRUE for identify_fully_item(),
			   we can fix the 'Au refresh overwrites *id* info'
			   visual glitch here, by sending gold first. */
			p_ptr->au -= bcost;
			//store_prt_gold(Ind);
			Send_gold(Ind, p_ptr->au, p_ptr->balance);

//			paid = research_item(Ind, item);
			paid = identify_fully_item(Ind, item);

			return FALSE;//..and to complete the Send_gold() hack, return here
			//break;
#if 0
		case BACT_TOWN_HISTORY:
			town_history();
			break;
#endif
		case BACT_RACE_LEGENDS:
			race_legends(Ind);
			break;
#if 1
		case BACT_GREET_KING:
//			castle_greet();
			break;
		case BACT_QUEST1:
		case BACT_QUEST2:
		case BACT_QUEST3:
		case BACT_QUEST4:
		{
#if 0
			int y = 1, x = 1;
			bool ok = FALSE;

			while ((x < cur_wid - 1) && !ok) {
				y = 1;
				while ((y < cur_hgt - 1) && !ok) {
					/* Found the location of the quest info ? */
					if (bact - BACT_QUEST1 + FEAT_QUEST1 == cave[y][x].feat) {
						/* Stop the loop */
						ok = TRUE;
					}
					y++;
				}
				x++;
			}

			if (ok) recreate = castle_quest(y - 1, x - 1);;
			else msg_format(Ind, "ERROR: no quest info feature found: %d", bact - BACT_QUEST1 + FEAT_QUEST1);
#else
			/* get an extermination order */
			u16b flags = QUEST_MONSTER | QUEST_RANDOM | QUEST_RACE;
			int lev = p_ptr->lev;
			u16b type, num;
			if (prepare_xorder(Ind, Ind, flags, &lev, &type, &num))
				add_xorder(Ind, Ind, type, num, flags);
#endif
			break;
		}
#endif
#if 0
		case BACT_KING_LEGENDS:
		case BACT_ARENA_LEGENDS:
		case BACT_LEGENDS:
			show_highclass(building_loc);
			break;
		case BACT_POSTER:
		case BACT_ARENA_RULES:
		case BACT_ARENA:
			arena_comm(bact);
			break;
#endif	// 0

		case BACT_IN_BETWEEN:
		case BACT_CRAPS:
		//case BACT_SPIN_WHEEL:
		//case BACT_DICE_SLOTS:
		case BACT_GAMBLE_RULES:
			gamble_comm(Ind, bact, gold);
			break;
		case BACT_REST:
		case BACT_RUMORS:
		case BACT_FOOD:
			paid = inn_comm(Ind, bact);
			break;
#if 0
		case BACT_RESEARCH_MONSTER:
			paid = research_mon();
			break;
		case BACT_COMPARE_WEAPONS:
			paid = compare_weapons();
			break;
#if 0
		case BACT_GREET:
			greet_char();
			break;
#endif
#endif	// 0

		case BACT_ENCHANT_WEAPON:
			paid = fix_item(Ind, INVEN_WIELD, INVEN_ARM, 0, FALSE,
					BACT_ENCHANT_WEAPON, set_reward);
			break;
		case BACT_ENCHANT_ARMOR:
			paid = fix_item(Ind, INVEN_ARM, INVEN_FEET, 0, TRUE,
					BACT_ENCHANT_ARMOR, set_reward);
			break;
		/* needs work */
		case BACT_RECHARGE:
		{
			object_type		*o_ptr;

			/* Only accept legal items */
			item_tester_hook = item_tester_hook_recharge;

			/* Get the item (in the pack) */
			o_ptr = &p_ptr->inventory[item];

			if (!item_tester_hook(o_ptr)) {
				msg_print(Ind, "You cannot recharge that item.");
				//get_item(Ind);
			} else {
				//if (recharge(80)) paid = TRUE;
				recharge_aux(Ind, item, 80);
				paid = TRUE;
			}
			break;
		}
		/* needs work */
		case BACT_IDENTS:
			identify_pack(Ind);
			msg_print(Ind, "Your posessions have been identified.");
			//msg_print(Ind, NULL);
			paid = TRUE;
			break;
#if 0
		case BACT_LEARN:
			do_cmd_study();
			break;
#endif	// 0

		/* needs work */
		case BACT_STAR_HEAL:
			hp_player(Ind, 200);
			set_poisoned(Ind, 0, 0);
			set_blind(Ind, 0);
			set_confused(Ind, 0);
			set_cut(Ind, 0, 0);
			set_stun(Ind, 0);
			if (p_ptr->black_breath) {
				msg_print(Ind, "The hold of the Black Breath on you is broken!");
				p_ptr->black_breath = FALSE;
			}
			paid = TRUE;
			break;
		/* needs work */
		case BACT_HEALING:
			hp_player(Ind, 200);
			set_poisoned(Ind, 0, 0);
			set_blind(Ind, 0);
			set_confused(Ind, 0);
			set_cut(Ind, 0, 0);
			set_stun(Ind, 0);
			paid = TRUE;
			break;
		/* needs work */
		case BACT_RESTORE:
			if (do_res_stat(Ind, A_STR)) paid = TRUE;
			if (do_res_stat(Ind, A_INT)) paid = TRUE;
			if (do_res_stat(Ind, A_WIS)) paid = TRUE;
			if (do_res_stat(Ind, A_DEX)) paid = TRUE;
			if (do_res_stat(Ind, A_CON)) paid = TRUE;
			if (do_res_stat(Ind, A_CHR)) paid = TRUE;
			break;
		/* set timed reward flag */
		case BACT_GOLD:
#if 0
			if (!p_ptr->rewards[BACT_GOLD]) {
				share_gold();
				p_ptr->rewards[BACT_GOLD] = TRUE;
			} else {
				msg_print(Ind, "You just had your daily allowance!");
				msg_print(Ind, NULL);
			}
#endif
			break;
		case BACT_ENCHANT_ARROWS:
			paid = fix_item(Ind, INVEN_AMMO, INVEN_AMMO, 0, FALSE, BACT_ENCHANT_ARROWS, set_reward);
			break;
		case BACT_ENCHANT_BOW:
			//paid = fix_item(Ind, INVEN_BOW, INVEN_BOW, TV_BOW, FALSE, BACT_ENCHANT_BOW, set_reward);
			//allow boomerangs too:
			paid = fix_item(Ind, INVEN_BOW, INVEN_BOW, 0, FALSE, BACT_ENCHANT_BOW, set_reward);
			break;
#if 0
		case BACT_RECALL:
 #ifdef ANTI_TELE_CHEEZE
			if (p_ptr->anti_tele) {
				msg_print(Ind, "\377oThere is some static discharge in the air around you, but nothing happens.");
				break;
			}
 #endif
			p_ptr->word_recall = 1;
			msg_print(Ind, "\377oThe air about you becomes charged...");
			paid = TRUE;
			break;
		case BACT_TELEPORT_LEVEL:
			break;//disabled
			if (reset_recall(FALSE)) {
				p_ptr->word_recall = 1;
				msg_print(Ind, "\377oThe air about you becomes charged...");
				paid = TRUE;
			}
			break;
		case BACT_BUYFIRESTONE:
			amt = get_quantity("How many firestones (2000 gold each)? ", 10);
			if (amt > 0) {
				bcost = amt * 2000;
				if(p_ptr->au >= bcost) {
					paid = TRUE;
					msg_print(Ind, "You have bought some firestones !");

					/* Hack -- Give the player Firestone! */
					q_ptr = &forge;
					object_prep(q_ptr, lookup_kind(TV_FIRESTONE, SV_FIRE_SMALL));
					q_ptr->number = amt;
					object_aware(q_ptr);
					object_known(q_ptr);
					drop_near(0, q_ptr, -1, py, px);
				}
				else msg_print(Ind, "You do not have the gold!");
			}
			break;
		case BACT_COMEBACKTIME:
			if (PRACE_FLAG(PR1_TP)) {
				if (do_res_stat(A_STR, TRUE)) paid = TRUE;
				if (do_res_stat(A_INT, TRUE)) paid = TRUE;
				if (do_res_stat(A_WIS, TRUE)) paid = TRUE;
				if (do_res_stat(A_DEX, TRUE)) paid = TRUE;
				if (do_res_stat(A_CON, TRUE)) paid = TRUE;
				if (do_res_stat(A_CHR, TRUE)) paid = TRUE;
				p_ptr->chp -= 1000;
				if (p_ptr->chp <= 0) p_ptr->chp = 1;
			} else {
				msg_print(Ind, "Hum .. you are NOT a DragonRider, "
						"you need a dragon to go between !");
			}
			break;
#endif	// 0

		case BACT_MIMIC_NORMAL:
			if (set_mimic(Ind, 0, 0)) paid = TRUE;	/* Undo temporary mimicry (It's Shadow mimicry)*/
			if (p_ptr->fruit_bat == 2) { /* Undo fruit bat form from chauve-souris potion */
				p_ptr->fruit_bat = 0;
				p_ptr->update |= (PU_BONUS | PU_HP);
				paid = TRUE;
			}
			if (p_ptr->body_monster) /* Undo normal mimicry */ {
				p_ptr->body_monster_prev = p_ptr->body_monster;
				p_ptr->body_monster = 0;
				p_ptr->body_changed = TRUE;
				paid = TRUE;

				note_spot(Ind, p_ptr->py, p_ptr->px);
				everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);

				/* Recalculate mana */
				p_ptr->update |= (PU_MANA | PU_HP | PU_BONUS | PU_VIEW);

				/* Tell the client */
				p_ptr->redraw |= PR_VARIOUS;

				/* Window stuff */
				p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
			}
			if (paid) msg_print(Ind, "You are polymorphed back to normal form!");
			break;
#if 1
		case BACT_VIEW_BOUNTIES:
//			show_bounties();
			break;
		case BACT_VIEW_QUEST_MON:
//			show_quest_monster();
			break;
		case BACT_SELL_QUEST_MON:
//			sell_quest_monster();
			break;
		case BACT_SELL_CORPSES:
//			sell_corpses();
			break;
		/* XXX no fates, for now */
		case BACT_DIVINATION:
#if 0
			int i, count = 0;
			bool something = FALSE;

			while(count < 1000) {
				count++;
				i = rand_int(MAX_FATES);
				if(!fates[i].fate) continue;
				if(fates[i].know) continue;
				msg_print(Ind, "You know a little more of your fate.");

				fates[i].know = TRUE;
				something = TRUE;
				break;
			}

			if(!something) msg_print(Ind, "Well, you have no fate, anyway I'll keep your money!");
#else /* fortune cookies for now, lol */
			fortune(Ind, FALSE);
#endif
			paid = TRUE;
			break;
#endif	// 1

		case BACT_BUY:
			store_purchase(Ind, item, amt);
			break;
		case BACT_SELL:
			store_sell(Ind, item, amt);
			break;
		case BACT_EXAMINE:
			store_examine(Ind, item);
			break;
		case BACT_STEAL:
			store_stole(Ind, item);
			break;
#if 0
		/* XXX we'd simply better not to backport it */
		case BACT_REQUEST_ITEM:
			store_request_item();
			paid = TRUE;
			break;
		/* XXX This will be quite abusable.. */
		case BACT_GET_LOAN:
			s64b i, price, req;

			if(p_ptr->loan) {
				msg_print(Ind, "You already have a loan!");
				break;
			}
			for (i = price = 0; i < INVEN_TOTAL; i++)
				price += object_value_real(&inventory[i]);

			/* hack: prevent s32b overflow */
			if (2000000000 - p_ptr->au < price) {
				price = 2000000000;
			} else
				price += p_ptr->au;

			if (price > p_ptr->loan - 30000) price = p_ptr->loan - 30000;

			msg_format(Ind, "You have a loan of %i.", p_ptr->loan);

			req = get_quantity("How much would you like to get? ", price);
			if (req > 100000) req = 100000;

			if (2000000000 - req < p_ptr->loan) {
				msg_format(Ind, "\377yYou cannot have a greater loan than 2 billion worth of gold!");
				break;
			}

			if (!gain_au(Ind, req, FALSE, FALSE)) break;

			p_ptr->loan += req;
			//? if (p_ptr->au > PY_MAX_GOLD) p_ptr->au = PY_MAX_GOLD;
			p_ptr->loan_time += req;

			msg_format(Ind, "You receive %i gold pieces", req);

			paid = TRUE;
			break;
		case BACT_PAY_BACK_LOAN:
			s32b req;

			msg_format(Ind, "You have a loan of %i.", p_ptr->loan);

			req = get_quantity("How much would you like to pay back?", p_ptr->loan);

			if (req > p_ptr->au) req = p_ptr->au;
			if (req > p_ptr->loan) req = p_ptr->loan;

			p_ptr->loan -= req;
			p_ptr->au -= req;

			if (p_ptr->loan_time)
				p_ptr->loan_time = MAX(p_ptr->loan/2, p_ptr->loan_time);

			if (!p_ptr->loan) p_ptr->loan_time = 0;

			msg_format(Ind, "You pay back %i gold pieces", req);

			paid = TRUE;
			break;
#endif	// 0

		case BACT_DEPOSIT:
			if (gold > p_ptr->au) gold = p_ptr->au;
			if (gold < 1) break;

			/* hack: prevent s32b overflow */
			if (2000000000 - gold < p_ptr->balance) {
				msg_format(Ind, "\377yYou cannot deposit more than 2 billion worth of gold!");
				break;
			}

			p_ptr->balance += gold;
			p_ptr->au -= gold;
#ifdef USE_SOUND_2010
			sound(Ind, "drop_gold", NULL, SFX_TYPE_COMMAND, FALSE);
#endif

			msg_format(Ind, "You deposit %i gold pieces.", gold);
			s_printf("Deposit: %s - %d Au.\n", p_ptr->name, gold);

			paid = TRUE;
			break;
		case BACT_WITHDRAW:
			if (gold > p_ptr->balance) gold = p_ptr->balance;
			if (gold < 1) break;

			/* hack: prevent s32b overflow */
			if (2000000000 - gold < p_ptr->au) {
				msg_format(Ind, "\377yYou cannot carry more than 2 billion worth of gold!");
				break;
			}

			p_ptr->balance -= gold;
			p_ptr->au += gold;
#ifdef USE_SOUND_2010
			sound(Ind, "pickup_gold", NULL, SFX_TYPE_COMMAND, FALSE);
#endif

			msg_format(Ind, "You withdraw %i gold pieces.", gold);
			s_printf("Withdraw: %s - %d Au.\n", p_ptr->name, gold);

			paid = TRUE;
			break;
		case BACT_EXTEND_HOUSE:
			//paid = home_extend(Ind);
			home_extend(Ind);
			break;
		case BACT_CHEEZE_LIST:
			view_cheeze_list(Ind);
			break;
		case BACT_DEED_ITEM:
			reward_deed_item(Ind, item);
			break;
		case BACT_DEED_BLESSING:
			reward_deed_blessing(Ind, item);
			break;
		case BACT_GO:
#ifdef ENABLE_GO_GAME
			if (is_newer_than(&p_ptr->version, 4, 4, 6, 1, 0, 0))
				go_challenge(Ind);
			else
				msg_print(Ind, "\377oThis feature requires at least client 4.4.6b");
#endif
			break;
		case BACT_INSTANT_RES:
#ifdef ENABLE_INSTANT_RES
			if (p_ptr->mode & MODE_EVERLASTING) {
				if (p_ptr->insta_res) {
					p_ptr->insta_res = FALSE;
					msg_print(Ind, "\377rYou decide that you do not require the Instant Resurrection service!");
				} else {
					p_ptr->insta_res = TRUE;
					msg_print(Ind, "\377GYou engage the Instant Resurrection service.");
				}
			} else {
				msg_print(Ind, "\377oInstant Resurrection is only available to everlasting characters.");
			}
#else
			msg_print(Ind, "\377oInstant Resurrection has not been enabled on this server.");
#endif
			break;
		case BACT_EXPLORATIONS:
			view_exploration_records(Ind);
			break;
#ifdef GLOBAL_DUNGEON_KNOWLEDGE
		case BACT_DUNGEONS:
			view_exploration_history(Ind);
			break;
#endif
		case BACT_RENAME_GUILD:
			if (is_older_than(&p_ptr->version, 4, 4, 6, 2, 0, 0)) {
				msg_print(Ind, "You need an up-to-date client to rename a guild.");
				break;
			}
			Send_request_cfr(Ind, RID_GUILD_RENAME, format("Renaming your guild costs %d Au. Are you sure?", GUILD_PRICE), 2);
			break;
		case BACT_STATIC:
#if 1
if (is_admin(p_ptr))
			{
				int x, y, i, j, k;
				struct dungeon_type *d_ptr;
				worldpos tpos;
				bool found = FALSE, first;
				char dun_name[MAX_CHARS];

				struct dun_level *l_ptr;
				time_t now = time(&now);
				int grace;

				FILE *fff;
				char buf[1024];

				/* Open a new file */
				fff = my_fopen(p_ptr->infofile, "wb");
				/* Current file viewing */
				strcpy(p_ptr->cur_file, p_ptr->infofile);
				/* Let the player scroll through this info */
				p_ptr->special_file_type = TRUE;

				for (x = 0; x < 64; x++) for (y = 0; y < 64; y++) {
					/* check tower */
					if ((d_ptr = wild_info[y][x].tower)) {
						/* skip non-canonical dungeons */
						if (!d_ptr->type) continue;
						tpos.wx = x; tpos.wy = y;
						tpos.wz = 1;
						/* skip IDDC (included in check above) */
						//if (in_irondeepdive(&tpos)) continue;
						strcpy(dun_name, get_dun_name(x, y, TRUE, d_ptr, d_ptr->type, TRUE));
						first = TRUE;

						for (i = 0; i < d_ptr->maxdepth; i++) {
							k = 0;
							tpos.wz = i + 1;

							for (j = 1; j <= NumPlayers; j++) if (inarea(&Players[j]->wpos, &tpos)) k++;
							if (d_ptr->level[i].ondepth <= k) continue;

							/* add an inter-dungeon spacer */
							if (first && found) fprintf(fff, "\n");

							l_ptr = &d_ptr->level[ABS(tpos.wz) - 1];
							if (!l_ptr) {
								/* paranoia */
								fprintf(fff, "Something is wrong with %2d,%2d,%2d\n", x, y, i + 1);
								continue;
							}
#ifdef SAURON_FLOOR_FAST_UNSTAT
							if (d_ptr->type == DI_MT_DOOM && i + 1 == d_ptr->maxdepth) grace = 60 * 60; //1h
							else
#endif
							grace = cfg.level_unstatic_chance * getlevel(&tpos) * 60;

							fprintf(fff, "\377s          %28s %5dft for %d more minutes\n", dun_name, (i + 1) * 50, (int)((grace - (now - l_ptr->lastused)) / 60));
							/* display spacer instead of repeating the dungeon name subsequently */
							if (first) for (j = 0; j < strlen(dun_name); j++) dun_name[j] = ' ';
							found = TRUE;
							first = FALSE;
						}
					}
					/* check dungeon */
					if ((d_ptr = wild_info[y][x].dungeon)) {
						/* skip non-canonical dungeons */
						if (!d_ptr->type) continue;
						tpos.wx = x; tpos.wy = y;
						tpos.wz = -1;
						/* skip IDDC (included in check above) */
						//if (in_irondeepdive(&tpos)) continue;
						strcpy(dun_name, get_dun_name(x, y, FALSE, d_ptr, d_ptr->type, TRUE));
						first = TRUE;

						for (i = 0; i < d_ptr->maxdepth; i++) {
							k = 0;
							tpos.wz = -(i + 1);

							for (j = 1; j <= NumPlayers; j++) if (inarea(&Players[j]->wpos, &tpos)) k++;
							if (d_ptr->level[i].ondepth <= k) continue;

							/* add an inter-dungeon spacer */
							if (first && found) fprintf(fff, "\n");

							l_ptr = &d_ptr->level[ABS(tpos.wz) - 1];
							if (!l_ptr) {
								/* paranoia */
								fprintf(fff, "Something is wrong with %2d,%2d,%2d\n", x, y, -(i + 1));
								continue;
							}

#ifdef SAURON_FLOOR_FAST_UNSTAT
							if (d_ptr->type == DI_MT_DOOM && i + 1 == d_ptr->maxdepth) grace = 60 * 60; //1h
							else
#endif
							grace = cfg.level_unstatic_chance * getlevel(&tpos) * 60;

							fprintf(fff, "\377s          %28s %5dft for %d more minutes\n", dun_name, -(i + 1) * 50, (int)((grace - (now - l_ptr->lastused)) / 60));
							/* display spacer instead of repeating the dungeon name subsequently */
							if (first) for (j = 0; j < strlen(dun_name); j++) dun_name[j] = ' ';
							found = TRUE;
							first = FALSE;
						}
					}
				}

				if (!found) {
					fprintf(fff, "No static floors recorded.\n");
				}

				/* Close the file */
				my_fclose(fff);
				/* Hack -- anything written? (rewrite me) */
				fff = my_fopen(p_ptr->infofile, "rb");
				if (my_fgets(fff, buf, 1024, FALSE)) {
					my_fclose(fff);
					return (FALSE);
				}
				my_fclose(fff);
				/* Let the client know it's about to get some info */
				strcpy(p_ptr->cur_file_title, "Static floor records");
				Send_special_other(Ind);

				break;
			}
#endif
#ifdef SOLO_REKING
		case BACT_SR_DONATE:
			if (is_older_than(&p_ptr->version, 4, 4, 6, 2, 0, 0)) {
				msg_print(Ind, "You need an up-to-date client to donate.");
				break;
			}
			Send_request_num(Ind, RID_SR_DONATE, "Donate how much gold to the temple? ", p_ptr->au);
			break;
#endif
#ifdef ENABLE_ITEM_ORDER
		case BACT_ITEM_ORDER:
			if (is_older_than(&p_ptr->version, 4, 4, 6, 2, 0, 0)) {
				msg_print(Ind, "You need an up-to-date client to order an item.");
				break;
			}
			Send_request_str(Ind, RID_ITEM_ORDER, "Which item would you like to order? ", "");
			break;
#endif
#ifdef ENABLE_MERCHANT_MAIL
		case BACT_SEND_ITEM:
		case BACT_SEND_ITEM_PAY: {
			int i;
			object_type *o_ptr;
			/* Get the item (in the pack) */
			o_ptr = &p_ptr->inventory[item];
			u32b fee;

			if (!o_ptr->k_idx) {
				msg_print(Ind, "Invalid item.");
				break;
			}

			if (is_older_than(&p_ptr->version, 4, 4, 6, 2, 0, 0)) {
				msg_print(Ind, "\377oYou need an up-to-date client to send an item.");
				break;
			}
			if (!(o_ptr->ident & ID_KNOWN)) {
				msg_print(Ind, "\377yItems must be identified before you can send them.");
				break;
			}

			for (i = 0; i < MAX_MERCHANT_MAILS; i++) {
				if (!mail_sender[i][0]) break;
				if (!strcmp(mail_sender[i], p_ptr->name)) {
					msg_print(Ind, "\377yYou can only have one active shipment at a time.");
					return FALSE;
				}
			}
			if (i == MAX_MERCHANT_MAILS) {
				msg_print(Ind, "\377yWe're very sorry, our service is currently overloaded! Please try again later.");
				break;
			}

			fee = object_value_real(0, o_ptr) / 20;
			if (fee < 5) fee = 5;

			p_ptr->mail_item = item;
			p_ptr->mail_fee = fee;
			p_ptr->mail_xfee = 0;
			Send_request_cfr(Ind, bact == BACT_SEND_ITEM ? RID_SEND_ITEM : RID_SEND_ITEM_PAY,
			    format("The fee for sending this item is %d Au, accept?", fee), 2);
			break; }
		case BACT_SEND_GOLD: {
			int i;
			u32b fee;

			if (is_older_than(&p_ptr->version, 4, 4, 6, 2, 0, 0)) {
				msg_print(Ind, "\377oYou need an up-to-date client to send gold.");
				break;
			}

			for (i = 0; i < MAX_MERCHANT_MAILS; i++) {
				if (!mail_sender[i][0]) break;
				if (!strcmp(mail_sender[i], p_ptr->name)) {
					msg_print(Ind, "\377yYou can only have one active shipment at a time.");
					return FALSE;
				}
			}
			if (i == MAX_MERCHANT_MAILS) {
				msg_print(Ind, "\377yWe're very sorry, our service is currently overloaded! Please try again later.");
				break;
			}

			if (gold < 1) break;
			if (gold > p_ptr->au) gold = p_ptr->au;
			fee = gold / 20;
			if (fee < 5) fee = 5;

			p_ptr->mail_gold = gold;
			p_ptr->mail_fee = fee;
			p_ptr->mail_xfee = 0;
			Send_request_cfr(Ind, RID_SEND_GOLD, format("The fee for sending %d Au is %d Au, accept?", gold, fee), 2);
			break; }
#endif
		default:
#if 0
			if (process_hooks_ret(HOOK_BUILDING_ACTION, "d", "(d)", bact)) {
				paid = process_hooks_return[0].num;
			}
#endif	// 0
			break;
	}

	if (paid) {
		p_ptr->au -= bcost;

		/* Display the current gold */
//		store_prt_gold(Ind);
		Send_gold(Ind, p_ptr->au, p_ptr->balance);
	}

	return (recreate);
}


#if 0
/*
 * Enter quest level
 */
void enter_quest(void)
{
	if (!(cave[py][px].feat == FEAT_QUEST_ENTER))
	{
		msg_print("You see no quest level here.");
		return;
	}
	else
	{
		/* Player enters a new quest */
		p_ptr->oldpy = py;
		p_ptr->oldpx = px;

		leaving_quest = p_ptr->inside_quest;

		p_ptr->inside_quest = cave[py][px].special;
		dun_level = 1;
		p_ptr->leaving = TRUE;
		p_ptr->oldpx = px;
		p_ptr->oldpy = py;
	}
}


/*
 * Do building commands
 */
void do_cmd_bldg(void)
{
	int i,which, x = px, y = py;
	char command;
	bool validcmd;
	store_type *s_ptr;
	store_info_type *st_ptr;
	store_action_type *ba_ptr;


	if (cave[py][px].feat != FEAT_SHOP)
	{
		msg_print("You see no building here.");
		return;
	}

	which = cave[py][px].special;
	building_loc = which;

	s_ptr = &town_info[p_ptr->town_num].store[which];
	st_ptr = &st_info[which];

	p_ptr->oldpy = py;
	p_ptr->oldpx = px;

	/* Forget the lite */
	/* forget_lite(); */

	/* Forget the view */
	forget_view();

	/* Hack -- Increase "icky" depth */
	character_icky++;

	command_arg = 0;
	command_rep = 0;
	command_new = 0;

	show_building(s_ptr);
	leave_bldg = FALSE;

	while (!leave_bldg)
	{
		validcmd = FALSE;
		prt("",1,0);
		command = inkey();

		if (command == ESCAPE)
		{
			leave_bldg = TRUE;
			p_ptr->inside_arena = FALSE;
			break;
		}

		for (i = 0; i < 6; i++)
		{
			ba_ptr = &ba_info[st_info->actions[i]];

			if (ba_ptr->letter)
			{
				if (ba_ptr->letter == command)
				{
					validcmd = TRUE;
					break;
				}
			}
		}

		if (validcmd)
			bldg_process_command(s_ptr, i);

		/* Notice stuff */
		notice_stuff();

		/* Handle stuff */
		handle_stuff();
	}

	/* Flush messages XXX XXX XXX */
	msg_print(NULL);

	/* Reinit wilderness to activate quests ... */
	wilderness_gen(TRUE);
	py = y;
	px = x;

	/* Hack -- Decrease "icky" depth */
	character_icky--;

	/* Clear the screen */
	Term_clear();

	/* Update the visuals */
	p_ptr->update |= (PU_VIEW | PU_MON_LITE | PU_MONSTERS | PU_BONUS);

	/* Redraw entire screen */
	p_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);
}
#endif	// 0
