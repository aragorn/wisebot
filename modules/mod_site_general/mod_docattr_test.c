#include "softbot.h"
#include "mod_api/index_word_extractor.h"
#include "mod_api/docattr.h"
#include "mod_api/qp.h"
#include "mod_docattr_test.h"
#include "mod_qp/mod_qp.h"

#include <stdio.h>

#define MAX_ENUM_NUM		1024
#define MAX_ENUM_LEN		SHORT_STRING_SIZE

static char *constants[MAX_ENUM_NUM] = { NULL };
static long long constants_value[MAX_ENUM_NUM];

static long long return_constants_value(char *value, int valuelen);

static int compare_function(void *dest, void *cond, uint32_t docid)
{
	test_attr_t *docattr = (test_attr_t*)dest;
	test_cond_t *doccond = (test_cond_t*)cond;


	/* always check delete mark */
	if ( docattr->is_deleted ) {
		return 0;
	}

	if ( doccond->date_check == 1 ) {
		if ( docattr->date != doccond->date ) return 0;
	}

	if ( doccond->int1_check == 1 ) {
		if ( docattr->int1 != doccond->int1 ) return 0;
	}

	if ( doccond->int2_check == 1 ) {
		if ( docattr->int2 != doccond->int2 ) return 0;
	}

	if ( doccond->bit1_check == 1 ) {
		if ( docattr->bit1 != doccond->bit1 ) return 0;
	}

	if ( doccond->bit2_check == 1 ) {
		if ( docattr->bit2 != doccond->bit2 ) return 0;
	}

	if ( doccond->cate_check == 1 ) {
		if ( docattr->cate != doccond->cate ) return 0;
	}

	return 1;
}

static int compare_function_for_qsort(const void *dest, const void *sour, void *userdata)
{
	int i, diff;
	docattr_sort_t *sh;
	test_attr_t attr1, attr2;

	if ( sb_run_docattr_get( ((doc_hit_t *) dest)->id, &attr1 ) < 0) {
		error("cannot get docattr element");
		return 0;
	}
	if ( sb_run_docattr_get( ((doc_hit_t *) sour)->id, &attr2 ) < 0) {
		error("cannot get docattr element");
		return 0;
	}
	

	sh = (docattr_sort_t *) userdata;
	for ( i = 0; i < MAX_SORTING_CRITERION && sh->keys[i].key[0] != '\0'; i++) {

		switch ( sh->keys[i].key[0] ) {
			case '0': // HIT
				diff = ((doc_hit_t*) dest)->hitratio - ((doc_hit_t*) sour)->hitratio;
				break;

			case '1': // title
				diff = hangul_strncmp( attr1.title, attr2.title, sizeof(attr1.title) );
				break;

			case '2': // date
				diff = attr1.date - attr2.date;
				break;

			case '3': // int1
				diff = attr1.int1 - attr2.int1;
				break;

			case '4': // int2
				diff = attr1.int2 - attr2.int2;
				break;

			case '5': // bit1
				diff = attr1.bit1 - attr2.bit1;
				break;

			case '6': // bit2
				diff = attr1.bit2 - attr2.bit2;
				break;

			case '7': // cate
				diff = attr1.cate - attr2.cate;
				break;

			default:
				continue;
		} // switch

		if ( diff != 0 ) {
			diff = diff > 0 ? 1 : -1;
			return diff * sh->keys[i].order;
		}
	} // for (i)

	return 0;
}

/*
 * NOTICE:
 * 	if success, return SUCCESS(1)
 */
