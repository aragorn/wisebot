/* $Id$ */
/*
**  MA_ENG.H
**  2001.05.  BY JaeBum, Kim.
*/
#ifndef _MA_ENG_
#define _MA_ENG_

#include "old_softbot.h"

#define ENG_IS_VOWEL(a)		((a) == 'A' || (a) == 'E' || (a) == 'I' || (a) == 'O' || (a) == 'U' )

typedef struct 
{
	TLONG	lID;
	char	*OldEnd;
	char	*NewEnd;
	TLONG	lOldOff;
	TLONG	lNewOff;
	TLONG	lMinWordSZ;
	TBOOL	(*pfnCond)(char *Word);
} CEngRL;

#endif	/*  _MA_ENG_  */

void	MAEanal(CMA *pMA, TBYTE *pszTok);


/*
**  END MA_ENG.H
*/

