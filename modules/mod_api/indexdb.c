#include "indexdb.h"

HOOK_STRUCT(
    HOOK_LINK(indexdb_create)
    HOOK_LINK(indexdb_destroy)
    HOOK_LINK(indexdb_open)
    HOOK_LINK(indexdb_close)
    HOOK_LINK(indexdb_append)
    HOOK_LINK(indexdb_read)
    HOOK_LINK(indexdb_getsize)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(void*, indexdb_create, (),(),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, indexdb_destroy, (void* indexdb),(indexdb),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, indexdb_open, (void* indexdb, int opt),(indexdb, opt),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, indexdb_close, (void* indexdb),(indexdb),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, indexdb_append, (void* indexdb, int file_id, int size, void* buf),(indexdb, file_id, size, buf),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, indexdb_read, (void* indexdb, int file_id, int offset, int size, void* buf),(indexdb, file_id, offset, size, buf),MINUS_DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, indexdb_getsize, (void* indexdb, int file_id),(indexdb, file_id),MINUS_DECLINE)

