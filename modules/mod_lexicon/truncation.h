/* $Id$ */
#ifndef _LEXICON_TRUNCATION_H_
#define _LEXICON_TRUNCATION_H_

#define TRUNCATION_NO				(0)
#define TRUNCATION_YES				(1)

#define MAX_SLOT_NUM_IN_BUCKET		(1024)
#define MAX_WORD_BUCKET				(600) // XXX not change this value!!

#define MAX_MEM_BLOCK_NUM			(100)
#define MAX_BUCKET_NUM_IN_BLOCK		(5000) // max then ( MAX_WORD_BUCKET * 2 ) 
#define MEM_BLOCK_SIZE				(MAX_BUCKET_NUM_IN_BLOCK * sizeof(truncation_bucket_t))
//#define MEM_BLOCK_SIZE				(MAX_BUCKET_NUM_IN_BLOCK * sizeof(truncation_bucket_t))
// is equal sizeof(truncation_bucket_block_t) 

#define TYPE_RIGHT_TRUNCATION		(1)
#define TYPE_LEFT_TRUNCATION		(2)

//XXX check this define 
#define MAX_SEARCH_WORD_NUM			(100)

typedef struct {
	uint32_t wordid;
} truncation_slot_t;

typedef struct {
	int type;
	int	ext_bucket_idx;
	int	slot_count;
	truncation_slot_t slot[MAX_SLOT_NUM_IN_BUCKET];
} truncation_bucket_t; 

typedef struct {
	truncation_bucket_t bucket[MAX_BUCKET_NUM_IN_BLOCK];
} truncation_bucket_block_t;

typedef struct {
	int magic;	
	uint32_t last_wordid;
	uint32_t ext_bucket_idx;	
	int block_num;
} truncation_shared_t;

typedef struct {
	truncation_bucket_block_t	*block[MAX_MEM_BLOCK_NUM];
	truncation_shared_t			*shared;
	char						path[MAX_PATH_LEN];
} truncation_db_t;


int truncation_open( word_db_t *word_db );
int truncation_sync( word_db_t *word_db );
int truncation_close( word_db_t *word_db );
int put_truncation_word(word_db_t *word_db, word_t word);
int print_truncation_bucket(word_db_t *word_db, int bucket_idx);
int truncation_search(word_db_t *word_db, char *word, word_t ret_word[], 
										  int max_search_word, int type); 

#endif
