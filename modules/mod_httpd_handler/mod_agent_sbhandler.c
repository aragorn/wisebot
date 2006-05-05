#include <stdlib.h> 
#include "common_core.h"
#include "common_util.h"
#include "timelog.h"
#include "memory.h"
#include "util.h"
#include "mod_api/sbhandler.h"
#include "mod_httpd_handler/mod_standard_handler.h"
#include "mod_httpd/protocol.h"
#include "mod_httpd/http_util.h"
#include "mod_api/httpd.h"
#include "mod_api/indexer.h"
#include "mod_api/qp2.h"
#include "mod_api/http_client.h"
#include "handler_util.h"

#define MAX_ENUM_NUM        (1024)
#define MAX_ENUM_LEN        SHORT_STRING_SIZE
#define DEFAULT_PORT        "8000"

static int search_handler(request_rec *r, softbot_handler_rec *s);
static int light_search_handler(request_rec *r, softbot_handler_rec *s);
static int abstract_search_handler(request_rec *r, softbot_handler_rec *s);

static softbot_handler_key_t agent_handler_tbl[] =
{	
	{NULL, NULL}
};

static softbot_handler_key_t agent_search_handler_tbl[] =
{	
	{"search", search_handler },
	{"light_search", light_search_handler }, 
	{"abstract_search", abstract_search_handler }, 
	{NULL, NULL}
};

/////////////////////////////////////////////////////////////////////
// node 정보
typedef struct {
    char ip[STRING_SIZE];
    char port[SHORT_STRING_SIZE];
    http_client_t* client;
	uint32_t node_id;
} node_t;
static uint32_t this_node_id; // 하위 4bit만 쓴다.

/*
 * search_nodes, clients는 1:1로 순서대로 맵핑된다.
 */
static int search_node_num = 0;
node_t search_nodes[MAX_SEARCH_NODE];


/////////////////////////////////////////////////////////////////////
// qp request/response
static request_t qp_request;
static response_t qp_response;

static doc_hit_t* g_dochits_buffer;
static int g_dochit_cnt;
static docattr_t* g_docattr_buffer;
static int g_docattr_cnt;


// enum
static char *constants[MAX_ENUM_NUM] = { NULL };
static int constants_value[MAX_ENUM_NUM];

