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
#include "mod_api/qp.h"
#include "mod_qp/mod_qp.h"
#include "mod_api/http_client.h"

static int search_handler(request_rec *r, softbot_handler_rec *s);
static int light_search_handler(request_rec *r, softbot_handler_rec *s);
static int abstract_search_handler(request_rec *r, softbot_handler_rec *s);
static agent_request_t* g_agent_request;
static agent_doc_hits_t* g_agent_doc_hits;

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
// node ����
typedef struct {
    char ip[STRING_SIZE];
    char port[SHORT_STRING_SIZE];
    http_client_t* client;
	uint32_t node_id;
} node_t;
static uint32_t this_node_id; // ���� 4bit�� ����.

/*
 * search_nodes, clients�� 1:1�� ������� ���εȴ�.
 */
static int search_node_num = 0;
node_t search_nodes[MAX_SEARCH_NODE];

//--------------------------------------------------------------//
//	*   custom function	
//--------------------------------------------------------------//
static char *replace_newline_to_space(char *str) {
    char *ch;
    ch = str;       
    while ( (ch = strchr(ch, '\n')) != NULL ) {
        *ch = ' ';
    }
    return str;
}   

/*
 * ���� 4bit�� node_id�� push
 * push �� node_id�� ����
 */
static uint32_t push_node_id(uint32_t node_id)
{
	// ���� 4bit�� ������ ������ ���̻� depth�� �ø��� ����.
	if((node_id >> 28) > 0) {
		error("depth overflower[0x%0X]", node_id);
		return 0;
	}

    node_id = node_id << 4;
	node_id |= this_node_id;

	return node_id;
}

/*
 * ���� 4bit�� pop�Ѵ�.
 * pop �� node_id�� ����.
 */
static uint32_t pop_node_id(uint32_t node_id)
{
    return (node_id >> 4);
}

