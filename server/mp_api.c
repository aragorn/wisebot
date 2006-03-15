/* $Id$ */

#include "common_core.h"
#include "hook.h"
#include "log_error.h"
#include "mp_api.h"

HOOK_STRUCT(
	HOOK_LINK(set_default_sighandlers)
	HOOK_LINK(init_scoreboard)
	HOOK_LINK(spawn_processes)
	HOOK_LINK(monitor_processes)

	HOOK_LINK(spawn_processes_for_each_module)
	HOOK_LINK(spawn_process_for_module)
	HOOK_LINK(monitor_processes_for_modules)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, set_default_sighandlers,\
		(void (*shutdown_handler)(int), void (*graceful_shutdown_handler)(int)),\
		(shutdown_handler,graceful_shutdown_handler),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, init_scoreboard, (scoreboard_t *s),(s),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, spawn_processes,\
		(scoreboard_t *s, const char *name, int (*main)(slot_t *)),(s, name, main),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, monitor_processes, (scoreboard_t *s),(s),DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, spawn_processes_for_each_module,\
		(scoreboard_t *s, module *mod),(s, mod),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, spawn_process_for_module,\
		(scoreboard_t *s, module *mod),(s, mod),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, monitor_processes_for_modules,\
		(scoreboard_t *s, module *first_mod),(s, first_mod),DECLINE)


