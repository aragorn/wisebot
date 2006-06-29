/* $Id$ */
#include "common_core.h"
#include "stack.h"
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
	int offset;
	int size;
} _slot_t;

struct _xmlparser_stack_t {
	int usedsize;
	int top;

	int allocatedsize;
	int maxtop;
	void *db;
	_slot_t *slots;
	int expandable;
};

#define DEFAULT_STACK_DB_SIZE			4096
#define DEFAULT_DEPTH					32
#define EXPAND_STACK_DB_SIZE			4096
#define EXPAND_DEPTH					32

static int expandstack(xmlparser_stack_t *st);
static int expandslot(xmlparser_stack_t *st);

xmlparser_stack_t *st_create()
{
	xmlparser_stack_t *st;
	st = (xmlparser_stack_t *)sb_malloc(sizeof(xmlparser_stack_t));
	if (st == NULL) {
		return NULL;
	}
	st->usedsize = 0;
	st->db = sb_malloc(DEFAULT_STACK_DB_SIZE);
	if (st->db == NULL) {
		sb_free(st);
		return NULL;
	}
	st->allocatedsize = DEFAULT_STACK_DB_SIZE;

	st->slots = sb_malloc(DEFAULT_DEPTH * sizeof(_slot_t));
	if (st->slots == NULL) {
		sb_free(st->db);
		sb_free(st);
		return NULL;
	}
	st->maxtop = DEFAULT_DEPTH;

	st->top = -1;
	st->expandable = 1;

	return st;
}

xmlparser_stack_t *st_create2(void *p, int size)
{
	xmlparser_stack_t *st;
	st = (xmlparser_stack_t *)sb_malloc(sizeof(xmlparser_stack_t));
	if (st == NULL) {
		return NULL;
	}
	st->usedsize = 0;

	if (p == NULL) {
		sb_free(st);
		return NULL;
	}
	st->db = p;
	st->allocatedsize = size;

	st->slots = sb_malloc(DEFAULT_DEPTH * sizeof(_slot_t));
	if (st->slots == NULL) {
		sb_free(st);
		return NULL;
	}
	st->maxtop = DEFAULT_DEPTH;

	st->top = -1;
	st->expandable = 0;

	return st;
}

void st_destroy(xmlparser_stack_t *st)
{
	if (st == NULL) return;
	if (st->slots) sb_free(st->slots);
	if (st->db && st->expandable) sb_free(st->db);
	sb_free(st);
}

int st_pop(xmlparser_stack_t *st, void **ptr, int *len)
{
	void *tmp;
	if (st->top == -1) {
		return -1;
	}
	if (ptr == NULL) {
		return -1;
	}
	tmp = sb_malloc(st->slots[st->top].size);
	if (tmp == NULL) {
		return -1;
	}
	if (st->slots[st->top].size > 0) {
		memcpy(tmp, st->db + st->slots[st->top].offset, st->slots[st->top].size);
		*ptr = tmp;
		*len = st->slots[st->top].size;
	}
	else {
		*ptr = NULL;
		*len = 0;
	}

	st->usedsize -= st->slots[st->top].size;
	st->top--;
	return 1;
}

int st_pop2(xmlparser_stack_t *st, void **ptr, int *len)
{
	if (st->top == -1) {
		return -1;
	}
	if (ptr == NULL) {
		return -1;
	}
	if (st->slots[st->top].size > 0) {
		*ptr = st->db + st->slots[st->top].offset;
		*len = st->slots[st->top].size;
	}
	else {
		*ptr = NULL;
		*len = 0;
	}

	st->usedsize -= st->slots[st->top].size;
	st->top--;
	return 1;
}

int st_push(xmlparser_stack_t *st, void *data, int len)
{
	if (st->top + 1 >= st->maxtop) {
		if (expandslot(st) == -1) {
			return -1;
		}
	}
	st->top++;

	while (st->usedsize + len > st->allocatedsize) {
		if (st->expandable == 0) {
			return -1;
		}
		if (expandstack(st) == -1) {
			return -1;
		}
	}

	if (data) {
		memcpy(st->db + st->usedsize, data, len);
		st->slots[st->top].offset = st->usedsize;
		st->slots[st->top].size = len;
		st->usedsize += len;
	}
	else {
		st->slots[st->top].offset = st->usedsize;
		st->slots[st->top].size = 0;
	}

	return 1;
}

int st_append(xmlparser_stack_t *st, void *data, int len)
{
	while (st->usedsize + len > st->allocatedsize) {
		if (st->expandable == 0) {
			return -1;
		}
		if (expandstack(st) == -1) {
			return -1;
		}
	}

	if (data) {
		memcpy(st->db + st->usedsize, data, len);
		st->slots[st->top].size += len;
		st->usedsize += len;
	}

	return 1;
}

static int expandstack(xmlparser_stack_t *st)
{
	void *tmp;
	tmp = sb_realloc(st->db, st->allocatedsize + EXPAND_STACK_DB_SIZE);
	if (tmp == NULL) {
		return -1;
	}
	st->db = tmp;
	st->allocatedsize += EXPAND_STACK_DB_SIZE;
	return 1;
}

static int expandslot(xmlparser_stack_t *st)
{
	_slot_t *tmp;
	tmp = (_slot_t *)sb_realloc(st->slots, (st->maxtop + EXPAND_DEPTH) * sizeof(_slot_t));
	if (tmp == NULL) {
		return -1;
	}
	st->slots = tmp;
	st->maxtop += EXPAND_DEPTH;
	return 1;
}
