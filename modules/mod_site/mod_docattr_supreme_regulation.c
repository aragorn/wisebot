/* $Id$ */
#include "common_core.h"
#include "common_util.h"
#include "mod_api/index_word_extractor.h"
#include "mod_api/docattr.h"
#include "mod_docattr_supreme_regulation.h"
#include "mod_qp/mod_qp.h"

#include <stdio.h>
#include <stdlib.h>

#define MAX_ENUM_NUM		1024
#define MAX_ENUM_LEN		SHORT_STRING_SIZE

static char *constants[MAX_ENUM_NUM] = { NULL };
static int constants_value[MAX_ENUM_NUM];

static int return_constants_value(char *value, int valuelen);
/*
 * NOTICE:
 * 	if you want to filter out the element, this function should return 0
 * 	othersize, return other number
 */
static int compare_function(void *dest, void *cond, uint32_t docid) {  /* 검색시 비교 함수 */
	int i;
	supreme_regulation_attr_t *docattr = (supreme_regulation_attr_t *)dest;
	supreme_regulation_cond_t *doccond = (supreme_regulation_cond_t *)cond;

	/* always check delete mark */
	if (docattr->is_deleted) {
		return 0;
	}

	if (doccond->enactdate_check == 1) {
		if (doccond->enactdate_start > docattr->enactdate ||
				doccond->enactdate_finish < docattr->enactdate)
			return 0;
	}

	if (doccond->enfodate_check == 1) {
		if (doccond->enfodate_start > docattr->enfodate ||
				doccond->enfodate_finish < docattr->enfodate)
			return 0;
	}

	if (doccond->history_check == 1 && doccond->history != docattr->history) {
		return 0;
	}

	if (doccond->gubun_check > 0 && doccond->gubun[0] != 255) {
		for (i=0; i<doccond->gubun_check; i++) {
			if (doccond->gubun[i] == docattr->gubun) break;
		}
		if (i == doccond->gubun_check) return 0;
	}

	return 1;
}

