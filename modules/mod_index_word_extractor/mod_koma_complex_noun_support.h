/* $Id$ */
#ifndef MOD_KOMA_COMPLEX_NOUN_SUPPORT_H
#define MOD_KOMA_COMPLEX_NOUN_SUPPORT_H 1

#include "mod_api/index_word_extractor.h"
#include "mod_koma.h"

// FIXME: 
// mod_koma.c에서 가져왔음. word속성이 재정의되면 수정되어야..
// from mod_koma_complex_noun_support.c
/* 
 * NNCG    :체언(N):명사(N):보통(C):일반(G) 
 * NNCV    :체언(N):명사(N):보통(C):동사(V) 
 * NNCJ    :체언(N):명사(N):보통(C):형용사(J) 
 * NNB     :체언(N):명사(N):의존(B) 
 * NNBU    :체언(N):명사(N):의존(B):단위(U) 
 * NNP     :체언(N):명사(N):고유(P) 
 * NN?     :체언(N):명사(N):추정(?) 
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
