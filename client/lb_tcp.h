/*
**  SOFTBOT v4.5
**  LB_TCP.H
**  2000.04.  BY KJB
*/
#ifndef _LB_TCP_
#define _LB_TCP_

#include "softbot4.h"
#ifdef _UNIX_VER_
#	include <unistd.h>
#	include <sys/types.h>
#	include <sys/time.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <netinet/tcp.h>
#	include <netdb.h>
#	include <arpa/inet.h>
#	include <poll.h>
#	include <fcntl.h>
#else
#	include <windows.h>
#endif

typedef struct sockaddr_in  CSockAddr;

/* *************************************************************** */
#define TCP_TIMEOUT	(10)
#define UDP_TIMEOUT     (1)
#define MAX_SENDSZ	(1024)

#define TAG_END		'E'
#define TAG_CONT	'C'

#define OP_ACK		("ACK")
#define OP_NAK		("NAK")

/* added by woosong for keep alive connection 2001.06.11. */
#define OP_KEEPALIVE ("000")

#define OP_RG		("100")
#define OP_RG_USER	("100")
#define OP_RG_CAT	("101")
#define OP_RG_CATNM	("102")
#define OP_RG_DOC	("103")
#define OP_RG_DOC_M	("113")
#define OP_RG_OID	("104")
#define OP_RG_HIS	("105")

#define OP_IQ		("200")
#define OP_IQ_USER	("200")
#define OP_IQ_UN	("201")
#define OP_IQ_CAT	("202")
#define OP_IQ_CN	("203")
#define OP_IQ_DOC	("204")
#define OP_IQ_OID	("205")

#define OP_UP		("300")
#define OP_UP_USER	("300")
#define OP_UP_CAT	("301")
#define OP_UP_DOC	("302")

#define OP_DL		("400")
#define OP_DL_USER	("400")
#define OP_DL_CAT	("401")
#define OP_DL_DOC	("402")

#define OP_RS		("500")
#define OP_RS_USER	("500")
#define OP_RS_CAT	("501")
#define OP_RS_DOC	("502")

#define OP_KM			("600")
#define OP_KM_LINKCC	("600")
#define OP_KM_UNLINKCC	("601")
#define OP_KM_MOVECC	("602")
#define OP_KM_LINKDC	("603")
#define OP_KM_UNLINKDC	("604")
#define OP_KM_LINKDD	("605")
#define OP_KM_UNLINKDD	("606")

#define OP_SR		("700")
#define OP_SR_DOC	("700")
#define OP_SR_DTC	("701")
#define OP_SR_DTR	("702")
#define _USE_FILTER_DOCS_ //FIXME
#ifdef _USE_FILTER_DOCS_
	#define OP_SR_FILTER	("710")
#endif

#define OP_NV		("800")
#define OP_NV_CAT	("800")
#define OP_NV_CATDOC ("801")
#define OP_NV_CATDOC_PRE	("802")

#define OP_AS			("900")
#define OP_AS_RQ_CATTXT	("900")
#define OP_AS_MK_CATTXT	("901")
#define OP_AS_RQ_ORG	("902")
#define OP_AS_RQ_TXT	("903")
#define OP_AS_RQ_DID	("904")
#define OP_AS_RQ_DAC	("905")

#define OP_RMA_DOC	("191")

#endif

/* ***************************************************** */
void  TCPInit();
void  TCPEnd();
TSOCK TCPSocket();
TBOOL TCPConnect(TSOCK Sock, char *IP, TWORD Port);
TBOOL TCPBind(TSOCK Sock, TWORD wPort);
TBOOL TCPListen(TSOCK Sock, TINT BackLog);
TSOCK TCPAccept(TSOCK LisSock);
void  TCPClose(TSOCK Sock);
TBOOL TCPSendData(TSOCK Sock, void *Data, TINT Len);
TBOOL TCPRecvData(TSOCK Sock, void *pBuf, TINT *pLen);
TBOOL TCPSendFile(TSOCK Sock, FILE *fpFile);
TBOOL TCPRecvFile(TSOCK Sock, FILE *fpFile);
TBOOL TCPSendFileOff(TSOCK Sock, FILE *fpFile, TLONG lSrt, TLONG lSize);
TBOOL TCPRecvFileOff(TSOCK Sock, FILE *fpFile, TLONG *plSrt);
TBOOL TCPCmpOP(char *Src, char *Dest);
TBOOL TCPCmpJob(char *Src, char *Dest);

TSOCK  UDPsocket();
TBOOL  UDPbind(TSOCK Sock, TWORD wPort);
TBOOL  UDPmakeSockAddr(CSockAddr *pSockAddr, char *HostName, TWORD wPort);
TINT  UDPaccept(TSOCK Sock, CSockAddr *pSockAddr, char* pBuf, TINT nBufLen);
TBOOL  UDPsendTo(TSOCK Sock, CSockAddr *pSockAddr, char* pBuf, TINT nBufLen);
TBOOL  UDPrecvFrom(TSOCK Sock, CSockAddr *pSockAddr, char *pBuf, TINT nBufLen);
void  UDPflushRecvBuf(TSOCK Sock, char *pBuf, TINT nBufLen);

/*
** CHead
*/
#ifndef _CHEAD_
#define _CHEAD_
typedef struct tagHead
{
	char chTag;
	char sSize[7];
} CHead; /* 8Byte */
#endif
/*
** Prototype
*/
TBOOL TCPSend(TSOCK Sock, CHead *pHead, void *pData);
TBOOL TCPRecv(TSOCK Sock, CHead *pHead, void *Buf);

/* **
** END LB_TCP.H
** */
