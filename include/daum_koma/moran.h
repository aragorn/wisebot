#ifndef	__MORAN_H__
#define __MORAN_H__
/****************************************************************
*	���ξ� ������� ���� ���Ǿ� �κ� 			*
*			Programmed by Kim, Eung Gyun 		*
*			email : jchern@daumcorp.com			*
****************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#define		RUN		1
/* SPECIAL TAG OR NOT */
#define         NORMAL_ENTRY    0
#define         SPECIAL_ENTRY   1

/* newline ���� */
#define DelNewline(str,size) {if(*(str+(size - 1)) == '\n') *(str+(size - 1)) = '\0';}


/* MAX LINE ��¥�� ������ ó���ϴµ�, �Ѵ� ��� ���� ��� 	*/
#define		MAX_FILE_NAME		128	/* ȭ�� �̸��� ũ�� */
#define		MAX_LINE			1024/* �Է� ���ڿ��� �ִ� ũ�� */
#define		MAX_DICT_REF		128 /*�� ������ 128����Ʈ���� ���� */
#define		MAX_DICT_FIRSTS 	256	/* ù������ ����, 1����Ʈ�� �ִ�� 256 */
#define		MAX_TAG_SET_SIZE 	4096	/* ��Ÿ�ױ��� ���� */
#define		MAX_LRC_SIZE		40	/* ��Ÿ�ױ׳��� ����� ǰ�翭�� �ִ� ���� */
#define		MAX_OUTPUT			4096
#define		MAX_STACK_SIZE		128	/* �� ������ ���¼Ұ� 100������ ���� */
#define		MAX_RESULT			48	/* ���� ������ �����ϱ� ���� �ӽ� ������� slot ����*/

	
/* �ý��ۿ��� ����ϴ� ������ ��Ī�� ���� ���� */
#define         SYSTEM_DICT             "Dict.skm"
#define         USER_DICT               "User.skm"
#define         X_USER_DICT             "Stop.skm"
#define         SPACE_DICT              "Space.dic"
#define         FREQ_DICT               "Freq.skm"
#define         PRE_DICT                "pre.trie"
#define         ALIAS_DICT              "alias.trie"
#define			CONFIG_HANL				"CONFIG.HANL"
#define			DIC_PATH				"/data3/daumsoft/HANL_DEAMON/DICT"

#define			SEP						"\t"
#define		PREDIC			0
#define		ALIASDIC		1
#define		SYSDIC			2
#define		USRDIC			3
/* �Ϲ����� ȭ���� ��� ���� ���� 	*/
#define		EOF			(-1)
#define		EOLN			('\n')
#define		CONT_LN			0

/* �Ϲ����� YES and No�� ���� ����		*/
#define		YES			1
#define		NO			0
#define		TWO_SIDE		2	/* ��, �� ������ �ǹ�. �׷��Ƿ� 2 */

/* �Ϲ����� YES and No�� ���� ����		*/
#define		TRUE			1
#define		FALSE			0

/* �Ϲ����� YES and No�� ���� ����		*/
#define		DECISION		3
#define		ALMOSTDECISION	2

#ifndef		SUCCESS
#	define		SUCCESS			1
#endif

#ifndef		FAIL
#	define		FAIL			0
#endif

/* ������ ���� ����				*/
#define         NORMAL_GRAM_001         1
#define         NAME_GUESS_DONE         2
#define         NORMAL_GRAM_003         3
#define         NORMAL_GRAM_007         4
#define         FAIL_N_GUESSING         5

/* ���ξ��� ���� 				*/
#define		GARBAGY_INDEX		300
#define		BY_POSI_INDEX		301  
#define		BY_JOSA_INDEX		302  
#define		BY_DICT_INDEX		303  
#define		UNKNOWN_INDEX		304  
#define		GUESS_JOSA              10


