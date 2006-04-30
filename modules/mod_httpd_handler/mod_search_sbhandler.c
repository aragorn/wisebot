#include <stdlib.h>
#include "common_core.h"
#include "common_util.h"
#include "util.h"  //for sb_trim()
#include "util.h"
#include "timelog.h"
#include "memory.h"
#include "mod_api/qp2.h"
#include "mod_api/sbhandler.h"
#include "mod_api/docattr2.h"
#include "mod_api/indexer.h"
#include "mod_api/http_client.h"
#include "mod_standard_handler.h"
#include "mod_httpd/http_protocol.h"
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

// enum
static char *constants[MAX_ENUM_NUM] = { NULL };
static int constants_value[MAX_ENUM_NUM];

// node info
static uint32_t this_node_id; // 하위 4bit만 쓴다.

//--------------------------------------------------------------//
//  *   custom function 
//--------------------------------------------------------------//

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

    rv = sb_run_qp_init_request(&qp_request, 
                                (char *)apr_table_get(s->parameters_in, "q"));
    if(rv != SUCCESS) {
        error("can not init request");
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
        ap_rprintf(r, "<relevancy>%u</relevancy>\n", vd->relevancy);

        for(j = 0; j < vd->dochit_cnt; j++) {
            ap_rprintf(r, "<comment><![CDATA[%s]]></comment>\n", qp_response.comments[cmt_idx++]);
		}

        ap_rprintf(r, "</row>\n");
    }

	ap_rprintf(r, "</search>\n</xml>\n");

	//sb_run_sbhandler_append_msg_record(r, &(s->msgs));
	timelog("send_result_finish");

	sb_run_qp_finalize_search(&qp_request, &qp_response);
	timelog("qp_finalize");

	return SUCCESS;
}

///////////////////////////////////////////////////////////////////////
static int light_search_handler(request_rec *r, softbot_handler_rec *s)
{
#if 0
    request_t req; 
    int rv = 0, i = 0;
    int search_total_cnt = 0; // 총 검색결과수
    int search_send_cnt = 0;  // 전송하는 검색결과 수
	char query[MAX_QUERY_STRING_SIZE];

    if(make_request(s, &req, query) != SUCCESS) {
        error("can not make request string");
        return FAIL;
    }

    req.type = LIGHT_SEARCH;

    req.sb4error = 0;
    req.result_list = NULL; 

	timelog("light_search_start");
	rv = sb_run_qp_light_search(get_word_db(), &req);
	timelog("light_search_finish");
	if (rv != SUCCESS) {
		error("sb_run_qp_light_search failed: query[%s]", query);
		return FAIL;
	}

	if (req.result_list == NULL) {
        search_total_cnt = 0;
        search_send_cnt = 0;

		error("no result: query[%s]", query);
	} else {
        search_total_cnt = req.result_list->ndochits;
        search_send_cnt = get_send_count(&req);
	}

	ap_set_content_type(r, "x-softbotd/binary");

	timelog("send_result_start");
	// 0. node id - 0 ~ 16 
	ap_rwrite(&(this_node_id), sizeof(uint32_t), r);

	// 1. 총 검색건수 
	ap_rwrite(&(search_total_cnt), sizeof(uint32_t), r);

	// 2. 전송 검색건수
	ap_rwrite(&(search_send_cnt), sizeof(uint32_t), r);
	debug("send cnt[%d]", search_send_cnt);

	// 3. 검색 단어 리스트
	ap_rwrite(req.word_list, sizeof(char)*STRING_SIZE, r);

	debug("search_total_cnt[%u], search_send_cnt[%u], word_list[%s]", 
			search_total_cnt,
			search_send_cnt,
			req.word_list);

    // 4. 검색결과 전송
    index_list_t *result = req.result_list;
    for(i = 0; i < search_send_cnt; i++) {
        uint32_t docid = result->doc_hits[i].id;
        docattr_t* attr;

		// 4-1. 관련성 전송
		ap_rwrite((void*)&result->doc_hits[i].hitratio, sizeof(uint32_t), r);

		// 4-2. doc_hits 전송
		ap_rwrite((void*)&result->doc_hits[i], sizeof(doc_hit_t), r);

		// 4-3. node id - 32bit
		ap_rwrite(&(this_node_id), sizeof(uint32_t), r);
		debug("send node_id[%u]", this_node_id);
    
		// 4-4. docattr 전송
        if ( sb_run_docattr_ptr_get(docid, &attr) != SUCCESS ) {
            docattr_t dummy;
            memset(&dummy, 0x00, sizeof(docattr_t));
            error("cannot get docattr element");

            // dummy 전송. 최소한 상위 agent에서 오류나지 않도록한다.
	        ap_rwrite((void*)&dummy, sizeof(docattr_t), r);
            continue;
        }

	    ap_rwrite((void*)attr, sizeof(docattr_t), r);
    }

    timelog("send_result_finish");

	sb_run_qp_finalize_search(&req);
	timelog("qp_finalize");
#endif
	return SUCCESS;
}

