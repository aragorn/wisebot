/* $Id$ */
#ifndef RELATE_TERM_H
#define RELATE_TERM_H 1

#include <stdint.h>

typedef struct{
	int8_t relType;
	char   word[MAX_WORD_LEN];
} RelatedTerm;

int RT_get(char aWord[], RelatedTerm aRelTerm[], int maxItem);

#endif
