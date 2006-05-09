#ifndef _MOD_CDM2_LOCK_H_
#define _MOD_CDM2_LOCK_H_

#include <fcntl.h>
#include <errno.h>
#include <string.h>

/* lock for index file */
#define INDEX_RD_LOCK(db, action) \
	if ( !(db)->locked ) { \
		if ( rd_lock((db)->fdIndexFile, SEEK_SET, 0, 0) == -1 ) { \
			error("error flock[%d] index file: %s", (db)->fdIndexFile, strerror(errno)); \
			action; \
		} \
		else (db)->locked = 1; \
	} \
	else warn("already have lock");

#define INDEX_WR_LOCK(db, action) \
	if ( !(db)->locked ) { \
		if ( wr_lock((db)->fdIndexFile, SEEK_SET, 0, 0) == -1 ) { \
			error("error flock[%d] index file: %s", (db)->fdIndexFile, strerror(errno)); \
			action; \
		} \
		else (db)->locked = 1; \
	} \
	else warn("already have lock");

#define INDEX_UN_LOCK(db, action) \
	if ( (db)->locked ) { \
		if ( un_lock((db)->fdIndexFile, SEEK_SET, 0, 0) == -1 ) { \
			error("unlock index file failed: %s", strerror(errno)); \
			action; \
		} \
		else (db)->locked = 0; \
	} \
	else warn("have no lock");

#endif // _MOD_CDM2_LOCK_H_

