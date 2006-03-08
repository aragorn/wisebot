/* $Id$ */
#ifndef _HTTP_BUFFERING_H_
#define _HTTP_BUFFERING_H_

#include "memfile.h"

typedef enum {
	INITIAL,
	/* HEADER */
	HEADER_FIRST_LINE,
	HEADER_PARSING,
	HEADER_PARSING_CONTENT_LENGTH,
	HEADER_PARSING_TRANSFER_ENCODING,

	CONTENT_LENGTH,
	/* CHUNK */
	CHUNK_HEX,
    CHUNK_DATA,
	CHUNK_CRLF,
	CHUNK_READ_ENT_HEADER,
	CHUNK_LAST,

	MULTIPART_BYTERANGE,
	UNTIL_CLOSE,
	PARSING_COMPLETE
} http_parsing_state;


typedef struct {
	unsigned char invalidFlag;
	long flushed_content_len;
	long content_buf_offset;
	long parsed_data_size;
	long flushed_data_size;
	long wanted_data_size;
	http_parsing_state state;
} http_parsing_t;

typedef struct {
	http_parsing_t *parsing_status;
	memfile *recvbuffer;
} http_buffering_t;

/* By Pattern */
char *          
http_buffering_getUntil(http_buffering_t *handle, char **buf, char *pattern);

int
http_buffering_fill_memfile_Until(http_buffering_t *handle, memfile *buf, char *pattern);


/* By Bytes */
int
http_buffering_getNByte(http_buffering_t *handle, void **buf, int size);

int
http_buffering_fill_memfile_NByte(http_buffering_t *handle, memfile *mfile, int size);


/* By Connection Close */
int
http_buffering_fill_memfile_UntilClose(http_buffering_t *handle, 
										memfile *mfile, char *connClosedFlag);

#endif //_HTTP_BUFFERING_H_


