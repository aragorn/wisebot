/*
**  SOFTBOT v4.0
**  API_SRCH.H
**  1999.10. By Jae-Bum, Kim.
*/
#ifndef _API_SR_
#define _API_SR_

#include  "softbot4.h"

#endif  /* _API_SRCH_  */

/* 몇가지 상수값을 지정한다.
 * 아마도 api_sr 의 버전이 v4.0 이어서 문제가 생기는 것으로 추측.
 * 4.0 버전의 api_sr 을 수정할 필요가 있다.
 * aragorn 2001-03-10
 */
#define MAX_DIT_LEN 1024
#define SCL_BUF64 64
#define SCL_BUF1024 1024


#ifdef _SBA_DLL_
EXPORT TBOOL  __stdcall  SBAConstructHSR(SBHANDLE *phSR, TINT LC);
EXPORT void  __stdcall  SBADestroyHSR(SBHANDLE *phSR);
EXPORT TINT __stdcall	SBASrchDoc(char *IP, TWORD Port, char *QueryST, SBHANDLE hRet);
EXPORT TINT __stdcall	SBASrchDocDTC(char *IP, TWORD Port, char *QueryST, SBHANDLE hRet);
#ifdef _NOT_USE_  /* 991103 KJB */
EXPORT TINT __stdcall	SBASrchDocDTR(char *IP, TWORD Port, char *QueryST, SBHANDLE hRet);
#endif  /* 991103 KJB */
EXPORT TINT __stdcall	SBAGetWordCnt(SBHANDLE hRet, TLONG *WordCnt);
EXPORT TINT __stdcall	SBAGetWordList(SBHANDLE hRet, char *Buf, TINT BufLen);
EXPORT TINT	__stdcall SBAGetTotalCnt(SBHANDLE hRet, TLONG *TotCnt);
#ifdef _NOT_USE_  /* 991103 KJB */
EXPORT TINT __stdcall SBAGetNSID(SBHANDLE hRet, TLONG *NSID);
#endif  /* 991103  KJB */
EXPORT TINT	__stdcall SBAGetRecvCnt(SBHANDLE hRet, TLONG *RecvCnt);
EXPORT TINT	__stdcall SBAGetDocInfo(SBHANDLE hRet, TINT Order, char *Buf, TINT BufLen);

#else
TBOOL  SBAConstructHSR(SBHANDLE *phSR, TINT LC);
void  SBADestroyHSR(SBHANDLE *phSR);
TINT	SBASrchDoc(char *IP, TWORD Port, char *QueryST, SBHANDLE hRet);
#ifdef _USE_FILTER_DOCS_
TINT	SBASrchDocFilter(char *IP, TWORD Port, char *QueryST, SBHANDLE hRet);
#endif
TINT	SBASrchDocDTC(char *IP, TWORD Port, char *QueryST, SBHANDLE hRet);
#ifdef _NOT_USE_  /* 991103 KJB */
TINT	SBASrchDocDTR(char *IP, TWORD Port, char *QueryST, SBHANDLE hRet);
TINT  SBAGetNSID(SBHANDLE hRet, TLONG *NSID);
#endif  /* 991103 KJB */
TINT	SBAGetWordCnt(SBHANDLE hRet, TLONG *WordCnt);
TINT	SBAGetWordList(SBHANDLE hRet, char *Buf, TINT BufLen);
TINT	SBAGetTotalCnt(SBHANDLE hRet, TLONG *TotCnt);
TINT	SBAGetRecvCnt(SBHANDLE hRet, TLONG *RecvCnt);
TINT	SBAGetDocInfo(SBHANDLE hRet, TINT Order, char *Buf, TINT BufLen);
#endif

/*
**  END API_SRCH.H
*/
