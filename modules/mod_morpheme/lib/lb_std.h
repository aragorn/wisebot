/* $Id$ */
/*
** SOFTBOT v4.5
** LB_STD.H
** 2000.04. BY KJB
*/
#ifndef _LB_STD_
#define _LB_STD_

#include "old_softbot.h"

/*
               CONSTANT
*/
/* *************************************************************** */
#define CONST_BUF1024	(1024)
#define CONST_BUF512	(512)
#define CONST_BUF		(256)
#define CONST_BUF256	(256)
#define CONST_BUF128	(128)
#define CONST_BUF64		(64)
#define CONST_BUF32		(32)
#define CONST_DRV		(4)
#define CONST_DIR		(256)
#define CONST_PATH		(256)
#define CONST_FILE		(32)
#define CONST_EXT		(8)
#define CONST_ITEM		(256)

/*
               CToday
*/
/* *************************************************************** */
typedef struct tagToday
{
	TWORD	wYear;
	TWORD	wMonth;
	TWORD	wDay;
	TWORD	wHour;
	TWORD	wMin;
	TWORD	wSec;
} CToday;

/*
	           CSBTM
*/
/* *************************************************************** */
typedef struct tagSBTM
{
	TLONG  lSec;
	TLONG  lUSec;
} CSBTM;

#endif /* _LB_STD_ */

/*
               ProtoType
*/
/* *************************************************************** */
TBOOL  STDisWhiteChar(char Ch);
TDWORD STDhash(const char *pszData);
TINT   STDcmpStrCase(char *pszSrc, char *pszDest);
TINT   STDcmpStrCaseN(char *pszSrc, char *pszDest, TINT nLen);
char*  STDtrimStr(char *pszPH);
char*  STDgetInput();

void   STDmakeDR(char *pszDR);
void   STDsplitPH(char *Buf, char *Drv, char *Dir, char *FileNM, char *Ext);
TSIZE  STDgetItem(char *Src, char *ItemNM, char *Brk, char *Buf, TINT Size);
TSIZE  STDgetTotElement(char *Src, char Brk);
TSIZE  STDgetElement(char *Src, char Brk, TINT Order, char *Buf);
TBOOL  STDsplitVal(char *Src, char *Var, char *Val);
void   STDmakeDocIO(char *pszDit, CDocIO *pDocIO);

FILE   *STDopenFP(char *pszFileNM, char *pszMode);
void   STDcloseFP(FILE *fpFile);
void  STDseekFP(FILE *fpFile, TLONG Off, TINT Base);
TBOOL  STDreadFP(FILE *fpFile, void *pObj, TINT nSize, TINT nCnt);
TBOOL  STDwriteFP(FILE *fpFile, void *pObj, TINT nSize, TINT nCnt);
TBOOL  STDreadFPNum(FILE *fpFile, TINT *pnNum);
TBOOL  STDwriteFPNum(FILE *fpFile, TINT nNum);
TBOOL  STDmakeFileOff(FILE *fpDest, FILE *fpSrc, TLONG lSrt, TLONG lSize);

void   STDgetTime(CSBTM  *pSBTM);
char*  STDdiffTime(CSBTM *pEndTM, CSBTM *pSrtTM);
void   STDgetToday(CToday *pToday);
void   STDgetSubDate(CToday *pToday, TINT Sub);
TLONG  STDmakeDate(CToday *pToday);
char*  STDmakeDateStr(CToday *pToday);

void  STDErrRep(char *fmt, ...);
void  STDErrSys(char *fmt, ...);
void  STDErrQuit(TINT nErr, char *pszPlace);
void  STDsetErrDesc(TINT ErrNU, char *Buf);

/*
** END LB_STD.H
*/
