/* search performance test tool */
/* by aragorn 2001/03/10 */

#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>	// gettimeofday
#include <unistd.h>		// gettimeofday
#include "softbot4.h"
#include "api_sr.h"
#include "ut_stest.h"

//#ifdef _NOT_USED_
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
//#endif

#define DEBUG
#define MAX_LIST_COUNT 100
#define SLOT_SIZE 20
#define SLOT_TIME_INTERVAL 100
#define SLOT_TIME_BASE 20



typedef struct {
	pthread_mutex_t	*fp_lock;
	FILE *fp;		/* fp of query list file */
	pthread_mutex_t	*count_lock;
	int volatile	success;	/* number of requests successed */
	int volatile	fail;		/* number of requests failed */
	int volatile	read;		/* number of bytes read */
	int volatile	time;		/* total sum of time in ms for connections */
	int volatile	min_time;	/* minimum time in ms for connection */
	int volatile	max_time;	/* maximum time in ms for connection */
	int volatile	ended;		/* check if test has ended */
	int volatile	time_dist[SLOT_SIZE];	/* table of time distribution */
	int volatile	min_count;
	int volatile	max_count;

	/* modified by woosong for load balanced test 2001.05.16 */
#define	MAX_HOST	128
	pthread_mutex_t	*host_lock;
	int volatile	host_count;
	int volatile	host_load[MAX_HOST];
	char			hostname[MAX_HOST][256];

	long long volatile	total_count;
	struct timeval start;		/* start time */
	struct timeval end;		/* end time */

	int requests;				/* total number of requests */
	int concurrency;
} status_t;

char host[SHORT_STRING_SIZE];
int  port;
int  verbose = 0;
int  filter = 0; /* enable SBASrchDocFilter */
/* modified by woosong for load balanced test 2001.05.16 */
int  balance = 0;
status_t status;


void get_query_string (char *query, int page, char *buf);
static int timedif(struct timeval a, struct timeval b);
void softbot_bench(int *i);


static void err(char *s)
{
	if (errno) {
		perror(s);
	} else {
		printf("%s", s);
	}
}

void
get_query_string (char *query, int page, char *buf)
{
	sprintf(buf, "QU=%s^PG=%d^LC=%d^",
					query, page, 15);
}


static int timedif(struct timeval a, struct timeval b)
{
	register int us, s;
	us = a.tv_usec - b.tv_usec;
	us /= 1000;
	s = a.tv_sec - b.tv_sec;
	s *= 1000;

	return s + us;
}

static char *timestr(struct timeval a, char *buf)
{
	sprintf(buf, "%dsec, %3dms", a.tv_sec, a.tv_usec/1000);
	return buf;
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
	char  str[STRING_SIZE];
	struct timeval start, end;
	int   timetaken; /* time difference in ms */
	SBHANDLE	SBHD;
	int	  r, n;
	char  *s;
	int   i = *num;
	char  *host_to_use;
	int   j, k, min;
	
	r = SBAConstructHSR(&SBHD, MAX_LIST_COUNT);
	if ( r < 1 ) {
		perror("SBAConstructHSR");
		return;
	}

	while ( 1 ) {

		pthread_mutex_lock(status.fp_lock);
		s = fgets(str, STRING_SIZE, status.fp);
		pthread_mutex_unlock(status.fp_lock);
		if ( s == NULL ) {
			return;
		}
		/* remove \n from end of string */
		n = strlen(str);
		if ( str[n-1] == '\n' ) str[n-1] = '\0';
		get_query_string(str, 0, buf);

		gettimeofday(&start, 0);
		/* modified by woosong for load balanced test 2001.05.16 */
		if ( balance )
		{
			pthread_mutex_lock(status.host_lock);
			min = status.host_load[0];
			k = 0;
			for(j = 0; j < status.host_count; j++)
			{
				if (status.host_load[j] < min)
				{
					min = status.host_load[j];
					k = j;
				}	
			}
			status.host_load[k]++;
			host_to_use = status.hostname[k];
			pthread_mutex_unlock(status.host_lock);
		}
		else
			host_to_use = host;
	
		if ( filter )
			r = SBASrchDocFilter(host_to_use, port, buf, SBHD);
		else
			r = SBASrchDoc(host_to_use, port, buf, SBHD);
		
		gettimeofday(&end, 0);
		timetaken = timedif(end, start);

		pthread_mutex_lock(status.count_lock);
		if ( status.success >= status.requests ) {
			if ( status.ended == 0 ) {
				gettimeofday(&status.end, 0);
				status.ended = 1;
				printf ("[%d] test ended. joining threads...\n", i);
			}
			pthread_mutex_unlock(status.count_lock);
			return;
		}
		if ( r != 0 ) {
			status.fail++;
			if ( verbose )
				printf("F (%d) %03dms : %s\n", r, timetaken, str);
		} else {
			if ( status.success % 100 == 0 ) {
				printf(".");
				fflush(stdout);
			}
			status.success++;
			status.time += timetaken;
			status.min_time = timetaken < status.min_time ?
						timetaken : status.min_time;
			status.max_time = timetaken > status.max_time ?
						timetaken : status.max_time;
			status.time_dist[timeslot(timetaken)]++;
			status.max_count = status.max_count > ((CHSR*)SBHD)->TotCnt ?
				   		status.max_count : ((CHSR*)SBHD)->TotCnt;	
			status.min_count = status.min_count < ((CHSR*)SBHD)->TotCnt ?
				   		status.min_count : ((CHSR*)SBHD)->TotCnt;	
			status.total_count += ((CHSR*)SBHD)->TotCnt;
			if ( timetaken < SLOT_TIME_BASE && verbose )
				printf("S %03dms : %s\n", timetaken, str);

		}
		pthread_mutex_unlock(status.count_lock);
		
		/* modified by woosong for load balanced test 2001.05.16 */
		if ( balance )
		{
			pthread_mutex_lock(status.host_lock);
			status.host_load[k]--;
			pthread_mutex_unlock(status.host_lock);
		}
	}

}

