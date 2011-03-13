/* $Id$
 *
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-95 by
 *
 *      Bjørn Stabell        (bjoerns@staff.cs.uit.no)
 *      Ken Ronny Schouten   (kenrsc@stud.cs.uit.no)
 *      Bert Gÿsbers         (bert@mc.bio.uva.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define SERVER

#include "angband.h"
#include <signal.h>
#ifdef DUMB_WIN
#include <sys/time.h>
#endif

/*char sched_version[] = VERSION;*/

#ifdef sony_news
/*
 * Sony NEWS doesn't have the sigset family.
 */
typedef unsigned int    sigset_t;

#define sigemptyset(set)        (*(set) = 0)
#define sigfillset(set)         (*(set) = ~(sigset_t)0, 0)
#define sigaddset(set,signo)    (*(set) |= sigmask(signo), 0)
#define sigdelset(set,signo)    (*(set) &= ~sigmask(signo), 0)
#define sigismember(set,signo)  ((*(set) & sigmask(signo)) != 0)

#define SIG_BLOCK		1
#define SIG_UNBLOCK		2
#define SIG_SETMASK		3

int sigprocmask(int how, const sigset_t *set, sigset_t *oset)
{
    int			mask;

    if (how == SIG_BLOCK) {
	mask = sigblock(0) | *set;
    }
    else if (how == SIG_UNBLOCK) {
	mask = sigblock(0) & ~(*set);
    }
    else if (how == SIG_SETMASK) {
	mask = *set;
    }
    else {
	mask = sigblock(0);
    }
    mask = sigsetmask(mask);
    if (oset != NULL) {
	*oset = mask;
    }

    return 0;
}

/*
 * Sony NEWS doesn't have sigaction(), using sigvec() instead.
 */
#define sigaction	sigvec
#define sa_handler	sv_handler
#define sa_mask		sv_mask
#define sa_flags	sv_flags
#endif


/*
 * Block or unblock a single signal.
 */
static void sig_ok(int signum, int flag)
{
    sigset_t    sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, signum);
    if (sigprocmask((flag) ? SIG_UNBLOCK : SIG_BLOCK, &sigset, NULL) == -1) {
	plog(format("sigprocmask(%d,%d)", signum, flag));
	exit(1);
    }
}

/*
 * Prevent the real-time timer from interrupting system calls.
 * Globally accessible.
 */
void block_timer(void)
{
    sig_ok(SIGALRM, 0);
}

/*
 * Unblock the real-time timer.
 * Globally accessible.
 */
void allow_timer(void)
{
    sig_ok(SIGALRM, 1);
}

static volatile long	timer_ticks;	/* SIGALRMs that have occurred */
static long		timers_used;	/* SIGALRMs that have been used */
static long		timer_freq;	/* rate at which timer ticks. */
static void		(*timer_handler)(void);
static time_t		current_time;
static int		ticks_till_second;
static int		sched_running;

/*
 * Catch SIGALRM.
 */
static void catch_timer(int signum)
{
    timer_ticks++;
}


/*
 * Setup the handling of the SIGALRM signal
 * and setup the real-time interval timer.
 */
void setup_timer(void)
{
	struct itimerval itv;
	struct sigaction act;

	/*
	 * Prevent SIGALRMs from disturbing the initialization.
	 */
	block_timer();

	/*
	 * Install a signal handler for the alarm signal.
	 */
	act.sa_handler = catch_timer;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGALRM);
	if (sigaction(SIGALRM, &act, (struct sigaction *)NULL) == -1) {
		plog("sigaction SIGALRM");
		exit(1);
	}

	/*
	 * Install a real-time timer.
	 */
	if (timer_freq <= 0) {
		plog(format("illegal timer frequency: %ld", timer_freq));
		exit(1);
	}
	itv.it_interval.tv_sec = 0;
	itv.it_interval.tv_usec = 1000000 / timer_freq;
	itv.it_value = itv.it_interval;
	if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {
		plog("setitimer");
		exit(1);
	}

	timers_used = timer_ticks;
	time(&current_time);
	ticks_till_second = timer_freq;

	/*
	 * Allow the real-time timer to generate SIGALRM signals.
	 */
	allow_timer();
}