/*  �� ���¼� �ؼ��⿡�� �ν��ϴ� �ڼ��� �з�....                       */
#define         HANGUL          'H'     
#define         HANJA           'C'     
#define         HANNUM          'N'     
#define         HANENG          'E'     
#define         HANSPC          '_'     
#define         HANETC          '?' 	/* �׷��� ���ڵ�*/    
#define         ALPHABET        'e'     /* a,b,c 	*/
#define			NUMBER			'n'	/* 1,2,3	*/
#define			SPACE			'b'	/* <SPACE>	*/
#define         QUOTE           '"'     /* ', "         */
#define         OPENER          '['     /* [, {, (      */
#define         CLOSER          ']'     /* ], }, )      */
#define         POINT           '.'     /* .            */
#define         BAR           	'-'     /* -            */
#define         PLUS           	'+'     /* +            */
#define         COMMA           ','     /* ,            */
#define         ETC             '@'     /* !, @, #, ... */
#define         EOSTR           'X'

/* ======================================================================
 * KOREAN ��ū Ÿ�� ����
 * ====================================================================== */
#define	T_BL	0x0	  		/* Blank		*/
#define	T_EN	0x1  		/* English		*/
#define	T_DG	0x2  		/* Digit		*/
#define	T_ED	0x3  		/* English & Digit		*/
#define	T_SC	0x4  		/* Special Character	*/
#define	T_KO	0x10  		/* Korean Code		*/
#define	T_HJ	0x20	  	/* Hanja  Code		*/
#define	T_JH	0x40 		/* Japanse Code ����ī��*/
#define	T_JK	0x80		/* Japanse Code ��Ÿ����*/
#define	T_U1	0x100  		/* Unknown(1 byte)	*/
#define	T_U2	0x200  		/* Unknown(2 byte)	*/
#define	T_CK	0x400  		/* Control key		*/
#define	T_BF	0x800  		/* Special Character alike blank (. , " etc) */
#define	T_EX	0x1000		  	/* '\n'			*/


//#undef  MAXTOKENLEN


/* ���ξ�� �����ϴ� ��쿡 ���־�� �ϴ� ó���鿡 ���� ���� */
#define		NO_IDX			0
#define		NOUN_IDX		1
#define		JUBDUSA			2
#define		IDX_JUBMISA		3
#define		JOSA			4
#define		VERB_IDX		5
#define		VERB_IRR		6

/* �ý����� ��ü���� �������� ���Ǹ� ���Ͽ� (���� ���� �����)	*/
#define		BOE				1
#define		EOE				2
#define		NO_WHITE		3
#define		UNOUN_N_TAG		4
#define		UNOUN_Y_TAG		5
#define		UNOUN_L_TAG		6
#define		ENG_N_TAG		7
#define		ENG_Y_TAG		8
#define		ENG_C_TAG		9
#define		NUM_TAG			10
#define		HANJA_TAG		12
#define		UNKNOWN			13
#define		SDICT_TAG_START	20

/* ���¼� �м� ��ġ ���� */
#define		WHERE_PREDICTIONARY		0
#define		WHERE_PREPROCESSING		1
#define		WHERE_NAMEGUESS			2
#define		WHERE_SUCCESS			3
#define		WHERE_GUESS_NOUN		4
#define		WHERE_GUESS_JOSA		5
#define		WHERE_ALIAS_KEYWORD		6
#define		WHERE_GET_TOKEN			7
#define		WHERE_UNKNOWN			8

#define		TAG_UNKNOWN			"unknown"
#define		TAG_NOUN90			"noun?"
#define		TAG_NOUN			"noun"
#define		TAG_NAME90			"name?"
#define		TAG_POST90			"post?"
#define		TAG_JOSA90			"josa?"
#define		TAG_JOSA			"josa"
#define		TAG_GATA			"hira"
#define		TAG_HIRA			"gata"
#define		TAG_ALPHABET		"eng"
#define		TAG_NUMBER			"num"
#define		TAG_EJIDX			"ejidx"

