/* $Id$ */
#ifndef _MOD_QP_H_
#define _MOD_QP_H_ 1

#include "softbot.h"
#include "mod_indexer/hit.h"
#include "mod_api/qpp.h" /* XXX: why? */

#define MAX_QUERY_STRING_SIZE	LONG_LONG_STRING_SIZE //LONG_STRING_SIZE	
#define MAX_ATTR_STRING_SIZE	LONG_STRING_SIZE
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

struct index_list_t {
	struct index_list_t *prev; /* for managing free index list */
	struct index_list_t *next;

	uint32_t	ndochits; /* min of ndochits and MAX_DOC_HITS_SIZE 
						  (and could be smaller if filtered by field) */
	uint32_t 	list_size; /* size of doc_hits array/idf/relevancy */
	uint32_t	nhits;
	uint32_t	df;
	uint32_t	*relevancy;
	doc_hit_t	*doc_hits;
	uint32_t	field;
	uint32_t	wordid;
	enum		index_list_type list_type;
	char		word[STRING_SIZE];
	char 		is_complete_list; /* if doc_hits, relevancy, idf is allocated, its true */
};

struct request_t{
	enum requesttype type;
	char query_string[MAX_QUERY_STRING_SIZE];
	char attr_string[MAX_ATTR_STRING_SIZE];
	char sort_string[MAX_SORT_STRING_SIZE];
	uint32_t first_result; /* first_result = result page number*list_size + 1*/
	int  list_size;    /* size of result list */

	char word_list[MAX_QUERY_STRING_SIZE];
	struct index_list_t *result_list;
	char titles[COMMENT_LIST_SIZE][STRING_SIZE];
	char comments[COMMENT_LIST_SIZE][LONG_LONG_STRING_SIZE]; 
									/* XXX:COMMENT_SIZE? */
	char otherId[COMMENT_LIST_SIZE][STRING_SIZE];
	uint8_t filtering_id;

	int sb4error;
};

/* stack related stuff */
typedef struct {
	uint32_t size;
	struct index_list_t *first;
	struct index_list_t *last;
} sb_stack_t;

void init_stack(sb_stack_t *stack);
void stack_push(sb_stack_t *stack, struct index_list_t *this);
struct index_list_t *stack_pop(sb_stack_t *stack);
/* stack related stuff end */

int light_search (struct request_t *req);
int getAutoComment(char *pszStr, int lPosition);

#endif
