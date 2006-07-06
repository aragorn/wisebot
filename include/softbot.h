/* $Id$ */
#ifndef SOFTBOT_H
#define SOFTBOT_H 1

#warning *** Using softbot.h is deprecated as of 2006/03/13. Please use common_core.h instead. ***

#define _GNU_SOURCE

#include "common_core.h"
#include "common_util.h"


#ifdef AIX5
/* XXX install iconv library from srclib : make iconv-install
 *     we declare some iconv functions to be exported in server/sb.exp .
 */
#  ifdef SRCLIB_ICONV
#    include "iconv/iconv.h"
#  elif defined(USR_LOCAL_INCLUDE_ICONV)
#    include "/usr/local/include/iconv.h"
#  else
#    include <iconv.h>
#  endif
SB_DECLARE(iconv_t) iconv_open (const char* tocode, const char* fromcode);
SB_DECLARE(size_t) iconv(iconv_t cd,  const char** inbuf, size_t *inbytesleft, char* * outbuf, size_t *outbytesleft);
SB_DECLARE(int) iconv_close(iconv_t cd);
#endif /* AIX5 */

#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
//#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32           /* The windows equivalent to unistd.h is io.h .*/
# include <io.h>
#else
# include <unistd.h>
#endif
#include <inttypes.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <pthread.h>

/* modules.h depends on registry.h and config.h and the order matters */
#include "registry.h"
#include "config.h"
#include "modules.h"
#include "scoreboard.h"

#include "log_error.h"
#include "ipc.h"
#include "memory.h"
#include "hook.h"
#include "timelog.h"
#include "util.h"
#include "setproctitle.h"
#include "md5.h"
#include "hash.h"

#endif
