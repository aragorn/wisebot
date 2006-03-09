/* $Id$ */
#include <string.h>
#include "softbot.h"
#include "../mod_httpd/conf.h"
#include "../mod_httpd/protocol.h"
#include "mod_api/qp.h"
#include "mod_api/lexicon.h"
#include "mod_api/http_client.h"
#include "mod_qp/mod_qp.h"
#include "mod_httpd_softbot_handler.h"
#include "apr_strings.h"

#define MAX_SEARCH_UNIT 60
#define DEFAULT_PORT "8000"

typedef struct {
	char address[STRING_SIZE];
	char port[SHORT_STRING_SIZE];
	http_client_t *client;
} search_unit_t;

static search_unit_t sunit[MAX_SEARCH_UNIT];
static int num_of_sunit= 0;

static int def_atoi(const char *s, int def)
{
	if (s)
		return atoi(s);
	return def;
}

static char *replace_newline_to_space(char *str)
{
	char *ch;
	ch = str;
	while ( (ch = strchr(ch, '\n')) != NULL ) {
		*ch = ' ';
	}
	return str;
}
//---------------------------------------------------------//
/* make request_t->result_list from merged search result */
static 
void * make_result_list_from_merged(light_search_row * array, int array_size){
	int i;
	struct index_list_t * list = sb_malloc(sizeof(struct index_list_t));
	if(!list){
		error("sb_malloc failed : list");
		return NULL;
	}
	memset(list, 0, sizeof(struct index_list_t) );

	list->ndochits = array_size;
	list->doc_hits = sb_calloc(array_size, sizeof(doc_hit_t));
	if(!list->doc_hits){
		error("sb_calloc failed : list->doc_hits");
		sb_free(list);
		return NULL;
	}

	list->relevancy = sb_calloc(array_size, sizeof(uint32_t));
	if(!list->relevancy){
		error("sb_calloc failed : list->relevancy ");
		sb_free(list->doc_hits);
		sb_free(list);
		return NULL;
	}

	memset(list->doc_hits, 0, sizeof(doc_hit_t)*array_size);
	for( i=0; i<array_size; i++){
		list->relevancy[i] = array[i].relevance;
		list->doc_hits[i].id = array[i].docid;
	}
	
	return list;
}

/* compare function for search result mergesort */
static 
int _cmp_search_results(const void *a, const void *b){
	light_search_row *v1 = (light_search_row *)a;
	light_search_row *v2 = (light_search_row *)b;

	if ( v1->relevance < v2->relevance ) 
		return 1;
	else if ( v1->relevance > v2->relevance )
		return -1;
	if ( v1->docid > v2->docid )
		return 1;
	else if ( v1->docid < v2->docid )
		return -1;
	
	return 0;
}




