/* $Id */
#include <string.h> /* strncpy */
#include <stdlib.h> /* atoi */
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "common_core.h"
#include "common_util.h"
#include "util.h"
#include "memory.h"
#include "ipc.h"
#include "memfile.h"

#include "mod_cdm3.h"
#include "mod_api/cdm2.h"
#include "mod_api/did.h"
#include "mod_api/docattr.h"
#include "mod_api/xmlparser.h"

static int ifs_db_set = -1;  // used by ifs

static cdm_db_set_t* cdm_set = NULL;
static int current_cdm_set = -1;

// open을 singleton으로 구현하기 위한 것
static cdm_db_t* singleton_cdm_db[MAX_CDM_SET];
static int singleton_cdm_db_ref[MAX_CDM_SET];

/////////////////////////////////////////////////////////////
// for config Field, DocAttrField, FieldRootName
static char docattr_fields[MAX_FIELD_NUM][SHORT_STRING_SIZE];
static int docattr_field_count = 0;

static char field_root_name[SHORT_STRING_SIZE] = "Document";
/////////////////////////////////////////////////////////////

#define ACQUIRE_LOCK() \
    if ( acquire_lock( db->lock_id ) != SUCCESS ) { \
        error("acquire_lock failed"); \
        return FAIL; \
    }   
        
#define RELEASE_LOCK() \
    if ( release_lock( db->lock_id ) != SUCCESS ) { \
        error("release_lock failed. but go on..."); \
    } 

/////////////////////////////////////////////////////////////
static int init()
{
	ipc_t lock;
	int i = 0;

	lock.type = IPC_TYPE_SEM;
	lock.pid = SYS5_CDM;

	if ( cdm_set == NULL ) {
		warn("cdm_set is NULL. you must set CdmSet in config file");
		return DECLINE;
	}

	for ( i = 0; i < MAX_CDM_SET; i++ ) {
		singleton_cdm_db[i] = NULL;

		if ( !cdm_set[i].set ) continue;

        lock.pathname = cdm_set[current_cdm_set].cdm_path;

        if ( get_sem(&lock) != SUCCESS )
            return FAIL;

        cdm_set[i].lock_id = lock.id;
        snprintf( cdm_set[i].shared_file, MAX_PATH_LEN,"%s/cdm3.shared", cdm_set[i].cdm_path );	
	}

	return SUCCESS;
}

int set_delete(uint32_t docid)
{
    docattr_mask_t docmask;
        
    DOCMASK_SET_ZERO(&docmask);
    sb_run_docattr_set_docmask_function(&docmask, "Delete", "1");
    sb_run_docattr_set_array(&docid, 1, SC_MASK, &docmask);

    return SUCCESS;
}   

int is_deleted(uint32_t docid, int* deleted)
{
    docattr_t attr;
    char buf[10];
    
    if ( sb_run_docattr_get(docid, &attr) != SUCCESS ) {
        error("cannot get docattr");
        return FAIL;
    }
    
    if ( sb_run_docattr_get_docattr_function(&attr, "Delete", buf, sizeof(buf)) != SUCCESS ) {
        error("cannot get Delete field value");
        return FAIL;
    }   
    
    *deleted = (buf[0] == '1');
    
    return SUCCESS;
}  

static int alloc_shared_cdm_db(cdm_db_custom_t* cdm_db, char* shared_file, int *mmap_attr)
{
    ipc_t mmap;

    mmap.type = IPC_TYPE_MMAP;
    mmap.pathname = shared_file;
    mmap.size = sizeof(cdm_db_shared_t);

    if ( alloc_mmap(&mmap, 0) != SUCCESS ) {
        crit("error while allocating mmap for cdm_db");
        return FAIL;
    }

    if ( mmap.attr == MMAP_CREATED ) memset( mmap.addr, 0, mmap.size );

    cdm_db->shared = mmap.addr;
    *mmap_attr = mmap.attr;

    debug("cdm_db->shared [%p]", cdm_db->shared);

    return SUCCESS;
}

static int free_shared_cdm_db(cdm_db_custom_t* cdm_db)
{
	int ret;

	ret = free_mmap(cdm_db->shared, sizeof(cdm_db_shared_t));
	if ( ret == SUCCESS ) cdm_db->shared = NULL;

	return ret;
}

