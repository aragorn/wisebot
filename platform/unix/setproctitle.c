/* $Id$ */
#include "setproctitle.h"

char proc_name[STRING_SIZE] = "";

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
#endif

	va_start(msg,fmt);
	memset(statbuf2, 0, sizeof(statbuf2));
	vsnprintf(statbuf2, sizeof(statbuf2), fmt, msg);
	va_end(msg);

	if ( strlen(proc_name) > 0 )
		snprintf(statbuf, sizeof(statbuf), "%s - %s", proc_name, statbuf2);
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
#endif