static
int retrieve_sunit(void){
	int i, ret;
	fd_set rset, wset;
	int maxFd;
	struct timeval t;

	/* connect each search machine */
	for (i = 0; i < num_of_sunit; i++) {
		memfile *buf;
		int size;
		char *str;

		if ( sb_run_http_client_connect(sunit[i].client) == FAIL ) {
			error("error while http client connect to %s:%s",
					sunit[i].address, sunit[i].port);
		}
		ret = sb_run_http_client_makeRequest(sunit[i].client, NULL);
		if(ret != SUCCESS){
			error("sb_run_http_client_makeFullRequest");
			return -1;
		}

		buf = sunit[i].client->full_current_request;
		memfile_setOffset(buf, 0);
		size = memfile_getSize(buf);
		str = (char *) sb_malloc(size);
		
		if( size == memfile_read(buf, (char *)str, size) ){
			debug("request :\n%s", str);
		}
		sb_free(str);
		memfile_setOffset(buf, 0);
	}

	while(1) {
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		maxFd = -1;

		for (i = 0; i < num_of_sunit; i++) {
			/* if parsing complete, skip */
			if (sunit[i].client->parsing_status.state == 
					PARSING_COMPLETE) {
				continue;
			}

			if (sunit[i].client->statusFlag & CON_FLAG_CONNECTED) {
				if ( sunit[i].client->statusFlag & CON_FLAG_REQUEST_OK )
				{
					FD_SET(sunit[i].client->sockFd, &rset);
				} else {
					FD_SET(sunit[i].client->sockFd, &wset);
				}

				if (maxFd < sunit[i].client->sockFd) {
					maxFd = sunit[i].client->sockFd;
				}
			}

			if (sunit[i].client->statusFlag & CON_FLAG_CONNECTING) {
				FD_SET(sunit[i].client->sockFd, &wset);
				if (maxFd < sunit[i].client->sockFd) {
					maxFd = sunit[i].client->sockFd;
				}
			}
		}

		t.tv_sec = 1;
		t.tv_usec = 0;

		/* there is no socket to read or write */
		if ( maxFd == -1 ) {
			break;
		}

		if ( select(maxFd + 1, &rset, &wset, NULL, &t) < 0 ) {
			error("select : %s", strerror(errno));
			continue;
		}

		for (i = 0;i < num_of_sunit; i++) {
			/* set connected */
			if ( (sunit[i].client->statusFlag & CON_FLAG_CONNECTING) &&
				   	(FD_ISSET(sunit[i].client->sockFd, &wset)) ) {
				sunit[i].client->statusFlag = CON_FLAG_CONNECTED;
			}

			/* send request */
			if ( (sunit[i].client->statusFlag & CON_FLAG_CONNECTED) &&
					(FD_ISSET(sunit[i].client->sockFd, &wset)) ) {
				sb_run_http_client_sendRequest(sunit[i].client);
			}

			/* recv response */
			if ( (sunit[i].client->statusFlag & CON_FLAG_CONNECTED) &&
					(FD_ISSET(sunit[i].client->sockFd, &rset)) ) {
				ret = sb_run_http_client_recvResponse(sunit[i].client);
				if ( ret == SUCCESS ) {
					ret = sb_run_http_client_parseResponse(sunit[i].client);
					if ( (ret == SUCCESS) &&
							(sunit[i].client->parsing_status.state != PARSING_COMPLETE) ) {
						error("parsing error");
						break;
					}
				}
			}
		}
	}

	return 0;
}

//------------------------------------------------------//


/* generate xml search result 
 * retual value : FAIL - when result list is null
 * 				  SUCCESS - others
 */
static int make_xml_search_result(request_rec *r, request_t *req)
{
	int i;
	int searched_list_size=0,tmp=0;

	ap_set_content_type(r, "text/xml; charset=euc-kr");

	ap_rvputs(r,
			"<?xml version=\"1.0\" encoding=\"euc-kr\"?>",
			NULL);
	
	/* word_list char * req->word_list */
	ap_rvputs(r,
			"<xml>",
			"<search>\n",
			"<summary>",
			"<query>",
			req->query_string,
			"</query>",
			"<words>",
			req->word_list,
			"</words>",
			NULL);

	/* if req->result list is NULL, */
	if (req->result_list == NULL) {
		ap_rprintf(r,
				"<total>%d</total>"
				"<start>%d</start>"
				"<num>%d</num>"
				"</summary>\n"
				"</search>\n"
				"</xml>\n",
				0, req->first_result, 0);
		/* send total list count(0) to client */
		debug("result is NULL");

		return FAIL;
	}

	/* total list count int req->result_list->ndochits*/
	/* list count */
	if (req->result_list->ndochits < req->first_result) {
		searched_list_size = 0;
	}
	else {
		tmp = req->result_list->ndochits - req->first_result;

		if (tmp > req->list_size) {
			searched_list_size = req->list_size;
		}
		else {
			searched_list_size = tmp;
		}
	}

	ap_rprintf(r,
			"<total>%d</total>" 
			"<start>%d</start>"
			"<num>%d</num>"
			"</summary>\n",
			req->result_list->ndochits, req->first_result, searched_list_size);

	debug("searched_list_size[%d] req->result_list->ndochits [%d] req->first_result[%d]"
			,searched_list_size , req->result_list->ndochits , req->first_result);

	/* each result */
	for (i = 0; i < searched_list_size; i++) {

		if (req->type == FULL_SEARCH) {
			ap_rprintf(r, 
					"<row>"
					"<rowno>%d</rowno>"
					"<docid>%d</docid>"
					"<relevance>%d</relevance>"
					"<comment><![CDATA[%s]]></comment>"
					"</row>\n",
					req->first_result + i,
					req->result_list->doc_hits[i].id,
					req->result_list->relevancy[i],
					replace_newline_to_space(req->comments[i])
					);
		}
		else {
			ap_rprintf(r, 
					"<row>"
					"<rowno>%d</rowno>"
					"<docid>%d</docid>"
					"<relevance>%d</relevance>"
					"</row>\n",
					req->first_result + i,
					req->result_list->doc_hits[req->first_result+i].id,
					req->result_list->relevancy[req->first_result+i]
					);
		}
	}
	ap_rvputs(r,
			"</search>\n",
			"</xml>\n",
			NULL);

	return SUCCESS;
}

