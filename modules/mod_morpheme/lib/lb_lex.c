/* $Id$ */
/*
**  LB_LEX.C
**  2001.05.  BY JaeBum, Kim.
*/
#include	<stdio.h>
#include	<stdlib.h>
#include	"old_softbot.h"
#include	"lb_lex.h"

/*
#ifdef 0
TBYTE	*m_pszStr;
TLONG	m_iStr;
TBYTE	*m_pszTok;
TLONG	m_iTok;
CLex	*m_pLex; 
#endif
*/

/*	Internal Function Prototype  */
TBOOL	LEXreadTokStr(CLexVar *pLexHandle);
void	LEXextractToks(CLexVar *pLexHandle);
void	LEXextractKOR(CLexVar *pLexHandle);
void	LEXextractCHN(CLexVar *pLexHandle);
void	LEXextractJPN(CLexVar *pLexHandle);
void	LEXextractDIG(CLexVar *pLexHandle);
void	LEXextractDIG_Period(CLexVar *pLexHandle, TINT *pni);
void	LEXextractENG(CLexVar *pLexHandle);
void	LEXextractENG_Period(CLexVar *pLexHandle, TINT *pni);
void	LEXextractENG_Empersent(CLexVar *pLexHandle, TINT *pni);
void	LEXextractENG_Plus(CLexVar *pLexHandle, TINT *pni);
void	LEXextractENG_Digit(CLexVar *pLexHandle, TINT *pni);


/*********************************************************/
void	LEXset(CLexVar *pLexHandle , CLex *pLex, TBYTE *pStr, TLONG lCmtBL, TLONG lPos)
{
	pLexHandle->pszStr = pStr;
	pLexHandle->iStr = 0;

	pLexHandle->pLex = pLex;
	pLexHandle->pLex->lCmtBL = lCmtBL;
	pLexHandle->pLex->lCmt = 0;
	pLexHandle->pLex->lPos = lPos;
	pLexHandle->pLex->lTokCnt = 0;

	return;
}

/*********************************************************/
TBOOL	LEXgetToks(CLexVar *pLexHandle)
{
	pLexHandle->pLex->lTokCnt = 0;

	if ( !LEXreadTokStr(pLexHandle) )
		return 0;

	LEXextractToks(pLexHandle);

	return 1;
}

/*********************************************************/
TBOOL	LEXreadTokStr(CLexVar *pLexHandle)
{
	pLexHandle->pLex->lTokStrLen = 0;

	for ( ; ; )
	{
		if ( !LEXM_IS_WHITE(pLexHandle->pszStr[pLexHandle->iStr]) )
			break;

		pLexHandle->iStr++;
	}

	for ( ; ; )
	{
		if ( !LEXM_IS_EOS(pLexHandle->pszStr[pLexHandle->iStr]) )
			break;

		pLexHandle->iStr++;
	}

	for ( ; ; )
	{
		if ( !LEXM_IS_WHITE(pLexHandle->pszStr[pLexHandle->iStr]) )
			break;

		pLexHandle->iStr++;
	}

	if ( pLexHandle->pszStr[pLexHandle->iStr] == 0x00 )
		return 0;

	if ( pLexHandle->pLex->lCmtBL != 0 )
		pLexHandle->pLex->lCmt = pLexHandle->iStr / pLexHandle->pLex->lCmtBL;

	pLexHandle->pLex->lBytePos = pLexHandle->iStr;

	for ( ; ; )
	{
		if ( pLexHandle->pszStr[pLexHandle->iStr] == 0x00 )
			break;
		if ( LEXM_IS_WHITE(pLexHandle->pszStr[pLexHandle->iStr]) || LEXM_IS_EOS(pLexHandle->pszStr[pLexHandle->iStr]) )
		{
			pLexHandle->pLex->lPos++;
			break;
		}
		
		if ( pLexHandle->pLex->lTokStrLen < (LEX_TOK_STR_LEN - 1) )
			pLexHandle->pLex->szTokStr[(pLexHandle->pLex->lTokStrLen)++] = pLexHandle->pszStr[pLexHandle->iStr];
		else
			break;

		pLexHandle->iStr++;
	}

	pLexHandle->pLex->szTokStr[pLexHandle->pLex->lTokStrLen] = 0x00;

	return 1;
}

