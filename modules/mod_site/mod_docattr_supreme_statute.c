/* $Id$ */
#include "softbot.h"
#include "mod_api/index_word_extractor.h"
#include "mod_api/docattr.h"
#include "mod_api/qp.h"
#include "mod_docattr_supreme_statute.h"
#include "mod_qp/mod_qp.h"

#include <stdio.h>

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
	int i=0;
	supreme_statute_attr_t *docattr = (supreme_statute_attr_t *)dest;
	supreme_statute_cond_t *doccond = (supreme_statute_cond_t *)cond;

	/* always check delete mark */
	if (docattr->is_deleted) {
		return 0;
	}

	if (doccond->law_prodate_check == 1) {
		if (doccond->law_prodate_start > docattr->law_prodate ||
				doccond->law_prodate_finish < docattr->law_prodate)
			return 0;
	}

	if (doccond->law_enfodate_check == 1) {
		if (doccond->law_enfodate_start > docattr->law_enfodate ||
				doccond->law_enfodate_finish < docattr->law_enfodate)
			return 0;
	}

	if (doccond->history_check == 1 && doccond->history != docattr->history) {
		if (doccond->history != 1) { // if history = 1, all document must be passed by.
			return 0;
		}
	}

	if (doccond->law_unit_check == 1 && doccond->law_unit != docattr->law_unit) {
		return 0;
	}

	if (doccond->law_part_check > 0 && doccond->law_part[i] != 255) {
		for (i=0; i<doccond->law_part_check; i++) {
			if (doccond->law_part[i] == (docattr->law_part/100) * 100) break;
		}
		if (i == doccond->law_part_check) return 0;
	}

	return 1;
}

//static uint8_t zerostr[DOCATTR_RID_LEN] = { 0 };
static int compare_rid(const void *dest, const void *sour)
{
	unsigned long long l1, l2;
	supreme_statute_attr_t attr1, attr2;

	if (sb_run_docattr_get(((doc_hit_t *)dest)->id, &attr1) < 0) {
		error("cannot get docattr element");
		return 0;
	}

	if (sb_run_docattr_get(((doc_hit_t *)sour)->id, &attr2) < 0) {
		error("cannot get docattr element");
		return 0;
	}
	
	l1 = *((unsigned long long*)attr1.rid);
	l2 = *((unsigned long long*)attr2.rid);

	if (l1 > l2) return 1;
	else if (l1 == l2) return 0;
	else return -1;
}
static int docattr_distinct_rid(int id, index_list_t *list)
{
	int i, abst;
	supreme_statute_attr_t attr1, attr2;
	unsigned long long rid1, rid2;

	/* default filtering id */
//	if (id != 0) return DECLINE;
	qsort(list->doc_hits, list->ndochits, sizeof(doc_hit_t), compare_rid);

	abst = 0;

	if (sb_run_docattr_get(list->doc_hits[0].id, &attr1) < 0) {
		error("cannot get docattr element");
		return -1;
	}
	rid2 = *((unsigned long long*)attr1.rid);

	for (i=0; i<list->ndochits; ) {
		rid1 = rid2;

		if (abst != i) {
			memcpy(&(list->doc_hits[abst]), &(list->doc_hits[i]),
					sizeof(doc_hit_t));
		}

		for (i++; i<list->ndochits; i++) {
			if (sb_run_docattr_get(list->doc_hits[i].id, &attr2) < 0) {
				error("cannot get docattr element");
				return FAIL;
			}

			rid2 = *((unsigned long long*)attr2.rid);

			if (rid1 == 0 || rid2 == 0) break;
			if (rid1 != rid2) break;
		}

		abst++;
	}

	list->ndochits = abst;

	return SUCCESS;
}

