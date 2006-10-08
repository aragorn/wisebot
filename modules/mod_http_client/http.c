/* $Id$ */
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "common_core.h"
#include "memory.h"
#include "parsedate.h" /* curl_getdate() */
#include "http_reserved_words.h"
#include "http.h"

#define HTTPHEADER_LINEEND	"\r\n"
#define HTTPHEADER_NECESSARY_LINEEND	"\n"
#define CR	'\r'
#define NL	'\n'

#define REMOVE_CR(buf, len)	\
{	\
	char *ptr = memchr(buf, CR, len);	\
	if ( ptr && (*(ptr+1) == NL || *(ptr+1) == '\0')  )	*ptr = '\0';	\
}


//----------------------------------------------------------
/* about HTTP version */
// major version translated like '1 -> 100'
// minor version translated like '1 -> 1'
// ex) HTTP 1.1 -> 1001 | 1.0 -> 1000
//----------------------------------------------------------
//
//---------------------------------------------------------------
/* about header-list */
static slist ** 
find_header(slist **where, const char *fieldName){ 
	int fieldname_len;
	char *separator;
	if( fieldName == NULL){
		error("null fieldname input");
		return NULL;
	}
	
	separator = strchr(fieldName, ':');
	if(separator != NULL ){
		fieldname_len = separator - fieldName;
	}else {
		fieldname_len = strlen(fieldName);
	}

	while( *where != NULL ){
		if ( (*where)->data == NULL ){
			error("impossible status of slist");
			return NULL;
		}
		
		if ( strncasecmp(fieldName, (*where)->data, fieldname_len) == 0 ){
			return where;
		}else {
			where = &((*where)->next);
		}
	}
	return where;
}

static int
set_headerByLine(slist **list, char *headerLine){
	slist *new, *now, **attachPnt;
	
	if (headerLine == NULL ){
		error("null header ");
		return -1;
	}

	attachPnt = find_header(list, headerLine);

	if ( attachPnt == NULL ){
		return -1;
	}

	now = *attachPnt;

	new = sb_malloc(sizeof(slist));
	if ( new == NULL ){
		error("sb_malloc failed: slist");
		return -1;
	}
	new->data = headerLine;

	*attachPnt = new;
	if(now == NULL){
		new->next = NULL;
	}else {
		new->next = now->next;
		sb_free(now->data);
		sb_free(now);
	}
	
	return 0;
}

static int
set_header(slist **list, const char *fieldName, const char *fieldVal){
	char *headerLine = sb_malloc( 
			sizeof(char)*( strlen(fieldName)+strlen(fieldVal)+3 ) ); // for ": " \0
	if ( headerLine == NULL) {
		error("sb_malloc failed: headerLine ");
		return -1;
	}
	sprintf(headerLine, "%s: %s"HTTPHEADER_LINEEND, fieldName, fieldVal);

	if ( set_headerByLine(list, headerLine) < 0 ){
		sb_free(headerLine);
		return -1;
	}
	return 0;
}


static char * 
get_header(slist **list, const char *fieldName){
	slist *now, **attachPnt;
	
	attachPnt = find_header(list, fieldName);
   	if (attachPnt == NULL ){
		return NULL;
	}

	now = *attachPnt;
	
	if ( now == NULL ){
		DEBUG("no matching line");
		return NULL;
	}
	return now->data;
}

static char *
pop_header(slist **list, const char *fieldName){
	slist *now, **attachPnt;
	char *p;
	
	attachPnt = find_header(list, fieldName);
   	if(attachPnt == NULL ){
		debug("can't find field [%s] ", fieldName );
		return NULL;
	}
	
	now = *attachPnt;

    if ( now == NULL ){
        DEBUG("no matching line");
        return NULL;
    }

	p = now->data;
	*attachPnt = now->next;
	sb_free(now);
	return p;
}
static void
free_header(slist *list){
	slist *temp;
	while(list){
		temp = list->next;
		sb_free(list->data);
		sb_free(list);
		list = temp;
	}
	return;
}

/* XXX where do we use this??
static void
print_header(slist *ptr){
    slist *temp;
    temp=ptr;
    fprintf(stderr, "\n-------- BEGIN : header ---------------");
    while(temp != NULL){
		fprintf(stderr, "\n[%s]", temp->data);
        temp=temp->next;
    }
    fprintf(stderr, "\n-------- END : header ---------------\n");
} 
*/


