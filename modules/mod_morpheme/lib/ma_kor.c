/* $Id$ */
/*
**  MA_KOR.C
**  2002. 01.  BY JaeBum, Kim.
*/
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	"old_softbot.h"
#include	"ma_code.h"
#include	"ma_data.h"
#include	"ma.h"
#include	"mak_data.h"
#include	"ma_kor.h"

/*  
**			MEMBER
*/
CJamo	m_JamoHF;	/*  Head First  */
CJamo	m_JamoHE;	/*  Head End    */
CJamo	m_Jamo;		/*  Tmp Jamo    */
CJamo	m_Jamo2;

/*********************************************************/
void	MAKsetPOS(CHeadSL *pHeadSL)
{
	if ( pHeadSL->wPred == S_DEF || pHeadSL->wPred == S_N )
	{
		if ( IS_USER(pHeadSL->adwPos[2]) )
			pHeadSL->wPred = MA_KOR_N;
		else if ( IS_NOISE(pHeadSL->adwPos[2]) )
			pHeadSL->wPred = MA_NOISE;
		else if ( IS_N(pHeadSL->adwPos[0], pHeadSL->adwPos[1]) )
			pHeadSL->wPred = MA_KOR_N;
		else if ( IS_V(pHeadSL->adwPos[0], pHeadSL->adwPos[1]) )
			pHeadSL->wPred = MA_KOR_V;
		else if ( IS_F(pHeadSL->adwPos[0], pHeadSL->adwPos[1]) )
			pHeadSL->wPred = MA_KOR_F;
		else
			pHeadSL->wPred = MA_NOISE;
	}
	else
	{
		if ( IS_USER(pHeadSL->adwPos[2]) )
			pHeadSL->wPred = MA_KOR_N;
		else if ( IS_N(pHeadSL->adwPos[0], pHeadSL->adwPos[1]) )
			pHeadSL->wPred = MA_KOR_N;
		else if ( IS_V(pHeadSL->adwPos[0], pHeadSL->adwPos[1]) )
			pHeadSL->wPred = MA_KOR_V;
		else if ( IS_F(pHeadSL->adwPos[0], pHeadSL->adwPos[1]) )
			pHeadSL->wPred = MA_KOR_F;
		else
			pHeadSL->wPred = MA_NOISE;
	}

	return;
}

/*********************************************************/
TBOOL	MAKchkUnit()
{
	if ( !MADgetPOS(g_AnalBK.pHeadSL->szWord, g_AnalBK.pHeadSL->adwPos) )
		return 0;

	MAKsetPOS(g_AnalBK.pHeadSL);
	MAKDpushMorph(g_AnalBK.pHeadSL);

#ifdef _NOT_USE_
	{
		if ( g_AnalBK.shHeadLen <= 2 )			
		{
			g_AnalBK.pHeadSL->wPred = MA_NOISE;
			MAKDpushMorph(g_AnalBK.pHeadSL);
			return 1;
		}
		else
			return 0;
	}

	MAKsetPOS(g_AnalBK.pHeadSL);

	if ( g_AnalBK.shHeadLen == 2 )
	{
		if ( g_AnalBK.pHeadSL->wPred == MA_KOR_N )
		{
			if ( IS_USER(g_AnalBK.pHeadSL->adwPos[2]) ||
				IS_N(g_AnalBK.pHeadSL->adwPos[0], g_AnalBK.pHeadSL->adwPos[1]) )
				;
			else
				g_AnalBK.pHeadSL->wPred = MA_NOISE;
		}
	}

	MAKDpushMorph(g_AnalBK.pHeadSL);
#endif

	return 1;
}

/*********************************************************/
void	MAKsetMA(CMA *pMA)
{
	TINT	i;

	pMA->shCntSL = 0;
	for ( i = g_MorBK.shCntSL - 1; i >= 0; i-- )
	{
		if ( !(g_MorBK.aMorSL[i].wPred & pMA->wMask) )
			continue;
		
		CDconvKSSM2KS(g_MorBK.aMorSL[i].szWord, pMA->aMASL[pMA->shCntSL].szWord );
		pMA->aMASL[pMA->shCntSL].wPosTag = g_MorBK.aMorSL[i].wPred;
		pMA->shCntSL += 1;
	}

	return;
}