//--------------------------------------------------------------//
//	*   custom function	
//--------------------------------------------------------------//
static void print_virtual_document(virtual_document_t* vd)
{
    int i = 0;

    debug("vid[%u]", vd->id);
    debug("relevancy[%u]", vd->relevancy);
    debug("dochit_cnt[%u]", vd->dochit_cnt);
    debug("node_id[%u]", this_node_id);
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

static int get_client_by_node_id(uint32_t node_id)
{ 
    int i = 0; 
    for(i = 0; i < search_node_num; i++) {
        if(search_nodes[i].node_id == node_id) {
            return i;
        }
    } 
 
    return -1; 
}

static void free_mfile_list(memfile **mfile_list){
	int i;
	for (i=0; i<search_node_num; i++ ) {
		if ( mfile_list[i] ) memfile_free(mfile_list[i]);
	}
}

static void init_agent()
{
    g_dochit_cnt = 0;
    if(g_dochits_buffer == NULL) {
        g_dochits_buffer = (doc_hit_t*)sb_calloc(MAX_AGENT_DOC_HITS_COUNT, sizeof(doc_hit_t));
    }

    g_docattr_cnt = 0;
    if(g_docattr_buffer == NULL) {
        g_docattr_buffer = (docattr_t*)sb_calloc(MAX_AGENT_DOCATTR_COUNT, sizeof(docattr_t));
    }
}

static char* find_comment(comment_t* com_list, uint32_t did, uint32_t node_id)
{
    int i = 0;

    debug("did[%u], node_id[%u]", did, node_id);

	for(; i < COMMENT_LIST_SIZE; i++) {
        if(com_list[i].node_id == 0)
            break;

debug("[%d] : cmt_did[%u], cmt_node_id[%u]", i, com_list[i].did, com_list[i].node_id);
		if(did == com_list[i].did &&
		   node_id == com_list[i].node_id) {
			return com_list[i].s;
		}
	}

    return NULL;
}

//--------------------------------------------------------------//
//	*	agent_search implemetation
//--------------------------------------------------------------//
static int agent_lightsearch(request_rec *r, request_t* req, response_t* res) 
{
	int i = 0;
    int rv = 0;
	int node_idx = 0;
	char path[MAX_QUERY_STRING_SIZE];
	char query[MAX_QUERY_STRING_SIZE];
    http_client_t* clients[MAX_SEARCH_NODE];
    limit_t* limit = NULL;
    limit_t limit_buffer;

    if(req->op_list_vid.cnt <= 0) {
        error("null operation");
        return FAIL;
    }

    // 마지막에 반드시 LIMIT OPERATION이 존재해야 한다.
	operation_t* op = &(req->op_list_vid.list[req->op_list_vid.cnt-1]);
	if(op->type == LIMIT) {
		limit = &op->rule.limit;
        limit_buffer = *limit;

		// agent는 하위노드에게 lc*(pg+1) 의 결과를 요청함.
		limit->cnt = op->rule.limit.start + op->rule.limit.cnt;
		limit->start = 0;
		sprintf(op->clause, "LIMIT %d, %d", limit->start, limit->cnt);
	} else {
        error("can not found limit operation");
        return FAIL;
    }

    if(sb_run_qp_get_query_string(req, query) != SUCCESS) {
        error("can not make request");
        return FAIL;
    }
	
	if ( snprintf(path, MAX_QUERY_STRING_SIZE, 
                 "/search/light_search?q=%s", ap_escape_uri(r->pool, query)) <= 0 ){
		error("query to long");
		return FAIL;
	}
    debug("light query[%s]", path);

	for ( i=0; i<search_node_num; i++ ) {
	    http_client_t* client = search_nodes[i].client;

		if(client == NULL) {
		    search_nodes[i].client = sb_run_http_client_new(
                                         search_nodes[i].ip, 
                                         search_nodes[i].port);
			if ( search_nodes[i].client == NULL ) {
				error("sb_run_http_client_new failed");
				return FAIL;
			}
            client = search_nodes[i].client;
		} else {
			sb_run_http_client_reset(search_nodes[i].client);
			client = search_nodes[i].client;
		}

		client->http->request_http_ver = 1001;
		client->http->method = "GET";
		client->http->host = search_nodes[i].ip;
		client->http->path = path;
		client->http->req_content_type = "x-softbotd/binary";
		http_print(client->http);

		if ( sb_run_http_client_makeRequest(client, NULL)	!= SUCCESS ) {
			error("sb_run_http_client_makeRequest failed");
			return FAIL;
		}

        clients[i] = client;
		debug("request ip[%s], port[%s]", search_nodes[i].ip, search_nodes[i].port); 
	}

	if ( sb_run_http_client_retrieve(search_node_num, clients) != SUCCESS ){
		error("sb_run_http_client_retrieve failed");
		return FAIL;
	}

	for (node_idx = 0; node_idx < search_node_num ; node_idx++ ) {
		int i = 0;
        int recv_data_size = 0;
		uint32_t total_cnt = 0;
		uint32_t recv_cnt = 0;
		memfile *buf = clients[node_idx]->http->content_buf;
		if (!buf ) {
			error("no targetnode result at [%d]th node", i);
			return FAIL;
		}

        debug("recv data, address[%s]:[%s] :  size[%lu]", search_nodes[node_idx].ip,
				                                         search_nodes[node_idx].port,
														 memfile_getSize(buf));

		memfile_setOffset(buf, 0);

        // 0. node id  - 0 ~ 16 값임.
        recv_data_size = sizeof(uint32_t);
		if ( memfile_read(buf, (char *)&search_nodes[node_idx].node_id, recv_data_size) != recv_data_size ){
			error("incomplete result at [%d]th node: tot_cnt ", i);
			return FAIL;
		}

        // 1. 총 검색건수 
        recv_data_size = sizeof(uint32_t);
		if ( memfile_read(buf, (char *)&total_cnt, recv_data_size) != recv_data_size ){
			error("incomplete result at [%d]th node: tot_cnt ", i);
			return FAIL;
		}


        // 2. 전송 검색건수
        recv_data_size = sizeof(uint32_t);
		if ( memfile_read(buf, (char *)&recv_cnt, recv_data_size) != recv_data_size ){
			error("incomplete result at [%d]th node: num_row ", i);
			return FAIL;
		}

        // 3. 검색 단어 리스트, 모든 node가 같은 word_list를 전송해올것이다. 중복작업임.
        recv_data_size = STRING_SIZE;
		if ( memfile_read(buf, res->word_list, STRING_SIZE) != recv_data_size ) {
			error("incomplete result at [%d]th node: search_words ", i);
			return FAIL;
		}

		debug("node_id[%u], total_cnt[%d], recv_cnt[%d], word_list[%s]", 
				search_nodes[node_idx].node_id, total_cnt, recv_cnt, res->word_list);

		for( i = 0; i < recv_cnt; i++) {
            virtual_document_t* vd = &(res->vdl->data[res->vdl->cnt+i]);

			if(res->vdl->cnt + i > MAX_DOC_HITS_SIZE) {
                error("not enought agent_doc_hits buffer[%d], recv count[%d]", 
						MAX_AGENT_DOC_HITS_COUNT, res->vdl->cnt + i);
			    return FAIL;
			}

			recv_data_size = sizeof(uint32_t);
			if ( memfile_read(buf, (char*)&vd->id, recv_data_size) 
					!= recv_data_size ) {
				error("incomplete result at [%d]th node: vid ", i);
			    return FAIL;
			}

			recv_data_size = sizeof(uint32_t);
			if ( memfile_read(buf, (char*)&vd->relevancy, recv_data_size) 
					!= recv_data_size ) {
				error("incomplete result at [%d]th node: relevancy ", i);
			    return FAIL;
			}

			recv_data_size = sizeof(uint32_t);
			if ( memfile_read(buf, (char*)&vd->dochit_cnt, recv_data_size) 
					!= recv_data_size ) {
				error("incomplete result at [%d]th node: dochit_cnt ", i);
			    return FAIL;
			}

			if(g_dochit_cnt + vd->dochit_cnt > 
					MAX_AGENT_DOC_HITS_COUNT) {
				error("over max hit count[%u]", MAX_AGENT_DOC_HITS_COUNT);
				return FAIL;
			}

			recv_data_size = sizeof(doc_hit_t)*vd->dochit_cnt;
			if ( memfile_read(buf, 
                 (char*)&g_dochits_buffer[g_dochit_cnt],
                 recv_data_size) != recv_data_size ) {
				error("incomplete result at [%d]th node: doc_hits ", i);
			    return FAIL;
			}

			vd->dochits = &g_dochits_buffer[g_dochit_cnt];
			g_dochit_cnt += vd->dochit_cnt;

			recv_data_size = sizeof(uint32_t);
			if ( memfile_read(buf, (char*)&vd->node_id, recv_data_size) 
					!= recv_data_size ) {
				error("incomplete result at [%d]th node: node_id ", i);
			    return FAIL;
			}

			if(g_docattr_cnt + 1 > MAX_AGENT_DOCATTR_COUNT) {
				error("over max docattr count[%u]", MAX_AGENT_DOCATTR_COUNT);
				return FAIL;
			}

			// 6. docattr
			recv_data_size = sizeof(docattr_t);
			if ( memfile_read(buf, (char*)&g_docattr_buffer[g_docattr_cnt], recv_data_size) 
					!= recv_data_size ) {
				error("incomplete result at [%d]th node: docattr ", i);
			    return FAIL;
			}
			vd->docattr = &g_docattr_buffer[g_docattr_cnt];
			g_docattr_cnt++;

            print_virtual_document(vd);
		}
        res->vdl->cnt += recv_cnt;
        res->search_result += total_cnt;
	}

    // LIMIT operation 복귀
    *limit = limit_buffer;
    rv = sb_run_qp_do_filter_operation(req, res, VIRTUAL_DOCUMENT);
    if(rv == FAIL) {
        error("can not fileter operation - virtual_document");
        return FAIL;
    }
	
	return SUCCESS;
}


/*
 * req에는 send_first, send_cnt ali.agent_doc_hits(doc_hits, node_id) 만 유효하다.
 */
static int agent_abstractsearch(request_rec *r, request_t* req, response_t* res)
{
	int i = 0;
    int cmt_idx = 0;
	memfile *msg_body_list[MAX_SEARCH_NODE];
    http_client_t* clients[MAX_SEARCH_NODE];
	char path[MAX_QUERY_STRING_SIZE];

	// make request line
	if ( snprintf(path, MAX_QUERY_STRING_SIZE, 
                 "/search/abstract_search?q=%s", ap_escape_uri(r->pool, req->query)) <= 0 ){
		error("query to long");
		return FAIL;
	}

    // client가 초기화 되지 않았다면 초기화 한다.
	for (i=0; i<search_node_num; i++ ){
	    http_client_t *client = search_nodes[i].client;

		msg_body_list[i] = NULL;
		if( client == NULL ) {
			search_nodes[i].client = sb_run_http_client_new(
                                        search_nodes[i].ip, 
                                        search_nodes[i].port);
			if ( !client ) {
				error("sb_run_http_client_new failed");
				return FAIL;
			}
            client = search_nodes[i].client;
		} else {
			sb_run_http_client_reset(search_nodes[i].client);
			client = search_nodes[i].client;
		}

		client->http->request_http_ver = 1001;
		client->http->method = "POST";
		client->http->host = "softbot";
		client->http->path = path;
		client->http->req_content_type = "x-softbotd/binary";
		client->timeout = r->server->keep_alive_timeout;
		http_print(client->http);
	}

    // send buffer 초기화
    for ( i=0; i<search_node_num; i++ ) {
        memfile **buf = &(msg_body_list[i]);
        if ( *buf == NULL ) {
            *buf = memfile_new();
            if ( *buf == NULL ) {
                error("memfile_new failed");
                free_mfile_list(msg_body_list);
                return FAIL;
            }
        }
    }

	for(i = 0; i < res->vdl->cnt; i++) {
        virtual_document_t* vd = &(res->vdl->data[i]);
        memfile *buf = NULL;
		int client_idx = 0;
        uint32_t child_node_id = 0;
        uint32_t node_id = vd->node_id;

		// child의 node_id를 알아오기
		node_id = pop_node_id(node_id);
		child_node_id = get_node_id(node_id);
		debug("node_id[0x%0X] client_node_id[0x%0X]", node_id, child_node_id);

        client_idx = get_client_by_node_id(child_node_id);
		
        if(client_idx < 0) {
            error("cannot find client, client_idx[%d], node_id[0x%04X]", client_idx, child_node_id);
            continue;
        }

        buf = msg_body_list[client_idx];
		
	    //////////// abstract 요청 전송 ////////////////////////////////////////
        for(i = 0; i < vd->dochit_cnt; i++) {
			memfile_append(buf, (void *)&(vd->dochits[i]), sizeof(doc_hit_t));
			memfile_append(buf, (void *)&node_id, sizeof(uint32_t));
			debug("bufsize[%lu], abstractsearch docid[%u]", memfile_getSize(buf), vd->dochits[i].id);
        }
	} 

    // send...
	for ( i=0; i<search_node_num; i++ ) {
		memfile **buf = &(msg_body_list[i]);
		http_client_t* client = search_nodes[i].client;
		if ( *buf == NULL ) {
            error("client is null, [%d]", i);
			continue;
		}

		http_setMessageBody(client->http, *buf, "x-softbotd/binary", memfile_getSize(*buf));
		if ( sb_run_http_client_makeRequest(client, NULL)	!= SUCCESS ) {
			client->http->req_message_body = NULL;
			error("sb_run_http_client_makeRequest failed");
			free_mfile_list(msg_body_list);
			return FAIL;
		}
		client->http->req_message_body = NULL;

		memfile_free(*buf);
		*buf= NULL;

        clients[i] = client;
		debug("request ip[%s], port[%s]", search_nodes[i].ip, search_nodes[i].port); 
	}
	free_mfile_list(msg_body_list);

    // recv...
	if ( sb_run_http_client_retrieve(search_node_num, clients) != SUCCESS){
		error("sb_run_http_client_retrieve failed");
		//FIXME : return FAIL;
	}
	
	for (i=0; i <search_node_num; i++ ) {
        int j = 0;
		int recv_data_size = 0;
		int recv_cnt = 0;

		memfile *buf = clients[i]->http->content_buf;
		if (!buf ) {
			error("no targetnode result at [%d]th node", i);
			return FAIL;
		}
		memfile_setOffset(buf, 0);

	    /////////// abstarct search 결과 수신 ////////////////////////////////////////
        // 1. 전송 comment 건수
        recv_data_size = sizeof(uint32_t);
		if ( memfile_read(buf, (char *)&recv_cnt, recv_data_size) != recv_data_size ){
			error("incomplete result at [%d]th node: num_row ", i);
			continue;
		}

		info("client[%d], recv_cnt[%u]", i, recv_cnt);

		for(j = 0; j < recv_cnt; j++) {
            comment_t* cmt = &res->comments[cmt_idx];

            if(cmt_idx > COMMENT_LIST_SIZE) {
                error("over max comment count[%d]", COMMENT_LIST_SIZE);
                return FAIL;
            }

			// 2.2 did
			recv_data_size = sizeof(uint32_t);
			if ( memfile_read(buf, (char*)&cmt->did, recv_data_size) 
					!= recv_data_size ) {
				error("incomplete result at [%d]th node: doc_id ", j);
				return FAIL;
			}

			// 2.2 node_id
			recv_data_size = sizeof(uint32_t);
			if ( memfile_read(buf, (char*)&cmt->node_id, recv_data_size) 
					!= recv_data_size ) {
				error("incomplete result at [%d]th node: node_id ", j);
				return FAIL;
			}

			// 2.3 comment
			recv_data_size = LONG_LONG_STRING_SIZE;
			if ( memfile_read(buf, (char*)cmt->s, recv_data_size) 
					!= recv_data_size ) {
				error("incomplete result at [%d]th node: comment ", j);
				return FAIL;
			}

            debug("did[%u], node_id[%u], comments[%s]", 
                     cmt->did, cmt->node_id, cmt->s);
            cmt_idx++;
		}
	}

	return SUCCESS;
}

/*
 * root agent 에서 실행됨
 */
static int search_handler(request_rec *r, softbot_handler_rec *s){
    int rv = 0;
    int i = 0, j = 0;
    groupby_result_list_t* groupby_result = NULL;

    init_agent();

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

	timelog("agent_lightsearch_start");
	rv = agent_lightsearch(r, &qp_request, &qp_response);
	if ( rv != SUCCESS ) {
		error("agent_lightsearch failed");
		return FAIL;
	}
	timelog("agent_lightsearch_finish");


    // 자신의 node_id를 push.
    for(i = 0; i < qp_response.vdl->cnt; i++) {
        qp_response.vdl->data[i].node_id = 
                 push_node_id(qp_response.vdl->data[i].node_id, this_node_id);
    }

    // 출력해야할 문서가 있으면 comment를 추가로 가져옴
	if ( qp_response.vdl->cnt > 0 ){
	    timelog("agent_abstractsearch_start");
		rv = agent_abstractsearch(r, &qp_request, &qp_response);
		if( rv != SUCCESS ) {
			error("agent_abstractsearch failed");
			return FAIL;
		}
	    timelog("agent_abstractsearch_finish");
	}

	ap_set_content_type(r, "text/xml; charset=euc-kr");
	ap_rprintf(r,
			"<?xml version=\"1.0\" encoding=\"euc-kr\" ?>\n");

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

        // group °a°u
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
        ap_rprintf(r, "<comment_count>%u</comment_count>\n", vd->dochit_cnt);

        for(j = 0; j < vd->dochit_cnt; j++) {
            ap_rprintf(r, "<comment><![CDATA[%s]]></comment>\n", 
               find_comment(qp_response.comments, vd->dochits[j].id, pop_node_id(vd->node_id)));
        }

        ap_rprintf(r, "</row>\n");
    }

    ap_rprintf(r, "</search>\n</xml>\n");
    timelog("send_result_finish");

    timelog("qp_finalize");

	return SUCCESS;
}
/////////////////////////////////////////////////////////////////////
/*
 * 상위 agent로 부터 request를 하위 agent로 bypass 
 * 하위 agent의 response 상위 agent로 by pass
 */
