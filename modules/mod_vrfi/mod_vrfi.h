/* $Id$ */
#ifndef MOD_VRFI_H
#define MOD_VRFI_H 1

//#define VRFI_DEBUG
#undef VRFI_DEBUG

#include "softbot.h"
#include "mod_api/vrfi.h"

#include "header_handle.h"
#include "data_handle.h"

#define DB_STAMP "STAMP:VRF Improved 2002/11/26 -- Jiwon Seo\n"

#ifdef VRFI_DEBUG
#  define show_vrfi_info(this, stream) _show_vrfi_info(this, stream, __FUNCTION__)
#else
#  define show_vrfi_info(this, stream) 
#endif

#define STRING_INFO_SIZE LONG_STRING_SIZE
#define BINARY_INFO_SIZE LONG_STRING_SIZE
#define BINARY_INFO_OFFSET STRING_INFO_SIZE
#define INFO_SIZE (STRING_INFO_SIZE+BINARY_INFO_SIZE)

#define VRFI_SHMID_TABLE_ID 'V'
#define VRFI_CACHED_MEM_ID 'v' /* XXX: depends on 'V' < 'v' */
#define MAX_VRF_SHM_NUM 50
#define NUM_OF_EACH_BUNDLED_KEYS 500000
//#define NUM_OF_EACH_BUNDLED_KEYS 50000

#define MASTER_DATA_SIZE(this) \
	((this)->dbinfo.fixedsize + \
	 sizeof(variable_data_info_t) + sizeof(header_pos_t))

#define VRFI_EACH_SHM_SIZE(this) \
	(MASTER_DATA_SIZE(this) * NUM_OF_EACH_BUNDLED_KEYS)

typedef struct {
	int fixedsize;
	int default_variable_size;
	uint32_t last_unused_key;
	char path[STRING_SIZE];
	char dbstamp[STRING_SIZE];
} dbinfo_t;

typedef struct {
	uint32_t ndata;
} variable_data_info_t;

struct _VariableRecordFile {
	int *shmid_table; /* must be in shared memory. also array size is MAX_VRF_SHM_NUM */
	int shmid_table_id;
	uint8_t attach_flag_table[MAX_VRF_SHM_NUM];
	void *attached_memory_table[MAX_VRF_SHM_NUM];
	uint8_t dirtybit_table[MAX_VRF_SHM_NUM];

	int master_fd;
	dbinfo_t dbinfo;
	header_handle_t *header_handle;
	data_handle_t *data_handle;
	int flags;
};
#endif
