/* $Id$ */
#ifndef SETPROCTITLE_H
#define SETPROCTITLE_H

#include "auto_config.h"
#include "softbot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_PROCNAME ""

extern char proc_name[STRING_SIZE];

#ifdef HAVE_SETPROCTITLE
#  include <sys/types.h>
#  include <unistd.h>
#else
#  include <stdarg.h>
SB_DECLARE(void) setproctitle(char *fmt, ...);
#endif

#if PF_ARGV_TYPE == PF_ARGV_PSTAT
# ifdef HAVE_SYS_PSTAT_H
#  include <sys/pstat.h>
# else
#  undef PF_ARGV_TYPE
#  define PF_ARGV_TYPE PF_ARGV_WRITEABLE
# endif /* HAVE_SYS_PSTAT_H */
#endif /* PF_ARGV_PSTAT */

#if PF_ARGV_TYPE == PF_ARGV_PSSTRINGS
# ifndef HAVE_SYS_EXEC_H
#  undef PF_ARGV_TYPE
#  define PF_ARGV_TYPE PF_ARGV_WRITEABLE
# else
#  include <machine/vmparam.h>
#  include <sys/exec.h>
# endif /* HAVE_SYS_EXEC_H */
#endif /* PF_ARGV_PSSTRINGS */

SB_DECLARE_DATA extern char **g_argv;
SB_DECLARE_DATA extern char *g_lastArgv;
SB_DECLARE(void) init_set_proc_title(int argc, char *argv[], char *envp[]);

#endif
