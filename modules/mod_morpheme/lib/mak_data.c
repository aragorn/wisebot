/* $Id$ */
/*
**  MAK_DATA.C
**  2002. 01.  BY JaeBum, Kim.
*/
#include	<stdio.h>
#include	<stdlib.h>
#include	"old_softbot.h"
#include	"ma_jamo.h"
#include	"ma.h"
#include	"ma_data.h"
#include	"mak_data.h"

/*
**			MEMBER
*/
CHeadBK		m_HeadBK;

/*  
**			GLOBAL
*/
CMorBK		g_MorBK;
CAnalBK		g_AnalBK;

/*********************************************************/
void	MAKDreset()
{
	m_HeadBK.shTop = m_HeadBK.shBottom = 0;
	g_MorBK.shCntSL = 0;

	return;
}

/*********************************************************/
TBOOL	MAKDpushHead(TBYTE *pszWord, TWORD wPred)
{
	if ( m_HeadBK.shFlag == 1 && m_HeadBK.shBottom == m_HeadBK.shTop )
		return 0;

	if ( strlen(pszWord) == 0 )
		return 1;

	strcpy(m_HeadBK.aHeadSL[m_HeadBK.shTop].szWord, pszWord);
	m_HeadBK.aHeadSL[m_HeadBK.shTop].wPred = wPred;
	m_HeadBK.shTop += 1;
	if ( m_HeadBK.shTop == HTS_HEAD_SL )
	{
		m_HeadBK.shTop = 0;
		m_HeadBK.shFlag = 1;
	}

	return 1;
}

/*********************************************************/
TBOOL	MAKDpopHead()
{
	if ( m_HeadBK.shFlag == 0 && m_HeadBK.shBottom == m_HeadBK.shTop )
		return 0;

	g_AnalBK.pHeadSL = &(m_HeadBK.aHeadSL[m_HeadBK.shBottom]);
	m_HeadBK.shBottom += 1;
	if ( m_HeadBK.shBottom == HTS_HEAD_SL )
	{
		m_HeadBK.shBottom = 0;
		m_HeadBK.shFlag = 0;
	}

	g_AnalBK.shHeadLen = strlen(g_AnalBK.pHeadSL->szWord);

	return 1;
}

/*********************************************************/
void	MAKDpushMorph(CHeadSL *pHeadSL)
{
	if ( g_MorBK.shCntSL < HTS_HEAD_SL )
	{
		memcpy(&(g_MorBK.aMorSL[g_MorBK.shCntSL]), pHeadSL, sizeof(CHeadSL));
		g_MorBK.shCntSL += 1;
	}

	return;
}