/* ���ξ� ������ ��ó���� ���� ���� */
#define		NOTHING					0	/* (x)																				*/
#define		L_LEFT_APPEND			1	/* (����+[��]) -> �����															*/
#define		L_LEFT_APPEND_ATTACH	2	/* (����+[��]) -> ����+�����														*/
#define		L_RESTORE				3	/* (��) -> �Դ�																		*/

#define		R_RESTORE				1	/* (��) -> (�Դ�)																	*/
#define		R_RIGHT_APPEND			2	/* ([��]+����) -> ������, 		([��]+[��]+�����) -> ���������					*/
#define		R_ATTACH_COMPOUND		3	/* ([��]+����) -> ������, ����, ([��]+[��]+�����) -> ���������+�������+�����	*/

#define		STAND_ALON				1
#define		APPEND_LEFT				2
#define		APPEND_IF_SAME_LEFT		3
#define		RECOVER_FORM			4	/* ǰ�� ����Ʈ */




#define ROW 310890
#define LEN 32


typedef struct DIC_INFO__/* ���� ������ ������� ����ü */
{
	int				tags_count;
	unsigned char 	*Ext_Dict;
	unsigned long   Ext_Dict_Size;
	unsigned long	DICT_FIRST_CHAR_ADDR[MAX_DICT_FIRSTS];
	int		USE_DICT;
}DIC_INFO;


typedef struct TAG_INFO__/* �ܾ ���� ���¼� ���� ����Ʈ�� �����ϱ� ���� ���� */
{
	unsigned int	*L_TAG_LIST[MAX_TAG_SET_SIZE];	
	unsigned int	*R_TAG_LIST[MAX_TAG_SET_SIZE];		
	unsigned int	TAG_SIZE[MAX_TAG_SET_SIZE];	
}TAG_INFO;


typedef struct STACK_INFO__/* ���� �˻��� �߰� ����� �����ϴ� ������ ���� */
{
	int				unknowninfo;

	unsigned int	STACK_POSI_FROM[MAX_STACK_SIZE];
	unsigned int	STACK_POSI_TO[MAX_STACK_SIZE];
	unsigned int	STACK_CIT_NUM[MAX_STACK_SIZE];
	unsigned int	STACK_CIT_LIST[MAX_STACK_SIZE][MAX_LRC_SIZE][TWO_SIDE];
	unsigned int	STACK_CIT_INDEX;
	int             is_special[MAX_STACK_SIZE];
}STACK_INFO;

typedef struct STACKS_INFO__
{
	int				unknown;
	STACK_INFO stack_info;
	STACK_INFO stack_info_backup;
}STACKS_INFO;

typedef struct RESULT_INFO__/* ���ξ� ������ ����� �����ϱ� ���� ����Ÿ ���� ���� */
{
	int	level;		/* �м��Ǿ��� ���� ���� ���� */
	int where;
	int	type;		/* �̵������� ���¼ҿ���(���� ��) */
	int	expand;		/* Ȯ��� ���¼�  ������|����, 1|1ȸ */
	int start;
	int end;
	int	Ltag;
	int	Rtag;
	int	Lidx;
	int	Ridx;
	int	Laction;
	int	Raction;
	char	word[MAX_DICT_REF];	/* ���¼� */
	char	tag [MAX_DICT_REF];	/* ǰ�� ����Ʈ */
}RESULT_INFO;

typedef struct FINAL_INFO__
{
	int	numberoftoken;		/* �м��� ��ū�� �� */
	int	size;
	RESULT_INFO result_info[MAX_STACK_SIZE];
}FINAL_INFO;


struct  token_type 
{
	char    word[MAX_STACK_SIZE];       /* ���¼� */
	int     position;       /* ��ġ */
};

typedef struct  morphem_
{
	char    word[MAX_STACK_SIZE];       /* ���¼� */
	int     tagnum;       /* ��ġ */
}morphem;

typedef	struct	str2str_
{
	char	str1[MAX_DICT_REF];
	char	str2[MAX_DICT_REF];
}STR2STR;

#include "dha.h"

#ifdef __cplusplus
}
#endif

#endif
