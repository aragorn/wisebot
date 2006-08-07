/****************************************************************
 *						DS OPTION				      			*
 *                                                              *
 *                       Program by Kim, Eung, Gyun				*
 *                       email : jchern@daumcorp.com			*
 *                       at : 2005.11.04						*
 ****************************************************************/
#include 	<stdio.h>
#include 	<string.h>
#include	<malloc.h>
#include 	<ctype.h>

#ifndef WIN32
#	include    <unistd.h>
#endif

#include	<sys/stat.h>
#include	<fcntl.h>
#include  	"moran.h"
#include	"moran_tagdef.h"
#include	"moran_grammar.h"


#define		OPT_DEFAULT		0xFF
/*										01234567	89012345	*/
#define		OPT_INDEX_APOS1	0xFFFF	/*	11111111	11111111	00 - 15*/
#define		OPT_INDEX_APOS2	0xFF0D	/*	11111111	00001101	16 - 31*/
#define		OPT_INDEX_APOS3	0xC410	/*	11000100	00000000	32 - 47*/
#define		OPT_INDEX_APOS4	0x0001	/*	00000000	00000001	48 - 63*/	//00000000 00000001 : 사장 추출(o) | 00000000 00000000 : 사장 추출(x)
#define		OPT_INDEX_APOS5	0x0034	/*	00000000	00110100	64 - 79*/

/* 색인 옵션 ( DS 기준 ) */
#define		QUERY				1
#define		VERBADJ         	2
#define		NUMENG	        	3
#define		ALLINDEX        	4 


static unsigned short int	g_usIndex_option[5];


int	SetOption(int opt);
int	CheckOption(int opt);
int DS_NLPGetOpcode(const char *szOptionString,int * opcode);
