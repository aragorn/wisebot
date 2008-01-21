/* $Id$ */
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "common_core.h"
#include "ipc.h"
#include "mod_httpd/protocol.h"
#include "mod_httpd/util_filter.h"
#include "mod_httpd/http_config.h"
#include "mod_httpd/http_util.h"
#include "mod_standard_handler.h"
#include "apr_strings.h"
#include "mod_api/sbhandler.h"
#include "handler_util.h"
#include "util.h"

//implemented in common_handler.c
int sbhandler_common_get_table(char *name_space, void **tab);
//implemented in document_handler.c
int sbhandler_document_get_table(char *name_space, void **tab);
//implemented in index_handler.c
int sbhandler_index_get_table(char *name_space, void **tab);

static int initialized = 0;
static int did_set = 1;
did_db_t* did_db = NULL;
static int cdm_set = 1;
cdm_db_t* cdm_db = NULL;
static int word_db_set = 1;
word_db_t* word_db = NULL;
static int index_db_set = 1;
index_db_t *index_db = NULL;

// 읽기 전용으로 사용해야 함. indexer module에서 write시에 동기화 하지 않음
indexer_shared_t* indexer_shared = NULL;

// query log
enum qlogtype{ CUSTOM, XML };
static enum qlogtype qlog_type = CUSTOM;
static int qlog_used = 1;
static int qlog_lock = -1;
static FILE* qlog_fp = NULL;
static char qlog_path[MAX_PATH_LEN] = "logs/query_log";
static void (*sighup_handler)(int sig) = NULL;
static void reopen_qlog(int sig);
static int init_index_info();
char indexer_shared_file[MAX_FILE_LEN] = "dat/indexer/indexer.shared";

static int init_db()
{
    int rv = 0;
	ipc_t lock;
	struct sigaction act, oldact;

	if ( initialized ) return SUCCESS;
	else initialized = 1;
	
	// DID_DB open
	rv = sb_run_open_did_db( &did_db, did_set ); 
	if ( rv != SUCCESS && rv != DECLINE ) { 
		error("did db open failed: did_set[%d]", did_set);
		return FAIL;
    }

	// CDM_DB open
	rv = sb_run_cdm_open( &cdm_db, cdm_set );
    if ( rv != SUCCESS && rv != DECLINE ) {
    	error( "cdm module open failed: cdm_set[%d]", cdm_set);
		return FAIL;
    }

    // WORD_DB open
    rv = sb_run_open_word_db( &word_db, word_db_set );
    if ( rv != SUCCESS && rv != DECLINE ) {
        error("word db open failed: word_db_set[%d]", word_db_set);
        return FAIL;
    }

    // INDEX_DB open
    rv = sb_run_indexdb_open( &index_db, index_db_set );
    if ( rv != SUCCESS && rv != DECLINE ) {
        error("index db open failed: index_db_set[%d]", index_db_set);
        return FAIL;
    }

	/* register HUP signal handler */
	memset(&act, 0x0, sizeof(act));
	act.sa_flags = SA_RESTART;
	sigfillset(&act.sa_mask);
	act.sa_handler = reopen_qlog;
	sigaction(SIGHUP, &act, &oldact);
	sighup_handler = oldact.sa_handler;

	/* open file */
	qlog_fp = sb_fopen(qlog_path, "a");
	if ( qlog_fp == NULL ) {
		error("%s open failed: %s", qlog_path, strerror(errno));
	}
	else setlinebuf(qlog_fp);

    // query log lock
	lock.type     = IPC_TYPE_SEM;
	lock.pid      = 0;
	lock.pathname = qlog_path;

	if ( get_sem(&lock) != SUCCESS ) return FAIL;
	qlog_lock = lock.id;

	if ( init_index_info() != SUCCESS ) return FAIL;

	return SUCCESS;
}

static int init_index_info() {
    ipc_t ipc;
    
    ipc.type = IPC_TYPE_MMAP;
    ipc.pathname = indexer_shared_file;
    ipc.size = sizeof(indexer_shared_t);

    if ( alloc_mmap(&ipc, 0) != SUCCESS ) {
        error("alloc mmap to indexer_shared failed");
        return FAIL;        
    }
    indexer_shared = (indexer_shared_t*) ipc.addr;

    if ( ipc.attr == MMAP_CREATED )
        memset( indexer_shared, 0, ipc.size );

    /* some information about some data structure */
    info("sizeof(standard_hit_t) = %d", sizeof(standard_hit_t));
    info("sizeof(hit_t) = %d", sizeof(hit_t));
    info("sizeof(doc_hit_t) = %d", sizeof(doc_hit_t));
    info("STD_HITS_LEN = %d", STD_HITS_LEN);

    return SUCCESS;
}

