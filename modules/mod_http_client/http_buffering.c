/* $Id$ */
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>

#include "common_core.h"
#include "memory.h"
#include "http_buffering.h"

#define CONST_BUF_SIZE 	(512)

//----------------------------------------------------------
// Receiving until a pattern appears
//----------------------------------------------------------

static int
_http_buffering_receiveUntil(http_buffering_t *handle, const char *pattern){ 
/*	int sockfd = handle->sockfd;*/
/*	char *status_flag = handle->statusFlag;*/
	memfile *buffer = handle->recvbuffer;
	int len;
	char temp[CONST_BUF_SIZE];
	char *pattern_ptr;
	char *afterNullCh;
	unsigned long initOffset = memfile_getOffset(buffer);
	unsigned long lastOffset = initOffset;
	unsigned int  pattern_length;

	if ( pattern == NULL || (pattern_length = strlen(pattern)) <= 0 
			|| pattern_length >= CONST_BUF_SIZE ){
		error("invalid pattern : [%s]", pattern ? pattern : "null");
		return -1;
	}

	while(1){
		memfile_setOffset(buffer, lastOffset);
		len = memfile_read(buffer, temp, (CONST_BUF_SIZE-1));
		if( len < pattern_length ){
			DEBUG("more to receive but not enough buffer ");
			memfile_setOffset(buffer, initOffset);
			return -1;
			
			/*
			// getting more from socket
			if( (*status_flag & CON_FLAG_CONNECTED) == CON_FLAG_CLEAN ){
				error("more to receive but not connected");
				return -1;
			}else{
				int n;
				n = httpclient_tcp_recv(sockfd, temp, CONST_BUF_SIZE, handle->timeout);
				if ( n <= 0 ) {
					httpclient_tcp_close(sockfd);
					*status_flag |= CON_FLAG_RESPONSEFAILED;
					*status_flag &= ~CON_FLAG_CONNECTED;
					error("more to receive but receive failed in %d msec timeout: %s"
							,handle->timeout ,(n==0) ? "connection closed" : strerror(errno));
					return -1;
				}
							
				if ( 0 > memfile_append(buffer, temp, n) ){
					httpclient_tcp_close(sockfd);
					*status_flag |= CON_FLAG_RESPONSEFAILED;
					*status_flag &= ~CON_FLAG_CONNECTED;
					error("memfile_append : buffer");
					return -1;
				}
				
				continue;
			}
			*/
		}
		temp[len] = '\0';
		afterNullCh = temp;
		while( len - (afterNullCh-temp) > 0 ){
			char *temp_nullPos;
			pattern_ptr = strstr(afterNullCh, pattern);
			
			if(pattern_ptr){
				memfile_setOffset(buffer, initOffset);
				return pattern_ptr - temp + pattern_length + lastOffset - initOffset;
			}
			temp_nullPos= memchr(afterNullCh, '\0', temp + len - afterNullCh);
			if(temp_nullPos == NULL){
				break;
			}else{
				afterNullCh = temp_nullPos + 1;
			}
		}
			
		lastOffset += len - pattern_length +1 ;
	}
	crit("can't reach here : logic has flaw ");
	return -1;
}

