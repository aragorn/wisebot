/*
 * $Id$
 * 
 */
#ifndef _RELATE_TERM_H_
#define _RELATE_TERM_H_

#include "softbot.h"

typedef struct{
	Int8 relType;
	char word[MAX_WORD_LEN];
} RelatedTerm;

int RT_get(char aWord[], RelatedTerm aRelTerm[], int maxItem);

#endif