/*********************************************************/
TBOOL	MAKchkHE_F()
{
	if ( !MADgetPOS(g_AnalBK.Real.szWord, g_AnalBK.Real.adwPos) )
		return 0;

	if ( g_AnalBK.isJosa || g_AnalBK.isXS )
	{
		if ( (IS_N(g_AnalBK.Real.adwPos[0], g_AnalBK.Real.adwPos[1]) ||
			IS_USER(g_AnalBK.Real.adwPos[2])) )
			return 1;
	}
	if ( g_AnalBK.isEomi )
	{
		if ( IS_V(g_AnalBK.Real.adwPos[0], g_AnalBK.Real.adwPos[1]) )
			return 1;
	}
	if ( g_AnalBK.isXI )
	{
		if ( IS_NVJ(g_AnalBK.Real.adwPos[0], g_AnalBK.Real.adwPos[1]) )
			return 1;
	}

	return 0;
}

/*********************************************************/
TBOOL	MAKchkHE_NRMB()
{
	if ( g_AnalBK.shHeadLen < 8 )
		return 0;
	if ( g_AnalBK.isJosa || g_AnalBK.isEomi )
		return 0;

	if ( !MAKDisNRMB(&m_JamoHE) )
		return 0;

	g_AnalBK.shFormalLen = 4;
	g_AnalBK.shRealLen = g_AnalBK.shHeadLen - g_AnalBK.shFormalLen;
	strncpy(g_AnalBK.Formal.szWord, g_AnalBK.pHeadSL->szWord + g_AnalBK.shRealLen, 2);
	g_AnalBK.Formal.szWord[2] = 0x00;
	CDcopySyl(&(g_AnalBK.Formal.szWord[2]), m_JamoHE.i, m_JamoHE.m, PHF_FIL);
	g_AnalBK.Formal.szWord[g_AnalBK.shFormalLen] = 0x00;

	if ( !MADgetPOS(g_AnalBK.Formal.szWord, g_AnalBK.Formal.adwPos) )
		return 0;

	if ( !IS_V(g_AnalBK.Formal.adwPos[0], g_AnalBK.Formal.adwPos[1]) )
		return 0;

	CDcopySyl(g_AnalBK.TmpSL.szWord, PHI_FIL, PHM_FIL, m_JamoHE.f);
	g_AnalBK.TmpSL.szWord[2] = 0x00;
	g_AnalBK.TmpSL.wPred = MA_KOR_F;
	MAKDpushMorph(&(g_AnalBK.TmpSL));

	g_AnalBK.Formal.wPred = MA_KOR_V;
	MAKDpushMorph(&(g_AnalBK.Formal));

	strncpy(g_AnalBK.Real.szWord, g_AnalBK.pHeadSL->szWord, g_AnalBK.shRealLen);
	g_AnalBK.Real.szWord[g_AnalBK.shRealLen] = 0x00;
	MAKDpushHead(g_AnalBK.Real.szWord, S_V);
	
	return 1;
}


/*********************************************************/
TBOOL	MAKchkHE()
{
	/*  Head맨끝자를 Jamo Type으로 변화  */
	CDconvKSSM2Jamo((char*)(g_AnalBK.pHeadSL->szWord + g_AnalBK.shHeadLen - 2), &m_JamoHE );

	if ( MAKDisJosa(&m_JamoHE) )
		g_AnalBK.isJosa = 1;
	else
		g_AnalBK.isJosa = 0;

	if ( MAKDisEomi(&m_JamoHE) )
		g_AnalBK.isEomi = 1;
	else 
		g_AnalBK.isEomi = 0;

	if ( MAKDisXS(&m_JamoHE) )
		g_AnalBK.isXS = 1;
	else
		g_AnalBK.isXS = 0;

	if ( MAKDisXI(&m_JamoHE) )
		g_AnalBK.isXI = 1;
	else
		g_AnalBK.isXI = 0;

	if ( g_AnalBK.isJosa || g_AnalBK.isEomi || g_AnalBK.isXS || g_AnalBK.isXI )
	{
		g_AnalBK.shFormalLen = 2;
		g_AnalBK.shRealLen = g_AnalBK.shHeadLen - g_AnalBK.shFormalLen;
		strncpy(g_AnalBK.Real.szWord, g_AnalBK.pHeadSL->szWord, g_AnalBK.shRealLen);
		g_AnalBK.Real.szWord[g_AnalBK.shRealLen] = 0x00;
		strcpy(g_AnalBK.Formal.szWord, g_AnalBK.pHeadSL->szWord + g_AnalBK.shHeadLen - 2);

		if ( MAKchkHE_F() )
		{
			g_AnalBK.Formal.wPred = MA_KOR_F;
			MAKDpushMorph(&g_AnalBK.Formal);

			g_AnalBK.Real.wPred = S_F;
			MAKsetPOS(&(g_AnalBK.Real));
			MAKDpushMorph(&(g_AnalBK.Real));
		
			return 1;
		}
	}

	if ( MAKchkHE_NRMB() )
		return 1;

	return 0;
}

