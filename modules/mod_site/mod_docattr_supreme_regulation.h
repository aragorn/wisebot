/* $Id$ */
#ifndef MOD_DOCATTR_SUPREME_REGULATION_H
#define MOD_DOCATTR_SUPREME_REGULATION_H 1

#include <string.h>

/* must be 64 byte */
typedef struct {
	uint8_t is_deleted:8;

	uint8_t gubun:8;
	uint8_t history:8;
	uint32_t enactdate:32;
	uint32_t enfodate:32; /* --> 11 byte */
	char title[16];
	uint32_t ruleno1:32;
	uint32_t ruleno2:32;
	uint8_t ctrltype:8; /* --> 36 byte */
	uint8_t rsv[28];
} __attribute__((packed)) supreme_regulation_attr_t;

/* must smaller than STRING_SIZE(256) byte */
typedef struct {
	uint8_t delete_mark;
	uint8_t undelete_mark;

	uint8_t set_gubun;
	uint8_t gubun;

	uint8_t set_history;
	uint8_t history;

	uint8_t set_enactdate;
	uint32_t enactdate;

	uint8_t set_enfodate;
	uint32_t enfodate;

	uint8_t set_title;
	char title[16];

	uint8_t set_ruleno1;
	uint32_t ruleno1;

	uint32_t set_ruleno2;
	uint32_t ruleno2;

	uint8_t set_ctrltype;
	uint8_t ctrltype;
} supreme_regulation_mask_t;

/* must smaller than STRING_SIZE(256) byte */
typedef struct {
	uint8_t delete_check;

	uint8_t gubun_check;
	uint8_t gubun[12];

	uint8_t history_check;
	uint8_t history;

	uint8_t enactdate_check;
	uint32_t enactdate_start;
	uint32_t enactdate_finish;

	uint8_t enfodate_check;
	uint32_t enfodate_start;
	uint32_t enfodate_finish;
} supreme_regulation_cond_t;

#endif
