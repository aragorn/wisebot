/* $Id$ */
/*
**  MK_DIC.C
**  2001.05. BY JaeBum, Kim.
*/
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	"old_softbot.h"
#include	"lb_std.h"
#include	"ma_dic.h"
#include	"ma_code.h"
#include	"ma_data.h"
#include	"mk_dic.h"
#include    "softbot.h"

/*********************************************************/
void	loadEntry(char *RmdPH, char **dpEntry)
{
	TLONG	lFileLen;
	FILE	*fpRmd;

	/*  load base.rmd to memory  */
	fpRmd = STDopenFP(RmdPH, "rb");
	if ( fpRmd == 0x00 )
		STDErrSys("ERR open %s\n", RmdPH);

	STDseekFP(fpRmd, 0L, SEEK_END);
	lFileLen = (TLONG)ftell(fpRmd);
	STDseekFP(fpRmd, 0L, SEEK_SET);

	*dpEntry = (char*)sb_calloc(lFileLen + 1, 1);
	if ( *dpEntry == 0x00 )
		STDErrSys("ERR lack memory\n");

	if ( !STDreadFP(fpRmd, *dpEntry, lFileLen, 1) )
		STDErrSys("ERR read %s\n", RmdPH);

	STDcloseFP(fpRmd);
}

/*********************************************************/
TLONG	getEntryNum(char *pEntry)
{
	TLONG	 lEntryNum;

	lEntryNum = 0;
	if ( strtok(pEntry, "\r\n") != 0x00 )
	{
		do {
			lEntryNum++;
		} while ( strtok(0x00, "\r\n") != 0x00 );
	}

	return lEntryNum;
}

/*********************************************************/
void	setEntryTB(char *pEntry, char **dpEntryTB, TLONG lEntryNum)
{
	TLONG	i;
	char	*pCh, *pCh2;

	pCh = pEntry;
	for ( i = 0; i < lEntryNum; i++ )
	{
		dpEntryTB[i] = pCh;
		if ( strlen(pCh) == 0 )
			STDErrSys("ERR strange data\n");

		/*  move pCh to next word  */
		if ( i < (lEntryNum - 1) )
			for ( pCh += strlen(pCh) + 1; (*pCh == 0x00) || (*pCh == '\r') || (*pCh == '\n'); pCh++ );
	
		/*  isolate word  */
		if ( (pCh2 = strstr(dpEntryTB[i], " {")) == 0x00 )
			STDErrSys("ERR invalid data\n");

		*pCh2 = 0x00;
	}

	return;
}

/*********************************************************/
CBaseLN*	addBaseLN(CBaseLN **BaseRT, char *Word)
{
	TLONG	lBKNO;
	CBaseLN	*pBaseLN;

	pBaseLN = (CBaseLN*)sb_calloc(1, sizeof(CBaseLN));
	if ( pBaseLN == 0x00 )
		return (CBaseLN *)0x00;

	strcpy(pBaseLN->BaseSL.szWord, Word);

	lBKNO = GET_BASE_BKNO(Word);
	pBaseLN->pNext = BaseRT[lBKNO];
	BaseRT[lBKNO] = pBaseLN;

	return pBaseLN;
}

/*********************************************************/
void	setBaseRT(char **EntryTB, CBaseLN **BaseRT, TLONG lEntryNum)
{
	TLONG	i, lTotNum;
	TDWORD	dwPos;
	CBaseLN	*pBaseLN;
	char	*pCh;

	lTotNum = 0;
	for ( i = 0; i < lEntryNum; i++ )
	{
		if ( strlen(EntryTB[i]) >= MAX_BASE_LEN )
			continue;
		if ( strchr(EntryTB[i], '+') != 0x00 )
			continue;

		pBaseLN = addBaseLN(BaseRT, EntryTB[i]);
		if (pBaseLN == 0x00 )
			STDErrSys("ERR lack Mem BaseLN\n");

		lTotNum++;
		pCh = EntryTB[i] + strlen(EntryTB[i]) + 1;

		if ( (pCh = strtok(pCh, " \r\n")) == 0x00 )
			STDErrSys("ERR invalid data : %s\n", EntryTB[i]);
		do
		{
			if ( (strcmp(pCh, "{") == 0) || (strcmp(pCh, "}") == 0) )
				continue;
		
			GET_INT_POS(pCh, dwPos);
			if ( dwPos < MAX_POS_NUM && dwPos >= 0)
				pBaseLN->BaseSL.POS[dwPos / 32] += ( 1 << (dwPos % 32) );
			else
				printf("POS : %s, dwPos : %d\n", pCh, dwPos);
		} while( (pCh = strtok(0x00, " \r\n")) != 0x00 );
	}	/*  for(i)  */

	return;
}

