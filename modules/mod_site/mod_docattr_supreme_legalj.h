/* $Id$ */
#ifndef MOD_DOCATTR_SUPREME_COURT_H
#define MOD_DOCATTR_SUPREME_COURT_H 1

#include <string.h>

#define DOCATTR_CASENAME_LEN   (16)
#define DOCATTR_CASENUM2_LEN   ( 8)
#define DOCATTR_NAME_LEN       (10)
#define DOCATTR_OPINION_LEN    (12)
#define DOCATTR_COND_ROW_NUM   (15) /* casekind, partkind, reportgrade */

/* must smaller than 64 byte */
typedef struct {
	uint8_t is_deleted:1;
	uint8_t casekind:7;
	uint8_t partkind:8;
	int32_t reportdate:32; 
	uint8_t reportgrade:8; /* 1 + 1 + 4 + 1 = 7 */
	uint16_t casenum1:16;  /* 7 + 2 = 9 */
	uint32_t casenum3:32;  /* 9 + 4 = 13 */
	char casenum2[DOCATTR_CASENUM2_LEN]; /* ---\  8   */
	char casename[DOCATTR_CASENAME_LEN]; /*    | 16   */
	char name[DOCATTR_NAME_LEN];         /* ---' 10   */
                                         /*    ->34   13 + 34 = 47 */
	char rsv2[17];
} __attribute__((packed)) supreme_legalj_attr_t;


/* must smaller than STRING_SIZE(256) byte */
typedef struct {
	uint8_t delete_check;

	uint8_t casekind_check;
	uint8_t casekind[DOCATTR_COND_ROW_NUM];

	uint8_t partkind_check;
	uint8_t partkind[DOCATTR_COND_ROW_NUM];

	uint8_t reportdate_check;
	int32_t reportdate_start;
	int32_t reportdate_finish;

	uint8_t reportgrade_check;
	uint8_t reportgrade[DOCATTR_COND_ROW_NUM];
} supreme_legalj_cond_t;

/* must smaller than STRING_SIZE(256) byte */
typedef struct {
	uint8_t delete_mark;
	uint8_t undelete_mark;

	uint8_t set_casekind;
	uint8_t casekind;

	uint8_t set_partkind;
	uint8_t partkind;

	uint8_t set_reportdate;
	int32_t reportdate;

	uint8_t set_reportgrade;
	uint8_t reportgrade;

	uint8_t set_casename;
	char casename[DOCATTR_CASENAME_LEN];

	uint8_t set_casenum;
	uint16_t casenum1;
	char     casenum2[DOCATTR_CASENUM2_LEN];
	uint32_t casenum3;

	uint8_t set_name;
	char name[DOCATTR_NAME_LEN];


} supreme_legalj_mask_t;

#endif
