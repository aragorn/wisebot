#include "softbot.h"
#include "mod_api/docattr.h"
#include "mod_api/qp.h"
#include "mod_api/index_word_extractor.h"
#include "mod_docattr_nhrd.h"
#include "mod_qp/mod_qp.h"

#include <stdio.h>

#define MAX_ENUM_NUM		1024
#define MAX_ENUM_LEN		SHORT_STRING_SIZE

static char *constants[MAX_ENUM_NUM] = { NULL };
static long long constants_value[MAX_ENUM_NUM];

static int cate1_limit[MAX_CATE1] = { -1 };
static int cate2_limit[MAX_CATE2] = { -1 };

static void init_cate_limit()
{
	int i;

	if ( cate2_limit[0] != -1 ) return;

	for ( i = 0; i < MAX_CATE2; i++ )
		cate2_limit[i] = 10;

	for ( i = 0; i < MAX_CATE1; i++ )
		cate1_limit[i] = 20;
}

static int return_constants_value(char *value, int valuelen);
static char* return_constants(int value);

static int compare_function(void *dest, void *cond, uint32_t docid)
{
	nhrd_attr_t *docattr = (nhrd_attr_t*)dest;
	nhrd_cond_t *doccond = (nhrd_cond_t*)cond;

	/* always check delete mark */
	if (docattr->is_deleted) {
		return 0;
	}

	if (doccond->Date_check == 1) {
		if (doccond->Date_start > docattr->Date ||
			doccond->Date_finish < docattr->Date)
			return 0;
	}

	return 1;
}

static int compare2_function(void *dest, void *cond, uint32_t docid)
{
	nhrd_attr_t *docattr = (nhrd_attr_t*)dest;
	nhrd_cond_t *doccond = (nhrd_cond_t*)cond;

	if ( docattr->Cate1 < 1 || docattr->Cate1 >= MAX_CATE1 ) {
		warn("invalid cate1 value: %u(MAX_CATE1:%d), docid[%u]", docattr->Cate1, MAX_CATE1, docid);
		return 0;
	}
	doccond->Cate1Sum[0]++;
	doccond->Cate1Sum[docattr->Cate1]++;

	if ( docattr->Cate2 < 1 || docattr->Cate2 >= MAX_CATE2 ) {
		warn("invalid cate2 value: %u(MAX_CATE2:%d), docid[%u]", docattr->Cate2, MAX_CATE2, docid);
		return 0;
	}
	doccond->Cate2Sum[0]++;
	doccond->Cate2Sum[docattr->Cate2]++;

	if ( doccond->Cate1Sum_check == 1 ) {
		if ( (doccond->Cate1_count > 0 && doccond->Cate1Sum[docattr->Cate1] > doccond->Cate1_count)
				|| doccond->Cate1Sum[docattr->Cate1] > cate1_limit[docattr->Cate1] ) return 0;
	}

	if ( doccond->Cate2Sum_check == 1 ) {
		if ( (doccond->Cate2_count > 0 && doccond->Cate2Sum[docattr->Cate2] > doccond->Cate2_count)
				|| doccond->Cate2Sum[docattr->Cate2] > cate2_limit[docattr->Cate2] ) return 0;
	}

	if (doccond->Cate1_check == 1 && doccond->Cate1 != docattr->Cate1) {
		return 0;
    }

    if (doccond->Cate2_check == 1 && doccond->Cate2 != docattr->Cate2) {
		return 0;
    }

	return 1;
}

static int set_group_result_function(void* cond, group_result_t* group_result, int* size)
{
	nhrd_cond_t *doccond = (nhrd_cond_t*)cond;
	int i, curr = 0;

	strcpy( group_result[curr].field, "Cate1" );
	strcpy( group_result[curr].value, "#" );
	group_result[curr++].count = doccond->Cate1Sum[0];

	for ( i = 1; i < MAX_CATE1 && curr < *size; i++ ) {
		if ( doccond->Cate1Sum[i] == 0 ) continue;

		strcpy( group_result[curr].field, "Cate1" );
		strcpy( group_result[curr].value, return_constants( i ) );
		group_result[curr++].count = doccond->Cate1Sum[i];
	}

	if ( curr < *size ) {
		strcpy( group_result[curr].field, "Cate2" );
		strcpy( group_result[curr].value, "#" );
		group_result[curr++].count = doccond->Cate2Sum[0];
	}

	for ( i = 1; i < MAX_CATE2 && curr < *size; i++ ) {
		if ( doccond->Cate2Sum[i] == 0 ) continue;

		strcpy( group_result[curr].field, "Cate2" );
		strcpy( group_result[curr].value, return_constants( i ) );
		group_result[curr++].count = doccond->Cate2Sum[i];
	}

	if ( curr >= *size ) {
		error("not enough group_result size[%d]", *size);
		return FAIL;
	}

	*size = curr;

	return SUCCESS;
}