// ���� 4bit�� node_id �˾Ƴ���
static uint32_t get_node_id(uint32_t node_id)
{
	return node_id & 0x0f;
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

/*
 * �˻� �������� &, +�� url syntax �� ����Ǵ� �׸�����
 * escape ��󿡼� ���ܵǹǷ� Ư���ϰ� �����Ͽ� �ش�.
 * URL reserved characters
 * $ & + , / : ; = ? @
 */
static char* escape_ampersand(apr_pool_t *p, const char *path)
{   
    char *copy = apr_palloc(p, 3 * strlen(path) + 3);
    const unsigned char *s = (const unsigned char *)path;
    unsigned char *d = (unsigned char *)copy;
    unsigned c;

    while ((c = *s)) {
    if (c == '&') {
        *d++ = '%';
        *d++ = '2';
        *d++ = '6';
    } else if( c == '+') {
        *d++ = '%';
        *d++ = '2';
        *d++ = 'B';
    }               
    else {
        *d++ = c;
    }
    ++s;
    }
    *d = '\0';
    return copy;
}

//--------------------------------------------------------------//
//	*	agent_search implemetation
//--------------------------------------------------------------//
static int agent_lightsearch(request_rec *r, agent_request_t* req) {
	int i = 0;
	int node_idx = 0;
	char path[MAX_QUERY_STRING_SIZE];
    agent_light_info_t* ali = &req->ali;
    http_client_t* clients[MAX_SEARCH_NODE];

	// make request line : agent�� ������忡�� lc*(pg+1) �� ����� ��û��.
	if ( snprintf(path, MAX_QUERY_STRING_SIZE, 
				"/search/light_search?q=%s&at=%s&at2=%s&gr=%s&sh=%s&lc=%d&pg=0&ft=%d",
				escape_ampersand(r->pool, ap_escape_uri(r->pool, req->query) ), 
				escape_ampersand(r->pool, ap_escape_uri(r->pool, req->at) ), 
				escape_ampersand(r->pool, ap_escape_uri(r->pool, req->at2) ),
				req->gr, req->sh, req->lc*(req->pg+1), req->ft) <= 0 ){
		error("query to long");
		return FAIL;
	}
	
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
		client->timeout = r->server->keep_alive_timeout;
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
			continue;
		}

        debug("recv data, address[%s]:[%s] :  size[%lu]", search_nodes[node_idx].ip,
				                                         search_nodes[node_idx].port,
														 memfile_getSize(buf));

		memfile_setOffset(buf, 0);

        // 0. node id  - 0 ~ 16 ����.
        recv_data_size = sizeof(uint32_t);
		if ( memfile_read(buf, (char *)&search_nodes[node_idx].node_id, recv_data_size) != recv_data_size ){
			error("incomplete result at [%d]th node: tot_cnt ", i);
			continue;
		}

        // 1. �� �˻��Ǽ� 
        recv_data_size = sizeof(uint32_t);
		if ( memfile_read(buf, (char *)&total_cnt, recv_data_size) != recv_data_size ){
			error("incomplete result at [%d]th node: tot_cnt ", i);
			continue;
		}


        // 2. ���� �˻��Ǽ�
        recv_data_size = sizeof(uint32_t);
		if ( memfile_read(buf, (char *)&recv_cnt, recv_data_size) != recv_data_size ){
			error("incomplete result at [%d]th node: num_row ", i);
			continue;
		}

		debug("recv_cnt[%d]", recv_cnt);


        // 3. �˻� �ܾ� ����Ʈ, ��� node�� ���� word_list�� �����ؿð��̴�. �ߺ��۾���.
        recv_data_size = STRING_SIZE;
		if ( memfile_read(buf, ali->word_list, STRING_SIZE) != recv_data_size ) {
			error("incomplete result at [%d]th node: search_words ", i);
			continue;
		}

		for( i = 0; i < recv_cnt; i++) {
			if(ali->recv_cnt + i > MAX_AGENT_DOC_HITS_COUNT) {
                error("not enought agent_doc_hits buffer[%d], recv count[%d]", 
						MAX_AGENT_DOC_HITS_COUNT, ali->recv_cnt+i);
				break;
			}

			// 4-1. ���ü�
			recv_data_size = sizeof(uint32_t);
			if ( memfile_read(buf, (char*)&ali->agent_doc_hits[ali->recv_cnt+i]->relevancy, recv_data_size) 
					!= recv_data_size ) {
				error("incomplete result at [%d]th node: relevancy ", i);
				continue;
			}

			// 4-2. doc_hits 
			recv_data_size = sizeof(doc_hit_t);
			if ( memfile_read(buf, (char*)&ali->agent_doc_hits[ali->recv_cnt+i]->doc_hits, recv_data_size) 
					!= recv_data_size ) {
				error("incomplete result at [%d]th node: doc_hits ", i);
				continue;
			}
		    debug("abstractsearch docid[%u]", ali->agent_doc_hits[ali->recv_cnt+i]->doc_hits.id);

			// 5. node_id
			recv_data_size = sizeof(uint32_t);
			if ( memfile_read(buf, (char*)&ali->agent_doc_hits[ali->recv_cnt+i]->node_id, recv_data_size) 
					!= recv_data_size ) {
				error("incomplete result at [%d]th node: node_id ", i);
				continue;
			}

			debug("recv node_id[0x%0X]", ali->agent_doc_hits[ali->recv_cnt+i]->node_id);

			// 6. docattr
			recv_data_size = sizeof(docattr_t);
			if ( memfile_read(buf, (char*)&ali->agent_doc_hits[ali->recv_cnt+i]->docattr, recv_data_size) 
					!= recv_data_size ) {
				error("incomplete result at [%d]th node: doc_attr ", i);
				continue;
			}
		}
        ali->total_cnt += total_cnt;
        ali->recv_cnt += recv_cnt;
		debug("total recv_cnt[%d]", ali->recv_cnt);
	}

	//merge
	// �̹� recv �ÿ� merge �ǵ��� �Ѵ�.
	//sort
	sb_run_qp_agent_info_sort(req);
	
	return SUCCESS;
}


static void free_mfile_list(memfile **mfile_list){
	int i;
	for (i=0; i<search_node_num; i++ ) {
		if ( mfile_list[i] ) memfile_free(mfile_list[i]);
	}
}

