/* $Id$ */
#include "softbot.h"
#include "vrfi.h"

HOOK_STRUCT(
	HOOK_LINK(vrfi_alloc)
	HOOK_LINK(vrfi_open)
	HOOK_LINK(vrfi_close)
	HOOK_LINK(vrfi_sync)
	HOOK_LINK(vrfi_set_fixed)
	HOOK_LINK(vrfi_get_fixed)
	HOOK_LINK(vrfi_get_fixed_dup)
	HOOK_LINK(vrfi_append_variable)
	HOOK_LINK(vrfi_get_variable)
	HOOK_LINK(vrfi_get_num_of_data)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, vrfi_alloc, \
	(VariableRecordFile **this), (this), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, vrfi_open, \
	(VariableRecordFile *this,\
	 char filepath[],int fixedsize,int default_variable_size, int flags), \
	(this, filepath, fixedsize, default_variable_size, flags), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, vrfi_close, \
	(VariableRecordFile *this), (this), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, vrfi_sync, \
	(VariableRecordFile *this), (this), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, vrfi_set_fixed, \
	(VariableRecordFile *this, uint32_t key, void *data), \
	(this,key,data), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, vrfi_get_fixed_dup, \
	(VariableRecordFile *this, uint32_t key, void *data), \
	(this,key,data), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, vrfi_get_fixed, \
	(VariableRecordFile *this, uint32_t key, void **data), \
	(this,key,data), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, vrfi_append_variable, \
	(VariableRecordFile *this, uint32_t key, uint32_t nelm, void *data),\
	(this,key,nelm,data),DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, vrfi_get_variable, \
	(VariableRecordFile *this, uint32_t key, uint32_t skip, uint32_t nelm, void *data),\
	(this,key,skip,nelm,data),DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, vrfi_get_num_of_data, \
	(VariableRecordFile *this, uint32_t key, uint32_t *nelm) ,\
	(this, key, nelm), DECLINE)