static int abstract_search_handler(request_rec *r, softbot_handler_rec *s)
{
#if 0
    int i = 0;
    memfile *buf;
    int recv_data_size = 0;
    int recv_cancel_cnt = 0;
	agent_request_t* req = NULL;

    init_agent_request(&req);

	CHECK_REQUEST_CONTENT_TYPE(r, "x-softbotd/binary");

	if (sb_run_sbhandler_make_memfile(r, &buf) != SUCCESS) {
		error("make_memfile_from_postdata failed");
		return FAIL;
	}

    // 1. 수신건수(agent에서 recv_cnt를 계산할수 없기 때문에 size로 해야 한다.)
    req->ali.recv_cnt = memfile_getSize(buf) / (sizeof(doc_hit_t) + sizeof(uint32_t));
	debug("recv cnt[%d]", req->ali.recv_cnt);

    // 2. agent_doc_hits 수신
    for (i=0; i<req->ali.recv_cnt; i++) {
        // 2.1 doc_hit 수신
        recv_data_size = sizeof(doc_hit_t);
        if ( memfile_read(buf, (char*)&req->ali.agent_doc_hits[i]->doc_hits, recv_data_size)
                != recv_data_size ) {
            error("incomplete result at [%d]th node: doc_hits ", i);
            continue;
        }

		// 2. node_id 수신
		recv_data_size = sizeof(uint32_t);
		if ( memfile_read(buf, (char*)&req->ali.agent_doc_hits[i]->node_id, recv_data_size)
				!= recv_data_size ) {
			error("incomplete result at [%d]th node: doc_hits ", i);
			continue;
		}

		// 자신의 node_id가 아니면 error
		if(get_node_id(req->ali.agent_doc_hits[i]->node_id) != this_node_id) {
			error("node_id[%u] not equals this_node_id[%u]",
						  get_node_id(req->ali.agent_doc_hits[i]->node_id),
						  this_node_id);
			recv_cancel_cnt++; // 한건을 취소시킨다.
			continue;
		}

        // 자신의 node_id pop
        req->ali.agent_doc_hits[i]->node_id = 
			pop_node_id(req->ali.agent_doc_hits[i]->node_id);

        // 수신해야할 comment 초기화
        req->ali.comments[i][0] = '\0';
    }

    req->ali.recv_cnt -= recv_cancel_cnt;

	if (sb_run_qp_abstract_info(req) != SUCCESS) {
		error("sb_run_qp_get_abstract_list failed");
		return FAIL;
	}

    /////////// abstarct search 결과 전송 ////////////////////////////////////////
    // binary로 전송한다.
    ap_set_content_type(r, "x-softbotd/binary");

    // 1. 전송건수.
    ap_rwrite(&req->ali.recv_cnt, sizeof(uint32_t), r);
	
	// 2. agent_doc_hits 전송.
    for (i=0; i<req->ali.recv_cnt; i++ ) {
        // 2.1 doc_id 전송.
        ap_rwrite(&req->ali.agent_doc_hits[i]->doc_hits.id, sizeof(uint32_t), r);
        // 2.2 node_id 전송.
        ap_rwrite(&this_node_id, sizeof(uint32_t), r);
        // 2.3. comment
        ap_rwrite(req->ali.comments[i], LONG_LONG_STRING_SIZE, r);
		debug("comments[%s]", req->ali.comments[i]);
    }

#endif
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
