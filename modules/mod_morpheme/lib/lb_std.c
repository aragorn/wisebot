/* $Id$ */
/*
**  SOFTBOT v4.5
**  LB_STD.C
**  2000.04 By KJB
*/
/* *************************************************************** */
#include "auto_config.h"
#include "softbot.h"

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>
#include  "old_softbot.h"
#if   defined(FREEBSD)
#	include  <sys/time.h>
#elif defined(LINUX)
#	include <sys/time.h>
#	include <time.h>
#elif defined(SOLARIS)
#	include  <sys/time.h>
#elif defined(AIX5)
#	include  <sys/time.h>
#else
#	error undefined platform
#endif

#include  <stdarg.h>

#include  "lb_std.h"

/*
               CHAR
*/
/* *************************************************************** */
TBOOL  STDisWhiteChar(char Ch)
{
	if ( Ch == ' ' || Ch == '\t' || Ch == '\n' || Ch == '\r' )
		return 1;
	else
		return 0;
}


/*
               STRING
*/
/* *************************************************************** */
TDWORD	STDhash(const char *pszData)
{
	TDWORD g, h = 0;

	while ( *pszData )
	{
		h = (h << 4 ) + *(TBYTE*)pszData++;
		g = h & (TDWORD)(0xF0000000);
		if ( g )
			h ^= g >> 24;

		h &= ~g;
	}

	return h;
}

/* *************************************************************** */
TINT  STDcmpStrCase(char *pszSrc, char *pszDest)
{
	TINT  nRet;

#ifdef _UNIX_VER_
	nRet = strcasecmp(pszSrc, pszDest);
#else
	nRet = strcmpi(pszSrc, pszDest);
#endif

	return nRet;
}

/* *************************************************************** */
TINT  STDcmpStrCaseN(char *pszSrc, char *pszDest, TINT nLen)
{
	TINT  nRet;

#ifdef _UNIX_VER_
	nRet = strncasecmp(pszSrc, pszDest, nLen);
#else
	nRet = strnicmp(pszSrc, pszDest, nLen);
#endif

	return nRet;
}

/* *************************************************************** */
char*  STDtrimStr(char *pszPH)
{
	TINT  i, nLen;
	char  *pCh;

	nLen = strlen(pszPH);
	if ( nLen <= 0 )
		return 0x00;

	for ( i = nLen - 1; i > 0; i-- )
	{
		if ( !STDisWhiteChar(pszPH[i]) )
			break;
		else
			pszPH[i] = 0x00;
	}


	pCh = pszPH;
	for ( ; ; )
	{
		if ( *pCh == 0x00 )
			return 0x00;

		if ( STDisWhiteChar(*pCh) )
			pCh++;
		else
			break;
	}

	return pCh;
}

/* *************************************************************** */
char*  STDgetInput()
{
	static char  szInput[256];  /*  [SBC_DOC_OID]; */
	TINT  i = 0, nCh;

	for ( ; ; )
	{
		nCh = fgetc(stdin);
		if ( nCh == EOF )
			break;
		else if ( (char)nCh == '\n' )
			break;

		szInput[i++] = (char)nCh;
	}

	szInput[i] = 0x00;
	if ( i == 0 )
		return 0x00;

	return szInput;
}

/*
               DIRECTORY
*/
/* *************************************************************** */
void  STDmakeDR(char *pszDR)
{
	TINT  nLen;
	char  *pSrt;

	if ( pszDR == 0x00 )
		return;

	pSrt = STDtrimStr(pszDR);
	if ( pSrt == 0x00 )
		sprintf(pszDR, ".%s", DS_DIR);
	else
	{
		nLen = strlen(pSrt);
		if ( pSrt[nLen - 1] != DC_DIR )
		{
			pSrt[nLen] = DC_DIR; pSrt[nLen + 1] = 0x00;
		}

		strcpy(pszDR, pSrt);
	}
}

