/* $Id$ */
/*
**  MA_ENG.C
**  2001.05.  BY JaeBum, Kim.
*/
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	"old_softbot.h"
#include	"ma_data.h"
#include	"ma.h"
#include	"ma_eng.h"

/*  Internal Function Prototype  */
TLONG	MAEreplaceEnd(char *pszTok, CEngRL *pEngRL);
TLONG	MAEchkWordSZ(char *pszTok);
TBOOL	MAEchkVowel(char *pszTok);
TBOOL	MAEaddE(char *pszTok);
TBOOL	MAErmvE(char *pszTok);
TBOOL	MAEchkCVC(char *pszTok);

static char LAMB[1] = "";
static char *m_pEnd;
TLONG	m_ID;
TLONG	m_nTokLen;

static CEngRL	m_Step1a[] =
{
	{ 101,	"SSES",	"SS",	3,	1,	-1,	0x00 },
	{ 102,	"IES",	"I",	2,	0,	-1,	0x00 },
	{ 103,	"SS",	"SS",	1,	1,	-1,	0x00 },
	{ 104,	"S",	LAMB,	0,	-1,	-1,	0x00 },
	{ 000,	0x00,	0x00,	0,	0,	0,	0x00 }
};
static CEngRL	m_Step1b[] =
{
	{ 105,	"EED",	"EE",	2,	1,	0,	0x00 },
	{ 106,	"ED",	LAMB,	1,	-1,	-1,	MAEchkVowel },
	{ 107,	"ING",	LAMB,	2,	-1,	-1,	MAEchkVowel },
	{ 000,	0x00,	0x00,	0,	0,	0,	0x00 },
};
static CEngRL	m_Step1b1[] = 
{
	{ 108,	"AT",	"ATE",	1,	2,	-1,	0x00 },
	{ 109,	"BL",	"BLE",	1,	2,	-1,	0x00 },
	{ 110,	"IZ",	"IZE",	1,	2,	-1,	0x00 },
	{ 111,	"BB",	"B",	1,	0,	-1,	0x00 },
	{ 112,	"DD",	"D",	1,	0,	-1,	0x00 },
	{ 113,	"FF",	"F",	1,	0,	-1,	0x00 },
	{ 114,	"GG",	"G",	1,	0,	-1,	0x00 },
	{ 115,	"MM",	"M",	1,	0,	-1,	0x00 },
	{ 116,	"NN",	"N",	1,	0,	-1,	0x00 },
	{ 117,	"PP",	"P",	1,	0,	-1,	0x00 },
	{ 118,	"RR",	"R",	1,	0,	-1,	0x00 },
	{ 119,	"TT",	"T",	1,	0,	-1,	0x00 },
	{ 120,	"WW",	"W",	1,	0,	-1,	0x00 },
	{ 121,	"XX",	"X",	1,	0,	-1,	0x00 },
	{ 122,	LAMB,	"E",	-1,	0,	-1,	MAEaddE },
	{ 000,	0x00,	0x00,	0,	0,	0,	0x00 },
};
static CEngRL	m_Step1c[] = 
{
	{ 123,	"Y",	"I",	0,	0,	-1,	MAEchkVowel },
	{ 000,	0x00,	0x00,	0,	0,	0,	0x00 },
};
static CEngRL	m_Step2[] =
{
	{ 203,	"ATIONAL",	"ATE",	6,	2,	0,	0x00 },
	{ 204,	"TIONAL",	"TION",	5,	3,	0,	0x00 },
	{ 205,	"ENCI",		"ENCE",	3,	3,	0,	0x00 },
	{ 206,	"ANCI",		"ANCE",	3,	3,	0,	0x00 },
	{ 207,	"IZER",		"IZE",	3,	2,	0,	0x00 },
	{ 208,	"ABLI",		"ABLE",	3,	3,	0,	0x00 },
	{ 209,	"ALLI",		"AL",	3,	1,	0,	0x00 },
	{ 210,	"ENTLI",	"ENT",	4,	2,	0,	0x00 },
	{ 211,	"ELI",		"E",	2,	0,	0,	0x00 },
	{ 213,	"OUSLI",	"OUS",	4,	2,	0,	0x00 },
	{ 214,	"IZATION",	"IZE",	6,	2,	0,	0x00 },
	{ 215,	"ATION",	"ATE",	4,	2,	0,	0x00 },
	{ 216,	"ATOR",		"ATE",	3,	2,	0,	0x00 },
	{ 217,	"ALISM",	"AL",	4,	1,	0,	0x00 },
	{ 218,	"IVENESS",	"IVE",	6,	2,	0,	0x00 },
	{ 219,	"FULNES",	"FUL",	5,	2,	0,	0x00 },
	{ 220,	"OUSNESS",	"OUS",	6,	2,	0,	0x00 },
	{ 221,	"ALITI",	"AL",	4,	1,	0,	0x00 },
	{ 222,	"IVITI",	"IVE",	4,	2,	0,	0x00 },
	{ 223,	"BILITI",	"BLE",	5,	2,	0,	0x00 },
	{ 000,	0x00,		0x00,	0,	0,	0,	0x00 },
};
static CEngRL	m_Step3[] =
{
	{ 301,	"ICATE",	"IC",	4,	1,	0,	0x00 },
	{ 302,	"ATIVE",	LAMB,	4,	-1,	0,	0x00 },
	{ 303,	"ALIZE",	"AL",	4,	1,	0,	0x00 },
	{ 304,	"ICITI",	"IC",	4,	1,	0,	0x00 },
	{ 305,	"ICAL",		"IC",	3,	1,	0,	0X00 },
	{ 308,	"FUL",		LAMB,	2,	-1,	0,	0x00 },
	{ 309,	"NESS",		LAMB,	3,	-1,	0,	0x00 },
	{ 000,	0x00,		0x00,	0,	0,	0,	0x00 },
};
static CEngRL	m_Step4[] =
{
	{ 401,	"AL",		LAMB,	1,	-1,	1,	0x00 },
	{ 402,	"ANCE",		LAMB,	3,	-1,	1,	0x00 },
	{ 403,	"ENCE",		LAMB,	3,	-1,	1,	0x00 },
	{ 405,	"ER",		LAMB,	1,	-1,	1,	0x00 },
	{ 406,	"IC",		LAMB,	1,	-1,	1,	0x00 },
	{ 407,	"ABLE",		LAMB,	3,	-1,	1,	0x00 },
	{ 408,	"IBLE",		LAMB,	3,	-1,	1,	0x00 },
	{ 409,	"ANT",		LAMB,	2,	-1,	1,	0x00 },
	{ 410,	"EMENT",	LAMB,	4,	-1,	1,	0x00 },
	{ 411,	"MENT",		LAMB,	3,	-1,	1,	0x00 },
	{ 412,	"ENT",		LAMB,	2,	-1,	1,	0x00 },
	{ 423,	"SION",		"S",	3,	0,	1,	0x00 },
	{ 424,	"TION",		"T",	3,	0,	1,	0x00 },
	{ 415,	"OU",		LAMB,	1,	-1,	1,	0x00 },
	{ 416,	"ISM",		LAMB,	2,	-1,	1,	0x00 },
	{ 417,	"ATE",		LAMB,	2,	-1,	1,	0x00 },
	{ 418,	"ITI",		LAMB,	2,	-1,	1,	0x00 },
	{ 419,	"OUS",		LAMB,	2,	-1,	1,	0x00 },
	{ 420,	"IVE",		LAMB,	2,	-1,	1,	0x00 },
	{ 421,	"IZE",		LAMB,	2,	-1,	1,	0x00 },
	{ 000,	0x00,		0x00,	0,	0,	0,	0x00 },
};
static CEngRL	m_Step5a[] =
{
	{ 501,	"E",	LAMB,	0,	-1,	1,	0x00 },
	{ 502,	"E",	LAMB,	0,	-1,	-1,	MAErmvE },
	{ 000,	0x00,	0x00,	0,	0,	0,	0x00 },
};
static CEngRL	m_Step5b[] =
{
	{ 503,	"LL",	"L",	1,	0,	1,	0x00 },
	{ 000,	0x00,	0x00,	0,	0,	0,	0x00 },
};

