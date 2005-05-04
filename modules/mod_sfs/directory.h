/* $Id$ */

#ifndef __DIRECTORY_H__
#define __DIRECTORY_H__

#include "sfs_types.h"

void dir_init_superblock(super_block_t *sb);
int dir_get_entry(sfs_t* sfs, dir_hash_entry_t* entry);
int dir_add_entry(sfs_t* sfs, dir_hash_entry_t* entry);
int dir_update_entry(sfs_t* sfs, dir_hash_entry_t* entry);

int dir_get_file_array(sfs_t* sfs, int* file_array, int b_sort);
int dir_copy(sfs_t* src, sfs_t* dest);

#endif

