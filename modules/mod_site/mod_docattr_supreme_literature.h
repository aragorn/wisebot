/* $Id$ */
#ifndef MOD_DOCATTR_SUPREME_LITERATURE_H
#define MOD_DOCATTR_SUPREME_LITERATURE_H 1

#define DOCATTR_RID_LEN			8

/* must be 64 bytes */
typedef struct {
	uint8_t is_deleted:1;
	uint8_t type2:3;
	uint8_t lan:4;
	uint8_t part:4; 
	uint8_t rsv2:4;
	uint8_t rid[DOCATTR_RID_LEN];
	uint32_t rsv3:16;
	int32_t date:32; /* --> 16byte */
	char title[16];
	char author[16];
	char pubsrc[16];
} __attribute__((packed)) supreme_literature_attr_t;

/* must be smaller than STRING_SIZE(256) byte */
typedef struct {
	uint8_t delete_check;
	uint8_t date_check;
	int32_t date_start;
	int32_t date_finish;
	uint8_t type2_check;
	uint8_t type2[3];
	uint8_t lan_check;
	uint8_t lan[3];
	uint8_t part_check;
	uint8_t part[3];
} supreme_literature_cond_t;

/* must be smaller than STRING_SIZE(256) byte */
typedef struct {
	uint8_t delete_mark;
	uint8_t undelete_mark;

	uint8_t set_rid;
	char rid[32];

	uint8_t set_ctrlno;
	char ctrlno[32];
	uint8_t set_ctrltype;
	char ctrltype[32];

	uint8_t set_type2;
	uint8_t type2;

	uint8_t set_lan;
	uint8_t lan;

	uint8_t set_part;
	uint8_t part; 

	uint8_t set_date;
	int32_t date;

	uint8_t set_title;
	char title[16];

	uint8_t set_author;
	char author[16];

	uint8_t set_pubsrc;
	char pubsrc[16];
} supreme_literature_mask_t;

#endif