/*
 * req���� send_first, send_cnt ali.agent_doc_hits(doc_hits, node_id) �� ��ȿ�ϴ�.
 */
static int agent_abstractsearch(request_rec *r, agent_request_t* req){
	int i = 0, last = 0;
	memfile *msg_body_list[MAX_SEARCH_NODE];
    http_client_t* clients[MAX_SEARCH_NODE];
    agent_light_info_t* ali = &req->ali;

	char path[LONG_STRING_SIZE];
	// make request line
	if ( snprintf(path, LONG_STRING_SIZE, 
				"/search/abstract_search") <= 0 ){
		error("query to long");
		return FAIL;
	}

    // client�� �ʱ�ȭ ���� �ʾҴٸ� �ʱ�ȭ �Ѵ�.
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

    // send buffer �ʱ�ȭ
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

    /*
	 * send buffer�� ���� ����Ÿ ����.
     *
     * child node�� node_id�� child�� ���� ����� �̷�� ���� �˼� �ְԵȴ�.
     * ������ abstract_search ��û�� ���� ��� child agent�� reqeust�ؾ� �ϴ��� ���� ���Ѵ�.
     * �̰��� �׳� jump
	 */
	// ����ؾ��� �������� comment�� ��û
	last = req->send_first + req->send_cnt;
	for(i = req->send_first; i < last; i++) {
        memfile *buf = NULL;
		int client_idx = 0;
        uint32_t child_node_id = 0;
        uint32_t node_id = ali->agent_doc_hits[i]->node_id;


		// child�� node_id�� �˾ƿ���
		node_id = pop_node_id(node_id);
		child_node_id = get_node_id(node_id);
		debug("node_id[0x%0X] client_node_id[0x%0X]", node_id, child_node_id);

        client_idx = get_client_by_node_id(child_node_id);
		
        if(client_idx < 0) {
            error("cannot find client, client_idx[%d], node_id[0x%04X]", client_idx, child_node_id);
            continue;
        }

        buf = msg_body_list[client_idx];
		
	    //////////// abstract ��û ���� ////////////////////////////////////////
		memfile_append(buf, (void *)&(ali->agent_doc_hits[i]->doc_hits), sizeof(doc_hit_t));
		memfile_append(buf, (void *)&node_id, sizeof(uint32_t));
		debug("bufsize[%lu], abstractsearch docid[%u]", memfile_getSize(buf), ali->agent_doc_hits[i]->doc_hits.id);
	} 

    // send...
	for ( i=0; i<search_node_num; i++ ) {
		memfile **buf = &(msg_body_list[i]);
		http_client_t* client = search_nodes[i].client;
		if ( *buf == NULL ) {
            error("client is null, [%d]", i);
			continue;
		}

		/*
		if ( memfile_getSize(*buf) == 0 ) {
            error("client[%d]'s buffer length 0, do not send request", i);
			client->current_request_buffer = NULL;
			continue;
		}
		*/
		
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
		uint32_t doc_id = 0;
		uint32_t recv_node_id = 0;
		int recv_cnt = 0;
		char comment[LONG_LONG_STRING_SIZE];

		memfile *buf = clients[i]->http->content_buf;
		if (!buf ) {
			error("no targetnode result at [%d]th node", i);
			return FAIL;
		}
		memfile_setOffset(buf, 0);

	    /////////// abstarct search ��� ���� ////////////////////////////////////////
        // 1. ���� comment �Ǽ�
        recv_data_size = sizeof(uint32_t);
		if ( memfile_read(buf, (char *)&recv_cnt, recv_data_size) != recv_data_size ){
			error("incomplete result at [%d]th node: num_row ", i);
			continue;
		}

		info("client[%d], recv_cnt[%u]", i, recv_cnt);

		for(j = 0; j < recv_cnt; j++) {
			int k = 0;
			int found = 0;

			// 2.1 doc_id
			recv_data_size = sizeof(uint32_t);
			if ( memfile_read(buf, (char*)&doc_id, recv_data_size) 
					!= recv_data_size ) {
				error("incomplete result at [%d]th node: doc_id ", j);
				continue;
			}

			// 2.2 node_id
			recv_data_size = sizeof(uint32_t);
			if ( memfile_read(buf, (char*)&recv_node_id, recv_data_size) 
					!= recv_data_size ) {
				error("incomplete result at [%d]th node: node_id ", j);
				continue;
			}

			recv_node_id = get_node_id(recv_node_id);

			// 2.3 comment
			recv_data_size = LONG_LONG_STRING_SIZE;
			if ( memfile_read(buf, (char*)comment, recv_data_size) 
					!= recv_data_size ) {
				error("incomplete result at [%d]th node: comment ", j);
				continue;
			}

            debug("doc_id[%u], comments[%s]", doc_id, comment);

		    // ������ comment�� ã�Ƽ� �ִ´�.
	        // ����ؾ��� ������ ������ ã���� �ȴ�.
			found = 0;
	        last = req->send_first + req->send_cnt;
			for(k = req->send_first; k < last; k++) {
				uint32_t child_node_id = get_node_id( pop_node_id(ali->agent_doc_hits[k]->node_id) );

				if(doc_id == ali->agent_doc_hits[k]->doc_hits.id &&
					child_node_id == recv_node_id) {
				    memcpy(ali->comments[k - req->send_first], comment, LONG_LONG_STRING_SIZE); 
				    found = 1;
				}
			}

			if(!found) {
				error("can not find doc_id[%u] for set comment", doc_id);
			}
		}
	}

	return SUCCESS;
}
//--------------------------------------------------------------//


