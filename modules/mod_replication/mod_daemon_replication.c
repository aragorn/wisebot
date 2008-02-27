/* $Id$ */
#include "common_core.h"
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "memory.h"
#include "ipc.h"
#include "setproctitle.h"
#include "common_util.h"
#include "mod_api/qp2.h"
#include "mod_api/did.h"
#include "mod_api/cdm2.h"
#include "mod_api/xmlparser.h"
#include "mod_api/http_client.h"

#include "mod_daemon_replication.h"

#define MONITORING_PERIOD      (2)
static scoreboard_t scoreboard[] = { PROCESS_SCOREBOARD(1) };

static RETSIGTYPE _do_nothing(int sig) { return; }

static int did_set = 1;
static did_db_t* did_db = NULL;
static int cdm_set = 1;
static cdm_db_t* cdm_db = NULL;
static http_client_t* client = NULL;
static char ip[STRING_SIZE];
static char port[SHORT_STRING_SIZE];
static char *canned_doc = NULL;
static struct timeval timeout;
static int min_delay_time_sec = 10;
static int max_delay_time_sec = 60;
static int delay_time_sec     = 10;

static char field_root_name[SHORT_STRING_SIZE] = "Document";
static char oid_name[SHORT_STRING_SIZE] = "OID";

static RETSIGTYPE _shutdown(int sig)
{
	struct sigaction act;

	memset(&act, 0x00, sizeof(act));

	sigfillset(&act.sa_mask);

	act.sa_handler = _do_nothing;
	sigaction(SIGHUP, &act, NULL);
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGQUIT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);

	scoreboard->shutdown++;
}

static RETSIGTYPE _graceful_shutdown(int sig)
{
	struct sigaction act;

	memset(&act, 0x00, sizeof(act));

	sigfillset(&act.sa_mask);

	act.sa_handler = _do_nothing;
	sigaction(SIGHUP, &act, NULL);
	sigaction(SIGINT, &act, NULL);

	scoreboard->graceful_shutdown++;
}

static int init()
{
    int rv = 0;
	
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

    // http client create
	client = sb_run_http_client_new( ip, port );
	if ( client == NULL ) {
	    error("can not create http client, ip[%s], port[%s]", ip, port);
		return FAIL;
	}

    if (canned_doc == NULL) {
        canned_doc = (char *)sb_malloc(DOCUMENT_SIZE);
        if (canned_doc == NULL) {
            error("out of memory: %s", strerror(errno));
            return FAIL;
        }
    }

	return SUCCESS;
}

char *_trim(char *str, int *len)
{
    char *tmp, *start;

    if (*len == 0) return str;

    start = str;
    for (tmp=str; tmp<str+*len; tmp++) {
        if (*tmp != ' ' && *tmp != '\t' && *tmp != '\n' && *tmp != '\r') {
            start = tmp;
            break;
        }
    }
    if (tmp == str + *len) {
        *len = 0;
        return str;
    }
    for (tmp=str+*len-1; tmp>=start; tmp--) {
        if (*tmp != ' ' && *tmp != '\t' && *tmp != '\n' && *tmp != '\r') {
            break;
        }
    }
    *len = tmp - start + 1;
    return start;
}

static int get_oid(void* xml, char* buf, size_t size)
{
    char* field_value; int field_length;
    char path[STRING_SIZE];
    char* val = NULL;
    int ret, len = 0;

    strcpy(path, "/");
    strcat(path, field_root_name);
    strcat(path, "/");
    strcat(path, oid_name);

    ret = sb_run_xmlparser_retrieve_field(xml, path, &field_value, &field_length);
    if (ret == FAIL) {
        warn("cannot get field[/%s/%s] (path:%s)",
                field_root_name,
                oid_name, path);
        return FAIL;
    }
    else if ( ret == DECLINE ) {
        warn("sb_run_xmlparser_retrieve_field() returned DECLINE(1)");
        return FAIL;
    }

    len = field_length;
    val = _trim(field_value, &len);
    len = (len>size-1)?size-1:len;
    strncpy(buf, val, len);
    buf[len] = '\0';

    return len;
}