/*********************************************************/
TBOOL	MAKDisNM(CJamo *pJamo)
{
	if ( pJamo->i == PHI_G )
	{
		if ( pJamo->m == PHM_A ) 
		{
			if ( pJamo->f == PHF_M || pJamo->f == PHF_NG )
				return 1;
		}
		else if ( pJamo->m == PHM_O )
		{
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_NG )
				return 1;
		}
		else if ( pJamo->m == PHM_OA )
		{
			if ( pJamo->f == PHF_G )
				return 1;
		}
		else if ( pJamo->m == PHM_U )
		{
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_NG )
				return 1;
		}
		else if ( pJamo->m == PHM_UEO )
		{
			if ( pJamo->f == PHF_N )
				return 1;
		}
		else if ( pJamo->m == PHM_EU )
		{
			if ( pJamo->f == PHF_M )
				return 1;
		}
		else if ( pJamo->m == PHM_I )
		{
			if ( pJamo->f == PHF_R || pJamo->f == PHF_M )
				return 1;
		}
	}
	else if ( pJamo->i == PHI_N )
	{
		if ( pJamo->m == PHM_A )
		{
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_M )
				return 1;
		}
		else if ( pJamo->m == PHM_O )
		{
			if ( pJamo->f == PHF_FIL )
				return 1;
		}
	}
	else if ( pJamo->i == PHI_D )
	{
		if ( pJamo->m == PHM_O )
		{
			if ( pJamo->f == PHF_FIL )
				return 1;
		}
	}
	else if ( pJamo->i == PHI_M )
	{
		if ( pJamo->m == PHM_A )
		{
			if ( pJamo->f == PHF_FIL )
				return 1;
		}
		else if ( pJamo->m == PHM_AI )
		{
			if ( pJamo->f == PHF_NG )
				return 1;
		}
		else if ( pJamo->m == PHM_U )
		{
			if ( pJamo->f == PHF_N )
				return 1;
		}
		else if ( pJamo->m == PHM_I )
		{
			if ( pJamo->f == PHF_N )
				return 1;
		}
	}
	else if ( pJamo->i == PHI_B )
	{
		if ( pJamo->m == PHM_A )
		{
			if ( pJamo->f == PHF_G || pJamo->f == PHF_N || pJamo->f == PHF_NG )
				return 1;
		}
		else if ( pJamo->m == PHM_AI )
		{
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_G )
				return 1;
		}
		else if ( pJamo->m == PHM_YEO )
		{
			if ( pJamo->f == PHF_N )
				return 1;
		}
	}
	else if ( pJamo->i == PHI_S )
	{
		if ( pJamo->m == PHM_EO )
		{
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_G || pJamo->f == PHF_N || pJamo->f == PHF_R || pJamo->f == PHF_NG )
				return 1;
		}
		else if ( pJamo->m == PHM_O )
		{
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_N || pJamo->f == PHF_NG )
				return 1;
		}
		else if ( pJamo->m == PHM_I )
		{
			if ( pJamo->f == PHF_N || pJamo->f == PHF_M )
				return 1;
		}
	}
	else if ( pJamo->i == PHI_NG )
	{
		if ( pJamo->m == PHM_A )
		{
			if ( pJamo->f == PHF_N )
				return 1;
		}
		else if ( pJamo->m == PHM_YA )
		{
			if ( pJamo->f == PHF_NG )
				return 1;
		}
		else if ( pJamo->m == PHM_EO )
		{
			if ( pJamo->f == PHF_M )
				return 1;
		}
		else if ( pJamo->m == PHM_YEO )
		{
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_M )
				return 1;
		}
		else if ( pJamo->m == PHM_O )
		{
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_G )
				return 1;
		}
		else if ( pJamo->m == PHM_OA )
		{
			if ( pJamo->f == PHF_NG )
				return 1;
		}
		else if ( pJamo->m == PHM_U )
		{
			if ( pJamo->f == PHF_FIL )
				return 1;
		}
		else if ( pJamo->m == PHM_UEO )
		{
			if ( pJamo->f == PHF_N )
				return 1;
		}
		else if ( pJamo->m == PHM_UI )
		{
			if ( pJamo->f == PHF_FIL )
				return 1;
		}
		else if ( pJamo->m == PHM_YU )
		{
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_N )
				return 1;
		}
		else if ( pJamo->m == PHM_I )
		{
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_M )
				return 1;
		}
	}
	else if ( pJamo->i == PHI_J )
	{
		if ( pJamo->m == PHM_A )
		{
			if ( pJamo->f == PHF_NG )
				return 1;
		}
		else if ( pJamo->m == PHM_EO )
		{
			if ( pJamo->f == PHF_N || pJamo->f == PHF_M || pJamo->f == PHF_NG )
				return 1;
		}
		else if ( pJamo->m == PHM_O )
		{
			if ( pJamo->f == PHF_FIL )
				return 1;
		}
		else if ( pJamo->m == PHM_U )
		{
			if ( pJamo->f == PHF_FIL )
				return 1;
		}
		else if ( pJamo->m == PHM_I )
		{
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_N )
				return 1;
		}
	}
	else if ( pJamo->i == PHI_CH )
	{
		if ( pJamo->m == PHM_AI )
		{
			if ( pJamo->f == PHF_FIL )
				return 1;
		}
		else if ( pJamo->m == PHM_EO )
		{
			if ( pJamo->f == PHF_N )
				return 1;
		}
		else if ( pJamo->m == PHM_OI )
		{
			if ( pJamo->f == PHF_FIL )
				return 1;
		}
		else if ( pJamo->m == PHM_U )
		{
			if ( pJamo->f == PHF_FIL )
				return 1;
		}
	}
	else if ( pJamo->i == PHI_T )
	{
		if ( pJamo->m == PHM_A )
		{
			if ( pJamo->f == PHF_G )
				return 1;
		}
	}
	else if ( pJamo->i == PHI_P )
	{
		if ( pJamo->m == PHM_YO )
		{
			if ( pJamo->f == PHF_FIL )
				return 1;
		}
	}
	else if ( pJamo->i == PHI_H )
	{
		if ( pJamo->m == PHM_A )
		{
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_M )
				return 1;
		}
		else if ( pJamo->m == PHM_EO )
		{
			if ( pJamo->f == PHF_FIL )
				return 1;
		}
		else if ( pJamo->m == PHM_O )
		{
			if ( pJamo->f == PHF_NG )
				return 1;
		}
		else if ( pJamo->m == PHM_OA )
		{
			if ( pJamo->f == PHF_NG )
				return 1;
		}
	}

	return 0;
}

