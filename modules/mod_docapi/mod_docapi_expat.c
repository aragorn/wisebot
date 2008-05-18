/**
 * $Id$
 * created by YoungHoon  
 */


// DAPI_freeDoc ÇÏ´Ù ¸»¾Ò½¿...

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
	char key[MAX_KEY_LENGTH];
	char charset[MAX_CHARSET_ID];
	char root[STRING_SIZE];
	struct FieldObject *fields;

	XML_Parser p;
};

struct FieldObject {
	char name[STRING_SIZE];
	struct AttrObject *attrs;
	char *value;
	int is_cdata;
	struct FieldObject *next;
	struct FieldObject *prev;
};

struct AttrObject {
	char name[SHORT_STRING_SIZE];
	char value[STRING_SIZE];
	struct AttrObject *next;
	struct AttrObject *prev;
};

struct localdata {
	int depth;
	int len;
	DocObject *doc;
	FieldObject *this_field;
};

/* if you want packed dom (such as serialize fieldobjects after
 * docobject), change these two allocation function
 */
static DocObject *doc_alloc(struct localdata *data) {
	DocObject *doc;

	doc = (DocObject *)sb_malloc(sizeof(DocObject));
	if (doc == NULL) {
		error("out of memory");
		return NULL;
	}
	doc->key[0] = '\0';
	doc->charset[0] = '\0';
	doc->root[0] = '\0';
	doc->fields= NULL;

	return doc;
}

static FieldObject *field_alloc(struct localdata *data) {
	FieldObject *field;

	field = (FieldObject *)sb_malloc(sizeof(FieldObject));
	if (field == NULL) {
		error("out of memory");
		return NULL;
	}

	field->name[0] = '\0';
	field->attr = NULL;
	field->value = NULL;
	field->is_cdata = 0;
	field->next = NULL;
	field->prev = NULL;

	return field;
}

static AttrObject *attr_alloc(struct localdata *data) {
	AttrObject *attr;
	int i;

	attr = (AttrObject *)sb_malloc(sizeof(AttrObject));
	if (attr == NULL) {
		error("out of memory");
		return NULL;
	}

	for (i=0; i<num; i++) {
		attr->name[0] = '\0';
		attr->value[0] = '\0';
	}

	attr->next = NULL;
	attr->prev = NULL;

	return attr;
}

static char *append(struct localdata *data, char *dest, char *sour, int len) {
	char *tmp;
	int moto_len = strlen(dest);
	tmp = (char *)sb_realloc(dest, moto_len + len + 1);
	if (tmp == NULL) {
		error("out of memory");
		return NULL;
	}

	memcpy(tmp + moto_len, sour, len);
	tmp[moto_len + len] = '\0';

	return tmp;
}

static void start_element_handler(void *data, const char *el, const char **attr) {
	FieldObject *prev, *tmp, *field;
	DocObject *doc = ((struct localdata *)data)->doc;
	int i;

	field = field_alloc(data);
	if (field == NULL) {
		error("out of memory");
		return;
	}

	// element name
	strncpy(field->name, el, SHORT_STRING_SIZE);
	
	// attributes
	for (i=0; attr[i*2]; i++) {
		AttrObject *tmpat, *at;

		at = attr_alloc(data);
		if (at == NULL) {
			return;
		}

		strncpy(at.name, attr[i*2], SHORT_STRING_SIZE);
		strncpy(at.value, attr[i*2+1], STRING_SIZE);

		if (field->attrs == NULL) {
			field->attrs = at;
		}
		else {
			tmpat = field->attrs;
			while (tmpat->next) {
				tmpat = tmpat->next;
			}
			tmpat->next = at;
			at->prev = tmpat;
		}
	}

	// add to linked list
	if (doc->fields == NULL) {
		doc->fields = field;
	}
	else {
		tmp = doc->fields;
		while (tmp->next) {
			tmp = tmp->next;
		}
		tmp->next = field;
		field->prev = tmp;
	}

	((struct localdata *)data)->this_field = field;
}

