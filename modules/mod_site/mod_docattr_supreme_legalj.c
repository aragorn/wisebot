/* $Id$ */
#include "common_core.h"
#include "common_util.h"
#include "mod_api/docattr.h"
#include "mod_api/qp.h"
#include "mod_api/index_word_extractor.h"
#include "mod_docattr_supreme_legalj.h"
#include "mod_qp/mod_qp.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h> // atoi

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
	supreme_legalj_attr_t *docattr = (supreme_legalj_attr_t *)dest;
	supreme_legalj_cond_t *doccond = (supreme_legalj_cond_t *)cond;

	/* always check delete mark */
	if (docattr->is_deleted) {
		return 0;
	}

    if (doccond->casekind_check > 0 && doccond->casekind[0] != 255) {
        for (i=0; i<doccond->casekind_check; i++) {
            if (doccond->casekind[i] == docattr->casekind) break;
        }
        if (i == doccond->casekind_check) return 0;
    }
    if (doccond->partkind_check > 0 && doccond->partkind[0] != 255) {
        for (i=0; i<doccond->partkind_check; i++) {
			/* 전속 = 10
			 * 공동 = 20
			 * 민사 = 21, 형사 = 22, 행정 = 23, ...
			 * 질의어에 "공동"이 입력되면, 민사, 형사, 행정 등이 검색되어야 한다. */
			if (doccond->partkind[i] % 10 == 0
				&& (int)(doccond->partkind[i] / 10) == (int)(docattr->partkind / 10)) break;
        }
        if (i == doccond->partkind_check) return 0;
    }
	if (doccond->reportdate_check == 1) {
		if (doccond->reportdate_start > docattr->reportdate ||
				doccond->reportdate_finish < docattr->reportdate)
			return 0;
	}
    if (doccond->reportgrade_check > 0 && doccond->reportgrade[0] != 255) {
        for (i=0; i<doccond->reportgrade_check; i++) {
            if (doccond->reportgrade[i] == docattr->reportgrade) break;
        }
        if (i == doccond->reportgrade_check) return 0;
    }

	return 1;
}

