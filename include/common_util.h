/* $Id$ */
#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H 1

#ifndef COMMON_CORE_H
# error You should include "common_core.h" first.
#endif

#include <time.h>
#include <sys/times.h>
#include <sys/socket.h>  /* struct sockaddr */

/*****************************************************************************/
#ifndef HAVE_MERGESORT
SB_DECLARE(int) mergesort(void *base, size_t nmemb, size_t size, \
				int (*compar)(const void *, const void *));
#endif
/*****************************************************************************/
#ifndef HAVE_QSORT2 
SB_DECLARE(void) qsort2(void *a, size_t n, size_t es, void *usr, \
						int (*cmp)(const void *, const void *, void *));
#endif
/*****************************************************************************/
#define rd_lock(a,b,c,d)		flock((a),F_RDLCK,(b),(c),(d))
#define wr_lock(a,b,c,d)		flock((a),F_WRLCK,(b),(c),(d))
#define un_lock(a,b,c,d)		flock((a),F_UNLCK,(b),(c),(d))

SB_DECLARE(int) flock(int fd, int locktype, int whence, int start, int length);
/*****************************************************************************/
/* Zero terminated string functions.
 *
 * Be careful when you use this,because it's differnt from original function
 * in that it overwrite '\0' at the end.
 */
SB_DECLARE(char*) sz_strncat(char *dest, const char *src, size_t n);
SB_DECLARE(char*) sz_strncpy(char *dest, const char *src, size_t n);
SB_DECLARE(int) sz_snprintf(char *str, size_t size, const char *format, ...);
/*****************************************************************************/
SB_DECLARE(char*) strtoupper(char *str);
SB_DECLARE(char*) strtolower(char *str);

SB_DECLARE(char*) strntoupper(char *str, int size);
SB_DECLARE(char*) strntolower(char *str, int size);

/* 한글이 깨어지지 않게 문자열을 복사한다. len보다 짧게 문자열을 복사하고,
 * 마지막에 항상 NULL을 붙여준다. */
SB_DECLARE(char*) strnhcpy(char* dest, char const* src, int len);
/*
 * NOTICE:
 * The comparison function must return an integer less than, equal to, or 
 * greater
 * than  zero  if  the first argument is considered to be respectively
 * less than, equal to, or greater than the second.  If two members compare
 * as equal,  their order in the sorted array is undefined.
 */
SB_DECLARE(int) hangul_strncmp(unsigned char *str1, unsigned char *str2, int size);
/*****************************************************************************/
SB_DECLARE(int) sb_lockfile(const char *path);
SB_DECLARE(int) sb_unlockfile(int fd);
/*****************************************************************************/
SB_DECLARE(int) connect_nonb(int sockfd, const struct sockaddr *saptr, socklen_t salen, int nsec);
/*****************************************************************************/
typedef struct {
	clock_t cs, cf;
	struct tms tms_s, tms_f;
	struct timeval tv_s, tv_f;
} tstat_t;

SB_DECLARE(int)  sb_tstat_start(tstat_t *tstat);
SB_DECLARE(int)  sb_tstat_finish(tstat_t *tstat);
SB_DECLARE(void) sb_tstat_print(tstat_t *tstat);
SB_DECLARE(int)  sb_tstat_log_init(const char* file_name);
SB_DECLARE(void) sb_tstat_log_destroy();
SB_DECLARE(void) sb_tstat_log(char* tag);
/*****************************************************************************/

#endif

