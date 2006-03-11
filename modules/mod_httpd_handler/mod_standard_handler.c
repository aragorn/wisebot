/* $Id$ */
#include <string.h>
#include <ctype.h>
#include "mod_api/did.h"
#include "mod_cdm/mod_cdm.h"
#include "../mod_httpd/conf.h"
#include "../mod_httpd/protocol.h"
#include "mod_mp/mod_mp.h"
#include "mod_api/qp.h"
#include "mod_api/lexicon.h"
#include "mod_qp/mod_qp.h"
#include "mod_standard_handler.h"
#include "apr_strings.h"

HOOK_STRUCT(
	HOOK_LINK(httpd_softbot_subhandler)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, httpd_softbot_subhandler,
						(request_rec *r, softbot_handler_rec *s), (r, s), DECLINE)

typedef struct _sbhandler_postdata sbhandler_postdata;

struct _sbhandler_postdata {
	int size;
	int pos;
	void *data;
	sbhandler_postdata *prev;
	sbhandler_postdata *next;
};

static int did_set = -1;
static did_db_t* did_db = NULL;
static int word_db_set = -1;
static word_db_t* word_db = NULL;

#define INIT_POST_DATA(a,b)	\
{ \
	(a) = apr_palloc(r->pool, sizeof(sbhandler_postdata)); \
	(a)->prev = NULL; \
	(a)->next = NULL; \
	(a)->size = (b); \
	(a)->pos = 0; \
	(a)->data = apr_pcalloc(r->pool, (b) + 1); \
}

#define NEXT_POST_DATA(a,b)	\
{ \
	(a)->next = apr_palloc(r->pool, sizeof(sbhandler_postdata)); \
	(a)->next->prev = (a); \
	(a)->next->next = NULL; \
	(a)->next->size = (b); \
	(a)->next->pos = 0; \
	(a)->next->data = apr_pcalloc(r->pool, (b) + 1); \
	(a) = (a)->next; \
}

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

#define POST_DATA_SIZE	(4096)

