/*
 * tweak workaround for qsort.c, qsort2.c, msort.c
 *
 * by aragorn, 2003/07/09
 */
#ifndef _PROTO_H_
#define _PROTO_H_ 1

#ifdef __STDC__
#  ifndef __P
#    define __P(x)  x
#  endif
#else
#  ifndef __P
#    define __P(x)  ()
#  endif
#endif /* __STDC__ */

#endif
 
