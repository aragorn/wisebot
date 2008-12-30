/* $Id$ */
#include "common_core.h"
#include "mod_standard_handler.h"
#include "handler_util.h"
#include "mod_httpd/http_util.h"
#include "mod_httpd/http_protocol.h"
#include "memory.h"
#include "apr_strings.h"
#include "mod_api/sbhandler.h"
#include "mod_api/indexdb.h"
#include "mod_api/lexicon.h"
#include "mod_api/cdm2.h"
#include <stdlib.h>

// function prototype
static int indexed_hit_count(request_rec *r, softbot_handler_rec *s);
static int last_word_id(request_rec *r, softbot_handler_rec *s);
static int index_status(request_rec *r, softbot_handler_rec *s);

static softbot_handler_key_t index_handler_tbl[] =
{	
	{"hit_count", indexed_hit_count},
	{"last_word_id", last_word_id},
	{"index_status", index_status},
	{NULL, NULL}
};

// 마지막 단어ID
// SB_DECLARE_HOOK(int,get_num_of_word,(word_db_t* word_db, uint32_t *number))
static int last_word_id(request_rec *r, softbot_handler_rec *s)
{
    uint32_t last_word_id = 0;
	char content_type[SHORT_STRING_SIZE+1];

	if ( sb_run_get_num_of_word( word_db, &last_word_id ) != SUCCESS ) {
		warn("lexicon failed to get last wordid");
		return FAIL;
	}

    snprintf( content_type, SHORT_STRING_SIZE, "text/xml; charset=%s", default_charset);
    ap_set_content_type(r, content_type);
    ap_rprintf(r, "<?xml version=\"1.0\" encoding=\"%s\"?>\n", default_charset);
	ap_rprintf(r, 
			"<xml>\n"
			    "<item>\n" 
			        "<column name=\"last_word_id\">%d</column>\n" 
			    "</item>\n" 
			"</xml>\n",
            last_word_id);

    return SUCCESS;
}

/* word.id를 setting 한 후 사용하여야 한다. */
static int get_word_info(word_t* word, int* hit_count) {
    int length = 0;

    if( word == NULL || word->id <= 0 ) {
        warn("word info invalide");
        return FAIL;
    }

	if ( sb_run_get_word_by_wordid( word_db, word ) != SUCCESS ) {
		warn("lexicon failed to get word");
		return FAIL;
	}

    length = sb_run_indexdb_getsize( index_db, word->id );
    if ( length == INDEXDB_FILE_NOT_EXISTS ) {
        warn("length is 0 of word[%d]: %s. something is wrong", word->id, word->string);
        return 0;
    }   
    else if ( length == FAIL ) {
        warn("indexdb_getsize failed. word[%d]: %s", word->id, word->string);
        return FAIL;
    }

    info("word[%s] - length: %d, count: %d", word->string, length, length/(int)sizeof(doc_hit_t));
        
    *hit_count = length / sizeof(doc_hit_t);

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////
// 단어ID에 대한 문서수
static int indexed_hit_count(request_rec *r, softbot_handler_rec *s)
{
    int hit_count = 0;
    word_t word;
    char* str_word_id = 0;
    char* str_count = 0;
    int count = 0;
    int i = 0;
	char content_type[SHORT_STRING_SIZE+1];

    str_word_id = apr_pstrdup(r->pool, apr_table_get(s->parameters_in, "word_id"));

    if(str_word_id == NULL || strlen(str_word_id) == 0) {
        MSG_RECORD(&s->msg, error, "word_id is null, get parameter exist with word_id.");
        return FAIL;
    }
    word.id = atoi(str_word_id);
    
    str_count = apr_pstrdup(r->pool, apr_table_get(s->parameters_in, "count"));

    if(str_count == NULL || strlen(str_count) == 0) {
        count = 1;
    } else {
        count = atoi(str_count);
    }

    snprintf( content_type, SHORT_STRING_SIZE, "text/xml; charset=%s", default_charset);
	ap_set_content_type(r, content_type);
	ap_rprintf(r, "<?xml version=\"1.0\" encoding=\"%s\"?>\n", default_charset);

	ap_rprintf(r, "<xml>\n");

    for( i = 0; i < count; i++ ) {
		if( get_word_info(&word, &hit_count) == FAIL) break;

		ap_rprintf(r, 
				"<item>\n" 
					"<column name=\"word_id\">%d</column>\n" 
					"<column name=\"word\"><![CDATA[%s]]></column>\n"
					"<column name=\"hit_count\">%d</column>\n"
				"</item>\n",
				word.id, 
				word.string, 
				hit_count);

        word.id += 1;
    }

	ap_rprintf(r, "</xml>\n");
    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////
// 문서 등록/색인 상태

static int index_status(request_rec *r, softbot_handler_rec *s)
{
    uint32_t last_registered_docid = 0;
    uint32_t last_indexed_docid = 0;
	char content_type[SHORT_STRING_SIZE+1];

    last_registered_docid = sb_run_cdm_last_docid(cdm_db);
    last_indexed_docid = indexer_shared->last_indexed_docid;

    snprintf( content_type, SHORT_STRING_SIZE, "text/xml; charset=%s", default_charset);
	ap_set_content_type(r, content_type);
	ap_rprintf(r, "<?xml version=\"1.0\" encoding=\"%s\"?>\n", default_charset);
	ap_rprintf(r, "<xml>\n");

	ap_rprintf(r, 
			"<item>\n" 
				"<column name=\"last_registered_docid\">%d</column>\n" 
				"<column name=\"last_indexed_docid\">%d</column>\n"
				"<column name=\"is_index_completed\">%s</column>\n"
			"</item>\n",
			last_registered_docid, 
			last_indexed_docid, 
			(last_registered_docid == last_indexed_docid ? "TRUE" : "FALSE") );

	ap_rprintf(r, "</xml>\n");

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////
// name space 노출 함수
int sbhandler_index_get_table(char *name_space, void **tab)
{
	if ( strcmp(name_space, "index") != 0 ){
		return DECLINE;
	}

	*tab = index_handler_tbl;

	return SUCCESS;
}
