/* $Id$ */
#include "common_core.h"
#include <pthread.h>
#include <unistd.h>  /* usleep(3) */
#include <signal.h>
#include <string.h>  /* memset(3) */
#include <errno.h>
#include <stdlib.h>  /* EXIT_SUCCESS .. */
//#include <sys/types.h>
#include <sys/wait.h>  /* waitpid(2) */
#include "hook.h"
#include "log_error.h"
#include "scoreboard.h"
#include "modules.h"
#include "memory.h"
#include "mod_mp.h"
#include "proc_list.h"

static void slow_start()
{
	//sleep(1);
	usleep(100*1000); /* 0.1 sec in micro second */
}

static void _do_nothing(int sig)
{
	return;
}

#if 0
static void _sig_panic(int sig)
{
	//chdir(coredump_dir);
	pthread_kill_other_threads_np();

	// XXX: use sigaction?
	signal(sig, SIG_DFL);
	raise(sig);

	/* At this point we've got sig blocked, because we're still inside
	 * the signal handler.  When we leave the signal handler it will
	 * be unblocked, and we'll take the signal... and coredump or whatever
	 * is appropriate for this particular Unix.  In addition the parent
	 * will see the real signal we received -- whereas if we called
	 * abort() here, the parent would only see SIGABRT.
	 */
}
#endif

static void _reopen_log_error(int sig)
{
	reopen_error_log(gErrorLogFile, gQueryLogFile);
	return;
}

static int set_default_sighandlers(void (*shutdown_handler)(int),
								   void (*graceful_shutdown_handler)(int))
{
	struct sigaction act;

	memset(&act, 0x00, sizeof(act));
//	act.sa_flags = SA_RESTART;
	sigfillset(&act.sa_mask);
	/* see set_sigaction() of server.c */
	// We need to do sigfillset(act.sa_mask) for signal deferring.
	//  --jiwon

	act.sa_handler = _do_nothing;
	sigaction(SIGCHLD, &act, NULL);
	sigaction(SIGPIPE, &act, NULL);
	sigaction(SIGXFSZ, &act, NULL);
	//sigaction(SIGALRM, &act, NULL);

	act.sa_handler = shutdown_handler;
	sigaction(SIGQUIT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);

	act.sa_handler = graceful_shutdown_handler;
	sigaction(SIGINT, &act, NULL);

	act.sa_handler = _reopen_log_error;
	sigaction(SIGHUP, &act, NULL);

/*	act.sa_handler = _sig_panic;*/
/*	sigaction(SIGSEGV, &act, NULL);*/
/*	sigaction(SIGBUS, &act, NULL);*/
/*	sigaction(SIGABRT, &act, NULL);*/
/*	sigaction(SIGILL, &act, NULL);*/
/*	sigaction(SIGTRAP, &act, NULL);*/
/*	sigaction(SIGFPE, &act, NULL);*/

	return SUCCESS;
}

static int spawn_thread(slot_t *slot, const char *name, int (*main)(slot_t *))
{
	int n;

	debug("slot[%p], name[%s], main[%p]", slot, name, main);
	if ( slot == NULL || main == NULL ) {
		error("invalid argument, slot[%p], main[%p]", slot, main);
		return FAIL;
	}

	slot->generation++;
	slot->state = SLOT_START;
	time(&(slot->start_time));
	slot->main = main;
	strncpy(slot->name, name, SHORT_STRING_SIZE);
	slot->name[SHORT_STRING_SIZE-1] = '\0';

	n = pthread_create(&(slot->pthread), NULL, (void *)main, slot);
	if ( n != 0 ) {
		error("pthread_create returned %d: %s", n, strerror(errno));
		return FAIL;
	}
	slot->pid = slot->pthread;

	n = pthread_detach(slot->pthread);
	if ( n != 0 ) warn("pthread_detach returned %d: %s", n, strerror(errno));

	return SUCCESS;
}


