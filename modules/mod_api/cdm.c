/* $Id$ */
#include "softbot.h"
#include "mod_api/cdm.h"

HOOK_STRUCT(
	HOOK_LINK(server_canneddoc_init)
	HOOK_LINK(server_canneddoc_close)
	HOOK_LINK(server_canneddoc_put)
	HOOK_LINK(server_canneddoc_put_with_oid)
	HOOK_LINK(server_canneddoc_get)
	HOOK_LINK(server_canneddoc_get_as_pointer)
	HOOK_LINK(server_canneddoc_get_size)
	HOOK_LINK(server_canneddoc_get_abstract)
	HOOK_LINK(server_canneddoc_last_registered_id)
	HOOK_LINK(server_canneddoc_delete)
	HOOK_LINK(server_canneddoc_update)
	HOOK_LINK(server_canneddoc_drop)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int,server_canneddoc_init,(),(),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,server_canneddoc_close,(),(),DECLINE)
SB_IMPLEMENT_HOOK_RUN_ALL(int,server_canneddoc_put,\
		(DocId docId, VariableBuffer *pCannedDoc),(docId, pCannedDoc),SUCCESS,DECLINE)
SB_IMPLEMENT_HOOK_RUN_ALL(int,server_canneddoc_put_with_oid,\
		(char *oid, DocId *registeredDocId, VariableBuffer *pCannedDoc), \
		(oid, registeredDocId, pCannedDoc),SUCCESS,DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,server_canneddoc_get,\
		(DocId docId, VariableBuffer *pCannedDoc),(docId, pCannedDoc),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,server_canneddoc_get_as_pointer,\
		(DocId docId, void *pCannedDoc, int size),(docId, pCannedDoc, size),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,server_canneddoc_get_size,\
		(DocId docId),(docId),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,server_canneddoc_get_abstract, \
		(int numRetrievedDoc, RetrievedDoc aRetrievedDoc[], \
		 VariableBuffer aCannedDoc[]), \
		(numRetrievedDoc, aRetrievedDoc, aCannedDoc),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(DocId,server_canneddoc_last_registered_id, \
		(void),(),((DocId)-1))
SB_IMPLEMENT_HOOK_RUN_FIRST(int,server_canneddoc_delete,(DocId docId),(docId),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,server_canneddoc_update,\
		(DocId docId, VariableBuffer *pCannedDoc),(docId, pCannedDoc),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,server_canneddoc_drop,(void),(),DECLINE)

