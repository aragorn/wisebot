#include "hash.h"

HOOK_STRUCT(
	HOOK_LINK(hash_create)
	HOOK_LINK(hash_destroy)
	HOOK_LINK(hash_open)
	HOOK_LINK(hash_sync)
	HOOK_LINK(hash_close)
	HOOK_LINK(hash_put)
	HOOK_LINK(hash_get)
	HOOK_LINK(hash_count)
)

SB_IMPLEMENT_HOOK_RUN_ALL(int, hash_create, (void** hash), (hash), SUCCESS, DECLINE)
SB_IMPLEMENT_HOOK_RUN_ALL(int, hash_destroy, (void* hash), (hash), SUCCESS, DECLINE)
SB_IMPLEMENT_HOOK_RUN_ALL(int, hash_open, (void* hash, int opt), (hash, opt), SUCCESS, DECLINE)
SB_IMPLEMENT_HOOK_RUN_ALL(int, hash_sync, (void* hash), (hash), SUCCESS, DECLINE)
SB_IMPLEMENT_HOOK_RUN_ALL(int, hash_close, (void* hash), (hash), SUCCESS, DECLINE)
SB_IMPLEMENT_HOOK_RUN_ALL(int, hash_put,
		(void* hash, hash_data_t* key, hash_data_t* value, int opt, hash_data_t* old_value),
		(hash, key, value, opt, old_value), SUCCESS, DECLINE)
SB_IMPLEMENT_HOOK_RUN_ALL(int, hash_get, (void* hash, hash_data_t* key, hash_data_t* value),
		(hash, key, value), SUCCESS, DECLINE) // value_len은 value 버퍼의 길이다.
SB_IMPLEMENT_HOOK_RUN_ALL(int, hash_count, (void* hash, int* count),
		(hash, count), SUCCESS, DECLINE)

