/* $Id$ */
#ifndef MOD_KOMA_H
#define MOD_KOMA_H 1

#include "koma/KomaAPI.h"
#include "koma/HanTagAPI.h"
#include "koma/koma_global.h"

#define MAX_NUM_OF_MORPHEMES	(MaxNumWrd)
#define MAX_RESULT_NUM			(MaxNumAmb)

#define KOMA_TAG_LEN			(4)

// #define MODE_NEW_KOMA		// -죄 분리
#undef MODE_NEW_KOMA		// -죄 분리

#define TAG_IS(a, b, c)			(!strncmp((a), (b), (c))) 

#define TAG_IS_JUPDUSA(a,b)		(b == 2 && (TAG_IS((a), "NNCG", 4) || \
								TAG_IS((a), "NNCV", 4) || \
								TAG_IS((a), "NNCJ", 4) || \
								TAG_IS((a), "NNP", 3) || \
								TAG_IS((a), "NPP", 3) || \
								TAG_IS((a), "NPI", 3) || \
								TAG_IS((a), "NNB", 3) || \
								TAG_IS((a), "XPNN", 4)))

#ifdef MODE_NEW_KOMA
#define TAG_IS_JUPMISA(a,b)		(b == 2 && (TAG_IS((a), "XSNN", 4) || \
								TAG_IS((a), "XSD", 3) || \
								TAG_IS((a), "NU", 2) || \
								TAG_IS((a), "DU", 2) || \
								TAG_IS((a), "EFC", 3) || \
								TAG_IS((a), "EFN", 3)))
#endif
#ifndef MODE_NEW_KOMA
#define TAG_IS_JUPMISA(a,b)		(b == 2 && (TAG_IS((a), "XSNN", 4) || \
								TAG_IS((a), "XSD", 3) || \
								TAG_IS((a), "NNCG", 4) || \
								TAG_IS((a), "NNCV", 4) || \
								TAG_IS((a), "NNCJ", 4) || \
								TAG_IS((a), "NNP", 3) || \
								TAG_IS((a), "NPP", 3) || \
								TAG_IS((a), "NPI", 3) || \
								TAG_IS((a), "NNB", 3) || \
								TAG_IS((a), "NU", 2) || \
								TAG_IS((a), "DU", 2) || \
								TAG_IS((a), "EFC", 3) || \
								TAG_IS((a), "EFN", 3)))
#endif

#define TAG_IS_JOSA(a)			(TAG_IS((a), "PS", 2) || \
								TAG_IS((a), "PC",2) || \
								TAG_IS((a), "PO", 2) || \
								TAG_IS((a), "PD", 2) || \
								TAG_IS((a), "PA", 2) || \
								TAG_IS((a), "PV", 2) || \
								TAG_IS((a), "PN", 2) || \
								TAG_IS((a), "PX", 2)) 

#define TAG_IS_MUNJANGBUHO(a)	(TAG_IS((a), "SS.", 3) || \
								TAG_IS((a), "SS?", 3) || \
								TAG_IS((a), "SS!", 3) || \
								TAG_IS((a), "SS,", 3) || \
								TAG_IS((a), "SS/", 3) || \
								TAG_IS((a), "SS:", 3) || \
								TAG_IS((a), "SS;", 3) || \
								TAG_IS((a), "SS`", 3) || \
								TAG_IS((a), "SS'", 3) || \
								TAG_IS((a), "SS(", 3) || \
								TAG_IS((a), "SS)", 3) || \
								TAG_IS((a), "SS-", 3) || \
								TAG_IS((a), "SSA", 3) || \
								TAG_IS((a), "SSX", 3) ) 

#define TAG_IS_GAMTAN(a)		TAG_IS((a), "C", 1)

#define TAG_IS_KYUKJOSA(a)		TAG_IS((a), "I", 1)

#define	TAG_IS_SPACE(a)			TAG_IS((a), "SPAC", 4)