/*********************************************************/
void	LEXextractToks(CLexVar *pLexHandle)
{
	pLexHandle->pszTok = pLexHandle->pLex->szTokStr;
	pLexHandle->iTok = 0;

	for ( ; ; )
	{
		if ( pLexHandle->pszTok[pLexHandle->iTok] == 0x00 )
			break;

		if ( pLexHandle->pLex->lTokCnt == LEX_TOKS_NUM )
			break;

		if ( LEXM_IS_KOR(pLexHandle->pszTok[pLexHandle->iTok]) )
			LEXextractKOR(pLexHandle);
		else if ( LEXM_IS_ENG(pLexHandle->pszTok[pLexHandle->iTok]) )
			LEXextractENG(pLexHandle);
		else if ( LEXM_IS_DIG(pLexHandle->pszTok[pLexHandle->iTok]) )
			LEXextractDIG(pLexHandle);
		else if ( LEXM_IS_CHN(pLexHandle->pszTok[pLexHandle->iTok]) )
			LEXextractCHN(pLexHandle);
		else if ( LEXM_IS_JPN(pLexHandle->pszTok[pLexHandle->iTok]) )
			LEXextractJPN(pLexHandle);
		else if ( LEXM_IS_2BYTE(pLexHandle->pszTok[pLexHandle->iTok]) )
		{
			pLexHandle->iTok++; 
			if ( pLexHandle->pszTok[pLexHandle->iTok] != 0x00 )
				pLexHandle->iTok++;
			else
				continue;
		}
		else
			pLexHandle->iTok++;
	} /* End For */

	return;
}

/*********************************************************/
void	LEXextractKOR(CLexVar *pLexHandle)
{
	TINT	i=0;

	if ( pLexHandle->pLex->lTokCnt == LEX_TOKS_NUM )
		return;

	pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].byTokID = TOK_KOR;

	for ( ; ; )
	{
		if ( i >= (LEX_TOK_LEN - 2) )
			break;

		pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[i++] =
			pLexHandle->pszTok[pLexHandle->iTok++];
		if ( pLexHandle->pszTok[pLexHandle->iTok] == 0x00 )
		{
			i--;
			break;
		}
		pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[i++] =
			pLexHandle->pszTok[pLexHandle->iTok++];

		if ( !LEXM_IS_KOR(pLexHandle->pszTok[pLexHandle->iTok]) )
			break;
	}

	pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[i] = 0x00;
	if ( i > 1 )
		pLexHandle->pLex->lTokCnt += 1;

	return;
}

/*********************************************************/
void	LEXextractCHN(CLexVar *pLexHandle)
{
	TINT	i=0;

	if ( pLexHandle->pLex->lTokCnt == LEX_TOKS_NUM )
		return;

	pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].byTokID = TOK_CHN;

	for ( ; ; )
	{
		if ( i >= (LEX_TOK_LEN - 2) )
			break;

		pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[i++] = pLexHandle->pszTok[pLexHandle->iTok++];
		if ( pLexHandle->pszTok[pLexHandle->iTok] == 0x00 )
		{
			i--;
			break;
		}
		pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[i++] = pLexHandle->pszTok[pLexHandle->iTok++];

		if ( !LEXM_IS_CHN(pLexHandle->pszTok[pLexHandle->iTok]) )
			break;
	}
	
	pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[i] = 0x00;
	if ( i > 1 )
		pLexHandle->pLex->lTokCnt += 1;

	return;
}

/*********************************************************/
void	LEXextractJPN(CLexVar *pLexHandle)
{
	TINT	i=0;

	if ( pLexHandle->pLex->lTokCnt == LEX_TOKS_NUM )
		return;

	pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].byTokID = TOK_JPN;

	for ( ; ; )
	{
		if ( i >= (LEX_TOK_LEN - 2) )
			break;

		pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[i++] =
			pLexHandle->pszTok[pLexHandle->iTok++];
		if ( pLexHandle->pszTok[pLexHandle->iTok] == 0x00 )
		{
			i--;
			break;
		}
		pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[i++] =
			pLexHandle->pszTok[pLexHandle->iTok++];

		if ( !LEXM_IS_JPN(pLexHandle->pszTok[pLexHandle->iTok]) )
			break;
	}

	pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[i] = 0x00;
	if ( i > 1 )
		pLexHandle->pLex->lTokCnt += 1;

	return;
}

