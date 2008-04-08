/* $Id$ */
#include "common_util.h"
/* moved to common_core.h due to precompiled header 
#include <ctype.h>      // strtoupper(),strtolower()
#include <sys/socket.h> // connect_nonb() 
#include <fcntl.h>      // connect_nonb() 

#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include "common_core.h"
#include "log_error.h"
*/

/*****************************************************************************/
/* Lock a part of a file */
int flock(int fd, int locktype, int whence, int start, int length)
{
	struct flock lock;
	int value;

	lock.l_type = (short)locktype;
	lock.l_whence = whence;
	lock.l_start = (long)start;
	lock.l_len = (long)length;

	if (fcntl(fd, F_SETLK, &lock) != -1)
		return 1;

	while ((value = fcntl(fd, F_SETLKW, &lock)) &&
			errno == EINTR);

	if (value != -1) return 1;
	if (errno == EINTR) errno = EAGAIN;

	/* We got an error. We don't want EACCES errors */
	errno = (errno == EACCES) ? EAGAIN : errno ? errno : -1;
	return -1;
}
/*****************************************************************************/
char *sz_strncat(char *dest, const char *src, size_t n)
{
	char *buf=NULL;

	if (n < 0) {
		warn("n is %d. setting it 0", (int)n);
		n = 0;
	}

	if (strlen(src) >= n) {
		int orig_len = strlen(dest);
		int n2=0;

		n2 = n-1;
		buf = strncat(dest, src, n2);
		dest[orig_len+n-1] = '\0';
	}
	else {
		buf = strncat(dest, src, n);
	}


	return buf;
}

char *sz_strncpy(char *dest, const char *src, size_t n)
{
	char *buf=NULL;

	if (n < 0) {
		warn("n is %d. setting it 0", (int)n);
		n = 0;
	}

	buf = strncpy(dest, src, n);
	dest[n-1] = '\0';

	return buf;
}

int sz_snprintf(char *str, size_t size, const char *format, ...)
{
	int rv=0;
	va_list args;

	if (size < 0) {
		warn("size is %d. setting it 0", (int)size);
		size = 0;
	}

	va_start(args, format);
	rv = vsnprintf(str, size, format, args);
	va_end(args);

	str[size-1] = '\0';

	return rv;
}
/*****************************************************************************/
char *strtoupper(char *str)
{
	char *tmp = str;
	for ( ; *tmp; tmp++) {
		*tmp = toupper(*tmp);
	}
	return str;
}

char *strtolower(char *str)
{
	char *tmp = str;
	for ( ; *tmp; tmp++) {
		*tmp = tolower(*tmp);
	}
	return str;
}

char *strntoupper(char *str, int size)
{
	char *tmp = str;
	int i=0;
	for (i=0; *tmp && i<size; tmp++, i++) {
		*tmp = toupper(*tmp);
	}
	return str;
}

char *strntolower(char *str, int size)
{
	char *tmp = str;
	int i=0;
	for (i=0; *tmp && i<size; tmp++, i++) {
		*tmp = tolower(*tmp);
	}
	return str;
}

char* strnhcpy(char* dest, char const* src, int len)
{
    int i = 0, j, last;
    do {
        if(src[i] == 0) return dest[i] = 0, dest;
        last = ((unsigned char)src[i] >= 0x80 &&
                (unsigned char)src[i+1] >= 0x30) ? 2 : 1;
        for(j = 0; j < last; i++, j++) dest[i] = src[i];
    } while(i < len && src[i] != 0);
    if(i > len) i -= last;
    dest[i] = 0;
    return dest;
} 

/*
 * NOTICE:
 * The comparison function must return an integer less than, equal to, or 
 * greater
 * than  zero  if  the first argument is considered to be respectively
 * less than, equal to, or greater than the second.  If two members compare
 * as equal,  their order in the sorted array is undefined.
 */
//#define _IS_HANGUL(c)	((0xb0 <= c) && (c <= 0xfe))
#define _IS_HANGUL(c)    ((0xb0 <= (unsigned char)c) && ((unsigned char)c <= 0xfe))
int hangul_strncmp(char *str1, char *str2, int size)
{
	int i, diff;

	for ( i = 0; i < size; ) {
		if ( str1[i] && !str2[i]) return  1;
		if (!str1[i] &&  str2[i]) return -1;
		if (!str1[i] && !str2[i]) return  0;

		if (_IS_HANGUL(str1[i]) && _IS_HANGUL(str2[i])) {
			if (i + 1 == size) { return str1[i] - str2[i]; }

			diff = strncmp(str1 + i, str2 + i, 2);
			if (diff != 0) return diff;
			i += 2;
		}
		else if (_IS_HANGUL(str1[i])) {
			return -1;
		}
		else if (_IS_HANGUL(str2[i])) {
			return 1;
		}
		else {
			diff = str1[i] - str2[i];
			if (diff != 0) return diff;
			i += 1;
		}
	}
	return 0;
}
/*****************************************************************************/

/**
 * path 에 해당하는 파일을 만들고 lock을 건다.
 *
 * RETURN VALUE
 *  생성된 파일의 fd
 *  에러시 FAIL(-1)
 **/
int sb_lockfile(const char *path)
{
	int fd;
	struct flock lock;

	fd = sb_open( path, O_CREAT|O_TRUNC|O_RDWR, 0666 );
	if ( fd == -1 ) {
		error("lockfile[%s] open failed: %s", path, strerror(errno));
		return FAIL;
	}

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = (long)0;
	lock.l_len = (long)0;

	if ( fcntl( fd, F_SETLK, &lock ) != 0 ) {
		error("lock failed: %s", strerror(errno));
		close( fd );
		return FAIL;
	}

	return fd;
}

