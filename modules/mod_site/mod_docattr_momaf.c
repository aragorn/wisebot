/* $Id$ */
#include "softbot.h"
#include "mod_api/docattr.h"
#include "mod_api/index_word_extractor.h"
#include "mod_docattr_momaf.h"
#include "mod_qp/mod_qp.h"
//#include "mod_docattr/mod_docattr.h"
//#include "mod_vrm/vrm.h"

#include <stdio.h>

#define MAX_ENUM_NUM		1024
#define MAX_ENUM_LEN		SHORT_STRING_SIZE

static char *constants[MAX_ENUM_NUM] = { NULL };
static long long constants_value[MAX_ENUM_NUM];

static long long return_constants_value(char *value, int valuelen);

static int compare_function(void *dest, void *cond, uint32_t docid) {

	momaf_attr_t *docattr = (momaf_attr_t*)dest;
	momaf_cond_t *doccond = (momaf_cond_t*)cond;


	/* always check delete mark */
	if (docattr->is_deleted) {
		return 0;
	}

	if (doccond->Date_check == 1) {
		if (doccond->Date_start > docattr->Date ||
			doccond->Date_finish < docattr->Date)
			return 0;
	}


    if (doccond->FD11_check == 1 && doccond->FD11 != docattr->FD11) {
            return 0;
    }

	return 1;
}

