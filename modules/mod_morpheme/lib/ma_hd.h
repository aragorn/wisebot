/* $Id$ */
/*
**  MA_HD.H
**  2001.05.  BY JaeBum, Kim.
*/
#ifndef _MA_HD_
#define _MA_HD_

#include "old_softbot.h"

#define MA_WORD_LEN	(32)

typedef struct tagMASL
{
	TBYTE	szWord[MA_WORD_LEN];
	TDWORD	dwPosTag;
} CMASL;

#define MA_SL_NUM	(32)
typedef struct tagPar
{
	TSINT	shCntSL;
	CMASL	aMASL[MA_SL_NUM];
} CMA;

#endif	/*  _MA_HD_  */

/*
**  END MA_HD.H
*/
