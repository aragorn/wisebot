/* $Id$ */
/*
**  SOFTBOT v46
**  SOFTBOT.H
**  2000.08. BY KJB
*/
#ifndef _SOFTBOT_
#define _SOFTBOT_

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>

#define  SB_VER		"v4.6"

/**
#define _DEBUG_
**/

/*	
               SoftBot Install Environment
*/
/* *************************************************************** */
#if !defined(_FILTER_MS) || !defined(_DEMO_VER_) || !defined(_USE_CAT_)
#endif

/*
			SITE
*/
/* ***************************************************** */
#if !defined(_SITE_YAHOO_) || !defined(_SITE_NETIAN_) || !defined(_SITE_THRUNET_)
#define _SITE_YAHOO_
#endif
/*
               OPERATING SYSTEM
*/
/* *************************************************************** */
#if !defined(_WIN_VER_) && !defined(_UNIX_VER_)
//#	define _WIN_VER_
#	define _UNIX_VER_
#endif

#ifdef _WIN_VER_
#	include <windows.h>
#	if !defined(_SBA_DLL_)  /*  for DLL  */
#	endif
#	ifdef _SBA_DLL_
#		define EXPORT	__declspec(dllexport)
#	endif
#	if !defined(_WIN_95_) && !defined(_WIN_31_) && !defined(_WIN_NT_)
#		define	_WIN_95_
#	endif
#	define DC_DIR	'\\'
#	define DS_DIR	"\\"
#endif  /* _WIN_VER_ */

#ifdef _UNIX_VER_
#	if !defined(_SYS_V_) && !defined(_BSD_)
#		define _SYS_V_
#	endif
#	if !defined(_LINUX_)
#		define _LINUX_
#	endif
#	if !defined(_IBM_SP2_)
#	endif
#	if !defined(_FREE_BSD_)
#	endif
#	define DC_DIR	'/'
#	define DS_DIR	"/"
#endif  /* _UNIX_VER_ */

#define DC_ITEM   '^'
#define DS_ITEM   "^"
#define DC_ELEM   ':'
#define DS_ELEM   ":"

/* 
               SoftBot Type
*/
/* *************************************************************** */
#ifdef _WIN_31_
	typedef char TCHAR;
	typedef unsigned char TBYTE;
	typedef int	TSINT;
	typedef unsigned int TWORD;
	typedef long TLONG;
	typedef unsigned long TDWORD;
#else
	typedef char TCHAR;
	typedef unsigned char TBYTE;
	typedef short TSINT;
	typedef unsigned short TWORD;
	typedef int TLONG;
	typedef unsigned int TDWORD;
#endif /* _WIN_31_ */

#ifdef _WIN_VER_
	typedef SOCKET	TSOCK;
#else
	typedef int		TSOCK;
#endif

typedef int		TINT;
typedef TINT	TBOOL;
typedef TINT	TSIZE;
typedef TINT	TNUM;
typedef void*	SBHANDLE;


/*
               OPERAND
*/
/* *************************************************************** */
#define OP_DEF       ' '

#ifdef _SITE_YAHOO_
#define OP_AND       '+'
#define OP_OR        ','
#define OP_TITLE     "TITLE"

#else
#define OP_AND       '&'
#define OP_OR        ','
#define OP_TITLE	 "TT"
#endif  /*  _SITE_YAHOO_  */

#define OP_AND_SRT   '{'
#define OP_AND_END   '}'
#define OP_OR_SRT    '('
#define OP_OR_END    ')'
#define OP_FD_SRT	 '#'
#define OP_FD_END    ':'
#define OP_SENT_SRT	 '"'
#define OP_SENT_END	 '"'

#define OP_NOT       '-'
#define OP_NEAR      '~'
#define OP_URL       "URL"
#define OP_FD1		 "FD1"
#define OP_FD2		 "FD2"
#define OP_FD3		 "FD3"
#define OP_FD4		 "FD4"
#define OP_FD5		 "FD5"

/*
               Sort How
*/
/* *************************************************************** */
#define SH_DEF  (0)
#define SH_HIT	(0)
#define SH_DATE	(1)
#define SH_DATE_INORDER	(2)

/*
               List Count
*/
#define LC_MIN	(10)
#define LC_MAX  (100)

/*
               SoftBot Constant
*/
/* *************************************************************** */
#define SBC_CAT_NM		(128)

#define SBC_DOC_DIT		(1024)
#define SBC_DOC_OID		(256)
#define SBC_DOC_TT		(100)
#define SBC_DOC_DN		(32)
#define SBC_DOC_AT		(16)
#define SBC_DOC_KW		(64)
#define SBC_DOC_FD		(64)

#define SBC_IO			(1024)
#define SBC_ITEM		(512)
#define SBC_WORD		(20)
#define SBC_QU			(512)
#define SBC_DT			(12)
#define SBC_COMMENT		(310)

