#ifndef _HASH_H_API_
#define _HASH_H_API_

// _HASH_H_ 는 platform/unix/hash.h 에서 이미 쓰고 있다.
// hash_t 도 마찬가지다.

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
// opt는 config의 option set을 가리킨다 (wiki 참고하자)
SB_DECLARE_HOOK(int, hash_open, (void* hash, int opt))
SB_DECLARE_HOOK(int, hash_sync, (void* hash))
SB_DECLARE_HOOK(int, hash_close, (void* hash))

/***********************************************************
 * hash_put
 *  opt : HASH_OVERWRITE - 이미 있는 key를 덮어쓴다.
 *  old_value : HASH_OVERWRITE인 경우만 원래 value 복사. size setting
 *              NULL인 경우 무시.
 *
 * return value
 *  SUCCESS, FAIL
 *  HASH_KEY_EXISTS - HASH_OVERWRITE가 아닌 경우만...
 *                    HASH_OVERWRITE가 설정되면 old_value.size를 보고
 *                    덮어 썼는지 어쨌는지 알 수 있다.
 ***********************************************************/
#define HASH_OVERWRITE (1)
SB_DECLARE_HOOK(int, hash_put,
		(void* hash, hash_data_t* key, hash_data_t* value, int opt, hash_data_t* old_value))

/***********************************************************
 * hash_get
 *
 * return value
 *  SUCCESS, FAIL, HASH_KEY_NOTEXISTS
 *  SUCCESS가 아닌경우 value.size == 0 이 된다.
 ***********************************************************/
SB_DECLARE_HOOK(int, hash_get, (void* hash, hash_data_t* key, hash_data_t* value))

SB_DECLARE_HOOK(int, hash_count, (void* hash, int* count))

#endif

