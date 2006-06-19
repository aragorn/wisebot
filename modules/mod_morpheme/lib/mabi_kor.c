/* $Id$ */
/*
**  MA_KOR.C
**  2002. 01.  BY JaeBum, Kim.
*/
#include "common_core.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	"ma_code.h"
#include	"ma_data.h"
#include	"mabi.h"
#include	"mak_data.h"
#include	"mabi_kor.h"

/*  
**			MEMBER
*/
CJamo	m_JamoHF;	/*  Head First  */
CJamo	m_JamoHE;	/*  Head End    */
CJamo	m_Jamo;		/*  Tmp Jamo    */
CJamo	m_Jamo2;

/*********************************************************/
void MAKDbigram( CMA * pMA, TBYTE *pszTok )
{
	int i;
	int lLen = strlen(pszTok);

	for( i=0 ;  i < lLen-2 ; i+=2 )
	{
		strncpy(pMA->aMASL[i/2].szWord,pszTok+i,4);
		pMA->aMASL[i/2].szWord[4]=0x00;
		pMA->shCntSL++;
	}

	if (lLen == 2) // XXX : 한글 1글자 유효.. e.g) 제1조1항 ; 제 , 조, 항 을 색인 하기 위해서..
	{
		strncpy(pMA->aMASL[0].szWord,pszTok,2);
		pMA->aMASL[0].szWord[2]=0x00;
		pMA->shCntSL=1;
	}

	return;
}

/*********************************************************/
void	MABIKanal(CMA *pMA, TBYTE *pszTok)
{
	MAKDbigram( pMA , pszTok);

	return;
}

/*
**  END MA_KOR.C
*/
