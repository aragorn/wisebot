/*
 * $Id$
 */
#ifndef _SYNONYM_H_
#define _SYNONYM_H_

#include "softbot.h"

typedef struct {
	Int8	wordType;
	char	word[MAX_WORD_LEN];
} Synonym;

int SE_init();
int SE_get(int fieldId, char aWord[], int numSynonym, Synonym aSynonym[]);

#endif
