#ifndef _STOPWATCH_H_
#define _STOPWATCH_H_

#ifdef DEBUGTIME
#  include <sys/time.h>
#  include <time.h>

#  ifndef MAX_MARK
#    define MAX_MARK 20
#  endif

#  ifndef TIME_COUNT // 이만큼 실행후 통계출력
#    define TIME_COUNT 300
#  endif

static uint64_t    time_lengths[MAX_MARK];
static const char* time_names[MAX_MARK];
static int         time_idx;
static uint64_t    time_old;

static int         time_count = -1;

static void _time_start()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	if ( time_count == -1 ) {
		memset(time_lengths, 0, sizeof(time_lengths));
		memset(time_names, 0, sizeof(time_names));
		time_count = 0;
	}

	time_idx = 0;
	time_count++;
	time_old = tv.tv_sec * 1000000 + tv.tv_usec;
}

static void _time_mark(const char* name)
{
	uint64_t time_new;
	struct timeval tv;

	if ( time_idx >= MAX_MARK ) {
		warn("too many time mark. max is %d", MAX_MARK);
		return;
	}

	gettimeofday(&tv, NULL);
	time_new = tv.tv_sec * 1000000 + tv.tv_usec;

	time_names[time_idx] = name;
	time_lengths[time_idx] += time_new - time_old;

	time_old = time_new;
	time_idx++;
}

static void _time_status()
{
	if ( time_count >= TIME_COUNT ) {
		int i;
		for ( i = 0; i < MAX_MARK && time_names[i] != NULL; i++ ) {
			info("%-20s: %'10" PRIu64 " usec", time_names[i], time_lengths[i]);
		}
		time_count = -1;
	}
}

#  define time_start() _time_start()
#  define time_mark(name) _time_mark(name)
#  define time_status() _time_status()
#else
#  define time_start()
#  define time_mark(name)
#  define time_status()
#endif // DEBUGTIME

#endif // _STOPWATCH_H_