static int compare_function_for_qsort(const void *dest, const void *sour, void *userdata)
{
	int i, diff;
	docattr_sort_t *sh;
	momaf_attr_t attr1, attr2;
	int hit1=0, hit2=0;

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
			case '0': // HIT
				hit1 = ((doc_hit_t *)dest)->hitratio;
				hit2 = ((doc_hit_t *)sour)->hitratio;
				
				if ((diff = hit1- hit2) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '1': // Title
				if ((diff = hangul_strncmp(attr1.Title, attr2.Title, 16)) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '2': // Dept
				if ((diff = hangul_strncmp(attr1.Dept, attr2.Dept, 16)) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '3': // Author
				if ((diff = hangul_strncmp(attr1.Author, attr2.Author, 16)) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '4': // Date
				if ((diff = attr1.Date - attr2.Date) == 0) {
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
	momaf_attr_t *docattr = (momaf_attr_t *)dest;
	momaf_mask_t *docmask = (momaf_mask_t *)mask;
	

	if (docmask->delete_mark)
		docattr->is_deleted = 1;

	if (docmask->undelete_mark)
		docattr->is_deleted = 0;

	if (docmask->set_Date)
		docattr->Date = docmask->Date;

	if (docmask->set_FD11)
		docattr->FD11 = docmask->FD11;
	
	if (docmask->set_Title)
		memcpy(docattr->Title, docmask->Title, 16);
		
	if (docmask->set_Dept)
		memcpy(docattr->Dept, docmask->Dept, 16);

	if (docmask->set_Author)
		memcpy(docattr->Author, docmask->Author, 16);
		
	return 1;
}

/* if fail, return -1 : 등록시 doc에 대한 docattr 설정 */
static int set_docattr_function(void *dest, char *key, char *value)
{
	momaf_attr_t *docattr = (momaf_attr_t *)dest;
	
    debug("key[%s] : value[%s]", key, value);

	if (strcasecmp(key, "Delete") == 0) {
		docattr->is_deleted = 1;
	}
	else if (strcasecmp(key, "Date") == 0) {
		docattr->Date = (uint32_t)atol(value);
	}
	else if (strcasecmp(key, "FD11") == 0) {
		docattr->FD11 = (uint8_t)return_constants_value(value, strlen(value));
	}
	else if (strcasecmp(key, "Title") == 0) {
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
		strncpy(docattr->Title, word.word, 16);
	}
	else if (strcasecmp(key, "Dept") == 0) {
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
		strncpy(docattr->Dept, word.word, 16);
	}
	else if (strcmp(key, "Author") == 0) {  
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
		strncpy(docattr->Author, word.word, 16);
	}
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

static int get_docattr_function(void *dest, char *key, char *buf, int buflen)
{
	momaf_attr_t *docattr = (momaf_attr_t *)dest;

	if (strcasecmp(key, "Delete") == 0) {
		snprintf(buf, buflen, "%u",docattr->is_deleted);
	}
	else if (strcasecmp(key, "Date") == 0) {
		snprintf(buf, buflen, "%u",docattr->Date);
	}
	else if (strcasecmp(key, "FD11") == 0) {
		snprintf(buf, buflen, "%u",docattr->FD11);
	}
	else if (strcasecmp(key, "Title") == 0) {
		snprintf(buf, buflen, "%s",docattr->Title);
	}
	else if (strcasecmp(key, "Dept") == 0) {
		snprintf(buf, buflen, "%s",docattr->Dept);
	}
	else if (strcasecmp(key, "Author") == 0) {
		snprintf(buf, buflen, "%s",docattr->Author);
	}
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}
	return 1;
}

/* if fail, return -1 : 검색 조건 set */
static int set_doccond_function(void *dest, char *key, char *value)
{
	momaf_cond_t *doccond = (momaf_cond_t *)dest;
	int n=0;

	if (strcmp(key, "Delete") == 0) {
		doccond->delete_check = 1;
		return 1;
	}

	INFO("key:%s, value:%s",key,value);

	if (strcmp(key, "Date") == 0) {
			n = sscanf(value, "%u-%u", &(doccond->Date_start), &(doccond->Date_finish));
			if (n != 2) {
				warn("wrong docattr query: Date");
				return -1;
			}
	        doccond->Date_check = 1;
	}
    else if (strcmp(key, "FD11") == 0) {
            doccond->FD11 = (uint8_t)return_constants_value(value, strlen(value));
            doccond->FD11_check = 1;
    }
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

static int set_docmask_function(void *dest, char *key, char *value)
{
	momaf_mask_t *docmask = (momaf_mask_t *)dest;

	if (strcmp(key, "Delete") == 0) {
		docmask->delete_mark = 1;
	}
   	else if (strcmp(key, "Undelete") == 0) {
       	docmask->undelete_mark = 1;
   	}
	else if (strcasecmp(key, "Date") == 0) {
		docmask->Date = (uint32_t)atol(value);
		docmask->set_Date = 1;
	}
	else if (strcasecmp(key, "FD11") == 0) {
		docmask->FD11 = (uint8_t)return_constants_value(value, strlen(value));
		docmask->set_FD11 = 1;
	}
	else if (strcasecmp(key, "Title") == 0) {
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
		strncpy(docmask->Title, word.word, 16);
		docmask->set_Title = 1;
	}
	else if (strcasecmp(key, "Dept") == 0) {
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
		strncpy(docmask->Dept, word.word, 16);
		docmask->set_Dept = 1;
	}
	else if (strcasecmp(key, "Author") == 0) {
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
		strncpy(docmask->Author, word.word, 16);
		docmask->set_Author = 1;
	}
	else {
		warn("there is no such a field[%s]", key);
	}
	return 1;
}
 
 /* src : 123,23,45 
    int return : count
    info : 123 
    delimiter: split(,) */
 
/* if fail, return 0 */
static long long return_constants_value(char *value, int valuelen)
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

    /* 세번째 인자 base(진법?) 가 0이면, 문자열의 처음 2글자에 따라 
     * base가 결정된다.
     * "0x1234..." -> 16진수
     * "01234..."  -> 8진수
     * "1234..."   -> 10진수
     */ 
	constants_value[i] = atoi(v.argument[1]);

	INFO("Enum[%s]: %lld", constants[i], constants_value[i]);
}

static config_t config[] = {
	CONFIG_GET("Enum", get_enum, 2, "constant"),
	{NULL}
};

static void register_hooks(void)
{
	sb_hook_docattr_compare_function(compare_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_mask_function(mask_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_sort_function(compare_function_for_qsort, NULL, NULL, HOOK_MIDDLE);
//	sb_hook_docattr_hit_sort_function(compare_hit_for_qsort, NULL, NULL, HOOK_MIDDLE);

	sb_hook_docattr_set_docattr_function(set_docattr_function,
			NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_get_docattr_function(get_docattr_function,
			NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_set_doccond_function(set_doccond_function,
			NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_set_docmask_function(set_docmask_function,
			NULL, NULL, HOOK_MIDDLE);
}

module docattr_momaf_module = {
	STANDARD_MODULE_STUFF,
	config,					/* conf table */
	NULL,					/* registry */
	NULL,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};
