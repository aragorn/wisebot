/* $Id$ */
#include "common_core.h"
#include "docattr2.h"

HOOK_STRUCT(
	HOOK_LINK(docattr_open)
	HOOK_LINK(docattr_close)
	HOOK_LINK(docattr_get_base_ptr)
	HOOK_LINK(docattr_synchronize)
	HOOK_LINK(docattr_get)
	HOOK_LINK(docattr_ptr_get)
	HOOK_LINK(docattr_set)
	HOOK_LINK(docattr_get_array)
	HOOK_LINK(docattr_set_array)
	HOOK_LINK(docattr_mask_function)
	HOOK_LINK(docattr_set_docattr_function)
	HOOK_LINK(docattr_get_docattr_function)
	HOOK_LINK(docattr_get_field_integer_function)
	HOOK_LINK(docattr_set_doccond_function)
	HOOK_LINK(docattr_set_docmask_function)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, docattr_open, (void), (), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, docattr_close, (void), (), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, docattr_get_base_ptr, (docattr_t** base_ptr, \
	int* record_size ), (base_ptr, record_size), DECLINE)
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
SB_IMPLEMENT_HOOK_RUN_FIRST(int,docattr_mask_function, \
	(void *dest, void *mask), (dest,mask), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int,docattr_set_docattr_function, \
	(void *dest, char *key, char *value), \
	(dest, key, value), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,docattr_get_docattr_function, \
	(void *dest, char *key, char *buf, int buflen), \
	(dest, key, buf, buflen), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,docattr_get_field_integer_function, \
	(void *dest, char *key, int *value), \
	(dest, key, value), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,docattr_set_doccond_function, \
	(void *dest, char *key, char *value), \
	(dest, key, value), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,docattr_set_docmask_function, \
	(void *dest, char *key, char *value), \
	(dest, key, value), DECLINE)
