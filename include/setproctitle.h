/* $Id$ */
#ifndef SETPROCTITLE_H
#define SETPROCTITLE_H

#ifndef COMMON_CORE_H
# error You should include "common_core.h" first.
#endif

#define DEFAULT_PROCNAME ""

#ifdef HAVE_SETPROCTITLE
#  include <sys/types.h>
#  include <unistd.h>
#else
#  include <stdarg.h>
SB_DECLARE(void) setproctitle(char *fmt, ...);
#endif

SB_DECLARE(void) init_setproctitle(int argc, char *argv[], char *envp[]);
SB_DECLARE(void) setproctitle_prefix(char *prefix);

#endif /* SETPROCTITLE_H */
