/* $Id $ */
#ifndef DOCABSTRACT_H
#define DOCABSTRACT_H 1

//#include "softbot.h"


#define MAX_EACH_FIELD_COMMENT_SIZE		(50)

#define MAX_ABSTRACT_INDEX_FIELD_NUM	(3)
#define MAX_ABSTRACT_ENTIRE_FIELD_NUM	(2)
#define MAX_ABSTRACT_FIELD_NUM   (MAX_ABSTRACT_INDEX_FIELD_NUM + MAX_ABSTRACT_ENTIRE_FIELD_NUM)
#define MAX_QP_WANT_ABSTRACT_POS_NUM	(10)
#define MAX_INDEX_SAVE_ABSTRACT_POS_NUM (10)
// get abstract string input (query send)
typedef struct {
	uint32_t start_word_pos;
	uint32_t finish_word_pos;
} abstract_qp_pos_t;

typedef struct {
	int field_id;
    int      abstract_pos_num;
    abstract_qp_pos_t abstract_pos_info[MAX_QP_WANT_ABSTRACT_POS_NUM];
} abstract_field_info_t;


typedef struct _abstract_info_t {
	int	abstract_field_info_num;
	abstract_field_info_t info[MAX_ABSTRACT_FIELD_NUM];
} abstract_info_t;

// doc abstract position  (indexer save)
typedef struct {
    uint32_t wordpos;
    uint32_t bytepos;
} abstract_pos_t;

typedef struct {
    uint16_t fieldid;
    abstract_pos_t pos[MAX_INDEX_SAVE_ABSTRACT_POS_NUM];
} doc_abstract_pos_attr_t;

typedef struct {
    uint32_t doc_header_size;
	//XXX change this name field..	
	doc_abstract_pos_attr_t field[MAX_ABSTRACT_INDEX_FIELD_NUM];
} fixed_attr_t;

#endif
