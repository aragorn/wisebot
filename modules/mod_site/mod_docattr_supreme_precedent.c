/* $Id$ */
#include "softbot.h"
#include "mod_api/docattr.h"
#include "mod_api/morpheme.h"
#include "mod_docattr_supreme_precedent.h"
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
	int i,  j;
	supreme_court_attr_t *docattr = (supreme_court_attr_t *)dest;
	supreme_court_cond_t *doccond = (supreme_court_cond_t *)cond;

	/* always check delete mark */
	if (docattr->is_deleted) {
		return 0;
	}

	if (doccond->pronouncedate_check == 1) {
		if (doccond->pronouncedate_start > docattr->pronouncedate ||
				doccond->pronouncedate_finish < docattr->pronouncedate)
			return 0;
	}

	if (doccond->rownum > 0) {
		for (i=0; i<doccond->rownum; i++) {
			
			if (docattr->courttype != doccond->rows[i].courttype) 
				continue;
			
/*			|+ if case grade is total, escape checking +|*/
/*			if (doccond->rows[i].casegrade != 0xFF) */
/*				if (docattr->casegrade != doccond->rows[i].casegrade) */
/*					continue;*/
            if (doccond->rows[i].gan != 0 && docattr->gan != doccond->rows[i].gan)
                continue;
            if (doccond->rows[i].won != 0 && docattr->won != doccond->rows[i].won)
                continue;
            if (doccond->rows[i].del != 0 && docattr->del != doccond->rows[i].del)
                continue;
            if (doccond->rows[i].close != 0 && docattr->close != doccond->rows[i].close)
                continue;

			if (doccond->rows[i].court != 0 && doccond->rows[i].court != 0xFF) {
				if (doccond->rows[i].court % 10000) {
					if (doccond->rows[i].court != docattr->court) {
						continue;
					}
				}
				else {
					if (doccond->rows[i].court > docattr->court || 
							doccond->rows[i].court+10000 <= docattr->court) {
						continue;
					}
				}
			}

			/* if law type is total */
			if (doccond->rows[i].lawtype[0] == 0xFF) 
				return 1;

			for (j=0; j<MAX_LAWTYPE_NUM && j<doccond->rows[i].nlawtype; j++)
				if (docattr->lawtype == doccond->rows[i].lawtype[j])
					return 1;
		}
		return 0;
	}

	return 1;
}

