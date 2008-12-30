/* $Id$ */
#ifndef _MOD_STANDARD_HANDLER_H_
#define _MOD_STANDARD_HANDLER_H_

#include "apr_tables.h"
#include "mod_httpd/mod_httpd.h"
#include "msg_record.h"
#include "mod_api/cdm2.h"
#include "mod_api/lexicon.h"
#include "mod_api/indexdb.h"
#include "mod_api/indexer.h"
#include "mod_api/did.h"
#include "mod_api/qp2.h"

typedef struct softbot_handler_rec softbot_handler_rec;

struct softbot_handler_rec {
	char *name_space;
	char *request_name;
	char *remain_uri;
	apr_table_t	*parameters_in;
	msg_record_t msg;

    request_t* req;
    response_t* res;
    uint32_t start_time;
    uint32_t end_time;
};

typedef struct {
    char *name;
    /* interfaces used when clients send requests */        
    int (*handler)(request_rec *r, softbot_handler_rec *s);
} softbot_handler_key_t; 

//--------------------------------------------------------------//
//  macro's used to handle request
//--------------------------------------------------------------//
#define CHECK_REQUEST_CONTENT_TYPE(rec, content_type) \
{ \
	const char *req_type = NULL; \
	req_type = apr_table_get(rec->headers_in, "Content-Type"); \
	if ( !content_type ) { \
		if ( req_type ) {	\
			error("content_type should be NULL but [%s]", req_type); \
			return FAIL;	\
		} \
	}else if ( !req_type || \
			strncmp(req_type, content_type, strlen(content_type)) != 0 ){ \
		error("[%s] is not expected content_type", \
				(req_type) ? req_type : "null" ); \
		return FAIL;	\
	}	\
}

#endif

/* external value */

extern did_db_t* did_db;
extern cdm_db_t* cdm_db;
extern word_db_t* word_db;
extern index_db_t* index_db;
extern indexer_shared_t* indexer_shared;
extern int max_replication_count;
extern char default_charset[SHORT_STRING_SIZE+1];
