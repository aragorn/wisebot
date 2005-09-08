/* $Id$ */
#ifndef SOFTBOT_H
#define SOFTBOT_H 1

#ifdef WIN32
# include "win32_config.h"
# ifdef _DEBUG
#  define DEBUG_SOFTBOTD  1
# else
#  undef  DEBUG_SOFTBOTD
# endif
#else // #ifdef WIN32
# include "auto_config.h" /* autoconf configuration stuffs */
# define DEBUG_SOFTBOTD // undef DEBUG when you release softbotd
//# undef  DEBUG_SOFTBOTD // undef DEBUG when you release softbotd
#endif // #ifdef WIN32

#define SB_DECLARE(type)	type
#define SB_DECLARE_DATA	

#ifdef AIX5

/* XXX AIX Porting FAQ 등을 좀 더 찾아볼 것. */
//#define UNIX98  /* this makes _XOPEN_SOURCE defined as 500 */

/* XXX install iconv library from srclib : make iconv-install
 *     we declare some iconv functions to be exported in server/sb.exp .
 */
#include "iconv/iconv.h"
SB_DECLARE(iconv_t) iconv_open (const char* tocode, const char* fromcode);
SB_DECLARE(size_t) iconv(iconv_t cd,  char* * inbuf, size_t *inbytesleft, char* * outbuf, size_t *outbytesleft);
SB_DECLARE(int) iconv_close(iconv_t cd);

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
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

#include <sys/types.h>
#ifdef WIN32           /* The windows equivalent to unistd.h is io.h .*/
# include <io.h>
#else
# include <unistd.h>
#endif
#include <stdarg.h>

#ifdef HAVE_INTTYPES_H /* for uint32_t, int8_t, .., etc supporting */
# include <inttypes.h>
#else
# //warn 'no inttypes.h'
#endif

#ifdef HAVE_GETOPT_H   /* for command-line argument supporting */
# include <getopt.h>
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