/*
**  STDsplitPH
**  Buf : Full Path To Be Divided
**  Drv : Drive Name
**  Dir : Directory Name
**  FileNM : File Name
**  Ext : Extention Name
*/
/* *************************************************************** */
void STDsplitPH(char *Buf, char *Drv, char *Dir, char *FileNM, char *Ext)
{
	TINT  i, nLen;
	char  *pSrtPH, *pCh;
	char  *pDrv, *pDir, *pFile, *pExt;

	pSrtPH = STDtrimStr(Buf);
	if ( pSrtPH == 0x00 )
		return;

	nLen = strlen(pSrtPH);

	pCh = pSrtPH;
	if ( (isalpha((int)*pCh)) && (*(pCh + 1) == ':') )
		pDrv = pSrtPH;
	else
		pDrv = 0x00;
	
	pDir = pSrtPH;
	for ( ; ; )
	{
		if ( *pDir == 0x00 )
			break;

		if ( *pDir == '.' )
			break;
		if ( *pDir == DC_DIR )
			break;
		else
			pDir++;
	}

	pFile = &(pSrtPH[nLen - 1]);
	for ( ; ; )
	{
		if ( pFile <= pSrtPH )
			break;

		if ( *pFile == DC_DIR )
		{
			pFile = pFile + 1; break;
		}
		else if ( *pFile == ':' )
		{
			pFile = 0x00; break;
		}

		pFile--;
	}

	if ( pFile == 0x00 )
		pExt = 0x00;
	else
	{
		for ( pExt = &(pSrtPH[nLen - 1]); pExt > pFile; pExt-- )
		{
			if ( *pExt == '.' )
				break;
		}
	}
	
	if ( Drv != 0x00 )
	{
		if ( pDrv == 0x00 )
			Drv[0] = 0x00;
		else
		{
			Drv[0] = *pDrv; Drv[1] = ':'; Drv[2] = 0x00;
		}
	}

	if ( Dir != 0x00 )
	{
		if ( pDir == 0x00 )
			Dir[0] = 0x00;
		else if ( pFile == 0x00 )
			strcpy(Dir, pDir);
		else
		{
			for ( i = 0; pDir < pFile; i++, pDir++ )
				Dir[i] = *pDir;
			Dir[i] = 0x00;
		}
	}

	if ( FileNM != 0x00 )
	{
		if ( pFile == 0x00 )
			FileNM[0] = 0x00;
		else if ( pExt == 0x00 )
			strcpy(FileNM, pFile);
		else
		{
			for ( i = 0; pFile < pExt; i++, pFile++ )
				FileNM[i] = *pFile;
			FileNM[i] = 0x00;
		}
	}

	if ( Ext != 0x00 )
	{
		if ( pExt == 0x00 )
			Ext[0] = 0x00;
		else
			strcpy(Ext, pExt);
	}
}

/*
               ITEM
*/
/* *************************************************************** */
TSIZE	STDgetItem(char *Src, char *ItemNM, char *Brk, char *Buf, TINT Size)
{
	TINT i, nSrcLen, nItemLen;
	char *pSrt, *pEnd, *pTmp;

	nSrcLen = strlen(Src);
	nItemLen = strlen(ItemNM);

	pSrt = strstr(Src, ItemNM);
	if ( pSrt == 0x00 )
		return 0;

	if ( Brk == 0x00 )
		pEnd = Src + nSrcLen;
	else
	{
		pEnd = strstr((pSrt + nItemLen), Brk);
		if ( pEnd == 0x00 )
			pEnd = Src + nSrcLen;
	}

	if ( pSrt >= pEnd )
		return 0;

	i = 0;
	for ( pTmp = pSrt + nItemLen; pTmp < pEnd; pTmp++, i++ )
	{
		if ( i >= Size - 1 )
			break;
		Buf[i] = *pTmp;
	}
	Buf[i] = 0x00;

	return i;
}

/* *************************************************************** */
TSIZE	STDgetTotElement(char *Src, char Brk)
{
	TINT	i, nLen, nCnt=0;

	nLen = strlen(Src);
	if ( nLen == 0 )
		return 0;

	if ( Src[nLen - 1] != Brk )
		nCnt = 1;

	for ( i = 0; i < nLen; i++ )
	{
		if ( Src[i] == Brk )
		{
			if ( i != 0 )
				nCnt++;
		}
	}
	
	return nCnt;
}

