/* $Id$ */
#include "common.h"

#define MAX_THREADS (100)

static scoreboard_t scoreboard[] = { THREAD_SCOREBOARD(MAX_THREADS) };

int *shared_int = NULL;
int needed_threads = 10;
int test_count = 1000*1000;
rwlock_t rwlock;

static int child_main (slot_t *slot)
{
	int i;
	int inc = (slot->id % 2 == 0) 1 : -1;

	for (i = 0; i < test_count; i++) {
		if (i % 10 == 0) {
			rwlock_wrlock(&rwlock);
			*shared_int += inc;
			rwlock_unlock(&rwlock);
		} else {
			rwlock_rdlock(&rwlock);
			if (*shared_int > 100 || *shared_int < -100)
				warn("[%d] shared int = %d", slot->id, *shared_int);
			rwlock_unlock(&rwlock);
		}
	}

	return EXIT_SUCCESS;
}

static int module_main (slot_t *slot)
{
	int n;
	char filename[L_tmpnam];
	ipc_t ipc;

	if (tmpnam(filename) == NULL) {
		error("tmpnam(3) returned NULL.");
		return 1;
	}
	ipc.type     = IPC_TYPE_MMAP;
	ipc.pathname = filename;
	ipc.size     = sizeof(int);
	if (alloc_mmap(&ipc,0) != SUCCESS) {
		error("alloc_mmap() returned FAIL.");
		return 1;
	}
	shared_int = ipc.addr;
	*shared_int = 0;

	CRIT("start of test: shared int = %d", *shared_int);
	rwlock_init(&rwlock, 0);

	scoreboard->size = (needed_threads < MAX_THREADS) ? needed_threads : MAX_THREADS;
	sb_run_init_scoreboard(scoreboard);

	rwlock_wrlock(&rwlock);
	sb_run_spawn_processes(scoreboard, "rwlock process", child_main);
	rwlock_unlock(&rwlock);

	sb_run_monitor_processes(scoreboard);

	CRIT("end of test: shared int = %d", *shared_int);
	free_mmap(shared_int, sizeof(int));

	return EXIT_SUCCESS;

}


static void config_threads(configValue v)
{
	needed_threads = atoi(v.argument[0]);
	if (needed_threads > MAX_THREADS)
		warn("You should not set Threads value more than MAX_THREADS(%d).", MAX_THREADS);
}

static void config_count(configValue v)
{
	test_count = atoi(v.argument[0]);
}

static config_t config[] = {
	CONFIG_GET("Threads", config_threads, 1, "number of threads"),
	CONFIG_GET("Count",   config_count,   1, "number of test count"),
	{NULL}
};


module test_rwlock_module = {
	STANDARD_MODULE_STUFF,
	config,				/* config */
	NULL,				/* registry */
	NULL,				/* initialize */
	module_main,		/* child_main */
	NULL,				/* scoreboard */
	NULL,				/* register hook api */
};