/*****************************************************************************/
static int softbot_handler(request_rec *r)
{
	softbot_handler_rec s;
	apr_bucket_brigade *bb = NULL;
/*	apr_bucket *b = NULL;*/
	int seen_eos;
	int rv;
	sbhandler_postdata *post_data_root = NULL;
	sbhandler_postdata *post_data = NULL;
	const char *content_type = NULL;
	char *pos;
	
	/* init data structures */
	s.name_space = NULL;
	s.remain_uri = NULL;
	s.parameters_in = apr_table_make(r->pool, 4);
	INIT_POST_DATA(post_data, POST_DATA_SIZE);
	post_data_root = post_data;

	/* parse uri */
	if (r->parsed_uri.path) {
		char *uri = apr_pstrdup(r->pool, r->parsed_uri.path);

		pos = strchr(uri, '/');
		if (!pos)
			return DECLINE;
		*pos = '\0';
		s.name_space = pos + 1;
		pos = strchr(s.name_space, '/');
		if (!pos)
			return DECLINE;
		*pos = '\0';
		s.remain_uri = pos + 1;
	}
	else
		return DECLINE;

/* XXX DEBUG */
	info("r->handler = '%s'\n", r->handler);
/*	r->server->timeout = 1; |+ hack +|*/
/*	ap_rvputs(r,*/
/*		  DOCTYPE_HTML_2_0*/
/*		  "<html>"*/
/*		    "<head>\n"*/
/*		      "<title>",*/
/*		        "test",*/
/*		      "</title>\n",*/
/*			  "<body>\n",*/
/*			  NULL);*/
/* XXX until here */

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
/* XXX DEBUG */
/*				ap_rvputs(r,*/
/*						"URI param : ",*/
/*						key,*/
/*						" = ",*/
/*						val,*/
/*						"<BR>\n",*/
/*						NULL*/
/*						);*/
/* XXX until here */
			}
			key = apr_strtok(NULL, "&", &strtok_buf);
		}
	}
	
	/* read POST data */
	bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);
	seen_eos = 0;
	do {
		apr_bucket *bucket;
		
		rv = ap_get_brigade(r->input_filters, bb, AP_MODE_READBYTES,
							APR_BLOCK_READ, HUGE_STRING_LEN);

		if (rv != APR_SUCCESS) {
			return rv;
		}

		APR_BRIGADE_FOREACH(bucket, bb) {
			const char *data;
			apr_size_t len;
			apr_size_t copy_len;

			if (APR_BUCKET_IS_EOS(bucket)) {
				seen_eos = 1;
				break;
			}

			/* We can't do much with this. */
			if (APR_BUCKET_IS_FLUSH(bucket)) {
				continue;
			}

			/* read */
			apr_bucket_read(bucket, &data, &len, APR_BLOCK_READ);

			/* Keep writing data to the child until done or too much time
			 *              * elapses with no progress or an error occurs.
			 *                           */
			while(len)
			{
				if (len < post_data->size - post_data->pos) {
					memcpy(post_data->data + post_data->pos, data, len);
					post_data->pos += len;
					len = 0;
				}
				else {
					info( "%8s", data );
					copy_len = post_data->size - post_data->pos;
					memcpy(post_data->data + post_data->pos, data, copy_len);
					post_data->pos += copy_len;
					len -= copy_len;
					data += copy_len;
					NEXT_POST_DATA(post_data, POST_DATA_SIZE);
				}
			}
		}
		apr_brigade_cleanup(bb);
	}
	while (!seen_eos);

	content_type = apr_table_get(r->headers_in, "Content-Type");

	if (content_type) {
		if (!strcasecmp(content_type, "multipart/form-data")) {
			/* TODO parse multipart form-data */
		}
		else {
			char *key, *val;
			int key_size, val_size;
			char *ptr;
			int offset;
			char *start_pos;
			sbhandler_postdata *start_post_data;
			sbhandler_postdata *post_data;

			start_post_data = post_data = post_data_root;
			start_pos = ptr = post_data->data;
			while(1) {
				key_size = 0;

				/* 1. try to find '=' */
				while( *ptr != '=') {

					/* go to next post_data if next is not null*/
					if (*ptr == '\0') {
						post_data = post_data->next;
						if (post_data == NULL) {
							break;
						}
						ptr = post_data->data;
						continue;
					}

					/* if we found '&' before '=' then skip data */
					if (*ptr == '&') {
						break;
					}
					key_size++;
					ptr++;
				}

				/* if we found '&' before '=' then skip data */
				if (*ptr == '&')  {
					ptr++;
					continue;
				}

				/* 2. alloc & copy key */
				key = apr_pcalloc(r->pool, key_size + 1);

				offset = 0;
				while(key_size > 0) {
					int cur_block_len = POST_DATA_SIZE - (start_pos - (char *)start_post_data->data);
					if (key_size < cur_block_len) {
						decodencpy(key + offset, start_pos, key_size);
						key[offset+key_size] = '\0';
					}
					else {
						decodencpy(key + offset, start_pos, cur_block_len); 
						start_post_data = start_post_data->next;
						start_pos = start_post_data->data;
					}
					offset += cur_block_len;
					key_size -= cur_block_len;
				}

				val_size = 0;
				start_post_data = post_data;
				start_pos = ++ptr;

				/* 3. try to find '&' */
				while( *ptr != '&') {

					/* go to next post_data if next is not null*/
					if (*ptr == '\0') {
						post_data = post_data->next;
						if (post_data == NULL) {
							break;
						}
						ptr = post_data->data;
						continue;
					}

					val_size++;
					ptr++;
				}

				/* 4. alloc & copy value */
				val = apr_pcalloc(r->pool, val_size + 1);

				offset = 0;
				while(val_size > 0) {
					int cur_block_len = POST_DATA_SIZE - (start_pos - (char *)start_post_data->data);
					if (val_size < cur_block_len) {
						decodencpy(val + offset, start_pos, val_size);
						val[offset+val_size] = '\0';
					}
					else {
						decodencpy(val + offset, start_pos, cur_block_len); 
						start_post_data = start_post_data->next;
						start_pos = start_post_data->data;
					}
					offset += cur_block_len;
					val_size -= cur_block_len;
				}

				apr_table_setn(s.parameters_in, key, val);
/* XXX DEBUG */
/*				ap_rvputs(r,*/
/*						key,*/
/*						" = ",*/
/*						val,*/
/*						"<BR>\n",*/
/*						NULL*/
/*						);*/
/* XXX until here */

				if (post_data == NULL)
					break;

				start_post_data = post_data;
				start_pos = ++ptr;
			}
		}
	}
	
	rv = sb_run_httpd_softbot_subhandler(r, &s);
	