/*********************************************************/
TBOOL	MAKchkH_JAMO()
{
	if ( g_AnalBK.shRealLen == 0 )
		return 0;
	else if ( g_AnalBK.shRealLen == 2 )
	{
		g_AnalBK.shFormalLen = g_AnalBK.shHeadLen - 2;
		if ( m_Jamo.i == PHI_H && m_Jamo.m == PHM_AI )
		{
			
			CDcopySyl(g_AnalBK.Formal.szWord, PHI_NG, PHM_YEO, m_Jamo.f);
			g_AnalBK.Formal.szWord[2] = 0x00;
			if ( g_AnalBK.shFormalLen > 2 )
				strcpy(g_AnalBK.Formal.szWord + 2, g_AnalBK.pHeadSL->szWord + 4);
			strncpy(g_AnalBK.Real.szWord, g_AnalBK.pHeadSL->szWord, 2);
			CDcopySyl(g_AnalBK.Real.szWord + 2, PHI_H, PHM_A, PHF_FIL);
			g_AnalBK.Real.szWord[4] = 0x00;
			g_AnalBK.Formal.wPred = MA_KOR_F;
			MAKDpushMorph(&(g_AnalBK.Formal));
			g_AnalBK.Real.wPred = MA_KOR_V;
			MAKDpushMorph(&(g_AnalBK.Real));
			return 1;
		}
		if (m_Jamo.i == PHI_NG && m_Jamo.m == PHM_YEO )
		{
			CDcopySyl(g_AnalBK.Formal.szWord, PHI_NG, PHM_EO, m_Jamo.f);
			g_AnalBK.Formal.szWord[2] = 0x00;
			if ( g_AnalBK.shFormalLen > 2 )
				strcpy(g_AnalBK.Formal.szWord + 2, g_AnalBK.pHeadSL->szWord + 4);
			strncpy(g_AnalBK.Real.szWord, g_AnalBK.pHeadSL->szWord, 2);
			CDcopySyl(g_AnalBK.Real.szWord + 2, PHI_NG, PHM_I, PHF_FIL);
			g_AnalBK.Real.szWord[4] = 0x00;
			g_AnalBK.Formal.wPred = MA_KOR_F;
			MAKDpushMorph(&(g_AnalBK.Formal));
			g_AnalBK.Real.wPred = MA_KOR_V;
			MAKDpushMorph(&(g_AnalBK.Real));
			return 1;
		}
		if ( m_Jamo.i == PHI_CH && m_Jamo.m == PHM_YEO )
		{
			CDcopySyl(g_AnalBK.Formal.szWord, PHI_NG, PHM_EO, m_Jamo.f);
			g_AnalBK.Formal.szWord[2] = 0x00;
			if ( g_AnalBK.shFormalLen > 2 )
				strcpy(g_AnalBK.Formal.szWord + 2, g_AnalBK.pHeadSL->szWord + 4);
			strncpy(g_AnalBK.Real.szWord, g_AnalBK.pHeadSL->szWord, 2);
			CDcopySyl(g_AnalBK.Real.szWord + 2, PHI_CH, PHM_I, PHF_FIL);
			g_AnalBK.Real.szWord[4] = 0x00;
			g_AnalBK.Formal.wPred = MA_KOR_F;
			MAKDpushMorph(&(g_AnalBK.Formal));
			g_AnalBK.Real.wPred = MA_KOR_V;
			MAKDpushMorph(&(g_AnalBK.Real));
			return 1;
		}
	}

	strncpy(g_AnalBK.Real.szWord, g_AnalBK.pHeadSL->szWord, g_AnalBK.shRealLen);
	g_AnalBK.Real.szWord[g_AnalBK.shRealLen] = 0x00;
	strcpy(g_AnalBK.Formal.szWord, g_AnalBK.pHeadSL->szWord + g_AnalBK.shRealLen);
	g_AnalBK.Formal.wPred = MA_KOR_F;
	MAKDpushMorph(&(g_AnalBK.Formal));
	MAKDpushHead(g_AnalBK.Real.szWord, MA_KOR_F);

	return 1;
}

