/* $Id$ 
 *	FIXME temporary, jiwon make system error code header file
 */
#ifndef ERROR_CODE_H
#define ERROR_CODE_H

#define SUCCESS		(1)    /* Function has succeeded. */
#define DECLINE		(0)    /* Function declines to serve the request. */
#define OK			(2)    /* All hooked function is called completely */
#define FAIL		(-1)   /* Function has failed to process the request. */
#define DONE		(-2)   /* Function has served the response completely
					        * - it's save to die() with no more output.
							*/
#define MINUS_DECLINE (-99)

#ifndef TRUE
#  define TRUE   (1)
#endif

#ifndef FALSE
#  define FALSE  (0)
#endif

#endif