/*********************************************************/
TBOOL	MAKDisFunctionWord(CJamo *pJamo)
{
	if ( MAKDisJosa(pJamo) )
	{
		g_AnalBK.isJosa = 1;
		return 1;
	}
	if ( MAKDisXS(pJamo) )
	{
		g_AnalBK.isXS = 1;
		return 1;
	}
	if ( MAKDisEomi(pJamo) )
	{
		g_AnalBK.isEomi = 1;
		return 1;
	}

	return 0;
}

/*********************************************************/
TBOOL	MAKDisNRMB(CJamo *pJamo)
{
	if ( pJamo->f == PHF_N )
		return 1;
	else if ( pJamo->f == PHF_R )
		return 1;
	else if ( pJamo->f == PHF_M )
		return 1;
	else if ( pJamo->f == PHF_B )
		return 1;

	return 0;
}


/*********************************************************/
TBOOL	MAKDisJosa(CJamo *pJamo)
{
	if ( pJamo->i == PHI_G )
	{	
		if ( pJamo->m == PHM_A ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_O ) {
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_D ) return 1; }
		else if ( pJamo->m == PHM_OA ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
	}
	else if ( pJamo->i == PHI_GG )
	{
		if ( pJamo->m == PHM_EOI ) {
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_N ) return 1; }
	}
	else if ( pJamo->i == PHI_N )
	{
		if ( pJamo->m == PHM_A ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_EU ) {
			if ( pJamo->f == PHF_N ) return 1; }
		else if ( pJamo->m == PHM_I ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
	}
	else if ( pJamo->i == PHI_D )
	{
		if ( pJamo->m == PHM_A ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_O ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_U ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_EU ) {
			if ( pJamo->f == PHF_N ) return 1; }
	}
	else if ( pJamo->i == PHI_R )
	{
		if ( pJamo->m == PHM_A ) {
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_N || pJamo->f == PHF_NG ) return 1; }
		else if ( pJamo->m == PHM_YA ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_O ) {
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_N ) return 1; }
		else if ( pJamo->m == PHM_EU ) {
			if ( pJamo->f == PHF_R ) return 1; }
	}
	else if ( pJamo->i == PHI_M )
	{
		if ( pJamo->m == PHM_A ) {
			if ( pJamo->f == PHF_N ) return 1; }
		else if ( pJamo->m == PHM_YEO ) {
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_N ) return 1; }
	}
	else if ( pJamo->i == PHI_BB ) 
	{
		if ( pJamo->m == PHM_U ) {
			if ( pJamo->f == PHF_N ) return 1; }
	}
	else if ( pJamo->i == PHI_S )
	{
		if ( pJamo->m == PHM_A ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_EO ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_O ) {
			if ( pJamo->f == PHF_N ) return 1; }
	}
	else if ( pJamo->i == PHI_SS )
	{
		if ( pJamo->m == PHM_EO ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
	}
	else if ( pJamo->i == PHI_NG )
	{
		if ( pJamo->m == PHM_A ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_YA ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_EOI ) {
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_N || pJamo->f == PHF_R ) return 1; }
		else if ( pJamo->m == PHM_YEO ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_OA ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_YO ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_EU ) {
			if ( pJamo->f == PHF_N || pJamo->f == PHF_R ) return 1; }
		else if ( pJamo->m == PHM_EUI ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_I ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
	}

	return 0;
}

