/* $Id$ */
#ifndef _MOD_DOCATTR_TYPE1_H_
#define _MOD_DOCATTR_TYPE1_H_ 1

typedef struct {
	uint8_t is_deleted:1;
	uint16_t category:15;
	uint32_t attr1:32; 
	uint32_t attr2:32; 
} __attribute__((packed)) doc_attr1_t;

// FIXME should be asserted that size of doc_attr1_t is equal to DOCATTR_ELEMENT_SIZE

#define DOCATTR_IS_DELETED		compare_function1
int compare_function1(void *dest, void *cond);

#define DOCATTR_DELETE			mask_function1
int mask_function1(void *dest, void *cond);

#endif
