#include <stdlib.h>
#include "mod_standard_handler.h"
#include "mod_api/sbhandler.h"
#include "mod_api/docattr2.h"
#include "mod_httpd/http_util.h"
#include "mod_httpd/http_protocol.h"
#include "memory.h"
#include "memfile.h"

static char *canned_doc = NULL;

// register log
/*
static int rglog_lock = -1;
static FILE* rglog_fp = NULL; 
static char rglog_path[MAX_PATH_LEN] = "logs/register_log";
static void (*sighup_handler)(int sig) = NULL; 
    
static void write_register_log(const char* type, const char* format, ...);
#define RGLOG_INFO(format, ...) \
    info(format, ##__VA_ARGS__); \
    write_register_log("info", format, ##__VA_ARGS__);
#define RGLOG_WARN(format, ...) \
    warn(format, ##__VA_ARGS__); \
    write_register_log("warn", format, ##__VA_ARGS__);
#define RGLOG_ERROR(format, ...) \
    error(format, ##__VA_ARGS__); \
    write_register_log("error", format, ##__VA_ARGS__);
*/


// function prototype
static int document_insert(request_rec *r, softbot_handler_rec *s);
static int document_delete(request_rec *r, softbot_handler_rec *s);
static int document_select(request_rec *r, softbot_handler_rec *s);

static softbot_handler_key_t document_handler_tbl[] =
{	
	{"insert", document_insert},
	{"update", document_insert},  // insert와 동일
	{"delete", document_delete},
	{"select", document_select},
	{NULL, NULL}
};

/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
// 문서 등록/수정 
static int document_insert(request_rec *r, softbot_handler_rec *s)
{
    int rv = 0;
    uint32_t docid = 0;
    uint32_t olddocid = 0;
    char* OID = 0;
    char* document = NULL;
    memfile* request_body = NULL;

    OID = (char*)apr_table_get(s->parameters_in, "OID");

    if(OID == NULL || strlen(OID) == 0) {
        MSG_RECORD(&s->msg, error, "oid is null, get parameter exist with oid.");
        return FAIL;
    }
    
    // 이미 존재하는 문서는 삭제.
    document_delete(r, s);

    decodencpy(OID, OID, strlen(OID));

    rv = sb_run_sbhandler_make_memfile(r, &request_body);
    if(rv != SUCCESS) {
        MSG_RECORD(&s->msg, error, "can not post data");
		if(request_body != NULL) memfile_free(request_body);
        return FAIL;
    }

    if (canned_doc == NULL) {
        canned_doc = (char *)sb_malloc(DOCUMENT_SIZE);
        if (canned_doc == NULL) {
            MSG_RECORD(&s->msg, crit, "out of memory: %s", strerror(errno));
		    if(request_body != NULL) memfile_free(request_body);
            return FAIL;
        }
    }

    if(memfile_getSize(request_body) >= DOCUMENT_SIZE) {
        MSG_RECORD(&s->msg, error, "can not insert document, max size[%d], current size[%ld]",
                                  DOCUMENT_SIZE, memfile_getSize(request_body));
		if(request_body != NULL) memfile_free(request_body);
        return FAIL;
    }

    rv = memfile_read(request_body, canned_doc, memfile_getSize(request_body));
    if(rv != memfile_getSize(request_body)) {
        MSG_RECORD(&s->msg, error, "can not read memfile");
		if(request_body != NULL) memfile_free(request_body);
        return FAIL;
    }
    canned_doc[memfile_getSize(request_body)] = '\0';

    decodencpy(canned_doc, canned_doc, strlen(canned_doc));

    document = strchr(canned_doc, '=');
    if(document == NULL || document - canned_doc > 20) {
        MSG_RECORD(&s->msg, error, "can not find body");
		if(request_body != NULL) memfile_free(request_body);
        return FAIL;
    }

    document++;
info("xml doc[%s]", document);

	rv = sb_run_cdm_put_xmldoc(cdm_db, did_db, OID,
			document, strlen(document), &docid, &olddocid);
	switch ( rv ) {
		case CDM2_PUT_NOT_WELL_FORMED_DOC:
			//RGLOG_ERROR("cannot register canned document[%s]. not well formed document", OID);
			break;
		case CDM2_PUT_OID_DUPLICATED:
			//RGLOG_INFO("doc[%u] is deleted. OID[%s] is registered by docid[%u]", olddocid, OID, docid);
			break;
		case SUCCESS:
			//RGLOG_INFO("OID[%s] is registered by docid[%u]", OID, docid);
			break;
		default:
			//RGLOG_ERROR("cannot register canned document[%s] because of error(%d)", OID, n);
			break;
	}

	if(request_body != NULL) memfile_free(request_body);
    return SUCCESS;
}

