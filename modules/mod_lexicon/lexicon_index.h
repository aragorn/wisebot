#ifndef _LEXICON_INDEX_H_
#define _LEXICON_INDEX_H_

#include "mod_api/lexicon.h"
#include "mod_lexicon.h"

int lexicon_index_open  ( lexicon_t *lexicon );
int lexicon_index_sync  ( lexicon_t *lexicon );
int lexicon_index_close ( lexicon_t *lexicon );

int lexicon_index_put   ( lexicon_t *lexicon, char* string, uint32_t* wordid, word_offset_t *offset);
int lexicon_index_get   ( lexicon_t *lexicon, char* string, uint32_t* wordid);
int lexicon_index_del   ( lexicon_t *lexicon, char* string);

#endif
