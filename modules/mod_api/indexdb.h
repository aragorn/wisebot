#ifndef _INDEXDB_H_
#define _INDEXDB_H_

#include "hook.h"

SB_DECLARE_HOOK(void*, indexdb_create, ())
SB_DECLARE_HOOK(int, indexdb_destroy, (void* indexdb))
SB_DECLARE_HOOK(int, indexdb_open, (void* indexdb, int opt))
SB_DECLARE_HOOK(int, indexdb_close, (void* indexdb))
SB_DECLARE_HOOK(int, indexdb_append, (void* indexdb, int file_id, int size, void* buf))

#define INDEXDB_FILE_NOT_EXISTS (-2)
SB_DECLARE_HOOK(int, indexdb_read, (void* indexdb, int file_id, int offset, int size, void* buf))
SB_DECLARE_HOOK(int, indexdb_getsize, (void* indexdb, int file_id))

#endif

