/* $Id$ */
#include "common_core.h"
#include "ipc.h"
#include <stdlib.h> /* EXIT_SUCCESS */
#include <signal.h>
#include <string.h>
#include <unistd.h> /* usleep(3) */

#define MAX_THREADS (20)

#define USE_THREAD
#undef  USE_THREAD

#ifdef USE_THREAD
static scoreboard_t scoreboard[] = { THREAD_SCOREBOARD(MAX_THREADS) };
#else
static scoreboard_t scoreboard[] = { PROCESS_SCOREBOARD(MAX_THREADS) };
#endif

int *shared_int = NULL;
int needed_threads = 10;
int test_count = 1000*1000;
rwlock_t* rwlock;

/****************************************************************************/
static void _do_nothing(int sig)
{
	return;
}

static void _shutdown(int sig)
{
#ifdef USE_THREAD
	pthread_exit(NULL);
#else
	exit(EXIT_SUCCESS);
#endif
}

static void _graceful_shutdown(int sig)
{
	struct sigaction act;

	memset(&act, 0x00, sizeof(act));

//	act.sa_flags = SA_RESTART;
	sigfillset(&act.sa_mask);

	act.sa_handler = _do_nothing;
	sigaction(SIGHUP, &act, NULL);
	sigaction(SIGINT, &act, NULL);

	scoreboard->graceful_shutdown++;
}
/****************************************************************************/
static int child_main (slot_t *slot)
{
	int i;
	int inc;
   
	inc = (slot->id % 2 == 0) ? 1 : -1;

	for (i = 0; i < test_count; i++) {
		if (i % 10 == 0) {
			rwlock_wrlock(rwlock);
			*shared_int += inc;
			rwlock_unlock(rwlock);
		} else {
			rwlock_rdlock(rwlock);
			if (*shared_int > 2 || *shared_int < -2)
				warn("[%d] shared int = %d", slot->id, *shared_int);
			rwlock_unlock(rwlock);
		}
		usleep(1);

		if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) break;
	}
	slot->state = SLOT_FINISH;
	debug("slot[id:%d][pthread:%lu] exits.", slot->id, slot->pthread);

	return EXIT_SUCCESS;
}

static int module_main (slot_t *slot)
{
	char filename[L_tmpnam];
	ipc_t ipc;

	if (tmpnam(filename) == NULL) {
		error("tmpnam(3) returned NULL.");
		return 1;
	}
	ipc.type     = IPC_TYPE_MMAP;
	ipc.pathname = filename;
	ipc.size     = sizeof(int)+rwlock_sizeof();
	if (alloc_mmap(&ipc,0) != SUCCESS) {
		error("alloc_mmap() returned FAIL.");
		return EXIT_FAILURE;
	}
	shared_int = ipc.addr;
	*shared_int = 0;

	rwlock = ipc.addr + sizeof(int);

	rwlock_init(rwlock);

	sb_run_set_default_sighandlers(_shutdown, _graceful_shutdown);

	scoreboard->size = (needed_threads < MAX_THREADS) ? needed_threads : MAX_THREADS;
	if (sb_run_init_scoreboard(scoreboard) != SUCCESS) return EXIT_FAILURE;

	scoreboard->period = 3;
	rwlock_wrlock(rwlock);
	sb_run_spawn_processes(scoreboard, "rwlock process", child_main);

	sleep(1);
	CRIT("start of test: shared int = %d", *shared_int);
	rwlock_unlock(rwlock);

	sb_run_monitor_processes(scoreboard);

	CRIT("end of test: shared int = %d", *shared_int);
	SB_DEBUG_ASSERT(*shared_int == 0);

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
	scoreboard,			/* scoreboard */
	NULL,				/* register hook api */
};
