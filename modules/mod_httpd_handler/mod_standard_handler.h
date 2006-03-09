/* $Id$ */
#ifndef _MOD_HTTPD_SOFTBOT_HANDLER_H_
#define _MOD_HTTPD_SOFTBOT_HANDLER_H_

#include "hook.h"
#include "softbot.h"
#include "../mod_httpd/mod_httpd.h"

typedef struct softbot_handler_rec softbot_handler_rec;
typedef struct light_search_summary light_search_summary;
typedef struct light_search_row light_search_row;

struct softbot_handler_rec {
	char *name_space;
	char *remain_uri;
	apr_table_t	*parameters_in;
};

struct light_search_summary { 
	char query[MAX_QUERY_STRING_SIZE]; // 검색 질의어
	int total_count; // 총 검색결과 건수 
	int num_of_rows; // 현재 검색된 건수 
};

struct light_search_row { 
	int docid; 
	int relevance; 
};
 
SB_DECLARE_HOOK(int,httpd_softbot_subhandler,(request_rec *r, softbot_handler_rec *s, word_db_t *w))

#endif
