#ifndef MOD_DOCATTR_SUPREME_HOME_H
#define MOD_DOCATTR_SUPREME_HOME_H

#include <string.h>

/**************************************************
 * 여기 구조체들은 만들때 4byte align을 지키줘야
 * solaris에서 문제가 생기지 않는다.
 **************************************************/

#define DOCATTR_RID_LEN (8)

/* XXX MUST BE OF 64 BYTES */
typedef struct {
	uint32_t is_deleted;
	char title[16];
	int date;
	int int1;
	int int2;
	char bit1;
	char bit2;
	int cate;

	uint64_t rid1;
	uint64_t rid2;
	uint64_t rid3;

	// 4 + 16 + 4 + 4 + 4 + 1 + 1 + 4 + 8 + 8 + 8= 58
	uint8_t rsv[2];

} __attribute__((packed)) test_attr_t;

/* must smaller than STRING_SIZE(256) byte */
typedef struct {
	uint32_t delete_check;

	int date_check;
	int date;

	int int1_check;
	int int1;

	int int2_check;
	int int2;

	int bit1_check;
	int bit1;

	int bit2_check;
	int bit2;

	int cate_check;
	int cate;
} test_cond_t;   /* 검색시 조건 입력 */

/* must smaller than STRING_SIZE(256) byte */
typedef struct {
	uint32_t delete_mark;
	uint32_t undelete_mark;

	uint32_t set_title;
	char title[16];

	int set_date;
	int date;

	int set_int1;
	int int1;

	int set_int2;
	int int2;

	int set_bit1;
	char bit1;

	int set_bit2;
	char bit2;

	int set_cate;
	int cate;

	int set_rid1;
	char rid1[32];

	int set_rid2;
	char rid2[32];

	int set_rid3;
	char rid3[32];
} test_mask_t;

#endif