static int cdm_open(cdm_db_t** cdm_db, int opt)
{
    int mmap_attr;
    int ret;
    cdm_db_custom_t* db = NULL;

    if ( cdm_set == NULL ) {
        warn("cdm_set is NULL. you must set CdmSet in config file");
        return DECLINE;
    }

    if ( opt >= MAX_CDM_SET || opt < 0 ) {
        error("opt[%d] is invalid. MAX_CDM_SET[%d]", opt, MAX_CDM_SET);
        return FAIL;
    }

    if ( !cdm_set[opt].set ) {
        warn("CdmSet[opt:%d] is not defined", opt);
        return DECLINE;
    }

    // 다른 module에서 이미 열었던 건데.. 그것을 return한다.
    if ( singleton_cdm_db[opt] != NULL ) {
        *cdm_db = singleton_cdm_db[opt];
        singleton_cdm_db_ref[opt]++;

        info("reopened cdm db[set:%d, ref:%d]", opt, singleton_cdm_db_ref[opt]);
        return SUCCESS;
    }

    *cdm_db = (cdm_db_t*) sb_calloc(1, sizeof(cdm_db_t));
    if ( *cdm_db == NULL ) {
        error("sb_calloc failed: %s", strerror(errno));
        goto error;
    }

    db = (cdm_db_custom_t*) sb_calloc(1, sizeof(cdm_db_custom_t));
    if ( db == NULL ) {
        error("sb_calloc failed: %s", strerror(errno));
        goto error;
    }

    if ( alloc_shared_cdm_db( db, cdm_set[opt].shared_file, &mmap_attr ) != SUCCESS ) {
        error("alloc_shared_cdm_db failed");
        goto error;
    }

    db->lock_id = cdm_set[opt].lock_id;

	ret = sb_run_indexdb_open( &db->ifs, ifs_db_set );
	if (ret == FAIL) {
		crit("indexdb_open failed.");
		return FAIL;
	}

    (*cdm_db)->set = opt;
    (*cdm_db)->db = (void*) db;

    singleton_cdm_db[opt] = *cdm_db;
    singleton_cdm_db_ref[opt] = 1;

    return SUCCESS;

error:
    if ( db ) {
        if ( db->shared ) free_shared_cdm_db( db );
        //if ( db->did_db ) sb_run_close_did_db( db->did_db );

        sb_free( db );
    }
    if ( *cdm_db ) {
        sb_free( *cdm_db );
        *cdm_db = NULL;
    }

    return FAIL;
}

static int cdm_close(cdm_db_t* cdm_db)
{
    cdm_db_custom_t* db;
    int set;

    if ( cdm_set == NULL || !cdm_set[cdm_db->set].set )
        return DECLINE;
    db = (cdm_db_custom_t*) cdm_db->db;
    set = cdm_db->set;

    // 아직 reference count가 남아있으면 close하지 말아야 한다.
    singleton_cdm_db_ref[set]--;
    if ( singleton_cdm_db_ref[set] ) {
        info("cdm db[set:%d, ref:%d] is not closing now",
                set, singleton_cdm_db_ref[set]);
        return SUCCESS;
    }

	sb_run_indexdb_close(db->ifs);
    if ( free_shared_cdm_db( db ) != SUCCESS ) {
        warn("free shared cdm db failed");
    }

    sb_free( cdm_db->db );
    sb_free( cdm_db );

    singleton_cdm_db[set] = NULL;

    return SUCCESS;
}