static int document_delete(request_rec *r, softbot_handler_rec *s)
{
    int rv = 0;
    uint32_t docid = 0;
    char* OID = 0;
    char* DID = 0;
    docattr_t* docattr = NULL;

    OID = (char*)apr_table_get(s->parameters_in, "oid");
    DID = (char*)apr_table_get(s->parameters_in, "did");

    if( (OID == NULL || strlen(OID) == 0) &&
        (DID == NULL || strlen(DID) == 0) ) {
        MSG_RECORD(&s->msg, error, "oid, did is null, get parameter exist with oid or did.");
        return FAIL;
    }
    
    if(OID != NULL) decodencpy(OID, OID, strlen(OID));

	if(DID == NULL) {
		rv = sb_run_get_docid(did_db, OID, &docid);
		if ( rv < 0 ) {                                 
			MSG_RECORD(&s->msg, error, "cannot get docid of OID[%s]", OID);
			return FAIL;
		}  

		if( rv == DOCID_NOT_REGISTERED ) {
			MSG_RECORD(&s->msg, error, "not registerd OID[%s]", OID);
			return FAIL;
		}
	} else {
		docid = atoi(DID);
	}

    rv = sb_run_docattr_ptr_get(docid, &docattr);
	if ( rv < 0 ) {                                 
        MSG_RECORD(&s->msg, error, "cannot get docattr of OID[%s]", OID);
        return FAIL;
	}  

    rv = sb_run_docattr_set_docattr_function(docattr, "Delete", "1");
	if ( rv < 0 ) {                                 
        MSG_RECORD(&s->msg, error, "cannot delete OID[%s]", OID);
        return FAIL;
	}  

    return SUCCESS;
}

static int document_select(request_rec *r, softbot_handler_rec *s)
{
    int rv = 0;
    uint32_t docid = 0;
    char* OID = 0;
    char* DID = 0;

    OID = (char*)apr_table_get(s->parameters_in, "oid");
    DID = (char*)apr_table_get(s->parameters_in, "did");

    if( (OID == NULL || strlen(OID) == 0) &&
        (DID == NULL || strlen(DID) == 0) ) {
        MSG_RECORD(&s->msg, error, "oid, did is null, get parameter exist with oid or did.");
        return FAIL;
    }

    if (canned_doc == NULL) {
        canned_doc = (char *)sb_malloc(DOCUMENT_SIZE);
        if (canned_doc == NULL) {
            MSG_RECORD(&s->msg, crit, "out of memory: %s", strerror(errno));
            return FAIL;
        }
    }
    
    if(OID != NULL) decodencpy(OID, OID, strlen(OID));

	if(DID == NULL) {
		rv = sb_run_get_docid(did_db, OID, &docid);
		if ( rv < 0 ) {                                 
			MSG_RECORD(&s->msg, error, "cannot get docid of OID[%s]", OID);
			return FAIL;
		}  

		if( rv == DOCID_NOT_REGISTERED ) {
			MSG_RECORD(&s->msg, error, "not registerd OID[%s]", OID);
			return FAIL;
		}
	} else {
		docid = atoi(DID);
	}

	rv = sb_run_cdm_get_xmldoc(cdm_db, docid, canned_doc, DOCUMENT_SIZE);
	if ( rv < 0 ) {
		MSG_RECORD(&s->msg, error, "cannot get document[%u]", docid);
		return FAIL;
	}

    ap_rwrite("<?xml version=\"1.0\" encoding=\"euc-kr\" ?>", strlen("<?xml version=\"1.0\" encoding=\"euc-kr\" ?>"), r);
    ap_rwrite(canned_doc, strlen(canned_doc), r);

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////
// name space 노출 함수
int sbhandler_document_get_table(char *name_space, void **tab)
{
	if ( strcmp(name_space, "document") != 0 ){
		return DECLINE;
	}

	*tab = document_handler_tbl;

	return SUCCESS;
}
