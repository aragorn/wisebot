/* $Id$ */
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>

#include <errno.h>

#include "common_core.h"
#include "memory.h"
#include "mod_api/tcp.h"
#include "mod_api/http_client.h"
#include "http_client_status.h"
#include "http_reserved_words.h" /* HTTP_CONNECTION,HTTP_WORD_CLOSE */
#include "mod_http_client.h"


#define DEFAULT_HTTPCLIENT_TIMEOUT	(5000)
#define DEFAULT_HTTPCLIENT_MAX_SIZE	(15000000)
#define CONST_BUF_SIZE	(16384)

#ifdef DEBUG_SOFTBOTD
/*#define DEBUG_HTTPCLIENT*/
#endif 

#ifdef DEBUG_HTTPCLIENT
#define httpclient_record_event(this, format, ...) \
{	\
	struct timeval now;	\
	gettimeofday(&now, NULL);	\
	if ( this->max_size > 0 && \
			memfile_getSize(this->debug_log) < this->max_size ) {	\
		memfile_appendF(this->debug_log, "diff [%f] sec : ", \
			now.tv_sec - this->tv.tv_sec + 0.000001 * ( now.tv_usec-this->tv.tv_usec) );	\
		memfile_appendF(this->debug_log, format, ##__VA_ARGS__ );	\
		memfile_appendF(this->debug_log, "\n");	\
	}	\
	memcpy(&(this->tv), &now, sizeof(struct timeval));	\
	error(format, ##__VA_ARGS__); 	\
}
#else
#define httpclient_record_event(this, format, ...) 	{}	
#endif

static http_client_t *
http_client_new(const char *host, const char *port){
	http_client_t * httpClt;
	http_t *http;
	memfile *mFile;
	if(!host ||!strlen(host) || !port ||!strlen(port)){
		error("invalid input : host [%s] port [%s]", 
				host ? host:"null" , port ? port:"null");
		return NULL;
	}
	httpClt = (http_client_t *)sb_malloc(sizeof(http_client_t));
	if(!httpClt){
		error("sb_malloc fialed : http_client_t");
		return NULL;
	}
	http = http_new();
	if(!http){
		error("http_new failed");
		sb_free(httpClt);
		return NULL;
	}
	mFile = memfile_new();
	if(!mFile){
		error("memfile_new failed:");
		sb_free(httpClt);
		http_free(http);
		return NULL;
	}

	httpClt->host= sb_strdup((char *)host);
	httpClt->port= sb_strdup((char *)port);
	if(!httpClt->host || !httpClt->port){
		error("sb_strdup failed : host [%s] port [%s]",
			host ? host:"null" , port ? port:"null");
		memfile_free(mFile);
		http_free(http);
		return NULL;
	}

#ifdef DEBUG_HTTPCLIENT
	httpClt->debug_log = memfile_new();
	if ( !httpClt->debug_log ) {
		error("debug_log buffer create failed");
		memfile_free(mFile);
		http_free(http);
		return NULL;
	}
	gettimeofday(&httpClt->tv, NULL);
#endif
		
	httpClt->sockFd = -1;
	httpClt->timeout = DEFAULT_HTTPCLIENT_TIMEOUT;
	httpClt->max_size = DEFAULT_HTTPCLIENT_MAX_SIZE;
	httpClt->statusFlag = CON_FLAG_CLEAN;
	httpClt->numRequestInOneConnect = 0;
	httpClt->current_request_buffer = NULL;
	httpClt->current_recv_buffer = mFile;
	httpClt->http = http;

	httpClt->parsing_status.invalidFlag = 0;
	httpClt->parsing_status.flushed_content_len = 0;
	httpClt->parsing_status.content_buf_offset = 0;
	httpClt->parsing_status.parsed_data_size = 0;
	httpClt->parsing_status.flushed_data_size= 0;
	httpClt->parsing_status.wanted_data_size = 0;
	httpClt->parsing_status.state = INITIAL;
	

	return httpClt;
}

//NOT YET TESTED
static void
http_client_reset(http_client_t *httpClt){
	http_prepareAnotherRequest(httpClt->http);
	http_freeResponse(httpClt->http);

	httpClt->statusFlag = 
		CON_FLAG_CLEAN || (httpClt->statusFlag & CON_FLAG_CONNECTED);
	if (httpClt->current_request_buffer)
		memfile_free(httpClt->current_request_buffer);
	memfile_reset(httpClt->current_recv_buffer);
	httpClt->current_request_buffer = NULL;

	httpClt->parsing_status.invalidFlag = 0;
	httpClt->parsing_status.flushed_content_len = 0;
	httpClt->parsing_status.content_buf_offset = 0;
	httpClt->parsing_status.parsed_data_size = 0;
	httpClt->parsing_status.flushed_data_size= 0;
	httpClt->parsing_status.wanted_data_size = 0;
	httpClt->parsing_status.state = INITIAL;
	
#ifdef DEBUG_HTTPCLIENT
	memfile_reset(httpClt->debug_log);
	gettimeofday(&httpClt->tv, NULL);
#endif
	return;
}


static void
http_client_free(http_client_t *httpClt){
	if(!httpClt)
		return;
	if(httpClt->sockFd != -1) {
		sb_run_tcp_close(httpClt->sockFd);
	}
	if(httpClt->host)
		sb_free(httpClt->host);
	if(httpClt->port)
		sb_free(httpClt->port);
	if(httpClt->current_request_buffer)
		memfile_free(httpClt->current_request_buffer);
	httpClt->statusFlag = CON_FLAG_CLEAN;
	memfile_free(httpClt->current_recv_buffer);
	if(httpClt->http)
		http_free(httpClt->http);
	
#ifdef DEBUG_HTTPCLIENT
	memfile_free(httpClt->debug_log);
#endif
	sb_free(httpClt);
	return;
}


static void
http_client_conn_close(http_client_t *this){
	if(!this ){
		return;
	}
	if(this->sockFd != -1) {
		sb_run_tcp_close(this->sockFd);
	}
	this->sockFd = -1;
	this->statusFlag = CON_FLAG_CLEAN;
	
	httpclient_record_event(this, "conn_close by force");
	return;
}


static int 
http_client_connect(http_client_t *this){
	int still_connected = FALSE;
	
	if(!this ){
		error("null http_client_t");
		return FAIL;
	}

	
#ifdef DEBUG_HTTPCLIENT
	memfile_reset(this->debug_log);
#endif
	
	/* check recv_buffer is empty or socket_connection valid */
	if ( this->statusFlag == CON_FLAG_CONNECTED ){
		char  temp[1];
		int n;
		
		httpclient_record_event(this, "tcp_recv try [%d]bytes : check if still connected", 1);
		n = sb_run_tcp_recv_nonb(this->sockFd, temp, 1, 0);
		httpclient_record_event(this, "tcp_recv return [%d]", n);
		if ( n == -1 && errno == ETIMEDOUT ){
			still_connected = TRUE;
		}
	}
	
	if ( still_connected != TRUE ) {
		httpclient_record_event(this, "tcp_close before connect");
		sb_run_tcp_close(this->sockFd);
		this->numRequestInOneConnect = 0;
		
		this->sockFd = -1;
		this->statusFlag = CON_FLAG_CLEAN;

		httpclient_record_event(this, "tcp_connect [%s:%s], timeout[%d]", 
										this->host, this->port, this->timeout);
		if (sb_run_tcp_connect(&(this->sockFd), 
					this->host, this->port) == SUCCESS){
			this->statusFlag = CON_FLAG_CONNECTED;
//LKSCHANGE handling timeout == 0 
/*		} else if ( errno == ETIMEDOUT ){*/
		} else if ( errno == ETIMEDOUT && this->timeout >= 0 ){
			this->statusFlag = CON_FLAG_CONNECTING;
		} else{
			this->statusFlag |= CON_FLAG_CONNECTFAILED;
			info("httpclient_tcp_connect failed host[%s] port[%s] timeout[%d]"
					": %s",	this->host, this->port, this->timeout, strerror(errno));
			return FAIL;
		}
	}
	memfile_reset(this->current_recv_buffer);
	this->parsing_status.state = INITIAL;

	return SUCCESS;
}

static int 
http_client_makeRequest(http_client_t *this, memfile *request){
	if(!this ){
		error("null http_client_t");
		return FAIL;
	}

	if(request == NULL){
		if(this->current_request_buffer){
			memfile_free(this->current_request_buffer);
		}
		this->current_request_buffer = http_makeRequest(this->http);
		if(!this->current_request_buffer){
			error("http_makeRequest failed");
			return FAIL;
		}
	}else{
		http_freeRequest(this->http);
		if(this->current_request_buffer){
			memfile_free(this->current_request_buffer);
		}
		this->current_request_buffer = request;
	}

	memfile_setOffset(this->current_request_buffer, 0);
	return SUCCESS;
}


static int 
http_client_sendRequest(http_client_t *this){
	char  temp[CONST_BUF_SIZE];
	int	read, sent;
	unsigned long offset;
	memfile *request = this->current_request_buffer;
	if(!this || !request ){
		error("null http_client_t or request not ready");
		return FAIL;
	}
	offset = memfile_getOffset(request);
	
	if( (this->statusFlag & CON_FLAG_CONNECTED) == CON_FLAG_CLEAN ){
		error("more to send but not connected");
		return FAIL;
	}

	while( offset < memfile_getSize(request) ){
		memfile_setOffset(request, offset);
		read = memfile_read(request, temp, CONST_BUF_SIZE);
		
		httpclient_record_event(this, "tcp_send try [%d] bytes", read);
		sent = sb_run_tcp_send_nonb(this->sockFd, temp, read, this->timeout);
		httpclient_record_event(this, "tcp send return [%d]", sent);
		if(sent <= 0 ) {
			this->statusFlag |= CON_FLAG_REQUESTFAILED;
			this->statusFlag &= ~CON_FLAG_REQUEST_OK;
			
			error("httpclient_tcp_send_nonb failed : %s "
					, (sent == 0 ) ? "maybe connection error" : strerror(errno));
			if(sent == 0 || errno != ETIMEDOUT ){
				httpclient_record_event(this, "tcp_close");
				sb_run_tcp_close(this->sockFd);
				this->sockFd = -1;
				this->statusFlag &= ~CON_FLAG_CONNECTED;
			}
			return FAIL;
		}
		offset += sent;
	}
	
	this->statusFlag &= ~CON_FLAG_REQUESTFAILED;
	this->statusFlag |= CON_FLAG_REQUEST_OK;
	this->numRequestInOneConnect++;
	return SUCCESS;
}

static int 
http_client_sendRequestMore(http_client_t *this, char *buf, int buflen, int *sentlen){
	*sentlen = 0;
	
	if(!this){
		error("null http_client_t");
		return FAIL;
	}
	
	if( (this->statusFlag & CON_FLAG_CONNECTED) == CON_FLAG_CLEAN ){
		error("more to send but not connected");
		return FAIL;
	}

	httpclient_record_event(this, "tcp_send try [%d] bytes", buflen);
	*sentlen = sb_run_tcp_send_nonb(this->sockFd, buf, buflen, this->timeout);
	httpclient_record_event(this, "tcp send return [%d]", *sentlen);
	if(*sentlen <= 0 ) {
		this->statusFlag |= CON_FLAG_REQUESTFAILED;
		this->statusFlag &= ~CON_FLAG_REQUEST_OK;

		error("httpclient_tcp_send_nonb failed : %s "
				, (*sentlen == 0 ) ? "maybe connection error" : strerror(errno));
		if(*sentlen == 0 || errno != ETIMEDOUT ){
			httpclient_record_event(this, "tcp_close");
			sb_run_tcp_close(this->sockFd);
			this->sockFd = -1;
			this->statusFlag &= ~CON_FLAG_CONNECTED;
		}
		return FAIL;
	}
	
	this->statusFlag &= ~CON_FLAG_REQUESTFAILED;
	this->statusFlag |= CON_FLAG_REQUEST_OK;
	return SUCCESS;
}

inline static int
http_client_flushBuffer(http_client_t *this){
	long flush_size = this->parsing_status.parsed_data_size 
					- this->parsing_status.flushed_data_size;
	if ( flush_size <= 0 ) {
		return SUCCESS;
	}
	memfile_shift(this->current_recv_buffer, flush_size);
	this->parsing_status.flushed_data_size += flush_size;
	this->parsing_status.flushed_content_len += memfile_getSize(this->http->content_buf);
	memfile_reset(this->http->content_buf);
	this->parsing_status.content_buf_offset = 0;
	return SUCCESS;
}




static int 
http_client_recvResponse(http_client_t *this){
	char temp[CONST_BUF_SIZE];
	int n, total_recv=0;

	if(!this ){
		error("null http_client_t");
		return FAIL;
	}
	
	if( (this->statusFlag & CON_FLAG_CONNECTED) == CON_FLAG_CLEAN ){
		error("more to receive but not connected");
		return FAIL;
	}

	n = CONST_BUF_SIZE;
	memfile_setOffset(this->current_recv_buffer, 
			memfile_getSize(this->current_recv_buffer) );
	
	while(n == CONST_BUF_SIZE){
		if ( this->max_size > 0 &&
				memfile_getSize(this->current_recv_buffer) >= this->max_size ){
			warn("too big content, buffer should be flushed :"
				   " [%ld] >= client_max_size[%ld]",
				memfile_getSize(this->current_recv_buffer), this->max_size);
			break;
		}
		httpclient_record_event(this, "tcp_recv try [%d] bytes", CONST_BUF_SIZE);
		debug("recv timeout[%d]", this->timeout);
		n = sb_run_tcp_recv_nonb(this->sockFd, temp, CONST_BUF_SIZE, this->timeout);
		httpclient_record_event(this, "tcp_recv return [%d]", n);
		if ( n <= 0 ) {
			if ( total_recv > 0 )	break;
			if(n == 0 || errno != ETIMEDOUT ){
				if ( memfile_isEndOfAppend(this->current_recv_buffer) != 1 ) {
					info("connection closed");
					memfile_setEndOfAppend(this->current_recv_buffer);
					if ( this->parsing_status.state != UNTIL_CLOSE ){
						httpclient_record_event(this, 
								"connection_closed while recv[%d][%ld] [%s|%s] : ret[%d]%s "
								, n, memfile_getSize(this->current_recv_buffer)
								, this->http->host, this->http->path, n, strerror(errno));
					}
					httpclient_record_event(this, "tcp_close");
					sb_run_tcp_close(this->sockFd);
					this->sockFd = -1;
					this->statusFlag &= ~CON_FLAG_CONNECTED;
					return SUCCESS;
				}
			}
			this->statusFlag |= CON_FLAG_RESPONSEFAILED;
			this->statusFlag &= ~CON_FLAG_RESPONSE_OK;
			error("more to receive but receive failed in %d msec timeout: %s"
					, this->timeout ,(n==0) ? "connection closed" : strerror(errno));
					
			return FAIL;
		}
		memfile_setOffset(this->current_recv_buffer, 
				memfile_getSize(this->current_recv_buffer));
		if ( 0 > memfile_append(this->current_recv_buffer, temp, n) ){
			httpclient_record_event(this, "tcp_close");
			sb_run_tcp_close(this->sockFd);
			this->sockFd = -1;
			this->statusFlag |= CON_FLAG_RESPONSEFAILED;
			this->statusFlag &= ~CON_FLAG_RESPONSE_OK;
			this->statusFlag &= ~CON_FLAG_CONNECTED;
			error("memfile_append failed: buffer");
			return FAIL;
		}
		
		total_recv += n;
	}

	this->statusFlag &= ~CON_FLAG_RESPONSEFAILED;
	this->statusFlag |= CON_FLAG_RESPONSE_OK;
	
	return SUCCESS;
}



static int 
http_client_parseResponse(http_client_t *this){
	int nRet;
	http_buffering_t buffering;
	char req_method[5];
	unsigned long offset;

	if(!this ){
		error("null http_client_t");
		return FAIL;
	}

	// for checking whether req_method is HEAD or not.
  	// 4 is strlen("HEAD")
	offset = memfile_getOffset(this->current_request_buffer);
	memfile_setOffset(this->current_request_buffer, 0 );
	memfile_read(this->current_request_buffer, req_method, 4);
	memfile_setOffset(this->current_request_buffer, offset );
	req_method[4]='\0';
		
	buffering.parsing_status = &(this->parsing_status);
	buffering.recvbuffer = this->current_recv_buffer;

	
	nRet = http_getResponse(this->http, req_method, &buffering);


	if ( nRet >= 0 ) {
		char *connection_header;
		connection_header =  http_getFromResponseHeader(this->http, HTTP_CONNECTION);
		if (connection_header && 
			strstr(connection_header, HTTP_WORD_CLOSE ) ){
			httpclient_record_event(this, "tcp_close");
			sb_run_tcp_close(this->sockFd);
			this->sockFd = -1;
			this->statusFlag &= ~CON_FLAG_CONNECTED;
		}
		return SUCCESS;
	}

	if ( this->parsing_status.invalidFlag == 1){
		error("http_getResponse Failed");
		httpclient_record_event(this, "tcp_close");
		sb_run_tcp_close(this->sockFd);
		this->sockFd = -1;
		this->statusFlag &= ~CON_FLAG_RESPONSE_OK;
		this->statusFlag |= CON_FLAG_RESPONSEFAILED;
		this->statusFlag &= ~CON_FLAG_CONNECTED;
	}
	return FAIL;
}

static int
http_client_retrieve(int num_of_clients, http_client_t **client_list){
	int i, nRet, ok= SUCCESS ;
	fd_set rset, wset;
	int maxFd;
	struct timeval t;

	for (i = 0; i < num_of_clients; i++) {
		//FIXME
		//do not retrieve clients that have no request
		if ( client_list[i]->http->req_content_len <= 0 || !client_list[i]->current_request_buffer )	continue;
		if ( http_client_connect(client_list[i]) == FAIL ) {
			error("http_client_connect to %s:%s failed",
					client_list[i]->host, client_list[i]->port);
			ok = FAIL;
		}
	}

	while(1) {
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		maxFd = -1;

		for (i = 0; i < num_of_clients; i++) {
			if ( client_list[i]->http->req_content_len <= 0 || !client_list[i]->current_request_buffer )	continue;
			/* if parsing complete, skip */
			if (client_list[i]->parsing_status.state == 
					PARSING_COMPLETE) {
				continue;
			}

			if (client_list[i]->statusFlag & CON_FLAG_CONNECTED) {
				if ( client_list[i]->statusFlag & CON_FLAG_REQUEST_OK )
				{
					FD_SET(client_list[i]->sockFd, &rset);
				} else {
					FD_SET(client_list[i]->sockFd, &wset);
				}

				if (maxFd < client_list[i]->sockFd) {
					maxFd = client_list[i]->sockFd;
				}
			}

			if (client_list[i]->statusFlag & CON_FLAG_CONNECTING) {
				FD_SET(client_list[i]->sockFd, &wset);
				if (maxFd < client_list[i]->sockFd) {
					maxFd = client_list[i]->sockFd;
				}
			}
		}

		t.tv_sec = 1;
		t.tv_usec = 0;


		/* there is no socket to read or write */
		if ( maxFd == -1 ) {
			break;
		}

		if ( select(maxFd + 1, &rset, &wset, NULL, &t) < 0 ) {
			error("select : %s", strerror(errno));
			continue;
		}

		for (i = 0;i < num_of_clients; i++) {
			if ( client_list[i]->http->req_content_len <= 0 || !client_list[i]->current_request_buffer )	continue;
			/* set connected */
			if ( (client_list[i]->statusFlag & CON_FLAG_CONNECTING) &&
				   	(FD_ISSET(client_list[i]->sockFd, &wset)) ) {
				client_list[i]->statusFlag = CON_FLAG_CONNECTED;
			}

			/* send request */
			debug("send request client[%d]", i);
			if ( (client_list[i]->statusFlag & CON_FLAG_CONNECTED) &&
					(FD_ISSET(client_list[i]->sockFd, &wset)) ) {
				if ( http_client_sendRequest(client_list[i]) != SUCCESS ) {
					error("send request failed at client[%d]", i);
					ok = FAIL;
				}
			}

			/* recv response */
			debug("recv response client[%d], sockFd[%d]", i, client_list[i]->sockFd);
			if ( (client_list[i]->statusFlag & CON_FLAG_CONNECTED) &&
					(FD_ISSET(client_list[i]->sockFd, &rset)) ) {
				nRet = http_client_recvResponse(client_list[i]);
				if ( nRet == SUCCESS ) {
					nRet = http_client_parseResponse(client_list[i]);
					if ( (nRet == SUCCESS) &&
							(client_list[i]->parsing_status.state != PARSING_COMPLETE) ) {
						error("parse response error at client[%d]", i);
						ok = FAIL;
						break;
					}
				}else {
					error("receive response failed at client[%d]", i);
					ok = FAIL;
				}
			}
		}
	}

	return ok;
}

/*
static int
http_client_run_memfile(http_client_t *this, memfile *request){
	
	if(!this || !request ){
		error("null http_client_t or request");
		return FAIL;
	}

	if ( SUCCESS != http_client_connect(this) ){
		error("http_client_connect failed ");
		return FAIL;
	}
	
	if ( SUCCESS != http_client_makeRequest(this, request) ){
		error("http_client_makeRequest failed ");
		return FAIL;
	}
	if ( SUCCESS != http_client_sendRequest(this) ){
		error("http_client_sendRequest failed ");
		return FAIL;
	}


	while(this->parsing_status.state != PARSING_COMPLETE){
		if ( SUCCESS != http_client_recvResponse(this)){
			error("http_client_recvResponse failed ");
			return FAIL;
		}

		if ( SUCCESS == http_client_parseResponse(this ) ){
			break;
		}
		
	}
	return SUCCESS;
}

static int
http_client_run(http_client_t *this){
	if(!this){
		error("null http_client_t");
		return FAIL;
	}

	if(this->current_request_buffer){
		memfile_free(this->current_request_buffer);
	}

	this->current_request_buffer = http_makeFullRequest(this->http);
	if (!this->current_request_buffer){
		error("http_makeFullRequest failed");
		return FAIL;
	}

	return http_client_run_memfile(this, this->current_request_buffer);
}

static int
http_client_run_custom_request(http_client_t *this, memfile *req){
	if(!this){
		error("null http_client_t");
		return FAIL;
	}
	http_freeRequest(this->http);
	return http_client_run_memfile(this, req);
}
*/
static void register_hooks(void)
{
	/* http_client */
	sb_hook_http_client_new(http_client_new, NULL, NULL, HOOK_MIDDLE);
	sb_hook_http_client_free(http_client_free, NULL, NULL, HOOK_MIDDLE);
	sb_hook_http_client_reset(http_client_reset, NULL, NULL, HOOK_MIDDLE);
	sb_hook_http_client_conn_close(http_client_conn_close, NULL, NULL, HOOK_MIDDLE);
	sb_hook_http_client_makeRequest(http_client_makeRequest, NULL, NULL, HOOK_MIDDLE);
	sb_hook_http_client_retrieve(http_client_retrieve, NULL, NULL, HOOK_MIDDLE);
	sb_hook_http_client_connect(http_client_connect, NULL, NULL, HOOK_MIDDLE);
	sb_hook_http_client_sendRequest(http_client_sendRequest, NULL, NULL, HOOK_MIDDLE);
	sb_hook_http_client_sendRequestMore(http_client_sendRequestMore, NULL, NULL, HOOK_MIDDLE);
	sb_hook_http_client_recvResponse(http_client_recvResponse, NULL, NULL, HOOK_MIDDLE);
	sb_hook_http_client_parseResponse(http_client_parseResponse, NULL, NULL, HOOK_MIDDLE);
	sb_hook_http_client_flushBuffer(http_client_flushBuffer, NULL, NULL, HOOK_MIDDLE);
}

module http_client_module = {
    STANDARD_MODULE_STUFF,
    NULL,                   /* config */
    NULL,                   /* registry */
    NULL,                   /* module initialize */
    NULL,                   /* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};