/***
 * sb_lockfile 로 잠근것을 해제한다
 *
 * RETURN VALUE
 *  SUCCESS/FAIL
 **/
int sb_unlockfile(int fd)
{
	struct flock lock;

	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = (long)0;
	lock.l_len = (long)0;

	if ( fcntl(fd, F_SETLK, &lock) != 0 ) {
		error("file unlock failed: %s", strerror(errno));
		close(fd);
		return FAIL;
	}

	close(fd);
	return SUCCESS;
}


/*****************************************************************************/

int
connect_nonb(int sockfd, const struct sockaddr *saptr, socklen_t salen, int nsec)
{
	int			flags, n, conn_error;
	socklen_t	len;
	fd_set		rset, wset;
	struct timeval	tval;

	flags = fcntl(sockfd, F_GETFL, 0);
	if ( flags == -1 ) {
		error("fcntl: %s", strerror(errno));
		return FAIL;
	}
	n = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
	if ( n == -1 ) {
		error("fcntl: %s", strerror(errno));
		return FAIL;
	}

	conn_error = 0;
	if ( (n = connect(sockfd, saptr, salen)) < 0 )
#ifdef AIX5
		if (!(errno == EINPROGRESS || errno == EEXIST || errno == 17)) {
#else
		if (errno != EINPROGRESS) {
#endif
			error("connect: %s", strerror(errno));
			return FAIL;
		}

	/* connect(2) is under progress */

	if ( n == 0 ) goto done;	/* connect(2) completed immediately */

	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	wset = rset;
	tval.tv_sec = nsec;
	tval.tv_usec = 0;

	if ( (n = select(sockfd + 1, &rset, &wset, NULL,
						nsec ? &tval : NULL)) == 0 ) {
		close(sockfd);
		warn("select timedout(timeout = %d)", nsec);
		errno = ETIMEDOUT;
		return FAIL;
	} else if ( n == -1 ) {
		error("select: %s", strerror(errno));
		return FAIL;
	}

	if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
		len = sizeof(conn_error);
		// refer to connect(2) EINPROGRESS error part
		if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &conn_error, &len) < 0) {
			error("getsockopt: %s", strerror(errno));
			return FAIL;
		}
	} else {
		error("select: sockfd not set");
		return FAIL;
	}

  done:
	n = fcntl(sockfd, F_SETFL, flags); /* restore file status flags */
	if ( n == -1 ) {
		error("fcntl: %s", strerror(errno));
		return FAIL;
	}

	if (conn_error) {
		close(sockfd);
		errno = conn_error;
		error("%s", strerror(errno));
		return FAIL;
	}

	return SUCCESS;
}



/*****************************************************************************/

int sb_tstat_start(tstat_t *tstat) {
	if ((tstat->cs = times(&(tstat->tms_s))) == (clock_t)-1) {
		error("%s", strerror(errno));
		return -1;
	}
	if (gettimeofday(&(tstat->tv_s), NULL) == -1) {
		error("%s", strerror(errno));
		return -1;
	}
	return 1;
}

int sb_tstat_finish(tstat_t *tstat) {
	if ((tstat->cf = times(&(tstat->tms_f))) == (clock_t)-1) {
		error("%s", strerror(errno));
		return -1;
	}
	if (gettimeofday(&(tstat->tv_f), NULL) == -1) {
		error("%s", strerror(errno));
		return -1;
	}
	return 1;
}

void sb_tstat_print(tstat_t *tstat) {
	clock_t clock_per_second;
/*	printf("gauged by gettimeofday(): Realtime:%d(ms)", */
/*			(tstat->tv_f.tv_sec - tstat->tv_s.tv_sec)*1000 +*/
/*			(tstat->tv_f.tv_usec - tstat->tv_s.tv_usec)/1000);*/
	info("gauged by gettimeofday(): Realtime:%d(s)", 
			(int)(tstat->tv_f.tv_sec - tstat->tv_s.tv_sec));

	clock_per_second = sysconf(_SC_CLK_TCK);
	info("gauged by times(): Realtime:%.2f(s), User Time %.2f(s), System Time %.2f(s)\n",
			(float)(tstat->cf - tstat->cs) / clock_per_second,
			(float)(tstat->tms_f.tms_utime - tstat->tms_s.tms_utime) / clock_per_second,
			(float)(tstat->tms_f.tms_stime - tstat->tms_s.tms_stime) / clock_per_second);
}

static FILE *fp_log = NULL;

int sb_tstat_log_init(const char* file_name) {
    if ((fp_log = fopen(file_name, "a")) == NULL) {
        crit("cannot open time log file %s: %s", file_name, strerror(errno));
        return FAIL;
    }
    setlinebuf(fp_log);

    return SUCCESS;
}

void sb_tstat_log_destroy() {
    if(fp_log != NULL) {
        fclose(fp_log);
    }
}



/* pid tag gettimeofday(ms) times(ms) usertime(ms) systemtime(ms) */
void sb_tstat_log(char *tag) {
	clock_t c;
	struct timeval tv;
	struct tms t;
	const float clock_per_millisecond = (float)sysconf(_SC_CLK_TCK) / 1000.;

	if (fp_log == NULL) return;

	gettimeofday(&tv, NULL);
	c = times(&t);

	fprintf(fp_log, "%d %s %.3f ",
			getpid(), tag, (float)tv.tv_sec * 1000. + (float)tv.tv_usec / 1000.);

	fprintf(fp_log, "%.3f %.3f %.3f\n",
			(float)c           / clock_per_millisecond,
			(float)t.tms_utime / clock_per_millisecond,
			(float)t.tms_stime / clock_per_millisecond);
}