#define	TAG_IS_UMI(a)			(TAG_IS((a), "EFF", 3) || \
								TAG_IS((a), "EFC", 3) || \
								TAG_IS((a), "EFN", 3) || \
				 				TAG_IS((a), "EFD", 3) || \
				 				TAG_IS((a), "EFA", 3) || \
								TAG_IS((a), "EP", 2) )

#define TAG_IS_JUPSA(a)			(TAG_IS((a), "XSNN", 4) || \
								TAG_IS((a), "XSNP", 4) || \
								TAG_IS((a), "XSNU", 4) || \
								TAG_IS((a), "XPNN", 4) || \
								TAG_IS((a), "XPNU", 4) || \
								TAG_IS((a), "XSD", 3) || \
								TAG_IS((a), "XSA", 3) || \
								TAG_IS((a), "XSVV", 4) || \
								TAG_IS((a), "XSVJ", 4) )

#define TAG_IS_KWANHYUNGSA(a)	(TAG_IS((a), "DA", 2) || \
								TAG_IS((a), "DI", 2) || \
								TAG_IS((a), "DU", 2) )

#define TAG_IS_BUSA(a)			(TAG_IS((a), "AA", 2) || \
								TAG_IS((a), "AP", 2) || \
								TAG_IS((a), "AI", 2) || \
								TAG_IS((a), "AC", 2) || \
								TAG_IS((a), "AV", 2) || \
								TAG_IS((a), "AJ", 2) )

#define TAG_IS_VERBAL(a)		(TAG_IS((a), "VV", 2) || \
								TAG_IS((a), "VX", 2) || \
								TAG_IS((a), "VJ", 2) || \
								TAG_IS((a), "V?", 2) )

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

#define TAG_IS_ETC(a)			(TAG_IS((a), "SCF", 3) || \
								TAG_IS((a), "SCH", 3) || \
								TAG_IS((a), "SCD", 3) )

// XXX for move_text function
// assuming avg. word len 3, 256 is max white space-tokenized length by mod-koma
#define MOD_KOMA_SENTENCE_LEN	 (256*3)
#define IS_WHITE_CHAR(c)	( (c)==' ' || (c)=='\n' || (c)=='\r' || (c)=='\t')

// XXX for koma_analyzer function
// 색인할 단어에서 제외되는 품사들 정의
#define TAG_TO_BE_IGNORED_MORE(tag)	( TAG_IS_JOSA(tag) || TAG_IS_MUNJANGBUHO(tag) \
		 						|| TAG_IS_UMI(tag) || TAG_IS_JUPSA(tag) \
								|| TAG_IS_GAMTAN(tag) || TAG_IS_KYUKJOSA(tag) \
								|| TAG_IS_KWANHYUNGSA(tag) || TAG_IS_BUSA(tag) )
					
#define TAG_TO_BE_IGNORED(tag) 	( TAG_IS_GAMTAN(tag) || TAG_IS_MUNJANGBUHO(tag) \
								|| TAG_IS_UMI(tag) || TAG_IS_JOSA(tag) )

typedef	struct koma_handle_t {
	char *Wrd[MAX_NUM_OF_MORPHEMES];
	int  bPos[MAX_NUM_OF_MORPHEMES];
	char *Result[MAX_NUM_OF_MORPHEMES][MAX_RESULT_NUM];
	int  result_count;
	void *HanTag;
	int	 position;
	int	 current_index;
	int  current_length;
	int	 next_length;
	char *orig_text;	// 원래 text point
	char *text;			// 현재 분석할 text 
	char *next_text;	// 다음에 분석할 text start point
	int  koma_done;
	int  current_bytes_position;
    int  is_raw_koma_text; // koma를 실행한 원본 결과 추출
} koma_handle_t;

extern koma_handle_t* new_koma();
extern void delete_koma(koma_handle_t *handle);
extern void koma_set_text(koma_handle_t* handle, char* text);
extern int koma_analyze(koma_handle_t *handle, index_word_t *out, int max);

#endif
