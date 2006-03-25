#include <stdlib.h>
#include "common_core.h"
#include "common_util.h"
#include "util.h"
#include "timelog.h"
#include "memory.h"
#include "mod_api/sbhandler.h"
#include "mod_api/docattr.h"
#include "mod_qp/mod_qp.h"
#include "mod_api/indexer.h"
#include "mod_standard_handler.h"
#include "mod_httpd/http_protocol.h"
#include "mod_api/http_client.h"

extern void ap_set_content_type(request_rec * r, const char *ct);

static int search_handler(request_rec *r, softbot_handler_rec *s);
static int light_search_handler(request_rec *r, softbot_handler_rec *s);
static int abstract_search_handler(request_rec *r, softbot_handler_rec *s);
static agent_request_t* g_agent_request;
static agent_doc_hits_t* g_agent_doc_hits;

static softbot_handler_key_t search_handler_tbl[] = {
	{"light_search", light_search_handler},
	{"abstract_search", abstract_search_handler},
	{"search", search_handler},
	{NULL, NULL}
};


/////////////////////////////////////////////////////////////////////
// node 정보
static uint32_t this_node_id; // 하위 4bit만 쓴다.

//--------------------------------------------------------------//
//  *   custom function 
//--------------------------------------------------------------//
static char *replace_newline_to_space(char *str) {
    char *ch;
    ch = str;       
    while ( (ch = strchr(ch, '\n')) != NULL ) {
        *ch = ' ';
    }
    return str;
}   

static int def_atoi(const char *s, int def)
{
	if (s)
		return atoi(s);
	return def;
}

/*
 * 하위 4bit에 node_id를 push
 * push 된 node_id를 리턴
 */
static uint32_t push_node_id(uint32_t node_id)
{
    // 상위 4bit에 내용이 있으면 더이상 depth를 늘릴수 없음.
    if((node_id >> 28) > 0) {
        error("depth overflower[%s]", sb_strbin(node_id, sizeof(uint32_t)));
        return 0;
    }

    node_id = node_id << 4;
    node_id |= this_node_id;

    return node_id;
}

/*
 * 하위 4bit를 pop한다.
 * pop 된 node_id를 리턴.
 */
static uint32_t pop_node_id(uint32_t node_id)
{
    return (node_id >> 4);
}

// 하위 4bit의 node_id 알아내기
static uint32_t get_node_id(uint32_t node_id)
{
    return node_id & 0x0f;
}

static void init_agent_request(agent_request_t** req)
{
	int i = 0;

    if(g_agent_request == NULL) {
		g_agent_request = (agent_request_t*)sb_malloc(sizeof(agent_request_t));
	}

	// MAX_AGENT_DOC_HITS_COUNT번 memory 할당 하지 않고 한번에 할당받고 연결한다.
	if(g_agent_doc_hits == NULL) {
		g_agent_doc_hits = (agent_doc_hits_t*)
			            sb_malloc(sizeof(agent_doc_hits_t)*MAX_AGENT_DOC_HITS_COUNT);

		// pointer 연결
		for(i = 0; i < MAX_AGENT_DOC_HITS_COUNT; i++) {
            g_agent_request->ali.agent_doc_hits[i] = &g_agent_doc_hits[i];
		}
	}

	//  초기화
	g_agent_request->lc = 0;
	g_agent_request->pg = 0;
	g_agent_request->ft = 0;
	g_agent_request->send_cnt = 0;
	g_agent_request->send_first = 0;
	g_agent_request->ali.total_cnt = 0;
	g_agent_request->ali.recv_cnt = 0;

	// g_agent_request->ali.agent_doc_hits를 절대 건들지 말것.

	*req = g_agent_request;

    return;
}

/////////////////////////////////////////////////////////////////////////
// 몇건 전송해야 하는지 계산한다.
static int get_send_count(request_t* req)
{
    int total_cnt = req->result_list->ndochits;
    int send_cnt = 0;
    int send_page = req->first_result/req->list_size;
    int lc = req->list_size;

    if (total_cnt < send_page*lc) {
        send_cnt = 0;
    }
    else {
        int remain = total_cnt - send_page*lc;

        if (remain > lc) {
            send_cnt = lc; // page_no 가 뒤로 한참 남았다.
        }
        else {
            send_cnt = remain; // 마지막 페이지다.
        }

        if ( send_cnt > COMMENT_LIST_SIZE ) {
            warn("searched_list_size[%d] > COMMENT_LIST_SIZE[%d]",
                    send_cnt, COMMENT_LIST_SIZE);
            send_cnt = COMMENT_LIST_SIZE;
        }
    }

    return send_cnt;
}

