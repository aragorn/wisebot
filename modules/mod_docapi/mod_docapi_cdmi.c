/**
 * $Id$
 * created by YoungHoon  
 */

#include "softbot.h"

#include "mod_api/docapi.h"
#include "mod_api/spool.h"

#include <errno.h>

static char f_spoolpath[MAX_SPOOL_PATH_LEN];
static int f_spool_queuesize = SPOOL_QUEUESIZE;

/*static char cdmdbname[MAX_DBNAME_LEN];*/
/*static char cdmdbpath[MAX_DBPATH_LEN];*/

static spool_t *filter_spool;

#define MAX_ATTR_NUM				32
#define DAPI_FIELD_NAME_LEN			20
#define DAPI_MAX_FIELD_NUM			50
#define DAPI_DEFAULT_BUFFER_SIZE	4096

#define DAPI_GETSIZE(a)				((a)->usedsize + sizeof(document_t))
#define OVERFLOW_BUFFER(a,b)		((a)->allocatedsize < (a)->usedsize + (b))

struct fieldobject_t {
	char fieldname[DAPI_FIELD_NAME_LEN];
	int fieldbodysize;
	int fieldbodyoffset;
};

struct document_t {
	int usedsize;
	int allocatedsize;
	void *buffer;

	int num_of_fields;
	fieldobject_t fields[DAPI_MAX_FIELD_NUM];
};

static int doubling_buffer(document_t *doc);

static document_t *
DAPI_initDoc () {
	int i;
	document_t *doc;
	doc = (document_t *)sb_malloc(sizeof(document_t));
	if (doc == NULL) {
		return NULL;
	}

	doc->buffer = (void *)sb_malloc(DAPI_DEFAULT_BUFFER_SIZE);
	if (doc->buffer == NULL) {
		sb_free(doc);
		return NULL;
	}
	doc->usedsize = 0;
	doc->allocatedsize = DAPI_DEFAULT_BUFFER_SIZE;
	doc->num_of_fields = 0;

	for (i=0; i<DAPI_MAX_FIELD_NUM; i++) {
		doc->fields[i].fieldname[0] = '\0';
	}

	return doc;
}

static int doubling_buffer(document_t *doc)
{
	void *tmp;
	tmp = (document_t *)sb_realloc(doc->buffer, (doc->allocatedsize * 2));
	if (tmp ==  NULL) {
		return FAIL;
	}

	doc->buffer = tmp;
	doc->allocatedsize *= 2;
	return SUCCESS;
}

static void
DAPI_freeDoc (document_t           *doc) {
	if (doc == NULL) return;

	if (doc->buffer)
		sb_free(doc->buffer);

	sb_free(doc);
}

static int
DAPI_put (document_t            *doc) {
	void *buf;
	uint32_t key = 1; //XXX: temporarily..

	if ((buf = sb_run_spool_malloc(filter_spool, DAPI_GETSIZE(doc))) == NULL) {
		error("spool malloc fail");
		return FAIL;
	}

	memcpy(buf, doc, sizeof(document_t));
	memcpy(buf+sizeof(document_t), doc->buffer, doc->usedsize);

	((document_t *) buf)->buffer = buf + sizeof(document_t);
	
	while (sb_run_spool_is_full(filter_spool)) {
		warn("filter spool is full.. wait 1 sec");
		sleep(1);
	}

	/* modified by fortuna */
	if (sb_run_spool_enqueue(filter_spool, key, buf, DAPI_GETSIZE(doc)) == -1) {
		error("spool enqueue fail");
		return FAIL;
	}
	return SUCCESS;
}

static document_t *
DAPI_get (void *data) {
	document_t		*doc;
	doc = (document_t *)data;
	doc->buffer = (void *)doc + sizeof(document_t);
	doc->allocatedsize = doc->usedsize;
	return doc;
}