/////////////////////////////////////////////////////////////////////
static int def_atoi(const char *s, int def)
{
	if (s)
		return atoi(s);
	return def;
}

static int make_request(softbot_handler_rec* s, agent_request_t* req, char* query_string)
{
    char* p = NULL;

    //1. set request_t
    /* query */
    p = (char *)apr_table_get(s->parameters_in, "q");
    if (p == NULL) {
        error("no query string");
        return FAIL;
    }
    strncpy(req->query, p, MAX_QUERY_STRING_SIZE-1);

    /* AT */
    p = (char *)apr_table_get(s->parameters_in, "at");
    if (p != NULL) {
        strncpy(req->at, p, MAX_ATTR_STRING_SIZE-1);
    }

    /* AT2 */
    p = (char *)apr_table_get(s->parameters_in, "at2");
    if (p != NULL) {
        strncpy(req->at2, p, MAX_ATTR_STRING_SIZE-1);
    }

    /* GR */
    p = (char *)apr_table_get(s->parameters_in, "gr");
    if (p != NULL) {
        strncpy(req->gr, p, MAX_GROUP_STRING_SIZE-1);
    }

    /* SH */
    p = (char *)apr_table_get(s->parameters_in, "sh");
    if (p != NULL) {
        strncpy(req->sh, p, MAX_SORT_STRING_SIZE-1);
    }

    /* LC */
    req->lc = def_atoi(apr_table_get(s->parameters_in, "lc"), 20);
    if(req->lc < 0) req->lc = 20;

    /* PG */
    req->pg = def_atoi(apr_table_get(s->parameters_in, "pg"), 0);
    if(req->pg < 0) req->pg = 0;

    /* FT */
    req->ft = def_atoi(apr_table_get(s->parameters_in, "ft"), 0);

    /* reqeust debug string */
    debug("req->query_string:[%s]",req->query);
    debug("req->attr_string:[%s]",req->at);
    debug("req->attr2_string:[%s]",req->at2);
    debug("req->group_string:[%s]",req->gr);
    debug("req->sort_string:[%s]",req->sh);
    debug("req->list_size:%d",req->lc);
    debug("req->first_result:%d",req->pg);
    debug("req->filtering_id:%d", req->ft);

    if( snprintf(query_string, MAX_QUERY_STRING_SIZE, 
				"q=%s&at=%s&at2=%s&gr=%s&sh=%s&lc=%d&pg=%d&ft=%d",
                   req->query, req->at, req->at2, req->gr,
                   req->sh, req->lc, req->pg, req->ft) <= 0 ){
        error("query to long");
        return FAIL;
    }

    return SUCCESS;
}