static int compare_function_for_qsort(const void *dest, const void *sour, 
		void *userdata)
{
	int i, diff;
	docattr_sort_t *criterion;
	supreme_legalj_attr_t attr1, attr2;

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
			case '1': // casenum1
				if ((diff = attr1.casenum1 - attr2.casenum1) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
				}
				break;
			case '2': // casenum2
				if ((diff = hangul_strncmp(attr1.casenum2, attr2.casenum2, DOCATTR_CASENUM2_LEN)) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
				}
				break;
			case '3': // casenum3
				if ((diff = attr1.casenum3 - attr2.casenum3) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
				}
				break;
			case '4': // casename
				if ((diff = hangul_strncmp(attr1.casename, attr2.casename, DOCATTR_CASENAME_LEN)) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
				}
				break;
			case '5': // reportdate
				if ((diff = attr1.reportdate - attr2.reportdate) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
				}
				break;
			case '6': // partkind
				if ((diff = attr1.partkind - attr2.partkind) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
				}
				break;
			case '7': // name
				if ((diff = hangul_strncmp(attr1.name, attr2.name, DOCATTR_NAME_LEN)) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
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
	supreme_legalj_attr_t *docattr = (supreme_legalj_attr_t *)dest;
	supreme_legalj_mask_t *docmask = (supreme_legalj_mask_t *)mask;

	if (docmask->delete_mark)
		docattr->is_deleted = 1;

	if (docmask->undelete_mark)
		docattr->is_deleted = 0;

	if (docmask->set_casekind) {
		docattr->casekind = docmask->casekind;
	}
	if (docmask->set_partkind) {
		docattr->partkind = docmask->partkind;
	}
	if (docmask->set_reportdate) {
		docattr->reportdate = docmask->reportdate;
	}
	if (docmask->set_reportgrade) {
		docattr->reportgrade = docmask->reportgrade;
	}
	if (docmask->set_casename) {
		memcpy(docattr->casename, docmask->casename, DOCATTR_CASENAME_LEN);
	}
	if (docmask->set_casenum) {
		docattr->casenum1 = docmask->casenum1;
		memcpy(docattr->casenum2, docmask->casenum2, DOCATTR_CASENUM2_LEN);
		docattr->casenum3 = docmask->casenum3;
	}
	if (docmask->set_name) {
		memcpy(docattr->name, docmask->name, DOCATTR_NAME_LEN);
	}

	return 1;
}

/* if fail, return -1 */
static int set_docattr_function(void *dest, char *key, char *value)
{
	supreme_legalj_attr_t *docattr = (supreme_legalj_attr_t *)dest;

	if (strcasecmp(key, "delete") == 0) {
		docattr->is_deleted = 1;
	}
	else if (strcasecmp(key, "casekind") == 0) {
		docattr->casekind = 
			(uint8_t)return_constants_value(value, strlen(value));
	}
	else if (strcasecmp(key, "partkind") == 0) {
		docattr->partkind = 
			(uint8_t)return_constants_value(value, strlen(value));
	}
	else if (strcasecmp(key, "reportdate") == 0) {
		docattr->reportdate = (int32_t)atoi(value); 
	}
	else if (strcasecmp(key, "reportgrade") == 0) {
		docattr->reportgrade = 
			(uint8_t)return_constants_value(value, strlen(value));
	}
	else if (strcasecmp(key, "casename") == 0) {
        /* chinese -> hangul */
        char my_value[SHORT_STRING_SIZE];
    
        index_word_extractor_t *ex = NULL;
        index_word_t word;
        
        strncpy(my_value, value, SHORT_STRING_SIZE-1);
        my_value[SHORT_STRING_SIZE-1] = '\0';
        
        ex = sb_run_new_index_word_extractor(2);
		if (ex == NULL || ex == (index_word_extractor_t*)MINUS_DECLINE) return FAIL;
        sb_run_index_word_extractor_set_text(ex, my_value);

        sb_run_get_index_words(ex, &word, DOCATTR_CASENAME_LEN);
        strncpy(docattr->casename, word.word, DOCATTR_CASENAME_LEN);
	}
	else if (strcasecmp(key, "casenum") == 0) {
		int i, j;
		char buf[SHORT_STRING_SIZE];

		/* ex) 97누16459 */

		/* 1. copy string while isdigit() */
		for(i=0, j=0; i < strlen(value); i++, j++)
		{
			if (isdigit(value[i])) buf[j] = value[j];
			else break;
		}
		buf[j] = '\0';

		docattr->casenum1 = (int32_t)atoi(buf);

		/* 2. copy string until isdigit() */
		for(j=0; i < strlen(value); i++, j++)
		{
			if (isdigit(value[i])) break;
			else buf[j] = value[j];
		}
		buf[j] = '\0';

		strncpy(docattr->casenum2, buf, DOCATTR_CASENUM2_LEN);

		/* 3. copy string from value[i] */
		docattr->casenum3 = (int32_t)atoi(&value[i]);
	}
	else if (strcasecmp(key, "name") == 0) {
		strncpy(docattr->name, value, DOCATTR_NAME_LEN);
	}
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

static int get_docattr_function(void *dest, char *key, char *buf, int buflen)
{
	supreme_legalj_attr_t *docattr = (supreme_legalj_attr_t *)dest;

	if (strcasecmp(key, "delete") == 0) {
		snprintf(buf, buflen, "%d",docattr->is_deleted);
	} else if (strcasecmp(key, "casekind") == 0) {
		snprintf(buf, buflen, "%u",docattr->casekind);
	} else if (strcasecmp(key, "partkind") == 0) {
		snprintf(buf, buflen, "%u",docattr->partkind);
	} else if (strcasecmp(key, "reportdate") == 0) {
		snprintf(buf, buflen, "%u",docattr->reportdate);
	} else if (strcasecmp(key, "reportgrade") == 0) {
		snprintf(buf, buflen, "%u",docattr->reportgrade);
	} else if (strcasecmp(key, "casename") == 0) {
		snprintf(buf, buflen, "%s",docattr->casename);
	} else if (strcasecmp(key, "casenum1") == 0) {
		snprintf(buf, buflen, "%u",docattr->casenum1);
	} else if (strcasecmp(key, "casenum2") == 0) {
		snprintf(buf, buflen, "%s",docattr->casenum2);
	} else if (strcasecmp(key, "casenum3") == 0) {
		snprintf(buf, buflen, "%u",docattr->casenum3);
	} else if (strcasecmp(key, "name") == 0) {
		snprintf(buf, buflen, "%s",docattr->name);
	} else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

/* if fail, return -1 */
static int set_doccond_function(void *dest, char *key, char *value)
{
	supreme_legalj_cond_t *doccond = (supreme_legalj_cond_t *)dest;
	char *c, values[DOCATTR_COND_ROW_NUM][SHORT_STRING_SIZE];
	int i, n = 0;

    if (strcasecmp(key, "delete") == 0) {
        doccond->delete_check = 1;
        return 1;
    }

    while ((c = strchr(value, ',')) != NULL) {
        *c = ' ';
    }

	INFO("key:%s, value:%s",key,value);

    if (strcasecmp(key, "casekind") == 0) {
        n = sscanf(value, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
                values[0], values[1], values[2],
                values[3], values[4], values[5],
                values[6], values[7], values[8],
                values[9], values[10], values[11],
                values[12], values[13], values[14]);
        for (i=0; i<n; i++) {
            doccond->casekind[i] =
                return_constants_value(values[i], strlen(values[i]));
        }
        doccond->casekind_check = n;
    } else if (strcasecmp(key, "partkind") == 0) {
        n = sscanf(value, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
                values[0], values[1], values[2],
                values[3], values[4], values[5],
                values[6], values[7], values[8],
                values[9], values[10], values[11],
                values[12], values[13], values[14]);
        for (i=0; i<n; i++) {
            doccond->partkind[i] =
                return_constants_value(values[i], strlen(values[i]));
        }
        doccond->partkind_check = n;
    } else if (strcasecmp(key, "reportdate") == 0) {
        n = sscanf(value, "%u-%u", &(doccond->reportdate_start), &(doccond->reportdate_finish));
        if (n != 2) {
            warn("wrong docattr query: reportdate");
            return -1;
        }
        doccond->reportdate_check = 1;
    } else if (strcasecmp(key, "reportgrade") == 0) {
        n = sscanf(value, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
                values[0], values[1], values[2],
                values[3], values[4], values[5],
                values[6], values[7], values[8],
                values[9], values[10], values[11],
                values[12], values[13], values[14]);
        for (i=0; i<n; i++) {
            doccond->reportgrade[i] =
                return_constants_value(values[i], strlen(values[i]));
        }
        doccond->reportgrade_check = n;
    } else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

static int set_docmask_function(void *dest, char *key, char *value)
{
	/* only use when set delete mark */
	supreme_legalj_mask_t *docmask = (supreme_legalj_mask_t *)dest;
	if (strcasecmp(key, "delete") == 0) {
		docmask->delete_mark = 1;
	}
    else if (strcasecmp(key, "undelete") == 0) {
        docmask->undelete_mark = 1;
    }
	else if (strcasecmp(key, "casekind") == 0) {
		docmask->casekind = 
			(uint8_t)return_constants_value(value, strlen(value));
		docmask->set_casekind = 1;
	}
	else if (strcasecmp(key, "partkind") == 0) {
		docmask->partkind = 
			(uint8_t)return_constants_value(value, strlen(value));
		docmask->set_partkind = 1;
	}
	else if (strcasecmp(key, "reportdate") == 0) {
		docmask->reportdate = (int32_t)atoi(value);
		docmask->set_reportdate = 1;
	}
	else if (strcasecmp(key, "reportgrade") == 0) {
		docmask->reportgrade = 
			(uint8_t)return_constants_value(value, strlen(value));
		docmask->set_reportgrade = 1;
	}
    else if (strcasecmp(key, "casename") == 0) {
        /* chinese -> hangul */
        char my_value[SHORT_STRING_SIZE];
    
        index_word_extractor_t *ex = NULL;
        index_word_t word;
        
        strncpy(my_value, value, SHORT_STRING_SIZE-1);
        my_value[SHORT_STRING_SIZE-1] = '\0';
        
        ex = sb_run_new_index_word_extractor(2);
		if (ex == NULL || ex == (index_word_extractor_t*)MINUS_DECLINE) return FAIL;
        sb_run_index_word_extractor_set_text(ex, my_value);

        sb_run_get_index_words(ex, &word, DOCATTR_CASENAME_LEN);
        strncpy(docmask->casename, word.word, DOCATTR_CASENAME_LEN);
        docmask->set_casename = 1;
    }
	else if (strcasecmp(key, "casenum") == 0) {
		int i, j;
		char buf[SHORT_STRING_SIZE];

		/* ex) 97누16459 */

		/* 1. copy string while isdigit() */
		for(i=0, j=0; i < strlen(value); i++, j++)
		{
			if (isdigit(value[i])) buf[j] = value[j];
			else break;
		}
		buf[j] = '\0';

		docmask->set_casenum = 1;
		docmask->casenum1 = (int32_t)atoi(buf);

		/* 2. copy string until isdigit() */
		for(j=0; i < strlen(value); i++, j++)
		{
			if (isdigit(value[i])) break;
			else buf[j] = value[j];
		}
		buf[j] = '\0';

		strncpy(docmask->casenum2, buf, DOCATTR_CASENUM2_LEN);

		/* 3. copy string from value[i] */
		docmask->casenum3 = (int32_t)atoi(&value[i]);
	}
	else if (strcasecmp(key, "name") == 0) {
		strncpy(docmask->name, value, DOCATTR_NAME_LEN);
		docmask->set_name = 1;
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
		if (strncasecmp(value, constants[i], MAX_ENUM_LEN) == 0) {
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
		error("too many constants are defined");
		return;
	}
	strncpy(enums[i], v.argument[0], MAX_ENUM_LEN);
	enums[i][MAX_ENUM_LEN-1] = '\0';
	constants[i] = enums[i];
	constants_value[i] = atoi(v.argument[1]);
/*	INFO("Enum[%s]: %d", constants[i], constants_value[i]);*/
}

static int init(void)
{
	sb_assert(sizeof(supreme_legalj_attr_t) == 64);

	return SUCCESS;
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

module docattr_supreme_legalj_module = {
	STANDARD_MODULE_STUFF,
	config,					/* conf table */
	NULL,					/* registry */
	init,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};
