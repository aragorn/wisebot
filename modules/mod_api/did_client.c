/* $Id$ */
#include "softbot.h"
#include "mod_api/did_client.h"

HOOK_STRUCT(
	HOOK_LINK(client_get_docid)
	HOOK_LINK(client_get_new_docid)
	HOOK_LINK(client_get_last_docid)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int,client_get_docid,\
				(char *pKey, DocId *docid), (pKey, docid),0)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,client_get_new_docid, \
				(char *pKey, DocId *docid, DocId *olddid), (pKey, docid, olddid),0)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,client_get_last_docid, \
				(DocId *pMaxId), (pMaxId),0)