static int mask_function(void *dest, void *mask) {
	test_attr_t *docattr = (test_attr_t*) dest;
	test_mask_t *docmask = (test_mask_t*) mask;
	
	if ( docmask->delete_mark )
		docattr->is_deleted = 1;

	if ( docmask->undelete_mark )
		docattr->is_deleted = 0;

	if ( docmask->set_title )
		memcpy( docattr->title, docmask->title, sizeof(docattr->title) );

	if ( docmask->set_date )
		docattr->date = docmask->date;
		
	if ( docmask->set_int1 )
		docattr->int1 = docmask->int1;
		
	if ( docmask->set_int2 )
		docattr->int2 = docmask->int2;
		
	if ( docmask->set_bit1 )
		docattr->bit1 = docmask->bit1;
		
	if ( docmask->set_bit2 )
		docattr->bit2 = docmask->bit2;
		
	if ( docmask->set_cate )
		docattr->cate = docmask->cate;

	if ( docmask->set_rid1 ) {
		MD5_CTX context;
		unsigned char digest[16];
		unsigned int len = strlen(docmask->rid1);

		MD5Init(&context);
		MD5Update(&context, docmask->rid1, len);
		MD5Final(digest, &context);

		memcpy(docmask->rid1, digest, DOCATTR_RID_LEN);
	}
		
	if ( docmask->set_rid2 ) {
		MD5_CTX context;
		unsigned char digest[16];
		unsigned int len = strlen(docmask->rid2);

		MD5Init(&context);
		MD5Update(&context, docmask->rid2, len);
		MD5Final(digest, &context);

		memcpy(docmask->rid2, digest, DOCATTR_RID_LEN);
	}
		
	if ( docmask->set_rid3 ) {
		MD5_CTX context;
		unsigned char digest[16];
		unsigned int len = strlen(docmask->rid3);

		MD5Init(&context);
		MD5Update(&context, docmask->rid3, len);
		MD5Final(digest, &context);

		memcpy(docmask->rid3, digest, DOCATTR_RID_LEN);
	}
		
	return 1;
}

