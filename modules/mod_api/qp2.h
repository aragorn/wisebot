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
typedef struct user_data_t user_data_t;
typedef struct comment_t comment_t;
typedef struct weight_t weight_t;
typedef struct weight_list_t weight_list_t;

#include "docattr2.h"
#include "indexer.h"
#include "msg_record.h"

#define MAX_QUERY_STRING_SIZE	LONG_STRING_SIZE
#define MAX_ATTR_STRING_SIZE	LONG_STRING_SIZE
#define MAX_GROUP_STRING_SIZE   STRING_SIZE
#define MAX_SORT_STRING_SIZE	STRING_SIZE

/* MAX_INDEX_LIST_POOL: should be larger than max nodes of qpp */
#define MAX_INDEX_LIST_POOL	(3)
#define MAX_INCOMPLETE_INDEX_LIST_POOL	(30) 
/* MAX_DOC_HITS_SIZE: size of the document set retrieved from VRF */
#define MAX_DOC_HITS_SIZE (1200000)


// 최대 하위 노드 수
// 4bit로 표현할수 있는 수.
#define MAX_SEARCH_NODE             (1 << 4) // 2^4 = 16(0은 사용 않함)

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

enum output_style { STYLE_XML, STYLE_SOFTBOT4, };
enum doc_type { DOCUMENT, VIRTUAL_DOCUMENT, };
enum key_type { DOCATTR, DID, RELEVANCY, };
enum sortarraytype { INDEX_LIST, AGENT_INFO, };
enum order_type { DESC=-1, ASC=1, };
enum clause_type { UNKNOWN = -1, SELECT, WEIGHT, SEARCH, VIRTUAL_ID,
    WHERE, GROUP_BY, ORDER_BY, COUNT_BY, LIMIT, 
	START_DID_RULE, END_DID_RULE, COMMENT, OUTPUT_STYLE, MAX_CLAUSE_TYPE};

struct weight_t {
	int weight;
	int field_id;
	char name[SHORT_STRING_SIZE];
};

#define MAX_WEIGHT (16)
struct weight_list_t {
	int cnt;
	weight_t list[MAX_WEIGHT];
};

struct virtual_document_t {
    uint32_t id;

    doc_hit_t* dochits;
    int dochit_cnt;      // doc_hits의 갯수.
    docattr_t* docattr;  // 항상 하나임.
	int comment_cnt;

	uint32_t relevance;  // doc_hists::relevance 의 합.

	uint32_t node_id;
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
	int is_enum;           // docattr의 field type
    limit_t limit;
    int cnt; // group의 건수
};

#define MAX_SORT_RULE (32)
struct orderby_rule_list_t {
    int cnt;
	orderby_rule_t list[MAX_SORT_RULE];
};

struct user_data_t {
	enum doc_type doc_type;
    orderby_rule_list_t* rules;
	void* docattr_base_ptr;
	int docattr_record_size;
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
    char clause[LONG_STRING_SIZE];

	union {
		groupby_rule_list_t groupby;
		orderby_rule_list_t orderby;
        limit_t limit;
        char where[LONG_STRING_SIZE];
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

struct comment_t {
    char s[LONG_LONG_STRING_SIZE];
	uint32_t did;
	uint32_t node_id;
};

#define MAX_VID_RULE 5
struct request_t {
	char query[MAX_QUERY_STRING_SIZE];
    char search[MAX_QUERY_STRING_SIZE];
    select_list_t select_list;
    weight_list_t weight_list;
	operation_list_t op_list_vid;
	operation_list_t op_list_did;
	key_rule_t virtual_rule[MAX_VID_RULE];
	int virtual_rule_cnt;
	msg_record_t msg;
    enum output_style output_style;
};

struct response_t {
    groupby_result_list_t groupby_result_vid;
    groupby_result_list_t groupby_result_did;
	virtual_document_list_t* vdl;
	char parsed_query[LONG_STRING_SIZE];
    comment_t comments[COMMENT_LIST_SIZE];
    uint32_t node_id;
    limit_t limit;
    int search_result;
};

struct index_list_t {
	index_list_t *prev; /* for managing free index list */
	index_list_t *next;

	uint32_t	ndochits; /* size of doc_hits array/idf/relevance */
	uint32_t 	list_size; /* MAX_DOC_HITS_SIZE */
	uint32_t	nhits;
	uint32_t	*relevance;
	doc_hit_t	*doc_hits;
	uint32_t	field;
	uint32_t	wordid;
	enum		index_list_type list_type;
	char		word[STRING_SIZE];
	char 		is_complete_list; /* if doc_hits, relevance, idf is allocated, its true */
};

SB_DECLARE_HOOK(int, qp_init,(void))
SB_DECLARE_HOOK(int, qp_init_request,(request_t* req, char* query))
SB_DECLARE_HOOK(int, qp_init_response,(response_t* res))
SB_DECLARE_HOOK(int, qp_light_search, (request_t *req, response_t *res))
SB_DECLARE_HOOK(int, qp_full_search, (request_t *req, response_t *res))
SB_DECLARE_HOOK(int, qp_abstract_search, (request_t *r, response_t *res))
SB_DECLARE_HOOK(int, qp_do_filter_operation, (request_t *r, response_t *res, enum doc_type type))
SB_DECLARE_HOOK(int, qp_get_query_string, (request_t *r, char query[MAX_QUERY_STRING_SIZE]))
SB_DECLARE_HOOK(int, qp_finalize_search, (request_t *r, response_t *res))

SB_DECLARE_HOOK(int, qp_cb_orderby_virtual_document, (const void *dest, const void *sour, void *userdata))
SB_DECLARE_HOOK(int, qp_cb_orderby_document, (const void *dest, const void *sour, void *userdata))
SB_DECLARE_HOOK(int, qp_cb_where, (const void *data))
SB_DECLARE_HOOK(int, qp_set_where_expression, (char *clause))

#endif
