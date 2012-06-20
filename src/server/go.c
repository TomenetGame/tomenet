/* File: go.c */
/* Purpose: Allow players to play a game of Go, by C. Blue */

/* Details:
 * Note that Go is called Igo in Japan, Weiqi in China and Baduk in Korea.
 * This code expects to find the Go engine executable in a subfolder 'go' and
 * also saves games into it. All engines that use the Go Text Protocol (GTP)
 * should be easily compatible.
 *
 * At the time of creation I decided on Fuego for the engine, which is free,
 * open source, and placed #2 in 9x9 tournament at Kanazawa 2010 Computer Go
 * Olympiad (the #1 program wasn't free). Might switch to GnuGo though.
 *
 * Another option might be GnuGo. Although somewhat weaker (might change, there
 * is also a Monte-Carlo version out now) it allows scaling via a '--level'
 * parameter.
 *
 * Other very strong bots that aren't open source are Zen, MoGo, Erica,
 * Valkyrie, CrazyStone and Aya. Unfortunately bot tournament results say quite
 * little about a bot's strength, because the hardware that must be use is not
 * standardized! So one bot might win while running on a large cluster, while
 * another bot comes in 2nd on a simple Quadcore..
 *                                                     - C. Blue -
 */


/* ------------------------------------------------------------------------- */

#define SERVER

#include "angband.h"

#ifdef ENABLE_GO_GAME

#include <sys/wait.h> /* for waitpid() */

/* Pick one of the engines to use: */
/* Use 'gnugno' engine? (recommended) */
#define ENGINE_GNUGO
/* Use 'fuego' engine? */
//#define ENGINE_FUEGO

/* Anti Mirror-Go engine?
   The normal engine will be swapped silently with this one
   if mirror Go is detected after a specific number of turns: */
#define ANTI_MIRROR	"pachi"
/* If above is defined: Invoke countermeasures at which # of mirrorred moves? (Full moves.) [6] */
#define ANTI_MIRROR_THRESHOLD	6

/* # of buffer lines for GTP responses: 9 lines,2 coordinate lines, +1, +1 for msg hack.
   Could probably be reduced now that we have board_line..[] helper vars. */
#define MAX_GTP_LINES (9+2+1+1)

/* Screen absolute x,y coordinates of the Go board */
#define GO_BOARD_X	34
#define GO_BOARD_Y	8

/* Next actions to perform after async reading finishes */
#define NACT_NONE		0 /* nothing */
#define NACT_MOVE_PLAYER	1 /* request next move from player */
#define NACT_MOVE_CPU		2 /* request next move from AI */

/* Maximum rank: At this rank you'll have to play a game as white too! */
#define TOP_RANK	7

/* Time limits for the player (and CPU) in seconds. [30]
   Should be *ABOVE 20*, since some AI players will require 20s per move */
#define GO_TIME_PY		30

/* Only play random moves instead of engine-generated ones up to this move number
   in the game. (Half moves.) [35] */
#define RND_MOVE_THRESHOLD	35


/* Error handling constants */
static const int NONE = 0, DOWN = 1, UP = 2;

/* Discard all responses we could've read from the engine
   in case we're just in the process of terminating it anyway.
   This avoids slight slowdowns, useful if we're doing a warm
   restart of the Go engine while keeping the TomeNET server running. */
#define DISCARD_RESPONSES_WHEN_TERMINATING

/* Display text output for debugging? */
//#define GO_DEBUGPRINT
/* Display log entries for debugging? */
#define GO_DEBUGLOG


/* Global control variables */
bool go_engine_up;		/* Go engine is online? */
int go_engine_processing = 0;	/* Go engine is expected to deliver replies to commands? */
bool go_game_up;		/* A game of Go is actually being played right now? */
u32b go_engine_player_id;	/* Player ID of the player who plays a game of Go right now. */
static char avatar_name[40], tmp[20];

/* Private control variable to determine what to do next
   after go_engine_processing reaches 0 again: */
static int go_engine_next_action = NACT_NONE;

/* Private stuff */
static void writeToPipe(char *data);
static void readFromPipe(char *buf, int *cont);
static int test_for_response(void); /* non-blocking read */
static int wait_for_response(void); /* blocking read */
#ifdef ENGINE_FUEGO
static int handle_loading(void);
#endif
static void go_engine_move_CPU(void);
static void go_engine_move_result(int move_result);
static void go_challenge_cleanup(bool server_shutdown);
static void go_engine_board(void);
static bool go_err(int engine_status, int game_status, char *name);
static void enable_anti_mirror(void);

/* Handle game initialization and colour assignment */
static bool CPU_has_white, game_over = TRUE, scoring = FALSE;
/* Manage player's time limit */
static int player_timelimit_sec, player_timeleft_sec;

/* Pipe-related vars */
static pid_t nPid;
static int pipeto[2];		/* pipe to feed the exec'ed program input */
static int pipefrom[2];		/* pipe to get the exec'ed program output */
static FILE *fw, *fr;
static char pipe_buf[MAX_GTP_LINES][240];
static int pipe_buf_current_line,  pipe_response_complete = 0;
#ifdef DISCARD_RESPONSES_WHEN_TERMINATING
static bool terminating = FALSE;
#endif
static bool received_board_visuals = FALSE;
static bool waiting_for_board_update = FALSE;

/* Keep track of game starting time and duration */
static time_t tstart, tend;
static struct tm* tmstart;
static struct tm* tmend;
/* Game record: Keep track of moves and passes (for undo-handling too) */
static int pass_count;
static bool last_move_was_pass = FALSE, CPU_to_move, CPU_now_to_move;
static FILE *sgf;
static char last_black_move[3], last_white_move[3], game_result[10];

/* Helper vars to update the board visuals between turns */
static char board_line_old[10][10], board_line[10][10];//set by receiving 'showboard' response
static int random_move_prob, move_count;

#ifdef ANTI_MIRROR
/* To detect Mirror-Go and replace AI by special anti-Mirror-Go engine silently */
static char last_cpu_move[3];
static int mirror_count;
static bool anti_mirror_active;
#endif

/* Gameplay configuration */
static int wager_lvl[9] = {
      5000,   7500,
     10000,  15000,
     20000,  30000,
     50000, 100000,
    0,
};


/* ------------------------------------------------------------------------- */


/* Start engine, create & connect pipes, on server startup usually */
int go_engine_init(void) {
	int n;
	char *gocmd = NULL;

	s_printf("GO_INIT: ---INIT---\n");

#ifdef ENGINE_FUEGO
	gocmd = "./go/fuego";
#endif
#ifdef ENGINE_GNUGO
	gocmd = "./go/gnugo";
#endif

	if (!gocmd) {
		s_printf("GO_ERROR: No engine defined during compile.\n");
		return 7;
	}

	if (access(gocmd, X_OK) != 0) {
		s_printf("GO_ERROR: Engine executable not found.\n");
		return 8;
	}

	/* Try to create pipes for STDIN and STDOUT */
	if (pipe(pipeto) != 0) {
		s_printf("GO_ERROR: pipe() to.\n");
		return 1;
	}
	if (pipe(pipefrom) != 0) {
		s_printf("GO_ERROR: pipe() from.\n");
		return 2;
	}

	/* Try to create Go AI engine process */
	nPid = fork();
	if (nPid < 0) {
		s_printf("GO_ERROR: fork().\n");
		return 3;
	} else if (nPid == 0) {
//		close(STDIN_FILENO); close(STDOUT_FILENO); close(STDERR_FILENO);
		/* dup pipe read/write to stdin/stdout */
		dup2(pipeto[0], STDIN_FILENO);
		dup2(pipefrom[1], STDOUT_FILENO);
//		dup2(pipefrom[1], STDERR_FILENO); leave this, to print error message further below.
		/* close unnecessary pipe descriptors for a clean environment */
		close(pipeto[1]);
		close(pipefrom[0]);

		/* Start a new session to prevent Ctrl-C in the terminal from killing the child */
		setsid();

/*		"Options:\n"
		"  -config file execute GTP commands from file before\n"
	        "               starting main command loop\n"
	        "  -help        display this help and exit\n"
	        "  -maxgames n  make clear_board fail after n invocations\n"
	        "  -nobook      don't automatically load opening book\n"
	        "  -nohandicap  don't support handicap commands\n"
	        "  -quiet       don't print debug messages\n"
	        "  -size        initial (and fixed) board size\n"
	        "  -srand       set random seed (-1:none, 0:time(0))\n";*/

#ifdef ENGINE_FUEGO
		execlp("./go/fuego", "fuego", NULL);
#endif
#ifdef ENGINE_GNUGO
		execlp("./go/gnugo", "gnugo", "--mode", "gtp", \
		    "--quiet", "--boardsize", "9x9", "--chinese-rules", \
		    "--capture-all-dead", \
		    "--monte-carlo", "--komi", "0", NULL);
		// --cache-size <megs>
#endif

		/* We were unable to execute the engine? Game over! */
		fprintf(stderr,"GO_ERROR: exec().\n");
//		return 4;

		/* Cause SIGPIPE to parent so they know what's up */
		close(pipeto[0]);
		close(pipefrom[1]);

		/* Terminate child process */
		exit(4);
	}

	/* We're parent */

	/* Clear the pipe_buf[] array */
	pipe_buf_current_line = 0;
	for (n = 0; n < MAX_GTP_LINES; n++) strcpy(pipe_buf[n], "");

	/* Prepare pipes (close unused ends) */
	close(pipeto[0]);
	close(pipefrom[1]);

	if ((fw = fdopen(pipeto[1], "w")) == NULL) {
		s_printf("GO_ERROR: fdopen() w.\n");
		return 5;
	}
	if ((fr = fdopen(pipefrom[0], "r")) == NULL) {
		fclose(fw);
		s_printf("GO_ERROR: fdopen() r.\n");
		return 6;
	}

	/* Set stream for reading to non-blocking */
	fcntl(pipefrom[0], F_SETFL, O_NONBLOCK);

	/* Power up engine */
	s_printf("GO_INIT: ---STARTING UP---\n");
	/* We're up and running -
	   at least if the next initializing writeToPipe() won't shut us down
	   again due to broken pipe, in case the child couldn't init the engine. */
	go_engine_up = TRUE;

#ifdef ENGINE_FUEGO
	handle_loading();
#endif

	/* Prepare general game configuration */
	s_printf("GO_INIT: ---INIT AI---\n");

	/* Hack: Allow child to close pipes in case it cannot start the engine */
	sleep(5);

#ifdef ENGINE_FUEGO
	writeToPipe("boardsize 9");
	wait_for_response();
/*	writeToPipe("clear_board");
	wait_for_response();
*/	writeToPipe("komi 0");
	wait_for_response();
	writeToPipe("time_settings 0 1 0");//infinite: b>0, s=0
	wait_for_response();

	writeToPipe("uct_param_player reuse_subtree 1");
	wait_for_response();

	/* multi-core: */
	/* only required for > 2 threads? doesn't matter: */
	writeToPipe("uct_param_search lock_free 1");
	wait_for_response();

	writeToPipe("uct_param_search number_threads 2");//we have a 2-core cpu
	wait_for_response();

	/* we have 512 MB.. */
	writeToPipe("uct_max_memory 300000000");
	wait_for_response();

	/* Hm, shouldn't this be default anyway? */
	writeToPipe("go_param_rules ko_rule pos_superko");
	wait_for_response();

#if 0 /* get more clue before using maybe^^ */
 #if 1
	/* more playouts = stronger? or obsolete?: */
	writeToPipe("uct_param_player max_games=32000");
	wait_for_response();
 #endif
 #if 0
	/* not already covered by max_games and max_memory? */
	writeToPipe("uct_param_search max_nodes=3750000");//seems max at 300MB RAM?
	wait_for_response();
 #endif
#endif

	/* slight improvements? (might be obsolete meanwhile): */
	writeToPipe("uct_param_search virtual_loss 1");
	wait_for_response();
	writeToPipe("uct_param_search number_playouts 2");
	wait_for_response();

	/* misc */
	//go_param debug_to_comment 1
	//go_param auto_save (filename)
	//go_sentinel_file (filename)  (for resuming games that were interrupted by disconnection)
	//final_status_list dead|seki|alive   returns list of stones; use to capture everything, to make a clear end?
#endif

#ifdef ENGINE_GNUGO
	writeToPipe("time_settings 0 0 0");//infinite: main = 0, byo = 0, stones = x
	wait_for_response();
#endif

	/* Init this 'flow control', to begin asynchronous pipe operation */
	go_engine_processing = 0;

	/* ready for games */

	return 0;
}

