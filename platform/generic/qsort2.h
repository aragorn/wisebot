/* $Id$ */
#ifndef _SB_QSORT2_H_
#define _SB_QSORT2_H_ 1

#include <stdlib.h>
#include "softbot.h"

#ifndef HAVE_QSORT2 
SB_DECLARE(void) qsort2(void *a, size_t n, size_t es, void *usr, \
						int (*cmp)(const void *, const void *, void *));
#endif

#endif