/*********************************************************/
void	makeBase(char *DicPH, CBaseLN **BaseRT)
{
	TLONG	i, lEndOff, lBKOff;
	FILE	*fpBase;
	CBaseBK	BaseBK;
	CBaseLN	*pBaseLN;

	fpBase = STDopenFP(DicPH, "w+b");
	if ( fpBase == 0x00 )
		STDErrSys("ERR open %s\n", DicPH);

	memset(&BaseBK, 0x00, sizeof(CBaseBK));

	STDseekFP(fpBase, 0L, SEEK_SET);
	for ( i = 0; i < MAX_BASE_BK; i++ )
	{
		if ( !STDwriteFP(fpBase, &BaseBK, sizeof(CBaseBK), 1) )
			STDErrSys("ERR write base.dic");
	}
	lEndOff = (TLONG)ftell(fpBase);

	for ( i = 0; i < MAX_BASE_BK; i++ )
	{
		if ( BaseRT[i] == 0x00 )
			continue;

		memset(&BaseBK, 0x00, sizeof(CBaseBK) );
		lBKOff = GET_BASE_OFF(i);
		pBaseLN = BaseRT[i];
		for ( ; ; )
		{
			if ( pBaseLN == 0x00 )
				break;

			memcpy(&(BaseBK.aBaseSL[(BaseBK.shCntSL)++]), &(pBaseLN->BaseSL), sizeof(CBaseSL));
			if ( BaseBK.shCntSL == MAX_BASE_SL )
			{
				BaseBK.lExtOff = lEndOff;
				lEndOff += sizeof(CBaseBK);
				STDseekFP(fpBase, lBKOff, SEEK_SET);
				if ( !STDwriteFP(fpBase, &BaseBK, sizeof(CBaseBK), 1) )
					STDErrSys("ERR write base.dic\n");

				lBKOff = BaseBK.lExtOff;
				memset(&BaseBK, 0x00, sizeof(CBaseBK));
			}
			pBaseLN = pBaseLN->pNext;
		}	/*  while()  */

		STDseekFP(fpBase, lBKOff, SEEK_SET);
		if ( !STDwriteFP(fpBase, &BaseBK, sizeof(CBaseBK), 1) )
			STDErrSys("ERR write base.dic\n");
	}	/*  for()  */

	STDcloseFP(fpBase);

	return;
}

/*********************************************************/
void	unloadEntry(char *Entry)
{
	sb_free(Entry);
}


/*
**  RmdPH : base.rmd Full Path
**  DicPH : base.dic Full Path  */
/*********************************************************/
void	DICmakeBase(char *RmdPH, char *DicPH)
{
	TLONG	i;
	TLONG	lEntryNum;
	TLONG	lErrCnt=0;
	char	*Entry;
	char	**EntryTB;
	CBaseLN	**BaseRT, *pBaseLN;

	/*  load rmd to memory  */
	loadEntry(RmdPH, &Entry);

	/*  count entry number  */
	lEntryNum = getEntryNum(Entry);
	if ( lEntryNum <= 0 )
		STDErrSys("ERR no entry!!\n");

	/*  create EntryTable  */
	EntryTB = (char**)sb_calloc(lEntryNum, sizeof(char*));
	if ( EntryTB == 0x00 )
		STDErrSys("ERR lack memory\n");

	/*  isolate word  */
	setEntryTB(Entry, EntryTB, lEntryNum);

	/*  create BaseRT  */
	BaseRT = (CBaseLN**)sb_calloc(MAX_BASE_BK, sizeof(CBaseLN*));
	if ( BaseRT == 0x00 )
		STDErrSys("ERR lack memory\n");
	
	/*  set BaseRT  */
	setBaseRT(EntryTB, BaseRT, lEntryNum);

	/*  make base.dic  */
	makeBase(DicPH, BaseRT);

	/*  destroy BaseRT  */
	for ( i = 0; i < MAX_BASE_BK; i++ )
	{
		pBaseLN = BaseRT[i];

		while(pBaseLN)
		{
			BaseRT[i] = pBaseLN->pNext;
			sb_free(pBaseLN);
			pBaseLN = BaseRT[i];
		}
	}
	sb_free(BaseRT);

	/*  destroy EntryTB  */
	sb_free(EntryTB);

	/*  unload RMD  */
	unloadEntry(Entry);

	return;
}

