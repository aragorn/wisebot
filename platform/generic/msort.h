/* $Id$ */
#ifndef _SB_MSORT_H_
#define _SB_MSORT_H_ 1

#include <stdlib.h>
#include "softbot.h"

#ifndef HAVE_MERGESORT
SB_DECLARE(int) mergesort(void *base, size_t nmemb, size_t size, \
				int (*compar)(const void *, const void *));
#endif

#endif