/*********************************************************/
void	LEXextractDIG(CLexVar *pLexHandle)
{
	TINT	i=0;

	if ( pLexHandle->pLex->lTokCnt == LEX_TOKS_NUM )
		return;

	pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].byTokID = TOK_DIG;

	for ( ; ; )
	{
		if ( i >= (LEX_TOK_LEN - 2) )
			break;

		pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[i++] = pLexHandle->pszTok[pLexHandle->iTok++];

		if ( !LEXM_IS_DIG(pLexHandle->pszTok[pLexHandle->iTok]) )
		{
#ifdef _NOT_
			if ( pLexHandle->pszTok[pLexHandle->iTok] == '.' )
				LEXextractDIG_Period(pLexHandle, &i);
#endif
			break;
		}
	}

	pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[i] = 0x00;
	if ( i > 0 )
		pLexHandle->pLex->lTokCnt += 1;

	return;
}

/*********************************************************/
void	LEXextractDIG_Period(CLexVar *pLexHandle , TINT *pni)
{
	TINT	i=1, j;

	if ( !LEXM_IS_DIG(pLexHandle->pszTok[pLexHandle->iTok + 1]) )
		return;

	for ( ; ; )
	{
		if ( !LEXM_IS_DIG(pLexHandle->pszTok[pLexHandle->iTok + i]) )
			break;
		i++;
	}

	if ( i > 1 )
	{
		for ( j = 0; j < i; j++ )
			pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[(*pni)++] = pLexHandle->pszTok[pLexHandle->iTok + j];
	}

	pLexHandle->iTok += i;
	return;
}

/*********************************************************/
void	LEXextractENG(CLexVar *pLexHandle)
{
	TINT	i=0;

	if ( pLexHandle->pLex->lTokCnt == LEX_TOKS_NUM )
		return;

	pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].byTokID = TOK_ENG;

	for ( ; ; )
	{
		if ( i >= (LEX_TOK_LEN - 1) )
			break;

		pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[i++] = (TBYTE)toupper(pLexHandle->pszTok[pLexHandle->iTok++]);

		if ( !LEXM_IS_ENG(pLexHandle->pszTok[pLexHandle->iTok]) )
		{
			if ( pLexHandle->pszTok[pLexHandle->iTok] == '.' )
				LEXextractENG_Period(pLexHandle, &i);
			else if ( pLexHandle->pszTok[pLexHandle->iTok] == '&' )
				LEXextractENG_Empersent(pLexHandle, &i);
			else if ( pLexHandle->pszTok[pLexHandle->iTok] == '+' )
				LEXextractENG_Plus(pLexHandle, &i);
			else if ( LEXM_IS_DIG(pLexHandle->pszTok[pLexHandle->iTok]) )
				LEXextractENG_Digit(pLexHandle, &i);

			break;
		}
	}

	pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[i] = 0x00;
	if ( i > 0 )
		pLexHandle->pLex->lTokCnt += 1;

	return;
}

/*********************************************************/
void	LEXextractENG_Period(CLexVar *pLexHandle,TINT *pni)
{
	TINT	i=0, j;

	if ( *pni != 1 )
		return;

	for ( ; ; )
	{
		if ( pLexHandle->pszTok[pLexHandle->iTok + i] == '.' )
			i++;
		else
			break;
		if ( LEXM_IS_ENG(pLexHandle->pszTok[pLexHandle->iTok + i]) )
			i++;
		else
			break;
	}

	if ( i > 0 )
	{
		for ( j = 1; j <= i; j+=2 )
		{
			if ( (*pni) < (LEX_TOK_LEN - 1) )
				pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[(*pni)++] = (TBYTE)toupper(pLexHandle->pszTok[pLexHandle->iTok + j]);
			else
				break;
		}
	}

	if ( LEXM_IS_ENG(pLexHandle->pszTok[pLexHandle->iTok + i]) )
		pLexHandle->iTok += (i + 1);
	else
		pLexHandle->iTok += i;

	pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].byTokID = TOK_ENG_HETERO;

	return;
}

/*********************************************************/
void	LEXextractENG_Empersent(CLexVar *pLexHandle,TINT *pni)
{
	if ( *pni != 1 )
		return;

	if ( !LEXM_IS_ENG(pLexHandle->pszTok[pLexHandle->iTok + 1]) )
		return;

	if ( LEXM_IS_ENG(pLexHandle->pszTok[pLexHandle->iTok + 2]) )
		return;

	pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[(*pni)++] = pLexHandle->pszTok[pLexHandle->iTok];
	pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[(*pni)++] = (TBYTE)toupper(pLexHandle->pszTok[pLexHandle->iTok + 1]);

	pLexHandle->iTok += 2;
	pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].byTokID = TOK_ENG_HETERO;
	return;
}

