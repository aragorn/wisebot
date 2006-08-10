#include "common_core.h"
#include "common_util.h"

#include "util.h"  //for sb_trim()
#include "timelog.h"
#include "memory.h"
#include "setproctitle.h"
#include "mod_api/qp2.h"
#include "mod_api/sbhandler.h"
#include "mod_api/docattr2.h"
#include "mod_api/indexer.h"
#include "mod_api/http_client.h"
#include "mod_standard_handler.h"
#include "mod_httpd/http_protocol.h"
#include "mod_httpd/http_util.h"
#include "handler_util.h"

#include <stdlib.h>

#define MAX_ENUM_NUM        1024
#define MAX_ENUM_LEN        SHORT_STRING_SIZE

extern void ap_set_content_type(request_rec * r, const char *ct);

static int search_handler(request_rec *r, softbot_handler_rec *s);
static int light_search_handler(request_rec *r, softbot_handler_rec *s);
static int abstract_search_handler(request_rec *r, softbot_handler_rec *s);

static softbot_handler_key_t search_handler_tbl[] = {
	{"light_search", light_search_handler},
	{"abstract_search", abstract_search_handler},
	{"search", search_handler},
	{NULL, NULL}
};


/////////////////////////////////////////////////////////////////////
// qp request/response
static request_t qp_request;
static response_t qp_response;
static doc_hit_t* g_dochits_buffer;

// enum
static char *constants[MAX_ENUM_NUM] = { NULL };
static int constants_value[MAX_ENUM_NUM];

// node info
static uint32_t this_node_id; // 하위 4bit만 쓴다.

// connection 
static apr_int64_t timeout = 3;
static apr_int64_t keep_alive_timeout = 6;
static int keep_alive_max = 100;
static int keep_alive = 1;

// elapsed_time을 output으로 출력할건지...
static int print_elapsed_time = 1;

//--------------------------------------------------------------//
//  *   custom function 
//--------------------------------------------------------------//

static void print_virtual_document(virtual_document_t* vd)
{
	int i = 0;

    debug("vid[%u]", vd->id);
    debug("relevance[%u]", vd->relevance);
    debug("dochit_cnt[%u]", vd->dochit_cnt);
    debug("node_id[%0X]", this_node_id);
	for(;i < vd->dochit_cnt; i++) {
		debug("did[%u]", vd->dochits[i].id);
	}
}

static char* return_constants_str(int value)
{
    int i;
    for (i=0; i<MAX_ENUM_NUM; i++) {
        if (constants_value[i] == value) {
            return constants[i];
        }
    }

    return NULL;
}

static void set_con_config(server_rec* rec)
{
    rec->timeout = timeout;
    rec->keep_alive_timeout = keep_alive_timeout;
    rec->keep_alive_max = keep_alive_max;
    rec->keep_alive = keep_alive;

	info("server timeout[%lld]", timeout);
	info("server keep_alive_timeout[%lld]", keep_alive_timeout);
	info("server keep_alive_max[%d]", keep_alive_max);
	info("server keep_alive[%d]", keep_alive);
}

static int print_group(request_rec *r, groupby_result_list_t* groupby_result)
{
	int i = 0;
	int j = 0;

    for(i = 0; i < groupby_result->rules.cnt; i++) {
        orderby_rule_t* sort_rule = &(groupby_result->rules.list[i].sort);
		int is_enum = groupby_result->rules.list[i].is_enum;
        int sum = 0;

		for(j = 0; j < MAX_CARDINALITY; j++) {
			sum += groupby_result->result[i][j];
        }
	    ap_rprintf(r, "<group name=\"%s\" result_count=\"%d\">\n", 
                      sort_rule->rule.name, sum);

		// group 결과
		for(j = 0; j < MAX_CARDINALITY; j++) {
			if(groupby_result->result[i][j] != 0) {
				if(is_enum) {
					ap_rprintf(r, "<field value=\"%s\" result_count=\"%d\" />\n", 
								  return_constants_str(j), 
								  groupby_result->result[i][j]);
				} else {
					ap_rprintf(r, "<field value=\"%d\" result_count=\"%d\" />\n", 
								  j, 
								  groupby_result->result[i][j]);
				}
            }
		}

	    ap_rprintf(r, "</group>\n"); 
    }

	return SUCCESS;
}

