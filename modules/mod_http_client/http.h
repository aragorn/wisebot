/* $Id$ */
#ifndef HTTP_H
#define HTTP_H

#include "memfile.h"
#include "http_buffering.h" /* http_parsing_t */

typedef struct slist slist;

struct slist {
	char	*data;
	slist	*next;
};

typedef struct {
	/* about request */
	int request_http_ver;	/* HTTP request version */
	char *method;			/* HTTP request method */
	char *path;				/* path to request */
	memfile *req_message_body;
	slist *custom_request_header;	/* list of custom req_header */
	//predefined header content
	char *user_agent;
	char *accept;
	char *host;
	long req_content_len;
	char *req_content_type;
	char *connection;

	/* about response */
	memfile *content_buf;	/* the body content */
	slist *response_header;		/* response headers */
	//resp-headers to be parsed
	long  content_len;		/* content length */
	char *content_type;		/* content type */
	char *transfer_encoding;/* transfer encoding */
	char *location;			/* the content of Location: header */
	time_t lm_date;			/* last modified date */
	int response_http_ver;		/* response HTTP version */
	int response_http_code;			/* HTTP response code */
} http_t;


/* functions */

http_t *http_new(void);
void http_free(http_t *http);
void http_prepareAnotherRequest(http_t *http);
void http_print(http_t *http);

void http_freeRequest(http_t *http);
int http_setCustomRequestHeaderByLine(http_t *http, char *oneLine);
int http_setCustomRequestHeader(http_t *http, const char *fldName, const char *fldVal);
int http_setMessageBody(http_t *http, memfile *msgBody, char *type, long len);
int http_reserveMessageBody(http_t *http, char *type, long len);
memfile *http_makeRequest(http_t *http);

void http_freeResponse(http_t *http);
char * http_getFromResponseHeader(http_t *http, const char *fieldName);
char * http_popFromResponseHeader(http_t *http, const char *fieldName);
int http_getResponse(http_t *http, const char *req_method, http_buffering_t *buffering_handle);

//#ifndef NULL
//#define NULL ((void *)0)
//#endif //NULL

#endif // HTTP_H
