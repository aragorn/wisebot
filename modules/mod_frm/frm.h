#include "softbot.h"
		
typedef struct mmap_file_t mmap_file_t;

SB_DECLARE_HOOK(int, frm_open, (char path[], mmap_file_t *mmap_file, int data_size))
SB_DECLARE_HOOK(int, frm_close, (mmap_file_t *mmap_file))
SB_DECLARE_HOOK(int, frm_read, (mmap_file_t *mmap_file, int key, void** data))
SB_DECLARE_HOOK(int, frm_add, (mmap_file_t *mmap_file, int key, void* data))