static int search_handler(request_rec *r, softbot_handler_rec *s)
{
    int rv = 0;
    int i = 0, j = 0, cmt_idx = 0;
	struct timeval tv;
	uint32_t start_time = 0, end_time = 0;

	if ( print_elapsed_time ) {
		gettimeofday(&tv, NULL);
		start_time = tv.tv_sec*1000 + tv.tv_usec/1000;
	}

	msg_record_init(&s->msg);
	set_con_config(r->server);

	rv = sb_run_qp_init();
    if(rv != SUCCESS && rv != DECLINE) {
	    MSG_RECORD(&s->msg, error, "qp init failed");
        return FAIL;
    }

	if(apr_table_get(s->parameters_in, "q") == NULL ||
			strlen(apr_table_get(s->parameters_in, "q")) == 0) {
	    MSG_RECORD(&s->msg, error, "query is null, must use http get method");
        return FAIL;
	}

	ap_unescape_url((char *)apr_table_get(s->parameters_in, "q"));

    rv = sb_run_qp_init_request(&qp_request, 
                                (char *)apr_table_get(s->parameters_in, "q"));
    if(rv != SUCCESS) {
        error("can not init request");
		s->msg = qp_request.msg;
        return FAIL;
    }

    rv = sb_run_qp_init_response(&qp_response);
    if(rv != SUCCESS) {
        error("can not init request");
        return FAIL;
    }

	//2. get result
	timelog("full_search_start");
	rv = sb_run_qp_full_search(&qp_request, &qp_response);
	timelog("full_search_finish");
	if (rv != SUCCESS) {
		error("sb_run_qp_full_search failed: query[%s]", qp_request.query);
		s->msg = qp_request.msg;
		return FAIL;
	}

	if ( print_elapsed_time ) {
		gettimeofday(&tv, NULL);
		end_time = tv.tv_sec*1000 + tv.tv_usec/1000;
	}

	timelog("send_result_start");
	ap_set_content_type(r, "text/xml; charset=euc-kr");
	ap_rprintf(r,
			"<?xml version=\"1.0\" encoding=\"euc-kr\"?>\n");

	ap_rprintf(r, 
			"<xml>\n"
			"<status>1</status>\n" 
			"<query><![CDATA[%s]]></query>\n"
			"<parsed_query><![CDATA[%s]]></parsed_query>\n"
			"<result_count>%d</result_count>\n", 
			qp_request.query, 
			qp_response.parsed_query,
            qp_response.search_result);

	/* elapsed time */
	if ( print_elapsed_time ) {
		ap_rprintf(r, "<elapsed_time>%u</elapsed_time>\n", end_time - start_time);
	}

    /* group result */
    ap_rprintf(r, "<groups>\n");
	print_group(r, &qp_response.groupby_result_vid);
    ap_rprintf(r, "</groups>\n");

	/*
    ap_rprintf(r, "<groups type=\"did\">\n");
	print_group(r, &qp_response.groupby_result_did);
    ap_rprintf(r, "</groups>\n");
	*/

	/* each result */
    ap_rprintf(r, "<vdocs count=\"%d\">\n", qp_response.vdl->cnt);
    for(i = 0; i < qp_response.vdl->cnt; i++) {
        virtual_document_t* vd = (virtual_document_t*)&qp_response.vdl->data[i];

        ap_rprintf(r, "<vdoc vid=\"%d\" node_id=\"%0X\" relevance=\"%d\">\n", 
				      vd->id, vd->node_id, vd->relevance);
        ap_rprintf(r, "<docs count=\"%d\">\n", vd->comment_cnt);

        for(j = 0; j < vd->comment_cnt; j++) {
            comment_t* cmt = &qp_response.comments[cmt_idx++];
            ap_rprintf(r, "<doc doc_id=\"%d\">\n", cmt->did);

			if(qp_request.output_style == STYLE_XML) {
                ap_rprintf(r, "%s\n", cmt->s);
			} else {
                ap_rprintf(r, "<![CDATA[%s]]>\n", cmt->s);
			}
            ap_rprintf(r, "</doc>\n");
		}

        ap_rprintf(r, "</docs>\n");
        ap_rprintf(r, "</vdoc>\n");
    }
    ap_rprintf(r, "</vdocs>\n");
	ap_rprintf(r, "</xml>\n");

	timelog("send_result_finish");

	s->msg = qp_request.msg;

	sb_run_qp_finalize_search(&qp_request, &qp_response);
	timelog("qp_finalize");

	return SUCCESS;
}

