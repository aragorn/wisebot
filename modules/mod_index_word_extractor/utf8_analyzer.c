#include "common_core.h"
#include "common_util.h"
#include "utf8_analyzer.h"
#include <string.h>


static int check_bom(unsigned char* header);
static int bigram_word_add(char* output, int* output_idx, int output_len,
                    char* cur_word, char* pre_word);
/*
 * uni_type_index 분석기 생성
 *
 * in : 없음
 * out : 핸들
 */
unia_t* utf8_initialize()
{
    unia_t* uni_analyzer = (unia_t*)malloc( sizeof(unia_t) );
    if( uni_analyzer == NULL ) {
		crit("failed to malloc uni_analyzer");
        return NULL;
    }

    return uni_analyzer;
}

/*
 * uni_type_index 분석기 소멸
 *
 * in : 핸들
 * out : 없음
 */
void utf8_destroy(unia_t* unia) {
    if( unia != NULL ) {
        free(unia);
    }
}

/*
 * byte order mark check.
 * http://ko.wikipedia.org/wiki/바이트_순서_표식
 *
 * in : 문서 최초 3byte
 * out : 0 : 없음
 *       1 : 있음
 */
int check_bom(unsigned char* header) {
    if( strlen( header ) < 3 ) {
        return 0;
    } else if( header[0] == 0xEF &&
               header[1] == 0xBB &&
               header[2] == 0xBF ) {
        return 1;
    }

    return 0;
}

/*
 * 두글자씩 결합한다.(bigram)
 *
 * in :
 *    output : 출력버퍼 
 *    output_idx : 출력버퍼의 입력 위치
 *    output_len : 출력버퍼의 길이
 *    cur_word : 현재 문자
 *    pre_word : 이전 문자
 *
 * out : 0 : 실패
 *       1 : 성공
 */
int bigram_word_add(char* output, int* output_idx, int output_len,
                    char* cur_word, char* pre_word)
{
    if( *output_idx == (output_len-1) ) {
	    warn("output buffer overflower, output_idx[%d], output_len[%d]", *output_idx, output_len);
        return 0;
	}

	if( strlen( pre_word ) > 0 ) {
		if( *output_idx == (output_len-2) ) {
			warn("output buffer overflower, output_idx[%d], output_len[%d]", *output_idx, output_len);
			return 0;
		}

	    // two word add.
        strcpy( &output[*output_idx], pre_word );
        (*output_idx) += strlen(pre_word);

        strcpy( &output[*output_idx], cur_word );
        (*output_idx) += strlen(cur_word);

        output[*output_idx] = ' ';
        (*output_idx)++;

        output[(*output_idx)] = '\0';
	}
	
    return 1;
}

/*
 * 참고 : http://ko.wikipedia.org/wiki/UTF-8
 * 유니코드 토큰 분리
 * 1. white space를 구분으로 하여 단어를 bigrame으로 분리
 *
 * in :
 * - unia : 핸들
 * - input : utf-8 문자열(null terminated string)
 * - ouput : space로 구분된 토큰 array
 * - output_len : output buffer의 길이
 *
 * out : 0 : 성공 
 *       -1 : 버퍼오버플로우(버퍼길이 까지만 분석)
 */
int utf8_analyze(unia_t* unia, char* input, char* output, int output_len)
{
    unsigned char* sp = (unsigned char*)input;
    int started_char = 0;
    int cur_char_len = 0;
    int cur_char_idx = 0;
    int output_idx = 0;
    int i;

    char cur_word[5] = { 0 };
    char pre_word[5] = { 0 };

    /* bom 체크하여 존재하면 skip */
    if( check_bom( sp ) ) {
        sp += 3;
    }

    for (i = 0; *sp != '\0'; i++) {
        //debug( "scan bytes[%d]sp[%x], cur_char_len[%d], cur_char_idx[%d]\n\n", i, *sp, cur_char_len, cur_char_idx );
        //sleep(1);
        
        /* 문자가 시작했으면 완성할때까지 loop */
        if( started_char ) {
            cur_word[cur_char_idx] = *sp;
            
			/* 문자가 완료됨 */
            if(cur_char_idx == (cur_char_len-1))  { // 4byte char -> idx : 0 ~ 3
                started_char = 0;
                cur_word[cur_char_idx+1] = '\0';

				if( ! bigram_word_add( output, &output_idx, output_len, cur_word, pre_word ) ) {
					warn("can not add bigram words");
					return -1;
				}

				strcpy( pre_word, cur_word);
				debug("word[%s]", cur_word);
            }
            
            cur_char_idx++;
            sp++;
            continue;
        }
        
        unsigned char checkbyte = *sp & 0xF0;
        
        /* 문자의 시작 byte를 체크하여 문자의 길이 체크 */
        switch( checkbyte ) {
            case 0xF0: // 4byte char
                started_char = 1;
                cur_char_len = 4;
                cur_char_idx = 0;
                //debug("4bytes char[%x][%x]\n", *sp, checkbyte);
            break;
            case 0xE0: // 3byte char
                started_char = 1;
                cur_char_len = 3;
                cur_char_idx = 0;
                //debug("3bytes char[%x][%x]\n", *sp, checkbyte);
            break;
            case 0xC0: // 2byte char
                started_char = 1;
                cur_char_len = 2;
                cur_char_idx = 0;
                //debug("2bytes char[%x][%x]\n", *sp, checkbyte);
            break;
            default:
                 /* 0x1000(최상위 비트 1) 이 아니면 1byte char */
                if( (*sp & 0x80) !=  0x80 ) {
                    started_char = 1;
                    cur_char_len = 1;
                    cur_char_idx = 0;
                } else {
                 /* 0x1000 이면 utf8문자가 이님 */
                    started_char = 1;
                    cur_char_len = 1;
                    cur_char_idx = 0;
                    warn("what is charset????[%x][%x]\n", *sp, checkbyte);
                }
            break;
        }
    }

    // 외글자인 경우 색인어로 채택
    if( strlen(pre_word) == 0 ) {
	    // word add.
        strcpy( output[output_idx], cur_word );
        output_idx += strlen(cur_word);

        output[output_idx] = ' ';
        output_idx++;

        output[output_idx] = '\0';
	}

    return 1;
}

int main(int argc, char* argv, char* env) {
    char buffer[102400];
    char output[204800];
    int read_bytes = 0;
    
    memset( buffer, 0x00, 102400);
    
    read_bytes = fread( buffer, 102300, sizeof(char), stdin);
    
    unia_t* unia = utf8_initialize();
    
    utf8_analyze( unia, buffer, output, 0 );
    
    utf8_destroy( unia );

    printf("output[%s]\n", output);
    
    return 0;
}
