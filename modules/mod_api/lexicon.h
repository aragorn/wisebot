/* $Id$ */
#ifndef LEXICON_H
#define LEXICON_H 1

#include "softbot.h"
#include "hook.h"

// XXX: should be moved to mod_lexicon.h
// lexicon 의 define value
#define MAX_WORD_LENGTH             (MAX_WORD_LEN)  /* long enough for general purpose */
#define MAX_BLOCK_NUM               254 /* should be smaller than 255 to prevent overflow */
#define BLOCK_DATA_SIZE             16000000 /* 16MB */
//#define BLOCK_DATA_SIZE             160000 /* 160kb */
#define NUM_WORDS_PER_BLOCK			(BLOCK_DATA_SIZE / sizeof(internal_word_t))
#define MAX_WORDID					MAX_BLOCK_NUM * NUM_WORDS_PER_BLOCK

#define BLOCK_TYPE_FIXED			(1)
#define BLOCK_TYPE_VARIABLE			(2)

// XXX: till here.. should be moved to mod_lexicon.h

//#define FIXED_BLOCK_MAX_WORD        100
//#define VARIABLE_BLOCK_MAX_OFFSET   1000 
#define DEFAULT_LEXICON_DB_PATH     "dat/lexicon/"

// lexicon 의 return value
#define WORD_OLD_REGISTERED         (1)
#define WORD_NEW_REGISTERED         (2)

#define WORD_NOT_REGISTERED         (-1)
#define WORD_ID_OVERFLOW            (-2)
#define WORD_NOT_EXIST              (-3)
#define WORD_ID_NOT_EXIST           (-4)
#define WORD_KEY_NOT_UNIQUE         (-5) /* FIXME: should be removed*/

/* XXX: these specific error should be hided 
 * and the caller just get SYSTEM_FAIL-like error message */
#define WORD_VARIABLE_DB_FAIL       (-10)
#define WORD_FIXED_DB_FAIL          (-11)
#define WORD_HASH_ADD_FAIL          (-20)
#define FILE_NOT_EXIST              (-21)

#define WORD_MEM_FAIL               (-30)

/* XXX: 외부 인터페이스용 구조체 외에는 모두 mod_lexicon.h등으로 옮겨져야 한다.
 *      즉, 외부에 드러나서는 안된다. --jiwon, 2002/11/23 Sat */
// 고정크기 data 를 다루기 위하여
typedef struct {
    uint8_t  block_idx  :8;  /* max 255 blocks */
    uint32_t offset     :24; /* block size of 16MB */
}__attribute__((packed)) word_offset_t;

typedef struct {
    uint32_t df;     /* document frequency */
    // 품사정보...
} word_attr_t;
// remove id & length......


typedef struct {
    word_offset_t  word_offset; /* 단어의 문자열을 가리킨다 */
    word_attr_t    word_attr;
    uint32_t       did;
} internal_word_t;

// 외부 인터페이스용 단어 구조체
typedef struct {
	// XXX: why MAX_WORD_LENGTH when there's MAX_WORD_LEN?(do we need this?) -- jiwon, 2002/11/13.
    char        string[MAX_WORD_LENGTH];
	uint32_t 	id;
    word_attr_t word_attr;
} word_t;

typedef struct {
	uint32_t	used_bytes;
	char		data[BLOCK_DATA_SIZE];
} word_block_t;


typedef struct {
    uint32_t    last_wordid; /* last wordid */
    int         alloc_fixed_block_num;
    int         alloc_variable_block_num;
} word_db_shared_t;

typedef struct {
	word_block_t		*fixed_block[MAX_BLOCK_NUM];
	word_block_t		*variable_block[MAX_BLOCK_NUM];
	word_db_shared_t	*shared;
	void				*hash;
	void				*hash_data;
	void				*truncation_db;
	char 				path[MAX_PATH_LEN];
} word_db_t;

SB_DECLARE_HOOK(word_db_t*,get_global_word_db,(void))

SB_DECLARE_HOOK(int,open_word_db,(word_db_t *word_db, char *path, int flag))
SB_DECLARE_HOOK(int,synchronize_wordids,(word_db_t *word_db))
SB_DECLARE_HOOK(int,sync_word_db,(word_db_t *word_db))
SB_DECLARE_HOOK(int,close_word_db,(word_db_t *word_db))

SB_DECLARE_HOOK(int,get_new_word,(word_db_t *word_db, word_t *word, uint32_t did))
SB_DECLARE_HOOK(int,get_word,(word_db_t *word_db, word_t *word))
SB_DECLARE_HOOK(int,get_word_by_wordid,(word_db_t *word_db, word_t *word ))
SB_DECLARE_HOOK(int,get_num_of_word,(word_db_t *word_db, uint32_t *number))

SB_DECLARE_HOOK(int,get_right_truncation_words,(word_db_t *word_db, char *word, 
												word_t ret_word[], int max_search_num))
SB_DECLARE_HOOK(int,get_left_truncation_words,(word_db_t *word_db, char *word, 
											   word_t ret_word[] , int max_search_num))

SB_DECLARE_HOOK(int,clear_wordids,(word_db_t *word_db))
	
SB_DECLARE_HOOK(int,print_hash_bucket,(word_db_t *word_db, int block_idx))
SB_DECLARE_HOOK(int,print_hash_status,(word_db_t *word_db))
SB_DECLARE_HOOK(int,print_truncation_bucket,(word_db_t *word_db, int block_idx))

extern word_db_t gWordDB;

#endif
