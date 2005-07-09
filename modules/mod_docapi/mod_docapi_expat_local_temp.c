/**
 * $Id$
 * created by YoungHoon  
 */

#include "softbot.h"

#include "mod_api/docapi.h"
#include "mod_api/cdm.h"
#include "mod_api/vbm.h"
#include "mod_api/xmlparser.h"

#include <errno.h>
#ifdef CYGWIN
#include "expat.h"
#else
#include "expat/expat.h"
#endif

#define MAX_ATTR_NUM			32

struct DocObject {
	parser_t *p;
};

struct FieldObject {
};

struct AttrObject {
};

int
DAPI_initDoc (DocObject            *doc,
              char                 *key,
			  char                 *version,
			  char                 *rootElement) {
	return FALSE;
}

int
DAPI_freeDoc (DocObject           *doc) {
	if (doc) {
		if (doc->p) sb_run_xmlparser_free_parser(doc->p);
		sb_free(doc);
	}
	return TRUE;
}

int
DAPI_put (DocObject            *doc) {
	return FALSE;
}

int
DAPI_get (DocId             docId,
		  DocObject        **pdoc) {
	DocObject		*doc;
	int              iResult,
	                 iSize,
                     size;
	VariableBuffer   buf;
	static char *tmpChar = NULL;
	if (tmpChar == NULL) {
		tmpChar = (char *)sb_malloc(DOCUMENT_SIZE);
		if (tmpChar == NULL) {
			crit("out of memory: %s", strerror(errno));
			return FALSE;
		}
	}

	doc = sb_malloc(sizeof(DocObject));
	if (doc == NULL) {
		error("cannot allocate memory for DocObject: %s", strerror(errno));
		sb_free(tmpChar);
		tmpChar = NULL;
		return FALSE;
	}

	/* get document */
	sb_run_buffer_initbuf(&buf);
	iResult = sb_run_server_canneddoc_get(docId, &buf);
	if (iResult == CDM_DELETED) {
		info("deleted document[%u]",(uint32_t)docId);
		sb_free(doc);
		return DOCAPI_DELETED;
	}
	if (iResult < 0) {
		error("cannot retrieve canned document from server");
		sb_free(doc);
		return FALSE;
	}
	iSize = sb_run_buffer_getsize(&buf);
	iResult = sb_run_buffer_get(&buf, 0, iSize, tmpChar);
	if (iResult < 0) {
		error("cannot get document[%u] from variable buffer", docId);
		sb_free(doc);
		return FALSE;
	}
	tmpChar[iSize] = '\0';
    size = sb_run_buffer_getsize(&buf);
	sb_run_buffer_freebuf(&buf);

	/* parse */
	doc->p = sb_run_xmlparser_parselen("CP949", tmpChar, size);
	if (doc->p == NULL) {
		error("cannot parse document[%u]", docId);
		sb_free(doc);
		return FALSE;
	}

	*pdoc = doc;

	return TRUE;
}

int
DAPI_getAbstract (int              numRetrievedDoc,
		          RetrievedDoc     aDoc[],
				  DocObject       *docs[]) {
	int              iResult,
	                 iSize,
					 i;
	VariableBuffer   bufs[MAX_NUM_RETRIEVED_DOC];
	static char *tmpChar = NULL;

	if (tmpChar == NULL) {
		tmpChar = (char *)sb_malloc(DOCUMENT_SIZE);
		if (tmpChar == NULL) {
			crit("out of memory: %s", strerror(errno));
			return FALSE;
		}
	}


	if (numRetrievedDoc > MAX_NUM_RETRIEVED_DOC) {
		error("too big number of argument 1; max number of retrieved document is %d",
				MAX_NUM_RETRIEVED_DOC);
		return FALSE;
	}

	/* get documents */
	for (i=0; i<numRetrievedDoc; i++) {
		docs[i] = sb_malloc(sizeof(DocObject));
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
		int size = 0;
		if (aDoc[i].docId == 0) continue;

		iSize = sb_run_buffer_getsize(&(bufs[i]));
		if (iSize <= 0) {
			info("size <= 0");
			// so that we know that this document isn't there
			info("Document has negative iSize! - skipping");
			docs[i]->p = NULL;
			// make core
			// abort();
			continue;
		}
		iResult = sb_run_buffer_get(&(bufs[i]), 0, iSize, tmpChar);
		tmpChar[iSize] = '\0';
        size = sb_run_buffer_getsize(&(bufs[i]));
		sb_run_buffer_freebuf(&(bufs[i]));

		docs[i]->p = sb_run_xmlparser_parselen("CP949", tmpChar, size);
		if (docs[i]->p == NULL) {
			error("cannot parse document[%ld]: charset:%s", aDoc[i].docId, tmpChar);
			continue;
		}
	}

	return TRUE;
}


int
DAPI_addField (DocObject          *doc,
               FieldObject        *field,
               char               *fieldname,
			   char               *data) {
	return FALSE;
}

int
DAPI_getField (DocObject          *doc,
		       FieldObject        *field,
			   char               *fieldname,
			   char              **buffer) {
	char path[MAX_PATHKEY_LEN];
	field_t *f;

	if (doc->p == NULL) {
		warn("no parsing result");
		return FAIL;
	}

	/* get field */
	strncpy(path, "/Document/", MAX_PATHKEY_LEN);
	strncat(path, fieldname, MAX_PATHKEY_LEN-11);
	f = sb_run_xmlparser_retrieve_field(doc->p, path);
	if (f == NULL) {
		warn("cannot retrieve field[%s]", fieldname);
		return FAIL;
	}

	/* duplicate */
	*buffer = (char *)sb_malloc(f->size + 1);
	if (*buffer == NULL) {
		error("cannot malloc: %s", strerror(errno));
		return FALSE;
	}
	strncpy(*buffer, f->value, f->size);
	(*buffer)[f->size] = '\0';

	return TRUE;
}

int
DAPI_setAttrOfField (FieldObject            *field,
                     char                   *attr,
					 char                   *value) {
	return FALSE;
}

int
DAPI_getAttrOfField (FieldObject          *field,
					 char                 *attr,
					 char                **value) {
	return FALSE;
}

static void register_hooks(void)
{
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
	NULL,  				/* initialize function */
	NULL,				/* child_main */
	NULL,				/* scoreboard */
	register_hooks		/* register hook api */
};

