/* $Id$ */
#include "softbot.h"
#include "mod_api/docattr.h"
#include "mod_api/morpheme.h"
#include "mod_api/qp.h"
#include "mod_docattr_supreme_literature.h"
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
	int check, i;
	supreme_literature_attr_t *docattr = (supreme_literature_attr_t *)dest;
	supreme_literature_cond_t *doccond = (supreme_literature_cond_t *)cond;

	/* always check delete mark */
	if (docattr->is_deleted) {
		return 0;
	}

	if (doccond->date_check == 1) {
		if (doccond->date_start > docattr->date ||
				doccond->date_finish < docattr->date)
			return 0;
	}

	if (doccond->type2[0] != 0xFF && doccond->type2_check > 0) {
		check = 0;
		for (i=0; i<doccond->type2_check; i++) {
			if (doccond->type2[i] == docattr->type2) check = 1;
		}
		if (check == 0) return 0;
	}

	if (doccond->lan[0] != 0xFF && doccond->lan_check > 0) {
		check = 0;
		for (i=0; i<doccond->lan_check; i++) {
			if (doccond->lan[i] == docattr->lan) check = 1;
		}
		if (check == 0) return 0;
	}

	if (doccond->part[0] != 0xFF && doccond->part_check > 0) {
		check = 0;
		for (i=0; i<doccond->part_check; i++) {
			if (doccond->part[i] == docattr->part) check = 1;
		}
		if (check == 0) return 0;
	}

	return 1;
}

static int compare_function_for_qsort(const void *dest, const void *sour, 
		void *userdata)
{
	int i, diff;
	docattr_sort_t *criterion;
	supreme_literature_attr_t attr1, attr2;

	if (sb_run_docattr_get(((doc_hit_t *)dest)->id, &attr1) < 0) {
		error("cannot get docattr element");
		return 0;
	}

	if (sb_run_docattr_get(((doc_hit_t *)sour)->id, &attr2) < 0) {
		error("cannot get docattr element");
		return 0;
	}

	criterion = (docattr_sort_t *)userdata;
	for (i=0; 
			i<MAX_SORTING_CRITERION && criterion->keys[i].key[0]!='\0'; 
			i++) {

		switch (criterion->keys[i].key[0]) {
			case '1':
				if ((diff = attr1.date - attr2.date) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
				}
				break;
			case '2':
				/*
                if (attr1.part == 2 && attr2.part == 2) {
                    break;
                }
                else if (attr1.part == 2 && attr2.part == 3) {
                    return 1;
                }
                else if (attr1.part == 3 && attr2.part == 2) {
                    return -1;
                }
                else if (attr1.part == 3 && attr2.part == 3) {
                    break;
                }
				*/
				if ((diff = attr1.part - attr2.part) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
				}
				break;
			case '3':
				if ((diff = attr1.lan - attr2.lan) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
				}
				break;
			case '4':
				if ((diff = hangul_strncmp(attr1.title, attr2.title, 16)) 
						== 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
				}
				break;
			case '5':
				if ((diff = hangul_strncmp(attr1.author, attr2.author, 16)) 
						== 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
				}
				break;
			case '6':
				if ((diff = hangul_strncmp(attr1.pubsrc, attr2.pubsrc, 16)) 
						== 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
				}
				break;
			case '7':
				if ((diff = attr1.type2 - attr2.type2) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
				}
				/*
				if (attr1.type2 == 1 && attr2.type2 == 1) {
					break;
				}
				else if (attr1.type2 == 1 && attr2.type2 == 3) {
					return 1;
				}
				else if (attr1.type2 == 3 && attr2.type2 == 1) {
					return -1;
				}
				else if (attr1.type2 == 3 && attr2.type2 == 3) {
					break;
				}
				*/
				break;
			case '8':
				break;
		}
	}

	return 0;
}

