/*
**  SOFTBOT v4.0
**  API_SR.C
**  1999.10. By Jae-Bum, Kim.
*/
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  "api_sr.h"
#include  "api_lib.h"
#include  "lb_std.h"
#include  "lb_tcp.h"
#ifdef _UNIX_VER_
#include  <sys/time.h>
#endif

/* ******************************************* */
#ifdef _SBA_DLL_
EXPORT TBOOL  __stdcall  SBAConstructHSR(SBHANDLE *phSR, TINT LC)
#else
TBOOL  SBAConstructHSR(SBHANDLE *phSR, TINT LC)
#endif
{
	TINT  i;

	if ( LC <= 0 )
		return 0;

	*phSR = (CHSR*)calloc(sizeof(CHSR), 1);
	if ( *phSR == 0x00 )
		return 0;

	((CHSR*)(*phSR))->WordList = (char*)calloc(MAX_DIT_LEN, 1);
	if ( ((CHSR*)(*phSR))->WordList == 0x00 )
		return 0;

	((CHSR*)(*phSR))->aDocInfo = (char**)calloc(sizeof(char*), LC);
	if ( ((CHSR*)(*phSR))->aDocInfo == 0x00 )
		return 0;

	((CHSR*)(*phSR))->AllocCnt = (TLONG)LC;
	for ( i = 0; i < LC; i++ )
	{
		((CHSR*)(*phSR))->aDocInfo[i] = (char*)calloc(MAX_DIT_LEN, 1);
		if ( ((CHSR*)(*phSR))->aDocInfo[i] == 0x00 )
			return 0;
	}

	return 1;
}

/* ******************************************* */
#ifdef _SBA_DLL_
EXPORT void  __stdcall  SBADestroyHSR(SBHANDLE *phSR)
#else
void  SBADestroyHSR(SBHANDLE *phSR)
#endif
{
	TINT  i;

	if ( *phSR == 0x00 )
		return;

	free( ((CHSR*)(*phSR))->WordList );
	for ( i = 0; i < ((CHSR*)(*phSR))->AllocCnt; i++ )
		free( ((CHSR*)(*phSR))->aDocInfo[i] );

	free( ((CHSR*)(*phSR)) );

	*phSR = 0x00;

	return;
}

