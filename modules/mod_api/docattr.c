/* $Id$ */
#include "softbot.h"
#include "mod_api/docattr.h"

HOOK_STRUCT(
	HOOK_LINK(docattr_open)
	HOOK_LINK(docattr_close)
	HOOK_LINK(docattr_synchronize)
	HOOK_LINK(docattr_get)
	HOOK_LINK(docattr_ptr_get)
	HOOK_LINK(docattr_set)
	HOOK_LINK(docattr_get_array)
	HOOK_LINK(docattr_set_array)
	HOOK_LINK(docattr_get_index_list)
	HOOK_LINK(docattr_set_index_list)
	HOOK_LINK(docattr_index_list_sortby)
	HOOK_LINK(docattr_compare_function)
	HOOK_LINK(docattr_compare2_function)
	HOOK_LINK(docattr_set_group_result_function)
	HOOK_LINK(docattr_mask_function)
	HOOK_LINK(docattr_sort_function)
	HOOK_LINK(docattr_set_docattr_function)
	HOOK_LINK(docattr_get_docattr_function)
	HOOK_LINK(docattr_set_doccond_function)
	HOOK_LINK(docattr_set_docmask_function)
	HOOK_LINK(docattr_modify_index_list)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, docattr_open, (void), (), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, docattr_close, (void), (), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, docattr_synchronize, (void), (), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, docattr_get, (uint32_t docid, void *p_doc_attr), \
	(docid, p_doc_attr), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, docattr_ptr_get, (uint32_t docid, docattr_t **p_doc_attr), \
	(docid, p_doc_attr), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, docattr_set, (uint32_t docid, void *p_doc_attr), \
	(docid, p_doc_attr), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, docattr_get_array, \
	(uint32_t *dest, int destsize, uint32_t *sour, int soursize, \
	 condfunc func, void *cond), \
	(dest, destsize, sour, soursize, func, cond), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, docattr_set_array, \
	(uint32_t *list, int listsize, maskfunc func, void *mask), \
	(list, listsize, func, mask), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, docattr_get_index_list, \
	(struct index_list_t *dest, struct index_list_t *sour, \
	 condfunc func, void *cond), \
	(dest, sour, func, cond), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, docattr_set_index_list, \
	(struct index_list_t *list, maskfunc func, void *mask), \
	(list, func, mask), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, docattr_index_list_sortby, \
	(struct index_list_t *list, void *userdata, \
	 int (*compar)(const void *, const void *, void *)), \
	(list, userdata, compar), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int,docattr_compare_function, \
	(void *dest, void *cond, uint32_t docid), (dest,cond,docid), MINUS_DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,docattr_compare2_function, \
	(void *dest, void *cond, uint32_t docid), (dest,cond,docid), MINUS_DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,docattr_set_group_result_function, \
	(void *cond, struct group_result_t* group_result, int* size), (cond,group_result,size), DECLINE)
	
SB_IMPLEMENT_HOOK_RUN_FIRST(int,docattr_mask_function, \
	(void *dest, void *mask), (dest,mask), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,docattr_sort_function, \
	(const void *dest, const void *sour, void *userdata), \
	(dest, sour, userdata), MINUS_DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int,docattr_set_docattr_function, \
	(void *dest, char *key, char *value), \
	(dest, key, value), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,docattr_get_docattr_function, \
	(void *dest, char *key, char *buf, int buflen), \
	(dest, key, buf, buflen), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,docattr_set_doccond_function, \
	(void *dest, char *key, char *value), \
	(dest, key, value), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,docattr_set_docmask_function, \
	(void *dest, char *key, char *value), \
	(dest, key, value), DECLINE)

SB_IMPLEMENT_HOOK_RUN_ALL(int,docattr_modify_index_list, \
	(int id, struct index_list_t *list), \
	(id, list), SUCCESS, DECLINE)
