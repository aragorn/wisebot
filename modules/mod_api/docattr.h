/* $Id$ */
#ifndef _DOCATTR_H_
#define _DOCATTR_H_ 1

#include "softbot.h"
#include <string.h>

#define DOCATTR_SET_ZERO(a) memset((a), 0x00, sizeof(*a))
#define DOCCOND_SET_ZERO(a) memset((a), 0x00, sizeof(*a))
#define DOCMASK_SET_ZERO(a) memset((a), 0x00, sizeof(*a))
#define DOCSORT_SET_ZERO(a) memset((a), 0x00, sizeof(*a))

#define SC_COMP  sb_run_docattr_compare_function  // sort 이전
#define SC_COMP2 sb_run_docattr_compare2_function // sort 이후
#define SC_MASK  sb_run_docattr_mask_function
#define SC_SORT  sb_run_docattr_sort_function

/* attr structure of user specific docattr module 
 * must be smaller than DOCATTR_ELEMENT_SIZE(64) byte */
#define MAX_DOCATTR_ELEMENT_SIZE		64 /*byte*/
typedef struct _docattr_t {
	char rsv[MAX_DOCATTR_ELEMENT_SIZE];
} docattr_t;

/* cond structure of user specific docattr module 
 * must be smaller than STRING_SIZE(256) byte */
typedef struct _docattr_cond_t {
	char rsv[STRING_SIZE];
} docattr_cond_t;

/* mask structure of user specific docattr module 
 * must be smaller than STRING_SIZE(256) byte */
typedef struct _docattr_mask_t {
	char rsv[STRING_SIZE];
} docattr_mask_t;

/* key means field name to sort by 
 * order is minus if descent, plus if ascent */
#define MAX_SORTING_CRITERION		8
typedef struct _docattr_sort_t {
	struct _keys {
		char key[STRING_SIZE];
		int order;
	} keys[MAX_SORTING_CRITERION];
} docattr_sort_t;

typedef int (*condfunc)(void *dest, void *cond, uint32_t docid);
typedef int (*maskfunc)(void *dest, void *mask);

struct index_list_t;

SB_DECLARE_HOOK(int,docattr_open,(void))
SB_DECLARE_HOOK(int,docattr_close,(void))
SB_DECLARE_HOOK(int,docattr_synchronize,(void))
SB_DECLARE_HOOK(int,docattr_get,(DocId docid, void *p_doc_attr))
SB_DECLARE_HOOK(int,docattr_set,(DocId docid, void *p_doc_attr))
SB_DECLARE_HOOK(int,docattr_get_array, \
	(DocId *dest, int destsize, DocId *sour, \
	 int soursize, condfunc func, void *cond))
SB_DECLARE_HOOK(int,docattr_set_array, \
	(DocId *list, int listsize, maskfunc func, void *mask))

SB_DECLARE_HOOK(int,docattr_get_index_list, \
	(struct index_list_t *dest, struct index_list_t *sour, \
	 condfunc func, void *cond))
SB_DECLARE_HOOK(int,docattr_set_index_list, \
	(struct index_list_t *list, maskfunc func, void *mask))
SB_DECLARE_HOOK(int,docattr_index_list_sortby, \
	(struct index_list_t *list, void *userdata, \
	 int (*compar)(const void *, const void *, void *)))

SB_DECLARE_HOOK(int,docattr_compare_function,(void *dest, void *cond, uint32_t docid))
SB_DECLARE_HOOK(int,docattr_compare2_function,(void *dest, void *cond, uint32_t docid))
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
		(int id, struct index_list_t *list))
#endif
