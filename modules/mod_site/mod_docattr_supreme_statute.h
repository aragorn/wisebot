/* $Id$ */
#ifndef MOD_DOCATTR_SUPREME_STATUTE_H
#define MOD_DOCATTR_SUPREME_STATUTE_H 1

#include <string.h>

/* must be 64 byte */
#define DOCATTR_RID_LEN				8
#define SORTING_STRING_LEN			32 
typedef struct {
	uint8_t is_deleted:8;

	uint32_t law_part:32;
	uint8_t history:8;
	uint8_t law_status:8;
	uint8_t law_unit:8;
	uint32_t law_prodate:32;
	uint32_t law_enfodate:32;
	uint16_t jono1:16;
	uint16_t jono2:16;
	uint32_t law_prono:32;
	char law_name[SORTING_STRING_LEN];
	uint8_t rid[DOCATTR_RID_LEN];
} __attribute__((packed)) supreme_statute_attr_t;

/* must smaller than STRING_SIZE(256) byte */
typedef struct {
	uint8_t delete_mark;
	uint8_t undelete_mark;

	uint8_t set_rid;
	char rid[SHORT_STRING_SIZE];

	uint8_t set_law_part;
	uint32_t law_part;

	uint8_t set_law_status;
	uint8_t law_status;

	uint8_t set_history;
	uint8_t history;

	uint8_t set_law_unit;
	uint8_t law_unit;

	uint8_t set_law_prodate;
	uint32_t law_prodate;

	uint8_t set_law_enfodate;
	uint32_t law_enfodate;

	uint8_t set_law_prono;
	uint32_t law_prono;

	uint8_t set_jono1;
	uint32_t jono1;

	uint8_t set_jono2;
	uint32_t jono2;

	uint8_t set_law_name;
	char law_name[SORTING_STRING_LEN];

} supreme_statute_mask_t;

/* must smaller than STRING_SIZE(256) byte */
typedef struct {
	uint8_t delete_check;

	uint8_t law_part_check;
	uint32_t law_part[12];

	uint8_t history_check;
	uint8_t history;

	uint8_t law_unit_check;
	uint8_t law_unit;

	uint8_t law_prodate_check;
	uint32_t law_prodate_start;
	uint32_t law_prodate_finish;

	uint8_t law_enfodate_check;
	uint32_t law_enfodate_start;
	uint32_t law_enfodate_finish;
} supreme_statute_cond_t;

#endif