///////////////////////////////////////////////////////////////////////
static int light_search_handler(request_rec *r, softbot_handler_rec *s)
{
    int rv = 0;
    int i = 0;

	set_con_config(r->server);
	msg_record_init(&s->msg);

	rv = sb_run_qp_init();
    if(rv != SUCCESS && rv != DECLINE) {
	    MSG_RECORD(&s->msg, error, "qp init failed");
        return FAIL;
    }

	if(apr_table_get(s->parameters_in, "q") == NULL ||
			strlen(apr_table_get(s->parameters_in, "q")) == 0) {
	    MSG_RECORD(&s->msg, error, "query is null, must use http get method");
        return FAIL;
	}

	ap_unescape_url((char *)apr_table_get(s->parameters_in, "q"));

    rv = sb_run_qp_init_request(&qp_request, 
                                (char *)apr_table_get(s->parameters_in, "q"));
    if(rv != SUCCESS) {
	    s->msg = qp_request.msg;
        error("can not init request");
        return FAIL;
    }

    rv = sb_run_qp_init_response(&qp_response);
    if(rv != SUCCESS) {
	    s->msg = qp_request.msg;
        error("can not init request");
        return FAIL;
    }

	timelog("sb_run_qp_light_search start");
	rv = sb_run_qp_light_search(&qp_request, &qp_response);
	timelog("sb_run_qp_light_search finish");
	if (rv != SUCCESS) {
	    s->msg = qp_request.msg;
		error("sb_run_qp_light_search failed: query[%s]", qp_request.query);
		return FAIL;
	}

	ap_set_content_type(r, "x-softbotd/binary");

	timelog("send_result_start");

	// 0. node id - 0 ~ 16 
	ap_rwrite(&(this_node_id), sizeof(uint32_t), r);

	// 1. 총 검색건수 
	ap_rwrite(&(qp_response.search_result), sizeof(uint32_t), r);

	// 2. 전송 검색건수
	ap_rwrite(&(qp_response.vdl->cnt), sizeof(uint32_t), r);

	// 3. 검색 단어 리스트
	ap_rwrite(qp_response.parsed_query, sizeof(qp_response.parsed_query), r);

	debug("search_result[%u], send_cnt[%u], parsed_query[%s]", 
			qp_response.search_result,
			qp_response.vdl->cnt,
			qp_response.parsed_query);

    // 4. 검색결과 전송
    for(i = 0; i < qp_response.vdl->cnt; i++) {
		virtual_document_t* vd = &(qp_response.vdl->data[i]);
   
        print_virtual_document(vd);

		ap_rwrite((void*)&vd->id, sizeof(uint32_t), r);

		ap_rwrite((void*)&vd->relevance, sizeof(uint32_t), r);

		ap_rwrite((void*)&vd->dochit_cnt, sizeof(uint32_t), r);

		ap_rwrite((void*)vd->dochits, sizeof(doc_hit_t)*vd->dochit_cnt, r);

		ap_rwrite(&(this_node_id), sizeof(uint32_t), r);
		debug("send node_id[%0X]", this_node_id);
    
	    ap_rwrite((void*)vd->docattr, sizeof(docattr_t), r);
    }

    timelog("send_result_finish");

	s->msg = qp_request.msg;
	sb_run_qp_finalize_search(&qp_request, &qp_response);
	timelog("qp_finalize");

	return SUCCESS;
}

