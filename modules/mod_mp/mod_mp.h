/* $Id$ */
#ifndef _MOD_MP_H_
#define _MOD_MP_H_

#include "softbot.h"
#include <pthread.h>

/* waitpid stuff - XXX autoconf */
#include <sys/types.h>
#include <sys/wait.h>

#define MONITORING_SCOREBOARD_PERIOD (3)

#define INIT_THREAD_SCOREBOARD(scoreboard,size) \
		sb_first_init_scoreboard(scoreboard,__FILE__,TYPE_THREAD,size)
#define INIT_PROCESS_SCOREBOARD(scoreboard,size) \
		sb_first_init_scoreboard(scoreboard,__FILE__,TYPE_PROCESS,size)

SB_DECLARE(int) sb_set_sighandlers(int signum, void (*handler)(int) );

SB_DECLARE(int) sb_run_set_default_sighandlers (
		void (*shutdown_handler)(int), void (*graceful_shutdown_handler)(int));
SB_DECLARE(int) sb_set_default_sighandlers (
		void (*shutdown_handler)(int), void (*graceful_shutdown_handler)(int));

SB_DECLARE(int) sb_run_init_scoreboard (scoreboard_t *scoreboard);
SB_DECLARE(int) sb_init_scoreboard (scoreboard_t *scoreboard);

SB_DECLARE(void) sb_run_first_init_scoreboard (
		scoreboard_t *scoreboard, char *name, int type, int slotnum);
SB_DECLARE(void) sb_first_init_scoreboard (
		scoreboard_t *scoreboard, char *name, int type, int slotnum);

SB_DECLARE(int) sb_run_monitor_processes(scoreboard_t *scoreboard);
SB_DECLARE(int) sb_monitor_processes(scoreboard_t *scoreboard);
SB_DECLARE(int) sb_monitor_processes_for_modules(scoreboard_t *scoreboard,
		module *first_module);

SB_DECLARE(int) sb_run_monitor_threads(scoreboard_t *scoreboard);
SB_DECLARE(int) sb_monitor_threads(scoreboard_t *scoreboard);

SB_DECLARE(int) sb_run_spawn_process (
		slot_t *slot, const char *name, int (*main)(slot_t *));
SB_DECLARE(int) sb_spawn_process (
		slot_t *slot, const char *name, int (*main)(slot_t *));

SB_DECLARE(int) sb_run_spawn_thread (
		slot_t *slot, const char *name, int (*main)(slot_t *));
SB_DECLARE(int) sb_spawn_thread (
		slot_t *slot, const char *name, int (*main)(slot_t *));

SB_DECLARE(int) sb_run_spawn_processes (
		scoreboard_t *scoreboard, 
		const char *name, int (*main)(slot_t *));
SB_DECLARE(int) sb_spawn_processes (
		scoreboard_t *scoreboard, const char *name, int (*main)(slot_t *));

SB_DECLARE(int) sb_run_spawn_threads (
		scoreboard_t *scoreboard, const char *name, int (*main)(slot_t *));
SB_DECLARE(int) sb_spawn_threads (
		scoreboard_t *scoreboard, const char *name, int (*main)(slot_t *));

SB_DECLARE(int) sb_run_spawn_processes_for_each_module(
		scoreboard_t *scoreboard, module *mod);
SB_DECLARE(int) sb_spawn_processes_for_each_module (
		scoreboard_t *scoreboard, module *mod);

SB_DECLARE(int) sb_run_spawn_process_for_module (
		scoreboard_t *scoreboard, module *mod);
SB_DECLARE(int) sb_spawn_process_for_module (
		scoreboard_t *scoreboard, module *mod);

#if 0
SB_DECLARE_HOOK(int,set_default_sighandlers,\
		(void (*shutdown_handler)(int), void (*graceful_shutdown_handler)(int)))
SB_DECLARE_HOOK(int,init_scoreboard,(scoreboard_t *scoreboard))
SB_DECLARE_HOOK(void,first_init_scoreboard,(scoreboard_t *scoreboard, \
		char *name, int type, int slotnum))
SB_DECLARE_HOOK(int,monitor_processes,(scoreboard_t *scoreboard))
SB_DECLARE_HOOK(int,monitor_threads,(scoreboard_t *scoreboard))
SB_DECLARE_HOOK(int,spawn_process,\
		(slot_t *slot, const char *name, int (*main)(slot_t *)))
SB_DECLARE_HOOK(int,spawn_thread,\
		(slot_t *slot, const char *name, int (*main)(slot_t *)))
SB_DECLARE_HOOK(int,spawn_processes,\
		(scoreboard_t *scoreboard, const char *name, int (*main)(slot_t *)))
SB_DECLARE_HOOK(int,spawn_threads,\
		(scoreboard_t *scoreboard, const char *name, int (*main)(slot_t *)))
SB_DECLARE_HOOK(int,spawn_processes_for_each_module,\
		(scoreboard_t *scoreboard, module *mod))
SB_DECLARE_HOOK(int,spawn_process_for_module,\
		(scoreboard_t *scoreboard, module *mod))
#endif

#endif