TDWORD	m_adwPos[3];

/*********************************************************/
void	MAEanal(CMA *pMA, TBYTE *pszTok)
{
	if ( strlen(pszTok) < 2 )
		return;

	if ( MADgetPOS(pszTok, m_adwPos) )
	{
		if ( IS_NOISE(m_adwPos[2]) )
		{
			if ( pMA->wMask & MA_NOISE )
			{
				strcpy(pMA->aMASL[pMA->shCntSL].szWord, pszTok);
				pMA->aMASL[pMA->shCntSL].wPosTag = MA_NOISE;
				pMA->shCntSL += 1;
			}
			return;
		}
	}

	m_pEnd = (char*)(pszTok + (m_nTokLen - 1));

	MAEreplaceEnd(pszTok, m_Step1a);
	m_ID = MAEreplaceEnd(pszTok, m_Step1b);
	if ( (m_ID == 106) || (m_ID == 107) )
		MAEreplaceEnd(pszTok, m_Step1b1);
	MAEreplaceEnd(pszTok, m_Step1c);
	MAEreplaceEnd(pszTok, m_Step2);
	MAEreplaceEnd(pszTok, m_Step3);
	MAEreplaceEnd(pszTok, m_Step4);
	MAEreplaceEnd(pszTok, m_Step5a);
	MAEreplaceEnd(pszTok, m_Step5b);

	strcpy(pMA->aMASL[pMA->shCntSL].szWord, pszTok);
	pMA->aMASL[pMA->shCntSL].wPosTag = MA_ENG;
	pMA->shCntSL += 1;

	return;
}
/*********************************************************/
TLONG	MAEreplaceEnd(char *pszTok, CEngRL *pEngRL)
{
	char	*pEnd;
	char	chTmp;

	while ( pEngRL->lID != 0 )
	{
		pEnd = m_pEnd - pEngRL->lOldOff;
		if ( pszTok <= pEnd )
		{
			if ( strcmp(pEnd, pEngRL->OldEnd) == 0 )
			{
				chTmp = *pEnd;
				*pEnd = 0x00;
				if ( pEngRL->lMinWordSZ < MAEchkWordSZ(pszTok) )
				{
					if ( !pEngRL->pfnCond || (*pEngRL->pfnCond)(pszTok) )
					{
						strcat(pszTok, pEngRL->NewEnd);
						m_pEnd = pEnd + pEngRL->lNewOff;
						break;
					}
				}

				*pEnd = chTmp;
			}	
		}

		pEngRL++;
	}

	return pEngRL->lID;
}

