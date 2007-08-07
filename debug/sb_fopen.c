/* $Id$ */
#include "common_core.h"
#include <string.h>
#include <errno.h>

char gSoftBotRoot[MAX_PATH_LEN] = SERVER_ROOT;
module *static_modules[1];

int main(int argc, char *argv[])
{
	int i;
	FILE *fp;

	for (i = 1; i < argc; i++)
	{
		fp = sb_fopen(argv[i], "r");
		if ( fp == NULL )
			error("sb_fopen(\"%s\", \"r\") failed: %s",
					argv[i], strerror(errno));
		else {
			info("sb_fopen(\"%s\", \"r\") succeeded", argv[i]);
			fclose(fp);
		}
	}
	printf("%s done.\n", argv[0]);

	return 0;
}