static int light_search_handler(request_rec *r, softbot_handler_rec *s){
    int rv = 0;
    int i = 0;

    init_agent();

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

    // 하위 agent에게 light search
	timelog("agent_lightsearch_start");
	rv = agent_lightsearch(r, &qp_request, &qp_response);
	if ( rv != SUCCESS ) {
		error("agent_lightsearch failed");
		return FAIL;
	}
	timelog("agent_lightsearch_finish");

    // 0. node id  - 0 ~ 16 값임.
	ap_rwrite(&this_node_id, sizeof(uint32_t), r);
    // 1. 총 검색건수 
	ap_rwrite(&qp_response.search_result, sizeof(uint32_t), r);
    // 2. 전송 검색건수
	ap_rwrite(&qp_response.vdl->cnt, sizeof(uint32_t), r);
    // 3. 검색 단어 리스트
	ap_rwrite(qp_response.word_list, sizeof(char)*STRING_SIZE, r);

    for(i = 0; i < qp_response.vdl->cnt; i++) {
        virtual_document_t* vd = &(qp_response.vdl->data[i]);
    
        ap_rwrite((void*)&vd->id, sizeof(uint32_t), r);
    
        ap_rwrite((void*)&vd->relevancy, sizeof(uint32_t), r);
    
        ap_rwrite((void*)&vd->dochit_cnt, sizeof(uint32_t), r);
        
        ap_rwrite((void*)vd->dochits, sizeof(doc_hit_t)*vd->dochit_cnt, r);

	    // light search 결과는 상위 agent에게 보고 되므로 자신의 node_id를 push
        vd->node_id = push_node_id(vd->node_id, this_node_id);

        ap_rwrite(&(vd->node_id), sizeof(uint32_t), r);
        debug("send node_id[%u]", vd->node_id);
        
        ap_rwrite((void*)vd->docattr, sizeof(docattr_t), r);
    }

	return SUCCESS;
}