/*********************************************************/
TBOOL	MAKDisEomi(CJamo *pJamo)
{
	if ( pJamo->i == PHI_G )
	{	
		if ( pJamo->m == PHM_EO ) {
			if ( pJamo->f == PHF_N || pJamo->f == PHF_SS ) return 1; }
		else if ( pJamo->m == PHM_EOI ) {
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_SS ) return 1; }
		else if ( pJamo->m == PHM_O ) {
			if ( pJamo->f == PHF_N ) return 1; }
		else if ( pJamo->m == PHM_U ) {
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_N ) return 1; }
		else if ( pJamo->m == PHM_I ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
	}
	else if ( pJamo->i == PHI_N )
	{
		if ( pJamo->m == PHM_YA ) {
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_N ) return 1; }
		else if ( pJamo->m == PHM_EOI ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_O ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
	}
	else if ( pJamo->i == PHI_D )
	{
		if ( pJamo->m == PHM_A ) {
			if ( pJamo->f == PHF_N ) return 1; }
		else if ( pJamo->m == PHM_AI ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_EO ) {
			if ( pJamo->f == PHF_N ) return 1; }
		else if ( pJamo->m == PHM_EOI ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_OI ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_EU ) {
			if ( pJamo->f == PHF_S ) return 1; }
		else if ( pJamo->m == PHM_I ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
	}
	else if ( pJamo->i == PHI_R )
	{
		if ( pJamo->m == PHM_A ) {
			if ( pJamo->f == PHF_G || pJamo->f == PHF_R || 
				pJamo->f == PHF_M ) return 1; }
		else if ( pJamo->m == PHM_AI ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_YA ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_EO ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_YEO ) {
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_N || pJamo->f == PHF_M ) return 1; }
		else if ( pJamo->m == PHM_I ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
	}
	else if ( pJamo->i == PHI_M )
	{
		if ( pJamo->m == PHM_A ) {
			if ( pJamo->f == PHF_N ) return 1; }
		else if ( pJamo->m == PHM_AI ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
	}
	else if ( pJamo->i == PHI_S )
	{
		if ( pJamo->m == PHM_A ) {
			if ( pJamo->f == PHF_B ) return 1; }
		else if ( pJamo->m == PHM_EOI ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_YEO ) {
			if ( pJamo->f == PHF_SS ) return 1; }
		else if ( pJamo->m == PHM_O ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_U ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_YU ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_I ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
	}
	else if ( pJamo->i == PHI_NG )
	{
		if ( pJamo->m == PHM_A ) {
			if ( pJamo->f == PHF_SS ) return 1; }
		else if ( pJamo->m == PHM_EO ) {
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_SS ) return 1; }
		else if ( pJamo->m == PHM_OA ) {
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_B ) return 1; }
		else if ( pJamo->m == PHM_U ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_YU ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_EU ) {
			if ( pJamo->f == PHF_M ) return 1; }
	}
	else if ( pJamo->i == PHI_J )
	{
		if ( pJamo->m == PHM_A ) {
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_N || pJamo->f == PHF_B ) return 1; }
		else if ( pJamo->m == PHM_AI ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_EOI ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_YO ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_I ) {
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_N || pJamo->f == PHF_R ) return 1; }
	}

	return 0;
}