/* XXX DEBUG */
/*	ap_rvputs(r,*/
/*		    "</body>"*/
/*		  "</html>\n",*/
/*		  NULL);*/
/* XXX until here */

	return SUCCESS;
}

/*****************************************************************************/
static int softbot_status(request_rec *r, softbot_handler_rec *s)
{
	char buf[1024];
	int filedes[2];
	FILE *fp[2];
	char *cmd;
	const char *argp;
	char *arg;
	
	if (!s->name_space)
		return DECLINE;
	
	if (strncasecmp(s->name_space, "status", STRING_SIZE))
		return DECLINE;

	if (pipe(filedes))
		return DECLINE;
	
	fp[0] = fdopen(filedes[0], "rb");
	if (!fp[0]) {
		close(filedes[0]);
		close(filedes[1]);
		return DECLINE;
	}

	fp[1] = fdopen(filedes[1], "wb");
	if (!fp[1]) {
		fclose(fp[0]);
		close(filedes[0]);
		close(filedes[1]);
		return DECLINE;
	}

	if (!s->remain_uri)
		cmd = "help";
	else 
		cmd = s->remain_uri;
	
	ap_rvputs(r,
			"<xml>\n",
			"<status>",
			"<command>",
			cmd,
			"</command>",
			"<![CDATA[",
			NULL);

	if ( strncasecmp("modules", cmd, STRING_SIZE) == 0 ) {
		list_modules(fp[1]);
	} else if ( strncasecmp("static_modules", cmd, STRING_SIZE) == 0 ) {
		list_static_modules(fp[1]);
	} else if ( strncasecmp("config", cmd, STRING_SIZE) == 0 ) {
		argp = apr_table_get(s->parameters_in, "id");
		if (argp) {
			arg = apr_pstrdup(r->pool, argp);
			list_config(fp[1], arg);
		}
		else {
			ap_rvputs(r,
				"Showing all configuration is not supported\n"
				"Instead, use config?id=module_name\n\n",
				NULL);
			list_modules(fp[1]);
		}
	} else if ( strncasecmp("registry", cmd, STRING_SIZE) == 0 ) {
		argp = apr_table_get(s->parameters_in, "id");
		if (argp) {
			arg = apr_pstrdup(r->pool, argp);
			list_registry(fp[1], arg);
		}
		else
			list_registry(fp[1], NULL);
	} else if ( strncasecmp("save_registry", cmd, STRING_SIZE) == 0 ) {
		argp = apr_table_get(s->parameters_in, "file");
		arg = apr_pstrdup(r->pool, argp);
		save_registry(stdout, arg);
	} else if ( strncasecmp("restore_registry", cmd, STRING_SIZE) == 0 ) {
		argp = apr_table_get(s->parameters_in, "file");
		arg = apr_pstrdup(r->pool, argp);
		restore_registry_file(arg);
	} else if ( strncasecmp("scoreboard", cmd, STRING_SIZE) == 0 ) {
		argp = apr_table_get(s->parameters_in, "id");
		arg = apr_pstrdup(r->pool, argp);
		list_scoreboard(fp[1], arg);
	} else {
		if ( strncasecmp("help", buf, STRING_SIZE) != 0 )
			fprintf(fp[1], "unknown option, [%s]\n", cmd);
		fprintf(fp[1], "usage: status <option>\n");
		fprintf(fp[1], " static_modules : show static modules\n");
		fprintf(fp[1], " modules    : show loaded modules\n");
		fprintf(fp[1], " config [module] : show config table of the module\n");
		fprintf(fp[1], " registry [module] : show registry table of the module\n");
		fprintf(fp[1], " scoreboard [module] : show scoreboard of the module\n");
		fprintf(fp[1], " save_registry [module] : save registry table of the module\n");
		fprintf(fp[1], " restore_registry [file] : restore registry from the file\n");
		fprintf(fp[1], " help       : show this message\n");
	}

	fclose(fp[1]);
	while(!feof(fp[0]))
	{
		int len;

		len = fread(buf, 1, 1024, fp[0]);
		ap_rwrite(buf, len, r);
	}

	ap_rvputs(r,
			"]]></status>\n",
			"</xml>\n",
			NULL);

	fclose(fp[0]);
	close(filedes[0]);
	close(filedes[1]);

	return SUCCESS;
}

