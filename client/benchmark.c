/* $Id$ */
#include "common_core.h"
//#include "scoreboard.h"
//#include "modules.h"
#include "ipc.h"
#include "commands.h"  /* search() */
#include "benchmark.h"
#include <string.h>
#include <pthread.h>
#include <sys/time.h>  /* gettimeofday(2) */
#include <unistd.h>    /* getopt(3) */
#include <stdlib.h>    /* atoi(3) */
#include <signal.h>

#define MAX_THREADS (100)
static scoreboard_t scoreboard[] = { THREAD_SCOREBOARD(MAX_THREADS) };

extern module benchmark_module;
/****************************************************************************/
static void _do_nothing(int sig)
{
    return;
}

static void _shutdown(int sig)
{
    pthread_exit(NULL);
}

static void _graceful_shutdown(int sig)
{
    struct sigaction act;

    memset(&act, 0x00, sizeof(act));

    sigfillset(&act.sa_mask);

    act.sa_handler = _do_nothing;
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    scoreboard->graceful_shutdown++;
}
/****************************************************************************/

/*
 prepare_test
   init_scoreboard
   get_step_lock
   make threads/processes to do_test
   release_step_lock

   wait for threads/processes to return

   print statistics

 do_test
   wait_for_start_sign

   while ( 1 )
   {
     get input data;
     break if no more input data;
     break if count >= max_count or no more input data;

     time the test

     break if count >= max_count or no more input data;

	 update_test_result
   } 

*/

typedef struct {
	char *str;
} bm_input_t;

#define SLOT_SIZE          (20)
#define SLOT_TIME_BASE     (20)
#define SLOT_TIME_INTERVAL (100)
typedef struct {
	int base;
	int interval;
	int volatile slots[SLOT_SIZE];
} bm_distribution_t;


struct bm_scoreboard_t {
	rwlock_t *lock;

	struct timeval	start_time;				/* start time */
	struct timeval	end_time;				/* end time */

	int volatile	success;				/* number of requests succeeded */
	int volatile	failure;				/* number of requests failed */

	int volatile	read;					/* number of bytes read */

	int volatile	total_time;				/* total sum of time in ms */
	int volatile	min_time;				/* minimum time in ms */
	int volatile	max_time;				/* maximum time in ms */
	int volatile	avg_time;				/* avg time in ms */
	bm_distribution_t *distrib;

	int volatile	ended;					/* check if test has ended */
};
/****************************************************************************/
static bm_scenario_t *static_scn;

static int rdlock(void)
{
	return rwlock_rdlock(static_scn->scoreboard->lock);
}
static int wrlock(void)
{
	return rwlock_wrlock(static_scn->scoreboard->lock);
}
static int unlock(void)
{
	return rwlock_unlock(static_scn->scoreboard->lock);
}

static int timeslot(int timetaken)
{
	int i;

	if ( timetaken < SLOT_TIME_BASE )
		i = 0;
	else {
		i = timetaken/SLOT_TIME_INTERVAL + 1;
		i = (i > SLOT_SIZE-1) ? SLOT_SIZE-1: i;
	}
	return i;
}

int benchmark_init(bm_scenario_t *scn)
{
	memset(scn, 0x00, sizeof(bm_scenario_t));

	scn->scoreboard = calloc(1, sizeof(bm_scoreboard_t));
	scn->scoreboard->distrib = calloc(1, sizeof(bm_distribution_t));

	scn->scoreboard->lock = calloc(1, rwlock_sizeof());
	rwlock_init(scn->scoreboard->lock);

	static_scn = scn;

	init_one_scoreboard(&benchmark_module);

	return SUCCESS;
}

int child_main(slot_t *slot)
{
	bm_scenario_t *scn = static_scn;
	struct timeval start, end;

	while ( 1 ) {
		char *in;

		wrlock();
		if (scn->scoreboard->ended)
		{
			unlock();
			break;
		}

		in = scn->read_input();

		unlock();

		if (in == NULL) break;

		scn->do_test(in);
	}

	return SUCCESS;
}