/*********************************************************/
TBOOL	MAKchkH_C()
{
	if ( m_Jamo.i == PHI_CH && m_Jamo.m == PHM_YEO )
	{
		strncpy(g_AnalBK.TmpSL.szWord, g_AnalBK.pHeadSL->szWord, g_AnalBK.shRealLen);
		g_AnalBK.TmpSL.szWord[g_AnalBK.shRealLen] = 0x00;
		CDcopySyl(&(g_AnalBK.TmpSL.szWord[g_AnalBK.shRealLen]), PHI_CH, PHM_I, PHF_FIL);
		CDcopySyl(&(g_AnalBK.TmpSL.szWord[g_AnalBK.shRealLen + 2]), PHI_NG, PHM_EO, m_Jamo.f);
	}
	else if ( m_Jamo.i == PHI_K && m_Jamo.m == PHM_YEO )
	{
		strncpy(g_AnalBK.TmpSL.szWord, g_AnalBK.pHeadSL->szWord, g_AnalBK.shRealLen);
		g_AnalBK.TmpSL.szWord[g_AnalBK.shRealLen] = 0x00;
		CDcopySyl(&(g_AnalBK.TmpSL.szWord[g_AnalBK.shRealLen]), PHI_K, PHM_I, PHF_FIL);
		CDcopySyl(&(g_AnalBK.TmpSL.szWord[g_AnalBK.shRealLen + 2]), PHI_NG, PHM_EO, m_Jamo.f);
	}
#ifdef _NOT_
	else if ( m_Jamo.i == PHI_H && m_Jamo.m == PHM_YEO )
	{
		strncpy(g_AnalBK.TmpSL.szWord, g_AnalBK.pHeadSL->szWord, g_AnalBK.shRealLen);
		g_AnalBK.TmpSL.szWord[g_AnalBK.shRealLen] = 0x00;
		CDcopySyl(&(g_AnalBK.TmpSL.szWord[g_AnalBK.shRealLen]), PHI_H, PHM_I, PHF_FIL);
		CDcopySyl(&(g_AnalBK.TmpSL.szWord[g_AnalBK.shRealLen + 2]), PHI_NG, PHM_EO, m_Jamo.f);
	}
#endif
	else
		return 0;

	g_AnalBK.shFormalLen = g_AnalBK.shHeadLen - g_AnalBK.shRealLen - 2;

	g_AnalBK.TmpSL.szWord[g_AnalBK.shRealLen + 4] = 0x00;
	if ( g_AnalBK.shFormalLen > 0 )
		strcpy(&(g_AnalBK.TmpSL.szWord[g_AnalBK.shRealLen + 4]), g_AnalBK.pHeadSL->szWord + g_AnalBK.shRealLen + 2);

	strcpy(g_AnalBK.pHeadSL->szWord, g_AnalBK.TmpSL.szWord);
	g_AnalBK.shHeadLen = strlen(g_AnalBK.pHeadSL->szWord);
	return 1;
}
/*********************************************************/
TBOOL	MAKchkH()
{
	for ( g_AnalBK.shRealLen = 0; g_AnalBK.shRealLen < g_AnalBK.shHeadLen; g_AnalBK.shRealLen += 2)
	{
		CDconvKSSM2Jamo((char*)(g_AnalBK.pHeadSL->szWord + g_AnalBK.shRealLen), &m_Jamo );

		if ( g_AnalBK.shRealLen == 0 )
		{
			if ( MAKDisXP(&m_Jamo) )
				g_AnalBK.isXP = 1;
			else
				g_AnalBK.isXP = 0;
		}

		if ( MAKchkH_C() )
			;
		else if ( m_Jamo.f == PHF_SS || m_Jamo.f == PHF_BS )
			break;
	}

	if ( g_AnalBK.shRealLen == g_AnalBK.shHeadLen )
		return 0;

	if ( MAKchkH_JAMO() )
		return 1;

	return 0;
}