/* Terminate engine & kill pipes, on server shutdown usually */
void go_engine_terminate(void) {
	int status_dummy;

	if (!go_engine_up) {
		/* No zombie child */
		waitpid(nPid, &status_dummy, WNOHANG);

		/* Termination complete. */
		return;
	}

#ifdef DISCARD_RESPONSES_WHEN_TERMINATING
	terminating = TRUE;
#endif

	/* try to exit running games gracefully */
	if (go_game_up) go_challenge_cleanup(TRUE);

	/* terminate cleanly */
	writeToPipe("quit");

#if 0	/* disable, in case we're terminating the engine because it \
	 stopped working! (would freeze server here, in that case) */
	wait_for_response();
#endif

	/* discard any possible answers we were still waiting for */
	go_engine_processing = 0;

#ifdef DISCARD_RESPONSES_WHEN_TERMINATING
	terminating = FALSE;
#endif

	/* close pipes to it */
	if (go_engine_up) {
		fclose(fr);
		fclose(fw);
		close(pipeto[1]);
		close(pipefrom[0]);
		go_engine_up = FALSE;
	}

	/* No zombie child */
	waitpid(nPid, &status_dummy, WNOHANG);

	s_printf("GO_ENGINE: TERMINATED\n");
}

void go_challenge(int Ind) {
	player_type *p_ptr = Players[Ind];
	char challenge_req[20];

	Send_store_special_clr(Ind, 4, 18);

	/* Only one player at a time */
	if (go_game_up) {
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Arrr.. sorry, I can't find a fitting player around right now");
		Send_store_special_str(Ind, 7, 3, TERM_ORANGE, "..wasn't that guy here just a minute ago?!");
		Send_store_special_str(Ind, 8, 3, TERM_ORANGE, "Well sorry, why don't you come back again a bit later!");
		return;
	}
	if (!go_engine_up) {
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Sorry, I don't have a fitting player around right now, darn..");
		Send_store_special_str(Ind, 7, 5, TERM_ORANGE, "why don't you come back a bit later!");
		return;
	}

	/* We're now busy playing Go */
	p_ptr->store_action = BACT_GO;

	/* Owner talk about how you suck..or not!
	   And making a new proposal accordingly. */
	strcpy(challenge_req, format("Bet %d Au? ", wager_lvl[p_ptr->go_level / 2]));
	switch (p_ptr->go_level) {
	case 0:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "So you think you're any good at this huh?");
		Send_store_special_str(Ind, 7, 3, TERM_ORANGE, "Well then, let's see some money. If you win, you'll get it double!");
		Send_store_special_str(Ind, 9, 3, TERM_ORANGE, "If you lose, and I dun' expect much really, I'll just keep it until");
		Send_store_special_str(Ind, 10, 3, TERM_ORANGE, "you've become a bit stronger and can win it back.. as if, haha!");
		Send_store_special_str(Ind, 12, 3, TERM_ORANGE, "..oh and I'll let you take black even, what d'you say eh?");
		Send_request_cfr(Ind, RID_GO, challenge_req);
		return;
	case 1:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Got a grip on the basic ideas by now?");
		Send_store_special_str(Ind, 7, 3, TERM_ORANGE, "Alright let's see if you can win the money now!");
		break;
	case 2:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "You begin to understand how this stuff works");
		Send_store_special_str(Ind, 7, 3, TERM_ORANGE, "eh? At least that's what you think..");
		Send_store_special_str(Ind, 8, 3, TERM_ORANGE, "You ready to bet a lil more dough this time?");
		Send_request_cfr(Ind, RID_GO, challenge_req);
		return;
	case 3:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Have you become at least somewhat adept by now?");
		Send_store_special_str(Ind, 7, 3, TERM_ORANGE, "That small change of yours is just annoying to keep.");
		break;
	case 4:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "You again, and you look like you want to make more");
		Send_store_special_str(Ind, 7, 3, TERM_ORANGE, "bucks off of me, hah. Well, I'll take you up on that!");
		Send_request_cfr(Ind, RID_GO, challenge_req);
		return;
	case 5:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "You trained harder? I guess we aren't done yet..");
		break;
	case 6:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Ah it's you. I've got someone who's not a beginner");
		Send_store_special_str(Ind, 7, 3, TERM_ORANGE, "anymore in the house today. You up for it?");
		Send_request_cfr(Ind, RID_GO, challenge_req);
		return;
	case 7:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "You've come back for your money, eh?");
		Send_store_special_str(Ind, 7, 3, TERM_ORANGE, "Hope your skill is somewhat polished.");
		break;
	case 8:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "I know your skill isn't too shabby.");
		Send_store_special_str(Ind, 7, 3, TERM_ORANGE, "I've got someone rather skilled for you,");
		Send_store_special_str(Ind, 8, 3, TERM_ORANGE, "but it'll take a higher wager this time!");
		Send_request_cfr(Ind, RID_GO, challenge_req);
		return;
	case 9:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Yeah, can't always win right away.");
		Send_store_special_str(Ind, 7, 3, TERM_ORANGE, "Well, go ahead if you think you're up to it now!");
		break;
	case 10:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Aha, it's the person skilled in this game..");
		Send_store_special_str(Ind, 7, 3, TERM_ORANGE, "dare to try someone of quite serious skills?");
		Send_store_special_str(Ind, 8, 3, TERM_ORANGE, "Then let's see some serious money, friend!");
		Send_request_cfr(Ind, RID_GO, challenge_req);
		return;
	case 11:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "I didn't really expect you to win that one.");
		Send_store_special_str(Ind, 7, 3, TERM_ORANGE, "This guy is pretty scary huh..");
		Send_store_special_str(Ind, 8, 3, TERM_ORANGE, "Well good luck, I'll be surprised if you make it.");
		break;
	case 12:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Alright alright, you're sort of becoming");
		Send_store_special_str(Ind, 7, 3, TERM_ORANGE, "renowned for ya skills. So I did my best");
		Send_store_special_str(Ind, 8, 3, TERM_ORANGE, "and invited a real master to play today!");
		Send_store_special_str(Ind, 9, 3, TERM_ORANGE, "He's got his price though, you got enough?");
		Send_request_cfr(Ind, RID_GO, challenge_req);
		return;
	case 13:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "There's just no way you could beat that guy.");
		Send_store_special_str(Ind, 7, 3, TERM_ORANGE, "Well go ahead, he's lacking opponents so he");
		Send_store_special_str(Ind, 8, 3, TERM_ORANGE, "should be free if you want to try again.");
		Send_store_special_str(Ind, 9, 3, TERM_ORANGE, "After all you're a paying customer, hah.");
		break;
	case TOP_RANK * 2:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "I can't believe you beat that guy!");
		Send_store_special_str(Ind, 7, 3, TERM_ORANGE, "I haven't heard of anyone stronger around,");
		Send_store_special_str(Ind, 8, 3, TERM_ORANGE, "but, can you beat him if you..play white!");
		Send_request_cfr(Ind, RID_GO, challenge_req);
		return;
	case TOP_RANK * 2 + 1:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "There's a limit to everything, you had to");
		Send_store_special_str(Ind, 7, 3, TERM_ORANGE, "hit it sooner or later. Don't mind me though");
		Send_store_special_str(Ind, 8, 3, TERM_ORANGE, "if you want to try and win back your cash.");
		break;
	case TOP_RANK * 2 + 2:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "I'll see if the master is around for a game.");
		Send_store_special_str(Ind, 7, 3, TERM_ORANGE, "And you can even take white now..");
		Send_store_special_str(Ind, 8, 3, TERM_ORANGE, "quite impressive, game is on the house!");
		break;
	}

	Send_request_key(Ind, RID_GO, "- press any key to continue -");
}