/* *************************************************************** */
TSIZE	STDgetElement(char *Src, char Brk, TINT Order, char *Buf)
{
	TINT	i, j, nLen, nCnt=0;
	char	*pSrt, *pEnd;

	nLen = strlen(Src);
	if ( nLen == 0 )
		return 0;

	for ( i = 0; i < nLen; i++ )
	{
		if ( Src[i] == Brk )
		{
			if ( i != 0 )
				nCnt++;
			i++;
		}
		if ( nCnt == Order )
			break;
	}

	pSrt = &(Src[i]);
	for ( j = i; j < nLen; j++ )
	{
		if ( Src[j] == Brk )
			break;
	}
	pEnd = &(Src[j]);

	nLen = pEnd - pSrt;

	if ( nLen == 0 )
		return 0;

	for ( i = 0; i < nLen; i++ )
		Buf[i] = *(pSrt + i);

	Buf[i] = 0x00;

	return nLen;
}


/* *************************************************************** */
TBOOL	STDsplitVal(char *Src, char *Var, char *Val)
{
	TINT	i, nLen;
	char	*pCh;

	pCh = strchr(Src, '=');
	if ( pCh == 0x00 )
		return 0;

	nLen = pCh - Src;
	for ( i = 0; i < nLen; i++ )
		Var[i] = toupper(Src[i]);
	Var[i] = 0x00;

	strcpy(Val, (pCh + 1));
	
	return 1;
}


/* *************************************************************** */
void  STDmakeDocIO(char *pszDit, CDocIO *pDocIO)
{
	char  szTmp[512];
	TINT  nLen;

	nLen = STDgetItem(pszDit, "HIT=", "^", szTmp, 512);
	if ( nLen <= 0 )
		pDocIO->HIT = 0;
	else 
		pDocIO->HIT = (TLONG)atol(szTmp);

	nLen = STDgetItem(pszDit, "DID=", "^", szTmp, 512);
	if ( nLen <= 0 )
		pDocIO->DID = -1;
	else
		pDocIO->DID = (TLONG)atol(szTmp);

	nLen = STDgetItem(pszDit, "DST=", "^", szTmp, 512);
	if ( nLen <= 0 )
		pDocIO->DST = 0;
	else
		pDocIO->DID = (TWORD)atoi(szTmp);

#ifdef _NOT_
	nLen = STDgetItem(pszDit, "DIC=", "^", szTmp, 512);
	if ( nLen <= 0 )
		pDocIO->DIC = 0;
	else
		pDocIO->DIC = (TWORD)atoi(szTmp);

	nLen = STDgetItem(pszDit, "DAC=", "^", szTmp, 512);
	if ( nLen <= 0 )
		pDocIO->DAC = 0;
	else
		pDocIO->DAC = (TLONG)atol(szTmp);
#endif

	nLen = STDgetItem(pszDit, "DN=", "^", szTmp, 512);
	if ( nLen == 0 || nLen >= SBC_DOC_DN)
		pDocIO->DN[0] = 0x00;
	else
		strcpy(pDocIO->DN, szTmp);

	nLen = STDgetItem(pszDit, "CMT=", "^", szTmp, 512);
	if ( nLen <= 0 )
		pDocIO->CMT[0] = 0x00;
	else 
		strcpy(pDocIO->CMT, szTmp);

	nLen = STDgetItem(pszDit, "OID=", "^", szTmp, 512);
	if ( nLen == 0 || nLen >= SBC_DOC_OID)
		pDocIO->OID[0] = 0x00;
	else
		strcpy(pDocIO->OID, szTmp);

	nLen = STDgetItem(pszDit, "DTR=", "^", szTmp, 512);
	if ( nLen != 8 )
		pDocIO->DTR[0] = 0x00;
	else
		strcpy(pDocIO->DTR, szTmp);

	nLen = STDgetItem(pszDit, "DTC=", "^", szTmp, 512);
	if ( nLen != 8 )
		pDocIO->DTC[0] = 0x00;
	else
		strcpy(pDocIO->DTC, szTmp);

	nLen = STDgetItem(pszDit, "CT=", "^", szTmp, 512);
	if ( nLen <= 0 )
		pDocIO->CT[0] = 0x00;
	else
		strcpy(pDocIO->CT, szTmp);

	nLen = STDgetItem(pszDit, "TT=", "^", szTmp, 512);
	if ( nLen <= 0 || nLen >= SBC_DOC_TT)
		pDocIO->TT[0] = 0x00;
	else
		strcpy(pDocIO->TT, szTmp);

	nLen = STDgetItem(pszDit, "AT=", "^", szTmp, 512);
	if ( nLen <= 0 || nLen >= SBC_DOC_AT )
		pDocIO->AT[0] = 0x00;
	else
		strcpy(pDocIO->AT, szTmp);

	nLen = STDgetItem(pszDit, "KW=", "^", szTmp, 512);
	if ( nLen <= 0 || nLen >= SBC_DOC_KW )
		pDocIO->KW[0] = 0x00;
	else
		strcpy(pDocIO->KW, szTmp);

	nLen = STDgetItem(pszDit, "FD1=", "^", szTmp, 512);
	if ( nLen <= 0 || nLen >= SBC_DOC_FD )
		pDocIO->FD1[0] = 0x00;
	else
		strcpy(pDocIO->FD1, szTmp);

	nLen = STDgetItem(pszDit, "FD2=", "^", szTmp, 512);
	if ( nLen <= 0 || nLen >= SBC_DOC_FD )
		pDocIO->FD2[0] = 0x00;
	else
		strcpy(pDocIO->FD2, szTmp);

	nLen = STDgetItem(pszDit, "FD3=", "^", szTmp, 512);
	if ( nLen <= 0 || nLen >= SBC_DOC_FD )
		pDocIO->FD3[0] = 0x00;
	else
		strcpy(pDocIO->FD3, szTmp);

	nLen = STDgetItem(pszDit, "FD4=", "^", szTmp, 512);
	if ( nLen <= 0 || nLen >= SBC_DOC_FD )
		pDocIO->FD4[0] = 0x00;
	else
		strcpy(pDocIO->FD4, szTmp);

	nLen = STDgetItem(pszDit, "FD5=", "^", szTmp, 512);
	if ( nLen <= 0 || SBC_DOC_FD )
		pDocIO->FD5[0] = 0x00;
	else
		strcpy(pDocIO->FD5, szTmp);

	return;
}

