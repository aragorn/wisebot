/* $Id$ */
/* 
 * softbot.c
 * declare global variables.
 * --aragorn, 2005-07-07
 */

#include "softbot.h"

char  gSoftBotRoot[MAX_PATH_LEN]  = SERVER_ROOT;
char  gErrorLogFile[MAX_PATH_LEN] = DEFAULT_ERROR_LOG_FILE;
char  gQueryLogFile[MAX_PATH_LEN] = DEFAULT_QUERY_LOG_FILE;
pid_t gRootPid = 0;