/*********************************************************/
void	DICfilterBase(char *BasePH, char *Filter)
{
	FILE	*fpBase, *fpTxt;
	TLONG	i, j, lBaseOff, lTotal=0;
	TLONG	Job=0;
	TBYTE	szBase[MAX_BASE_LEN];
	CBaseBK	BaseBK;
	CBaseSL *pBaseSL;

	if ( STDcmpStrCase(Filter, "JOSA") == 0 )
		Job = 1;
	else if ( STDcmpStrCase(Filter, "EOMI") == 0 )
		Job = 2;
	else if ( STDcmpStrCase(Filter, "NOISE") == 0 )
		Job = 3;
	else if ( STDcmpStrCase(Filter, "USER") == 0 )
		Job = 0;
	else if ( STDcmpStrCase(Filter, "XP") == 0 )
		Job = 4;
	else if ( STDcmpStrCase(Filter, "F") == 0 )
		Job = 5;
	else if ( STDcmpStrCase(Filter, "XS") == 0 )
		Job = 6;

	fpBase = STDopenFP(BasePH, "rb");
	if ( fpBase == 0x00 )
		STDErrSys("ERR open %s\n", BasePH);

	fpTxt = STDopenFP("tmp.___", "w+b");
	if ( fpTxt == 0x00 )
		STDErrSys("ERR open tmp.___\n");

	pBaseSL = &(BaseBK.aBaseSL[0]);
	for ( i =0; i < MAX_BASE_BK; i++ )
	{
		lBaseOff = GET_BASE_OFF(i);

		for ( ; ; )
		{
			STDseekFP(fpBase, lBaseOff, SEEK_SET);
			if ( !STDreadFP(fpBase, &BaseBK, sizeof(CBaseBK), 1) )
				STDErrSys("ERR read BaseBK\n");

			for ( j = 0; j < BaseBK.shCntSL; j++ )
			{
				switch ( Job )
				{
				case 1:
					if ( !IS_P( (pBaseSL + j)->POS[0], (pBaseSL + j)->POS[1]) )
						continue;
					break;
				case 2:
					if ( !IS_E( (pBaseSL + j)->POS[0], (pBaseSL + j)->POS[1]) )
						continue;

					break;
				case 3:
					if ( !IS_NOISE( (pBaseSL + j)->POS[2]) )
						continue;
					break;
				case 4:
					if ( !IS_XP( (pBaseSL + j)->POS[0], (pBaseSL + j)->POS[1] ) )
						continue;

					break;
				case 5:
					if ( !IS_F( (pBaseSL + j)->POS[0], (pBaseSL + j)->POS[1] ) )
						continue;

					break;
				case 6:
					if ( !IS_XS( (pBaseSL + j)->POS[0], (pBaseSL + j)->POS[1] ) )
						continue;
					break;
				default:
					if ( !IS_USER( (pBaseSL + j)->POS[2]) )
						continue;
					break;
				}

				CDconvKSSM2KS((pBaseSL + j)->szWord, szBase);
				lTotal++;
				fprintf(fpTxt, "%s\n", szBase);
				printf("BK[%d]CNT[%d] : %s\n", i, lTotal, szBase);
			}	/*  for (j)  */

			if ( BaseBK.lExtOff <= 0 )
				break;
			lBaseOff = BaseBK.lExtOff;
		}	/*  for (;;)  */
	}

	STDcloseFP(fpTxt);
	STDcloseFP(fpBase);
}

/*********************************************************/
TBOOL	seekWord(CBasePL *pBasePL, TLONG *plBaseOff, TSINT *pshSlotCnt, CBaseBK *pBaseBK, char *pszWord)
{
	for ( ; ; )
	{
		if ( *plBaseOff >= pBasePL->lAlocSZ )
			break;

		memcpy(pBaseBK, (pBasePL->BasePL + *plBaseOff), sizeof(CBaseBK));
		for ( *pshSlotCnt = 0; *pshSlotCnt < pBaseBK->shCntSL; (*pshSlotCnt)++ )
		{
			if ( strcmp(pBaseBK->aBaseSL[*pshSlotCnt].szWord, pszWord) == 0 )
				break;
		}

		if ( *pshSlotCnt < pBaseBK->shCntSL )
			return 1;

		if ( pBaseBK->lExtOff <= 0x00 )
			break;

		*plBaseOff = pBaseBK->lExtOff;
	}

	return 0;
}