/* if fail, return -1 : 등록시 doc에 대한 docattr 설정 */
static int set_docattr_function(void *dest, char *key, char *value)
{
	test_attr_t *docattr = (test_attr_t *) dest;
	
    debug("key[%s] : value[%s]", key, value);

	if ( strcasecmp( key, "Delete" ) == 0 ) {
		docattr->is_deleted = 1;
	}
	else if ( strcasecmp( key, "title" ) == 0) {
		/* chinese -> hangul */
		char _value[SHORT_STRING_SIZE];

		index_word_extractor_t *ex = NULL;
		index_word_t word;

		strncpy( _value, value, SHORT_STRING_SIZE-1 );
		_value[SHORT_STRING_SIZE-1] = '\0';

		ex = sb_run_new_index_word_extractor(2);
		if ( ex == NULL || ex == (index_word_extractor_t*) MINUS_DECLINE ) return FAIL;
		sb_run_index_word_extractor_set_text( ex, _value );

		sb_run_get_index_words( ex, &word, sizeof(docattr->title) );
		strncpy( docattr->title, word.word, sizeof(docattr->title) );
	}
	else if ( strcasecmp( key, "date" ) == 0 ) {
		docattr->cate = atoi( value );
	}
	else if ( strcasecmp( key, "int1" ) == 0 ) {
		docattr->int1 = atoi( value );
	}
	else if ( strcasecmp( key, "int2" ) == 0 ) {
		docattr->int2 = atoi( value );
	}
	else if ( strcasecmp( key, "bit1" ) == 0 ) {
		docattr->bit1 = atoi( value );
	}
	else if ( strcasecmp( key, "bit2" ) == 0 ) {
		docattr->bit2 = atoi( value );
	}
	else if ( strcasecmp( key, "cate" ) == 0 ) {
		docattr->cate = (int) return_constants_value( value, strlen( value ) );
	}
	else if ( strcasecmp( key, "rid1" ) == 0 ) {
		MD5_CTX context;
		unsigned char digest[16];
		unsigned int len = strlen(value);

		MD5Init(&context);
		MD5Update(&context, value, len);
		MD5Final(digest, &context);

		memcpy(&docattr->rid1, digest, DOCATTR_RID_LEN);
	}
	else if ( strcasecmp( key, "rid2" ) == 0 ) {
		MD5_CTX context;
		unsigned char digest[16];
		unsigned int len = strlen(value);

		MD5Init(&context);
		MD5Update(&context, value, len);
		MD5Final(digest, &context);

		memcpy(&docattr->rid2, digest, DOCATTR_RID_LEN);
	}
	else if ( strcasecmp( key, "rid3" ) == 0 ) {
		MD5_CTX context;
		unsigned char digest[16];
		unsigned int len = strlen(value);

		MD5Init(&context);
		MD5Update(&context, value, len);
		MD5Final(digest, &context);

		memcpy(&docattr->rid3, digest, DOCATTR_RID_LEN);
	}
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

static int get_docattr_function(void *dest, char *key, char *buf, int buflen)
{
	test_attr_t *docattr = (test_attr_t *) dest;

	if ( strcasecmp( key, "Delete" ) == 0 ) {
		snprintf( buf, buflen, "%u", docattr->is_deleted );
	}
	else if ( strcasecmp( key, "title" ) == 0 ) {
		snprintf( buf, buflen, "%s", docattr->title );
	}
	else if ( strcasecmp( key, "date" ) == 0 ) {
		snprintf( buf, buflen, "%d", docattr->date );
	}
	else if ( strcasecmp( key, "int1" ) == 0 ) {
		snprintf( buf, buflen, "%d", docattr->int1 );
	}
	else if ( strcasecmp( key, "int2" ) == 0 ) {
		snprintf( buf, buflen, "%d", docattr->int2 );
	}
	else if ( strcasecmp( key, "bit1" ) == 0 ) {
		snprintf( buf, buflen, "%d", docattr->bit1 );
	}
	else if ( strcasecmp( key, "bit2" ) == 0 ) {
		snprintf( buf, buflen, "%d", docattr->bit2 );
	}
	else if ( strcasecmp( key, "cate" ) == 0 ) {
		snprintf( buf, buflen, "%d", docattr->cate );
	}
	else if ( strcasecmp( key, "rid1" ) == 0 ) {
		snprintf( buf, buflen, "%" PRIu64, docattr->rid1 );
	}
	else if ( strcasecmp( key, "rid2" ) == 0 ) {
		snprintf( buf, buflen, "%" PRIu64, docattr->rid2 );
	}
	else if ( strcasecmp( key, "rid3" ) == 0 ) {
		snprintf( buf, buflen, "%" PRIu64, docattr->rid3 );
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
	test_cond_t *doccond = (test_cond_t *)dest;

	if ( strcasecmp(key, "Delete" ) == 0) {
		doccond->delete_check = 1;
		return 1;
	}

	INFO("key:%s, value:%s", key, value);

	if ( strcasecmp( key, "date" ) == 0 ) {
		doccond->date = atoi( value );
		doccond->date_check = 1;
	}
	else if ( strcasecmp( key, "int1" ) == 0 ) {
		doccond->int1 = atoi( value );
		doccond->int1_check = 1;
	}
	else if ( strcasecmp( key, "int2" ) == 0 ) {
		doccond->int2 = atoi( value );
		doccond->int2_check = 1;
	}
	else if ( strcasecmp( key, "bit1" ) == 0 ) {
		doccond->bit1 = atoi( value );
		doccond->bit1_check = 1;
	}
	else if ( strcasecmp( key, "bit2" ) == 0 ) {
		doccond->bit2 = atoi( value );
		doccond->bit2_check = 1;
	}
	else if ( strcasecmp( key, "cate" ) == 0 ) {
		doccond->cate = (int) return_constants_value( value, strlen(value) );
		doccond->cate_check = 1;
	}
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

static int set_docmask_function(void *dest, char *key, char *value)
{
	test_mask_t *docmask = (test_mask_t *)dest;

	if ( strcasecmp( key, "Delete" ) == 0 ) {
		docmask->delete_mark = 1;
	}
   	else if ( strcasecmp( key, "Undelete" ) == 0) {
       	docmask->undelete_mark = 1;
   	}
	else if ( strcasecmp( key, "title" ) == 0) {
		/* chinese -> hangul */
		char _value[SHORT_STRING_SIZE];

		index_word_extractor_t *ex = NULL;
		index_word_t word;

		strncpy( _value, value, SHORT_STRING_SIZE-1 );
		_value[SHORT_STRING_SIZE-1] = '\0';

		ex = sb_run_new_index_word_extractor(2);
		if ( ex == NULL || ex == (index_word_extractor_t*) MINUS_DECLINE ) return FAIL;
		sb_run_index_word_extractor_set_text( ex, _value );

		sb_run_get_index_words( ex, &word, sizeof(docmask->title) );
		strncpy( docmask->title, word.word, sizeof(docmask->title) );
		docmask->set_title = 1;
	}
	else if ( strcasecmp( key, "date" ) == 0 ) {
		docmask->date = atoi( value );
		docmask->set_date = 1;
	}
	else if ( strcasecmp( key, "int1" ) == 0 ) {
		docmask->int1 = atoi( value );
		docmask->set_int1 = 1;
	}
	else if ( strcasecmp( key, "int2" ) == 0 ) {
		docmask->int2 = atoi( value );
		docmask->set_int2 = 1;
	}
	else if ( strcasecmp( key, "bit1" ) == 0 ) {
		docmask->bit1 = atoi( value );
		docmask->set_bit1 = 1;
	}
	else if ( strcasecmp( key, "bit2" ) == 0 ) {
		docmask->bit2 = atoi( value );
		docmask->set_bit2 = 1;
	}
	else if ( strcasecmp( key, "cate" ) == 0 ) {
		docmask->cate = (int) return_constants_value( value, strlen(value) );
		docmask->set_cate = 1;
	}
	else if ( strcasecmp( key, "rid1" ) == 0 ) {
		docmask->set_rid1 = 1;
		strncpy( docmask->rid1, value, 32 );
		docmask->rid1[32-1] = '\0';
	}
	else if ( strcasecmp( key, "rid2" ) == 0 ) {
		docmask->set_rid2 = 1;
		strncpy( docmask->rid2, value, 32 );
		docmask->rid2[32-1] = '\0';
	}
	else if ( strcasecmp( key, "rid3" ) == 0 ) {
		docmask->set_rid3 = 1;
		strncpy( docmask->rid3, value, 32 );
		docmask->rid3[32-1] = '\0';
	}
	else {
		warn("there is no such a field[%s]", key);
	}
	return 1;
}

static int compare_rid1(const void *dest, const void *sour)
{
    uint64_t l1, l2;
    test_attr_t attr1, attr2;

    if (sb_run_docattr_get(((doc_hit_t *)dest)->id, &attr1) < 0) {
        error("cannot get docattr element");
        return 0;
    }   

    if (sb_run_docattr_get(((doc_hit_t *)sour)->id, &attr2) < 0) {
        error("cannot get docattr element");
        return 0;
    }
        
    l1 = attr1.rid1;
    l2 = attr2.rid1;
    
    if (l1 > l2) return 1;
    else if (l1 == l2) return 0;
    else return -1;
}

static int docattr_distinct_rid1(int id, index_list_t *list)
{
    int i, abst;
    test_attr_t attr1, attr2;
    uint64_t rid1, rid2;

    /* default filtering id */
//  if (id != 0) return DECLINE;
    qsort(list->doc_hits, list->ndochits, sizeof(doc_hit_t), compare_rid1);

    abst = 0;

    if (sb_run_docattr_get(list->doc_hits[0].id, &attr1) < 0) {
        error("cannot get docattr element");
        return -1;
    }
    rid2 = attr1.rid1;

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

            rid2 = attr2.rid1;

            if (rid1 == 0 || rid2 == 0) break;
            if (rid1 != rid2) break;
        }

        abst++;
	}

	list->ndochits = abst;

	return SUCCESS;
}
 
static int compare_rid2(const void *dest, const void *sour)
{
    uint64_t l1, l2;
    test_attr_t attr1, attr2;

    if (sb_run_docattr_get(((doc_hit_t *)dest)->id, &attr1) < 0) {
        error("cannot get docattr element");
        return 0;
    }   

    if (sb_run_docattr_get(((doc_hit_t *)sour)->id, &attr2) < 0) {
        error("cannot get docattr element");
        return 0;
    }
        
    l1 = attr1.rid2;
    l2 = attr2.rid2;
    
    if (l1 > l2) return 1;
    else if (l1 == l2) return 0;
    else return -1;
}

static int docattr_distinct_rid2(int id, index_list_t *list)
{
    int i, abst;
    test_attr_t attr1, attr2;
    uint64_t rid1, rid2;

    /* default filtering id */
//  if (id != 0) return DECLINE;
    qsort(list->doc_hits, list->ndochits, sizeof(doc_hit_t), compare_rid2);

    abst = 0;

    if (sb_run_docattr_get(list->doc_hits[0].id, &attr1) < 0) {
        error("cannot get docattr element");
        return -1;
    }
    rid2 = attr1.rid2;

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

            rid2 = attr2.rid2;

            if (rid1 == 0 || rid2 == 0) break;
            if (rid1 != rid2) break;
        }

        abst++;
	}

	list->ndochits = abst;

	return SUCCESS;
}
 