/*
               FIELD
*/
/* *************************************************************** */
#define MB_BODY		(0x01)
#define MB_TITLE	(0x02)
#define MB_URL		(0x04)
#define MB_FIELD1	(0x08)
#define MB_FIELD2	(0x10)
#define MB_FIELD3	(0x20)
#define MB_FIELD4	(0x40)
#define MB_FIELD5	(0x80)
#define MB_ALL		(0xff)

/*  
               Document I/O  
*/
/* *************************************************************** */
typedef struct tagDocIO
{
	TLONG HIT;
	TLONG DID;
	TWORD DST;
	char  CT[SBC_CAT_NM];
	char  DTR[SBC_DT];
	char  DTC[SBC_DT];
	char  DN[SBC_DOC_DN];
	char  OID[SBC_DOC_OID];
	char  TT[SBC_DOC_TT];
	char  AT[SBC_DOC_AT];
	char  KW[SBC_DOC_KW];
	char  FD1[SBC_DOC_FD];
	char  FD2[SBC_DOC_FD];
	char  FD3[SBC_DOC_FD];
	char  FD4[SBC_DOC_FD];
	char  FD5[SBC_DOC_FD];
	char  KP[SBC_DOC_FD];
	char  CMT[SBC_COMMENT];
} CDocIO;

/*
		       Search IO
*/
/* *************************************************************** */
typedef struct tagSRIO
{
	char	QU[SBC_QU];
	char    CT[SBC_CAT_NM];
	TINT	SH;
	TINT	PG;
	TINT	LC;
	TLONG	DT1;
	TLONG	DT2;
} CSRIO;

/*  
               Handle SR  
*/
/* *************************************************************** */
typedef struct tagHSR
{
#ifdef _NOT_
	char	QU[SBC_QU];
	char    TM[SBC_DT];
#endif
	char    *WordList;
	TLONG	TotCnt;
	TLONG	RecvCnt;
	TLONG   AllocCnt;
	char	**aDocInfo;
} CHSR;

/*                 
               ERROR CODE                  
*/
/* *************************************************************** */
/*   SYSTEM   */
#define ERS_JOB_NONE	(100)
#define ERS_FILE_OPEN	(101)
#define ERS_FILE_READ	(102)
#define ERS_FILE_WRITE	(103)
#define ERS_MEM_LACK	(104)
#define ERS_OBJ_NONE	(105)
#define ERS_SROP_NONE	(106)
#define ERS_OBJ_LACK	(107)
#define ERS_BUF_TOO_SMALL (108)
#define ERS_ORDER_FAULT (109)

/*   CATEGORY(300)   */

/*   DOCUMENT   */
#define ER_DOC_DN		(400)
#define ER_DOC_AT		(401)
#define ER_DOC_OID		(402)
#define ER_DOC_TT		(403)
#define ER_DOC_CMT		(404)
#define ER_DOC_KW		(405)
#define ER_DOC_FD		(406)
#define ER_DOC_FILTER	(407)
#define ER_DOC_DIT		(408)
#define ER_DOC_TOO_SMALL (409)
#define ER_DOC_TOO_BIG   (410)
#define ER_DOC_CT		(411)

/*   WORD   */
#define ERW_NO_INDEX	(500)
#define ER_WORD_INVALID_BK (501)

/*  FUNTION  */
#define ERF_QU_NONE		(600)
#define ERF_QU_TOO_LONG (601)
#define ERF_QU_FAULT	(602)
#define ERF_KW_NONE		(603)

#define ERF_WORD_NONE	(604)
#define ERF_DOC_NONE	(605)
#define ERF_PAGE_FAULT	(606)
#define ERF_SEND_WORD	(607)
#define ERF_BUF_TOO_SMALL	(608)
#define ERF_OID_TOO_LONG	(609)
#define ERF_OID_NONE	(610)
#define ERF_ARG_FAULT	(613)
#define ERF_DENY_LINKCC (614)
#define ERF_DENY_MOVECC (615)
#define ERF_DENY_UNLINKCC (616)
#define ERF_DENY_LINKDC (619)
#define ERF_DENY_UNLINKDC (620)
#define ERF_DENY_LINKDD (617)
#define ERF_DENY_UNLINKDD (618)
#define ERF_CT_NONE	(619)


/*   TCP/IP   */
#define ERT_MASK        (700)
#define ERT_SOCKET		(700)
#define ERT_BIND		(701)
#define ERT_LISTEN		(702)
#define ERT_ACCEPT		(703)
#define ERT_CONNECT		(704)
#define ERT_SEND_DATA	(706)
#define ERT_RECV_DATA	(707)
#define ERT_SEND_FILE	(709)
#define ERT_RECV_FILE	(708)
#define ERT_SEND_FILEOFF	(710)
#define ERT_SEND_OPCD	(711)
#define ERT_RECV_OPCD	(712)
#define ERT_SEND_ACK	(716)
#define ERT_RECV_ACK	(717)

#endif /* _SOFTBOT_ */

/*
** END SOFTBOT.H
*/
