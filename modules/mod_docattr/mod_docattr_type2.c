/* $Id$ */
#include "softbot.h"
#include "mod_docattr_type2.h"

/**
 * NOTICE:
 * 	if you want to filter out the element, this function should return 0
 * 	othersize, return other number
 */
int compare_function2(void *dest, void *cond) {
	doc_attr2_t *docattr = (doc_attr2_t *)dest;
	doc_cond2_t *doccond = (doc_cond2_t *)cond

	if (doccond->delete_check && docattr->is_deleted) {
		return 0;
	}

	switch (doccond->category_search_type) {
		case SEARCH_THIS_CATEGORY:
			break;
		case SEARCH_BELOW_CATEGORY:
			break;
		case SEARCH_MULTI_CATEGORY:
			break;
	}

	return -1;
}

/**
 * NOTICE:
 * 	if success, return SUCCESS(1)
 */
int mask_function2(void *dest, void *mask) {
	doc_attr2_t *docattr = (doc_attr1_t *)dest;

	if (docmask->delete_mark)
		docattr->is_deleted = 1;

	return 1;
}
