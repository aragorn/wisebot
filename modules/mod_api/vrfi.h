/* $Id$ */
#ifndef VRFI_H
#define VRFI_H 1

#warning *** Using vrfi.h is deprecated as of 2006/05/30. Please use ifs instead. ***

#include <stdint.h> /* uint32_t */

typedef struct VariableRecordFile VariableRecordFile;

SB_DECLARE_HOOK(int, vrfi_alloc,(VariableRecordFile **this))
SB_DECLARE_HOOK(int, vrfi_open,(VariableRecordFile *this, \
			char filepath[], int fixedsize, int default_variable_size, int flags))
SB_DECLARE_HOOK(int, vrfi_close, (VariableRecordFile *this))
SB_DECLARE_HOOK(int, vrfi_sync, (VariableRecordFile *this))

SB_DECLARE_HOOK(int, vrfi_set_fixed,(VariableRecordFile *this, \
					uint32_t key, void *data))
SB_DECLARE_HOOK(int, vrfi_get_fixed_dup,(VariableRecordFile *this, \
					uint32_t key, void *data))
SB_DECLARE_HOOK(int, vrfi_get_fixed,(VariableRecordFile *this, \
					uint32_t key, void **data))

SB_DECLARE_HOOK(int, vrfi_append_variable,(VariableRecordFile *this, \
					uint32_t key, uint32_t nelm, void *data))
SB_DECLARE_HOOK(int, vrfi_get_variable,(VariableRecordFile *this, \
					uint32_t key, uint32_t skip, uint32_t nelm, void *data))
SB_DECLARE_HOOK(int, vrfi_get_num_of_data,(VariableRecordFile *this, \
					uint32_t key, uint32_t *nelm))
#endif
