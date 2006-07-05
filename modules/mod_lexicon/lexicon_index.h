#ifndef LEXICON_INDEX_H
#define LEXICON_INDEX_H

#include "auto_config.h"

#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

#include "mod_lexicon.h" /* lexicon_t */

int lexicon_index_open  ( lexicon_t *lexicon );
int lexicon_index_sync  ( lexicon_t *lexicon );
int lexicon_index_close ( lexicon_t *lexicon );

int lexicon_index_put   ( lexicon_t *lexicon, char* string, uint32_t* wordid, word_offset_t *offset);
int lexicon_index_get   ( lexicon_t *lexicon, char* string, uint32_t* wordid);
int lexicon_index_del   ( lexicon_t *lexicon, char* string);

#endif
