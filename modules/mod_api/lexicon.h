/* $Id$ */
#ifndef LEXICON_H
#define LEXICON_H 1

#include "softbot.h"
#include "hook.h"

// lexicon �� return value
#define WORD_OLD_REGISTERED         (11)
#define WORD_NEW_REGISTERED         (12)
#define WORD_NOT_REGISTERED         (-11)

/* XXX: �ܺ� �������̽��� ����ü �ܿ��� ��� mod_lexicon.h������ �Ű����� �Ѵ�.
 *      ��, �ܺο� �巯������ �ȵȴ�. --jiwon, 2002/11/23 Sat */

// �ܺ� �������̽��� �ܾ� ����ü
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
