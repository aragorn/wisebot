#include "softbot.h"
#include "mod_api/sbhandler.h"
#include "mod_qp/mod_qp.h"
#include "mod_indexer/hit.h"
#include "mod_httpd/http_protocol.h"

extern void ap_set_content_type(request_rec * r, const char *ct);
//#include "mod_indexer/hit_new.h"
//#include "mod_api/qp.h"

//#include "mod_internet/internet.h"

#if 0
typedef struct _agent_abstract_t agent_abstract_t;
struct _agent_abstract_t {
	char title[STRING_SIZE];
	char comment[MAX_EACH_FIELD_COMMENT_SIZE]; /* XXX:COMMENT_SIZE? */
	char url[STRING_SIZE];
};

static void pull_out_comment_from_doc(search_abstract_t *search_abs, 
agent_abstract_t *agent_abs);
#endif
static int search_handler(request_rec *r, softbot_handler_rec *s);
static int light_search_handler(request_rec *r, softbot_handler_rec *s);
#if 0
static int abstract_search_handler(request_rec *r, softbot_handler_rec *s);
#endif

static softbot_handler_key_t search_handler_tbl[] = {
	{"light_search", light_search_handler},
	//{"abstract_search", abstract_search_handler},
	{"search", search_handler},
	{NULL, NULL}
};


/////////////////////////////////////////////////////////////////////////
static int def_atoi(const char *s, int def)
{
	if (s)
		return atoi(s);
	return def;
}

static char *replace_newline_to_space(char *str) {
    char *ch;       
    ch = str;       
    while ( (ch = strchr(ch, '\n')) != NULL ) {
        *ch = ' ';
    }   
    return str;
} 

static int get_send_count(request_t* req)
{
    int send_cnt = 0;

    if (req->result_list->ndochits < req->first_result) {
        send_cnt = 0;
    }
    else {
        int tmp = req->result_list->ndochits - req->first_result;

        if (tmp > req->list_size) {
            send_cnt = req->list_size;
        }
        else {
            send_cnt = tmp;
        }

        if ( send_cnt > COMMENT_LIST_SIZE ) {
            warn("searched_list_size[%d] > COMMENT_LIST_SIZE[%d]",
                    send_cnt, COMMENT_LIST_SIZE);
            send_cnt = COMMENT_LIST_SIZE;
        }
    }

    return send_cnt;
}

