#ifndef _INDEXDB_H_
#define _INDEXDB_H_

#include "hook.h"

typedef struct _index_db_t {
	int set;
	void* db;
} index_db_t;

#define MAX_INDEXDB_SET (10)

SB_DECLARE_HOOK(int, indexdb_open, (index_db_t** indexdb, int opt))
SB_DECLARE_HOOK(int, indexdb_close, (index_db_t* indexdb))
SB_DECLARE_HOOK(int, indexdb_append, (index_db_t* indexdb, int file_id, int size, void* buf))

#define INDEXDB_FILE_NOT_EXISTS (-2)
SB_DECLARE_HOOK(int, indexdb_read, (index_db_t* indexdb, int file_id, int offset, int size, void* buf))
SB_DECLARE_HOOK(int, indexdb_getsize, (index_db_t* indexdb, int file_id))

#endif

