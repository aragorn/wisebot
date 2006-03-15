/* $Id$ */
#include <string.h>
#include <ctype.h>
#include "mod_api/sbhandler.h"
#include "mod_httpd/protocol.h"
#include "mod_httpd/conf.h"
#include "mod_standard_handler.h"
#include "apr_strings.h"

//implemented in common_handler.c
int sbhandler_common_get_table(char *name_space, void **tab);

static int hex(unsigned char h)
{
	if (isdigit(h))
		return h-'0';
	else
		return toupper(h)-'A'+10;
}

static inline void decodencpy(unsigned char *dst, unsigned char *src, int n)
{
	register int x, y;

	x = y = 0;
	while(src[x]) {
		if (src[x] == '+')
			dst[y] = ' ';
		else if (src[x] == '%' && n - x >= 2 
				&& isxdigit((int)(src[x+1])) && isxdigit((int)(src[x+2]))) {
			dst[y] = (hex(src[x+1]) << 4) + hex(src[x+2]);
			x += 2;
		}
		else
			dst[y] = src[x];
		x++;
		y++;
		if (x >= n)
			break;
	}
	if (y < n)
		dst[y] = 0;
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

static void _make_fail_response(request_rec *r, softbot_handler_rec *s, int ret_code){
/*
    ap_set_content_type(r, "text/xml");
    ap_rprintf(r,
            "<?xml version=\"1.0\" encoding=\"euc-kr\" ?>"
            "<xml>\n<retcode>%d</retcode>\n"
            "<result>Response Failed</result>\n", ret_code);
    append_xml_msg_record(r, &s->msgs);
    ap_rprintf(r, "</xml>");
*/
    return;
}

/*****************************************************************************/
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

