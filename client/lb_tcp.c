/* 
**  SOFTBOT v4.5
**  LB_TCP.C
**  2000. 05. By Jae-Bum, Kim.
*/
#include <stdio.h>
#include <stdlib.h>
#include "lb_tcp.h"


/*
** CHead
*/
/*
typedef struct tagHead
{
	char chTag;
	char sSize[7];
} CHead;*/ /* 8Byte */
/*
** Prototype
*/
/*
TBOOL TCPSend(TSOCK Sock, CHead *pHead, void *pData);
TBOOL TCPRecv(TSOCK Sock, CHead *pHead, void *Buf);
*/

/* ******************************************* */
void  TCPInit()
{
#ifdef _WIN_VER_
	TWORD	WsaVer;
	WSADATA	WsaData;

	WsaVer = MAKEWORD(1,1);
	if ( WSAStartup(WsaVer, &WsaData) != 0 )
		return;
	
	if ( (LOBYTE(WsaData.wVersion) != LOBYTE(WsaVer)) ||
	     (HIBYTE(WsaData.wVersion) != HIBYTE(WsaVer)))
	{
	 	WSACleanup();
	 	return;
	}
#endif
}

/* ******************************************* */
void  TCPEnd()
{
#ifdef _WIN_VER_
	WSACleanup();
#endif
}

/* ******************************************* */
TSOCK TCPSocket()
{
	TSOCK Sock;

	Sock = socket(AF_INET, SOCK_STREAM, 0);

	return Sock;
}

/* ******************************************* */
TBOOL TCPConnect(TSOCK Sock, char *HostName, TWORD Port)
{
	int bFlag;
	char* pStr = HostName;
	struct sockaddr_in DestAddr;
	
	memset(&DestAddr, 0x00, sizeof(DestAddr) );

	DestAddr.sin_family = AF_INET;
	/*DestAddr.sin_addr.s_addr = inet_addr(IP);*/
	DestAddr.sin_port = htons(Port);

	/* Convert Name to Address */ 
	while (*pStr) {
		if (!isdigit((int) *pStr) && *pStr != '.')
			break;
		pStr++;
	}
	if (!*pStr) {
		DestAddr.sin_addr.s_addr = inet_addr(HostName);
	} else {
		struct hostent* HostElement;			/* netdb.h */
		
		HostElement = gethostbyname(HostName);
		if (!HostElement) {
			return 0;
		}
		memcpy((void *) &DestAddr.sin_addr, *HostElement->h_addr_list, HostElement->h_length);
	}

	if ( connect(Sock, 
		(struct sockaddr*)&DestAddr, sizeof(DestAddr)) < 0 )
		return 0;
	
	bFlag = 1;
	setsockopt(Sock, IPPROTO_TCP, TCP_NODELAY, (void*)&bFlag, sizeof(bFlag));

	return 1;
}

/* ******************************************* */
TBOOL TCPBind(TSOCK Sock, TWORD wPort)
{
	TINT bFlag;
	TINT nFlagLen;
	struct sockaddr_in SvrAddr;

	memset(&SvrAddr, 0x00, sizeof(SvrAddr));

	SvrAddr.sin_family = AF_INET;
	SvrAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	SvrAddr.sin_port = htons(wPort);

	bFlag = 1;
    nFlagLen = sizeof(bFlag);
    setsockopt(Sock, SOL_SOCKET, 
		SO_REUSEADDR, (void*)&bFlag, nFlagLen);
	if ( bind(Sock,
		(struct sockaddr *)&SvrAddr, sizeof(SvrAddr)) != 0 )
		return 0;

	return 1;
}

/* ******************************************* */
TBOOL TCPListen(TSOCK Sock, TINT BackLog)
{
	if ( listen(Sock, BackLog) != 0 )
		return 0;

	return 1;
}

/* ******************************************* */
TSOCK TCPAccept(TSOCK LisSock)
{
	TDWORD nRet;
	TINT nLen, bFlag;
	static struct sockaddr_in CliAddr;
	TSOCK AcpSock;

	nLen = sizeof(CliAddr);
	AcpSock = accept(LisSock, (struct sockaddr*)&CliAddr, (TINT*)&nLen);
#ifdef _WIN_VER_
	if ( AcpSock == INVALID_SOCKET )
	{
		nRet = GetLastError();
		printf("ER ACCEPT(%d)(Sock : %d)\n", nRet, AcpSock);
		return 0;
	}
#endif

	bFlag = 1;
	setsockopt(AcpSock, IPPROTO_TCP, TCP_NODELAY, (void*)&bFlag, sizeof(bFlag));
	return AcpSock;
}

