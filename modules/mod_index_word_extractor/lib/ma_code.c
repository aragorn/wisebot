/* $Id$ */
/*
**  MA_CODE.C
**  2001. 04.  BY JaeBum, Kim.
*/
#include	"old_softbot.h"
#include	"ma_code.h"

/* *************************************************************** */
TBOOL  CDconvChn2Kor(TBYTE *pszChn, TBYTE *pszKor)
{
	TINT	i, nLen;
	TINT    iPos;
	TBYTE   byCh1, byCh2;

	nLen = strlen(pszChn);
	for ( i = 0; i < nLen; i += 2 )
	{
		byCh1 = pszChn[i];
		byCh2 = pszChn[i+1];

		iPos = (TINT)((byCh1 - 202) * 94);
		iPos += (TINT)(byCh2 - 161);

		if ( iPos < 0 || iPos >= MAX_CHN_TB_NUM )
			return 0;

		pszKor[i] = CDM_GET_HIGH(m_ChnTB[iPos]);
		pszKor[i+1] = CDM_GET_LOW(m_ChnTB[iPos]);
	}

	pszKor[i] = 0x00;

	return 1;
}

/*********************************************************/
TBOOL	CDconvKS2KSSM(TBYTE *ks , TBYTE *tg)
{
	TINT	i, nLen, iPos;
    TBYTE	byCh1, byCh2;

    *tg = 0x00;
	nLen = strlen(ks);

	i = 0;
	while ( i < nLen )
	{
		if ( !(ks[i] & 0x80) )
		{
			if ( (ks[i] >= 'a') && (ks[i] <= 'z') )
				tg[i] = (TBYTE)toupper(ks[i]);
			else
				tg[i] = ks[i];
			i++;
			continue;
		}

		byCh1 = ks[i]; byCh2 = ks[i+1];

		iPos = (byCh1 - 0xb0) * 94;
		iPos += (byCh2 - 0xa1);

		if ( iPos < 0 || iPos > MAX_KSSM_NUM )
			return 0;

		tg[i] = CDM_GET_HIGH(m_KssmTB[iPos]);
		tg[i+1] = CDM_GET_LOW(m_KssmTB[iPos]);
		i += 2;
	}

	tg[i] = 0x00;

	return 1;
}

/*********************************************************/
TINT	CDsearchKssmTB(TWORD wTg, TINT nSrt, TINT nEnd)
{
	TINT	k;

	if ( nSrt > nEnd )
		return -1;

	k = (nSrt + nEnd) / 2;

	if ( m_KssmTB[k] == wTg )
		return k;
	else if ( m_KssmTB[k] < wTg )
		k = CDsearchKssmTB(wTg, (k + 1), nEnd);
	else
		k = CDsearchKssmTB(wTg, nSrt, (k - 1));

	return k;
}

/*********************************************************/
TINT	CDsearchKssm2TB(TWORD wTg)
{
	TINT	i;

	for ( i = 0; i < MAX_KSSM2_NUM; i++ )
	{
		if ( m_Kssm2TB[i] == wTg )
			break;
	}

	if ( i < MAX_KSSM2_NUM )
		return i;

	for ( i = 0; i < CON_NUM; i++ )
	{
		if ( m_ConTB[i][2] == wTg )
			break;
	}

	if ( i >= CON_NUM )
		return -1;

	wTg = m_ConTB[i][1];

	for ( i = 0; i < MAX_KSSM2_NUM; i++ )
	{
		if ( m_Kssm2TB[i] == wTg )
			break;
	}

	if ( i < MAX_KSSM2_NUM )
		return i;

	return -1;

}

/*********************************************************/
TBOOL	CDconvKSSM2KS(TBYTE *tg, TBYTE *ks)
{
	TINT	i, nLen, iPos;
	TWORD	wTg;
	TWORD   wCh1, wCh2;

    *ks = 0x00;
	nLen = strlen(tg);

	i = 0;
	while ( i < nLen )
	{
		if ( !(tg[i] & 0x80) )
		{
			if ( (tg[i] >= 'a') && (tg[i] <= 'z') )
				ks[i] = (TBYTE)toupper(tg[i]);
			else
				ks[i] = tg[i];
			i++;
			continue;
		}

		wCh1 = (TWORD)tg[i]; wCh2 = (TWORD)tg[i+1];
		wTg = CDM_MK_SYL(wCh1, wCh2);

		iPos = CDsearchKssmTB(wTg, 0, (MAX_KSSM_NUM - 1) );
		if ( iPos < 0 )
		{
			iPos = CDsearchKssm2TB(wTg);
			if ( iPos < 0 )
				return 0;

			ks[i] = 0xa4 & 0xff;
			ks[i+1] = 0xa1 + (iPos & 0xff);
		}
		else
		{
			ks[i] = (iPos / 94) + 0xb0;
			ks[i+1] = (iPos % 94) + 0xa1;
		}

		i += 2;
	}

	ks[i] = 0x00;

	return 1;
}

/*********************************************************/
void	CDconvKSSM2Jamo(TBYTE *pKssm, CJamo *pJamo)
{
	pJamo->i = ( (*pKssm) >> 2) & 0x1f;
	pJamo->m = ( ((*pKssm) << 3) & 0x18) | ((( (*(pKssm + 1)) >> 5) & 0x07));
	pJamo->f = (*(pKssm + 1)) & 0x1f;

	return;
}

/*********************************************************/
void	CDconvJamo2KSSM(CJamo *pJamo, TBYTE *pKssm)
{
	pKssm[0] = ( (0x80 | (pJamo->i << 2)) | ((pJamo->m & 0x18) >> 3) );
	pKssm[1] = ( ((pJamo->m & 0x07) << 5) | pJamo->f );

	return;
}

/*********************************************************/
void	CDcopySyl(TBYTE *pKssm, TBYTE phi, TBYTE phm, TBYTE phf) 
{
    pKssm[0] = (0x80 | ((phi & 0x1f) << 2) | ((phm & 0x18) >> 3)); 
    pKssm[1] = (((phm & 0x7) << 5) | (phf & 0x1f));
	pKssm[2] = 0x00;
}



/*
**  END MA_CODE.C
*/

