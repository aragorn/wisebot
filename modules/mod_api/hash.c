#include "hash.h"

HOOK_STRUCT(
	HOOK_LINK(hash_open)
	HOOK_LINK(hash_sync)
	HOOK_LINK(hash_close)
	HOOK_LINK(hash_put)
	HOOK_LINK(hash_get)
	HOOK_LINK(hash_count)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, hash_open, (api_hash_t** hash, int opt), (hash, opt), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, hash_sync, (api_hash_t* hash), (hash), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, hash_close, (api_hash_t* hash), (hash), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, hash_put,
		(api_hash_t* hash, hash_data_t* key, hash_data_t* value, int opt, hash_data_t* old_value),
		(hash, key, value, opt, old_value), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, hash_get, (api_hash_t* hash, hash_data_t* key, hash_data_t* value),
		(hash, key, value), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, hash_count, (api_hash_t* hash, int* count),
		(hash, count), DECLINE)

