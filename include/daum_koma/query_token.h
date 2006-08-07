/* ======================================================================
 *  * KOREAN ��ū Ÿ�� ����
 *   * ====================================================================== */
#define MAXTOKENLEN     256

#define TOKEN_NULL			0	/* no more token */
#define TOKEN_KOREAN		1	/* ��� ���ڰ� �ѱ� */
#define TOKEN_KOR_HANJA		2	/* ��� ���ڰ� ���� */
#define TOKEN_KOR_JAPAN_H	3	/* ��� ���ڰ� �Ϻ��� */
#define TOKEN_KOR_JAPAN_K	4	/* ��� ���ڰ� �Ϻ��� */
#define TOKEN_ENGLISH		5	/* ��� ���ڰ� ���� */
#define TOKEN_ENGLISH2		6	/* ���� �߰��� '.','&'�� ��Ÿ���� ��ū */
#define TOKEN_DIGIT			7	/* ��� ���ڰ� ���� */
#define TOKEN_DIGIT2		8	/* ���� �߰��� '.' �Ǵ� ':'�� ��Ÿ���� ��ū */
#define TOKEN_MIX_KOR_DGT	9	/* �ѱ۰� ���ڰ� ȥ�� */
#define TOKEN_MIX_KOR_ENG	10	/* �ѱ۰� ��� ȥ�� */
#define TOKEN_MIX_ENG_DGT	11	/* ����� ���ڰ� ȥ�� */
#define TOKEN_MIX_ENG_KOR	12	/* ����� �ѱ��� ȥ�� */
#define TOKEN_MIX_DGT_KOR	13	/* ���ڿ� �ѱ��� ȥ�� */
#define TOKEN_MIX_DGT_ENG	14	/* ���ڿ� �ѱ��� ȥ�� */

#define TOKEN_SPECIAL		15	/* Special Character */
#define TOKEN_CONTROL		16	/* Control Key */
#define TOKEN_BLANK			17	/* White Spaces */
#define TOKEN_UNKNOWN1		18	/* 1-byte Unknown Characters */
#define TOKEN_UNKNOWN2		19	/* 2-byte Unknown Characters */
#define TOKEN_MIX_UK_KOR	20	/* 2-byte Unknown Characters �� �ѱ������� ȥ��*/
#define TOKEN_FUNC_BLANK	21	/* Special Character alike blank (. , " etc) */

