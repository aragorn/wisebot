/* $Id$ */
#ifndef _MOD_DOCATTR_TYPE3_H_
#define _MOD_DOCATTR_TYPE3_H_ 1

typedef struct {
	uint8_t is_deleted:1;
	uint16_t rsv:15;
	uint32_t registered_time:32; 
	uint32_t created_time:32; 
} __attribute__((packed)) doc_attr3_t;

typedef struct {
	uint8_t delete_check;
	uint8_t registered_time_check;
	uint8_t registered_time_check_type;
#define EQUAL_TO			100
#define EARLIER_THAN		101
#define LATER_THAN			102

	time_t registered_pivot_time;
} doc_cond3_t;

typedef struct {
	uint8_t delete_mark;
	uint8_t registered_time_mark;
	time_t registered_time;
	uint8_t created_time_mark;
	time_t created_time;
} doc_mask3_t;

// FIXME should be asserted that size of doc_attr1_t is equal to DOCATTR_ELEMENT_SIZE

#define DOCATTR_SET_ZERO(a) bzero(&(a), sizeof(doc_attr3_t))
#define DOCCOND_SET_ZERO(a) bzero(&(a), sizeof(doc_cond3_t))
#define DOCMASK_SET_ZERO(a) bzero(&(a), sizeof(doc_mask3_t))

#define SC_COMP compare_function3
int compare_function3(void *dest, void *cond);

#define SC_MASK mask_function3
int mask_function3(void *dest, void *mask);

#define SORTBY_REGIST_TIME compare_function_for_qsort3_sort_by_registered_time
int compare_function_for_qsort3_sort_by_registered_time(const void *, const void *);

#endif
