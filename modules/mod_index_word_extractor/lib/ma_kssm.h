/* $Id$ */
/*
**  MA_KSSM.H
**  2002.01.  BY JaeBum, Kim.
*/
#ifndef _MA_KSSM_
#define _MA_KSSM_

#include "old_softbot.h"

#define MAX_KSSM_NUM		(2350)
#define MAX_KSSM2_NUM		(51)

#define GET_KSSM_TB_SZ		(sizeof(TWORD) * MAX_KSSM_NUM)
#define GET_KSSM2_TB_SZ		(sizeof(TWORD) * MAX_KSSM2_NUM)

extern TWORD m_KssmTB[MAX_KSSM_NUM];

/* 한글 낱자 51자의 조합형 코드 */
extern TWORD m_Kssm2TB[MAX_KSSM2_NUM];

#define INFILL 0x441  /* less than 0x8000 */
#define VOFILL 0x441  /* less than 0x8000 */
#define FIFILL 0x441  /* less than 0x8000 */
#define KSFILL 0xd4   /* less than 0xff */
#define CON_SZ (4)
#define VOW_SZ (3)
#define CON_NUM (83+1) /* number of consonants */
#define VOW_NUM (29+1) /* number of vowels */

extern TWORD m_ConTB [CON_NUM] [CON_SZ];

/* vowel converting table : ( ? , TG code , K8 code ) */

extern TWORD m_VowTB [VOW_NUM] [VOW_SZ];

#endif	/*  _MA_KSSM_  */

/*
**  END MA_KSSM.H
*/