static int compare_function_for_qsort(const void *dest, const void *sour, void *userdata)
{
	int i, diff;
	docattr_sort_t *sh;
	nhrd_attr_t attr1, attr2;
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
				diff = hit1 - hit2;
				break;
			case '1': // Title
				diff = hangul_strncmp(attr1.Title, attr2.Title, 16);
				break;
			case '2': // Author
				diff = hangul_strncmp(attr1.Author, attr2.Author, 16);
				break;
			case '3': // Date
				diff = attr1.Date - attr2.Date;
				break;
			case '4': // Cate1
				diff = attr1.Cate1 - attr2.Cate1;
				break;
			case '5': // Cate2
				diff = attr1.Cate2 - attr2.Cate2;
				break;
			default:
				diff = 0;
		}

		if ( diff == 0 ) continue;

		diff = diff > 0 ? 1 : -1;
		return diff * sh->keys[i].order;
	}

	return 0;
}
/*
static int compare_rid(const void *dest, const void *sour)
{
    unsigned long long l1, l2;
    nhrd_attr_t attr1, attr2;

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
    nhrd_attr_t attr1, attr2;
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
	nhrd_attr_t *docattr = (nhrd_attr_t *)dest;
	nhrd_mask_t *docmask = (nhrd_mask_t *)mask;
	

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

	if (docmask->set_Cate1)
		docattr->Cate1 = docmask->Cate1;
	
	if (docmask->set_Cate2)
		docattr->Cate2 = docmask->Cate2;
	
	if (docmask->set_Title)
		memcpy(docattr->Title, docmask->Title, 16);
		
	if (docmask->set_Author)
		memcpy(docattr->Author, docmask->Author, 16);

	return 1;
}

/* if fail, return -1 : 등록시 doc에 대한 docattr 설정 */
static int set_docattr_function(void *dest, char *key, char *value)
{
	nhrd_attr_t *docattr = (nhrd_attr_t *)dest;
	
    debug("key[%s] : value[%s]", key, value);

	if (strcasecmp(key, "Delete") == 0) {
		docattr->is_deleted = 1;
	}
	else if (strcasecmp(key, "Date") == 0) {
		docattr->Date = (uint32_t)atol(value);
	}
	else if (strcasecmp(key, "Cate1") == 0) {
		docattr->Cate1 = (uint32_t) return_constants_value( value, strlen(value) );
	}
	else if (strcasecmp(key, "Cate2") == 0) {
		docattr->Cate2 = (uint32_t) return_constants_value( value, strlen(value) );
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
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

static int get_docattr_function(void *dest, char *key, char *buf, int buflen)
{
	nhrd_attr_t *docattr = (nhrd_attr_t *)dest;

	if (strcasecmp(key, "Delete") == 0) {
		snprintf(buf, buflen, "%u",docattr->is_deleted);
	}
	else if (strcasecmp(key, "Date") == 0) {
		snprintf(buf, buflen, "%u",docattr->Date);
	}
	else if (strcasecmp(key, "Cate1") == 0) {
		snprintf(buf, buflen, "%u",docattr->Cate1);
	}
	else if (strcasecmp(key, "Cate2") == 0) {
		snprintf(buf, buflen, "%u",docattr->Cate2);
	}
	else if (strcasecmp(key, "Title") == 0) {
		snprintf(buf, buflen, "%s",docattr->Title);
	}
	else if (strcasecmp(key, "Author") == 0) {
		snprintf(buf, buflen, "%s",docattr->Author);
	}
	else if (strcasecmp(key, "Rid") == 0) {
		snprintf(buf, buflen, "%llu", *((unsigned long long *)docattr->Rid));
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
	nhrd_cond_t *doccond = (nhrd_cond_t *)dest;
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
    else if (strcasecmp(key, "Cate1Sum") == 0) {
		doccond->Cate1Sum_check = 1;
		doccond->Cate1_count = atoi( value );
    }
    else if (strcasecmp(key, "Cate2Sum") == 0) {
		doccond->Cate2Sum_check = 1;
		doccond->Cate2_count = atoi( value );
    }
    else if (strcasecmp(key, "Cate1") == 0) {
		doccond->Cate1 = (uint32_t) return_constants_value( value, strlen(value) );
		doccond->Cate1_check = 1;
    }
    else if (strcasecmp(key, "Cate2") == 0) {
		doccond->Cate2 = (uint32_t) return_constants_value( value, strlen(value) );
		doccond->Cate2_check = 1;
    }
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

static int set_docmask_function(void *dest, char *key, char *value)
{
	nhrd_mask_t *docmask = (nhrd_mask_t *)dest;

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
	else if (strcasecmp(key, "Cate1") == 0) {
		docmask->Cate1 = (uint32_t) return_constants_value( value, strlen(value) );
		docmask->set_Cate1 = 1;
	}
	else if (strcasecmp(key, "Cate2") == 0) {
		docmask->Cate2 = (uint32_t) return_constants_value( value, strlen(value) );
		docmask->set_Cate2 = 1;
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

static char* return_constants(int value)
{
	int i;
	static char unknown[16];

	for (i=0; i<MAX_ENUM_NUM && constants[i]; i++) {
		if ( constants_value[i] == value ) break;
	}

	if (i == MAX_ENUM_NUM || constants[i] == NULL) {
		snprintf(unknown, sizeof(unknown), "%d", value);
		return unknown;
	}
	return constants[i];
}

static int init()
{
	init_cate_limit();

	if ( sizeof(nhrd_attr_t) != 64 ) {
		crit("sizeof(nhrd_attr_t) (%d) != 64", (int)sizeof(nhrd_attr_t));
		return FAIL;
	}

	if ( sizeof(nhrd_cond_t) > STRING_SIZE ) {
		crit("sizeof(nhrd_cond_t) (%d) > STRING_SIZE(%d)",
				(int)sizeof(nhrd_cond_t), STRING_SIZE);
		return FAIL;
	}

	if ( sizeof(nhrd_mask_t) > STRING_SIZE ) {
		crit("sizeof(nhrd_mask_t) (%d) > STRING_SIZE(%d)",
				(int)sizeof(nhrd_mask_t), STRING_SIZE);
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

static void set_cate1_limit(configValue v)
{
	int cate1 = return_constants_value( v.argument[0], strlen( v.argument[0] ) );

	if ( cate1 >= MAX_CATE1 ) {
		warn("cate1[%d] is larger than MAX_CATE1[%d]", cate1, MAX_CATE1);
		return;
	}
	else if ( cate1 == 0 ) {
		warn("unknown cate1[%s]", v.argument[0]);
		return;
	}

	init_cate_limit();
	cate1_limit[cate1] = atoi( v.argument[1] );
}

static void set_cate2_limit(configValue v)
{
	int cate2 = return_constants_value( v.argument[0], strlen( v.argument[0] ) );

	if ( cate2 >= MAX_CATE2 ) {
		warn("cate2[%d] is larger than MAX_CATE2[%d]", cate2, MAX_CATE2);
		return;
	}
	else if ( cate2 == 0 ) {
		warn("unknown cate2[%s]", v.argument[0]);
		return;
	}

	init_cate_limit();
	cate2_limit[cate2] = atoi( v.argument[1] );
}

static config_t config[] = {
	CONFIG_GET("Enum", get_enum, 2, "constant"),
	CONFIG_GET("Cate1Limit", set_cate1_limit, 2, "max document of each cate1"),
	CONFIG_GET("Cate2Limit", set_cate2_limit, 2, "max document of each cate2"),
	{NULL}
};

static void register_hooks(void)
{
	sb_hook_docattr_compare_function(compare_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_compare2_function(compare2_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_set_group_result_function(set_group_result_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_mask_function(mask_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_sort_function(compare_function_for_qsort, NULL, NULL, HOOK_MIDDLE);
//	sb_hook_docattr_hit_sort_function(compare_hit_for_qsort, NULL, NULL, HOOK_MIDDLE);

	sb_hook_docattr_set_docattr_function(set_docattr_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_get_docattr_function(get_docattr_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_set_doccond_function(set_doccond_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_set_docmask_function(set_docmask_function, NULL, NULL, HOOK_MIDDLE);
//	sb_hook_docattr_modify_index_list(docattr_distinct_rid, NULL, NULL, HOOK_MIDDLE);
}

module docattr_nhrd_module = {
	STANDARD_MODULE_STUFF,
	config,					/* conf table */
	NULL,					/* registry */
	init,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};
