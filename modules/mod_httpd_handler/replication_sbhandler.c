/* $Id$ */
#include "common_core.h"
#include "mod_standard_handler.h"
#include "handler_util.h"
#include "mod_httpd/http_util.h"
#include "mod_httpd/http_protocol.h"
#include "memory.h"
#include "apr_strings.h"
#include "mod_api/sbhandler.h"
#include "mod_api/cdm2.h"
#include <stdlib.h>

static char *canned_doc = NULL;

// function prototype
static int get_document(request_rec *r, softbot_handler_rec *s);

static softbot_handler_key_t replication_handler_tbl[] =
{	
	{"get_document", get_document},
	{NULL, NULL}
};

static int get_send_document_count( uint32_t master_docid, uint32_t slave_docid, uint32_t max_count)
{
    uint32_t diff = master_docid - slave_docid;

	if( diff > max_count ) {
	    return max_count;
	} else {
	    return diff;
	}
}

static int get_document(request_rec *r, softbot_handler_rec *s)
{
    int rv = 0;
    uint32_t master_docid = 0;
    uint32_t slave_docid = 0;
    char* str_slave_docid = 0;
	int zero = 0;

    if (canned_doc == NULL) {
        canned_doc = (char *)sb_malloc(DOCUMENT_SIZE);
        if (canned_doc == NULL) {
            MSG_RECORD(&s->msg, crit, "out of memory: %s", strerror(errno));
            return FAIL;
        }
    }
    
    str_slave_docid = apr_pstrdup(r->pool, apr_table_get(s->parameters_in, "current_did"));

    if( (str_slave_docid == NULL || strlen(str_slave_docid) == 0) ) {
        error("current_did is null");
        return FAIL;
    }

	slave_docid = atoi(str_slave_docid);
	master_docid = sb_run_cdm_last_docid(cdm_db);

	if( slave_docid >= master_docid ) {
	    info("replication document zero");
		ap_rwrite(&zero, sizeof(zero), r);
	} else {
	    int i = 0;
	    int doc_size = 0;
	    int count = get_send_document_count(master_docid, slave_docid, max_replication_count);

        //총건수 전송
		ap_rwrite(&count, sizeof(count), r);

        for( i = 1; i <= count; i++ ) {
			rv = sb_run_cdm_get_xmldoc(cdm_db, slave_docid+i, canned_doc, DOCUMENT_SIZE);
			if ( rv < 0 ) {
				error("cannot get document[%u]", slave_docid+i);
				return FAIL;
			}

            doc_size = strlen(canned_doc);

            // 문서크기 전송
			info("doc_size[%d]", doc_size);
			ap_rwrite(&doc_size, sizeof(doc_size), r);

            // 문서 전송
			ap_rwrite(canned_doc, doc_size, r);

	        info("replication : send document id[%u]", slave_docid+i);
		}
	}

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////
// name space 노출 함수
int sbhandler_replication_get_table(char *name_space, void **tab)
{
	if ( strcmp(name_space, "replication") != 0 ){
		return DECLINE;
	}

	*tab = replication_handler_tbl;

	return SUCCESS;
}
