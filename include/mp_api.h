/* $Id$ */
#ifndef MP_API_H
#define MP_API_H

//#include <pthread.h>

/* waitpid stuff - XXX autoconf */
//#include <sys/types.h>
//#include <sys/wait.h>

#if 0
SB_DECLARE_HOOK(int,monitor_threads,(scoreboard_t *scoreboard))
SB_DECLARE_HOOK(int,spawn_process,\
		(slot_t *slot, const char *name, int (*main)(slot_t *)))
SB_DECLARE_HOOK(int,spawn_thread,\
		(slot_t *slot, const char *name, int (*main)(slot_t *)))
SB_DECLARE_HOOK(int,spawn_threads,\
		(scoreboard_t *scoreboard, const char *name, int (*main)(slot_t *)))
#endif

/* for modules */

SB_DECLARE_HOOK(int,set_default_sighandlers,\
		(void (*shutdown_handler)(int), void (*graceful_shutdown_handler)(int)))
SB_DECLARE_HOOK(int,init_scoreboard, (scoreboard_t *s))
SB_DECLARE_HOOK(int,spawn_processes, (scoreboard_t *s, const char *name, int (*main)(slot_t *)))
SB_DECLARE_HOOK(int,monitor_processes, (scoreboard_t *s))


/* for server */
SB_DECLARE_HOOK(int,spawn_processes_for_each_module, (scoreboard_t *s, module *mod))
SB_DECLARE_HOOK(int,spawn_process_for_module, (scoreboard_t *s, module *mod))
SB_DECLARE_HOOK(int,monitor_processes_for_modules, (scoreboard_t *s, module *first_mod))


#endif