//----------------------------------------------------------
/* about chunk-decoding */

static int
httpchunk_read(http_buffering_t * handle, memfile *content_buf, slist **resp_header)
{
	http_parsing_t *parsing_status = handle->parsing_status;
	http_parsing_state *state = &(parsing_status->state);
	int chunk_data_size = 0;
	int len;

	while (1) {
		char *buf;
		char *pattern_ptr;
		switch (*state) {
			case CHUNK_HEX:
				pattern_ptr = http_buffering_getUntil(handle, &buf, HTTPHEADER_NECESSARY_LINEEND);
				if (pattern_ptr == NULL ){
					DEBUG("expected \\r\\n not found : illegal chunk data");
					return -1;
				}
				*pattern_ptr='\0';
				REMOVE_CR(buf, strlen(buf));

				pattern_ptr = strchr(buf,';');
				if(pattern_ptr){
					//FIXME now, it ignores [chunk-extension]
					*pattern_ptr = '\0';
				}
				chunk_data_size = strtoul(buf, NULL, 16);
				parsing_status->wanted_data_size = chunk_data_size;
				sb_free(buf);

				if(chunk_data_size == 0){
					*state = CHUNK_READ_ENT_HEADER;
				}else *state = CHUNK_DATA;
				break;

			case CHUNK_DATA:
				chunk_data_size = parsing_status->wanted_data_size;
				len = http_buffering_fill_memfile_NByte(handle, content_buf, chunk_data_size);
				if( len != chunk_data_size){
					DEBUG("http_buffering_fill_memfile_NByte : wanted [%d] received [%d]",
							chunk_data_size, len);
					if(len > 0 ){
						parsing_status->wanted_data_size -= len;
						parsing_status->content_buf_offset += len;
					}
					return -1;
				}
				parsing_status->wanted_data_size = 0;
				parsing_status->content_buf_offset = memfile_getSize(content_buf);
				*state = CHUNK_CRLF;
				break;

			case CHUNK_CRLF:
				pattern_ptr = http_buffering_getUntil(handle, &buf, HTTPHEADER_NECESSARY_LINEEND);
				if (pattern_ptr == NULL ){
					DEBUG("expected \\r\\n not found after chunk-data : illegal chunk data");
					return -1;
				}
/*				if (pattern_ptr != buf ){*/
				*pattern_ptr = '\0';
				REMOVE_CR(buf, strlen(buf));
				if ( strlen(buf) != 0 ) {
					error("invalid data found before CHUNK_CRLF");
					parsing_status->invalidFlag = 1;
					sb_free(buf);
					return -1;
				}
				sb_free(buf);
				*state = CHUNK_HEX;
				break;

			case CHUNK_READ_ENT_HEADER:
				pattern_ptr = http_buffering_getUntil(handle, &buf, HTTPHEADER_LINEEND);
				if ( pattern_ptr == NULL ){
					DEBUG("expected \\r\\n not found at footer : illegal chunk data");
					return -1;
				}
/*				if ( pattern_ptr == buf ){*/
				*pattern_ptr = '\0';
				REMOVE_CR(buf, strlen(buf));
				if ( strlen(buf) == 0 ){
					sb_free(buf);
					return 0;
				}
				if ( set_headerByLine(resp_header, buf) < 0 ){
					error("set_headerByLine failed : [%s]", buf);
					sb_free(buf);
					return -1;
				}
				sb_free(buf);
				break;

			default:
				parsing_status->invalidFlag = 1;
				error("Chunk state error!");
				return -1;
		}
	}
}

//-------------------------------------------------------------------
	/* about multipart_byterange */
static int
httpmultipart_byterange_read(http_buffering_t *handle, memfile *file, const char *boundary){
	int boundary_len, n=0;
	char *temp_boundary;
	if( boundary == NULL ){
		error("null boundary");
		return -1;
	}
	boundary_len = strlen(boundary);
	temp_boundary=(char *) sb_malloc(boundary_len+5);
	if(temp_boundary == NULL ){
		error("sb_malloc failed : temp_boundary");
		return -1;
	}
	crit("not yet implemented!!");
/*	sprintf(temp_boundary,"--%s--",boundary);*/
/*	n = http_buffering_fill_memfile_Until(handle, file, temp_boundary);*/
/*	sb_free(temp_boundary);*/
	return n;
}
//----------------------------------------------------------