/* ******************************************* */
void TCPClose(TSOCK Sock)
{
#ifdef _UNIX_VER_
	close(Sock);
#else
	closesocket(Sock);
#endif
}

/* ******************************************* */
TBOOL TCPSend(TSOCK Sock, CHead *pHead, void *pData)
{
	TINT   nLeft, nSend;
	char   *pCh;
#ifdef _LINUX_
	fd_set fd;
#else
	struct fd_set fd;
#endif
	struct timeval timeout;

	timeout.tv_sec = TCP_TIMEOUT;
	timeout.tv_usec = 0;

	FD_ZERO(&fd);
	FD_SET(Sock, &fd);

	/* Send Head */
	pCh = (char*)pHead;
	nLeft = sizeof(CHead);

	while ( nLeft > 0 )
	{
		if ( select(Sock + 1, NULL, &fd, NULL, &timeout) == 0 )
			return 0;

		if ( !FD_ISSET(Sock, &fd) )
			return 0;

		nSend = send(Sock, pCh, nLeft, 0);
		if ( nSend <= 0 )
			return 0;

		nLeft -= nSend;
		pCh += nSend;
	}

	/* Send Data */
	pCh = (char*)pData;
	nLeft = atoi(pHead->sSize);
	
	while( nLeft > 0 )
	{
		if ( select(Sock + 1, NULL, &fd, NULL, &timeout) == 0 )
			return 0;

		if ( !FD_ISSET(Sock, &fd) )
			return 0;

		nSend = send(Sock, pCh, nLeft, 0);
		if ( nSend <= 0 )
			return 0;

		nLeft -= nSend;
		pCh += nSend;
	}

	return 1;
}

/* ******************************************* */
TBOOL TCPRecv(TSOCK Sock, CHead *pHead, void *Buf)
{
	TINT   nLeft, nRecv;
	char   *pCh;
#ifdef _LINUX_
	fd_set fd;
#else
	struct fd_set fd;
#endif
	struct timeval timeout;

	timeout.tv_sec = TCP_TIMEOUT;
	timeout.tv_usec = 0;
	FD_ZERO(&fd);
	FD_SET(Sock, &fd);

	/* Recv Head */
	pCh = (char*)pHead;
	nLeft = sizeof(CHead);

	while ( nLeft > 0 )
	{
		if (select(Sock + 1, &fd, NULL, NULL, &timeout) == 0 )
			return 0;
		if ( !FD_ISSET(Sock, &fd) )
			return 0;

		nRecv = recv(Sock, pCh, nLeft, 0);
		if ( nRecv <= 0 )
			return 0;

		nLeft -= nRecv;
		pCh += nRecv;
	}

	/* Recv Data */
	pCh = (char*)Buf;
	nLeft = atoi(pHead->sSize);

	while ( nLeft > 0 )
	{
		if (select(Sock + 1, &fd, NULL, NULL, &timeout) == 0 )
			return 0;
		if ( !FD_ISSET(Sock, &fd) )
			return 0;

		nRecv = recv(Sock, pCh, nLeft, 0);
		if ( nRecv <= 0 )
			return 0;

		nLeft -= nRecv;
		pCh += nRecv;
	}

	return 1;
}

/* ******************************************* */
TBOOL TCPSendData(TSOCK Sock, void *Data, TINT Len)
{
	CHead Head;

	if ( Len > MAX_SENDSZ)
		return 0;

	Head.chTag = TAG_END;
	sprintf(Head.sSize, "%d", Len);

	if ( !TCPSend(Sock, &Head, Data) )
		return 0;

	return 1;
}

/* ******************************************* */
TBOOL TCPRecvData(TSOCK Sock, void *pBuf, TINT *pLen)
{
	CHead Head;

	if ( !TCPRecv(Sock, &Head, pBuf) )
		return 0;

	*pLen = atoi(Head.sSize);
	
	return 1;
}

/* ******************************************* */
TBOOL TCPSendFile(TSOCK Sock, FILE *fpFile)
{
	TLONG i, lSize, lLoopCnt, lLeft;
	char  szBuf[MAX_SENDSZ + 1];
	CHead Head;

	fseek(fpFile, 0L, SEEK_END);
	lSize = (TLONG)ftell(fpFile);
	fseek(fpFile, 0L, SEEK_SET);

	lLoopCnt = (TLONG)(lSize / (TLONG)MAX_SENDSZ);
	lLeft = (TLONG)(lSize % MAX_SENDSZ);
	if ( (lLeft == 0) && (lSize > 0) )
	{
		lLoopCnt--;
		lLeft = (TLONG)MAX_SENDSZ;
	}

	Head.chTag = TAG_CONT;
	sprintf(Head.sSize, "%d", MAX_SENDSZ);
	for ( i = 0; i < lLoopCnt; i++ )
	{
		if ( fread(szBuf, MAX_SENDSZ, 1, fpFile) < 1 )
			return 0;

		if ( !TCPSend(Sock, &Head, szBuf) )
			return 0;
	}

	Head.chTag = TAG_END;
	sprintf(Head.sSize, "%d", lLeft);
	if ( fread(szBuf, lLeft, 1, fpFile) < 1 )
		return 0;

	if ( !TCPSend(Sock, &Head, szBuf) )
		return 0;

	return 1;
}

