/* $Id$ */
#ifndef QP_H
#define QP_H 1

typedef struct request_t request_t;
typedef struct filter_t filter_t;
typedef struct rank_t rank_t;
typedef struct index_list_t index_list_t;
typedef struct group_result_t group_result_t;

#include "docattr.h"
#include "indexer.h"

#define MAX_QUERY_STRING_SIZE	LONG_STRING_SIZE
#define MAX_ATTR_STRING_SIZE	LONG_STRING_SIZE
#define MAX_GROUP_STRING_SIZE   STRING_SIZE
#define MAX_SORT_STRING_SIZE	STRING_SIZE

/* MAX_INDEX_LIST_POOL: should be larger than max nodes of qpp */
//#define MAX_INDEX_LIST_POOL	(12)
//#define MAX_INDEX_LIST_POOL	(6)
#define MAX_INDEX_LIST_POOL	(3)
#define MAX_INCOMPLETE_INDEX_LIST_POOL	(30) 
/* MAX_DOC_HITS_SIZE: size of the document set retrieved from VRF */
#define MAX_DOC_HITS_SIZE (1200000)

enum index_list_type {
	EXCLUDE,
	FAKE_OP,
	NOT_EXIST_WORD,
	NORMAL
};

enum requesttype {
	FULL_SEARCH,
	LIGHT_SEARCH,
	ABSTRACT_INFO,
	FULL_INFO,
	NO_HANDLER
};

// 통합검색 지원
struct group_result_t {
	char field[20];
	char value[32];
	int count;
};

#define MAX_GROUP_RESULT 200

struct index_list_t {
	index_list_t *prev; /* for managing free index list */
	index_list_t *next;

	uint32_t	ndochits; /* min of ndochits and MAX_DOC_HITS_SIZE 
						  (and could be smaller if filtered by field) */
	uint32_t 	list_size; /* size of doc_hits array/idf/relevancy */
	uint32_t	nhits;
	uint32_t	*relevancy;
	doc_hit_t	*doc_hits;
	uint32_t	field;
	uint32_t	wordid;
	enum		index_list_type list_type;
	char		word[STRING_SIZE];
	char 		is_complete_list; /* if doc_hits, relevancy, idf is allocated, its true */

	group_result_t group_result[MAX_GROUP_RESULT];
	int group_result_count;
};

struct request_t {
	enum requesttype type;
	char query_string[MAX_QUERY_STRING_SIZE];
	char attr_string[MAX_ATTR_STRING_SIZE];
	char attr2_string[MAX_ATTR_STRING_SIZE];
	char group_string[MAX_GROUP_STRING_SIZE];
	char sort_string[MAX_SORT_STRING_SIZE];
	int first_result; /* first_result = result page number*list_size + 1*/
	int  list_size;    /* size of result list */

	char word_list[MAX_QUERY_STRING_SIZE];
	index_list_t *result_list;
	char titles[COMMENT_LIST_SIZE][STRING_SIZE];
	char comments[COMMENT_LIST_SIZE][LONG_LONG_STRING_SIZE]; 
									/* XXX:COMMENT_SIZE? */
	char otherId[COMMENT_LIST_SIZE][STRING_SIZE];
	int8_t filtering_id;

	int sb4error;
};

SB_DECLARE_HOOK(int, qp_init,(void))
SB_DECLARE_HOOK(int, qp_light_search, (void* word_db, request_t *r))
SB_DECLARE_HOOK(int, qp_full_search, (void* word_db, request_t *r))
SB_DECLARE_HOOK(int, qp_abstract_info, (request_t *r))
SB_DECLARE_HOOK(int, qp_full_info, (request_t *r))
SB_DECLARE_HOOK(int, qp_finalize_search, (request_t *r))

SB_DECLARE_HOOK(int, qp_docattr_query_process, (docattr_cond_t *cond, char *querystring)) // AT
SB_DECLARE_HOOK(int, qp_docattr_query2_process, (docattr_cond_t *cond, char *querystring)) // AT2
SB_DECLARE_HOOK(int, qp_docattr_group_query_process, (docattr_cond_t *cond, char *querystring)) // GR

#if 0
SB_DECLARE_HOOK(int, qp_filter,\
		(index_list_t *s1, index_list_t *s2, index_list_t *d, filter_t *f, \
		 int rank_func(rank_t*,rank_t*,uint32_t*)))
SB_DECLARE_HOOK(int, qp_rank, \
		(rank_t *r1, rank_t *r2, uint32_t *relevancy))
#endif	
#endif
