#include <stdlib.h>
#include "mod_standard_handler.h"
#include "handler_util.h"
#include "mod_httpd/http_util.h"
#include "mod_httpd/http_protocol.h"
#include "memory.h"
#include "memfile.h"
#include "apr_strings.h"
#include "mod_api/sbhandler.h"
#include "mod_api/docattr2.h"
#include "mod_api/rmas.h"
#include "mod_api/cdm2.h"
#include "mod_api/index_word_extractor.h"
#include "mod_api/xmlparser.h"

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
static int document_ma(request_rec *r, softbot_handler_rec *s);

// util function
static char *get_field_and_ma_id_from_meta_data(char *sptr , char* field_name,  int *id);

static softbot_handler_key_t document_handler_tbl[] =
{	
	{"insert", document_insert},
	{"update", document_insert},  // insert와 동일
	{"delete", document_delete},
	{"select", document_select},
	{"ma",   document_ma},
	{NULL, NULL}
};

/////////////////////////////////////////////////////////////////////////
static char *get_field_and_ma_id_from_meta_data(char *sptr , char* field_name,  int *id)
{
    int i;
    char ids[4];

    for(;*sptr && *sptr != ':' ; sptr++, field_name++)
    {
        *field_name = *sptr;
    }
    *field_name = '\0';
    sptr++;

    for(i=0;*sptr && *sptr != '^' && i < MAX_FIELD_NAME_LEN ; sptr++, i++)
    {
        ids[i] = *sptr;
    }
    ids[i] = '\0';
    sptr++;

    *id = atol(ids);
//  INFO("morp-id: %d", *id);

    return sptr;

}
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

    OID = apr_pstrdup(r->pool, apr_table_get(s->parameters_in, "OID"));

    if(OID == NULL || strlen(OID) == 0) {
        MSG_RECORD(&s->msg, error, "oid is null, get parameter exist with oid.");
        return FAIL;
    }
    
    // 이미 존재하는 문서는 삭제.
	rv = sb_run_get_docid(did_db, OID, &docid);
	if ( rv == SUCCESS ) document_delete(r, s);

    decodencpy(OID, OID, strlen(OID));

    rv = sb_run_sbhandler_make_memfile(r, &request_body);
    if(rv != SUCCESS) {
        MSG_RECORD(&s->msg, error, "cannot get POST data");
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
        MSG_RECORD(&s->msg, error, "cannot insert document, max size[%d], current size[%ld]",
                                  DOCUMENT_SIZE, memfile_getSize(request_body));
		if(request_body != NULL) memfile_free(request_body);
        return FAIL;
    }

    rv = memfile_read(request_body, canned_doc, memfile_getSize(request_body));
    if(rv != memfile_getSize(request_body)) {
        MSG_RECORD(&s->msg, error, "cannot read memfile");
		if(request_body != NULL) memfile_free(request_body);
        return FAIL;
    }
    canned_doc[memfile_getSize(request_body)] = '\0';
	debug("memfile_getSize(request_body) = [%ld]", memfile_getSize(request_body));

    decodencpy(canned_doc, canned_doc, strlen(canned_doc));

    document = strchr(canned_doc, '=');
    if(document == NULL || document - canned_doc > 20) {
        MSG_RECORD(&s->msg, error, "cannot find body");
		if(request_body != NULL) memfile_free(request_body);
        return FAIL;
    }

    document++;
	debug("xml doc[%s]", document);

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

    OID = apr_pstrdup(r->pool, apr_table_get(s->parameters_in, "OID"));
    DID = apr_pstrdup(r->pool, apr_table_get(s->parameters_in, "DID"));

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

    OID = apr_pstrdup(r->pool, apr_table_get(s->parameters_in, "OID"));
    DID = apr_pstrdup(r->pool, apr_table_get(s->parameters_in, "DID"));

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