/* display usage information */
static void usage(char *progname)
{
	fprintf(stderr, "Usage: %s [options] host port query_list_file\n", progname);
	fprintf(stderr, "Options are:\n");
	fprintf(stderr, "    -n requests    Number of requests to perform\n");
	fprintf(stderr, "    -c concurrency Number of multiple requests to make\n");
	fprintf(stderr, "    -f             Enable SBASrchDocFilter\n");
	/* modified by woosong for load balanced test 2001.05.16 */
	fprintf(stderr, "    -m             Lookup host name once\n");
	fprintf(stderr, "    -v             Verbose output\n");
	fprintf(stderr, "    -h             Display usage information(this message)\n");
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
	pthread_mutex_t host_lock;
	int  timetaken;
	char buf[STRING_SIZE];
	FILE *fp;

	/* modified by woosong for load balanced test 2001.05.16 */
	struct hostent* HostElement;
	struct in_addr	address;

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
	/* modified by woosong for load balanced test 2001.05.16 */
	status.host_count = 0;
	
	for(i = 0; i < SLOT_SIZE; i++) status.time_dist[i] = 0;
	optind = 1;
	/* modified by woosong for load balanced test 2001.05.16 */
	while ((c = getopt(argc,argv, "n:c:fvm")) > 0 ) {
		switch (c) {
		case 'n':
			status.requests = atoi(optarg);
			if (!status.requests) {
				err("Invalid number of requests\n");
				return 0;
			}
			break;
		case 'c':
			status.concurrency = atoi(optarg);
			break;
		case 'f':
			filter = 1;
			break;
		case 'v':
			verbose = 1;
			break;
	/* modified by woosong for load balanced test 2001.05.16 */
		case 'm':
			balance = 1;
			break;
		default:
			fprintf(stderr, "%s: invalid option '%c'\n", argv[0], c);
			usage(argv[0]);
			return 0;
		}
	}

	if (optind != argc - 3) {
		fprintf(stderr, "%s: wrong number of arguments\n", argv[0]);
		usage(argv[0]);
		return 0;
	}

	/* set values of struct status */
	threads = (pthread_t *) calloc(status.concurrency, sizeof(pthread_t));
	tindex = (int *) calloc(status.concurrency, sizeof(int));
	strcpy(host, argv[optind++]);
	port = atoi(argv[optind++]);
	fp = fopen(argv[optind++], "rt");
	if ( fp == NULL ) {
		perror("cannot open query list file");
		return 0;
	}
	status.fp = fp;

	if ( verbose )
		printf("verbose mode on\n");
	if ( filter )
		printf("SBASrchDocFilter enabled\n");
	/* modified by woosong for load balanced test 2001.05.16 */
	if ( balance )
	{
		printf("Load balancing + Name pre lookup enabled\n");
		
		memset(&address, 0x00, sizeof(address));

		HostElement = gethostbyname(host);
		if (!HostElement) {
			fprintf(stderr, "host %s look up error!\n", host);
			return 0;
		}

		i = 0;
		while(HostElement->h_addr_list[i])
		{
			memcpy((void *)&address, HostElement->h_addr_list[i], HostElement->h_length);
			status.host_load[i] = 0;
			strcpy(status.hostname[i++], inet_ntoa(address));
		}

		status.host_count = i;

		printf("%d search machines\n", status.host_count);
	}
		
	printf("requests = %d\n", status.requests);
	printf("concurrency = %d\n", status.concurrency);
	printf("host = %s\n", host);
	printf("port = %d\n", port);

	/* mutex initialization */
	pthread_mutex_init(&fp_lock, NULL);
	pthread_mutex_init(&count_lock, NULL);
	pthread_mutex_init(&host_lock, NULL);
	status.fp_lock = &fp_lock;
	status.count_lock = &count_lock;
	/* modified by woosong for load balanced test 2001.05.16 */
	status.host_lock = &host_lock;

	/* make threads */
	pthread_mutex_lock(status.fp_lock);
	for(i = 0; i < status.concurrency; i++)
	{
		tindex[i] = i;
		if (pthread_create(&threads[i], NULL, (void *)softbot_bench, &tindex[i])) {
			fprintf(stderr, "%s: cannot make thread\n", argv[0]);
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
			fprintf(stderr, "%s: cannot join thread\n", argv[0]);
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
	
	printf("max count %d, min count %d, avg count %ld\n", status.max_count, status.min_count, status.total_count/status.success);
	
	
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

		