/*********************************************************/
void	addWord2Base(CBasePL *pBasePL, FILE *fpSrc, TDWORD jagil)
{
	CBaseBK	BaseBK;
	TLONG	lTotal=0;
	TLONG	lBaseBK, lBaseOff;
	TSINT	shSlotCnt;
	char	szWord[MAX_BASE_LEN * 2];
	char	szWord2[MAX_BASE_LEN * 2];
	char	*pCh;

	if ( fpSrc == 0x00 )
		return;

	STDseekFP(fpSrc, 0L, SEEK_SET);

	for ( ; ; )
	{
		if ( !fgets(szWord, MAX_BASE_LEN * 2, fpSrc) )
			break;

		pCh = STDtrimStr(szWord);
		if ( (strlen(pCh) >= MAX_BASE_LEN) || (strlen(pCh) < 1) )
		{
			printf("Skip Len => %s, %d\n", pCh, strlen(szWord));
			continue;
		}

		CDconvKS2KSSM(pCh, szWord2);
		lBaseBK = GET_BASE_BKNO(szWord2);
		lBaseOff = GET_BASE_OFF(lBaseBK);
		if ( seekWord(pBasePL, &lBaseOff, &shSlotCnt, &BaseBK, szWord2) )
		{
			BaseBK.aBaseSL[shSlotCnt].POS[jagil / 32] |= ( 1 << (jagil % 32) );
			memcpy((pBasePL->BasePL + lBaseOff), &BaseBK, sizeof(CBaseBK));
			continue;
		}

		if ( BaseBK.shCntSL < MAX_BASE_SL )
		{
			strcpy(BaseBK.aBaseSL[BaseBK.shCntSL].szWord, szWord2);
			BaseBK.aBaseSL[BaseBK.shCntSL].POS[jagil / 32] |= ( 1 << (jagil % 32) );
			BaseBK.shCntSL += 1;

			memcpy((pBasePL->BasePL + lBaseOff), &BaseBK, sizeof(CBaseBK));
		}
		else
		{
			if ( (TLONG)(pBasePL->lAlocSZ + sizeof(CBaseBK)) >= pBasePL->lPoolSZ )
			{
				printf("Skip Pool Size => %s\n", pCh);
				continue;
			}
			BaseBK.lExtOff = pBasePL->lAlocSZ;
			memcpy((pBasePL->BasePL + lBaseOff), &BaseBK, sizeof(CBaseBK));
			lBaseOff = pBasePL->lAlocSZ;
			pBasePL->lAlocSZ += sizeof(CBaseBK);

			memset(&BaseBK, 0x00, sizeof(CBaseBK));
			strcpy(BaseBK.aBaseSL[0].szWord, szWord2);
			BaseBK.aBaseSL[0].POS[jagil / 32] |= ( 1 << (jagil % 32) );
			BaseBK.shCntSL = 1;
			memcpy((pBasePL->BasePL + lBaseOff), &BaseBK, sizeof(CBaseBK));
		}
	}
}

/*********************************************************/
void	loadBaseDic(char *BasePH, CBasePL *pBasePL)
{
	FILE	*fpBase;

	fpBase = STDopenFP(BasePH, "r+b");
	if ( fpBase == 0x00 )
		STDErrSys("ERR open %s\n", BasePH);

	STDseekFP(fpBase, 0L, SEEK_END);
	pBasePL->lAlocSZ = (TLONG)ftell(fpBase);
	pBasePL->lPoolSZ = pBasePL->lAlocSZ * 2;
	pBasePL->BasePL = (char*)sb_calloc( pBasePL->lPoolSZ, sizeof(char));
	if ( pBasePL->BasePL == 0x00 )
		STDErrSys("ERR lack Mem\n");
	
	STDseekFP(fpBase, 0L, SEEK_SET);
	if ( !STDreadFP(fpBase, pBasePL->BasePL, pBasePL->lAlocSZ, 1) )
		STDErrSys("ERR read base.dic\n");

	STDcloseFP(fpBase);
}

/*********************************************************/
void	makeDicMA(char *MAPH, CBasePL *pBasePL)
{
	FILE	*fpMA;

	fpMA = STDopenFP(MAPH, "w+b");
	if ( fpMA == 0x00 )
		STDErrSys("ERR open %s\n", MAPH);

	if ( !STDwriteFP(fpMA, pBasePL->BasePL, pBasePL->lAlocSZ, 1) )
		STDErrSys("ERR write %s\n", MAPH);

	STDcloseFP(fpMA);
}

