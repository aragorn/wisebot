/* $Id$ */

#include "common_core.h"
#include "common_util.h"
#include "mod_api/docattr.h"
#include "mod_api/qp.h"

#include "mod_qp/mod_qp.h"

#include <string.h>

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

	debug("attrquery[%s]", attrquery);
	if (sb_run_docattr_set_doccond_function(cond, "delete", buf) == -1) {
		warn("cannot check delete mark");
	}

	if (attrquery[0] != '\0') {
	
		len = strlen(attrquery);
		attrquery[len] = '&';
		attrquery[len+1] = '\0';

		get_str_item(buf, attrquery, "casekind:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "casekind", buf) == -1) {
				return -1;
			}
		}
		get_str_item(buf, attrquery, "partkind:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "partkind", buf) == -1) {
				return -1;
			}
		}
		get_str_item(buf, attrquery, "reportdate:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "reportdate", buf) == -1) {
				return -1;
			}
		}
		get_str_item(buf, attrquery, "reportgrade:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "reportgrade", buf) == -1) {
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

module qp_legalj_module = {
	STANDARD_MODULE_STUFF,
	NULL,					/* config */
	NULL,					/* registry */
	NULL,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};