//----------------------------------------------------------
//	HTTP_Functions .. 
//
//----------------------------------------------------------
//-------------------------------------------------------------------
int http_getResponse(http_t *http, const char *req_method, http_buffering_t *handle){
	char *pattern_ptr;

	int nc;

	int httpversion_major, httpversion_minor;
	int httpcode = 0 ;
	long contentlength;

	http_parsing_t *parsing_status = handle->parsing_status;
	http_parsing_state *state= &(parsing_status->state);
	memfile *recvbuffer = handle->recvbuffer;

	int temp;
	char *boundary;

	if ( *state == INITIAL ) {
		http_freeResponse(http);
		memfile_setOffset(recvbuffer, 0);
		
		*state = HEADER_FIRST_LINE;
		parsing_status->invalidFlag = 0;
		parsing_status->flushed_content_len = 0;
		parsing_status->content_buf_offset = 0;
		parsing_status->parsed_data_size = 0;
		parsing_status->flushed_data_size = 0;
		parsing_status->wanted_data_size = 0;
	}else {
/*		memfile_setOffset(handle->recvbuffer, parsing_status->parsed_data_size);*/
		memfile_setOffset(handle->recvbuffer, 
						parsing_status->parsed_data_size - parsing_status->flushed_data_size );
/*		if( http->content_buf ){*/
/*			memfile_setSize(http->content_buf, parsing_status->content_buf_offset);*/
/*			memfile_setOffset(http->content_buf, parsing_status->content_buf_offset);*/
/*		}*/
	}
	if( http->content_buf ){
		memfile_setSize(http->content_buf, parsing_status->content_buf_offset);
		memfile_setOffset(http->content_buf, parsing_status->content_buf_offset);
	}

	/* parse-the-header mode */
	while ( *state >= HEADER_FIRST_LINE 
			&& *state <= HEADER_PARSING_TRANSFER_ENCODING){
		char *header_buf;
		pattern_ptr = http_buffering_getUntil(handle, &header_buf, 
												HTTPHEADER_NECESSARY_LINEEND);
		if(!pattern_ptr){
			info("http_buffering_GetUntil : incomplete response header");
			return -1;
		}

		
		if (*state == HEADER_FIRST_LINE) {
			nc=sscanf(header_buf, "HTTP/%d.%d %3d",
					&httpversion_major, &httpversion_minor, &httpcode);
			if (nc!=3){
				/* NCSA 1.5.x returns this crap */
				nc=sscanf(header_buf, "HTTP %3d", &httpcode);
				httpversion_major = 1;
				httpversion_minor = 0;
			}

			sb_free(header_buf);

			if (nc) {
				http->response_http_code = httpcode;
				http->response_http_ver = 1000 * httpversion_major + httpversion_minor;
			} else {
				error("invalid response ");
				parsing_status->invalidFlag = 1;
				return -1;
			}
			parsing_status->state = HEADER_PARSING;
			parsing_status->wanted_data_size = 0;
			parsing_status->content_buf_offset = 0;
			continue;
		}
		*pattern_ptr = '\0';
		REMOVE_CR(header_buf, strlen(header_buf));

		/* handle the end of header */
/*		if (header_buf == pattern_ptr) {*/
		if ( strlen(header_buf) == 0 ) {
			sb_free(header_buf);
			break;
		}

		/* add this line to the response header list*/
		if(set_headerByLine(&(http->response_header), header_buf) < 0 ){
			error("set_headerByLine failed : [%s]", header_buf);
			sb_free(header_buf);
			parsing_status->invalidFlag = 1;
			return -1;
		}		
		
		/* header parsing */
		if (!strncasecmp(HTTP_CONTENT_LENGTH":", header_buf, 15) &&
				sscanf(header_buf+15, " %ld", &contentlength)) {
			http->content_len = contentlength;
			if( *state == HEADER_PARSING ) {
				*state = HEADER_PARSING_CONTENT_LENGTH;
			}
		} else if (!strncasecmp(HTTP_CONTENT_TYPE":", header_buf, 13)) {
/*			char *tmpPtr;*/
/*			sscanf(header_buf+13, " %s", contentType);*/
/*			|+ check optional parameters +|*/
/*			if ( (tmpPtr = strchr(contentType, ';')) ) {*/
/*				|+ XXX: ignore charset +|*/
/*				*tmpPtr = '\0';*/
/*			}*/
			char *tmp = sb_strdup(header_buf+13+1);
			if(tmp == NULL){
				error("sb_strdup failed : [%s]", header_buf);
				sb_free(header_buf);
				parsing_status->invalidFlag = 1;
				return -1;
			}
			http->content_type = tmp;
		} else if (!strncasecmp(HTTP_TRANSFER_ENCODING":", header_buf, 18)){
			char *tmp = sb_strdup(header_buf+18+1);
			if(tmp == NULL){
				error("sb_strdup failed : [%s]", header_buf);
				sb_free(header_buf);
				parsing_status->invalidFlag = 1;
				return -1;
			}
			http->transfer_encoding = tmp;
			if ( *state == HEADER_PARSING){
				*state = HEADER_PARSING_TRANSFER_ENCODING;
			}
		} else if (!strncasecmp(HTTP_LAST_MODIFIED":", header_buf, 14)) {
            time_t  secs=time(NULL);
            http->lm_date = curl_getdate(header_buf+14+1, &secs);
		} else if ((httpcode >= 300 && httpcode < 400) &&
				(!strncasecmp(HTTP_LOCATION":", header_buf, 9))) {
			char *start, *ptr, backup;
			
			start = header_buf + 9;
			while (*start && isspace((int)*start))
				start++;
			ptr=start;
			
			while (*ptr && !isspace((int)*ptr))
				ptr++;
			backup = *ptr;
			*ptr = '\0';
			http->location=(char *)sb_malloc(strlen(start)+1);
			if(!http->location){
				error("sb_malloc failed : [%s]", header_buf);
				sb_free(header_buf);
				parsing_status->invalidFlag = 1;
				return -1;
			}
			strcpy(http->location, start);
			*ptr = backup;
		}
		parsing_status->content_buf_offset = 0;
	} 
	/* end of header parsing */

	if ( *state == HEADER_PARSING  || *state == HEADER_PARSING_CONTENT_LENGTH
			|| *state == HEADER_PARSING_TRANSFER_ENCODING ) {
		//FIXME handle 1xx Status Code
		if ( http->response_http_code == 100 || http->response_http_code == 101 ) {
			http_freeResponse(http);
			*state = HEADER_FIRST_LINE;
			parsing_status->wanted_data_size = 0;
			parsing_status->content_buf_offset = 0;
			return -1;
		}

		if (http->response_http_code < 200 || http->response_http_code == 204 
			|| http->response_http_code == 304
			|| (req_method != NULL && !strcmp(req_method, HTTP_METHOD_HEAD)) ){
				/* 1) responses with no message-body 
				 * 1xx, 204, 304, and all responses to HEAD request */
				//FIXME you should set up request_method !! 
			parsing_status->wanted_data_size = 0;
			*state = PARSING_COMPLETE;
			return 0;
		} else if ( *state == HEADER_PARSING_CONTENT_LENGTH ){
			/* 2) when content_length exist.. 
			 * (and earlier than transfer-encoding) */
			parsing_status->wanted_data_size = http->content_len;
			*state = CONTENT_LENGTH;
		} else if ( http->transfer_encoding 
			&& strncmp(http->transfer_encoding, HTTP_WORD_IDENTITY, 8) != 0 ){
			/* 3) when transfer_encoding exist..
			 * (and earlier than content-length) */
			*state = CHUNK_HEX;
		} else if ( http->content_type 
			&& strncmp(http->content_type, HTTP_WORD_MULTIPART_BYTERANGE, 20) == 0 ){
			/* 4) when message uses multipart/byteranges */
			*state = MULTIPART_BYTERANGE;
		} else {
			/* 5) by the server closing the connection */
			*state = UNTIL_CLOSE;
		}
		
	}
	
	if(!http->content_buf){
		http->content_buf = memfile_new();
		if(!http->content_buf){
			error("memfile_new failed : content_buf");
			return -1;
		}
	}

	switch(*state) {
		case CONTENT_LENGTH:
			temp = http_buffering_fill_memfile_NByte(handle,
					http->content_buf, parsing_status->wanted_data_size);
			if ( temp != parsing_status->wanted_data_size ) {
				DEBUG("wanted data size was [%ld] but received [%d]", 
					parsing_status->wanted_data_size, temp);
				if( temp > 0 ){
					parsing_status->wanted_data_size -=temp;
					parsing_status->content_buf_offset += temp;
				}
				return -1;
			}else {
				parsing_status->wanted_data_size = 0;
				*state = PARSING_COMPLETE;
				return 0;
			}
			break;
		case CHUNK_HEX:
		case CHUNK_DATA:
		case CHUNK_CRLF:
		case CHUNK_READ_ENT_HEADER:
		case CHUNK_LAST:
			if ( 0 > httpchunk_read(handle, http->content_buf, 
					&(http->response_header) ) ){
				return -1;
			}else {
				parsing_status->wanted_data_size = 0;
				*state = PARSING_COMPLETE;
				return 0;
			}
			break;
		case MULTIPART_BYTERANGE:
			boundary = strstr(http->content_type, HTTP_WORD_BOUNDARY"=");
			if(boundary == NULL){
				DEBUG(HTTP_WORD_BOUNDARY" not found in" HTTP_WORD_MULTIPART_BYTERANGE" content");
				parsing_status->invalidFlag = 1;
				return -1;
			}
			boundary += 9;
			if ( 0 > httpmultipart_byterange_read(
					handle, http->content_buf, boundary) ){
				return -1;
			}else {
				parsing_status->wanted_data_size = 0;
				*state = PARSING_COMPLETE;
				return 0;
			}
			break;
		case UNTIL_CLOSE:
			{
				char connClosedFlag;
				int n;
				n = http_buffering_fill_memfile_UntilClose(handle, 
						http->content_buf, &connClosedFlag );
				if ( n < 0 ){
					error("error while receiving until connection closed");
					parsing_status->invalidFlag = 1;
					return -1;
				}
				parsing_status->content_buf_offset += n;

				if ( connClosedFlag == 1 ) {
					*state = PARSING_COMPLETE;
					return 0;
				}else {
					DEBUG("waiting for connection to be closed");
					return -1;
				}
			}
			break;
		default:
			error("unknown http_parsing status [%d] ", *state);
			return -1;
	}
}
//----------------------------------------------------------
static memfile *http_makeRequestHeader(http_t *http)
{
	memfile *req_buffer;
	
	if ( !http->method || !http->path ){
		error("request method or path has not been set");
		return NULL;
	}

	if (http->host == NULL && http->request_http_ver == 1001) {
		error("Host is required for HTTP/1.1");
		return NULL;
	}

	if (http->req_message_body != NULL ){
		if ( http->req_content_len < 0 || http->req_content_type == NULL){
			error("Request with 'body' require "
					HTTP_CONTENT_LENGTH" and "HTTP_CONTENT_TYPE);
			return NULL;
		}
	}
	
	req_buffer = memfile_new();		/* 버퍼를 초기화하는 함수 */
	if (!req_buffer) {
		error("memfile new failed : req_buffer");
		return NULL;
	}

	/*	버퍼에, "%s.." 같은 포맷으로 써서, 집어넣을 수 있는 함수 */
	memfile_appendF(req_buffer, 
		"%s "					/* GET/HEAD/POST/PUT */
		"%s HTTP/%d.%d"HTTPHEADER_LINEEND,	/* path, version */
		http->method,
		http->path, http->request_http_ver/1000, http->request_http_ver%1000 );
	
	if(http->user_agent){
		memfile_appendF(req_buffer, HTTP_USER_AGENT": %s"HTTPHEADER_LINEEND, http->user_agent);
	}
	if(http->accept){
		memfile_appendF(req_buffer, HTTP_ACCEPT": %s"HTTPHEADER_LINEEND, http->accept);
	}
	if(http->req_content_len >= 0){
		memfile_appendF(req_buffer, HTTP_CONTENT_LENGTH": %d"HTTPHEADER_LINEEND, http->req_content_len);
	}
	if(http->req_content_type){
		memfile_appendF(req_buffer, HTTP_CONTENT_TYPE": %s"HTTPHEADER_LINEEND, http->req_content_type);
	}
	if(http->connection){
		memfile_appendF(req_buffer, HTTP_CONNECTION": %s"HTTPHEADER_LINEEND, http->connection);
	}
	if(http->host){
		memfile_appendF(req_buffer, HTTP_HOST": %s"HTTPHEADER_LINEEND, http->host);
	}

	if (http->custom_request_header) {
		slist *head;
		for (head=http->custom_request_header; head; head=head->next)
			memfile_appendF(req_buffer, "%s"HTTPHEADER_LINEEND, head->data);
	}

	memfile_appendF(req_buffer, HTTPHEADER_LINEEND);

	return req_buffer;
}
//----------------------------------------------------------
inline memfile *http_makeRequest(http_t *http)
{
	memfile *req_buffer;

	req_buffer = http_makeRequestHeader(http);
	if ( req_buffer == NULL ) {
		return NULL;
	}
	if(http->req_message_body){
		memfile_setOffset(http->req_message_body, 0);
		memfile_read2memfile(http->req_message_body, req_buffer, 
				memfile_getSize(http->req_message_body));
	}

	return req_buffer;
}