static int document_ma(request_rec *r, softbot_handler_rec *s)
{
    int rv = 0;
    char* metadata = NULL;
    char* document = NULL;
    char* p = NULL;
    void *parser = NULL;
	char* field_value; int field_length;
    sb4_merge_buffer_t merge_buffer;
    memfile* request_body = NULL;
	char field_name[MAX_FIELD_NAME_LEN];
	int ma_id = 0;
    int field_id = 0;
	int data_size =0;
	uint32_t buffer_size=0;
	char *buffer = NULL;
    int is_binary = FAIL;

    is_binary = equals_content_type(r, "x-softbotd/binary");

    metadata = apr_pstrdup(r->pool, apr_table_get(s->parameters_in, "metadata"));

    if( metadata == NULL || strlen(metadata) == 0 ) {
        MSG_RECORD(&s->msg, error, "metadata is null, get parameter exist with metadata.");
        return FAIL;
    }

    if (canned_doc == NULL) {
        canned_doc = (char *)sb_malloc(DOCUMENT_SIZE);
        if (canned_doc == NULL) {
            MSG_RECORD(&s->msg, crit, "out of memory: %s", strerror(errno));
            return FAIL;
        }
    }
    
    if(metadata != NULL) decodencpy(metadata, metadata, strlen(metadata));

    rv = sb_run_sbhandler_make_memfile(r, &request_body);
    if(rv != SUCCESS) {
        MSG_RECORD(&s->msg, error, "can not post data");
		if(request_body != NULL) memfile_free(request_body);
        return FAIL;
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

    parser = sb_run_xmlparser_parselen("CP949" , (char *)document, strlen(document));
    if (parser == NULL) { 
        MSG_RECORD(&s->msg, error, "cannot parse document");
		if(request_body != NULL) memfile_free(request_body);
        return FAIL;
    }

    p = metadata;
    merge_buffer.data = NULL;
    merge_buffer.data_size = 0;
    merge_buffer.allocated_size = 0;

    do {
        char path[STRING_SIZE];
        char *field_id_ptr=0x00;
        char fid[3];
        int  field_id_is_given = 0;
        void* tmp_data = NULL;

        p = get_field_and_ma_id_from_meta_data(p , field_name,  &ma_id);
        info("fieldname: %s", field_name);

        field_id_ptr = strchr(field_name, '#');

        if (field_id_ptr != NULL) 
            field_id_is_given = 1;

        if (field_id_is_given == 1) {
            strcpy(fid, field_id_ptr+1);
            *field_id_ptr = '\0';
            field_id = atol(fid);
        }
        info("fieldname[%s] id[%d]", field_name, field_id);

        sprintf(path, "/Document/%s", field_name);
        path[STRING_SIZE-1] = '\0';
        rv = sb_run_xmlparser_retrieve_field(parser , path, &field_value, &field_length);

        if (field_id_is_given) {
            if (rv != SUCCESS) {
                warn("cannot retrieve field[%s]", path);
                continue;
            }

            if (field_length == 0) {
                continue;
            }
        } else {
            if (rv != SUCCESS) {
                warn("cannot retrieve field[%s]", path);
                field_id++;
                continue;
            }

            if (field_length == 0) {
                field_id++;
                continue;
            }
        }

        if ( field_length+1 > buffer_size ) {
            buffer = apr_palloc(r->pool, sizeof(char) * (field_length+1));
            buffer_size = field_length + 1;
        }

        memcpy(buffer, field_value, field_length);
        buffer[field_length] = '\0';

        info("starting ma [%s:%d] field_value:[%s]", field_name, field_id, buffer);

        tmp_data = NULL;
        rv = sb_run_rmas_morphological_analyzer(field_id, buffer, &tmp_data,
                &data_size, ma_id);
        if (rv == FAIL) {
            MSG_RECORD(&s->msg, warn, "failed to do morp.. analysis for doc(while analyzing)");
            sb_run_xmlparser_free_parser(parser);
            return FAIL;
        }
        info("finish ma: data_size: %d", data_size);

        rv = sb_run_rmas_merge_index_word_array( &merge_buffer , tmp_data , data_size);
        if (rv == FAIL) {
            MSG_RECORD(&s->msg, warn, "failed to do morp.. analysis for doc(while merging)");
            sb_free(tmp_data);
            sb_run_xmlparser_free_parser(parser);
            return FAIL;
        }
        sb_free(tmp_data);

        field_id++;
    } while (*p);

    sb_run_xmlparser_free_parser(parser);

    if(is_binary == SUCCESS) {
        ap_rwrite(merge_buffer.data, merge_buffer.data_size, r);
    } else {
        int i;
        index_word_t *idx = NULL;
        int cnt = merge_buffer.data_size / sizeof(index_word_t);

        ap_rwrite("<?xml version=\"1.0\" encoding=\"euc-kr\" ?>", strlen("<?xml version=\"1.0\" encoding=\"euc-kr\" ?>"), r);
        ap_rwrite("<xml>\n", strlen("<xml>\n"), r);
        ap_rprintf(r, "<words count=\"%d\">", cnt);
        for(i = 0; i < cnt; i++) {
            idx = (index_word_t*)merge_buffer.data + i;

            ap_rprintf(r, "<word pos = \"%d\"  field=\"%d\"><![CDATA[%s]]></word>\n", idx->pos, idx->field, idx->word);
        }
        ap_rprintf(r, "</words>");
        ap_rwrite("</rmas>\n", strlen("</rmas>\n"), r);
    }

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
