#include <stdio.h>
#include "moran_tagdef.h"

#define	KEYLENGTH 36
#define	VALUELENGTH 32


typedef	struct TN__
{
	char	TAG[32];
	int		tagnum;
}TN;

typedef	struct SRESULT__
{
	TN		ltag;
	char	keyword[32];
	TN		rtag;
}SRESULT;
