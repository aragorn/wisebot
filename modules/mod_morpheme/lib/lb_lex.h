/* $Id$ */
/*
**  LB_LEX.H
**  2001.05. BY JaeBum, Kim.
*/
#ifndef _LB_LEX_
#define _LB_LEX_

#include "old_softbot.h"

#define LEX_END		(-1)
#define LEX_CONT	(-2)

#define CH_1BYTE	(1)
#define CH_2BYTE	(2)
#define CH_KOREA	(3)
#define CH_CHINA	(4)

#define LEXM_IS_WHITE(a)	( (a == ' ') || (a == '\t') || (a == '\r') || (a == '\n') || (a == ':') )
#define LEXM_IS_EOS(a)		( (a == ',') || (a == '?') || (a == '!') || (a == ';') )
#define LEXM_IS_SENT(a)		( (a == '.') || (a == ',') || (a == '?') || (a == '!') || (a == ';') )

#define LEXM_IS_KOR(a)		( (a >= 0xb0) && (a <= 0xc8) )
#define LEXM_IS_ENG(a)		( ((a >= 'a') && (a <= 'z')) || ((a >= 'A') && (a <= 'Z')) )
#define LEXM_IS_CHN(a)		( (a >= 0xca) && (a <= 0xfd) )
#define LEXM_IS_JPN(a)		( (a == 0xaa) || (a == 0xab) )
#define LEXM_IS_DIG(a)		( (a >= '0') && (a <= '9') )
#define LEXM_IS_2BYTE(a)	( (a >= 0xa1) && (a <= 0xaf) )

typedef enum tagTokTP
{ TOK_KOR, TOK_ENG, TOK_ENG_HETERO, TOK_CHN, TOK_JPN, TOK_DIG } TokTP;

#define LEX_TOK_LEN		(30)
typedef struct tagTok
{
	char	byTokID;
	TBYTE	szTok[LEX_TOK_LEN];
} CTok;

#define LEX_TOK_STR_LEN		(256)
#define LEX_TOKS_NUM		(32)
typedef struct tagLex
{
	TLONG	lCmtBL;
	TLONG	lCmt;		/*  256Byte 단위  */
	TLONG	lPos;		/*  어절 단위(' ', \t, \r, \n)  */
	TBYTE	szTokStr[LEX_TOK_STR_LEN];
	TLONG	lTokStrLen;
	TLONG	lTokCnt;
	CTok	aTok[LEX_TOKS_NUM];
	TLONG	lBytePos;   /* byte 단위 위치 2002. 09. 10. */
} CLex;

typedef struct tagLexVar /* thread safe용 구조체 */
{
	// private : 
	TBYTE   *pszStr;
	TLONG   iStr;
	TBYTE   *pszTok;
	TLONG   iTok;
	// public : 
	CLex    *pLex;
} CLexVar;

#endif	/*  _LB_LEX_  */

void	LEXset(CLexVar *pLexHandle, CLex *pLex, TBYTE *pStr, TLONG lCmtBL, TLONG lPos);
TBOOL	LEXgetToks(CLexVar *pLexHandle);
void	LEXskipWhiteChar(TBYTE **pszQU);
TINT	LEXGetChar(FILE *fpSrc, char *pszTok);

/*
**  END LB_LEX.H
*/