static int cdm_get_doc(cdm_db_t* cdm_db, uint32_t docid, cdm_doc_t** doc)
{
	cdm_db_custom_t* db;
	int deleted, ret;
    int length = 0;
	parser_t* p = NULL;

    static char *xml_doc = NULL;
    if (xml_doc == NULL) {
        xml_doc = (char *)sb_malloc(DOCUMENT_SIZE);
        if (xml_doc == NULL) {
            crit("out of memory: %s", strerror(errno));
            return FAIL;
        }
    }

	if ( cdm_set == NULL || !cdm_set[cdm_db->set].set )
		return MINUS_DECLINE;

	db = (cdm_db_custom_t*) cdm_db->db;

	if ( docid > db->shared->last_docid ) return CDM2_GET_INVALID_DOCID;

	/****************************************/
	/* docattr 들여다보고 삭제문서인지 조사 */
	if ( is_deleted(docid, &deleted) != SUCCESS ) return FAIL;
	/****************************************/

    *doc = (cdm_doc_t*) sb_calloc(1, sizeof(cdm_doc_t));
    (*doc)->cdm_db = cdm_db;
    (*doc)->docid = docid;
    (*doc)->deleted = deleted;

	length = sb_run_indexdb_getsize( db->ifs, docid );
	if ( length == INDEXDB_FILE_NOT_EXISTS ) {
		warn("length is 0 of did[%u]. something is wrong", docid);
		return FAIL;
	}
	else if ( length == FAIL ) {
		crit("cdm_db(ifs) getsize failed. did[%d]", docid);
		return FAIL;
	}

    if(length >= DOCUMENT_SIZE) {
	    warn("document[%d] size[%d] larger than buffer size[%d]", docid, length, DOCUMENT_SIZE);
	}

	ret = sb_run_indexdb_read( db->ifs, docid, 0, length, (void*)xml_doc );
	if ( ret < 0 ) {
		crit("cdm_db(ifs) read failed. did[%d], ret: %d", docid, ret);
		return FAIL;
	}

	DEBUG("ret[%d] of cdm_db(ifs) read with did:%u)",ret, docid);
	if ( ret < length ) {
		warn( "ret[%d] is less than length[%d]", ret, length);
        return FAIL;
	}

	p = sb_run_xmlparser_parselen("CP949", xml_doc, length);
	if ( p == NULL ) {
		error("cannot parse document[%d]", docid);
		return CDM2_PUT_NOT_WELL_FORMED_DOC;
	}

    (*doc)->data = (void*)p;

	return SUCCESS;
}

static int cdmdoc_get_field_by_bytepos(cdm_doc_t* doc, const char* fieldname, int bytepos, char* buf, size_t size)
{
    parser_t* p = NULL;
    field_t* f = NULL;
    char path[STRING_SIZE];
    char* val = NULL;
    int len = 0;

    p = (parser_t*)doc->data;

	strcpy(path, "/");
	strcat(path, field_root_name);
	strcat(path, "/");
	strcat(path, fieldname);

	f = sb_run_xmlparser_retrieve_field(p, path);
	if (f == NULL) {
		warn("cannot get field[/%s/%s] (path:%s)", 
				field_root_name, 
				fieldname, path);
		return FAIL;
	}
	else if ( f == (field_t*)1 ) {
		warn("sb_run_xmlparser_retrieve_field() returned DECLINE(1)");
		return FAIL;
	}

	len = f->size;
	val = sb_trim(f->value);
	len = (len>STRING_SIZE-1)?STRING_SIZE-1:len;
	strncpy(buf, val+bytepos, size);
	buf[len] = '\0';

    return SUCCESS;
}

static int cdmdoc_get_field(cdm_doc_t* doc, const char* fieldname, char* buf, size_t size)
{
    return cdmdoc_get_field_by_bytepos(doc, fieldname, 0, buf, size);
}


static int cdmdoc_destroy(cdm_doc_t* doc)
{
	cdm_doc_custom_t* doc_custom;

	if ( cdm_set == NULL || !cdm_set[doc->cdm_db->set].set )
		return MINUS_DECLINE;

    free_parser((parser_t*)doc->data);
	sb_free(doc);

	return SUCCESS;
}

