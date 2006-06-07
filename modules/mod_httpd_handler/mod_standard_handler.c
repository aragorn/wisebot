/* $Id$ */
#include <string.h>
#include <ctype.h>
#include "mod_httpd/protocol.h"
#include "mod_httpd/conf.h"
#include "mod_standard_handler.h"
#include "apr_strings.h"
#include "mod_api/sbhandler.h"
#include "handler_util.h"

//implemented in common_handler.c
int sbhandler_common_get_table(char *name_space, void **tab);

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

		APR_BRIGADE_FOREACH(bucket, bb) {
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
    int nRet, id = 0;
    softbot_handler_key_t *tab = NULL;
    
    nRet =  sb_run_sbhandler_get_table(s->name_space, (void **) &tab);

    if ( nRet != SUCCESS || !tab ){
        error("sb_run_sbrh_get_request_table failed");
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


static int append_xml_msg_record(request_rec *r, msg_record_t *msg){
    int nRet;
    char buf[MAX_RECORDED_MSG_LEN];

    if(msg == NULL) {
        warn("msg is null");
        return FAIL;
    }

    if (strncmp(r->content_type, "text/xml", 8) != 0 ){/*strlen("text/xml")*/
        error("invalid content_type[%s]", r->content_type);
        return FAIL;
    }   
        
    ap_rprintf(r, "<messages>\n");
    msg_record_rewind(msg);
    while(1){
        nRet = msg_record_read(msg, buf);
        if ( nRet != SUCCESS )  break;
        ap_rprintf(r, "\t<msgline><![CDATA[%s]]></msgline>\n", buf);
    }   

    ap_rprintf(r, "</messages>\n");
    return SUCCESS;
}  

static void _make_fail_response(request_rec *r, softbot_handler_rec *s, int ret_code){
    ap_set_content_type(r, "text/xml");
    ap_rprintf(r,
            "<?xml version=\"1.0\" encoding=\"euc-kr\" ?>"
            "<xml>\n<retcode>%d</retcode>\n"
            "<result>Response Failed</result>\n", ret_code);
	append_xml_msg_record(r, &s->msg);
    ap_rprintf(r, "</xml>");

    return;
}

//--------------------------------------------------------------//
//	*	standard_search implemetation
//--------------------------------------------------------------//
static int standard_handler(request_rec *r)
{
        int nRet;
        softbot_handler_rec s;
/*      const char *content_type = NULL;*/
        char *pos;

        /* init softbot_handler_rec */

        //1. arranging ptrs
        s.name_space = NULL;
        s.request_name = NULL;
        s.remain_uri = NULL;
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
                                decodencpy(key, key, strlen(key));
                                decodencpy(val, val, strlen(val));
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

        nRet = _sub_handler(r, &s);
        if ( nRet != SUCCESS ) {
            _make_fail_response(r, &s, nRet);
        }
        INFO(" handling request end: '%s/%s', '%s'", 
                        s.name_space, s.request_name, 
                        (s.remain_uri) ? s.remain_uri : "null" );

        //XXX 
        //Every request will be answered by 200.

        return SUCCESS;
}

static void register_hooks(void)
{
	sb_hook_handler(standard_handler,NULL,NULL,HOOK_REALLY_FIRST);
	sb_hook_sbhandler_get_table(sbhandler_common_get_table,NULL,NULL,HOOK_REALLY_LAST);
    sb_hook_sbhandler_append_file(append_file, NULL, NULL, HOOK_MIDDLE);
	sb_hook_sbhandler_make_memfile(make_memfile_from_postdata, NULL, NULL, HOOK_MIDDLE);
	return;
}

module standard_handler_module = {
	STANDARD_MODULE_STUFF,
	NULL,					/* config */
	NULL,					/* registry */
	NULL,  /* initialize */
	NULL,				    /* child_main */
	NULL,				    /* scoreboard */
	register_hooks		    /* register hook api */
};