static int compare_rid(const void *dest, const void *sour)
{
	unsigned long long l1, l2;
	supreme_literature_attr_t attr1, attr2;

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
	supreme_literature_attr_t attr1, attr2;
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

/*
 * NOTICE:
 * 	if success, return SUCCESS(1)
 */
static int mask_function(void *dest, void *mask) 
{
	supreme_literature_attr_t *docattr = (supreme_literature_attr_t *)dest;
	supreme_literature_mask_t *docmask = (supreme_literature_mask_t *)mask;

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

	if (docmask->set_ctrlno && docmask->set_ctrltype) {
		MD5_CTX context;
		char tmp[STRING_SIZE];
		unsigned char digest[16];
		unsigned int len = strlen(docmask->rid);

		strncpy(tmp, docmask->ctrlno, 32);
		tmp[32] = '\0';
		strcat(tmp, "_");
		strncat(tmp, docmask->ctrltype, 32);

		MD5Init(&context);
		MD5Update(&context, tmp, len);
		MD5Final(digest, &context);

		memcpy(docattr->rid, digest, DOCATTR_RID_LEN);
	}

	if (docmask->set_type2) {
		docattr->type2 = docmask->type2;
	}

	if (docmask->set_date) {
		docattr->date = docmask->date;
	}

	if (docmask->set_lan) {
		docattr->lan = docmask->lan;
	}

	if (docmask->set_part) {
		docattr->part = docmask->part;
	}

	if (docmask->set_title) {
		strncpy(docattr->title, docmask->title, 16);
	}

	if (docmask->set_author) {
		strncpy(docattr->author, docmask->author, 16);
	}

	if (docmask->set_pubsrc) {
		strncpy(docattr->pubsrc, docmask->pubsrc, 16);
	}

	return 1;
}

/* if fail, return -1 */
static int set_docattr_function(void *dest, char *key, char *value)
{
	supreme_literature_attr_t *docattr = (supreme_literature_attr_t *)dest;

	if (strcasecmp(key, "Delete") == 0) {
		docattr->is_deleted = 1;
	}
	else if (strcasecmp(key, "TYPE2") == 0) {
		docattr->type2 = 
			(uint8_t)return_constants_value(value, strlen(value));
	}
	else if (strcasecmp(key, "date") == 0) {
		docattr->date = (int32_t)atoi(value);
	}
	else if (strcasecmp(key, "LAN") == 0) {
		docattr->lan = 
			(uint8_t)return_constants_value(value, strlen(value));
	}
	else if (strcasecmp(key, "part") == 0) {
		docattr->part = 
			(uint8_t)return_constants_value(value, strlen(value));
	}
	else if (strcasecmp(key, "title") == 0) {
		/* chinese -> hangul */
		int left;
		char title[SHORT_STRING_SIZE], _value[SHORT_STRING_SIZE];
		WordList wordlist;
		Morpheme morp;
		title[0] = '\0';
		strncpy(_value, value, SHORT_STRING_SIZE-1);
		_value[SHORT_STRING_SIZE-1] = '\0';
		sb_run_morp_set_text(&morp,_value, 4);
		left = SHORT_STRING_SIZE;
		while (sb_run_morp_get_wordlist(&morp,&wordlist,4) != FAIL && 
				left > 0) {
			strncat(title, wordlist.words[1].word, left);
			left -= strlen(wordlist.words[1].word);
		}
		strncpy(docattr->title, title, 16);
/*		strncpy(docattr->title, value, 16);*/
	}
	else if (strcasecmp(key, "author") == 0) {
		/* chinese -> hangul */
		int left;
		char author[SHORT_STRING_SIZE], _value[SHORT_STRING_SIZE];
		WordList wordlist;
		Morpheme morp;
		author[0] = '\0';
		strncpy(_value, value, SHORT_STRING_SIZE-1);
		_value[SHORT_STRING_SIZE-1] = '\0';
		sb_run_morp_set_text(&morp,_value,4);
		left = SHORT_STRING_SIZE;
		while (sb_run_morp_get_wordlist(&morp,&wordlist,4) != FAIL &&
				left > 0) {
			strncat(author, wordlist.words[1].word, left);
			left -= strlen(wordlist.words[1].word);
		}
		strncpy(docattr->author, author, 16);
/*		strncpy(docattr->author, value, 16);*/
	}
	else if (strcasecmp(key, "pubsrc") == 0) {
		/* chinese -> hangul */
		int left;
		char pubsrc[SHORT_STRING_SIZE], _value[SHORT_STRING_SIZE];
		WordList wordlist;
		Morpheme morp;
		pubsrc[0] = '\0';
		strncpy(_value, value, SHORT_STRING_SIZE-1);
		_value[SHORT_STRING_SIZE-1] = '\0';
		sb_run_morp_set_text(&morp,_value,4);
		left = SHORT_STRING_SIZE;
		while (sb_run_morp_get_wordlist(&morp,&wordlist,4) != FAIL &&
				left > 0) {
			strncat(pubsrc, wordlist.words[1].word, left);
			left -= strlen(wordlist.words[1].word);
		}
		strncpy(docattr->pubsrc, pubsrc, 16);
/*		strncpy(docattr->pubsrc, value, 16);*/
	}
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

static int get_docattr_function(void *dest, char *key, char *buf, int buflen)
{
	supreme_literature_attr_t *docattr = (supreme_literature_attr_t *)dest;

	if (strcasecmp(key, "Delete") == 0) {
		snprintf(buf, buflen, "%d", docattr->is_deleted);
	}
	else if (strcasecmp(key, "TYPE2") == 0) {
		snprintf(buf, buflen, "%u",docattr->type2);
	}
	else if (strcasecmp(key, "LAN") == 0) {
		snprintf(buf, buflen, "%u",docattr->lan);
	}
	else if (strcasecmp(key, "part") == 0) {
		snprintf(buf, buflen, "%u",docattr->part);
	}
	else if (strcasecmp(key, "date") == 0) {
		snprintf(buf, buflen, "%u",docattr->date);
	}
	else if (strcasecmp(key, "title") == 0) {
		snprintf(buf, buflen, "%s",docattr->title);
	}
	else if (strcasecmp(key, "author") == 0) {
		snprintf(buf, buflen, "%s",docattr->author);
	}
	else if (strcasecmp(key, "pubsrc") == 0) {
		snprintf(buf, buflen, "%s",docattr->pubsrc);
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
	int n, i;
	char *c, values[3][SHORT_STRING_SIZE];
	supreme_literature_cond_t *doccond = (supreme_literature_cond_t *)dest;

	while ((c = strchr(value, ',')) != NULL) {
			*c = ' ';
	}
	if (strcasecmp(key, "TYPE2") == 0) {
		n = sscanf(value, "%s %s %s", 
				values[0], values[1], values[2]);
		for (i=0; i<n; i++) {
			doccond->type2[i] = 
				return_constants_value(values[i], strlen(values[i]));
		}
		CRIT("type2 compare %d: %d, %d, %d", n, doccond->type2[0], doccond->type2[1], doccond->type2[2]);
		doccond->type2_check = n;
	}
	else if (strcasecmp(key, "date") == 0) {
		if (sscanf(value, "%d-%d", &(doccond->date_start),
					&(doccond->date_finish)) != 2) {
			warn("pronounce date filtering query is wrong");
			return -1;
		}
		doccond->date_check = 1;
	}
	else if (strcasecmp(key, "LAN") == 0) {
		n = sscanf(value, "%s %s %s", 
				values[0], values[1], values[2]);
		for (i=0; i<n; i++) {
			doccond->lan[i] = 
				return_constants_value(values[i], strlen(values[i]));
		}
		CRIT("lan compare %d: %d, %d, %d", n, doccond->lan[0], doccond->lan[1], doccond->lan[2]);
		doccond->lan_check = n;
	}
	else if (strcasecmp(key, "part") == 0) {
		n = sscanf(value, "%s %s %s", 
				values[0], values[1], values[2]);
		for (i=0; i<n; i++) {
			doccond->part[i] = 
				return_constants_value(values[i], strlen(values[i]));
		}
		CRIT("part compare %d: %d, %d, %d", n, doccond->part[0], doccond->part[1], doccond->part[2]);
		doccond->part_check = n;
	}
	else if (strcasecmp(key, "Delete") == 0) {
		doccond->delete_check = 1;
	}
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

static int set_docmask_function(void *dest, char *key, char *value)
{
	/* only use when set delete mark */
	supreme_literature_mask_t *docmask = (supreme_literature_mask_t *)dest;

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
	else if (strcasecmp(key, "TYPE2") == 0) {
		docmask->type2 = 
			(uint8_t)return_constants_value(value, strlen(value));
		docmask->set_type2 = 1;
	}
	else if (strcasecmp(key, "date") == 0) {
		docmask->date = (int32_t)atoi(value);
		docmask->set_date = 1;
	}
	else if (strcasecmp(key, "LAN") == 0) {
		docmask->lan = 
			(uint8_t)return_constants_value(value, strlen(value));
		docmask->set_lan = 1;
	}
	else if (strcasecmp(key, "part") == 0) {
		docmask->part = 
			(uint8_t)return_constants_value(value, strlen(value));
		docmask->set_part = 1;
	}
	else if (strcasecmp(key, "title") == 0) {
		/* chinese -> hangul */
		int left;
		char title[SHORT_STRING_SIZE], _value[SHORT_STRING_SIZE];
		WordList wordlist;
		Morpheme morp;
		title[0] = '\0';
		strncpy(_value, value, SHORT_STRING_SIZE-1);
		_value[SHORT_STRING_SIZE-1] = '\0';
		sb_run_morp_set_text(&morp,_value, 4);
		left = SHORT_STRING_SIZE;
		while (sb_run_morp_get_wordlist(&morp,&wordlist,4) != FAIL && 
				left > 0) {
			strncat(title, wordlist.words[1].word, left);
			left -= strlen(wordlist.words[1].word);
		}
		strncpy(docmask->title, title, 16);
		docmask->set_title = 1;
	}
	else if (strcasecmp(key, "author") == 0) {
		/* chinese -> hangul */
		int left;
		char author[SHORT_STRING_SIZE], _value[SHORT_STRING_SIZE];
		WordList wordlist;
		Morpheme morp;
		author[0] = '\0';
		strncpy(_value, value, SHORT_STRING_SIZE-1);
		_value[SHORT_STRING_SIZE-1] = '\0';
		sb_run_morp_set_text(&morp,_value,4);
		left = SHORT_STRING_SIZE;
		while (sb_run_morp_get_wordlist(&morp,&wordlist,4) != FAIL &&
				left > 0) {
			strncat(author, wordlist.words[1].word, left);
			left -= strlen(wordlist.words[1].word);
		}
		strncpy(docmask->author, author, 16);
		docmask->set_author = 1;
	}
	else if (strcasecmp(key, "pubsrc") == 0) {
		/* chinese -> hangul */
		int left;
		char pubsrc[SHORT_STRING_SIZE], _value[SHORT_STRING_SIZE];
		WordList wordlist;
		Morpheme morp;
		pubsrc[0] = '\0';
		strncpy(_value, value, SHORT_STRING_SIZE-1);
		_value[SHORT_STRING_SIZE-1] = '\0';
		sb_run_morp_set_text(&morp,_value,4);
		left = SHORT_STRING_SIZE;
		while (sb_run_morp_get_wordlist(&morp,&wordlist,4) != FAIL &&
				left > 0) {
			strncat(pubsrc, wordlist.words[1].word, left);
			left -= strlen(wordlist.words[1].word);
		}
		strncpy(docmask->pubsrc, pubsrc, 16);
		docmask->set_pubsrc = 1;
	}
	else if (strcasecmp(key, "ctrlno") == 0) {
		strncpy(docmask->ctrlno, value, 32);
		docmask->set_ctrlno = 1;
	}
	else if (strcasecmp(key, "ctrltype") == 0) {
		strncpy(docmask->ctrltype, value, 32);
		docmask->set_ctrltype = 1;
	}

	return 1;
}
 
/* if fail, return 0 */
static int return_constants_value(char *value, int valuelen)
{
	int i;
	for (i=0; i<MAX_ENUM_NUM && constants[i]; i++) {
		if (strncmp(value, constants[i], valuelen) == 0) {
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

module docattr_supreme_literature_module = {
	STANDARD_MODULE_STUFF,
	config,					/* conf table */
	NULL,					/* registry */
	NULL,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};