static int cdm_put_xmldoc(cdm_db_t* cdm_db, did_db_t* did_db, char* oid,
		const char* xmldoc, size_t size, uint32_t* newdocid, uint32_t* olddocid)
{
	cdm_db_custom_t* db;
	parser_t* p = NULL;
	field_t *f;
	int i = 0;
	docattr_mask_t docmask;
	int ret, oid_duplicated = 0;
    char *val, value[STRING_SIZE];
    char path[STRING_SIZE];

	if ( cdm_set == NULL || !cdm_set[cdm_db->set].set )
		return DECLINE;
	db = (cdm_db_custom_t*) cdm_db->db;

	p = sb_run_xmlparser_parselen("CP949", xmldoc, size);
	if ( p == NULL ) {
		error("cannot parse document[%s]", oid);
		return CDM2_PUT_NOT_WELL_FORMED_DOC;
	}

	/////////////////////////////////////////
	// docattr mask 생성
	DOCMASK_SET_ZERO(&docmask);
    for (i=0; i<docattr_field_count; i++) {
		int len = 0;
        strcpy(path, "/");
        strcat(path, field_root_name);
        strcat(path, "/");
        strcat(path, docattr_fields[i]);

        f = sb_run_xmlparser_retrieve_field(p, path);
        if (f == NULL) {
            warn("cannot get field[/%s/%s] of ducument[%s] (path:%s)", 
                    field_root_name, 
                    docattr_fields[i], oid, path);
            continue;
        }
        else if ( f == (field_t*)1 ) {
            warn("sb_run_xmlparser_retrieve_field() returned DECLINE(1)");
            continue;
        }

        len = f->size;
        val = sb_trim(f->value);
        len = (len>STRING_SIZE-1)?STRING_SIZE-1:len;
        strncpy(value, val, len);
        value[len] = '\0';

        if (len == 0) {
            continue;
        }

        if ( sb_run_docattr_set_docmask_function(
                    &docmask, docattr_fields[i], value) != SUCCESS ) {
            warn("wrong type of value of field[/%s/%s] of ducument[%s]", 
                    field_root_name, docattr_fields[i], oid);
        }
    }

	/////////////////////////////////////////
	// did 생성 가져오기
	ret = sb_run_get_new_docid(did_db, oid, newdocid, olddocid);
	if ( ret < 0 ) {
		error("cannot get new document id of oid[%s]:error(%s)",
				oid, strerror(errno));
		goto error;
	}
	else if ( ret == DOCID_OLD_REGISTERED ) {
		info("old docid[%u] of OID[%s] is deleted. new docid is %u",
				*olddocid, oid, *newdocid);
		set_delete(*olddocid);
		oid_duplicated = 1;
	}

	if ( *newdocid != db->shared->last_docid+1 ) {
		warn("newdocid[%"PRIu32"] != last_docid of cdm[%"PRIu32"]+1",
				*newdocid, db->shared->last_docid);
	}
	/////////////////////////////////////////

	/* mask에 넣었던 docattr실제로 저장 (need newdocid) */
	if ( sb_run_docattr_set_array(newdocid, 1, SC_MASK, &docmask) != SUCCESS ) {
		warn("cannot save docattr db of document[%s]", oid);
	}

	/*******************
	 * cdm 파일에 기록 *
	 *******************/
	if ( sb_run_indexdb_append( db->ifs, *newdocid, size, (void*)xmldoc ) == FAIL ) {
		error("cdm_db(ifs) append failed. did[%u], oid[%s]", *newdocid, oid);
		goto error;
	}

	/************************
	 * shared memory update *
	 ************************/
    ACQUIRE_LOCK()
	db->shared->last_docid = *newdocid;
    RELEASE_LOCK()

	if ( oid_duplicated ) {
	    sb_run_xmlparser_free_parser(p);
		return CDM2_PUT_OID_DUPLICATED;
	}
	else {
	    sb_run_xmlparser_free_parser(p);
		return SUCCESS;
	}
error:
	sb_run_xmlparser_free_parser(p);
	return FAIL;
}

static int cdm_get_xmldoc(cdm_db_t* cdm_db, uint32_t docid, char* buf, size_t size)
{
    int ret = 0;
    int length = 0;
    cdm_db_custom_t* db = NULL;

	if ( cdm_set == NULL || !cdm_set[cdm_db->set].set )
		return MINUS_DECLINE;
	db = (cdm_db_custom_t*) cdm_db->db;

	length = sb_run_indexdb_getsize( db->ifs, docid );
	if ( length == INDEXDB_FILE_NOT_EXISTS ) {
		warn("length is 0 of did[%u]. something is wrong", docid);
		return FAIL;
	}
	else if ( length == FAIL ) {
		crit("cdm_db(ifs) getsize failed. did[%d]", docid);
		return FAIL;
	}

    if(length >= size) {
	    warn("document[%d] size[%d] larger than buffer size[%d]", docid, length, size);
	} else {
        size = length;
	}

	ret = sb_run_indexdb_read( db->ifs, docid, 0, size, (void*)buf );
	if ( ret < 0 ) {
		crit("cdm_db(ifs) read failed. did[%u], ret: %d", docid, ret);
		return FAIL;
	}

	DEBUG("ret[%d] of cdm_db(ifs) read with did:%u)",ret, docid);
	if ( ret < length ) {
		warn( "ret[%d] is less than length[%d]", ret, length);
        return FAIL;
	}

	return ret;
}

static uint32_t cdm_last_docid(cdm_db_t* cdm_db)
{
	cdm_db_custom_t* db;

	db = (cdm_db_custom_t*) cdm_db->db;

	return db->shared->last_docid;
}