/* ******************************************* */
TBOOL TCPRecvFile(TSOCK Sock, FILE *fpFile)
{
	char  szBuf[MAX_SENDSZ + 1];
	TLONG lSize;
	CHead Head;

	fseek(fpFile, 0L, SEEK_SET);

	do
	{
		if ( !TCPRecv(Sock, &Head, szBuf) )
			return 0;

		lSize = (TLONG)atoi(Head.sSize);
		if ( fwrite(szBuf, lSize, 1, fpFile) < 1 )
			return 0;

	} while(Head.chTag == TAG_CONT);

	return 1;
}

/* ******************************************* */
TBOOL TCPSendFileOff(TSOCK Sock, FILE *fpFile, TLONG lSrt, TLONG lSize)
{
	TLONG i, lLoopCnt, lLeft;
	char  szBuf[MAX_SENDSZ + 1];
	CHead Head;

	fseek(fpFile, lSrt, SEEK_SET);

	lLoopCnt = lSize / MAX_SENDSZ;
	lLeft = lSize % MAX_SENDSZ;
	if ( (lLeft == 0) && (lSize > 0) )
	{
		lLoopCnt--;
		lLeft = (TLONG)MAX_SENDSZ;
	}

	Head.chTag = TAG_CONT;
	sprintf(Head.sSize, "%d", MAX_SENDSZ);

	for ( i = 0; i < lLoopCnt; i++ )
	{
		if ( fread(szBuf, MAX_SENDSZ, 1, fpFile) < 1 )
			return 0;

		if ( !TCPSend(Sock, &Head, szBuf) )
			return 0;

	}

	Head.chTag = TAG_END;
	sprintf(Head.sSize, "%d", lLeft);

	if ( fread(szBuf, lLeft, 1, fpFile) < 1 )
		return 0;

	if ( !TCPSend(Sock, &Head, szBuf) )
		return 0;

	return 1;
}

/* ******************************************* */
TBOOL TCPRecvFileOff(TSOCK Sock, FILE *fpFile, TLONG *plSrt)
{
	char  szBuf[MAX_SENDSZ + 1];
	TLONG lSize, lEnd;
	CHead Head;

	fseek(fpFile, 0L, SEEK_END);
	*plSrt = (TLONG)ftell(fpFile);
	lSize = 0;
	if ( fwrite(&lSize, 4, 1, fpFile) < 1 )
		return 0;

	do
	{
		if ( !TCPRecv(Sock, &Head, szBuf) )
			return 0;

		lSize = atoi(Head.sSize);
		if ( fwrite(szBuf, lSize, 1, fpFile) < 1 )
			return 0;
	} while(Head.chTag == TAG_CONT);

	lEnd = (TLONG)ftell(fpFile);
	lSize = lEnd - *plSrt + 1;

	fseek(fpFile, *plSrt, SEEK_SET);
	if ( fwrite(&lSize, 4, 1, fpFile) < 1 )
		return 0;

	return 1;
}

/* ******************************************* */
TBOOL TCPCmpOP(char *Src, char *Dest)
{
	if ( Src[0] == Dest[0] &&
		Src[1] == Dest[1] &&
		Src[2] == Dest[2] )
		return 1;

	return 0;
}

/* ******************************************* */
TBOOL	TCPCmpJob(char *Src, char *Dest)
{
	if ( Src[0] == Dest[0] )
		return 1;

	return 0;
}

/* *************************************************************** */
TSOCK  UDPsocket()
{
	TSOCK Sock;

	Sock = socket(AF_INET, SOCK_DGRAM, 0);

	return Sock;
}

/* *************************************************************** */
TBOOL  UDPbind(TSOCK Sock, TWORD wPort)
{
	TINT bFlag;
	TINT nFlagLen;
	struct sockaddr_in SvrAddr;

	memset(&SvrAddr, 0x00, sizeof(SvrAddr));

	SvrAddr.sin_family = AF_INET;
	SvrAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	SvrAddr.sin_port = htons(wPort);

	bFlag = 1;
    nFlagLen = sizeof(bFlag);
    setsockopt(Sock, SOL_SOCKET, 
		SO_REUSEADDR, (void*)&bFlag, nFlagLen);
	if ( bind(Sock,
		(struct sockaddr *)&SvrAddr, sizeof(SvrAddr)) != 0 )
		return 0;

	return 1;
}

