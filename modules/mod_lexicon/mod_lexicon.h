/* $Id$ */
#ifndef _MOD_LEXICON_H_
#define _MOD_LEXICON_H_ 

#include "softbot.h"
#include "mod_api/lexicon.h"

// see mod_api/lexicon.h
#define FILE_CREAT_FLAG (O_RDWR|O_CREAT)
#define FILE_CREAT_MODE (0600)

// for lexicon index dynamic hash
int new_vb_offset(word_db_t *word_db, word_offset_t *word_offset);
int block_write(word_db_t *word_db, word_offset_t *word_offset, 
				int type, void *data, int size);
int increase_block_offset(word_db_t *word_db, int len, int type);
void *offset2ptr(word_db_t *word_db, word_offset_t *word_offset, int type);

// for lexicon truncation function
int get_word_by_wordid(word_db_t *word_db, word_t *word);
#endif
