/**
 * $Id$
 * created by YoungHoon  
 */

#include "softbot.h"

#include "mod_api/docapi.h"
#include "mod_api/cdm.h"
#include "mod_api/vbm.h"
#include "mod_api/did_client.h"
#include "mod_api/charconv.h"

#include <errno.h>
#include "expat.h"

#define MAX_ATTR_NUM			32

struct DocObject {
	char *doc;
};

struct FieldObject {
};

struct AttrObject {
};

struct localdata {
	char name[STRING_SIZE];
	int is_this_field;
	char *buffer;
};

static void start_element_handler(void *data, const char *el, const char **attr) {
	struct localdata *local = (struct localdata *)data;
	if (strcmp(local->name, el) == 0) {
		local->is_this_field = 1;
	}
}

static void end_element_handler(void *data, const char *el) {
	struct localdata *local = (struct localdata *)data;
	if (local->is_this_field) local->is_this_field = 0;
}

static void character_handler(void *data, const char *s, int len) {
	struct localdata *local = (struct localdata *)data;
	char *tmp;
	int moto_len;

	if (local->is_this_field) {
		if (local->buffer) moto_len = strlen(local->buffer);
		else moto_len = 0;
		tmp = sb_realloc(local->buffer, moto_len + len + 1);
		if (tmp == NULL) {
			return;
		}
		memcpy(tmp + moto_len, s, len);
		tmp[moto_len + len] = '\0';
		local->buffer = tmp;
	}
}

static void init_localdata(struct localdata *data, char *field) {
	strcpy(data->name, field);
	data->is_this_field = 0;
	data->buffer = NULL;
}

static int 
DAPI_init () {
	return TRUE;
}

static int 
DAPI_close () {
	return TRUE;
}

static int
DAPI_initDoc (DocObject            *doc,
              char                 *key,
			  char                 *version,
			  char                 *rootElement) {
	return FALSE;
}

static int
DAPI_freeDoc (DocObject           *doc) {
	if (doc) {
		if (doc->doc) sb_free(doc->doc);
		sb_free(doc);
	}
	return TRUE;
}

static int
DAPI_put (DocObject            *doc) {
	return FALSE;
}

static int
DAPI_get (DocId             docId,
		  DocObject        **pdoc) {
	DocObject		*doc;
	int              iResult,
	                 iSize;
	VariableBuffer   buf;
	char            *to_parse;

	char tmpChar[DOCUMENT_SIZE];
	bzero(tmpChar, DOCUMENT_SIZE);

	doc = malloc(sizeof(DocObject));
	if (doc == NULL) {
		error("cannot allocate memory for DocObject: %s", strerror(errno));
		return FALSE;
	}

	sb_run_buffer_initbuf(&buf);

	iResult = sb_run_server_canneddoc_get(docId, &buf);
/*	iResult = sb_run_client_canneddoc_get(docId, &buf);*/
	if (iResult < 0) {
		error("cannot retreive canned document from server");
		sb_free(doc);
		return FALSE;
	}

	iSize = sb_run_buffer_getsize(&buf);

	iResult = sb_run_buffer_get(&buf, 0, iSize, tmpChar);
	tmpChar[iSize] = '\0';

	sb_run_buffer_freebuf(&buf);

	// charset transform
	if ((to_parse = EUCKR_TO_UTF8 ((char *)tmpChar)) == NULL) {
		error("cannot convert Euc-kr to UTF-8");
		return FALSE;
	}

	doc->doc = to_parse;

	*pdoc = doc;

	return TRUE;
}

static int
DAPI_getAbstract (int              numRetrievedDoc,
		          RetrievedDoc     aDoc[],
				  DocObject       *docs[]) {
	int              iResult,
	                 iSize,
					 i;
	VariableBuffer   bufs[MAX_NUM_RETRIEVED_DOC];
	char            *to_parse;

	char tmpChar[DOCUMENT_SIZE];

	if (numRetrievedDoc > MAX_NUM_RETRIEVED_DOC) {
		error("too big number of argument 1; max number of retrieved document is %d",
				MAX_NUM_RETRIEVED_DOC);
		return FALSE;
	}

	for (i=0; i<numRetrievedDoc; i++) {
		docs[i] = malloc(sizeof(DocObject));
		if (docs[i] == NULL) {
			error("out of memory: cannot alloc memory for DocObject");
			return FALSE;
		}
		sb_run_buffer_initbuf(&(bufs[i]));
	}

	iResult = sb_run_server_canneddoc_get_abstract(numRetrievedDoc, aDoc, bufs);
	if (iResult < 0) {
		error("cannot retreive canned document from server");
		return FALSE;
	}


	for (i=0; i<numRetrievedDoc; i++) {
		if (aDoc[i].docId == 0) continue;

		iSize = sb_run_buffer_getsize(&(bufs[i]));
		if (iSize <= 0) {
			info("iSize <= 0");
			warn("Document has negative iSize! - skipping");
			doc[i]->p = NULL; // mark as absent
			continue;
		}

		bzero(tmpChar, DOCUMENT_SIZE);
		iResult = sb_run_buffer_get(&(bufs[i]), 0, iSize, tmpChar);
		tmpChar[iSize] = '\0';

		sb_run_buffer_freebuf(&(bufs[i]));

		// charset transform
		if ((to_parse = EUCKR_TO_UTF8 ((char *)tmpChar)) == NULL) {
			error("cannot convert Euc-kr to UTF-8 of document[%ld]", aDoc[i].docId);
			continue;
		}

		docs[i]->doc = to_parse;
	}

	return TRUE;
}


