#ifndef _HASH_H_API_
#define _HASH_H_API_

// _HASH_H_ �� platform/unix/hash.h ���� �̹� ���� �ִ�.
// hash_t �� ����������.

#include "softbot.h"

typedef struct _hash_data_t {
	void *data;
	uint32_t size;
	uint32_t data_len;

	uint32_t partial_len;
	uint32_t partial_off;
	uint32_t partial_op;
} hash_data_t;

#define HASH_KEY_EXISTS (-21)
#define HASH_KEY_NOTEXISTS (-22)
#define HASH_BUFFER_SMALL (-23)

SB_DECLARE_HOOK(int, hash_create, (void** hash))
SB_DECLARE_HOOK(int, hash_destroy, (void* hash))
// opt�� config�� option set�� ����Ų�� (wiki ��������)
SB_DECLARE_HOOK(int, hash_open, (void* hash, int opt))
SB_DECLARE_HOOK(int, hash_sync, (void* hash))
SB_DECLARE_HOOK(int, hash_close, (void* hash))

/***********************************************************
 * hash_put
 *  opt : HASH_OVERWRITE - �̹� �ִ� key�� �����.
 *  old_value : HASH_OVERWRITE�� ��츸 ���� value ����. size setting
 *              NULL�� ��� ����.
 *
 * return value
 *  SUCCESS, FAIL
 *  HASH_KEY_EXISTS - HASH_OVERWRITE�� �ƴ� ��츸...
 *                    HASH_OVERWRITE�� �����Ǹ� old_value.size�� ����
 *                    ���� ����� ��·���� �� �� �ִ�.
 ***********************************************************/
#define HASH_OVERWRITE (1)
SB_DECLARE_HOOK(int, hash_put,
		(void* hash, hash_data_t* key, hash_data_t* value, int opt, hash_data_t* old_value))

/***********************************************************
 * hash_get
 *
 * return value
 *  SUCCESS, FAIL, HASH_KEY_NOTEXISTS
 *  SUCCESS�� �ƴѰ�� value.size == 0 �� �ȴ�.
 ***********************************************************/
SB_DECLARE_HOOK(int, hash_get, (void* hash, hash_data_t* key, hash_data_t* value))

SB_DECLARE_HOOK(int, hash_count, (void* hash, int* count))

#endif

