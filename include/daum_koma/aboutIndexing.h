#ifndef _ABOUTINDEXING_H_
#define _ABOUTINDEXING_H_
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
#include	"config.h"
#include    "moran.h"
#include    "moran_tagdef.h"
#include    "moran_grammar.h"

#define		DEF_HA_		"èá"//ÇÏ
#define		DEF_DOI_	"ä÷"//µÇ
#define		DEF_HEA_	"è´"//ÇØ
#define		DEF_DOEA_	"ä÷"//µÅ

#define		DEF_DA		"´Ù"
#define		DEF_HA		"ÇÏ"
#define		DEF_DOI		"µÇ"
#define		DEF_HADA	"ÇÏ´Ù"
#define		DEF_DOIDA	"µÇ´Ù"
typedef	int (*HANL_Post)(FINAL_INFO*);
typedef	int	(*HANL_opt)();
typedef	struct index
{
	FINAL_INFO	*final_info;
	HANL_Post post;
	HANL_opt opt;
}postindex;
#endif
