/**
 * $Id$
 * Created By YoungHoon
 */

#ifndef _DOCAPI_H_
#define _DOCAPI_H_ 1

#include "softbot.h"
#include "mod_api/cdm.h"

#define DOCAPI_DELETED	-2

#define MAX_KEY_LENGTH				256
#define MAX_CHARSET_ID				64

/* XXX incomplete type declaration
 * no need to include "libxml/parser.h"
 *
#include "libxml/parser.h"
struct DOCOBJECT {
	char key[MAX_KEY_LENGTH];
	char charset[MAX_CHARSET_ID];
	xmlDocPtr doc;
	xmlNodePtr root;
	xmlDtdPtr dtd;
};
struct FIELDOBJECT {
	xmlNodePtr node;
};
*/

typedef struct document_t document_t;
typedef struct fieldobject_t fieldobject_t;

typedef struct DocObject DocObject;
typedef struct FieldObject FieldObject;
typedef struct AttrObject AttrObject;

#if defined(__cplusplus)
extern "C" {
#endif

SB_DECLARE_HOOK(int,doc_put,(DocObject* doc))
SB_DECLARE_HOOK(int,doc_get,(uint32_t docId, DocObject** pdoc))
SB_DECLARE_HOOK(int,doc_get_abstract, \
		(int numRetrievedDoc,RetrievedDoc aDoc[], DocObject *docs[]))
SB_DECLARE_HOOK(int,doc_init,\
		(DocObject* doc, char* key, char* version, char* rootElement))
SB_DECLARE_HOOK(int,doc_free,(DocObject* doc))
SB_DECLARE_HOOK(int,doc_add_field,\
		(DocObject* doc, FieldObject* field, char* fieldname, char* data))
SB_DECLARE_HOOK(int,doc_get_field,\
		(DocObject* doc, FieldObject* field, char* fieldname, char** buffer))
SB_DECLARE_HOOK(int,doc_set_fieldattr,\
		(FieldObject* field, char* attr, char* value))
SB_DECLARE_HOOK(int,doc_get_fieldattr,\
		(FieldObject* field, char* attr, char** value))

SB_DECLARE_HOOK(document_t*,docapi_initdoc,())
SB_DECLARE_HOOK(void,docapi_freedoc,(document_t* doc))
SB_DECLARE_HOOK(int,docapi_put,(document_t* doc))
SB_DECLARE_HOOK(document_t*,docapi_get,(void *data))
SB_DECLARE_HOOK(int,docapi_add_field,\
		(document_t* doc, char* fieldname, void* data, int datalen))
SB_DECLARE_HOOK(int,docapi_get_field,\
		(document_t* doc, char* fieldname, void** buffer, int *bufferlen))

#if defined(__cplusplus)
}
#endif

#endif
