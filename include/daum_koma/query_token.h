/* ======================================================================
 *  * KOREAN 토큰 타입 정의
 *   * ====================================================================== */
#define MAXTOKENLEN     256

#define TOKEN_NULL			0	/* no more token */
#define TOKEN_KOREAN		1	/* 모든 문자가 한글 */
#define TOKEN_KOR_HANJA		2	/* 모든 문자가 한자 */
#define TOKEN_KOR_JAPAN_H	3	/* 모든 문자가 일본어 */
#define TOKEN_KOR_JAPAN_K	4	/* 모든 문자가 일본어 */
#define TOKEN_ENGLISH		5	/* 모든 문자가 영어 */
#define TOKEN_ENGLISH2		6	/* 영어 중간에 '.','&'이 나타나는 토큰 */
#define TOKEN_DIGIT			7	/* 모든 문자가 숫자 */
#define TOKEN_DIGIT2		8	/* 숫자 중간에 '.' 또는 ':'이 나타나는 토큰 */
#define TOKEN_MIX_KOR_DGT	9	/* 한글과 숫자가 혼합 */
#define TOKEN_MIX_KOR_ENG	10	/* 한글과 영어가 혼합 */
#define TOKEN_MIX_ENG_DGT	11	/* 영어와 숫자가 혼합 */
#define TOKEN_MIX_ENG_KOR	12	/* 영어와 한글이 혼합 */
#define TOKEN_MIX_DGT_KOR	13	/* 숫자와 한글이 혼합 */
#define TOKEN_MIX_DGT_ENG	14	/* 숫자와 한글이 혼합 */

#define TOKEN_SPECIAL		15	/* Special Character */
#define TOKEN_CONTROL		16	/* Control Key */
#define TOKEN_BLANK			17	/* White Spaces */
#define TOKEN_UNKNOWN1		18	/* 1-byte Unknown Characters */
#define TOKEN_UNKNOWN2		19	/* 2-byte Unknown Characters */
#define TOKEN_MIX_UK_KOR	20	/* 2-byte Unknown Characters 와 한글조사의 혼합*/
#define TOKEN_FUNC_BLANK	21	/* Special Character alike blank (. , " etc) */