/* display usage information */
static void usage(char *progname)
{
	info("Usage: %s [options] query_list_file", progname);
	info("Options are:");
	info("    -n requests    Number of requests to perform");
	info("    -c concurrency Number of multiple requests to make");
	info("    -v             Verbose output");
}


int benchmark_run(bm_scenario_t *scn)
{
	int needed_threads = 10;

	if (static_scn == NULL) {
		error("you should call benchmark_init() first.");
		return FAIL;
	}
	sb_run_set_default_sighandlers(_shutdown,_graceful_shutdown);

	scoreboard->size = (needed_threads < MAX_THREADS) ? needed_threads : MAX_THREADS;
	if (sb_run_init_scoreboard(scoreboard) != SUCCESS) return FAIL;

    scoreboard->period = 3;
	wrlock();
    sb_run_spawn_processes(scoreboard, "benchmark", child_main);

    sleep(1);
    rwlock_unlock(scn->scoreboard->lock);

    sb_run_monitor_processes(scoreboard);

	return SUCCESS;
}

module benchmark_module = {
    STANDARD_MODULE_STUFF,
    NULL,               /* config */
    NULL,               /* registry */
    NULL,               /* initialize */
    NULL,               /* child_main */
    scoreboard,         /* scoreboard */
    NULL,               /* register hook api */
};


#if 0

void softbot_bench(int *num)
{
	char  buf[STRING_SIZE];
	struct timeval start, end;
	int   timetaken; /* time difference in ms */
	int	  r=0, n;
	char  *s;
	int   i = *num;
	int   total_cnt;
    
	while ( 1 ) {
		pthread_mutex_lock(status.fp_lock);
		s = fgets(buf, STRING_SIZE, status.fp);
		pthread_mutex_unlock(status.fp_lock);
		if ( s == NULL ) {
			return;
		}
		/* remove \n from end of string */
		n = strlen(buf);
		if ( buf[n-1] == '\n' ) buf[n-1] = '\0';

		gettimeofday(&start, 0);
        search(buf, 1, &total_cnt);
        
		gettimeofday(&end, 0);
		timetaken = timedif(end, start);

		pthread_mutex_lock(status.count_lock);
		if ( status.success >= status.requests ) {
			if ( status.ended == 0 ) {
				gettimeofday(&status.end, 0);
				status.ended = 1;
				info("[%d] test ended. joining threads...\n", i);
			}
			pthread_mutex_unlock(status.count_lock);
			return;
		}

		if ( r != 0 ) {
			status.fail++;
			if ( verbose )
				info("F (%d) %03dms : %s\n", r, timetaken, buf);
		} else {
			if ( status.success % 100 == 0 ) {
				info(".");
				fflush(stdout);
			}
			status.success++;
			status.time += timetaken;
			status.min_time = timetaken < status.min_time ?
						timetaken : status.min_time;
			status.max_time = timetaken > status.max_time ?
						timetaken : status.max_time;
			status.time_dist[timeslot(timetaken)]++;
			status.max_count = status.max_count > total_cnt ?
				   		status.max_count : total_cnt;	
			status.min_count = status.min_count < total_cnt ?
				   		status.min_count : total_cnt;	
			status.total_count += total_cnt;
			if ( timetaken < SLOT_TIME_BASE && verbose )
				info("S %03dms : %s\n", timetaken, buf);

		}
		pthread_mutex_unlock(status.count_lock);
    }
}

/* display usage information */
static void usage(char *progname)
{
	info("Usage: %s [options] query_list_file", progname);
	info("Options are:");
	info("    -n requests    Number of requests to perform");
	info("    -c concurrency Number of multiple requests to make");
	info("    -v             Verbose output");
}


extern char *optarg;
extern int optind, opterr, optopt;