// ��� �����ؾ� �ϴ��� ����Ѵ�.
static void get_send_count(agent_request_t* req)
{
    int pg = req->pg;
    int lc = req->lc;
	int total_cnt = req->ali.total_cnt;
    
    if (total_cnt < pg*lc) {
        req->send_cnt = 0;
    }
    else {
        int remain = total_cnt - pg*lc;
        
        if (remain > lc) { 
            req->send_cnt = lc; // page �� �ڷ� ���� ���Ҵ�.
        }
        else {
            req->send_cnt = remain; // ������ page. 
        }
        
        if ( req->send_cnt > COMMENT_LIST_SIZE ) {
            warn("searched_list_size[%d] > COMMENT_LIST_SIZE[%d]",
                    req->send_cnt, COMMENT_LIST_SIZE);
            req->send_cnt = COMMENT_LIST_SIZE;
        }
    }

	req->send_first = pg*lc;
}

static void init_agent_request(agent_request_t** req)
{
	int i = 0;

    if(g_agent_request == NULL) {
		g_agent_request = (agent_request_t*)sb_malloc(sizeof(agent_request_t));
	}

	// MAX_AGENT_DOC_HITS_COUNT�� memory �Ҵ� ���� �ʰ� �ѹ��� �Ҵ�ް� �����Ѵ�.
	if(g_agent_doc_hits == NULL) {
		g_agent_doc_hits = (agent_doc_hits_t*)
			            sb_malloc(sizeof(agent_doc_hits_t)*MAX_AGENT_DOC_HITS_COUNT);

		// pointer ����
		for(i = 0; i < MAX_AGENT_DOC_HITS_COUNT; i++) {
            g_agent_request->ali.agent_doc_hits[i] = &g_agent_doc_hits[i];
		}
	}

	//  �ʱ�ȭ
	g_agent_request->lc = 0;
	g_agent_request->pg = 0;
	g_agent_request->ft = 0;
	g_agent_request->send_cnt = 0;
	g_agent_request->send_first = 0;
	g_agent_request->ali.total_cnt = 0;
	g_agent_request->ali.recv_cnt = 0;

	// g_agent_request->ali�� ���� �ʱ�ȭ ��Ű�� ����, ���� pointer ���ᶧ����. 

	*req = g_agent_request;

	return;
}

/*
 * root agent ���� �����
 */