http_t *http_new(void)
{
	http_t *http;
	
	http = (http_t *) sb_calloc(1, sizeof(http_t));
	if (!http) {
		error("sb_malloc: http");
		return NULL;
	}

	/* about request */
	http->method = HTTP_METHOD_GET;
	http->path = NULL;
	http->req_message_body = NULL;
	http->custom_request_header = NULL;
	http->user_agent = NULL ;
	http->accept = NULL;
	http->req_content_len = -1;
	http->req_content_type = NULL;
	http->connection = NULL;

	http->host = NULL;
	http->request_http_ver = 1000;

	/* about response */
	http->content_buf = NULL;
	http->response_header = NULL;
	http->content_type = NULL;
	http->content_len = -1;
	http->transfer_encoding = NULL;
	http->location = NULL;
	http->lm_date = 0;
	http->response_http_ver = -1;
	http->response_http_code = -1;

	return http;
}

//------------------------------------------------------------
// freeing.
void
http_freeRequest(http_t *http){
	if (!http)
		return;
	
	http->method = NULL;
	http->path = NULL;

	if(http->req_message_body){
		memfile_free(http->req_message_body);
		http->req_message_body = NULL;
	}

	if(http->custom_request_header){
		free_header(http->custom_request_header);
		http->custom_request_header = NULL;
	}

	http->user_agent = NULL;
	http->accept = NULL;
	http->host = NULL;
	http->req_content_len = -1;

	http->req_content_type = NULL;
	http->connection = NULL;
}
//----------------------------------------------------------
void
http_freeResponse(http_t *http){
	if (!http)
		return;

	if(http->content_buf){
		memfile_free(http->content_buf);
		http->content_buf = NULL;
	}

	if(http->response_header){
		free_header(http->response_header);
		http->response_header = NULL;
	}

	if(http->transfer_encoding){
		sb_free(http->transfer_encoding);
		http->transfer_encoding = NULL;
	}
	http->content_len = -1;
	http->lm_date = 0;
	http->response_http_ver = -1;
	http->response_http_code = -1;
	if(http->content_type){
		sb_free(http->content_type);
		http->content_type = NULL;
	}
	if(http->location){
		sb_free(http->location);
		http->location = NULL;
	}
}
//----------------------------------------------------------
void
http_free(http_t *http){
	http_freeRequest(http);
	http_freeResponse(http);
	sb_free(http);
}
//--------------------------------------------------------------

