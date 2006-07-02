/* $Id */
#ifndef SB_BENCHMARK_H
#define SB_BENCHMARK_H

typedef struct bm_scoreboard_t bm_scoreboard_t;

typedef struct {
	char *host;
	char *port;
	int  total_requests; /* num of requests */
	int  concurrency;    /* num of threads */
	int  delay;          /* miliseconds */
	int  verbose;        /* verbose mode when set 1 */
	char *(*read_input)(void);
	int  (*do_test)(char *in);
	bm_scoreboard_t *scoreboard;
} bm_scenario_t;

int benchmark_init(bm_scenario_t *scn);
int benchmark_run(bm_scenario_t *scn);
int benchmark_destroy(bm_scenario_t *scn);

#endif /* SB_BENCHMARK_H */
