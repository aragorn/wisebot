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
#include <libxml/tree.h>
#include <libxml/parser.h>

#define TEMP_BUF_ICONV					2048

struct DocObject {
	char key[MAX_KEY_LENGTH];
	char charset[MAX_CHARSET_ID];
	xmlDocPtr doc;
	xmlNodePtr root;
	xmlDtdPtr dtd;
};

struct FieldObject {
	xmlNodePtr node;
};

static xmlDtdPtr cannedDocDtd;
// XXX should be set by config
//static char canned_doc_dtd_url[]="file:" TEST_DB_PATH "dtd/canneddoc.dtd";

int 
DAPI_init () {
	// FIXME: canned_doc_dtd_url must be set by configuration module.
	//        refer to log_error.c or mod_vbm.c
/*	debug("function started");*/
/*	cannedDocDtd = xmlParseDTD (NULL, canned_doc_dtd_url);*/
/*	debug("after xmlParseDTD called");*/
/*	if (cannedDocDtd == NULL) {*/
/*		warn ("cannot parse dtd");*/
/*		return FALSE;*/
/*	}*/
	cannedDocDtd = NULL;

	return TRUE;
}

int 
DAPI_close () {
	xmlFreeDtd (cannedDocDtd);

	return TRUE;
}

int
DAPI_put (DocObject            *doc) {
	xmlChar       *tmpChar;
	char          *locate,
	              *to_put;
	int            iSize,
	               iResult;
	DocId          docId;
	VariableBuffer buf;
/*	xmlValidCtxt   cvp;*/

	// FIXME
/*	static int sdocId = 0;*/

/*	cvp.userData = (void *)stderr;*/
/*	cvp.error = (xmlValidityErrorFunc) fprintf;*/
/*	cvp.warning = (xmlValidityWarningFunc) fprintf;*/

/*	if (!xmlValidateDtd (&cvp, doc->doc, cannedDocDtd)) {*/
/*		if (debug) {*/
/*			fprintf (stderr, "%s : %d : (DAPI_put) invalid canned document\n", */
/*					__FILE__, __LINE__);*/
/*			fflush (stderr);*/
/*		}*/
/*		return FALSE;*/
/*	}*/

	// FIXME -- get document id
/*	sdocId++;*/
/*	docId = sdocId;*/

	iResult = sb_run_client_get_new_docid(doc->key, &docId);
	if (iResult < 0) {
		error("cannot get new document id of key: \"%s\"", doc->key);
		return FALSE;
	}

	xmlDocDumpMemory(doc->doc, &tmpChar, &iSize);

	// charset transform
	if ((to_put = UTF8_TO_EUCKR ((char *)tmpChar)) == NULL) {
		free (tmpChar);
		return FALSE;
	}
	free (tmpChar);

	// remove <?xml ... ?>\n
	locate = to_put;
	if (strstr(to_put, "<?xml")) {
		locate = strchr (to_put, '>');
		locate += 2;
		iSize -= (int)(locate - to_put);
	}

	sb_run_buffer_initbuf(&buf);

	iResult = sb_run_buffer_append(&buf, iSize, (char *)locate);
	sb_free(to_put);
	if (iResult < 0) {
		error ("out of memory");
		return FALSE;
	}

	iResult = sb_run_server_canneddoc_put(docId, &buf);
/*	iResult = sb_run_client_canneddoc_put(docId, &buf);*/
	sb_run_buffer_freebuf (&buf);
	if (iResult < 0) {
		error ("cannot regist document : %ld", docId);
		return FALSE;
	}

	return TRUE;
}