static void reopen_qlog(int sig)
{
	fclose(qlog_fp);

	qlog_fp = sb_fopen(qlog_path, "a");
	if ( qlog_fp == NULL ) {
		error("%s open failed: %s", qlog_path, strerror(errno));
	}
	else setlinebuf(qlog_fp);

	if ( sighup_handler != NULL
			&& sighup_handler != SIG_DFL && sighup_handler != SIG_IGN )
		sighup_handler(sig);
}

static void write_qlog(request_rec *r, softbot_handler_rec* s, int ret)
{
    request_t* req = s->req;
    response_t* res = s->res;
    char* query = apr_pstrdup(r->pool, req->query);

    if(req == NULL || res == NULL) return;

    ap_unescape_url(query);
    query = replace( query , '\n', '^');
    query = replace( query , '\r', ' ');

	if ( acquire_lock(qlog_lock) != SUCCESS ) {
		error("qlog lock failed.");
		return;
	}

    switch(qlog_type) {
        case CUSTOM:
		fprintf(qlog_fp, "[%s]"
						 "[%s]"
						 "[%d]"
						 "[%u]"
						 "[%u]"
						 "[%s]"
						 "[%s]"
						 "\n",
						  get_time("%Y-%m-%d %k:%M:%S"),
                          s->request_name,
						  ret,
						  res->search_result,
						  s->end_time - s->start_time,
						  res->parsed_query,
						  query);
        break;
        case XML:
		fprintf(qlog_fp, "<query_log>"
						 "<date>%s</date>"
						 "<request_name>%s</request_name>"
						 "<ret_code>%d</ret_code>"
						 "<result_count>%u</result_count>"
						 "<elapsed_time>%u</elapsed_time>"
						 "<parsed_morph><![CDATA[%s]]></parsed_morph>"
						 "<query><![CDATA[%s]]></query>"
						 "</query_log>\n",
						  get_time("%Y-%m-%d %k:%M:%S"),
                          s->request_name,
						  ret,
						  res->search_result,
						  s->end_time - s->start_time,
						  res->parsed_query,
						  query);
        break;
        default:
        break;
    }

	release_lock(qlog_lock);
    return;
}

//--------------------------------------------------------------//
//	*   custom function	
//--------------------------------------------------------------//
static int make_memfile_from_postdata(request_rec *r, memfile **output){
	apr_bucket_brigade *bb = NULL;
	int seen_eos, rv;
	
	memfile *mfile = memfile_new();
	if ( !mfile ) {
		error("memfile_new failed");
		return FAIL;
	}
	
	/* read POST data */
	bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);
	seen_eos = 0;
	do {
		apr_bucket *bucket;
		
		rv = ap_get_brigade(r->input_filters, bb, AP_MODE_READBYTES,
							APR_BLOCK_READ, HUGE_STRING_LEN);

		if (rv != APR_SUCCESS) {
			error("ap_get_brigade failed");
			memfile_free(mfile);
			return rv;
		}

		for (bucket = APR_BRIGADE_FIRST(bb);
		     bucket != APR_BRIGADE_SENTINEL(bb);
		     bucket = APR_BUCKET_NEXT(bucket))
		{
			const char *data;
			apr_size_t len;

			if (APR_BUCKET_IS_EOS(bucket)) {
				seen_eos = 1;
				break;
			}

			/* We can't do much with this. */
			if (APR_BUCKET_IS_FLUSH(bucket)) {
				continue;
			}

			/* read */
			apr_bucket_read(bucket, &data, &len, APR_BLOCK_READ);

			/* Keep writing data to the child until done or too much time
			 *              * elapses with no progress or an error occurs.
			 *                           */
			if ( memfile_append(mfile, (char *)data, len) != len ) {
				error("memfile_append failed");
				memfile_free(mfile);
				apr_brigade_cleanup(bb);
				return FAIL;
			}
		}
		apr_brigade_cleanup(bb);
	}
	while (!seen_eos);
	*output = mfile;
	memfile_setOffset(mfile, 0);
	return SUCCESS;
}

static int _sub_handler(request_rec *rec, softbot_handler_rec *s){
    int rv, id = 0;
    softbot_handler_key_t *tab = NULL;
   
	rv = init_db();
	if ( rv != SUCCESS ) { return FAIL; }

    rv =  sb_run_sbhandler_get_table(s->name_space, (void **) &tab);
	if ( rv == DECLINE) return rv;
    if ( rv != SUCCESS || !tab ) {
        error("sb_run_sbhandler_get_table() failed");
        return FAIL;
    }

    while ( tab[id].name ) {
        if ( strcmp(tab[id].name, s->request_name) == 0 ) {
            break;
        }
        id++;
    }

    if ( !tab[id].handler ) {
        error("no handler implemented");
        return FAIL;
    }

    return tab[id].handler(rec, s);
}

