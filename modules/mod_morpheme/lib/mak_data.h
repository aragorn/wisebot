/* $Id$ */
/*
**  MAK_DATA.H
**  2002. 01.  BY JaeBum, Kim.
*/
#ifndef _MAK_DATA_
#define _MAK_DATA_

#include "old_softbot.h"

typedef struct tagHeadSL
{
	TBYTE	szWord[MA_WORD_LEN];
	TWORD	wPred;
	TDWORD	adwPos[3];
} CHeadSL;

#define HTS_HEAD_SL		(32)
typedef struct tagHeadBK
{
	TSINT	shTop;
	TSINT	shBottom;
	TSINT	shFlag;
	CHeadSL	aHeadSL[HTS_HEAD_SL];
} CHeadBK;

typedef struct tagMorBK
{
	TSINT	shCntSL;
	CHeadSL	aMorSL[HTS_HEAD_SL];
} CMorBK;

typedef struct tagAnalBK
{
	TSINT	shHeadLen;
	TSINT	shFormalLen;
	TSINT	shRealLen;
	TSINT	shTmpLen;
	char	isF;		/*  기능어  */
	char	isNRMB;		/*  ㄴ ㄹ ㅁ ㅂ */
	char	isJosa;
	char	isEomi;
	char	isXS;
	char	isXP;		/*  접두사  */
	char	isXI;
	char	Rsv;
	CHeadSL	*pHeadSL;
	CHeadSL	Formal;
	CHeadSL	Real;
	CHeadSL	TmpSL;
} CAnalBK;


/* state prediction for predictive morphological analysis. */
enum eSTATE
{
	S_DEF,
	S_N,
	S_V,
	S_F,
	S_NOISE
};

#endif	/*  _MAK_DATA_  */

/*  Global Variable  */
extern CMorBK		g_MorBK;
extern CAnalBK		g_AnalBK;

/*  Function Prototype  */
void	MAKDreset();
TBOOL	MAKDpushHead(TBYTE *pszWord, TWORD wPred);
TBOOL	MAKDpopHead();
void	MAKDpushMorph(CHeadSL *pHeadSL);
TBOOL	MAKDisFunctionWord(CJamo *pJamo);
TBOOL	MAKDisJosa(CJamo *pJamo);
TBOOL	MAKDisEomi(CJamo *pJamo);
TBOOL	MAKDisXP(CJamo *pJamo);
TBOOL	MAKDisXS(CJamo *pJamo);
TBOOL	MAKDisXI(CJamo *pJamo);
TBOOL	MAKDisNM(CJamo *pJamo);
TBOOL	MAKDisNRMB(CJamo *pJamo);
void	MAKDclrAnalBK();

/*
**  END MAK_DATA.H
*/
