/* $Id$ */
#include "common_core.h"
#include "did.h"

HOOK_STRUCT(
	HOOK_LINK(open_did_db)
	HOOK_LINK(sync_did_db)
	HOOK_LINK(close_did_db)

	HOOK_LINK(get_new_docid)
	HOOK_LINK(get_docid)
	HOOK_LINK(get_last_docid)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, open_did_db, \
		(did_db_t** did_db, int opt), (did_db, opt), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sync_did_db, ( did_db_t* did_db ), (did_db), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, close_did_db, ( did_db_t* did_db ), (did_db), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, get_new_docid, \
	(did_db_t* did_db, char* pKey, uint32_t* docid, uint32_t* olddocid),\
	(did_db, pKey, docid, olddocid), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, get_docid, \
	(did_db_t* did_db, char *pKey, uint32_t *docid), (did_db, pKey, docid), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(uint32_t, get_last_docid, (did_db_t* did_db), (did_db), DECLINE)