/* *************************************************************** */
TBOOL  UDPmakeSockAddr(CSockAddr *pSockAddr, char *HostName, TWORD wPort)
{
	char* pStr = HostName;

	
	memset(pSockAddr, 0x00, sizeof(CSockAddr) );

	pSockAddr->sin_family = AF_INET;
	pSockAddr->sin_port = htons(wPort);

	/* Convert Name to Address */ 
	while (*pStr) {
		if (!isdigit((int) *pStr) && *pStr != '.')
			break;
		pStr++;
	}
	if (!*pStr) {
		pSockAddr->sin_addr.s_addr = inet_addr(HostName);
	} else {
		struct hostent* HostElement;			/* netdb.h */
		
		HostElement = gethostbyname(HostName);

		if (!HostElement) {
			return 0;
		}
		memcpy((void *) &(pSockAddr->sin_addr), *HostElement->h_addr_list, HostElement->h_length);
	}

//		printf("HostName : %s \n", HostName);
	return 1;
}

/* *************************************************************** */
TINT  UDPaccept(TSOCK Sock, CSockAddr *pSockAddr, char* pBuf, TINT nBufLen)
{
	TINT  nRet, nAddrLen;
	
	nAddrLen = sizeof(CSockAddr);
	nRet = recvfrom(Sock, pBuf, nBufLen, 0, (struct sockaddr*)pSockAddr, &nAddrLen);
	if ( nRet < nBufLen )
		pBuf[nRet] = 0x00;

	return nRet;
}

/* *************************************************************** */
TBOOL  UDPsendTo(TSOCK Sock, CSockAddr *pSockAddr, char* pBuf, TINT nBufLen)
{
	TINT   nAddrLen, nSend;
#ifdef _LINUX_
	fd_set fd;
#else
	struct fd_set fd;
#endif
	struct timeval timeout;

	timeout.tv_sec = UDP_TIMEOUT;
	timeout.tv_usec = 0;

	FD_ZERO(&fd);
	FD_SET(Sock, &fd);

	if ( select(Sock + 1, NULL, &fd, NULL, &timeout) == 0 )
		return 0;

	if ( !FD_ISSET(Sock, &fd) )
		return 0;

	nAddrLen = sizeof(CSockAddr);
	nSend = sendto(Sock, pBuf, nBufLen, 0, (struct sockaddr*)pSockAddr, nAddrLen);
	if ( nSend <= 0 )
		return 0;

	return 1;
}

/* *************************************************************** */
TBOOL  UDPrecvFrom(TSOCK Sock, CSockAddr *pSockAddr, char *pBuf, TINT nBufLen)
{
	TINT   nAddrLen, nRecv;
#ifdef _LINUX_
	fd_set fd;
#else
	struct fd_set fd;
#endif
	struct timeval timeout;

	timeout.tv_sec = UDP_TIMEOUT;
	timeout.tv_usec = 0;
	FD_ZERO(&fd);
	FD_SET(Sock, &fd);

	if (select(Sock + 1, &fd, NULL, NULL, &timeout) == 0 )
		return 0;
	if ( !FD_ISSET(Sock, &fd) )
		return 0;

	nAddrLen = sizeof(CSockAddr);
	nRecv = recvfrom(Sock, pBuf, nBufLen, 0, (struct sockaddr*)pSockAddr, &nAddrLen);
	if ( nRecv <= 0 )
		return 0;

	if ( nRecv < nBufLen )
		pBuf[nRecv] = 0x00;

	return 1;
}

/* *************************************************************** */
void  UDPflushRecvBuf(TSOCK Sock, char *pBuf, TINT nBufLen)
{
	TINT   nAddrLen, nRecv;

	nAddrLen = sizeof(CSockAddr);
	for ( ; ; )
	{
#ifdef AIX5
		nRecv = recvfrom(Sock, pBuf, nBufLen, MSG_PEEK|MSG_NONBLOCK, 0x00, 0x00);
#else
		nRecv = recvfrom(Sock, pBuf, nBufLen, MSG_PEEK|MSG_DONTWAIT, 0x00, 0x00);
#endif
		if ( nRecv <= 0 )
			break;

		nRecv = recvfrom(Sock, pBuf, nBufLen, 0, 0x00, 0x00);
		if ( nRecv <= 0 )
			break;
	}

	return;
}

/* **
** END LB_TCP.C
** */
