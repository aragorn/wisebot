/* $Id$ */
/*
**  MA.H
**  2001.05.  BY JaeBum, Kim.
*/
#ifndef _MA_H_
#define _MA_H_

#include "old_softbot.h"
#include "lb_lex.h"

#define MA_DIG_LEN	(5)
#define MA_WORD_LEN	(30)
#define MA_KOR_N	(0x0001)	/*  ü��(��� ���� ����)  */
#define MA_KOR_V	(0x0002)	/*  ���(�����,����)  */
#define MA_KOR_F	(0x0004)	/*  �����(����, ���, ����)  */
#define MA_KOR_UN	(0x0008)	/*  �̵�Ͼ�  */
#define MA_KOR_ALL	(0x00ff)
#define MA_KOR_KEY	(0x00fb)

#define MA_ENG			(0x0100)
#define MA_ENG_HETERO	(0x0200)
#define MA_DIG		(0x0400)
#define MA_NOISE	(0x0800)
#define MA_ENG_KEY		(0x0300)

#define MA_N		(0x0701)
#define MA_V		(0x0002)
#define MA_ALL		(0xffff)
#define MA_KEY		(MA_KOR_KEY | MA_ENG_KEY | MA_DIG)

typedef struct tagMASL
{
	TBYTE	szWord[MA_WORD_LEN];
	TWORD	wPosTag;
} CMASL;

#define MA_SL_NUM	(32)
typedef struct tagMA
{
	TSINT	shCntSL;
	TWORD	wMask;
	CMASL	aMASL[MA_SL_NUM];
} CMA;

#endif	/*  _MA_H_  */

void	MAinit(char *pszBasePH);
void	MAend();
TLONG	MAanal(CTok *pTok, CMA *pMA, TLONG lTokCnt);

/*
**  END MA.H
*/
