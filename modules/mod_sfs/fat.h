/* $Id$ */

#ifndef __FAT_H__
#define  __FAT_H__

#include "sfs_types.h"

void fat_init_superblock(super_block_t *sb);
int fat_allocate(sfs_t* sfs, fat_entry_t* entry, fat_entry_t* next_entry, int before_block_num, int* alloc_count);
int fat_get_next_entry(sfs_t* sfs, int fat_entry_num, fat_entry_t* next_entry);
int fat_get_entry(sfs_t* sfs, inode_t* inode, int block_num, fat_entry_t* entry);

#endif

