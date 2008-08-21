/* $Id$ */
#include "common_core.h"
#include <stdio.h>

int child_main_test1(void)
{
	info("test1 started.");

	return TRUE;
}

int main(int argc, char *argv[])
{
	int pid;
	int (*child_main)(void);

	printf("hello, world!\n");
	if ( (pid = fork()) == 0 )
	{ /* child */
		info("child process");
		;
	} else
	if ( pid > 0 )
	{ /* parent */
		info("parent process");
		;
	} else
	{
		error("cannot fork: %s", strerror(errno));
	}
	

	return EXIT_SUCCESS;
}