/*
 * Configure timer tick callback.
 */
void install_timer_tick(void (*func)(void), int freq)
{
    timer_handler = func;
    timer_freq = freq;
    setup_timer();
}

struct to_handler {
    struct to_handler	*next;
    time_t		when;
    void		(*func)(void *);
    void		*arg;
};
static struct to_handler *to_busy_list = NULL;
static struct to_handler *to_free_list = NULL;
//static int		to_min_free = 3; /* not used? - mikaelh */
static int		to_max_free = 5;
static int		to_cur_free = 0;

#if 0 /* to_alloc is only used by install_timeout() that's also #if 0'd - mikaelh */
static void to_fill(void)
{
    if (to_cur_free < to_min_free) {
	do {
	    struct to_handler *top =
		(struct to_handler *)malloc(sizeof(struct to_handler));
	    if (!top) {
		break;
	    }
	    top->next = to_free_list;
	    to_free_list = top;
	    to_cur_free++;
	} while (to_cur_free < to_max_free);
    }
}

static struct to_handler *to_alloc(void)
{
    struct to_handler *top;

    to_fill();
    if (!to_free_list) {
	plog("Not enough memory for timeouts");
	exit(1);
    }

    top = to_free_list;
    to_free_list = top->next;
    to_cur_free--;
    top->next = 0;

    return top;
}
#endif

static void to_free(struct to_handler *top)
{
    if (to_cur_free < to_max_free) {
	top->next = to_free_list;
	to_free_list = top;
	to_cur_free++;
    }
    else {
	free(top);
    }
}

#if 0 /* CURRENTLY not used */
/*
 * Configure timout callback.
 */
void install_timeout(void (*func)(void *), int offset, void *arg)
{
    struct to_handler *top = to_alloc();
    top->func = func;
    top->when = current_time + offset;
    top->arg = arg;
    if (!to_busy_list || to_busy_list->when >= top->when) {
	top->next = NULL;
	to_busy_list = top;
    }
    else {
	struct to_handler *prev = to_busy_list;
	struct to_handler *lp = prev->next;
	while (lp && lp->when < top->when) {
	    prev = lp;
	    lp = lp->next;
	}
	top->next = lp;
	prev->next = top;
    }
}

void remove_timeout(void (*func)(void *), void *arg)
{
    struct to_handler *prev = 0;
    struct to_handler *lp = to_busy_list;
    while (lp) {
	if (lp->func == func && lp->arg == arg) {
	    struct to_handler *top = lp;
	    lp = lp->next;
	    if (prev) {
		prev->next = lp;
	    } else {
		to_busy_list = lp;
	    }
	    to_free(top);
	}
	else {
	    prev = lp;
	    lp = lp->next;
	}
    }
}
#endif

static void timeout_chime(void)
{
    while (to_busy_list && to_busy_list->when <= current_time) {
	struct to_handler *top = to_busy_list;
	void (*func)(void *) = top->func;
	void *arg = top->arg;
	to_busy_list = top->next;
	to_free(top);
	(*func)(arg);
    }
}

#define NUM_SELECT_FD		(sizeof(int) * 8)

struct io_handler {
    void		(*func)(int, int);
    int			arg;
};

/* donald sharp - I have modified this file such that it will */
/* allow more than 32 file descriptors at once.  This is a good */
/* thing and will allow future modifications to mangband */
  
static struct io_handler *input_handlers = NULL;
static int              biggest_fd = -1;
static fd_set		input_mask;
static int              input_mask_cleared = FALSE;
static int		max_fd;

/* clear_mask */
/* We need to explicitly clear out the input_mask for the select call*/
static void clear_mask( void )
{
    FD_ZERO(&input_mask);
}

