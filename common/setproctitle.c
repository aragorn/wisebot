/* $Id$ */
#include <string.h>
#include <stdlib.h> /* malloc(3) */
#define CORE_PRIVATE 1
#include "common_core.h"
#include "setproctitle.h"

#define DEFAULT_PROCNAME ""

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

//SB_DECLARE_DATA extern char **g_argv;
//SB_DECLARE_DATA extern char *g_lastArgv;

static char **g_argv = NULL;
static char *g_lastArgv = NULL;


void init_setproctitle(int argc, char *argv[], char *envp[])
{
#ifdef HAVE___PROGNAME
    extern char *__progname, *__progname_full;
#endif
    extern char **environ; /* see environ(7) */

    int i, envpsize;
    char **p;

    /* move the environment so setproctitle can use the space.
     */
    /* 1. malloc for the array of pointer. */
    for(i = envpsize = 0; envp[i] != NULL; i++)
        envpsize += strlen(envp[i]) + 1;

    /* 2. malloc for the string of each envp and copy the value. */
    if((p = (char **) malloc((i + 1) * sizeof(char *))) != NULL ) {
        environ = p;

        for(i = 0; envp[i] != NULL; i++) {
            if((environ[i] = malloc(strlen(envp[i]) + 1)) != NULL)
                strcpy(environ[i], envp[i]);
        }

        environ[i] = NULL;
    }

    g_argv = argv;
    for(i = 0; i < argc; i++) {
        if(i == 0 || (g_lastArgv + 1 == argv[i]))
            g_lastArgv = argv[i] + strlen(argv[i]);
            /* g_lastArgv points to the last null character of the last argv[i],
               while argv[i] array has continuous memory block. */
    }

    for(i = 0; envp[i] != NULL; i++) {
        if((g_lastArgv + 1) == envp[i])
            g_lastArgv = envp[i] + strlen(envp[i]);
            /* g_lastArgv points to the last null character of the last envp[i],
               while envp[i] array has continuous memory block. */
    }

#ifdef HAVE___PROGNAME
    /* set the __progname and __progname full variables so glibc and company 
	 * don't go nuts. - MacGyver
     */
    __progname = strdup("softbot");
    __progname_full = strdup(argv[0]);
#endif /* HAVE___PROGNAME */
}

static char prefix[SHORT_STRING_SIZE] = "";

void setproctitle_prefix(char *s)
{
	strncpy(prefix, s, SHORT_STRING_SIZE);
}

#ifndef HAVE_SETPROCTITLE

void setproctitle(char *fmt, ...)
{
	va_list msg;
	static char statbuf[BUFSIZ], statbuf2[BUFSIZ];

#if SB_ARGV_TYPE == SB_ARGV_PSTAT
	union pstun pst;
#endif /* SB_ARGV_PSTAT */
	int i;
#if SB_ARGV_TYPE == SB_ARGV_WRITABLE	
	char *p;
	int maxlen = (g_lastArgv - g_argv[0]) -2;
#endif /* SB_ARGV_WRITABLE */

	va_start(msg,fmt);
	memset(statbuf2, 0, sizeof(statbuf2));
	vsnprintf(statbuf2, sizeof(statbuf2), fmt, msg);
	va_end(msg);

	if ( strlen(prefix) > 0 )
		snprintf(statbuf, sizeof(statbuf), "%s - %s", prefix, statbuf2);
	else strncpy(statbuf, statbuf2, sizeof(statbuf));

	i = strlen(statbuf);

#if SB_ARGV_TYPE == SB_ARGV_NEW
	/* we can just replace argv[] arguments. nice and easy.
	 */
	g_argv[0] = statbuf;
	g_argv[1] = NULL;
#endif /* SB_ARGV_NEW */
#if SB_ARGV_TYPE == SB_ARGV_WRITABLE
	/* we can overwrite individual argv[] arguments. semi-nice.
	 */
	snprintf(g_argv[0], maxlen, "%s", statbuf);

	/* wipe out the empty space of g_argv[0], because the garbage of empty
	   space would be printed in ps(1). */
	p = &g_argv[0][i];
	while(p < g_lastArgv)
		*p++ = '\0';

	/* g_argv[0] is the name of this program, g_argv[1] is the first argument.
	   g_argv[1] is set by NULL and the program looks like it has no argument. */
	g_argv[1] = NULL;
#endif /* SB_ARGV_WRITABLE */
	
#if SB_ARGV_TYPE == SB_ARGV_PSTAT
	pst.pst_command = statbuf;
	pstat(PSTAT_SETCMD, pst, i, 0, 0);
#endif /* SB_ARGV_PSTAT */

#if SB_ARGV_TYPE == SB_ARGV_PSSTRINGS
	PS_STRINGS->ps_nargvstr = 1;
	PS_STRINGS->ps_argvstr = statbuf;
#endif /* SB_ARGV_PSSTRINGS */
}

#endif /* HAVE_SETPROCTITLE */
