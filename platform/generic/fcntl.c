/* $Id$ */
#include "sb_fcntl.h"

int
sb_fcntl(int fd, int cmd, struct flock *lock)
{
	int flags;

	flags = fcntl(fd, cmd, lock);
	if ( flags == -1 ) {
		error("fcntl: %s", strerror(errno));
		return FAIL;
	}

	return flags;
}
