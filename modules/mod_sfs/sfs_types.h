#ifndef _SFS_TYPES_H_
#define _SFS_TYPES_H_

#include "softbot.h"

#define SFS_MAGIC "SFS0"
 
#define O_UNLOAD         0
#define O_MMAP           1
#define O_FILE           2
    
#define O_FAT            1
#define O_ARRAY_ROOT_DIR 2
#define O_HASH_ROOT_DIR  4
    
#define SFS_BEGIN       SEEK_SET
#define SFS_CURRENT     SEEK_CUR
#define SFS_END         SEEK_END
    
#define MAX_BLOCK_SIZE (4096)
#define SUPER_BLOCK_NUM 0   //must be start zero
#define SUPER_BLOCK_COUNT 2

typedef struct _super_block_t {
    char magic[4];

    int start_fat_block;
    int end_fat_block;

    int start_file_block;
    int end_file_block;

    int start_dir_block;
    int end_dir_block;

    int min_file_id;
    int max_file_id;
    int file_count;
    int file_total_byte;

    int option;
    int size;
    int block_count;
    int block_size;

    /* directory */
    int entry_count_in_block;
    int dir_block_count;

    /* fat */
    int free_block_num;
} super_block_t;

/* 반드시 32 bit가 되어야 한다. */
#define COUNT_MAX_BIT 30
#define NUM_MAX_BIT 30

typedef struct _fat_entry_t {
    unsigned int count;
    unsigned int num;
} fat_entry_t;

typedef struct _dir_hash_entry_t{
    uint32_t id;
    uint32_t size;
    int first_block_num;
    int last_block_num;
} dir_hash_entry_t;

// 사용하지 않는다?
typedef struct _dir_array_entgry_t {
    uint32_t size;
    fat_entry_t* first_fat_entry;
} dir_array_entry_t;

typedef struct _sfs_t {
    int fd;
    int seq;
    int type;

    int base_offset;
    char* base_ptr;

    super_block_t* super_block;
} sfs_t;

typedef struct _sfs_entry_info_t {
    int file_size;
} sfs_entry_info_t;

typedef struct _sfs_info_t {
    int min_file_id;
    int max_file_id;
    int file_count;
    sfs_entry_info_t sfs_entry_info;
} sfs_info_t;

typedef struct _inode_t {
    dir_hash_entry_t dir_entry;
    fat_entry_t curr_fat_entry;
    int sfs_block_num;

    int offset_in_sfs_blocks; // <= end_of_sfs_blocks
    int end_of_sfs_blocks;    // count, not index

    int file_size_in_sfs_blocks;
} inode_t;

#endif

