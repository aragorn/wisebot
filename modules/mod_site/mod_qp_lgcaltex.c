/* $Id$ */

#include "softbot.h"
#include "mod_api/docattr.h"
#include "mod_api/qp.h"
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

	debug("query = [%s]", attrquery);
	if (attrquery[0] != '\0') {

		//XXX: ... --;; i'm in hurry...
		len = strlen(attrquery);
		attrquery[len] = '&';
		attrquery[len+1] = '\0';

		get_str_item(buf, attrquery, "SystemName:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "SystemName", buf) == -1) {
				return -1;
			}
		}

		get_str_item(buf, attrquery, "Part:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "Part", buf) == -1) {
				return -1;
			}
		}

		get_str_item(buf, attrquery, "AppFlag:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "AppFlag", buf) == -1) {
				return -1;
			}
		}

		get_str_item(buf, attrquery, "FileExt:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "FileExt", buf) == -1) {
				return -1;
			}
		}

		get_str_item(buf, attrquery, "Duty:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "Duty", buf) == -1) {
				return -1;
			}
		}
		
		get_str_item(buf, attrquery, "Structure:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "Structure", buf) == -1) {
				return -1;
			}
		}
		
		get_str_item(buf, attrquery, "Person:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "Person", buf) == -1) {
				return -1;
			}
		}
		
		get_str_item(buf, attrquery, "TFT:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "TFT", buf) == -1) {
				return -1;
			}
		}
		
		get_str_item(buf, attrquery, "SUPER:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "SUPER", buf) == -1) {
				return -1;
			}
		}
		
		get_str_item(buf, attrquery, "Date1:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "Date1", buf) == -1) {
				return -1;
			}
		}
		
		get_str_item(buf, attrquery, "Date2:", '&', STRING_SIZE);
		if (buf[0]) {
			if (sb_run_docattr_set_doccond_function(cond, "Date2", buf) == -1) {
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

module qp_lgcaltex_module = {
	STANDARD_MODULE_STUFF,
	NULL,					/* config */
	NULL,					/* registry */
	NULL,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};
