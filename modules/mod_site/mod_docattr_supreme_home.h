#ifndef MOD_DOCATTR_SUPREME_HOME_H
#define MOD_DOCATTR_SUPREME_HOME_H

#include <string.h>

#define DOCATTR_RID_LEN (8)

/* XXX MUST BE OF 64 BYTES */
typedef struct {
	uint32_t is_deleted;
	char Rid[DOCATTR_RID_LEN];

	char Title[16];
	char Author[16];

	uint32_t Date;
	uint32_t System;	
	uint32_t FileNo;

	// 4 + 8 + 16 + 16 + 4 + 4 + 4 = 56

	char rsv2[8];
} __attribute__((packed)) supreme_home_attr_t;

#define MAX_SYSTEM (8)

/* must smaller than STRING_SIZE(256) byte */
typedef struct {
	uint32_t delete_check;

	uint32_t System_check;	
	uint32_t System;	

	uint32_t Date_check;
	uint32_t Date_start;
	uint32_t Date_finish;

	uint32_t SystemSum_check;
	uint32_t SystemSum[MAX_SYSTEM]; // 1~6 �� ������ �� ������ �ȴ�.

	uint32_t Type_check;
	uint32_t Type; // 0(all), 1(doc):���� ������, 2(file):÷������ ������
} supreme_home_cond_t;   /* �˻��� ���� �Է� */


/* must smaller than STRING_SIZE(256) byte */
typedef struct {
	uint32_t delete_mark;
	uint32_t undelete_mark;

	uint32_t set_Rid;
	char Rid[60];     // WG_MBOARD_TBL_LOG;SEQNUM:xxxxxxxx;SUBSEQNUM:xxxxxxxx

	uint32_t set_Title;
	char Title[16];

	uint32_t set_Author;
	char Author[16];

	uint32_t set_System;
	uint32_t System;	

	uint32_t set_Date;
	uint32_t Date;

	uint32_t set_FileNo;
	uint32_t FileNo;
} supreme_home_mask_t;
 

#endif