static int agent_light_search(request_t *req)
{
	int i,k;
	http_client_t *client = NULL;
	char path[LONG_STRING_SIZE];
	int ret;
	light_search_summary summary;
	light_search_row result_row[10000];
	int summary_size= sizeof(light_search_summary);
	int row_size = sizeof(light_search_row);
	int result_count=0;

	memset(path, 0, LONG_STRING_SIZE);

	/* make request line */
	snprintf(path, LONG_STRING_SIZE, "/default/light_search?p=%s&lc=%d",
			req->query_string, req->list_size);

	/* initialize http clients */
	for (i = 0; i < num_of_sunit; i++) {
		if (sunit[i].client) {
			http_prepareAnotherRequest(sunit[i].client->http);
			sunit[i].client->http->path = path;
			continue;
		}

		/* make new instance of http client */
		client = sb_run_http_client_new(sunit[i].address, sunit[i].port);
		if (!client) {
			error("error while http_client_new");
			return -1;
		}

		client->http->request_http_ver = 1001;
		client->http->method = "GET";
		client->http->host = sunit[i].address;
		client->http->path = path;
		client->http->accept = "application/x-softbot";
		client->timeout = 0;

		sunit[i].client = client;
	}
	
	if ( 0 != retrieve_sunit() ){
		error("retrieve_sunit failed ");
		return -1;
	}

	for (i = 0;i < num_of_sunit; i++) {
		memfile *buf = sunit[i].client->http->content_buf;
		memfile_setOffset(buf, 0);
		if( summary_size != memfile_read(buf, (char *)&summary, summary_size) ){
			error("incomplete result");
			continue;
		}
		debug("\n ========= %dth client result ============== ", i);
		debug(" summary_size [%d] row_size [%d] ", summary_size, row_size);
		debug(" buf size [%ld]  querystring[%s], tot[%d], numrw[%d]",
				memfile_getSize(buf), summary.query, summary.total_count,
					summary.num_of_rows);
		for (k = 0; k < summary.num_of_rows; k++){
			light_search_row *temp = &(result_row[result_count]);
			if( row_size != memfile_read(buf, (char *)temp , row_size) ){
				error("invalid result");
				continue;
			}
			temp->docid = temp->docid+ 1000000*(i+1);
			result_count++;
			if( result_count >= 10000 ) {
				error("too many results");
				break;
			}
			debug(" %dth row : docid[%d] relevance[%d] ", k, 
					temp->docid, temp->relevance );
		}
					
	}

	ret = mergesort(result_row, result_count+1, sizeof(light_search_row),
					_cmp_search_results);
	debug("\n mergesort return code [%d] ", ret);

	/* fill in response part of request_t */
	req->result_list = make_result_list_from_merged(result_row, result_count);

	return 0;
}