void go_challenge_accept(int Ind, bool new_wager) {
	player_type *p_ptr = Players[Ind];
	int wager = 0;

	Send_store_special_clr(Ind, 4, 18);

	if (new_wager) {
		/* Prepare to deduct wager */
		wager = wager_lvl[p_ptr->go_level / 2];
		if (p_ptr->au < wager) {
			Send_store_special_str(Ind, 8, 5, TERM_RED, "You don't have the money!");

			/* We're no longer busy */
			p_ptr->store_action = 0;
			return;
		}
	}

	/* Go engine busy? */
	if (go_game_up) {
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Arrr.. sorry, I can't find a fitting player around right now");
		Send_store_special_str(Ind, 7, 3, TERM_ORANGE, "..wasn't that guy here just a minute ago?!");
		Send_store_special_str(Ind, 8, 3, TERM_ORANGE, "Well sorry, why don't you come back again a bit later!");

		/* We're no longer busy */
		p_ptr->store_action = 0;
		return;
	}
	/* We're in! */
	go_game_up = TRUE;
	go_engine_player_id = p_ptr->id;

	if (new_wager) {
		/* Deduct wager */
		p_ptr->au -= wager;
		p_ptr->redraw |= PR_GOLD;
		Send_gold(Ind, p_ptr->au, p_ptr->balance);
	}

	/* Introduce opponent */
	switch (p_ptr->go_level) {
	case 0:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Alright then, I've got just the man for you, wait here..");
		Send_store_special_str(Ind, 8, 10, TERM_ORANGE, "<you hear some shouting>");
		Send_store_special_str(Ind, 10, 3, TERM_ORANGE, "This is \377yDougan, The Quick\377o,");
		Send_store_special_str(Ind, 11, 3, TERM_ORANGE, "let's see if you're any good at all..I doubt it. You ready?");
		break;
	case 1:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Alright, wait here for a moment, I saw \377yDougan, The Quick\377o just now..");
		Send_store_special_str(Ind, 8, 10, TERM_ORANGE, "<you hear some shouting>");
		Send_store_special_str(Ind, 11, 3, TERM_ORANGE, "let's see if you got the slightest clue now.. You ready?");
		break;
	case 2:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Alright then, I've got just the man for you, wait here..");
		Send_store_special_str(Ind, 8, 10, TERM_ORANGE, "<you hear some shouting>");
		Send_store_special_str(Ind, 10, 3, TERM_ORANGE, "This is \377yRabbik, The Bold\377o,");
		Send_store_special_str(Ind, 11, 3, TERM_ORANGE, "let's see what you can do.. You ready?");
		break;
	case 3:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Alright, wait here for a moment, I saw \377yRabbik, The Bold\377o just now..");
		Send_store_special_str(Ind, 8, 10, TERM_ORANGE, "<you hear some shouting>");
		Send_store_special_str(Ind, 11, 3, TERM_ORANGE, "let's see what you can do.. You ready?");
		break;
	case 4:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Alright then, I know the right opponent for you, wait here..");
		Send_store_special_str(Ind, 8, 10, TERM_ORANGE, "<you hear some shouting>");
		Send_store_special_str(Ind, 10, 3, TERM_ORANGE, "This is \377yLima, The Witty\377o,");
		Send_store_special_str(Ind, 11, 3, TERM_ORANGE, "let's see what you can do.. You ready?");
		break;
	case 5:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Alright, wait here for a moment, I saw \377yLima, The Witty\377o just now..");
		Send_store_special_str(Ind, 8, 10, TERM_ORANGE, "<you hear some shouting>");
		Send_store_special_str(Ind, 11, 3, TERM_ORANGE, "let's see what you can do.. You ready?");
		break;
	case 6:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Alright then, wait here for a moment..");
		Send_store_special_str(Ind, 8, 10, TERM_ORANGE, "<you hear some shouting>");
		Send_store_special_str(Ind, 10, 3, TERM_ORANGE, "This is \377yZar'am, The Stoic\377o,");
		Send_store_special_str(Ind, 11, 3, TERM_ORANGE, "let's see what you can do.. You ready?");
		break;
	case 7:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Alright, wait here for a moment, I saw \377yZar'am, The Stoic\377o just now..");
		Send_store_special_str(Ind, 8, 10, TERM_ORANGE, "<you hear some shouting>");
		Send_store_special_str(Ind, 11, 3, TERM_ORANGE, "let's see what you can do.. You ready?");
		break;
	case 8:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Alright then, wait here for a moment..");
		Send_store_special_str(Ind, 8, 10, TERM_ORANGE, "<you hear some shouting>");
		Send_store_special_str(Ind, 10, 3, TERM_ORANGE, "This is \377yTalithe, The Sharp\377o,");
		Send_store_special_str(Ind, 11, 3, TERM_ORANGE, "let's see what you can do.. You ready?");
		break;
	case 9:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Alright, wait here for a moment, I saw \377yTalithe, The Sharp\377o just now..");
		Send_store_special_str(Ind, 8, 10, TERM_ORANGE, "<you hear some shouting>");
		Send_store_special_str(Ind, 11, 3, TERM_ORANGE, "let's see what you can do.. You ready?");
		break;
	case 10:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Alright then, wait here for a moment..");
		Send_store_special_str(Ind, 8, 10, TERM_ORANGE, "<you hear some shouting>");
		Send_store_special_str(Ind, 10, 3, TERM_ORANGE, "This is \377yGlynn, The Silent\377o,");
		Send_store_special_str(Ind, 11, 3, TERM_ORANGE, "let's see what you can do.. You ready?");
		break;
	case 11:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Alright, wait here for a moment, I saw \377yGlynn, The Silent\377o just now..");
		Send_store_special_str(Ind, 8, 10, TERM_ORANGE, "<you hear some shouting>");
		Send_store_special_str(Ind, 11, 3, TERM_ORANGE, "let's see what you can do.. You ready?");
		break;
	case 12:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Alright then, wait here for a moment..");
		Send_store_special_str(Ind, 8, 10, TERM_ORANGE, "<you hear some shouting>");
		Send_store_special_str(Ind, 10, 3, TERM_ORANGE, "This is \377yArro, The Wise\377o,");
		Send_store_special_str(Ind, 11, 3, TERM_ORANGE, "let's see what you can do.. You ready?");
		break;
	case 13:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Alright, wait here for a moment, I saw \377yArro, The Wise\377o just now..");
		Send_store_special_str(Ind, 8, 10, TERM_ORANGE, "<you hear some shouting>");
		Send_store_special_str(Ind, 11, 3, TERM_ORANGE, "let's see what you can do.. You ready?");
		break;
	case TOP_RANK * 2:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Alright, wait here for a moment, I saw \377yArro, The Wise\377o just now..");
		Send_store_special_str(Ind, 8, 10, TERM_ORANGE, "<you hear some shouting>");
		Send_store_special_str(Ind, 11, 3, TERM_ORANGE, "let's see what you can do.. You ready?");
		break;
	case TOP_RANK * 2 + 1:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Alright, wait here for a moment, I saw \377yArro, The Wise\377o just now..");
		Send_store_special_str(Ind, 8, 10, TERM_ORANGE, "<you hear some shouting>");
		Send_store_special_str(Ind, 11, 3, TERM_ORANGE, "let's see what you can do.. You ready?");
		break;
	case TOP_RANK * 2 + 2:
		Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Alright, wait here for a moment, I saw \377yArro, The Wise\377o just now..");
		Send_store_special_str(Ind, 8, 10, TERM_ORANGE, "<you hear some shouting>");
		Send_store_special_str(Ind, 11, 3, TERM_ORANGE, "let's see what you can do.. You ready?");
		break;
	}

	/* Increase go level by 1, indicating that we paid the
	   wager for this level and can play free from now on. */
	if (new_wager) p_ptr->go_level++;

	/* Wait for player keypress to start */
	Send_request_key(Ind, RID_GO_START, "- press any key to start the game -");
}

/* Initialize game and initiate first move */
void go_challenge_start(int Ind) {
	player_type *p_ptr = Players[Ind];
	int n;
#ifdef ENGINE_GNUGO
	char path[80];
#endif

	Send_store_special_clr(Ind, 4, 18);
	if (go_err(DOWN, DOWN, "go_challenge_start")) return;

	/* somehow spread from 30k to dan level.. Final stage: Take white!
	   Place opponent and configure game parameters accordingly. */
	/* we have 512 MB.. *//* time per CPU move default is 10s */
	switch (p_ptr->go_level) {
	case 1:
		strcpy(avatar_name, "Dougan, The Quick");
#ifdef ENGINE_FUEGO
		writeToPipe("uct_max_memory 10000000");
		writeToPipe("go_param timelimit 3");
#endif
#ifdef ENGINE_GNUGO
		writeToPipe("level 1");
		random_move_prob = 50;
		writeToPipe("time_settings 0 4 1");//increased from 3, in case 3s lag tolerance exists and messes things up maybe
		/* some site claimed 'm' or 's' can be added, with 'm' for minutes being default.
		   appearently that is nonsense though.
		   someone else claimed there was a 3s lag tolerance, doesn't make much sense
		   regarding its behaviour (exceeding even the specified time frame) though. */
#endif
		player_timelimit_sec = GO_TIME_PY;
		break;
	case 3:
		strcpy(avatar_name, "Rabbik, The Bold");
#ifdef ENGINE_FUEGO
		writeToPipe("uct_max_memory 100000000");
		writeToPipe("go_param timelimit 5");
#endif
#ifdef ENGINE_GNUGO
		writeToPipe("level 1");
		random_move_prob = 15;
		writeToPipe("time_settings 0 5 1");
#endif
		player_timelimit_sec = GO_TIME_PY;
		break;
	case 5:
		strcpy(avatar_name, "Lima, The Witty");
#ifdef ENGINE_FUEGO
		writeToPipe("uct_max_memory 100000000");
		writeToPipe("go_param timelimit 10");
#endif
#ifdef ENGINE_GNUGO
		writeToPipe("level 1");
		random_move_prob = 5;
		writeToPipe("time_settings 0 5 1");
#endif
		player_timelimit_sec = GO_TIME_PY;
		break;
	case 7:
		strcpy(avatar_name, "Zar'am, The Stoic");
#ifdef ENGINE_FUEGO
		writeToPipe("uct_max_memory 200000000");
		writeToPipe("go_param timelimit 10");
#endif
#ifdef ENGINE_GNUGO
		writeToPipe("level 1");
		random_move_prob = 0;
		writeToPipe("time_settings 0 5 1");
#endif
		player_timelimit_sec = GO_TIME_PY;
		break;
	case 9:
		strcpy(avatar_name, "Talithe, The Sharp");
#ifdef ENGINE_FUEGO
		writeToPipe("uct_max_memory 200000000");
		writeToPipe("go_param timelimit 15");
#endif
#ifdef ENGINE_GNUGO
		writeToPipe("level 2");
		random_move_prob = 0;
		writeToPipe("time_settings 0 10 1");
#endif
		player_timelimit_sec = GO_TIME_PY;
		break;
	case 11:
		strcpy(avatar_name, "Glynn, The Silent");
#ifdef ENGINE_FUEGO
		writeToPipe("uct_max_memory 300000000");
		writeToPipe("go_param timelimit 20");
#endif
#ifdef ENGINE_GNUGO
		writeToPipe("level 6");
		random_move_prob = 0;
		writeToPipe("time_settings 0 15 1");
#endif
		player_timelimit_sec = GO_TIME_PY;
		break;
	case 13:
		strcpy(avatar_name, "Arro, The Wise");
#ifdef ENGINE_FUEGO
		writeToPipe("uct_max_memory 300000000");
		writeToPipe("go_param timelimit 20");
#endif
#ifdef ENGINE_GNUGO
		writeToPipe("level 10");
		random_move_prob = 0;
		writeToPipe("time_settings 0 20 1");
#endif
		player_timelimit_sec = GO_TIME_PY;
		break;
	case TOP_RANK * 2 + 1: case TOP_RANK * 2 + 2:
		strcpy(avatar_name, "Arro, The Wise");
#ifdef ENGINE_FUEGO
		writeToPipe("uct_max_memory 300000000");
		writeToPipe("go_param timelimit 20");
#endif
#ifdef ENGINE_GNUGO
		writeToPipe("level 10");
		random_move_prob = 0;
		writeToPipe("time_settings 0 20 1");
#endif
		player_timelimit_sec = GO_TIME_PY;
		break;
	default: /* paranoia */
		s_printf("GO_GAME: BAD LEVEL! %s:%d\n", p_ptr->name, p_ptr->go_level);
		go_challenge_cleanup(FALSE);
		return;
	}
	Send_store_special_str(Ind, 5, GO_BOARD_X + 6 - (strlen(avatar_name) + 1) / 2, TERM_YELLOW, avatar_name);

#ifdef ENGINE_FUEGO
	/* think while opponent is thinking? */
	if (p_ptr->go_level <= 0) writeToPipe("uct_param_player ponder 0");
	else writeToPipe("uct_param_player ponder 1");
#endif

	/* Set colour and notice game start. */
	CPU_has_white = (p_ptr->go_level < TOP_RANK * 2);
	CPU_now_to_move = !CPU_has_white;
	player_timeleft_sec = player_timelimit_sec;

	/* Draw board */
	Send_store_special_str(Ind, GO_BOARD_Y, GO_BOARD_X, TERM_UMBER, " ABCDEFGHJ ");
	Send_store_special_str(Ind, GO_BOARD_Y + 1, GO_BOARD_X, TERM_UMBER, "9.........9");
	Send_store_special_str(Ind, GO_BOARD_Y + 2, GO_BOARD_X, TERM_UMBER, "8.........8");
	Send_store_special_str(Ind, GO_BOARD_Y + 3, GO_BOARD_X, TERM_UMBER, "7.........7");
	Send_store_special_str(Ind, GO_BOARD_Y + 4, GO_BOARD_X, TERM_UMBER, "6.........6");
	Send_store_special_str(Ind, GO_BOARD_Y + 5, GO_BOARD_X, TERM_UMBER, "5.........5");
	Send_store_special_str(Ind, GO_BOARD_Y + 6, GO_BOARD_X, TERM_UMBER, "4.........4");
	Send_store_special_str(Ind, GO_BOARD_Y + 7, GO_BOARD_X, TERM_UMBER, "3.........3");
	Send_store_special_str(Ind, GO_BOARD_Y + 8, GO_BOARD_X, TERM_UMBER, "2.........2");
	Send_store_special_str(Ind, GO_BOARD_Y + 9, GO_BOARD_X, TERM_UMBER, "1.........1");
	Send_store_special_str(Ind, GO_BOARD_Y + 10, GO_BOARD_X, TERM_UMBER, " ABCDEFGHJ ");

	/* Initialize game board tracker */
	for (n = 0; n < 9; n++) {
		strcpy(board_line_old[n], ".........");
		strcpy(board_line[n], ".........");
	}

	/* Initialize moves/passes tracker */
	pass_count = 0;
	last_move_was_pass = FALSE;
	game_over = scoring = FALSE;
	waiting_for_board_update = FALSE;
	move_count = 0; /* to determine when to stop playing random moves really */

	/* Init misc control elements */
	received_board_visuals = FALSE;
	strcpy(last_black_move, "");
	strcpy(last_white_move, "");
	strcpy(game_result, "");

#ifdef ANTI_MIRROR
	/* Init anti-Mirror-Go elements */
	anti_mirror_active = FALSE;
	mirror_count = 0;
	last_cpu_move[0] = 0;
#endif

	/* 'Start the clock' aka the game */
	s_printf("GO_INIT: GAME START '%s' (%d)\n", p_ptr->name, p_ptr->go_level);
	tstart = time(NULL);
	tmstart = localtime(&tstart);

#ifdef ENGINE_GNUGO
	/* Open new SGF file for writing (abuse var 'name') */
	sprintf(path, "go/TomeNET-%04d%02d%02d-%02d%02d%02d.sgf",
	    1900 + tmstart->tm_year, tmstart->tm_mon + 1, tmstart->tm_mday,
	    tmstart->tm_hour, tmstart->tm_min, tmstart->tm_sec);
	sgf = fopen(path, "w");
	if (sgf) {
		fprintf(sgf, "(;SZ[9]RU[Chinese]KM[0]\n");
		fprintf(sgf, "PC[TomeNET - http://www.tomenet.net/]\n");
		if (CPU_has_white) fprintf(sgf, "PW[%s (AI)]PB[%s]BR[%dp]\n", avatar_name, p_ptr->name, p_ptr->go_level);
		else fprintf(sgf, "PW[%s]PB[%s (AI)]WR[%dp]\n", p_ptr->name, avatar_name, p_ptr->go_level);
	}
 #ifdef GO_DEBUGLOG
	else s_printf("GO_SGF: Couldn't open file.\n");
 #endif
#endif

	/* Initiate human player input loop */
	Send_store_special_str(Ind, 1, 1, TERM_WHITE, ">Type coordinates to place a stone, eg \"d5\". Hit ENTER to pass/ESC to resign.<");
	if (CPU_has_white) Send_request_str(Ind, RID_GO_MOVE, "Enter your move: ", "");
	else go_engine_move_CPU();
}

