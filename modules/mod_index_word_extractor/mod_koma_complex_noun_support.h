/* $Id$ */
#ifndef MOD_KOMA_COMPLEX_NOUN_SUPPORT_H
#define MOD_KOMA_COMPLEX_NOUN_SUPPORT_H 1

#include "mod_api/index_word_extractor.h"
#include "mod_koma.h"

// FIXME: 
// mod_koma.c���� ��������. word�Ӽ��� �����ǵǸ� �����Ǿ��..
// from mod_koma_complex_noun_support.c
/* 
 * NNCG    :ü��(N):���(N):����(C):�Ϲ�(G) 
 * NNCV    :ü��(N):���(N):����(C):����(V) 
 * NNCJ    :ü��(N):���(N):����(C):�����(J) 
 * NNB     :ü��(N):���(N):����(B) 
 * NNBU    :ü��(N):���(N):����(B):����(U) 
 * NNP     :ü��(N):���(N):����(P) 
 * NN?     :ü��(N):���(N):����(?) 
*/

/* XXX : mod_koma.h include no need
#define TAG_IS(a, b, c)			(!strncmp((a), (b), (c))) 
#define TAG_IS_KYUKJOSA(a)		TAG_IS((a), "I", 1)
#define TAG_IS_NOUN(a)			(TAG_IS((a), "NNCG", 4) || \
								TAG_IS((a), "NNCV", 4) || \
								TAG_IS((a), "NNCJ", 4) || \
								TAG_IS((a), "NNB", 3) || \
								TAG_IS((a), "NNBU", 4) || \
								TAG_IS((a), "NNP", 3) || \
								TAG_IS((a), "NPP", 3) || \
								TAG_IS((a), "NPI", 3) || \
								TAG_IS((a), "NU", 2) || \
								TAG_IS((a), "UNK", 3) || \
								TAG_IS((a), "COMP", 4) )
*/
typedef struct {
	koma_handle_t *koma;
	int32_t last_position;
	index_word_t last_index_word;
	index_word_t last_index_full_word;
	int32_t last_index_full_word_distance;
	int8_t end_of_text;
} koma_complex_noun_support_t;

#endif