int
benchmark(int argc, char *argv[])
{
	int c, i, j, max_slot_count;
	int *tindex;
	pthread_t *threads;
	pthread_mutex_t fp_lock;
	pthread_mutex_t count_lock;
	int  timetaken;
	FILE *fp;

	/* default status */
	status.time = 0;
	status.min_time = 999999;
	status.max_time = 0;
	status.concurrency = 1;
	status.requests = 1;
	status.success = 0;
	status.fail = 0;
	status.ended = 0;
	status.max_count = 0;
	status.min_count = 10000000;
	status.total_count = 0;
	
	for(i = 0; i < SLOT_SIZE; i++) status.time_dist[i] = 0;

	optind = 1;

	while ((c = getopt(argc,argv, "vn:c:")) > 0 ) {
		switch (c) {
		case 'n':
			status.requests = atoi(optarg);
			if (!status.requests) {
				error("Invalid number of requests\n");
				return 0;
			}
			break;
		case 'c':
			status.concurrency = atoi(optarg);
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			error("%s: invalid option '%c'\n", argv[0], c);
			usage(argv[0]);
			return 0;
		}
	}

	if (optind != argc - 1) {
		error("%s: wrong number of arguments, %d\n", argv[0], optind);
		usage(argv[0]);
		return 0;
	}

	/* set values of struct status */
	threads = (pthread_t *) calloc(status.concurrency, sizeof(pthread_t));
	tindex = (int *) calloc(status.concurrency, sizeof(int));

	fp = fopen(argv[optind++], "rt");
	if ( fp == NULL ) {
		error("cannot open query list file");
		return 0;
	}
	status.fp = fp;

	if ( verbose )
		info("verbose mode on\n");

	info("requests = %d\n", status.requests);
	info("concurrency = %d\n", status.concurrency);

	/* mutex initialization */
	pthread_mutex_init(&fp_lock, NULL);
	pthread_mutex_init(&count_lock, NULL);
	//pthread_mutex_init(&host_lock, NULL);
	status.fp_lock = &fp_lock;
	status.count_lock = &count_lock;

	/* make threads */
	pthread_mutex_lock(status.fp_lock);
	for(i = 0; i < status.concurrency; i++)
	{
		tindex[i] = i;
		if (pthread_create(&threads[i], NULL, (void *)softbot_bench, &tindex[i])) {
			error("%s: cannot make thread\n", argv[0]);
			return 0;
		}
	}
	gettimeofday(&status.start, 0);
	/* release fp_lock and threads starts the test. */
	pthread_mutex_unlock(status.fp_lock);
	
	/* join threads */
	for(i = 0; i < status.concurrency; i++)
	{
		if (pthread_join(threads[i], NULL)) {
			error("%s: cannot join thread\n", argv[0]);
			return 0;
		}
	}

	/* print statistics */
	timetaken = timedif(status.end, status.start);
	printf("time elapsed %d ms\n", timetaken);
	printf("success %d, fail %d\n", status.success, status.fail);
	printf("average successed %4.2f requests/sec \n", (float)status.success/timetaken*1000);
	printf("average total %4.2f requests/sec \n", (float)(status.success+status.fail)/timetaken*1000);
	printf("min time %dms, avg time %dms, max time %dms\n",
					status.min_time, status.time/status.success, status.max_time);
	
	printf("max count %d, min count %d, avg count %lld\n", status.max_count, status.min_count, status.total_count/status.success);
	
	for(i = 0, max_slot_count = 0; i < SLOT_SIZE; i++)
		max_slot_count = (max_slot_count < status.time_dist[i]) ?
							status.time_dist[i] : max_slot_count;

	/* statistic for SLOT_BASE_TIME */
	printf("%4dms ", SLOT_TIME_BASE);
	printf("%4d ", status.time_dist[0]);
	printf("%2d%%", (int)((float)status.time_dist[0]/status.requests*100));
	for(j = 0; j < (int)((float)status.time_dist[0]/max_slot_count*60); j++)
			printf(".");
	printf("\n");
	for(i = 1; i < SLOT_SIZE; i++) {
		printf("%4dms ", i * SLOT_TIME_INTERVAL);
		printf("%4d ", status.time_dist[i]);
		printf("%2d%%", (int)((float)status.time_dist[i]/status.requests*100));
		for(j = 0; j < (int)((float)status.time_dist[i]/max_slot_count*60); j++)
			printf(".");
		printf("\n");
	}

	fclose(fp);
	return 0;
}

#endif

