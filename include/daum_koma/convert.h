#ifndef	__CONVERT_H__
#define	__CONVERT_H__

#include "hantype.h"

#ifdef	__cplusplus
extern	"C"	{
#endif

extern	hancode euckr2johab(hancode * word, hancode *dest);
extern	hancode johab2euckr(hancode * word, hancode *dest);

#ifdef	__cplusplus
};
#endif

#endif	// __CONVERT_H__

