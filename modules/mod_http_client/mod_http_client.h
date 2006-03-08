/* $Id$ */
#ifndef _MOD_HTTP_CLIENT_H_
#define _MOD_HTTP_CLIENT_H_

#include "softbot.h"
#include "memfile.h"
#include "http.h"
#include "http_client_status.h"

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
