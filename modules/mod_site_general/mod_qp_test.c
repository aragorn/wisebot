/* $Id$ */
#include <string.h> /* strstr(3),strlen(3),strchar(3),strncpy(3) */
#include "common_core.h"
#include "mod_api/qp.h"

static int get_str_item(char *dest, char *dit, char *key, char delimiter, int len)
{
	char *start, *end, restore;

	start = strstr(dit, key);
	if ( start == NULL ) {
		dest[0] = '\0';
		return FAIL;
	}
	start += strlen(key);

	end = strchr(start, delimiter);
	if ( end == NULL ) 
	{
		strncpy(dest, start, len);
		return SUCCESS;
	}

	restore = *end;
	*end = '\0';

	strncpy(dest, start, len);
	dest[len-1] = '\0';
	*end = restore;

	return SUCCESS;
}

static int docattr_filtering(docattr_cond_t *cond, char *attrquery)
{
	int len;
	char buf[STRING_SIZE];

	DOCCOND_SET_ZERO(cond);
	if (sb_run_docattr_set_doccond_function(cond, "Delete", buf) == -1) {
		warn("cannot check delete mark");
	}

	if (attrquery[0] != '\0') {

		//XXX: ... --;; i'm in hurry...
		len = strlen(attrquery);
		attrquery[len] = '&';
		attrquery[len+1] = '\0';
		
		get_str_item(buf, attrquery, "date:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "date", buf) == -1) {
				return -1;
			}
		}

		get_str_item(buf, attrquery, "int1:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "int1", buf) == -1) {
				return -1;
			}
		}

		get_str_item(buf, attrquery, "int2:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "int2", buf) == -1) {
				return -1;
			}
		}

		get_str_item(buf, attrquery, "bit1:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "bit1", buf) == -1) {
				return -1;
			}
		}

		get_str_item(buf, attrquery, "bit2:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "bit2", buf) == -1) {
				return -1;
			}
		}

		get_str_item(buf, attrquery, "cate:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "cate", buf) == -1) {
				return -1;
			}
		}

	}

	return 1;
}

static void register_hooks(void)
{
	/* XXX: module which uses qp should call sb_run_qp_init once after fork. */
	sb_hook_qp_docattr_query_process(docattr_filtering,NULL,NULL,HOOK_MIDDLE);
}

module qp_test_module = {
	STANDARD_MODULE_STUFF,
	NULL,					/* config */
	NULL,					/* registry */
	NULL,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};