/* Get a response, non-blocking; using go_engine_processing.
   This should be called in the main loop, every second or more frequently,
   eg  if (go_engine_processing) go_engine_process(); . */
void go_engine_process(void) {
//	if (go_err(DOWN, DOWN, "go_engine_process")) {
	if (go_err(DOWN, NONE, "go_engine_process")) {
		go_engine_processing = 0;
		return;
	}

	/* Read all the data that's already waiting for us
	   in the receive buffer (pipe) */
	while (test_for_response() >= 0);
}

/* Actually play the game.
   If CPU starts, py_move is NULL on first call of this function.
   Return values:
   0 - game goes on		1 - invalid move, try again
   2 - py lost on time		3 - py resigned
   4 - cpu lost on time		5 - cpu resigned
   1000 + score - black wins	2000 + score - white wins
   6 - jigo. */
int go_engine_move_human(int Ind, char *py_move) {
	char tmp[80];

	if (go_err(DOWN, DOWN, "go_engine_move_human")) return -1;

	/* parse player move */
	if (strlen(py_move) == 2) { /* Play a normal move on a board grid */
		/* send move */
		if (CPU_has_white) snprintf(tmp, 14, "play black %s", py_move);
		else snprintf(tmp, 14, "play white %s", py_move);
		tmp[13] = '\0';
		writeToPipe(tmp);
		if (CPU_has_white) {
			last_black_move[0] = tolower(py_move[0]);
			if (last_black_move[0] == 'j') last_black_move[0] = 'i';
			last_black_move[1] = py_move[1];
			last_black_move[2] = 0;
#ifdef ANTI_MIRROR
			if ((last_cpu_move[0] == 'i' - last_black_move[0] + 'a') &&
			    (last_cpu_move[1] == '9' - last_black_move[1] + '1')) {
				mirror_count++;
				if (mirror_count == ANTI_MIRROR_THRESHOLD) enable_anti_mirror();
			} else mirror_count = 0;
#endif
		} else {
			last_white_move[0] = tolower(py_move[0]);
			if (last_white_move[0] == 'j') last_white_move[0] = 'i';
			last_white_move[1] = py_move[1];
			last_white_move[2] = 0;
#ifdef ANTI_MIRROR
			if ((last_cpu_move[0] == 'i' - last_white_move[0] + 'a') &&
			    (last_cpu_move[1] == '9' - last_white_move[1] + '1')) {
				mirror_count++;
				if (mirror_count == ANTI_MIRROR_THRESHOLD) enable_anti_mirror();
			} else mirror_count = 0;
#endif
		}
		go_engine_next_action = NACT_MOVE_CPU;
		move_count++;
		return 0;
	} else if (!strcmp(py_move, "p") ||
	    !strcmp(py_move, "")) { /* Pass */
		if (CPU_has_white) writeToPipe("play black pass");
		else writeToPipe("play white pass");
		go_engine_next_action = NACT_MOVE_CPU;
		pass_count++;
		last_move_was_pass = TRUE;
		if (CPU_has_white) strcpy(last_black_move, "");
		else strcpy(last_white_move, "");
		move_count++;
		return 0;
	} else if (!strcmp(py_move, "r") ||
	    !strcmp(py_move, "\e")) { /* Resign */
#if 0 /* there is no resign command */
		if (CPU_has_white) writeToPipe("play black resign");
		else writeToPipe("play white resign");
		go_engine_next_action = NACT_MOVE_CPU;
#endif
		go_engine_move_result(3);
		return 3; //resign
	}

	Send_request_str(Ind, RID_GO_MOVE, "Invalid move. Try again: ", "");
	return 1; //invalid move
}

/* Display and countdown 'clocks' */
void go_engine_clocks(void) {
	int Ind;
	char clock[6];

	if (go_err(DOWN, DOWN, "go_engine_clocks")) return;

	/* Hold clocks during preparation phase (ie before game really commences)
	   or while counting the score at the end: */
	if (game_over || scoring) return;

	/* Player still with us? */
	if (!(Ind = lookup_player_ind(go_engine_player_id))) {
		go_challenge_cleanup(FALSE);
		return;
	}

	/* no timelimit? */
	if (!player_timelimit_sec) return;

	/* countdown */
	if (player_timeleft_sec) player_timeleft_sec--;
	if (!player_timeleft_sec) {
		/* loss on time */
		Send_request_abort(Ind);
		Players[Ind]->request_id = RID_NONE;

		if (CPU_now_to_move) go_engine_move_result(4); /* not possible except for mad lag */
		else go_engine_move_result(2);
	}

	sprintf(clock, "%02d", player_timeleft_sec);
	if (CPU_now_to_move)
		Send_store_special_str(Ind, GO_BOARD_Y + 1, GO_BOARD_X - 8, TERM_SLATE, clock);
	else
		Send_store_special_str(Ind, GO_BOARD_Y + 9, GO_BOARD_X - 8, TERM_L_RED, clock);
}