static int register_document( memfile*  buf )
{
    int body_size = 0;
    int rv = 0;
    uint32_t docid = 0;
    uint32_t olddocid = 0;
    char oid[STRING_SIZE];
    void* p = NULL;
    docattr_t* docattr = NULL;

	if( buf == NULL ) {
		error("content buf is NULL");
        return FAIL;
	}

	if(client->parsing_status.state != PARSING_COMPLETE) {
		error("parse response error");
        return FAIL;
	}

	memfile_setOffset(buf, 0);

	body_size = memfile_getSize(buf);

	if(body_size == 0) {
		error("data size zero");
        return FAIL;
	}

	setproctitle("softbotd: %s register documents", __FILE__);

	p = NULL;

	// 문서 읽기
	if ( memfile_read( buf, canned_doc, body_size) != body_size ){
		error("incomplete result document[%d]", body_size);
		return FAIL;
	}
	canned_doc[body_size] = '\0';

	// OID 추출을 위한 xml 파싱
	p = sb_run_xmlparser_parselen("CP949", canned_doc, body_size);
	if ( p == NULL ) {
		error("cannot parse document");
		return FAIL;
	}

	if( get_oid( p, oid, STRING_SIZE ) == FAIL ) {
		sb_run_xmlparser_free_parser(p);
		return FAIL;
	} else {
		sb_run_xmlparser_free_parser(p);
	}

	// OID를 대상으로 이미 존재하는 문서는 삭제.
	rv = sb_run_get_docid(did_db, oid, &docid);
	if ( rv < 0 ) {
		rv = sb_run_docattr_ptr_get(docid, &docattr);
		if ( rv < 0 ) {
			info("cannot get docattr of OID[%s]", oid);
			return SUCCESS;
		}

		rv = sb_run_docattr_set_docattr_function(docattr, "Delete", "1");
		if ( rv < 0 ) {
			info("cannot delete OID[%s]", oid);
			return SUCCESS;
		}
	}

	rv = sb_run_cdm_put_xmldoc(cdm_db, did_db, oid,
			canned_doc, strlen(canned_doc), &docid, &olddocid);
	switch ( rv ) {
		case CDM2_PUT_NOT_WELL_FORMED_DOC:
			warn("cannot register canned document[%s]. not well formed document", oid);
			break;
		case CDM2_PUT_OID_DUPLICATED:
			warn("doc[%u] is deleted. OID[%s] is registered by docid[%u]", olddocid, oid, docid);
			break;
		case SUCCESS:
			info("OID[%s] is registered by docid[%u]", oid, docid);
			break;
		default:
			error("cannot register canned document[%s]", oid);
			break;
	}

    return SUCCESS;
}

static void sleep(slot_t *slot)
{
	setproctitle("softbotd: %s sleep[%d], master[%s:%s]", __FILE__, delay_time_sec, ip, port);

	timeout.tv_sec = delay_time_sec;
	timeout.tv_usec = 0;

	slot->state = SLOT_WAIT;
	select(0, NULL, NULL, NULL, &timeout);
	slot->state = SLOT_PROCESS;
}

static int replication_main(slot_t *slot)
{
	char path[MAX_QUERY_STRING_SIZE];
    uint32_t last_registered_docid = 0;

	slot->state = SLOT_PROCESS;

	while (1) { /* endless while loop */
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown )
			break;

	    sb_run_http_client_reset( client );
		last_registered_docid = sb_run_cdm_last_docid(cdm_db);
		if ( snprintf(path, MAX_QUERY_STRING_SIZE, 
					  "/document/select?did=%u", 
					  last_registered_docid+1) <= 0 )
		{
			error("path string is too long, max[%d]", MAX_QUERY_STRING_SIZE);
			goto error_return;
		}

        client->http->request_http_ver = 1001;
        client->http->method = "GET";
        client->http->host = "softbot";
        client->http->req_content_type = "x-softbotd/binary";
		client->http->path = path;

        if ( sb_run_http_client_makeRequest(client, NULL)   != SUCCESS ) {
            error("sb_run_http_client_makeRequest failed");
            goto sleep;
        }

		setproctitle("softbotd: %s connecting...%s:%s", __FILE__, ip, port);

        if( sb_run_http_client_connect( client ) == FAIL ) {
            error("sb_run_http_client_connect failed[%s:%s]", ip, port);
	        delay_time_sec += 1;
            goto sleep;
        }

		setproctitle("softbotd: %s sending request %s:%s", __FILE__, ip, port);

        if( sb_run_http_client_sendRequest( client ) == FAIL )  {
            error("sb_run_http_client_sendRequest failed[%s:%s]", ip, port);
	        delay_time_sec += 1;
            goto sleep;
        }

		setproctitle("softbotd: %s receiving response %s:%s", __FILE__, ip, port);

        if( sb_run_http_client_recvResponse( client ) == FAIL )  {
            error("sb_run_http_client_recvResponse failed[%s:%s]", ip, port);
	        delay_time_sec += 1;
            goto sleep;
        }

		setproctitle("softbotd: %s parsing response %s:%s", __FILE__, ip, port);

        if( sb_run_http_client_parseResponse( client ) == FAIL )  {
            error("sb_run_http_client_parseResponse failed[%s:%s]", ip, port);
	        delay_time_sec += 1;
            goto sleep;
        }

        if( client->http->response_http_code != 200 ) {
            debug("server response code[%d], may be not exist did[%d]", 
			        client->http->response_http_code,
					last_registered_docid+1);
		    if( delay_time_sec < max_delay_time_sec ) {
	            delay_time_sec += 1;
			}
            goto sleep;
        } else {
		    delay_time_sec = min_delay_time_sec;
		}

        memfile *buf = client->http->content_buf;
        if( buf == NULL ) {
            error("content buf is NULL");
	        delay_time_sec += 1;
            goto sleep;
        }

        if(client->parsing_status.state != PARSING_COMPLETE) {
            error("parse response error");
	        delay_time_sec += 1;
            goto sleep;
        }

        register_document( buf );

sleep:
        sleep(slot);

	} /* endless while loop */

	return 0;