static int search_handler(request_rec *r, softbot_handler_rec *s)
{
    char* p = NULL;
    request_t req; 
    int rv = 0, i = 0;
    int search_total_cnt = 0; // 총 검색결과수
    int search_send_cnt = 0;  // 전송하는 검색결과 수
	char query[MAX_QUERY_STRING_SIZE];

	//1. set request_t
    /* query */
	p = (char *)apr_table_get(s->parameters_in, "q");
	if (p == NULL) {
		error("no query string");
		return FAIL;
	}
	strncpy(req.query_string, p, MAX_QUERY_STRING_SIZE-1);

    /* AT */
	p = (char *)apr_table_get(s->parameters_in, "at");
	if (p != NULL) {
	    strncpy(req.attr_string, p, MAX_ATTR_STRING_SIZE-1);
	}

    /* AT2 */
	p = (char *)apr_table_get(s->parameters_in, "at2");
	if (p != NULL) {
	    strncpy(req.attr2_string, p, MAX_ATTR_STRING_SIZE-1);
	}

    /* GR */
	p = (char *)apr_table_get(s->parameters_in, "gr");
	if (p != NULL) {
	    strncpy(req.group_string, p, MAX_GROUP_STRING_SIZE-1);
	}

    /* SH */
	p = (char *)apr_table_get(s->parameters_in, "sh");
	if (p != NULL) {
	    strncpy(req.sort_string, p, MAX_SORT_STRING_SIZE-1);
	}

    /* LC */
	req.list_size = def_atoi(apr_table_get(s->parameters_in, "lc"), 20);
	if(req.list_size < 0) req.list_size = 20;

    /* PG */
    req.first_result = def_atoi(apr_table_get(s->parameters_in, "pg"), 0);
	if(req.first_result < 0) req.first_result = 0;

	req.first_result *= req.list_size;

    /* FT */
    req.filtering_id = def_atoi(apr_table_get(s->parameters_in, "ft"), 0);

    req.type = FULL_SEARCH;

    /* reqeust debug string */
    debug("req.query_string:[%s]",req.query_string);
    debug("req.attr_string:[%s]",req.attr_string);
    debug("req.attr2_string:[%s]",req.attr2_string);
    debug("req.group_string:[%s]",req.group_string);
    debug("req.sort_string:[%s]",req.sort_string);
    debug("req.list_size:%d",req.list_size);
    debug("req.first_result:%d",req.first_result);
    debug("req.filtering_id:%d", req.filtering_id);

	sprintf(query, "q=%s&at=%s&at2=%s&gr=%s&sh=%s&lc=%d&pg=%d&ft=%d",
			       req.query_string, req.attr_string,
				   req.attr2_string, req.group_string,
				   req.sort_string, req.list_size, 
				   req.first_result, req.filtering_id);

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
	for (i = 0; i < search_send_cnt; i++) {
		ap_rprintf(r, 
				"<row>"
				"<rowno>%d</rowno>"
				"<did>%d</did>"
				"<relevance>%d</relevance>"
				"<wordhist>%d</wordhist>"
				"<comment><![CDATA[%s]]></comment>"
				"</row>\n",
				req.first_result + i,
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
    char* p = NULL;
    request_t req; 
    int rv = 0, i = 0;
    int search_total_cnt = 0; // 총 검색결과수
    int search_send_cnt = 0;  // 전송하는 검색결과 수
	char query[MAX_QUERY_STRING_SIZE];
	//agent_depth_t agent_depth;

	//1. set request_t
    /* P : query */
	p = (char *)apr_table_get(s->parameters_in, "q");
	if (p == NULL) {
		error("no query string");
		return FAIL;
	}
	strncpy(req.query_string, p, MAX_QUERY_STRING_SIZE-1);

    /* AT */
	p = (char *)apr_table_get(s->parameters_in, "at");
	if (p != NULL) {
	    strncpy(req.attr_string, p, MAX_ATTR_STRING_SIZE-1);
	}

    /* AT2 */
	p = (char *)apr_table_get(s->parameters_in, "at2");
	if (p != NULL) {
	    strncpy(req.attr2_string, p, MAX_ATTR_STRING_SIZE-1);
	}

    /* GR */
	p = (char *)apr_table_get(s->parameters_in, "gr");
	if (p != NULL) {
	    strncpy(req.group_string, p, MAX_GROUP_STRING_SIZE-1);
	}

    /* SH */
	p = (char *)apr_table_get(s->parameters_in, "sh");
	if (p != NULL) {
	    strncpy(req.sort_string, p, MAX_SORT_STRING_SIZE-1);
	}

    /* LC */
	req.list_size = def_atoi(apr_table_get(s->parameters_in, "lc"), 20);

    /* PG */
    req.first_result = def_atoi(apr_table_get(s->parameters_in, "pg"), 0) * req.list_size;

    /* FT */
    req.filtering_id = def_atoi(apr_table_get(s->parameters_in, "ft"), 0);

    req.type = LIGHT_SEARCH;

    /* reqeust debug string */
    debug("req.query_string:[%s]",req.query_string);
    debug("req.attr_string:[%s]",req.attr_string);
    debug("req.attr2_string:[%s]",req.attr2_string);
    debug("req.group_string:[%s]",req.group_string);
    debug("req.sort_string:[%s]",req.sort_string);
    debug("req.list_size:%d",req.list_size);
    debug("req.first_result:%d",req.first_result);
    debug("req.filtering_id:%d", req.filtering_id);
	sprintf(query, "q=%s&at=%s&at2=%s&gr=%s&sh=%s&lc=%d&pg=%d&ft=%d",
			       req.query_string, req.attr_string,
				   req.attr2_string, req.group_string,
				   req.sort_string, req.list_size, 
				   req.first_result, req.filtering_id);

    req.sb4error = 0;
    req.result_list = NULL; 

	//2. get result
	timelog("light_search_start");
	rv = sb_run_qp_light_search(get_word_db(), &req);
	timelog("light_search_finish");
	if (rv != SUCCESS) {
		error("sb_run_qp_light_search failed: query[%s]", query);
		return FAIL;
	}

	//we've got no search result
	if (req.result_list == NULL) {
        search_total_cnt = 0;
        search_send_cnt = 0;

		error("no result: query[%s]", query);
	} else {
        search_total_cnt = req.result_list->ndochits;
        search_send_cnt = get_send_count(&req);
	}

	timelog("send_result_start");
	ap_set_content_type(r, "x-softbotd/binary");

	//3-1 search_words
	ap_rwrite(req.word_list, sizeof(char)*STRING_SIZE, r);
	//3-2 total_cnt 
	ap_rwrite(&(search_total_cnt), sizeof(uint32_t), r);
	//3-3 cnt 
	ap_rwrite(&(search_send_cnt), sizeof(uint32_t), r);
#if 0
	//3-4 relevency list
	ap_rwrite(res.relevancy, sizeof(uint32_t)*res.cnt, r);
#endif
	//3-5 agent_depth list
	//for(i=0; i<res.cnt; i++) {
	//	memset(&agent_depth, AGENT_DEPTH_NULL_TARGET, sizeof(agent_depth_t));
	//	ap_rwrite(&agent_depth, sizeof(agent_depth_t), r);
	//}
#if 0
	//3-6 dochit list
	ap_rwrite(res.doc_hits, DOCHIT_SIZE*res.cnt, r);
	timelog("send_result_finish");
#endif
    // result list
	for (i = req.first_result; i < search_send_cnt; i++) {
        index_list_t *result;
        uint32_t word_id;
        uint32_t relevancy;
        uint16_t length;

        result = &req.result_list[i];
        word_id = result->wordid;
	    ap_rwrite(&word_id, sizeof(uint32_t), r);
        relevancy = result->relevancy;
	    ap_rwrite((void*)&relevancy, sizeof(uint32_t), r);
        length = result->list_size;
	    ap_rwrite(&length, sizeof(uint16_t), r);
        ap_rwrite(result->doc_hits, sizeof(doc_hit_t) * length, r);
        info("result %d. docid [%d] relevancy [%d] length [%d]",
                i,
                word_id,
                relevancy,
                length);
    }
    timelog("send_result_finish");

	sb_run_qp_finalize_search(&req);
	timelog("qp_finalize");
	return SUCCESS;
}

#if 0
static int abstract_search_handler(request_rec *r, softbot_handler_rec *s)
{
	memfile *req_body;
	int cnt, i;
	index_list_t *list;

	CHECK_REQUEST_CONTENT_TYPE(r, "x-softbotd/binary");

	if (sb_run_sbhandler_make_memfile(r, &req_body) != SUCCESS) {
		MSG_RECORD(&(s->msgs), error, "make_memfile_from_postdata failed");
		return FAIL;
	}

	cnt = memfile_getSize(req_body) / (sizeof(agent_depth_t) + sizeof(hit_t));

	list = (index_list_t *)apr_palloc(r->pool, cnt*sizeof(index_list_t));
	memfile_setOffset(req_body, cnt*sizeof(agent_depth_t));
    for(i = 0; i < cnt; i++)
    {
        memfile_read(req_body, (void *)&(list[i].word_id), sizeof(list[i].word_id));
        memfile_read(req_body, (void *)&(list[i].relevancy), sizeof(list[i].relevancy));
        memfile_read(req_body, (void *)&(list[i].list_size), sizeof(list[i].list_size));
        list[i].doc_hits = apr_palloc(r->pool, sizeof(doc_hit_t) * list[i].length);
        memfile_read(req_body, (void *)list[i].start, sizeof(hit_base_t) * list[i].length);
    }
	memfile_free(req_body);

	search_abs_list = apr_palloc(r->pool, cnt*sizeof(search_abstract_t));
	if (!search_abs_list) {
		MSG_RECORD(&(s->msgs), error, "search_abs_list apr_palloc failed");
		return FAIL;
	}

	if (sb_run_qp_abstract_info(cnt, list, search_abs_list) != SUCCESS) {
		MSG_RECORD(&(s->msgs), error, "sb_run_qp_get_abstract_list failed");
		return FAIL;
	}

	agent_abs_list = apr_palloc(r->pool, cnt*sizeof(agent_abstract_t));
	if (!agent_abs_list) {
		MSG_RECORD(&(s->msgs), error, "agent_abs_list apr_palloc failed");
		for(i = 0;i < cnt; i++) {
			sb_run_free_document_object(search_abs_list[i].doc);
		}
		return FAIL;
	}

	for(i = 0;i < cnt; i++) {
		pull_out_comment_from_doc(&(search_abs_list[i]), &(agent_abs_list[i]));
		sb_run_free_document_object(search_abs_list[i].doc);
	}

	ap_set_content_type(r, "x-softbotd/binary");
	ap_rwrite(agent_abs_list, cnt*sizeof(agent_abstract_t), r);

	return SUCCESS;
}

static void pull_out_comment_from_doc(search_abstract_t *search_abs,
agent_abstract_t *agent_abs) {
	int i, rv, len;
	char *val;
	char *ptr;
	
	memset(agent_abs, 0, sizeof(agent_abstract_t));

	for (i=0; i<search_abs->info.abstract_field_info_num; i++){
		if (!search_abs->doc) continue;
		rv = sb_run_document_object_search_field_i
			(search_abs->doc, search_abs->info.info[i].field_id, (void**)&val, &len);
		if (rv == FAIL || len == 0) {
			warn("cannot get field[%d]:rv[%d]len[%d]", 
					search_abs->info.info[i].field_id, rv, len);
			continue;
		}
		
		DEBUG("field_id: %d", search_abs->info.info[i].field_id);
		DEBUG("val: %s", val);
		//url
		if (search_abs->info.info[i].field_id == 0) {
			strncpy(agent_abs->url, val, STRING_SIZE);
			agent_abs->url[STRING_SIZE-1] = '\0';
		
		//comment
		} else if (search_abs->info.info[i].field_id == 1) {
			strncpy(agent_abs->comment, val, MAX_EACH_FIELD_COMMENT_SIZE);
			agent_abs->comment[MAX_EACH_FIELD_COMMENT_SIZE-1] = '\0';
		//title
		} else if (search_abs->info.info[i].field_id == 2) {
			strncpy(agent_abs->title, val, STRING_SIZE);
			agent_abs->title[STRING_SIZE-1] = '\0';
		}
	}

	/* remove \n: replace with space */
	while ((ptr = strchr(agent_abs->comment, '\n')) != NULL) *ptr = ' ';

	return;
}
#endif
/////////////////////////////////////////////////////////////////////////

static int get_table(char *name_space, void **tab)
{
	if ( strcmp(name_space, "search") != 0 ){
		return DECLINE;
	}

	*tab = search_handler_tbl;

	return SUCCESS;
}

static void register_hooks(void)
{
	sb_hook_sbhandler_get_table(get_table,NULL,NULL,HOOK_MIDDLE);
	return;
}

module search_handler_module = {
	STANDARD_MODULE_STUFF,
	NULL,                   /* config */
	NULL,                   /* registry */
	NULL,               /* initialize */
	NULL,               /* child_main */
	NULL,               /* scoreboard */
	register_hooks              /* register hook api */
};