int
DAPI_get (DocId             docId,
		  DocObject        **pdoc) {
	DocObject		*doc;
	int              iResult,
	                 iSize;
	VariableBuffer   buf;
	char            *to_parse;
	xmlDocPtr        xmldoc;

	int tmpChar[DOCUMENT_SIZE];
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

	doc->dtd = cannedDocDtd;

	iSize = sb_run_buffer_getsize(&buf);

	iResult = sb_run_buffer_get(&buf, 0, iSize, tmpChar);
	tmpChar[iSize] = '\0';

	sb_run_buffer_freebuf(&buf);

	// charset transform
	if ((to_parse = EUCKR_TO_UTF8 ((char *)tmpChar)) == NULL) {
		sb_free(doc);
		error("cannot convert Euc-kr to UTF-8");
		return FALSE;
	}

/*	iSize = strlen(to_parse);*/
/*	xmldoc = xmlParseMemory (to_parse, iSize);*/

	xmldoc = xmlParseDoc(to_parse);
	free (to_parse);
	if (xmldoc == NULL) {
		error("cannot parse document[%ld]", docId);
		sb_free(doc);
		return FALSE;
	}

	doc->doc = xmldoc;

	doc->root = xmlDocGetRootElement (xmldoc);
	if (doc->root == NULL) {
		error("cannot get root element of cannot document[%ld]", docId);
		xmlFreeDoc (doc->doc);
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
	char            *to_parse;
	xmlDocPtr        xmldoc;

	int tmpChar[DOCUMENT_SIZE];

	if (numRetrievedDoc > MAX_NUM_RETRIEVED_DOC) {
		error("too big number of argument 1; max number of retrieved document is %d",
				MAX_NUM_RETRIEVED_DOC);
		return FALSE;
	}

	for (i=0; i<numRetrievedDoc; i++) {
		docs[i] = malloc(sizeof(DocObject));
		if (docs[i] == NULL) {
			error("out of memory");
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
		docs[i]->dtd = cannedDocDtd;

		iSize = sb_run_buffer_getsize(&(bufs[i])); // XXX: deleteme
		if (iSize <= 0) {
			warn("Document has negative iSize! - skipping");
			doc[i]->p = NULL; // mark as absent
			continue;
		}

		if (iSize > DOCUMENT_SIZE-1) iSize = DOCUMENT_SIZE - 1;

		bzero(tmpChar, DOCUMENT_SIZE); // XXX
		iResult = sb_run_buffer_get(&(bufs[i]), 0, iSize, tmpChar);
		tmpChar[iSize] = '\0';

		sb_run_buffer_freebuf(&(bufs[i]));

		// charset transform
		if ((to_parse = EUCKR_TO_UTF8 ((char *)tmpChar)) == NULL) {
			error("cannot convert Euc-kr to UTF-8 of document[%ld]", aDoc[i].docId);
			continue;
		}

		xmldoc = xmlParseDoc(to_parse);
		free (to_parse);
		if (xmldoc == NULL) {
			error("cannot parse document[%ld]", aDoc[i].docId);
			continue;
		}

		docs[i]->doc = xmldoc;

		docs[i]->root = xmlDocGetRootElement (xmldoc);
		if (docs[i]->root == NULL) {
			error("cannot get root element object of document[%ld]", aDoc[i].docId);
			continue;
		}
	}

	return TRUE;
}

int
DAPI_initDoc (DocObject            *doc,
              char                 *key,
			  char                 *version,
			  char                 *rootElement) {
	int                  len;
	xmlNodePtr           tmpNode;

	len = strlen (key);
	if (len > MAX_KEY_LENGTH) {
		error ("too long key");
		return FALSE;
	}

	strncpy (doc->key, key, MAX_KEY_LENGTH);

	doc->doc = xmlNewDoc (NULL); //"1.0");
	if (doc->doc == NULL) {
		error ("cannot create xmlDoc object");
		return FALSE;
	}

	tmpNode = xmlNewNode (NULL, BAD_CAST rootElement);
	if (tmpNode == NULL) {
		error ("cannot create xmlNode object");
		return FALSE;
	}

	xmlDocSetRootElement (doc->doc, tmpNode);

	doc->root = tmpNode;

	// FIXME -- Version info
/*	tmpNode = xmlNewNode (NULL, "Version");*/
/*	if (tmpNode == NULL) {*/
/*		if (debug) {*/
/*			fprintf (stderr, "%s : %d : (DAPI_initDoc) cannot create xmlNode object\n",*/
/*					__FILE__, __LINE__);*/
/*			fflush (stderr);*/
/*		}*/
/*		return FALSE;*/
/*	}*/

/*	xmlNodeSetContent (tmpNode, BAD_CAST version);*/

	doc->dtd = cannedDocDtd;

	return TRUE;
}

int
DAPI_freeDoc (DocObject           *doc) {
	if (doc->doc) xmlFreeDoc (doc->doc);
	sb_free(doc); // malloc in DAPI_get

	return TRUE;
}

int
DAPI_addField (DocObject          *doc,
               FieldObject        *field,
               char               *fieldname,
			   char               *data) {
	xmlNodePtr               tmpNode;
	char                    *tmpChar;

	tmpNode = xmlNewNode (NULL, BAD_CAST fieldname);
	if (tmpNode == NULL) {
		error ("cannot create xmlNode object");
		return FALSE;
	}

	if ((tmpChar = EUCKR_TO_UTF8 (data)) == NULL) {
		return FALSE;
	}

	xmlNodeSetContent (tmpNode, BAD_CAST tmpChar);
	free (tmpChar);

	xmlAddChild (doc->root, tmpNode);

	if (field != NULL) {
		field->node = tmpNode;
	}

	return TRUE;
}

int
DAPI_getField (DocObject             *doc,
		       FieldObject           *field,
			   char                  *fieldname,
			   char                 **buffer) {
	xmlNodePtr              tmpNode;
	char                   *buf,
	                       *tmpChar;

	tmpNode = doc->root;
	if (tmpNode == NULL) {
		error("no root element");
		return FALSE;
	}

	tmpNode = tmpNode->xmlChildrenNode;
	if (tmpNode == NULL) {
		error("no child no of root element");
		return FALSE;
	}

	while (tmpNode && xmlStrcmp (tmpNode->name, fieldname)) {
		tmpNode = tmpNode->next;
	}

	if (tmpNode == NULL) {
		error("cannot find field[%s]", fieldname);
		return FALSE;
	}

	buf = xmlNodeGetContent (tmpNode);


	if ((tmpChar = UTF8_TO_EUCKR (buf)) == NULL) {
		sb_free(buf);
		return FALSE;
	}
	sb_free(buf);

	if (field != NULL) {
		field->node = tmpNode;
	}

	*buffer = tmpChar;

	return TRUE;
}

int
DAPI_setAttrOfField (FieldObject            *field,
                     char                   *attr,
					 char                   *value) {
	xmlAttrPtr             tmpAttr;

	tmpAttr = xmlNewProp (field->node, BAD_CAST attr, BAD_CAST value);
	if (tmpAttr == NULL) {
		error ("cannot set attribute");
		return FALSE;
	}

	return TRUE;
}

int
DAPI_getAttrOfField (FieldObject          *field,
					 char                 *attr,
					 char                **value) {
	char               *tmpChar;

	tmpChar = xmlGetProp (field->node, BAD_CAST attr);

	if (tmpChar == NULL) {
		error ("cannot get attribute value");
		return FALSE;
	}

	*value = tmpChar;

	return TRUE;
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

