/* $Id$ */
#ifndef _MOD_DOCAPI_H_
#define _MOD_DOCAPI_H_ 1

#include "mod_api/docapi.h"

int DAPI_init (void);
int DAPI_close (void);
int DAPI_put (DocObject* doc);
int DAPI_get (DocId docId, DocObject **pdoc);
int DAPI_initDoc (DocObject* doc, char* key, char* version, char* rootElement);
int DAPI_freeDoc (DocObject* doc);
int DAPI_addField (DocObject* doc, FieldObject* field, char* fieldname, char* data);
int DAPI_getField (DocObject* doc, FieldObject* field, char* fieldname, char** buffer);
int DAPI_setAttrOfField (FieldObject* field, char* attr, char* value);
int DAPI_getAttrOfField (FieldObject* field, char* attr, char** value);

#endif
