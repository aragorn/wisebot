#include <stdlib.h>
#include "common_core.h"
#include "common_util.h"
#include "util.h"  //for sb_trim()
#include "timelog.h"
#include "memory.h"
#include "mod_api/qp2.h"
#include "mod_api/sbhandler.h"
#include "mod_api/docattr2.h"
#include "mod_api/indexer.h"
#include "mod_api/http_client.h"
#include "mod_standard_handler.h"
#include "mod_httpd/http_protocol.h"
#include "mod_httpd/http_util.h"
#include "handler_util.h"

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

//--------------------------------------------------------------//
//  *   custom function 
//--------------------------------------------------------------//

static void print_virtual_document(virtual_document_t* vd)
{
	int i = 0;

    debug("vid[%u]", vd->id);
    debug("relevancy[%u]", vd->relevancy);
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

static int search_handler(request_rec *r, softbot_handler_rec *s)
{
    int rv = 0;
    int i = 0, j = 0, cmt_idx = 0;
	groupby_result_list_t* groupby_result = NULL;

	rv = sb_run_qp_init();
    if(rv != SUCCESS && rv != DECLINE) {
        error("qp init failed");
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

	timelog("send_result_start");
	ap_set_content_type(r, "text/xml; charset=euc-kr");
	ap_rprintf(r,
			"<?xml version=\"1.0\" encoding=\"euc-kr\"?>\n");

	/* word_list char * req->word_list */
	ap_rprintf(r, 
			"<xml>"
			"<search>\n"
			"<summary>\n"
			"<query><![CDATA[%s]]></query>\n"
			"<words><![CDATA[%s]]></words>\n"
			"<total_count>%d</total_count>\n" 
			"<result_count>%d</result_count>\n" 
			"</summary>\n",
			qp_request.query, 
            qp_response.word_list, 
            qp_response.search_result, 
            qp_response.vdl->cnt);
    /* group result */
    ap_rprintf(r, "<groups>\n");

	groupby_result = &qp_response.groupby_result_vid;
    for(i = 0; i < groupby_result->rules.cnt; i++) {
        orderby_rule_t* sort_rule = &(groupby_result->rules.list[i].sort);
        //limit_t* limit_rule = &(groupby_result->rules.list[i].limit);

		// group 결과
		for(j = 0; j < MAX_CARDINALITY; j++) {
			if(groupby_result->result[i][j] != 0) {
				ap_rprintf(r, "<group name=\"%s\" field=\"%s\" count=\"%d\" />\n", 
						      return_constants_str(j), 
							  sort_rule->rule.name,
							  groupby_result->result[i][j]);
            }
		}
    }
    ap_rprintf(r, "</groups>\n");
	/* each result */
    for(i = 0; i < qp_response.vdl->cnt; i++) {
        virtual_document_t* vd = (virtual_document_t*)&qp_response.vdl->data[i];

        ap_rprintf(r, "<row no=\"%d\">\n", i);
        ap_rprintf(r, "<id>%u</id>\n", vd->id);
        ap_rprintf(r, "<node_id>%0X</node_id>\n", vd->node_id);
        ap_rprintf(r, "<relevancy>%u</relevancy>\n", vd->relevancy);
        ap_rprintf(r, "<comment_count>%u</comment_count>\n", vd->dochit_cnt);

        for(j = 0; j < vd->dochit_cnt; j++) {
            ap_rprintf(r, "<comment><![CDATA[%s]]></comment>\n", qp_response.comments[cmt_idx++].s);
		}

        ap_rprintf(r, "</row>\n");
    }

	ap_rprintf(r, "</search>\n</xml>\n");

	//sb_run_sbhandler_append_msg_record(r, &(s->msgs));
	timelog("send_result_finish");

	sb_run_qp_finalize_search(&qp_request, &qp_response);
	timelog("qp_finalize");

	s->msg = qp_request.msg;
	return SUCCESS;
}

///////////////////////////////////////////////////////////////////////
static int light_search_handler(request_rec *r, softbot_handler_rec *s)
{
    int rv = 0;
    int i = 0;

	rv = sb_run_qp_init();
    if(rv != SUCCESS && rv != DECLINE) {
	    s->msg = qp_request.msg;
        error("qp init failed");
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
	ap_rwrite(qp_response.word_list, sizeof(char)*STRING_SIZE, r);

	debug("search_result[%u], send_cnt[%u], word_list[%s]", 
			qp_response.search_result,
			qp_response.vdl->cnt,
			qp_response.word_list);

    // 4. 검색결과 전송
    for(i = 0; i < qp_response.vdl->cnt; i++) {
		virtual_document_t* vd = &(qp_response.vdl->data[i]);
   
        print_virtual_document(vd);

		ap_rwrite((void*)&vd->id, sizeof(uint32_t), r);

		ap_rwrite((void*)&vd->relevancy, sizeof(uint32_t), r);

		ap_rwrite((void*)&vd->dochit_cnt, sizeof(uint32_t), r);

		ap_rwrite((void*)vd->dochits, sizeof(doc_hit_t)*vd->dochit_cnt, r);

		ap_rwrite(&(this_node_id), sizeof(uint32_t), r);
		debug("send node_id[%0X]", this_node_id);
    
	    ap_rwrite((void*)vd->docattr, sizeof(docattr_t), r);
    }

    timelog("send_result_finish");

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

	rv = sb_run_qp_init();
    if(rv != SUCCESS && rv != DECLINE) {
	    s->msg = qp_request.msg;
        error("qp init failed");
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
            return FAIL;
        }

		// 2. node_id 수신
		recv_data_size = sizeof(uint32_t);
		if ( memfile_read(buf, (char*)&node_id, recv_data_size)
				!= recv_data_size ) {
			MSG_RECORD(&s->msg, error, "incomplete result at [%d]th node: node_id ", i);
			return FAIL;
		}

		// 자신의 node_id가 아니면 error
		if(get_node_id(node_id) != this_node_id) {
			MSG_RECORD(&s->msg, 
			              error, "node_id[%u] not equals this_node_id[%0X]",
						  get_node_id(node_id),
						  this_node_id);
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
		return FAIL;
	}

    /////////// abstarct search 결과 전송 ////////////////////////////////////////
    ap_set_content_type(r, "x-softbotd/binary");

    // 1. 전송건수.
    ap_rwrite(&vd->dochit_cnt, sizeof(uint32_t), r);
	
	// 2. doc_hits 전송.
    for (i = 0; i < vd->dochit_cnt; i++ ) {
        // 2.1 doc_id 전송.
        ap_rwrite(&vd->dochits[i].id, sizeof(uint32_t), r);
        // 2.2 node_id 전송.
        ap_rwrite(&this_node_id, sizeof(uint32_t), r);
        // 2.3. comment
        ap_rwrite(qp_response.comments[i].s, LONG_LONG_STRING_SIZE, r);
		debug("comments[%s]", qp_response.comments[i].s);
    }

	return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////
static void set_node_id(configValue v)
{
    this_node_id = atoi(v.argument[0]);
    if(this_node_id > 16) {
        error("node_id should be smaller than 16");
    }
}

static void get_enum(configValue v)
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

static int get_table(char *name_space, void **tab)
{
	if ( strcmp(name_space, "search") != 0 ){
		return DECLINE;
	}

	*tab = search_handler_tbl;

	return SUCCESS;
}

static config_t config[] = {
    CONFIG_GET("NodeId", set_node_id, 1, "set node id : NodeId [no]"),
    CONFIG_GET("Enum", get_enum, 2, "constant"),
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
