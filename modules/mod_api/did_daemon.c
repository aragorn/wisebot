/* $Id$ */
#include "softbot.h"
#include "mod_api/did_daemon.h"

HOOK_STRUCT(
	HOOK_LINK(load_docid_db)
	HOOK_LINK(unload_docid_db)
	HOOK_LINK(local_get_docid)
	HOOK_LINK(local_get_new_docid)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int,load_docid_db,(),(),0)
SB_IMPLEMENT_HOOK_RUN_VOID_ALL(unload_docid_db,(),())

SB_IMPLEMENT_HOOK_RUN_FIRST(int,local_get_docid,\
	(char *pKey, DocId *docid), (pKey, docid),0)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,local_get_new_docid, \
	(char *pKey, DocId *docid, DocId *olddid), (pKey, docid, olddid),0)