static int compare_rid3(const void *dest, const void *sour)
{
    uint64_t l1, l2;
    test_attr_t attr1, attr2;

    if (sb_run_docattr_get(((doc_hit_t *)dest)->id, &attr1) < 0) {
        error("cannot get docattr element");
        return 0;
    }   

    if (sb_run_docattr_get(((doc_hit_t *)sour)->id, &attr2) < 0) {
        error("cannot get docattr element");
        return 0;
    }
        
    l1 = attr1.rid3;
    l2 = attr2.rid3;
    
    if (l1 > l2) return 1;
    else if (l1 == l2) return 0;
    else return -1;
}

static int docattr_distinct_rid3(int id, index_list_t *list)
{
    int i, abst;
    test_attr_t attr1, attr2;
    uint64_t rid1, rid2;

    /* default filtering id */
//  if (id != 0) return DECLINE;
    qsort(list->doc_hits, list->ndochits, sizeof(doc_hit_t), compare_rid3);

    abst = 0;

    if (sb_run_docattr_get(list->doc_hits[0].id, &attr1) < 0) {
        error("cannot get docattr element");
        return -1;
    }
    rid2 = attr1.rid3;

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

            rid2 = attr2.rid3;

            if (rid1 == 0 || rid2 == 0) break;
            if (rid1 != rid2) break;
        }

        abst++;
	}

	list->ndochits = abst;

	return SUCCESS;
}
 