/*
               FILE
*/
/* *************************************************************** */
FILE *STDopenFP(char *pszFileNM, char *pszMode)
{
	FILE *fpFile;

	fpFile = sb_fopen(pszFileNM, pszMode);
	
	return fpFile;
}

/* *************************************************************** */
void STDcloseFP(FILE *fpFile)
{
	fclose(fpFile);
}

/* *************************************************************** */
void  STDseekFP(FILE *fpFile, TLONG Off, TINT Base)
{
	for ( ; ; )
	{
		if ( fseek(fpFile, Off, Base) >= 0 )
			break;
	}

	return;
}

/* *************************************************************** */
TBOOL STDreadFP(FILE *fpFile, void *pObj, TINT nSize, TINT nCnt)
{
	if ( fread(pObj, nSize, nCnt, fpFile) != (TDWORD)nCnt )
		return 0;

	return 1;
}

/* *************************************************************** */
TBOOL STDwriteFP(FILE *fpFile, void *pObj, TINT nSize, TINT nCnt)
{
	if ( fwrite(pObj, nSize, nCnt, fpFile) != (TDWORD)nCnt )
		return 0;

	return 1;
}

/* *************************************************************** */
TBOOL	STDreadFPNum(FILE *fpFile, TINT *pnNum)
{

	fseek(fpFile, 0L, SEEK_SET);
	if ( fscanf(fpFile, "%d", pnNum) == EOF )
		return 0;

	return 1;
}

/* *************************************************************** */
TBOOL	STDwriteFPNum(FILE *fpFile, TINT nNum)
{
	fseek(fpFile, 0L, SEEK_SET);
	if ( fprintf(fpFile, "%d", nNum) == EOF )
		return 0;

	return 1;
}

