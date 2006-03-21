/* $Id$ */
#ifndef MOD_DOCATTR_SUPREME_COURT_H
#define MOD_DOCATTR_SUPREME_COURT_H 1

#include <string.h>

#define DOCATTR_RID_LEN			6

/* must smaller than 64 byte */
typedef struct {
	uint8_t is_deleted:1;
	uint8_t courttype:2;
	uint8_t lawtype:5;
	uint8_t gan:2; 
	uint8_t won:2; 
	uint8_t del:2; 
	uint8_t close:2; 
	uint32_t court;
	int32_t pronouncedate; 
	uint16_t casenum1;
	uint32_t casenum3;
	char casenum2[16];
	char casename[16];
	uint8_t miganopen:2; // 2006/02 미간행 공개판결
	uint8_t rsv:6;
	char rsv2[15];
} __attribute__((packed)) supreme_court_attr_t;

#define MAX_LAWTYPE_NUM		8
typedef struct _supreme_row{
	uint8_t courttype;
	uint8_t gan; 
	uint8_t won; 
	uint8_t miganopen;
	uint8_t del; 
	uint8_t close; 
	uint32_t court;
	uint8_t nlawtype;
	uint8_t lawtype[MAX_LAWTYPE_NUM];
	uint8_t outall; // 외부종법전체. gan | won | miganopen
} supreme_row;

#define MAX_CATEGORY_ROWS	3
/* must smaller than STRING_SIZE(256) byte */
typedef struct {
	uint8_t delete_check;
	uint8_t rownum;
	supreme_row rows[MAX_CATEGORY_ROWS];
	uint8_t pronouncedate_check;
	int32_t pronouncedate_start;
	int32_t pronouncedate_finish;
} supreme_court_cond_t;

/* must smaller than STRING_SIZE(256) byte */
typedef struct {
	uint8_t delete_mark;
	uint8_t undelete_mark;

	uint8_t set_rid;
	char rid[SHORT_STRING_SIZE];

	uint8_t set_casename;
	char casename[16];

	uint8_t set_court;
	uint32_t court;

	uint8_t set_courttype;
	uint8_t courttype;

	uint8_t set_lawtype;
	uint8_t lawtype;

	uint8_t set_gan;
	uint8_t gan; 

	uint8_t set_won;
	uint8_t won; 

	uint8_t set_miganopen;
	uint8_t miganopen;

	uint8_t set_del;
	uint8_t del; 

	uint8_t set_close;
	uint8_t close; 

	uint8_t set_pronouncedate;
	int32_t pronouncedate; 

	uint8_t set_casenum1;
	uint16_t casenum1;

	uint8_t set_casenum3;
	uint32_t casenum3;

	uint8_t set_casenum2;
	char casenum2[16];
} supreme_court_mask_t;

#endif
