/* $Id$ */
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <softbot.h>

/* Lock a part of a file */
SB_DECLARE(int) flock(int fd, int locktype, int whence, int start, int length)
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