static int compare_function_for_qsort(const void *dest, const void *sour, 
		void *userdata)
{
	int i, diff;
	docattr_sort_t *sh;
	supreme_statute_attr_t attr1, attr2;

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
			case '1': // law_name 
				if ((diff = hangul_strncmp(attr1.law_name, attr2.law_name, SORTING_STRING_LEN)) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '2': // law_prodate
				if ((diff = attr1.law_prodate - attr2.law_prodate) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '3': // law_enfodate
				if ((diff = attr1.law_enfodate - attr2.law_enfodate) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '4': // law_status
				if ((diff = attr1.law_status - attr2.law_status) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '5': // jono1
				if ((diff = attr1.jono1 - attr2.jono1) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '6': // jono2
				if ((diff = attr1.jono2 - attr2.jono2) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '7': // law_prono
				if ((diff = attr1.law_prono - attr2.law_prono) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '8': // law_part
				if ((diff = attr1.law_part - attr2.law_part) == 0) {
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
	supreme_statute_attr_t *docattr = (supreme_statute_attr_t *)dest;
	supreme_statute_mask_t *docmask = (supreme_statute_mask_t *)mask;

	if (docmask->delete_mark)
		docattr->is_deleted = 1;

	if (docmask->undelete_mark)
		docattr->is_deleted = 0;

	if (docmask->set_rid) {
		MD5_CTX context;
		unsigned char digest[16];
		unsigned int len = strlen(docmask->rid);

		MD5Init(&context);
		MD5Update(&context, docmask->rid, len);
		MD5Final(digest, &context);

		memcpy(docattr->rid, digest, DOCATTR_RID_LEN);
	}

	if (docmask->set_history)
		docattr->history = docmask->history;

	if (docmask->set_law_part)
		docattr->law_part = docmask->law_part;

	if (docmask->set_law_unit)
		docattr->law_unit = docmask->law_unit;

	if (docmask->set_law_status)
		docattr->law_status = docmask->law_status;

	if (docmask->set_law_prodate)
		docattr->law_prodate = docmask->law_prodate;

	if (docmask->set_law_enfodate)
		docattr->law_enfodate = docmask->law_enfodate;

	if (docmask->set_law_name)
		memcpy(docattr->law_name, docmask->law_name, SORTING_STRING_LEN);

	if (docmask->set_jono1)
		docattr->jono1 = docmask->jono1;

	if (docmask->set_jono2)
		docattr->jono2 = docmask->jono2;

	if (docmask->set_law_prono)
		docattr->law_prono = docmask->law_prono;

	return 1;
}

/* if fail, return -1 */
static int set_docattr_function(void *dest, char *key, char *value)
{
	supreme_statute_attr_t *docattr = (supreme_statute_attr_t *)dest;

	if (strcasecmp(key, "Delete") == 0) {
		docattr->is_deleted = 1;
	}
	else if (strcasecmp(key, "Law_Part") == 0) {
		docattr->law_part = 
			(uint32_t)return_constants_value(value, strlen(value));
	}
	else if (strcasecmp(key, "history") == 0) {
		docattr->history = (uint8_t)atoi(value);
	}
	else if (strcasecmp(key, "Law_Unit") == 0) {
		docattr->law_unit = (uint8_t)atoi(value);
	}
	else if (strcasecmp(key, "Law_Status") == 0) {
		docattr->law_status = 
			(uint8_t)return_constants_value(value, strlen(value));
	}
	else if (strcasecmp(key, "Law_ProDate") == 0) {
		docattr->law_prodate = (int32_t)atol(value);
	}
	else if (strcasecmp(key, "Law_EnfoDate") == 0) {
		docattr->law_enfodate = (int32_t)atol(value);
	}
	else if (strcasecmp(key, "Law_Name") == 0) {
		/* chinese -> hangul */
		char _value[SHORT_STRING_SIZE];
		index_word_extractor_t *ex = NULL;
		index_word_t word;

		strncpy(_value, value, SHORT_STRING_SIZE-1);
		_value[SHORT_STRING_SIZE-1] = '\0';

		ex = sb_run_new_index_word_extractor(2);
		if (ex == NULL || ex == (index_word_extractor_t*)MINUS_DECLINE) return FAIL;
		sb_run_index_word_extractor_set_text(ex, _value);

		sb_run_get_index_words(ex, &word, SORTING_STRING_LEN);
		strncpy(docattr->law_name, word.word, SORTING_STRING_LEN);

		sb_run_delete_index_word_extractor(ex);
	}
	else if (strcasecmp(key, "JoNo1") == 0) {
		docattr->jono1 = (uint16_t)atoi(value);
	}
	else if (strcasecmp(key, "JoNo2") == 0) {
		docattr->jono2 = (uint16_t)atoi(value);
	}
	else if (strcasecmp(key, "Law_ProNo") == 0) {
		docattr->law_prono = (uint32_t)atoi(value);
	}
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

static int get_docattr_function(void *dest, char *key, char *buf, int buflen)
{
	supreme_statute_attr_t *docattr = (supreme_statute_attr_t *)dest;

	if (strcasecmp(key, "Delete") == 0) {
		snprintf(buf, buflen, "%u",docattr->is_deleted);
	}
	else if (strcasecmp(key, "Law_Part") == 0) {
		snprintf(buf, buflen, "%u",docattr->law_part);
	}
	else if (strcasecmp(key, "history") == 0) {
		snprintf(buf, buflen, "%u",docattr->history);
	}
	else if (strcasecmp(key, "Law_Status") == 0) {
		snprintf(buf, buflen, "%u",docattr->law_status);
	}
	else if (strcasecmp(key, "Law_Unit") == 0) {
		snprintf(buf, buflen, "%u",docattr->law_unit);
	}
	else if (strcasecmp(key, "Law_ProDate") == 0) {
		snprintf(buf, buflen, "%u",docattr->law_prodate);
	}
	else if (strcasecmp(key, "Law_EnfoDate") == 0) {
		snprintf(buf, buflen, "%u",docattr->law_enfodate);
	}
	else if (strcasecmp(key, "Law_Name") == 0) {
		snprintf(buf, buflen, "%s",docattr->law_name);
	}
	else if (strcasecmp(key, "JoNo1") == 0) {
		snprintf(buf, buflen, "%u",docattr->jono1);
	}
	else if (strcasecmp(key, "JoNo2") == 0) {
		snprintf(buf, buflen, "%u",docattr->jono2);
	}
	else if (strcasecmp(key, "Law_ProNo") == 0) {
		snprintf(buf, buflen, "%u",docattr->law_prono);
	}
	else if (strcasecmp(key, "Rid") == 0) {
		snprintf(buf, buflen, "%u",*((uint32_t *)docattr->rid));
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
	supreme_statute_cond_t *doccond = (supreme_statute_cond_t *)dest;
	char *c, values[4][SHORT_STRING_SIZE];
	int i, n=0;

	if (strcmp(key, "Delete") == 0) {
		doccond->delete_check = 1;
		return 1;
	}

	INFO("key:%s, value:%s",key,value);

	while ((c = strchr(value, ',')) != NULL) {
			*c = ' ';
	}

	if (strcmp(key, "Law_Part") == 0) {
		n = sscanf(value, "%s %s %s %s", 
				values[0], values[1], values[2], values[3]);
		for (i=0; i<n; i++) {
			doccond->law_part[i] = 
				return_constants_value(values[i], strlen(values[i]));
		}
		doccond->law_part_check = n;
	}
	else if (strcmp(key, "history") == 0) {
		doccond->history = (uint8_t)atoi(value);
		doccond->history_check = 1;
	}
	else if (strcmp(key, "Law_Unit") == 0) {
		doccond->law_unit = (uint8_t)atoi(value);
		doccond->law_unit_check = 1;
	}
    else if (strcmp(key, "Law_ProDate") == 0) {
		n = sscanf(value, "%u-%u", &(doccond->law_prodate_start), &(doccond->law_prodate_finish));
		if (n != 2) {
			warn("wrong docattr query: law_prodate");
			return -1;
		}
        doccond->law_prodate_check = 1;
    }
    else if (strcmp(key, "Law_EnfoDate") == 0) {
		n = sscanf(value, "%u-%u", &(doccond->law_enfodate_start), &(doccond->law_enfodate_finish));
		if (n != 2) {
			warn("wrong docattr query: law_enfodate");
			return -1;
		}
        doccond->law_enfodate_check = 1;
    }
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

static int set_docmask_function(void *dest, char *key, char *value)
{
	supreme_statute_mask_t *docmask = (supreme_statute_mask_t *)dest;

	if (strcasecmp(key, "Delete") == 0) {
		docmask->delete_mark = 1;
	}
    else if (strcasecmp(key, "Undelete") == 0) {
        docmask->undelete_mark = 1;
    }
	else if (strcasecmp(key, "Rid") == 0) {
		docmask->set_rid = 1;
		strncpy(docmask->rid, value, SHORT_STRING_SIZE);
		docmask->rid[SHORT_STRING_SIZE-1] = '\0';
	}
	else if (strcasecmp(key, "history") == 0) {
		docmask->history = (uint8_t)atoi(value);
		docmask->set_history = 1;
	}
	else if (strcasecmp(key, "Law_Part") == 0) {
		docmask->law_part = return_constants_value(value, strlen(value));
		docmask->set_law_part = 1;
	}
	else if (strcasecmp(key, "Law_Unit") == 0) {
		docmask->law_unit = (uint8_t)atoi(value);
		docmask->set_law_unit = 1;
	}
	else if (strcasecmp(key, "Law_Status") == 0) {
		docmask->law_status = 
			(uint8_t)return_constants_value(value, strlen(value));
		docmask->set_law_status = 1;
	}
	else if (strcasecmp(key, "Law_ProDate") == 0) {
		docmask->law_prodate = (int32_t)atol(value);
		docmask->set_law_prodate = 1;
	}
	else if (strcasecmp(key, "Law_EnfoDate") == 0) {
		docmask->law_enfodate = (int32_t)atol(value);
		docmask->set_law_enfodate = 1;
	}
	else if (strcasecmp(key, "Law_Name") == 0) {
		/* chinese -> hangul */
		char _value[SHORT_STRING_SIZE];
		index_word_extractor_t *ex = NULL;
		index_word_t word;

		strncpy(_value, value, SHORT_STRING_SIZE-1);
		_value[SHORT_STRING_SIZE-1] = '\0';

		ex = sb_run_new_index_word_extractor(2);
		if (ex == NULL || ex == (index_word_extractor_t*)MINUS_DECLINE) return FAIL;
		sb_run_index_word_extractor_set_text(ex, _value);

		sb_run_get_index_words(ex, &word, SORTING_STRING_LEN);
		strncpy(docmask->law_name, word.word, SORTING_STRING_LEN);
		docmask->set_law_name = 1;

		sb_run_delete_index_word_extractor(ex);
	}
	else if (strcasecmp(key, "JoNo1") == 0) {
		docmask->jono1 = (uint16_t)atoi(value);
		docmask->set_jono1 = 1;
	}
	else if (strcasecmp(key, "JoNo2") == 0) {
		docmask->jono2 = (uint16_t)atoi(value);
		docmask->set_jono2 = 1;
	}
	else if (strcasecmp(key, "Law_ProNo") == 0) {
		docmask->law_prono = (uint32_t)atoi(value);
		docmask->set_law_prono = 1;
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

/*****************************************************************************/
static int module_init(void)
{
	debug("sizeof(supreme_statute_attr_t) = %d", (int)sizeof(supreme_statute_attr_t));
	debug("sizeof(supreme_statute_mask_t) = %d", (int)sizeof(supreme_statute_mask_t));
	debug("sizeof(supreme_statute_cond_t) = %d", (int)sizeof(supreme_statute_cond_t));
	sb_assert(sizeof(supreme_statute_attr_t) == 64);
	sb_assert(sizeof(supreme_statute_mask_t) <= STRING_SIZE);
	sb_assert(sizeof(supreme_statute_cond_t) <= STRING_SIZE);

	return SUCCESS;
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
	INFO("Enum[%s]: %d", constants[i], constants_value[i]);
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
	sb_hook_docattr_modify_index_list(docattr_distinct_rid,
			NULL, NULL, HOOK_MIDDLE);
}

module docattr_supreme_statute_module = {
	STANDARD_MODULE_STUFF,
	config,					/* conf table */
	NULL,					/* registry */
	module_init,			/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};
