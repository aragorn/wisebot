/* $Id$ */
#ifndef MOD_DOCATTR_MOMAF_H
#define MOD_DOCATTR_MOMAF_H 1

#include <string.h>

/* XXX MUST BE OF 64 BYTES */
typedef struct {
	uint8_t is_deleted;
	uint8_t FD11;	
	uint32_t Date;

	char Title[16];
	char Dept[16];
	char Author[16];    /* 1 + 4 + 16 + 16 + 16 = 5 + 48 = 53 */
	char rsv2[10];
} __attribute__((packed)) momaf_attr_t;


/* must smaller than STRING_SIZE(256) byte */
typedef struct {
	uint8_t delete_check;
	uint8_t FD11_check;	
	uint8_t Date_check;
	uint8_t FD11;	
	uint32_t Date_start;
	uint32_t Date_finish;

} momaf_cond_t;   /* 검색시 조건 입력 */


/* must smaller than STRING_SIZE(256) byte */
typedef struct {
	uint8_t delete_mark;
	uint8_t undelete_mark;

	uint8_t set_FD11;
	uint8_t FD11;	

	uint8_t set_Date;
	uint32_t Date;

	uint8_t set_Title;
	char Title[16];
	
	uint8_t set_Dept;
	char Dept[16];

	uint8_t set_Author;
	char Author[16];

} momaf_mask_t;
 

#endif
