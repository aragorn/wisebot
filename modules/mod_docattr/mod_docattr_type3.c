/* $Id$ */
#include "softbot.h"
#include "mod_docattr_type3.h"
#include "mod_qp/mod_qp.h"
#include "mod_api/mod_api.h"

/*
 * NOTICE:
 * 	if you want to filter out the element, this function should return 0
 * 	othersize, return other number
 */
int compare_function3(void *dest, void *cond) {
	doc_attr3_t *docattr = (doc_attr3_t *)dest;
	doc_cond3_t *doccond = (doc_cond3_t *)cond;

	if (doccond->delete_check && docattr->is_deleted) {
		return 0;
	}

	if (doccond->registered_time_check) {
		switch (doccond->registered_time_check_type) {
			case EQUAL_TO:
				if (doccond->registered_pivot_time == docattr->registered_time)
					return 0;
				break;
			case EARLIER_THAN:
				if (doccond->registered_pivot_time <= docattr->registered_time)
					return 0;
				break;
			case LATER_THAN:
				if (doccond->registered_pivot_time >= docattr->registered_time)
					return 0;
				break;
		}
	}

	return -1;
}

/*
 * NOTICE:
 * The comparison function must return an integer less than, equal to, or greater
 * than  zero  if  the first argument is considered to be respectively
 * less than, equal to, or greater than the second.  If two members compare
 * as equal,  their order in the sorted array is undefined.
 */
int compare_function_for_qsort3_sort_by_registered_time(const void *dest, const void *sour) {
	doc_attr3_t attr1, attr2;

	if (sb_run_docattr_get(((doc_hit_t *)dest)->docid, &attr1) < 0) {
		error("cannot get docattr element");
		// XXX which value should be returned on error?
		return 0;
	}

	if (sb_run_docattr_get(((doc_hit_t *)sour)->docid, &attr2) < 0) {
		error("cannot get docattr element");
		return 0;
	}

	return ((int)attr1.registered_time - (int)attr2.registered_time);
}

/*
 * NOTICE:
 * 	if success, return SUCCESS(1)
 */
int mask_function3(void *dest, void *mask) {
	doc_attr3_t *docattr = (doc_attr3_t *)dest;
	doc_mask3_t *docmask = (doc_mask3_t *)mask;

	if (docmask->delete_mark)
		docattr->is_deleted = 1;

	if (docmask->registered_time_mark)
		docattr->registered_time = docmask->registered_time;

	if (docmask->created_time_mark)
		docattr->created_time = docmask->created_time;

	return 1;
}