static void end_element_handler(void *data, const char *el, const char **attr) {
	((struct localdata *)data)->this_field = NULL;
}

static void character_handler(void *data, const char *s, int len) {
	char *tmp;
	FieldObject *field = ((struct localdata *)data)->this_field;

	if (field == NULL) {
		return;
	}

	tmp = append(data, field->value, s, len);
	if (tmp == NULL) {
		return;
	}

	field->value = tmp;
}

void start_cdata_handler(void *data) {
	((struct localdata *)data)->this_field->is_cdata = 1;
}

void init_localdata(struct localdata *data) {
	data->depth = 0;
	data->len = 0;
	data->doc = NULL;
	data->this_field = NULL;
}

int 
DAPI_init () {
	return TRUE;
}

int 
DAPI_close () {
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

	doc->p = XML_ParserCreate(NULL);

	xmlDocSetRootElement (doc->doc, tmpNode);

	doc->root = tmpNode;

	return TRUE;
}

int
DAPI_freeDoc (DocObject           *doc) {
	int i;
	FieldObject *field, *tmpfield;

	for (field = doc->fields; field; field = field->next) {
		tmpfield = field->next;
		sb_free(field);
	}
	XML_ParserFree(doc->p);
	return TRUE;
}
int
DAPI_put (DocObject            *doc) {
	char		  *tmpChar;
	char          *locate,
	              *to_put;
	int            iSize,
	               iResult;
	DocId          docId;
	VariableBuffer buf;

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
	struct localdata data;

	int tmpChar[DOCUMENT_SIZE];
	bzero(tmpChar, DOCUMENT_SIZE);

	doc = sb_malloc(sizeof(DocObject));
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
		sb_free(doc);
		error("cannot convert Euc-kr to UTF-8");
		return FALSE;
	}

	// create parser & set event handler
	doc->p = XML_ParserCreate(NULL);

	init_localdata(&data);

	XML_SetUserData(doc->p, &data);
	XML_SetElementHandler(doc->p, start_element_handler, end_element_handler);
	XML_SetCharacterDataHandler(doc->p, character_handler);
	XML_SetStartCdataSectionHandler(doc->p, cdata_handler);

	// parse document
	iSize = strlen(to_parse);
	iResult = XML_Parse(doc->p, to_parse, iSize, 1);
	sb_free(to_parse);
	if (iResult == 0) {
		warn("cannot parse docuemnt[%d]", docId);
		return FALSE;
	}

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
		struct localdata data;

		iSize = sb_run_buffer_getsize(&(bufs[i]));
		if (iSize <= 0) {
			// mark as absent...
			warn("Document has negative iSize! - skipping");
			doc[i]->p = NULL;
			continue;
		}

		bzero(tmpChar, DOCUMENT_SIZE); // XXX
		iResult = sb_run_buffer_get(&(bufs[i]), 0, iSize, tmpChar);
		tmpChar[iSize] = '\0';

		sb_run_buffer_freebuf(&(bufs[i]));

		// charset transform
		if ((to_parse = EUCKR_TO_UTF8 ((char *)tmpChar)) == NULL) {
			error("cannot convert Euc-kr to UTF-8 of document[%ld]", aDoc[i].docId);
			continue;
		}

		// create parser & set event handler
		docs[i]->p = XML_ParserCreate(NULL);

		info("docs[i]->p = %x\n", docs[i]->p);

		init_localdata(&data);

		XML_SetUserData(docs[i]->p, &data);
		XML_SetElementHandler(docs[i]->p, start_element_handler, end_element_handler);
		XML_SetCharacterDataHandler(docs[i]->p, character_handler);
		XML_SetStartCdataSectionHandler(docs[i]->p, cdata_handler);

		// parse document
		iSize = strlen(to_parse);
		iResult = XML_Parse(docs[i]->p, to_parse, iSize, 1);
		sb_free(to_parse);
		if (iResult == 0) {
			warn("cannot parse docuemnt[%d]", docId);
			return FALSE;
		}
	}

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

