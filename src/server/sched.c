/*
 * sched.c
 * Scheduling code for UNIX-like systems
 */

#include "angband.h"
#include <signal.h>
#include <sys/time.h>

/*
 * Block or unblock a single signal.
 */
static void sig_ok(int signum, int flag) {
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
void block_timer(void) {
	sig_ok(SIGALRM, 0);
}

/*
 * Unblock the real-time timer.
 * Globally accessible.
 */
void allow_timer(void) {
	sig_ok(SIGALRM, 1);
}

static volatile long	timer_ticks;	/* SIGALRMs that have occurred */
static long		timer_freq;	/* rate at which timer ticks. */
static void		(*timer_handler)(void);

/*
 * Catch SIGALRM.
 */
static void catch_timer(int signum) {
	timer_ticks++;
}


/*
 * Setup the handling of the SIGALRM signal
 * and setup the real-time interval timer.
 */
void setup_timer(void) {
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

	/*
	 * Allow the real-time timer to generate SIGALRM signals.
	 */
	allow_timer();
}


/*
 * Configure timer tick callback.
 */
void install_timer_tick(void (*func)(void), int freq) {
	timer_handler = func;
	timer_freq = freq;
	setup_timer();
}


struct io_handler {
    void		(*func)(int, int);
    int			arg;
};

/* donald sharp - I have modified this file such that it will */
/* allow more than 32 file descriptors at once.  This is a good */
/* thing and will allow future modifications to mangband */

static struct io_handler *input_handlers = NULL;
static int		biggest_fd = -1;
static fd_set		input_mask;
static int		input_mask_cleared = FALSE;
static int		max_fd;

/*
 * Clear the input mask when starting up
 */
static void clear_mask(void) {
	FD_ZERO(&input_mask);
}

void install_input(void (*func)(int, int), int fd, int arg) {
	if (!input_mask_cleared) {
		clear_mask();
		input_mask_cleared = TRUE;
	}

	/* Sanity check */
	if (fd < 0 ) {
		plog(format("install illegal input handler fd %d", fd));
		exit(1);
	}

	/* Another sanity check */
	if (FD_ISSET(fd, &input_mask)) {
		plog(format("input handler %d busy", fd));
		exit(1);
	}

	if (fd > biggest_fd) {
		input_handlers = realloc(input_handlers, sizeof(struct io_handler) * (fd + 1));
		biggest_fd = fd;
		if (input_handlers == NULL) {
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

void remove_input(int fd) {
	if (fd < 0) {
		plog(format("remove illegal input handler fd %d", fd));
		exit(1);
	}

	if (FD_ISSET(fd, &input_mask)) {
		input_handlers[fd].func = 0;
		FD_CLR(fd, &input_mask);
		if (fd == (max_fd - 1)) {
			int i;
			max_fd = 0;
			for (i = fd; --i >= 0; ) {
				if (FD_ISSET(i, &input_mask)) {
					max_fd = i + 1;
					break;
				}
			}
		}
	}
}

/*
 * I/O + timer dispatcher.
 */
void sched(void) {
	long timers_used = timer_ticks;	/* SIGALRMs that have been used */
	fd_set readmask;
	int n;

	while (1) {
		if (timers_used < timer_ticks) {
			if (timer_handler) {
				(*timer_handler)();
			}

			timers_used = timer_ticks;
		}

		readmask = input_mask;

		n = select(max_fd, &readmask, NULL, NULL, NULL);
		if (n < 0) {
			int errval = errno;
			if (errval != EINTR) {
				save_game_panic();
				fprintf(stderr, "sched select failed, errno = %d\n", errval);
				/* plog("sched select error"); */
				core("sched select error");
				exit(1);
			}
		} else if (n == 0) {
			/* Do nothing */
		} else {
			int i;
			for (i = max_fd; i >= 0; i--) {
				if (FD_ISSET(i, &readmask)) {
					(*input_handlers[i].func)(i, input_handlers[i].arg);
					if (--n == 0) {
						break;
					}
				}
			}
		}
	}
}
