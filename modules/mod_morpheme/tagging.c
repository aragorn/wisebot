/* $Id$ */
/**
 * @file tagging.c
 * @brief UnitMorphologicalAnalyzer application core.
 *
 * required library
 *	-# Koma.a 
 *	-# Tag.a
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "koma/KomaAPI.h"
#include "koma/HanTagAPI.h"
#include "koma/nlp.def"

//from nextsoftbot "constants.h"
//
#define MAX_WORD_LEN			(40)			/**< 1-morpheme size limit */
#define DOCUMENT_SIZE			(700*1024)		/**< document size limit */

#define	MaxTokenNum		1024*16					/**< max morpheme array size limit */
#define MAX_TAG_SIZE	6						/**< tag info size limit */

#define MainFSTFilename "../../dat/koma/main.FST"				/**< 사전 FST 파일 */
#define MainDataFilename "../../dat/koma/main.dat"				/**< 사전 dat 파일 */
#define ConnectionTableFilename "../../dat/koma/connection.txt" /**< 품사 접속 정보 파일 */
#define TagFilename "../../dat/koma/tag.nam"					/**< 품사 파일 */
#define TagOutFilename "../../dat/koma/tagout.nam"				/**< 출력 품사 파일 */
#define ProbEntryFilename "../../dat/koma/prob.FST"			/**< 확률정보 FST 파일 */
#define ProbDataFilename "../../dat/koma/prob.dat"				/**< 확률정보 dat 파일 */

/** indexer 에게 전달하는 자료구조 */
typedef struct _index_word_t {
	long position;				/**< 토큰 속성 ; 한글, 영어, 특수문자, 종결토큰 등등..		*/
	long len;					/**< 토큰 길이		*/
	char string[MAX_WORD_LEN];	/**< 토큰 저장공간		*/
} index_word_t;

/** 품사 태깅 결과 정리하는 자료구조 */
typedef struct _out_word_t {
	long position;				/**< position in document		*/
	long len;					/**< word size		*/
	char string[MAX_WORD_LEN];	/**< word		*/
	char tag[MAX_TAG_SIZE];		/**< word tag		*/
} out_word_t;

/**
 * main function 
 * 	-# load global variable : LoadKomaEngine(), LoadHanTagEngine()
 * 	-# analyzer sentence : DoKomaAndHanTag()
 * 	-# build out_word_t
 * 	-# convert index_word_t
 * 	-# return output
 */
int main(int argc, char *argv[])
{
    int i, j, k, kk, status=0;
	int	position=0;
	int	count=0;
	int token_num=0;
    char Sentence[DOCUMENT_SIZE];
    char *Wrd[MaxNumWrd];
    char *Result[MaxNumWrd][MaxNumAmb];
	char tmp[MaxNumWrd];
    void *HanTag;
	out_word_t		*out= NULL;
//	index_word_t	*tout= NULL;

    if (LoadKomaEngine
        (MainFSTFilename, MainDataFilename, ConnectionTableFilename,
         TagFilename, TagOutFilename) == false) {
        fprintf(stderr, "ERROR :: cannot load KOMA engine\n");
        return 1;
    }

    if (LoadHanTagEngine(ProbEntryFilename, ProbDataFilename) == false) {
        fprintf(stderr, "ERROR :: cannot load HanTag engine\n");
        return 1;
    }

    if ((HanTag = CreateHanTag()) == NULL) {
        fprintf(stderr, "ERROR :: cannot make HanTag instance\n");
        return 1;
    }

	token_num = MaxTokenNum;
	out = (out_word_t *)sb_malloc(token_num*sizeof(out_word_t));
    while (fscanf(stdin, "%s", Sentence) != EOF ) {
        DoKomaAndHanTag(HanTag, PATH_BASED_TAGGING, Sentence, Wrd, Result);
		for (i = 0; Result[i][0]; i++) {
			j = k = kk = 0;
			status = 0;
			strcpy(tmp, Result[i][0]);
			// Result -> out_word_t convert
			while( status != -1 ) {
				if (tmp[j] != '\0' && tmp[j] != '/' && tmp[j] != '+' && status == 0) {
					out[count].string[k] = tmp[j];
					k++;
					j++;
				} else if (tmp[j] != '\0' && tmp[j] != '/' && tmp[j] != '+' && status == 1) {
					out[count].tag[kk] = tmp[j];
					kk++;
					j++;
				} else if (tmp[j] == '/') {
					out[count].string[k] = '\0';
					out[count].len = k;
					out[count].position = (long)position;
					status= 1;
					j++;
				} else if (tmp[j] == '+') {
					out[count].tag[kk] = '\0';
					count++;
					k = kk = 0;
					status = 0;
					j++;
				} else if (tmp[j] == '\0') {
					out[count].tag[kk] = '\0';
					count++;
					k = kk = 0;
					status = -1;
					j++;
				}

				// overflow check
				if(count > token_num) {
					token_num += MaxTokenNum;
					out = sb_realloc(out, token_num*sizeof(out_word_t));
				}
			}
			position++;
		}
    }

	for(i=0;i<count;i++) {
			printf(" word : %s ", out[i].string);
			printf(" tag : %s ", out[i].tag);
			printf(" position : %ld ", out[i].position);
			printf(" len : %ld \n", out[i].len);
	}

	printf(" total token : %d  memory allocation size : %d \n", count, token_num);

	// out_word_t -> index_word_t convert 
	
	if(out != NULL) sb_free(out);

    FreeHanTag(HanTag);

    EndHanTagEngine();
    EndKomaEngine();
    return 0;
}