static int make_request(softbot_handler_rec* s, request_t* req, char* query_string)
{
    char* p = NULL;

	//1. set request_t
    /* query */
	p = (char *)apr_table_get(s->parameters_in, "q");
	if (p == NULL) {
		error("no query string");
		return FAIL;
	}
	strncpy(req->query_string, p, MAX_QUERY_STRING_SIZE-1);

    /* AT */
	p = (char *)apr_table_get(s->parameters_in, "at");
	if (p != NULL) {
	    strncpy(req->attr_string, p, MAX_ATTR_STRING_SIZE-1);
	}

    /* AT2 */
	p = (char *)apr_table_get(s->parameters_in, "at2");
	if (p != NULL) {
	    strncpy(req->attr2_string, p, MAX_ATTR_STRING_SIZE-1);
	}

    /* GR */
	p = (char *)apr_table_get(s->parameters_in, "gr");
	if (p != NULL) {
	    strncpy(req->group_string, p, MAX_GROUP_STRING_SIZE-1);
	}

    /* SH */
	p = (char *)apr_table_get(s->parameters_in, "sh");
	if (p != NULL) {
	    strncpy(req->sort_string, p, MAX_SORT_STRING_SIZE-1);
	}

    /* LC */
	req->list_size = def_atoi(apr_table_get(s->parameters_in, "lc"), 20);
	if(req->list_size < 0) req->list_size = 20;

    /* PG */
    req->first_result = def_atoi(apr_table_get(s->parameters_in, "pg"), 0) * req->list_size;
	if(req->first_result < 0) req->first_result = 0;

	//req->first_result *= req.list_size;

    /* FT */
    req->filtering_id = def_atoi(apr_table_get(s->parameters_in, "ft"), 0);

    /* reqeust debug string */
    debug("req->query_string:[%s]",req->query_string);
    debug("req->attr_string:[%s]",req->attr_string);
    debug("req->attr2_string:[%s]",req->attr2_string);
    debug("req->group_string:[%s]",req->group_string);
    debug("req->sort_string:[%s]",req->sort_string);
    debug("req->list_size:%d",req->list_size);
    debug("req->first_result:%d",req->first_result);
    debug("req->filtering_id:%d", req->filtering_id);

    if( snprintf(query_string, MAX_QUERY_STRING_SIZE, "q=%s&at=%s&at2=%s&gr=%s&sh=%s&lc=%d&pg=%d&ft=%d",
                   req->query_string, req->attr_string,
                   req->attr2_string, req->group_string,
                   req->sort_string, req->list_size, 
                   req->first_result, req->filtering_id) <= 0) {
		error("query to long");
		return FAIL;
    }

    return SUCCESS;
}

static int search_handler(request_rec *r, softbot_handler_rec *s)
{
    request_t req; 
    int rv = 0, i = 0;
    int search_total_cnt = 0; // 총 검색결과수
    int search_send_cnt = 0;  // 전송하는 검색결과 수
	char query[MAX_QUERY_STRING_SIZE];

    if(make_request(s, &req, query) != SUCCESS) {
        error("can not make request string");
        return FAIL;
    }

    req.type = FULL_SEARCH;

	//2. get result
	timelog("full_search_start");
	rv = sb_run_qp_full_search(get_word_db(), &req);
	timelog("full_search_finish");
	if (rv != SUCCESS) {
		error("sb_run_qp_full_search failed: query[%s]", query);
		return FAIL;
	}

	//we've got no search result
	if (req.result_list == NULL) {
        search_total_cnt = 0;
        search_send_cnt = 0;

		INFO("no result: query[%s]", query);
	} else {
        search_total_cnt = req.result_list->ndochits;
        search_send_cnt = get_send_count(&req);
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
			"<total_page>%d</total_page>\n"
			"<page_no>%d</page_no>\n"
			"<page_size>%d</page_size>\n"
			"<result_count>%d</result_count>\n"
			"</summary>\n",
			query, req.word_list, search_total_cnt, 
			search_total_cnt/req.list_size + ((search_total_cnt > 0 && search_total_cnt%req.list_size == 0) ? -1 : 0),
			req.first_result/req.list_size,
			req.list_size, search_send_cnt);

	/* each result */
	for (i = req.first_result; i < search_send_cnt; i++) {
		ap_rprintf(r, 
				"<row>"
				"<rowno>%d</rowno>"
				"<did>%d</did>"
				"<relevance>%d</relevance>"
				"<wordhist>%d</wordhist>"
				"<comment><![CDATA[%s]]></comment>"
				"</row>\n",
				i,
                req.result_list->doc_hits[i].id,
                req.result_list->doc_hits[i].hitratio,
                req.result_list->doc_hits[i].nhits,
                replace_newline_to_space(req.comments[i]));
	}

	ap_rprintf(r, "</search>\n");

	//sb_run_sbhandler_append_msg_record(r, &(s->msgs));
	ap_rprintf(r, "</xml>");
	timelog("send_result_finish");

	sb_run_qp_finalize_search(&req);
	timelog("qp_finalize");

	return SUCCESS;
}

///////////////////////////////////////////////////////////////////////
static int light_search_handler(request_rec *r, softbot_handler_rec *s)
{
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
	return SUCCESS;
}

static int abstract_search_handler(request_rec *r, softbot_handler_rec *s)
{
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
    for (i=0; i<req->ali.recv_cnt; i++ ) {
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
        req->ali.agent_doc_hits[i]->comments[0] = '\0';
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
        // 2.2. comment
        ap_rwrite(req->ali.agent_doc_hits[i]->comments, LONG_LONG_STRING_SIZE, r);
		debug("comments[%s]", req->ali.agent_doc_hits[i]->comments);
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