/*********************************************************/
void	LEXextractENG_Plus(CLexVar *pLexHandle,TINT *pni)
{
	if ( *pni != 1 )
		return;

	if ( pLexHandle->pszTok[pLexHandle->iTok + 1] != '+' )
		return;
	if ( pLexHandle->pszTok[pLexHandle->iTok + 2] == '=' )
		return;

	pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[(*pni)++] = '+';
	pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[(*pni)++] = '+';

	pLexHandle->iTok += 2;
	pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].byTokID = TOK_ENG_HETERO;
	return;
}

/*********************************************************/
void	LEXextractENG_Digit(CLexVar *pLexHandle , TINT *pni)
{
	TINT	i=0, j;

	for ( ; ; )
	{
		if ( LEXM_IS_DIG(pLexHandle->pszTok[pLexHandle->iTok + i]) )
			;
		else if ( LEXM_IS_ENG(pLexHandle->pszTok[pLexHandle->iTok + i]) )
			return;
		else
			break;
		i++;
	}

	if ( (*pni + i) < LEX_TOK_LEN )
	{
		for ( j = 0; j < i; j++ )
			pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].szTok[(*pni)++] = pLexHandle->pszTok[pLexHandle->iTok + j];
	}

	pLexHandle->iTok += i;
	pLexHandle->pLex->aTok[pLexHandle->pLex->lTokCnt].byTokID = TOK_ENG_HETERO;

	return;
}

/*********************************************************/
void	LEXskipWhiteChar(TBYTE **pszQU)
{
	for ( ; ; )
	{
		if ( LEXM_IS_WHITE(**pszQU) )
		{
			(*pszQU)++;
			continue;
		}

		break;
	}

	return;
}

/*********************************************************/
TINT  LEXGetChar(FILE *fpSrc, char *pszTok)
{
	TINT	nCnt, nCh;

	for ( ; ; )
	{
		nCnt = 0;
		nCh = getc(fpSrc);
		if (nCh == EOF)
			return LEX_END;

		/*	KSC5601
		**
		**
		*/
		else if ((nCh >= 0x00) && (nCh <= 0xa0))	/* 0 ~ 160 */
		{
			pszTok[nCnt++] = (char)nCh;
			pszTok[nCnt] =0x00;
			return CH_1BYTE;
		}
		/*	KSC5601에서 특수 문자가 차지하는 범위
		**		0xA1A1 ~ 0xA1FE
		**			   ~
		**		0xACA1 ~ 0xACFE
		*/
		else if ((nCh >= 0xa1) && (nCh <= 0xac))	
		{
			pszTok[nCnt++] = (char)nCh;
			nCh = getc(fpSrc);
			if (nCh == EOF)
				return LEX_END;
			else if ((nCh >= 0xa1) && (nCh <= 0xfe))
			{
				pszTok[nCnt++] = (char)nCh;
				pszTok[nCnt] = 0x00;
				return CH_2BYTE;
			}
			else
			{
				ungetc(nCh, fpSrc);
				continue;
			}
		}
		/*	KSC5601에서 한글이 차지하는 범위
		**		0xB080 ~ 0xB0FF
		**			   ~
		**		0xC880 ~ 0xC8FF
		*/
		else if ((nCh >= 0xb0) && (nCh <= 0xc8))
		{  
			pszTok[nCnt++] = (char)nCh;
			nCh = getc(fpSrc);
			if (nCh == EOF)
				return LEX_END;
			else if ((nCh >= 0xa1) && (nCh <= 0xfe))
			{
				pszTok[nCnt++] = (char)nCh;
				pszTok[nCnt] = 0x00;
				return CH_KOREA;
			}
			else
			{
				ungetc(nCh, fpSrc);
				continue;
			}
		}
		/*	KSC5601에서 한문이 차지하는 범위 
		**		0xCA80 ~ 0xCAFF
		**			   ~
		**		0xFD80 ~ 0xFDFF
		*/
		else if ((nCh >= 0xca) && (nCh <= 0xfd))	/* China */
		{   
			pszTok[nCnt++] = (char)nCh;
			nCh = getc(fpSrc);
			if (nCh == EOF)
				return LEX_END;
			else if ((nCh >= 0xa1) && (nCh <= 0xfe))
			{
				pszTok[nCnt++] = (char) nCh;
				pszTok[nCnt] = 0x00;
				return CH_CHINA;
			}
			else
			{
				ungetc(nCh, fpSrc);
				continue;
			}
		}
		else
			continue;
	}
}


/*
**  END LB_LEX.C
*/
