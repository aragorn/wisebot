#include "common_core.h"
#include "common_util.h"
#include "utf8_analyzer.h"
#include <string.h>


static int check_bom(unsigned char* header);
static int bigram_word_add(char* output, int* output_idx, int output_len,
                    char* cur_word, char* pre_word);
/*
 * uni_type_index �м��� ����
 *
 * in : ����
 * out : �ڵ�
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
 * uni_type_index �м��� �Ҹ�
 *
 * in : �ڵ�
 * out : ����
 */
void utf8_destroy(unia_t* unia) {
    if( unia != NULL ) {
        free(unia);
    }
}

/*
 * byte order mark check.
 * http://ko.wikipedia.org/wiki/����Ʈ_����_ǥ��
 *
 * in : ���� ���� 3byte
 * out : 0 : ����
 *       1 : ����
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
 * �α��ھ� �����Ѵ�.(bigram)
 *
 * in :
 *    output : ��¹��� 
 *    output_idx : ��¹����� �Է� ��ġ
 *    output_len : ��¹����� ����
 *    cur_word : ���� ����
 *    pre_word : ���� ����
 *
 * out : 0 : ����
 *       1 : ����
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
 * ���� : http://ko.wikipedia.org/wiki/UTF-8
 * �����ڵ� ��ū �и�
 * 1. white space�� �������� �Ͽ� �ܾ bigrame���� �и�
 *
 * in :
 * - unia : �ڵ�
 * - input : utf-8 ���ڿ�(null terminated string)
 * - ouput : space�� ���е� ��ū array
 * - output_len : output buffer�� ����
 *
 * out : 0 : ���� 
 *       -1 : ���ۿ����÷ο�(���۱��� ������ �м�)
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

    /* bom üũ�Ͽ� �����ϸ� skip */
    if( check_bom( sp ) ) {
        sp += 3;
    }

    for (i = 0; *sp != '\0'; i++) {
        //debug( "scan bytes[%d]sp[%x], cur_char_len[%d], cur_char_idx[%d]\n\n", i, *sp, cur_char_len, cur_char_idx );
        //sleep(1);
        
        /* ���ڰ� ���������� �ϼ��Ҷ����� loop */
        if( started_char ) {
            cur_word[cur_char_idx] = *sp;
            
			/* ���ڰ� �Ϸ�� */
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
        
        /* ������ ���� byte�� üũ�Ͽ� ������ ���� üũ */
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
                 /* 0x1000(�ֻ��� ��Ʈ 1) �� �ƴϸ� 1byte char */
                if( (*sp & 0x80) !=  0x80 ) {
                    started_char = 1;
                    cur_char_len = 1;
                    cur_char_idx = 0;
                } else {
                 /* 0x1000 �̸� utf8���ڰ� �̴� */
                    started_char = 1;
                    cur_char_len = 1;
                    cur_char_idx = 0;
                    warn("what is charset????[%x][%x]\n", *sp, checkbyte);
                }
            break;
        }
    }

    // �ܱ����� ��� ���ξ�� ä��
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
