/* $Id$ */

#ifndef __SUPER_BLOCK_H__
#define __SUPER_BLOCK_H__

#include "sfs_types.h"

int superblock_add_file_count(super_block_t* s, int file_id);
int superblock_update_write_byte(super_block_t* s, int write_byte);
void superblock_view(super_block_t * s);
#endif