/*********************************************************/
TBOOL	MAKchkRF_PE()
{
	if ( !IS_PE(g_AnalBK.Formal.adwPos[0], g_AnalBK.Formal.adwPos[1]) )
		return 0;

	if ( IS_NCP(g_AnalBK.Formal.adwPos[0], g_AnalBK.Formal.adwPos[1]) )
		return 0;

	g_AnalBK.Formal.wPred = MA_KOR_F;
	MAKDpushMorph(&(g_AnalBK.Formal));

	MAKDpushHead(g_AnalBK.Real.szWord, S_F);

	return 1;
}

/*********************************************************/
TBOOL	MAKchkRF_N()
{

	if ( !(IS_N(g_AnalBK.Formal.adwPos[0], g_AnalBK.Formal.adwPos[1]) ||
		IS_USER(g_AnalBK.Formal.adwPos[2])) )
		return 0;

	if ( g_AnalBK.shFormalLen == 2 )
	{
		if ( g_AnalBK.isJosa || g_AnalBK.isEomi || g_AnalBK.isXS )
			return 0;
		else if ( g_AnalBK.shRealLen <= 4 )
			return 0;
	}
	else if ( g_AnalBK.shFormalLen == 4 )
	{
		switch ( g_AnalBK.shRealLen )
		{
		case 2:
			if ( g_AnalBK.isXP )
			{
				g_AnalBK.pHeadSL->wPred = MA_KOR_N;
				MAKDpushMorph(g_AnalBK.pHeadSL);
				return 1;
			}
			else if ( IS_NNP(g_AnalBK.Formal.adwPos[0], g_AnalBK.Formal.adwPos[1]) )
			{
				g_AnalBK.pHeadSL->wPred = MA_KOR_N;
				MAKDpushMorph(g_AnalBK.pHeadSL);
				return 1;
			}
			break;
		case 4:
			if ( g_AnalBK.isJosa )
			{
				if ( IS_NNCM(g_AnalBK.Formal.adwPos[2]) )
					return 0;
				if ( IS_NVJ(g_AnalBK.Formal.adwPos[0], g_AnalBK.Formal.adwPos[1]) )
					return 0;
			}
			else if ( IS_NVJ(g_AnalBK.Formal.adwPos[0], g_AnalBK.Formal.adwPos[1]) )
				;
			else if ( g_AnalBK.isXS )
				return 0;
			break;
		case 6:
			if ( g_AnalBK.isJosa )
			{
				if ( IS_NNCM(g_AnalBK.Formal.adwPos[2]) )
					return 0;
				if ( IS_NVJ(g_AnalBK.Formal.adwPos[0], g_AnalBK.Formal.adwPos[1]) )
					return 0;
			}
			CDconvKSSM2Jamo(g_AnalBK.Real.szWord + g_AnalBK.shRealLen - 2, &m_Jamo);
			if ( MAKDisJosa(&m_Jamo) )
				;
			else if ( MADgetPOS(g_AnalBK.Real.szWord, g_AnalBK.Real.adwPos) )
			{
				g_AnalBK.Formal.wPred = MA_KOR_N;
				g_AnalBK.Real.wPred = MA_KOR_N;
				MAKDpushMorph(&(g_AnalBK.Formal));
				MAKDpushMorph(&(g_AnalBK.Real));
				return 1;
			}
			else if ( !(g_AnalBK.isJosa || g_AnalBK.isEomi || g_AnalBK.isXS) )
				return 0;

			break;
		default:
			if ( ((g_AnalBK.shRealLen / 2) % 2) == 1 )
			{
				if ( g_AnalBK.isJosa )
					return 0;
			}
			else
			{
				if ( g_AnalBK.isJosa )
				{
					if ( IS_NNCM(g_AnalBK.Formal.adwPos[2]) )
						return 0;
				}
			}

			break;
		}
	}

	g_AnalBK.Formal.wPred = MA_KOR_N;
	MAKDpushMorph(&(g_AnalBK.Formal));

	MAKDpushHead(g_AnalBK.Real.szWord, S_N);

	return 1;
}