/*********************************************************/
TBOOL	MAKDisXP(CJamo *pJamo)
{
	if ( pJamo->i == PHI_G )
	{
		if ( pJamo->m == PHM_A ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_AI ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_O ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
	}
	else if ( pJamo->i == PHI_N )
	{
		if ( pJamo->m == PHM_A ) {
			if ( pJamo->f == PHF_R ) return 1; }
	}
	else if ( pJamo->i == PHI_D )
	{
		if ( pJamo->m == PHM_AI ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
	}
	else if ( pJamo->i == PHI_M )
	{
		if ( pJamo->m == PHM_A ) {
			if ( pJamo->f == PHF_D ) return 1; }
		else if ( pJamo->m == PHM_AI ) {
			if ( pJamo->f == PHF_N ) return 1; }
		else if ( pJamo->m == PHM_U ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
	}
	else if ( pJamo->i == PHI_B )
	{
		if ( pJamo->m == PHM_A ) {
			if ( pJamo->f == PHF_N ) return 1; }
		else if ( pJamo->m == PHM_EO ) {
			if ( pJamo->f == PHF_M ) return 1; }
		else if ( pJamo->m == PHM_U ) {
			if (pJamo->f == PHF_FIL || pJamo->f == PHF_R) return 1; }
		else if ( pJamo->m == PHM_I ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
	}
	else if ( pJamo->i == PHI_S )
	{
		if ( pJamo->m == PHM_AI ) {
			if ( pJamo->f == PHF_NG ) return 1; }
		else if ( pJamo->m == PHM_O ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_U ) {
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_S ) return 1; }
		else if ( pJamo->m == PHM_I ) {
			if ( pJamo->f == PHF_N ) return 1; }
	}
	else if ( pJamo->i == PHI_NG )
	{
		if ( pJamo->m == PHM_YA ) {
			if ( pJamo->f == PHF_NG ) return 1; }
		else if ( pJamo->m == PHM_OA ) {
			if ( pJamo->f == PHF_NG ) return 1; }
		else if ( pJamo->m == PHM_OAI ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_OI ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_U ) {
			if ( pJamo->f == PHF_S ) return 1; }
	}
	else if ( pJamo->i == PHI_J )
	{
		if ( pJamo->m == PHM_AI ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_EOI ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
	}
	else if ( pJamo->i == PHI_CH )
	{
		if ( pJamo->m == PHM_O ) {
			if ( pJamo->f == PHF_FIL || pJamo->f == PHF_NG ) return 1; }
		else if ( pJamo->m == PHM_OI ) {
			if ( pJamo->f == PHF_FIL ) return 1; }
		else if ( pJamo->m == PHM_I ) {
			if ( pJamo->f == PHF_N ) return 1; }
	}
	else if ( pJamo->i == PHI_P )
	{
		if ( pJamo->m == PHM_U ) {
			if ( pJamo->f == PHF_S ) return 1; }
	}

	return 0;
}

/*********************************************************/
TBOOL	MAKDisXS(CJamo *pJamo)
{
	if ( pJamo->i == PHI_G )
	{
		if ( pJamo->m == PHM_A ) {
			if ( pJamo->f == PHF_N ) return 1; }
		else if ( pJamo->m == PHM_YEO ) {
			if ( pJamo->f == PHF_NG ) return 1; }
		else if ( pJamo->m == PHM_EU ) {
			if ( pJamo->f == PHF_M ) return 1; }
	}
	else if ( pJamo->i == PHI_GG )
	{
		if ( pJamo->m == PHM_AI && pJamo->f == PHF_FIL )
			return 1;
		else if ( pJamo->m == PHM_EO && pJamo->f == PHF_S )
			return 1;
		else if ( pJamo->m == PHM_O && pJamo->f == PHF_R )
			return 1;
		else if ( pJamo->m == PHM_U && pJamo->f == PHF_N )
			return 1;
	}
	else if ( pJamo->i == PHI_N )
	{
		if ( pJamo->m == PHM_I && pJamo->f == PHF_M )
			return 1;
	}
	else if ( pJamo->i == PHI_D )
	{
		if ( pJamo->m == PHM_A &&
			(pJamo->f == PHF_B || pJamo->f == PHF_NG) )
			return 1;
		else if ( pJamo->m == PHM_O && pJamo->f == PHF_FIL )
			return 1;
		else if ( pJamo->m == PHM_OI && pJamo->f == PHF_FIL )
			return 1;
		else if ( pJamo->m == PHM_EU && (pJamo->f == PHF_NG || pJamo->f == PHF_R) )
			return 1;
	}
	else if ( pJamo->i == PHI_R )
	{
		if ( pJamo->m == PHM_YO && pJamo->f == PHF_FIL )
			return 1;
		else if ( pJamo->m == PHM_YU && pJamo->f == PHF_FIL )
			return 1;
	}
	else if ( pJamo->i == PHI_M )
	{
		if ( pJamo->m == PHM_A && pJamo->f == PHF_R )
			return 1;
		else if ( pJamo->m == PHM_YEO && pJamo->f == PHF_NG )
			return 1;
	}
	else if ( pJamo->i == PHI_B )
	{
		if ( pJamo->m == PHM_A && pJamo->f == PHF_D )
			return 1;
		else if ( pJamo->m == PHM_YEO && pJamo->f == PHF_R )
			return 1;
		else if ( pJamo->m == PHM_I && pJamo->f == PHF_FIL )
			return 1;
	}
	else if ( pJamo->i == PHI_BB )
	{
		if ( pJamo->m == PHM_EO && pJamo->f == PHF_R )
			return 1;
		else if ( pJamo->m == PHM_U && pJamo->f == PHF_N )
			return 1;
	}
	else if ( pJamo->i == PHI_S )
	{
		if ( pJamo->m == PHM_A && (pJamo->f == PHF_NG) )
			return 1;
		else if ( pJamo->m == PHM_AI && (pJamo->f == PHF_FIL || pJamo->f == PHF_NG) )
			return 1;
		else if ( pJamo->m == PHM_EO && (pJamo->f == PHF_R) )
			return 1;
	}
	else if ( pJamo->i == PHI_SS )
	{
		if ( pJamo->m == PHM_I && (pJamo->f == PHF_FIL || pJamo->f == PHF_G) )
			return 1;
	}
	else if ( pJamo->i == PHI_NG )
	{
		if ( pJamo->m == PHM_YA && pJamo->f == PHF_NG )
			return 1;
		else if ( pJamo->m == PHM_O && pJamo->f == PHF_NG )
			return 1;
		else if ( pJamo->m == PHM_YO && pJamo->f == PHF_NG )
			return 1;
	}
	else if ( pJamo->i == PHI_J )
	{
		if ( pJamo->m == PHM_A && ( pJamo->f == PHF_NG) )
			return 1;
		else if ( pJamo->m == PHM_EO && (pJamo->f == PHF_G) )
			return 1;
		else if ( pJamo->m == PHM_I && (pJamo->f == PHF_R || pJamo->f == PHF_S) )
			return 1;
	}
	else if ( pJamo->i == PHI_JJ )
	{
		if ( pJamo->m == PHM_AI && pJamo->f == PHF_FIL )
			return 1;
		else if ( pJamo->m == PHM_U && pJamo->f == PHF_NG )
			return 1;
		else if ( pJamo->m == PHM_EU && pJamo->f == PHF_M )
			return 1;
	}
	else if ( pJamo->i == PHI_CH )
	{
		if ( pJamo->m == PHM_EOI && pJamo->f == PHF_FIL )
			return 1;
		else if ( pJamo->m == PHM_I && pJamo->f == PHF_FIL )
			return 1;
	}
	else if ( pJamo->i == PHI_P )
	{
		if ( pJamo->m == PHM_O && pJamo->f == PHF_FIL )
			return 1;
	}

	return 0;
}

/*********************************************************/
TBOOL	MAKDisXI(CJamo *pJamo)
{
	if ( pJamo->i == PHI_D )
	{
		if ( pJamo->m == PHM_OI )
			return 1;
		else if ( pJamo->m == PHM_OAI )
			return 1;

	}
	else if ( pJamo->i == PHI_NG )
	{
		if ( pJamo->m == PHM_I ) {
			if ( pJamo->f == PHF_M ) return 1; }
	}
	else if ( pJamo->i == PHI_H )
	{
		if ( pJamo->m == PHM_A || pJamo->m == PHM_AI )
			return 1;
	}

	return 0;
}

/*********************************************************/
void	MAKDclrAnalBK()
{
	g_AnalBK.shHeadLen = 0;
	g_AnalBK.shFormalLen = 0;
	g_AnalBK.shRealLen = 0;
	g_AnalBK.shTmpLen = 0;

	g_AnalBK.isJosa = 0;
	g_AnalBK.isEomi = 0;
	g_AnalBK.isXS = 0;
	g_AnalBK.isXP = 0;
	g_AnalBK.isXI = 0;
}

/*
**  END MAK_DATA.C
*/
