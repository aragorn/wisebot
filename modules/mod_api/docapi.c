/* $Id$ */
#include "softbot.h"
#include "mod_api/docapi.h"

HOOK_STRUCT(
	HOOK_LINK(doc_put)
	HOOK_LINK(doc_get)
	HOOK_LINK(doc_get_abstract)
	HOOK_LINK(doc_init)
	HOOK_LINK(doc_free)
	HOOK_LINK(doc_add_field)
	HOOK_LINK(doc_get_field)
	HOOK_LINK(doc_set_fieldattr)
	HOOK_LINK(doc_get_fieldattr)

	HOOK_LINK(docapi_initdoc)
	HOOK_LINK(docapi_freedoc)
	HOOK_LINK(docapi_put)
	HOOK_LINK(docapi_get)
	HOOK_LINK(docapi_add_field)
	HOOK_LINK(docapi_get_field)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int,doc_put,(DocObject* doc),(doc),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,doc_get,\
		(DocId docId, DocObject** pdoc),(docId,pdoc),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,doc_get_abstract, \
		(int numRetrievedDoc,RetrievedDoc aDoc[],DocObject *docs[]), \
		(numRetrievedDoc,aDoc,docs),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,doc_init, \
		(DocObject* doc, char* key, char* version, char* rootElement), \
		(doc,key,version,rootElement),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,doc_free,(DocObject* doc),(doc),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,doc_add_field, \
		(DocObject* doc, FieldObject* field, char* fieldname, char* data), \
		(doc,field,fieldname,data),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,doc_get_field, \
		(DocObject* doc, FieldObject* field, char* fieldname, char** buffer), \
		(doc,field,fieldname,buffer),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,doc_set_fieldattr, \
		(FieldObject* field, char* attr, char* value), \
		(field,attr,value),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,doc_get_fieldattr, \
		(FieldObject* field, char* attr, char** value), \
		(field,attr,value),DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(document_t*,docapi_initdoc,(),(),((void *)-1))
SB_IMPLEMENT_HOOK_RUN_VOID_ALL(docapi_freedoc,(document_t* doc),(doc))
SB_IMPLEMENT_HOOK_RUN_FIRST(int,docapi_put,(document_t* doc),(doc),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(document_t*,docapi_get,(void *data),(data),((void*)-1))
SB_IMPLEMENT_HOOK_RUN_FIRST(int,docapi_add_field,\
		(document_t* doc, char* fieldname, void* data, int datalen),\
		(doc,fieldname,data,datalen), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,docapi_get_field,\
		(document_t* doc, char* fieldname, void** buffer, int *bufferlen),\
		(doc,fieldname,buffer,bufferlen), DECLINE)