/*********************************************************/
TLONG	MAEchkWordSZ(char *pszTok)
{
	TINT	nRet=0;
	TINT	nState=0;

	while ( *pszTok != 0x00 )
	{
		switch ( nState )
		{
		case 0: nState = (ENG_IS_VOWEL(*pszTok))? 1 : 2;
				break;
		case 1:	nState = (ENG_IS_VOWEL(*pszTok))? 1 : 2;
				if ( nState == 2 )
					nRet++;
				break;
		case 2:	nState = (ENG_IS_VOWEL(*pszTok) || ('Y' == *pszTok))? 1 : 2;
				break;
		}
		pszTok++;
	}

	return nRet;
}
/*********************************************************/
TBOOL	MAEchkVowel(char *pszTok)
{
	if ( *pszTok == 0x00 )
		return 0;
	else
		return ( ENG_IS_VOWEL(*pszTok) || (strpbrk(pszTok + 1, "AEIOUY") != 0x00) );
}
/*********************************************************/
TBOOL	MAEaddE(char *pszTok)
{
	return ( (MAEchkWordSZ(pszTok) == 1) && MAEchkCVC(pszTok) );
}
/*********************************************************/
TBOOL	MAErmvE(char *pszTok)
{
	return ( (MAEchkWordSZ(pszTok) == 1) && !MAEchkCVC(pszTok) );
}
/*********************************************************/
TBOOL	MAEchkCVC(char *pszTok)
{
	TINT	nLen;

	nLen = strlen(pszTok);
	if ( nLen < 2 )
		return 0;

	m_pEnd = pszTok + nLen - 1;
	return ( (strchr("AEIOUWXY", *m_pEnd--) == 0x00) &&
		(strchr("AEIOUY", *m_pEnd--) != 0x00 ) &&
		(strchr("AEIOU", *m_pEnd) == 0x00) );
}

/*
**  END MA_ENG.C
*/