static int append_file(request_rec *r, char *path){
    FILE *fp;
    char buf[BUFSIZ];
    int readsize;

    if( (fp=sb_fopen(path, "r")) == NULL ) {
        error("file [%s] open failed :%s", path, strerror(errno));
        return FAIL;
    }

    while(!feof(fp)){
        readsize = fread(buf, sizeof(char), BUFSIZ, fp);
        if ( readsize < 0 ) {
            error("fread failed at [%s]", path);
            fclose(fp);
            return FAIL;
        }
        ap_rwrite(buf, readsize, r);
    }
    fclose(fp);
    return SUCCESS;
}


static int append_xml_msg_record(memfile *m, msg_record_t *msg){
    int nRet;
    char buf[MAX_RECORDED_MSG_LEN];

    if(msg == NULL) {
        warn("msg is null");
        return FAIL;
    }

    memfile_appendF(m, "<li>messages</li>\n<ul>\n");
    msg_record_rewind(msg);
    while(1){
        nRet = msg_record_read(msg, buf);
        if ( nRet != SUCCESS )  break;
        memfile_appendF(m, "\t<li>%s</li>\n", buf);
    }   

    memfile_appendF(m, "</ul>\n");

    return SUCCESS;
}  

static void _make_fail_response(request_rec *r, softbot_handler_rec *s, int ret_code){
    if( equals_content_type(r, "x-softbotd/binary") == TRUE) {
        return;
    } else {
        char* error = NULL;
        int error_size = 0;

	    memfile *mfile = memfile_new();
		if ( !mfile ) {
			error("memfile_new failed");
			return;
		}

		ap_set_content_type(r, "text/xml");

        memfile_appendF(mfile, 
				"<ul>\n"
				"<li>return code : %d</li>\n"
				"<li>reason : Response Failed</li>\n", ret_code);
		append_xml_msg_record(mfile, &s->msg);
        memfile_appendF(mfile, "</ul>");

        error_size = memfile_getSize(mfile);
        error = apr_palloc(r->pool, error_size + 1);
        memfile_setOffset(mfile, 0);
        memfile_read(mfile, error, error_size);
        error[error_size] = '\0';

		apr_table_set(r->notes, "error-notes", error);
		apr_table_set(r->notes, "verbose-error-to", "*");
        
        memfile_free(mfile);
    }

    return;
}

//--------------------------------------------------------------//
//	*	standard_search implemetation
//--------------------------------------------------------------//
static int standard_handler(request_rec *r)
{
        int nRet;
        softbot_handler_rec s;
	    struct timeval tv;
/*      const char *content_type = NULL;*/
        char *pos;

        /* init softbot_handler_rec */
		msg_record_init(&s.msg);

        //1. arranging ptrs
        s.name_space = NULL;
        s.request_name = NULL;
        s.remain_uri = NULL;
        s.req = NULL;
        s.res = NULL;
/*      s.request_body = NULL;*/
/*      s.request_content_type = NULL;*/
/*      s.response_body = NULL;*/
/*      s.response_content_type = NULL;*/

        s.parameters_in = apr_table_make(r->pool, 4);

        //2. parse uri
        if (r->parsed_uri.path && r->unparsed_uri) {
                char *path = apr_pstrdup(r->pool, r->parsed_uri.path);
                char *unparsed_uri = apr_pstrdup(r->pool, r->unparsed_uri);
                int sb_uri_len=strlen(path);

                pos = strchr(path, '/');
                if (!pos)       return DECLINE;
                s.name_space = pos + 1;
                pos = strchr(s.name_space, '/');
                if (!pos)       return DECLINE;
                *pos = '\0';
                s.request_name = pos + 1;

                pos = strchr(s.request_name, '/');
                if (pos)        {
                        sb_uri_len = pos - path;
                        *pos = '\0';
                } 

                if ( strlen(unparsed_uri) == strlen(path) )     s.remain_uri = NULL;
                else s.remain_uri = unparsed_uri + sb_uri_len;
        }else   return DECLINE;

        decodencpy((unsigned char*)r->unparsed_uri, (unsigned char*)r->unparsed_uri, strlen(r->unparsed_uri));

        INFO("unparsed_uri : [%s]", r->unparsed_uri);
        INFO("name_space[%s] request_name[%s] remain_uri[%s]",
                        s.name_space, s.request_name, (s.remain_uri) ? s.remain_uri : "null" );

        /* parse query */
        if (r->parsed_uri.query)
        {
                char *query;
                char *key, *val;
                char *strtok_buf = NULL;

                query = apr_palloc(r->pool, strlen(r->parsed_uri.query) + 1);

                strcpy(query, r->parsed_uri.query);

                key = apr_strtok(query, "&", &strtok_buf);

                while (key) {
                        val = strchr(key, '=');
                        if (val) { /* have a value */
                                *val++ = '\0';
                                decodencpy((unsigned char*)key, (unsigned char*)key, strlen(key));
                                decodencpy((unsigned char*)val, (unsigned char*)val, strlen(val));
                                apr_table_setn(s.parameters_in, key, val);
                        }
                        key = apr_strtok(NULL, "&", &strtok_buf);
                }
        }

#ifdef DEBUG_SOFTBOTD
        {
                const char *content_type = apr_table_get(r->headers_in, "Content-Type");
                INFO("Content-Type[%s]", (content_type) ? content_type : "null" );
        }
#endif

        //check is_admin
        //s.is_admin= def_atoi(apr_table_get(s.parameters_in, "is_admin"), 0);

		if ( qlog_used ) {
			gettimeofday(&tv, NULL);
			s.start_time = tv.tv_sec*1000 + tv.tv_usec/1000;
		}

        nRet = _sub_handler(r, &s);

		if ( qlog_used ) {
			gettimeofday(&tv, NULL);
			s.end_time = tv.tv_sec*1000 + tv.tv_usec/1000;

            write_qlog(r, &s, nRet);
		}
 
        if ( nRet != SUCCESS) {
            _make_fail_response(r, &s, nRet);
        }

        INFO(" handling request end: '%s/%s', '%s'", 
                        s.name_space, s.request_name, 
                        (s.remain_uri) ? s.remain_uri : "null" );

        if(nRet != SUCCESS) {
		    if(nRet == DECLINE) return nRet;

            return HTTP_INTERNAL_SERVER_ERROR;
		}
        else
            return SUCCESS;
}

