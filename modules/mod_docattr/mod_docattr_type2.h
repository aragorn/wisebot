/* $Id$ */
#ifndef _MOD_DOCATTR_TYPE2_H_
#define _MOD_DOCATTR_TYPE2_H_ 1

typedef struct {
	uint8_t is_deleted:1;
	uint16_t category:15;
	uint32_t attr1:32; 
	uint32_t attr2:32; 
} __attribute__((packed)) doc_attr2_t;

#define MAX_MULTI_SEARCH_CATEGORY_NUM			10

typedef struct {
	uint8_t delete_check;
	uint8_t category_search_type;
	uint8_t category_ids[MAX_MULTI_SEARCH_CATEGORY_NUM];
	uint8_t category_num;
} doc_cond2_t;

#define SEARCH_THIS_CATEGORY		1
#define SEARCH_BELOW_CATEGORY		2
#define SEARCH_MULTI_CATEGORY		3

typedef struct {
	uint8_t delete_mark;
} doc_mask2_t;

// FIXME should be asserted that size of doc_attr1_t is equal to DOCATTR_ELEMENT_SIZE

int compare_function2(void *dest, void *cond);

int mask_function2(void *dest, void *cond);

#endif