/*****************************************************************************/
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

/* generate xml search result 
 * retual value : FAIL - when result list is null
 * 				  SUCCESS - others
 */
static int make_xml_search_result(request_rec *r, request_t *req)
{
	int i;
	int searched_list_size=0,tmp=0;
	request_rec *rq;

	rq = r;
	ap_set_content_type(rq, "text/xml; charset=euc-kr");

	ap_rvputs(rq,
			"<?xml version=\"1.0\" encoding=\"euc-kr\"?>",
			NULL);

	/* word_list char * req->word_list */
	ap_rvputs(rq,
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
		ap_rprintf(rq,
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

	ap_rprintf(rq,
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
			ap_rprintf(rq, 
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
			ap_rprintf(rq, 
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

static int make_light_search_result(request_rec *r, request_t *req)
{
	int i;
	int searched_list_size=0,tmp=0;
	light_search_summary summary;
	light_search_row row;

	ap_set_content_type(r, "application/x-softbot");

	memset(&summary, 0, sizeof(light_search_summary));
	strncpy(summary.query, req->query_string, MAX_QUERY_STRING_SIZE);

	if (req->result_list == NULL) {
		summary.total_count = 0;
		summary.num_of_rows = 0;

		ap_rwrite(&summary, sizeof(summary), r);

		return FAIL;
	}

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

	summary.total_count = req->result_list->ndochits;
	summary.num_of_rows = searched_list_size;

	ap_rwrite(&summary, sizeof(summary), r);

	for(i = 0; i < searched_list_size; i++) {
		row.docid = req->result_list->doc_hits[req->first_result+i].id;
		row.relevance = req->result_list->relevancy[req->first_result+i];
		ap_rwrite(&row, sizeof(row), r);
	}

	return SUCCESS;
}

static int softbot_default(request_rec *r, softbot_handler_rec *s)
{
	char *cmd;
	char *arg;
	const char *pos;
	const char *accept;
	request_t req;
	
	info("softbot_default");
	
	/* default handler for handling database operations */
	if (!s->name_space) {
		return DECLINE;
	}

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

	/* start search */
	if ( strncasecmp("search", cmd, STRING_SIZE) == 0 ) {
		/* full search */
		req.list_size = def_atoi(apr_table_get(s->parameters_in, "lc"), 10);
		req.first_result = def_atoi(apr_table_get(s->parameters_in, "pg"), 0) * req.list_size;
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

		sb_run_qp_full_search(word_db, &req);

		if (make_xml_search_result(r, &req) == FAIL) {
		    sb_run_qp_finalize_search(&req);
			return SUCCESS;
		}

		sb_run_qp_finalize_search(&req);
	}
	else if ( strncasecmp("light_search", cmd, STRING_SIZE) == 0 ) {
		/* light search */
		req.list_size = def_atoi(apr_table_get(s->parameters_in, "lc"), 10);
		req.first_result = def_atoi(apr_table_get(s->parameters_in, "pg"), 0) * req.list_size;
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

		sb_run_qp_light_search(word_db, &req);

		accept = apr_table_get(r->headers_in, "Accept");
		debug("accept : %s", accept);
		if ( accept && !strcasecmp(accept, "application/x-softbot") ) {
			if (make_light_search_result(r, &req) == FAIL) {
		        sb_run_qp_finalize_search(&req);
				return SUCCESS;
			}
		} else {
			if (make_xml_search_result(r, &req) == FAIL) {
		        sb_run_qp_finalize_search(&req);
				return SUCCESS;
			}
		}

		sb_run_qp_finalize_search(&req);
	}
	else {
		return DECLINE;
	}
	
	/* FIXME test code needs tobe fixed FIXME */

	return SUCCESS;
}

static int softbot_lexicon(request_rec *r, softbot_handler_rec *s)
{
	char *cmd;
	char *arg;

	if ( strncasecmp("lexicon", s->name_space, STRING_SIZE) ) {
		return DECLINE;
	}

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
	if ( strncasecmp("word", cmd, STRING_SIZE) == 0 ) {
		int ret;
		word_t lexicon;

		strncpy(lexicon.string, arg , MAX_WORD_LEN);
		ret = sb_run_get_word(word_db, &lexicon);

		debug("word:[%s]",arg);
		ap_rvputs(r, "<xml>\n" "<result>", NULL);

		if (ret < 0) {
			error("ret[%d] : no such word[%s]",ret ,arg);
			ap_rvputs(r,
					"<retcode>no such word</retcode>",
					"</result>\n",
					"</xml>\n",
					NULL
					);
			return SUCCESS;
		}

		ap_rprintf(r,
				"<retcode>%d</retcode>\n"
				"<word>%s"
				"</word>\n"
				//"<df>%d</df>\n"
				"<id>%d</id>\n"
				"</result>\n"
				"</xml>\n",
				ret,
				lexicon.string,
				//lexicon.word_attr.df,
				lexicon.id
				);
	}
	else {
		return DECLINE;
	}

	return SUCCESS;
}

static int standard_handler_init(void)
{
    int ret = 0;

    ret = sb_run_open_did_db( &did_db, did_set );
    if ( ret != SUCCESS && ret != DECLINE ) {
        error("did db open failed");
        return FAIL;
    }

    if ( sb_run_open_word_db( &word_db, word_db_set ) != SUCCESS ) {
        error("lexicon open failed");
        return FAIL;
    }

	sb_run_qp_init();

    ret = sb_run_server_canneddoc_init();
    if ( ret != SUCCESS && ret != DECLINE ) {
        error( "cdm module init failed" );
        return FAIL;
    }

	return SUCCESS;
}

/* ###########################################################################
 * frame & configuration stuff here
 * ########################################################################## */

static void set_did_set(configValue v)
{
    did_set = atoi( v.argument[0] );
}

static void set_word_db_set(configValue v)
{
    word_db_set = atoi( v.argument[0] );
}

static config_t config[] = {
    CONFIG_GET("DidSet", set_did_set, 1, "Did Set 0~..."),
    CONFIG_GET("WordDbSet", set_word_db_set, 1, "WordDbSet {number}"),
    {NULL}
};

static void register_hooks(void)
{
	sb_hook_handler(softbot_handler,NULL,NULL,HOOK_REALLY_FIRST);
	sb_hook_handler_init(standard_handler_init,NULL,NULL,HOOK_MIDDLE);
	sb_hook_httpd_softbot_subhandler(softbot_status,NULL,NULL,HOOK_MIDDLE);
	sb_hook_httpd_softbot_subhandler(softbot_lexicon,NULL,NULL,HOOK_MIDDLE);
	sb_hook_httpd_softbot_subhandler(softbot_default,NULL,NULL,HOOK_REALLY_LAST);
	return;
}

module standard_handler_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	NULL,					/* registry */
	standard_handler_init,  /* initialize */
	NULL,				    /* child_main */
	NULL,				    /* scoreboard */
	register_hooks		    /* register hook api */
};

