#include "softbot.h"
#include "mod_api/mod_api.h"
#include "mod_api/index_word_extractor.h"
#include "mod_docattr_supreme_home.h"
#include "mod_qp/mod_qp.h"

#include <stdio.h>

#define MAX_ENUM_NUM		1024
#define MAX_ENUM_LEN		SHORT_STRING_SIZE

static char *constants[MAX_ENUM_NUM] = { NULL };
static long long constants_value[MAX_ENUM_NUM];

static int system_limit[MAX_SYSTEM] = { -1 };

static void init_system_limit()
{
	int i;

	if ( system_limit[0] != -1 ) return;

	for ( i = 0; i < MAX_SYSTEM; i++ )
		system_limit[i] = 20;
}

static long long return_constants_value(char *value, int valuelen);

static int compare_function(void *dest, void *cond, uint32_t docid)
{
	supreme_home_attr_t *docattr = (supreme_home_attr_t*)dest;
	supreme_home_cond_t *doccond = (supreme_home_cond_t*)cond;

	/* always check delete mark */
	if (docattr->is_deleted) {
		return 0;
	}

	if (doccond->Date_check == 1) {
		if (doccond->Date_start > docattr->Date ||
			doccond->Date_finish < docattr->Date)
			return 0;
	}


    if (doccond->System_check == 1 && doccond->System != docattr->System) {
            return 0;
    }

	if (doccond->Type_check == 1) {
		// Type:1 - 문서만, Type:2 - 첨부파일만
		if ( doccond->Type == 1 && docattr->FileNo > 0 ) return 0;
		else if ( doccond->Type == 2 && docattr->FileNo == 0 ) return 0;
	}

	return 1;
}

static int compare2_function(void *dest, void *cond, uint32_t docid)
{
	supreme_home_attr_t *docattr = (supreme_home_attr_t*)dest;
	supreme_home_cond_t *doccond = (supreme_home_cond_t*)cond;

	if ( doccond->SystemSum_check == 1 ) {
		if ( docattr->System < 1 || docattr->System >= MAX_SYSTEM ) return 0;

		// ugly hack...
		if ( docattr->System == 5 && docattr->FileNo == 0 ) return 0; // 양식은 파일만..

		if ( doccond->SystemSum[docattr->System] >= system_limit[docattr->System] ) return 0;
		else {
			doccond->SystemSum[docattr->System]++;
			return 1;
		}
	}

	return 1;
}

