/* $Id$ */
/*
**  MA_DIC.H
**  2002. 01.  BY JaeBum, Kim.
*/
#ifndef _MA_DIC_
#define _MA_DIC_

#include "old_softbot.h"

/*
			BASE.DIC
*/
/*********************************************************/
#define GET_BASE_BKNO(a)	((TLONG)STDhash(a) % (TLONG)MAX_BASE_BK)
#define GET_BASE_OFF(a)	((TLONG)a * (TLONG)sizeof(CBaseBK))

#define MAX_BASE_BK		(8903)
#define MAX_BASE_SL		(18)
#define MAX_BASE_LEN		(16)
typedef struct tagBaseSL
{
	char	szWord[MAX_BASE_LEN];
	TDWORD	POS[3];
} CBaseSL;
typedef struct tagBaseBK
{
	TLONG	lExtOff;
	TSINT	shCntSL;
	char	Rsv[2];
	CBaseSL	aBaseSL[MAX_BASE_SL];
} CBaseBK;	/*  512Byte  */

typedef struct tagBaseLN
{
	CBaseSL	BaseSL;
	struct tagBaseLN	*pNext;
} CBaseLN;


/*
**		BasePL
*/
/*********************************************************/
typedef struct tagBasePL
{
	TLONG	lPoolSZ;
	TLONG	lAlocSZ;
	char	*BasePL;
} CBasePL;

#endif	/*  _MA_DIC_  */

/*
** END MA_DIC.H
*/
