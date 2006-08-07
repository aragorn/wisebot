/**
 * FILE : dema.h
 */


#ifndef __MORAN_H__
#include "moran.h"
#endif

#define AFFIX_MAX_SIZE	16
#define ROOT_MAX_SIZE	64
#define TOKEN_MAX_SIZE	128

#define NO_SUFFIX_TYPE		0
#define ES_PL_TYPE			1
#define S_POSS_TYPE			2
#define S_PL_TYPE			3
#define ED_PAST_TYPE		4
#define ER_COMP_TYPE		5
#define ING_GERUND_TYPE		6
#define EST_SUP_TYPE		7


typedef struct _word {
	char prefix[AFFIX_MAX_SIZE];
	char suffix[AFFIX_MAX_SIZE];
	char root[ROOT_MAX_SIZE];
} EngWord;

int startDaumEMA (const char* pFile, const char* sFile, const char* dFile, const char* iFile);
void endDaumEMA();
//int analyzeEnglishWord (char* word, EngWord* buf);		// 최종적으로는 삭제..
int HANL_AnalyzeEnglish(FINAL_INFO* info, char* inStr, int tokenType);

