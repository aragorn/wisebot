/* $Id$ */
#ifndef RELATE_TERM_H
#define RELATE_TERM_H 1

#include "auto_config.h"

#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

typedef struct{
	int8_t relType;
	char   word[MAX_WORD_LEN];
} RelatedTerm;

int RT_get(char aWord[], RelatedTerm aRelTerm[], int maxItem);

#endif
