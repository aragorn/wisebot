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
	char buf[STRING_SIZE], key[STRING_SIZE], *query, *restore, *lts, *ltres;
	int i;

	DOCCOND_SET_ZERO(cond);

	if (sb_run_docattr_set_doccond_function(cond, "Delete", buf) == -1) {
		warn("cannot check delete mark");
	}

	if (attrquery[0] != '\0') {
	
	//XXX: overflow expected
//	len = strlen(attrquery);
//	attrquery[len] = '&';
//	attrquery[len+1] = '\0';

	query = restore = attrquery;
	query = strstr(attrquery, "PronounceDate:");
	if (query) {
		query += strlen("PronounceDate:");
		restore = strchr(query, '&');
		if (restore) {
			strncpy(buf, query, restore-query);
			buf[restore-query] = '\0';
		}
		else {
			strcpy(buf, query);
		}
		if (sb_run_docattr_set_doccond_function(cond, "PronounceDate", buf) 
				== -1) {
			warn("pronounce date query format is wrong");
		}
	}

	i = 1;
	query = restore = attrquery;
	while (1) {
		restore = strchr(query, '|');
		if (restore == NULL)
		{
			if (*query)
			{
				restore = query;
				while(*restore) restore++;
			}
			else
				break;
		}
		else
		{
			*restore = '\0';
			restore++;
		}

		get_str_item(buf, query, "CourtType:", '&', STRING_SIZE);
		if (buf[0]) {
			sprintf(key, "%d:%s", i, "CourtType");
			if (sb_run_docattr_set_doccond_function(cond, key, buf) == -1) {
				break;
			}
		}

		get_str_item(buf, query, "LawType:", '&', STRING_SIZE);
		if (buf[0]) {
			sprintf(key, "%d:%s", i, "LawType");
			ltres = lts = buf;
			while (1) {
				ltres = strchr(lts, ',');
				if (ltres == NULL)
				{
					if (*lts)
					{
						ltres = lts;
						while(*ltres) ltres++;
					}
					else
						break;
				}
				else
				{
					*ltres = '\0';
					ltres++;
				}

				if (sb_run_docattr_set_doccond_function(cond, key, lts) 
						== -1) {
					break;
				}

				lts = ltres;
			}
		}

		get_str_item(buf, query, "GAN:", '&', STRING_SIZE);
		if (buf[0]) {
			sprintf(key, "%d:%s", i, "GAN");
			INFO("GAN key: %s", key);
			if (sb_run_docattr_set_doccond_function(cond, key, buf) == -1) {
				break;
			}
		}

		get_str_item(buf, query, "WON:", '&', STRING_SIZE);
		if (buf[0]) {
			sprintf(key, "%d:%s", i, "WON");
			if (sb_run_docattr_set_doccond_function(cond, key, buf) == -1) {
				break;
			}
		}

		get_str_item(buf, query, "MIGANOPEN:", '&', STRING_SIZE);
		if (buf[0]) {
			sprintf(key, "%d:%s", i, "MIGANOPEN");
			if (sb_run_docattr_set_doccond_function(cond, key, buf) == -1) {
				break;
			}
		}

		get_str_item(buf, query, "DEL:", '&', STRING_SIZE);
		if (buf[0]) {
			sprintf(key, "%d:%s", i, "DEL");
			if (sb_run_docattr_set_doccond_function(cond, key, buf) == -1) {
				break;
			}
		}

		get_str_item(buf, query, "CLOSE:", '&', STRING_SIZE);
		if (buf[0]) {
			sprintf(key, "%d:%s", i, "CLOSE");
			if (sb_run_docattr_set_doccond_function(cond, key, buf) == -1) {
				break;
			}
		}

		get_str_item(buf, query, "Court:", '&', STRING_SIZE);
		if (buf[0]) {
			sprintf(key, "%d:%s", i, "Court");
			if (sb_run_docattr_set_doccond_function(cond, key, buf) == -1) {
				break;
			}
		}

		i++;
		query = restore;
	}

	}

	return 1;
}

static void register_hooks(void)
{
	/* XXX: module which uses qp should call sb_run_qp_init once after fork. */
	sb_hook_qp_docattr_query_process(docattr_filtering,NULL,NULL,HOOK_MIDDLE);
}

module qp_precedent_module = {
	STANDARD_MODULE_STUFF,
	NULL,					/* config */
	NULL,					/* registry */
	NULL,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};