void go_challenge_cancel(void) {
#if 0 /* done by handle_store_leave() */
void go_challenge_cancel(int Ind) {
	int Ind_go;

	/* Go player still online? */
	if (!(Ind_go = lookup_player_ind(go_engine_player_id))) {
		go_challenge_cleanup(FALSE);
		return;
	}

	/* Go player isn't the player who left the store? (paranoia) */
	if (Ind_go != Ind) return;
#endif

	go_challenge_cleanup(FALSE);
}


/* ------------------------------------------------------------------------- */


static void go_engine_move_CPU() {
	int Ind;
	int tries = 5000, x = -1, y = -1, liberties;
	bool random_move = FALSE;
	char cpu_rnd_move[14], coord[2], cpu_move[7] = {32, 32, 0, 0, 32, 32, 0};

	if (go_err(DOWN, DOWN, "go_engine_move_CPU")) return;

	/* Player still with us? */
	if (!(Ind = lookup_player_ind(go_engine_player_id))) {
		go_challenge_cleanup(FALSE);
		return;
	}

	/* Get CPU move */

	/* replace CPU moves by random moves to simulate lower strength.
	   NOTE: Free grid doesn't necessarily mean LEGAL grid! */
	if (move_count <= RND_MOVE_THRESHOLD && (random_move = magik(random_move_prob))) {
		/* Try to find a free grid randomly */
		while (tries != 0) {
			tries--;
			/* Don't play on the first line! That's TOO bad.. */
			x = rand_int(9 - 2) + 1;
			y = rand_int(9 - 2) + 1;
			if (board_line[y][x] != '.') continue;

			/* Let's go lazy:
			   Test if it has at least 1 more liberty, to avoid us
			   having to test for capturing and nasty stuff.. */
			liberties = 0;
			if (x > 0 && board_line[y][x - 1] == '.') liberties++;
			if (x < 9 - 1 && board_line[y][x + 1] == '.') liberties++;
			if (y > 0 && board_line[y - 1][x] == '.') liberties++;
			if (y < 9 - 1 && board_line[y + 1][x] == '.') liberties++;
			if (liberties == 0) continue;

			s_printf("GO_RND_OK %d,%d\n", x, y);
			break;
		}
		if (tries == 0) random_move = FALSE;
	}

	if (CPU_has_white) {
		if (!random_move) writeToPipe("genmove white");
		else {
			strcpy(cpu_rnd_move, "play white ");
			sprintf(coord, "%c", 'A' + x);
			strcat(cpu_rnd_move, coord);
			sprintf(coord, "%c", '1' + y);
			strcat(cpu_rnd_move, coord);
			writeToPipe(cpu_rnd_move);
			pass_count = 0;

			cpu_move[2] = 'A' + x;
			cpu_move[3] = '1' + y;
//			Send_store_special_str(Ind, 6, GO_BOARD_X + 3, TERM_YELLOW, cpu_move);
			sprintf(tmp, "just played %s", cpu_move + 2);
			Send_store_special_str(Ind, 6, GO_BOARD_X - 1, TERM_YELLOW, tmp);
#ifdef GO_DEBUGLOG
			s_printf("GO_RND: %s\n", cpu_rnd_move + 11);
#endif
			last_white_move[0] = 'a' + x;
			last_white_move[1] = '1' + y;
			last_white_move[2] = 0;
#ifdef ANTI_MIRROR
			strcpy(last_cpu_move, last_white_move);
#endif
			move_count++;
		}
	} else {
		if (!random_move) writeToPipe("genmove black");
		else {
			strcpy(cpu_rnd_move, "play black ");
			sprintf(coord, "%c", 'A' + x);
			strcat(cpu_rnd_move, coord);
			sprintf(coord, "%c", '1' + y);
			strcat(cpu_rnd_move, coord);
			writeToPipe(cpu_rnd_move);
			pass_count = 0;

			cpu_move[2] = 'A' + x;
			cpu_move[3] = '1' + y;
//			Send_store_special_str(Ind, 6, GO_BOARD_X + 3, TERM_YELLOW, cpu_move);
			sprintf(tmp, "just played %s", cpu_move + 2);
			Send_store_special_str(Ind, 6, GO_BOARD_X - 1, TERM_YELLOW, tmp);
#ifdef GO_DEBUGLOG
			s_printf("GO_RND: %s\n", cpu_rnd_move + 11);
#endif
			last_black_move[0] = 'a' + x;
			last_black_move[1] = '1' + y;
			last_black_move[2] = 0;
#ifdef ANTI_MIRROR
			strcpy(last_cpu_move, last_black_move);
#endif
			move_count++;
		}
	}

	go_engine_next_action = NACT_MOVE_PLAYER;

/* (TODO) gnugo3.9.1: <genmove black> -> <= = F6> ??? */
}

/* Reads current pipe_buf[][] and assumes it contains the response
   to a player move. Evaluates result and further proceeding from that. */
static int verify_move_human(void) {
	int Ind;
	player_type *p_ptr;
	char clock[6];

	if (go_err(DOWN, DOWN, "verify_move_human")) return -2;

	/* Player still with us? */
	if (!(Ind = lookup_player_ind(go_engine_player_id))) {
		go_challenge_cleanup(FALSE);
		return -1;
	}
	p_ptr = Players[Ind];

#ifdef ENGINE_FUEGO
	/* Catch timeout! Oops. */
	if (!strcmp(pipe_buf[MAX_GTP_LINES - 1], "SgTimeRecord: outOfTime")) {
 #ifdef GO_DEBUGLOG
		s_printf("GO_GAME: timeout %s\n", p_ptr->name);
 #endif
		return 2; //time over (py)
	}
#endif
	/* Catch illegal moves */
//	if (strstr("? point outside board"))
	if (pipe_buf[MAX_GTP_LINES - 1][0] == '?') {
//		Send_request_str(Ind, RID_GO_MOVE, "Invalid move. Try again: ", "");
		return 1; //invalid move
	}

#ifdef ENGINE_GNUGO
	if (sgf) {
		/* translate char/num coords into char/char coords */
		char m[3];
		if (CPU_has_white) {
			if (strlen(last_black_move) == 2) {
				m[0] = last_black_move[0];
				m[1] = 'a' + 9 - 1 - (last_black_move[1] - '1');
				m[2] = 0;
			} else strcpy(m, "");
			fprintf(sgf, ";B[%s]\n", m);
		} else {
			if (strlen(last_white_move) == 2) {
				m[0] = last_white_move[0];
				m[1] = 'a' + 9 - 1 - (last_white_move[1] - '1');
				m[2] = 0;
			} else strcpy(m, "");
			fprintf(sgf, ";W[%s]\n", m);
		}
	}
#endif

	/* Test for game over by double-passe */
	if (pass_count == 2) {
#ifdef GO_DEBUGPRINT
		printf("Both players passed consecutively, so the game ends.\n");
#endif
		/* determine score */
		scoring = TRUE;
		writeToPipe("final_score");
		return 0;
	} else if (!last_move_was_pass) pass_count = 0;
	last_move_was_pass = FALSE;

	player_timeleft_sec = player_timelimit_sec;
	Send_store_special_str(Ind, GO_BOARD_Y + 9, GO_BOARD_X - 8, TERM_L_RED, "  ");
	sprintf(clock, "%02d", player_timeleft_sec);
	Send_store_special_str(Ind, GO_BOARD_Y + 1, GO_BOARD_X - 8, TERM_SLATE, clock);
	CPU_now_to_move = TRUE;

	/* display board after player's move */
	writeToPipe("showboard");
	waiting_for_board_update = TRUE;

	/* continue playing */
	return 0;
}

/* Reads current pipe_buf[][] and assumes it contains the response
   to a CPU move. Evaluates result and further proceeding from that. */
static int verify_move_CPU(void) {
	int Ind;
	char cpu_move[80], clock[6];

	/* Player still with us? */
	if (!(Ind = lookup_player_ind(go_engine_player_id))) {
		go_challenge_cleanup(FALSE);
		return -1;
	}

	if (go_err(DOWN, DOWN, "verify_move_CPU")) return -2;

#ifdef ENGINE_FUEGO
	/* Catch timeout! Oops. */
	if (!strcmp(pipe_buf[MAX_GTP_LINES - 1], "SgTimeRecord: outOfTime")) {
 #ifdef GO_DEBUGPRINT
		printf("Your opponent's time has run out, therefore you have won the match!\n");
 #endif
		return 4;
	}
#endif
	/* CPU resigns */
	else if (!strcmp(pipe_buf[MAX_GTP_LINES - 1], "= resign")) {
#ifdef GO_DEBUGPRINT
		printf("Your opponent has resigned, you have won the match!\n");
#endif
		return 5;
	}
	/* CPU passes */
	else if (!strcmp(pipe_buf[MAX_GTP_LINES - 1], "= PASS")) {
#ifdef GO_DEBUGPRINT
		printf("CPU passed.\n");
#endif
//		Send_store_special_str(Ind, 6, GO_BOARD_X + 3, TERM_YELLOW, "'pass'");
		Send_store_special_str(Ind, 6, GO_BOARD_X + 3, TERM_YELLOW, "passed");
		pass_count++;

		if (CPU_has_white) strcpy(last_white_move, "");
		else strcpy(last_black_move, "");

		move_count++;
	}
	/* CPU plays a normal move */
	else if (strlen(pipe_buf[MAX_GTP_LINES - 1]) == 4 &&
	    pipe_buf[MAX_GTP_LINES - 1][0] == '=' &&
	    pipe_buf[MAX_GTP_LINES - 1][1] == ' ') {
#if 0
		strncpy(cpu_move, pipe_buf[MAX_GTP_LINES - 1], 4);
		cpu_move[0] = cpu_move[2];
		cpu_move[1] = cpu_move[3];
		cpu_move[2] = 0;
//		Send_store_special_str(Ind, 6, GO_BOARD_X + 5, TERM_YELLOW, cpu_move);
		sprintf(tmp, "just played %s", cpu_move + 2);
		Send_store_special_str(Ind, 6, GO_BOARD_X, TERM_YELLOW, tmp);
#else
		strcpy(cpu_move, pipe_buf[MAX_GTP_LINES - 1]);
		cpu_move[0] = cpu_move[1] = ' ';
		cpu_move[4] = cpu_move[5] = ' ';
		cpu_move[6] = 0;
//		Send_store_special_str(Ind, 6, GO_BOARD_X + 3, TERM_YELLOW, cpu_move);
		sprintf(tmp, "just played %s", cpu_move + 2);
		Send_store_special_str(Ind, 6, GO_BOARD_X - 1, TERM_YELLOW, tmp);
#endif
		pass_count = 0;

		if (CPU_has_white) {
			last_white_move[0] = tolower(cpu_move[2]);
			if (last_white_move[0] == 'j') last_white_move[0] = 'i';
			last_white_move[1] = cpu_move[3];
			last_white_move[2] = 0;
#ifdef ANTI_MIRROR
			strcpy(last_cpu_move, last_white_move);
#endif
		} else {
			last_black_move[0] = tolower(cpu_move[2]);
			if (last_black_move[0] == 'j') last_black_move[0] = 'i';
			last_black_move[1] = cpu_move[3];
			last_black_move[2] = 0;
#ifdef ANTI_MIRROR
			strcpy(last_cpu_move, last_black_move);
#endif
		}
		move_count++;
	}

#ifdef ENGINE_GNUGO
	if (sgf) {
		/* translate char/num coords into char/char coords */
		char m[3];
		if (CPU_has_white) {
			if (strlen(last_white_move) == 2) {
				m[0] = last_white_move[0];
				m[1] = 'a' + 9 - 1 - (last_white_move[1] - '1');
				m[2] = 0;
			} else strcpy(m, "");
			fprintf(sgf, ";W[%s]\n", m);
		} else {
			if (strlen(last_black_move) == 2) {
				m[0] = last_black_move[0];
				m[1] = 'a' + 9 - 1 - (last_black_move[1] - '1');
				m[2] = 0;
			} else strcpy(m, "");
			fprintf(sgf, ";B[%s]\n", m);
		}
	}
#endif

	/* Test for game over by double-passe */
	if (pass_count == 2) {
#ifdef GO_DEBUGPRINT
		printf("Both players passed consecutively, so the game ends.\n");
#endif
		/* determine score */
		scoring = TRUE;
		writeToPipe("final_score");
		return 0;
	}

	player_timeleft_sec = player_timelimit_sec;
	Send_store_special_str(Ind, GO_BOARD_Y + 1, GO_BOARD_X - 8, TERM_SLATE, "  ");
	sprintf(clock, "%02d", player_timeleft_sec);
	Send_store_special_str(Ind, GO_BOARD_Y + 9, GO_BOARD_X - 8, TERM_L_RED, clock);
	CPU_now_to_move = FALSE;

	/* display board after CPU's move */
	writeToPipe("showboard");
	waiting_for_board_update = TRUE;

	/* continue playing */
	return 0;
}

static void go_engine_move_result(int move_result) {
	bool invalid_move = FALSE, lost = FALSE, won = FALSE;
	char result[80];
	int Ind, wager = 0;
	player_type *p_ptr;

	if (go_err(DOWN, DOWN, "go_engine_move_result")) return;

	/* Player still with us? */
	if (!(Ind = lookup_player_ind(go_engine_player_id))) {
		go_challenge_cleanup(FALSE);
		return;
	}
	p_ptr = Players[Ind];

	switch (move_result) {
	case 0: /* player's move was valid */
		if (CPU_to_move) {
			/* clear previous cpu move display */
			Send_store_special_str(Ind, 6, GO_BOARD_X - 2, TERM_YELLOW, "              ");
		}
		break;
	case 1: invalid_move = TRUE; break;
	case 2:
#ifdef ENGINE_FUEGO
		if (CPU_has_white) writeToPipe("go_set_info result W+Time");
		else writeToPipe("go_set_info result B+Time");
#endif
#ifdef ENGINE_GNUGO
		if (sgf) {
			if (CPU_has_white) fprintf(sgf, "RE[W+Time]\n");
			else fprintf(sgf, "RE[B+Time]\n");
		}
#endif
		strcpy(result, "\377r*** You lost on time! ***");
		lost = TRUE;

		break;
	case 3:
#ifdef ENGINE_FUEGO
		if (CPU_has_white) writeToPipe("go_set_info result W+Resign");
		else writeToPipe("go_set_info result B+Resign");
#endif
#ifdef ENGINE_GNUGO
		if (sgf) {
			if (CPU_has_white) fprintf(sgf, "RE[W+Resign]\n");
			else fprintf(sgf, "RE[B+Resign]\n");
		}
#endif
		strcpy(result, "\377r*** You resigned! ***");
		lost = TRUE;
		break;
	case 4:
#ifdef ENGINE_FUEGO
		if (CPU_has_white) writeToPipe("go_set_info result B+Time");
		else writeToPipe("go_set_info result W+Time");
#endif
#ifdef ENGINE_GNUGO
		if (sgf) {
			if (CPU_has_white) fprintf(sgf, "RE[B+Time]\n");
			else fprintf(sgf, "RE[W+Time]\n");
		}
#endif
		strcpy(result, "\377G*** Your opponent lost on time! ***");
		won = TRUE;
		break;
	case 5:
#ifdef ENGINE_FUEGO
		if (CPU_has_white) writeToPipe("go_set_info result B+Resign");
		else writeToPipe("go_set_info result W+Resign");
#endif
#ifdef ENGINE_GNUGO
		if (sgf) {
			if (CPU_has_white) fprintf(sgf, "RE[B+Resign]\n");
			else fprintf(sgf, "RE[W+Resign]\n");
		}
#endif
		strcpy(result, "\377G*** Your opponent resigned! ***");
		won = TRUE;
		break;
	case 6:
#ifdef ENGINE_FUEGO
		writeToPipe("go_set_info result Jigo");
#endif
#ifdef ENGINE_GNUGO
		if (sgf) fprintf(sgf, "RE[0]\n");
#endif
		strcpy(result, "\377o** The game is a draw! **");
		break;
	default:
		if (move_result >= 2000) { /* White wins */
#ifdef ENGINE_FUEGO
			sprintf(result, "go_set_info result W+%d", move_result - 2000);
			writeToPipe(result);
#endif
#ifdef ENGINE_GNUGO
			if (sgf) fprintf(sgf, "RE[W+%d]\n", move_result - 2000);
#endif
			if (CPU_has_white) {
				sprintf(result, "\377r*** Your opponent won by %d point%s! ***", move_result - 2000, move_result - 2000 == 1 ? "" : "s");
				lost = TRUE;
			} else {
				sprintf(result, "\377G*** You won by %d point%s! ***", move_result - 2000, move_result - 2000 == 1 ? "" : "s");
				won = TRUE;
			}
		} else { /* Black wins */
#ifdef ENGINE_FUEGO
			sprintf(result, "go_set_info result B+%d", move_result - 1000);
			writeToPipe(result);
#endif
#ifdef ENGINE_GNUGO
			if (sgf) fprintf(sgf, "RE[B+%d]\n", move_result - 1000);
#endif
			if (CPU_has_white) {
				sprintf(result, "\377G*** You won by %d point%s! ***", move_result - 1000, move_result - 1000 == 1 ? "" : "s");
				won = TRUE;
			} else {
				sprintf(result, "\377r*** Your opponent won by %d point%s! ***", move_result - 1000, move_result - 1000 == 1 ? "" : "s");
				lost = TRUE;
			}
		}
	}

	if (move_result >= 2) {
		Send_store_special_str(Ind, 1, 0, TERM_WHITE, "                                                                              ");
		Send_store_special_str(Ind, 6, GO_BOARD_X + 6 - strlen(result) / 2, TERM_WHITE, result);

		/* hide clocks */
		Send_store_special_str(Ind, GO_BOARD_Y + 1, GO_BOARD_X - 8, TERM_L_RED, "  ");
		Send_store_special_str(Ind, GO_BOARD_Y + 9, GO_BOARD_X - 8, TERM_L_RED, "  ");

#ifdef GO_DEBUGPRINT
		printf("Result: %s\n", result);
#endif
#ifdef GO_DEBUGLOG
		s_printf("GO_RESULT: %s\n", result);
#endif

		switch (p_ptr->go_level) {
		case 1:
			if (lost) {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Heh, bloody beginner are ya?");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "Ah well, figure it out and");
				Send_store_special_str(Ind, 11, GO_BOARD_X + 13, TERM_ORANGE, "you'll beat him next time..");
			} else if (won) {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Hah, I thought you'd make");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "short work of Dougan, saw");
				Send_store_special_str(Ind, 11, GO_BOARD_X + 13, TERM_ORANGE, "right through ya! I'll make");
				Send_store_special_str(Ind, 12, GO_BOARD_X + 13, TERM_ORANGE, "y'a better offer next time!");
				wager = wager_lvl[p_ptr->go_level / 2];
				p_ptr->go_level++;
			} else {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "You two are on the same..");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "level eh? Hahaha! That's");
				Send_store_special_str(Ind, 11, GO_BOARD_X + 13, TERM_ORANGE, "just great!");
			}
			break;
		case 3:
			if (lost) {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Hm, so Rabbik is already");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "to much for you, eh?");
			} else if (won) {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Yeah yeah, Rabbik isn't strong");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "really, I was sure you'd win.");
				wager = wager_lvl[p_ptr->go_level / 2];
				p_ptr->go_level++;
			} else {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Oh, a draw. Happens rarely.");
			}
			break;
		case 5:
			if (lost) {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Lima too tough for you?");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "She's by far not among the");
				Send_store_special_str(Ind, 11, GO_BOARD_X + 13, TERM_ORANGE, "most skillful players really,");
				Send_store_special_str(Ind, 12, GO_BOARD_X + 13, TERM_ORANGE, "you just gotta train more.");
			} else if (won) {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Ho, so Lima wasn't witty");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "enough for you. Go on!");
				wager = wager_lvl[p_ptr->go_level / 2];
				p_ptr->go_level++;
			} else {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "A draw.. small chance for");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "that to happen.");
			}
			break;
		case 7:
			if (lost) {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Zar'am ain't no weakling.");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "To be honest, I thought you'd");
				Send_store_special_str(Ind, 11, GO_BOARD_X + 13, TERM_ORANGE, "beat him though!");
			} else if (won) {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "You beat Zar'am, not bad.");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "I'll see to giving you a");
				Send_store_special_str(Ind, 11, GO_BOARD_X + 13, TERM_ORANGE, "stronger opponent next time!");
				wager = wager_lvl[p_ptr->go_level / 2];
				p_ptr->go_level++;
			} else {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Hah, that's not result, it's");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "an excuse!");
			}
			break;
		case 9:
			if (lost) {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Yeah yeah, she's too strong.");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "Was obvious ya can't do much");
				Send_store_special_str(Ind, 11, GO_BOARD_X + 13, TERM_ORANGE, "against her. So.. train more!");
			} else if (won) {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Heh, you managed to defend");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "Talithe? She's quite strong.");
				Send_store_special_str(Ind, 11, GO_BOARD_X + 13, TERM_ORANGE, "Well looks like things start");
				Send_store_special_str(Ind, 12, GO_BOARD_X + 13, TERM_ORANGE, "getting interesting.");
				wager = wager_lvl[p_ptr->go_level / 2];
				p_ptr->go_level++;
			} else {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "You kidding me, I don't want");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "a draw. I want a winner!");
			}
			break;
		case 11:
			if (lost) {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "There's just no way for");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "you to beat someone scary");
				Send_store_special_str(Ind, 11, GO_BOARD_X + 13, TERM_ORANGE, "like Glynn in the next");
				Send_store_special_str(Ind, 12, GO_BOARD_X + 13, TERM_ORANGE, "hundred years, haha.");
			} else if (won) {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "What the.. you won against");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "Glynn? I didn't expect that.");
				Send_store_special_str(Ind, 11, GO_BOARD_X + 13, TERM_ORANGE, "Seems it takes a master to");
				Send_store_special_str(Ind, 12, GO_BOARD_X + 13, TERM_ORANGE, "stop you.");
				wager = wager_lvl[p_ptr->go_level / 2];
				p_ptr->go_level++;
			} else {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Whoa, that's one high-level");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "draw if I ever saw one.");
			}
			break;
		case 13:
			if (lost) {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Don't be angry, this guy's");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "a true master of the game,");
				Send_store_special_str(Ind, 11, GO_BOARD_X + 13, TERM_ORANGE, "friend. Even someone skilled");
				Send_store_special_str(Ind, 12, GO_BOARD_X + 13, TERM_ORANGE, "as you will have to admit");
				Send_store_special_str(Ind, 13, GO_BOARD_X + 13, TERM_ORANGE, "defeat in his case!");
			} else if (won) {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "What the heck! Impossible!");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "You defeated master Arro?");
				Send_store_special_str(Ind, 11, GO_BOARD_X + 13, TERM_ORANGE, "Well that leaves me kinda");
				Send_store_special_str(Ind, 12, GO_BOARD_X + 13, TERM_ORANGE, "dumb-founded. I can just say");
				Send_store_special_str(Ind, 13, GO_BOARD_X + 13, TERM_ORANGE, "why don't you switch colours!");
				wager = wager_lvl[p_ptr->go_level / 2];
				p_ptr->go_level++;
			} else {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Impressive, never believed");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "you could achieve so much as");
				Send_store_special_str(Ind, 11, GO_BOARD_X + 13, TERM_ORANGE, "a draw versus master Arro!");
			}
			break;
		case TOP_RANK * 2 + 1:
			if (lost) {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Well you've surprised us all");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "when you beat him with black.");
				Send_store_special_str(Ind, 11, GO_BOARD_X + 13, TERM_ORANGE, "Taking white is a different");
				Send_store_special_str(Ind, 12, GO_BOARD_X + 13, TERM_ORANGE, "matter though, hear me!");
			} else if (won) {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Unbelievable what you managed");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "to pull off there!");
				Send_store_special_str(Ind, 11, GO_BOARD_X + 13, TERM_ORANGE, "Beating a master like Arro");
				Send_store_special_str(Ind, 12, GO_BOARD_X + 13, TERM_ORANGE, "with white..here's your money!");
				Send_store_special_str(Ind, 13, GO_BOARD_X + 13, TERM_ORANGE, "If you want to play again,");
				Send_store_special_str(Ind, 14, GO_BOARD_X + 13, TERM_ORANGE, "it'll be on the house, my word!");
				wager = wager_lvl[p_ptr->go_level / 2];
				p_ptr->go_level++;
			} else {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Oho a draw between masters..");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "can't ask for much more!");
			}
			break;
		case TOP_RANK * 2 + 2:
			if (lost) {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Hah, I can't judge what went");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "wrong there, your games go way");
				Send_store_special_str(Ind, 11, GO_BOARD_X + 13, TERM_ORANGE, "over my head..");
			} else if (won) {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Impressive as always, love to");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "watch this stuff!");
			} else {
				Send_store_special_str(Ind, 9, GO_BOARD_X + 13, TERM_ORANGE, "Awesome when it turns out a");
				Send_store_special_str(Ind, 10, GO_BOARD_X + 13, TERM_ORANGE, "draw! You'd think it's so");
				Send_store_special_str(Ind, 11, GO_BOARD_X + 13, TERM_ORANGE, "totally unlikely..");
			}
			break;
		}

		/* earn winnings */
		if (wager) {
			/* double return */
			wager *= 2;
			gain_au(Ind, wager, FALSE, FALSE);
			Send_gold(Ind, p_ptr->au, p_ptr->balance);
		}

		game_over = TRUE;
		go_challenge_cleanup(FALSE);
		return;
	}

	if (p_ptr->store_num == -1) {
		/* "player resigned by leaving the store" */
		go_challenge_cleanup(FALSE);
		return;
	}

	if (invalid_move) {
		Send_request_str(Ind, RID_GO_MOVE, "Invalid move. Try again: ", "");
		return;
	}

	/* If we're in the scoring phase due to 2 consecutive passes,
	   don't prompt for next move anymore. */
	if (scoring) return;


	/* moved the next move prompting to receiving a reply to 'showboard' command!
	   (was required for random move generation algo) */
	return;