static int
DAPI_addField (DocObject          *doc,
               FieldObject        *field,
               char               *fieldname,
			   char               *data) {
	return FALSE;
}

static int
DAPI_getField (DocObject             *doc,
		       FieldObject           *field,
			   char                  *fieldname,
			   char                 **buffer) {
	XML_Parser p;
	struct localdata local;
	char *tmp;
	int iSize, iResult;

	// create parser & set event handler
	p = XML_ParserCreate(NULL);

	init_localdata(&local, fieldname);
	XML_SetUserData(p, &local);
	XML_SetElementHandler(p, start_element_handler, end_element_handler);
	XML_SetCharacterDataHandler(p, character_handler);

	// parse document
	iSize = strlen(doc->doc);
	iResult = XML_Parse(p, doc->doc, iSize, 1);
	if (iResult == 0) {
		warn("cannot parse docuemnt: at line %d: %s", XML_GetCurrentLineNumber(p),
				XML_ErrorString(XML_GetErrorCode(p)));
		debug("%s", doc->doc);
		return FALSE;
	}

	XML_ParserFree(p);

	// charset transform
	if (local.buffer == NULL || strlen(local.buffer) == 0 ||
			(tmp = UTF8_TO_EUCKR((char *)local.buffer)) == NULL) {
		warn("cannot convert UTF-8 to Euc-kr of document");
		sb_free(local.buffer);
		*buffer = NULL;
		return FALSE;
	}
	sb_free(local.buffer);

	*buffer = tmp;

	return TRUE;
}

static int
DAPI_setAttrOfField (FieldObject            *field,
                     char                   *attr,
					 char                   *value) {
	return FALSE;
}

static int
DAPI_getAttrOfField (FieldObject          *field,
					 char                 *attr,
					 char                **value) {
	return FALSE;
}

static int init () {
	int ret;
	ret = DAPI_init ();
	if (ret < 0) {
		error ("Cannot initialize module");
		return FAIL;
	}
	return SUCCESS;
}

static void register_hooks(void)
{
/*	sb_hook_doc_init(DAPI_init,NULL,NULL,HOOK_MIDDLE);*/
/*	sb_hook_doc_close(DAPI_close,NULL,NULL,HOOK_MIDDLE);*/
	sb_hook_doc_put(DAPI_put,NULL,NULL,HOOK_MIDDLE);
	sb_hook_doc_get(DAPI_get,NULL,NULL,HOOK_MIDDLE);
	sb_hook_doc_get_abstract(DAPI_getAbstract,NULL,NULL,HOOK_MIDDLE);
	sb_hook_doc_init(DAPI_initDoc,NULL,NULL,HOOK_MIDDLE);
	sb_hook_doc_free(DAPI_freeDoc,NULL,NULL,HOOK_MIDDLE);
	sb_hook_doc_add_field(DAPI_addField,NULL,NULL,HOOK_MIDDLE);
	sb_hook_doc_get_field(DAPI_getField,NULL,NULL,HOOK_MIDDLE);
	sb_hook_doc_set_fieldattr(DAPI_setAttrOfField,NULL,NULL,HOOK_MIDDLE);
	sb_hook_doc_get_fieldattr(DAPI_getAttrOfField,NULL,NULL,HOOK_MIDDLE);
}

module docapi_module = {
	STANDARD_MODULE_STUFF,
	NULL,				/* config */
	NULL,				/* registry */
	init,  				/* initialize function */
	NULL,				/* child_main */
	NULL,				/* scoreboard */
	register_hooks		/* register hook api */
};