///////////////////////////////////////////////////////////////////////
/*
 * dochits 정보를 받기.
 * abstract_search
 * comment 전송하기
 */
static int abstract_search_handler(request_rec *r, softbot_handler_rec *s)
{
    int rv = 0;
    int i = 0;
    int recv_pos = 0;
    memfile *buf = NULL;
    virtual_document_t* vd = NULL;
    
    init_agent();

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
    
    qp_response.vdl->cnt = 1;
    vd = &(qp_response.vdl->data[0]);
    
    if (sb_run_sbhandler_make_memfile(r, &buf) != SUCCESS) {
        error("make_memfile_from_postdata failed");
        return FAIL;
    }
    
    vd->dochit_cnt = memfile_getSize(buf) / (sizeof(doc_hit_t) + sizeof(uint32_t));
    for (i = 0; i < vd->dochit_cnt; i++) {
        uint32_t node_id = 0;
        uint32_t recv_data_size = 0;

        recv_data_size = sizeof(doc_hit_t);
        if ( memfile_read(buf, (char*)&(vd->dochits[recv_pos]), recv_data_size)
                != recv_data_size ) {
            error("incomplete result at [%d]th node: doc_hits ", i);
            continue;
        }

        recv_data_size = sizeof(uint32_t);
        if ( memfile_read(buf, (char*)&node_id, recv_data_size)
                != recv_data_size ) {
            error("incomplete result at [%d]th node: doc_hits ", i);
            continue;
        }

        if(get_node_id(node_id) != this_node_id) {
            error("node_id[%u] not equals this_node_id[%u]",
                          get_node_id(node_id),
                          this_node_id);
            continue;
        }

        recv_pos++;
    }
    vd->dochit_cnt = recv_pos;


	timelog("agent_abstractsearch_start");
	rv = agent_abstractsearch(r, &qp_request, &qp_response);
	if( rv != SUCCESS ) {
		error("agent_abstractsearch failed");
		return FAIL;
	}
	timelog("agent_abstractsearch_end");

    ap_set_content_type(r, "x-softbotd/binary");

    ap_rwrite(&vd->dochit_cnt, sizeof(uint32_t), r);

    for (i = 0; i < vd->dochit_cnt; i++ ) {
        ap_rwrite(&qp_response.comments[i].did, sizeof(uint32_t), r);
        qp_response.comments[i].node_id = 
              push_node_id(qp_response.comments[i].node_id, this_node_id);
        ap_rwrite(&qp_response.comments[i].node_id, sizeof(uint32_t), r);
        ap_rwrite(qp_response.comments[i].s, LONG_LONG_STRING_SIZE, r);
        debug("comments[%s], node_id[%u]", 
                    qp_response.comments[i].s,
                    qp_response.comments[i].node_id);
    }

	return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////
static int get_table(char *name_space, void **tab)
{
	if ( strcmp(name_space, "agent") == 0 ){
		*tab = agent_handler_tbl;
		return SUCCESS;
	} else if ( strcmp(name_space, "search") == 0 ){
		*tab = agent_search_handler_tbl;
		return SUCCESS;
	}
	return DECLINE;
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

static void set_node_id(configValue v)
{
    this_node_id = atoi(v.argument[0]);
	if(this_node_id > 16) {
        error("node_id should be smaller than 16");
	}

	info("node_id[%u]", this_node_id);
}

static void set_search_node(configValue v)
{
    char *ptr;

    memset(&search_nodes[search_node_num], 0, sizeof(node_t));

    strncpy(search_nodes[search_node_num].ip, v.argument[0], STRING_SIZE);

    ptr = strchr(search_nodes[search_node_num].ip, ':');
    if (!ptr) {
        strncpy(search_nodes[search_node_num].port, DEFAULT_PORT, SHORT_STRING_SIZE);
    } else {
        strncpy(search_nodes[search_node_num].port, ptr+1, SHORT_STRING_SIZE);
        *ptr = '\0';
    }

    search_nodes[search_node_num].client = NULL;

    info("search node[%d] added: ip[%s], port[%s]", search_node_num,
            search_nodes[search_node_num].ip, search_nodes[search_node_num].port);

    search_node_num++;
}

static config_t config[] = {
    CONFIG_GET("AddSearchNode", set_search_node, 1,
            "add search machine : AddSearchNode [ip:port]"),
    CONFIG_GET("NodeId", set_node_id, 1, "set node id : NodeId [no]"),
    CONFIG_GET("Enum", get_enum, 2, "constant"),
    {NULL}
};

static void register_hooks(void)
{
	sb_hook_sbhandler_get_table(get_table,NULL,NULL,HOOK_MIDDLE);
	return;
}

module agent_sbhandler_module = {
	STANDARD_MODULE_STUFF,
	config,                   /* config            */
	NULL,                     /* registry          */
	NULL,                     /* initialize        */
	NULL,                     /* child_main        */
	NULL,                     /* scoreboard        */
	register_hooks            /* register hook api */
};