static int spawn_process(slot_t *slot, const char *name, int (*main)(slot_t *))
{
	pid_t pid = -1;

	//debug("slot[%p], name[%s], main[%p]", slot, name, main);
	if ( slot == NULL || main == NULL ) {
		error("invalid argument, slot[%p], main[%p]", slot, main);
		return FAIL;
	}

	pid = sb_fork();
	if ( pid > 0 ) {
		slot->pid = pid;
		return SUCCESS; /* parent */
	} else if ( pid < 0 ) {
		error("error while sb_fork() for %s: %s", name, strerror(errno));
		return FAIL;
	}

	/* child process */
	slot->generation++;
	slot->state = SLOT_START;
	time(&(slot->start_time));
	slot->main = main;

	if(slot->name != name) {
	    strncpy(slot->name, name, SHORT_STRING_SIZE);   
	    slot->name[SHORT_STRING_SIZE-1] = '\0'; 
	}

	set_proc_desc(slot, "softbotd: %s", name);
	if ( main(slot) == EXIT_SUCCESS ) 
		_exit(EXIT_SUCCESS); /* module_main returns EXIT_SUCCESS when it's ok */
	else 
		_exit(EXIT_FAILURE);

	return SUCCESS; /* never comes here */
}


static int spawn_processes(scoreboard_t *scoreboard, const char *name, int (*main)(slot_t *))
{
	int i=0;
	if ( scoreboard->type != TYPE_PROCESS ) {
		error("scoreboard type [%d] is not suitable.", scoreboard->type);
		return FAIL;
	}
	scoreboard->slot[0].pid = getpid();
	scoreboard->slot[0].generation++;
	scoreboard->slot[0].state = SLOT_START;
	time(&(scoreboard->slot[0].start_time));
	strncpy(scoreboard->slot[0].name, "PARENT", SHORT_STRING_SIZE);

	debug("spawning each process...");
	for (i = 1; i <= scoreboard->size; i++) {
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) break;

		slow_start();
		spawn_process(&(scoreboard->slot[i]), name, main);
	}

	return SUCCESS;
}

static int spawn_threads(scoreboard_t *scoreboard, const char *name, int (*main)(slot_t *))
{
	int i=0;

	debug("thread [%s]", name);
	if ( scoreboard->type != TYPE_THREAD ) {
		error("scoreboard type [%d] is not suitable.", scoreboard->type);
		return FAIL;
	}
	scoreboard->slot[0].pid = getpid();
	scoreboard->slot[0].generation++;
	scoreboard->slot[0].state = SLOT_START;
	time(&(scoreboard->slot[0].start_time));
	strncpy(scoreboard->slot[0].name, "PARENT", SHORT_STRING_SIZE);

	/* slot[1] is the first slot for children */
	debug("spawning each thread...");
	for (i = 1; i <= scoreboard->size; i++) {
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) break;
		slow_start();
		spawn_thread(&(scoreboard->slot[i]), name, main);
	}

	return SUCCESS;
}

static int spawn_processes_for_each_module(scoreboard_t *scoreboard, module *mod)
{
	int i;
	module *m;
	slot_t *slot;

	if ( scoreboard->type != TYPE_PROCESS ) {
		error("scoreboard type [%d] is not suitable.", scoreboard->type);
		return FAIL;
	}
	scoreboard->slot[0].pid = getpid();
	scoreboard->slot[0].generation++;
	scoreboard->slot[0].state = SLOT_START;
	time(&(scoreboard->slot[0].start_time));
	strncpy(scoreboard->slot[0].name, "PARENT", SHORT_STRING_SIZE);

	i = 1; /* slot[1] is the first slot for children */
	for (m = mod; m; m = m->next) {
		if ( m->main == NULL ) continue;
		if ( i > scoreboard->size ) {
			warn("scoreboard's slots[%d] are now full", scoreboard->size);
			break;
		}

		slot = &(scoreboard->slot[i]); i++;

		if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) break;

		slow_start();
		crit("spawn process for module[%s]", m->name);
		strncpy(scoreboard->slot[i].name, m->name, SHORT_STRING_SIZE);
		scoreboard->slot[i].name[SHORT_STRING_SIZE-1] = '\0';
		scoreboard->slot[i].main = m->main;
		spawn_process(slot, m->name, m->main);
	}

	if (i == 1) {
		crit("no processes are forked");
	}

	return SUCCESS;
}