#if 0
	/* Prompt for next move */
	if (CPU_to_move) {
		go_engine_move_CPU();
	} else {
		Send_request_str(Ind, RID_GO_MOVE, "Enter your move: ", "");
		Send_store_special_str(Ind, 1, 1, TERM_WHITE, ">Type coordinates to place a stone, eg \"d5\". Hit ENTER to pass/ESC to resign.<");
	}
#endif
}

/* Process incoming reply pieces to a previous command (Async read) */
static int test_for_response() {
	int i;
	char tmp[80];//, *tptr = tmp + 79;
	char pipe_line_buf[160];
	int Ind = lookup_player_ind(go_engine_player_id);

#ifdef DISCARD_RESPONSES_WHEN_TERMINATING
	/* we might not even want any responses (and them causing slowdown) here,
	   in case we're doing a warm restart while TomeNET server keeps running! */
	if (terminating) return 0;
#endif

	if (go_err(DOWN, NONE, "test_for_response")) return -3;

	/* paranoia: invalid file/pipe? */
	if (!fr) {
		s_printf("GO_ENGINE: ERROR - fr is NULL.\n");
		go_engine_processing = 0;
		return -2;
	}

	strcpy(tmp, "");
	strcpy(pipe_line_buf, "");


	/* Handle program output ------------------------------------------- */
	readFromPipe(pipe_line_buf, &pipe_response_complete);

	/* No data waiting to be read? */
	if (!pipe_line_buf[0]) return -1;

	/* Process data: Build lines from it. Build whole response from several lines. */
	if (strlen(pipe_buf[pipe_buf_current_line]) + strlen(pipe_line_buf) >= 240) {
		s_printf("GO_ERROR: LINE TOO LONG\n");
		return 1;
	} else strcat(pipe_buf[pipe_buf_current_line], pipe_line_buf);

	/* Trim ending '\n' */
	if (pipe_buf[pipe_buf_current_line][strlen(pipe_buf[pipe_buf_current_line]) - 1] == '\n')
		pipe_buf[pipe_buf_current_line][strlen(pipe_buf[pipe_buf_current_line]) - 1] = 0;


	/* Not even got a while line? Exit now. ---------------------------- */
	if (pipe_line_buf[strlen(pipe_line_buf) - 1] != '\n') return 0;

	/* don't output just a single line feed */
#ifdef GO_DEBUGPRINT
	if (strlen(pipe_buf[pipe_buf_current_line])) printf("<%s>\n", pipe_buf[pipe_buf_current_line]);
#endif

	/* Check for certain important responses */
	if (strlen(pipe_buf[pipe_buf_current_line]) > 2 &&
	    /* Special answers? */
	    (strstr(pipe_buf[pipe_buf_current_line], "outOfTime") ||
	    /* either postitive answer? '= ...' */
	    (pipe_buf[pipe_buf_current_line][0] == '=' && pipe_buf[pipe_buf_current_line][1] == ' ') ||
	    /* or negative answer? '? ...' */
	    (pipe_buf[pipe_buf_current_line][0] == '?' && pipe_buf[pipe_buf_current_line][1] == ' '))) {
		/* Keep it in mind! */
		strncpy(tmp, pipe_buf[pipe_buf_current_line], 79);
		tmp[79] = 0;
	}

	/* Received a board layout line? (from 'showboard') */
	if (strlen(pipe_buf[pipe_buf_current_line]) > 2 &&
#ifdef ENGINE_FUEGO
	    pipe_buf[pipe_buf_current_line][1] == ' ' &&
	    pipe_buf[pipe_buf_current_line][0] >= '1' && pipe_buf[pipe_buf_current_line][0] <= '9') {
		i = pipe_buf[pipe_buf_current_line][0] - '1';
		//strncpy(board_line[i], pipe_buf[pipe_buf_current_line] + 2, 10); it's SPACED!:
		/* despace */
		board_line[i][0] = pipe_buf[pipe_buf_current_line][2];
		board_line[i][1] = pipe_buf[pipe_buf_current_line][4];
		board_line[i][2] = pipe_buf[pipe_buf_current_line][6];
		board_line[i][3] = pipe_buf[pipe_buf_current_line][8];
		board_line[i][4] = pipe_buf[pipe_buf_current_line][10];
		board_line[i][5] = pipe_buf[pipe_buf_current_line][12];
		board_line[i][6] = pipe_buf[pipe_buf_current_line][14];
		board_line[i][7] = pipe_buf[pipe_buf_current_line][16];
		board_line[i][8] = pipe_buf[pipe_buf_current_line][18];
#endif
#ifdef ENGINE_GNUGO
	    pipe_buf[pipe_buf_current_line][0] == ' ' &&
	    pipe_buf[pipe_buf_current_line][1] >= '1' && pipe_buf[pipe_buf_current_line][1] <= '9') {
		i = pipe_buf[pipe_buf_current_line][1] - '1';
		/* despace */
		board_line[i][0] = pipe_buf[pipe_buf_current_line][3];
		board_line[i][1] = pipe_buf[pipe_buf_current_line][5];
		board_line[i][2] = pipe_buf[pipe_buf_current_line][7];
		board_line[i][3] = pipe_buf[pipe_buf_current_line][9];
		board_line[i][4] = pipe_buf[pipe_buf_current_line][11];
		board_line[i][5] = pipe_buf[pipe_buf_current_line][13];
		board_line[i][6] = pipe_buf[pipe_buf_current_line][15];
		board_line[i][7] = pipe_buf[pipe_buf_current_line][17];
		board_line[i][8] = pipe_buf[pipe_buf_current_line][19];
#endif
		board_line[i][9] = 0;
		received_board_visuals = TRUE;
	}


	/* If response is too long, just overwrite the final line repeatedly -
	   it should be all that we need at this point anymore. */
	if (pipe_buf_current_line < MAX_GTP_LINES - 1 -1) {
		pipe_buf_current_line++;
	}
	strcpy(pipe_buf[pipe_buf_current_line], "");

	/* Hack: Deliver certain important responses back outside,
	   using an extra last line, reserved for this purpose: */
	/* also convert to upper-case */
//	while (tptr > tmp) *tptr-- = toupper(*tptr);
	if (tmp[0]) strcpy(pipe_buf[MAX_GTP_LINES - 1], tmp);


	/* Not yet a whole response? Exit here. ---------------------------- */
	if (pipe_response_complete != 2) return 0;

	/* If we read a complete response to a command,
	   reduce amount of pending responses left,
	   and check if we have to react ie initiate a new command */
//	printf("---RESPONSE COMPLETE---\n");
	pipe_response_complete = 0;

#ifdef GO_DEBUGLOG
	if (pipe_buf[MAX_GTP_LINES - 1][0])
		s_printf("GO_REPLY: <%s>\n", pipe_buf[MAX_GTP_LINES - 1]);
#endif

	/* Read 'final_score' reply */
	if (strlen(pipe_buf[MAX_GTP_LINES - 1]) >= 4) {
		if (pipe_buf[MAX_GTP_LINES - 1][2] == 'W' &&
		    pipe_buf[MAX_GTP_LINES - 1][3] == '+') {
			scoring = FALSE;
			/* paranoia: '= W+0' */
			if (pipe_buf[MAX_GTP_LINES - 1][4] != '0') {
#ifdef GO_DEBUGPRINT
				printf("White wins!\n");
#endif
				go_engine_move_result(2000 + atoi(&pipe_buf[MAX_GTP_LINES - 1][4]));
			} else {
#ifdef GO_DEBUGPRINT
				printf("The game is a draw!\n");
#endif
				go_engine_move_result(6);
			}
		} else if (pipe_buf[MAX_GTP_LINES - 1][2] == 'B' &&
		    pipe_buf[MAX_GTP_LINES - 1][3] == '+') {
			scoring = FALSE;
			/* paranoia: '= B+0' */
			if (pipe_buf[MAX_GTP_LINES - 1][4] != '0') {
#ifdef GO_DEBUGPRINT
				printf("Black wins!\n");
#endif
				go_engine_move_result(1000 + atoi(&pipe_buf[MAX_GTP_LINES - 1][4]));
			} else {
#ifdef GO_DEBUGPRINT
				printf("The game is a draw!\n");
#endif
				go_engine_move_result(6);
			}
		}
	} else if (strlen(pipe_buf[MAX_GTP_LINES - 1]) == 3 &&
	    pipe_buf[MAX_GTP_LINES - 1][0] == '=' &&
	    pipe_buf[MAX_GTP_LINES - 1][1] == ' ' &&
	    pipe_buf[MAX_GTP_LINES - 1][2] == '0') {
		scoring = FALSE;
#ifdef GO_DEBUGPRINT
		printf("The game is a draw!\n");
#endif
		go_engine_move_result(6); //jigo!
	}

	/* If we lost on time, go_game_up might be FALSE */
	if (go_game_up && received_board_visuals) go_engine_board();
	received_board_visuals = FALSE;

	/* one less pending response */
	go_engine_processing--;

	/* do we have to react with a new command?
	   This should only happen if no more responses are expected
	   to come in: So we don't initiate a new command before an
	   'important' command's response has been processed completely. */
	if (go_engine_processing == 0) {
		/* Did we read the last 'showboard's response?
		   Then we can proceed with the game. */
	    if (waiting_for_board_update) {
			waiting_for_board_update = FALSE;
			/* Prompt for next move */
			if (CPU_to_move) {
				go_engine_move_CPU();
			} else {
				if (Ind) {
					Send_request_str(Ind, RID_GO_MOVE, "Enter your move: ", "");
					Send_store_special_str(Ind, 1, 1, TERM_WHITE, ">Type coordinates to place a stone, eg \"d5\". Hit ENTER to pass/ESC to resign.<");
				}
			}
	    } else {

		/* If we lost on time, go_game_up might be FALSE */
		if (go_game_up) switch (go_engine_next_action) {
		case NACT_NONE:		/* nothing to do, last command was just misc stuff */
			break;
		case NACT_MOVE_PLAYER:	/* Last command was to request the player's next move */
			/* remember whose turn it is, so we can continue with erased NACT */
			CPU_to_move = FALSE;
			/* erase NACT, so we can writeToPipe misc stuff again */
			go_engine_next_action = NACT_NONE;
			/* evaluate last move, and initiate next one */
			go_engine_move_result(verify_move_CPU());
			break;
		case NACT_MOVE_CPU:	/* Last command was to request the CPU's next move */
			/* remember whose turn it is, so we can continue with erased NACT */
			CPU_to_move = TRUE;
			/* erase NACT, so we can writeToPipe misc stuff again */
			go_engine_next_action = NACT_NONE;
			/* evaluate last move, and initiate next one */
			go_engine_move_result(verify_move_human());
			break;
		}
		else go_engine_next_action = NACT_NONE;

	    }
	}

	/* Initialize response buffer for next response.
	   (important for the last 'tmp' line storing special info.) */
	pipe_buf_current_line = 0;
	for (i = 0; i < MAX_GTP_LINES; i++) strcpy(pipe_buf[i], "");

	/* exit normally, after having read a complete response,
	   ready to proceed the next one on next call */
	return 0;
}

