/* $Id$ */
#ifndef CONSTANTS_H
#define CONSTANTS_H

#define SUCCESS		(1)    /* Function has succeeded. */
#define DECLINE		(0)    /* Function declines to serve the request. */
#define OK			(2)    /* All hooked function is called completely */
#define FAIL		(-1)   /* Function has failed to process the request. */
#define DONE		(-2)   /* Function has served the response completely
					        * - it's safe to die() with no more output.
							*/

#define MINUS_DECLINE (-99)

#ifndef TRUE
#  define TRUE   (1)
#endif

#ifndef FALSE
#  define FALSE  (0)
#endif

#define MAX_WORD_LEN			(40)
#define MAX_PHRASE_LEN			(200)
#define MAX_WORD_PER_PHRASE		(128)

#define MAX_PATH_LEN			(256)
#define MAX_FILE_LEN			(256)

#define COMMENT_LIST_SIZE		(130)
#define DOCUMENT_SIZE			(10000*1024)
#define DEFAULT_DOCUMENT_SIZE	(10000*1024)
#define BIN_DOCUMENT_SIZE		(40000*1024)

#define LONG_LONG_STRING_SIZE	(8192)
#define LONG_STRING_SIZE		(1024)
#define STRING_SIZE				(256)
#define SHORT_STRING_SIZE		(64)

#define FIELD_NUM_PER_DOC		(64)
#define MAX_FIELD_NUM			FIELD_NUM_PER_DOC
#define MAX_FIELD_LEN			SHORT_STRING_SIZE
#endif
