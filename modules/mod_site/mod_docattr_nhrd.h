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
	uint32_t Cate1;	
	uint32_t Cate2;

	// 4 + 8 + 16 + 16 + 4 + 4 + 4 = 56

	char rsv2[8];
} __attribute__((packed)) nhrd_attr_t;

#define MAX_CATE1 (6)
#define MAX_CATE2 (30)

/* must smaller than STRING_SIZE(256) byte */
typedef struct {
	uint32_t delete_check;

	uint32_t Date_check;
	uint32_t Date_start;
	uint32_t Date_finish;

	// cate1 로 통합검색
	uint32_t Cate1_count;
	uint32_t Cate1Sum_check;
	uint32_t Cate1Sum[MAX_CATE1]; // [0]은 전체 합계로 사용하자, 1~4

	// cate2 로 통합검색
	uint32_t Cate2_count; // cate2를 몇개씩 출력할 것인가 하는...
	uint32_t Cate2Sum_check;
	uint32_t Cate2Sum[MAX_CATE2]; // 1~29 만 저장할 수 있으면 된다.

	uint32_t Cate1_check;
	uint32_t Cate1;

	uint32_t Cate2_check;
	uint32_t Cate2;

} nhrd_cond_t;   /* 검색시 조건 입력 */


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

	uint32_t set_Date;
	uint32_t Date;

	uint32_t set_Cate1;
	uint32_t Cate1;	

	uint32_t set_Cate2;
	uint32_t Cate2;
} nhrd_mask_t;
 

#endif