/* if fail, return 0 */
static long long return_constants_value(char *value, int valuelen)
{
	int i;

	for ( i = 0; i < MAX_ENUM_NUM && constants[i]; i++ ) {
		if ( strncmp( value, constants[i], MAX_ENUM_LEN ) == 0 ) {
			break;
		}
	}
	if ( i == MAX_ENUM_NUM || constants[i] == NULL ) {
		return 0;
	}
	return constants_value[i];
}

static int init()
{
	if ( sizeof(test_attr_t) != 64 ) {
		crit("sizeof(test_attr_t) (%d) != 64", sizeof(test_attr_t) );
		return FAIL;
	}

	if ( sizeof(test_cond_t) >= STRING_SIZE ) {
		crit("sizeof(test_cond_t) (%d) >= STRING_SIZE(%d)", sizeof(test_cond_t), STRING_SIZE);
		return FAIL;
	}

	if ( sizeof(test_mask_t) >= STRING_SIZE ) {
		crit("sizeof(test_mask_t) (%d) >= STRING_SIZE(%d)", sizeof(test_mask_t), STRING_SIZE);
		return FAIL;
	}

	return SUCCESS;
}

static void get_enum(configValue v)
{
	int i;
	static char enums[MAX_ENUM_NUM][MAX_ENUM_LEN];

	for ( i = 0; i < MAX_ENUM_NUM && constants[i]; i++ );
	if ( i == MAX_ENUM_NUM ) {
		error("too many constant is defined");
		return;
	}
	strncpy( enums[i], v.argument[0], MAX_ENUM_LEN );
	enums[i][MAX_ENUM_LEN-1] = '\0';
	constants[i] = enums[i];

    /* 세번째 인자 base(진법?) 가 0이면, 문자열의 처음 2글자에 따라 
     * base가 결정된다.
     * "0x1234..." -> 16진수
     * "01234..."  -> 8진수
     * "1234..."   -> 10진수
     */ 
	constants_value[i] = atoi( v.argument[1] );

	INFO("Enum[%s]: %lld", constants[i], constants_value[i]);
}

static config_t config[] = {
	CONFIG_GET("Enum", get_enum, 2, "constant"),
	{NULL}
};

static void register_hooks(void)
{
	sb_hook_docattr_compare_function(compare_function, NULL, NULL, HOOK_MIDDLE);
//	sb_hook_docattr_compare2_function(compare2_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_mask_function(mask_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_sort_function(compare_function_for_qsort, NULL, NULL, HOOK_MIDDLE);
//	sb_hook_docattr_hit_sort_function(compare_hit_for_qsort, NULL, NULL, HOOK_MIDDLE);

	sb_hook_docattr_set_docattr_function(set_docattr_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_get_docattr_function(get_docattr_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_set_doccond_function(set_doccond_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_set_docmask_function(set_docmask_function, NULL, NULL, HOOK_MIDDLE);
}

module docattr_test_module = {
	STANDARD_MODULE_STUFF,
	config,					/* conf table */
	NULL,					/* registry */
	init,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};

static void register_hooks_rid1(void)
{
	sb_hook_docattr_modify_index_list(docattr_distinct_rid1, NULL, NULL, HOOK_MIDDLE);
}

module docattr_test_rid1_module = {
	STANDARD_MODULE_STUFF,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	register_hooks_rid1
};

static void register_hooks_rid2(void)
{
	sb_hook_docattr_modify_index_list(docattr_distinct_rid2, NULL, NULL, HOOK_MIDDLE);
}

module docattr_test_rid2_module = {
	STANDARD_MODULE_STUFF,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	register_hooks_rid2
};

static void register_hooks_rid3(void)
{
	sb_hook_docattr_modify_index_list(docattr_distinct_rid3, NULL, NULL, HOOK_MIDDLE);
}

module docattr_test_rid3_module = {
	STANDARD_MODULE_STUFF,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	register_hooks_rid3
};
