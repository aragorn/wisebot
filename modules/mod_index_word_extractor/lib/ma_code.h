/* $Id$ */
/*
**  MA_CODE.H
**  2002.01.  BY JaeBum, Kim.
*/
#ifndef _MA_CODE_
#define _MA_CODE_

#include "old_softbot.h"

#include	"ma_chn.h"
#include	"ma_kssm.h"
#include	"ma_jamo.h"

#define CDM_GET_HIGH(a)		((a >> 8) & 0xff)
#define CDM_GET_LOW(a)		(a & 0xff)

#define CDM_MK_SYL(a, b)	( ((a << 8) & 0xff00) | (b & 0x00ff) )


TBOOL  CDconvChn2Kor(TBYTE *pszChn, TBYTE *pszKor);
TBOOL	CDconvKS2KSSM(TBYTE *ks , TBYTE *tg);
TBOOL	CDconvKSSM2KS(TBYTE *tg, TBYTE *ks);
void	CDconvKSSM2Jamo(TBYTE *pKssm, CJamo *pJamo);
void	CDconvJamo2KSSM(CJamo *pJamo, TBYTE *pKssm);
void	CDcopySyl(TBYTE *pKssm, TBYTE phi, TBYTE phm, TBYTE phf);

#endif	/*  _MA_CODE_  */
/*
**  END MA_CODE.H
*/