static int compare_function_for_qsort(const void *dest, const void *sour, void *userdata)
{
	int i, diff;
	docattr_sort_t *sh;
	supreme_home_attr_t attr1, attr2;
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
			case '2': // Author
				if ((diff = hangul_strncmp(attr1.Author, attr2.Author, 16)) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '3': // Date
				if ((diff = attr1.Date - attr2.Date) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '4': // System
				if ((diff = attr1.System - attr2.System) == 0) {
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
static int compare_rid(const void *dest, const void *sour)
{
    unsigned long long l1, l2;
    supreme_home_attr_t attr1, attr2;

    if (sb_run_docattr_get(((doc_hit_t *)dest)->id, &attr1) < 0) {
        error("cannot get docattr element");
        return 0;
    }  

    if (sb_run_docattr_get(((doc_hit_t *)sour)->id, &attr2) < 0) {
        error("cannot get docattr element");
        return 0;
    }
    
    l1 = *((unsigned long long*)attr1.Rid);
    l2 = *((unsigned long long*)attr2.Rid);

    if (l1 > l2) return 1;
    else if (l1 == l2) return 0;
    else return -1;
}

static int docattr_distinct_rid(int id, index_list_t *list)
{
    int i, abst;
    supreme_home_attr_t attr1, attr2;
    unsigned long long rid1, rid2;

    // default filtering id 
//  if (id != 0) return DECLINE;
    qsort(list->doc_hits, list->ndochits, sizeof(doc_hit_t), compare_rid);

    abst = 0;

    if (sb_run_docattr_get(list->doc_hits[0].id, &attr1) < 0) {
        error("cannot get docattr element");
        return -1;
    }
    rid2 = *((unsigned long long*)attr1.Rid);

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
            rid2 = *((unsigned long long*)attr2.Rid);

            if (rid1 == 0 || rid2 == 0) break;
            if (rid1 != rid2) break;
        }

        abst++;
    }

    list->ndochits = abst;

    return SUCCESS;
}
*/

/*
 * NOTICE:
 * 	if success, return SUCCESS(1)
 */
static int mask_function(void *dest, void *mask) {
	supreme_home_attr_t *docattr = (supreme_home_attr_t *)dest;
	supreme_home_mask_t *docmask = (supreme_home_mask_t *)mask;
	

	if (docmask->delete_mark)
		docattr->is_deleted = 1;

	if (docmask->undelete_mark)
		docattr->is_deleted = 0;

	if (docmask->set_Rid) {
		MD5_CTX context;
		unsigned char digest[16];
		unsigned int len = strlen(docmask->Rid);

		MD5Init(&context);
		MD5Update(&context, docmask->Rid, len);
		MD5Final(digest, &context);

		memcpy(docattr->Rid, digest, DOCATTR_RID_LEN);
	}

	if (docmask->set_Date)
		docattr->Date = docmask->Date;

	if (docmask->set_System)
		docattr->System = docmask->System;
	
	if (docmask->set_Title)
		memcpy(docattr->Title, docmask->Title, 16);
		
	if (docmask->set_Author)
		memcpy(docattr->Author, docmask->Author, 16);

	if (docmask->set_FileNo)
		docattr->FileNo = docmask->FileNo;
		
	return 1;
}

/* if fail, return -1 : 등록시 doc에 대한 docattr 설정 */
static int set_docattr_function(void *dest, char *key, char *value)
{
	supreme_home_attr_t *docattr = (supreme_home_attr_t *)dest;
	
    debug("key[%s] : value[%s]", key, value);

	if (strcasecmp(key, "Delete") == 0) {
		docattr->is_deleted = 1;
	}
	else if (strcasecmp(key, "Date") == 0) {
		docattr->Date = (uint32_t)atol(value);
	}
	else if (strcasecmp(key, "System") == 0) {
		docattr->System = (uint32_t) return_constants_value( value, strlen(value) );
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
		strncpy(docattr->Author, word.word, 16);
	}
	else if (strcasecmp(key, "FileNo") == 0) {
		docattr->FileNo = (uint32_t)atoi(value);
	}
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

static int get_docattr_function(void *dest, char *key, char *buf, int buflen)
{
	supreme_home_attr_t *docattr = (supreme_home_attr_t *)dest;

	if (strcasecmp(key, "Delete") == 0) {
		snprintf(buf, buflen, "%u",docattr->is_deleted);
	}
	else if (strcasecmp(key, "Date") == 0) {
		snprintf(buf, buflen, "%u",docattr->Date);
	}
	else if (strcasecmp(key, "System") == 0) {
		snprintf(buf, buflen, "%u",docattr->System);
	}
	else if (strcasecmp(key, "Title") == 0) {
		snprintf(buf, buflen, "%s",docattr->Title);
	}
	else if (strcasecmp(key, "Author") == 0) {
		snprintf(buf, buflen, "%s",docattr->Author);
	}
	else if (strcasecmp(key, "Rid") == 0) {
		snprintf(buf, buflen, "%llu", *((uint64_t *)docattr->Rid));
	}
	else if (strcasecmp(key, "FileNo") == 0) {
		snprintf(buf, buflen, "%u",docattr->FileNo);
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
	supreme_home_cond_t *doccond = (supreme_home_cond_t *)dest;
	int n=0;

	if (strcasecmp(key, "Delete") == 0) {
		doccond->delete_check = 1;
		return 1;
	}

	INFO("key:%s, value:%s",key,value);

	if (strcasecmp(key, "Date") == 0) {
		n = sscanf(value, "%u-%u", &(doccond->Date_start), &(doccond->Date_finish));
		if (n != 2) {
			warn("wrong docattr query: Date");
            return -1;
		}
	    doccond->Date_check = 1;
	}
    else if (strcasecmp(key, "System") == 0) {
		doccond->System = (uint32_t) return_constants_value( value, strlen(value) );

		if ( doccond->System == 0 ) { // 통합검색
			memset( doccond->SystemSum, 0, sizeof(doccond->SystemSum) );
			doccond->SystemSum_check = 1;
		}
		else {
            doccond->System_check = 1;
	    }
    }
	else if (strcasecmp(key, "Type") == 0) {
		doccond->Type = (uint32_t) return_constants_value( value, strlen(value) );
		doccond->Type_check = 1;
	}
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

static int set_docmask_function(void *dest, char *key, char *value)
{
	supreme_home_mask_t *docmask = (supreme_home_mask_t *)dest;

	if (strcasecmp(key, "Delete") == 0) {
		docmask->delete_mark = 1;
	}
   	else if (strcasecmp(key, "Undelete") == 0) {
       	docmask->undelete_mark = 1;
   	}
	else if (strcasecmp(key, "Rid") == 0) {
		docmask->set_Rid = 1;
		strncpy(docmask->Rid, value, sizeof(docmask->Rid));
		docmask->Rid[sizeof(docmask->Rid)-1] = '\0';
	}
	else if (strcasecmp(key, "Date") == 0) {
		docmask->Date = (uint32_t)atol(value);
		docmask->set_Date = 1;
	}
	else if (strcasecmp(key, "System") == 0) {
		docmask->System = (uint32_t) return_constants_value( value, strlen(value) );
		docmask->set_System = 1;
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
	else if (strcasecmp(key, "FileNo") == 0) {
		docmask->FileNo = (uint32_t)atoi(value);
		docmask->set_FileNo = 1;
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

static int init()
{
	init_system_limit();

	if ( sizeof(supreme_home_attr_t) != 64 ) {
		crit("sizeof(supreme_home_attr_t) (%d) != 64", (int)sizeof(supreme_home_attr_t));
		return FAIL;
	}

	if ( sizeof(supreme_home_cond_t) > STRING_SIZE ) {
		crit("sizeof(supreme_home_cond_t) (%d) > STRING_SIZE(%d)",
				(int)sizeof(supreme_home_cond_t), STRING_SIZE);
		return FAIL;
	}

	if ( sizeof(supreme_home_mask_t) > STRING_SIZE ) {
		crit("sizeof(supreme_home_mask_t) (%d) > STRING_SIZE(%d)",
				(int)sizeof(supreme_home_mask_t), STRING_SIZE);
		return FAIL;
	}

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

    /* 세번째 인자 base(진법?) 가 0이면, 문자열의 처음 2글자에 따라 
     * base가 결정된다.
     * "0x1234..." -> 16진수
     * "01234..."  -> 8진수
     * "1234..."   -> 10진수
     */ 
	constants_value[i] = atoi(v.argument[1]);

	INFO("Enum[%s]: %lld", constants[i], constants_value[i]);
}

static void set_system_limit(configValue v)
{
	int system = return_constants_value( v.argument[0], strlen( v.argument[0] ) );

	if ( system >= MAX_SYSTEM ) {
		warn("system[%d] is larger than MAX_SYSTEM[%d]", system, MAX_SYSTEM);
		return;
	}
	else if ( system == 0 ) {
		warn("unknown system[%s]", v.argument[0]);
		return;
	}

	init_system_limit();
	system_limit[system] = atoi( v.argument[1] );

warn("setting system[%s] to %s", v.argument[0], v.argument[1]);
}

static config_t config[] = {
	CONFIG_GET("Enum", get_enum, 2, "constant"),
	CONFIG_GET("SystemLimit", set_system_limit, 2, "max document of each system"),
	{NULL}
};

static void register_hooks(void)
{
	sb_hook_docattr_compare_function(compare_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_compare2_function(compare2_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_mask_function(mask_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_sort_function(compare_function_for_qsort, NULL, NULL, HOOK_MIDDLE);
//	sb_hook_docattr_hit_sort_function(compare_hit_for_qsort, NULL, NULL, HOOK_MIDDLE);

	sb_hook_docattr_set_docattr_function(set_docattr_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_get_docattr_function(get_docattr_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_set_doccond_function(set_doccond_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_set_docmask_function(set_docmask_function, NULL, NULL, HOOK_MIDDLE);
//	sb_hook_docattr_modify_index_list(docattr_distinct_rid, NULL, NULL, HOOK_MIDDLE);
}

module docattr_supreme_home_module = {
	STANDARD_MODULE_STUFF,
	config,					/* conf table */
	NULL,					/* registry */
	init,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};