static void register_hooks(void)
{
	sb_hook_handler(standard_handler,NULL,NULL,HOOK_REALLY_FIRST);
	sb_hook_sbhandler_get_table(sbhandler_common_get_table,NULL,NULL,HOOK_REALLY_LAST);
	sb_hook_sbhandler_get_table(sbhandler_document_get_table,NULL,NULL,HOOK_REALLY_LAST);
	sb_hook_sbhandler_get_table(sbhandler_index_get_table,NULL,NULL,HOOK_REALLY_LAST);
    sb_hook_sbhandler_append_file(append_file, NULL, NULL, HOOK_MIDDLE);
	sb_hook_sbhandler_make_memfile(make_memfile_from_postdata, NULL, NULL, HOOK_MIDDLE);
	return;
}

static void get_cdm_set(configValue v)
{
    cdm_set = atoi( v.argument[0] );
}

static void get_did_set(configValue v)
{
    did_set = atoi( v.argument[0] );
}

static void get_word_db_set(configValue v)
{
    word_db_set = atoi( v.argument[0] );
}

static void get_index_db_set(configValue v)
{
    index_db_set = atoi( v.argument[0] );
}

static void get_query_log_path(configValue v)
{
    if(v.argument[0]) {
        strncpy(qlog_path, v.argument[0], sizeof(qlog_path)-1);
    }
}

static void get_query_log_used(configValue v)
{
    qlog_used = atoi( v.argument[0] );
}

static void get_query_log_type(configValue v)
{
    qlog_type = atoi(v.argument[0]);
}

static void set_shared_file(configValue v)
{
    strncpy(indexer_shared_file, v.argument[0], MAX_FILE_LEN);
    indexer_shared_file[MAX_FILE_LEN-1] = '\0';
}

static config_t config[] = {
    CONFIG_GET("CdmSet", get_cdm_set, 1, "Cdm Set 0~..."),
    CONFIG_GET("DidSet", get_did_set, 1, "Did Set 0~..."),
    CONFIG_GET("IndexDbSet",get_index_db_set, 1,
            "index db set (type is indexdb) (e.g: IndexDbSet 1)"),
    CONFIG_GET("WordDbSet", get_word_db_set, 1, "WordDb Set 0~..."),
    CONFIG_GET("QueryLogPath", get_query_log_path, 1, "QueryLogPath log/query_log"),
    CONFIG_GET("QueryLogUsed", get_query_log_used, 1, "QueryLogUsed 1"),
    CONFIG_GET("QueryLogType", get_query_log_type, 1, "QueryLogType CUSTOM:0 XML:1"),
    CONFIG_GET("SharedFile",set_shared_file,1,"(e.g: SharedFile dat/indexer/indexer.shared)"),
    {NULL}
};

module standard_handler_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	NULL,					/* registry */
	NULL,                   /* initialize */
	NULL,				    /* child_main */
	NULL,				    /* scoreboard */
	register_hooks		    /* register hook api */
};

