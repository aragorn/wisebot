/* $Id$ */
#ifndef MOD_PKOMA_H
#define MOD_PKOMA_H 1

#include "pkoma/KoMAApi.h"

#define MAX_NUM_OF_MORPHEMES	(5120) /* MaxNumWrd */
#define MAX_RESULT_NUM			(64)   /* MaxNumAmb */

/* This is for move_text() function.
 * Assuming that avg. word len 3, 256 is max number of 어절 by mod_pkoma.
 * 2001-??-??
 */
/* MAX_SENTENCE of koma2c/include/koma_global.h is now 10240 bytes
 * and MaxNumWrd (한 문장 내의 최대 어절 개수) is now 5120.
 * We can increase MOD_PKOMA_SENTENCE_LEN to 10240.
 * 2006-10-07 김정겸
 */
#define MOD_PKOMA_SENTENCE_LEN	 (1024*10)

#define PKOMA_TAG_LEN			(6)

#define TAG_IS(a, b, c)			(!strncmp((a), (b), (c))) 

/* NOTE: a = tag, b = token_len */
#define TAG_IS_JUPDUSA(a,b) ( TAG_IS((a), "fpp",  3) ) 
#define TAG_IS_JUPMISA(a,b) ( TAG_IS((a), "fps",  3) )

#define TAG_IS_JOSA(a)		   (TAG_IS((a), "fj", 2) )

#define TAG_IS_MUNJANGBUHO(a)  (TAG_IS((a), "g", 1) )

#define TAG_IS_GAMTAN(a)	   (TAG_IS((a), "K", 1) )

#define TAG_IS_KYUKJOSA(a)	   ( 0 )

#define	TAG_IS_SPACE(a)		   ( 0 )

#define	TAG_IS_UMI(a)		   (TAG_IS((a), "fm", 2))

#define TAG_IS_JUPSA(a)		   (TAG_IS((a), "fp", 2))

/* 접사 가운데, 명사를 만드는 접미사, '-적'을 골라낸다.
 * 즉, XSA, XSVV, XSVJ 등 용언을 만드는 접사를 판별할 수
 * 있게 도와준다.
 * -하다/XSVV 가 앞의 명사와 결합하여 복합명사/COMP를 만들어내지
 * 않아야 한다. --김정겸, 2006-09-25 */
#define TAG_IS_XPN_XSN_XSD(a)  (TAG_IS((a), "XSNN", 4) || \
								TAG_IS((a), "XSNP", 4) || \
								TAG_IS((a), "XSNU", 4) || \
								TAG_IS((a), "XPNN", 4) || \
								TAG_IS((a), "XPNU", 4) || \
								TAG_IS((a), "XSD",  3) )

#define TAG_IS_KWANHYUNGSA(a)  (TAG_IS((a), "SG", 2) )

#define TAG_IS_BUSA(a)			(TAG_IS((a), "SB", 2) )

#define TAG_IS_VERBAL(a)		(TAG_IS((a), "YB", 2) || \
                                 TAG_IS((a), "YA", 2) )

#define TAG_IS_NOUN(a)			(TAG_IS((a), "CMC", 3) || \
								 TAG_IS((a), "CMP", 3) || \
								 TAG_IS((a), "CSC", 3) || \
								 TAG_IS((a), "UM", 2) || \
								 TAG_IS((a), "UD",  2) )

#define TAG_IS_ETC(a)			(TAG_IS((a), "F", 1) )

#define IS_WHITE_CHAR(c)	( (c)==' ' || (c)=='\n' || (c)=='\r' || (c)=='\t')

// XXX for pkoma_analyzer function
/* 색인할 단어에서 제외되는 품사들 정의 */
#define TAG_TO_BE_IGNORED_MORE(tag)	\
                                 ( TAG_IS_JOSA(tag) \
                                || TAG_IS_MUNJANGBUHO(tag) \
		 						|| TAG_IS_UMI(tag) \
								|| TAG_IS_JUPSA(tag) \
								|| TAG_IS_GAMTAN(tag) \
								|| TAG_IS_KYUKJOSA(tag) \
								|| TAG_IS_KWANHYUNGSA(tag) \
								|| TAG_IS_BUSA(tag) )
					
#define TAG_TO_BE_IGNORED(tag) 	 ( TAG_IS_GAMTAN(tag) \
					            || TAG_IS_MUNJANGBUHO(tag) \
								|| TAG_IS_UMI(tag) \
								|| TAG_IS_JOSA(tag) \
								|| TAG_IS_KYUKJOSA(tag) )

//								|| (TAG_IS_JUPSA(tag) && (! TAG_IS_XPN_XSN_XSD(tag))) )

typedef	struct pkoma_handle_t {
	path_ptr tag_result;   /* 태깅 결과 */
	path_ptr current_path; /* 현재 처리 중인 어절 */
	unsigned char option; /* API 옵션 */
	int  eojeol_count; /* API_Tagger()의 리턴값 = 어절 갯수 */
	int  eojeol_index; /* 태깅 결과에서 현재 처리 중인 어절 위치 */

	int	 eojeol_position;  /* 본문에서 현재 어절 위치 */
	int  byte_position;    /* 본문에서 현재 바이트 위치 */
	int	 next_length;
	const char *orig_text;	// 원래 text point
	const char *next_text;	// 다음에 분석할 text start point
	char text[MOD_PKOMA_SENTENCE_LEN]; // 현재 분석할 text 
	int  text_length;
	int  is_completed;
    int  is_raw_koma_text; // koma를 실행한 원본 결과 추출
} pkoma_handle_t;

extern pkoma_handle_t* new_pkoma();
extern void delete_pkoma(pkoma_handle_t *handle);
extern void pkoma_set_text(pkoma_handle_t* handle, const char* text);
extern int pkoma_analyze(pkoma_handle_t *handle, index_word_t *out, int max);

#endif
