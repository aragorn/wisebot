/* $Id$ */
#ifndef _SB_FLOCK_H_
#define _SB_FLOCK_H_ 1

#define rd_lock(a,b,c,d)		flock((a),F_RDLCK,(b),(c),(d))
#define wr_lock(a,b,c,d)		flock((a),F_WRLCK,(b),(c),(d))
#define un_lock(a,b,c,d)		flock((a),F_UNLCK,(b),(c),(d))

SB_DECLARE(int) flock(int fd, int locktype, int whence, int start, int length);
#endif
