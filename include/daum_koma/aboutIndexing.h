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

#define		DEF_HA_		"��"//��
#define		DEF_DOI_	"��"//��
#define		DEF_HEA_	"�"//��
#define		DEF_DOEA_	"��"//��

#define		DEF_DA		"��"
#define		DEF_HA		"��"
#define		DEF_DOI		"��"
#define		DEF_HADA	"�ϴ�"
#define		DEF_DOIDA	"�Ǵ�"
typedef	int (*HANL_Post)(FINAL_INFO*);
typedef	int	(*HANL_opt)();
typedef	struct index
{
	FINAL_INFO	*final_info;
	HANL_Post post;
	HANL_opt opt;
}postindex;
#endif
