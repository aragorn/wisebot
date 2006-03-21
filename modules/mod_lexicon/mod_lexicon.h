/* $Id$ */
#ifndef MOD_LEXICON_H
#define MOD_LEXICON_H

#include "mod_api/lexicon.h"

#define MAX_BLOCK_NUM       254
#define BLOCK_DATA_SIZE     16000000 /* 16MB */
#define NUM_WORDS_PER_BLOCK (BLOCK_DATA_SIZE / sizeof(internal_word_t))

#define BLOCK_TYPE_FIXED    (1)
#define BLOCK_TYPE_VARIABLE (2)

typedef struct {
	uint8_t           block_idx    :8; /* max 255 blocks */
	uint32_t          offset       :24; /* block size of 16MB */
} __attribute__((packed)) word_offset_t;

typedef struct {
	word_offset_t     word_offset;
} internal_word_t;

typedef struct {
	uint32_t          used_bytes;
	char              data[BLOCK_DATA_SIZE];
} word_block_t;

typedef struct {
	uint32_t          last_wordid;
	int               alloc_fixed_block_num;
	int               alloc_variable_block_num;
} lexicon_shared_t;

typedef struct {
	word_block_t      *fixed_block[MAX_BLOCK_NUM];
	word_block_t      *variable_block[MAX_BLOCK_NUM];
	lexicon_shared_t  *shared;
	void              *hash;
	void              *hash_data;
	char              path[MAX_PATH_LEN];

	int               lock_id;
	int               lock_ref_count;
} lexicon_t;

// for lexicon index dynamic hash
int new_vb_offset(lexicon_t *word_db, word_offset_t *word_offset);
int block_write(lexicon_t *word_db, word_offset_t *word_offset, 
				int type, void *data, int size);
int increase_block_offset(lexicon_t *word_db, int len, int type);
void *offset2ptr(lexicon_t *word_db, word_offset_t *word_offset, int type);

#endif