//-------------------------------------------------------------------
inline int
http_setCustomRequestHeaderByLine(http_t *http, char *oneLine){
	if( set_headerByLine(&(http->custom_request_header), oneLine) < 0 ){
		return FAIL;
	}else return SUCCESS;
}
//----------------------------------------------------------
inline int
http_setCustomRequestHeader(http_t *http, const char *fieldName, const char *fieldVal ){
	if ( set_header(&(http->custom_request_header), fieldName, fieldVal) < 0 ){
		return FAIL;
	}else return SUCCESS;
}
//----------------------------------------------------------
inline char *
http_popFromResponseHeader(http_t *http, const char *fieldName){
	return pop_header(&(http->response_header), fieldName);
}
//----------------------------------------------------------
inline char *
http_getFromResponseHeader(http_t *http, const char *fieldName){
	return get_header(&(http->response_header), fieldName);
}
//----------------------------------------------------------
inline int
http_reserveMessageBody(http_t *http, char *type, long len){
	if(!http || !type || len <=0 ){
		error("invalid input to reserve message body");
		return FAIL;
	}
	http->req_message_body = NULL;
	http->req_content_type = type;

	http->req_content_len = len;
	return SUCCESS;
}
//----------------------------------------------------------
inline int
http_setMessageBody(http_t *http, memfile *msgBody, char *type, long len){
	if(!http) {
		error("invalid input to set message body, http null");
		return FAIL;
	}
	if(!msgBody) {
		error("invalid input to set message body, msgBody null");
		return FAIL;
	}
	if(!type) {
		error("invalid input to set message body, type null");
		return FAIL;
	}
	if(len <= 0) {
		error("invalid input to set message body, len[%ld] <= 0", len);
		return FAIL;
	}

	http->req_message_body = msgBody;
	http->req_content_type = type;

	http->req_content_len = len;
	return SUCCESS;
}
//----------------------------------------------------------
void
http_prepareAnotherRequest(http_t *http){
	if (!http)
		return;

	http->path = NULL;
	http->host = NULL;
	if(http->req_message_body){
		memfile_free(http->req_message_body);
		http->req_message_body = NULL;
	}
	if(http->custom_request_header){
		free_header(http->custom_request_header);
		http->custom_request_header = NULL;
	}
	http->req_content_len = -1;
	http->req_content_type = NULL;
}
//----------------------------------------------------------
void
http_print(http_t *http){
	slist *temp;
	if(!http)
		return;
	printf("----------- BEGIN : http --------\n");
	printf("Request :\n");
	printf("http_version [%d.%d]\n", 
			http->request_http_ver/1000 , http->request_http_ver%1000 );
	printf("method [%s]\n", http->method ? http->method : "null");
	printf("path [%s]\n", http->path ? http->path : "null");
	printf("message_body : \n");
	if( http->req_message_body ) {
		memfile_print(http->req_message_body);
	}
	temp = http->custom_request_header;
	while (temp){
		printf("custom_req_heaer -- %s\n",temp->data ? temp->data : "null");
		temp = temp->next;
	}
	printf("user_agent : [%s]\n", http->user_agent ? http->user_agent : "null");
	printf("accept : [%s]\n", http->accept ? http->accept : "null");
	printf("host : [%s]\n", http->host ? http->host : "null");
	printf("req_content_len : [%ld]\n", http->req_content_len);
	printf("req_content_type : [%s]\n", 
			http->req_content_type ? http->req_content_type : "null");
	printf("connection : [%s]\n", http->connection ? http->connection : "null");
	printf("\n");
	printf("Response :\n");
	printf("http_version : [%d.%d]\n",
			http->response_http_ver/1000 , http->response_http_ver%1000);
	printf("http_code : [%d]\n", http->response_http_code);
	printf("content-len : [%ld]\n", http->content_len);
	printf("content_type : [%s]\n", 
			http->content_type ? http->content_type : "null");
	printf("transfer_encoding : [%s]\n",
			http->transfer_encoding ? http->transfer_encoding : "null");
	printf("location : [%s]\n",
			http->location ? http->location : "null");
	printf("lm_date : [%ld]\n", http->lm_date);
	temp = http->response_header;
	while (temp){
		printf("response_headers -- %s\n",temp->data ? temp->data : "null");
		temp = temp->next;
	}
	
	printf("content :\n");
	if( http->content_buf ) {
		memfile_print(http->content_buf);
	}
	printf("----------- END : http --------\n");
	return;
}

void *sb_hack_http_print = http_print;

//----------------------------------------------------------
//