//----------------------------------------------------------
//return: pointer to beginning of the pattern.
char *
http_buffering_getUntil(http_buffering_t *handle, char **buf, char *pattern){
	memfile *buffer = handle->recvbuffer;
/*	unsigned long initOffset = memfile_getOffset(buffer);*/
	unsigned long initOffset = 
		handle->parsing_status->parsed_data_size 
		- handle->parsing_status->flushed_data_size;
	long size = 0;
	char *pattern_ptr;

	
	memfile_setOffset(buffer, initOffset);
	size = _http_buffering_receiveUntil(handle, pattern);

   	if ( size < 0 ){
		return NULL;
	}

	*buf = (char * )sb_malloc(size);
	if(!*buf){
		error("sb_malloc failed : buf");
		return NULL;
	}
	memfile_setOffset(buffer, initOffset);
	memfile_read(buffer, *buf, size);
	pattern_ptr= *buf + size - strlen(pattern);
	if( (pattern_ptr = strstr(pattern_ptr, pattern)) == NULL ){
		crit("error that can not occur.. there is problem in logic");
		sb_free(*buf);
		return NULL;
	}
	handle->parsing_status->parsed_data_size = 
		handle->parsing_status->flushed_data_size + memfile_getOffset(buffer);
	return pattern_ptr;
}
//----------------------------------------------------------
int
http_buffering_fill_memfile_Until(http_buffering_t *handle, memfile *buf, char *pattern){
	memfile *buffer = handle->recvbuffer;
/*	unsigned long initOffset = memfile_getOffset(buffer);*/
	unsigned long initOffset = 
		handle->parsing_status->parsed_data_size
		-handle->parsing_status->flushed_data_size;
	long size = 0;

	memfile_setOffset(buffer, initOffset);
	
	size = _http_buffering_receiveUntil(handle, pattern);
   	if ( size < 0 ){
		return size;
	}

	memfile_setOffset(buffer, initOffset);
	if ( memfile_read2memfile(buffer, buf, size) != size){
		error("memfile_read2memfile : buf size[%ld]", size);
		return -1;
	}
	handle->parsing_status->parsed_data_size = 
		handle->parsing_status->flushed_data_size + memfile_getOffset(buffer);

	return size;
}
//----------------------------------------------------------




//----------------------------------------------------------
//	Receiving N Bytes
//----------------------------------------------------------
static int
_http_buffering_receiveNByte(http_buffering_t *handle, int size){
/*	int sockfd = handle->sockfd;*/
/*	char *status_flag = handle->statusFlag;*/
	memfile *buffer = handle->recvbuffer;

/*	char temp[CONST_BUF_SIZE];*/
/*	int sizeToRecv=0;*/
	int recved=0;
	unsigned long initOffset = memfile_getOffset(buffer);

	if ( size < 0 ){
		error("invalid size : [%d]", size);
		return -1;
	}
	if ( size == 0 ){
		return 0;
	}
	recved = memfile_getSize(buffer) - initOffset;
	if( recved >= size ){
		recved = size;
	}

	if( size > recved ){
		DEBUG("more to receive but not enough buffer");
		return recved;
	}

	/*
	while( size > recved ){
		int n;
		sizeToRecv = ( size-recved > CONST_BUF_SIZE ) ?
						CONST_BUF_SIZE : size-recved;
		if( (*status_flag & CON_FLAG_CONNECTED) == CON_FLAG_CLEAN ){
			error("more to receive but not connected");
			break;
		}
		n = httpclient_tcp_recv(sockfd, temp, sizeToRecv, handle->timeout);
		if ( n > 0 ){
			recved += n;
			if ( 0 > memfile_append(buffer, temp, n) ){
				httpclient_tcp_close(sockfd);
				*status_flag |= CON_FLAG_RESPONSEFAILED;
				*status_flag &= ~CON_FLAG_CONNECTED;
				error("memfile_append : buffer");
				return -1;
			}
		}
		if ( n <= 0 ) {
			httpclient_tcp_close(sockfd);
			*status_flag |= CON_FLAG_RESPONSEFAILED;
			*status_flag &= ~CON_FLAG_CONNECTED;
			error("more to receive but receive failed in %d msec timeout : %s"
					, handle->timeout, (n == 0) ? "connection closed" : strerror(errno));
			break;
		}
	}
	*/
	return recved;
}
//----------------------------------------------------------
int
http_buffering_getNByte(http_buffering_t *handle, void **buf, int size){
	memfile *buffer = handle->recvbuffer;
/*	unsigned long initOffset = memfile_getOffset(buffer);*/
	unsigned long initOffset = 
		handle->parsing_status->parsed_data_size
		- handle->parsing_status->flushed_data_size;
	long recvedSize = 0;

	memfile_setOffset(buffer, initOffset);

	recvedSize = _http_buffering_receiveNByte(handle, size);

	if( recvedSize < 0 ){
		return -1;
	}

	*buf = (void *)sb_malloc(recvedSize);
	if(!*buf && recvedSize > 0){
		error("sb_malloc : buf");
		return -1;
	}
	memfile_setOffset(buffer, initOffset);
	if ( memfile_read(buffer, *buf, recvedSize) != recvedSize ){
		error("memfile_read failed ");
		sb_free(*buf);
		return -1;
	}
	handle->parsing_status->parsed_data_size = 
		handle->parsing_status->flushed_data_size + memfile_getOffset(buffer);
	return recvedSize;
}
//----------------------------------------------------------
int
http_buffering_fill_memfile_NByte(http_buffering_t *handle, memfile *mfile, int size){
	memfile *buffer = handle->recvbuffer;
/*	unsigned long initOffset = memfile_getOffset(buffer);*/
	unsigned long initOffset = 
		handle->parsing_status->parsed_data_size
		- handle->parsing_status->flushed_data_size;
	long recvedSize=0;

	memfile_setOffset(buffer, initOffset);
	recvedSize = _http_buffering_receiveNByte(handle, size);

	if( recvedSize < 0 ){
		return -1;
	}

	memfile_setOffset(buffer, initOffset);
	if ( memfile_read2memfile(buffer, mfile, recvedSize) != recvedSize ){
		error("memfile_read failed ");
		return -1;
	}
	handle->parsing_status->parsed_data_size = 
		handle->parsing_status->flushed_data_size + memfile_getOffset(buffer);
	return recvedSize;
}
//----------------------------------------------------------




