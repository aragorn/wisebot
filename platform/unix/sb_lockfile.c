#include "sb_lockfile.h"
#include <fcntl.h>

/****************************************************
 * path �� �ش��ϴ� ������ ����� lock�� �Ǵ�.
 *
 * RETURN VALUE
 *  ������ ������ fd
 *  ������ FAIL(-1)
 ****************************************************/
SB_DECLARE(int) sb_lockfile(const char *path)
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

/**********************************************************
 * sb_lockfile �� ��ٰ��� �����Ѵ�
 *
 * RETURN VALUE
 *  SUCCESS/FAIL
 **********************************************************/
SB_DECLARE(int) sb_unlockfile(int fd)
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

