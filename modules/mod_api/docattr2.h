/* $Id$ */
#ifndef DOCATTR2_H
#define DOCATTR2_H 1

#include <stdint.h> /* uint32_t */

typedef struct docattr_t docattr_t;
typedef struct docattr_cond_t docattr_cond_t;
typedef struct docattr_mask_t docattr_mask_t;

/* attr structure of user specific docattr module 
 * must be smaller than DOCATTR_ELEMENT_SIZE(64) byte */
#define MAX_DOCATTR_ELEMENT_SIZE		64 /*byte*/
struct docattr_t {
	char rsv[MAX_DOCATTR_ELEMENT_SIZE];
};

#define DOCATTR_SET_ZERO(a) memset((a), 0x00, sizeof(*a))
#define DOCCOND_SET_ZERO(a) memset((a), 0x00, sizeof(*a))
#define DOCMASK_SET_ZERO(a) memset((a), 0x00, sizeof(*a))
#define DOCSORT_SET_ZERO(a) memset((a), 0x00, sizeof(*a))

typedef int (*condfunc)(void *dest, void *cond, uint32_t docid);
typedef int (*maskfunc)(void *dest, void *mask);

SB_DECLARE_HOOK(int,docattr_open,(void))
SB_DECLARE_HOOK(int,docattr_close,(void))
SB_DECLARE_HOOK(int,docattr_get_base_ptr,(docattr_t **base_ptr, int* record_size ))
SB_DECLARE_HOOK(int,docattr_synchronize,(void))
SB_DECLARE_HOOK(int,docattr_get,(uint32_t docid, void *p_doc_attr))
SB_DECLARE_HOOK(int,docattr_ptr_get,(uint32_t docid, docattr_t **p_doc_attr))
SB_DECLARE_HOOK(int,docattr_set,(uint32_t docid, void *p_doc_attr))
SB_DECLARE_HOOK(int,docattr_get_array, \
	(uint32_t *dest, int destsize, uint32_t *sour, \
	 int soursize, condfunc func, void *cond))
SB_DECLARE_HOOK(int,docattr_set_array, \
	(uint32_t *list, int listsize, maskfunc func, void *mask))
SB_DECLARE_HOOK(int,docattr_mask_function,(void *dest, void *mask))

/* key means field name of document */
SB_DECLARE_HOOK(int,docattr_set_docattr_function, \
		(void *dest, char *key, char *value))
SB_DECLARE_HOOK(int,docattr_get_docattr_function, \
		(void *dest, char *key, char *buf, int buflen))
SB_DECLARE_HOOK(int,docattr_get_field_integer_function, \
		(void *dest, char *key, int *value))
SB_DECLARE_HOOK(int,docattr_set_doccond_function, \
		(void *dest, char *key, char *value))
SB_DECLARE_HOOK(int,docattr_set_docmask_function, \
		(void *dest, char *key, char *value))
#endif
