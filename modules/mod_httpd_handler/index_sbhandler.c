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
#include <stdlib.h>

// function prototype
static int indexed_hit_count(request_rec *r, softbot_handler_rec *s);
static int last_word_id(request_rec *r, softbot_handler_rec *s);

static softbot_handler_key_t index_handler_tbl[] =
{	
	{"hit_count", indexed_hit_count},
	{"last_word_id", last_word_id},
	{NULL, NULL}
};

// 마지막 단어ID
// SB_DECLARE_HOOK(int,get_num_of_word,(word_db_t* word_db, uint32_t *number))
static int last_word_id(request_rec *r, softbot_handler_rec *s)
{
    uint32_t last_word_id = 0;

	if ( sb_run_get_num_of_word( word_db, &last_word_id ) != SUCCESS ) {
		warn("lexicon failed to get last wordid");
		return FAIL;
	}

	ap_rprintf(r,
			"<?xml version=\"1.0\" encoding=\"euc-kr\"?>\n");
	ap_rprintf(r, 
			"<xml>\n"
			    "<item name=\"last_word_id\">%d</item>\n" 
			"</xml>\n",
            last_word_id);

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////
// 단어ID에 대한 문서수
static int indexed_hit_count(request_rec *r, softbot_handler_rec *s)
{
    int length;
    int ndochits;
    word_t word;
    char* word_id = 0;

    word_id = apr_pstrdup(r->pool, apr_table_get(s->parameters_in, "word_id"));

    if(word_id == NULL || strlen(word_id) == 0) {
        MSG_RECORD(&s->msg, error, "word_id is null, get parameter exist with word_id.");
        return FAIL;
    }
    word.id = atoi(word_id);
    
	if ( sb_run_get_word_by_wordid( word_db, &word ) != SUCCESS ) {
		warn("lexicon failed to get word");
		return FAIL;
	}

    length = sb_run_indexdb_getsize( index_db, word.id );
    if ( length == INDEXDB_FILE_NOT_EXISTS ) {
        warn("length is 0 of word[%d]: %s. something is wrong", word.id, word.string);
        return 0;
    }   
    else if ( length == FAIL ) {
        warn("indexdb_getsize failed. word[%d]: %s", word.id, word.string);
        return FAIL;
    }
        
    info("word[%s] - length: %d, count: %d", word.string, length, length/(int)sizeof(doc_hit_t));
        
    ndochits = length / sizeof(doc_hit_t);

	ap_rprintf(r,
			"<?xml version=\"1.0\" encoding=\"euc-kr\"?>\n");
	ap_rprintf(r, 
			"<xml>\n"
			    "<item name=\"word_id\">%d</item>\n" 
			    "<item name=\"word\"><![CDATA[%s]]></item>\n"
			    "<item name=\"hit_count\">%d</item>\n"
			"</xml>\n",
			word.id, 
			word.string, 
            ndochits);

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