static int softbot_agent(request_rec *r, softbot_handler_rec *s)
{
	char *cmd;
	char *arg;
	const char *pos;
	request_t req;
	
	info("softbot_default");
	
	/* default handler for handling database operations */
	if (!s->name_space)
		return DECLINE;

	if (!s->remain_uri) {
		arg = NULL;
		cmd = "help";
	}
	else { 
		cmd = s->remain_uri;
		arg = strchr(cmd, '/');
		if (arg) {
			*arg = '\0';
			arg++;
		}
	}
		
	/* FIXME test code needs tobe fixed FIXME */
	if ( strncasecmp("search", cmd, STRING_SIZE) == 0 ) {

		/* Search */
		req.list_size = def_atoi(apr_table_get(s->parameters_in, "lc"), 10) * def_atoi(apr_table_get(s->parameters_in, "pg"), 1);
		req.first_result = 0;

		strncpy(req.query_string, apr_table_get(s->parameters_in, "p"), MAX_QUERY_STRING_SIZE);
		pos = apr_table_get(s->parameters_in, "at");
		if (!pos) {
			pos = "";
		}
		strncpy(req.attr_string, pos, MAX_ATTR_STRING_SIZE);

		pos = apr_table_get(s->parameters_in, "sh");
		if (!pos) {
			pos = "";
		}
		strncpy(req.sort_string, pos, MAX_SORT_STRING_SIZE);
	
		debug("req.query_string:[%s]",req.query_string);
		debug("req.attr_string:[%s]",req.attr_string);
		debug("req.sort_string:[%s]",req.sort_string);
		debug("req.list_size:%d",req.list_size);
		debug("req.first_result:%d",req.first_result);

		req.type = FULL_SEARCH;

		/* TODO: 분산 검색 */
		agent_light_search(&req);

		if (make_xml_search_result(r, &req) == FAIL) {
			return SUCCESS;
		}
	}
	else
	if ( strncasecmp("light_search", cmd, STRING_SIZE) == 0 ) {
		/* Light Search */
		req.list_size = def_atoi(apr_table_get(s->parameters_in, "lc"), 10) * def_atoi(apr_table_get(s->parameters_in, "pg"), 1);
		req.first_result = 0;

		strncpy(req.query_string, apr_table_get(s->parameters_in, "p"), MAX_QUERY_STRING_SIZE);

		pos = apr_table_get(s->parameters_in, "at");
		if (!pos) {
			pos = "";
		}

		strncpy(req.attr_string, pos, MAX_ATTR_STRING_SIZE);
		pos = apr_table_get(s->parameters_in, "sh");
		if (!pos) {
			pos = "";
		}

		strncpy(req.sort_string, pos, MAX_SORT_STRING_SIZE);
	
		debug("req.query_string:[%s]",req.query_string);
		debug("req.attr_string:[%s]",req.attr_string);
		debug("req.sort_string:[%s]",req.sort_string);
		debug("req.list_size:%d",req.list_size);
		debug("req.first_result:%d",req.first_result);

		req.type = LIGHT_SEARCH;

		/* TODO : distributed search */
/*		sb_run_qp_light_search(&req);*/

		if (make_xml_search_result(r, &req) == FAIL) {
			return SUCCESS;
		}
	}
	else
		return DECLINE;
	
	/* FIXME test code needs tobe fixed FIXME */

	return SUCCESS;
}


static void register_hooks(void)
{
	sb_hook_httpd_softbot_subhandler(softbot_agent,NULL,NULL,HOOK_MIDDLE);
	return;
}

static void set_search_unit(configValue v)
{
	char *ptr;

	memset(&sunit[num_of_sunit], 0, sizeof(search_unit_t));

	strncpy(sunit[num_of_sunit].address, v.argument[0], STRING_SIZE);

	ptr = strchr(sunit[num_of_sunit].address, ':');
	if (!ptr) {
		strncpy(sunit[num_of_sunit].port, DEFAULT_PORT, SHORT_STRING_SIZE);
	} else {
		strncpy(sunit[num_of_sunit].port, ptr+1, SHORT_STRING_SIZE);
		*ptr = '\0';
	}

	info("search machine[%d] added: ip[%s], port[%s]", num_of_sunit,
			sunit[num_of_sunit].address, sunit[num_of_sunit].port);

	num_of_sunit++;
}

static config_t config[] = {
	CONFIG_GET("AddSearchUnit", set_search_unit, 1,
			"add search machine : AddSearchUnit [ip:port]"),
	{NULL}
};

module httpd_softbot_agent_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	NULL,					/* registry */
	NULL,				/* initialize */
	NULL,				/* child_main */
	NULL,				/* scoreboard */
	register_hooks				/* register hook api */
};

