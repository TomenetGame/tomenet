/*
* File: sched-win.c
* Purpose: Windows port of sched.c
*/


#include <winsock2.h>
#include "angband.h"
#include <mmsystem.h>
#include <winbase.h>


static volatile long timer_ticks;
static long timer_freq; /* frequency (Hz) at which timer ticks. */
static void (*timer_handler)(void);
static DWORD resolution;
static MMRESULT timer_id = 0;


/*
* Our timer callback
*/
void CALLBACK timer_callback(UINT wTimerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2) {
	timer_ticks++;
}


/*
* Stop the high resolution timer and tidy up
*/
void stop_timer(void) {
	timeKillEvent(timer_id);
	timer_id = 0;
	timeEndPeriod(resolution);
}


/*
* Start a high resolution timer
*/
void setup_timer(void) {
	DWORD delay;
	TIMECAPS tc;

	/* If we are already running a timer, stop it */
	if (timer_id != 0) stop_timer();

	/* Query the high resolution timer */
	timeGetDevCaps(&tc, sizeof(TIMECAPS));
	resolution = min(max(tc.wPeriodMin, 1), tc.wPeriodMax);

	/* We want the fastest timer possible idealy.  Most systems will support this, but we'll
	* try to cater for those systems which only have low resolution timers...
	*/
	delay = 1000 / timer_freq;
	delay = delay / resolution;
	plog_fmt("Timer delay %i ms timer resolution is %i ms", delay, resolution);
	if (!timeBeginPeriod(resolution) == TIMERR_NOERROR)
	{
		plog("Could not set the timer to the required resolution");
		exit(1);
	}

	/* Start the timer thread */
	if ((timer_id = timeSetEvent(delay, resolution, timer_callback, (DWORD)0, TIME_PERIODIC)) == 0)
	{
		plog("Could not start timer thread");
		exit(1);
	}
}


/*
* Configure timer tick callback.  freq is FPS from the .cfg file
*/
void install_timer_tick(void (*func)(void), int freq)
{
	timer_handler = func;
	timer_freq = freq;
	setup_timer();
}


struct io_handler
{
    void (*func)(int, int);
    int arg;
};

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
	struct timeval tv;
	int n;

	while (1) {
		if (timers_used < timer_ticks) {
			if (timer_handler) {
				(*timer_handler)();
			}

			timers_used = timer_ticks;
		}

		readmask = input_mask;
		tv.tv_sec = 0;
		tv.tv_usec = 333;

		n = select(max_fd, &readmask, NULL, NULL, &tv);
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