static int compare_function_for_qsort(const void *dest, const void *sour, 
		void *userdata)
{
	int i, diff;
	docattr_sort_t *criterion;
	supreme_court_attr_t attr1, attr2;

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
			case '1': // PronounceDate
				if ((diff = attr1.pronouncedate - attr2.pronouncedate) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
				}
				break;
			case '2': // bubwon simgup
				if ((diff = (int)(attr1.court/10000) - (int)(attr2.court/10000)) 
						== 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
				}
				break;
			case '3': // Court
				if ((diff = attr1.court - attr2.court) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
				}
				break;
			case '4': // CaseNum1
				if ((diff = attr1.casenum1 - attr2.casenum1) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
				}
				break;
			case '5': // CaseNum2
				if ((diff = hangul_strncmp(attr1.casenum2, attr2.casenum2, 16)) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
				}
				break;
			case '6': // CaseNum3
				if ((diff = attr1.casenum3 - attr2.casenum3) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * criterion->keys[i].order;
				}
				break;
			case '7': // CaseName
				if ((diff = hangul_strncmp(attr1.casename, attr2.casename, 16)) == 0) {
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
	supreme_court_attr_t *docattr = (supreme_court_attr_t *)dest;
	supreme_court_mask_t *docmask = (supreme_court_mask_t *)mask;

	if (docmask->delete_mark)
		docattr->is_deleted = 1;

	if (docmask->undelete_mark)
		docattr->is_deleted = 0;

	if (docmask->set_casename) {
		memcpy(docattr->casename, docmask->casename, 16);
	}

	if (docmask->set_courttype) {
		docattr->courttype = docmask->courttype;
	}

	if (docmask->set_lawtype) {
		docattr->lawtype = docmask->lawtype;
	}

	if (docmask->set_court) {
		docattr->court = docmask->court;
	}

    if (docmask->set_gan) {
		docattr->gan = docmask->gan;
    }

    if (docmask->set_won) {
        docattr->won = docmask->won;
    }

    if (docmask->set_del) {
        docattr->del = docmask->del;
    }

    if (docmask->set_close) {
        docattr->close = docmask->close;
    }

	if (docmask->set_pronouncedate) {
		docattr->pronouncedate = docmask->pronouncedate;
	}

	if (docmask->set_casenum1) {
		docattr->casenum1 = docmask->casenum1;
	}

	if (docmask->set_casenum2) {
		strncpy(docattr->casenum2, docmask->casenum2, 16);
	}

	if (docmask->set_casenum3) {
		docattr->casenum3 = docmask->casenum3;
	}
	return 1;
}

/* if fail, return -1 */
static int set_docattr_function(void *dest, char *key, char *value)
{
	supreme_court_attr_t *docattr = (supreme_court_attr_t *)dest;

	if (strcasecmp(key, "Delete") == 0) {
		docattr->is_deleted = 1;
	}
	else if (strcasecmp(key, "CourtType") == 0) {
		docattr->courttype = 
			(uint8_t)return_constants_value(value, strlen(value));
	}
	else if (strcasecmp(key, "LawType") == 0) {
		docattr->lawtype = 
			(uint8_t)return_constants_value(value, strlen(value));
	}
	else if (strcasecmp(key, "CaseName") == 0) {
		/* chinese -> hangul */
		int left;
		char casename[SHORT_STRING_SIZE], _value[SHORT_STRING_SIZE];
		WordList wordlist;
		Morpheme morp;
		casename[0] = '\0';
		strncpy(_value, value, SHORT_STRING_SIZE-1);
		_value[SHORT_STRING_SIZE-1] = '\0';
		sb_run_morp_set_text(&morp,_value,4);
		left = SHORT_STRING_SIZE;
		while (sb_run_morp_get_wordlist(&morp,&wordlist,4) != FAIL &&
				left > 0) {
			strncat(casename, wordlist.words[1].word, left);
			left -= strlen(wordlist.words[1].word);
		}
		strncpy(docattr->casename, casename, 16);
	}
    else if (strcasecmp(key, "GAN") == 0) {
        docattr->gan =
            (uint8_t)return_constants_value(value, strlen(value));
    }
    else if (strcasecmp(key, "WON") == 0) {
        docattr->won =
            (uint8_t)return_constants_value(value, strlen(value));
    }
    else if (strcasecmp(key, "DEL") == 0) {
        docattr->del =
            (uint8_t)return_constants_value(value, strlen(value));
    }
    else if (strcasecmp(key, "CLOSE") == 0) {
        docattr->close =
            (uint8_t)return_constants_value(value, strlen(value));
    }
	else if (strcasecmp(key, "Court") == 0) {
		docattr->court = 
			(uint32_t)return_constants_value(value, strlen(value));
		if (docattr->court == 0) docattr->court = 99999;
	}
	else if (strcasecmp(key, "PronounceDate") == 0) {
		docattr->pronouncedate = (int32_t)atoi(value);
	}
	else if (strcasecmp(key, "CaseNum1") == 0) {
		docattr->casenum1 = (int32_t)atoi(value);
	}
	else if (strcasecmp(key, "CaseNum2") == 0) {
		strncpy(docattr->casenum2, value, 16);
	}
	else if (strcasecmp(key, "CaseNum3") == 0) {
		docattr->casenum3 = (int32_t)atoi(value);
	}
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

static int get_docattr_function(void *dest, char *key, char *buf, int buflen)
{
	supreme_court_attr_t *docattr = (supreme_court_attr_t *)dest;

	if (strcasecmp(key, "CourtType") == 0) {
		snprintf(buf, buflen, "%u",docattr->courttype);
	}
	else if (strcasecmp(key, "LawType") == 0) {
		snprintf(buf, buflen, "%u",docattr->lawtype);
	}
	else if (strcasecmp(key, "CaseName") == 0) {
		snprintf(buf, buflen, "%s",docattr->casename);
	}
    else if (strcasecmp(key, "GAN") == 0) {
        snprintf(buf, buflen, "%u",docattr->gan);
    }
    else if (strcasecmp(key, "WON") == 0) {
        snprintf(buf, buflen, "%u",docattr->won);
    }
    else if (strcasecmp(key, "DEL") == 0) {
        snprintf(buf, buflen, "%u",docattr->del);
    }
    else if (strcasecmp(key, "CLOSE") == 0) {
        snprintf(buf, buflen, "%u",docattr->close);
    }
	else if (strcasecmp(key, "Court") == 0) {
		snprintf(buf, buflen, "%u",docattr->court);
	}
	else if (strcasecmp(key, "PronounceDate") == 0) {
		snprintf(buf, buflen, "%u",docattr->pronouncedate);
	}
	else if (strcasecmp(key, "CaseNum1") == 0) {
		snprintf(buf, buflen, "%d",docattr->casenum1);
	}
	else if (strcasecmp(key, "CaseNum2") == 0) {
		snprintf(buf, buflen, "%s",docattr->casenum2);
	}
	else if (strcasecmp(key, "CaseNum3") == 0) {
		snprintf(buf, buflen, "%d",docattr->casenum3);
	}
	else if (strcasecmp(key, "Delete") == 0) {
		snprintf(buf, buflen, "%d",docattr->is_deleted);
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
	supreme_court_cond_t *doccond = (supreme_court_cond_t *)dest;
	char *field, tmpkey[STRING_SIZE];
	int row=0;

	strncpy(tmpkey, key, STRING_SIZE);
	tmpkey[STRING_SIZE-1] = '\0';
	key = tmpkey;

	if (strcmp(key, "Delete") == 0) {
		doccond->delete_check = 1;
		return 1;
	}

	INFO("key:%s, value:%s",key,value);

	field = strchr(key, ':');
	if (field == NULL) {
		if (strcmp(key, "PronounceDate") == 0) {
			if (sscanf(value, "%d-%d", &(doccond->pronouncedate_start),
						&(doccond->pronouncedate_finish)) != 2) {
				warn("pronounce date filtering query is wrong");
				return -1;
			}
			doccond->pronouncedate_check = 1;
			return 1;
		}
	}
	else {
		*field = '\0';
		row = atoi(key);
		key = ++field;
	}

	if (row > MAX_CATEGORY_ROWS) {
		error("row(%d) > MAX_CATEGORY_ROWS(%d)", row, MAX_CATEGORY_ROWS);
		return -1;
	}

	if (strcmp(key, "CourtType") == 0) {
		doccond->rows[row-1].courttype = 
			return_constants_value(value, strlen(value));
	}
	else if (strcmp(key, "LawType") == 0) {
		doccond->rows[row-1].lawtype[doccond->rows[row-1].nlawtype++] = 
			return_constants_value(value, strlen(value));
	}
/*	else if (strcmp(key, "CaseGrade") == 0) {*/
/*		doccond->rows[row-1].casegrade = */
/*			return_constants_value(value, strlen(value));*/
/*	}*/
    else if (strcmp(key, "GAN") == 0) {
        doccond->rows[row-1].gan =
            return_constants_value(value, strlen(value));
//		INFO("set cond if key[%s]: %d(%s)", key, doccond->rows[row-1].gan, value);
    }
    else if (strcmp(key, "WON") == 0) {
        doccond->rows[row-1].won =
            return_constants_value(value, strlen(value));
    }
    else if (strcmp(key, "DEL") == 0) {
        doccond->rows[row-1].del =
            return_constants_value(value, strlen(value));
    }
    else if (strcmp(key, "CLOSE") == 0) {
        doccond->rows[row-1].close =
            return_constants_value(value, strlen(value));
    }
	else if (strcmp(key, "Court") == 0) {
		doccond->rows[row-1].court = 
			return_constants_value(value, strlen(value));
	}
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	if (doccond->rownum < row) doccond->rownum = row;

#ifdef DEBUG_SOFTBOTD
    { int i;
    for (i=0; i<doccond->rownum; i++) {
        CRIT("[%d]CourtType: %d, Gan: %d Won: %d Del: %d Close: %d", doccond->rownum, doccond->rows[i].courttype,
              doccond->rows[i].gan, doccond->rows[i].won, doccond->rows[i].del, doccond->rows[i].close);
        CRIT("\t[%d]LawType:%d", doccond->rows[i].nlawtype, doccond->rows[i].lawtype[0]);
    }
    }
#endif
	return 1;
}

static int set_docmask_function(void *dest, char *key, char *value)
{
	/* only use when set delete mark */
	supreme_court_mask_t *docmask = (supreme_court_mask_t *)dest;
	if (strcmp(key, "Delete") == 0) {
		docmask->delete_mark = 1;
	}
    else if (strcmp(key, "Undelete") == 0) {
        docmask->undelete_mark = 1;
    }
	else if (strcasecmp(key, "CourtType") == 0) {
		docmask->courttype = 
			(uint8_t)return_constants_value(value, strlen(value));
		docmask->set_courttype = 1;
	}
	else if (strcasecmp(key, "LawType") == 0) {
		docmask->lawtype = 
			(uint8_t)return_constants_value(value, strlen(value));
		docmask->set_lawtype = 1;
	}
    else if (strcasecmp(key, "CaseName") == 0) {
        /* chinese -> hangul */
        int left;
        char casename[SHORT_STRING_SIZE], _value[SHORT_STRING_SIZE];
        WordList wordlist;
        Morpheme morp;
        casename[0] = '\0';
        strncpy(_value, value, SHORT_STRING_SIZE-1);
        _value[SHORT_STRING_SIZE-1] = '\0';
        sb_run_morp_set_text(&morp,_value,4);
        left = SHORT_STRING_SIZE;
        while (sb_run_morp_get_wordlist(&morp,&wordlist,4) != FAIL &&
                left > 0) {
            strncat(casename, wordlist.words[1].word, left);
            left -= strlen(wordlist.words[1].word);
        }
        strncpy(docmask->casename, casename, 16);

        docmask->set_casename = 1;
    }
	else if (strcasecmp(key, "Court") == 0) {
		docmask->court = 
			(uint32_t)return_constants_value(value, strlen(value));
		if (docmask->court == 0) docmask->court = 99999;

		docmask->set_court = 1;
	}
    else if (strcasecmp(key, "GAN") == 0) {
        docmask->gan =
            (uint8_t)return_constants_value(value, strlen(value));
		docmask->set_gan = 1;
    }
    else if (strcasecmp(key, "WON") == 0) {
        docmask->won =
            (uint8_t)return_constants_value(value, strlen(value));
        docmask->set_won = 1;
    }
    else if (strcasecmp(key, "DEL") == 0) {
        docmask->del =
            (uint8_t)return_constants_value(value, strlen(value));
        docmask->set_del = 1;
    }
    else if (strcasecmp(key, "CLOSE") == 0) {
        docmask->close =
            (uint8_t)return_constants_value(value, strlen(value));
        docmask->set_close = 1;
    }
	else if (strcasecmp(key, "PronounceDate") == 0) {
		docmask->pronouncedate = (int32_t)atoi(value);
		docmask->set_pronouncedate = 1;
	}
	else if (strcasecmp(key, "CaseNum1") == 0) {
		docmask->casenum1 = (int32_t)atoi(value);
		docmask->set_casenum1 = 1;
	}
	else if (strcasecmp(key, "CaseNum2") == 0) {
		strncpy(docmask->casenum2, value, 16);
		docmask->set_casenum2 = 1;
	}
	else if (strcasecmp(key, "CaseNum3") == 0) {
		docmask->casenum3 = (int32_t)atoi(value);
		docmask->set_casenum3 = 1;
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

module docattr_supreme_precedent_module = {
	STANDARD_MODULE_STUFF,
	config,					/* conf table */
	NULL,					/* registry */
	NULL,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};