error_return:
	if ( cdm_db ) sb_run_cdm_close( cdm_db );
	if ( did_db ) sb_run_close_did_db( did_db );
	if ( client ) sb_run_http_client_free( client );

	slot->state = SLOT_FINISH;
	return -1;
}

static int module_main(slot_t *slot) {
	sb_run_set_default_sighandlers(_shutdown, _graceful_shutdown);

	sb_run_init_scoreboard(scoreboard);
	sb_run_spawn_processes(scoreboard,"replication process", replication_main);
	scoreboard->period = MONITORING_PERIOD;
	sb_run_monitor_processes(scoreboard);

	return 0;
}

static void set_cdm_set(configValue v)
{
    cdm_set = atoi( v.argument[0] );
}

static void set_did_set(configValue v)
{
    did_set = atoi( v.argument[0] );
}

static void set_ip_and_port(configValue v)
{
    char tmp[STRING_SIZE];
    char *ptmp;
    int i;           
    strncpy(tmp, v.argument[0] , STRING_SIZE);
                     
    for(i=0; tmp[i]!=':'; i++)
        ip[i] = tmp[i];
    
    ip[i] = '\0';

    ptmp = tmp + i + 1;
    
    for(i=0;i<STRING_SIZE && ptmp[i];i++)
        port[i] = ptmp[i];

    info("master ip[%s], port[%s]", ip, port);
}


static void set_field_root_name(configValue v)
{
    if(strlen(v.argument[0]) >= SHORT_STRING_SIZE) {
        warn("max root name size[%d], current size[%d], string[%s]",
                SHORT_STRING_SIZE, strlen(v.argument[0]), v.argument[0]);
    }

    strncpy(field_root_name, v.argument[0], SHORT_STRING_SIZE-1);
}

static void set_oid_name(configValue v)
{
    if(strlen(v.argument[0]) >= SHORT_STRING_SIZE) {
        warn("max oid name size[%d], current size[%d], string[%s]",
                SHORT_STRING_SIZE, strlen(v.argument[0]), v.argument[0]);
    }

    strncpy(oid_name, v.argument[0], SHORT_STRING_SIZE-1);
}

static void set_min_delay_time(configValue v)
{
    min_delay_time_sec = atoi(v.argument[0]);
	delay_time_sec = min_delay_time_sec;
    DEBUG("min_delay_time_sec is set to %d", min_delay_time_sec);
}

static void set_max_delay_time(configValue v)
{
    max_delay_time_sec = atoi(v.argument[0]);
    DEBUG("max_delay_time_sec is set to %d", max_delay_time_sec);
}

static config_t config[] = {
    CONFIG_GET("CdmSet", set_cdm_set, 1, "Cdm Set 0~..."),
    CONFIG_GET("DidSet", set_did_set, 1, "Did Set 0~..."),
    CONFIG_GET("Master", set_ip_and_port, 1, "master ip:port"),
    CONFIG_GET("OidName", set_oid_name, 1, "oid element name"),
    CONFIG_GET("FieldRootName", set_field_root_name, 1, \
            "canned document root element name"),
    CONFIG_GET("MinDelayTime",set_min_delay_time,1, "min delay time(sec) for request"),
    CONFIG_GET("MaxDelayTime",set_max_delay_time,1, "max delay time(sec) for request"),
	{NULL}
};

module daemon_replication_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	NULL,    				/* registry */
	init,					/* initialize function of module */
	module_main,			/* child_main */
	scoreboard,				/* scoreboard */
	NULL        			/* register hook api */
};
