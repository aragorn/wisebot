/* $Id$ */
#include "softbot.h"
#include "mod_docattr_type1.h"

/**
 * NOTICE:
 * 	if you want to filter out the element, this function should return 0
 * 	othersize, return other number
 */
int compare_function1(void *dest, void *cond) {
	doc_attr1_t *docattr = (doc_attr1_t *)dest;

	if (docattr->is_deleted) {
		return 0;
	}
	return -1;
}

/**
 * NOTICE:
 * 	if success, return SUCCESS(1)
 */
int mask_function1(void *dest, void *mask) {
	doc_attr1_t *docattr = (doc_attr1_t *)dest;

	docattr->is_deleted = 1;

	return 1;
}