static int search_handler(request_rec *r, softbot_handler_rec *s){
    int i = 0, j = 0, last = 0, ret = 0;
    char query[MAX_QUERY_STRING_SIZE];
	agent_request_t* req = NULL;

	init_agent_request(&req);

    // url parameter �м�.
    if(make_request(s, req, query) != SUCCESS) {
        error("can not make request string");
        return FAIL;
    }
    
    // light search
	timelog("agent_lightsearch_start");
	debug("r->server->keep_alive_timeout[%lld]", r->server->keep_alive_timeout);
	debug("r->server->keep_timeout[%lld]", r->server->timeout);
	ret = agent_lightsearch(r, req);
	if ( ret != SUCCESS ) {
		error("agent_lightsearch failed");
		return FAIL;
	}
	timelog("agent_lightsearch_finish");

    // �ڽ��� node_id�� push.
    for(i = 0; i < req->ali.recv_cnt; i++) {
        req->ali.agent_doc_hits[i]->node_id = push_node_id(req->ali.agent_doc_hits[i]->node_id);
    }
    // url parameter, light search ����� �м��Ͽ� ��°Ǽ��� ����
    get_send_count(req);

    // ����ؾ��� ������ ������ comment�� �߰��� ������
	if ( req->send_cnt > 0 ){
	    timelog("agent_abstractsearch_start");
		ret = agent_abstractsearch(r, req);
		if( ret != SUCCESS ) {
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
            "<total_page>%d</total_page>\n"
            "<page_no>%d</page_no>\n"
            "<page_size>%d</page_size>\n"
            "<result_count>%d</result_count>\n"
            "</summary>\n",
            query, 
			req->ali.word_list, 
			req->ali.total_cnt, 
            req->ali.total_cnt/req->lc + ((req->ali.total_cnt > 0 && req->ali.total_cnt%req->lc == 0) ? -1 : 0),
            req->pg,
            req->lc, 
			req->send_cnt);
    
    /* each result */
	last = req->send_first + req->send_cnt;
    for (i = req->send_first, j = 0; i < last; i++, j++) {
        ap_rprintf(r, 
                "<row>"
                "<rowno>%d</rowno>"
                "<did>%d</did>"
				"<nodeid>%08X</nodeid>"
                "<relevance>%d</relevance>"
                "<comment><![CDATA[%s]]></comment>"
                "</row>\n",
                i,
                req->ali.agent_doc_hits[i]->doc_hits.id,
                req->ali.agent_doc_hits[i]->node_id,
                req->ali.agent_doc_hits[i]->relevancy,
                replace_newline_to_space(req->ali.comments[j]));
    }
	ap_rprintf(r, "</search>\n</xml>");

	return SUCCESS;
}
/////////////////////////////////////////////////////////////////////
/*
 * ���� agent�� ���� request�� ���� agent�� bypass 
 * ���� agent�� response ���� agent�� by pass
 */
static int light_search_handler(request_rec *r, softbot_handler_rec *s){
    int i = 0, ret = 0;
    char query[MAX_QUERY_STRING_SIZE];
	agent_request_t* req = NULL;
    
	init_agent_request(&req);

    // url parameter �м�.
    if(make_request(s, req, query) != SUCCESS) {
        error("can not make request string");
        return FAIL;
    }

    // light search�� binary ����ϱ�� �Ծ�Ǿ� ����.
	CHECK_REQUEST_CONTENT_TYPE(r, "x-softbotd/binary");
    
    // ���� agent���� light search
	timelog("agent_lightsearch_start");
	ret = agent_lightsearch(r, req);
	if ( ret != SUCCESS ) {
		error("agent_lightsearch failed");
		return FAIL;
	}
	timelog("agent_lightsearch_finish");

    // url parameter, lightseach ����� �м��Ͽ� ����� ������ ����.
    get_send_count(req);

    // 0. node id  - 0 ~ 16 ����.
	ap_rwrite(&this_node_id, sizeof(uint32_t), r);
    // 1. �� �˻��Ǽ� 
	ap_rwrite(&req->ali.total_cnt, sizeof(uint32_t), r);
    // 2. ���� �˻��Ǽ�
	ap_rwrite(&req->send_cnt, sizeof(uint32_t), r);
    // 3. �˻� �ܾ� ����Ʈ
	ap_rwrite(req->ali.word_list, sizeof(char)*STRING_SIZE, r);

    // 4. �˻���� ����
	for (i=0; i< req->send_cnt; i++ ){
        // 4-1. ���ü� ����
		ap_rwrite(&(req->ali.agent_doc_hits[i]->relevancy), sizeof(uint32_t), r);
        // 4-2. doc_hits ����
		ap_rwrite(&(req->ali.agent_doc_hits[i]->doc_hits), sizeof(doc_hit_t), r);
        // 4-3. node id - 32bit
	    // light search ����� ���� agent���� ���� �ǹǷ� �ڽ��� node_id�� push
	    req->ali.agent_doc_hits[i]->node_id = 
	    push_node_id(req->ali.agent_doc_hits[i]->node_id);
        ap_rwrite(&(req->ali.agent_doc_hits[i]->node_id), sizeof(uint32_t), r);
        // 4-4. docattr ����
	 	ap_rwrite(&(req->ali.agent_doc_hits[i]->docattr), sizeof(docattr_t), r);
	}

	return SUCCESS;
}

///////////////////////////////////////////////////////////////////////
/*
 * dochits ������ �ޱ�.
 * abstract_search
 * comment �����ϱ�
 */
static int abstract_search_handler(request_rec *r, softbot_handler_rec *s){
    int i = 0, ret = 0, last = 0;
    char query[MAX_QUERY_STRING_SIZE];
	memfile *buf;
	int recv_data_size = 0;
    int recv_cancel_cnt = 0;
	agent_request_t* req = NULL;

	init_agent_request(&req);
	/* url parameter ����.
    if(make_request(s, req, query) != SUCCESS) {
        error("can not make request string");
        return FAIL;
    }
	*/
	
	CHECK_REQUEST_CONTENT_TYPE(r, "x-softbotd/binary");

	if (sb_run_sbhandler_make_memfile(r, &buf) != SUCCESS) {
		error("make_memfile_from_postdata failed");
		return FAIL;
	}

	//////////// abstract ��û ���� ////////////////////////////////////////
	// 1. ���۰Ǽ� ����
    // 1. ���ŰǼ�(agent���� recv_cnt�� ����Ҽ� ���� ������ size�� �ؾ� �Ѵ�.)
    req->ali.recv_cnt = memfile_getSize(buf) / (sizeof(doc_hit_t) + sizeof(uint32_t));
    info("recv cnt[%d]", req->ali.recv_cnt);

	// 2. agent_doc_hits ����
	for (i=0; i<req->ali.recv_cnt; i++ ) {
        // 2.1 doc_hit ����
		recv_data_size = sizeof(doc_hit_t);
		if ( memfile_read(buf, (char*)&req->ali.agent_doc_hits[i]->doc_hits, recv_data_size) 
				!= recv_data_size ) {
			error("incomplete result at [%d]th node: doc_hits ", i);
			continue;
		}
        
        // 2.2 node_id ����
		recv_data_size = sizeof(uint32_t);
		if ( memfile_read(buf, (char*)&req->ali.agent_doc_hits[i]->node_id, recv_data_size) 
				!= recv_data_size ) {
			error("incomplete result at [%d]th node: doc_hits ", i);
			continue;
		}

        // �ڽ��� node_id�� �ƴϸ� error
        if(get_node_id(req->ali.agent_doc_hits[i]->node_id) != this_node_id) {
            error("node_id[0x%08X] not equals this_node_id[0x%08X]",
                          get_node_id(req->ali.agent_doc_hits[i]->node_id),
                          this_node_id);
            recv_cancel_cnt++; // �Ѱ��� ��ҽ�Ų��.
            continue; 
        }

        // �����ؾ��� comment �ʱ�ȭ
        req->ali.comments[i][0] = '\0';
	}

    req->ali.recv_cnt -= recv_cancel_cnt;

    /*
     * abstract_search�� doc_hits�� �ش��ϴ� comment���� ����� ����ϱ� ������
     * ���� page ����̳� ���� data �Ǽ����� ������� �ʴ´�.
     */
	req->send_cnt = req->ali.recv_cnt;
	req->send_first = 0;

	timelog("agent_abstractsearch_start");
	ret = agent_abstractsearch(r, req);
	if( ret != SUCCESS ) {
		error("agent_abstractsearch failed");
		return FAIL;
	}
	timelog("agent_abstractsearch_end");

	/////////// abstarct search ��� ���� ////////////////////////////////////////
    // binary�� �����Ѵ�.
	ap_set_content_type(r, "x-softbotd/binary");

	// 1. ���۰Ǽ�.
	ap_rwrite(&req->send_cnt, sizeof(uint32_t), r);
    info("send cnt[%d]", req->send_cnt);

	last = req->send_cnt + req->send_first;
	for (i=req->send_first; i<last; i++ ) {
		// 2.1 docid
	    ap_rwrite(&req->ali.agent_doc_hits[i]->doc_hits.id, sizeof(uint32_t), r);
		// 2.2 nodeid
	    ap_rwrite(&req->ali.agent_doc_hits[i]->node_id, sizeof(uint32_t), r);
		// 2.3 comment
		ap_rwrite(req->ali.comments[i], LONG_LONG_STRING_SIZE, r);
        debug("doc_id[%u], comments[%s]", req->ali.agent_doc_hits[i]->doc_hits.id, 
				req->ali.comments[i]);
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

//--------------------------------------------------------------//
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