static int spawn_process_for_module(scoreboard_t *scoreboard, module *mod)
{
	if ( scoreboard->type != TYPE_PROCESS ) {
		error("scoreboard type [%d] is not suitable.", scoreboard->type);
		return FAIL;
	}

	scoreboard->slot[0].pid = getpid();
	scoreboard->slot[0].generation++;
	scoreboard->slot[0].state = SLOT_START;
	time(&(scoreboard->slot[0].start_time));
	strncpy(scoreboard->slot[0].name, "PARENT", SHORT_STRING_SIZE);

	if ( mod->main == NULL ) {
		error("module %s has no main function.", mod->name);
		return FAIL;
	}

	if ( scoreboard->size < 1 ) {
		error("scoreboard size [%d] is invalid!", scoreboard->size);
		return FAIL;
	}

	return spawn_process(&scoreboard->slot[1], mod->name, mod->main);
}


/* pthread does not allow to join on any thread that happens to terminate
 * like wait(2) call for processes.
 * here are three solutions:
 *  1) make detached threads and have them decrease a counter as they exit.
 *  2) make a condition variable and send signal when a thread exits.
 *  3) make a monitoring thread and check if any thread exited periodically.
 * currenly 3) is chosen.
 * 
 * well.. 2) looks like better than others. no busy wait and immediate
 * reaction is possible. BUT, 1) or 2) is very dependent on pthread calls
 * and spawned thread has to do something like decreasing a counter.
 * this causes incompatible code with monitor_processes().
 * --aragorn, 2002/06/08
 */

static int monitor_threads(scoreboard_t *scoreboard)
{
	slot_t *slot;
	struct timeval timeout;
	int i=0, n=0, alive;

	if ( scoreboard->period == 0 ) {
		warn("%s monitor: scoreboard->period[0] is too short."
			 " temporarily setting it [3]", scoreboard->name);
		scoreboard->period = 3;
	}

	for ( ; ; ) {
		/* we need to reinitialize timeval.
		 * see NOTES of select(2). Linux specific problem */
		timeout.tv_sec = scoreboard->period;
		timeout.tv_usec = 0;

		set_proc_desc(NULL, "softbotd: monitoring %s",scoreboard->name);

		// checking threads and if died, restart thread
		for (i = 1, alive = 0; i <= scoreboard->size; i++) {
			slot = &(scoreboard->slot[i]);
			if (slot->pthread == 0 || slot->state == SLOT_OPEN) continue;

			n = pthread_kill(slot->pthread, 0);
			if ( n == 0 ) {
			/* 쓰레드가 살아있다. */
				alive++;
				if ( scoreboard->shutdown || scoreboard->graceful_shutdown )
				{
					int shutdown_signal = SIGNAL_GRACEFUL_SHUTDOWN;
					if ( scoreboard->shutdown ) shutdown_signal = SIGNAL_SHUTDOWN;

					pthread_kill(slot->pthread, shutdown_signal);
					debug("pthread_kill(slot[%d],signal[%d])",
											slot->id, shutdown_signal);
				}

			} else if ( n == ESRCH ) {
			/* 쓰레드가 종료되었다.
			 * 종료 상태인 경우, 무시한다.
			 * SLOT_RESTART인 경우, 재생성한다.
			 * 그 외의 경우, SLOT을 비우고, 무시한다.
			 */
				if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) continue;

				if ( slot->state == SLOT_RESTART ) {
					slow_start();
					spawn_thread(slot, slot->name, slot->main);
					alive++;
					continue;
				}
				/* do not spawn threads when slot state is finish */
				else if ( slot->state == SLOT_FINISH ) {
					info("thread of %s->slot[%d] has died with slot->state: SLOT_FINISH[%d]",
							scoreboard->name, i, slot->state);
					info("making slot state SLOT_OPEN");
					slot->state = SLOT_OPEN;
					continue;
				}
				else if (slot->state == SLOT_OPEN) {
					continue;
				}

				if (slot->state != SLOT_RESTART) {
					warn("thread of %s->slot[%d] has died with invalid state[%d]",
							scoreboard->name, i, slot->state);
				}
			} else {
				crit("%s monitor: unknown pthread error:"
					 " pthread_kill() returned %d", 
						scoreboard->name, n);
			}
		}

		/* shutdown flag is on, so we have to break and return. */
		if ( alive == 0 ) break;
		//if ( (scoreboard->shutdown ||
		//	  scoreboard->graceful_shutdown) && alive == 0 ) break;

		if ( scoreboard->shutdown || scoreboard->graceful_shutdown )
		{
			timeout.tv_sec  = 0;
			timeout.tv_usec = 1000 * 100;
		}
		select(0, NULL, NULL, NULL, &timeout);
	}
	scoreboard->slot[0].state = SLOT_FINISH;
	debug("%s monitor: returning SUCCESS", scoreboard->name);

	return SUCCESS;

}

