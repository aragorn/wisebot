/* $Id$ */
#include "softbot.h"
#include "util.h"

const char *sb_get_server_version(void)
{
	return PACKAGE_VERSION;
}

int sb_server_root_relative(char *buf, const char *filename)
{
	if (buf == NULL || filename == NULL)
		return FAIL;

	/* length of buf should be longer than MAX_PATH_LEN */
	if (filename[0] == '/')
		snprintf(buf, MAX_PATH_LEN, "%s", filename);
	else
		snprintf(buf, MAX_PATH_LEN, "%s/%s", gSoftBotRoot, filename);

	return SUCCESS;
}

/* XXX: not thread safe */
char* sb_strbin(uint32_t number, int size)
{
    static char strbin[64+1];
    int bitsize=size*8;
    int i=0;
    for (i=0; i<bitsize; i++) {
        strbin[i] = "01"[(number >> (bitsize-i-1))&1];
    }
    strbin[i]='\0';

    return strbin;
}


