/* $Id$ */
#ifndef SOFTBOT_H
#define SOFTBOT_H 1

#include "auto_config.h" /* autoconf configuration stuffs */

#define DEBUG_SOFTBOTD // undef DEBUG when you release softbotd
//#undef  DEBUG_SOFTBOTD // undef DEBUG when you release softbotd

#define SB_DECLARE(type)	type
#define SB_DECLARE_DATA	

#ifdef AIX5

/* XXX AIX Porting FAQ 등을 좀 더 찾아볼 것. */
//#define UNIX98  /* this makes _XOPEN_SOURCE defined as 500 */

/* XXX check /usr/local/include/iconv.h 
 *     we declare some iconv functions to be exported in server/sb.exp .
 */
#include "/usr/local/include/iconv.h"
SB_DECLARE(iconv_t) libiconv_open (const char* tocode, const char* fromcode);
SB_DECLARE(size_t) libiconv(iconv_t cd,  char* * inbuf, size_t *inbytesleft, char* * outbuf, size_t *outbytesleft);
SB_DECLARE(int) libiconv_close(iconv_t cd);

#include <pthread.h>
SB_DECLARE(int) pthread_mutex_init (pthread_mutex_t *, const pthread_mutexattr_t *);

SB_DECLARE(int) pthread_mutex_lock (pthread_mutex_t *);
SB_DECLARE(int) pthread_mutex_trylock (pthread_mutex_t *);

SB_DECLARE(int) pthread_mutex_unlock (pthread_mutex_t *);
SB_DECLARE(int) pthread_mutex_destroy (pthread_mutex_t *);
#endif

#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>

#ifdef HAVE_INTTYPES_H /* for uint32_t, int8_t, .., etc supporting */
#	include <inttypes.h>
#else
#	error
#endif

#ifdef HAVE_GETOPT_LONG /* for command-line argument supporting */
#	include <getopt.h>
#elif HAVE_GETOPT
#	include <unistd.h>
#else
#	error
#endif

#include "constants.h"

/* modules.h depends on registry.h and config.h and the order matters */
#include "registry.h"
#include "config.h"
#include "modules.h"

#include "log_error.h"
#include "ipc.h"
#include "memory.h"
#include "hook.h"

#include "platform.h" /* include all related files in platform/ directory */

#include "util.h"

extern SB_DECLARE_DATA char gSoftBotRoot[MAX_PATH_LEN];
extern SB_DECLARE_DATA char gErrorLogFile[MAX_PATH_LEN];
extern SB_DECLARE_DATA char gQueryLogFile[MAX_PATH_LEN];
extern SB_DECLARE_DATA char gRegistryFile[MAX_PATH_LEN];
extern SB_DECLARE_DATA pid_t gRootPid;

#endif
