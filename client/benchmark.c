/* search performance test tool */
/* by jso 2004/02/04 */

#include "benchmark.h"
#include "commands.h"
#include <pthread.h>

#define SLOT_SIZE 20
#define SLOT_TIME_INTERVAL 100
#define SLOT_TIME_BASE 20

typedef struct {
	pthread_mutex_t	*fp_lock;
	FILE *fp;		                        /* fp of query list file */
	pthread_mutex_t	*count_lock;
	int volatile	success;				/* number of requests successed */
	int volatile	fail;					/* number of requests failed */
	int volatile	read;					/* number of bytes read */
	int volatile	time;					/* total sum of time in ms for connections */
	int volatile	min_time;				/* minimum time in ms for connection */
	int volatile	max_time;				/* maximum time in ms for connection */
	int volatile	ended;					/* check if test has ended */
	int volatile	time_dist[SLOT_SIZE];	/* table of time distribution */
	int volatile	min_count;
	int volatile	max_count;

	long long volatile	total_count;
	struct timeval start;					/* start time */
	struct timeval end;						/* end time */

	int requests;							/* total number of requests */
	int concurrency;
} status_t;

int  verbose = 0;
status_t status;

static int timedif(struct timeval a, struct timeval b);
void softbot_bench(int *i);

static int timedif(struct timeval a, struct timeval b)
{
	register int us, s;
	us = a.tv_usec - b.tv_usec;
	us /= 1000;
	s = a.tv_sec - b.tv_sec;
	s *= 1000;

	return s + us;
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
