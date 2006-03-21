/* $id$ */
#include "common_core.h"
#include "indexdb.h"

HOOK_STRUCT(
    HOOK_LINK(indexdb_open)
    HOOK_LINK(indexdb_close)
    HOOK_LINK(indexdb_append)
    HOOK_LINK(indexdb_read)
    HOOK_LINK(indexdb_getsize)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, indexdb_open, (index_db_t** indexdb, int opt),(indexdb, opt),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, indexdb_close, (index_db_t* indexdb),(indexdb),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, indexdb_append, (index_db_t* indexdb, int file_id, int size, void* buf),(indexdb, file_id, size, buf),MINUS_DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, indexdb_read, (index_db_t* indexdb, int file_id, int offset, int size, void* buf),(indexdb, file_id, offset, size, buf),MINUS_DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, indexdb_getsize, (index_db_t* indexdb, int file_id),(indexdb, file_id),MINUS_DECLINE)

