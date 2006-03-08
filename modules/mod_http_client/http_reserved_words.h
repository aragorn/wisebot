/* $Id$ */
#ifndef _HTTP_RESERVED_WORDS_H_
#define _HTTP_RESERVED_WORDS_H_ 

/* about request method */
#define HTTP_METHOD_GET		"GET"
#define HTTP_METHOD_POST	"POST"
#define HTTP_METHOD_HEAD	"HEAD"
#define HTTP_METHOD_PUT		"PUT"

#define HTTP_ACCEPT				"Accept"
#define HTTP_CONNECTION			"Connection"
#define HTTP_CONTENT_LENGTH		"Content-Length"
#define HTTP_CONTENT_TYPE		"Content-Type"
#define HTTP_HOST				"Host"
#define HTTP_LAST_MODIFIED		"Last-Modified"
#define HTTP_LOCATION			"Location"
#define HTTP_TRANSFER_ENCODING	"Transfer-Encoding"
#define HTTP_USER_AGENT			"User-Agent"

//Transfer-Encoding
#define HTTP_WORD_IDENTITY		"identity"
//Content-Type
#define HTTP_WORD_MULTIPART_BYTERANGE	"multipart/byteranges"
#define HTTP_WORD_BOUNDARY		"boundary"
#define HTTP_WORD_CHARSET		"charset"
//Connection
#define HTTP_WORD_CLOSE			"close"
#define HTTP_WORD_KEEPALIVE		"Keep-Alive"

/* about http status code */
#define HTTP_STATUS_CODE_INFORMATION	100
#define HTTP_STATUS_CODE_SUCCESS		200
#define HTTP_STATUS_CODE_REDIRECTION 	300
#define HTTP_STATUS_CODE_ERROR			400
#define HTTP_STATUS_CODE_CLIENT_ERROR	400
#define HTTP_STATUS_CODE_SERVER_ERROR	500

#endif //_HTTP_RESERVED_WORDS_H_
