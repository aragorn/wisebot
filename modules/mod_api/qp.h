/* $Id$ */
#ifndef _QP_H_
#define _QP_H_ 1

#include "softbot.h"
#include "mod_api/docattr.h"

typedef struct request_t request_t;
typedef struct filter_t filter_t;
typedef struct rank_t rank_t;
typedef struct index_list_t index_list_t;

#if 0
//XXX: 기본적인 filter를 만들고
//      extend할 수 있게 한다. needless?? 2002/07/16 --jiwon
struct filter_t{
	int within;
	int phrase;// if it's phrase operator
	void* ext_filter;
};

//XXX: ranking을 조절할 수 있는 extention을 만든다.
struct rank_t{
	int word_freq;
	int near;//if it's near operator, we give more high relevancy to position
	int pagerank;// XXX: ???
	void* ext_rank;
}
#endif

SB_DECLARE_HOOK(int, qp_init,(void))
SB_DECLARE_HOOK(int, qp_light_search, (request_t *r))
SB_DECLARE_HOOK(int, qp_full_search, (request_t *r))
SB_DECLARE_HOOK(int, qp_abstract_info, (request_t *r))
SB_DECLARE_HOOK(int, qp_full_info, (request_t *r))
SB_DECLARE_HOOK(int, qp_finalize_search, (request_t *r))

SB_DECLARE_HOOK(int, qp_docattr_query_process, (docattr_cond_t *cond, char *querystring))

#if 0
SB_DECLARE_HOOK(int, qp_filter,\
		(index_list_t *s1, index_list_t *s2, index_list_t *d, filter_t *f, \
		 int rank_func(rank_t*,rank_t*,uint32_t*)))
SB_DECLARE_HOOK(int, qp_rank, \
		(rank_t *r1, rank_t *r2, uint32_t *relevancy))
#endif	
#endif