/*********************************************************/
TBOOL	MAKchkRF_V()
{
	if ( !IS_V(g_AnalBK.Formal.adwPos[0], g_AnalBK.Formal.adwPos[1]) )
		return 0;

	if ( g_AnalBK.shRealLen == 2 )
		return 0;

	if ( g_AnalBK.shFormalLen == 2 )
	{
		if ( g_AnalBK.isJosa || g_AnalBK.isEomi )
			g_AnalBK.Formal.wPred = MA_KOR_F;
		else
			g_AnalBK.Formal.wPred = MA_KOR_V;
	}
	else
		g_AnalBK.Formal.wPred = MA_KOR_V;
	MAKDpushMorph(&(g_AnalBK.Formal));

	MAKDpushHead(g_AnalBK.Real.szWord, S_V);

	return 1;
}

/*
**  형식, 실질 형태소 분리  */
/*********************************************************/
TBOOL	MAKchkRF()
{
	if ( g_AnalBK.shHeadLen < 6 )
		return 0;

	/*  실질, 형식(접미, 조사, 어미, 명사) 형태소로 나누어서 검사  */
	g_AnalBK.shRealLen = g_AnalBK.shHeadLen - 12;
	if ( g_AnalBK.shRealLen < 0 )
		g_AnalBK.shRealLen = 2;

	for ( ; ; )
	{
		g_AnalBK.shFormalLen = g_AnalBK.shHeadLen - g_AnalBK.shRealLen;
		if ( g_AnalBK.shFormalLen == 0 )
			break;

		/*  split  real, formal  */
		strncpy((char*)g_AnalBK.Real.szWord, (char*)g_AnalBK.pHeadSL->szWord, g_AnalBK.shRealLen);
		g_AnalBK.Real.szWord[g_AnalBK.shRealLen] = 0x00;
		strcpy((char*)g_AnalBK.Formal.szWord, (char*)(g_AnalBK.pHeadSL->szWord + g_AnalBK.shRealLen));

		if ( !MADgetPOS(g_AnalBK.Formal.szWord, g_AnalBK.Formal.adwPos) )
		{
			g_AnalBK.shRealLen += 2;
			continue;
		}
		
		if ( MAKchkRF_PE() )
			return 1;

		if ( MAKchkRF_N() )
			return 1;

		if ( MAKchkRF_V() )
			return 1;

		g_AnalBK.shRealLen += 2;
	}	/*  for (;;)  */

	return 0;
}


/*********************************************************/
TBOOL	MAKest4()
{
	if ( g_AnalBK.pHeadSL->wPred == S_DEF || g_AnalBK.pHeadSL->wPred == S_N )
	{
		g_AnalBK.pHeadSL->wPred = MA_KOR_UN;
		MAKDpushMorph(g_AnalBK.pHeadSL);
	}
	else
	{
		if ( g_AnalBK.shHeadLen == 2 )
		{
			g_AnalBK.pHeadSL->wPred = MA_NOISE;
			MAKDpushMorph(g_AnalBK.pHeadSL);
		}
		else
		{
			g_AnalBK.pHeadSL->wPred = MA_KOR_UN;
			MAKDpushMorph(g_AnalBK.pHeadSL);
		}
	}
	
	return 1;		
}

/*********************************************************/
TBOOL	MAKest6()
{
	if ( g_AnalBK.pHeadSL->wPred == S_DEF )
	{
		if ( g_AnalBK.isJosa || g_AnalBK.isXS )
		{
			g_AnalBK.shFormalLen = 2;
			g_AnalBK.shRealLen = g_AnalBK.shHeadLen - g_AnalBK.shFormalLen;
			strcpy(g_AnalBK.Formal.szWord, g_AnalBK.pHeadSL->szWord + g_AnalBK.shRealLen);
			strncpy(g_AnalBK.Real.szWord, g_AnalBK.pHeadSL->szWord, g_AnalBK.shRealLen);
			g_AnalBK.Real.szWord[g_AnalBK.shRealLen] = 0x00;
			g_AnalBK.Formal.wPred = MA_KOR_F;
			g_AnalBK.Real.wPred = MA_KOR_UN;
			MAKDpushMorph(&(g_AnalBK.Formal));
			MAKDpushMorph(&(g_AnalBK.Real));
			return 1;
		}
	}

	g_AnalBK.pHeadSL->wPred = MA_KOR_UN;
	MAKDpushMorph(g_AnalBK.pHeadSL);
	
	return 1;		
}

