/* $Id$ */
#include "softbot.h"

#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sched.h>

union sock
{
	struct sockaddr s;
	struct sockaddr_in i;
};

#define SMAX			15

struct scorebd
{
	time_t			tevent;
	thread_t		thread;
	int				sig;
} scorebd[SMAX];

mutex_t		sb_lock;
pthread_t	thr_master, thr_timing;
int			slave_thread_num;

void	*handling(void *);		/* The processing thread code. The prototype is 
								   as required by the thr_create library function.*/
void	handto();				/* The signal handler for the processing thread. */
void	*timing(void *);		/* The timing thread code. */
void	timer();				/* The signal handler for the timing thread. */
void	master();				/* The signal handler for the master thread. */

void	reqto(int);				/* Used by handling threads to request a time out. */
void	sigtype();				/* Used by handling threads to determine why they 
								   have been signalled. */
void	thr_reg();				/* Used by a handling thread to create an entry for
								   itself in the scoreboard. */
void	thr_unreg();			/* Used by a handling thread to remove its entry from
								   the scoreboard. */

int module_main(int argc, char *argv[]) 
{
	int port;
	union sock sock, work;
	int wsd, sd;
	int addlen;

	struct sigaction siginfo;

	int i, rv, nrv;

	sigemptyset(&(siginfo.sa_mask));
	sigaddset(&(siginfo.sa_mask), SIGHUP);
	sigaddset(&(siginfo.sa.mask), SIGALRM);
	sigprocmask(SIG_SETMASK, &(siginfo.sa_mask), NULL);

	if (pthread_create(thr_timing, NULL, timing, NULL) == -1) {
		error("cannot create timing thread");
		return FAIL;
	}

	thr_master = pthread_self();

	sigemptyset(&(siginfo.sa_mask));
	sigaddset(&(siginfo.sa_mask), SIGUSR1);
	pthread_sigmask(SIG_UNBLOCK, &(siginfo.sa_mask), NULL);
	siginfo.sa_handler = master;
	siginfo.sa_flags = 0;
	sigaction(SIGUSR1, &siginfo, NULL);

	// FIXME -- complement next function
	sock = bind_and_listen();

	for (i=0; i<slave_thread_num; i++)
	{
		if (pthread_create(NULL, NULL, handling, NULL) == -1) 
		{
			error("cannot create slave thread");
		}
	}
}

void *timing(void *arg) 
{
	struct sigaction siginfo;

	sigemptyset(&(siginfo.sa_mask));
	sigaddset(&(siginfo.sa_mask), SIGALRM);
	sigaddset(&(siginfo.sa_mask), SIGHUP);
	siginfo.sa_handler = timer;
	siginfo.sa_flags = 0;
	pthread_sigmask(SIG_UNBLOCK, &(siginfo.sa_mask), NULL);
	sigaction(SIGALRM, &siginfo, NULL);
	sigaction(SIGHUP, &siginfo, NULL);

	alarm(1);
	do
	{
		pause();
		sched_yield();
	} while(1);
}

void timer(int sig)
{
	int i,n;
	time_t now;
	time(&now);
	mutex_lock(&sb_lock);

	for (i=0; i<SMAX; i++)
	{
		if (scorebd[i].thread)
		{
			if (sig == SIGHUP)
			{
				scorebd[i].sig = SIGHUP;
				pthread_kill(scorebd[i].thread, SIGUSR1);
			}
			else if (sig == SIGALRM) 
			{
				if (scorebd[i].tevent < now)
				{
					scorebd[i].sig = SIGALRM;
					pthread_kill(scorebd[i].thread, SIGUSR1);
				}
			}
		}
	}

	if (sig == SIGHUP) pthread_kill(thr_master, SIGUSR1);

	mytex_unlock(&sb_lock);
	alarm(1);
	return;
}

void *handling(void *arg)
{
	int nrv, wsd;
	int i;
	char buff[BUFSIZ];

	do {
		// FIXME -- complement next function
		wsd = accept();
	} while(1);
}