#define ERROR_TIME_SLOT_SIZE	(10)
#define TOO_MANY_ERRORS			(10)
#define WAIT_WHEN_NO_CHILD		(3)
static int _monitor_processes(scoreboard_t *scoreboard, module *mod)
{
	int status;
	pid_t pid;
	slot_t *slot;
	struct timeval timeout;
	int timeslot = 0;
	int errors = 0;
	int now = 0;
	proc_node *proc_list = NULL, *proc_node;
	int slot_id;
	int wait_when_no_child = 3;


	if ( scoreboard->period == 0 ) {
		warn("%s monitor: scoreboard->period[0] is too short."
			 " temporarily setting it [3]", scoreboard->name);
		scoreboard->period = 3;
	}

	debug("scoreboard slot size:%d",scoreboard->size);
	for ( ; ; ) {
		// 살릴 시간이 된 녀석들은 살린다. 여러개일 수도 있지만 하나씩만 천천히 살리자.
		proc_node = delete_from_first(&proc_list);
		if ( proc_node ) {
			slot_id = proc_node->slot_id;
			sb_free(proc_node);

			slot = &scoreboard->slot[slot_id];
			if ( slot == NULL ) {
				error("slot[%d] is NULL", slot_id);
				continue;
			}
			slow_start();
			spawn_process(slot, slot->name, slot->main);
		}

		/* we need to reinitialize timeval.
		 * see NOTES of select(2). Linux specific problem */
		timeout.tv_sec = scoreboard->period;
		timeout.tv_usec = 0;

		set_proc_desc(NULL, "softbotd: monitoring %s",scoreboard->name);

		/* monitor child processes */
		pid = waitpid(-1, &status, WNOHANG);
		if ( pid != 0 ) debug("waitpid returned pid[%d]", (int) pid);
		if ( pid == 0 ) {
			/* 종료된 child process가 없다.
			 * 종료 시그널을 받은 경우, 각 child process에서 종료시그널을 보내고,
			 * 다음번 루프로 돌아간다. */
			wait_when_no_child = WAIT_WHEN_NO_CHILD;
			int i;
			if ( scoreboard->shutdown || scoreboard->graceful_shutdown )
			{
				int shutdown_signal = SIGNAL_GRACEFUL_SHUTDOWN;
				if ( scoreboard->shutdown ) shutdown_signal = SIGNAL_SHUTDOWN;

				for (i = 1; i <= scoreboard->size; i++)
				{
					slot = &(scoreboard->slot[i]);
					if (slot->pid == 0 || slot->state == SLOT_OPEN) continue;

					if (kill(slot->pid, 0) == 0) // process is alive
					{
						kill(slot->pid, shutdown_signal);
						debug("sent child process[%d] signal[%d]", slot->pid, shutdown_signal);
					}
				}
			}

			/* TODO check each child process actively */
			/* send signal 0 to each signal and check return value */
			//check_children(scoreboard);
			
			select(0, NULL, NULL, NULL, &timeout);

		} else if ( pid > 0 ) {
			/* 종료된 child process가 있다.
			 * 1) 정상종료이고, 재시작이 필요하면 프로세스를 다시 생성한다.
			 * 2) 정상종료이고, 재시작이 필요없으면, SLOT을 비우고 내버려둔다.
			 * 3) 비정상종료이고, 짧은 시간 반복적으로 종료한다면, 전체 시스템을
			 *    종료한다.
			 * 4) 시그널 받은 비정상종료이고, 종료 중인 경우, 무시한다.
			 * 5) 시그널 받은 비정상종료이고, 종료 중이지 않은 경우, 재시작 큐에 추가한다.
			 * 6) 시그널 없이 비정상종료한 경우, 종료 중인 경우, 무시한다.
			 * 7) 시그널 없이 비정상종료한 경우, 종료 중이지 않은 경우, 재생성한다.
			 * 8) 그 외 경우 - 예측 불가
			 */
			slot = get_slot_by_pid(scoreboard, pid);
			if ( slot == NULL ) {
				info("waited process[%d] is not my child", (int)pid);
				continue;
			}

			if ( WIFEXITED(status) && WEXITSTATUS(status) == 0 ) {
				info("%s monitor: child process[%d, %s] "
						"exited with status[%d].",
						scoreboard->name, (int) pid, slot->name, status);
				if (slot->state == SLOT_RESTART) {
					slow_start();
					spawn_process(slot, slot->name, slot->main);
					continue;
				}
				slot->state = SLOT_OPEN;
				slot->pid = 0;
				continue;
				/* go back to start of this loop and wait for more child processes */
			}
			slot->state = SLOT_OPEN;
			slot->pid = 0;

			now = (int)time(NULL)/ERROR_TIME_SLOT_SIZE;
			if ( now == timeslot ) {
				errors++;
				if ( errors > TOO_MANY_ERRORS ) {
					alert("%s monitor: too many errors have occured "
							"for a short time!!!", scoreboard->name);
					scoreboard->graceful_shutdown++;
				}
				if ( errors > TOO_MANY_ERRORS * 2 )
					scoreboard->shutdown++;
			} else {
				timeslot = now;
				errors = 0;
			}

			if ( WIFSIGNALED(status) ) { /* killed by signal */
				int sig = WTERMSIG(status);

				warn("%s monitor: child process[%d, %s] killed by signal[%d].",
						scoreboard->name, (int)pid, slot->name, sig);

				if (scoreboard->shutdown ||
					scoreboard->graceful_shutdown ) continue;

				/* 비정상 종료인 경우, proc_list queue에 넣어두고, 다음 루프에
				 * 천천히 재시작시킨다. */
				if ( add_to_last( &proc_list, slot->id ) == SUCCESS ) continue;

				slow_start();
				spawn_process(slot, slot->name, slot->main);
			} else if ( WIFEXITED(status) && WEXITSTATUS(status) > 0 ) {
				warn("%s monitor: child process[%d, %s] exited with exitstatus[%d].",
						scoreboard->name, (int)pid, slot->name, WEXITSTATUS(status));

				if (scoreboard->shutdown ||
					scoreboard->graceful_shutdown ) continue;

				slow_start();
				spawn_process(slot, slot->name, slot->main);
			} else {
				info("%s monitor: child process[%d, %s] exited with status[%d].",
						scoreboard->name, (int)pid, slot->name, status);
			}
		} else if ( pid == -1 && errno == ECHILD ) {
			/* 자식 프로세스가 없다.
			 * 1) 종료해야 하는 경우, 종료한다.
			 * 2) 종료하지 않아야 하는 경우, 이상하다.
			 */
			--wait_when_no_child;
			info("%s monitor: no child process. wait for %d times. [%d][%d]",
					scoreboard->name, wait_when_no_child,
					scoreboard->shutdown, scoreboard->graceful_shutdown);
			set_proc_desc(NULL, "softbotd: %s monitor: no child process [%d][%d]",
					scoreboard->name, scoreboard->shutdown, scoreboard->graceful_shutdown);

			if (scoreboard->shutdown || scoreboard->graceful_shutdown) break;
			if (wait_when_no_child <= 0) break;

			select(0, NULL, NULL, NULL, &timeout);
		} else {
			error("%s monitor: what's wrong?: %s", scoreboard->name, strerror(errno));
		}
		/* nothing to be placed here,
		 * for 'continue' is used in the 'if' clause above */
	}

	return SUCCESS;
}