/* ******************************************* */
#ifdef _SBA_DLL_
EXPORT TINT __stdcall	SBASrchDoc(char *IP, TWORD Port, char *QueryST, SBHANDLE hRet)
#else
TINT	SBASrchDoc(char *IP, TWORD Port, char *QueryST, SBHANDLE hRet)
#endif
{
	char	szBuf[SCL_BUF64];
	char	szBuf2[SCL_BUF1024];
	TINT	i, nLen;
	TSOCK	Sock;

	Sock = TCPSocket();
	if ( Sock < 0 )
		return ERT_SOCKET;

	if ( !TCPConnect(Sock, IP, Port) )
		return SBAErrRet(Sock, ERT_CONNECT);

	/* Send OP CODE */
	if ( !TCPSendData(Sock, OP_SR_DOC, 3) )
		return SBAErrRet(Sock, ERT_SEND_DATA);

	/* Recv ACK NAK */
	if ( !TCPRecvData(Sock, szBuf, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);
	if ( TCPCmpOP(szBuf, OP_NAK) )
		return SBAErrRecv(Sock);

	/* Send Data */
	strcpy(szBuf2, QueryST);
	if ( !TCPSendData(Sock, szBuf2, strlen(szBuf2)) )
		return SBAErrRet(Sock, ERT_SEND_DATA);

	/* Recv ACK NAK */
	if ( !TCPRecvData(Sock, szBuf, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);

	if ( TCPCmpOP(szBuf, OP_NAK) )
		return SBAErrRecv(Sock);

	/* Recv Word List From Server */
	if ( !TCPRecvData(Sock, szBuf2, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);
	szBuf2[nLen] = 0x00;
	strcpy(((CHSR*)hRet)->WordList, szBuf2);

	/* Recv Total List Count From Server */
	if ( !TCPRecvData(Sock, szBuf2, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);
	szBuf2[nLen] = 0x00;

	((CHSR*)hRet)->TotCnt = (TLONG)atol(szBuf2);


	/* Recv List Count From Server */
	if ( !TCPRecvData(Sock, szBuf2, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);
	szBuf2[nLen] = 0x00;

	((CHSR*)hRet)->RecvCnt = (TLONG)atol(szBuf2);

	/* Recv Each List From Server */
	for ( i = 0; i < ((CHSR*)hRet)->RecvCnt; i++ )
	{
		if ( !TCPRecvData(Sock, szBuf2, &nLen) )
			return SBAErrRet(Sock, ERT_RECV_DATA);
		szBuf2[nLen] = 0x00;

		strcpy(((CHSR*)hRet)->aDocInfo[i], szBuf2);
	}

	TCPClose(Sock);

	return 0;
}


#define _USE_FILTER_DOCS_ //FIXME
#ifdef _USE_FILTER_DOCS_
/* ******************************************* */
#ifdef _SBA_DLL_
EXPORT TINT __stdcall	SBASrchDocFilter(char *IP, TWORD Port, char *QueryST, SBHANDLE hRet)
#else
TINT	SBASrchDocFilter(char *IP, TWORD Port, char *QueryST, SBHANDLE hRet)
#endif
{
	char	szBuf[SCL_BUF64];
	char	szBuf2[SCL_BUF1024];
	TINT	i, nLen;
	TSOCK	Sock;

	Sock = TCPSocket();
	if ( Sock < 0 )
		return ERT_SOCKET;

	if ( !TCPConnect(Sock, IP, Port) )
		return SBAErrRet(Sock, ERT_CONNECT);

	/* Send OP CODE */
	if ( !TCPSendData(Sock, OP_SR_FILTER, 3) )
		return SBAErrRet(Sock, ERT_SEND_DATA);

	/* Recv ACK NAK */
	if ( !TCPRecvData(Sock, szBuf, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);
	if ( TCPCmpOP(szBuf, OP_NAK) )
		return SBAErrRecv(Sock);

	/* Send Data */
	strcpy(szBuf2, QueryST);
	if ( !TCPSendData(Sock, szBuf2, strlen(szBuf2)) )
		return SBAErrRet(Sock, ERT_SEND_DATA);

	/* Recv ACK NAK */
	if ( !TCPRecvData(Sock, szBuf, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);

	if ( TCPCmpOP(szBuf, OP_NAK) )
		return SBAErrRecv(Sock);

	/* Recv Word List From Server */
	if ( !TCPRecvData(Sock, szBuf2, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);
	szBuf2[nLen] = 0x00;
	strcpy(((CHSR*)hRet)->WordList, szBuf2);

	/* Recv Total List Count From Server */
	if ( !TCPRecvData(Sock, szBuf2, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);
	szBuf2[nLen] = 0x00;
	((CHSR*)hRet)->TotCnt = (TLONG)atol(szBuf2);

	/* Recv List Count From Server */
	if ( !TCPRecvData(Sock, szBuf2, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);
	szBuf2[nLen] = 0x00;
	((CHSR*)hRet)->RecvCnt = (TLONG)atol(szBuf2);

	/* Recv End Count From Server */
	if ( !TCPRecvData(Sock, szBuf2, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);
	szBuf2[nLen] = 0x00;
	((CHSR*)hRet)->EndCnt = (TLONG)atol(szBuf2);

	/* Recv Each List From Server */
	for ( i = 0; i < ((CHSR*)hRet)->RecvCnt; i++ )
	{
		if ( !TCPRecvData(Sock, szBuf2, &nLen) )
			return SBAErrRet(Sock, ERT_RECV_DATA);
		szBuf2[nLen] = 0x00;

		strcpy(((CHSR*)hRet)->aDocInfo[i], szBuf2);
	}

	TCPClose(Sock);

	return 0;
}
#endif

/* ******************************************* */
#ifdef _SBA_DLL_
EXPORT TINT __stdcall	SBASrchDocDTC(char *IP, TWORD Port, char *QueryST, SBHANDLE hRet)
#else
TINT	SBASrchDocDTC(char *IP, TWORD Port, char *QueryST, SBHANDLE hRet)
#endif
{
	char	szBuf[SCL_BUF64];
	char	szBuf2[SCL_BUF1024];
	TINT	i, nLen;
	TSOCK	Sock;

	Sock = TCPSocket();
	if ( Sock < 0 )
		return ERT_SOCKET;

	if ( !TCPConnect(Sock, IP, Port) )
		return SBAErrRet(Sock, ERT_CONNECT);

	/* Send OP CODE */
	if ( !TCPSendData(Sock, OP_SR_DTC, 3) )
		return SBAErrRet(Sock, ERT_SEND_DATA);

	/* Recv ACK NAK */
	if ( !TCPRecvData(Sock, szBuf, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);

	if ( TCPCmpOP(szBuf, OP_NAK) )
		return SBAErrRecv(Sock);

	/* Send Data */
	if ( !TCPSendData(Sock, QueryST, strlen(QueryST)) )
		return SBAErrRet(Sock, ERT_SEND_DATA);

	/* Recv ACK NAK */
	if ( !TCPRecvData(Sock, szBuf, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);

	if ( TCPCmpOP(szBuf, OP_NAK) )
		return SBAErrRecv(Sock);

	/* Recv Total List Count From Server */
	if ( !TCPRecvData(Sock, szBuf2, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);
	szBuf2[nLen] = 0x00;
	((CHSR*)hRet)->TotCnt = (TLONG)atol(szBuf2);

#ifdef _NOT_USE_  /* 991103 KJB */
	/* Recv NSID */
	if ( !TCPRecvData(Sock, szBuf2, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);
	szBuf2[nLen] = 0x00;
	((CHSR*)hRet)->NSID = (TLONG)atol(szBuf2);
#endif  /* 991103 KJB */

	/* Recv List Count From Server */
	if ( !TCPRecvData(Sock, szBuf2, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);
	szBuf2[nLen] = 0x00;
	((CHSR*)hRet)->RecvCnt = (TLONG)atol(szBuf2);

	/* Recv Each List From Server */
	for ( i = 0; i < ((CHSR*)hRet)->RecvCnt; i++ )
	{
		if ( !TCPRecvData(Sock, szBuf2, &nLen) )
			return SBAErrRet(Sock, ERT_RECV_DATA);
		szBuf2[nLen] = 0x00;

		strcpy(((CHSR*)hRet)->aDocInfo[i], szBuf2);
	}

	TCPClose(Sock);

	return 0;
}

/* ******************************************* */
#ifdef _SBA_DLL_
EXPORT TINT __stdcall	SBASrchDocDTR(char *IP, TWORD Port, char *QueryST, SBHANDLE hRet)
#else
TINT	SBASrchDocDTR(char *IP, TWORD Port, char *QueryST, SBHANDLE hRet)
#endif
{
	char	szBuf[SCL_BUF64];
	char	szBuf2[SCL_BUF1024];
	TINT	i, nLen;
	TSOCK	Sock;

	Sock = TCPSocket();
	if ( Sock < 0 )
		return ERT_SOCKET;

	if ( !TCPConnect(Sock, IP, Port) )
		return SBAErrRet(Sock, ERT_CONNECT);

	/* Send OP CODE */
	if ( !TCPSendData(Sock, OP_SR_DTR, 3) )
		return SBAErrRet(Sock, ERT_SEND_DATA);

	/* Recv ACK NAK */
	if ( !TCPRecvData(Sock, szBuf, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);

	if ( TCPCmpOP(szBuf, OP_NAK) )
		return SBAErrRecv(Sock);

	/* Send Data */
	if ( !TCPSendData(Sock, QueryST, strlen(QueryST)) )
		return SBAErrRet(Sock, ERT_SEND_DATA);

	/* Recv ACK NAK */
	if ( !TCPRecvData(Sock, szBuf, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);

	if ( TCPCmpOP(szBuf, OP_NAK) )
		return SBAErrRecv(Sock);

	/* Recv Total List Count From Server */
	if ( !TCPRecvData(Sock, szBuf2, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);
	szBuf2[nLen] = 0x00;
	((CHSR*)hRet)->TotCnt = (TLONG)atol(szBuf2);

	/* Recv List Count From Server */
	if ( !TCPRecvData(Sock, szBuf2, &nLen) )
		return SBAErrRet(Sock, ERT_RECV_DATA);
	szBuf2[nLen] = 0x00;
	((CHSR*)hRet)->RecvCnt = (TLONG)atol(szBuf2);

	/* Recv Each List From Server */
	for ( i = 0; i < ((CHSR*)hRet)->RecvCnt; i++ )
	{
		if ( !TCPRecvData(Sock, szBuf2, &nLen) )
			return SBAErrRet(Sock, ERT_RECV_DATA);
		szBuf2[nLen] = 0x00;

		strcpy(((CHSR*)hRet)->aDocInfo[i], szBuf2);
	}

	TCPClose(Sock);

	return 0;
}

/* ******************************************* */
#ifdef _SBA_DLL_
EXPORT TINT __stdcall	SBAGetWordCnt(SBHANDLE hRet, TLONG *WordCnt)
#else
TINT	SBAGetWordCnt(SBHANDLE hRet, TLONG *WordCnt)
#endif
{
	if ( hRet == 0x00 )
		return ERS_OBJ_NONE;

	*WordCnt = STDgetTotElement(((CHSR*)hRet)->WordList, '^');
	
	return 0;
}

/* ******************************************* */
#ifdef _SBA_DLL_
EXPORT TINT __stdcall	SBAGetWordList(SBHANDLE hRet, char *Buf, TINT BufLen)
#else
TINT	SBAGetWordList(SBHANDLE hRet, char *Buf, TINT BufLen)
#endif
{
	if ( hRet == 0x00 )
		return ERS_OBJ_NONE;

	if ( BufLen < (TINT)strlen(((CHSR*)hRet)->WordList) )
		return ERF_BUF_TOO_SMALL;

	strcpy(Buf, ((CHSR*)hRet)->WordList);

	return 0;
}

/* ******************************************* */
#ifdef _SBA_DLL_
EXPORT TINT	__stdcall SBAGetTotalCnt(SBHANDLE hRet, TLONG *TotCnt)
#else
TINT	SBAGetTotalCnt(SBHANDLE hRet, TLONG *TotCnt)
#endif
{
	if ( hRet == 0x00 )
		return ERS_OBJ_NONE;

	*TotCnt = ((CHSR*)hRet)->TotCnt;

	return 0;
}

#ifdef _NOT_USE_  /* 991103 KJB */
/* ******************************************* */
#ifdef _SBA_DLL_
EXPORT TINT __stdcall SBAGetNSID(SBHANDLE hRet, TLONG *NSID)
#else
TINT  SBAGetNSID(SBHANDLE hRet, TLONG *NSID)
#endif
{
	if ( hRet == 0x00 )
		return ERS_OBJ_NONE;

	*NSID = ((CHSR*)hRet)->NSID;

	return 0;
}
#endif  /* 991103 KJB  */

/* ******************************************* */
#ifdef _SBA_DLL_
EXPORT TINT	__stdcall SBAGetRecvCnt(SBHANDLE hRet, TLONG *RecvCnt)
#else
TINT	SBAGetRecvCnt(SBHANDLE hRet, TLONG *RecvCnt)
#endif
{
	if ( hRet == 0x00 )
		return ERS_OBJ_NONE;

	*RecvCnt = ((CHSR*)hRet)->RecvCnt;

	return 0;
}

/* ******************************************* */
#ifdef _SBA_DLL_
EXPORT TINT	__stdcall SBAGetDocInfo(SBHANDLE hRet, TINT Order, char *Buf, TINT BufLen)
#else
TINT	SBAGetDocInfo(SBHANDLE hRet, TINT Order, char *Buf, TINT BufLen)
#endif
{
	if ( hRet == 0x00 )
		return ERS_OBJ_NONE;

	if ( Order >= ((CHSR*)hRet)->RecvCnt )
		return ERS_OBJ_NONE;

	if ( BufLen < (TINT)strlen(((CHSR*)hRet)->aDocInfo[Order]) )
		return ERF_BUF_TOO_SMALL;

	strcpy(Buf, ((CHSR*)hRet)->aDocInfo[Order]);

	return 0;
}

/*
**  END API_SR.C
*/