/* Get a response, blocking */
static int wait_for_response() {
	int cont, r = 0, i;
	char lbuf[80], tmp[80];//, *tptr = tmp + 79;
	strcpy(tmp, "");

	if (!go_engine_up) return 2;

#ifdef DISCARD_RESPONSES_WHEN_TERMINATING
	/* we might not even want any responses (and them causing slowdown) here,
	   in case we're doing a warm restart while TomeNET server keeps running! */
	if (terminating) return 0;
#endif

	/* paranoia: invalid file/pipe? */
	if (!fr) {
		s_printf("GO_ENGINE: ERROR - fr is NULL.\n");
		return 1;
	}

	/* reduce pending responses by one, which we are waiting for, in here */
	go_engine_processing--;

	cont = 0;
	do {
		strcpy(lbuf, "");
		strcpy(pipe_buf[r], "");
		while (1) {
			/* handle program output */
			readFromPipe(lbuf, &cont);
			if (lbuf[0]) {
				if (strlen(pipe_buf[r]) + strlen(lbuf) >= 240) {
					s_printf("GO_ERROR: LINE TOO LONG\n");
					break;
				} else strcat(pipe_buf[r], lbuf);
				if (pipe_buf[r][strlen(pipe_buf[r]) - 1] == '\n') pipe_buf[r][strlen(pipe_buf[r]) - 1] = 0;

				if (lbuf[strlen(lbuf) - 1] == '\n') {
					/* don't output just a single line feed */
#ifdef GO_DEBUGPRINT
					if (strlen(pipe_buf[r])) printf("<%s>\n", pipe_buf[r]);
#endif
					/* Check for certain important responses */
					if (strlen(pipe_buf[r]) > 2 &&
					    /* Special answers? */
					    (strstr(pipe_buf[r], "outOfTime") ||
					    /* either postitive answer? '= ...' */
					    (pipe_buf[r][0] == '=' && pipe_buf[r][1] == ' ') ||
					    /* or negative answer? '? ...' */
					    (pipe_buf[r][0] == '?' && pipe_buf[r][1] == ' '))) {
						/* Keep it in mind! */
						strncpy(tmp, pipe_buf[r], 79);
						tmp[79] = 0;
					}
					/* Received a board layout line? (from 'showboard') */
					if (strlen(pipe_buf[r]) > 2 &&
					    pipe_buf[r][1] == ' ' &&
					    pipe_buf[r][0] >= '1' && pipe_buf[r][0] <= '9') {
						i = pipe_buf[r][0] - '1';
						//strncpy(board_line[i], pipe_buf[r] + 2, 10); it's SPACED!:
						/* despace */
						board_line[i][0] = pipe_buf[r][2];
						board_line[i][1] = pipe_buf[r][4];
						board_line[i][2] = pipe_buf[r][6];
						board_line[i][3] = pipe_buf[r][8];
						board_line[i][4] = pipe_buf[r][10];
						board_line[i][5] = pipe_buf[r][12];
						board_line[i][6] = pipe_buf[r][14];
						board_line[i][7] = pipe_buf[r][16];
						board_line[i][8] = pipe_buf[r][18];
						board_line[i][9] = 0;
					}
					break;
				}
			}
		}
		if (r < MAX_GTP_LINES - 1 -1) r++;
	} while (cont != 2);
//	printf("---RESPONSE COMPLETE---\n");

	/* Hack: Deliver certain important responses back outside */
	if (tmp[0]) {
#if 0
		/* also convert to upper-case */
		char *tptr = tmp + strlen(tmp) - 1;
		while (tptr > tmp) *tptr-- = toupper(*tptr);
#endif
#ifdef GO_DEBUGLOG
		s_printf("GO_REPLY: <%s>\n", tmp);
#endif
	}
	strcpy(pipe_buf[MAX_GTP_LINES - 1], tmp);

	return 0;
}

