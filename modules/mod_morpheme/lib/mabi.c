/*
 * $Id$
 *  MABi.C
 *  2001.05.  BY JaeBum, Kim.
 *
 *  modified by gramo, nominam
*/
#include	<stdio.h>
#include	<stdlib.h>
#include	"old_softbot.h"
#include	"lb_std.h"
#include	"lb_lex.h"
#include	"ma_code.h"
#include	"ma_data.h"
#include	"mabi.h"
#include	"mabi_kor.h"
#include	"ma_eng.h"

/*  Member Variable  */
CMA	*m_pMA;
char m_szTmp[MA_WORD_LEN + 4];

/*********************************************************/
void	MABIinit(char *pszMAPH)
{
	MADinit(pszMAPH);

	return;
}

/*********************************************************/
void	MABIend()
{
	MADend();

	return;
}

/*********************************************************/
TLONG	MABIanal(CTok *pTok, CMA *pMA, TLONG lTokCnt)
{
	m_pMA = pMA;

	m_pMA->shCntSL = 0;

	switch ( pTok->byTokID )
	{
	case TOK_KOR:
#ifdef _MA_KOR_
		MABIKanal(m_pMA, pTok->szTok);
#else
		printf("TOK_KOR : %s\n", pTok->szTok);
#endif
		break;
	case TOK_ENG:
#ifdef _MA_ENG_
		MAEanal(m_pMA, pTok->szTok);
#else
		printf("TOK_ENG : %s\n", pTok->szTok);
#endif
		break;
	case TOK_ENG_HETERO:
		strcpy(m_pMA->aMASL[m_pMA->shCntSL].szWord, pTok->szTok);
		m_pMA->aMASL[m_pMA->shCntSL].wPosTag = MA_ENG_HETERO;
		m_pMA->shCntSL += 1;
		break;
	case TOK_CHN:
		if ( !CDconvChn2Kor(pTok->szTok, m_szTmp) )
			break;
#ifdef _MA_KOR_
		MABIKanal(m_pMA, m_szTmp);
#else
		printf("TOK_CHN : %s\n", m_szTmp);
#endif
		break;
	case TOK_DIG:
		if ( strlen(pTok->szTok) > MA_DIG_LEN )
			break;
		else if ( lTokCnt > 1 )
		{
			strcpy(m_pMA->aMASL[m_pMA->shCntSL].szWord, pTok->szTok);
			m_pMA->aMASL[m_pMA->shCntSL].wPosTag = MA_DIG;
			m_pMA->shCntSL += 1;
		}
		break;
	}

	return m_pMA->shCntSL;
}

/*
**  END MABI.C
*/