/* ***************************************************** */
TBOOL  STDmakeFileOff(FILE *fpDest, FILE *fpSrc, TLONG lSrt, TLONG lSize)
{
	TLONG i, lLoopCnt, lLeft;
	char  szBuf[SBC_DOC_DIT + 4];

	lLoopCnt = lSize / SBC_DOC_DIT;
	lLeft = lSize % SBC_DOC_DIT;
	if ( (lLeft == 0) && (lSize > 0) )
	{
		lLoopCnt--;
		lLeft = (TLONG)SBC_DOC_DIT;
	}

	STDseekFP(fpSrc, lSrt, SEEK_SET);
	STDseekFP(fpDest, 0L, SEEK_SET);
	for ( i = 0; i < lLoopCnt; i++ )
	{
		if ( !STDreadFP(fpSrc, szBuf, SBC_DOC_DIT, 1) < 1 )
			return 0;

		if ( !STDwriteFP(fpDest, szBuf, SBC_DOC_DIT, 1) )
			return 0;
	}

	if ( !STDreadFP(fpSrc, szBuf, lLeft, 1) )
		return 0;

	if ( !STDwriteFP(fpDest, szBuf, lLeft, 1) )
		return 0;

	return 1;
}

/*
               DATE / TIME
*/
/* *************************************************************** */
void  STDgetTime(CSBTM  *pSBTM)
{
#ifdef _WIN_VER_
	time_t  TM;
#else
	struct timeval  TM;
#endif

#ifdef _WIN_VER_
	time(&TM);
	pSBTM->lSec = TM;
	pSBTM->lUSec = 0;
#else
	gettimeofday(&TM, NULL);
	pSBTM->lSec = TM.tv_sec;
	pSBTM->lUSec = TM.tv_usec;
#endif
}

/* *************************************************************** */
char*  STDdiffTime(CSBTM *pEndTM, CSBTM *pSrtTM)
{
	static  char  szDiffTM[32];

	sprintf(szDiffTM, "%.4f", (((pEndTM->lSec - pSrtTM->lSec) * 1000000) + (pEndTM->lUSec - pSrtTM->lUSec)) / 1000000.);

	return szDiffTM;
}

/* *************************************************************** */
void STDgetToday(CToday *pToday)
{
	time_t	Time;
	struct tm *pTm;

	time(&Time);
	pTm = localtime(&Time);

	pToday->wYear = pTm->tm_year + 1900;
	pToday->wMonth = pTm->tm_mon + 1;
	pToday->wDay = pTm->tm_mday;
	pToday->wHour = pTm->tm_hour;
	pToday->wMin = pTm->tm_min;
	pToday->wSec = pTm->tm_sec;
}

/* *************************************************************** */
void STDgetSubDate(CToday *pToday, TINT Sub)
{
	time_t	Time;
	struct tm *pTm;

	time(&Time);
	Time -= (Sub * 86400);
	pTm = localtime(&Time);

	pToday->wYear = pTm->tm_year + 1900;
	pToday->wMonth = pTm->tm_mon + 1;
	pToday->wDay = pTm->tm_mday;
	pToday->wHour = pTm->tm_hour;
	pToday->wMin = pTm->tm_min;
	pToday->wSec = pTm->tm_sec;
}

/* *************************************************************** */
TLONG STDmakeDate(CToday *pToday)
{
	TLONG lDate;
	char  szBuf[24];

	sprintf(szBuf, "%04d%02d%02d", pToday->wYear, 
		pToday->wMonth, pToday->wDay);

	lDate = (TLONG)atol(szBuf);

	return lDate;
}

/* *************************************************************** */
char*  STDmakeDateStr(CToday *pToday)
{
	static char szDate[SBC_DT];

	sprintf(szDate, "%04d%02d%02d", pToday->wYear, 
		pToday->wMonth, pToday->wDay);

	return (char*)&(szDate[0]);
}

/*
               ERROR
*/
/* *************************************************************** */
void  STDErrRep(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);

	return;
}

/* *************************************************************** */
void  STDErrSys(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
	exit(1);
}

/* *************************************************************** */
void STDErrQuit(TINT nErr, char *pszPlace)
{
	printf("\n>>>   ERROR   <<<\n");
	printf("ERROR NO(%d) At %s\n", nErr, pszPlace);
	exit(1);
}


/* ******************************************* */
void STDsetErrDesc(TINT ErrNU, char *Buf)
{
	switch ( ErrNU )
	{
	case ERS_FILE_OPEN:
		strcpy(Buf, "Can't File Open");
		break;
	case ERS_FILE_READ:
		strcpy(Buf, "Can't File Read");
		break;
	case ERS_FILE_WRITE:
		strcpy(Buf, "Can't File Write");
		break;
	default:
		strcpy(Buf, "Unknown Error");
		break;
	}
}

/* **
** END SBSTDLIB.C
** */