/* Write a line of data into the pipe */
static void writeToPipe(char *data) {
#ifdef GO_DEBUGLOG
	if (strcmp(data, "showboard")) /* this command is a bit spammy maybe */
		s_printf("GO_COMMAND: <%s>\n", data);//go_engine_next_action
//	printf("writeToPipe: %s\n", fw == NULL ? "NULL" : "ok");
//fw==NULL -> Send_store_special_str(Ind, 6, 3, TERM_ORANGE, "Arrr.. sorry, I can't find a fitting player around right now");
#endif
//	if (go_err(DOWN, NONE, "writeToPipe")) return;

	/* Before actually writing, clear all pending replies we can get! */
	if (go_engine_processing) go_engine_process();

	/* paranoia: catch overflows */
	if (go_engine_processing > 1000) {
		s_printf("GO_ENGINE: ERROR - over 1000 pending replies.\n");
		return;
	}

	/* more paranoia: invalid file/pipe? */
	if (!fw) {
		s_printf("GO_ENGINE: ERROR - fw is NULL.\n");
		return;
	}

	if (fprintf(fw, "%s\n", data) < 0) {
		s_printf("GO_ENGINE: ERROR in fprintf().\n");
		fclose(fw);
		fclose(fr);
		close(pipeto[1]);
		close(pipefrom[0]);
		go_engine_up = FALSE;
		return;
	}

	if (fflush(fw) != 0) {
		s_printf("GO_ENGINE: ERROR in fflush().\n");
		fclose(fw);
		fclose(fr);
		close(pipeto[1]);
		close(pipefrom[0]);
		go_engine_up = FALSE;
		return;
	}

	/* increase pending replies by one, ie the one to this command we now send */
	go_engine_processing++;
}

/* Read a line of data from the pipe if there is any, else skip */
static void readFromPipe(char *buf, int *cont) {
	int ch = 0;
	char c[2];

//	if (go_err(DOWN, NONE, "readFromPipe")) return;

	strcpy(buf, "");
	while (1) {
		ch = fgetc(fr);
		if (!ch) break;
		if (ch == EOF) {
//printf("feof: %d, ferror: %d\n", feof(fr), ferror(fr));
			(*cont) = 0;
			break;
		}

		sprintf(c, "%c", (char)ch);
		if (c[0] == '\n') (*cont)++;
		else (*cont) = 0;

		strcat(buf, c);
		if (c[0] == '\n' || strlen(buf) == 79) break;
	}
}

#ifdef ENGINE_FUEGO
/* Handle engine startup process.
   Blocking reading is ok, because server is just starting up. */
static int handle_loading() {
	char lbuf[80], reply[240];
	int cont = 0;
	strcpy(reply, "");
	strcpy(lbuf, "");

	do {
		if (lbuf[0] && lbuf[strlen(lbuf) - 1] == '\n') strcpy(reply, "");
		readFromPipe(lbuf, &cont);
		if (lbuf[0]) {
//			printf("<%s>\n", lbuf);
			if (strlen(reply) + strlen(lbuf) >= 240) {
				s_printf("GO_ERROR: LINE TOO LONG\n");
				return 1;
			}
			strcat(reply, lbuf);
			if (reply[0] && reply[strlen(reply) - 1] == '\n') {
				reply[strlen(reply) - 1] = 0;
#ifdef GO_DEBUGPRINT
				printf("<%s>\n", reply);
#endif
			}
		}
		/* wait till startup has been completed before proceeding */
		if (strstr(reply, "... ok") != NULL) cont = 3;
	} while (cont != 3);
	return 0;
}
#endif

/* Cleanup: Save game, reset board.
   Possibly ignore responses since they aren't needed, and just flush
   the replies out of the pipe when the next game is initialized.
   'server_shutdown' must be TRUE exactly if server is shut down,
   it'll just use blocking reading, for reading the replies. */
static void go_challenge_cleanup(bool server_shutdown) {
	int Ind;
#ifdef ENGINE_FUEGO
	char tmp[80];
#endif

	/* Cancel prompt for move */
	if ((Ind = lookup_player_ind(go_engine_player_id))) {
		Send_request_abort(Ind);
		Players[Ind]->request_id = RID_NONE;

		/* We're no longer busy */
		Players[Ind]->store_action = 0;
	}

	if (go_err(DOWN, DOWN, "go_challenge_cleanup")) return;

	/* If we have to stop prematurely, interprete it as loss for the player.
	   Note: The 'result' info is cleared by 'clear_board' below. */
	if (!game_over) {
#ifdef ENGINE_FUEGO
		if (CPU_has_white) writeToPipe("go_set_info result W+Forfeit");
		else writeToPipe("go_set_info result B+Forfeit");
		if (server_shutdown) wait_for_response();
#endif
#ifdef ENGINE_GNUGO
		if (sgf) {
			if (CPU_has_white) fprintf(sgf, "RE[W+Forfeit]\n");
			else fprintf(sgf, "RE[B+Forfeit]\n");
		}
#endif
		game_over = TRUE;
	}

	/* for saving game record to disk */
	tend = time(NULL);
	tmend = localtime(&tend);

#ifdef ENGINE_FUEGO
	/* Save it as sgf */
	sprintf(tmp, "savesgf go/TomeNET-%04d%02d%02d-%02d%02d%02d.sgf",
	    1900 + tmstart->tm_year, tmstart->tm_mon + 1, tmstart->tm_mday,
	    tmstart->tm_hour, tmstart->tm_min, tmstart->tm_sec);
	writeToPipe(tmp); //(saves current game (and tree if some global flag was set) to sgf)
	if (server_shutdown) wait_for_response();
#endif
#ifdef ENGINE_GNUGO
	/* Close SGF file */
	if (sgf) {
		fprintf(sgf, ")\n");
		fclose(sgf);
	}
#endif

	/* Prepare for next game in advance - skip on server shutdown */
	if (!server_shutdown) writeToPipe("clear_board");

	/* Clean up everything */
	go_game_up = FALSE;
	go_engine_next_action = NACT_NONE;
//	go_engine_processing = 0;
}

/* Screen operations only: Update the board visuals for the player */
static void go_engine_board(void) {
	int n, x;
	char nline[30];
	int Ind;
	player_type *p_ptr;

	if (go_err(DOWN, DOWN, "go_engine_board")) return;

	/* Player still with us? */
	if (!(Ind = lookup_player_ind(go_engine_player_id))) {
		go_challenge_cleanup(FALSE);
		return;
	}
	p_ptr = Players[Ind];

	for (n = 0; n < 9; n++) {
		/* line hasn't been changed? */
		if (!strcmp(board_line_old[n], board_line[n])) continue;
		/* line has been changed: Update visuals */
		strcpy(nline, "");
		for (x = 0; x < 9; x++) {
			if (board_line[n][x] == 'X') strcat(nline, "\377Do"); //black
			else if (board_line[n][x] == 'O') strcat(nline, "\377wo"); //white
			else strcat(nline, "\377u."); //free
		}

		/* send it */
		Send_store_special_str(Ind, GO_BOARD_Y + 9 - n, GO_BOARD_X + 1, TERM_UMBER, nline);
		strcpy(board_line_old[n], board_line[n]);
	}
	Net_output1(Ind);
}

static bool go_err(int engine_status, int game_status, char *name) {
	if (engine_status == DOWN) {
		if (!go_engine_up) {
			s_printf("GO_ERR: go engine down in %s().\n", name);
			return TRUE;
		}
	} else if (engine_status == UP) {
		if (go_engine_up) {
			s_printf("GO_ERR: go engine up in %s().\n", name);
			return TRUE;
		}
	}
	if (game_status == DOWN) {
		if (!go_game_up) {
			s_printf("GO_ERR: go game down in %s().\n", name);
			return TRUE;
		}
	} else if (game_status == UP) {
		if (go_game_up) {
			s_printf("GO_ERR: go game up in %s().\n", name);
			return TRUE;
		}
	}
	return FALSE;
}

/* Silently switch Go engine for special anti-Mirror-Go engine */
static void enable_anti_mirror(void) {
	s_printf("GO_MIRROR: Attempting to start anti-Mirror-Go engine...");


	s_printf("failure.\n");
	return;


	//set its timelimit to GO_TIME_PY - 2, ie max viably possible.


	anti_mirror_active = TRUE;
	s_printf("success.\n");
	return;
}

#endif /* ENABLE_GO_GAME */