static int
DAPI_addField (document_t          *doc,
               char               *fieldname,
			   void               *data, 
			   int				  datalen) {
	if (doc->num_of_fields == DAPI_MAX_FIELD_NUM) {
		return FAIL;
	}

	while (OVERFLOW_BUFFER(doc, datalen)) {
		if (doubling_buffer(doc) == FAIL) {
			error("cannot doubling buffer");
			return FAIL;
		}
	}

	strncpy(doc->fields[doc->num_of_fields].fieldname, 
			fieldname, DAPI_FIELD_NAME_LEN-1);
	doc->fields[doc->num_of_fields].fieldname[DAPI_FIELD_NAME_LEN-1] = '\0';

	doc->fields[doc->num_of_fields].fieldbodysize = datalen;
	doc->fields[doc->num_of_fields].fieldbodyoffset = doc->usedsize;

	memcpy(doc->buffer + doc->usedsize, data, datalen);
	doc->usedsize += datalen;

	doc->num_of_fields++;

	return SUCCESS;
}

static int
DAPI_getField (document_t             *doc,
			   char                   *fieldname,
			   void                  **buffer,
			   int					  *bufferlen) {
	int i;
	for (i=0; i<DAPI_MAX_FIELD_NUM && doc->fields[i].fieldname[0]; i++) {
		if (strncmp(doc->fields[i].fieldname, fieldname, DAPI_FIELD_NAME_LEN) 
				== 0) {
			break;
		}
	}
	if (i == DAPI_MAX_FIELD_NUM || !doc->fields[i].fieldname[0]) {
		return FAIL;
	}
	*buffer = doc->fields[i].fieldbodyoffset + doc->buffer;
	*bufferlen = doc->fields[i].fieldbodysize;
	return SUCCESS;
}

static int init(void)
{
	filter_spool = sb_run_spool_open(f_spoolpath, f_spool_queuesize,
			SPOOL_MPOOLSIZE);
	if (filter_spool == NULL) {
		crit("cannot open spool[%s]", f_spoolpath);
		return -1;
	}

/*	if (sb_run_cdm_spool_select_db(filter_spool, cdmdbname, cdmdbpath) == -1) {*/
/*		crit("cannot open cdmdb[%s, %s]", cdmdbname, cdmdbpath);*/
/*		return -1;*/
/*	}*/
	return 1;
}

/****************************************************************************/
/*static void set_spool(configValue v)*/
/*{*/
/*	snprintf(spoolpath, MAX_SPOOL_PATH_LEN, "%s/%s",*/
/*			gSoftBotRoot, v.argument[0]);*/
/*	spoolpath[MAX_SPOOL_PATH_LEN-1] = '\0';*/

/*	strncpy(cdmdbname, v.argument[1], MAX_DBNAME_LEN);*/
/*	cdmdbname[MAX_DBNAME_LEN-1] = '\0';*/

/*	snprintf(cdmdbpath, MAX_DBPATH_LEN, "%s/%s/",*/
/*			gSoftBotRoot, v.argument[2]);*/
/*	cdmdbpath[MAX_DBPATH_LEN-1] = '\0';*/

/*	info("configure: spool[%s] that use cdmdb[%s, %s]",*/
/*			spoolpath, cdmdbname, cdmdbpath);*/
/*}*/

static void set_filter_spool(configValue v)
{
	snprintf(f_spoolpath, MAX_SPOOL_PATH_LEN, "%s/%s",
			gSoftBotRoot, v.argument[0]);
	f_spoolpath[MAX_SPOOL_PATH_LEN-1] = '\0';

	f_spool_queuesize = atoi(v.argument[1]);

	info("configure: use filter spool[%s, %d]", f_spoolpath, f_spool_queuesize);
}

static void register_hooks(void)
{
	sb_hook_docapi_initdoc(DAPI_initDoc,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docapi_freedoc(DAPI_freeDoc,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docapi_put(DAPI_put,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docapi_get(DAPI_get,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docapi_add_field(DAPI_addField,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docapi_get_field(DAPI_getField,NULL,NULL,HOOK_MIDDLE);
}

static config_t config[] = {
/*	CONFIG_GET("Spool", set_spool, 3,*/
/*			"spool database db file(include path): Spool [spool db path]" \*/
/*			"[cdm db name] [cdm db path]"),*/
	CONFIG_GET("FilterSpool", set_filter_spool, 2,
			"spool database db file(include path): FilterSpool [spool db path] [spool queue size]"), 
	{NULL}
};

module docapi_cdmi_module = {
	STANDARD_MODULE_STUFF,
	config,				/* config */
	NULL,				/* registry */
	init,  				/* initialize function */
	NULL,				/* child_main */
	NULL,				/* scoreboard */
	register_hooks		/* register hook api */
};

