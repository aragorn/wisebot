/*
**  SOFTBOT v4.0
**  API_LIB.C
**  1999.10. By Jae-Bum, Kim.
*/
#include	<stdio.h>
#include	<stdlib.h>
#include	"api_lib.h"
#include	"lb_std.h"
#include	"lb_tcp.h"

/* ******************************************* */
TINT  SBAErrRet(TSOCK Sock, TINT nErr)
{
	if ( Sock > 0 )
		TCPClose(Sock);

	return nErr;
}

/* ******************************************* */
TINT  SBAErrRecv(TSOCK Sock)
{
	char  szBuf[CONST_BUF];
	TINT  nLen;

	if ( Sock > 0 )
	{
		if ( !TCPRecvData(Sock, szBuf, &nLen) )
		{
			TCPClose(Sock);
			return ERT_RECV_DATA;
		}
		szBuf[nLen] = 0x00;
		nLen = atoi(szBuf);

		TCPClose(Sock);
	}
	
	return nLen;
}


/*
**  END API_LIB.C
*/