static int compare_function_for_qsort(const void *dest, const void *sour, 
		void *userdata)
{
	int i, diff;
	docattr_sort_t *sh;
	supreme_regulation_attr_t attr1, attr2;

	if (sb_run_docattr_get(((doc_hit_t *)dest)->id, &attr1) < 0) {
		error("cannot get docattr element");
		return 0;
	}
	if (sb_run_docattr_get(((doc_hit_t *)sour)->id, &attr2) < 0) {
		error("cannot get docattr element");
		return 0;
	}

	sh = (docattr_sort_t *)userdata;
	for (i=0; 
			i<MAX_SORTING_CRITERION && sh->keys[i].key[0]!='\0'; 
			i++) {

		switch (sh->keys[i].key[0]) {
			case '1': // title
				if ((diff = hangul_strncmp(attr1.title, attr2.title, 16)) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '2': // enactdate
				if ((diff = attr1.enactdate - attr2.enactdate) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '3': // ruleno1
				if ((diff = attr1.ruleno1 - attr2.ruleno1) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '4': // ctrltype
				if ((diff = attr1.ctrltype - attr2.ctrltype) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '5': // ruleno2
				if ((diff = attr1.ruleno2 - attr2.ruleno2) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
		}
	}

	return 0;
}

/*
 * NOTICE:
 * 	if success, return SUCCESS(1)
 */
static int mask_function(void *dest, void *mask) {
	supreme_regulation_attr_t *docattr = (supreme_regulation_attr_t *)dest;
	supreme_regulation_mask_t *docmask = (supreme_regulation_mask_t *)mask;

	if (docmask->delete_mark)
		docattr->is_deleted = 1;

	if (docmask->undelete_mark)
		docattr->is_deleted = 0;

	if (docmask->set_history)
		docattr->history = docmask->history;

	if (docmask->set_gubun)
		docattr->gubun = docmask->gubun;

	if (docmask->set_enactdate)
		docattr->enactdate = docmask->enactdate;

	if (docmask->set_enfodate)
		docattr->enfodate = docmask->enfodate;

	if (docmask->set_title)
		memcpy(docattr->title, docmask->title, 16);

	if (docmask->set_ruleno1)
		docattr->ruleno1 = docmask->ruleno1;

	if (docmask->set_ruleno2)
		docattr->ruleno2 = docmask->ruleno2;

	if (docmask->set_ctrltype)
		docattr->ctrltype = docmask->ctrltype;

	return 1;
}

/* if fail, return -1 */
static int set_docattr_function(void *dest, char *key, char *value)
{
	supreme_regulation_attr_t *docattr = (supreme_regulation_attr_t *)dest;

	if (strcasecmp(key, "Delete") == 0) {
		docattr->is_deleted = 1;
	}
	else if (strcasecmp(key, "gubun") == 0) {
		docattr->gubun = 
			(uint8_t)return_constants_value(value, strlen(value));
	}
	else if (strcasecmp(key, "history") == 0) {
		docattr->history = (uint8_t)atoi(value);
	}
	else if (strcasecmp(key, "enactdate") == 0) {
		docattr->enactdate = (int32_t)atol(value);
	}
	else if (strcasecmp(key, "enfodate") == 0) {
		docattr->enfodate = (int32_t)atol(value);
	}
	else if (strcasecmp(key, "title") == 0) {
		/* chinese -> hangul */
		char _value[SHORT_STRING_SIZE];

		index_word_extractor_t *ex = NULL;
		index_word_t word;

		strncpy(_value, value, SHORT_STRING_SIZE-1);
		_value[SHORT_STRING_SIZE-1] = '\0';

		ex = sb_run_new_index_word_extractor(2);
		if (ex == NULL || ex == (index_word_extractor_t*)MINUS_DECLINE) return FAIL;
		sb_run_index_word_extractor_set_text(ex, _value);

		sb_run_get_index_words(ex, &word, 16);
		strncpy(docattr->title, word.word, 16);
	}
	else if (strcasecmp(key, "ruleNo1") == 0) {
		docattr->ruleno1 = (uint32_t)atol(value);
	}
	else if (strcasecmp(key, "ruleNo2") == 0) {
		docattr->ruleno2 = (uint32_t)atol(value);
	}
	else if (strcasecmp(key, "ctrltype") == 0) {
		docattr->ctrltype = 
			(uint32_t)return_constants_value(value, strlen(value));
	}
#if 0
	else if (strcasecmp(key, "type_oldruleno")) {
		if (!docattr->history && strlen(value)) {
			int n;
			DocId docid;
			docattr_mask_t docmask;

			/* get docid with type_oldruleno */
			if ((n = sb_run_client_get_docid(value, &docid)) < 0 &&
					n == DI_NOT_REGISTERED) {
				error("cannot get new docid");
				return -1;
			}

			/* set mask of history to 0 */
			DOCMASK_SET_ZERO(&docmask);
			sb_run_docattr_set_docmask_function(&docmask, "history", "0");

			/* mask docattr of old ruleno with history bit on */
			sb_run_docattr_set_array(&docid, 1, SC_MASK, &docmask);
		}
	}
#endif
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

static int get_docattr_function(void *dest, char *key, char *buf, int buflen)
{
	supreme_regulation_attr_t *docattr = (supreme_regulation_attr_t *)dest;

	if (strcasecmp(key, "Delete") == 0) {
		snprintf(buf, buflen, "%u",docattr->is_deleted);
	}
	else if (strcasecmp(key, "gubun") == 0) {
		snprintf(buf, buflen, "%u",docattr->gubun);
	}
	else if (strcasecmp(key, "history") == 0) {
		snprintf(buf, buflen, "%u",docattr->history);
	}
	else if (strcasecmp(key, "enactdate") == 0) {
		snprintf(buf, buflen, "%u",docattr->enactdate);
	}
	else if (strcasecmp(key, "enfodate") == 0) {
		snprintf(buf, buflen, "%u",docattr->enfodate);
	}
	else if (strcasecmp(key, "title") == 0) {
		snprintf(buf, buflen, "%s",docattr->title);
	}
	else if (strcasecmp(key, "ruleNo1") == 0) {
		snprintf(buf, buflen, "%u",docattr->ruleno1);
	}
	else if (strcasecmp(key, "ruleNo2") == 0) {
		snprintf(buf, buflen, "%u",docattr->ruleno2);
	}
	else if (strcasecmp(key, "ctrltype") == 0) {
		snprintf(buf, buflen, "%u",docattr->ctrltype);
	}
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}
	return 1;
}

/* if fail, return -1 */
static int set_doccond_function(void *dest, char *key, char *value)
{
	supreme_regulation_cond_t *doccond = (supreme_regulation_cond_t *)dest;
	char *c, values[12][SHORT_STRING_SIZE];
	int i, n=0;

	if (strcmp(key, "Delete") == 0) {
		doccond->delete_check = 1;
		return 1;
	}

	INFO("key:%s, value:%s",key,value);

	while ((c = strchr(value, ',')) != NULL) {
			*c = ' ';
	}

	if (strcmp(key, "gubun") == 0) {
		n = sscanf(value, "%s %s %s %s %s %s %s %s %s %s %s %s ", 
				values[0], values[1], values[2],
				values[3], values[4], values[5],
				values[6], values[7], values[8],
				values[9], values[10], values[11]);
		for (i=0; i<n; i++) {
			doccond->gubun[i] = 
				return_constants_value(values[i], strlen(values[i]));
		}
		doccond->gubun_check = n;
	}
	else if (strcmp(key, "history") == 0) {
		doccond->history = (uint8_t)atoi(value);
		doccond->history_check = 1;
	}
    else if (strcmp(key, "enactdate") == 0) {
		n = sscanf(value, "%u-%u", &(doccond->enactdate_start), &(doccond->enactdate_finish));
		if (n != 2) {
			warn("wrong docattr query: enactdate");
			return -1;
		}
        doccond->enactdate_check = 1;
    }
    else if (strcmp(key, "enfodate") == 0) {
		n = sscanf(value, "%u-%u", &(doccond->enfodate_start), &(doccond->enfodate_finish));
		if (n != 2) {
			warn("wrong docattr query: enfodate");
			return -1;
		}
        doccond->enfodate_check = 1;
    }
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

static int set_docmask_function(void *dest, char *key, char *value)
{
	supreme_regulation_mask_t *docmask = (supreme_regulation_mask_t *)dest;

	if (strcmp(key, "Delete") == 0) {
		docmask->delete_mark = 1;
	}
    else if (strcmp(key, "Undelete") == 0) {
        docmask->undelete_mark = 1;
    }
	else if (strcmp(key, "history") == 0) {
		docmask->history = (uint8_t)atoi(value);
		docmask->set_history = 1;
	}
	else if (strcasecmp(key, "gubun") == 0) {
		docmask->gubun = 
			(uint8_t)return_constants_value(value, strlen(value));
		docmask->set_gubun = 1;
	}
	else if (strcasecmp(key, "enactdate") == 0) {
		docmask->enactdate = (int32_t)atol(value);
		docmask->set_enactdate = 1;
	}
	else if (strcasecmp(key, "enfodate") == 0) {
		docmask->enfodate = (int32_t)atol(value);
		docmask->set_enfodate = 1;
	}
	else if (strcasecmp(key, "title") == 0) {
		/* chinese -> hangul */
		char _value[SHORT_STRING_SIZE];

		index_word_extractor_t *ex = NULL;
		index_word_t word;

		strncpy(_value, value, SHORT_STRING_SIZE-1);
		_value[SHORT_STRING_SIZE-1] = '\0';

		ex = sb_run_new_index_word_extractor(2);
		if (ex == NULL || ex == (index_word_extractor_t*)MINUS_DECLINE) return FAIL;
		sb_run_index_word_extractor_set_text(ex, _value);

		sb_run_get_index_words(ex, &word, 16);
		strncpy(docmask->title, word.word, 16);
		docmask->set_title = 1;
	}
	else if (strcasecmp(key, "ruleNo1") == 0) {
		docmask->ruleno1 = (uint32_t)atol(value);
		docmask->set_ruleno1 = 1;
	}
	else if (strcasecmp(key, "ruleNo2") == 0) {
		docmask->ruleno2 = (uint32_t)atol(value);
		docmask->set_ruleno2 = 1;
	}
	else if (strcasecmp(key, "ctrltype") == 0) {
		docmask->ctrltype = 
			(uint32_t)return_constants_value(value, strlen(value));
		docmask->set_ctrltype = 1;
	}
	else {
		warn("there is no such a field[%s]", key);
	}
	return 1;
}
 
/* if fail, return 0 */
static int return_constants_value(char *value, int valuelen)
{
	int i;
	for (i=0; i<MAX_ENUM_NUM && constants[i]; i++) {
		if (strncmp(value, constants[i], MAX_ENUM_LEN) == 0) {
			break;
		}
	}
	if (i == MAX_ENUM_NUM || constants[i] == NULL) {
		return 0;
	}
	return constants_value[i];
}

static void get_enum(configValue v)
{
	int i;
	static char enums[MAX_ENUM_NUM][MAX_ENUM_LEN];
	for (i=0; i<MAX_ENUM_NUM && constants[i]; i++);
	if (i == MAX_ENUM_NUM) {
		error("too many constant is defined");
		return;
	}
	strncpy(enums[i], v.argument[0], MAX_ENUM_LEN);
	enums[i][MAX_ENUM_LEN-1] = '\0';
	constants[i] = enums[i];
	constants_value[i] = atoi(v.argument[1]);
/*	INFO("Enum[%s]: %d", constants[i], constants_value[i]);*/
}

static config_t config[] = {
	CONFIG_GET("Enum", get_enum, 2, "constant"),
	{NULL}
};

static void register_hooks(void)
{
	sb_hook_docattr_compare_function(compare_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_mask_function(mask_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_sort_function(compare_function_for_qsort, 
			NULL, NULL, HOOK_MIDDLE);

	sb_hook_docattr_set_docattr_function(set_docattr_function,
			NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_get_docattr_function(get_docattr_function,
			NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_set_doccond_function(set_doccond_function,
			NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_set_docmask_function(set_docmask_function,
			NULL, NULL, HOOK_MIDDLE);
}

module docattr_supreme_regulation_module = {
	STANDARD_MODULE_STUFF,
	config,					/* conf table */
	NULL,					/* registry */
	NULL,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};
