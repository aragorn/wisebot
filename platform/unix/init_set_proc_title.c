/* $Id$ */
#include "setproctitle.h"

#include <stdlib.h>

char **g_argv = NULL;
char *g_lastArgv = NULL;

void init_set_proc_title(int argc, char *argv[], char *envp[])
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
