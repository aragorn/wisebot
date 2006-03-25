/* $Id$ */
#ifndef DOCATTR_H
#define DOCATTR_H 1

#include <stdint.h> /* uint32_t */

typedef struct docattr_t docattr_t;
typedef struct docattr_cond_t docattr_cond_t;
typedef struct docattr_mask_t docattr_mask_t;
typedef struct docattr_sort_t docattr_sort_t;

/* attr structure of user specific docattr module 
 * must be smaller than DOCATTR_ELEMENT_SIZE(64) byte */
#define MAX_DOCATTR_ELEMENT_SIZE		64 /*byte*/
struct docattr_t {
	char rsv[MAX_DOCATTR_ELEMENT_SIZE];
};

#include "qp.h"

#define DOCATTR_SET_ZERO(a) memset((a), 0x00, sizeof(*a))
#define DOCCOND_SET_ZERO(a) memset((a), 0x00, sizeof(*a))
#define DOCMASK_SET_ZERO(a) memset((a), 0x00, sizeof(*a))
#define DOCSORT_SET_ZERO(a) memset((a), 0x00, sizeof(*a))

#define SC_COMP  sb_run_docattr_compare_function  // sort 이전
#define SC_COMP2 sb_run_docattr_compare2_function // sort 이후
#define SC_MASK  sb_run_docattr_mask_function
#define SC_SORT  sb_run_docattr_sort_function


/* cond structure of user specific docattr module 
 * must be smaller than STRING_SIZE(256) x 2 bytes */
struct docattr_cond_t {
	char rsv1[STRING_SIZE];
	char rsv2[STRING_SIZE];
};

/* mask structure of user specific docattr module 
 * must be smaller than STRING_SIZE(256) bytes */
struct docattr_mask_t {
	char rsv[STRING_SIZE];
};

/* key means field name to sort by 
 * order is minus if descent, plus if ascent */
#define MAX_SORTING_CRITERION		8
struct docattr_sort_t {
	int index;
	struct {
		char key[STRING_SIZE];
		int order;
	} keys[MAX_SORTING_CRITERION];
    sort_base_t* sort_base;
};

typedef int (*condfunc)(void *dest, void *cond, uint32_t docid);
typedef int (*maskfunc)(void *dest, void *mask);

SB_DECLARE_HOOK(int,docattr_open,(void))
SB_DECLARE_HOOK(int,docattr_close,(void))
SB_DECLARE_HOOK(int,docattr_synchronize,(void))
SB_DECLARE_HOOK(int,docattr_get,(uint32_t docid, void *p_doc_attr))
SB_DECLARE_HOOK(int,docattr_ptr_get,(uint32_t docid, docattr_t **p_doc_attr))
SB_DECLARE_HOOK(int,docattr_set,(uint32_t docid, void *p_doc_attr))
SB_DECLARE_HOOK(int,docattr_get_array, \
	(uint32_t *dest, int destsize, uint32_t *sour, \
	 int soursize, condfunc func, void *cond))
SB_DECLARE_HOOK(int,docattr_set_array, \
	(uint32_t *list, int listsize, maskfunc func, void *mask))

SB_DECLARE_HOOK(int,docattr_get_index_list, \
	(index_list_t *dest, index_list_t *sour, \
	 condfunc func, void *cond))
SB_DECLARE_HOOK(int,docattr_set_index_list, \
	(index_list_t *list, maskfunc func, void *mask))
SB_DECLARE_HOOK(int,docattr_index_list_sortby, \
	(sort_base_t *sort_base, void *userdata, \
	 int (*compar)(const void *, const void *, void *)))

SB_DECLARE_HOOK(int,docattr_compare_function,(void *dest, void *cond, uint32_t docid))
SB_DECLARE_HOOK(int,docattr_compare2_function,(void *dest, void *cond, uint32_t docid))
SB_DECLARE_HOOK(int,docattr_set_group_result_function,
		(void *cond, group_result_t* group_result, int* size))
SB_DECLARE_HOOK(int,docattr_mask_function,(void *dest, void *mask))
SB_DECLARE_HOOK(int,docattr_sort_function, \
	(const void *dest, const void *sour, void *userdata))

/* key means field name of document */
SB_DECLARE_HOOK(int,docattr_set_docattr_function, \
		(void *dest, char *key, char *value))
SB_DECLARE_HOOK(int,docattr_get_docattr_function, \
		(void *dest, char *key, char *buf, int buflen))
SB_DECLARE_HOOK(int,docattr_set_doccond_function, \
		(void *dest, char *key, char *value))
SB_DECLARE_HOOK(int,docattr_set_docmask_function, \
		(void *dest, char *key, char *value))

SB_DECLARE_HOOK(int,docattr_modify_index_list, \
		(int id, index_list_t *list))
#endif
