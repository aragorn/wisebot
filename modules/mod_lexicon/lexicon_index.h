/* $Id$ */
#ifndef _LEXICON_INDEX_H_
#define _LEXICON_INDEX_H_

#include "mod_api/lexicon.h"

#define LEXICON_INDEX_NEW_WORD			(100)
#define LEXICON_INDEX_EXIST_WORD		(101)
#define LEXICON_INDEX_NOT_EXIST_WORD	(102)



int lexicon_index_open	( word_db_t *word_db );
int lexicon_index_sync	( word_db_t *word_db );
int lexicon_index_close	( word_db_t *word_db );

int lexicon_index_put	( word_db_t *word_db, char* string, uint32_t* wordid, word_offset_t *offset);
int lexicon_index_get	( word_db_t *word_db, char* string, uint32_t* wordid);
int lexicon_index_del	( word_db_t *word_db, char* string);

#endif
