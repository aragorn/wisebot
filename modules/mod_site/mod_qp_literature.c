/* $Id$ */

#include "softbot.h"
#include "mod_api/mod_api.h"
#include "mod_api/docattr.h"
#include "mod_qp/mod_qp.h"

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

		len = strlen(attrquery);
		attrquery[len] = '&';
		attrquery[len+1] = '\0';

		get_str_item(buf, attrquery, "TYPE2:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "TYPE2", buf) == -1) {
				return -1;
			}
		}

		get_str_item(buf, attrquery, "date:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "date", buf) == -1) {
				return -1;
			}
		}

		get_str_item(buf, attrquery, "LAN:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "LAN", buf) == -1) {
				return -1;
			}
		}

		get_str_item(buf, attrquery, "part:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "part", buf) == -1) {
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

module qp_literature_module = {
	STANDARD_MODULE_STUFF,
	NULL,					/* config */
	NULL,					/* registry */
	NULL,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};