//----------------------------------------------------------
//	Receiving Until Connection Close
//----------------------------------------------------------
/*
int
http_buffering_receiveUntilClose(http_buffering_t *handle){
	int sockfd = handle->sockfd;
	char *status_flag = handle->statusFlag;
	memfile *buffer = handle->recvbuffer;

	char temp[CONST_BUF_SIZE];

	while( 1 ){
		int n;
		n = httpclient_tcp_recv(sockfd, temp, CONST_BUF_SIZE, handle->timeout);
		if ( n > 0 ){
			if ( 0 > memfile_append(buffer, temp, n) ){
				httpclient_tcp_close(sockfd);
				*status_flag |= CON_FLAG_RESPONSEFAILED;
				*status_flag &= ~CON_FLAG_CONNECTED;
				error("memfile_append : buffer");
				return -1;
			}
		}
		if ( n <= 0 ) {
			httpclient_tcp_close(sockfd);
			*status_flag &= ~CON_FLAG_CONNECTED;
			warn("connection close or socket error  : ");
			break;
		}
	}

	return 0;
}
*/
//----------------------------------------------------------
int
http_buffering_fill_memfile_UntilClose
(http_buffering_t *handle, memfile *mfile, char *connClosedFlag){
	int n;
	memfile *buffer = handle->recvbuffer;
/*	unsigned long initOffset = memfile_getOffset(buffer);*/
	unsigned long initOffset = 
		handle->parsing_status->parsed_data_size
		- handle->parsing_status->flushed_data_size;
	memfile_setOffset(buffer, initOffset);

/*	if ( http_buffering_receiveUntilClose(handle) < 0 ){*/
/*		return -1;*/
/*	}*/
	*connClosedFlag = 0;
/*	memfile_setOffset(buffer, initOffset);*/
	if ( memfile_isEndOfAppend(buffer) == 1 ) {
		*connClosedFlag = 1;
	}else debug("connection is not closed");

	n = memfile_read2memfile(buffer, mfile, memfile_getSize(buffer)-initOffset);
	if ( n < 0 ) {
		error("memfile_read2memfile failed : sizeToRead [%ld]", 
				memfile_getSize(buffer)-initOffset);
	}else {
		handle->parsing_status->parsed_data_size = 
			handle->parsing_status->flushed_data_size + memfile_getOffset(buffer);
	}
	return n;
		
}
//----------------------------------------------------------
