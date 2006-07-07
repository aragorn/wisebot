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
	char *host;        //��巹���̰ų�, ȣ��Ʈ�̸�. 
	char *port;          //��Ʈ 
	int sockFd;        //������ ���� ��ũ���� 
	int timeout;		// in milli-sec

	long max_size;		// in byte

	/* current job status */  
	char statusFlag;   
	int numRequestInOneConnect;

    int skip;          // 1 : retrieve�ÿ� ó������ �ʴ´�.
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