/*********************************************************/
void	DICmakeMA(char *MAPH, char *BasePH, char *NoisePH, char *UserPH)
{
	CBasePL	BasePL;
	FILE	*fpNoise, *fpUser;

	loadBaseDic(BasePH, &BasePL);

	fpNoise = STDopenFP(NoisePH, "rb");
	fpUser = STDopenFP(UserPH, "rb");

	if ( fpNoise != 0x00 )
	{
		printf("Add noise...\n");
		addWord2Base(&BasePL, fpNoise, 94);
	}
	if ( fpUser != 0x00 )
	{
		printf("Add user...\n");
		addWord2Base(&BasePL, fpUser, 95);
	}

	makeDicMA(MAPH, &BasePL);

	if ( fpNoise != 0x00 )
		STDcloseFP(fpNoise);
	if ( fpUser != 0x00 )
		STDcloseFP(fpUser);

	return;
}
/*********************************************************/
TBOOL	seekWordMA(FILE *fpMA, CBaseBK *pBaseBK, TSINT *pshSlotCnt, char *pszWord)
{
	TLONG	lBaseBK;
	TLONG	lBaseOff;

	lBaseBK = GET_BASE_BKNO(pszWord);
	lBaseOff = GET_BASE_OFF(lBaseBK);

	for ( ; ; )
	{
		STDseekFP(fpMA, lBaseOff, SEEK_SET);
		if ( !STDreadFP(fpMA, pBaseBK, sizeof(CBaseBK), 1) )
			return 0;

		for ( *pshSlotCnt = 0; *pshSlotCnt < pBaseBK->shCntSL; (*pshSlotCnt)++ )
		{
			if ( strcmp(pBaseBK->aBaseSL[*pshSlotCnt].szWord, pszWord) == 0 )
				break;
		}

		if ( *pshSlotCnt < pBaseBK->shCntSL )
			return 1;

		if ( pBaseBK->lExtOff <= 0 )
			break;

		lBaseOff = pBaseBK->lExtOff;
	}

	return 0;
}

/*********************************************************/
void	DICinqWordFromMA()
{
	CBaseBK	BaseBK;
	TSINT	shSlotCnt;
	FILE	*fpMA;
	char	*pCh;
	char	szWord2[MAX_BASE_LEN * 2];
	char	szWord3[MAX_BASE_LEN * 2];

	fpMA = STDopenFP("ma.dic", "rb");
	if ( fpMA == 0x00 )
		STDErrSys("ERR open ma.dic\n");

	for ( ; ; )
	{
		printf("Enter the word : ");
		pCh = STDgetInput();
		if ( pCh == 0x00 )
			break;

		CDconvKS2KSSM(pCh, szWord2);
		if ( !seekWordMA(fpMA, &BaseBK, &shSlotCnt, szWord2) )
		{
			printf("No exist => %s\n", pCh);
			CDconvKSSM2KS(szWord2, szWord3);
			printf("Word => %s\n", szWord3);
			continue;
		}

		CDconvKSSM2KS(BaseBK.aBaseSL[shSlotCnt].szWord, szWord2);
		printf("WORD => %s\n", szWord2);
		printf("POS0[%x]POS1[%x]POS2[%x]\n", BaseBK.aBaseSL[shSlotCnt].POS[0],
			BaseBK.aBaseSL[shSlotCnt].POS[1], BaseBK.aBaseSL[shSlotCnt].POS[2]);

	}

	STDcloseFP(fpMA);
}
/*********************************************************/
int main(int argc, char *argv[])
{

	if ( (argc < 2) )
	{
		printf("USAGE : mk_dic -Opt [Filter_Val]\n");
		printf("Opt : 'i', 'f', 'c'\n");
		printf(" i : Inquery\n");
		printf(" f : Filter\n");
		printf(" c : Create\n");
		exit(0);
	}

	switch ( argv[1][1] )
	{
	case 'i':
	case 'I':
		DICinqWordFromMA();
		break;
	case 'f':
	case 'F':
		DICfilterBase("ma.dic", argv[2]);
		break;
	case 'c':
	case 'C':
		DICmakeMA("ma.dic", "base.dic", "noise.txt", "user.txt");
		break;
	case 'b':
	case 'B':
		DICmakeBase("base.rmd", "base.dic");
		break;
	}

	return 0;
}


/*
**  END MK_DIC.C
*/

