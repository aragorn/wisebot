/* $Id$ */
#ifndef LEXICON_H
#define LEXICON_H 1

#include "softbot.h"
#include "hook.h"

// lexicon 의 return value
#define WORD_OLD_REGISTERED         (11)
#define WORD_NEW_REGISTERED         (12)
#define WORD_NOT_REGISTERED         (-11)

/* XXX: 외부 인터페이스용 구조체 외에는 모두 mod_lexicon.h등으로 옮겨져야 한다.
 *      즉, 외부에 드러나서는 안된다. --jiwon, 2002/11/23 Sat */

// 외부 인터페이스용 단어 구조체
typedef struct {
    char        string[MAX_WORD_LEN+1];
	uint32_t 	id;
} word_t;

typedef struct _word_db_t {
	int set;
	void* db;
} word_db_t;

#define MAX_WORD_DB_SET (10)

SB_DECLARE_HOOK(int,open_word_db,(word_db_t** word_db, int opt))
SB_DECLARE_HOOK(int,sync_word_db,(word_db_t* word_db))
SB_DECLARE_HOOK(int,close_word_db,(word_db_t* word_db))

SB_DECLARE_HOOK(int,get_new_wordid,(word_db_t* word_db, word_t *word))
SB_DECLARE_HOOK(int,get_word,(word_db_t* word_db, word_t *word))
SB_DECLARE_HOOK(int,get_word_by_wordid,(word_db_t* word_db, word_t *word ))
SB_DECLARE_HOOK(int,get_num_of_word,(word_db_t* word_db, uint32_t *number))

#endif