/*********************************************************/
TBOOL	MAKest8()
{
	if ( g_AnalBK.pHeadSL->wPred == S_DEF )
	{
		if ( g_AnalBK.isJosa || g_AnalBK.isXS )
		{
			g_AnalBK.shFormalLen = 2;
			g_AnalBK.shRealLen = g_AnalBK.shHeadLen - g_AnalBK.shFormalLen;
			strcpy(g_AnalBK.Formal.szWord, g_AnalBK.pHeadSL->szWord + g_AnalBK.shRealLen);
			strncpy(g_AnalBK.Real.szWord, g_AnalBK.pHeadSL->szWord, g_AnalBK.shRealLen);
			g_AnalBK.Real.szWord[g_AnalBK.shRealLen] = 0x00;
			g_AnalBK.Formal.wPred = MA_KOR_F;
			g_AnalBK.Real.wPred = MA_KOR_UN;
			MAKDpushMorph(&(g_AnalBK.Formal));
			MAKDpushHead(g_AnalBK.Real.szWord, S_F);
			return 1;
		}
	}

	g_AnalBK.pHeadSL->wPred = MA_KOR_UN;
	MAKDpushMorph(g_AnalBK.pHeadSL);
	
	return 1;		
}

/*********************************************************/
TBOOL	MAKestMore()
{
	if ( g_AnalBK.isJosa || g_AnalBK.isXS )
	{
		g_AnalBK.shRealLen = g_AnalBK.shHeadLen - 2;
		g_AnalBK.shFormalLen = 2;
		strcpy(g_AnalBK.Formal.szWord, g_AnalBK.pHeadSL->szWord + g_AnalBK.shRealLen);
		strncpy(g_AnalBK.Real.szWord, g_AnalBK.pHeadSL->szWord, g_AnalBK.shRealLen);
		g_AnalBK.Real.szWord[g_AnalBK.shRealLen] = 0x00;
		
		g_AnalBK.Formal.wPred = MA_KOR_F;
		g_AnalBK.Real.wPred = MA_KOR_UN;
		MAKDpushMorph(&(g_AnalBK.Formal));
		MAKDpushHead(g_AnalBK.Real.szWord, S_F);
		return 1;
	}

	if ( g_AnalBK.shHeadLen > 12 )
		g_AnalBK.pHeadSL->wPred = MA_NOISE;
	else
		g_AnalBK.pHeadSL->wPred = MA_KOR_UN;
	MAKDpushMorph(g_AnalBK.pHeadSL);

	return 1;
}


/*********************************************************/
void	MAKest()
{
	switch ( g_AnalBK.shHeadLen )
	{
	case 2:
	case 4:
		if ( MAKest4() )
			return;
		break;
	case 6:
		if ( MAKest6() )
			return;
		break;
	case 8:
		if ( MAKest8() )
			return;
		break;
	default:
		if ( MAKestMore() )
			return;
		break;
	}


	return;
}

/*********************************************************/
void	MAKanal(CMA *pMA, TBYTE *pszTok)
{
	MAKDreset();

	if ( strlen(pszTok) >= MA_WORD_LEN )
		return;

	if ( !CDconvKS2KSSM(pszTok, g_AnalBK.TmpSL.szWord) )
		return;

	MAKDpushHead(g_AnalBK.TmpSL.szWord, S_DEF);

	for ( ; ; )
	{
		if ( !MAKDpopHead() )
			break;

		/*  단일 형태소 분석  */
		if ( MAKchkUnit() )
			continue;

		if ( MAKchkH() )
			continue;

		/*  체언, 용언 + 기능어 분석  */
		if ( MAKchkHE() )
			continue;

		/*  다중 형태소 분석  */
		if ( MAKchkRF() )
			continue;

		/*  형태소 추정  */
		MAKest();
	}

	MAKsetMA(pMA);

	return;
}

/*
**  END MA_KOR.C
*/