static int abstract_search_handler(request_rec *r, softbot_handler_rec *s)
{
    int rv = 0;
    int i = 0;
	int recv_pos = 0;
    memfile *buf = NULL;
	virtual_document_t* vd = NULL;

	set_con_config(r->server);
	msg_record_init(&s->msg);

	rv = sb_run_qp_init();
    if(rv != SUCCESS && rv != DECLINE) {
	    MSG_RECORD(&s->msg, error, "qp init failed");
        return FAIL;
    }

	if(apr_table_get(s->parameters_in, "q") == NULL ||
			strlen(apr_table_get(s->parameters_in, "q")) == 0) {
	    MSG_RECORD(&s->msg, error, "query is null, must use http get method");
        return FAIL;
	}

	ap_unescape_url((char *)apr_table_get(s->parameters_in, "q"));

    rv = sb_run_qp_init_request(&qp_request, 
                                (char *)apr_table_get(s->parameters_in, "q"));
    if(rv != SUCCESS) {
	    s->msg = qp_request.msg;
        error("can not init request");
        return FAIL;
    }

    rv = sb_run_qp_init_response(&qp_response);
    if(rv != SUCCESS) {
	    s->msg = qp_request.msg;
        error("can not init request");
        return FAIL;
    }

	/* 수신할 doc_hit 를 assign */
	qp_response.vdl->cnt = 1;
	vd = &(qp_response.vdl->data[0]);

	if(g_dochits_buffer == NULL) {
		g_dochits_buffer = (doc_hit_t*)sb_calloc(MAX_DOC_HITS_SIZE, sizeof(doc_hit_t));
	}

    vd->dochits = g_dochits_buffer; 

	/* comment를 추출할 virtual document 정보 받기 */
	CHECK_REQUEST_CONTENT_TYPE(r, "x-softbotd/binary");

	if (sb_run_sbhandler_make_memfile(r, &buf) != SUCCESS) {
	    MSG_RECORD(&s->msg, error, "make_memfile_from_postdata failed");
        if(buf != NULL) memfile_free(buf);
		return FAIL;
	}

    vd->dochit_cnt = memfile_getSize(buf) / (sizeof(doc_hit_t) + sizeof(uint32_t));
    for (i = 0; i < vd->dochit_cnt; i++) {
		uint32_t node_id = 0;
		uint32_t recv_data_size = 0;

        recv_data_size = sizeof(doc_hit_t);
        if ( memfile_read(buf, (char*)&(vd->dochits[recv_pos]), recv_data_size)
                != recv_data_size ) {
			MSG_RECORD(&s->msg, error, "incomplete result at [%d]th node: doc_hits ", i);
            if(buf != NULL) memfile_free(buf);
            return FAIL;
        }

		// 2. node_id 수신
		recv_data_size = sizeof(uint32_t);
		if ( memfile_read(buf, (char*)&node_id, recv_data_size)
				!= recv_data_size ) {
			MSG_RECORD(&s->msg, error, "incomplete result at [%d]th node: node_id ", i);
            if(buf != NULL) memfile_free(buf);
			return FAIL;
		}

		// 자신의 node_id가 아니면 error
		if(get_node_id(node_id) != this_node_id) {
			MSG_RECORD(&s->msg, 
			              error, "node_id[%u] not equals this_node_id[%0X]",
						  get_node_id(node_id),
						  this_node_id);
            if(buf != NULL) memfile_free(buf);
			return FAIL;
		}

		debug("did[%u], node_id[%0X]", vd->dochits[recv_pos].id, node_id);

        // 자신의 node_id pop
        node_id = pop_node_id(node_id);
		recv_pos++;
    }
	vd->dochit_cnt = recv_pos;


	/* comment 추출 */
	timelog("sb_run_qp_abstract_search start");
	rv = sb_run_qp_abstract_search(&qp_request, &qp_response);
	timelog("sb_run_qp_abstract_search finish");
	if (rv != SUCCESS) {
	    s->msg = qp_request.msg;
		error("sb_run_qp_abstract_search failed: query[%s]", qp_request.query);
        if(buf != NULL) memfile_free(buf);
		return FAIL;
	}

    /////////// abstarct search 결과 전송 ////////////////////////////////////////
    ap_set_content_type(r, "x-softbotd/binary");

    // 1. 전송건수.
    ap_rwrite(&vd->comment_cnt, sizeof(uint32_t), r);
	
	// 2. doc_hits 전송.
    for (i = 0; i < vd->comment_cnt; i++ ) {
        // 2.1 doc_id 전송.
        ap_rwrite(&vd->dochits[i].id, sizeof(uint32_t), r);
        // 2.2 node_id 전송.
        ap_rwrite(&this_node_id, sizeof(uint32_t), r);
        // 2.3. comment
        ap_rwrite(qp_response.comments[i].s, LONG_LONG_STRING_SIZE, r);
		debug("comments[%s]", qp_response.comments[i].s);
    }

	s->msg = qp_request.msg;

    if(buf != NULL) memfile_free(buf);
	return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////
static void set_node_id(configValue v)
{
    this_node_id = atoi(v.argument[0]);
    if(this_node_id > 16) {
        error("node_id should be smaller than 16");
    }

	char subprefix[SHORT_STRING_SIZE];
	snprintf(subprefix, sizeof(subprefix), "node%d", this_node_id);
	setproctitle_subprefix(subprefix);
}

static void set_enum(configValue v)
{
    int i;

    static char enums[MAX_ENUM_NUM][MAX_ENUM_LEN];
    for (i=0; i<MAX_ENUM_NUM && constants[i]; i++);
    if (i == MAX_ENUM_NUM) {
        error("too many constant is defined");
        return;
    }
    strncpy(enums[i], v.argument[0], MAX_ENUM_LEN);
    enums[i][MAX_ENUM_LEN-1] = '\0';
    constants[i] = enums[i];

    constants_value[i] = atoi(v.argument[1]);

    debug("Enum[%s]: %d", constants[i], constants_value[i]);
}

static void set_timeout(configValue v)
{
    timeout = atoll(v.argument[0]);
}

static void set_keep_alive_timeout(configValue v)
{
    keep_alive_timeout = atoll(v.argument[0]);
}

static void set_keep_alive_max(configValue v)
{
    keep_alive_max = atoi(v.argument[0]);
}

static void set_keep_alive(configValue v)
{
    keep_alive = atoi(v.argument[0]);
}

static int get_table(char *name_space, void **tab)
{
	if ( strcmp(name_space, "search") != 0 ){
		return DECLINE;
	}

	*tab = search_handler_tbl;

	return SUCCESS;
}

static void set_elapsed_time(configValue v)
{
	print_elapsed_time = strcasecmp(v.argument[0], "true") == 0;
}

static config_t config[] = {
    CONFIG_GET("NodeId", set_node_id, 1, "set node id : NodeId [no]"),
    CONFIG_GET("Enum", set_enum, 2, "constant"),
	CONFIG_GET("TimeOut", set_timeout, 1, "constant"),
	CONFIG_GET("KeepAliveTimeOut", set_keep_alive_timeout, 1, "constant"),
	CONFIG_GET("KeepAliveMax", set_keep_alive_max, 1, "constant"),
	CONFIG_GET("KeepAlive", set_keep_alive, 1, "constant"),
	CONFIG_GET("PrintElapsedTime", set_elapsed_time, 1, "default is True"),
    {NULL}
};

static void register_hooks(void)
{
	sb_hook_sbhandler_get_table(get_table,NULL,NULL,HOOK_MIDDLE);
	return;
}

module search_sbhandler_module = {
	STANDARD_MODULE_STUFF,
	config,                   /* config            */
	NULL,                     /* registry          */
	NULL,                     /* initialize        */
	NULL,                     /* child_main        */
	NULL,                     /* scoreboard        */
	register_hooks            /* register hook api */
};