void install_input(void (*func)(int, int), int fd, int arg)
{
    if ( input_mask_cleared == FALSE ) {
       clear_mask();
       input_mask_cleared = TRUE;
    }
    if (fd < 0 ) {
	plog(format("install illegal input handler fd %d", fd));
	exit(1);
    }
    if (FD_ISSET(fd,&input_mask)) {
	plog(format("input handler %d busy", fd));
	exit(1);
    }
    if(input_handlers == NULL || fd > biggest_fd)
    {
        if( biggest_fd < fd )
        {
           input_handlers = realloc( input_handlers, 
                                     sizeof(struct io_handler) * (fd + 1) );
           biggest_fd = fd;
        }
        if( input_handlers == NULL )
        {
           plog(format("input handler %d realloc failed", fd));
           exit(1);
        }
    }
    input_handlers[fd].func = func;
    input_handlers[fd].arg = arg;
    FD_SET(fd, &input_mask);
    if (fd >= max_fd) {
	max_fd = fd + 1;
    }
}

void remove_input(int fd)
{
    if ( fd < 0 ) {
	plog(format("remove illegal input handler fd %d", fd));
	exit(1);
    }
    if (FD_ISSET( fd, &input_mask )) {
	input_handlers[fd].func = 0;
        FD_CLR(fd, &input_mask);
	if (fd == (max_fd - 1)) {
	    int i;
	    max_fd = 0;
	    for (i = fd; --i >= 0; ) {
                if ( FD_ISSET( i, &input_mask ) ) {
		    max_fd = i + 1;
		    break;
		}
	    }
	}
    }
}

// static int		sched_running;

#if 0 /* CURRENTLY unused */
void stop_sched(void)
{
    sched_running = 0;
}
#endif

/*
 * I/O + timer dispatcher.
 */
void sched(void)
{
    int			io_done = 0, io_todo = 3;
    struct timeval	tv, *tvp = &tv;
    fd_set              readmask;

    readmask = input_mask;
#ifdef VMS
    extern int NumPlayers, NumRobots, NumPseudoPlayers, NumQueuedPlayers, NumNonAdminPlayers;
    extern int login_in_progress;
#endif

    if (sched_running) {
	plog("sched already running");
	exit(1);
    }
    sched_running = 1;

    while (sched_running) {

#ifdef VMS
        if (NumPlayers > NumRobots + NumPseudoPlayers
            || login_in_progress != 0
            || NumQueuedPlayers > 0) {

            /* need fast I/O checks now! (2 or 3 times per frames) */
            tv.tv_sec = 0;
            tv.tv_usec = 1000000 / (3 * timer_freq + 1); 
        }
        else {
            /* slow I/O checks are possible here... (2 times per second) */ ; 
            tv.tv_sec = 0;
            tv.tv_usec = 500000;
        }
#else
        tv.tv_sec = 0;
        tv.tv_usec = 0;
#endif

	if (io_todo == 0 && timers_used < timer_ticks) {
	    io_done = 0;
	    io_todo = 3;
	    tvp = &tv;

	    if (timer_handler) {
		(*timer_handler)();
	    }

	    do {
		++timers_used;
		if (--ticks_till_second <= 0) {
		    ticks_till_second += timer_freq;
		    current_time++;
		    timeout_chime();
		}
	    } while (timers_used + 1 < timer_ticks);

	}
	else {
            int n;

/*
 * KLJ -- Prevent crashes caused by "input_mask" changing during
 * the "timeout_chime" function call (which happens when a player 
 * dies).
 */
	    readmask = input_mask;

	    n = select(max_fd, &readmask, 0, 0, tvp);
	    if (n < 0) {
		if (errno != EINTR) {
		    save_game_panic();
                    printf("Errno: %d\n",errno);
		    /* plog("sched select error"); */
		    core("sched select error");
		    exit(1);
		}
		io_todo = 0;
	    }
	    else if (n == 0) {
                readmask = input_mask;
		io_todo = 0;
	    }
	    else {
		int i;
		for (i = max_fd; i >= 0; i--) {
                    if (FD_ISSET(i,&readmask))  {
			(*input_handlers[i].func)(i, input_handlers[i].arg);
/* evileye *//*
                        readmask = input_mask; 
*//* oh ;( */
			if (--n == 0) {
			    break;
			}
		    }
		}
		io_done++;
		if (io_todo > 0) {
		    io_todo--;
		}
	    }
#ifndef VMS
	    if (io_todo == 0) {
		tvp = NULL;
	    }
#endif
	}
    }
}

