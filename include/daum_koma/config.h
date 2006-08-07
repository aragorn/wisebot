#ifndef _CONFIG_H
#define _CONFIG_H 1
/****************************************************************
 *                      CONFIG                                  *
 *                                                              *
 *                       Programmed by Kim, Eung Gyun           *
 *                       email : jchern@daumcorp.com			*
 *                       at : 2005.11.04						*
 ****************************************************************/
#include    <stdio.h>
#include    <string.h>
#include    <malloc.h>
#include    <ctype.h>

#ifndef WIN32
#	include    <unistd.h>
#endif

#include    <sys/stat.h>
#include    <fcntl.h>
#include    "moran.h"
#include    "moran_tagdef.h"
#include    "moran_grammar.h"

#define	CONFIGCOUNT			26
#define	SET					1
#define	UNSET				0

int	Check_PRE_DIC();
int	Check_STOP_DIC();
int	Check_USER_DIC();
int	Check_EJIDX();
int	Check_COMB_NOUN();
int	Check_POSITION();
int	Check_FIRST_NAME();
int	Check_PX_VERB();
int	Check_HADA_VERB();
int	Check_INPUT_WORD();
int	Check_ROOT_VERB();
int	Check_HANJA();
int	Check_DIGIT();
int	Check_HYPHEN();
int	Check_BOOLEAN();
int	Check_LEFT_ATTACH();
int	Check_RIGHT_ATTACH();
int	Check_HANNUM();
int	Check_ENGLISH();
int	Check_1SYALL();
int	Check_INPUT_CNOUN();
int	Check_AUTOCORRECTION();
int	Check_TAGGED_RESULT();
int	Get_SIM_WORD();
int	Check_INDEXING_LEVEL(int i);

void    Set_SIM_WORD_ORG(void);
void    Set_SIM_WORD_MULTI(void);
void    Set_SIM_WORD_ONLY(void);
void    Set_EJIDX(void);
void    Set_COMB_NOUN(void);
void    Set_ROOT_VERB(void);
void    Set_BOOLEAN(void);
void    Set_INDEXING_LEVEL1(void);
void    Set_INDEXING_LEVEL2(void);
void    Set_INDEXING_LEVEL3(void);
void    Set_INDEXING_LEVEL4(void);

#endif
