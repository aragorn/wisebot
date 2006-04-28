/* $Id$ */
#ifndef QP2_H
#define QP2_H 1

typedef struct request_t request_t;
typedef struct response_t response_t;
typedef struct filter_t filter_t;
typedef struct rank_t rank_t;
typedef struct index_list_t index_list_t;
typedef struct virtual_document_t virtual_document_t;
typedef struct virtual_document_list_t virtual_document_list_t;
typedef struct key_rule_t key_rule_t;
typedef struct limit_t limit_t;
typedef struct orderby_rule_t orderby_rule_t;
typedef struct group_t group_t;
typedef struct groupby_rule_t groupby_rule_t;
typedef struct groupby_result_list_t groupby_result_list_t;
typedef struct orderby_rule_list_t orderby_rule_list_t;
typedef struct groupby_rule_list_t groupby_rule_list_t;
typedef struct operation_t operation_t;
typedef struct operation_list_t operation_list_t;
typedef struct select_list_t select_list_t;

#include "docattr2.h"
#include "indexer.h"

#define MAX_QUERY_STRING_SIZE	LONG_STRING_SIZE
#define MAX_ATTR_STRING_SIZE	LONG_STRING_SIZE
#define MAX_GROUP_STRING_SIZE   STRING_SIZE
#define MAX_SORT_STRING_SIZE	STRING_SIZE

/* MAX_INDEX_LIST_POOL: should be larger than max nodes of qpp */
#define MAX_INDEX_LIST_POOL	(3)
#define MAX_INCOMPLETE_INDEX_LIST_POOL	(30) 
/* MAX_DOC_HITS_SIZE: size of the document set retrieved from VRF */
#define MAX_DOC_HITS_SIZE (1200000)
#define MAX_GROUP_RESULT (200)

#define MAX_AGENT_SEARCH_ROW_NUM    (1000)
#define MAX_SEARCH_LIST_COUNT 		(100)

// 최대 하위 노드 수
// 4bit로 표현할수 있는 수.
#define MAX_SEARCH_NODE (1 << 4) 
#define DEFAULT_PORT "8000"

#define MAX_AGENT_DOC_HITS_COUNT (MAX_SEARCH_NODE * MAX_AGENT_SEARCH_ROW_NUM)

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

enum doc_type { DOCUMENT, VIRTUAL_DOCUMENT, };
enum key_type { DOCATTR, DID, RELEVANCY, };
enum sortarraytype { INDEX_LIST, AGENT_INFO, };
enum order_type { DESC=-1, ASC=1, };
enum clause_type { UNKNOWN = -1, SELECT, SEARCH, VIRTUAL_ID,
    WHERE, GROUP_BY, ORDER_BY, LIMIT, 
	START_DID_RULE, END_DID_RULE, MAX_CLAUSE_TYPE};

struct virtual_document_t {
    uint32_t id;

    doc_hit_t* dochits;
    int dochit_cnt;      // doc_hits의 갯수.
    docattr_t* docattr;  // 항상 하나임.
	uint32_t relevancy;  // doc_hists::relevancy 의 합.
};

struct key_rule_t {
    enum key_type type;           // DOCATTR, DID, RELEVANCY
    char name[SHORT_STRING_SIZE];   // type이 DOCATTR 경우 field 명이 저장됨
};

struct virtual_document_list_t {
    int cnt;
    virtual_document_t* data;    
};

struct limit_t {
    int start;
    int cnt;
};

struct orderby_rule_t {
    enum order_type type; //DESC, ASC
	key_rule_t rule;
};

struct groupby_rule_t {
    orderby_rule_t sort;
    limit_t limit;
    int cnt; // group의 건수
};

#define MAX_SORT_RULE (32)
struct orderby_rule_list_t {
    int cnt;
	orderby_rule_t list[MAX_SORT_RULE];
};

#define MAX_GROUP_RULE (8)
struct groupby_rule_list_t {
    int cnt;
    groupby_rule_t list[MAX_GROUP_RULE];
}; 

#define MAX_CARDINALITY (256)
struct groupby_result_list_t {
    groupby_rule_list_t rules;
    int result[MAX_GROUP_RULE][MAX_CARDINALITY];
};

struct operation_t {
    enum clause_type type; // WHERE, GROUP_BY, ORDER_BY
    char clause[SHORT_STRING_SIZE];

	union {
		groupby_rule_list_t groupby;
		orderby_rule_list_t orderby;
        limit_t limit;
        char* where;
	}rule;
};

#define MAX_OPERATION 16
struct operation_list_t {
    int cnt;
    operation_t list[MAX_OPERATION];
};

struct select_list_t {
	int cnt;
	char field_name[MAX_EXT_FIELD][SHORT_STRING_SIZE];
};

struct request_t {
	char query[MAX_QUERY_STRING_SIZE];
    char search[MAX_QUERY_STRING_SIZE];
    select_list_t select_list;
	operation_list_t op_list_vid;
	operation_list_t op_list_did;
	key_rule_t virtual_rule;
};

struct response_t {
    groupby_result_list_t groupby_result_vid;
    groupby_result_list_t groupby_result_did;
	virtual_document_list_t* vdl;
	char word_list[STRING_SIZE];
    char comments[COMMENT_LIST_SIZE][LONG_LONG_STRING_SIZE];
    uint32_t node_id;
    limit_t limit;
    int search_result;
};

struct index_list_t {
	index_list_t *prev; /* for managing free index list */
	index_list_t *next;

	uint32_t	ndochits; /* size of doc_hits array/idf/relevancy */
	uint32_t 	list_size; /* MAX_DOC_HITS_SIZE */
	uint32_t	nhits;
	uint32_t	*relevancy;
	doc_hit_t	*doc_hits;
	uint32_t	field;
	uint32_t	wordid;
	enum		index_list_type list_type;
	char		word[STRING_SIZE];
	char 		is_complete_list; /* if doc_hits, relevancy, idf is allocated, its true */
};

SB_DECLARE_HOOK(int, qp_init,(void))
SB_DECLARE_HOOK(int, qp_init_request,(request_t* req, char* query))
SB_DECLARE_HOOK(int, qp_init_response,(response_t* res))
SB_DECLARE_HOOK(int, qp_light_search, (request_t *req, response_t *res))
SB_DECLARE_HOOK(int, qp_full_search, (request_t *req, response_t *res))
SB_DECLARE_HOOK(int, qp_abstract_search, (request_t *r, response_t *res))
SB_DECLARE_HOOK(int, qp_do_filter_operate, (request_t *r, response_t *res, enum doc_type type))
SB_DECLARE_HOOK(int, qp_finalize_search, (request_t *r, response_t *res))

SB_DECLARE_HOOK(int, qp_cb_orderby_virtual_document, (const void *dest, const void *sour, void *userdata))
SB_DECLARE_HOOK(int, qp_cb_where_virtual_document, (const void *data))
SB_DECLARE_HOOK(int, qp_set_where_expression, (char *clause))

#endif