/******************* not api from here *****************/
static void get_docattrfield(configValue v)
{
	if(strlen(v.argument[0]) >= SHORT_STRING_SIZE) {
		warn("max field size[%d], current size[%d], string[%s]", 
				SHORT_STRING_SIZE, strlen(v.argument[0]), v.argument[0]);
	}

	strncpy(docattr_fields[docattr_field_count], v.argument[0], SHORT_STRING_SIZE-1);
	docattr_field_count++;
}

static void get_field_root_name(configValue v)
{
	if(strlen(v.argument[0]) >= SHORT_STRING_SIZE) {
		warn("max root name size[%d], current size[%d], string[%s]", 
				SHORT_STRING_SIZE, strlen(v.argument[0]), v.argument[0]);
	}

	strncpy(field_root_name, v.argument[0], SHORT_STRING_SIZE-1);
}

static void setIndexDbSet(configValue v)
{
    ifs_db_set = atoi( v.argument[0] );
}

static void get_cdm_set(configValue v)
{   
    static cdm_db_set_t local_cdm_set[MAX_CDM_SET];
    int value = atoi( v.argument[0] );

    if ( value < 0 || value >= MAX_CDM_SET ) {
        error("Invalid CdmSet value[%s], MAX_CDM_SET[%d]",
                v.argument[0], MAX_CDM_SET);
        return;
    }

    if ( cdm_set == NULL ) {
        memset( local_cdm_set, 0, sizeof(local_cdm_set) );
        cdm_set = local_cdm_set;
    }

    current_cdm_set = value;
    cdm_set[value].set = 1;
}

static void get_cdm_path(configValue v)
{   
    char* cdm_path; 
    size_t path_len;
        
	if(strlen(v.argument[0]) >= MAX_FILE_LEN) {
		warn("max file name size[%d], current size[%d], string[%s]", 
				MAX_FILE_LEN, strlen(v.argument[0]), v.argument[0]);
	}

    if ( cdm_set == NULL || current_cdm_set < 0 ) {
        error("first, set CdmSet");
        return;
    }
    
    cdm_path = cdm_set[current_cdm_set].cdm_path;
    
    strncpy(cdm_path, v.argument[0], MAX_FILE_LEN-1);
    path_len = strlen( cdm_path );
    if ( cdm_path[path_len-1] == '/' ) cdm_path[path_len-1] = '\0';
}

static config_t config[] = {
	CONFIG_GET("DocAttrField", get_docattrfield, VAR_ARG, "..."),
	CONFIG_GET("FieldRootName", get_field_root_name, 1, \
			"canned document root element name"),
	CONFIG_GET("IndexDbSet",setIndexDbSet,1,
						"index db set (type is indexdb) (e.g: IndexDbSet 1)"),
    CONFIG_GET("CdmSet", get_cdm_set, 1, "CdmSet {number}"),
    CONFIG_GET("CdmPath", get_cdm_path, 1, "cdm directory path (relative, no trailing /)"),
	{NULL}
};

static void register_hooks(void)
{
	sb_hook_cdm_open( cdm_open, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdm_close( cdm_close, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdm_get_doc( cdm_get_doc, NULL, NULL, HOOK_MIDDLE );
	//sb_hook_cdmdoc_get_field_count( cdmdoc_get_field_count, NULL, NULL, HOOK_MIDDLE );
	//sb_hook_cdmdoc_get_field_names( cdmdoc_get_field_names, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdmdoc_get_field( cdmdoc_get_field, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdmdoc_get_field_by_bytepos( cdmdoc_get_field_by_bytepos, NULL, NULL, HOOK_MIDDLE );
	//sb_hook_cdmdoc_get_field_by_wordpos( cdmdoc_get_field_by_wordpos, NULL, NULL, HOOK_MIDDLE );
	//sb_hook_cdmdoc_update_field( cdmdoc_update_field, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdmdoc_destroy( cdmdoc_destroy, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdm_put_xmldoc( cdm_put_xmldoc, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdm_get_xmldoc( cdm_get_xmldoc, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdm_last_docid( cdm_last_docid, NULL, NULL, HOOK_MIDDLE );
}

module cdm3_module =
{
	STANDARD_MODULE_STUFF,
	config,                /* config */
	NULL,                  /* registry */
	init,                  /* initialize function */
	NULL,                  /* child_main */
	NULL,                  /* scoreboard */
	register_hooks         /* register hook api */
};
