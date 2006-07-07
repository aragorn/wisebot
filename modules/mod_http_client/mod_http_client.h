/* $Id$ */
#ifndef MOD_HTTP_CLIENT_H
#define MOD_HTTP_CLIENT_H

#include "memfile.h"
#include "http_buffering.h" /* http_parsing_t */
#include "http.h"

#ifdef DEBUG_SOFTBOTD
#include <sys/time.h>
#endif


typedef struct { 
	/* connection */ 
	char *host;        //어드레스이거나, 호스트이름. 
	char *port;          //포트 
	int sockFd;        //소켓의 파일 디스크립터 
	int timeout;		// in milli-sec

	long max_size;		// in byte

	/* current job status */  
	char statusFlag;   
	int numRequestInOneConnect;

    int skip;          // 1 : retrieve시에 처리하지 않는다.
	memfile *current_request_buffer;
	memfile *current_recv_buffer; 

	/* http parsing */
	http_parsing_t parsing_status;

	/* HTTP */ 
	http_t *http;
	
#ifdef DEBUG_SOFTBOTD
	struct timeval tv;
	memfile *debug_log;
#endif

} http_client_t;

#endif //_MOD_HTTP_CLIENT_H_