static int monitor_processes(scoreboard_t *scoreboard)
{
	return _monitor_processes(scoreboard, NULL);
}

static int monitor_processes_for_modules(scoreboard_t *scoreboard, module *mod)
{
	return _monitor_processes(scoreboard, mod);
}


static int init_scoreboard(scoreboard_t *scoreboard)
{
	int i;

	if (scoreboard->init == 0) {
		error("uninitialized scoreboard. "
			"check if the module object has a proper scoreboard pointer.");
		return FAIL;
	}
	scoreboard->shutdown = 0;
	scoreboard->graceful_shutdown = 0;
	scoreboard->period = MONITORING_SCOREBOARD_PERIOD;

	for(i = 0; i <= scoreboard->size; i++) {
		scoreboard->slot[i].id = i;
		/* scoreboard->slot[i].generation */
	    /* do not initialize generation value. */
		scoreboard->slot[i].state = SLOT_OPEN;
		scoreboard->slot[i].pid = 0;
		scoreboard->slot[i].pthread = -1;
		scoreboard->slot[i].start_time = 0;
		scoreboard->slot[i].recent_request = 0;
		scoreboard->slot[i].main = NULL;
		scoreboard->slot[i].name[0] = '\0';
	}

	return SUCCESS;
}

/*****************************************************************************/
static void register_hooks(void)
{
	sb_hook_set_default_sighandlers(set_default_sighandlers,NULL,NULL,HOOK_MIDDLE);/*{{{*/
	sb_hook_init_scoreboard(init_scoreboard,NULL,NULL,HOOK_MIDDLE);

	sb_hook_spawn_processes(spawn_processes,NULL,NULL,HOOK_MIDDLE);
	sb_hook_monitor_processes(monitor_processes,NULL,NULL,HOOK_MIDDLE);

	sb_hook_spawn_threads(spawn_threads,NULL,NULL,HOOK_MIDDLE);
	sb_hook_monitor_threads(monitor_threads,NULL,NULL,HOOK_MIDDLE);

	//sb_hook_spawn_process(spawn_process,NULL,NULL,HOOK_MIDDLE);
	sb_hook_spawn_processes_for_each_module(spawn_processes_for_each_module,NULL,NULL,HOOK_MIDDLE);
	sb_hook_spawn_process_for_module(spawn_process_for_module,NULL,NULL,HOOK_MIDDLE);
	sb_hook_monitor_processes_for_modules(monitor_processes_for_modules,NULL,NULL,HOOK_MIDDLE);
}

module mp_module = {
	STANDARD_MODULE_STUFF,
	NULL,					/* config */
	NULL,					/* registry */
	NULL,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};


